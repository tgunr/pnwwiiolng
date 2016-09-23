#include <syscfg.h> // Path is project specific.
// Created: Thu Mar  8 12:44:11 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Eric "Meat" Hall
// Supporting Programmer:  Shane "Elmo" Wickson

#include <sys/stat.h>
#include <set>

#include "mem/mem.h"
#include "sdm/sdmParse.h"
#include "fio/fio.h"
#if defined(LIMITED_FIO) && LIMITED_FIO != 0
# include "fio/fioDefs.h"
# include "fio/fiod.h"
#endif

#include "include/io_mapped_file.h"
#include "sdm/sdmConfigVars.h"

// Instead of importing pi.h and its baggage
typedef struct
{
 FLOAT32   x;
 FLOAT32   y;
 FLOAT32   z;

} piVECTOR, *piLPVECTOR;

#include "trn/trntypes.h"

// Translate CP/Facility names.
typedef std::map<const std::string,const std::string> TRANSLATIONS;
static TRANSLATIONS s_translations;

// Errors translating CPs/Facilities
typedef std::set<std::string> TRANSLATION_FAILS;
static TRANSLATION_FAILS* s_cpTranslationFails, *s_facilityTranslationFails;
static bool s_translationsRequired = false;
static const char* s_translationLanguage = "en";

#define _TFSIZE 4*sizeof(UINT16) + 2*sizeof(UINT32)
//#undef CANTPACKSTRUCT
static STRT_DTL_HEADER header;
static void*			currentData = nullptr;
static CP*				cpList = nullptr;
static FACILITY*		facilityList = nullptr;
static BUILDING*		buildingList = nullptr;
static DEPOT_LINK*		linkList = nullptr;
static WAYPOINT*		waypointList = nullptr;
static UINT32 maxId = 0;
static UINT16 currentCP = 0, currentFacility  = 0, currentBuilding = 0, currentLink = 0, currentWaypoint = 0;

const STRT_DTL_HEADER&
sdmGetDtlHeader()
{
	return header;
}

//////////////////////////////////////////////////////////////////////
// Check to see if translations are required

static const char*
_getTranslationFile()
{
	const char* envValue = cv_stratLanguage.GetString();

	static char translationFileName[128];
	if ( envValue == nullptr || strcmp(envValue, "en") == 0 )
	{
		s_translationsRequired = false;
		envValue = "en";
	}
	else
	{
		s_translationsRequired = true;
		s_translationLanguage = envValue;
	}

	snprintf(translationFileName, 128, "data/strat-trans.%s.txt", envValue);
	fprintf(stderr, "- Using STRAT_LANGUAGE: '%s', translation file: %s\n", s_translationLanguage, translationFileName);

	struct stat buf;
	if ( s_translationsRequired && stat(translationFileName, &buf) != 0 )
	{
		fprintf(stderr, "*** strat translation file [%s] does not exist/read error\n", translationFileName);
		LogAbort("strat translation file [%s] does not exist/read error", translationFileName);
	}

	return translationFileName;
}

//////////////////////////////////////////////////////////////////////
// Load CP/facility name translations from a file

static void
sdmLoadTranslationList()
{
	if ( s_translations.empty() == false )
		return;

	static const char* fromFile = _getTranslationFile();
	if ( fromFile == nullptr )
		return;

	FILE* fp = fopen(fromFile, "rb");
	if ( fp == nullptr )
	{
		if ( s_translationsRequired )
			LogAbort("Unable to open translation file: %s", fromFile);
		return;
	}

	static const UINT32 maxLen = 500;
	char readBuf[maxLen + 4 ];

	while ( fgets(readBuf, maxLen, fp) )
	{
		char* key = readBuf;
		while ( *key == ' ' || *key == '\t' )
			++key;
		if ( !*key || *key == '\r' || *key == '\n' )
			continue;

		char* translation = strchr(key, ',');
		if ( translation == nullptr )
			continue;
		*(translation++) = 0;

		// Chomp the trailing whitespace.
		char* p = translation;
		while ( *p && *p != '\r' && *p != '\n' )
			++p;
		*p = 0;

		std::string keyStr(key);

		if ( s_translations.find(keyStr) != s_translations.end() )
		{
			fprintf(stderr, "Duplicate translation for %s: %s vs %s\n"
							, key
							, s_translations[keyStr].c_str()
							, translation);
			dbugLog(_FLW, "Duplicate translation for %s: %s vs %s"
							, key
							, s_translations[keyStr].c_str()
							, translation);
			continue;
		}

		s_translations.insert(std::make_pair(keyStr, translation));
	}

	fclose(fp);
}

