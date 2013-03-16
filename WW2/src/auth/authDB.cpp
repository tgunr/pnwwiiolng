#include <syscfg.h> // Path is project specific.
// Created: Thu Jun 22 14:28:10 2000
//
// The database layer for the Authentication server.
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

#include "dbug/dbug.h"
#include "teul/teulBasic.h"
#include "auth/authTypes.h"
#include "dbse/dbseConnection.h"
#include "dbse/dbseWrapper.h"
#include "auth/authDB.h"
#include "mem/mem.h"
#include <mysql.h>
#include "include/infoMessageTypes.h"
#include "include/infoHostMessages.h"
#include "include/mydbQuery.h"
#include "wwii/wwiiThreads.h"
#include "wwii/wwiiThreadText.h"
#include "auth/authHost.h"
#include "auth/authClientData.h"
#include "auth/authGame.h"
#include "auth/authPlaygateTypes.h"
#include "auth/authDBConnection.h"
#include "auth/authDatabaseSchemas.h"

#define MAX_APPIDS	256

struct APP_TABLE_ENTRY
{
	UINT32	appID;				//The application ID that one uses to
	UINT32	gameID;				//The application ID that one uses to
	SCHAR	appleID[APPLE_ID_LEN+1];
	SCHAR	description[MAX_DESCRIPTION+1];
} ;

typedef std::vector<APP_TABLE_ENTRY> APP_TABLE ;

struct apps_holder
{
	SINT32	numEntries;
	SINT32	appID;
	struct vers_manifest* manifest;
	SCHAR	appleID[APPLE_ID_MAX+1];
};


struct conf_struct
{
	SINT32	numApps;
	struct apps_holder apps[MAX_APPIDS];
};

// Queries

// Insert an otp value for a player
static const char QRY_INSERT_OTP[] =
	"INSERT INTO auth_otp"
		" (username,otp,homedir,timestamp)"
	" VALUES"
		" (\'%s\',%d,\'%s\',NOW())"
;

// Select list of app_ids from app_manifest
static const char QRY_SELECT_APPIDS_FROM_MANIFEST[] =
	"SELECT DISTINCT(appID) FROM app_manifest"
;

// Get information on each app from the auth_apps table
static const char QRY_SELECT_APP_INFO[] =
	"SELECT appid, gameid, appleID, description FROM auth_apps"
;

// Get the platform/version number for specific app/host
static const char QRY_SELECT_HOST_VERSIONS_BY_APPHOST[] =
	"SELECT platform, minClientVersion, maxClientVersion"
		" FROM app_host_versions"
		" WHERE appID = %u"
			" AND hostVersion = '%s'"
		" ORDER BY platform"
;

#define AUTH_VAR_LAST_OTP "last_otp"
static const char QRY_GET_LAST_OTP[] =
	"SELECT value FROM auth_variables WHERE name = '" AUTH_VAR_LAST_OTP "'" ;

#if defined( HAPPYOO )
# define AUTH_PASSWORD "m4nd0l1n"			// Happyoo's auth password
#else
# define AUTH_PASSWORD "scalar449"			// Playnet's auth password
#endif


static struct conf_struct config;

static struct apps_holder* apps = config.apps;

static APP_TABLE appTable ;

DBHANDLE dbh;

typedef std::map<UINT32, std::string> ARENA_ALIASES ;
static ARENA_ALIASES g_arenaAliases ;

DBCONNECTION dbConnection ;

//////////////////////////////////////////////////////////////////////
//connect function
void authInitDB(void)
{
	authGetDBConnectionInfo(&dbConnection) ;

	if ( dbh.connect(dbConnection) == false )
		LogAbort("Unable to initialize auth database connection - check grants for %s on %s", dbConnection.username, dbConnection.host) ;

	DB::Schema updater(dbh, "auth") ;
	updater.Update(revisions) ;

	DBSimpleQuery(dbh, "UPDATE `auth`.`auth_arenas` SET lock_state = %d, players = 0", AL_OFFLINE) ;

	dbugLog(_FLN, "auth database ready") ;
}

//////////////////////////////////////////////////////////////////////
//this function resets the who's online for this server
void
authResetConnectInfo(UINT32 arenaID)
{
	/* Do nothing for now */
}

//////////////////////////////////////////////////////////////////////
// Inserts otp records int ot the authOTP table to be used by
// our ftp servers for authenication.
void
authDBInsertOTP(const APP_LAUNCH& launch , const APP_LAUNCH_REQ& request, const char* dir)
{
	DBSimpleQuery(dbh, QRY_INSERT_OTP, request.username, launch.otp, dir) ;
}

//////////////////////////////////////////////////////////////////////
//This function takes the connectinfo struct and logs it in
//	the connection activity table, then updates the who's online
//	table
//

static std::vector<MANAGEMENT_MSG> logins ;

