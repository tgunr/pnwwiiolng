#include <syscfg.h> // Path is project specific.
// Copyright (C) PlayNet Inc 2011 -- All Rights Reserved.
// Originating Programmer: Oliver "kfsone" Smith
// Implement the functionality behind Scoring::Death

#include "chat/chatLStringIDs.h"
#include "chat/chatSupportRPCMessages.h"
#include "inst/instTypes.h"
#include "map/mapPlayer.h"
#include "score/scoreDeath.h"
#include "score/scoreKillNotice.h"
#include "score/scoreSortie.h"
#include "weap/weapDefs.h"
#include "wwii/wwiiConfig.h"
#include "wwii/wwiiThreads.h"

//////////////////////////////////////////////////////////////////////
// Player killed themselves.

static void
_doSuicide(const Player& idiotPlayer, const Scoring::Sortie& idiotSortie)
{
#ifdef BETA_RELEASE
	///TODO:134:REMOVE
	char explanation[81];
	sprintf(explanation, "%s killed by self or AI.", idiotPlayer.callsign());
	chatSendBroadcastAll(LSTR_BETA, LSTR_GENERIC, explanation);
#endif
}

//////////////////////////////////////////////////////////////////////
// Notify people near a death of the death.

static void
_areaKillNotice(const Scoring::Sortie& victim, const Scoring::Sortie& killer)
{
/*
#if !defined(CHINA) && !defined(HAPPYOO)
	if ( ! host_config.kills.notifyKiller )
		return;
	if ( victim.spec().wCategory != _INFANTRY && victim.spec().wCategory != _GROUND_VEHICLE )
	{
		dbugLog(_FLD, "v:%u no kill notice: not inf/grd", victim.sortieID()); ///TODO:134:REMOVE
		return;
	}

	const Player* const activePlayer = victim.activePlayer();
	if ( activePlayer == NULL )
	{
		dbugLog(_FLD, "v:%u no kill notice: no active player", victim.sortieID()); ///TODO:134:REMOVE
		return;
	}

	char victimName[MAX_HANDLE_SIZE];
	char killerName[MAX_HANDLE_SIZE];
	chatGetCallsign(victimName, victim.chatHandle());
	chatGetCallsign(killerName, killer.chatHandle());

	char kill[128];
	const char* fmts[7] =
	{
		"%s was killed by %s."
	,	"%s laid to rest by %s."
	,	"%s bought the farm from %s."
	,	"Waaugh! %s just got dropped by %s."
	,	"%s got whacked by %s."
	,	"%s fell victim to %s."
	,	"%s was just cut down by %s."
	};

	const char* const fmt = fmts[rand()%7];
	snprintf(kill, 127, fmt, victimName, killerName);

	chatEmitMessageFromPlayer(activePlayer->playerID(), LSTR_CAUTION, LSTR_GENERIC, kill, "");
#endif
*/
}

//////////////////////////////////////////////////////////////////////
// Notify a player he got a kill.

static void
_playerKillNotice(const Scoring::Sortie& victim, const Scoring::Sortie& killer, const bool friendlyFire)
{
	if ( host_config.kills.notifyKiller != 2 )
		return;

	char vehicleName[MAX_HANDLE_SIZE];
#if defined(BETA_RELEASE)
	chatGetCallsign(vehicleName, killer.chatHandle());
	if ( victim.pilot() )
		victim.pilot()->message(LSTR_BETA, LSTR_KILL_AWARD_TO, vehicleName);
	if ( victim.crew() )
		victim.crew()->message(LSTR_BETA, LSTR_KILL_AWARD_TO, vehicleName);
#endif

	// Who done it?
	chatGetCallsign(vehicleName, victim.chatHandle());

	const LSTRING_ID messageID = friendlyFire ? LSTR_FRIENDLY_FIRE : LSTR_KILL_CREDIT;
	if ( killer.pilot() )
		killer.pilot()->message(LSTR_NOTE, messageID, vehicleName);
	if ( killer.crew() )
		killer.crew()->message(LSTR_NOTE, messageID, vehicleName);
}

//////////////////////////////////////////////////////////////////////
// Generate kill-a-rat notices.

