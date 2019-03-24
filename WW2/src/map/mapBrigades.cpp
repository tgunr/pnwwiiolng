#include <syscfg.h> // Path is project specific.
// Date: Thu Mar  3 15:10:00 CST 2005
//
// Brigade Functionality
//
// Copyright (C) PlayNet, Inc 2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith
// Supporting Programmer: Patrick "thunder" Macgruder

#include <stdexcept>
#include <algorithm>

#include "miss/missionTypes.h"
#include "miss/missionServerTypes.h"
#include "strt/strtCPStruct.h"
#include "map/mapPlayer.h"
#include "map/mapBrigades.h"
#include "miss/missionMission.h"
#include "miss/missionListUpdate.h"

// Temporary
#include "chat/chatSupportRPCMessages.h"
#include "chat/chatBroadcast.h"
#include "chat/chatTypes.h"

BRIGADES g_brigades[MAX_COUNTRIES][MAX_BRANCHES];	// Categorized list of brigades

BRIGADE_MOVEMENTS brigadeMovements;
UINT64 lastBrigadeMovement = 0LL;			// How many times brigades have been moved

//////////////////////////////////////////////////////////////////////
// Constructor for a BrigadeDetail object

Brigade::Brigade(UINT32 withBrigadeID, HCUNIT_TYPE withTypeID, UINT32 withParentID
				, WWII_COUNTRY withCountryID, BRANCH_ENUM withBranchID, BRANCH_ENUM withMovementType, UCHAR withUnitClass
				, const ChatHandle withHandle, const char* withName, const char* nick, UINT8 withRank
				, const FULL_OBJ_ID homeCP, const ChatHandle withOwner, RANK_NO withOwnerRank)
		: BrigadeDetail(withBrigadeID, withTypeID, withParentID, withCountryID, withBranchID, withMovementType, withUnitClass
						, withHandle, withName, nick, withRank, homeCP, withOwner, withOwnerRank)
		, m_missionPartitionIt(m_missionList.end())
		, m_ownerID(0)
		, m_syncMissionList(false)
		, m_objective(nullptr)
{
	dbugLog(_FLI, "brig:%d cmd:%d ctry:%d brch:%d(%s) name=%s nick=%s parent=%s rank=%d"
					, m_unitID, m_parentID, m_countryID, m_branchID, callsign(), m_name, m_shortName
					, (m_parent) ? m_parent->name() : "<none>"
					, (UINT32)m_rank
					);

	g_brigades[m_countryID][m_branchID].insert(this);
}

//////////////////////////////////////////////////////////////////////
// Destructor for a brigade detail object

Brigade::~Brigade()
{
	g_brigades[m_countryID][m_branchID].erase(this);
}

//////////////////////////////////////////////////////////////////////
// Determine if a particular player has access to this brigade

bool
Brigade::accessible(const Player* player) const
{
	if ( player->side() != side() ) return false;
	if ( player->branch() != m_branchID ) return false;
//	if ( missions() == 0 && officers() == 0 && !player->permissions(POST_MISSIONS) ) return false;
	return true;
}

//////////////////////////////////////////////////////////////////////
// Manage the brigade counts at a particular CP

static void
_addBrigadeMission(Brigade* brigade, ACTIVE_BRIGADES& list)
{
	++ list[brigade];
}

static void
_removeBrigadeMission(Brigade* brigade, ACTIVE_BRIGADES& list)
{
	ACTIVE_BRIGADES::iterator it = list.find(brigade);
	if ( it == list.end() )
		return;
	if ( it->second <= 1 )
		list.erase(it);
	else
		-- it->second;
}

//////////////////////////////////////////////////////////////////////
// Add a mission to the brigade's list

void
Brigade::addMission(Mission* const mission)
{
	if ( mission == nullptr )
		throw std::runtime_error("NULL mission in brigade->addMission");	///Terminal

	if ( mission->isSystemMission() )
		return;

	BrigadeMissions::iterator it;

#ifdef BETA_RELEASE
	it = find(m_missionList.begin(), m_missionList.end(), mission);
	if ( it != m_missionList.end() )
		dbugLog(_FLA, "brig:%d mission %d already in mission list", brigadeID(), mission->missionID());

	if ( mission->brigade() != this )
		dbugLog(_FLA, "brig:%d mission %d doesn't appear to belong to us!", brigadeID(), mission->missionID());
#endif

	findMissionPartition();

	// Insert it where the active objective list ends
	m_missionList.insert(m_missionPartitionIt, mission);

	dbugLog(_FLI, "brig:%d added mission %d", brigadeID(), mission->missionID());

	if ( mission->active() )
		updateDestinations();

	if ( mission->originCP() )
		_addBrigadeMission(this, mission->originCP()->m_brigades);
	if ( mission->targetCP() && mission->targetCP() != mission->originCP() )
		_addBrigadeMission(this, mission->targetCP()->m_brigades);
}

//////////////////////////////////////////////////////////////////////
// Add a player to the brigade's membership list