void
authdbSaveLogins()
{
	if ( logins.empty() )
		return ;

	char* statement = threadsAllocText() ;
										/*  ,   (   %u   ,   %u   ,   %u   )   \0	*/
	static const size_t EST_STATEMENT_LEN = 1 + 1 + 12 + 1 + 12 + 1 + 12 + 1 + 1 ;
	char* cutoff = statement + TEXT_POOL_SIZE - EST_STATEMENT_LEN ;

	static const char* const prefix = "INSERT INTO `auth`.`auth_connections`(`customerID`, `arenaID`, `ip`)VALUES" ;
	char* p = statement + sprintf(statement, "%s", prefix) ;
	char* comma = p ;
	auto it = logins.begin() ;
	while ( it != logins.end() && p < cutoff )
	{
		p += sprintf(p, ",(%u,%u,%u)", (UINT32)it->m_customerID, (UINT32)it->m_arenaID, (UINT32)it->m_ip) ;
		++it ;
	}

	// Replace that leading comma with a space.
	*comma = ' ' ;

	// Dispatch the query.
	DBThread::Do(DBThread::Auth, statement) ;

	// Remove the logins we've accumulated.
	logins.erase(logins.begin(), it) ;
}

void
MANAGEMENT_MSG::process() const
{
	// When a player is logging in (CLIENT_ENTER), record the connection
	if ( m_action == CLIENT_ENTER )
		logins.push_back(*this) ;
}

//////////////////////////////////////////////////////////////////////
//this function validates a user against the database
UINT32
authValidateUser(SINT32 teulIndex, const APP_LAUNCH_REQ& request, Auth::Dispatch& gamecookie, const Auth::ClientData& cdata)
{
	std::string username = request.username ;
	if ( username.length() < 2 || username.length() > 16 )
	{
		dbugLog(_FLW, "t:%d c:%s ip:%s arena:%u plat:%u username is invalid"
						, teulIndex, username.c_str(), teulGetAddress(cdata.address), request.arenaID, cdata.platform) ;
		authdbLogBadPlayername(cdata.address, username, "illegal username") ;
		return WEB_ERR_USER ;
	}

	std::string password = (const char*)request.password ;
	if ( password.length() > 80 )
	{
		dbugLog(_FLW, "t:%d c:%s ip:%s arena:%u plat:%u password '%s' is invalid"
						, teulIndex, username.c_str(), teulGetAddress(cdata.address), request.arenaID, cdata.platform
						, password.c_str()) ;
		authdbLogBadPlayername(cdata.address, username, "illegal password") ;
		return WEB_ERR_INTERNAL ;
	}

	char buffer[512] ;
	wwiiEscapeString(username) ;
	wwiiEscapeString(password) ;

	UINT32 len = snprintf(buffer, sizeof(buffer)-1
					, "CALL `auth`.`p_authenticate_user_for_arena`('%s','%s',%u)"
						, username.c_str()
						, password.c_str()
						, request.arenaID
						) ;
	if ( len >= sizeof(buffer) - 1 )
	{
		dbugLog(_FLW, "t:%d c:%s ip:%s arena:%u plat:%u login attempt produces overlong query (%u bytes)"
						, teulIndex, request.username, teulGetAddress(cdata.address), request.arenaID, cdata.platform
						, len) ;
		authdbLogBadPlayername(cdata.address, request.username, "illegal query") ;
		return WEB_ERR_INTERNAL ;
	}

	UINT32 passwordMatch = 0, isActive = 0, haveInfo = 0 ;
	bool foundUser = false ;

	try
	{
		DBQuery q(dbh, buffer) ;
		if ( q.HasRows() && q.FetchRow() == true )
		{
			DBRow& row = q.Row() ;

			passwordMatch = row.NextUInt() ;
			isActive = row.NextUInt() ;
			gamecookie.m_customerID = row.NextUInt() ;
			row.NextString(gamecookie.m_callsign, sizeof(gamecookie.m_callsign)) ;
			haveInfo = row.NextUInt() ;

			if ( gamecookie.m_callsign[0] == 0 )
				throw std::invalid_argument("null game name") ;

			foundUser = true ;
		}
	}
	catch (std::exception& e)
	{
		authdbLogBadPlayername(cdata.address, request.username, e.what()) ;
		LogAbort("error looking up %s with %s: %s", request.username, request.password, e.what()) ;
		return WEB_ERR_INTERNAL ;
	}

	if ( foundUser == false )
	{
		dbugLog(_FLN, "t:%d c:%s ip:%s arena:%u plat:%u no such user"
						, teulIndex, request.username, teulGetAddress(cdata.address), request.arenaID, cdata.platform) ;
		authdbLogBadPlayername(cdata.address, request.username, "no such user") ;
		return WEB_ERR_USER ;
	}

	if ( passwordMatch == 0 )
	{
		dbugLog(_FLN, "t:%d c:%s ip:%s arena:%u plat:%u incorrect password"
						, teulIndex, request.username, teulGetAddress(cdata.address), request.arenaID, cdata.platform) ;
		authdbLogBadPlayername(cdata.address, request.username, "incorrect password") ;
		return WEB_ERR_PASSWORD ;
	}

	if ( isActive == 0 )
	{
		dbugLog(_FLN, "t:%d c:%s ip:%s arena:%u plat:%u account not active"
						, teulIndex, request.username, teulGetAddress(cdata.address)
						, request.arenaID, cdata.platform
						) ;
		authdbLogBadPlayername(cdata.address, request.username, "account not active") ;
		return WEB_ERR_SUB ;
	}

	if ( cdata.oVersion > 1 && haveInfo == 0 )
		infoSendCompInfoRequest(teulIndex, "Updating Terms of Service") ;

	return WEB_ERR_NONE ;
}

