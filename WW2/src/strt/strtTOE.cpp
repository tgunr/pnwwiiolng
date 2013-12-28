#include <syscfg.h> // Path is project specific.
//
// Created: Tue Jun 12 15:30:52 CDT 2007
//
// Routines for loading, managing and operating TOEs
//
// Copyright (C) PlayNet, Inc 1999-2007 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith
//

#include "dbug/dbug.h"							// For host "dbugLog" access

#include <assert.h>
#include <vector>
#include <math.h>

#include "chat/chatHandle.h"
#include "chat/chatName.h"
#include "strt/strtCPStruct.h"
#include "strt/strtTOE.h"
#include "strt/strtHCUnits.h"
#include "wwii/wwiiDBThreads.h"
#include "wwii/wwiiThreadText.h"
#include "wwii/wwiiConfig.h"
#include "include/mydbQuery.h"

extern DBHANDLE dbh;

// TOE templates.
TOE_TEMPLATES toeTemplates;

// TOE_::WeaponInstances that need saving and the original number of units
typedef std::vector< std::pair<TOE_::WeaponInstance*,UINT16> > TOE_SAVE_LIST;
TOE_SAVE_LIST toeSaveList;

//////////////////////////////////////////////////////////////////////
// Lookup a TOE_::TemplateHeader by ID or handle

TOE_::TemplateHeader*
TOE_::TemplateHeader::Lookup(const char* id)
{
	TOE_TEMPLATES::iterator it = toeTemplates.find(chatGetHandle(id));
	return (it != toeTemplates.end()) ? &it->second : NULL;
}

TOE_::TemplateHeader*
TOE_::TemplateHeader::Lookup(const ChatHandle& handle)
{
	TOE_TEMPLATES::iterator it = toeTemplates.find(handle);
	return (it != toeTemplates.end()) ? &it->second : NULL;
}

//////////////////////////////////////////////////////////////////////
// First time initialization of a TOE header

void
TOE_::TemplateHeader::initialize(const ChatHandle& withHandle, const char* withName, UINT32 withCountry, UINT32 withBranch)
{
	// If ID isn't null, we're already initialized
	assert( m_handle == 0LL );

	Chat::Name newName(withHandle);
	dbugLog(_FLI, "toe:%s(%llu) c:%u b:%u %s", newName.c_str(), withHandle, withCountry, withBranch, withName);

	m_handle = withHandle;
	m_name = withName;
	m_country = withCountry;
	m_branch = withBranch;
}

//////////////////////////////////////////////////////////////////////
// Incoming update of a TOE header

void
TOE_::TemplateHeader::setName(const char* newName)
{
	assert( newName != NULL );
	if ( name().compare(newName) == 0 )
		return;

	Chat::Name oldName(handle());
	dbugLog(_FLI, "toe:%s(%llu) new name: %s", oldName.c_str(), handle(), newName);

	m_name = newName;
}

//////////////////////////////////////////////////////////////////////
// Update or add a weapon entry and its capacity to this TOE

void
TOE_::TemplateHeader::updateWeapon(const WEAPON_SPEC& spec, UINT16 capacity, UCHAR access, UINT16 resupplyTime_m)
{
	TOE_WEAPONS::iterator it = m_weapons.lower_bound(spec);
	bool newWeapon = false;

	// Is this a new entry?
	if ( it == m_weapons.end() || it->first != spec )
	{
		Chat::Name toeName(handle());
		const WEAPON* weapon = weapLookup(spec);
		if ( weapon == NULL )
			LogAbort("toe:%s(%llu) wanted unknown weapon spec %s"
							, toeName.c_str(), handle(), weapPrintSpecC(&spec));

		dbugLog(_FLI, "toe:%s(%llu) adding %s(%s) capacity %u"
						, toeName.c_str(), handle()
						, weapon->name
						, weapPrintSpecC(&spec)
						, capacity
						);

		it = m_weapons.insert(it, std::make_pair(spec, TOE_::WeaponTemplate(*weapon)));
		newWeapon = true;
	}

	TOE_::WeaponTemplate& weaponTemplate = it->second;

	// Remember what capacity was
	UINT32 oldCapacity = weaponTemplate.capacity();

	// Update the fields
	weaponTemplate.setCapacity(capacity);
//	weaponTemplate.setAccess(access);
	weaponTemplate.setResupplyTime(resupplyTime_m);

	// Adjust our total capacity value
	if ( m_totalCapacity > oldCapacity )
		m_totalCapacity -= oldCapacity;
	else m_totalCapacity = 0;
	m_totalCapacity += capacity;

	// If we increased the capacity, we may need to provide additional vehicles
	if ( capacity > oldCapacity && m_subscribers.empty() == false )
	{
		UINT32 increase = capacity - oldCapacity;
		SUBSCRIBERS::iterator sit;

		for ( sit = m_subscribers.begin(); sit != m_subscribers.end(); ++sit )
		{
			StratHCUnit* unit = *sit;
			assert( unit != NULL );
			if ( newWeapon )
				unit->addTOEInstance(weaponTemplate);
			if ( (*sit)->offMap() || (*sit)->homeCP()->isSupplied() == false )
				continue;
			// Inject this many vehicles into the unit's spawn list,
			// false = don't allow overstocking.
			(*sit)->depositWeapons(spec, increase, false, NULL);
		}
	}
}

//////////////////////////////////////////////////////////////////////
// Register a StratHCUnit as a subscriber to this template.

