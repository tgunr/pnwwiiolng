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

class Player ;
struct FULL_OBJ_ID ;
namespace Chat { class Channel ; }
extern SINT32 g_chatTeulIndex ;

static const int SYS_MOTD_LEN = 160 ;

// Which side does this country belong to ?
bool chatSideMessage(WWII_SIDE, LSTRING_ID messageID, const char* ptr0="", const char* ptr1="", const char* ptr2="", const char* ptr3="", const char* ptr4="", const char* ptr5="") ;

// Helper: Describe a property by name or co-ordinates if not found in lookup
const char* chatPropertyName(const FULL_OBJ_ID& id) ;
const char* chatPropertyName(const class Property* property, const FULL_OBJ_ID& id) ;
Chat::HostMessage& chatBuildHostMessage(const ChatHandle& senderID, UINT32 msgID, UINT8 chanIcon, const char* msg) ;

//This is the logic that is run whenever someone sends a message to the host
extern void chatRouteMessage(
			class Player* player
			, Chat::ClientMessage* incomingMessage
			, UINT32 messageID
			, bool allowDotCommands
			);

//This is the logic that is run whenver someone sends a handle update message
extern LOGICAL chatUpdateHandle(Player* player, struct UPDATEHANDLE& updateHandle);

extern UINT32 nextMessageID(void);

extern void chatDispatchCRSMessage(const SCHAR* source, const SCHAR* text, const ChatHandle& excludeID=0LL) ;

extern void chatDispatchGMMessage(const SCHAR* source, const SCHAR* text, const ChatHandle& excludeID=0LL) ;

extern bool chatDispatchHCText(WWII_SIDE side, const SCHAR* source, const SCHAR* text) ;
extern bool chatHCMessage(WWII_SIDE, LSTRING_ID, const char* ptr0="", const char* ptr1="", const char* ptr2="", const char* ptr3="", const char* ptr4="", const char* ptr5="") ;
extern bool chatHCMessage(WWII_SIDE, Chat::LocalizedMessage&, const ChatHandle& exclude) ;

extern bool chatSystemMessage(LSTRING_ID messageID, const char* ptr0="", const char* ptr1="", const char* ptr2="", const char* ptr3="", const char* ptr4="", const char* ptr5="") ;

extern bool chatSendToRecipient(Player* player, Player* recipient, const Chat::HostMessage& message) ;

// Send a smite instruction to a player -- wrapper for chatSendSmiteUser since
// anyone outside of chatHost.cpp doesn't have the broadcast index.
extern void chatSmitePlayer(const playerid_t playerID) ;
extern void chatSmiteHandle(const char* handle) ;

// Multi-purpose release-from-grab function, will work out whether the
// client is either the grabber or the grabbed, and will cope with one half
// of the grab no-longer being in game.
extern void chatReleaseGrab(Player& player) ;

extern ChatHandle alliedHandleID;
extern ChatHandle axisHandleID;
extern ChatHandle crsHandleID ;
extern ChatHandle adminHandleID ;
extern ChatHandle systemHandleID ;
extern ChatHandle ircHandleID ;

// Commonly used channels so we don't have to keep finding them
extern Chat::Channel* crsChannel ;
extern Chat::Channel* adminChannel ;

extern UINT32 chatGetStratAddr(void) ;

extern bool chatRouteSideMessage(WWII_SIDE toSide
							, LSTRING_ID prefix
							, LSTRING_ID message
							, const struct FULL_OBJ_ID& objectiveID
							, const char* arg1
							, const char* arg2
							) ;

extern const char* chatForeignizeMessage(WWII_COUNTRY country, size_t maxLen) ;

extern void chatRouteGCIMessage(Chat::GCIMessage& message, UINT32 messageID) ;

extern void chatReportPlaceHolder(Player* player, CHANNEL_TYPE type) ;

extern void chatWarnAll(LSTRING_ID msg, const char* ptr0="", const char* ptr1="", const char* ptr2="", const char* ptr3="", const char* ptr4="", const char* ptr5="") ;

extern bool chatSetShutdownDowntime(UINT32) ;
extern void chatSetShutdownTime(UINT32, const char* caller) ;
extern SINT32 chatGetShutdownTimeRemaining() ;	// Returns -1 if no shutdown pending.

extern bool inDebug ;

extern void chatSendAllMessage(const ChatHandle& handle, const char* message, UINT32 messageID) ;

static const UCHAR LOBBY_CHAN_ICON = 10 ;

extern void chatSetHCMotd(const char* callsign, const char* motd, WWII_SIDE forSide) ;

