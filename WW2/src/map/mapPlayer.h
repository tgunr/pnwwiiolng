#pragma once
// Created: Thu Feb  1 13:26:23 2001
//
// Description goes here
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.

#include <map>

#include "chat/chatGroupsType.h"
//#include "server/serverTypes.h"
#include "miss/missionTypes.h"
#include "miss/missionPacket.h"
#include "map/mapPlayerTypes.h"
#include "wwii/wwiiPermissions.h"
#include "wwii/wwiiPlayer.h"

#define MAX_SHORT_TITLE 16
#define MAX_LONG_TITLE 64
#define MAX_ABV_TITLE 5

static const UINT32 DEFAULT_PLAYER_ID = 1;

class MissionDamageEvent;
struct ChatPermissions;
namespace Server
{
	class ServerInfo;
}

#include "wwii/wwiiHash.h"
typedef std::unordered_map<UINT64, UINT32> SORTIE_DAMAGE_HISTORY;

class FacilityStruct;
class Brigade;

namespace Scoring
{
	class Sortie;
}

class Player : public wwiiPlayer
{
public:
	Player();
	Player(SINT32 teulIndex, const Auth::Dispatch& disp, const playerid_t forPlayerID);
	virtual ~Player();

	UINT32 accountGoodUntil() const
	{
		if ( meteredAccess() )
			return ((teulTime() / 60) + m_minuteBalance) * 60 + 59;
		return m_accountGoodUntil;
	}

	bool allowLaunch() const;

	// Is persona able to post* at all*?
	bool ableToPost() const;

	bool ableToPostIn(const class Brigade&) const;

	void addMobileSpawnee() { ++ missionPacket().scoring().spawnees; }

	void addStratHit(const class MissionDamageEvent&);

	// Lookup this personas current mission
	class Mission* activeMission() const;

	void analyzeDamageHistory();

	// Use the supplied mission packet to populate helper fields etc
	void applyMissionPacket(class MissionPacket& newMP);

	// Increase and save player rank if they are below max rank
	void bumpRank();

	// Evaluate the results of a mission
	bool calculateMissionResults(MM_PERS_STATE);

	// Calcluate the score for their sortie
	UINT32 calculateSortieScore();

	void cancelWeapon(bool notifyStrat);

	const ChatPermissions* chatPermissions() const { return m_permissions; }

	// Charge next minutely balance if the player is due.
	bool charge();

	// Match executable checksum
	void checksum(UINT64);

	// Check if the player has access to the WEAPON_SPEC.asUINT16() id of this weapon
	bool CheckWeaponMask(UINT16 spec) const;

	void clearMissionTakerList();

	// Clear out the current sortie results; if leaving the mission,
	// clear out additional fields.
	void clearSortieResults(bool leavingMission);

	// Create a new persona for me
	UINT32 createPersonaRecord(WWII_COUNTRY, BRANCH_ENUM) const;

	void debug(const char* message) const;
	void debug(const char* label, const char* mask, ...) const;

	void deflateMissionPacket();

	// Mark that a persona is entering a mission
	bool enterMission(class Mission* mission);

	// Evaluate a players mission packet for scoring purposes
	// (Assumes all pertinent data has been gathered)
	bool evaluateMissionPacket();

	// Exit the brigade/mission/etc you are currently on, if any
	virtual void ExitBrigade(UINT32 state) OVERRIDE_FN;
	void ExitMission();

	bool fillMissionPacket(class Mission*);

	bool hasActiveMission() const { return (activeMission() != nullptr); }

	bool hasAvailableVehicle() const;
	bool hasCompletedSpam() const { return (m_loginState == 255); }
	bool hasVehicle() const { return (weapon().serial > 0); }

	// "In" queries
	bool inState(MM_PERS_STATE testState) const { return (m_state == testState); }

	bool isAtLobby() const { return (isConnected() == true && isSpawned() == false); }
	bool isBrigadeLeader() const;
	bool isConnected() const { return teulIndex() >= 0 && !inState(PERS_EXITING); }

	// Tell if this persona is prevented from this side
	UINT32 isSideLocked(WWII_SIDE forSide) const;

	void loadChatMask();

	// How many minutes do they have remaining
	bool meteredAccess() const { return permissions(UNMETERED_ACCESS) == 0; }
	UINT32 minuteBalance() const { return (meteredAccess() ? m_minuteBalance : 0); }

	//! Send player to the chat host.
	bool mirrorToChat();

	//! Send player to a cell host.
	bool mirrorToCellHost(Server::ServerInfo* cellhost);

	class Mission* mission() const { return m_mission; }

	const MissionPacket& missionPacket() const { return m_mp; }
	MissionPacket& missionPacket() { return m_mp; }

