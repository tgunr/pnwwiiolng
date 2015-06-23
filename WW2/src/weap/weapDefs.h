#pragma once
// Created: Fri May 11 18:44:36 CDT 2007
//
// Copyright (C) PlayNet, Inc 1999-2007 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

//tolua_begin

#define MAX_CLASS_NAME_LEN	64
#define MAX_WEAP_NAME_LEN	16
#define MAX_WEAP_FULL_NAME_LEN	32

#define MAX_CNAME_SIZE		32
#define MAX_SNAME_SIZE		16

#define MAX_WEAP_CLASSES	32
#define MAX_BIAS_INDEX			10
#define DEFAULT_BIAS_SLOTS		4
#define DEFAULT_FY_BIAS_SLOTS	10
#define DEFAULT_EY_BIAS_SLOTS	1000

// Number of /players/ that can man a vehicle
#define MAX_WEAP_STATIONS	2

enum WWII_SERVICE
{
	WWII_ARMY,
	WWII_NAVY,
	WWII_AIRFORCE,

	//Add new services here
	MAX_SERVICE
};

//#define WWII_SIDE SIDE
//#define WWII_MAX_COUNTRY MAX_COUNTRIES
// ============================>
// enusre the next three declarations
// are all updated togeather

enum WEAPON_CATEGORY
{
	_NO_CATEGORY = 0,
	_AIRCRAFT,
	_GROUND_VEHICLE,
	_SEA_VEHICLE,
	_INFANTRY,
	_MUNITIONS,
	MAX_WEAP_CATEGORY
};

enum WEAPON_CAT_ABV
{
	NO_CAT = 0,	// _NO_CATEGORY
	AC_,		// _AIRCRAFT
	GV_,		// _GROUND_VEHICLE
	SV_,		// _SEA_VEHICLE
	IN_,		// _INFANTRY
	MN_			// _MUNITIONS
};

//tolua_end
