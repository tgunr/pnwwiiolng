#include <syscfg.h> // Path is project specific.
//
// Created: Wed Dec 14 18:52:52 CST 2005
//
// Common member functions of strtFacilityStruct
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith
//


#include <assert.h>

#include "strt/strtCaptureState.h"
#include "strt/strtCPStruct.h"
#include "strt/strtFacilityStruct.h"
#include "strt/strtHostDB.h"
#include "strt/strtHostMessages.h"
#include "strt/strtJoinList.h"
#include "strt/strtLinkStruct.h"
#include "strt/strtWeaponEntry.h"
#include "weap/weapSpec.h"
#include "wwii/wwiiConfig.h"

FacilityStruct::~FacilityStruct()
{
	WEAPON_ENTRIES::iterator it;

	if ( m_joinList )
	{
		delete m_joinList;
		m_joinList = NULL;
	}

	for ( it = m_weaponEntries.begin(); it != m_weaponEntries.end(); ++it )
	{
		if ( it->second )
		{
			delete it->second;
			it->second = NULL;
		}
	}
}

//////////////////////////////////////////////////////////////////////
// Test if this facility is currently a candidate for capture states

bool
FacilityStruct::isCapturable() const
{
	if ( canBeCaptured() == false )
		return false;
	if ( CP().isObjective() == false )
		return false;
	if ( isMilitaryFacility() && CP().isContested() == false )
		return false;
	return true;
}

//////////////////////////////////////////////////////////////////////
// Get the HCUnit providing supply to this facility for a given unit

