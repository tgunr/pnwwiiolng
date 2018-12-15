#include <syscfg.h> // Path is project specific.
// Created: Fri Dec  2 15:43:12 CST 2005
//
// Strat-specific HCUnits extension
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include <set>

#include "chat/chatBrigadeMove.h"
#include "chat/chatName.h"
#include "chat/chatStratSupportMessages.h"
#include "cnvy/cnvyMain.h"
#include "strt/strtCPStruct.h"
#include "strt/strtData.h"
#include "strt/strtFacilityStruct.h"
#include "strt/strtHCUnitImpl.h"
#include "strt/strtHCUnits.h"
#include "strt/strtHostDB.h"
#include "strt/strtLinkStruct.h"
#include "strt/strtPostedWeaponList.h"
#include "strt/strtResupplyTicket.h"
#include "strt/strtTOE.h"
#include "strt/strtWeaponEntry.h"
#include "teul/teulTime.h"
#include "wwii/wwiiConfig.h"
#include "wwii/wwiiDBThreads.h"

UINT32 brigadeMoveTime_s;
UINT32 brigadeFallbackTime_s;
UINT32 divisionMoveTime_s;
UINT32 divisionFallbackTime_s;
FLOAT32 pocketMovementDelayFactor = 2.0f;
UINT32 pocketTimeToSlowMoves_s = 60 * 60;

// If you get kicked off the map...
UINT32 unitReformTime_s;

CPStruct* routDestination[MAX_COUNTRIES];

static std::vector<StratHCUnit*> s_units;

//////////////////////////////////////////////////////////////////////
// Tell the database and other servers where this HCUnit is

void
StratHCUnit::announceHome() const
{
	if ( isTrainingUnit() )
		return;

	ChatHandle movedBy = lastMovedBy();
	if ( movedBy == 0LL )
		movedBy = chatGetHandle("SYSTEM");
	BRIGADE_MOVE_MSG outgoing(movedBy, 0, BRIGADE_MOVE_MSG::EXCHANGE);
	BrigadeMove& message = outgoing.data();
	message.m_brigadeID = unitID();
	message.m_destination = homeCP()->fullObjID;
	message.m_action = BMR_APPROVED;
	message.m_actionable = true;

	chatSendBrigadeMoveResult(outgoing);
}

// This is used by strat to broadcast a Unit country change to both the map and chat hosts.
// Chat sends the message on to all players.
void
StratHCUnit::announceGarrisonCountryChange() const
{
	if (isTrainingUnit())
		return;
	ChatHandle movedBy = lastMovedBy();
	if (movedBy == 0LL)
		movedBy = chatGetHandle("SYSTEM");
	GARRISON_COUNTRY_CHANGE_MSG outgoing(movedBy, 0, GARRISON_COUNTRY_CHANGE_MSG::EXCHANGE);
	GarrisonCountryChange& message = outgoing.data();
	message.m_brigadeID = unitID();
	message.m_newCountryId = country();

	stratSendGarrisonCountryChange(outgoing);
}

void
StratHCUnit::addTOEInstance(const TOE_::WeaponTemplate& weaponTemplate)
{
	if ( !isTrainingUnit() )
		m_toe.insert(std::make_pair(weaponTemplate.spec(), TOE_::WeaponInstance(weaponTemplate, *this, weaponTemplate.capacity())));
}

void StratHCUnit::clearTOEInstance()
{
	if (!isTrainingUnit())
		m_toe.clear();
}

//////////////////////////////////////////////////////////////////////
// Work out how the percentage supply level

UINT32
StratHCUnit::getSupplyPctg(WEAPON_CATEGORY cat) const
{
	if ( isTrainingUnit() )
		return 100;

	UINT32 present = 0, capacity = 0;

	WEAPON_SPEC start(country(), cat, 0, 0);
	WEAPON_SPEC end(country(), cat, 255, 255);

	TOE::const_iterator it;
	for ( it = m_toe.lower_bound(start); it != m_toe.upper_bound(end); ++it )
	{
		present += it->second.saveable();
		capacity += it->second.capacity();
	}

	if ( present == 0 || capacity == 0 )
		return 0;

	return present * 100 / capacity;
}

//////////////////////////////////////////////////////////////////////
// Needed for HCUnit

void
HCUnit::setHomeCPID(const FULL_OBJ_ID destination)
{
	m_homeCPID = destination;
}

//////////////////////////////////////////////////////////////////////
// Constructor

StratHCUnit::StratHCUnit(UINT32 withUnitID, HCUNIT_TYPE withTypeID, UINT32 withParentID
				, WWII_COUNTRY withCountryID, BRANCH_ENUM withBranchID, BRANCH_ENUM withMovementType, UCHAR withUnitClass
				, const ChatHandle withHandle, const char* withName, const char* withNick, UINT8 withRank
				, const FULL_OBJ_ID withHomeCPID
				, const ChatHandle withOwner, RANK_NO withOwnerRank
				, UINT32 withStartCpOID, UINT32 withFallbackCpOID
				, UINT32 withLastMove, const ChatHandle withLastMovedBy, UINT32 withNextMove
				, const char* withToeTemplate
				)
	: HCUnit(withUnitID, withTypeID, withParentID, withCountryID, withBranchID, withMovementType, withUnitClass, withHandle, withName, withNick, withRank, withHomeCPID, withOwner, withOwnerRank)
	, m_homeCP(nullptr)
	, m_startCP(strtGetCPByOID(withStartCpOID))
	, m_fallbackCP(strtGetCPByOID(withFallbackCpOID))
	, m_lastMoved(withLastMove)
	, m_nextMove(withNextMove)
	, m_lastCaptureTime(0)
	, m_next(nullptr)
	, m_lastMovedBy(withLastMovedBy)
	, m_nextEvent(nullptr)
	, m_postedWeaponList()
	, m_toeTemplateHeader(TOE_::TemplateHeader::Lookup(withToeTemplate))
	, m_toeTemplateHeaderName(withToeTemplate)
	, m_toe()
	, m_impl(*(new StratHCUnitImpl(*this)))
{
	if ( nullptr == m_startCP && 0 != withStartCpOID )
		dbugLog(_FLA, "hcu:%u(%s) invalid startCP %u", withUnitID, withNick, withStartCpOID);
	if ( nullptr == m_fallbackCP && 0 != withFallbackCpOID )
		dbugLog(_FLA, "hcu:%u(%s) invalid fallbackCP %u", withUnitID, withName, withFallbackCpOID);

	// If I'm a brigade, add me to the tail of my parent's list
	if ( isBrigade() && parent() )
	{
		StratHCUnit* tail = parent();
		while ( tail->next() != nullptr )
			tail = tail->next();
		tail->m_next = this;
	}
	else if ( isBrigade() && !parent() && !isTrainingUnit() )
		LogAbort("brigades loaded out of sequence: %u:%s", m_unitID, m_name);

	if ( isTrainingUnit() )
		m_toeTemplateHeader = nullptr;

	if ( m_toeTemplateHeader == nullptr )
	{
		if ( !isTrainingUnit() )
			dbugLog(_FLM, "hcu:%u %s has no/missing TOE template [%s]", m_unitID, m_name, (withToeTemplate ? withToeTemplate : "(null)"));
	}
	else
	{
		// Subscribe to the template and have it populate our vehicle list
		// Note: This gives us "default" equipment levels (i.e. full)
		// we will need to read in any stored levels from the database later.
		m_toeTemplateHeader->subscribe(this, m_toe);
	}

	if (isDivision() || isBrigade() || isGarrison())
		s_units.push_back(this);
}

