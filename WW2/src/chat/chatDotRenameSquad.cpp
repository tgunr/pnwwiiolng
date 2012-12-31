#include <syscfg.h> // Path is project specific.
//
// Date: Thu Aug  3 11:29:51 CDT 2006
//
// GM Command for renaming squads
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith
//

#include "dbug/dbug.h"							// For host "dbugLog" access

#include <string>

#include "chat/chatPlayer.h"
#include "chat/chatDotCommand.h"
#include "chat/chatHost.h"
#include "chat/chatMakeTempString.h"
#include "chat/chatSquadList.h"
#include "chat/chatSquadListTypes.h"
#include "include/mydbQuery.h"
#include "wwii/wwiiDBThreads.h"

//////////////////////////////////////////////////////////////////////
// Class definition for handling the database query intended to change
// the name of a squad

class SquadRename : public DBThreadQuery
{
public:
	SquadRename(const ChatHandle& requester, const char* from, const char* to)
		: DBThreadQuery(DBThread::Default, "SquadRename", DBThreadQuery::_CALLBACK
						,	"UPDATE LOW_PRIORITY wwii_squads"
			  				" SET short_name = '%s'"
							" WHERE LOWER(short_name) = LOWER('%s')"
								, to
								, from
						)
		, m_requester(requester)
		, m_from(from)
		, m_to(to)
		, m_affectedRows(0)
		{}

	virtual ~SquadRename() {}

protected:
	ChatHandle	m_requester ;
	std::string	m_from ;
	std::string	m_to ;
	size_t		m_affectedRows ;

	virtual bool Process(const size_t numRows)
	{
		m_affectedRows = Query()->AffectedRows() ;

		return true ;
	}

	virtual void Success()
	{
		if ( m_affectedRows == 0 )
			return Failure() ;

		if ( m_affectedRows > 1 )
			threadLog(_FLA, "Updated multiple squads during rename!") ;

		Player* player = PlayerInterface::Lookup(m_requester) ;
		if ( player == NULL )
			return ;

		chatRenameSquad(m_from.c_str(), m_to.c_str()) ;

		///LOCALIZE
		const char* message =
			chatMakeTempString("%s has renamed squad '%s' to '%s'.", player->callsign(), m_from.c_str(), m_to.c_str()) ;
		chatDispatchGMMessage("INFO", message) ;
	}

	virtual void Failure()
	{
		Player* player = PlayerInterface::Lookup(m_requester) ;
		if ( player == NULL )
			return ;

		player->message(LSTR_CAUTION, LSTR_DB_QUERY_FAILED, "rename squad", "Database error") ;
	}
} ;

//////////////////////////////////////////////////////////////////////
// Check the validity of a name

static bool
_checkName(char* name, bool isCurrent, Player& player)
{
	size_t len = strlen(name) ;
	if ( len < 2 || len > MAX_HANDLE_LEN )
	{
		player.message(LSTR_CAUTION, LSTR_INVALID_SQUAD_NAME, name) ;
		return false ;
	}

	ChatHandle id = chatGetHandle(name) ;
	if ( !chatIsValidCallsign(name) || id == 0 || strcmp(name, chatGetCallsign(id)) != 0 )
	{
		player.message(LSTR_CAUTION, LSTR_INVALID_SQUAD_NAME, name) ;
		return false ;
	}

	if ( chatCheckForSquadName(id) != isCurrent )
	{
		if ( isCurrent )
			player.message(LSTR_CAUTION, LSTR_NO_SUCH_SQUAD, name) ;
		else
			player.message(LSTR_CAUTION, LSTR_SQUAD_ALREADY_EXISTS, name) ;
		return false ;
	}

	// Just to make absolutely sure the string is sanitized, reverse engineer it
	chatGetCallsign(name, id) ;

	return true ;
}

//////////////////////////////////////////////////////////////////////
// Define the DotCommand class for handling the renamesquad action

class RenameSquad : public DotCommand
{
public:
	RenameSquad() : DotCommand("renamesquad", "Change the callsign (short name) of a player squad"
								, NO_MASK, LSTR_RENAMESQUAD_USAGE, LSTR_RENAMESQUAD_DETAIL) {}

	virtual void actionWith(Player& player, Chat::ClientMessage* /*message*/, char* arguments) const OVERRIDE_FN
	{
		ARGV argv ;

		// Usage: renamesquad <from> <to>
		if ( vectorize(arguments, argv, 3) != 2 )
		{
			help(player) ;
			return ;
		}

		char from[MAX_HANDLE_SIZE] ;
		char to[MAX_HANDLE_SIZE] ;
		strcpy(from, argv[0]) ;
		strcpy(to, argv[1]) ;

		if ( !_checkName(from, true, player) )
			return ;

		if ( strcmp(from, to) == 0 )
		{
			player.message(LSTR_CAUTION, LSTR_SQUAD_OLD_AND_NEW_SAME) ;
			return ;
		}

		bool isCaseChange = (strcasecmp(from, to) == 0) ;
		if ( !isCaseChange && !_checkName(to, false, player) )
			return ;

		player.gmNote("GM", command(), arguments, NULL) ;

		(new SquadRename(player.matchHandle(), from, to))->Queue() ;
	}

} dotRenameSquad ;
