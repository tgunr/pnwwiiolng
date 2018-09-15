#include "syscfg.h" // Path is project specific.
// This module provides access to the socket
// functions used by the teul API. The end-user developer
// should never need the functions provided here.
//
// Copyright 1999 Playnet Inc, All rights reserved
// Not to be distributed in API form except as part of a compiled
// library
//
// Originating Programmer: Martin Poulin

#include "mem/mem.h"
#include "server/serverInterfaceDescription.h"
#include "server/serverServerInfo.h"
#include "teul/teulBasic.h"
#include "teul/teulCallbackStruct.h"
#include "teul/teulConstants.h"
#include "teul/teulEndpoint.h"
#include "teul/teulEndpointImpl.h"
#include "teul/teulEndpointIdleList.h"
#include "teul/teulModule.h"
#include "teul/teulModCalls.h"
#include "teul/teulPackers.h"
#include "teul/teulPacket.h"
#include "teul/teulPacketHeader.h"
#include "teul/teulTickSample.h"
#include "teul/teulTime.h"
#include "wwii/wwiiConfig.h"
#include "wwii/wwiiNetPorts.h"

using namespace Teul;

static constexpr const TeulTimeMS ENDPOINT_IDLE_INTERVAL = 500;

struct DATA_GRAM
{
    static constexpr const size_t NORMAL_ROUNDING = 256;   // round datagram sizes up to this.
    static_assert((NORMAL_ROUNDING & (NORMAL_ROUNDING - 1)) == 0, "pooled datagram rounding must be power of 2");

    static constexpr const size_t HUGE_ROUNDING = 4096;    // round uncached datagram sizes up to this.
    static_assert((HUGE_ROUNDING & (HUGE_ROUNDING - 1)) == 0, "unpooled datagram rounding must be power of 2");

    static constexpr const size_t NUM_DATAGRAM_SIZES = 16;
    static_assert((NUM_DATAGRAM_SIZES & (NUM_DATAGRAM_SIZES - 1)) == 0, "nuber of pooled datagram buckets must be power of 2");

    static constexpr const size_t LARGEST_POOLED_SIZE = NUM_DATAGRAM_SIZES * NORMAL_ROUNDING;
    static_assert((LARGEST_POOLED_SIZE & (LARGEST_POOLED_SIZE - 1)) == 0, "calculated max pooling size must be power of 2");

    struct sockaddr_in    sockAddress;
    Teul::PacketHeader*   contents;    //!< Points to the beginning of the packet data.
    UINT32                m_size;      //!< Size of the entire object (might be smaller than allocated size)
    UINT32                m_reqSize;   //!< Original requested size of bucket
};

static size_t   	numTeulUpdates = 0;
static TeulTimeMS	lastTeulUpdate = 0;
static TeulTimeMS	averageDelta = 0;
static TeulTimeMS	maxDelta = 0;
static TeulTimeMS   _lastIdle = 0;

#if platOS == OS_WIN32

static fd_set 		master_readfs, master_writefs;
static size_t		maxSock;

#else

struct POLL_STRUCT
{
	static const size_t CAPACITY = Teul::MaxConnections;
	struct pollfd	fd[CAPACITY];
	SOCKET		m_maxSock;
	nfds_t		m_size;
	nfds_t		m_count;
};
const size_t POLL_STRUCT::CAPACITY;
static POLL_STRUCT pollData;

static void _pollSet();

#endif

static void    _pollSockets(TeulTimeMS duration_);
static void    _routeCALLBACKs(ENDPOINT& inEndpoint);

using SocketToEndpointMap = std::unordered_map<SOCKET, ENDPOINT*>;
static SocketToEndpointMap socketToEndpoint;

static std::array<std::stack<DATA_GRAM*>, DATA_GRAM::NUM_DATAGRAM_SIZES> s_freeDatagrams;

//////////////////////////////////////////////////////////////////////
// Pooled and unpooled datagram management
static inline size_t _roundDatagramSize(size_t size)
{
    size_t rounding = (size <= DATA_GRAM::LARGEST_POOLED_SIZE ? DATA_GRAM::NORMAL_ROUNDING : DATA_GRAM::HUGE_ROUNDING);
    
	return ((size + sizeof(DATA_GRAM)) + (rounding - 1)) & ~(rounding - 1);
}

static DATA_GRAM *teulAllocDatagram(const UINT32 size)
{
    assert(size > 0 && "datagram request size must be nonzero"); // ABORT_IF() is not debug-only

	size_t allocSize = _roundDatagramSize(size);

	// If the request is small enough to be pooled and there's something available, grab it.
	if (PN_LIKELY(allocSize <= DATA_GRAM::LARGEST_POOLED_SIZE)) {
        size_t bucketIdx = (allocSize / DATA_GRAM::NORMAL_ROUNDING) - 1;
        assert(bucketIdx < s_freeDatagrams.size() && "The laws of physics are now held in abeyance.");
        
		auto& bucket = s_freeDatagrams[bucketIdx];
        
		if (!bucket.empty()) {
			DATA_GRAM *datagram = bucket.top();
            
			bucket.pop();
            datagram->m_size = size; // reset datagram's initial size; req size and contents don't need resetting
			return datagram;
		}
    }
    
    // Need a new allocation.
    DATA_GRAM *newDatagram = static_cast<DATA_GRAM *>(std::calloc(1, allocSize));

    newDatagram->contents = reinterpret_cast<Teul::PacketHeader *>(newDatagram + 1);
    newDatagram->m_size = size;
    newDatagram->m_reqSize = allocSize; // actual size
    return newDatagram;
}

void teulReleaseDatagram(DATA_GRAM *datagram)
{
    assert(datagram->m_reqSize == _roundDatagramSize(datagram->m_reqSize) && "Datagram allocation size corrupted");
    assert(datagram->contents == reinterpret_cast<Teul::PacketHeader *>(datagram + 1) && "Datagram contents pointer was moved");
    
    if (PN_LIKELY(datagram->m_reqSize < DATA_GRAM::LARGEST_POOLED_SIZE)) {
        s_freeDatagrams[(datagram->m_reqSize / DATA_GRAM::NORMAL_ROUNDING) - 1].push(datagram);
    } else {
		std::free(datagram);
	}
}

#if DEBUG
UNUSED_DECL static void _cleanupDatagramPool()
{
    for (std::stack<DATA_GRAM *> & bucket : s_freeDatagrams) {
        while (!bucket.empty()) {
            std::free(bucket.top());
            bucket.pop();
        }
    }
}
#endif

//////////////////////////////////////////////////////////////////////
// Initialize module

