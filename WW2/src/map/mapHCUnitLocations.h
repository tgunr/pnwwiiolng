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

//////////////////////////////////////////////////////////////////////
// Garrison Change Requested Action - Server Directed Actions

enum GARRISON_CHANGE_ACTION
{
	CHANGE_NOTHING = 0			//  NO Changes.
	, CHANGE_COUNTRY			//  Garrison Changes Country = HC Changes the Garrison unit Country. Persona = Change.	
	, CHANGE_SIDE				//  Garrison Changes Side = CP captured by the Enemy. Persona = Probably NO Change.
	, CHANGE_BRANCH				//  Garrison Changes Branch = The Replacement Unit has a different Branch. Persona = Change.
	, CHANGE_COUNTRY_BRANCH		//  Garrison Changes Country and Branch = The Replacement Unit has a different Country AND Branch. Persona = Change.
};

class HCUnitLocation
{
public:
	// Constructors
	HCUnitLocation(UINT32 unitID, const FULL_OBJ_ID fid)
		: m_unitID(unitID), m_homeCPID(fid)
		{}
	HCUnitLocation(ACTION_OBJECT* const a) { packer(TEUL_UNPACK, a); }
	HCUnitLocation() : m_unitID(0), m_homeCPID(nullObjID) {}

	// Destructors
	~HCUnitLocation() {}

	// Shared member functions
	SINT32 packer(PackMode mode, ACTION_OBJECT* a);

	// host only member functions

	// client only member functions

public:
	// Members
	UINT32		m_unitID;			// 16 bits	} nice round
	FULL_OBJ_ID	m_homeCPID;			// 48 bits	} 64 bits!
};

using HC_UNIT_LOCATIONS = teul_network_array<HCUnitLocation>;

class HCUnitCountryChange
{
public:
	// Constructors
	HCUnitCountryChange(UINT32 unitID, const FULL_OBJ_ID fid)
		: m_unitID(unitID), m_country(WWII_NO_COUNTRY), m_replacementUnitID(0), m_action(CHANGE_NOTHING)
	{}
	HCUnitCountryChange(ACTION_OBJECT* const a) { packer(TEUL_UNPACK, a); }
	HCUnitCountryChange() : m_unitID(0), m_country(WWII_NO_COUNTRY), m_replacementUnitID(0), m_action(CHANGE_NOTHING) {}

	// Destructors
	~HCUnitCountryChange() {}

	// Shared member functions
	SINT32 packer(PackMode mode, ACTION_OBJECT* a);

	SINT32 receive(SINT32 teulIndex);
	
public:
	// Members
	UINT32 m_unitID;
	WWII_COUNTRY m_country;
	UINT32 m_replacementUnitID;  //  If NOT = 0, the proposed replacement Favoured Garrison Unit (Possibly a Training Unit)
	GARRISON_CHANGE_ACTION m_action;  //  Desired Action for the Client
};

using HC_UNIT_COUNTRY_CHANGES = teul_network_array<HCUnitCountryChange>;

