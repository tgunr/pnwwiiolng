#pragma once
// This module provides object send and receive routines
// as well as the callback object allocations that use them.
//
// in rare instances there are also driver routines that
// call teulSendCALLBACK, but these are depritiated.
// instead data structures should be set up to contain
// the minumum data required to send or pointers to all
// data to send a callback.
//
// Copyright 1999 Playnet Inc, All rights reserved
// Not to be distributed in API form except as part of a compiled
// library
//
// Originating Programmer: Martin Poulin

#include "auth/authTypes.h"
#include "teul/teulFuncTypes.h"		// For ERROR_FUNCTION_REF

extern TEUL_CALLBACK*	authGAME_INIT;
extern TEUL_CALLBACK*	authSERVER_LOAD;
extern TEUL_CALLBACK*	authGAME_CAPACITY;

// init auth game callbacks
SINT32 authInstallGameCallbacks(ERROR_FUNCTION* externErrorFn);


// used to report client actions (login, logout etc..)
// <connection> the auth connection
// <costomerID> the customer ID of the client.
// <gameID> the game that is being played.
// <actionType> action (login, logout, etc..)
// <ip> ip address of the player
SINT32 authSendClientAction(SINT32 connection, UINT32 arenaID,
							UINT32 customerID
							, CLIENT_ACTIONS action
							, UINT32 ip);

// used to clear the current client connection table.  Sent at the
//	beginning to whack any stale data from the current connection table.
//	<connection> is the connection the message should go out on
SINT32 authSendGameInit(SINT32 connection);

// from teulAuth.c
SINT32 authAcceptPlayer(SINT32 teulIndex, UINT32 playerIP, struct PLAYER_LOGIN& loginData);

