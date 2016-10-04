#include <syscfg.h> // Path is project specific.
// Created: Fri Sep  8 13:56:52 2000
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Eric Hall

#include "dbse/dbseConstants.h"
#include "teul/teulBasic.h"
#include "wwii/wwiiCallbacks.h"
#include "dbse/dbseClientMessages.h"
#include "dbse/dbseClient.h"
#include "dbse/dbseTypes.h"

SINT32 dbseRecvHandshake(ACTION_OBJECT* a, Dbse::Handshake&);

// --->error functions

//Installs the client callbacks
SINT32 dbseInstallClientCallbacks(void)
{
	BEGIN_CALLBACK_REGISTRATION(DBSE_GENERAL_X)
	{
		REGISTER_RECEIVEOBJ(DBSE_SERVER_HANDSHAKE_Y, Dbse::Handshake, dbseRecvHandshake);
	}
	END_CALLBACK_REGISTRATION(DBSE_GENERAL_X)
}

SINT32 dbseSendHandshake(SINT32 teulIndex, UINT32 arena)
{
	Teul::Packet outgoing(Teul::PacketId(DBSE_GENERAL_X, DBSE_CLIENT_HANDSHAKE_Y));
	outgoing.construct<Dbse::Handshake>(arena);
	return outgoing.send(teulIndex);
}
