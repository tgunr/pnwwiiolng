#pragma once
// Created: Mon Mar  7 11:59:10 CST 2005
//
// Base unit of all HC units, the HCUnit, and the
// HCUnitList structure for exchanging with the host,
// and the HCUNIT_INDEX mapping for looking up a unit
//
// Copyright (C) PlayNet, Inc 2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include <map>

#include "chat/chatHandle.h"
#include "mm/mmEnums.h"
#include "wwii/wwiiFullObjID.h"
#include "wwii/wwiiHCUnitsIndex.h"
#include "wwii/wwiiOwnershipEnums.h"

//tolua_begin
enum HCUNIT_TYPE
{
	HCU_GENERIC = 0				// Just part o'the structure
,	HCU_BRIGADE = 1				// A brigade
,	HCU_DIVISION = 2			// A division
,	HCU_GARRISON = 3
,	MAX_HCUNIT_TYPES
};

// Map unit_id to a HCUnit
extern HCUNIT_INDEX hcUnits;

//tolua_end

#define MAX_BRIGADE_SHORT_LENGTH 25
#define MAX_HCUNIT_NAME_LENGTH 60
#define MAX_HCUNIT_NAME_SIZE (MAX_HCUNIT_NAME_LENGTH+1)

//tolua_begin

// Underlying data about a particular unit
class HCUnit
{
//tolua_end
public:
	//// ---- Class Operators ---- ////
	// Create from scratch
	HCUnit(UINT32 unit, HCUNIT_TYPE type, UINT32 parent
			, WWII_COUNTRY country, BRANCH_ENUM branch, BRANCH_ENUM movementType, UCHAR unitClass
			, const ChatHandle handle, const char* name, const char* nick, UINT8 rank
			, const FULL_OBJ_ID home=nullObjID
			, const ChatHandle owner=0LL, RANK_NO ownerRank=0
			);

	// Empty constructor
	HCUnit(UINT32 unitID=0, HCUNIT_TYPE type=HCU_GENERIC);

	//// Client only functions

	// Create from network object
	HCUnit(ACTION_OBJECT* const a);

	void addToRegistry();

public:
	// Get the homeCPID by unpacking from an action object
	void setHomeCPID(const FULL_OBJ_ID newHomeCPID);
	SINT32 packer(PackMode mode, ACTION_OBJECT* a);
//tolua_begin

	// Destructor -- remove me from the hcUnits table if I'm not a clone
	virtual ~HCUnit();

	////
	// Accessors
	UINT32 unitID() const { return m_unitID; }
	UINT32 parentID() const { return m_parentID; }
	HCUnit* parent() const { return m_parent; }
	WWII_COUNTRY country() const { return m_countryID; }
	WWII_SIDE side() const { return ( m_countryID == 4 ) ? AXIS : ALLIED; }
	BRANCH_ENUM branch() const { return m_branchID; }
	FULL_OBJ_ID homeCPID() const { return m_homeCPID; }
	BRANCH_ENUM movementType() const { return m_movementType; }
	const char* name() const { return m_name; }
	ChatHandle owner() const { return m_owner; }
	RANK_NO ownerRank() const { return m_ownerRank; }
	UINT8 rank() const { return m_rank; }
	HCUNIT_TYPE type() const { return m_type; }
	UCHAR unitClass() const { return m_unitClass; }
	ChatHandle handle() const { return m_handle; }
	const char* callsign() const { return m_callsign; }
	const char* shortName() const { return m_shortName; }

	////
	// Member functions

	void getDescriptor(std::string& into) const
	{
		into = m_name;
#if defined(DEV_BUILD) || platBuild == BUILD_SERVER
		into += " (";
		into += m_callsign;
		into += ")";
#endif
	}

	// Test if this brigade is allow mission origins from a specified
	// type of facility
	bool canUseOrigin(int facilityType) const;

	// Is this object below the right hand side object
	bool isAbove(const HCUnit* rhs) const { return (rhs && rhs->isBelow(unitID())); }
	bool isAbove(UINT32 rhs) const { return isAbove(Lookup(rhs)); }
	bool isBelow(UINT32 rhs) const;
	bool isBelow(const HCUnit* rhs) const { return rhs && isBelow(rhs->unitID()); }

	bool isBelowOrEqual(UINT32 rhs) const { return unitID() == rhs || isBelow(rhs); }
	bool isBelowOrEqual(const HCUnit* rhs) const { return rhs && ((rhs == this) || isBelow(rhs)); }

