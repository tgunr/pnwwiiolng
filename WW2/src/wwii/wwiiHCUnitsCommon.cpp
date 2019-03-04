#include "syscfg.h" // Path is project specific.
// Created: Thu Nov  9 15:09:09 CST 2006
//
// Client/Host standard functions for HCUnits
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "strt/strtDTL.h"					// For FACILITY_TYPE enums
#include "teul/teulPackers.h"
#include "wwii/wwiiHCUnits.h"
#include "wwii/wwiiPack.h"
#include "str/stringLib.h"

// Map unit_id to a HCUnit
HCUNIT_INDEX hcUnits;

//// Short Name -> HCUnit mapping
using HCUNIT_HANDLES = std::unordered_map<ChatHandle, HCUnit*>;
static HCUNIT_HANDLES hcUnitsByHandle;

// Ownership of HCUnits
static std::map<UINT32, std::unique_ptr<HCUnit>> s_hcUnitPointers;

// Pack a brigade ID

SINT32
wwiiPackBrigadeID(PackMode mode, UCHAR* data, SINT32 offset, UINT32& brigadeID, SINT32 /*size*/)
{
	return teulPackUINT32(mode, data, offset, &brigadeID, 16);
}

//////////////////////////////////////////////////////////////////////
// Register a HCUnit

void HCUnit::addToRegistry()
{
	s_hcUnitPointers[m_unitID].reset(this);
	hcUnits.emplace(m_unitID, this);
	dbugLog(_FLI, "registered %u->%s", m_unitID, m_callsign);
	hcUnitsByHandle.emplace(m_handle, this);
}

//////////////////////////////////////////////////////////////////////

HCUnit::HCUnit(UINT32 withUnit, HCUNIT_TYPE withType, UINT32 withParent
			, WWII_COUNTRY withCountry, BRANCH_ENUM withBranch, BRANCH_ENUM withMovementType, UCHAR withUnitClass
			, const ChatHandle withHandle
			, const char* withName, const char* nick, UINT8 Rank
			, const FULL_OBJ_ID home
			, const ChatHandle withOwner, RANK_NO withOwnerRank
			)
	: m_unitID(withUnit), m_type(withType), m_parentID(withParent)
	, m_countryID(withCountry), m_branchID(withBranch), m_movementType(withMovementType)
	, m_handle(withHandle)
	, m_owner(withOwner)
	, m_homeCPID(nullObjID)
	, m_rank(Rank)
	, m_ownerRank(withOwnerRank)
	, m_unitClass(withUnitClass)
	, m_parent(nullptr)
	, m_registered(true)
	, m_training(false)
{
	if ( withName == nullptr )
		throw std::runtime_error("HCUnit with null name");			///Terminal

	strsafecpy(m_name, withName);
	strsafecpy(m_shortName, nick);
	chatGetCallsign(m_callsign, withHandle);
	lookupParent();

	if ( home.id != 0 )
		setHomeCPID(home);
}

	// Empty constructor
HCUnit::HCUnit(UINT32 UnitID, HCUNIT_TYPE Type)
	: m_unitID(UnitID), m_type(Type), m_parentID(0)
	, m_countryID(WWII_NO_COUNTRY), m_branchID(BRANCH_NOT_USED)
	, m_handle(0LL)
	, m_homeCPID(nullObjID)
	, m_rank(0)
	, m_ownerRank(0LL)
	, m_unitClass(0)
	, m_parent(nullptr)
	, m_registered(false)
	, m_training(false)
{
	m_callsign[0] = 'U';	// WTF? For consistency
	m_callsign[1] = 0;
	m_name[0] = 'U';
	m_name[1] = 0;
	m_shortName[0] = 'U';
	m_shortName[1] = 0;
}

HCUnit::HCUnit(ACTION_OBJECT* const a)
	: m_registered(false)
	, m_training(false)
{
	a->offset = packer(TEUL_UNPACK, a);
}

// Destructor

