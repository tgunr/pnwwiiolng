#include <syscfg.h> // Path is project specific.
// Created: Mon Nov 22 12:29:12 EST 2004
//
// Mission host functionality for manipulating mobile spawns
//
// Copyright (c) 2004 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "map/mapPlayer.h"
#include "miss/mission.h"
#include "miss/missionTypes.h"
#include "miss/missionMission.h"
#include "miss/missMobileSpawn.h"
#include "map/mapBrigades.h"
#include "chat/chatSupportMessages.h"
#include "chat/chatHandle.h"
#include "chat/chatName.h"
#include "miss/missionMobileSpawnWaypoint.h"
#include "strt/strtCPStruct.h"
#include "strt/strtFacilityStruct.h"
#include "strt/strtLinkStruct.h"
#include "weap/weapWeapon.h"
#include "wwii/wwiiThreads.h"

//	------------------------------	Class Members

//////////////////////////////////////////////////////////////////////
// Activate a mobile spawn

void
MobileSpawn::activate(
	  Mission* const mission
	, Player* const player
	, UINT32 stoUID
	, SINT32 stoSubType
	, FLOAT64 x, FLOAT64 y, FLOAT64 z
	, UINT32 vehicleID
	)
{
	betaLog(_FLD, "msp: m:%u p:%u c:%s sto:%u stoST:%d activating", mission->missionID(), player->playerID(), player->callsign(), stoUID, stoSubType);

	m_active = true;

	m_playerID = player->playerID();
	chatGetCallsign(m_callsign, player->chatHandle());
	m_handle = player->matchHandle();
	m_stoUID = stoUID;
	m_stoSubType = stoSubType;
	m_x = x;
	m_y = y;
	m_z = z;

	m_vehicleID = vehicleID;

	m_spawnees.clear();
}

//////////////////////////////////////////////////////////////////////
// Flush and deactivate a particular mobile spawn

void
MobileSpawn::deactivate(MSPAWN_RESULT reason, UINT32 missionID)
{
	MobileSpawnWaypoint wp(missionID, m_playerID, m_handle, m_x, m_y, m_z, false, m_stoUID, m_stoSubType, reason);
	chatSendMissionMobileSpawnWaypoint(wp);

	dbugLog(_FLI, "msp: m:%u p:%u sto:%u stoSubType:%d deactivating", missionID, m_playerID, m_stoUID, m_stoSubType);

	// Clear the instance specific data, but retain the positional data.
	m_playerID = 0;
	m_callsign[0] = 0;
	m_handle = 0LL;

	m_active = false;

	m_spawnees.clear();
}

//////////////////////////////////////////////////////////////////////
// If we have a persona record for this spawner, increment the
// spawnee count

void
MobileSpawn::incrementSpawnees(const playerid_t forPlayer) const
{
	if ( m_playerID == 0 )
		return;

	// Check the spawnees list to see if this guy has
	// spawned through you recently
	SPAWNEES::iterator it = m_spawnees.find(forPlayer);
	if ( it != m_spawnees.end() )
	{
		static const UINT32 SPAWNEE_INTERVAL = 3 * 60;
		if ( it->second >= teulTime() - SPAWNEE_INTERVAL )
			return;
	}

	// You are either new or more than INTERVAL secs ago
	Player* const player = PlayerInterface::Lookup(m_playerID);
	if ( player != nullptr )
	{
		player->addMobileSpawnee();
		m_spawnees[forPlayer] = teulTime();
	}
}

//////////////////////////////////////////////////////////////////////
// Activate a mobile spawn

bool
Mission::activateMobileSpawn(Player* const player, UINT32 stoUID, SINT32 stoSubType, FLOAT64 x, FLOAT64 y, FLOAT64 z)
{
	bool wasActive = spawner().isActive();

	// - Activate the spawner
	spawner().activate(this, player, stoUID, stoSubType, x, y, z);

	if ( !wasActive )
	{
		if ( brigade() != nullptr )
			++(brigade()->m_mobileSpawns);
	}

	// Notify all of the listeners
	summary().mobileSpawnAvailable = true;
	sendLaunchUpdates(true);

	announceMobileSpawnSTO();

	return true;
}

//////////////////////////////////////////////////////////////////////
// Notify the chat host of this STO

void
Mission::announceMobileSpawnSTO() const
{
	if ( spawner().isActive() == false )
		return;
	MobileSpawnWaypoint wp(missionID()
							, spawner().m_playerID
							, spawner().m_handle
							, spawner().m_x
							, spawner().m_y
							, spawner().m_z
							, true
							, spawner().m_stoUID
							, spawner().m_stoSubType
							, MSDR_GRANTED
							);
	chatSendMissionMobileSpawnWaypoint(wp);
}

