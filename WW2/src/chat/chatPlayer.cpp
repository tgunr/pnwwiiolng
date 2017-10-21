#include <syscfg.h> // Path is project specific.
// Created: Thu Apr 29 17:00:00 2004
//
// Implementation for the "Player" class
//
// Copyright (c) 2004 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "mem/mem.h"
#include "chat/chatBroadcastObject.h"
#include "chat/chatData.h"
#include "chat/chatDB.h"
#include "chat/chatGCIMessage.h"
#include "chat/chatGroupPermissions.h"
#include "chat/chatGroupPermissionTypes.h"
#include "chat/chatHandle.h"
#include "chat/chatHostBrigadeControl.h"
#include "chat/chatHost.h"
#include "chat/chatHostMessage.h"
#include "chat/chatHostMessages.h"
#include "chat/chatLatLong.h"
#include "chat/chatList.h"
#include "chat/chatManagementIDs.h"
#include "chat/chatMissionMember.h"
#include "chat/chatMissions.h"
#include "chat/chatMissionTypes.h"
#include "chat/chatModReport.h"
#include "chat/chatName.h"
#include "chat/chatPersonaUpdate.h"
#include "chat/chatPlayer.h"
#include "chat/chatPreferenceEnum.h"
#include "chat/chatSquadInvite.h"
#include "chat/chatSquadList.h"
#include "chat/chatSquadListTypes.h"
#include "chat/chatTrainers.h"
#include "chat/chatTuneToChannel.h"
#include "chat/chatUnitMotds.h"
#include "chat/chatUpdateChannel.h"
#include "include/mydbQuery.h"
#include "inst/instTypes.h"
#include "server/serverInterface.h"
#include "server/serverPlayerStatusNode.h"
#include "server/serverServerInfo.h"
#include "strt/strtChatTypes.h"
#include "wwii/wwiiConfig.h"
#include "wwii/wwiiDBThreads.h"
#include "wwii/wwiiHash.h"
#include "wwii/wwiiHCUnits.h"
#include "wwii/wwiiServerTypeEnums.h"
#include "wwii/wwiiString.h"
#include "wwii/wwiiThreadText.h"
#include "wwii/wwiiThreadTextQuery.h"

#include <mutex>

// Temporary
#include "map/mapAWS.h"

static HANDLES _spawning;

using RECRUITS = std::unordered_map<UINT32, std::set<UINT32>>;
static RECRUITS _recruits;

using PLAYERIDS = std::unordered_set<UINT32>;
static PLAYERIDS _logins;
#ifdef RECORD_LOGOUTS
static PLAYERIDS _logouts;
#endif

extern void chatSendCommandListPacket(const Player* const);

///TEMPORARY:
// Counts of row/cell contents
struct AWS_READING
{
	UINT16	allied;
	UINT16	axis;
};
using AWS_GRID = std::map<UINT32, AWS_READING>;
using AWS_INSTANCES = std::map<instanceid_t, AWS_GRID>;
AWS_INSTANCES g_awsGrids, g_awsSnapshots;

//////////////////////////////////////////////////////////////////////
// Internal, private functionality

class PlayerImpl
{
public:
	PlayerImpl(Player& player)
		: m_player(player)
		, m_featureLocks()
		, m_groups()
		, m_recruiter(0)
		, m_lastPing(0)
		, m_awsSide(WWII_NO_SIDE)
		, m_awsX(0), m_awsY(0)
		, m_timers()
	{
		memset(m_prefs, 0, sizeof(m_prefs));
		m_timers.clear();
		m_groups.clear();
	}
	~PlayerImpl();

	bool onAWS() const { return m_awsSide > 0; }

	// Reference to my base player class
	Player&			m_player;

	// Feature locks
	std::unordered_map<std::string, UINT32> m_featureLocks;

	UINT32			m_prefs[MAX_CHAT_PREFERENCE];

	// List of groups a player belongs to
	CHAT_GROUPS		m_groups;

	// Recruiting activity
	UINT32			m_recruiter;

	// Last ping from the cell host
	UINT32			m_lastPing;

	// AWS Data
	WWII_SIDE		m_awsSide;
	SINT16			m_awsX, m_awsY;

	// Timer data
	std::unordered_map<std::string, UINT64> m_timers;
};

//////////////////////////////////////////////////////////////////////
// Destructor for implementation class

PlayerImpl::~PlayerImpl()
{
	assert( m_recruiter == 0 );
	assert( _recruits.find(m_player.playerID()) == _recruits.end() );
}

//////////////////////////////////////////////////////////////////////
// Base construction for a player
void
Player::_construct(const playerid_t withPlayerID)
{
	m_playerID = withPlayerID;

	//Initialize the rest of the structure
	for ( int i = 0; i < MAX_CLIENT_CHANNELS; i++)
	{
		Chat::ChannelSubscription& cnode = this->getChannelSub(i);
		//The owner pointer ALWAYS points to the client
		cnode.m_subscriber = this;
		cnode.m_type = NO_CHANNEL;
	}

	m_lastX = m_lastY = m_lastZ = INVALID_GRID_COORD;
	m_lastCP = nullptr;
	m_insideFacility = nullptr;

	m_gridX = INVALID_GRID_COORD;
	m_gridY = INVALID_GRID_COORD;

	m_chatPermissions = &nullChatPermissions;

	m_impl = new PlayerImpl(*this);
}

//////////////////////////////////////////////////////////////////////
// Record that a player has logged in to the database

void
Player::_recordLogin()
{
	if ( !isManager() && playerID() > 1 )
		_logins.insert(playerID());
}

//////////////////////////////////////////////////////////////////////
// Record that a player has logged out to the database

void
Player::_recordLogout()
{
#ifdef RECORD_LOGOUTS
	if ( !isManager() && playerID() > 1 )
		_logouts.insert(playerID());
#endif
}

//////////////////////////////////////////////////////////////////////
// Used by tune radio and tune place holder to populate the cnode
// structure and send the player notice of a channel tuning

bool
Player::_setChannel(UCHAR index, CHANNEL_TYPE type, Chat::Channel* toChannel, bool rx)
{
	Chat::ChannelSubscription& cnode = this->getChannelSub(index);
	assert( cnode.m_subscriber == this );

	cnode.subscribe(type, toChannel);

	// We have to send the player a tune instructions
	ChatHandle chHandle = (toChannel ? toChannel->m_handle : 0ULL);
	Chat::TuneToChannel playerMessage(index, type, chHandle, rx, false);
	if ( playerMessage.send(teulIndex()) == false )
	{
		dbugLog(_FLA, "t:%d p:%u c:%s FAILED tuning player to channel", teulIndex(), playerID(), callsign());
		return false;
	}

	if ( g_debugRadios )
		dbugLog(_FLD, "t:%d p:%u c:%s tuned radio index %d to %s", teulIndex(), playerID(), callsign(), index, chanTypeName[cnode.type()]);

	return true;
}

//////////////////////////////////////////////////////////////////////
// Constructor

Player::Player(UINT32 withPlayerid, bool playerIsManager)
	: wwiiPlayer()
{
	dbugLog(_FLI, "p:%u creating %s player %p", withPlayerid, (playerIsManager ? "management" : "standard"), this);
	_construct(withPlayerid);
}

//////////////////////////////////////////////////////////////////////
// Construct from baseline data

Player::Player(const PlayerStatusNode* psn, const Chat::PlayerInfo* playerInfo)
	: wwiiPlayer()
{
	if ( psn == nullptr )
		throw std::domain_error("Tried to create Client with NULL player");
	if ( playerInfo == nullptr )
		throw std::domain_error("Tried to create Client with NULL playerInfo");

	const bool playnet = (playerInfo->m_permissionMask & IS_PLAYNET) == IS_PLAYNET;
	Chat::Name newCallsign(playerInfo->m_handle);
	dbugLog(_FLI, "t:%d p:%u c:%s creating %s player %p"
					, psn->teulIndex
					, playerInfo->playerID
					, newCallsign.c_str()
					, (playnet) ? "CRS" : "player"
					, this
					);

	_construct(playerInfo->playerID);

	_setCallsign(newCallsign.c_str(), playnet);

	SetInstance(Instance::g_campaign);

	SetDispatch(psn->dispatch());

	m_squadID = playerInfo->squadID;
	m_playerMask = playerInfo->m_permissionMask;

	// Check the playnet permissions
	if ( playnet || m_playerMask & DARK_GLASSES )
		m_playerMask |= IS_PLAYNET;
	else
		m_playerMask &= ~IS_PLAYNET;

	// We need to fetch the player's group list to ensure we have their permissions
	m_impl->m_groups.clear();
	m_chatPermissions = chatGroupPermissionsForGroups(m_impl->m_groups);
	m_maxRank = m_chatPermissions->m_limitRank[branch()];

	readGroups();

	_setCallsign(callsign(), isPlaynet());

	// Make sure the squad is loaded
	if ( m_squadID > 0 )
		chatRequestSquad(m_squadID, this);

	// Default, non-zero, preferences
	SetPreference(CPR_AUTO_LEADER, TRUE, false);
}

//////////////////////////////////////////////////////////////////////
// Destructor for the player class

Player::~Player()
{
	dbugLog(_FLI, "p:%u c:%s releasing player %p", playerID(), callsign(), this);

	SetSpawned(false);

	removeFromAWS();
	removeFromGrid();

	SetMission(0);
	changeBrigade(0);

	SetInstance(Instance::g_invalid);

	// Make sure we don't leak OICs
	assert( leadingBrigade() == 0 );

	SetSquadInvite(nullptr);
	if ( m_tunes )
	{
		memFree(m_tunes);
		m_tunes = nullptr;
	}

	delete m_impl;
}

//////////////////////////////////////////////////////////////////////
// Set the branch

void
Player::SetBranch(BRANCH_ENUM newBranch)
{
	m_branch = newBranch;
	if ( m_chatPermissions )
	{
		m_maxRank = m_chatPermissions->m_limitRank[newBranch];
		dbugLog(_FLD, "t:%d p:%u c:%s new branch %u, maxRank now %u"
						, teulIndex(), playerID(), callsign()
						, newBranch, m_maxRank);
	}
}

