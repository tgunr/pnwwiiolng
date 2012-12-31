#include <syscfg.h> // Path is project specific.
// Date: Sun Feb 16 2004
//
// Chat callbacks for communicating with dbse
//
// Copyright (c) 2004 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Oliver "kfsone" Smith

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "wwii/wwiiCallbacks.h"
#include "chat/chatdbClientMessages.h"
#include "dbse/dbseClient.h"
#include "dbse/dbseConstants.h"
#include "chat/chatTypes.h"
#include "chat/chatHandle.h"
#include "chat/chatHostMessages.h"
#include "chat/chatList.h"
#include "chat/chatPlayer.h"
#include "chat/chatdbSquadXchg.h"
#include "chat/chatSquadMemberUpdates.h"

// "XCHG" indicates a callback which has send/recv

static TEUL_CALLBACK*	_SEND_SQUAD_REQUEST ;
static TEUL_CALLBACK*	_XCHG_SQUAD_MEMBER_UPDATE ;

//////////////////////////////////////////////////////////////////////
// Private functions
//////////////////////////////////////////////////////////////////////

//// Error functions

//// Send functions

//// Recv functions

//////////////////////////////////////////////////////////////////////

static SINT32
_recvSquadMemberUpdate(ACTION_OBJECT* const a)
{
	SquadMemberUpdate result(a) ;
	if ( a->offset < 0 )
		return a->offset ;
	if ( result.receive(a->teulIndex) < 0 )
		return -EINVAL ;
	return a->offset ;
}

//////////////////////////////////////////////////////////////////////

static SINT32
_sendSquadRequest(ACTION_OBJECT* const a)
{
	a->offset = teulPackUINT32(TEUL_PACK, a->packed, a->offset, (UINT32*)(a->localRawData), 0) ;
	return a->offset ;
}

static SINT32
_sendSquadMemberUpdate(ACTION_OBJECT* const a)
{
	return ((SquadMemberUpdate*)(a->localRawData))->packer(TEUL_PACK, a) ;
}

//////////////////////////////////////////////////////////////////////
// Public functions
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////

SINT32
chatdbRequestSquad(UINT32 squadID)
{
	return dbseSend(_SEND_SQUAD_REQUEST, &squadID) ;
}

//////////////////////////////////////////////////////////////////////
// Send a squad member update to dbse

SINT32
chatdbSendSquadMemberUpdate(SquadMemberUpdate& smu)
{
	return dbseSend(_XCHG_SQUAD_MEMBER_UPDATE, &smu) ;
}

//////////////////////////////////////////////////////////////////////
// Install the callbacks

SINT32
chatdbInstallClientCallbacks()
{
	BEGIN_CALLBACK_REGISTRATION(CHAT_DBSE_X)

		REGISTER_SENDER(CHATDB_SQUAD_REQUEST_Y, _sendSquadRequest, _SEND_SQUAD_REQUEST) ;

		REGISTER_MSG_RECEIVER(CHATDB_SQUAD_INFO_Y, CHATDB_SQUAD_XCHG) ;

		REGISTER_SENDRECV(CHATDB_SQUAD_MEMBER_UPDATE_Y, _sendSquadMemberUpdate, _recvSquadMemberUpdate, _XCHG_SQUAD_MEMBER_UPDATE) ;

	END_CALLBACK_REGISTRATION
}
