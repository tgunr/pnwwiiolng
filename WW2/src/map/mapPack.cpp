#include <syscfg.h> // Path is project specific.
// Date: Tue Jan 23 13:04:30 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Eric "Meat" Hall

#include "wwii/wwiiHCUnits.h"
#include "map/mapBrigadeSummary.h"
#include "map/mapMissionList.h"
#include "map/mapPersonaSummary.h"
#include "mm/mmPack.h"
#include "teul/teulPackers.h"
#include "wwii/wwiiPack.h"
#include "wwii/wwiiPackString.h"

//////////////////////////////////////////////////////////////////////
// Packer function for the PersonaSummary class

SINT32
PersonaSummary::packer(LOGICAL mode, ACTION_OBJECT* a)
{
	a->offset = teulPackUINT32(mode, a->packed, a->offset, &m_personaID, 0);
	a->offset = teulPackUINT32(mode, a->packed, a->offset, &m_lockedThru, 0);
	a->offset = wwiiPackBrigadeID(mode, a->packed, a->offset, m_brigadeID);
	a->offset = teulPackUINT32(mode, a->packed, a->offset, &m_lastUsed, 0);
	a->offset = teulPackUINT32(mode, a->packed, a->offset, (UINT32*)&m_countryID, 5);
	a->offset = teulPackUINT32(mode, a->packed, a->offset, (UINT32*)&m_branchID, 3);
	a->offset = wwiiPackRankNo(mode, a->packed, a->offset, &m_rankID, 0);
	a->offset = teulPackUCHAR(mode, a->packed, a->offset, (UCHAR*)&m_rankPctg, 0);
	a->offset = teulPackUINT32(mode, a->packed, a->offset, &m_availability, 8);
	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Function for packing an entire PERSONA_SUMMARYS vector

SINT32
PersonaSummaries::packer(LOGICAL mode, ACTION_OBJECT* a)
{
	a->offset = teulPackUINT32(mode, a->packed, a->offset, &m_error, 1);
	a->offset = teulPackUINT32(mode, a->packed, a->offset, &m_count, 7);
	a->offset = wwiiPackHandle(mode, a->packed, a->offset, &m_callsign, 0);

	for ( UINT32 i = 0; i < m_count; ++i )
	{
		a->offset = m_summaries[i].packer(mode, a);
	}

	a->offset = teulPackUINT32(mode, a->packed, a->offset, (UINT32*)&m_sidelocked, 4);

	a->offset = m_activeMission.packer(mode, a);

	m_chatPermissions.packer(mode, a);

	a->packSecondsDelta(mode, m_accountGoodUntil);

	return a->offset;
}

//////////////////////////////////////////////////////////////////////
// Pack a persona selection packet

SINT32
PersonaSelection::packer(LOGICAL mode, ACTION_OBJECT* a)
{
	a->offset = teulPackUINT32(mode, a->packed, a->offset, (UINT32*)&m_status, 8);
	a->offset = teulPackUINT32(mode, a->packed, a->offset, &m_personaID, 0);
	a->offset = teulPackUINT32(mode, a->packed, a->offset, (UINT32*)&m_countryID, 5);
	a->offset = teulPackUINT32(mode, a->packed, a->offset, (UINT32*)&m_branchID, 3);
	a->offset = teulPackUINT64(mode, a->packed, a->offset, &m_checksum, 0);
	a->offset = wwiiPackRankNo(mode, a->packed, a->offset, &m_rank, 0);
	a->offset = teulPackUINT32(mode, a->packed, a->offset, &m_rankPctg, 16);

	if ( m_status == PSS_SELECT )
	{
		m_chatPermissions.packer(mode, a);
		a->packSecondsDelta(mode, m_accountGoodUntil);
	}

	return a->offset;
}

SINT32
BrigadeSummary::packer(LOGICAL mode, ACTION_OBJECT* a)
{
	a->offset = wwiiPackBrigadeID(mode, a->packed, a->offset, m_unitID);

	// Byte-aligned group (1+1+6+6+6+4 = 16)
	a->packer(mode, m_active);
	a->packer(mode, m_canPostHere);
	a->offset = teulPackUINT16(mode, a->packed, a->offset, &m_players, 6);
	a->offset = teulPackUINT16(mode, a->packed, a->offset, &m_missions, 6);
	a->offset = teulPackUINT16(mode, a->packed, a->offset, &m_officers, 5);
	a->offset = teulPackUINT16(mode, a->packed, a->offset, &m_mobileSpawns, 4);

//	a->offset = teulPackVoid(mode, a->packed, a->offset, m_languages, MAX_LANGUAGES_PER_BRIGADE);


	a->offset = wwiiPackFullObjID(mode, a->packed, a->offset, &m_objectiveCPID, 0);

	return a->offset;
}

SINT32
BrigadeSummaries::packer(LOGICAL mode, ACTION_OBJECT* a)
{
	a->offset = teulPackUINT32(mode, a->packed, a->offset, (UINT32*)&m_error, 1);
	if ( !m_error )
	{
		a->offset = teulPackUINT32(mode, a->packed, a->offset, &m_count, 7);
		for ( UINT32 i = 0; i < m_count && i < MAX_LISTED_BRIGADES; ++i )
		{
			a->offset = m_summaries[i].packer(mode, a);
		}
	}
	else if ( mode == TEUL_UNPACK )
	{
		m_count = 0;
	}

	return a->offset;
}

SINT32
BrigadeDetail::packer(LOGICAL mode, ACTION_OBJECT* a)
{
	a->offset = HCUnit::packer(mode, a);
	a->offset = BrigadeSummary::packer(mode, a);

	a->offset = wwiiPackText(mode, a->packed, a->offset, m_motd, MAX_BRIGADE_NOTES_LENGTH);
	a->offset = wwiiPackText(mode, a->packed, a->offset, m_motto, MAX_BRIGADE_MOTTO_LENGTH);

	for ( UINT32 i = 0; i < officers() && i < _MAX_OFFICERS; ++i )
	{
		a->offset = wwiiPackHandle(mode, a->packed, a->offset, &m_officerHandles[i], 0);
		a->offset = wwiiPackRankNo(mode, a->packed, a->offset, &m_officerRanks[i], 0);
	}

	return a->offset;
}

SINT32
BrigadeSelection::packer(LOGICAL mode, ACTION_OBJECT* a)
{
	a->offset = teulPackUINT32(mode, a->packed, a->offset, (UINT32*)&m_status, 8);

	// Status determines how much or how little we need to send
	if ( m_status == BRIG_DETAIL || m_status == BRIG_SELECT )
		a->offset = m_detail.packer(mode, a);
	else
		a->offset = wwiiPackBrigadeID(mode, a->packed, a->offset, m_detail.m_unitID);

	return a->offset;
}

SINT32
MissionSynopsis::packer(LOGICAL mode, ACTION_OBJECT* const a)
{
	a->packer(mode, &m_missionID, 0);

	a->packer(mode, m_isCurrent);      // |
	a->packer(mode, m_hasMobileSpawn); // |
	a->packer(mode, m_isOpen);         // } 8 bits
	a->packer(mode, m_canMobileSpawn); // |
	a->packer(mode, &m_type, 4);       // |

	a->packer(mode, &m_subType, 4);
	a->packer(mode, &m_population, 4);

	a->offset = wwiiPackFullObjID(mode, a->packed, a->offset, &m_origin, 0); // 48 bits
	a->offset = wwiiPackFullObjID(mode, a->packed, a->offset, &m_target, 0); // 48 bits

	a->offset = wwiiPackBrigadeID(mode, a->packed, a->offset, m_brigadeID, 0);
	a->offset = wwiiPackCountry(mode, a->packed, a->offset, &m_countryID, 0);
	// TODO: Less hard-coding numbers, more deriving them properly.
	static_assert ( MAX_BRANCHES < 8 , "Packing code assumes branch ID fits in 3 bits." );
	a->packer(mode, &m_branchID, 3);

	a->packChatHandle(mode, &m_poster);

	return a->offset;
}

SINT32
MissionList::packer(LOGICAL mode, ACTION_OBJECT* a)
{
	a->offset = teulPackUINT32(mode, a->packed, a->offset, (UINT32*)&m_status, 2);
	if ( m_status == MLR_OK )
	{
		a->offset = teulPackUCHAR(mode, a->packed, a->offset, (UCHAR*)&m_count, 6);
		for ( UINT32 i = 0; i < m_count && i < MAX_MISSIONS_PER_LIST; ++i )
		{
			a->offset = m_missions[i].packer(mode, a);
		}
		a->offset = teulPackUINT32(mode, a->packed, a->offset, (UINT32*)&m_listStart, 1);
		a->offset = teulPackUINT32(mode, a->packed, a->offset, (UINT32*)&m_listEnd, 1);
	}
	else if ( mode == TEUL_UNPACK )
	{
		m_listStart = m_listEnd = true;
		m_count = 0;
	}

	a->offset = teulPackUINT32(mode, a->packed, a->offset, (UINT32*)&m_canPost, 1);

	a->offset = m_activeMission.packer(mode, a);

	return a->offset;
}

