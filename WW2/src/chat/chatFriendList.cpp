#include <syscfg.h> // Path is project specific.
// Date: Sat Feb  4 15:02:43 CST 2006
//
// Functions for manipulating a player's friends list
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "chat/chatLStringIDs.h"
#include "chat/chatName.h"
#include "chat/chatPlayer.h"
#include "include/mydbQuery.h"
#include "wwii/wwiiDBThreads.h"
#include "chat/chatFriendList.h"

using Chat::ARGV;

//	------------------------------	Classes

static const char* const _LIST_QUERY = "SELECT f.target FROM chat_%s f WHERE f.playerID = %u";

class FriendsQuery : public DBThreadQuery
{
public:
	FriendsQuery(const playerid_t playerID, bool ignore)
		: DBThreadQuery(DBThread::Default, "FriendsQuery", DBThreadQuery::_CALLBACK, _LIST_QUERY, (ignore) ? "ignore" : "friends", playerID)
		, m_list()
		, m_playerID(playerID), m_ignore(ignore)
		{}

private:
	HANDLES m_list;

public:
	virtual bool Process(const size_t numRows) override
	{
		m_list.clear();

		if ( numRows <= 0 )
			return true;

		while ( Query()->FetchRow() )
		{
			ChatHandle handle = chatGetHandle(Query()->Row()[0]);
			m_list.insert(handle);
		}

		return true;
	}

	virtual void Success() override;
	virtual void Failure() override;

public:
	playerid_t	m_playerID;
	bool		m_ignore;
};

static const char* _CHECK_QUERY = "SELECT p.callsign FROM wwii_player p WHERE callsign = '%s'";

class AddSocialCheck : public DBThreadQuery
{
public:
	AddSocialCheck(const playerid_t playerID, const ChatHandle handle, const bool ignore)
		: DBThreadQuery(DBThread::Default, "AddSocialCheck", DBThreadQuery::_CALLBACK, _CHECK_QUERY, Chat::Name(handle).c_str())
		, m_playerID(playerID), m_handle(handle), m_ignore(ignore), m_success(false)
		{}

	virtual bool Process(const size_t numRows) override
	{
		m_success = (numRows > 0);
		return true;
	}

	virtual void Success() override;
	virtual void Failure() override;

private:
	playerid_t	m_playerID;
	ChatHandle	m_handle;
	bool		m_ignore;
	bool		m_success;
};

static const char* _ADD_QUERY = "INSERT INTO chat_%s (playerid, target) VALUES (%u, '%s')";

class FriendAdd : public DBThreadQuery
{
public:
	FriendAdd(const playerid_t playerID, const Chat::Name& name, const bool ignore)
		: DBThreadQuery(DBThread::Default, "FriendAdd", DBThreadQuery::_IMMEDIATE, _ADD_QUERY, (ignore) ? "ignore" : "friends", playerID, name.c_str())
		{}
};

static const char* _DEL_QUERY = "DELETE FROM chat_%s WHERE playerid = %u AND target = '%s'";

class FriendRemove : public DBThreadQuery
{
public:
	FriendRemove(const playerid_t playerID, const Chat::Name& name, const bool ignore)
		: DBThreadQuery(DBThread::Default, "FriendRemove", DBThreadQuery::_IMMEDIATE, _DEL_QUERY, (ignore) ? "ignore" : "friends", playerID, name.c_str())
		{}
};

//	------------------------------	Class Functions

//////////////////////////////////////////////////////////////////////
// What to do when the query is successful

void
FriendsQuery::Success()
{
	Player* player = PlayerInterface::Lookup(m_playerID);
	if ( player == nullptr )
		return;

	HANDLES& list = (m_ignore) ? player->ignores() : player->friends();
	list.clear();
	list = m_list;

	Chat::Name name;
	for ( auto it = list.begin(), end = list.end(); it != end; ++it )
	{
		name = *it;
	}
}

//////////////////////////////////////////////////////////////////////
// What to do when the friends query fails

void
FriendsQuery::Failure()
{
	Player* player = PlayerInterface::Lookup(m_playerID);
	if ( player != nullptr )
		player->message(LSTR_CAUTION, (m_ignore) ? LSTR_IGNORE_ERROR : LSTR_FRIENDS_ERROR);
}

//////////////////////////////////////////////////////////////////////
// Complete the process of adding someone to the actual social list

static void
_addSocialToList(Player& player, const ChatHandle handle, bool ignore)
{
	Chat::Name name(handle);

	(new FriendAdd(player.playerID(), name, ignore))->Queue();

	if (ignore)
		player.ignores().insert(handle);
	else
		player.friends().insert(handle);

	player.message(LSTR_NOTE, (ignore) ? LSTR_IGNORING : LSTR_FRIEND_ADDED, name.c_str());
}

//////////////////////////////////////////////////////////////////////
// What to do when a friend check comes back successful

void
AddSocialCheck::Success()
{
	Player* player = PlayerInterface::Lookup(m_playerID);
	if ( player == nullptr )
		return;

	if ( m_success == false )
	{
		player->message(LSTR_CAUTION, LSTR_INVALID_CALLSIGN, Chat::Name(m_handle).c_str());
		return;
	}

	_addSocialToList(*player, m_handle, m_ignore);
}

//////////////////////////////////////////////////////////////////////
// What to do when a friends check gives an error

void
AddSocialCheck::Failure()
{
	Player* player = PlayerInterface::Lookup(m_playerID);
	if ( player != nullptr )
		player->message(LSTR_CAUTION, (m_ignore) ? LSTR_IGNORE_ERROR : LSTR_FRIENDS_ERROR);
}