void teulInitModRecv()
{
#if platOS != OS_WIN32
	dbugLog(_FLI, "FD_SETSIZE = %u, sizeof(fd_set) = %" PRSizeT ", MAX_POLL_FD = %" PRSizeT, FD_SETSIZE, sizeof(fd_set), POLL_STRUCT::CAPACITY);

	memset(&pollData, 0, sizeof(pollData));
	for (size_t i = 0; i < pollData.CAPACITY; i++)
	{
		pollData.fd[i].fd = -1;
	}
	_pollSet();
#endif

    // This assertion is here only because there used to be a list clear here
    // which serves no sensible purpose.
    assert(socketToEndpoint.size() == 0 && "Why wouldn't the endpoint list be clear at startup?");

	_lastIdle = teulClock64;
	lastTeulUpdate = teulClock64;
}

static void _checkConnectTCP(ENDPOINT& endpoint)
{
	const SOCKET& sock = endpoint.socket;

#if platOS != OS_WIN32
    int sockerr = 0;
	socklen_t errlen = sizeof(sockerr);
	int gsoret = getsockopt(sock, SOL_SOCKET, SO_ERROR, &sockerr, &errlen);
    
    if (gsoret < 0) {
        int error = dbugGetErrno();
        
        dbugLog(_FLA, "t:%d sock:" PRF_SOCKET " getsockopt error %d", endpoint.teulIndex(), PRA_SOCKET(sock), error);
        endpoint.doError(error);
        teulClose(sock);
        return;
    } else if (sockerr != 0) {
        dbugLog(_FLA,"t:%d sock:" PRF_SOCKET " open err:%d", endpoint.teulIndex(), PRA_SOCKET(sock), sockerr);
        endpoint.doError(sockerr);
        teulClose(sock);
        return;
    }
#endif
    
    endpoint.lastDataRecv.makeCurrent();
    endpoint.lastDataSent.makeCurrent();

    socklen_t addrlen = sizeof(struct sockaddr_in);

    if (PN_UNLIKELY(getsockname(sock, (struct sockaddr *)&endpoint.localSockAddress, &addrlen) < 0)) {
        int error = dbugGetErrno();
        dbugLog(_FLA, "t:%d sock:" PRF_SOCKET " getsockname failed err:%d", endpoint.teulIndex(), PRA_SOCKET(sock), error);
        // It seems ridiculous to close out the endpoint here, but there isn't much else sane to do.
        endpoint.doError(error);
        teulClose(endpoint);
        return;
    }
    
    assert(addrlen == sizeof(struct sockaddr_in));
    endpoint.setConnected();

    SINT32 synret = teulModSendCALLBACKe(endpoint, *SYN_TOKEN);

    if (PN_UNLIKELY(synret < 0)) {
        dbugLog(_FLA, "t:%d sock:" PRF_SOCKET " new connection send failed err:%d", endpoint.teulIndex(), PRA_SOCKET(sock), synret);
        endpoint.doError(synret);
        teulClose(endpoint);
    }
}

static void _acceptConnectTCP(ENDPOINT& endpoint)
{
	dbugLog(_FLD, "t:%d entered _acceptConnectTCP, time:%" PR64BitUT "", endpoint.teulIndex(), teulClock64);
	// Multiple simultaneous incoming connections may only generate a single "event", so while we
	// are here, we should try and drain any/all incoming connections queued up (hence the loop).
    struct sockaddr_in newFromAddress, newLocalAddress;
    socklen_t len = sizeof(struct sockaddr_in);
    SOCKET newSock = INVALID_SOCKET;
    
    while ((newSock = accept(endpoint.socket, reinterpret_cast<struct sockaddr *>(&newFromAddress), &len)) != INVALID_SOCKET) {
        assert(len == sizeof(struct sockaddr_in));
        if (getsockname(newSock, reinterpret_cast<struct sockaddr *>(&newLocalAddress), &len) != 0) {
            int error = dbugGetErrno();
            dbugLog(_FLA, "t:%d sock:" PRF_SOCKET " getsockname failed err:%d", endpoint.teulIndex(), PRA_SOCKET(newSock), error);
            OS_CLOSESOCKET(newSock);
            break;
        }
        
        dbugLog(_FLI, "t:%d sock:" PRF_SOCKET " accepted connection", endpoint.teulIndex(), PRA_SOCKET(newSock));
        ENDPOINT *newEndpoint = teulCreateEndpoint(false, endpoint.timeout, static_cast<PROTOCOL>(endpoint.protocol));
        if (PN_UNLIKELY(!newEndpoint)) {
            OS_CLOSESOCKET(newSock);
            break;
        }
        newEndpoint->socket = newSock;
        newEndpoint->fromSockAddress = newFromAddress;
        newEndpoint->localSockAddress = newLocalAddress;
        teulConfigureEndpointSocket(*newEndpoint);

        newEndpoint->setConnected();
        newEndpoint->m_parent = &endpoint;
#if platBuild == BUILD_SERVER
        newEndpoint->m_peer = (endpoint.localPort() == WWII::Net::Ports::ServerPeering + Server::myServerInfo.stats.type);
#endif
        if ((newEndpoint->fromSockAddress.sin_addr.s_addr & 0x00ffffff) == 0) {
            newEndpoint->fromSockAddress.sin_addr.s_addr |= (newEndpoint->localSockAddress.sin_addr.s_addr & 0x00ffffff);
        }
        teulMRecvAddToPoll(*newEndpoint);
        
        // Just use the unsafe one-arg form of teulGetAddress(), the whole thing's a trainwreck anyway.
        dbugLog(_FLN, "t:%d accepted %s:%u->%s:%u as t:%d sock:" PRF_SOCKET,
            endpoint.teulIndex(), teulGetAddress(newEndpoint->destAddress()), newEndpoint->fromPort(),
            teulGetAddress(newEndpoint->localAddress()), newEndpoint->localPort(), PRA_SOCKET(newSock), newEndpoint->teulIndex());
    }
    int error = dbugGetErrno();
    if (error == PLAT_EAGAIN || error == PLAT_EWOULDBLOCK) {
        ; // no more connections to accept
    } else if (error == PLAT_EINTR) {
        ; // let it come around again
    } else {
        dbugLog(_FLA, "t:%d sock:" PRF_SOCKET " accept error: %d", endpoint.teulIndex(), PRA_SOCKET(endpoint.socket), error);
    }
}

