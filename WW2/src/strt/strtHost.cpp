#include <syscfg.h> // Path is project specific.
// Created: Tue Apr 10 13:43:09 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

#include "versionNo.h"
#include "hostproj/hostversion.h"
#include "hostproj/revision.h"

#include <signal.h>  //to catch SIGHUPS with

#include "mem/mem.h"
#include "auth/authDaemon.h"
#include "chat/chatStratSupportMessages.h"
#include "chat/chatSupportRPCMessages.h"
#include "cnvy/cnvyMain.h"
#include "dbse/dbseConnection.h"
#include "dbse/dbseHandle.h"
#include "include/arenaLockEnums.h"
#include "sdm/sdmConfigVars.h"
#include "server/serverCodeEnum.h"
#include "server/serverFuncTypes.h"
#include "server/server.h"
#include "server/serverIndexes.h"
#include "server/serverInterfaceDescription.h"
#include "server/serverPersonaExit.h"
#include "server/serverPersonaLogin.h"
#include "server/serverReleaseTypeEnum.h"
#include "server/serverServerInfo.h"
#include "strt/strtConstants.h"
#include "strt/strtCPClass.h"
#include "strt/strtCP.h"
#include "strt/strtData.h"
#include "strt/strtDB.h"
#include "strt/strtDotCommands.h"
#include "strt/strtEnemyReport.h"
#include "strt/strtHostDB.h"
#include "strt/strtHostMessages.h"
#include "strt/strtMain.h"
#include "strt/strtObjectiveStats.h"
#include "strt/strtSupply.h"
#include "strt/strtSync.h"
#include "strt/strtTOE.h"
#include "teul/teulEndpoint.h"
#include "teul/teulKit.h"
#include "teul/teulServer.h"
#include "wwii/wwiiConfig.h"
#include "wwii/wwiiIRC.h"
#include "wwii/wwiiNetPorts.h"
#include "wwii/wwiiThreads.h"

#include "include/luaGlue.h"
#include "strt/strtHostLua.h"

extern ObjectiveStats g_objectiveStats;

DBHANDLE dbh;
DBCONNECTION dbConnection;

#ifdef BETA_RELEASE
	#define DEF_ERR_LEVEL Debug::ERR_DBUG
#else
	#define DEF_ERR_LEVEL Debug::ERR_INFO
	//#define DEF_ERR_LEVEL Debug::ERR_NOTE
#endif

#define DEF_ERR_FLAGS 0

const char* HostLanguage = "en";

static LOGICAL _exitSignaled = FALSE;
static bool _mustSyncChatHost = true;
extern int tolua_stratd_open(struct lua_State*);
extern ConfigPath cv_luaPath;

//////////////////////////////////////////////////////////////////////////
// Users never directly connect to strat, so while we are aware of them
// thru teul-server, we don't actually need to process them.

static SINT32	_personaCreate(const Server::PersonaLogin& login, PlayerStatusNode& psn)
{
	return 0;
}

static SINT32	_personaRelease(SINT32 teulIndex, PlayerStatusNode* psn, Server::ReleaseType relaseType)
{
	return 0;
}

static SINT32	_personaActivate(SINT32 teulIndex , PlayerStatusNode* psn)
{
	return 0;
}

static SINT32	_personaExit(SINT32 teulIndex, PlayerStatusNode* psn, Server::PersonaExit& persXfer)
{
	return 0;
}

static void	_personaErrorFunction (SINT32 teulIndex, const UCHAR* const data, SINT32 size, const TEUL_CALLBACK* const object, SINT32 error)
{
}

//////////////////////////////////////////////////////////////////////////
//This is my error function for bailing out of teulkit

static void _cnvyErrorFunction(SINT32 teulIndex, const UCHAR* const data, SINT32 size, const TEUL_CALLBACK* const object, SINT32 error)
{
	dbugLog(_FLA,"cnvyErrorFunction: index %d code %d\n\n",teulIndex, error);
}

//////////////////////////////////////////////////////////////////////////
// Theoretically invoked by teulKit shutting down - so what this actually
// does is probably not a really good idea.

static void _killConvoy(SINT32 teulIndex, const UCHAR* const data, SINT32 size, const TEUL_CALLBACK* const object, SINT32 error)
{
	//Close transport

#ifdef DEBUG
	dbugLog(_FLN,"closing transport");
#endif
	teulCloseTransport();

	dbugLog(_FLN,"server exiting normally");

	exit(0);
}

