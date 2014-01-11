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

#include "chat/chatBroadcastObject.h"
#include "chat/chatDB.h"
#include "chat/chatDotCommand.h"
#include "chat/chatDotCommandIndex.h"
#include "chat/chatFriendList.h"
#include "chat/chatHost.h"
#include "chat/chatHostMessages.h"
#include "chat/chatList.h"
#include "chat/chatLocalizedMessage.h"
#include "chat/chatLocFile.h"
#include "chat/chatLStringErr.h"
#include "chat/chatMissions.h"
#include "chat/chatMissionTypes.h"
#include "chat/chatPlayer.h"
#include "chat/chatSaveChannels.h"
#include "chat/chatSay.h"
#include "chat/chatTimeInterval.h"
#include "chat/chatTimeString.h"
#include "chat/chatTrainers.h"
#include "chat/chatUnitMotds.h"
#include "chat/chatWhoList.h"
#include "chat/chatWhos.h"
#include "miss/missionConstants.h"
#include "teul/teulPacket.h"
#include "updt/updtFraming.h"
#include "wwii/wwiiConfig.h"
#include "wwii/wwiiIRC.h"
#include "wwii/wwiiRank.h"
#include "wwii/wwiiString.h"

//////////////////////////////////////////////////////////////////////
// Private variables

#if defined(BETA_RELEASE)
 static const int REPORT_TIME_LIMIT_M = 0;
// static const UINT32 OVERRUN_LOCK_TIME_M = 0;
#elif defined(PUBLIC_RELEASE)
 static const int REPORT_TIME_LIMIT_M = 2;
// static const UINT32 OVERRUN_LOCK_TIME_M = 45;
#else
 static const int REPORT_TIME_LIMIT_M = 5;
// static const UINT32 OVERRUN_LOCK_TIME_M = 5;
#endif

// static const UINT32 ORDERS_LOCK_TIME_M = 1;

static const UCHAR PRIVATE_CHAN_ICON = 15;


//////////////////////////////////////////////////////////////////////
//// Private helper functions
//////////////////////////////////////////////////////////////////////

