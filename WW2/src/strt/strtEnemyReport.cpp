#include <syscfg.h> // Path is project specific.
// Date: Fri Nov  3 13:52:34 2000
//
// These are the messages that the host side sends and receives.  This file
// contains all the setups and such nonsense for this bit of fun.
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Marty "Vandal" Poulin

#include "versionNo.h"
#include "dbug/dbug.h"

#include "chat/chatGCIMessage.h"
#include "chat/chatGCITypes.h"
#include "chat/chatSupportRPCMessages.h"
#include "chat/chatTypes.h"
#include "strt/strtCPStruct.h"
#include "strt/strtData.h"
#include "strt/strtDB.h"
#include "strt/strtFacilityStruct.h"
#include "strt/strtHostMessages.h"
#include "strt/strtPostedWeaponList.h"
#include "strt/strtServerTypes.h"
#include "strt/strtTypes.h"
#include "teul/teulTime.h"
#include "weap/weapWeapon.h"
#include "wwii/wwiiConfig.h"

UINT32 ewsHighThreshold[MAX_WEAP_CATEGORY];

static const UINT32 EWS_MIN_TIMEOUT = 60;			// ~1 minute
static const UINT32 EWS_SHORT_TIMEOUT = 120;		// ~2 minutes
static const UINT32 EWS_MAIN_TIMEOUT = 4 * 60;		// ~4 minutes

typedef std::set<UINT16> CHANGED_CPS;
static CHANGED_CPS _changedCPs;

/*
class EWSScoreCompare
{
	bool operator(const CPStruct* p1, const CPStruct* p2) const
	{
		return (p1->m_players[0][0] > p2->m_players[0][0]);
	}
};
*/

#ifdef MAINTAIN_ACTIVE_EWS_LIST
typedef std::set<CPStruct*> ACTIVE_EWS;
static ACTIVE_EWS g_activeEws;
#endif

//////////////////////////////////////////////////////////////////////
// Add a new enemy report

bool
strtAddEnemyReport(const STRAT_ENEMY_REPORT& report)
{
	FacilityStruct* facility;
	facility = strtGetFacility(report.reportingFacility);
	if (facility == nullptr)
		return false;

	facility->cp->enemyReport(report.enemyReported, facility);

	return true;
}

//////////////////////////////////////////////////////////////////////
// Add a CP to the queue

void
_queueUpdateForCP(UINT16 oid)
{
	CHANGED_CPS::iterator it = _changedCPs.lower_bound(oid);
	if ( it == _changedCPs.end() || *it != oid )
		_changedCPs.insert(it, oid);
}

//////////////////////////////////////////////////////////////////////
// Evalute all the CPs that have pending updates

void
strtEvaluateEWSUpdates()
{
	// Have any CPs changed?
	if ( _changedCPs.empty() )
		return;

	dbugLog(_FLD, "evaluating EWS at %" PRSizeT " CPs", _changedCPs.size());

	// Go through the list and evaluate EWS at each CP, but
	// do a simple bulk "clear" at the end rather than
	// faffing around deleting each individually.
	CHANGED_CPS::iterator it;
	CPStruct* cp;
	for ( it = _changedCPs.begin(); it != _changedCPs.end(); ++it )
	{
		cp = strtGetCPByOID(*it);
		if ( cp != nullptr )
			cp->evaluateEWS();
	}

	_changedCPs.clear();
}

//////////////////////////////////////////////////////////////////////
// Evaluate what weapon category a particular vehicle should be treated
// as - e.g. treat trucks as "ground" units (aka infantry)
///TODO: Use facility in the differentiation?

static UINT8
_effectiveCategory(const WEAPON_SERIAL& weapon)
{
	WEAPON* entry = weapLookup(weapon.spec);
	if ( entry->alertDist <= 0 )
		return 0;

	if ( weapon.spec.wCategory == _GROUND_VEHICLE )
	{
		// Non-tank ground vehicles behave as "ground units" and not armor
		static WEAPON_SPEC tankSpec;
		if ( (*(UINT32*)&tankSpec) == 0)
			tankSpec = weapFindClass("tank");

		if ( weapon.spec.wClass != tankSpec.wClass )
			return _MUNITIONS;
	}

	return weapon.spec.wCategory;
}

//////////////////////////////////////////////////////////////////////
// CP function for registering an incoming EWS report to a specific
// facility

