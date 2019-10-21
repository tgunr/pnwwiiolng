#pragma once
// Created: Tue Jan 15 12:57:29 CST 2008
//
// Defines the HostVehicle structure.
//
// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "weap/weapDefs.h"		// For MAX_WEAP_STATIONS
#include "inst/instDescriptor.h"
#include "host/host.h"
#include "updt/updtTypes.h"

class DespawnTimer
{
public:
	void reset() { m_despawn = m_promotion = -1; }
	SINT32 m_despawn;
	SINT32 m_promotion;
};

class Viewing;
using Viewings = std::vector<Viewing>;

class HostVehicleImpl;
struct WEAPON;

namespace Grid { class Cell; }
namespace Teul { class Packet; }

extern const char* chatGetCallsign(const ChatHandle handle);

/// @class	HostVehicle
/// @brief	Describes an avatar (infantry, vehicle, etc) in the game world.
//	@see	wwiiPlayer
//	@see	Player

class HostVehicle
{
public:
	static HostVehicle* Lookup(const ChatHandle handle);

public:
	void initialize(struct VEHICLE_HOLDER* _holder, class HostVehicleImpl* impl, UINT32 index, const struct WEAPON_SERIAL& ws, const struct WEAPON* weapon);

	void acceptResupply(UINT32 fromSerial, const ChatHandle fromHandle, HostVehicle* fromVehicle, const UINT32 fromCellIP);
	class Player* activeCommander() const;
//	void addVisListRemove(struct GRID_HOLDER* const);

	void attachControllingPlayer(const Player& player);

	void interruptCapture();

	UINT32 brigadeID() const { return vehicleData.brigadeID; }
	const char* callsign() const { return vehicleData.callsign; }
	bool canDespawnAt(const HostVehicle*) const;
	bool canSeeHiddenBuzzards() const;
	void checkCapture();
	void checkLocality(Teul::Packet&);
	void findClosestCPs(SINT32 x, SINT32 y, WWII_SIDE side, UINT16 &closestEnemyDist, UINT16 &closestFriendlyDist);
	// Check if the clock sync has expired
	bool clockExpired() const;
	class Player* commander() const;
	DespawnTimer& despawnTime() const;
	MISS_FLAGS& flags() { return m_flags; }	// Flags the commander provided.
	const MISS_FLAGS& flags() const { return m_flags; }
	void forceExit();
	bool hasConnectedPlayers() const;
	void issueResupply(const Player* const player, UINT32 toSerial);
	bool isActive() const { return m_isActive; }
	bool isBuzzard() const { return m_isBuzzard; }
	bool isHiddenBuzzard() const { return m_isBuzzard && (vehicleData.stateFlags0 & 0x01) == 0; }
	WWII_SIDE isInCaptureBuilding() const;
	bool isInBuilding() const { return m_inBuilding != 0LL; }
	bool isMoving() const;
	bool isMultiCrewed() const { return players().empty() == false && players().size() > 1; }
	bool isReallyActive() const { return m_isReallyActive; }
	bool shouldBeReallyActive() const { return spawnedTime() > ACTIVE_TO_REALLY_ACTIVE_TIME_S; }
	bool isRemote() const { return m_isRemote; }
	bool isVehicleToast();
	bool hasBeenNotifiedOfVehicleDeath() { return m_deathNoticeNeeded; }
	void setVehicleNeedsDeathNotice( bool notice ) { m_deathNoticeNeeded = notice; }

	UINT32 killedBySortieID() const { return m_killedBySortieID; }

	UINT32 maxVisibleVehicles() const;
	UINT32 missionID() const { return m_missionID; }
	MissionPacket& missionPacket() { return m_missionPacket; }
	const MissionPacket& missionPacket() const { return m_missionPacket; }
//	void removeVehicleFromVisList(HostVehicle* vehicle);
	void scoreResupply(const ChatHandle ofHandle);
	void selectBestFramingMode();
	void sendCaptureContribution(Player* player, UINT32 status);
	void sendCaptureState(const class CaptureState& capState);
	void setInBuilding(FULL_OBJ_ID newBuilding);
	void setInCaptureBuilding(bool newValue);
	void setNextUpdate(UINT32 delay) { m_nextUpdateClock = teulClock64 + delay; }
	UINT32 getOneTimeBitsFor(const HostVehicle* rhs) const;
	bool getHideCallsign(const HostVehicle* rhs) const;
	void setCrewDead();
	void setDespawnTimer(bool on);
	bool setVehicleKilled(sortieid_t bySortieID=0);
	const WEAPON_SERIAL& serial() const { return vehicleData.wSerial; }
	WEAPON_SERIAL& serial() { return vehicleData.wSerial; }
	WWII_SIDE side() const { return m_side; }
	UINT32 sortieID() const { return index; }	// For now ...
	WEAPON_SPEC spec() const { return serial().spec; }
	UINT32 spawnedTime() const { return ( m_spawnTime > 0 ) ? teulTime() - m_spawnTime : m_spawnTime; }
	UINT32 squadID() const { return vehicleData.squad; }
	void updateStrat(bool forceFullUpdate);

