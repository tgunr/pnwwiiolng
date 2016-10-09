#pragma once
// Created: Thu Sep 29 19:42:46 CDT 2005
//
// Lets the MMHost notify the chat host of a mobile spawn
// on a particular mission
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "teul/teulNetworkObject.h"

struct MobileSpawnWaypoint
{
	MobileSpawnWaypoint() {}
	MobileSpawnWaypoint(UINT32 missionID, UINT32 playerID, const ChatHandle owner
						, const FLOAT64& x, const FLOAT64& y, const FLOAT64& z
						, bool deployed, UINT32 stoUID, UINT32 reason)
		: m_missionID(missionID), m_playerID(playerID), m_ownerCall(owner)
		, m_x(x), m_y(y), m_z(z)
		, m_deployed(deployed)
		, m_stoUID(stoUID)
		, m_reason(reason)
		{}

	SINT32 packer(LOGICAL mode, ACTION_OBJECT* a) { return a->offset = teulPackVoid(mode, a->packed, a->offset, this, sizeof(*this)); }

	UINT32		m_missionID;				// ID of the mission
	UINT32		m_playerID;
	ChatHandle	m_ownerCall;				// Callsign of the owner
	FLOAT64		m_x, m_y, m_z;				// Co-ordinates
	bool		m_deployed;					// So we can tell its state
	UINT32		m_stoUID;					// STO if any associated with this
	UINT32		m_reason;					// MSPAWN_RESULT code.
};

using MOBILE_SPAWN_WAYPOINT = teul_network_object<MobileSpawnWaypoint>;

