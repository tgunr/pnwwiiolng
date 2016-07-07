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

#include "versionNo.h"
#include "server/serverReleaseTypeEnum.h"
#include "server/serverInterface.h"

#include "auth/authPlayerDetails.h"		// For AuthPlayerDetails structure
#include "chat/chatBanPlayer.h"
#include "chat/chatBroadcastObject.h"
#include "chat/chatBuzzardSpawn.h"
#include "chat/chatConstants.h"
#include "chat/chatDB.h"
#include "chat/chatData.h"
#include "chat/chatDotCommand.h"
#include "chat/chatDotCommandIndex.h"
#include "chat/chatGroupPermissions.h"
#include "chat/chatHost.h"
#include "chat/chatHostMessages.h"
#include "chat/chatLStringErr.h"
#include "chat/chatList.h"
#include "chat/chatMissionTypes.h"
#include "chat/chatMissions.h"
#include "chat/chatName.h"
#include "chat/chatPlayer.h"
#include "chat/chatPreferenceEnum.h"
#include "chat/chatSquadList.h"
#include "chat/chatSquadListTypes.h"
#include "chat/chatTimeInterval.h"
#include "chat/chatTimeString.h"
#include "chat/chatUnitMotds.h"
#include "include/mydbQuery.h"
#include "inst/instHost.h"
#include "server/serverPlayerStatusNode.h"
#include "server/serverServerInfo.h"
#include "wwii/wwiiConfig.h"
#include "wwii/wwiiDBThreads.h"
#include "wwii/wwiiHCUnits.h"
#include "wwii/wwiiIRC.h"
#include "wwii/wwiiRank.h"
#include "wwii/wwiiSquads.h"
#include "wwii/wwiiThreadText.h"
#include "wwii/wwiiThreads.h"

extern void serverBroadcastClusterShutdown();
extern void chatSendChatLog(const playerid_t& subjectID, const ChatHandle& requester);

//////////////////////////////////////////////////////////////////////
// Private variables

#if defined(BETA_RELEASE)
 static const UINT32 RESYNC_LOCK_TIME_M = 0;
#elif defined(PUBLIC_RELEASE)
 static const UINT32 RESYNC_LOCK_TIME_M = 5;
#else
 static const UINT32 RESYNC_LOCK_TIME_M = 1;
#endif

//////////////////////////////////////////////////////////////////////
//// Private helper functions
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////

static void
_smite(Player* target)
{
	if ( target == nullptr ) return;

	// Yes, use the broadcast index because we don't know where this guy is
	///FIXME: We should know where this player is, thank you teul
	chatSmitePlayer(target->playerID());

	// Now forcibly get rid of them
	PlayerStatusNode* psn = ServerInterface::PlayerStatusNodeByPlayerID(target->playerID());
	if ( psn != nullptr )
		target->Release(psn->teulIndex, Server::ReleaseType::Exit);
}

//////////////////////////////////////////////////////////////////////
// Process a ban instruction

static void
_executeBan(Player& player, const ARGV& argv, bool kick)
{
	ChatHandle subjectHandle = chatGetLowercaseHandle(argv[0]);
	if ( !chatIsValidCallsign(argv[0]) || subjectHandle == 0 )
	{
		player.message(LSTR_CAUTION, LSTR_INVALID_CALLSIGN, argv[0]);
		return;
	}

	UINT32 duration = 0;
	if ( !chatGetTimeInterval(player, argv[1], duration, 'd') )
		return;

	if ( duration < 300 )
	{
		player.message(LSTR_CAUTION, LSTR_MINIMUM_5MINUTE_BAN);
		return;
	}

	PREP_CHAT_MSG(msg, "%s %s", argv[1], argv[2]);
	player.gmNote("GM", (kick) ? "kickban" : "ban", argv[0], msg);

	Player* subject = PlayerInterface::Lookup(argv[0]);
	if ( kick )
	{
		if ( subject != nullptr )
		{
			player.message(LSTR_WARNING, LSTR_KICKBAN_PLAYER, subject->callsign());
			subject->message(LSTR_WARNING, LSTR_GENERIC, argv[2]);
			_smite(subject);
		}
	}
	else
	{
		if ( subject != nullptr )
			player.message(LSTR_WARNING, LSTR_BAN_ONLINE_PLAYER, subject->callsign());
	}

	chatBanPlayer(subjectHandle, duration, argv[2], player.playerID(), player.chatHandle());
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
// .all command sends all players a message with the sender identified
//////////////////////////////////////////////////////////////////////

//! @internal
class DotAll : public DotCommand
{
public:
	DotAll() : DotCommand("all", "Send named message to all players"
							, NO_MASK, LSTR_ALL_USAGE, LSTR_ALL_DETAIL
							) { }

	virtual void actionWith(Player& player, Chat::ClientMessage* message, char* arguments) const override
	{
	#if !defined(BETA_RELEASE) && !defined(TEST_RELEASE)
		player.gmNote("GM", command(), arguments, NULL);
	#endif

		chatSendAllMessage(player.chatHandle(), arguments, message->m_id);
	}
} dotAll;

//////////////////////////////////////////////////////////////////////
// Banish a player
//////////////////////////////////////////////////////////////////////

//! @internal
class DotBan : public DotCommand
{
public:
	DotBan() : DotCommand("ban", "Suspend a player's access"
							, NO_MASK, LSTR_BAN_USAGE, LSTR_BAN_DETAIL)
							{}

	virtual void actionWith(Player& player, Chat::ClientMessage* /*message*/, char* arguments) const override
	{
		static ARGV argv;
		int argc = vectorize(arguments, argv, 3);
		if ( argc != 3 )
		{
			help(player);
			return;
		}

		_executeBan(player, argv, false);
	}
} dotBan;

//////////////////////////////////////////////////////////////////////
// Displace a player by a few feet
//////////////////////////////////////////////////////////////////////

//! @internal
class DotBump : public DotCommand
{
public:
	DotBump() : DotCommand("bump", "Displace a player for GM reasons"
							, NO_MASK, LSTR_BUMP_USAGE, LSTR_BUMP_DETAIL)
							{}

	SINT32 delta(const FLOAT64& from, const FLOAT64& to, const FLOAT64& max) const
	{
		if ( fabs(from - to) < DBL_EPSILON )
			return (SINT32) (to);
		else if ( to > from + max )
			return (SINT32)(from + max);
		else if ( to < from - max )
			return (SINT32)(from - max);
		return (SINT32) to;
	}

	virtual void actionWith(Player& player, Chat::ClientMessage* /*message*/, char* arguments) const override
	{
		ARGV args;
		bool straightUp = false, allowDown = false;

		if ( strncmp(arguments, "-up", 3) == 0 )
		{
			straightUp = true;
			arguments += 3;
		}

		if ( strncmp(arguments, "-down", 5) == 0 )
		{
			allowDown = true;
			arguments += 5;
		}

		if ( vectorize(arguments, args, 2) != 2 || args[0][0] == '-' || args[1][0] == '-' )
		{
			// Don't know what you're talking about. Go away.
			help(player);
			return;
		}

		if ( player.isSpawned() == false )
			throw Chat::LStringErr(LSTR_ONLY_WHILE_SPAWNED);
		if ( player.isBuzzard() == false )
			throw Chat::LStringErr(LSTR_ONLY_WHILE_BUZZARD);

		Player* target = PlayerInterface::Lookup(args[0]);
		if ( target == nullptr )
			throw Chat::LStringErr(LSTR_USER_UNAVAILABLE, args[0]);

		if ( target->matchHandle() == player.matchHandle() )
			throw Chat::LStringErr(LSTR_CANT_VERB_SELF, command());

		if ( target->isSpawned() == false )
			throw Chat::LStringErr(LSTR_PLAYER_NOT_SPAWNED_NOW, target->callsign());
		if ( target->isBuzzard() )
			throw Chat::LStringErr(LSTR_CANT_BUMP_BUZZARD);

		if ( abs((SINT32)(target->lastX() - player.lastX())) > 75
			|| abs((SINT32)(target->lastY() - player.lastY())) > 75
			|| abs((SINT32)(target->lastZ() - player.lastZ())) > 200 )
			throw Chat::LStringErr(LSTR_BUMP_75M_LIMIT);

		Chat::PlayerMove outgoing;
		outgoing.m_playerID = target->playerID();
		if ( straightUp == false )
		{
			outgoing.m_x = delta(target->lastX(), player.lastX(), 4.0);
			outgoing.m_y = delta(target->lastY(), player.lastY(), 4.0);
			outgoing.m_z = delta(target->lastZ(), player.lastZ(), 6.0);
		}
		else
		{
			outgoing.m_x = (SINT32) target->lastX();
			outgoing.m_y = (SINT32) target->lastY();
			outgoing.m_z = (SINT32) (target->lastZ() + 5.0);
		}

		if ( allowDown == false && outgoing.m_z < target->lastZ() )
			outgoing.m_z = (SINT32) (target->lastZ() + 0.25);

		if ( Chat::SendPlayerMove(target->getCellHost(), outgoing) == false )
			throw Chat::LStringErr(LSTR_GENERIC, "/bump failed: internal error");

		player.gmNote("GM", command(), target->callsign(), args[1]);

		target->message(LSTR_WARNING, LSTR_GM_MOVED_YOU, args[1]);

		std::string note = args[1];
                DBThread::EscapeString(note);

		if ( note.length() > 400 )
			note.resize(400);
		DBThread::Do(DBThread::Default, "CALL wwii.p_add_player_note('%s', %u, 3, '%s')", target->callsign(), player.playerID(), note.c_str());

		PREP_CHAT_MSG(cmsg, "%s bumping %s: %s", player.callsign(), target->callsign(), args[1]);
		chatDispatchGMMessage("INFO", cmsg);
	}
} dotBump;

//////////////////////////////////////////////////////////////////////
// Spawn as a buzzard
//////////////////////////////////////////////////////////////////////

class DotBuzzardSpawn : public DotCommand
{
public:
	DotBuzzardSpawn() : DotCommand("buzzardspawn", "Spawns the issuer as a buzzard in the area of a named player"
								   , NO_MASK, LSTR_BUZZARDSPAWN_USAGE, LSTR_BUZZARDSPAWN_DETAIL, false)
	{
	}

	virtual void actionWith(Player& player, Chat::ClientMessage * /*message*/, char* arguments) const override
	{
		ARGV args;
		if (vectorize(arguments, args, 2) < 2)
		{
			help(player);
			return;
		}

		if (player.isSpawned())
		{
			player.message(LSTR_CAUTION, LSTR_ALREADY_SPAWNED);
			return;
		}

		ChatHandle targetHandle = chatGetLowercaseHandle(args[0]);
		if (!chatIsValidCallsign(args[0]) || args[0] == nullptr)
		{
			player.message(LSTR_CAUTION, LSTR_INVALID_CALLSIGN);
			return;
		}
		if (targetHandle == player.matchHandle())
		{
			player.message(LSTR_CAUTION, LSTR_CANT_VERB_SELF, command());
			return;
		}

		Player* target = PlayerInterface::Lookup(targetHandle);
		if (target == nullptr)
		{
			player.message(LSTR_CAUTION, LSTR_USER_UNAVAILABLE, args[0]);
			return;
		}

		if (static_cast<UINT64>(target->lastX()) == static_cast<UINT64>(INVALID_GRID_COORD))
		{
			player.message(LSTR_CAUTION, LSTR_PLAYER_NOT_SPAWNED_YET, target->callsign());
			return;
		}

		FLOAT64VECTOR location;
		static const ChatHandle snail = chatGetLowercaseHandle("snail");
		if (player.matchHandle() == snail && (target->playerID() == 3204 || target->playerID() == 833))
		{
			location.x = location.y = location.z = 0;
		} else
		{
			location.x = target->lastX();
			location.y = target->lastY();
			location.z = target->lastZ();
		}

		player.gmNote("GM", command(), args[0], args[1]);
		PREP_CHAT_MSG(cmsg, "%s /buzzardspawning %s: %s", player.callsign(), target->callsign(), args[1]);
		chatDispatchGMMessage("INFO", cmsg);

		Teul::Packet outgoing(Teul::PacketId(Chat::CallbackX, Chat::BUZZARD_SPAWN_Y));
		outgoing.construct<BuzzardSpawn>(player.playerID(), true, location);
		serverSendPacketToType(outgoing, MAP_HOST);
	}
} dotBuzzardSpawn;

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
		{
		}

protected:
	ChatHandle					m_requester;
	std::vector<std::string>	m_results;

protected:
	virtual bool Process(const size_t numRows) override
	{
		if (numRows <= 0)
			return true;

		m_results.resize(numRows);
		auto resultIt = m_results.begin();
		while (Query()->FetchRow())
		{
			DBRow& row = Query()->Row();
			std::string& result = *(resultIt++);
			result = row[0];
			result += ": ";
			result += row[1];
		}

		return true;
	}

public:
	virtual void Success() override
	{
		Player* const player = PlayerInterface::Lookup(m_requester);
		if (player == nullptr)
			return;

		const size_t numRows = m_results.size();
		if (numRows <= 0)
		{
			player->message(LSTR_NOTE, LSTR_NO_MATCHES);
			return;
		}

		for (auto it = m_results.rbegin(); it != m_results.rend(); ++it)
		{
			player->message(LSTR_NONE, LSTR_GENERIC, it->c_str());
		}
	}

	virtual void Failure() override
	{
		Player* const player = PlayerInterface::Lookup(m_requester);
		if (player != nullptr)
			player->message(LSTR_CAUTION, LSTR_DB_QUERY_FAILED, "chatlog", "Database Error");
	}
};

