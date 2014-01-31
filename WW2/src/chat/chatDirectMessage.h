#pragma once
// Created: Sun Jan 13 19:58:47 CST 2008
//
// Inter-host structure for sending a message
// directly to a specific playerID
//
// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "chat/chatLocalizedMessage.h"

#pragma pack(push, 1)
namespace Chat
{
	// Direct message data structure
	struct DirectMessage : public Chat::LocalizedMessage
	{
		DirectMessage(playerid_t playerID=0)
			: m_playerID(playerID)
		{}

		DirectMessage(playerid_t playerID
						, int prefixID, int messageID
						, const char* const ptr0="", const char* const ptr1="", const char* const ptr2="", const char* const ptr3="", const char* const ptr4="", const char* const ptr5="")
			: LocalizedMessage(prefixID, messageID, ptr0, ptr1, ptr2, ptr3, ptr4, ptr5)
			, m_playerID(playerID)
		{
		}
		playerid_t		m_playerID;
	};
}
#pragma pack(pop)

