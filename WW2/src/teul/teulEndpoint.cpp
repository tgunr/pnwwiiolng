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

#include <string.h>
#include "include/inplace_vector.h"
#include <queue>
#include "teul/teulBasic.h"
#include "teul/teulModule.h"
#include "teul/teulPacket.h"
#include "teul/teulEndpoint.h"
#include "teul/teulEndpointIdleList.h"
#include "teul/teulEndpointImpl.h"
#include "teul/teulTime.h"
#include "teul/teulUtils.h"
#include "teul/teulVersion.h"
#include "teul/raknetConn.h"

//#define DEBUG_ENCRYPTION
//#define DEBUG_CONNECTIONS

static size_t _teulConnects;

using ENDPOINT_ARRAY = WWII::inplace_vector<ENDPOINT, Teul::MaxConnections>;
using ENDPOINT_IMPL_ARRAY = WWII::inplace_vector<EndpointImpl, Teul::MaxConnections>;
static ENDPOINT_ARRAY _endpoints;
static ENDPOINT_IMPL_ARRAY _endpointImpls;

GUID_ENDPOINT_MAP guidToEndPoint;

using namespace Teul;
namespace Teul
{
	ENDPOINT_LIST idleEndpoints;
}

static ENDPOINT_LIST _freeEndpoints;

static ERROR_FUNCTION _endpointErrorFn;

extern void teulReleaseDatagram(struct DATA_GRAM*);

void
teulCloseAllEndpoints(const SINT32 errorCode)
{
	dbugLog(_FLM, "Orderly shutdown in progress [%d].", errorCode);

	Teul::ENDPOINT_LIST eps = idleEndpoints;

#if platBuild == BUILD_SERVER
	if (errorCode != 0) {
		for (auto ep : eps) {
			ep->sendError(errorCode);
		}
	}
#endif
    
    eps = idleEndpoints;
    
	for (auto ep : eps) {
		if (ep != nullptr) {
			teulClose(*ep);
        }
	}
}

//////////////////////////////////////////////////////////////////////
// Encrypt outgoing data.

void ENDPOINT::encrypt(UCHAR* const stream, const size_t bytes)
{
	if (m_impl->m_cryptVersion >= Teul::TeulV2) {
		m_impl->m_txCrypt.translate(stream, bytes);
	}
}

//////////////////////////////////////////////////////////////////////
// Decrypt incoming data.

void ENDPOINT::decrypt(UCHAR* const stream, const size_t bytes)
{
	if (m_impl->m_cryptVersion >= Teul::TeulV2) {
		m_impl->m_rxCrypt.translate(stream, bytes);
	}
}

