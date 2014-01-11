#include <syscfg.h> // Path is project specific.
// Date: Tue Oct  4 13:29:53 CDT 2005
//
// class SquadVote and friends
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "chat/chatHandle.h"
#include "chat/chatHost.h"
#include "chat/chatTimeInterval.h"
#include "chat/chatSquadVote.h"
#include "chat/chatSquadListTypes.h"
#include "chat/chatSquadList.h"
#include "chat/chatPlayer.h"
#include "wwii/wwiiConfig.h"

#include "wwii/wwiiHash.h"
typedef std::unordered_map<UINT32,SquadVote> SQUAD_VOTES;

static SQUAD_VOTES g_squadVotes;

//////////////////////////////////////////////////////////////////////
// Register a vote

void
SquadVote::addVote(UINT32 squadID, const ChatHandle& voteBy, const ChatHandle& voteFor)
{
	if ( m_started == 0 )
		startVote();
	(*m_votes)[voteBy] = voteFor;
	UINT32 votes = ++(*m_counts)[voteFor];
	if ( votes > m_lead )
	{
		m_colead = false;
		m_leader = voteFor;
		m_lead = votes;
	}
	else if ( votes == m_lead )
		m_colead = true;

	if ( m_votes->size() == 1 )
		announce(squadID, nullptr);
}

//////////////////////////////////////////////////////////////////////
// Announce the existence of a squad vote either to a player or to
// the entire squad if player is NULL

void
SquadVote::announce(UINT32 squadID, Player* player)
{
	if ( !m_votes )
		return;

	SquadList* list = nullptr;
	if ( player == nullptr )
		list = chatFindSquadList(squadID);
	if ( player == nullptr && list == nullptr )
		return;

	char str[40+1];
	std::string results = "";

	for ( COUNTS::iterator it = m_counts->begin(); it != m_counts->end(); ++it )
	{
		if ( results.size() > 0 ) results += ", ";
		strprintf(str, "%s (%u)", Chat::Name(it->first).c_str(), it->second);
		results += str;
	}

	strprintf(str, "%" PRSizeT "", m_votes->size());

	if ( player != nullptr )
	{
		player->message(LSTR_SQUAD, LSTR_SQUAD_ELECTION, str, results.c_str());
		if ( m_votes->find(player->matchHandle()) == m_votes->end() )
			player->message(LSTR_SQUAD, LSTR_SQUAD_NOT_VOTED, chatTimeInterval(expires()));
	}
	else
	{
		list->sendMessage(LSTR_SQUAD, LSTR_SQUAD_ELECTION, str, results.c_str());
		for ( SQUAD_MEMBERS::iterator it = list->begin(); it != list->end(); ++it )
		{
			SquadMemberEntry& member = it->second;
			if ( member.teulIndex() < 0 ) continue;
			if ( m_votes->find(member.player()->matchHandle()) != m_votes->end() ) continue;
			member.player()->message(LSTR_SQUAD, LSTR_SQUAD_NOT_VOTED, chatTimeInterval(expires()));
		}
	}
}

//////////////////////////////////////////////////////////////////////
// Close voting on a vote for <duration> seconds. If duration is zero
// then we use the default specified (in days) in squadElectionInterval

void
SquadVote::close(UINT32 duration)
{
	if ( m_closedUntil != 0 )
		return;

	if ( duration == 0 )
	{
		duration = host_config.squad.electionInterval * DAY_S;
		if ( duration == 0 )
			duration = 7 * DAY_S;
	}

	m_started = 0;
	m_closedUntil = teulTime() + duration;
	if ( m_counts )
	{
		delete m_counts;
		m_counts = nullptr;
	}
	if ( m_votes )
	{
		delete m_votes;
		m_votes = nullptr;
	}
	recount();
}

void
SquadVote::erase(UINT32 squadID)
{
	close();
	g_squadVotes.erase(squadID);
}

//////////////////////////////////////////////////////////////////////
// Return the number of seconds until the squad vote expires

UINT32
SquadVote::expires() const
{
	if ( m_started == 0 )
		return 0;
	const UINT32 cutoff = m_started + host_config.squad.electionDuration * DAY_S;
	if ( teulTime() > cutoff )
		return 0;
	return (UINT32)(cutoff - teulTime());
}

//////////////////////////////////////////////////////////////////////
// When is the next vote allowed to start?

