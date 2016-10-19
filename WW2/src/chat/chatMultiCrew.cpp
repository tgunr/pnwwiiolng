#include <syscfg.h> // Path is project specific.
// Date: Wed Jul 19 15:31:51 CDT 2006
//
// Multi-crew classes and messages
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "chat/chatMultiCrew.h"
#include "teul/teulPackers.h"
#include "wwii/wwiiPack.h"

//////////////////////////////////////////////////////////////////////
// Not the simplest of packers, designed to try and minimize
// bandwidth use by selecting which bits will be transmitted

SINT32
MultiCrewEntry::packer(PackMode mode, ACTION_OBJECT* a)
{
	// Which player is being described
	a->offset = wwiiPackHandle(mode, a->packed, a->offset, &m_handle, 0);
	a->packer(mode, m_isRemoval);

	// That's all we need to send if its a removal
	if ( isRemoval() )
		return a->offset;

	bool packWeapon = false, packAttached = false;
	if ( mode == TEUL_PACK )
	{
		if ( weapon().wType != 0 )
			packWeapon = true;
		if ( isAttached() )
			packAttached = true;
	}

	a->packer(mode, m_isPilot);
	a->packer(mode, m_isAvailable);
	a->packer(mode, packWeapon);
	a->packer(mode, packAttached);

	// Adjust visibility bits to produce +8 (its currently 3)
	a->offset = teulPackUCHAR(mode, a->packed, a->offset, &m_visibility, _VISIBILITY_BITS);

	if ( packWeapon )
	{
		m_weapon.packer(mode, a);
		a->offset = teulPackUINT32(mode, a->packed, a->offset, &m_serial, 0);
	}

	if ( packAttached )
		a->offset = wwiiPackHandle(mode, a->packed, a->offset, &m_attachedTo, 0);

	a->offset = wwiiPackHandle(mode, a->packed, a->offset, &m_handle, 0);

	return a->offset;
}