void
Brigade::addPlayer(Player* const player)
{
	PLAYER_MAP::iterator it = m_currentMembers.lower_bound(player->playerID());
	// Is this player already in the index?
	if ( it != m_currentMembers.end() && it->first == player->playerID() )
	{
		if ( it->second != player )
		{
			dbugLog(_FLD, "t:%d p:%u c:%s stray player lingering in brigade list", player->teulIndex(), player->playerID(), player->callsign());
			it->second = player;
		}
		return;
	}

	// Ok - this is where they should be added then
	m_currentMembers.insert(it, std::make_pair(player->playerID(), player));
	m_players = (UINT16)m_currentMembers.size();

	// Are they an officer?
	if ( player->useRank().commissioned() )
	{
		m_currentOfficers.insert(player);
		updateOfficers();
	}
}

//////////////////////////////////////////////////////////////////////
// Find the parition in the mission list

static bool _isObjectiveMission(const Mission* const mission) { return (mission->active() && mission->targetCP()->isCity() && mission->targetCP()->isObjective()); }

static bool _destinationCompare(const Mission* const lhs, const Mission* const rhs) { return lhs->targetCP() < rhs->targetCP(); }

void
Brigade::findMissionPartition()
{
		m_missionPartitionIt = stable_partition(m_missionList.begin(), m_missionList.end(), _isObjectiveMission);
		stable_sort(m_missionList.begin(), m_missionPartitionIt, _destinationCompare);
}

//////////////////////////////////////////////////////////////////////
// Remove a player from the brigade

void
Brigade::removePlayer(Player* const player)
{
	m_currentMembers.erase(player->playerID());
	m_currentOfficers.erase(player);

	m_players = (UINT16)m_currentMembers.size();
	updateOfficers();
}

//////////////////////////////////////////////////////////////////////
// Remove a mission from the brigade's list

void
Brigade::removeMission(Mission* const mission)
{
	if ( mission == nullptr )
		throw std::runtime_error("NULL mission in brigade->removeMission");	///Terminal

	if ( mission->isSystemMission() )
		return;

	if ( m_missionList.empty() )
		return;

	BrigadeMissions::iterator it = find(m_missionList.begin(), m_missionList.end(), mission);
	if ( it == m_missionList.end() )
	{
		dbugLog(_FLA, "brig:%u mission %u already removed from list", brigadeID(), mission->missionID());
		return;
	}

	if ( mission->originCP() )
		_removeBrigadeMission(this, mission->originCP()->m_brigades);
	if ( mission->targetCP() && mission->targetCP() != mission->originCP() )
		_removeBrigadeMission(this, mission->targetCP()->m_brigades);

	m_missionList.erase(it);

	if ( mission->active() )
		deactivateMission();

	updateDestinations();
}

//////////////////////////////////////////////////////////////////////
// Let listeners know if there are mission-list updates

void
Brigade::sendMissionUpdates()
{
	if ( !m_syncMissionList )
		return;

	m_syncMissionList = false;

	if ( m_currentMembers.empty() )
		return;

	MISSION_LIST_UPDATE outgoing((UINT64)brigadeID(), 0, MISSION_LIST_UPDATE::STUB_ONLY); ///REDO
	PLAYER_MAP::iterator it;
	Player* player;

	dbugLog(_FLD, "b:%d sending mission list update", brigadeID());
	for ( it = m_currentMembers.begin(); it != m_currentMembers.end(); ++it )
	{
		player = it->second;
		if ( player == nullptr || player->teulIndex() < 0 || !player->inState(PERS_SELECTING_MISSION) )
			continue;
		outgoing.request(player->teulIndex());
	}
}

//////////////////////////////////////////////////////////////////////
// Update the list of brigade destinations

