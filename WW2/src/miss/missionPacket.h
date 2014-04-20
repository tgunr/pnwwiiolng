#pragma once
// Created: Sat Apr  9 12:49:50 CDT 2005
//
// Describes the MissionPacket structure
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "mm/mmEnums.h"
#include "wwii/wwiiRank.h"
#include "weap/weapSerial.h"
#include "chat/chatHandle.h"
#include "omg/omgMissionSortieData.hxx"

#include "miss/missFlags.h"

class MissionPacket : public MM::SortieData
{
public:
	MissionPacket() : MM::SortieData() { }

	// If you're not leaving your mission but just cleaning
	// up the per-sortie fields...
	void discardSortieData();

	// Clear out all non-persistant values
	void flushVolatiles();

	// Clear out all the hit-array fields
	void resetHitArray();

	// Was the mission successful?
	bool successful(const WEAPON_SPEC&) const;
	bool successfulDuration() const;

public:
	MISSION_TYPE	m_missionType;	// so we can score the silly thing;-)

public:
	FLOAT32			m_killScore;		// Points accrued from scoring kills.
	bool			wasSuccessful : 1;
	bool			rtbPromotion : 1;
	bool			m_altF4ed : 1;
	bool			m_saved : 1;
	bool			m_vehicleResupplied : 1;	// Was the vehicle resupplied?

public:
	WEAPON_SERIAL	rtbReservedWeapon;	// Weapon reserved for them at rtbFacility
};

