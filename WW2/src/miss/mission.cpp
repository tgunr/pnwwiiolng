#include <syscfg.h> // Path is project specific.
// Created: Thu Feb  1 16:09:08 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Brian "Stick" Duelm

#include "versionNo.h"

#include "chat/chatBroadcast.h"
#include "chat/chatHandle.h"
#include "chat/chatSupportMessages.h"
#include "chat/chatSupportRPCMessages.h"
#include "inst/instTypes.h"					// For the enums
#include "map/mapBrigades.h"
#include "map/mapPlayer.h"
#include "miss/missionExtMission.h"
#include "miss/mission.h"
#include "miss/missionHostMessages.h"
#include "miss/missionMission.h"
#include "miss/missionPacket.h"
#include "miss/missionTypes.h"
#include "miss/missLeaderChange.h"
#include "mm/mmMain.h"
#include "server/serverInterface.h"
#include "strt/strtCPStruct.h"
#include "strt/strtData.h"
#include "strt/strtFacilityStruct.h"
#include "strt/strtLinkStruct.h"
#include "strt/strtMapMessages.h"
#include "teul/teulUtils.h"
#include "include/wthrTime.h"
#include "wwii/wwiiConfig.h"
#include "wwii/wwiiExceptions.h"
#include "wwii/wwiiHostMessages.h"
#include "wwii/wwiiSerialNumber.h"
#include "wwii/wwiiUTF.h"

#include "include/logging.h"

typedef std::map<UINT32,Mission*> MissionIndex;

typedef std::vector<Mission*> InstanceMissions;
typedef std::map< instanceid_t, InstanceMissions > InstanceMissionIndex;

extern const std::string& sdmTranslateName(const std::string& from);

// Use the self-serializing SerialNumber class to
// track mission IDs across a campaign.
static SerialNumber missionSerial("missionid", MIN_MISSION_ID, 100, MAX_MISSION_ID);

const char* _missionTypes[MAX_MISSION_TYPES] =
{
	"POINT_ATTACK"
,	"POINT_DEFENSE"
,	"AREA_ATTACK"
,	"AREA_DEFENSE"
,	"RECON"
,	"RESUPPLY"
,	"SYSTEM_MISSION"
};

#ifdef OUTPUT_FACILITIES
FILE * blahFile;
#endif

static WEAPON_SPEC weap_truck;
static WEAPON_SPEC weap_bomber;

static MissionIndex g_missions;			// Missions by ID
static MissionIndex g_postedMissions;		// Missions by Player
static InstanceMissionIndex g_instanceMissionIndex;

// ------------------- private functions --------------------

static void
_roomFillEntry(RoomListEntry& entry, RMLIST_ACTION action, const Player* player)
{
	entry.m_action = action;
	entry.m_handle = player->chatHandle();
	if ( player->hasVehicle() )
	{
		entry.m_weapon = player->weapon().spec;
		entry.m_serial = player->weapon().serial;
	}
	else
	{
		entry.m_weapon = NULL_WEAP_SPEC;
		entry.m_serial = 0;
	}
	entry.m_flags = player->roomListFlags();
	entry.m_rank = player->effectiveRank();
}

//////////////////////////////////////////////////////////////////////
// Determines if the facility is a valid origin for the listed branch/brigade

static bool
_validBrigadeOrigin(const HCUnit* unit, const FacilityStruct* facility)
{
	const CPStruct* cp = facility->cp;
	if ( cp == nullptr )
	{
		dbugLog(_FLA, "no cp");
		return false;
	}

	if ( facility->side != unit->side() )
	{
		dbugLog(_FLI, "%s/%s side mismatch", facility->name, unit->name());
		return false;
	}

	const CPStruct* bcp = strtGetCP(unit->homeCPID());
	if ( bcp )
		dbugLog(_FLI, "XXX posting mission at %s:%s for brigade %s@%s units %" PRSizeT ":%" PRSizeT ""
						, facility->name, cp->name
						, unit->name(), bcp->name
						, cp->m_units.size(), bcp->m_units.size());
	else
		dbugLog(_FLI, "XXX can't find brigade home");

	if ( unit->canUseOrigin(facility->type) == false )
	{
		dbugLog(_FLIB, " CANT USE ORIGIN");
		return false;
	}

	// We know this depot isn't in the same town as the brigade, so is it
	// a combat depot origin?
	if ( facility->getSupplyingUnit(unit, bcp) == 0 )
	{
		dbugLog(_FLIB, " NO SUP UNIT");
		return false;
	}

	return true;
}

bool
missionValidBrigadeOrigin(const FacilityStruct* facility, UINT32 brigadeID)
{
	if ( facility == nullptr || brigadeID == 0 )
	{
		dbugLog(_FLA, "invalid facility/brigade lookup");
		return false;
	}

	const HCUnit* unit = HCUnit::Lookup(brigadeID);
	if ( unit == nullptr )
	{
		dbugLog(_FLI, "XXX can't find brigade %u", brigadeID);
		return false;
	}

	return _validBrigadeOrigin(unit, facility);
}

//////////////////////////////////////////////////////////////////////
// Check all missions at a particular CP for ongoing validity

void
missionCheckOrigins(CPStruct* cp)
{
	assert( nullptr != cp );

	Mission* current;
	Mission* next;

	for ( current = cp->m_missionsFrom; current != nullptr; current = next )
	{
		next = current->m_nextFromOrigin;
		current->checkOrigin();
	}
}

bool
Mission::checkOrigin()
{
	if ( !ongoing() || !brigade() )
		return true;
	if ( _validBrigadeOrigin(brigade(), originFac()) )
		return true;
	chatSendBroadcast(Chat::_ORDERS, m_instanceID, missionID(), LSTR_ORDERS, LSTR_NO_LINK_TO_BRIGADE);
	cancel();

	return false;
}

//////////////////////////////////////////////////////////////////////
// When an FB is destroyed, tell anyone who is on a destroy mission

void
missionFirebaseDestroyed(const FacilityStruct& facility)
{
	const Mission* current;
	for ( current = facility.cp->m_missionsTo; current != nullptr; current = current->m_nextToTarget )
	{
		if ( current->numTakers() <= 0 )
			continue;
		if ( current->originFac() == &facility || current->targetFac() == &facility )
			chatSendBroadcast(Chat::_ORDERS, current->instanceID(), current->missionID(), LSTR_ORDERS, LSTR_OBJECT_DESTROYED, facility.name);
	}

	for ( current = facility.cp->m_missionsFrom; current != nullptr; current = current->m_nextFromOrigin )
	{
		if ( current->numTakers() <= 0 )
			continue;
		if ( current->originFac() == &facility || current->targetFac() == &facility )
			chatSendBroadcast(Chat::_ORDERS, current->instanceID(), current->missionID(), LSTR_ORDERS, LSTR_OBJECT_DESTROYED, facility.name);
	}
}

//////////////////////////////////////////////////////////////////////
// All missions at this facility are no-longer takeable until they
// retake the facility

