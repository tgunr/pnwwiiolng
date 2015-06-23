#pragma once
// Created: Tue Sep 20 21:33:40 CDT 2005
//
// Squad Invitation tracking
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "teul/teulTime.h"

class SquadInvite
{
public:
	static const UINT32 EXPIRY_MS = 3 * 60 * 1000;

	SquadInvite(UINT32 squadID=0, const ChatHandle& from=0LL, RANK_NO rank=0)
		: m_squadID(squadID), m_from(from), m_expires(teulClock64 + EXPIRY_MS), m_rank(rank) {}
	~SquadInvite() {}

	bool expired() const { return (teulClock64 >= m_expires); }
	bool isCreation() const { return (m_squadID == 0); }

public:
	UINT32			m_squadID;
	ChatHandle		m_from;
	TeulTimeMS		m_expires;				// Milisecond timestamp
	RANK_NO			m_rank;
};
