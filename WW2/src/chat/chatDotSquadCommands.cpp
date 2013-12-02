#include <syscfg.h> // Path is project specific.
//
// Date: Thu Oct  6 15:55:21 CDT 2005
//
// Squad Commands
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith
//

#include "versionNo.h"

#include "chat/chatPlayer.h"
#include "chat/chatDotCommand.h"
#include "chat/chatHandle.h"
#include "chat/chatList.h"
#include "chat/chatMakeTempString.h"
#include "chat/chatSquadControl.h"
#include "chat/chatSquadControlResults.h"
#include "chat/chatSquadList.h"
#include "chat/chatSquadListTypes.h"
#include "chat/chatSquadVote.h"
#include "wwii/wwiiThreads.h"
#include "wwii/wwiiThreadText.h"
#include "chat/chatTimeInterval.h"
#include "chat/chatWhoList.h"
#include "chat/chatWhos.h"
#include "teul/teulEndpointDefs.h"

static SquadList*
_findClientSquadList(const Player& player)
{
	SquadList* list = chatFindSquadList(player.squadID());

	if ( list != NULL ) return list;

	SquadControlResult error(SQR_SQUAD_UNAVAILABLE, SquadControl::_INVALID, 0LL);
	error.send(player.teulIndex());

	return NULL;
}

static inline ChatHandle
	_validateSquadName(const char* name)
{
	ChatHandle handle = chatGetLowercaseHandle(name);
	if ( handle == 0LL )
		return handle;
	char unpacked[MAX_HANDLE_SIZE];
	chatGetCallsign(unpacked, handle);
	if ( strcasecmp(name, unpacked) != 0 )
		return 0LL;
	int len = strlen(unpacked);
	if ( len < 2 || len > Squad::SHORT_NAME_LEN )
		return 0LL;

	return handle;
}

//////////////////////////////////////////////////////////////////////
// Request a change of the squad's name

class DotChangeSquadName : public DotCommand
{
public:
	DotChangeSquadName() : DotCommand("changesquadname", "Request a change of the squad's short name"
										, SQUAD_CO, LSTR_GENERIC, LSTR_GENERIC
										) {}
	virtual bool help(Player& player) const OVERRIDE_FN
	{
		genericHelp(player, "/changesquadname <newname>", "Request a name change for your squad.");
		return true;
	}

	virtual void actionWith(Player& player, Chat::ClientMessage* message, char* arguments) const OVERRIDE_FN
	{
		if ( !player.squadID() || player.squadRank() != Squad::CO )
		{
			help(player);
			return;
		}

		SquadList* list = _findClientSquadList(player);
		if ( !list )
			return;

		ChatHandle squadHandle = _validateSquadName(arguments);
		if ( squadHandle == 0LL )
		{
			player.message(LSTR_CAUTION, LSTR_INVALID_CALLSIGN, arguments);
			return;
		}

		///TODO: Check if squad already exists in memory
		///TODO: Pass to thread routines to check
		///TODO: Add
	}
	virtual void action(Player& player, Chat::ClientMessage* message) const OVERRIDE_FN
	{
		// Confirmation
		help(player);
	}
} dotChangeSquadName;

//////////////////////////////////////////////////////////////////////
// Vote for a CO
//////////////////////////////////////////////////////////////////////

static bool
_checkVoteStart(const SquadVote& vote, Player& player)
{
	if ( vote.nextStart() > teulTime() )
	{
		player.message(LSTR_CAUTION, LSTR_SQUAD_VOTE_CLOSED, chatTimeInterval(vote.nextStart() - teulTime()));
		return false;
	}

	return true;
}

class DotVoteCO : public DotCommand
{
public:
	DotVoteCO() : DotCommand("voteco", "Vote for Squad's CO"
							, NO_MASK, LSTR_VOTECO_USAGE, LSTR_VOTECO_DETAIL
							) {}

