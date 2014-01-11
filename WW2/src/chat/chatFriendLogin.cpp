#include <syscfg.h> // Path is project specific.
// Date: Sun Feb  5 13:29:26 CST 2006
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "chat/chatBroadcastObject.h"
#include "chat/chatPlayer.h"
#include "chat/chatName.h"
#include "include/mydbQuery.h"
#include "wwii/wwiiDBThreads.h"

//	------------------------------	Classes

class FriendLoginQuery : public DBThreadQuery
{
public:
	FriendLoginQuery(const ChatHandle& handle);
	virtual bool Process(const size_t numRows) OVERRIDE_FN
	{
		if ( numRows <= 0 )
			return true;

		while ( Query()->FetchRow() )
		{
			m_players.push_back(Query()->Row().NextUInt());
		}

		return true;
	}

	virtual void Success() OVERRIDE_FN;
public:
	ChatHandle m_handle;
	std::vector<playerid_t> m_players;
};

//	------------------------------	Class Members

//////////////////////////////////////////////////////////////////////
// Constructor for the query

static const char* _QUERY = "SELECT f.playerID FROM chat_friends f WHERE f.target = LOWER('%s')";

FriendLoginQuery::FriendLoginQuery(const ChatHandle& handle)
	: DBThreadQuery(DBThread::Default, "FriendLoginQuery", DBThreadQuery::_CALLBACK, _QUERY, Chat::Name(handle).c_str())
	, m_handle(handle)
	, m_players()
{
}

//////////////////////////////////////////////////////////////////////
// What to do when we've found something

void
FriendLoginQuery::Success()
{
	// No rows, then no reason to do anything else
	if ( m_players.empty() == true )
		return;

	// If the player is somehow no-longer online, then don't do anything
	const Player* player = PlayerInterface::Lookup(m_handle);
	if ( player == nullptr )
		return;

	const size_t numRows = m_players.size();

	// Rather than sending a seperate message to each player,
	// we're going to use broadcast, so we need to build a
	// list of teulindexes.
	std::vector<SINT32 /*teulIndex*/> buddies;
	buddies.reserve(numRows);

	// Iterate through the rows looking for players who are online
	for ( size_t i = 0; i < numRows; ++i )
	{
		Player* buddy = PlayerInterface::Lookup(m_players[i]);

		// Ignore various "invalid" buddies (not online or logged in)
		if ( buddy == nullptr )
			continue;
		if ( buddy->teulIndex() < 0 || buddy->initialized() == false )
			continue;
		if ( buddy->matchHandle() == m_handle )
			continue;

		buddies.push_back(buddy->teulIndex());
	}

	Chat::LocalizedMessage loc(LSTR_NOTE, LSTR_FRIEND_LOGGING_IN, player->callsign());
	chatBroadcastLocMessage(buddies.begin(), buddies.end(), loc);
}

//////////////////////////////////////////////////////////////////////
// Check for friends to notify of your login

void
chatReportFriendLogin(Player* player)
{
	dbugLog(_FLD, "t:%d p:%u c:%s notifying friends", player->teulIndex(), player->playerID(), player->callsign());
	(new FriendLoginQuery(player->matchHandle()))->Queue();
}

