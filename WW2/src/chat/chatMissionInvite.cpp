#include <syscfg.h> // Path is project specific.
//
// Date: Tue Mar 13 15:34:08 CDT 2007
//
// CLIENT/HOST: Common chatMissionInvite functions.
//
// Copyright (C) PlayNet, Inc 1999-2007 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith
//


#include "teul/teulPackers.h"
#include "wwii/wwiiPack.h"
#include "chat/chatMissionInvite.h"

MissionInvite::MissionInvite(ACTION_OBJECT* const a)
{
	a->offset = packer(TEUL_UNPACK, a) ;
}

SINT32
MissionInvite::packer(LOGICAL mode, ACTION_OBJECT* a)
{
	a->offset = teulPackUINT32(mode, a->packed, a->offset, &m_missionID, 0) ;
	a->offset = wwiiPackCountry(mode, a->packed, a->offset, &m_countryID, 4) ;
	a->offset = teulPackUINT32(mode, a->packed, a->offset, (UINT32*)&m_branchID, 4) ;
	a->offset = wwiiPackBrigadeID(mode, a->packed, a->offset, m_brigadeID, 0) ;
	a->offset = wwiiPackHandle(mode, a->packed, a->offset, &m_fromHandle, 0) ;
	a->offset = wwiiPackHandle(mode, a->packed, a->offset, &m_squadHandle, 0) ;
	a->packer(mode, m_isDirective) ;

	return a->offset ;
}
