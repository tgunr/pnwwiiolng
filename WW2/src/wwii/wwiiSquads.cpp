#include <syscfg.h> // Path is project specific.
// Created: Mon Sep 12 15:51:40 CDT 2005
//
// Routines for wwiiSquad class
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "wwii/wwiiSquads.h"
#include "teul/teulPackers.h"
#include "wwii/wwiiPack.h"
#include "wwii/wwiiPackString.h"
#include "chat/chatName.h"

//////////////////////////////////////////////////////////////////////
// Default constructor

Squad::Squad(UINT32 withSquadID, const ChatHandle handle, const std::string& withName)
	: m_squadID(withSquadID)
	, m_shortHandle(handle)
	, m_creatorHandle(0LL)
	, m_coHandle(0LL)
	, m_lastJoiner(0LL)
	, m_flags(0)
	, m_created(0)
	, m_lastJoined(0)
	, m_rookieDays(DEFAULT_ROOKIE_DAYS)
	, m_squadNight(0)
	, m_squadHour(0)
	, m_language(0) // english
	, m_squadTimezone(0)
	, m_votingEnabled(true)
	, m_recruiting(false)
	, m_name(withName)
	, m_description("")
	, m_motd("")
	, m_homepage("")
	, m_callsign(Chat::Name(handle))
	, m_voiceInfo("")
	, m_slogan("")
{
	if ( m_name.length() > NAME_LENGTH )
		m_name.erase(NAME_LENGTH);
	if ( m_description.length() > DESCRIPTION_LENGTH )
		m_description.erase(DESCRIPTION_LENGTH);
	if ( m_motd.length() > MOTD_LENGTH )
		m_motd.erase(MOTD_LENGTH);
	if ( m_homepage.length() > HOMEPAGE_LENGTH )
		m_homepage.erase(HOMEPAGE_LENGTH);
	if ( m_callsign.length() > (size_t)MAX_HANDLE_LEN )
		m_callsign.erase(MAX_HANDLE_LEN);
}

//////////////////////////////////////////////////////////////////////
// Packer

SINT32
Squad::packer(LOGICAL mode, ACTION_OBJECT* a)
{
	a->offset = teulPackUINT32(mode, a->packed, a->offset, &m_squadID, 0);
	a->offset = wwiiPackHandle(mode, a->packed, a->offset, &m_shortHandle, 0);
	a->offset = wwiiPackHandle(mode, a->packed, a->offset, &m_creatorHandle, 0);
	a->offset = wwiiPackHandle(mode, a->packed, a->offset, &m_coHandle, 0);
	// Flags doesn't get sent
	a->packer(mode, m_votingEnabled);
	a->packer(mode, m_recruiting);
	a->offset = teulPackUINT32(mode, a->packed, a->offset, &m_created, 0);
	a->offset = teulPackUINT32(mode, a->packed, a->offset, &m_rookieDays, 8);
	a->offset = wwiiPackString(mode, a->packed, a->offset, m_name, NAME_LENGTH);
	a->offset = wwiiPackString(mode, a->packed, a->offset, m_description, DESCRIPTION_LENGTH);
	a->offset = wwiiPackString(mode, a->packed, a->offset, m_motd, MOTD_LENGTH);
	a->offset = wwiiPackString(mode, a->packed, a->offset, m_homepage, HOMEPAGE_LENGTH);
	a->offset = wwiiPackHandle(mode, a->packed, a->offset, &m_lastJoiner, 0);
	a->offset = teulPackUINT32(mode, a->packed, a->offset, &m_lastJoined, 0);
	a->offset = teulPackUCHAR(mode, a->packed, a->offset, &m_squadNight, 0);
	a->offset = teulPackUCHAR(mode, a->packed, a->offset, &m_squadHour, 0);
	a->offset = teulPackSCHAR(mode, a->packed, a->offset, &m_squadTimezone, 0);
	a->offset = teulPackUCHAR(mode, a->packed, a->offset, &m_language, 0);
	a->offset = wwiiPackString(mode, a->packed, a->offset, m_slogan, SLOGAN_LENGTH);

	m_callsign = Chat::Name(m_shortHandle).c_str();

	return a->offset;
}

bool
Squad::setSquadNight(const char* string)
{
	UINT32 night = 0, hour = 0;
	SINT32 tz = 0;

	if ( sscanf(string, "%u,%u,%d", &night, &hour, &tz) != 3 )
		return false;

	bool changed = false;

	if ( m_squadNight != night )
	{
		m_squadNight = night;
		changed = true;
	}

	if ( m_squadHour != hour )
	{
		m_squadHour = hour;
		changed = true;
	}

	if ( m_squadTimezone != tz )
	{
		m_squadTimezone = tz;
		changed = true;
	}

	return changed;
}

//////////////////////////////////////////////////////////////////////
// Default constructor

Recruiter::Recruiter(const ChatHandle withHandle, const Squad* squad)
	: m_handle(withHandle)
	, m_squadHandle(0LL)
	, m_country(0)
	, m_branch(0)
	, m_language(0) // english
	, m_mission(0)
	, m_activity(1) // at least the recruiter is on the mission
	, m_squadName("")
	, m_slogan("")
	, m_origin(nullObjID)
	, m_target(nullObjID)
{
	if(squad)
	{
		m_squadHandle	= squad->shortHandle();
		m_squadName = squad->name();
		m_slogan	= squad->slogan();
		m_language	= squad->language();
	}
}

//////////////////////////////////////////////////////////////////////
// Packer

SINT32
Recruiter::packer(LOGICAL mode, ACTION_OBJECT* a)
{
	a->offset = wwiiPackHandle(mode, a->packed, a->offset, &m_handle, 0);
	a->offset = wwiiPackHandle(mode, a->packed, a->offset, &m_squadHandle, 0);
	a->offset = teulPackUCHAR(mode, a->packed, a->offset, &m_country, 0);
	a->offset = teulPackUCHAR(mode, a->packed, a->offset, &m_branch, 0);
	a->offset = teulPackUCHAR(mode, a->packed, a->offset, &m_language, 0);
	a->offset = teulPackUCHAR(mode, a->packed, a->offset, &m_mission, 0);
	a->offset = teulPackUCHAR(mode, a->packed, a->offset, &m_activity, 0);
	a->offset = wwiiPackString(mode, a->packed, a->offset, m_squadName, Squad::NAME_LENGTH);
	a->offset = wwiiPackString(mode, a->packed, a->offset, m_slogan, Squad::SLOGAN_LENGTH);
	a->offset = wwiiPackFullObjID(mode, a->packed, a->offset, &m_origin, 0);

	return a->offset;
}
