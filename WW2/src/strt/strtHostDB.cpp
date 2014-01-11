#include <syscfg.h> // Path is project specific.
// Created: Mon Dec 12 18:37:57 CST 2005
//
// StratDB functions exclusive to the stratd
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "versionNo.h"

#include "strt/strtDB.h"
#include "strt/strtHostDB.h"
#include "wwii/wwiiThreads.h"
#include "wwii/wwiiThreadTypes.h"
#include "wwii/wwiiSerialNumber.h"

#include "dbse/dbseClient.h"
#include "dbse/dbseWrapper.h"
#include "include/mydbQuery.h"
#include "strt/strtdbSaveWorld.h"
#include "strt/strtCPStruct.h"
#include "strt/strtData.h"
#include "strt/strtFacilityStruct.h"
#include "strt/strtHostMessages.h"
#include "strt/strtLinkStruct.h"
#include "strt/strtObjectStruct.h"
#include "strt/strtServerTypes.h"
#include "strt/strtSpawn.h"
#include "strt/strtSupply.h"
#include "strt/strtTOE.h"
#include "weap/weapList.h"
#include "wwii/wwiiConfig.h"
#include "wwii/wwiiDBThreads.h"
#include "wwii/wwiiHCUnits.h"
#include "wwii/wwiiString.h"
#include "wwii/wwiiThreads.h"
#include "wwii/wwiiThreadText.h"
#include "strt/strtObjectiveStats.h"
#include "strt/strtDotCommands.h"
#include "teul/teulTime.h"
#include "include/wthrTime.h"
#include "wwii/wwiiThreads.h"
#include "weap/weapLoadIndexes.h"

#ifdef IS_STRAT_HOST
#include "strt/strtHCUnits.h"
#endif

static const char QRY_SAVE_VEHICLE_ACCESS[] =
	"UPDATE LOW_PRIORITY wwii_vehtype"
		" SET campaign_ready = 1"
			", initial_rank = %u"
			", current_rank = %u"
			", next_cycle_rank = %u"
			", cvc_value = %u"
		" WHERE vehtype_oid = %u"
;

static const UINT32 MAX_QUEUE_SIZE = 256000;

struct QUEUE
{
	const char* name;
	void* obj[MAX_QUEUE_SIZE];
	UINT32 uHead, uTail;

public:
	bool isEmpty() const { return obj[uHead] == NULL; }
};

static QUEUE objQueue, weaponQueue;

bool _queueEnabled = false;

extern DBHANDLE dbh;

UINT32 g_gameDay;

static SINT32
_addToQueue(void* object, QUEUE* queue)
{
	if (queue->obj[queue->uTail] != NULL)
	{
		dbugLog(_FLA,"%s queue out of space [h=%d,t=%d], object not added!"
					, queue->name, queue->uHead, queue->uTail);
		return -1;
	}

	queue->obj[queue->uTail] = object;
	queue->uTail++;
	if (queue->uTail >= MAX_QUEUE_SIZE)
		queue->uTail = 0;

	return queue->uTail;
}

static void*
_removeFromQueue(QUEUE* queue)
{
	void* retval = NULL;

	if ( queue->isEmpty() )
		return retval;

	retval = queue->obj[queue->uHead];
	queue->obj[queue->uHead] = NULL;
	queue->uHead++;
	if (queue->uHead >= MAX_QUEUE_SIZE)
		queue->uHead = 0;

	return retval;
}

char*
strAdd(char* dest, char* src)
{
	while ( *src )
	{
		*(dest++) = *(src++);
	}

	return dest;
}

#include <map>
#include <set>
#include "wwii/wwiiThreadTextQuery.h"

typedef std::map< std::string, std::set<UINT32> > SAVE_SET;
static SAVE_SET _cpSaves, _facSaves, _objOwnSaves, _objMaxSaves, _objStateSaves;

