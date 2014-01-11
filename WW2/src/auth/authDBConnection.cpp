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

extern ConfigAddr cv_authdbAddress;
extern ConfigStr cv_authdbPort;
extern ConfigStr cv_authdbPassword;
extern ConfigStr cv_authdbSchema;
extern ConfigStr cv_authdbUser;

//////////////////////////////////////////////////////////////////////
// Populate the supplied DBCONNECTION with details.

void
authGetDBConnectionInfo(DBCONNECTION* connectionInfo)
{
	// Look up the cannonical name/address of the auth database
	// and resolve it to an IP
	UINT32* dbIp = teulGetHostByName(cv_authdbAddress.GetString());
	if ( dbIp == nullptr )
		LogAbort("Invalid %s: %s (unable to resolve)"
					, cv_authdbAddress.GetEnvVarName(), cv_authdbAddress.GetString());

	// Populate the connection info structure.
	strcpy(connectionInfo->host, teulGetAddress(*dbIp));
	strcpy(connectionInfo->dbName, cv_authdbSchema.GetString());
	strcpy(connectionInfo->username, cv_authdbUser.GetString());
	strcpy(connectionInfo->password, cv_authdbPassword.GetString());
	strcpy(connectionInfo->port, cv_authdbPort.GetString());

	// Release the list of addresses we were given
	memFree(dbIp);
}

