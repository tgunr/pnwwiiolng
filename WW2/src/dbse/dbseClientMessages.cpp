#include <syscfg.h> // Path is project specific.
// Created: Fri Sep  8 13:56:52 2000
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Eric Hall

#include "dbse/dbseConstants.h"
#include "dbse/dbseTypes.h"
#include "teul/teulKit.h"
#include "wwii/wwiiCallbacks.h"
#include "dbse/dbseClientMessages.h"
#include "dbse/dbseClient.h"

extern void dbseSetConnectionStatus(CONNECTION_STATUS);

static SINT32 _recvHandshake(ACTION_OBJECT* const a, DBSE_HANDSHAKE& handshake)
{
	switch (handshake.m_status)
	{
	case CONNECT_BAD:
		dbugLog(_FLA, "dbse host version mismatch, rejected version %u", handshake.m_version);
		break;

	case CONNECT_REFUSE:
		dbugLog(_FLA, "dbse hose arena mismatch, rejected arena request for arena %u", handshake.m_arenaID);
		break;

	case CONNECT_ACCEPT:
		dbugLog(_FLI,"db connection accepted!");
		break;

	default:
		LogAbort("ERROR: Unable to determine db connection status\n Abort!\n");
	}

	dbseSetConnectionStatus(handshake.m_status);

	return(a->offset);
}

// --->error functions

//Installs the client callbacks
SINT32 dbseInstallClientCallbacks(void)
{
	BEGIN_CALLBACK_REGISTRATION(DBSE_GENERAL_X)

		REGISTER_RECEIVEOBJ(DBSE_SERVER_HANDSHAKE_Y, DBSE_HANDSHAKE, _recvHandshake);

	END_CALLBACK_REGISTRATION
}

SINT32 dbseSendHandshake(SINT32 teulIndex, UINT32 arena)
{
	Teul::Packet outgoing(sizeof(DBSE_HANDSHAKE), DBSE_GENERAL_X, DBSE_CLIENT_HANDSHAKE_Y);
	(void)CONSTRUCT_IN_PACKET(outgoing, DBSE_HANDSHAKE, arena);
	return outgoing.send(teulIndex);
}
