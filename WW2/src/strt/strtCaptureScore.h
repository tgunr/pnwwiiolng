#pragma once
// Date: Mon Sep 10 23:54:29 CDT 2007
//
// Capture score exchange information
//
// Copyright (C) PlayNet, Inc 1999-2007 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

//////////////////////////////////////////////////////////////////////
// Cell->Strat notification of capture contribution

#include "wwii/wwiiFullObjID.h"
#include "wwii/wwiiOwnershipEnums.h"

#pragma pack(push, 1)
//tolua_begin
namespace Strat
{
struct CaptureContributionEvent
{
	enum Status
	{
		Active
	,	Interrupted
	,	Left
	,	Killed
	};
	CaptureContributionEvent() {}
	CaptureContributionEvent(Status withStatus, UINT32 facilityID, const ChatHandle handle, UINT32 playerID, UINT32 playerCountry, UINT32 sortieID, UINT32 missionID, UINT32 brigadeID)
		: m_status(withStatus), m_facilityID(facilityID), m_handle(handle), m_playerID(playerID), m_playerCountry(playerCountry), m_sortieID(sortieID), m_missionID(missionID), m_unitID(brigadeID)
		{}

	Status		m_status;
	UINT32		m_facilityID;	// Facility being affected
	ChatHandle	m_handle;		// Chat handle of player
	UINT32		m_playerID;		// Player ID
	UINT32		m_playerCountry;// Player's country
	UINT32		m_sortieID;		// Vehicle Sortie ID
	UINT32		m_missionID;	// Player's mission
	UINT32		m_unitID;		// Player's brigade
};
}
//tolua_end
#pragma pack(pop)

//////////////////////////////////////////////////////////////////////
// Strat->Cell notification of capture scoring

#pragma pack(push, 1)
struct CaptureScore
{
	CaptureScore() {}
	CaptureScore(const FULL_OBJ_ID fid, const WWII_SIDE side) : m_fullObjID(fid), m_side(side) {}

	FULL_OBJ_ID		m_fullObjID;
	WWII_SIDE		m_side;
};
#pragma pack(pop)
