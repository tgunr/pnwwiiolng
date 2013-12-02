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

//This function installs all the host callbacks
extern SINT32 chatInstallHostCallbacks(void);

//Sends a userNLA to the front end.  I should be using more of these
//	functions, but you know how it goes.  I didn't come up with this method of
//	doing things until long after I was finished with most of chat.
extern SINT32 chatSendUserNLA(SINT32 connection, UCHAR radioIndex, const ChatHandle& handleID);

//Sends a message to the mmHost telling it to kill the user referenced
extern SINT32 chatSendSmiteUser(UINT32 customerID, const char* handle);

// Send a strat message containing a command request from a specific player regarding a specific cp
extern bool chatSendStratCommand(SINT32 playerID, int cmd, const struct FULL_OBJ_ID& cp, WWII_SIDE side);

extern void chatSendBrigadeMove(const ChatHandle& byPlayer, bool isGM, UINT32 brigadeID, const FULL_OBJ_ID& fid, bool isActionable);
extern void chatSendHCUnitUpdate(const Player*, UINT32 unitID);

extern SINT32 chatSendPopulation();

extern void chatSendLocateUser(Player& requester, UINT32 playerID);

extern bool chatSendBandwidthNotice(playerid_t playerID, UINT32 mode);

extern bool chatMessageChannel(const ChannelID& channelID, LSTRING_ID prefix, LSTRING_ID message, const char* arg1 = NULL, const char* arg2 = NULL);

extern void chatSendMotd(SINT32 teulIndex, struct MOTD_REPLY& reply);

extern void chatSendLeaderChange(UINT32 missionID, UINT32 playerID, const ChatHandle& handle, const ChatHandle& requester);
extern void chatSendBackupLeaderChange(UINT32 missionID, UINT32 playerID);

extern void chatSendMapPersonaUpdate(UINT32 playerID, bool isSquadUpdate, UINT32 squadID, const ChatHandle& squadHandle, bool isDecalUpdate, UINT32 decal);

extern const char* g_countries[];
extern const char* g_sides[];

// Notify a player's cell host that the player MUST despawn.
extern bool chatSendForcedDespawn(Player* player, LSTRING_ID messageID, const char* reason);

extern bool chatSendUMSKill(const Player* player, const UINT32 stoUID);

namespace Chat
{
	extern void SetupRaknet(UINT32 baseHost, UINT32 localHost);

	struct PlayerMove;
	extern bool SendPlayerMove(UINT32 cellIP, Chat::PlayerMove& move);

	// Save any player submitted tutorial progress updates.
	extern bool CommitProgressUpdates();
	extern bool CommitDecalUpdates();
}