static void
_saveData(SAVE_SET& saves, const char* const tableName, const char* const indexField)
{
	std::string prefix;
	char id[64];

	for ( SAVE_SET::iterator vit = saves.begin(); vit != saves.end(); ++vit )
	{
		prefix = "UPDATE ";
		prefix += tableName;
		prefix += " SET ";
		prefix += vit->first;
		prefix += " WHERE ";
		prefix += indexField;
		prefix += " IN (";

		ThreadTextQuery query(DBThread::Default, prefix.c_str(), ")");

		for ( std::set<UINT32>::iterator idit = vit->second.begin(); idit != vit->second.end(); ++idit )
		{
			sprintf(id, "%u", *idit);
			query.addEntry(id);
		}

		query.commit(0);
	}

	saves.clear();
}

static void
_commitCP(CPStruct* cp)
{
	UINT16 attacker = (cp->isCity() && cp->attackingUnit() != NULL) ? cp->attackingUnit()->unitID() : 0;
	char valuesKey[128];
	sprintf(valuesKey, "conside=%u,concountry=%u,side=%u,country=%u,contention=%u,is_objective=%u"
			, cp->controllingSide
			, cp->controllingCountry
			, cp->side
			, cp->country
			, (cp->isContested()) ? 1 : 0
			, attacker
			);
	_cpSaves[valuesKey].insert(cp->uOID);
	cp->modified = false;
}

static void
_commitFacility(FacilityStruct* fac)
{
	char valuesKey[128];
	sprintf(valuesKey, "side=%u,country=%u,capacity=%d,storedrps=%d,open=%u"
			, fac->side
			, fac->country
			, fac->capacity
			, fac->storedRPs
			, fac->isOpen() ? 1 : 0
			);
	_facSaves[valuesKey].insert(fac->uOID);

	fac->modified = false;
}

template<typename Type_>
static bool
_needsSave(const Type_& value, Type_& saved)
{
	if ( value == saved )
		return false;
	saved = value;
	return true;
}

static void
_commitObject(ObjectStruct* obj)
{
	char valuesKey[80];
	sprintf(valuesKey, "side=%u,country=%u", obj->m_side, obj->m_country);
	_objOwnSaves[valuesKey].insert(obj->m_uid);
	if ( _needsSave(obj->m_maxDamageJoules, obj->m_maxDamageJoulesSaved) || _needsSave(obj->m_maxCapacity, obj->m_maxCapacitySaved) )
	{
		sprintf(valuesKey, "maxdamagejoules=%llu,maxcapacity=%u"
				, obj->m_maxDamageJoules
				, obj->m_maxCapacity
				);
		_objMaxSaves[valuesKey].insert(obj->m_uid);
	}
	if ( _needsSave(obj->m_damageJoules, obj->m_damageJoulesSaved)
			|| _needsSave(obj->m_capacity, obj->m_capacitySaved)
			|| _needsSave(obj->m_lastCapacity, obj->m_lastCapacitySaved)
			|| _needsSave(obj->m_repairRPs, obj->m_repairRPsSaved)
		)
	{
		sprintf(valuesKey, "damagejoules=%llu"
				",capacity=%d"
				",lastcapacity=%d"
				",repairrps=%d"
				, obj->m_damageJoules
				, obj->m_capacity
				, obj->m_lastCapacity
				, obj->m_repairRPs
				);
		_objStateSaves[valuesKey].insert(obj->m_uid);
	}
	obj->modified = false;
}

typedef std::vector<UINT16> _SAVE_IDS;

static void
_saveRows(const char* table, const char* field, UINT64 value, const char* idField, _SAVE_IDS& ids)
{
	if ( ids.empty() )
		return;

	char* query = threadsAllocText();
	char* p = query;

	p += sprintf(p, "UPDATE %s SET %s=%llu WHERE %s IN (", table, field, value, idField);
	_SAVE_IDS::iterator it;
	for ( it = ids.begin(); it != ids.end(); ++it )
	{
		p += sprintf(p, "%hu,", *it);
	}
	// Replace the last , with a close parenthesis
	*(p-1) = ')';

	// Send it to the daemon
	DBThread::Do(DBThread::Default, query);

	ids.clear();
}

static void
_commitLink(LinkStruct* link)
{
	static _SAVE_IDS _enabled, _disabled;

	if ( link != NULL )
	{
		if ( link->isEnabled() )
			_enabled.push_back(link->m_uid);
		else
			_disabled.push_back(link->m_uid);
		link->modified = false;
	}

	if ( link == NULL || _enabled.size() > 200 )
		_saveRows("strat_link", "disabled", 0, "link_oid", _enabled);
	if ( link == NULL || _disabled.size() > 200 )
		_saveRows("strat_link", "disabled", 0, "link_oid", _disabled);
}

