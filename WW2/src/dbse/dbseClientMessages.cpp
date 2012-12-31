#include <syscfg.h> // Path is project specific.
// Created: Fri Sep  8 13:56:52 2000
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Eric Hall

#include <stdlib.h>
#include <ctype.h>

#include "dbse/dbseConstants.h"
#include "dbse/dbseTypes.h"
#include "teul/teulKit.h"
#include "wwii/wwiiCallbacks.h"
#include "dbse/dbseClientMessages.h"
#include "dbse/dbseClient.h"

TEUL_CALLBACK* DBSE_SEND_CLIENT_HANDSHAKE;

extern void dbseSetConnectionStatus(CONNECTION_STATUS) ;

static SINT32
_sendHandshake(ACTION_OBJECT* const a)
{
	return dbsePackHandshake(TEUL_PACK, a->packed, a->offset,(HANDSHAKE*) a->localRawData,  sizeof(HANDSHAKE));
}

static SINT32 _recvHandshake(ACTION_OBJECT* const a)
{
	HANDSHAKE handshake;
	const SCHAR* address;

	a->offset = dbsePackHandshake(TEUL_UNPACK, a->packed, a->offset, &handshake, sizeof(HANDSHAKE));

	switch (handshake.status)
	{
	case CONNECT_REFUSE:
		dbugLog(_FLA,"DBHost arena does not match! Host version: %u",handshake.version);
		break;
	case CONNECT_ACCEPT:
	//case CONNECT_OPEN:
		dbugLog(_FLI,"DB Connection complete!");
		break;
	case CONNECT_REDIRECTED:
		//theHost = handshake.host;
		address = teulGetAddress(handshake.host);
		dbugLog(_FLW,"DB Connection redirected to %s",address);
		break;
	default:
		LogAbort("ERROR: Unable to determine db connection status\n Abort!\n");
	}
	dbseSetConnectionStatus(handshake.status);
	return(a->offset);
}

// --->error functions

//Installs the client callbacks
SINT32 dbseInstallClientCallbacks(void)
{
	BEGIN_CALLBACK_REGISTRATION(DBSE_GENERAL_X)

		REGISTER_SENDER(DBSE_CLIENT_HANDSHAKE_Y, _sendHandshake, DBSE_SEND_CLIENT_HANDSHAKE) ;
		REGISTER_RECEIVER(DBSE_SERVER_HANDSHAKE_Y, _recvHandshake) ;

	END_CALLBACK_REGISTRATION
}

SINT32
dbseSendHandshake(SINT32 teulIndex, UINT32 arena)
{
	static HANDSHAKE handshake;
	handshake.version = arena;
	return teulSend(teulIndex,DBSE_SEND_CLIENT_HANDSHAKE,&handshake);
}

