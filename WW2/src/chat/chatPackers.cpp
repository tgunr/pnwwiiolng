#include <syscfg.h> // Path is project specific.
// Date: Thu Jul 27 16:51:21 2000
//
// The actual pack routines for the chat system
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

#include "chat/chatClientMessage.h"
#include "chat/chatHandle.h"
#include "chat/chatHostMessage.h"
#include "chat/chatLocality.h"
#include "chat/chatLocalizedMessage.h"
#include "chat/chatServerInfo.h"
#include "chat/chatSTOJoules.h"
#include "chat/chatTuneToChannel.h"
#include "chat/chatUpdateChannel.h"
#include "chat/chatUserNLA.h"
#include "chat/chatTypes.h"
#include "chat/chatWaypointExchg.h"
#include "str/stringLib.h"
#include "strt/strtDTL.h"			// For MAX_STRT_NAME_LENGTH
#include "teul/teulPackers.h"
#include "teul/teulTime.h"
#include "weap/weapDefs.h"
#include "wwii/wwiiPack.h"
#include "wwii/wwiiPackString.h"

#if platBuild == BUILD_SERVER
# include "chat/chatSideMessage.h"
#endif

//////////////////////////////////////////////////////////////////////

#if platBuild == BUILD_SERVER
#include "chat/chatPlayerInfo.h"
SINT32
Chat::PlayerInfo::packer(PackMode mode, ACTION_OBJECT* const a)
{
	a->bytePacker(mode, a->localRawData, sizeof(*this));
	return a->offset;
}
#endif

SINT32
Chat::ClientMessage::packer(PackMode mode, ACTION_OBJECT* a)
{
	//Make sure our length isn't greater than it should be
	if ( mode == TEUL_PACK && m_length > (MAX_MESSAGE_LENGTH - 1))
		m_length = MAX_MESSAGE_LENGTH - 1;
	a->offset = teulPackUINT32(mode, a->packed, a->offset, &m_length, MAX_MESSAGE_LENGTH_BITS);
	if ( mode == TEUL_UNPACK && m_length > (MAX_MESSAGE_LENGTH - 1))
		m_length = MAX_MESSAGE_LENGTH - 1;

	a->offset = teulPackUINT16(mode, a->packed, a->offset, &m_radios, 8);

	a->offset = teulPackVoid(mode, a->packed, a->offset, &m_message, m_length);
	m_message[m_length] = 0;

	a->offset = wwiiPackFullObjID(mode, a->packed, a->offset, &m_uiTarget, 0);

	return a->offset;
}

SINT32
Chat::HostMessage::packer(PackMode mode, ACTION_OBJECT* a)
{
	// User must initialize the "msg" pointer before calling.
	if ( PN_UNLIKELY(msg == nullptr) )
		return -EINVAL;

	// Pack these two together to align to an 8 bit boundary
	a->offset = teulPackUINT32(mode, a->packed, a->offset, &length, MAX_MESSAGE_LENGTH_BITS);
	a->offset = teulPackUCHAR(mode, a->packed, a->offset, &icon, 6);

	//Packing the radioTypes it was sent on.
	for ( UINT32 i = 0; i<MAX_CLIENT_CHANNELS; i++ )
		a->offset = teulPackUCHAR(mode, a->packed, a->offset, &radioType[i], 6);

	a->offset = wwiiPackHandle(mode, a->packed, a->offset, &handle, 0);

	//Make sure our length isn't greater than it should be
	if ( mode == TEUL_UNPACK && length > MAX_MESSAGE_LENGTH - 1 )
		length = MAX_MESSAGE_LENGTH - 1;

	a->offset = teulPackVoid(mode, a->packed, a->offset, const_cast<char*>(msg), length+1);

	return a->offset;
}

SINT32
Chat::TuneToChannel::packer(PackMode mode, ACTION_OBJECT* a)
{
	a->offset = teulPackUCHAR(mode, a->packed, a->offset, &m_radioIndex, 4);
	a->offset = teulPackUINT32(mode, a->packed, a->offset, (UINT32*)&m_type, 12);
	a->offset = wwiiPackHandle(mode, a->packed, a->offset, &m_handle, 0);
	a->packer(mode, m_rx);
	a->packer(mode, m_tx);

	return a->offset;
}

SINT32
Chat::UpdateChannel::packer(PackMode mode, ACTION_OBJECT* a)
{
	a->offset = teulPackUCHAR(mode, a->packed, a->offset, &radioIndex, 7);
	a->packer(mode, receiveStatus);

	return a->offset;
}

SINT32
Chat::UserNLA::packer(PackMode mode, ACTION_OBJECT* a)
{
	a->offset = wwiiPackHandle(mode, a->packed, a->offset, &handle, 0);
	a->offset = teulPackUCHAR(mode, a->packed, a->offset, &radioIndex, 0);
	return a->offset;
}

SINT32
ChatServerInfo::packer(PackMode mode, ACTION_OBJECT* a)
{
	a->offset = teulPackUINT32(mode, a->packed, a->offset, &m_otp, 0);
	a->offset = teulPackUINT32(mode, a->packed, a->offset, &m_ipAddress, 0);
	a->offset = teulPackUINT16(mode, a->packed, a->offset, &m_port, 0);

	return a->offset;
}