//////////////////////////////////////////////////////////////////////
// Format a packet for dbug display output (if configured)
void ENDPOINT::_dbugOutputPacket(bool isRecv, const uint8_t *input, size_t packetBytes)
{
    static constexpr const size_t SZ_MAX_BUFF = 1600;
    static constexpr const size_t NUM_TAIL_AMT = 8;
    static constexpr const size_t SZ_HEX_DIG_TEXT = 3;
    static constexpr const size_t NUM_MAX_HDR_BYTES = SZ_MAX_BUFF / SZ_HEX_DIG_TEXT;
	static constexpr const char * const STR_CONTINUATION = " ... ";
    static constexpr const char hex[] = { '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F' };
    static constexpr const size_t BUFFER_SZ = SZ_MAX_BUFF + NUM_TAIL_AMT * SZ_HEX_DIG_TEXT + sizeof(STR_CONTINUATION);

	// NOTE: Actual buffer is the maximux header area plus all the fixed trailer information.
	auto debugBuff = std::make_unique<char []>(BUFFER_SZ);
	char charToHex[SZ_HEX_DIG_TEXT+1] = { 0, 0, ' ', 0 };
    
	for (size_t i = 0; i < packetBytes && i < NUM_MAX_HDR_BYTES; i++) {
		charToHex[0] = hex[input[i] >> 4];
		charToHex[1] = hex[input[i] & 0xf];
        strlcat(debugBuff.get(), charToHex, BUFFER_SZ);
	}

	if (packetBytes >= NUM_MAX_HDR_BYTES) {
		strlcat(debugBuff.get(), STR_CONTINUATION, BUFFER_SZ);
		for (size_t i = NUM_TAIL_AMT; i > 0; i--) {
			charToHex[0] = hex[input[packetBytes - i] >> 4];
			charToHex[1] = hex[input[packetBytes - i] & 0xf];
			strlcat(debugBuff.get(), charToHex, BUFFER_SZ);
        }
    }

    dbugLog(_FLD, "%s MSG (%" PRSizeT " bytes): SourcePort %d DestPort %d IP %s Sent Dump:\n-->%s",
            isRecv ? "Receiving" : "Sending", packetBytes, destPort(), fromPort(), teulGetAddress(fromAddress()), debugBuff.get());
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
// Determines if the caller would be able to allocate an endpoint.

bool
teulHasFreeEndpoints()
{
	if ( PN_UNLIKELY(_freeEndpoints.empty()) ) {
		teulKillDeadEndpoints();
	}
	return _freeEndpoints.size() > 0;

}
//////////////////////////////////////////////////////////////////////
// Allocate a new endpoint object: ABORTS if no endpoints are
// available. Check teulHasFreeEndpoints() before calling.

static ENDPOINT&
_allocEndpoint()
{
	auto teulIndex = _freeEndpoints.front()->teulIndex();
	_freeEndpoints.pop_front();

	ENDPOINT& endpoint = _endpoints[teulIndex];

	_teulConnects++;

	endpoint.m_closed = false;

#ifdef DEV_BUILD
	dbugLog(_FLD, "t:%d allocated endpoint %p", endpoint.teulIndex(), &endpoint);
#endif

	return endpoint;
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

	// remove from the ip/endpoint list
	GUID_ENDPOINT_MAP::iterator it = guidToEndPoint.begin();
	while (it != guidToEndPoint.end())
	{
		if (it->second == &endpoint)
		{
			guidToEndPoint.erase(it);
			break;
		}
		else
			it++;
	}

	// Push me onto the free list.
	_freeEndpoints.push_back(&endpoint);

#ifdef DEV_BUILD
	dbugLog(_FLD, "t:%d pushed to freeList (%" PRSizeT " endpoints)", endpoint.teulIndex(), _freeEndpoints.size());
	dbugWrite(_FLN, "t:%d released", endpoint.teulIndex());
#endif

	endpoint.m_inUse = false;
}

//////////////////////////////////////////////////////////////////////
// Manage dead endpoints. This allows us to provide controlled access
// with a narrow contract that gives a strong-guarantee of non-null
// pointers in the list.

class DeadEndpointManager
{
	std::vector<ENDPOINT*> m_endpoints;
public:
	void reserve(size_t n) noexcept { m_endpoints.reserve(n); }

	void add(ENDPOINT* endpoint) noexcept
	{
		if (endpoint != nullptr) {
			dbugLog(_FLD, "t:%d (%p) adding to dead endpoints", endpoint->m_teulIndex, endpoint);
#ifdef DEBUG
			ABORT_IF_EX( std::find(m_endpoints.begin(), m_endpoints.end(), endpoint) != m_endpoints.end(),
							"endpoint added to dead endpoints manager twice." );
#endif
			m_endpoints.push_back(endpoint);
		}
	}

	void process()
	{
		// a. Freeing the endpoint could eventually lead to code that tries
		// to modify the dead endpoint list, which would break our walk of the list.
		// b. The list is in a nicely pre-allocated vector,
		// c. The list *could* be large enough to make copying expensive.
		// So:
		// We *steal* the list (move) to a local, exchanging it for the empty temporary
		// we stole it to, making the manager object itself safe from modification.
		// We crawl the stolen data, free those endpoints, and then put the stolen
		// vector back so that we retain the original pre-sized allocation.
		if ( ! m_endpoints.empty() ) {
			dbugLog(_FLM | STD_ERR, "processing %" PRSizeT " dead endpoints", m_endpoints.size());
            auto eps = std::exchange(m_endpoints, {});
			for (auto* ep : eps) {
				teulFreeEndpoint(ep);
			}
			eps.clear();
			// we don't expect this to ever do anything, but it's there so that if the
			// free process somehow *adds* an endpoint to the list, we keep it.
			eps.insert(eps.end(), m_endpoints.begin(), m_endpoints.end());
			// put the original vector back where we got it from.
			m_endpoints = std::move(eps);
		}
	}

	void discard() noexcept
	{
		m_endpoints.clear();
	}

	size_t size() const noexcept { return m_endpoints.size(); }
};
DeadEndpointManager s_deadEndpointManager;

//////////////////////////////////////////////////////////////////////
// Move an endpoint to the dead list.

void
teulRemoveEndpoint(ENDPOINT* endpoint)
{
	if ( endpoint == nullptr )
		return;

	dbugLog(_FLM, "teulRemoveEndpoint(%p) t:%d iu:%d cl:%d st:%d", endpoint, endpoint->teulIndex(), (int)endpoint->m_inUse, (int)endpoint->m_closed, endpoint->m_state);

	if ( ! endpoint->m_inUse )
	{
		dbugLog(_FLD, "t:%d tried to remove endpoint not in use", endpoint->teulIndex());
		return;
	}

	if ( endpoint->m_closed == false )
	{
		dbugLog(_FLA, "t:%d tried to remove unclosed endpoint", endpoint->teulIndex());
		endpoint->doError(ECONNABORTED);
		endpoint->shutdown();
	}

	s_deadEndpointManager.add(endpoint);
}

//////////////////////////////////////////////////////////////////////
// Transfer dead endpoints to the free endpoint list.

void
teulKillDeadEndpoints()
{
	s_deadEndpointManager.process();
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
teulInitEndpoints(ERROR_FUNCTION const endpointErrorFn)
{
	_endpointErrorFn = endpointErrorFn;
	_teulConnects = 0;

	_endpoints.resize(Teul::MaxConnections);
	_endpointImpls.resize(Teul::MaxConnections);
	s_deadEndpointManager.reserve(Teul::MaxConnections);

	ABORT_IF(_endpoints.empty() == true);	// Just a sanity check really.
	ABORT_IF(_endpoints.size() != Teul::MaxConnections);

	const size_t numConnections = _endpoints.size();
	for ( size_t i = 0; i < numConnections; i++)
	{
		_endpoints[i].initialize(i);
	}

	dbugLog(_FLI, "Teul::MaxConnections %" PRSizeT " freeEndpoints %" PRSizeT " sizeof(ENDPOINT):%" PRSizeT " sizeof(ENDPOINTImpl):%" PRSizeT, Teul::MaxConnections, _endpoints.size(), sizeof(ENDPOINT), sizeof(EndpointImpl));

	ABORT_IF_EX(_freeEndpoints.empty() == true, "endpointFreeList should not be empty after initialization");
}

void 
teulEndpointBackchannels()
{
    for (ENDPOINT& endpoint : _endpoints) {
        if (endpoint.m_pairdUdpConn &&
            ((endpoint.m_pairdUdpConn->isConnected() || endpoint.m_pairdUdpConn->isConnecting()) ||
            (endpoint.m_pairdUdpConn->isServer() && endpoint.m_pairdUdpConn->isInitialized())))
        {
            endpoint.m_pairdUdpConn->update();
        }
    }
}

void
teulCleanupEndpoints()
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
	s_deadEndpointManager.discard();
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
		m_impl->kliveInterval = Teul::MaxKliveInterval;
		m_doTimeoutCheck = false;
	}
	else
	{
		TeulTimeMS interval = timeout / Teul::KlivesPerTimeout;
		if ( interval < Teul::MinKliveInterval && Teul::MinKliveInterval < timeout )
			interval = Teul::MinKliveInterval;
		if ( interval > Teul::MaxKliveInterval )
			interval  = Teul::MaxKliveInterval;
		m_impl->kliveInterval = interval;
		m_doTimeoutCheck = true;
#if defined(DEBUG_CONNECTIONS)
		dbugLog(_FLD, "t:%d timeout:%u, iv:%" PR64BitUT ", dtc:true", teulIndex(), timeout_, interval);
#endif
	}

	lastKliveSent.makeCurrent();

	return m_impl->kliveInterval;
}

