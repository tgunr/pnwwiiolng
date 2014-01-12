#pragma once
// Created: Wed Jul 26 16:16:51 CDT 2006
//
// Defines the host implementation of a hulk
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "chat/chatServerSTO.h"
#include "chat/chatSTODamage.h"

class HulkSTO : public ServerSTO
{
public:
	HulkSTO(UINT16 subType, const WorldPosition_m& position, const AttitudeVector& attitude, const class Player* originator);
	HulkSTO(ACTION_OBJECT* const a);
	virtual const char* typeName() const OVERRIDE_FN { return "hulk"; }

public:
	virtual bool applyJoules(const SINT64& joules, class Player* player=nullptr) OVERRIDE_FN;

public:
	const STODamage& damage() const { return m_damage; }
	STODamage& damage() { return m_damage; }

private:
	STODamage	m_damage;					//!< Track damage done.
};

