#include <syscfg.h> // Path is project specific.
// Created: Tue Mar 5 10:40:54 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Marty "Vandal" Poulin
// Supporting Programmer:  Ron "Caveman" McClellan

#include "mem/mem.h"
#include <map>

#if platBuild != BUILD_SERVER
#include "pi/pi.h"
#endif

#include "parse/parse.h"
#include "parse/parseIO.h"
#include "parse/parseWor.h"
#include "strt/strtWOR.h"
#include "strt/strtDTL.h"
#include "weap/weapList.h"
#include "teul/teulKit.h"

#if platBuild == BUILD_SERVER
	#include "fio/fioDefs.h"
	#include "fio/fiod.h"
	#include "str/stringLib.h"
	#include "wwii/wwiiConfigVars.h"

extern ConfigPath cv_thblitzPath;
#else
	#include "fio/fio.h"
#endif

enum SECTION_ENUM
{
	WOR_CP,
	WOR_FACILITY,
	WOR_BUILDING,
	WOR_DEPOT_LINK
};
static bool _fillCp(SINT32 numArgs, FUNCARGS* args);
static bool _fillFacility(SINT32 numArgs, FUNCARGS* args);
static bool _fillDepotLink(SINT32 numArgs, FUNCARGS* args);
static bool _fillCpObject(SINT32 numArgss, FUNCARGS* args);

using ObjMap = std::map<const std::string, void*>;

struct SECTION_HEADER
{
	const SCHAR* name;
	ObjMap objects;
};


static SECTION_HEADER _stratSections[] ={{"CP", ObjMap()},
										{"FACILITY", ObjMap()},
										{"BUILDING", ObjMap()},
										{"DEPOT_LINK", ObjMap()},
										{"", ObjMap()}};

static PARSE_CALLBACK	_stratCallbacks[] = {{"cp",			_fillCp}
											,{"facility",	_fillFacility}
											,{"building",	_fillCpObject}
											,{"depot_link",	_fillDepotLink}
											,{"",			nullptr}};

#if platBuild == BUILD_SERVER
static bool doNothing(SINT32 numArgss, FUNCARGS* args) { return true; }

static PARSE_CALLBACK  blank[]={{"backdrop",				doNothing},
                               {"sphere",					doNothing},
                               {"spherevoxel",				doNothing},
                               {"changeflags",				doNothing},
                               {"changeflag2",				doNothing},
                               {"changelayer",				doNothing},
                               {"changename",				doNothing},
                               {"loadmodel",				doNothing},
                               {"loadcockpitmodel",			doNothing},
                               {"offset",					doNothing},
                               {"offsetgroup",				doNothing},
                               {"placeonsphere",			doNothing},
                               {"rotateplayer",				doNothing},
                               {"moveplayer",				doNothing},
                               {"resetplayer",				doNothing},
                               {"staticlights",				doNothing},
                               {"dynamiclights",			doNothing},
                               {"adddynamiclight",			doNothing},
                               {"addshadow",				doNothing},
                               {"changelightflag",			doNothing},
                               {"addlightning",				doNothing},
                               {"reversenormals",			doNothing},
                               {"loadterrain",				doNothing},
                               {"reduceobject",				doNothing},
                               {"loadanimation",			doNothing},
                               {"setterrainorigin",			doNothing},
                               {"placeobjectonterrain",		doNothing},
                               {"attachanimationtoobject",	doNothing},
                               {"loadsound",				doNothing},
                               {"multitexture",				doNothing},
                               {"",						nullptr},
                              };
#endif

static UINT32 worFileLineNum;
static char _fileName[PN_MAX_PATH];

#if platBuild == BUILD_CLIENT
extern SINT32 LoadingScreen;
#endif

void parseWorKill()
{
	for(UINT32 i = 0; _stratSections[i].name[0] != 0;i++)
	{
		for ( ObjMap::iterator it = _stratSections[i].objects.begin()
				; it != _stratSections[i].objects.end()
				; ++it )
		{
			memFree(it->second);
		}
		_stratSections[i].objects.clear();
	}
}

template<typename _Type>
static _Type* _getValue(SECTION_ENUM sec, const std::string& name)
{
	ObjMap& objects = _stratSections[sec].objects;
	ObjMap::iterator it = objects.find(name);
	return (_Type*)( it != objects.end() ? it->second : nullptr );
}


CP_PARAMS* parseGetCPValues(const std::string& objName)
{
	return _getValue<CP_PARAMS>(WOR_CP, objName);
}

FACILITY_PARAMS* parseGetFacilityValues(const std::string& objName)
{
	return _getValue<FACILITY_PARAMS>(WOR_FACILITY, objName);
}

