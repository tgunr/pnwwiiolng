#include <syscfg.h> // Path is project specific.
// Created: Tue Jun 28 16:11:04 CDT 2005
//
// Functions for managing mission data
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "versionNo.h"

#include "chat/chatBroadcastObject.h"
#include "chat/chatChannelID.h"
#include "chat/chatChannelList.h"
#include "chat/chatChannelNumber.h"
#include "chat/chatDotCommand.h"
#include "chat/chatHost.h"
#include "chat/chatHostMessages.h"
#include "chat/chatList.h"
#include "chat/chatLocalizedMessage.h"
#include "chat/chatLStringErr.h"
#include "chat/chatMissions.h"
#include "chat/chatMissionTypes.h"
#include "chat/chatName.h"
#include "chat/chatPlayer.h"
#include "chat/chatPreferenceEnum.h"
#include "chat/chatServerTrackedObjects.h"
#include "chat/chatTimeInterval.h"
#include "chat/chatWaypointExchg.h"
#include "chat/chatWaypoint.h"
#include "include/mydbQuery.h"
#include "miss/missionMobileSpawnWaypoint.h"
#include "omg/omgChatMissionStatus.hxx"
#include "server/serverPeers.h"
#include "str/stringLib.h"
#include "teul/teulEndpointDefs.h"
#include "teul/teulServer.h"
#include "wwii/wwiiConfig.h"
#include "wwii/wwiiDBThreads.h"
#include "wwii/wwiiHash.h"
#include "wwii/wwiiServerTrackedObject.h"
#include "wwii/wwiiThreads.h"
#include "wwii/wwiiThreadText.h"

typedef std::map<UINT32, ChatMission*> CHAT_MISSIONS;
typedef std::set<UINT32> MISSION_STATUSES;

CHAT_MISSIONS g_missions;
MISSION_STATUSES g_statuses;

//////////////////////////////////////////////////////////////////////
// Retrieve the currently configured mobile spawn handle

static inline
ChatHandle
_getMSPHandle()
{
	if ( host_config.arena.mobileSpawnWaypoint[0] == 0 )
	{
		static ChatHandle defaultHandle = chatGetHandle("MSPAWN");
		return defaultHandle;
	}

	return chatGetHandle(host_config.arena.mobileSpawnWaypoint);
}


//////////////////////////////////////////////////////////////////////
// Look a mission up but don't create it

ChatMission*
chatFindMission(UINT32 missionID)
{
	CHAT_MISSIONS::iterator it = g_missions.find(missionID);
	if ( it == g_missions.end() )
		return nullptr;
	return it->second;
}

//////////////////////////////////////////////////////////////////////
// Return a reference

ChatMission&
chatGetMission(UINT32 missionID)
{
	CHAT_MISSIONS::iterator it = g_missions.find(missionID);
	if ( it != g_missions.end() )
		return *(it->second);

	ChatMission* const mission = new ChatMission;
	mission->header().missionID(missionID);
	g_missions[missionID] = mission;

	return *mission;
}

//////////////////////////////////////////////////////////////////////
// Share a waypoint across all missions from the origin/objective

bool
chatShareWaypoint(Waypoint& waypoint, ChatMission* const mission, WWII_SIDE side)
{
#ifndef TARGET_SHARE_CONTACTS
	return mission->addWaypoint(waypoint);
#else
	if ( waypoint.type() != WPT_ENEMY || mission->targetCP() == 0LL )
		return mission->addWaypoint(waypoint);

	UINT32 missions = 0;

	if ( mission->addWaypoint(waypoint) )
		++missions;

	for ( auto it = g_missions.begin(), end = g_missions.end(); it != end; ++it )
	{
		ChatMission& target = *(it->second);
		if ( side != target.m_side || target.targetCP() != mission->targetCP() )
			continue;
		if ( &target == mission )
			continue;

		if ( target.addWaypoint(waypoint) )
			++missions;
	}

	return (missions);
#endif
}

//////////////////////////////////////////////////////////////////////
// Tell a mission who's leading it now

void
chatSetMissionLeader(UINT32 missionID, const ChatHandle& leader, WWII_SIDE side, const UINT64& origin, const UINT64& objective)
{
	ChatMission& mission = chatGetMission(missionID);

	mission.SetLeader(leader);

	mission.m_side = side;
	mission.setOrigin(origin);
	mission.setTarget(objective);
}

//////////////////////////////////////////////////////////////////////
// ChatMission constructor.

ChatMission::ChatMission()
	: Chat::MissionStatus()
	, m_ordersMsg(LSTR_NONE), m_ordersText1(""), m_ordersText2("")
	, m_originCP(0LL), m_targetCP(0LL)
	, m_stoUID(0)
	, m_matchLeader(0LL)
	, m_displacedLeader(0LL)
	, m_leaderLockFrom(teulTime()), m_leaderLockFor(600)
	, m_leaderDisplaced(0)
	, m_lastUpdateSent(0ULL)
{
}

//////////////////////////////////////////////////////////////////////
// Destructor for a mission entry

ChatMission::~ChatMission()
{
	removeMSPWaypoint(0, true);
	message(LSTR_CAUTION, LSTR_MISSION_CLOSED);
}

//////////////////////////////////////////////////////////////////////
// Set the origin for a mission.

void
ChatMission::setOrigin(const UINT64& newOrigin)
{
	m_originCP = newOrigin;
	FULL_OBJ_ID fid;
	fid.FromKey(newOrigin);
	origin().cp(fid);
}

//////////////////////////////////////////////////////////////////////
// Set the target for a mission.

void
ChatMission::setTarget(const UINT64& newTarget)
{
	m_targetCP = newTarget;
	FULL_OBJ_ID fid;
	fid.FromKey(newTarget);
	target().cp(fid);
}

//////////////////////////////////////////////////////////////////////
// Set the ORDERS for this mission

