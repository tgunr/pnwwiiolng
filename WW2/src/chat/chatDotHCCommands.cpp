#include <syscfg.h> // Path is project specific.
// Date: Thu Mar 04 12:00:00 2004
//
// Routines the provide and deal with dot commands on the
// chat host server
//
// Copyright (c) 2004 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "chat/chatBroadcast.h"
#include "chat/chatData.h"
#include "chat/chatdbClientMessages.h"
#include "chat/chatDB.h"
#include "chat/chatDecisions.h"
#include "chat/chatDotCommand.h"
#include "chat/chatDotCommandIndex.h"
#include "chat/chatHost.h"
#include "chat/chatHostMessages.h"
#include "chat/chatList.h"
#include "chat/chatPlayer.h"
#include "chat/chatUnitMotds.h"
#include "chat/chatWhoList.h"
#include "chat/chatWhos.h"
#include "include/mydbQuery.h"
#include "inst/instTypes.h"
#include "teul/teulEndpointDefs.h"
#include "teul/teulTime.h"
#include "wwii/wwiiConfig.h"
#include "wwii/wwiiDBThreads.h"
#include "wwii/wwiiHCUnits.h"

//////////////////////////////////////////////////////////////////////
// Private variables

#if defined(BETA_RELEASE)
// static const int REPORT_TIME_LIMIT_M = 0;
 static const UINT32 OVERRUN_LOCK_TIME_M = 0;
// static const UINT32 RESYNC_LOCK_TIME_M = 0;
#elif defined(PUBLIC_RELEASE)
// static const int REPORT_TIME_LIMIT_M = 5;
 static const UINT32 OVERRUN_LOCK_TIME_M = 45;
// static const UINT32 RESYNC_LOCK_TIME_M = 5;
#else
// static const int REPORT_TIME_LIMIT_M = 5;
 static const UINT32 OVERRUN_LOCK_TIME_M = 5;
// static const UINT32 RESYNC_LOCK_TIME_M = 1;
#endif

// static const UCHAR PRIVATE_CHAN_ICON = 15;


//////////////////////////////////////////////////////////////////////
//// Private helper functions
//////////////////////////////////////////////////////////////////////

void
chatSetHCMotd(const char* callsign, const char* text, WWII_SIDE side)
{
	std::string prefix = "MOTD-";
	prefix += callsign;

	chatDispatchHCText(side, prefix.c_str(), text);

	prefix += ": ";

	chatSetUnitMotd(side, prefix + text);
}

