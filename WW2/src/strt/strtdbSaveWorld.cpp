#include <syscfg.h> // Path is project specific.
//
// Created: Thu Oct  6 18:53:24 CDT 2005
//
// Functions to save the game worlda after startup
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith
//

#include "dbug/dbug.h"

#include <stdio.h>
#include <string>
#include <map>

#include "teul/teulBasic.h"
#include "strt/strtTypes.h"
#include "strt/strtCPStruct.h"
#include "strt/strtFacilityStruct.h"
#include "strt/strtObjectStruct.h"
#include "strt/strtLinkStruct.h"
#include "strt/strtData.h"
#include "include/mydbQuery.h"

FILE* fp = NULL;

class WorldSave
{
public:
	enum { CP_COUNTRY, CP_CONCOUNTRY, CP_CONTENTION };
	enum { FAC_COUNTRY, FAC_CAPACITY, FAC_STORED, FAC_OPEN };
	enum { OBJ_COUNTRY, OBJ_DMG, OBJ_MAXDMG, OBJ_CAP, OBJ_MAXCAP, OBJ_RPS };
	enum { LINK_DISABLED };
	enum { WS_CP, WS_FACILITY, WS_OBJECT, WS_LINK, WS_TYPES };
	static const UINT32 BUFFER_SIZE = 10240;
	static const UINT32 MAX_ENTRIES = 1000;

public:
	WorldSave(DBQuery& q) : m_query(q) {}
	~WorldSave() {}

public:
	void add(CPStruct* cp)
	{
		_entry(WS_CP, cp->uOID);
		_add(CP_COUNTRY, cp->country);
		if ( cp->controllingCountry != cp->country ) _add(CP_CONCOUNTRY, cp->controllingCountry);
		if ( cp->isContested() ) _add(CP_CONTENTION, 1);

		for ( FacilityStruct* fac = cp->facilityList; fac != NULL; fac = fac->next )
			add(fac);

		cp->modified = false;
	}

	void add(FacilityStruct* fac)
	{
		_entry(WS_FACILITY, fac->uOID);
		_add(FAC_COUNTRY, fac->country);
		if ( fac->capacity != fac->maxCapacity() ) _add(FAC_CAPACITY, fac->capacity);
		if ( fac->storedRPs != 0 ) _add(FAC_STORED, fac->storedRPs);
		if ( fac->isOpen() == false ) _add(FAC_OPEN, false);
		fac->modified = false;

		for ( ObjectStruct* obj = fac->objectList; obj != NULL; obj = obj->next )
			add(obj);

		if ( fac->m_outgoingLink )
			add(fac->m_outgoingLink);

		if ( fac->m_incomingLink )
			add(fac->m_incomingLink);
	}

	void add(ObjectStruct* obj)
	{
		_entry(WS_OBJECT, obj->m_uid);
		_add(OBJ_COUNTRY, obj->m_country);
		if ( obj->m_damageJoules > 0 ) _add(OBJ_DMG, obj->m_damageJoules);
		_add(OBJ_MAXDMG, obj->m_maxDamageJoules);
		if ( obj->m_capacity > 0 ) _add(OBJ_CAP, obj->m_capacity);
		if ( obj->m_maxCapacity != 1000 ) _add(OBJ_MAXCAP, obj->m_maxCapacity);
		if ( (SINT32)obj->m_repairRPs >= 0 ) _add(OBJ_RPS, obj->m_repairRPs);
		obj->modified = false;
	}

	void add(LinkStruct* link)
	{
		_entry(WS_LINK, link->m_uid);
		if ( !link->isEnabled() )
			_add(LINK_DISABLED, 1);
		link->modified = false;
	}

	void save();

private:
	typedef std::set<UINT32> UIDS;
	typedef std::map<UINT64,UIDS> VALUE_MAP;
	typedef std::map<UINT32,VALUE_MAP> CATEGORY_MAP;
	typedef std::map<UINT32,CATEGORY_MAP> TYPE_MAP;

	void _entry(UINT32 type, UINT32 uid) { m_type = type; m_uid = uid; }

	void _add(UINT32 category, UINT64 value)
	{
		m_entry[m_type][category][value].insert(m_uid);
	}

	void _saveBuffer(UINT32& next)
	{
		buffer[next++] = ')';
		buffer[next] = 0;
		m_query.Query(next, buffer);
		buffer[next++] = ';';
		buffer[next++] = '\n';
		buffer[next] = 0;
		fprintf(fp, "%s", buffer);
	}

	void _query(const char* str)
	{
		m_query.Query(strlen(str), str);
		fprintf(fp, "%s\n", str);
	}

	void _addQueries(const UIDS& uids)
	{
		UINT32 entries = 0;
		UIDS::iterator it;
		UINT32 prefixEnd = strlen(buffer);
		UINT32 next = prefixEnd;
		for ( it = uids.begin(); it != uids.end(); ++it )
		{
			if ( next > prefixEnd )
				buffer[next++] = ',';
			next += sprintf(buffer + next, "%u", *it);
			++entries;
			if ( entries > MAX_ENTRIES || next > (BUFFER_SIZE - 16) )
			{
				_saveBuffer(next);
				entries = 0;
				next = prefixEnd;
			}
		}
		if ( entries > 0 )
			_saveBuffer(next);
	}

	void _addQueriesSimple(VALUE_MAP& map, const char* format);

	void _saveCPs(CATEGORY_MAP&);
	void _saveFacs(CATEGORY_MAP&);
	void _saveObjs(CATEGORY_MAP&);
	void _saveLinks(CATEGORY_MAP&);

