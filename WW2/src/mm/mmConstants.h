#pragma once
// Created: Wed Apr 18 14:57:48 2001
//
// Constants for the mmhost
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

namespace MM
{
	enum { CallbackX = 227 };

	enum
	{
		FE_POST_REQ_Y = 200,
		FE_POST_ACCEPT_Y,
		FE_POST_DENY_Y,
		FE_POST_LIST_REQ_Y,
		FE_POST_LIST_RESPONSE_Y,
		FE_JOIN_REQ_Y,
		FE_JOIN_ACCEPT_Y,
		FE_JOIN_DENY_Y,
		FE_JOIN_PERM_REQUEST_Y,
		FE_CREW_REQUEST_Y,
		FE_CREW_ACCEPT_Y,	// 210
		FE_POST_EXPIRED_Y,
		FE_POST_CANCEL_Y,
		FE_JOIN_PERM_KILL_Y,
		FE_CLIENT_SPAWN_Y,
		FE_SPAWN_NOTIFY_Y,
		FE_SPAWN_CANCEL_Y,
		FE_CREW_BAILED_Y,
		WEAPONS_HANDOFF_Y,
		MOBILE_SPAWN_UPDATE_Y,
		DIRECT_SPAWN_Y,		// 220
		DO_DESPAWN_Y			// Player returning to lobby.
	};
}