void
	chatSendChatLog(const playerid_t& subjectID, const ChatHandle& requester)
{
	(new ChatHistory(subjectID, requester))->Queue();
}

//! @internal
class DotChatLog : public DotCommand
{
public:
	DotChatLog() : DotCommand("chatlog", "List recent chat of a given player"
							  , GAME_MANAGER, LSTR_NONE, LSTR_NONE
							  )
	{
	}

	virtual bool help(Player& player) const override
	{
		if (!allow(player)) return false;
		player.message(LSTR_USAGE, LSTR_GENERIC, "/chatlog <player> <reason>");
		player.message(LSTR_DETAIL, LSTR_GENERIC, "Shows the last 10 lines of chat messages received from <player>.");
		return true;
	}

	virtual void actionWith(Player& player, Chat::ClientMessage * /*message*/, char* arguments) const override
	{
		ARGV args;
		if (vectorize(arguments, args, 2) < 2)
		{
			help(player);
			return;
		}

		const ChatHandle handle = chatGetHandle(args[0]);
		const Player* const subject = PlayerInterface::Lookup(handle);
		if (subject == nullptr)
		{
			player.messagef(LSTR_CAUTION, LSTR_GENERIC, "Player '%s' not online or does not exist. /chatlog only works for online players.", args[0]);
			help(player);
			return;
		}

		if (subject->matchHandle() != player.matchHandle())
		{
			// No point logging self-reviews.
			PREP_CHAT_MSG(cmsg, "%s /chatlog'ing %s: %s", player.callsign(), subject->callsign(), args[1]);
			chatDispatchGMMessage("INFO", cmsg);
			player.gmNote("GM", command(), arguments, nullptr);
		}

		player.gmNote("GM", command(), arguments, nullptr);
		if (player.isPlaynet() == false && subject->isPlaynet() == true)
		{
			player.message(LSTR_CAUTION, LSTR_GENERIC, "Permission denied.");
			return;
		}

		chatSendChatLog(subject->playerID(), player.matchHandle());
	}
} dotChatLog;

//////////////////////////////////////////////////////////////////////
// Transmit a message to people on .CRS
//////////////////////////////////////////////////////////////////////

//! @internal
class DotCRS : public DotCommand
{
public:
	DotCRS() : DotCommand("crs", "Roster or message the CRS channel"
							, IS_PLAYNET, LSTR_CRS_USAGE, LSTR_CRS_DETAIL
							, true
							) { }
	virtual void action(Player& player, Chat::ClientMessage* message) const override
	{
		std::string reply = "";

		PLAYERS::iterator it;
		for ( it = commandList.begin(); it != commandList.end(); ++it )
		{
			Player* cur = static_cast<Player*>(*it);

			if ( cur->isPlaynet() )
			{
				///TODO: Make this doable with addname
				if ( reply.length() )
					reply += " ";
				if ( cur->gmHide() )
					reply += "*";
				if ( cur->isAFK() )
					reply += "(";
				reply += cur->callsign();
				if ( cur->isAFK() )
					reply += ")";
			}
		}

		if ( reply.length() == 0 )
			player.message(LSTR_NONE, LSTR_NO_MATCHES, nullptr, nullptr);
		else
			player.message(LSTR_NONE, LSTR_GENERIC, reply.c_str(), nullptr);
	}

	virtual void actionWith(Player& player, Chat::ClientMessage* message, char* arguments) const override
	{
		if ( arguments[0] == '+' )
		{
			std::string motd = player.callsign();
			motd += ": ";
			motd += (arguments+1);

			chatSetUnitMotd(CRS_MOTD_UNIT, motd.c_str());
			chatDispatchCRSMessage("MOTD", motd.c_str());

			return;
		}

		chatDispatchCRSMessage(player.callsign(), arguments);
	}

} dotCRS;

//////////////////////////////////////////////////////////////////////
// Manage the state of the debug system on the chat host.
//////////////////////////////////////////////////////////////////////

//! @internal
class DotDebug : public DotCommand
{
public:
	DotDebug() : DotCommand("debug", "Manipulate the chat host's debug logging settings"
							, IS_PLAYNET, LSTR_GENERIC, LSTR_GENERIC
							, true
							)
	{
	}
	virtual bool help(Player& player) const override
	{
		if (!allow(player)) return false;
		player.message(LSTR_USAGE, LSTR_GENERIC, "'/debug <condition> on|off'");
		player.message(LSTR_DETAIL, LSTR_GENERIC, "Sets or clears a debug logging condition on the chat host.");
		player.message(LSTR_DETAIL, LSTR_GENERIC, "Conditions are: debug, radios, squads, personas and chat.");
		player.message(LSTR_NOTE, LSTR_GENERIC, "Condition 'debug' is a master setting for all other conditions.");
		return true;
	}

	virtual void actionWith(Player& player, Chat::ClientMessage * /*message*/, char* arguments) const override
	{
		ARGV argv;
		int argc = vectorize(arguments, argv, 3);
		if (argc != 2)
		{
			help(player);
			return;
		}

		static std::map<std::string, bool*> conditions;
		if (conditions.empty())
		{
			conditions["debug"] = &g_logDebug;
			conditions["radios"] = &g_debugRadios;
			conditions["squads"] = &g_debugSquads;
			conditions["personas"] = &g_debugPersonas;
			conditions["chat"] = &g_debugChat;
		}

		auto it = conditions.find(argv[0]);
		if (it == conditions.end())
		{
			player.message(LSTR_ERROR, LSTR_LABELLED_GENERIC, "Unrecognized condition", argv[0]);
			help(player);
			return;
		}

		const bool enable = (strcasecmp(argv[1], "on") == 0 || strcmp(argv[1], "1") == 0);
		const char* const report = (enable ? "enabled" : "disabled");

		if (*(it->second) == enable)
		{
			player.message(LSTR_CAUTION, LSTR_GENERIC, "It's already enabled.");
			return;
		}

		*(it->second) = enable;
		char announce[128];
		strprintf(announce, "%s has %s '%s' logging.", player.callsign(), report, (it->first).c_str());
		chatDispatchGMMessage(player.callsign(), announce);
	}

} dotDebug;

//////////////////////////////////////////////////////////////////////
// .find -- locate a player out in the game world
//////////////////////////////////////////////////////////////////////

//! @internal
class DotFind : public DotCommand
{
public:
	DotFind() : DotCommand("find", "Describe where a player is in the game"
								, NO_MASK, LSTR_FIND_USAGE, LSTR_FIND_DETAIL
								, true
								) { }

	virtual void actionWith(Player& player, Chat::ClientMessage* /*message*/, char* arguments) const override
	{
		ARGV args;
		if ( vectorize(arguments, args, 2) != 1 )
		{
			// Don't know what you're talking about. Go away.
			help(player);
			return;
		}

		// Locate the user
		Player* subject = PlayerInterface::Lookup(args[0]);

		if ( subject == nullptr )
		{
			player.message(LSTR_CAUTION, LSTR_USER_UNAVAILABLE, args[0], nullptr);
			return;
		}

		chatSendLocateUser(player, subject->playerID());
	}
} dotFind;

//////////////////////////////////////////////////////////////////////
// Send a GM message or list who is on the channel
//////////////////////////////////////////////////////////////////////

//! @internal
class DotGM : public DotCommand
{
public:
	DotGM() : DotCommand("gm", "Roster or message the GM channel"
							, GAME_MANAGER, LSTR_GM_USAGE, LSTR_GM_DETAIL
							) { }
	// List people on the channel
	virtual void action(Player& player, Chat::ClientMessage* message) const override
	{
		std::string roster = "";
		bool onDuty;
		UINT32 i  = 0;

		PLAYERS::iterator it;
		for ( it = commandList.begin(); it != commandList.end(); ++it )
		{
			Player* cur = static_cast<Player*>(*it);

			// Only show GMs/trainers
			if ( !cur->isExplicitGM() )
				continue;

			onDuty = cur->isTunedTo(ADMIN_CHANNEL);

			// Obscurity rules: you can't see a playnet/hidden GM unless
			// you are a GM yourself
			if ( !player.isGM() )
			{
				if ( !onDuty || cur->gmHide() || cur->isPlaynet() || cur->inGrab() )
					continue;
			}

			///TODO: Make this doable with addname
			if ( roster.length() )
				roster += " ";
			if ( cur->isAFK() )
				roster += "(";
			if ( cur->gmHide() )
				roster += "!";
			roster += cur->callsign();
			if ( !onDuty )
				roster += "*";
			if ( cur->isAFK() )
				roster += ")";

			// If they're in a grab with someone, show whom
			if ( cur->inGrab() )
			{
				const Player* grabbed = PlayerInterface::Lookup(cur->grabbing());
				if ( grabbed != nullptr )
				{
					roster += "<=>";
					roster += grabbed->callsign();
				}
			}

			// Try not to overflow the message length buffer
			if ( roster.length() >= MAX_MESSAGE_LENGTH - 12 )
			{
				player.message(LSTR_NONE, LSTR_GENERIC, roster.c_str(), nullptr);
				roster = "";
			}

			i++;
		}

		if ( i == 0 )
			player.message(LSTR_NONE, LSTR_NO_MATCHES, nullptr, nullptr);
		else if ( roster.length() )
			player.message(LSTR_NONE, LSTR_GENERIC, roster.c_str(), nullptr);
	}

