#include <syscfg.h> // Path is project specific.
// Created: Fri Nov 12 14:41:14 CST 2010
//
// Packers for the server tracked object classes
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith
//

#include "wwii/wwiiServerTrackedObject.h"
#include "teul/teulPackers.h"

//////////////////////////////////////////////////////////////////////
// Pack routines for server tracked objects.

SINT32
WorldPosition_m::packer(LOGICAL mode, ACTION_OBJECT* const a)
{
	a->offset = teulPackFLOAT64(mode, a->packed, a->offset, &m_x, 0) ;
	a->offset = teulPackFLOAT64(mode, a->packed, a->offset, &m_y, 0) ;
	FLOAT32 toZ = (FLOAT32)m_z ;
	a->offset = teulPackFLOAT32(mode, a->packed, a->offset, &toZ, 0) ;
	if ( mode == TEUL_UNPACK )
		m_z = (FLOAT64)toZ ;

	return a->offset ;
}

//////////////////////////////////////////////////////////////////////
// Pack routine

SINT32
AttitudeVector::packer(LOGICAL mode, ACTION_OBJECT* const a)
{
	a->offset = teulPackFLOAT32(mode, a->packed, a->offset, &m_x, 0) ;
	a->offset = teulPackFLOAT32(mode, a->packed, a->offset, &m_y, 0) ;
	a->offset = teulPackFLOAT32(mode, a->packed, a->offset, &m_z, 0) ;
	return a->offset ;
}

