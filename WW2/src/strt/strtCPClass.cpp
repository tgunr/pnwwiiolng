#include <syscfg.h> // Path is project specific.
// Created: Wed Aug 25 12:00:00 2004
//
// Members of the CPStruct class.
//
// Copyright (c) 2004 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Oliver "kfsone" Smith
// Supporting Programmer:  Patrick "thunder" Magruder

#include "chat/chatBroadcast.h"
#include "chat/chatDefines.h"			// For GCI_...
#include "chat/chatGCIMessage.h"
#include "chat/chatGCITypes.h"
#include "chat/chatStratSupportMessages.h"
#include "chat/chatSupportRPCMessages.h"
#include "cnvy/cnvyMain.h"
#include "strt/strtCPClass.h"
#include "strt/strtCP.h"
#include "strt/strtCPStruct.h"
#include "strt/strtData.h"
#include "strt/strtFacilityStruct.h"
#include "strt/strtHCUnits.h"
#include "strt/strtHostDB.h"
#include "strt/strtHostLua.h"
#include "strt/strtHostMessages.h"
#include "strt/strtLinkStruct.h"
#include "strt/strtMain.h"
#include "strt/strtObjectiveStats.h"
#include "strt/strtObjectStruct.h"
#include "strt/strtServerTypes.h"
#include "strt/strtSpawn.h"
#include "strt/strtSupplyPropagator.h"
#include "strt/strtTypes.h"
#include "teul/teulTime.h"
#include "strt/strtTOE.h"
#include "wwii/wwiiConfig.h"
#include "wwii/wwiiHCUnits.h"
#include "garrisons/garrisonCountryChangeRequest.h"

static CPSET g_objectives[MAX_SIDES];
ObjectiveStats g_objectiveStats;

bool g_budgetHadGrace[MAX_SIDES];
UINT32 g_budgetGoodUntil[MAX_SIDES];

extern CPSET sideCPs[MAX_SIDES];

// Supply routing
static CPSET suppliers;

static bool supplyStabilized = false;

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
// Related but non-class functions
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// Adjust the figures for one side only

void
ObjectiveStats::update(WWII_SIDE forSide)
{
	// Only do this once every 5 seconds
	static const UINT32 STAT_UPDATE_INTERVAL_S = 5;
	if ( m_updated[forSide] > 0 && teulTime() - m_updated[forSide] < STAT_UPDATE_INTERVAL_S )
		return;

	m_cityAttacks[forSide] = m_cityDefends[forSide] = m_battles[forSide] = 0;
	m_bridgeAttacks[forSide] = m_bridgeDefends[forSide] = 0;

	// No players, no AOs
	const CPSET& objectives = g_objectives[forSide];
	CPSET::iterator it;

	for ( it = objectives.begin(); it != objectives.end(); ++it )
	{
		CPStruct* cp = (CPStruct*)(*it);
		if ( cp->isAttackObjective(forSide) )
		{
			if ( cp->isBridge() ) ++m_bridgeAttacks[forSide];
			else
			{
				++m_cityAttacks[forSide];
				if ( cp->numUnits() > 0 || cp->isSupplyCenter() )
					++m_battles[forSide];
			}
		}
		else if ( cp->isDefendObjective(forSide) )
		{
			if ( cp->isBridge() ) ++m_bridgeDefends[forSide];
			else ++m_cityDefends[forSide];
		}
	}

	m_cityAvailable[forSide] = m_cityLimit - std::min(m_cityAttacks[forSide], m_cityLimit);

	UINT32 bridgeObjectives = m_bridgeAttacks[forSide] + m_bridgeDefends[forSide];
	m_bridgeAvailable[forSide] = m_bridgeLimit - std::min(bridgeObjectives, m_bridgeLimit);

	m_updated[forSide] = teulTime();
}

//////////////////////////////////////////////////////////////////////
// Return stats on the current placement of capture objectives

void
ObjectiveStats::calculate()
{
	if ( chatGetPopulation(WWII_NO_SIDE) > 0 )
		m_cityLimit = stratCaptureObjectiveLimit();
	else
		m_cityLimit = 0;

	m_bridgeLimit = stratBridgeObjectiveLimit();

	voidStats(ALLIED);
	voidStats(AXIS);

	update(ALLIED);
	update(AXIS);
}

//////////////////////////////////////////////////////////////////////

static CPStruct*
_leastCaptured(CPStruct* prev, CPStruct* cur, WWII_SIDE forSide)
{
	if ( prev == nullptr )
		return cur;

	// If we control one but not the other, volunteer the uncontrolled one
	if ( prev->controllingSide != forSide && cur->controllingSide == forSide )
		return prev;
	if ( cur->controllingSide != forSide && prev->controllingSide == forSide )
		return cur;

	// If we own more armybases at either, volunteer the lesser
	if ( prev->armyBasesOwned[forSide] > cur->armyBasesOwned[forSide] )
		return cur;
	if ( prev->armyBasesOwned[forSide] < cur->armyBasesOwned[forSide] )
		return prev;

	// If we own less facilities at either, volunteer it
	if ( prev->facilitiesOwned[forSide] > cur->facilitiesOwned[forSide] )
		return cur;
	if ( cur->facilitiesOwned[forSide] > prev->facilitiesOwned[forSide] )
		return prev;

	// Does the new candidate have less EWS flags going off?
	if ( prev->flags & CP_HEAVY_EWS_FLAG_MASK && !(cur->flags & CP_HEAVY_EWS_FLAG_MASK) )
		return cur;
	if ( prev->flags & CP_NORM_EWS_FLAG_MASK && !(cur->flags & CP_NORM_EWS_FLAG_MASK) )
		return cur;

	return prev;
}

//////////////////////////////////////////////////////////////////////

static CPStruct*
_findSacrificialObjective(WWII_SIDE forSide, UINT32 currentTime)
{
	CPSET::iterator it;
	CPStruct* lastFit = nullptr;
	CPStruct* desperateFit = nullptr;

	// Look through all the iterators
	for ( it = g_objectives[forSide].begin(); it != g_objectives[forSide].end(); ++it )
	{
		CPStruct* current = (*it);

		// Only look at attack objectives on cities
		if ( !current->isCaptureObjective(forSide) )
			continue;

		// If there is an objective being cleared, there's nothing to do
		if ( current->m_clearing[forSide] )
			return current;

		if ( current->minimumObjectiveLifetime(currentTime) > 0 )
		{
			desperateFit = _leastCaptured(desperateFit, current, forSide);
			continue;
		}

		lastFit = _leastCaptured(lastFit, current, forSide);
	}

	// Did we find a fit?
	if ( lastFit != nullptr )
		return lastFit;

	// Ok - are we desperate enough to use a desperate fit?
	// i.e. two or more objectives out of whack
	if ( desperateFit == nullptr )
		return nullptr;

	SINT32 overBudget = (SINT32)(g_objectiveStats.m_cityLimit - g_objectiveStats.m_cityAttacks[forSide]);
	if ( overBudget <= 1 )
		return nullptr;

	dbugLog(_FLI, "ao: %s sacrificing %s objective out of desperation (%d over budget)"
					, _sideName[forSide]
					, desperateFit->name
					, overBudget
					);

	return desperateFit;
}

