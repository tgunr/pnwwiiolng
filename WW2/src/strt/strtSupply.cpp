#include <syscfg.h> // Path is project specific.
//
// Created: Fri 29 Aug 11:30:00 2003
//
// Funcitons dealing with supply
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Oliver "kfsone" Smith
// Maintaining Programmer: Oliver "kfsone" Smith
// Supporting Programmer:  Marty "Vandal" Poulin

#include "versionNo.h"
#include "dbug/dbug.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <assert.h>
#include <stack>

#include "include/arenaLockEnums.h"
#include "chat/chatSupportRPCMessages.h"
#include "chat/chatBroadcast.h"
#include "strt/strtCPStruct.h"
#include "strt/strtData.h"
#include "strt/strtFacilityStruct.h"
#include "strt/strtHCUnits.h"
#include "strt/strtHCUnitImpl.h"
#include "strt/strtHostDB.h"
#include "strt/strtIO.h"
#include "strt/strtLinkStruct.h"
#include "strt/strtObjectStruct.h"
#include "strt/strtPostedWeaponList.h"
#include "strt/strtResupplyTicket.h"
#include "strt/strtServerTypes.h"
#include "strt/strtSpawn.h"
#include "strt/strtSupply.h"
#include "strt/strtWeaponEntry.h"
#include "wwii/wwiiConfig.h"

// This is the list of units which are pending resupply
typedef std::set<StratHCUnit*> CONSUMERS ;
static CONSUMERS _resupplyConsumers ;

struct ResupplyTicketStore
{
	~ResupplyTicketStore()
	{
		for ( UINT32 i = 0 ; i < m_allocations.size() ; ++i )
		{
			delete [] m_allocations[i] ;
		}
	}

	ResupplyTicket* get()
	{
		if ( m_freeTickets.empty() )
			_reserveNewTickets() ;
		ResupplyTicket* result = m_freeTickets.top() ;

		m_freeTickets.pop() ;
		return result ;
	}

	void free(ResupplyTicket* ticket)
	{
		if ( ticket->m_postedWeapon != NULL )
		{
			assert( ticket->m_postedWeapon->m_resupplyTicket == NULL || ticket->m_postedWeapon->m_resupplyTicket == ticket ) ;
			ticket->m_postedWeapon->m_resupplyTicket = NULL ;
		}
		memset(ticket, 0, sizeof(*ticket)) ;
		assert( ticket->m_postedWeapon == NULL ) ;
		m_freeTickets.push(ticket) ;
	}

private:
	void _reserveNewTickets()
	{
		static const UINT32 ALLOC_RATE = 1024 ;
		ResupplyTicket* ticket = new ResupplyTicket[ALLOC_RATE] ;
		m_allocations.push_back(ticket) ;
		for ( UINT32 i = 0 ; i < ALLOC_RATE ; ++i )
		{
			m_freeTickets.push(&ticket[i]) ;
		}
	}

	std::stack<ResupplyTicket*> m_freeTickets ;
	std::vector<ResupplyTicket*> m_allocations ;
} ;
static ResupplyTicketStore _resupplyTicketStore ;

static int _debugSupplyEnabled = FALSE ;

//////////////////////////////////////////////////////////////////////
// Enable supply debug messages

void
strtEnableDebugSupply()
{
	_debugSupplyEnabled = TRUE ;
}

//////////////////////////////////////////////////////////////////////
// Disable supply debug messages

void
strtDisableDebugSupply()
{
	_debugSupplyEnabled = FALSE ;
}

//////////////////////////////////////////////////////////////////////
// Query supply debugging

LOGICAL
strtGetDebugSupply()
{
	return _debugSupplyEnabled ;
}

//////////////////////////////////////////////////////////////////////
// Find the UnitID of the HQUnit that is providing supply for this
// brigade at the specified facility

UINT32
strtGetSupplyingUnit(const FacilityStruct& facility, UINT32 brigadeID)
{
	// Nothing doing if the town is farked
	if ( facility.isMilitaryFacility() && facility.CP().side != facility.CP().controllingSide )
		return 0 ;

	const StratHCUnit* brigade = StratHCUnit::Lookup(brigadeID) ;
	if ( brigade == NULL || !brigade->isBrigade() )
	{
		dbugLog(_FLA, "hcu:%u is not a brigade", brigadeID) ;
		return 0 ;
	}

	return facility.getSupplyingUnit(brigade, brigade->homeCP()) ;
}

