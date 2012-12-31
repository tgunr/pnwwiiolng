#include <syscfg.h> // Path is project specific.
// Created: Tue Feb 20 10:40:54 2001
//
// Each weapon type in the game has a unique, four-octet
// identifier. Each octet has a significance that can be
// used to derive the vehicle. They are:
//  country.category.class.type
// "Categories" are derived from the branch, and are
//  AC_ = Aircraft
//  IN_ = Infantry
//  GV_ = Ground Vehicle
//  SV_ = Naval Vehicle
//  MN_ = Munitions
//
// Data is read from the following files:
//  weapCountry.csv
//	  lists countries in use and their active state
//  weapClasses.csv
//    enumerates classes within each category
//  <COUNTRY>_<CATEGORY>_weap.csv
//    declares the vehicles for a country+category,
//    vehicles are grouped by class within the .csv file
//    typeid is enumerated from 1 within each 3c group
//  <COUNTRY>_<CATEGORY>_spawn.csv
//    game-world spawn-objects are associated with a
//    category and optional class pair. The _spawn file
//    lists which vehicles are available at which category
//    of spawn object.
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Marty "Vandal" Poulin

#include "parse/parse.h"
#include "parse/parseIO.h"
#include "weap/weapInternal.h"
#include "weap/weapParser.h"
#include "weap/weapList.h"
#include "parse/countryCodes.h"
#include "strt/strtDTL.h"

#if platOS != OS_MACOS
	#include "str/stringLib.h"
#endif

enum
{
	PARSE_COUNTRY,
	PARSE_CLASS,
	PARSE_WEAPON,
	PARSE_SPAWN
};

static bool _setSection(int numargs, FUNCARGS* args);

static bool _fillCountry(SINT32 numArgs, FUNCARGS* args);
static bool _fillClass(SINT32 numArgs, FUNCARGS* args);
static bool _fillWeapon(SINT32 numArgs, FUNCARGS* args);
static bool _fillSpawn(SINT32 numArgs, FUNCARGS* args);

static PARSE_CALLBACK	_sectionCallbacks[] = {{"section",	_setSection}
											,{"",			NULL}};


static PARSE_CALLBACK	_userCallbacks[] = {{"",			NULL}
											,{"",			NULL}};


static PARSE_CALLBACK	_spawnCallbacks[] = {{"spawn",		_fillSpawn}
											,{"",			NULL}};
struct WEAP_SECTION_HEAD
{
	WEAPON_SPEC		spec;
	WEAPON_LIST*	spawnList;
	SCHAR			name[MAX_OBJNAME_LENGTH];
};

static WEAP_SECTION_HEAD _section;

static UINT32	weapFileLineNum;
static char _fileName[PN_MAX_PATH];

static SINT32 _curCountry;

static SINT32 _curCategory;

static SINT32 _parseConstant;

bool weapParseCountry(char* countryfile)
{
	strncpy(_fileName,countryfile,PN_MAX_PATH);

	_userCallbacks[0].function = _fillCountry;

	_parseConstant = PARSE_COUNTRY;

	return parseFile(_fileName,_sectionCallbacks,_userCallbacks, &weapFileLineNum);
}

bool weapParseClasses(char* classFile)
{
	strncpy(_fileName,classFile,PN_MAX_PATH);

	_userCallbacks[0].function = _fillClass;
	_parseConstant = PARSE_CLASS;

	return parseFile(_fileName,_sectionCallbacks,_userCallbacks, &weapFileLineNum);
}

bool weapParseWeapons(char* weapfile, SINT32 country, SINT32 category)
{
	strncpy(_fileName,weapfile,PN_MAX_PATH);

	_userCallbacks[0].function = _fillWeapon;
	_parseConstant = PARSE_WEAPON;

	_curCountry = country;
	_curCategory = category;

	return parseFile(_fileName,_sectionCallbacks,_userCallbacks, &weapFileLineNum);
}

bool weapParseSpawn(char* weapfile, SINT32 country, SINT32 category)
{
	strncpy(_fileName,weapfile,PN_MAX_PATH);

	_parseConstant = PARSE_SPAWN;

	_curCountry = country;
	_curCategory = category;

	return parseFile(_fileName, _sectionCallbacks, _spawnCallbacks, &weapFileLineNum);
}