	// Send message to the channel
	virtual void actionWith(Player& player, Chat::ClientMessage* message, char* arguments) const override
	{
		if ( !player.isEffectiveGM() )
		{
			help(player);
			return;
		}

		if ( arguments[0] == '+' )
		{
			std::string motd = player.callsign();
			motd += ": ";
			motd += (arguments+1);

			chatSetUnitMotd(GM_MOTD_UNIT, motd.c_str());
			chatDispatchGMMessage("MOTD", motd.c_str());

			return;
		}

		chatDispatchGMMessage(player.callsign(), arguments);
	}
} dotGM;

//////////////////////////////////////////////////////////////////////
// Toggle GM invisible mode
//////////////////////////////////////////////////////////////////////

//! @internal
class DotGMMode : public DotCommand
{
public:
	DotGMMode() : DotCommand("gmmode", "Toggle GM Mode"
								, NO_MASK, LSTR_GMMODE_USAGE, LSTR_GMMODE_DETAIL
								) { }

	virtual void action(Player& player, Chat::ClientMessage* /*message*/) const override
	{
		player.SetGMHide(!player.gmHide());
		if ( player.gmHide() )
			player.message(LSTR_NOTE, LSTR_GM_ON, nullptr, nullptr);
		else
			player.message(LSTR_NOTE, LSTR_GM_OFF, nullptr, nullptr);
	}

	virtual void actionWith(Player& player, Chat::ClientMessage* message, char* arguments) const override
	{
		static const DotCommand* dotMsg = DotCommand::index()->lookup("m", true);
		if ( nullptr == dotMsg )
		{
			player.message(LSTR_CAUTION, LSTR_INTERNAL_ERROR);
			return;
		}

		bool wasHiding = player.gmHide();
		if ( false == wasHiding )
			player.SetGMHide(true);

		dotMsg->actionWith(player, message, arguments);

		if ( false == wasHiding )
			player.SetGMHide(false);
	}
} dotGMMode;

//////////////////////////////////////////////////////////////////////
// Dot Grab
//////////////////////////////////////////////////////////////////////

//! @internal
class DotGrab : public DotCommand
{
public:
	DotGrab() : DotCommand("grab", "Direct comms with specified player"
							, NO_MASK, LSTR_GRAB_USAGE, LSTR_GRAB_DETAIL
							) { }
	virtual void actionWith(Player& player, Chat::ClientMessage* message, char* arguments) const override
	{
		ARGV args;

		if ( vectorize(arguments, args, 2) < 1 )
		{
			help(player);
			return;
		}

		// Make sure the subject user is online/spelled correctly, etc
		Player* target = PlayerInterface::Lookup(args[0]);
		if ( target == nullptr )
		{
			player.message(LSTR_CAUTION, LSTR_USER_UNAVAILABLE, args[0], nullptr);
			return;
		}

		// If we have been grabbed, then we don't continue
		if ( player.takenBy() )
		{
			player.message(LSTR_SYSTEM, LSTR_GRABBED_ALREADY, nullptr, nullptr);
			return;
		}

		if ( args.size() == 1 )
			player.gmNote("GM", command(), args[0], nullptr);
		else
			player.gmNote("GM", command(), args[0], args[1]);

		// However if we have grabbed someone else, we should cancel that grab
		if ( player.inGrab() )
		{
			player.message(LSTR_NOTE, LSTR_GRABBED_RELEASING, nullptr, nullptr);
			chatReleaseGrab(player);
		}

		char cmsg[MAX_MESSAGE_LENGTH+1];
		if ( args.size() > 1 && args[1][0] != 0 )
			snprintf(cmsg, MAX_MESSAGE_LENGTH, "%s grabbing %s: %s", player.callsign(), target->callsign(), args[1]);
		else
			snprintf(cmsg, MAX_MESSAGE_LENGTH, "%s grabbing %s", player.callsign(), target->callsign());
		chatDispatchGMMessage("INFO", cmsg);

		// I want to know if the sender is currently afk'd
		target->notifyOtherIfAFK(player);

		// Mark the user as grabbed by me and me as in communication with them
		player.grab(target);

		// Tell me that we were successful
		player.message(LSTR_NOTE, LSTR_HAVE_GRABBED, target->callsign(), nullptr);

		// Now let the subject know what is happening to them
		const char* grabber = player.callsign();

#ifdef PUBLIC_RELEASE
		if ( !target->isGM() && !target->isPlaynet() )
			grabber = "(GM)";
#endif

		if ( args.size() > 1 )
			target->message(LSTR_WARNING, LSTR_BEEN_GRABBED_MSG, grabber, args[1]);
		else
			target->message(LSTR_WARNING, LSTR_BEEN_GRABBED, grabber, nullptr);
	}
} dotGrab;

//////////////////////////////////////////////////////////////////////
// Eject a player
//////////////////////////////////////////////////////////////////////

//! @internal
class DotKick : public DotCommand
{
public:
	DotKick() : DotCommand("kick", "Eject a player from the game"
								, NO_MASK, LSTR_KICK_USAGE, LSTR_KICK_DETAIL
								) { }
	virtual void actionWith(Player& player, Chat::ClientMessage* message, char* arguments) const override
	{
		// Split the incoming string into two or less arguments
		ARGV args;
		const char* reason;
		if ( vectorize(arguments, args, 2) >= 2 )
			reason = args[1];
		else
			reason = nullptr;

		const char* reasonText;
		reasonText = (reason) ? reason : "Reason not given";

		// Log the activity
		player.gmNote("GM", command(), args[0], reason);

		// Determine if the specified user is online
		Player* target = PlayerInterface::Lookup(args[0]);
		if ( target == nullptr )
		{
			chatForceKickPlayer(chatGetHandle(args[0]), reasonText, player.playerID(), player.matchHandle());
			return;
		}

		PREP_CHAT_MSG(cmsg, "%s kicking %s: %s", player.callsign(), target->callsign(), reasonText);
		chatDispatchGMMessage("INFO", cmsg);

		// If the victim is afk, it'd be fun to know
		target->notifyOtherIfAFK(player);

		if ( reason == nullptr )
		{
			target->message(LSTR_WARNING, LSTR_BEEN_KICKED, nullptr, nullptr);
			player.message(LSTR_WARNING, LSTR_HAVE_KICKED, args[0], nullptr);
		}
		else
		{
			target->message(LSTR_WARNING, LSTR_BEEN_KICKED_MSG, reason, nullptr);
			player.message(LSTR_WARNING, LSTR_HAVE_KICKED_MSG, args[0], reason);
		}

		_smite(target);

		std::string sqlReason = (reason ? reason : "<no reason given>");
                DBThread::EscapeString(sqlReason);

		DBThread::Do(DBThread::Default, "CALL `wwii`.`p_add_player_note`('%s',%u,4,'%s')", target->callsign(), player.playerID(), sqlReason.c_str());
	}
} dotKick;

//////////////////////////////////////////////////////////////////////
// Kick ALL players
//////////////////////////////////////////////////////////////////////

//! @internal
class DotKickAll : public DotCommand
{
public:
	DotKickAll() : DotCommand("kickall", "Eject ALL players from the game"
								, IS_PLAYNET, LSTR_KICKALL_USAGE, LSTR_KICKALL_DETAIL
								, true
								) { }

	virtual void actionWith(Player& player, Chat::ClientMessage* /*message*/, char* arguments) const override
	{
		player.gmNote("GM", command(), arguments, nullptr);

		PLAYER_INDEX::iterator it, cur;
		for ( it = playerIndex.begin(); it != playerIndex.end(); )
		{
			Player* target = it->second;
			++it;

			if ( target->matchHandle() == player.matchHandle() )
				target->message(LSTR_WARNING, LSTR_KICKING_ALL);
			else if ( player.isPlaynet() )
				target->message(LSTR_WARNING, LSTR_LABELLED_GENERIC, player.callsign(), arguments);
			else
			{
				target->message(LSTR_WARNING, LSTR_GENERIC, arguments, nullptr);
				_smite(target);
			}
		}
	}
} dotKickAll;

//////////////////////////////////////////////////////////////////////
// Despawn all players
//////////////////////////////////////////////////////////////////////

//! @internal
class DotDespawnAll : public DotCommand
{
public:
	DotDespawnAll() : DotCommand("despawnall", "Force all players to despawn"
								, GAME_MANAGER, LSTR_DESPAWNALL_USAGE, LSTR_DESPAWNALL_DETAIL
								, false
								) { }

	virtual void actionWith(Player& player, Chat::ClientMessage* /*message*/, char* arguments) const override
	{
		player.gmNote("GM", command(), arguments, nullptr);
		PREP_CHAT_MSG(cmsg, "%s forcing all players to despawn: %s", player.callsign(), arguments);
		chatDispatchGMMessage("INFO", cmsg);

		UINT32 affected = 0;

		PLAYER_INDEX::iterator it;
		for ( it = playerIndex.begin(); it != playerIndex.end(); )
		{
			Player* target = it->second;
			++it;

			if ( chatSendForcedDespawn(target, LSTR_DESPAWN_ALL, arguments) )
				++affected;
		}

		FORMAT_CHAT_MSG(cmsg, "%u %s affected.", affected, (affected != 1) ? "players" : "player");
		player.message(LSTR_NOTE, LSTR_GENERIC, cmsg);
	}
} dotDespawnAll;

//////////////////////////////////////////////////////////////////////
// Kick and ban a player
//////////////////////////////////////////////////////////////////////

//! @internal
class DotKickBan : public DotCommand
{
public:
	DotKickBan() : DotCommand("kickban", "Kick a player and temporarily suspend them"
								, NO_MASK, LSTR_KICKBAN_USAGE, LSTR_KICKBAN_DETAIL
								) {}

	virtual void actionWith(Player& player, Chat::ClientMessage* /*message*/, char* arguments) const override
	{
		static ARGV argv;
		int argc = vectorize(arguments, argv, 3);
		if ( argc != 3 )
		{
			help(player);
			return;
		}

		_executeBan(player, argv, true);
	}
} dotKickBan;

//////////////////////////////////////////////////////////////////////
// List the last captures
//////////////////////////////////////////////////////////////////////

