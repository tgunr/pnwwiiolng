#pragma once
// Created: Tue Jul 12 15:31:52 CDT 2005
//
// Describes an instance of a player damaging or reparing an object
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "strt/strtDamageEffects.h"					// Enumerations

#pragma pack(push, 1)
class MissionDamageEvent
{
public:
	MissionDamageEvent()
		: m_playerID(0), m_sortieID(0), m_targetID(0ULL), m_targetType(SDT_INVALID), m_effectType(SDE_INVALID), m_joules(0LL) {}

	MissionDamageEvent(const playerid_t& playerID_, const sortieid_t& sortieID_, const UINT64& targetID_, STRAT_DAMAGE_TARGET targetType, STRAT_DAMAGE_EFFECT effectType, SINT64 joules)
		: m_playerID(playerID_), m_sortieID(sortieID_), m_targetID(targetID_), m_targetType(targetType), m_effectType(effectType), m_joules(joules) {}

	~MissionDamageEvent() {}

public:
	playerid_t			m_playerID;
	sortieid_t			m_sortieID;
	UINT64				m_targetID;
	STRAT_DAMAGE_TARGET	m_targetType;
	STRAT_DAMAGE_EFFECT	m_effectType;
	UINT64				m_joules;
};
#pragma pack(pop)

