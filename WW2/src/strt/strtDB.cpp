#include <syscfg.h> // Path is project specific.
// Created: Wed May 24 13:51:27 2000
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

#include "strt/strtProtos.h"

#include "wwii/wwiiDBConfig.h"
#include "dbse/dbseHandle.h"
#include "include/mydbQuery.h"
#include "strt/strtCPStruct.h"
#include "strt/strtData.h"
#include "strt/strtFacilityStruct.h"
#include "strt/strtHostDB.h"
#include "strt/strtLinkStruct.h"
#include "strt/strtObjectStruct.h"
#include "strt/strtSupercell.h"
#include "teul/teulTime.h"
#include "wwii/wwiiConfig.h"
#include "wwii/wwiiRank.h"

//#define HEAVY_DEBUG

using _PROCESSOR = void(*)(DBRow&, void*, UINT32 oid);

extern void strtInitializeVehicleAccessLevels();

//////////////////////////////////////////////////////////////////////
//Tool for opening DB connection
bool
strtInitDB()
{
	bool retval = true;

	dbugLog(_FLI, "initializing strat database");
	if ( !dbh.connected() )
		dbh.connect();

#ifdef IS_STRAT_HOST
	if ( !strtInitHostDB() )
		retval = false;
#endif // IS_STRAT_HOST

	srand((UINT32)teulClock64);

	if ( ! dbGetWWIIConfig() )
		dbugLog(_FLA, "No configuration from database");

#ifdef IS_STRAT_HOST
	wwiiReadRanks();

	strtInitializeVehicleAccessLevels();
#endif

	return retval;
}

void
strtCloseDB()
{
	dbh.disconnect();
}

void
strtLoadTOEsAndHCUnits()
{
#ifdef IS_STRAT_HOST
	// Read in the complete list of TOE templates so that we're ready for HCUnits.
	strtdbLoadTOETemplates();
	// Read in the list of HCUnits
#endif

	if ( !wwiiReadHCUnits() )
		dbugLog(_FLA, "Unable to read HC Units");
}

static const char* _FETCH_CPS_QUERY =
	"SELECT scx, scy, cp_id, cp_oid, contention, concountry, country, is_objective"
		" FROM strat_cp"
		" ORDER BY scx,scy"
;
static const char* _FETCH_FACS_QUERY =
	"SELECT scx, scy, facility_id, facility_oid, country, open, building, storedrps, capacity"
		" FROM strat_facility"
		" ORDER BY scx,scy"
;
static const char* _FETCH_OBJS_QUERY =
	"SELECT scx, scy, object_id, object_oid, repairrps, lastcapacity, capacity, damagejoules, country"
		" FROM strat_object"
		" ORDER BY scx,scy"
;
static const char* _FETCH_LINKS_QUERY =
	"SELECT scx, scy, link_id, link_oid, disabled, convoy"
		" FROM strat_link"
		" ORDER BY scx, scy"
;

static void
_processCP(DBRow& row, void* data, UINT32 oid)
{
	CPStruct* cp = (CPStruct*) data;

	cp->uOID = oid;

	cp->setContention((bool)row.NextUInt());
	cp->controllingCountry = (WWII_COUNTRY)row.NextUInt();
	cp->controllingSide = wwiiSide(cp->controllingCountry);
	cp->country = (WWII_COUNTRY)row.NextUInt();
	cp->setSide(wwiiSide(cp->country));

#if defined(IS_STRAT_HOST)
	if ( cp->isCity() )
	{
		UINT32 attackingBrigade = row.NextUInt();
		if ( attackingBrigade != 0 )
		{
			HCUnit* unit = HCUnit::Lookup(attackingBrigade);
			if ( unit && unit->side() != cp->side )
			{
				cp->setAttackObjective(true, unit);
				cp->m_attackStarted = teulTime() - (host_config.objective.minLifetime_m * 60);
			}
		}
	}
#endif

	strtAddCPoid(cp, cp->uOID);
}

static void
_processFacility(DBRow& row, void* data, UINT32 oid)
{
	FacilityStruct* facility = (FacilityStruct*) data;

	facility->uOID = oid;

	facility->setCountry((WWII_COUNTRY)row.NextUInt());
	facility->setOpen(row.NextUInt());
	facility->m_building = row.NextUInt();
	facility->storedRPs = row.NextUInt();
	facility->capacity = row.NextUInt();

	strtAddFacilityoid(facility, facility->uOID);
}

static void
_processObject(DBRow& row, void* data, UINT32 oid)
{
	ObjectStruct* object = (ObjectStruct*) data;

	object->m_uid = oid;

	object->m_repairRPs = row.NextUInt();
	object->m_lastCapacity = row.NextUInt();
	object->m_capacity = row.NextUInt();
	object->m_damageJoules = row.NextUINT64();
	object->m_country = (WWII_COUNTRY)row.NextUInt();
	object->m_side = wwiiSide(object->m_country);

	object->calcPercentDamage();

#ifdef IS_STRAT_HOST
	object->recalculateCapacity();
#endif
}

static void
_processLink(DBRow& row, void* data, UINT32 oid)
{
	LinkStruct* link = (LinkStruct*) data;

	link->m_uid = oid;

	UINT32 disabled = row.NextUInt();
	link->setEnabled(!disabled);
	link->m_convoy = false;
}

static void
_initAllEntries(const char* query, _PROCESSOR processFn)
{
	SINT32 scx = 0, scy = 0;
	UINT32 id = 0;

	DBQuery q(dbh, query);

	if ( q.Rows() <= 0 )
		return;

	Supercell* sc = nullptr;

	while ( q.FetchRow() )
	{
		DBRow& row = q.Row();

		scx = row.NextSInt();
		scy = row.NextSInt();
		id = row.NextUInt();

		if ( !sc || sc->x != scx || sc->y != scy )
			sc = strtGetSupercell(scx, scy);

		if ( !sc )
		{
			dbugLog(_FLI, "Database supercell %d,%d not found", scx, scy);
			throw std::domain_error("Unable to find supercell");
		}

		if ( id > sc->indexSize )
			throw std::domain_error("id outside range for supercell");

		void* data = sc->index[id];
		if ( data == nullptr )
			throw std::domain_error("no matching sdm data");

		UINT32 oid = row.NextUInt();
		if ( oid == 0 )
			throw std::domain_error("zero oid");

		processFn(row, data, oid);
	}
}

void
strtdbInitSupercells()
{
	const char* resource = nullptr;

	try
	{
		resource = "cp";
		_initAllEntries(_FETCH_CPS_QUERY, _processCP);

		resource = "fac";
		_initAllEntries(_FETCH_FACS_QUERY, _processFacility);

		resource = "obj";
		_initAllEntries(_FETCH_OBJS_QUERY, _processObject);

		resource = "link";
		_initAllEntries(_FETCH_LINKS_QUERY, _processLink);
	}
	catch ( std::exception& e )
	{
		LogAbort("failed initializing %ss from database: %s", resource, e.what());
	}
}
