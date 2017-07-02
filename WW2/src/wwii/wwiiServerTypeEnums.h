#pragma once
// Created: Tue Jan 15 15:35:21 CST 2008
//
// Server type data shared between client/host outside of serverTypes
//
// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

enum SERVER_TYPE
{
	CONTROLLER_HOST = 0U,
	AUTH_HOST,
	DBD_HOST,
	STRAT_HOST,
	CHAT_HOST,
	MAP_HOST,
	CELL_HOST,
	EXIT_SERVER,
	MAX_SERVER_TYPE,
};

extern const char* g_serverTypes[MAX_SERVER_TYPE + 1];
