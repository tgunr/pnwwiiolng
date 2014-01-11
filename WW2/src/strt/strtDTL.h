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

#include "wwii/wwiiTypes.h"
#include "wwii/wwiiFullObjID.h"

#if platOS == OS_MACOS
	#pragma options align=packed
#elif platOS == OS_WIN32
	#pragma pack(push, 1)
#elif platOS == OS_LINUX
	#pragma pack(push, 1)
#else
	#error No packing specified
#endif


//tolua_begin

#define MAX_STRT_NAME_LENGTH	64
#define MAX_OBJNAME_LENGTH		32

#define GLOBAL_FLAG_NEW			0x8000
#define GLOBAL_FLAG_CHANGED		0x4000

#define LINK_FLAG_SETOPEN		0x0001
#define LINK_FLAG_DISABLED		0x0002
#define LINK_FLAG_CONVOY		0x0004


// Enumerated types

enum CP_TYPE
{
	CP_GENERIC=1,
	CP_SMALL,
	CP_MEDIUM,
	CP_LARGE,
	CP_BRIDGE,
	// Add new CP types here
	MAX_CP_TYPES

};


enum FACILITY_TYPE
{
	FACILITY_CITY=1,
	FACILITY_FACTORY,
	FACILITY_BASE,
	FACILITY_DEPOT,
	FACILITY_OMT,				// Off Map Transfer (OMT)
	FACILITY_SHIPYARD,
	FACILITY_FIREBASE,
	FACILITY_AIRBASE,
	FACILITY_ARMYBASE,
	FACILITY_NAVALBASE,
	// Add new facility types here
	MAX_FACILITY_TYPES

};


enum FACTORY_SUBTYPE
{
	FACTORY_GENERIC = 1,
	FACTORY_MUNITIONS,
	FACTORY_AIRCRAFT,
	FACTORY_OIL_REFINERY,
	FACTORY_CHEMICAL_PLANT,
	FACTORY_MACHINERY,
	FACTORY_VEHICLE,
	// Add new facility subtypes here
	MAX_FACTORY_SUBTYPES

};


enum DEPOT_SUBTYPE
{
	DEPOT_GENERIC=1,
	DEPOT_TRAIN,
	DEPOT_TRUCK,
	DEPOT_BARGE,
	DEPOT_CONVOY,
	DEPOT_PACK_ANIMAL,
	DEPOT_WAGON,
	DEPOT_PORTER,
	// Add new depot subtypes here
	MAX_DEPOT_SUBTYPES

};


enum OMT_SUBTYPE
{
	OMT_GENERIC=1,
	OMT_RAIL,
	OMT_ROAD,
	OMT_SEA,
	// Add new facility omt subtypes here
	MAX_OMT_SUBTYPES

};


enum BUILDING_TYPE
{
	BUILDING_GENERIC=1,
	BUILDING_RESEARCH,
	BUILDING_SPAWN,
	BUILDING_AUTOGUNNER,
	BUILDING_CAPTURE,
	BUILDING_BRIDGE,
	// Add new building types here
	MAX_BUILDING_TYPES

};


enum AUTOGUNNER_SUBTYPE
{
	AUTOGUNNER_MGNEST=1,
	AUTOGUNNER_RIFLEMAN,
	AUTOGUNNER_RIFLEPIT,
	AUTOGUNNER_MORTARPIT,
	AUTOGUNNER_ARTILLERYLIGHT,
	AUTOGUNNER_ARTILLERYMEDIUM,
	AUTOGUNNER_ARTILLERYHEAVY,
	AUTOGUNNER_COASTDEFENSE,
	AUTOGUNNER_FORTCUPOUT,
	AUTOGUNNER_AAALIGHT,
	AUTOGUNNER_AAAMEDIUM,
	AUTOGUNNER_AAAHEAVY,
	AUTOGUNNER_TORPEDO,
	AUTOGUNNER_ATLIGHT,
	AUTOGUNNER_ATMEDIUM,
	AUTOGUNNER_ATHEAVY,
	// Add new autogunner subtypes here
	MAX_AUTOGUNNER_SUBTYPES

};