	bool isBrigade() const { return m_type == HCU_BRIGADE; }
	bool isDivision() const { return m_type == HCU_DIVISION; }
	bool isGarrison() const { return m_type == HCU_GARRISON;  }
	bool isDisabled() const { return m_disabled; }

	bool isMarineUnit() const { return m_branchID == BRANCH_LAND && m_movementType == BRANCH_SEA; }

	bool isOwner(const ChatHandle candidate) const { return (chatGetLowercaseHandle(candidate) == chatGetLowercaseHandle(m_owner)); }

	bool isParatroopUnit() const { return m_branchID == BRANCH_LAND && m_movementType == BRANCH_AIR; }

	bool isTrainingUnit() const { return m_training; }

	void setCountry(WWII_COUNTRY newCountry) { m_countryID = newCountry;}

	void setDisabled(bool disabled) { m_disabled = disabled; }

	////
	// Static functions

	// Find a HCUnit by its unitID
	static HCUnit* Lookup(UINT32 unitID);
	// Find a HCUnit by its 64bit packed callsign
	static HCUnit* Lookup(const ChatHandle handle);
	// Find a HCUnit by its short name
	static HCUnit* Lookup(const char* callsign)
		{ return Lookup(chatGetLowercaseHandle(callsign)); }

	// Get the command unit for a particular country/branch
	static HCUnit* GetCommandUnit(WWII_COUNTRY forCountry, BRANCH_ENUM forBranch);

	// Get the training unit for a particular country/branch
	static HCUnit* GetTrainingUnit(WWII_COUNTRY forCountry, BRANCH_ENUM forBranch);

	// Get a list of all the Garrisons for a particular Country/CP
	static void GetGarrisonUnits(std::list<HCUnit> &unitsList, UINT16 cpID);


	//// ---- Member Functions ---- ////

	//// ---- Class Variables ---- ////

public:
	UINT32			m_unitID;			//!< Non-sequential IDs from the host database
	HCUNIT_TYPE		m_type;			//!< What type of structure it is
	UINT32			m_parentID;		//!< unitID of the parent
	WWII_COUNTRY	m_countryID;		//!< which country
	BRANCH_ENUM		m_branchID;		//!< which branch
	BRANCH_ENUM		m_movementType;	//!< Which branch movement behavior we observe
	ChatHandle		m_handle;			//!< Packed short name
	ChatHandle		m_owner;			//!< chatHandle of the owner
	FULL_OBJ_ID		m_homeCPID;		//!< Home of the brigade
	UINT8			m_rank;			//!< Ramp's "rank" value
	UCHAR			m_ownerRank;		//!< The RANK of the owner, e.g. lt col
	UCHAR			m_unitClass;		//!< Unit class (infantry, paratroop, naval, etc)
	char			m_callsign[MAX_HANDLE_SIZE];	//!< Unpacked short name, SIZE includes null byte
	char			m_name[MAX_HCUNIT_NAME_SIZE];	//!< Full name of the unit
	char			m_shortName[MAX_BRIGADE_SHORT_LENGTH+1];

//tolua_end
protected:
	HCUnit*			m_parent;			// So we can climb the tree
	bool			m_registered;		// Whether we are the registered instance
	bool			m_training;
	bool            m_disabled;

private:
	void lookupParent();
//tolua_begin
};

//tolua_end

// Wrapper structure for the purposes of transport

// Maximum number of units that can be sent in each packet
static const UINT32 MAX_HCUNITS_PER_LIST = 14;

// List of HCUnits
class HCUnitList
{
public:
	HCUnitList() : m_listStart(true), m_listEnd(true), m_count(0) { }
	HCUnitList(ACTION_OBJECT* const a);
	~HCUnitList() { }
	SINT32 packer(PackMode mode, ACTION_OBJECT* a);

	// Send/Receive functions (defined as needed)
	// Returns: error < 0  /  ok >= 0
	SINT32 send(SINT32 teulIndex);
	SINT32 receive(SINT32 teulIndex);

	LOGICAL			m_listStart;						// True if this is the start of list
	LOGICAL			m_listEnd;							// True if this is the end of the list
	UCHAR			m_count;							// Number of units described
#if platBuild == BUILD_SERVER
	HCUnit*			m_units[MAX_HCUNITS_PER_LIST];		// Pointers to units
#else
	HCUnit			m_units[MAX_HCUNITS_PER_LIST];		// Array of units
#endif
};

#if platBuild == BUILD_SERVER
extern bool wwiiReadHCUnits();
#endif
