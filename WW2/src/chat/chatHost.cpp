#include <syscfg.h> // Path is project specific.
// Created: Wed Jul 26 14:57:05 2000
//
// Main loop for the chat system
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

#include "hostproj/hostversion.h"

#include "mem/mem.h"
#include "chat/chatBanPlayer.h"
#include "chat/chatBroadcast.h"
#include "chat/chatBroadcastHCPacket.h"
#include "chat/chatBroadcastObject.h"
#include "chat/chatCellTypes.h"
#include "chat/chatChannelID.h"
#include "chat/chatChannelList.h"
#include "chat/chatChannelNumber.h"
#include "chat/chatConstants.h"
#include "chat/chatData.h"
#include "chat/chatdbClientMessages.h"
#include "chat/chatDB.h"
#include "chat/chatDecisions.h"
#include "chat/chatDotCommand.h"
#include "chat/chatDotCommandIndex.h"
#include "chat/chatGCIMessage.h"
#include "chat/chatGrid.h"
#include "chat/chatGroupPermissions.h"
#include "chat/chatHandle.h"
#include "chat/chatHoldFallVote.h"
#include "chat/chatHostBrigadeControl.h"
#include "chat/chatHost.h"
#include "chat/chatHostMessage.h"
#include "chat/chatHostMessages.h"
#include "chat/chatIcons.h"
#include "chat/chatIRCPlayer.h"
#include "chat/chatList.h"
#include "chat/chatLocFile.h"
#include "chat/chatManagementHostMessages.h"
#include "chat/chatManagementTypes.h"
#include "chat/chatMissions.h"
#include "chat/chatMissionTypes.h"
#include "chat/chatModReport.h"
#include "chat/chatName.h"
#include "chat/chatPlayer.h"
#include "chat/chatPlayerInfo.h"
#include "chat/chatSay.h"
#include "chat/chatServerTrackedObjects.h"
#include "chat/chatSpamCan.h"
#include "chat/chatSquadCreate.h"
#include "chat/chatSquadInvite.h"
#include "chat/chatSquadList.h"
#include "chat/chatSquadListPurge.h"
#include "chat/chatSquadListTypes.h"
#include "chat/chatSquadVote.h"
#include "chat/chatThreads.h"
#include "chat/chatTimeInterval.h"
#include "chat/chatTuneToChannel.h"
#include "chat/chatUnitMotds.h"
#include "chat/chatClientMessages.h"
#include "dbse/dbseClient.h"
#include "inst/instHostInstance.h"
#include "server/serverCodeEnum.h"
#include "server/serverFuncTypes.h"
#include "server/server.h"
#include "server/serverIndexes.h"
#include "server/serverInitializeProcess.h"
#include "server/serverInterfaceDescription.h"
#include "server/serverInterface.h"
#include "server/serverPersonaExit.h"
#include "server/serverPersonaLogin.h"
#include "server/serverPlayerStatusNode.h"
#include "server/serverReleaseTypeEnum.h"
#include "server/serverServerInfo.h"
#include "str/stringLib.h"
#include "strt/strtChatTypes.h"
#include "teul/teulEndpoint.h"
#include "teul/teulPacket.h"
#include "teul/teulServer.h"
#include "teul/teulTickSample.h"
#include "wwii/wwiiConfig.h"
#include "wwii/wwiiDBConfig.h"
#include "wwii/wwiiHCUnits.h"
#include "wwii/wwiiIRC.h"
#include "wwii/wwiiNetPorts.h"
#include "wwii/wwiiServerTrackedObject.h"
#include "wwii/wwiiServerTypeEnums.h"
#include "wwii/wwiiDBThreads.h"
#include "wwii/wwiiPPOLoader.h"

extern UINT32 serverGetAuthIP();

extern SINT32 serverBroadcastClusterShutdown();

extern void chatRegisterDotCommands();

#ifndef NEW_UI
	#define HQ_HANDLE_TUNE
#endif

#define KICK_BANS

#define CHAT_TIMEOUT 500000

#ifdef DEBUG
	#define DEF_ERR_LEVEL Debug::ERR_DBUG
    #define DEF_ERR_FLAGS STD_ERR  // LOG_ALL_TO_STDERR? look at the logs
#else
	#define DEF_ERR_LEVEL Debug::ERR_INFO
    #define DEF_ERR_FLAGS STD_ERR
#endif

#if defined(DEV_BUILD)
# define DEFAULT_SYS_MOTD		"MOTD: Internal Dev Cluster"
#elif defined(CLOSED_BETA)
# define DEFAULT_SYS_MOTD		"MOTD: WWII Online Closed Beta"
#elif defined(BETA_RELEASE)
# define DEFAULT_SYS_MOTD		"MOTD: WWII Online Beta Release"
#elif defined(TEST_RELEASE)
# define DEFAULT_SYS_MOTD		"MOTD: WWII Online BETA Release-Candidate"
#elif defined(TRAINING_SERVER)
# define DEFAULT_SYS_MOTD		"MOTD: Welcome to the Training Server"
#else
# define DEFAULT_SYS_MOTD		"MOTD: Welcome to the Virtual Battlefield"
#endif

#define DEFAULT_ALLIED_HC_MOTD	"MOTD: Welcome to the Allied High Command"
#define DEFAULT_AXIS_HC_MOTD	"MOTD: Welcome to the Axis High Command"

// Maximum number of people you can have on an ignore list
//static const UINT32 MAX_IGNORES = 42;

//static SCHAR hcMotd[MAX_SIDES][SYS_MOTD_LEN+1];

const char* const channelTypeName[] = {
	"NO_CHANNEL"
,	"BRIGADE_CHANNEL"
,	"MISSION_CHANNEL"
,	"SQUAD_CHANNEL"
,	"GCI_CHANNEL"
,	"CP_CHANNEL"
,	"NUMBERED_CHANNEL"
// Below here are custom instances of channel types
,	"PRIVATE_CHANNEL"
,	"GM_CHANNEL"
,	"ORIGIN_CP_CHANNEL"
,	"OBJECTIVE_CP_CHANNEL"
,	"SIDE_GENERAL_CHANNEL"
,	"SIDE_HELP_CHANNEL"
,	"SIDE_OPS_CHANNEL"
,	"MAX_CHANNEL_TYPES"
};

static bool _running = true;

bool g_debugRadios = false;
bool g_debugSquads = false;
bool g_debugPersonas = false;
bool g_debugChat = false;

const char* const chanTypeName[MAX_CHANNEL_TYPES] =
{
	"NONE"
,	"BRIGADE"
,	"MISSION"
,	"SQUAD"
,	"HQ"
,	"CP"
,	"NUMBERED"
,	"PRIVATE"
,	"GM"
,	"ORIGIN"
,	"OBJECTIVE"
,	"SIDE"
,	"HELP"
,	"OPS"
};

static UINT32	_stratHost = 0;
static UINT16	_stratPort = 0;
//static UINT32	_chatHost = 0;

// This is getting ugly. Repeat after me: I will rewrite the chat-shite.

// Convert commonly used callsigns into 64bit handles
ChatHandle alliedHandleID = chatGetHandle("ALLIED");
ChatHandle axisHandleID = chatGetHandle("AXIS");
ChatHandle crsHandleID = chatGetHandle("CRS");
ChatHandle adminHandleID = chatGetHandle("GM");
ChatHandle systemHandleID = chatGetHandle("SYSTEM");

#ifdef STATS_LOGGING
UINT32 _numMessages=0;
UINT32 _shortestProcessingTime=0;
UINT32 _longestProcessingTime=0;
FLOAT32 _avgProcessingTime=0;
FLOAT32 _avgMessageLength=0;
#endif

// Commonly used channels so we don't have to keep finding them
Chat::Channel* adminChannel = nullptr;

PPOSpecLoader specLoader;

UINT32 g_shutdownLockAt = 0;
UINT32 g_shutdownDowntime = 0;
static Chat::Name g_shutdownRequestedBy;
static UINT32 g_shutdownLockedArena = 0;
static UINT32 g_finalShutdownTime = 0;

// Dot command list at end of file to avoid prototyping

static bool _tuneHQ(Player& player, UCHAR index);
static bool _tuneOrigin(Player& player, UCHAR index);
static bool _tuneGM(Player& player, UCHAR index);
extern bool chatUpdateHCUnitLocations();

struct TUNE_COMMAND
{
	using Callback = bool(*)(Player& player, UCHAR index);

	ChatHandle			matchHandle_;
	Callback			callback_;
	PermissionMask		perms_;

	TUNE_COMMAND(const char* name, Callback callback, PermissionMask perms=0ULL)
		: matchHandle_(chatGetLowercaseHandle(name))
		, callback_(callback)
		, perms_(perms)
	{}
};

static std::vector<TUNE_COMMAND> _tuneCommands = {
	TUNE_COMMAND("hq", _tuneHQ),
	TUNE_COMMAND("origin", _tuneOrigin),
	TUNE_COMMAND("gm", _tuneGM, GAME_MANAGER)
};

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
// Chat Host
//////////////////////////////////////////////////////////////////////

struct CampaignFilterFn : public FilterFn
{
	bool operator()(const wwiiPlayer* player) const { return player->instance().isCampaign(); }
};

//////////////////////////////////////////////////////////////////////
// Send a message to a user connected through the IRC interface.

bool
IRCPlayer::message(int prefix, int messageID
					, const char* ptr0, const char* ptr1, const char* ptr2, const char* ptr3, const char* ptr4, const char* ptr5) const
{
	char text[MAX_MESSAGE_LENGTH+1];
	strsafecpy(text, chatLString((LSTRING_ID)prefix, (LSTRING_ID)messageID, ptr0, ptr1, ptr2, ptr3, ptr4, ptr5));

	char* p = text;
	do
	{
		char* eol = strchr(p, '\n');
		if ( eol != nullptr )
			*(eol++) = 0;
		wwiiIRCMessage(m_lastChannel.empty() ? callsign() : m_lastChannel.c_str(), "%s", p);
		p = eol;
	}
	while ( p != nullptr );

	return true;
}

//////////////////////////////////////////////////////////////////////
// Describe an IRC user to a GM.

void
IRCPlayer::describeTo(const Player& player, const FLOAT64& /*x*/, const FLOAT64& /*y*/, bool /*includePermissions*/) const
{
	std::string response = "User is connected via IRC on channel";
	if ( m_channels.size() > 1 )
		response += "s";

	for ( auto it = m_channels.begin(), end = m_channels.end(); it != end; ++it )
	{
		response += " ";
		response += *it;
	}

	player.message(LSTR_NONE, LSTR_LABELLED_GENERIC, callsign(), response.c_str());
}

//////////////////////////////////////////////////////////////////////

SINT32
chatGetShutdownTimeRemaining()
{
	SINT32 remaining = (SINT32)((TeulDeltaMS)g_shutdownLockAt - (TeulDeltaMS)teulTime());
	return remaining;
}

bool
chatSetShutdownDowntime(UINT32 downtime)
{
	if ( g_shutdownLockAt == 0 )
		return false;
	g_shutdownDowntime = downtime;

	if ( downtime > 0 )
		chatSystemMessage(LSTR_EXPECTED_DOWNTIME, chatTimeInterval(downtime*60));

	return true;
}

void
chatSetShutdownTime(UINT32 forTime, const char* caller)
{
	if ( forTime == 0 )
	{
		if ( g_shutdownLockAt == 0 )
			return;

		Chat::LocalizedMessage lmsg(LSTR_SYSTEM, LSTR_SHUTDOWN_CANCELLED);
		chatBroadcastLocMessage(playerIndex.begin(), playerIndex.end(), lmsg);

		g_shutdownDowntime = 0;
	}
	else
	{
		if ( g_shutdownLockAt > 0 )
		{
			if ( forTime > g_shutdownLockAt )
				chatWarnAll(LSTR_SHUTDOWN_DELAYED);
			else if ( forTime < g_shutdownLockAt - 30 )
				chatWarnAll(LSTR_SHUTDOWN_ADVANCED, chatTimeInterval(g_shutdownLockAt - forTime));
		}

		g_shutdownRequestedBy = caller;
	}

	g_shutdownLockAt = forTime;

	g_shutdownLockedArena = 0;
	g_finalShutdownTime = 0;
}

//////////////////////////////////////////////////////////////////////
// Helper function that returns a property's name or prints the ox/oy/id

const char*
chatPropertyName(const FULL_OBJ_ID id)
{
	Property* property = chatFindProperty(id);
	if ( property != nullptr )
		return property->Name();

	static char name[20 + 1];
	strprintf(name, "[%d.%d.%d]", id.cell.ox, id.cell.oy, id.id);

	return name;
}

// Variant for when you want the pointer yourself
const char*
chatPropertyName(const Property* property, const FULL_OBJ_ID id)
{
	if ( property != nullptr )
		return property->Name();

	static char name[20 + 1];
	strprintf(name, "[%d.%d.%d]", id.cell.ox, id.cell.oy, id.id);

	return name;
}

//////////////////////////////////////////////////////////////////////
//This function sends a system message to the chat server for distribution

bool
chatSystemMessage(LSTRING_ID messageID, const char* const ptr0, const char* const ptr1, const char* const ptr2, const char* const ptr3, const char* const ptr4, const char* const ptr5)
{
	Chat::Broadcast broadcast(Chat::_EVERYONE, 0, 0, LSTR_SYSTEM, messageID, ptr0, ptr1, ptr2, ptr3, ptr4, ptr5);
	return broadcast.route();
}

//////////////////////////////////////////////////////////////////////
// Implementation for HCUnitBroadcast...

using SUBORDINATES = std::vector<UINT32>;
static SUBORDINATES _currentUnits;

//////////////////////////////////////////////////////////////////////
// Determine if a player is in one of the current units.

bool
HCUnitBroadcast::operator()(wwiiPlayer* const& wwiiPlayer) const
{
	const Player* player = static_cast<const Player*>(wwiiPlayer);
	if ( player->teulIndex() < 0 || !player->brigadeID() )
		return false;
	if ( player->isActiveOfficer() && !player->hcOff() )
		return true;
	return ( find(_currentUnits.begin(), _currentUnits.end(), player->brigadeID()) != _currentUnits.end() );
}

//////////////////////////////////////////////////////////////////////
// Populate _currentUnits with a specified unit and all of its descendants

void
HCUnitBroadcast::buildUnitList(const HCUnit* unit)
{
	_currentUnits.clear();
	_currentUnits.push_back(unit->unitID());
	for ( auto it = hcUnits.begin(), end = hcUnits.end(); it != end; ++it )
	{
		if ( it->second->isBelow(unit) )
			_currentUnits.push_back(it->second->unitID());
	}
}

//////////////////////////////////////////////////////////////////////
// Send a packet to a hc unit, all its subunits and active HC staff

