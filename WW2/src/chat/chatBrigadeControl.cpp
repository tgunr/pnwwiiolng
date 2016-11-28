#include <syscfg.h> // Path is project specific.
// Date: Mon Oct  9 14:31:06 CDT 2006
//
// Classes used to request and manipulate brigade lists and OICs
// This file provides the common client/host functionality
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "chat/chatBrigadeControl.h"
#include "chat/chatBrigadeControlList.h"
#include "wwii/wwiiPack.h"

TEUL_CALLBACK* BrigadeControl::s_callback = nullptr;

//////////////////////////////////////////////////////////////////////
// Base class packer (so much more elegant with Teul::Serialize)

SINT32
BrigadeControl::packer(PackMode mode, ACTION_OBJECT* a)
{
	bool haveOfficer = (mode == TEUL_PACK) ? (m_officer != 0) : false;
	bool haveSponsor = (mode == TEUL_PACK) ? (m_sponsor != 0) : false;

	a->packer(mode, &m_action, 6);											//}
	a->packer(mode, haveOfficer, 1);										//} 8 bits
	a->packer(mode, haveSponsor, 1);										//}

	a->offset = teulPackUCHAR(mode, a->packed, a->offset, &m_country, 5);	// }
	a->offset = teulPackUCHAR(mode, a->packed, a->offset, &m_branch, 3);	// } 8 bits

	a->offset = wwiiPackBrigadeID(mode, a->packed, a->offset, m_brigadeID, 0);

	if ( haveOfficer )
		a->offset = wwiiPackHandle(mode, a->packed, a->offset, &m_officer, 0);
	if ( haveSponsor )
		a->offset = wwiiPackHandle(mode, a->packed, a->offset, &m_sponsor, 0);

	a->offset = wwiiPackFullObjID(mode, a->packed, a->offset, &m_target, 0);
	a->offset = wwiiPackFullObjID(mode, a->packed, a->offset, &m_origin, 0);

	return a->offset;
}

SINT32
BrigadeControlNotice::packer(PackMode mode, ACTION_OBJECT* a)
{
	BrigadeControl::packer(mode, a);

	a->offset = teulPackUINT32(mode, a->packed, a->offset, &m_notice, 8);
	if ( m_notice == BrigadeControl::_ERROR )
		a->offset = teulPackUINT32(mode, a->packed, a->offset, &m_error, 16);
	else if ( mode == TEUL_UNPACK )
		m_error = 0;

	// How long until the action will occur
	a->offset = teulPackUINT32(mode, a->packed, a->offset, &m_time, 16);

	a->offset = teulPackUINT32(mode, a->packed, a->offset, &m_votePermit, 8);

	if( mode == TEUL_UNPACK )
		m_received = teulTime();

	return a->offset;
}