UINT32
Auth::LoginRequest::validate(SINT32 teulIndex, Auth::ClientData& cdata)
{
	std::string username = m_account ;
	if ( username.length() < 2 || username.length() > 16 )
	{
		dbugLog(_FLW, "t:%d c:%s ip:%s plat:%u username is invalid"
						, teulIndex, username.c_str()
						, teulGetAddress(cdata.address), m_platform) ;
		authdbLogBadPlayername(cdata.address, username, "illegal username") ;
		return WEB_ERR_USER ;
	}

	std::string password = m_passwordMD5 ;
	if ( password.length() > 80 )
	{
		dbugLog(_FLW, "t:%d c:%s ip:%s plat:%u password '%s' is invalid"
						, teulIndex, username.c_str(), teulGetAddress(cdata.address), m_platform
						, password.c_str()) ;
		authdbLogBadPlayername(cdata.address, username, "illegal password") ;
		return WEB_ERR_INTERNAL ;
	}

	char buffer[512] ;
	wwiiEscapeString(username) ;
	wwiiEscapeString(password) ;

	UINT32 len = snprintf(buffer, sizeof(buffer)-1
					, "CALL `auth`.`p_get_user_subscriptions`('%s','%s')"
						, username.c_str()
						, password.c_str()
						) ;
	if ( len >= sizeof(buffer) - 1 )
	{
		dbugLog(_FLW, "t:%d c:%s ip:%s plat:%u login attempt produces overlong query (%u bytes)"
						, teulIndex, username.c_str(), teulGetAddress(cdata.address), m_platform
						, len) ;
		authdbLogBadPlayername(cdata.address, username, "illegal query") ;
		return WEB_ERR_INTERNAL ;
	}

	UINT32 passwordMatch = 0, isActive = 0, haveInfo = 0 ;
	bool foundUser = false ;

	try
	{
		DBQuery q(dbh, buffer) ;
		if ( q.HasRows() && q.FetchRow() == true )
		{
			DBRow& row = q.Row() ;

			passwordMatch = row.NextUInt() ;
			isActive = row.NextUInt() ;
			cdata.customerID = row.NextUInt() ;
			haveInfo = row.NextUInt() ;
			cdata.isPlaynet = (row.NextUInt() == 1) ;
			const char* subs = row.NextString() ;
			while ( subs && *subs )
			{
				char* p = const_cast<char*>(strchr(subs, ',')) ;
				if ( p )
				{
					*(p++) = 0 ;
					if ( *p == ',' )
						continue ;
				}
				cdata.m_subscribedArenaIDs.push_back((UCHAR)atoi(subs)) ;
				subs = p ;
			}
			cdata.isPremium = (row.NextString()[0] == 'Y') ;

			foundUser = true ;
		}
	}
	catch (std::exception& e)
	{
		authdbLogBadPlayername(cdata.address, username, e.what()) ;
		LogAbort("error looking up %s with %s: %s", username.c_str(), password.c_str(), e.what()) ;
		return WEB_ERR_INTERNAL ;
	}

	if ( foundUser == false )
	{
		dbugLog(_FLN, "t:%d c:%s ip:%s plat:%u no such user"
						, teulIndex, username.c_str(), teulGetAddress(cdata.address), m_platform) ;
		authdbLogBadPlayername(cdata.address, username, "no such user") ;
		return WEB_ERR_USER ;
	}

	if ( passwordMatch == 0 )
	{
		dbugLog(_FLN, "t:%d c:%s ip:%s plat:%u incorrect password"
						, teulIndex, username.c_str(), teulGetAddress(cdata.address), m_platform) ;
		authdbLogBadPlayername(cdata.address, username, "incorrect password") ;
		return WEB_ERR_PASSWORD ;
	}

	if ( isActive == 0 )
	{
		dbugLog(_FLN, "t:%d c:%s ip:%s plat:%u account not active"
						, teulIndex, username.c_str(), teulGetAddress(cdata.address), m_platform
						) ;
		authdbLogBadPlayername(cdata.address, username, "account not active") ;
		return WEB_ERR_SUB ;
	}

	if ( cdata.m_subscribedArenaIDs.empty() )
	{
		dbugLog(_FLN, "t:%d c:%s ip:%s plat:%u account has no subscriptions"
						, teulIndex, username.c_str(), teulGetAddress(cdata.address), m_platform
						) ;
		authdbLogBadPlayername(cdata.address, username, "no active subs") ;
		return WEB_ERR_SUB ;
	}

	if ( cdata.oVersion > 1 && haveInfo == 0 )
		infoSendCompInfoRequest(teulIndex, "Updating Terms of Service") ;

	return WEB_ERR_NONE ;
}

