#include <syscfg.h> // Path is project specific.
// This module provides access to the socket
// functions used by the teul API. The end-user developer
// should never need the functions provided here.
//
// Copyright 1999 Playnet Inc, All rights reserved
// Not to be distributed in API form except as part of a compiled
// library
//
// Originating Programmer: Martin Poulin

#if platOS == OS_LINUX
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <netinet/tcp.h>
	#include <arpa/inet.h>
	#include <sys/un.h>
	#include <sys/ioctl.h>
	#include <limits.h>
	#include <sys/types.h>
#elif platOS == OS_MACOS
	#include <sys/types.h>
	#include <netinet/in.h>
	#include <netinet/tcp.h>
	#include <sys/socket.h>
	#include <arpa/inet.h>
#elif platOS == OS_WIN32
	#include <sys/types.h>
	#include <winsock2.h>
	#include <windows.h>
#endif

#include <string.h>
#include "include/inplace_vector.h"
#include <queue>
#include "teul/teulBasic.h"
#include "teul/teulEndpoint.h"
#include "teul/teulEndpointIdleList.h"
#include "teul/teulEndpointImpl.h"
#include "teul/teulModule.h"
#include "teul/teulPacket.h"
#include "teul/teulSockConfig.h"
#include "teul/teulTime.h"
#include "teul/teulUtils.h"
#include "teul/teulVersion.h"

//#define DEBUG_ENCRYPTION
//#define DEBUG_CONNECTIONS

static size_t _teulConnects;

typedef WWII::inplace_vector<ENDPOINT, Teul::MaxConnections> ENDPOINT_ARRAY;
typedef WWII::inplace_vector<EndpointImpl, Teul::MaxConnections> ENDPOINT_IMPL_ARRAY;
ENDPOINT_ARRAY _endpoints;
static ENDPOINT_IMPL_ARRAY _endpointImpls;

typedef std::vector<ENDPOINT*> ENDPOINT_VEC;
static ENDPOINT_VEC _deadEndpoints;

using namespace Teul;
namespace Teul
{
	ENDPOINT_LIST idleEndpoints;
}

static ENDPOINT_LIST _freeEndpoints;

static ERROR_FUNCTION* _endpointErrorFn;

extern void teulReleaseDatagram(struct DATA_GRAM*);

void
teulCloseAllEndpoints(const SINT32 errorCode)
{
	dbugLog(_FLM, "Orderly shutdown in progress [%d].", errorCode);

	Teul::ENDPOINT_LIST eps = idleEndpoints;

#if platBuild == TEUL_SERVER
	// Make a copy.
	if ( errorCode != 0 )
	{
		for ( auto it = eps.begin(), end = eps.end(); it != end; ++it )
		{
			(*it)->sendError(errorCode);
		}
	}
#endif
	for ( auto it = eps.begin(), end = eps.end(); it != end; ++it )
	{
		if ( *it != nullptr )
			teulClose(*(*it), Teul::Closing::OrderlyShutdown);
	}
}

//////////////////////////////////////////////////////////////////////
// Encrypt outgoing data.

void
ENDPOINT::encrypt(UCHAR* const stream, const size_t bytes)
{
	if ( m_impl->m_cryptVersion >= Teul::TeulV2 )
	{
#ifdef DEBUG_ENCRYPTION
		printf("Encrypt %p %d [%u] seed %llx\n", stream, bytes, m_impl->m_cryptVersion, m_impl->m_txCrypt.seed());
#endif
		m_impl->m_txCrypt.translate(stream, bytes);
	}
}

//////////////////////////////////////////////////////////////////////
// Decrypt incoming data.

void
ENDPOINT::decrypt(UCHAR* const stream, const size_t bytes)
{
	if ( m_impl->m_cryptVersion >= Teul::TeulV2 )
	{
#ifdef DEBUG_ENCRYPTION
		printf("Decrypt %p %d [%u] seed %llx\n", stream, bytes, m_impl->m_cryptVersion, m_impl->m_rxCrypt.seed());
#endif
		m_impl->m_rxCrypt.translate(stream, bytes);
	}
}

#define SZ_MAX_BUFF       1600                            // Largest size hex text area to display not including extra for trailer (header info)
#define NUM_TAIL_AMT      8                               // For oversized packets, number of hex bytes to show at tail end
#define SZ_HEX_DIG_TEXT   3                               // Length of each printed hex digit - which includes a space
#define NUM_MAX_HDR_BYTES (SZ_MAX_BUFF / SZ_HEX_DIG_TEXT) // Number of heading bytes that can be displayed in the buffer allotted