void
ChatMission::SetOrders(LSTRING_ID msg, const std::string& text1/*=""*/, const std::string& text2/*=""*/)
{
	m_ordersMsg = msg;
	m_ordersText1 = text1;
	m_ordersText2 = text2;

	if ( msg != LSTR_NONE )
		message(LSTR_ORDERS, (LSTRING_ID)msg, text1.c_str(), text2.c_str());
}

//////////////////////////////////////////////////////////////////////
// Send the mission orders to a particular player

void
chatMissionOrders(const Player* player)
{
	ChatMission* mission = chatFindMission(player->missionID());
	if ( mission == nullptr || mission->m_ordersMsg == LSTR_NONE )
		player->message(LSTR_NOTE, LSTR_NO_STANDING_ORDERS);
	else
		player->message(LSTR_ORDERS, (LSTRING_ID)mission->m_ordersMsg, mission->m_ordersText1.c_str(), mission->m_ordersText2.c_str());
}

//////////////////////////////////////////////////////////////////////
// Set the orders message for a mission and, if its non-null,
// update the mission channel.

void
chatSetMissionOrders(UINT32 missionID, const std::string& orders, bool lockLeader)
{
	if ( missionID == 0 )
		return;

	// Set the text
	dbugLog(_FLD, "m:%u mission orders set to: %s", missionID, orders.c_str());

	ChatMission& mission = chatGetMission(missionID);
	if ( orders.size() == 0 )
		mission.SetOrders(LSTR_NONE);
	else
		mission.SetOrders(LSTR_GENERIC, orders);

	if ( lockLeader )
		mission.lockLeader();
}

//////////////////////////////////////////////////////////////////////
// Remove the various data items we keep for a mission, including
// orders, waypoints, etc.

void
chatDeleteMission(UINT32 missionID)
{
	dbugLog(_FLI, "m:%u deleting mission", missionID);
	auto it = g_missions.find(missionID);
	if ( it != g_missions.end() )
	{
		delete it->second;
		g_missions.erase(it);
	}
}

//////////////////////////////////////////////////////////////////////
// When a mission channel goes empty, track it for deletion incase
// we somehow don't get the message from the map host.

void
chatMissionChannelEmpty(UINT32 /*missionID*/)
{
	///TODO: Mark this one as needing deletion sooner or later
}

//////////////////////////////////////////////////////////////////////
// Lock the leader for a period of time

void
ChatMission::lockLeader()
{
	static const UINT32 LEADER_LOCK_TIME_S = 6 * 60;
	UINT32 newLockTime = teulTime() + LEADER_LOCK_TIME_S;
	if ( newLockTime < m_leaderLockFrom + m_leaderLockFor )
	{
		m_leaderLockFrom += 15;
	}
	else
	{
		m_leaderLockFrom = teulTime();
		m_leaderLockFor = LEADER_LOCK_TIME_S;
	}
}

//////////////////////////////////////////////////////////////////////

void
chatMessageMissionChannel(UINT32 missionID, Chat::LocalizedMessage& lmsg)
{
	ChatMission* const mission = chatFindMission(missionID);
	if ( mission != nullptr && mission->m_members.empty() == false )
		chatBroadcastLocMessage(mission->m_members.begin(), mission->m_members.end(), lmsg);
}

void
ChatMission::message(LSTRING_ID prefix, LSTRING_ID messageID, const char* arg1/*=NULL*/, const char* arg2/*=NULL*/)
{
	if ( g_logDebug )
		dbugLog(_FLD, "m:%u sending mission channel message %d.%d", missionID(), prefix, messageID);

	Chat::LocalizedMessage loc(prefix, messageID, arg1, arg2);
	chatBroadcastLocMessage(m_members.begin(), m_members.end(), loc);
}

//////////////////////////////////////////////////////////////////////
// Remove the server tracked object (if any) attached to a mission

void
ChatMission::removeSTO()
{
	if ( m_stoUID == 0 )
		return;
	chatRemoveServerTrackedObject(m_stoUID);
	m_stoUID = 0;
}

//////////////////////////////////////////////////////////////////////
// Send all these waypoints to a player
void
_sendWaypoints(const ChatMission::WAYPOINTS& wp, Player* player)
{
	// None to send
	if ( wp.size() == 0 )
		return;

	// Only one to send, send it solo
	if ( wp.size() == 1 )
	{
		WAYPOINT_XFER xfer(wp.begin()->second, WPXT_POST);
		if ( xfer.send(player->teulIndex()) < 0 )
			dbugLog(_FLI, "t:%d XXX send failed", player->teulIndex());
		return;
	}

	// For any more, send them as an array
	WAYPOINT_XFER_ARRAY array(0, 0, Teul::TCPStdPayload);
	for ( auto it = wp.begin(), end = wp.end(); it != end; ++it )
	{
		array.add(it->second);
	}

	if ( array.send(player->teulIndex()) < 0 )
		dbugLog(_FLI, "t:%d XXX send failed", player->teulIndex());
}

//////////////////////////////////////////////////////////////////////
// Tell a sponsor that a recruit joined/left their mission

static void
_tellSponsor(UINT32 sponsor, const Player* player, LSTRING_ID messageID, const ChatMission& mission)
{
	if ( !sponsor || sponsor == player->playerID() )
		return;
	ChatMission::MEMBERS::const_iterator it = mission.m_members.find(sponsor);
	if ( it == mission.m_members.end() )
		return;

	it->second->message(LSTR_NOTE, messageID, player->callsign());
}

//////////////////////////////////////////////////////////////////////
// Add a player to a mission, send him all the waypoints

void
chatMissionAddPlayer(Player* player, UINT32 missionID, UINT32 sponsor)
{
	if ( missionID == 0 )
		return;

	ChatMission& mission = chatGetMission(missionID);

	// Add me to the list; god another reference to the player =(
	mission.m_members[player->playerID()] = player;

	_tellSponsor(sponsor, player, LSTR_RECRUIT_JOINED_MISSION, mission);

	betaLog(_FLI, "m:%u p:%u c:%s joining mission [%" PRSizeT " members]", missionID, player->playerID(), player->callsign(), mission.m_members.size());

	// And you generate a membership update by joining
	mission.addMemberUpdate(player);

	// Now send me the current greeting list
	mission.sendFirstUpdate(player);

	_sendWaypoints(mission.m_waypoints, player);

	// Check who should take leadership if the leader leaves
	mission.electBackupLeader();
}

