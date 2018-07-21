#pragma once
// Created: Thu Mar  3 15:10:00 CST 2005
//
// Brigade classes/structures and types
//
// Copyright (C) PlayNet, Inc 2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "map/mapBrigadeSummary.h"
#include "map/mapPersonaTypes.h"
#include "host/hostPlayers.h"

class Mission;
class CPStruct;
using BrigadeMissions = std::vector<Mission*>;
using BRIGADE_MOVEMENTS = std::map<UINT32,UINT64>;
extern BRIGADE_MOVEMENTS brigadeMovements;
extern UINT64 lastBrigadeMovement;				// How many times brigades have been moved

class Brigade : public BrigadeDetail
{
public:
	Brigade(UINT32 brigadeID, HCUNIT_TYPE typeID, UINT32 parentID
			, WWII_COUNTRY countryID, BRANCH_ENUM branchID, BRANCH_ENUM movementType, UCHAR withUnitClass
			, const ChatHandle handle, const char* name, const char* nick, UINT8 rank
			, const FULL_OBJ_ID homeCP, const ChatHandle owner, RANK_NO ownerRank);
	virtual ~Brigade();

	// Determine if this brigade is accessible by that persona
	bool accessible(const Player* player) const;

	void activateMission() { if ( m_missions < actualMissions() ) m_missions++; updateDestinations(); }

	UINT32 actualMissions() { return (m_missionList.empty() ? 0 : static_cast<UINT32>(m_missionList.size())); }

	void addMission(Mission* const mission);
	void addPlayer(Player* const player);

	void checkOrigins();

	void deactivateMission() { if ( m_missions > 0 ) m_missions--; updateDestinations(); }

	void findMissionPartition();

	bool hasObjectives() const { return (m_missionPartitionIt != m_missionList.begin()); }

	bool hasMissionAt(const CPStruct* cp);

	void removeMission(Mission* const mission);
	void removePlayer(Player* const player);

	void sendMissionUpdates();

	void updateDestinations();
	void updateMissionList() { m_syncMissionList = true; }
	void updateOfficers();

	// Fields
public:
	// List of the missions posted by this brigade
	BrigadeMissions	m_missionList;
	// Pointer to the cut-off between active objective missions and inactive/non-objective missions
	BrigadeMissions::iterator m_missionPartitionIt;
	// People currently in the brigade
	PLAYER_MAP		m_currentMembers;
	PLAYERS			m_currentOfficers;
	UINT32			m_ownerID;				// PlayerID of the owner
	bool			m_syncMissionList;		// Mission list needs synchronizing
	CPStruct*		m_objective;
};

using BRIGADES = std::set<Brigade*>;

extern BRIGADES g_brigades[MAX_COUNTRIES][MAX_BRANCHES];

extern Brigade* mapFindBrigade(UINT32 brigadeID);
extern void mapUpdateBrigadesWithMissionsAt(const CPStruct* cp);