//////////////////////////////////////////////////////////////////////
// Convert a cp/facility/etc name to 7-bit ASCII, removing umlauts etc
// and then apply any translation file we have.
// Returns:
//  - true if the translations table is empty,
//  - true if the ASCII-7 name was translated via the translation table,
//  - false if the ASCII-7 name was not found in the translation table.

static bool
_applyTranslationsToName(char* name, TRANSLATION_FAILS* fails, bool basicOnly)
{
	// Translate all names down to ASCII-7.
	size_t nameLen;
	for ( nameLen = 0; name[nameLen] != 0; ++nameLen )
	{
		if ( (unsigned char)name[nameLen] & 0x80 )
		{
			switch ( (unsigned char) name[nameLen] )
			{
			case 0xC8: case 0xC9: case 0xCA: case 0xCB: name[nameLen] = 'E';
			break;
			case 0xD2: case 0xD3: case 0xD4: case 0xD5: case 0xD6: name[nameLen] = 'O';
			break;
			case 0xE0: case 0xE1: case 0xE2: case 0xE3: case 0xE4: case 0xE5: case 0xE6: name[nameLen] = 'a';
			break;
			case 0xE8: case 0xE9: case 0xEA: case 0xEB: name[nameLen] = 'e';
			break;
			case 0xF2: case 0xF3: case 0xF4: case 0xF5: case 0xF6: case 0xF7: case 0xF8: name[nameLen] = 'o';
			break;
			case 0xF9: case 0xFA: case 0xFB: case 0xFC: name[nameLen] = 'u';
			break;
			default:
				LogAbort("Invalid character in strat name [%c (%u)] %s", name[nameLen], (UINT32)((UCHAR)name[nameLen]), name);
			}
		}
	}

	// Chomp trailing whitespaces.
	while (nameLen > 0 && isspace(name[nameLen - 1]))
		--nameLen;
	name[nameLen] = 0;

	// Return true if the translations table is empty.
	if ( basicOnly == true || s_translations.empty() )
		return true;

	// Lookup the ASCII-7 name in the translations table.
	TRANSLATIONS::iterator it = s_translations.find(name);
	if ( it == s_translations.end() )
	{
		if ( fails )
			fails->insert(name);
		// No match, untranslated.
		return false;
	}

	// Replace the name with the translation.
	strcpy(name, it->second.c_str());
	return true;
}

//////////////////////////////////////////////////////////////////////
// If a TRANSLATION_FAILS set is populated, report it as a list of
// errors.

static bool
_reportTranslationFails(const char* listType, TRANSLATION_FAILS& fails)
{
	if ( fails.empty() )
		return false;

	TRANSLATION_FAILS::iterator it = fails.begin();
	fprintf(stderr, "\n########################################\n## %s missing %s translations:\n%s", listType, s_translationLanguage, (*it).c_str());
	dbugLog(_FLA, "%s missing translations:", listType);
	dbugLog(_FLA, "%s", (*it).c_str());
	while ( (++it) != fails.end() )
	{
		fprintf(stderr, ", %s", (*it).c_str());
		dbugLog(_FLA, ", %s", (*it).c_str());
	}
	fprintf(stderr, "\n\n");

	return true;
}