//////////////////////////////////////////////////////////////////////
// Destructor

StratHCUnit::~StratHCUnit()
{
	if ( m_toeTemplateHeader != nullptr )
		m_toeTemplateHeader->unsubscribe(this, m_toe);
	s_units.erase(std::remove(s_units.begin(), s_units.end(), this));
	delete &m_impl;
}

//////////////////////////////////////////////////////////////////////
// Determine the rate at which we should consume dockets

TeulTimeMS
StratHCUnitImpl::docketRate_ms() const
{
	TeulTimeMS rate = DOCKET_RATE_MS;

	if ( rate > 0 && host_config.supply.populationAdjust > 0 )
	{
		FLOAT64 adjustRate  = (FLOAT64) host_config.supply.populationAdjust;
		FLOAT64 totalAdjust = 0, diffAdjust = 0;

		UINT32 myPop = chatGetPopulation(m_unit.side());
		UINT32 yourPop = chatGetPopulation(wwiiOtherSide(m_unit.side()));

		bool amUnderPop = (yourPop > myPop && yourPop - myPop > 10);
		if ( amUnderPop )
		{
			totalAdjust = (myPop + myPop) / adjustRate;
			diffAdjust = (100.0 - (myPop * 100.0 / yourPop)) / 2.75;
		}
		else
		{
			totalAdjust = (myPop + yourPop) / adjustRate;
		}

		FLOAT64 percent = 102.0 - (totalAdjust + diffAdjust);
		if ( percent > 100.0 )
			percent = 100.0;
		if ( percent < 50.0 )
			percent = 50.0;

		rate = (TeulTimeMS)ceil((FLOAT64)rate * percent / 100.0);
	}

#ifdef SLOWER_TICKETS_IN_POCKETS
	if ( m_unit.homeCP() == NULL || m_unit.homeCP()->isSupplied() == false )
		return rate * 110 / 100;
#endif

	return rate;
}

//////////////////////////////////////////////////////////////////////
// Call the impl function

//////////////////////////////////////////////////////////////////////
// Save the current data for this brigade to the database.

void
StratHCUnit::save() const
{
	// Safely derefence the current and fallback CP destinations into OIDs.
	UINT32 homeOID = (homeCP() ? homeCP()->uOID : 0);
	UINT32 fallbackOID = (fallbackCP() ? fallbackCP()->uOID : 0);

	DBThread::Do(DBThread::Default
		, "UPDATE strat_hc_units"
			" SET home_cp_oid = %u"
				", fallback_cp_oid = %u"
				", next_move = FROM_UNIXTIME(%u)"
				", modified = now()"
				", last_moved = %u"
				", last_moved_by = '%s'"
		        ", country_id = %u"
			" WHERE unit_id = %u"
				, homeOID
				, fallbackOID
				, nextMove()
				, lastMoved()
				, ((0LL != lastMovedBy()) ? Chat::Name(lastMovedBy()).c_str() : "SYSTEM")
		        , country()
				, unitID()
				);
}

//////////////////////////////////////////////////////////////////////
// Check for a TOE entry

const TOE_::WeaponInstance*
StratHCUnit::toeWeaponInstance(const WEAPON_SPEC forWeapon) const
{
	TOE::const_iterator it = m_toe.find(forWeapon);
	return ( it != m_toe.end() ) ? &(it->second) : nullptr;
}

TOE_::WeaponInstance*
StratHCUnit::toeWeaponInstance(const WEAPON_SPEC forWeapon)
{
	TOE::iterator it = m_toe.find(forWeapon);
	return ( it != m_toe.end() ) ? &(it->second) : nullptr;
}

//////////////////////////////////////////////////////////////////////
// Initialize a unit once the strat system is ready for it

