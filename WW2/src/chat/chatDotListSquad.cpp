#include <syscfg.h> // Path is project specific.
//
// Date: Fri Aug 11 16:28:12 CDT 2006
//
// Dot Command that lists all known squad names or
// the online members of an active squad
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith
//


#include "chat/chatDotCommand.h"
#include "chat/chatSquadList.h"
#include "chat/chatSquadListTypes.h"
#include "chat/chatPlayer.h"
#include "chat/chatMakeTempString.h"
#include "chat/chatTimeString.h"

class ListSquad : public DotCommand
{
public:
	ListSquad() : DotCommand("listsquad", "List the members of an [active] squad"
								, NO_MASK, LSTR_LISTSQUAD_USAGE, LSTR_LISTSQUAD_DETAIL) {}

	virtual void actionWith(Player& player, Chat::ClientMessage* /*message*/, char* arguments) const OVERRIDE_FN
	{
		bool listAll = false ;

		ARGV argv ;
		int argc = vectorize(arguments, argv, 2) ;

		if ( argc > 0 && strcasecmp(argv[0], "-all") == 0 )
		{
			listAll = true ;
			argv.erase(argv.begin()) ;
			argc-- ;
		}

		if ( argc != 1 )
		{
			help(player) ;
			return ;
		}

		SquadList* list = chatFindSquadListByName(argv[0]) ;
		if ( list == NULL )
		{
			if ( chatCheckForSquadName(argv[0]) )
			{
				player.message(LSTR_CAUTION, LSTR_NO_SUCH_SQUAD, argv[0]) ;
				return ;
			}

			player.message(LSTR_NOTE, LSTR_SQUAD_NOT_ONLINE, argv[0]) ;
			return ;
		}

		// No more than once per minute
		if ( _featureLockedM(player, 1) )
			return ;

		const Squad& squad = list->squad() ;
		std::string response = "" ;
		response += chatMakeTempString("Sq#%u %s: %s. Online: %u/%u.\n"
									, squad.squadID()
									, squad.callsign().c_str()
									, squad.name().c_str()
									, list->online()
									, list->members().size()) ;
		response += chatMakeTempString("Created: %s by %s. ", chatTimeString(squad.created()), chatGetCallsign(squad.creatorHandle())) ;
		response += chatMakeTempString("CO: %s. ", chatGetCallsign(squad.coHandle())) ;
		response += chatMakeTempString("R: %s. ", squad.recruiting() ? "Yes" : "No") ;
		response += chatMakeTempString("V: %s. ", squad.recruiting() ? "Yes" : "No") ;
		response += chatMakeTempString("URL: %s", squad.homepage().c_str()) ;
		player.message(LSTR_NOTE, LSTR_GENERIC, response.c_str()) ;

		if ( !listAll && list->online() == 0 )
			return ;

		response = "" ;
		SQUAD_MEMBERS::iterator it ;
		LSTRING_ID lstrID = LSTR_TEXT_START ;
		std::string entry = "" ;

		for ( it = list->begin() ; it != list->end() ; ++it )
		{
			SquadMemberEntry& member = it->second ;

			if ( !listAll && !member.player() )
				continue ;


			if ( listAll )
			{
				if ( !response.empty() )
					response += " " ;
				entry = chatGetCallsign(it->first) ;
			}
			else
			{
				if ( !response.empty() )
					response += ", " ;
				const Player* subject = member.player() ;
				entry = "" ;
				subject->addName(entry) ;
			}

			if ( response.length() + entry.length() + 2 >= MAX_MESSAGE_LENGTH )
			{
				player.message(LSTR_NONE, lstrID, response.c_str()) ;
				lstrID = LSTR_TEXT ;
				response = "" ;
			}

			response += entry ;
		}

		if ( response.length() )
		{
			lstrID = (lstrID == LSTR_TEXT_START) ? LSTR_GENERIC : LSTR_TEXT_END ;
			player.message(LSTR_NONE, lstrID, response.c_str()) ;
		}
	}
} dotListSquad ;

