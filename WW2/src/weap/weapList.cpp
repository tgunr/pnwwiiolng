#include <syscfg.h> // Path is project specific.
// Created: Tue Jan 28 20:00:08 2001
//
// All the routines for reading in the weap files
// to create and access the weapon hierarchies.
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Marty "Vandal" Poulin

#include "weap/weapInternal.h"
#include "weap/weapList.h"
#include "weap/weapParser.h"
#include "parse/countryCodes.h"
#include "wwii/wwiiHash.h"
#include "wwii/wwiiPermissions.h"

#include "str/stringLib.h"

#define COUNTRY_P2			9
#define	CAT_P2				5
#define MAX_WEAP_CLASS_NAME	32

#define WEAP_IDENT_SIZE 3

static bool g_weaponListIsInitialized;

typedef std::map<UINT32, WEAPON*> WEAPON_INDEXES;
static WEAPON_INDEXES s_weaponIndexes;

const char* facilityTypeName[] = {	"NONE",
									"CITY",
									"FACTORY",
									"BASE",
									"DEPOT",
									"OMT",				// Off Map Transfer (OMT)
									"SHIPYARD",
									"FIREBASE",
									"AIRBASE",
									"ARMYBASE",
									"NAVALBASE"
								};

const char* wwii_country[MAX_COUNTRIES] =	{
									"WWII_UNDEF",
									"WWII_ENGLAND",
									"WWII_USA",
									"WWII_FRANCE",
									"WWII_GERMANY",
									"WWII_ITALY",
									"WWII_CHINA",
									"WWII_RUSSIA",
								};

const char* wwii_side[] =		{
									"NO_SIDE",
									"ALLIED",
									"AXIS",
									"NEUTRAL",
								};

const char* wwii_cat_long[] =	{
									"_NO_CATEGORY",
									"_AIRCRAFT",
									"_GROUND_VEHICLE",
									"_SEA_VEHICLE",
									"_INFANTRY",
									"_MUNITIONS",
								};

const char* wwii_category[] = {	"NO_CAT"
									,"AC_"
									,"GV_"
									,"SV_"
									,"IN_"
									,"MN_"
								};

/*
static const UCHAR* relation[] = {
									"NONE",
									"P",	// Parent or Primary
									"C",	// Child or Crew
								};
*/

const WEAPON_SERIAL NULL_WEAP_SERIAL = { { { 0, 0 }, 0, 0, 0 }, 0, NULL_WEAP_SPEC };

const FULL_OBJ_ID nullObjID = { { 0, 0 }, 0, 0, 0 };


// If you have a country index, this is the place
// to find the weap country info.
WEAPON_COUNTRY weaponCountry[MAX_COUNTRIES];

// lookup weapCountry indexes from country codes;
SINT16	weapCountryIndex[Parse::MAX_COUNTRY_CODES];

// If you ever have to look up a class name.
WEAPON_CLASS weaponClass[MAX_WEAP_CLASSES];
size_t g_maxWeaponClass = 0;

// All known weapons
WEAPON_LIST g_weaponList;
extern std::vector<std::unique_ptr<WEAPON>> g_weaponPtrs;

// Weapon Category tables
WEAPON_SPAWNS g_weaponSpawns;

// Weapons indexed by name
typedef std::unordered_map<std::string,WEAPON*> WEAPON_NAME_INDEX;
static WEAPON_NAME_INDEX weaponNameIndex;

bool weapHasInitialized()
{
	return g_weaponListIsInitialized;
}

WWII_SIDE weapFindSide(const char* side)
{
SINT32 i;

	for( i = 1; i < MAX_SIDES; i++ )
	{
		if ( !strcasecmp(wwii_side[i], side) )
			return (WWII_SIDE)i;
	}

	return (WWII_SIDE)0;
}

WEAPON_SPEC weapFindCountryFromCode(const char* cCode)
{
	WEAPON_SPEC weap = NULL_WEAP_SPEC;
	SINT32 i, cnum;

	cnum = findCountryCode(cCode);

	for( i = 1; i < MAX_COUNTRIES; i++ )
	{
		if ( weaponCountry[i].code == cnum )
		{
			weap.wCountry = i;
			break;
		}
	}

	return weap;
}

