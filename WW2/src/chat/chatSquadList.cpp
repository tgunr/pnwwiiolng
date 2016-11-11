#include <syscfg.h> // Path is project specific.
// Date: Mon Sep 12 15:51:40 CDT 2005
//
// Management of squad lists
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include <mutex>

#include "wwii/wwiiHash.h"
#include "wwii/wwiiSquads.h"
#include "chat/chatSquadListTypes.h"
#include "chat/chatSquadList.h"
#include "teul/teulPackers.h"
#include "teul/teulTime.h"
#include "teul/teulEndpointDefs.h"
#include "chat/chatdbSquadXchg.h"
#include "chat/chatdbClientMessages.h"
#include "chat/chatPlayer.h"
#include "chat/chatWhoList.h"
#include "chat/chatData.h"
#include "wwii/wwiiSquadMsg.h"
#include "chat/chatSquadInvite.h"
#include "chat/chatBroadcastObject.h"
#include "chat/chatSquadVote.h"
#include "chat/chatList.h"
#include "chat/chatTimeInterval.h"
#include "wwii/wwiiConfig.h"
#include "wwii/wwiiDBThreads.h"
#include "wwii/wwiiThreadText.h"
#include "wwii/wwiiThreadTypes.h"
#include "include/mydbQuery.h"
#include "chat/chatPreferenceEnum.h"
#include "chat/chatMissionTypes.h"
#include "chat/chatMissions.h"
#include "chat/chatRecruiterList.h"
#include "chat/chatDebugConditions.h"
#include "chat/chatName.h"

static const UINT32 SQUAD_UPDATE_INTERVAL_MS = 5 * 1000;

using SQUAD_LISTS = std::unordered_map<UINT32,SquadList>;
using SQUAD_NAMES = std::unordered_map<ChatHandle,ChatHandle>;

using RECRUITERS = std::set<const Player*>;
static RECRUITERS g_recruiters;

static SQUAD_LISTS g_squads;

static SQUAD_NAMES g_squadNames;
static std::mutex _namesMutex;

extern void chatSendCOAppointment(UINT32 squadID, const ChatHandle handle);

//////////////////////////////////////////////////////////////////////
// Fetch the list of members of a squad

static const char* QRY_SQUAD_MEMBERS =
	"SELECT	p.callsign, p.playerid, p.squad_rank, UNIX_TIMESTAMP(p.squad_joined)"
		" FROM wwii_player p"
		" WHERE p.squadid = %u"
	;

class SquadMemberQuery : public DBThreadQuery
{
public:
	SquadMemberQuery(UINT32 squadID)
		: DBThreadQuery(DBThread::Default, "SquadMemberQuery", DBThreadQuery::_CALLBACK, QRY_SQUAD_MEMBERS, squadID)
		, m_squadID(squadID)
		, m_results()
		{
			m_results.clear();
		}

public:
	UINT32 m_squadID;

	struct Result
	{
		ChatHandle	m_handle;
		UINT32		m_playerID;
		UINT32		m_rank;
		UINT32		m_joined;
	};

	std::vector<Result> m_results;

	virtual bool Process(const size_t numRows) override
	{
		if ( numRows <= 0 )
			return true;

		dbugLog(_FLI, "squad list process: sq:%u rows:%" PRSizeT "", m_squadID, numRows);

		Result result;
		while ( Query()->FetchRow() )
		{
			DBRow& row = Query()->Row();

			result.m_handle = chatGetHandle(row.NextString());
			result.m_playerID = row.NextUInt();
			result.m_rank = row.NextUInt();
			result.m_joined = row.NextUInt();
			m_results.push_back(result);
		}

		return true;
	}

	virtual void Success() override
	{
		SquadList* list = chatFindSquadList(m_squadID);
		if ( list == nullptr )
		{
			dbugLog(_FLI, "sq:%u list went away while querying members", m_squadID);
			Failure();
			return;
		}

		if ( g_debugSquads )
			dbugLog(_FLD, "sq:%u '%s' got list of %" PRSizeT " members was %" PRSizeT ""
						, m_squadID, list->squad().shortName(), m_results.size(), list->members().size());

		// In order that we can tell who is no-longer in the squad according
		// to the database, we erase all member's joined dates. Then as members
		// are read back from the database, we set the joined dates to the
		// values we read from the database.
		SQUAD_MEMBERS::iterator mit;
		for ( mit = list->begin(); mit != list->end(); ++mit )
		{
			mit->second.m_joined = 0;
		}

		// See who was listed.
		Player* player;
		for ( size_t i = 0; i < m_results.size(); ++i )
		{
			const Result& result = m_results[i];
			ChatHandle handle = result.m_handle;
			const UINT32 playerID = result.m_playerID;
			const UINT32 rank = result.m_rank;
			UINT32 joined = result.m_joined;

			if ( rank <= 0 )
			{
				// A rank of zero means they've been removed externally.
				mit = list->m_members.find(handle);
				if ( mit != list->m_members.end() )
					list->memberLeave(true, &mit->second, LSTR_SQUAD_LEAVER, "removed");
				else
					list->removeMember(handle);
			}

			if ( joined == 0 )
			{
				dbugLog(_FLA, "sq:%u p:%u c:%s has squad id but null squad_joined"
								, m_squadID, playerID, Chat::Name(handle).c_str());
				joined = teulTime();
				DBThread::Do(DBThread::Default
								, "UPDATE wwii_player"
								   " SET squad_joined=NOW()"
					 			" WHERE playerID=%u AND squadid=%u AND squad_joined is NULL"
									, playerID, m_squadID
					);
			}

			mit = list->m_members.find(handle);
			// Check for an existing entry with no change
			if ( mit != list->end() && mit->second.squadRank() == rank )
			{
				mit->second.m_joined = joined;
				continue;
			}

			player = PlayerInterface::Lookup(handle);
			const bool announce = (list->squad().callsign() != host_config.squad.defaultSquad);
			list->memberJoin(player, handle, rank, joined, nullptr, announce);
		}

		// Anyone who still has a zero joined data must no-longer be
		// in the squad according to the database, so eliminate them.
		for ( mit = list->begin(); mit != list->end();  )
		{
			std::pair<const ChatHandle, SquadMemberEntry>& entry = *(mit++);
			if ( entry.second.joined() == 0 )
			{
				dbugLog(_FLD, "sq:%u player %s no-longer in member list", m_squadID, Chat::Name(entry.first).c_str());
				list->memberLeave(true, &entry.second, LSTR_SQUAD_LEAVER, "missing");
			}
		}

		list->announce();
	}

