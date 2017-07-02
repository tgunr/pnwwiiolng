#include <syscfg.h> // Path is project specific.
// Date: Thu Oct 13 12:50:22 CDT 2005
//
// Class for exchanging details of a generic string
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "chat/chatGenericString.h"
#include "wwii/wwiiPack.h"

GenericString::GenericString(ACTION_OBJECT* const a)
{
	a->offset = packer(TEUL_UNPACK, a);
}

SINT32
GenericString::packer(PackMode mode, ACTION_OBJECT* a)
{
	a->offset = EditString::packer(mode, a);
	if ( m_error == _NO_ERROR || m_error == _ACCEPTED )
	{
		a->offset = teulPackUINT32(mode, a->packed, a->offset, &m_modified, 0);
		a->offset = wwiiPackHandle(mode, a->packed, a->offset, &m_author, 0);
		a->packer(mode, m_editable);
	}

	return a->offset;
}