WEAPON_SPEC
weapFindCountry(const char* country)
{
	WEAPON_SPEC weap = NULL_WEAP_SPEC;
	SINT32 i;

	for( i=1;i<MAX_COUNTRIES;i++)
	{
		if ( ! strcasecmp(wwii_country[i],country))
		{
			weap.wCountry = i;
			break;
		}
	}

	return weap;
}

WEAPON_SPEC
weapFindCategory(const char* weapCat)
{
	WEAPON_SPEC weap = NULL_WEAP_SPEC;
	SINT32 i;

	for( i=1;i<MAX_WEAP_CATEGORY;i++)
	{
		if ( ! strcasecmp(wwii_category[i],weapCat))
		{
			weap.wCategory = i;
			break;
		}
	}

	return weap;
}

UINT8 weapFindClassIndex(const std::string& className)
{
	UINT32 hash = WEAPON_CLASS::GetHash(className);
	for ( size_t i = 1; i <= g_maxWeaponClass; ++i )
	{
		if (PN_UNLIKELY(weaponClass[i].active == false))
			continue;
		if (weaponClass[i].m_hash == hash && weaponClass[i].name == className)
			return static_cast<UINT8>(i);
	}
	return 0;
}

WEAPON_SPEC weapCreateSpec(UINT32 country_, UINT32 category_, UINT32 class_, UINT32 type_)
{
    WEAPON_SPEC weap = NULL_WEAP_SPEC;

    weap.wCountry  = (UINT8)country_;
    weap.wCategory = (UINT8)category_;
    weap.wClass    = (UINT8)class_;
    weap.wType     = (UINT8)type_;

    return weap;
}

WEAPON_SPEC weapCreateSpec(const char* weapString)
{
	WEAPON_SPEC weap = NULL_WEAP_SPEC;
	SINT32 wType;
	char sCountry[WEAP_IDENT_SIZE+1]
		,sCategory[WEAP_IDENT_SIZE+1]
		,sClass[MAX_CLASS_NAME_LEN]
		;

	const char* const sType = strstr(weapString, ".");

	if (sType == nullptr || sType < weapString )
		return NULL_WEAP_SPEC;

	wType = (SINT32)atol(sType+1);
	weap.wType = wType;

	if (wType < 0 || wType > 255)
		return NULL_WEAP_SPEC;

	const char* tempString = strrchr(weapString, '_');

	if (tempString < weapString)
		return NULL_WEAP_SPEC;

	memset(sClass,0,MAX_CLASS_NAME_LEN);
	memset(sCountry,0,WEAP_IDENT_SIZE+1);
	memset(sCategory,0,WEAP_IDENT_SIZE+1);

	if (tempString != nullptr)
	{
		tempString++;

		size_t sClassLen = sType - tempString;
		strncpy(sClass, tempString, std::min(sizeof(sClass), sClassLen));

		if (! strncmp(sClass,"null",MAX_CLASS_NAME_LEN) )
			weap.wClass = 0;
		else
			weap.wClass = weapFindClass(sClass).wClass;

		tempString -= WEAP_IDENT_SIZE;
		if (tempString >= weapString)
		{
			memcpy(sCategory,tempString,WEAP_IDENT_SIZE);
			weap.wCategory = weapFindCategory(sCategory).wCategory;

			tempString -= WEAP_IDENT_SIZE;
			if (tempString >= weapString)
			{
				memcpy(sCountry,tempString,WEAP_IDENT_SIZE);
				weap.wCountry = weapFindCountryFromCode(sCountry).wCountry;
			}
		}

		return weap;
	}
	else
	{
		size_t sClassLen = sType - weapString;
		strncpy(sClass, weapString, sClassLen);
		weap.wClass = weapFindClass(sClass).wClass;
		weap.wCategory = 0;
		weap.wCountry = 0;

		return weap;
	}

}