//////////////////////////////////////////////////////////////////////
// Check to see if the current objective limits are valid and
// save the stats

void
ObjectiveStats::checkLimits(UINT32 currentTime)
{
	// Lets make sure the figures are fresh
	calculate();

	// Step #1 - save the damn things :)
#if !defined(TRAINING_SERVER)
	save();

	// Now check the stats of both sides to see if they're valid
	checkSideLimits(ALLIED, currentTime);
	checkSideLimits(AXIS, currentTime);
#endif
}

//////////////////////////////////////////////////////////////////////
// Mini-helpers for checkSideLimits

static void
_setBudgetStatus(WWII_SIDE forSide, bool hadGrace, UINT32 atTime)
{
	g_budgetHadGrace[forSide] = hadGrace;
	g_budgetGoodUntil[forSide] = atTime;
}

static void
_startGracePeriod(WWII_SIDE forSide, UINT32 currentTime)
{
	const UINT32 gracePeriod = host_config.objective.gracePeriod_m * 60;
	_setBudgetStatus(forSide, false, currentTime + gracePeriod);
}

//////////////////////////////////////////////////////////////////////
// Check the limits for a particular side

void
ObjectiveStats::checkSideLimits(WWII_SIDE forSide, UINT32 currentTime)
{
	if ( m_cityAttacks[forSide] <= m_cityLimit )
	{
		_setBudgetStatus(forSide, false, 0);
		return;
	}

	// See if they are in their grace interval
	if ( !g_budgetHadGrace[forSide] )
	{
		if ( g_budgetGoodUntil[forSide] == 0 )
		{
			_startGracePeriod(forSide, currentTime);
#ifdef BETA_RELEASE
			if ( currentTime < g_budgetGoodUntil[forSide] )
			{
				///LOCALIZE
				char buf[128];
				strprintf(buf, "[beta] Too many objectives (%u/%u): grace period", m_cityAttacks[forSide], m_cityLimit);
				chatSendHCMessage(forSide, LSTR_GENERIC, buf);
			}
#endif
		}
	}

	// We've given them grace or a warning
	if ( currentTime < g_budgetGoodUntil[forSide] )
		return;

	CPStruct* cp = _findSacrificialObjective((WWII_SIDE)forSide, currentTime);

	if ( cp == nullptr )
	{
#ifdef BETA_RELEASE
		chatSendHCMessage(forSide, LSTR_GENERIC, "[beta] Too many objectives, no fit for removal");
#endif
		_startGracePeriod(forSide, currentTime);
		return;
	}

	if ( cp->m_clearing[forSide] )
	{
		// Since they're doing something re-start the grace period
		// if they're still overbudget once its gone
		_setBudgetStatus(forSide, false, 0);
		return;
	}

	dbugLog(_FLI, "ao: %s over budget on capture objectives %d/%d sacrificial cp is %s"
					, _sideName[forSide]
					, m_cityAttacks[forSide]
					, m_cityLimit
					, ((cp) ? cp->name : "<null>")
					);

	if ( !g_budgetHadGrace[forSide] )
	{
		dbugLog(_FLN, "ao: serving %s over-budget notice", _sideName[forSide]);

		// Give them a warning
		const UINT32 expiry = host_config.objective.warningPeriod_m;
		char buf[16];
		snprintf(buf, 15, "%u", expiry);
		chatSendHCMessage((WWII_SIDE)forSide, LSTR_TOO_MANY_AOS, buf);

		_setBudgetStatus(forSide, true, currentTime + (expiry * 60));

		return;
	}


#ifdef BETA_RELEASE
	chatSendHCMessage(forSide, LSTR_LABELLED_GENERIC, "[beta] Too many objectives - auto-culling", cp->name);
#endif
	dbugLog(_FLI, "ao: Sacrificing %s objective on %s", _sideNameAdj[forSide], cp->name);

	cp->startObjectiveExpiry((WWII_SIDE)forSide, host_config.objective.clearTime_m * 60);

	// Give them a fresh grace period
	_startGracePeriod(forSide, currentTime);
}

//////////////////////////////////////////////////////////////////////
// Return a list of current objectives for a given side; if possible, try to use
// the cached strings instead of building the list every time.