//////////////////////////////////////////////////////////////////////
// Ask for a friends list

void
chatFetchFriendsAndIgnoreList(Player* player)
{
	dbugLog(_FLD, "t:%d p:%u c:%s requesting friends&ignore list", player->teulIndex(), player->playerID(), player->callsign());
	(new FriendsQuery(player->playerID(), false))->Queue();
	(new FriendsQuery(player->playerID(), true))->Queue();
}

//////////////////////////////////////////////////////////////////////
// Tell a player who their friends are

void
chatIgnoreList(Player* player)
{
	std::string list = "";

	for ( auto it = player->ignores().begin(), end = player->ignores().end(); it != end; ++it )
	{
		if ( !list.empty() )
			list += ", ";
		list += Chat::Name(*it).c_str();
	}

	if ( list.empty() )
	{
		player->message(LSTR_NOTE, LSTR_NOT_IGNORING_ANYONE);
		return;
	}

	player->message(LSTR_NOTE, LSTR_PLAYERS_IGNORED);
	player->message(LSTR_NOTE, LSTR_GENERIC, list.c_str());
}

//////////////////////////////////////////////////////////////////////

void
chatFriendsList(Player& player)
{
	std::string onlineList = "";
	std::string offlineList = "";

	if ( player.friends().empty() )
	{
		player.message(LSTR_NOTE, LSTR_FRIENDS_LIST_EMPTY);
		return;
	}

	for ( auto it = player.friends().begin(), end = player.friends().end(); it != end; ++it )
	{
		Player* buddy = PlayerInterface::Lookup(*it);
		if ( buddy && buddy->teulIndex() >= 0 && buddy->side() == player.side() )
		{
			if ( !onlineList.empty() )
				onlineList += " ";
			onlineList += buddy->callsign();
		}
		else
		{
			if ( !offlineList.empty() )
				offlineList += " ";
			offlineList += Chat::Name(*it).c_str();
		}
	}

	if ( !offlineList.empty() )
		player.message(LSTR_NONE, LSTR_OFFLINE_LIST, offlineList.c_str());
	if ( !onlineList.empty() )
		player.message(LSTR_NONE, LSTR_ONLINE_LIST, onlineList.c_str());
}

//////////////////////////////////////////////////////////////////////
// Add someone to a social list

void
_addSocial(Player& player, const ChatHandle handle, bool ignore)
{
	HANDLES& list = (ignore) ? player.ignores() : player.friends();

	if ( !list.empty() && list.size() > 42 )
	{
		player.message(LSTR_CAUTION, (ignore) ? LSTR_IGNORE_LIST_FULL : LSTR_FRIENDS_LIST_FULL);
		return;
	}

	ChatHandle lowHandle = chatGetLowercaseHandle(handle);
	if ( ignore && lowHandle == chatGetLowercaseHandle(systemHandleID) )
	{
		player.message(LSTR_SYSTEM, LSTR_IGNORE_SYSTEM1);
		player.message(LSTR_WARNING, LSTR_IGNORE_SYSTEM2);
		return;
	}
	if ( !ignore && lowHandle == chatGetLowercaseHandle(systemHandleID) )
	{
		player.message(LSTR_CAUTION, LSTR_FRIEND_SYSTEM);
		return;
	}
	static const ChatHandle rdpHandle = chatGetLowercaseHandle("rdp");
	if ( lowHandle == rdpHandle )
	{
		player.message(LSTR_SYSTEM, LSTR_FRIEND_IGNORE_RDP, (ignore ? "ignore" : "friend"));
		return;
	}

	Player* buddy = PlayerInterface::Lookup(handle);
	if ( !buddy )
	{
		(new AddSocialCheck(player.playerID(), handle, ignore))->Queue();
		return;
	}

	_addSocialToList(player, handle, ignore);
}

//////////////////////////////////////////////////////////////////////
// Operate on a social list (ignore/friends)

void
chatModifySocialList(Player& player, ARGV& args, bool isIgnore)
{
	// One argument
	if ( args.size() > 10 )
	{
		player.message(LSTR_CAUTION, LSTR_SOCIAL_LIMIT_OF_10);
		return;
	}

	HANDLES& list = (isIgnore) ? player.ignores() : player.friends();
	HANDLES done;
	HANDLES::iterator it;
	for ( size_t i = 0; i < args.size(); ++i )
	{
		ChatHandle handle = chatGetLowercaseHandle(args[i]);
		if ( done.find(handle) != done.end() )
			continue;
		done.insert(handle);
		if ( chatIsValidCallsign(args[i]) == false || handle == 0 )
		{
			player.message(LSTR_CAUTION, LSTR_INVALID_CALLSIGN, Chat::Name(handle).c_str());
			continue;
		}
		if ( handle == player.matchHandle() )
		{
			player.message(LSTR_CAUTION, LSTR_CANT_VERB_SELF, (isIgnore) ? "/ignore" : "/friend");
			continue;
		}

		it = list.find(handle);
		if ( it == list.end() )
		{
			_addSocial(player, handle, isIgnore);
		}
		else
		{
			Chat::Name name(handle);
			(new FriendRemove(player.playerID(), name, isIgnore))->Queue();
			list.erase(it);
			player.message(LSTR_NOTE, (isIgnore) ? LSTR_UNIGNORING : LSTR_FRIEND_REMOVED, name.c_str());
		}
	}
}