//! @internal
class LastCapQuery : public DBThreadQuery
{
public:
	LastCapQuery(const ChatHandle& byMatchID, const char* clause)
		: DBThreadQuery(DBThread::Default, "LastCapQuery", DBThreadQuery::_CALLBACK
			, "SELECT * FROM `strat`.`v_last_captures` %s LIMIT 8", clause
			)
		, m_matchID(byMatchID)
		, m_results()
		{
			m_results.clear();
		}

public:
	virtual ~LastCapQuery() {}

protected:
	ChatHandle	m_matchID;
	std::vector<std::string> m_results;

protected:
	virtual bool Process(const size_t numRows) override
	{
		if ( numRows <= 0 )
			return true;

		char buffer[1024];
		while ( Query()->FetchRow() )
		{
			DBRow& row = Query()->Row();
			UINT32 ctry = atoi(row[1]);
			strprintf(buffer, "%19s %s@%s(%s) %s", row[0], row[2], row[3], _countryName[ctry], row[4]);
			m_results.insert(m_results.begin(), buffer);
		}

		return true;
	}

protected:
	virtual void Success() override
	{
		Player* const player = PlayerInterface::Lookup(m_matchID);
		if ( player == nullptr )
			return;

		const size_t numRows = m_results.size();
		if ( numRows <= 0 )
		{
			player->message(LSTR_CAUTION, LSTR_LASTCAP_NO_MATCH);
			return;
		}

		for ( size_t i = 0; i < numRows; ++i )
		{
			player->message(LSTR_NONE, LSTR_GENERIC, m_results[i].c_str());
		}
	}

protected:
	virtual void Failure() override
	{
		Player* player = PlayerInterface::Lookup(m_matchID);
		if ( player == nullptr )
			return;

		player->message(LSTR_CAUTION, LSTR_DB_QUERY_FAILED, "lastcap", "Database Error");
	}
};

//! @internal
class DotLastCap : public DotCommand
{
public:
	DotLastCap() : DotCommand("lastcap", "List the last few captures"
								, NO_MASK, LSTR_LASTCAP_USAGE, LSTR_LASTCAP_DETAIL
							) {}

	virtual void action(Player& player, Chat::ClientMessage* /*message*/) const override
	{
		char clause[MAX_MESSAGE_LENGTH+1];
		clause[0] = '\0';
		Property* property = chatFindProperty(player.uiTarget());
		if ( property != nullptr )
		{
			if ( property->IsCP() )
				snprintf(clause, MAX_MESSAGE_LENGTH, "WHERE cp_oid = %u", property->OID());
			else if ( property->IsFacility() )
				snprintf(clause, MAX_MESSAGE_LENGTH, "WHERE facility_oid = %u", property->OID());
			player.gmNote("GM", command(), property->Name(), nullptr);
		}
		else
			player.gmNote("GM", command(), "global", nullptr);

		(new LastCapQuery(player.matchHandle(), clause))->Queue();
	}

	virtual void actionWith(Player& player, Chat::ClientMessage* /*message*/, char* arguments) const override
	{
		char clause[MAX_MESSAGE_LENGTH+1];
		clause[0] = '\0';

		if ( strncasecmp(arguments, "glob", 4) == 0 || strcasecmp(arguments, "all") == 0 )
		{
			player.gmNote("GM", command(), "global", nullptr);
			(new LastCapQuery(player.matchHandle(), clause))->Queue();
			return;
		}

		UINT32 i = isSideName(arguments);
		if ( i == ALLIED )
		{
			player.gmNote("GM", command(), "allied", nullptr);
			snprintf(clause, MAX_MESSAGE_LENGTH, "AND ctry IN (1,2,3,7,8)");
		}
		else if ( i == AXIS )
		{
			player.gmNote("GM", command(), "axis", nullptr);
			snprintf(clause, MAX_MESSAGE_LENGTH,  "AND ctry IN (4,5,6)");
		}
		else
		{
			i = isCountry(arguments);
			if ( i > WWII_NO_COUNTRY )
			{
				player.gmNote("GM", command(), _countryName[i], nullptr);
				snprintf(clause, MAX_MESSAGE_LENGTH, "AND ctry = %u", i);
			}
			else
			{
				Chat::Name callsign(chatGetHandle(arguments));
				player.gmNote("GM", command(), callsign.c_str(), nullptr);
				snprintf(clause, MAX_MESSAGE_LENGTH, "AND call = '%s'", callsign.c_str());
			}
		}

		(new LastCapQuery(player.matchHandle(), clause))->Queue();
	}
} dotLastCap;

//////////////////////////////////////////////////////////////////////
// List the last killers of a particular player
//////////////////////////////////////////////////////////////////////

//! @internal
class LastDeathsQuery : public DBThreadQuery
{
public:
	LastDeathsQuery(const ChatHandle& requester, const ChatHandle& handleID)
		: DBThreadQuery(DBThread::Default, "LastDeathsQuery", DBThreadQuery::_CALLBACK
							, "CALL wwii.p_get_last_killers('%s')"
								, Chat::Name(handleID).c_str())
		, m_requesterID(requester)
		, m_callsign(handleID)
		, m_results()
		{
			m_results.clear();
		}
	virtual ~LastDeathsQuery() {}

protected:
	ChatHandle	m_requesterID;
	Chat::Name	m_callsign;
	std::vector<std::string> m_results;

protected:
	virtual bool Process(const size_t numRows) override
	{
		if ( numRows <= 0 )
			return true;

		std::string line;
		while ( Query()->FetchRow() )
		{
			DBRow& row = Query()->Row();
			line = row[0];
			line += '/';
			line += row[2];
			line += " by ";
			line += row[1];
			line += '/';
			line += row[3];
			m_results.insert(m_results.begin(), line);
		}

		return true;
	}

public:
	virtual void Success() override
	{
		Player* const player = PlayerInterface::Lookup(m_requesterID);
		if ( player == nullptr )
			return;

		const size_t numRows = m_results.size();
		if ( numRows <= 0 )
		{
			player->message(LSTR_NOTE, LSTR_NO_MATCHES);
			return;
		}

		player->message(LSTR_NOTE, LSTR_LAST_DEATHS, m_callsign.c_str());

		for ( size_t i = 0; i < numRows; ++i )
		{
			player->message(LSTR_NONE, LSTR_GENERIC, m_results[i].c_str());
		}
	}

	virtual void Failure() override
	{
		Player* player = PlayerInterface::Lookup(m_requesterID);
		if ( player != nullptr )
			player->message(LSTR_CAUTION, LSTR_DB_QUERY_FAILED, "lastdeath", "Database Error");
	}
};

//! @internal
class DotLastDeaths : public DotCommand
{
public:
	DotLastDeaths() : DotCommand("lastdeaths", "List the last killers of a given player"
									, NO_MASK, LSTR_LASTDEATH_USAGE, LSTR_LASTDEATH_DETAIL
								) {}

	virtual void actionWith(Player& player, Chat::ClientMessage* /*message*/, char* arguments) const override
	{
		ChatHandle handle = chatGetLowercaseHandle(arguments);
		if ( !chatIsValidCallsign(arguments) || handle == 0 )
		{
			player.message(LSTR_CAUTION, LSTR_INVALID_CALLSIGN, arguments);
			return;
		}

		player.gmNote("GM", command(), arguments, nullptr);

		if ( !player.isPlaynet() && handle == player.matchHandle() )
		{
			player.message(LSTR_CAUTION, LSTR_GENERIC, "Due to overuse, we have disabled self-queries.");
			return;
		}

		(new LastDeathsQuery(player.matchHandle(), handle))->Queue();
	}
} dotLastDeaths;

//////////////////////////////////////////////////////////////////////
// List the last kills of a particular player
//////////////////////////////////////////////////////////////////////

//! @internal
class LastKillsQuery : public DBThreadQuery
{
public:
	LastKillsQuery(const ChatHandle& requester, const ChatHandle& handleID)
		: DBThreadQuery(DBThread::Default, "LastKillsQuery", DBThreadQuery::_CALLBACK
						, "CALL wwii.p_get_last_kills('%s')"
							, Chat::Name(handleID).c_str())
		, m_requesterID(requester)
		, m_callsign(handleID)
		, m_results()
		{
			m_results.clear();
		}
	virtual ~LastKillsQuery() {}

protected:
	ChatHandle	m_requesterID;
	Chat::Name	m_callsign;
	std::vector<std::string> m_results;

protected:
	virtual bool Process(const size_t numRows) override
	{
		if ( numRows <= 0 )
			return true;

		std::string line;
		while ( Query()->FetchRow() )
		{
			DBRow& row = Query()->Row();
			line = row[0];
			line += '/';
			line += row[1];
			line += " on ";
			line += row[2];
			line += '/';
			line += row[3];
			m_results.insert(m_results.begin(), line);
		}

		return true;
	}

public:
	virtual void Success() override
	{
		Player* const player = PlayerInterface::Lookup(m_requesterID);
		if ( player == nullptr )
			return;

		const size_t numRows = m_results.size();
		if ( numRows <= 0 )
		{
			player->message(LSTR_NOTE, LSTR_NO_MATCHES);
			return;
		}

		player->message(LSTR_NOTE, LSTR_LAST_KILLS, m_callsign.c_str());

		for ( size_t i = 0; i < numRows; ++i )
		{
			player->message(LSTR_NONE, LSTR_GENERIC, m_results[i].c_str());
		}
	}

	virtual void Failure() override
	{
		Player* const player = PlayerInterface::Lookup(m_requesterID);
		if ( player != nullptr )
			player->message(LSTR_CAUTION, LSTR_DB_QUERY_FAILED, "lastkills", "Database Error");
	}
};

//! @internal
class DotLastKills : public DotCommand
{
public:
	DotLastKills() : DotCommand("lastkills", "List the last kills of a given player"
									, NO_MASK, LSTR_LASTKILLS_USAGE, LSTR_LASTKILLS_DETAIL
								) {}

	virtual void actionWith(Player& player, Chat::ClientMessage* /*message*/, char* arguments) const override
	{
		ChatHandle handle = chatGetLowercaseHandle(arguments);
		if ( !chatIsValidCallsign(arguments) || handle == 0 )
		{
			player.message(LSTR_CAUTION, LSTR_INVALID_CALLSIGN, arguments);
			return;
		}

		player.gmNote("GM", command(), arguments, nullptr);
		if ( !player.isPlaynet() && handle == player.matchHandle() )
		{
			player.message(LSTR_CAUTION, LSTR_GENERIC, "Due to overuse, we have disabled self-queries.");
			return;
		}

		(new LastKillsQuery(player.matchHandle(), handle))->Queue();
	}
} dotLastKills;

//////////////////////////////////////////////////////////////////////
// List Squad COs currently online.
//////////////////////////////////////////////////////////////////////

class ListCOs : public DotCommand
{
public:
	ListCOs() : DotCommand("listcos", "List all SquadCOs currently online"
						   , NO_MASK, LSTR_LISTCOS_USAGE, LSTR_LISTCOS_DETAIL)
	{
	}

	virtual void action(Player& player, Chat::ClientMessage * /*message*/) const override
	{
		PLAYER_INDEX::iterator it;
		std::string result = "";
		for (it = playerIndex.begin(); it != playerIndex.end(); ++it)
		{
			Player* subject = it->second;

			if (!subject->squadID() || subject->squadRank() != Squad::CO)
				continue;

			if (!subject->isVisibleTo(player))
				continue;

			const char* squadName = chatGetSquadCallsign(subject->squadID());
			if (!squadName)
				continue;

			if (result.length() + 20 >= MAX_MESSAGE_LENGTH)
			{
				player.message(LSTR_NOTE, LSTR_GENERIC, result.c_str());
				result = "";
			}

			subject->addName(result, ", ");
			result += ":";
			result += squadName;
		}

		if (result.empty())
			player.message(LSTR_CAUTION, LSTR_NO_MATCHES);
		else
			player.message(LSTR_NOTE, LSTR_GENERIC, result.c_str());
	}
} listCOs;