void
HCUnitBroadcast::sendToOfficers(const HCUnit* unit, void* object, TEUL_CALLBACK* callback)
{
	PLAYERS& recipients = sidePlayers[unit->side()];
	if ( recipients.empty() )
		return;

	// Clear the units list since we're only sending to officers.
	_currentUnits.clear();

	TeulTransmission broadcast(callback);
	broadcast.populate(object);
	broadcast.send(recipients.begin(), recipients.end(), HCUnitBroadcast());
}

//////////////////////////////////////////////////////////////////////
// Send a packet to a hc unit, all its subunits and active HC staff

void
HCUnitBroadcast::send(const HCUnit* unit, void* object, TEUL_CALLBACK* callback)
{
	PLAYERS& recipients = sidePlayers[unit->side()];
	if ( recipients.empty() )
		return;

	// Prepare a list of my parent unit and/or
	if ( unit->parent() )
		buildUnitList(unit->parent());
	else
		buildUnitList(unit);

	TeulTransmission broadcast(callback);
	broadcast.populate(object);
	broadcast.send(recipients.begin(), recipients.end(), HCUnitBroadcast());
}

//////////////////////////////////////////////////////////////////////
// Prep a host message for a side-style message

static char smbuf[MAX_MESSAGE_LENGTH+1];

Chat::HostMessage&
_prepSideMessage(WWII_SIDE side, UINT8 leftIcon, UINT8* auxIcon)
{
	ChatHandle handleID = 0;
	UINT8 rightIcon = 0;
	UINT8 sideIcon = 0;

	switch (side)
	{
	case ALLIED:
		handleID = alliedHandleID;
		sideIcon = Chat::ALLIED_ICON;	// Allied icon
	break;
	case AXIS:
		handleID = axisHandleID;
		sideIcon = Chat::AXIS_ICON;		// Axis icon
	break;
	default:
	break;
	}

	if ( leftIcon == 0 )
	{
		*auxIcon = sideIcon;
	}
	else
	{
		rightIcon = sideIcon;
		*auxIcon = Chat::HC_ICON;		// HC: icon
	}

	Chat::HostMessage& msg = chatBuildHostMessage(handleID, nextMessageID(), *auxIcon, smbuf);

	msg.radioType[0] = leftIcon;
	msg.radioType[1] = rightIcon;

	return msg;
}

//////////////////////////////////////////////////////////////////////
// This function sends a system message to the chat server for distribution
// You should use GCI messages instead of this.

bool
chatSideMessage(WWII_SIDE side, LSTRING_ID messageID, const char* const ptr0, const char* const ptr1, const char* const ptr2, const char* const ptr3, const char* const ptr4, const char* const ptr5)
{
	LSTRING_ID prefix = LSTR_NONE;
	if ( ALLIED == side )
		prefix = LSTR_ALLIED;
	else if ( AXIS == side )
		prefix = LSTR_AXIS;
	else
	{
		dbugLog(_FLW, "tried to side message side %d", side);
		return false;
	}

	if ( sidePlayers[side].empty() == false )
	{
		Chat::LocalizedMessage message(prefix, messageID, ptr0, ptr1, ptr2, ptr3, ptr4, ptr5);
		chatBroadcastLocMessage(sidePlayers[side].begin(), sidePlayers[side].end(), message, CampaignFilterFn());
	}

	return true;
}

//////////////////////////////////////////////////////////////////////
// Schedule a ban and kick

#ifdef KICK_BANS
struct Ban
{
private:
	static UINT32 _randomBanDelay()
	{
		// Sometimes, leave it a REALLY long time before doing the ban.
		const bool goLong = (rand() % 5) == 0;
		const UINT32 longDelay = goLong ? (404 + (rand() % 86400)) : (rand() % 900);
		return	337			// 5 minute baseline,
				+ (rand() % 108)	// ~2 minutes.
				+ (rand() % 1809)	// ~30 minutes.
				+ longDelay;
	}

public:
	Ban(const ChatHandle handle_, UINT32 days_, bool kick_)
		: m_handle(handle_)
		, m_days(days_)
		, m_start(teulTime() + _randomBanDelay())
		, m_kickTime(kick_ ? m_start + 27 + (rand() % 31) : 0)
	{}

	ChatHandle		m_handle;
	UINT32			m_days;
	UINT32			m_start;
	UINT32			m_kickTime;
};

using DELAYED_BANS = std::vector<Ban>;
static DELAYED_BANS _delayedBans;

static void
_processDelayedBan()
{
	if ( _delayedBans.empty() )
		return;

	static const UINT32 SYSTEM_PLAYER_ID = 69611;

	char days[20];

	for ( auto it = _delayedBans.begin(); it != _delayedBans.end(); ++it )
	{
		Ban& ban = *it;

		bool remove = false;

		if ( (ban.m_start > 0) & (ban.m_start <= teulTime()) )
		{
			if ( ban.m_days <= 90 )
				chatBanPlayer(ban.m_handle, (ban.m_days * DAY_S + (3 * 60)), "ACCOUNT SUSPENDED: Due to unusual activity, your account is being suspended pending review. Contact agm@playnet.com.", SYSTEM_PLAYER_ID, systemHandleID);
			else
				chatBanPlayer(ban.m_handle, (ban.m_days * DAY_S + (3 * 60)), "ACCOUNT SUSPENDED: Your account has been suspended indefinitely. Contact agm@playnet.com assistance.", SYSTEM_PLAYER_ID, systemHandleID);

			Chat::Name name(ban.m_handle);
			strprintf(days, "%u days", ban.m_days);
			dbugLog(_FLN, "!GM:%s:banned:auto:%s", name.c_str(), days);
			chatdbRecordEvent("GM", name.c_str(), "banned", "auto", days);

			if ( ban.m_kickTime == 0 || PlayerInterface::Lookup(ban.m_handle) == nullptr )
				remove = true;
			else
			{
				// Prevent it repeating the ban warning.
				ban.m_start = 0;
			}
		}
		else if ( (ban.m_kickTime > 0) & (ban.m_kickTime <= teulTime()) )
		{
			// Make sure it can't repeat.
			ban.m_kickTime = 0;

			remove = true;

			Player* const player = PlayerInterface::Lookup(ban.m_handle);
			if ( player != nullptr )
			{
				chatSmitePlayer(player->playerID());

				PlayerStatusNode* const psn = ServerInterface::PlayerStatusNodeByPlayerID(player->playerID());
				if ( psn != nullptr )
					player->Release(psn->teulIndex, Server::ReleaseType::EConn);
			}
		}

		if ( remove == true )
		{
			// tail-swap trick.
			*it = _delayedBans.back();
			_delayedBans.pop_back();
			// This invalidates iterators, so a processing event stops the run.
			break;
		}
	}
}

static void
_delayedBan(const ChatHandle handle, UINT32 days, bool kick=true)
{
	Ban ban(handle, days, kick);
	dbugLog(_FLD, "c:%s will be banned for %u days in %u seconds.", Chat::Name(handle).c_str(), ban.m_days, ban.m_start - teulTime());
	_delayedBans.push_back(ban);
}
#endif // KICK_BANS

//////////////////////////////////////////////////////////////////////
// Check if a player is online and if they have the CAN_BE_EJECTED flag

static bool
_isBanable(const ChatHandle handle)
{
	const Player* player = PlayerInterface::Lookup(handle);
	return ( player && player->permissions(CAN_BE_EJECTED) );
}

//////////////////////////////////////////////////////////////////////
// Process a report of a modified player

#ifdef BETA_RELEASE
class DotDetectMe : public DotCommand
{
public:
	DotDetectMe() : DotCommand("detectme", "Bans you for X days", IS_PLAYNET, LSTR_NONE, LSTR_NONE, true) {}

	virtual void actionWith(Player& player, Chat::ClientMessage* /*message*/, char* arguments) const override
	{
		ARGV args;
		if ( vectorize(arguments, args, 2) < 1 )
		{
			help(player);
			return;
		}

		UINT32 days = atoi(args[0]);
		if ( days <= 0 )
		{
			player.message(LSTR_ERROR, LSTR_GENERIC, "Usage: /detectme <number of days greater than 0>");
			return;
		}

		player.message(LSTR_CAUTION, LSTR_GENERIC, "Ok, doing it!");
		_delayedBan(player.matchHandle(), days);
	}
};
static DotDetectMe detectMe;
#endif

void
Chat::SendModReport(const ChatHandle handle, const Chat::ModReport modType)
{
	Chat::Name callsign(handle);
	const char* type = nullptr;
	UINT32 days = 0;

	switch ( modType )
	{
	case Chat::ModUpdateSeq:
		type = "packet";
	break;
	case Chat::ModUpdatePrime:
		if ( _isBanable(handle) )
			days = 10;
		type = "memory";
	break;
	case Chat::ModTamper:
		type = "tamper";
	break;
	case Chat::ModHack:
		return;
//		ban = _isBanable(handle);
//		type = "injection";
	case Chat::ModDamned:
		// Only ban detects one time in 5.
		if ( ((rand() % 5) == 1 && host_config.arena.banCheats > 0) || _isBanable(handle) )
			days = 9999;
		type = "cheat";
	break;
	case Chat::ModMax:
		type = "[test]";
	break;
	default:
		type = "application";
	break;
	}

	PREP_CHAT_MSG(message, "%s %s client-mod detected", callsign.c_str(), type);
	chatDispatchGMMessage("HOST", message);

	dbugLog(_FLN, "!HACK:%s:%s", callsign.c_str(), type);
	chatdbRecordEvent("HACK", callsign.c_str(), type, "", "");

	if ( days > 0 )
	{
#if defined(KICK_BANS)
		_delayedBan(handle, days);
#endif
	}
}

//////////////////////////////////////////////////////////////////////
// Transmit a HQ message

void
chatRouteGCIMessage(Chat::GCIMessage& message, UINT32 messageID)
{
	if ( PN_UNLIKELY(playerIndex.empty()) )		// And when it's true, we can afford the hit.
		return;

	std::vector<ENDPOINT*> endpoints;
	endpoints.reserve(playerIndex.size());

	for ( auto it = playerIndex.begin(), end = playerIndex.end(); it != end; ++it )
	{
		Player* const player = it->second;
		if ( player->approveGCIMessage(messageID, message, true) )
		{
			ENDPOINT* ep = teulGetEndpoint(player->teulIndex());
			if ( ep != nullptr )
				endpoints.push_back(ep);
		}
	}

	if (PN_UNLIKELY(endpoints.empty()))
		return;

	Teul::Packet outgoing(Teul::PacketId(Chat::CallbackX, Chat::CLIENT_GCI_MESSAGE_Y), message);
	outgoing.send(endpoints);
}

//////////////////////////////////////////////////////////////////////
// Send an "all" message.

void
chatSendAllMessage(const ChatHandle handle, const char* message, UINT32 messageID)
{
	Chat::HostMessage& msg = chatBuildHostMessage(handle, messageID, LOBBY_CHAN_ICON, message);
	chatBroadcastHostMessage(playerIndex.begin(), playerIndex.end(), msg);

	wwiiIRCMessage("-sys", "ALL:%s: %s", Chat::Name(handle).c_str(), message);
}

//////////////////////////////////////////////////////////////////////

static UCHAR
_getChanIconFromType(UINT32 type, WWII_COUNTRY country=WWII_NO_COUNTRY)
{
	switch(type)
	{
		case ADMIN_CHANNEL:
			return 15;
		case BRIGADE_CHANNEL:
			return 19;
		case MISSION_CHANNEL:
			return 12;
		case SQUAD_CHANNEL:
			return 13;
		case CP_CHANNEL:
			return 14;
		case NUMBERED_CHANNEL:
		case PRIVATE_CHANNEL:
			return 15;
		case ORIGIN_CP_CHANNEL:
			return 14;
		case OBJECTIVE_CP_CHANNEL:
			return  21;
		case SIDE_GENERAL_CHANNEL:
			if ( country > WWII_NO_COUNTRY && country < MAX_COUNTRIES )
				return 15 + wwiiSide(country);
			else
				return 11;
		case SIDE_HELP_CHANNEL:
			return 20;
		case SIDE_OPS_CHANNEL:
			return 11;
		default:
			return 0;
	}
}

//////////////////////////////////////////////////////////////////////
//Sends a message to a player* without* ignore checking

SINT32
Player::send(const Chat::HostMessage& outgoing)
{
	if ( PN_UNLIKELY(teulIndex() < 0) )
		return 0;

	if ( g_debugChat )
		dbugLog(_FLD,"t:%d p:%u c:%s sending message %d", teulIndex(), playerID(), callsign(), outgoing.id);

	m_lastMessageID = outgoing.id;
	return teulSend(m_teulIndex, CHAT_SEND_MESSAGE, (void*)&outgoing);
}

//////////////////////////////////////////////////////////////////////
// check a snoop connection and if it looks valid, tell the snooper

static void
_snoop(const char* type, Player* player, Player* snooper, const char* to, const char* text)
{
	if ( snooper == nullptr || player == nullptr || snooper == player )
		return;

	PREP_CHAT_MSG(msg, "%s->%s", player->callsign(), to)
	snooper->message(LSTR_GM, LSTR_LABELLED_GENERIC, msg, text);
	chatdbRecordEvent(type, player->callsign(), "(snoop)", to, text);
}

//////////////////////////////////////////////////////////////////////

bool
chatSendToRecipient(Player* player, Player* recipient, const Chat::HostMessage& message)
{
	if ( recipient == nullptr || recipient->teulIndex() < 0 || recipient->active() == false )
		return false;

	if ( recipient->inGrab() || recipient->lastMessageID() == message.id )
		return false;

	// Don't do all the work involved if we know now in advance we're ignoring them
	if ( recipient->ignoring(player) )
		return false;

	if ( recipient->send(message) < 0)
	{
		dbugLog(_FLA,"Something wrong in sending message ID %d to player %s",message.id,recipient->callsign());
	}

	if ( message.auxChanType == PRIVATE_CHANNEL && player != recipient )
	{
		_snoop("GM", player, recipient->m_snoopBy, recipient->callsign(), message.msg);
		_snoop("CRS", player, recipient->m_crsSnoopBy, recipient->callsign(), message.msg);
	}

	return true;
}

//////////////////////////////////////////////////////////////////////
// Handle a message send attempt by someone who is in grab mode

static void
_distributeSquelched(Player& player, Chat::HostMessage* hostMessage)
{
	Player*	recipient = nullptr;

	//If we are the game manager, then the recipient is who we squelched
	if ( player.grabbing() )
		recipient = PlayerInterface::Lookup(player.grabbing());
	//else, the recipient is who has taken us
	else if ( player.takenBy() )
		recipient = PlayerInterface::Lookup(player.takenBy());
	else
		return;

	if ( recipient == nullptr )
	{
		player.message(LSTR_CAUTION, LSTR_PARTY_UNAVAILABLE);
		chatReleaseGrab(player);
		return;
	}

	// Log all grab conversations
	dbugLog(_FLI, "!GM:%s:GRAB:%s:%s"
					, player.callsign(), recipient->callsign(), hostMessage->msg);
	chatdbRecordEvent("GM", "GRAB", player.callsign(), recipient->callsign(), hostMessage->msg);

	hostMessage->icon = _getChanIconFromType(PRIVATE_CHANNEL);

	//Send it directly to the player
	if ( !recipient->send(*hostMessage) || !player.send(*hostMessage) )
	{
		chatReleaseGrab(player);
	}
}

