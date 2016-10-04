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

#include "comm/commTypes.h"

SINT32 commInstallHostCallbacks(void);
// installs callbacks for the host system
// no return value or arguments
// should be called only once

SINT32 activateClient(SINT32 teulIndex);

SINT32 commSendPlayerExit(SINT32 teulIndex, PLAYER_DESTINATION destination, PLAYER_EXIT_CODE exitCode, const FULL_OBJ_ID facility);

void commDropPlayer(class Player& persona, const bool ignoredPriorKick);
void commForcePlayerExit(class Player& persona, const char* reason, bool alert);

// Function that takes packed data and processes it as an update being received from a connection
SINT32 commRecvPeriodicUpdate(SINT32 teulIndex, Player* persona, UCHAR* message, SINT32 offset, bool isFromTeul);

// Function that receives data for a vehicle update
void commRecvVehicleUpdate(UINT32 serverAddress, const UCHAR* const buffer);

// Function that dispatches a resupply issue to a client
class HostVehicle;
void commSendClientResupplyIssue(HostVehicle* vehicle, UINT32 fromSerial, const ChatHandle fromHandle);

namespace Chat { struct PlayerMove; }
SINT32 commSendMove(SINT32 teulIndex, const Chat::PlayerMove& pos);