//////////////////////////////////////////////////////////////////////
// List the members of a given squad
//////////////////////////////////////////////////////////////////////

class ListSquad : public DotCommand
{
public:
	ListSquad() : DotCommand("listsquad", "List the members of an [active] squad"
								, NO_MASK, LSTR_LISTSQUAD_USAGE, LSTR_LISTSQUAD_DETAIL) {}

	virtual void actionWith(Player& player, Chat::ClientMessage* /*message*/, char* arguments) const override
	{
		bool listAll = false;

		ARGV argv;
		int argc = vectorize(arguments, argv, 2);

		if ( argc > 0 && strcasecmp(argv[0], "-all") == 0 )
		{
			listAll = true;
			argv.erase(argv.begin());
			argc--;
		}

		if ( argc != 1 )
		{
			help(player);
			return;
		}

		SquadList* list = chatFindSquadListByName(argv[0]);
		if ( list == nullptr )
		{
			if ( chatCheckForSquadName(argv[0]) )
			{
				player.message(LSTR_CAUTION, LSTR_NO_SUCH_SQUAD, argv[0]);
				return;
			}

			player.message(LSTR_NOTE, LSTR_SQUAD_NOT_ONLINE, argv[0]);
			return;
		}

		// No more than once per minute
		if ( _featureLockedM(player, 1) )
			return;

		const Squad& squad = list->squad();
		std::string response = "";
		response += "Sq#";
		response += std::to_string(squad.squadID());
		response += " ";
		response += squad.callsign();
		response += ": ";
		response += squad.name();
		response += ". Online: ";

		response += std::to_string(list->online());
		response += "/";
		response += std::to_string(list->members().size());
		response += ".\n";
		response += "Created: ";
		response += chatTimeString(squad.created());
		response += " by ";
		response.append(Chat::Name(squad.creatorHandle()).c_str());
		response += ". ";
		response += "CO: ";
		response.append(Chat::Name(squad.coHandle()).c_str());
		response += ". ";
		response += "R: ";
		response += (squad.recruiting() ? "Yes" : "No");
		response += ". ";
		response += "V: ";
		response += (squad.votingEnabled() ? "Yes" : "No");
		response += ". ";
		response += "URL: ";
		response += squad.homepage();

		player.message(LSTR_NOTE, LSTR_GENERIC, response.c_str());

		if ( !listAll && list->online() == 0 )
			return;

		response = "";
		SQUAD_MEMBERS::iterator it;
		LSTRING_ID lstrID = LSTR_TEXT_START;
		std::string entry = "";

		for ( it = list->begin(); it != list->end(); ++it )
		{
			SquadMemberEntry& member = it->second;

			if ( !listAll && !member.player() )
				continue;


			if ( listAll )
			{
				if ( !response.empty() )
					response += " ";
				entry = Chat::Name(it->first).c_str();
			}
			else
			{
				if ( !response.empty() )
					response += ", ";
				const Player* subject = member.player();
				entry = "";
				subject->addName(entry);
			}

			if ( response.length() + entry.length() + 2 >= MAX_MESSAGE_LENGTH )
			{
				player.message(LSTR_NONE, lstrID, response.c_str());
				lstrID = LSTR_TEXT;
				response = "";
			}

			response += entry;
		}

		if ( response.length() )
		{
			lstrID = (lstrID == LSTR_TEXT_START) ? LSTR_GENERIC : LSTR_TEXT_END;
			player.message(LSTR_NONE, lstrID, response.c_str());
		}
	}
} dotListSquad;

//////////////////////////////////////////////////////////////////////
// GM command for attaching/leaving squads
//////////////////////////////////////////////////////////////////////

//! @internal
class DotMakeCO : public DotCommand
{
public:
	DotMakeCO() : DotCommand("makeco", "Promote a player to the status of CO in their squad"
									, NO_MASK, LSTR_MAKECO_USAGE, LSTR_MAKECO_DETAIL
								) {}

	void
	actionWith(Player& player, Chat::ClientMessage* /*message*/, char* arguments) const override
	{
		ARGV argv;
		int argc = vectorize(arguments, argv, 3);

		bool force = false;
		if ( strcasecmp(argv[0], "-force") == 0 )
		{
			force = true;
			argv.erase(argv.begin());
			argc--;
		}

		if ( argc < 1 )
		{
			help(player);
			return;
		}

		ChatHandle handle = chatGetLowercaseHandle(argv[0]);
		Player* subject = PlayerInterface::Lookup(handle);
		if ( subject == nullptr )
		{
			player.message(LSTR_CAUTION, LSTR_USER_UNAVAILABLE, argv[0]);
			return;
		}
		if ( !subject->squadID() )
		{
			player.message(LSTR_CAUTION, LSTR_PLAYER_NOT_IN_A_SQUAD, subject->callsign());
			return;
		}

		if ( subject->squadRank() == Squad::CO )
		{
			player.message(LSTR_CAUTION, LSTR_PLAYER_ALREADY_CO, subject->callsign(), Chat::Name(subject->squadHandle()).c_str());
			return;
		}

		SquadList* list = chatFindSquadList(subject->squadID());
		if ( list == nullptr || list->online() == 0 )
		{
			player.message(LSTR_CAUTION, LSTR_SQUAD_NOT_ONLINE, Chat::Name(subject->squadHandle()).c_str());
			return;
		}

		auto& squad = list->squad();
		ChatHandle oldCO = squad.coHandle();
		if ( oldCO != 0LL && oldCO != handle )
		{
			Player* co = PlayerInterface::Lookup(oldCO);
			if ( co )
			{
				player.message(LSTR_CAUTION, LSTR_SQUAD_CO_ONLINE, squad.shortName(), co->callsign());
				return;
			}
			else if ( !force )
			{
				player.message(LSTR_CAUTION, LSTR_MAKECO_USE_FORCE, squad.shortName(), Chat::Name(oldCO).c_str());
				return;
			}
		}

		list->appointNewCO(subject->matchHandle(), subject);

		player.gmNote("GM", command(), arguments, squad.shortName());
		list->sendMessage(LSTR_SQUAD, LSTR_SQUAD_MAKECO_APPOINTED, subject->callsign(), squad.shortName());
		player.message(LSTR_NOTE, LSTR_MAKECO_APPOINTED, subject->callsign(), squad.shortName());
	}
} dotMakeCO;

//////////////////////////////////////////////////////////////////////
// GM command to show information about a player
//////////////////////////////////////////////////////////////////////

//! @internal
class QueryPlayerInfo : public DBThreadQuery
{
public:
	QueryPlayerInfo(const ChatHandle& byMatchID, const ChatHandle& handle)
		: DBThreadQuery(DBThread::Default, "QueryPlayerInfo", DBThreadQuery::_CALLBACK
							, "CALL auth.p_get_player_details((SELECT playerid FROM wwiionline.wwii_player WHERE callsign = '%s'), %u)"
								, Chat::Name(handle).c_str(), Server::myServerInfo.stats.arenaID
							)
		, m_matchID(byMatchID)
		, m_handle(handle)
		, m_name()
		{
			m_response[0] = 0;
		}
protected:
	ChatHandle	m_matchID, m_handle;
	Chat::Name	m_name;
	char		m_response[MAX_MESSAGE_LENGTH];

protected:
	virtual bool Process(const size_t numRows) override
	{
		if ( numRows <= 0 || Query()->FetchRow() == false )
			return true;

		AuthPlayerDetails details(Query()->Row());
		char* p = m_response;

		p += sprintf(p, "Pid#%u", details.playerID);
		if ( details.squadCallsign && details.squadRank )
			p += sprintf(p, " %s:%s.", chatGetSquadRankName(details.squadRank), details.squadCallsign);

		p += sprintf(p, " Not Online.");
		if ( details.lastLogin == nullptr || *details.lastLogin == 0 )
			p += sprintf(p, " No logins recorded.");
		else
			p += sprintf(p, " Last Login: %s", details.lastLogin);

		if ( details.lockSide == 3 && details.lockTime > (UINT32)teulTime() && details.lockTimePretty != nullptr && *details.lockTimePretty )
		{
			if ( details.lockTime > teulTime() + (7 * DAY_S) )
				p += sprintf(p, " BANNED UNTIL %s.", details.lockTimePretty);
			else
				p += sprintf(p, " BANNED FOR %s.", chatTimeInterval(details.lockTime - teulTime()));
		}
		else if ( details.lockSide > 0 )
			p += sprintf(p, " Last side: %s", _sideName[details.lockSide]);

		m_name = details.callsign;

		return true;
	}

protected:
	virtual void Success() override
	{
		Player* player = PlayerInterface::Lookup(m_matchID);
		if ( player == nullptr )
			return;

		if (m_name.Empty() || (m_response[0] == 0))
		{
			player->message(LSTR_CAUTION, LSTR_INVALID_CALLSIGN, Chat::Name(m_handle).c_str());
		}
		else
		{
			player->message(LSTR_NONE, LSTR_LABELLED_GENERIC, m_name.c_str(), m_response);
		}
	}

	virtual void Failure() override
	{
		Player* player = PlayerInterface::Lookup(m_matchID);
		if ( player == nullptr ) return;
		player->message(LSTR_CAUTION, LSTR_DB_QUERY_FAILED, "pinfo", "Database Error");
	}
};

//! @internal
class DotPInfo : public DotCommand
{
public:
	DotPInfo() : DotCommand("pinfo", "Display player info"
								, NO_MASK, LSTR_PINFO_USAGE, LSTR_PINFO_DETAIL
								) { }

	virtual void actionWith(Player& player, Chat::ClientMessage* message, char* arguments) const override;
	virtual void action(Player& player, Chat::ClientMessage* /*message*/) const override
	{
		player.describeTo(player, player.lastX(), player.lastY(), false);
	}

} dotPInfo;

void
_addStr(std::string & dest, const char* src)
{
	dest.append(src);
}

static const char*
_rankType(Rank::TYPE type)
{
	switch (type)
	{
	case Rank::ENLISTED:
		return "Enlisted";
	case Rank::OFFICER:
		return "Officer";
	case Rank::COMMAND:
		return "Commander";
	case Rank::STAFF:
		return "STAFF";
	case Rank::NO_TYPE:
	default:
		return "???";
	}
}

void
DotPInfo::actionWith(Player& player, Chat::ClientMessage* message, char* arguments) const
{
	bool includePermissions = false;

	// Check for '-f' (full) option.
	const char* sourceName = arguments;
	if ( sourceName[0] == '-' )
	{
		if ( sourceName[1] == 'f' )
		{
			includePermissions = true;
			sourceName = strchr(sourceName, ' ');
			if ( sourceName )
			{
				while ( *sourceName == ' ' )
					++sourceName;
			}
		}
		else
		{
			char usage[80];
			strprintf(usage, "/%s [-full] [gamename]", command());
			player.message(LSTR_USAGE, LSTR_GENERIC, usage);
			return;
		}
	}

	if ( !sourceName || !*sourceName )
	{
		return action(player, message);
	}

	ChatHandle handle = chatGetLowercaseHandle(sourceName);
	const Player* const target = PlayerInterface::Lookup(handle);
	if ( target == nullptr )
	{
		(new QueryPlayerInfo(player.matchHandle(), handle))->Queue();
		return;
	}

	target->describeTo(player, target->lastX(), target->lastY(), includePermissions);
}

