#pragma once
// Created: Thu Jun 03 10:00:00 2004
//
// Copyright (c) 2004 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Oliver "kfsone" Smith

// Vote "hold" on a specified decision
extern bool chatVoteHold(class Player* player, const char* id) ;
extern bool chatStartHold(Player* player, const struct FULL_OBJ_ID& target) ;

// Vote "fallback" on a specified decision
extern bool chatVoteFallback(Player* player, const char* id) ;
extern bool chatStartFallback(Player* player, const FULL_OBJ_ID& target) ;

// Vote "delay" (veto) on a specified decision
extern bool chatVetoDecision(Player* player, const char* id) ;

// Check/drain the decision queues
extern bool chatCheckDecisionQueues() ;

// Send a player an SCMD Result message
extern void chatSendSCMDResultMessage(Player* player, UINT32 result, const char* cmd, const char* cpName, UINT32 delay) ;

