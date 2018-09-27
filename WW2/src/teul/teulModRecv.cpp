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
    static_assert((NUM_DATAGRAM_SIZES & (NUM_DATAGRAM_SIZES - 1)) == 0, "number of pooled datagram buckets must be power of 2");

    static constexpr const size_t LARGEST_POOLED_SIZE = NUM_DATAGRAM_SIZES * NORMAL_ROUNDING;
    static_assert((LARGEST_POOLED_SIZE & (LARGEST_POOLED_SIZE - 1)) == 0, "calculated max pooling size must be power of 2");

    struct sockaddr_in    sockAddress;
    Teul::PacketHeader*   contents;    //!< Points to the beginning of the packet data.
    UINT32                m_size;      //!< Size of the entire object (might be smaller than allocated size)
    UINT32                m_reqSize;   //!< Original requested size of bucket
    
    inline char * decryptedDataStart() const { return reinterpret_cast<char * const>(&contents->m_tag); }
    inline char * callbackPayload() const { return reinterpret_cast<char * const>(contents + 1); } // eww
};

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

static void _pollRegenerateSet();

static void    _pollSockets(TeulTimeMS duration_);
static void    _routeCallbacks(ENDPOINT& inEndpoint);

using SocketToEndpointMap = std::unordered_map<SOCKET, ENDPOINT*>;
static SocketToEndpointMap socketToEndpoint;

static std::array<std::stack<DATA_GRAM*>, DATA_GRAM::NUM_DATAGRAM_SIZES> s_freeDatagrams;