//////////////////////////////////////////////////////////////////////
//This actually distributes a message

static void
_distributeChannelMessage(Chat::HostMessage& message, Chat::Channel& channel, Player& sender)
{
	//If there is nobody on the channel, bail out
	if ( channel.subscribers().empty() )
	{
		if ( g_debugChat )
			dbugLog(_FLD, "Nobody on channel list");
		return;
	}

	dbugLog(_FLD, "t:%d p:%u c:%s channel:{type:%u,name:'%s',subs:%" PRSizeT "} distributing: %s"
					, sender.teulIndex(), sender.playerID(), sender.callsign()
					, channel.type(), channel.m_name.c_str(), channel.numSubscribers()
					, message.msg
					);

	std::vector<SINT32 /*teulIndex*/> indexes;
	auto it = channel.subscribers().begin();
	while ( it != channel.subscribers().end() )
	{
		const Chat::ChannelSubscription* const cnode = *(it++);
		Player* const recipient = cnode->m_subscriber;
		if ( recipient )
		{
			if ( (recipient->teulIndex() < 0) | (recipient->active() == false) | (recipient->inGrab()) | (recipient->lastMessageID() >= message.id) )
				continue;
			if ( recipient->ignoring(sender) )
				continue;
			indexes.push_back(recipient->teulIndex());
			recipient->m_lastMessageID = message.id;
		}
	}

    dbugLog(_FLD, " [%" PRSizeT " recipients]", indexes.size());

	chatBroadcastHostMessage(indexes.begin(), indexes.end(), message);
}

//////////////////////////////////////////////////////////////////////
// Tell player when they have tried to transmit on a dummy/placeholder
// channel
void
chatReportPlaceHolder(Player* player, CHANNEL_TYPE type)
{
	LSTRING_ID message = LSTR_NONE;
	const char* arg = nullptr;

	switch ( type )
	{
	case BRIGADE_CHANNEL:
		message = LSTR_TX_NO_BRIGADE;
	break;
	case MISSION_CHANNEL:
		message = LSTR_TX_NO_MISSION;
	break;
	case CP_CHANNEL:
		message = LSTR_TX_NO_MISSION_CITYS;
	break;
	case ORIGIN_CP_CHANNEL:
		message = LSTR_TX_NO_MISSION_ORIGIN;
	break;
	case OBJECTIVE_CP_CHANNEL:
		message = LSTR_TX_NO_MISSION_OBJECTIVE;
	break;
	case SQUAD_CHANNEL:
		if ( !player->squadID() )
			message = LSTR_TX_NO_SQUAD;
		else
			message = LSTR_TX_SQUAD_UNAVAILABLE;
	break;
	case GCI_CHANNEL:
		message = LSTR_TX_CH_RX_ONLY;
		arg = "HQ";
	break;
	case SIDE_GENERAL_CHANNEL:
	case SIDE_HELP_CHANNEL:
	case SIDE_OPS_CHANNEL:
		message = LSTR_TX_NO_SIDE;
	break;
	default:
	break;
	}

	if ( message != LSTR_NONE )
		player->message(LSTR_CAUTION, message, arg);
}

//////////////////////////////////////////////////////////////////////
// Process a chat message

bool
_dotCommand(Player& player, Chat::ClientMessage* message)
{
	dbugLog(_FLD, "Is this a dot command?");
	if ( message->m_message[0] != '.' && message->m_message[0] != '/' )
		return false;
	if ( message->m_message[0] == message->m_message[1] )
		return false;
	if ( !isalnum(message->m_message[1]) )
		return false;
	if ( message->m_message[0] == '/' && message->m_message[1] && message->m_message[2] == '!' )
		return false;

	dbugLog(_FLD, "Yes, dot command - convert to slash and go look it up");

	if ( message->m_message[0] != '/' )
		message->m_message[0] = '/';
	if ( ! DotCommand::index()->process(player, message) )
		player.message(LSTR_CAUTION, LSTR_UNKNOWN_COMMAND, message->m_message);

	return true;
}

static bool
_stratCommand(const char* input, Player* player)
{
	if ( !player->isPlaynet() )
		return false;
	if ( input[0] != ';' )
		return false;
#ifdef PUBLIC_RELEASE
	if ( !player->isExplicitGM() )
		return false;
#endif

	player->sendStratCommand(input+1, nullptr, true);
	return TRUE;
}

static void
_radioSnoop(Player* player, Chat::ClientMessage* message)
{
	ABORT_IF( player == nullptr );
	if ( (player->m_snoopBy == nullptr || player->m_snoopBy == player)
		&& (player->m_crsSnoopBy == nullptr || player->m_crsSnoopBy == player) )
		return;

	// Caller has determined we are being snooped
	std::string channels = "[";

	for ( UINT32 i = 0; i < MAX_CLIENT_CHANNELS; ++i )
	{
		if ( !(message->m_radios & (1 << i)) )
			continue;

		const Chat::ChannelSubscription& cnode = player->getChannelSub(i);
		if ( !cnode.isTuned() )
			continue;

		if ( channels.size() > 1 )
			channels += ",";
		if ( cnode.type() == PRIVATE_CHANNEL && cnode.channel() )
			channels += cnode.channel()->m_name;
		else
			channels += chanTypeName[cnode.type()];
	}

	if ( channels.size() <= 1 && player->isSpawned() )
		channels = "[Area]";
	else if ( channels.size() <= 1 )
		channels = "[Lobby]";
	else
		channels += "]";

	std::string text = message->m_message;
	if ( text.length() >= MAX_MESSAGE_LENGTH )
	{
		text.resize(MAX_MESSAGE_LENGTH - 4);
		text += "...";
	}

	_snoop("GM", player, player->m_snoopBy, channels.c_str(), text.c_str());
	_snoop("CRS", player, player->m_crsSnoopBy, channels.c_str(), text.c_str());
}

#ifdef OLD_EASTER_EGG
static inline void
_wtfEasterEgg(Player* player, const Chat::ClientMessage* message)
{
	if ( strncasecmp(message->m_message, "wtf", 3) == 0 && message->m_length < 7 )
	{
		Chat::ClientMessage fake = *message;
		strcpy(fake.m_message, "/wtf");
		DotCommand::index()->process(player, &fake);
	}
}
#endif

//////////////////////////////////////////////////////////////////////
// Inject the unit name and mission leader as they are available.

bool
_injectUnitAndLeader(std::string& buffer, const size_t pos, UINT32 brigadeID, UINT32 missionID, const size_t charsToSubtitute)
{
	std::string unitName = "";

	if ( brigadeID )
	{
		const HCUnit* const unit = HCUnit::Lookup(brigadeID);
		if ( unit != nullptr )
		{
			if ( unit->parent() )
			{
				unitName += unit->parent()->shortName();
				unitName += "/";
			}
			unitName += unit->shortName();
		}
	}

	if ( missionID )
	{
		ChatMission* mission = chatFindMission(missionID);
		if ( mission != nullptr )
		{
			const UINT64 leader = mission->leader();
			if ( leader )
			{
				if ( unitName.empty() == false )
					unitName += "/";
				unitName += Chat::Name(mission->leader()).c_str();
			}
		}
	}

	if ( unitName.empty() == true )
		return false;

	buffer.replace(pos, charsToSubtitute, unitName);
	return true;
}

//////////////////////////////////////////////////////////////////////
// Replace a tag with the player's capture location/status.

static bool
_injectCaptureStatus(std::string& buffer, const size_t pos, const size_t charsToSubstitute, const PropFacility* const capping, const SINT32 progress, const PropCP* const nearCP)
{
	std::string status = "";
	if ( capping )
	{
		status = capping->m_name;
		if ( progress > -1 )
		{
			char pctg[12];
			strprintf(pctg, " (%u%%)", std::min(progress, 100));
			status += pctg;
		}
	}
	else if ( nearCP )
	{
		status = nearCP->m_name;
	}
	else
		return false;

	buffer.replace(pos, charsToSubstitute, status);
	return true;
}

//////////////////////////////////////////////////////////////////////
// Process $.... subs in your input text.

static bool
_processDollars(const Player* player, const char* base, char* start)
{
	char* p = start;
	bool translations = false;
	std::string buffer(p);
	size_t pos = 0;

	// How long are the names?
	static const size_t _NAME_LEN = 4;
	// So how many character is the entire tag?
	static const size_t _TAG_LEN = _NAME_LEN + 1;

	while ( pos != buffer.npos )
	{
		// Stop looking when there aren't enough characters remaining.
		if ( pos + _NAME_LEN >= buffer.length() )
			break;

		// All of the sequences are /[$%][a-z]{4}/
		if ( !isalpha(buffer[pos + 1]) || isupper(buffer[pos+1]) )
		{
			// Allow escaping of shortcut prefix, e.g. '$$', '$%', '%%', '%$'
			if ( buffer[pos + 1] == '%' || buffer[pos + 1] == '$' )
			{
				buffer.erase(pos, 1);
				// Since this modifies the string, it counts as a translation.
				translations = true;
			}
		}
		else if ( buffer.compare(pos + 1, _NAME_LEN, "miss") == 0 )
		{
			translations = _injectUnitAndLeader(buffer, pos, player->brigadeID(), player->missionID(), _TAG_LEN);
		}
		else if ( buffer.compare(pos + 1, _NAME_LEN, "here") == 0 )
		{
			translations = _injectCaptureStatus(buffer, pos, _TAG_LEN, player->m_capping, player->m_capProgress, static_cast<const PropCP*>(player->lastCP()));
		}
		else if ( buffer.compare(pos + 1, _NAME_LEN, "lead") == 0 )
		{
			translations = _injectUnitAndLeader(buffer, pos, 0, player->missionID(), _TAG_LEN);
		}
		else if ( buffer.compare(pos + 1, _NAME_LEN, "targ") == 0 )
		{
			if ( player->objectiveCpID().id != 0 )
			{
				buffer.replace(pos, _TAG_LEN, player->objectiveCP());
				translations = true;
			}
		}
		else if ( buffer.compare(pos + 1, _NAME_LEN, "orig") == 0 )
		{
			if ( player->originCpID().id != 0 )
			{
				buffer.replace(pos, _TAG_LEN, player->objectiveCP());
				translations = true;
			}
		}
		else if ( buffer.compare(pos + 1, _NAME_LEN, "unit") == 0 )
		{
			translations = _injectUnitAndLeader(buffer, pos, player->brigadeID(), 0, _TAG_LEN);
		}
		else if ( buffer.compare(pos + 1, _NAME_LEN, "hcid") == 0 )
		{
			HCUnit* unit = HCUnit::Lookup(player->brigadeID());
			if ( unit != nullptr )
			{
				buffer.replace(pos, _TAG_LEN, unit->callsign());
				translations = true;
			}
		}
		else if ( buffer.compare(pos + 1, _NAME_LEN, "town") == 0 )
		{
			if ( player->lastCP() )
			{
				buffer.replace(pos, _TAG_LEN, player->lastCP()->Name());
				translations = true;
			}
		}

		pos = buffer.find_first_of("$%", pos + 1);
	}

	if ( translations )
	{
		const size_t limit = MAX_MESSAGE_LENGTH - 1 - (start - base);
		if ( buffer.length() > limit )
			buffer.resize(limit);
		strcpy(start, buffer.c_str());
	}

	return translations;
}

//////////////////////////////////////////////////////////////////////
// chatRouteMessage helper:
// Make sure the various fields in the incoming message are properly
// set up because we assume the caller hasn't.

static inline void
_sanitizeIncomingMessage(Chat::ClientMessage* inMessage, Player* player, UINT32 messageID)
{
	inMessage->m_id = messageID;
	// make sure its null terminated!!
	inMessage->m_message[MAX_MESSAGE_LENGTH-1] = '\0';
	inMessage->m_handle = player->chatHandle();

	//If their country has been set, attach it to the message
	if (player->country() > 0)
		inMessage->m_country = player->country();
	//else, we use their side and translate it to an icon
	else if ( player->side() > 0 )
		inMessage->m_country = player->side() + 15;
	else
		inMessage->m_country = 0;
}

//////////////////////////////////////////////////////////////////////
// chatRouteMessage helper:
// Prepare a ChatHostMessage for sending and return a pointer to a
// temporary list of channels.

static inline void
_prepareHostMessage(Player* player, const Chat::ClientMessage* inMessage, Chat::HostMessage* xmitMessage)
{
	for ( UINT32 i = 0; i < MAX_CLIENT_CHANNELS; ++i )
	{
		xmitMessage->radioType[i] = 0;
	}

	//Prepare the host message
	xmitMessage->handle = inMessage->m_handle;
	xmitMessage->id = inMessage->m_id;
	xmitMessage->isDotMessage = false;
	xmitMessage->auxChanType = 0;
	xmitMessage->length = inMessage->m_length;
	xmitMessage->icon = inMessage->m_country;
	xmitMessage->msg = inMessage->m_message;
}

//////////////////////////////////////////////////////////////////////
// Some channels produce a periodic usage reminder to users.

static void
_checkForChannelUsageReminders(Player& player, const Chat::ChannelSubscription& cnode)
{
	switch ( cnode.type() )
	{
	case SIDE_HELP_CHANNEL:
		// Transmitting on "help", remind player of it's use.
		if ( false == player.checkTimerActive("helpch") )
		{
			player.message(LSTR_CAUTION, LSTR_HELP_CHANNEL_NOTE);
			player.SetTimer("helpch", teulClock64 + 5 * 60 * 1000);
		}
	break;
	case SIDE_OPS_CHANNEL:
		if ( false == player.checkTimerActive("opsch") )
		{
			player.message(LSTR_CAUTION, LSTR_OPS_CHANNEL_NOTE);
			player.SetTimer("opsch", teulClock64 + 15 * 60 * 1000);
		}
	break;
	default:
	break;
	}
}

//////////////////////////////////////////////////////////////////////
// Insert a prefix ahead of a message body.

static void
_prefixMessageBody(std::string& prefix, Chat::HostMessage& message)
{
	prefix += message.msg;

	message.msg = prefix.c_str();
	message.length = (UINT32) ( (UINT32)prefix.length() < MAX_MESSAGE_LENGTH ) ? (UINT32)prefix.length() : MAX_MESSAGE_LENGTH;
}

//////////////////////////////////////////////////////////////////////
// Process an incoming chat message.