//////////////////////////////////////////////////////////////////////
// Cancel a resupply ticket

void
StratHCUnit::cancelResupplyTicket(ResupplyTicket* ticket)
{
	UINT32 docketNo = ticket->m_docket ;

	RESUPPLY_QUEUE::iterator qit = m_impl.m_resupplyQueue.find(docketNo) ;
	if ( qit == m_impl.m_resupplyQueue.end() )
	{
		dbugLog(_FLA, "hcu:%u queue:%u ticket:%p did not find ticket queue"
						, unitID(), docketNo, ticket) ;
		return ;
	}

	// Remove me from the docket
	RESUPPLY_DOCKET& docket = qit->second ;
	docket.remove(ticket) ;

	TOE_::WeaponInstance* entry = toeWeaponInstance(ticket->m_spec) ;
	if ( entry != NULL )
		entry->adjustResupply(-ticket->m_units, ticket->m_isTransfer) ;

	if ( ticket->m_isTransfer )
		m_impl.m_transfers.remove(ticket) ;

	_resupplyTicketStore.free(ticket) ;

	m_impl.m_totalTickets-- ;
}

//////////////////////////////////////////////////////////////////////
// Create a resupply ticket to deliver some vehicles
// postedWeapon is NULL for non-posted vehicles
// Set isTransfer to 'true' if you want the units to ship regardless
// of the bde's homeCP().isSupplied status.

ResupplyTicket*
StratHCUnit::createResupplyTicket(
	TOE_::WeaponInstance& entry
	, bool isTransfer				// Ship regardless of supplied status
	, UINT32 seconds, UINT16 units
	, POSTED_WEAPON* postedWeapon /* = NULL */
	)
{
	SINT32 spareCapacity = (SINT32) entry.capacity() - (SINT32)(entry.present() + entry.resupply()) ;
	if ( isTransfer == false )
	{
		if ( spareCapacity < 1 )
			return NULL ;

		if ( units > spareCapacity )
			units = (UINT16) spareCapacity ;
	}

	// If the posted entry already has a resupply ticket, cancel it
	if ( postedWeapon && postedWeapon->m_resupplyTicket )
		postedWeapon->cancelReinforcement() ;

	// Non-supplied weapons (which is all weapons in the case of training)
	// just automatically get returned
	if ( entry.weapon().isSupplied() == false || host_config.supply.enabled == false )
	{
		entry.setPresent(entry.capacity()) ;
		return NULL ;
	}

	if ( offMap() == true )
		return NULL ;

	if ( seconds == 0 && isTransfer == false )
	{
		if ( homeCP()->isSupplied() == true )
		{
			betaLog(_FLD, "hcu:%u(%s) weap:%s immediate delivery", unitID(), callsign(), weapPrintName(&entry.spec())) ;
			entry.setPresent(entry.present() + units) ;
		}
		return NULL ;
	}

	ResupplyTicket* ticket = _resupplyTicketStore.get() ;

	ticket->m_docket = m_impl.getDocketFor(seconds) ;
	ticket->m_spec = entry.spec() ;
	ticket->m_units = units ;
	ticket->m_postedWeapon = postedWeapon ;
	ticket->m_isTransfer = isTransfer ;

	if ( m_impl.m_resupplyQueue.empty() )
		_resupplyConsumers.insert(this) ;

	RESUPPLY_DOCKET& docket = m_impl.m_resupplyQueue[ticket->m_docket] ;
	docket.push_back(ticket) ;

	if ( isTransfer )
		m_impl.m_transfers.push_back(ticket) ;

	m_impl.m_totalTickets++ ;

	entry.adjustResupply(units, isTransfer) ;

	dbugLog(_FLD, "hcu:%u(%s) %s scheduled %u %s for %u [%p]"
					, unitID(), callsign()
					, (isTransfer ? "supply" : "resupply")
					, units, weapPrintName(&entry.spec())
					, seconds, postedWeapon
					) ;

	return ticket ;
}

//////////////////////////////////////////////////////////////////////
// Read the resupply queue for a unit into the supplied vector
// (for dot commands to dump the resupply queue of a brigade)