HCUnit::~HCUnit()
{
	if ( m_registered && m_unitID > 0 )
	{
		HCUNIT_INDEX::iterator it = hcUnits.find(m_unitID);
		if ( it != hcUnits.end() && it->second == this )
		{
			dbugLog(_FLI, "unregistering %u", m_unitID);
			hcUnits.erase(it);
		}
	}
	if ( m_registered && m_handle > 0 )
	{
		HCUNIT_HANDLES::iterator it = hcUnitsByHandle.find(m_handle);
		if ( it != hcUnitsByHandle.end() && it->second == this )
		{
			dbugLog(_FLI, "unregistering %u handle", m_unitID);
			hcUnitsByHandle.erase(it);
		}
	}
}

//////////////////////////////////////////////////////////////////////
// Test if this brigade is allow mission origins from a facility of a
// specified facility type

bool
HCUnit::canUseOrigin(int facilityType) const
{
	if ( isTrainingUnit() )
		return true;
	switch ( facilityType )
	{
	case FACILITY_AIRBASE:
		if ( movementType() != BRANCH_AIR )
			return false;
	break;
	case FACILITY_NAVALBASE:
		if ( movementType() != BRANCH_SEA )
			return false;
	break;
	case FACILITY_ARMYBASE:
		if ( movementType() != branch() )
			return false;
	break;
	case FACILITY_FIREBASE:
		if ( movementType() != BRANCH_LAND || isDivision() == true )
			return false;
		/* PASSTHRU */
	default:
		// Para brigades (land+air) can only use air facilities
		if ( movementType() == BRANCH_AIR )
			return false;
	break;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////
// HCUnit Packer

SINT32
HCUnit::packer(PackMode mode, ACTION_OBJECT* a)
{
	a->offset = wwiiPackBrigadeID(mode, a->packed, a->offset, m_unitID);
	a->offset = wwiiPackBrigadeID(mode, a->packed, a->offset, m_parentID);

	a->offset = teulPackUINT32(mode, a->packed, a->offset, (UINT32*)&m_type, 2);		//
	a->offset = teulPackUINT32(mode, a->packed, a->offset, (UINT32*)&m_countryID, 4);	// 8 bits
	a->offset = teulPackUINT32(mode, a->packed, a->offset, (UINT32*)&m_branchID, 2);	//

	a->offset = wwiiPackHandle(mode, a->packed, a->offset, &m_handle, 0);			// 48 bits

	if ( mode == TEUL_UNPACK )
		chatGetCallsign(m_callsign, m_handle);

	a->offset = teulPackUINT32(mode, a->packed, a->offset, (UINT32*)&m_movementType, 3);	// } 8 bits
	a->offset = teulPackUCHAR(mode, a->packed, a->offset, &m_unitClass, 5);				// }

	a->offset = wwiiPackRankNo(mode, a->packed, a->offset, &m_ownerRank, 0);				// 8 bits

	a->offset = wwiiPackHandle(mode, a->packed, a->offset, &m_owner, 0);
		// 48 bits

	a->offset = wwiiPackText(mode, a->packed, a->offset, m_name, MAX_HCUNIT_NAME_LENGTH);
	a->offset = wwiiPackText(mode, a->packed, a->offset, m_shortName, MAX_BRIGADE_SHORT_LENGTH);

	if ( mode == TEUL_UNPACK && m_parentID )
		lookupParent();

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Determine if a unit is ordered below the unit on the right hand side

bool
HCUnit::isBelow(UINT32 rhs) const
{
	if ( rhs == 0 || unitID() == rhs || m_parentID == 0 )
		return false;

	HCUnit* ancestor;
	for ( ancestor = m_parent; ancestor != nullptr; ancestor = ancestor->m_parent )
	{
		if ( ancestor->unitID() == rhs )
			return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////
// Find our parent

void
HCUnit::lookupParent()
{
	if ( !m_parentID || m_parentID == m_unitID )
		m_parent = nullptr;
	else
		m_parent = Lookup(m_parentID);
	m_training = ( isBrigade() && m_parent == nullptr && strncmp(m_callsign, "trng", 4) == 0 );
}

//////////////////////////////////////////////////////////////////////
// Look up a HCUnit by unitID

HCUnit*
HCUnit::Lookup(UINT32 unitID)
{
	HCUNIT_INDEX::iterator it = hcUnits.find(unitID);
	return (it != hcUnits.end()) ? it->second : nullptr;
}

//////////////////////////////////////////////////////////////////////
// Look up a HCUnit by its handle

HCUnit*
HCUnit::Lookup(const ChatHandle handle)
{
	HCUNIT_HANDLES::iterator it = hcUnitsByHandle.find(handle);
	if ( it == hcUnitsByHandle.end() )
		it = hcUnitsByHandle.find(chatGetLowercaseHandle(handle));
	return (it != hcUnitsByHandle.end()) ? it->second : nullptr;
}

//////////////////////////////////////////////////////////////////////
// HCUnitList network constructor

HCUnitList::HCUnitList(ACTION_OBJECT* const a)
{
	a->offset = packer(TEUL_UNPACK, a);
}

//////////////////////////////////////////////////////////////////////
// Packer for the list of HCUnits

SINT32
HCUnitList::packer(PackMode mode, ACTION_OBJECT* a)
{
	a->offset = teulPackUINT32(mode, a->packed, a->offset, &m_listStart, 1);	//
	a->offset = teulPackUINT32(mode, a->packed, a->offset, &m_listEnd, 1);	// 8-bits
	a->offset = teulPackUCHAR(mode, a->packed, a->offset, &m_count, 6);		//

	for ( UINT32 i = 0; i < m_count && i < MAX_HCUNITS_PER_LIST; ++i ) {
#if platBuild == BUILD_SERVER
		a->offset = m_units[i]->packer(mode, a);
#else
		a->offset = m_units[i].packer(mode, a);
#endif
	}

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Return the commanding unit for a particular country/branch

HCUnit*
HCUnit::GetCommandUnit(WWII_COUNTRY forCountry, BRANCH_ENUM forBranch)
{
	for ( auto it = hcUnits.begin(), end = hcUnits.end(); it != end; ++it )
	{
		if (it->second->parent() != nullptr)
			continue;
		// Ignore training units.
		if ( it->second->isTrainingUnit() == true )
			continue;
		if ( it->second->country() != forCountry || it->second->branch() != forBranch )
			continue;
		if ( it->second->isBrigade() || it->second->isDivision() || it->second->isGarrison() )
			continue;
		return it->second;
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////
// Return the training unit for a particular country/branch

HCUnit*
HCUnit::GetTrainingUnit(WWII_COUNTRY forCountry, BRANCH_ENUM forBranch)
{
	//  Returns:  a pointer to the Selected Training HCUnit  OR  nullptr.
	
	HCUnit* retVal = nullptr;

	for ( auto it = hcUnits.begin(), end = hcUnits.end(); it != end; ++it )
	{
		if (it->second->isTrainingUnit() == true)
		{
			if (it->second->country() == forCountry && it->second->branch() == forBranch)
			{
				retVal = it->second;
				break;
			}
		}
	}

	return nullptr;
}

void HCUnit::GetGarrisonUnits(std::list<HCUnit *>& unitsList, UINT16 cpID)
{
	//dbugLog(_FLN, "Performing HCUnit::GetGarrisonUnits");
	for (auto it = hcUnits.begin(), end = hcUnits.end(); it != end; ++it)
	{
		if (it->second->isGarrison())
		{
			if (it->second->homeCPID().id == cpID)
			{
				unitsList.push_back(it->second);
			}
		}
	}
}

UINT32 HCUnit::GetArmyGarrison(UINT16 cpID)
{
	// Need to look up primary brigade for the target CP
	UINT32 brigadeID = 0;
	std::list<HCUnit *> garrisons;
	HCUnit::GetGarrisonUnits(garrisons, cpID);
	for (HCUnit *unit : garrisons)
	{
		if (!unit->isMarineUnit() && !unit->isParatroopUnit())
		{
			brigadeID = unit->m_unitID;
			break;
		}
	}

	return brigadeID;
}