void
Player::describeTo(const Player& player, const FLOAT64& x, const FLOAT64& y, bool includePermissions) const
{
	std::string response;
	char buf[128+1];
	buf[0] = '\0';

	if ( player.isPlaynet() )
	{
		snprintf(buf, sizeof(buf), "Pid#%u", playerID());
		_addStr(response, buf);
	}

	if ( player.instance().isCampaign() )
		_addStr(response, "in:campaign");
	else if ( player.instance().id() == 0 )
		_addStr(response, "in:none");
	else
	{
		snprintf(buf, sizeof(buf), "in:%llu", player.instance().id());
		_addStr(response, buf);
	}

	if ( squadID() )
	{
		const char* squadName = chatGetSquadCallsign(squadID());
		if ( squadName != nullptr && squadRank() > 0 )
		{
			snprintf(buf, sizeof(buf), "%s:%s"
				, chatGetSquadRankName(squadRank())
				, squadName);
		}
		else if ( squadRank() == 0 && player.isPlaynet() )
		{
			snprintf(buf, sizeof(buf), "Sqd:%s (UNRANKED!)", squadName);
		}
		else if ( player.isPlaynet() )
		{
			snprintf(buf, sizeof(buf), "Sqd#%u(NO DATA)", squadID());
		}
		else
		{
			strsafecpy(buf, "[Invalid Squad]");
		}
		if ( getPreference(CPR_IS_RECRUITING) )
		{
			if ( side() == player.side() && isVisibleTo(player) && !isAFK() && missionID() )
				strcat(buf, "[R]");
			else
				strcat(buf, "[r]");
		}
		_addStr(response, buf);
	}

	if ( brigadeID() )
	{
		HCUnit* brig = HCUnit::Lookup(brigadeID());
		if ( brig )
		{
			std::string descriptor;
			brig->getDescriptor(descriptor);
			_addStr(response, descriptor.c_str());
		}
		else if ( player.isPlaynet() )
		{
			snprintf(buf, sizeof(buf), "B#%u:UNK", brigadeID());
			_addStr(response, buf);
		}
	}

	if ( effectiveRank() > 0 )
	{
		RANK_NO erank = effectiveRank();
		const Rank* myRank = Rank::lookup(erank);
		if ( myRank != nullptr )
			snprintf(buf, sizeof(buf)
						, "R#%u(%s/%s)"
							, erank
							, _rankType(myRank->type())
							, rankNames[erank]
							);
		else if ( player.isPlaynet() )
			snprintf(buf, sizeof(buf), "R#%u(INVALID)", erank);
		_addStr(response, buf);
	}

	if ( isAFK() )
		_addStr(response, "AFK");
	if ( inGrab() )
		_addStr(response, "Grab");

	if ( leadingBrigade() )
	{
		char oic[21];
		HCUnit* unit = HCUnit::Lookup(leadingBrigade());
		if ( unit )
			snprintf(oic, sizeof(buf), "OIC:%s", unit->callsign());
		else if ( player.isPlaynet() )
			snprintf(oic, sizeof(buf), "OIC:#%u", leadingBrigade());
		_addStr(response, "OIC");
	}
	if ( permissions(ALLIED_COMMAND_STAFF) )
		_addStr(response, (side() == ALLIED) ? "AHC" : "ahc");
	if ( permissions(AXIS_COMMAND_STAFF) )
		_addStr(response, (side() == AXIS) ? "GHC" : "ghc");
	if ( isActiveOfficer() )
		_addStr(response, "ActOff");
	if ( hcOff() )
		_addStr(response, "<HC:off>");
	if ( m_flags.checksumsDirty )
		_addStr(response, "!BadFE!");
	if ( player.isPlaynet() )
	{
		char permissionsHex[64];
		snprintf(permissionsHex, sizeof(permissionsHex), "PM: %016llx", permissionMask());
		_addStr(response, permissionsHex);
	}

	player.message(LSTR_NONE, LSTR_LABELLED_GENERIC, callsign(), response.c_str());

	if ( includePermissions )
	{
		// Strip dark glasses/playnet/gm permission for anyone but playnet/dark glasses accounts
		PermissionMask filteredPermissions = permissionMask();
		if ( permissions(DARK_GLASSES) && !player.permissions(DARK_GLASSES | IS_PLAYNET) )
		{
			filteredPermissions &= ~(DARK_GLASSES|IS_PLAYNET|GAME_MANAGER);
		}

		LSTRING_ID lstrID = LSTR_TEXT_START;
		UINT32 index = 0;
		do
		{
			index = chatGetPermissionNames(filteredPermissions, response, index);
			if ( response[0] != 0 )
			{
				if ( index == 0 )
					lstrID = LSTR_TEXT_END;
				player.message(LSTR_NONE, lstrID, response.c_str());
			}
		}
		while ( index > 0 );
		if ( lstrID == LSTR_TEXT_START )
			player.message(LSTR_CAUTION, LSTR_GENERIC, "Player has no permissions assigned");
	}

	whereabouts(response, x, y);

	player.message(LSTR_NONE, LSTR_GENERIC, response.c_str(), nullptr);

	return;
}

//////////////////////////////////////////////////////////////////////
// Force re-read of chat permissions
//////////////////////////////////////////////////////////////////////

//! @internal
class DotResync : public DotCommand
{
public:
	DotResync() :
						DotCommand("resync", "Force reload of global or single player permissions"
								, NO_MASK, LSTR_RESYNC_USAGE, LSTR_RESYNC_DETAIL
								, true
								)
								{ }

	virtual void action(Player& player, Chat::ClientMessage* /*message*/) const override
	{
		if ( _featureLockedM(player, RESYNC_LOCK_TIME_M) )
			return;

		index()->readPermissions();
		chatReadGroupPermissions();

		PLAYER_INDEX::iterator it;
		for ( it = playerIndex.begin(); it != playerIndex.end(); ++it )
			it->second->invalidateGroups();

		player.message(LSTR_SYSTEM, LSTR_ALL_CHAT_PERMS_RELOADED);

		return;
	}

	virtual void actionWith(Player& player, Chat::ClientMessage* /*message*/, SCHAR* arguments) const override
	{
		ARGV args;
		if ( vectorize(arguments, args, 2) != 1 )
		{
			// Don't know what you're talking about. Go away.
			help(player);
			return;
		}

		// Locate the user
		Player* subject = PlayerInterface::Lookup(args[0]);
		if ( subject == nullptr )
		{
			player.message(LSTR_CAUTION, LSTR_USER_UNAVAILABLE, args[0], nullptr);
			return;
		}

		subject->readGroups();

		player.message(LSTR_SYSTEM, LSTR_PLAYER_CHAT_PERMS_RELOADED, subject->callsign());
	}
} dotResync;

//////////////////////////////////////////////////////////////////////
// The .roster command
//////////////////////////////////////////////////////////////////////

//! @internal
class DotRoster : public DotCommand
{
public:
	DotRoster() : DotCommand("roster", "Show player counts"
								, NO_MASK, LSTR_ROSTER_USAGE, LSTR_ROSTER_DETAIL
								) { }

	virtual void action(Player& player, Chat::ClientMessage* message) const override
	{
		actionWith(player, message, nullptr);
	}

	virtual void actionWith(Player& player, Chat::ClientMessage* /*message*/, SCHAR* arguments) const override
	{
		UINT32 axis = 0, allied = 0;
		char _roster[100+1];		// Place to store rosters we generate
		if ( arguments == nullptr || strcmp(arguments, "all") == 0 )
		{
			axis = _sideCount[AXIS];
			allied = _sideCount[ALLIED];
		}
		else if ( strncmp(arguments, "camp", 4) == 0 )
		{
			axis = instGetCampaignSideCount(AXIS);
			allied = instGetCampaignSideCount(ALLIED);
		}
		else
		{
			strprintf(_roster, "Usage: /%s [all|campaign]", command());
			player.message(LSTR_CAUTION, LSTR_GENERIC, _roster);
			return;
		}

		strprintf(_roster, "Axis: %d. Allied: %d.", axis, allied);
		player.message(LSTR_NONE, LSTR_GENERIC, _roster, nullptr);
	}

} dotRoster;


//////////////////////////////////////////////////////////////////////
// Set the sysmotd
//////////////////////////////////////////////////////////////////////

//! @internal
class DotSetMotd : public DotCommand
{
public:
	DotSetMotd() : DotCommand("setmotd", "Set the system message of the day"
								, NO_MASK, LSTR_SYSMOTD_STAFF_USAGE, LSTR_SYSMOTD_STAFF_DETAIL
								) { }
	virtual void actionWith(Player& player, Chat::ClientMessage* /*message*/, char* arguments) const override
	{
		player.gmNote("GM", command(), arguments, nullptr);

		// Set a new motd
		chatSetUnitMotd(SYSTEM_MOTD_UNIT, arguments);

		chatSystemMessage(LSTR_MOTD, chatGetUnitMotd(SYSTEM_MOTD_UNIT));
	}
} dotSetMotd;

//////////////////////////////////////////////////////////////////////
// Change your rank
//////////////////////////////////////////////////////////////////////

//! @internal
class DotSetRank : public DotCommand
{
public:
	DotSetRank() : DotCommand("setrank", "Set your current persona's rank"
								, NO_MASK, LSTR_SETRANK_USAGE, LSTR_SETRANK_DETAIL
								) {}

	virtual void actionWith(Player& player, Chat::ClientMessage* /*message*/, char* arguments) const override
	{
		if ( player.isSpawned() )
		{
			player.message(LSTR_CAUTION, LSTR_ALREADY_SPAWNED);
			return;
		}

		if ( !isdigit(arguments[0]) || arguments[0] == '0' )
		{
			char max[11];
			strprintf(max, "%u", Rank::max());
			player.message(LSTR_CAUTION, LSTR_INVALID_RANK, max);
			return;
		}

		RANK_NO rankNo = atoi(arguments);
		const Rank* rank = Rank::lookup(rankNo);
		if ( !player.isPlaynet() && rank && rank->type() == Rank::STAFF )
			rank = nullptr;

		if ( rank == nullptr )
		{
			char max[11];
			strprintf(max, "%u", Rank::max());
			player.message(LSTR_CAUTION, LSTR_INVALID_RANK, max);
			return;
		}

		DBThread::Do(DBThread::Default, "UPDATE wwii_persona p, wwii_rank r SET p.rankid = %u, p.rankpoints = (r.pointsrequired - 20)"
						" WHERE p.playerID = %u AND p.countryid = %u and p.branchid = %u and r.rankid = %u"
						, rankNo, player.playerID(), player.country(), player.branch(), rankNo);

		player.message(LSTR_NOTE, LSTR_RANK_SET, rankNames[rankNo]);
	}
} dotSetRank;

