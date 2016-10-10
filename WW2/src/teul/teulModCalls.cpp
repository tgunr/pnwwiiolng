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

//#define DEBUG_CONNECTIONS

#include "teul/teulCallback.h"
#include "teul/teulCallbackInit.h"
#include "teul/teulCallbackStruct.h"
#include "teul/teulConnectionInstruction.h"
#include "teul/teulConstants.h"
#include "teul/teulEndpoint.h"
#include "teul/teulEndpointImpl.h"
#include "teul/teulModule.h"
#include "teul/teulPacket.h"
#include "teul/teulVersion.h"
#include "wwii/wwiiNetPorts.h"

// Data sent with a "SYN" message.
struct SYN_TOKEN_STRUCT
{
	UINT32	timeout;
	UINT16	version;
	UINT64	seed;
};

struct KDATA
{
	KDATA() : random((UINT8)(rand() & 255)) {}
	UINT8	random;
};

TEUL_CALLBACK*		RST_TOKEN;

TEUL_CALLBACK*		SYN_TOKEN;

TEUL_CALLBACK*		SYN_ACK_TOKEN;
TEUL_CALLBACK*		SYN_ACK_ACK_TOKEN;
TEUL_CALLBACK*		FYN_TOKEN;
TEUL_CALLBACK*		FYN_ACK_TOKEN;
TEUL_CALLBACK*		KLIVE_TOKEN;
TEUL_CALLBACK*		KLIVE_ACK_TOKEN;
TEUL_CALLBACK*		Teul::ConnectionInstruction::s_callback;

//////////////////////////////////////////////////////////////////////
///TODO: Make this generally available as the null error handler.

static void
_errorCALLBACK(SINT32 /*teulIndex*/, const UCHAR* const /*data*/, SINT32 /*size*/, const TEUL_CALLBACK* const /*object*/, SINT32 /*error*/)
{
}

//////////////////////////////////////////////////////////////////////
///TODO: Make this generally available as the nullptr callback.

// base TM callbacks
static SINT32
_actionSendRST_TOKEN(ACTION_OBJECT* /*a*/)
{
	return 0;
}

//////////////////////////////////////////////////////////////////////

static SINT32
_actionRecvRST_TOKEN(ACTION_OBJECT* const a)
{
	teulClose(*a->endpoint, Teul::Closing::RstToken);
	return a->offset;
}

//////////////////////////////////////////////////////////////////////

static void
_errorSYN_CALLBACK(SINT32 teulIndex, const UCHAR* const /*data*/, SINT32 /*size*/, const TEUL_CALLBACK* const /*object*/, SINT32 error)
{
	ENDPOINT* const inEndpoint = teulGetEndpoint(teulIndex);
	if ( inEndpoint == nullptr)
	{
		dbugLog(_FLA, "t:%d illegal index", teulIndex);
		return;
	}

	dbugLog(_FLA, "t:%d sock:" PRF_SOCKET " errorSYN: closing error %d clock:%llu", teulIndex, PRA_SOCKET(inEndpoint->socket), error, teulClock64);
	teulClose(*inEndpoint, Teul::Closing::SynErr);
}

//////////////////////////////////////////////////////////////////////

static SINT32
_packSYN_TOKEN( LOGICAL mode, UCHAR* const data, SINT32 offset, struct SYN_TOKEN_STRUCT* const tokenRef, SINT32 /*size*/)
{
	offset = teulPackUINT16(mode, data, offset, &tokenRef->version, 0);
	offset = teulPackUINT32(mode, data, offset, &tokenRef->timeout, 24);
	// Introduce some noise to scramble seeds.
	UINT32 random = (mode == TEUL_PACK) ? rand() : 0;
	offset = teulPackUINT32(mode, data, offset, &random, 0);
	offset = teulPackUINT64(mode, data, offset, &tokenRef->seed, 0);
	return offset;
}

static inline void _report(const ACTION_OBJECT* const a, const char* const txt)
{
#ifdef DEBUG_CONNECTIONS
	dbugLog(_FLD, "t:%d[%d] %s", a->teulIndex, (a->endpoint ? a->endpoint->teulIndex() : -999), txt);
#else
	silence_unused_parameter_warning(a, txt);
#endif
}

