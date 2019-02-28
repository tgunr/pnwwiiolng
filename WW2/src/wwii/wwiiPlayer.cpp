#include <syscfg.h> // Path is project specific.
// Created: Sun Oct 12 16:28:46 CDT 2008
//
// Members of the standardised host player class.
//
// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include <map>
#include "chat/chatLocalizedMessage.h"
#include "wwii/wwiiPlayer.h"
#include "wwii/wwiiExitCodes.h"
#include "wwii/wwiiPermissions.h"
#include "wwii/wwiiThreadTypes.h"
#include "wwii/wwiiTypes.h"				// For PEXIT_DEAD
#include "inst/instHost.h"
#include "str/stringLib.h"
#include "server/server.h"				// For serverReleasePlayer
#include "teul/teulBasic.h"				// For teulClose
#include "mem/mem.h"

// Callback registration for sending localized messages.
namespace Chat
{
	TEUL_CALLBACK*	_SEND_LOCALIZED_MESSAGE;
}

//////////////////////////////////////////////////////////////////////
// Copy operator.

wwiiPlayer&
wwiiPlayer::operator=(const wwiiPlayer* const rhs)
{
	m_hostInstance = nullptr;
	m_instance = Instance::g_invalid;

	m_teulIndex = -EINVAL;
	m_playerID = rhs->m_playerID;
	m_personaID = rhs->m_personaID;
	m_squadID = rhs->m_squadID;
	m_brigadeID = rhs->m_brigadeID;
	m_missionID = rhs->m_missionID;
	m_sortieID = rhs->m_sortieID;
	m_side = WWII_NO_SIDE;
	m_country = WWII_NO_COUNTRY;
	m_branch = rhs->m_branch;
	m_maxRank = rhs->m_maxRank;
	m_maxSquadRank = rhs->m_maxSquadRank;
	m_realRank = rhs->m_realRank;
	m_realSquadRank = rhs->m_realSquadRank;
	m_weaponSpecNo = rhs->m_weaponSpecNo;
	m_chatHandle = rhs->m_chatHandle;
	m_matchHandle = rhs->m_matchHandle;
	m_squadHandle = rhs->m_squadHandle;
	m_permissionMask = rhs->m_permissionMask;
	// Do not copy m_instance
	// Do not copy m_hostInstance
	m_flags = rhs->m_flags;
	m_dispatch = rhs->m_dispatch;

	SetCountry(rhs->m_country, rhs->m_side);
	SetInstance(rhs->m_instance);

	return *this;
}

//////////////////////////////////////////////////////////////////////
// Populate our dispatch from another

void
wwiiPlayer::SetDispatch(const Auth::Dispatch& rhs, UINT32 overridingCustomerID)
{
	m_dispatch.m_customerID = (overridingCustomerID) ? overridingCustomerID : rhs.m_customerID;
	m_dispatch.m_OTP = rhs.m_OTP;
	m_dispatch.m_clientIP = rhs.m_clientIP;
	m_dispatch.m_appVersion = rhs.m_appVersion;
	m_dispatch.m_platform = rhs.m_platform;
	strsafecpy(m_dispatch.m_callsign, rhs.m_callsign);
}

//////////////////////////////////////////////////////////////////////
// Log a message in the name and context of this player

void
wwiiPlayer::log(const char* msg) const
{
	dbugLog(_FLD, "t:%d p:%u c:%s in:%u.%u.%" PR64BitUT ".%" PR64BitUT " %s"
					, teulIndex(), playerID(), callsign()
					, instance().category(), instance().type(), instance().handle(), instance().id()
					, msg);
}

void
wwiiPlayer::log(const char* file, int lineno, unsigned int errType, const char* mask, ...) const
{
	static char buf[512];

	va_list args;
	va_start(args, mask);
	vsnprintf(buf, 511, mask, args);
	va_end(args);

	dbugLog(file, lineno, errType
				, "t:%d p:%u c:%s in:%u.%u.%" PR64BitUT ".%" PR64BitUT " %s"
				, teulIndex(), playerID(), callsign()
				, instance().category(), instance().type(), instance().handle(), instance().id()
				, buf);
}

//////////////////////////////////////////////////////////////////////
// Does this player effectively have GM privileges.

bool
wwiiPlayer::isEffectiveGM() const
{
#if defined(TRAINING_SERVER) || defined(BETA_RELEASE)
	return permissions(IS_PLAYNET | GAME_MANAGER | GAME_TRAINER);
#else
	return permissions(IS_PLAYNET | GAME_MANAGER);
#endif
}

//////////////////////////////////////////////////////////////////////
// Test if the player does not currently have a valid side.

bool
wwiiPlayer::noSide() const
{
	return false == isValidSide(side());
}

//////////////////////////////////////////////////////////////////////
// Set the player's callsign

void
wwiiPlayer::_setCallsign(const char* to, bool isPlayerPlaynet)
{
	m_matchHandle = chatGetLowercaseHandle(to);
	memset(m_dispatch.m_callsign, 0, sizeof(m_dispatch.m_callsign));
	chatGetCallsign(m_dispatch.m_callsign, m_matchHandle);
	if ( isPlayerPlaynet )
		strupr(m_dispatch.m_callsign);
	else
		strlwr(m_dispatch.m_callsign);
	m_chatHandle = chatGetHandle(m_dispatch.m_callsign);
}

//////////////////////////////////////////////////////////////////////
// Send a player to the exit server

void
wwiiPlayer::Release(SINT32 onTeulIndex, Server::ReleaseType releaseType)
{
	bool closeRequired = (onTeulIndex >= 0 && onTeulIndex == m_teulIndex);
	this->m_teulIndex = -1;
	serverReleasePlayer(onTeulIndex, this->playerID(), this->OTP(), PEXIT_DEAD, releaseType);

	if ( closeRequired )
		teulClose(onTeulIndex);
}

//////////////////////////////////////////////////////////////////////
// Return true if the persona is an acting high commander

bool
wwiiPlayer::isOfficer() const
{
	if ( side() == ALLIED )
		return permissions(ALLIED_COMMAND_STAFF);
	else if ( side() == AXIS )
		return permissions(AXIS_COMMAND_STAFF);
	return false;
}

