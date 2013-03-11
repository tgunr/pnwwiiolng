#include <syscfg.h> // Path is project specific.
// Date: Wed Jan 17 17:23:20 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Marty "Vandal" Poulin

#include "versionNo.h"

#include "dbse/dbseConnection.h"
#include "dbse/dbseWrapper.h"
#include "include/mydbQuery.h"
#include "wwii/wwiiDBConfig.h"
#include "chat/chatDotCommand.h"
#include "wwii/wwiiHCUnits.h"
#include "wwii/wwiiRank.h"
#include "chat/chatCellTypes.h"
#include "chat/chatData.h"
#include "wwii/wwiiThreads.h"
#include "wwii/wwiiThreadText.h"
#include "chat/chatMotds.h"
#include "chat/chatHostMotds.h"

DBCONNECTION dbConnection ;
DBHANDLE dbh;
extern DBHANDLE& chatdbGetDBH() { return dbh ; }

PROPERTY_INDEX _properties ;

static const char QRY_SELECT_MOTD_BY_TYPE[] =
	"SELECT unix_timestamp(m.added), IF(p.isplaynet,UPPER(p.callsign),p.callsign), m.motd"
		" FROM wwii_motd m LEFT JOIN wwii_player p ON (m.added_by = p.playerid)"
		" WHERE m.type = %u"
		" ORDER BY 1 DESC"
		" LIMIT 1"
		;

// Insert a report of a speed hacker
static const char QRY_INSERT_SPEED_HACK[] =
	"INSERT INTO wwii_hack_reports"
		" (customerID, cycleno, hosttime, host_elapsed_ms, client_elapsed_ms"
		" , delta_avg, faults, packets_counted, drift, kicked)"
		" VALUES"
		" (%u, %u, %u, %llu, %llu, %lld, %u, %u, %lld, '%c')"
;

static const char QRY_INSERT_CHAT_EVENT[] =
	"INSERT INTO chat_events"
		 " (event_stamp, category, callsign, event, parameter, data)"
		" VALUES"
		 " (now(), \"%s\", \"%s\", \"%s\", \"%s\", \"%s\")"
;

static std::string permissionNames[64] ;

void
_loadMotd(MOTD_REPLY& motd)
{
	try
	{
		// Works, but we may want to retrieve the latest motd id and
		// then try to find the most apropriate version for it
		// e.g. IF (m.languageID = %d,0,1) AS preference
		//       ORDER BY motdid, preference, languageid
		DBQuery q(dbh, QRY_SELECT_MOTD_BY_TYPE, motd.type);

		if ( q.HasRows() == false )
			return ;

		if ( q.FetchRow() == false )
			throw std::logic_error("fetchrow failed") ;

		DBRow& row = q.Row() ;

		motd.added = row.NextUInt() ;
		const char* poster = row.NextString() ;
		motd.poster = chatGetHandle(poster) ;
		motd.message = row.NextString() ;

		return ;
	}
	catch (std::exception& e) { dbugLog(_FLA, "%s: %s", __FUNCTION__, e.what()) ; }

	return ;
}

void
chatLoadMotds()
{
	for ( UINT32 i = 0 ; i < MOTD::_MAX_TYPES ; ++i )
	{
		MOTD_REPLY& motd = chatGetMotd(i) ;
		motd.type = i ;
		_loadMotd(motd) ;
	}
}

//////////////////////////////////////////////////////////////////////
// Helper function for finding a particular property.

Property*
chatFindProperty(const FULL_OBJ_ID& id)
{
	PROPERTY_INDEX::iterator it ;

	it = _properties.find(id.Key()) ;
	if ( it == _properties.end() )
		return NULL ;

	return it->second ;
}

//////////////////////////////////////////////////////////////////////
// Constructor for Facility properties; slightly more complex than
// the others since it has to connect itself to its parent cp

