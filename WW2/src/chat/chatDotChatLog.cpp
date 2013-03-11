#include <syscfg.h>	// Path is project specific.

#include <vector>

#include "chat/chatDotCommand.h"
#include "chat/chatLStringIDs.h"
#include "chat/chatMakeTempString.h"
#include "chat/chatPlayer.h"
#include "include/mydbQuery.h"
#include "wwii/wwiiDBThreads.h"

extern void chatDispatchGMMessage(const SCHAR* source, const SCHAR* text, const ChatHandle& excludeID=0LL) ;

//////////////////////////////////////////////////////////////////////
// List the last N chat messages of a given player.
//////////////////////////////////////////////////////////////////////

//! @internal
class ChatHistory : public DBThreadQuery
{
public:	ChatHistory(const playerid_t& playerID_, const ChatHandle& requester_)
			: DBThreadQuery(DBThread::Audit, "ChatHistoryQuery", DBThreadQuery::_CALLBACK
							, "SELECT logged, message FROM `audit`.`chat_messages`"
								" WHERE player_id = %u"
								" ORDER BY logged DESC"
								" LIMIT 10"
								, playerID_
								)
			, m_requester(requester_)
			, m_results()
			{}

protected:
	ChatHandle					m_requester ;
	std::vector<std::string>	m_results ;

protected:
	virtual bool Process(const size_t numRows)
	{
		if ( numRows <= 0 )
			return true ;

		m_results.resize(numRows) ;
		auto resultIt = m_results.begin() ;
		while ( Query()->FetchRow() )
		{
			DBRow& row = Query()->Row() ;
			std::string& result = *(resultIt++) ;
			result = row[0] ;
			result += ": " ;
			result += row[1] ;
		}

		return true ;
	}

public:
	virtual void Success()
	{
		Player* const player = PlayerInterface::Lookup(m_requester) ;
		if ( player == NULL )
			return ;

		const size_t numRows = m_results.size() ;
		if ( numRows <= 0 )
		{
			player->message(LSTR_NOTE, LSTR_NO_MATCHES) ;
			return ;
		}

		for ( auto it = m_results.rbegin() ; it != m_results.rend() ; ++it )
		{
			player->message(LSTR_NONE, LSTR_GENERIC, it->c_str()) ;
		}
	}

	virtual void Failure()
	{
		Player* const player = PlayerInterface::Lookup(m_requester) ;
		if ( player != NULL )
			player->message(LSTR_CAUTION, LSTR_DB_QUERY_FAILED, "chatlog", "Database Error") ;
	}
} ;

void
chatSendChatLog(const playerid_t& subjectID, const ChatHandle& requester)
{
	(new ChatHistory(subjectID, requester))->Queue() ;
}

//! @internal
class DotChatLog : public DotCommand
{
public:
	DotChatLog() : DotCommand("chatlog", "List recent chat of a given player"
								, GAME_MANAGER, LSTR_NONE, LSTR_NONE
							) {}

	virtual bool help(Player& player) const OVERRIDE_FN
	{
		if ( !allow(player) ) return false ;
		player.message(LSTR_USAGE, LSTR_GENERIC, "/chatlog <player> <reason>") ;
		player.message(LSTR_DETAIL, LSTR_GENERIC, "Shows the last 10 lines of chat messages received from <player>.") ;
		return true ;
	}

	virtual void actionWith(Player& player, Chat::ClientMessage* /*message*/, char* arguments) const OVERRIDE_FN
	{
		ARGV args ;
		if ( vectorize(arguments, args, 2) < 2 )
		{
			help(player) ;
			return ;
		}

		const ChatHandle handle = chatGetHandle(args[0]) ;
		const Player* const subject = PlayerInterface::Lookup(handle) ;
		if ( subject == NULL )
		{
			player.messagef(LSTR_CAUTION, LSTR_GENERIC, "Player '%s' not online or does not exist. /chatlog only works for online players.", args[0]) ;
			help(player) ;
			return ;
		}

		if ( subject->matchHandle() != player.matchHandle() )
		{
			// No point logging self-reviews.
			chatDispatchGMMessage("INFO", chatMakeTempString("%s /chatlog'ing %s: %s", player.callsign(), subject->callsign(), args[1])) ;
			player.gmNote("GM", command(), arguments, NULL) ;
		}

		player.gmNote("GM", command(), arguments, NULL) ;
		if ( player.isPlaynet() == false && subject->isPlaynet() == true )
		{
			player.message(LSTR_CAUTION, LSTR_GENERIC, "Permission denied.") ;
			return ;
		}

		chatSendChatLog(subject->playerID(), player.matchHandle()) ;
	}
} dotChatLog ;