//////////////////////////////////////////////////////////////////////////
// Handle a direct, explicit map connection.
///TODO: Remove: Use serverPeer API instead now that it provides us a
/// direct, TCP based connection to all peers.

static void
_acceptMapConnection(ENDPOINT& endpoint)
{
	dbugLog(_FLN, "t:%d map connection opened %s", endpoint.teulIndex(), teulGetAddress(endpoint.fromAddress()));
	cnvySetMapConnection(endpoint.teulIndex());
}

static void _mapErrorFunction(SINT32 teulIndex, const UCHAR* const data, SINT32 size, const TEUL_CALLBACK* const object, SINT32 error)
{
	dbugLog(_FLN, "t:%d map connection closed", teulIndex);
}

//////////////////////////////////////////////////////////////////////////
// Describe our configuration to the log file so we can tell at a glance
// what is actually running when looking at log files.

static void _describeBuildConfigurationToLogFile()
{
	dbugLog(_FLI, REPOSITORY_INFO " HOST FLAGS:");

#if defined(DEV_BUILD)
	dbugLog(_FLIB, "DEV_BUILD");
#endif
#if defined(BETA_RELEASE)
	dbugLog(_FLIB, " BETA_RELEASE");
#endif
#if defined(TEST_RELEASE)
	dbugLog(_FLIB, " TEST_RELEASE");
#endif
#if defined(OPEN_BETA)
	dbugLog(_FLIB, " OPEN_BETA");
#endif
#if defined(CLOSED_BETA)
	dbugLog(_FLIB, " CLOSED_BETA");
#endif
#if defined(FRES)
	dbugLog(_FLIB, " FRES");
#endif
#if defined(HAPPYOO)
	dbugLog(_FLIB, " HAPPYOO");
#endif
#if defined(TRAINING_SERVER)
	dbugLog(_FLIB, " TRAINING_SERVER");
#endif
#if defined(PUBLIC_RELEASE)
	dbugLog(_FLIB, " PUBLIC_RELEASE");
#endif

	dbugLog(_FLIB, ".");
}

//////////////////////////////////////////////////////////////////////////
// If the cluster has gone into a strange state, e.g. the map server is
// restarting, put it into a "locked" state which disables things like
// capture or kills, and also alert the players.

static const char* _serverCode[] =
{
	"INVALID-CODE"
,	"added"
,	"exited"
,	"replaced"
,	"status"
,	"isMaster"
,	"notMaster"
,	"deleted"
,	"timedOut"
,	"insertError"
};

static const char* _serverStatus[] =
{
	"login"
,	"initializing"
,	"accepting"
,	"full"
};

void
_checkArenalock(Server::ServerInfo* server, Server::Status state)
{
	static bool _autolock = false;
	static UCHAR _previousLockState = 0;
	static const UINT32 MMHOST_LOCK = AL_SPAWN | AL_CAPTURE | AL_KILL;

	if ( server->stats.type != MAP_HOST )
		return;

	// If the map mission host goes into accepting mode while we are autolocked,
	// then turn the autolock back off.
	if ( state >= Server::SERVER_ACCEPTING && _autolock )
	{
		dbugLog(_FLW, "AUTOLOCK OFF: mmhost has come back");
		_autolock = false;
		if ( host_config.arena.lock == MMHOST_LOCK )
		{
			host_config.arena.lock = _previousLockState;
			dotApplyConfig();
		}
	}
	// If the map mission host goes away, turn on the autolock.
	else if ( state < Server::SERVER_ACCEPTING && host_config.isArenaOpen() )
	{
		dbugLog(_FLW, "AUTOLOCK ON: mmhost gone away!");
		_autolock = true;
		_previousLockState = host_config.arena.lock;
		host_config.arena.lock = MMHOST_LOCK;
		dotApplyConfig();
	}
}

//////////////////////////////////////////////////////////////////////////
// teulServer API notifying us of a first time sighting of a process,
// either it just started or we did.