//////////////////////////////////////////////////////////////////////
// Format a packet for dbug display output (if configured)
void
ENDPOINT::_dbugOutputPacket( bool isRecv, const uint8_t *input, size_t packetBytes )
{
	static const char *STR_CONTINUATION = " ... ";

	// NOTE: Actual buffer is the maximux header area plus all the fixed trailer information.
	auto debugBuff = std::make_unique<char []>(SZ_MAX_BUFF + NUM_TAIL_AMT * SZ_HEX_DIG_TEXT + strlen(STR_CONTINUATION) + 1);
	debugBuff[0] = 0;
	char hex[] = { '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F' };
	char charToHex[SZ_HEX_DIG_TEXT+1] = {};
	charToHex[2] = ' '; 
	charToHex[SZ_HEX_DIG_TEXT] = 0;

	size_t i = 0;
	for ( ; i < packetBytes && i < NUM_MAX_HDR_BYTES; i++)
	{
		charToHex[0] = hex[input[i] >> 4];
		charToHex[1] = hex[input[i] & 0xf];
		strncat(debugBuff.get(), charToHex, sizeof(debugBuff));
	}

	if (i >= NUM_MAX_HDR_BYTES)
	{
		strncat(debugBuff.get(), STR_CONTINUATION, sizeof(debugBuff));
		for ( i = NUM_TAIL_AMT; i > 0; i--)
		{
			size_t backupAmt = packetBytes - i;
			charToHex[0] = hex[input[backupAmt] >> 4];
			charToHex[1] = hex[input[backupAmt] & 0xf];
			strncat(debugBuff.get(), charToHex, sizeof(debugBuff));
			}
		}

	if (!isRecv)
		dbugLog(_FLD, "Sending MSG (%d bytes): SourcePort %d DestPort %d IP %s Sent Dump:\n-->%s",
		        packetBytes, destPort(), fromPort(), teulGetAddress(fromAddress()), debugBuff.get());
	else
		dbugLog(_FLD, "Receiving MSG (%d bytes): SourcePort %d DestPort %d IP %s Recv Dump:\n<--%s",
		        packetBytes, destPort(), fromPort(), teulGetAddress(fromAddress()), debugBuff.get());
}

//////////////////////////////////////////////////////////////////////
// Nagle's algorithm is designed to delay small packets to allow for
// some aggregation of payloads and reduce header:payload overhead.
//
// We used to disable it for all connections, but it may not be
// required for all of our connections.

bool
ENDPOINT::setNagleEnabled(bool enabled_)
{
	int previousValue = 0;
	socklen_t argLen = sizeof(previousValue);
	getsockopt(this->socket, IPPROTO_TCP, TCP_NODELAY, (char*)&previousValue, &argLen);

	// Find the previous value of nagle.
	// To enable nagle, set TCP_NODELAY to FALSE.
	const int argValue = enabled_ ? 0 : 1;
	if ( previousValue != argValue )
	{
		setsockopt(this->socket, IPPROTO_TCP, TCP_NODELAY, (const char*)&argValue, sizeof(argValue));
	}

	return (previousValue == 0);
}

//////////////////////////////////////////////////////////////////////
// Returns a pointer to an endpoint given a index.

ENDPOINT*
teulGetEndpoint(const SINT32 teulIndex)
{
	if ( PN_UNLIKELY((size_t)teulIndex >= _endpoints.size()) )	// Coercing to unsigned makes negative indexes very large.
	{
		//LogAbort("t:%d endpoint index out of range (0 <= endpoint <= %" PRSizeT ")", teulIndex, _endpoints.size());
		return nullptr;
	}

	ENDPOINT& endpoint = _endpoints[teulIndex];
	if ( PN_UNLIKELY(endpoint.m_inUse == false) )
	{
		dbugLog(_FLW, "t:%d endpoint not in use", teulIndex);
		return nullptr;
	}

	return &endpoint;
}

//////////////////////////////////////////////////////////////////////
// Returns a pointer to an endpoint, but only if it's connected/open.