//////////////////////////////////////////////////////////////////////
static bool
_getAppTable()
{
	appTable.clear() ;

	try
	{
		DBQuery q(dbh, QRY_SELECT_APP_INFO) ;
		if ( q.HasRows() == false )
			throw std::invalid_argument("auth_apps is empty") ;

		appTable.resize(q.Rows()) ;

		UINT32 rown = 0 ;
		while ( q.FetchRow() )
		{
			DBRow& row = q.Row() ;
			APP_TABLE_ENTRY& entry = appTable[rown++] ;

			entry.appID = row.NextUInt() ;
			entry.gameID = row.NextUInt() ;
			row.NextString(entry.appleID, sizeof(entry.appleID)) ;
			entry.description[0] = 0 ;
			row.NextString(entry.description, sizeof(entry.description)) ;
			if ( entry.description[0] == 0 )
				sprintf(entry.description, "Unkown Game#%u/%u", entry.appID, entry.gameID) ;
		}
	}
	catch (std::exception& e)
	{
		LogAbort("%s: %s", __FUNCTION__, e.what()) ;
	}

	dbugLog(_FLN, "appTable initialized with %" PRSizeT " applications", appTable.size());

	return true;
}

//////////////////////////////////////////////////////////////////////
static APP_TABLE_ENTRY*
_findAppTable(UINT32 appID)
{
	for ( UINT32 i = 0 ; i < appTable.size() ; ++i )
	{
		if ( appID == appTable[i].appID )
			return &appTable[i] ;
	}

	return NULL;
}

//////////////////////////////////////////////////////////////////////
static bool
_initAppInfoTable(GAME& gameEntry)
{
	SCHAR statement[512];
	SINT32 i, platform;

	for ( i = 0;i < OS_MAX_PLAT; i++)
	{
		gameEntry.appInfo[i].max = gameEntry.curHostVersion;
		gameEntry.appInfo[i].min = gameEntry.curHostVersion;
	}

	sprintf(statement, QRY_SELECT_HOST_VERSIONS_BY_APPHOST, gameEntry.appID, teulGetAddress(gameEntry.curHostVersion));
	try
	{
		DBQuery q(dbh, statement) ;
		if ( !q.HasRows() )
		{
			//We don't have that host version
			dbugLog(_FLA,"couldnt find app entry for appID %d hostVersion %s"
					, gameEntry.appID, teulGetAddress(gameEntry.curHostVersion) );
			return true ;
		}

		while ( q.FetchRow() )
		{
			auto& row = q.Row() ;
			platform = (SINT32)atol(row[0]);

			if ( platform < 0 || platform > OS_MAX_PLAT)
				throw std::logic_error("illegal platform value in app_host_versions") ;

			teulGetHost(row[1], &gameEntry.appInfo[platform].min);
			teulGetHost(row[2], &gameEntry.appInfo[platform].max);
		}

		return true ;
	}
	catch ( std::exception& e )
	{
		dbugLog(_FLA, "error selecting host versions [%s]", statement) ;
		LogAbort("%s", e.what()) ;
	}

	return false ;
}

//////////////////////////////////////////////////////////////////////
static SINT32
_findIsFile(SCHAR manisfile)
{
	switch(manisfile)
	{
		case 'f':
			return ISFILE_FILE;
		break;
		case 'u':
			return ISFILE_URL;
		break;
		case 'n':
			return ISFILE_NUN;
		break;
		default:
		break;
	}
	return ISFILE_URL;
}