const std::string&
strtListObjectives(WWII_SIDE forSide, bool citysOnly)
{
	///LOCALIZE or transfer to client.
	static std::string text;
	text.clear();

	const CPSET& objectives = g_objectives[forSide];

	std::string attack = "", defend = "", soft = "";

	CPSET::iterator it;
	for ( it = objectives.begin(); it != objectives.end(); ++it )
	{
		CPStruct* cp = *it;
		std::string* list;
		bool isSoftcap = (cp->isCity() && cp->numUnits() == 0);

		if ( citysOnly && (cp->isCity() == false || isSoftcap) )
			continue;

		if ( cp->isDefendObjective(forSide) )
			list = &defend;
		else if ( isSoftcap )
			list = &soft;
		else if ( cp->isAttackObjective(forSide) )
			list = &attack;
		else
			continue;

		if ( list->size() > 0 )
			*list += ", ";

		if ( cp->m_clearing[forSide] )
			*list += "[";
		else if ( isSoftcap )
			*list += "(";
		*list += cp->name;
		if ( cp->isCity() && cp->isContested() )
			*list += "/con";
		if ( cp->m_clearing[forSide] )
			*list += "]";
		else if ( isSoftcap )
			*list += "/no bde)";
	}

	if ( attack.size() > 0 )
		text += "Attack: " + attack + ". ";
	if ( defend.size() > 0 )
		text += "Defend: " + defend + ". ";
	if ( citysOnly == false && soft.size() > 0 )
		text += "Maneuver: " + soft + ". ";

	if ( text.size() == 0 )
		text = "No current objectives.";
	else if ( citysOnly == true && soft.size() > 0 )
		text += "See '/ob l' for full list.";
	else if ( citysOnly == false )
		text += "See '/ob c' for city battles only.";

	return text;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
// C++ class functions
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

void
CPStruct::_changeFlags(UINT32 newFlags)
{
	if ( flags != (CP_STATUS_FLAGS)newFlags )
	{
		flags = (CP_STATUS_FLAGS)newFlags;
#ifdef IS_STRAT_HOST
		strtQueueCPStatus(this);
#endif // IS_STRAT_HOST
	}
}

//////////////////////////////////////////////////////////////////////
// Try to issue a fallback order
// If 'execute' is true, it's time to actually execute the fallback

const char* scmds[] = {
	"SCMD_NO_EFFECT"					// Caused nothing to happen
,	"SCMD_SUCCESS"					// Success
,	"SCMD_ALLOWED"					// Fallback is allowed but wasn't executed
,	"SCMD_NOT_OWNER"					// Country doesn't own town
,	"SCMD_WRONG_SIDE"					// Side doesn't own town
,	"SCMD_MUST_OAC"					// Must own and control
,	"SCMD_NO_ENEMY"					// No enemy presence
,	"SCMD_NOT_OVERRUN"				// Town isn't overrun
,	"SCMD_NO_ABS"						// No abs to fallback from
,	"SCMD_NO_LINKS"					// No links to fallback over
,	"SCMD_NO_CONTENTION"				// Town is not contested
,	"SCMD_REPEAT"						// Flag is already applied
,	"SCMD_OTHER_ORDERS"				// Already has other orders
,	"SCMD_ORDER_LIMIT"				// Side has too many orders posted
,	"SCMD_NO_OBJECTIVE"				// No objective assigned to %s
,	"SCMD_REQUEST_OUTSTANDING"		// You already have a request at this place
,	"SCMD_OBJECTIVE_FRONTLINE"		// Objectives require frontline towns
,	"SCMD_FRIENDLY_TOWN"				// Our town
,	"SCMD_CITY_AUTODEFENSE"			// You can only defend bridges
,	"SCMD_YOUNG_OBJECTIVE"			// Objective cannot be expired yet
,	"SCMD_STUCK_UNITS"				// Units that are trapped
,	"SCMD_SOFTCAP_DENIED"			// Too many softcaps
,	"SCMD_NO_BDE_LINK"				// No direct link for ground bde
,	"SCMD_NO_DIV_LINK"				// No members with direct link for ground div
,	"SCMD_ERROR"						// Must always be the last
};

SCMD_RESULT
CPStruct::_fallback(WWII_SIDE issuer, FALLBACK_MODE mode, UINT32 playerID)
{
	// Don't try to execute before having issued
	if ( mode == FALLBACK_EXECUTE && (!isFallingBack() || m_fallback == WWII_NO_SIDE || m_fallbackTime == 0) )
		return SCMD_ERROR;

	const char* fallbackType;
	switch ( mode )
	{
	case FALLBACK_TEST: fallbackType = "test"; break;
	case FALLBACK_START: fallbackType = "start"; break;
	case FALLBACK_EXECUTE: fallbackType = "execute"; break;
	default: fallbackType = "!UNKNOWN!"; break;
	}

	dbugLog(_FLI, "%s fallback order on %s for %s", fallbackType, name, _sideName[issuer]);

	// Dummy-run the fallback operation
	SCMD_RESULT testResult = strtFallback(this, issuer, mode, playerID);
	dbugLog(_FLI, "strtFallback(%s, %s, %s, %u) = %s(%u)"
					, name, _sideName[issuer], fallbackType, (UINT32)playerID, scmds[testResult], (UINT32)testResult
					);

	// How to interpret the result when we're not in execution mode
	if ( mode == FALLBACK_TEST )
		return testResult;

	if ( mode == FALLBACK_START )
	{
		if ( testResult != SCMD_ALLOWED )
			return testResult;

		if ( !setAlert(CP_STAT_FALLBACK)->isFallingBack() )
		{
			dbugLog(_FLW, "Somehow failed to set fallback flag");
			return SCMD_ERROR;
		}

		m_fallback = issuer;
		m_fallbackTime = teulTime() + (UINT32)(host_config.objective.fallbackNotice_m * 60);
		m_fallbackBy = playerID;

		return SCMD_SUCCESS;
	}

	// We're executing, SCMD_ALLOWED isn't what we're expecting
	if ( testResult == SCMD_ALLOWED )
	{
		dbugLog(_FLW, "Got SCMD_ALLOWED on a fallback on execution");
		return SCMD_ERROR;
	}

	// If this didn't uncontest the town, we still need to clear the fallback flag
	if ( isContested() )
		clearFlag(CP_STAT_FALLBACK);

	return testResult;
}

//////////////////////////////////////////////////////////////////////
// Wrapper for clearObjective which announces the withdrawl

void
CPStruct::announceObjectiveWithdrawl() const
{
	// Tell the attacker their objective has been withdrawn
	chatSendSideMessage(attacker(), nullObjID, LSTR_NONE, LSTR_OBJECTIVE_WITHDRAWN, name);

	// Tell the defender
	chatSendGCIMessage(GCI_CP_OBJECTIVE_WITHDRAWN, side, *this, nullObjID);
}

//////////////////////////////////////////////////////////////////////
// Change the ownership of an entire CP

bool CPStruct::changeOwnershipNoCapture(WWII_COUNTRY toCountry, bool force, bool quietly)
{
	dbugLog(_FLI, "%s %s %s change of ownership triggered to %s", ((quietly) ? "quietly" : "visibly"), ((force) ? "forcing" : "setting"),
			name, _countryName[toCountry]);

	// Firstly change standard facilities
	FacilityStruct *facility;
	for (facility = facilityList; facility != nullptr; facility = facility->next)
	{
		if (facility->type == FACILITY_FIREBASE)
			continue;
		if (force || facility->country != toCountry)
			strtConvertFacility(*facility, toCountry, force, quietly);
	}

	// Clear out the routeing
//	resetRouteing();

	return true;
}

bool
CPStruct::canChangeOwnership(WWII_COUNTRY toCountry, bool force)
{
	if (toCountry >= MAX_COUNTRIES)
	{
		dbugLog(_FLW, "Tried to change ownership of %s to %d (>= %d)", name, toCountry, MAX_COUNTRIES);
		return false;
	}

	// If the ownership already matches, we fail unless force is true
	if (!force && !isContested() && wwiiSide(toCountry) == wwiiSide(country))
	{
		dbugLog(_FLI, "%s.changeOwnership(%s) uneccessary - skipping", name, _countryName[toCountry]);
		return false;
	}

	return true;
}

bool
CPStruct::changeOwnership(WWII_COUNTRY toCountry, bool force, bool quietly)
{
	bool retVal = false;

	if (canChangeOwnership(toCountry, force))
	{
		dbugLog(_FLI, "%s %s %s change of ownership to %s", ((quietly) ? "quietly" : "visibly"), ((force) ? "forcing" : "setting"),
				name, _countryName[toCountry]);

		// Firstly change standard facilities
		FacilityStruct *facility;
		for (facility = facilityList; facility != nullptr; facility = facility->next)
		{
			if (facility->type == FACILITY_FIREBASE)
				continue;
			if ( force || facility->country != toCountry )
				strtConvertFacility(*facility, toCountry, force, quietly);
		}

		bool justCaptured = true; // This is needed to trigger town status check because town status may not have changed as a result of capture
		checkTownStatus(this, justCaptured, 0);

		// Clear out the routeing
		resetRouteing();
		retVal = true;
	}

	return retVal;
}

void CPStruct::changeOwnershipFromScript(WWII_COUNTRY toCountry, bool force, bool quietly)
{
	if (changeOwnership(toCountry, force, quietly))
	{
		changeAllGarrisonCountries(toCountry, false);
	}
}

//////////////////////////////////////////////////////////////////////
// Check the status of cp and update all linked cps recursively.

void
CPStruct::checkTownStatus(CPStruct *cp, bool justCaptured, int recurseLevel)
{
	TOWN_STATUS oldStatus = cp->m_townstatus;
	TOWN_STATUS newStatus = INACTIVE;

	dbugLog(_FLN, "1: Level %d Town status check started for %s. Current town status: %d.", recurseLevel, cp->name, oldStatus);

	for (auto it = cp->m_links.begin(), end = cp->m_links.end(); it != end; ++it) // Iterate though linked towns list
	{
		LinkStruct* outLink = *it;
		if (PN_UNLIKELY(outLink == nullptr || outLink->endDepot == nullptr))
			continue;

		CPStruct* otherCP = outLink->endDepot->cp; //This will point to the linked town
		dbugLog(_FLN, "2: Level %d %s is outgoing linked to %s", recurseLevel, cp->name, otherCP->name);

		if ((otherCP->side != cp->side) && (isValidSide(otherCP->side) && otherCP->country != WWII_CHINA)) //Check for enemy link
		{
			//Linked town is enemy
			newStatus = FRONT_LINE;
		}
		else if (otherCP->m_townstatus == FRONT_LINE)
		{
			if (newStatus != FRONT_LINE) //Dont change to lower status if higher status has been detected
			{
				newStatus = BACK_LINE;
			}
		}
		else if (otherCP->m_townstatus == BACK_LINE)
		{
			if (newStatus != FRONT_LINE && newStatus != BACK_LINE) //Dont change to lower status if higher status has been detected
			{
				newStatus = RESERVE;
			}
		}
	}

	if (newStatus == RESERVE) //Set to INACTIVE if no airfield or naval base
	{
		newStatus = INACTIVE;

		if ((cp->facilityTypeIndex[FACILITY_AIRBASE] != nullptr) || (cp->facilityTypeIndex[FACILITY_NAVALBASE] != nullptr))
		{
			newStatus = RESERVE;
		}
	}

	cp->m_townstatus = newStatus;
	switch (cp->m_townstatus)
	{
	case FRONT_LINE:
		strtFrontline(*cp, true);
		strtFrontlineLink(*cp, false);
		break;
	case BACK_LINE:
		strtFrontline(*cp, false);
		strtFrontlineLink(*cp, true);
		break;
	case RESERVE:
		strtFrontline(*cp, false);
		strtFrontlineLink(*cp, false);
		break;
	case INACTIVE:
		strtFrontline(*cp, false);
		strtFrontlineLink(*cp, false);
		break;
	}

	if (newStatus != oldStatus || justCaptured) //Capture or change of town status must trigger check of linked towns
	{
		//	Refresh Town Garrison Display mikemudd - Haven't considered this yet
		justCaptured = false;

		for (auto it = cp->m_links.begin(), end = cp->m_links.end(); it != end; ++it)
		{
			LinkStruct* outLink = *it;
			if (PN_UNLIKELY(outLink == nullptr || outLink->endDepot == nullptr))
				continue;

			CPStruct* otherCP = outLink->endDepot->cp; //This will point to the linked town
			dbugLog(_FLN, "3: Level %d About to do town status check for %s.", recurseLevel, otherCP->name);
			checkTownStatus(otherCP, justCaptured, ++recurseLevel);

		}

		strtQueueCPStatus(cp);

		dbugLog(_FLN, "4: Level %d Town status check complete for %s. Old town status: %d New town status: %d.", recurseLevel, cp->name, oldStatus, newStatus);
	}
	else
	{
		dbugLog(_FLN, "3: Level %d No status change detected for %s. Status is %d", recurseLevel, cp->name, cp->m_townstatus);
	}
}

//////////////////////////////////////////////////////////////////////
// Check the HCUnits at this city to make sure they belong here.

static void
_checkUnit(StratHCUnit* unit, const CPStruct* cp)
{
	if ( nullptr == unit || nullptr == cp || cp != unit->homeCP() )
		return;
	// Ensure we rout parents first.
	if ( nullptr != unit->parent() && cp == unit->parent()->homeCP() )
		_checkUnit(unit->parent(), cp);
	dbugLog(_FLI, "(hcu:%d name:%s side:%u) ", unit->unitID(), unit->callsign(), unit->side());
	
	if ( unit->canDeployTo(cp) )
		dbugLog(_FLI, "ok ");
	else if ( !unit->rout() )
		dbugLog(_FLI, "FAILED ");
	else
		dbugLog(_FLI, "routed ");
}

void
CPStruct::checkHCUnits()
{
	dbugLog(_FLI, "cp:%u name:%s hcus:%" PRSizeT " side:%u ", uOID, name, m_units.size(), side);

	HCUNIT_INDEX unitList = m_units;
	HCUNIT_INDEX::iterator it;
	for ( it = unitList.begin(); it != unitList.end(); ++it )
	{
		_checkUnit((StratHCUnit*)it->second, this);
	}
	dbugLog(_FLI, "End");
}

void CPStruct::setLastCaptureTime(UINT32 time) 
{
	m_lastCaptureTime = time;
}

//////////////////////////////////////////////////////////////////////
// Check whether the flags say we're under attack or not

CPStruct*
CPStruct::checkForUnderAttack()
{
	///TODO: Differentiate between "people here" and "under attack"
	if ( flags & CP_EWS_FLAG_MASK )
	{
		if ( !isUnderAttack )
		{
			dbugLog(_FLI, "ews: marking %s as under attack", name);
		}
		isUnderAttack = true;
	}
	else
	{
		if ( isUnderAttack )
		{
			dbugLog(_FLI, "ews: removing %s under attack flag", name);
		}
		isUnderAttack = false;
	}

	///TODO: Distinguish between "enemy here" and "fight here"

	return this;
}

//////////////////////////////////////////////////////////////////////
// Cancel an attack objective if it's still pending

void
CPStruct::_removeObjective(WWII_SIDE forSide)
{
	clearFlag(objectiveFlags[forSide]);

	// Remove the "work in progress" flag and remove the objective
	m_clearing[forSide] = false;

	g_objectives[forSide].erase(this);

#ifdef IS_STRAT_HOST
	strtQueueCPStatus(this);
	strtSaveCP(this);
#endif
}

CPStruct*
CPStruct::clearObjective(WWII_SIDE forSide)
{
	bool wasAttack = isAttackObjective(forSide);

	_removeObjective(forSide);
	if ( isCity() && isValidSide(forSide) )
		_removeObjective(wwiiOtherSide(forSide));

	// If you remove an AO from a town which is contested,
	// then the town has to uncontest itself.
	if ( isCity() && isContested() )
	{
		dbugLog(_FLI, "Aborted attack during contention: restoring %d ownership of %s"
						, country
						, name
						);
		changeOwnership(country, false, true);
	}

	if ( wasAttack )
	{
		if ( isCity () )
		{
			setTableStates();
			m_attackStarted = 0;
			m_attackingUnit = nullptr;
		}
		m_attacker = WWII_NO_SIDE;
	}

	if ( isCity() )
		strtCPEventTrigger(*this, "cpClearedAO");

	strtSaveCP(this);

	return this;
}

//////////////////////////////////////////////////////////////////////
// Remove any overruns at the top of the queue that are timed out

void
CPStruct::drainHistories()
{
	if ( m_overruns == nullptr )
		return;

	// Drain the overruns
	const UINT32 runTime = teulTime();
	const UINT32 lifeTime = (host_config.objective.overrunWindow_m * 60);
	UINT32 limit = runTime - lifeTime;

	// If we're overrun, overrun requests last a while longer
	if ( !isOverrun() )
		limit = runTime - lifeTime;
	else
		limit = runTime - 2 * lifeTime;

	while ( m_overruns->size() > 0 && m_overruns->front() <= limit )
	{
		m_overruns->erase(m_overruns->begin());
	}

	// If we've emptied the queue and we're in overrun state, exit the state
	if ( m_overruns->size() < 2 && isOverrun() )
		clearOverrun();

	if ( m_overruns->size() < 1 )
		clearOverrunHistory();
}

//////////////////////////////////////////////////////////////////////
// Clear out the various flags that clear when contention ends

CPStruct*
CPStruct::endContention()
{
	setContention(false);

#ifdef IS_STRAT_HOST
	clearAlerts();
	clearFallback();
	clearOverrunHistory();

	if ( isCity() && isValidSide(side) )
	{
		if ( isAttackObjective(side) )
		{
			dbugLog(_FLN, "cp:%s removing AO on endContention", name);
			// Victory: City captured, objective complete
			clearObjective(side);
		}
		else if ( isDefendObjective(side) && !isAttackObjective(wwiiOtherSide(side)) )
		{
			dbugLog(_FLA, "cp:%s lopsided capture objective", name);
			// WTF: This shouldn't happen
			clearObjective(side);
		}
		else if ( !isEnemyLinked() && isObjective() )
		{
			// Can only hold an AO on a frontline town
			if ( isAttackObjective(ALLIED) )
			{
				dbugLog(_FLI, "cp:%s removing ALLIED AO due to lack of link", name);
				announceObjectiveWithdrawl();
				clearObjective(ALLIED);
			}
			else if ( isAttackObjective(AXIS) )
			{
				dbugLog(_FLI, "cp:%s removing AXIS AO due to lack of link", name);
				announceObjectiveWithdrawl();
				clearObjective(AXIS);
			}
		}
	}

	// Make sure all the tables get re-processed
	setTableStates();

	resetRouteing();

	strtQueueCPStatus(this);
#endif

	strtCPEventTrigger(*this, "cpLeftContention");

	return this;
}

//////////////////////////////////////////////////////////////////////
// Complete any currently pending fallback procedure for this CP if it has timed
// out

SCMD_RESULT
CPStruct::executeFallback()
{
	const UINT32 runTime = teulTime();

	// If the flag isn't set, obviously nothing to do
	if ( !isFallingBack() || m_fallbackTime > runTime )
		return SCMD_NO_EFFECT;

	if ( m_fallback == WWII_NO_SIDE  )
	{
		clearFallback();
		return SCMD_NO_EFFECT;
	}

	WWII_SIDE forSide = m_fallback;
	WWII_SIDE otherSide = ((forSide == ALLIED) ? AXIS : ALLIED);

	SCMD_RESULT result = _fallback(m_fallback, FALLBACK_EXECUTE, m_fallbackBy);

	if ( result != SCMD_SUCCESS )
	{
		// If we were not successful, we still need to clear the fallback flag.
		clearFlag(CP_STAT_FALLBACK);

#ifdef BETA_RELEASE
		chatSendBroadcast(Chat::_HC, Instance::_CAMPAIGN, forSide, LSTR_BETA, LSTR_FALLBACK_FAILED, name);
#endif
		return result;
	}

	// Success, tell the world
	chatSendBroadcast(Chat::_SIDE, Instance::_CAMPAIGN, forSide, LSTR_NONE, LSTR_FALLING_BACK, name);
	chatSendBroadcast(Chat::_SIDE, Instance::_CAMPAIGN, otherSide, LSTR_NONE, LSTR_ENEMY_FALLING_BACK, name);

	// Leave the flag there for one CP run cycle
	m_fallback = WWII_NO_SIDE;

	return SCMD_SUCCESS;
}

//////////////////////////////////////////////////////////////////////
// Try to execute a hold-at-all-costs command

SCMD_RESULT
CPStruct::holdAtAllCosts(WWII_SIDE issuer, bool execute)
{
    //HACK: DEV-824 - .hold command temporary disabled
    return SCMD_ERROR;

#ifdef ENABLE_HOLD
	if ( side != issuer )
		return SCMD_WRONG_SIDE;

	if ( isFallingBack() )
		return SCMD_OTHER_ORDERS;

	if ( isHolding() )
		return SCMD_REPEAT;

	if ( !isDefendObjective(issuer) )
		return SCMD_NO_OBJECTIVE;

	if ( !isContested() )
		return SCMD_NO_CONTENTION;

	if ( controllingSide != issuer )
		return SCMD_MUST_OAC;

	if ( !execute )
		return SCMD_ALLOWED;

	if ( setAlert(CP_STAT_OWN_HAAC)->isHolding() )
		return SCMD_SUCCESS;

	return SCMD_ERROR;
#endif
}

//////////////////////////////////////////////////////////////////////
// Start a fallback procedure

SCMD_RESULT
CPStruct::issueFallback(WWII_SIDE issuer, bool testOnly, UINT32 playerID)
{
	if ( issuer == WWII_NO_SIDE )
	{
		dbugLog(_FLW, "p:%u Tried to fallback %s for invalid side", playerID, name);
		return SCMD_ERROR;
	}

	if ( testOnly )
		return _fallback(issuer, FALLBACK_TEST, playerID);
	else
		return _fallback(issuer, FALLBACK_START, playerID);
}

//////////////////////////////////////////////////////////////////////
// Add an entry to the overrun history

void
CPStruct::markOverrunReported()
{
	UINT32 now = teulTime();

	if ( m_overruns == nullptr )
		m_overruns = new OVERRUNS;

	// Record the timestamp of this event
	m_overruns->push_back(now);

	// Expire any leading wastage
	drainHistories();

	// Don't let the list get stupidly large
	if ( m_overruns->size() > 20 )
		m_overruns->erase(m_overruns->begin());
}

//////////////////////////////////////////////////////////////////////
// Determine how long until the current objective expires
// NOTE: Does not check if there is an objective

UINT32
CPStruct::minimumObjectiveLifetime(UINT32 currentTime) const
{
	UINT32 expires = m_attackStarted + (host_config.objective.minLifetime_m * 60);

	if ( expires <= currentTime )
		return 0;

	return expires - currentTime;
}

//////////////////////////////////////////////////////////////////////
// Calculate how many overruns required for a trigger

#ifdef BETA_RELEASE
static const UINT32 MIN_OVERRUNS = 1;
#else
static const UINT32 MIN_OVERRUNS = 3;
#endif

static UINT32
_overrunReportsRequired(WWII_SIDE forSide)
{
	UINT32 players = chatGetPopulation(forSide);

	if ( players <= 200 )
		return MIN_OVERRUNS;

	return MIN_OVERRUNS + (players / 200);
}

//////////////////////////////////////////////////////////////////////
// Try to execute an overrun order

SCMD_RESULT
CPStruct::overrun(WWII_SIDE issuer)
{
	if ( side != issuer )
		return SCMD_WRONG_SIDE;

	if ( isHolding() || isFallingBack() )
		return SCMD_OTHER_ORDERS;

#ifndef BETA_RELEASE
	if ( !isUnderAttack )
		return SCMD_NO_ENEMY;
#endif

	if ( !isContested() )
		return SCMD_NO_CONTENTION;

	if ( controllingSide != issuer )
		return SCMD_MUST_OAC;

	if ( armyBasesOwned[issuer] == 0 )
		return SCMD_NO_ABS;

	markOverrunReported();

	// If it's already flagged as overrun, the report is uneccessary
	if ( isOverrun() )
		return SCMD_REPEAT;

	//// You meet the criteria, did you raise the bar?

	if ( m_overruns->size() < _overrunReportsRequired(side) )
	{
		return SCMD_ALLOWED;
	}

	if ( setAlert(CP_STAT_OVERRUN)->isOverrun() )
		return SCMD_SUCCESS;

	return SCMD_ERROR;
}

//////////////////////////////////////////////////////////////////////
// Determine if a side has any objectives available

static bool
_sideObjectivesAvailable(WWII_SIDE forSide, bool isCity)
{
	g_objectiveStats.update(forSide);

	if ( isCity )
		return (g_objectiveStats.m_cityAvailable[forSide] > 0);
	else
		return (g_objectiveStats.m_bridgeAvailable[forSide] > 0);
}

//////////////////////////////////////////////////////////////////////
// Determine if this capture is within engagement/softcap quotas.
// true = engagement capacity allows this,
// false = softcap with insufficient engagements placed

static bool
_checkEngagementOrSoftcap(WWII_SIDE forSide, const CPStruct* cp)
{
	// A soft cap is a city with no brigade to defend
	if ( cp->isCity() == false || cp->numUnits() > 0 )
		return true;
	// The one exception is factory towns which have a high
	// strategic value and therefore may be subject to abuse
	// of the rules to deprive the enemy the opportunity to
	// attack.
	if ( cp->isSupplyCenter() )
		return true;

	if ( stratCountsAsEngagement(*cp) )
		return true;

	// Find out how many - if any - engagements are required before a
	// softcap can be placed.
	UINT32 engagementQuota = stratEngagementObjectivesQuota(g_objectiveStats.attackCapacity());
	// Per Doc's request, when it's zero, allow softcaps for low pop.
	if ( engagementQuota == 0 )
		return true;

	// Get the latest figures
	g_objectiveStats.update(forSide);

	return ( g_objectiveStats.battles(forSide) >= engagementQuota );
}

//////////////////////////////////////////////////////////////////////
// Attack Objective placement request

SCMD_RESULT
CPStruct::requestAttack(bool execute, UINT32 brigadeID)
{
	StratHCUnit* unit = StratHCUnit::Lookup(brigadeID);
	if ( unit == nullptr )
	{
		dbugLog(_FLA, "Attempted to issue attack request with brigadeID %u", brigadeID);
		return SCMD_ERROR;
	}

	if ( isCity() )
	{
		if ( side == unit->side() || country == WWII_CHINA )
			return SCMD_FRIENDLY_TOWN;
		if ( !isEnemyLinked() )
			return SCMD_OBJECTIVE_FRONTLINE;
	}

	if ( unit->offMap() )
		return SCMD_OTHER_ORDERS;

	if ( isObjective(unit->side()) )
		return SCMD_OTHER_ORDERS;

	if ( _sideObjectivesAvailable(unit->side(), isCity()) == false )
		return SCMD_ORDER_LIMIT;

	if ( _checkEngagementOrSoftcap(unit->side(), this) == false )
		return SCMD_SOFTCAP_DENIED;

#ifdef REQUIRE_BRIGADE_LINK_TO_TARGET
	if ( BRANCH_LAND == unit->movementType() && this->linkTo(unit->homeCP()) == false )
	{
		// Brigades require a direct link.
		if ( unit->isBrigade() )
			return SCMD_NO_BDE_LINK;

		// Divisions just require a member link.
		if ( this->hasContactWithDivision(unit) == false )
			return SCMD_NO_DIV_LINK;
	}
#endif

	if ( !execute )
		return SCMD_ALLOWED;

#ifdef STRAT_ANNOUNCE_OBJECTIVES
	if ( isCity() )
		chatSendSideMessage(unit->side(), nullObjID, LSTR_NONE, LSTR_CAPTURE_ORDERS, name);
	else
		chatSendSideMessage(unit->side(), nullObjID, LSTR_NONE, LSTR_ATTACK_ORDERS, name);
#endif

	setAttackObjective(true, unit);

	return SCMD_SUCCESS;
}

//////////////////////////////////////////////////////////////////////
// Clear objectives request
// We pass back the time it will take in minutes to remove the objective in data

SCMD_RESULT
CPStruct::requestClear(WWII_SIDE forSide, UINT32& data, bool execute)
{
	// Can't place a defend where an attack already sits
	if ( !isObjective(forSide) )
		return SCMD_NO_OBJECTIVE;

	if ( isCity() && isDefendObjective(forSide) )
		return SCMD_CITY_AUTODEFENSE;

	if ( m_clearing[forSide] )
		return SCMD_OTHER_ORDERS;

	if ( isCaptureObjective(forSide) )
	{
		data = minimumObjectiveLifetime(teulTime());
		if ( data > 0 )
			return SCMD_YOUNG_OBJECTIVE;
	}

	if ( !execute )
		return SCMD_ALLOWED;

	startObjectiveExpiry(forSide, host_config.objective.clearTime_m * 60);

	return SCMD_SUCCESS;
}

//////////////////////////////////////////////////////////////////////
// Attack Objective placement request

SCMD_RESULT
CPStruct::requestDefend(WWII_SIDE forSide, bool execute)
{
	// Can't place a defend where an attack already sits etc
	if ( isObjective(forSide) )
		return SCMD_OTHER_ORDERS;

	if ( !isBridge() && side != forSide )
		return SCMD_WRONG_SIDE;

	if ( isCity() )
		return SCMD_CITY_AUTODEFENSE;

	if ( !_sideObjectivesAvailable(forSide, isCity()) )
		return SCMD_ORDER_LIMIT;

	if ( !execute )
		return SCMD_ALLOWED;

	setDefendObjective(forSide, true);

	return SCMD_SUCCESS;
}

//////////////////////////////////////////////////////////////////////
// Events that are run once every CP cycle

CPStruct*
CPStruct::runCycleEvents(UINT32 runtime)
{
	// Expire EWS reports
	expireEnemyReports(runtime);

	// Reconsider the EWS state
	evaluateEWS();

	// Execute fullback if there is one
	executeFallback();

	// Drain the various fallback/etc histories
	drainHistories();

	// Is this an attack objective being withdrawn?
	if ( isObjective() && m_attacker != WWII_NO_SIDE && m_clearing[m_attacker] )
	{
		chatSendSideMessage(m_attacker, fullObjID, LSTR_CAUTION, LSTR_WITHDRAW_REMINDER, name);
	}

	return this;
}

//////////////////////////////////////////////////////////////////////
// Flag this location as having an attack objective without modifying counters

CPStruct*
CPStruct::setAttackObjective(bool modifyObjectiveList, HCUnit* unit)
{
	WWII_SIDE forSide = unit->side();

	if ( !isAttackObjective(forSide) )
	{
		setFlag(attackFlags[forSide]);

		if ( isCity() && isValidSide(side) && !isDefendObjective(side) )
		{
# ifdef STRAT_ANNOUNCE_OBJECTIVES
			chatSendSideMessage(side, nullObjID, LSTR_NONE, LSTR_AUTODEFENSE_ORDERS, name);
# endif // STRAT_ANNOUNCE_OBJECTIVES
			setDefendObjective(side, true);
		}

		if ( isCity() )
			setTableStates(true);
	}

	m_attacker = forSide;

	if ( isCity() )
	{
		m_attackStarted = teulTime();
		m_attackingUnit = unit;
	}

	// Abandon any clear attempt we were making
	m_clearing[forSide] = false;
	g_objectiveStats.voidStats(forSide);

	if ( modifyObjectiveList )
	{
		g_objectives[forSide].insert(this);
	}

#ifdef IS_STRAT_HOST
	strtQueueCPStatus(this);
	strtSaveCP(this);
#endif

	dbugLog(_FLN, "cp:%s hcu:%s placing attack objective", name, unit->callsign());

	return this;
}

//////////////////////////////////////////////////////////////////////
// Flag this location as having a defense objective without modifying counters

CPStruct*
CPStruct::setDefendObjective(WWII_SIDE forSide, bool modifyObjectiveList)
{
	WWII_SIDE otherSide = wwiiOtherSide(forSide);
	assert( isBridge() || !isValidSide(otherSide) || isAttackObjective(otherSide) );
	if ( isCity() && !isAttackObjective(otherSide) )
		dbugLog(_FLA, "cp:%s setting city defend objective without attack???", name);

	setFlag(defendFlags[forSide]);

	// Abandon any clear attempt we were making
	m_clearing[forSide] = false;

	g_objectiveStats.voidStats(forSide);

	if ( modifyObjectiveList )
	{
		g_objectives[forSide].insert(this);
	}

#ifdef IS_STRAT_HOST
	strtQueueCPStatus(this);
	strtSaveCP(this);
#endif

	dbugLog(_FLN, "cp:%s placing defense objective", name);

	return this;
}

//////////////////////////////////////////////////////////////////////
// Flag a CP as a supply center and push it onto the queue

void
CPStruct::setSupplyCenter()
{
	if ( m_isSupplyCenter )
		return;

	m_isSupplyCenter = true;
	suppliers.insert(this);
}


//////////////////////////////////////////////////////////////////////
// On entering or exiting contention, process all of the table states.

void
CPStruct::setTableStates(bool isNewObjective)
{
	FacilityStruct* fac;

	dbugLog(_FLI, "contentionTables(%s) : %s", name, ((isContested())?"con":"uncon"));

	for ( fac = facilityList; fac != nullptr; fac = fac->next )
	{
		if ( fac->canBeCaptured() == false )
			continue;

		if ( !isObjective() )
		{
			// No objective, no tables
			dbugLog(_FLD, "NORB hiding %s non-obj table", fac->name);
			fac->setCaptureDisabled();

			// If the facility is spawnable and closed, time to wake it up
			if ( fac->m_spawnable && !fac->isOpen() )
			{
				dbugLog(_FLI, "opening %s since no objective", fac->name);
				strtOpenBase(*fac);
			}
			continue;
		}

		if ( fac->isMilitaryFacility() )
		{
			// Military tables will be hiding, however if we've just
			// entered contention, they should redraw soon
			if ( isContested() )
			{
				dbugLog(_FLD, "RB-MF table %s", fac->name);
				fac->setCapturePending();
			}
			else
			{
				dbugLog(_FLD, "NORB-MF hiding %s", fac->name);
				fac->setCaptureDisabled();
			}
		}
		else if ( isNewObjective )
		{
			dbugLog(_FLD, "RB show %s table no radio", fac->name);
			fac->setCapturePending();
		}
	}
}


//////////////////////////////////////////////////////////////////////
// Initiate the expiry of an attack objective

void
CPStruct::startObjectiveExpiry(WWII_SIDE forSide, UINT32 delay)
{
	cnvyScheduleObjectiveExpiry(this, forSide, delay);

#ifdef STRAT_ANNOUNCE_OBJECTIVES
	chatSendSideMessage(forSide, nullObjID, LSTR_NONE, LSTR_OBJECTIVE_WITHDRAWING, name);
#endif

	m_clearing[forSide] = true;
}

//////////////////////////////////////////////////////////////////////
// Iterate across the supply centers and propagate supply links outward
// from them to all same-side CPs within reach

bool
strtBuildSupplyChains(bool force)
{
	if ( supplyStabilized && force == false )
		return false;

	supplyStabilized = true;

	RoutingPropagator propagator;

	teulSyncClock();
	const TeulTimeMS started = teulClock64;

	propagator.Run(suppliers);
	teulSyncClock();
	const TeulTimeMS runEnded = teulClock64;

	propagator.Apply(strtGetCPList());

	teulSyncClock();
	const TeulTimeMS applyEnded = teulClock64;
	dbugLog(_FLI, "sup: supply routeing took %" PR64BitUT "ms [%" PR64BitUT " run, %" PR64BitUT " apply]", teulClock64 - started, runEnded - started, applyEnded - runEnded);

	return true;
}

bool
CPStruct::hasContactWithDivision(const StratHCUnit* unit) const
{
	const StratHCUnit* candidate;

	if ( unit->type() == HCU_BRIGADE )
		candidate = unit->parent();
	else if ( unit->type() == HCU_DIVISION )
		candidate = unit;
	else
		return false;

	for (; candidate != nullptr; candidate = candidate->next() )
	{
		if ( candidate->unitID() == unit->unitID() || candidate->homeCP() == nullptr )
			continue;

		if ( candidate->homeCP() == this )
		{
			return true;
		}

		if ( linkTo(candidate->homeCP()) != nullptr )
		{
			return true;
		}
	}

	return false;
}

static UINT32 _getValue(const HCUnit& unit)
{
	UINT32 base = (unit.isBrigade() ? 1 : 3);
	switch ( unit.movementType() )
	{
	case BRANCH_AIR:
		return base + 1;
	case BRANCH_SEA:
		return base + 2;
	default:
	case BRANCH_LAND:
		return base;
	}
}

//////////////////////////////////////////////////////////////////////
// Check all links for firebases

static inline bool
_updateFirebases(const HCUnit& unit, CPStruct* cp)
{
	if ( cp == nullptr )
		return false;
	if ( unit.movementType() == BRANCH_LAND && unit.branch() == BRANCH_LAND && (unit.isBrigade() == true || unit.isGarrison() == true) )
		return true;
	if ( cp->isEnemyLinked() && cp->m_units.empty() == true )
		return true;
	return false;
}

//////////////////////////////////////////////////////////////////////
// Add a HCUnit to the unit list

void
CPStruct::addUnit(HCUnit& unit)
{
	m_units[unit.unitID()] = &unit;
	m_value += _getValue(unit);

	if ( _updateFirebases(unit, this) )
		processFirebases(false);
}

//////////////////////////////////////////////////////////////////////
// Remove a HCunit from the unit list

void
CPStruct::removeUnit(const HCUnit& unit)
{
	if ( m_units.find(unit.unitID()) != m_units.end() )
		m_value -= _getValue(unit);
	m_units.erase(unit.unitID());

	if ( _updateFirebases(unit, this) == true )
		processFirebases(true);
}

//////////////////////////////////////////////////////////////////////
// Is a particular HCUnit in this city?

bool
CPStruct::hasHCUnit(UINT32 unitID) const
{
	if ( m_units.empty() )
		return false;

	HCUNIT_INDEX::const_iterator it;
	for ( it = m_units.begin(); it != m_units.end(); ++it )
	{
		if ( it->first == unitID )
			return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////
// Reset the routeing values for a particular CP

void
CPStruct::resetRouteing()
{
	if ( (isCity() == false) | (numLinks == 0) | (country == WWII_CHINA) )
	{
		m_isRouteingConnected = true;
		return;
	}

	setRouteing(false);
}

void
CPStruct::setRouteing(bool isConnected)
{
	if ( isConnected == m_isRouteingConnected )
		return;

	// We need to rebuild the links again
	supplyStabilized = false;

	m_isRouteingConnected = isConnected;

	if ( m_isRouteingConnected )
	{
		flags |= CP_STAT_IN_SUPPLY;
		restartSupplies();
	}
	else
		flags &= ~CP_STAT_IN_SUPPLY;

#ifdef IS_STRAT_HOST
	strtQueueCPStatus(this);
#endif
}


void CPStruct::removeAllGarrisonsSupply()
{
	HCUNIT_INDEX unitList = m_units;
	HCUNIT_INDEX::iterator it;
	for (it = unitList.begin(); it != unitList.end(); ++it)
	{
		StratHCUnit* unit = (StratHCUnit*)it->second;
		if (unit->isGarrison())
		{
			unit->purgeResupply(true);
			unit->toeTemplateHeader()->unsubscribe(unit, unit->toe());
		}
	}
}


void CPStruct::changeAllGarrisonCountries(WWII_COUNTRY countryID, bool manualChange)
{

	HCUNIT_INDEX unitList = m_units;
	HCUNIT_INDEX::iterator it;
	for (it = unitList.begin(); it != unitList.end(); ++it)
	{
		StratHCUnit* unit = (StratHCUnit*)it->second;
		if (unit != nullptr)
		{
			if (unit->isGarrison())
			{
				std::string toeTemplateHeaderName = unit->getToeTemplateHeaderName();
				unit->m_countryID = GarrisonCountryChangeReq::getReplacementCountry(countryID, unit);

				std::string countryPrefix;

				if (unit->m_countryID == WWII_ENGLAND)
				{
					countryPrefix = "uk";
				}
				else if (unit->m_countryID == WWII_USA)
				{
					countryPrefix = "us";
				}
				else if (unit->m_countryID == WWII_FRANCE)
				{
					countryPrefix = "fr";
				}
				else if (unit->m_countryID == WWII_GERMANY)
				{
					countryPrefix = "de";
				}

				// Replace the first 2 letters in order to change the country.
				toeTemplateHeaderName.replace(0, 2, countryPrefix);

				// Get the new toeTemplate for the unit and assign it.
				TOE_::TemplateHeader* toeTemplate(TOE_::TemplateHeader::Lookup(toeTemplateHeaderName.c_str()));

				if (toeTemplate)
				{
					strtClearTOESaveList(*unit);
                    // Clear all supply.
                    //unit->purgeResupply(true);
					// Remove the old toe template
					unit->toeTemplateHeader()->unsubscribe(unit, unit->toe());
					// Set the new countries toe template
					unit->setToeTemplateHeader(toeTemplate);
					// Subscribe to the template and have it populate our vehicle list
					// Note: This gives us "default" equipment levels (i.e. full)
					toeTemplate->subscribe(unit, unit->toe());
					// Clear all supply.
					//unit->purgeResupply(true);
					// Reset supply and announce the country change to map, chat and clients.
					unit->announceGarrisonCountryChange();
					// Restart supply with trickle timer.
					unit->restartGarrisonSupply(manualChange);
				}

				unit->save();
			}
		}
	}

}

StratHCUnit * CPStruct::getGarrisonHCUnit(BRANCH_ENUM branchID, BRANCH_ENUM movementType)
{
	HCUNIT_INDEX unitList = m_units;
	HCUNIT_INDEX::iterator it;
	for (it = unitList.begin(); it != unitList.end(); ++it)
	{
		StratHCUnit* unit = (StratHCUnit*)it->second;
		if (unit->branch() == branchID && unit->movementType() == movementType && unit->isGarrison())
		{
			return unit;
		}
	}
	return nullptr;
}

void CPStruct::disableAllGarrisons()
{
	HCUNIT_INDEX unitList = m_units;
	HCUNIT_INDEX::iterator it;
	for (it = unitList.begin(); it != unitList.end(); ++it)
	{
		StratHCUnit* unit = (StratHCUnit*)it->second;
		if (unit != nullptr)
		{
			if (unit->isGarrison())
			{
				unit->setDisabled(true);
				dbugLog(_FLW, "Garrison %s has been disabled"
					, unit->name());
			}
		}
	}
}


void CPStruct::enableAllGarrisons()
{
	HCUNIT_INDEX unitList = m_units;
	HCUNIT_INDEX::iterator it;
	for (it = unitList.begin(); it != unitList.end(); ++it)
	{
		StratHCUnit* unit = (StratHCUnit*)it->second;
		if (unit != nullptr)
		{
			if (unit->isGarrison())
			{
				unit->setDisabled(false);
				dbugLog(_FLW, "Garrison %s has been enabled"
					, unit->name());
			}
		}
	}
}