void
strtProcessDBQueue()
{
	// strtBroadcastStratUpdates returns true when the packet sent
	// contained cps and facilities. To help the draining process,
	// send two packets when that occurs
	strtBroadcastStratUpdates();

	FULL_OBJ_ID* object = NULL;

	while ( objQueue.isEmpty() == false )
	{
		object = (FULL_OBJ_ID*)_removeFromQueue(&objQueue);
		if (object == NULL)
			break;

		switch(object->objClass)
		{
		case CLASS_CP:
			_commitCP((CPStruct*)object);
		break;

		case CLASS_FACILITY:
			_commitFacility((FacilityStruct*)object);
		break;

		case CLASS_BUILDING:
			_commitObject((ObjectStruct*)object);
		break;

		case CLASS_LINK:
			_commitLink((LinkStruct*)object);
		break;

		default :
			LogAbort("Queue processing excpetion! Unhandled case.");
		break;
		}
	}

	_saveData(_cpSaves, "strat_cp", "cp_oid");
	_saveData(_facSaves, "strat_facility", "facility_oid");
	_saveData(_objOwnSaves, "strat_object", "object_oid");
	_saveData(_objMaxSaves, "strat_object", "object_oid");
	_saveData(_objStateSaves, "strat_object", "object_oid");

	// Save off any uncommitted link state changes
	_commitLink(NULL);
}

void
strtSaveCP(CPStruct* cp)
{
	if ( cp == NULL || !_queueEnabled || cp->modified )
		return;

	_addToQueue(cp, &objQueue);
	cp->modified = true;
}

void
strtSaveFacility(FacilityStruct* facility)
{
	if ( facility == NULL || !_queueEnabled || facility->modified )
		return;

	_addToQueue(facility, &objQueue);
	facility->modified = true;
}

bool
ObjectStruct::save()
{
	if ( modified == true || _queueEnabled == false )
		return false;

	_addToQueue(this, &objQueue);

	return (modified = true);
}

//////////////////////////////////////////////////////////////////////
// Save the game world state as much as possible

void
strtdbSaveGameWorld(CPStruct* cp)
{
	_queueEnabled = true;

	try
	{
		DBQuery query(dbh);
		strtdbSaveWorld(cp, query);
		dbugLog(_FLI, "World state saved");
	}
	catch (std::exception &e)
	{
		LogAbort("Failed to save world state: %s", e.what());
	}
}

//////////////////////////////////////////////////////////////////////
// Save our entire vehicle list to the database so that any new
// entries in th_blitz are reflected in the database.

