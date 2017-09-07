#pragma once
// Created: Tue Feb 15 17:16:22 CST 2005
//
// Defines all of the query strings used by mapDB.cpp
// Use char arrays rather than pointers-to so that
// sizeof() can be used.
//
// Copyright (C) Playnet, Inc. 2005, All rights reserved
// Not for redistribution
//
// Originating Programmer: Oliver "kfsone" Smith
//

// Record a new instance of the host so that we can get
// an instance ID via lastInsertID
#if !defined(DEV_BUILD)
static const char QRY_INSERT_INSTANCE[] =
	"INSERT INTO wwii_instance"
		" (arenaID, hostIP, timestamp)"
	" VALUES"
		" ('%s', '%s', now())"
;
#endif

static const char QRY_SELECT_ACTIVE_COUNTRIES[] =
	"SELECT c.countryID"
		" FROM wwii_country c"
			" WHERE is_active > 0"
;

static const char QRY_SELECT_ARENA_DESCRIPTION[] =
	"SELECT a.name, a.value"
		" FROM wwii_arena a"
;

// Get a player's list of personas
static const char QRY_SELECT_PERSONA_LIST[] =
	"SELECT	c.countryid"
			", b.branchid"
			", pers.personaid"
			", pers.rankid"
			", pers.rankpoints"
			", r.pointsrequired"
			", unix_timestamp(pers.modified)"
			", pers.brigade_id"
		" FROM wwii_player plyr INNER JOIN wwii_country c INNER JOIN wwii_branch b"
			" LEFT OUTER JOIN wwii_persona pers"
				" ON (plyr.playerid = pers.playerid"
					" AND c.countryID = pers.countryID"
					" AND b.branchid = pers.branchid)"
					" LEFT OUTER JOIN wwii_rank r USING (rankID)"
			" WHERE plyr.playerid = %u AND c.is_active > 0"
			" ORDER BY pers.modified desc"
			" LIMIT %u"
;

// Add a new persona
// Note that we don't specify the rankID so that the server can set the default
static const char QRY_INSERT_NEW_PERSONA_RECORD[] =
	"INSERT INTO wwii_persona"
		"(playerID, countryID, branchID, usedmissnpts, rankpoints, brigade_id, added)"
	" VALUES"
		"("
		 "%u,"     "%u,"      "%u,"     "0,"          "0,"         "0,"      "now()"
		")"
;

static const char QRY_SELECT_PERSONA_DATA[] = "CALL `wwii`.`p_get_persona_data`(%u, %u, %u)";
