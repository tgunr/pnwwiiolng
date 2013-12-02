#pragma once
// Created: Thu Oct  4 14:15:12 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

#ifdef BETA_RELEASE
# define DEBUG_SUPPLY
#endif

#include "strt/strtChatTypes.h"
#include "chat/chatLStringIDs.h"

//Installs the callbacks
extern SINT32 chatInstallStratSupportCallbacks(void);

// Send an SCMD result to an incoming oob message
extern bool chatSendSCMDResult(SINT32 playerID, SCMD_ENUM command, FULL_OBJ_ID cp, SCMD_RESULT result, UINT32 data = 0);

//tolua_begin
extern UINT32 chatGetPopulation(WWII_SIDE side);
extern bool chatSendSideMessage(WWII_SIDE side, const FULL_OBJ_ID& objective, LSTRING_ID prefix, LSTRING_ID message, const char* arg1=NULL, const char* arg2=NULL);
extern bool chatSendHCMessage(WWII_SIDE side, LSTRING_ID message, const char* arg1=NULL, const char* arg2=NULL);
//tolua_end

