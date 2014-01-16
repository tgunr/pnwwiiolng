#pragma once
// Copyright 1999 Playnet Inc, All rights reserved
// Not to be distributed in API form except as part of a compiled
// library
//
// Originating Programmer: Martin Poulin

#include "comm/commTypes.h"
#include "wwii/wwiiExitCodes.h"

extern SINT32	_LogonError;
extern SINT32	_IsConnected;
extern SINT32	GameId;

extern SINT32	frametime;

struct TEUL_CALLBACK;

// All teul callback modules use a mixture of global functions and
// global callback refs.

// an external callback ref is used instead of an external function by calling
//
//		teulSend(SINT32 connection,TEUL_CALLBACK* callback,(void*) data);
//
// Where:
//
// connection is the connection you want to send the callback on
// callback is the callback ref
// data is the data structure passed to the send routine.
//
// simply fill out the data structure being passed and call teulSend.

// sends a regular update to the host from the client.
//	Usage:
//		teulSend(Connection,commREQ_VEHICLE,(SHARED_VEHICLE_INFO*)localVehicleHandle);
//
// refer to typedef of SHARED_VEHICLE_INFO in netTypes.h for parameters
extern TEUL_CALLBACK*		commUPDATE;
struct COMM_UPDATE_BUFFER { UCHAR data[1400]; SINT32 size; };

// sends a strat damage message to the strat updater
// i.e. cp or building hit.. go bombs!
extern TEUL_CALLBACK*		commSTRAT_DAMAGE;

// sends a simple, no-data message to the host that we are disconnecting.
//	Usage:
//		teulSend(Connection,commDISCONNECT,nullptr);
//
extern TEUL_CALLBACK*		commDISCONNECT;

// sends a message to the host from the client.
//	Usage:
//		teulSend(Connection,commCLIENT_HIT, Vehicle::Hit& hit);
//
// refer to typedef of VEHICLE_HIT in comTypes.h for parameters
extern TEUL_CALLBACK*		commCLIENT_HIT;

SINT32 commSendHostMessage(SINT32 connection, SINT32 identifier, SINT32 flags,
								  const char* message);
// sends a text message to the host.
// index refers to the player to whom the message is directed,
// optionally for use as a channel.
// flags is a union of information about the message
// message is a buffer of data, max 79 characters in length, to be sent.
// returns 0 if message was sent
// returns -1 if message was not sent

SINT32 commInstallClientCallbacks(void);
// installs all callbacks for client into system
// no return value or arguments
// should be called only once

SINT32 commSendPlayerExit(SINT32 teulIndex, PLAYER_DESTINATION destination,	PLAYER_EXIT_CODE exitCode, FULL_OBJ_ID facility);

SINT32 commSendVehicleJoin(SINT32 teulIndex, VEHICLE_REQUEST* lvr);

// Function that unpacks and processes an incoming world update
SINT32 commRecvWorldUpdateData(const SINT32 teulIndex, UCHAR* const data, SINT32 offset, const SINT32 size_bytes);

// Notify the host that a vehicle has caused us to die
void commSendKillNotice(SINT32 hostIndex, UINT32 myVehicleID, UINT32 hostVehicleID);
