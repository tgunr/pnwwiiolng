#pragma once
// Created: Wed Apr 27 12:42:06 CDT 2005
//
// Mission detail/check reply definitions
//
// Copyright (C) PlayNet, Inc 1999-2014 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "map/mapMissionList.h"
#include "teul/teulNetworkMsg.h"
#include "miss/missionTypes.h"

class MissionDetail : public MissionSynopsis
{
public:
	MissionDetail() : m_result(MC_ERROR),m_timeLaunch(0) { *m_desc=0; }
	~MissionDetail() {}

	SINT32 packer(LOGICAL mode, ACTION_OBJECT* a);
	void populate(const Mission* mission, bool isCurrent, bool isLeader, UINT32 launchDelay);

	UINT32 launchDelay() const;

public:
	MISSION_CODE	m_result;			// Mission check result
	UINT32			m_timeLaunch;		// Host unixtime of launch
	char			m_desc[MapMiss::MISSION_DESC_LENGTH+1];
};

using MISSION_DETAIL = teul_network_msg<MissionDetail>;

