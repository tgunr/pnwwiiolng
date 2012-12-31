#pragma once
// Date: Fri Dec  2 18:39:55 CST 2005
//
// Availity record for a particular weapon type at a given facility;
// describes where the object will spawn at this facility, rules for
// access to the vehicle, and counters for availability and resupply.
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "strt/strtTypes.h"
#include <vector>
#include "strt/strtObjectStruct.h"
#include "weap/weapWeapon.h"

class FacilityStruct ;
class ObjectStruct ;
namespace TOE_
{
 class WeaponInstance ;
}

//tolua_begin

class WeaponEntry
{
public:
	WeaponCounter* getCounter(WeaponCounter* into, const TOE_::WeaponInstance* toeEntry) const ;

	// Determine if a weapon is currently spawnable at a facility
	// for a given mission ID
	bool isAccessible(const FacilityStruct& fac) const ;

	bool isPlayerWeapon() const { return (access() < SYSTEM_START_GRADE) ; }

	// Buzzard/skull/etc type vehicles
	bool isSystemWeapon() const { return (access() >= SYSTEM_START_GRADE) ; }

public:
	// Accessors
	SINT16 access() const { return m_weapon.access() ; }
	void addSpawner(const ObjectStruct& spawner) ;
	const char* fullname() const { return m_weapon.fullName ; }
	UINT32 localIndex() const { return m_weapon.localIndex() ; }
	bool isThrottled() const ;			 // If we are throttled and the CP isn't in hold at all costs
	const char* name() const { return m_weapon.name ; }
	const ObjectStruct& portals(UINT32 index) const { return *m_portals[index] ; }
	const FacilityStruct* facility() const { return m_portals[0]->facility ; }
	FacilityStruct* facility() { return m_portals[0]->facility ; }
	const WEAPON_SPEC& spec() const { return m_weapon.spec() ; }
	const WEAPON& weapon() const { return m_weapon ; }

public:
	WeaponEntry(const WEAPON& weapon, const ObjectStruct& spawner) ;

//tolua_end
public:
	typedef std::vector<const ObjectStruct*> PORTALS ;
	const PORTALS& portals() const { return m_portals ; }

protected:
	const WEAPON&	m_weapon ;			// Pointed to the weapon we represent
	PORTALS			m_portals ;			// List of spawn locations
//tolua_begin
} ;

//tolua_end
