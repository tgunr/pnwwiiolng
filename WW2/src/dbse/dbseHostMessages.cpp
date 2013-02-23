#include <syscfg.h> // Path is project specific.
// Created: Mon Sep 11 18:49:19 2000
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Eric Hall

#include "dbse/dbseConstants.h"
#include "dbse/dbseTypes.h"
#include "teul/teulServer.h"
#include "teul/teulPackers.h"
#include "wwii/wwiiCallbacks.h"
#include "teul/teulBasic.h"
#include "server/serverConfigVars.h"

TEUL_CALLBACK*	DBSE_SEND_HANDSHAKE;

static SINT32 _recvHandshake(ACTION_OBJECT* const a);
static SINT32 _sendHandshake(ACTION_OBJECT* const a);

SINT32
dbseInstallHostCallbacks()
{
	BEGIN_CALLBACK_REGISTRATION(DBSE_GENERAL_X)

		REGISTER_RECEIVER(DBSE_CLIENT_HANDSHAKE_Y, _recvHandshake) ;
		REGISTER_SENDER(DBSE_SERVER_HANDSHAKE_Y, _sendHandshake, DBSE_SEND_HANDSHAKE) ;

	END_CALLBACK_REGISTRATION
}

static SINT32 _sendHandshake(ACTION_OBJECT* const a)
{
	return dbsePackHandshake(TEUL_PACK, a->packed, a->offset, (HANDSHAKE*) a->localRawData, sizeof(HANDSHAKE));
}

static SINT32 _recvHandshake(ACTION_OBJECT* const a)
{
	dbugLog(_FLI,"Recieving Handshake for Client");

	HANDSHAKE handshake;
	a->offset = dbsePackHandshake(TEUL_UNPACK, a->packed, a->offset, &handshake, sizeof(HANDSHAKE));

	if (a->offset >= 0)
	{
		if (handshake.version != cv_arenaID.GetUint32())
		{
			handshake.status = CONNECT_REFUSE;
			handshake.version = cv_arenaID.GetUint32();
		}else
		{
			handshake.status = CONNECT_ACCEPT;
			const UINT32 destAddress = teulGetEndpointRemoteAddress(a->teulIndex) ;
			dbugLog(_FLI,"Connection opened from %s",teulGetAddress(destAddress));
		}

		teulSend(a->teulIndex,DBSE_SEND_HANDSHAKE, &handshake);
	}
	return a->offset;
}

