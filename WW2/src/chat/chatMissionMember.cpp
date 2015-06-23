#include <syscfg.h> // Path is project specific.
// Date: Tue Sep  6 20:21:53 CDT 2005
//
// Functions for the chatMissionMember.cpp class
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "teul/teulPackers.h"
#include "wwii/wwiiPack.h"
#include "chat/chatMissionMemberUpdates.h"

MissionMember::MissionMember(ACTION_OBJECT* const a)
{
	a->offset = packer(TEUL_UNPACK, a);
}

SINT32
MissionMember::packer(LOGICAL mode, ACTION_OBJECT* a)
{
	a->offset = wwiiPackHandle(mode, a->packed, a->offset, &m_handle, 0);
	a->offset = wwiiPackRankNo(mode, a->packed, a->offset, &m_rank, 0);

	// If rank is 0 then this is a delete notice
	if ( m_rank > 0 )
	{
		m_weapon.packer(mode, a);

		if ( mode == TEUL_UNPACK )
			m_x = m_y = INVALID_GRID_COORD;

		if ( m_weapon.wType != 0 )
		{
			UCHAR packPos = (mode == TEUL_PACK && m_x != INVALID_GRID_COORD) ? 1 : 0;
			a->offset = teulPackUCHAR(mode, a->packed, a->offset, &packPos, 1);
			if ( packPos > 0 )
			{
				a->offset = teulPackSINT32(mode, a->packed, a->offset, &m_y, 31);
				a->offset = teulPackSINT32(mode, a->packed, a->offset, &m_x, 32);
			}
		}
	}

	return a->offset;
}

