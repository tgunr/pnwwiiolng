#pragma once
// Created: Thu Sep  7 17:11:01 2000
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Eric Hall

enum CONNECTION_STATUS
{
	CONNECT_NONE,
	CONNECT_BAD,
	CONNECT_OPEN,
	CONNECT_REFUSE,
	CONNECT_ACCEPT,
	CONNECT_DROPPED
};

struct DBSE_HANDSHAKE
{
	enum { CurrentVersion = 1 };
	DBSE_HANDSHAKE(UINT32 arenaID_) : m_version(CurrentVersion), m_arenaID(arenaID_), m_status(CONNECT_NONE) {}
	UINT32				m_version;
	UINT32				m_arenaID;
	CONNECTION_STATUS 	m_status;
};

enum DBSE_RANK
{
	DBSE_PRIMARY = 0,
	DBSE_SECONDARY,
	DBSE_SLAVE,
	DBSE_REDIRECT,
	DBSE_MAX_RANK
};

enum
{
	SUCCESS = 0,
	FAIL_VERSION,
	FAIL_CONNECT
};