static bool _setSection(int /*numargs*/, FUNCARGS* args)
{
	strncpy(_section.name, args[0].svalue, sizeof(_section.name));

	switch (_parseConstant)
	{
	case PARSE_COUNTRY:
		strlwr(_section.name);

		strncpy(_userCallbacks[0].type, _section.name, MAX_FUNC_NAME);

		break;
	case PARSE_CLASS:
		strlwr(_section.name);

		strncpy(_userCallbacks[0].type, _section.name,MAX_FUNC_NAME);

		_section.spec = weapFindCategory(_section.name);

		if (*((UINT32*)&_section.spec) == *((UINT32*)&NULL_WEAP_SPEC))
			return false ;
		break;

	case PARSE_WEAPON:
		// "weapon" section handles entries from the weapon.csv file;
		// these define the list of known weapons.
		strlwr(_section.name);

		strncpy(_userCallbacks[0].type, _section.name,MAX_FUNC_NAME);

		// Populate the wClass
		_section.spec = weapFindClass(_section.name);

		if (*((UINT32*)&_section.spec) == *((UINT32*)&NULL_WEAP_SPEC))
		{
#ifdef FRONT_END
			Write_Debug("invlid class name %s",_section.name);
#endif // FRONT_END
			return false ;
		}

		_section.spec.wCountry	= _curCountry;	// set country
		_section.spec.wCategory = _curCategory;	// set category
		// wClass populated from section name
		_section.spec.wType		= 0;			// set type

		break;
	case PARSE_SPAWN:
		// "spawn" section handles the spawn.csv file which builds
		// maps spawn objects <=> spawnable vehicles.

		strlwr(_section.name);

		if ( strncmp(_section.name,"category",MAX_FUNC_NAME) != 0 )
		{
			_section.spec = weapFindClass(_section.name);
			if ( _section.spec.wClass == 0 )
				return false ;
		}
		else
			_section.spec.wClass = 0 ;

		_section.spec.wCountry	= _curCountry;	// set country
		_section.spec.wCategory = _curCategory;	// set category
		// wClass populated from section name
		_section.spec.wType = 0 ;

		_section.spawnList = &g_weaponSpawns[weapClassIndex(_section.spec)] ;

		break;
	default:
		return false ;
	};

	strupr(_section.name);

	return true ;
}

static bool _fillCountry(SINT32 /*numArgs*/, FUNCARGS* args)
{
	static ARG_SPEC argSpec[] = {	{T_INT,4}					// country number
									,{T_STRING,4}				// country code
									,{T_STRING,MAX_CNAME_SIZE}	// country name to match with enum
									,{T_STRING,MAX_SNAME_SIZE} // side
									,{T_INT,1}};	// active

	static const SINT32 _NUM_SPECS = sizeof(argSpec)/sizeof(ARG_SPEC);
	WEAPON_COUNTRY* thisCountry;
	WEAPON_SPEC spec;

	thisCountry = NULL;

	if ( ! parseValidArgs(argSpec, args, _NUM_SPECS, _section.name, weapFileLineNum) )
		return false ;

	spec = weapFindCountry(args[2].svalue);

	if (spec.wCountry == 0)
		return false ;

	thisCountry = &weaponCountry[spec.wCountry];
	strcpy((SCHAR*)thisCountry->name, args[2].svalue);

	thisCountry->code = findCountryCode(args[1].svalue);

	thisCountry->side = weapFindSide(args[3].svalue);

	thisCountry->active = (args[4].dvalue == 0) ? false : true;

	if (thisCountry->code == 0 || thisCountry->side == 0)
		thisCountry->active = false ;

	return true ;

}

void
weapAddClass(UINT32 classIndex, const char* name)
{
	strncpy((SCHAR*)weaponClass[classIndex].name, name, MAX_CLASS_NAME_LEN);
	weaponClass[classIndex].active = true ;
	weaponClass[classIndex].spec = NULL_WEAP_SPEC;
	weaponClass[classIndex].spec.wClass = classIndex ;
}