	virtual void Failure() override
	{
		dbugLog(_FLW, "sq:%u error reading squad list", m_squadID);
	}
};

void
chatRequestSquadMemberList(const UINT32 squadID)
{
	(new SquadMemberQuery(squadID))->Queue();
}

//////////////////////////////////////////////////////////////////////
// Prevent the same squad being requested multiple times within a
// short period of time

static bool
_requestPending(UINT32 squadID)
{
	static const UINT32 PENDING_DURATION_S = 15;
	using GUARD = std::vector<std::pair<UINT32, UINT32>>;
	static GUARD guard;
	GUARD::iterator it;

	// Remove all the expired entries at the front of the list
	for ( it = guard.begin(); it != guard.end(); ++it )
	{
		if ( it->first >= teulTime() )
			break;
	}
	if ( it != guard.begin() )
		guard.erase(guard.begin(), it);

	for ( it = guard.begin(); it != guard.end(); ++it )
	{
		if ( it->second == squadID )
			return true;
	}

	guard.push_back(std::make_pair(teulTime() + PENDING_DURATION_S, squadID));

	return false;
}

//////////////////////////////////////////////////////////////////////
// Update the squad data for a player

void
Player::updateSquadEntry(bool squadAnnounce)
{
	if ( !squadID() )
		return;

	chatRequestSquad(squadID(), this);

	if ( squadAnnounce )
		announceToSquadChannel();
}

//////////////////////////////////////////////////////////////////////
// Constructor for a squad list with a supplied squad object

SquadList::SquadList(const Squad& withSquad)
	: m_msstamp(teulClock64), m_lastMemberMod(0ULL)
	, m_squad(withSquad), m_pulled(0), m_members(), m_online(0)
	, m_newSquad(_requestPending(withSquad.squadID()))
{
}

//////////////////////////////////////////////////////////////////////
// SquadList internal function: add a member and update if necessary

void
SquadList::_addMember(const ChatHandle handle, RANK_NO squadRank, Player* player, UINT32 joined)
{
	// You're not online until you've initialized
	Player* usePlayer = (player && player->initialized() && player->teulIndex() >= 0) ? player : nullptr;

#ifdef HEAVY_DEBUG
	if ( g_debugSquads )
		dbugLog(_FLD, "sq:%u +%s [%s]", squad().squadID(), Chat::Name(handle).c_str(), (usePlayer) ? "online" : "offline");
#endif

	if ( joined == 0 )
		joined = teulTime();

	SquadMemberEntry entry(handle, usePlayer, squadRank, joined);
	m_members.insert(std::make_pair(handle, entry));
	if ( usePlayer )
	{
		_updateOnline(+1);
	}
	m_lastMemberMod = teulClock64;
}

//////////////////////////////////////////////////////////////////////
// SquadList internal function: update an existing member entry

void
SquadList::changeMemberEntry(SquadMemberEntry& member, RANK_NO squadRank, Player* player)
{
	// You're not online until you've initialized
	Player* usePlayer = (player && player->initialized() && player->teulIndex() >= 0) ? player : nullptr;

	if ( member.m_squadRank != squadRank )
		member.m_squadRank = squadRank;

	bool needSquadInfo = false;
	// See if they are going on or offline
	if ( member.m_player != usePlayer )
	{
		if ( usePlayer )			// Coming online
		{
			_updateOnline(+1);
			needSquadInfo = true;
		}
		else
			_updateOnline(-1);

		member.m_player = usePlayer;
	}
	else if ( usePlayer && player->lastSquadInfoMS() == 0 )
		needSquadInfo = true;

	if ( usePlayer )
	{
		chatUpdateRecruiterList(usePlayer, usePlayer->getPreference(CPR_IS_RECRUITING), this);
	}

	if ( needSquadInfo )
		usePlayer->sendSquadInfo();

	m_lastMemberMod = member.m_msstamp = teulClock64;
}

//////////////////////////////////////////////////////////////////////
// Update the members-online count of a squad and bring the timestamp
// upto date.

void
SquadList::_updateOnline(int by)
{
	m_online += by;
	setUpdated();
}

//////////////////////////////////////////////////////////////////////
// Complete the construction of a new squad

void
SquadList::created()
{
	if ( !m_newSquad )
		return;

	dbugLog(_FLI, "sq:%u %s creation completed", squad().squadID(), squad().shortName());
	m_newSquad = false;

	SQUAD_MEMBERS::iterator it;
	Player* player;
	for ( it = begin(); it != end (); ++it )
	{
		player = it->second.m_player;
		if ( player && player->teulIndex() >= 0 )
			player->announceToSquadChannel();
	}
}

//////////////////////////////////////////////////////////////////////
// Add or modify an entry for a squad member; online status is
// determined based on whether Player is NULL or not.

