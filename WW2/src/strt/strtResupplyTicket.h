#pragma once
// Date: Thu Jul  5 16:12:27 CDT 2007
//
// Object used to schedule delivery of a vehicle to a brigade over time,
// and to reference a posted weapon for spawned vehicles.
//
// Copyright (C) PlayNet, Inc 1999-2007 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "weap/weapSpec.h"			// For WEAPON_SPEC and NULL_WEAP_SPEC

struct POSTED_WEAPON ;

class ResupplyTicket
{
public:
	ResupplyTicket() : m_docket(0), m_spec(NULL_WEAP_SPEC), m_units(0), m_postedWeapon(NULL), m_isTransfer(false) {}

	// To allow TOEs to change templates, we can't persist any entry
	// references outside of the template/toe itself.
	UINT32			m_docket ;				// Number of the docket we're on
	WEAPON_SPEC		m_spec ;				// What type of weapon
	UINT16			m_units ;				// How many will be delivered
	POSTED_WEAPON*	m_postedWeapon ;		// Can be NULL
	bool			m_isTransfer ;			// If true, don't check cp.isSupplied

//	bool operator==(const ResupplyTicket* rhs) { return this == rhs ; }
} ;