static SINT32 _getTerrainFileLength(SINT32 fp );
SINT32 sdmOpenFile(const SCHAR* path, UINT16 /* mode */)
{
	maxId = 0;

	if ( PN_UNLIKELY(path == nullptr || *path == '\0') )
		return SDMFILE_READ_ERR;

	sdmLoadTranslationList();

	int fd = fiodOpen(path);
	if ( PN_UNLIKELY(fd <= -1) )
		return SDMFILE_READ_ERR;

	//get the starting position of cpf data
	SINT32 sdmLength = _getTerrainFileLength(fd);
	fiodSeek(fd, sdmLength + (SINT32)sizeof(TERRAINFILE), fioSEEK_SET);

	if ( PN_UNLIKELY(fiodRead(fd,&header,sizeof(STRT_DTL_HEADER)) != (int)sizeof(STRT_DTL_HEADER)) )
	{
		fioClose(fd);
		return SDMFILE_READ_ERR;
	}

	if (sdmGetTotalStratObjects() == 0)
	{
		fioClose(fd);
		return SDMNO_STRAT_DATA;
	}

	currentCP = 0;
	currentFacility  = 0;
	currentBuilding = 0;
	currentLink = 0;
	currentWaypoint = 0;

	if ( s_translationsRequired == true )
	{
		if ( PN_UNLIKELY(s_cpTranslationFails == nullptr) )
			s_cpTranslationFails = new TRANSLATION_FAILS;
		if ( PN_UNLIKELY(s_facilityTranslationFails == nullptr) )
			s_facilityTranslationFails = new TRANSLATION_FAILS;
	}

	const size_t cpSize = header.numCPs * sizeof(CP);
	const size_t facilitySize = header.numFacilities * sizeof(FACILITY);
	const size_t buildingSize = header.numBuildings * sizeof(BUILDING);
	const size_t depotSize = header.numLinks * sizeof(DEPOT_LINK);
	const size_t waypointSize = header.numWaypoints * sizeof(WAYPOINT);
	const size_t dataSize = cpSize + facilitySize + buildingSize + depotSize + waypointSize;
	if ( PN_UNLIKELY(dataSize == 0) )
	{
		fioClose(fd);
		return 0;
	}
	void* data = currentData = memAlloc(dataSize);

	size_t dataRead = fioRead(fd, data, dataSize);
	fioClose(fd);
	if (dataRead != dataSize)
	{
		sdmCloseFile();
		return SDMFILE_READ_ERR;
	}

	if (header.numCPs)
	{
		cpList = static_cast<CP*>(data);
		data = cpList + header.numCPs;

		for(SINT32 _CPindex=0; _CPindex < header.numCPs; ++_CPindex)
		{
			CP& cp = cpList[_CPindex];

			///OGS:06/22/07: Allow code to supersede SDM files as to
			///which side a country indicates.
			cp.country = wwiiAdjustedCountry(cp.country);
			cp.side = wwiiSide(static_cast<WWII_COUNTRY>(cp.country));

			_applyTranslationsToName(cp.name, s_cpTranslationFails, /*basicOnly=*/false);

			maxId = (maxId >= cp.oid.id) ? maxId : cp.oid.id;
		}
	}

	if (header.numFacilities)
	{
		facilityList = static_cast<FACILITY*>(data);
		data = facilityList + header.numFacilities;

		for(SINT32 _FACindex = 0; _FACindex < header.numFacilities; ++_FACindex)
		{
			FACILITY& fac = facilityList[_FACindex];

			///OGS:06/22/07: Allow code to supersede SDM files as to
			///which side a country indicates.
			fac.originalCountry = wwiiAdjustedCountry(fac.originalCountry);
			fac.originalSide = wwiiSide(static_cast<WWII_COUNTRY>(fac.originalCountry));

			_applyTranslationsToName(fac.name, s_facilityTranslationFails, /*basicOnly=*/false);

			maxId = (maxId >= fac.oid.id) ? maxId : fac.oid.id;
		}
	}

	if (header.numBuildings)
	{
		buildingList = static_cast<BUILDING*>(data);
		data = buildingList + header.numBuildings;

		for(SINT32 _BUILDindex=0; _BUILDindex < header.numBuildings; ++_BUILDindex)
		{
			BUILDING& building = buildingList[_BUILDindex];

			_applyTranslationsToName(building.name, nullptr, /*basicOnly=*/true);

			maxId = (maxId >= building.oid.id) ? maxId : building.oid.id;
		}
	}

	if (header.numLinks)
	{
		linkList = static_cast<DEPOT_LINK*>(data);
		data = linkList + header.numLinks;

		for(SINT32 _DEPOTindex=0; _DEPOTindex < header.numLinks; ++_DEPOTindex)
		{
			DEPOT_LINK& link = linkList[_DEPOTindex];

			_applyTranslationsToName(link.name, nullptr, /*basicOnly=*/true);

			maxId = (maxId >= link.oid.id) ? maxId : link.oid.id;
		}
	}

	if (header.numWaypoints)
	{
		waypointList = static_cast<WAYPOINT*>(data);
		data = waypointList + header.numWaypoints;

		for(SINT32 _WAYPOINTindex=0; _WAYPOINTindex < header.numWaypoints; ++_WAYPOINTindex)
		{
			WAYPOINT& waypoint = waypointList[_WAYPOINTindex];

			maxId = (maxId >= waypoint.oid.id) ? maxId : waypoint.oid.id;
		}

	}

	return 0;
}

