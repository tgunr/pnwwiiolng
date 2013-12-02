#include <syscfg.h> // Path is project specific.
//
// Date: Sat Sep 23 19:52:40 CDT 2006
//
// << DESCRIPTION >>
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith
//


#include "chat/chatDotCommand.h"
#include "chat/chatPlayer.h"
#include "chat/chatList.h"
#include "wwii/wwiiSquads.h"
#include "chat/chatSquadList.h"

class ListCOs : public DotCommand
{
public:
	ListCOs() : DotCommand("listcos", "List all SquadCOs currently online"
										, NO_MASK, LSTR_LISTCOS_USAGE, LSTR_LISTCOS_DETAIL) {}

	virtual void action(Player& player, Chat::ClientMessage* /*message*/) const OVERRIDE_FN
	{
		PLAYER_INDEX::iterator it;
		std::string result = "";
		for ( it = playerIndex.begin(); it != playerIndex.end(); ++it )
		{
			Player* subject = it->second;

			if ( !subject->squadID() || subject->squadRank() != Squad::CO )
				continue;

			if ( !subject->isVisibleTo(player) )
				continue;

			const char* squadName = chatGetSquadCallsign(subject->squadID());
			if ( !squadName )
				continue;

			if ( result.length() + 20 >= MAX_MESSAGE_LENGTH )
			{
				player.message(LSTR_NOTE, LSTR_GENERIC, result.c_str());
				result = "";
			}

			subject->addName(result, ", ");
			result += ":";
			result += squadName;
		}

		if ( result.empty() )
			player.message(LSTR_CAUTION, LSTR_NO_MATCHES);
		else
			player.message(LSTR_NOTE, LSTR_GENERIC, result.c_str());
	}
} listCOs;