void
strtdbPopulateVehTypesTable()
{
	dbugLog(_FLD, "populating vehtype table");

	std::string inserts =
		"INSERT IGNORE into wwii_vehtype"
			"(countryID,categoryID,classID,typeID,fullName,initial_rank,current_rank,next_cycle_rank,campaign_ready,cvc_value,blitz_name,added,weapon_spec) VALUES";
	std::string updates =
		"UPDATE wwii_vehtype SET in_service=1 WHERE weapon_spec IN (";
	bool needInsert = false;
	static const size_t _BUF_SIZE = 512;
	char buf[_BUF_SIZE+1];

	dbugLog(_FLI, "%s", inserts.c_str());

	for ( auto it = g_weaponList.begin(), end = g_weaponList.end(); it != end; ++it )
	{
		assert( it->second != NULL );

		WEAPON& weap = *it->second;

		if ( weap.localIndex() > 0 )
			LogAbort("weapon entry already has a local index [called %s after vehicles assigned indexes]", __FUNCTION__);

		const WEAPON_SPEC& spec = it->first;

		if ( spec.wCountry == 0 || spec.wCountry > MAX_COUNTRIES )
		{
			LogAbort("illegal weapon spec %s", weapPrintSpecC(&spec));
			continue;
		}

		snprintf(buf, _BUF_SIZE, "%c(%u,%u,%u,%u,\"%s\",%u,%u,%u,1,%u,'%s',now(),%u)"
								, (needInsert ? ',' : ' ')
								, spec.wCountry, spec.wCategory, spec.wClass, spec.wType
								, weap.name
								, weap.access(), weap.access(), weap.access()
								, weap.value
								, weap.name
								, spec.asUINT32()
								);
		dbugLog(_FLIB, "%s", buf);
		inserts += buf;

		snprintf(buf, _BUF_SIZE, (needInsert ? ",%u" : "%u"), spec.asUINT32());
		updates += buf;

		needInsert = true;
	}

	// There were no vehicles.
	if ( needInsert == false )
		return;

	updates += ")";					// Add close bracket to updates

	// Maintain the wwii_vehtype table so that all old entries are
	// flagged as "in_service=0" and all current th_blitz entries are
	// guaranteed to be in the table and marked as "in_service=1"
	DBQuery q(dbh);
	try
	{
		q.Query("BEGIN");
		/// Transaction
		{
			// Change all the pre-existing vehicles to be marked as "not in service"
			q.Query("UPDATE wwiionline.wwii_vehtype v SET v.in_service = 0");

			// Insert the new vehicles
			q.Query(inserts.size(), inserts.c_str());

			// Now flag the list of vehicles we know about as in-service
			q.Query(updates.size(), updates.c_str());
		}
		q.Query("COMMIT");
	}
	catch (std::exception &e)
	{
		try { q.Query("ROLLBACK"); } catch ( ... ) { }
		LogAbort("vehtype insert failed: %s", e.what());
	}
}

//////////////////////////////////////////////////////////////////////
// Mark all vehicles as modified so that they get saved

void
strtdbSaveVehicleCVCs()
{
	dbugLog(_FLD, "saving vehicle cvcs");

	DBQuery q(dbh);
	try
	{
		static const UINT32 _BUF_SIZE = 300;
		char buf[_BUF_SIZE+1];
		int length;
		UINT32 saved = 0;

		q.Query("START TRANSACTION");

		for ( auto it = g_weaponList.begin(), end = g_weaponList.end(); it != end; ++it )
		{
			WEAPON* weap = it->second;
			if ( weap == NULL || weap->localIndex() == 0 )
				continue;

			length = snprintf(buf, _BUF_SIZE
								, "UPDATE LOW_PRIORITY wwii_vehtype SET cvc_value=%u WHERE vehtype_oid=%u AND cvc_value != %u"
									, weap->value, weap->localIndex(), weap->value
									);
			q.Query(length, buf);

			++saved;
		}

		q.Query("COMMIT");
	}
	catch (std::exception &e)
	{
		dbugLog(_FLA, "vehicle cvc value save failed: %s", e.what());
		q.Query("ROLLBACK");
	}
}

//////////////////////////////////////////////////////////////////////
// Save all WEAPONs with a non-zero modified field

///TODO:+TOE Move global (WEAPON*) vehicle rank restrictions into TOEs
void
strtdbSaveVehicleAccess()
{
	dbugLog(_FLD, "saving vehicle access");

	try
	{
		DBQuery q(dbh);
		static const UINT32 _BUF_SIZE = 250;
		char buf[_BUF_SIZE+1];
		int length;
		UINT32 saved = 0;

		for ( auto it = g_weaponList.begin(), end = g_weaponList.end(); it != end; ++it )
		{
			WEAPON* weap = it->second;
			if ( weap == NULL || weap->localIndex() == 0 || !weap->m_modified )
				continue;
			length = snprintf(buf, _BUF_SIZE
								, QRY_SAVE_VEHICLE_ACCESS
									, weap->initialAccess(), weap->access(), weap->access(), weap->value
									, weap->localIndex());
			q.Query(length, buf);
			weap->m_modified = false;
			++saved;
		}

		dbugLog(_FLD, "saved %u vehicle access entries", saved);
	}
	catch (std::exception &e)
	{
		dbugLog(_FLA, "vehicle access save failed: %s", e.what());
	}
}

//////////////////////////////////////////////////////////////////////
// Read in all of the vehicle access records and update any that need it