void
TOE_::TemplateHeader::subscribe(StratHCUnit* unit, TOE& toe)
{
	assert( unit != NULL );
	assert( m_subscribers.find(unit) == m_subscribers.end() );
	assert( toe.empty() );

	m_subscribers.insert(unit);

	// Ensure that the HCUnit has all of the weapons in this template
	TOE_WEAPONS::const_iterator it;
	for ( it = m_weapons.begin(); it != m_weapons.end(); ++it )
	{
		unit->addTOEInstance(it->second);
	}
}

//////////////////////////////////////////////////////////////////////
// Delist an HCUnit from this template

void
TOE_::TemplateHeader::unsubscribe(StratHCUnit* unit, TOE& toe)
{
	assert( m_subscribers.find(unit) != m_subscribers.end() );

	m_subscribers.erase(unit);

	toe.clear();
}

//////////////////////////////////////////////////////////////////////
// Return either the custom resupply time or, if it is zero, return
// the system configuration value.

UINT32
TOE_::WeaponTemplate::resupplyTime() const
{
	return (UINT32)(( m_resupplyTime_m > 0 ) ? m_resupplyTime_m : host_config.supply.resupplyTime_m) * 60;
}

//////////////////////////////////////////////////////////////////////
// Return the maximum overstock ceiling for this vehicle

UINT32
TOE_::WeaponInstance::overstockCapacity() const
{
	if ( capacity() == 0 )
		return 0;

	// Get percentage expressed as, e.g. 110%
	UINT32 limit = (UINT32)(host_config.supply.overstockLimit + 100);
	if ( limit <= 100 )
		return (UINT32)capacity();

	// Get percentage as an actual number of units
	limit = (UINT32)ceilf((FLOAT32)(capacity() * limit) / 100.0f);
	if ( limit <= 0 )
		limit = 1;

	// Client only supports 3 digits, so do we (missionPack.cpp:WEAP_COUNT_MAX_XMIT)
	if ( limit > 999 )
		limit = 999;

	return limit;
}

//////////////////////////////////////////////////////////////////////
// Set the "present" level of a TOE Weapon Entry

bool TOE_::WeaponInstance::setPresent(UINT16 nowPresent)
{
	if ( nowPresent == present() )
		return false;

	// Queue this entry for saving.
	if ( !m_modified && weapon().isSupplied() && host_config.supply.enabled )
	{
		toeSaveList.push_back(std::make_pair(this, saveable()));
		m_modified = true;
	}

	m_present = nowPresent;

	return true;
}

//////////////////////////////////////////////////////////////////////
// Tell me how many of this vehicle type should be present through
// a throttled facility (e.g. a depot)

UINT16
TOE_::WeaponInstance::throttleCapacity() const
{
	return (UINT16)ceilf((float)capacity() * (float)host_config.throttle.percentage / 100.0f);
}

//////////////////////////////////////////////////////////////////////
// Create or update a TOE_::WeaponTemplate header entry

void
strtLoadTOEWeaponTemplateHeader(const char* id, const char* name, UINT32 country, UINT32 branch)
{
	assert( id != NULL );

	ChatHandle handle = chatGetHandle(id);
	if ( handle == 0LL )
		LogAbort("Illegal TOE Template %s", id);

	// Find or create an entry for this template
	TOE_::TemplateHeader& header = toeTemplates[handle];

	// Is it a new header?
	if ( header.handle() == 0LL )
		header.initialize(handle, name, country, branch);
	else
		header.setName(name);
}

//////////////////////////////////////////////////////////////////////
// Update a vehicle entry within a TOE template and all of its subscribers

void
strtLoadTOEWeaponInstance(const char* id, const WEAPON_SPEC& spec, UINT16 capacity, UCHAR access, UINT16 resupplyTime_m)
{
	assert( id != NULL );

	ChatHandle handle = chatGetHandle(id);
	if ( handle == 0LL )
		LogAbort("Illegal TOE Template %s", id);

	TOE_TEMPLATES::iterator it = toeTemplates.find(handle);
	if ( it == toeTemplates.end() )
		LogAbort("Got weapons for unrecognized TOE Template %s", id);

	TOE_::TemplateHeader& header = it->second;
	header.updateWeapon(spec, capacity, access, resupplyTime_m);
}

//////////////////////////////////////////////////////////////////////
// Generates the text for saving this entry to alloc

void
TOE_::WeaponInstance::saveText(UINT32& len, char* buffer) const
{
	// If we've filled the buffer,
	if ( len >= TEXT_POOL_SIZE - 30 )
	{
		DBSimpleQuery(dbh, buffer);
		len = 0;
	}
	const char* format;
	if ( len == 0  )
		format = "REPLACE INTO toe.supply(unit_id,vehtype_oid,present)VALUES(%u,%u,%u)";
	else
		format = ",(%u,%u,%u)";

	len += sprintf(buffer + len, format, m_unit.unitID(), weapon().localIndex(), saveable());
}

//////////////////////////////////////////////////////////////////////
// Save all modified TOEWeaponEntries. Must be called once before
// TOEWeaponEntries will begin saving.

void
strtSaveTOESupplies()
{
	if ( toeSaveList.empty() )
		return;

	// Get a big hunka-chunka-memory
	char buffer[TEXT_POOL_SIZE+1];

	UINT32 len = 0;
	TOE_SAVE_LIST::iterator it;
	for ( it = toeSaveList.begin(); it != toeSaveList.end(); ++it )
	{
		TOE_::WeaponInstance& entry = *it->first;

		// Ignore entries that have gotten reset since they were queued.
		if ( entry.saveable() != it->second )
			entry.saveText(len, buffer);

		entry.m_modified = false;
	}

	if ( len > 0 )
		DBSimpleQuery(dbh, buffer);

	toeSaveList.clear();
}