//////////////////////////////////////////////////////////////////////
static bool
_getAppManifest()
{
	SCHAR statement[512];
	SINT32 appNum;
	static struct vers_manifest* vers_mans;

	for (size_t i = 0; i < MAX_APPIDS; i++)
	{
		if ( config.apps[i].manifest )
			memFree(config.apps[i].manifest);
	}

	memset(config.apps, 0, sizeof(config.apps));

	try
	{
		DBQuery q(dbh, QRY_SELECT_APPIDS_FROM_MANIFEST) ;

		config.numApps = q.Rows() ;
		if ( config.numApps == 0 )
		{
			//We don't have that host version
			LogAbort("couldnt find any applications!!");
		}

		for ( size_t i = 0 ; q.FetchRow() ; ++i )
		{
			config.apps[i].appID = (SINT32)atol(q.Row()[0]);
		}
	}
	catch ( std::exception& e )
	{
		dbugLog(_FLA, "error fetching manifests: %s", e.what()) ;
		return false ;
	}

	try
	{
		for (appNum = 0; appNum < config.numApps; appNum++)
		{
			sprintf(statement,
						"SELECT old,new,platform,actionType,webURL,ftpPort,ftpProtocol,ftpURL,ftpDir,ftpFile"
							",INET_ATON(old) AS sorter"
							" FROM auth.app_manifest"
							" WHERE appID = %u"
							" ORDER BY platform, sorter"
							, config.apps[appNum].appID) ;
			DBQuery q(dbh, statement) ;

			//Build the reply
			if ( !q.HasRows() )
			{
				//We don't have that host version
				dbugLog(_FLA,"couldnt find any manifests for app %d", config.apps[appNum].appID );
				continue ;
			}

			const size_t numRows = config.apps[appNum].numEntries = q.Rows() ;
			config.apps[appNum].manifest = vers_mans = (vers_manifest*)memCalloc(numRows, sizeof(struct vers_manifest));

			for ( size_t i = 0 ; q.FetchRow() ; ++i )
			{
				vers_mans[i].appid = config.apps[appNum].appID;
				vers_mans[i].port = DEF_PORT;
				vers_mans[i].protocol = DEF_PROTOCOL;

				auto& row = q.Row() ;

				teulGetHost(row[0], &vers_mans[i].oldNo);
				teulGetHost(row[1], &vers_mans[i].newNo);
				vers_mans[i].platform = (SINT32)atol(row[2]);
				vers_mans[i].isfile = _findIsFile(row[3][0]);
				strncpy(vers_mans[i].location, row[4], URL_STRING_MAX);

				if (vers_mans[i].isfile == ISFILE_FILE)
				{
					vers_mans[i].port = (SINT32)atol(row[5]);
					if (row[6][0] == 's')
						vers_mans[i].protocol = 0;
					else if (row[6][0] == 'a')
						vers_mans[i].protocol = DEF_PROTOCOL;
					else
					{
						dbugLog(_FLW, "invalid protocol %c specified for appID %d seeting to default"
									, row[6][0], appNum);
						vers_mans[i].protocol = DEF_PROTOCOL;
					}

					strncpy(vers_mans[i].ftpURL, row[7], URL_STRING_MAX);
					strncpy(vers_mans[i].ftpDir, row[8], URL_STRING_MAX);
					strncpy(vers_mans[i].ftpFile, row[9], URL_STRING_MAX);
				}
			}
		}
	}
	catch ( std::exception& e )
	{
		dbugLog(_FLA, "err reading manifests: %s", e.what()) ;
	}

	dbugLog(_FLN, "appTable Initialized %d apps", config.numApps);

	return true ;
}

//////////////////////////////////////////////////////////////////////
void
authInitApps(void)
{
	_getAppTable();
	_getAppManifest();
}

//////////////////////////////////////////////////////////////////////
static bool
_getPlayerTable(GAME& gameEntry)
{
	try
	{
		DBQuery q(dbh, "SELECT playertable FROM `auth`.`auth_games` WHERE gameID = %u", gameEntry.gameID) ;
		if ( q.HasRows() == false || q.FetchRow() == false )
			throw std::invalid_argument("couldn't find game entry") ;

		strncpy(gameEntry.playerTable, q.Row()[0], MAX_DESCRIPTION) ;

		dbugLog(_FLI, "arena:%d game:%d setting playerTable to %s", gameEntry.arenaID, gameEntry.gameID, gameEntry.playerTable) ;
		return true ;
	}
	catch (std::exception& e) { dbugLog(_FLA, "arena:%d game:%d %s", gameEntry.arenaID, gameEntry.gameID, e.what()) ; }

	return false ;
}