///TODO:+TOE:Minor Move global (WEAPON*) vehicle rank restrictions into TOEs
void
strtdbReadVehicleAccess()
{
	dbugLog(_FLI, "reading vehicle access");

	try
	{
		// Load 'em in.
		weapLoadIndexes(dbh);

		// Save the values straight back out
		strtdbSaveVehicleAccess();
	}
	catch (std::exception& e)
	{
		dbugLog(_FLA,"Failed reading vehicle access ranks: %s", e.what());
	}

	DBQuery q(dbh, "SELECT vehtype_oid FROM wwii_vehtype WHERE blitz_name is NULL");
	while ( q.FetchRow() )
	{
		UINT32 localIndex = q.Row().NextUInt();
		WEAPON* weap = weapLookupWeaponByIndex(localIndex);
		if ( weap == NULL )
		{
			dbugLog(_FLD, "no vehtype_oid entry for %u", localIndex);
			continue;
		}

		DBSimpleQuery(dbh, "UPDATE wwii_vehtype SET blitz_name='%s' WHERE vehtype_oid=%u"
							, weap->name
							, localIndex);
	}
}

bool
strtInitHostDB()
{
	memset(&objQueue,0,sizeof(objQueue));
	objQueue.name = "Object";
	memset(&weaponQueue,0,sizeof(weaponQueue));
	weaponQueue.name = "Weapon";

	return true;
}

//////////////////////////////////////////////////////////////////////
// Save the output of a particular factory

void
strtSaveFactoryOutput(ObjectStruct* factory)
{
#if defined(TRAINING_SERVER) || defined(DEV_BUILD) || defined(FRES)
	return;
#else
	if ( host_config.arena.intermission > 0 || factory == NULL )
		return;

	UINT32 contributing = 0;
	UINT32 visLevel = 0;
	if ( factory->visLevel() == 0 )
	{
		visLevel = 1;
		if ( factory->facility->side == factory->facility->originalSide )
			contributing = 1;
	}

	static const char* QUERY_SAVE_FACTORY =
		"INSERT INTO strat_factory_outputs"
			" (facility_oid, output_time"
			" , country, side, vis_level, damage_pctg, contributing)"
			" VALUES (%d, now(), %d, %d, %d, %d, %d)"
			;

	DBThread::Do(DBThread::Default
				, QUERY_SAVE_FACTORY
					, factory->facility->uOID
					, factory->m_country
					, factory->m_side
					, visLevel
					, factory->percentDamage
					, contributing
					);

	dbugLog(_FLD, "Saving factory %s: %u,%d,%d,%d,%d :: %d"
					, factory->name
					, factory->facility->uOID
					, factory->m_country
					, factory->m_side
					, visLevel
					, factory->percentDamage
					, contributing
					);
#endif
}

//////////////////////////////////////////////////////////////////////
// Save an objective stats record -- we don't care about the time
// value here...

void
ObjectiveStats::save()
{
#if !defined(TRAINING_SERVER) && !defined(FRES) && !defined(DEV_BUILD)
	static const UINT32 OBJECTIVE_STAT_SAVE_INTERVAL = 600;	// Save after this many seconds
	static UINT32 nextSaveTime = teulTime();					// First time round, save now

	if ( teulTime() <= nextSaveTime )
		return;

	dbugLog(_FLD, "Saving objective stats");

	DBThread::Do(DBThread::Audit
			, "INSERT INTO audit.objective_counts"
				" ( recorded"
				 ", city_limit"
				 ", allied_city_attacks"
				 ", axis_city_attack"
				 ", bridge_limit"
				 ", allied_bridge_attacks"
				 ", axis_bridge_attacks"
				 ", allied_bridge_defends"
				 ", axis_bridge_defends"
				 ", allied_fights"
				 ", axis_fights"
				 ")"
				" VALUES (FROM_UNIXTIME(%u), %u, %u, %u, %u, %u, %u, %u, %u, %u, %u)"
			, teulTime()
			, m_cityLimit
			, m_cityAttacks[ALLIED]
			, m_cityAttacks[AXIS]
			, m_bridgeLimit
			, m_bridgeAttacks[ALLIED]
			, m_bridgeAttacks[AXIS]
			, m_bridgeDefends[ALLIED]
			, m_bridgeDefends[AXIS]
			, m_battles[ALLIED]
			, m_battles[AXIS]
	);

	nextSaveTime = teulTime() + OBJECTIVE_STAT_SAVE_INTERVAL;
#endif
}

