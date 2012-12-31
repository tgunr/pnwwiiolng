#pragma once
// Created: Tue Jan 15 15:35:21 CST 2008
//
// Server type data shared between client/host outside of serverTypes
//
// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith
//
//tolua_begin

enum SERVER_TYPE
{
	AUTH_HOST = 0,
	CELL_HOST,
	MAP_MISSION,
	STRAT_DAEMON,
	STRAT_UPDATER,
	CHAT_DAEMON,
	NIQ_HOST,
	MAX_SERVER_TYPE
} ;

#define MAX_HOSTS_IP 4

//tolua_end
