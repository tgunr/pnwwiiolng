#pragma once
// Date: Mon Apr  4 20:17:26 CDT 2005
//
// Defines a class for describing strat updates
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "teul/teulPackers.h"

class StratUpdates
{
public:
	enum MODE { CP, FACILITY, BUILDING };
public:
	FULL_OBJ_ID fullObjID() const { return m_fullObjID; }
	UINT32 flags() const { return m_flags; }
	WWII_COUNTRY owner() const { return (WWII_COUNTRY)m_owner; }
	WWII_COUNTRY controller() const { return (WWII_COUNTRY)u_extra.m_controller; }
	UINT32 captureStage() const { return (UINT32)u_extra.m_capture.m_stage; }
	UINT16 captureCounter() const { return u_extra.m_capture.m_counter; }
	UINT32 lastChange() const { return u_extra.m_capture.m_lastChange; }

	union ExtraFields
	{
		UINT64			m_controller : 48;
		UINT64			m_buildingDamagedBy : 48;
		struct
		{
			UINT32	m_stage : 4;
			UINT32	m_lastChange : 28;
			UINT16	m_counter;
		} m_capture;
	} u_extra;
	FULL_OBJ_ID		m_fullObjID;
	UINT32			m_flags;
	UCHAR			m_owner;
};