void
chatRouteMessage(Player* player, Chat::ClientMessage* inMessage, UINT32 messageID, bool allowDotCommands)
{
	if ( player == nullptr || inMessage == nullptr )
	{
		dbugLog(_FLA,"illegal message data player %p iMess %p"
				, player, inMessage);
		return;
	}

	if ( allowDotCommands )
	{
		char* p = strpbrk(inMessage->m_message, "$%");
		if ( p != nullptr )
		{
			if ( _processDollars(player, inMessage->m_message, p) )
				inMessage->m_length = (UINT32)strlen(inMessage->m_message);
		}
	}

	_sanitizeIncomingMessage(inMessage, player, messageID);

	if ( allowDotCommands )
	{
		std::string msgText = inMessage->m_message;
		DBThread::EscapeString(msgText);

		DBThread::Do(DBThread::Audit, "INSERT INTO `audit`.`chat_messages` (player_id, message) VALUES (%u, '%s')", player->playerID(), msgText.c_str());

		_radioSnoop(player, inMessage);
		if ( _dotCommand(*player, inMessage) )
			return;
		if ( _stratCommand(inMessage->m_message, player) )
			return;
#ifdef BETA_RELEASE
		if ( player->isPlaynet() && inMessage->m_message[0] == '$' )
		{
		}
#endif
	}

	// check for area channel, if it's set, invoke the say function
	bool isAreaChat = false;
	if ( inMessage->m_radios & AREA_CHN_BIT && player->inGrab() == false )
	{
		inMessage->m_radios &= ~AREA_CHN_BIT;		// Remove the bit

		if ( player->isSpawned() )
		{
			isAreaChat = true;
			chatSay(player, inMessage, inMessage->m_message, AREA_SAY, false);
		}
		else if ( inMessage->m_radios == 0 )
		{
			// You've just hit "enter" at the lobby/debrief screen; pick some useful channels
			player->message(LSTR_CAUTION, LSTR_NO_CHANNELS_FOR_AREA);
			return;
		}
	}

//	inMessage->m_handle = player->chatHandle();
//
	Chat::HostMessage xmitMessage;
	_prepareHostMessage(player, inMessage, &xmitMessage);

	//If we are takenBy or are 'grabbing' someone
	if ( player->inGrab() )
	{
		_distributeSquelched(*player, &xmitMessage);
		return;
	}

	std::string prefix = "";
	Chat::Channel* channel = nullptr;
	bool foundChannel = false;

	for ( UINT32 i = 0; i < MAX_CLIENT_CHANNELS; ++i )
	{
		if ( !(inMessage->m_radios & (1 << i)) )
			continue;

		Chat::ChannelSubscription& cnode = player->getChannelSub(i);

		// Place-holder channel, not actually tuned
		if ( !cnode.inUse() )
		{
			continue;
		}

		if ( cnode.isPlaceHolder() )
		{
			// Player doesn't have access to this channel right now.
			chatReportPlaceHolder(player, cnode.type());
			continue;
		}

		foundChannel = true;
		isAreaChat = false;

		_checkForChannelUsageReminders(*player, cnode);

		// Are they tuned to a standard or named channel
		channel = cnode.channel();
		if ( channel )
		{	// Standard channel
			xmitMessage.radioType[0] = _getChanIconFromType(cnode.type(), player->country());

			if ( cnode.type() == NUMBERED_CHANNEL )
			{
				prefix = "[";
				prefix += channel->m_name;
				prefix += "] ";
			}
			else if ( cnode.type() == SIDE_OPS_CHANNEL && player->isActiveCommander() )
			{
				xmitMessage.icon = Chat::HC_ICON;
			}
		}
		else
		{	// Named channel
			xmitMessage.auxChanType = PRIVATE_CHANNEL;
			xmitMessage.radioType[0] = _getChanIconFromType(PRIVATE_CHANNEL);
		}

		break;
	}

	// Anything to send?
	if ( !isAreaChat && !foundChannel )
		return;

	if ( isAreaChat )
		xmitMessage.icon = Chat::SAY_ICON;
	else if ( xmitMessage.auxChanType != 0 )
	{
		xmitMessage.icon = _getChanIconFromType(PRIVATE_CHANNEL);
	}

	if ( prefix.length() > 0 )
		_prefixMessageBody(prefix, xmitMessage);

	// Channel distributions
	if ( channel && channel->type() != GCI_CHANNEL )
		_distributeChannelMessage(xmitMessage, *channel, *player);

	// Check to see if you didn't see a copy of the message?
	if ( player->lastMessageID() != xmitMessage.id )
	{
		if ( !isAreaChat )
		{
			std::string echo("You said: ");
			_prefixMessageBody(echo, xmitMessage);
		}
		player->send(xmitMessage);
	}

	return;
}

//////////////////////////////////////////////////////////////////////
// Based on old formatOutMessage but without uneccessary (and broken)
// bells, whistles and trimmings

Chat::HostMessage&
chatBuildHostMessage(const ChatHandle senderID
					, UINT32 msgID
					, UINT8 chanIcon
					, const char* text
					)
{
	static Chat::HostMessage msg;

	msg.isDotMessage = true;
	msg.auxChanType = PRIVATE_CHANNEL;
	msg.length = 0;
	msg.id = msgID;
	msg.icon = chanIcon;
	msg.handle = senderID;

	if ( text == nullptr )
		msg.msg = "";
	else
		msg.msg = text;
	msg.length = (UINT32)strnlen(msg.msg, MAX_MESSAGE_LENGTH);

	for ( int i = 0; i < MAX_CLIENT_CHANNELS; i++ )
		msg.radioType[i] = 0;

	return msg;
}

//////////////////////////////////////////////////////////////////////

struct IsRat : public FilterFn
{
	static ChatHandle excludeID;
	bool operator()(const wwiiPlayer* player) const { return player->isPlaynet() && ((const Player*)player)->initialized() && player->matchHandle() != excludeID; }
};
ChatHandle IsRat::excludeID = 0LL;

void
chatDispatchCRSMessage(const SCHAR* source, const SCHAR* text, const ChatHandle excludeID)
{
	Chat::LocalizedMessage lmsg(LSTR_NONE, LSTR_CRS_MSG, source, text);

	const char* ircMessage = chatLString(lmsg);
	wwiiIRCChannelMessage("-sys", ircMessage);

	IsRat::excludeID = excludeID;
	chatBroadcastLocMessage(commandList.begin(), commandList.end(), lmsg, IsRat());
}

//////////////////////////////////////////////////////////////////////

struct IsGM : public FilterFn
{
	static ChatHandle excludeID;
	bool operator()(const wwiiPlayer* player) const { return (player->isGM() && player->matchHandle() != excludeID && ((const Player*)player)->initialized()); }
};
ChatHandle IsGM::excludeID = 0LL;

void
chatDispatchGMMessage(const SCHAR* source, const SCHAR* text, const ChatHandle excludeID)
{
	Chat::LocalizedMessage lmsg(LSTR_NONE, LSTR_GM_MSG, source, text);

	const char* ircMessage = chatLString(lmsg);
	wwiiIRCChannelMessage("-sys", ircMessage);

	IsGM::excludeID = excludeID;
	chatBroadcastLocMessage(commandList.begin(), commandList.end(), lmsg, IsGM());
}

//////////////////////////////////////////////////////////////////////
// Send a textual message to a high command channel

bool
chatDispatchHCText(WWII_SIDE side, const SCHAR* source, const SCHAR* text)
{
	return chatHCMessage(side, LSTR_LABELLED_GENERIC, source, text);
}

//////////////////////////////////////////////////////////////////////
// Send a localized message to a high command channel

struct IsActiveCommander : public FilterFn
{
	WWII_SIDE m_side;
	ChatHandle m_exclude;
	IsActiveCommander(WWII_SIDE side, ChatHandle exclude)
		: m_side(side), m_exclude(exclude) {}
	bool operator()(const wwiiPlayer* wwiiPlayer) const
	{
		const Player* player = static_cast<const Player*>(wwiiPlayer);
		return (player->effectiveOfficerSide() == m_side && player->initialized() && !player->hcOff() && player->matchHandle() != m_exclude);
	}
};

bool
chatHCMessage(WWII_SIDE side, LSTRING_ID message, const char* const ptr0, const char* const ptr1, const char* const ptr2, const char* const ptr3, const char* const ptr4, const char* const ptr5)
{
	Chat::LocalizedMessage lmsg(LSTR_NONE, message, ptr0, ptr1, ptr2, ptr3, ptr4, ptr5);
	return chatHCMessage(side, lmsg, 0ULL);
}

bool
chatHCMessage(WWII_SIDE side, Chat::LocalizedMessage& lmsg, const ChatHandle exclude)
{
	const char* channel;
	if ( side == ALLIED )
	{
		lmsg.m_prefix = LSTR_ALLIED_HC;
		channel = "-ahc,-hcs";
	}
	else if ( side == AXIS )
	{
		lmsg.m_prefix = LSTR_AXIS_HC;
		channel = "-ghc,-hcs";
	}
	else
		return false;

	wwiiIRCChannelMessage(channel, chatLString(lmsg));
	chatBroadcastLocMessage(commandList.begin(), commandList.end(), lmsg, IsActiveCommander(side, exclude));

	return true;
}

//////////////////////////////////////////////////////////////////////
// Destructor for hold/fall object -- sends the object.

HoldFallVote::~HoldFallVote()
{
	chatBroadcastObject(commandList.begin(), commandList.end(), s_callback, *this, IsActiveCommander(m_side, 0LL));
}

//////////////////////////////////////////////////////////////////////
// Function to release a player from grab; used in several situations

void
chatReleaseGrab(Player& player)
{
	Player* grabber = nullptr;
	Player* grabbed = nullptr;
	const char* subject;

	// Resolve which way around this is
	if ( player.grabbing() )
	{
		grabber = &player;
		grabbed = PlayerInterface::Lookup(player.grabbing());
	}
	else if ( player.takenBy() )
	{
		grabbed = &player;
		grabber = PlayerInterface::Lookup(player.takenBy());
	}

	Player* logAs = nullptr;

	if ( grabber )
	{
		logAs = grabber;

		grabber->ungrabbed();

		subject = (grabbed) ? grabbed->callsign() : "subject";
		grabber->message(LSTR_NOTE, LSTR_HAVE_UNGRABBED, subject, nullptr);
	}

	if ( grabbed )
	{
		if ( logAs == nullptr ) logAs = grabbed;

		grabbed->ungrabbed();

		subject = (grabber && (grabbed->isGM() || grabbed->isPlaynet())) ? grabber->callsign() : "(GM)";
		grabbed->message(LSTR_NOTE, LSTR_BEEN_UNGRABBED, subject, nullptr);
	}

	if (logAs != nullptr)
		logAs->gmNote("GM", "ungrab", nullptr, nullptr);
}

//////////////////////////////////////////////////////////////////////
//Player death, no mission points

void
chatSmitePlayer(const playerid_t playerID)
{
	chatSendSmiteUser(playerID, "");
}

//////////////////////////////////////////////////////////////////////
// Smite player by handle - ghost remover

void
chatSmiteHandle(const char* handle)
{
	chatSendSmiteUser(0, handle);
}

//////////////////////////////////////////////////////////////////////
// Tune a specific radio to a specific channel

bool
Player::tuneRadioToChannel(UCHAR index, Chat::Channel* toChannel, CHANNEL_TYPE type)
{
	if ( index >= MAX_CLIENT_CHANNELS || toChannel == nullptr )
		return false;

	ABORT_IF_EX(type == NO_CHANNEL, "t:%d p:%u c:%s attempting to tune radio %d to NO_CHANNEL", teulIndex(), playerID(), callsign(), index);

	if ( g_debugRadios )
		dbugLog(_FLD, "t:%d p:%u c:%s tuning radio %d to %s %s", teulIndex(), playerID(), callsign(), index, chanTypeName[type], toChannel->m_name.c_str());

	return _setChannel(index, type, toChannel, true);
}

//////////////////////////////////////////////////////////////////////

void
Player::announceToSquadChannel()
{
	if ( !squadID() || !isValidSide(side()) ||  m_announcedToSquad )
		return;

	SquadList* squadList = chatFindSquadList(squadID());
	if ( squadList == nullptr || squadList->isNewSquad() )
		return;

	if ( g_debugSquads )
		dbugLog(_FLD, "t:%d p:%u c:%s sq:%u announcing to squad"
						, teulIndex(), playerID(), callsign()
						, m_squadID
						);

	const ChannelID& channelID = chatGetSquadChannelID(0ULL, squadID(), side());
	chatMessageChannel(channelID, LSTR_SQUAD, LSTR_SIGNED_ON, callsign());

	const char* motd = chatFindSquadMotd(squadID());
	if ( motd != nullptr && *motd != 0 )
		message(LSTR_SQUAD, LSTR_MOTD_STRING, motd);

	// Find out if the squad is running a vote
	SquadVote* vote = chatFindSquadVote(squadID());
	if ( vote != nullptr )
		vote->announce(squadID(), this);

	squelchSquadAnnounce();
}

//////////////////////////////////////////////////////////////////////

static Chat::Channel*
_getChannel(const ChannelID& id)
{
	static SCHAR channelName[80+1];

	//See if its already there
	Chat::Channel* const channel = chatFindChannel(id);
	if ( channel )
		return channel;

	//If not, create it, and use it
	if ( id.Type() == NUMBERED_CHANNEL )
	{
		strprintf(channelName, "%" PR64BitUT "", id.SubID());
		ChatHandle handle = chatGetHandle(channelName);
		strprintf(channelName, "%" PR64BitUT "", id.SubID());
		return chatCreateChannel(id, channelName, handle);
	}
	else
	{
		strprintf(channelName, "Channel %" PR64BitUT ".%u.%u.%" PR64BitUT "", id.InstID(), id.Type(), id.Side(), id.SubID());
		return chatCreateChannel(id, channelName, 0ULL);
	}
}

//////////////////////////////////////////////////////////////////////