static inline void _status(const char* const doing, const char* const what, UINT32 version, const ENDPOINT* const endpoint)
{
#ifdef DEBUG_CONNECTIONS
	dbugLog(_FLD, "t:%d [%2s] [%3s] v=%04x ep->ver=%04x ep->crypt=%04x txs=%llx rxs=%16llx",
            endpoint->teulIndex(), doing, what, version, endpoint->m_impl->m_version, endpoint->m_impl->m_cryptVersion,
            endpoint->m_impl->m_txCrypt.seed(), endpoint->m_impl->m_rxCrypt.seed());
#else
	silence_unused_parameter_warning(doing, what, version, endpoint);
#endif
}

//////////////////////////////////////////////////////////////////////
// Connectee (client) sends a SYN token.

static SINT32
_actionSendSYN_TOKEN(ACTION_OBJECT* const a)
{
	_report(a, "Tx SYN");

	if ( PN_UNLIKELY(a->endpoint == nullptr) )
		return -EINVAL;

	struct SYN_TOKEN_STRUCT token;

	token.version = Teul::VersionNumber;
	token.timeout = a->endpoint->timeout;
	token.seed = a->endpoint->m_impl->m_txCrypt.seed();

	a->endpoint->lastKliveSent.makeCurrent();

	_status("tx", "SYN", token.version, a->endpoint);

	return _packSYN_TOKEN(TEUL_PACK, a->packed, a->offset, &token, 0);
}

static UINT16
_negotiateVersionNumber(UINT16 remoteVersion, const ENDPOINT* const endpoint)
{
	// Datagram delivery services don't use encryption.
	if ( !endpoint->isFromClient() )
	{
		dbugLog(_FLM, "t:%d non-client connection, disabling encryption", endpoint->teulIndex());
		return 0;
	}

	// We can't support versions higher than ourselves.
	if ( remoteVersion >= Teul::VersionNumber )
		return Teul::VersionNumber;

	// The minimum version we* can* support.
	if ( remoteVersion <= Teul::BaseVersion )
		return Teul::BaseVersion;

	return remoteVersion;
}

//////////////////////////////////////////////////////////////////////
// Host (accepter) receives a SYN token.

static SINT32
_actionRecvSYN_TOKEN(ACTION_OBJECT* const a)
{
	_report(a, "Rx SYN");

	if ( PN_UNLIKELY((a->endpoint == nullptr) | (SYN_ACK_TOKEN == nullptr)) )
		return -EINVAL;

	struct SYN_TOKEN_STRUCT token;
	a->offset = _packSYN_TOKEN(TEUL_UNPACK, a->packed, a->offset, &token, 0);// get when the probe was sent

	a->endpoint->m_impl->m_version = _negotiateVersionNumber(token.version, a->endpoint);
	a->endpoint->m_impl->m_rxCrypt.setSeed(token.seed);
	token.seed = a->endpoint->m_impl->m_txCrypt.seed();

	_status("rx", "SYN", token.version, a->endpoint);

	// Temporarily make dest the same as from then restore it.
	const sockaddr_in savedDestAddress = a->endpoint->destSockAddress;
	a->endpoint->destSockAddress = a->endpoint->fromSockAddress;

	a->offset = teulModSendCALLBACKe(*a->endpoint, *SYN_ACK_TOKEN);	// send it

	a->endpoint->destSockAddress = savedDestAddress;

	return a->offset;
}

//////////////////////////////////////////////////////////////////////

static SINT32
_packSYN_ACK_TOKEN(LOGICAL mode, UCHAR* const data, SINT32 offset, struct SYN_TOKEN_STRUCT* tokenRef, SINT32 /*size*/)
{
	offset = teulPackUINT16(mode, data, offset, &tokenRef->version, 0);
	offset = teulPackUINT32(mode, data, offset, &tokenRef->timeout, 24);
	// Introduce some more noise.
	UINT32 random = rand();
	offset = teulPackUINT32(mode, data, offset, &random, 0);
	offset = teulPackUINT64(mode, data, offset, &tokenRef->seed, 0);
	return offset;
}

