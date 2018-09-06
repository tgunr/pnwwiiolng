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

//#define DEBUG_RECV_PACKET

#include "mem/mem.h"
#include "server/serverInterfaceDescription.h"
#include "server/serverServerInfo.h"
#include "teul/teulBasic.h"
#include "teul/teulCallbackStruct.h"
#include "teul/teulEndpointIdleList.h"
#include "teul/teulModule.h"
#include "teul/teulPackers.h"
#include "teul/teulPacket.h"
#include "teul/teulPacketHeader.h"
#include "teul/teulTickSample.h"
#include "teul/teulTime.h"
#include "wwii/wwiiConfig.h"
#include "wwii/wwiiNetPorts.h"

using namespace Teul;

#if platOS == OS_WIN32
# define WINDOWS_VERSION
# define VALID_SOCKET(n) ( (n) != (INVALID_SOCKET) )
#else
# define NIX_VERSION
# define VALID_SOCKET(n) ( (n) >= (0) )
#endif

#if defined(NIX_VERSION)
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <poll.h>
#include <fcntl.h>
#endif

#if defined(WINDOWS_VERSION)
#include <winsock2.h>
#endif

#include "teul/teulEndpoint.h"
#include "teul/teulEndpointImpl.h"
#include "teul/teulModCalls.h"
#include "teul/teulSockConfig.h"

#include "teul/teulConstants.h"

#define COMMAND_UPDATE_INTERVAL	10

#define MAX_POLL_SOCK	16384
#define ENDPOINT_IDLE_INTERVAL 500

#if platOS == OS_WIN32 && CHECK_PERFORMANCE
static SINT32 iterations;
static _int64 first, last, diff, freq;
static FLOAT64	dur, min, max, average, total;

#endif

SINT32	numTeulUpdates = 0;
static TeulTimeMS	lastTeulUpdate = 0;
static TeulTimeMS	averageDelta = 0;
static TeulTimeMS	maxDelta	= 0;

SINT32 _updatesPerSec = 0;
SINT32 _updatesThisSec = 0;

#if defined(WINDOWS_VERSION)
static fd_set 		master_readfs, master_writefs;//, exceptfs;
static size_t		maxSock;
static bool			_needSetFs = true;
#else
// Linux uses "poll()" for checking sockets for events.
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
#endif

// To save ourselves having to look up endpoints later...
#include <map>
using SocketToEndpoint = std::map<SINT32, ENDPOINT*>;
static SocketToEndpoint socketToEndpoint;

static TeulTimeMS	_lastIdle;

static void	_pollSockets(TeulTimeMS duration_);

static void	_routeCALLBACKs(ENDPOINT& inEndpoint);

#if defined(NIX_VERSION)
static void _pollSet();
#endif

static const socklen_t SockAddrLen = sizeof(struct sockaddr_in);

struct DATA_GRAM
{
	struct sockaddr_in	sockAddress;
	Teul::PacketHeader*	contents;	//!< Points to the beginning of the packet data.
	UINT32				m_size;		//!< Size of the entire object (might be smaller than allocated size)
	UINT32				m_reqSize;	//!< Original requested size of bucket
};

#include <stack>
#include <array>

static const size_t NUM_DATAGRAM_SIZES = 16;
static const size_t DATAGRAM_ROUNDING = 1024;	// round datagram sizes up to this.
static std::array<std::stack<DATA_GRAM*>, NUM_DATAGRAM_SIZES> s_freeDatagrams;
static const size_t LARGEST_POOLED_SIZE = NUM_DATAGRAM_SIZES * DATAGRAM_ROUNDING;

//////////////////////////////////////////////////////////////////////
// Given a requested data size, work out the best buffer size for it

template<size_t roundingValue>
static inline size_t _roundDatagramSize(size_t size)
{
	static_assert(roundingValue && (roundingValue & (roundingValue - 1)) == 0, "roundingValue must be a power of 2");
	return ((size + sizeof(DATA_GRAM)) + (roundingValue - 1)) & ~(roundingValue - 1);
}

