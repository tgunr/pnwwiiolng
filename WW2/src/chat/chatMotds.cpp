#include "syscfg.h" // Path is project specific.
//
// Date: Thu Oct 13 17:49:48 CDT 2005
//
// CRS/Allied/Axis Message of the Day
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith
//


#include "chat/chatMotds.h"
#include "teul/teulPackers.h"
#include "wwii/wwiiPackString.h"
#include "wwii/wwiiPack.h"

SINT32
MOTD_REQUEST::packer(PackMode mode, ACTION_OBJECT* a)
{
	a->offset = teulPackUINT32(mode, a->packed, a->offset, &type, 8);
	return a->offset;
}

SINT32
MOTD_REPLY::packer(PackMode mode, ACTION_OBJECT* a)
{
	a->offset = teulPackUINT32(mode, a->packed, a->offset, &type, 8);
	a->offset = teulPackUINT32(mode, a->packed, a->offset, &added, 0);
	a->offset = wwiiPackHandle(mode, a->packed, a->offset, &poster, 0);
	a->offset = wwiiPackString(mode, a->packed, a->offset, message, _MAX_LENGTH);

	return a->offset;
}
