#pragma once
// Created: Tue May 31 07:44:28 CDT 2005
//
// Describes a player's currently active mission, and
// tries to provide all the fields neccessary for the
// player to auto-jump to it without having to do lots
// of queries to get there
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "wwii/wwiiHCUnitsIndex.h"
#include "wwii/wwiiOwnershipEnums.h"
#include "mm/mmEnums.h"
#include "teul/teulPackers.h"

class Mission;

class ActiveMission
{
public:
	ActiveMission(UINT32 mission=0, WWII_COUNTRY country=WWII_NO_COUNTRY, BRANCH_ENUM branch=BRANCH_NOT_USED, HCUnitID brigade = HCUnitID::null)
		: m_missionID(mission)
		, m_countryID(country)
		, m_branchID(branch)
		, m_brigadeID(brigade)
			{}
	ActiveMission(ACTION_OBJECT* const a) { a->offset = packer(TEUL_UNPACK, a); }

public:
	SINT32 packer(PackMode mode, ACTION_OBJECT* a);

// Server only
public:
	SINT32 send(SINT32 teulIndex);
	void populate(const Mission*);

public:
	UINT32			m_missionID;		// 0 or ID of active mission
	WWII_COUNTRY	m_countryID;		// Country of mission
	BRANCH_ENUM		m_branchID;		// What branch
	HCUnitID		m_brigadeID;		// The brigade it's in
};

