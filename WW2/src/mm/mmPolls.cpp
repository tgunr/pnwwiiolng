#include <syscfg.h> // Path is project specific.
// Date: Thu Feb 16 13:20:35 CST 2006
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "include/mydbQuery.h"
#include "map/mapPlayer.h"
#include "teul/teulTime.h"
#include "wwii/wwiiDBThreads.h"
#include "wwii/wwiiOpinionPoll.h"
#include "teul/teulBasic.h"

//////////////////////////////////////////////////////////////////////

typedef std::map<UINT32,OpinionPoll> _POLLS;
static _POLLS polls;
static UINT32 g_maxPoll = 0;
static std::mutex _pollsLock;

//////////////////////////////////////////////////////////////////////

static const char* _FETCH_POLL_QUERY =
	"SELECT		p.pollid, IF(e.entryid=0,p.title,''), e.entry"
		" FROM	wwii_poll p"
			" LEFT JOIN wwii_poll_entry e USING (pollid)"
	" WHERE"
		" opened IS NOT NULL"
			" AND opened < now()"
			" AND (closed IS NULL OR closed > now())"
	" ORDER BY 1, entryid"
;

static void
_parseQuery(DBQuery& query)
{
	// Build a new poll table first,
	_POLLS newPolls;
	UINT32 maxPoll = 0;

	while ( query.FetchRow() )
	{
		const DBRow& row = query.Row();
		const UINT32 pollID = atoi(row[0]);
		if ( pollID <= 0 )
			continue;

		if ( pollID > maxPoll )
			maxPoll = pollID;

		OpinionPoll& poll = newPolls[pollID];
		if ( poll.empty() )
		{
			threadLog(_FLD, "poll:%u %s", pollID, row[1]);
			poll.m_pollID = pollID;
			poll.m_title = row[1];
		}

		poll.addEntry(row[2]);
	}

	// Now copy them over with a lock.
	std::lock_guard<std::mutex> lock(_pollsLock);
	polls = newPolls;
	g_maxPoll = maxPoll;
}

class FetchPollQuery : public DBThreadQuery
{
public:
	FetchPollQuery() : DBThreadQuery(DBThread::Default, "FetchPollQuery", _FETCH_POLL_QUERY, DBThreadQuery::_IMMEDIATE) {}

public:
	virtual void Success() OVERRIDE_FN
	{
		if ( Query()->Rows() > 0 )
		{
			try
			{
				_parseQuery(*Query());
			}
			catch ( std::exception& e)
			{
				threadLog(_FLW, "Polls: %s", e.what());
			}
		}

		threadLog(_FLD, "%u polls, highest %u", polls.size(), g_maxPoll);
	}
};

//////////////////////////////////////////////////////////////////////
// Periodically poll for a new

void
mmFetchOpinionPolls(bool force)
{
	static const UINT32 _POLL_INTERVAL = 15 * 60;
	static UINT32 nextPoll = 0;

	if ( !force && teulTime() < nextPoll )
		return;

	(new FetchPollQuery())->Queue();

	nextPoll = teulTime() + _POLL_INTERVAL;
}

//////////////////////////////////////////////////////////////////////
// Check to see if there is a new opinion poll for a particular player

static OpinionPoll*
_checkPolls(Player* player)
{
	if ( player->m_lastPoll >= g_maxPoll )
		return nullptr;

	_POLLS::iterator it;
	for ( it = polls.begin(); it != polls.end(); ++it )
	{
		if ( it->first > player->m_lastPoll )
			return &(it->second);
	}

	return nullptr;
}

bool
mmCheckOpinionPolls(Player* player)
{
	if ( player->teulIndex() < 0 )
		return false;

	std::lock_guard<std::mutex> lock(_pollsLock);
	try
	{
		const OpinionPoll* const sendPoll = _checkPolls(player);

		if ( sendPoll )
		{
			OPINION_POLL_MSG outgoing(*sendPoll);
			outgoing.send(player->teulIndex());

			player->m_lastPoll = sendPoll->pollID();
			player->m_allowPollVote = true;
		}
	}
	catch ( std::exception& e )
	{
		dbugLog(_FLW, "%s", e.what());
	}

	return true;
}

//////////////////////////////////////////////////////////////////////
// Handle an incoming result from a player

template<>
SINT32
OPINION_CHOICE_MSG::receive(SINT32 teulIndex, OPINION_CHOICE_MSG& incoming)
{
	// Check we know someone on this connection
	Player* player = PlayerInterface::Lookup(teulIndex);
	if ( player == nullptr )
		return 0;

	// Is it the same poll we've asked the player about?
	OpinionChoice& data = incoming.data();
	if ( data.pollID() != player->m_lastPoll )
		return 0;

	// Are they allowed to vote?
	if ( !player->m_allowPollVote )
		return 0;

	DBThread::Do(DBThread::Default
		, "UPDATE LOW_PRIORITY wwii_player SET last_poll = %u WHERE playerid = %u AND last_poll < %u"
			, player->m_lastPoll
			, player->playerID()
			, player->m_lastPoll
	);

	DBThread::Do(DBThread::Default
		, "UPDATE LOW_PRIORITY wwii_poll_entry SET votes = votes + 1 WHERE pollid = %u AND entryid = %u"
			, player->m_lastPoll
			, data.choice()
	);

	// Prevent them from voting again
	player->m_allowPollVote = false;

	return 0;
}

