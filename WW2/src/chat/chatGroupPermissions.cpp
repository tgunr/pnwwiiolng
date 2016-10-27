#include <syscfg.h> // Path is project specific.
// Created: Mon Feb  2 16:48:15 CST 2009
//
// Manage the reading/processing of the chat_groups
// and chat_group_permissions tables.
//
// Copyright (C) PlayNet, Inc 1999-2009 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "chat/chatGroupsType.h"			// For CHAT_GROUPS
#include "chat/chatGroupPermissionTypes.h"	// For ChatPermissions struct
#include "chat/chatDB.h"
#include "include/mydbQuery.h"
#include "teul/teulBlipRepeater.h"
#include "weap/weapWeapon.h"
#include "wwii/wwiiDBThreads.h"
#include "wwii/wwiiPermissions.h"

//	------------------------------	Declarations

// Zero permissions
const ChatPermissions nullChatPermissions;

static const UINT16 _MAX_GROUP_OID = (UINT16)~0;

using GROUP_PERMISSIONS = std::map<UINT16, ChatPermissions>;
GROUP_PERMISSIONS g_groupPermissions;

struct PermissionGroup
{
	UINT32			m_lastUsed;
	ChatPermissions	m_permissions;		// What this produces.
};
using PERMISSION_GROUP_CACHE = std::map<CHAT_GROUPS, PermissionGroup>;
PERMISSION_GROUP_CACHE g_pgc;

//////////////////////////////////////////////////////////////////////
// Take list of groups and aggregate the vehicle lists

static void
_aggregateVehicles(ChatPermissions& lhs, const CHAT_GROUPS& groups)
{
	lhs.m_vehicleGrants.clear();

	for ( auto it = groups.begin(), end = groups.end(); it != end; ++it )
	{
		GROUP_PERMISSIONS::iterator pit = g_groupPermissions.find(*it);
		if( pit == g_groupPermissions.end() )
			continue;

		ChatPermissions& rhs = pit->second;
		if ( rhs.m_enabled == true )
			lhs.m_vehicleGrants.insert(rhs.m_vehicleGrants.begin(), rhs.m_vehicleGrants.end());
	}
}

//////////////////////////////////////////////////////////////////////
// Take the list of chat groups and aggregate the values

static void
_aggregatePermissions(ChatPermissions& lhs, const CHAT_GROUPS& groups)
{
	lhs = ChatPermissions();

	// Read each chat group and aggregate
	for ( auto it = groups.begin(), end = groups.end(); it != end; ++it )
	{
		GROUP_PERMISSIONS::iterator pit = g_groupPermissions.find(*it);
		if( pit == g_groupPermissions.end() )
			continue;

		ChatPermissions& rhs = pit->second;
		if ( rhs.m_enabled == false )
			continue;

		// Aggregate the ranks
		lhs.m_limitRank[BRANCH_LAND] = std::max(lhs.m_limitRank[BRANCH_LAND], rhs.m_limitRank[BRANCH_LAND]);
		lhs.m_limitRank[BRANCH_AIR] = std::max(lhs.m_limitRank[BRANCH_AIR], rhs.m_limitRank[BRANCH_AIR]);
		lhs.m_limitRank[BRANCH_SEA] = std::max(lhs.m_limitRank[BRANCH_SEA], rhs.m_limitRank[BRANCH_SEA]);
		lhs.m_limitSquadRank = std::max(lhs.m_limitSquadRank, rhs.m_limitSquadRank);

		// Or the masks
		lhs.m_permissionMask |= rhs.m_permissionMask | SUBSCRIPTION_ACTIVE;
	}

	if ( lhs.m_permissionMask & UNMASKED_VEHICLES )
	{
		lhs.m_vehicleGrants.clear();
	}
	else
	{
		_aggregateVehicles(lhs, groups);
	}

    std::string groupNos = "";
    for ( auto it = groups.begin(), end = groups.end(); it != end; ++it )
    {
        char num[32];
        strprintf(num, (it != groups.begin() ? ",%u" : "%u"), *it);
        groupNos += num;
    }
    char buffer[2048];
    strprintf(buffer, "For groups {%s} maxRanks{army:%u,air:%u,navy:%u,sqd:%u} perms=%" PRPermMaskT " vehGrants=%" PRSizeT ""
                            , groupNos.c_str()
                            , lhs.m_limitRank[BRANCH_LAND]
                            , lhs.m_limitRank[BRANCH_AIR]
                            , lhs.m_limitRank[BRANCH_SEA]
                            , lhs.m_limitSquadRank
                            , lhs.m_permissionMask
                            , lhs.m_vehicleGrants.size()
                            );
    threadLog(_FLD, "%s", buffer);
}

