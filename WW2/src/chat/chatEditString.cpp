#include <syscfg.h> // Path is project specific.
// Date: Thu Oct 13 12:34:07 CDT 2005
//
// Allow a player to send a string up to the host
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "chat/chatEditString.h"
#include "wwii/wwiiPackString.h"

//////////////////////////////////////////////////////////////////////
// Network based constructor

EditString::EditString(ACTION_OBJECT* const a)
{
	packer(TEUL_UNPACK, a);
}

//////////////////////////////////////////////////////////////////////
// Packer

SINT32
EditString::packer(LOGICAL mode, ACTION_OBJECT* a)
{
	a->offset = teulPackUINT32(mode, a->packed, a->offset, &m_category, 8);
	a->offset = teulPackUINT64(mode, a->packed, a->offset, &m_id, 0);
	a->offset = teulPackUINT32(mode, a->packed, a->offset, &m_error, 8);
	if ( m_error == _NO_ERROR )
		a->offset = wwiiPackString(mode, a->packed, a->offset, m_text, _MAX_TEXT_LENGTH);
	else if ( mode == TEUL_UNPACK )
		m_text = "";

	return a->offset;
}