static bool _fillClass(SINT32 /*numArgs*/, FUNCARGS* args)
{
	static ARG_SPEC argSpec[] = {	{T_INT,4}
									,{T_STRING,MAX_CLASS_NAME_LEN}
								};

	static const SINT32 _NUM_SPECS = sizeof(argSpec)/sizeof(ARG_SPEC);

	if ( ! parseValidArgs(argSpec, args, _NUM_SPECS, _section.name, weapFileLineNum) )
		return false ;

	UINT32 classIndex = (UINT32)args[0].dvalue ;
	if ( classIndex >= MAX_WEAP_CLASSES )
	{
		dbugLog(_FLA, "invalid weapClass '%s' (%s)", args[0].svalue, args[1].svalue) ;
		return false ;
	}

	if ( weaponClass[classIndex].active )
	{
		dbugLog(_FLA, "duplicate weapClass %u (%s and %s)", classIndex, weaponClass[classIndex].name, args[1].svalue) ;
		return false ;
	}

	weapAddClass((UINT32)classIndex, args[1].svalue) ;

	return true ;
}

//////////////////////////////////////////////////////////////////////
// <ctry>_<category>_weap.csv provides a baseline list of all the
// vehicles for this country+category pair. Vehicles are listed
// by class with each vehicle specifying its own type id, e.g.
// UK_GV_weap.csv <-- sets the country and category
// section,tank,...   <- creates the tank category
// 1,a13,..           <- wType = 1 => UK_GV_tank_1
// "weap" files are read before "spawn" files

static bool
_fillWeapon(SINT32 numArgs, FUNCARGS* args)
{
	static ARG_SPEC argSpec[] = {	{T_INT,4}
									,{T_STRING,MAX_CNAME_SIZE}
									,{T_STRING,3}
									,{T_INT,4}
									,{T_INT,4}

									,{T_STRING,2}
									,{T_STRING,MAX_CNAME_SIZE}

									,{T_INT,4}		// LOD
									,{T_INT,4}		// AlertDistance
									,{T_INT,4}		// SPEED
									,{T_INT,4}};	// VALUE
	static const SINT32 _NUM_SPECS = sizeof(argSpec)/sizeof(ARG_SPEC);
	WEAPON_SPEC thisSpec = NULL_WEAP_SPEC ;

#ifndef FRONT_END
	bool fNext = false, eNext = false;
#endif

	try
	{

		if ( ! parseValidArgs(argSpec, args, _NUM_SPECS, _section.name, weapFileLineNum) )
				throw "invalid entry" ;

		thisSpec = _section.spec;			// set class
		thisSpec.wType = args[0].dvalue;	// set type

		if ( weapLookup(thisSpec) != NULL )
			throw "multiple definitions of vehicle" ;

		WEAPON* weapon = new WEAPON(thisSpec, args[1].svalue, args[2].svalue
									, args[4].dvalue, args[7].dvalue * 100, args[8].dvalue, args[9].dvalue, args[10].dvalue
									) ;

#ifndef FRONT_END
		SINT32 i, bIndex;
		WEAPON_SPEC biasSpec;

		bIndex = 0;
		weapon->m_friendlyBias =  DEFAULT_FY_BIAS_SLOTS;
		weapon->m_enemyBias =  DEFAULT_EY_BIAS_SLOTS;

		// put bias code here
		for (i=11; i < numArgs && args[i].svalue[0] !='\0'; i++)
		{
			if (args[i].baseType == T_STRING)
			{
				biasSpec = weapCreateSpec(args[i].svalue);

				if ( SPEC_IS_NULL(biasSpec) )
				{
					biasSpec.wCountry = findCountryCode(args[i].svalue);

					if ( SPEC_IS_NULL(biasSpec) )
						throw "invalid bias data" ;
					else
					{
						if (biasSpec.wCountry == Parse::FY_)
							fNext = true;
						else if (biasSpec.wCountry == Parse::EY_)
							eNext = true;
						else
							return false;

						continue;
					}
				}

				if (biasSpec.wCountry == 0) // assume enemy
					biasSpec.wCountry = Parse::EY_;

				weapon->bias[bIndex].spec = biasSpec;
				weapon->bias[bIndex].slots = DEFAULT_BIAS_SLOTS;

#ifdef EXTREME_DEBUG
				dbugLog(_FLN,"loaded bias %s spec->%s for vehicle %s"
								,args[i].svalue, _printWeapSpecC(&biasSpec)
								,weapon->name);
#endif

				++bIndex;
				if (bIndex > MAX_BIAS_INDEX)
					break;
			}
			else if (fNext)
			{
				weapon->m_friendlyBias =  args[i].dvalue;
			}
			else if (eNext)
			{
				weapon->m_enemyBias =  args[i].dvalue;
			}
			else if (args[i].baseType == T_INT && bIndex > 0)
			{
				bIndex--;

				if ( SPEC_IS_NULL(weapon->bias[bIndex].spec) )
					throw "illegal weapon bias slot" ;

				weapon->bias[bIndex].slots = args[i].dvalue;

#ifdef EXTREME_DEBUG
				dbugLog(_FLN,"updated min %d for bias %s spec->%s for vechile %s"
								,args[i].dvalue
								,args[i-1].svalue, _printWeapSpecC(&biasSpec)
								,weapon->name);
#endif
				bIndex++;
			}
			else
				throw "illegal value in bias data" ;

		}
#endif //!FRONT_END

#ifdef EXTREME_DEBUG
		dbugLog(_FLD, "adding %s => %p", weapPrintSpecC(&thisSpec), weapon) ;
#endif
		g_weaponList[thisSpec] = weapon ;

		return true ;
	}
	catch ( std::exception& e )
	{
#ifdef FRONT_END
		Write_Log(LOG_WEAP, LOG_FATAL, "%s:%u %s/%s %s", _fileName, weapFileLineNum, _section.name, args[1].svalue, e.what()) ;
#else
		dbugLog(_FLA, "%s:%u %s/%s %s", _fileName, weapFileLineNum, _section.name, args[1].svalue, e.what()) ;
#endif
	}
	catch ( const char* reason )
	{
#ifdef FRONT_END
		Write_Log(LOG_WEAP, LOG_FATAL, "%s:%u %s/%s %s", _fileName, weapFileLineNum, _section.name, args[1].svalue, reason) ;
#else
		dbugLog(_FLA, "%s:%u %s/%s %s", _fileName, weapFileLineNum, _section.name, args[1].svalue, reason) ;
#endif
	}

	return false ;
}

