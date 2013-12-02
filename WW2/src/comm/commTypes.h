#pragma once
//	This is where all types for universal game specific object structures
//	that will be transmited accross the network must reside or be
//	included.  All pack routines must have an entry for a reference
//	object in this file.
//
//	Copyright 1999 Playnet Inc, All rights reserved
//
//	Originating Programmer : Martin J. Poulin
//	Rev 0.1  11/08/99

#include "updt/updtEnums.h"
#include "wwii/wwiiExitCodes.h"

enum PLAYER_DESTINATION
{
	PDEST_MAP,
	PDEST_EXIT
};

#include "wwii/wwiiFullObjID.h"			// For FULL_OBJ_ID

struct PLAYER_EXIT
{
	PLAYER_DESTINATION destination;
	PLAYER_EXIT_CODE exitCode;
	FULL_OBJ_ID facility;		// Override facility
};

#include "weap/weapSerial.h"	// For WEAPON_SERIAL

struct VEHICLE_REQUEST
{
	SINT32			timeStamp;	//timestamp
	WEAPON_SERIAL	weapon;		// spec for this weapon so we can find it.
	UPDT_STATION	station; 	// station in the vehicle.
	UCHAR			side;		// Axis or Allied
	UCHAR			country;	// what country the persona belongs to
};

struct VEHICLE_REPLY
{
	UINT32			timeStamp;	//timestamp
	UINT32			hostIndex;	// host index of this vehicle.
	WEAPON_SPEC		spec;		// spec for this weapon so we can find it.
	FULL_OBJ_ID		source;		// Facility to spawn at.
	SINT32			replyCode;
	UINT16			rankLevel;
	UCHAR 			persVehicleIndex;
							// index of the vehicle in this persona
							// used to refereence this vehicle by persona
	UPDT_STATION	station;	// station in the vehicle.
//	LOAD_OUT_FLAGS	loadOutFlags;
};

struct VOID_STRUCT
{
	SINT32 size;
	void* data;
};
