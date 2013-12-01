#pragma once
// Date: Wed Dec  7 21:24:21 CST 2005
//
// Defines the ObjectStruct class
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "strt/strtProtos.h"
#include "strt/strtWOR.h"

struct SPAWN_ABILITY;

//tolua_begin

//The actual object struct
class ObjectStruct
{
public:
	ObjectStruct() { memset(this, 0, sizeof(*this)); }
	//// ---- Accessors
	//-- Read
	bool isValid() const { return NULL != facility; }
	const FacilityStruct& Facility() const { return *facility; }
	FacilityStruct& Facility() { return *facility; }

	UCHAR effectiveVisLevel() const { return (visLevel() == 0 || visLevel() == 255) ? visLevel() : visLevel() + smokeLevel(); }
	UCHAR smokeLevel() const { return m_smokeLevel; }
	UCHAR visLevel() const { return m_visLevel; }

	//-- Write

	// Change the vis level - note: also clears the damagedBy field.
	void setVisLevel(UCHAR newlevel);

	// Change what smoke level the building is showing.
	void setSmokeLevel(UCHAR newLevel);

	//// ---- Queries
	bool isProductionCenter() const;
	bool isCapturable() const { return m_isCaptureBuilding; }

	//// ---- Operations

	void applyDamage(const SINT64& joules, UINT32 originatingSortieID);

	// Calculate and cache the percentage damage
	UINT32 calcPercentDamage()
	{
		return percentDamage = (UINT32)((FLOAT64)100 * (FLOAT64)m_damageJoules / (FLOAT64)m_maxDamageJoules);
	}

	// Determine and switch to the appropriate damaged level
	// for this building along with various related state
	// settings
	void damagedVisLevel(UINT32 damagedBy);

	// Calculate the RDP damage factor
	FLOAT64 factoryDamageFactor() const;

	// Recalculate our "capacity"
	void recalculateCapacity();

	// Determine and switch to the appropriate repaired vis
	// level for this building in its current state.
	void repairVisLevel();

	// If we aren't already queued for saving, put us on the queue
	bool save();

	void setCapacity(UINT32 newCapacity);
	void setCountry(WWII_COUNTRY newCountry, WWII_SIDE newSide);
	bool setDamageJoules(SINT64 newDamageJoules);
	void setMaxCapacity(UINT32 newMax);
	void setRepairRPs(UINT32 newRepairRPs);
	void addRepairTime(UINT32 time);

	// Set the table timer and vis-level related values
	// for a table;
	void setTableTimer(UCHAR withVisLevel, bool allowRebuild, bool shortRebuild=false);

public:
//FULL_OBJ_ID must be the first field in the structure.
//DO NOT MOVE!!!!!!!!!! -Meat
	FULL_OBJ_ID		fullObjID;
	bool			modified;
	ObjectStruct* next;		// the next object belonging to this facility.
	FacilityStruct* facility;	//the facility to which it belongs

	SINT8			subType;
	FULL_OBJ_ID		facilityID;
	char			name[MAX_STRT_NAME_LENGTH];
	const char*		filename;
	WWII_SIDE		m_side;		//the side that owns the object
	WWII_COUNTRY	m_country;		//the country that owns the object
	WWII_SIDE		originalSide;
	WWII_COUNTRY	originalCountry;
	bool			control;	//Whether the object is a control object
	OBJECTTYPE		type;		//What type of object it is

	//Defined values
	SINT64			m_damageJoules;		//The damage the object has sustained
	SINT64			m_damageJoulesSaved;
	SINT64			m_maxDamageJoules;	//The max damage the object can sustain
	SINT64			m_maxDamageJoulesSaved;
	UINT32			totalRPs;			//The total # of RPs to rebuild this structure
	UINT32			rebuildPeriod;		//The minimum time in seconds to rebuild damage
	UINT32			rpsPerDamageLevel;	//The number of RPs per damage level

	//Derived values
	UINT32			percentDamage;		//The percent damage the object has sustained
	UINT32			m_capacity;			//The capacity the object is operating at
	UINT32			m_capacitySaved;
	UINT32			m_lastCapacity;		//The last capacity the object is operating at
	UINT32			m_lastCapacitySaved;
	UINT32			m_maxCapacity;		//The max capacity the object can operate at
	UINT32			m_maxCapacitySaved;

	UINT32			m_repairRPs;		//These RPs are the ones stored for repairs
										//on the object
	UINT32			m_repairRPsSaved;
	UINT32			m_repairTime;		//Time of the next repair
	UCHAR 			m_visLevel;			//Visual level of the building
	UCHAR 			m_smokeLevel;		// What level of smoke the building is producing
	UCHAR 			maxVislevel;		//Max Visual level of the building

	//For spawnpoints
	SPAWN_ABILITY	spawnAbility;

	struct BRIDGE_ENTRY*	linkList;
	SINT32			objectValue;		//The objectValue of a building
	SINT32			posX, posY;			// X/Y Position [in supercell?] (in meters)
	Supercell*		supercell;

	bool			m_needsSync;		// True when queued for broadcasting
	bool			m_isCaptureBuilding;

	UINT32			m_lastSmokeTime;	// Last time smoke changed state
	UINT32			m_uid;				// Unique ID from the database
	UINT32			m_damagedBy;		// SortieID of vehicle that put bldg into damaged vislevel
	UINT32			m_bid;				// Building ID within supercell.
};

//tolua_end