PropFacility::PropFacility(const FULL_OBJ_ID& id, WWII_COUNTRY owner, const char* name, const FULL_OBJ_ID& cpID, UINT32 oid)
	: Property(id, owner, name, oid)
	, m_cp(NULL)
{
	Property* cp = chatFindProperty(cpID) ;
	if ( cp == NULL )
	{
		dbugLog(_FLA, "%s has invalid CP data %d.%d.%u", name, cpID.cell.ox, cpID.cell.oy, cpID.id) ;
		return ;
	}

	if ( !cp->IsCP() )
	{
		dbugLog(_FLA, "%s has CP data that references a non-CP object", name) ;
		return ;
	}

	// Add me to my parent CP's list
	m_cp = reinterpret_cast<PropCP*>(cp) ;
}

//////////////////////////////////////////////////////////////////////
// Load CP and Facility relational data (relational only)

#include "wwii/wwiiConfig.h"
static bool
chatdbLoadStratData()
{
	UINT32 loadedCPs = 0 ;
	UINT32 loadedFacs = 0 ;
	UINT32 owner ;

	assert(sizeof(FULL_OBJ_ID) == sizeof(UINT64)) ;

	FULL_OBJ_ID fid ;
	FULL_OBJ_ID cpid ;
	UINT32 oid ;
	char name[128] ;

	try
	{
		UINT32 controller ;

		DBQuery q(dbh, "SELECT cp_ox, cp_oy, cp_class, cp_type, cp_id"
									", country, concountry"
									", cp_oid, name FROM strat_cp") ;
		while ( q.FetchRow() )
		{
			auto& row = q.Row() ;
			fid.cell.ox = row.NextSInt() ;
			fid.cell.oy = row.NextSInt() ;
			fid.objClass = row.NextUInt() ;
			fid.objType = row.NextUInt() ;
			fid.id = row.NextUInt() ;
			owner = row.NextUInt() ;
			controller = row.NextUInt() ;
			oid = row.NextUInt() ;
			row.NextString(name, sizeof(name)) ;

			new PropCP(fid, (WWII_COUNTRY)owner, name, (WWII_COUNTRY)controller, oid) ;
			loadedCPs++ ;
		}
	}
	catch (std::exception& e)
	{
		dbugLog(_FLW, "chatdbLoadStratData failed: %s", e.what()) ;
		return false ;
	}

	try
	{
		memset(&cpid, 0, sizeof(cpid)) ;

		DBQuery q(dbh, "SELECT facility_ox, facility_oy, facility_class, facility_type, facility_id"
							", cp_ox, cp_oy, cp_id"
							", country, facility_oid, name FROM strat_facility") ;

		while ( q.FetchRow() )
		{
			auto& row = q.Row() ;
			fid.cell.ox = row.NextSInt() ;
			fid.cell.oy = row.NextSInt() ;
			fid.objClass = row.NextUInt() ;
			fid.objType = row.NextUInt() ;
			fid.id = row.NextUInt() ;
			cpid.cell.ox = row.NextSInt() ;
			cpid.cell.oy = row.NextSInt() ;
			cpid.id = row.NextUInt() ;
			owner = row.NextUInt() ;
			oid = row.NextUInt() ;
			row.NextString(name, sizeof(name)) ;

			new PropFacility(fid, (WWII_COUNTRY)owner, name, cpid, oid) ;

			loadedFacs++ ;
		}
	}
	catch (std::exception& e)
	{
		dbugLog(_FLW, "chatdbLoadStratData failed: %s", e.what()) ;
		return false ;
	}

	if ( loadedCPs == 0 || loadedFacs == 0 )
	{
		dbugLog(_FLW, "CPs:%d, Facs: %d", loadedCPs, loadedFacs) ;
		return false ;
	}

	dbugLog(_FLI, "CPs:%d, Facs: %d", loadedCPs, loadedFacs) ;
	return true ;
}

//////////////////////////////////////////////////////////////////////

