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
	KDATA() : randDat((UINT8)(random() & 255)) {}
	UINT8	randDat;
};

TEUL_CALLBACK* SYN_TOKEN;
TEUL_CALLBACK* SYN_ACK_TOKEN;
TEUL_CALLBACK* SYN_ACK_ACK_TOKEN;
TEUL_CALLBACK* KLIVE_TOKEN;
TEUL_CALLBACK* KLIVE_ACK_TOKEN;
TEUL_CALLBACK* Teul::ConnectionInstruction::s_callback;

//////////////////////////////////////////////////////////////////////
static SINT32 _packSYN_TOKEN( PackMode mode, UCHAR* const data, SINT32 offset, struct SYN_TOKEN_STRUCT* const tokenRef, SINT32 /*size*/)
{
    // Introduce some noise to scramble seeds.
    UINT32 randVal = (mode == TEUL_PACK) ? random() : 0;

	offset = teulPackUINT16(mode, data, offset, &tokenRef->version, 0);
	offset = teulPackUINT32(mode, data, offset, &tokenRef->timeout, 24);
	offset = teulPackUINT32(mode, data, offset, &randVal, 0);
	offset = teulPackUINT64(mode, data, offset, &tokenRef->seed, 0);
	return offset;
}

//static inline void _report(const ACTION_OBJECT* const a, const char* const txt)
//{
//#ifdef DEBUG_CONNECTIONS
//    dbugLog(_FLD, "t:%d[%d] %s", a->teulIndex, (a->endpoint ? a->endpoint->teulIndex() : -999), txt);
//#endif
//}

static inline void _status(const char* const doing, const char* const what, UINT32 version, const ENDPOINT* const endpoint)
{
#ifdef DEBUG_CONNECTIONS
	dbugLog(_FLD, "t:%d [%2s] [%3s] v=%04x ep->ver=%04x ep->crypt=%04x txs=%" PR64BitUT " rxs=%" PR64BitUT "",
            endpoint->teulIndex(), doing, what, version, endpoint->m_impl->m_version, endpoint->m_impl->m_cryptVersion,
            endpoint->m_impl->m_txCrypt.seed(), endpoint->m_impl->m_rxCrypt.seed());
#endif
}

//////////////////////////////////////////////////////////////////////
// Connectee (client) sends a SYN token.

static SINT32
_actionSendSYN_TOKEN(ACTION_OBJECT* const a)
{
	//_report(a, "Tx SYN");

    assert(a->endpoint);

	struct SYN_TOKEN_STRUCT token;

	token.version = Teul::VersionNumber;
	token.timeout = a->endpoint->timeout;
	token.seed = a->endpoint->m_impl->m_txCrypt.seed();
	a->endpoint->lastKliveSent.makeCurrent();

	_status("tx", "SYN", token.version, a->endpoint);
	return _packSYN_TOKEN(TEUL_PACK, a->packed, a->offset, &token, 0);
}

static UINT16 _negotiateVersionNumber(UINT16 remoteVersion, const ENDPOINT* const endpoint)
{
	// Datagram delivery services don't use encryption.
	if (!endpoint->isFromClient()) {
		dbugLog(_FLM, "t:%d non-client connection, disabling encryption", endpoint->teulIndex());
		return 0;
	}

	// We can't support versions higher than ourselves.
	if (remoteVersion >= Teul::VersionNumber) {
		return Teul::VersionNumber;
    }

	// The minimum version we can support.
	if (remoteVersion <= Teul::BaseVersion) {
		return Teul::BaseVersion;
    }
    
	return remoteVersion;
}

//////////////////////////////////////////////////////////////////////
// Host (accepter) receives a SYN token.
static SINT32 _actionRecvSYN_TOKEN(ACTION_OBJECT* const a)
{
	//_report(a, "Rx SYN");

    assert(a->endpoint);

	struct SYN_TOKEN_STRUCT token;

	a->offset = _packSYN_TOKEN(TEUL_UNPACK, a->packed, a->offset, &token, 0);// get when the probe was sent

	a->endpoint->m_impl->m_version = _negotiateVersionNumber(token.version, a->endpoint);
	a->endpoint->m_impl->m_rxCrypt.setSeed(token.seed);

	_status("rx", "SYN", token.version, a->endpoint);

	// Temporarily make dest the same as from then restore it.
	sockaddr_in savedDestAddress = std::exchange(a->endpoint->destSockAddress, a->endpoint->fromSockAddress);

	a->offset = teulModSendCALLBACKe(*a->endpoint, *SYN_ACK_TOKEN);	// send it
	a->endpoint->destSockAddress = savedDestAddress;

	return a->offset;
}