//////////////////////////////////////////////////////////////////////
LOGICAL
authInitGameEntry(GAME& gameEntry)
{
	APP_TABLE_ENTRY* appEntry;

	 appEntry = _findAppTable(gameEntry.appID);
	 if (appEntry == NULL)
	 	return false;

	memcpy(gameEntry.appleID, appEntry->appleID, APPLE_ID_LEN);

	try
	{
		DBQuery q(dbh
				,	"SELECT gameid, groupid, clusterid, method, description"
			 		" FROM auth_arenas WHERE arenaID = %u"
			 		, gameEntry.arenaID
			);
		if ( q.HasRows() == false || q.FetchRow() == false )
		{
			dbugLog(_FLA, "Missing auth_arenas entry for arenaID=%u", gameEntry.arenaID) ;
			return false ;
		}

		DBRow& row = q.Row() ;
		gameEntry.gameID = row.NextUInt() ;
		gameEntry.groupID = row.NextUInt() ;
		gameEntry.clusterID = row.NextUInt() ;
		gameEntry.usePlayerID = (row.NextString()[0] == 'p') ;
		gameEntry.description[0] = 0 ;
		row.NextString(gameEntry.description, sizeof(gameEntry.description)) ;
		if ( gameEntry.description[0] == 0 )
			sprintf(gameEntry.description, "Unk.Game#%u/%u", gameEntry.gameID, gameEntry.clusterID) ;
	}
	catch ( std::exception& e )
	{
		dbugLog(_FLA, "%s", e.what()) ;
		return false ;
	}

	dbugLog(_FLN,"setting game entry gameID %d groupID %d clusterID %d usePlayerID %d description %s"
				, gameEntry.gameID,gameEntry.groupID,gameEntry.clusterID
				, gameEntry.usePlayerID,  gameEntry.description);

	if ( gameEntry.usePlayerID && !_getPlayerTable(gameEntry) )
		return false ;

	return _initAppInfoTable(gameEntry) ;
}

//////////////////////////////////////////////////////////////////////
LOGICAL
authdbStoreComputerInfo(const Auth::ClientData& cdata, const PLAY_COMPUTER_INFO& pci)
{
	SCHAR	statement[8000+1] ;
	SCHAR	cardName[MAX_OGL_CARD_NAME*2 + 1] ;
	SCHAR	vendor[MAX_OGL_VENDOR_NAME*2 + 1] ;
	SCHAR	descs[MAX_COMPRESSED_TEXTURE*5 + 1] ; // %4x + " "
	std::string cpuManufacturer ;
	std::string cpuBrandName ;
	SCHAR	soundCard[MAX_MIX_NAME*2 + 1] ;
	SCHAR	exts[1024+1] ;
	SCHAR*	p ;
	UINT32	i ;
	UINT32	numCompTextures ;
	const PLAY_OGL& ogl = pci.video.ogl ;
	const CPUFeatures& cpu = pci.cpu ;
	const PLAY_MEM& mem = pci.memory ;
	const PLAY_MIXER_CAPS& mix = pci.sound.mixer ;

	if ( cpu.manufacturer[0] == 0 )
		return true ;

	try
	{
		// These values might not be MySQL safe, so escape 'em
		// I would have used strnlen but redhat 6 didn't appear to provide it
		// This convoluted method of copying the data was intended to protect
		// us from malicious data being sent up by the user...
		strncpy(statement, ogl.cardName, MAX_OGL_CARD_NAME) ;
		statement[MAX_OGL_CARD_NAME] = 0;
		mysql_escape_string(cardName, statement, strlen(statement)) ;

		strncpy(statement, ogl.vendor, MAX_OGL_VENDOR_NAME) ;
		statement[MAX_OGL_VENDOR_NAME] = 0 ;
		mysql_escape_string(vendor, statement, strlen(statement)) ;

		cpuManufacturer = cpu.manufacturer ;
		cpuBrandName = cpu.brandName ;

		wwiiEscapeString(cpuManufacturer) ;
		wwiiEscapeString(cpuBrandName) ;

		if ( pci.sound.isValid )
		{
			strncpy(statement, mix.szPname, MAX_MIX_NAME) ;
			statement[MAX_MIX_NAME] = 0 ;
			mysql_escape_string(soundCard, statement, strlen(statement)) ;
		}
		else
			strncpy(soundCard, "*none*", MAX_MIX_NAME) ;

		descs[0] = 0 ;
		// Check we can trust the data
		if ( ogl.compTextures.num >= MAX_COMPRESSED_TEXTURE )
			numCompTextures = 0 ;
		else
		{
			numCompTextures = ogl.compTextures.num ;
			for ( i = 0, p = descs ; i < numCompTextures ; i++, p += 5 )
				snprintf(p, 6, "%04X ", (UINT32)ogl.compTextures.desc[i].value) ;
		}

		for ( i = 0, p = exts ; i < (UINT32)(MAX_OGL_EXTN_SPACE) ; i++, p += 2 )
		{
			snprintf(p, 3, "%02X", (UINT32)ogl.extension.bitmap[i]) ;
		}

		// For this to work depends on keeping the insert fields in the order
		// they are stored in the table definition. BEWARE.
		DBThread::Do(DBThread::Auth
					, "INSERT INTO `auth`.`info_ogl`"
						" VALUES"
						"("
						"  '%s', '%s', %u, %u, '%s', %u, '%s'"
						", %u, %u, %u, %u, %u"
						", %u, %u, %u, %u, %u"
						", %u, %u, %u, %u, %u"
						", %u, %u"
						", %d, %d, %d"
						", '%s', %d, %d"
						", '%s', '%s'"			// cpu manuf/brand
						", %u, %u, %u, %u"
						", %u, %u, %u, %u, %u"
						", %u, %u, %u, %u, %u"
						", %llu"				// Speed
						" )"
					, cardName
					, vendor
					, (UINT32)ogl.oglVersion
					, numCompTextures
					, descs
					, (UINT32)ogl.extension.numExtensions
					, exts

					, ogl.pixelFormat.formatNumber
					, ogl.pixelFormat.nSize
					, ogl.pixelFormat.nVersion
					, ogl.pixelFormat.dwFlags
					, ogl.pixelFormat.iPixelType

					, ogl.pixelFormat.cColorBits
					, ogl.pixelFormat.cRedBits
					, ogl.pixelFormat.cGreenBits
					, ogl.pixelFormat.cBlueBits
					, ogl.pixelFormat.cAlphaBits

					, ogl.pixelFormat.cAccumBits
					, ogl.pixelFormat.cAccumRedBits
					, ogl.pixelFormat.cAccumGreenBits
					, ogl.pixelFormat.cAccumBlueBits
					, ogl.pixelFormat.cAccumAlphaBits

					, ogl.pixelFormat.cDepthBits
					, ogl.pixelFormat.cStencilBits

					, mem.dwTotalPhys
					, mem.dwTotalPageFile
					, mem.dwTotalVirtual

					, mix.szPname
					, ((pci.sound.isValid) ? mix.wMid : 0)
					, ((pci.sound.isValid) ? mix.wPid : 0)

					, cpuManufacturer.c_str()
					, cpuBrandName.c_str()

					, (unsigned int)cpu.model
					, (unsigned int)cpu.family
					, (unsigned int)cpu.stepping
					, (unsigned int)cpu.processorType

					, (unsigned int)cpu.brandIndex
					, (unsigned int)cpu.extendedModel
					, (unsigned int)cpu.extendedFamily
					, (unsigned int)cpu.featureInfo
					, (unsigned int)cpu.extFeatureInfo

					, (unsigned int)cpu.apicPhysicalID
					, (unsigned int)cpu.l2Associativity
					, (unsigned int)cpu.cacheLineSize
					, (unsigned int)cpu.clflushCLS
					, (unsigned int)cpu.cacheSizeK

					, cpu.speed
					) ;

		mysql_escape_string(statement, cdata.m_username.c_str(), MAX_HANDLE_LEN * 2 + 1) ;
		DBSimpleQuery(dbh, "UPDATE `auth`.`auth_customer` SET `flags` = (`flags` | 2) WHERE `username` = '%s'", statement) ;

		return true ;
	}
	catch (std::exception& e)
	{
		_dbugLog(_FLA, "%s: %s", __FUNCTION__, e.what()) ;
	}

	return false ;
}

