#include <syscfg.h> // Path is project specific.
// Created: Wed Apr 27 13:51:09 CDT 2005
//
// Copyright (C) PlayNet, Inc. 2005 - All rights reserved.
// Not for redistribution without permission as part of an API.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "miss/missionDetail.h"
#include "wwii/wwiiPack.h"
#include "teul/teulTime.h"

SINT32
MissionDetail::packer(LOGICAL mode, ACTION_OBJECT* a)
{
	a->offset = teulPackUINT32(mode, a->packed, a->offset, (UINT32*)&m_result, 8) ;
	if ( m_result < MC_ERROR )
	{
		a->offset = MissionSynopsis::packer(mode, a) ;
		UINT32 delta = 0 ;
		if ( mode == TEUL_PACK )
			delta = ( m_timeLaunch > (UINT32)teulTime() ) ? m_timeLaunch + 1 - teulTime() : 0 ;
		a->offset = teulPackUINT32(mode, a->packed, a->offset, &delta, 16) ;
		if ( mode == TEUL_UNPACK )
			m_timeLaunch = delta + teulTime() ;
		a->offset = wwiiPackText(mode, a->packed, a->offset, m_desc, MapMiss::MISSION_DESC_LENGTH) ;
		a->offset = wwiiPackBrigadeID(mode, a->packed, a->offset, m_brigadeID) ;
	}

	return a->offset ;
}

UINT32
MissionDetail::launchDelay() const
{
	UINT32 now = (UINT32)teulTime() ;
	return ( m_timeLaunch > now ) ? m_timeLaunch - now : 0 ;
}

