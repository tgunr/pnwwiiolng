#include <syscfg.h> // Path is project specific.
// Created: Mon Sep  5 14:32:34 PDT 2016
//
// Capture errors from the strat system
//
// Copyright (c) 2016 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Oliver "kfsone" Smith

//#include "map/mapConstants.h"
#include "parse/parseWor.h"
//#include "sdm/sdmConfigVars.h"
//#include "strt/strtCP.h"
//#include "strt/strtCPStruct.h"
//#include "strt/strtData.h"
//#include "strt/strtDB.h"
//#include "strt/strtFacilityStruct.h"
//#include "strt/strtFB.h"
//#include "strt/strtHostDB.h"
//#include "strt/strtLinkStruct.h"
//#include "strt/strtObjectStruct.h"
//#include "strt/strtServerTypes.h"
//#include "strt/strtSpawn.h"
//#include "strt/strtSupercell.h"
//#include "strt/strtSupercells.h"
//#include "strt/strtTypes.h"
//#include "strt/strtWeaponUpdate.h"
//#include "teul/teulTime.h"
//#include "weap/weapSpawnXML.h"
//#include "wwii/wwiiConfig.h"
#include "strt/strtDataErrors.h"
#include "strt/strtDataErrorInfo.h"

ErrorInfo errorInfo[MAX_ERR_CODES] =
{
	{
		"wrong-supercell"
		, "The following entities have been displaced from their original SUPERCELL during the BuildTerrain process. This will prevent the building from participating in strat (e.g. finding its parent facility)"
		, 1
		, false
	}
,	{
		"no-parent-fac"
		, "The following BUILDINGs referred to FACILITYs that could not be found - this can caused by placing a building in a different supercell to their parent facility or when a building instance has become detached from its strat data."
		, 1
		, false
	}
,	{
		"link-start-mult"
		, "The following link-start depots were associated with multiple links!"
		, 1
		, false
	}
,	{
		"no-cap-table"
		, "The following facilities did not contain any capturable buildings. Capturable state is determined by the word 'cap' in the flags (11th) column of the building's entry in blitzWor.csv. Can occur if the host's blitzWor is out of data."
		, 1
		, true
	}
,	{
		"zero-links"
		, "Some DEPOT faclities did not have any LINKs (incoming OR outgoing) - this is normally the case for edge-of-map depots"
		, 0
		, true
	}
,	{
		"zero-in-links"
		, "Some DEPOT facilities had outgoing LINKs but were missing /incoming/ LINKs - this is a CRITICAL data error"
		, 0
		, false
	}
,	{
		"zero-out-links"
		, "Some DEPOT facilities had incoming LINKs but were missing /outgoing/ LINKs - this is a CRITICAL data error"
		, 0
		, false
	}
,	{
		"mismatch-links"
		, "Some DEPOT facilities did not have a matching number of incoming vs outgoing LINKs - the code depends on reciprocating and matching LINKs, so this is a CRITICAL data error"
		, 0
		, false
	}
,	{
		"null-in-link"
		, "Some DEPOT facilities had a NULL incoming LINK - this is a CRITICAL data error since code assumes valid LINKs"
		, 0
		, false
	}
,	{
		"null-out-link"
		, "Some DEPOT facilities had a NULL outgoing LINK - this is a CRITICAL data error since code assumes valid LINKs"
		, 0
		, false
	}
,	{
		"fb-both-check"
		, "Some firebases do not have spawn objects for* both* infantry and ground units"
		, 0
		, true
	}
,	{
		"fb-dmg-spawn"
		, "Some firebases had a damageable spawn object - this is a SIGNIFICANT data error"
		, 0
		, false
	}
,	{
		"fb-divisor"
		, "The following firebases have an object count which does not divide evenly into 100 - this is a MAJOR data error"
		, 0
		, false
	}
,	{
		"fb-no-link"
		, "The following firebases do not appear to have a LINK associated with them - this is potentially a SIGNIFICANT data error"
		, 0
		, false
	}
,	{
		"fb-no-mate"
		, "These firebases did not appear to be accompanied by a reciprocating 'mate' fb - this is a CRITICAL data error, since the code assumes all Firebases are paired"
		, 0
		, false
	}
,	{
		"ab-both-check"
		, "The following ARMYBASE facilities do not have INFANTRY and VEHICLE spawn objects - this is normal for newer CPs and the Zeelands. The validator does /not/ read spawn.xml"
		, 0
		, true
	}
,	{
		"af-no-spawn"
		, "These AIRFIELD facilities do not have an AIRCRAFT spawn object"
		, 0
		, true
	}
,	{
		"cp-no-caps"
		, "The following CPs did not have any non-firebase facilities associated with them - this should never happen to a CP in the playspace"
		, 1
		, true
	}
,	{
		"cp-defined-contested"
		, "These CPs have member facilities that are defined in data as belonging to a different side than the CP itself. This can causes unpredictable behavior when the strat server starts up."
		, 1
		, true
	}
,	{
		"obj-unused"
		, "The following buildings are objects that are not described in blitzWor. If the object is no-longer used, please add an \"Ignore,objectname,obj-unused\" entry to blitzWor.csv"
		, 0
		, true
	}
,	{
		"link-no-bldg"
		, "The following LINKs referenced buildings that could not be found in the specified supercell. (N,N:N is ox,oy:ID)"
		, 0
		, false
	}
,	{
		"link-bad-start"
		, "The following LINKs found a startDepot with a facility who's outgoingLink is NULL"
		, 0
		, false
	}
,	{
		"link-bad-end"
		, "The following LINKs found an endDepot with a facility who's incomingLink is NULL"
		, 0
		, false
	}
,	{
		"link-bad-fb"
		, "The following links had invalid firebase IDs that could not be found."
		, 0
		, false
	}
,	{
		"fb-no-bldg"
		, "The following firebases appear to have no buildings/objects associated with them."
		, 0
		, false
	}
,	{
		"fac-mult-cap"
		, "The following BUILDINGs conflicted with an existing capture object for their parent facility."
		, 1
		, true
	}
};