static DATA_GRAM* _getNextDatagram(size_t size)
{
	assert(size > 0);
	if (PN_UNLIKELY(size <= 0))
		return nullptr;

	size_t allocSize = _roundDatagramSize<DATAGRAM_ROUNDING>(size);
	// Determine if the packet is in the range of sizes we try to pool.
	if (PN_LIKELY(allocSize <= LARGEST_POOLED_SIZE))
	{
		const size_t bucketNo = (allocSize / DATAGRAM_ROUNDING) - 1;
		auto& bucket = s_freeDatagrams[bucketNo];
		if (bucket.empty() == false)
		{
			DATA_GRAM* gram = bucket.top();
			bucket.pop();
			return gram;
		}
	}
	else
	{
		// use more aggressive, 4k (page size) rounding for big packets.
		allocSize = _roundDatagramSize<DATAGRAM_ROUNDING * 4>(size);
	}
	return static_cast<DATA_GRAM*>(memCalloc(1, allocSize));
}

DATA_GRAM *
teulGetDatagram(const UINT32 size)
{
	DATA_GRAM* gram = _getNextDatagram(size);
	// Point contents to memory immediately AFTER the gram header.
	gram->contents = (Teul::PacketHeader*)(gram + 1);
	gram->m_size = size;		// The actual size (for now size requested)
	gram->m_reqSize = size;		// The actual requested size.

	return gram;
}

//////////////////////////////////////////////////////////////////////
// Release a datagram - if it fits into the cache buckets, put it there.

void
teulReleaseDatagram(DATA_GRAM* gram)
{
	size_t allocSize = _roundDatagramSize<DATAGRAM_ROUNDING>(gram->m_reqSize);
	if (PN_LIKELY(allocSize <= s_freeDatagrams.size() * DATAGRAM_ROUNDING))
		s_freeDatagrams[(allocSize / DATAGRAM_ROUNDING) - 1].push(gram);
	else
	{
		memFree(gram);
	}
}

//////////////////////////////////////////////////////////////////////
// Initialize module

void teulInitModRecv()
{
#if defined(NIX_VERSION)
	dbugLog(_FLI, "FD_SETSIZE = %u, sizeof(fd_set) = %" PRSizeT ", MAX_POLL_FD = %" PRSizeT, FD_SETSIZE, sizeof(fd_set), POLL_STRUCT::CAPACITY);

	memset(&pollData, 0, sizeof(pollData));
	for (size_t i = 0; i < pollData.CAPACITY; i++)
	{
		pollData.fd[i].fd = -1;
	}
	_pollSet();
#endif

	socketToEndpoint.clear();

	_lastIdle = teulClock64;
	lastTeulUpdate = teulClock64;
}

void teulCleanupModRecv()
{
	for (size_t i = 0; i < s_freeDatagrams.size(); ++i)
	{
		auto& bucket = s_freeDatagrams[i];
		dbugLog(_FLN, "Freeing %" PRSizeT "Kb DATA_GRAMs: %" PRSizeT "", (i * DATAGRAM_ROUNDING) / 1024, bucket.size());
		while (bucket.empty() == false)
		{
			memFree(bucket.top());
			bucket.pop();
		}
	}
}

static void
_connectionRefused(ENDPOINT& endpoint, const SINT32 error)
{
	dbugLog(_FLA, "t:%d connection refused.", endpoint.teulIndex());

	dbugSetErrno(error);
	endpoint.doError(error);
	teulClose(endpoint, Teul::Closing::Refused);

	dbugSetErrno(error);
}


