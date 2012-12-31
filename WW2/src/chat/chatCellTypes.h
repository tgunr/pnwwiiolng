#pragma once
// Created: Wed Apr 20 15:27:18 CDT 2005
//
// Types shared between the chat host and the cell hosts
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "weap/weapSpec.h"

struct CHAT_SMITE_USER
{
	playerid_t	customerID;
	ChatHandle	m_handle ;
} ;

struct CHAT_LOC_USER
{
	playerid_t playerID;
	UINT32 reqPlayer;
	SINT32 reqConnection;
} ;

struct RAW_POSITION
{
	SINT32	rawX;
	SINT32	rawY;
	SINT32	rawZ;
} ;

struct CHAT_FOUND_USER
{
	CHAT_LOC_USER	locUser ;
	UINT32			serverAddress ;
	SINT16			serverType ;
	UINT32			vehicleID ; 	// vehicle index
	RAW_POSITION 	position ;		// position
	LOGICAL 		isFound ;
	WEAPON_SPEC		weapon ;
} ;