bool
StratHCUnit::fetchResupplyQueue(std::vector<std::string>& result, UINT32 limit, bool listAll)
{
	result.clear() ;
	if ( m_impl.m_resupplyQueue.empty() )
		return false ;

	if ( offMap() )
	{
		result.push_back("Brigade is OffMap - no supply") ;
		return true ;
	}

	result.reserve(limit + 1) ;
	result.push_back("placeholder") ;

	const bool supplyLinked = homeCP()->isSupplied() ;

	UINT32 entries = 0 ;
	RESUPPLY_QUEUE::iterator it ;
	UINT32 tickets = 0, listed = 0 ;

	UINT32 seconds ;

	char buffer[MAX_MESSAGE_LENGTH + 1] ;
	int offset ;

	for ( it = m_impl.m_resupplyQueue.begin() ; it != m_impl.m_resupplyQueue.end() ; ++it )
	{
		seconds = m_impl.nextServiceIn_s() ;
		if ( it->first > m_impl.m_nextServing )
			seconds += (it->first - m_impl.m_nextServing) * m_impl.docketRate_s() ;

		if ( seconds < 120 )
			offset = sprintf(buffer, "%u sec: ", seconds) ;
		else if ( seconds < 7200 )
			offset = sprintf(buffer, "%u min: ", seconds / 60) ;
		else
			offset = sprintf(buffer, "%u hr: ", seconds / 3600) ;

		RESUPPLY_MANIFEST manifest ;
		bool printable = false ;
		RESUPPLY_DOCKET::iterator dit ;
		for ( dit = it->second.begin() ; dit != it->second.end() ; ++dit )
		{
			if ( entries < limit && (listAll || supplyLinked || (*dit)->m_isTransfer) )
			{
				manifest[(*dit)->m_spec] += (*dit)->m_units ;
				printable = true ;
			}
			++tickets ;
		}
		if ( printable == false )
			continue ;

		RESUPPLY_MANIFEST::iterator order ;
		bool addComma = false ;
		for ( order = manifest.begin() ; order != manifest.end() ; ++order, ++entries )
		{
			if ( offset >= MAX_MESSAGE_LENGTH - 20 )
			{
				strcpy(buffer + offset, "...") ;
				break ;
			}
			if ( entries >= limit )
				continue ;
			if ( addComma )
			{
				buffer[offset++] = ',' ;
				buffer[offset++] = ' ' ;
			}
			else
				addComma = true ;
			offset += sprintf(buffer + offset, "%u*%s", order->second, weapPrintName(&order->first)) ;
			++listed ;
		}
		if ( addComma == false )
			continue ;
		buffer[offset++] = '.' ;
		buffer[offset] = 0 ;
		result.push_back(buffer) ;
	}

	offset = sprintf(buffer, "%u %s, ", tickets, (tickets == 1) ? "ticket" : "tickets") ;
	if ( listed == entries )
		offset += sprintf(buffer + offset, "%u %s", listed, (listed == 1) ? "delivery" : "deliveries") ;
	else
		offset += sprintf(buffer + offset, "%u/%u %s listed", listed, entries, (entries == 1) ? "delivery" : "deliveries") ;

	if ( supplyLinked == false )
		strcpy(buffer + offset, " [NO SUPPLY LINK]") ;
	if ( m_impl.m_inTransit )
		strcpy(buffer + offset, " [IN TRANSIT]") ;
	result[0] = buffer ;

	return true ;
}

//////////////////////////////////////////////////////////////////////
// Return the number of seconds until the next resupply ticket

UINT32
StratHCUnit::getSecondsToNextSupply() const
{
	if ( m_impl.m_resupplyQueue.empty() || host_config.supply.enabled == false )
		return 0 ;
	return m_impl.nextServiceIn_s() + (m_impl.m_resupplyQueue.begin()->first - m_impl.m_nextServing) * m_impl.docketRate_s() ;
}

//////////////////////////////////////////////////////////////////////
// Check the resupply queue for this unit

