#include <syscfg.h> // Path is project specific.
//
// Created: Thu Dec  8 12:56:05 CST 2005
//
// Out Of Band messages and Strat Commands
// Common (chat/strat) portions go here
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith
//

#include "dbug/dbug.h"
#include <stdexcept>

#include "wwii/wwiiConfig.h"
#include "teul/teulPackers.h"
#include "strt/strtChatTypes.h"
#include "wwii/wwiiString.h"

OOBMessage
::OOBMessage(UINT32 playerID, UINT32 brigadeID, const ChatHandle& handleID, const char* command, const char* arguments
				, const FULL_OBJ_ID& cpID, WWII_SIDE side, WWII_COUNTRY country)
	: m_originatorID(playerID)
	, m_brigadeID(brigadeID)
	, m_handleID(handleID)
	, m_cpID(cpID)
	, m_country(country)
	, m_side(side)
{
	if ( arguments == NULL || !*arguments )
		strlcpy(m_text, command, sizeof(m_text));
	else
	{
		snprintf(m_text, OOB_STRING_LEN, "%s %s", command, arguments);
		m_text[sizeof(m_text)-1] = 0;
	}
}

OOBMessage::OOBMessage()
{
	memset(this, 0, sizeof(*this));
}

OOBMessage::OOBMessage(ACTION_OBJECT* const a)
{
	a->offset = packer(TEUL_UNPACK, a);
}

SINT32
OOBMessage::packer(LOGICAL mode, ACTION_OBJECT* a)
{
	a->offset = teulPackVoid(mode, a->packed, a->offset, this, sizeof(*this));
	return a->offset;
}

StratCommand::StratCommand(UINT32 issuer, SCMD_ENUM command, FULL_OBJ_ID cp, WWII_SIDE side, SCMD_RESULT result)
	: m_issuer(issuer), m_command(command), m_cp(cp), m_side(side), m_result(result), m_data(0)
{
	if ( command <= SC_NO_COMMAND || command >= SC_NUM_COMMANDS )
		throw std::domain_error("command number out of bounds");
}

StratCommand::StratCommand(ACTION_OBJECT* const a)
{
	a->offset = pack(TEUL_UNPACK, a);
}

SINT32
StratCommand::pack(LOGICAL mode, ACTION_OBJECT* a)
{
	a->offset = teulPackVoid(mode, a->packed, a->offset, this, sizeof(*this));
	return a->offset;
}

bool
StratCommand::describe() const
{
	if ( m_command <= SC_NO_COMMAND || m_command >= SC_NUM_COMMANDS )
	{
		dbugLog(_FLW, "p:%u Invalid scmd number (%d)"
						, m_issuer, m_command);
		return false;
	}

	// Report what we're receiving
	dbugLog(_FLI, "p:%u Strat command %d: target %d.%d.%u for side %u"
					, m_issuer, m_command
					, m_cp.cell.ox, m_cp.cell.oy, m_cp.id
					, m_side);
	return true;
}