//tolua_end
enum WAYPOINT_TYPE
{
	WAYPOINT_NORMAL=1,
	WAYPOINT_BRIDGE=2,
	// Add new waypoint types here
	MAX_WAYPOINT_TYPES
};
//tolua_begin


// These appear to be flags for certain strat element classes.
// They may be changed to #defines as code solidifies.

// Flags
enum STRAT_FLAG
{
	STRAT_MODIFIED=1,
	STRAT_CONTROL=2,
	// Add new strat flags here
	MAX_STRAT_FLAGS

};

// Real data structures

struct STRT_DTL_HEADER
{
	UINT16 numCPs;
	UINT16 numFacilities;
	UINT16 numBuildings;
	UINT16 numLinks;
	UINT16 numWaypoints;
	UINT16 reserved1;
	UINT16 reserved2;
	UINT16 reserved3;

};

//tolua_end

struct CP
{
	REL_OBJ_ID	oid;				// object ID of this CP
	UINT32		serial;				// supercell relative serial number

	SINT32		posx;				// X Position in supercell (in meters)
	SINT32		posy;				// Y Position in supercell (in meters)

	SINT8		subType;			// Subtype of REL_OBJ_ID

	SINT8		side;				// Side of the CP
	SINT8		country;			// Country of the CP

	SINT8		stratFlags;			// general strat object flags

	SINT8		name[MAX_STRT_NAME_LENGTH];
	SINT8		fileName[MAX_OBJNAME_LENGTH];

};


struct FACILITY
{
	REL_OBJ_ID	oid;				// Facility ID of this facility
	UINT32		serial;				// supercell relative serial number

	FULL_OBJ_ID	cpid;				// Parent CP ID
	SINT32		posx;				// X Position in supercell (in meters)
	SINT32		posy;				// Y Position in supercell (in meters)

	SINT8		subType;			// Subtype of REL_OBJ_ID

	SINT8		originalSide;		// Side of the Facility
	SINT8		originalCountry;	// Country of the Facility

	SINT8		stratFlags;			// general strat object flags

	SINT8		name[MAX_STRT_NAME_LENGTH];
	SINT8		fileName[MAX_OBJNAME_LENGTH];

};

struct BUILDING
{
	REL_OBJ_ID	oid;				// object ID of this object
	UINT32		serial;				// supercell relative serial number

	REL_OBJ_ID	pid;				// Parent object ID
	SINT32		posx;				// X Position in supercell (in meters)
	SINT32		posy;				// Y Position in supercell (in meters)

	SINT8		subType;			// Subtype of REL_OBJ_ID

	SINT8		Xcontrol;			// Control building or not - only 1 building

	SINT8		stratFlags;			// general strat object flags

	SINT8		name[MAX_STRT_NAME_LENGTH];
	SINT8		fileName[MAX_OBJNAME_LENGTH];

};


struct DEPOT_LINK
{
	REL_OBJ_ID	oid;				// object ID of this link
	UINT32		serial;				// supercell relative serial number

	FULL_OBJ_ID	startDepotId;		// starting depot Building ID
	FULL_OBJ_ID	endDepotId;			// ending depot Building ID
	FULL_OBJ_ID firebaseId;			// optional firebase associated with this link

	SINT32		distance;			// Length of Link

	SINT8		stratFlags;			// general strat object flags

	SINT8		name[MAX_STRT_NAME_LENGTH];

};


struct WAYPOINT
{
	REL_OBJ_ID	oid;				// object ID of this waypoint
	UINT32		serial;				// supercell relative serial number

	REL_OBJ_ID	pid;				// Parent object ID (to bridge?) (object attachment)
	SINT32		posx;				// X Position in supercell (in meters)
	SINT32		posy;				// Y Position in supercell (in meters)

	SINT8		subType;			// Subtype of REL_OBJ_ID

	FULL_OBJ_ID	nextpt;				// WAY_POINT ID of next waypoint
	FULL_OBJ_ID	lid;				// Link ID

	SINT32		distance;			// distance from last waypoint over terrain.

	SINT8		stratFlags;			// general strat object flags

};

#if platOS == OS_MACOS
	#pragma options align=reset
#elif platOS == OS_WIN32
	#pragma pack(pop)
#elif platOS == OS_LINUX
	#pragma pack(pop)
#endif