	virtual void actionWith(Player& player, Chat::ClientMessage* message, char* arguments) const OVERRIDE_FN
	{
		if ( !player.squadID() )
		{
			player.message(LSTR_CAUTION, LSTR_NO_SQUAD);
			return;
		}

		SquadList* list = _findClientSquadList(player);
		if ( list == NULL )
			return;

		if ( !list->squad().votingEnabled() )
		{
			player.message(LSTR_CAUTION, LSTR_VOTING_DISABLED);
			return;
		}

		if ( player.squadRank() == Squad::RECRUIT && chatFindSquadVote(player.squadID()) == NULL )
		{
			player.message(LSTR_CAUTION, LSTR_SQUAD_NOT_VOTING);
			return;
		}
		SquadVote& vote = chatGetSquadVote(player.squadID());
		if ( _checkVoteStart(vote, player) == false )
		{
			return;
		}

		if ( vote.hasVoted(player.matchHandle())  )
		{
			player.message(LSTR_CAUTION, LSTR_SQUAD_ALREADY_VOTED);
			return;
		}

		ChatHandle handle = chatGetLowercaseHandle(arguments);
		if ( list->find(handle) == NULL )
		{
			player.message(LSTR_CAUTION, LSTR_PLAYER_NOT_IN_SQUAD, chatGetCallsign(handle));
			return;
		}

		vote.addVote(player.squadID(), player.matchHandle(), handle);

		player.message(LSTR_NOTE, LSTR_SQUAD_VOTE_REGISTERED);
	}

	virtual void action(Player& player, Chat::ClientMessage* message) const OVERRIDE_FN
	{
		if ( !player.squadID() )
		{
			player.message(LSTR_CAUTION, LSTR_NO_SQUAD);
			return;
		}

		SquadList* list = _findClientSquadList(player);
		if ( list == NULL ) return;

		if ( !list->squad().votingEnabled() )
		{
			player.message(LSTR_NOTE, LSTR_VOTING_DISABLED);
			return;
		}

		SquadVote* vote = chatFindSquadVote(player.squadID());

		if ( vote == NULL || vote->m_started == false )
		{
			player.message(LSTR_CAUTION, LSTR_SQUAD_NOT_VOTING);
			return;
		}

		// This would be a good time to tell them that they are
		// between votes right now.
		if ( _checkVoteStart(*vote, player) == false )
			return;

		vote->announce(player.squadID(), &player);
	}
} dotVoteCO;

//////////////////////////////////////////////////////////////////////
// Enable or disable voting
//////////////////////////////////////////////////////////////////////

class DotVoting : public DotCommand
{
public:
	DotVoting() : DotCommand("voting", "Enable or disable voting for the squad"
							, SQUAD_CO, LSTR_VOTING_USAGE, LSTR_VOTING_DETAIL
							) {}

	virtual void actionWith(Player& player, Chat::ClientMessage* /*message*/, char* arguments) const OVERRIDE_FN
	{
		if ( !player.squadID() || player.squadRank() != Squad::CO )
		{
			player.updatePermMask();
			help(player);
			return;
		}

		// Make sure the squad is in memory
		SquadList* list = _findClientSquadList(player);
		if ( list == NULL ) return;

		if ( strcasecmp(arguments, "on") == 0 )			 // Turn voting on
		{
			// Check if it is already on
			if ( list->squad().votingEnabled() )
			{
				player.message(LSTR_CAUTION, LSTR_VOTING_ALREADY_ENABLED);
				return;
			}

			// Update the database, but make it low-priority
			DBThread::Do(DBThread::Default, "UPDATE LOW_PRIORITY wwii_squads SET voting = 1 WHERE squadid = %u", player.squadID());

			// Update the in-memory image, it's called "active" for now
			list->squad().setVotingEnabled(true);
			list->setUpdated();

			// Notify the squad
			list->sendMessage(LSTR_SQUAD, LSTR_CO_ENABLED_ELECTIONS, player.callsign(), list->squad().shortName());
		}
		else if ( strcasecmp(arguments, "off") == 0 )	// Turn voting off
		{
			// Check if it's already off
			if ( !list->squad().votingEnabled() )
			{
				player.message(LSTR_CAUTION, LSTR_VOTING_ALREADY_DISABLED);
				return;
			}

			// Low-pri update of the table
			DBThread::Do(DBThread::Default, "UPDATE LOW_PRIORITY wwii_squads SET voting = 0 WHERE squadid = %u", player.squadID());

			// Update in-memory
			list->squad().setVotingEnabled(false);
			list->setUpdated();

			list->sendMessage(LSTR_SQUAD, LSTR_CO_DISABLED_ELECTIONS, player.callsign(), list->squad().shortName());

			// Cancel any pending vote
			SquadVote* vote = chatFindSquadVote(player.squadID());
			if ( vote != NULL )
				vote->erase(player.squadID());
		}
		else
			help(player);
	}
} dotVoting;