bool
chatGetWWIIConfig(void)
{
	dbGetWWIIConfig(dbh) ;
	return true ;
}

//////////////////////////////////////////////////////////////////////

LOGICAL
chatdbRecordEvent(const SCHAR* category
					, const SCHAR* callsign
					, const SCHAR* event
					, const SCHAR* parameter
					, const SCHAR* data
					)
{
	static char escCategory[64] ;
	static char escCallsign[64] ;
	static char escEvent[128] ;
	static char escParam[1024] ;
	static char escData[1024] ;

	DBThread::Do(DBThread::Default
					, QRY_INSERT_CHAT_EVENT
						, escapeString(category, escCategory, sizeof(escCategory))
						, escapeString(callsign, escCallsign, sizeof(escCallsign))
						, escapeString(event, escEvent, sizeof(escEvent))
						, escapeString(parameter, escParam, sizeof(escParam))
						, escapeString(data, escData, sizeof(escData))
				) ;
	return true ;
}

//////////////////////////////////////////////////////////////////////
// Read the names of the permissions from the database

static inline void
_readPermissionNames()
{
	DBQuery qry(dbh, "SELECT `permissionid` - 1, `description` FROM `auth`.`wwii_permissions`") ;
	if ( qry.HasRows() == false )
		throw std::runtime_error("No rows returned from auth.wwii_permissions") ;	///Terminal
	while ( qry.FetchRow() )
	{
		DBRow& row = qry.Row() ;
		UINT32 mask = row.NextUInt() ;
		assert( mask < 64 ) ;
		permissionNames[mask] = row.NextString() ;
	}
	char name[64] ;
	for ( UINT32 i = 0 ; i < 64 ; ++i )
	{
		if ( permissionNames[i].empty() )
		{
			sprintf(name, "UNK_PERM_%u", i+1) ;
			permissionNames[i] = name ;
		}
	}
}

//////////////////////////////////////////////////////////////////////
// Initialize the database and load in bootstrap db data.

LOGICAL
chatInitDB()
{
	dbugLog(_FLD, "Initializing database") ;

	try
	{
		if ( dbh.connect(dbConnection) == false )
			throw std::runtime_error("database connection failed") ;

		wwiiReadHCUnits() ;
		wwiiReadRanks(dbh) ;

		_readPermissionNames() ;
		return chatdbLoadStratData() ;
	}
	catch ( std::exception& e )
	{
		LogAbort("Unable to initialize Chat Database: %s", e.what()) ;
	}
}

LOGICAL
chatCloseDB(void)
{
	dbh.disconnect() ;
	return TRUE;
}

#ifdef NEVER
static const char QRY_SELECT_TASK_LIST[] =
	"SELECT"
		" t.id"
		",t.summary"
#endif
void
chatdbListTasks()
{
	try
	{
//		DBQuery q(dbh,
	}
	catch (std::exception& e) { dbugLog(_FLW, "%s: %s", __FUNCTION__, e.what()) ; }
}

//////////////////////////////////////////////////////////////////////
// Return the list of permissions for a given mask value

UINT32
chatGetPermissionNames(const PermissionMask& mask, char* into, UINT32 from)
{
	bool matched = (from != 0) ;
	char* cutoff = into + MAX_MESSAGE_LENGTH - 4 ;
	PermissionMask bit = 1ULL << from ;
	for ( UINT32 i = from ; i < 64 ; ++i, bit <<= 1 )
	{
		if ( (mask & bit) == 0ULL )
			continue ;
		if ( into + permissionNames[i].size() > cutoff )
		{
			*into = 0 ;
			return i ;
		}
		if ( matched == true )
		{
			*(into++) = ',' ;
			*(into++) = ' ' ;
		}
		else
			matched = true ;
		for ( const char* p = permissionNames[i].c_str() ; *p != 0 ; ++p, ++into )
			*into = tolower(*p) ;
	}
	*into = 0 ;
	return 0 ;
}

