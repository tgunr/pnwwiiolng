#include <syscfg.h> // Path is project specific.
// Date: Sun Sep 25 16:43:16 CDT 2005
//
// Structures for tracking reports
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith


#include "chat/chatReports.h"
#include "teul/teulPackers.h"
#include "weap/weapSpec.h"
#include "wwii/wwiiPack.h"
#include "wwii/wwiiPackString.h"

ChatReport::ChatReport(UINT16 category, const std::string& report, const ChatHandle& offender)
	: m_hostID(0)
	, m_time(0)
	, m_category(category)
	, m_status(0)
	, m_poster(0LL)
	, m_offender(offender)
	, m_assigned(0LL)
	, m_country(0)
	, m_branch(0)
	, m_brigade(0)
	, m_mission(0)
	, m_leader(false)
	, m_weapon(NULL_WEAP_SPEC)
	, m_sortie(0)
	, m_spawned(false)
	, m_x(0x7fffffff), m_y(0x7fffffff), m_z(0x7fffffff)
	, m_report(report)
	, m_notes("")
{
}

SINT32
ChatReport::packer(PackMode mode, ACTION_OBJECT* a)
{
	a->offset = teulPackUINT16(mode, a->packed, a->offset, &m_status, 4);
	a->offset = teulPackUINT16(mode, a->packed, a->offset, &m_category, 4);
	wwiiPackString(mode, a->packed, a->offset, m_report, MAX_TEXT_LENGTH);

	if ( m_category == PLAYER )
		a->offset = wwiiPackHandle(mode, a->packed, a->offset, &m_offender, 0);

	if ( m_status > SUBMISSION )
	{
		a->offset = wwiiPackHandle(mode, a->packed, a->offset, &m_poster, 0);
		a->offset = teulPackUINT32(mode, a->packed, a->offset, &m_hostID, 0);
		a->offset = teulPackUINT32(mode, a->packed, a->offset, &m_time, 0);
		a->offset = teulPackUINT16(mode, a->packed, a->offset, &m_country, 4);
		a->offset = teulPackUINT16(mode, a->packed, a->offset, &m_branch, 4);
		a->offset = wwiiPackBrigadeID(mode, a->packed, a->offset, m_brigade, 0);

		if ( m_brigade > 0 )
		{
			a->offset = teulPackUINT32(mode, a->packed, a->offset, &m_mission, 31);
			a->packer(mode, m_leader);
			m_weapon.packer(mode, a);
			a->packer(mode, m_sortie);
			a->packer(mode, m_x);
			a->packer(mode, m_y);
			a->packer(mode, m_z);
		}

		if ( m_status > NEW )
		{
			wwiiPackString(mode, a->packed, a->offset, m_notes, MAX_TEXT_LENGTH);
			a->offset = wwiiPackHandle(mode, a->packed, a->offset, &m_assigned, 0);
		}
	}

	return a->offset;
}