static ssize_t _receiveTCP(ENDPOINT& endpoint, void * const ioData, size_t* ioSize)
{
    ssize_t recvRet = INVALID_SOCKET;
    int error = 0;
    
    do {
        recvRet = recv(endpoint.socket, static_cast<char *>(ioData), static_cast<size_t>(*ioSize), 0);
    } while (recvRet == INVALID_SOCKET && (error = dbugGetErrno()) == PLAT_EINTR);
    
    if (recvRet < 0) {
        if (error == PLAT_EAGAIN || error == PLAT_EWOULDBLOCK) {
            // This shouldn't happen in reality, since getting here means we were told data was available
            // on the socket. But, networking is weird, so just in case, we treat it as zero data received.
            // We used to treat it as a silent error, but that's stupid.
            return 0;
        }
        
        dbugLog(_FLN, "t:%d sock:" PRF_SOCKET " tcp close: read err:%d", endpoint.teulIndex(), PRA_SOCKET(endpoint.socket), error);
        endpoint.doError(error);
        teulClose(endpoint);
        return -error;
    } else if (recvRet == 0) {
        // For some reason Teul treats EOF as a type of error. Sigh.
        dbugLog(_FLN, "t:%d sock:" PRF_SOCKET " tcp close: EOF", endpoint.teulIndex(), PRA_SOCKET(endpoint.socket));
        endpoint.doError(EPIPE); // eh, more or less.
        teulClose(endpoint);
        return 0;
    } else {
        endpoint.lastDataRecv.makeCurrent();
        *ioSize = static_cast<UINT32>(recvRet);
        return recvRet;
    }
}

static ssize_t _startNewPacket(ENDPOINT& endpoint)
{
	// Read the 3-byte lead-in (token + length)
	char * const prefixBuffer = reinterpret_cast<char * const>(&endpoint.m_readAheadBuffer[0]);
	size_t readSize = sizeof(endpoint.m_readAheadBuffer) - endpoint.m_readBufferUsed;
	ssize_t readVal = _receiveTCP(endpoint, prefixBuffer + endpoint.m_readBufferUsed, &readSize);
 
    if (readVal <= 0) { // yes, we actually mean <=, not <, this check includes the EOF case
		return readVal;
    }

	// Verify it has valid prefix data. We read at least 1 byte if we got to this point.
    Teul::PacketHeader partialHeader(prefixBuffer);
    
	if (partialHeader.packetToken() != Teul::PacketToken::Encrypted && partialHeader.packetToken() != Teul::PacketToken::Unencrypted) {
		dbugOutputPacket(endpoint, true, prefixBuffer, readSize);
		dbugLog(_FLN, "t:%d garbage packet prefix %u", endpoint.teulIndex(), partialHeader.packetType());
		teulClose(endpoint);
		return -EINVAL;
	}

	size_t totalRead = endpoint.m_readBufferUsed + readVal;

    // It is possible, though rather unlikely, for various conditions to cause an underrun here; we'll need to wait for more data.
	if (totalRead < Teul::PacketPrefixBytes) {
		dbugLog(_FLD, "t:%d underrun reading packet prefix, waiting for more data.", endpoint.teulIndex());
		endpoint.m_readBufferUsed = totalRead;
		return -PLAT_EAGAIN;
	}

	ABORT_IF_EX(partialHeader.packetSizeBytes() > Teul::MaxAllowedPacketSize_bytes, "You done messed up some math somewhere.");
	if (partialHeader.packetSizeBytes() < 1) {
		dbugOutputPacket(endpoint, true, prefixBuffer, readSize);
		dbugLog(_FLN, "t:%d invalid packet size: %u", endpoint.teulIndex(), partialHeader.packetSizeBytes());
        endpoint.doError(EINVAL);
		teulClose(endpoint);
		return -EINVAL;
	}

	// Add the envelope size.
	size_t totalSize = Teul::PacketPrefixBytes + partialHeader.packetSizeBytes();
	
    endpoint.m_impl->m_curDatagram = teulAllocDatagram(totalSize);
	endpoint.m_impl->m_curDatagram->sockAddress = endpoint.destSockAddress;
	memcpy(endpoint.m_impl->m_curDatagram->contents, prefixBuffer, totalRead);
	endpoint.m_readBufferUsed = totalRead;
	return static_cast<ssize_t>(totalRead);
}

static ssize_t _continuePacket(ENDPOINT& endpoint)
{
	size_t readSize = endpoint.m_impl->m_curDatagram->m_size - endpoint.m_readBufferUsed;
	char * const readBuffer = reinterpret_cast<char * const>(endpoint.m_impl->m_curDatagram->contents) + endpoint.m_readBufferUsed;
	ssize_t readVal = _receiveTCP(endpoint, readBuffer, &readSize);
    
	if (readVal <= 0) { // Yes, we want <= here
		return static_cast<SINT32>(readVal);
	}
	endpoint.m_readBufferUsed += static_cast<size_t>(readVal);
	return readVal;
}

static ssize_t _recvOneFromStream(ENDPOINT& endpoint)
{
	if (!endpoint.m_impl->m_curDatagram) {
		ssize_t retval = _startNewPacket(endpoint);
        
		if (retval <= 0) {
			return retval;
        }
        assert(endpoint.m_impl->m_curDatagram);
	}

	UINT32 datagramSize = endpoint.m_impl->m_curDatagram->m_size;
    
	if (endpoint.m_readBufferUsed < datagramSize) {
		ssize_t retval = _continuePacket(endpoint);
        
		if (retval <= 0) {
			return retval;
		}

		// Don't progress further until we've read the entire payload.
		if (endpoint.m_readBufferUsed < datagramSize) {
			return -PLAT_EAGAIN;
		}
	}

    dbugOutputPacket(endpoint, true, endpoint.m_impl->m_curDatagram->contents, endpoint.m_readBufferUsed);
	endpoint.m_impl->recvQueue.push_back(endpoint.m_impl->m_curDatagram);
	endpoint.m_impl->m_curDatagram = nullptr;
	endpoint.m_readBufferUsed = 0;
	return static_cast<ssize_t>(datagramSize);
}

static ssize_t _recvManyFromPacketSource(ENDPOINT& endpoint)
{
	DATA_GRAM *dgram = nullptr;
	socklen_t addrLen = sizeof(dgram->sockAddress);
	UINT32 packets = 0;

    do {
        ssize_t readVal = 0;
        int error = 0;
        
        dgram = teulAllocDatagram(Teul::MaxMTU + 4);
        
        do {
            readVal = ::recvfrom(endpoint.socket, reinterpret_cast<char *>(dgram->contents), Teul::MaxMTU, 0,
                                 reinterpret_cast<struct sockaddr *>(&dgram->sockAddress), &addrLen);
        } while (readVal == INVALID_SOCKET && (error = dbugGetErrno()) == PLAT_EINTR);
        
        if (readVal < 0) {
            teulReleaseDatagram(dgram);
            return -error;
        } else if (readVal == 0) {
            teulReleaseDatagram(dgram);
            return -EPIPE; // close enough
        } else if (readVal != dgram->contents->packetSizeBytes() + Teul::PacketPrefixBytes) {
            dbugLog(_FLW, "t:%d sock:" PRF_SOCKET " datagram from %s declared itself at %u bytes, but received %" PRSsizeT " bytes",
                    endpoint.teulIndex(), PRA_SOCKET(endpoint.socket), teulGetAddress(ntohl(dgram->sockAddress.sin_addr.s_addr)),
                    dgram->contents->packetSizeBytes(), readVal);
            teulReleaseDatagram(dgram);
            return -EINVAL;
#if platBuild == BUILD_SERVER
        } else if (endpoint.protocol == PROTOCOL_UDB && (dgram->sockAddress.sin_addr.s_addr & 0xff000000) == (Server::myInterface.addr[0].naddr & 0xff000000)) {
            dbugLog(_FLD, "t:%d sock:" PRF_SOCKET " ignoring our own broadcast (from %s)",
                    endpoint.teulIndex(), PRA_SOCKET(endpoint.socket), teulGetAddress(dgram->sockAddress.sin_addr.s_addr));
            teulReleaseDatagram(dgram); // my kingdom for defer {}
            continue;
#endif
        } else {
            dgram->m_size = dgram->contents->packetSizeBytes() + Teul::PacketPrefixBytes;
            endpoint.m_impl->recvQueue.push_back(dgram);
            dgram = nullptr;
            ++packets;
        }
    } while (dgram == nullptr);
	return static_cast<ssize_t>(packets);
}

