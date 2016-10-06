#pragma once
// Created: Fri May 11 19:19:08 CDT 2007
//
// Copyright (C) PlayNet, Inc 1999-2007 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "weap/weapDefs.h"
#include "weap/weapSpec.h"			// For WEAP_SPEC and NULL_WEAP_SPEC
#include "wwii/wwiiPermissions.h"

static const SINT16 SYSTEM_START_GRADE = 8;

//tolua_begin

struct WEAP_BIAS
{
	WEAPON_SPEC spec;
	UINT32		slots;
};

//
struct WEAPON
{
	WEAPON() : m_spec(NULL_WEAP_SPEC), m_modified(false) { name[0] = fullName[0] = 0; }
//tolua_end
	WEAPON(const WEAPON_SPEC spec, const char* Name, const char* access
				, UINT16 count, UINT32 lod, UINT32 AlertDist, UINT32 Speed, UINT32 CVCValue
				);
//tolua_begin
	~WEAPON();

public:
	UINT16 access() const { return m_access; }
	UINT16 count() const { return m_count; }
	UINT16 initialAccess() const { return m_initialAccess; }
	UINT32 localIndex() const { return m_localIndex; }
	UINT16 nextAccess() const { return m_nextAccess; }
	WEAPON_SPEC spec() const { return m_spec; }
	//WEAPON_SPEC& spec() { return m_spec; }

public:
	// HOST: Determine if a particular permission mask allows
	// access to this vehicle
	bool isPermitted(const PermissionMask& mask, bool disableRankLimits) const;

	// Weapons available to standard players
	bool isPlayerWeapon() const { return (access() < SYSTEM_START_GRADE); }

	bool isSupplied() const { return isSystemWeapon() == false; }

	bool isSupplied(const PermissionMask& mask) const
	{
		// If you have all spawn or this is a system weapon, it is not supplied
		return isSupplied() || (mask & ALL_SPAWN) != 0;
	}

	// Buzzard/skull/etc type vehicles
	bool isSystemWeapon() const { return (access() >= SYSTEM_START_GRADE); }

public:
	WEAPON_SPEC		m_spec;		// unique spec

	UINT16			m_access;		// 0 indicates general (associate with Rank)
	UINT16			m_initialAccess;	// Value we read from th_blitz
	UINT16			m_nextAccess;	// Value next rdp cycle
	UINT16			m_count;		// number of units available

	WEAP_BIAS		bias[MAX_BIAS_INDEX];		// Vis bias, allocated as loaded

	UINT32			m_friendlyBias;
	UINT32			m_enemyBias;

	UINT32			m_localIndex;	// local association for FRONT_END (set on fe only) to vehicle
	UINT32			LOD;			// max dispaly distance in cm
	UINT32			speed;		// speed...
	UINT32			value;
	UINT32			alertDist;	// distance at which we alert an enemy facility that we are there
	UINT64			maxLod_cm2;	// max vis distance squared
	UINT64			alertDist2;	// " squared

	char			name[MAX_WEAP_NAME_LEN];
	char			fullName[MAX_WEAP_FULL_NAME_LEN];

	bool			m_modified;
};

// find a WEAPON entry based on the weapon's name
extern WEAPON* weapLookupWeaponByName(const char* weaponName);

// Lookup the weapon with the fast index
extern WEAPON* weapLookup(const WEAPON_SPEC spec);

// Look up the weapon by the local index
extern WEAPON* weapLookupWeaponByIndex(UINT32 localIndex);

static inline const char* weapPrintName(const WEAPON_SPEC spec)
{
	const WEAPON* weap = weapLookup(spec);
	return ( weap == nullptr ) ? weapPrintSpecC(spec) : weap->name;
}

static inline const char* weapPrintFullName(const WEAPON_SPEC spec)
{
	const WEAPON* weap = weapLookup(spec);
	return ( weap == nullptr ) ? weapPrintSpecC(spec) : weap->fullName;
}

//tolua_end
