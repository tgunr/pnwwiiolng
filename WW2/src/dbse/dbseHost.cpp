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
#include "dbse/dbseConnection.h"
#include "server/serverConfigVars.h"
#include "server/serverInterfaceDescription.h"			// For myInterface
#include "server/serverInitializeProcess.h"
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

extern SINT32 dbseInstallHostCallbacks();

static void _acceptFunction(ENDPOINT& endpoint)
{
	dbugLog(_FLN, "t:%d client connection %s", endpoint.teulIndex(), teulGetAddress(endpoint.fromAddress()));
}

static void _errorFunction(SINT32 teulIndex, const UCHAR* const /*data*/, SINT32 /*size*/, const TEUL_CALLBACK* const /*object*/, SINT32 error)
{
	if (error == ENOTCONN)
		dbugLog(_FLW, "t:%d connection closed", teulIndex);
	else
		dbugLog(_FLA, "t:%d recieved error %d", error, teulIndex);
}

static void _initDB()
{
	ABORT_IF_EX(dbh.connect(dbConnection) == false, "DBD can't operate without a database connection.");

	dbugLog(_FLI, "Connected to WWII_DB");
}

static const char* g_serverName = "dbd";

int main(int argc, const char* argv[])
{
	Server::InitializeProcess(g_serverName, DBD_HOST, _errorFunction, nullptr, argc, argv, {});
	dbugSetWriteLevel(Debug::ERR_INFO | DEF_ERR_FLAGS);
	dbugSetLogLevel(DEF_ERR_LEVEL | DEF_ERR_FLAGS);

	threadsReadDBEnvironment();
	dbugLog(_FLI,"Database Proxy Host starting. Compiled %s at %s", __DATE__, __TIME__);

	if ( chatdbInstallHostCallbacks() )
		LogAbort("Error installing callbacks");

	_initDB();

	wwiiIRCInitialize(Server::myInterface.addr[0].ip, false);
	wwiiIRCActivate(true);

	ABORT_IF(dbseInstallHostCallbacks() < 0);

	dbugLog(_FLI, "Opening listen socket for arena:%u on port:%u", cv_arenaID.GetUint32(), WWII::Net::Ports::Dbse);

	teulindex_t listenIndex = teulOpenListen(WWII::Net::Ports::Dbse, _acceptFunction, 20000, PROTOCOL_TCP, _errorFunction);
	ABORT_IF(listenIndex < 0);

	std::this_thread::sleep_for(std::chrono::milliseconds(50));

	wwiiIRCChannelMessage("-sys,#servers", "++ Running ++");

	while (1)
	{
		teulUpdateTransport(50);
		threadsLogging();
	}

	wwiiIRCTerminate("[[ Shutting Down ]]");
	teulClose(listenIndex);

	return 0;
}