void
StratHCUnit::processResupply()
{
	RESUPPLY_QUEUE& resupply = m_impl.m_resupplyQueue ;

	// Purge empty dockets from the head of the queue.
	while ( resupply.empty() == false && resupply.begin()->second.empty() )
	{
		resupply.erase(resupply.begin()) ;
	}

	// Check to see if we're due another ticket yet
	if ( m_impl.m_nextServiceAt > teulClock64 )
		return ;

	// Transit is implemented as a delay; once the delay lapses,
	// the transit period is complete and supply resumes.
	if ( m_impl.m_inTransit )
		m_impl.m_inTransit = false ;

	// Have we moved out of supply? If so - nuke our resupply tickets
	if ( offMap() == true )
	{
		dbugLog(_FLI, "hcu:%u(%s) @ %s: Out of supply, flushing resupply queue"
						, unitID()
						, callsign()
						, ((homeCP() == NULL || homeCP() == routDestination[country()]) ? "offmap" : homeCP()->name)
						) ;

		// Flush the resupply queue and reset the capacities
		purgeResupply(true) ;

		return ;
	}

	// Update the last time we were in supply.
	if ( homeCP() && homeCP()->isSupplied() )
	{
		m_impl.m_lastTimeInSupply = teulTime() ;
	}

	// If the resupply queue is empty, we don't need to be on the
	// consumers list.
	if ( resupply.empty() )
	{
		m_impl.m_totalTickets = 0 ;
		_resupplyConsumers.erase(this) ;
		return ;
	}

	// Advance the service schedule
	m_impl.m_nextServiceAt = teulClock64 + m_impl.docketRate_ms() ;
	++m_impl.m_nextServing ;

	if ( resupply.begin()->first >= m_impl.m_nextServing )
		return ;

	bool isOutOfSupply = ( offMap() == true || homeCP()->isSupplied() == false ) ;

	// Pull off any/all tickets that have become due to a temporary
	// stash (to maintain iterator integrity)
	static RESUPPLY_MANIFEST supplies, transfers ;
	supplies.clear() ;
	transfers.clear() ;

	UINT32 processed = 0 ;
	while ( resupply.empty() == false )
	{
		RESUPPLY_QUEUE::iterator front = resupply.begin() ;
		RESUPPLY_DOCKET& docket = front->second ;
		if ( docket.empty() )
		{
			resupply.erase(front) ;
			continue ;
		}

		if ( front->first >= m_impl.m_nextServing )
			break ;

		// Summarize the tickets into a manifest
		RESUPPLY_DOCKET::iterator dit ;
		for ( dit = docket.begin() ; dit != docket.end() ; ++dit )
		{
			ResupplyTicket* ticket = *dit ;
			bool isTransfer = ticket->m_isTransfer ;

			RESUPPLY_MANIFEST& manifest = (!isTransfer) ? supplies : transfers ;

			if ( isTransfer || isOutOfSupply == false )
				manifest[ticket->m_spec] += ticket->m_units ;
			if ( ticket->m_isTransfer )
				m_impl.m_transfers.remove(ticket) ;
			_resupplyTicketStore.free(ticket) ;
			++processed ;
		}

		m_impl.m_totalTickets -= docket.size() ;

		docket.clear() ;

		resupply.erase(front) ;
	}

	// Import transfers
	RESUPPLY_MANIFEST::iterator order ;
	for ( order = transfers.begin() ; order != transfers.end() ; ++order )
	{
		TOE_::WeaponInstance* entry = toeWeaponInstance(order->first) ;
		if ( entry != NULL )
		{
			entry->adjustResupply(-(SINT32)(order->second), true) ;
			depositWeapons(order->first, order->second, true, NULL) ;
		}
	}

	// Import resupplies
	for ( order = supplies.begin() ; order != supplies.end() ; ++order )
	{
		TOE_::WeaponInstance* entry = toeWeaponInstance(order->first) ;
		if ( entry != NULL )
		{
			entry->adjustResupply(-(SINT32)(order->second), false) ;
			depositWeapons(order->first, order->second, false, NULL) ;
		}
	}

	dbugLog(_FLD, "hcu:%u processed %u tickets resupplyQueue has %zu queues left", unitID(), processed, resupply.size()) ;

	if ( resupply.empty() )
		_resupplyConsumers.erase(this) ;
}

//////////////////////////////////////////////////////////////////////
// Set all supplies to zero, purge all resupplies

