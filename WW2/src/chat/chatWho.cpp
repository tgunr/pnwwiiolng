#include <syscfg.h> // Path is project specific.
//
// Date: Thu Jul  7 12:23:04 CDT 2005
//
// Defines functions relating to the Chat::Who class
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith
//


#include "teul/teulTime.h"
#include "chat/chatWho.h"
#include "wwii/wwiiPack.h"
#include "wwii/wwiiSquads.h"

Chat::Who::Who(ACTION_OBJECT* const a)
{
	a->offset = packer(TEUL_UNPACK, a) ;
}

static inline SINT32
_packTimeDelta(LOGICAL mode, UCHAR* message, SINT32 offset, UINT32& time, bool include)
{
	if ( !include )
	{
		if ( mode == TEUL_UNPACK )
			time = 0 ;
		return offset ;
	}

	UINT16 transport = 0 ;

	const UINT32 now = (UINT32)teulTime() ;

	if ( mode == TEUL_PACK )
	{
		if ( time > now )
			transport = 0 ;
		else
		{
			transport = (UINT16)((now - time) / Chat::Who::_SECONDS_PER_DAY) ;
			if ( transport >= Chat::Who::_MAX_DELTA_DAYS )
				transport = Chat::Who::_MAX_DELTA_DAYS + (transport / Chat::Who::_DAYS_PER_MONTH) ;
			if ( transport > 255 )
				transport = 255 ;
		}
	}
	offset = teulPackUINT16(mode, message, offset, &transport, 8) ;
	if ( mode == TEUL_UNPACK )
	{
		if ( transport >= Chat::Who::_MAX_DELTA_DAYS )
			transport = (transport - Chat::Who::_MAX_DELTA_DAYS) * Chat::Who::_DAYS_PER_MONTH ;
		time = now - (transport * Chat::Who::_SECONDS_PER_DAY) ;
	}

	return offset ;
}

SINT32
Chat::Who::packer(LOGICAL mode, ACTION_OBJECT* a)
{
	if ( mode == TEUL_UNPACK )
		memset(this, 0, sizeof(*this)) ;

	a->offset = wwiiPackHandle(mode, a->packed, a->offset, &m_handle, 0) ;
	a->packer(mode, m_online) ;

	a->packer(mode, m_inSameSquad) ;
	a->offset = wwiiPackRankNo(mode, a->packed, a->offset, &m_squadRank, 3) ;
	a->offset = _packTimeDelta(mode, a->packed, a->offset, m_joinedSquad, m_inSameSquad) ;

/*
	if ( !m_inSameSquad && m_squadRank > 0 )
		a->offset = wwiiPackHandle(mode, a->packed, a->offset, &m_squadHandle, 0) ;
	else
*/
	if ( mode == TEUL_UNPACK )
		m_squadHandle = 0LL ;

	if ( !m_online )
		return a->offset ;

	a->packer(mode, m_afk) ;
	a->packer(mode, m_hc) ;
	a->packer(mode, m_extendedInfo) ;
	if ( m_inSameSquad && m_squadRank >= Squad::STAFFER )
		a->packer(mode, m_recruiting) ;
	else if ( mode == TEUL_UNPACK )
		m_recruiting = false ;

	a->offset = wwiiPackRankNo(mode, a->packed, a->offset, &m_rank, 6) ;

	if ( m_rank <= 0 )
		return a->offset ;

	a->offset = wwiiPackCountry(mode, a->packed, a->offset, &m_country, 0) ;
	if ( m_country <= 0 )
		return a->offset ;

	a->offset = teulPackUINT32(mode, a->packed, a->offset, (UINT32*)&m_branch, 3) ;
	if ( m_branch <= 0 )
		return a->offset ;

	a->offset = teulPackUINT32(mode, a->packed, a->offset, &m_hcUnitID, 16) ;
	if ( m_hcUnitID <= 0 || (!m_inSameSquad && !m_extendedInfo) )
		return a->offset ;

	a->offset = teulPackUINT32(mode, a->packed, a->offset, &m_missionID, 0) ;
	if ( m_missionID <= 0 || !m_extendedInfo )
		return a->offset ;

	a->packer(mode, m_leadingMission) ;

	a->packer(mode, m_spawned) ;
	if ( !m_spawned )
		return a->offset ;

	a->offset = wwiiPackFullObjID(mode, a->packed, a->offset, &m_nearCP, 0) ;

	return a->offset ;
}