//////////////////////////////////////////////////////////////////////
// Apply the changes in a persona update which doesn't involve a
// change of sides.

bool
Player::_applyPersonaUpdate(const Chat::PersonaUpdate& update, bool sideChanged)
{
	bool newCountry = false;
	bool newBrigade = false;
	bool newMission = false;
	bool newOrigin = false;
	bool newObjective = false;
	bool newRank = false;
	bool wasLeader = isMissionLeader();
	bool needSquadUpdate = false;
	WWII_SIDE oldCommandSide = effectiveCommandSide();

	if ( update.m_sortieID != sortieID() )
	{
		m_sortieID = update.m_sortieID;
		needSquadUpdate = true;
	}
	if ( sortieID() == 0 && isSpawned() )
	{
		SetSpawned(false);
	}
	else if ( update.m_isSpawned != isSpawned() )
	{
		SetSpawned(update.m_isSpawned);
	}

	const bool newInstance = ( update.m_instance.id() != instance().id() );

	SetInstance(update.m_instance);

	if ( sideChanged )
	{
		needSquadUpdate = true;
		newCountry = newBrigade = newMission = newOrigin = newObjective = newRank = true;
	}
	else if ( country() != update.m_country )
	{
		needSquadUpdate = true;
		newCountry = true;
	}

	m_flags.checksumsDirty = update.m_badChecksum;

	wwiiPlayer::SetCountry(update.m_country, wwiiSide(update.m_country));
	SetBranch(update.m_branch);

	if ( !isManager() )
		m_playerMask = update.m_permissionMask;

	if ( m_brigadeID != update.m_brigadeID )
	{
		needSquadUpdate = newBrigade = true;
		changeBrigade(update.m_brigadeID);
	}

	if ( m_missionID != update.m_missionID )
	{
		needSquadUpdate = newMission = true;
		SetMission(update.m_missionID);
	}

	if ( memcmp(&m_originCpID, &update.m_origin, sizeof(m_originCpID)) != 0 )
	{
		newOrigin = true;
		m_originCpID = update.m_origin;
	}

	if ( memcmp(&m_objectiveCpID, &update.m_target, sizeof(m_objectiveCpID)) != 0 )
	{
		newObjective = true;
		m_objectiveCpID = update.m_target;
	}

	if ( m_realRank != update.m_rank )
	{
		needSquadUpdate = newRank = true;
		m_realRank = update.m_rank;
	}

	m_inFacility = (update.m_origin.id != 0);

	// Any one of high-command+ or who has not selected a CP gets all HQ messages
	if ( !m_inFacility || isCommander() )
		m_rxAllHQ = true;
	else
		m_rxAllHQ = false;

	if ( newCountry || newRank )
	{
		needSquadUpdate = true;
		readGroups();
	}

	updatePermMask();

	updateRadios(newInstance, sideChanged, newBrigade, newMission, newOrigin, newObjective);

	WWII_SIDE commandSide = effectiveCommandSide();
	if ( isValidSide(commandSide) && (sideChanged || commandSide != oldCommandSide) )
	{
		needSquadUpdate = true;
		const char* motd = chatGetUnitMotd(commandSide);
		if ( motd && *motd )
		switch (commandSide)
		{
			case ALLIED:
				message(LSTR_ALLIED_HC, LSTR_GENERIC, motd);
			break;
			case AXIS:
				message(LSTR_AXIS_HC, LSTR_GENERIC, motd);
			break;
			default: break;
		}
	}

	dbugLog(_FLD, "p:%u c:%s br:%u sq:%u m:%u nc:%c nbr:%c nm:%c nor:%c nob:%c nr:%c ni:%c nsu:%c"
					, playerID(), callsign(), brigadeID(), squadID(), missionID()
					, (newCountry ? 'y' : 'n')
					, (newBrigade ? 'y' : 'n')
					, (newMission ? 'y' : 'n')
					, (newOrigin ? 'y' : 'n')
					, (newObjective ? 'y' : 'n')
					, (newRank ? 'y' : 'n')
					, (newInstance ? 'y' : 'n')
					, (needSquadUpdate ? 'y' : 'n')
					);

	// Has the player become a mission leader?
	if ( missionID() && isMissionLeader() && !wasLeader )
	{
		needSquadUpdate = true;
		chatSetMissionLeader(missionID(), update.m_leader, side(), m_originCpID.Key(), m_objectiveCpID.Key());
	}

	if ( sideChanged )
	{
		if ( host_config.arena.limitedDamage )
		{
			message(LSTR_CAUTION, LSTR_LIMITED_DAMAGE_ENABLED);
		}
	}

	return needSquadUpdate;
}

//////////////////////////////////////////////////////////////////////
// Determine whether a player is currently an "active" commander

WWII_SIDE
Player::activeCommandSide() const
{
	if ( side() == ALLIED && permissions(ALLIED_COMMAND_STAFF) )
		return ALLIED;
	if ( side() == AXIS && permissions(AXIS_COMMAND_STAFF) )
		return AXIS;
	return WWII_NO_SIDE;
}

//////////////////////////////////////////////////////////////////////
// Add my name (with markup) to a string

void
Player::addName(std::string& toString, const char* joiner /* = " " */) const
{
	if ( !toString.empty() )
		toString += joiner;

	if ( isAFK() )
		toString += "(";

	toString += callsign();

	if ( isSpawned() )
		toString += "*";

	if ( isAFK() )
		toString += ")";
}

//////////////////////////////////////////////////////////////////////
// If the player has relevant permissions, include them in the
// "Command" channel linkage. This provides a quick way of indexing
// only players who are commanders, GMs or playnet without having to
// go through the huge number of regular players.
//

void
Player::addToCommandList()
{
	if ( m_onCommandList || !m_initialized || !active() )
		return;

	if ( !isActiveOfficer() && !isEffectiveGM() )
	{
		dbugLog(_FLI, "%s[%d] lacks permissions for command list", callsign(), playerID());
		return;
	}

	dbugLog(_FLI, "%s[%d] adding to command list", callsign(), playerID());

	commandList.insert(this);
	m_onCommandList = true;
}

//////////////////////////////////////////////////////////////////////
// Leave my current brigade

void
Player::changeBrigade(UINT32 newBrigadeID)
{
	if ( newBrigadeID == brigadeID() )
		return;

	// If we're leading a HCUnit let us stay within it, but leaving it removes OIC status
	if ( leadingBrigade() )
	{
		HCUnit* newUnit = HCUnit::Lookup(newBrigadeID);
		if ( !newUnit || !newUnit->isBelowOrEqual(leadingBrigade()) )
			chatRemoveOICStatus(leadingBrigade(), chatHandle());
	}

	m_brigadeID = newBrigadeID;
}

//////////////////////////////////////////////////////////////////////
// If the last group read was less more than a minute ago,
// then force a re-read of the group memberships
void
Player::checkGroups()
{
	static const UINT32 _PERMISSION_TIMEOUT = 60 * 60;		// Re-read permissions once an hour
	if ( m_lastGroupRead > 0 && m_lastGroupRead + _PERMISSION_TIMEOUT < teulTime() )
		invalidateGroups();
}

//////////////////////////////////////////////////////////////////////
// Check if a named timer is active; returns false if the timer is not
// set or has not yet elapsed.

bool
Player::checkTimerActive(const char* timer) const
{
	auto it = m_impl->m_timers.find(timer);
	if ( it == m_impl->m_timers.end() )
		return false;
	return ( teulClock64 <= it->second );
}

//////////////////////////////////////////////////////////////////////
// Check if a named timer is set and has expired.

bool
Player::checkTimerElapsed(const char* timer) const
{
	auto it = m_impl->m_timers.find(timer);
	if ( it == m_impl->m_timers.end() )
		return false;
	return ( teulClock64 > it->second );
}

//////////////////////////////////////////////////////////////////////
// Send a contextual strat command

bool
Player::contextualStratCommand(const char* string, const char* arguments, WWII_COUNTRY withCountry, WWII_SIDE withSide, const Property* property)
{
	if ( !isalpha(string[0]) )
		return false;

	OOBMessage outgoing(playerID(), brigadeID(), chatHandle(), string, arguments, property->m_fid, withSide, withCountry);

	dbugLog(_FLI, "t:%d p:%u c:%s contextual oob: %s [at %s]", teulIndex(), playerID(), callsign(), outgoing.m_text, property->Name());

	outgoing.send();

	return true;
}

//////////////////////////////////////////////////////////////////////
// deactivate a player

void
Player::deactivate()
{
	if ( initialized() )
	{
		if ( m_impl->m_recruiter )
			unrecruit(LSTR_RECRUIT_LOGGED_OFF);
		else
			stopRecruiting();
		_recordLogout();
	}

	m_teulIndex = -1;
	m_active = false;

	SetMission(0);
	changeBrigade(0);

	assert( leadingBrigade() == 0 );

	removeFromCommandList();

	if ( squadID() )
	{
		SquadList* squadList = chatFindSquadList(squadID());
		if ( squadList != nullptr )
			squadList->updateMember(matchHandle(), squadRank(), nullptr);
	}

	// Remove from trainer list etc
	chatUpdateTrainerStatus(playerID(), false);
	// Remove from recruiter list etc
	chatUpdateRecruiterList(this, false, nullptr);

	removeFromAWS();
	removeFromGrid();
//	m_lastX = m_lastY = m_lastZ = INVALID_GRID_COORD;
//	m_lastCP = NULL;
}

//////////////////////////////////////////////////////////////////////
// Detune ALL radios

extern const char* const channelTypeName[];