void
missionRevokeMissionsFrom(const FacilityStruct& facility)
{
	if ( facility.cp->m_missionsTo == nullptr )
		return;

	Mission* current;
	Mission* next;

	for ( current = facility.cp->m_missionsFrom; current != nullptr; current = next )
	{
		next = current->m_nextFromOrigin;

		if ( current->originFac()->uOID != facility.uOID )
			continue;
		if ( current->side() != facility.side )
			continue;
		if ( !current->ongoing() )
			continue;

		if ( current->numTakers() > 0 && facility.type != FACILITY_FIREBASE )
		{
			chatSendBroadcast(Chat::_ORDERS, current->instanceID(), current->missionID(), LSTR_ORDERS, LSTR_BASE_CAPTURED, facility.name);
		}

		current->cancel();
	}
}

//////////////////////////////////////////////////////////////////////
// see if a person can post at a specific priority

bool
Player::ableToPost() const
{
	if ( brigadeID() == 0 )
		return false;

	return ableToPostIn(*brigade());
}

bool
Player::ableToPostIn(const Brigade& toBrigade) const
{
	// If you have POST_MISSIONS permission, ahead you go
	if ( permissions(POST_MISSIONS) )
		return true;

	// If you don't have brevet posting permission,
	// then you won't be able to post regardless
	if ( permissions(BREVET_POSTING) == false )
		return false;

	// You can't post here unless there's an officer present
	if ( toBrigade.officers() < 1 )
		return false;

	return true;
}

//////////////////////////////////////////////////////////////////////
// Mission constructor: It's very simple because several things
// construct missions, so anyone doing so will have to call all
// the right stuff...

Mission::Mission(const instanceid_t& forInstanceID, const MISSION_SUMMARY& withSummary)
	: m_takers(), m_spawner()
{
	dbugLog(_FLD, "ALLOC: +mission at %p in:%llu", this, forInstanceID);
	memset((MISSION_C_STRUCTURE*)this, 0, sizeof(MISSION_C_STRUCTURE));
	m_instanceID = forInstanceID;
	m_summary = withSummary;
}

//////////////////////////////////////////////////////////////////////
// Set this mission as the player's current active mission

static void
_postPlayerPosting(Mission* mission)
{
	g_postedMissions[mission->playerID()] = mission;
}

//////////////////////////////////////////////////////////////////////
// Release a player posting

static void
_releasePlayerPosting(const Mission* mission)
{
	MissionIndex::iterator it = g_postedMissions.find(mission->playerID());
	if ( it != g_postedMissions.end() && it->second == mission )
		g_postedMissions.erase(it);
}

//////////////////////////////////////////////////////////////////////
// Determine if a player has a mission in the posted list

Mission*
Player::activeMission() const
{
	MissionIndex::iterator it = g_postedMissions.find(playerID());
	return ( it == g_postedMissions.end() ) ? nullptr : it->second;
}

//////////////////////////////////////////////////////////////////////
// Remove a mission from the instance index

static void
_removeMissionFromInstanceIndex(const Mission* mission)
{
	InstanceMissionIndex::iterator it = g_instanceMissionIndex.find(mission->instanceID());
	if ( it == g_instanceMissionIndex.end() )
		return;

	for ( size_t i = 0; i < it->second.size(); ++i )
	{
		if ( it->second[i] == mission )
		{
			it->second[i] = it->second.back();
			it->second.pop_back();
			break;
		}
	}

	// If we just removed the last column, delete this row.
	if ( it->second.empty() )
		g_instanceMissionIndex.erase(it);
}

//////////////////////////////////////////////////////////////////////
// Mission destructor - free up all the stuff we know that we might
// have allocated/registered

Mission::~Mission()
{
	if ( missionID() > 0 )
	{
		dbugLog(_FLD, "m:%u posted:%d[%s] country:%d branch:%d side:%d brig:%d[%s] -REL"
						, missionID()
						, playerID()
						, Chat::Name(m_poster).c_str()
						, country()
						, branch()
						, side()
						, (brigade()) ? brigade()->brigadeID() : 0
						, (brigade()) ? brigade()->name() : "<none>"
						);

		purgeTakers();
		removeFromCPLists();

		if ( brigade() != nullptr )
			brigade()->removeMission(this);

		g_missions.erase(missionID());
		_releasePlayerPosting(this);

		// Tell chat it can discard waypoints
		chatSendMissionDeletionNotice(missionID());

		// Remove from the instance list and cleanup
		_removeMissionFromInstanceIndex(this);
	}

	dbugLog(_FLD, "ALLOC: -mission at %p", this);
}

//////////////////////////////////////////////////////////////////////
// Activate a mission to indicate it's takeable, etc

void
Mission::activate()
{
	dbugLog(_FLI, "m:%u activating", missionID());

	bool wasActive = m_isActive;

	m_isActive = m_isOngoing = true;
	m_deactivatedAt = 0;

	if ( brigade() != nullptr && !isSystemMission() )
		brigade()->activateMission();

	sendLaunchUpdates(!wasActive);
}

//////////////////////////////////////////////////////////////////////
// Return the branch of a mission

BRANCH_ENUM
Mission::branch() const
{
	return (nullptr != brigade()) ? brigade()->branch() : BRANCH_LAND;
}

//////////////////////////////////////////////////////////////////////
// Deactivate a mission but leave it 'ongoing' to be canceled if
// the leader doesn't rejoin in a few minutes

void
Mission::deactivate()
{
	if ( isSystemMission() )
		return;

	dbugLog(_FLI, "m:%u deactivating", missionID());

	bool wasActive = m_isActive;

	m_isActive = false;
	m_isOngoing = true;

	if ( brigade() != nullptr )
		brigade()->deactivateMission();

	m_deactivatedAt = teulTime();

	bool forceUpdate = (wasActive != m_isActive);
	sendLaunchUpdates(forceUpdate);
}

//////////////////////////////////////////////////////////////////////
// Add a persona to a mission

bool
Mission::addPlayer(Player* player)
{
	if ( player == nullptr )
	{
		dbugLog(_FLA, "NULL persona in addPlayer");
		return false;
	}
	if ( player->playerID() <= DEFAULT_PLAYER_ID )
	{
		dbugLog(_FLW, "t:%d p:%u c:%s illegal playerID", player->teulIndex(), player->playerID(), player->callsign());
		return false;
	}

	m_takers[player->playerID()] = player;

	return true;
}

//////////////////////////////////////////////////////////////////////
// Determine if the mission is accessible to a persona

MISSION_CODE
Mission::accessibility(Player* player) const
{
	if ( !player->personaID() )
		return MC_BRIGADE;

	// You can't take the mission once it's pending
	if ( !ongoing() )
		return MC_EXPIRED;

	if ( !active() && player->missionID() != missionID() && player->playerID() != playerID() )
		return MC_EXPIRED;

	if ( player->brigadeID() != brigade()->brigadeID() )
		return MC_BRIGADE;

	if ( player->branch() != branch() )
		return MC_BRANCH;

	if ( m_originFac == nullptr || m_originFac->side != side() )
		return MC_CAPTURED;

	if ( player->side() != side() )
		return MC_CAPTURED;

	return MC_JOIN;
}

//////////////////////////////////////////////////////////////////////
// Flag a mission for removal once it's players leave