//////////////////////////////////////////////////////////////////////
// Remove a player from a mission if they're on it

void
chatMissionRemovePlayer(Player* player, UINT32 missionID, UINT32 sponsor)
{
	ChatMission* const mission = chatFindMission(missionID);
	if ( mission == nullptr )
	{
		dbugLog(_FLA, "t:%d p:%u c:%s m:%u no mission data for removal"
						, player->teulIndex(), player->playerID(), player->callsign(), missionID);
		return;
	}

	mission->m_members.erase(player->playerID());

	mission->addMemberRemoval(player);

	_tellSponsor(sponsor, player, LSTR_RECRUIT_LEFT_MISSION, *mission);

	dbugLog(_FLI, "m:%u p:%u c:%s left mission [%" PRSizeT " remaining]", missionID, player->playerID(), player->callsign(), mission->m_members.size());
	if ( mission->m_members.empty() || mission->m_members.size() == 0 )
	{
		dbugLog(_FLNB, " *deleting");
		g_missions.erase(missionID);
		delete mission;
		return;
	}

	// Was this the backup leader?
	if ( player->matchHandle() == chatGetLowercaseHandle(mission->backupLeader()) )
	{
		Player* leader = PlayerInterface::Lookup(mission->m_matchLeader);
		if ( leader != nullptr )
			leader->message(LSTR_CAUTION, LSTR_BACKUP_LEADER_LEFT, player->callsign());
		mission->setBackupLeader(0);
	}

	mission->electBackupLeader();
}

#ifdef AUTO_NAME_WAYPOINTS
//////////////////////////////////////////////////////////////////////
// Determine prefix for this type of waypoint

static char*
_getWaypointTypePrefix(char* const p, WP_TYPE type, UINT32 subType)
{
	switch (type)
	{
	case WPT_MARKER:
		*(p++) = 'M';
	break;
	case WPT_RALLY:
		*(p++) = 'R';
	break;
	case WPT_ENEMY:
		*(p++) = 'E';
		switch (subType)
		{
		case WTP_ENEMY_AIR: *(p++) = 'A'; break;
		case WTP_ENEMY_SEA: *(p++) = 'B'; break;
		case WTP_ENEMY_TROOP: *(p++) = 'I'; break;
		default: break;
		}
	break;
	case WPT_ATTACK:
		*(p++) = 'A';
	break;
	case WPT_DEFEND:
		*(p++) = 'D';
	break;
	case WPT_WAYPOINT:
		*(p++) = 'W';
	break;
	case WPT_PATROL:
		*(p++) = 'P';
	break;
	default:
		*(p++) = 'x';
	break;
	}

	return p;
}

//////////////////////////////////////////////////////////////////////
// Get the waypoint handle

static ChatHandle
_getHandle(ChatMission& mission, const Waypoint& waypoint)
{
	char wpname[6] = "\0\0\0\0\0";
	char* p = wpname;

	p = _getWaypointTypePrefix(p, waypoint.type(), (UINT32)waypoint.subType());

	UCHAR& counter = mission.m_waypointNo[waypoint.type()];
	sprintf(p, "%X", ++counter);
	counter %= 31;

	return chatGetHandle(wpname);
}
#endif // AUTO_NAME_WAYPOINTS

//////////////////////////////////////////////////////////////////////
// Request a mobile spawn from the MMHost

void
ChatMission::sendMobileSpawn(UINT32 playerID, const ChatHandle& handle, FLOAT64 x, FLOAT64 y, FLOAT64 z, bool deployed, UINT32 stosUID)
{
	dbugLog(_FLD, "mswp %s m:%u p:%u sto:%u @ %0.3g,%0.3g,%0.3g", (deployed ? "cre" : "del"), missionID(), playerID, stosUID, x, y, z);

	ENDPOINT* const endpoint = serverPeerEndpointByType(MAP_HOST);
	if ( endpoint != nullptr )
	{
		MobileSpawnWaypoint wp(missionID(), playerID, handle, x, y, z, deployed, stosUID, 0);
		MOBILE_SPAWN_WAYPOINT packet(wp);
		packet.send(*endpoint);
	}
}

//////////////////////////////////////////////////////////////////////
// Add a waypoint to a mission or replace an existing entry
// NOTE: Does not send

bool
ChatMission::addWaypoint(Waypoint& waypoint)
{
	// Get a handle for it
	if ( waypoint.handle() == 0 )
#ifdef AUTO_NAME_WAYPOINTS
		waypoint.setHandle(_getHandle(*this, waypoint));
#else
		return false;
#endif // AUTO_NAME_WAYPOINTS

	// Transient waypoints need to be sent to mission members
	// without actually being retained in the mission.
	if ( waypoint.isTransient() == false )
	{
		WAYPOINTS::iterator it = m_waypoints.find(waypoint.handle());
		if ( it != m_waypoints.end() && it->second == waypoint )
			removeWaypoint(waypoint.handle(), false);

#ifdef BETA_RELEASE
		Chat::Name wpName(waypoint.handle()), wpPlayer(waypoint.playerHandle());
		dbugLog(_FLD, "m:%u adding waypoint %s from p:%s", missionID(), wpName.c_str(), wpPlayer.c_str());
#endif

		m_waypoints[waypoint.handle()] = waypoint;
	}

	// But in all cases, we're going to send this posting.
	waypoint.setPosted((UINT32)teulTime());
	WAYPOINT_XFER xfer(waypoint, WPXT_POST);

	chatBroadcastObject(m_members.begin(), m_members.end(), xfer.callback(), xfer);

	return true;
}

