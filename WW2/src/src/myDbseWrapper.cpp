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

#include "dbse/dbseConnection.h"
#include "dbse/dbseWrapper.h"
#include "dbug/dbug.h"
#include "mem/mem.h"
#include <mutex>
#include <mysql.h>
#include "wwii/wwiidbConfigVars.h"
#include "wwii/wwiiThreads.h"

#include "teul/teulConfigVar.h"

extern ConfigBool cv_mysqlCompression;

//////////////////////////////////////////////////////////////////////
// Low-level MySQL connection initialization

static DB::handle_t*
_doMyConnect(const DBCONNECTION& connection, bool autoReconnect)
{
	DB::handle_t* tempConn;

	// Make sure MySQL is initialized and construct a connection structure.
	tempConn = mysql_init(NULL);
	if (tempConn == NULL)
	{
		dbugLog(_FLA, "mysql_init() failed!");
		return(NULL);
	}

	// Attempt to establish the connection
	auto flags = CLIENT_MULTI_STATEMENTS | CLIENT_MULTI_RESULTS;
	if ( cv_mysqlCompression.GetBool() )
		flags |= CLIENT_COMPRESS;

	if ( mysql_real_connect(tempConn, connection.host, connection.username, connection.password, connection.dbName, 0, NULL, flags) == NULL )
	{
		threadLog(_FLA, "mysql_real_connect failed:\nError %u (%s) \n", mysql_errno(tempConn), mysql_error(tempConn));
		threadLog(_FLA, "using host %s name %s password %s dbname %s"
						, connection.host, connection.username, connection.password, connection.dbName);
		return(NULL);
	}

#if MYSQL_VERSION_ID >= 50003
	// Enable automatic reconnection
	my_bool reconnect = (autoReconnect) ? 1 : 0;
	mysql_options(tempConn, MYSQL_OPT_RECONNECT, &reconnect);
#endif

	if ( mysql_set_character_set(tempConn, "utf8") != 0 )
	{
		threadLog(_FLA, "set names failed");
		mysql_close(tempConn);
		return NULL;
	}

//	mysql_set_character_set(tempConn, "utf8");

	return(tempConn);
}

// ------ Public Functions

static std::mutex dbconnMutex;

DB::handle_t*
dbseGetConnection(const DBCONNECTION& connection, bool report, bool autoReconnect)
{
	if ( connection.host == NULL ) return NULL;
	if ( connection.dbName == NULL ) return NULL;
	if ( connection.username == NULL ) return NULL;
	if ( connection.password == NULL ) return NULL;

	DB::handle_t* newConn = NULL;
	std::this_thread::sleep_for(std::chrono::microseconds(250));
	{
		std::lock_guard<std::mutex> dbconnLock(dbconnMutex);
		newConn = _doMyConnect(connection, autoReconnect);
	}

	if ( report )
	{
		dbugLog(_FLI, "Connection to host '%s' database '%s' user '%s' password '%s' reconnect '%s' %s."
						, connection.host, connection.dbName
						, connection.username, connection.password
						, (autoReconnect ? "auto" : "manual")
						, ((newConn != NULL) ? "successful" : "FAILED")
						);
	}

	std::this_thread::yield();

	return newConn;
}

bool
DBHANDLE::connect(const DBCONNECTION& connection)
{
	assert( m_initialized == false );

	//Initialization stuff;
	m_conn = dbseGetConnection(connection, true);
	if ( NULL == m_conn )
		return false;

	m_initialized = true;

	return true;
}

void
DBHANDLE::disconnect()
{
	DB::handle_t* const oldConn = m_conn;
	m_conn = NULL;

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

