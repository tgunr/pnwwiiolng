#include <syscfg.h> // Path is project specific.
// Date: Thu Oct 13 20:33:57 CDT 2005
//
// Host MOTDs
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "chat/chatHandle.h"
#include "chat/chatHostMotds.h"
#include "chat/chatMotds.h"
#include "chat/chatName.h"
#include "teul/teulTime.h"

static MOTD_REPLY _motds[MOTD::_MAX_TYPES];

//////////////////////////////////////////////////////////////////////
// Accessor to get a particular MOTD

MOTD_REPLY&
chatGetMotd(UINT32 type)
{
	if ( type <= 0 || type >= MOTD::_MAX_TYPES )
		return _motds[0];
	return _motds[type];
}

//////////////////////////////////////////////////////////////////////
// Update an MOTD

bool
chatSetMotd(UINT32 type, const ChatHandle submitter, const std::string& text)
{
	if ( type >= MOTD::_MAX_TYPES )
	{
		dbugLog(_FLA, "c:%s attempting to update motd %u", Chat::Name(submitter).c_str(), type);
		return false;
	}

	MOTD_REPLY& motd = _motds[type];

	motd.poster = submitter;
	motd.message = text.substr(0, MOTD_REPLY::_MAX_LENGTH);

	// We have to always increment the motd time
	if ( motd.added >= (UINT32)teulTime() )
		++motd.added;
	else
		motd.added = teulTime();

	dbugLog(_FLI, "c:%s updated motd:%u to %s", Chat::Name(submitter).c_str(), type, motd.message.c_str());

	return true;
}

