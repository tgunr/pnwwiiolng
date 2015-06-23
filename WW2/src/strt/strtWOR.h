#pragma once
// Date: Thu Jan 09 15:50:16 2001
//
// These are all the data types that wll be utilized by the Strat System.
// All  common structures and enumerated types used between the terrain
// and host should be here so they can that impact the entire system.
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Marty "Vandal" Poulin

#include "strt/strtDTL.h"
#include "weap/weapSpec.h"

extern const char* facilityTypeName[]; // defined in weapList.c

struct CP_PARAMS
{
	SINT8		filename[MAX_OBJNAME_LENGTH];
	SINT32		ttp;			// Time to process (-1) (300 seconds)
	SINT8		stratFlags;	// general strat object flags
};

struct FACILITY_PARAMS
{
	SINT8		filename[MAX_OBJNAME_LENGTH];
	SINT32 		shippingThreshold;	//Threshold below which a depot won't ship out
	SINT32 		requestThreshold;	//Threshold below which a depot will request RPs
	SINT32		rptl;				// RP transfer limit (-1)
	SINT32		rput;				// RP unit time (-1)
	SINT8		multiplier;		// Multiplier against RP production (-1)

	SINT8		stratFlags;		// general strat object flags

};

struct SPAWN_ABILITY
{
	WEAPON_SPEC	weap;		// type should allways be 0
	SINT8		slots;		// number of vehicle slots for complex
	SINT8		points;
};

struct BUILDING_PARAMS
{
	SINT8		filename[MAX_OBJNAME_LENGTH];
	UINT64		maxdamage;	// Joules needed to destroy the building (from model)
	SINT32		rpsrepair;		// RPs needed to rebuild building (from model)
	SINT32		repairtime;	// Time to repair a building (between states) (seconds) (from model)
	SINT32		rpslevel;		// Maximum RPs that can be used at one time (from model)
	SINT32 		damageThreshold; // on FRONT_END only not used by host (minumum damage acepted)
	SINT8 		pRebuildlevel;	// parent rebuild level needed to start rebuilding
	SINT8		stratFlags;	// general strat object flags
	SPAWN_ABILITY spawn;		// what the spawn point can spawn
	SINT32		objectValue;	//The value of the object

};

struct DEPOT_LINK_PARAMS
{
	SINT8		filename[MAX_OBJNAME_LENGTH];
	SINT32		haulSpeed;			// speed of hauler
	SINT32		haulCapacity;		// capacity of hauler
	SINT32		minCapacity;		// min capacity of link
	SINT32		maxCapacity;		// max capacity of link
};
