#pragma once
// Created: Tue Jan 28 20:00:08 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

#include "parse/countryCodes.h"
#include "weap/weapSerial.h"
#include "weap/weapWeapon.h"

//tolua_begin

struct WEAPON_CLASS
{
	WEAPON_SPEC	spec;
	bool		active;
	std::string	name;
	UINT32		m_hash;

	static UINT32 GetHash(const std::string& name_)
	{
		UINT32 hash = 937901;
		for (auto it = name_.begin(), end = name_.end(); it != end; ++it)
		{
			hash ^= *it;
			hash = (hash << 3) | (hash >> 29);
		}
		hash ^= name_.length();
		return hash;
	}
};

//tolua_end

// Weapons by spec
typedef std::map<WEAPON_SPEC, WEAPON*> WEAPON_LIST;
// g_weaponList defines ALL known vehicles, including unusable ones
extern WEAPON_LIST g_weaponList;

// Lists of WEAPON_ARRAYs by country+category+class
typedef std::map<UINT32, WEAPON_LIST> WEAPON_SPAWNS;

// g_weaponSpawns provides lists of which weapons can be spawned
// by which category of spawner.
extern WEAPON_SPAWNS g_weaponSpawns;

// If you ever have to look up a class name.
extern size_t g_maxWeaponClass;
extern WEAPON_CLASS weaponClass[MAX_WEAP_CLASSES];

//tolua_begin

// returns true once weapListInit() has been called once
bool weapHasInitialized(void);

// Init the structures
bool weapListInit(const char* theaterPath);

static inline UINT32 weapClassIndex(const WEAPON_SPEC& spec)
{
	return (spec.wClass > 0) ? spec.wClass : ((UINT32)spec.wCategory << 8);
}

// return spawn list for the spec if available
static inline WEAPON_LIST* weapFindSpawnList(const WEAPON_SPEC& spec)
{
	WEAPON_SPAWNS::iterator it = g_weaponSpawns.find(weapClassIndex(spec));
	return ( it != g_weaponSpawns.end() ) ? &(it->second) : nullptr;
}

extern WWII_SIDE weapFindSide(const char* side);

// turn country string into weap spec
extern WEAPON_SPEC	weapFindCountry(const char* country);
// turn country code string into weap spec
extern WEAPON_SPEC	weapFindCountryFromCode(const char* cCode);

// turn category abbreviation string into a weap spec
extern WEAPON_SPEC	weapFindCategory(const char* category);

// find a weapon spec by its name (used to match weapon specs to names
// from the vehicles.txt file on fe etc..) and associate it with a local index
extern WEAPON_SPEC weapAssociateIndex(const char* weapName, UINT32 localIndex);

// Add a class to the class index.
extern void weapAddClass(UINT32 classIndex, const std::string& className);

extern void weapListKill(void);

extern void weapAddWeaponIndex(WEAPON* weapon);

//tolua_end
