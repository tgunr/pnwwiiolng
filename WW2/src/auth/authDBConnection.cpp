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

#include "teul/teulConfigVar.h"

CVAR_ADDR(authdbAddress, NULL, "AUTH_DB", true
			, "Address of the MySQL server hosting the auth database.");
CVAR_STRING(authdbPort, "", "AUTH_DB_PORT", true
			, "Port of the Auth MySQL server if not default.");
#if defined( HAPPYOO )
CVAR_STRING(authdbPassword, "m4nd0l1n", "AUTH_DB_PASSWD", true
			, "Password to use for the auth database.");
#else
CVAR_STRING(authdbPassword, "scalar449", "AUTH_DB_PASSWD", true
			, "Password to use for the auth database.");
#endif
CVAR_STRING(authdbSchema, "auth", "AUTH_DB_SCHEMA", true
			, "Schema to use for auth data.");
CVAR_STRING(authdbUser, "authuser", "AUTH_DB_USER", true
			, "Username when connecting to the auth database.");

//////////////////////////////////////////////////////////////////////
// Populate the supplied DBCONNECTION with details.

void
authGetDBConnectionInfo(DBCONNECTION* connectionInfo)
{
	// Look up the cannonical name/address of the auth database
	// and resolve it to an IP
	UINT32* dbIp = teulGetHostByName(cv_authdbAddress.GetString()) ;
	if ( dbIp == NULL )
		LogAbort("Invalid %s: %s (unable to resolve)"
					, cv_authdbAddress.GetEnvVarName(), cv_authdbAddress.GetString()) ;

	// Populate the connection info structure.
	strcpy(connectionInfo->host, teulGetAddress(*dbIp)) ;
	strcpy(connectionInfo->dbName, cv_authdbSchema.GetString()) ;
	strcpy(connectionInfo->username, cv_authdbUser.GetString()) ;
	strcpy(connectionInfo->password, cv_authdbPassword.GetString()) ;
	strcpy(connectionInfo->port, cv_authdbPort.GetString()) ;

	// Release the list of addresses we were given
	memFree(dbIp) ;
}

