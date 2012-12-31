#pragma once
// Date: Tue Dec 26 12:33:50 CST 2006
//
// Table and Order of Equipment Entries for an HCUnit
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "weap/weapSpec.h"
#include "weap/weapWeapon.h"
#include <map>
#include <set>
#include <vector>
#include <string>

class StratHCUnit ;
struct POSTED_WEAPON ;
namespace TOE_
{
	class TemplateHeader ;
	class WeaponTemplate ;
	class WeaponInstance ;
	class ResupplyTicket ;
}

// Template organization
typedef std::map<WEAPON_SPEC, TOE_::WeaponTemplate> TOE_WEAPONS ;
typedef std::map<ChatHandle, TOE_::TemplateHeader> TOE_TEMPLATES ;

extern TOE_TEMPLATES toeTemplates ;

// Per-brigade weapon lists
typedef std::map<WEAPON_SPEC, TOE_::WeaponInstance> TOE ;

extern void strtSaveTOESupplies() ;

//tolua_begin

namespace TOE_
{

class TemplateHeader
{
public:
	TemplateHeader() : m_handle(0LL), m_name("undef"), m_country(0), m_branch(0), m_weapons(), m_totalCapacity(0), m_subscribers() {}

	const ChatHandle& handle() const { return m_handle ; }
	const std::string& name() const { return m_name ; }
	UINT32 country() const { return m_country ; }
	UINT32 branch() const { return m_branch ; }
	const TOE_WEAPONS& weapons() const { return m_weapons ; }

	// Only call the first time you're setting up
	void initialize(const ChatHandle& handle, const char* name, UINT32 country, UINT32 branch) ;

	// Update the name of this template
	void setName(const char* name) ;

	// Add or update the capacity of a weapon -- updates all subscribers
	void updateWeapon(const WEAPON_SPEC& spec, UINT16 capacity, UCHAR access, UINT16 resupplyTime_m) ;

	// Subscribe to this Template and have it populate your TOE
	void subscribe(StratHCUnit* unit, TOE& toe) ;

	// Unsubscribe from this template - remember to clear your TOE!
	void unsubscribe(StratHCUnit* unit, TOE& toe) ;

	// Lookup functions
public:
	static TemplateHeader* Lookup(const char* id) ;
	static TemplateHeader* Lookup(const ChatHandle& handle) ;

//tolua_end
private:
	ChatHandle		m_handle ;					// packed short name of the template
	std::string		m_name ;					// Longer name of the template
	UINT32			m_country ;					// Country
	UINT32			m_branch ;					// Branch
	TOE_WEAPONS 	m_weapons ;					// Weapon templates
	UINT32			m_totalCapacity ;			// Sum of capacities

	// Which brigades/divisions are using us?
	typedef std::set<StratHCUnit*> SUBSCRIBERS ;
	SUBSCRIBERS		m_subscribers ;
//tolua_begin
} ;

// Defines the structure which provides the background information for a particular
// weapon within a given TOE Template
class WeaponTemplate
{
public:
	WeaponTemplate(const WEAPON& withWeapon) : m_weapon(withWeapon), m_capacity(0) {}

	bool operator<(const WEAPON_SPEC& rhs) const { return m_weapon.spec() < rhs ; }

	void setCapacity(UINT32 newCapacity) { m_capacity = (UINT16)newCapacity ; }
	void setResupplyTime(UINT16 resupplyTime_m) { if ( resupplyTime_m != m_resupplyTime_m ) m_resupplyTime_m = resupplyTime_m ; }
//	void setAccess(UCHAR newAccess) { m_access = newAccess ; }

	const WEAPON_SPEC& spec() const { return m_weapon.spec() ; }
	const WEAPON& weapon() const { return m_weapon ; }
	UINT16 capacity() const { return m_capacity ; }
	UCHAR access() const { return (UCHAR)m_weapon.access() ; }
	UINT32 resupplyTime() const ;

private:
//tolua_end
	const WEAPON&	m_weapon ;
	UINT16			m_capacity ;			// Automatic resupply level
	UINT16			m_resupplyTime_m ;		// Resupply Time (minutes) or 0 for default
//tolua_begin
} ;

// Provides a specific instance of a use of a WeaponTemplate
class WeaponInstance
{
public:
	WeaponInstance(const WeaponTemplate& withWeaponTemplate, StratHCUnit& withUnit, UINT16 withPresent)
		: m_weaponTemplate(withWeaponTemplate)
		, m_unit(withUnit)
		, m_present(withPresent)
		, m_resupply(0)
		, m_transfers(0)
		, m_modified(false)
		{ }

	bool operator<(const WeaponInstance& rhs) const { return spec() < rhs.spec() ; }
	bool operator<(const WeaponTemplate& rhs) const { return spec() < rhs.spec() ; }
	bool operator<(const WEAPON_SPEC& rhs) const { return spec() < rhs ; }

	UCHAR access() const { return weaponTemplate().access() ; }
	UINT16 capacity() const { return weaponTemplate().capacity() ; }
	UINT16 present() const { return m_present ; }
	UINT16 resupply() const { return m_resupply ; }
	UINT16 transfers() const { return m_transfers ; }
	UINT16 saveable() const { return m_present + m_transfers ; }
	UINT32 resupplyTime() const { return weaponTemplate().resupplyTime() ; }
	const WeaponTemplate& weaponTemplate() const { return m_weaponTemplate ; }

	const WEAPON_SPEC& spec() const { return weaponTemplate().spec() ; }
	const WEAPON& weapon() const { return weaponTemplate().weapon() ; }
	const StratHCUnit& unit() const { return m_unit ; }
	StratHCUnit& unit() { return m_unit ; }

	void adjustResupply(SINT32 adjustment, bool isTransfer)
	{
		SINT32 result = (SINT32)m_resupply + adjustment ;
		m_resupply = (result >= 0) ? (UINT16) result : 0 ;
		if ( isTransfer )
		{
			result = (SINT32)m_transfers + adjustment ;
			m_transfers = (result >= 0 ) ? (UINT16) result : 0 ;
		}
	}

	// Get the maximum we can overstock to
	UINT32 overstockCapacity() const ;

//	bool clear() { return setPresent(0) ; }
	bool setPresent(UINT16 present) ;

	// Create the text to save this entry into alloc, which is assumed
	// to be NULL or a pointer to a threadsAllocText allocated by
	// a previod call to saveText. When you are done calling this,
	// you should check if len > 0 and then either free the alloc
	// or queue it as a DBThreadQuery
	void saveText(UINT32& len, char* buffer) const ;

	// Tell me how many should be available through a throttled
	// facility (e.g. a depot)
	UINT16 throttleCapacity() const ;

private:
//tolua_end
	const WeaponTemplate& m_weaponTemplate ;	// Template we're based on
	StratHCUnit& m_unit ;						// StratHCUnit we belong to

	UINT16		m_present ;						// Number of units in-stock
	UINT16		m_resupply ;					// How many do we have scheduled for
												// replacement?
	UINT16		m_transfers ;					// How many are in transit?

	// This gets set true when this TOE is queued for saving
	bool		m_modified ;					// The value it was before modifications

	friend void ::strtSaveTOESupplies() ;
//tolua_begin
} ;

} // namespace TOE

//tolua_end
