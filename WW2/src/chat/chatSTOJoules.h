#pragma once
// Created: Tue Apr  4 14:20:37 CDT 2006
//
// Apply joules of damage to an STO
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "teul/teulNetworkArray.h"

class STOJoules
{
public:
	STOJoules(UINT32 uid, const SINT64& joules) : m_uid(uid), m_joules(joules) {}
	STOJoules(ACTION_OBJECT* const a) : m_uid(0), m_joules(0LL) { packer(TEUL_UNPACK, a); }
	STOJoules() : m_uid(0), m_joules(0LL) {}
	~STOJoules() {}

public:
	SINT32 packer(LOGICAL mode, ACTION_OBJECT* a);

public:
	UINT32		m_uid;						// UID of the STO being damaged
	SINT64		m_joules;					// Joules being done
};

typedef teul_network_array<STOJoules> STO_JOULES;