ssize_t _recvDatagram(ENDPOINT& endpoint)
{
	if (PN_UNLIKELY(endpoint.socket == INVALID_SOCKET)) {
		return -EINVAL;
    }
    
    ssize_t result = 0;
    
    switch (endpoint.protocol) {
        case PROTOCOL_TCP:
        case PROTOCOL_CTRL:
            do {
                result = _recvOneFromStream(endpoint);
                if (result == -PLAT_EAGAIN || result == -PLAT_EWOULDBLOCK) {
                    // need to wait for more data, bail
                    break;
                } else if (result <= 0) {
                    return result; // I don't care if we've recieved at least some packets, an error's an error, including EOF
                }
                endpoint.lastDataRecv.makeCurrent();
                // Packet was placed on endpoint's receive queue, nothing more to do here
            } while (result > 0); // never write an infinite loop, no matter how guaranteed your break condition
            break;
        case PROTOCOL_UDB:
        case PROTOCOL_UDP:
            result = _recvManyFromPacketSource(endpoint);
            if (result > 0) {
                endpoint.lastDataRecv.makeCurrent();
            }
            break;
        default:
            LogAbort("Invalid protocol");
    }
    return result;
}

#if platOS != OS_WIN32
//////////////////////////////////////////////////////////////////////
// Process a receive event on an endpoint.
// Returns true if the connection got closed.

static void _handleReceive(ENDPOINT& endpoint)
{
	ssize_t result = _recvDatagram(endpoint);
    
    // Result may be an error indication, but for steaming endpoints we might ALSO have put packets on the
    // endpoint's receive queue before the error hit. Make sure we don't drop packets on the floor after
    // errors, which include things like normal connection closure.
    // TODO: This may not do what we want, since routing callbacks can trigger an essentially infinite amount
    // of additional processing that can throw all kinds of errors of its own around. Design something better.
    if (PN_LIKELY(endpoint.state() < IS_CLOSED)) {
        _routeCALLBACKs(endpoint);
    }
    
    if (PN_UNLIKELY(result == -PLAT_EAGAIN || result == -PLAT_EWOULDBLOCK)) {
        ; // need more data
    } else if (PN_UNLIKELY(result < 0)) {
        dbugLog(_FLI, "t:%d sock:" PRF_SOCKET " receive error %" PRSsizeT ", closing", endpoint.teulIndex(), PRA_SOCKET(endpoint.socket), -result);
        teulClose(endpoint);
    }
}

static void _runPollEndpoints(struct pollfd *fdArray, nfds_t maxSlots, int numSlots)
{
	for (size_t slot = 0; slot < maxSlots; ++slot) {
		pollfd& sfd = fdArray[slot];
  
        if (sfd.fd < 0 || sfd.revents == 0) {
            continue;
        }
        
		auto it = socketToEndpoint.find(sfd.fd);
        ABORT_IF_EX(it == socketToEndpoint.end() || !it->second, "sock:" PRF_SOCKET " slot:%" PRSizeT " no mapped endpoint", PRA_SOCKET(sfd.fd), slot);

		ENDPOINT& endpoint = *(it->second);

		if (!endpoint.m_inUse || endpoint.socket == INVALID_SOCKET || endpoint.state() > IS_CLOSING) {
			dbugLog(_FLI,"t:%d sock:" PRF_SOCKET " slot:%" PRSizeT " poll signaled for invalid or dead endpoint", endpoint.teulIndex(), PRA_SOCKET(sfd.fd), slot);
			teulMRecvRemoveFromPoll(endpoint);
			continue;
		}

		if ((sfd.revents & POLLOUT) != 0) {
            if (endpoint.protocol == PROTOCOL_TCP && endpoint.state() == IS_OPENING && !endpoint.m_connected) {
                // TCP connect completion?
                _checkConnectTCP(endpoint);
            } else {
                // Only push send queue if we didn't check for TCP connect
                endpoint.pushSendQueue();
            }
		}

		if ((sfd.revents & POLLIN) != 0) {
            if (endpoint.protocol == PROTOCOL_TCP && endpoint.state() == IS_LISTENING) {
                _acceptConnectTCP(endpoint);
            } else {
                _handleReceive(endpoint);
            }
		}

#if defined(POLLRDHUP)
		if (PN_UNLIKELY((revents & POLLRDHUP) != 0)) {
			dbugLog(_FLD, "t:%d sock:" PRF_SOCKET " remote disconnected", endpoint.teulIndex(), PRA_SOCKET(sfd.fd));
            endpoint.doError(ECONNRESET);
			teulClose(endpoint);
		}
        else
#endif
		if (PN_UNLIKELY((sfd.revents & POLLHUP) != 0)) {
			dbugLog(_FLM, "t:%d sock:" PRF_SOCKET " socket disconnected", endpoint.teulIndex(), PRA_SOCKET(sfd.fd));
			teulClose(endpoint);
		}
  
        sfd.revents = 0; // clear for next poll()
	}
}
#endif

