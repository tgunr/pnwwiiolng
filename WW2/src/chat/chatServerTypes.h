#pragma once
// Created: Tue Jul 25 16:51:50 2000
//
// Types for the chat system
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

//Ok here's the deal: clients only send a message to their radio channels.
//	The host knows which channels the client is tuned to, so it sends the
//	message out on those channels.  Before it sends the message out to the
//	end user, it changes the radioChannels bitmask to match that of the user
//	that its sending the message to.
//
//	I'm sure that makes no sense, so read the design doc

class Player;
#include "chat/chatPlayerInfo.h"
#include "str/stringLib.h"	// for strsafecpy

#pragma pack(push, 1)
namespace Chat
{
	struct KickNotice
	{
		KickNotice(const playerid_t playerID_, const ChatHandle handle_, const char* const problem_)
			: m_playerID(playerID_)
			, m_handle(handle_)
			, m_null(0)
			{
				strsafecpy(m_problem, problem_);
			}
		KickNotice() : m_playerID(0), m_handle(0ULL), m_null(0) { m_problem[0] = 0; }

		playerid_t	m_playerID;
		ChatHandle	m_handle;
		char		m_problem[201];
		char		m_null;
	};

	struct PlayerMove
	{
		playerid_t	m_playerID;
		SINT32		m_x, m_y, m_z;
	};
}
#pragma pack(pop)

extern const char* const channelTypeName[];
extern const char* const chanTypeName[];

