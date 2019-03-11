#pragma once
// Created: Mon Jul 31 13:29:38 2000
//
// The chat system host messages.  This module is actually included in the
// game host, not the chat host.
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

#include "chat/chatLStringIDs.h"
#include "chat/chatChannelID.h"

extern struct TEUL_CALLBACK*	CHAT_SEND_MESSAGE;
extern struct TEUL_CALLBACK*	CHAT_XCHG_SERVER_TRACKED_OBJS;

extern const char* g_countries[];
extern const char* g_sides[];

//This function installs all the host callbacks
SINT32 chatInstallHostCallbacks();

//Sends a userNLA to the front end.  I should be using more of these
//	functions, but you know how it goes.  I didn't come up with this method of
//	doing things until long after I was finished with most of chat.
SINT32 chatSendUserNLA(SINT32 connection, UCHAR radioIndex, const ChatHandle handleID);

//Sends a message to the mmHost telling it to kill the user referenced
SINT32 chatSendSmiteUser(UINT32 customerID, const char* handle);

// Send a strat message containing a command request from a specific player regarding a specific cp
bool chatSendStratCommand(SINT32 playerID, int cmd, const struct FULL_OBJ_ID cp, WWII_SIDE side);

void chatSendBrigadeMove(const ChatHandle byPlayer, bool isGM, UINT32 brigadeID, const FULL_OBJ_ID fid, bool isActionable);
void chatSendGarrisonChangeCountry(const ChatHandle byPlayer, bool isGM, UINT32 brigadeID, const FULL_OBJ_ID fid, const WWII_COUNTRY country, bool isActionable);

void chatSendHCUnitUpdate(const Player*, UINT32 unitID);

SINT32 chatSendPopulation();

void chatSendLocateUser(Player& requester, UINT32 playerID);

bool chatSendBandwidthNotice(const Player& player, UINT32 mode);
bool chatSendNetcodeNotice(const Player& player, bool netcode);

bool chatMessageChannel(const ChannelID& channelID, LSTRING_ID prefix, LSTRING_ID message, const char* arg1 = nullptr, const char* arg2 = nullptr);

void chatSendMotd(SINT32 teulIndex, struct MOTD_REPLY& reply);

void chatSendLeaderChange(UINT32 missionID, UINT32 playerID, const ChatHandle handle, const ChatHandle requester);
void chatSendBackupLeaderChange(UINT32 missionID, UINT32 playerID);

void chatSendMapPersonaUpdate(UINT32 playerID, bool isSquadUpdate, UINT32 squadID, const ChatHandle squadHandle, bool isDecalUpdate, UINT32 decal);

// Notify a player's cell host that the player MUST despawn.
bool chatSendForcedDespawn(Player* player, LSTRING_ID messageID, const char* reason);

bool chatSendUMSKill(const Player* player, const UINT32 stoUID);

namespace Chat
{
	struct PlayerMove;
	bool SendPlayerMove(UINT32 cellIP, Chat::PlayerMove& move);

	// Save any player submitted tutorial progress updates.
	bool CommitProgressUpdates();
}