	TYPE_MAP	m_entry;
	UINT32		m_type;
	UINT32		m_uid;
	DBQuery&	m_query;

	char		buffer[BUFFER_SIZE];
};

void
WorldSave::_addQueriesSimple(VALUE_MAP& map, const char* format)
{
	for ( auto it = map.begin(); it != map.end(); ++it)
	{
		sprintf(buffer, format, (UINT64)it->first);
		_addQueries(it->second);
	}
}

void
WorldSave::_saveCPs(CATEGORY_MAP& map)
{
	if ( map.empty() )
	{
		dbugLog(_FLA, "no cps to save");
		return;
	}

	_query("UPDATE strat_cp SET contention=0");

	// Save countries
	for ( auto it = map[CP_COUNTRY].begin(); it != map[CP_COUNTRY].end(); ++it)
	{
		UINT32 country = (UINT32)it->first;
		UINT32 side = (UINT32)wwiiSide(country);
		sprintf(buffer, "UPDATE strat_cp SET country=%u,side=%u,concountry=%u,conside=%u WHERE cp_oid IN (", country, side, country, side);
		_addQueries(it->second);
	}

	for ( auto it = map[CP_CONCOUNTRY].begin(); it != map[CP_CONCOUNTRY].end(); ++it)
	{
		UINT32 country = (UINT32)it->first;
		UINT32 side = (UINT32)wwiiSide(country);
		sprintf(buffer, "UPDATE strat_cp SET concountry=%u,conside=%u WHERE cp_oid IN (", country, side);
		_addQueries(it->second);
	}

	_addQueriesSimple(map[CP_CONTENTION], "UPDATE strat_cp SET contention=%u WHERE cp_oid IN (");
}

void
WorldSave::_saveFacs(CATEGORY_MAP& map)
{
	if ( map.empty() )
	{
		dbugLog(_FLA, "no facilities to save");
		return;
	}

	_query("UPDATE strat_facility SET open=1,storedrps=0,capacity=1000");

	for ( auto it = map[FAC_COUNTRY].begin(); it != map[FAC_COUNTRY].end(); ++it )
	{
		UINT32 country = (UINT32)it->first;
		UINT32 side = (UINT32)wwiiSide(country);
		sprintf(buffer, "UPDATE strat_facility SET country=%u,side=%u WHERE facility_oid IN(", country, side);
		_addQueries(it->second);
	}

	_addQueriesSimple(map[FAC_CAPACITY], "UPDATE strat_facility SET capacity=%llu WHERE facility_oid IN(");
	_addQueriesSimple(map[FAC_STORED], "UPDATE strat_facility SET storedrps=%llu WHERE facility_oid IN(");
	_addQueriesSimple(map[FAC_OPEN], "UPDATE strat_facility SET open=%llu WHERE facility_oid IN(");
}

void
WorldSave::_saveObjs(CATEGORY_MAP& map)
{
	ABORT_IF_EX( map.empty(), "no objects to save" );

	_query("UPDATE strat_object SET damagejoules=0,capacity=0,lastcapacity=0,maxcapacity=1000,repairrps=0");

	for ( auto it = map[OBJ_COUNTRY].begin(); it != map[OBJ_COUNTRY].end(); ++it )
	{
		UINT32 country = (UINT32)it->first;
		UINT32 side = (UINT32)wwiiSide(country);
		sprintf(buffer, "UPDATE strat_object SET country=%u,side=%u WHERE object_oid IN(", country, side);
		_addQueries(it->second);
	}

	for ( auto it = map[OBJ_CAP].begin(); it != map[OBJ_CAP].end(); ++it )
	{
		sprintf(buffer, "UPDATE strat_object SET capacity=%llu,lastcapacity=%llu WHERE object_oid IN(", it->first, it->first);
		_addQueries(it->second);
	}

	_addQueriesSimple(map[OBJ_DMG], "UPDATE strat_object SET damagejoules=%llu WHERE object_oid IN (");
	_addQueriesSimple(map[OBJ_MAXDMG], "UPDATE strat_object SET maxdamagejoules=%llu WHERE object_oid IN (");
	_addQueriesSimple(map[OBJ_MAXCAP], "UPDATE strat_object SET maxcapacity=%llu WHERE object_oid IN (");
	_addQueriesSimple(map[OBJ_RPS], "UPDATE strat_object SET repairrps=%llu WHERE object_oid IN (");
}

void
WorldSave::_saveLinks(CATEGORY_MAP& map)
{
	ABORT_IF_EX( map.empty(), "no objects to save" );

	_query("UPDATE strat_link SET disabled=0");
	_addQueriesSimple(map[LINK_DISABLED], "UPDATE strat_link SET disabled=%u WHERE link_oid IN (");
}

void
WorldSave::save()
{
	unlink("/tmp/save.sql");
	fp = fopen("/tmp/save.sql", "wb");

	_query("LOCK TABLES strat_cp write, strat_facility write, strat_object write, strat_link write");

	_saveCPs(m_entry[WS_CP]);
	teulUpdateTransport(5);
	_saveFacs(m_entry[WS_FACILITY]);
	teulUpdateTransport(5);
	_saveObjs(m_entry[WS_OBJECT]);
	teulUpdateTransport(5);
	_saveLinks(m_entry[WS_LINK]);

	_query("UNLOCK TABLES");

	fclose(fp);
}

void
strtdbSaveWorld(CPStruct* cp, DBQuery& q)
{
	return;

	dbugLog(_FLI, "%s", __FUNCTION__);
	WorldSave worldSave(q);

	for (; cp != NULL; cp = cp->next )
	{
		worldSave.add(cp);
	}

	worldSave.save();
}
