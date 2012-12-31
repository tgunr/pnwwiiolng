#include <syscfg.h> // Path is project specific.
// Created: Wed Mar  4 11:57:17 CST 2009
//
// Populate a DBCONNECTION with details for connecting
// to the auth server.
//
// Copyright (C) PlayNet, Inc 1999-2009 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "dbug/dbug.h"
#include "dbse/dbseConnection.h"
#include "teul/teulBasic.h"
#include "mem/mem.h"

#if defined( HAPPYOO )
# define AUTH_PASSWORD "m4nd0l1n"			// Happyoo's auth password
#else
# define AUTH_PASSWORD "scalar449"			// Playnet's auth password
#endif

//////////////////////////////////////////////////////////////////////
// Populate the supplied DBCONNECTION with details.

void
authGetDBConnectionInfo(DBCONNECTION* connectionInfo)
{
	// Look up the cannonical name/address of the auth database.
	static const char* varName = "AUTH_DB" ;
	char* dbAddr = getenv(varName) ;
	if ( !dbAddr || *dbAddr == 0 )
		LogAbort("Unable to read %s from environment", varName) ;

	// Resolve it to an IP
	UINT32* dbIp = teulGetHostByName(dbAddr) ;
	if ( dbIp == NULL )
		LogAbort("Invalid %s: %s (unable to resolve)", varName, dbAddr) ;

	// Populate the connection info structure.
	strcpy(connectionInfo->host, teulGetAddress(*dbIp)) ;
	strcpy(connectionInfo->dbName, "auth") ;
	strcpy(connectionInfo->username, "authuser") ;
	strcpy(connectionInfo->password, AUTH_PASSWORD) ;
	strcpy(connectionInfo->port, "") ;

	// Release the list of addresses we were given
	memFree(dbIp) ;
}

