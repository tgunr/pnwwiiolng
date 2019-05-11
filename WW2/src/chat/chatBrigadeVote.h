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
	enum Mode { 
		NOMINATION,			//  NOMINATION - election for a player (60s veto window)
		OFFER,				//	OFFER - completed ELECT now offering role to a player (90s window)
		TOREMOVE,			//  TOREMOVE - election for removal of a player (60s veto window)
		REMOVING,			//	REMOVING - player in the process of being removed/resigned/replaced (60s window)
		TOREPLACE, 
		TOMOVE, 
		TOATTACK, 
		TOCLEAR, 
		TODEFEND, 
		CHANGECOUNTRY, 
		MAX_VOTE_MODES };

	Vote(Mode voteMode, BrigadeControl::BRIGADE_ACTION_REQ forAction, const ChatHandle nominee, const ChatHandle sponsor, UINT32 duration, WWII_COUNTRY country=WWII_NO_COUNTRY, const FULL_OBJ_ID origin=nullObjID)
		: m_voteMode(voteMode), m_forAction(forAction)
		, m_nominee(nominee), m_sponsor(sponsor)
		, m_startedAt(teulTime()), m_closeAt(teulTime() + duration)
		, m_origin(origin)
		, m_country(country)
	{
	}

public:
	static bool IsTargettedAction(UINT32 forAction)
	{
		return ( 
			forAction == TOMOVE || 
			forAction == TOATTACK || 
			forAction == TOCLEAR || 
			forAction == TODEFEND ||
			forAction == CHANGECOUNTRY );
	}
	bool isTargettedAction()
	{
		return IsTargettedAction(m_voteMode);
	}

public:
	bool expired() const { return m_closeAt < teulTime(); }

public:
	Mode		m_voteMode;		// See enums
	BrigadeControl::BRIGADE_ACTION_REQ		m_forAction;
	// Vote is either for a player or a destination/target
	ChatHandle	m_nominee;			// Player being proposed
	ChatHandle	m_sponsor;			// Player who sponsored them
	UINT32		m_startedAt;		// When the vote begins
	UINT32		m_closeAt;			// When to close the vote
	FULL_OBJ_ID	m_origin;			// Where it wants to move from
	WWII_COUNTRY m_country;			// target country (for garrison move)
};