bool
CPStruct::enemyReport(const WEAPON_SERIAL& weapon, FacilityStruct* facility)
{
	assert(facility != nullptr);

	if ( weapon.serial <= 0 )
	{
		dbugLog(_FLA, "ews:invalid weapon serial submitting enemy report");
		return false;
	}

	UINT8 category = _effectiveCategory(weapon);
	if ( category == 0 )
	{
		dbugLog(_FLD, "ews: %s doesn't register EWS", weapPrintSpecC(&weapon.spec));
		return false;
	}
	if ( category != weapon.spec.wCategory )
	{
		dbugLog(_FLD, "ews: serial %d is category %d treating as %d"
						, weapon.serial
						, weapon.spec.wCategory
						, category
						);
	}

	// Now we know it's positive, save to uint32 it
	UINT32 serial = (UINT32)weapon.serial;

	POSTED_WEAPON* culprit = strtFindPostedWeapon(serial);
	if ( culprit != nullptr )
	{
		culprit->m_lastEWSFacility = facility;
	}
	else
		dbugLog(_FLD, "EWS report at %s for non-posted weapon %d", facility->name, serial);

	EWSREPORTS::iterator i = ews.find(serial);
	UINT8 weaponSide = weapon.spec.side();

	// Existing entry - this is an update or alteration
	if ( i != ews.end() )
	{
		dbugLog(_FLD, "ews: update from %d at %s", serial, facility->name);
		i->second->update(teulTime(), facility);
	}
	else
	{
		// New entry
		dbugLog(_FLD, "ews: EWS report from %d at %s", serial, facility->name);

		// If the ews list is currently empty, this is the first report
		bool wasFirstReport = ews.empty();

		ews[serial] = new EWSReport(serial, facility, weaponSide, category);

		// Immediate notice to non-frontline towns
		if ( wasFirstReport || !isEnemyLinked() )
			evaluateEWS();
		else
			_queueUpdateForCP(uOID);
	}

	dbugLog(_FLD, "ews: %d units reporting at %s now", facility->m_players[0] , facility->name);

	return true;
}

//////////////////////////////////////////////////////////////////////
// Check if the EWS numbers merrit a change in EWS level

static UINT32
_ewsMask(CPStruct* const cp
			, const UINT32 flags
			, const WWII_SIDE enemy
			, const WWII_SIDE friendly
			, const WEAPON_CATEGORY category
			, const UINT32 lowFlag
			, const UINT32 highFlag
			, const GCI_TYPE lowAlert
			, const GCI_TYPE highAlert
			, const bool annotate
			)
{
	UINT32 enemies = cp->m_players[enemy][category];
	if ( annotate )
		dbugLog(_FLI, "players[%u][%u] = %u", enemy, category, enemies);

	if ( category == _INFANTRY )
	{
		const UINT32 mun = cp->m_players[enemy][_MUNITIONS];
		if ( mun > 0 )
		{
			enemies += mun;
			if ( annotate )
				dbugLog(_FLI, "adding[%u][%u]:%u => %u", enemy, _MUNITIONS, mun, enemies);
		}
	}

	if ( enemies < (UINT32)host_config.minEWSTrip )
	{
		if ( annotate )
			dbugLog(_FLI, "%u enemies < %u requirement for EWS", enemies, host_config.minEWSTrip);
		return flags & ~(lowFlag|highFlag);
	}

	UINT32 newFlag = flags | lowFlag;

	if ( category == _INFANTRY )
	{
		const UINT32 mun = cp->m_players[friendly][_MUNITIONS];
		if ( mun > 0 )
		{
			enemies += mun;
			if ( annotate )
				dbugLog(_FLI, "adding friendly [%u][%u]:%u => %u", friendly, _MUNITIONS, mun, enemies);
		}
	}

	if ( ewsHighThreshold[category] > 0 && enemies >= (UINT32)ewsHighThreshold[category] )
	{
		if ( annotate )
			dbugLog(_FLI, "%u enemies, %u required for high, check!", enemies, ewsHighThreshold[category]);

		if ( (newFlag & highFlag) != 0 )
		{
			dbugLog(_FLI, "already flagged");
			return newFlag;
		}

		dbugLog(_FLI, "ews: %s reached high alert state for category %d", cp->name, category);

		if ( highAlert != GCI_NONE )
			chatSendGCIMessage(highAlert, friendly, *cp, nullObjID);

		return (newFlag | highFlag);
	}

	if ( annotate )
		dbugLog(_FLI, "%u enemies, %u required for high, fail", enemies, ewsHighThreshold[category]);

	if ( !(flags & lowFlag) )
	{
		dbugLog(_FLI, "ews: %s reached low alert state for category %d", cp->name, category);

		// If this flag wasn't set before
		if ( lowAlert != GCI_NONE )
			chatSendGCIMessage(lowAlert, friendly, *cp, nullObjID);
	}

	// Exclude the high-flag bits
	return (newFlag & ~highFlag);
}