void
StratHCUnit::purgeResupply(bool zeroSupply)
{
	if ( zeroSupply )
		dbugLog(_FLI, "hcu:%u(%s) purging entire supply list", unitID(), callsign()) ;
	else
		dbugLog(_FLI, "hcu:%u(%s) purging resupply list", unitID(), callsign()) ;
	TOE::iterator it ;
	for ( it = m_toe.begin() ; it != m_toe.end() ; ++it )
	{
		it->second.adjustResupply(- it->second.resupply(), true) ;
		if ( zeroSupply )
			it->second.setPresent(0) ;
		dbugLog(_FLI, "hcu:%u(%s) purged %s: %u/%u(%u/%u)"
						, unitID(), callsign()
						, weapPrintName(&it->second.spec())
						, it->second.present(), it->second.capacity()
						, it->second.transfers(), it->second.resupply()) ;
	}

	RESUPPLY_QUEUE::iterator rit ;
	for ( rit = m_impl.m_resupplyQueue.begin() ; rit != m_impl.m_resupplyQueue.end() ; ++rit )
	{
		// Summarize the tickets into a manifest
		RESUPPLY_DOCKET& docket = rit->second ;
		RESUPPLY_DOCKET::iterator dit ;
		for ( dit = docket.begin() ; dit != docket.end() ; ++dit )
		{
			_resupplyTicketStore.free(*dit) ;
		}
		docket.clear() ;
	}

	m_impl.m_resupplyQueue.clear() ;
	m_impl.m_transfers.clear() ;
	m_impl.m_totalTickets = 0 ;
	m_impl.m_nextServing = 0 ;
	m_impl.m_nextServiceAt = 0LL ;
	m_impl.m_inTransit = false ;

	_resupplyConsumers.erase(this) ;
}

//////////////////////////////////////////////////////////////////////
// Describe me

const char*
StratHCUnit::describe() const
{
	static char buffer[400] ;
	sprintf(buffer, "hcu:%u(%s) toe:%s(nv=%zu) %s dkts=%zu, tkts=%u, supplies=%zu, xfers=%zu"
					, unitID(), callsign()
					, toeTemplateHeader() ? toeTemplateHeader()->name().c_str() : "none", m_toe.size()
					, (m_impl.m_inTransit ? "InTransit" : "Stationed")
					, m_impl.m_resupplyQueue.size(), m_impl.m_totalTickets
					, m_impl.m_totalTickets - m_impl.m_transfers.size()
					, m_impl.m_transfers.size()
					) ;

	return buffer ;
}

//////////////////////////////////////////////////////////////////////
// Deliver all outstanding transfer tickets from the resupply queue

void
StratHCUnit::deliverTransfers()
{
	if ( m_impl.m_resupplyQueue.empty() || m_impl.m_transfers.empty() )
	{
		dbugLog(_FLD, "hcu:%u No transfers to deliver", unitID()) ;
		return ;
	}

	RESUPPLY_MANIFEST returns ;
	TRANSFERS::iterator it = m_impl.m_transfers.begin() ;
	while ( it != m_impl.m_transfers.end() )
	{
		ResupplyTicket* ticket = *it ;
		++it ;

		returns[ticket->m_spec] += ticket->m_units ;
		cancelResupplyTicket(ticket) ;
	}

	RESUPPLY_MANIFEST::iterator mit ;
	for ( mit = returns.begin() ; mit != returns.end() ; ++mit )
	{
		depositWeapons(mit->first, mit->second, true, NULL) ;
	}
}

//////////////////////////////////////////////////////////////////////
// Set everything back to full

void
StratHCUnit::resetSupply()
{
	purgeResupply(false) ;
	TOE::iterator it ;
	for ( it = m_toe.begin() ; it != m_toe.end() ; ++it )
	{
		TOE_::WeaponInstance& entry = it->second ;

		UINT32 capacity = entry.capacity() ;
		if ( capacity <= 0 )
			continue ;

		UINT32 present = entry.present() ;
		UINT32 limit = entry.overstockCapacity() ;
		if ( present >= capacity && present <= limit )
			continue ;

		entry.setPresent(capacity) ;
	}
}

//////////////////////////////////////////////////////////////////////
// Generate supply tickets for all absent vehicles
// If "isTransfer" is true, all vehicles with a capacity will be set
// to zero and scheduled to forcibly resupply (but not overstock),
// as when a brigade moves.
// Honestly, its starting to look like TransferSupply should be its
// own function.