static void
_doKillARat(const Scoring::Sortie& victim, const Scoring::Sortie& killer, bool friendlyFire)
{
	if ( friendlyFire || ! host_config.kills.reportRats )
		return;

	const bool ratKiller = killer.isPlaynet();
	const bool ratVictim = victim.isPlaynet();

	if ( !ratKiller && !ratVictim )
		return;

	const LSTRING_ID msg = (ratKiller ? (ratVictim ? LSTR_RAT_KILLED_RAT : LSTR_KILLED_BY_DEV) : LSTR_KILLED_A_DEV);

	char victimName[MAX_HANDLE_SIZE];
	char killerName[MAX_HANDLE_SIZE];
	chatGetCallsign(victimName, victim.chatHandle());
	chatGetCallsign(killerName, killer.chatHandle());

	chatSendBroadcastAll(LSTR_SYSTEM, msg, killerName, victimName);
}

static FLOAT32
_calculateKillValue(const Scoring::Sortie& killer, const Scoring::Sortie& victim)
{
	if ( killer.value() == 0.0f )
		return 0.0f;

	const bool isFriendlyFire = killer.side() == victim.side();
	if ( isFriendlyFire && !host_config.kills.deductFriendly )
		return 0.0f;

	const FLOAT32 maxValue = (FLOAT32)host_config.kills.maxValue;
	const FLOAT32 value = std::min(victim.value() / killer.value(), maxValue);

	static const FLOAT32 KILL_VALUE_TO_POINTS = 10.0f;
	return KILL_VALUE_TO_POINTS * (( isFriendlyFire == false ) ? value : -value);
}

//////////////////////////////////////////////////////////////////////
// Give the actual scoring kill credit, database record, etc.

static void
_creditKill(Player* const killer, Scoring::Sortie& killerSortie, const Scoring::Actor& victim, const Scoring::Sortie& victimSortie, const bool fromCapture)
{
	if ( killer == NULL )
		return;

	killerSortie.addKill(victimSortie.sortieID(), victimSortie.spec());

	// Determine how much this kill is worth by comparing vehicles.
	FLOAT32 killValue = _calculateKillValue(killerSortie, victimSortie);
	///TODO:SCORING: Adjust for balance.

	if ( killValue == 0.0f )
		return;

	++ killer->missionPacket().scoring().kills;
	killer->missionPacket().m_killScore += killValue;

	// Multiplier for converting a raw kill value into points.
	if ( victim.inCaptureBldg() )
		++ killer->missionPacket().scoring().cappingKills;

	if ( killerSortie.spec().wCategory == _INFANTRY )
	{
		if (fromCapture )
			++ killer->missionPacket().scoring().guardingKills;
	}
}

//////////////////////////////////////////////////////////////////////
// Give point/count credit to a player for an assist.

static bool
_awardAssistToPlayer(Scoring::Sortie& victimSortie, Player* const player, UINT32 sortieID, const char* const victimName, const char* const killerName, const FLOAT32& value)
{
	if ( player == NULL )
		return false;
	if ( player->sortieID() != sortieID )
		return false;

	player->message(LSTR_NOTE, LSTR_ASSIST_AWARD, victimName, killerName);

	++ player->missionPacket().scoring().assists;

	// Assist = half score.
	player->missionPacket().m_killScore += value;

	return true;
}

//////////////////////////////////////////////////////////////////////
// Give credit for assists.

static void
_awardAssists(Scoring::Sortie& deadSortie, Scoring::Sortie& killerSortie)
{
	// No hits (e.g. death by building)
	if ( deadSortie.hitsTaken().empty() )
		return;

	if ( ! host_config.kills.notifyKiller )
		return;

	char killerName[MAX_HANDLE_SIZE];
	char victimName[MAX_HANDLE_SIZE];
	chatGetCallsign(killerName, killerSortie.chatHandle());
	chatGetCallsign(victimName, deadSortie.chatHandle());

	std::set<UINT32> assists;

	auto& hitsTaken = deadSortie.hitsTaken();
	const TeulTimeMS window = host_config.kills.assistWindow_s * 1000;
	for ( auto it = hitsTaken.rbegin(); it != hitsTaken.rend(); ++it )
	{
		auto& entry = *it;
		const TeulTimeMS delta = teulClock64 - entry.m_timestamp;
		if ( delta >= window )
			break;
		if ( entry.sortieID() == killerSortie.sortieID() )
			continue;
		if ( assists.find(entry.sortieID()) != assists.end() )
			continue;

		Scoring::Sortie* const assistSortie = Scoring::Sortie::Lookup(entry.sortieID());
		if ( assistSortie == NULL )
			continue;

		// The more recent your damage, the more score you get, with a minimum of 1/4 value
		// and a maximum of 90% value if you *just* hit the vehicle.
		const FLOAT32 killValue = std::min(1.0f, _calculateKillValue(killerSortie, deadSortie));
		static const FLOAT32 modifier = 10.0f;
		const FLOAT32 value = killValue / (modifier + ((float)delta / ((float)window / modifier)));
		if ( !_awardAssistToPlayer(deadSortie, assistSortie->pilot(), assistSortie->sortieID(), victimName, killerName, value) )
			continue;
		_awardAssistToPlayer(deadSortie, assistSortie->crew(), assistSortie->sortieID(), victimName, killerName, value);

		assists.insert(assistSortie->sortieID());
	}
}