//////////////////////////////////////////////////////////////////////
struct vers_manifest*
matchversion(UINT32 needle, SINT32 appID, UINT32 platform)
{
	if ( appID < 0 || appID > MAX_APPIDS )
		return NULL;

	struct vers_manifest* vers_mans = NULL;
	SINT32 numEntries = 0;

	for ( SINT32 i = 0 ; i < config.numApps ; ++i )
	{
		if ( config.apps[i].appID != appID)
			continue ;

		vers_mans = config.apps[i].manifest;
		numEntries = apps[i].numEntries;
		break;
	}

	if ( vers_mans == NULL || numEntries < 1 )
		return NULL;

	struct vers_manifest* fullInstaller = NULL ;
	SINT32 lastMatchingEntry = 0 ;
	bool foundPlatform = false ;

	for ( SINT32 i = 0 ; i < numEntries ; ++i )
	{
		if ( vers_mans[i].platform != platform )
		{
			if ( !foundPlatform )
				continue ;
			break ;
		}

		foundPlatform = true ;

		// Fallback version
		if ( vers_mans[i].oldNo == 0 && vers_mans[i].newNo > needle
			&& (fullInstaller == NULL  || fullInstaller->newNo < vers_mans[i].newNo) )
			fullInstaller = &vers_mans[i] ;

		if( needle == vers_mans[i].oldNo )
		{
			return &vers_mans[i];
		}
		else if ( needle < vers_mans[i].oldNo )
		{
			break;
		}

		lastMatchingEntry = i;
	}

	if ( foundPlatform == false )
		return NULL ;

	if ( needle > vers_mans[lastMatchingEntry].oldNo
		&& needle < vers_mans[lastMatchingEntry].newNo )
	{
		return &vers_mans[lastMatchingEntry] ;
	}

	if ( fullInstaller != NULL )
		return fullInstaller ;

	return NULL;
}