void
Mission::cancel()
{
	if ( isMobileSpawnAvailable() )
	{
		betaLog(_FLI, "m:%u cancel killing mobile spawn", missionID());
		deactivateMobileSpawn(MSDR_MISSION_EXPIRED);
	}

	if ( isSystemMission() )
		return;

	if ( active() || ongoing() )
	{
		dbugLog(_FLI, "m:%u being cancelled", missionID());

		if ( active() && brigade() != nullptr )
			brigade()->deactivateMission();

		m_isActive = m_isOngoing = false;
		_releasePlayerPosting(this);

		sendLaunchUpdates(true);
	}
}

//////////////////////////////////////////////////////////////////////
// Transfer the leadership of this mission

void
Mission::changeLeadership(Player* player)
{
	UINT32 oldLeaderID = m_playerID;
	_releasePlayerPosting(this);

	if ( player == nullptr )
	{
		player = PlayerInterface::Lookup(m_backupLeader);
		if ( player == nullptr || player->missionID() != missionID() || player->playerID() == playerID() )
		{
			dbugLog(_FLD, "m:%u no new leadership, deactivating", missionID());
			deactivate();
		}
		return;
	}

	m_playerID = player->playerID();
	m_poster = player->chatHandle();
	chatGetCallsign(summary().callsign, player->chatHandle());

	// Ok - mission now belongs to you
	_postPlayerPosting(this);

	player->SetPersonaMissionFlags();
	player->updateMissionTaker(false);
	player->updateChat();

	player = PlayerInterface::Lookup(oldLeaderID);
	if ( player != nullptr && player->missionID() == missionID() )
	{
		player->SetPersonaMissionFlags();
		player->updateMissionTaker(false);
		player->updateChat();
	}

	if ( !active() )
		activate();

	m_backupLeader = 0;

	markForUpdate();
}

//////////////////////////////////////////////////////////////////////
// Initialize "helper" variables within the mission structure

bool
Mission::initializeHelpers(WWII_SIDE withSide, Brigade* withBrigade)
{
	if ( isValidSide(withSide) == false )
	{
		///TODO: Find a better way to cope with this
		m_side = AXIS;
	}
	else
		m_side = withSide;

	m_originFac = strtGetFacility(summary().origin);
	if ( m_originFac != nullptr )
	{
		summary().origin = m_originFac->fullObjID;
		m_originCP = m_originFac->cp;
	}
	else
	{
		m_originCP = strtGetCPorBridge(summary().origin);
		if ( m_originCP != nullptr )
			summary().origin = m_originCP->fullObjID;
	}

	m_targetFac = strtGetFacility(summary().target);
	if ( m_targetFac != nullptr )
	{
		summary().target = m_targetFac->fullObjID;
		m_targetCP = m_targetFac->cp;
	}
	else
	{
		m_targetCP = strtGetCPorBridge(summary().target);
		if ( m_targetCP != nullptr )
			summary().target = m_targetCP->fullObjID;
	}

	if ( m_originCP == nullptr )
	{
		dbugLog(_FLA, "can't create mission with invalid origin " PRF_FULL_OBJ_ID ""
						, PRA_FULL_OBJ_ID(summary().origin));
		return false;
	}

	if ( m_targetCP == nullptr )
	{
		m_targetFac = m_originFac;
		m_targetCP = m_originCP;
	}

	dbugLog(_FLI, "Initialized Mission Helpers: origin(" PRF_FULL_OBJ_ID ") = %s, target(" PRF_FULL_OBJ_ID ") = %s"
					, PRA_FULL_OBJ_ID(summary().origin)
					, m_originCP->name
					, PRA_FULL_OBJ_ID(summary().target)
					, m_targetCP->name
					);

	m_linkFromTarget = m_targetCP->linkTo(m_originCP);

	m_brigadeID = withBrigade->unitID();
	m_brigade = withBrigade;
	m_country = withBrigade->country();

	m_poster = chatGetHandle(summary().callsign);

	return true;
}

//////////////////////////////////////////////////////////////////////
// Determine if the mission is currently considered "frontline"

bool
Mission::isFrontLine() const
{
	return ( m_originCP != nullptr && (m_originCP->isEnemyLinked() || m_originCP->isContested()) );
}

//////////////////////////////////////////////////////////////////////
// Let listeners know that this missions state has changed

void
Mission::markForUpdate()
{
	if ( brigade() != nullptr )
		brigade()->updateMissionList();
}

//////////////////////////////////////////////////////////////////////
// Remove all the takers from a mission list

void
Mission::purgeTakers()
{
	// Any takers to remove?
	if ( m_takers.empty() )
		return;

	dbugLog(_FLI, "m:%u removing all %u takers", summary().missionID, numTakers());
	while ( !m_takers.empty() )
	{
		if ( !removePlayer(m_takers.begin()->first, false) )
		{
			LogAbort("mission %d: unable to remove %d from takerlist", summary().missionID, m_takers.begin()->first);
			return;
		}
	}
}

//////////////////////////////////////////////////////////////////////
// Decouple this mission from the mission lists

void
Mission::removeFromCPLists()
{
	if ( !m_registered )
		return;

	// Remove it from the CP lists
	if ( m_nextFromOrigin != nullptr )
		m_nextFromOrigin->m_prevFromOrigin = m_prevFromOrigin;
	if ( m_prevFromOrigin )
		m_prevFromOrigin->m_nextFromOrigin = m_nextFromOrigin;
	else
		m_originCP->m_missionsFrom = m_nextFromOrigin;

	// And the target list
	if ( m_nextToTarget )
		m_nextToTarget->m_prevToTarget = m_prevToTarget;
	if ( m_prevToTarget )
		m_prevToTarget->m_nextToTarget = m_nextToTarget;
	else
		m_targetCP->m_missionsTo = m_nextToTarget;

	m_registered = false;
}

//////////////////////////////////////////////////////////////////////
// Remove a single persona record from a mission taker list

bool
Mission::removePlayer(UINT32 withPlayerID, bool announce)
{
	if ( withPlayerID == m_backupLeader )
		m_backupLeader = 0;

	PLAYER_MAP::iterator it = m_takers.find(withPlayerID);
	if ( it == m_takers.end() )
	{
		dbugLog(_FLW, "p:%u m:%u player not on mission list to extract", withPlayerID, missionID());
		return false;
	}

	Player* player = it->second;

	m_takers.erase(it);

	if ( player == nullptr )
	{
		dbugLog(_FLA, "p:%u m:%u player has NULL persona on mission list", withPlayerID, missionID());
		return false;
	}

	dbugLog(_FLI, "t:%d p:%u c:%s %s extracted from mission %u (%p) leaving %d members"
					, player->teulIndex(), player->playerID(), player->callsign()
					, ((player->isMissionLeader()) ? "LEADER" : "taker")
					, summary().missionID, this, numTakers());

	if ( announce )
		updateTaker(player, RMLIST_DELETE, true);

	if ( active() && player->isMissionLeader() )
	{
		if ( m_takers.empty() )
			cancel();
		else if ( m_backupLeader == 0 )
			deactivate();
		else
		{
			it = m_takers.find(m_backupLeader);
			if ( it == m_takers.end() || it->second->missionID() != missionID() )
			{
				dbugLog(_FLI, "m:%u bl:%u is no-longer a member", missionID(), backupLeader());
			}
			else
				changeLeadership(it->second);
		}

	}

	player->unsetMission();

	return true;
}