static SINT32
_serverEnterCallback(Server::ServerInfo* server, Server::Code scode)
{
	UINT32 broadAddr = Server::BroadcastMaskAddress(server->stats.address);

 	dbugLog(_FLN,"server %s added code %s[%d], type %d", teulGetAddress(server->stats.address), _serverCode[scode], scode, server->stats.type);

	//If its a cell host and its already up, we need to sync it, since the
	//	firebases have changed
	switch ( server->stats.type )
	{
		case MAP_HOST:
			_checkArenalock(server, server->stats.status);
			// FALL THRU

		case CELL_HOST:
			if ( server->stats.status == Server::SERVER_ACCEPTING )
			{
				dbugLog(_FLN, "syncing already initialized %s server at %s"
								, ((server->stats.type == CELL_HOST) ? "cell" : "map")
								, teulGetAddress(broadAddr)
								);
				strtAddSyncPeer(broadAddr);
			}
			else
			{
				// Otherwise if it's entering, it may have hiccuped;
				// remove any pending sync attempt we're trying on it
				dbugLog(_FLN, "removing %s at %s from sync peer queue due to state change"
								, ((server->stats.type == CELL_HOST) ? "cell" : "map")
								, teulGetAddress(broadAddr)
								);
				strtRemoveSyncPeer(broadAddr);
			}
		break;

		case CHAT_HOST:
			_mustSyncChatHost = true;
		break;

		default: break;
	}

	strtBroadcastConfigUpdate();

 	return 0;
}

//////////////////////////////////////////////////////////////////////////
// teulServer API notifying us that a server process went away.

static SINT32
_serverLeaveCallback(Server::ServerInfo* server, Server::Code scode)
{
 	dbugLog(_FLN,"server %s left code %s[%d], type %d"
					, teulGetAddress(server->stats.address), _serverCode[scode], scode, server->stats.type);

	_checkArenalock(server, Server::SERVER_INITIALIZING);
	strtRemoveSyncPeer(Server::BroadcastMaskAddress(server->stats.address));

	switch ( server->stats.type )
	{
	case CHAT_HOST:
		_mustSyncChatHost = false;
	break;
	default: break;
	}

 	return 0;
}

//////////////////////////////////////////////////////////////////////////
// teulServer API providing us a heartbeat status for a server.

static SINT32
_serverStatusCallback(Server::ServerInfo* server, Server::Code scode)
{
	UINT32 broadAddr = Server::BroadcastMaskAddress(server->stats.address);

 	dbugLog(_FLN,"server status changed for %s, code is %s[%d] status is %s[%d]"
				, teulGetAddress(server->stats.address)
				, _serverCode[scode], scode
				, _serverStatus[server->stats.status], server->stats.status
				);

	//Next few lines swiped very unceremoniously from Stick
	//if they're just coming on line, bring them up to speed on the strat
	//	system
	//Removed master crap since "THERE CAN BE ONLY ONE" strat server (for
	//	right now
	if ( scode == Server::SCODE_STATUS )
	{
		if ( server->stats.status == Server::SERVER_INITIALIZING)
		{
			switch (server->stats.type)
			{
				case MAP_HOST:
				case CELL_HOST:
				dbugLog(_FLN, "syncing %s server at %s"
									, ((server->stats.type == CELL_HOST) ? "cell" : "map")
									, teulGetAddress(broadAddr));
					strtAddSyncPeer(broadAddr);
				break;

				case CHAT_HOST:
					_mustSyncChatHost = false;
				break;

				default: break;
			}
		}
		else
		{
			if ( server->stats.status != Server::SERVER_ACCEPTING )
				strtRemoveSyncPeer(broadAddr);
		}

		_checkArenalock(server, server->stats.status);
	}

 	return 0;
}

//////////////////////////////////////////////////////////////////////////
// Handle sig-int.

static void
_strtSIGINT(int signo)
{
	dbugLog(_FLI, "Received exit signal");
	_exitSignaled = TRUE;
}

//////////////////////////////////////////////////////////////////////////
///HACK: Work around for firebases getting clobbered during startup by
// deployments etc. We periodically save the current map state out as a
// set of lua commands that we can replay to put things back as they
// were.

static void
_restoreMapState()
{
	char mapStateScript[1024];
	strprintf(mapStateScript, SUB_PATH("%s", "map-state.lua"), cv_luaPath.GetString());
	Lua::RunScript(mapStateScript);
}

//////////////////////////////////////////////////////////////////////////
// Run various hearbeat events.