WEAPON_SPEC
weapAssociateIndex(const char* objName, UINT32 localIndex)
{
	if ( objName == nullptr )
		return NULL_WEAP_SPEC;

	char weapName[MAX_WEAP_NAME_LEN];
	strsafecpy(weapName, objName);
	_strlwr(weapName);

	WEAPON* weapon = weapLookupWeaponByName(weapName);
	if ( weapon == nullptr )
	{
#if platBuild == BUILD_CLIENT
		Write_Debug("weapon %s not defined li:%u", weapName, localIndex);
#else
		LogAbort("weap:%s li:%u no weapon to associate with", weapName, localIndex);
#endif
		return NULL_WEAP_SPEC;
	}

	weapon->m_localIndex = localIndex;
	s_weaponIndexes[localIndex] = weapon;

	return weapon->spec();
}

void
weapAddWeaponIndex(WEAPON* weapon)
{
	s_weaponIndexes[weapon->m_localIndex] = weapon;
}

bool weapListInit(const char* theaterPath)
{
	SINT32 country,category;
	char importFile[PN_MAX_PATH];

	memset(weapCountryIndex, 0, sizeof(weapCountryIndex));
	g_weaponList.clear();
	g_weaponSpawns.clear();
	g_weaponPtrs.clear();

#if platOS == OS_WIN32
	strprintf(importFile, "%s\\%s", theaterPath, "weapCountry.csv");
#else
	strprintf(importFile, "%s/%s", theaterPath, "weapCountry.csv");
#endif
	if ( ! weapParseCountry(importFile))
	{
		dbugLog(_FLA, "couldnt parse countries file (%s)", importFile);
		return false;
	}

#if platOS == OS_WIN32
	strprintf(importFile, "%s\\%s", theaterPath, "weapClasses.csv");
#else
	strprintf(importFile, "%s/%s", theaterPath, "weapClasses.csv");
#endif
	if ( ! weapParseClasses(importFile))
	{
		dbugLog(_FLA, "couldnt parse class file (%s)", importFile);
		return false;
	}

	for ( country = 1; country < MAX_COUNTRIES; country++)
	{
		if (! weaponCountry[country].active )
			continue;

		for ( category = 1; category < MAX_WEAP_CATEGORY; category++)
		{
			strprintf(importFile, "%s\\%s%s%s"
					, theaterPath, Parse::countryCode[weaponCountry[country].code], wwii_category[category], "weap.csv");
//#define LIVE_DANGEROUS

			if (!weapParseWeapons(importFile,country,category) && category != _MUNITIONS )
			{
				dbugLog(_FLA,"couldn't parse %u.%u weapons file (%s)", country, category, importFile);
#ifndef LIVE_DANGEROUS
				return false;
#endif
			}
		}
	}

	for ( country = 1; country < MAX_COUNTRIES; country++)
	{
		if (! weaponCountry[country].active )
			continue;

		for ( category = 1; category < MAX_WEAP_CATEGORY; category++)
		{
			strprintf(importFile, "%s\\%s%s%s"
					, theaterPath, Parse::countryCode[weaponCountry[country].code], wwii_category[category], "spawn.csv");
			if ( ! weapParseSpawn(importFile, country, category) && category != _MUNITIONS )
			{
				dbugLog(_FLA,"couldn't parse spawn file for %s",importFile);
#ifndef LIVE_DANGEROUS
				return false;
#endif
			}
		}
	}

	g_weaponListIsInitialized = true;

	return true;
}

void weapListKill(void)
{
	g_weaponList.clear();
	g_weaponSpawns.clear();
	g_weaponPtrs.clear();
}

#if platOS != OS_WIN32
# define strnicmp strncasecmp
#endif

// Find the weapon spec of a weapon by its name
const WEAPON_SPEC&
weapFindWeaponByName(const char* name)
{
	if ( name == nullptr || *name == 0 )
		return NULL_WEAP_SPEC;

	WEAPON_NAME_INDEX::iterator it = weaponNameIndex.find(name);
	if ( it == weaponNameIndex.end() || it->second == nullptr )
		return NULL_WEAP_SPEC;
	return it->second->spec();
}

WEAPON*
weapLookupWeaponByName(const char* name)
{
	if ( name == nullptr || *name == 0 )
		return nullptr;

	WEAPON_NAME_INDEX::iterator it = weaponNameIndex.find(name);
	return ( it != weaponNameIndex.end() ) ? it->second : nullptr;
}