//////////////////////////////////////////////////////////////////////

static SINT32 _packSYN_ACK_TOKEN(PackMode mode, UCHAR* const data, SINT32 offset, struct SYN_TOKEN_STRUCT* tokenRef, SINT32 /*size*/)
{
    // Introduce some more noise.
    UINT32 randVal = random();

	offset = teulPackUINT16(mode, data, offset, &tokenRef->version, 0);
	offset = teulPackUINT32(mode, data, offset, &tokenRef->timeout, 24);
	offset = teulPackUINT32(mode, data, offset, &randVal, 0);
	offset = teulPackUINT64(mode, data, offset, &tokenRef->seed, 0);
	return offset;
}

//////////////////////////////////////////////////////////////////////
// The host (accepter) sends a SYN_ACK token.
static SINT32 _actionSendSYN_ACK_TOKEN(ACTION_OBJECT* const a)
{
	//_report(a, "Tx SAK");

    assert(a->endpoint);

	struct SYN_TOKEN_STRUCT token;

	token.version = a->endpoint->m_impl->m_version;
	token.timeout = a->endpoint->timeout;
	token.seed = a->endpoint->m_impl->m_txCrypt.seed();
    a->endpoint->lastKliveSent.makeCurrent();

	_status("tx", "SAK", token.version, a->endpoint);
	a->offset = _packSYN_ACK_TOKEN(TEUL_PACK, a->packed, a->offset, &token, 0);	// when probe was sent

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// The client (connectee) receives a SYN_ACK token.
static SINT32 _actionRecvSYN_ACK_TOKEN(ACTION_OBJECT* const a)
{
	//_report(a, "Rx SAK");

    assert(a->endpoint);

	if (a->endpoint->state() == IS_LISTENING) {
		LogAbort("someone sent a SYN_ACK to the listen port!! This is a critical programmer error.");
	}

	if (a->endpoint->state() == IS_OPEN) {
        // This logic previously re-sent the SAA token instead of erroring. That's wrong.
        dbugLog(_FLA, "t:%d duplicate SYN-ACK token received, dropping connection", a->teulIndex);
        teulClose(*a->endpoint);
		return -EALREADY;
	}

	struct SYN_TOKEN_STRUCT token;

	a->offset = _packSYN_ACK_TOKEN(TEUL_UNPACK, a->packed, a->offset, &token, 0);
    
    UINT16 version = _negotiateVersionNumber(token.version, a->endpoint);

    a->endpoint->setTimeout(token.timeout); // use the timeout given.
    a->endpoint->m_impl->m_version = version;
    a->endpoint->m_impl->m_rxCrypt.setSeed(token.seed);
	dbugLog(_FLD, "t:%d at:%d negotiated teul version:%04x timeout: %d", a->teulIndex, a->endpoint->teulIndex(), a->endpoint->m_impl->m_version, token.timeout);

	_status("rx", "SAK", token.version, a->endpoint);

	a->endpoint->destSockAddress = a->endpoint->fromSockAddress;
    
    SINT32 connErr = teulConnectENDPOINT(*a->endpoint);
 
	if (connErr < 0) {
        dbugLog(_FLA, "t:%d failed to connect endpoint %d", a->endpoint->teulIndex(), connErr);
        teulClose(*a->endpoint);
        return connErr;
    }
    
    SINT32 sendErr = teulModSendCALLBACKe(*a->endpoint, *SYN_ACK_ACK_TOKEN);
    
    if (sendErr < 0) {
        dbugLog(_FLA, "t:%d cant send SYN_ACK_ACK callback %d", a->endpoint->teulIndex(), sendErr);
        teulClose(*a->endpoint);
        return sendErr;
    }

    a->endpoint->m_impl->m_cryptVersion = version;
    a->endpoint->doConnect();
	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// The host (accepter) receives an ACK_ACK token.
static SINT32 _actionRecvSYN_ACK_ACK_TOKEN(ACTION_OBJECT* const a)
{
	//_report(a, "Rx SAA");

	assert(a->endpoint);

	struct SYN_TOKEN_STRUCT token;
    
	a->offset = _packSYN_ACK_TOKEN(TEUL_UNPACK, a->packed, a->offset, &token, 0);	// when probe was sent

	if (a->endpoint->state() == IS_OPEN) {
		dbugLog(_FLA, "t:%d duplicate SYN-ACK-ACK token received, dropping connection", a->endpoint->teulIndex());
        teulClose(*a->endpoint);
		return -EALREADY;
	}

	UINT16 version = _negotiateVersionNumber(token.version, a->endpoint);
    
	a->endpoint->m_impl->m_version = version;
	a->endpoint->m_impl->m_cryptVersion = version;
    a->endpoint->destSockAddress = a->endpoint->fromSockAddress;

	_status("rx", "SAA", token.version, a->endpoint);
	teulConnectENDPOINT( *a->endpoint );

	dbugWrite(_FLN, "RecvSYN_ACK_ACK: parent: %d index:%d", a->endpoint->m_parent->teulIndex(), a->endpoint->teulIndex());
	a->endpoint->m_parent->doAccept(*a->endpoint);

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
static SINT32 _packKLIVE_TOKEN(PackMode mode, UCHAR* const data, SINT32 offset, KDATA* kt, SINT32 /*size*/)
{
	return teulPackUCHAR(mode, data, offset, &kt->randDat, 8);
}

//////////////////////////////////////////////////////////////////////
static SINT32 _packKLIVE_ACK_TOKEN(PackMode mode,  UCHAR* const data, SINT32 offset, KDATA* kt, SINT32 /*size*/)
{
	return _packKLIVE_TOKEN(mode, data, offset, kt, 0);
}

//////////////////////////////////////////////////////////////////////
static SINT32 _actionSendKLIVE_TOKEN(ACTION_OBJECT* const a)
{
    assert(a->endpoint);

	a->endpoint->lastKliveSent.makeCurrent();

	struct KDATA kt;

	return _packKLIVE_TOKEN(TEUL_PACK, a->packed, a->offset, &kt, 0);
}

//////////////////////////////////////////////////////////////////////
static SINT32 _actionRecvKLIVE_TOKEN(ACTION_OBJECT* const a)
{
	assert(a->endpoint);

	struct KDATA kt;

	a->offset = _packKLIVE_TOKEN(TEUL_UNPACK, a->packed, a->offset, &kt, 0);
    
    // TODO: Sure, why not, but let's try actually doing it right at some point
    KLIVE_ACK_TOKEN->outData = &kt; // point to the struct
	teulModSendCALLBACKe(*a->endpoint, *KLIVE_ACK_TOKEN); // send it
	KLIVE_ACK_TOKEN->outData = nullptr;

	return a->offset;
}

//////////////////////////////////////////////////////////////////////

static SINT32 _actionSendKLIVE_ACK_TOKEN(ACTION_OBJECT* const a)
{
	struct KDATA kt;

	a->endpoint->lastKliveSent.makeCurrent();
	return _packKLIVE_ACK_TOKEN(TEUL_PACK, a->packed, a->offset, &kt, 0);
}

//////////////////////////////////////////////////////////////////////

static SINT32 _actionRecvKLIVE_ACK_TOKEN(ACTION_OBJECT* const a)
{
    assert(a->endpoint);

	struct KDATA kt;

	return _packKLIVE_ACK_TOKEN(TEUL_UNPACK, a->packed, a->offset, &kt, 0);
}

//////////////////////////////////////////////////////////////////////
static SINT32 _recvConnectionInstruction(ACTION_OBJECT* const a)
{
    Teul::ConnectionInstruction connIns(a);
    
    if (connIns.receive(a->teulIndex) < 0) {
        return -EINVAL;
    }
    return a->offset;
}

//////////////////////////////////////////////////////////////////////
static SINT32 _sendConnectionInstruction(ACTION_OBJECT* const a)
{
    Teul::ConnectionInstruction *connIns = reinterpret_cast<Teul::ConnectionInstruction *>(a->localRawData);
    
    return connIns->packer(TEUL_PACK, a);
}

//////////////////////////////////////////////////////////////////////
// Receive an error on a teulIndex.

static SINT32 _recvError(ACTION_OBJECT* const a)
{
    assert(a->endpoint);
    
	SINT32 err = 0;

    a->offset = teulPackSINT32(TEUL_UNPACK, a->packed, a->offset, &err, 32);
    a->endpoint->doError(err);
  
    return a->offset;
}

//////////////////////////////////////////////////////////////////////
SINT32 teulInitModCallbacks()
{
    Teul::CallbackInit ci;

    ci.callX = Teul::Module::CallbackX;
    ci.callXName = TEUL_PACKET_NAMES ? "TMCALLBACKS_X" : "";

    // SYN
    ci.callY = TM_SYN_TOKEN_Y;
    ci.callYName = TEUL_PACKET_NAMES ? "TM_SYN_TOKEN_Y" : "";
    ci.sendAction = _actionSendSYN_TOKEN;
    ci.recvAction = _actionRecvSYN_TOKEN;
    SYN_TOKEN = teulAllocSystemCallback(&ci);

    // SYN-ACK
    ci.callY = TM_SYN_ACK_TOKEN_Y;
    ci.callYName = TEUL_PACKET_NAMES ? "TM_SYN_ACK_TOKEN_Y" : "";
    ci.sendAction = _actionSendSYN_ACK_TOKEN;
    ci.recvAction = _actionRecvSYN_ACK_TOKEN;
    SYN_ACK_TOKEN = teulAllocSystemCallback(&ci);
    
    // SYN-ACK-ACK
    ci.callY = TM_SYN_ACK_ACK_TOKEN_Y;
    ci.callYName = TEUL_PACKET_NAMES ? "TM_SYN_ACK_ACK_TOKEN_Y" : "";
    ci.sendAction = _actionSendSYN_ACK_TOKEN;
    ci.recvAction = _actionRecvSYN_ACK_ACK_TOKEN;
    SYN_ACK_ACK_TOKEN = teulAllocSystemCallback(&ci);

    // KLIVE
    ci.callY = TM_KLIVE_TOKEN_Y;
    ci.callYName = TEUL_PACKET_NAMES ? "TM_KLIVE_TOKEN_Y" : "";
    ci.sendAction = _actionSendKLIVE_TOKEN;
    ci.recvAction = _actionRecvKLIVE_TOKEN;
    KLIVE_TOKEN = teulAllocSystemCallback(&ci);

    // KLIVE-ACK
    ci.callY = TM_KLIVE_ACK_TOKEN_Y;
    ci.callYName = TEUL_PACKET_NAMES ? "TM_KLIVE_ACK_TOKEN_Y" : "";
    ci.sendAction = _actionSendKLIVE_ACK_TOKEN;
    ci.recvAction = _actionRecvKLIVE_ACK_TOKEN;
    KLIVE_ACK_TOKEN = teulAllocSystemCallback(&ci);

    // CONNECTION-INSTRUCTION
    ci.callY = TM_CONNECTION_INSTRUCTION_Y;
    ci.callYName = TEUL_PACKET_NAMES ? "TM_CONNECTION_INSTRUCTION_Y" : "";
    ci.sendAction = _sendConnectionInstruction;
    ci.recvAction = _recvConnectionInstruction;
    Teul::ConnectionInstruction::s_callback = teulAllocSystemCallback(&ci);

    // ERR
    ci.callY = TM_ERROR_Y;
    ci.callYName = TEUL_PACKET_NAMES ? "TM_ERROR_Y" : "";
    ci.recvAction = _recvError;
    teulAllocSystemCallback(&ci);

    return 0;
}

//////////////////////////////////////////////////////////////////////
// Have an endpoint send an error to the remote side.
void ENDPOINT::sendError(const SINT32 error_)
{
	if (teulIndex() < 0 || !m_inUse || m_closed) {
		return;
    }

	Teul::Packet(Teul::PacketId(Teul::Module::CallbackX, TM_ERROR_Y), error_).send(teulIndex());
}

//////////////////////////////////////////////////////////////////////
// Send error to an endpoint.
void teulSendError(const SINT32 teulIndex, const SINT32 error)
{
	if (ENDPOINT* const endpoint = teulGetEndpoint(teulIndex)) {
		endpoint->sendError(error);
    }
}
