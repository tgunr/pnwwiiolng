#include <syscfg.h> // Path is project specific.
// Created: Tue Jan  5 11:34:44 CST 2010
//
// Implements management-client specific player functions.
//
// Copyright (C) PlayNet, Inc 1999-2010 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "chat/chatGroupPermissions.h"
#include "chat/chatLStringIDs.h"
#include "chat/chatManagementIDs.h"
#include "chat/chatManagementPlayer.h"

//	------------------------------	Declarations

//////////////////////////////////////////////////////////////////////
// ManagementPlayer constructor

ManagementPlayer::ManagementPlayer(SINT32 newIndex, const char* withGameName)
	: Player(Chat::ManagementIDAllocator::AllocateID(this), true)		// May throw runtime_error
{
	m_teulIndex = newIndex;

	_setCallsign(withGameName, true);

	SetCountry(WWII_NO_COUNTRY, WWII_NO_SIDE);

	m_gmNote = true;
	m_gmHide = true;
	m_playerMask = 0xffffffffffffffffLL;
	m_permissionMask = m_playerMask;

	m_chatPermissions = &nullChatPermissions;

	m_initialized = true;
}

//////////////////////////////////////////////////////////////////////
// Destructor: Free up the id.

ManagementPlayer::~ManagementPlayer()
{
	Chat::ManagementIDAllocator::ReleaseID(playerID());
}

//////////////////////////////////////////////////////////////////////
// Describe me to a GM

void
ManagementPlayer::describeTo(const Player& player, const FLOAT64& /*x*/, const FLOAT64& /*y*/, bool /*includePermissions*/) const
{
	player.message(LSTR_NONE, LSTR_LABELLED_GENERIC, callsign(), "User is connected via management client.");
}
