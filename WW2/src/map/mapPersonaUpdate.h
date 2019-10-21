#pragma once
// Created: Wed Aug  2 15:04:38 CDT 2006
//
// Update fields in the map persona from the chat host
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#pragma pack(push, 1)
class MapPersonaUpdate
{
public:
	MapPersonaUpdate() : m_squadHandle(0), m_isSquadUpdate(false), m_isDecalUpdate(false), m_playerID(0), m_squadID(0), m_decal(0) {}
	MapPersonaUpdate(const ChatHandle squadHandle_, const bool isSquadUpdate_, const bool isDecalUpdate_, const UINT32 playerID_, const UINT32 squadID_, const UINT32 decal_)
		: m_squadHandle(squadHandle_)
		, m_isSquadUpdate(isSquadUpdate_)
		, m_isDecalUpdate(isDecalUpdate_)
		, m_playerID(playerID_)
		, m_squadID(squadID_)
		, m_decal(decal_) {}
	~MapPersonaUpdate() {}

public:
	ChatHandle	m_squadHandle;
	bool		m_isSquadUpdate;
	bool		m_isDecalUpdate;
	UINT32		m_playerID;
	UINT32		m_squadID;
	UINT32		m_decal;
};
#pragma pack(pop)

