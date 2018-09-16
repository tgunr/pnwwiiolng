#include <syscfg.h> // Path is project specific.
// Created: Thu Jun  1 13:45:40 2000
//
// Encapsulation of the strat system data. This allows me to have quick
// access to the pointers for all these strat system objects without having
// to mess with direct access stuff.
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson
// Supporting Programmer:  Marty "Vandal" Poulin

#include "map/mapConstants.h"
#include "parse/parseWor.h"
#include "include/configVars.h"
#include "strt/strtCP.h"
#include "strt/strtCPStruct.h"
#include "strt/strtData.h"
#include "strt/strtDataErrorInfo.h"
#include "strt/strtDataErrors.h"
#include "strt/strtDB.h"
#include "strt/strtFacilityStruct.h"
#include "strt/strtFB.h"
#include "strt/strtHostDB.h"
#include "strt/strtLinkStruct.h"
#include "strt/strtObjectStruct.h"
#include "strt/strtServerTypes.h"
#include "strt/strtSpawn.h"
#include "strt/strtSupercell.h"
#include "strt/strtSupercells.h"
#include "strt/strtTypes.h"
#include "strt/strtWeaponUpdate.h"
#include "teul/teulTime.h"
#include "weap/weapSpawnXML.h"
#include "wwii/wwiiConfig.h"
#include "fio/fiod.h"                                           // for fiodGetPrepath()

static SpawnLoader spawnLoader;

namespace Strat
{
	CPStruct*	cpList;
}

static UINT32 totalCPs;

static UINT16 uMaxFacilityOID = 0;

//local function to verify capture and spawn objects
static bool _verifySubtypes(FacilityStruct* facility);

//More helper functions
static void _calcCapacity(ObjectStruct& object);
static void _checkCPStatus(CPStruct* cpList);

// Bitmask flags for capture objectives
const UINT32 attackFlags[MAX_SIDES] = { 0, CP_STAT_ALLIED_ATTACK_OBJ, CP_STAT_AXIS_ATTACK_OBJ, 0 };
const UINT32 defendFlags[MAX_SIDES] = { 0, CP_STAT_ALLIED_DEFEND_OBJ, CP_STAT_AXIS_DEFEND_OBJ, 0 };
const UINT32 objectiveFlags[MAX_SIDES] = {
	CP_OBJECTIVES
,	CP_ALLIED_OBJECTIVES
,	CP_AXIS_OBJECTIVES
,	0
};

//////////////////////////////////////////////////////////////////////
// Filename string pool definitions

using FilenamePool = std::unordered_map<UINT64, std::vector<std::unique_ptr<std::string>>>;
FilenamePool g_filenamePool;

//////////////////////////////////////////////////////////////////////
// Code that provides a shared memory of filename spaces, rather than using
// 128 bytes per cp/facility/building/link/etc for the same names

static const std::string*
_filenamePool(const char* filename)
{
	auto hash = ConstCharHasher()(filename);
	auto& names = g_filenamePool[hash];
	auto it = names.begin();
	for (auto end = names.end(); it != end; ++it)
	{
		if (strcmp((*it)->c_str(), filename) == 0)
			return it->get();
	}
	names.emplace_back(new std::string(filename));
	return names.back().get();
}

UINT32
strtNumCPs()
{
	return totalCPs;
}

void*
g_malloc0(size_t size, int units=1)
{
	return calloc(size, units);
}

//////////////////////////////////////////////////////////////////////
// Determine if this link has an active enemy firebase for side "forSide"

bool
LinkStruct::hasEnemyFirebase(WWII_SIDE forSide) const
{
	if ( firebase == nullptr )
		return false;
	if ( firebase->isOpen() && firebase->side != forSide )
		return true;
	LinkStruct* other = endDepot->m_outgoingLink;
	if ( other == nullptr || other->firebase == nullptr || !other->firebase->isOpen() )
		return false;
	return other->firebase->side != forSide;
}

//////////////////////////////////////////////////////////////////////
// Flag toggles

void
strtEnemyLink(CPStruct& cp, bool state)
{
	cp.m_enemyLinked = state;
	if ( state )
		cp.flags |= CP_STAT_ENEMY_LINKED;
	else
	{
#ifdef IS_STRAT_HOST
		WWII_SIDE attacker = cp.attacker();
		if ( cp.isCity() && attacker != WWII_NO_SIDE && !cp.isContested() )
		{
			cp.clearObjective(attacker);
		}
#endif

		cp.flags &= ~CP_STAT_ENEMY_LINKED;
	}
}

static inline void
_checkSupplyLinksIn(const Supercell& sc)
{
	const LinkStruct* newLink;
	const ObjectStruct* bldg;

	for ( UINT32 counter = 0; counter < sc.numLinks; ++counter )
	{
		newLink = &sc.links[counter];

		bldg = sc.Get<ObjectStruct>(newLink->startDepotID.id);
		if ( bldg != nullptr )
		{
			if (bldg->facility == nullptr)	// problem raised elsewhere
				continue;
			const FacilityStruct& facility = bldg->Facility();
			if ( facility.m_outgoingLink == nullptr)
			{
				char buffer[256];
				strprintf(buffer, "%d,%d:%d -> %s"
								, newLink->fullObjID.cell.ox
								, newLink->fullObjID.cell.oy
								, newLink->fullObjID.id
								, facility.name);
				g_dataErrors.emplace(ERR_LINK_START_DEPOT_LINK, newLink->name, buffer);
			}
		}
		else
		{
			strtDataCantFindBuilding(newLink->name, "start", newLink->startDepotID);
		}

		// end depot is unlikely to be in the same sc.
		bldg = strtGetObject(newLink->endDepotID);
		if ( bldg != nullptr )
		{
			if (bldg->facility == nullptr)	// problem raised elsewhere
				continue;
			const FacilityStruct& facility = bldg->Facility();
			if ( facility.m_incomingLink == nullptr)
			{
				char buffer[256];
				strprintf(buffer, "(%d,%d:%d -> %s)"
								, newLink->fullObjID.cell.ox
								, newLink->fullObjID.cell.oy
								, newLink->fullObjID.id
								, facility.name);
				g_dataErrors.emplace(ERR_LINK_END_DEPOT_LINK, newLink->name, buffer);
			}
		}
		else
		{
			strtDataCantFindBuilding(newLink->name, "end", newLink->endDepotID);
		}

	}
}

//////////////////////////////////////////////////////////////////////
// Check that all the links are properly anchored and bi-directional

static void
_checkSupplyLinks()
{
	for ( auto it = g_supercells.begin(), end = g_supercells.end(); it != end; ++it )
	{
		_checkSupplyLinksIn(it->second);
	}
}

//////////////////////////////////////////////////////////////////////
//

bool ObjectStruct::isProductionCenter() const
{
	return ( (type == BUILDING_RESEARCH)
				&& *(const UINT32*)(filename) == *(const UINT32*)("fact")	// 4-byte string comparison
				&& (facility->type == FACILITY_FACTORY)
				&& (facility->subType == FACTORY_VEHICLE)
				);
}

//////////////////////////////////////////////////////////////////////
// Caution: This gets called from both CP initialization and facility
// initialization. If the supercell is nullptr, then we are forward-
// referencing a CP in a supercell that hasn't been reached yet.
// However, during our forward reference, we will establish a
// facilityList for the CP. Only when both of these values are nullptr
// do we know that we need to initialize the values of the CP struct

static void
_ensureCPInitialization(CPStruct* cp)
{
	if ( !(cp->facilityList == nullptr && cp->supercell == nullptr) )
		return;

	cp->ownableFacilities = 0;
	cp->numArmyBases = 0;
	cp->numFirebases = 0;
	memset(cp->facilitiesOwned, 0, sizeof(cp->facilitiesOwned));
	memset(cp->armyBasesOwned, 0, sizeof(cp->armyBasesOwned));
}

//////////////////////////////////////////////////////////////////////
// Compare the distance between two different facilities

static UINT32
_facilityDistance(const FacilityStruct* facil1, const FacilityStruct* facil2)
{
	if ( facil1 == nullptr || facil2 == nullptr )
	{
		dbugLog(_FLA, "null facility pointer!");
		return 0xffffffff;				// A very long distance
	}

	// m_posX and m_posY should be the full X/Y of the facility;
	// previous versions of the code thought they were
	// supercell relative.
	SINT64 xDist = facil1->m_posX - facil2->m_posX;
	SINT64 yDist = facil1->m_posY - facil2->m_posY;

	return (UINT32)sqrtl((double)(xDist * xDist) + (double)(yDist * yDist));
}


//////////////////////////////////////////////////////////////////////
// Initialize the strat environment
// Builds indices on all strat items and resolves pointers on all objects

void
_resetAllObjects()
{
	for ( auto it = g_supercells.begin(), end = g_supercells.end(); it != end; ++it )
	{
		Supercell& sc = it->second;
		for ( int counter = 0; counter < sc.numObjects; ++counter )
		{
			strtResetObject(sc.objects[counter]);
		}
	}
}

//////////////////////////////////////////////////////////////////////
// Check if an entity is inside the correct supercell

static bool
_supercellCheck(const char* typeName, const char* name, const Supercell& sc, int posX, int posY)
{
	int actualScx = posX / SUPERCELL_SIZE_M;
	int actualScy = posY / SUPERCELL_SIZE_M;

	if ( PN_LIKELY(actualScx == sc.x && actualScy == sc.y) )
		return true;

	char entity[256], expected[256];
	strprintf(entity, "%s '%s' from %d.%d", typeName, name, sc.x, sc.y);
	strprintf(expected, "sdm puts it in %d.%d", actualScx, actualScy);

	g_dataErrors.emplace(ERR_WRONG_SUPERCELL, entity, expected);

	return false;
}

//////////////////////////////////////////////////////////////////////
// First pass initialization of CPs
//  - Establish Strat::cpList prev/next/head linkage
//	- Make sure essential base variables are zeroed
//	- Link the CP to its supercell

