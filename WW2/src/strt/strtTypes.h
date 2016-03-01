#pragma once
// Date: Thu Mar 30 15:50:16 2000
//
// These are all the data types that wll be utilized by the Strat System.
// All structures and enumerated types should go here, as well as any defines
// that impact the entire system.
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

#include "weap/weapSerial.h"
#include "wwii/wwiiFullObjID.h"			// For FULL_OBJ_ID

//tolua_begin

//------------------------------DEFINE SECTION-------------------------------

enum { FB_CLOSED_VISLEVEL = 255 };

// Bridges use their country to declare damaged state
enum
{
	BRIDGE_BUILT_COUNTRY = 1,
	BRIDGE_DESTROYED_COUNTRY = 2,
	BRIDGE_SIDE = 1
};

//------------------------------ENUM SECTION---------------------------------

enum SPAWN_DENY_REASONCODE
{
	SPAWN_GRANTED = -1,
	SPAWN_FACILITY_NOT_FOUND,				// Invalid origin
	SPAWN_FACILITY_CLOSED,					// This facility has closed
	SPAWN_WEAPON_NOT_AT_FACILITY,			// Current vehicle not valid at this facility
	SPAWN_COMPLEX_DISABLED,					// Invalid/expired mission [yeah, makes no sense]
	SPAWN_DISABLED_BY_GM,					// Arena is locked
	SPAWN_MOBILE_UNAVAILABLE,				// Mobile spawn is no-longer available (and you asked for it)
	SPAWN_DISABLED_BY_SERVER,				// No servers available for spawning
	SPAWN_LOST_CREW,						// Crew bailed
	SPAWN_NO_BRIGADE_SUPPLY,				// Brigade/division not here
	SPAWN_RESERVATION_CANCELLED,			// Reservation has expired
	SPAWN_DELAYED,							// Denied by launch timer
	SPAWN_ACCOUNT_MINUTES,					// Not enough minutes on your account
	SPAWN_ACCOUNT_MASK,						// Your account doesn't have access to this vehicle.
	SPAWN_ERROR								// Unknown error.
};

// Object visibility flags

enum {
	RADIO_CAPPABLE = 0
,	RADIO_TABLE = 1
,	RADIO_HIDDEN = 255
};

// visLevels
enum {
	VISLEVEL_NORMAL = 0
,	VISLEVEL_DAMAGED = 1
,	VISLEVEL_HIDDEN = 255
};

// smokeLevels
// visLevel + smokeLevels are combined to produce effective visLevel
// but only when visLevel is VISLEVEL_DAMAGED
enum {
	SMOKELEVEL_OFF = 0
,	SMOKELEVEL_LIGHT = 1
,	SMOKELEVEL_HEAVY = 2
,	SMOKELEVEL_FIRE = 3
};

//------------------------------STRUCT SECTION-------------------------------

//-------------------------MESSAGE SECTION-------------------------

struct CP_OBJ_HIT
{
	FULL_OBJ_ID		object;
	SINT64			damage;
};

struct CP_HIT_HEADER
{
	SINT32			numHits;
	CP_OBJ_HIT*		list;
};

//This message goes to the game host to apprise it of an ownership change
//	for an object.
struct OWNERSHIP_UPDATE
{
	FULL_OBJ_ID		objectID;
	WWII_COUNTRY	country;
	UINT32			flags;
};

//this message goes to the game host to tell the front ends to update their
//	visualization of a certain object
struct VISLEVEL_UPDATE
{
	FULL_OBJ_ID		objectID;
	UCHAR			vislevel;
};

// Structure for defining allocations of vehicles
class WeaponCounter
{
public:
	WeaponCounter() : capacity(0), current(0), reinforcements(0), spawnable(0), restricted(0) {}
    WeaponCounter (const WeaponCounter & rhs) { Copy(rhs); }

public:
	// add LOCKED value to this enum for client state notification of subscription content locking
	enum { SPAWNABLE, NOT_DEPLOYED, NOT_PRESENT, NOT_PERMITTED }; //LOCKED
	void clear() { capacity = current = reinforcements = 0; spawnable = NOT_PERMITTED; }
	void setCapacity(UINT16 newCapacity) { capacity = newCapacity; }
	void setCurrent(UINT16 newCurrent) { current = newCurrent; }

//tolua_end
	WeaponCounter& operator = (const WeaponCounter& rhs)
	{
        Copy(rhs);
		return *this;
	}
    
//tolua_begin

private:
    void Copy( const WeaponCounter & rhs ) {
        capacity = rhs.capacity;
        current = rhs.current;
        reinforcements = rhs.reinforcements;
        spawnable = rhs.spawnable;
        restricted = rhs.restricted;
    }
    
public:
	UINT16	capacity;				// Automatic resupply level
	UINT16	current;				// Current actual level
	UINT16	reinforcements;		// Number of pending reinforcements
	UCHAR	spawnable;				// Used by the client
	UCHAR	restricted;			// If your account prevents using this
};

#pragma pack(push, 1)				// Wire format capable.
struct STRAT_ENEMY_REPORT
{
	FULL_OBJ_ID		reportingFacility;
	WEAPON_SERIAL	enemyReported;
	UINT32			distance;
	UINT32			heading;
	UINT32			altitude;
	UINT32			speed;
};
#pragma pack(pop)

//tolua_end
