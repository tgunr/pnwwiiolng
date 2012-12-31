#pragma once
// Created: Thu Jul 31 11:19:12 CDT 2008
//
// Structure that passes information on a HAAC/Fallback vote
// to high command players.
//
// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "teul/teulPackers.h"
#include "wwii/wwiiPack.h"

class HoldFallVote
{
public:
	enum Status { _VOTED, _WAITING, _STANDS, _TIED } ;

public:
	HoldFallVote(const class StratDecision* decision, Status status) ;
	HoldFallVote() {}
	HoldFallVote(struct ACTION_OBJECT* a) { a->offset = packer(TEUL_UNPACK, a) ; }
	~HoldFallVote() ;

public:
	ChatHandle		m_requester ;
	FULL_OBJ_ID		m_fullObjID ;
	Status			m_status ;
	UINT32			m_remaining ;

	WWII_SIDE		m_side ;
	UINT32			m_score ;
	UINT32			m_holds ;
	UINT32			m_falls ;
	UINT32			m_vetos ;

public:
	static TEUL_CALLBACK*	s_callback ;
	SINT32 packer(LOGICAL mode, ACTION_OBJECT* a)
	{
		a->offset = wwiiPackHandle(mode, a->packed, a->offset, &m_requester, 0) ;
		a->offset = wwiiPackFullObjID(mode, a->packed, a->offset, &m_fullObjID, 0) ;
		a->offset = teulPackUINT32(mode, a->packed, a->offset, (UINT32*)&m_status, 8) ;
		a->offset = teulPackUINT32(mode, a->packed, a->offset, &m_remaining, 16) ;
		a->offset = teulPackUINT32(mode, a->packed, a->offset, (UINT32*)&m_side, 8) ;
		a->offset = teulPackUINT32(mode, a->packed, a->offset, &m_score, 8) ;
		a->offset = teulPackUINT32(mode, a->packed, a->offset, &m_holds, 8) ;
		a->offset = teulPackUINT32(mode, a->packed, a->offset, &m_falls, 8) ;
		a->offset = teulPackUINT32(mode, a->packed, a->offset, &m_vetos, 8) ;
		return a->offset ;
	}
} ;

