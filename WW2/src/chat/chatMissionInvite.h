#pragma once
// Created: Tue Mar 13 15:03:55 CDT 2007
//
// Packet describing an invitation to join another player on
// another mission
//
// Copyright (C) PlayNet, Inc 1999-2007 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "mm/mmEnums.h"
#include "wwii/wwiiHCUnitsIndex.h"

class MissionInvite
{
public:
	MissionInvite()
		: m_missionID(0), m_countryID(WWII_NO_COUNTRY), m_branchID(BRANCH_NOT_USED), m_brigadeID(HCUnitID::null)
		, m_fromHandle(0LL), m_squadHandle(0LL), m_isDirective(false)
			{}
	MissionInvite(UINT32 mission, WWII_COUNTRY country, BRANCH_ENUM branch, HCUnitID brigade, ChatHandle fromHandle, ChatHandle squadHandle, bool isDirective)
		: m_missionID(mission)
		, m_countryID(country)
		, m_branchID(branch)
		, m_brigadeID(brigade)
		, m_fromHandle(fromHandle)
		, m_squadHandle(squadHandle)
		, m_isDirective(isDirective)
			{}
	MissionInvite(ACTION_OBJECT* const a);

public:
	SINT32 packer(PackMode mode, ACTION_OBJECT* a);

public:
	UINT32			m_missionID;		// 0 or ID of active mission
	WWII_COUNTRY	m_countryID;		// Country of mission
	BRANCH_ENUM		m_branchID;			// What branch
	HCUnitID		m_brigadeID;		// The brigade it's in
	ChatHandle		m_fromHandle;		// Handle of the requesting player
	ChatHandle		m_squadHandle;		// Handle of requester's squad (or 0)
	bool			m_isDirective;
};
