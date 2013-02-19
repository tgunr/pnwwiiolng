#include <syscfg.h> // Path is project specific.
// Created: Mon Sep 11 18:47:34 2000
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Eric Hall

#include "dbse/dbseConstants.h"
#include "dbse/dbseTypes.h"
#include "teul/teulServer.h"
#include "dbse/dbseDaemon.h"
#include "teul/teulEndpoint.h"
#include "server/serverConfigVars.h"

static LOGICAL listening = FALSE;
static SINT32 hostListenTCP;
static SINT32 arenaID;

extern SINT32 dbseInstallHostCallbacks() ;

static void _errorFunction(SINT32 teulIndex, const UCHAR* const /*data*/, SINT32 /*size*/, const TEUL_CALLBACK* const /*object*/, SINT32 error)
{
	if (error == ENOTCONN)
		dbugLog(_FLA, "connection index %d closed", teulIndex);
	else
		dbugLog(_FLA,"recieved error %d on index %d",error,teulIndex);

}

static void
_acceptFunction(ENDPOINT& endpoint)
{
	dbugLog(_FLN, "t:%d client connection %s", endpoint.teulIndex(), teulGetAddress(endpoint.fromAddress()));
	listening = TRUE;
}

SINT32 dbseInitHost(UINT16 port)
{
	ABORT_IF(dbseInstallHostCallbacks() < 0);

	dbugLog(_FLI, "Opening listensocket for arena:%d on port:%d", arenaID,port);

	hostListenTCP = teulOpenListen(port, _acceptFunction, 20000, TCP, _errorFunction) ;
	ABORT_IF(hostListenTCP < 0);

	return hostListenTCP;
}

