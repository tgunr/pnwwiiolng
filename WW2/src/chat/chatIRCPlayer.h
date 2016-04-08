#pragma once
// Created: Tue Jul 31 14:58:24 CDT 2007
//
// Player classes extended to handle IRC
//
// Copyright (C) PlayNet, Inc 1999-2007 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "chat/chatManagementPlayer.h"
#include "chat/chatLocalizedMessage.h"

class IRCPlayer : public ManagementPlayer
{
public:
	typedef std::set<std::string> IRC_CHANNELS;

public:
	IRCPlayer(const char* nick);
	virtual ~IRCPlayer();

	virtual bool message(int prefix, int msgid
						, const char* const ptr0="", const char* const ptr1="", const char* const ptr2=""
						, const char* const ptr3="", const char* const ptr4="", const char* const ptr5=""
						) const override;
	virtual bool message(const Chat::LocalizedMessage& lmsg) const override
	{
		return message(lmsg.m_prefix, lmsg.m_message, lmsg.m_ptr[0], lmsg.m_ptr[1], lmsg.m_ptr[2], lmsg.m_ptr[3], lmsg.m_ptr[4], lmsg.m_ptr[5]);
	}

	// Describe to another GM
	virtual void describeTo(const Player& player, const FLOAT64& x, const FLOAT64& y, bool includePermissions) const override;

	void SetLastChannel(const char* to) { m_lastChannel = to; }
	void AddChannel(const char* channel);
	void SetChannels(const IRC_CHANNELS& rhs) { m_channels = rhs; }
	UINT32 RemoveChannel(const char* channel);	 // Returns number of channels remaining.

	const IRC_CHANNELS& channels() const { return m_channels; }

	void ClearBuffer() { m_buffer = ""; }
	void AddLineToBuffer(const char* line) { m_buffer += line; }
	const char* GetBuffer() const { return m_buffer.c_str(); }

private:
	std::string	m_lastChannel;
	IRC_CHANNELS m_channels;
	std::string m_buffer;
};