static void
_checkEndpointState(ENDPOINT& endpoint)
{
	switch (endpoint.state())
	{
		// Connection is doing handshakes, etc.
	case IS_OPENING:
	break;

	case IS_LISTENING:
	break;

		// Connection has opened, needs to do keep-alive.
	case IS_OPEN:
		if ( endpoint.m_doTimeoutCheck )
		{
			const TeulDeltaMS timeSinceLastKlive = endpoint.lastKliveSent.elapsed();
#if defined(DEBUG_CONNECTIONS)
			dbugLog(_FLD, "t:%d dtc:true tslk:%" PR64BitUT " kli:%" PR64BitUT "", endpoint.teulIndex(), timeSinceLastKlive, (UINT64)endpoint.m_impl->kliveInterval);
#endif
			if ( timeSinceLastKlive >= (TeulDeltaMS)(UINT64)endpoint.m_impl->kliveInterval )
			{
#if defined(DEBUG_CONNECTIONS)
				dbugLog(_FLD, " sending klive");
#endif
				const bool oldNagleValue = endpoint.setNagleEnabled(false);
				const SINT32 sendResult = PN_LIKELY(KLIVE_TOKEN != nullptr) ? teulModSendCALLBACKe(endpoint, *KLIVE_TOKEN) : -EINVAL;
				if(sendResult >= 0)
				{
					endpoint.lastKliveSent.makeCurrent();
					endpoint.setNagleEnabled(oldNagleValue);
				}
				else
					endpoint.doError(ETEUL_CONNECTION_STALLED);
			}
		}

	break;

		// Connection is negotiating shutdown.
	case IS_CLOSING:
		if (!endpoint.m_reliable)
		{
			if (!endpoint.m_closed)
			{
				dbugWrite(_FLN,"IS_CLOSING");
				endpoint.doError(ENOTCONN);
                // We used to send a FYN_TOKEN here. This has absolutely no value and serves absolutely no purpose.
                // Close out the damned endpoint already. Oh BTW, we only do this for UDP sockets, not TCP. How TCP
                // sockets get clsoed out the rest of the way I don't even know yet.
                teulClose(endpoint);
			}
		}
	break;

		// Connection has completed its close and is now dead.
	case IS_CLOSED:
		if(  endpoint.m_closed == false )
		{
			dbugWrite(_FLN,"IS_CLOSED");
			endpoint.doError(ENOTCONN);
		}
	break;

	case IS_DEAD:
		// It may reach this state, but why would it still be here?
	break;

	default:
		LogAbort("t:%d endpoint in unknown state %d", endpoint.teulIndex(), endpoint.state());
	}
}

void
_checkEndpointTimeouts(ENDPOINT& endpoint)
{
	if ( ! endpoint.m_doTimeoutCheck || endpoint.timeout == 0 )
		return;

	const TeulDeltaMS elapsed = (TeulDeltaMS)(teulClock64 - endpoint.lastDataRecv.time());
	if ( elapsed <= (TeulDeltaMS)endpoint.timeout )
		return;

	if (endpoint.state() <= IS_CLOSED)
	{
		printf("t:%d timeout\n", endpoint.teulIndex());
		dbugLog(_FLN, "t:%d endpoint timeout error, closed timeout:%d elapsed:%" PR64BitT "ms"
					, endpoint.teulIndex()
					, endpoint.timeout
					, elapsed
					);
		teulClose(endpoint);
	}
}

static void
_idleEndpoint(ENDPOINT& endpoint)
{
	_checkEndpointState(endpoint);

	_checkEndpointTimeouts(endpoint);
}

#if platOS == OS_WIN32
static void
_endpointError(const char* const text, ENDPOINT& endpoint, const SINT32 err)
{
	dbugLog(_FLA, "t:%d endpoint connect error %d (%s), closing", endpoint.teulIndex(), err, text);
	endpoint.doError(err);
	teulClose(endpoint);
}

static bool
_endpointFDConnect(ENDPOINT& endpoint, const WSANETWORKEVENTS& eventData)
{
	const int err = eventData.iErrorCode[FD_CONNECT_BIT];
	switch ( err )
	{
	case WSAECONNREFUSED:
		_connectionRefused(endpoint, err);
		return false;
	break;
	case WSAENETUNREACH:
		_endpointError("netunreach", endpoint, err);
		return false;
	break;
	case WSAEAFNOSUPPORT:
		_endpointError("afnosup", endpoint, err);
		return false;
	break;
	case WSAETIMEDOUT:
		_endpointError("timedout", endpoint, err);
		return false;
	break;
	case WSAENOBUFS:
		_endpointError("nobuffs", endpoint, err);
		return false;
	break;
	case WSAENETDOWN:
		_endpointError("netdown", endpoint, err);
		return false;
	break;
	case 0:
		if ( endpoint.protocol != PROTOCOL_TCP ) // && endpoint->protocol != CMD )
		{
			dbugLog(_FLA, "received connect for endpoint %d ilegal protocol %d"
						, endpoint.teulIndex(), endpoint.protocol);
			return false;
		}

		if ( endpoint.state() != IS_OPENING || endpoint.m_connected == true )
		{
			dbugLog(_FLA,"received connect on endpoint %d in state %d"
						, endpoint.teulIndex(), endpoint.state());
			return false;
		}

		_checkConnectTCP(endpoint);
	break;
	default:
	break;
	}

	return true;
}

static UINT64 g_idleInstance = 0;

