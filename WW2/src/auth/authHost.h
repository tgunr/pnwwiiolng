#pragma once
// Created: Wed Jul 26 11:00:52 2000
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Marty "Vandal" Poulin

enum WEB_ERR
{
	WEB_ERR_NONE=0,
	WEB_ERR_FULL,		//already maxusers in game
	WEB_ERR_LOGIN,		//login/passwd don't match
	WEB_ERR_GAME,		//arena is down
	WEB_ERR_INSTALL,	//need to install or update
	WEB_ERR_LOCKED,		//administratively locked
	WEB_ERR_INTERNAL,	// internal server error
	WEB_ERR_USER,		// Invalid username
	WEB_ERR_PASSWORD,	// Incorrect password
	WEB_ERR_SUB,		// Inactive subscription
	WEB_ERR_AUTH,		// Auth error
	WEB_ERR_MAX
};

bool authEstablishServer(SINT32 teulIndex, struct AUTH_GAME_INIT& hello);
void authProcessArenaRequest(SINT32 teulIndex, struct ARENA_REQ& request);
SINT32 authLaunchRequest(SINT32 teulIndex, struct APP_LAUNCH_REQ& request);

void handleServerLoadUpdate(const struct AUTH_SERVER_LOAD& message);
void handleGameCapacityUpdate(const struct AUTH_GAME_CAPACITY& message);

// Verify a specific checksum for the TOS for a given appid/platform
bool authVerifyChecksum(const SINT32 teulIndex, const UINT32 appID, const UINT32 platform, const char* const checksum);

namespace Auth { class GameList; }
extern Auth::GameList* authGetGameList(SINT32 teulIndex);
