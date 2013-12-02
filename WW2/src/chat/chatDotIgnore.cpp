#include <syscfg.h> // Path is project specific.
//
// Date: Sun Nov  5 02:34:01 CST 2006
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
#include "chat/chatFriendList.h"

//////////////////////////////////////////////////////////////////////
// Ignore commands
//////////////////////////////////////////////////////////////////////

class DotIgnore : public DotCommand
{
public:
	DotIgnore() : DotCommand("ignore", "Manage/view ignore list"
								, NO_MASK, LSTR_IGNORE_USAGE, LSTR_IGNORE_DETAIL
								) {}

	virtual void action(Player& player, Chat::ClientMessage* /*message*/) const OVERRIDE_FN
	{
		static std::string textlist = "";

		const HANDLES& ignores = player.ignores();
		if ( ignores.empty() )
		{
			player.message(LSTR_NOTE, LSTR_NOT_IGNORING_ANYONE);
			return;
		}

		textlist.clear();

		player.message(LSTR_NOTE, LSTR_PLAYERS_IGNORED);
		HANDLES::const_iterator it;
		for ( it = ignores.begin(); it != ignores.end(); ++it )
		{
			if ( !textlist.empty() )
				textlist += ", ";
			textlist += chatGetCallsign(*it);
		}
		player.message(LSTR_NONE, LSTR_GENERIC, textlist.c_str());
	}

	virtual void actionWith(Player& player, Chat::ClientMessage* /*message*/, char* arguments) const OVERRIDE_FN
	{
		static ARGV args;

		vectorize(arguments, args, 11);

		// Request for assistance
		if ( args.size() < 1 || strcasecmp(args[0], "help") == 0 )
		{
			help(player);
			return;
		}

		chatModifySocialList(player, args, true);
	}
} dotIgnore;