//////////////////////////////////////////////////////////////////////
// Invoke a system shutdown
//////////////////////////////////////////////////////////////////////

//! @internal
class DotShutdown : public DotCommand
{
public:
	DotShutdown() : DotCommand("shutdown", "Notify players of imminent shutdown"
											, IS_PLAYNET, LSTR_NONE, LSTR_NONE) {}

	virtual void action(Player& player, Chat::ClientMessage* /*message*/) const override
	{
		SINT32 remaining = chatGetShutdownTimeRemaining();
		if ( remaining >= 0 )
		{
			player.message(LSTR_NOTE, LSTR_LABELLED_GENERIC, "Shutdown scheduled for", chatTimeInterval(remaining));
		}
		else
		{
			player.message(LSTR_NOTE, LSTR_GENERIC, "No shutdown scheduled");
		}
	}

	virtual void actionWith(Player& player, Chat::ClientMessage* message, char* arguments) const override
	{
		ARGV argv;
		int args = vectorize(arguments, argv, 3);

		if ( strcasecmp(argv[0], "help") == 0 )
		{
			if ( args >= 2 && strcasecmp(argv[1], "stop") == 0 )
			{
				player.message(LSTR_USAGE, LSTR_SHUTDOWN_STOP_USAGE);
				player.message(LSTR_DETAIL, LSTR_SHUTDOWN_STOP_DETAIL);
			}
			else if ( args >= 2 && strcasecmp(argv[1], "in") == 0 )
			{
				player.message(LSTR_USAGE, LSTR_SHUTDOWN_IN_USAGE);
				player.message(LSTR_DETAIL, LSTR_SHUTDOWN_IN_USAGE1);
				player.message(LSTR_DETAIL, LSTR_SHUTDOWN_IN_USAGE2);
			}
			else if ( args >= 2 && strcasecmp(argv[1], "downtime") == 0 )
			{
				player.message(LSTR_USAGE, LSTR_SHUTDOWN_DOWNTIME_USAGE);
				player.message(LSTR_DETAIL, LSTR_SHUTDOWN_DOWNTIME_DETAIL);
			}
			else if ( args >= 2 )
			{
				player.message(LSTR_CAUTION, LSTR_UNKNOWN_SHUTDOWN_COMMAND, argv[1]);
			}
			else
			{
				help(player);
			}

			return;
		}

		if ( strcmp(argv[0], "stop") == 0 )
		{
			chatSetShutdownTime(0, player.callsign());
			player.gmNote("SYS", "shutdown", "stopped", nullptr);
			return;
		}

		if ( strcmp(argv[0], "downtime") == 0 )
		{
			if ( argv.size() == 1 )
			{
				help(player);
				return;
			}
			UINT32 interval = 0;
			if ( ! chatGetTimeInterval(player, argv[1], interval) )
				return;
			chatSetShutdownDowntime(interval / 60);
		}

#ifdef BETA_RELEASE
		if ( strcmp(argv[0], "now") == 0 )
		{
			exit(0);
			return;
		}
#endif

		if ( strcmp(argv[0], "in") != 0 || argv.size() < 3 || !isdigit(*argv[1]) )
		{
			help(player);
			return;
		}

		UINT32 now = teulTime();
		UINT32 interval = 0;
		if ( !chatGetTimeInterval(player, argv[1], interval) )
			return;

#ifdef BETA_RELEASE
		static const UINT32 MIN_MINUTES = 1;
#else
		static const UINT32 MIN_MINUTES = 5;
#endif
		static const UINT32 MAX_HOURS = 12;

#define _QUOTEME(x) #x

		if ( interval < (UINT32)(MIN_MINUTES * MINUTE_S) || interval > (UINT32)(MAX_HOURS * HOUR_S) )
		{
			player.message(LSTR_CAUTION, LSTR_SHUTDOWN_LIMITS, _QUOTEME(MIN_MINUTES), _QUOTEME(MAX_MINUTES));
			return;
		}

		chatSetShutdownTime(now + interval, player.callsign());

		const char* intervalText = chatTimeInterval(interval);

		player.message(LSTR_NOTE, LSTR_SHUTDOWN_SCHEDULED, intervalText);

		player.gmNote("SYS", "shutdown", intervalText, (argv.size() > 2) ? argv[2] : nullptr);
		chatSystemMessage(LSTR_SHUTDOWN_ANNOUNCEMENT, intervalText, argv[2]);

		chatSetUnitMotd(SYSTEM_MOTD_UNIT, argv[2]);
	}

	virtual bool help(Player& player) const override
	{
		if ( !allow(player) )
			return false;
		player.message(LSTR_WARNING, LSTR_SHUTDOWN_WARNING);
		player.message(LSTR_USAGE, LSTR_SHUTDOWN_USAGE);
		player.message(LSTR_DETAIL, LSTR_SHUTDOWN_DETAIL);
		return true;
	}
} dotShutdown;

//////////////////////////////////////////////////////////////////////
// Snoop on a player's tells
//////////////////////////////////////////////////////////////////////

//! @internal
class DotSnoop : public DotCommand
{
public:
	DotSnoop() : DotCommand("snoop", "Records & displays all messages to/from a particular player"
								, NO_MASK, LSTR_SNOOP_USAGE, LSTR_SNOOP_DETAIL) {}

	virtual void action(Player& player, Chat::ClientMessage* /*message*/) const override
	{
		if ( !player.m_snoopOn )
			player.message(LSTR_NOTE, LSTR_NOT_SNOOPING, command());
		else
			player.message(LSTR_NOTE, LSTR_CURRENT_SNOOP, command(), player.m_snoopOn->callsign());
	}

	virtual void actionWith(Player& player, Chat::ClientMessage* message, char* arguments) const override
	{
		Player* target = player.m_snoopOn;
		if ( strcasecmp(arguments, "off") == 0 )
		{
			if ( !target )
				action(player, message);
			else
			{
				///LOCALIZE
				PREP_CHAT_MSG(cmsg, "%s is no-longer snooping on %s", player.callsign(), target->callsign());
				chatDispatchGMMessage("INFO", cmsg);
				player.m_snoopOn = nullptr;
				target->m_snoopBy = nullptr;
			}

			return;
		}

		ARGV argv;

		if ( vectorize(arguments, argv, 2) < 2 )
		{
			help(player);
			return;
		}

		if ( target )
			throw Chat::LStringErr(LSTR_USE_SNOOP_OFF, command(), target->callsign());

		///LOCALIZE
		if ( strlen(argv[1]) < 4 )
			throw Chat::LStringErr("Please give a reason for your snoop.");

		target = PlayerInterface::Lookup(argv[0]);
		if ( target == nullptr )
			throw Chat::LStringErr(LSTR_USER_UNAVAILABLE, argv[0]);
		if ( target->matchHandle() == player.matchHandle() )
			throw Chat::LStringErr(LSTR_CANT_VERB_SELF, command());

		// time to let people know about this attempt
		player.gmNote("GM", command(), arguments, argv[1]);
		if ( target->isPlaynet() )
			throw Chat::LStringErr(LSTR_NO_SNOOP_RATS);

		///LOCALIZE
		PREP_CHAT_MSG(cmsg, "%s snooping on %s: %s", player.callsign(), target->callsign(), argv[1]);
		chatDispatchGMMessage("INFO", cmsg);

		chatSendChatLog(target->playerID(), player.matchHandle());

		player.m_snoopOn = target;
		target->m_snoopBy = &player;
	}
} dotSnoop;

//////////////////////////////////////////////////////////////////////
// Super-secret Snoop on a player's tells
//////////////////////////////////////////////////////////////////////

//! @internal
class DotCRSSnoop : public DotCommand
{
public:
	DotCRSSnoop() : DotCommand("crssnoop", "CRS snoop that does not get announced to GMs"
								, IS_PLAYNET, LSTR_GENERIC, LSTR_GENERIC) {}

	virtual bool help(Player& player) const override
	{
		if ( !allow(player) ) return false;
		player.message(LSTR_USAGE, LSTR_GENERIC, "'/crssnoop <player>'  or  '/crssnoop off'");
		player.message(LSTR_DETAIL, LSTR_GENERIC, "Logs and displays private messages to/from a player. Use /crssnoop off to end a snoop session. Use /crssnoop for current snoop status.");
		player.message(LSTR_DETAIL, LSTR_GENERIC, "CRS snoops do not stack, but you can stack /crssnoop with /snoop");
		return true;
	}

	virtual void action(Player& player, Chat::ClientMessage* /*message*/) const override
	{
		if ( !player.m_crsSnoopOn )
			player.message(LSTR_NOTE, LSTR_NOT_SNOOPING, command());
		else
			player.message(LSTR_NOTE, LSTR_CURRENT_SNOOP, command(), player.m_crsSnoopOn->callsign());
	}

	virtual void actionWith(Player& player, Chat::ClientMessage* message, char* arguments) const override
	{
		Player* target = player.m_crsSnoopOn;
		if ( strcasecmp(arguments, "off") == 0 )
		{
			if ( !target )
				action(player, message);
			else
			{
				PREP_CHAT_MSG(cmsg, "%s is no-longer crssnooping on %s", player.callsign(), target->callsign());
				chatDispatchCRSMessage("INFO", cmsg);
				player.m_crsSnoopOn = nullptr;
				target->m_crsSnoopBy = nullptr;
			}

			return;
		}

		ARGV argv;

		if ( vectorize(arguments, argv, 2) > 1 )
		{
			help(player);
			return;
		}

		if ( target )
		{
			player.message(LSTR_CAUTION, LSTR_USE_SNOOP_OFF, command(), target->callsign());
			return;
		}

		target = PlayerInterface::Lookup(argv[0]);
		if ( target == nullptr )
		{
			player.message(LSTR_CAUTION, LSTR_USER_UNAVAILABLE, argv[0]);
			return;
		}
		if ( target->matchHandle() == player.matchHandle() )
		{
			player.message(LSTR_CAUTION, LSTR_CANT_VERB_SELF, command());
			return;
		}

		// time to let people know about this attempt
		player.gmNote("CRS", command(), arguments, nullptr);

		///LOCALIZE
		PREP_CHAT_MSG(cmsg, "%s crsSnooping on %s", player.callsign(), target->callsign());
		chatDispatchCRSMessage("INFO", cmsg);

		chatSendChatLog(target->playerID(), player.matchHandle());

		player.m_crsSnoopOn = target;
		target->m_crsSnoopBy = &player;
	}
} dotCRSSnoop;

//////////////////////////////////////////////////////////////////////
// Fetch a player to your current location.
//////////////////////////////////////////////////////////////////////

//! @internal
class DotSummon : public DotCommand
{
public:
	DotSummon() : DotCommand("summon", "Fetch a player to your current location"
							, NO_MASK, LSTR_GENERIC, LSTR_GENERIC)
							{}

	virtual bool help(Player& player) const override
	{
		if ( !allow(player) ) return false;
		player.message(LSTR_USAGE, LSTR_GENERIC, "'/summon <player>'  or  '/summon <player> <reason>'");
		player.message(LSTR_DETAIL, LSTR_GENERIC, "Transports a player to your current location.");
		return true;
	}