static void
_checkConnectTCP(ENDPOINT& endpoint)
{
	SINT32 retval;
	socklen_t len;
	sockval_t error;
	const SOCKET& sock = endpoint.socket;

#if defined(WINDOWS_VERSION)
	retval = connect(sock, (struct sockaddr*)&endpoint.destSockAddress, sizeof(endpoint.destSockAddress));
	error = dbugGetErrno();
	dbugWrite(_FLN, "t:%d sock:" PRF_SOCKET " check connect err:%d retval:%d"
					, endpoint.teulIndex(), PRA_SOCKET(sock), error, retval);
//	if (retval < 0 && error == EISCONN)
#else
	char* err = (char*) &error;
	len = sizeof(error);
	retval = getsockopt(sock, SOL_SOCKET, SO_ERROR, err, &len);
	if ( retval == 0 && error == 0 )
#endif
	{
		endpoint.lastDataRecv.makeCurrent();
		endpoint.lastDataSent.makeCurrent();

		len = SockAddrLen;
		getsockname(sock, (struct sockaddr*)&endpoint.localSockAddress, &len);
		assert( len == SockAddrLen );
		endpoint.setConnected();
		retval = PN_LIKELY(SYN_TOKEN != nullptr) ? teulModSendCALLBACKe(endpoint, *SYN_TOKEN) : -EINVAL;
		if (PN_UNLIKELY(retval < 0))
			dbugLog(_FLA, "t:%d sock:" PRF_SOCKET " new connection send failed err:%d"
						, endpoint.teulIndex(), PRA_SOCKET(sock), retval);
//		return 0;
	}
#if defined(NIX_VERSION)
	else
	{
		dbugWrite(_FLM, "t:%d sock:" PRF_SOCKET " check connect err:%d rev:%d"
						, endpoint.teulIndex(), PRA_SOCKET(sock), error, retval);
		if (error == PLAT_EINTR || error == EROFS)
		{
			len = SockAddrLen;
			if (getsockopt(sock, SOL_SOCKET, SO_ERROR, err, &len) == 0 && error == 0)
			{
				endpoint.lastDataRecv.makeCurrent();
				endpoint.lastDataSent.makeCurrent();
				len = SockAddrLen;
				getsockname(sock, (struct sockaddr*) &endpoint.localSockAddress, &len);
				dbugLog(_FLA, "t:%d sock:" PRF_SOCKET " open err:%d retval:%d"
							, endpoint.teulIndex(), PRA_SOCKET(sock), error, retval);
				endpoint.setConnected();
				retval = PN_LIKELY(SYN_TOKEN != nullptr) ? teulModSendCALLBACKe(endpoint, *SYN_TOKEN) : -EINVAL;
				if ( retval < 0 )
				{
					dbugLog(_FLA,"t:%d sock:" PRF_SOCKET " couldn't send to new connection err:%d"
								, endpoint.teulIndex(), PRA_SOCKET(sock), retval);
					teulClose(endpoint, Teul::Closing::ConnSend);
				}
//				return 0;
			}
			else
			{
				dbugLog(_FLA, "t:%d sock:" PRF_SOCKET " connect error, closing", endpoint.teulIndex(), PRA_SOCKET(sock));
				teulClose(endpoint, Teul::Closing::ConnError);
//				return -ENOTCONN;
			}
		}
		else if ( error == ECONNREFUSED )
		{
			_connectionRefused(endpoint, error);
		}
		else
		{
			dbugLog(_FLA,"t:%d sock:" PRF_SOCKET " open err:%d retval:%d", endpoint.teulIndex(), PRA_SOCKET(sock), error, retval);
			dbugSetErrno(error);
//			return -error;
		}
	}
#endif
}

