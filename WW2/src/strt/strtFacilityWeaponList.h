#pragma once
// Date: Sat Apr 23 20:30:30 CDT 2005
//
// Class for exchanging Facility Weapon Lists
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

/*
Special cases:
 if ( status > FacilityWeaponList::_IVNALID && status < FacilityWeaponList::_NORMAL )
	you can spawn here if equipment is delivered manually
	but there is no [current] connection to any supply source

FacilityWeaponList::_NORMAL				Everything is OK
FacilityWeaponList::_NORMAL + supplyFacID.id != 0
										Everything is OK, supplied by <supplyFacID>

FacilityWeaponList::_CUT_OFF			"Facility is isolated from supply"
	- but you can still manually supply the facility,
		so unless everything is 0 you can still spawn there
FacilityWeaponList::_CUT_BY_FB			"Enemy firebase on supply link"
	- as per FacilityWeaponList::_CUT_OFF
FacilityWeaponList::_CUT_BY_MATE		We don't own the connecting depot
FacilityWeaponList::_CUT_BY_NO_LINKS	There are actually* no* links in the strat database
	- map edge case

Non-spawnable:
FacilityWeaponList::_CLOSED				"This facility is closed"
FacilityWeaponList::_NON_SPAWNABLE		Not a spawnable facility (e.g. city)
FacilityWeaponList::_ENEMY_OWNS			"This facility is owned by the enemy"
FacilityWeaponList::_WRONG_OWNER		Owned by an ally, but not your country
FacilityWeaponList::_WRONG_SUPPLY		Nobody can spawn here, same-side diff-country owner of supply source
FacilityWeaponList::_SUPPLY_CLOSED		"Supplying armybase is closed"
FacilityWeaponList::_NOT_LINKED			Supplying armybase does not match your country [shouldn't happen]
FacilityWeaponList::_EMPTY				Nothing here
FacilityWeaponList::_ENEMY_CONTROLS_CP	"Enemy controls this CP"
FacilityWeaponList::_BDE_DISABLED		Supplying HC Unit is disabled.
*/

#include "teul/teulPackers.h"
#include "strt/strtTypes.h"

class WeaponCounter;
class FacilityStruct;
class StratHCUnit;

#pragma pack(push, 1)
class FacilityWeaponList
{
public:
	enum REASON_CODE
	{
		_INVALID						// 0 is not valid
		// Spawnable but not supplied states
		, _CUT_OFF						// But still spawnable
		, _CUT_BY_FB						// Depot's supply is cut-off by enemy FB
		, _CUT_BY_NO_LINKS				// This depot has* no* strat links at all
		, _CUT_BY_MATE					// Don't own connecting depot
		, _SUPPLY_IN_TRANSIT				// Supply is in transit (may not be spawnable)
		// Spawnable state
		, _NORMAL							// Healthy list
		// Non-spawnable states
		, _ENEMY_OWNS						// Enemy now owns this facility
		, _ENEMY_OWNS_CP					// Enemy has ownership of the CP
		, _ENEMY_CONTROLS_CP				// Enemy has control of the CP
		, _CLOSED							// This facility is closed
		, _NOT_LINKED						// No link for supply
		, _EMPTY							// No equipment at this facility
		, _NON_SPAWNABLE					// Facility not spawnable
		, _SUPPLY_CLOSED					// Source facility closed
		, _WRONG_SUPPLY					// Wrong country supplying
		, _WRONG_OWNER					// Owned by an ally
		, _BDE_DISABLED					// Brigade is disabled by TOE.
	};

	FacilityWeaponList(UINT32 playerID = 0, const FULL_OBJ_ID& fullObjID = nullObjID, FacilityWeaponList::REASON_CODE status=FacilityWeaponList::_INVALID)
		: m_status(status)
		, m_playerID(playerID), m_fullObjID(fullObjID)
		, m_numWeapons(0)
		, m_refreshSeconds(30)
		{ }
	FacilityWeaponList(ACTION_OBJECT* const a) : m_status(FacilityWeaponList::_INVALID), m_numWeapons(0) { a->offset = packer(TEUL_UNPACK, a); }
	~FacilityWeaponList() {}

	bool isSpawnable() const { return (m_fullObjID.id > 0 && m_status > FacilityWeaponList::_INVALID && m_status <= FacilityWeaponList::_NORMAL); }

public:
	SINT32 packer(PackMode mode, ACTION_OBJECT* a);
	bool populate(const StratHCUnit& unit, const FacilityStruct& facility, UINT32 playerID, const PermissionMask& permMask);		// Host function
	SINT32 send(SINT32 teulIndex);

public:
	static const UINT32 _MAX_WEAPONS = 64;

public:
	REASON_CODE			m_status;			// Summary of the weapon list status
	UINT32				m_playerID;			// player to send info to
	FULL_OBJ_ID			m_fullObjID;		// FullObjID of the facility this list is for
	WEAPON_SPEC			m_weapons[_MAX_WEAPONS];
	WeaponCounter		m_counters[_MAX_WEAPONS];
	UCHAR				m_levels[_MAX_WEAPONS];
	UINT16				m_numWeapons;		// Numberof entries in counter
	UINT16				m_refreshSeconds;	// How long to wait before refreshing
	UINT16				m_nextSupplyM;		// ETA on next supply
};
#pragma pack(pop)
