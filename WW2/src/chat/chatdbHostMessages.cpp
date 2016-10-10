#include <syscfg.h> // Path is project specific.
// Date: Sun Feb 15 2004
//
// DBSE side of chatdb callbacks
//
// Copyright (c) 2004 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "wwii/wwiiCallbacks.h"
#include "chat/chatdbHostMessages.h"
#include "dbse/dbseConstants.h"
#include "teul/teulPackers.h"
#include "chat/chatTypes.h"
#include "chat/chatdbParser.h"
#include "chat/chatdbSquadXchg.h"
#include "chat/chatSquadMemberUpdates.h"

// "XCHG" indicates a callback which has send/recv

static TEUL_CALLBACK*	_XCHG_SQUAD_MEMBER_UPDATE;

//////////////////////////////////////////////////////////////////////
// Private functions
//////////////////////////////////////////////////////////////////////

//// Send functions

static SINT32
_sendSquadMemberUpdate(ACTION_OBJECT* const a)
{
	return ((SquadMemberUpdate*)a->localRawData)->packer(TEUL_PACK, a);
}

//// Recv functions

//////////////////////////////////////////////////////////////////////

static SINT32
_recvSquadRequest(ACTION_OBJECT* const a)
{
	UINT32 squadID;
	a->offset = teulPackUINT32(TEUL_UNPACK, a->packed, a->offset, &squadID, 0);

	if ( squadID <= 0 )
		return a->offset;

	chatdbSquadInfo(a->teulIndex, squadID, true);

	return a->offset;
}

static SINT32
_recvSquadMemberUpdate(ACTION_OBJECT* const a)
{
	SquadMemberUpdate incoming(a);
	if ( a->offset <= 0 )
		return a->offset;

	chatdbSquadMemberUpdate(a->teulIndex, incoming);

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Public functions
//////////////////////////////////////////////////////////////////////

SINT32
chatdbSendSquadMemberUpdateResult(SINT32 teulIndex, SquadMemberUpdate& result)
{
	return teulSend(teulIndex, _XCHG_SQUAD_MEMBER_UPDATE, &result);
}

//////////////////////////////////////////////////////////////////////
// Install the callbacks

SINT32
chatdbInstallHostCallbacks()
{
	BEGIN_CALLBACK_REGISTRATION(CHAT_DBSE_X)
	{
		REGISTER_RECEIVER(CHATDB_SQUAD_REQUEST_Y, _recvSquadRequest);

		REGISTER_MSG_SENDER(CHATDB_SQUAD_INFO_Y, CHATDB_SQUAD_XCHG);

		REGISTER_SENDRECV(CHATDB_SQUAD_MEMBER_UPDATE_Y, _sendSquadMemberUpdate, _recvSquadMemberUpdate, _XCHG_SQUAD_MEMBER_UPDATE);
	}
	END_CALLBACK_REGISTRATION(CHAT_DBSE_X)
}
