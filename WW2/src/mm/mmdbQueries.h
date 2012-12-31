#pragma once
// Created: Thu Mar  3 12:26:17 CST 2005
//
// Defines all of the query strings used by mmdbParser.cpp
// Use char arrays rather than pointers-to so that
// sizeof() can be used.
//
// Copyright (C) Playnet, Inc. 2005, All rights reserved
// Not for redistribution
//
// Originating Programmer: Oliver "kfsone" Smith

// Update the sidelock on a player

static const char QRY_UPDATE_PENDING_EXT_MISSIONS[] =
	"UPDATE ext_missions"
		" SET pending = 0"
		" WHERE pending > 0"
			" AND mission_id IN (%s)"
;

static const char QRY_SELECT_EXT_MISSIONS[] =
	"SELECT"
		"  e.mission_id"
		", e.mission_type"
		", e.priority"
		", e.poster_id"
		", IF (p.isplaynet,UPPER(p.callsign),LOWER(p.callsign))"
		", e.posted_time"
		", e.description"
		", e.country"
		", e.brigade_id"
		", UNIX_TIMESTAMP(e.post_at)"
		", UNIX_TIMESTAMP(e.launch_at)"
		", e.launch_cp_oid"
		", e.launch_fac_oid"
		", e.target_cp_oid"
		", e.target_fac_oid"
		", e.vcountry"
		", e.vcategory"
		", e.vclass"
		", e.vtype"
	" FROM ext_missions e LEFT JOIN wwii_player p ON (e.poster_id = p.playerid)"
	" WHERE e.pending = 1"
		" AND e.post_at <= now()"
	" ORDER BY e.post_at ASC, e.launch_at ASC"
	" LIMIT 10"
;

