#include "syscfg.h" // Path is project specific.
// Created: Tue Feb 20 10:40:54 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Marty "Vandal" Poulin

#include "parse/countryCodes.h"

namespace Parse
{
	const SCHAR* countryCode[MAX_COUNTRY_CODES+1] = {
		"ND_",   // Not Defined
		"UK_",   // United Kingdom
		"US_",   // United States
		"DE_",   // Germany
		"FR_",   // France
		"IT_",   // Italy
		"FY_",   // Friendly
		"EY_",   // Enemy
		"MAX_COUNTRY_CODES"
	};
}

Parse::COUNTRY_CODE
findCountryCode(const char* country)
{
	for ( UINT32 i = 0; i < Parse::MAX_COUNTRY_CODES; ++i )
	{
		if ( strcasecmp(Parse::countryCode[i],country) == 0 )
			return (Parse::COUNTRY_CODE)i;
	}

	return Parse::ND_;
}

