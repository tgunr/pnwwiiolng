#pragma once
// Date: April 18 2000
//
// These are all the data types that wll be utilized by the Strat System.
// All structures and enumerated types should go here, as well as any defines
// that impact the entire system.
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Marty "Vandal" Poulin

#include <vector>
#include <string>
#include <map>
#include <set>

#include "strt/strtProtos.h"
#include "strt/strtWOR.h"
#include "sdm/sdmHash.h"
#include <time.h>
#include "strt/strtMapTypes.h"
#include "strt/strtChatTypes.h"

struct JOIN_LIST;

using CPSET = std::set<CPStruct*>;

//tolua_begin

// Maximum CPs in the strat system
#define		MAXCPS			1000

// Maximum Facilities in one CP
#define		MAXFACILITIES	10*MAXCPS


// Maximum objects in one CP
#define		MAXOBJECTS	10*MAXFACILITIES

//The maximum number of convoys the Convoy system will handle at one time
#define		MAXCONVOYS	10

//The time to sleep for the stratSystem, in seconds
#define		STRATSLEEP		1
#define 	STRAT_INTERVAL	5//150

//Default Value for cp Time To Produce
#define		STRTCP_DEFAULT_TTP	10//300
#define		STRTFACILITY_DEFAULT_RPTL 200
#define		STRTFACILITY_DEFAULT_RPUT 1000
#define		STRTFACILITY_DEFAULT_MULT	2
#define		STRTFACILITY_DEFAULT_THRESH	50000
#define		STRTFACILITY_DEFAULT_SHIPTHRESH	500
#define		STRTFACILITY_DEFAULT_REQTHRESH	20000
#define		STRTOBJECT_DEFAULT_MAXDAM	1000000
#define		STRTOBJECT_DEFAULT_RPREP	1000
#define		STRTOBJECT_DEFAULT_REPTIME	10//60
#define		STRTOBJECT_DEFAULT_RPLEVEL	500//50
#define		STRTOBJECT_DEFAULT_SPAWNGROUP	1
#define		STRTOBJECT_DEFAULT_SPAWNSLOTS	5
#define 	STRTLINK_DEFAULT_CAPACITY	1000
#define		STRTLINK_DEFAULT_LATENCY		15//480
#define		STRTLINK_DEFAULT_DISTANCE		5
#define		STRTLINK_DEFAULT_FLAGS	0x00000000

#define 	SPAWN_TIMEOUT_FULL 1800
#define 	SPAWN_TIMEOUT_CONTESTED 1800

#define 	FAC_OPEN_FLAG	0x80

//tolua_end

//Structure used to build a list of people who have requested
//	permission to join the posters vehicle
class WeapJoinRequestNode
{
	// Look for a specific node
	public: static WeapJoinRequestNode* Lookup(UINT32 playerID);

	// Ctor/Dtor
	public: WeapJoinRequestNode(UINT32 playerID, const ChatHandle handle, UINT32 missionID);
	public: ~WeapJoinRequestNode();

	// Add this node to a list - automatically extracts itself
	// from any list it is currently registered to.
	public: bool AddToList(JOIN_LIST* toList);

	private: void _register();
	private: void _unregister();

public:
	//CL linked list stuff
	WeapJoinRequestNode* prev;
	WeapJoinRequestNode* next;

	JOIN_LIST*	m_joinList;

	UINT32		m_playerID;
	UINT32		m_missionID;
	SINT32		m_capacityTeulIndex;
	ChatHandle	m_handle;
	MM_CAPACITY_REQUEST m_capacityRequest;
};

//The weapon structure
struct SPAWN_WEAPON
{
	//The weapon or loadout availability
	enum _ALLOCATION
	{
		_GENERAL,
		_LIMITED,
		_RARE,
		_MAX_ALLOCATION
	};

	SPAWN_WEAPON*		prev;	//Prev weapon in global list
	SPAWN_WEAPON*		next;	//Next weapon in global list
	WEAPON_SPEC			weapon;
	char				name[32];
	_ALLOCATION			allocation;  //Whether the weapon is common, rare, etc.
};

//This allows a bridge object to be associated with a link.  A list of these
//	are attached to the bridge in question, so it can close/open all the links
//	upon significant bridge events
struct BRIDGE_ENTRY
{
	BRIDGE_ENTRY* next;
	ObjectStruct* bridgeObject;
	LinkStruct* link;
};

//tolua_begin
// What smoke to aim for when real damage is done
#define SMOKE_START_LEVEL SMOKELEVEL_LIGHT

// What smoke to aim for when ongoing damage is done
#define SMOKE_LINGER_LEVEL SMOKELEVEL_LIGHT

// What's the damage required to "ping" the smoke level?
#define SMOKE_JOULES 1000
//tolua_end

class EWSReport
{
private:
	UINT32			m_weapon;			// Which vehicle is generating this
	UINT32			m_time;				// Time of report
	FacilityStruct*	m_facility;			// Location reported at
	UINT8			m_side;				// Side of report
	UINT8			m_weapCategory;		// Category of weapon in report

	void _incPresence();				// Increase presence counts
	void _decPresence();				// Decrease presence counts

public:
	// Constructor
	EWSReport(UINT32 weaponSerial, FacilityStruct* facility, UINT8 side, UINT8 category);
	// Destructor
	~EWSReport();

	bool hasExpired(UINT32 now) const;

	// Routine for updating an existing EWS report.
	void update(UINT32 now, FacilityStruct* curFacility);
};

//-------------------------MESSAGE SECTION-------------------------

struct CP_STATUS
{
	FULL_OBJ_ID			fullObjID;
	WWII_COUNTRY		country;
	WWII_COUNTRY		controllingCountry;
	CP_STATUS_FLAGS		flags;
};

