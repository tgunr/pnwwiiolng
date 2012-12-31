#pragma once
// Created: Wed Nov 22 14:09:34 CST 2006
//
// Class for managing brigade control votes
// Should probably be renamed Proposal, since it
// has been simplified from a voting scheme to a
// request/veto system
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "teul/teulTime.h"

//////////////////////////////////////////////////////////////////////
// Voting structure, tracks a vote in progress

class Vote
{
public:
	// Modes of voting
	//  NOMINATION - election for a player (60s veto window)
	//	OFFER - completed ELECT now offering role to a player (90s window)
	//  TOREMOVE - election for removal of a player (60s veto window)
	//	REMOVING - player in the process of being removed/resigned/replaced (60s window)
	//	REPLACING - player being replaced at end of timeout
	enum Mode { NOMINATION, OFFER, TOREMOVE, REMOVING, TOREPLACE, TOMOVE, TOATTACK, TOCLEAR, TODEFEND, MAX_VOTE_MODES } ;

	Vote(Mode voteMode, UINT32 forAction, const ChatHandle& nominee, const ChatHandle& sponsor, UINT32 duration, const FULL_OBJ_ID& origin=nullObjID)
		: m_voteMode(voteMode), m_forAction(forAction)
		, m_nominee(nominee), m_sponsor(sponsor)
		, m_startedAt(teulTime()), m_closeAt(teulTime() + duration)
		, m_origin(origin)
	{
	}

public:
	static bool IsTargettedAction(UINT32 forAction)
	{
		return ( forAction == TOMOVE || forAction == TOATTACK || forAction == TOCLEAR || forAction == TODEFEND ) ;
	}
	bool isTargettedAction()
	{
		return IsTargettedAction(m_voteMode) ;
	}

public:
	bool expired() const { return m_closeAt < teulTime() ; }

public:
	Mode		m_voteMode ;		// See enums
	UINT32		m_forAction ;
	// Vote is either for a player or a destination/target
	ChatHandle	m_nominee ;			// Player being proposed
	ChatHandle	m_sponsor ;			// Player who sponsored them
	UINT32		m_startedAt ;		// When the vote begins
	UINT32		m_closeAt ;			// When to close the vote
	FULL_OBJ_ID	m_origin ;			// Where it wants to move from
} ;