//////////////////////////////////////////////////////////////////////
// Log a capture event and send DBSE a note to record it for posterity
//

void
strtCaptureLog(FacilityStruct* facility, UINT32 state, UINT32 fromCountry
				, const ChatHandle& handle, UINT32 customerID, UINT32 missionID
				, const HCUnit* unit, UINT32 started, UINT32 seconds
				, bool wasContested, bool tookControl, bool tookOwnership)
{
	const char* contentionState;
	if ( !wasContested && facility->cp->isContested() == wasContested )
		contentionState = "Uncon";
	else if ( !wasContested )
		contentionState = "Enter";
	else if ( wasContested && facility->cp->isContested() == wasContested )
		contentionState = "IsCon";
	else
		contentionState = "End";

	// We found that recording the start events pummeled the database,
	// and generated WAY too many entries. So we now only record
	// capture and cheat events.
	if ( state != CAP_START )
	{
		DBThread::Do(DBThread::Default
					, "INSERT INTO strat_captures"
						" (state, customerID, cust_country, brigade"
						" , facil_country, started_at, seconds, missionID"
						" , facility_oid"
						" , contention, took_control, took_ownership)"
						" VALUES"
						" (%d, %u, %u, %u"
						" , %u, %u, %u, %u"
						" , %u"
						" , '%s', %u, %u"
						" )"
						, state, customerID, unit->country(), unit->unitID()
						, fromCountry, started, seconds, missionID
						, facility->uOID
						, contentionState, tookControl, tookOwnership
					);
	}

	switch (state)
	{
		case CAP_START:
			dbugLog(_FLI, "cap: START %s#%d(%s)@%s(%s %s) requires %ds cs:%s"
							, Chat::Name(handle).c_str()
							, customerID
							, _countryCode[unit->country()]
							, unit->callsign()
							, facility->name
							, _countryCode[fromCountry]
							, seconds
							, contentionState
							);
		break;
		case CAP_ILLEGAL:
			dbugLog(_FLW, "cap: ILLEGAL %s#%d(%s)@%s(%s %s) started %d took %d seconds"
							, Chat::Name(handle).c_str()
							, customerID
							, _countryCode[unit->country()]
							, unit->callsign()
							, facility->name
							, _countryCode[fromCountry]
							, started
							, seconds
							);
		break;
		case CAP_CAPTURED:
			dbugLog(_FLI, "cap: CAPTURED %s#%d(%s)@%s(%s %s) started %u took %u seconds cs:%s ctrl:%s own:%s"
							, Chat::Name(handle).c_str()
							, customerID
							, _countryCode[unit->country()]
							, unit->callsign()
							, facility->name
							, _countryCode[fromCountry]
							, started
							, seconds
							, contentionState
							, ((tookControl) ? _sideName[facility->side] : "same")
							, ((tookOwnership) ? _sideName[facility->side] : "same")
							);
		break;
		case CAP_SAVED:
			dbugLog(_FLI, "cap: SAVED %s#%d(%s)@%s(%s %s) started %d took %d seconds"
							, Chat::Name(handle).c_str()
							, customerID
							, _countryCode[unit->country()]
							, unit->callsign()
							, facility->name
							, _countryCode[fromCountry]
							, started
							, seconds
							);
		break;
		default:
			dbugLog(_FLA, "unknown CAP_ type %d", state);
		break;
	}
}

bool
strtdbFetchLuaScript(const char* name, std::string& obj)
{
	try
	{
		DBQuery q(dbh, "SELECT code FROM strat_scripts WHERE name = '%s'", name);
		if ( q.Rows() > 0 && q.FetchRow() )
		{
			obj = q.Row().NextString();
			if ( obj.size() > 0 )
				return true;
		}
	}
	catch ( std::exception& e )
	{
		dbugLog(_FLA, "%s(%s) error %s", __FUNCTION__, name, e.what());
	}

	return false;
}

void
strtdbSaveConfigVar(const char* name, const char* value)
{
	DBSimpleQuery(dbh, "UPDATE host_config SET value = '%s' WHERE variableName = '%s'", value, name);
}