//////////////////////////////////////////////////////////////////////
// Send mission timer/mobile spawn updates to all mission listeners

void
Mission::sendLaunchUpdates(bool force)
{
	const bool allowMobileSpawn = isMobileSpawnAllowed();
	if ( allowMobileSpawn != summary().m_canMobileSpawn )
	{
		force = true;
		summary().m_canMobileSpawn = allowMobileSpawn;
	}

	bool launchable = true;

	if ( force )
		markForUpdate();

	// Use one time value throughout
	UINT32 now = teulTime();

	// Determine whether or not the timer needs to be changed
	if ( summary().timeLaunch != 0 )
	{
		UINT32 launchWindow = now;
		UINT32 nextLaunch = now + host_config.mission.launchCycle_s;

		// Launchable depends on whether or not the timer has lapsed
		launchable = (summary().timeLaunch < nextLaunch);

		// If we're employing launch windows, adjust the launch timers as we go.
		if ( nextLaunch > now && summary().timeLaunch < launchWindow )
			summary().timeLaunch = nextLaunch;
	}
	else
		launchable = true;

	if ( numTakers() == 0 )
		return;

	// Notify all the players on the mission list
	const bool isActive = active();
	const bool isOngoing = ongoing();
	const bool hasMsp = isMobileSpawnAvailable();
	const UINT32 id = missionID();
	for ( auto it = m_takers.begin(); it != m_takers.end(); )
	{
		Player* const player = it->second;
		++it;
		if ( player == nullptr || player->isAtLobby() == false )
			continue;

		bool goForLaunch = false;
		if ( player->m_nextLaunch <= (UINT32)teulTime() && launchable )
		{
			player->m_launchPermitted = true;
			goForLaunch = true;

		}
		if ( (force | goForLaunch) )
		{
			sendToClientMissTimeUpdate(player->teulIndex()
								, id
								, player->nextLaunchTime()
								, hasMsp
								, allowMobileSpawn
								, isActive
								, isOngoing
								);
		}
	}
}

//////////////////////////////////////////////////////////////////////
// Notify everyone in a mission room of an update to an individual taker

void
Mission::updateTaker(Player* player, RMLIST_ACTION action, bool excludeSelf)
{
	RoomList oneList;
	Player* current;

	if ( numTakers() == 0 )
		return;

	player->updateRoomListFlags();

	oneList.m_numEntries = 1;
	_roomFillEntry(oneList.m_entry[0], action, player);

	PLAYER_MAP::iterator it;
	for ( it = m_takers.begin(); it != m_takers.end(); ++it )
	{
		current = it->second;
		if ( current == nullptr )
			continue;

		if ( excludeSelf && current->playerID() == player->playerID() )
			continue;

		if ( current->isConnected() )
		{
			dbugLog(_FLD, "t:%d p:%u c:%s sending mission update for %s; isSpawned:%s"
							, current->teulIndex(), current->playerID(), current->callsign(), player->callsign(), ((oneList.m_entry[0].m_flags & RMLIST_SPAWNED) ? "yes" : "no"));
			sendToClientRoomList(current->teulIndex(), oneList);
		}
		else
		{
			dbugLog(_FLD, "t:%d p:%u c:%s not connected for mission update for %s; isSpawned:%s"
							, current->teulIndex(), current->playerID(), current->callsign(), player->callsign(), ((oneList.m_entry[0].m_flags & RMLIST_SPAWNED) ? "yes" : "no"));
		}
	}
}

//////////////////////////////////////////////////////////////////////
// Clear out the volatile (non-persistant) members of the mission packet
void
MissionPacket::flushVolatiles()
{
	discardSortieData();

	sortie().originFac = nullObjID;
	sortie().target = nullObjID;
	vehicle().rtbFacility = nullObjID;
	m_missionType = RECON;
	rtbReservedWeapon = NULL_WEAP_SERIAL;
}

//////////////////////////////////////////////////////////////////////
// Did it meet minimum time requirements?

bool
MissionPacket::successfulDuration() const
{
	return (duration_s() >= MINIMUM_TIME_FOR_MISSION_SUCCESS);
}

//////////////////////////////////////////////////////////////////////
// missionSuccessful()
//
// did they successfully complete their mission?
//
// for 1.2.5, this is simply, did they rtb and damage someone?

bool
MissionPacket::successful(const WEAPON_SPEC& weaponSpec) const
{
	if ( weaponSpec.wType == 0 )
		return false;

	const UINT8 rtb = vehicle().rtbState();
	if ( m_missionType == RESUPPLY )
		return ( rtb == RTB_SUCCESS && m_vehicleResupplied == true );

	if ( !successfulDuration() )
		return false;

	if ( rtb != RTB_SUCCESS && rtb != RTB_BAIL_FRIENDLY )
		return false;

	if ( scoring().kills() > 0 )
		return true;

	// Vehicles that don't neccessarily get a kill
	if ( weaponSpec.wClass == weap_bomber.wClass || weaponSpec.wClass == weap_truck.wClass )
		return true;

	return false;
}

// Tell players what time their mission will launch at.
static void
_sendTimeUpdates(UINT32 now)
{
	MissionIndex::iterator it;
	Mission* mission;

	UINT32 timeout = now - (host_config.mission.timeOut_m * 60);

#if defined(DEV_BUILD)
	if ( g_logDebug )
		betaLog(_FLD, "sendTimeUpdates");
#endif

	// We may delete the mission while we're iterating, so make sure
	// the iterator iterates before we obliterate
	for ( it = g_missions.begin(); it != g_missions.end(); )
	{
		mission = it->second;
		++it;

#if defined(DEV_BUILD)
		if ( g_logDebug )
		{
			betaLog(_FLDB, " #%u[act:%u,og:%u,nt:%u]", mission->missionID(), mission->active(), mission->ongoing(), mission->numTakers());
		}
#endif

		if ( mission->spawner().isActive() )
		{
			MSPAWN_RESULT result = mission->getMobileSpawnPermission();
			if ( result != MSDR_GRANTED )
				mission->deactivateMobileSpawn(result);
		}

		if ( mission->active() )
		{
			if ( mission->summary().timeLaunch != 0 )
				mission->sendLaunchUpdates(false);
		}

		if ( mission->isSystemMission() )
			continue;

		// On the event server the mission can be inactive because the
		// mission leader hasn't yet logged in. This may present
		// some interesting challenges to the leader hand-off system.

		if ( !mission->ongoing() && mission->numTakers() == 0 )
		{
			dbugLog(_FLI, "m:%u empty, no takers, deleting", mission->missionID());

			// We're thru with this mission - so be gone!
			delete mission;
			continue;
		}

		if ( !mission->active() && mission->ongoing() && mission->m_deactivatedAt <= timeout )
		{
			dbugLog(_FLD, "m:%u inactive for %u seconds, cancelling", mission->missionID(), now - mission->m_deactivatedAt);
			mission->cancel();
		}
	}
}