//////////////////////////////////////////////////////////////////////
// Process a player's death and translate into kill/assist awards.

bool
Scoring::Death::process(Player* const lhsPlayer)
{
	dbugLog(_FLIB, " death");	///TODO:134:REMOVE

	// We need to look up the sortie on which the player died.
	Scoring::Sortie* const deadSortie = FindSortieForPlayer(lhsPlayer, lhs().sortieID());
	if ( deadSortie == NULL )
		return false;

	// Suicide.
	if ( rhs().sortieID() == lhs().sortieID() || (rhs().sortieID() == 0 && deadSortie->lastHitBySortieID() == 0) )
	{
		_doSuicide(*lhsPlayer, *deadSortie);
		return true;
	}

	Sortie* killerSortie = NULL;
	const UINT32 statedKillerID = rhs().sortieID();
	if ( statedKillerID != 0 )
	{
		// rhs = sortieID of killer as a hint.
		killerSortie = Sortie::Lookup(statedKillerID);
	}

	// Do we need to try the last person to hit us?
	const TeulTimeMS lastHitCutoff = teulClock64 - 60 * 1000;	// 60 seconds ago.
	if ( killerSortie == NULL && (deadSortie->lastHitBySortieID() != 0 && deadSortie->lastHitTime() >= lastHitCutoff) )
	{
		killerSortie = Sortie::Lookup(deadSortie->lastHitBySortieID());
	}

	if ( killerSortie == deadSortie )
	{
		_doSuicide(*lhsPlayer, *deadSortie);
		return true;
	}

	if ( killerSortie == NULL )
	{
		dbugLog(_FLW, "p:%u c:%s v:%u killer sortie #%u not found"
						, lhs().playerID(), Chat::Name(lhs().handle()).c_str(), lhs().sortieID()
						, statedKillerID
						);
		return false;
	}

	dbugLog(_FLD, "found killer sortie: %u. deadSortie=%p killerSortie=%p", killerSortie->sortieID(), deadSortie, killerSortie);

	const bool friendlyFire = (deadSortie->side() == killerSortie->side());

	const WEAPON_SPEC& killerSpec = killerSortie->spec();
	const WEAPON_SPEC& deadSpec = deadSortie->spec();
	DBThread::Do(DBThread::Default
		, "CALL wwii.p_add_kill"
			"(%u, %u,%u,%u,%u, %u,%u,%u,%u, %u, %u,%u,%u,%u, %u,%u,%u,%u, 0, %d,%d,%d)"
			, killerSortie->sortieID()
			, 0 /*enemy->m_crew[0]*/, 0 /*enemy->m_personas[0]*/, 0 /*enemy->m_crew[1]*/, 0 /*enemy->m_personas[1]*/
			, killerSpec.wCountry, killerSpec.wCategory, killerSpec.wClass, killerSpec.wType
			, deadSortie->sortieID()
			, deadSortie->pilotID(), 0 /*deadVehicle->m_personas[0]*/, deadSortie->crewID(), 0 /*deadVehicle->m_personas[1]*/
			, deadSpec.wCountry, deadSpec.wCategory, deadSpec.wClass, deadSpec.wType
			, deadX(), deadY(), deadZ()
			);

	_areaKillNotice(*deadSortie, *killerSortie);
	_playerKillNotice(*deadSortie, *killerSortie, friendlyFire);

	_doKillARat(*deadSortie, *killerSortie, friendlyFire);

	_creditKill(killerSortie->pilot(), *killerSortie, lhs(), *deadSortie, rhs().inCaptureBldg());
	_creditKill(killerSortie->crew(), *killerSortie, lhs(), *deadSortie, rhs().inCaptureBldg());

	if ( lhsPlayer->sortie() == deadSortie )
		lhsPlayer->missionPacket().vehicle().killedBy = killerSortie->chatHandle();

	_awardAssists(*deadSortie, *killerSortie);

	return true;
}

