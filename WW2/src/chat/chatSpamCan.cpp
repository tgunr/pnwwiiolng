#include <syscfg.h> // Path is project specific.
// Date: Sat Apr  1 23:55:25 CST 2006
//
// Implements the "Spam can" public service announcer.
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "chat/chatDotCommand.h"
#include "chat/chatLStringIDs.h"
#include "chat/chatPlayer.h"
#include "chat/chatSpamCan.h"
#include "chat/chatTimeInterval.h"
#include "teul/teulTime.h"

using Chat::ARGV;

// Global "spam can", accessible by chatHost.cpp
SPAM_CAN	theSpamCan;
UINT32		spamLoops;
TeulTimeMS	nextSpamTime;
UINT32		nextSpamLine;
UINT32		spamInterval;

//////////////////////////////////////////////////////////////////////
// Play with the spam can
//////////////////////////////////////////////////////////////////////

class DotSpam : public DotCommand
{
public:
	DotSpam() : DotCommand("spam", "Add to or clear the spam can"
								, IS_PLAYNET, LSTR_SPAM_USAGE, LSTR_SPAM_DETAIL
								) { }
	virtual void actionWith(Player& player, Chat::ClientMessage* message, char* arguments) const override
	{
		if ( *arguments != '+' )
		{
			ARGV argv;
			UINT32 args = vectorize(arguments, argv, 4);
			if ( strcasecmp("clear", argv[0]) == 0 )
			{
				player.message(LSTR_NOTE, LSTR_SPAM_CLEAR);
				theSpamCan.clear();
				nextSpamLine = 0;
				spamLoops = 0;
			}
			else if ( strcasecmp("start", argv[0]) == 0 )
			{
				int numLoops = 0;
				spamInterval = 0;
				if ( args >= 2 )
					numLoops = atoi(argv[1]);
				if ( args >= 3 && !chatGetTimeInterval(player, argv[2], spamInterval) )
					return;
				if ( args < 3 || numLoops <= 0 || spamInterval <= 0 )
				{
					player.message(LSTR_USAGE, LSTR_SPAM_START_USAGE);
					player.message(LSTR_DETAIL, LSTR_SPAM_START_DETAIL);
					return;
				}
				if ( spamInterval < 30 )
				{
					player.message(LSTR_CAUTION, LSTR_SPAM_INTERVAL_MIN);
					return;
				}
				spamLoops = (UINT32)numLoops;
				nextSpamTime = teulClock64;
				nextSpamLine = 0;
				const char* interval = chatTimeInterval(spamInterval);
				char times[32];
				strprintf(times, "%u", spamLoops);
				player.message(LSTR_NOTE, LSTR_SPAM_OPENING, times, interval);
			}
			else if ( strcasecmp("stop", argv[0]) == 0 )
			{
				spamLoops = 0;
				nextSpamLine = 0;
				theSpamCan.clear();
				player.message(LSTR_NOTE, LSTR_SPAM_CLOSEING);
			}
			else
			{
				help(player);
				return;
			}
		}
		else
		{
			arguments++;
			while ( *arguments != 0 && isspace(*arguments) )
				++arguments;
			theSpamCan.push_back(arguments);
			player.message(LSTR_CAUTION, LSTR_SPAM_ADDED, arguments);
		}
	}

	virtual void action(Player& player, Chat::ClientMessage* message) const override
	{
		if ( theSpamCan.size() == 0 )
		{
			player.message(LSTR_NOTE, LSTR_SPAM_EMPTY);
			return;
		}

		char numLines[32];
		char loops[64];
		strprintf(numLines, "%" PRSizeT "", theSpamCan.size());
		strprintf(loops, "%u (%s)", spamLoops, chatTimeInterval(spamInterval));
		player.message(LSTR_NOTE, LSTR_SPAM_STATUS, numLines, loops);

		for ( UINT32 i = 0; i < theSpamCan.size(); ++i )
		{
			player.message(LSTR_NOTE, LSTR_GENERIC, theSpamCan[i].c_str());
		}
	}
} dotSpam;
