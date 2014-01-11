#pragma once
// Created: Fri Aug  6 17:14:44 CDT 2010
//
//! Chat Host implementation of Unmanned Mobile Spawn STO.
//
// Copyright (C) PlayNet, Inc 1999-2010 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "chat/chatSTODamage.h"

class UMSSTO : public ServerSTO
{
public:
	UMSSTO(UINT16 subType, const WorldPosition_m& position, const AttitudeVector& attitude, Player* originator) ;
	UMSSTO(ACTION_OBJECT* const a) ;

	virtual bool needsBroadcastRemoval() const { return true ; }
	virtual const char* typeName() const { return "ums" ; }

public:
	virtual bool activate() ;		// Special activation case.
	virtual bool applyJoules(const SINT64& joules, class Player* player=nullptr) ;

public:
	STODamage& damage() { return m_damage ; }

private:
	STODamage		m_damage ;
} ;