BUILDING_PARAMS* parseGetBuildingValues(const std::string& objName)
{
	return _getValue<BUILDING_PARAMS>(WOR_BUILDING, objName);
}

DEPOT_LINK_PARAMS* parseGetDepotLinkValues(const std::string& objName)
{
	return _getValue<DEPOT_LINK_PARAMS>(WOR_DEPOT_LINK, objName);
}

bool
parseWor(const char* levelfile, PARSE_CALLBACK* world)
{
	atexit(parseWorKill);

#ifndef VALIDATOR
 #if platBuild == BUILD_SERVER
	if ( !weapHasInitialized() && !weapListInit(cv_thblitzPath.GetString()) )
 #else
	if ( !weapListInit("th_blitz/weapon") )
 #endif // BUILD_SERVER
	{
#if platBuild != BUILD_SERVER
		piExitProgram(_FL,"Weapon Lists did not initialize");
#else
		DEBUG_FATAL("WEAPON_LISTS DID NOT INITIALIZE!!!");
#endif
	}
#endif //VALIDATOR

	strsafecpy(_fileName, levelfile);

#if platBuild == BUILD_SERVER
	world = blank;
#endif

#if platBuild == BUILD_CLIENT
	LoadingScreen = 1;
#endif

	return parseFile(_fileName, _stratCallbacks, world, &worFileLineNum);
}

template<size_t DestSize>
static void _copyObjName(const char* sourceName, char (&destName)[DestSize])
{
    std::string realSourceName = sourceName;
	// the name of an object is basically the filename with the extension
	// and path removed, so we need to check for and strip those.
	auto pos = realSourceName.find_last_of('\\');
	if (pos != std::string::npos)
		realSourceName.erase(0, pos + 1);
	pos = realSourceName.find_last_of('.');
	if (pos != std::string::npos)
		realSourceName.erase(pos);
    assert(realSourceName.length() > 0 && "empty filename or filename ends with '\\'");
	assert(realSourceName.length() < MAX_OBJNAME_LENGTH - 1 && "filename too long" );
    strsafecpy(destName, realSourceName.c_str());
}

template<typename _Type>
static _Type*
_grow(SECTION_HEADER* header, FUNCARGS* args)
{
#ifdef OLD
	static const UINT32 _GROWTH_RATE = 0x40;		// Must be a power-of-2
	if ( ((UINT32)header->numObjects & (_GROWTH_RATE - 1)) == 0 )
	{
		void* newData = memAlloc((header->numObjects + _GROWTH_RATE) * sizeof(_Type));
		if ( header->objArray != nullptr )
		{
			memcpy(newData, header->objArray, header->numObjects * sizeof(_Type));
			memFree(header->objArray);
		}
		header->objArray = newData;
	}
	return &((_Type*)header->objArray)[header->numObjects];
#else
	_Type* thisParam = (_Type*)memAlloc(sizeof(_Type));
	_copyObjName(args[0].svalue, thisParam->filename);
	assert( header->objects.find(thisParam->filename) == header->objects.end() );
	header->objects[thisParam->filename] = thisParam;
	return thisParam;
#endif
}

static bool _fillDepotLink(SINT32 /*numArgs*/, FUNCARGS* args)
{
static ARG_SPEC argSpec[] = {	{T_STRING,PN_MAX_PATH}

								,{T_INT,4}
								,{T_INT,4}
								,{T_INT,4}
								,{T_INT,4}};


static SINT32 numSpecs = sizeof(argSpec)/sizeof(ARG_SPEC);
SECTION_HEADER* dest;

	dest = &_stratSections[WOR_DEPOT_LINK];

	if ( ! parseValidArgs(argSpec, args, numSpecs, "DEPOT_LINK", worFileLineNum) )
		return false;

	DEPOT_LINK_PARAMS* thisParam = _grow<DEPOT_LINK_PARAMS>(dest, args);

	thisParam->haulSpeed		= args[3].dvalue;
	thisParam->haulCapacity		= args[4].dvalue;
	thisParam->minCapacity		= args[5].dvalue;
	thisParam->maxCapacity		= args[6].dvalue;

	return true;

}

static bool _fillCp(SINT32 /*numArgs*/, FUNCARGS* args)
{
static ARG_SPEC argSpec[] = {	LOAD_MODULE_SPEC
								,{T_INT,1}};

static SINT32 numSpecs = sizeof(argSpec)/sizeof(ARG_SPEC);
SECTION_HEADER* dest = &_stratSections[WOR_CP];

	if ( ! parseValidArgs(argSpec, args, numSpecs, "cp", worFileLineNum))
		return false;

	CP_PARAMS* thisParam = _grow<CP_PARAMS>(dest, args);

	thisParam->ttp		= args[MAX_LM_ARG+0].dvalue;

	return true;
}