void
Brigade::updateDestinations()
{
	// This function is a little complex because we need to try and avoid
	// spending too much time messing with these lists.
	//
	// So what we do is partition the mission list into a convenient
	// active objective missions vs inactive/non-objective/non-city list.
	// The cost per update should be relatively minor because we're
	// using a linked list, so the list should be relatively stable.
	//
	// Having partitioned, the "isObjective" partition will be empty
	// if we have nothing to do, or we will sort it into a by-cp
	// list, so that we can then easily find the most occuring CP
	// and choose that as our objective.

	CPStruct* newObjective = nullptr;
	if ( !m_missionList.empty() && m_missionList.size() > 0 )
	{
		// Split the mission list into active and inactive missions,
		// because we only need to sort active missions
		findMissionPartition();

		// If the first entry in the "inactive" partition is at the beginning
		// of the list, then there are no active missions
		BrigadeMissions::iterator it = m_missionList.begin();

#ifdef BETA_RELEASE
		dbugLog(_FLI, "brig:%d missions: ", brigadeID());
		for ( it = m_missionList.begin(); it != m_missionList.end(); ++it )
		{
			dbugLog(_FLI, "%c%d:%s[%d] "
							, ((it == m_missionPartitionIt) ? '*' : '+')
							, (*it)->missionID(), (*it)->targetCP()->name, _isObjectiveMission(*it));
		}
#endif

		UINT32 missionsToCurrentObjective = 0, mostMissions = 0;
		it = m_missionList.begin();
		while ( it != m_missionPartitionIt )
		{
			CPStruct* curTarget;
			UINT32 missionsToTarget;
			// Because the list is sorted,
			for (
				  curTarget = (*it)->targetCP(), missionsToTarget = 0
				; it != m_missionPartitionIt && (*it)->targetCP() == curTarget
				; ++it
				)
			{
				missionsToTarget++;
			}

			// Flap avoidance: prefer the currentObjective if we can
			if ( m_objective != nullptr && m_objective == curTarget )
				missionsToCurrentObjective = missionsToTarget;

			// Remember which target we saw most of
			if ( missionsToTarget > mostMissions )
			{
				newObjective = curTarget;
				mostMissions = missionsToTarget;
			}
		}

		// Try to prefer the current objective
		if ( m_objective && missionsToCurrentObjective > 0 && missionsToCurrentObjective >= mostMissions )
		{
			newObjective = m_objective;
			dbugLog(_FLD, "br:%d sticking with %s, %d missions", brigadeID(), newObjective->name, missionsToCurrentObjective);
		}
		else if ( newObjective && mostMissions > 0 )
		{
			dbugLog(_FLD, "br:%d new objective %s, %d missions", brigadeID(), newObjective->name, mostMissions);
		}
		else
			dbugLog(_FLD, "br:%d no objective picked", brigadeID());
	}

	if ( m_objective != newObjective )
	{
		m_objective = newObjective;

		if ( newObjective == nullptr )
		{
#ifdef BETA_RELEASE
			///LOCALIZE
			chatSendBroadcast(Chat::_HCUNIT, Instance::_CAMPAIGN, brigadeID(), LSTR_BETA, LSTR_GENERIC, "Brigade no-longer has an objective");
#endif
			m_objectiveCPID = nullObjID;
		}
		else
		{
#ifdef BETA_RELEASE
			chatSendBroadcast(Chat::_HCUNIT, Instance::_CAMPAIGN, brigadeID(), LSTR_BETA, LSTR_LABELLED_GENERIC, "New brigade Objective", newObjective->name);
#endif
			m_objectiveCPID = newObjective->fullObjID;
		}
	}

	if ( m_objective == nullptr )
		m_missionPartitionIt = m_missionList.end();
}

//////////////////////////////////////////////////////////////////////
// Update the officer count and lists

void
Brigade::updateOfficers()
{
	m_officers = (UINT16)m_currentOfficers.size();
	if ( m_officers == 0 )
		return;

	wwiiPlayer* topOfficers[_MAX_OFFICERS];

	partial_sort_copy(m_currentOfficers.begin(), m_currentOfficers.end(), std::begin(topOfficers), std::end(topOfficers), [](auto* lhs, auto* rhs) {
		return rhs->effectiveRank() < lhs->effectiveRank();
	});

	for ( size_t i = 0; i < _MAX_OFFICERS; ++i )
	{
		if ( i < m_officers )
		{
			m_officerHandles[i] = topOfficers[i]->chatHandle();
			m_officerRanks[i] = topOfficers[i]->effectiveRank();
		}
		else
		{
			m_officerHandles[i] = 0L;
			m_officerRanks[i] = 0;
		}
	}
}

//////////////////////////////////////////////////////////////////////
// Lookup a brigade ID and return a pointer to it or NULL

Brigade*
mapFindBrigade(UINT32 brigadeID)
{
	HCUnit* unit = HCUnit::Lookup(brigadeID);
	return ( unit != nullptr && (unit->type() == HCU_BRIGADE || unit->type() == HCU_GARRISON) ) ? (Brigade*)unit : nullptr;
}

void mapClearBrigadeMissions(UINT32 brigadeID)
{
	Brigade* brigade = (Brigade*)HCUnit::Lookup(brigadeID);
	if (brigade->missions() > 0)
	{
		while (brigade->m_missionList.size() > 0)
		{
			Mission *mission = *brigade->m_missionList.begin();
			delete mission;
		}

		mapNotifyPlayersInGarrison(brigadeID, brigade, brigade->country());
	}
}

//////////////////////////////////////////////////////////////////////
// Update the status of all brigades with missions at a particular CP

using BRIGADE_SET = std::set<Brigade*>;

void
mapUpdateBrigadesWithMissionsAt(const CPStruct* cp)
{
	for ( auto it = cp->m_brigades.begin(), end = cp->m_brigades.end(); it != end; ++it )
	{
		it->first->updateDestinations();
	}
}

void
Brigade::checkOrigins()
{
	BrigadeMissions::iterator it;
	BrigadeMissions::iterator next = m_missionList.begin();
	while ( next != m_missionList.end() )
	{
		it = next++;

		(*it)->checkOrigin();
	}
}