//////////////////////////////////////////////////////////////////////
// Deactivate a mobile spawn

void
Mission::deactivateMobileSpawn(MSPAWN_RESULT reason)
{
	// Nothing to do if we're already deactivated
	if ( !spawner().m_active )
		return;

	// - Update the mission summary
	dbugLog(_FLI, "msp: deactivating %u(%s) from mission %u reason %d"
					, spawner().m_playerID, spawner().m_callsign
					, missionID()
					, reason);

	if ( brigade() != nullptr )
		--(brigade()->m_mobileSpawns);

	/// - Send a timer update to all our listners
	summary().mobileSpawnAvailable = false;
	sendLaunchUpdates(true);

	// - Finally deactivate the mobile spawn itself
	spawner().deactivate(reason, missionID());
}

//////////////////////////////////////////////////////////////////////
// Determine if the incoming firebase is enemy or not

static inline bool
_isIncomingEnemyFirebase(FacilityStruct* fb, CPStruct* cp, WWII_SIDE forSide)
{
	if ( fb == nullptr ) return false;
	if ( fb->type != FACILITY_FIREBASE ) return false;
	if ( !fb->isOpen() ) return false;
	if ( fb->side == WWII_NO_SIDE || fb->side == forSide ) return false;
	if ( fb->link == nullptr ) return false;
	if ( fb->link->endDepot == nullptr ) return false;
	if ( fb->link->endDepot->cp != cp ) return false;
	return true;
}

//////////////////////////////////////////////////////////////////////
// Test to see the weapon is allowed to be a mobile spawn for this
// mission

MSPAWN_RESULT
Mission::getMobileSpawnPermission() const
{
	if ( !ongoing() )
		return MSDR_MISSION_EXPIRED;

	///RULE: OWNER
	// Must own the origin facility
	if ( m_originFac == nullptr || m_originFac->side != m_side )
	{
		threadLog(_FLD, "msp: m#%d no-longer origin owner", missionID());
		return MSDR_OWNER;
	}

	bool offense = ( m_originCP != m_targetCP && !_isIncomingEnemyFirebase(m_targetFac, m_originCP, m_side) );

	// Only apply the full set of rules when the target CP is
	// not the origin CP and the target facility is not an
	// incoming enemy firebase to the target CP
	if ( offense )
	{
		///RULE: LINK
		// Must be a link between non-same cp
		if ( m_linkFromTarget == nullptr )
		{
			threadLog(_FLD, "msp: m#%d no link from %s to %s"
							, missionID()
							, m_originCP->name
							, m_targetCP->name
							);
			return MSDR_LINK;
		}
	}

	if ( m_targetFac && m_targetFac->type == FACILITY_FIREBASE && !m_targetFac->isOpen() )
	{
		threadLog(_FLD, "msp: denied: firebase %s is destroyed", m_targetFac->name);
		return MSDR_TARGET_DESTROYED;
	}

	if ( offense )
	{
		///RULE: VLINK
		// Must be a supply connection
		if ( m_linkFromTarget && m_linkFromTarget->hasEnemyFirebase(m_side) )
		{
			threadLog(_FLD, "msp: m#%d supply cut from %s to %s"
							, missionID()
							, m_originCP->name
							, m_targetCP->name
							);
			return MSDR_VLINK;
		}

		///RULE: EOC
		// Enemy has to own or control target to mobile spawn
		if ( !(m_targetCP->isContested() || (m_targetCP->numUnits() == 0 && m_targetCP->isDefendObjective(m_side)))
			&& m_targetCP->side == m_side
			&& m_targetCP->controllingSide == m_side
			&& (m_targetCP->isEnemyLinked() == false || m_targetCP->fullObjID == m_brigade->homeCPID())
			)
		{
			threadLog(_FLD, "msp: m#%d(%d) target %s o:%d/c:%d"
							, missionID()
							, m_side
							, m_targetCP->name
							, m_targetCP->side
							, m_targetCP->controllingSide
							);
			return MSDR_EOC;
		}
	}

	return MSDR_GRANTED;
}