SINT32
Chat::LocalizedMessage::packer(PackMode mode, ACTION_OBJECT* const a)
{
	a->packer(mode, (SINT32*)&m_prefix, 12);
	a->packer(mode, (SINT32*)&m_message, 12);
	a->packer(mode, &m_size, 16);
	a->bytePacker(mode, m_data, m_size);
	if ( mode == TEUL_UNPACK )
		SetPointers();

	return a->offset;
}

void
Chat::LocalizedMessage::_copy(const char* const p0, const char* const p1, const char* const p2, const char* const p3, const char* const p4, const char* const p5)
{
	if ( (p0[0] | p1[0] | p2[0] | p3[0] | p4[0] | p5[0]) != 0 )
	{
		m_size = strprintf(m_data
							, "%s%c%s%c%s%c%s%c%s%c%s"
							, p0,0 , p1,0 , p2,0 , p3,0 , p4,0 , p5
							);
		if ( m_size > MAX_MESSAGE_LENGTH )
			m_size = MAX_MESSAGE_LENGTH - 1;
	}
	else
	{
		m_size = 0;
	}
	m_data[m_size++] = 0;
	m_data[m_size] = 0;

	SetPointers();
}

void
Chat::LocalizedMessage::SetPointers()
{
	m_ptr[0] = m_data;
	char* ptr = m_data;
	char* const end = (m_size <= 0 ? m_data : m_data + m_size - 1);
	size_t position = 1;
	do
	{
		while ( ptr < end && *(ptr++) != 0 )
			;
		m_ptr[position++] = ptr;
	}
	while ( position < _NUM_ARGUMENTS );
}


//////////////////////////////////////////////////////////////////////
// Network constructor for a Locality object

Chat::Locality::Locality(ACTION_OBJECT* const a)
	: m_x(0)
	, m_y(0)
	, m_z(0)
	, m_nearCP(nullObjID)
{
	a->offset = packer(TEUL_UNPACK, a);
}

// Packer for the locality class.
SINT32
Chat::Locality::packer(PackMode mode, ACTION_OBJECT* a)
{
	a->offset = teulPackFLOAT64(mode, a->packed, a->offset, &m_x, 0);
	a->offset = teulPackFLOAT64(mode, a->packed, a->offset, &m_z, 0);
	a->offset = teulPackFLOAT64(mode, a->packed, a->offset, &m_y, 0);
	a->offset = wwiiPackFullObjID(mode, a->packed, a->offset, &m_nearCP, 0);
	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Network constructor for a Chat::SideMessage

#if platBuild == BUILD_SERVER
Chat::SideMessage::SideMessage(ACTION_OBJECT* const a)
	: m_type(static_cast<SIDE_MESSAGE_TYPE>(0))
	, m_side(WWII_NO_SIDE)
	, m_objectiveID(nullObjID)
	, m_prefix(LSTR_NONE)
	, m_message(LSTR_NONE)
	, m_arg1()
	, m_arg2()
{
	a->offset = packer(TEUL_UNPACK, a);
}

SINT32
Chat::SideMessage::packer(PackMode mode, ACTION_OBJECT* a)
{
	a->offset = teulPackUINT32(mode, a->packed, a->offset, (UINT32*)&m_type, 4);
	a->offset = teulPackUINT32(mode, a->packed, a->offset, (UINT32*)&m_side, 4);

	a->offset = wwiiPackFullObjID(mode, a->packed, a->offset, &m_objectiveID, 0);

	a->offset = teulPackUINT32(mode, a->packed, a->offset, (UINT32*)&m_prefix, 12);
	a->offset = teulPackUINT32(mode, a->packed, a->offset, (UINT32*)&m_message, 12);

	a->offset = wwiiPackString(mode, a->packed, a->offset, m_arg1, MAX_MESSAGE_LENGTH);
	if ( m_arg1.length() > 0 )
		a->offset = wwiiPackString(mode, a->packed, a->offset, m_arg2, MAX_MESSAGE_LENGTH);
	else if ( mode == TEUL_UNPACK )
		m_arg2 = "";

	return a->offset;
}
#endif

SINT32
Waypoint::packer(PackMode mode, ACTION_OBJECT* a)
{
	a->offset = wwiiPackHandle(mode, a->packed, a->offset, &m_handle, MAX_WP_HANDLE_BITS);
	a->offset = wwiiPackHandle(mode, a->packed, a->offset, &m_playerHandle, 0);
	a->offset = teulPackUINT32(mode, a->packed, a->offset, (UINT32*)&m_type, 8);
	a->offset = teulPackUINT32(mode, a->packed, a->offset, &m_subType, 8);
	a->offset = teulPackSINT32(mode, a->packed, a->offset, &m_x, 24);
	a->offset = teulPackSINT32(mode, a->packed, a->offset, &m_y, 24);

	if ( mode == TEUL_PACK )
	{
		UINT32 delta = teulTime() - m_posted;
		a->offset = teulPackUINT32(mode, a->packed, a->offset, &delta, 16);
	}
	else
	{
		a->offset = teulPackUINT32(mode, a->packed, a->offset, (UINT32*)&m_posted, 16);
		m_posted = teulTime() - m_posted;
	}

	a->offset = wwiiPackText(mode, a->packed, a->offset, m_notes, MAX_WP_NOTE_LENGTH);

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Joules being applied to a server tracked object

SINT32
STOJoules::packer(PackMode mode, ACTION_OBJECT* a)
{
	a->offset = teulPackUINT32(mode, a->packed, a->offset, &m_uid, 0);
	a->offset = teulPackUINT64(mode, a->packed, a->offset, (UINT64*)&m_joules, 0);
	return a->offset;
}
