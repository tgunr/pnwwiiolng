#pragma once
// Created: Tue Nov 29 17:20:56 CST 2005
//
// Structure/transfer mechanism for describing
// movements of HCUnits
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "teul/teulNetworkArray.h"
#include "wwii/wwiiFullObjID.h"
#include "wwii/wwiiHCUnitsIndex.h"

class HCUnitLocation
{
public:
	// Constructors
	HCUnitLocation(HCUnitID unitID, const FULL_OBJ_ID fid)
		: m_unitID(unitID), m_homeCPID(fid)
		{}
	HCUnitLocation(ACTION_OBJECT* const a) { packer(TEUL_UNPACK, a); }
	HCUnitLocation() : m_unitID(HCUnitID::null), m_homeCPID(nullObjID) {}

	// Destructors
	~HCUnitLocation() {}

	// Shared member functions
	SINT32 packer(PackMode mode, ACTION_OBJECT* a);

	// host only member functions

	// client only member functions

public:
	// Members
	HCUnitID	m_unitID;			// 16 bits	} nice round (Actually 32. Oops!)
	FULL_OBJ_ID	m_homeCPID;			// 48 bits	} 64 bits!
};

using HC_UNIT_LOCATIONS = teul_network_array<HCUnitLocation>;