UINT32
SquadVote::nextStart() const
{
	return m_closedUntil;
}

//////////////////////////////////////////////////////////////////////
// Recount the leader in a vote

void
SquadVote::recount()
{
	m_leader = 0LL;
	m_lead = 0;
	m_colead = false;

	if ( m_counts == nullptr || m_counts->empty() )
		return;

	COUNTS::iterator it;
	for ( it = m_counts->begin(); it != m_counts->end(); ++it )
	{
		if ( it->second > m_lead )
		{
			m_leader = it->first;
			m_lead = it->second;
			m_colead = false;
		}
		else if ( it->second == m_lead )
			m_colead = true;
	}
}

//////////////////////////////////////////////////////////////////////
// Reset counters etc and indicate that a new vote has started

void
SquadVote::startVote()
{
	m_closedUntil = 0;
	m_started = teulTime();
	if ( m_counts )
		delete m_counts;
	m_counts = new COUNTS;
	if ( m_votes )
		delete m_votes;
	m_votes = new VOTES;
	m_leader = 0LL;
	m_lead = 0;
	m_colead = false;
}

//////////////////////////////////////////////////////////////////////
// Determine if there is a clear winner of the squad vote

ChatHandle
SquadVote::winner(SquadList* list)
{
	if ( m_started == 0 || m_votes == nullptr || m_counts == nullptr || m_votes->empty() || m_votes->size() <= 1 )
		return 0LL;

	// Check if the leader has left the squad
	if ( !list->find(m_leader) )
	{
		list->sendMessage(LSTR_SQUAD, LSTR_SQUAD_VOTE_FAIL_LEAVER);
		close(HOUR_S);
		return 0ULL;
	}

	if ( m_leader == 0LL || m_colead )
		return 0ULL;

	// Does someone have more half or more of the votes?
	size_t members = list->members().size();
	if ( m_lead > members / 2 )
		return m_leader;

	if ( m_lead < 5 )
		return 0ULL;

	if ( expires() == 0 )
	{
		return m_leader;
	}

	return 0ULL;
}

//////////////////////////////////////////////////////////////////////
// Withdraw the vote of a squad member

void
SquadVote::withdraw(UINT32 squadID, const ChatHandle& handle, SquadList* list)
{
	// Is the vote active?
	if ( !m_started || m_closedUntil || !m_votes || !m_counts )
		return;

	// Has this individual vote
	VOTES::iterator vit;
	COUNTS::iterator cit;
	bool candidateRemoved = false;

	vit = m_votes->find(handle);
	if ( vit != m_votes->end() )
	{
		// Candidate iterator
		cit = m_counts->find(vit->second);
		if ( cit != m_counts->end() )
		{
			// If this is the candidates last vote, withdraw them
			if ( cit->second <= 1 )
				m_counts->erase(cit);
			else
				--(cit->second);
		}

		// Remove the vote
		m_votes->erase(vit);
	}

	// Have people voted for you?
	cit = m_counts->find(handle);
	if ( cit != m_counts->end() )
	{
		for ( vit = m_votes->begin(); vit != m_votes->end(); ++vit )
		{
			if ( vit->second != handle ) continue;
				m_votes->erase(vit);
		}
		m_counts->erase(cit);
		candidateRemoved = true;
	}

	// Retally the vote scores
	recount();

	// Check to see if we still have a vote
	if ( m_lead == 0 )
	{
		list->sendMessage(LSTR_SQUAD, LSTR_SQUAD_VOTES_WITHDRAWN, "*");
		g_squadVotes.erase(squadID);
		// DO NOT PASS HERE
		return;
	}
	else if ( candidateRemoved )
		list->sendMessage(LSTR_SQUAD, LSTR_SQUAD_ALL_VOTES_WITHDRAWN, Chat::Name(handle).c_str());
}

//////////////////////////////////////////////////////////////////////
// Return a reference to a squad vote, forcing creation of one for the
// squad if it doesn't already exist

SquadVote&
chatGetSquadVote(UINT32 squadID)
{
	return g_squadVotes[squadID];
}

//////////////////////////////////////////////////////////////////////
// Look up the vote for a squad if it exists else return NULL

SquadVote*
chatFindSquadVote(UINT32 squadID)
{
	SQUAD_VOTES::iterator it = g_squadVotes.find(squadID);
	return ( it == g_squadVotes.end() ) ? nullptr : &(it->second);
}
