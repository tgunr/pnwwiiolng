#include <syscfg.h> // Path is project specific.
// Created: Tue May 31 07:50:51 CDT 2005
//
// Provides code to implement the active mission class
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "wwii/wwiiPack.h"
#include "miss/missActiveMission.h"

//////////////////////////////////////////////////////////////////////
// Packer/unpacker

SINT32
ActiveMission::packer(PackMode mode, ACTION_OBJECT* a)
{
	a->offset = teulPackUINT32(mode, a->packed, a->offset, &m_missionID, 0);
	if ( m_missionID > 0 )
	{
		a->offset = wwiiPackCountry(mode, a->packed, a->offset, &m_countryID, 0);
		a->offset = teulPackUINT32(mode, a->packed, a->offset, (UINT32*)&m_branchID, 3);
		a->offset = wwiiPackBrigadeID(mode, a->packed, a->offset, m_brigadeID);
	}

	return a->offset;
}