ENDPOINT*
Teul::GetConnectedEndpoint(const SINT32 teulIndex)
{
	ENDPOINT* const endpoint = teulGetEndpoint(teulIndex);
	if ( PN_UNLIKELY(endpoint == nullptr) )
		return nullptr;
	if ( PN_UNLIKELY((endpoint->state() != IS_OPEN) | (endpoint->m_closed) | (endpoint->m_connected == false)) )
		return nullptr;
	return endpoint;
}

//////////////////////////////////////////////////////////////////////

UINT32
teulGetEndpointLocalAddress(const SINT32 teulIndex)
{
	const ENDPOINT* const endpoint = teulGetEndpoint(teulIndex);
	return PN_LIKELY(endpoint != nullptr) ? endpoint->localAddress() : Teul::INVALID_INDEX;
}

//////////////////////////////////////////////////////////////////////

UINT32
teulGetEndpointRemoteAddress(const SINT32 teulIndex)
{
	const ENDPOINT* const endpoint = teulGetEndpoint(teulIndex);
	return PN_LIKELY(endpoint != nullptr) ? endpoint->destAddress() : Teul::INVALID_INDEX;
}

//////////////////////////////////////////////////////////////////////
// Returns the local IP address and port associated with an index.

std::pair<UINT32, UINT16>
Teul::GetLocal(const SINT32 teulIndex)
{
	const ENDPOINT* const endpoint = teulGetEndpoint(teulIndex);
	return PN_LIKELY(endpoint != nullptr) ? std::make_pair(endpoint->localAddress(), endpoint->localPort()) : std::make_pair(Teul::INVALID_INDEX, (UINT16) - 1);
}

//////////////////////////////////////////////////////////////////////
// Returns the remote IP address and port associated with an index.

std::pair<UINT32, UINT16>
Teul::GetRemote(const SINT32 teulIndex)
{
	const ENDPOINT* const endpoint = teulGetEndpoint(teulIndex);
	return PN_LIKELY(endpoint != nullptr) ? std::make_pair(endpoint->destAddress(), endpoint->destPort()) : std::make_pair(Teul::INVALID_INDEX, (UINT16) - 1);
}

//////////////////////////////////////////////////////////////////////
// TeulTransport needs to find out what the next teulIndex will be.

SINT32
teulQueryNextFreeIndex()
{
	teulKillDeadEndpoints();

	if ( PN_UNLIKELY(_freeEndpoints.empty()) )
	{
		return -ENOSPC;
	}

	return _freeEndpoints.front()->teulIndex();
}

//////////////////////////////////////////////////////////////////////
// Allocate a new endpoint object.

static ENDPOINT*
_allocEndpoint(SINT32 wantIndex)
{
	if ( PN_UNLIKELY(_freeEndpoints.empty()) )
	{
		teulKillDeadEndpoints();
		ABORT_IF(_freeEndpoints.empty());
	}

	if ( wantIndex < 0 )
	{
		ABORT_IF_EX(wantIndex != Teul::AnyIndex, "illegal new endpoint index: %d", wantIndex);

		wantIndex = _freeEndpoints.front()->teulIndex();
		_freeEndpoints.pop_front();
	}
	else
	{
		assert( (size_t)wantIndex < _endpoints.size() );

		// In truth, this should usually be the first endpoint available.
		ENDPOINT_LIST::iterator it = std::find(_freeEndpoints.begin(), _freeEndpoints.end(), &_endpoints[wantIndex]);
		ABORT_IF(it == _freeEndpoints.end());
		_freeEndpoints.erase(it);
	}

	ENDPOINT& endpoint = _endpoints[wantIndex];

	_teulConnects++;

	endpoint.m_closed = false;

#ifdef DEV_BUILD
	dbugLog(_FLD, "t:%d allocated endpoint %p", endpoint.teulIndex(), &endpoint);
#endif

	return &endpoint;
}

//////////////////////////////////////////////////////////////////////
// Free up an endpoint (usually only on shutdown).

static void
_releaseEndpoint(ENDPOINT& endpoint)
{
	endpoint.m_closed = false;
	endpoint.m_impl->Reset();

	// Remove me from the idle list.
	idleEndpoints.remove(&endpoint);

	// Push me onto the free list.
	_freeEndpoints.push_back(&endpoint);

#ifdef DEV_BUILD
	dbugLog(_FLD, "t:%d pushed to freeList (%" PRSizeT " endpoints)", endpoint.teulIndex(), _freeEndpoints.size());
	dbugWrite(_FLN, "t:%d released", endpoint.teulIndex());
#endif

	endpoint.m_inUse = false;
}

