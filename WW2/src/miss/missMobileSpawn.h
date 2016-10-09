#pragma once
// Created: Fri Dec 10 17:24:22 EST 2004
//
// Prototypes and values specific to the mobile spawn sub-system
//
// Copyright (c) 2004 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "include/mobileSpawnEnums.h"
#include "wwii/wwiiHash.h"

class Mission;

class MobileSpawn
{
	// Functions
public:
	MobileSpawn() : m_active(false), m_playerID(0), m_stoUID(0), m_vehicleID(0), m_spawnees(0) {}

	void activate(class Mission* mission, Player* player, UINT32 stoUID, FLOAT64 x, FLOAT64 y, FLOAT64 z, UINT32 vehicleID=0);

	void deactivate(MSPAWN_RESULT reason, UINT32 missionID);

	void incrementSpawnees(const playerid_t forPlayer) const;

	bool isActive() const { return m_active; }

	// Data
public:
	bool				m_active;			//!< Indicates whether or not this spawner is active.
	playerid_t			m_playerID;			//!< PlayerID of creator/owner.
	UINT32				m_stoUID;			//!< STO we are attached to or 0.
	FLOAT64				m_x, m_y, m_z;		//!< Spawn co-ordinates.
	char				m_callsign[MAX_HANDLE_SIZE];	//!< Callsign of owner.
	ChatHandle			m_handle;			//!< Packed lowercase handle of owner.
	UINT32				m_vehicleID;		//!< ID of vehicle we are attached to or 0.

private:
	using SPAWNEES = std::unordered_map<UINT32, UINT32>;
	mutable SPAWNEES	m_spawnees;
};

// Process an incoming mobile spawn update packet
extern void missMobileSpawnCancel(UINT32 missionID, const ChatHandle ownerID, SINT32 x, SINT32 y);
extern void missAddMobileSpawn(UINT32 missionID, const playerid_t playerID, const ChatHandle handle, const FLOAT64& x, const FLOAT64& y, const FLOAT64& z, UINT32 stoUID);