DataErrors g_dataErrors;

//////////////////////////////////////////////////////////////////////
// Add an error list entry for an object, if it isn't ignoring the
// given error.

void strtAddErrorListEntry(int errorNo, const char* objectName, const char* entryName, const char* entryData)
{
	if ( errorInfo[errorNo].canIgnore && parseIsObjectIgnoringError(objectName, errorInfo[errorNo].name) )
		return;

	g_dataErrors.emplace(errorNo, entryName, entryData);
}

//////////////////////////////////////////////////////////////////////
// Validity checks on the supply links in a particular supercell

void
strtDataCantFindBuilding(const char* linkName, const char* role, FULL_OBJ_ID fid)
{
	char description[256];
	strprintf(description, "%s (%d,%d:%u)", role, fid.cell.ox, fid.cell.oy, fid.id);
	g_dataErrors.emplace(ERR_CANT_FIND_LINK_BUILDING, linkName, description);
}

//////////////////////////////////////////////////////////////////////
// Report an error finding the parent facility for a building

void
strtDataErrorParentFacility(const char* buildingName, const char* filename, SINT32 scx, SINT32 scy, UINT16 bid, UINT16 bno, UINT16 fid)
{
	char description[256];
	strprintf(description, "%s sc:%d,%d bid:%d bldg:%d fid:%d"
				, filename, scx, scy, bid, bno, fid);
	g_dataErrors.emplace(ERR_CANT_FIND_PARENT_FACILITY, buildingName, description);
}

//////////////////////////////////////////////////////////////////////
// Dump the current list of strat errors.

UINT32
strtReportDataErrors(UINT32 level)
{
	UINT32 errors = 0;

//	const UINT32 errorLimit = cv_stratErrorLimit().GetUint32();
	for ( UINT32 i = 0; i < MAX_ERR_CODES; ++i )
	{
		if ( level < errorInfo[i].level )
			continue;
		auto errorMap = g_dataErrors[i];
		if ( errorMap.empty() )
			continue;

		dbugLog(_FLI, "\n============================================================================\n"
						"[%s] "
						"%s"
						"\n============================================================================\n"
						, (errorInfo[i].canIgnore ? errorInfo[i].name : "CRITICAL")
						, errorInfo[i].text);

		std::string list = "";
		std::string detail = "";
		UINT32 reportNo = 0;
		for ( auto it = errorMap.cbegin(); it != errorMap.cend(); ++it, ++reportNo )
		{
			if ( list.empty() == false )
				list += ", ";

//			if ( errorLimit > 0 && level > 0 && errorMap.size() > errorLimit + 10 && reportNo >= errorLimit )
//			{
//				char conclusion[80];
//				sprintf(conclusion, "{ %u more occurances skipped }", (UINT32)(errorMap.size() - reportNo));
//				list += conclusion;
//				break;
//			}

			detail = it->first;
			if ( it->second.empty() == false )
			{
				detail += " [";
				detail += it->second.c_str();
				detail += "]";
			}

			if ( list.size() + detail.size() > 77 )
			{
				dbugLog(_FLA, "  %s", list.c_str());
				list = "";
			}

			list += detail;

			if ( errorInfo[i].canIgnore == false )
				++errors;
		}

		if ( list.empty() == false )
			dbugLog(_FLA, "  %s", list.c_str());

		dbugLog(_FLI, "\n");
	}

	return errors;
}