static void
_acceptConnectTCP(ENDPOINT& endpoint)
{
	u_long argp = 1;

	dbugLog(_FLD, "t:%d entered _acceptConnectTCP, time:%" PR64BitUT "", endpoint.teulIndex(), teulClock64);
	// Multiple simultaneous incoming connections may only generate a single "event", so while we
	// are here, we should try and drain any/all incoming connections queued up (hence the loop).
	for ( ; ; )
	{
		socklen_t len = SockAddrLen;
		SOCKET newSock = accept(endpoint.socket, (struct sockaddr*) &endpoint.fromSockAddress, &len);
		if ( newSock == INVALID_SOCKET )
		{
			// Nothing to accept, we've drained the socket of incoming connections.
			return;
		}

		// set socket to non-blocking.
		const SINT32 retval = ioctlsocket(newSock, FIONBIO, &argp);
		if ( PN_UNLIKELY(retval < 0) )
		{
			dbugLog(_FLA, "t:%d could not accept connection", endpoint.teulIndex());
			closesocket(newSock);
			continue;
		}

		dbugLog(_FLI, "t:%d sock:" PRF_SOCKET " accepting connection", endpoint.teulIndex(), PRA_SOCKET(newSock));

		ENDPOINT* newEndpoint = teulCreateEndpoint(false, endpoint.timeout, (PROTOCOL)endpoint.protocol);
		if ( PN_UNLIKELY(newEndpoint == nullptr) )
		{
			shutdown(newSock, 0x02);
			closesocket(newSock);
			continue;
		}

		newEndpoint->setConnected();
		newEndpoint->m_parent = &endpoint;
		newEndpoint->socket = newSock;
#if platBuild == BUILD_SERVER
		newEndpoint->m_peer = (endpoint.localPort() == WWII::Net::Ports::ServerPeering + Server::myServerInfo.stats.type);
#else
		newEndpoint->m_peer = false;
#endif

		// Look up the address of our side of the socket.
		len = SockAddrLen;
		getsockname(newSock, (sockaddr*)&newEndpoint->localSockAddress, &len);
		assert( len == SockAddrLen );

		// endpoints created inherit the parents backchannel
		newEndpoint->m_pairdUdpConn = endpoint.m_pairdUdpConn;
		newEndpoint->fromSockAddress = endpoint.fromSockAddress;

		// If we only have a partial address, use the local address to fill it out.
		static const UINT32 netMask = 0x00ffffff;
		if ( (newEndpoint->fromSockAddress.sin_addr.s_addr & netMask) == 0 )
				newEndpoint->fromSockAddress.sin_addr.s_addr |= (newEndpoint->localSockAddress.sin_addr.s_addr & netMask);
		newEndpoint->destSockAddress = newEndpoint->fromSockAddress;

		char remoteIP[32];
		teulGetAddress(newEndpoint->destAddress(), remoteIP, sizeof(remoteIP));
		char localIP[32];
		teulGetAddress(newEndpoint->localAddress(), localIP, sizeof(localIP));

		dbugLog(_FLN, "t:%d accepted %s:%u->%s:%u as t:%d sock:" PRF_SOCKET " (li:%" PR64BitUT ")"
					, endpoint.teulIndex()
					, remoteIP, newEndpoint->fromPort()
					, localIP, newEndpoint->localPort()
					, newEndpoint->teulIndex()
					, PRA_SOCKET(newSock)
					, (teulClock64 - _lastIdle)
					);

#if platOS != OS_WIN32
		newEndpoint->setRcvBufferSize(16 * 1024);
		newEndpoint->setSndBufferSize(4 * 1024);
#endif
//			teulModReadSet();
		teulMRecvAddToPoll(*newEndpoint);
	}
}