//////////////////////////////////////////////////////////////////////
// Given a database row, update the chat group.

static const char* _GROUP_QUERY = "CALL auth.p_get_chat_group_summary()";
class GroupReadQuery : public DBThreadQuery
{
public:
	GroupReadQuery() : DBThreadQuery(DBThread::Default, "GroupReadQuery", _GROUP_QUERY, DBThreadQuery::_CALLBACK), perms() { }

private:
	std::vector<ChatPermissions> perms;
	std::vector<std::string> vehicles;

public:
	virtual bool Process(const size_t numRows) override
	{
		if ( numRows <= 0 )
			return true;

		while ( Query()->FetchRow() )
		{
			ChatPermissions perm;
			auto& row = Query()->Row();
			perm.m_groupOID = row.NextUInt();
			perm.m_enabled = row.NextUInt();
			perm.m_limitRank[BRANCH_LAND] = (RANK_NO)row.NextUInt();
			perm.m_limitRank[BRANCH_AIR] = (RANK_NO)row.NextUInt();
			perm.m_limitRank[BRANCH_SEA] = (RANK_NO)row.NextUInt();
			perm.m_limitSquadRank = row.NextUInt();
			perm.m_permissionMask = row.NextUINT64();
			perms.push_back(perm);
			vehicles.push_back(row.NextString());
		}

		return true;
	}

public:
	virtual void Success() override
	{
		// Clear the previous list of groups
		g_groupPermissions.clear();

		const size_t numRows = perms.size();
		for ( size_t i = 0; i < numRows; ++i )
		{
			ProcessRow(perms[i], vehicles[i]);
		}

		// Recreate all the cached permissions
		for ( auto it = g_pgc.begin(), end = g_pgc.end(); it != end; ++it )
		{
			_aggregatePermissions(it->second.m_permissions, it->first);
		}

        threadLog(_FLD, "%u groups loaded, %u in cache", numRows, g_pgc.size());
	}

	virtual void Failure() override
	{
		dbugLog(_FLA, "GroupReadQuery failed!");
	}

	void ProcessRow(const ChatPermissions& perm, const std::string& vehicleList) const
	{
		GROUP_PERMISSIONS::iterator git = g_groupPermissions.lower_bound(perm.m_groupOID);
		if ( git == g_groupPermissions.end() )
		{
			// This group is not in the cache; if it is not enabled
			// then we should just ignore it as we won't read it
			// again until someone modifies it (e.g. enables it)
			if ( perm.m_enabled == false )
			{
				// We aren't in memory and we aren't enabled; ignore me.
				return;
			}

			git = g_groupPermissions.insert(git, std::make_pair(perm.m_groupOID, perm));
			assert( git != g_groupPermissions.end() );
		}
		else
		{
			// Modify/register this group.
			auto& dest = git->second;
			dest.m_groupOID = perm.m_groupOID;
			dest.m_enabled = perm.m_enabled;
			dest.m_limitRank[BRANCH_LAND] = perm.m_limitRank[BRANCH_LAND];
			dest.m_limitRank[BRANCH_AIR] = perm.m_limitRank[BRANCH_AIR];
			dest.m_limitRank[BRANCH_SEA] = perm.m_limitRank[BRANCH_SEA];
			dest.m_limitSquadRank = perm.m_limitSquadRank;
			dest.m_permissionMask = perm.m_permissionMask;
		}

		ProcessVehicles(git->second, vehicleList.c_str());

        dbugLog(_FLD, "grp:%u %s maxRanks{army:%u,air:%u,navy:%u,sqd:%u} perm:%" PRPermMaskT " vehs:%" PRSizeT ""
                        , perm.m_groupOID
                        , (perm.m_enabled ? "enabled" : "disabled")
                        , perm.m_limitRank[BRANCH_LAND]
                        , perm.m_limitRank[BRANCH_AIR]
                        , perm.m_limitRank[BRANCH_SEA]
                        , perm.m_limitSquadRank
                        , perm.m_permissionMask
                        , perm.m_vehicleGrants.size()
                        );
	}

