#include <syscfg.h> // Path is project specific.
// Created: Sat Jan 23 21:37:16 CST 2010
//
// Implements the auth host login queues
//
// Copyright (C) PlayNet, Inc 1999-2010 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith
//


#include "dbug/dbug.h"
#include "auth/authQueue.h"				// Our header file
#include "teul/teulTime.h"
#include "wwii/wwiiRunningAverage.h"
#include <map>
#include <assert.h>
#include <vector>

// How long do we think the wait time is likely to be?
static const UINT32 _DEFAULT_WAIT_TIME_S = 60;

// How frequently should we send status updates?
static const UINT32 _UPDATE_INTERVAL = 5;

// Entry for individual players
struct	PlayerEntry
{
	PlayerEntry(Auth::ClientData* cdata) : m_cdata(cdata), m_entryTime(teulTime()) {}
	bool operator==(const Auth::ClientData* rhs) const { return m_cdata == rhs; }

	Auth::ClientData* m_cdata;			// Client data for this entry
	UINT32	m_entryTime;				// When they were placed on the queue
};

// Queue of Player Entries
typedef std::vector<PlayerEntry> PlayerQueue;

// Header for a particular queue
struct	QueueHeader
{
	QueueHeader() : m_waitTimes(), m_queue(), m_nextSend(teulTime() + _UPDATE_INTERVAL), m_modified(false), m_hasLaunched(false) {}
	// Track the wait times of the last 10 players.
	RunningAverage<UINT32, 10>	m_waitTimes;
	// Track the players waiting to get in
	PlayerQueue					m_queue;
	// When should we send the next update?
	UINT32						m_nextSend;
	// Has the queue been modified since the last poll?
	bool						m_modified : 1;
	// Has the queue launched anyone yet?
	bool						m_hasLaunched : 1;
};

struct ArenaQueues
{
	QueueHeader m_premium, m_nonPremium;
};

// Arena<->Queue mapping
typedef std::map<UINT32, ArenaQueues> Arenas;

// Individual queues for paid / free customers.
static Arenas arenas;

//	------------------------------	Declarations

//////////////////////////////////////////////////////////////////////
// Calculate how long a player has to wait based on the histogram
// of wait times, their position in the queue and the time they have
// already waited.

static UINT32
_getWaitTime(const PlayerEntry& entry, UINT32 position, const QueueHeader& header)
{
	assert( position > 0 );

	// How long the player has waited.
	UINT32 waited = teulTime() - entry.m_entryTime;

	// Do we have any useful data?
	const UINT32 numEntries = header.m_waitTimes.population();
	const UINT32 avgWaitTime = (numEntries > 0 ? header.m_waitTimes.average() : _DEFAULT_WAIT_TIME_S);
	UINT32 totalWaitTime = avgWaitTime * position;

	static const UINT32 PAD_TIME_S = _DEFAULT_WAIT_TIME_S - 1;

	if ( totalWaitTime <= waited + _DEFAULT_WAIT_TIME_S )
		return PAD_TIME_S;

	// If you have already waited longer then the average wait time,
	// add a pad value.
	if ( waited >= avgWaitTime )
		totalWaitTime += PAD_TIME_S;

	return totalWaitTime - waited;
}

//////////////////////////////////////////////////////////////////////
// Send an update to a player to let them know they are in a queue,
// what position they're at, and what their expected wait time is.

extern void authSendPlayerInQueue(Auth::ClientData* cdata, UINT32 position, UINT32 waitTime);

static void
_sendPlayerInQueue(const PlayerEntry& entry, UINT32 position, const QueueHeader& header)
{
	// Ignore players with a teulIndex of 0.
	if ( entry.m_cdata == NULL )
		return;

	// How long do we expect them to have to wait?
	UINT32 waitTime = _getWaitTime(entry, position, header);
	authSendPlayerInQueue(entry.m_cdata, position, waitTime);
}

//////////////////////////////////////////////////////////////////////
// Batch-erase any dead entries at the top of the queue.

static void
_trimDeadEntries(QueueHeader& header)
{
	// Find the first non-dead entry
	PlayerQueue::iterator it = header.m_queue.begin();
	while ( it != header.m_queue.end() && it->m_cdata == NULL )
	{
		++it;
	}
	if ( it == header.m_queue.begin() )
		return;

	// Ok, we're skipping entries: batch erase.
	header.m_queue.erase(header.m_queue.begin(), it);

	// Don't send update notifications immediately.
	if ( header.m_modified == false && header.m_nextSend <= teulTime() )
		header.m_nextSend = teulTime() + 1;

	header.m_modified = (header.m_queue.size() > 0);
}

//////////////////////////////////////////////////////////////////////
// When a queue hasn't had anyone launch from it yet but it has people
// waiting, use the guy at the front of the queue to estimate launch
// times.

