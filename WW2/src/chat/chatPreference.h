#pragma once
// Created: Tue Mar 20 10:43:42 CDT 2007
//
// Send one or more client preferences to the chat host.
//
// Copyright (C) PlayNet, Inc 1999-2007 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "chat/chatPreferenceEnum.h"

namespace Chat
{

#pragma pack(push, 1)
struct Preference
{
	Preference(CHAT_PREFERENCE prefID, UINT32 value) : m_value(value), m_prefID(prefID) {}
	Preference() : m_value(0), m_prefID(CPR_INVALID) {}
	Preference(const Preference& rhs) : m_value(rhs.m_value), m_prefID(rhs.m_prefID) {}

	UINT32			m_value;
	CHAT_PREFERENCE	m_prefID;
};
#pragma pack(pop)

}