WEAPON*
weapLookupWeaponByIndex(UINT32 localIndex)
{
	WEAPON_INDEXES::iterator it = s_weaponIndexes.find(localIndex);
	return ( it != s_weaponIndexes.end() ? it->second : nullptr );
}

// Lookup the weapon with the fast index
WEAPON* weapLookup(const WEAPON_SPEC& spec)
{
	WEAPON_LIST::iterator it = g_weaponList.find(spec);
	return ( it != g_weaponList.end() ) ? it->second : nullptr;
}

//////////////////////////////////////////////////////////////////////
// Constructor for WEAPON objects

static SINT16
_getAccessValue(const char* key)
{
	if ( key == nullptr || *key == 0 )
		return 0;
	if ( isdigit(*key) )
		return atoi(key);
	return atoi(key+1);
}

WEAPON::WEAPON(const WEAPON_SPEC& withSpec
				, const char* Name, const char* withAccess
				, UINT16 unitCount
				, UINT32 lod_cm, UINT32 AlertDist, UINT32 Speed, UINT32 CVCValue
				)
	: m_spec(withSpec)
	, m_access(_getAccessValue(withAccess))
	, m_initialAccess(m_access)
	, m_nextAccess(m_access)
	, m_count(unitCount)
	, m_friendlyBias(0)
	, m_enemyBias(0)
	, m_localIndex(0)
	, LOD(lod_cm)
	, speed(Speed)
	, value(CVCValue)
	, alertDist(AlertDist)
	, maxLod_cm2((UINT64)lod_cm * (UINT64)lod_cm)
	, alertDist2((UINT64)AlertDist * (UINT64)AlertDist)
	, m_modified(false)
{
	strncpy(name, Name, MAX_WEAP_NAME_LEN);
	strncpy(fullName, Name, MAX_WEAP_NAME_LEN);
	strlwr(name);

	memset(bias, 0, sizeof(bias));

	if ( weapLookupWeaponByName(name) != nullptr )
		throw std::runtime_error("multiple definitions of vehicle");		///Terminal

	weaponNameIndex[name] = this;
}

//////////////////////////////////////////////////////////////////////
// Destructor which deregisters weapons from the list

WEAPON::~WEAPON()
{
	if ( name[0] != 0 )
	{
		// De-register myself
		WEAPON_NAME_INDEX::iterator it = weaponNameIndex.find(name);
		if ( it != weaponNameIndex.end() && it->second == this )
			weaponNameIndex.erase(it);
	}
}

//////////////////////////////////////////////////////////////////////
// Determine whether a particular permissions mask grants access to
// this particular weapon; disableRankLimits is usually passed from
// host_config on the host.

bool
WEAPON::isPermitted(const PermissionMask& permMask, bool disableRankLimits) const
{
	switch (access())
	{
		case 0:
			return true;

		// On the training server, drop thru to "default"
		case 1:
			return (disableRankLimits || permMask & (VEHICLE_LEVEL1_MASK|ALL_SPAWN));
		case 2:
			return (disableRankLimits || permMask & (VEHICLE_LEVEL2_MASK|ALL_SPAWN));
		case 3:
			return (disableRankLimits || permMask & (VEHICLE_LEVEL3_MASK|ALL_SPAWN));
		case 4:
			return (disableRankLimits || permMask & (VEHICLE_LEVEL4_MASK|ALL_SPAWN));
		case 5:
			return (disableRankLimits || permMask & (VEHICLE_LEVEL5_MASK|ALL_SPAWN));
		case 6:
			return (disableRankLimits || permMask & (VEHICLE_LEVEL6_MASK|ALL_SPAWN));
		case 7:
			return (disableRankLimits || permMask & (VEHICLE_LEVEL7_MASK|ALL_SPAWN));
		case 8:			// Skull
			return (permMask & VEHICLE_LEVEL8_MASK) != 0;
		case 9:			// Buzzard and above
		case 10:
		case 11:
			return (permMask & VEHICLE_LEVEL9_MASK) != 0;

		default:
			// If this changes to 'false', the training server
			// will need a special case version.
			return true;
	}
}

