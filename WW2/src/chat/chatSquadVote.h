#pragma once
// Created: Tue Oct  4 13:31:13 CDT 2005
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include <map>

class Player;
class SquadList;

class SquadVote
{
public:
	SquadVote() : m_started(0), m_closedUntil(0), m_counts(nullptr), m_votes(nullptr), m_colead(false) {}
	~SquadVote()
	{
		if ( m_votes ) delete m_votes;
		if ( m_counts ) delete m_counts;
	}

public:
	typedef std::map<ChatHandle,UINT32> COUNTS;
	typedef std::map<ChatHandle,ChatHandle> VOTES;

private:
	void startVote();

public:
	UINT32		m_started, m_closedUntil;
	COUNTS*		m_counts;
	VOTES*		m_votes;
	ChatHandle	m_leader;
	UINT32		m_lead;
	bool		m_colead;

	void addVote(UINT32 squadID, const ChatHandle voteBy, const ChatHandle voteFor);
	void announce(UINT32 squadID, Player* player=nullptr);
	void close(UINT32 duration=0);				// 0 = use default
	void erase(UINT32 squadID);				// Close it and erase it
	UINT32 expires() const;
	bool hasVoted(const ChatHandle voter) { return (m_votes && m_votes->find(voter) != m_votes->end()); }
	UINT32 nextStart() const;
	void recount();
	UINT64 winner(SquadList* list);
	void withdraw(UINT32 squadID, const ChatHandle voterHandle, SquadList* list);
};

SquadVote& chatGetSquadVote(UINT32 squadID);
SquadVote* chatFindSquadVote(UINT32 squadID);
