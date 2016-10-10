#pragma once
// Date: Fri Apr 14 16:36:55 2000
//
// Header file for all the functions that manipulate a CP
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

#include "wwii/wwiiOwnershipEnums.h"

//	------------------------------	Non-exposed functions

class CPStruct;
class FacilityStruct;
class ObjectStruct;
struct CONVOYSTRUCT;
struct WEAPON_SERIAL;
struct FULL_OBJ_ID;
class HCUnit;

//This function initializes all the bases: ARMY, AIR, and NAVAL
extern bool strtInitBases(CPStruct* cpList);

//	------------------------------	Exposed functions
//tolua_begin

//This function actually runs the resource point allocation and
//	distribution sequences on one CP.  It takes care of producing
//	resources, doing repairs, and storing the leftovers (if any)
//	in the depots of a CP
extern void strtRunCP(CPStruct& cp);

//This function allows the Strat System to accept a convoy into its
//	depot
extern void strtAcceptConvoy(CONVOYSTRUCT& convoy);

//This function damages an object referenced by fullID with damageJoules
extern void strtDamage(const FULL_OBJ_ID& fullObjID, SINT64 damageJoules, const WEAPON_SERIAL& weapon, UINT32 playerID);

// Capture a facility and report it.
extern void strtCaptureAndLog(FacilityStruct* facility, const ChatHandle& handle, UINT32 customerID, UINT32 missionID, const HCUnit* unit, UINT32 seconds);

// Force an initial-timer based redraw of capture table for a facility
extern bool strtCaptureFacility(FacilityStruct& facility, WWII_COUNTRY taker, WWII_COUNTRY holder, bool force=false, bool quietly=false);

// Convert a facility only if it needs to be or if force is supplied
extern bool strtConvertFacility(FacilityStruct& facility, WWII_COUNTRY toCountry, bool force, bool quietly);

// Force rebuilding of the objects in a CP
extern void strtForceRebuildCP(CPStruct& cp);

extern bool strtProcessFirebase(FacilityStruct& firebase);

extern void strtCloseBase(FacilityStruct& base);
extern void strtOpenBase(FacilityStruct& base);

extern const std::string& strtListObjectives(WWII_SIDE, bool citysOnly);

extern void strtResetMonkeyHead(ObjectStruct& object, UINT32 vislevel);

extern void strtDestroyObject(ObjectStruct& object);

extern void strtAddFactory(ObjectStruct* object);

extern void strtCalculateRDPOutput();

extern void strtProcessFBRepairQueue();

extern FLOAT64 factoryDamageFactor[MAX_COUNTRIES];

//tolua_end