// checkTimedOutMissions()
//
// run down the PGMList and mv any missions that are timed out to the
// timeoutlist.  this means people can no longer take them, but they are
// still "running" and people are on them
void
checkForTimedOutMissions()
{
	static TeulTimeMS nextCheck = 0;

	if ( g_missions.empty() )
		return;

	if ( teulClock64 >= nextCheck )
	{
#if defined(DEV_BUILD)
		dbugLog(_FLD, "%s: patrolling %" PRSizeT " missions", __FUNCTION__, g_missions.size());
#endif
		_sendTimeUpdates(teulTime());
		nextCheck = teulClock64 + 5000;
	}
}

//////////////////////////////////////////////////////////////////////
// Create a suitable mission description for this mission

static void
_populateMissionDescription(Mission& mission, const char* desc)
{
	//make a description
	if ( desc == nullptr )
		desc = "";
	else
	{
		// Look for non-whitespace characters; if we don't find
		// any then treat this as an empty string.
		const char* p = desc;
		while ( *p && WWII::BytesFromCharacter[(unsigned int)*p] == 1 && isspace(*p) )
			++p;
		if ( *p == 0 )
			desc = "";
	}

	// Size-restrained copy and size-tracking.
	mission.m_descSize = 0;
	while ( *desc && mission.m_descSize < MapMiss::MISSION_DESC_LENGTH )
	{
		mission.m_desc[mission.m_descSize++] = *(desc++);
	}
	mission.m_desc[mission.m_descSize] = 0;
}

//////////////////////////////////////////////////////////////////////
//

static Mission*
_createMission(instanceid_t instanceID
				, MISSION_SUMMARY& summary
				, WWII_SIDE side
				, WWII_COUNTRY country
				, const char* desc
				, CREATOR_TYPE creatorType
				, UINT32 playerID
				, UINT32 postingTime
				, UINT32 currentTime
				, Brigade* brigade
				, bool makeAvailable
				)
{
	Mission* mission = nullptr;

	try
	{
		dbugLog(_FLI, "create in:%llu mission type is %d[%s] side is %d[%s] country is %d[%s] by %s playerID %d"
					, instanceID
					, summary.missionType, _missionTypes[(int)summary.missionType]
					, side, _sideName[side]
					, country, _countryName[country]
					, _EXPAND_CREATOR_TYPE(creatorType), playerID
					);

		{
			dbugLog(_FLIB, " origin: " PRF_FULL_OBJ_ID ", targ: " PRF_FULL_OBJ_ID ""
							, PRA_FULL_OBJ_ID(summary.origin)
							, PRA_FULL_OBJ_ID(summary.target)
							);
		}

		if ( activeCountries[country] == false )
			throw std::invalid_argument("illegal country selection");

		if ( creatorType != PLAYER_GENERATED && creatorType != EXTERNALLY_GENERATED )
			throw std::invalid_argument("only PLAYER_GENERATED and EXTERNALLY_GENERATED mission types handled");

		if ( !isValidSide(side) )
			throw std::invalid_argument("can only create missions for ALLIED or AXIS");

		if ( brigade == nullptr )
			throw std::invalid_argument("can't create mission with NULL brigade");

		//set up space for the mission structure
		mission = new Mission(instanceID, summary);

#ifdef BETA_RELEASE
		if ( brigade == nullptr )
			LogAbort("managed to get past NULL brigade check");
#endif

		if ( ! mission->initializeHelpers(side, brigade) )
			throw std::runtime_error("unable to initialize mission helpers");	///Terminal

#ifdef BETA_RELEASE
		if ( brigade == nullptr )
			LogAbort("stack clobber?");
#endif

		mission->m_creationTime = postingTime;
		mission->m_playerID = playerID;
		mission->m_backupLeader = 0;

		if ( host_config.mission.minStartTime_s > 0
			&& mission->creatorType() == PLAYER_GENERATED
			&& mission->isFrontLine()
			)
		{
			UINT32 minStart = host_config.mission.minStartTime_s + currentTime;
			if ( mission->summary().timeLaunch < minStart)
			{
				mission->summary().timeLaunch = minStart;
			}
		}

		//do specific things for PLAYER_GENERATED missions
		//set the timeIn stuff
		mission->m_deactivatedAt = mission->m_timePost = teulTime();
//		mission->m_timeTimeout = mission->m_timePost + (host_config.mission.timeOut_m * 60);
//		mission->m_timeRemove = mission->m_timeTimeout + (host_config.mission.expire_m * 60);

		mission->m_isActive = makeAvailable;
		mission->m_isOngoing = true;

		mission->m_creatorType = creatorType;
		if ( creatorType != EXTERNALLY_GENERATED || mission->missionID() == 0 )
			mission->summary().missionID = static_cast<UINT32>(missionSerial.getNext());

		mission->registerToLists();

		_populateMissionDescription(*mission, desc);

		mission->summary().m_canMobileSpawn = mission->isMobileSpawnAllowed();

		//broadcast missions
#ifdef MULTIPLE_MISSION_HOSTS
		mmBroadcastMission(mission);
#endif

		dbugLog(_FLI,"created mission %p id %d", mission, mission->summary().missionID);
	}
	catch (std::exception& e)
	{
		dbugLog(_FLA, "unable to create mission: %s", e.what());
		if ( mission != nullptr )
			delete mission;
		return nullptr;
	}

	return mission;
}

Mission*
mapFindMission(UINT32 id)
{
	MissionIndex::iterator it = g_missions.find(id);
	if ( it == g_missions.end() )
		return nullptr;
	return it->second;
}