//////////////////////////////////////////////////////////////////////
// Move an endpoint to the dead list.

void
teulRemoveEndpoint(ENDPOINT* inEndpoint)
{
	if ( ! inEndpoint->m_inUse )
	{
		dbugLog(_FLD, "t:%d tried to remove endpoint not in use", inEndpoint->teulIndex());
		return;
	}

	if ( inEndpoint->m_closed == false )
	{
		dbugLog(_FLD, "t:%d tried to remove unclosed endpoint", inEndpoint->teulIndex());
		inEndpoint->doError(ECONNABORTED);
		inEndpoint->shutdown();
	}

	_deadEndpoints.push_back(inEndpoint);
}

//////////////////////////////////////////////////////////////////////
// Transfer dead endpoints to the free endpoint list.

void
teulKillDeadEndpoints(void)
{
	if ( _deadEndpoints.empty() )
	{
		return;
	}

	for ( ENDPOINT_VEC::iterator it = _deadEndpoints.begin(), end = _deadEndpoints.end(); it != end; ++it )
	{
		teulFreeEndpoint(*it);
	}

	_deadEndpoints.clear();
}

//////////////////////////////////////////////////////////////////////
// Release the current datagram.

void
ENDPOINT::releaseDatagram()
{
	if ( m_impl->m_curDatagram != nullptr )
	{
		teulReleaseDatagram(m_impl->m_curDatagram);
		m_impl->m_curDatagram = nullptr;
	}
	if ( m_readBufferUsed != 0 )
		m_readBufferUsed = 0;
}

//////////////////////////////////////////////////////////////////////
// Free an endpoint.

void
teulFreeEndpoint(ENDPOINT* inEndpoint)
{
	if (inEndpoint == nullptr || (!inEndpoint->m_inUse))
		return;

#ifdef DEV_BUILD
	dbugLog(_FLD, "t:%d freeing endpoint %p", inEndpoint->teulIndex(), inEndpoint);
#endif

	inEndpoint->releaseDatagram();

	inEndpoint->m_impl->Reset();

//	dbugLog(_FLD, "pushing endpoint %p to freelist", inEndpoint);
	_releaseEndpoint(*inEndpoint);
}

void
ENDPOINT::initialize(SINT32 i)
{
	m_impl = &_endpointImpls[i];
	m_teulIndex = (SINT16)i;
	m_inUse = false;
	m_state = IS_DEAD;

	_freeEndpoints.push_back(this);
}

//////////////////////////////////////////////////////////////////////
// Initialize the endpoint system.

void
teulInitEndpoints(ERROR_FUNCTION* const endpointErrorFn)
{
	_endpointErrorFn = endpointErrorFn;
	_teulConnects = 0;

	_endpoints.resize(Teul::MaxConnections);
	_endpointImpls.resize(Teul::MaxConnections);

	ABORT_IF(_endpoints.empty() == true);	// Just a sanity check really.

	const size_t numConnections = _endpoints.size();
	for ( size_t i = 0; i < numConnections; i++)
	{
		_endpoints[i].initialize(i);
	}

	dbugLog(_FLI, "Teul::MaxConnections %" PRSizeT " freeEndpoints %" PRSizeT " sizeof(ENDPOINT):%" PRSizeT " sizeof(ENDPOINTImpl):%" PRSizeT, Teul::MaxConnections, _endpoints.size(), sizeof(ENDPOINT), sizeof(EndpointImpl));

	ABORT_IF_EX(_freeEndpoints.empty() == true, "endpointFreeList should not be empty after initialization");
}

void
teulCleanupEndpoints(void)
{
	teulKillDeadEndpoints();

	if ( _endpoints.empty() == false )
	{
		const size_t numEndpoints = _endpoints.size();
		for ( size_t i = 0; i < numEndpoints; i++)
		{
			ENDPOINT& endpoint = _endpoints[i];

			endpoint.m_impl->Reset();

			endpoint.releaseDatagram();
		}
	}

	_freeEndpoints.clear();
	_deadEndpoints.clear();
	idleEndpoints.clear();
	_endpoints.clear();
	_endpointImpls.clear();
}