static void
_pollSocketsInner(TeulTimeMS remainder)
{
	// Make ourselves re-entrant by aborting if someone else comes along and
	// processes any endpoints.
	const UINT64 idleInstance = ++g_idleInstance;
	ENDPOINT_LIST::iterator it;

	// Expire endpoints.
	it = idleEndpoints.begin();
	while ( it != idleEndpoints.end() )
	{
		ENDPOINT& endpoint = *(*it);
		++it;
		_idleEndpoint(endpoint);
		if ( idleInstance != g_idleInstance )
		{
			printf("abort early\n");
			return;
		}
	}

	// Build up a list of endpoints we're still interested in.
	static std::vector<WSAEVENT> eventHandles;
	eventHandles.reserve(Teul::MaxConnections);
	eventHandles.clear();

	for ( auto& ep : idleEndpoints )
	{
		if ( ep->m_inUse && ep->socket != INVALID_SOCKET )
			eventHandles.push_back(ep->eventObject);
	}

	if ( eventHandles.empty() )
	{
#if platBuild == BUILD_SERVER
		// Server wants us to spend time in the loop.
		if ( remainder > 0 )
			std::this_thread::sleep_for(std::chrono::milliseconds(remainder));
#endif
		return;
	}

#if defined(MULTI_CLIENT)
	int segments = (eventHandles.size() / WSA_MAXIMUM_WAIT_EVENTS) + 1;
	DWORD tWait = static_cast<DWORD>(remainder / segments);
	int eventOffset = 0;
	for (size_t eventOffset = 0; eventOffset < eventHandles.size(); eventOffset += WSA_MAXIMUM_WAIT_EVENTS) {
#else 
	DWORD tWait = static_cast<DWORD>(remainder);
	size_t eventOffset = 0;
#endif

		// Wait for some network activity and then check all the endpoints.
		const size_t sizeEvents = std::min(static_cast<size_t>(WSA_MAXIMUM_WAIT_EVENTS),
			(eventHandles.size() - eventOffset));
		DWORD waitMessage = WSAWaitForMultipleEvents(sizeEvents, &eventHandles[eventOffset], FALSE, tWait, FALSE);
		if (waitMessage == WSA_WAIT_TIMEOUT || waitMessage == WSA_WAIT_FAILED)
		{
#if defined(MULTI_CLIENT)
			continue;
#else
			return;
#endif
		}

		WSANETWORKEVENTS eventData;
		it = idleEndpoints.begin();
		for (; g_idleInstance == idleInstance && it != idleEndpoints.end(); ++it)
		{
			ABORT_IF(*it == nullptr);
			ENDPOINT& endpoint = *(*it);

			const int enumErr = WSAEnumNetworkEvents(endpoint.socket, endpoint.eventObject, &eventData);
			if (enumErr != 0)
			{
				dbugLog(_FLD, "Aborting, error returned from WSAEnumNetworkEvents %d", enumErr);
				continue;
			}

			if (eventData.lNetworkEvents & FD_CONNECT)
			{
				if (!_endpointFDConnect(endpoint, eventData))
				{
					continue;
				}
			}

			if ((eventData.lNetworkEvents & FD_WRITE) != 0)
			{
                endpoint.pushSendQueue();
            }

			if ((eventData.lNetworkEvents & (FD_READ | FD_OOB)) != 0)
			{
				const SINT32 theErr = _recvDatagram(endpoint);
				if (theErr < 0)
				{
					const SINT32 errNo = dbugGetErrno();
					if (errNo != 0)
					{
						const bool notCompleteYet = (errNo == PLAT_EINTR) | (errNo == PLAT_EAGAIN) | (errNo == PLAT_EWOULDBLOCK);
						if (notCompleteYet)
						{
							dbugSetErrno(0);	// Not important.
						}
						else
						{
							dbugLog(_FLI, "t:%d recv error:%d, closing. time:%" PR64BitUT "", endpoint.teulIndex(), errNo, teulClock64);
							teulClose(endpoint);
						}
						continue;
					}
				}
			}
			else if (eventData.lNetworkEvents & FD_ACCEPT)
			{
				if (endpoint.protocol != PROTOCOL_TCP) // && endpoint->protocol != CMD )
				{
					dbugLog(_FLA, "t:%d received accept for ilegal protocol %d", endpoint.teulIndex(), endpoint.protocol);
					continue;
				}

				if (endpoint.state() != IS_LISTENING)
				{
					dbugLog(_FLA, "t:%d received accept on non listening endpoint", endpoint.teulIndex());
					continue;
				}

				_acceptConnectTCP(endpoint);
			}
			else if (eventData.lNetworkEvents & FD_CLOSE)
			{
				dbugLog(_FLI, "t:%d close code:%d, closing. time:%" PR64BitUT "", endpoint.teulIndex(), eventData.iErrorCode[FD_CLOSE_BIT], teulClock64);
				teulClose(endpoint);
				continue;
			}

			_routeCALLBACKs(endpoint);
		}

#if defined(MULTI_CLIENT)
	}
#endif

}
#endif

void
ENDPOINT::updateSocketEvents()
{
#if platOS != OS_WIN32
	const UINT32 slot = m_impl->pollSlot();
	// Since slot is now unsigned, negative values
	// will be really big.
	if ( slot >= pollData.CAPACITY )
		return;

	socketToEndpoint[socket] = this;
	pollData.fd[slot].fd = socket;
#if defined(POLLRDHUP)
	pollData.fd[slot].events = POLLHUP | POLLRDHUP | POLLIN;
#else
	pollData.fd[slot].events = POLLHUP | POLLIN;
#endif
    if ( ((protocol == PROTOCOL_TCP) && (state() == IS_OPENING) && (!m_connected)) || (!m_impl->m_sendQueue.empty()) )
        pollData.fd[slot].events |= POLLOUT;

	pollData.fd[slot].revents = 0;

	if (pollData.m_maxSock == INVALID_SOCKET)
		pollData.m_maxSock = socket;
	else
		pollData.m_maxSock = std::max(pollData.m_maxSock, socket);
#else
	auto flags = FD_READ | FD_OOB | FD_CLOSE;
	if ( m_connected == false )
		flags |= FD_CONNECT | FD_ACCEPT;
	if ( !m_impl->m_sendQueue.empty() )
		flags |= FD_WRITE;

	WSAEventSelect(socket, eventObject, flags);
#endif
}


#if platOS != OS_WIN32
static void _pollAddEndpoint(ENDPOINT& endpoint)
{
	nfds_t slot;
	for (slot = 0; slot < pollData.m_size; ++slot)
	{
		if ( pollData.fd[slot].fd == INVALID_SOCKET )
			break;
	}

	if (slot == pollData.m_size)
		pollData.m_size++;

	pollData.m_count++;
	endpoint.m_impl->pollSlot(slot);

	dbugLog(_FLD,"t:%d sock:" PRF_SOCKET " adding endpoint state:%d slot:%d count:%" PRSizeT ""
				, endpoint.teulIndex(), PRA_SOCKET(endpoint.socket)
				, endpoint.state(), endpoint.m_impl->pollSlot(), static_cast<size_t>(pollData.m_count));

	endpoint.updateSocketEvents();
}

// should only be called from teulMRecvRemoveFromPoll
static void _pollRemoveEndpoint(ENDPOINT& endpoint)
{
	const SINT16 slot = endpoint.m_impl->pollSlot();
	if (slot == INVALID_SOCKET)	// not actually in the list
		return;

	assert(pollData.m_count > 0);
	pollData.m_count--;

	dbugLog(_FLD, "t:%d sock:" PRF_SOCKET " removing endpoint state:%d slot:%d count:%" PRSizeT ""
				, endpoint.teulIndex(), PRA_SOCKET(endpoint.socket)
				, endpoint.state(), slot, static_cast<size_t>(pollData.m_count));

	if (pollData.m_count < (pollData.m_size / 2))
	{
		_pollSet();
	}
	else
	{
		pollData.fd[slot].fd = INVALID_SOCKET;
		endpoint.m_impl->pollSlot(-1);
	}
}
#endif

void
teulMRecvRemoveFromPoll(ENDPOINT& endpoint)
{
#if platOS != OS_WIN32
	if ( endpoint.socket != INVALID_SOCKET )
	{
		// set these here since _pollSet keys off of INVALID_SOCKET
		socketToEndpoint.erase(endpoint.socket);
		endpoint.socket = INVALID_SOCKET;
	}

	_pollRemoveEndpoint(endpoint);

#else
	if ( endpoint.socket != INVALID_SOCKET )
	{
		// Disassociate the socket / event object with any events.
		WSAEventSelect(endpoint.socket, endpoint.eventObject, 0);
		// Now close the event object.
		if (WSACloseEvent(endpoint.eventObject) < 0)
			dbugLog(_FLA,"t:%d error closing wsa event: %d", endpoint.teulIndex(), WSAGetLastError());
	}
#endif
}

#if platOS != OS_WIN32
void teulMRecvUpdatePoll(ENDPOINT& endpoint)
{
	endpoint.updateSocketEvents();
#ifdef EXTREME_DEBUG
	dbugLog(_FLD, "t:%d sock:" PRF_SOCKET " updating endpoint slot:%d combined"
				, endpoint.teulIndex(), PRA_SOCKET(endpoint.socket), endpoint.m_impl->pollSlot());
#endif
}
#else
void teulMRecvUpdatePoll(ENDPOINT& /* endpoint */)
{
}
#endif