//////////////////////////////////////////////////////////////////////
// missionSendMissionPostReply()
//
// this function handles the MissionPostRequest that comes from the FE
// we doublecheck to make sure they can post, how many pts they have,
// if there's max # missions here, then create a mission for them, and send a
// reply
SINT32
missionSendMissionPostReply(MISSION_POST_REQ& request, Player* player, SINT32 connection)
{
	if ( player == nullptr )
	{
		dbugLog(_FLA, "NULL persona passed to %s()",__FUNCTION__);
		return -EINVAL;
	}

	player->ExitMission();

	if ( g_logDebug )
	{
		dbugLog(_FLD, "t:%d p:%u c:%s Mission Post: "
			"brig:%d mt:%d " PRF_FULL_OBJ_ID " -> " PRF_FULL_OBJ_ID " [%s]"
						, player->teulIndex(), player->playerID(), player->callsign()
						, player->brigadeID()
						, request.missionType
						, PRA_FULL_OBJ_ID(request.origin)
						, PRA_FULL_OBJ_ID(request.target)
						, request.desc
						);
	}

	FacilityStruct* postingFacility;
	UINT32 now = teulTime();

	MISSION_POST_REPLY reply;
	static MISSION_SUMMARY summary;

	memset(&summary, 0, sizeof(summary));

	reply.missionID = 0;
	reply.reasonCode = MC_ERROR;
	reply.posted = FALSE;

	summary.missionID = 0;
	summary.missionType = request.missionType;
	summary.subType = request.subType;
	summary.origin = request.origin;
	summary.target = request.target;
	summary.timeLaunch = 0;

	memcpy(summary.callsign, player->callsign(), sizeof(summary.callsign));

	postingFacility = strtGetFacility(request.origin);

	if ( player->hasActiveMission() )
	{
		player->log("Mission post denied: already has an active mission");
		player->message(LSTR_MISSION_POST_DENIED, LSTR_MPD_EXISTING_MISSION);
		reply.reasonCode = MC_BY_USER_EXISTS;
	}
	else if ( player->brigadeID() == 0 )
	{
		player->log("Mission post denied: no brigade selected");
		player->message(LSTR_MISSION_POST_DENIED, LSTR_MPD_NO_BRIGADE);
		reply.reasonCode = MC_BRIGADE;
	}
	else if ( !player->ableToPost() )
	{
//		if ( player->permissions(BREVET_POSTING) )
//		{
//			player->debug("Mission post denied", "Brigade no-longer active");
//			reply.reasonCode = MC_BRIGADE_INACTIVE;
//		}
//		else
//		{
			player->log("Mission post denied: no post permission");
			if ( !player->permissions(NO_ADS) && player->realRank() >= player->maxRank() )
				player->message(LSTR_MISSION_POST_DENIED, LSTR_MPD_RANK_AD);
			else
				player->message(LSTR_MISSION_POST_DENIED, LSTR_MPD_RANK_NO_AD);
			reply.reasonCode = MC_NO_PERMISSION;
//		}
	}
	else if ( postingFacility == nullptr )
	{
		player->log("Mission post denied: Invalid facility");
		char fidBuffer[FULL_OBJ_ID::PrintLen];
		player->message(LSTR_MISSION_POST_DENIED, LSTR_MPD_INVALID_FAC, weapPrintFullObjIDC(&request.origin, fidBuffer));
		reply.reasonCode = MC_ERROR;
	}
	else if ( postingFacility->side != player->side() || activeCountries[postingFacility->country] == false )
	{
		player->log("Mission post denied: Wrong facility ownership");
		player->message(LSTR_MISSION_POST_DENIED, LSTR_MPD_DONT_OWN);
		reply.reasonCode = MC_CAPTURED;
	}
	else if ( _validBrigadeOrigin(player->brigade(), postingFacility) == false )
	{
		player->log("Mission post denied: Facility not usable by brigade");
		player->message(LSTR_MISSION_POST_DENIED, LSTR_MPD_NON_BDE_ORIG);
		reply.reasonCode = MC_BRIGADE;
	}
	else
	{
		player->log("Mission post approved");
		player->message(LSTR_NOTE, LSTR_MISSION_POST_APPROVED);
		// Success
		reply.posted = TRUE;
		reply.reasonCode = MC_DETAIL;
	}

	//if any of the above cases matched, don't try to create, just return the
	//errorcode
	if( reply.posted == FALSE )
		return sendToClientMissionPostReply(reply, connection);

	//set up the mission
	Mission* mission = _createMission(player->instance().id(), summary, player->side(), player->country(), request.desc, PLAYER_GENERATED, player->playerID(), now, now, player->brigade(), false);

	if ( mission == nullptr )
	{
		dbugLog(_FLA, "t:%d p:%u c:%s failed to create mission", player->teulIndex(), player->playerID(), player->callsign());
		reply.posted=FALSE;
		reply.reasonCode = MC_ERROR;
	}
	else
	{
		reply.missionID = mission->missionID();
		reply.posted = TRUE;
	}

	return sendToClientMissionPostReply(reply, connection);
}


// missInit()
//
// set up the space for list of freeMissions, that we can grab from
void
missInit()
{
#ifdef OUTPUT_FACILITIES
	blahFile=fopen("/tmp/facilities","w");
#endif

	weap_truck = weapCreateSpec("GV_truck.1");
	weap_bomber = weapCreateSpec("AC_bomber.1");
}

//////////////////////////////////////////////////////////////////////
///TODO: Maintain an active room list per mission, rather than
/// having to build from scratch every time.
///CAVEAT: We don't check the player isn't already on this mission

bool
Player::enterMission(Mission* newMission)
{
	BETAInfo(teulIndex());

	if ( newMission == nullptr )
	{
		dbugLog(_FLA, "t:%d p:%u c:%s trying to add to NULL mission", teulIndex(), playerID(), callsign());
		return false;
	}

	if ( hasVehicle() )
		cancelWeapon(true);

	if ( !newMission->addPlayer(this) )
	{
		dbugLog(_FLA,"t:%d p:%u c:%s coudln't add to mission %d", teulIndex(), playerID(), callsign(), newMission->missionID());
		return false;
	}

	SetMission(newMission);

	sendFullMissionTakerList();

	return true;
}

//////////////////////////////////////////////////////////////////////
// Send me a full list of current mission population

void
Player::sendFullMissionTakerList()
{
	assert( mission() != nullptr );

	// Make sure we have a clear list
	clearMissionTakerList();

	// Set the room list flags
	updateRoomListFlags();

	// Entry used for announcing myself to people I visit
	RoomList myAnnouncement;
	myAnnouncement.m_numEntries = 1;
	_roomFillEntry(myAnnouncement.m_entry[0], RMLIST_ADD, this);

	// List of all the player's we're going to send to me
	RoomList currentRoomList;
	UINT32 i = 0;
	PLAYER_MAP::iterator it;
	for ( it = mission()->m_takers.begin(); it != mission()->m_takers.end(); ++it )
	{
		Player* const subject = it->second;
		if ( subject == nullptr )
			LogAbort("t:%d p:%u c:%s m:%u NULL persona in list for %u", teulIndex(), playerID(), callsign(), missionID(), it->first);

		if ( i >= MAX_ROOM_LIST_ENTRIES )
		{
			currentRoomList.m_numEntries = i;
			if ( sendToClientRoomList(teulIndex(), currentRoomList) < 0 )
				dbugLog(_FLW, "sendToClientRoomList failed");
			i = 0;
		}

		// Notify other players on the list that we have joined
		if ( subject->isConnected() && subject->playerID() != playerID() )
		{
			if ( sendToClientRoomList(subject->teulIndex(), myAnnouncement) < 0 )
				dbugLog(_FLW, "sendToClientRoomList failed");
		}

		// Now add this player to the temporary room list
		_roomFillEntry(currentRoomList.m_entry[i], RMLIST_ADD, subject);

		i++;
	}

	if ( i > 0 )
	{
		currentRoomList.m_numEntries = i;
		if ( sendToClientRoomList(teulIndex(), currentRoomList) < 0 )
			dbugLog(_FLW, "sendToClientRoomList failed");
	}
}

void
Mission::registerToLists()
{
	if ( mapFindMission(missionID()) )
	{
		dbugLog(_FLW, "mission: %d already registered", missionID());
		return;
	}

	if ( m_originCP == nullptr || m_targetCP == nullptr )
	{
		LogAbort("crap mission");
	}

	g_missions[missionID()] = this;

	if ( isSystemMission() )
		return;

	m_brigade->addMission(this);
	markForUpdate();

	m_nextFromOrigin = m_originCP->m_missionsFrom;
	if ( m_nextFromOrigin )
		m_nextFromOrigin->m_prevFromOrigin = this;
	m_originCP->m_missionsFrom = this;

	m_nextToTarget = m_targetCP->m_missionsTo;
	if ( m_nextToTarget )
		m_nextToTarget->m_prevToTarget = this;
	m_targetCP->m_missionsTo = this;

	if ( creatorType() == PLAYER_GENERATED )
		_postPlayerPosting(this);

	m_registered = true;
}

