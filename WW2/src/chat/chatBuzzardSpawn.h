#pragma once
// Created: Wed Jun 21 17:59:22 CDT 2006
//
// Message for describing a buzzard spawn request
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#pragma pack(push, 1)
class BuzzardSpawn
{
public:
	BuzzardSpawn() {}
	BuzzardSpawn(playerid_t playerID, bool useLocation, const FLOAT64VECTOR& location)
		: m_playerID(playerID), m_location(location), m_useLocation(useLocation) {}

public:
	playerid_t		m_playerID;			// ID of player to spawn
	FLOAT64VECTOR	m_location;			// Location to spawn them at
	bool			m_useLocation;		// Whether to use custom location or not
};
#pragma pack(pop)

