#pragma once
// Created: Tue Sep  6 20:18:01 CDT 2005
//
// Describes a single member of a mission
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "weap/weapSpec.h"

static const int INVALID_GRID_COORD = 0x7fffffff;

class MissionMember final
{
public:
	MissionMember(ChatHandle handle=0LL, RANK_NO rank=0, SINT32 x=INVALID_GRID_COORD, SINT32 y=INVALID_GRID_COORD, WEAPON_SPEC weapon=NULL_WEAP_SPEC)
		: m_handle(handle), m_rank(rank), m_x(x), m_y(y), m_weapon(weapon) {}

	MissionMember(ACTION_OBJECT* const a);

public:
	SINT32 packer(PackMode mode, ACTION_OBJECT* a);
	bool isSpawned() const { return (m_weapon.wType != 0 && m_x != INVALID_GRID_COORD); }

public:
	ChatHandle		m_handle;				// Always set
	RANK_NO			m_rank;					// Set to zero to indicate leaving
	SINT32			m_x, m_y;				// X/Y co-ords
	WEAPON_SPEC		m_weapon;				// What they're in
};