SINT32 missionSendMMCapacityRequest(Player* player, CAPACITY_REQUEST_STATE tag)
{
	if (PN_UNLIKELY(player == nullptr || !player->validMissionPacket()))
		return -1;

	BETAInfo(player->teulIndex());

	ENDPOINT* stratHost = serverPeerEndpointByType(STRAT_HOST);
	if (PN_UNLIKELY(stratHost == nullptr)) {
		dbugLog(_FLW, "unable to send capacity request: no strat host");
		return -1;
	}

	MissionPacket& mp = player->missionPacket();

	SINT32 retval = -1;
	switch ( mp.m_missionType )
	{
	case POINT_ATTACK:
	case AREA_ATTACK:
	case RESUPPLY:
	case SYSTEM_MISSION:
		//get capacity of target
		retval = strtSendMMCapacityRequest(mapGetStratIndex(), tag,
						player->missionID(),
						player->playerID(),
						mp.sortie().target(),
						(CODE_RTB)mp.vehicle().rtbState()
						);
	break;
	case POINT_DEFENSE:
	case AREA_DEFENSE:
		//get capacity of origin
		retval = strtSendMMCapacityRequest(mapGetStratIndex(), tag,
						player->missionID(),
						player->playerID(),
						mp.sortie().originFac(),
						(CODE_RTB)mp.vehicle().rtbState()
						);
	break;
	default:
		//some other mission type that we can't process
		dbugLog(_FLW, "cap request not sent: don't know how to process mission type %u", mp.m_missionType);
	break;
	}

	if ( retval >= 0 )
	{
		player->setRequestedCapacity(true);
		player->setReceivedCapacity(false);
	}

	return retval;
}

//////////////////////////////////////////////////////////////////////
// Based on a NEW_MISSION structure, do all the neccessary ground work to insert
// a mission into the system. Returns NULL if mission post failed.

Mission*
NewMission::post()
{
	static MISSION_SUMMARY summary;
	CPStruct* launchCP = nullptr;
	FacilityStruct* launchFac = nullptr;
	CPStruct* targetCP = nullptr;
	FacilityStruct* targetFac = nullptr;
	Brigade* brigade = nullptr;
	WWII_SIDE side = wwiiSide(country);

	try
	{
		if ( missionID > 0 && mapFindMission(missionID) != nullptr )
			throw std::domain_error("mission already posted");

		if ( instanceID == 0 )
			throw std::domain_error("mission has no instance specified");

		// Locate the origin and target
		launchCP = strtGetCPByOID(launchCP_oid);
		if ( launchCP == nullptr )
			throw std::invalid_argument("Unable to find origin cp");
		launchFac = strtGetFacilityByOID(launchFac_oid);
		if ( launchFac == nullptr )
			throw std::invalid_argument("Unable to find origin facility");

		if ( targetFac_oid == launchFac_oid )
		{
			targetCP = launchCP;
			targetFac = launchFac;
		}
		else
		{
			if ( targetCP_oid == launchCP_oid )
				targetCP = launchCP;
			else
			{
				targetCP = strtGetCPByOID(targetCP_oid);
				if ( targetCP == nullptr )
					throw std::invalid_argument("Unable to find target CP");
			}
			if ( targetFac_oid == 0 )
				targetFac = nullptr;
			else
			{
				targetFac = strtGetFacilityByOID(targetFac_oid);
				if ( targetFac == nullptr )
					throw std::invalid_argument("Unable to find target facility");
			}
		}

		if ( launchFac->side != side || activeCountries[launchFac->country] == false )
			throw std::domain_error("Mission/Facility country mismatch");

		brigade = mapFindBrigade(brigadeID);
		if ( brigade == nullptr )
			throw std::domain_error("Invalid brigade ID for mission");

		// Populate the summary structure
		summary.missionID = missionID;
		strsafecpy(summary.callsign, posterCallsign);
		summary.origin = launchFac->fullObjID;
		summary.target = (targetFac != nullptr) ? targetFac->fullObjID : targetCP->fullObjID;
		summary.missionType = missionType;
		summary.subType = 0;	///TODO: Fixme

		SINT32 now = teulTime();

		if ( launchAt == 0 || launchAt <= now )
			summary.timeLaunch = 0;
		else
			summary.timeLaunch = launchAt;

		Mission* mission;
		mission = _createMission(instanceID
						, summary
						, wwiiSide(country), country
						, description
						, EXTERNALLY_GENERATED
						, postedByID
						, postedAt
						, now
						, brigade
						, true
						);

		if ( mission != nullptr )
			mission->activate();

		return mission;
	}
	catch (std::exception& e)
	{
		dbugLog(_FLW, "mis:%d ctry:%u side:%u brig %d from %d/%d to %d/%d: %s"
						, missionID, country, side, brigadeID, launchCP_oid, launchFac_oid, targetCP_oid, targetFac_oid, e.what());
		return nullptr;
	}
}

#ifdef MULTIPLE_MISSION_HOSTS
void handleInboundMission(Mission* mission)
{
	if(mission==NULL)
	{
		dbugLog(_FLA, "%s() null ptr passed in ",__FUNCTION__);
		return;
	}

#error("We need to set up the brigade etc")
	Brigade* brigade = mapFindBrigade(mission->m_brigadeID);
	if ( !mission->initializeHelpers(mission->side, brigade) )
		return;

	if ( mission->summary().missionID >= nextMissionID )
		nextMissionID = mission->summary().missionID + 1;
	if ( nextMissionID >= maxMissionID )
		nextMissionID = minMissionID;

	_addMissionToAllLists(mission, mission->side());
	//well, that was easy, wasn't it?
}

void
missionCatchupServer(Server::ServerInfo* recipientServer)
{
	Mission* tempMission;

	if(recipientServer==NULL)
	{
		dbugLog(_FLA,"%s null ptr passed in", __FUNCTION__);
		return;
	}

	tempMission=PGMList;  //do they need the PGMTimeOutList?  I'll assume not, for now

	while(tempMission)
	{
		mmSendExistingMissionTo(tempMission, Server::BroadcastMaskAddress(recipientServer->stats.address) );
		tempMission=tempMission->next;
	}
}
#endif

