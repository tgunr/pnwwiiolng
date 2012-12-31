#pragma once
// Created: Mon Dec  4 20:00:41 CST 2006
//
// Ordnance (bombs, grenades, oxtails)
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "chat/chatServerSTO.h"

class OrdnanceSTO : public ServerSTO
{
public:
	OrdnanceSTO(UINT16 withType, UINT16 withSubType, const WorldPosition_m& withPosition, const AttitudeVector& withAttitude, UINT32 withLifetime)
		: ServerSTO(withType, withSubType, withPosition, withAttitude, 0, withLifetime, 0, WWII_NO_SIDE, 0)
		{}
	OrdnanceSTO(ACTION_OBJECT* const a) ;

	virtual bool needsBroadcastRemoval() const { return false ; }
	virtual const char* typeName() const { return "ord" ; }

public:
	SINT32 fromTeulIndex() const { return m_fromTeulIndex ; }

private:
	SINT32	m_fromTeulIndex ;
} ;
