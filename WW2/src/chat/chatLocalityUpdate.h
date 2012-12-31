#pragma once
// Created: Sat Mar  3 16:00:51 CST 2007
//
// Classes for serializing player locality updates for cell->chat notices
//
// Copyright (C) PlayNet, Inc 1999-2007 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#pragma pack(push, 1)
struct LocalityUpdate
{
	FLOAT32		m_x, m_y, m_z ;		// Position in meters
	FULL_OBJ_ID	m_nearCP ;
	FULL_OBJ_ID	m_captureFac ;
	UINT32		m_playerID ;
	UINT32		m_riding ;
	UINT32		m_lastPing ;		// 0 = no ping
	bool		m_cpChanged : 1 ;
	bool		m_onAWS : 1 ;
	bool		m_inCaptureBldg : 1 ;
	SINT32		m_progress ;
} ;
#pragma pack(pop)

