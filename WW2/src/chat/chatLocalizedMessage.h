#pragma once
// Created: Sun Jan 13 19:27:56 CST 2008
//
// Structure for communicating localized messages host->client
//
// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "chat/chatDefines.h"			// For MAX_MESSAGE_LENGTH
#include "chat/chatLStringIDs.h"		// For LSTRING_ID

namespace Chat
{
	extern TEUL_CALLBACK* _SEND_LOCALIZED_MESSAGE ;

	// Localized messages
	struct LocalizedMessage
	{
		// Maximum number of arguments that come with a LoalizedMessage.
		static const UINT32 _NUM_ARGUMENTS = 6 ;

private:
		void _copy(const char* const p0, const char* const p1, const char* const p2, const char* const p3, const char* const p4, const char* const p5) ;
public:
#pragma pack(push, 1)
		LocalizedMessage()
			: m_prefix(LSTR_NONE), m_message(LSTR_NONE)
			, m_size(0)
		{
			_copy("", "", "", "", "", "") ;
		}

		LocalizedMessage(const LocalizedMessage& rhs)
			: m_prefix(rhs.m_prefix), m_message(rhs.m_message)
			, m_size(rhs.m_size)
		{
			memcpy(m_data, rhs.m_data, rhs.m_size) ;
			for ( size_t i = 0 ; i < _NUM_ARGUMENTS ; ++i )
			{
				m_ptr[i] = rhs.m_ptr[i] - rhs.m_data + this->m_data ;
			}
		}

		LocalizedMessage(int prefixID, int messageID
						, const char* const ptr0="", const char* const ptr1="", const char* const ptr2="", const char* const ptr3="", const char* const ptr4="", const char* const ptr5="")
			: m_prefix((LSTRING_ID)prefixID)
			, m_message((LSTRING_ID)messageID)
			, m_size(0)
		{
			_copy(ptr0 ? ptr0 : "", ptr1 ? ptr1 : "", ptr2 ? ptr2 : "", ptr3 ? ptr3 : "", ptr4 ? ptr4 : "", ptr5 ? ptr5 : "") ;
		}

		LSTRING_ID		m_prefix ;
		LSTRING_ID		m_message ;
		char*			m_ptr[_NUM_ARGUMENTS] ;
		char			m_data[MAX_MESSAGE_LENGTH + 2] ;	// Zero-terminated text strings
		UINT16			m_size ;							// Size including null bytes

		// Member function for packing
		SINT32 packer(LOGICAL mode, ACTION_OBJECT* a) ;

		void Populate(int prefixID, int messageID
						, const char* const ptr0="", const char* const ptr1="", const char* const ptr2="", const char* const ptr3="", const char* const ptr4="", const char* const ptr5="")
		{
			m_prefix = (LSTRING_ID)prefixID ;
			m_message = (LSTRING_ID)messageID ;
			_copy(ptr0 ? ptr0 : "", ptr1 ? ptr1 : "", ptr2 ? ptr2 : "", ptr3 ? ptr3 : "", ptr4 ? ptr4 : "", ptr5 ? ptr5 : "") ;
		}

		void SetPointers() ;
	} ;
#pragma pack(pop)
}
