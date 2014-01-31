#pragma once
// Created: Thu Jun 03 10:00:00 2004
//
// Definitions private to chatDecisions.cpp; makes it tidier
//
// Copyright (c) 2004 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Oliver "kfsone" Smith

//// ---> Mandatory includes

#include <vector>
#include "strt/strtChatTypes.h"
#include "chat/chatData.h"
#include "wwii/wwiiString.h"
#include "teul/teulTime.h"

//// ---> Typedefs

// Forward declaration
class StratDecision;

// Array's of votes: maybe a userlist would be more appropriate
typedef std::vector<ChatHandle> VOTES;

// Type we use for storing queues of strat events
typedef std::vector<StratDecision*> DECISION_QUEUE;

//// ---> Constants

static const UINT32 OVERRUN_TIMEOUT_MIN = 12;

//// ---> Class/Struct definitions

// Strat Decision is how we store a particular vote and/or result.

class StratDecision
{
public:
	UINT32			m_time;					// When it occured
	WWII_SIDE		m_side;					// Which side it's for
	const Property*	m_property;				// The facility it occured at
	VOTES			m_voters;				// People voting
	UINT32			m_score;				// Highest vote
	UINT32			m_holds;				// People who chose 'hold'
	UINT32			m_falls;				// People who voted 'fall'
	UINT32			m_vetos;				// People who veto'd
	UINT32			m_conceed;				// Conceed at this time if no other votes
	UINT32			m_chosen;				// Which vote won?

	void clear()
	{
		m_voters.clear();
		m_score = m_holds = m_falls = m_vetos = 0;
		m_conceed = 0;
		m_chosen = SC_OVERRUN;
	}

	StratDecision(WWII_SIDE side, const Property* property)
		: m_time(teulTime()), m_side(side), m_property(property), m_chosen(0)
	{
		clear();
	}

	bool won() const;
	UINT32 eta() const;
	void reminder();
};

