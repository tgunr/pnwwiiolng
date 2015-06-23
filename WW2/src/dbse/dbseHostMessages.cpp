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
#include "wwii/wwiiCallbacks.h"
#include "teul/teulBasic.h"
#include "server/serverConfigVars.h"

static SINT32 _recvHandshake(ACTION_OBJECT* const a, Dbse::Handshake& handshake)
{
	const UINT32 destAddress = teulGetEndpointRemoteAddress(a->teulIndex);
	const UINT32 myArenaID = cv_arenaID.GetUint32();
	if ( handshake.m_version != handshake.CurrentVersion )
	{
		handshake.m_status = Dbse::ConnectionStatus::Bad;
		dbugLog(_FLW, "t:%d %s dbse request has wrong version %u, expected %u"
					, a->teulIndex, teulGetAddress(destAddress)
					, handshake.m_version, handshake.CurrentVersion
					);
	}
	else if ( handshake.m_arenaID != myArenaID )
	{
		handshake.m_status = Dbse::ConnectionStatus::Refuse;
		dbugLog(_FLM, "t:%d %s requested arenaID %u"
					, a->teulIndex, teulGetAddress(destAddress), myArenaID);
	}
	else
	{
		handshake.m_status = Dbse::ConnectionStatus::Accept;
		dbugLog(_FLI, "t:%d ar:%u connection opened from %s"
					, a->teulIndex, myArenaID, teulGetAddress(destAddress));
	}

	Teul::Packet outgoing(Teul::PacketId(DBSE_GENERAL_X, DBSE_SERVER_HANDSHAKE_Y), handshake);
	outgoing.send(*a->endpoint);

	return a->offset;
}

SINT32 dbseInstallHostCallbacks()
{
	BEGIN_CALLBACK_REGISTRATION(DBSE_GENERAL_X)

		REGISTER_RECEIVEOBJ(DBSE_CLIENT_HANDSHAKE_Y, Dbse::Handshake, _recvHandshake);

	END_CALLBACK_REGISTRATION
}