static SINT32
_receiveTCP(ENDPOINT& endpoint, void* const ioData, UINT32* const ioSize, UINT32* const outFlags)
{
	const SOCKET sock = endpoint.socket;

	SINT32 retval = (SINT32)recv(sock, (char*) ioData, *ioSize, *outFlags);
	if (retval == 0)
	{
		*ioSize = 0;

		SINT32 err = 0;

#if platOS != OS_WIN32
		err = errno;
		const bool notCompleteYet = (err == PLAT_EINTR) | (err == PLAT_EAGAIN) | (err == PLAT_EWOULDBLOCK);
		if ( notCompleteYet )
		{
			retval = -err;
		}
		else
#endif
		{
			// When recv() returns 0, this indicates that the connection has closed.
			// TeulKit doesn't distinguish between deliberate and fault-related
			// closure; in both cases we notify the application of closure with
			// an "error"; it is then left to the application to track whether
			// it is important or not.

			err = dbugGetErrno();
			dbugLog(_FLN, "t:%d sock:" PRF_SOCKET " tcp close: end of stream (recv:0, err:%d), time %" PR64BitUT "",
					endpoint.teulIndex(), PRA_SOCKET(sock), err, teulClock64);

			// Tear down all of the teulkit abstractions over the socket and
			// send notification to the application.
			teulClose(endpoint, Teul::Closing::Closed);
			return retval;
		}
	}

	if (retval <= 0)
	{
		*ioSize = 0;
		const SINT32 err = dbugGetErrno();
		const bool notCompleteYet = (err == PLAT_EINTR) | (err == PLAT_EAGAIN) | (err == PLAT_EWOULDBLOCK);
		
		if ( notCompleteYet ) {
			// This is not an error, simply means data isn't available yet
			return -err;
		} else if ( err == ECONNREFUSED ) {
			_connectionRefused(endpoint, err);
		} else {
			dbugLog(_FLN, "t:%d sock:" PRF_SOCKET " read err:%d retval:%d", endpoint.teulIndex(), PRA_SOCKET(sock), err, retval);
			teulClose(endpoint, Teul::Closing::ReadErr);
			dbugSetErrno(err);
		}
	}
	else
	{
		*ioSize = retval;
		// Got some data.
		endpoint.lastDataRecv.makeCurrent();
	}

	return retval;
}

static SINT32
_startNewPacket(ENDPOINT& endpoint)
{
	// Start by reading the first 3 bytes from the stream,
	// which should comprise the start token and the two-byte
	// length value.
	static const size_t LeadInSizeBytes = sizeof(UCHAR) + sizeof(UINT16);

	///TODO: Read in as much data as we can and split it into packets.
	UCHAR* const leadinBuffer = endpoint.m_readAheadBuffer;

	UINT32 readSize = sizeof(endpoint.m_readAheadBuffer) - endpoint.m_readBufferUsed;
	UINT32 flags = 0; // Don't actually pull the data.
	SINT32 retval = _receiveTCP(endpoint, leadinBuffer + endpoint.m_readBufferUsed, &readSize, &flags);
	if ( retval <= 0 )
		return retval;

	// Verify it has valid prefix data.
	const Teul::PacketHeader* const header = (Teul::PacketHeader*)leadinBuffer;
	const UINT32 token = header->packetType();
	if ( token != (UINT32)Teul::Tokens::Encrypted && token != (UINT32)Teul::Tokens::Unencrypted )
	{
		dbugOutputPacket( endpoint, true, leadinBuffer, readSize );

		dbugLog(_FLN, "t:%d garbage packet prefix %u", endpoint.teulIndex(), token);
		teulClose(endpoint, Teul::Closing::Prefix);
		return -EINVAL;
	}

	const size_t totalRead = endpoint.m_readBufferUsed + readSize;

	// It's possible, but unlikely, that retransmits may cause a case
	// where the first byte of a TeulFrame is the last byte of a TCPFrame
	// in which case we should just leave it there for a bit.
	if ( totalRead < LeadInSizeBytes )
	{
		dbugOutputPacket( endpoint, true, leadinBuffer, readSize );

		dbugLog(_FLN, "t:%d garbage short packets, ignoring.", endpoint.teulIndex());
		endpoint.m_readBufferUsed = totalRead;
		dbugSetErrno(0);
		return -1;
	}

	size_t size = header->packetSizeBytes();
	if ( size < 1 || size > Teul::MaxAllowedPacketSize_bytes )
	{
		dbugOutputPacket( endpoint, true, leadinBuffer, readSize );

		dbugLog(_FLN, "t:%d invalid packet size: %" PRSizeT, endpoint.teulIndex(), size);
		teulClose(endpoint, Teul::Closing::BadSize);
		return -EINVAL;
	}

	// Add the envelope size.
	size += Teul::Packet::UnencryptedBytes;

	DATA_GRAM* dgram = endpoint.m_impl->m_curDatagram = teulGetDatagram(size);
	dgram->sockAddress = endpoint.destSockAddress;

	memcpy(dgram->contents, leadinBuffer, totalRead);
	endpoint.m_readBufferUsed = totalRead;

	return 1;
}

