#pragma once
// Created: Sat Jan 23 21:28:35 CST 2010
//
// Defines the 'GAME' structure used by the authHost
// to track game instance stats for individual cluster
// hosts.
//
// Copyright (C) PlayNet, Inc 1999-2010 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

struct GAME
{
public:
	GAME();
	GAME(const struct AUTH_GAME_INIT& fromInit);

	~GAME();

	void server(SERVER* server_) { m_server = server_; }
	SERVER* server() const { return m_server; }

	// Check the login queue and allow any waiting
	// players into the game.
	void CheckLoginQueue();

	// Does this game need its queue checking?
	bool NeedsQueueProcessing() const;

public:
	UINT32	arenaID;				//!< The arena number (distinguishes unique server clusters).
	UINT32	curHostVersion;			//!< Version of host running on cluster.
	UINT32	appID;					//!< Which product (e.g. WWII Public, WWII Beta etc).

	SINT32	currentUsers;			//!< Number of users logged in.
	UINT32	maxUsers;				//!< Maximum users supported by cluster.
	SINT32	realIndex;				//!< Value sent to us by the mmhost which probably means nothing.
	UINT32	gameID;
	UINT32	groupID;
	UINT32	clusterID;
	UINT32	m_lastReported;			//!< Limit frequency of reporting to reduce spam.
	SINT32	lock;					//!< ArenaLock status.

protected:
	SERVER*	m_server;				//!< Server currently authoritative for this game.

public:
	bool	requiresMax : 1;			//!< If the game can only be run using the latest version
	bool	calcMaxUsersOnTheFly : 1; 	//!< Keep tally of maxUsers ourself.
	bool	usePlayerID : 1;			//!< Use playerIDs in database queries.

	APP_INFO appInfo[OS_MAX_PLAT];

	SCHAR	appleID[APPLE_ID_LEN+1]; //	connect to this game
	SCHAR	description[MAX_DESCRIPTION+1];
	SCHAR	playerTable[MAX_DESCRIPTION+1];
	SCHAR	alias[MAX_ARENA_ALIAS_SIZE];

	UINT32	m_lastIRCUsers, m_lastIRCReport;
};

