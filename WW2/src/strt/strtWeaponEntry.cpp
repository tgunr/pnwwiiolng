#include <syscfg.h> // Path is project specific.
//
// Date: Wed Dec  7 21:37:13 CST 2005
//
// Member functions of the WeaponEntry struct
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith
//


#include <assert.h>
#include <algorithm>

#include "strt/strtProtos.h"
#include "strt/strtServerTypes.h"
#include "strt/strtWeaponEntry.h"
#include "strt/strtObjectStruct.h"
#include "strt/strtFacilityStruct.h"
#include "strt/strtCPStruct.h"
#include "wwii/wwiiConfig.h"
#include "strt/strtData.h"

WeaponEntry::WeaponEntry(const WEAPON& withWeapon, const ObjectStruct& withSpawner)
	: m_weapon(withWeapon)
	, m_portals()
{
	addSpawner(withSpawner);
}

void
WeaponEntry::addSpawner(const ObjectStruct& spawner)
{
	PORTALS::iterator it = std::find(m_portals.begin(), m_portals.end(), &spawner);
	if ( it == m_portals.end() )
		m_portals.push_back(&spawner);
}

// Determine if a vehicle is available at a specific facility; usually
// the facility will be our current facility
bool
WeaponEntry::isAccessible(const FacilityStruct& spawnFac) const
{
	return ( spawnFac.isOpen() == true && spawnFac.side == spec().side() );
}

//////////////////////////////////////////////////////////////////////
// Is an entry currently throttled?

bool
WeaponEntry::isThrottled() const
{
	if ( facility() == NULL || facility()->type != FACILITY_DEPOT )
		return false;

	if ( host_config.supply.enabled == false || host_config.throttle.enabled == false )
		return false;

	WWII_SIDE wSide = (WWII_SIDE)spec().side();

	assert( !m_portals.empty() );

	const CPStruct& cp = *facility()->cp;

	// If we are in charge of this CP and we have HAAC in effect, no throttling.
	if ( cp.spawningEnabledFor(wSide) && cp.isHolding(wSide) )
		return false;

	return true;
}