static void
_fakeLaunchEstimates(QueueHeader& header)
{
	header.m_waitTimes.reset();
	header.m_waitTimes.add(teulTime() + header.m_queue.begin()->m_entryTime + _DEFAULT_WAIT_TIME_S);
}

//////////////////////////////////////////////////////////////////////
// Process a specific subscription/arena queue.

static void
_processArenaQueue(QueueHeader& header)
{
	if ( header.m_nextSend > teulTime() )
		return;

	if ( header.m_hasLaunched == false && header.m_queue.empty() == false )
		_fakeLaunchEstimates(header);

	// Any updates to send?
	if ( header.m_modified == false )
		return;

	// Is there a queue to send updates to?
	if ( header.m_queue.empty() )
	{
		// Something set it to true, or we would have failed the first test.
		header.m_modified = false;
		return;
	}

	// So we have modifications to send...
	UINT32 i = 0;
	PlayerQueue::const_iterator it;
	for ( it = header.m_queue.begin(); it != header.m_queue.end(); ++it )
	{
		if ( it->m_cdata != NULL )
		{
			_sendPlayerInQueue(*it, ++i, header);
		}
	}
}

namespace Auth { namespace Queue {

//////////////////////////////////////////////////////////////////////
// Process all of the queues and send updates to players.

void
ProcessQueues()
{
	Arenas::iterator it;
	for ( it = arenas.begin(); it != arenas.end(); ++it )
	{
		_processArenaQueue(it->second.m_premium);
		_processArenaQueue(it->second.m_nonPremium);
	}
}

//////////////////////////////////////////////////////////////////////
// Lookup the next player on a queue.

static Auth::ClientData*
_getNextPlayer(QueueHeader& header)
{
	_trimDeadEntries(header);
	if ( header.m_queue.empty() )
		return NULL;
	return header.m_queue.front().m_cdata;
}

Auth::ClientData*
GetNextPlayer(UINT32 arenaID)
{
	Arenas::iterator it = arenas.find(arenaID);
	if ( it == arenas.end() )
		return NULL;

	ClientData* cdata = _getNextPlayer(it->second.m_premium);
	if ( cdata != NULL )
		return cdata;
	return _getNextPlayer(it->second.m_nonPremium);
}

//////////////////////////////////////////////////////////////////////
// Add a player to an arena's wait queue.

bool
AddPlayer(UINT32 arenaID, Auth::ClientData* cdata, bool premium)
{
	assert( arenaID > 0 );

	dbugLog(_FLI, "cd:%p a:%u type:%s Auth::Queue::AddPlayer", cdata, arenaID, (premium ? "premium" : "free"));

	QueueHeader& header = (premium ? arenas[arenaID].m_premium : arenas[arenaID].m_nonPremium);

	PlayerQueue::iterator it = find(header.m_queue.begin(), header.m_queue.end(), cdata);
	if ( it != header.m_queue.end() )
		return false;

	// Insert the player into that queue
	PlayerEntry entry(cdata);
	header.m_queue.push_back(entry);

	_sendPlayerInQueue(entry, (UINT32)header.m_queue.size(), header);

	return true;
}

//////////////////////////////////////////////////////////////////////
// Remove a player from a queue.

bool
RemovePlayer(UINT32 arenaID, Auth::ClientData* cdata, bool premium, bool enteredGame)
{
	assert( arenaID > 0 );

	Arenas::iterator arena = arenas.find(arenaID);
	if ( arena == arenas.end() )
		return false;

	QueueHeader& header = (premium ? arena->second.m_premium : arena->second.m_nonPremium);

	// Look the player up.
	UINT32 position = 1;
	PlayerQueue::iterator it;
	for ( it = header.m_queue.begin(); it != header.m_queue.end(); ++it, ++position )
	{
		if ( it->m_cdata == cdata )
			break;
	}
	if ( it == header.m_queue.end() )
	{
		dbugLog(_FLD, "cd:%p a:%u type:%s Auth::Queue::RemovePlayer - not in queue", cdata, arenaID, (premium ? "premium" : "free"));
		return false;
	}

	dbugLog(_FLI, "cd:%p a:%u type:%s Auth::Queue::RemovePlayer(%s) from position %u"
					, cdata, arenaID, (premium ? "premium" : "free")
					, (enteredGame ? "to-game" : "disconnected")
					, position
					);

	// Easy dereference
	PlayerEntry& entry = *it;

	// If the player got in, update waitTimes.
	if ( enteredGame )
	{
		if ( header.m_hasLaunched == false )
		{
			header.m_hasLaunched = true;
			header.m_waitTimes.reset();
		}
		header.m_waitTimes.add(teulTime() - entry.m_entryTime);
	}

	// Kill the entry.
	entry.m_cdata = NULL;

	// If the player is at the front of the queue, trimDeadEntries
	// will remove all the dead entries from the front of the queue
	// including our entry.
	_trimDeadEntries(header);

	return true;
}

} }	// namespace Auth::Queue