	const PLAYER_LIST& players() const;
	PLAYER_LIST& players();

	SINT32 packFirstTime(PackMode mode, UCHAR* data, SINT32 offset);

	//! Send the specified packet to ALL players attached to me,
	//! with protection against them CTHLing midway.
	void sendPacket(Teul::Packet& packet);

	// World update functions.
	SINT32 UpdateVehicleList();
	SINT32 CreateWorldUpdate();
	void SendRemoveList();

	// Grid functions.
	bool inCorrectGrid() const noexcept;
	bool updateGrid();
	void removeFromGrid();

	Viewings::iterator removeFromVisList(Viewings::iterator& it);

public:
	// Interface functions
	ChatHandle chatHandle() const { return vehicleData.packedCallsign; }

public:
	void setActive(bool newState) { m_isActive = newState; }
	void setInstance(const Instance::Descriptor&);
	void setReallyActive(bool newState) { m_isReallyActive = newState; }
	void setRemote() { m_isRemote = true; }
	const Instance::Descriptor& instance() const { return m_instance; }

	bool message(int prefix, int msgid, const char* t1=nullptr, const char* t2=nullptr) const;

	// Presence functions.
public:
	instanceid_t instID() const { return m_instance.id(); }
	SINT32 x_cm() const { return vehicleData.position.x(); }
	SINT32 y_cm() const { return vehicleData.position.y(); }
	SINT32 z_cm() const { return vehicleData.position.z(); }

public:
	UINT32				index; 	// index is a copy of weapon.serial

	// Booleans
	bool				m_isRemote;
	bool				m_isActive;
	bool				m_isReallyActive;
	bool				m_isFighter;
	bool				m_despawnTimerActive;
	bool				m_isBuzzard;
	bool				m_hasBeenKilled;
	bool				m_isInCaptureBuilding;	// True when we're in a capture building.
	bool				m_deathNoticeNeeded;

	bool				wasCapPendingLastTick = false; // These 2 are used to stop the capture-
	UINT16				previousCaptureTimeRemaining = 0; // timer getting stuck at 1 second.

	HostVehicleImpl*	m_impl;
	Grid::Cell*			m_cell;

	struct WORLD_UPDATE* m_worldUpdate;
	VEHICLE_HOLDER*		m_holder;

	PLAYER_EXIT_CODE	exitState;

	Viewings			m_visList;
	UINT32				m_visListPressure;	// Actual number of vehicles that want to be on our vis list.
//	Shooters			m_shooters;

	const WEAP_BIAS*	m_bias;				// Vis bias, pointer to vehicles instance
	UINT64				m_maxLod_cm2;		// Maximum distance I can be seen from in cm2.

	SHARED_VEHICLE_INFO	vehicleData;
	UINT32				friendlyBias;
	UINT32				enemyBias;

	UINT32				mtu;
	UINT32				framingMode;
	UINT32				m_maxVehiclesPerUpdate;
	TeulTimeMS			lastRX;
	TeulTimeMS			m_lastVisListUpdate;
	UINT32				m_lastUpdateCount;			// No. of vehicles in previous update
	UINT32				m_updateInterval;

	Instance::Descriptor m_instance;

		// Track the despawn timer
	TeulTimeMS			m_despawnStarted;
	TeulTimeMS			m_enteredBuilding;
	UINT64				m_inBuilding;
	TeulTimeMS			m_nextUpdateClock;
	TeulTimeMS			m_nextCaptureTick;
	TeulTimeMS			m_guardingSince;
	TeulTimeMS			m_nextLocUpdate;

//	WEAPON_SERIAL		wSerial;
	WWII_SIDE			m_side;
	VEHICLE_STATE		state;

	MissionPacket		m_missionPacket;

	UINT32 				seats[MAX_WEAP_STATIONS]; // expected player id's
//	UINT32				m_personas[MAX_WEAP_STATIONS]; //persona id's

//	DAMAGE_NODE*			damageList;
//	SINT32				numDamage;
//	STATION_REF			stationList;
	UINT32				serverAddress;

	UINT32 				timeOfDeath;
//	UPDATE_LIST			shooters;
	// Serial number of the sortie we think killed us
	TeulTimeMS			m_lastHitTime;
	UINT32				m_killedBySortieID;

	UINT32				syncTime;			// game world time sync
	UINT32				m_spawnTime;		// Used for determining if player is really active
	SINT32				crew;

	UINT32				m_missionID;
	MISS_FLAGS			m_flags;

	class ObjectStruct*	m_inBuildingObject;
};

// Manage the free list.
namespace Host
{
	namespace Vehicles
	{
		extern void AllocateArray(UINT32 num);
		extern VEHICLE_HOLDER* GetHolder(UINT32 index, const WEAPON_SERIAL& ws, const WEAPON* weapon);
		extern void FreeHolder(VEHICLE_HOLDER* holder);
		extern void FreeArray();
	}
}

extern void updateScheduleVehicle(HostVehicle* vehicle, UINT32 interval);
extern void updateUnscheduleVehicle(HostVehicle* vehicle);

