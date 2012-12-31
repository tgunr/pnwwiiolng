#pragma once
// Created: Wed Feb 11 15:42:39 CST 2009
//
// Simple struct that encapsulates the results from
//  auth.p_get_player_details
//
// Copyright (C) PlayNet, Inc 1999-2009 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "include/mydbQuery.h"
#include "wwii/wwiiPermissions.h"	// For SUBSCRIPTION_ACTIVE

struct AuthPlayerDetails
{
	AuthPlayerDetails(const DBRow& row)
		: playerID(row.NextUInt())
		, permissions(row.NextUINT64())
		, callsign(row.NextString())
		, customerID(row.NextUInt())
		, squadID(row.NextUInt())
		, squadRank(row.NextUInt())
		, squadCallsign(row.NextString())
		, decal(row.NextUInt())
		, lockTime(row.NextUInt())
		, lockSide(row.NextUInt())
		, lastPoll(row.NextUInt())
		, lastLogin(row.NextString())
		, groups(row.NextString())
		, lockTimePretty(row.NextString())
		, minuteBalance(row.NextUInt())
		{
			if ( groups == NULL || *groups == 0 )
			{
				// If you have no groups, you don't have any active subscriptions.
				permissions &= ~SUBSCRIPTION_ACTIVE ;
			}
			else
			{
				// Your subscription is active/
				permissions |= SUBSCRIPTION_ACTIVE ;
			}
		}

	playerid_t			playerID ;
	PermissionMask		permissions ;
	const char*			callsign ;
	playerid_t			customerID ;
	UINT32				squadID ;
	UINT32				squadRank ;
	const char*			squadCallsign ;
	UINT32				decal ;
	UINT32				lockTime ;
	UINT32				lockSide ;
	UINT32				lastPoll ;
	const char*			lastLogin ;
	const char*			groups ;
	const char*			lockTimePretty ;
	UINT32				minuteBalance ;
} ;

