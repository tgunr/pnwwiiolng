#include <syscfg.h> // Path is project specific.
// Created: Tue Sep 12 14:56:22 2000
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Eric Hall

#include "chat/chatdbHostMessages.h"
#include "chat/chatdbParser.h"
#include "dbse/dbseTypes.h"
#include "dbse/dbseHandle.h"
#include "dbse/dbseDaemon.h"
#include "dbse/dbseConnection.h"
#include "server/serverInterfaceDescription.h"			// For myInterface
#include "teul/teulKit.h"
#include "wwii/wwiiIRC.h"
#include "wwii/wwiiNetPorts.h"
#include "wwii/wwiiThreads.h"

#ifdef BETA_RELEASE
	#define DEF_ERR_LEVEL Debug::ERR_DBUG
#else
	#define DEF_ERR_LEVEL Debug::ERR_INFO
	//#define DEF_ERR_LEVEL Debug::ERR_NOTE
#endif

#define DEF_ERR_FLAGS STD_ERR

DBCONNECTION dbConnection;
DBHANDLE dbh;

static void _errorFunction(SINT32 teulIndex, const UCHAR* const /*data*/, SINT32 /*size*/, const TEUL_CALLBACK* const /*object*/, SINT32 error)
{
	if (error == ENOTCONN)
		dbugLog(_FLW, "t:%d connection closed", teulIndex);
	else
		dbugLog(_FLA, "t:%d recieved error %d", error, teulIndex);
}

static void
_initDB()
{
	ABORT_IF_EX(dbh.connect(dbConnection) == false, "DBD can't operate without a database connection.");

	dbugLog(_FLI, "Connected to WWII_DB");
}

static const char* g_serverName = "dbd";

int main(int argc, char* argv[])
{
	Teul::SetAppName(g_serverName);
	Teul::ConfigVar::InitializeConfigVars();

	SINT32 listenIndex;

	if (teulInitTransport(LOG_TO_FILE, g_serverName, _errorFunction) < 0)
	{
		fprintf(stderr, "Unable to initialize teulTransport\n");
		exit(-ENOTCONN);
	}
	dbugSetWriteLevel(Debug::ERR_INFO | DEF_ERR_FLAGS);
	dbugSetLogLevel(DEF_ERR_LEVEL | DEF_ERR_FLAGS);
	dbugLog(_FLI,"Database Proxy Host starting. Compiled %s at %s", __DATE__, __TIME__);

	threadsReadDBEnvironment();

	if ( chatdbInstallHostCallbacks() )
		LogAbort("Error installing callbacks");

	_initDB();

	wwiiIRCInitialize(Server::myInterface.addr[0].ip, false);
	wwiiIRCActivate(true);

	listenIndex = dbseInitHost(WWII::Net::Ports::Dbse);

	std::this_thread::sleep_for(std::chrono::milliseconds(50));

	wwiiIRCChannelMessage("-sys,#servers", "++ Running ++");

	while (1)
	{
		teulUpdateTransport(50);
		threadsLogging();
	}

	wwiiIRCTerminate("[[ Shutting Down ]]");
	teulClose(listenIndex);
	return(0);
}

