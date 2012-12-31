#pragma once
// Created: Sun Jan 13 15:58:07 CST 2008
//
// Chat::SideMessage structure and defines
//
// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "chat/chatDefines.h"
#include "chat/chatLStringIDs.h"

namespace Chat
{
	struct SideMessage
	{
		SideMessage()
			: m_type(SM_UNDEFINED), m_side(WWII_NO_SIDE), m_objectiveID(nullObjID)
			, m_prefix(LSTR_NONE), m_message(LSTR_NONE), m_arg1(""), m_arg2("")
			{}
		SideMessage(ACTION_OBJECT* const a) ;
		~SideMessage() {}

		SINT32 packer(LOGICAL mode, ACTION_OBJECT* a) ;

		SIDE_MESSAGE_TYPE	m_type ;		// Type of message - side, hc, etc...
		WWII_SIDE			m_side ;		// 0 or side to restrict message to
		FULL_OBJ_ID			m_objectiveID ;	// nullObjID or CP to restrict to
		LSTRING_ID			m_prefix ;		// Prefix LString
		LSTRING_ID			m_message ;		// Message's LString
		std::string			m_arg1, m_arg2 ;	// 2 arguments
	} ;
}
