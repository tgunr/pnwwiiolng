#pragma once
// Created: Thu Sep  7 17:11:01 2000
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Eric Hall

namespace Dbse
{
	enum class ConnectionStatus
	{
		None,
		Bad,
		Open,
		Refuse,
		Accept,
		Dropped
	};

	struct Handshake
	{
		enum { CurrentVersion = 1 };
		Handshake(UINT32 arenaID) : m_version(CurrentVersion), m_arenaID(arenaID), m_status(ConnectionStatus::None) {}
		UINT32				m_version;
		UINT32				m_arenaID;
		ConnectionStatus 	m_status;
	};
}
