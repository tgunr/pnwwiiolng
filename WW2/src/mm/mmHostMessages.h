#pragma once
// Created: Wed Apr 18 14:46:38 2001
//
// Host side message handling for the mmhost.  Specifically this module
// contains all the spawn message handling
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

#include "mm/mmSpawnTypes.h"

//Installs the host callbacks
extern SINT32 mmHostInstallCallbacks(void);

//Sends a non-mobile post Accept the front end on the connection
extern SINT32 mmSendPostAccept(SINT32 connection, WEAPON_SERIAL weaponSerial, LOGICAL newVehicle, SINT16 portal, bool mobileSpawnAvailable);

//sends a post deny to the front end on the connection
extern SINT32 mmSendPostDeny(SINT32 connection, WEAPON_SPEC weaponSpec, UINT32 reasonCode, bool loseWeapon);

//Sends a post Expired the front end on the connection
extern SINT32 mmSendPostExpired(SINT32 connection, WEAPON_SERIAL weaponSerial);

//Sends a post List response to the front end indexed by the connection
extern SINT32 mmSendPostListResponse(SINT32 connection, FULL_OBJ_ID facility, SINT16 numWeapons, COMMANDED_WEAPON* commandedWeapon);

//Sends a Join accept to a front end
extern SINT32 mmSendJoinAccept(SINT32 connection, MM_POSTED_WEAPON postedWeapon);

//Sends a Join Deny to a front end
extern SINT32 mmSendJoinDeny(SINT32 connection, UINT32 reasonCode);

//Sends a Join Permission Request to an FE
extern SINT32 mmSendJoinPermRequest(SINT32 connection, UINT32 joinRequestID, const ChatHandle gameName);

//Sends an FE Spawn Notify
extern SINT32 mmSendSpawnNotify(SINT32 connection, const WEAPON_SERIAL& weaponSerial, sortieid_t spawnVehicleID, FLOAT64 x, FLOAT64 y, FLOAT64 z, UINT8 decal, bool isBuilder, bool isHighCommander, bool isMissionLeader);

//Sends a crew bailed
extern SINT32 mmSendCrewBailed(SINT32 connection, UINT32 joinRequestID, UINT32 reasonCode);

//Sends a notify to the cell host that a vehicle is on its way
extern SINT32 mmSendWeaponsHandoff(UINT32 hostAddress, WEAPON_SERIAL weaponSerial, UINT32 commander, UINT32 crew);