//////////////////////////////////////////////////////////////////////
LOGICAL
authdbGetFileMD5(const UINT32 appID, const UINT32 platform, const char* filename, char* buffer, UINT32 bufLen)
{
	SCHAR statement[512+1] ;
	SCHAR myFilename[MAXPATHLEN*2+1] ;

	assert(buffer != NULL && bufLen > 1) ;

	strncpy(statement, filename, sizeof(statement)) ;
	statement[sizeof(statement) - 1] = 0;
	mysql_escape_string(myFilename, statement, strlen(statement)) ;

	snprintf(statement, sizeof(statement)-1
				, "SELECT checksum"
					" FROM auth_checksums"
					" WHERE appID = appID"
						" AND platform = platform"
						" AND filename = LOWER('%s')"
						" LIMIT 1"
					, myFilename
					) ;

	try
	{
		DBQuery q(dbh, statement) ;
		if ( q.HasRows() && q.FetchRow() )
		{
			q.Row().NextString(buffer, bufLen) ;
			return true ;
		}
	}
	catch ( std::exception& e )
	{
		dbugLog(_FLA, "error reading checksum for %s:%d.%d err:%s", myFilename, appID, platform, e.what()) ;
	}

	dbugLog(_FLW, "No checksums for %s:%d.%d", myFilename, appID, platform) ;
	return false ;
}

//////////////////////////////////////////////////////////////////////
// Save the last OTP value we used in this sequence

void
authdbSaveLastOTP(UINT64 otp)
{
	try
	{
		DBQuery q(dbh, "REPLACE INTO `auth`.`auth_variables` (`name`, `value`) VALUES ('last_otp', %llu)", otp) ;
		dbugLog(_FLI, "%s: saved last OTP of %llu", __FUNCTION__, otp) ;
	}
	catch (std::exception& e)
	{
		dbugLog(_FLA, "Unable to save OTP to database: %s", e.what()) ;
	}
}

//////////////////////////////////////////////////////////////////////
// Log an invalid username request

void
authdbLogBadPlayername(UINT32 ip, const std::string& username, std::string reason)
{
	try
	{
		if ( username == "blitz" )
			return ;
		wwiiEscapeString(reason) ;
		DBQuery q(dbh, "INSERT INTO `auth`.`auth_illegal_users` (`recorded`, `ip`, `username`, `reason`) VALUES (NOW(), %u, '%s', '%s')", ip, username.c_str(), reason.c_str()) ;
	}
	catch ( std::exception& e )
	{
		dbugLog(_FLA, "%s: %s", __FUNCTION__, e.what()) ;
	}
}

//////////////////////////////////////////////////////////////////////
// Get the last OTP generated in the previous sequence run

UINT32
authdbGetLastOTP()
{
	try
	{
		DBQuery q(dbh, QRY_GET_LAST_OTP) ;
		if ( q.HasRows() == false || q.FetchRow() == false )
		{
			dbugLog(_FLW, "No rows returned from next-otp table") ;
			return 1 ;
		}

		UINT32 nextOTP = q.Row().NextUInt() ;
		dbugLog(_FLN, "%s: loaded last OTP of %u", __FUNCTION__, nextOTP) ;

		return nextOTP ;
	}
	catch (std::exception& e)
	{
		dbugLog(_FLA, "Unable to fetch next OTP from database: %s", e.what()) ;
	}

	return 1 ;
}

//////////////////////////////////////////////////////////////////////
void
authdbUpdateArenaStatus(UINT32 arenaID, SINT32 lock, UINT32 players)
{
	try
	{
		DBQuery q(dbh
					, "UPDATE `auth`.`auth_arenas`"
						" SET  `lock_state` = %d"
							  ", `players` = %u"
						" WHERE `arenaID` = %u"
							, lock, players, arenaID
				) ;
	}
	catch ( std::exception& e )
	{
		dbugLog(_FLA, "%s: %s", __FUNCTION__, e.what()) ;
	}
}

//////////////////////////////////////////////////////////////////////
void
authdbReadArenaAliases()
{
	try
	{
		DBQuery q(dbh, "SELECT aa.arenaID, aa.alias"
						" FROM auth.auth_arenas aa"
						" WHERE aa.controlVisible = 'visible'"
							" AND aa.alias IS NOT NULL"
							" AND alias <> ''") ;
		g_arenaAliases.clear() ;
		while ( q.FetchRow() )
		{
			DBRow& row = q.Row() ;
			UINT32 arenaID = row.NextUInt() ;
			g_arenaAliases[arenaID] = row.NextColumn() ;
		}
	}
	catch (std::exception& e)
	{
		dbugLog(_FLA, "Unable to fetch arena alias list: %s", e.what()) ;
	}
}

//////////////////////////////////////////////////////////////////////
const char*
authdbGetArenaAlias(UINT32 arenaID)
{
	ARENA_ALIASES::iterator it = g_arenaAliases.find(arenaID) ;
	return ( it != g_arenaAliases.end() ) ? it->second.c_str() : NULL ;
}