bool
Player::tuneRadio(UCHAR index, CHANNEL_TYPE type, const ChatHandle handleID)
{
	ChannelID channelID((CHANNEL_TYPE)0);
	Chat::Channel* tuneChannel = nullptr;

	if ( g_debugRadios )
		dbugLog(_FLD, "t:%d p:%u c:%s rid:%d tune request to type %s [%d]", teulIndex(), playerID(), callsign(), index, chanTypeName[type], type);

	//Get the channelID
	switch (type)
	{
		case NO_CHANNEL:
			detuneRadio(index);
			return true;

		case PRIVATE_CHANNEL:
		case NUMBERED_CHANNEL:
			return tuneRadioToHandle(index, handleID);

		case SQUAD_CHANNEL:
			if ( squadID() )
				channelID = chatGetSquadChannelID(0ULL, squadID(), side());
			else
			{
				tunePlaceHolder(index, type);
				message(LSTR_CAUTION, LSTR_NO_SQUAD);
				return true;
			}
		break;

		case MISSION_CHANNEL:
			channelID = chatGetMissionChannelID(instance().id(), m_missionID, side());
		break;

		case CP_CHANNEL:
		case ORIGIN_CP_CHANNEL:
			channelID = chatGetCPChannelID(instance().id(), m_originCpID, side());
		break;

		case OBJECTIVE_CP_CHANNEL:
			channelID = chatGetCPChannelID(instance().id(), m_objectiveCpID, side());
		break;

		case GCI_CHANNEL:
			channelID = chatGetGCIChannelID(instance().id(), side());
		break;

		case BRIGADE_CHANNEL:
			channelID = chatGetBrigadeChannelID(instance().id(), m_brigadeID, side());
		break;

		case SIDE_GENERAL_CHANNEL:
			channelID = chatGetSideChannelID(0ULL, SIDE_SUB_GENERAL, side());
		break;

		case SIDE_HELP_CHANNEL:
			channelID = chatGetSideChannelID(0ULL, SIDE_SUB_HELP, side());
		break;

		case SIDE_OPS_CHANNEL:
			channelID = chatGetSideChannelID(instance().id(), SIDE_SUB_OPS, side());
		break;

		case ADMIN_CHANNEL:
			if ( isGM() )
			{
				dbugLog(_FLW, "p:%u c:%s non-GM attempted admin channel", playerID(), callsign());
			}
			else
			{
				channelID = chatGetAdminChannelID();
				tuneChannel = adminChannel;
			}
		break;

		default:
		break;
	}

	if ( channelID.IsPlaceHolder() )
	{
		if ( g_debugRadios )
			dbugLog(_FLD, " [placeholder]");
		tunePlaceHolder(index, type);
		return true;
	}

	if ( channelID.IsSet() && tuneChannel == nullptr )
	{
		tuneChannel = _getChannel(channelID);
	}

	if ( tuneChannel == nullptr )
	{
		message(LSTR_CAUTION, LSTR_CHANNEL_UNAVAILABLE);
		detuneRadio(index);
		return false;
	}

	if ( type == SQUAD_CHANNEL )
		updateSquadEntry(true);
	else if ( type == SIDE_HELP_CHANNEL )
		message(LSTR_SYSTEM, LSTR_HELP_CHANNEL_GREETING);
	else if ( type == MISSION_CHANNEL )
		chatMissionOrders(this);

	tuneRadioToChannel(index, tuneChannel, type);
	m_channels[index].m_type = type;

	return true;
}

//////////////////////////////////////////////////////////////////////

//Look for an hq tune
//This temporary measure allows someone to tune to the hq channel
static bool _tuneHQ(Player& player, UCHAR /*index*/)
{
	player.SetHqOff(false);
	return true;
}

//////////////////////////////////////////////////////////////////////

static bool _tuneOrigin(Player& player, UCHAR index)
{
	// Remove any pre-existing origin tune
	player.detuneChannelsByType(ORIGIN_CP_CHANNEL);
	// And now tune this one
	return player.tuneRadio(index, ORIGIN_CP_CHANNEL, 0);
}

//////////////////////////////////////////////////////////////////////

static bool _tuneGM(Player& player, UCHAR index)
{
	if ( !player.isGM() )
		return false;

	dbugLog(_FLI, "tuning %s to GM channel", player.callsign());

	if ( adminChannel == nullptr )
	{
		dbugLog(_FLW, "adminChannel is null");
		return false;
	}

	return player.tuneRadioToChannel(index, adminChannel, adminChannel->type());
}

//////////////////////////////////////////////////////////////////////
// Send a tune acknowledgment to the player