void
Player::detuneAllRadios()
{
    dbugLog(_FLD, "t:%d p:%u c:%s DETUNE", teulIndex(), playerID(), callsign());

    for ( UINT32 i = 0; i < MAX_CLIENT_CHANNELS; ++i )
    {
        auto& cnode = this->getChannelSub(i);
        dbugLog(_FLD, " radio%u:{chan:%p, type:%u[%s], subs:%" PRSizeT "}"
            , i, cnode.channel(), cnode.type(), channelTypeName[cnode.type()], (cnode.channel() ? cnode.channel()->numSubscribers() : 0));
    }

	for ( UINT32 i = 0; i < MAX_CLIENT_CHANNELS; ++i )
		detuneRadio(i);
}

//////////////////////////////////////////////////////////////////////
// Detune all non-player channels

void
Player::detuneChannels()
{
	UINT32 i;

	for ( i = 0; i< MAX_CLIENT_CHANNELS; ++i )
	{
		if ( this->getChannelSub(i).channel() )
			detuneRadio(i);
	}
}

//////////////////////////////////////////////////////////////////////

void
Player::detuneChannelsByType(CHANNEL_TYPE type)
{
	UINT32 i;

	for ( i = 0; i< MAX_CLIENT_CHANNELS; ++i )
	{
		Chat::ChannelSubscription& cnode = this->getChannelSub(i);
		if ( cnode.type() == type && cnode.channel() )
			detuneRadio(i);
	}
}

//////////////////////////////////////////////////////////////////////
// Detune a particular channel index

void
Player::detuneRadio(UCHAR index)
{
	Chat::ChannelSubscription& cnode = this->getChannelSub(index);

	if ( cnode.channel() == nullptr && cnode.type() == NO_CHANNEL )
		return;

	cnode.unsubscribe();

	if ( active() && teulIndex() >= 0 )
		chatSendUserNLA(teulIndex(), index, 0);
}

//////////////////////////////////////////////////////////////////////
// Determine whether a player is currently an active commander or a
// rat or GM

WWII_SIDE
Player::effectiveCommandSide() const
{
	if ( isEffectiveGM() )
		return side();
	return activeCommandSide();
}

//////////////////////////////////////////////////////////////////////
// Determine whether a player is currently an active officer or a
// rat or GM

WWII_SIDE
Player::effectiveOfficerSide() const
{
	if ( isValidSide(side()) && (isActiveOfficer() || isEffectiveGM()) )
		return side();
	return WWII_NO_SIDE;
}

//////////////////////////////////////////////////////////////////////
// Determine if a feature was locked, or lock it if it wasn't

bool
Player::featureLock(const std::string& feature, UINT32 now, UINT32 duration)
{
	// If we were empty, zero or more than 'duration' ago,
	// set the lock time to now so it becomes locked.
	if ( duration == 0 || now == 0 || m_impl->m_featureLocks[feature] <= now - duration )
	{
		m_impl->m_featureLocks[feature] = now;
		return false;
	}

	// Otherwise we had a value that was >= now-duration, so
	// we are still locked.
	return true;
}


//////////////////////////////////////////////////////////////////////
// Find a free channel

UINT32
Player::getOpenChannelIndex(UCHAR type)
{
	UINT32 openSpot = MAX_CLIENT_CHANNELS - 1;
	UINT32 i;

	// allways start at 1, 0 is reserved for side, cp and world
	for ( i = 0; i < MAX_CLIENT_CHANNELS; ++i )
	{
		const Chat::ChannelSubscription& cnode = this->getChannelSub(i);
		// Preferable to find an existing use of this channel type
		if ( cnode.type() == type )
			return i;

		if ( !cnode.inUse() && i < openSpot )
			openSpot = i;
	}

	if ( this->getChannelSub(openSpot).isTuned() )
		detuneRadio(openSpot);

	return openSpot;
}

//////////////////////////////////////////////////////////////////////
// Lookup a preference value with bounds checking

UINT32
Player::getPreference(UINT32 prefId) const
{
	if ( prefId <= CPR_INVALID || prefId >= MAX_CHAT_PREFERENCE )
	{
		dbugLog(_FLW, "t:%d p:%u c:%s attempting to fetch preference#%u", teulIndex(), playerID(), callsign(), prefId);
		return 0;
	}

	return m_impl->m_prefs[prefId];
}

//////////////////////////////////////////////////////////////////////
// Record an instance of a GM log-worthy event

void
Player::gmNote(const SCHAR* level, const std::string& command, const SCHAR* data1, const SCHAR* data2)
{
	// Do we need to give the player the GM "intro" message?
	if ( !m_gmNote )
	{
		message(LSTR_NOTE, LSTR_COMMANDS_LOGGED);
		m_gmNote = true;
	}

	recordEvent(level, command.c_str(), data1, data2);
}

//////////////////////////////////////////////////////////////////////
// Link up two players so that they are chatting directly to each
// other.

void
Player::grab(Player* rhs)
{
	m_grabbing = rhs->matchHandle();
	rhs->m_takenBy = matchHandle();
	m_grabbed = rhs->m_grabbed = true;
}

//////////////////////////////////////////////////////////////////////
// Returns a reference to our group list after ensuring it is current

const CHAT_GROUPS&
Player::groups()
{
	if ( m_lastGroupRead == 0 )
		readGroups();
	return m_impl->m_groups;
}

//////////////////////////////////////////////////////////////////////
// Test if the player is available as a recruiter for the specified
// secondary player. NULL if not, otherwise returns the squad list
// of this player.

SquadList*
Player::isAvailableRecruiter(const Player* player) const
{
	// Do inexpensive tests first
	if ( player->squadID() || !this->squadID() )
		return nullptr;

	if ( player->side() != this->side() )
		return nullptr;

	// Can we see each other?
	if ( !this->isVisibleTo(player) || !player->isVisibleTo(this) )
		return nullptr;

	if ( this->missionID() <= 0 || this->teulIndex() < 0 || player->teulIndex() < 0 )
		return nullptr;

/*	/// TODO: Enable this code
	if ( country() != player->country() || branch() != player->branch() )
		continue;
*/

	if ( isAFK() || isBuzzard() || player->isBuzzard() )
		return nullptr;

	if ( squadRank() < Squad::STAFFER )
		return nullptr;

	// Am I actually recruiting?
	if ( getPreference(CPR_IS_RECRUITING) == 0 )
		return nullptr;

	if ( ignoring(player) || player->ignoring(this) )
		return nullptr;

	SquadList* squadList = chatFindSquadList(squadID());
	if ( squadList == nullptr )
		return nullptr;
	if ( !squadList->squad().recruiting() )
		return nullptr;

	return squadList;
}

//////////////////////////////////////////////////////////////////////
// Test if player is tuned to a particular channel typ

bool
Player::isTunedTo(CHANNEL_TYPE type) const
{
	for ( UINT32 i = 0; i < MAX_CLIENT_CHANNELS; i++ )
	{
		const Chat::ChannelSubscription& cnode = this->getChannelSub(i);
		if ( cnode.type() == type )
			return true;
	}

	return false;
}


//////////////////////////////////////////////////////////////////////
// Determine if another player can see me

bool
Player::isVisibleTo(const Player* viewer, bool ignoreSide) const
{
	if ( !ignoreSide && (noSide() || effectiveRank() == 0) )
		return false;
	if ( !initialized() || teulIndex() < 0 )
		return false;
	// GMs can always send to everyone
	if ( viewer->isGM() )
		return true;
	// Training chat can be turned OFF.
	if (m_trOff)
		return false;
	// Non-GMs get "unreachable" when a GM is in GM hide mode
	if ( isGM() )
		return ( !m_gmHide );
	if ( ignoreSide || viewer->side() == this->side() )
		return true;
	// Only commanders and above can send across sides
	if ( permissions(CROSS_TALK) && viewer->permissions(CROSS_TALK) )
		return true;

	return false;
}

//////////////////////////////////////////////////////////////////////
// Describe the last position of a player

const char*
Player::lastPosition(const char* prefix, const FLOAT64& x, const FLOAT64& y, bool onlyIfValid) const
{
	static char posn[80+1];
	if ( !isSpawned() && m_lastCP == nullptr )
	{
		if ( onlyIfValid )
			return "";
		return " (not spawned yet)";
	}
	if ( isSpawned() && (x >= INVALID_GRID_COORD || y >= INVALID_GRID_COORD) )
	{
		return " (spawning)";
	}
	if ( m_lastCP == nullptr )
	{
#ifdef BETA_RELEASE
		return " nowhere (unexpected result)";
#else
		return " *nowhere*";
#endif
	}

	LatLong lastLoc(x * 100, y * 100);

	strprintf(posn
					, " %s%s %s"
					, prefix, m_lastCP->Name(), lastLoc.m_text);

	return posn;
}

//////////////////////////////////////////////////////////////////////
// Notify a sender that we are afk, and remember that we did it

void
Player::notifyOtherIfAFK(Player& other) const
{
	// Only send it if they're actually AFK
	if ( !isAFK() )
		return;

	// Don't spam me with reminders for the same person
	if ( other.m_lastAfkNoticeFrom == m_playerID )
	{
		static const UINT32 _AFK_TIMEOUT = 120;		// Forget afk reminders after ~2 minutes
		if ( other.featureLock("afk", teulTime(), _AFK_TIMEOUT) )
			return;
	}

	if ( m_afkMessage[0] != 0 )
		other.message(LSTR_CAUTION, LSTR_IS_AFK_MSG, callsign(), m_afkMessage);
	else
		other.message(LSTR_CAUTION, LSTR_IS_AFK, callsign());

	other.m_lastAfkNoticeFrom = m_playerID;
}

//////////////////////////////////////////////////////////////////////
// Return a valid string describing the origin CP

const char*
Player::originCP() const
{
	const Property* prop = chatFindProperty(originCpID());

	return ( (prop == nullptr) ? "<unknown>" : prop->Name() );
}

const char*
Player::objectiveCP() const
{
	const Property* prop = chatFindProperty(objectiveCpID());
	return ( (prop == nullptr) ? "<unknown>" : prop->Name() );
}

#include "wwii/wwiiHash.h"
static std::unordered_map<playerid_t, std::vector<UINT32>> playerGroups;
static std::mutex playerGroupsMutex;

