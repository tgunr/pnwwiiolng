#pragma once
// Date: Mon Oct 30 15:05:46 CST 2006
//
// Defines the WeaponsReturn structure used by the
// cell hosts to tell strat that a vehicle is despawning
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "teul/teulPackers.h"			// For ACTION_OBJECT.
#include "updt/updtVehiclePosition.h"

class WeaponsReturn
{
public:
	WeaponsReturn(const UINT32 sortieID_, const FULL_OBJ_ID rtbFacility, const UINT32 exitCode_, const bool isVehicleDead, const bool hasBeenHit, const bool rtbPromoted, const bool resupplyMission)
		: m_sortieID(sortieID_), m_rtbFacility(rtbFacility), m_exitCode(exitCode_), m_isVehicleDead(isVehicleDead), m_hasBeenHit(hasBeenHit), m_rtbPromoted(rtbPromoted)
		, m_resupplyMission(resupplyMission)
		{}
	WeaponsReturn(ACTION_OBJECT* const a)
	{
		a->offset = teulPackVoid(TEUL_UNPACK, a->packed, a->offset, this, sizeof(*this));
	}
	~WeaponsReturn() {}

	SINT32 packer(PackMode mode, ACTION_OBJECT* a)
	{
		return (a->offset = teulPackVoid(mode, a->packed, a->offset, this, sizeof(*this)));
	}

	void process() const;

public:
	// Which vehicle is being returned
	UINT32				m_sortieID;
	// Where in the world they are rtbing
	FULL_OBJ_ID			m_rtbFacility;
	VEHICLE_POSITION	m_position;
	// What state where they in?
	UINT32				m_exitCode;
	bool				m_isVehicleDead;
	bool				m_hasBeenHit;
	bool				m_rtbPromoted;
	bool				m_resupplyMission;
};

