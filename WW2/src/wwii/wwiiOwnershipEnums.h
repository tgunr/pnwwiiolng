#pragma once
// Created: Thu Jul 20 13:55:21 CDT 2006
//
// Enumerations for country/side and related macros/globals
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

//tolua_begin

//] Ownership of in-game real-estate consists of a
//+ 'country' and a 'side'. Each country is associated
//+ with one side and the country=>side mapping is
//+ fixed and persistent.

enum WWII_COUNTRY			//] Definitive list of in-game countries.
							//] Not all countries are active or playable,
							//+ and some countries (CN/RU) have special
							//+ meaning.
{
	WWII_NO_COUNTRY = 0,	//] No known country or uninitialized data
	WWII_ENGLAND,			//] 'UK' England (Allied)
	WWII_USA,				//] 'US' USA (Allied)
	WWII_FRANCE,			//] 'FR' France (Allied)
	WWII_GERMANY,			//]	'DE' Germany (Axis)
	WWII_ITALY,				//] 'IT' Italy (Axis)
	WWII_CHINA,				//] 'CN' China (neutral+friendly)
							//]  Chinese towns/facilites are not capturable but
							//+  allow supply to flow thru them.
	WWII_RUSSIA,			//] 'RU' Russia (neutral+hostile)
							//]  Russian towns/facilities are capturable but
							//+  obstruct supply.
	// Add new countries here
	MAX_COUNTRIES			//] Guaranteed to be greater than the highest country no.
};


enum WWII_SIDE				//] Definitive list of in-game sides.
							//] Not all sides are active or playable,
							//+ and some sides have special meaning.
{
	WWII_NO_SIDE,			//] No known side or uninitialized data
	ALLIED=1,				//] Allied group of countries
	AXIS,					//] Axis group of countries
	NEUTRAL,				//] Used to indicate "neither Allied nor Axis"
							//] Players with a sidelock of NEUTRAL are banned.
	// Add new sides here
	MAX_SIDES				//] Guaranteed to be greater than the highest side no.
};

static inline WWII_SIDE wwiiSide(UINT32 country)
{
	switch ( static_cast<WWII_COUNTRY>(country) )
	{
	case WWII_NO_COUNTRY:
		return WWII_NO_SIDE;
	case WWII_ENGLAND:
	case WWII_FRANCE:
	case WWII_USA:
		return ALLIED;
	case WWII_GERMANY:
	case WWII_ITALY:
		return AXIS;
	case WWII_CHINA:
	case WWII_RUSSIA:
	case MAX_COUNTRIES:
	default:
		return NEUTRAL;
	}
}

static inline WWII_SIDE wwiiOtherSide(UINT32 side)
{
	switch ( static_cast<WWII_SIDE>(side) )
	{
		case WWII_NO_SIDE: return WWII_NO_SIDE;
		case ALLIED: return AXIS;
		case AXIS: return ALLIED;
		case NEUTRAL:
		case MAX_SIDES:
		default:
			return NEUTRAL;
	}
}

static inline WWII_COUNTRY wwiiAdjustedCountry(UINT32 country)
{
	return static_cast<WWII_COUNTRY>((country < MAX_COUNTRIES) ? country : country - 2);
}

//tolua_end
static inline bool isValidSide(WWII_SIDE n) { return (n == ALLIED || n == AXIS); }