UINT32
ENDPOINT::setTimeout(const UINT32 timeout_)
{
	timeout = timeout_;

	if ( timeout == 0 || state() == IS_LISTENING || protocol == PROTOCOL_UDB || protocol == PROTOCOL_CTRL )
	{
		m_impl->kliveInterval = Teul::MaxKliveNeeded;
		m_doTimeoutCheck = false;
	}
	else
	{
		SINT32 interval = timeout / Teul::KlivesPerTimeout;
		if ( interval < Teul::MinKliveNeeded && Teul::MinKliveNeeded < timeout )
			interval = Teul::MinKliveNeeded;
		if ( interval > Teul::MaxKliveNeeded )
			interval  = Teul::MaxKliveNeeded;
		m_impl->kliveInterval = (UINT16)interval;
		m_doTimeoutCheck = true;
#if defined(DEBUG_CONNECTIONS)
		dbugLog(_FLD, "t:%d timeout:%u, iv:%u, dtc:true", teulIndex(), timeout_, interval);
		printf("t:%d timeout:%u, iv:%u, dtc:true\n", teulIndex(), timeout_, interval);
#endif
	}

	lastKliveSent.makeCurrent();

	return m_impl->kliveInterval;
}

ENDPOINT*
teulCreateEndpoint(SINT32 wantIndex
					, const bool isListening
					, SINT32 timeout
					, const PROTOCOL protocol
					)
{
	ENDPOINT* const newEndpoint = _allocEndpoint(wantIndex);
	if (PN_UNLIKELY(newEndpoint == nullptr))
	{
		dbugLog(_FLA, "_createEndpoint: could not allocate buffer for endpoint");
		return nullptr;
	}

	newEndpoint->socket = INVALID_SOCKET;
	newEndpoint->m_connected = false;
	newEndpoint->lastDataRecv.makeCurrent();
	newEndpoint->lastDataSent.makeCurrent();

	newEndpoint->m_impl->m_curDatagram = nullptr;
	newEndpoint->m_readBufferUsed = 0;
	newEndpoint->lastKliveSent.makeNever();

	// populated in accept if we are the spawn of a listening connection.
	newEndpoint->m_parent = nullptr;

	if (timeout != 0 && timeout < Teul::MinEndpointTimeout)
		timeout = Teul::MinEndpointTimeout;

	newEndpoint->m_impl->Reset();

	newEndpoint->protocol = (UCHAR)protocol;
	newEndpoint->m_peer = (protocol == PROTOCOL_UDB) | (protocol == PROTOCOL_CTRL);

	newEndpoint->m_state = (SCHAR)(isListening ? IS_LISTENING : IS_OPENING);

	newEndpoint->setTimeout(timeout);

	newEndpoint->m_created.makeCurrent();
	newEndpoint->m_haveTicks = false;
	newEndpoint->m_tickAccumulation = 0ULL;

	// allocates the packet buffers for write and reliable.
	// initializes timeout and RTO to 1000ms and therefore defaults
	// each packet to the same.  Specifying a different timeout for
	// a callback will over-ride this for a packet.

	switch (protocol)
	{
		case PROTOCOL_TCP:
		case PROTOCOL_UDP:
		case PROTOCOL_UDB:
		case PROTOCOL_CTRL:
			newEndpoint->fromSockAddress.sin_family = AF_INET;
			newEndpoint->localSockAddress.sin_family = AF_INET;
			newEndpoint->destSockAddress.sin_family = AF_INET;
			break;

		default:
			dbugSetErrno(EINVAL);
			dbugLog(_FLA, "error allocating endpoint, invalid protocol");
			teulFreeEndpoint(newEndpoint);
			return nullptr;
			break;
	}

	newEndpoint->m_impl->m_version = Teul::VersionNumber;
	newEndpoint->m_impl->m_cryptVersion = (newEndpoint->protocol == PROTOCOL_TCP && !newEndpoint->m_peer) ? Teul::BaseVersion : 0x0000;
	newEndpoint->m_impl->m_txCrypt.newSeed();
	newEndpoint->m_impl->m_rxCrypt.setSeed(0ULL);
//	newEndpoint->m_recvCrypt = newEndpoint->m_sendCrypt;

	switch (protocol)
	{
	case PROTOCOL_UDP:
	case PROTOCOL_UDB:
	case PROTOCOL_CTRL:
		newEndpoint->m_reliable = false;
		break;
	case PROTOCOL_TCP:
	default:
		newEndpoint->m_reliable = true;
		break;
	}

	idleEndpoints.push_back(newEndpoint);
	newEndpoint->m_inUse = true;

	return newEndpoint;
}