UINT32
FacilityStruct::getSupplyingUnit(const HCUnit* brigade, const CPStruct* brigadeHomeCP) const
{
	// Ensure facility side matches brigade; then we can shortcut to that.
	if ( side != brigade->side() )
		return 0;

	if ( cp->spawningEnabledFor(side) == false && type != FACILITY_DEPOT )
		return 0;

	// You can only use firebases when spawning your brigade or divisional equipment.
	if ( type == FACILITY_FIREBASE && (brigade->isBrigade() == false || brigade->movementType() != BRANCH_LAND) )
		return 0;

	// If the CP is here, then its definitely the supplying unit.
	if ( cp == brigadeHomeCP )
		return brigade->unitID();

	// If the division is in this town, then it is the supplying unit.
	if ( brigade->parent() && brigade->parent()->homeCPID() == cp->fullObjID )
		return brigade->parentID();

	if ( type == FACILITY_DEPOT && brigade->movementType() == BRANCH_LAND )
	{
		const FacilityStruct* supplyFac = supplyingFacility(*brigade, false, true);
		if ( supplyFac == NULL )
			return 0;
		if ( supplyFac->cp == brigadeHomeCP )
			return brigade->unitID();
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Return true/false whether this facility is an RDP factory

bool
FacilityStruct::isRDPFactory() const
{
	return type == FACILITY_FACTORY && cp && cp->m_isSupplyCenter;
}

//////////////////////////////////////////////////////////////////////
// Change the open state of a facility

void
FacilityStruct::setOpen(bool toOpen)
{
	UINT32 newFlags = flags & ~CP_STAT_OPEN;
	if ( toOpen )
		newFlags |= CP_STAT_OPEN;
	if ( open == toOpen && flags == newFlags )
		return;

	open = toOpen;
	flags = newFlags;

#ifdef IS_STRAT_HOST
	strtSaveFacility(this);
	strtQueueFacilityStatus(this);
#endif
}

//////////////////////////////////////////////////////////////////////
// Update the country/side of a facility and schedule a save/broadcast
// if this is an actual change

void
FacilityStruct::setCountry(WWII_COUNTRY newCountry)
{
	WWII_SIDE newSide = wwiiSide(newCountry);
	if ( country == newCountry && side == newSide )
		return;

	country = newCountry;
	side = newSide;

#ifdef IS_STRAT_HOST
	strtSaveFacility(this);
	strtQueueFacilityStatus(this);
#endif
}

//////////////////////////////////////////////////////////////////////
// Determine if this facility is allowed to follow its links

bool
FacilityStruct::canFollowSupplyLink() const
{
	if ( type != FACILITY_DEPOT || !host_config.supply.nomanDepots )
		return false;
	if ( cp->numUnits() > 0 )
		return false;
	return ( cp->isDefendObjective(side) );
}

//////////////////////////////////////////////////////////////////////
// Find the supply facility for a specified depot for a particular country

FacilityStruct*
FacilityStruct::supplyingFacility(const HCUnit& forUnit, bool ignoreDepot, bool followLinks) const
{
	switch ( type )
	{
	case FACILITY_ARMYBASE:
	case FACILITY_NAVALBASE:
	case FACILITY_AIRBASE:
		assert( armyBase != NULL );

		if ( armyBase->canABSupply(forUnit.side()) == false )
			return NULL;

		// Is the unit here?
		if ( forUnit.homeCPID() != cp->fullObjID )
			return NULL;

		return armyBase;
	break;

	case FACILITY_FIREBASE:
		assert( armyBase != NULL );

		if ( isOpen() == false )
			return NULL;

		if ( side != forUnit.side() )
			return NULL;

		return armyBase->supplyingFacility(forUnit, false);
	break;

	case FACILITY_DEPOT:
		assert( armyBase != NULL );

		if ( !ignoreDepot && (side != forUnit.side() || isOpen() == false) )
			return NULL;

		// Check local ABs if we own and control this town.
		if ( cp->side == forUnit.side() && cp->controllingSide == forUnit.side() )
		{
			FacilityStruct* const sourceAB = findLocalAB(forUnit.side());
			if ( sourceAB && this->canFollowSupplyLink() == false )
			{
				return sourceAB->supplyingFacility(forUnit, false, false);
			}
		}

		// Don't follow links if we have units here.
		if ( followLinks == false )
			return NULL;

		//// I haven't found an armybase here, so I need to follow the link
		//// that this depot sits on to see if I am connected to the
		//// location of my brigade/division.

		// Do we have links?
		if ( m_incomingLink == NULL )
			return NULL;

		if ( m_incomingLink->hasEnemyFirebase(forUnit.side()) )
			return NULL;

		if ( forUnit.homeCPID() != m_incomingLink->startDepot->cp->fullObjID )
			return NULL;

		return m_incomingLink->startDepot->supplyingFacility(forUnit, false, false);
	break;

	default:
		return NULL;
	break;
	}
}

//////////////////////////////////////////////////////////////////////
// Determine if the facility is an AB that can provide supply to the
// specified side.

bool
FacilityStruct::canABSupply(WWII_SIDE forSide) const
{
	return ( this != NULL && isOpen() == true && side == forSide && cp->side == forSide );
}

//////////////////////////////////////////////////////////////////////
// Disable capture for this facility.

void
FacilityStruct::setCaptureDisabled()
{
	if ( false == canBeCaptured() )
		return;

	_setCaptureState(_CAP_DISABLED, 0);
}

//////////////////////////////////////////////////////////////////////
// Start countdown to capturable timer for this facility.

void
FacilityStruct::setCapturePending()
{
	if ( false == this->isCapturable() )
		return this->setCaptureDisabled();
	if ( this->m_captureStage == _CAP_PENDING )
		return;

	// If capture is currently enabled, we will be using
	// a short rebuild time. Otherwise we will be using
	// the extended rebuild time.
	bool fromCaptureEnabled = this->isCaptureEnabled();
	UINT32 rebuildDelay_s = (fromCaptureEnabled ? host_config.capture.rebuildTime_s : (host_config.capture.start_m * 60));

	// Minimum 10s rebuild delay.
	static const UINT32 minimumDelay_s = 15;
	rebuildDelay_s = std::max(rebuildDelay_s, minimumDelay_s);

	_setCaptureState(_CAP_PENDING, rebuildDelay_s);
}

//////////////////////////////////////////////////////////////////////
// Mark this facility as available for capture.

void
FacilityStruct::setCaptureEnabled()
{
	if ( false == this->isCapturable() )
		return this->setCaptureDisabled();

	// Special case for military facilities.
	if ( this->isMilitaryFacility() )
	{
		if ( this->isCapturePending() == false )
			return this->setCapturePending();
	}

	_setCaptureState(_CAP_ENABLED, 0);
}

//////////////////////////////////////////////////////////////////////
// Return time or percentage capture remaining

UINT16
FacilityStruct::getCaptureRemaining() const
{
	switch ( m_captureStage )
	{
	case _CAP_DISABLED:
		return 100;
	break;
	case _CAP_PENDING:
		{
			UINT32 timeOut = m_captureStateChanged + m_captureCounter;
			return (UINT16)( timeOut > teulTime() ? timeOut - teulTime() : 0 );
		}
	break;
	case _CAP_ENABLED:
		return (UINT16)100 - std::min(m_captureCounter, (UINT16)100);
	break;
	default:
	break;
	}
	return 100;
}

//////////////////////////////////////////////////////////////////////
// Populate a capture state for the client

void
CaptureState::populate(const class FacilityStruct* facility
						, UINT32 forSide
						, SINT32 rate
						, bool spawnable
					)
{
	// Is this a capture?
	const bool enemyFacility = ((UINT32)forSide != (UINT32)facility->side);

	m_stage = (rate < 100 ? (Stage)facility->m_captureStage : CapCaptured);

	switch ( m_stage )
	{
	case CapPending:
		m_counter = facility->getCaptureRemaining();
	break;
	case CapEnabled:
		if ( enemyFacility )
			m_counter = facility->m_captureCounter;
		else
			m_counter = 100 - facility->m_captureCounter;
	break;
	default:
		m_counter = 0;
	break;
	}

	// If we own the facility, then rate is inverted.
	m_rate = (enemyFacility ? rate : -rate);

	m_spawnable = spawnable;

	m_fidKey = facility->m_captureBuilding->fullObjID.Key();
	m_forSide = forSide;
}

#ifdef IS_STRAT_HOST
//////////////////////////////////////////////////////////////////////
// Find the spawn entry for a weapon type for a given facility
// replaces strtFindWeaponEntry

WeaponEntry*
FacilityStruct::weaponEntry(const WEAPON_SPEC& weapon) const
{
	WEAPON_ENTRIES::const_iterator it = m_weaponEntries.find(weapon);
	return ( it != m_weaponEntries.end() ) ? (it->second) : NULL;
}

//////////////////////////////////////////////////////////////////////
// Add a spawn entry for a particular weapon through a particular
// building (object) to this facility. Creates weapon entries as
// needed and then adds the object to the weapon entry's portals
// list.

void
FacilityStruct::addWeaponSpawner(const WEAPON& weapon, const ObjectStruct& object)
{
	// Do we need to create a weapon entry for this
	WeaponEntry* entry = weaponEntry(weapon.spec());
	if ( entry != NULL )
		entry->addSpawner(object);
	else
	{
		// Need to create a weapon entry
		entry = new WeaponEntry(weapon, object);
		m_weaponEntries[weapon.spec()] = entry;
	}
}
#endif

