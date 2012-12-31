#pragma once
// Created: Sun Sep  2 14:12:20 CDT 2007
//
// Describes a player's transfer to or from the cellhosts
//
// Copyright (C) PlayNet, Inc 1999-2007 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#pragma pack(push, 1)
struct CellXfer
{
	CellXfer(playerid_t playerID, bool onCell) : m_playerID(playerID), m_onCell(onCell) {}

	playerid_t	m_playerID ;
	bool		m_onCell ;
} ;
#pragma pack(pop)