static SINT32
_continuePacket(ENDPOINT& endpoint)
{
	UINT32 readSize = endpoint.m_impl->m_curDatagram->m_size - endpoint.m_readBufferUsed;
	UINT32 flags = 0;
	UCHAR* const readBuffer = (UCHAR*)endpoint.m_impl->m_curDatagram->contents + endpoint.m_readBufferUsed;
	SINT32 retval = _receiveTCP(endpoint, readBuffer, &readSize, &flags);
	if ( retval <= 0 )
	{
		// error flows up, might not be fatal - might be a wait
		return retval;
	}

	endpoint.m_readBufferUsed += readSize;

	return readSize;
}

static SINT32
_recvfromStream(ENDPOINT& endpoint)
{
	// Most of the time we should get packets in their own frames.
	if ( endpoint.m_impl->m_curDatagram == nullptr )
	{
		const SINT32 retval = _startNewPacket(endpoint);
		if ( retval <= 0 || endpoint.m_impl->m_curDatagram == nullptr )
			return retval;
	}

	const UINT32 datagramSize = endpoint.m_impl->m_curDatagram->m_size;
	if ( endpoint.m_readBufferUsed < datagramSize )
	{
		const SINT32 retval = _continuePacket(endpoint);
		if ( retval <= 0 )
		{
			// might not be fatal, might just be wait condition
			return retval;
		}
#ifdef DEBUG_RECV_PACKET
		const DATA_GRAM& dgram = *endpoint.m_impl->m_curDatagram;
		// Note that the start logic above will make it so every packet starts with three bytes.
		dbugLog(_FLD, "t:%d RecvPacket%s [%u:%u] : %u(+%d)/%u bytes",
		        endpoint.teulIndex(), endpoint.m_readBufferUsed < dgram.m_size ? "Cont" : "End",
		        dgram.contents->m_callX, dgram.contents->m_callY,
		        endpoint.m_readBufferUsed, retval, dgram.m_size);
#endif

		// Don't pass here until we've read all of the payload.
		if ( endpoint.m_readBufferUsed < datagramSize )
		{
			return 0;
		}
	}

    dbugOutputPacket( endpoint, true, endpoint.m_impl->m_curDatagram->contents, endpoint.m_readBufferUsed );

	endpoint.m_impl->recvQueue.push_back(endpoint.m_impl->m_curDatagram);
	endpoint.m_impl->m_curDatagram = nullptr;
	endpoint.m_readBufferUsed = 0;

	return (SINT32)datagramSize;
}

static SINT32
_recvfromPacketSource(ENDPOINT& endpoint)
{
	DATA_GRAM* dgram = nullptr;
	socklen_t sockSize = sizeof(dgram->sockAddress);

	UINT32 packets = 0;
	while ( VALID_SOCKET(endpoint.socket) )
	{
		if ( ! dgram)
			dgram = teulGetDatagram(Teul::MaxMTU + 4);

		Teul::PacketHeader* const hdr = dgram->contents;

		const int rxSize = (SINT32)recvfrom(endpoint.socket, (char*)hdr, Teul::MaxMTU, 0, (struct sockaddr*)&dgram->sockAddress, &sockSize);
		if ( rxSize <= 0 )
			break;

#if platBuild == BUILD_SERVER
		// ignore packets we broadcast..
		if ( endpoint.protocol == PROTOCOL_UDB && (dgram->sockAddress.sin_addr.s_addr & 0xff000000) == (Server::myInterface.addr[0].naddr & 0xff000000) )
		{
			dbugWrite(_FLD, "ignoring packet from %s!", teulGetAddress(ntohl(dgram->sockAddress.sin_addr.s_addr)) );
			continue;
		}
		else
#endif
		{
			++packets;

			const UINT32 dgramSize = hdr->packetSizeBytes() + Teul::Packet::UnencryptedBytes;
			if (dgramSize == (UINT32)rxSize)
			{
				dgram->m_size = dgramSize;

				endpoint.m_impl->recvQueue.push_back(dgram);
				dgram = nullptr;
			}
			else
			{
				dbugLog(_FLW, "Received datagram from %s for packet of size %d where the size indicated was %u, ignoring!!!",
					teulGetAddress(ntohl(dgram->sockAddress.sin_addr.s_addr)),
					rxSize,
					dgramSize);
				break;
			}
		}

	}

	if (dgram)
		teulReleaseDatagram(dgram);

	return packets;
}

