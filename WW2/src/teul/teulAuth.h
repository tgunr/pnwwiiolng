#pragma once
// The main client-side API for the games to interface with the authHost
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a compiled library.
//
// Originating Programmer: Marty "Vandal" Poulin

#include "updt/loginTypes.h"

using APID = int;

namespace Auth { struct Dispatch; }
using PLAYER_ACCEPT_FUNCTION = SINT32(*)(SINT32 teulIndex, Auth::Dispatch* authPersona);

// This function initializes the game servers communications to the auth
//	server.  It takes all the information provided and establishes the
//	permanent link to the auth server that is used for authentication
//	purposes.
//	PARAMETERS:
//  	gameAddress - the IP address the game server is running on
//		gamePort - the port the game server is listening for front ends on
//		appID - the application ID necessary to connect to this game server
//		appVersion - the minimum version of the application necessary to
//			connect to this server
//		arenaID - the arenaID the server handles
//		hostVersion - the version of the arena the server is handling
//		maxUsers - the maximum users this server can handle
//		externErrorFn - the error function to invoke when things go bad for
//			this connection
SINT32 authGameInit(UINT32 gameHost, UINT16 gamePort
				, UINT32 authHost, UINT16 parentPort
				, APID appID, UINT32 appVersion
				, UINT32 hostVersion
				, UINT32 maxUsers
				, PLAYER_ACCEPT_FUNCTION acceptFn
				, ERROR_FUNCTION externErrorFn);

// authExitPlayer notifies the auth server that a player has left the game
//	PARAMETERS:
//		player - the pointer to the Auth::Dispatch struct that was returned by
//			authValidatePlayer above
SINT32 authExitPlayer(const Auth::Dispatch& player, UINT32 arenaID);


SINT32 authUpdateServerLoad(SINT32 numPlayers, UINT32 hostAddress, UINT32 arenaID);
SINT32 authSetGameCapacity(SINT32 capacity, SINT32 totalPlayers, UINT32 arenaID, SINT32 lock);
