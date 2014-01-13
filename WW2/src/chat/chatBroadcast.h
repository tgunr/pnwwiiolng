#pragma once
// Created: Fri Sep  2 14:11:42 CDT 2005
//
// Definition of the Broadcast type and enums
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "chat/chatLocalizedMessage.h"

//tolua_begin
namespace Chat
{
	enum BCAST_TYPE
	{
		_EVERYONE			// Everyone connected
	,	_ADMINISTRATIVE		// GM Channel
	,	_HCUNIT				// WWIIOL: Specific brigade
	,	_HCUNIT_DOWN		// WWIIOL: Unit and children
	,	_HCUNIT_UP			// WWIIOL: Unit and parents
	,	_HC					// WWIIOL: High Commands
	,	_MISSION			// WWIIOL: Specific mission
	,	_SIDE
	,	_ORDERS				// Mission orders
	,	_INSTANCE			// Everyone in an instance by instanceid_t
	,	_MAX_TYPES
	} ;
//tolua_end

#pragma pack(push, 1)
	class Broadcast : public Chat::LocalizedMessage
	{
	public:
		Broadcast() : LocalizedMessage((LSTRING_ID)-1, (LSTRING_ID)-1), m_type(_EVERYONE), m_instanceID(0ULL), m_targetID(0ULL) {}

	public:
		Broadcast(BCAST_TYPE typeID_, const instanceid_t& instanceID_, const UINT64 targetID_
					, LSTRING_ID prefixID_, LSTRING_ID messageID_
					, const char* const data0_="", const char* const data1_=""
					, const char* const data2_="", const char* const data3_=""
					, const char* const data4_="", const char* const data5_=""
					)
			: LocalizedMessage(prefixID_, messageID_, data0_, data1_, data2_, data3_, data4_, data5_)
			, m_type(typeID_)
			, m_instanceID(instanceID_)
			, m_targetID(targetID_)
		{
		}

		BCAST_TYPE			m_type ;
		instanceid_t		m_instanceID ;
		UINT64				m_targetID ;

		// Host Only:
		bool route() ;

		bool send() ;
	} ;
#pragma pack(pop)
//tolua_begin
}

// HOST: (Non-Chathost) Sends a targeted SYSTEM message to a specific target
bool chatSendBroadcast(Chat::BCAST_TYPE type, const instanceid_t& instanceID, UINT64 targetID, LSTRING_ID prefix, LSTRING_ID message
						, const char* ptr0="", const char* ptr1="", const char* ptr2="", const char* ptr3="", const char* ptr4="", const char* ptr5="") ;
//tolua_end