	void ProcessVehicles(ChatPermissions& perm, const char* list) const
	{
		perm.m_vehicleGrants.clear();

		if ( perm.m_permissionMask & UNMASKED_VEHICLES || list == nullptr || *list == 0 )
		{
			return;
		}

		const char* end = nullptr;
		while ( list && *list != 0 )
		{
			// Read the number between here and there
			UINT32 typeOID = (UINT32)strtol(list, nullptr, 10);
			WEAPON* weap = weapLookupWeaponByIndex(typeOID);
			if ( weap == nullptr )
			{
				dbugLog(_FLD, "Group %u contains unknown vehtype_oid %u", perm.m_groupOID, typeOID);
			}
			else
			{
				// Add it to the vehicle list
				perm.m_vehicleGrants.insert(weap->spec().asUINT16());
			}

			// Find the next comma
			end = strchr(list, ',');
			if ( end && *end == ',' )
				++end;
			list = end;
		}
	}
};

void
chatReadGroupPermissions()
{
#ifdef WHEN_POINTERS_TO_THEM_ARE_REFCOUNTED
	// Expire old entries
	UINT32 cutoff = teulTime() - 3 * 60;
	PERMISSION_GROUP_CACHE::iterator cur, it = g_pgc.begin();
	while ( it != g_pgc.end() )
	{
		cur = it++;
		if ( cur->second.m_lastUsed <= cutoff )
			g_pgc.erase(cur);
	}
#endif

	(new GroupReadQuery())->Queue();
}

//////////////////////////////////////////////////////////////////////
// Housekeeping entry for reading group permissions.

bool
chatHousekeepGroupPermissions()
{
	// Refresh data once an hour
	static Teul::BlipRepeater nextRepeat(5 * 60 * 1000);
	if ( !nextRepeat.Ready() )
		return true;

	chatReadGroupPermissions();

	return true;
}

//////////////////////////////////////////////////////////////////////
// Given a group list, look it up or populate it.

const ChatPermissions*
chatGroupPermissionsForGroups(const CHAT_GROUPS& groups)
{
	PERMISSION_GROUP_CACHE::iterator it = g_pgc.find(groups);
	if ( it != g_pgc.end() )
	{
		if ( it->second.m_lastUsed < teulTime() )
			it->second.m_lastUsed = teulTime();
		return &(it->second.m_permissions);
	}

	PermissionGroup& pg = g_pgc[groups];
	pg.m_lastUsed = teulTime();
	_aggregatePermissions(pg.m_permissions, groups);

	return &pg.m_permissions;
}

//////////////////////////////////////////////////////////////////////
// Read a comma-separated list of groups into a CHAT_GROUPS structure

void
chatReadGroups(const char* source, CHAT_GROUPS& into)
{
	into.clear();

	UINT32 groupOID;
	while ( source != nullptr && *source )
	{
		// Find and skip the trailing comma
		const char* p = strchr(source, ',');
		if ( p )
			++p;

		groupOID = (UINT32)atoi(source);
		if ( groupOID > (UINT32)_MAX_GROUP_OID )
			LogAbort("chat_group group_oid > %u is invalid", _MAX_GROUP_OID);
		into.insert((UINT16)groupOID);

		source = p;
	}
}