SINT32
_recvDatagram(ENDPOINT& endpoint)
{
	if ( PN_UNLIKELY(!VALID_SOCKET(endpoint.socket)) )
		return -EINVAL;

	SINT32 retval = -1;
	if ( endpoint.protocol == PROTOCOL_TCP || endpoint.protocol == PROTOCOL_CTRL )
	{
		UINT32 packetsReceived = 0;
		UINT32 errorCount = 0;
		do
		{
			retval = _recvfromStream(endpoint);
			if ( retval == 0 )
			{
				break;
			}

			int testWait = retval * -1;
			if ( (testWait == PLAT_EINTR) || (testWait == PLAT_EAGAIN) || (testWait == PLAT_EWOULDBLOCK))
			{
				retval = 0;
				// these are legit no data non-errors that should be ignored
				break;
			}
			else if (retval < 0)
			{
				const int err = dbugGetErrno();
				if ( err != PLAT_EINTR && err != 0 )
				{
					dbugLog(_FLD, "Non EINTR error detected %d setting return value to %d", retval, packetsReceived);
					retval = packetsReceived;
					break;
				}
				errorCount += 1;
				dbugLog(_FLD, "Error count now %d last error %d", errorCount, err);
			}
			else
				++packetsReceived;
		} while ( errorCount < 16 && endpoint.m_impl->recvQueue.size() == 0 );

		if ( errorCount > 2 )
			dbugLog(_FLD, "t:%d sock:" PRF_SOCKET " packets:%u errors:%u", endpoint.teulIndex(), PRA_SOCKET(endpoint.socket), packetsReceived, errorCount);
	}
	else if ( endpoint.m_reliable == false )
	{
		retval = _recvfromPacketSource(endpoint);
	}
	else
	{
		LogAbort("Invalid endpoint type");
	}

	if ( retval > 0 && (endpoint.protocol != PROTOCOL_TCP || endpoint.m_reliable == false) )
		endpoint.lastDataRecv.makeCurrent();

	return retval;
}

#if defined(NIX_VERSION)
//////////////////////////////////////////////////////////////////////
// Process a receive event on an endpoint.
// Returns true if the connection got closed.

static bool
_handleReceive(ENDPOINT& endpoint)
{
	SINT32 theErr = _recvDatagram(endpoint);

	if ( PN_UNLIKELY(theErr < 0) )
	{
		theErr = dbugGetErrno();
		if ( theErr != 0 )
		{
			dbugSetErrno(0);
			if ( (theErr == PLAT_EAGAIN) | (theErr == PLAT_EWOULDBLOCK) | (theErr == PLAT_EINTR) )
				return false;
			else
			{
				dbugLog(_FLI, "t:%d endpoint error %d; closing", endpoint.teulIndex(), theErr);
				teulClose(endpoint, Teul::Closing::RecvError);
				return true;
			}
		}
	}

	if ( PN_UNLIKELY(endpoint.state() >= IS_CLOSED) )
	{
		return true;
	}
	_routeCALLBACKs(endpoint);
	return false;
}

