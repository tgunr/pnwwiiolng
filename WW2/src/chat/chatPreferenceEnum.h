#pragma once
// Created: Tue Mar 20 10:58:28 CDT 2007
//
// List of preferences that can be sent to chat
//
// Copyright (C) PlayNet, Inc 1999-2007 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

enum CHAT_PREFERENCE
{
	CPR_INVALID = 0

,	CPR_UPDATE_MODE			// Request a specific update mode
,	CPR_CLIENT_LANGUAGE		// Language the client is in
,	CPR_PLAYER_LANGUAGE		// Language player speaks
,	CPR_IS_RECRUITING		// Player is actively seeking recruits
,	CPR_AUTO_LEADER			// Allow auto-promotion to leadership
,	CPR_MISSION_SPAM		// Announce people joining/leaving mission
,	CPR_SET_DECAL			// When the client makes this non-zero,
							// the host will change the player's decal
							// and update the database
,	CPR_SET_NETCODE			// Enables or disables the new netcode 

// Add new entries above here

,	MAX_CHAT_PREFERENCE
};
