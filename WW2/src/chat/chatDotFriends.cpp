#include <syscfg.h> // Path is project specific.
// Date: Sun Nov  5 02:34:12 CST 2006
//
// Friends list command
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "chat/chatDotCommand.h"
#include "chat/chatFriendList.h"
#include "chat/chatLStringIDs.h"

//////////////////////////////////////////////////////////////////////
// .friends command
//////////////////////////////////////////////////////////////////////

class DotFriends : public DotCommand
{
public:
	DotFriends() : DotCommand("friends", "View/manipulate 'friends' list"
								, NO_MASK, LSTR_FRIENDS_USAGE, LSTR_FRIENDS_DETAIL
								) { }

public:
	virtual void action(Player& player, Chat::ClientMessage* /*message*/) const { chatFriendsList(player) ; }

	virtual void actionWith(Player& player, Chat::ClientMessage* /*message*/, char* arguments) const OVERRIDE_FN
	{
		static ARGV args ;

		vectorize(arguments, args, 11) ;

		// Request for assistance
		if ( args.size() < 1 || strcasecmp(args[0], "help") == 0 )
		{
			help(player) ;
			return ;
		}

		chatModifySocialList(player, args, false) ;
	}
} dotFriends ;