//////////////////////////////////////////////////////////////////////
// The host (accepter) sends a SYN_ACK token.

static SINT32
_actionSendSYN_ACK_TOKEN(ACTION_OBJECT* const a)
{
	_report(a, "Tx SAK");

	if ( PN_UNLIKELY(a->endpoint == nullptr) )
		return -EINVAL;

	struct SYN_TOKEN_STRUCT token;

	token.version		= a->endpoint->m_impl->m_version;
	a->endpoint->lastKliveSent.makeCurrent();
	token.timeout		= a->endpoint->timeout;
	token.seed			= a->endpoint->m_impl->m_txCrypt.seed();

	_status("tx", "SAK", token.version, a->endpoint);

	a->offset = _packSYN_ACK_TOKEN(TEUL_PACK, a->packed, a->offset, &token, 0);	// when probe was sent

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// The client (connectee) receives a SYN_ACK token.

static SINT32
_actionRecvSYN_ACK_TOKEN(ACTION_OBJECT* const a)
{
	_report(a, "Rx SAK");

	if ( PN_UNLIKELY((a->endpoint == nullptr) | (SYN_ACK_ACK_TOKEN == nullptr)) )
		return -EINVAL;

	if (PN_UNLIKELY(a->endpoint->state() == IS_LISTENING))
	{
		dbugLog(_FLA, "someone sending a SYN_ACK to the listen port!!");
		return -EINVAL;
	}

	if (a->endpoint->state() == IS_OPEN)						// is it a duplicate?
	{
		// already open, must be duplicate, re-send SYN_ACK_ACK_TOKEN
		const SINT32 retval = teulModSendCALLBACKe(*a->endpoint, *SYN_ACK_ACK_TOKEN);
		if ( retval < 0 )
		{
			dbugLog(_FLA, "cant send SYN_ACK_ACK callback");
			return retval;
		}
		///ISSUE: Shouldn't we unpack the token to consume it?
		return a->offset;
	}

	struct SYN_TOKEN_STRUCT token;
	a->offset = _packSYN_ACK_TOKEN(TEUL_UNPACK, a->packed, a->offset, &token, 0);

	dbugLog(_FLD, "t:%d at:%d negotiated teul version:%04x negotiated timeout: %d", a->teulIndex, a->endpoint->teulIndex(), token.version, token.timeout);

	a->endpoint->setTimeout(token.timeout);				// use the timeout and version given.

	const UINT16 version = _negotiateVersionNumber(token.version, a->endpoint);
	a->endpoint->m_impl->m_version = version;
	a->endpoint->m_impl->m_rxCrypt.setSeed(token.seed);
	token.seed = a->endpoint->m_impl->m_txCrypt.seed();

	_status("rx", "SAK", token.version, a->endpoint);

	a->endpoint->destSockAddress = a->endpoint->fromSockAddress;

	if( teulConnectENDPOINT(*a->endpoint) == 0 )
	{
		const SINT32 retval = teulModSendCALLBACKe(*a->endpoint, *SYN_ACK_ACK_TOKEN);
		if ( retval < 0)
		{
			dbugLog(_FLA, "t:%d cant send SYN_ACK_ACK callback", a->endpoint->teulIndex());
			teulClose(*a->endpoint, Teul::Closing::SynAckAckFail);
			///ISSUE Should this really be -retval?
			return -retval;
		}

		a->endpoint->m_impl->m_cryptVersion = version;

		a->endpoint->doConnect();
	}
	else
	{
		dbugLog(_FLA, "t:%d failed to connect endpoint", a->endpoint->teulIndex());
		teulClose(*a->endpoint, Teul::Closing::ConnectFail);
	}

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// The host (accepter) receives an ACK_ACK token.

static SINT32
_actionRecvSYN_ACK_ACK_TOKEN(ACTION_OBJECT* const a)
{
	_report(a, "Rx SAA");

	if ( PN_UNLIKELY(a->endpoint == nullptr) )
		return -EINVAL;

	struct SYN_TOKEN_STRUCT token;
	a->offset = _packSYN_ACK_TOKEN(TEUL_UNPACK, a->packed, a->offset, &token, 0);	// when probe was sent

	if (a->endpoint->state() == IS_OPEN)						// is it a duplicate?
	{
		dbugLog(_FLN, "RecvSYN_ACK_ACK %d is already open, this may be normal", a->endpoint->teulIndex());
		return a->offset;
	}

	const UINT16 version = _negotiateVersionNumber(token.version, a->endpoint);
	a->endpoint->m_impl->m_version = version;
	a->endpoint->m_impl->m_cryptVersion = version;

	_status("rx", "SAA", token.version, a->endpoint);

	a->endpoint->destSockAddress = a->endpoint->fromSockAddress;

	teulConnectENDPOINT( *a->endpoint );

	dbugWrite(_FLN, "RecvSYN_ACK_ACK: parent: %d index:%d", a->endpoint->m_parent->teulIndex(), a->endpoint->teulIndex());
	a->endpoint->m_parent->doAccept(*a->endpoint);

	return a->offset;
}

//////////////////////////////////////////////////////////////////////

static SINT32
_actionSendFYN_TOKEN(ACTION_OBJECT* const a)
{
	_report(a, "Tx FYN");

	return 0;
}

//////////////////////////////////////////////////////////////////////

static SINT32
_actionRecvFYN_TOKEN(ACTION_OBJECT* const a)
{
	_report(a, "Rx FYN");

	ENDPOINT* const inEndpoint = teulGetEndpoint(a->teulIndex);
	if ( (inEndpoint == nullptr) | (FYN_ACK_TOKEN == nullptr) )
		return -EINVAL;

	if ( (inEndpoint->state() != IS_CLOSING) & (inEndpoint->state() != IS_CLOSED) )
	{
		dbugLog(_FLD, "t:%d rxFyn: closing", a->teulIndex);
		if ( FYN_TOKEN != nullptr )
			teulModSendCALLBACKe(*inEndpoint, *FYN_TOKEN);
		teulClose(*inEndpoint, Teul::Closing::FynRx);
	}

	return teulModSendCALLBACKe(*inEndpoint, *FYN_ACK_TOKEN);
}

//////////////////////////////////////////////////////////////////////

static SINT32
_actionSendFYN_ACK_TOKEN(ACTION_OBJECT* const a)
{
	_report(a, "Tx FAK");

	return 0;
}

//////////////////////////////////////////////////////////////////////

static SINT32
_actionRecvFYN_ACK_TOKEN(ACTION_OBJECT* const a)
{
	_report(a, "Rx FAK");

	teulClose(*a->endpoint, Teul::Closing::FynAck);
	return 0;
}

//////////////////////////////////////////////////////////////////////

static SINT32
_packKLIVE_TOKEN (LOGICAL mode, UCHAR* const data, SINT32 offset, KDATA* kt, SINT32 /*size*/)
{
	return teulPackUCHAR(mode, data, offset, &kt->random, 8);
}

//////////////////////////////////////////////////////////////////////

static SINT32
_packKLIVE_ACK_TOKEN(LOGICAL mode,  UCHAR* const data, SINT32 offset, KDATA* kt, SINT32 /*size*/)
{
	return _packKLIVE_TOKEN(mode, data, offset, kt, 0);
}

//////////////////////////////////////////////////////////////////////

static SINT32
_actionSendKLIVE_TOKEN(ACTION_OBJECT* const a)
{
	if ( PN_UNLIKELY(a->endpoint == nullptr) )
		return -EINVAL;

	a->endpoint->lastKliveSent.makeCurrent();

	struct KDATA kt;
	return _packKLIVE_TOKEN(TEUL_PACK, a->packed, a->offset, &kt, 0);
}

//////////////////////////////////////////////////////////////////////

static SINT32
_actionRecvKLIVE_TOKEN(ACTION_OBJECT* const a)
{
	if ( PN_UNLIKELY((a->endpoint == nullptr) | (KLIVE_ACK_TOKEN == nullptr)) )
		return -EINVAL;

	struct KDATA kt;

	a->offset = _packKLIVE_TOKEN(TEUL_UNPACK, a->packed, a->offset, &kt, 0);

	const bool oldNagleValue = a->endpoint->setNagleEnabled(false);
	KLIVE_ACK_TOKEN->outData = &kt;				// point to the struct
	teulModSendCALLBACKe(*a->endpoint, *KLIVE_ACK_TOKEN);	// send it
	KLIVE_ACK_TOKEN->outData = nullptr;
	if ( oldNagleValue == true )
		a->endpoint->setNagleEnabled(oldNagleValue);

	return a->offset;
}

//////////////////////////////////////////////////////////////////////

static SINT32
_actionSendKLIVE_ACK_TOKEN(ACTION_OBJECT* const a)
{
	struct KDATA kt;

	a->endpoint->lastKliveSent.makeCurrent();

	return _packKLIVE_ACK_TOKEN(TEUL_PACK, a->packed, a->offset, &kt, 0);
}

//////////////////////////////////////////////////////////////////////

static SINT32
_actionRecvKLIVE_ACK_TOKEN(ACTION_OBJECT* const a)
{
	if ( PN_UNLIKELY(a->endpoint == nullptr) )
		return -EINVAL;

	struct KDATA kt;

	a->offset = _packKLIVE_ACK_TOKEN(TEUL_UNPACK, a->packed, a->offset, &kt, 0);

	return a->offset;
}

//////////////////////////////////////////////////////////////////////

static SINT32
_recvConnectionInstruction(ACTION_OBJECT* const a)
{
	Teul::ConnectionInstruction connIns(a);
	if ( PN_UNLIKELY(connIns.receive(a->teulIndex) < 0) )
		return -EINVAL;
	return a->offset;
}

//////////////////////////////////////////////////////////////////////

static SINT32
_sendConnectionInstruction(ACTION_OBJECT* const a)
{
	Teul::ConnectionInstruction* const connIns = (Teul::ConnectionInstruction*)(a->localRawData);
	return connIns->packer(TEUL_PACK, a);
}

//////////////////////////////////////////////////////////////////////
// Receive an error on a teulIndex.

static SINT32
_recvError(ACTION_OBJECT* const a)
{
	const SINT32 err = *((SINT32*)(a->packed));
	if ( a->endpoint )
		a->endpoint->doError(err);
	return a->offset + Teul::BytesToBits(err);
}

//////////////////////////////////////////////////////////////////////

SINT32
teulInitModCallbacks()
{
	Teul::CallbackInit ci{};

	ci.callX 		= Teul::Module::CallbackX;		// the X position in the callback matrix
	ci.callXName	= TEUL_PACKET_NAMES ? "TMCALLBACKS_X" : "";
	ci.error		= _errorCALLBACK;

	ci.callY		= TM_RST_TOKEN_Y;				// the Y position in the callback matrix
	ci.callYName	= TEUL_PACKET_NAMES ? "TM_RST_TOKEN_Y" : "";
	ci.sendAction	= _actionSendRST_TOKEN;			// how to send data
	ci.recvAction	= _actionRecvRST_TOKEN;			// how to receive data

	RST_TOKEN	= teulAllocSystemCallback( &ci );

	// following callbacks use the errSYN_CALLBACK  error handler which closes the connetion
	ci.error		= _errorSYN_CALLBACK;

	ci.callY		= TM_SYN_TOKEN_Y;                // the Y position in the callback matrix
	ci.callYName	= TEUL_PACKET_NAMES ? "TM_SYN_TOKEN_Y" : "";
	ci.sendAction	= _actionSendSYN_TOKEN;            // how to send data
	ci.recvAction	= _actionRecvSYN_TOKEN;			// how to receive data

	SYN_TOKEN	= teulAllocSystemCallback( &ci );

	ci.callY		= TM_SYN_ACK_TOKEN_Y;			// the Y position in the callback matrix
	ci.callYName	= TEUL_PACKET_NAMES ? "TM_SYN_ACK_TOKEN_Y" : "";
	ci.sendAction	= _actionSendSYN_ACK_TOKEN;		// how to send data
	ci.recvAction	= _actionRecvSYN_ACK_TOKEN;		// how to receive data

	SYN_ACK_TOKEN	= teulAllocSystemCallback( &ci );


	ci.callY		= TM_SYN_ACK_ACK_TOKEN_Y;		// the Y position in the callback matrix
	ci.callYName	= TEUL_PACKET_NAMES ? "TM_SYN_ACK_ACK_TOKEN_Y" : "";
	ci.sendAction	= _actionSendSYN_ACK_TOKEN;		// how to send data
	ci.recvAction	= _actionRecvSYN_ACK_ACK_TOKEN;	// how to receive data

	SYN_ACK_ACK_TOKEN	= teulAllocSystemCallback( &ci );


	ci.callY		= TM_FYN_TOKEN_Y;				// the Y position in the callback matrix
	ci.callYName	= TEUL_PACKET_NAMES ? "TM_FYN_TOKEN_Y" : "";
	ci.sendAction	= _actionSendFYN_TOKEN;			// how to send data
	ci.recvAction	= _actionRecvFYN_TOKEN;			// how to receive data

	FYN_TOKEN	= teulAllocSystemCallback( &ci );


	ci.callY		= TM_FYN_ACK_TOKEN_Y;			// the Y position in the callback matrix
	ci.callYName	= TEUL_PACKET_NAMES ? "TM_FYN_ACK_TOKEN_Y" : "";
	ci.sendAction	= _actionSendFYN_ACK_TOKEN;		// how to send data
	ci.recvAction	= _actionRecvFYN_ACK_TOKEN;		// how to receive data

	FYN_ACK_TOKEN	= teulAllocSystemCallback( &ci );


	// following callbqcks use the generic _errorCallback that does nothing at this time
	ci.error		= _errorCALLBACK;

	ci.callY		= TM_KLIVE_TOKEN_Y;				// the Y position in the callback matrix
	ci.callYName	= TEUL_PACKET_NAMES ? "TM_KLIVE_TOKEN_Y" : "";
	ci.sendAction	= _actionSendKLIVE_TOKEN;		// how to send data
	ci.recvAction	= _actionRecvKLIVE_TOKEN;		// how to receive data

	KLIVE_TOKEN	= teulAllocSystemCallback( &ci );


	ci.callY		= TM_KLIVE_ACK_TOKEN_Y;			// the Y position in the callback matrix
	ci.callYName	= TEUL_PACKET_NAMES ? "TM_KLIVE_ACK_TOKEN_Y" : "";
	ci.sendAction	= _actionSendKLIVE_ACK_TOKEN;	// how to send data
	ci.recvAction	= _actionRecvKLIVE_ACK_TOKEN;	// how to receive data

	KLIVE_ACK_TOKEN	= teulAllocSystemCallback( &ci );

	// Callback indicating a connection needs to be sent/received.
	ci.callY		= TM_CONNECTION_INSTRUCTION_Y;
	ci.callYName	= TEUL_PACKET_NAMES ? "TM_CONNECTION_INSTRUCTION_Y" : "";
	ci.sendAction	= _sendConnectionInstruction;
	ci.recvAction	= _recvConnectionInstruction;

	Teul::ConnectionInstruction::s_callback = teulAllocSystemCallback( &ci );

	ci.callY = TM_ERROR_Y;
	ci.callYName = TEUL_PACKET_NAMES ? "TM_ERROR_Y" : "";
	ci.recvAction = _recvError;

	teulAllocSystemCallback( &ci );

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Have an endpoint send an error to the remote side.

void
ENDPOINT::sendError(const SINT32 error_)
{
	if ( (teulIndex() < 0) | (m_inUse == false) | (m_closed == true) )
		return;

	Teul::Packet outgoing(Teul::PacketId(Teul::Module::CallbackX, TM_ERROR_Y), error_);
	outgoing.send(teulIndex());
}

//////////////////////////////////////////////////////////////////////
// Send error to an endpoint.

void
teulSendError(const SINT32 teulIndex, const SINT32 error)
{
	ENDPOINT* const endpoint = teulGetEndpoint(teulIndex);
	if ( endpoint != nullptr )
		endpoint->sendError(error);
}