/*
static SINT32 _validFacilityTypeName(UCHAR* typeName)
{
SINT32 i;

	for (i = 0; i < MAX_FACILITY_TYPES; i++)
	{
		if ( ! strncmp(typeName,facilityTypeName[i],PN_MAX_PATH))
			return (i);
	}

	Write_Debug("missing or bad facility type name %s, section %s, line %d"
				,typeName,_section->name,worFileLineNum);

	return -EINVAL;

}
*/
static bool _fillFacility(SINT32 /*numArgs*/, FUNCARGS* args)
{
static ARG_SPEC argSpec[] = {	LOAD_MODULE_SPEC

								,{T_INT,4}
								,{T_INT,4}
								,{T_INT,4}
								,{T_INT,4}
								,{T_INT,1}};

static SINT32 numSpecs = sizeof(argSpec)/sizeof(ARG_SPEC);
SECTION_HEADER* dest;

	dest = &_stratSections[WOR_FACILITY];

	if ( ! parseValidArgs(argSpec, args, numSpecs,"FACILITY", worFileLineNum))
		return false;

	FACILITY_PARAMS* thisParam = _grow<FACILITY_PARAMS>(dest, args);

	thisParam->shippingThreshold= args[MAX_LM_ARG+0].dvalue;
	thisParam->requestThreshold	= args[MAX_LM_ARG+1].dvalue;
	thisParam->rptl				= args[MAX_LM_ARG+2].dvalue;
	thisParam->rput				= args[MAX_LM_ARG+3].dvalue;
	thisParam->multiplier		= args[MAX_LM_ARG+4].dvalue;

	return true;
}

static bool _fillCpObject(SINT32 /*numArgs*/, FUNCARGS* args)
{
static ARG_SPEC argSpec[] = {	LOAD_MODULE_SPEC

								,{T_STRING,19}
								,{T_INT,4}
								,{T_INT,4}
								,{T_INT,4}
								,{T_INT,4}

								,{T_INT,4}

								,{T_STRING,10}
								,{T_STRING,MAX_OBJNAME_LENGTH}

								,{T_INT,4}
								,{T_INT,1}
								,{T_INT,4}};

static SINT32 numSpecs = sizeof(argSpec)/sizeof(ARG_SPEC);
SECTION_HEADER* dest;
WEAPON_SPEC spec;

	dest = &_stratSections[WOR_BUILDING];

	if ( ! parseValidArgs(argSpec, args, numSpecs, "BUILDING", worFileLineNum))
		return false;

	spec = NULL_WEAP_SPEC;

	if ( strcmp(args[MAX_LM_ARG+7].svalue, "NONE") != 0)
	{
		spec = weapCreateSpec(args[MAX_LM_ARG+7].svalue);
		if (spec.wCategory == 0)
		{
			DEBUG_ERR("Invalid weapon spec %s", args[MAX_LM_ARG+7].svalue);
			return false;
		}
	}

	BUILDING_PARAMS* thisParam = _grow<BUILDING_PARAMS>(dest, args);

	// Some mechanism may need to be used for the 64 bit ASCII to 64 bit integer conversion.
	// The atoll function is not supported on the PC; however, _atoi64 is supported.
#if platBuild == BUILD_SERVER // max damage is not used on front end anyway
	thisParam->maxdamage		= strtoull(args[MAX_LM_ARG+0].svalue, nullptr, 10);
#endif
	thisParam->rpsrepair		= args[MAX_LM_ARG+1].dvalue;
	thisParam->repairtime		= args[MAX_LM_ARG+2].dvalue;
	thisParam->rpslevel			= args[MAX_LM_ARG+3].dvalue;
	thisParam->damageThreshold	= args[MAX_LM_ARG+4].dvalue;
	thisParam->pRebuildlevel	= args[MAX_LM_ARG+5].dvalue;
	thisParam->stratFlags		= (strncmp(args[MAX_LM_ARG+6].svalue, "cap", 3) != 0) ? 0 : 1;
	thisParam->spawn.weap		= spec;
	thisParam->spawn.slots		= args[MAX_LM_ARG+8].dvalue;
	thisParam->spawn.points		= args[MAX_LM_ARG+9].dvalue;
	thisParam->objectValue		= args[MAX_LM_ARG+10].dvalue;

	return true;
}

