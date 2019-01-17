#pragma once
// Created: Wed Jan 31 15:04:52 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

#include "wwii/wwiiRTBCodes.h"
#include "garrisons/garrisonCountryChangeRequest.h"

extern SINT32 strtInstallMapCallbacks();

//this function sends a message from the MMhost to the strat system telling
//	it to reserve a weapon for a front end
//This function sends a request to post a weapon at a facility
extern SINT32 strtSendMMPostRequest(SINT32 connection, UINT32 customerID, const char* callsign
						, UINT32 missionID, UINT32 brigadeID, const PermissionMask permMask
						, const FULL_OBJ_ID facility, WEAPON_SPEC weapon
						, bool multicrew, bool immediateSpawn);

//This function sends a request to list all the posted weapons at a facility
extern SINT32 strtSendMMPostListRequest(SINT32 connection, UINT32 customerID, const char* callsign, UINT32 missionID, UINT32 brigadeID, const FULL_OBJ_ID facility);

//This function sends a join request to the strat system
extern SINT32 strtSendMMJoinRequest(SINT32 connection, UINT32 customerID, const struct WEAPON_SERIAL& weaponSerial);

//Asks the strat system to grant the spawn of the weapon Serial referenced
extern SINT32 strtSendMMSpawnGrantRequest(SINT32 connection, const WEAPON_SERIAL& weaponSerial, class Player& player);

//This function sends a post cancel to the strat system
extern SINT32 strtSendMMPostCancel(SINT32 connection, const WEAPON_SERIAL& weaponSerial);

//This function sends a spawn cancel to the strat system
//extern SINT32 strtSendMMSpawnCancel(SINT32 connection, UINT32 customerID, WEAPON_SERIAL weaponSerial);
extern SINT32 strtSendMMSpawnCancel(SINT32 connection, UINT32 customerID, const FULL_OBJ_ID facility, const WEAPON_SERIAL& weaponSerial, const FULL_OBJ_ID originFacility = nullObjID);

//This function sends a capacity request to the strat system
extern SINT32 strtSendMMCapacityRequest(SINT32 connection, UINT32 tag, UINT32 missionID, UINT32 playerID, const FULL_OBJ_ID facility, CODE_RTB rtb);

extern SINT32 strtSendMMPostInfoUpdate(SINT32 connection, UINT32 customerID, const WEAPON_SERIAL& weaponSerial, bool multicrew);

extern SINT32 strtSendMMJoinListRequest(SINT32 connection, UINT32 crewID, const WEAPON_SERIAL& weaponSerial, const ChatHandle handle, bool join);

// This function sends a Garrison country change request to the strat system.
SINT32 strtSendGarrisonCountryChangeRequest(GarrisonCountryChangeReq& garrisonCountryChangeReq);
