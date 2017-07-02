#pragma once
// Date: Sat Apr 23 18:44:26 CDT 2005
//
// Structure the describes statistics for the objective set
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "teul/teulPackers.h"

//tolua_begin
class ObjectiveStats
{
public:
	// Default constructor
	ObjectiveStats() { }

	// Recalculate the values from scratch
	void calculate();

	// Check if the current limits are valid
	void checkLimits(UINT32 currentTime);

	// Check the limits for one particular side
	void checkSideLimits(WWII_SIDE, UINT32);

	// Packer/Unpacker
	SINT32 packer(PackMode mode, ACTION_OBJECT* a) { return teulPackVoid(mode, a->packed, a->offset, this, sizeof(*this)); }

	// Save the stats to the database
	void save();

	// Determine if stats need to be refreshed and if so updat ethem
	void update(WWII_SIDE forSide);

	// Mark the current stats as in need of updating
	void voidStats(WWII_SIDE forSide) { m_updated[forSide] = 0; }

	UINT32 attacks(WWII_SIDE forSide) const { return m_cityAttacks[forSide]; }
	UINT32 attackCapacity() const { return m_cityLimit; }
	UINT32 battles(WWII_SIDE forSide) const { return m_battles[forSide]; }

// Members
public:
	UINT32		m_cityLimit;			// Maximum captures allowed
	UINT32		m_bridgeLimit;			// Maximum bridges allowed
	UINT32		m_cityAttacks[MAX_SIDES];
	UINT32		m_cityDefends[MAX_SIDES];
	UINT32		m_bridgeAttacks[MAX_SIDES];
	UINT32		m_bridgeDefends[MAX_SIDES];
	UINT32		m_cityAvailable[MAX_SIDES];
	UINT32		m_bridgeAvailable[MAX_SIDES];
	UINT32		m_battles[MAX_SIDES];

//tolua_end
private:
	UINT32		m_updated[MAX_SIDES];
//tolua_begin
};

extern ObjectiveStats g_objectiveStats;

//tolua_end

