#include <syscfg.h> // Path is project specific.
// Created: Tue Apr 10 13:43:09 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

#include "hostproj/hostversion.h"
#include <signal.h>  //to catch SIGHUPS with
#include "build_info.hpp"
#include "mem/mem.h"
#include "chat/chatStratSupportMessages.h"
#include "chat/chatSupportRPCMessages.h"
#include "cnvy/cnvyMain.h"
#include "include/arenaLockEnums.h"
#include "sdm/sdmConfigVars.h"
#include "server/serverCodeEnum.h"
#include "server/serverFuncTypes.h"
#include "server/server.h"
#include "server/serverIndexes.h"
#include "server/serverInitializeProcess.h"
#include "server/serverInterfaceDescription.h"
#include "server/serverPersonaExit.h"
#include "server/serverPersonaLogin.h"
#include "server/serverReleaseTypeEnum.h"
#include "server/serverServerInfo.h"
#include "server/serverCodeEnum.h"
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

#ifdef DEBUG
	#define DEF_ERR_LEVEL Debug::ERR_DBUG
    #define DEF_ERR_FLAGS LOG_ALL_TO_STDERR
#else
	#define DEF_ERR_LEVEL Debug::ERR_INFO
    #define DEF_ERR_FLAGS STD_ERR
#endif

const char* HostLanguage = "en";

static bool _exitSignaled = false;
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

static void _killConvoy(int /*signo*/)
{
	dbugLog(_FLN, "server exiting normally");

	teulCloseTransport();

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

#if defined(DEV_BUILD)
	dbugLog(_FLI, "DEV_BUILD");
#endif
#if defined(BETA_RELEASE)
	dbugLog(_FLI, " BETA_RELEASE");
#endif
#if defined(TEST_RELEASE)
	dbugLog(_FLI, " TEST_RELEASE");
#endif
#if defined(OPEN_BETA)
	dbugLog(_FLI, " OPEN_BETA");
#endif
#if defined(CLOSED_BETA)
	dbugLog(_FLI, " CLOSED_BETA");
#endif
#if defined(FRES)
	dbugLog(_FLI, " FRES");
#endif
#if defined(HAPPYOO)
	dbugLog(_FLI, " HAPPYOO");
	dbugLog(_FLI, "%s HOST FLAGS:", PN_git_commitish_string);
#endif
#if defined(TRAINING_SERVER)
	dbugLog(_FLI, " TRAINING_SERVER");
#endif
#if defined(PUBLIC_RELEASE)
	dbugLog(_FLI, " PUBLIC_RELEASE");
#endif

	dbugLog(_FLI, ".");
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

 	dbugLog(_FLN,"server %s added code %s[%d], type %d"
				, teulGetAddress(server->stats.address)
				, _serverCode[static_cast<size_t>(scode)]
				, static_cast<int>(scode)
				, static_cast<int>(server->stats.type));

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
				, teulGetAddress(server->stats.address)
				, _serverCode[static_cast<size_t>(scode)]
				, static_cast<int>(scode), static_cast<int>(server->stats.type));

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
				, _serverCode[static_cast<size_t>(scode)], static_cast<int>(scode)
				, _serverStatus[server->stats.status], static_cast<int>(server->stats.status)
				);

	//Next few lines swiped very unceremoniously from Stick
	//if they're just coming on line, bring them up to speed on the strat
	//	system
	//Removed master crap since "THERE CAN BE ONLY ONE" strat server (for
	//	right now
	if ( scode == Server::Code::Status )
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
	_exitSignaled = true;
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
	strprintf(mapStateScript, "%s/map-state.lua", cv_luaPath.GetString());
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
	static TeulTimeMS nextSecond = teulClock64+1000;
	static TeulTimeMS nextHalfSecond = teulClock64 + 500;
	static UINT32 nextObjectiveUpdate = teulTime() + 5;
	static UINT32 nextForcedSupplyRouteCheck = 0;
	static UINT32 nextTOESave = teulTime() + 120;
	static UINT32 nextSupplyRouteCheck = teulTime() + 1;

	static bool bInitiateFBRepairCycle = false;
	static bool bInitiateHalfSecProcessing = false;
	static bool bInitiateSecProcessing = false;
	static bool bInitiateObjectiveUpdateProcessing = false;
	static bool bInitiateSupplyRouteCheckProcessing = false;
	static bool bInitiateForcedSupplyRouteCheckProcessing = false;
	static bool bInitiateSaveTOESupplieskProcessing = false;

	// Upate flags that control scheduling - but make sure we don't allow processing to affect the
	// scheduled time.
	if ( teulTime() >= nextFBRepairCycle ) {
		nextFBRepairCycle += (teulTime()-nextFBRepairCycle) + 5;
		bInitiateFBRepairCycle = true;
	}

	if ( teulClock64 >= nextHalfSecond ) {
		nextHalfSecond +=  500;
		if (nextHalfSecond < teulClock64) {
			nextHalfSecond = teulClock64;
			dbugLog(_FLI, "Lost time, need to schedule next half second processing immediately");
		}
		bInitiateHalfSecProcessing = true;
	}

	if ( teulClock64 >= nextSecond ) {
		dbugLog(_FLI, "strat _housekeeping Second procesing, scheduled");
		nextSecond += 1000;
		if (nextSecond < teulClock64) {
			nextSecond = teulClock64;
			dbugLog(_FLI, "Lost time, need to schedule next second processing immediately");
		}
		bInitiateSecProcessing = true;
	}

	if ( teulTime() >= nextObjectiveUpdate )
	{
		nextObjectiveUpdate += (teulTime()-nextObjectiveUpdate) + 5;
		bInitiateObjectiveUpdateProcessing = true;
	}

	if ( teulTime() >= nextSupplyRouteCheck ) {
	
		nextSupplyRouteCheck = teulTime() + 2;
		bInitiateSupplyRouteCheckProcessing = true;
		if ( teulTime() >= nextForcedSupplyRouteCheck ) {
			nextForcedSupplyRouteCheck += (teulTime()-nextForcedSupplyRouteCheck) + 60;
			bInitiateForcedSupplyRouteCheckProcessing = true;
		}
	}

	if ( teulTime() >= nextTOESave ) {
		nextTOESave +=  60;
		bInitiateSaveTOESupplieskProcessing = true;
	}

	// Now actually act on the info above
	if (bInitiateFBRepairCycle)
	{
		strtProcessFBRepairQueue();
		bInitiateFBRepairCycle = false;
	}

	if (bInitiateHalfSecProcessing)
	{
		strtProcessDBQueue();
		strtEvaluateEWSUpdates();
		bInitiateHalfSecProcessing = false;

		if (bInitiateSecProcessing) {
			dbugLog(_FLI, "strat _housekeeping Half Second processing subverted");
		}
		return nextHouseKeep;
	}

	if (bInitiateSecProcessing)
	{
		dbugLog(_FLI, "strat _housekeeping Second procesing, activated");
		strtProcessResupply();
		stratProcessLuaEventQueue();
		bInitiateSecProcessing = false;
		return nextHouseKeep;
	}

	if (bInitiateObjectiveUpdateProcessing)
	{
		g_objectiveStats.calculate();
		if ( g_objectiveStats.m_cityAvailable[ALLIED] >= 1 )
			strtGameEventTrigger("gameSpareAOCapacity", ALLIED);
		if ( g_objectiveStats.m_cityAvailable[AXIS] >= 1 )
			strtGameEventTrigger("gameSpareAOCapacity", AXIS);
		bInitiateObjectiveUpdateProcessing = false;
		return nextHouseKeep;
	}

	if (bInitiateSupplyRouteCheckProcessing)
	{
		strtBuildSupplyChains(bInitiateForcedSupplyRouteCheckProcessing);
		bInitiateSupplyRouteCheckProcessing = false;
		bInitiateForcedSupplyRouteCheckProcessing = false;
		return nextHouseKeep;
	}

	// Wait 2 minutes before starting to save the lists
	if (bInitiateSaveTOESupplieskProcessing)
	{
		strtSaveTOESupplies();
		// But then save every minute
		bInitiateSaveTOESupplieskProcessing = false;
		return nextHouseKeep;
	}

	return (((teulClock64 + 5) / 100) + 1) * 100;
}