//////////////////////////////////////////////////////////////////////
// Load TOE Templates - this should probably occur in a background
// thread.
///CAVEAT: Must be done before we load in the HCUnits.

static const char* QRY_BOOTSTRAP_TOE_HEADERS =
"INSERT INTO toe.headers"
	"(id, name, country, branch)"
" VALUES"
	" ('ukgrdfor', 'BEF Ground Forces', 1, 1)"
	",('ukairfor', 'RAF Fighter Forces', 1, 2)"
	",('ukseafor', 'RAN Naval Forces', 1, 3)"
	",('frgrdfor', 'French Ground Forces', 3, 1)"
	",('frairfor', 'Armee de l\\'Air Forces', 3, 2)"
	",('frseafor', 'French Naval Forces', 3, 3)"
	",('degrdfor', 'German Ground Forces', 4, 1)"
	",('deairfor', 'Luftwaffe Fighter Forces', 4, 2)"
	",('deseafor', 'Kreigsmarine Forces', 4, 3)"
;

void
strtdbLoadTOETemplates()
{
	static UINT32 lastLoad = 0;
	char statement[512];

	dbugLog(_FLI, "Loading TOE Templates");

	try
	{
		DBQuery q(dbh);

		// Take the current database timestamp so that we can perform
		// incremental reads in subsequent calls.
		q.Query("SELECT COUNT(*), UNIX_TIMESTAMP(NOW()) FROM toe.headers t");
		if ( q.Rows() == 0 || !q.FetchRow() )
			throw "Failed reading 'now' from database";

		// We want the time before we start our actual reads because
		// our reads may occur in parallel to changes being made
		// whereas a simple call to read the database time should
		// occur well before any such overlap.
		DBRow& hdrRow = q.Row();
		UINT32 numHeaders = hdrRow.NextUInt();
		UINT32 thisLoad = hdrRow.NextUInt();

		// Do we need to bootstrap the TOEs?
		if ( numHeaders == 0 )
			DBSimpleQuery(dbh, QRY_BOOTSTRAP_TOE_HEADERS);

		snprintf(statement, 500
				, "SELECT DISTINCT t.id, t.name, t.country, t.branch"
					" FROM toe.headers t"
						" LEFT JOIN toe.vehicles v USING (id)"
					" WHERE t.modified >= FROM_UNIXTIME(%u)"
						" OR v.modified >= FROM_UNIXTIME(%u)"
					" GROUP BY 1"
					" ORDER BY 1 ASC"
						, lastLoad, lastLoad
				);
		q.Query(statement);

		if ( q.Rows() == 0 && lastLoad == 0 )
			throw std::domain_error("No TOE Templates present!");

		while ( q.FetchRow() )
		{
			DBRow& row = q.Row();
			const char* id = row.NextColumn();
			const char* name = row.NextColumn();
			UINT32 country = row.NextUInt();
			UINT32 branch = row.NextUInt();

			dbugLog(_FLD, "unit:%s toe:%s", id, name);
			strtLoadTOEWeaponTemplateHeader(id, name, country, branch);
		}

		strprintf(statement
				, "SELECT v.id, vt.countryID, vt.categoryID, vt.classID, vt.typeID, v.capacity, v.resupply_time_m"
					" FROM toe.headers t"
						" JOIN toe.vehicles v USING (id)"
							" JOIN wwii_vehtype vt USING (vehtype_oid)"
					" WHERE vt.in_service=1 AND v.modified >= FROM_UNIXTIME(%u)"
					" ORDER BY 1, 2"
						, lastLoad
				);
		q.Query(statement);
		if ( q.Rows() == 0 && lastLoad == 0 )
		{
			dbugLog(_FLW, "No TOE vehicle lists");
			return;
		}

		WEAPON_SPEC spec;
		while ( q.FetchRow() )
		{
			DBRow& row = q.Row();
			const char* id = row.NextColumn();
			spec.wCountry = row.NextUInt();
			spec.wCategory = row.NextUInt();
			spec.wClass = row.NextUInt();
			spec.wType = row.NextUInt();
			UINT32 capacity = row.NextUInt();
			UINT16 resupplyTime_m = row.NextUInt();
			static const UCHAR access = 0;

			dbugLog(_FLD, "unit:%s spec:%u.%u.%u.%u cap:%u acc:%u resup:%u"
								, id, spec.wCountry, spec.wCategory, spec.wClass, spec.wType, capacity, access, resupplyTime_m);
			strtLoadTOEWeaponInstance(id, spec, capacity, access, resupplyTime_m);
		}

		// Update the watermark so that we only read changes made after these queries
		// including any that may have been blocked by our queries.
		lastLoad = thisLoad;
	}
	catch (std::exception& e)
	{
		LogAbort("Failed reading TOE Templates: %s", e.what());
	}

}