	void setRequestedCapacity(const bool state_) { m_requestedCapacity = state_; }
	void setReceivedCapacity(const bool state_) { m_receivedCapacity = state_; }

	UINT32 nextLaunchTime() const;

	void postDenied(const WEAPON_SPEC& weapon, UINT32 reasonCode, bool loseWeapon);

	// Receive and interpret a capacity response
	// calls evaluateMissionPacket()
	void processCapacityResponse(const FULL_OBJ_ID&, const WEAPON_SERIAL&, const bool vehicleDelivered);

	void saveMissionPacket();

	// Player has selected (or unselected) their affiliations
	bool SelectBrigade(Brigade* brigade);
	MISSION_CODE selectMission(class Mission* mission, bool enter);
	void selectNullPersona();
	PERSONA_LOAD_STATUS selectPersona(UINT32 personaID, WWII_COUNTRY country, BRANCH_ENUM branch);
	// Retrieve the player-specific fields from the DB
	bool selectPlayer(const playerid_t newPlayerID);

	// Send me a complete list of mission takers
	void sendFullMissionTakerList();

	// If persona has been flagged with updateChat(), sends the
	// chat host a persona update for this persona. Called automatically
	void sendUpdateToChat();

	void SetBranch(BRANCH_ENUM newBranch);
	void SetBrigade(Brigade* newBrig);
	void SetChecksumIsDirty(bool toState) { m_flags.checksumsDirty = toState; }
	virtual void SetCountry(WWII_COUNTRY newCountry, WWII_SIDE newSide) OVERRIDE_FN;
	void SetDecal(UINT8 newDecal) { m_mp.m_player.decal = newDecal; }
	virtual bool SetInstance(const Instance::Descriptor& _instance) OVERRIDE_FN;
	void SetGroups(const char* groupList);
	void SetMeteredAccess(UINT32 newMinuteBalance);
	void SetMission(Mission* to);
	void SetPersonaID(UINT32 newID) { m_personaID = newID; }
	// Set mission packet flags to correspond to persona flags
	void SetPersonaMissionFlags();
	void SetPlayerID(UINT32 newID) { m_playerID = newID; }
	void SetRank(UINT8 level);
	void SetSideLock();
	void SetSquad(UINT32 newSquad) { m_squadID = newSquad; }
	void SetSquadHandle(const ChatHandle& newHandle) { m_squadHandle = newHandle; }
	void SetState(MM_PERS_STATE toState);
	// Set the mission target
	void SetTarget(const FacilityStruct* facility);
	void SetVehicleState(MM_VEH_STATE toState) { m_vehicleState = toState; }
	void SetWeapon(const WEAPON_SERIAL& weapon);

	bool isSpawned() const { return inState(PERS_SPAWNING) | vehicleState(MM_VEH_INXFER); }

	void startLoginSpam(bool clientActivated, bool xferedToChat);

	// Clear out the mission fields (not the missionPacket)
	// NOTE: Does not remove the player* from* the mission
	void unsetMission() { m_mission = nullptr; m_permissionMask &= ~MISSION_LEADER; }

	// Notify chat of a persona status/whereabouts change
	void updateChat();

	// Send the latest neccessary CP updates
	void updateMap();

	void updateMask();

	void updateMissionTaker(bool excludeSelf);

	// Work out when the player can launch again
	void updateNextLaunchTime();

	UINT32 updateRoomListFlags();

	// Is the current mission packet valid?
	bool validMissionPacket() const { return m_hasValidMissionPacket; }

	MM_VEH_STATE vehicleState() const { return m_vehicleState; }
	bool vehicleState(MM_VEH_STATE query) const { return m_vehicleState == query; }

	// Read accessors
	Brigade* brigade() const { return m_brigade; }
	UINT32 customerID() const { return m_customerID; }
	UINT8 decal() const { return m_mp.m_player.decal(); }
	bool isBuzzard() const { return m_isBuzzard; }
	bool isChecksumDirty() const { return m_flags.checksumsDirty; }
	UINT32 loginState() const { return m_loginState; }
	bool needsChatSync() const { return m_needsChatSync; }
	bool requestedCapacity() const { return m_requestedCapacity; }
	bool receivedCapacity() const { return m_receivedCapacity; }
	UINT32 points() const { return m_rankPointsEarned; }
	UINT32 progress() const;
	bool promoted() const { return m_promoted; }
	const Rank& useRank() const { return *m_useRank; }
	const Rank& actualRank() const { return *m_actualRank; }
	UINT32 roomListFlags() const { return m_roomFlags; }
	MM_PERS_STATE state() const { return m_state; }
	const WEAPON_SERIAL& weapon() const { return m_weapon; }
	WEAPON_SERIAL& weapon() { return m_weapon; }

