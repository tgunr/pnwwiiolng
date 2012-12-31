#include <syscfg.h> // Path is project specific.
//
// Date: Tue Nov 29 17:20:56 CST 2005
//
// Shared client/host functions for HCUnitLocations
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith


#include "map/mapHCUnitLocations.h"
#include "wwii/wwiiPack.h"

SINT32
HCUnitLocation::packer(LOGICAL mode, ACTION_OBJECT* a)
{
	a->offset = wwiiPackBrigadeID(mode, a->packed, a->offset, m_unitID, 0) ;
	a->offset = wwiiPackFullObjID(mode, a->packed, a->offset, &m_homeCPID, 0) ;
	return a->offset ;
}