static void
_setHCMotd(Player& player, const char* text)
{
	///TODO: Set a limit on how often this can be changed
	WWII_SIDE commandSide = player.activeCommandSide();

	if ( commandSide == WWII_NO_SIDE )
	{
		player.message(LSTR_CAUTION, LSTR_NOT_ACTIVE_HC);
		return;
	}

	chatSetHCMotd(player.callsign(), text, commandSide);
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//// Dot Command Definitions
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// Command to toggle afk (away from keyboard) status
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// Sends a system message to only the allied players
//////////////////////////////////////////////////////////////////////

class DotAllied : public DotCommand
{
public:
	DotAllied() : DotCommand("allied", "Named message to all ALLIED players"
								, NO_MASK
								, LSTR_ALLIED_USAGE, LSTR_ALLIED_DETAIL
								) { }

	virtual void actionWith(Player& player, Chat::ClientMessage* /*message*/, char* arguments) const override
	{
		if ( player.side() == AXIS )
		{
			help(player);
			return;
		}

		player.gmNote("HC", command(), arguments, nullptr);

		Chat::Broadcast broadcast(Chat::_SIDE, Instance::_CAMPAIGN, ALLIED, LSTR_ALLIED, LSTR_LABELLED_GENERIC, player.callsign(), arguments);
		broadcast.route();
	}
} dotAllied;

//////////////////////////////////////////////////////////////////////
// Sends a system message to only the axis players
//////////////////////////////////////////////////////////////////////

class DotAxis : public DotCommand
{
public:
	DotAxis() : DotCommand("axis", "Named message to all AXIS players"
							, NO_MASK
							, LSTR_AXIS_USAGE, LSTR_AXIS_DETAIL
							) { }

	virtual void actionWith(Player& player, Chat::ClientMessage* /*message*/, char* arguments) const override
	{
		if ( player.side() == ALLIED )
		{
			help(player);
			return;
		}

		player.gmNote("HC", command(), arguments, nullptr);

		Chat::Broadcast broadcast(Chat::_SIDE, Instance::_CAMPAIGN, AXIS, LSTR_AXIS, LSTR_LABELLED_GENERIC, player.callsign(), arguments);
		broadcast.route();
	}
} dotAxis;

//////////////////////////////////////////////////////////////////////
// Hold at all costs - preliminary implementation
//////////////////////////////////////////////////////////////////////

class DotDelay : public DotCommand
{
public:
	DotDelay() : DotCommand("delay", "Requests no action in response to the specified OVERRUN order request"
								, NO_MASK, LSTR_DELAY_USAGE, LSTR_DELAY_DETAIL
								, false
								) { }
	virtual void actionWith(Player& player, Chat::ClientMessage* /*message*/, char* arguments) const override
	{
		if ( player.noSide() )
		{
			player.message(LSTR_SYSTEM, LSTR_SELECT_SIDE, nullptr, nullptr);
			return;
		}

		// Can't vote more that once every 5 seconds
		if ( DotCommand::lockedFeature(player, "vote", 5, teulTime()) )
			return;

		chatVetoDecision(&player, arguments);
	}
} dotDelay;

//////////////////////////////////////////////////////////////////////
// .fallback - temporary implementation
//////////////////////////////////////////////////////////////////////

class DotFallback : public DotCommand
{
public:
	DotFallback() : DotCommand("fallback", "Initiate a fallback"
								, NO_MASK, LSTR_FALLBACK_USAGE, LSTR_FALLBACK_DETAIL
								, false
								) { }

	virtual void action(Player& player, Chat::ClientMessage* message) const override
	{
		if ( player.noSide() )
		{
			player.message(LSTR_SYSTEM, LSTR_SELECT_SIDE, nullptr, nullptr);
			return;
		}

		if ( !player.inFacility() )
		{
			player.message(LSTR_SYSTEM, LSTR_SELECT_FACILITY, command(), nullptr);
			return;
		}

		if ( DotCommand::lockedFeature(player, "vote", 5, teulTime()) )
			return;

		chatStartFallback(&player, player.originCpID());
	}
	virtual void actionWith(Player& player, Chat::ClientMessage* message, char* arguments) const override
	{
		if ( player.noSide() )
		{
			player.message(LSTR_SYSTEM, LSTR_SELECT_SIDE, nullptr, nullptr);
			return;
		}

		if ( DotCommand::lockedFeature(player, "vote", 5, teulTime()) )
			return;

		chatVoteFallback(&player, arguments);
	}
} dotFallback;

//////////////////////////////////////////////////////////////////////
// List all the HC staff on a particular side

static void
_hclistRoster(Player& player, WWII_SIDE side)
{
	static std::string roster;
	UINT32 i = 0;

	if ( side == ALLIED )
		player.message(LSTR_NONE, LSTR_ALLIED_OFFICERS, nullptr, nullptr);
	else if ( side == AXIS )
		player.message(LSTR_NONE, LSTR_AXIS_OFFICERS, nullptr, nullptr);
	else
	{
		player.message(LSTR_NONE, LSTR_SELECT_SIDE, nullptr, nullptr);
		return;
	}

	if ( commandList.empty() )
	{
		player.message(LSTR_NONE, LSTR_NO_MATCHES, nullptr, nullptr);
		return;
	}

	roster = "";

	PLAYERS::iterator it;
	for ( it = commandList.begin(); it != commandList.end(); ++it )
	{
		Player* cur = static_cast<Player*>(*it);

		if ( cur->side() != side || cur->hcOff() || !cur->isActiveOfficer() || cur->isHiddenFrom(player) )
			continue;

		cur->addName(roster, ", ");
		i++;

		// Try not to overflow the message length buffer
		if ( roster.length() + 12 >= MAX_MESSAGE_LENGTH )
		{
			player.message(LSTR_NONE, LSTR_GENERIC, roster.c_str(), nullptr);
			roster = "";
		}
	}

	if ( i == 0 )
		player.message(LSTR_NONE, LSTR_NO_MATCHES, nullptr, nullptr);
	else if ( roster.length() )
		player.message(LSTR_NONE, LSTR_GENERIC, roster.c_str(), nullptr);
}

//////////////////////////////////////////////////////////////////////
// .hc command
//////////////////////////////////////////////////////////////////////

class DotHC : public DotCommand
{
public:
	DotHC() : DotCommand("hc", "Roster or message the High Command channel(s)"
							, GAME_MANAGER|ACTIVE_OFFICER
							, LSTR_HC_USAGE, LSTR_HC_DETAIL
							)
	{
		addUsage(NO_MASK, LSTR_HC_USAGE, LSTR_HC_DETAIL);
	}

	// .hc
	virtual void action(Player& player, Chat::ClientMessage* message) const override;
	// .hc <message>
	virtual void actionWith(Player& player, Chat::ClientMessage* message, char* arguments) const override;
} dotHC;

static void
_hcRoster(Player& player, WWII_SIDE forSide)
{
	CHAT_WHO_LIST result(CWT_HC, 0, Teul::TCPStdPayload);
	result.clear();

	if ( commandList.empty() )
	{
		result.send(player.teulIndex());
		return;
	}

	Chat::Who who;
	PLAYERS::iterator it;
	for ( it = commandList.begin(); it != commandList.end(); ++it )
	{
		Player* cur = static_cast<Player*>(*it);

		if ( cur->side() != forSide || cur->hcOff() || !cur->isActiveOfficer() || cur->isHiddenFrom(player) )
			continue;
		if ( !player.activeCommandSide() && !player.isGM() && cur->noWho() )
			continue;

		chatWhoPopulate(who, &player, cur, false);
		result.add(who);
	}

	result.send(player.teulIndex());
}


// Player has typed ".hc"
void
DotHC::action(Player& player, Chat::ClientMessage* /*message*/) const
{
	if ( player.isManager() )
	{
		_hclistRoster(player, ALLIED);
		_hclistRoster(player, AXIS);
		return;
	}

	if ( !isValidSide(player.side()) )
	{
		player.message(LSTR_NONE, LSTR_SELECT_SIDE, nullptr, nullptr);
		return;
	}

	_hcRoster(player, player.side());
}

static void
_hcMessage(WWII_SIDE side, Player& player, const char* arguments)
{
	std::string callsign = player.callsign();
	if ( !player.isActiveCommander() && player.isActiveOfficer() )
		callsign.insert(0, "oic:");
	chatDispatchHCText(side, callsign.c_str(), arguments);
}

// Player has typed ".hc <something>"
void
DotHC::actionWith(Player& player, Chat::ClientMessage* /*message*/, char* arguments) const
{
	if ( player.noSide() && player.permissions(GAME_MANAGER) )
	{
		chatDispatchHCText(ALLIED, player.callsign(), arguments);
		chatDispatchHCText(AXIS, player.callsign(), arguments);
		return;
	}
	else if ( player.side() == ALLIED && player.permissions(ACTIVE_OFFICER|ALLIED_COMMAND_STAFF|IS_PLAYNET) )
	{
		_hcMessage(ALLIED, player, arguments);
		return;
	}
	else if ( player.side() == AXIS && player.permissions(ACTIVE_OFFICER|AXIS_COMMAND_STAFF|IS_PLAYNET) )
	{
		_hcMessage(AXIS, player, arguments);
		return;
	}

	help(player);
}


//////////////////////////////////////////////////////////////////////
// Turn HC messages on or off
//////////////////////////////////////////////////////////////////////

class DotHCOff : public DotCommand
{
public:
	DotHCOff() : DotCommand("hcoff", "Detune the HC channel"
								, NO_MASK, LSTR_HCOFF_USAGE, LSTR_HCOFF_DETAIL
								) { }

	virtual void action(Player& player, Chat::ClientMessage* /*message*/) const override
	{
		player.SetHcOff(true);
		player.message(LSTR_NOTE, LSTR_HC_OFF, nullptr, nullptr);
	}
} dotHCOff;

class DotHCOn : public DotCommand
{
public:
	DotHCOn() : DotCommand("hcon", "Retune the HC channel"
								, NO_MASK, LSTR_HCON_USAGE, LSTR_HCON_DETAIL
								) { }

	virtual void action(Player& player, Chat::ClientMessage* /*message*/) const override
	{
		player.SetHcOff(false);
		player.message(LSTR_NOTE, LSTR_HC_ON, nullptr, nullptr);
	}
} dotHCOn;

//////////////////////////////////////////////////////////////////////
// Simple text-list of who's on the HC channel

class DotHCList : public DotCommand
{
public:
	DotHCList() : DotCommand("hclist", "Short text-list of HC officers"
								, NO_MASK, LSTR_HCLIST_USAGE, LSTR_HCLIST_DETAIL
								) { }
	virtual void action(Player& player, Chat::ClientMessage* message) const override;
} dotHCList;

// Player has typed ".hclist"
void
DotHCList::action(Player& player, Chat::ClientMessage* /*message*/) const
{
	if ( player.noSide() && player.permissions(GAME_MANAGER) )
	{
		_hclistRoster(player, ALLIED);
		_hclistRoster(player, AXIS);
	}
	else if ( player.side() == ALLIED )
		_hclistRoster(player, ALLIED);
	else if ( player.side() == AXIS )
		_hclistRoster(player, AXIS);
	else
		help(player);
}

//////////////////////////////////////////////////////////////////////
// Hold at all costs - preliminary implementation
//////////////////////////////////////////////////////////////////////

class DotHold : public DotCommand
{
public:
	DotHold() : DotCommand("hold", "Initiate hold at all costs"
								, NO_MASK, LSTR_HOLD_USAGE, LSTR_HOLD_DETAIL
								, false
								) { }

	virtual void action(Player& player, Chat::ClientMessage* message) const override;
	virtual void actionWith(Player& player, Chat::ClientMessage* message, char* arguments) const override;
} dotHold;

void
DotHold::action(Player& player, Chat::ClientMessage* /*message*/) const
{
	if ( player.noSide() )
	{
		player.message(LSTR_SYSTEM, LSTR_SELECT_SIDE, nullptr, nullptr);
		return;
	}

	if ( !player.inFacility() )
	{
		player.message(LSTR_SYSTEM, LSTR_SELECT_FACILITY, command(), nullptr);
		return;
	}

	if ( DotCommand::lockedFeature(player, "vote", 5, teulTime()) )
		return;

	chatStartHold(&player, player.originCpID());
}

void
DotHold::actionWith(Player& player, Chat::ClientMessage* /*message*/, char* arguments) const
{
	if ( player.noSide() )
	{
		player.message(LSTR_SYSTEM, LSTR_SELECT_SIDE, nullptr, nullptr);
		return;
	}

	if ( DotCommand::lockedFeature(player, "vote", 5, teulTime()) )
		return;

	chatVoteHold(&player, arguments);
}

//////////////////////////////////////////////////////////////////////
// Brigade Home Operations
//////////////////////////////////////////////////////////////////////

class DotHome : public DotCommand
{
public:
	DotHome() : DotCommand("home", "Display brigade's home"
								, NO_MASK, LSTR_HOME_USAGE, LSTR_HOME_DETAIL
							) { }
	virtual void action(Player& player, Chat::ClientMessage* /*message*/) const override
	{
		HCUnit* brigade = HCUnit::Lookup(player.brigadeID());
		if ( !brigade )
		{
			player.message(LSTR_CAUTION, LSTR_NO_BRIGADE);
		}
		else
		{
			Property* home = chatFindProperty(brigade->homeCPID());
			if ( home != nullptr )
				player.message(LSTR_NOTE, LSTR_LABELLED_GENERIC, brigade->name(), home->Name());
			else
				player.message(LSTR_NOTE, LSTR_UNIT_HAS_NO_HOME, brigade->name());
		}
	}
} dotHome;

//////////////////////////////////////////////////////////////////////
// Set the HC motd
//////////////////////////////////////////////////////////////////////

class DotHCMotd : public DotCommand
{
public:
	DotHCMotd() : DotCommand("hcmotd", "Show/set the HC Message of the Day"
								, NO_MASK, LSTR_HCMOTD_USAGE, LSTR_HCMOTD_DETAIL
								) { }

	virtual void action(Player& player, Chat::ClientMessage* /*message*/) const override
	{
		WWII_SIDE commandSide = player.effectiveCommandSide();
		if ( commandSide == ALLIED )
			player.message(LSTR_ALLIED, LSTR_GENERIC, chatGetUnitMotd(commandSide));
		else if ( commandSide == AXIS )
			player.message(LSTR_AXIS, LSTR_GENERIC, chatGetUnitMotd(commandSide));
		else
			player.message(LSTR_CAUTION, LSTR_NOT_ACTIVE_HC);
	}

	virtual void actionWith(Player& player, Chat::ClientMessage* /*message*/, char* arguments) const override
	{
		_setHCMotd(player, arguments);
	}
} dotHCMotd;

//////////////////////////////////////////////////////////////////////
// Overrun command -- Player stipulation that a base is hopelessly
// camped or overrun. When a single CP has enough of these, then we
// bump it up to the HC for approval for Fallback or Hold at All Costs
//////////////////////////////////////////////////////////////////////

class DotOverrun : public DotCommand
{
public:
	DotOverrun() : DotCommand("overrun", "Signals your town has been overrun by the enemy"
								, NO_MASK, LSTR_OVERRUN_USAGE, LSTR_OVERRUN_DETAIL
								) { };

	virtual void action(Player& player, Chat::ClientMessage* /*message*/) const override
	{
		if ( player.noSide() )
		{
			player.message(LSTR_SYSTEM, LSTR_SELECT_SIDE);
			return;
		}

		if ( !player.inFacility() )
		{
			player.message(LSTR_SYSTEM, LSTR_SELECT_FACILITY, command(), nullptr);
			return;
		}

		// You can only use it every so-often
		if ( _featureLockedM(player, OVERRUN_LOCK_TIME_M) )
			return;

		player.sendStratCommand(command(), "", false);
	}
} dotOverrun;

//////////////////////////////////////////////////////////////////////

class LastMoves : public DBThreadQuery
{
public:
	LastMoves(const ChatHandle byMatchID, WWII_COUNTRY forCountry)
		: DBThreadQuery(DBThread::Default, "LastMoves", DBThreadQuery::_CALLBACK
				,"SELECT	m.moved"
					 ", u.name"
					 ",	m.moved_by"
					 ",	cp.name"
				 " FROM	strat_hc_moves m"
						" LEFT JOIN strat_hc_units u USING(unit_id)"
							" LEFT JOIN strat_cp cp ON (m.to_cp_oid = cp.cp_oid)"
				" WHERE	country_id = %u"
				" ORDER BY m.move_id DESC"
				" LIMIT 8"
					, forCountry
		), m_matchHandle(byMatchID)
		, m_results()
		{}
	virtual ~LastMoves() {}

private:
	ChatHandle	m_matchHandle;
	std::vector<std::string> m_results;

public:
	virtual bool Process(const size_t numRows) override
	{
		if ( numRows <= 0 )
			return true;

		std::string result;
		while ( Query()->FetchRow() )
		{
			DBRow& row = Query()->Row();
			result = row[0];
			result += " ";
			result += row[1];
			result += ": ";
			result += row[2];
			result += " -> ";
			result += row[3];
			m_results.push_back(result);
		}

		return true;
	}

public:
	virtual void Success() override
	{
		Player* player = PlayerInterface::Lookup(m_matchHandle);
		if ( player == nullptr )
			return;
		if ( m_results.empty() == true )
		{
			player->message(LSTR_CAUTION, LSTR_NO_MOVES_FOUND);
			return;
		}

		const size_t numRows = m_results.size();
		for ( size_t i = 0; i < numRows; ++i )
		{
			player->message(LSTR_NONE, LSTR_GENERIC, m_results[i].c_str());
		}
	}
	virtual void Failure() override
	{
		Player* player = PlayerInterface::Lookup(m_matchHandle);
		if ( player == nullptr ) return;

		player->message(LSTR_CAUTION, LSTR_DB_QUERY_FAILED, "lastmoves", "Database error");
	}
};

class DotLastMoves : public DotCommand
{
public:
	DotLastMoves() : DotCommand("lastmoves", "List the last movements of brigades"
											, NO_MASK, LSTR_LASTMOVE_USAGE, LSTR_LASTMOVE_DETAIL
							) {}

	virtual void action(Player& player, Chat::ClientMessage* /*message*/) const override
	{
		if ( _featureLockedM(player, 2) )
			return;
		if ( !player.country() )
		{
			player.message(LSTR_CAUTION, LSTR_SELECT_SIDE);
			return;
		}
		(new LastMoves(player.matchHandle(), player.country()))->Queue();
	};
} dotLastMoves;
