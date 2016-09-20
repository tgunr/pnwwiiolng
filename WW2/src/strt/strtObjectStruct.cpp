#include <syscfg.h> // Path is project specific.
// Created: Sat Dec 22 21:19:00 CST 2007
//
// ObjectStruct functions.
//
// Copyright (C) PlayNet, Inc 1999-2007 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "strt/strtFacilityStruct.h"		// Facility
#include "strt/strtHostDB.h"				// strtSaveObject
#include "strt/strtObjectStruct.h"			// ObjectStruct definition
#include "teul/teulTime.h"					// teulTime()
#include "wwii/wwiiConfig.h"

extern void strtQueueVisLevelUpdate(ObjectStruct* object);

//////////////////////////////////////////////////////////////////////
// Extend how long this building is going to take to repair

void
ObjectStruct::addRepairTime(UINT32 time)
{
	m_repairTime = teulTime() + (time * host_config.rpTime);
	// Does not get saved
}

//////////////////////////////////////////////////////////////////////
// Calculate the RDP bombing effect if this is a production factory

FLOAT64
ObjectStruct::factoryDamageFactor() const
{
	if ( !facility || facility->side != facility->originalSide || m_maxDamageJoules == 0 )
		return 1.0;
	return (FLOAT64) m_damageJoules / (FLOAT64) m_maxDamageJoules;
}

//////////////////////////////////////////////////////////////////////
// Recalculate the "capacity" for a particular object

void
ObjectStruct::recalculateCapacity()
{
	SINT32 newCapacity = 0;
	if ( m_maxCapacity > 0 )
	{
		if ( percentDamage == 0 )
			newCapacity = m_maxCapacity;
		else if ( percentDamage < 100 )
		{
			newCapacity = m_maxCapacity - (UINT32)((FLOAT32)m_maxCapacity * (FLOAT32)percentDamage / 100.0f);
			if ( newCapacity < 0 )
				newCapacity = 0;
		}
	}

	setCapacity((UINT32)newCapacity);
}

//////////////////////////////////////////////////////////////////////
// Strat definition is in strtHostDB.cpp

#ifndef IS_STRAT_HOST
bool ObjectStruct::save() { return true; }
#endif

//////////////////////////////////////////////////////////////////////
// Set the current damage capacity

void
ObjectStruct::setCapacity(UINT32 newCapacity)
{
	if ( newCapacity > m_maxCapacity )
		newCapacity = m_maxCapacity;

	if ( m_capacity == newCapacity && m_lastCapacity == m_capacity )
		return;

	m_lastCapacity = m_capacity;
	m_capacity = newCapacity;

	save();
}

//////////////////////////////////////////////////////////////////////
// Change the owner of a building

void
ObjectStruct::setCountry(WWII_COUNTRY newCountry, WWII_SIDE newSide)
{
	if ( newCountry == m_country && newSide == m_side )
		return;

	m_country = newCountry;
	m_side = newSide;

	save();
}

//////////////////////////////////////////////////////////////////////
// Change the number of joules damage taken by this building

bool
ObjectStruct::setDamageJoules(SINT64 newDamageJoules)
{
	// Validate the value
	if ( newDamageJoules > m_maxDamageJoules )
		newDamageJoules = m_maxDamageJoules;
	else if ( newDamageJoules < 0 )
		newDamageJoules = 0;

	if ( m_damageJoules == newDamageJoules )
		return false;

	m_damageJoules = newDamageJoules;

	save();

	return true;
}

//////////////////////////////////////////////////////////////////////
// Initialize the maximum and current capacities of an object

void
ObjectStruct::setMaxCapacity(UINT32 newMax)
{
	if ( newMax != m_maxCapacity )
	{
		m_maxCapacity = newMax;
		save();
	}

	if ( m_capacity != newMax || m_lastCapacity != newMax )
	{
		m_capacity = m_lastCapacity = newMax;
		save();
	}
}

//////////////////////////////////////////////////////////////////////
// Change the number of Repair Resource Points stocked at this bldg

void
ObjectStruct::setRepairRPs(UINT32 newRepairRPs)
{
	if ( m_repairRPs == newRepairRPs )
		return;
	m_repairRPs = newRepairRPs;
	save();
}

//////////////////////////////////////////////////////////////////////
// Force a capture table to dissapear and rebuild in the "initial draw"
// time taken from blitzWor.
// If 'shortRebuild' is set, then use the 'tableTime' value from host_config

void
ObjectStruct::setTableTimer(UCHAR inVisLevel, bool allowRebuild, bool shortRebuild)
{
	betaLog(_FLI, "setTableTimer(%s:%d -> %d) %s, %s"
			, name , visLevel()
			, inVisLevel
			, ((allowRebuild)?"RB":"NORB")
			, ((shortRebuild)?"short":"long")
			);

	if ( inVisLevel != visLevel() )
	{
		setVisLevel(inVisLevel);
		strtQueueVisLevelUpdate(this);
	}

	if ( allowRebuild )
	{
		// Set values that will cause the table to be rebuilt
		m_damageJoules = m_maxDamageJoules;
		// Choose between the tables innate rebuild and the
		// host_config "short" one used after a capture, e.g.
		if ( !shortRebuild )
			addRepairTime(rebuildPeriod);
		else
			addRepairTime((host_config.capture.rebuildTime_s + 59) / 60);
	}
	else
	{
		m_repairTime = 0;
		m_damageJoules = 0;
	}
}

//////////////////////////////////////////////////////////////////////
// Change the smoke level of a building

void
ObjectStruct::setSmokeLevel(UCHAR newLevel)
{
	// Nothing to do
	if ( m_smokeLevel == newLevel )
		return;

	// Only applies to some buildings
	if ( type != BUILDING_GENERIC && type != BUILDING_RESEARCH && type != BUILDING_SPAWN )
		return;

	m_smokeLevel = newLevel;

	m_lastSmokeTime = teulTime();

	strtQueueVisLevelUpdate(this);
}

//////////////////////////////////////////////////////////////////////
// Change the visLevel on a building, also clears the damagedBy field

void
ObjectStruct::setVisLevel(UCHAR newLevel)
{
	// No change to make
	if ( newLevel == m_visLevel )
		return;

	m_visLevel = newLevel;
	m_damagedBy = 0;

	strtQueueVisLevelUpdate(this);
}

//////////////////////////////////////////////////////////////////////
// Stubs for a non-strat host

#ifndef IS_STRAT_HOST

SINT32 runtime = 0;

void strtQueueVisLevelUpdate(ObjectStruct* /*obj*/)
{
}
#endif