//////////////////////////////////////////////////////////////////////
// Re-load vehicle levels from the database into the TOEs

void
strtdbRestoreTOESupplies()
{
	try
	{
		DBQuery q(dbh, "SELECT s.unit_id, v.countryID, v.categoryID, v.classID, v.typeID, s.present, s.vehtype_oid"
						" FROM toe.supply s JOIN wwii_vehtype v USING (vehtype_oid)"
						" ORDER BY 1, 2");

		StratHCUnit* unit = NULL;
		UINT32 lastUnitID = 0;
		WEAPON_SPEC spec;
		while ( q.FetchRow() )
		{
			UINT32 unitID = q.Row().NextUInt();
			spec.wCountry = q.Row().NextUInt();
			spec.wCategory = q.Row().NextUInt();
			spec.wClass = q.Row().NextUInt();
			spec.wType = q.Row().NextUInt();
			UINT32 present = q.Row().NextUInt();
			UINT32 vehTypeOID = q.Row().NextUInt();

			// See if we've already looked up the unit
			if ( unitID != lastUnitID )
			{
				unit = StratHCUnit::Lookup(unitID);
				if ( unit == NULL )
					dbugLog(_FLW, "toe.supply: unrecognized unit ID %u", unitID);
				lastUnitID = unitID;
			}

			// See if the unit lookup failed.
			if ( unit == NULL )
				continue;

			// Set this as the initial value
			if ( unit->initializeWeapon(spec, present) == false )
			{
				DBThread::Do(DBThread::Default, "DELETE LOW_PRIORITY IGNORE FROM toe.supply WHERE unit_id=%u AND vehtype_oid=%u", unitID, vehTypeOID);
			}
		}
	}
	catch (std::exception& e)
	{
		LogAbort("Failed restoring TOE Supplies: %s", e.what());
	}

	dbugLog(_FLI, "Saving initial TOE levels");
	// Purge the old entries
	DBSimpleQuery(dbh, "TRUNCATE toe.supply");

	char buffer[TEXT_POOL_SIZE+1];
	UINT32 len = 0;

	// Go over every strat unit and iterate thru its TOE
	HCUNIT_INDEX::iterator it;
	for ( it = hcUnits.begin(); it != hcUnits.end(); ++it )
	{
		if ( it->second->isDivision() == false && it->second->isBrigade() == false )
			continue;

		StratHCUnit* unit = (StratHCUnit*)it->second;

		TOE::const_iterator wit;
		for ( wit = unit->toe().begin(); wit != unit->toe().end(); ++wit )
		{
			if ( wit->second.weapon().isSupplied() )
				wit->second.saveText(len, buffer);
		}
		threadsLogging();
	}

	if ( len > 0 )
		DBSimpleQuery(dbh, buffer);
}

//////////////////////////////////////////////////////////////////////
// Start a new day cycle (actually occurs about 4am game time) and
// write out a 'weather report'

void
strtStartNewDay()
{
	static SerialNumber gameDay("gameday", 0, 1, ~0);
	g_gameDay = static_cast<UINT32>(gameDay.getNext());

	wthrStartNewDay();

	DBThread::Do(DBThread::Default
					, "INSERT"
						" INTO `strat`.`wthr_report` (real_start, game_day, start_time, weather_pattern, rain_pattern, weather_forecast)"
						" VALUES (NOW(), %u, %u, %u, %u, %u)"
							, g_gameDay
							, wthrGetGameTime()
							, wthrGetWeatherPattern()
							, wthrGetFogPattern()
							, wthrGetWeatherForecast()
							);
}
