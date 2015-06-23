#pragma once
// Copyright (C) PlayNet Inc 2012 -- All rights reserved.
// Created: Sat Apr 21 15:18:37 CDT 2012
// Originating Programmer: Oliver "kfsone" Smith

//! @struct AntiHack
//! @brief Fields relating to anti-cheat systems for the cell host.

class AntiHack
{
public:
	AntiHack()
		: m_lastSpeedhackDelta(0)
		, m_lastSpeedhackBlip(0)
		, m_numBlips(0)
		{}

public:
	TeulDeltaMS		m_lastSpeedhackDelta;
	UINT32			m_lastSpeedhackBlip;
	UINT32			m_numBlips;
};