template <typename T, typename = std::enable_if_t<std::is_integral<T>::value>>
static inline constexpr bool safe_isEAGAIN(T const value) {
    return value == PLAT_EAGAIN || value == PLAT_EWOULDBLOCK;
}

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
	if (allocSize <= DATA_GRAM::LARGEST_POOLED_SIZE) {
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
    assert(datagram->m_reqSize == _roundDatagramSize(datagram->m_reqSize - sizeof(DATA_GRAM)) && "Datagram allocation size corrupted");
    assert(datagram->contents == reinterpret_cast<Teul::PacketHeader *>(datagram + 1) && "Datagram contents pointer was moved");
    
    if (datagram->m_reqSize < DATA_GRAM::LARGEST_POOLED_SIZE) {
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
	dbugLog(_FLI, "FD_SETSIZE = %u, sizeof(fd_set) = %" PRSizeT ", MAX_POLL_FD = %" PRSizeT, FD_SETSIZE, sizeof(fd_set), POLL_STRUCT::CAPACITY);

	memset(&pollData, 0, sizeof(pollData));
	for (size_t i = 0; i < pollData.CAPACITY; i++)
	{
		pollData.fd[i].fd = -1;
	}
	_pollRegenerateSet();

    // This assertion is here only because there used to be a list clear here
    // which serves no sensible purpose.
    assert(socketToEndpoint.size() == 0 && "Why wouldn't the endpoint list be clear at startup?");
}

static ssize_t _checkConnectTCP(ENDPOINT& endpoint)
{
	const SOCKET& sock = endpoint.socket;

#if platOS != OS_WIN32
    // The check of SO_ERROR is strictly mandated on Linux, strongly recommended on macOS, and actively discouraged on Windows.
    int sockerr = 0;
	socklen_t errlen = sizeof(sockerr);
	int gsoret = getsockopt(sock, SOL_SOCKET, SO_ERROR, &sockerr, &errlen);
    
    if (gsoret < 0) {
        int error = dbugGetErrno();
        
        dbugLog(_FLA, "t:%d sock:" PRF_SOCKET " getsockopt failed during connect err:%d", endpoint.teulIndex(), PRA_SOCKET(sock), error);
        return -error;
    } else if (sockerr != 0) {
        dbugLog(_FLA,"t:%d sock:" PRF_SOCKET " open err:%d", endpoint.teulIndex(), PRA_SOCKET(sock), sockerr);
        return -sockerr;
    }
#endif
    
    endpoint.lastDataRecv.makeCurrent();
    endpoint.lastDataSent.makeCurrent();

    socklen_t addrlen = sizeof(struct sockaddr_in);

    if (getsockname(sock, (struct sockaddr *)&endpoint.localSockAddress, &addrlen) == SOCKET_ERROR) {
        int error = dbugGetErrno();
        dbugLog(_FLA, "t:%d sock:" PRF_SOCKET " getsockname failed during connect err:%d", endpoint.teulIndex(), PRA_SOCKET(sock), error);
        return -error;
    }
    
    assert(addrlen == sizeof(struct sockaddr_in));
    endpoint.setConnected();

    SINT32 synret = teulModSendCALLBACKe(endpoint, *SYN_TOKEN);

    if (synret < 0) {
        dbugLog(_FLA, "t:%d sock:" PRF_SOCKET " new connection send failed err:%d", endpoint.teulIndex(), PRA_SOCKET(sock), -synret);
        return synret;
    }
    
    return 0;
}

static ssize_t _acceptConnectTCP(ENDPOINT& endpoint)
{
	// Multiple simultaneous incoming connections may only generate a single "event", so while we
	// are here, we should try and drain any/all incoming connections queued up (hence the loop).
    struct sockaddr_in newFromAddress, newLocalAddress;
    socklen_t len = sizeof(struct sockaddr_in);
    SOCKET newSock = INVALID_SOCKET;
    
    do {
        int error = 0;
        
        do {
            newSock = accept(endpoint.socket, reinterpret_cast<struct sockaddr *>(&newFromAddress), &len);
        } while (newSock == INVALID_SOCKET && (error = dbugGetErrno()) == PLAT_EINTR);
        
        if (newSock == INVALID_SOCKET) {
            if (error == PLAT_ECONNABORTED || error == EPERM) {
                continue; // connection failure or firewall block are non-fatal
            } else if (safe_isEAGAIN(error)) {
                return 0; // Hey laughing boy, no more sockets!
            } else {
                dbugLog(_FLA, "t:%d sock:" PRF_SOCKET " accept error:%d", endpoint.teulIndex(), PRA_SOCKET(endpoint.socket), error);
                return -error;
            }
        }
        
        assert(len == sizeof(struct sockaddr_in));
        if (getsockname(newSock, reinterpret_cast<struct sockaddr *>(&newLocalAddress), &len) == SOCKET_ERROR) {
            error = dbugGetErrno();
            dbugLog(_FLA, "t:%d sock:" PRF_SOCKET " getsockname failed err:%d", endpoint.teulIndex(), PRA_SOCKET(newSock), error);
            OS_CLOSESOCKET(newSock);
            continue;
        }
        
        dbugLog(_FLI, "t:%d sock:" PRF_SOCKET " accepted connection", endpoint.teulIndex(), PRA_SOCKET(newSock));
        ENDPOINT *newEndpoint = teulCreateEndpoint(false, endpoint.timeout, static_cast<PROTOCOL>(endpoint.protocol));
        if (!newEndpoint) {
            // this happens only if we're completely out of endpoints at the Teul layer
            OS_CLOSESOCKET(newSock);
            return -EMFILE;
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
    } while (newSock != INVALID_SOCKET);
    return 0;
}

static ssize_t _receiveTCP(ENDPOINT& endpoint, void * const ioData, size_t *ioSize)
{
    ssize_t recvRet = SOCKET_ERROR;
    int error = 0;
    
#if platOS == OS_WIN32
    // macOS and Linux sensibly use size_t for recv()'s buffer size. Windows uses int.
    assert(*ioSize <= std::numeric_limits<int>::max());
#endif

    do {
        recvRet = recv(endpoint.socket, static_cast<char *>(ioData), *ioSize, 0);
    } while (recvRet == SOCKET_ERROR && (error = dbugGetErrno()) == PLAT_EINTR);
    
    if (recvRet < 0) {
        if (!safe_isEAGAIN(error)) {
            dbugLog(_FLN, "t:%d sock:" PRF_SOCKET " tcp close: read err:%d", endpoint.teulIndex(), PRA_SOCKET(endpoint.socket), error);
            return -error;
        } else {
            // for EAGAIN we return the error with no logging
            return -error;
        }
    } else if (recvRet == 0) {
        dbugLog(_FLN, "t:%d sock:" PRF_SOCKET " tcp close: EOF", endpoint.teulIndex(), PRA_SOCKET(endpoint.socket));
        return -EPIPE;
    } else {
        *ioSize = static_cast<size_t>(recvRet);
    }
    return 0;
}

static ssize_t _startNewTCPPacket(ENDPOINT& endpoint)
{
	// Read the 3-byte lead-in (token + length)
	char * const prefixBuffer = reinterpret_cast<char * const>(&endpoint.m_readAheadBuffer[0]);
	size_t readSize = sizeof(endpoint.m_readAheadBuffer) - endpoint.m_readBufferUsed;
	ssize_t readVal = _receiveTCP(endpoint, prefixBuffer + endpoint.m_readBufferUsed, &readSize);
 
    if (readVal < 0) {
		return readVal;
    }

	// Verify it has valid prefix data. We read at least 1 byte if we got to this point.
    Teul::PacketHeader partialHeader(prefixBuffer);
    
	if (partialHeader.packetToken() != Teul::PacketToken::Encrypted && partialHeader.packetToken() != Teul::PacketToken::Unencrypted) {
		dbugOutputPacket(endpoint, true, prefixBuffer, readSize);
		dbugLog(_FLN, "t:%d garbage packet prefix %u", endpoint.teulIndex(), partialHeader.packetType());
		return -EINVAL;
	}

	size_t totalRead = endpoint.m_readBufferUsed + readSize;

    // It is possible, though rather unlikely, for various conditions to cause an underrun here; we'll need to wait for more data.
	if (totalRead < Teul::PacketPrefixBytes) {
		dbugLog(_FLD, "t:%d underrun reading packet prefix, waiting for more data.", endpoint.teulIndex());
		endpoint.m_readBufferUsed = totalRead;
		return 0;
	}

	ABORT_IF_EX(partialHeader.packetSizeBytes() > Teul::MaxAllowedPacketSize_bytes, "You done messed up some math somewhere.");
	if (partialHeader.packetSizeBytes() < 1) {
		dbugOutputPacket(endpoint, true, prefixBuffer, readSize);
		dbugLog(_FLN, "t:%d invalid packet size: %u", endpoint.teulIndex(), partialHeader.packetSizeBytes());
		return -EINVAL;
	}

	// Add the envelope size.
	size_t totalSize = Teul::PacketPrefixBytes + partialHeader.packetSizeBytes();
	
    endpoint.m_impl->m_curDatagram = teulAllocDatagram(totalSize);
	endpoint.m_impl->m_curDatagram->sockAddress = endpoint.destSockAddress;
	memcpy(endpoint.m_impl->m_curDatagram->contents, prefixBuffer, totalRead);
	endpoint.m_readBufferUsed = totalRead;
	return 0;
}

static ssize_t _continueTCPPacket(ENDPOINT& endpoint)
{
	size_t readSize = endpoint.m_impl->m_curDatagram->m_size - endpoint.m_readBufferUsed;
	char * const readBuffer = reinterpret_cast<char * const>(endpoint.m_impl->m_curDatagram->contents) + endpoint.m_readBufferUsed;
	ssize_t readVal = _receiveTCP(endpoint, readBuffer, &readSize);
    
	if (readVal < 0) {
		return static_cast<SINT32>(readVal);
	}
	endpoint.m_readBufferUsed += static_cast<size_t>(readSize);
	return 0;
}

static ssize_t _recvOneFromStream(ENDPOINT& endpoint)
{
	if (!endpoint.m_impl->m_curDatagram) {
		ssize_t retval = _startNewTCPPacket(endpoint);
        
		if (retval < 0) {
			return retval;
        }
        assert(endpoint.m_impl->m_curDatagram);
	}

	UINT32 datagramSize = endpoint.m_impl->m_curDatagram->m_size;
    
	if (endpoint.m_readBufferUsed < datagramSize) {
		ssize_t retval = _continueTCPPacket(endpoint);
        
		if (retval < 0) {
			return retval;
		}

		// Don't progress further until we've read the entire payload.
		if (endpoint.m_readBufferUsed < datagramSize) {
			return 0;
		}
	}

    dbugOutputPacket(endpoint, true, endpoint.m_impl->m_curDatagram->contents, endpoint.m_readBufferUsed);
	endpoint.m_impl->m_recvQueue.push_back(endpoint.m_impl->m_curDatagram);
	endpoint.m_impl->m_curDatagram = nullptr;
	endpoint.m_readBufferUsed = 0;
	return 0;
}

static ssize_t _recvManyFromPacketSource(ENDPOINT& endpoint)
{
	socklen_t addrLen = sizeof(DATA_GRAM().sockAddress);
    int error = 0;

    do {
        ssize_t readVal = 0;
        
        std::unique_ptr<DATA_GRAM, std::function<void(DATA_GRAM *)>> dgram( // Yay, RAII!
            teulAllocDatagram(Teul::MaxMTU + 4),
            teulReleaseDatagram
        );
        
        do {
            readVal = ::recvfrom(endpoint.socket, reinterpret_cast<char *>(dgram->contents), Teul::MaxMTU, 0,
                                 reinterpret_cast<struct sockaddr *>(&dgram->sockAddress), &addrLen);
        } while (readVal == SOCKET_ERROR && (error = dbugGetErrno()) == PLAT_EINTR);
        
        if (readVal == SOCKET_ERROR) {
            return safe_isEAGAIN(error) ? 0 : -error; // This is the loop's primary exit point
        } else if (readVal == 0) {
            return -EPIPE;
        } else if (readVal != dgram->contents->packetSizeBytes() + Teul::PacketPrefixBytes) {
            dbugLog(_FLW, "t:%d sock:" PRF_SOCKET " datagram from %s declared itself at %u bytes, but received %" PRSsizeT " bytes",
                    endpoint.teulIndex(), PRA_SOCKET(endpoint.socket), teulGetAddress(ntohl(dgram->sockAddress.sin_addr.s_addr)),
                    dgram->contents->packetSizeBytes(), readVal);
            return -EINVAL;
#if platBuild == BUILD_SERVER
        } else if (endpoint.protocol == PROTOCOL_UDB && (dgram->sockAddress.sin_addr.s_addr & 0xff000000) == (Server::myInterface.addr[0].naddr & 0xff000000)) {
            dbugLog(_FLD, "t:%d sock:" PRF_SOCKET " ignoring our own broadcast (from %s)",
                    endpoint.teulIndex(), PRA_SOCKET(endpoint.socket), teulGetAddress(dgram->sockAddress.sin_addr.s_addr));
            continue;
#endif
        } else {
            dgram->m_size = dgram->contents->packetSizeBytes() + Teul::PacketPrefixBytes;
            endpoint.m_impl->m_recvQueue.push_back(dgram.release());
        }
    } while (error == 0 || error == PLAT_EINTR); // this is here to avoid writing while(true)
    
    assert(false);
    return -EINVAL;
}

ssize_t _recvDatagram(ENDPOINT& endpoint)
{
	if (endpoint.socket == INVALID_SOCKET) {
		return -EINVAL;
    }
    
    ssize_t result = 0;
    
    switch (endpoint.protocol) {
        case PROTOCOL_TCP:
        case PROTOCOL_CTRL:
            result = _recvOneFromStream(endpoint);
            if (result == 0) {
                endpoint.lastDataRecv.makeCurrent();
            } else if (safe_isEAGAIN(-result)) {
                result = 0; // EAGAIN is not a real error here.
            }
            break;
        case PROTOCOL_UDB:
        case PROTOCOL_UDP:
            result = _recvManyFromPacketSource(endpoint);
            if (result == 0) {
                endpoint.lastDataRecv.makeCurrent();
            }
            break;
        default:
            LogAbort("Invalid protocol");
    }
    // Packet(s) were placed on endpoint's receive queue, nothing more to do here
    return result;
}

//////////////////////////////////////////////////////////////////////
// Process a receive event on an endpoint.
static ssize_t _handleReceive(ENDPOINT& endpoint)
{
	ssize_t result = _recvDatagram(endpoint);
    
    assert(!safe_isEAGAIN(-result) && result != -PLAT_EINTR); // we're supposed to have translated EAGAIN into a success value and ignored EINTR
    
    if (result < 0) {
        dbugLog(_FLI, "t:%d sock:" PRF_SOCKET " receive error:%" PRSsizeT, endpoint.teulIndex(), PRA_SOCKET(endpoint.socket), -result);
        return result;
    }

    return result;
}

static void _runPollEndpoints(struct pollfd *fdArray, nfds_t maxSlots, int numSlots)
{
    // This generation counter allows us to track reentrancy of the teul processing machinery.
    // It doesn't strictly need to be atomic, but making it so enforces better usage. Each check
    // of the generation counter effectively acts as a cancellation point within Teul's core
    // processing loop. Since we use relaxed ordering for all operations on the atomic, we don't
    // take any memory barrier overhead on x86.
    static std::atomic_uint64_t pollGeneration(0);
    
    if (numSlots == 0) {
        return;
    }
    
    // Use RAII semantics to force regenerate of the pollset even if we hit a cancellation point.
    // This is not hugely efficient, but it has the virtue that we'll never leave the pollset in a weird state.
    struct _PollRegenerator {
        _PollRegenerator() {}
        ~_PollRegenerator() { _pollRegenerateSet(); }
    } _regenerator;
    
    uint64_t currentPollGeneration = pollGeneration.fetch_add(1, std::memory_order_relaxed);
    
    for (size_t slot = 0; slot < maxSlots; ++slot) {
		pollfd& sfd = fdArray[slot];
  
        if (sfd.fd < 0 || sfd.revents == 0) {
            continue;
        }
        
		auto it = socketToEndpoint.find(sfd.fd);
        ABORT_IF_EX(it == socketToEndpoint.end() || !it->second, "sock:" PRF_SOCKET " slot:%" PRSizeT " no mapped endpoint", PRA_SOCKET(sfd.fd), slot);

		ENDPOINT& endpoint = *(it->second);

		if (!endpoint.m_inUse || endpoint.socket == INVALID_SOCKET || endpoint.state() > IS_OPEN) {
			dbugLog(_FLI,"t:%d sock:" PRF_SOCKET " slot:%" PRSizeT " poll signaled for invalid or dead endpoint", endpoint.teulIndex(), PRA_SOCKET(sfd.fd), slot);
			teulMRecvRemoveFromPoll(endpoint, false); // disallow compaction to avoid invalidating our loop
			continue;
		}

        auto revents = std::exchange(sfd.revents, 0); // Reset the pollfd and save the value we're checking for now.
        ssize_t error = 0;

		if (error == 0 && (revents & POLLOUT) != 0) {
            if (endpoint.protocol == PROTOCOL_TCP && endpoint.state() == IS_OPENING && !endpoint.m_connected) {
                error = _checkConnectTCP(endpoint);
            } else {
                error = endpoint.pushSendQueue();
            }
		}

		if (error == 0 && (revents & POLLIN) != 0) {
            if (endpoint.protocol == PROTOCOL_TCP && endpoint.state() == IS_LISTENING) {
                // Via teulMRecvAddToPoll(), this can change our underlying poll set out from underneath us in
                // the middle of the loop. However, we guarantee by "the code is just written this way for now"
                // that it will only ever add more endpoints to the existing poll data; as a result the maximal
                // side effect is that we will service some endpoints earlier than others, which is harmless.
                error = _acceptConnectTCP(endpoint);
            } else {
                error = _handleReceive(endpoint);
            }
		}

		if ((revents & POLLHUP) != 0 || (revents & POLLERR) != 0) { // check for HUP and ERR even if an error occurred above
			dbugLog(_FLM, "t:%d sock:" PRF_SOCKET " socket disconnected", endpoint.teulIndex(), PRA_SOCKET(sfd.fd));
            if (error == 0) { // Don't overwrite any existing error
                error = -ECONNRESET; // getsockopt(SO_ERROR) has a small chance of telling us more on some platforms, but it's not worth it
            }
		}
        
        // doError() and teulClose() can both recurse, but we need to be able to safely call both.
        // Correctly solving this involves redesigning several parts of the architecture to have a
        // "dyingEndpoints" queue alongside the dead endpoints one, and possibly a generation counter
        // on endpoints to deal with the descriptor reuse problem. Since that's out of scope for the
        // changes in progress at the time of this writing, we call both without a cancellation
        // point in-between (since teulClose() is itself forced to be reentrant). The risk is that we
        // will teulClose() an endpoint which was already closed, cleaned up from the dead endpoints
        // manager, *and* reallocated to a new connection during reentrancy from doError(). This risk
        // can be minimized by reviewing all endpoint error functions and minimizing their own
        // reentrant calls into this machinery.
        if (error < 0) {
            endpoint.doError(-error);
            teulClose(endpoint);
        }
        
        if (currentPollGeneration != pollGeneration.load(std::memory_order_relaxed)) {
            return;
        }

        // This is the major point of possible reentrancy - it's all but guaranteed to reenter in many cases.
        _routeCallbacks(endpoint);

        if (currentPollGeneration != pollGeneration.load(std::memory_order_relaxed)) {
            return;
        }
    }
}

static void _pollAddEndpoint(ENDPOINT& endpoint)
{
    if (endpoint.state() == IS_DEAD || endpoint.socket == INVALID_SOCKET) {
        return;
    }

    auto firstSlot = &pollData.fd[0], lastSlot = &pollData.fd[pollData.m_size];
    auto openSlot = std::find_if(firstSlot, lastSlot, [] (pollfd const &p) { return p.fd == INVALID_SOCKET; });
    
    assert(lastSlot >= openSlot);
    if (openSlot == lastSlot) {
        ABORT_IF_EX(pollData.m_size >= POLL_STRUCT::CAPACITY, "No space left in poll setup");
        ++pollData.m_size;
    }
    nfds_t slotIdx = static_cast<nfds_t>(std::distance(firstSlot, openSlot));
    
    ++pollData.m_count;
    endpoint.m_impl->setPollSlot(slotIdx);
    endpoint.updateSocketEvents();
}

void teulMRecvAddToPoll(ENDPOINT& endpoint)
{
    // This check is made in _pollAddEndpoint() as well, but we want to ignore the
    // error silently there, while logging it for this case. This allows _pollRegenerateSet()
    // to avoid having to deal with the dead endpoints list while still tracking when we
    // do an add that isn't part of that logic.
    if (endpoint.state() == IS_DEAD || endpoint.socket == INVALID_SOCKET) {
        dbugLog(_FLA, "t:%d attempt to add an invalid endpoint to poll", endpoint.teulIndex());
        return;
    }
    _pollAddEndpoint(endpoint);
    dbugLog(_FLD, "t:%d sock:" PRF_SOCKET " adding endpoint state:%d slot:%d count:%" PRSizeT,
                endpoint.teulIndex(), PRA_SOCKET(endpoint.socket), endpoint.state(),
                endpoint.m_impl->pollSlot(), static_cast<size_t>(pollData.m_count));
}

void teulMRecvRemoveFromPoll(ENDPOINT& endpoint, bool permitCompaction)
{
    if (endpoint.socket != INVALID_SOCKET) {
        socketToEndpoint.erase(endpoint.socket);
    }

    nfds_t slot = endpoint.m_impl->pollSlot();
    
    if (slot == std::numeric_limits<nfds_t>::max()) {
        dbugLog(_FLD, "t:%d removing endpoint state:%d not in poll list", endpoint.teulIndex(), endpoint.state());
        return;
    }
    
    assert(pollData.m_count > 0 && pollData.m_size > slot && slot < POLL_STRUCT::CAPACITY);
    
    endpoint.m_impl->setPollSlot(std::numeric_limits<nfds_t>::max());
    pollData.fd[slot].fd = INVALID_SOCKET;
    --pollData.m_count;
    
    dbugLog(_FLD, "t:%d sock:" PRF_SOCKET " removing endpoint state:%d slot:%d count:%" PRSizeT,
                endpoint.teulIndex(), PRA_SOCKET(endpoint.socket),
                endpoint.state(), slot, static_cast<size_t>(pollData.m_count));

    if (pollData.m_count < pollData.m_size / 2 && permitCompaction) {
        // If the global poll structure has more holes than entries, compact it by rebuild.
        _pollRegenerateSet();
    }
}

static void _pollRegenerateSet()
{
    pollData.m_count = 0;
    pollData.m_size = 0;
    pollData.m_maxSock = INVALID_SOCKET;

    for (ENDPOINT * const endpoint : idleEndpoints) {
        assert(endpoint);
        _pollAddEndpoint(*endpoint);
    }

    dbugLog(_FLD, "set up %" PRSizeT " sockets for poll, max " PRF_SOCKET, static_cast<size_t>(pollData.m_count), PRA_SOCKET(pollData.m_maxSock));
}

static void _pollSocketsInner(TeulTimeMS duration_)
{
    int pollret = 0;
    int error = 0;
    
    do {
#if platOS != OS_WIN32
        pollret = poll(&pollData.fd[0], pollData.m_size, duration_);
#else
        pollret = WSAPoll(&pollData.fd[0], pollData.m_size, duration_);
#endif
    } while (pollret < 0 && (error = dbugGetErrno()) == PLAT_EINTR);

    if (pollret < 0) {
        LogAbort("poll returned error %d, and all poll errors are fatal", error);
    }
    _runPollEndpoints(pollData.fd, pollData.m_size, pollret);
}

static void _pollSockets(TeulTimeMS duration_)
{
    // Data consistency check
    // Leaving this commented out for now because it can actually fail in practice due to reentrancy
    // of the teulUpdateTransport() machinery
    //assert(idleEndpoints.empty() ? pollData.m_size == 0 : pollData.m_size > 0);
    
    if (idleEndpoints.empty()) {
        // Yield CPU, even if duration is zero
        std::this_thread::sleep_for(std::chrono::milliseconds(duration_));
    } else {
        _pollSocketsInner(duration_);
    }
}

static void _sendEndpointKlive(ENDPOINT& endpoint)
{
    assert(KLIVE_TOKEN != nullptr);
    
    if (!endpoint.m_doTimeoutCheck) {
        return;
    }
    
    if (endpoint.lastKliveSent.elapsed() < static_cast<TeulDeltaMS>(endpoint.m_impl->kliveInterval)) {
        return;
    }
    
#if defined(DEBUG_CONNECTIONS)
    dbugLog(_FLD, "t:%d sending klive", endpoint.teulIndex());
#endif

    // Warning: Can reenter.
    SINT32 sendResult = teulModSendCALLBACKe(endpoint, *KLIVE_TOKEN);
    
    if (sendResult >= 0) {
        endpoint.lastKliveSent.makeCurrent();
    }
    // Do NOT attempt to report an endpoint error here; teulModSendCALLBACKe() already did so.
}

static void _checkEndpointState(ENDPOINT& endpoint)
{
    switch (endpoint.state())
    {
        // Connection is doing handshakes, etc.
        case IS_OPENING:
            break;

        // Connection is a listener
        case IS_LISTENING:
            break;

        // Connection has opened, needs to do keep-alive.
        case IS_OPEN:
            _sendEndpointKlive(endpoint);
            break;

        // It may reach this state, but why would it still be here?
        case IS_DEAD:
            break;

        default:
            LogAbort("t:%d endpoint in unknown state %d", endpoint.teulIndex(), endpoint.state());
    }
}

void _checkEndpointTimeouts(ENDPOINT& endpoint)
{
    if (!endpoint.m_doTimeoutCheck || endpoint.timeout == 0 || endpoint.state() == IS_DEAD) {
        return;
    }

    TeulDeltaMS elapsed = (TeulDeltaMS)(teulClock64 - endpoint.lastDataRecv.time());
    
    if (elapsed <= endpoint.timeout) {
        return;
    }

    printf("t:%d timeout\n", endpoint.teulIndex());
    dbugLog(_FLN, "t:%d endpoint timeout error, closed timeout:%d elapsed:%" PR64BitT "ms",
                endpoint.teulIndex(), endpoint.timeout, elapsed);
    teulClose(endpoint);
}

static void _idleEndpoint(ENDPOINT& endpoint)
{
    _checkEndpointState(endpoint);
    _checkEndpointTimeouts(endpoint);
}

void ENDPOINT::updateSocketEvents()
{
	nfds_t slot = m_impl->pollSlot();
    
	if (slot == std::numeric_limits<nfds_t>::max()) {
		return;
    }

	socketToEndpoint[socket] = this;
	pollData.fd[slot].fd = socket;
#if defined(POLLRDHUP)
	pollData.fd[slot].events = POLLHUP | POLLRDHUP | POLLIN;
#else
	pollData.fd[slot].events = POLLHUP | POLLIN;
#endif
    
    // Ask for writeability if we're a TCP socket that's waiting for connect() completion, or if
    // we have data to send.
    if ((protocol == PROTOCOL_TCP && state() == IS_OPENING && !m_connected) || !m_impl->m_sendQueue.empty()) {
        pollData.fd[slot].events |= POLLOUT;
    }
	pollData.fd[slot].revents = 0;

	if (pollData.m_maxSock == INVALID_SOCKET) {
		pollData.m_maxSock = socket;
	} else {
		pollData.m_maxSock = std::max(pollData.m_maxSock, socket);
    }
}

static void _attemptDecryption(DATA_GRAM* dgram, char * const decrypted, const UINT32 payloadSize, ENDPOINT& endpoint)
{
    if (dgram->contents->packetToken() == Teul::PacketToken::Unencrypted) {
        return;
    }
    ABORT_UNLESS(dgram->contents->packetToken() == Teul::PacketToken::Encrypted);

    endpoint.decrypt(reinterpret_cast<unsigned char * const>(decrypted), payloadSize);
}

static bool _isValidSynData(const Teul::PacketHeader* const header, size_t payloadSize)
{
	if (payloadSize >= Teul::PacketPrefixBytes && header->m_callX == Teul::Module::CallbackX) {
		switch (header->m_callY) {
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

static bool _checkPacketOrdering(ENDPOINT& endpoint, const Teul::PacketHeader& header)
{
	static_assert(sizeof(Teul::PacketHeader::tag_t) == sizeof(EndpointImpl().recvTag), "Endpoint and Packet disagree on size of packet tags");

	Teul::PacketHeader::tag_t prevTag = endpoint.m_impl->recvTag;
	Teul::PacketHeader::tag_t expectTag = prevTag + 1;

	// The new tag should be *exactly* one higher than the old
	if (header.m_tag != expectTag) {
		dbugLog(_FLW, "t:%d out of order packet rx:%hhu, prev:%hhu, expected:%hhu",
                        endpoint.teulIndex(), header.m_tag, prevTag, expectTag);
		return false;
	}
	endpoint.m_impl->recvTag = header.m_tag;
	return true;
}

static void _processTicks(ENDPOINT& endpoint, const Teul::PacketHeader& header)
{
#if platBuild == BUILD_SERVER
	if (!endpoint.m_parent || !endpoint.isFromClient()) {
		return;
    }

	// The host assigns the created time when the TCP session is started; so the client time should lag
    // a few milliseconds behind the host. If the client gets more than a few ms ahead of the host, we
    // know they are speed hacking.

	if (!endpoint.m_haveTicks) { // true briefly at the start of a connection.
        endpoint.m_impl->m_lastPacketTick = header.m_tick;
        endpoint.m_haveTicks = true;
	}

    // Below logic relies on it being safe to treat the tick values as signed.
    // Tick values are 64 bits, which in signed form gives 300 million years of
    // leeway. That might just barely last long enough for this code to actually
    // get a proper rewrite.
    assert(endpoint.m_impl->m_lastPacketTick < std::numeric_limits<TeulDeltaMS>::max());

	TeulDeltaMS newTick = static_cast<TeulDeltaMS>(header.m_tick);
	TeulDeltaMS lastTick = static_cast<TeulDeltaMS>(endpoint.m_impl->m_lastPacketTick);
    TeulTimeMS delta = static_cast<TeulTimeMS>(std::llabs(newTick - lastTick)); // both values are guaranteed positive; this can not underflow
    
    endpoint.m_tickAccumulation += delta;

	// Save the value.
	endpoint.m_impl->m_lastPacketTick = header.m_tick;
	endpoint.m_impl->m_lastTickRx = teulClock64;

	// Only evaluate every 8th packet for speedhack.
	if ((endpoint.m_impl->recvTag & 7) > 0) {
		return;
    }

	teulSyncClock();

	Teul::TickSample sample(endpoint.m_created.elapsed(), endpoint.m_tickAccumulation);

	if (sample.delta > -100) { // In other words, if the client is less than 100ms ahead of the host
		return;
    }

	FLOAT64 minRatio = host_config.arena.speedHackRatio;
    
    // All is well if detection is set above 1.0 (off) or the client:host ratio is below the threshold
	if (minRatio >= 1.0 & sample.ratio < minRatio) {
		return;
    }
    
    // Oops. Fire a speedhack error.
    // Do NOT teulClose(); we don't want to tip off the hacker of the detection.
	endpoint.doError(ETEUL_ILLEGAL_TICKS, &sample);
#endif
}

static void _executeCallback(ENDPOINT& endpoint, DATA_GRAM* const dgram)
{
	Teul::PacketHeader * const hdr = dgram->contents;
	char * const decryptedData = dgram->decryptedDataStart();
	size_t payloadSize_bytes = dgram->m_size - Teul::PacketPrefixBytes;
    
	_attemptDecryption(dgram, decryptedData, payloadSize_bytes, endpoint);

	switch (endpoint.state())
	{
        case IS_OPENING:
            // when the state == IS_OPENING, we only accept SYN tokens.
            // if they aren't recognized we assume that the encryption has failed
            // and try a different encryption
            if (!_isValidSynData(hdr, payloadSize_bytes)) {
                dbugLog(_FLA, "t:%d non-syn data in opening mode", endpoint.teulIndex());
                return;
            }
            break;
        case IS_LISTENING:
            ABORT_IF_EX(hdr->m_callX != Teul::Module::CallbackX, "t:%d listening sockets must NOT receive callbacks", endpoint.teulIndex());
            break;
        default:
            break;
    }

	// check for someone hacking our TCP stream by deleting packets...
	if (endpoint.m_reliable) {
		if (!_checkPacketOrdering(endpoint, *hdr)) {
            // This can recurse but we're already expecting that here.
            endpoint.doError(ETEUL_ILLEGAL_PACKET_ORDER);
            teulClose(endpoint);
			return;
        }
		_processTicks(endpoint, *hdr);
	}

	TEUL_CALLBACK * const callback = TEUL_CALLBACK::Lookup(hdr->m_callX, hdr->m_callY);

	if (!callback) {
		dbugLog(_FLA, "t:%d ip:%s cb:%u,%u callback does not exist", endpoint.teulIndex(), teulGetAddress(endpoint.fromAddress()), hdr->m_callX, hdr->m_callY);
		return;
	}

	if (hdr->m_callX == Teul::Serv::CallbackX && !endpoint.m_peer) {
		dbugLog(_FLA,"t:%d ip:%s cb:%d,%d callback access denied", endpoint.teulIndex(), teulGetAddress(endpoint.fromAddress()), hdr->m_callX, hdr->m_callY);
		return;
	}

	callback->processCallback(endpoint, reinterpret_cast<UCHAR * const>(dgram->callbackPayload()), dgram->m_size - sizeof(Teul::PacketHeader));
}

static void _routeCallbacks(ENDPOINT& endpoint)
{
	while (!endpoint.m_impl->m_recvQueue.empty()) {
		DATA_GRAM * const datagram = endpoint.m_impl->m_recvQueue.front();
		endpoint.m_impl->m_recvQueue.pop_front();

		// Temporarily set the endpoint address to correspond to the sender so responses "just work".
        // This is really bad and should be fixed.
		auto saveFromAddress = endpoint.fromSockAddress;
        
		endpoint.fromSockAddress = datagram->sockAddress;
		_executeCallback(endpoint, datagram); // this can reenter!
		endpoint.fromSockAddress = saveFromAddress;

		teulReleaseDatagram(datagram);
	}
}

#ifdef DEBUG
static constexpr const TeulTimeMS AlertInterval_ms = 2 * 1000;
#else
static constexpr const TeulTimeMS AlertInterval_ms = 75;
#endif

SINT32 teulModIdle(const TeulTimeMS duration_)
{
	teulSyncClock();

	// At 1s intervals, kill dead endpoints
    static TeulTimeMS lastDeadEndpointsCleanup = 0;
    
	if (teulClock64 - std::min(teulClock64, lastDeadEndpointsCleanup) > 1000) {
		lastDeadEndpointsCleanup = teulClock64;
		teulKillDeadEndpoints();
	}

    // Poll sockets. This will yield the CPU for at least one context switch even if duration is zero.
	_pollSockets(duration_);

    // Warn about teul stalls (failure to give the network stack a CPU slice to use for updating)
    static TeulTimeMS lastModIdle = 0;
    
	if (teulClock64 - std::min(teulClock64, lastModIdle) > duration_ + AlertInterval_ms && lastModIdle != 0) {
		dbugLog(_FLW, "STALL. teulModIdle interval=%" PR64BitUT "", teulClock64 - std::min(teulClock64, lastModIdle));
    }
	lastModIdle = teulClock64;
    
    // Only run the endpoint idles (klives and timeouts) every 500ms
	static TeulTimeMS lastIdleCycle = teulClock64;

	if (teulClock64 - std::min(teulClock64, lastIdleCycle) >= ENDPOINT_IDLE_INTERVAL) {
        for (ENDPOINT * const ep : idleEndpoints) {
            if (ep) {
                _idleEndpoint(*ep);
            }
        }
    }
	lastIdleCycle = teulClock64;
	return 0;
}

void ENDPOINT::setConnected()
{
	m_connected = true;
	updateSocketEvents();
}

void ENDPOINT::setWritesPending(const bool pending_)
{
	m_writesPending = pending_;
	updateSocketEvents();
}