static TeulTimeMS
_housekeeping()
{
	TeulTimeMS nextHouseKeep = teulClock64 + 25;

	threadsLogging();

	if ( !strtIsStratInitialized() )
		return nextHouseKeep;

	if ( _mustSyncChatHost && serverPeeringWithType(CHAT_HOST) == true )
	{
		strtSendChatInitialUpdates();
		_mustSyncChatHost = false;
	}

	static UINT32 nextFBRepairCycle = 0;
	if ( teulTime() >= nextFBRepairCycle )
	{
		strtProcessFBRepairQueue();
		nextFBRepairCycle = teulTime() + 5;
	}

	static TeulTimeMS nextHalfSecond = teulClock64 + 500;
	if ( teulClock64 >= nextHalfSecond )
	{
		strtProcessDBQueue();
		strtEvaluateEWSUpdates();
		nextHalfSecond = teulClock64 + 500;
		return nextHouseKeep;
	}

	static TeulTimeMS nextSecond = 0;
	if ( teulClock64 >= nextSecond )
	{
		strtProcessResupply();
		stratProcessLuaEventQueue();
		nextSecond = teulClock64 + 1000;
		return nextHouseKeep;
	}

	static UINT32 nextObjectiveUpdate = teulTime() + 5;
	if ( teulTime() >= nextObjectiveUpdate )
	{
		g_objectiveStats.calculate();
		if ( g_objectiveStats.m_cityAvailable[ALLIED] >= 1 )
			strtGameEventTrigger("gameSpareAOCapacity", ALLIED);
		if ( g_objectiveStats.m_cityAvailable[AXIS] >= 1 )
			strtGameEventTrigger("gameSpareAOCapacity", AXIS);
		nextObjectiveUpdate = teulTime() + 5;
		return nextHouseKeep;
	}

	static UINT32 nextSupplyRouteCheck = teulTime() + 1;
	static UINT32 nextForcedSupplyRouteCheck = 0;
	if ( teulTime() >= nextSupplyRouteCheck )
	{
		bool force = false;
		if ( teulTime() >= nextForcedSupplyRouteCheck )
		{
			force = true;
			nextForcedSupplyRouteCheck = teulTime() + 60;
		}
		strtBuildSupplyChains(force);
		nextSupplyRouteCheck = teulTime() + 2;
		return nextHouseKeep;
	}

	// Wait 2 minutes before starting to save the lists
	static UINT32 nextTOESave = teulTime() + 120;
	if ( teulTime() >= nextTOESave )
	{
		strtSaveTOESupplies();
		// But then save every minute
		nextTOESave = teulTime() + 60;
		return nextHouseKeep;
	}

	return (((teulClock64 + 5) / 100) + 1) * 100;
}

//////////////////////////////////////////////////////////////////////////
// Main entry point.

const char* g_serverName = "stratd";