//////////////////////////////////////////////////////////////////////
// Remove a waypoint from the mission

bool
ChatMission::removeWaypoint(const ChatHandle& handle, bool byPlayer)
{
	WAYPOINTS::iterator it = m_waypoints.find(handle);
	if ( it == m_waypoints.end() )
		return false;
	Waypoint& waypoint = it->second;

	// If this is a player removing a mobile spawn...
	if ( byPlayer && handle == _getMSPHandle() )
	{
		betaLog(_FLD, "m:%u removing MSPAWN waypoint", missionID());
		sendMobileSpawn(0, waypoint.playerHandle(), (FLOAT32)waypoint.x(), (FLOAT32)waypoint.y(), 0.0f, false, 0);
	}

	dbugLog(_FLD, "m:%u removing waypoint %llu[%s]", missionID(), waypoint.handle(), Chat::Name(waypoint.handle()).c_str());

	if ( !m_members.empty() )
	{
		WAYPOINT_CONTROL remove(waypoint.handle(), WPCTRL_REMOVE);
		chatBroadcastObject(m_members.begin(), m_members.end(), remove.callback(), remove);
	}

	m_waypoints.erase(it);

	return true;
}

//////////////////////////////////////////////////////////////////////
// Queue an update for this player on the updates list for this mission

void
ChatMission::addMemberUpdate(Player* const player)
{
	// If there are no members, there is no update to send
	// If I am the only person on the mission, I don't need updates
	if ( m_members.size() < 1 )
		return;

#ifdef DEV_BUILD
	dbugLog(_FLD, "m:%u t:%d p:%u c:%s update"
					, missionID()
					, player->teulIndex()
					, player->playerID()
					, player->callsign()
					);
#endif

	// Otherwise it's worth knowing about
	m_removes.erase(player->chatHandle());
	m_updates[player->chatHandle()] = player;
}

//////////////////////////////////////////////////////////////////////
// Find the mission for a player and flag the player for updates

void
chatMissionUpdateMember(Player* const player)
{
	if ( player->missionID() == 0 )
		return;

	ChatMission* const mission = chatFindMission(player->missionID());
	if ( mission == nullptr )
		return;

	mission->addMemberUpdate(player);
}

//////////////////////////////////////////////////////////////////////
// Queue a removal update for this player on the updates list for this
// mission -- because this is a removal, we cannot make any assumption
// about the future validity of the player - so we just use the handle.

void
ChatMission::addMemberRemoval(Player* const player)
{
	betaLog(_FLD, "m:%u t:%d p:%u c:%s removed"
				, missionID()
				, player->teulIndex()
				, player->playerID()
				, player->callsign()
				);

	m_removes.insert(player->chatHandle());
	m_updates.erase(player->chatHandle());
}

//////////////////////////////////////////////////////////////////////
// Expire all of the waypoints

void
ChatMission::expireWaypoints()
{
	const UINT32 cutoff = (UINT32) teulTime() - CONTACT_REPORT_LIFETIME_S;
	for ( auto it = m_waypoints.begin(), end = m_waypoints.end(); it != end; ++it )
	{
		Waypoint& wp = it->second;
		if ( wp.type() == WPT_ENEMY && wp.posted() <= cutoff )
		{
			betaLog(_FLI, "m:%u removing waypoint %s", missionID(), Chat::Name(it->first).c_str());
			removeWaypoint(it->first, false);
			break;
		}
	}
}

//////////////////////////////////////////////////////////////////////
// Easter eggs for certain mission leaders

static bool LEADER_EGGS_ENABLED = true;
typedef std::map<ChatHandle, std::string> EGGS;
static EGGS eggs;

static UINT32 nextEggFetch = 0;
static const UINT32 EGG_FETCH_INTERVAL = 15 * 60;	// 15 minutes

class EggQuery : public DBThreadQuery
{
public:
	EggQuery() : DBThreadQuery(DBThread::Default, "EggQuery", "SELECT callsign, egg FROM mission_eggs", DBThreadQuery::_CALLBACK), m_result() {}

	EGGS m_result;

public:
	virtual bool Process(const size_t numRows) OVERRIDE_FN
	{
		m_result.clear();

		if ( numRows <= 0 )
			return true;

		while ( Query()->FetchRow() )
		{
			const DBRow& row = Query()->Row();
			ChatHandle handle = chatGetLowercaseHandle(row[0]);
			m_result[handle] = row[1];
		}

		threadLog(_FLN, "Processed %u mission eggs :)", m_result.size());

		return true;
	}

public:
	virtual void Success() OVERRIDE_FN
	{
		eggs = m_result;
	}
};

void
chatFetchEggs()
{
	nextEggFetch = teulTime() + EGG_FETCH_INTERVAL;

	// Create a new egg query -- destruction is automatic
	(new EggQuery())->Queue();
}

static inline bool
_egg(Player* const player)
{
	if ( ! LEADER_EGGS_ENABLED )
		return false;

	if ( teulTime() >= nextEggFetch )
	{
		chatFetchEggs();
		return false;
	}

	if ( (rand() & 3) != 0 )
		return false;

	bool egged = false;

	EGGS::iterator egg = eggs.find(player->matchHandle());
	if ( egg != eggs.end() && !egg->second.empty() && egg->second.size() > 2 )
	{
		player->message(LSTR_ORDERS, LSTR_NEWLEADER_EGG, egg->second.c_str());
		egged = true;
	}

	return egged;
}

//////////////////////////////////////////////////////////////////////
// Set the mission leader, send messages to players if neccessary