void
CPStruct::evaluateEWS(bool annotate)
{
	WWII_SIDE enemy;
	if ( side == ALLIED )
		enemy = AXIS;
	else if ( side == AXIS )
		enemy = ALLIED;
	else
	{
		dbugLog(_FLW, "ews: Cannot evaluate EWS for neutral CP %s", name);
		return;
	}

	UINT32 newFlags = this->flags;

	if ( this->m_players[WWII_NO_SIDE][0] == 0 )
	{
		if ( annotate )
			dbugLog(_FLI, "evaluateEWS: nobody here");
		// Majority case: nobody here; a worthwhile shortcut
		newFlags &= ~CP_EWS_FLAG_MASK;
	}
	else
	{
		newFlags = _ewsMask(this, newFlags, enemy, side
							, _INFANTRY
							, CP_STAT_ENEMY_INFANTRY, CP_STAT_HEAVY_ENEMY_INF
							, GCI_CP_ENEMY_GROUND_SPOTTED, GCI_NONE
							, annotate
							);

		newFlags = _ewsMask(this, newFlags, enemy, side
							, _GROUND_VEHICLE
							, CP_STAT_ENEMY_ARMOR, CP_STAT_HEAVY_ENEMY_ARMOR
							, GCI_CP_ENEMY_ARMOR_SPOTTED, GCI_NONE
							, annotate
							);

		newFlags = _ewsMask(this, newFlags, enemy, side
							, _AIRCRAFT
							, CP_STAT_ENEMY_AIRCRAFT, CP_STAT_HEAVY_ENEMY_AIR
							, GCI_NONE, GCI_NONE
							, annotate
							);

		newFlags = _ewsMask(this, newFlags, enemy, side
							, _SEA_VEHICLE
							, CP_STAT_ENEMY_SEACRAFT, CP_STAT_HEAVY_ENEMY_SEA
							, GCI_CP_ENEMY_SEA_SPOTTED, GCI_NONE
							, annotate
							);
	}

	if ( newFlags == flags )
	{
		if ( annotate )
			dbugLog(_FLI, "evaluateEWS: no change in flags %x", flags);
		return;
	}

	dbugLog(_FLI, "ews: %s flags changed from %x -> %x", name, flags, newFlags);

	// Update the flags if they need to be updated and push the
	// broadcast that requires.
	_changeFlags(newFlags);

	// If any of the EWS flags are set, they're under attack
	checkForUnderAttack();
}

//////////////////////////////////////////////////////////////////////
// Determine if an EWS report has expired

bool
EWSReport::hasExpired(UINT32 now) const
{
	UINT32 age = now - m_time;

	// Short timeout is the soonest they could expire
	if ( age < EWS_MIN_TIMEOUT )
		return false;

	// If they have despawned then use the short timeout
	///NOTE: Don't cache this value, it doubles as a test of
	/// whether or not the vehicle is still spawned. OGS
	POSTED_WEAPON* culprit = strtFindPostedWeapon(m_weapon);
	if ( culprit == nullptr )
		return (age >= EWS_SHORT_TIMEOUT);

	// If they have reported EWS at someplace else, move along
	if ( culprit->m_lastEWSFacility != m_facility )
		return true;

	return (age >= EWS_MAIN_TIMEOUT);
}

//////////////////////////////////////////////////////////////////////
// Eliminate old EWS reports; it may prove better to create a global
// expiry list that we can just continually drain, using a queue;
// When we do that: to avoid ongoing tear up/tear down times, let the
// entries hit the bottom and get moved back up to the top.

