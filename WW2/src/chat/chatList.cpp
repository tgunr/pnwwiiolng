#include <syscfg.h> // Path is project specific.
// Date: Wed Jul 26 17:06:08 2000
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

//#define _GNU_SOURCE

#include "chat/chatConstants.h"
#include "chat/chatFriendList.h"
#include "chat/chatHost.h"
#include "chat/chatHostMessages.h"
#include "chat/chatIRCPlayer.h"
#include "chat/chatList.h"
#include "chat/chatManagementIDs.h"
#include "chat/chatManagementPlayer.h"
#include "chat/chatManagementTypes.h"
#include "chat/chatPlayer.h"
#include "chat/chatSaveChannels.h"
#include "server/serverPlayerStatusNode.h"
#include "server/serverTypes.h"
#include "teul/teulBasic.h"
#include "teul/teulClosingEnums.h"
#include "teul/teulPacket.h"
#include "wwii/wwiiThreads.h"
#include "wwii/wwiiThreadText.h"

UINT32 _sideCount[MAX_SIDES];

PLAYERS commandList;

PLAYER_INDEX playerIndex;
static PLAYERS s_limboPlayers;					// Indexed by playerID tho
PLAYERS sidePlayers[MAX_SIDES];

static void _registerPlayer(Player* player);

// Add a player to the playerIndex
static bool
_indexPlayer(Player* player)
{
	if ( player == nullptr )
		LogAbort("Tried to map NULL player");

	dbugLog(_FLD, "t:%d p:%u c:%s indexing matchid:%llx", player->teulIndex(), player->playerID(), player->callsign(), player->matchHandle());

	// hash_map doesn't provide lower_bound, so we can't insert-at-place
	PLAYER_INDEX::iterator it = playerIndex.find(player->matchHandle());

	// If it's already in the list, we may either already be registered (bug),
	// have an empty record (bug) or be displacing a previous instance (normal)
	if ( it != playerIndex.end() )
	{
		if ( it->second == player )
		{
			// This isn't expected to happen
			dbugLog(_FLW, "t:%d p:%u c:%s already in list", player->teulIndex(), player->playerID(), player->callsign());
			return true;
		}

		if ( it->second == nullptr )
		{
			// Maybe we should FLF because clearly data is garbled
			dbugLog(_FLA, "t:%d p:%u c:%s NULL player pointer in playerIndex", player->teulIndex(), player->playerID(), player->callsign());
		}
		else
		{
			// If you are re-logging, this may happen
			dbugLog(_FLN, "t:%d p:%u c:%s freeing prior instance", player->teulIndex(), player->playerID(), player->callsign());
			chatFreePlayer(it->second);
		}

		it->second = player;

		return true;
	}

	// Otherwise add a new entry
	playerIndex.insert(std::make_pair(player->matchHandle(), player));

	return true;
}

void
chatInitPlayerLists(void)
{
	_sideCount[WWII_NO_SIDE] = 0;
	_sideCount[AXIS] = 0;
	_sideCount[ALLIED] = 0;

	Chat::ManagementIDAllocator::Initialize();
}

void
Player::activate(SINT32 newIndex, PlayerStatusNode* psn)
{
	// Are we re-activating?
	if ( active() )
	{
		if ( PlayerInterface::Lookup(matchHandle()) != this )
			throw std::logic_error("lookup failed during reactivation");

		// Clobber the old instance
		teulSetPersonaData(teulIndex(), nullptr);
		chatPlayerDeactivate(this);
		dbugLog(_FLN, "t:%d p:%u c:%s reactivated active player %p on teulIndex %d"
						, teulIndex(), playerID(), callsign(), this, newIndex);
	}

	m_teulIndex = newIndex;

	s_limboPlayers.erase(this);

	m_active = true;

	_indexPlayer(this);

	dbugLog(_FLI, "t:%d p:%u c:%s otp:%u ip:%s activating", teulIndex(), playerID(), callsign(), OTP(), teulGetAddress(dispatch().m_clientIP));

	if ( isManager() )
		return;

	Teul::Packet outgoing(Teul::PacketId(CHAT_X, Chat::MAP_REFRESH_PLAYER_Y), playerID());
	outgoing.setUnencrypted();
	serverSendPacketToType(outgoing, MAP_HOST);

	// If they're done loading, add them to the command list
	readGroups();

	if ( psn )
		psn->localInfo = this;

	// Fetch friends and ignore lists
	chatFetchFriendsAndIgnoreList(this);
	chatRestoreChannels(this);

	_recordLogin();
}