void
ChatMission::SetLeader(const ChatHandle& newLeaderHandle)
{
	static const UINT32 _NUM_TIPS = 6;
	static const LSTRING_ID tips[_NUM_TIPS] = {
		LSTR_NEWLEADER_1
	,	LSTR_NEWLEADER_2
	,	LSTR_NEWLEADER_3
	,	LSTR_NEWLEADER_4
	,	LSTR_NEWLEADER_5
	,	LSTR_NEWLEADER_6
	};
	const ChatHandle matchHandle = chatGetLowercaseHandle(newLeaderHandle);

	// If the new leader is a previously displaced leader,
	// remove their displacement flags.
	if ( m_displacedLeader == matchHandle )
		m_leaderDisplaced = 0;
	checkDisplacedLeader();

	if ( leader() == newLeaderHandle || m_matchLeader == matchHandle )
		return;

	Chat::Name callsign(newLeaderHandle);

	dbugLog(_FLI, "m:%u new leader %s", missionID(), callsign.c_str());

	if ( newLeaderHandle != 0LL )
	{
		SetOrders(LSTR_NEW_LEADER, callsign.c_str());
		Player* const newLeader = PlayerInterface::Lookup(newLeaderHandle);
		if ( newLeader != nullptr )
		{
			if ( !_egg(newLeader) )
			{
				UINT32 i = rand() % _NUM_TIPS;
				newLeader->message(LSTR_ORDERS, tips[i]);
			}
		}

		// Flush the orders out so that newbies don't see them
		SetOrders(LSTR_NONE);
	}

	lead().current(newLeaderHandle);
	m_matchLeader = matchHandle;
	lockLeader();

	setBackupLeader(0LL);
	m_backupCandidate = 0;
	electBackupLeader();
}

//////////////////////////////////////////////////////////////////////
// Avoid treating rats as high ranking players; if there are officers
// around let the mission go to them

static RANK_NO
_candidateRank(const Player* const player)
{
	static const RANK_NO RAT_TREATMENT_RANK = 3;
	if ( !player->isPlaynet() || player->effectiveRank() <= RAT_TREATMENT_RANK )
		return player->effectiveRank();
	return RAT_TREATMENT_RANK;
}

//////////////////////////////////////////////////////////////////////
// Determine which player should be the new mission leader

static const Player*
_findNewLeader(ChatMission::MEMBERS::const_iterator start, ChatMission::MEMBERS::const_iterator end, bool allowAfk)
{
	const Player* preferred = nullptr;
	const Player* previous = nullptr;
	RANK_NO preferredRank = (previous) ? _candidateRank(previous) : 0;

	while ( start != end )
	{
		const Player* const candidate = start->second;
		++start;

		if ( candidate->teulIndex() < 0 || !candidate->initialized() || candidate->isMissionLeader() )
			continue;
		// Are they accepting promotions?
		if ( candidate->rejectLead() )
			continue;
		if (!allowAfk && candidate->isAFK())
			continue;

		const RANK_NO candidateRank = _candidateRank(candidate);
		if ( preferredRank > candidateRank )
			continue;

		if ( preferredRank == candidateRank && previous )
		{
			if ( previous->isSpawned() && !candidate->isSpawned() )
				continue;
			if ( candidate->timeOnMission() < previous->timeOnMission() )
				continue;
		}

		previous = candidate;
		preferred = candidate;
		preferredRank = candidateRank;
	}

	return preferred;
}

//////////////////////////////////////////////////////////////////////
// Check the status of the displaced leader fields

void
ChatMission::checkDisplacedLeader() const
{
	if ( m_displacedLeader != 0LL && m_leaderDisplaced < teulTime() - (8 * 60) )
	{
		m_displacedLeader = 0;
		m_leaderDisplaced = 0;
	}
}

//////////////////////////////////////////////////////////////////////
// Assign a new player as leader

bool
ChatMission::electBackupLeader()
{
	const Player* candidate = nullptr;
	if ( !m_members.empty() && m_members.size() > 1 )
	{
		if ( backupLeader() != 0LL )
		{
			candidate = PlayerInterface::Lookup(backupLeader());
			// Check to see if their AFK/autoLeader status has changed
			if ( candidate->rejectLead() )
			{
				lead().backup(0);
				candidate = nullptr;
			}
		}

		// Try and find a non-afk leader
		if ( candidate == nullptr )
			candidate = _findNewLeader(m_members.begin(), m_members.end(), false);

		// Try and find* any* leader
		if ( candidate == nullptr )
			candidate = _findNewLeader(m_members.begin(), m_members.end(), true);
	}

	if ( candidate == nullptr || candidate->missionID() != missionID() || candidate->isMissionLeader() )
	{
		lead().backup(0ULL);
		m_backupCandidate = 0;
		dbugLog(_FLD, "m:%u unable to find a replacement leader", missionID());
		return false;
	}

	if ( candidate->playerID() != m_backupCandidate )
	{
		dbugLog(_FLI, "m:%u p:%u c:%s is now backup leader candidate", missionID(), candidate->playerID(), candidate->callsign());
		m_backupCandidate = candidate->playerID();
		chatSendBackupLeaderChange(missionID(), m_backupCandidate);
	}

	return true;
}

//////////////////////////////////////////////////////////////////////
// Add a greeting for a given player to an MMU

static inline void
_addMemberGreeting(MISSION_MEMBER_UPDATES& mmu, const Player* const player)
{
	MissionMember update;
	update.m_handle = player->chatHandle();
	update.m_rank = player->effectiveRank();
	update.m_weapon = player->weapon();
	update.m_x = (SINT32)player->currentX();
	update.m_y = (SINT32)player->currentY();
	mmu.add(update);
}

//////////////////////////////////////////////////////////////////////
// Populate an MMU from a mission members list; helper for
// send first update to stop it from getting ugly

static void
_populateGreeting(MISSION_MEMBER_UPDATES& mmu, ChatMission::MEMBERS& members)
{
	for ( ChatMission::MEMBERS::iterator it = members.begin()
		; it != members.end()
		; ++it
		)
	{
		_addMemberGreeting(mmu, it->second);
	}
}

//////////////////////////////////////////////////////////////////////
// Send a player their first update

