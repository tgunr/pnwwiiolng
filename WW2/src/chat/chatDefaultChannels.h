#pragma once
// Created: Tue Mar 24 11:15:31 CST 2009
//
// An array of the default channels. Only include once.
//
// Copyright (C) PlayNet, Inc 1999-2009 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith
//

#include "chat/chatDefines.h"
#include "chat/chatChannelTypes.h"

namespace Chat
{
	//! Channels that have no default will be set to NO_CHANNEL
	static const UINT32 defaultChannels[MAX_CLIENT_CHANNELS] =
	{
			MISSION_CHANNEL
	,		OBJECTIVE_CP_CHANNEL
	,		NO_CHANNEL
	,		SQUAD_CHANNEL
	,		SIDE_OPS_CHANNEL
	,		SIDE_HELP_CHANNEL
	} ;
}