void
StratHCUnit::initialize()
{
	if ( isTrainingUnit() )
	{
		setHomeCP(routDestination[country()], ~0, true);
		return;
	}

	CPStruct* home = strtGetCP(homeCPID());
	if ( home == nullptr )
	{
		// A "new" HCU might not have a home and need to be
		// deployed to either its startCP or else to the
		// off-map routing destination.
		if ( m_startCP != nullptr )
			home = m_startCP;
		else
			home = routDestination[country()];
	}

	// Carry-over any delay until next move after a host restart
	UINT32 redeployDelay;
	if ( m_nextMove > (UINT32)teulTime() )
		redeployDelay = m_nextMove - teulTime();
	else
		redeployDelay = 0;

	setHomeCP(home, redeployDelay, true);

	if ( canDeployTo(homeCP()) )
	{
		dbugLog(_FLI, "hcu:%u name:%s initial deployment at %s " PRF_FULL_OBJ_ID ""
						, unitID(), callsign(), (homeCP() ? homeCP()->name : "unknown")
						, PRA_FULL_OBJ_ID(homeCPID())
						);
		announceHome();
	}
	else
	{
		dbugLog(_FLW, "hcu:%u name:%s unable to allow deployment at %s " PRF_FULL_OBJ_ID ""
						, unitID(), callsign(), (homeCP() ? homeCP()->name : "unknown")
						, PRA_FULL_OBJ_ID(homeCPID())
						);
		rout();
	}

	if ( m_nextEvent != nullptr )
	{
		cnvyCancelBrigadeEvent(m_nextEvent);
		m_nextEvent = nullptr;
	}

	if ( isBrigade() && offMap() )
		m_nextEvent = cnvyScheduleBrigadeReformation(unitID(), nextMoveIn());

	if ( isBrigade() && offMap() == false && fallbackCP() == nullptr && parent() != nullptr && parent()->homeCP() != nullptr )
		m_fallbackCP = parent()->homeCP();
}

//////////////////////////////////////////////////////////////////////

void
strtInitializeHCUnits()
{
	HCUNIT_INDEX::iterator it;
	for ( it = hcUnits.begin(); it != hcUnits.end(); ++it )
	{
		HCUnit* hcUnit = it->second;
		if ( !hcUnit->isDivision() && !hcUnit->isBrigade() && !hcUnit->isGarrison() )
			continue;
		static_cast<StratHCUnit*>(hcUnit)->initialize();
	}

	strtdbRestoreTOESupplies();

	dbugLog(_FLI, "Scheduling resupply for all brigades");
	StratHCUnit* unit;
	for ( it = hcUnits.begin(); it != hcUnits.end(); ++it )
	{
		if ( it->second->isDivision() == false && it->second->isBrigade() == false && it->second->isGarrison() == false )
			continue;
		unit = (StratHCUnit*)it->second;
		if ( unit->homeCP() && unit->homeCP()->isSupplied() == false )
			continue;
		unit->restartSupply(60, 900, false);
	}
}

//////////////////////////////////////////////////////////////////////
// Find a StratHCUnit by unitID

StratHCUnit*
StratHCUnit::Lookup(UINT32 unitID)
{
	HCUnit* unit = HCUnit::Lookup(unitID);
	if ( unit == nullptr )
		return nullptr;
	if ( unit->isBrigade() || unit->isDivision() || unit->isGarrison() )
		return (StratHCUnit*) unit;
	return nullptr;
}