void
ChatMission::sendFirstUpdate(const Player* const player)
{
	std::pair<Chat::MissionStatus*, TeulTimeMS> data(this, 0ULL);

	// Send the player the mission status.
	teulSend(player->teulIndex(), Callback(), &data);

	// If the list is empty, populate it from scratch
	// otherwise add me to the list first
	if ( m_mmu.empty() )
		_populateGreeting(m_mmu, m_members);
	else
		_addMemberGreeting(m_mmu, player);

	m_mmu.send(player->teulIndex());
}

//////////////////////////////////////////////////////////////////////
// Send all the current Chat::MissionStatus changes to all members.

void
ChatMission::broadcastStatusChanges()
{
	// Has the mission status information changed?
	if ( lastModified() > m_lastUpdateSent )
	{
		std::pair<Chat::MissionStatus*, TeulTimeMS> data(this, m_lastUpdateSent);
		chatBroadcastObject(m_members.begin(), m_members.end(), Callback(), data);
		m_lastUpdateSent = teulClock64;
	}
}

//////////////////////////////////////////////////////////////////////
// Send the pending updates for this mission, returns the number sent

UINT32
ChatMission::sendUpdates()
{
	if ( !m_waypoints.empty() )
		expireWaypoints();

	// If there's nobody on the list, nothing to do.
	if ( m_members.empty() )
		return 0;

	broadcastStatusChanges();

	if ( m_removes.empty() && m_updates.empty() )
		return 0;

#ifdef CLOSED_BETA
	dbugLog(_FLD, "m:%u members:%" PRSizeT " updates:%" PRSizeT " "
					, missionID()
					, m_members.size()
					, m_updates.size()
					);
#endif

	// The greeting list is no-longer valid.
	m_mmu.clear();

	// Insert the deletions (rank = 0)
	if ( !m_removes.empty() )
	{
		MissionMember update;
		update.m_rank = 0;
		for ( ChatMission::MEMBER_REMOVES::iterator it = m_removes.begin()
			; it != m_removes.end()
			; ++it
			)
		{
			update.m_handle = *it;
#ifdef CLOSED_BETA
			if ( g_logDebug )
				dbugLog(_FLDB, "-%s ", Chat::Name(update.m_handle).c_str());
#endif
			m_mmu.add(update);
		}
		m_removes.clear();
	}

	// Insert the updates
	for ( ChatMission::MEMBER_UPDATES::iterator it = m_updates.begin()
		; it != m_updates.end()
		; ++it
		)
	{
#ifdef CLOSED_BETA
		if ( g_logDebug )
			dbugLog(_FLDB, "*%s ", Chat::Name(it->first).c_str());
#endif

		Player* const player = it->second;
		if ( player )
		{
			_addMemberGreeting(m_mmu, player);
		}
	}
	m_updates.clear();
	if ( !m_updates.empty() )
		LogAbort("failed to clear update list");

	UINT32 updatesSent = 0;

	if ( m_mmu.size() > 0 )
	{
		ChatMission::MEMBERS::iterator it = m_members.begin();
		while ( it != m_members.end() )
		{
			Player* const player = it->second;
			++it;

			if ( player == nullptr || player->teulIndex() < 0 )
				continue;

#ifdef CLOSED_BETA
			if ( g_logDebug )
				dbugLog(_FLDB, ">%d:%u:%s "
								, player->teulIndex()
								, player->playerID()
								, player->callsign()
								);
#endif

			if ( m_mmu.send(player->teulIndex()) >= 0 )
				++updatesSent;
			else
				dbugLog(_FLI, "t:%d XXX send failed", player->teulIndex());
		}
	}

	// Clear the mmu so that it can be re-used as a greeting list
	m_mmu.clear();

	return updatesSent;
}

//////////////////////////////////////////////////////////////////////
// Send the next batch of updates

void
chatSendMissionUpdates()
{
	static const UINT32 MISSION_UPDATE_LIMIT = 60;
	static const UINT32 MISSION_UPDATE_RATE_MS = 5000;	// 5 seconds

	// Keep track of the ID of the last mission we worked with
	static UINT32 lastMission = 0;
	static TeulTimeMS startUpdates = 0;

	if ( teulClock64 < startUpdates )
		return;

	// find the next mission
	UINT32 updatesSent = 0;
	CHAT_MISSIONS::iterator it = g_missions.upper_bound(lastMission);

	while ( it != g_missions.end() && updatesSent < MISSION_UPDATE_LIMIT )
	{
		ChatMission& mission = *(it->second);
		++it;

		const UINT32 updatesSentThisMission = mission.sendUpdates();
		if ( updatesSentThisMission && g_logDebug )
		{
			betaLog(_FLD, "m:%u sent %u updates", mission.missionID(), updatesSentThisMission);
		}
		updatesSent += updatesSentThisMission;
		lastMission = mission.missionID();
	}

	if ( it == g_missions.end() )
	{
		lastMission = 0;
		startUpdates = startUpdates + MISSION_UPDATE_RATE_MS;
		if ( teulClock64 > startUpdates )
			startUpdates = teulClock64 + MISSION_UPDATE_RATE_MS;
		return;
	}

	return;
}

void
ChatMission::addMSPWaypoint(const ChatHandle& ownerCall, FLOAT64 x, FLOAT64 y, FLOAT64 z, UINT32 stoId)
{
	message(LSTR_MISSION, LSTR_MOBILE_SPAWN_AVAILABLE, Chat::Name(ownerCall).c_str());
	Waypoint newWP(false, _getMSPHandle(), ownerCall
					, WPT_MOBILESPAWN, WPT_MS_LOCATION
					, static_cast<SINT32>(x), static_cast<SINT32>(y)
					, teulTime()
					, "Current mobile spawn for this mission");
	addWaypoint(newWP);

	fru().active(true);
	fru().creator(ownerCall);
	fru().position(WorldPosition_m(x, y, z));

	m_stoUID = stoId;

	setFRUCooldown(host_config.sto.fruPlaceCooldown_s);

	flagForStatusUpdate();
}

