#include <syscfg.h> // Path is project specific.
// Created: Thu Apr 12 19:30:21 2001
//
// Management for lists of posted weapons (weapons which have
// been assigned a serial number and are curently in play or
// spawned)
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson
// Supporting Programmer:  Eric "Meat" Hall

#include "versionNo.h"

#include <list>
#include <set>
#include <algorithm>
#include <sys/stat.h>
#include <fcntl.h>

#include "chat/chatLStringIDs.h"
#include "chat/chatSupportRPCMessages.h"
#include "cnvy/cnvyMain.h"
#include "mem/mem.h"
#include "strt/strtCPStruct.h"
#include "strt/strtDTL.h"
#include "strt/strtHCUnits.h"
#include "strt/strtHostLua.h"
#include "strt/strtPostedWeaponList.h"
#include "strt/strtResupplyTicket.h"
#include "strt/strtSpawn.h"
#include "strt/strtTypes.h"
#include "strt/strtWeaponEntry.h"
#include "teul/teulTime.h"
#include "wwii/wwiiConfig.h"
#include "wwii/wwiiSerialNumber.h"

static SerialNumber serialCounter("weaponserial", 1, 250, 0x3fffffff);

typedef std::map<UINT32, POSTED_WEAPON*> POSTED_WEAPONS;
static POSTED_WEAPONS postedWeapons;

//////////////////////////////////////////////////////////////////////
// Helper to the constructor

void
POSTED_WEAPON::_register()
{
	m_weaponSerial.serial = static_cast<UINT32>(serialCounter.getNext());
	if ( m_weaponSerial.serial == 0 )
		LogAbort("Got zero for posted weapon serial number");
	if ( strtFindPostedWeapon(m_weaponSerial.serial) != nullptr )
		LogAbort("v:%u already in use", m_weaponSerial.serial);

	postedWeapons[m_weaponSerial.serial] = this;
}

//////////////////////////////////////////////////////////////////////
// Helper for the destructor and for pre-reserve.

void
POSTED_WEAPON::_unregister()
{
	POSTED_WEAPONS::iterator it = postedWeapons.find(serial());
	if ( it == postedWeapons.end() )
		LogAbort("v:%u attempt to free node %p twice!!", serial(), this);

	if ( it->second != this )
		LogAbort("v:%u trying to release wrong instance (%p/%p)", serial(), it->second, this);

	postedWeapons.erase(it);
}

//////////////////////////////////////////////////////////////////////
// Hack for assert

#ifndef NDEBUG
static bool
_cantFindPost(POSTED_WEAPON* posting, const POSTED_WEAPON_LIST& list)
{
	return std::find(list.begin(), list.end(), posting) == list.end();
}
#endif

//////////////////////////////////////////////////////////////////////
// Destructor, deregisters this posted weapon

POSTED_WEAPON::~POSTED_WEAPON()
{
	assert( m_status == _FREE || m_status == _CANCELLED );
	assert( _cantFindPost(this, m_supplyingBrigade->postedWeaponList()) == true );

	dbugLog(_FLD,"v:%u freeing posted weapon %p", serial(), this);

	_unregister();

	if ( m_joinList.empty() == false || m_crewList.empty() == false )
		LogAbort("v:%u attempt to free in use node %p, jList->head %p jList->type %d, cList->head %p cList->type %d"
					, serial(), this
					, m_joinList.front(), m_joinList.m_type
					, m_crewList.front(), m_crewList.m_type);
}

//Finds someone in the joinRequest list
POSTED_WEAPON*
strtFindPostedWeapon(UINT32 serialID)
{
	POSTED_WEAPONS::iterator it = postedWeapons.find(serialID);
	return ( it != postedWeapons.end() ) ? it->second : nullptr;
}
UINT32
strtGetNextSerial()
{
	return static_cast<UINT32>(serialCounter.getNext());
}

//////////////////////////////////////////////////////////////////////
// Cancel my reinforcement

void
POSTED_WEAPON::cancelReinforcement()
{
	ResupplyTicket* ticket = m_resupplyTicket;

	if ( ticket == nullptr )
		return;

	// We don't handle multiple-unit deliveries just yet
	assert( ticket->m_postedWeapon == this );
	assert( ticket->m_units == 1 );

	dbugLog(_FLD, "v:%u sup:%u c:%s cancelling reinforcement"
					, serial()
					, m_supplyingBrigade->unitID()
					, Chat::Name(m_handle).c_str()
					);

	m_supplyingBrigade->cancelResupplyTicket(ticket);

	assert( m_resupplyTicket == nullptr );
	assert( ticket->m_postedWeapon == nullptr );
}