static const char* const msdrResults[MSPAWN_RESULT_MAX][2] =
{
	{ "msdr_invalid", "Unknown error" }
,	{ "msdr_granted", "Granted" }
,	{ "msdr_failed", "Internal error" }
,	{ "msdr_vehicle", "Your unit-type cannot deploy FRUs/MSPs" }
,	{ "msdr_disabled", "Spawning is currently disabled on the server" }
,	{ "msdr_denied", "Permission Denied" }
,	{ "msdr_no_mission", "Must be on a valid mission" }
,	{ "msdr_dead", "Can't do that while you're dead" }
,	{ "msdr_rank", "Your rank is too low" }
,	{ "msdr_owner", "Your origin has been captured" }
,	{ "msdr_tfr", "Too far from target" }
,	{ "msdr_efr", "Too close to enemy facility" }
,	{ "msdr_1per", "One FRU/MSP allowed per mission" }
,	{ "msdr_eoc", "Enemy must Own or Control target" }
,	{ "msdr_link", "No link between origin and target" }
,	{ "msdr_vlink", "Enemy controls link between origin and target" }
,	{ "msdr_moving", "Cannot deploy while moving" }
,	{ "msdr_stationary", "Cannot deploy while stationary" }
,	{ "msdr_displaced", "You have been displaced" }
,	{ "msdr_undeployed", "Undeployed" }
,	{ "msdr_lapsed", "Timed out" }
,	{ "msdr_not_commander", "Must be pilot/driver of vehicle" }
,	{ "msdr_mission_expired", "Your mission has been closed/expired" }
,	{ "msdr_target_destroyed", "Target firebase has been destroyed or closed due to brigade movements" }
,	{ "msdr_target_invalid", "Invalid target" }
,	{ "msdr_invalid_state", "Invalid state" }
};
#include "chat/chatLStringIDs.h"

//////////////////////////////////////////////////////////////////////
// Add a non-vehicle mobile spawn that has been instigated by the system

void
missAddMobileSpawn(UINT32 missionID, const playerid_t playerID, const ChatHandle handle, const FLOAT64& x, const FLOAT64& y, const FLOAT64& z, UINT32 stoUID, SINT32 stoSubType)
{
	MSPAWN_RESULT error = MSDR_FAILED;
	try
	{
		Mission* const mission = mapFindMission(missionID);
		if ( mission == nullptr )
			throw std::domain_error("Unknown mission");
		Player* const player = PlayerInterface::Lookup(playerID);
		if ( player == nullptr )
			throw std::domain_error("Unknown player");

		error = mission->getMobileSpawnPermission();
		dbugLog(_FLI, "m:%u p:%u c:%s sto:%u stoType:%d msp @ %0.3g/%0.3g/%0.3g result:%u",
			missionID, player->playerID(), player->callsign(), stoUID, stoSubType, x, y, z, error);

		if ( error != MSDR_GRANTED )
		{
			if ( mission->spawner().isActive() )
				mission->deactivateMobileSpawn(MSDR_DISPLACED);

			std::string reason = "msp placement refused: ";
			if ( error > 0 && error < MSPAWN_RESULT_MAX )
			{
				player->message(LSTR_ERROR, LSTR_LABELLED_GENERIC, "FRU/MSP placement failed", msdrResults[error][1]);
				reason += msdrResults[error][0];
			}
			else
				reason += "BAD MSDR_RESULT!";
			throw std::domain_error(reason);
		}

		mission->activateMobileSpawn(player, stoUID, stoSubType, x, y, z);
	}
	catch ( std::domain_error& e )
	{
		Chat::Name callsign(handle);
		dbugLog(_FLA, "m:%u p:%u c:%s sto:%u stoSubType:%d unable to add mobile spawn: %s", missionID, playerID, callsign.c_str(), stoUID, stoSubType, e.what());

		MobileSpawnWaypoint wp(missionID, playerID, handle, x, y, z, false, stoUID, stoSubType, error);
		chatSendMissionMobileSpawnWaypoint(wp);
	}
}

//////////////////////////////////////////////////////////////////////
// Check and process a cancel instruction for a mobile spawn

void
missMobileSpawnCancel(UINT32 missionID, const ChatHandle handle, SINT32 x, SINT32 y)
{
	Mission* mission = mapFindMission(missionID);
	if ( mission == nullptr )
	{
		dbugLog(_FLM, "m:%u no such mission for msp cancel", missionID);
		return;
	}

	// Race-condition check
	if ( !mission->spawner().isActive() )
	{
		dbugLog(_FLM, "m:%u msp isn't active", missionID);
		return;
	}

	if ( chatGetLowercaseHandle(mission->spawner().m_handle) != chatGetLowercaseHandle(handle) )
	{
		dbugLog(_FLW, "msp: handle mismatch");
		return;
	}

	mission->deactivateMobileSpawn(MSDR_DISPLACED);
}