//////////////////////////////////////////////////////////////////////
// Convert an error code into an error name.

const char*
Teul::GetErrorName(SINT32 error)
{
	if ( error < 0 )
		error = -error;

#define ErrorCase(__Error__) case __Error__: return #__Error__; break
#define TeulErrorCase(__Error__) case Teul :: Errors :: __Error__ : return #__Error__; break
	switch ( (UINT32)error )
	{
	ErrorCase( EAGAIN );
	ErrorCase( EINTR );
	ErrorCase( ETEUL_UNDEF_CALLBACK );
	ErrorCase( ETEUL_ENOSPC_SEND_BUFFER );
	ErrorCase( ETEUL_ENOSPC_SEND_SOCKET );
	ErrorCase( ENETDOWN );
	ErrorCase( EPFNOSUPPORT );
	ErrorCase( EAFNOSUPPORT );
	ErrorCase( ENETUNREACH );
	ErrorCase( EADDRINUSE );
	ErrorCase( EADDRNOTAVAIL );
	ErrorCase( ENETRESET );
	ErrorCase( ECONNABORTED );
	ErrorCase( ECONNRESET );
	ErrorCase( ENOBUFS );
	ErrorCase( ENOTCONN );
	ErrorCase( ETIMEDOUT );
	ErrorCase( ECONNREFUSED );
#if EWOULDBLOCK != EAGAIN
	ErrorCase( EWOULDBLOCK );
#endif
#if platOS == OS_LINUX
	ErrorCase( ESHUTDOWN );
	ErrorCase( EHOSTDOWN );
#endif
	ErrorCase( EHOSTUNREACH );
	ErrorCase( ETEUL_CONNECTION_STALLED );
	ErrorCase( ETEUL_CB_CONN_DEAD );	// Tried to send a callback to a dead connection.
	ErrorCase( ETEUL_CB_SEND_NULL );	// Tried to send a null callback.
	ErrorCase( ETEUL_SERVER_SHUTDOWN );
	ErrorCase( ETEUL_CB_REQ_DATA );	// Bad receive.
	case ETEUL_ILLEGAL_PACKET_ORDER:
		return "ecorrupt";
	break;

	TeulErrorCase( LoginFail_ActivateNoLocalInfo );
	TeulErrorCase( LoginFail_ActivateFailed );
	TeulErrorCase( LoginFail_DespawnNoPlayer );
	TeulErrorCase( LoginFail_DespawnError );
	TeulErrorCase( LoginFail_ActivateNoPlayerInfo );
	TeulErrorCase( LoginFail_AcceptBadIndex );
	TeulErrorCase( LoginFail_AcceptBadTeul );
	TeulErrorCase( LoginFail_AddPlayer );
	TeulErrorCase( LoginFail_SessionValidation );
	TeulErrorCase( LoginFail_NoSession );
	TeulErrorCase( LoginFail_Client );
	TeulErrorCase( LoginFail_SessionOwn );
	TeulErrorCase( LoginFail_Validation );
	TeulErrorCase( LoginFail_Collision );

	default: break;
	}
#undef ErrorCase

	if ( error >= Teul::Errors::CategoryMultiplier )
	{
		switch ( error / Teul::Errors::CategoryMultiplier )
		{
		case Teul::Errors::PackCallbackCategory:
			return "pkcb";
		break;
		case Teul::Errors::SendFailCategory:
			return "txf";
		break;
		case Teul::Errors::PushPacketCategory:
			return "pshp";
		break;
		default:
		break;
		}
	}

	return "INTERNALERRROR";
}

//////////////////////////////////////////////////////////////////////
// Increase the size of a socket's receive buffer.

int
ENDPOINT::setRcvBufferSize(UINT32 newSize)
{
	static const UINT32 chunkSize = 4096;		// 4Kb - must be power of 2.
	static const UINT32 chunkMask = chunkSize - 1;

	const UINT32 goalSize = (std::min(newSize, (UINT32)ENDPOINT::MaxRcvBufferSize) + chunkSize) & (~chunkMask);

#if defined(DEV_BUILD)
	dbugLog(_FLD, "t:%d sock:" PRF_SOCKET " setting rcv to %dbytes", teulIndex(), PRA_SOCKET(socket), goalSize);
#endif

	return setsockopt(socket, SOL_SOCKET, SO_RCVBUF, (const char*)&goalSize, sizeof(goalSize));
}