void
StratHCUnit::restartSupply(UINT32 delay, UINT32 window, bool isTransfer)
{
	if ( offMap() )
		return ;

	if ( isTransfer == false && homeCP()->isSupplied() == false )
		return ;

	bool supplyEnabled = (host_config.supply.enabled != 0) ;
	bool trickleEnabled = supplyEnabled && host_config.supply.trickleEnabled ;
	const UINT32 trickleSlices = host_config.supply.trickleSlices ;

	if ( g_logDebug )
	{
		char buffer[256] ;
		sprintf(buffer, "hcu:%u(%s) %s supply in %um over %um [supply:%u, trickle:%u, slices:%u]"
						, unitID(), callsign()
						, (isTransfer) ? "transfering" : "restarting"
						, delay / 60, window / 60
						, supplyEnabled, trickleEnabled, trickleSlices
						) ;
		dbugLog(_FLD, "%s", buffer) ;
#ifdef BETA_RELEASE
		chatSendBroadcast(Chat::_HCUNIT, Instance::_CAMPAIGN, unitID(), LSTR_NOTE, LSTR_GENERIC, buffer) ;
#endif
	}

	if ( isTransfer )
	{
		dbugLog(_FLI, "hcu:%u(%s) is transfer: delivering transfers", unitID(), callsign()) ;
		deliverTransfers() ;
		dbugLog(_FLI, "hcu:%u(%s) is transfer: pausing resupplyQueue(%u)", unitID(), callsign(), delay) ;
		pauseResupplyQueue(delay) ;
		delay = 0 ;
	}

	// Go through all of our weapon entries and tell them to restart
	// their resupply tickets.
	TOE::iterator it ;
	for ( it = m_toe.begin() ; it != m_toe.end() ; ++it )
	{
		TOE_::WeaponInstance& entry = it->second ;
		if ( entry.capacity() == 0 )
			continue ;

		SINT32 shortfall = (SINT32)entry.capacity() - (SINT32)(entry.present() + entry.resupply()) ;
		SINT32 absent ;
		if ( isTransfer )
		{
			// We are transfering vehicles from A->B, take all present units
			absent = entry.present() ;
			entry.setPresent(0) ;
			betaLog(_FLI, "hcu:%u(%s) is transfer: %s absent = %u/%u", unitID(), callsign(), weapPrintName(&entry.spec()), absent, entry.capacity()) ;
		}
		else
			absent = shortfall ;

		if ( absent <= 0 )
			continue ;

		// If supply or trickle is disabled, just fill the list.
		if ( !trickleEnabled || (delay == 0 && window == 0) )
		{
			UINT32 delivery = supplyEnabled ? (entry.present() + absent) : 1 ;
			betaLog(_FLI, "hcu:%u(%s) %s instant delivery of %u", unitID(), callsign(), weapPrintName(&entry.spec()), delivery) ;
			entry.setPresent(delivery) ;
			continue ;
		}

		UINT32 target = (absent < entry.capacity()) ? (UINT32) absent : entry.capacity() ;

		// To avoid confusion, we're going to work with a consistent
		// resupply rate per-vehicle based on capacity rather than absent.
		UINT32 intervals = ( trickleSlices > 0 ) ? trickleSlices : target ;
		if ( intervals == 0 || window == 0 )
		{
			betaLog(_FLI, "hcu:%u(%s) %s immediate ticket of %u:%u:%u", unitID(), callsign(), weapPrintName(&entry.spec()), isTransfer, delay, absent) ;
			createResupplyTicket(entry, isTransfer, delay, absent) ;
			continue ;
		}

		// Tend towards a lazy (+1) interval
		// so 3 intervals in a 5 second window takes 6 seconds instead of 4
		UINT32 interval = (window + intervals - 1) / intervals ;

		// Make sure perInterval isn't 0
		UINT16 perInterval ;
		if ( target > intervals )
			perInterval = (target + intervals - 1) / intervals ;
		else
			perInterval = 1 ;

		for ( UINT32 i = 0 ; absent > 0 ; ++i )
		{
			UINT32 load = (absent >= perInterval) ? perInterval : absent ;
			createResupplyTicket(entry, isTransfer, delay + i * interval, load) ;
			absent -= load ;
		}
	}
}

//////////////////////////////////////////////////////////////////////
// During beta -- check all units to see if units that should be in
// supply are in supply.

#ifdef BETA_RELEASE
static void
_checkResupplyStatus(bool fullCheck)
{
	for ( HCUNIT_INDEX::iterator it = hcUnits.begin() ; it != hcUnits.end() ; ++it )
	{
		if ( it->second->isBrigade() == false && it->second->isDivision() == false )
			continue ;
		if ( it->second->isTrainingUnit() )
			continue ;

		StratHCUnit* unit = (StratHCUnit*)(it->second) ;
		try
		{
			unit->CheckResupplyStatus(fullCheck) ;
		}
		catch ( const char* error )
		{
			dbugLog(_FLA, "unit:%s error:%s", unit->callsign(), error) ;
		}
	}
}

