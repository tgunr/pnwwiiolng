#include <syscfg.h> // Path is project specific.
// Date: Thu Oct  6 15:55:21 CDT 2005
//
// Squad Commands
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "versionNo.h"

#include "chat/chatPlayer.h"
#include "chat/chatDotCommand.h"
#include "chat/chatHandle.h"
#include "chat/chatHost.h"
#include "chat/chatList.h"
#include "chat/chatName.h"
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
#include "include/mydbQuery.h"
#include "wwii/wwiiDBThreads.h"

using Chat::ARGV;

static SquadList*
_findClientSquadList(const Player& player)
{
	SquadList* list = chatFindSquadList(player.squadID());

	if ( list != nullptr ) return list;

	SquadControlResult error(SQR_SQUAD_UNAVAILABLE, SquadControl::_INVALID, 0LL);
	error.send(player.teulIndex());

	return nullptr;
}

static inline ChatHandle
_validateSquadName(const char* name)
{
	ChatHandle handle = chatGetLowercaseHandle(name);
	if ( handle == 0LL )
		return handle;
	Chat::Name unpacked(handle);
	if ( unpacked != name )
		return 0LL;
	size_t len = strlen(name);
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
	virtual bool help(Player& player) const override
	{
		genericHelp(player, "/changesquadname <newname>", "Request a name change for your squad.");
		return true;
	}

	virtual void actionWith(Player& player, Chat::ClientMessage* message, char* arguments) const override
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
	virtual void action(Player& player, Chat::ClientMessage* message) const override
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

	virtual void actionWith(Player& player, Chat::ClientMessage* message, char* arguments) const override
	{
		if ( !player.squadID() )
		{
			player.message(LSTR_CAUTION, LSTR_NO_SQUAD);
			return;
		}

		SquadList* list = _findClientSquadList(player);
		if ( list == nullptr )
			return;

		if ( !list->squad().votingEnabled() )
		{
			player.message(LSTR_CAUTION, LSTR_VOTING_DISABLED);
			return;
		}

		if ( player.squadRank() == Squad::RECRUIT && chatFindSquadVote(player.squadID()) == nullptr )
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
		if ( list->find(handle) == nullptr )
		{
			player.message(LSTR_CAUTION, LSTR_PLAYER_NOT_IN_SQUAD, Chat::Name(handle).c_str());
			return;
		}

		vote.addVote(player.squadID(), player.matchHandle(), handle);

		player.message(LSTR_NOTE, LSTR_SQUAD_VOTE_REGISTERED);
	}

	virtual void action(Player& player, Chat::ClientMessage* message) const override
	{
		if ( !player.squadID() )
		{
			player.message(LSTR_CAUTION, LSTR_NO_SQUAD);
			return;
		}

		SquadList* list = _findClientSquadList(player);
		if ( list == nullptr ) return;

		if ( !list->squad().votingEnabled() )
		{
			player.message(LSTR_NOTE, LSTR_VOTING_DISABLED);
			return;
		}

		SquadVote* vote = chatFindSquadVote(player.squadID());

		if ( vote == nullptr || vote->m_started == false )
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

	virtual void actionWith(Player& player, Chat::ClientMessage* /*message*/, char* arguments) const override
	{
		if ( !player.squadID() || player.squadRank() != Squad::CO )
		{
			player.updatePermMask();
			help(player);
			return;
		}

		// Make sure the squad is in memory
		SquadList* list = _findClientSquadList(player);
		if ( list == nullptr ) return;

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
			if ( vote != nullptr )
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
	virtual void actionWith(Player& player, Chat::ClientMessage* message, char* arguments) const override;
	virtual void action(Player& player, Chat::ClientMessage* message) const override;
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
	SquadMemberEntry* entry = nullptr;

	whoList.clear();

	for ( UINT32 i = 0; i < args.size() && i < _MAX_NAMES; ++i )
	{
		ChatHandle handleID = chatGetLowercaseHandle(args[i]);
		Player* target = PlayerInterface::Lookup(handleID);

		if ( target != nullptr
			&& target->isVisibleTo(player)
			&& (!target->noWho() || player.isEffectiveGM())
			)
		{
			chatWhoPopulate(who, &player, target, false);
		}
		else
		{
			entry = (list) ? list->find(handleID) : nullptr;
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
	if ( player.squadID() == 0 )
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
		if ( !subject->inSquad(&player) )
			continue;
#endif // !BETA_RELEASE

		if ( subject->isVisibleTo(player) && (subject->noWho() == false || player.isEffectiveGM()) )
		{
			if ( reply.length() + 10 > MAX_MESSAGE_LENGTH )
			{
				player.message(LSTR_NONE, lstringID, reply.c_str(), nullptr);
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
		player.message(LSTR_NONE, LSTR_NO_MATCHES, nullptr, nullptr);
	else if ( reply.length() > 0 )
		player.message(LSTR_NONE, LSTR_TEXT_END, reply.c_str(), nullptr);
}

//////////////////////////////////////////////////////////////////////
// Class definition for handling the database query intended to change
// the name of a squad

class SquadRename : public DBThreadQuery
{
public:
	SquadRename(const ChatHandle requester, const Chat::Name& from, const Chat::Name& to)
		: DBThreadQuery(DBThread::Default, "SquadRename", DBThreadQuery::_CALLBACK
						, "UPDATE LOW_PRIORITY wwii_squads"
						" SET short_name = '%s'"
						" WHERE LOWER(short_name) = LOWER('%s')"
						, to.c_str()
						, from.c_str()
						)
		, m_requester(requester)
		, m_from(from)
		, m_to(to)
		, m_affectedRows(0)
	{
	}

	virtual ~SquadRename() {}

protected:
	ChatHandle	m_requester;
	Chat::Name	m_from;
	Chat::Name	m_to;
	size_t		m_affectedRows;

	virtual bool Process(const size_t numRows) override
	{
		m_affectedRows = Query()->AffectedRows();

		return true;
	}

	virtual void Success() override
	{
		if (m_affectedRows == 0)
			return Failure();

		if (m_affectedRows > 1)
			threadLog(_FLA, "Updated multiple squads during rename!");

		Player* player = PlayerInterface::Lookup(m_requester);
		if (player == nullptr)
			return;

		chatRenameSquad(m_from, m_to);

		///LOCALIZE
		PREP_CHAT_MSG(cmsg, "%s has renamed squad '%s' to '%s'.", player->callsign(), m_from.c_str(), m_to.c_str());
		chatDispatchGMMessage("INFO", cmsg);
	}

	virtual void Failure() override
	{
		Player* player = PlayerInterface::Lookup(m_requester);
		if (player == nullptr)
			return;

		player->message(LSTR_CAUTION, LSTR_DB_QUERY_FAILED, "rename squad", "Database error");
	}
};

//////////////////////////////////////////////////////////////////////
// Check the validity of a name

static bool _checkName(Chat::Name& name, bool isCurrent, Player& player)
{
	size_t len = strlen(name.c_str());
	if (len < 2 || len > MAX_HANDLE_LEN)
	{
		player.message(LSTR_CAUTION, LSTR_INVALID_SQUAD_NAME, name);
		return false;
	}

	ChatHandle id = chatGetHandle(name);
	Chat::Name idName(id);
	if (idName.Empty() || !idName.IsValid() || idName != name)
	{
		player.message(LSTR_CAUTION, LSTR_INVALID_SQUAD_NAME, name);
		return false;
	}

	if (chatCheckForSquadName(id) != isCurrent)
	{
		if (isCurrent)
			player.message(LSTR_CAUTION, LSTR_NO_SUCH_SQUAD, name);
		else
			player.message(LSTR_CAUTION, LSTR_SQUAD_ALREADY_EXISTS, name);
		return false;
	}

	// Just to make absolutely sure the string is sanitized, reverse engineer it
	name = id;

	return true;
}

//////////////////////////////////////////////////////////////////////
// Define the DotCommand class for handling the renamesquad action

class RenameSquad : public DotCommand
{
public:
	RenameSquad() : DotCommand("renamesquad", "Change the callsign (short name) of a player squad"
							   , NO_MASK, LSTR_RENAMESQUAD_USAGE, LSTR_RENAMESQUAD_DETAIL)
	{
	}

	virtual void actionWith(Player& player, Chat::ClientMessage * /*message*/, char* arguments) const override {
		ARGV argv;

		// Usage: renamesquad <from> <to>
		if (vectorize(arguments, argv, 3) != 2) {
			help(player);
			return;
		}

		Chat::Name from(argv[0]), to(argv[1]);

		if (!_checkName(from, true, player))
			return;

		if (strcmp(from, to) == 0) {
			player.message(LSTR_CAUTION, LSTR_SQUAD_OLD_AND_NEW_SAME);
			return;
		}

		bool isCaseChange = (strcasecmp(from, to) == 0);
		if (!isCaseChange && !_checkName(to, false, player))
			return;

		player.gmNote("GM", command(), arguments, nullptr);

		(new SquadRename(player.matchHandle(), from, to))->Queue();
	}

} dotRenameSquad;
