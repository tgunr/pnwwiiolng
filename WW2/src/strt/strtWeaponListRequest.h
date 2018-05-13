#pragma once
// Date: Wed Jun 13 15:30:26 CDT 2007
//
// Request a list of weapon availability by brigade/facility/permissions
//
// Copyright (C) PlayNet, Inc 1999-2007 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "wwii/wwiiFullObjID.h"			// For FULL_OBJ_ID

#pragma pack(push, 1)
struct WeaponListRequest
{
	WeaponListRequest(UINT32 playerID, UINT32 brigadeID, const FULL_OBJ_ID withFullObjID
						, const PermissionMask& permMask
						, bool allWeapons, bool isRefresh
						)
		: m_playerID(playerID), m_brigadeID(brigadeID), m_fullObjID(withFullObjID)
		, m_permMask(permMask)
		, m_allWeapons(allWeapons), m_isRefresh(isRefresh)
		{}
	WeaponListRequest() {}

	bool send();

	UINT32			m_playerID;		// ID of the player issueing the request
	UINT32			m_brigadeID;	// ID of the briagde
	FULL_OBJ_ID		m_fullObjID;	// the facility we want to spawn at
	PermissionMask	m_permMask;		// Permission mask requested
	bool			m_allWeapons;	// Flag to display all the weapons instead
									//of doing any sort of limiting
	bool			m_isRefresh;	// whether or not we are requesting a refresh
};
#pragma pack(pop)