void
SquadList::updateMember(const ChatHandle handle, RANK_NO squadRank, Player* player, UINT32 joined)
{
	SQUAD_MEMBERS::iterator it = m_members.find(handle);

	if ( it != m_members.end() )
		changeMemberEntry(it->second, squadRank, player);
	else
		_addMember(handle, squadRank, player, joined);

	if ( player )
	{
		player->SetSquad(squad().squadID(), squadRank, squad().shortHandle(), joined);
	}
}

//////////////////////////////////////////////////////////////////////
// Toggle the recruiting status of a member

void
SquadList::updateRecruiter(const ChatHandle handle, bool isRecruiting)
{
	SquadMemberEntry* member = find(handle);
	if ( member == nullptr || member->recruiting() == isRecruiting )
		return;

	member->m_recruiting = isRecruiting;

	m_lastMemberMod = member->m_msstamp = teulClock64;
}

//////////////////////////////////////////////////////////////////////
// Remove a member from the SquadList, indicating they are no-longer
// in the squad. Note: this does not actually remove them from the squad

void
SquadList::removeMember(ChatHandle handle)
{
	SQUAD_MEMBERS::iterator it = m_members.find(handle);
	if ( it == m_members.end() )
	{
		it = m_members.find(chatGetLowercaseHandle(handle));
		if ( it == m_members.end() )
			return;
	}

	// Remember their player, if there is one
	Player* subject = it->second.m_player;

	// Remove the member entry
	m_members.erase(it);

	if ( subject )
	{
		betaLog(_FLI, "t:%d p:%u c:%s sq:%u removing player from %s"
						, subject->teulIndex(), subject->playerID(), subject->callsign(), squad().squadID(), squad().shortName());
		_updateOnline(-1);
		subject->SetSquad(0, Squad::NOT_IN_SQUAD, 0LL);
		subject->sendSquadInfo(nullptr);
		subject->updateRadios(true, false, false, false, false, false);

		chatUpdateRecruiterList(subject, false, this);
	}

	CHAT_WHO_LIST cwl(CWT_SQUAD);
	Chat::Who who;
	who.setBasics(handle, (subject != nullptr), false, Squad::NOT_IN_SQUAD);
	cwl.add(who);

	for ( it = begin(); it != end(); ++it )
	{
		subject = it->second.m_player;
		if ( subject != nullptr && subject->teulIndex() >= 0 && subject->initialized() )
			cwl.send(subject->teulIndex());
	}

	// Did this player take a vote with them?
	SquadVote* vote = chatFindSquadVote(squad().squadID());
	if ( vote != nullptr )
		vote->withdraw(squad().squadID(), handle, this);
}

//////////////////////////////////////////////////////////////////////
// Send a request for squad information ready for a player to log in

void
chatRequestSquad(UINT32 squadID, Player* player)
{
	SQUAD_LISTS::iterator it = g_squads.find(squadID);
	if ( it == g_squads.end() )
	{
		if ( g_debugSquads )
			dbugLog(_FLI, "sq:%u requesting info", squadID);
		chatdbRequestSquad(squadID);
		return;
	}

	if ( player == nullptr )
		return;

	// Squad is already there, so lets get the info
	SquadList& list = it->second;
	SquadMemberEntry* entry = list.find(player->chatHandle());
	if ( entry == nullptr ) entry = list.find(player->matchHandle());
	if ( entry == nullptr )
	{
		dbugLog(_FLA, "t:%d p:%u c:%s sq:%u unable to find this player in their squad", player->teulIndex(), player->playerID(), player->callsign(), squadID);
		// Squad may not have finished loading yet.
		return;
	}

	if ( g_debugSquads )
	{
		betaLog(_FLI, "t:%d p:%u c:%s sq:%u changing member entry at t:%u", player->teulIndex(), player->playerID(), player->callsign(), squadID, teulTime());
	}

	list.changeMemberEntry(*entry, entry->squadRank(), player);
	player->SetSquad(squadID, entry->squadRank(), list.squad().shortHandle());

	return;
}

//////////////////////////////////////////////////////////////////////
// Say goodbye to a squad

void
chatRemoveSquadList(UINT32 squadID)
{
	if ( g_debugSquads )
		dbugLog(_FLN, "sq:%u erasing", squadID);
	g_squads.erase(squadID);
}

//////////////////////////////////////////////////////////////////////
// Incoming squad information from the dbd