//////////////////////////////////////////////////////////////////////
// The .who command
//////////////////////////////////////////////////////////////////////

class DotWho : public DotCommand
{
public:
	DotWho() : DotCommand("who", "Display list of squad mates online"
							, NO_MASK, LSTR_WHO_USAGE, LSTR_WHO_DETAIL
							) { }
	virtual void actionWith(Player& player, Chat::ClientMessage* message, char* arguments) const OVERRIDE_FN;
	virtual void action(Player& player, Chat::ClientMessage* message) const OVERRIDE_FN;
} dotWho;

// Provide info on the listed players
void
DotWho::actionWith(Player& player, Chat::ClientMessage* /*message*/, char* arguments) const
{
	static const UINT32 _MAX_NAMES = 8;

	if ( DotCommand::lockedFeature(player, "who", 5, teulTime()) )
		return;

	ARGV args;
	if ( vectorize(arguments, args, _MAX_NAMES + 1) < 1 )
	{
		help(player);
		return;
	}

	CHAT_WHO_LIST whoList(CWT_HC, 0, Teul::TCPStdPayload);
	Chat::Who who;
	SquadList* list = chatFindSquadList(player.squadID());
	SquadMemberEntry* entry = NULL;

	whoList.clear();

	for ( UINT32 i = 0; i < args.size() && i < _MAX_NAMES; ++i )
	{
		ChatHandle handleID = chatGetLowercaseHandle(args[i]);
		Player* target = PlayerInterface::Lookup(handleID);

		if ( target != NULL
			&& target->isVisibleTo(player)
			&& (!target->noWho() || player.isEffectiveGM())
			)
		{
			chatWhoPopulate(who, &player, target, false);
		}
		else
		{
			entry = (list) ? list->find(handleID) : NULL;
			if ( entry )
				who.setBasics(handleID, false, true, entry->squadRank(), list->squad().shortHandle());
			else
				who.setBasics(handleID, false, false, 0, 0);
		}
		whoList.add(who);
	}

	whoList.send(player.teulIndex());
}

void
DotWho::action(Player& player, Chat::ClientMessage* /*message*/) const
{
#if !defined(BETA_RELEASE) && !defined(TEST_RELEASE) && !defined(TRAINING_SERVER) && !defined(FRES)
	if ( player.squadID() == false )
	{
		player.message(LSTR_CAUTION, LSTR_NO_SQUAD, NULL, NULL);
		return;
	}
#endif // BETA_RELEASE

	std::string reply = "";
	PLAYER_INDEX::const_iterator it;
	UINT32 count = 0;

	static const UINT32 listLimit = 300;

	// Produce a list of upto N people who meet the criteria that are online.
	// Caveat: If there are N+ then truncate the list and add "..." instead
	// of the (N+1)th person.
	LSTRING_ID lstringID = LSTR_TEXT_START;

	for ( it = playerIndex.begin(); count < listLimit && it != playerIndex.end(); ++it )
	{
		const Player* subject = it->second;
#if !defined(BETA_RELEASE) && !defined(TEST_RELEASE) && !defined(TRAINING_SERVER) && !defined(FRES)
		if ( subject->inSquad(player) == false )
			continue;
#endif // !BETA_RELEASE

		if ( subject->isVisibleTo(player) && (subject->noWho() == false || player.isEffectiveGM()) )
		{
			if ( reply.length() + 10 > MAX_MESSAGE_LENGTH )
			{
				player.message(LSTR_NONE, lstringID, reply.c_str(), NULL);
				lstringID = LSTR_TEXT;
				reply.clear();
			}

			// Reached our limit, truncate the list
			if ( count >= listLimit )
			{
				reply += "...";
				break;
			}

			subject->addName(reply);

			count++;
		}
	}
	if ( count == 0 )
		player.message(LSTR_NONE, LSTR_NO_MATCHES, NULL, NULL);
	else if ( reply.length() > 0 )
		player.message(LSTR_NONE, LSTR_TEXT_END, reply.c_str(), NULL);
}

