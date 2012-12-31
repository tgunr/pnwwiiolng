#pragma once
// Created: Wed Aug  8 13:51:08 CDT 2007
//
// Message sending RPC functions for chat support
//
// Copyright (C) PlayNet, Inc 1999-2007 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "chat/chatLStringIDs.h"

class CPStruct ;
struct FULL_OBJ_ID ;

//// Because these have varargs, tolua++ doesn't yet support them

// Send a [formatted] message that is broadcast to all players serverwide as SYSTEM:
extern bool chatSendAdminMessage(const char* fmt, ...) ;

// Formats a text string into an LString to send to a player
bool chatSendDirectMessage(const playerid_t playerID, LSTRING_ID prefix, LSTRING_ID message, const char* fmt, ...) ;

//tolua_begin
// Send an LString message directly to a single player
extern bool chatSendLocalizedMessage(const playerid_t playerID, LSTRING_ID prefix, LSTRING_ID message
 									, const char* ptr0="", const char* ptr1="", const char* ptr2="", const char* ptr3="", const char* ptr4="", const char* ptr5="") ;

// Broadcast a "SYSTEM:" message.
extern bool chatSendBroadcastAll(LSTRING_ID prefix, LSTRING_ID message
 									, const char* ptr0="", const char* ptr1="", const char* ptr2="", const char* ptr3="", const char* ptr4="", const char* ptr5="") ;

// Send a TickSample to the chat host.
extern void chatSendTickSample(UINT32 source, playerid_t playerID, const void* data) ;
//tolua_end
