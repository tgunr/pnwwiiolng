#pragma once
// Created: Wed Aug 31 12:59:54 CDT 2005
//
// Definitions of weapon types used between weapon files
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "parse/countryCodes.h"
#include "weap/weapDefs.h"				// For MAX_CNAME_SIZE

struct WEAPON_COUNTRY
{
	Parse::COUNTRY_CODE	code;	// country code from countryCode.h
	LOGICAL				active;		// if loaded its active
	WWII_SIDE			side;
	UCHAR				name[MAX_CNAME_SIZE];	// text name of country
} ;

// all countries with weapons
// If you have a county index, this is the place
// to find the weap country info.
extern WEAPON_COUNTRY weaponCountry[MAX_COUNTRIES];

namespace Parse
{
	extern const SCHAR* countryCode[MAX_COUNTRY_CODES+1] ;
}