//////////////////////////////////////////////////////////////////////
// Detach (remove my references, but leave active) my reinforcement
// request if it still exists

void
POSTED_WEAPON::detachReinforcement()
{
	ResupplyTicket* ticket = m_resupplyTicket;

	if ( ticket == nullptr )
		return;

	assert( ticket->m_postedWeapon == this );
	assert( ticket->m_units == 1 );

	dbugLog(_FLD, "v:%u shcu:%u c:%s detaching reinforcement"
					, serial()
					, m_supplyingBrigade->unitID()
					, Chat::Name(m_handle).c_str()
					);

	// Sever the reverse reference to ourselves
	m_resupplyTicket->m_postedWeapon = nullptr;
	m_resupplyTicket = nullptr;
}

//////////////////////////////////////////////////////////////////////
// Generate a reinforcement request for this piece of equipment
// Returns true on success.

void
POSTED_WEAPON::scheduleReinforcement()
{
	assert( m_supplyingBrigade != nullptr );

	detachReinforcement();

	if ( host_config.supply.enabled == false )
	{
		TELL_TESTER(m_commander, "Supply-free weapon: supply is disabled on this server.");
		return;
	}

	if ( isNonSupplied() )
	{
		TELL_TESTER(m_commander, "NON-SUPPLY weapon: not requesting reinforcement");
		return;
	}

	if ( m_supplyingBrigade->homeCP() == nullptr )
	{
		TELL_TESTER(m_commander, "Brigade is off-map, not scheduling reinforcement");
		return;
	}

	TOE_::WeaponInstance* toeEntry = m_supplyingBrigade->toeWeaponInstance(spec());
	if ( toeEntry == nullptr )
	{
		TELL_TESTER(m_commander, "ERROR: No toeWeaponInstance!");
		dbugLog(_FLA, "hcu:%u TOE has no toeWeaponInstance for %s while scheduling reinforcement"
						, m_supplyingBrigade->unitID()
						, weapPrintName(&spec()));
		return;
	}

	UINT32 resupplyTime = stratResupplyTime(spec(), m_supplyingBrigade, m_facility, toeEntry->resupplyTime());
	if ( resupplyTime == 0 )
	{
		TELL_TESTER(m_commander, "Welcome to your shiny new %s, resupplyTime was zero, so I guess its free", weapPrintFullName(&spec()));
		return;
	}

	m_resupplyTicket = m_supplyingBrigade->createResupplyTicket(*toeEntry, false, resupplyTime, 1, this);
	if ( m_resupplyTicket == nullptr )
	{
		TELL_TESTER(m_commander, "I tried to create a resupply ticket for you, but all I got was this NULL");
		return;
	}

	TELL_TESTER(m_commander, "Resupply ticket scheduled for %u minutes", resupplyTime / 60);

	assert( m_resupplyTicket->m_postedWeapon == this );
	assert( m_resupplyTicket->m_units == 1 );
}

//////////////////////////////////////////////////////////////////////
// Post a weapon reservation to the post lists

void
POSTED_WEAPON::post(WeaponEntry* atEntry, bool isReserve)
{
	assert( m_supplyingBrigade != nullptr );

	if ( isReserve )
		m_status = _RESERVED;
	else
		m_status = _POSTED;

	//put the timestamp on it
	m_posted = teulTime();

	if ( isReserve == false )
		m_supplyingBrigade->postedWeaponList().push_back(this);

	cnvyAddWeaponToExpireList(*this, (UINT32)host_config.supply.reserveTimeout_m * 60);

	if ( isNonSupplied() == false && isReserve == false )
	{
		m_postingEntry = atEntry;
		m_supplyingBrigade->incrementThrottle(this);
	}
	else
		m_postingEntry = nullptr;

	assert( m_resupplyTicket == nullptr || m_resupplyTicket->m_postedWeapon == this );
}

void
POSTED_WEAPON::cancelPost()
{
	assert( m_status == _RESERVED || m_status == _POSTED || m_status == _SPAWNED );
	assert( m_supplyingBrigade != nullptr );

	dbugLog(_FLD, "v:%u cancelling post", serial());

	cancelReinforcement();

	setMulticrew(false);

	// If the weapon is posted rather than spawned, cancel the throttle entry
	// (where applicable, e.g. depots)
	m_supplyingBrigade->decrementThrottle(this);

	if ( m_expiryUpdate != nullptr )
		cnvyRemoveWeaponFromExpireList(*this);

	// Remove the posting facility link
	if ( m_postingEntry )
		m_postingEntry = nullptr;

	m_supplyingBrigade->postedWeaponList().remove(this);

	m_status = _CANCELLED;

	assert( m_resupplyTicket == nullptr );
}

