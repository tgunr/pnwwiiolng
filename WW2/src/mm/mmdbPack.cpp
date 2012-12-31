#include <syscfg.h> // Path is project specific.
// Date: Thu May 10 11:15:15 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Eric "Meat" Hall

#include "wwii/wwiiPack.h"
#include "map/mapPlayer.h"
#include "mm/mmdbPack.h"
#include "wwii/wwiiHCUnits.h"
#include "map/mapBrigadeSummary.h"

SINT32
BrigadeDetailsUpdate::packer(LOGICAL mode, ACTION_OBJECT* a)
{
	a->offset = wwiiPackBrigadeID(mode, a->packed, a->offset, m_brigadeID) ;
	a->offset = teulPackUINT32(mode, a->packed, a->offset, &m_lastModified, 0) ;
	a->offset = wwiiPackHandle(mode, a->packed, a->offset, &m_owner, 0) ;
	a->offset = wwiiPackRankNo(mode, a->packed, a->offset, &m_ownerRank, 0) ;
	a->offset = wwiiPackText(mode, a->packed, a->offset, m_title, MAX_HCUNIT_NAME_LENGTH) ;
	a->offset = wwiiPackText(mode, a->packed, a->offset, m_motd, MAX_BRIGADE_NOTES_LENGTH) ;
	a->offset = wwiiPackText(mode, a->packed, a->offset, m_motto, MAX_BRIGADE_MOTTO_LENGTH) ;
	a->offset = wwiiPackText(mode, a->packed, a->offset, m_shortName, MAX_BRIGADE_SHORT_LENGTH) ;
	a->offset = wwiiPackFullObjID(mode, a->packed, a->offset, &m_homeCPID, 0) ;
	a->offset = teulPackUINT32(mode, a->packed, a->offset, &m_ownerID, 0) ;

	return a->offset ;
}