SINT32 sdmCloseFile(void)
{
	header.numCPs = currentCP = 0;
	header.numFacilities = currentFacility  = 0;
	header.numBuildings = currentBuilding = 0;
	header.numLinks = currentLink = 0;
	header.numWaypoints = currentWaypoint = 0;

	cpList = nullptr;
	facilityList = nullptr;
	buildingList = nullptr;
	linkList = nullptr;
	waypointList = nullptr;

	if (currentData)
	{
		memFree(currentData);
		currentData = nullptr;
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Check to see if we had any fatal errors and report them now.

void
sdmFinish()
{
	bool failed = false;

	if ( s_cpTranslationFails )
	{
		if ( _reportTranslationFails("CPs", *s_cpTranslationFails) )
			failed = true;
		delete s_cpTranslationFails;
		s_cpTranslationFails = nullptr;
	}

	if ( s_facilityTranslationFails )
	{
		if ( _reportTranslationFails("Facilities", *s_facilityTranslationFails) )
			failed = true;
		delete s_facilityTranslationFails;
		s_facilityTranslationFails = nullptr;
	}

	if ( failed )
	{
		LogAbort("Aborting due to errors");
	}
}

UINT16 sdmGetNumCP(void)
{
	return header.numCPs;
}

CP* sdmGetNextCP(void)
{
	if (cpList == nullptr || currentCP >= header.numCPs)
		return nullptr;
	return &cpList[currentCP++];
}

CP* sdmGetCPArray(void)
{
    return cpList;
}

UINT16 sdmGetNumFACILITY(void)
{
	return header.numFacilities;
}

FACILITY* sdmGetNextFACILITY(void)
{
	if (facilityList == nullptr || currentFacility >= header.numFacilities)
		return nullptr;
	return &facilityList[currentFacility++];
}

FACILITY* sdmGetFACILITYArray(void)
{
	return facilityList;
}

UINT16 sdmGetNumBUILDING(void)
{
	return header.numBuildings;
}

BUILDING* sdmGetNextBUILDING(void)
{
	if (PN_LIKELY(buildingList != nullptr && currentBuilding < header.numBuildings))
		return &buildingList[currentBuilding++];
	return nullptr;
}

BUILDING* sdmGetBUILDINGArray(void)
{
	return buildingList;
}

UINT16 sdmGetNumDEPOT_LINK(void)
{
	return header.numLinks;
}

DEPOT_LINK* sdmGetNextDEPOT_LINK(void)
{
	if (linkList == nullptr || currentLink >= header.numLinks)
		return nullptr;
	return &linkList[currentLink++];
}

UINT16 sdmGetNumWAYPOINT(void)
{
	return header.numWaypoints;
}

WAYPOINT* sdmGetNextWAYPOINT(void)
{
	if (waypointList == nullptr || currentWaypoint >= header.numWaypoints)
		return nullptr;
	return &waypointList[currentWaypoint++];
}

UINT32 sdmGetTotalStratObjects(void)
{
	return header.numCPs + header.numFacilities + header.numBuildings + header.numLinks + header.numWaypoints;
}

UINT32 sdmGetMaxStratId(void)
{
	return maxId;
}

SINT32 _getTerrainFileLength(SINT32  fp)
{
	TERRAINFILE tf;

	fioRead(fp, &tf, sizeof(TERRAINFILE));

	return tf.length;
}

const std::string&
sdmTranslateName(const std::string& from)
{
	TRANSLATIONS::iterator it = s_translations.find(from);
	return ( it == s_translations.end() ) ? from : it->second;
}