	// For the scoring sortie thing.
	void closeCurrentSortie();
	void newSortie(UINT32 newSortieID, bool isPilot);
	const Scoring::Sortie* sortie() const { return m_sortie; }
	Scoring::Sortie* sortie() { return m_sortie; }

public:
	// Event handlers.
	bool onDespawn(SINT32 teulIndex);	//! Process return-to-map signal from client.

public:

	//Playermask is individual permissions that a player has it is NOT rank related
	//Personamask is permissions related specifically to the persona
	// this includes rank related privs.
	//Permmask is a bitwise or of the two other masks
	// this is where permissions should be checked
	PermissionMask	m_playerMask;		// Base account permissions
	PermissionMask	m_personaMask;		// Set via updateMask()
	TeulTimeMS		m_nextSpam;

	UINT64			m_lastHCUpdate;		// Last HCUnit position update
	FLOAT64VECTOR	m_customSpawnLocation;

	UINT32			m_customerID;		// Billing ID
	UINT32 			timeIn;
	UINT32			m_rankPointsEarned;	//how many they currently have
	UINT32			m_sideLockTime;
	UINT32			m_portal;

	MM_PERS_STATE	m_state;
	MM_VEH_STATE	m_vehicleState;

	//system time at which a side was chosen.
	//ability to select side is based of this time
	WWII_SIDE		m_lastSide;
	WWII_SIDE		m_sessionSide;

	class Brigade*	m_brigade;
	class Mission*	m_mission;
	struct AAStatus* m_antiAddiction;

	UINT32			m_roomFlags;		// Last known room list flags
	UINT32			m_loginState;		// How far into their login they are
	UINT32			m_loginSub;			// Sub-state for login process
	UINT32			m_nextLaunch;		// When player can next launch (based on delays etc)

	// Stats
	UINT32			m_lastPoll;			// Last opinion poll player responded to

	const Rank*		m_useRank;			// Pointer to the Rank of our effective rank
	const Rank*		m_actualRank;		// Pointer to the Rank of our actual rank

public:
	bool			m_hasValidMissionPacket : 1;	// Is the mission packet valid?
	bool			m_registered : 1;			// True if this persona is the registered instance
	bool			m_launchPermitted:1;		// Whether player is allowed to click launch yet
	bool			m_wantMobileSpawn:1;		// Has player asked for a mobile spawn?
	bool			m_requestedCapacity : 1;	// True when capacity has been requested.
	bool			m_receivedCapacity : 1;		// True when we've received a capacity reply
	bool			m_needsChatSync:1;			// Needs to be sync'd with chat
	bool			m_promoted : 1; 			// If his last mission gave a promotion
	bool			m_clientActivated : 1;		// Has the client been activated yet?
	bool			m_xferedToChat : 1;			// Has the client completed chat transfer yet?
	bool			m_allowPollVote : 1;		// Defaults to no
	bool			m_hasBalanced : 1;
	bool			m_hasSelectedPersonaOnce : 1;	// has selected a persona at least once
	bool			m_useCustomSpawnLocation : 1;
	bool			m_isBuzzard : 1;
	bool			m_implementAntiAddiction : 1;
	UINT32			m_spawnedAsSide : 2;		// Which side they spawned as - or 0.
	UINT32			m_lastSpawnedTo;			//! Address of cell they were last sent to.

	//save the mission packet for later
	//in mmSpawn.c we'll ship it off in a
	//persXfer struct to the updater
	MissionPacket	m_mp;				// Mission packet
	FULL_OBJ_ID		m_weapOrigin;		// object id of the current origin facility
	WEAPON_SERIAL	m_weapon;

private:
	SORTIE_DAMAGE_HISTORY	m_damageRecord, m_repairRecord;
	CHAT_GROUPS		m_groups;
	const ChatPermissions* m_permissions;

	// Accounting
	UINT32				m_accountGoodUntil;		//!< When does metered play time expire?
	UINT32				m_minuteBalance;		//!< Metered balance
	UINT32				m_nextCharge;			//!< Next time to charge balance
	Scoring::Sortie*	m_sortie;				//!< Sortie in-progress.

public:
	struct
	MISSION_RESULTS*	m_missionResults;		//!< Last held mission result.
};

extern void* teulGetPersonaData(SINT32);

class PlayerInterface
{
public:
	static Player* Lookup(SINT32 teulIndex) { return (Player*)teulGetPersonaData(teulIndex); }
	static Player* Lookup(UINT32 playerID);
};

extern void mapSendChatUpdates();
extern void mapSendHCUpdates();
extern void mapProcessLogins();

extern void mapCheckSideCounts();

extern WWII_SIDE g_overdog, g_underdog;