bool
Player::ackTuneHandle(UCHAR index, const ChatHandle handleID)
{
	Chat::TuneToChannel tune(index, PRIVATE_CHANNEL, handleID, true, false);
	if ( tune.send(teulIndex()) == false )
	{
		dbugLog(_FLN, "t:%d p:%u c:%s send ack for tune:%d failed", teulIndex(), playerID(), callsign(), index);
		detuneRadio(index);
		return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////
// Determine whether a named channel exists with the given name
// and whether the player has permissions to tune it

bool
Player::tuneRadioToNamedChannel(UCHAR index, const char* const channelName)
{
	if ( channelName == nullptr )
		return false;

	auto channelHandle = chatGetLowercaseHandle(channelName);
	auto it = std::find_if(_tuneCommands.begin(), _tuneCommands.end(),
			[=](const auto& cmd) { return cmd.matchHandle_ == channelHandle; });
	if ( it == _tuneCommands.end() )
		return false;

	const auto& tuneCommand = *it;

	detuneRadio(index);

	if ( !isPlaynet() && tuneCommand.perms_ && !permissions(tuneCommand.perms_) )
	{
		dbugLog(_FLW, "%s does not have permission to tune to %s"
						, callsign()
						, channelName);
		return false;
	}

	return tuneCommand.callback_(*this, index);
}

//////////////////////////////////////////////////////////////////////
// Expand a channel name and attempt to tune to the relevant
// channel or player.
// Handle can be one of:
// -- Named Channel (e.g. "HQ", "GM")
// -- Channel Number (1-99)
// -- Player callsign ('kfs1')

static inline bool _isANumber(const char* text) { while (isdigit(*text)) ++text; return (*text == 0); }

bool
Player::tuneRadioToHandle(UCHAR index, const ChatHandle handleID)
{
	if ( handleID == 0 )
	{
		detuneRadio(index);
		return true;
	}

	if ( noSide() )
	{
		detuneRadio(index);
		message(LSTR_CAUTION, LSTR_CHANNEL_UNAVAILABLE);
		return false;
	}

	if ( index >= MAX_CLIENT_CHANNELS )
	{
		dbugLog(_FLW, "t:%d p:%u c:%s attempting to tune index %d"
						, teulIndex(), playerID(), callsign()
						, index);
		message(LSTR_CAUTION, LSTR_CHANNEL_UNAVAILABLE);
		return false;
	}

	Chat::Name chanName(chatGetLowercaseHandle(handleID));

	if ( g_debugRadios )
		dbugLog(_FLD, "t:%d p:%u c:%s tuning #%d to %s(%" PR64BitUT ")"
						, teulIndex(), playerID(), callsign()
						, index
						, chanName.c_str(), handleID
						);

	//See if we are doing a 1-99 channel tune
	if ( _isANumber(chanName.c_str()) )
	{
		const UINT32 channelNo = atoi(chanName.c_str());
		if ( channelNo > 0 && channelNo <= CHAT_MAX_NUM_STATIONS )
		{
			if ( tuneRadioToNumberedChannel(index, channelNo) )
				return true;
		}
	}

	// Check to see if it's a tuning command (e.g. "HQ" or "GM")
	if ( tuneRadioToNamedChannel(index, chanName.c_str()) )
		return true;

	detuneRadio(index);
	message(LSTR_CAUTION, LSTR_CHANNEL_UNAVAILABLE);
	return false;
}

//////////////////////////////////////////////////////////////////////
// Tune a player to a numbered channel

bool
Player::tuneRadioToNumberedChannel(UCHAR index, UINT32 channelNo)
{
	if ( g_debugRadios )
		dbugLog(_FLD, "t:%d p:%u c:%s tuning #%d to ch#%d side %d"
						, teulIndex(), playerID(), callsign()
						, index
						, channelNo
						, side()
						);

	detuneRadio(index);

	const ChannelID& channelID = chatGetNumberedChannelID(0ULL, channelNo, side());

	Chat::Channel* const tuneChannel = _getChannel(channelID);
	if ( tuneChannel == nullptr )
	{
		dbugLog(_FLA, "t:%d p:%u c:%s tuning #%d to ch#%d side %d CREATE FAILED"
						, teulIndex(), playerID(), callsign()
						, index
						, channelNo
						, side()
						);
		return false;
	}

	return tuneRadioToChannel(index, tuneChannel, NUMBERED_CHANNEL);
}

//////////////////////////////////////////////////////////////////////
// Send a message to everyone in an HCUnit

struct BrigadeFilter : public FilterFn
{
	UINT32 m_brigadeID;
	BrigadeFilter(UINT32 brigadeID) : m_brigadeID(brigadeID) {}
	bool operator()(const wwiiPlayer* cur) const { return (cur->brigadeID() == m_brigadeID); }
};

static void
_distributeHCUnitMessage(Chat::Broadcast& broadcast)
{
	HCUnit* unit = HCUnit::Lookup(broadcast.m_targetID);
	if ( unit == nullptr || unit->country() <= WWII_NO_COUNTRY || unit->country() >= MAX_COUNTRIES )
		return;

	PLAYERS& side = sidePlayers[unit->side()];
	chatBroadcastLocMessage(side.begin(), side.end(), broadcast, BrigadeFilter(static_cast<UINT32>(broadcast.m_targetID)));
}

static void
_distributeSystemSideMessage(Chat::Broadcast& broadcast)
{
	WWII_SIDE forSide = (WWII_SIDE)broadcast.m_targetID;
	if ( forSide == ALLIED || forSide == WWII_NO_SIDE )
	{
		broadcast.m_prefix = LSTR_ALLIED;
		chatBroadcastLocMessage(sidePlayers[ALLIED].begin(), sidePlayers[ALLIED].end(), broadcast);
		wwiiIRCChannelMessage("-ahc,-hcs", chatLString(broadcast));
	}
	if ( forSide == AXIS || forSide == WWII_NO_SIDE )
	{
		broadcast.m_prefix = LSTR_AXIS;
		chatBroadcastLocMessage(sidePlayers[AXIS].begin(), sidePlayers[AXIS].end(), broadcast);
		wwiiIRCChannelMessage("-ghc,-hcs", chatLString(broadcast));
	}
}

static void
_distributeInstanceMessage(Chat::Broadcast& broadcast)
{
	Instance::HostInstance* inst = Instance::HostInstance::LookupByUID(broadcast.m_targetID);
	if ( inst != nullptr )
	{
		chatBroadcastLocMessage(inst->players().begin(), inst->players().end(), broadcast);
	}
}

//////////////////////////////////////////////////////////////////////

bool
Chat::Broadcast::route()
{
	switch ( m_type )
	{
	case _ADMINISTRATIVE:
		chatDispatchGMMessage(m_ptr[0], m_ptr[1]);
	break;

	case _HC:
		chatHCMessage((WWII_SIDE)m_targetID, *this, 0LL);
	break;

	case _HCUNIT:
	case _HCUNIT_DOWN:
	case _HCUNIT_UP:
		_distributeHCUnitMessage(*this);
	break;

	case _SIDE:
		_distributeSystemSideMessage(*this);
	break;

	case _MISSION:
		chatMessageMissionChannel(static_cast<UINT32>(m_targetID), *this);
	break;

	case _ORDERS:
		chatSetMissionOrders(static_cast<UINT32>(m_targetID), m_ptr[0], false);
	break;

	case _INSTANCE:
		_distributeInstanceMessage(*this);
	break;

	case _EVERYONE:
	default:
		wwiiIRCMessage("-sys", "%s", chatLString(*this));
		chatBroadcastLocMessage(playerIndex.begin(), playerIndex.end(), *this);
	break;
	}

	return true;
}

struct ObjectiveFilterFn : public FilterFn
{
	const FULL_OBJ_ID m_objective;
	ObjectiveFilterFn(const FULL_OBJ_ID objective) : m_objective(objective) {}
	bool operator()(const wwiiPlayer* wwiiPlayer) const
	{
		const Player* player = static_cast<const Player*>(wwiiPlayer);
		return ( player->objectiveCpID() == m_objective || player->originCpID() == m_objective );
	}
};

//////////////////////////////////////////////////////////////////////
// Send a message to a specific side and/or objective;
//  - Set objective to nullObjID to not limit by objective;
//	- arg1 and arg2 are optional

bool
chatRouteSideMessage(WWII_SIDE toSide
					, LSTRING_ID prefix
					, LSTRING_ID message
					, const FULL_OBJ_ID objectiveID
					, const char* const arg1
					, const char* const arg2
					)
{
	if ( playerIndex.empty() ) return false;

	Chat::LocalizedMessage lmsg(prefix, message, arg1, arg2);
	// Optimization: Determine if it is an "Objective" message first
	if ( objectiveID.id != 0 )
	{
		chatBroadcastLocMessage(sidePlayers[toSide].begin(), sidePlayers[toSide].end(), lmsg, ObjectiveFilterFn(objectiveID));
	}
	else if ( isValidSide(toSide) )
	{
		chatBroadcastLocMessage(sidePlayers[toSide].begin(), sidePlayers[toSide].end(), lmsg);
		if ( toSide == ALLIED )
			wwiiIRCChannelMessage("-ahc", chatLString(lmsg));
		else if ( toSide == AXIS )
			wwiiIRCChannelMessage("-ghc", chatLString(lmsg));
	}
	else
	{
		chatBroadcastLocMessage(playerIndex.begin(), playerIndex.end(), lmsg);
	}

	wwiiIRCChannelMessage("-hcs", chatLString(lmsg));

	return true;
}

//////////////////////////////////////////////////////////////////////

void
chatHandleTickSample(const UINT32 source, Player& player, const Teul::TickSample& sample)
{
	// TEMP/HACK: This message is occurring far too often, and it's a little
	// too late to fix it properly. For now, we're just going to ignore it.
	return;

#ifdef ENABLE_TICK_SAMPLES
	static std::map<playerid_t, std::pair<UINT32, TeulDeltaMS>> reports;
	auto it = reports.find(player.playerID());
	const UINT32 lastReport = (it != reports.end()) ? it->second.first : 0;
	const TeulDeltaMS lastDelta = (it != reports.end()) ? it->second.second : 0;

	if ( lastReport > teulTime() - 60 && std::abs(sample.delta - lastDelta) <= 120 )
		return;

	/* According to new types list: CONTROLLER_HOST = 0U, AUTH_HOST, DBD_HOST, STRAT_HOST, CHAT_HOST, MAP_HOST, CELL_HOST  */
	char hostname[64];
	char report[256];

	strprintf(hostname, "%shost:%s", g_serverTypes[source], player.callsign());
	strprintf(report, "Client clock anomaly: delta of %" PR64BitT "ms (%s), drift %.6g", sample.delta, sample.delta < 0 ? "fast" : "slow", sample.ratio);
	chatDispatchGMMessage(hostname, report);
	reports[player.playerID()] = std::make_pair(teulTime(), sample.delta);

	player.log(_FLW, "%s, host:%" PR64BitUT ", client:%" PR64BitUT "", report, sample.host_ms, sample.client_ms);

	strprintf(report , "{\"src\":\"%s\",\"elapsed\":{\"host\":%" PR64BitUT ",\"client\":%" PR64BitUT "},\"delta\":%" PR64BitUT ",\"ratio\",\"%0.8g\"}",
	          g_serverTypes[source],
	          sample.host_ms, sample.client_ms,
	          sample.delta, sample.ratio);
	chatdbRecordEvent("HACK", player.callsign(), "speedhack", report, "");
#endif
}

//////////////////////////////////////////////////////////////////////
//	this is the error callback for TCP connections

void
_errorFunction(SINT32 teulIndex, const UCHAR* const data, SINT32 /*size*/, const TEUL_CALLBACK* const /*object*/, SINT32 error_)
{
	UINT32 error = (UINT32) abs(error_);
	if ( error == ETEUL_ILLEGAL_TICKS )
	{
		auto* sample = (const Teul::TickSample*)(data);
		Player* player = PlayerInterface::Lookup(teulIndex);
		if ( player != nullptr )
		{
			chatHandleTickSample(CHAT_HOST, *player, *sample);
		}
		else
			dbugLog(_FLW, "t:%d clock drift host:%" PR64BitUT ", client:%" PR64BitUT ", delta:%" PR64BitT ", ratio:%.6g", teulIndex, sample->host_ms, sample->client_ms, sample->delta, sample->ratio);

		return;
	}

	if (error != ENOTCONN)
	{
		dbugLog(_FLD, "t:%d received %s (%d)", teulIndex, Teul::GetErrorName(error), error);
		return;
	}

	dbugLog(_FLD, "t:%d connection closed", teulIndex);
	Player* const player = PlayerInterface::Lookup(teulIndex);
	if ( player != nullptr )
	{
		// Player has lost their connection.
		player->setDisconnected();
	}
}

//////////////////////////////////////////////////////////////////////

void _killFunction(int /*signo*/)
{
	dbugLog(_FLN,"server exiting normally");

#ifdef STATS_LOGGING
	dbugLog(_FLI,"server stats");
	dbugLog(_FLI,"  messages handled: %d", _numMessages);
	dbugLog(_FLI,"  average message length: %5.2f", _avgMessageLength);
	dbugLog(_FLI,"  shortest Processing time: %d", _shortestProcessingTime);
	dbugLog(_FLI,"  longest Processing time: %d", _longestProcessingTime);
	dbugLog(_FLI,"  average Processing time: %5.2f", _avgProcessingTime);
#endif

	if ( ! _running )
		exit(1);

	_running = false;
}

//////////////////////////////////////////////////////////////////////

extern UINT32 _DEFAULT_MTU;
UINT32 _DEFAULT_MTU = Teul::TCPStdPayload;

void
_acceptPlayerConnection(ENDPOINT& endpoint)
{
	dbugLog(_FLN, "t:%d player connection from %s:%hu", endpoint.teulIndex(), teulGetAddress(endpoint.fromAddress()), endpoint.fromPort());
	endpoint.setNagleEnabled(true);
}

//////////////////////////////////////////////////////////////////////

static void
_listenErrorFunction(SINT32 teulIndex, const UCHAR* const /*data*/, SINT32 /*size*/, const TEUL_CALLBACK* const /*object*/, SINT32 error)
{
	if (error == ENOTCONN)
	{
//		chatLogoutPlayer(teulIndex);

		Player* player = PlayerInterface::Lookup(teulIndex);
		if ( player != nullptr )
		{
			dbugLog(_FLN, "t:%d p:%u c:%s closed", teulIndex, player->playerID(), player->callsign());
			chatPlayerDeactivate(player);
#ifdef PREVIOUSLY_IN_CHAT_HOST
			serverReleasePlayer(teulIndex, player->playerID(), player->OTP(), 0, ReleaseType::EConn);
#else
			player->Release(teulIndex, Server::ReleaseType::EConn);
#endif
		}
		else
			dbugLog(_FLN, "t:%d closed", teulIndex);

	}
	else
		dbugLog(_FLN, "t:%d error %d", teulIndex, error);
}

//////////////////////////////////////////////////////////////////////

void
_acceptManagementConnection(ENDPOINT& endpoint)
{
	dbugLog(_FLN, "t:%d management connection from %s:%hu", endpoint.teulIndex(), teulGetAddress(endpoint.fromAddress()), endpoint.fromPort());
}

//////////////////////////////////////////////////////////////////////

static void
_managementErrorFunction(SINT32 teulIndex, const UCHAR* const data, SINT32 /*size*/, const TEUL_CALLBACK* const /*object*/, SINT32 error)
{
	if (error == ENOTCONN)
	{
		chatLogoutPlayer(teulIndex);
	}
	else if ( error == ETEUL_ILLEGAL_TICKS )
	{
		auto* sample = (const Teul::TickSample*)(data);
		Player* player = PlayerInterface::Lookup(teulIndex);
		if ( player != nullptr )
		{
			dbugLog(_FLW, "t:%d p:%u c:%s clock drift host:%" PR64BitUT ", client:%" PR64BitUT ", delta:%" PR64BitT ", ratio:%.6g", player->teulIndex(), player->playerID(), player->callsign(), sample->host_ms, sample->client_ms, sample->delta, sample->ratio);
			chatHandleTickSample(CHAT_HOST, *player, *sample);
		}
	}
	else
		dbugLog(_FLN, "t:%d received error:%d", teulIndex, error);
}

//////////////////////////////////////////////////////////////////////

static SINT32
_personaCreate(const Server::PersonaLogin& login, PlayerStatusNode& psn)
{
	if ( login.data.auxData == nullptr )
	{
		dbugLog(_FLA, "t:%d p:%u c:%s personaCreate with null persXfer->data"
					, psn.teulIndex, psn.dispatch().m_customerID, psn.dispatch().m_callsign);
		return -EINVAL;
	}

	if ( g_debugPersonas )
		dbugLog(_FLD, "t:%d p:%u c:%s -> personaCreate reason %d"
						, psn.teulIndex, psn.dispatch().m_customerID, psn.dispatch().m_callsign
						, static_cast<int>(login.reason())
						);

	chatCreatePlayer(&psn, (struct Chat::PlayerInfo*) login.data.auxData);

	if ( g_debugPersonas )
		dbugLog(_FLD, "t:%d p:%u c:%s <- personaCreate"
						, psn.teulIndex, psn.dispatch().m_customerID, psn.dispatch().m_callsign);

	return 0;
}

//////////////////////////////////////////////////////////////////////

static SINT32
_personaRelease(SINT32 teulIndex, PlayerStatusNode* psn, Server::ReleaseType relaseType )
{
	dbugLog(_FLN, "t:%d p:%u c:%s _personaRelease type %d"
					, teulIndex
					, psn->dispatch().m_customerID
					, psn->dispatch().m_callsign
					, static_cast<int>(relaseType)
					);

	if ( psn->localInfo != nullptr )
	{
		dbugLog(_FLN, " *freeing");
		chatFreePlayer((Player*)psn->localInfo);
	}
	else
		dbugLog(_FLN, " no player to free");

	return 0;

}

//////////////////////////////////////////////////////////////////////

static SINT32
_personaActivate(SINT32 teulIndex, PlayerStatusNode* psn)
{
	Player* player = (Player*)psn->localInfo;
	if ( player == nullptr )
	{
		dbugLog(_FLA, "t:%d received null Player in PSN", teulIndex);
		return -Teul::Errors::LoginFail_ActivateNoLocalInfo;
	}

	try
	{
		player->activate(teulIndex, psn);
		return 0;
	}
	catch (std::exception &e)
	{
		dbugLog(_FLW, "t:%d p:%u c:%s client:%p activation error: %s"
						, player->teulIndex(), player->playerID(), player->callsign()
						, player
						, e.what()
						);
		return -Teul::Errors::LoginFail_ActivateFailed;
	}
}

//////////////////////////////////////////////////////////////////////

static SINT32
_personaExit(SINT32 teulIndex, PlayerStatusNode* psn, Server::PersonaExit& persExit)
{
	dbugLog(_FLN, "t:%d p:%u c:%s _personaExit (%p)", teulIndex, persExit.dispatch().m_customerID, persExit.dispatch().m_callsign, psn);
	return 0;
}

//////////////////////////////////////////////////////////////////////

static void
_personaErrorFunction (SINT32 teulIndex, const UCHAR* const /*data*/, SINT32 /*size*/, const TEUL_CALLBACK* const /*object*/, SINT32 error)
{
	dbugLog(_FLW, "t:%d e:%d personaErrorFunction", teulIndex, error); 	///TODO:134:REMOVE
}

//////////////////////////////////////////////////////////////////////
//Initializes all the global data structures, etc.

static void
_dataInit()
{
	chatInitPlayerLists();
}

//////////////////////////////////////////////////////////////////////

static SINT32
_serverEnterCallback(Server::ServerInfo* server, Server::Code scode)
{
	dbugLog(_FLN,"server %s added code %d, type %d",
			teulGetAddress(server->stats.address),
			static_cast<int>(scode), static_cast<int>(server->stats.type));

	switch ( server->stats.type )
	{
	case STRAT_HOST:
		_stratHost = server->stats.address & DIRECT_MASK;
		_stratPort = server->stats.port;
	break;
	default:
	break;
	}

	return 0;
 }

//////////////////////////////////////////////////////////////////////

static SINT32
_serverLeaveCallback(Server::ServerInfo* server, Server::Code scode)
{
	dbugLog(_FLN,"server %s left code %d, type %d",
			teulGetAddress(server->stats.address),
			static_cast<int>(scode), static_cast<int>(server->stats.type));

	switch ( server->stats.type )
	{
	case STRAT_HOST:
		_stratHost = 0;
		_stratPort = 0;
	break;
	default:
	break;
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////

static SINT32
_serverStatusCallback(Server::ServerInfo* server, Server::Code scode)
{
	dbugLog(_FLN,"server %s added code %d, type %d",
			teulGetAddress(server->stats.address),
			static_cast<int>(scode), static_cast<int>(server->stats.type));
	return 0;
}
//////////////////////////////////////////////////////////////////////
//This recounts the number of players on each side across the arena.
//	It should be run only very periodically, since this could take a while

static void _countUserSides()
{
	_sideCount[WWII_NO_SIDE] = 0;
	_sideCount[AXIS] = 0;
	_sideCount[ALLIED] = 0;
	_sideCount[NEUTRAL] = 0;

	PLAYER_INDEX::iterator it = playerIndex.begin();
	while ( it != playerIndex.end() )
	{
		Player* player = it->second;
		++it;

		_sideCount[player->side()]++;

		if ( player->squadInvite() && player->squadInvite()->expired() )
		{
			if ( g_debugSquads )
				dbugLog(_FLD, "t:%d p:%u c:%s squad invite expired", player->teulIndex(), player->playerID(), player->callsign());
			player->SetSquadInvite(nullptr);
		}
	}

	UINT32 newCount = _sideCount[ALLIED] + _sideCount[AXIS];
	Server::myServerInfo.stats.m_numPlayers = newCount;

	dbugLog(_FLN,"SIDE COUNT: %d:%" PRSizeT " axis, %d:%" PRSizeT " allied, %d:%" PRSizeT " neutral"
					, _sideCount[AXIS], sidePlayers[AXIS].size()
					, _sideCount[ALLIED], sidePlayers[ALLIED].size()
					, _sideCount[WWII_NO_SIDE], sidePlayers[WWII_NO_SIDE].size()
					);

	static UINT32 lastCount = 0;
	static UINT32 lastNotice = 0;
	UINT32 gap = 50;
	if ( lastCount >= 200 && newCount >= 200 )
		gap = 200;

	if ( (UINT32)std::abs((SINT32)(newCount - lastCount)) >= gap && lastNotice <= teulTime() - 300 )
	{
		PREP_CHAT_MSG(update, "Update: %u players [%u allied, %u axis]", newCount, _sideCount[ALLIED], _sideCount[AXIS]);
		wwiiIRCChannelMessage("-sys", update);
		lastNotice = teulTime();
		lastCount = newCount;
	}
}

//////////////////////////////////////////////////////////////////////

UINT32
chatGetStratAddr()
{
	return _stratHost;
}

//////////////////////////////////////////////////////////////////////
// Send a warning to all players

void
chatWarnAll(LSTRING_ID msg, const char* const ptr0, const char* const ptr1, const char* const ptr2, const char* const ptr3, const char* const ptr4, const char* const ptr5)
{
	Chat::LocalizedMessage lmsg(LSTR_WARNING, msg, ptr0, ptr1, ptr2, ptr3, ptr4, ptr5);
	wwiiIRCChannelMessage("-sys", chatLString(lmsg));
	chatBroadcastLocMessage(playerIndex.begin(), playerIndex.end(), lmsg);
}

//////////////////////////////////////////////////////////////////////
// Periodically check if there is spam waiting in the can
// Returns number of MS to wait until next spam poll

static UINT32
_manTheSpamCan()
{
	if ( spamLoops == 0 || theSpamCan.size() == 0 )
		return LONG_SPAM_INTERVAL_MS;

	if ( nextSpamLine >= theSpamCan.size() )
		nextSpamLine = 0;

	std::string message = "*";
	message += theSpamCan[nextSpamLine++];
	chatSystemMessage(LSTR_GENERIC, message.c_str());

	// If we reached end of the can, use the long interval
	if ( nextSpamLine >= theSpamCan.size() )
	{
		nextSpamLine = 0;
		--spamLoops;

		if ( spamLoops == 0 )
			theSpamCan.clear();

		return (spamInterval * 1000);
	}

	// Pause + 10ms per character
	return SHORT_SPAM_INTERVAL_MS + (UINT32)(message.length() * 10);
}

//////////////////////////////////////////////////////////////////////
// Periodically reload the default squad.

static void
_refreshDefaultSquad()
{
	if ( host_config.squad.defaultSquad[0] == 0 )
		return;

	dbugLog(_FLD, "requesting %s squad list", host_config.squad.defaultSquad);

	SquadList* recruitSquad = chatFindSquadListByName(host_config.squad.defaultSquad);
	if ( recruitSquad != nullptr )
	{
		chatRequestSquadMemberList(recruitSquad->squad().squadID());
	}
	else
	{
		dbugLog(_FLD, ": not loaded.");
	}
}

//////////////////////////////////////////////////////////////////////
// Periodic activities

extern void chatDeleteUnusedChannels();

static bool
_lowFrequencyHouseKeeping()
{
	chatDeleteUnusedChannels();

	static int tickCount = 0;

	// Less frequent activities
	++tickCount;
	if ( (tickCount % 10) == 0 )
		wwiiPlayer::logStats();

	bool reset = true;
	if ( (tickCount & 3) == 0 )
	{
		//Count up the side
		_countUserSides();
		chatSendPopulation();
	}
	else
		reset = false;

	if ( (tickCount & 15) == 0 )
	{
		chatLoadMotds();
		chatFetchSquadNames();
	}
	else
		reset = false;

	_refreshDefaultSquad();

	chatPurgeExpiredSquadRecruits();

	// If all the intervals line up, we can go back to zero
	if ( reset )
		tickCount = 0;

	return true;
}

//////////////////////////////////////////////////////////////////////

extern bool chatProcessPlayerGroups();

static bool
_houseKeeping()
{
	static int step = 0;

	static TeulTimeMS nextHouseKeep = 0ULL;
	if ( teulClock64 < nextHouseKeep )
		return false;

	// nullptr TERMINATED List of functions to call
	using HOUSEKEEPING = bool(*)();
	static HOUSEKEEPING housekeeping[] =
	{
		chatSaveLogRecords
	,	Chat::CommitProgressUpdates
	,	Chat::CommitDecalUpdates
	,	chatHousekeepGroupPermissions
	,	chatProcessPlayerGroups
	,	chatCheckDecisionQueues
	,	chatCheckSquadLists
	,	chatExpireSquadCreates
	,	chatGridSnapshot
	,	chatPatrolBrigadeElections

	// Insert new entries here

	,	_lowFrequencyHouseKeeping
	,	nullptr
	};

	if ( housekeeping[step] == nullptr )
	{
		step = 0;
		nextHouseKeep = teulClock64 + (30 * 1000);		// Every 30s.
		return false;
	}

	// The function must return true to indicate it's done
	if ( housekeeping[step]() )
		step++;

	return true;
}

//////////////////////////////////////////////////////////////////////

static void
_startShutdown()
{
	DBThread::Do(DBThread::Default, "UPDATE host_config SET value = '3' WHERE variablename = 'arenaLock'");

	chatdbRecordEvent("SYS", g_shutdownRequestedBy.c_str(), "shutdown", "NOW", "executing shutdown");

	chatDistributeLocMessage(playerIndex.begin(), playerIndex.end(), LSTR_WARNING, LSTR_SERVER_SHUTDOWN_NOW);

	wwiiIRCChannelMessage("-sys,#servers", "*Automated shutdown");

	std::this_thread::sleep_for(std::chrono::microseconds(500));

	PLAYER_INDEX::iterator it = playerIndex.begin();
	while ( it != playerIndex.end() )
	{
		Player* target = it->second;
		++it;

		chatSmitePlayer(target->playerID());
		target->Release(target->teulIndex(), Server::ReleaseType::Exit);
	}

	serverBroadcastClusterShutdown();

	if ( system(PN_PLAYNET_PATH "/bin/stophost &") < 0 )
		LogAbort("system " PN_PLAYNET_PATH "/bin/stophost & failed");
}

//////////////////////////////////////////////////////////////////////

static bool
_checkForShutdown()
{
	static const UINT32 _SHUTDOWN_LOCK_DURATION = 30;
#ifdef BETA_RELEASE
	static const SINT32 MIN_SHUTDOWN_TIME_S = 1 * 60;		// 1 minute
#else
	static const SINT32 MIN_SHUTDOWN_TIME_S = 5 * 60;		// 5 minutes
#endif

	// Note: 'false' below is the initial value, not an assigned value
	static bool p_shutdownImminent = false;
	static UINT32 p_lastNotice = 0;

	// Has shutdown been requested?
	if ( g_shutdownLockAt == 0 && p_shutdownImminent == false )
		return false;

	if ( g_shutdownLockAt == 0 )
	{
		p_shutdownImminent = false;
		p_lastNotice = 0;
		g_shutdownLockedArena = 0;
		g_finalShutdownTime = 0;
		g_shutdownDowntime = 0;
		return false;
	}

	UINT32 now = teulTime();
	SINT32 remaining = (SINT32)g_shutdownLockAt - (SINT32)now;

	if ( p_shutdownImminent == false && remaining < MIN_SHUTDOWN_TIME_S )
		remaining = MIN_SHUTDOWN_TIME_S;

	if ( remaining > 0 || p_shutdownImminent == false )
	{
		// Round the number to something nice
		remaining += 5;
		remaining -= remaining % 15;

		// Have we started the shutdown process?
		if ( p_shutdownImminent == false )
		{
			g_shutdownLockAt = now + remaining;
			p_shutdownImminent = true;
			p_lastNotice = now;
			return false;
		}

		// Over an hour, only repeat notices every 15 minutes.
		if ( remaining > 60 * 60 && p_lastNotice + 15 * 60 > now )
			return false;
		// Over 5 minutes, only repeat notices every 5 minutes.
		if ( remaining > 5 * 60 && p_lastNotice + 5 * 60 > now )
			return false;
		// Over 60 seconds, only repeat once a minute.
		if ( remaining >= 60 && p_lastNotice + 60 > now )
			return false;
		else if ( p_lastNotice + 15 > now )
			return false;

		if ( remaining > 60 )
			remaining -= remaining % 60;

		if ( remaining > 15 * 60 )
			chatSystemMessage(LSTR_GAME_COMING_DOWN_IN, chatTimeInterval(remaining));
		else
			chatWarnAll(LSTR_GAME_COMING_DOWN_IN, chatTimeInterval(remaining));

		if ( remaining >= 60 && g_shutdownDowntime > 0 )
		{
			chatSystemMessage(LSTR_EXPECTED_DOWNTIME, chatTimeInterval(g_shutdownDowntime*60));
		}

		p_lastNotice = now;

		dbugLog(_FLW, "%s until scheduled shutdown for %s.", chatTimeInterval(remaining), g_shutdownRequestedBy.c_str());

		return false;
	}

	// If we haven't already scheduled a final shutdown time.
	if ( g_finalShutdownTime == 0 )
		g_finalShutdownTime = teulTime() + _SHUTDOWN_LOCK_DURATION;

	// Time is up.
	if ( teulTime() >= g_finalShutdownTime )
	{
		_startShutdown();
		return true;
	}

	// Time is up; lock out new players, new spawns, new captures,
	// but give players a few moments to despawn. Repeat the
	// warning every 5 seconds.
	if ( g_shutdownLockedArena + 5 > teulTime() )
		return false;

	OOBMessage message(0, 0, systemHandleID, "arenalock", "15", nullObjID, WWII_NO_SIDE, WWII_NO_COUNTRY);
	message.send();
	g_shutdownLockedArena = teulTime();

	char buf[16];
	strprintf(buf, "%2u", g_finalShutdownTime - teulTime());
	chatWarnAll(LSTR_SHUTDOWN_NOW, buf);

	return false;
}

//////////////////////////////////////////////////////////////////////
// Handle messages coming in from IRC and check to see if they are
// special channel messages
// Returns true if the message is processed or ignored.

class IRC_HCMessage
{
public:
	IRC_HCMessage(WWII_SIDE side, const char* nick, const char* args)
		: m_side(side), m_nick(nick), m_args(args) {}

	WWII_SIDE m_side;
	std::string m_nick;
	std::string m_args;
};

static void
_processIRCHCMessage(void* v)
{
	IRC_HCMessage* msg = (IRC_HCMessage*)v;

	const char* nick = msg->m_nick.c_str();
	const char* arg = msg->m_args.c_str();

	switch ( msg->m_side )
	{
	case NEUTRAL:
		if ( arg[0] == '+' )
		{
			chatSetHCMotd(nick, arg + 1, ALLIED);
			chatSetHCMotd(nick, arg + 1, AXIS);
		}
		else
		{
			// Both sides
			chatDispatchHCText(ALLIED, nick, arg);
			chatDispatchHCText(AXIS, nick, arg);
		}
	break;
	case ALLIED:
		if ( arg[0] == '+' )
			chatSetHCMotd(nick, arg + 1, ALLIED);
		else
			chatDispatchHCText(ALLIED, nick, arg);
	break;
	case AXIS:
		if ( arg[0] == '+' )
			chatSetHCMotd(nick, arg + 1, AXIS);
		else
			chatDispatchHCText(AXIS, nick, arg);
	break;
	default:
	break;
	}

	delete msg;
}

static bool
_handleIRCChannelMessage(irc_session_t* session, const char* /*event*/, const char* origin, const char** params, UINT32 count)
{
	if ( origin[0] == IRC::_SERVER_PREFIX )
		return true;
	if ( count < 1 || params[0][0] != '#' || (count > 1 && params[1][0] == '.' && params[1][1] != '.' ) )
		return false;

	// We're ignoring this channel, so it is "processed".
	IRC* irc = (IRC*)irc_get_ctx(session);
	if ( irc->ignoring(params[0]) == true )
		return true;

	const char* p = strchr(params[0], '-');
	if ( p == nullptr )
		return false;
	++p;
	if ( *p == 0 )
		return false;

	WWII_SIDE forSide = WWII_NO_SIDE;
	if ( strcmp(p, "hcs") == 0 )
		forSide = NEUTRAL;
	else if ( strcmp(p, "ahc") == 0 )
		forSide = ALLIED;
	else if ( strcmp(p, "ghc") == 0 )
		forSide = AXIS;
	else
		return false;

	char nick[32];
	nick[0] = '\0';
	irc_target_get_nick(origin, nick, 31);

	strupr(nick);

	IRC_HCMessage* msg = new IRC_HCMessage(forSide, nick, params[1]);
	threadCallback(_processIRCHCMessage, msg);

	return false;
}

static void
_ircChannelMessages(irc_session_t* session, const char* event, const char* origin, const char** params, unsigned int count)
{
	if ( _handleIRCChannelMessage(session, event, origin, params, count) == false )
		wwiiIRCDefaultPrivMsg(session, event, origin, params, count);
}

//////////////////////////////////////////////////////////////////////
// Trim an IRC name down to a simple username.

static bool
_extractUserName(const char* ircName, std::string& name)
{
	name = "";

	// Is it telling us they are an operator?
	if ( ircName[0] == '@' )
		++ircName;

	// Is this a server?
	if ( ircName[0] == IRC::_SERVER_PREFIX )
		return false;

	name = ircName;

	// Username format is '[username!]login@host' or just 'username'
	size_t symbolPos = name.find('!');
	if ( symbolPos == std::string::npos )
		symbolPos = name.find('@');
	if ( symbolPos != std::string::npos )
		name.resize(symbolPos);

	return true;
}

//////////////////////////////////////////////////////////////////////
// Manage the list of IRC Players.

using IRC_PLAYERS = std::map<ChatHandle, IRCPlayer*>;
static IRC_PLAYERS ircPlayers;

static IRCPlayer*
_lookupIRCPlayer(const char* gamename)
{
	ChatHandle matchHandle = chatGetLowercaseHandle(gamename);
	IRC_PLAYERS::iterator it = ircPlayers.find(matchHandle);
	return ( it == ircPlayers.end() ) ? nullptr : it->second;
}

//////////////////////////////////////////////////////////////////////
// When an IRC player gets deleted, remove them from the list.

IRCPlayer::~IRCPlayer()
{
	IRC_PLAYERS::iterator it = ircPlayers.find(matchHandle());
	if ( it != ircPlayers.end() )
		ircPlayers.erase(it);
}

//////////////////////////////////////////////////////////////////////
// Manage channel lists in IRCPlayer objects

void
IRCPlayer::AddChannel(const char* _channel)
{
	m_channels.insert(_channel);
}

UINT32
IRCPlayer::RemoveChannel(const char* _channel)
{
	if ( _channel[0] == '*' )
		m_channels.clear();
	else
		m_channels.erase(_channel);
	return (UINT32)m_channels.size();
}

struct IRCRegistration
{
	enum _TYPE { _ADD, _REMOVE, _NICK };

	IRCRegistration(_TYPE type, const std::string& username, const char* channel)
		: m_type(type), m_username(username), m_channel(channel)
		{}

	_TYPE m_type;
	std::string m_username, m_channel;

	void Add(IRCPlayer* ircPlayer);
	void Remove(IRCPlayer* ircPlayer);
	void Nick(IRCPlayer* ircPlayer);
};

static void
_processIRCRegistration(void* v)
{
	IRCRegistration& reg = *(IRCRegistration*)(v);

	IRCPlayer* ircPlayer = _lookupIRCPlayer(reg.m_username.c_str());
	switch ( reg.m_type )
	{
	case IRCRegistration::_ADD:
		reg.Add(ircPlayer);
	break;

	case IRCRegistration::_REMOVE:
		reg.Remove(ircPlayer);
	break;

	case IRCRegistration::_NICK:
		reg.Nick(ircPlayer);
	break;
	}
}

void
IRCRegistration::Add(IRCPlayer* ircPlayer)
{
	dbugLog(_FLI, "irc:%s:joined %s", m_username.c_str(), m_channel.c_str());

	if ( ircPlayer == nullptr )
	{
		Player* player = PlayerInterface::Lookup(m_username.c_str());
		if ( player != nullptr )
		{
			dbugLog(_FLA, "irc:cannot register irc user %s, user is logged in", m_username.c_str());
			return;
		}

		ircPlayer = new IRCPlayer(m_username.c_str());
		if ( ircPlayer == nullptr )
		{
			dbugLog(_FLA, "irc:failed to create irc user '%s'", m_username.c_str());
			return;
		}

		ircPlayers[ircPlayer->matchHandle()] = ircPlayer;
	}

	ircPlayer->AddChannel(m_channel.c_str());
}

void
IRCRegistration::Remove(IRCPlayer* ircPlayer)
{
	if ( ircPlayer == nullptr )
		return;

	dbugLog(_FLI, "irc:%s:left %s", m_username.c_str(), m_channel.c_str());

	// Removing channels from a player.
	if ( ircPlayer->RemoveChannel(m_channel.c_str()) == 0 )
	{
		dbugLog(_FLD, "irc:%s:exit (no channels)", m_username.c_str());
		chatFreePlayer(ircPlayer);
	}
}

void
IRCRegistration::Nick(IRCPlayer* ircPlayer)
{
	if ( ircPlayer == nullptr )
		return;

	const char* newUsername = m_channel.c_str();

	if ( strcasecmp(newUsername, m_username.c_str()) == 0 )
		return;

	// Clone the existing user's channel list.
	IRCPlayer::IRC_CHANNELS channels = ircPlayer->channels();

	dbugLog(_FLD, "irc:%s:exit (changing nick to %s)", m_username.c_str(), newUsername);

	// Free up the old player
	chatFreePlayer(ircPlayer);

	if ( chatIsValidCallsign(newUsername) == false )
	{
		dbugLog(_FLW, "irc:cannot register new nick %s, invalid gamename", newUsername);
		return;
	}

	Player* player = PlayerInterface::Lookup(newUsername);
	if ( player != nullptr )
	{
		dbugLog(_FLA, "irc:cannot register new nick %s, user is logged in", newUsername);
		return;
	}

	ircPlayer = _lookupIRCPlayer(newUsername);
	if ( ircPlayer != nullptr )
	{
		dbugLog(_FLA, "irc:cannot register new nick %s, irc user exists", newUsername);
		return;
	}

	try
	{
		ircPlayer = new IRCPlayer(newUsername);
		if ( ircPlayer == nullptr )
		{
			dbugLog(_FLA, "irc:failed registering new nick %s, internal error", newUsername);
			return;
		}

		ircPlayers[ircPlayer->matchHandle()] = ircPlayer;
	}
	catch ( std::exception& e )
	{
		dbugLog(_FLA, "irc:failed registering new nick %s, %s", newUsername, e.what());
		return;
	}

	ircPlayer->SetChannels(channels);

	std::string channelList = "";
	IRCPlayer::IRC_CHANNELS::const_iterator it = ircPlayer->channels().begin();
	while ( it != ircPlayer->channels().end() )
	{
		channelList += " ";
		channelList += *it;
		++it;
	}

	dbugLog(_FLI, "irc:%s:nick (from %s) channels:%s", ircPlayer->callsign(), m_username.c_str(), channelList.c_str());
}

//////////////////////////////////////////////////////////////////////
// Register an IRC user as known for being on a specific channel.

static void
_registerIrcUser(const char* ircName, const char* channel)
{
	std::string name = "";
	if ( !_extractUserName(ircName, name) )
	{
		return;
	}

	if ( chatIsValidCallsign(name.c_str()) == false )
	{
		return;
	}

	IRCRegistration* reg = new IRCRegistration(IRCRegistration::_ADD, name, channel);
	threadCallback(_processIRCRegistration, reg);
}

//////////////////////////////////////////////////////////////////////
// IRC user is leaving.

static void
_unregisterIrcUser(const char* ircName, const char* channel)
{
	std::string name = "";
	if ( !_extractUserName(ircName, name) )
		return;

	if ( chatIsValidCallsign(name.c_str()) == false )
		return;

	IRCRegistration* reg = new IRCRegistration(IRCRegistration::_REMOVE, name, channel);
	threadCallback(_processIRCRegistration, reg);
}

//////////////////////////////////////////////////////////////////////
// Intercept join messages to see people joining our channels.

static void
_ircJoinMessages(irc_session_t* session, const char* event, const char* origin, const char** params, unsigned int count)
{
	// Make sure it gets logged.
	wwiiIRCDefaultJoin(session, event, origin, params, count);

	// Needs to be at least one argument -- channel name.
	if ( count < 1 )
		return;

	_registerIrcUser(origin, params[0]);
}

//////////////////////////////////////////////////////////////////////
// User leaving a channel

static void
_ircPartMessages(irc_session_t* session, const char* event, const char* origin, const char** params, unsigned int count)
{
	wwiiIRCDumpEvent(session, event, origin, params, count);

	if ( count < 1 )
		return;

	// I [Sun 1/10 04:54.22.129 wwiiIRC.cpp:454] IRC e:PART o:kfsone_home!osmith@shivers.playnet.com p:2[#beta-sys|Leaving]
	_unregisterIrcUser(origin, params[0]);
}

//////////////////////////////////////////////////////////////////////
// Take a list of people in a channel and register them as users.

static void
_registerUsers(const char* channel, const char* names)
{
	char buf[64];
	while ( names && *names )
	{
		const char* p = strchr(names, ' ');
		if ( p == nullptr )
		{
			// Last name in the list
			_registerIrcUser(names, channel);
			return;
		}

		size_t len = (size_t)(p - names);
		if ( len >= sizeof(buf) )
			len = sizeof(buf) - 1;

		strncpy(buf, names, len);
		buf[len] = 0;

		_registerIrcUser(buf, channel);

		names = p + 1;
	}
}

//////////////////////////////////////////////////////////////////////
// Intercept numeric channel messages

static void
_ircNumericMessages(irc_session_t * session, unsigned int event, const char* origin, const char** params, unsigned int count)
{
	wwiiIRCDefaultNumeric(session, event, origin, params, count);

	switch ( event )
	{
	case 353:
		// User list on joining a channel
		// Params should be { Recipient = channel name ... name }
		if ( count > 3 && strcmp(params[1], "=") == 0 )
			_registerUsers(params[2], params[3]);
		return;

	default:
	break;
	}
}

//////////////////////////////////////////////////////////////////////
// Handle a nick change.

static void
_ircNickMessages(irc_session_t* session, const char* event, const char* origin, const char** params, unsigned int count)
{
	wwiiIRCDumpEvent(session, event, origin, params, count);

	if ( count < 1 )
		return;

	std::string name = "";
	if ( !_extractUserName(origin, name) )
		return;

	const char* newName = params[0];

	// Purge all the existing channels.
	IRCRegistration* reg = new IRCRegistration(IRCRegistration::_NICK, name, newName);
	threadCallback(_processIRCRegistration, reg);
}

//////////////////////////////////////////////////////////////////////
// Execute a dot command as though it was from a real player

class IRC_DotPassthru
{
public:
	IRC_DotPassthru(const char* command, const char* from, std::string args, const char* tell)
		: m_command(command), m_from(from), m_args(args), m_tell(tell) {}
	std::string m_command;
	std::string m_from;
	std::string m_args;
	std::string m_tell;
};

static void
_processIRCDotPassthru(void* v)
{
	IRC_DotPassthru* data = (IRC_DotPassthru*)v;
	const char* command = data->m_command.c_str();
	const char* from = data->m_from.c_str();
	const char* args = data->m_args.c_str();
	const char* tell = data->m_tell.c_str();

	IRCPlayer* ircPlayer = _lookupIRCPlayer(from);
	if ( ircPlayer == nullptr )
	{
		wwiiIRCMessage(tell, "Sorry, %s, you cannot use '.' commands unless you are on at least one channel with me.", from);
		return;
	}

	ircPlayer->SetLastChannel(tell);

	static Chat::ClientMessage fakeMessage;

	fakeMessage.m_handle = ircPlayer->chatHandle();
	fakeMessage.m_length = strprintf(fakeMessage.m_message, "/%s %s", command, args);

	if ( ! DotCommand::index()->process(*ircPlayer, &fakeMessage) )
		wwiiIRCMessage(tell, "Oops, had an internal error processing .%s for %s", command, from);
}

static void
_ircdotPassthru(Host::CommandMsg& iCmd)
{
	if ( !host_config.arena.enableIRCCommands )
	{
		wwiiIRCMessage(iCmd.Tell(), "Sorry, %s, '.' commands are disabled on this cluster at the moment.", iCmd.From());
		return;
	}

	IRC_DotPassthru* data = new IRC_DotPassthru(iCmd.Command(), iCmd.From(), iCmd.Input(), iCmd.Tell());
	threadCallback(_processIRCDotPassthru, data);
}

//////////////////////////////////////////////////////////////////////
// Handle a system message - check to see if it is on #servers

static void
_ircdotSys(Host::CommandMsg& iCmd)
{
	// If its on the servers channel, announce it boldly
	if ( strcmp(iCmd.Tell(), "#servers") == 0 )
	{
		IRC_Command modified = (IRC_Command&)iCmd;
		modified.input.insert(0, "[ALL SERVERS] ");
		_ircdotPassthru(modified);
	}
	else
		_ircdotPassthru(iCmd);
}

//////////////////////////////////////////////////////////////////////
// Central host loop.

static void
_runHost()
{
	while ( _running )
	{
		// Process network updates.
		teulUpdateTransport(25);

		threadsCallNextCallback();

		// Catch up on housekeeping.
		while ( _houseKeeping() )
			;

		chatSendMissionUpdates();
		wwiiExpireSTOs();

		// Check to see if the spam can should run now
		// (it has finer than 30s/tick granularity)
		if ( PN_UNLIKELY(teulClock64 > nextSpamTime) )
		{
			nextSpamTime = teulClock64 + _manTheSpamCan();
		}

		chatUpdateHCUnitLocations();
		chatCheckSpawningPlayers();
		chatUpdateAWSSnapshot();
		chatPatrolGridExpirations();

#ifdef KICK_BANS
		_processDelayedBan();
#endif

		if ( _running && _checkForShutdown() )
			_running = false;
	}
}

UINT32 serverStartTime;

UINT32 serverUpTimeMinutes()
{
        UINT32 secs = teulTime() - serverStartTime;
        return secs / 60;
}

//////////////////////////////////////////////////////////////////////
//

const char* const g_serverName = "chatd";

int
main(int argc, const char* argv[])
{
	Server::InitializeProcess(g_serverName, CHAT_HOST, _errorFunction, _killFunction, argc, argv, {});
	dbugSetWriteLevel(DEF_ERR_LEVEL | DEF_ERR_FLAGS);
	dbugSetLogLevel(DEF_ERR_LEVEL | DEF_ERR_FLAGS);

	chatReadHostLocFile();

	wwiiIRCInitialize(Server::localHost, false);

	ircCallbacks.event_privmsg = _ircChannelMessages;
	ircCallbacks.event_channel = _ircChannelMessages;
	ircCallbacks.event_numeric = _ircNumericMessages;
	ircCallbacks.event_join    = _ircJoinMessages;
	ircCallbacks.event_part    = _ircPartMessages;
	ircCallbacks.event_nick    = _ircNickMessages;

	wwiiIRCAddInitialChannel("-hcs", "", "In-Game /hc channel (both sides)");
	wwiiIRCAddInitialChannel("-ghc", "", "In-Game Allied HC channel");
	wwiiIRCAddInitialChannel("-ahc", "", "In-Game Axis HC channel");

	wwiiIRCRegisterCommand("all", _ircdotPassthru);
	wwiiIRCRegisterCommand("allied", _ircdotPassthru);
	wwiiIRCRegisterCommand("arenalock", _ircdotPassthru);
	wwiiIRCRegisterCommand("axis", _ircdotPassthru);
	wwiiIRCRegisterCommand("cpinfo", _ircdotPassthru);
	wwiiIRCRegisterCommand("crs", _ircdotPassthru);
	wwiiIRCRegisterCommand("gm", _ircdotPassthru);
	wwiiIRCRegisterCommand("hc", _ircdotPassthru);
	wwiiIRCRegisterCommand("hclist", _ircdotPassthru);
	wwiiIRCRegisterCommand("pinfo", _ircdotPassthru);
	wwiiIRCRegisterCommand("rdp", _ircdotPassthru);
	wwiiIRCRegisterCommand("roster", _ircdotPassthru);
	wwiiIRCRegisterCommand("setmotd", _ircdotPassthru);
	wwiiIRCRegisterCommand("shutdown", _ircdotPassthru);
	wwiiIRCRegisterCommand("spam", _ircdotPassthru);
	wwiiIRCRegisterCommand("sys", _ircdotSys);
	wwiiIRCRegisterCommand("sysmotd", _ircdotPassthru);
	wwiiIRCRegisterCommand("warnall", _ircdotPassthru);
	wwiiIRCRegisterCommand("m", _ircdotPassthru);
	wwiiIRCRegisterCommand("who", _ircdotPassthru);
	wwiiIRCRegisterCommand("wtf", _ircdotPassthru);

	chatThreadsSetup();

	// Initialize dbse callbacks
	chatdbInstallClientCallbacks();
	if ( !dbseInit() )
		return FALSE;

	// Connect to the database
	chatInitDB();

	dbGetWWIIConfig();
	specLoader.loadSpecMap();

	chatSetUnitMotd(SYSTEM_MOTD_UNIT, DEFAULT_SYS_MOTD);
	chatSetUnitMotd(ALLIED, DEFAULT_ALLIED_HC_MOTD);
	chatSetUnitMotd(AXIS, DEFAULT_AXIS_HC_MOTD);

	//Initialize all our data
	_dataInit();

	wwiiIRCActivate(true);

	//Create all the static channels that exist All the time
	adminChannel = chatCreateChannel(chatGetAdminChannelID(), "GM Channel", chatGetHandle("GM"));
	chatCreateChannel(chatGetGCIChannelID(Instance::g_campaign.id(), ALLIED), "Allied GCI Channel", chatGetHandle("Allied"));
	chatCreateChannel(chatGetGCIChannelID(Instance::g_campaign.id(), AXIS), "Axis GCI Channel", chatGetHandle("Axis"));

	serverOpenBroadcastIndexes();

	Server::myServerInfo.stats.port = WWII::Net::Ports::ChatHost;

	serverInit(HOST_VERSION
				, CHAT_HOST, CHAT_HOST
				, _serverEnterCallback
				, _serverLeaveCallback
				, _serverStatusCallback
				, _personaCreate
				, _personaRelease
				, _personaActivate
				, _personaExit
				, nullptr					// Don't care about cluster exits
				, _personaErrorFunction);

	Server::myServerInfo.flags = Server::SVRFLAG_RELOGIN;//SVRFLAG_SEND_PSTAT;
	Server::myServerInfo.stats.m_maxPlayers = Teul::MaxClusterUsers;

	ABORT_IF(chatInstallHostCallbacks() < 0);
	ABORT_IF(chatInstallHostManagementCallbacks() < 0);

	//End broadcast configuration

	//Find map server

	if ( !teulOpenListenInterface(WWII::Net::Ports::ChatHost, _acceptPlayerConnection, 45000, _listenErrorFunction) )
		LogAbort("Opening player connection ports failed.");

	dbugLog(_FLN, "Listening for players on port %u", WWII::Net::Ports::ChatHost);

	const SINT32 managementListenInterface = teulOpenListen(WWII::Net::Ports::ChatManagement, _acceptManagementConnection, 20000, PROTOCOL_TCP, _managementErrorFunction);
	ABORT_IF(managementListenInterface < 0);

	dbugLog(_FLN, "Listening for management players on port %u", WWII::Net::Ports::ChatManagement);

	Server::myServerInfo.stats.status = Server::SERVER_ACCEPTING;
	Server::myServerInfo.sync();

	_countUserSides();
	chatSendPopulation();

	teulSyncClock();

	nextSpamTime = teulClock64 + 1000;

	serverStartTime = teulTime();

	// Load the MOTDs in
	chatLoadMotds();
	chatFetchSquadNames();

	// Enable any threads that were waiting to be allowed
	chatThreadsEnable();

	dbugLog(_FLI, "registering chat commands");

	chatRegisterDotCommands();

	// Now we can register the dot-commands; permission data
	// is taken from the database so register after connecting
	try
	{
		DotCommandIndex& index = *DotCommand::index();
		index.addAlias("whisper", "w");
		index.addAlias("m", "tell");
		index.addAlias("m", "t");
		index.addAlias("m", "r");

		dbugLog(_FLI, "DotIndex contains %" PRSizeT " commands", index.size());

		index.readPermissions();
	}
	catch (std::exception& e)
	{
		dbugLog(_FLA, "failed registering dot commands: %s", e.what());
		exit(0);
	}

	wwiiIRCChannelMessage("-sys,#servers", "++ Running ++");

	_runHost();

	teulClose(managementListenInterface);

	wwiiIRCTerminate("[[ Normal shutdown ]]");

	chatThreadsShutdown();

	teulCloseTransport();
}
