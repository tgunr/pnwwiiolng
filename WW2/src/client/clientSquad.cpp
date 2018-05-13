#include "syscfg.h" // Path is project specific.
// Date: Mon Sep 12 15:51:40 CDT 2005
//
// Routines for wwiiSquad class
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "client/clientSquad.h"
#include "client/GameClient.h"
#include "map/mapPersonaSummary.h"

//////////////////////////////////////////////////////////////////////
// Default constructor

ClientSquad::ClientSquad() : Squad(), m_modified(true)
{
}

ClientSquad::ClientSquad(const Squad& s) : Squad(s), m_modified(true)
{
}

void ClientSquad::addMember(const Chat::Who& who)
{
	m_modified = true;
	m_members[chatGetLowercaseHandle(who.handle())] = who;
}

void ClientSquad::addMembers(const SquadMembers& members)
{
	SquadMembers::const_iterator it = members.begin();

	for(; it != members.end(); ++it)
		addMember(it->second);
}

void ClientSquad::removeMember(const Chat::Who& who)
{
	m_modified = true;
	m_members.erase(chatGetLowercaseHandle(who.handle()));
}

bool ClientSquad::isMember(const ChatHandle handle) const
{
	SquadMembers::const_iterator found = m_members.find(chatGetLowercaseHandle(handle));

	return (found != m_members.end()) ? true: false;
}

bool ClientSquad::isSquadBrigade(const UINT32 brigadeID, bool excludeMe) const
{
	SquadMembers::const_iterator it = m_members.begin();
	ChatHandle me = (excludeMe) ? chatGetLowercaseHandle(PlayerClient()->Personas().m_callsign): 0;

	for(; it != m_members.end(); ++it)
	{
		if(it->second.hcUnitID() == brigadeID)
		{
			if(!excludeMe || it->first != me)
				return true;
		}
	}

	return false;
}

bool ClientSquad::isSquadMission(const UINT32 missionID, bool excludeMe) const
{
	SquadMembers::const_iterator it = m_members.begin();
	ChatHandle me = (excludeMe) ? chatGetLowercaseHandle(PlayerClient()->Personas().m_callsign): 0;

	for(; it != m_members.end(); ++it)
	{
		if(it->second.missionID() == missionID)
		{
			if(!excludeMe || it->first != me)
				return true;
		}
	}

	return false;
}

const SquadMembers& ClientSquad::members(bool reset) const
{
	if(reset)
		m_modified = false;

	return m_members;
}
