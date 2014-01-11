#include <syscfg.h> // Path is project specific.
// Created: Thu Apr  7 11:30:49 CDT 2005
//
// Rank information
//
// Copyright (C) PlayNet, Inc 2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "wwii/wwiiRank.h"
#include "include/mydbQuery.h"

// The table of all high command units
Rank::RANKS Rank::s_ranks;
RANK_NAMES rankNames;
UINT32 Rank::s_max;

static const char* QRY_SELECT_RANKS =
	"SELECT"
		" r.rankid"
		", r.type"
		", r.pointsrequired"
		", r.description"
		", bit_or(p.maskvalue)"
		" FROM wwii_rank r"
			" LEFT JOIN wwii_rank_permiss rp USING (rankid)"
				" LEFT JOIN auth.wwii_permissions p USING (permissionid)"
		" GROUP BY 1"
		" ORDER BY 1 desc"
;

//////////////////////////////////////////////////////////////////////
// Add a new rank entry to the static group

void
Rank::add(const RANK_NO level, const Rank::TYPE type, const UINT32 points, const char* description, const PermissionMask& perm)
{
	if ( !valid(level) )
		Rank::setSize(level);

	if ( level > s_max )
		s_max = level;

	if ( description == nullptr )
		description = rankNames[0];

	dbugLog(_FLD, "r:%d new rank: type:%d, perms:%llx, points: %d :: %s", level, type, perm, points, description);

	Rank& rank = s_ranks[level];
	rank.m_level = level;
	rank.m_type = type;
	rank.m_perm = perm;
	rank.m_points = points;

	rankNames[level] = strdup(description);
}

//////////////////////////////////////////////////////////////////////
// Return the next scoreable rank

Rank*
Rank::next() const
{
	if ( level() == 0 )
		return nullptr;

	if ( kingOfTheHill() )
		return nullptr;

	RANK_NO nextLevel = level() + 1;
	if ( nextLevel >= s_ranks.size() )
		return nullptr;

	// To become "if ( s_ranks[nextLevel].kingOfTheHill() )" once KOTH includes officers
	if ( s_ranks[nextLevel].kingOfTheHill() )
		return nullptr;

	return &s_ranks[nextLevel];
}

//////////////////////////////////////////////////////////////////////
// Pre-allocate enough vector storage for the ranks

void
Rank::setSize(size_t num)
{
	if ( num >= s_ranks.size() )
	{
		s_ranks.resize(num + 1);
		rankNames.resize(num + 1);
	}

	s_ranks[0].m_type = s_ranks[num].m_type = NO_TYPE;
	s_ranks[0].m_level = s_ranks[num].m_level = 0;
	s_ranks[0].m_perm = s_ranks[num].m_perm = 0L;
	s_ranks[0].m_points = s_ranks[num].m_points = 65535;
}

//////////////////////////////////////////////////////////////////////
// Load in the entire hierachy of HC Units in depth-descending order
// so that we can build a complete tree in a single pass.

void
wwiiReadRanks(DBHANDLE& dbh)
{
	try
	{
		DBQuery dbq(dbh, QRY_SELECT_RANKS);

		Rank::setSize(dbq.Rows());

		// Ranks are 1-indexed
		for ( UINT32 i = 1; i <= dbq.Rows(); ++i )
		{
			rankNames[i] = nullptr;
		}
		rankNames[0] = strdup("<none>");

		while (dbq.FetchRow())
		{
			DBRow& row = dbq.Row();
			UINT32 level = row.NextUInt();
			UINT32 type = row.NextUInt();
			UINT32 points = row.NextUInt();
			const char* description = row.NextColumn();
			PermissionMask mask = row.NextUINT64();

			// rankid, type, points, description, mask
			Rank::add(level, (Rank::TYPE)type, points, description, mask);
		}

		dbugLog(_FLI, "Added %u ranks, %" PRSizeT " in list", dbq.Rows(), Rank::count());
	}
	catch (std::exception &e)
	{
		dbugLog(_FLA, "%s: %s", __FUNCTION__, e.what());
	}
}

//////////////////////////////////////////////////////////////////////
// Release all the rank information

void
wwiiFreeRanks()
{
	UINT32 i;

	for ( i = 0; i < Rank::count(); ++i )
	{
		if ( rankNames[i] != nullptr )
		{
			free((void*)rankNames[i]);
			rankNames[i] = nullptr;
		}
	}
}

