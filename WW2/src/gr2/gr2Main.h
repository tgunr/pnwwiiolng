#pragma once
//	 GR2 System Core
//	 Copyright 2001 Playnet Inc, All rights reserved
//	 Not to be distributed in API form except as part of a compiled library
//	 under License or NDA
//
//	Originating Programmer : Troy Lowe

#include "ml/mltypes.h"
#include "gr2/gr2Object.h"

class gr2Vehicle;
struct gr2Model;
struct vehVehicle;
struct OBJECT;
struct SHARED_VEHICLE_INFO;
struct VEHICLE_UPDATE_HEADER;

// init gr2 resources; granny models, sounds, etc...
LOGICAL gr2MainInit();

// updates all granny objects; sets up underlying vehicle as needed
LOGICAL gr2MainUpdate(FLOAT64 DeltaTime);

// kill all mech resources
LOGICAL gr2MainKill();

// init mech resources; granny models, sounds, etc...
LOGICAL gr2InitVehicle(vehVehicle* v);

// update the vehicle
LOGICAL gr2UpdateVehicle(vehVehicle* v, SINT32 TicTime);

// kill all vehicle resources
LOGICAL gr2KillVehicle(vehVehicle* v);

// create a new object; links up the vehicle and the gr2 class
OBJECT* gr2CreateVehicleObject(vehVehicle* v, UINT32 type);

// process incoming animation and damage state flags; 3rd person
void gr2UnpackFlags(vehVehicle* v, const VEHICLE_UPDATE_HEADER* header);

// send out animation and damage state flags
void gr2PackFlags(VEHICLE_UPDATE_HEADER* header, SHARED_VEHICLE_INFO* info);

// weapon collision call
LOGICAL gr2DoWeaponCollision(vehVehicle* v, piVECTOR* start, piVECTOR* end, SINT32* location, piVECTOR* impact);

// damage applied to me
UINT32 gr2ApplyDamage(UINT32 Location, FLOAT32 Damage);

// process incoming animation flags; 3rd person
UINT32 gr2ProcessControlInputs(SINT32 param, UINT32 id, UINT32 reserve);

// returns a gr2 vehicle by vehicle; can return null
gr2Vehicle* gr2GetVehicle(vehVehicle* v);

// returns a gr2 vehicle by game object; can return null
gr2Vehicle* gr2GetVehicleByObject(OBJECT* o);

// get my persona's gr2 vehicle
gr2Vehicle* gr2GetPersonaVehicle();

// return true is this vehicle is a gr2 vehicle
LOGICAL gr2IsAnimVehicle(vehVehicle* v);

// return true is this vehicle is a gr2 vehicle
LOGICAL gr2IsValidVehicle(gr2Vehicle* v);

// called by the camera system to get
// an eye point for the gr2 vehicle
LOGICAL gr2GetEyePoint(piMATRIX* m);

// create a new object; links up the vehicle and the gr2 class
void gr2RegisterVehicleCreator();