bool
chatProcessPlayerGroups()
{
	size_t numGroupEntries = 0;
	{
		std::lock_guard<std::mutex> hashLock(playerGroupsMutex);
		numGroupEntries = playerGroups.size();
		for ( auto it = playerGroups.begin(), end = playerGroups.end(); it != end; ++it )
		{
			Player* const player = PlayerInterface::Lookup(it->first);
			if ( player == nullptr )
				continue;
			auto& groups = it->second;
			player->setGroups(groups);
		}
		playerGroups.clear();
	}
	if ( numGroupEntries > 0 )
		dbugLog(_FLI, "Processed %" PRSizeT " group entries.", numGroupEntries);

	return true;
}

//////////////////////////////////////////////////////////////////////
// Query the groups a player is in.

class PlayerGroupQuery : public DBThreadQuery
{
public:
	PlayerGroupQuery(const playerid_t _playerID)
		: DBThreadQuery(DBThread::Default, "PlayerGroupQuery", DBThreadQuery::_IMMEDIATE
							, "SELECT v.group_oid"
								" FROM `auth`.`v_player_active_groups` v"
								" WHERE v.playerid = %u AND v.arenaid = %u"
									, _playerID
									, Server::myServerInfo.stats.arenaID
									)
		, m_playerID(_playerID)
		{}

protected:
	const playerid_t	m_playerID;
	virtual void Success() override
	{
		std::vector<UINT32> groupIDs;

		groupIDs.clear();
		while ( Query()->FetchRow() )
		{
			groupIDs.push_back(Query()->Row().NextUInt());
		}

		std::lock_guard<std::mutex> hashLock(playerGroupsMutex);
		playerGroups[m_playerID] = groupIDs;
	}

	virtual void Failure() override
	{
		dbugLog(_FLA, "p:%u unable to read player groups from database", m_playerID);
	}
};

//////////////////////////////////////////////////////////////////////
// Request a load of the chat-group set for this player

void
Player::readGroups()
{
	// Mark that we don't need to re-read for a while
	m_lastGroupRead = teulTime();

	// Request the group list asynchronously.
	(new PlayerGroupQuery(playerID()))->Queue();
}

//////////////////////////////////////////////////////////////////////
// Apply the results from an asynchronous query

void
Player::setGroups(std::vector<UINT32>& groups_)
{
	if ( g_debugPersonas )
		dbugLog(_FLD, "t:%d p:%u c:%s got group list", teulIndex(), playerID(), callsign());

	// Mark that we don't need to re-read for a while
	m_lastGroupRead = teulTime();

	try
	{
		m_impl->m_groups.clear();
		const size_t numGroups = groups_.size();
		for ( UINT32 i = 0; i < numGroups; ++i )
		{
			m_impl->m_groups.insert(groups_[i]);
		}

		m_chatPermissions = chatGroupPermissionsForGroups(m_impl->m_groups);
		m_maxRank = m_chatPermissions->m_limitRank[branch()];
	}
	catch (std::exception& e)
	{
		dbugLog(_FLA, "Couldn't read %s(%u)'s chat groups: %s"
						, callsign()
						, playerID()
						, e.what()
						);
	}

	updatePermMask();

	_setCallsign(callsign(), isPlaynet());
}

//////////////////////////////////////////////////////////////////////
// Make a note of an event to the log and the database in standardized
// formats.

void
Player::recordEvent(const char* level, const char* index, const char* data1, const char* data2) const
{
	const SCHAR* link1 = ":";
	const SCHAR* link2 = ":";

	if ( data1 == nullptr )
		link1 = data1 = "";
	if ( data2 == nullptr )
		link2 = data2 = "";

	dbugLog(_FLN, "!%s:%s:.%s%s%s%s%s", level, callsign(), index, link1, data1, link2, data2);
	chatdbRecordEvent(level, callsign(), index, data1, data2);
}

//////////////////////////////////////////////////////////////////////
// This player has recruited someone, make a note of it.

bool
Player::recruited(const Player* byPlayer)
{
	assert( byPlayer != nullptr );

	dbugLog(_FLI, "t:%d p:%u c:%s has been recruited by t:%d p:%u c:%s"
					, teulIndex(), playerID(), callsign()
					, byPlayer->teulIndex(), byPlayer->playerID(), byPlayer->callsign());

	m_impl->m_recruiter = byPlayer->playerID();
	_recruits[m_impl->m_recruiter].insert(playerID());

	return true;
}

//////////////////////////////////////////////////////////////////////
// Determine whether the player is rejecting auto leadership

bool
Player::rejectLead() const
{
	// Buzzards can never be promoted to leader.
	if ( m_isBuzzard )
		return true;
	if ( teulIndex() < 0 )
		return true;
	if ( isAFK() )
		return true;
	return ( getPreference(CPR_AUTO_LEADER) == FALSE );
}

//////////////////////////////////////////////////////////////////////
// Detach any linkages a player has to the command channel. Arguably
// this should perform the same check as the linkTo function.
//

void
Player::removeFromCommandList()
{
	if ( !m_onCommandList )
		return;

	dbugLog(_FLI, "%s[%d] removing from command list", callsign(), playerID());

	commandList.erase(this);
	m_onCommandList = false;
}

//////////////////////////////////////////////////////////////////////
// Send a GCI ('Global Chat Interface') aka HQ message to a player