void teulMRecvAddToPoll(ENDPOINT& endpoint)
{
#if platOS != OS_WIN32
	if ( PN_UNLIKELY((endpoint.state() == IS_DEAD) | (endpoint.socket == INVALID_SOCKET)) )
	{
		dbugLog(_FLA, "t:%d attempt to add an invalid endpoint to poll", endpoint.teulIndex());
		return;
	}

	_pollAddEndpoint(endpoint);
#else
	endpoint.eventObject = WSACreateEvent();
	endpoint.updateSocketEvents();
#endif
}

#if platOS != OS_WIN32
static void
_pollSet()
{
	pollData.m_count = 0;
	pollData.m_size = 0;
	pollData.m_maxSock = INVALID_SOCKET;

	// don't bother checking for when we don't have any endpoints
	if ( idleEndpoints.empty() )
		return;

	// set the read and write file descriptor bits for all active endpoints
	ENDPOINT_LIST::iterator it = idleEndpoints.begin();
	while ( it != idleEndpoints.end() )
	{
		ENDPOINT* const endpoint = *it;
		++it;

		const SOCKET sock = endpoint->socket;
		if ( endpoint->state() != IS_DEAD && sock != INVALID_SOCKET )
		{
			assert(pollData.m_count <= std::numeric_limits<UINT16>::max());
			endpoint->m_impl->pollSlot(pollData.m_count);
			endpoint->updateSocketEvents();

			pollData.m_count++;
			pollData.m_size++;
		}
		else
		{
			dbugWrite(_FLA, "t:%d sock:" PRF_SOCKET " endpoint not set, state:%d"
					, endpoint->teulIndex(), PRA_SOCKET(endpoint->socket), endpoint->state());
		}
	}

	dbugLog(_FLN, "combined %" PRSizeT " sockets for reading, max %d"
				, static_cast<size_t>(pollData.m_count), static_cast<int>(pollData.m_maxSock));
}
#endif

#if platOS != OS_WIN32
static void
_pollSocketsInner(TeulTimeMS duration_)
{
	SINT32	retval = 0;

	auto pollFds = &pollData.fd[0];
	int numfds = retval = poll(pollFds, pollData.m_size, duration_);

	if (PN_UNLIKELY(retval < 0 && dbugGetErrno() != PLAT_EINTR))
		LogAbort("illegal return %d from _poll",retval);

	// are there any sockets that need reading?
	if ( numfds > 0 )
	{
		_runPollEndpoints(pollData.fd, pollData.m_size, numfds);
	}
}

static bool
_pollSetIsEmpty()
{
	return pollData.m_size <= 0;
}
#else
static bool
_pollSetIsEmpty()
{
	return idleEndpoints.empty();
}
#endif

static void
_pollSockets(TeulTimeMS duration_)
{
	if ( _pollSetIsEmpty() )
	{
		// Yield CPU for a short period of time.
		if ( duration_ > 0 )
			std::this_thread::sleep_for(std::chrono::milliseconds(duration_));
		return;
	}

	_pollSocketsInner(duration_);
}

static void
_attemptDecryption(DATA_GRAM* dgram, UCHAR* const decrypted, const UINT32 payloadSize, ENDPOINT& endpoint)
{
	if ( dgram->contents->packetToken() == Teul::PacketToken::Unencrypted )
		return;
	ABORT_UNLESS(dgram->contents->packetToken() == Teul::PacketToken::Encrypted);

	endpoint.decrypt(decrypted, payloadSize);
}

static bool
_isValidSynData(const Teul::PacketHeader* const header, size_t payloadSize)
{
	if ( PN_UNLIKELY(payloadSize < 2) )
		return false;
	if ( PN_LIKELY(header->m_callX == Teul::Module::CallbackX) )
	{
		switch ( header->m_callY )
		{
		case TM_SYN_TOKEN_Y:
		case TM_SYN_ACK_TOKEN_Y:
		case TM_SYN_ACK_ACK_TOKEN_Y:
			return true;
		default:
			break;
		}
	}
	return false;
}

static bool
_checkPacketOrdering(ENDPOINT& endpoint, const Teul::PacketHeader& header)
{
	static_assert( sizeof(Teul::PacketHeader::tag_t) == sizeof(endpoint.m_impl->recvTag), "Endpoint and Packet disagree on size of packet tags" );

	const Teul::PacketHeader::tag_t prevTag = (Teul::PacketHeader::tag_t)endpoint.m_impl->recvTag;
	const Teul::PacketHeader::tag_t expectTag = prevTag + (Teul::PacketHeader::tag_t)1;

	// The new tag should be *exactly* one higher than the old
	if ( PN_UNLIKELY(header.m_tag != expectTag) )
	{
		dbugLog(_FLW, "t:%d out of order packet rx:%u, prev:%u, expected:%u"
						, endpoint.teulIndex()
						, static_cast<UINT32>(header.m_tag), static_cast<UINT32>(prevTag), static_cast<UINT32>(expectTag));
		endpoint.doError(ETEUL_ILLEGAL_PACKET_ORDER);
		teulClose(endpoint);
		return false;
	}

	endpoint.m_impl->recvTag = (UINT16)header.m_tag;

	return true;
}

static void
_processTicks(ENDPOINT& endpoint, const Teul::PacketHeader& header)
{
#if platBuild == BUILD_SERVER
	if ( endpoint.m_parent == nullptr || !endpoint.isFromClient() )
		return;

	// The host assigns the created time when the TCP session is
	// started; so the client time should lag a few milliseconds
	// behind the host. If the client gets more than a few ms
	// ahead of the host, we know they are speed hacking.

	if ( PN_UNLIKELY(endpoint.m_haveTicks == false) )	// true briefly at the start of a connection.
	{
		endpoint.m_impl->m_lastPacketTick = header.m_tick;
		endpoint.m_haveTicks = true;
	}

	TeulTimeMS newTick = (TeulTimeMS)header.m_tick;
	const TeulTimeMS lastTick = (TeulTimeMS)endpoint.m_impl->m_lastPacketTick;
	if ( newTick < lastTick )
	{
		// ensure the value can never be negative
		const TeulTimeMS adjusted = newTick | (1ULL << (sizeof(header.m_tick) * 8));
		newTick = adjusted;
	}
	const TeulTimeMS delta = newTick - lastTick;

	if ( delta > 0 )
		endpoint.m_tickAccumulation += delta;

	// Save the value.
	endpoint.m_impl->m_lastPacketTick = header.m_tick;
	endpoint.m_impl->m_lastTickRx = teulClock64;

	// Only evaluate every 8th packet.
	if ( PN_LIKELY((endpoint.m_impl->recvTag & 7) > 0) )
		return;

	teulSyncClock();
	Teul::TickSample sample(endpoint.m_created.elapsed(), endpoint.m_tickAccumulation);

	if ( PN_LIKELY(sample.delta > -100) )
		return;

	const FLOAT64 minRatio = host_config.arena.speedHackRatio;
	if ( (minRatio >= 1.0) & (sample.ratio < minRatio) )
		return;

	endpoint.doError(ETEUL_ILLEGAL_TICKS, &sample);
#endif
}