int
main(int argc, char* argv[])
{
	Teul::SetAppName(g_serverName);

	Teul::ConfigVar::InitializeConfigVars();
	serverEnableDebugWebServer(STRAT_HOST);
	memInit();

	bool runOnce = false;

	HostLanguage = cv_stratLanguage.GetString();

	if (teulInitTransport(LOG_TO_FILE, g_serverName, _cnvyErrorFunction) < 0)
		LogAbort("Cannot initialize teulTransport");

	dbugSetWriteLevel((Debug::ERROR_TYPE)(Debug::ERR_INFO | DEF_ERR_FLAGS));
	dbugSetLogLevel((Debug::ERROR_TYPE)(DEF_ERR_LEVEL | DEF_ERR_FLAGS));
	dbugLog(_FLN, "Strat Host starting. Compiled %s at %s", __DATE__, __TIME__);

	_describeBuildConfigurationToLogFile();
	serverInitAddresses();

	threadsReadDBEnvironment();
	threadsSetup();

	//set up the signal handler
	authInitDaemon(_killConvoy);

	teulSyncClock();

	wwiiIRCInitialize(Server::localHost);
	wwiiIRCActivate(true);

	Lua::Start("stratd");

	if ( !stratStartLua() )
		exit(0);

#ifdef IS_STRAT_HOST
	if (!strtInitDB())
	{
		LogAbort("Couldn't initialize database");
	}
#endif

	StatsThread statsThread;
	statsThread.start();
	threadsEnable();

	//Initializes the convoy system
	cnvyInit();

	dbugLog(_FLN,"opening broadcast ports");
	serverOpenBroadcastIndexes();

	serverInit(HOST_VERSION
				, STRAT_HOST, STRAT_HOST
				, _serverEnterCallback
				, _serverLeaveCallback
				, _serverStatusCallback
				,_personaCreate
				,_personaRelease
				,_personaActivate
				,_personaExit
				,NULL
				,_personaErrorFunction);

	Server::myServerInfo.flags = (Server::Flags)0;//SVRFLAG_SEND_PSTAT | SVRFLAG_RELOGIN;
	Server::myServerInfo.stats.m_maxPlayers = Teul::MaxClusterUsers;

#ifdef DEBUG
	dbugLog(_FLN,"setting up direct broadcast address");
#endif

	const SINT32 mapConnection = teulOpenListenHost(Server::broadcastHost, WWII::Net::Ports::InternalStrat, _acceptMapConnection, 200000, PROTOCOL_TCP, _mapErrorFunction);
	dbugLog(_FLN,"Listening for map connections on port %d", WWII::Net::Ports::InternalStrat);

	// Give the chat host 20 seconds to start
	teulSyncClock();
	TeulTimeMS cutoff = teulClock64 + 30000;
	dbugLog(_FLI, "Waiting for chat server");
	for (; teulClock64 < cutoff && serverPeeringWithType(CHAT_HOST) == false; )
	{
		teulUpdateTransport(50);
		threadsLogging();
	}

#ifdef DEBUG
	dbugLog(_FLN,"Beginning strat processing");
#endif

#ifdef DEBUG
	dbugLog(_FLN,"Installing callbacks");
#endif

	ABORT_IF(strtHostInstallCallbacks() < 0);

	ABORT_IF(chatInstallStratSupportCallbacks() < 0);

	chatSendBroadcastAll(LSTR_SYSTEM, LSTR_STRAT_STARTING);
	for ( UINT32 i = 0; i < 10; ++i )
	{
		teulUpdateTransport(20);
		threadsLogging();
	}

	// Initialize the strat objective limits
	g_objectiveStats.calculate();

	teulSyncClock();

	strtInitializeStratSystem();

	threadsLogging();

	signal(SIGINT, _strtSIGINT);

	// Run the map-restoration hack for restarts.
	_restoreMapState();

	if ( stratStartIntermission() == false )
	{
		std::this_thread::sleep_for(std::chrono::seconds(1));
		LogAbort("bailing because intermission script failed.");
	}
	threadsLogging();

	strtBroadcastConfigUpdate();
	threadsLogging();

	Server::myServerInfo.stats.status = Server::SERVER_ACCEPTING;
	Server::myServerInfo.sync();

	extern void serverOpenLuaClientInterface(void);
	serverOpenLuaClientInterface();

	wwiiIRCChannelMessage("-sys", "[[ Syncing ]]");

	threadsLogging();

	TeulTimeMS tick100ms = 0;
	while ( !_exitSignaled )
	{
		// Process the strat update queue
		cnvyProcessStratUpdates(teulTime());

		threadsCallNextCallback();

		// Things to do every 100ms
		if ( teulClock64 >= tick100ms )
			tick100ms = _housekeeping();

		// Be a strat server for 10ms
		teulUpdateTransport(25);

		// Send the next pending sync updates
		UINT32 peers = strtSyncPeers();

		// Have we run thru once, yet?
		if ( runOnce == false )
		{
			// Don't announce anything until we've done all the syncs
			if ( peers > 0 )
				continue;

			wwiiIRCChannelMessage("-sys,#servers", "++ Running ++");
			chatSendBroadcastAll(LSTR_SYSTEM, LSTR_STRAT_READY);
			strtAllowSyncing();

			runOnce = true;
		}
	}

	teulClose(mapConnection, Teul::Closing::MapSrvClose);

	wwiiIRCChannelMessage("-sys", "[[ Shutting down normally ]]");
	chatSendBroadcastAll(LSTR_SYSTEM, LSTR_STRAT_EXITED);

	strtProcessDBQueue();
	strtSaveTOESupplies();
	std::this_thread::sleep_for(std::chrono::seconds(1));

	teulCloseTransport();

	threadsLogging();

#ifdef IS_STRAT_HOST
	strtCloseDB();
#endif
	dbugLog(_FLN,"strat system shutting down");

	Lua::Stop();

	strtShutdownStratSystem();

	statsThread.cancel();

	exit(0);
}
