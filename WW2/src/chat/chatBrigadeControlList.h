#pragma once
// Created: Mon Oct  9 14:33:23 CDT 2006
//
// Defines the classes used in sending a list of
// brigades and who controls them
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

//////////////////////////////////////////////////////////////////////
// Describes an entry in a brigade control list

#pragma pack(push, 1)
class BrigadeControlList
{
public:
	BrigadeControlList(const UINT16 brigade_=0, bool canVote_=false) // need default values for the client side
		: m_brigade(brigade_)
		, m_officer(0)
		, m_officerRank(0)
		, m_canTakeCharge(false)
		, m_canVote(canVote_)
	{
	}
	~BrigadeControlList() {}

public:
#ifdef SOON
	// States
	enum
	{
		_INVALID	// Garbage data
	,	_ATTACK		// Attacking
	,	_DEFEND		// Defending
	,	_RESERVE	// Resupply/not doing anything
	,	_ROUTED		// Off-map/routed (unavailable)
	,	_TRANSIT	// Brigade is in transit
	} ;
#endif

	// Get country/branch info from main brigade list
	UINT16		m_brigade ;			// ID of the brigade
	ChatHandle	m_officer ;			// Callsign of active OIC or NULL
	RANK_NO		m_officerRank ;		// His rank
	bool		m_canTakeCharge ;	// True if you can take charge of this brigade
	bool		m_canVote ;			// Can I vote on this brigade's OIC?
} ;
#pragma pack(pop)