void chatPlayerDeactivate(Player* player)
{
	if ( PN_UNLIKELY(player == nullptr) )
		return;

	if ( isValidSide(player->side()) )
	{
		betaLog(_FLI, "removing %p from %d side lists", player, player->side());
		sidePlayers[player->side()].erase(player);
	}

	if ( !player->active() )
		return;

	player->deactivate();

	playerIndex.erase(player->matchHandle());

	s_limboPlayers.insert(player);
}


//This is the actual logic that is run whenever a login is received from
//	the player.  At this point, we match the player teulIndex up with the
//	playerInfo we received earlier, put the handle on our main player list
//	and take are of all the initialization and such
void chatCreatePlayer(PlayerStatusNode* psn, struct Chat::PlayerInfo* playerInfo)
{
	ABORT_IF(playerInfo == nullptr);
	ABORT_IF(psn == nullptr);

	// Create a new player and populate it
	Player* player = new Player(psn, playerInfo);
	ABORT_IF(player == nullptr);

	// Populate the player and add us to the limbo list.
	_registerPlayer(player);

#ifdef STATS_LOGGING
	player->m_loginTime = teulTime();
	player->messagesSent = 0;
	player->shortestMessage = 300;
	player->longestMessage = 0;
	player->bytesSent = 0;
#endif

	psn->localInfo = player;

	dbugLog(_FLD,"created %s player %p id %s[%d]"
		, ( player->isPlaynet() ) ? "CRS" : "player"
		, player
		, player->callsign()
		, player->playerID()
		);
}

//////////////////////////////////////////////////////////////////////
// Common management login functionality to both formats.

static Player* _loginManagerAccount(const char* source, SINT32 teulIndex, const Chat::Name& gameName)
{
	dbugLog(_FLI, "t:%d %s requesting name '%s'", teulIndex, source, gameName.c_str());

	Player* player = new ManagementPlayer(teulIndex, gameName.c_str());
	if ( player == nullptr )
		throw std::runtime_error("Out of memory for new player");		///Terminal

	_registerPlayer(player);

	teulSetPersonaData(teulIndex, player);

	return player;
}

IRCPlayer::IRCPlayer(const char* nick)
	: ManagementPlayer(-1, nick)
	, m_lastChannel("")
	, m_channels()
{
	m_gmHide = false;

	_registerPlayer(this);

	activate(-1, nullptr);
}

//////////////////////////////////////////////////////////////////////
// Version 2 Management clients

bool
Chat::ManagementLogin2::Login(SINT32 teulIndex)
{
	static const ChatHandle autoHandle = chatGetHandle("auto");
	Chat::Name gameName;
	if ( this->m_wantHandle == autoHandle )
	{
		gameName.format("CRS-%04u", (UINT32)teulIndex);
		this->m_wantHandle = gameName.Handle();
	}
	else
		gameName = this->m_wantHandle;

	try
	{
		if ( this->m_wantHandle == 0ULL )
			throw std::domain_error("NULL gamename");

		Player* player = PlayerInterface::Lookup(this->m_wantHandle);
		if ( player != nullptr )
			throw std::domain_error("name in use");

		player = _loginManagerAccount("ManagementLogin2", teulIndex, gameName);

		player->activate(teulIndex, nullptr);

		player->updatePermMask();

		return true;
	}
	catch ( std::exception& e )
	{
		dbugLog(_FLA, "t:%d ManagementLogin2:%s: %s", teulIndex, gameName.c_str(), e.what());
	}

	return false;
}

//This is the logic that is run whenever a logout is received from the host
void chatLogoutPlayer(SINT32 teulIndex)
{
	Player* player = PlayerInterface::Lookup(teulIndex);
	chatFreePlayer(player);
}

static Player*
_findLimboPlayer(const playerid_t playerID)
{
	for ( auto it = s_limboPlayers.begin(), end = s_limboPlayers.end(); it != end; ++it )
	{
		if ( (*it)->playerID() == playerID )
			return static_cast<Player*>(*it);
	}

	return nullptr;
}