	virtual void actionWith(Player& player, Chat::ClientMessage* /*message*/, char* arguments) const override
	{
		ARGV args;
		if ( vectorize(arguments, args, 2) == 0 )
		{
			// Don't know what you're talking about. Go away.
			help(player);
			return;
		}

		if ( player.isSpawned() == false )
			throw Chat::LStringErr(LSTR_ONLY_WHILE_SPAWNED);

		Player* target = PlayerInterface::Lookup(args[0]);
		if ( target == nullptr )
			throw Chat::LStringErr(LSTR_USER_UNAVAILABLE, args[0]);

		if ( target->matchHandle() == player.matchHandle() )
			throw Chat::LStringErr(LSTR_CANT_VERB_SELF, command());

		if ( target->isSpawned() == false )
			throw Chat::LStringErr(LSTR_PLAYER_NOT_SPAWNED_NOW, target->callsign());

		Chat::PlayerMove outgoing;
		outgoing.m_playerID = target->playerID();
		outgoing.m_x = (SINT32) player.lastX();
		outgoing.m_y = (SINT32) player.lastY();
		outgoing.m_z = (SINT32) (player.lastZ() + 0.25);

		if ( Chat::SendPlayerMove(target->getCellHost(), outgoing) == false )
			throw Chat::LStringErr(LSTR_GENERIC, "summon failed: internal error");

		if ( args.size() > 1 )
		{
			player.gmNote("GM", command(), target->callsign(), args[1]);
			target->message(LSTR_WARNING, LSTR_GM_MOVED_YOU, args[1]);
		}
	}
} dotSummon;


//////////////////////////////////////////////////////////////////////
// Broadcast a system message
//////////////////////////////////////////////////////////////////////

//! @internal
class DotSys : public DotCommand
{
public:
	DotSys() : DotCommand("sys", "Anonymous SYSTEM message to all players"
							, NO_MASK, LSTR_SYS_USAGE, LSTR_SYS_DETAIL
							) { }
	virtual void actionWith(Player& player, Chat::ClientMessage* /*message*/, char* arguments) const override
	{
		player.gmNote("GM", command(), arguments, nullptr);
		chatSystemMessage(LSTR_GENERIC, arguments);
	}
} dotSys;

//////////////////////////////////////////////////////////////////////
// Release a grab target
//////////////////////////////////////////////////////////////////////

//! @internal
class DotUnGrab : public DotCommand
{
public:
	DotUnGrab() : DotCommand("ungrab", "Release a previously /grab'd player"
								, NO_MASK, LSTR_UNGRAB_USAGE, LSTR_UNGRAB_DETAIL
								) { }
	virtual void action(Player& player, Chat::ClientMessage* /*message*/) const override
	{
		// GMs can remove themeselves from another person's grab
		if ( player.takenBy() )
		{
			dbugLog(_FLI, "User %s removing self from other's grab", player.callsign());
			chatReleaseGrab(player);
			return;
		}

		if ( !player.grabbing() )
		{
			player.message(LSTR_SYSTEM, LSTR_NOBODY_GRABBED, nullptr, nullptr);
			return;
		}

		chatReleaseGrab(player);
	}
} dotUngrab;

//////////////////////////////////////////////////////////////////////
// Add a note to a player's record

//! @internal
class PlayerNote : public DBThreadQuery
{
public:
	PlayerNote(const ChatHandle& handle, UINT32 byPlayerID, const std::string& note, const char* orig)
		: DBThreadQuery(DBThread::Default, "PlayerNote", DBThreadQuery::_CALLBACK
							, "CALL `wwii`.`p_add_player_note`('%s', %u, 1, '%s')", Chat::Name(handle).c_str(), byPlayerID, note.c_str())
								, m_handle(handle), m_byPlayerID(byPlayerID), m_text(orig), m_err("")
		{
			Chat::Name name(m_handle);
			m_err = name.c_str();
		}

protected:
	ChatHandle	m_handle;
	playerid_t	m_byPlayerID;
	std::string	m_text;
	std::string m_err;

protected:
	virtual bool Process(const size_t numRows) override
	{
		if ( numRows <= 0 || Query()->FetchRow() == false )
		{
			m_err = "No notes found.";
			return false;
		}

		DBRow& row = Query()->Row();
		if ( atoi(row[0]) == 0 )
		{
			m_err = row[1];
			return false;
		}

		return true;
	}

public:
	virtual void Success() override
	{
		Player* const player = PlayerInterface::Lookup(m_byPlayerID);
		if ( player == nullptr )
			return;

		///LOCALIZE
		PREP_CHAT_MSG(cmsg, "%s noted %s: %s", player->callsign(), Chat::Name(m_handle).c_str(), m_text.c_str());
		chatDispatchGMMessage("INFO", cmsg);
	}

	virtual void Failure() override
	{
		Player* const player = PlayerInterface::Lookup(m_byPlayerID);
		if ( player != nullptr )
			player->message(LSTR_CAUTION, LSTR_DB_QUERY_FAILED, "pnote", m_err.c_str());
	}
};

//! @internal
class PlayerNoteQuery : public DBThreadQuery
{
public:
	PlayerNoteQuery(const ChatHandle& handle, UINT32 byPlayerID)
		: DBThreadQuery(DBThread::Default, "PlayerNoteQuery", DBThreadQuery::_CALLBACK
						, "SELECT	n.description, n.note_at, n.note_end, n.callsign, n.note"
			 				" FROM wwiionline.wwii_player p"
								" LEFT JOIN wwii.v_player_notes n USING (playerid)"
			 				" WHERE p.callsign = '%s'"
			 				" ORDER BY n.note_at DESC"
			 				" LIMIT 10"
							, Chat::Name(handle).c_str())
		, m_handle(handle), m_byPlayerID(byPlayerID)
		, m_results()
		{
			m_results.clear();
			m_results.reserve(6);
		}

protected:
	ChatHandle	m_handle;
	playerid_t	m_byPlayerID;
	std::vector<std::string> m_results;

protected:
	virtual bool Process(const size_t numRows) override
	{
		if ( numRows <= 0 )
			return true;

		Chat::Name callsign(m_handle);

		std::string line;
		while ( Query()->FetchRow() )
		{
			DBRow& row = Query()->Row();

			const char* const desc = row[0];
			if ( desc == nullptr || *desc == 0 )
				continue;

			const char* const at = row[1];
			const char* const end = row[2];
			const char* const by = row[3];
			const char* const note = row[4];

			line = callsign.c_str();
			line += ':';
			line += by;
			line += ':';
			line += desc;
			line += ':';
			line += at;
			if ( end != nullptr && *end )
			{
				line += "->";
				line += end;
			}
			line += ':';
			line += note;

			m_results.insert(m_results.begin(), line);

			if ( m_results.size() >= 6 )
				break;
		}

		return true;
	}

public:
	virtual void Success() override
	{
		Player* player = PlayerInterface::Lookup(m_byPlayerID);
		if ( player == nullptr )
			return;

		const size_t numRows = m_results.size();
		if ( numRows <= 0 )
		{
			player->message(LSTR_NOTE, LSTR_NO_PNOTES_FOUND, Chat::Name(m_handle).c_str());
			return;
		}

		for ( size_t i = 0; i < numRows; ++i )
		{
			player->message(LSTR_NOTE, LSTR_GENERIC, m_results[i].c_str());
		}
	}

	virtual void Failure() override
	{
		Player* player = PlayerInterface::Lookup(m_byPlayerID);
		if ( player != nullptr )
			player->message(LSTR_NOTE, LSTR_INVALID_CALLSIGN, Chat::Name(m_handle).c_str());

	}
};

//! @internal
class DotPNote : public DotCommand
{
public:
	DotPNote() : DotCommand("pnote", "Add a note to a player's record or view previous notes/messages"
							, GAME_MANAGER|GAME_TRAINER, LSTR_PNOTE_USAGE, LSTR_PNOTE_DETAIL)
							{}

	virtual void actionWith(Player& player, Chat::ClientMessage* /*message*/, char* arguments) const override
	{
		ARGV args;
		if ( vectorize(arguments, args, 2) < 1 )
		{
			help(player);
			return;
		}

		ChatHandle handle = chatGetLowercaseHandle(args[0]);
		if ( handle == 0ULL || strlen(args[0]) > (size_t)MAX_HANDLE_LEN )
		{
			player.message(LSTR_CAUTION, LSTR_INVALID_CALLSIGN, args[0]);
			return;
		}

		if ( args.size() > 1 )
		{
			std::string note = args[1];
                        DBThread::EscapeString(note);

			if ( note.length() > 399 )
			{
				player.message(LSTR_CAUTION, LSTR_PNOTE_TOO_LONG);
				return;
			}

			(new PlayerNote(handle, player.playerID(), note, args[1]))->Queue();

			return;
		}

		(new PlayerNoteQuery(handle, player.playerID()))->Queue();
	}
} dotPNote;

//////////////////////////////////////////////////////////////////////
// Send a player a warning
//////////////////////////////////////////////////////////////////////

//! @internal
class DotWarn : public DotCommand
{
public:
	DotWarn() : DotCommand("warn", "Send anonymous WARNING message to a player"
							, GAME_MANAGER|GAME_TRAINER, LSTR_WARN_USAGE, LSTR_WARN_DETAIL
							) { }
	virtual void actionWith(Player& player, Chat::ClientMessage* /*message*/, char* arguments) const override
	{
		ARGV args;
		if ( vectorize(arguments, args, 2)  < 2 )
		{
			help(player);
			return;
		}

		Player* target = PlayerInterface::Lookup(args[0]);
		if ( target == nullptr )
		{
			player.message(LSTR_CAUTION, LSTR_USER_UNAVAILABLE, args[0], nullptr);
			return;
		}

		player.gmNote("GM", command(), args[0], args[1]);

		///LOCALIZE
		PREP_CHAT_MSG(cmsg, "%s warning %s: %s", player.callsign(), target->callsign(), args[1]);
		chatDispatchGMMessage("INFO", cmsg);

		target->notifyOtherIfAFK(player);

		player.message(LSTR_WARNING, LSTR_GENERIC, args[1], nullptr);
		if ( target->matchHandle() != player.matchHandle() )
			target->message(LSTR_WARNING, LSTR_GENERIC, args[1], nullptr);

		// Add the relevant note
		std::string note = args[1];
                DBThread::EscapeString(note);

		if ( note.length() > 400 )
			note.resize(400);
		DBThread::Do(DBThread::Default, "CALL wwii.p_add_player_note('%s', %u, 3, '%s')", target->callsign(), player.playerID(), note.c_str());
	}
} dotWarn;


//////////////////////////////////////////////////////////////////////
// Broadcast a warning to ALL players
//////////////////////////////////////////////////////////////////////

//! @internal
class DotWarnAll : public DotCommand
{
public:
	DotWarnAll() : DotCommand("warnall", "Send anonymous WARNING message to ALL players"
								, GAME_MANAGER|GAME_TRAINER, LSTR_WARNALL_USAGE, LSTR_WARNALL_DETAIL
								, true
								) { }
	virtual void actionWith(Player& player, Chat::ClientMessage* /*message*/, char* arguments) const override
	{
		player.gmNote("GM", command(), arguments, nullptr);

		chatWarnAll(LSTR_WARN_ALL_PLAYERS, arguments);
	}
} dotWarnAll;
