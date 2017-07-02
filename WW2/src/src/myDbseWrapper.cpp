#include <syscfg.h> // Path is project specific.
// Created: Wed May 17 17:03:35 2000
//
// This is the Database Wrapper main file.  It contains all the fun code
// to connect, disconnect, and interact with the database backend.
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

#include <mutex>
#include <mysql.h>

#include "dbse/dbseWrapper.h"
#include "mem/mem.h"
#include "wwii/wwiiThreads.h"
#include "include/configVars.h"

static std::atomic_bool s_dbInitialized;
static const char* s_dbAddress;
static unsigned short s_dbPort;
static const char* s_dbSchema;
static const char* s_dbUser;
static const char* s_dbPassword;

DBHANDLE dbh;

//////////////////////////////////////////////////////////////////////
// Each process communicates with one database.

void
dbseSetDatabase(const ConfigAddr& addr, const ConfigAddr& port, const ConfigStr& schema, const ConfigStr& user, const ConfigStr& password)
{
	static std::mutex mutex;
	std::lock_guard<std::mutex> lock(mutex);
	if (s_dbInitialized)
		return;

	UINT32 dbIP = Teul::GetSingleIP(addr.GetString());
	if ( !dbIP ) {
		LogAbort("Invalid %s: %s (unable to resolve)",
					addr.GetEnvVarName(), addr.GetString());
	}

	dbugLog(_FLD, "Database is %s:%s [%08x] schema:%s user:%s",
					addr.GetString(), port.GetString(), dbIP,
					schema.GetString(), user.GetString());

	s_dbAddress = addr.GetString();
	s_dbPort = (unsigned short)strtoul(port.GetString(), nullptr, 10);
	s_dbSchema = schema.GetString();
	s_dbUser = user.GetString();
	s_dbPassword = password.GetString();

	s_dbInitialized = true;
}


//////////////////////////////////////////////////////////////////////
// Low-level MySQL connection initialization

static DB::handle_t*
_doMyConnect(bool autoReconnect)
{
	// Make sure MySQL is initialized and construct a connection structure.
	DB::handle_t* tempConn = mysql_init(nullptr);
	if (tempConn == nullptr)
	{
		dbugLog(_FLA, "mysql_init() failed!");
		return(nullptr);
	}

	// Attempt to establish the connection
	auto flags = CLIENT_MULTI_STATEMENTS | CLIENT_MULTI_RESULTS;
	if ( cv_mysqlCompression.GetBool() )
		flags |= CLIENT_COMPRESS;

	if ( mysql_real_connect(tempConn, s_dbAddress, s_dbUser, s_dbPassword, s_dbSchema, s_dbPort, nullptr, flags) == nullptr )
	{
		dbugLog(_FLA, "mysql_real_connect failed:\nError %u (%s) \n", mysql_errno(tempConn), mysql_error(tempConn));
		dbugLog(_FLA, "using host %s:%hu user %s schema %s", s_dbAddress, s_dbPort, s_dbUser, s_dbSchema);
		return nullptr;
	}

#if MYSQL_VERSION_ID >= 50003
	// Enable automatic reconnection
	my_bool reconnect = (autoReconnect) ? 1 : 0;
	mysql_options(tempConn, MYSQL_OPT_RECONNECT, &reconnect);
#endif

	if ( mysql_select_db(tempConn, s_dbSchema) != 0 )
	{
		dbugLog(_FLA, "unable to select schema %s", s_dbSchema);
		mysql_close(tempConn);
		return nullptr;
	}

	if ( mysql_set_character_set(tempConn, "utf8") != 0 )
	{
		dbugLog(_FLA, "set names failed");
		mysql_close(tempConn);
		return nullptr;
	}

	return tempConn;
}

// ------ Public Functions

static std::mutex dbconnMutex;

DB::handle_t*
dbseGetConnection(bool report, bool autoReconnect)
{
	dbseSetDatabase(cv_wwiiDBAddress, cv_wwiiDBPort, cv_wwiiDBDefaultSchema, cv_wwiiDBUser, cv_wwiiDBPassword);
	if ( !s_dbAddress[0] || !s_dbPort || !s_dbSchema[0] || !s_dbUser[0] || !s_dbPassword[0] ) {
		LogAbort("Missing database configuration, can't connect");
	}

    dbugLog(_FLD, "Getting thread connection to host '%s:%hu' database '%s' user '%s'"
				, s_dbAddress, s_dbPort, s_dbSchema, s_dbUser);

	DB::handle_t* newConn = nullptr;
	std::this_thread::sleep_for(std::chrono::microseconds(250));
	{
		std::lock_guard<std::mutex> dbconnLock(dbconnMutex);
		newConn = _doMyConnect(autoReconnect);
	}

	if ( report )
	{
		dbugLog(_FLI, "Connection to host '%s:%hu' database '%s' user '%s' reconnect '%s' %s."
						, s_dbAddress, s_dbPort, s_dbSchema, s_dbUser
						, (autoReconnect ? "auto" : "manual")
						, ((newConn != nullptr) ? "successful" : "FAILED")
						);
	}

	std::this_thread::yield();

	return newConn;
}

bool
DBHANDLE::connect()
{
	assert( m_initialized == false );

	//Initialization stuff;
	m_conn = dbseGetConnection(true);
	if ( nullptr == m_conn )
		return false;

	m_initialized = true;

	return true;
}

void
DBHANDLE::disconnect()
{
	DB::handle_t* const oldConn = m_conn;
	m_conn = nullptr;

	if ( m_initialized == false )
		return;

	mysql_close(oldConn);
}

const char*
escapeString(const char* src, char* dest, size_t destSize)
{
	size_t len = strlen(src);
	if ( len > destSize / 2 )
		len = destSize / 2;

	mysql_escape_string(dest, src, len);

	return dest;
}