//////////////////////////////////////////////////////////////////////////
// Main entry point.

const char* g_serverName = "stratd";

int
main(int argc, const char* argv[])
{
	Server::InitializeProcess(g_serverName, STRAT_HOST, _cnvyErrorFunction, _killConvoy, argc, argv, {});
	dbugSetWriteLevel(DEF_ERR_LEVEL | DEF_ERR_FLAGS);
	dbugSetLogLevel(DEF_ERR_LEVEL | DEF_ERR_FLAGS);

	HostLanguage = cv_stratLanguage.GetString();

	_describeBuildConfigurationToLogFile();

	threadsSetup();

	teulSyncClock();

	wwiiIRCInitialize(Server::localHost);
	wwiiIRCActivate(true);

	Lua::Start("stratd");

	if ( !stratStartLua() )
		exit(0);

	if (!strtInitDB())
	{
		LogAbort("Couldn't initialize database");
	}

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
				,nullptr
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

	extern void serverOpenLuaClientInterface();
	serverOpenLuaClientInterface();

	wwiiIRCChannelMessage("-sys", "[[ Syncing ]]");

	threadsLogging();

	TeulTimeMS tick100ms = 0;
	bool runOnce = false;
	while (!_exitSignaled)
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

	strtCloseDB();
	dbugLog(_FLN,"strat system shutting down");

	Lua::Stop();

	strtShutdownStratSystem();

	statsThread.cancel();

	exit(0);
}