template<>
SINT32
CHATDB_SQUAD_XCHG::receive(SINT32 /*teulIndex*/, CHATDB_SQUAD_XCHG& incoming)
{
	Squad& squad = incoming.data();

	if ( squad.squadID() <= 0 )
	{
		dbugLog(_FLW, "invalid squad: %u", squad.squadID());
		return -EINVAL;
	}

	if ( g_debugSquads )
	{
		Chat::Name co(squad.coHandle()), creator(squad.creatorHandle());
		dbugLog(_FLD, "sq:%u received info short:%s name:%s co:%s creator:%s", squad.squadID(), squad.callsign().c_str(), squad.name().c_str(), co.c_str(), creator.c_str());
	}

	// The query might have been delayed and we might already have
	// the info on this squad
	SquadList* list = chatFindSquadList(squad.squadID());
	if ( list != nullptr )
	{
		if ( g_debugSquads )
			dbugLog(_FLI, "sq:%u received update", squad.squadID());
		list->setSquad(squad);
		return 0;
	}

	// No existing entry so lets grease this pig
	g_squads[squad.squadID()] = SquadList(squad);

	chatRequestSquadMemberList(squad.squadID());

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Lookup a squad, returns NULL if the squad isn't known

SquadList*
chatFindSquadList(UINT32 squadID)
{
	if ( squadID > 0 )
	{
		SQUAD_LISTS::iterator it = g_squads.find(squadID);
		if ( it != g_squads.end() ) return &(it->second);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////
// Find a squad list by name

SquadList*
chatFindSquadListByName(const char* squadName)
{
	ChatHandle wantHandle = chatGetLowercaseHandle(squadName);
	SQUAD_LISTS::iterator it;
	for ( it = g_squads.begin(); it != g_squads.end(); ++it )
	{
		UINT64 squadHandle = it->second.squad().shortHandle();
		if ( wantHandle == squadHandle || wantHandle == chatGetLowercaseHandle(squadHandle) )
			return &(it->second);
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////
// Find the MOTD for a squad

const char*
chatFindSquadMotd(UINT32 squadID)
{
	const SquadList* const list = chatFindSquadList(squadID);
	if ( list == nullptr || list->squad().motd().empty() || list->squad().motd().length() <= 1 )
		return nullptr;
	const char* p = list->squad().motd().c_str();
	while ( *p && *p == ' ' )
		++p;

	return (*p) ? p : nullptr;
}

//////////////////////////////////////////////////////////////////////
// For the times when all you want is the squad's handle

ChatHandle
chatGetSquadHandle(UINT32 squadID)
{
	SQUAD_LISTS::iterator it = g_squads.find(squadID);
	if ( it == g_squads.end() )
		return 0LL;
	return it->second.squad().shortHandle();
}

//////////////////////////////////////////////////////////////////////
// And for the times when all you want is the string

const char*
chatGetSquadCallsign(UINT32 squadID)
{
	SQUAD_LISTS::iterator it = g_squads.find(squadID);
	if ( it == g_squads.end() )
		return nullptr;
	return it->second.squad().callsign().c_str();
}

//////////////////////////////////////////////////////////////////////
// Get the name for a squad rank

const char*
chatGetSquadRankName(RANK_NO squadRank)
{
	static const char* squadRanks[Squad::MAX_RANK+1] =
	{
		"NON-MEMBER"
	,	"Recruit"
	,	"Member"
	,	"Staff"
	,	"XO"
	,	"CO"
	,	"??"
	};
	if ( squadRank >= Squad::MAX_RANK )
		return squadRanks[Squad::MAX_RANK];
	return squadRanks[squadRank];
}

#ifdef AUTO_UPDATE_SQUADS
//////////////////////////////////////////////////////////////////////
// Auto-update all the members of a squad-list
///NOTE: We decided to save some cpu/bandwidth by making the client
///periodically request updated squad info when the player has their
///squad tab up.

void
_autoUpdateSquadlist(SquadList& list)
{
	SQUAD_MEMBERS::iterator it;
	Player* player;

	// We use a cut-off to eliminate over-spammage
	// if there are players within the cutoff window,
	// we mark the squadlist as out-of-sync and
	// don't lose the updated time
	TeulTimeMS cutoff_ms = teulClock64 - 1200;
	bool outOfSync = false;
	UINT32 synced = 0;

	if ( g_debugSquads )
		dbugLog(_FLD, "sq:%u auto-syncing. %u members", list.squad().squadID(), list.members().size());

	for ( it = list.begin(); it != list.end(); ++it )
	{
		player = it->second.m_player;
		if ( player == NULL || !player->initialized() || player->teulIndex() < 0 ) continue;

		// Ignore any that have polled since the update
		if ( player->lastSquadInfoMS() >= list.updated() ) continue;
		if ( player->lastSquadInfoMS() >= cutoff_ms )
		{
			// Lets not spam this guy
			outOfSync = true;
			continue;
		}
		player->sendSquadInfo();
		++synced;
	}

	if ( g_debugSquads )
		dbugLog(_FLD, ", %u synced", synced);

	// If nobody got missed, then we can forget these updates
	if ( !outOfSync )
		list.clearUpdated();
}
#endif

//////////////////////////////////////////////////////////////////////
// Announce the squad to all members

void
SquadList::announce()
{
	// Only do this when the squad list hasn't previously been received
	if ( m_pulled == 0 )
	{
		SQUAD_MEMBERS::iterator it;
		for ( it = begin(); it != end(); ++it )
		{
			Player* player = it->second.player();
			if ( !player || player->teulIndex() < 0 || !player->initialized() )
				continue;
			player->sendSquadInfo(this);
		}
	}

	m_pulled = teulTime();
}

//////////////////////////////////////////////////////////////////////
// Patrol the squad lists for squads that have gone dormant
// Only removes one squad at a time

bool
chatCheckSquadLists()
{
	static const UINT32 EMPTY_SQUAD_RETENTION_TIME_MS = 5 * 60 * 1000;

	bool didNothing = true;

	if ( g_debugSquads )
		dbugLog(_FLD, "%" PRSizeT " squads in memory", g_squads.size());

	TeulTimeMS cutoff = teulClock64 - EMPTY_SQUAD_RETENTION_TIME_MS;

#ifdef UPDATE_SQUADS_FROM_DB
	static const UINT32 SQUAD_REFRESH_INTERVAL_S = 5 * 60;
	UINT32 updatePullsBefore = teulTime() - SQUAD_REFRESH_INTERVAL_S;
#endif

	SQUAD_LISTS::iterator it;
	for ( it = g_squads.begin(); it != g_squads.end(); )
	{
		SquadList& squadList = it->second;
		++it;
		UINT32 squadID = squadList.squad().squadID();

#ifdef UPDATE_SQUADS_FROM_DB
		if ( squadList.pulled() < updatePullsBefore )
		{
			chatRequestSquadMemberList(squadID);
			continue;
		}
#endif //UPDATE_SQUADS_FROM_DB

		if ( squadList.online() == 0 )
		{
			if ( squadList.msstamp() > cutoff )
				continue;

			if ( squadList.squad().callsign() == host_config.squad.defaultSquad )
				continue;

			dbugLog(_FLI, "sq:%u expiring - nobody online", squadID);
			g_squads.erase(squadID);

			didNothing = false;

			continue;
		}

		squadList.electNewCO();

#ifdef AUTO_UPDATE_SQUADS
		if ( squadList.updated() )
			_autoUpdateSquadlist(squadList);
#endif
	}

	return didNothing;
}

//////////////////////////////////////////////////////////////////////
// Send a player a who list update

static inline void
_sendWhoList(const Player* player, const SquadList* list, TeulTimeMS lastInfo)
{
	CHAT_WHO_LIST cwl(CWT_SQUAD, 0, Teul::TCPStdPayload);
	Chat::Who who;
	bool isOnline;
	bool isInSquad;
	FULL_OBJ_ID nearObjID;

	cwl.clear();

	SQUAD_MEMBERS::const_iterator it;
	for ( it = list->begin(); it != list->end(); ++it )
	{
		// Eliminate members who don't have a recent update
		const SquadMemberEntry& member = it->second;
		if ( member.msstamp() < lastInfo )
			continue;

		isOnline = (member.player() != nullptr && member.player()->isVisibleTo(player, true));
		isInSquad = member.squadRank() != Squad::NOT_IN_SQUAD;

		who.setBasics(it->first, isOnline, isInSquad, member.squadRank(), 0LL, member.joined());

		if ( isOnline )
		{
			const Player* target = member.player();
			who.setPrimaryDetails(
				target->isAFK(), target->isActiveOfficer(), target->country(), target->branch(), target->effectiveRank()
				, target->brigadeID(), target->missionID(), target->isMissionLeader(), member.recruiting()
			);

			if ( target->isSpawned() )
			{
				nearObjID = nullObjID;
				if ( target->lastCP() )
					nearObjID = target->lastCP()->Fid();
				who.setExtendedDetails(true, nearObjID);
			}
		}

		cwl.add(who);
	}

	cwl.send(player->teulIndex());
}

//////////////////////////////////////////////////////////////////////
// Helper for the invite functions

static void
_invite(Player* player, UINT32 squadID, const Squad& squad, const ChatHandle from, RANK_NO rank)
{
	SQUAD_MSG invite(squad);
	invite.data().setInviter(from);
	invite.data().setRank(rank);
	invite.send(player->teulIndex());

	player->SetSquadInvite(new SquadInvite(squadID, from, rank));
}

//////////////////////////////////////////////////////////////////////
// Send a player their latest squad info

bool
Player::sendSquadInvite(UINT32 forSquadID, const ChatHandle from)
{
	SquadList* squadList = chatFindSquadList(forSquadID);
	if ( !squadList )
	{
		betaLog(_FLA, "NO SQUAD LIST FOR SQUAD INVITE");
		return false;
	}

	_invite(this, forSquadID, squadList->squad(), from, Squad::RECRUIT);

	return true;
}

//////////////////////////////////////////////////////////////////////
// Send a player their latest squad info
///NOTE: This is currently /requested/ by the client periodically

void
Player::sendSquadCreateInvite(const ChatHandle handle, const std::string& name, const ChatHandle from, RANK_NO withRank)
{
	Squad squad(0, handle, name);
	_invite(this, 0, squad, from, withRank);
}

void
Player::sendSquadInfo(SquadList* squadList/*=NULL*/)
{
	if ( !initialized() )
		return;

	SQUAD_MSG reply;
	SquadMsg& msg = reply.data();

	msg.setStatus(SquadMsg::NO_SQUAD);

	if ( squadList == nullptr )
	{
		if ( !squadID() )
		{
			reply.send(teulIndex());
			return;
		}
		squadList = chatFindSquadList(squadID());
	}

	if ( squadList == nullptr )
	{
		dbugLog(_FLD, "t:%d p:%u c:%s sq:%u Unable to find squad", teulIndex(), playerID(), callsign(), squadID());
		reply.send(teulIndex());
		return;
	}

	msg.setStatus(SquadMsg::NO_CHANGES);

	if ( lastSquadInfoMS() > teulClock64 - SQUAD_UPDATE_INTERVAL_MS )
	{
		dbugLog(_FLD, "t:%d p:%u c:%s sq:%u No changes", teulIndex(), playerID(), callsign(), squadID());
		reply.send(teulIndex());
	}

	// Has the squad info changed since your last update?
	if ( squadList->msstamp() >= lastSquadInfoMS() )
	{
		msg.setSquad(squadList->squad());
		msg.setStatus(SquadMsg::INFO_ONLY);
	}

	bool sendWhoList = false;
	// Check to see if any of the members need sending to you
	if ( squadList->lastMemberMod() >= lastSquadInfoMS() )
	{
		// This will make status be either LIST_ONLY or BOTH
		msg.setStatus(msg.status()+1);
		sendWhoList = true;
	}
	if ( g_debugSquads )
		dbugLog(_FLD, "t:%d p:%u c:%s sq:%u sending squad update %u%s time:%" PR64BitUT " mod:%" PR64BitUT " mmod:%" PR64BitUT " lsi:%" PR64BitUT ""
						, teulIndex(), playerID(), callsign(), squadID()
						, msg.status(), ((sendWhoList) ? "+who" : "")
						, teulClock64, squadList->msstamp(), squadList->lastMemberMod(), lastSquadInfoMS());
	reply.send(teulIndex());

	if ( sendWhoList )
		_sendWhoList(this, squadList, lastSquadInfoMS());

	m_lastSquadInfo_ms = teulClock64;
}

//////////////////////////////////////////////////////////////////////
// Promote a squad member to the rank of CO, deposing any existing CO

void
SquadList::appointNewCO(const ChatHandle newCO, Player* player)
{
	ChatHandle oldCO = squad().coHandle();
	chatSendCOAppointment(squad().squadID(), newCO);
	if ( oldCO != 0LL )
		updateMember(oldCO, Squad::XO, PlayerInterface::Lookup(oldCO));
	updateMember(newCO, Squad::CO, player);
	m_squad.setCOHandle(newCO);
}

//////////////////////////////////////////////////////////////////////
// Send notice of a field being changed by a player

void
SquadList::changeNotice(Player* player, int message)
{
	if ( player )
		sendMessage(LSTR_SQUAD, message, player->callsign());
}

//////////////////////////////////////////////////////////////////////
// Process any vote in progress for a squad

void
SquadList::electNewCO()
{
	SquadVote* vote = chatFindSquadVote(squad().squadID());
	if ( vote == nullptr || vote->m_started == 0 || vote->m_closedUntil != 0 )
		return;

	ChatHandle winner = vote->winner(this);
	if ( winner == 0LL )
	{
		if ( vote->expires() == 0 )
		{
			dbugLog(_FLI, "sq:%s(%u) vote failed: no clear winner", squad().callsign().c_str(), squad().squadID());
			sendMessage(LSTR_SQUAD, LSTR_SQUAD_VOTE_FAIL);
			vote->close(DAY_S);
		}

		return;
	}

	dbugLog(_FLN, "sq:%s(%u) voted new CO: %s", squad().callsign().c_str(), squad().squadID(), Chat::Name(winner).c_str());

	// Ok - now we have to elect a new CO
	ChatHandle oldCO = squad().coHandle();
	LSTRING_ID msg;
	if ( winner == oldCO )
		msg = LSTR_SQUAD_VOTE_SAME;
	else
	{
		msg = LSTR_SQUAD_VOTE_NEW;

		appointNewCO(winner, PlayerInterface::Lookup(winner));
	}

	char lead[16+1];
	strprintf(lead, "%u", vote->m_lead);
	sendMessage(LSTR_SQUAD, msg, Chat::Name(winner).c_str(), lead);
	setUpdated();

	// Close for the default period
	vote->close(0);
}

//////////////////////////////////////////////////////////////////////
// Update all the necessary squad data after a renaming

void
SquadList::renamed(const ChatHandle newHandle)
{
	const std::string oldCallsign = squad().m_callsign;
	const Chat::Name newCallsign(newHandle);
	dbugLog(_FLN, "Squad %s renamed to %s, updating tables and players", squad().m_callsign.c_str(), newCallsign.c_str());
	// Mark the squad as needing its details resent
	squad().m_shortHandle = newHandle;
	squad().m_callsign = newCallsign.c_str();
	setUpdated();

	SQUAD_MEMBERS::iterator it;
	Player* player;
	for ( it = begin(); it != end(); ++it )
	{
		player = it->second.player();
		if ( !player || player->teulIndex() < 0 || !player->initialized() )
			continue;
		player->SetSquad(squad().squadID(), player->squadRank(), newHandle, it->second.joined());
		player->sendSquadInfo(this);
		player->message(LSTR_SQUAD, LSTR_SQUAD_RENAMED, oldCallsign.c_str(), newCallsign.c_str());
	}
}

void
chatRenameSquad(const Chat::Name& fromName, const Chat::Name& toName)
{
	chatRemoveSquadName(fromName.c_str());

	ChatHandle newHandle = toName;
	SquadList* list = chatFindSquadListByName(fromName.c_str());
	if ( list )
		list->renamed(newHandle);
	else
		dbugLog(_FLN, "Renaming squad %s->%s: squad not loaded", fromName.c_str(), toName.c_str());

	chatAddSquadName(newHandle);

	dbugLog(_FLN, "Completed rename of squad %s to %s", fromName.c_str(), toName.c_str());

#ifdef BETA_RELEASE
	if ( !list )
		return;
	SquadList* altList;
	altList = chatFindSquadListByName(fromName.c_str());
	if ( altList != nullptr )
		LogAbort("Found list for old name after squad rename");
	altList = chatFindSquadListByName(toName.c_str());
	if ( altList == nullptr )
		LogAbort("No list for squad after rename");
	else if ( altList->squad().shortHandle() != newHandle )
		LogAbort("Squad name does not appear to have changed after rename");
#endif
}

void
SquadList::setDescription(const std::string& description)
{
	if ( m_squad.m_description != description )
	{
		m_squad.m_description = description;
		setUpdated();
	}
}

void
SquadList::setFullName(const std::string& name)
{
	if ( m_squad.m_name != name )
	{
		m_squad.m_name = name;
		setUpdated();
	}
}

void
SquadList::setHomepage(const std::string& homepage)
{
	if ( m_squad.m_homepage != homepage )
	{
		m_squad.m_homepage = homepage;
		setUpdated();
	}
}

void
SquadList::setMOTD(const std::string& motd)
{
	if ( m_squad.m_motd != motd )
	{
		m_squad.m_motd = motd;
		setUpdated();
	}
}

void
SquadList::setLanguage(UINT32 language)
{
	if ( m_squad.m_language != language )
	{
		m_squad.m_language = language;
		setUpdated();
	}
}

void
SquadList::setRecruiting(bool newFlag, Player* player)
{
	if ( newFlag == m_squad.m_recruiting )
		return;
	dbugLog(_FLD, "t:%d p:%u c:%s sq:%u(%s) recruiting %s"
					, player->teulIndex(), player->playerID(), player->callsign()
					, squad().squadID(), squad().shortName()
					, ((newFlag) ? "enabled" : "disabled"));
	m_squad.m_recruiting = newFlag;
	setUpdated();
	changeNotice(player, newFlag ? LSTR_SQUAD_RECRUITING_ENABLED : LSTR_SQUAD_RECRUITING_DISABLED);
	SQUAD_MEMBERS::iterator it;
	for ( it = begin(); it != end (); ++it )
	{
		Player* member = it->second.player();
		if ( member )
		{
			if ( newFlag && member->getPreference(CPR_IS_RECRUITING) )
				chatUpdateRecruiterList(member, true, this);
			else if ( !newFlag )
				chatUpdateRecruiterList(member, false, this);
		}
	}
}

void
SquadList::setSlogan(const std::string& slogan)
{
	if ( m_squad.m_slogan != slogan )
	{
		m_squad.m_slogan = slogan;
		setUpdated();
	}
}

void
SquadList::setSquadNight(const std::string& squadNight)
{
	if ( m_squad.setSquadNight(squadNight.c_str()) )
		setUpdated();
}

void
SquadList::setVoting(bool newFlag, Player* player)
{
	if ( newFlag == m_squad.m_votingEnabled )
		return;
	m_squad.m_votingEnabled = newFlag;
	setUpdated();
	changeNotice(player, newFlag ? LSTR_SQUAD_VOTING_ENABLED : LSTR_SQUAD_VOTING_DISABLED);
}

SINT32
SquadMemberEntry::teulIndex() const
{
	return ( m_player ) ? m_player->teulIndex() : -1;
}

static bool
_exclusiveSquadFetch(UINT32 squadID)
{
	using _FETCHES = std::vector<std::pair<UINT32,UINT32>>;
	_FETCHES fetches;
	_FETCHES::iterator it;

	// Eliminate expired entries
	for ( it = fetches.begin(); it != fetches.end() && it->first > (UINT32)teulTime(); ++it )
		;
	if ( it != fetches.begin() )
		fetches.erase(fetches.begin(), it);

	for ( it = fetches.begin(); it != fetches.end(); ++it )
	{
		if ( it->second == squadID )
			return false;
	}

	static const UINT32 _LOCK_TIME_S = 30;
	UINT32 lockUntil = teulTime() + _LOCK_TIME_S;
	fetches.push_back(std::make_pair(lockUntil, squadID));

	return true;
}

//////////////////////////////////////////////////////////////////////
// Fetch the squad information

static const char* QRY_SQUAD_INFO =
		"SELECT s.short_name, s.name"
				", cr.callsign, unix_timestamp(s.created)"
				", s.flags, s.voting, s.recruiting"
				", s.rookie_days, s.last_joined, s.last_joiner"
				", s.description, s.motd, s.homepage"
			" FROM wwii_squads s"
				" LEFT JOIN wwii_player cr ON (s.creatorID = cr.playerID)"
			" WHERE s.squadID = %u"
;

class SquadInfoQuery : public DBThreadQuery
{
public:
	SquadInfoQuery(UINT32 squadID)
		: DBThreadQuery(DBThread::Default, "SquadInfoQuery", DBThreadQuery::_CALLBACK, QRY_SQUAD_INFO, squadID)
		{}

public:
	virtual bool Process(const size_t) override
	{
		return true;
	}

	virtual void Success() override
	{
		///TODO: Implement non-DBD squad updating
	}
};

void
chatFetchSquadInfo(UINT32 squadID)
{
	// Is there already a request in progress for this squad
	if ( !_exclusiveSquadFetch(squadID) )
		return;

	// Create a query and hand it to the db threads
	(new SquadInfoQuery(squadID))->Queue();
}

//////////////////////////////////////////////////////////////////////
// Code for managing the squad names list
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// The DB Thread class for taking the names and building a list

class SquadNamesQuery : public DBThreadQuery
{
public:
	SquadNamesQuery() : DBThreadQuery(DBThread::Default, "SquadNamesQuery", "SELECT short_name FROM wwii_squads WHERE disbanded = 0", DBThreadQuery::_IMMEDIATE) {}

protected:
	virtual void Success() override;
};

void
SquadNamesQuery::Success()
{
	std::lock_guard<std::mutex> lock(_namesMutex);
	g_squadNames.clear();

	if ( Query()->HasRows() )
	{
		try
		{
			while ( Query()->FetchRow() )
			{
				const char* name = Query()->Row()[0];
				ChatHandle handle = chatGetHandle(name);
				g_squadNames[chatGetLowercaseHandle(handle)] = handle;
			}
		}
		catch (std::exception& e)
		{
			dbugLog(_FLW, "SquadNames: %s", e.what());
			g_squadNames.clear();
		}
	}
}

void
chatFetchSquadNames()
{
	static UINT32 nextFetch = 0;
	if ( teulTime() < nextFetch )
		return;
	// Fetch again once every hour.
	static const UINT32 _FETCH_INTERVAL_S = 5 * 60;
	nextFetch = teulTime() + _FETCH_INTERVAL_S;
	(new SquadNamesQuery())->Queue();
}

//////////////////////////////////////////////////////////////////////
// Add a squad name to the index

void
chatAddSquadName(const char* name)
{
	chatAddSquadName(chatGetHandle(name));
}

void
chatAddSquadName(const ChatHandle handle)
{
	std::lock_guard<std::mutex> lock(_namesMutex);
	g_squadNames[chatGetLowercaseHandle(handle)] = handle;
}

//////////////////////////////////////////////////////////////////////
// Return true if a squad name is in the list

bool
chatCheckForSquadName(const char* name)
{
	return chatCheckForSquadName(chatGetHandle(name));
}

bool
chatCheckForSquadName(const ChatHandle handle)
{
	std::lock_guard<std::mutex> lock(_namesMutex);
	const bool exists = (g_squadNames.find(chatGetLowercaseHandle(handle)) != g_squadNames.end());
	return exists;
}

//////////////////////////////////////////////////////////////////////
// Remove a squad name if it is in the list

void
chatRemoveSquadName(const char* name)
{
	chatRemoveSquadName(chatGetHandle(name));
}

void
chatRemoveSquadName(const ChatHandle handle)
{
	std::lock_guard<std::mutex> lock(_namesMutex);
	SQUAD_NAMES::iterator it = g_squadNames.find(chatGetLowercaseHandle(handle));
	if ( it != g_squadNames.end() )
		g_squadNames.erase(it);
}

//////////////////////////////////////////////////////////////////////
// Add or remove a player to/from the list of active recruiters

void
chatUpdateRecruiterList(const Player* player, bool recruiting, SquadList* list)
{
	if ( player == nullptr )
		return;

	if ( list != nullptr )
		list->updateRecruiter(player->matchHandle(), recruiting);

	// Simplest case: player is not recruiting.
	if ( recruiting == false )
	{
		g_recruiters.erase(player);
		return;
	}

	try
	{
		if ( !player->squadID() )
			throw "Player not in a squad";
		if ( player->squadRank() < Squad::STAFFER )
			throw "Player has insufficient squad-rank";

		if ( list == nullptr )
			throw "Squad not loaded/no such squad";

		if ( !list->squad().recruiting() )
			throw "Squad has recruiting disabled";

        dbugLog(_FLD, "t:%d p:%u c:%s adding to recruiter list"
                        , player->teulIndex(), player->playerID(), player->callsign());

		g_recruiters.insert(player);
	}
	catch (const char* e)
	{
		RECRUITERS::iterator it = g_recruiters.find(player);
		if ( it != g_recruiters.end() )
		{
            dbugLog(_FLD, "t:%d p:%u c:%s removing from recruiter list: %s"
                            , player->teulIndex(), player->playerID(), player->callsign(), e);
			g_recruiters.erase(it);
		}
	}
}

//////////////////////////////////////////////////////////////////////
// Build a recruiter object from a player and a squad

Recruiter::Recruiter(const Player* player, const SquadList* list, const ChatMission* mission)
	: m_handle(player->chatHandle())
	, m_squadHandle(list->squad().shortHandle())
	, m_country(player->country())
	, m_branch(player->branch())
	, m_language(player->getPreference(CPR_PLAYER_LANGUAGE))
	, m_mission(0)
	, m_activity(list->online())
	, m_squadName(list->squad().name())
	, m_slogan(list->squad().slogan())
	, m_origin(nullObjID)
	, m_target(nullObjID)
{
	m_origin.FromKey(mission->originCP());
	m_target.FromKey(mission->targetCP());
}

//////////////////////////////////////////////////////////////////////
// Receive a request for a list of recruiters by a player

void
chatRequestRecruiter(Player* player, UCHAR language)
{
	RECRUITER_LIST outgoing;
	RECRUITERS::const_iterator it;

	using RECRUITER_WEIGHTING = std::multimap<UINT32, Recruiter>;
	RECRUITER_WEIGHTING dupes, spares;
	std::set<UINT32> listedSquads;

	for ( it = g_recruiters.begin(); outgoing.size() < host_config.squad.recruiterListMin && it != g_recruiters.end(); ++it )
	{
		const Player* recruiter = *it;

		SquadList* list = recruiter->isAvailableRecruiter(player);
		if ( list == nullptr )
			continue;

		ChatMission* mission = chatFindMission(recruiter->missionID());
		if ( mission == nullptr )
			continue;

		Recruiter entry(recruiter, list, mission);

		UINT32 weight = 0;

		// See if they are a good match
		if ( recruiter->country() != player->country() )
			weight += 1;
		if ( recruiter->branch() != player->branch() )
			weight += 2;
		if ( recruiter->getPreference(CPR_PLAYER_LANGUAGE) != language )
			weight += 4;

		// So far so good, now perform cream-of-the-crop filtering
		if ( weight == 0 )
		{
			// Guys with recruits or who's squads have already been
			// listed move down the list
			weight += chatGetNumberOfRecruits(recruiter->playerID());
			if ( listedSquads.find(recruiter->squadID()) != listedSquads.end() )
				weight += 3;

			// No penalties, you get added straight away
			if ( weight == 0 )
			{
				outgoing.add(entry);
				listedSquads.insert(recruiter->squadID());
			}
			else
			{
				// Otherwise you get filed for later consideration
				dupes.insert(std::make_pair(weight, entry));
			}
		}
		else
			spares.insert(std::make_pair(weight, entry));
	}

	RECRUITER_WEIGHTING::iterator rit;

	// Do we have room for guys who weren't ideal?
	for ( rit = dupes.begin(); outgoing.size() < host_config.squad.recruiterListMax && rit != dupes.end(); ++rit )
	{
		outgoing.add(rit->second);
	}

	// If we didn't reach the minimum list length yet, fill the list out with a bunch more guys.
	if ( outgoing.size() < host_config.squad.recruiterListMin )
	{
		for ( rit = spares.begin(); outgoing.size() < host_config.squad.recruiterListMax && rit != spares.end(); ++rit )
		{
			outgoing.add(rit->second);
		}
	}

	outgoing.send(player->teulIndex());
}