bool
Player::approveGCIMessage(UINT32 messageID, const Chat::GCIMessage& gci, bool isGlobal)
{
	// Duplicate avoidance
	if ( lastMessageID() >= messageID )
		return false;

	m_lastMessageID = messageID;

	if ( !active() || teulIndex() < 0 )
		return false;

	if ( isGlobal )
	{
		// Global messages have to filter their own audiences
		if ( hqOff() )
			return false;
#ifdef CHANNEL_FILTERED_HQ
		// Player isn't in a position to receive global messages
		if ( !rxAllHQ() )
			return false;
#endif

		if ( isValidSide(gci.side) && side() != gci.side )
			return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////
// Send a command to the strat host in the current context of the player

bool
Player::sendStratCommand(const char* string, const char* arguments, bool useMapTarget/*=true*/)
{
	if ( !isalpha(string[0]) )
		return false;

	// Has the player selected something on the map?
	Property* property = nullptr;
	if ( useMapTarget && uiTarget().id != 0 )
		property = chatFindProperty(uiTarget());

	FULL_OBJ_ID asCpID;
	if ( property != nullptr )
		asCpID = property->m_fid;
	else if ( isSpawned() && lastCP() )
		asCpID = lastCP()->m_fid;
	else if ( inFacility() )
		asCpID = originCpID();
	else
		asCpID = nullObjID;

	WWII_SIDE asSide;
	WWII_COUNTRY asCountry;

	if ( noSide() )
	{
		asSide = WWII_NO_SIDE;
		asCountry = WWII_NO_COUNTRY;
	}
	else
	{
		asSide = side();
		asCountry = country();
	}

	OOBMessage oob(playerID(), brigadeID(), chatHandle(), string, arguments, asCpID, asSide, asCountry);
	oob.send();

	return true;
}

//////////////////////////////////////////////////////////////////////
// Set the AFK state and optional message

void
Player::SetAFK(bool goingAFK, const char* newMessage)
{
	if ( goingAFK )
		m_goneAFK = teulTime();
	else
		m_goneAFK = 0;

	if ( !isAFK() || newMessage == nullptr )
		m_afkMessage[0] = 0;
	else
		strsafecpy(m_afkMessage, newMessage);
}

//////////////////////////////////////////////////////////////////////
// Update and log the cell host they've been appointed to

void
Player::SetCellHost(UINT32 newHost)
{
	if (newHost == m_cellHost)
		return;

	dbugLog(_FLI, "t:%d p:%u c:%s changed cell host %08x -> %08x",
		teulIndex(), playerID(), callsign(), m_cellHost, newHost
		);
	m_cellHost = newHost;
}

//////////////////////////////////////////////////////////////////////
// Handling for a Persona Update where a user's national-allegiance has changed
// Returns 'true' if side changes

void
Player::SetCountry(WWII_COUNTRY toCountry, WWII_SIDE toSide)
{
	if ( toCountry == country() )
		return;

	WWII_COUNTRY oldCountry = country();

	if ( side() > 0 && _sideCount[side()] > 0 )
		_sideCount[side()]--;

	if ( g_debugPersonas )
		dbugLog(_FLD, "t:%d p:%u c:%s side select %u->%u"
						, teulIndex(), playerID(), callsign(), oldCountry, toCountry);

	// Their permissions may need to be re-evaluated
	invalidateGroups();

	// You're at the lobby now so you receive all HQ messages
	wwiiPlayer::SetCountry(toCountry, toSide);
	ExitBrigade(0);

	m_inFacility = false;
	m_originCpID = m_objectiveCpID = nullObjID;
	m_rxAllHQ = true;

	if ( isValidSide(side()) )
	{
		//Update the side count
		_sideCount[side()]++;

		message(LSTR_SYSTEM, LSTR_GENERIC, chatGetUnitMotd(SYSTEM_MOTD_UNIT));
	}

	return;
}

//////////////////////////////////////////////////////////////////////
// Set that this player is leading (or not) a brigade

void
Player::SetLeadingBrigade(UINT32 toBrigadeID)
{
	if ( leadingBrigade() == toBrigadeID )
		return;

	m_leadingBrigade = toBrigadeID;

	// Recalculate the player's permission mask
	updatePermMask();
	// Add/remove them to/from the HC chat list
	updateCommandStatus();
	// Make sure that next time they try to do something we will
	// pick up their chat groups
	invalidateGroups();
}
//////////////////////////////////////////////////////////////////////
// Set the player's mission ID and take heed of what that means

void
Player::SetMission(UINT32 toMissionID)
{
	if ( toMissionID == m_missionID )
		return;

	if ( m_missionID != 0 )
		chatMissionRemovePlayer(this, m_missionID, m_impl->m_recruiter);

	m_missionID = toMissionID;
	m_joinedMission = teulClock64;

	chatMissionAddPlayer(this, m_missionID, m_impl->m_recruiter);
}

//////////////////////////////////////////////////////////////////////
// Set when to send the next reminder not to spam the help channel

void
Player::SetTimer(const char* timer, const TeulTimeMS time)
{
	m_impl->m_timers[timer] = time;
}

//////////////////////////////////////////////////////////////////////
// Given a request from a client, return an apropriate update mode.
// This is to stop clients requesting update modes which are not
// available to player clients.

static UINT32
_validateUpdateMode(UINT32 request)
{
	static const UINT32 _MAX_CONFIG_MODE = 3;
	if ( request == 0 )
		return host_config.update.defaultUM;
	else if ( request > _MAX_CONFIG_MODE )
		return _MAX_CONFIG_MODE;

	return request - 1;
}

//////////////////////////////////////////////////////////////////////
// Set a preference field to a specific value. If the value fromClient
// is true, then the value is sanitized before being accepted.

void
Player::SetPreference(UINT32 prefId, UINT32 request, bool fromClient)
{
	if ( prefId == 0 || prefId >= MAX_CHAT_PREFERENCE )
	{
		dbugLog(_FLA, "t:%d p:%u c:%s Illegal preference ID: %u"
						, teulIndex(), playerID(), callsign()
						, prefId);
		return;
	}

	UINT32 value = (fromClient) ? validatePreference(prefId, request) : request;

	switch ( prefId )
	{
	case CPR_UPDATE_MODE:
	{
		if (fromClient)
		{
			dbugLog(_FLD, "t:%d p:%u c:%s updateMode from Client:%u req:%u"
				, teulIndex(), playerID(), callsign()
				, value, request);
			chatSendBandwidthNotice(*this, (UINT32)value);
		}
		else
		{
			dbugLog(_FLD, "t:%d p:%u c:%s updateMode response from cell host:%u req:%u"
				, teulIndex(), playerID(), callsign()
				, value, request);
			SetUpdateMode(value);
		}
	}
	break;
	case CPR_IS_RECRUITING:
		if ( squadID() )
		{
			SquadList* list = chatFindSquadList(squadID());
			chatUpdateRecruiterList(this, (value != 0), list);
		}
	break;
	case CPR_SET_DECAL:
		if ( value > 0 )
		{
			log(_FLI, "setting decal to %u", value);
			DBThread::Do(DBThread::Default,
					"UPDATE `wwiionline`.`wwii_player`"
						" SET player_decal=%u"
						" WHERE playerid=%u",
							value, playerID());
			chatSendMapPersonaUpdate(playerID(), false, 0, 0, true, value);
		}
	break;
	case CPR_SET_NETCODE:
	{
		if (fromClient)
		{
			dbugLog(_FLD, "t:%d p:%u c:%s netcode from Client:%u req:%u"
				, teulIndex(), playerID(), callsign()
				, value, request);
			SetNetcode(value);
			chatSendNetcodeNotice(*this, (UINT32)value);
		}
		else
		{
			dbugLog(_FLD, "t:%d p:%u c:%s netcode response from cell host:%u req:%u"
				, teulIndex(), playerID(), callsign()
				, value, request);
		}
	}
	break;
	default:
	break;
	}

	m_impl->m_prefs[prefId] = value;
}

//////////////////////////////////////////////////////////////////////
// Check on players who are in transit to a cell host

bool
chatCheckSpawningPlayers()
{
	HANDLES::iterator it, nextIt;
	for ( it = _spawning.begin(), nextIt = it; it != _spawning.end(); it = nextIt )
	{
		++nextIt;
		Player* player = PlayerInterface::Lookup(*it);
		if ( player == nullptr || player->isSpawned() )
		{
			_spawning.erase(it);
			continue;
		}
		UINT32 cellHost = player->getCellHost();
		if ( cellHost == 0 )
			continue;
		if ( g_debugPersonas )
			dbugLog(_FLD, "t:%d p:%u c:%s has transferred to cell#%u", player->teulIndex(), player->playerID(), player->callsign(), cellHost);
		player->SetSpawned(true);
		_spawning.erase(it);
	}

	return true;
}

//////////////////////////////////////////////////////////////////////
// Wrapper for setSpawned -- don't let someone get flagged as spawned
// unless they are connected to s cell host

static bool
_flagClientSpawned(Player* player)
{
	UINT32 cellHost = player->getCellHost();
	if ( cellHost == 0 )
		return false;

	HANDLES::iterator it = _spawning.find(player->matchHandle());

	if ( it != _spawning.end() )
		_spawning.erase(it);

	if ( g_debugPersonas )
		dbugLog(_FLD, "t:%d p:%u c:%s has completed transfer to cell#%u", player->teulIndex(), player->playerID(), player->callsign(), cellHost);

	player->SetSpawned(true);

	return true;
}

//////////////////////////////////////////////////////////////////////
// Set the player's connection up as required when we detect that they
// have spawned.

void
Player::SetSpawned(bool newValue)
{
	if ( isSpawned() == newValue )
		return;

	m_isSpawned = newValue;

	if ( newValue == true )
	{
		m_lastCP = chatFindProperty(originCpID());

		// If they have a non-zero update mode, then update their vis mode
		chatSendBandwidthNotice(*this, getPreference(CPR_UPDATE_MODE));
		chatSendNetcodeNotice(*this, getPreference(CPR_SET_NETCODE));

		if ( m_snoopBy != nullptr )
		{
			PREP_CHAT_MSG(cmsg, "Snooped player %s is spawning", callsign());
			chatDispatchGMMessage("SNOOP", cmsg);
		}
		else if ( m_crsSnoopBy != nullptr )
		{
			PREP_CHAT_MSG(cmsg, "CRSSnooped player %s is spawning", callsign());
			chatDispatchCRSMessage("SNOOP", cmsg);
		}
	}
	else
	{
		m_capping = nullptr;

		removeFromAWS();
		removeFromGrid();
	}

	updatePermMask();

	chatMissionUpdateMember(this);

	updateSquadEntry(false);
}

void
Player::SetSquad(UINT32 toSquadID, RANK_NO newRank, const ChatHandle handle, UINT32 joined)
{
	if ( m_squadID != toSquadID )
		m_announcedToSquad = false;

	if ( m_squadID != toSquadID || handle != m_squadHandle )
	{
		m_lastSquadInfo_ms = 0;
		m_squadID = toSquadID;
		m_squadHandle = handle;
		chatSendMapPersonaUpdate(playerID(), true, m_squadID, handle, false, 0);
	}

	if ( newRank != m_realSquadRank )
	{
		RANK_NO oldRank = m_realSquadRank;
		m_realSquadRank = newRank;
		if ( newRank == Squad::CO || oldRank == Squad::CO )
			updatePermMask();
	}

	if ( toSquadID == 0 || joined > m_joinedSquad )
		m_joinedSquad = joined;
}

//////////////////////////////////////////////////////////////////////
// Change the squad invite

void
Player::SetSquadInvite(SquadInvite* invite)
{
	delete m_squadInvite;
	m_squadInvite = invite;
}

//////////////////////////////////////////////////////////////////////
// Set the player's current weapon and the flag indicating whether or
// not it is a buzzard

void
Player::SetWeapon(const WEAPON_SPEC toWeapon)
{
	dbugLog(_FLD, "Weapon was %d.%d.%d.%d", m_weapon.wType, m_weapon.wClass, m_weapon.wCategory, m_weapon.wCountry);
	m_weapon = toWeapon;
	dbugLog(_FLD, "Weapon is now %d.%d.%d.%d", m_weapon.wType, m_weapon.wClass, m_weapon.wCategory, m_weapon.wCountry);

	m_isBuzzard = (m_weapon.wCategory == 4 && m_weapon.wClass == 9 && (m_weapon.wType == 4 || m_weapon.wType == 5));
}

void Player::GetWeaponStr(std::string &str) const
{
	str.append(std::to_string(m_weapon.wType));
	str.append(".");
	str.append(std::to_string(m_weapon.wClass));
	str.append(".");
	str.append(std::to_string(m_weapon.wCategory));
	str.append(".");
	str.append(std::to_string(m_weapon.wCountry));
}

//////////////////////////////////////////////////////////////////////
// Sever this player's recruiter link to all the players he is
// currently recruiting (if any) and notify them

void
Player::stopRecruiting()
{
	RECRUITS::iterator rit = _recruits.find(playerID());
	if ( rit == _recruits.end() )
		return;

	// Send a message to all of my recruits
	auto& recruits = rit->second;
	if ( !recruits.empty() )
	{
		std::vector<SINT32> indexes;
		for ( auto it = recruits.begin(), end = recruits.end(); it != end; ++it )
		{
			Player* player = PlayerInterface::Lookup(*it);
			if ( player != nullptr && player->teulIndex() >= 0 )
				indexes.push_back(player->teulIndex());
		}

		if ( indexes.size() > 0 )
		{
			chatDistributeLocMessage(indexes.begin(), indexes.end()
										, LSTR_CAUTION, LSTR_RECRUITER_UNAVAILABLE, callsign());
		}
	}

	_recruits.erase(rit);
}

//////////////////////////////////////////////////////////////////////
// Determine how long the player has been on their current mission

TeulTimeMS
Player::timeOnMission() const
{
	if ( missionID() == 0 )
		return 0LL;
	return (teulClock64 - m_joinedMission);
}

//////////////////////////////////////////////////////////////////////
//Sends a message to a player with ignore checking

SINT32
Player::trySend(Chat::HostMessage& outgoing)
{
	if ( ignoring(outgoing.handle) )
	{
		if ( g_debugChat )
			dbugLog(_FLD, "p:%u c:%s message %u ignored", m_playerID, callsign(), outgoing.id);

		return 1;
	}

	return send(outgoing);
}


//////////////////////////////////////////////////////////////////////
// PlaceHolders allow a particular radio slot to hold a particular
// channel type when a player is not in a position to have the
// real channel tuned on that slot. E.g. before you select a
// brigade, you can't be "on" the brigade channel. Instead we put
// a place holder on the radio slot so that when we try to tune
// you to brigade, we already have a slot ear marked for it.

void
Player::tunePlaceHolder(UCHAR index, CHANNEL_TYPE type)
{
	if ( type == NO_CHANNEL )
	{
		LogAbort("t:%d p:%u c:%s attempting to tune radio %d to NO_CHANNEL placeholder", teulIndex(), playerID(), callsign(), index);
	}

	if ( g_debugRadios )
		dbugLog(_FLD, "t:%d p:%u c:%s tuning radio %d to %s placeholder", teulIndex(), playerID(), callsign(), index, chanTypeName[type]);

	_setChannel(index, type, nullptr, false);
}

//////////////////////////////////////////////////////////////////////
// Update the receive status of a particular radio slot

bool
Player::updateChannelStatus(const Chat::UpdateChannel& update)
{
	if ( g_debugRadios )
		dbugLog(_FLD, "t:%d p:%u c:%s channel update for #%d status %d"
						, teulIndex(), playerID(), callsign()
						, update.radioIndex, update.receiveStatus);

	UINT32 index = update.radioIndex;

	if ( index >= MAX_CLIENT_CHANNELS )
	{
		dbugLog(_FLW, "t:%d p:%u c:%s invalid radio index in update: %d"
						, teulIndex(), playerID(), callsign()
						, index);
		return false;
	}

	Chat::ChannelSubscription& cnode = this->getChannelSub(index);
	if ( cnode.isPlaceHolder() )
	{
		chatReportPlaceHolder(this, cnode.type() );
		return false;
	}

	if ( cnode.isTuned() == false )
	{
		dbugLog(_FLW, "t:%d p:%u c:%s #%d no channel on that radio"
						, teulIndex(), playerID(), callsign()
						,index);
		return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////
// Update a player's permission mask

void
Player::updatePermMask()
{
	const bool wasTrainer = isTrainer();

	if ( this->squadID() && this->squadRank() == Squad::CO )
		m_playerMask |= SQUAD_CO;
	else
		m_playerMask &= ~SQUAD_CO;
	if ( isSpawned() )
		m_playerMask |= IS_SPAWNED;
	else
		m_playerMask &= ~IS_SPAWNED;
	if ( isActiveOfficer() )
		m_playerMask |= ACTIVE_OFFICER;
	else
		m_playerMask &= ~ACTIVE_OFFICER;

	m_permissionMask = m_playerMask | m_chatPermissions->m_permissionMask;

	updateCommandStatus();

	const bool isTrainerNow = isTrainer();
	if ( wasTrainer != isTrainerNow )
		chatUpdateTrainerStatus(playerID(), isTrainerNow);
}

//////////////////////////////////////////////////////////////////////
// As a player moves around their persona/brigade/mission selection
// we have to validate the radio channel tunings and possibly
// change channels to be place holders

void
Player::updateRadios(bool newInstance, bool newSide, bool newBrigade, bool newMission, bool newOrigin, bool newObjective)
{
	if ( !haveChannels() )
	{
		tuneInitialChannels();
		return;
	}

	for ( UCHAR index = 0; index < MAX_CLIENT_CHANNELS; ++index )
	{
		Chat::ChannelSubscription& node = this->getChannelSub(index);
		bool setPlaceHolder = false;
		bool setRealChannel = false;
		bool retune = false;

		// We need to know what channel type we are for the duration
		// of this call, as some operations may clear out node
		CHANNEL_TYPE type = node.type();

		switch (type)
		{
		case NO_CHANNEL:
			// Not tuned
		break;
		case ADMIN_CHANNEL:
			// Nothing to do
			if ( !isGM() )
				detuneRadio(index);
		break;
		case BRIGADE_CHANNEL:
			if ( !node.isPlaceHolder() && !brigadeID() )
				setPlaceHolder = true;
			else if ( node.isPlaceHolder() && brigadeID() )
				setRealChannel = true;
			else if ( newBrigade | newInstance )
				retune = true;
		break;
		case MISSION_CHANNEL:
			if ( !node.isPlaceHolder() && !missionID() )
				setPlaceHolder = true;
			else if ( node.isPlaceHolder() && missionID() )
				setRealChannel = true;
			else if ( newMission | newInstance )
				retune = true;
		break;
		case CP_CHANNEL:
			dbugLog(_FLA, "t:%d p:%u c:%s player managed to get CP channel tuned"
						, teulIndex(), playerID(), callsign());
			detuneRadio(index);
		break;
		case OBJECTIVE_CP_CHANNEL:
			if ( !node.isPlaceHolder() && !(missionID() && inFacility()) )
				setPlaceHolder = true;
			else if ( node.isPlaceHolder() && missionID() && inFacility() )
				setRealChannel = true;
			else if ( newObjective | newInstance )
				retune = true;
		break;
		case ORIGIN_CP_CHANNEL:
			if ( !node.isPlaceHolder() && !(missionID() && inFacility()) )
				setPlaceHolder = true;
			else if ( node.isPlaceHolder() && missionID() && inFacility() )
				setRealChannel = true;
			else if ( newOrigin )
				retune = true;
		break;
		case SQUAD_CHANNEL:
			if ( !squadID() && !node.isPlaceHolder() )
				setPlaceHolder = true;
			else if ( newSide | newInstance )
				retune = true;
		break;
		case NUMBERED_CHANNEL:
			if ( newSide && node.channel() )
			{
				const ChannelID& id = node.channel()->channelID;
				if ( side() != id.Side() || newInstance )
					retune = true;
				else
					detuneRadio(index);
			}
		break;

		case SIDE_GENERAL_CHANNEL:
		case SIDE_HELP_CHANNEL:
		case SIDE_OPS_CHANNEL:
			if ( !(newSide | newInstance) )
				continue;
			if ( noSide() && !node.isPlaceHolder() )
				setPlaceHolder = true;
			else if ( !noSide() && node.isPlaceHolder() )
				setRealChannel = true;
			else
				retune = true;
		break;

		default:
			dbugLog(_FLA, "t:%d p:%u c:%s unknown channel type %d"
							, teulIndex(), playerID(), callsign()
							, type);
		break;
		}

		if ( setPlaceHolder )
		{
			detuneRadio(index);
			tunePlaceHolder(index, type);
		}
		else if ( setRealChannel )
		{
			tuneRadio(index, type, 0);
		}
		else if ( retune )
		{
			detuneRadio(index);
			tuneRadio(index, type, 0);
		}
	}
}

//////////////////////////////////////////////////////////////////////
// Update a player with an incoming locality packet

const char*
_nameForCP(const Property* nearCP, const Property* lastCP)
{
	static char buf[80+1];
	if ( nearCP != nullptr )
		return nearCP->Name();
	if ( lastCP == nullptr )
		return "<wilderness>";
	strprintf(buf, "<last near %s>", lastCP->Name());
	return buf;
}

#include "wwii/wwiiOhYeahCodes.h"

void
Player::updateLocality(FLOAT64 x, FLOAT64 y, FLOAT64 z
						, bool cpChanged, const FULL_OBJ_ID nearCP
						, UINT32 ride
						, const FULL_OBJ_ID nowInFacility
						, UINT32 lastPing
						, bool isOnAWS
						, bool inCapBldg
						, const FULL_OBJ_ID capFac
						, SINT32 progress
						)
{
	if ( x >= INVALID_GRID_COORD )
		return;

	// Feh. We send ourselves this when the player is hacking wwiiCovert
DISABLE_WARNING("-Wfloat-equal")
    if ( x == 39 && y == 40 && z == 41 && nearCP.id == 42 )
	{
		///TODO:COVERT:
		if ( !m_invalidData )
		{
			switch ( (Chat::ModReport)nearCP.cell.ox )
			{
			case OH_YEAH_CHECKS:
				Chat::SendModReport(chatHandle(), Chat::ModHack);
			break;
			case OH_YEAH_DAMNED:
				Chat::SendModReport(chatHandle(), Chat::ModDamned);
			break;
			case OH_YEAH_COVERT:
			default:
				Chat::SendModReport(chatHandle(), Chat::ModTamper);
			break;
			}
			m_invalidData = true;
		}
		return;
	}

	m_impl->m_lastPing = lastPing;
	bool capBldgChanged = false;
	if ( (m_capping == nullptr && inCapBldg == true) || (m_capping != nullptr && inCapBldg == false) )
	{
		capBldgChanged = true;
		m_capping = nullptr;
	}
	if ( isSpawned() && m_lastX == x && m_lastY == y && m_lastZ == z && cpChanged == false && isOnAWS == m_impl->onAWS() && capBldgChanged == false && progress == m_capProgress )
		return;
POP_WARNINGS()

	Property* nearest = (cpChanged) ? chatFindProperty(nearCP) : m_lastCP;

	// Do we know they've spawned yet
	if ( isSpawned() == false )
	{
		if ( g_debugPersonas )
			dbugLog(_FLD, "t:%d p:%u c:%s marking player as spawned %f, %f, %f"
							, teulIndex(), playerID(), callsign(), x, y, z);
		if ( _flagClientSpawned(this) == false )
			return;
		if ( nearest == nullptr )
			nearest = m_lastCP;
	}

	if ( nowInFacility.id == 0 )
		m_insideFacility = nullptr;
	else if ( m_insideFacility == nullptr || m_insideFacility->Fid().id != nowInFacility.id )
	{
		m_insideFacility = chatFindProperty(nowInFacility);
	}

	m_lastX = x;
	m_lastY = y;
	m_lastZ = z;

	if ( nearest != nullptr )
	{
		if ( nearest != m_lastCP )
		{
			m_lastCP = nearest;
			updateSquadEntry(false);
		}
	}
	else
	{
		if ( memcmp(&nearCP, &nullObjID, sizeof(FULL_OBJ_ID)) != 0 )
		{
			dbugLog(_FLI, "%s[%u] reporting invalid nearCP at " PRF_FULL_OBJ_ID ""
							, callsign(), playerID()
							, PRA_FULL_OBJ_ID(nearCP)
						);
		}
	}

	updateAWSPosition(isOnAWS);
	updateGridPosition();

	chatMissionUpdateMember(this);

	if ( inCapBldg )
	{
		if ( m_capping == nullptr )
			m_capping = (PropFacility*)chatFindProperty(capFac);
	}
	else
	{
		m_capping = nullptr;
	}

	m_capProgress = progress;
}

//////////////////////////////////////////////////////////////////////
// Apply same-side changes to a persona (cp, objective, mission)
// and effect apropriate channel tuning

extern void chatReportFriendLogin(class Player*);

bool
Player::updatePersona(const Chat::PersonaUpdate& update)
{
	bool squadEntryNeedsUpdate = false;

	SetWeapon(update.m_weapon);

	if ( update.m_country == WWII_NO_COUNTRY )
	{
		SetSpawned(false);
		m_inFacility = false;
		m_originCpID = m_objectiveCpID = nullObjID;
		squadEntryNeedsUpdate = true;
	}
	else
	{
		if ( update.m_isSpawned && isSpawned() == false )
		{
			if ( g_debugPersonas )
				dbugLog(_FLD, "t:%d p:%u c:%s has begun cell transfer", teulIndex(), playerID(), callsign());
			_spawning.insert(matchHandle());
		}
	}

	if ( g_debugPersonas )
	{
		WEAPON_SPEC ws = m_weapon;
		dbugLog(_FLD, "t:%d p:%u c:%s update c:%u, b:%u, br:%u, m:%u, or:" PRF_FULL_OBJ_ID ", ob:" PRF_FULL_OBJ_ID " spawned:%d weap:" PRF_WEAPON_SPEC ""
						, teulIndex(), playerID(), callsign()
						, update.m_country, update.m_branch, update.m_brigadeID, update.m_missionID
						, PRA_FULL_OBJ_ID(update.m_origin)
						, PRA_FULL_OBJ_ID(update.m_target)
						, (int)update.m_isSpawned
						, PRA_WEAPON_SPEC(ws)
						);
	}

	bool sideChanged = false;
	WWII_SIDE newSide = wwiiSide(update.m_country);
	bool firstLogin = false;

	if ( newSide != m_prevSide )
	{
		dbugLog(_FLI, "removing %p from %d side index", this, m_prevSide);
		sidePlayers[m_prevSide].erase(this);
	}

	if ( isValidSide(newSide) )
	{
		if ( newSide != m_prevSide )
		{
			sideChanged = true;
			if ( m_prevSide == WWII_NO_SIDE )
				firstLogin = true;
		}

		if ( newSide != side() )
		{
			dbugLog(_FLI, "insert %p to %d side index", this, newSide);
			sidePlayers[newSide].insert(this);
		}

		m_prevSide = newSide;
	}

	// If player changes side while being recruited...
	if ( sideChanged && !firstLogin && m_impl->m_recruiter )
		unrecruit(LSTR_RECRUIT_CHANGED_SIDE);

	if ( country() == WWII_NO_COUNTRY && update.m_country != WWII_NO_COUNTRY )
	{
		SetCountry(update.m_country, newSide);
		squadEntryNeedsUpdate = true;
	}

	if ( _applyPersonaUpdate(update, sideChanged) )
		squadEntryNeedsUpdate = true;

	// Once a player exits a facility, forget what their last CP was.
	if ( !inFacility() )
	{
//		m_lastCP = NULL;
		m_insideFacility = nullptr;
	}

	// Read groups and apply permissions as apropriate
	checkGroups();

	if ( !m_initialized )
	{
		dbugLog(_FLI, "t:%d p:%u c:%s logged in", teulIndex(), playerID(), callsign());

		squadEntryNeedsUpdate = true;

		chatReportFriendLogin(this);

		m_initialized = true;
	}

	if ( firstLogin )
	{
		static SCHAR note[81];
		bool notManagement = memcmp("CRS", callsign(), 3) != 0;
		if ( isPlaynet() && notManagement )
		{
			strprintf(note, "%s has logged in", callsign());		///LSTR_SIGNED_ON
			chatDispatchCRSMessage("SYS", note, matchHandle());
			const char* motd = chatGetUnitMotd(CRS_MOTD_UNIT);
			if ( motd != nullptr && *motd != 0 )
				message(LSTR_NONE, LSTR_CRS_MOTD, motd);
		}
		if ( isEffectiveGM() )
		{
			if ( !permissions(DARK_GLASSES) && (isExplicitGM() == true || isPlaynet() == false) )
			{
				strprintf(note, "%s has logged in", callsign());	///LSTR_SIGNED_ON
				chatDispatchGMMessage("SYS", note, matchHandle());
			}
			const char* motd = chatGetUnitMotd(GM_MOTD_UNIT);
			if ( motd != nullptr && *motd != 0 )
				message(LSTR_NONE, LSTR_GM_MOTD, motd);
		}
		if ( !permissions(DARK_GLASSES) && isActiveCommander() && isPlaynet() == false && isExplicitGM() == false )
		{
			Chat::LocalizedMessage lmsg(LSTR_NONE, LSTR_SIGNED_ON, callsign());
			chatHCMessage(side(), lmsg, matchHandle());
		}

		chatSendCommandListPacket(this);
	}

	if ( squadEntryNeedsUpdate )
		updateSquadEntry(false);

	return true;
}

//////////////////////////////////////////////////////////////////////
// This player is no-longer an acting recruit

void
Player::unrecruit(LSTRING_ID notifyRecruiter)
{
	if ( m_impl->m_recruiter == 0 )
		return;

	RECRUITS::iterator it = _recruits.find(m_impl->m_recruiter);
	if ( it != _recruits.end() )
	{
		it->second.erase(playerID());
		if ( it->second.empty() )
			_recruits.erase(it);
	}

	if ( notifyRecruiter != LSTR_NONE )
	{
		Player* player = PlayerInterface::Lookup(m_impl->m_recruiter);
		if ( player != nullptr )
			player->message(LSTR_NOTE, notifyRecruiter, callsign());
	}

	m_impl->m_recruiter = 0;
}

//////////////////////////////////////////////////////////////////////
// Find a valid value for a preference based on a client request.

UINT32
Player::validatePreference(UINT32 prefId, UINT32 request) const
{
	switch ( prefId )
	{
		case CPR_UPDATE_MODE:
			return _validateUpdateMode(request);
		default:
			return request;
	}
}

//////////////////////////////////////////////////////////////////////
// Describe the whereabouts of a player

void
Player::whereabouts(std::string &into, const FLOAT64& x, const FLOAT64& y) const
{
	char tmpbuf[MAX_MESSAGE_LENGTH+1];

	if ( noSide() )
	{
		into.append("PersonaScrn");
		into.append(lastPosition("last near", x, y, true));
		return;
	}

	const char* const affiliation = (country() == WWII_NO_COUNTRY)
					? _sideNameAdj[side()]
					: _countryNameAdj[country()]
					;

	// He's not in a facility, ergo at the map
	if ( !missionID() )
	{
		strprintf(tmpbuf, "%s brigade scrn", affiliation);
		into.append(tmpbuf);
		into.append(lastPosition("last near", x, y, true));
		return;
	}

	into.append(affiliation);
	into.append(" ");

	HCUnit* unit = HCUnit::Lookup(brigadeID());
	if ( unit != nullptr )
	{
		into.append(unit->name());
		into.append(" ");
	}

	ChatMission* mission = chatFindMission(missionID());
	if ( mission )
		into.append(Chat::Name(mission->leader()).c_str());
	else
		into.append("unknown");
	into.append(" mission ");

	// He's started the spawning process
	UINT32 cellIP = getCellHost();
	if ( cellIP == 0 )
	{
		into.append(originCP());
		into.append(" mission room");
		into.append(lastPosition("last posn. ", x, y, false));
	}
	else
	{
		into.append("cell#");
		into.append(std::to_string((cellIP & 0xff)));
		// He may not have finished spawning
		if ( !isSpawned() )
		{
			into.append(" spawning ");
			into.append(originCP());
		}
		else
		{
			if ( sortieID() == 0 )
				into.append("?missingsortieid?");
			else
			{
				into.append("sortie #");
				into.append(std::to_string(sortieID()));
			}

			if ( isBuzzard() )
				into.append(" buzzard");
			into.append(lastPosition("near ", x, y, false));
			if ( lastCP() == nullptr || lastCP()->Matches(originCpID()) == false )
			{
				into.append(" origin: ");
				into.append(originCP());
			}
		}
	}

	if ( isSpawned() && m_impl->m_lastPing > 0 )
	{
		snprintf(tmpbuf, sizeof(tmpbuf), " ping:%ums", m_impl->m_lastPing);
		into.append(tmpbuf);
	}
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//// TEMPORARY HOME OF THE AWS SYSTEM
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// Make a UINT32 index for the grid

static inline UINT32
_awsGridIndex(UINT16 x, UINT16 y)
{
	return (y << 16) | x;
}

//////////////////////////////////////////////////////////////////////
// Populate ann AWS::Grid with instance specific AWS readings.

static void
_populateAWSReply(Player& player, const AWS::Request& request, const AWS_GRID& grid, AWS::Grid& reply)
{
	UINT16 awsY = request.m_row;
	UINT16 awsLeftX = request.m_startCol;
	UINT16 awsRightX = awsLeftX + AWS::MAX_COLUMNS;
	UINT16 awsTopY = awsY + AWS::MAX_COLUMNS;
	UINT16 populatedRows = 0;
	UINT16 populatedCells = 0;

	bool isAllied = (player.side() == ALLIED);

	dbugLog(_FLD, "t:%d p:%u c:%s in:%u.%u.%" PR64BitUT ".%" PR64BitUT " requesting %s AWS [%hu,%hu]-[%hu,%hu](%u)"
					, player.teulIndex(), player.playerID(), player.callsign()
					, player.instance().category(), player.instance().type(), player.instance().handle(), player.instance().id()
					, (isAllied ? "axis" : "allied")
					, request.m_startCol, request.m_row
					, awsRightX, awsTopY
					, AWS::MAX_COLUMNS);

	// Find all the rows in the square requested.
	for ( UINT16 curY = awsY; curY < awsTopY; ++curY )
	{
		// Find the next populated row.
		AWS_GRID::const_iterator it = grid.lower_bound(_awsGridIndex(awsLeftX, curY));
		if ( it == grid.end() )
			break;
		curY = (UINT16)((it->first) >> 16);
		// Beyond the top of the window?
		if ( curY >= awsTopY )
			break;

		UINT16 thisX = (UINT16)(it->first);
		// Is it within the window bounds?
		if ( thisX >= awsRightX )
			continue;

		UINT16 row = curY - awsY;

		for ( UINT16 x = 0; x < AWS::MAX_COLUMNS; ++x )
		{
			if ( awsLeftX + x == thisX )
			{
				UINT32 reading = (isAllied) ? it->second.axis : it->second.allied;

				++it;
				thisX = (UINT16)(it->first);

				if ( reading > 0 )
				{
					UCHAR value = (reading < host_config.update.awsHighThreshold) ? AWS::LIGHT_PRESENCE : AWS::HEAVY_PRESENCE;
					reply.m_rowHasData[row] = true;
					reply.m_data[row][x] = value;
					++populatedCells;

					continue;
				}
			}
			reply.m_data[row][x] = 0;
		}

		reply.m_hasData = true;
		++populatedRows;
	}

    dbugLog(_FLD, "t:%d p:%u c:%s %s aws request [%u-%u,%u] data:%s rows:%u, cells:%u"
                    , player.teulIndex()
                    , player.playerID()
                    , player.callsign()
                    , (isAllied ? "allied" : "axis")
                    , static_cast<UINT32>(request.m_startCol), static_cast<UINT32>(request.m_startCol) + AWS::MAX_COLUMNS, static_cast<UINT32>(request.m_row)
                    , (reply.m_hasData ? "yes" : "no")
                    , populatedRows, populatedCells
                    );
}

//////////////////////////////////////////////////////////////////////
// Process an AWS Grid Request

void
chatProcessAWSRequest(Player& player, const AWS::Request& request)
{
	AWS::Grid reply(request.m_row, request.m_startCol);
	AWS_INSTANCES::iterator it = g_awsSnapshots.find(player.instance().id());
	if ( it != g_awsSnapshots.end() )
	{
		_populateAWSReply(player, request, it->second, reply);
	}

	teulSend(player.teulIndex(), AWS::callback, &reply);

	player.m_lastAWSRequest = teulClock64;
}

//////////////////////////////////////////////////////////////////////
// Update the player's entry on the AWS grid

void
Player::updateAWSPosition(bool onAWS)
{
	// Is the player being removed from the AWS system?
	if ( onAWS == false )
	{
		removeFromAWS();
		return;
	}

	SINT16 awsX = (SINT16)(lastX() / AWS::CELL_WIDTH_M) + AWS::GRID_OFFSET_LEFT;
	SINT16 awsY = (SINT16)(lastY() / AWS::CELL_HEIGHT_M) - AWS::GRID_OFFSET_UP;

	if ( m_impl->onAWS() == true )
	{
		if ( awsX == m_impl->m_awsX && awsY == m_impl->m_awsY )
			return;

		// Must be a position update
		removeFromAWS();
	}

	if ( side() != ALLIED && side() != AXIS )
		return;

	if ( awsY < 0 || awsX < 0 )
	{
		dbugLog(_FLW, "t:%d p:%u c:%s player off map [%f,%f => %d,%d]"
						, teulIndex(), playerID(), callsign()
						, lastX(), lastY()
						, awsX, awsY
						);
	}
	else
	{
		dbugLog(_FLD, "t:%d p:%u c:%s in:%u.%u.%" PR64BitUT ".%" PR64BitUT " aws side %u update %f.%f => %hd/%hd"
						, teulIndex(), playerID(), callsign()
						, instance().category(), instance().type(), instance().handle(), instance().id()
						, side()
						, lastX(), lastY()
						, awsX, awsY
						);
	}

	AWS_READING& reading = g_awsGrids[instance().id()][_awsGridIndex(awsX, awsY)];
	if ( side() == ALLIED )
		++(reading.allied);
	else
		++(reading.axis);

	m_impl->m_awsSide = side();
	m_impl->m_awsX = awsX;
	m_impl->m_awsY = awsY;
}

//////////////////////////////////////////////////////////////////////
// Remove a player from the AWS grid if they are on it

void
Player::removeFromAWS()
{
	if ( m_impl->onAWS() == false )
		return;

	UINT32 index = _awsGridIndex(m_impl->m_awsX, m_impl->m_awsY);
	AWS_INSTANCES::iterator git = g_awsGrids.find(instance().id());
	if ( git != g_awsGrids.end() )
	{
		AWS_GRID& grid = git->second;
		AWS_GRID::iterator it = grid.find(index);
		if ( it != grid.end() )
		{
			auto& awsReading = it->second;
			UINT16& reading = (m_impl->m_awsSide == ALLIED) ? awsReading.allied : awsReading.axis;
			if ( reading > 0 )
				--reading;
			if ( awsReading.allied <= 0U && awsReading.axis <= 0U )
				grid.erase(it);
		}

		///TODO:INST: Do this when the instance goes away rather than
		/// when players despawn.
		if ( grid.empty() && instance().id() != Instance::_CAMPAIGN )
		{
			g_awsGrids.erase(git);
			g_awsSnapshots.erase(instance().id());
		}
	}

	dbugLog(_FLD, "t:%d p:%u c:%s in:%u.%u.%" PR64BitUT ".%" PR64BitUT " aws side %u extraction %f.%f => %d/%d"
					, teulIndex(), playerID(), callsign()
					, instance().category(), instance().type(), instance().handle(), instance().id()
					, m_impl->m_awsSide
					, lastX(), lastY()
					, m_impl->m_awsX, m_impl->m_awsY
					);

	m_impl->m_awsSide = WWII_NO_SIDE;
	m_impl->m_awsX = m_impl->m_awsY = ~0;
}

//////////////////////////////////////////////////////////////////////
// Periodically transfer the data from the "live" AWS information
// to the display snapshot

void
chatUpdateAWSSnapshot()
{
	static TeulTimeMS nextCheck = 0;
	if ( teulClock64 < nextCheck )
		return;
	nextCheck = teulClock64 + (AWS::UPDATE_INTERVAL_S * 1000);

    const AWS_GRID& awsSnapshot = g_awsSnapshots[Instance::_CAMPAIGN];
    const AWS_GRID& awsGrid = g_awsGrids[Instance::_CAMPAIGN];
    UINT16 oldX = 0, oldY = 0;
    UINT16 newX = 0, newY = 0;
    if ( awsSnapshot.empty() == false )
    {
        oldX = awsSnapshot.begin()->first & 0xff;
        oldY = awsSnapshot.begin()->first >> 16;
    }
    if ( awsGrid.empty() == false )
    {
        newX = awsGrid.begin()->first & 0xff;
        newY = awsGrid.begin()->first >> 16;
    }
    dbugLog(_FLD, "Updating aws grid: Old grid %" PRSizeT " entries starting %hu,%hu; New grid %" PRSizeT " entries starting %hu,%hu"
                    , awsSnapshot.size(), oldX, oldY
                    , awsGrid.size(), newX, newY
                    );

	for (auto it = g_awsGrids.begin(), end = g_awsGrids.end(); it != end; ++it)
	{
		g_awsSnapshots[it->first] = it->second;
	}
}

//////////////////////////////////////////////////////////////////////
// Return the number of people a player is currently holding as recruits

UINT32
chatGetNumberOfRecruits(UINT32 playerID)
{
	RECRUITS::iterator it = _recruits.find(playerID);
	if ( it == _recruits.end() || it->second.empty() )
		return 0;
	return (UINT32)it->second.size();
}

//////////////////////////////////////////////////////////////////////
// Save all the login/logout entries

bool
chatSaveLogRecords()
{
	if ( !_logins.empty() )
	{
		char query[TEXT_POOL_SIZE + 1];
		char* qp = query + strlcpy(query, "UPDATE wwii_player SET last_login=NOW() WHERE playerID IN (", TEXT_POOL_SIZE + 1);
		auto it = _logins.begin();
		for ( auto end = _logins.end(); it != end && qp < query + TEXT_POOL_SIZE - 24; ++it )
		{
			if ( it != _logins.begin() )
				*(qp++) = ',';
			qp += sprintf(qp, "%u", *it);
		}
		*(qp++) = ')';
		*(qp++) = 0;
		(new DBThreadQuery(DBThread::Default, "SaveLogins", query, DBThreadQuery::_IMMEDIATE))->Queue();

		// Erase all entries below the last one we used - which will either be
		// end() or the first entry we didn't send. Remember, set::end() is
		// a pointer* beyond* the end of the list, so for erase(begin(),end())
		// to work, it must stop before the second iterator.
		_logins.erase(_logins.begin(), it);
	}

#ifdef RECORD_LOGOUTS
	if ( !_logouts.empty() )
	{
		_logouts.clear();
	}
#endif

	return true;
}

//////////////////////////////////////////////////////////////////////
///// UNSORTED WORK IN PROGRESS

Player*
PlayerInterface::Lookup(UINT32 playerID)
{
	Player* player = (Player*)ServerInterface::LookupByPlayerID(playerID);
	if ( player != nullptr )
		return player;
	return (Player*)Chat::ManagementIDAllocator::Lookup(playerID);
}

Player*
PlayerInterface::Lookup(const ChatHandle handle)
{
	PLAYER_INDEX::iterator it = playerIndex.find(chatGetLowercaseHandle(handle));
	return ( it == playerIndex.end() ) ? nullptr : it->second;
}