static void
_executeCallbacks(ENDPOINT& endpoint, DATA_GRAM* const dgram)
{
	Teul::PacketHeader* const hdr = dgram->contents;
	UCHAR* const decryptedData = &hdr->m_tag;

	const UINT32 payloadSize_bytes = dgram->m_size - Teul::PacketPrefixBytes;
	_attemptDecryption(dgram, decryptedData, payloadSize_bytes, endpoint);

	switch ( endpoint.state() )
	{
	case IS_OPENING:
		// when the state == IS_OPENING, we only accept SYN tokens.
		// if they aren't recognized we assume that the encryption has failed
		// and try a different encryption
		if ( PN_UNLIKELY(_isValidSynData(hdr, payloadSize_bytes) == false) )
		{
			dbugLog(_FLA, "t:%d non-syn data in opening mode", endpoint.teulIndex());
			dbugSetErrno(EINVAL);
			return;
		}
	break;
	case IS_LISTENING:
		ABORT_IF_EX(hdr->m_callX != Teul::Module::CallbackX, "t:%d listening sockets must NOT receive callbacks", endpoint.teulIndex());
	break;
	case IS_CLOSING:
		if ( hdr->m_callX != Teul::Module::CallbackX )
		{
			// Ignore stuff trying to arrive while we're trying to close.
			return;
		}
	default:
	break;
	}

	// check for someone hacking our TCP stream by deleting packets...
	if ( endpoint.m_reliable )
	{
		if ( PN_UNLIKELY(! _checkPacketOrdering(endpoint, *hdr)) )
			return;
		_processTicks(endpoint, *hdr);
	}

	//_updateRecvError(curEndpoint->li, dataHeader.tag, dataHeader.mode);

	TEUL_CALLBACK* const callback = TEUL_CALLBACK::Lookup(hdr->m_callX, hdr->m_callY);
	if ( PN_UNLIKELY(callback == nullptr) )
	{
		dbugLog(_FLW, "t:%d ip:%s cb:%u,%u callback does not exist", endpoint.teulIndex(), teulGetAddress(endpoint.fromAddress()), hdr->m_callX, hdr->m_callY);
		return;
	}

	if ( PN_UNLIKELY(hdr->m_callX == Teul::Serv::CallbackX && endpoint.teulIndex() >= 0 && !(endpoint.m_peer)) )
	{
		dbugLog(_FLA,"t:%d ip:%s cb:%d,%d callback access denied", endpoint.teulIndex(), teulGetAddress(endpoint.fromAddress()), hdr->m_callX, hdr->m_callY);
		return;
	}

	UCHAR* const dataStart = (UCHAR*)(&hdr[1]);	// Immediately after the header.
	callback->processCallback(endpoint, dataStart, dgram->m_size - sizeof(*hdr));
}

static void
_routeCALLBACKs(ENDPOINT& endpoint)
{
	EndpointImpl& impl = *endpoint.m_impl;
	while ( impl.recvQueue.empty() == false )
	{
		DATA_GRAM* const datagram = impl.recvQueue.front();
		impl.recvQueue.erase(impl.recvQueue.begin());

		// Temporarily set the endpoint address to correspond to the sender
		// so that we can respond symmetrically.
		const struct sockaddr_in saveFromAddress = endpoint.fromSockAddress;
		endpoint.fromSockAddress = datagram->sockAddress;

		_executeCallbacks(endpoint, datagram);

		endpoint.fromSockAddress = saveFromAddress;

		teulReleaseDatagram(datagram);
	}
}

UINT32
teulElapsedTotal(UINT32 newTotal, UINT32 oldTotal)
{
	UINT32 total;
	if ( newTotal >= oldTotal)
		total = newTotal - oldTotal;
	else
		total = (0xffffffff-oldTotal) + newTotal;

	return total;
}

static void
teulIdleEndpoints()
{
	ENDPOINT_LIST::iterator it = idleEndpoints.begin();
	while ( it != idleEndpoints.end() )
	{
		ENDPOINT* const endpoint = *it;
		++it;

		if ( PN_LIKELY(endpoint != nullptr) )
			_idleEndpoint(*endpoint);
	}
}

SINT32
teulModIdle(const TeulTimeMS duration_)
{
	if ( duration_ == 0 )
		std::this_thread::yield();

	teulSyncClock();

	// At 1s intervals, count stats, kill dead endpoints
	TeulTimeMS interval = teulClock64 - std::min(teulClock64, lastUpdateTime);
	if ( PN_UNLIKELY(interval > 1000) )
	{
		//time to recalc updates per sec
		lastUpdateTime = teulClock64;
		teulKillDeadEndpoints();
	}

	_pollSockets(duration_);

	numTeulUpdates++;

	interval = teulClock64 - std::min(teulClock64, lastTeulUpdate);
	averageDelta += interval;		// divide by numTeulUpdates to get average
	maxDelta = std::max(interval, maxDelta);

	// Can't merge with _lastIdle because this gets set elsewhere too
	lastTeulUpdate = teulClock64;

	interval = teulClock64 - std::min(teulClock64, _lastIdle);
#ifdef DEBUG
	static const TeulTimeMS AlertInterval_ms = 2 * 1000;
#else
	static const TeulTimeMS AlertInterval_ms = 75;
#endif
	if ( PN_UNLIKELY(interval > duration_ + AlertInterval_ms) )
		dbugLog(_FLA, "STALL. teulModIdle interval=%" PR64BitUT "", interval);

	_lastIdle = teulClock64;

	static TeulTimeMS lastIdleCycle = teulClock64;
	interval = teulClock64 - std::min(teulClock64, lastIdleCycle);
	if ( interval < ENDPOINT_IDLE_INTERVAL )
		return 0;

	lastIdleCycle = teulClock64;

	teulIdleEndpoints();

	teulSyncClock();
	interval = teulClock64 - std::min(teulClock64, lastIdleCycle);
#ifdef BETA_RELEASE
	static const TeulTimeMS idleLimit = 50;
#else
	static const TeulTimeMS idleLimit = 25;
#endif
	if ( PN_UNLIKELY(interval > idleLimit + duration_) )
		dbugLog(_FLA, "STALL. idle endpoints interval=%" PR64BitUT "ms", interval);

	return 0;
}

void
ENDPOINT::setConnected()
{
	m_connected = true;
	updateSocketEvents();
}

void
ENDPOINT::setWritesPending(const bool pending_)
{
	m_writesPending = pending_;
	updateSocketEvents();
}
