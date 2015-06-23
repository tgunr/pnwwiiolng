#pragma once
// Created: Wed Mar 29 18:38:21 CST 2006
//
// Chat Grid types
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include <vector>
#include "host/hostPlayers.h"

class Player;
class ServerTrackedObject;

typedef std::set<class ServerSTO*> STOS;
typedef std::vector<ServerSTO*> STO_BATCH;

typedef std::vector<const PLAYERS*> PLAYERS_ARRAY;

class STOBatch
{
public:
	STOBatch() : m_removes(), m_adds(), m_sent(0) {}

	void add(const STOS& stos, bool removals)
	{
		if ( stos.empty() ) return;
		if ( removals ) { m_removes.insert(m_removes.end(), stos.begin(), stos.end()); }
		else { m_adds.insert(m_adds.end(), stos.begin(), stos.end()); }
	}
	void clear() { m_x = m_y = 0.0f; m_sent = 0; m_adds.clear(); m_removes.clear(); }
	void setCoords(FLOAT64 x, FLOAT64 y) { m_x = (SINT32)(x/512); m_y = (SINT32)(y / 512); }

	size_t size() const { return m_removes.size() + m_adds.size(); }
	bool completed() const { return m_sent >= size(); }

	ServerSTO* fetch()
	{
		if ( completed() ) return nullptr;
		if ( isRemoval() ) return m_removes[m_sent];
		else return m_adds[m_sent - m_removes.size()];
	}
	void next() { m_sent++; }
	bool isRemoval() const { return m_sent < m_removes.size(); }

	SINT32 send(SINT32 teulIndex);

public:
	STO_BATCH			m_removes, m_adds;
	UINT32				m_sent;
	FLOAT64				m_x, m_y;
};

void chatGridGetPlayersVector(const Player* player, const SINT64& range, PLAYERS_ARRAY& array);

