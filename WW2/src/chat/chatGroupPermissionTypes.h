#pragma once
// Created: Mon Feb  2 18:37:26 CST 2009
//
// Structure that describes permissions for a particular
// chat group/aggregate
//
// Copyright (C) PlayNet, Inc 1999-2009 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "mm/mmEnums.h"
#include "wwii/wwiiOwnershipEnums.h"
#include "teul/teulPackers.h"

#if platBuild == BUILD_SERVER
#	include <set>
#endif

// Description of what masks a group creates
struct ChatPermissions
{
	ChatPermissions()
		: m_permissionMask(0ULL), m_lastModified(0)
		, m_groupOID(0), m_enabled(false), m_limitSquadRank(0)
#if platBuild == BUILD_SERVER
		, m_vehicleGrants()
#endif
	{
		m_limitRank[0] = m_limitRank[1] = m_limitRank[2] = m_limitRank[3] = 0;
	}

	PermissionMask	m_permissionMask;
	UINT32			m_lastModified;
	UINT16			m_groupOID;
	bool			m_enabled;
	RANK_NO			m_limitRank[MAX_BRANCHES];
	RANK_NO			m_limitSquadRank;

#if platBuild == BUILD_SERVER
	using VEHTYPES = std::set<UINT16>;
	VEHTYPES		m_vehicleGrants;
#endif

	void packer(LOGICAL mode, ACTION_OBJECT* a)
	{
		a->packer(mode, &m_permissionMask, 0);
		a->packer(mode, &m_limitRank[BRANCH_LAND], 0);
		a->packer(mode, &m_limitRank[BRANCH_AIR], 0);
		a->packer(mode, &m_limitRank[BRANCH_SEA], 0);
		a->packer(mode, &m_limitSquadRank, 0);
	}
};

extern const ChatPermissions nullChatPermissions;

