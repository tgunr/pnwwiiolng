#pragma once
// Created: Wed Jul 26 11:00:52 2000
//
// The main client-side API for the chat system
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

#include "chat/chatChannelTypes.h"
#include "chat/chatLStringIDs.h"
#include "chat/chatPrototypes.h"
#include "wwii/wwiiOwnershipEnums.h"
#include "chat/chatDebugConditions.h"
#include "chat/chatCommandWithArgs.h"

#define PREP_CHAT_MSG(varname, fmt, ...) \
        char varname[MAX_MESSAGE_LENGTH+1]; \
        snprintf(varname, MAX_MESSAGE_LENGTH, fmt, ##__VA_ARGS__);
#define FORMAT_CHAT_MSG(varname, fmt, ...) \
        snprintf(varname, MAX_MESSAGE_LENGTH, fmt, ##__VA_ARGS__);

class Player;
struct FULL_OBJ_ID;
namespace Chat { class Channel; }
extern SINT32 g_chatTeulIndex;

extern ChatHandle alliedHandleID;
extern ChatHandle axisHandleID;
extern ChatHandle crsHandleID;
extern ChatHandle adminHandleID;
extern ChatHandle systemHandleID;
extern ChatHandle ircHandleID;

// Commonly used channels so we don't have to keep finding them
extern Chat::Channel* crsChannel;
extern Chat::Channel* adminChannel;

extern bool inDebug;

static const UCHAR LOBBY_CHAN_ICON = 10;

static const int SYS_MOTD_LEN = 160;

// Which side does this country belong to ?
bool chatSideMessage(WWII_SIDE, LSTRING_ID messageID, const char* ptr0="", const char* ptr1="", const char* ptr2="", const char* ptr3="", const char* ptr4="", const char* ptr5="");

// Helper: Describe a property by name or co-ordinates if not found in lookup
const char* chatPropertyName(const FULL_OBJ_ID id);
const char* chatPropertyName(const class Property* property, const FULL_OBJ_ID id);
Chat::HostMessage& chatBuildHostMessage(const ChatHandle senderID, UINT32 msgID, UINT8 chanIcon, const char* msg);

//This is the logic that is run whenever someone sends a message to the host
void chatRouteMessage(
		class Player* player
		, Chat::ClientMessage* incomingMessage
		, UINT32 messageID
		, bool allowDotCommands
		);

UINT32 nextMessageID(void);

void chatDispatchCRSMessage(const SCHAR* source, const SCHAR* text, const ChatHandle excludeID=0LL);

void chatDispatchGMMessage(const SCHAR* source, const SCHAR* text, const ChatHandle excludeID=0LL);

bool chatDispatchHCText(WWII_SIDE side, const SCHAR* source, const SCHAR* text);
bool chatHCMessage(WWII_SIDE, LSTRING_ID, const char* ptr0="", const char* ptr1="", const char* ptr2="", const char* ptr3="", const char* ptr4="", const char* ptr5="");
bool chatHCMessage(WWII_SIDE, Chat::LocalizedMessage&, const ChatHandle exclude);

bool chatSystemMessage(LSTRING_ID messageID, const char* ptr0="", const char* ptr1="", const char* ptr2="", const char* ptr3="", const char* ptr4="", const char* ptr5="");

bool chatSendToRecipient(Player* player, Player* recipient, const Chat::HostMessage& message);

// Send a smite instruction to a player -- wrapper for chatSendSmiteUser since
// anyone outside of chatHost.cpp doesn't have the broadcast index.
void chatSmitePlayer(const playerid_t playerID);
void chatSmiteHandle(const char* handle);

// Multi-purpose release-from-grab function, will work out whether the
// client is either the grabber or the grabbed, and will cope with one half
// of the grab no-longer being in game.
void chatReleaseGrab(Player& player);

UINT32 chatGetStratAddr(void);

bool chatRouteSideMessage(WWII_SIDE toSide
						, LSTRING_ID prefix
						, LSTRING_ID message
						, const FULL_OBJ_ID objectiveID
						, const char* arg1
						, const char* arg2
						);

const char* chatForeignizeMessage(WWII_COUNTRY country, size_t maxLen);

void chatRouteGCIMessage(Chat::GCIMessage& message, UINT32 messageID);

void chatReportPlaceHolder(Player* player, CHANNEL_TYPE type);

void chatWarnAll(LSTRING_ID msg, const char* ptr0="", const char* ptr1="", const char* ptr2="", const char* ptr3="", const char* ptr4="", const char* ptr5="");

bool chatSetShutdownDowntime(UINT32);
void chatSetShutdownTime(UINT32, const char* caller);
SINT32 chatGetShutdownTimeRemaining();	// Returns -1 if no shutdown pending.

void chatSendAllMessage(const ChatHandle handle, const char* message, UINT32 messageID);

void chatSetHCMotd(const char* callsign, const char* motd, WWII_SIDE forSide);