SINT32
missionCancel(UINT32 missionID)
{
	Mission* mission = mapFindMission(missionID);
	if ( mission == nullptr )
	{
		dbugLog(_FLN, "m:%u no such mission for cancellation", missionID);
		return -EINVAL;
	}

	dbugLog(_FLI, "m:%u received deletion notification for mission", missionID);

	mission->cancel();

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Change the backup leader.

static void
_setBackupLeader(UINT32 missionID, UINT32 playerID)
{
	Mission* mission = mapFindMission(missionID);
	if ( mission == nullptr || mission->m_backupLeader == playerID )
		return;
	dbugLog(_FLD, "m:%u leader:%u got backup leader change to p:%u"
					, missionID, mission->playerID(), playerID);
	mission->m_backupLeader = playerID;
}

//////////////////////////////////////////////////////////////////////
// Process a leader or backup leader change

UINT32
LeaderChange::process() const
{
	if ( m_result == _SET_BACKUP_LEADER )
	{
		_setBackupLeader(m_missionID, m_leaderID);
		return _SUCCESS;
	}

	dbugLog(_FLI, "m:%u processing leader change to %s", m_missionID, Chat::Name(m_leaderHandle).c_str());

	// See if we know about this mission
	Mission* const mission = mapFindMission(m_missionID);
	if ( mission == nullptr )
		return _FAIL_NO_MISSION;

	if ( mission->isSystemMission() )
		return _FAIL_NO_MISSION;

	// Is this guy already leading?
	UINT32 oldLeaderID = mission->playerID();
	ChatHandle oldLeaderHandle = mission->posterHandle();

	if ( oldLeaderID == m_leaderID || oldLeaderHandle == m_leaderHandle )
		return _FAIL_IS_LEADER;

	Player* player = PlayerInterface::Lookup(m_leaderID);
	if ( player == nullptr || player->missionID() != m_missionID )
		return _FAIL_OTHER_MISSION;

	mission->m_backupLeader = m_leaderID;
	mission->changeLeadership(player);

	return _SUCCESS;
}

//////////////////////////////////////////////////////////////////////
// Check the instance->missions index to see if this country already
// has a stock mission posted for this country.

static Mission*
_lookupExistingInstanceMission(instanceid_t instanceID, WWII_COUNTRY forCountry, BRANCH_ENUM forBranch)
{
	// Start by looking for a matching mission
	InstanceMissionIndex::iterator it = g_instanceMissionIndex.find(instanceID);
	if ( it != g_instanceMissionIndex.end() && it->second.empty() == false )
	{
		for ( size_t i = 0; i < it->second.size(); ++i )
		{
			if ( it->second[i]->country() == forCountry && it->second[i]->branch() == forBranch )
				return it->second[i];
		}
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////
// Post a new mission for this country in this instance.

static Mission*
_postNewInstanceMission(instanceid_t instanceID, WWII_COUNTRY forCountry, BRANCH_ENUM forBranch)
{
	static const char* adminCPs[MAX_COUNTRIES] = { nullptr, "British Training", "British Training", "French Training", "German Training", nullptr, nullptr, nullptr };

	try
	{
		if ( instanceID == Instance::_INVALID )
			throw "Invalid instance";

		if ( forCountry == WWII_NO_COUNTRY || forCountry >= MAX_COUNTRIES )
			throw "Invalid country";

		const char* facName = adminCPs[forCountry];
		if ( facName == nullptr )
			throw "Country has no adminCP entry";

		// See if we can find the training ground for this country
		const std::string facilityName = sdmTranslateName(facName);
		CPStruct* const cp = strtGetCPByName(facilityName.c_str());
		if ( cp == nullptr )
			throw "No training facility";

		FacilityStruct* ab = cp->facilityTypeIndex[FACILITY_ARMYBASE];
		if ( !ab )
			throw "No armybase at training facility";

		HCUNIT_INDEX::iterator it;
		for ( it = hcUnits.begin(); it != hcUnits.end(); ++it )
		{
			if ( it->second->isTrainingUnit() == true && it->second->country() == forCountry && it->second->branch() == forBranch )
				break;
		}
		if ( it == hcUnits.end() )
			throw "No suitable HC units";

		HCUnit* unit = it->second;

		dbugLog(_FLI, "in:%llu posting mission for %s at %s/%s brig#%u %s", instanceID, _countryName[forCountry], cp->name, ab->name, unit->unitID(), unit->name());

		NewMission post;

		post.missionID = (instanceID == Instance::_CAMPAIGN) ? forCountry : 0;
		post.country = forCountry;
		post.brigadeID = unit->unitID();
		post.missionType = SYSTEM_MISSION;
		post.postedByID = 1;
		strcpy(post.posterCallsign, "SYSTEM");
		post.postedAt = teulTime();
		post.postAt = teulTime();
		post.launchAt = teulTime();
		post.launchCP_oid = cp->uOID;
		post.launchFac_oid = ab->uOID;
		post.targetCP_oid = cp->uOID;
		post.targetFac_oid = ab->uOID;
		post.instanceID = instanceID;
		sprintf(post.description, "%s system mission", _countryNameAdj[forCountry]);

		Mission* mission = post.post();
		if ( mission != nullptr )
		{
			InstanceMissions& index = g_instanceMissionIndex[mission->instanceID()];
			index.push_back(mission);
			if ( mission->instanceID() != 0 )
			{
				dbugLog(_FLD, "in:%llu now has %" PRSizeT " missions", mission->instanceID(), index.size());
			}
		}

		return mission;
	}
	catch (const char* error)
	{
		LogAbort("Unable to post system mission for country:%u br:%u [%s]: %s", forCountry, forBranch, _countryNameAdj[forCountry], error);
	}
}

//////////////////////////////////////////////////////////////////////
// Look up or create a mission for a specific instance for a specific
// country. The resulting mission will be

Mission*
missionGetInstanceMission(instanceid_t instanceID, WWII_COUNTRY forCountry, BRANCH_ENUM forBranch)
{
	Mission* const mission = _lookupExistingInstanceMission(instanceID, forCountry, forBranch);
	if ( mission != nullptr )
		return mission;

	return _postNewInstanceMission(instanceID, forCountry, forBranch);
}

//////////////////////////////////////////////////////////////////////
// Delete all the missions associated with an instance.

void
missDeleteInstanceMissions(instanceid_t instanceID)
{
	InstanceMissionIndex::iterator it = g_instanceMissionIndex.find(instanceID);
	if ( it == g_instanceMissionIndex.end() )
	{
		betaLog(_FLD, "in:%llu has no missions to expire", instanceID);
		return;
	}

	InstanceMissions& index = it->second;

	betaLog(_FLD, "in:%llu deleting instance mission list (%" PRSizeT " missions)", instanceID, index.size());

	const UINT32 numMissions = index.size();
	for ( UINT32 i = 0; i < numMissions; ++i )
	{
		// Pull the front and delete it, which will cause the
		// destructor to swap the element with the index.back()
		// so we can just keep popping the front off.
		Mission* mission = index.front();
		if ( mission == nullptr )
		{
			LogAbort("in:%llu contains NULL in mission index at position %u", instanceID, i);
		}

		betaLog(_FLD, "in:%llu m[%u]:%u", instanceID, i, mission->missionID());

		mission->deactivate();
		mission->cancel();

		delete mission;
	}
}

void
missAnnounceMissionsToChat()
{
	for ( MissionIndex::iterator it = g_missions.begin(); it != g_missions.end(); ++it )
	{
		// Send the mobile spawn STO so that they don't become invisible.
		(it->second)->announceMobileSpawnSTO();
	}
}

void
missCancelAllMobileSpawns()
{
	for ( MissionIndex::iterator it = g_missions.begin(); it != g_missions.end(); ++it )
	{
		(it->second)->deactivateMobileSpawn(MSDR_DISPLACED);
	}
}