void
ChatMission::removeMSPWaypoint(UINT32, bool byPlayer)
{
	if ( removeWaypoint(_getMSPHandle(), byPlayer) )
		message(LSTR_MISSION, LSTR_MOBILE_SPAWN_UNAVAILABLE);

	removeSTO();

	fru().active(false);
	fru().creator(0ULL);

	flagForStatusUpdate();
}

//////////////////////////////////////////////////////////////////////
// Send pending statuses.

static void
_sendPendingStatuses(void*)
{
	for ( auto it = g_statuses.begin(), end = g_statuses.end(); it != end; ++it )
	{
		ChatMission* const mission = chatFindMission(*it);
		if ( mission )
		{
			dbugLog(_FLD, "mission:%u sending status change", mission->missionID());
			mission->broadcastStatusChanges();
		}
	}

	g_statuses.clear();
}

//////////////////////////////////////////////////////////////////////
// Add a mission to the statuses list.

void
ChatMission::flagForStatusUpdate()
{
	const bool firstMission = g_statuses.empty();
	g_statuses.insert(missionID());
	if ( firstMission )
		threadCallback(_sendPendingStatuses, nullptr);
}

//////////////////////////////////////////////////////////////////////
// Receive a mobile spawn waypoint instruction from the mmhost

template<>
SINT32
MOBILE_SPAWN_WAYPOINT::receive(SINT32 /*teulIndex*/, MOBILE_SPAWN_WAYPOINT& incoming)
{
	const MobileSpawnWaypoint& wp = incoming.data();

	betaLog(_FLI, "mspwp: m:%u p:%u c:%s d:%u sto:%u", wp.m_missionID, wp.m_playerID, Chat::Name(wp.m_ownerCall).c_str(), wp.m_deployed, wp.m_stoUID);

	if ( wp.m_missionID == 0 )
	{
		dbugLog(_FLA, "received waypoint for missionID 0");
		return -EINVAL;
	}

	ChatMission* const mission = chatFindMission(wp.m_missionID);
	if ( mission == nullptr )
	{
		dbugLog(_FLA, "m:%u no such mission to delete waypoint for", wp.m_missionID);
		return -EINVAL;
	}

	if ( wp.m_deployed )
	{
		if ( mission->m_stoUID && mission->m_stoUID != wp.m_stoUID )
			mission->removeSTO();

		mission->addMSPWaypoint(wp.m_ownerCall, wp.m_x, wp.m_y, wp.m_z, wp.m_stoUID);
	}
	else
	{
		mission->removeMSPWaypoint(wp.m_stoUID, false);
		if ( wp.m_stoUID )
			chatRemoveServerTrackedObject(wp.m_stoUID);
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Control whether or not I will accept mission leader status
//////////////////////////////////////////////////////////////////////

class DotAutoLeader : public DotCommand
{
public:
	DotAutoLeader() : DotCommand("autoleader", "Controls whether or not you will take leadership of missions"
							, NO_MASK, LSTR_LEADER_USAGE, LSTR_LEADER_DETAIL) {}

	void inline _report(Player& player) const
	{
		player.message(LSTR_NOTE, (!player.getPreference(CPR_AUTO_LEADER)) ? LSTR_LEADER_STATE_OFF : LSTR_LEADER_STATE_ON);
	}

	virtual void action(Player& player, Chat::ClientMessage* /*message*/) const OVERRIDE_FN
	{
		player.SetPreference(CPR_AUTO_LEADER, !player.getPreference(CPR_AUTO_LEADER));
		_report(player);
	}

	virtual void actionWith(Player& player, Chat::ClientMessage* /*message*/, char* arguments) const OVERRIDE_FN
	{
		if ( strncasecmp("on", arguments, 2) == 0 )
			player.SetPreference(CPR_AUTO_LEADER, TRUE);
		else if ( strncasecmp("of", arguments, 2) == 0 )
			player.SetPreference(CPR_AUTO_LEADER, FALSE);
		else
		{
			help(player);
			return;
		}

		_report(player);
	}
} dotAutoLeader;

//////////////////////////////////////////////////////////////////////
// Forcibly take control of a mission
//////////////////////////////////////////////////////////////////////

static void
_checkNewLeader(const ChatMission* mission, const Player& player, const Player* oldLeader)
{
	// Wipe the displacement memory if it has expired
	mission->checkDisplacedLeader();
	// If displacement data matches, you're a recently displaced leader.
	if ( player.matchHandle() == mission->m_displacedLeader )
		return;

	TeulDeltaMS lockFor = (TeulDeltaMS)mission->m_leaderLockFor;

	if ( oldLeader->goneAFK() )
		lockFor -= (TeulDeltaMS) oldLeader->goneAFK() + 60;

	if ( player.effectiveRank() > oldLeader->effectiveRank() )
		lockFor -= (TeulDeltaMS) (player.effectiveRank() - oldLeader->effectiveRank());

	if ( player.squadID() && player.squadID() == oldLeader->squadID() && lockFor > 30 )
		lockFor = 30;

	if ( lockFor < 0 )
		return;

	UINT32 lockUntil = mission->m_leaderLockFrom + (UINT32) lockFor;
	if ( lockUntil <= teulTime() )
		return;
	lockUntil -= teulTime();

	throw Chat::LStringErr(LSTR_CANNOT_TAKELEAD, oldLeader->callsign(), chatTimeInterval(lockUntil));
}

class DotTakeLead : public DotCommand
{
public:
	DotTakeLead() : DotCommand("takelead", "Take over the mission"
								, NO_MASK, LSTR_TAKELEAD_USAGE, LSTR_TAKELEAD_DETAIL
								) {}
	DotTakeLead(const std::string& newCommand, const std::string& withDescription
				, const PermissionMask& mask, LSTRING_ID usage, LSTRING_ID detail, bool playnetOnly=false)
				: DotCommand(newCommand, withDescription, mask, usage, detail, playnetOnly) {}

	virtual void action(Player& player, Chat::ClientMessage* /*message*/) const OVERRIDE_FN
	{
		if ( !player.missionID() )
			throw LSTR_NOT_ON_MISSION;
		ChatMission* mission = chatFindMission(player.missionID());
		if ( !mission )
			throw Chat::LStringErr(LSTR_INTERNAL_ERROR);
		if ( player.isMissionLeader() )
			throw Chat::LStringErr(LSTR_ALREADY_LEADER);
		if ( player.isAFK() )
			throw Chat::LStringErr(LSTR_NO_AFK_TAKELEAD);

		Player* leader = PlayerInterface::Lookup(mission->m_matchLeader);

		// Additional rules for non-GMs/backup leaders when there is
		// an existing leader
		if ( leader != nullptr )
		{
			if ( player.matchHandle() != mission->backupLeader() )
				_checkNewLeader(mission, player, leader);
			if ( mission->m_matchLeader != 0 && mission->m_displacedLeader == 0 )
			{
				mission->m_displacedLeader = leader->matchHandle();
				mission->m_leaderDisplaced = teulTime();
			}
		}
		else
		{
			mission->m_displacedLeader = 0LL;
			mission->m_leaderDisplaced = 0;
		}

		// Temporary lock
		mission->m_leaderLockFrom = teulTime();
		mission->m_leaderLockFor = 90;

		chatSendLeaderChange(player.missionID(), player.playerID(), player.chatHandle(), player.matchHandle());
		if ( leader && mission->m_displacedLeader == leader->matchHandle() )
		{
			leader->message(LSTR_NOTE, LSTR_USURPED, player.callsign());
			player.message(LSTR_NOTE, LSTR_USURPING, leader->callsign());
		}
	}
} dotTakeLead;

class DotGMTakeLead : public DotTakeLead
{
public:
	DotGMTakeLead() : DotTakeLead("gmtakelead", "Forcibly take over the mission"
								, NO_MASK, LSTR_TAKELEAD_USAGE, LSTR_TAKELEAD_DETAIL
								) {}
	virtual void action(Player& player, Chat::ClientMessage* message) const OVERRIDE_FN
	{
		if ( player.isEffectiveGM() == false )
			return DotTakeLead::action(player, message);

		if ( !player.missionID() )
			throw LSTR_NOT_ON_MISSION;
		ChatMission* mission = chatFindMission(player.missionID());
		if ( !mission )
			throw Chat::LStringErr(LSTR_INTERNAL_ERROR);
		if ( player.isMissionLeader() )
			throw Chat::LStringErr(LSTR_ALREADY_LEADER);
		if ( player.isAFK() )
			throw Chat::LStringErr(LSTR_NO_AFK_TAKELEAD);

		player.gmNote("GM", command(), (mission->m_matchLeader) ? Chat::Name(mission->m_matchLeader).c_str() : "[no leader]", nullptr);

		mission->m_displacedLeader = 0LL;
		mission->m_leaderDisplaced = 0;

		// Temporary lock
		mission->m_leaderLockFrom = teulTime();
		mission->m_leaderLockFor = 60 * 60;
		chatSendLeaderChange(player.missionID(), player.playerID(), player.chatHandle(), player.matchHandle());
	}
} dotGMTakeLead;

//////////////////////////////////////////////////////////////////////
// List the players on your mission
//////////////////////////////////////////////////////////////////////

class DotMList : public DotCommand
{
public:
	DotMList() : DotCommand("mlist", "List mission membership stats"
							, NO_MASK, LSTR_MLIST_USAGE, LSTR_MLIST_DETAIL
							) {}

	bool help(Player& player) const OVERRIDE_FN
	{
		return genericHelp(player, "/mlist", "Lists current mission members and their status");
	}

	void action(Player& player, Chat::ClientMessage* message) const OVERRIDE_FN;
} dotMList;

void
DotMList::action(Player& player, Chat::ClientMessage* const /*message*/) const
{
	if ( !player.missionID() )
	{
		player.message(LSTR_CAUTION, LSTR_NOT_ON_MISSION);
		return;
	}

	ChatMission* mission = chatFindMission(player.missionID());
	if ( !mission )
	{
		player.message(LSTR_CAUTION, LSTR_INTERNAL_ERROR);
		return;
	}

	if ( player.isPlaynet() )
	{
		const char* const p = chatMakeTempString("m:%u leader:%s memb:%u wps:%u mu:%u mr:%u"
											, mission->missionID()
											, Chat::Name(mission->leader()).c_str()
											, mission->m_members.size()
											, mission->m_waypoints.size()
											, mission->m_updates.size()
											, mission->m_removes.size()
											);

		player.message(LSTR_NOTE, LSTR_GENERIC, p);
	}

	std::string list = "";

	ChatMission::MEMBERS::iterator it;
	for ( it = mission->m_members.begin()
		; it != mission->m_members.end()
			&& list.length() < 390
		; ++it
		)
	{
		Player* subj = it->second;
		if ( subj->isHiddenFrom(player) )
			continue;
		// Don't allow them to see GMs with buzzards
		if ( !player.isGM() && subj->isBuzzard() )
			continue;
		subj->addName(list);
	}

	player.message(LSTR_NOTE, LSTR_GENERIC, list.c_str());
}

//////////////////////////////////////////////////////////////////////
// Set the nextPlaceAvail value of the FRU field.

void
ChatMission::setFRUCooldown(const UINT32 cooldown_s)
{
	// Don't do anything if we're resetting the cool down.
	if ( cooldown_s == 0 && fru().nextPlaceAvail() < teulClock64 )
		return;

	const TeulTimeMS cooldown_ms = cooldown_s * 1000;
	fru().nextPlaceAvail(teulClock64 + cooldown_ms);

	dbugLog(_FLD, "m:%u fru cooldown set for %u seconds (%llums)", missionID(), cooldown_s, cooldown_ms);

	flagForStatusUpdate();
}