//////////////////////////////////////////////////////////////////////
// The "spawn" files provide control over which spawners can
// spawn which vehicles. This allows us to restrict specific vehicles
// to specific spawn locations.

static bool _fillSpawn(SINT32 /*numArgs*/, FUNCARGS* args)
{
	static ARG_SPEC argSpec[] = {	{T_STRING,MAX_CNAME_SIZE} };
	static const SINT32 _NUM_SPECS = sizeof(argSpec)/sizeof(ARG_SPEC);
	char weapName[MAX_SVALUE_SIZE+1] ;

	try
	{
		if ( ! parseValidArgs(argSpec, args, _NUM_SPECS, _section.name, weapFileLineNum) )
			throw "invalid entry" ;

		UINT32 len ;
		for ( len = 0 ; len < MAX_SVALUE_SIZE && args[0].svalue[len] != 0 ; ++len )
			weapName[len] = tolower(args[0].svalue[len]) ;
		weapName[len] = 0 ;

		const WEAPON_SPEC& mySpec = _section.spec ;
		WEAPON* weap = weapLookupWeaponByName(weapName) ;
		if ( weap == NULL || weap->spec().wType == 0 )
			throw "weapon not found" ;
		const WEAPON_SPEC& spec = weap->spec() ;
		if ( mySpec.wCountry != spec.wCountry )
			throw "country mismatch" ;
		// Allow infantry to spawn at GV_ and AC_ spawners
		if ( mySpec.wCategory != spec.wCategory && (mySpec.wCategory != IN_ && spec.wCategory != GV_) && (spec.wCategory != IN_ || mySpec.wCategory == SV_) )
			throw "category mismatch" ;

		WEAPON_LIST& spawnList = *_section.spawnList ;

		if ( spawnList.find(spec) != spawnList.end() )
			throw "multiple spawn list entries" ;

		spawnList[spec] = weap ;
	}
	catch ( const char* reason )
	{
#ifdef FRONT_END
		Write_Log(LOG_WEAP, LOG_FATAL, "%s:%u %s/%s: %s", _fileName, weapFileLineNum, _section.name, weapName, reason) ;
		return false ;
#else
		LogAbort("%s:%u %s/%s: %s", _fileName, weapFileLineNum, _section.name, weapName, reason) ;
#endif
	}

	return true ;
}