static bool
_leaderFindMember(Player& player, const char* command, char* arguments, ChatMission*& mission, const Player*& subject)
{
	subject = nullptr;

	if ( player.isMissionLeader() )
		mission = chatFindMission(player.missionID());
	else
		mission = nullptr;

	if ( mission == nullptr )
	{
		player.message(LSTR_CAUTION, LSTR_NOT_MISSION_LEADER);
		return false;
	}

	ChatHandle member = chatGetLowercaseHandle(arguments);
	if ( member == 0LL )
	{
		player.message(LSTR_CAUTION, LSTR_INVALID_CALLSIGN, arguments);
		return false;
	}

	if ( member == player.matchHandle() )
	{
		player.message(LSTR_CAUTION, LSTR_CANT_VERB_SELF, command);
		return false;
	}

	subject = PlayerInterface::Lookup(member);
	if ( subject == nullptr || !subject->isVisibleTo(player) || subject->missionID() != player.missionID() )
	{
		player.message(LSTR_CAUTION, LSTR_NOT_ON_SAME_MISSION, arguments);
		return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//// Dot Command Definitions
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// Command to toggle afk (away from keyboard) status
//////////////////////////////////////////////////////////////////////

//! @internal
class DotAFK : public DotCommand
{
public:
	DotAFK() : DotCommand("afk", "Toggle Away From Keyboard Flag"
							, NO_MASK, LSTR_AFK_USAGE, LSTR_AFK_DETAIL
							) { }

	virtual void actionWith(Player& player, Chat::ClientMessage* message, char* arguments) const OVERRIDE_FN
	{
		// When specifying a message, you always turn afk* on*
		player.SetAFK(true, arguments);
		if ( strcasecmp(arguments, "bio") == 0 )
			player.message(LSTR_NOTE, LSTR_GENERIC, "Uhh, good luck with that.");
		else
			player.message(LSTR_NOTE, LSTR_AFK, nullptr, nullptr);
	}

	virtual void action(Player& player, Chat::ClientMessage* message) const OVERRIDE_FN
	{
		// Otherwise they're toggling
		if ( player.isAFK() )
		{	// Toggle from ON to OFF
			player.SetAFK(false);
			player.message(LSTR_NOTE, LSTR_NOT_AFK, nullptr, nullptr);
		}
		else
		{	// Toggle from OFF to ON
			player.SetAFK(true);
			player.message(LSTR_NOTE, LSTR_AFK, nullptr, nullptr);
		}
	}
} dotAfk;

#ifdef DOT_ALARM
//////////////////////////////////////////////////////////////////////
// .alarm requests an alarm message after a given number of seconds
//////////////////////////////////////////////////////////////////////

//! @internal
class DotAlarm : public DotCommand
{
public:
	DotAlarm() : DotCommand("alarm", "Request timed notice"
							, IS_PLAYNET, LSTR_NONE, LSTR_NONE
							) { }
	virtual void actionWith(Player& player, Chat::ClientMessage* message, char* arguments) const OVERRIDE_FN;
}

void
DotAlarm::actionWith(Player& player, Chat::ClientMessage* message, char* arguments) const OVERRIDE_FN
{
	static ARGV args;
	vectorize(arguments, args, 3);
	const char* usage = "Usage: /alarm <number> [<Seconds|Minutes|Hours>";
	char unit = 's';

	if ( args.size() < 1 )
	{
		help(player);
		return;
	}

	UINT32 seconds = atol(args[0]);
	if ( seconds == 0 )
	{
		player.message(LSTR_CAUTION, LSTR_LABELLED_GENERIC, "Invalid number of seconds", argv[0]);
		return;
	}

	if ( args.size() > 1 )
		unit = tolower(args[1][0]);

	switch ( unit )
	{
		case 's': break;
		case 'm': seconds *= 60; break;
		case 'h': seconds *= 3600; break;
		default:
			throw Chat::LStringErr("Please specify one of Seconds, Minutes or Hours (abbreviations s, m or h)");
	}

	if ( seconds < 10 )
		throw Chat::LStringErr("Minimum of 10 seconds for an alarm");

	if ( seconds > 72 * 3600 )
		throw Chat::LStringErr("Maximum of 72 hours for an alarm");

	if ( player.hasAlarm() )
		player.message(LSTR_NOTE, LSTR_GENERIC, "Cancelling previous alarm");

	player.addAlarm(seconds);
}
#endif

//////////////////////////////////////////////////////////////////////
// /backupleader command
//////////////////////////////////////////////////////////////////////

//! @internal
class DotBackupLeader : public DotCommand
{
public:
	DotBackupLeader() : DotCommand("backupleader", "Set backup leader"
									, MISSION_LEADER, LSTR_BACKUPLEADER_USAGE, LSTR_BACKUPLEADER_DETAIL
									) {}

	virtual void actionWith(Player& player, Chat::ClientMessage* message, char* arguments) const OVERRIDE_FN
	{
		// Make sure no spaces
		if ( strchr(arguments, ' ') != nullptr )
		{
			help(player);
			return;
		}

		ChatMission* mission = nullptr;
		const Player* subject = nullptr;

		if ( !_leaderFindMember(player, command(), arguments, mission, subject) )
			return;

		if ( subject->rejectLead() )
		{
			player.message(LSTR_NOTE, LSTR_NOT_ACCEPTING, subject->callsign());
			return;
		}

		mission->lockLeader();

		mission->setBackupLeader(subject->matchHandle());
		player.message(LSTR_NOTE, LSTR_BACKUP_LEADER_SET, subject->callsign());
		chatSendBackupLeaderChange(mission->missionID(), subject->playerID());

	}
} dotBackupLeader;

//////////////////////////////////////////////////////////////////////
// .friends command
//////////////////////////////////////////////////////////////////////

class DotFriends : public DotCommand
{
public:
	DotFriends() : DotCommand("friends", "View/manipulate 'friends' list"
							  , NO_MASK, LSTR_FRIENDS_USAGE, LSTR_FRIENDS_DETAIL
							  )
	{
	}

public:
	virtual void action(Player& player, Chat::ClientMessage * /*message*/) const { chatFriendsList(player); }

	virtual void actionWith(Player& player, Chat::ClientMessage * /*message*/, char* arguments) const OVERRIDE_FN
	{
		static ARGV args;

		vectorize(arguments, args, 11);

		// Request for assistance
		if (args.size() < 1 || strcasecmp(args[0], "help") == 0)
		{
			help(player);
			return;
		}

		chatModifySocialList(player, args, false);
	}
} dotFriends;

//////////////////////////////////////////////////////////////////////
// The .help command; takes a DotCommandIndex as a parameter so that it
// can be used on different indexes
//////////////////////////////////////////////////////////////////////

//! @internal
class DotHelp : public DotCommand
{
public:
	DotHelp() :
		DotCommand("help", "Display commands or command-specific help", NO_MASK, LSTR_HELP_USAGE, LSTR_HELP_DETAIL)
		{ }

	// When a user requests a list of known commands
	virtual void action(Player& player, Chat::ClientMessage*) const OVERRIDE_FN
	{
		LSTRING_ID lstrID = LSTR_TEXT_START;
		std::string reply;
		UINT32 i;

		for ( i = 0; i < index()->size(); i++ )
		{
			const DotCommand* cur = index()->operator[](i);
			// Limit ourselves to the variants of commands that
			// this user has access to
			const DotUsage* usage = cur->allow(player);
			if ( usage != nullptr && usage->usage != LSTR_CAUTION )
			{
				if ( reply.length() )	// Add , between entries
				{
					if ( reply.length() + cur->m_command.length() + 2 >= MAX_MESSAGE_LENGTH )
					{
						player.message(LSTR_NONE, lstrID, reply.c_str());
						lstrID = LSTR_TEXT;
						reply = ", ";
					}
					else
						reply += ", ";
				}
				reply += cur->command();
			}
		}
		player.message(LSTR_NONE, LSTR_TEXT_END, reply.c_str(), nullptr);
	}

	// When a user requests help with a specific command
	virtual void actionWith(Player& player, Chat::ClientMessage*, char* arguments) const OVERRIDE_FN
	{
		while ( *arguments == '.' || * arguments == '/' )
			arguments++;

		if ( !index()->helpFor(player, arguments) )
		{
			// They typed .help <something unintelligible>
			player.message(LSTR_ERROR, LSTR_NO_SUCH_COMMAND, arguments, nullptr);
		}
	}
} dotHelp;

//////////////////////////////////////////////////////////////////////
// Turn HQ messages on or off
//////////////////////////////////////////////////////////////////////

//! @internal
class DotHQOff : public DotCommand
{
public:
	DotHQOff() : DotCommand("hqoff", "Detune the HQ channel"
								, NO_MASK, LSTR_HQOFF_USAGE, LSTR_HQOFF_DETAIL
								) { }
	virtual void action(Player& player, Chat::ClientMessage* /*message*/) const OVERRIDE_FN
	{
		player.SetHqOff(true);
		player.message(LSTR_NOTE, LSTR_HQ_UNSUBS, nullptr, nullptr);
	}
} dotHQOff;

class DotHQOn : public DotCommand
{
public:
	DotHQOn() : DotCommand("hqon", "Retune the HQ channel"
								, NO_MASK, LSTR_HQON_USAGE, LSTR_HQON_DETAIL
								) { }
	virtual void action(Player& player, Chat::ClientMessage* /*message*/) const OVERRIDE_FN
	{
		player.SetHqOff(false);
		player.message(LSTR_NOTE, LSTR_HQ_SUBS, nullptr, nullptr);
	}
} dotHQOn;

//////////////////////////////////////////////////////////////////////
// Ignore commands
//////////////////////////////////////////////////////////////////////

class DotIgnore : public DotCommand
{
public:
	DotIgnore() : DotCommand("ignore", "Manage/view ignore list"
							 , NO_MASK, LSTR_IGNORE_USAGE, LSTR_IGNORE_DETAIL
							 )
	{
	}

	virtual void action(Player& player, Chat::ClientMessage * /*message*/) const OVERRIDE_FN
	{
		static std::string textlist = "";

		const HANDLES& ignores = player.ignores();
		if (ignores.empty())
		{
			player.message(LSTR_NOTE, LSTR_NOT_IGNORING_ANYONE);
			return;
		}

		textlist.clear();

		player.message(LSTR_NOTE, LSTR_PLAYERS_IGNORED);
		for ( auto it = ignores.begin(), end = ignores.end(); it != end; ++it )
		{
			if (!textlist.empty())
				textlist += ", ";
			textlist += Chat::Name(*it).c_str();
		}
		player.message(LSTR_NONE, LSTR_GENERIC, textlist.c_str());
	}

	virtual void actionWith(Player& player, Chat::ClientMessage * /*message*/, char* arguments) const OVERRIDE_FN
	{
		static ARGV args;

		vectorize(arguments, args, 11);

		// Request for assistance
		if (args.size() < 1 || strcasecmp(args[0], "help") == 0)
		{
			help(player);
			return;
		}

		chatModifySocialList(player, args, true);
	}
} dotIgnore;

//////////////////////////////////////////////////////////////////////
// Change the mission leader

//! @internal
class DotMakeLeader : public DotCommand
{
public:
	DotMakeLeader() : DotCommand("makeleader", "Appoint a new mission leader"
									, NO_MASK, LSTR_MAKELEADER_USAGE, LSTR_MAKELEADER_DETAIL
									) {}

	virtual void actionWith(Player& player, Chat::ClientMessage* message, char* arguments) const OVERRIDE_FN
	{
		// Make sure no spaces
		if ( strchr(arguments, ' ') != nullptr )
		{
			help(player);
			return;
		}

		ChatMission* mission = nullptr;
		const Player* subject = nullptr;

		if ( !_leaderFindMember(player, command(), arguments, mission, subject) )
			return;

		// Ok, you're nominally approved for this; you've given up your displacement.
		if ( mission->m_displacedLeader == player.matchHandle() )
			mission->m_leaderDisplaced = 0;

		mission->checkDisplacedLeader();

		chatSendLeaderChange(player.missionID(), subject->playerID(), subject->chatHandle(), player.matchHandle());
	}
} dotMakeLeader;

//////////////////////////////////////////////////////////////////////
// Send a private message from one player to another
//////////////////////////////////////////////////////////////////////

//! @internal
class DotMsg : public DotCommand
{
public:
	DotMsg() : DotCommand("m", "Send a direct private message to a player"
							, NO_MASK, LSTR_M_USAGE, LSTR_M_DETAIL
							) { }
	virtual void actionWith(Player& player, Chat::ClientMessage* message, char* arguments) const OVERRIDE_FN
	{
		static ARGV args;

		// Get the arguments
		if ( vectorize(arguments, args, 2) < 2 )
		{
			help(player);
			return;
		}

		//Find the target
		Player* dest = PlayerInterface::Lookup(args[0]);

		// Trying to send to a non-existant player or CRS staff in GM mode gets not available message
		if ( dest == nullptr || !dest->isVisibleTo(player) )
		{
			//Notify the user that the person is unavailable for messaging
			player.message(LSTR_CAUTION, LSTR_USER_UNAVAILABLE, args[0], nullptr);

			// If the destination isn't a GM in hide mode, return now,
			// otherwise pass the message thru
			if ( dest == nullptr || !dest->gmHide() )
				return;
		}
		else
		{
			// You're here, so we tell the sender that we're sending your message.
			dest->notifyOtherIfAFK(player);

			// Show me how its going to look to him
			if ( player.isHiddenFrom(dest) == false )
			{
				try
				{
					Chat::HostMessage& out = chatBuildHostMessage(player.chatHandle(), message->m_id, PRIVATE_CHAN_ICON, args[1]);
					chatSendToRecipient(&player, &player, out);
				}
				catch (std::exception &e)
				{
					dbugLog(_FLW, "%s: %s", __FUNCTION__, e.what());
				}
			}
		}

		if ( dest->playerID() == player.playerID() )
			return;

		if ( player.isHiddenFrom(dest) )
		{
			player.gmNote("GM", "/gmm", dest->callsign(), args[1]);
			chatDispatchGMMessage("GMM", chatMakeTempString("%s->%s %s", player.callsign(), dest->callsign(), args[1]));
			dest->message(LSTR_GM, LSTR_GENERIC, args[1], nullptr);
		}
		else
		{
			try
			{
				Chat::HostMessage& out =  chatBuildHostMessage(player.chatHandle(), message->m_id, PRIVATE_CHAN_ICON, args[1]);
				chatSendToRecipient(&player, dest, out);
			}
			catch (std::exception &e)
			{
				dbugLog(_FLW, "%s: %s", __FUNCTION__, e.what());
			}
		}
	}
} dotMsg;

//////////////////////////////////////////////////////////////////////
// .1, .2, .3, .4 etc, which sends a message to the channel tuned in
// a particular slot on the player
//////////////////////////////////////////////////////////////////////

//! @internal
class DotN : public DotCommand
{
private:
	UINT32	_radioNo;
public:
	DotN(const std::string& which) :
		DotCommand(which, "Send a message to a specific radio", NO_MASK, LSTR_N_USAGE, LSTR_N_DETAIL)
		, _radioNo(atoi(which.c_str()) - 1)
		{ }
	virtual void actionWith(Player& player, Chat::ClientMessage* message, char* arguments) const OVERRIDE_FN
	{
		const Chat::ChannelSubscription& cnode = player.getChannelSub(_radioNo);
		// Do they have a radio tuned in this slot?
		if ( !cnode.inUse() )
		{
			player.message(LSTR_SYSTEM, LSTR_CHANNEL_UNAVAILABLE, nullptr, nullptr);
			return;
		}

		if ( cnode.isPlaceHolder() )
		{
			chatReportPlaceHolder(&player, cnode.type());
			return;
		}

		// Prepare the new message
		Chat::ClientMessage send;
		memcpy(&send, message, sizeof(send));
		strlcpy(send.m_message, arguments, sizeof(send.m_message));
		send.m_radios = 1 << _radioNo;

		// And send it
		chatRouteMessage(&player, &send, send.m_id, false);
	}
} dot1("1"), dot2("2"), dot3("3"), dot4("4"), dot5("5"), dot6("6");

//////////////////////////////////////////////////////////////////////
// Hide yourself from "who" queries
//////////////////////////////////////////////////////////////////////

//! @internal
class DotNoEChat : public DotCommand
{
public:
	DotNoEChat() : DotCommand("noechat", "Toggle receipt of enemy area chat on/off"
								, NO_MASK, LSTR_NOECHAT_USAGE, LSTR_NOECHAT_DETAIL
								) { };
	virtual void action(Player& player, Chat::ClientMessage* /*message*/) const OVERRIDE_FN
	{
		if ( player.noEnemyChat() )
		{
			player.SetNoEnemyChat(false);
			player.message(LSTR_NOTE, LSTR_NOECHAT_OFF);
			return;
		}

		player.SetNoEnemyChat(true);
		player.message(LSTR_NOTE, LSTR_NOECHAT_ON);
	}
} dotNoEChat;

//////////////////////////////////////////////////////////////////////
// Hide yourself from "who" queries
//////////////////////////////////////////////////////////////////////

//! @internal
class DotNoWho : public DotCommand
{
public:
	DotNoWho() : DotCommand("nowho", "Allows you to hide from .who"
								, NO_MASK, LSTR_NOWHO_USAGE, LSTR_NOWHO_DETAIL
								) { };

	virtual void action(Player& player, Chat::ClientMessage* /*message*/) const OVERRIDE_FN
	{
		if ( player.noWho() )
		{
			player.SetNoWho(false);
			player.message(LSTR_NOTE, LSTR_NOWHO_OFF);
			return;
		}

		player.SetNoWho(true);
		player.message(LSTR_NOTE, LSTR_NOWHO_ON);
	}
} dotNoWho;

//////////////////////////////////////////////////////////////////////
// Mission orders command
//////////////////////////////////////////////////////////////////////

//! @internal
class DotOrders : public DotCommand
{
public:
	DotOrders() : DotCommand("orders", "Set/view current mission orders"
							, NO_MASK, LSTR_ORDERS_USAGE, LSTR_ORDERS_DETAIL
							) { };

	virtual void action(Player& player, Chat::ClientMessage* message) const  OVERRIDE_FN
	{
		if ( !player.missionID() )
			throw Chat::LStringErr(LSTR_NOT_ON_MISSION);
		chatMissionOrders(&player);
	}
	virtual void actionWith(Player& player, Chat::ClientMessage* message, SCHAR* arguments) const  OVERRIDE_FN
	{
		if ( !player.missionID() )
			throw Chat::LStringErr(LSTR_NOT_ON_MISSION);

		if ( !player.isMissionLeader() )
			throw Chat::LStringErr(LSTR_NOT_MISSION_LEADER);

		if ( strcasecmp(arguments, "clear") == 0 )
		{
			chatSetMissionOrders(player.missionID(), "", true);
			player.message(LSTR_NOTE, LSTR_ORDERS_CLEARED);
			return;
		}

#if defined(BETA_RELEASE)
		if ( strcasecmp(arguments, "private") == 0 )
		{
			Teul::Packet outgoing(sizeof(UINT32), MISSION_X, HOST_LOCK_MISSION_Y);
			outgoing.append(player.missionID());
			serverSendPacketToType(outgoing, MAP_HOST);
			chatSetMissionOrders(player.missionID(), "Mission set to Private: No new joiners", false);
			return;
		}
#endif

		std::string orders;
		orders  = "[";
		orders += player.callsign();
		orders += "] ";
		orders += arguments;

		chatSetMissionOrders(player.missionID(), orders, true);
	}
} dotOrders;

//////////////////////////////////////////////////////////////////////
// Player report submission
//////////////////////////////////////////////////////////////////////

//! @internal
class DotReport : public DotCommand
{
public:
	DotReport() : DotCommand("report", "Submit a bug/problem/feedback report"
								, NO_MASK, LSTR_REPORT_USAGE, LSTR_REPORT_DETAIL
								) { }
	virtual void actionWith(Player& player, Chat::ClientMessage* message, SCHAR* arguments) const  OVERRIDE_FN
	{
		SCHAR info[MAX_MESSAGE_LENGTH + 1];

		if ( !player.isPlaynet() && !player.isEffectiveGM() && _featureLockedM(player, REPORT_TIME_LIMIT_M) )
			return;

		info[0] = '[';
		player.whereabouts(info+1, player.lastX(), player.lastY());
		strcat(info, "]");

		dbugLog(_FLI, "!REPORT:%s:.%s:%s %s", player.callsign(), command(), arguments, info);
		chatdbRecordEvent("RPRT", player.callsign(), command(), arguments, info);

		std::string reply(arguments);
		reply += " ";
		reply += info;

		Chat::LocalizedMessage lmsg(LSTR_REPORT, LSTR_REPORTING, player.callsign(), reply.c_str());
		const char* ircMessage = chatLString(lmsg);
		wwiiIRCChannelMessage("-sys", ircMessage);
		chatBroadcastLocMessage(commandList.begin(), commandList.end(), lmsg, [] (wwiiPlayer* const& subject) { return subject->isEffectiveGM(); });

		player.message(LSTR_NOTE, LSTR_REPORTED, nullptr, nullptr);

		return;
	}
} dotReport;

//////////////////////////////////////////////////////////////////////
// Reset your radio channels to the defaults
//////////////////////////////////////////////////////////////////////

//! @internal
class DotResetRadios : public DotCommand
{
public:
	DotResetRadios() : DotCommand("resetradios", "Reset your radio channels to the defaults"
												, NO_MASK, LSTR_RESET_RADIO_USAGE, LSTR_RESET_RADIO_DETAIL
												) { }
	// Player must type "/resetradios +mycallsign"
	virtual void action(Player& player, Chat::ClientMessage* /*message*/) const OVERRIDE_FN
	{
		if ( _featureLockedM(player, 3) )
			return;

		// Force reset of channels.
		player.tuneInitialChannels(true);

		// Save them to the database.
		chatSaveChannels(player);

		// Tell them it's done.
		player.message(LSTR_NOTE, LSTR_RESET_RADIO_DONE, player.callsign());
	}
} dotResetRadios;

//////////////////////////////////////////////////////////////////////
// Display the sysmotd
//////////////////////////////////////////////////////////////////////

//! @internal
class DotSysMotd : public DotCommand
{
public:
	DotSysMotd() : DotCommand("sysmotd", "View the system message of the day"
								, NO_MASK, LSTR_SYSMOTD_USAGE, LSTR_SYSMOTD_DETAIL
								) { }
	// Player typed ".sysmotd" - display the sysmotd
	virtual void action(Player& player, Chat::ClientMessage* /*message*/) const OVERRIDE_FN
	{
		player.message(LSTR_SYSTEM, LSTR_GENERIC, chatGetUnitMotd(SYSTEM_MOTD_UNIT), nullptr);
	}
} dotSysMotd;

//////////////////////////////////////////////////////////////////////
// Display (or set) the time
//////////////////////////////////////////////////////////////////////

//! @internal
class DotTime : public DotCommand
{
public:
	DotTime() : DotCommand("time", "Shows real-world server time"
							, NO_MASK, LSTR_TIME_USAGE, LSTR_TIME_DETAIL
							) { }

	virtual void action(Player& player, Chat::ClientMessage* /*message*/) const OVERRIDE_FN
	{
		player.message(LSTR_NOTE, LSTR_SERVER_TIME, chatTimeString(teulTime()));
	}
} dotTime;

//////////////////////////////////////////////////////////////////////
// List trainers in the game
//////////////////////////////////////////////////////////////////////

static void
_listTrainersForTrainer(Player& player)
{
	const TRAINERS& trainers = chatGetTrainers();
	TRAINERS::const_iterator it;

	CHAT_WHO_LIST whoList(CWT_TRAINERS);
	Chat::Who who;

	for ( it = trainers.begin(); it != trainers.end(); ++it )
	{
		Player* trainer = PlayerInterface::Lookup(*it);
		if ( trainer == nullptr )
			continue;
		if ( !trainer->isVisibleTo(player) )
			continue;
		if ( trainer->noWho() )
			continue;

		chatWhoPopulate(who, &player, trainer, true);
		whoList.add(who);
	}

	whoList.send(player.teulIndex());
}

static void
_listTrainersForPlayer(Player& player)
{
	const TRAINERS& trainers = chatGetTrainers();
	if ( trainers.empty() )
		throw Chat::LStringErr(LSTR_NO_TRAINERS);

	std::string result = "";
	TRAINERS::const_iterator it;

	for ( it = trainers.begin(); it != trainers.end(); ++it )
	{
		Player* trainer = PlayerInterface::Lookup(*it);
		if ( trainer == nullptr )
			continue;
		if ( !trainer->isVisibleTo(player) )
			continue;
		if ( trainer->noWho() )
			continue;

		trainer->addName(result, ", ");
	}

	if ( result.size() == 0 )
		throw Chat::LStringErr(LSTR_NO_TRAINERS);

	player.message(LSTR_NOTE, LSTR_TRAINER_LIST, result.c_str());
}

//! @internal
class DotTrainers : public DotCommand
{
public:
	DotTrainers() : DotCommand("trainers", "List any trainers currently available"
								, NO_MASK, LSTR_TRAINERS_USAGE, LSTR_TRAINERS_DETAIL
								) {}
	virtual void action(Player& player, Chat::ClientMessage* message) const OVERRIDE_FN
	{
		if ( player.isTrainer() )
			_listTrainersForTrainer(player);
		else
			_listTrainersForPlayer(player);
	}

	virtual void actionWith(Player& player, Chat::ClientMessage* message, char* arguments) const OVERRIDE_FN
	{
		if ( player.isTrainer() == false )
		{
			help(player);
			return;
		}

		char buffer[MAX_MESSAGE_LENGTH + 1];
		strprintf(buffer, "%s: %s", player.callsign(), arguments);

		const TRAINERS& trainers = chatGetTrainers();
		TRAINERS::const_iterator it = trainers.begin();
		while ( it != trainers.end() )
		{
			Player* recipient = PlayerInterface::Lookup(*(it++));
			if ( recipient != nullptr )
				recipient->message(LSTR_NONE, LSTR_TRAINER_LIST, buffer);
		}
	}
} dotTrainers;

//////////////////////////////////////////////////////////////////////
// The .um command
//////////////////////////////////////////////////////////////////////

//! @internal
class DotUM : public DotCommand
{
public:
	DotUM() : DotCommand("um", "Set current update mode"
							, NO_MASK, LSTR_UM_USAGE, LSTR_UM_DETAIL
							) { }

	virtual void actionWith(Player& player, Chat::ClientMessage* /*message*/, char* arguments) const OVERRIDE_FN
	{
		SINT32 mode = -1;

		if ( !player.isSpawned() )
			throw Chat::LStringErr(LSTR_ONLY_WHILE_SPAWNED);

		if ( arguments[0] != '?' )
		{
			(void)sscanf(arguments, "%u", &mode);
			if ( mode < 0 || mode >= UpdateFraming::_MODES )
				throw Chat::LStringErr(LSTR_INVALID_UM, arguments);
		}
		else
		{
			mode = 255;
		}

		chatSendBandwidthNotice(player.playerID(), (UINT32)mode);
	}

} dotUM;

//////////////////////////////////////////////////////////////////////
// Small Area chat message
//////////////////////////////////////////////////////////////////////

//! @internal
class DotWhisper : public DotCommand
{
public:
	DotWhisper() : DotCommand("whisper", "Small-area chat message"
							, NO_MASK, LSTR_WHISPER_USAGE, LSTR_WHISPER_DETAIL
							) { }

	virtual void actionWith(Player& player, Chat::ClientMessage* message, char* arguments) const OVERRIDE_FN
	{
		chatSay(&player, message, arguments, AREA_WHISPER, true);
	}
} dotWhisper;


//////////////////////////////////////////////////////////////////////
// Stupid command
//////////////////////////////////////////////////////////////////////

//! @internal
class DotWTF : public DotCommand
{
private:
	mutable std::vector<std::string>responses;
	void
	reset() const
	{
		responses.clear();
		responses.push_back("i am bed sorry");	// Can't have this often enough.
		responses.push_back("i am bed sorry");
		responses.push_back("i am bed sorry");
		responses.push_back("i go. i am bed sorry");
		responses.push_back("i am wtf sorry");
		responses.push_back("you wtf. i am wtf sorry.");
		responses.push_back("you am bed sorry.");
		responses.push_back("you am pwned sorry.");
		responses.push_back("htf should I know?");
		responses.push_back("surprize!");
		responses.push_back("n00b.");
		responses.push_back("huh?");
		responses.push_back("stuff happens.");
		responses.push_back("did you get pwned?");
		responses.push_back("omg!!!!!1!");
		responses.push_back("not in the face!");
		responses.push_back("pwned?");
		responses.push_back("pwned");
		responses.push_back("PWNED!");
		responses.push_back("teh?");
		responses.push_back("teh");
		responses.push_back("wtf?!?!?!?!!!11111!!!!!!!!!");
		responses.push_back("there is a rumor you may just have been PWNED");
		responses.push_back("dude it happens all the time");
		responses.push_back("there is no ctd, only rapid restoration of desktop functionality");
		responses.push_back(" badger badger badger badger badger badger badger badger mushroom mushroom\n badger badger badger badger badger badger badger badger mushroom mushroom\n badger badger badger badger badger badger badger badger mushroom mushroom\n snaaaaaake snaaaaake snaaaaake its a snaaaaaaake!");
		responses.push_back("t\ne\nh\n?");
		responses.push_back("knife fight!");
		responses.push_back("oh yeah?");
		responses.push_back("all your bases ar belgon to pwnozr?");
		responses.push_back("opel?");
		responses.push_back("you put 50 rounds into the commanders sandwich tin and it didn't flame up?");
		responses.push_back("what? you accidentally remapped 'start engine' to the 'any key'?");
		responses.push_back("what? what did I miss?");
		responses.push_back("nawh its not a speed hacker, your system just needs new batteries");
		responses.push_back("unh... hold on.. urgh... here it comes... URGH... C'MON DESKTOP...\n<burp>\nDAMMIT!\nThis time you get to stay in-game... *this* time...");
		responses.push_back("HOLY CReeP WOULD YOU LOOK AT THAT!");
		responses.push_back("You're not just a pretty monkey eh?");
		responses.push_back("CRS is <Insert Name Here> biased");
		responses.push_back("Bilton* is* the numbers imbalance.");
		responses.push_back("Give up while you're ahead.");
		responses.push_back("whipped");
		responses.push_back("wdwhipped");
		responses.push_back("blame rooke");
		responses.push_back("its tooky's fault");
		responses.push_back("its not purdy");
		responses.push_back("<FX> BOOM! HEADSHOT!");
		responses.push_back("Buy more ram");
		responses.push_back("You're the only smart one here.");
		responses.push_back("nerf");
		responses.push_back("Go on, tell me that never happens in camelot");
		responses.push_back("Close your eyes and think of sheep");
		responses.push_back("Head: the sweet spot on the infantry");
		responses.push_back("Crew member 1 was just hit in the angryzone?");
		responses.push_back("Display Bilton's CSR stats now? (Y)es/(N)o/(P)wned?");
		responses.push_back("ZZZzzz...");
		responses.push_back("Nyaah, it was your fault");
		responses.push_back("WTF!!!!1 LTP!");
		responses.push_back("Ahm, well, it made ME laugh");
		responses.push_back("That's supposed to happen");
		responses.push_back("So don't catch five inch shells with your teeth, duh");
		responses.push_back("Muahahahahahahahahahahahahahahaha");
		responses.push_back("<Your journey to the pwned side is compleat, my son>");
		responses.push_back("It makes baby jeebus cry =(");
#ifdef BETA_RELEASE
		responses.push_back("Jeez, don't you like have stuff to test and stuff?");
		responses.push_back("Don't pr0n & drive");
		responses.push_back("Female lady dogs of the night male organsuckers!! Dobodily fluidsment it!!!!");
#endif
		responses.push_back("PLACEHOLDER\n(for loud, angry comment by DOC not related to subject at hand)");
		responses.push_back("YOU'RE NOT DOING IT RIGHT");
		responses.push_back("YOU JUST DONT UNDRESTAND");
		responses.push_back("Historically accurate: you were pwned then, you are pwned now");
		responses.push_back("ei!");
		responses.push_back("Hmm. It must be 'new and improved'!");
		responses.push_back("In the world of (A)bort/(R)etry/(F)ail - you rank F.");
		responses.push_back("Insert paper in drive C:");
		responses.push_back("/wtf says it all");
		responses.push_back("Do you know how much memory this list of responses takes up?");
		responses.push_back("What makes you think \"wtf\"?");
		responses.push_back("Does hearing about \"wtf\" make you angry?");
		responses.push_back("Do you kiss your mother with that mouth?");
		responses.push_back("Tell me, why is it that you say \"wtf\"?");
		responses.push_back("Think happy thoughts. Airmessy with a sheep. Maybe not that happy.");
		responses.push_back("At least now you can rest in peace. Oh, no, he's shooting your corpse to see you twitch");
		responses.push_back("Remember the burrito accident.");
		responses.push_back("Physics ran on rubber-bands back in the war days...");
		responses.push_back("Feature.");
		responses.push_back("<This response used to say 'ROFLAMO' but some people were offended. But I am planning my revenge...>");
		responses.push_back("If you're wondering what this command does, this is what it does.");
		responses.push_back("Magic 8-ball says, Certainly not.");
		responses.push_back("Magic 8-ball says, Certainly.");
		responses.push_back("Magic 8-ball says, That would be a yes.");
		responses.push_back("Magic 8-ball says, Absolutely");
		responses.push_back("Magic 8-ball says, Care to bet?");
		responses.push_back("ftw!");
		responses.push_back("Probably not a good time to tell you that your dog died then?");
		responses.push_back("So not your favorite kind of surprize?");
		responses.push_back("I guess we ought to redesign that then");
		responses.push_back("AUDIT!");
		responses.push_back("Thru-the-window grenade throwing is a dying artform. Waugh!");
		responses.push_back("''Waugh!''?");
		responses.push_back("Not implemented.");
		responses.push_back("I'll fix it for a crunchie?");
		responses.push_back("oh noes - I think it says 'take the blue pill', not 'take the two pills'!");
		responses.push_back("pirate > ninja");
		responses.push_back("ninja > pirate");
		responses.push_back("Don't blame the game there, Leeroy");
		responses.push_back("So I was telling this guy, gimme some ammo. He was like wre r u? I looked around to ask him what that meant but I couldn't see him. Then some dude said 'waaaaugh'. I said, \"groug, that's not funny\". But all he did was twitch");
		responses.push_back("Le waaaaaugh?");
		responses.push_back("Grass wasn't greener on the other side of* that* berm?");
		responses.push_back("Blame Hasselt for the poor quality of this wtf. It was gonna be funny until I got shot while typing it.");
		responses.push_back("Too busy watching mercy with a machine gun?");
		responses.push_back("Parachuting dogs should be careful where they pack their bone");
		responses.push_back("Battleground Europe bedside manner: You're going to live a long and happy life, as long as nobody breaks wind... *polite cough*");
		responses.push_back("Battleground Europe train announcements: Welcome to the London Underground. Trains may be in the tunnels below.");
		responses.push_back("Could everyone just stop getting shot?");
		responses.push_back("WTF = Willy Tee Funny?");
		responses.push_back("Please wait while we bill you a second time...");
		responses.push_back("It's all greek to me");
		responses.push_back("Its all sdrawkcab, em ksa uoy fi");
		responses.push_back("Life's like a box of chocolate, Forrest. You never know when you're going to cause a CTD.");
		responses.push_back("Connection to host lost!!!!!!!1!");
		responses.push_back("Connection to beerz lost!!!!!!!eleventy!");
		responses.push_back("You think you can handle wtf\?\?\?\? *KILLER* can't handle wtf!");
		responses.push_back("''What the hell is an aluminum falcon\?\?\?''");
		responses.push_back("''You've been flying around for two weeks trying to get a signal? You must smell like old feet wrapped in leathery, burnt bacon!");
		responses.push_back("It wouldn't happen if you'd come to the 2007 Convention");
		responses.push_back("ERROR: Unable to give you a wtf due to salad balancing restrictions");
		responses.push_back("WWIIOL MiniFact: The 'waaaugh' cry of dying infantry was captured by playing one of Willy Tee's cartoons to an audience and taking the earmuffs of one of them when Wheels sang");
		responses.push_back("Server coming down in 15 minutes for a patch.");
		responses.push_back("Maybe they were aiming for KenneY?");
		responses.push_back("This space left intentionally blank. Please do not refer to it as \"bilton's brain\".");
		responses.push_back("Gameplay error detected. Please press the Any Key now to correct this issue.");
		responses.push_back("Sorry, I wasn't looking, what happened?");
		responses.push_back("Zebbeee suggested this calming response. Did it work?");
		responses.push_back("Remember to turn safety off on all weapons before attempting to engage the enemy");
		responses.push_back("Crew member 1 hit with the nerf bat");
		responses.push_back("Language Violation: You are charged with two hits of the nerf bat.");
		responses.push_back("Bloo says: Nay.");
		responses.push_back("Blame the klingons.");
		responses.push_back("Made in China.");
		responses.push_back("Unable to refund your quarter - spent it on bubble gum.");
		responses.push_back("You mis-spelled 'I was pwned and I'm very happy about it'");
		responses.push_back("That's what happens when you don't say 'please'");
		responses.push_back("LOOK IT UP ON THE INTERNET");
	}
public:
	DotWTF() : DotCommand("wtf", "Pointless test command", NO_MASK, LSTR_NONE, LSTR_NONE)
	{
		reset();
	}

	virtual void actionWith(Player& player, Chat::ClientMessage* message, char* arguments) const OVERRIDE_FN
	{
		if ( player.playerID() != 3204 )
			action(player, message);
		else
		{
			responses.push_back(arguments);
			player.message(LSTR_NONE, LSTR_GENERIC, arguments);
		}
	}

	virtual void action(Player& player, Chat::ClientMessage* /*message*/) const OVERRIDE_FN
	{
		const char* str;
		str = responses[rand() % responses.size()].c_str();
		LSTRING_ID prefix = ( (rand() & 1023) == 0 ) ? LSTR_ERROR : LSTR_NONE;
		if ( (rand() & 127) == 42 )
			player.message(prefix, LSTR_LABELLED_GENERIC, "dewd", str);
		else
			player.message(prefix, LSTR_GENERIC, str);
	}
} dotWtf;

//////////////////////////////////////////////////////////////////////
// Large Area chat message
//////////////////////////////////////////////////////////////////////

//! @internal
class DotYell : public DotCommand
{
public:
	DotYell() : DotCommand("yell", "Large-area chat message"
							, NO_MASK, LSTR_YELL_USAGE, LSTR_YELL_DETAIL
							) { }
	virtual void actionWith(Player& player, Chat::ClientMessage* message, char* arguments) const OVERRIDE_FN
	{
		chatSay(&player, message, arguments, AREA_YELL, true);
	}
} dotYell;

//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//! @internal

#include "dbug/dbugExtErrors.h"

class DotTestError : public DotCommand
{
public:
	DotTestError() : DotCommand("testerror", "Test error codes", NO_MASK, LSTR_GENERIC, LSTR_GENERIC, false) {}

	virtual bool help(Player& player) const OVERRIDE_FN
	{
		if ( !allow(player) )
			return false;
		player.message(LSTR_USAGE, LSTR_GENERIC, "/testerror [category] <errorcode>");
		player.message(LSTR_DETAIL, LSTR_GENERIC, "Sends the specified [combination of] error codes to your client. 'errorcode' is a numeric code, e.g. 20033. To test error categories, specify the category number and normal error code, e.g. '/testerror 6 101' to test SendFailCategory + error 101.");
		return true;
	}

	virtual void actionWith(Player& player, Chat::ClientMessage* message, char* arguments) const OVERRIDE_FN
	{
		ARGV args;
		vectorize(arguments, args, 3);
		if ( args.size() < 1 || args.size() > 2 )
			return (void) help(player);

		SINT32 err = -1;
		if ( args.size() == 1 )
		{
			err = atol(args[0]);
		}
		else if ( args.size() == 2 )
		{
			const SINT32 cat = atol(args[0]);
			if ( cat >= 0 )
			{
				const SINT32 code = atol(args[1]);
				if ( code >= 0 )
					err = Teul::Errors::combineCodes(cat, code);
			}
		}

		if ( err < 0 )
		{
			help(player);
			return;
		}

		char response[80];
		sprintf(response, "Sending you error code #%u", (UINT32)err);
		player.message(LSTR_NOTE, LSTR_GENERIC, response);

		teulSendError(player.teulIndex(), err);
	}
} dotTestErrors;