StratHCUnit*
StratHCUnit::Lookup(const char* name)
{
	HCUnit* unit = HCUnit::Lookup(name);
	if ( unit == nullptr )
		return nullptr;
	if ( unit->isBrigade() || unit->isDivision() || unit->isGarrison() )
		return (StratHCUnit*) unit;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////
// Check if a HCUnit has a link to a given CP

bool
StratHCUnit::hasLinkTo(const CPStruct* cp, LinkStruct*& link ) const
{
	if ( cp == nullptr || homeCP() == nullptr )
		return false;
	link = nullptr;
	if ( cp == homeCP() )
		return true;
	link = cp->linkTo(homeCP());
	return ( link != nullptr );
}

//////////////////////////////////////////////////////////////////////
// Check if a HCUnit has a link to a particular

bool
StratHCUnit::hasLinkTo(const StratHCUnit* rhs) const
{
	return ( rhs && rhs->homeCP() && homeCP() && (rhs->homeCP() == homeCP() || rhs->homeCP()->linkTo(homeCP())) );
}

//////////////////////////////////////////////////////////////////////
// Tell me when this unit was last in supply.

UINT32
StratHCUnit::lastTimeInSupply() const
{
	return m_impl.m_lastTimeInSupply;
}

//////////////////////////////////////////////////////////////////////
// Check if this unit is currently marked as in transit

bool
StratHCUnit::isSupplyInTransit() const
{
	return (offMap() == false && m_impl.m_inTransit == true);
}

//////////////////////////////////////////////////////////////////////
// Override the fallback destination

bool
StratHCUnit::setFallbackCP(CPStruct* toCP)
{
	assert( toCP != nullptr );

	// Cannot fallback to off-map
	if ( toCP == routDestination[country()] )
	{
		dbugLog(_FLI, "hcu:%u(%s) cannot fallback to offmap", unitID(), callsign());
		return false;
	}

	if ( deployPermission(toCP, true) != BMR_APPROVED )
	{
		dbugLog(_FLI, "hcu:%u(%s) deployPermission to %s denied for fallback destination", unitID(), callsign(), toCP->name);
		return false;
	}

	m_fallbackCP = toCP;

	return true;
}

//////////////////////////////////////////////////////////////////////
// Set the current homeCP

void
StratHCUnit::setHomeCP(CPStruct* toCP, UINT32 redeployDelay, bool forceDelay)
{
	// If the brigade is already deployed somewhere, remove it from the list
	// of units at that CP
	if ( homeCP() )
		homeCP()->removeUnit(*this);

	// During normal movement, the previous CP is remembered to provide
	// a first option for automatic brigade movement (routing, etc)
	if ( offMap() || toCP == routDestination[country()] )
		m_fallbackCP = nullptr;
	else if ( toCP != homeCP() )
		m_fallbackCP = homeCP();

	// Delete all of the throttles.
	if ( toCP != homeCP() )
		m_impl.m_throttles.clear();

	// Each brigade has a resting period after being moved before a
	// player can re-locate them, but only for moves or if forceDelay is true
	if ( toCP != homeCP() || forceDelay )
	{
		// Change -- record movement
		m_lastMoved = teulTime();
		m_homeCP = toCP;
		if ( toCP != nullptr )
			setHomeCPID(toCP->fullObjID);

		if ( redeployDelay > 0 )
			m_nextMove = teulTime() + redeployDelay;
		else
			m_nextMove = 0;
	}
	else
		m_nextMove = 0;

	// toCP might be null if the brigade is being withdrawn, but otherwise
	// it now needs adding to the list of units at that CP
	if ( toCP )
		homeCP()->addUnit(*this);

	// Update the database
	save();

	// Routed/offmap brigades should be automatically returned to the map
	// after a period of time
	if ( isBrigade() )
	{
		if ( m_nextEvent != nullptr )
		{
			cnvyCancelBrigadeEvent(m_nextEvent);
			m_nextEvent = nullptr;
		}

		if ( offMap() )
		{
			dbugLog(_FLI, "hcu:%u brigade reforms in %u", unitID(), unitReformTime_s);
			m_nextEvent = cnvyScheduleBrigadeReformation(unitID(), unitReformTime_s);
		}
	}
}

//////////////////////////////////////////////////////////////////////
// Return the number of sibling units a unit has in-play

UINT32
StratHCUnit::unitsOnMap() const
{
	UINT32 count = 0;
	for ( const StratHCUnit* unit = division(); unit != nullptr; unit = unit->next() )
	{
		if ( !unit->offMap() )
			++count;
	}

	return count;
}

//////////////////////////////////////////////////////////////////////
// Check if a StratHCUnit can redeploy to a particular CP
// Manual movements are subject to some extra checks

BRIGADE_MOVE_RESULT
StratHCUnit::deployPermission(const CPStruct* const cp, bool manualMove) const
{
	if ( cp == nullptr )
		return BMR_FAILED;

	if ( manualMove == true && cp != routDestination[country()] && toeTemplateHeader() == nullptr )
	{
		return BMR_FAILED_PERMISSION;
	}

	if ( isTrainingUnit() )
	{
		// Training units can ONLY be moved to offMap
		if ( cp == routDestination[country()] )
			return BMR_APPROVED;
		return BMR_FAILED;
	}

	// Always allow deployment to the offmap destination
	if ( cp == routDestination[country()] )
	{
		if ( manualMove == true )
		{
			if ( movementType() != BRANCH_LAND )
				return BMR_FAILED_OFFMAP;
			if ( homeCP() && homeCP()->isSupplied() == true )
				return BMR_FAILED_OFFMAP_NP;
			if ( getSupplyPctg(_INFANTRY) > host_config.supply.surrenderInf )
				return BMR_FAILED_PREM_SURDR;
		}
		return BMR_APPROVED;
	}

	// Otherwise we must always do a side check
	if ( cp->side != side() )
		return BMR_FAILED_OWNER;

	if ( cp->controllingSide != side() )
		return BMR_FAILED_CONTROL;

	if ( offMap() && cp != homeCP() && cp->isEnemyLinked() )
		return BMR_FAILED_FRONTLINE;

	if ( offMap() && cp->isSupplied() == false )
		return BMR_FAILED_NFL;

	// You can't move from a supplied town to a non-supplied town;
	// this allows movement within a pocket but not into a pocket
	if ( homeCP() && cp != homeCP() && homeCP()->isSupplied() && cp->isSupplied() == false )
		return BMR_FAILED_NFL;

	switch ( movementType() )
	{
	case BRANCH_LAND:
		if ( !offMap() )
		{
			LinkStruct* link = nullptr;
			// We must already be at the CP or directly linked to it.
			// If a link is involved, return it to us so we don't have
			// to look it up again.
			if ( hasLinkTo(cp, link) == false )
				return BMR_FAILED_LINK;
			// If the brigade is being moved and a link is involved, is it enemy owned?
			if ( manualMove == true && link != nullptr )
			{
				if ( link->endDepot->side != side() || link->startDepot->side != side() )
					return BMR_FAILED_ENEMY_DEPOT;
				if ( link->hasEnemyFirebase(side()) == true )
					return BMR_FAILED_ENEMY_FB;
			}

			// If we are moving, destination must be near the pack
			// Don't check this if we aren't moving
			if ( cp != homeCP() && unitsOnMap() > 1 && !cp->hasContactWithDivision(this) )
				return BMR_FAILED_CONTACT;
		}
		else
		{
			if ( isBrigade() && !cp->hasContactWithDivision(this) )
				return BMR_FAILED_CONTACT;
		}

		// Gotta have ABs to have a land brigade
		if ( branch() == BRANCH_LAND && !cp->hasABs(side()) )
			return BMR_FAILED_ABS;
	break;
	case BRANCH_AIR:
		if ( cp != homeCP() && cp->isEnemyLinked() )
			return BMR_FAILED_FRONTLINE;
		if ( !cp->hasFacilities(FACILITY_AIRBASE, side()) )
			return BMR_FAILED_AIR;
	break;
	case BRANCH_SEA:
		if ( !cp->hasFacilities(FACILITY_NAVALBASE, side()) )
			return BMR_FAILED_NAVAL;
	break;
	default:
		return BMR_FAILED;
	}

	return BMR_APPROVED;
}

//////////////////////////////////////////////////////////////////////
// Deploy a HC Unit at a particular CP

void
StratHCUnit::deploy(CPStruct* cp, UINT32 redeployDelay, const char* byCallsign, bool transferSupply, bool forceDelay)
{
	// Training units won't move anywhere.
	if ( isTrainingUnit() )
	{
		setHomeCP(routDestination[country()], ~0, true);
		return;
	}

	dbugLog(_FLI, "hcu:%u(%s) delay:%u redeploying to %s from %s"
					, unitID(), callsign(), redeployDelay
					, cp ? cp->name : "offmap"
					, (homeCP() != nullptr ? homeCP()->name : "<nowhere>")
					);

	const UINT32 fromOID = homeCP() ? homeCP()->uOID : 0;
	const UINT32 toOID = cp ? cp->uOID : 0;

	m_lastMovedBy = chatGetHandle(byCallsign);
	DBThread::Do(DBThread::Default
		, "INSERT INTO strat_hc_moves"
				" (unit_id, from_cp_oid, to_cp_oid, moved_by, delay, moved)"
		 	" VALUES"
				" (%u, %u, %u, '%s', %u, now())"
			, unitID(), fromOID, toOID, byCallsign, redeployDelay
		);

	cnvyCancelBrigadeEvent(m_nextEvent);
	m_nextEvent = nullptr;

	bool wasOffMap = !offMap();

	if ( wasOffMap )
		m_fallbackCP = nullptr;
	else
		m_fallbackCP = m_homeCP;
	setHomeCP(cp, redeployDelay, forceDelay);

	announceHome();

	UINT32 startTimeS = host_config.supply.trickleStart_m * 60;
	UINT32 trickleTimeS = host_config.supply.trickleTime_m * 60;

	if ( offMap() )
	{
		dbugLog(_FLD, "hcu:%u(%s) moved off map, purging resupply", unitID(), callsign());
		// Going off map nukes resupply tickest and resets
		// stock levels to capacity.
		purgeResupply(true);
	}
	else if ( transferSupply )
	{
		dbugLog(_FLD, "hcu:%u(%s) transfering supply", unitID(), callsign());
		// If we are moving between on-map towns, then we want
		// all stockpiled equipment to be forwarded to our new
		// address, otherwise we want a full supply list sending
		restartSupply(startTimeS, trickleTimeS, true);
	}
	else
	{
		dbugLog(_FLD, "hcu:%u(%s) moved with no transfer", unitID(), callsign());
	}

	// Fill up any gaps
	restartSupply(startTimeS, trickleTimeS, false);
}

//////////////////////////////////////////////////////////////////////
// Consider a link for its distance in kilometers and its number
// of armybases and deployed units to provide a desirability candle
// for falling back along this link.

static SINT32
_linkFallbackWeight(const LinkStruct* link)
{
	return ( (SINT32)link->endDepot->cp->numUnits() * 5 - (SINT32)link->endDepot->cp->numArmyBases );
}

//////////////////////////////////////////////////////////////////////
// Compare fallback weights of two links and determine if the current
// link is more desirable for a fallback than the previous link.

static bool
_preferableFallback(const LinkStruct* currentLink, const LinkStruct* previousLink)
{
	SINT32 curWeight = _linkFallbackWeight(currentLink);
	SINT32 prevWeight = _linkFallbackWeight(previousLink);
	if ( curWeight != prevWeight )
		return ( curWeight < prevWeight );
	return (currentLink->distance < previousLink->distance);
}

//////////////////////////////////////////////////////////////////////
// Find a fallback destination for this current CP

const CPStruct*
StratHCUnit::findFallback() const
{
	dbugLog(_FLI, "hcu:%u name:%s findFallback from %s: "
					, unitID(), callsign(), (homeCP() ? homeCP()->name : "offmap"));

	if ( offMap() )
	{
		dbugLog(_FLI, "offmaps don't fallback");
		return nullptr;
	}

	// Priority 1: Last location
	if ( fallbackCP()
			&& fallbackCP() != homeCP()
			&& fallbackCP() != routDestination[country()]
			&& canDeployTo(fallbackCP())
		)
	{
		dbugLog(_FLI, "fallback CP %s is valid", fallbackCP()->name);
		return fallbackCP();
	}

	CPStruct::LINKS::iterator it;
	LinkStruct* link;
	LinkStruct* candidate = nullptr;
	bool hadDivisionLink = false;
	bool hadSupply = false;

	const CPStruct* parentCP = (nullptr != parent()) ? parent()->homeCP() : homeCP();

	// Look thru the links for the shortest link
	// if one or more links closes in on the parent, use that.
	for ( it = homeCP()->m_links.begin(); it != homeCP()->m_links.end(); ++it )
	{
		link = *it;
		CPStruct* cp = link->endDepot->cp;

		dbugLog(_FLI, "%s? ", cp->name);

		// Priority 2: DivHQ
		if ( parentCP == cp )
		{
			dbugLog(_FLI, "has division!");
			return parentCP;
		}

		const bool isSupplied = cp->isSupplied();
		if ( hadSupply == true && isSupplied == false )
		{
			dbugLog(_FLI, "<supply ");
			continue;
		}

		// Does this destination meet the movement requirements.
		BRIGADE_MOVE_RESULT bmrResult = deployPermission(cp, false);
		if ( BMR_APPROVED != bmrResult )
		{
			dbugLog(_FLI, "Xrules[%u] ", bmrResult);
			continue;
		}

		bool divisionLink = (nullptr != parentCP && parentCP->linkTo(cp) != nullptr);

		// Priority 3: DivHQ + 1
		if ( false == divisionLink && true == hadDivisionLink )
		{
			dbugLog(_FLI, "no-divlink ");
			continue;
		}

		dbugLog(_FLI, "%d[%c] ", link->distance, (divisionLink ? 'Y' : 'n'));

		// Show less interest in contested towns when an uncontested town is available.
		if ( candidate != nullptr && true == cp->isContested() && false == candidate->endDepot->cp->isContested() )
		{
			dbugLog(_FLI, "<contested ");
			continue;
		}

		// This destination must meet one or more of the deployment/movement
		// rules for adjacency, etc, which means its either towards a member
		// brigade or it is towards a friendly brigade.
		if ( nullptr == candidate || (divisionLink && !hadDivisionLink) || (isSupplied && !hadSupply) || _preferableFallback(candidate, link) )
		{
			dbugLog(_FLI, "candidate ");
			candidate = link;
			hadDivisionLink = divisionLink;
			hadSupply = isSupplied;
		}
	}

	if ( candidate == nullptr )
	{
		dbugLog(_FLI, "no fallback options");
		return nullptr;
	}

	dbugLog(_FLI, "Using %s", candidate->endDepot->cp->name);

	return candidate->endDepot->cp;
}

//////////////////////////////////////////////////////////////////////
// Fallback to your previous location

bool
StratHCUnit::fallback()
{
	// If you're not somewhere, you can't fallback from there.
	if ( offMap() )
		return false;

	m_fallbackCP = const_cast<CPStruct*>(findFallback());
	if ( !fallbackCP() )
		return false;
	if (type() == HCU_GARRISON)
		return false;

	dbugLog(_FLI, "hcu:%u name:%s falling back from %s -> %s"
					, unitID(), callsign()
					, homeCP()->name, fallbackCP()->name
					);

	deploy(fallbackCP(), brigadeFallbackTime_s, "SYSTEM", true);

	m_fallbackCP = nullptr;

	return true;
}

//////////////////////////////////////////////////////////////////////
// Rout a unit

bool
StratHCUnit::rout()
{
	if ( isTrainingUnit() )
		return false;

	if ( fallback() )
		return true;

	if (type() == HCU_GARRISON)
		return false;

	dbugLog(_FLI, "hcu:%u(%s) routing ", unitID(), callsign());

	// Make sure there IS an offmap location
	CPStruct* destination = routDestination[country()];
	if ( destination == nullptr )
	{
		dbugLog(_FLA, "*** No fallback destination for country %u", country());
		return false;
	}

	// Fallback to the offmap location
	deploy(destination, unitReformTime_s, "SYSTEM", true);

	// You can't fallback
	m_fallbackCP = nullptr;

	return false;
}

//////////////////////////////////////////////////////////////////////
// Return number of seconds until next move

UINT32
StratHCUnit::nextMoveIn() const
{
	if ( nextMove() == 0 )
		return 0;

	if ( isTrainingUnit() )
		return 999;

	SINT32 delay = (SINT32)(nextMove() - teulTime());
	if ( delay < 0 )
		return 0;
	return (UINT32)delay;
}

static UINT32
_redeployTime(UINT32 deployTime, const CPStruct* const cp, UINT32 lastTimeInSupply)
{
	if ( cp->isSupplied() )
		return deployTime;

	// If this CP is not in supply, then check if the brigade has been
	// out of supply long enough to warrant a delay to the movement timer.
	if ( lastTimeInSupply >= teulTime() - pocketTimeToSlowMoves_s )
		return deployTime;

	return (UINT32)((FLOAT32)deployTime * pocketMovementDelayFactor);
}

//////////////////////////////////////////////////////////////////////
// Try and move a brigade, and return a BMR_ACTION result

BRIGADE_MOVE_RESULT
StratHCUnit::move(const char* byCallsign, CPStruct* cp)
{
	if ( cp == nullptr )
	{
		dbugLog(_FLW, "%s requested move %s to NULL CP", byCallsign, callsign());
		return BMR_FAILED_TARGET;
	}

	BRIGADE_MOVE_RESULT result = deployPermission(cp, true);

	dbugLog(_FLI, "%s requested move %s %s->%s returned %u"
					, byCallsign
					, callsign()
					, (homeCP() ? homeCP()->name : "offmap")
					, cp->name
					, result
					);

	if ( result != BMR_APPROVED )
		return result;

	if ( type() == HCU_GARRISON)
	{
		return BMR_FAILED_PERMISSION;
	}

	const bool fromOffMap = offMap();
	const bool frontMovement = cp->isEnemyLinked() || homeCP()->isEnemyLinked();

	// A brigade moving into a pocket from off-map gets a refresh on its
	// movement delay timers.
	if ( fromOffMap || (m_impl.m_lastTimeInSupply == 0) )
		m_impl.m_lastTimeInSupply = teulTime();

	UINT32 redeployDelay;
	if ( cp == routDestination[country()] )
		redeployDelay = unitReformTime_s;
	else if ( isBrigade() )
	{
		redeployDelay = _redeployTime(frontMovement ? brigadeMoveTime_s : brigadeFallbackTime_s, cp, m_impl.m_lastTimeInSupply);
	}
	else
	{
		redeployDelay = _redeployTime(frontMovement ? divisionMoveTime_s : divisionFallbackTime_s, cp, m_impl.m_lastTimeInSupply);
	}

	deploy(cp, redeployDelay, byCallsign, true);

	if ( fromOffMap && isDivision() )
	{
		for ( auto brigade = next(); brigade != nullptr; brigade = brigade->next() )
		{
			if ( brigade->offMap() && brigade->nextMoveIn() < 30 )
				brigade->deploy(cp, brigadeMoveTime_s, byCallsign, true);
		}
	}

	return result;
}

//////////////////////////////////////////////////////////////////////
// Relocate a division

bool
strtDeployDivision(HCUnit* unit, CPStruct* cp, bool transferSupply)
{
	if ( !unit->isDivision() )
	{
		dbugLog(_FLW, "Tried to rout non-division#%u %s [%d v %d]", unit->unitID(), unit->name(), unit->type(), HCU_DIVISION);
		return false;
	}

	if ( unit->type() == HCU_GARRISON ) 
	{
		dbugLog(_FLW, "Tried to rout Garrison#%u %s", unit->unitID(), unit->name());
		return false;
	}

	StratHCUnit* const division = (StratHCUnit*)unit;

	if ( division->homeCP() == cp )
		return true;

	if ( division->side() != cp->side )
	{
		dbugLog(_FLW, "Cannot move division#%u %s to enemy town", division->unitID(), division->name());
		return false;
	}

	division->deploy(cp, 0, "SYSTEM", transferSupply);

	for ( auto brigade = division->next(); brigade != nullptr; brigade = brigade->next() )
	{
		brigade->deploy(cp, 0, "SYSTEM", transferSupply);
	}

	return true;
}

//////////////////////////////////////////////////////////////////////
// Reform a brigade at its division HQ

bool
strtReformBrigade(UINT32 brigadeID, STRAT_UPDATE* const fromEvent)
{
	StratHCUnit* const brigade = StratHCUnit::Lookup(brigadeID);
	ABORT_IF(brigade == nullptr);
	if ( brigade->isTrainingUnit() )
		return false;
	if ( brigade->type() == HCU_GARRISON )
		return false;

	if ( !brigade->isBrigade() )
	{
		dbugLog(_FLW, "hcu:%u tried to reform non-brigade", brigadeID);
		return false;
	}

	if ( fromEvent != nullptr && brigade->nextEvent() != fromEvent )
	{
		dbugLog(_FLW, "hcu:%u incorrect event %p", brigadeID, fromEvent);
		return false;
	}

	if ( !brigade->offMap() )
	{
		dbugLog(_FLW, "hcu:%u brigade not off-map", brigadeID);
		return true;
	}

	if ( !brigade->parent() )
	{
		dbugLog(_FLA, "hcu:%u parentless brigade", brigadeID);
		return false;
	}

	if ( brigade->parent()->offMap() )
	{
		dbugLog(_FLN, "hcu:%u reformation failed -- parent is off-map", brigadeID);
		return false;
	}

	dbugLog(_FLI, "hcu:%u reforming to parent:%u at:%s", brigadeID, brigade->parentID(), brigade->parent()->homeCP()->name);
	brigade->deploy(brigade->parent()->homeCP(), brigadeFallbackTime_s, "SYSTEM", true);

	return true;
}

//////////////////////////////////////////////////////////////////////
// Deliver a number of vehicles into this TOE

bool
StratHCUnit::depositWeapons(const WEAPON_SPEC spec, UINT16 numUnits, bool allowOverstock, ResupplyTicket* const replaceTicket)
{
	if ( allowOverstock && host_config.supply.overstockLimit == 0 )
		allowOverstock = false;

	// Can't deposit weapons into an off-map brigade.
	if ( offMap() || isTrainingUnit() )
	{
		dbugLog(_FLD, "hcu:%u(%s) weap:%s num:%u ignoring -- unit is offmap"
						, unitID(), callsign(), weapPrintName(spec), numUnits);
		return false;
	}

	// Zero units is pointless
	if ( numUnits == 0 )
	{
		dbugLog(_FLW, "hcu:%u(%s) weap:%s trying to deposit 0 units"
						, unitID(), callsign(), weapPrintName(spec));
		return false;
	}

	TOE_::WeaponInstance* entry = toeWeaponInstance(spec);
	if ( entry == nullptr )
	{
		dbugLog(_FLA, "hcu:%u(%s) weap:%s weapon not in TOE"
						, unitID(), callsign(), weapPrintName(spec));
		return false;
	}

	const UINT32 overstockLimit = entry->overstockCapacity();
	if ( entry->present() >= overstockLimit )
	{
		dbugLog(_FLI, "hcu:%u(%s) weap:%s equipment pool at/over max oversupply limit (%u/%u->%u)"
						, unitID(), callsign(), weapPrintName(spec)
						, entry->present(), entry->capacity(), overstockLimit
						);
		return false;
	}

	if ( allowOverstock == false && entry->present() >= entry->capacity() )
	{
		dbugLog(_FLI, "hcu:%u(%s) weap:%s equipment pool at capacity (%u/%u)"
						, unitID(), callsign(), weapPrintName(spec)
						, entry->present(), entry->capacity()
						);
		return false;
	}

	UINT16 newPresent = entry->present() + numUnits;
	if ( allowOverstock == false && newPresent > entry->capacity() )
		newPresent = entry->capacity();
	// If they are overstocking, find what the ceiling is.
	if ( newPresent > overstockLimit )
		newPresent = overstockLimit;

	if ( newPresent < entry->present() )
		newPresent = entry->present();

	dbugLog(_FLD, "hcu:%u(%s) weap:%s deposit:%u %u=>%u"
					, unitID(), callsign(), weapPrintName(spec), numUnits
					, entry->present(), newPresent
					);

	entry->setPresent(newPresent);


	// Deposited -- if it came with a resupply ticket, cancel it
	if ( replaceTicket != nullptr )
		cancelResupplyTicket(replaceTicket);

	// Don't check resupply overage if we are over-limit but not over-supply.
	if ( entry->resupply() <= 0 )
		return true;

	const SINT32 resupplies = (SINT32)entry->resupply() - (SINT32)entry->transfers();
	SINT32 overage = (SINT32) entry->present() + resupplies - (SINT32) entry->capacity();
	if ( overage <= 0 )
		return true;

	dbugLog(_FLN, "hcu:%u(%s) removing %u resupply overage", unitID(), callsign(), overage);

	// Prune the resupply list

	// Because cancel resupplyTicket is destructive; pick out the candidates first.
	std::vector<ResupplyTicket*> candidates;

	RESUPPLY_QUEUE::iterator qit;
	RESUPPLY_DOCKET::iterator mit;
	for ( qit = m_impl.m_resupplyQueue.begin(); overage > 0 && qit != m_impl.m_resupplyQueue.end(); ++qit )
	{
		for ( mit = qit->second.begin(); overage > 0 && mit != qit->second.end(); ++mit )
		{
			ResupplyTicket* ticket = *mit;
			if ( ticket->m_isTransfer || ticket->m_spec != entry->spec() )
				continue;
			if ( ticket->m_units > overage )
			{
				ticket->m_units -= (UINT16)overage;
				entry->adjustResupply(-overage, false);
				overage = 0;
				break;
			}

			overage -= ticket->m_units;
			candidates.push_back(ticket);
		}
	}

	for ( UINT32 i = 0; i < candidates.size(); ++i )
		cancelResupplyTicket(candidates[i]);

	return true;
}

//////////////////////////////////////////////////////////////////////
// Specify a level for a particular vehicle

bool
StratHCUnit::initializeWeapon(const WEAPON_SPEC spec, UINT16 numUnits)
{
	TOE_::WeaponInstance* const entry = toeWeaponInstance(spec);
	if ( entry == nullptr )
	{
		dbugLog(_FLA, "hcu:%u weap:" PRF_WEAPON_SPEC " weapon not in TOE", unitID(), PRA_WEAPON_SPEC(spec));
		return false;
	}

	entry->setPresent(numUnits);

	return true;
}

//////////////////////////////////////////////////////////////////////
// Pause the resupply queue for this many seconds

void
StratHCUnit::pauseResupplyQueue(UINT32 seconds)
{
	if ( m_impl.m_nextServiceAt < teulClock64 )
		m_impl.m_nextServiceAt = teulClock64;
	m_impl.m_nextServiceAt += seconds * 1000;
	m_impl.m_inTransit = true;
}

//////////////////////////////////////////////////////////////////////
// Deliver a number of vehicles into this TOE

bool
StratHCUnit::withdrawWeapons(const WEAPON_SPEC spec, UINT16 numUnits)
{
	if ( isTrainingUnit() )
		return true;

	if ( numUnits == 0 )
	{
		dbugLog(_FLW, "hcu:%u weap:%s trying to withdraw 0 units", unitID(), weapPrintName(spec));
		return false;
	}

	TOE_::WeaponInstance* const entry = toeWeaponInstance(spec);
	if ( entry == nullptr )
	{
		dbugLog(_FLA, "hcu:%u weap:%s weapon not in TOE", unitID(), weapPrintName(spec));
		return false;
	}

	if ( entry->present() <= 0 )
	{
		dbugLog(_FLA, "hcu:%u weap:%s equipment pool already empty", unitID(), weapPrintName(spec));
		return false;
	}

	UINT32 newPresent = (numUnits < entry->present()) ? entry->present() - numUnits : 0;

	dbugLog(_FLD, "hcu:%u weap:%s withdraw:%u %u=>%u"
					, unitID(), weapPrintName(spec), numUnits
					, entry->present(), newPresent
					);

	entry->setPresent(newPresent);

	return true;
}

//////////////////////////////////////////////////////////////////////
// Reset an HCUnit to its original position and spawn lists

void
StratHCUnit::reset()
{
	// Purge all resupply
	purgeResupply(true);

	// Move to its original location
	deploy(startCP(), 0, "SYSTEM", false, true);

	// Reset the supply queue
	restartSupply(0, 0, false);

	// If we're a brigade and we have a division parent,
	// its location should be our default fallback position
	if ( isBrigade() == true && division() != nullptr )
		m_fallbackCP = division()->startCP();
	else
		m_fallbackCP = nullptr;
}

////////////////////////////////////////////////////////////////////////
// Restarts a Garrisons supply and initiates the trickle timer.

void StratHCUnit::restartGarrisonSupply(bool manualChange)
{
	if (!manualChange)
	{
		dbugLog(_FLN, "Garrison not changed manually");
		restartSupply(host_config.supply.garrisonInactiveTime_m * 60, host_config.supply.garrisonTrickleTime_m * 60, true);
	}
	else
	{
		UINT32 currentTime_s = teulTime();
		UINT32 startTime_s = getLastCaptureTime();
		UINT32 expireTime_s = startTime_s + host_config.supply.noPenaltySwitchPeriod_m * 60;
		if (startTime_s != 0 && (currentTime_s <= expireTime_s))
		{
			dbugLog(_FLN, "Garrison changed manually within no-penalty period");
			restartSupply(host_config.supply.garrisonInactiveTime_m * 60, host_config.supply.garrisonTrickleTime_m * 60, true);
		}
		else
		{
			dbugLog(_FLN, "Garrison changed manually");
			restartSupply(host_config.supply.mGarrisonInactiveTime_m * 60, host_config.supply.mGarrisonTrickleTime_m * 60, true);
		}
	}
}  

//////////////////////////////////////////////////////////////////////
// Force the HCUnit to process the next supply ticket

bool
StratHCUnit::forciblyProcessSupply()
{
	if ( m_impl.m_resupplyQueue.empty() )
		return false;

	m_impl.m_nextServiceAt = teulClock64;
	m_impl.m_nextServing = m_impl.m_resupplyQueue.begin()->first;
	processResupply();

	return true;
}

//////////////////////////////////////////////////////////////////////
// Facility throttle window handling.

void
StratHCUnit::decrementThrottle(const POSTED_WEAPON* const postedWeapon)
{
	if ( host_config.supply.enabled == false || host_config.throttle.enabled == false )
		return;

	if ( postedWeapon->m_postingEntry != nullptr && (postedWeapon->m_status == POSTED_WEAPON::_POSTED || postedWeapon->m_status == POSTED_WEAPON::_RESERVED) )
		m_impl.decrementThrottle(StratHCUnitImpl::ThrottleKey(postedWeapon->m_postingEntry->facility(), postedWeapon->spec()));
}

void
StratHCUnit::incrementThrottle(const POSTED_WEAPON* const postedWeapon)
{
	if ( host_config.supply.enabled == false || host_config.throttle.enabled == false )
		return;

	if ( postedWeapon->m_postingEntry != nullptr )
		m_impl.incrementThrottle(StratHCUnitImpl::ThrottleKey(postedWeapon->m_postingEntry->facility(), postedWeapon->spec()));
}

//////////////////////////////////////////////////////////////////////
// Find throttles that have timed out and reset them to zero.

void
StratHCUnit::checkThrottleWindows(const FacilityStruct* const facility) const
{
	if ( host_config.supply.enabled == false || host_config.throttle.enabled == false )
		return;

	StratHCUnitImpl::THROTTLE_MAP::iterator it = m_impl.m_throttles.lower_bound(StratHCUnitImpl::ThrottleKey(facility, NULL_WEAP_SPEC));
	for (; it != m_impl.m_throttles.end() && ((it->first >> 16) == facility->uOID); ++it )
	{
		it->second.Check();
	}
}

//////////////////////////////////////////////////////////////////////
// Check an individual throttle window

UINT32
StratHCUnit::checkThrottleWindow(const FacilityStruct* const facility, const WEAPON_SPEC spec) const
{
	if ( host_config.supply.enabled == false || host_config.throttle.enabled == false )
		return 0;

	StratHCUnitImpl::THROTTLE_MAP::iterator it = m_impl.m_throttles.find(StratHCUnitImpl::ThrottleKey(facility, spec));
	if ( it == m_impl.m_throttles.end() )
		return 0;
	return it->second.Check();
}

//////////////////////////////////////////////////////////////////////
// decrement the throttle counter

void
StratHCUnitImpl::decrementThrottle(UINT32 key)
{
	THROTTLE_MAP::iterator it = m_throttles.find(key);
	if ( it == m_throttles.end() )
		return;
	it->second.Decrement();
}

//////////////////////////////////////////////////////////////////////
// Raise the throttle window

void
StratHCUnitImpl::incrementThrottle(UINT32 key)
{
	const UINT16 windowDuration = host_config.throttle.timeM * 60;
	if ( windowDuration == 0 )
		return;

	Throttle& throttle = m_throttles[key];
	throttle.Increment(teulTime() + windowDuration);
}

//////////////////////////////////////////////////////////////////////
// Returns a list of all division/brigade units.

std::vector<StratHCUnit*>& strtGetDivisionsAndBrigades()
{
	return s_units;
}
