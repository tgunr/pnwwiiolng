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

void
WorldPosition_m::pack(ACTION_OBJECT* const a, SINT32) const
{
	a->pack(m_x, 0);
	a->pack(m_y, 0);
	FLOAT32 z32bit = FLOAT32(m_z);
	a->pack(z32bit, 0);
}

void
WorldPosition_m::unpack(ACTION_OBJECT* const a, SINT32)
{
	a->unpack(m_x, 0);
	a->unpack(m_y, 0);
	FLOAT32 z32bit = 0;
	a->unpack(z32bit, 0);
	m_z = FLOAT64(z32bit);
}

//////////////////////////////////////////////////////////////////////
// Pack routine

void
AttitudeVector::pack(ACTION_OBJECT* const a, SINT32) const
{
	a->pack(m_x, 0);
	a->pack(m_y, 0);
	a->pack(m_z, 0);
}

void
AttitudeVector::unpack(ACTION_OBJECT* const a, SINT32)
{
	a->unpack(m_x, 0);
	a->unpack(m_y, 0);
	a->unpack(m_z, 0);
}