static void
_registerPlayer(Player* newPlayer)
{
	if ( newPlayer == nullptr )
	{
		dbugLog(_FLA, "Tried to register a NULL player?");
		return;
	}

	Player* oldPlayer = _findLimboPlayer(newPlayer->playerID());
	if (oldPlayer)
	{
		if ( oldPlayer == newPlayer )
			LogAbort("relogin of old player");
		chatFreePlayer(oldPlayer);
	}

	s_limboPlayers.insert(newPlayer);
}

//This is the logic that is run whenever a logout is received from the host
void chatFreePlayer(Player* player)
{
	if (PN_UNLIKELY(player == nullptr))
		return;

	SINT32 teulIndex;

	// This value is going to get clobbered, so remember it ourselves
	teulIndex = player->teulIndex();

	//Handle taken/squelched
	if ( player->inGrab() )
	{
		dbugLog(_FLI, "takenby = %lld, grabbing = %lld"
						, player->takenBy()
						, player->grabbing()
						);
		chatReleaseGrab(*player);
	}

	if ( player->m_snoopBy )
	{
		PREP_CHAT_MSG(cmsg, "%s is no-longer snooping on %s [subject disconnected]", player->m_snoopBy->callsign(), player->callsign());
		chatDispatchGMMessage("INFO", cmsg);
		player->m_snoopBy->m_snoopOn = nullptr;
		player->m_snoopBy = nullptr;
	}
	if ( player->m_snoopOn )
	{
		PREP_CHAT_MSG(cmsg, "%s is no-longer snooping on %s [snooper disconnected]", player->callsign(), player->m_snoopOn->callsign());
		chatDispatchGMMessage("INFO", cmsg);
		player->m_snoopOn->m_snoopBy = nullptr;
		player->m_snoopOn = nullptr;
	}
	if ( player->m_crsSnoopBy )
	{
		PREP_CHAT_MSG(cmsg, "%s is no-longer snooping on %s [subject disconnected]", player->m_crsSnoopBy->callsign(), player->callsign());
		chatDispatchCRSMessage("INFO", cmsg);
		player->m_crsSnoopBy->m_crsSnoopOn = nullptr;
		player->m_crsSnoopBy = nullptr;
	}
	if ( player->m_crsSnoopOn )
	{
		PREP_CHAT_MSG(cmsg, "%s is no-longer snooping on %s [snooper disconnected]", player->callsign(), player->m_crsSnoopOn->callsign());
		chatDispatchCRSMessage("INFO", cmsg);
		player->m_crsSnoopOn->m_crsSnoopBy = nullptr;
		player->m_crsSnoopOn = nullptr;
	}

	// Remove them from any mission we think they are on
	player->SetMission(0);

//	dbugLog(_FLD,"1) allocating node %p p->%p n->%p"	, player, player->prev(), player->next());

	if ( player->teulIndex() >= 0 )
	{
		Player* other = PlayerInterface::Lookup(player->teulIndex());
		if ( other == player )
			teulSetPersonaData(teulIndex, nullptr);
	}

	chatPlayerDeactivate(player);
	s_limboPlayers.erase(player);

	dbugLog(_FLN,"t:%d p:%u c:%s received logout", teulIndex, player->playerID(), player->callsign());

	//Decrement the side count when they logout
	if ( isValidSide(player->side()) )
		_sideCount[player->side()]--;

#ifdef STATS_LOGGING
	{
		UINT32 timeOnline;

		timeOnline = teulTime() - player->loginTime;

		dbugLog(_FLI,"stats for %s:",player->callsign);
		dbugLog(_FLI,"  time online (sec): %d", timeOnline);
		dbugLog(_FLI,"  messagesSent: %d", player->messagesSent);
		dbugLog(_FLI,"  avg message Length %5.2f", ((FLOAT32) player->bytesSent) / ((FLOAT32) player->messagesSent));
		dbugLog(_FLI,"  shortestMessage: %d",player->shortestMessage);
		dbugLog(_FLI,"  longestMessage: %d",player->longestMessage);
	}
#endif

	chatSaveChannels(*player);

	player->detuneAllRadios();

// HACK: Code does nothing but run an assert().
#ifndef NDEBUG
	//Initialize the rest of the structure
	if ( g_debugRadios )
	{
		for ( SINT32 i = 0; i < MAX_CLIENT_CHANNELS; ++i )
		{
			const Chat::ChannelSubscription& cnode = player->getChannelSub(i);
			assert( cnode.m_subscriber == player );
			assert( cnode.m_channel == nullptr );
		}
	}
#endif

	delete player;
}
