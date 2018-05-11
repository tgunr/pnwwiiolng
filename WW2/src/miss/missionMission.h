#pragma once
// Created: Sat Apr  9 12:45:52 CDT 2005
//
// Defines basis for the Mission class
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "map/mapPersonaTypes.h"
#include "miss/missMobileSpawn.h"

class	Brigade;
class	CPStruct;
class	FacilityStruct;
class	LinkStruct;

struct MISSION_C_STRUCTURE	// The "c" components of the mission structure that can be memset
{
	ChatHandle			m_poster;			// "chatHandle" of the poster (not a numeric ID)
	instanceid_t		m_instanceID;		// What instance it is in

	MISSION_SUMMARY		m_summary;

	Mission*			m_nextFromOrigin;	// Next mission from this CP
	Mission*			m_prevFromOrigin;	// Previous mission from this CP
	Mission*			m_nextToTarget;		// Next mission to this CP
	Mission*			m_prevToTarget;		// Previous mission to this CP

	playerid_t			m_playerID;  		//same as Player's playerID
	playerid_t			m_backupLeader;		// PlayerID of the backup leader (from chat)
	HCUnitID			m_brigadeID;		// ID of the brigade we belong to
	WWII_COUNTRY		m_country;
	Brigade*			m_brigade;			// Which brigade it belongs to

	UINT32				m_timePost;			// When the mission was posted (timestamp)

	UINT32				m_creationTime;		// in seconds sice epoch
											// mostly for sorting missions by

	UINT32				m_deactivatedAt;	// What time mission was deactivated

	WWII_SIDE			m_side;				// Which side this mission belongs to

	// CP and Facility of the mission - looked up for convenience
	CPStruct*			m_originCP;
	CPStruct*			m_targetCP;
	FacilityStruct*		m_originFac;
	FacilityStruct*		m_targetFac;

	// The link from the target facility (to allow link validity check)
	LinkStruct*			m_linkFromTarget;

	SINT16				m_descSize;			//num chars being sent in desc
	SINT8				m_creatorType;		//computer or player  (comes from CREATOR_TYPE)
	SCHAR				m_desc[MapMiss::MISSION_DESC_LENGTH + 1];	//the text description of the mission

	bool				m_isActive : 1;		// true: Takeable and mission leader present
	bool				m_isOngoing : 1;	// true: Activatable if mission leader returns
	bool				m_registered : 1;
};

class Mission : public MISSION_C_STRUCTURE
{
public:
	Mission(const instanceid_t instanceID, const MISSION_SUMMARY& withSummary); // Constructor
	~Mission();

	// Query operators
	bool isFrontLine() const;
	bool isMobileSpawnAllowed() const { return getMobileSpawnPermission() == MSDR_GRANTED; }
	MSPAWN_RESULT getMobileSpawnPermission() const;
	bool isMobileSpawnAvailable() const { return m_summary.mobileSpawnAvailable && spawner().m_active; }

	bool activateMobileSpawn(Player* player, UINT32 stoUID, SINT32 stoSubType, FLOAT64 x, FLOAT64 y, FLOAT64 z);

	// Tell me whether or not this is accessible to the player
	bool accessible(Player* player) const { return (accessibility(player) == MC_JOIN); }
	MISSION_CODE accessibility(Player* player) const;

	// Flag a mission as being takeable/listed because the mission leader has joined it
	void activate();

	void announceMobileSpawnSTO() const;

	bool addPlayer(Player* player);

	// Flag a mission for removability once the players exit
	// Mission will be deactivated and non-ongoing
	void cancel();

	void changeLeadership(Player* player);

	// Check if the origin is valid; returns false if the origin was invalid and
	// the mission cancelled itself.
	bool checkOrigin();

	// Flag a mission as takeable but unlisted because the mission leader is absent
	void deactivate();

	void deactivateMobileSpawn(MSPAWN_RESULT reason);

	// Setup the helper variables stored in the mission (side, etc)
	bool initializeHelpers(WWII_SIDE side, Brigade* brigade);

	bool isSystemMission() const { return missionType() == SYSTEM_MISSION; }

	UINT32 numTakers() const { return (m_takers.empty()) ? 0 : static_cast<UINT32>(m_takers.size()); }

	void markForUpdate();

	void purgeTakers();

	void registerToLists();

	void removeFromCPLists();

	bool removePlayer(const playerid_t playerID, bool announce);

	// Send mission timer packets to all listeners (covers mobile spawn availabilty too)
	void sendLaunchUpdates(bool force);

	bool updateMobileSpawn(FLOAT32 x, FLOAT32 y, FLOAT32 z);

	void updateTaker(Player* player, RMLIST_ACTION action, bool excludeSelf);

	// Accessors
	bool active() const { return m_isActive; }
	playerid_t backupLeader() const { return m_backupLeader; }
	BRANCH_ENUM branch() const;
	const Brigade* brigade() const { return m_brigade; }
	Brigade* brigade() { return m_brigade; }
	WWII_COUNTRY country() const { return m_country; }
	CREATOR_TYPE creatorType() const { return (CREATOR_TYPE)m_creatorType; }
	instanceid_t instanceID() const { return m_instanceID; }
	UINT32 missionID() const { return summary().missionID; }
	MISSION_TYPE missionType() const { return (MISSION_TYPE)summary().missionType; }
	const CPStruct* originCP() const { return m_originCP; }
	CPStruct* originCP() { return m_originCP; }
	const FacilityStruct* originFac() const { return m_originFac; }
	FacilityStruct* originFac() { return m_originFac; }
	bool ongoing() const { return m_isOngoing; }
	playerid_t playerID() const { return m_playerID; }
	ChatHandle posterHandle() const { return m_poster; }
	WWII_SIDE side() const { return (WWII_SIDE)m_side; }
	MobileSpawn& spawner() { return m_spawner; }
	const MobileSpawn& spawner() const { return m_spawner; }
	SINT8 subType() const { return summary().subType; }
	MISSION_SUMMARY& summary() { return m_summary; }
	const MISSION_SUMMARY& summary() const { return m_summary; }
	const CPStruct* targetCP() const { return m_targetCP; }
	CPStruct* targetCP() { return m_targetCP; }
	const FacilityStruct* targetFac() const { return m_targetFac; }
	FacilityStruct* targetFac() { return m_targetFac; }

public:
	PLAYER_MAP			m_takers;
	MobileSpawn			m_spawner;
};

using MISSION = Mission;

