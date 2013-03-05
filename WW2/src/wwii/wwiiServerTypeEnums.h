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
	CONTROLLER_HOST = 0U,
	AUTH_HOST,
	DBD_HOST,
	STRAT_HOST,
	CHAT_HOST,
	MAP_HOST,
	PING_HOST,
	CELL_HOST,
	MAX_SERVER_TYPE,
} ;

//tolua_end