void
CPStruct::expireEnemyReports(UINT32 now)
{
	if ( ews.empty() || ews.size() == 0 )
		return;				// Nothing to process

	dbugLog(_FLD, "ews: expiring at %s", name);

	EWSREPORTS::iterator cur, nextIt;

	for ( cur = nextIt = ews.begin(); cur != ews.end(); cur = nextIt )
	{
		// Deletion invalidates the current iterator; so, we use
		// a skip iterator (next) to take us forward
		++nextIt;

		if ( cur->second->hasExpired(now) )
		{
			// Destructor will clean up for itself
			delete cur->second;
			ews.erase(cur);
			_queueUpdateForCP(uOID);
		}
	}

	// Ifthis list is empty now, free up any memory associated with it
	if ( ews.empty() || ews.size() == 0 )
	{
		ews.clear();
		evaluateEWS();
	}
}

//////////////////////////////////////////////////////////////////////
// Bean counting functions which modify the facility counters.
// The constructor and destructor modify the CP counters, because
// an EWS report can't move between CPs.

void
EWSReport::_incPresence()
{
	// We use WWII_NO_SIDE to hold cross-side totals
	m_facility->m_players[WWII_NO_SIDE]++;
	m_facility->m_players[m_side]++;
}

void
EWSReport::_decPresence()
{
	assert(m_facility->m_players[WWII_NO_SIDE] > 0);
	m_facility->m_players[WWII_NO_SIDE]--;
	assert(m_facility->m_players[m_side] > 0);
	m_facility->m_players[m_side]--;
}

static void
_maintainActiveEWSList(const CPStruct* cp)
{
#ifdef MAINTAIN_ACTIVE_EWS_LIST
	if ( cp->m_players[ALLIED][0] > host_config.uMinEWSTrip
		&& cp->m_players[AXIS][0] > host_config.uMinEWSTrip )
		g_activeEWS.insert(cp);
	else
		g_activeEWS.erase(cp);
#endif
}

EWSReport::EWSReport(UINT32 weaponSerial, FacilityStruct* facility, UINT8 side, UINT8 weapCategory)
	: m_weapon(weaponSerial)
	, m_time(teulTime())
	, m_facility(facility)
	, m_side(side)
	, m_weapCategory(weapCategory)
{
	register CPStruct* cp = m_facility->cp;
	assert(cp != nullptr);

	// Adjust the facility counters
	_incPresence();

	// Update the "total total"
	cp->m_players[WWII_NO_SIDE][0]++;

	// Update the total/category counter
	cp->m_players[WWII_NO_SIDE][m_weapCategory]++;

	// Update the side/all-category counter
	cp->m_players[m_side][0]++;

	// Update the side/category counter
	cp->m_players[m_side][m_weapCategory]++;

	_maintainActiveEWSList(cp);
}

EWSReport::~EWSReport()
{
	register CPStruct* cp = m_facility->cp;
	assert(cp != nullptr);

	dbugLog(_FLD, "ews: deleting report for %d at %s", m_weapon, cp->name);

	// Drop the facility values
	_decPresence();

	// See constructor for what these array positions mean
	assert(cp->m_players[WWII_NO_SIDE][0] > 0);
	assert(cp->m_players[WWII_NO_SIDE][m_weapCategory] > 0);
	assert(cp->m_players[m_side][0] > 0);
	assert(cp->m_players[m_side][m_weapCategory] > 0);

	cp->m_players[WWII_NO_SIDE][0]--;
	cp->m_players[WWII_NO_SIDE][m_weapCategory]--;
	cp->m_players[m_side][0]--;
	cp->m_players[m_side][m_weapCategory]--;

	_maintainActiveEWSList(cp);
}

//////////////////////////////////////////////////////////////////////
// Update an existing EWS entry - the player may have changed facility

void
EWSReport::update(UINT32 now, FacilityStruct* facility)
{
	// Always update the time stamp
	m_time = now;

	// We need to shuffle bean counts if the player has moved facility
	if ( facility != m_facility )
	{
		dbugLog(_FLD, "ews: update for %d moving %s->%s", m_weapon, m_facility->name, facility->name);

		// Decrease counters at prior facility
		_decPresence();

		// Relocate
		m_facility = facility;

		// Bump the new counters
		_incPresence();
	}
}

///TODO: Add function for periodically saving the EWS entries from various
///TODO: CPs and Facilities in run CP via the dbse.
///TODO: Make a note of a player's last EWS facility, and then
///TODO: remove them when they despawn (to stop EWS spamming by depots)