static void
_runPollEndpoints(struct pollfd* fdArray, nfds_t maxSlots, int numSlots)
{
	SINT32		count = 0, loopCount = 0;

	for ( size_t slot = 0; slot < maxSlots && count < numSlots; ++slot )
	{
		loopCount++;

		pollfd& sfd = fdArray[slot];

		const SOCKET sock = sfd.fd;
		if ( sock < 0 )
		{
			continue;
		}

		const short revents = sfd.revents;
		if ( revents == 0 )
		{
			continue;
		}
		sfd.revents = 0;

		count++;

		SocketToEndpoint::iterator it = socketToEndpoint.find(sock);
		if ( it == socketToEndpoint.end() )
		{
			// not sure how we get here yet but I know its bad...
			dbugLog(_FLI, "sock:" PRF_SOCKET " attempt to use invalid null endpoint whacking slot %" PRSizeT "", PRA_SOCKET(sock), slot);
			sfd.fd = INVALID_SOCKET;
			continue;
		}
		assert( it->second != nullptr );

		ENDPOINT& endpoint = *(it->second);

		if ( (endpoint.m_inUse == false) || (endpoint.socket == INVALID_SOCKET) )
		{
			dbugLog(_FLI,"t:%d attempted used of invalid endpoint", endpoint.teulIndex());
			teulMRecvRemoveFromPoll(endpoint);
			continue;
		}

		if ( (revents & POLLOUT) != 0 )
		{
            if ( endpoint.protocol == PROTOCOL_TCP )
            {
                if ( endpoint.state() == IS_OPENING && !endpoint.m_connected )
                {
                    // looks like the TCP connection is open, double check
                    _checkConnectTCP(endpoint);
                    continue;                      // service the next endpoint
                }
            }
            endpoint.pushSendQueue();
		}

		if ( (revents & POLLIN) != 0 )
		{
            if ( endpoint.protocol == PROTOCOL_TCP )
            {
                if (endpoint.state() == IS_LISTENING )
                {
                    _acceptConnectTCP(endpoint);
                    continue;
                }
            }
            if ( _handleReceive(endpoint) ) {
				continue;
            }
		}

#if defined(POLLRDHUP)
		if ( PN_UNLIKELY((revents & POLLRDHUP) != 0) )
		{
			dbugLog(_FLD, "t:%d sock:" PRF_SOCKET " remote disconnected", endpoint.teulIndex(), PRA_SOCKET(sock));
			teulClose(endpoint, Teul::Closing::RemoteDisc);
			continue;
		}
#endif

		if ( PN_UNLIKELY((revents & POLLHUP) != 0) )
		{
			dbugLog(_FLM, "t:%d sock:" PRF_SOCKET " socket terminated", endpoint.teulIndex(), PRA_SOCKET(sock));
			teulClose(endpoint, Teul::Closing::LocalDisc);
			continue;
		}
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
			if (endpoint.m_closed == false)
			{
				dbugWrite(_FLN,"IS_CLOSING");
				endpoint.doError(ENOTCONN);
			}
			if ( ((endpoint.m_closed == false) || (endpoint.lastKliveSent.elapsed() >= Teul::FynNeeded)) && (FYN_TOKEN != nullptr) )
			{
				teulModSendCALLBACKe(endpoint, *FYN_TOKEN);
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
		teulClose(endpoint, Teul::Closing::Timeout);
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
	teulClose(endpoint, Teul::Closing::EndpointError);
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
							teulClose(endpoint, Teul::Closing::RcvDgramError);
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
				teulClose(endpoint, Teul::Closing::FDClose);
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
	_needSetFs = true;
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
	if ( dgram->contents->packetType() == (UINT32)Teul::Tokens::Unencrypted )
		return;
	ABORT_UNLESS(dgram->contents->packetType() == (UINT32)Teul::Tokens::Encrypted);

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
		teulClose(endpoint, ETEUL_ILLEGAL_PACKET_ORDER);
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

	const UINT32 payloadSize_bytes = dgram->m_size - Teul::Packet::UnencryptedBytes;
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
		_updatesPerSec = _updatesThisSec;
		_updatesThisSec = 0;
		lastUpdateTime = teulClock64;
		teulKillDeadEndpoints();
	}

	_updatesThisSec++;

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