void
StratHCUnit::CheckResupplyStatus(bool fullCheck)
{
	if ( !host_config.supply.enabled || isTrainingUnit() || !host_config.supply.debug )
		return ;

	bool inConsumerList = (_resupplyConsumers.find(this) != _resupplyConsumers.end()) ;
	bool needsResupply = false ;
	if ( offMap() && inConsumerList )
		throw "offmap && in consumer list" ;
	if ( m_impl.m_resupplyQueue.empty() && inConsumerList )
		throw "resupplyQueue empty && in consumer list" ;
	if ( homeCP()->isSupplied() == false )
		return ;

	bool error = false ;
	static std::string text = "" ;
	text.clear() ;

	TOE::const_iterator it ;
	for ( it = m_toe.begin() ; it != m_toe.end() ; ++it )
	{
		const TOE_::WeaponInstance& entry = it->second ;
		if ( entry.weapon().isSystemWeapon() )
			continue ;

		UINT32 present = entry.present() ;
		UINT32 transfers = entry.transfers() ;
		UINT32 resupply = entry.resupply() ;
		UINT32 capacity = entry.capacity() ;
		SINT32 delta = (SINT32)(capacity - (present + transfers + resupply)) ;
		UINT32 tickets = 0, queued = 0 ;

		RESUPPLY_QUEUE::iterator qit ;
		for ( qit = m_impl.m_resupplyQueue.begin()
			; qit != m_impl.m_resupplyQueue.end()
			; ++qit
			)
		{
			RESUPPLY_DOCKET::iterator dit ;
			for ( dit = qit->second.begin() ; dit != qit->second.end() ; ++dit )
			{
				ResupplyTicket* ticket = *dit ;
				if ( ticket->m_spec == entry.spec() )
				{
					++tickets ;
					queued += ticket->m_units ;
				}
			}
		}

		static char buffer[128] = " " ;
		if ( present >= capacity )
		{
			if ( fullCheck == false )
				continue ;
			sprintf(buffer+1, "%s:full", entry.weapon().name) ;
		}
		else if ( delta == 0 && capacity - present == queued )
		{
			if ( fullCheck == false )
				continue ;
			needsResupply = true ;
			sprintf(buffer+1, "%s:ok", entry.weapon().name) ;
		}
		else
		{
			needsResupply = true ;
			sprintf(buffer+1, "%s:[p:%u,t:%u,r:%u,c:%u,delta:%d,q:%u,t:%u]"
								, entry.weapon().name
								, present, transfers, resupply, capacity
								, delta
								, queued, tickets) ;
			error = true ;
		}

		text += buffer ;
	}

	if ( text.empty() == false )
	{
		if ( error )
			dbugLog(_FLA, "unit:%s %s", callsign(), text.c_str()) ;
		else
			dbugLog(_FLI, "unit:%s %s", callsign(), text.c_str()) ;
	}

	if ( needsResupply && inConsumerList == false && fullCheck == true )
		throw "Not in consumer list!" ;
}
#endif

//////////////////////////////////////////////////////////////////////
// Process the resupply consumers

void
strtProcessResupply()
{
	// Don't process resupply while the arena is locked
	if ( host_config.isArenaLocked(AL_SPAWN | AL_ENTRY) )
		return ;

#ifdef BETA_RELEASE
	static UINT32 lastCheck = teulTime() ;
	if ( lastCheck < teulTime() )
	{
		static UINT32 nextFullCheck = 0 ;
		if ( nextFullCheck <= teulTime() )
		{
			_checkResupplyStatus(true) ;
			nextFullCheck = teulTime() + 300 ;
		}
		else
			_checkResupplyStatus(false) ;
		lastCheck = teulTime() ;
	}
#endif

	CONSUMERS::iterator it ;
	for ( it = _resupplyConsumers.begin() ; it != _resupplyConsumers.end() ; )
	{
		// Tag this unit and increment the iterator incase we remove ourself
		StratHCUnit* unit = *(it++) ;
		unit->processResupply() ;
	}
}