ENDPOINT*
teulCreateEndpoint(const bool isListening, SINT32 timeout, const PROTOCOL protocol)
{
    ABORT_IF(protocol != PROTOCOL_TCP && protocol != PROTOCOL_UDP && protocol != PROTOCOL_UDB && protocol != PROTOCOL_CTRL);
    
	teulSyncClock();  // for granular reporting, previously all our callers did this

	if (PN_UNLIKELY(teulHasFreeEndpoints() == false)) {
		dbugLog(_FLA, "no free endpoints available");
		return nullptr;
	}
	ENDPOINT& endpoint = _allocEndpoint();

	endpoint.socket = INVALID_SOCKET;
	endpoint.m_connected = false;
	endpoint.m_rakNetEnabled = false;
	endpoint.lastDataRecv.makeCurrent();
	endpoint.lastDataSent.makeCurrent();

	endpoint.m_pairdUdpConn = nullptr;
	endpoint.m_impl->m_curDatagram = nullptr;
	endpoint.m_readBufferUsed = 0;
	endpoint.lastKliveSent.makeNever();

	// populated in accept if we are the spawn of a listening connection.
	endpoint.m_parent = nullptr;

	if (timeout != 0 && timeout < Teul::MinEndpointTimeout)
		timeout = Teul::MinEndpointTimeout;

	endpoint.m_impl->Reset();

	endpoint.protocol = (UCHAR)protocol;
	endpoint.m_peer = (protocol == PROTOCOL_UDB) || (protocol == PROTOCOL_CTRL);

	endpoint.m_state = (SCHAR)(isListening ? IS_LISTENING : IS_OPENING);

	endpoint.setTimeout(timeout);

	endpoint.m_created.makeCurrent();
	endpoint.m_haveTicks = false;
	endpoint.m_tickAccumulation = 0ULL;

	// allocates the packet buffers for write and reliable.
	// initializes timeout and RTO to 1000ms and therefore defaults
	// each packet to the same.  Specifying a different timeout for
	// a callback will over-ride this for a packet.

    endpoint.fromSockAddress.sin_family = AF_INET;
    endpoint.localSockAddress.sin_family = AF_INET;
    endpoint.destSockAddress.sin_family = AF_INET;

	endpoint.m_impl->m_version = Teul::VersionNumber;
	endpoint.m_impl->m_cryptVersion = (endpoint.protocol == PROTOCOL_TCP && !endpoint.m_peer) ? Teul::BaseVersion : 0x0000;
	endpoint.m_impl->m_txCrypt.newSeed();
	endpoint.m_impl->m_rxCrypt.setSeed(0ULL);

    endpoint.m_reliable = (protocol == PROTOCOL_TCP);

	idleEndpoints.push_back(&endpoint);
	endpoint.m_inUse = true;

	return &endpoint;
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
#ifdef EPFNOSUPPORT
	ErrorCase( EPFNOSUPPORT );
#endif
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
#if defined( WSAEWOULDBLOCK )
	ErrorCase( WSAEWOULDBLOCK );
#endif
#if defined( WSAEINPROGRESS )
	ErrorCase( WSAEINPROGRESS );
#endif
#if defined( WSAEINTR )
	ErrorCase( WSAEINTR );
#endif
#ifdef ESHUTDOWN
	ErrorCase( ESHUTDOWN );
#endif
#ifdef EHOSTDOWN
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

		case Teul::Errors::SendFailCategory:
			return "txf";

		case Teul::Errors::PushPacketCategory:
			return "pshp";

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
	ERROR_FUNCTION errorFunction = this->errorFn;
    
	if (this->m_parent && (state() == IS_OPENING || !errorFunction)) {
		errorFunction = this->m_parent->errorFn;
    }
    if (!errorFunction) {
		errorFunction = _endpointErrorFn;
    }

	if (errorFunction) {
		errorFunction(this->teulIndex(), (UCHAR*)data, 0, nullptr, error);
    }
}

void ENDPOINT::shutdown()
{
	if (this->socket != INVALID_SOCKET) {
        teulMRecvRemoveFromPoll(*this);
		OS_CLOSESOCKET(this->socket);
		dbugLog(_FLD, "t:%d sock:" PRF_SOCKET " closed socket", this->teulIndex(), PRA_SOCKET(this->socket));
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
    for (auto * const dgram : m_recvQueue) {
		teulReleaseDatagram(dgram);
    }
	m_recvQueue.clear();
	while (!m_sendQueue.empty()) {
		freeFirstPacketInSendQueue();
	}
	m_version = 0;
	m_cryptVersion = 0;
	setPollSlot(std::numeric_limits<nfds_t>::max());
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
