#include <syscfg.h> // Path is project specific.
// Date: Tue Sep 20 18:26:36 CDT 2005
//
// Packers etc for SquadControl
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith


#include "dbug/dbug.h"

#include "chat/chatSquadControl.h"
#include "wwii/wwiiPack.h"
#include "wwii/wwiiPackString.h"
#include "teul/teulBasic.h"
#include "chat/chatHandle.h"
#include "chat/chatName.h"

SINT32
SquadControl::packer(LOGICAL mode, ACTION_OBJECT* a)
{
	a->offset = teulPackUINT32(mode, a->packed, a->offset, &m_action, 8);

	if ( mode == TEUL_UNPACK )
	{
		m_players.clear();
		m_playerIDs.clear();
	}

	switch ( m_action )
	{
	case _CREATE:
		CREATE_packer(mode, a);
	break;
	case _DISBAND:
		// None
	break;
	case _JOIN:
		handle_packer(mode, a);
	break;
	case _RECRUIT:
		handle_packer(mode, a);
	break;
	case _ACCEPT:
		// None
	break;
	case _KICK:
		handle_packer(mode, a);
	break;
	case _LEAVE:
		// None
	break;
	case _PROMOTE:
		handle_packer(mode, a);
	break;
	case _DEMOTE:
		handle_packer(mode, a);
	break;
	case _TEXT:
		text_packer(mode, a);
	break;
	case _RESIGN:
		handle_packer(mode, a);
	break;
	case _INTEGER:
		int_packer(mode, a);
	break;
	}

	return a->offset;
}

SINT32
SquadControl::CREATE_packer(LOGICAL mode, ACTION_OBJECT* a)
{
	UINT32 len = 0;

	a->offset = wwiiPackHandle(mode, a->packed, a->offset, &m_handle, 0);
	a->offset = wwiiPackString(mode, a->packed, a->offset, m_text, MAX_TEXT_LEN);

	if ( mode == TEUL_PACK )
		len = (UINT32)m_players.size();

	a->offset = teulPackUINT32(mode, a->packed, a->offset, &len, 8);

	if ( mode == TEUL_UNPACK )
	{
		if ( len > CREATE_PLAYERS_REQUIRED )
			len = CREATE_PLAYERS_REQUIRED;
		m_players.resize(len);
	}

	for ( UINT32 i = 0; i < len; ++i )
	{
		a->offset = wwiiPackHandle(mode, a->packed, a->offset, &m_players[i], 0);
		dbugLog(_FLI, "squad contains %s", Chat::Name(m_players[i]).c_str());
	}
	return a->offset;
}

SINT32
SquadControl::handle_packer(LOGICAL mode, ACTION_OBJECT* a)
{
	a->offset = wwiiPackHandle(mode, a->packed, a->offset, &m_handle, 0);
	return a->offset;
}

SINT32
SquadControl::text_packer(LOGICAL mode, ACTION_OBJECT* a)
{
	a->offset = teulPackUINT32(mode, a->packed, a->offset, &m_key, 8);
	a->offset = wwiiPackString(mode, a->packed, a->offset, m_text, MAX_TEXT_LEN);
	return a->offset;
}

SINT32
SquadControl::int_packer(LOGICAL mode, ACTION_OBJECT* a)
{
	a->offset = teulPackUINT32(mode, a->packed, a->offset, &m_key, 8);
	a->offset = teulPackUINT32(mode, a->packed, a->offset, &m_value, 0);
	return a->offset;
}

SINT32
SquadControlResult::packer(LOGICAL mode, ACTION_OBJECT* a)
{
	a->offset = teulPackUINT32(mode, a->packed, a->offset, &m_result, 8);
	a->offset = teulPackUINT32(mode, a->packed, a->offset, &m_action, 8);
	a->offset = wwiiPackHandle(mode, a->packed, a->offset, &m_handle, 0);
	return a->offset;
}

extern TEUL_CALLBACK* CHAT_SQUAD_CONTROL;

SINT32
SquadControl::_sendBasic(SINT32 connection, UINT32 action, const ChatHandle& handle, const std::string& text)
{
	if ( PN_UNLIKELY(connection < 0) )
		return -EINVAL;

	m_action = action;
	m_handle = handle;
	m_text = text;

	return teulSend(connection, CHAT_SQUAD_CONTROL, this);
}

