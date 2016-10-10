#include <syscfg.h> // Path is project specific.
// Created: Tue 03 Feb 2004, 11:53:00
//
// Receive client packet describing hardware information.
//
// Copyright (c) 2004 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "auth/authClientData.h"
#include "auth/authDB.h"
#include "include/infoMessageTypes.h"
#include "include/infoPack.h"
#include "str/stringLib.h"
#include "teul/teulKit.h"
#include "wwii/wwiiCallbacks.h"

// Callback pointers
static TEUL_CALLBACK*		_INFO_REQ;

//////////////////////////////////////////////////////////////////////

SINT32
infoSendCompInfoRequest(SINT32 connection, const char* message)
{
	COMPUTER_INFO_REQ req;

	// Don't pass a NULL, pass an empty string.
	assert(message != nullptr);

	strsafecpy(req.text, message);
	req.size = (UINT16)strlen(req.text) + sizeof('\0');

	return teulSend(connection, _INFO_REQ, &req);
}

//////////////////////////////////////////////////////////////////////
// receive a "Playgate InfoSend" packet

static SINT32
_recvPlayInfoSend(ACTION_OBJECT* const a)
{
	PLAY_COMPUTER_INFO	info;
	info.version = 0;

	if ( a->sizeInBytes() != sizeof(info) )
	{
		dbugLog(_FLM, "t:%d ignoring hardware survey (size mismatch): expected %" PRSizeT ", incoming %d"
						, a->teulIndex, sizeof(info), a->sizeInBytes());
		return a->offset;
	}

	a->offset = teulPackVoid(TEUL_UNPACK, a->packed, a->offset, &info, sizeof(info));
	if ( info.version != info.CurrentVersion() )
	{
		dbugLog(_FLM, "t:%d ignoring old version of hardware survey ver:%uv%u"
						, a->teulIndex
						, info.version, info.CurrentVersion()
						);
		return a->offset;
	}

	Auth::ClientData* cdata = (Auth::ClientData*)teulGetPersonaData(a->teulIndex);
	if ( cdata == nullptr )
	{
		dbugLog(_FLW, "t:%d no connection data for playInfoSend", a->teulIndex);
		return -EINVAL;
	}

	if ( g_logDebug )
		dbugLog(_FLD, "t:%d c:%s received playinfosend", a->teulIndex, cdata->m_username.c_str());

	// Do something with the data
	authdbStoreComputerInfo(*cdata, info);

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// request a PlayGate info packet

static SINT32
_sendPlayInfoReq(ACTION_OBJECT* const a)
{
	return packCompInfoReq(TEUL_PACK, a->packed, a->offset, (COMPUTER_INFO_REQ*)a->localRawData, sizeof(COMPUTER_INFO_REQ));
}

//////////////////////////////////////////////////////////////////////
// Install the callbacks for the playgate messages

SINT32
infoInstallHostCallbacks()
{
	BEGIN_CALLBACK_REGISTRATION(PgInfo::CallbackX)
	{
		REGISTER_RECEIVER(PLAY_INFO_SEND_Y, _recvPlayInfoSend);
		REGISTER_SENDER(PLAY_INFO_REQ_RECV_Y, _sendPlayInfoReq, _INFO_REQ);
	}
	END_CALLBACK_REGISTRATION(PgInfo::CallbackX)
}

