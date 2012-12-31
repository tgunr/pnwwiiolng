#include <syscfg.h> // Path is project specific.
// Created: Mon Apr 27 02:25:33 CDT 2009
//
// Copyright (C) PlayNet, Inc 1999-2009 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "dbug/dbug.h"
#include "include/mydbQuery.h"
#include "weap/weapList.h"					// For weapAddWeaponIndex
#include "wwii/wwiiThreadTextQuery.h"

//	------------------------------	Declarations

static const char QRY_SAVE_VEHICLES[] =
	"INSERT IGNORE INTO `wwiionline`.`wwii_vehtype`"
		"(countryID, categoryID, classID, typeID, fullName, cvc_value, blitz_name, added)"
	"VALUES"
;

void
weapSaveVehicleTypes()
{
	ThreadTextQuery qry(DBThread::Default, QRY_SAVE_VEHICLES, "") ;
	char buf[256] ;

	WEAPON_LIST::iterator it ;
	for ( it = g_weaponList.begin() ; it != g_weaponList.end() ; ++it )
	{
		WEAPON& weap = *it->second ;
		const WEAPON_SPEC& spec = it->first ;
		if ( (UINT32)spec.wCountry > (UINT32)MAX_COUNTRIES )
		{
			LogAbort("illegal weapon spec (country): %u.%u.%u.%u", spec.wCountry, spec.wCategory, spec.wClass, spec.wType) ;
		}

		snprintf(buf, sizeof(buf), "(%u,%u,%u,%u,\"%s\",%u,\"%s\",NOW())"
					, spec.wCountry, spec.wCategory, spec.wClass, spec.wType
					, weap.name
					, weap.value
					, weap.name
					) ;
		qry.addEntry(buf) ;
	}

	qry.commit(0) ;
}

void
weapLoadIndexes(DB::handle_t* conn)
{
	DBQuery query(conn
		, "SELECT v.countryID, v.categoryID, v.classID, v.typeID"
				", v.fullName, v.vehtype_oid, v.campaign_ready"
				", v.current_rank, v.next_cycle_rank"
			" FROM wwiionline.wwii_vehtype v"
			" WHERE v.in_service=1"
	) ;
	if ( !query.HasRows() )
	{
		dbugLog(_FLA, "vehtype table appears to be empty") ;
		return ;
	}

	UINT32 campaignReady = 1, access = 0, nextAccess = 0 ;
	UINT8 wCountry, wCategory, wClass, wType ;
	char fullName[128] ;
	UINT32 oid ;

	while ( query.FetchRow() )
	{
		const DBRow& row = query.Row() ;

		wCountry = row.NextUInt() ;
		wCategory = row.NextUInt() ;
		wClass = row.NextUInt() ;
		wType = row.NextUInt() ;

		row.NextString(fullName, sizeof(fullName)) ;
		oid = row.NextUInt() ;

		campaignReady = row.NextUInt() ;
		access = row.NextUInt() ;
		nextAccess = row.NextUInt() ;

		WEAPON_SPEC spec(wCountry, wCategory, wClass, wType) ;
		WEAPON* const weap = weapLookup(spec) ;

#ifdef DEV_BUILD
		dbugLog(__FILE__, __LINE__, (weap ? Debug::ERR_DBUG : Debug::ERR_WARN)
						, "v:%u %s [%u.%u.%u.%u] campaignReady:%u access:%u next:%u weap:%p(%s)"
						, oid, fullName
						, (UINT32)spec.wCountry, (UINT32)spec.wCategory, (UINT32)spec.wClass, (UINT32)spec.wType
						, campaignReady
						, access, nextAccess
						, weap
						, (weap ? weap->fullName : "UNRECOGNIZED")
						) ;
#endif

		if ( weap == NULL )
			continue ;

		weap->m_localIndex = oid ;
		strncpy(weap->fullName, fullName, sizeof(weap->fullName)) ;

		weapAddWeaponIndex(weap) ;

		if ( campaignReady )
		{
			weap->m_access = access ;
			weap->m_nextAccess = nextAccess ;
		}
		else
		{
			weap->m_modified = true ;
		}
	}
}