//////////////////////////////////////////////////////////////////////
// Increase the size of a socket's send buffer.

int
ENDPOINT::setSndBufferSize(UINT32 newSize)
{
	static const UINT32 chunkSize = 4096;		// 4Kb - must be power of 2.
	static const UINT32 chunkMask = chunkSize - 1;

	const UINT32 goalSize = (std::min(newSize, (UINT32)ENDPOINT::MaxSndBufferSize) + chunkSize) & (~chunkMask);

#if defined(DEV_BUILD)
	dbugLog(_FLD, "t:%d sock:" PRF_SOCKET " setting sndbuf to %dbytes", teulIndex(), PRA_SOCKET(socket), goalSize);
#endif

	return setsockopt(socket, SOL_SOCKET, SO_SNDBUF, (const char*)&goalSize, sizeof(goalSize));
}

void
ENDPOINT::doError(const SINT32 error, void* data)
{
	ERROR_FUNCTION* errorFunction = this->errorFn;
	if ( this->m_parent != nullptr && ((state() == IS_OPENING) | (errorFunction == nullptr)) )
		errorFunction = this->m_parent->errorFn;

	if ( errorFunction == nullptr )
		errorFunction = _endpointErrorFn;

	if ( errorFunction != nullptr )
		(errorFunction)(this->teulIndex(), (UCHAR*)data, 0, nullptr, error);
}

void
ENDPOINT::shutdown()
{
	if ( this->socket != INVALID_SOCKET )
	{
		::shutdown(this->socket, 2);		// Shutdown both sides of the connection.
		OS_CLOSESOCKET(this->socket);

#ifdef DEV_BUILD
		dbugLog(_FLD, "t:%d sock:" PRF_SOCKET " closed socket", this->teulIndex(), PRA_SOCKET(this->socket));
#endif

		this->socket = INVALID_SOCKET;

		teulMRecvRemoveFromPoll(*this);
	}

	this->m_closed = true;
}

bool
Teul::GetEndpointStatus(const SINT32 teulIndex, SINT32* const outReason, TeulDeltaMS* const outLastRx_ms)
{
	if ( PN_UNLIKELY(teulIndex < 0) )
	{
		*outReason = -EINVAL;
		return false;
	}

	ENDPOINT* const endpoint = teulGetEndpoint(teulIndex);
	if ( PN_UNLIKELY(endpoint == nullptr) )
	{
		*outReason = -ENOENT;
		return false;
	}

	if ( PN_UNLIKELY(endpoint->m_closed == true) )
	{
		*outReason = -ECONNABORTED;
		return false;
	}

	if ( PN_UNLIKELY(endpoint->m_connected == false) )
	{
		*outReason = -ENOTCONN;
		return false;
	}

	*outReason = 0;
	*outLastRx_ms = endpoint->lastDataRecv.elapsed();

	return true;
}

//////////////////////////////////////////////////////////////////////
// Clear out values/data in an EndpointImpl struct.

void
EndpointImpl::Reset()
{
	m_txCrypt.setSeed(0ULL);
	m_rxCrypt.setSeed(0ULL);
	recvTag = 0;
	sentTag = 0;
	assert( m_curDatagram == nullptr );
	for (auto it = recvQueue.begin(), end = recvQueue.end(); it != end; ++it)
		teulReleaseDatagram(*it);
	recvQueue.clear();
	while ( m_sendQueue.empty() == false )
	{
		FreeFirstPacketInSendQueue();
	}
	m_version = 0;
	m_cryptVersion = 0;
	pollSlot(-1);
	kliveInterval = 0;
	m_lastPacketTick = 0;
}

//////////////////////////////////////////////////////////////////////
// Return the local address of a given endpoint.

UINT32 ENDPOINT::localAddress() const { return ntohl(localSockAddress.sin_addr.s_addr); }
UINT16 ENDPOINT::localPort() const { return ntohs(localSockAddress.sin_port); }

UINT32 ENDPOINT::fromAddress() const { return ntohl(fromSockAddress.sin_addr.s_addr); }
UINT16 ENDPOINT::fromPort() const { return ntohs(fromSockAddress.sin_port); }

UINT32 ENDPOINT::destAddress() const { return ntohl(destSockAddress.sin_addr.s_addr); }
UINT16 ENDPOINT::destPort() const { return ntohs(destSockAddress.sin_port); }