static void
_scPass1_cps(Supercell& sc)
{
	for ( UINT32 counter = 0; counter < sc.numCPs; ++counter )
	{
		CPStruct* newCP = &sc.cps[counter];

		_supercellCheck("CP", newCP->name, sc, newCP->m_posX, newCP->m_posY);

		if ( Strat::cpList )
			Strat::cpList->prev = newCP;
		newCP->next = Strat::cpList;
		Strat::cpList = newCP;
		newCP->prev = nullptr;

		if (!strtInitCPfromWOR(*newCP))
		{
			dbugLog(_FLA,"couldn't initialize CP %s from WOR file", newCP->name);
			continue;
		}

		// Make sure the CP is initialized correctly if it hasn't
		// already been visited by a facility forward reference
		_ensureCPInitialization(newCP);
		newCP->supercell = &sc;
	}
}

//////////////////////////////////////////////////////////////////////
// First pass initialization of Facilities
//  - Make sure the necessary parent CP is initialized (its valid
//    for a facility to exist in a different SC than its CP, e.g.
//    firebases)
//  - Link to its supercell
//  - Check that the parentCP is known
//	- Add to the parent CP next and typeNext linkages
//	- Adjust parentCP stats
//	- Initialize initial values

static void
_scPass1_facs(Supercell& sc)
{
	for ( UINT32 counter = 0; counter < sc.numFacilities; ++counter )
	{
		FacilityStruct* newFacility = &sc.facilities[counter];

		_supercellCheck("FAC", newFacility->name, sc, newFacility->m_posX, newFacility->m_posY);

		// facilities aren't required to be in the same sc; e.g. firebases.
		CPStruct* parentCP = strtGetCP(newFacility->cpID);
		if (parentCP == nullptr)
		{
			dbugLog(_FLA,"Supercell %d:%d: couldn't find parent CP %d for facility %d %s"
						, sc.x, sc.y
						, newFacility->cpID.id
						, newFacility->fullObjID.id
						, newFacility->name);
			continue;
		}

		_ensureCPInitialization(parentCP);

		if ( newFacility->supercell != nullptr )
			dbugLog(_FLW, "reinitialization of facility %s in supercell %04hx/%04hx"
							, newFacility->name, sc.x, sc.y);

		newFacility->supercell = &sc;

		if ( !strtInitFacilityfromWOR(*newFacility) )
		{
			dbugLog(_FLA,"couldn't initialize facility %s from WOR file", newFacility->name);
			continue;
		}

		newFacility->cp = parentCP;
		newFacility->next = parentCP->facilityList;

		// Add to the CPs by-type facility indexes
		newFacility->typeNext = parentCP->facilityTypeIndex[newFacility->type];
		parentCP->facilityTypeIndex[newFacility->type] = newFacility;

		if (newFacility->type == FACILITY_ARMYBASE)
		{
			// We track the # of army bases so we can track control of the CP
			parentCP->armyBasesOwned[newFacility->side]++;
			parentCP->numArmyBases++;
		}

		if (newFacility->type == FACILITY_FIREBASE)
		{
			//Increment the number of Firebases the Facility has
			parentCP->numFirebases++;
		}
		else
		{
			parentCP->ownableFacilities++;
			if (newFacility->type == FACILITY_NAVALBASE)
				parentCP->flags |= CP_STAT_HAS_NAVYBASE;
			if (newFacility->type == FACILITY_AIRBASE)
				parentCP->flags |= CP_STAT_HAS_AIRBASE;

			parentCP->facilitiesOwned[newFacility->side] ++;
		}
		parentCP->facilityList = newFacility;

		//Initialize the capacity for the facility
		newFacility->capacity = newFacility->maxCapacity();

		newFacility->m_captureBuilding = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////
// First pass initialization of building objects
//  - Make sure the necessary parent CP is initialized (see pass1 facs)
//	- Add parent linkage
//	- Update parent stats
//	- [Add production centers to the production queue]

static void
_scPass1_objs(Supercell& sc)
{
	for ( UINT32 counter = 0; counter < sc.numObjects; ++counter )
	{
		ObjectStruct* newObject = &sc.objects[counter];

		_supercellCheck("BLDG", newObject->name, sc, newObject->posX, newObject->posY);

		// facility might possibly be in a different supercell to us.
		FacilityStruct* parentFacility = strtGetFacility(newObject->facilityID);
		if (parentFacility == nullptr)
		{
			strtDataErrorParentFacility(newObject->name, newObject->filename, sc.x, sc.y, newObject->fullObjID.id, counter, newObject->facilityID.id);
			continue;
		}

		if ( !strtInitObjectfromWOR(*newObject) )
		{
			strtAddErrorListEntry(ERR_OBJ_NOT_IN_BLITZWOR, newObject->filename, newObject->name, newObject->filename);
			continue;
		}

		if ( newObject->m_isCaptureBuilding != 0 )
		{
			if ( parentFacility->m_captureBuilding && parentFacility->m_captureBuilding != newObject )
			{
				char error[512];
				strprintf(error, "type:%s vs %s type:%s"
								, newObject->filename
								, parentFacility->m_captureBuilding->name
								, parentFacility->m_captureBuilding->filename
								);
				g_dataErrors.emplace(ERR_MULT_CAP_OBJECTS, newObject->name, error);
			}
			else
			{
				parentFacility->m_captureBuilding = newObject;
			}
		}

		newObject->supercell = &sc;
		newObject->facility = parentFacility;
		newObject->next = parentFacility->objectList;
		// try to not increment the number of objects a firebase has
		// if it's a spawn object
		if( newObject->type != BUILDING_SPAWN || parentFacility->type!=FACILITY_FIREBASE)
			parentFacility->numObjects++;
		//else
			//dbugLog(_FLN,"Ignoring invisible spawn portal %s",newObject->name);

		if (newObject->type == BUILDING_SPAWN)
			parentFacility->m_spawnable = true;

		parentFacility->objectList = newObject;

		newObject->setCountry(parentFacility->country, parentFacility->side);

#ifdef IS_STRAT_HOST
		// Is it a production center?
		if ( newObject->isProductionCenter() )
			strtAddFactory(newObject);
#endif // IS_STRAT_HOST
	}
}

//////////////////////////////////////////////////////////////////////
// Go through all the basic strat objects and ensure they are base-
// initialized and establish all the basic object indexes. Also pull
// baseline data from the strat database

static void
_supercellsPass1()
{
#if defined(IS_STRAT_HOST) || defined(IS_MAP_HOST)
	dbugLog(_FLI, "Loading data from database");
	strtdbInitSupercells();
#endif

	for ( auto it = g_supercells.begin(), end = g_supercells.end(); it != end; ++it )
	{
		totalCPs += it->second.numCPs;

		_scPass1_cps(it->second);
		_scPass1_facs(it->second);
		_scPass1_objs(it->second);
	}
}

//////////////////////////////////////////////////////////////////////
// Link initialization

static void
_scPass2_linkSetup(Supercell& sc)
{
	for ( UINT32 counter = 0; counter < sc.numLinks; ++counter )
	{
		LinkStruct* newLink = &sc.links[counter];

		ObjectStruct* startBldg = sc.Get<ObjectStruct>(newLink->startDepotID.id);
		if ( startBldg == nullptr )
		{
			strtDataCantFindBuilding(newLink->name, "start", newLink->startDepotID);
			continue;
		}

		FacilityStruct* start = startBldg->facility;
		if ( start == nullptr )
		{
			dbugLog(_FLA,"Supercell %d:%d: couldn't find start facility %d for Link %d %s", sc.x, sc.y, newLink->startDepotID.id, counter, newLink->name);
			continue;
		}
		if ( start->cp == nullptr )
		{
			dbugLog(_FLA,"Supercell %d:%d: couldn't find start cp %d for Link %d %s", sc.x, sc.y, newLink->startDepotID.id, counter, newLink->name);
			continue;
		}

		// end building can be in a different supercell.
		ObjectStruct* endBldg = strtGetObject(newLink->endDepotID);
		if ( endBldg == nullptr )
		{
			strtDataCantFindBuilding(newLink->name, "end", newLink->endDepotID);
			continue;
		}

		FacilityStruct* end = endBldg->facility;
		if ( end == nullptr )
		{
			dbugLog(_FLA,"Supercell %d:%d: couldn't find end facility %d for Link %d %s", sc.x, sc.y, newLink->endDepotID.id, counter, newLink->name);
			continue;
		}
		if ( end->cp == nullptr )
		{
			dbugLog(_FLA,"Supercell %d:%d: couldn't find end cp %d for Link %d %s", sc.x, sc.y, newLink->endDepotID.id, counter, newLink->name);
			continue;
		}

		if ( start->m_outgoingLink != nullptr )
		{
			std::string links = start->m_outgoingLink->name;
			links += "/";
			links += newLink->name;
			g_dataErrors.emplace(ERR_LINK_START_MULTIPLE_LINKS, start->name, std::move(links));
			continue;
		}
		if ( end->m_incomingLink != nullptr )
		{
			dbugLog(_FLA, "link-end %s is associated with multiple links", end->name);
			continue;
		}

		newLink->startDepot = start;
		start->m_outgoingLink = newLink;

		newLink->endDepot = end;
		end->m_incomingLink = newLink;

		start->cp->m_links.insert(newLink);

		// Link up the firebase if we have one. Can easily be in a different supercell.
		FacilityStruct* firebase = strtGetFacility(newLink->firebaseID);
		if ( firebase == nullptr )
		{
			if ( newLink->firebaseID == nullObjID )
				continue;
			char buffer[256];
			strprintf(buffer, "%d,%d:%u", newLink->firebaseID.cell.ox, newLink->firebaseID.cell.oy, newLink->firebaseID.id);
			g_dataErrors.emplace(ERR_LINK_FIREBASE, newLink->name, buffer);
			continue;
		}

		firebase->link = newLink;
		newLink->firebase = firebase;
#ifdef HEAVY_DEBUG
        dbugLog(_FLD,"link %s to %s " PRF_FULL_OBJ_ID " is associated with firebase %s " PRF_FULL_OBJ_ID ""
                , start->name, end->name, PRA_FULL_OBJ_ID(newLink->fullObjID)
                , firebase->name, PRA_FULL_OBJ_ID(firebase->fullObjID)
                );
#endif // HEAVY_DEBUG

		if (!strtInitLinkfromWOR(*newLink))
		{
			dbugLog(_FLA,"couldn't initialize link %s from WOR file", newLink->name);
			return;
		}
	}
}

//////////////////////////////////////////////////////////////////////
// Now everything is linked up nicely, we can start reading WOR file
// values

static void
_scPass2_cps(Supercell& sc)
{
	//CP initialization stuff
	for ( UINT32 counter = 0; counter < sc.numCPs; ++counter )
	{
		CPStruct& cp = sc.cps[counter];

		cp.m_isSupplyCenter = false;
#ifdef IS_STRAT_HOST
		cp.m_isRouteingConnected = false;
		cp.m_routeingRun = 0ULL;
		cp.flags &= ~CP_STAT_IN_SUPPLY;
#endif
	}
}

//////////////////////////////////////////////////////////////////////
// Load WOR data into the prepared facilities

static void
_scPass2_facs(Supercell& sc)
{
	//Facility initialization
	for ( UINT32 counter = 0; counter < sc.numFacilities; ++counter )
	{
		FacilityStruct& fac = sc.facilities[counter];

		if ( fac.type != FACILITY_FIREBASE && fac.m_captureBuilding == nullptr
			&& strncmp(fac.name, "Camp ", 5) != 0
			&& strncmp(fac.name, "Fort ", 5) != 0
			&& strncmp(fac.name, "Ft ", 3) != 0
		   )
		{
			strtAddErrorListEntry(ERR_NO_CAPTURE_TABLE, fac.name, fac.name, (fac.cp ? fac.cp->name : "unknown"));
		}
	}
}

//////////////////////////////////////////////////////////////////////
// - Load WOR info into buildings
// - Propagate attributes up to facilities/cps

static void
_scPass2_objs(Supercell& sc)
{
	for ( UINT32 counter = 0; counter < sc.numObjects; ++counter )
	{
		//initialize each Object
		ObjectStruct& obj = sc.objects[counter];

		///CAVEAT: If we couldn't look the facility up in the
		/// first pass, we shouldn't assume it's there now
		if ( obj.isValid() == false )
			continue;

		WEAPON_SPEC spec = weapFindClass(obj.filename);
		if ( spec.wClass != 0 )
			obj.spawnAbility.weap = spec;

		if ( obj.spawnAbility.weap.wCategory > 0 || obj.spawnAbility.weap.wClass > 0 )
		{
			obj.type = BUILDING_SPAWN;
			obj.Facility().m_spawnable = true;

			// Flag which types of spawn this facility has
			switch (obj.spawnAbility.weap.wCategory)
			{
				case _INFANTRY:
					obj.Facility().m_spawnsInf = true;
				break;
				case _GROUND_VEHICLE:
					obj.Facility().m_spawnsVehicles = true;
				break;
				default:
				break;
			}
		}
		if ( obj.isProductionCenter() )
		{
			obj.Facility().CP().setSupplyCenter();
		}
	}
}

//////////////////////////////////////////////////////////////////////
// With everything else finalized, we can load the WOR info for the
// links

static void
_scPass2_links(Supercell& sc)
{
	for ( UINT32 counter = 0; counter < sc.numLinks; ++counter )
	{
		LinkStruct& link = sc.links[counter];
		if ( link.startDepot == nullptr )
		{
			// We couldn't initialize this link, so ignore it.
			continue;
		}
		link.distance = _facilityDistance(link.startDepot, link.endDepot);
	}
}


//////////////////////////////////////////////////////////////////////
// Second pass over the supercells. At this point all the basic object
// initialization has been done and we can start making assertions and
// we can also complete the link initialization in good confidence
// that all the elements of the links are indexed in the strat system

static void
_supercellsPass2()
{
	// Now that all other pointers have been resolved
	// we can do the links and do other initialization
	for ( auto it = g_supercells.begin(), end = g_supercells.end(); it != end; ++it )
	{
		_scPass2_linkSetup(it->second);
		_scPass2_cps(it->second);
		_scPass2_facs(it->second);
		_scPass2_objs(it->second);
		_scPass2_links(it->second);
	}
}

//////////////////////////////////////////////////////////////////////
// Now that everything everywhere has been initialized, it's time for
// candy: put the this saves link-crossing from always having to
// access the LinkStructure and makes seeing the other FB on a link
// a heck of a lot easier.
// firebase->link->endDepot->m_outgoingLink->firebase
// becomes
// firebase->mate

static void
_mate()
{
	for ( auto cp = Strat::cpList; cp != nullptr; cp = cp->next )
	{
		for ( auto facility = cp->facilityTypeIndex[FACILITY_DEPOT]
			; facility != nullptr
			; facility = facility->next
			)
		{
			LinkStruct* const link = facility->m_outgoingLink;
			if ( link )
			{
				facility->mate = link->endDepot;
				FacilityStruct* const mate = facility->m_incomingLink->firebase;
				if ( mate && link->firebase )
					link->firebase->mate = mate;
			}
		}
	}
}

static void
_checkCPLinks()
{
#ifdef CHECK_SUPPLY
	CPStruct* cp;
	FacilityStruct* facility;

	for ( cp = Strat::cpList; cp != nullptr; cp = cp->next )
	{
		cp->numLinks = 0;
		cp->numOpenLinks = 0;
		strtEnemyLink(*cp, false);

		for ( facility = cp->facilityTypeIndex[FACILITY_DEPOT]
			; facility != nullptr
			; facility = facility->next
			)
		{
			if ( facility->m_outgoingLink == nullptr || facility->m_incomingLink == nullptr )
				continue;

			if ( facility->subType != DEPOT_TRAIN)
				cp->numLinks++;

			const CPStruct* const otherCP = facility->m_outgoingLink->endDepot->cp;
			if( otherCP->ControllingSide() == cp->ControllingSide() )
			{
				facility->m_outgoingLink->setEnabled(true);
				facility->m_incomingLink->setEnabled(true);

				if ( facility->subType != DEPOT_TRAIN)
					cp->numOpenLinks++;
			}
			else
			{
				if (isValidSide(otherCP->side) && otherCP->country != WWII_CHINA)	// Russia is neutral-hostile
				{
					strtEnemyLink(*cp, true);
					cp->m_townstatus = FRONT_LINE;
				}

				facility->m_outgoingLink->setEnabled(false);
				facility->m_incomingLink->setEnabled(false);
			}
		}

		// If the facility has no open links, but IS linked to an enemy CP, then it must be out of supply
		cp->updateFriendlyLinkStatus();
		cp->processFirebases(cp->isContested());

		for ( facility=cp->facilityList; facility != nullptr; facility=facility->next)
		{
			facility->setCaptureDisabled();
		}
	}

	dbugLog(_FLN,"finished cp link inits for %u cps", totalCPs);
#endif
}

static void
_setTownStatus()
{
	SINT64 totalBL = 0;
	SINT64 totalFL = 0;
	SINT64 totalRL = 0;
	SINT64 totalIN = 0;
	SINT64 totalChecked = 0;
	CPStruct* cp;
//	FacilityStruct* facility;

	for (cp = Strat::cpList; cp != nullptr; cp = cp->next)
	{
		bool hasAirOrNavalbase = false;
		totalChecked++;

		//Default status is INACTIVE. Set status to REAR_LINE if there is an air base or a naval base
		if (cp->facilityTypeIndex[FACILITY_AIRBASE] != nullptr)
			hasAirOrNavalbase = true;
		if (cp->facilityTypeIndex[FACILITY_NAVALBASE] != nullptr)
			hasAirOrNavalbase = true;

		for (auto it = cp->m_links.begin(), end = cp->m_links.end(); it != end; ++it) //Iterate though linked towns list
		{
			LinkStruct* outLink = *it;
			if (PN_UNLIKELY(outLink == nullptr || outLink->endDepot == nullptr))
				continue;

			const CPStruct* const otherCP = outLink->endDepot->cp;
			if (otherCP->m_townstatus == FRONT_LINE && (cp->m_townstatus == INACTIVE))
			{
				cp->m_townstatus = BACK_LINE;
				dbugLog(_FLN, "Set BACK_LINE town status init for %s", cp->name);
				totalBL++;
			}
		}
		if (cp->m_townstatus == INACTIVE)
		{
			if (hasAirOrNavalbase)
				cp->m_townstatus = REAR_LINE;
			else
				totalIN++;
//			dbugLog(_FLN, "Set INACTIVE town status init for %s", cp->name);
		}
		if (cp->m_townstatus == REAR_LINE)
		{
			totalRL++;
			dbugLog(_FLN, "Set REAR_LINE town status init for %s", cp->name);
		}
		if (cp->m_townstatus == FRONT_LINE)
		{
			totalFL++;
			dbugLog(_FLN, "Set FRONT_LINE town status init for %s", cp->name);
		}
	}

  	dbugLog(_FLN, "finished town status inits for %u front line cps", totalFL);
	dbugLog(_FLN, "finished town status inits for %u back line cps", totalBL);
	dbugLog(_FLN, "finished town status inits for %u rear line cps", totalRL);
	dbugLog(_FLN, "finished town status inits for %u inactive cps", totalIN);
	dbugLog(_FLN, "finished town status inits for %u cps", totalChecked);
}

static void
_killStratData()
{
	g_filenamePool.clear();
}

//////////////////////////////////////////////////////////////////////
// Apply blitzwor and database information to the basic strat data

CPStruct*
strtInitializeStratData()
{
	atexit(_killStratData);

	// Load in the blitzWor modifier data
	if (!parseWor(cv_blitzworPath.GetString(), nullptr))
		LogAbort("Unable to load blitzWor file: %s", cv_blitzworPath.GetString());

#ifdef IS_STRAT_HOST
	// Load the virtual spawn classes.
	spawnLoader.load();
#endif

	totalCPs = 0;

	// Go through all the basic strat objects and ensure they are base-
	// initialized and establish all the basic object indexes. Also pull
	// baseline data from the strat database
	_supercellsPass1();

	// Now that all the base values are in place, we can go through and
	// tie everything up
	_supercellsPass2();

	// Now set up all the reciprocations between FBs/depots
	_mate();

	_checkSupplyLinks();
	_checkCPStatus(Strat::cpList);
	_resetAllObjects();

	_checkCPLinks();

	_setTownStatus();

	_checkSupplyLinks();

	dbugLog(_FLD, "completed InitStratSystem()");

	return Strat::cpList;
}

static FULL_OBJ_ID*
_getStratObject(const FULL_ID index)
{
	Supercell* sc = strtGetSupercell(index.sx, index.sy);
	return sc ? sc->Ptr(index.id) : nullptr;
}

static FULL_OBJ_ID*
_getStratObject(const FULL_OBJ_ID obj)
{
	Supercell* sc = strtGetSupercell(obj.cell.ox >> 4, obj.cell.oy >> 4);
	return sc ? sc->Ptr(obj.id) : nullptr;
}

//////////////////////////////////////////////////////////////////////

Supercell*
strtGetSupercellFULL_OBJ_ID(const FULL_OBJ_ID fullObjID)
{
	return strtGetSupercell(fullObjID.cell.ox >> 4, fullObjID.cell.oy >> 4);
}

Supercell*
strtGetSupercell(const SINT16 x, const SINT16 y)
{
	SUPERCELLS::iterator it = g_supercells.find(static_cast<SINT32>(x << 16) | static_cast<SINT32>(y));
	return (it != g_supercells.end()) ? &it->second : nullptr;
}

Supercell*
strtGetSupercellFromPosition_cm(const SINT32 x_cm, const SINT32 y_cm)
{
	const SINT32 x_m = x_cm / 100;
	const SINT32 y_m = y_cm / 100;
	return strtGetSupercellFromPosition_m(x_m, y_m);
}

Supercell*
strtGetSupercellFromPosition_m(const SINT32 x_m, const SINT32 y_m)
{
	return strtGetSupercell(x_m / SUPERCELL_SIZE_M, y_m / SUPERCELL_SIZE_M);
}

FULL_OBJ_ID*
strtGetFullObjID(const FULL_OBJ_ID index)
{
	return _getStratObject(index);
}

CPStruct*
strtGetCP(const FULL_OBJ_ID index)
{
	FULL_OBJ_ID* id = _getStratObject(index);
	if (PN_LIKELY(id && id->objClass == CLASS_CP))
		return reinterpret_cast<CPStruct*>(id);
	return nullptr;
}

CPStruct*
strtGetCPorBridge(const FULL_OBJ_ID index)
{
	FULL_OBJ_ID* cp = _getStratObject(index);
	if (PN_UNLIKELY(!cp))
		return nullptr;
	if ( cp->objClass == CLASS_CP )
		return reinterpret_cast<CPStruct*>(cp);
	if ( cp->objClass == CLASS_BUILDING )
	{
		ObjectStruct* building = (ObjectStruct*)cp;
		if ( PN_LIKELY(building->type == BUILDING_BRIDGE && building->facility) )
			return building->facility->cp;
	}

	return nullptr;
}

CPStruct*
strtGetCPFromTarget(const FULL_OBJ_ID index)
{
	FULL_OBJ_ID* target = _getStratObject(index);
	while ( target != nullptr )
	{
		if ( target->objClass == CLASS_CP )
			return (CPStruct*) target;
		if ( target->objClass == CLASS_FACILITY )
			target = (FULL_OBJ_ID*)((FacilityStruct*)target)->cp;
		else if ( target->objClass == CLASS_BUILDING )
			target = (FULL_OBJ_ID*)((ObjectStruct*)target)->facility;
		else
			target = nullptr;
	}

	return nullptr;
}

FacilityStruct*
strtGetFacility(const FULL_OBJ_ID index)
{
	FULL_OBJ_ID* fac = _getStratObject(index);
	if (PN_LIKELY(fac && fac->objClass == CLASS_FACILITY))
		return reinterpret_cast<FacilityStruct*>(fac);
	return nullptr;
}

ObjectStruct*
strtGetObject(const FULL_OBJ_ID index)
{
	FULL_OBJ_ID* obj = _getStratObject(index);
	if (PN_LIKELY(obj && obj->objClass == CLASS_BUILDING))
		return reinterpret_cast<ObjectStruct*>(obj);
	return nullptr;
}

LinkStruct* strtGetLink(const FULL_OBJ_ID index)
{
	FULL_OBJ_ID* link = _getStratObject(index);
	if (PN_LIKELY(link && link->objClass == CLASS_LINK))
		return reinterpret_cast<LinkStruct*>(link);
	return nullptr;
}

CPStruct* strtGetCP_FULL_ID(const FULL_ID& index)
{
	FULL_OBJ_ID* id = _getStratObject(index);
	if (PN_LIKELY(id && id->objClass == CLASS_CP))
		return reinterpret_cast<CPStruct*>(id);
	return nullptr;
}

FacilityStruct* strtGetFacility_FULL_ID(const FULL_ID& index)
{
	FULL_OBJ_ID* id = _getStratObject(index);
	if (PN_LIKELY(id && id->objClass == CLASS_FACILITY))
		return reinterpret_cast<FacilityStruct*>(id);
	return nullptr;
}

ObjectStruct* strtGetObject_FULL_ID(const FULL_ID& index)
{
	FULL_OBJ_ID* id = _getStratObject(index);
	if (PN_LIKELY(id && id->objClass == CLASS_BUILDING))
		return reinterpret_cast<ObjectStruct*>(id);
	return nullptr;
}

LinkStruct* strtGetLink_FULL_ID(const FULL_ID& index)
{
	FULL_OBJ_ID* id = _getStratObject(index);
	if (PN_LIKELY(id && id->objClass == CLASS_LINK))
		return reinterpret_cast<LinkStruct*>(id);
	return nullptr;
}

void
strtResetObject(ObjectStruct& object)
{
	if ( object.isValid() == false )
		return;

	UINT32 newMax;
	if ( object.Facility().type != FACILITY_FIREBASE || object.m_maxDamageJoules > 0 )
		newMax = 1000;
	else
		newMax = 0;

	object.setMaxCapacity(newMax);

	object.setVisLevel(VISLEVEL_NORMAL);
	object.m_smokeLevel = 0;

	_calcCapacity(object);

	//Add the RPs required to rebuild the object to the total
	//for the Facility
	object.Facility().objectRPTotal += object.totalRPs;

	object.save();
}

static bool _checkFirebasePair(FacilityStruct& firebase)
{
	if (firebase.type != FACILITY_FIREBASE)
		return false;

	if (firebase.link->endDepot->m_outgoingLink == nullptr)
	{
		dbugLog(_FLA,"%s: Firebase end depot %s doesn't have an outgoing link"
					,firebase.name,firebase.link->endDepot->name);
		return false;
	}

	if (firebase.link->endDepot->m_outgoingLink->firebase == nullptr)
		return false;

	return true;
}

// Search the armybases of a facility's cp to see which is the
// closest and therefore its "controlling ab".

FacilityStruct*
FacilityStruct::findLocalAB(WWII_SIDE favorSide) const
{
	UINT32	currentDistance = 0;			// How far current ab is
	UINT32	nearestDistance = 0xffffffff;	// How far closest was
	const FacilityStruct* currentAB;		// AB being considered
	const FacilityStruct* chosenAB;		// The AB we are choosing
	const FacilityStruct* lastResort;

	// If the facility is an armybase itself, then it is its own spawn source
	if ( isSpawnSource() )
		return const_cast<FacilityStruct*>(this);

	if ( type == FACILITY_FIREBASE )
	{
		// If you're looking for an ownership matchup, check sides first.
		if ( (favorSide > 0 && side != favorSide) || link == nullptr || link->startDepot == nullptr )
			return nullptr;

		return link->startDepot->findLocalAB(favorSide);
	}

	// If we aren't the owners, none of the armybases are supplying
	if ( favorSide > 0 && (cp->side != favorSide || cp->ControllingSide() != favorSide) )
		return nullptr;

	// No ABs means that the facilities have to rely on themselves.
	if ( cp->numArmyBases <= 0 )
	{
		return const_cast<FacilityStruct*>(this);
	}

	chosenAB = nullptr;
	lastResort = nullptr;

	// Otherwise we need to do a radial search
	for ( currentAB = cp->facilityTypeIndex[FACILITY_ARMYBASE]
		; currentAB != nullptr
		; currentAB = currentAB->typeNext
		)
	{
		// If a country has been supplied ignore any non-country ABs
		// or armybases which are currently closed
		if ( favorSide && (!currentAB->isOpen() || currentAB->side != favorSide) )
			continue;

		// Otherwise we need to know how far apart they are
		currentDistance = _facilityDistance(this, currentAB);
		if ( currentDistance < nearestDistance )
		{
			if ( currentAB->m_spawnsInf && currentAB->m_spawnsVehicles )
			{
				chosenAB = currentAB;
				nearestDistance = currentDistance;
			}
			else
				lastResort = currentAB;
		}
	}

	if ( chosenAB == nullptr )
		chosenAB = lastResort;

	return const_cast<FacilityStruct*>(chosenAB);
}

//returns TRUE if the facility can spawn ground vehicles
static bool
_spawnsGroundVehicles(const FacilityStruct& facility)
{
	if ( facility.m_spawnsVehicles )
		return true;

	for ( auto obj = facility.objectList; obj; obj = obj->next )
	{
		if ( obj->type == BUILDING_SPAWN && obj->spawnAbility.weap.wCategory == _GROUND_VEHICLE )
			return true;
	}

	return false;
}

#ifdef STRAT_VALIDATOR
//returns TRUE if the facility can spawn aircraft
static bool
_spawnsAircraft(const FacilityStruct& facility)
{
	for ( auto obj = facility->objectList; obj; obj = obj->next )
	{
		if ( obj->type == BUILDING_SPAWN && obj->spawnAbility.weap.wCategory == _AIRCRAFT )
			return true;
	}

	return false;
}
#endif

//returns TRUE if the facility can spawn infantry
static bool
_spawnsInfantry(const FacilityStruct& facility)
{
	if ( facility.m_spawnsInf )
		return true;

	for ( auto obj = facility.objectList; obj; obj = obj->next )
	{
		if ( obj->type == BUILDING_SPAWN && obj->spawnAbility.weap.wCategory == _INFANTRY )
			return true;
	}

	return false;
}

//returns TRUE if the facility can spawn sea vehicles
static bool
_spawnsSeaVehicles(const FacilityStruct& facility)
{
	for ( auto obj = facility.objectList; obj; obj = obj->next )
	{
		if ( obj->type == BUILDING_SPAWN && obj->spawnAbility.weap.wCategory == _SEA_VEHICLE )
			return true;
	}

	return false;
}


//Verifies the integrity of the strat System, using the following heuristics:
//	1.  Link must have a valid starting and ending Depot
//	2.  Each depot must have no more than one incoming and one outgoing link
bool
strtVerifyStratSystem(CPStruct* cpList, UINT32 curTestLevel)
{
	if (cpList == nullptr)
	{
		dbugLog(_FLA,"CP List is null, strat system is not initialized");
		return false;
	}

	CPStruct* currentCP;
	FacilityStruct* fac;
	bool retval = true;
	ObjectStruct* building;

	for ( currentCP = cpList; currentCP != nullptr; currentCP = currentCP->next )
	{
		if (wwiiSide(currentCP->country) != currentCP->side)
		{
			dbugLog(_FLA,"CP %s has side %d and country %d", currentCP->name, currentCP->side, currentCP->country);
			retval = false;
		}

		if (curTestLevel == 3)
		{
			dbugLog(_FLI,"Depot cp %s Cside %d Ccountry %d side %d country %d"
						, currentCP->name
						, currentCP->ControllingSide()
						, currentCP->ControllingCountry()
						, currentCP->side
						, currentCP->country
						);
		}

		fac = currentCP->facilityList;
		while (fac != nullptr)
		{
			WWII_SIDE expectSide = wwiiSide(fac->country);
			if ( fac->side != expectSide )
			{
				dbugLog(_FLA,"%s Side and country do not match: country %s(%u), side %s(%u) should be %s(%u)"
								, fac->name
								, _countryCode[fac->country], fac->country
								, _sideName[fac->side], fac->side
								, _sideName[expectSide], expectSide
								);
				retval = false;
			}

			if (curTestLevel == 2)
			{
				for (building = fac->objectList
					; building != nullptr
					; building = building->next)
				{
					if ( building->type == BUILDING_AUTOGUNNER )
					{
						dbugLog(_FLI, "found autogunner %s " PRF_FULL_OBJ_ID " subtype %d rebuild period %d, totRPS %d RPS/dlevel %d max djoules %" PR64BitT ""
									, building->name
									, PRA_FULL_OBJ_ID(building->fullObjID)
									, building->subType
									, building->rebuildPeriod
									, building->totalRPs
									, building->rpsPerDamageLevel
									, building->m_maxDamageJoules);
					}
				}
			}

			if (curTestLevel == 3)
			{
				dbugLog(_FLI,"facility %s side %d country %d"
							, fac->name
							, fac->side
							, fac->country);

			}

			switch(fac->type)
			{
				case FACILITY_DEPOT:
				case FACILITY_OMT:
				case FACILITY_SHIPYARD:
					//We aren't doing this check for Phase V.2 testing, but
					//	it needs to go into the release version
					if ( fac->m_outgoingLink == nullptr && fac->m_incomingLink == nullptr )
					{
						if ( curTestLevel >= 1 )
						{
							strtAddErrorListEntry(ERR_ZERO_LINKS, fac->name, fac->name, "");
							retval = false;
						}
						// Otherwise its a warning
					}
					else if ( fac->m_incomingLink == nullptr )
					{
						g_dataErrors.emplace(ERR_ZERO_ILINKS, fac->name, "");
						retval = false;
					}
					else if ( fac->m_outgoingLink == nullptr )
					{
						g_dataErrors.emplace(ERR_ZERO_OLINKS, fac->name, "");
						retval = false;
					}
				break;
				case FACILITY_FIREBASE:
					//Verify ground troops can spawn
					if (curTestLevel >= 1)
					{
						//verify ground troops can spawn
						if (!(_spawnsGroundVehicles(*fac) && _spawnsInfantry(*fac)))
						//Print the log message if they can't
						{
							strtAddErrorListEntry(ERR_FB_BOTH_CHECK, fac->name, fac->name, "");
							retval = false;
						}
					}
					if ( fac->numObjects == 0 )
					{
						g_dataErrors.emplace(ERR_FB_ZERO_OBJECTS, fac->name, "");
						retval = false;
						break;
					}
					{
						UINT64 capacity = 0LL;
						UINT32 damageables = 0;
						bool tainted = false;
						for ( ObjectStruct* it = fac->objectList; it != nullptr; it = it->next )
						{
							if ( strstr(it->filename, "spawn") != nullptr && it->m_maxDamageJoules > 0 )
							{
								g_dataErrors.emplace(ERR_FB_DAMAGEABLE_SPAWN, fac->name, "");
								tainted = true;
								retval = false;
							}
							if ( it->m_maxDamageJoules > 0 )
							{
								capacity += it->m_maxCapacity;
								++damageables;
							}
						}
						capacity /= fac->numObjects;
						if ( capacity != (UINT64)fac->maxCapacity() && (!tainted || curTestLevel > 0) )
						{
							g_dataErrors.emplace(ERR_FB_DIVISOR, fac->name, "");
							retval = false;
						}
					}
					if (fac->link == nullptr)
					{
						g_dataErrors.emplace(ERR_FB_NO_LINK, fac->name, "");
						retval = false;
					}
					else
					{
						// Check for matching firebase
						if (!_checkFirebasePair(*fac))
						{
							g_dataErrors.emplace(ERR_FB_NO_MATE, fac->name, "");
							retval = false;
						}
					}
				break;
				case FACILITY_ARMYBASE:
					if (curTestLevel >= 1)
					{
						//verify ground troops can spawn
						if (!(_spawnsGroundVehicles(*fac) && _spawnsInfantry(*fac)))
						//Print the log message if they can't
						{
							strtAddErrorListEntry(ERR_AB_BOTH_CHECK, fac->name, fac->name, "");
							retval = false;
						}
					}
				break;
#ifdef STRAT_VALIDATOR
				case FACILITY_AIRBASE:
					//verify aircraft can spawn
					//Print the log message if they can't
					if (!_spawnsAircraft(*fac))
					{
						strtAddErrorListEntry(ERR_AF_NO_SPAWN, fac->name, fac->name, "");
						retval = false;
					}
				break;
#endif
				case FACILITY_NAVALBASE:
					if (curTestLevel >= 1)
					{
						//verify aircraft can spawn
						//Print the log message if they can't
						if (!_spawnsSeaVehicles(*fac))
						{
							dbugLog(_FLA,"%s: Navalbase cannot spawn sea vehicles",fac->name);
							retval = false;
						}
					}
				break;
				default: break;
			}

			if (curTestLevel >= 1)
			{
				//Verify the existence of a capture object
				bool doCaptureTest = (curTestLevel > 1 && strcmp(fac->name, "UK") != 0 && strcmp(fac->name, "FR") != 0 && strcmp(fac->name, "DE") != 0);
				if ( doCaptureTest && fac->type != FACILITY_FIREBASE && fac->m_captureBuilding == nullptr )
				{
					dbugLog(_FLA,"%s: non-firebase with no capture object",fac->name);
					retval = false;
				}
				//Also verify subtypes where appropiate for all buildings
				if (!_verifySubtypes(fac))
				{
					dbugLog(_FLA,"subtype is not apropriate for all buildings for facility %s",fac->name);
					retval = false;
				}

			}

			//Advance the pointer
			fac = fac->next;
		}
	}

	if ( strtReportDataErrors(curTestLevel) > 0 )
		retval = false;

	if ( cv_stratIgnoreErrors.GetBool() )
		return true;

	return retval;
}

static bool _isSpawnObject(ObjectStruct* object)
{
	static const char* spawners[] = {
		"offlineac_spawn1", "ac_spawn1", "gv_spawn1", "in_spawn1", "in_spawnb1c",
		"gv_spawng1c", "in_spawnfb1c", "in_spawnfb2c", "gv_spawnfb1c",
		"gv_spawnfb2c", "sv_spawng1c", "ac_spawn1c", "acrare_spawn1c",
		"acspit_spawn1c", "mshack1c",
		nullptr
	};

	for ( int i = 0; spawners[i] != nullptr; ++i )
	{
		if ( !strncmp(object->filename, spawners[i], MAX_OBJNAME_LENGTH) )
			return true;
	}
	return false;
}

//Returns true if all buildings that are capture objects have the capture subtype
//also checks spawn objects as well
//returns false otherwise
static bool _verifySubtypes(FacilityStruct* facility)
{
	bool retval = true;
	ObjectStruct* currentObject;

	if (facility==nullptr)
	{
		dbugLog(_FLA,"NUll facility in _verifySubtypes!");
		return retval;
	}

	currentObject = facility->objectList;
	while (currentObject!=nullptr)
	{
	//check every object at this facility for capture and spawn
		if (_isSpawnObject(currentObject))
		{
			if (currentObject->type != BUILDING_SPAWN)
			{
				dbugLog(_FLA,"%s is object %s and type %d.  Should be type %d"
								, currentObject->name
								, currentObject->filename
								, currentObject->type
								, BUILDING_SPAWN);
				retval = false;
			}
		}
		currentObject = currentObject->next;
	}

	return retval;
}

//strtDTL initialization functions
void strtInitCPStructwithCP(CPStruct& newCP, const CP& cp)
{
	//Fill in the fullObjID
	//Calculate octet x and y from POSITIONAL x and y
	newCP.fullObjID.cell.ox = cp.posx / 800;
	newCP.fullObjID.cell.oy = cp.posy / 800;
	newCP.m_posX = cp.posx;
	newCP.m_posY = cp.posy;

	//Get the class and type from oid
	newCP.fullObjID.id = cp.oid.id;
	newCP.fullObjID.objClass = cp.oid.objClass;
	newCP.fullObjID.objType = cp.oid.objType;

	newCP.subType = cp.subType;

	newCP.setSide((WWII_SIDE)cp.side);
	newCP.originalSide = newCP.controllingSide = (WWII_SIDE) cp.side;

	newCP.country = newCP.originalCountry = newCP.controllingCountry = (WWII_COUNTRY) cp.country;

	strsafecpy(newCP.name, cp.name);
	newCP.m_filename = _filenamePool(cp.fileName);
	newCP.filename = newCP.m_filename->c_str();
}

void strtInitFacilityStructwithFACILITY(FacilityStruct& newFacility, const FACILITY& facility)
{
	// Remember the location
	newFacility.m_posX = facility.posx;
	newFacility.m_posY = facility.posy;

	//Fill in the fullObjID
	//Calculate octet x and y from x and y location in meters
	newFacility.fullObjID.cell.ox = facility.posx / 800;
	newFacility.fullObjID.cell.oy = facility.posy / 800;

	//Get the class and type from oid
	newFacility.fullObjID.id = facility.oid.id;
	newFacility.fullObjID.objClass = facility.oid.objClass;
	newFacility.fullObjID.objType = newFacility.type = (FACILITY_TYPE) facility.oid.objType;

	newFacility.subType = facility.subType;

	newFacility.cpID = facility.cpid;

	newFacility.side = newFacility.originalSide = (WWII_SIDE) facility.originalSide;

	newFacility.country = newFacility.originalCountry = (WWII_COUNTRY) facility.originalCountry;

	strsafecpy(newFacility.name, facility.name);
	newFacility.m_filename = _filenamePool(facility.fileName);
	newFacility.filename = newFacility.m_filename->c_str();
}

void strtInitObjectStructwithBUILDING(ObjectStruct& newObject, const BUILDING& building)
{
	//Remember the location
	newObject.posX = building.posx;
	newObject.posY = building.posy;

	//Fill in the fullObjID
	//Calculate octet x and y from supercell x and y
	newObject.fullObjID.cell.ox = building.posx / 800;
	newObject.fullObjID.cell.oy = building.posy / 800;

	//Get the class and type from oid
	newObject.fullObjID.id = building.oid.id;
	newObject.fullObjID.objClass = building.oid.objClass;
	newObject.fullObjID.objType = newObject.type = (OBJECTTYPE)building.oid.objType;

	newObject.subType = building.subType;

	//Fake the facility ID for the parent. Why?  B/c we have no idea what
	//	Octet the parent is in.  THis field is only used for lookup, and
	//	we still store things based on supercells, not on octets
	//	The long and the short of it is that this is more FULL_OBJ_ID BS
	newObject.facilityID.cell.ox = building.posx / 800;
	newObject.facilityID.cell.oy = building.posy / 800;
	newObject.facilityID.id = building.pid.id;
	newObject.facilityID.objClass = building.pid.objClass;
	newObject.facilityID.objType = building.pid.objType;

	strsafecpy(newObject.name, building.name);
	newObject.m_filename = _filenamePool(building.fileName);
	newObject.filename = newObject.m_filename->c_str();
}

void strtInitLinkStructwithDEPOT_LINK(LinkStruct& newLink, const DEPOT_LINK& depotlink)
{
	//Links don't have a posx and a posy, so we get the ox and oy from its
	//	start depot
	newLink.fullObjID.cell.ox = depotlink.startDepotId.cell.ox;
	newLink.fullObjID.cell.oy = depotlink.startDepotId.cell.oy;

	//Get the class and type from oid
	newLink.fullObjID.id = depotlink.oid.id;
	newLink.fullObjID.objClass = depotlink.oid.objClass;
	newLink.fullObjID.objType = depotlink.oid.objType;

	newLink.startDepotID = depotlink.startDepotId;
	newLink.endDepotID = depotlink.endDepotId;
	newLink.firebaseID = depotlink.firebaseId;

	newLink.distance = depotlink.distance;
	strsafecpy(newLink.name, depotlink.name);
}

//This function looks up the information for a CP from
//	the WOR parser that Marty gave us
bool strtInitCPfromWOR(CPStruct& cp)
{
    CP_PARAMS* cpParams;

	//First try and find out if we have any information on the object name
	cpParams = parseGetCPValues(*cp.m_filename);

	if (cpParams == nullptr)
	{
		static const std::string cpGeneric("CP_GENERIC"), cpSmall("SMALL_CP"), cpMed("MED_CP"), cpLarge("LARGE_CP"), cpBridge("CP_BRIDGE");
		switch (cp.fullObjID.objType)
		{
			case CP_GENERIC:
				cpParams = parseGetCPValues(cpGeneric);
			break;
			case CP_SMALL:
				cpParams = parseGetCPValues(cpSmall);
			break;
			case CP_MEDIUM:
				cpParams = parseGetCPValues(cpMed);
			break;
			case CP_LARGE:
				cpParams = parseGetCPValues(cpLarge);
			break;
			case CP_BRIDGE:
				cpParams = parseGetCPValues(cpBridge);
			break;
			default:
				cpParams = parseGetCPValues(cpGeneric);
			break;
		}
	}

	if (PN_UNLIKELY(cpParams == nullptr))
		return false;

	cp.timeToProduce = cpParams->ttp;

	return true;
}

//This function looks up the information for a Facility from
//	the WOR parser that Marty gave us
bool strtInitFacilityfromWOR(FacilityStruct& facility)
{
    FACILITY_PARAMS* facilityParams;

	//First try and find out if we have any information on the object name
	facilityParams = parseGetFacilityValues(*facility.m_filename);

	if (facilityParams == nullptr)
	{
		static const std::string facCity("CITY"), facFactory("FACTORY"), facBase("BASE"), facDepot("DEPOT"), facOMT("OMT"), facShipyard("SHIPYARD"),
						facFirebase("FIREBASE"), facAirbase("AIRBASE"), facArmybase("ARMYBASE"), facNavalBase("NAVALBASE");
		switch(facility.fullObjID.objType)
		{
			case FACILITY_CITY:
				facilityParams = parseGetFacilityValues(facCity);
			break;
			case FACILITY_FACTORY:
				facilityParams = parseGetFacilityValues(facFactory);
			break;
			case FACILITY_BASE:
				facilityParams = parseGetFacilityValues(facBase);
			break;
			case FACILITY_DEPOT:
				facilityParams = parseGetFacilityValues(facDepot);
			break;
			case FACILITY_OMT:
				facilityParams = parseGetFacilityValues(facOMT);
			break;
			case FACILITY_SHIPYARD:
				facilityParams = parseGetFacilityValues(facShipyard);
			break;
			case FACILITY_FIREBASE:
				facilityParams = parseGetFacilityValues(facFirebase);
			break;
			case FACILITY_AIRBASE:
				facilityParams = parseGetFacilityValues(facAirbase);
			break;
			case FACILITY_ARMYBASE:
				facilityParams = parseGetFacilityValues(facArmybase);
			break;
			case FACILITY_NAVALBASE:
				facilityParams = parseGetFacilityValues(facNavalBase);
			break;
		}
	}

	if (PN_UNLIKELY(facilityParams == nullptr))
		return false;

	//Populate the struct
	facility.shippingThreshold = facilityParams->shippingThreshold;
	facility.requestThreshold = facilityParams->requestThreshold;
	facility.rpTransferLimit = facilityParams->rptl;
	facility.rpsPerUnitTime = facilityParams->rput;
	facility.multiplier = facilityParams->multiplier;

	return true;
}

//This function looks up the information for an Object from
//	the WOR parser that Marty gave us
bool strtInitObjectfromWOR(ObjectStruct& object)
{
    BUILDING_PARAMS* objectParams;

	//Get the information from the object name
	objectParams = parseGetBuildingValues(*object.m_filename);

	if (PN_UNLIKELY(objectParams == nullptr))
		return false;

	object.m_maxDamageJoules = objectParams->maxdamage;
	object.totalRPs = objectParams->rpsrepair;
	object.rebuildPeriod = objectParams->repairtime;
	object.rpsPerDamageLevel = objectParams->rpslevel;
	object.objectValue = objectParams->objectValue;

	//Get spawn ability
	object.spawnAbility = objectParams->spawn;

	object.m_isCaptureBuilding = (objectParams->stratFlags != 0);

	return true;
}

//This function looks up the information for a Link from
//	the WOR parser that Marty gave us
bool strtInitLinkfromWOR(LinkStruct& link)
{
	if (PN_UNLIKELY(link.startDepot == nullptr))
		return false;

	DEPOT_LINK_PARAMS* linkParams;
	static const std::string linkRail("RAIL_LINK"), linkBarge("BARGE_LINK"), linkConvoy("CONVOY_LINK"),
						linkPackAnimal("PACK_ANIMAL_LINK"), linkWagon("WAGON_LINK"), linkPorter("PORTER_LINK"),
						linkTruck("TRUCK_LINK");
	switch (link.startDepot->subType)
	{
		case DEPOT_TRAIN:
			linkParams = parseGetDepotLinkValues(linkRail);
		break;
		case DEPOT_BARGE:
			linkParams = parseGetDepotLinkValues(linkBarge);
		break;
		case DEPOT_CONVOY:
			linkParams = parseGetDepotLinkValues(linkConvoy);
		break;
		case DEPOT_PACK_ANIMAL:
			linkParams = parseGetDepotLinkValues(linkPackAnimal);
		break;
		case DEPOT_WAGON:
			linkParams = parseGetDepotLinkValues(linkWagon);
		break;
		case DEPOT_PORTER:
			linkParams = parseGetDepotLinkValues(linkPorter);
		break;
		case DEPOT_GENERIC:
		case DEPOT_TRUCK:
		default:
			linkParams = parseGetDepotLinkValues(linkTruck);
		break;
	}

	if (PN_UNLIKELY(linkParams == nullptr))
		return false;

	//FIXME!!!!
	link.latency = 10;
	link.capacity = 1000;
	//link.capacity = linkParams->maxCapacity;

	return true;
}

static void _calcCapacity(ObjectStruct& object)
{
	SINT32 lastPercentDamage = object.percentDamage;
	object.calcPercentDamage();

	if (lastPercentDamage < 50 && object.percentDamage >= 50)
		object.setVisLevel(1);

#ifdef IS_STRAT_HOST
	object.recalculateCapacity();
	auto& fac = object.Facility();
	fac.recalculateCapacity();

	if ( fac.capacity == 0
		&& fac.type == FACILITY_FIREBASE
		&& fac.isOpen() )
		strtProcessFirebase(fac);
#endif
}

//////////////////////////////////////////////////////////////////////
// Match ownership of a sub-objects to their parent

static void _setObjectsOwner(ObjectStruct* objectList, WWII_SIDE side, WWII_COUNTRY country)
{
	///FIXME: We should probably put an assert here
	if ( objectList == nullptr )
		return;
	if ( country == WWII_NO_COUNTRY || country >= MAX_COUNTRIES )
		return;
	if ( side == WWII_NO_SIDE || side >= MAX_SIDES )
		return;

	ObjectStruct* current;

	//Rip through the object list, changing ownership along the way
	for ( current = objectList; current != nullptr; current = current->next )
	{
		current->setCountry(country, side);
	}
}

//////////////////////////////////////////////////////////////////////
// Test to see if a country/side pair are valid

static inline
bool _validOwnership(WWII_COUNTRY country, WWII_SIDE side)
{
	if ( country == WWII_NO_COUNTRY || country >= MAX_COUNTRIES )
		return false;
	if ( side == WWII_NO_SIDE || side >= MAX_SIDES )
		return false;
	return true;
}

//////////////////////////////////////////////////////////////////////
// Increment ownership counters

static void
_ownFacility(FacilityStruct& facility)
{
	CPStruct& cp = facility.CP();

	if ( facility.type == FACILITY_ARMYBASE && cp.armyBasesOwned[facility.side] < cp.numArmyBases )
		cp.armyBasesOwned[facility.side]++;
	if ( facility.type != FACILITY_FIREBASE && cp.facilitiesOwned[facility.side] < cp.ownableFacilities )
		cp.facilitiesOwned[facility.side]++;
}

//////////////////////////////////////////////////////////////////////
// Decrement ownership counters (before forcing ownership of a facility)

static void
_disownFacility(FacilityStruct& facility)
{
	CPStruct& cp = facility.CP();

	if ( facility.type == FACILITY_ARMYBASE && cp.armyBasesOwned[facility.side] > 0 )
		cp.armyBasesOwned[facility.side]--;
	if ( facility.type != FACILITY_FIREBASE  && cp.facilitiesOwned[facility.side] > 0 )
		cp.facilitiesOwned[facility.side]--;
}

//////////////////////////////////////////////////////////////////////
// Make sure a facility matches the specified ownership, including all of its
// sub-objects.

static void
_forceFacility(FacilityStruct& facility, const WWII_SIDE side, WWII_COUNTRY country)
{
	// Decrement the counters for the side that currently holds it
	_disownFacility(facility);

	if ( facility.isRDPFactory() && side == facility.originalSide )
		country = facility.originalCountry;

	if ( facility.side != side || facility.country != country )
	{
		facility.side = side;
		if ( facility.armyBase != nullptr && facility.armyBase != &facility && facility.armyBase->side == side )
			facility.setCountry(facility.armyBase->country);
		else
			facility.setCountry(country);

#ifdef IS_STRAT_HOST
		strtSaveFacility(&facility);
#endif
	}

	// Increment the counters for the side that just gained it
	_ownFacility(facility);

	// Make sure all sub-objects are correctly assigned
	_setObjectsOwner(facility.objectList, side, facility.country);
}

//////////////////////////////////////////////////////////////////////

static void
_forceFacilitySide(FacilityStruct& fac, WWII_SIDE side, WWII_COUNTRY country)
{
	if ( fac.side != side )
		_forceFacility(fac, side, country);
	else
		_setObjectsOwner(fac.objectList, side, fac.country);
}

//////////////////////////////////////////////////////////////////////
// Force complete, uncontested, ownership on a town

static void
_cpOwnership(CPStruct& cp, WWII_COUNTRY country)
{
	FacilityStruct* facility;
	WWII_SIDE side;

	if ( country == WWII_NO_COUNTRY || country >= MAX_COUNTRIES )
		country = cp.originalCountry;

	side = wwiiSide(country);

#if defined(DEV_BUILD)
	dbugLog(_FLI, "forcing %d:%d ownership on %s from %d:%d"
					, country, side
					, cp.name
					, cp.country, cp.side
					);
#endif

	cp.setSide(side);
	cp.country = country;
	cp.setContention(false);
	cp.controllingSide = side;
	cp.controllingCountry = country;

	// Remove any entries that belong to the wrong side, but be lenient on
	// country differences; this allows CPs to have mixed ownership, e.g.
	// a French town with a British airfield.

	// Start with Army bases, since depots etc will take their country from
	// the AB that they belong to to make them spawnable
	for ( facility = cp.facilityTypeIndex[FACILITY_ARMYBASE]; facility != nullptr; facility = facility->typeNext )
		_forceFacilitySide(*facility, side, country);

	for ( facility = cp.facilityList; facility != nullptr; facility = facility->next )
	{
		_forceFacilitySide(*facility, side, country);

		if ( facility->type != FACILITY_FIREBASE && facility->m_spawnable )
			facility->setOpen(true);
	}
}

//////////////////////////////////////////////////////////////////////
// Force control on a town: if we know the town is fully owned by one side,
// then we would force "ownership". Enforcing control is just making sure that
// spawnable entities become spawnable again; making sure depot ownerships
// match, etc.

static void
_cpControl(CPStruct& cp, WWII_COUNTRY country)
{
	FacilityStruct* facility;
	WWII_SIDE side;

	if ( country == WWII_NO_COUNTRY || country >= MAX_COUNTRIES )
		country = cp.originalCountry;

	side = wwiiSide(country);

#if defined(DEV_BUILD)
	dbugLog(_FLI, "forcing %d:%d control on %s from %d:%d"
					, country, side
					, cp.name
					, cp.country, cp.side
					);
#endif

	cp.controllingSide = side;
	cp.controllingCountry = country;

	// Make sure the counters are correct
	memset(cp.armyBasesOwned, 0 , sizeof(cp.armyBasesOwned));
	cp.armyBasesOwned[side] = cp.numArmyBases;

	// Check the army bases first
	for ( facility = cp.facilityTypeIndex[FACILITY_ARMYBASE]; facility != nullptr; facility = facility->typeNext )
	{
		_forceFacilitySide(*facility, side, country);
		facility->setOpen(true);
	}

	// Link depots
	for ( facility = cp.facilityTypeIndex[FACILITY_DEPOT]; facility != nullptr; facility = facility->typeNext )
	{
		// If the side matches, make sure the country does too so its spawnable
		_setObjectsOwner(facility->objectList, facility->side, facility->country);
		facility->setOpen(true);
	}

	// Associate FBs too
	for ( facility = cp.facilityTypeIndex[FACILITY_FIREBASE]; facility != nullptr; facility = facility->typeNext )
	{
		if ( facility->armyBase )
			_forceFacility(*facility, side, facility->armyBase->country);
	}
}

//////////////////////////////////////////////////////////////////////
// Locate an army base for a firebase

static bool
_findFirebaseArmybase(FacilityStruct* fb)
{
	if ( fb->link == nullptr )
	{
		g_dataErrors.emplace(ERR_FB_NO_LINK, fb->name, "");
		return false;
	}

	FacilityStruct* depot = fb->link->startDepot;

	if ( !depot )
	{
		dbugLog(_FLA, "link from FireBase %s doesnt have an start depot", fb->name);
		return false;
	}

	if ( depot->armyBase == nullptr )
		depot->armyBase = depot->findLocalAB(WWII_NO_SIDE);
	if ( depot->armyBase == nullptr )
	{
		dbugLog(_FLW, "FB '%s' depot '%s' has null armybase!"
					, fb->name
					, depot->name
					);
		return false;
	}
#ifdef HEAVY_DEBUG
	else
		dbugLog(_FLI, "FB %s has AB %s", fb->name, depot->armyBase->name);
#endif
	fb->armyBase = depot->armyBase;

	return true;
}

//////////////////////////////////////////////////////////////////////
//Function verifies the integrity of the cpList, fixing any problems it
//	comes across
//	Checks for:
//		1. Ownership matches at least one of the owners of the CP
//		2. Control makes sense given the ownership of the army bases* AND*
//			whether they are open or closed
static void
_checkCPStatus(CPStruct* cpList)
{
	if ( cpList == nullptr )
		return;

	WWII_COUNTRY maxSideCountry[MAX_SIDES];
	SINT32 ownerCount[MAX_COUNTRIES];
	WWII_SIDE otherSide;

	// We take snapshots of each CP and facility object before we process them,
	// so we can tell if *anything* changed during the processing. We allocate
	// memory for this as an opaque container and then cast pointers to it,
	// this is to prevent any complex objects (stl containers) being destructed.
	std::unique_ptr<uint8_t[]> cpSnapAlloc(new uint8_t[sizeof(CSAFE_CPStruct)]);
	auto* cpSnapshot = reinterpret_cast<CSAFE_CPStruct*>(cpSnapAlloc.get());

	std::unique_ptr<uint8_t[]> facSnapAlloc(new uint8_t[sizeof(CSAFE_FacilityStruct)]);
	auto* facSnapshot = reinterpret_cast<CSAFE_FacilityStruct*>(facSnapAlloc.get());

	for ( CPStruct* current = cpList; current != nullptr; current = current->next )
	{
		CPStruct& cp = *current;

#if defined(STRAT_DB)
		// The validator doesn't use the database, so doesn't load these values
		if ( cp.uOID == 0 )
			dbugLog(_FLW, "CP %s has no unique id", cp.name);
#endif // VALIDATOR

		if ( cp.fullObjID.objType == CP_BRIDGE )
		{
			cp.country = (WWII_COUNTRY)BRIDGE_BUILT_COUNTRY;
			cp.setSide((WWII_SIDE)BRIDGE_SIDE);
			cp.originalCountry = cp.country;
			cp.originalSide = cp.side;
			cp.controllingCountry = cp.country;
			cp.controllingSide = cp.side;
			cp.setContention(false);
#ifdef IS_STRAT_HOST
			strtSaveCP(&cp);
#endif
			continue;
		}

		if ( cp.supercell == nullptr )
			dbugLog(_FLW, "%s: CP has no supercell data", cp.name);
		else if ( cp.ownableFacilities == 0 )
			strtAddErrorListEntry(ERR_CP_NO_OWNABLES, cp.name, cp.name, "");

		memcpy(cpSnapshot, &cp, sizeof(*cpSnapshot));
		//Set our default values
		memset(ownerCount, 0, sizeof(ownerCount));
		memset(maxSideCountry, 0, sizeof(maxSideCountry));
		memset(cp.armyBasesOwned, 0, sizeof(cp.armyBasesOwned));
		memset(cp.facilitiesOwned, 0, sizeof(cp.facilitiesOwned));

		// This would be the right place to count the number of
		// firebases that a CP has rather than during the sdm
		// initialization, since we have all supercells loaded now
		// - as long as this function is called on the other hosts
		// OGS

		cp.setContention(false);
		cp.m_attackStarted = 0;

		for ( FacilityStruct* facility = cp.facilityList
			; facility != nullptr
			; facility = facility->next)
		{
			memcpy(facSnapshot, facility, sizeof(*facSnapshot));
			facility->side = wwiiSide(facility->country);

			if ( !_validOwnership(facility->country, facility->side) )
				_forceFacility(*facility, facility->originalSide, facility->originalCountry);

			if ( facility->armyBase == nullptr )
			{
				if ( facility->type == FACILITY_FIREBASE )
					_findFirebaseArmybase(facility);
				else
					facility->armyBase = facility->findLocalAB(WWII_NO_SIDE);

				// A change of armybase doesn't require a save
				facSnapshot->armyBase = facility->armyBase;
			}

			// Add this facility to it's owner's counts
			_ownFacility(*facility);

			if ( facility->type == FACILITY_ARMYBASE )
			{
					ownerCount[facility->country]++;
					if (ownerCount[facility->country] > ownerCount[maxSideCountry[facility->side]])
						maxSideCountry[facility->side] = facility->country;
			}

			if (memcmp(facSnapshot, facility, sizeof(*facSnapshot)) != 0)
			{
				 _setObjectsOwner(facility->objectList, facility->side, facility->country);
				//Save the facility to the DB
#ifdef IS_STRAT_HOST
# ifdef HEAVY_DEBUG
				dbugLog(_FLI,"fixed facility %s saving to DB",facility->name);
# endif // HEAVY_DEBUG
				strtSaveFacility(facility);
#endif
			}
		}

// Declare complete ownership
		if ( !_validOwnership(cp.country, cp.side) )
			dbugLog(_FLA, "Invalid ownership[%u:%u] at %s"
							, cp.country, cp.side, cp.name);

		if ( !_validOwnership(cp.controllingCountry, cp.controllingSide) )
			dbugLog(_FLA, "Invalid control states at %s", cp.name);

		// Conditions that we can easily assert
		if ( cp.facilitiesOwned[ALLIED] >= cp.ownableFacilities )
			_cpOwnership(cp, maxSideCountry[ALLIED]);
		else if ( cp.facilitiesOwned[AXIS] >= cp.ownableFacilities )
			_cpOwnership(cp, maxSideCountry[AXIS]);

		if ( cp.armyBasesOwned[ALLIED] >= cp.numArmyBases )
			_cpControl(cp, maxSideCountry[ALLIED]);
		else if ( cp.armyBasesOwned[AXIS] >= cp.numArmyBases )
			_cpControl(cp, maxSideCountry[AXIS]);

		otherSide = (cp.controllingSide == ALLIED) ? AXIS : ALLIED;

		if ( cp.facilitiesOwned[otherSide] == 0 )
			cp.setContention(false);
		else
		{
			FacilityStruct* fac = cp.facilityList;
			while ( fac != nullptr )
			{
				if ( fac->side != cp.side )
				{
					std::string cpDetail = cp.name;
					cpDetail += "/";
					cpDetail += _sideNameAdj[cp.side];
					std::string facDetail = fac->name;
					facDetail += "/";
					facDetail += _sideNameAdj[fac->side];
					strtAddErrorListEntry(ERR_DEFINED_CONTESTED, cp.name, cpDetail.c_str(), facDetail.c_str());
					break;
				}
				fac = fac->next;
			}
			cp.setContention(true);
		}

#ifdef IS_STRAT_HOST
		if ( cp.isContested() )
			cp.changeOwnership(cp.country, false, true);

		if (memcmp(cpSnapshot, &cp, sizeof(*cpSnapshot)) != 0)
		{
			//Save the CP to the DB
# ifdef HEAVY_DEBUG
			dbugLog(_FLI,"fixed cp %s saving to DB",cp.name);
# endif // HEAVY_DEBUG
			strtSaveCP(&cp);
		}
#endif
	}
}

//////////////////////////////////////////////////////////////////////
/// FOR SCRIPTING ONLY

FacilityStruct*
strtGetFacilityByName(const char* name)
{
	CPStruct* cp;
	FacilityStruct* fac;

	for ( cp = Strat::cpList; cp != nullptr; cp = cp->next )
	{
		for ( fac = cp->facilityList; fac != nullptr; fac = fac->next )
		{
			if ( strcasecmp(fac->name, name) == 0 )
				return fac;
		}
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////

CPStruct*
strtGetCPByName(const char* name)
{
	if ( name == nullptr || *name == 0 )
	{
		dbugLog(_FLA, "null or empty name");
		return nullptr;
	}

	if ( Strat::cpList == nullptr )
	{
		dbugLog(_FLA, "called before Strat::cpList initialized");
		return nullptr;
	}

	for ( CPStruct* cp = Strat::cpList; cp != nullptr; cp = cp->next )
	{
		if ( strcasecmp(name, cp->name) == 0 )
			return cp;
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////
// Indexes for quickly looking up cp/facilities by oid
#define MAX_CP_LIST_OIDS		2000
#define	MAX_FACILITY_LIST_OIDS	(MAX_CP_LIST_OIDS*7)

CPStruct* cpOIDList[MAX_CP_LIST_OIDS];
FacilityStruct* facilityOIDList[MAX_FACILITY_LIST_OIDS];
UINT32 g_maxCPOID = 0;

void
strtAddCPoid(CPStruct* newEntry, UINT16 entryNum)
{
	if ( entryNum >= MAX_CP_LIST_OIDS )
	{
		dbugLog(_FLA, "Exceeded maximum CP ID number (%d vs %d)"
						, entryNum
						, MAX_CP_LIST_OIDS
						);
		return;
	}

	cpOIDList[entryNum] = newEntry;
	if ( entryNum > g_maxCPOID )
		g_maxCPOID = entryNum;
}

//////////////////////////////////////////////////////////////////////
// Locate a CPStruct based on its global unique ID

CPStruct*
strtGetCPByOID(UINT16 uOID)
{
#ifdef DEBUG
	if ( uOID >= MAX_CP_LIST_OIDS )
	{
		dbugLog(_FLA, "out-of-bounds cp OID request: #%u", uOID);
		return nullptr;
	}
#endif // DEBUG

	return cpOIDList[uOID];
}


void
strtAddFacilityoid(FacilityStruct* newEntry, UINT16 uEntryNum)
{
	if ( uEntryNum >= MAX_FACILITY_LIST_OIDS )
	{
		dbugLog(_FLA, "Exceeded maximum Facility ID number (%d vs %d)"
						, uEntryNum
						, MAX_FACILITY_LIST_OIDS
						);
		return;
	}

	if ( uEntryNum > uMaxFacilityOID )
		uMaxFacilityOID = uEntryNum;

	facilityOIDList[uEntryNum] = newEntry;
}

FacilityStruct*
strtNextMatchingFacility(const char* string, UINT16 uStartOID)
{
	UINT32 uStringLen = strlen(string);
	FacilityStruct* cur;
	UINT16 uOID;

	for ( uOID = uStartOID; uOID <= uMaxFacilityOID; uOID++ )
	{
		cur = facilityOIDList[uOID];
		if ( cur == nullptr )
			continue;
		if ( strncasecmp(cur->name, string, uStringLen) == 0 )
			return cur;
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////
// Locate a FacilityStruct based on its global unique ID

FacilityStruct*
strtGetFacilityByOID(UINT16 uOID)
{
#ifdef DEBUG
	if ( uOID >= MAX_FACILITY_LIST_OIDS )
	{
		dbugLog(_FLA, "out-of-bounds facility OID request: #%u", uOID);
		return nullptr;
	}
#endif // DEBUG

	return facilityOIDList[uOID];
}

//////////////////////////////////////////////////////////////////////
// Flag a CP as a supply center

#ifndef IS_STRAT_HOST
void
CPStruct::setSupplyCenter()
{
	m_isSupplyCenter = true;
}
#else
// It's defined in strtCPClass.cpp
#endif
