#include <syscfg.h> // Path is project specific.
// Date: Thu Jan 18 08:39:26 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Brian "Stick" Duelm

#include "build_info.hpp"

#include <mysql.h>
#if platOS != OS_WIN32
#include <netinet/in.h>
#else
#include <winsock2.h>
#endif
#include <signal.h>

#include "mem/mem.h"
#include "chat/chatConstants.h"
#include "chat/chatGroupPermissions.h"
#include "chat/chatSupportMessages.h"
#include "chat/chatSupportRPCMessages.h"
#include "chat/chatTypes.h"
#include "comm/commTypes.h"
#include "include/arenaLockEnums.h"
#include "include/configVars.h"
#include "include/wthrTime.h"
#include "inst/instHostMessages.h"
#include "inst/instTypes.h"
#include "map/mapArenaDescription.h"		// For g_arenaDescription.
#include "map/mapBrigades.h"
#include "map/mapChecksums.h"
#include "map/mapConstants.h"
#include "map/mapData.h"
#include "map/mapDB.h"
#include "map/mapHostMessages.h"
#include "map/mapPersonaSummary.h"
#include "map/mapPlayer.h"
#include "map/mapThreads.h"
#include "miss/mission.h"
#include "miss/missionHostMessages.h"
#include "miss/missionUtils.h"
#include "mm/mmHostInstanceImpl.h"
#include "mm/mmHostMessages.h"
#include "mm/mmMain.h"
#include "mm/mmPolls.h"
#include "include/configVars.h"
#include "server/serverFlagsEnum.h"
#include "server/serverFuncTypes.h"
#include "server/server.h"
#include "server/serverIndexes.h"
#include "server/serverInterfaceDescription.h"
#include "server/serverPersonaExit.h"
#include "server/serverPersonaLogin.h"
#include "server/serverPersonaMirror.h"
#include "server/serverPersonaXfer.h"
#include "server/serverPlayerStatusNode.h"
#include "server/serverReleaseTypeEnum.h"
#include "server/serverServerInfo.h"
#include "server/serverInitializeProcess.h"
#include "strt/strtData.h"
#include "strt/strtDataErrors.h"
#include "strt/strtDB.h"
#include "strt/strtEvents.h"
#include "strt/strtIO.h"
#include "strt/strtMapMessages.h"
#include "strt/strtSyncSupportMessages.h"
#include "strt/strtTypes.h"
#include "teul/teulAuth.h"
#include "teul/teulConnectionInstruction.h"
#include "teul/teulEndpoint.h"
#include "teul/teulEndpointImpl.h"
#include "teul/teulHousekeeper.h"
#include "teul/teulKit.h"
#include "teul/teulTickSample.h"
#include "teul/teulVersion.h"
#include "weap/weapList.h"
#include "wwii/wwiiActivate.h"
#include "wwii/wwiiConfig.h"
#include "wwii/wwiiDBThreads.h"
#include "wwii/wwiiThreads.h"
#include "wwii/wwiiHostMessages.h"
#include "wwii/wwiiIRC.h"
#include "wwii/wwiiNetPorts.h"
#include "wwii/wwiiSortieManager.h"
#include "wwii/wwiiPPOLoader.h"

const char* g_serverName = "mmhostd";

PPOSpecLoader specLoader;

#define CHECKSUM_POLL_INTERVAL	300000	// Every 5 minutes

//broadcast stuff
#define		CLIENT_TIMEOUT	500000

#ifdef DEBUG
	#define DEF_ERR_LEVEL Debug::ERR_DBUG
    #define DEF_ERR_FLAGS STD_ERR  // LOG_ALL_TO_STDERR
#else
	#define DEF_ERR_LEVEL Debug::ERR_INFO
    #define DEF_ERR_FLAGS STD_ERR
#endif

extern void mapPlayerInit();

UINT32 myMaxUsers;

UINT32 hostAddress = 0;

static SINT32 _userConnections = 0;

UINT32 mmStratInternalAddress = 0;
static SINT32 _stratIndex = -1;
static bool _stratOpen = false;
static bool _stratError = false;

static SINT32 _chatClientPort = 0;

static bool _chatReady = false;

CPStruct* mapCPList;

// These are fetched from the enviroment
// which needs to set in a shell . file

static UINT32	_authHost = 0;
static UINT32	_stratHost = 0;
static UINT32 _chatClientHost = 0;

static bool _stratReady = false;
bool g_initialized = false;

SortieManager g_sortieReturns("`spawn_time` = IF(`spawn_time` IS NULL,added,NOW()),`return_time` = NOW()", "`return_time` IS NULL");

const char* HostLanguage = "en";
std::vector<const char*> script_args;

extern void mapInitializeHostSupercellData();

extern SINT32 mapCreatePlayer(SINT32 teulIndex, PlayerStatusNode* psn, const Server::PersonaXfer& xfer);
extern SINT32 mapReleasePlayer(SINT32 teulIndex, PlayerStatusNode* psn, Server::ReleaseType releaseType);
extern SINT32 mapPlayerExitCluster(PlayerStatusNode& psn);
extern void teulCloseAllEndpoints(const SINT32 errorCode);


namespace {
	UINT32 GetPublicAddress(UINT32 addr) {
		// HACK: Assumes the last octet is both the only difference
		// and remains in the same position relative to the mmhost.
		return cv_publicIP.GetUint32() + (addr - Server::myServerInfo.stats.address);
	}
}


void
mapSendActivate(SINT32 teulIndex, const playerid_t playerID, const char* callsign)
{
	WWII_ACTIVATE activate;

	activate.serverType = MAP_HOST;
	activate.hostIP = Server::myServerInfo.stats.address;
	activate.port = WWII::Net::Ports::MapHost;
	activate.teulIndex = 0;
	activate.playerID = 0;
	activate.otp = 0;
	activate.theme = host_config.arena.theme;
	
	const SINT32 activateResult = wwiiSendActivate(teulIndex, &activate);
	if ( activateResult < 0 )
	{
		dbugLog(_FLW, "t:%d p:%u c:%s wwiiSendActivate failed %d"
						, teulIndex, playerID, callsign, activateResult);
	}
	else
		dbugLog(_FLI, "t:%d p:%u c:%s sent activate", teulIndex, playerID, callsign);
}

//////////////////////////////////////////////////////////////////////
// Implement our own PersonaMirrorTo class so we can overload
// the onComplete functions per-server.

template<SERVER_TYPE _ServerType>
class PersonaMirrorTo : public Server::PersonaMirror
{
	public:	PersonaMirrorTo(const Server::PersonaXferData& _data, Server::ServerInfo* const _destination, const char* const serverType)
				: PersonaMirror(_data, _destination)
			{
				char servIP[64];
				teulGetAddress(_destination->stats.address, servIP, sizeof(servIP));
				dbugLog(_FLD, "p:%u c:%s otp:%u xfer:%" PR64BitUT " sending mirror to %s: %s"
								, dispatch().m_customerID
								, dispatch().m_callsign
								, dispatch().m_OTP
								, m_xferID
								, serverType
								, servIP
								);
			}

	public:	virtual SINT32 onComplete(SINT32 resultCode) const override;
};

//////////////////////////////////////////////////////////////////////
// We transfer player's to chat early on, and once they are connected,
// we turn on the fire hose of login spam.

template<>
SINT32
PersonaMirrorTo<CHAT_HOST>::onComplete(SINT32 resultCode) const
{
	Player* const player = PlayerInterface::Lookup(dispatch().m_customerID);
	if ( player == nullptr )
	{
		// Player has logged out.
		if ( resultCode >= 0 )
			dbugLog(_FLM, "p:%u c:%s xfer:%" PR64BitUT " player logged out during transfer to chat"
							, dispatch().m_customerID
							, dispatch().m_callsign
							, m_xferID
							);
		return -EINVAL;
	}

	dbugLog(_FLD, "p:%u c:%s xfer:%" PR64BitUT " mirror to chat complete, rc:%d"
					, dispatch().m_customerID
					, dispatch().m_callsign
					, m_xferID
					, resultCode
					);

	player->startLoginSpam(false, true);

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Every time a player spawns, we have to mirror them over to their
// specific cell host.

template<>
SINT32
PersonaMirrorTo<CELL_HOST>::onComplete(SINT32 resultCode) const
{
	dbugLog(_FLD, "p:%u c:%s xfer:%" PR64BitUT " mirrored player to cell host", dispatch().m_customerID, dispatch().m_callsign, m_xferID);
	return -EINVAL;
}

//////////////////////////////////////////////////////////////////////
// We don't send mirrors of people any place else, if we do, the
// server needs to shut down promptly for investigation.

template<SERVER_TYPE _ServerType>
SINT32
PersonaMirrorTo<_ServerType>::onComplete(SINT32 resultCode) const
{
	LogAbort("mirrored player to some other type of host!");
}

//////////////////////////////////////////////////////////////////////
// Initiate the mirroring of a player to the chat host.

bool
Player::mirrorToChat()
{
	Server::ServerInfo* const server = Server::ServerInfo::GetBest(CHAT_HOST);
	if ( server == nullptr )
	{
		dbugLog(_FLA, "No chat server for logins!");
		return false;
	}

	// Construct the packet chat is going to need.
	Chat::PlayerInfo pinfo;
	pinfo.playerID = playerID();
	pinfo.otp = OTP();
	pinfo.squadID = squadID();
	pinfo.m_permissionMask = m_playerMask;
	pinfo.m_handle = chatHandle();

	// Stupid mirror-to stuff needs a PersonaXferData :(
	Server::PersonaXferData pxd(dispatch(), sizeof(pinfo), &pinfo);

	this->log(_FLD, "mirroring to chat host %s", teulGetAddress(server->stats.address));

	// Create the xfer node
	auto mirror = new PersonaMirrorTo<CHAT_HOST>(pxd, server, "chat");

	// Do teul magic (send the mirror instruction to the player, and forewarn chat).
	server->mirrorPlayer(mirror);

	///TODO:PERSIST: This should use Teul::ConnectionInstruction instead :/
	chatSendServerInfo(teulIndex(), OTP(), _chatClientHost, _chatClientPort);

	return true;
}

bool
Player::mirrorToCellHost(Server::ServerInfo* const server)
{
	Server::PersonaXferData pxd(dispatch(), sizeof(wwiiPlayer), static_cast<wwiiPlayer*>(this));
	auto mirror = new PersonaMirrorTo<CELL_HOST>(pxd, server, "cell");
	server->mirrorPlayer(mirror);

	Teul::ConnectionInstruction connIns(Teul::ConnectionInstruction::Cell, GetPublicAddress(server->stats.address), server->stats.port, OTP());
	const SINT32 retval = connIns.send(teulIndex());

	m_lastSpawnedTo = server->stats.address;
	return ( retval >= 0 );
}

SINT32
Teul::ConnectionInstruction::send(SINT32 teulIndex)
{
	return teulSend(teulIndex, s_callback, this);
}

static SINT32
_playerActivate(SINT32 teulIndex, PlayerStatusNode* psn)
{
	Player* player = (Player*)psn->localInfo;

	if (player == nullptr)
	{
		return -Teul::Errors::LoginFail_ActivateNoPlayerInfo;
	}

	if (player->teulIndex() >= 0)
		teulSetPersonaData(player->teulIndex(), nullptr);

#ifdef NEW_FOUND_BEST_METHOD
	// Until they do the alter connect bit, don't let them in
	if ( player->m_foundBest == false )
	{
		dbugLog(_FLI, "t:%d p:%u c:%s best connection not established, skipping activation", teulIndex, player->playerID(), player->callsign());
		return 0;
	}
#endif

	// set the local persona teulIndex since we keep track of it.
	player->SetTeulIndex(teulIndex);

	if ( psn->xferState == Server::Transfer::XferReason::Login )
	{
		if ( player->teulIndex() != teulIndex )
			LogAbort("teulIndexes not synced yet: %d,%d", teulIndex, player->teulIndex());

		if ( player->mirrorToChat() == false )
			return -EINVAL;
	}
	else
	{
		dbugLog(_FLI, "t:%d:%d p:%u c:%s activating non-login client", teulIndex, player->teulIndex(), player->playerID(), player->callsign());
	}

	mapSendActivate(teulIndex, player->playerID(), player->callsign());

	if ( psn->xferState == Server::Transfer::XferReason::Login )
		player->startLoginSpam(true, false);

	player->updateChat();

	return 0;
}

static void
_acceptUserConnection(ENDPOINT& endpoint)
{
	_userConnections++;

	dbugLog(_FLN,"t:%d connection opened from %s on local port %d; connections: %d"
				, endpoint.teulIndex(), teulGetAddress(endpoint.fromAddress()), endpoint.fromPort()
				, _userConnections
				);
}

static SINT32
_playerCreate(const Server::PersonaLogin& xfer, PlayerStatusNode& psn)
{
	dbugLog(_FLD, "t:%d PERS personaCreate", psn.teulIndex);
	return mapCreatePlayer(psn.teulIndex, &psn, xfer);
}

static SINT32
_playerExit(SINT32 teulIndex, PlayerStatusNode* psn, Server::PersonaExit& xfer)
{
	dbugLog(_FLD, "t:%d PERS personaExit pl:%p %u", teulIndex, psn, xfer.exitCode);
	// NOTE: Poor choice of naming;
	// mapCreatePlayer has a clause to deal with a reason == PXR_EXIT
	return mapCreatePlayer(teulIndex, psn, xfer);
}

static void
_playerErrorFunction(SINT32 teulIndex, const UCHAR* const data, SINT32 size, const TEUL_CALLBACK* const object, SINT32 error)
{
	dbugLog(_FLA,"playerError: index %d, error %d", teulIndex, error);

	Player* player = PlayerInterface::Lookup(teulIndex);
	if ( player != nullptr)
	{
		teulClose(player->teulIndex());
		player->setDisconnected();
	}
}

static void
_stratErrorFunction(SINT32 teulIndex, const UCHAR* const data, SINT32 size, const TEUL_CALLBACK* const object, SINT32 error)
{
	if (error == ENOTCONN)
	{
		//oh crap, the strat system dropped.  free the CGM list
		dbugLog(_FLW, "t:%d strat system went away", teulIndex);
		_stratIndex = -1;
	}
	else
	{
		dbugLog(_FLA,"t:%d strat error: %d", teulIndex, error );
	}

	_stratError = true;
}

//////////////////////////////////////////////////////////////////////
// Set user capacity to zero and report why.

static void
_zeroUserCapacity(const char* const reason)
{
	if ( myMaxUsers == 0 )
		return;

	dbugLog(_FLW, "cluster capacity: %s. Setting effective capacity to ZERO (0).",  reason);

	myMaxUsers = 0;
}

//////////////////////////////////////////////////////////////////////
// Notify the auth host of our current capacity

static void
_sendAuthHostStats()
{
	static SINT32 lastTotalPlayers = 0;
	static TeulTimeMS nextLoadUpdate = 0LL;
	SINT32 totalPlayers = serverGetPlayerCount(CELL_HOST);

	// Because we redirect players to chat, players connected here
	// won't show up.
	SINT32 chatPlayers = serverGetPlayerCount(CHAT_HOST);
	if ( chatPlayers > totalPlayers )
		totalPlayers = chatPlayers;

	// Don't send packets like a crazy mofo
	if ( teulClock64 >= nextLoadUpdate || (teulClock64 + 5000 >= nextLoadUpdate && totalPlayers != lastTotalPlayers) )
	{
		authUpdateServerLoad(totalPlayers, Server::myServerInfo.stats.address, Server::myServerInfo.stats.arenaID);
		lastTotalPlayers = totalPlayers;
		nextLoadUpdate = teulClock64 + 60*1000;
	}

	static UINT32 lastCapacity = 0;
	static TeulTimeMS nextCapacityUpdate = 0LL;
	static SINT32 lastLock = AL_OFFLINE;
	myMaxUsers = serverGetClusterCapacity();
	static TeulTimeMS nextCapReport = 0;
	static UINT32 lastCapReported = 0;
	if ( teulClock64 >= nextCapReport && lastCapReported != myMaxUsers )
	{
		dbugLog(_FLN, "current capacity constraint: %u players", myMaxUsers);
		nextCapReport = teulClock64 + 60;
		lastCapReported = myMaxUsers;
	}

	if ( _stratReady == false || Server::ServerInfo::IsAvailable(STRAT_HOST) == false )
	{
		_zeroUserCapacity("No strat host available");
		host_config.arena.lock = ~0;
	}
	if ( Server::ServerInfo::IsAvailable(CHAT_HOST) == false )
		_zeroUserCapacity("No chat host available");
	else if ( Server::ServerInfo::IsAvailable(CELL_HOST) == false )
		_zeroUserCapacity("No cell hosts available");
	else if ( Server::myServerInfo.stats.status != Server::SERVER_ACCEPTING && Server::myServerInfo.stats.status != Server::SERVER_FULL )
		_zeroUserCapacity("Map process still initializing (not ready for logins)");

	const SINT32 lock = ( g_initialized && myMaxUsers > 0 ) ? (SINT32)host_config.arena.lock : (SINT32)AL_SYNCING;

	if ( lastCapacity != myMaxUsers || teulClock64 >= nextCapacityUpdate || lock != lastLock )
	{
		authSetGameCapacity(myMaxUsers, totalPlayers, Server::myServerInfo.stats.arenaID, lock);
		lastCapacity = myMaxUsers;
		lastLock = lock;
		nextCapacityUpdate = teulClock64 + 60*1000;
	}

	return;
}

static void
_authErrorFunction(SINT32 teulIndex, const UCHAR* const data, SINT32 size, const TEUL_CALLBACK* const object, SINT32 error)
{
	dbugLog(_FLA,"t:%d _authErrorFunction error %d", teulIndex, error );
	if (error == ETEUL_AUTH_RESTARTED)
	{
		dbugLog(_FLI,"%s authhost restarted, sending capacity again",__FUNCTION__);
		_sendAuthHostStats();
	}
}

static void
_errorFunction(SINT32 teulIndex, const UCHAR* const data, SINT32 size, const TEUL_CALLBACK* const object, SINT32 error_)
{
	const UINT32 error = (UINT32)abs(error_);
	if ( error == ENOTCONN )
	{
		Player* player = PlayerInterface::Lookup(teulIndex);
		if ( player != nullptr)
		{
			--_userConnections;
			dbugLog(_FLI, "t:%d p:%u c:%s connection closed %d connections remaining"
							, teulIndex, player->playerID(), player->callsign(), _userConnections);
			player->Release(teulIndex, Server::ReleaseType::EConn);
			player->setDisconnected();
		}
		else
		{
			dbugLog(_FLD, "t:%d non-player connection closed %d connections remaining", teulIndex, _userConnections);
		}
	}
	else if ( error == ETEUL_ILLEGAL_TICKS )
	{
		auto* sample = (const Teul::TickSample*)(data);
		Player* player = PlayerInterface::Lookup(teulIndex);
		if ( player != nullptr )
		{
			dbugLog(_FLD, "t:%d p:%u c:%s clock drift host:%" PR64BitUT ", client:%" PR64BitUT ", delta:%" PR64BitT ", ratio:%.6g", player->teulIndex(), player->playerID(), player->callsign(), sample->host_ms, sample->client_ms, sample->delta, sample->ratio);
			chatSendTickSample(MAP_HOST, player->playerID(), data);
		}
		else
			dbugLog(_FLW, "t:%d clock drift host:%" PR64BitUT ", client:%" PR64BitUT ", delta:%" PR64BitT ", ratio:%.6g", teulIndex, sample->host_ms, sample->client_ms, sample->delta, sample->ratio);
	}
	else
	{
		dbugLog(_FLW, "t:%d teul Error %d", teulIndex, error);
	}
}

static void
_stratConnectionOpen(ENDPOINT& endpoint)
{
	dbugLog(_FLN,"t:%d strat connection opened %s", endpoint.teulIndex(), teulGetAddress(endpoint.fromAddress()));
	_stratOpen = true;
}

static void
_connectToStrat()
{
	_stratIndex = -1;
	_stratHost = 0;
	_stratError = false;

	dbugLog(_FLN,"waiting for strat server");

	//Get the address
	while ( !_stratReady || _stratHost == 0 )
	{
		teulUpdateTransport(25);
		if ( _stratReady )
		{
			const Server::ServerInfo* const server = Server::ServerInfo::GetBest(STRAT_HOST, true);
			if (server != nullptr)
			{
				_stratHost = Server::BroadcastMaskAddress(server->stats.address);
			}
		}
	}

	dbugLog(_FLN,"strat server found at %s",teulGetAddress(_stratHost));

	_stratIndex = teulOpenSend(_stratHost, WWII::Net::Ports::InternalStrat, _stratConnectionOpen, 20000, PROTOCOL_TCP, _stratErrorFunction);
	if ( _stratIndex < 0 )
		LogAbort("open to strat host failed: %d", _stratIndex);

	while ( !_stratOpen && !_stratError )
	{
		teulUpdateTransport(10);
	}

	if ( _stratError )
		LogAbort("unable to open strat connection, got error %d", _stratError);

	dbugLog(_FLN, "t:%d opened strat connection", _stratIndex);
}

//////////////////////////////////////////////////////////////////////
// Check to see who is master of the chat host

static bool
_checkChatMaster()
{
	Server::ServerInfo* server = Server::ServerInfo::GetBest(CHAT_HOST, true);
	if ( server == nullptr )
	{
		_chatReady = false;
		return false;
	}

	UINT32 chat_addr = GetPublicAddress(server->stats.address);
	if ( _chatClientHost != chat_addr )
	{
		_chatClientHost = chat_addr;
		_chatClientPort = server->stats.port;

		dbugLog(_FLN,"Chat host master at %s", teulGetAddress(_chatClientHost));
	}

	if ( !_chatReady )
	{
		_chatReady = true;
		missAnnounceMissionsToChat();
	}

	return true;
}

//////////////////////////////////////////////////////////////////////
// Busy-spin loop waiting for a chat host to come along

static void
_connectToChat()
{
	dbugLog(_FLN,"Waiting for chat host");
	//Spin merrily along here until we find a chat host
	while ( serverPeeringWithType(CHAT_HOST) == false )
	{
		if ( _checkChatMaster() )
			return;

		teulUpdateTransport(25);
	}
}

//////////////////////////////////////////////////////////////////////
// Called by auth to allow us to decide if we want to accept a
// player's login.

static SINT32
_acceptLogin(const SINT32 teulIndex, Auth::Dispatch* const dispatch)
{
	const ENDPOINT* const endpoint = teulGetEndpoint(teulIndex);
	if ( endpoint == nullptr )
	{
		dbugLog(_FLW, "t:%d p:%u c:%s otp:%u LOGIN DENIED: cannot look up teulIndex"
						, teulIndex, dispatch->m_customerID, dispatch->m_callsign, dispatch->m_OTP);
		return -Teul::Errors::LoginFail_AcceptBadIndex;
	}
	if ( endpoint->m_impl->m_cryptVersion < Teul::TeulV2 )
	{
		dbugLog(_FLW, "t:%d p:%u c:%s otp:%u LOGIN DENIED: client crypt version %u < required %u"
						, teulIndex, dispatch->m_customerID, dispatch->m_callsign, dispatch->m_OTP
						, endpoint->m_impl->m_cryptVersion, Teul::TeulV2);
		return -Teul::Errors::LoginFail_AcceptBadTeul;
	}

	dbugLog(_FLD, "t:%u p:%u c:%s otp:%u mapAcceptLogin"
					, teulIndex, dispatch->m_customerID, dispatch->m_callsign, dispatch->m_OTP);

	return 0;
}

#if defined(SIGUSR1)
void
_handleSIGUSR1(SINT32 k)
{
	// force ignore of signal while we process
	// -- gets rid of a race condition --
	signal(SIGUSR1, SIG_IGN);

	//Reconnect to the authHost
	SINT32 retval = authGameInit(Server::localHost, cv_publicIP.GetUint32(), WWII::Net::Ports::MapHost,
	                             _authHost, WWII::Net::Ports::InternalAuth, cv_appID.GetUint32(),
	                             PN_build_version_value, PN_build_version_value,
	                             Teul::MaxClusterUsers, _acceptLogin, _authErrorFunction);
	if ( retval < 0)
		LogAbort("%s authGameInit failed retval %d", __FUNCTION__, retval);

	signal(SIGUSR1, _handleSIGUSR1);
}
#endif

//////////////////////////////////////////////////////////////////////

SINT32
mapGetStratIndex()
{
	return _stratIndex;
}

//////////////////////////////////////////////////////////////////////

void
_installCallbacks()
{
	ABORT_IF(Map::HostInstallCallbacks() < 0);
	ABORT_IF(missionHostInstallCallbacks() < 0);
	ABORT_IF(mmHostInstallCallbacks() < 0);
	ABORT_IF(strtInstallMapCallbacks() < 0);
	ABORT_IF(strtSyncSupportInstallCallbacks() < 0);
	ABORT_IF(chatInstallSupportCallbacks() < 0);
	ABORT_IF(wwiiHostInstallCallbacks() < 0);
	ABORT_IF(Instance::InstallHostCallbacks() < 0);
}

//////////////////////////////////////////////////////////////////////

static void
_checkStratMaster()
{
	// Update the strat host master
	Server::ServerInfo* server = Server::ServerInfo::GetBest(STRAT_HOST, true);
	if (server != nullptr)
	{
		_stratHost = server->stats.address;
	}
}

//////////////////////////////////////////////////////////////////////

static void
_updateMasterInfo()
{
	// Check to see if the chat host master has changed
	_checkChatMaster();
	_checkStratMaster();
}

//////////////////////////////////////////////////////////////////////

static SINT32
_serverEnterCallback(Server::ServerInfo* server, Server::Code scode)
{
#ifdef BETA_RELEASE
	dbugLog(_FLN,"%s server %s added code %d"
				, g_serverTypes[server->stats.type], teulGetAddress(server->stats.address)
				, static_cast<int>(scode));
#endif

	if ( server->stats.type == STRAT_HOST && server->stats.status == Server::SERVER_ACCEPTING )
	{
		dbugLog(_FLI, "strat server is accepting");
		_stratReady = true;
	}

	_sendAuthHostStats();

	_updateMasterInfo();
	if ( server->stats.type == CHAT_HOST )
	{
		missCancelAllMobileSpawns();
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////

static SINT32
_serverLeaveCallback(Server::ServerInfo* server, Server::Code scode)
{
#ifdef BETA_RELEASE
	dbugLog(_FLN,"%s server %s left code %d", g_serverTypes[server->stats.type], teulGetAddress(server->stats.address)
				, static_cast<int>(scode));
#endif

	_sendAuthHostStats();

	if (server->stats.type == STRAT_HOST)
	{
		dbugLog(_FLN,"lost strat connection, resetting server");

		//Kill all the computer generated missions
		//	missionBlowAwayComputerMissions();

		_stratReady = false;
		host_config.arena.lock = ~0;

		chatSendBroadcastAll(LSTR_WARNING, LSTR_GENERIC, "[X] Server Locked: Strat system unavailable. [X]");
	}

	_updateMasterInfo();

	if ( server->stats.type == CHAT_HOST )
	{
		missCancelAllMobileSpawns();
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////

static SINT32
_serverStatusCallback(Server::ServerInfo* server, Server::Code scode)
{
#ifdef BETA_RELEASE
	dbugLog(_FLN,"server status changed for %s, code is %d status is %d", teulGetAddress(server->stats.address)
				, static_cast<int>(scode), static_cast<int>(server->stats.status));
#endif

	if ( server->stats.type == STRAT_HOST )
	{
		if ( server->stats.status == Server::SERVER_ACCEPTING )
		{
			if ( _stratReady == false )
				dbugLog(_FLI, "strat server is now accepting");
			_stratReady = true;
		}
		else
			_stratReady = false;
	}

	_sendAuthHostStats();

	_updateMasterInfo();

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Check to see if the server needs to return to accepting status

static void
_checkAcceptingStatus()
{
	bool turnAcceptingOn = false;
	bool updateCapacity = false;

	if ( (UINT32)Server::myServerInfo.stats.maxPlayers() != myMaxUsers )
	{
		Server::myServerInfo.stats.m_maxPlayers = myMaxUsers;
		updateCapacity = true;
	}

	switch (Server::myServerInfo.stats.status)
	{
		case Server::SERVER_FULL:
			if ( Server::myServerInfo.stats.availPlayers() <= 0 )
			{
				dbugLog(_FLN, "server is no longer full, on:%d max %d, NOW SERVER_ACCEPTING"
							, Server::myServerInfo.stats.numPlayers(), Server::myServerInfo.stats.maxPlayers());
				turnAcceptingOn = true;
			}
		break;

		case Server::SERVER_INITIALIZING:
		{
			static TeulTimeMS acceptAfterTime = teulClock64 + 15000;
			if ( teulClock64 >= acceptAfterTime )
			{
				//we've been in the initializing state for more than 30 secs without receiving
				//an update, so we're ready to start accepting
				dbugLog(_FLN, "%s() NOW SERVER_ACCEPTING",__FUNCTION__);
				turnAcceptingOn = true;
			}
		}
		break;
		default:
		break;
	}

	// Exit current state and turn ACCEPTING on.
	if ( turnAcceptingOn )
	{
		Server::myServerInfo.stats.status = Server::SERVER_ACCEPTING;
		updateCapacity = true;
		wwiiIRCChannelMessage("-sys,#servers", "++ Running ++");

		std::string acceptingMessage = "\n\n==================================================================================\n";
		acceptingMessage = acceptingMessage += "   ++ Now Accepting logins from Auth server: ";
		acceptingMessage += g_arenaDescription["host_rev"];
		acceptingMessage += " ++\n==================================================================================";
		wwiiIRCChannelMessage("-sys,#servers", acceptingMessage.c_str());
		printf("%s\n\n\n", acceptingMessage.c_str());

	}

	if ( updateCapacity )
	{
		Server::myServerInfo.sync();
	}
}

//////////////////////////////////////////////////////////////////////
// Check the HCUnits list for brigades which have updated mission lists

static void
_checkForUpdatedMissionLists()
{
	BRIGADES::iterator it;
	for ( int c = 1; c < MAX_COUNTRIES; ++c )
	{
		for ( int b = 1; b < MAX_BRANCHES; ++b )
		{
			for ( it = g_brigades[c][b].begin(); it != g_brigades[c][b].end(); ++it )
				(*it)->sendMissionUpdates();
		}
	}
}

//////////////////////////////////////////////////////////////////////
// Ping the host status table to indicate this server is running.

static void
_updateHostStatusTable()
{
	// Once a minute, update our active status in the database.
	static UINT32 nextSave = 0;
	if ( teulTime() < nextSave )
	{
		DBThread::Do(DBThread::Default, "REPLACE INTO wwii_host_status (name,ping) VALUES ('mmhost',now())");
		nextSave = teulTime() + 60;
	}
}

//////////////////////////////////////////////////////////////////////
// Report if the server is full

static void
_fullCheck()
{
	if ( Server::myServerInfo.stats.status == Server::SERVER_FULL )
	{
		static UINT32 nextBlip = 0ULL;
		if ( teulTime() < nextBlip )
			return;
		dbugLog(_FLW, "mmhost is over capacity on: %d max: %d, refusing new connections!"
					, Server::myServerInfo.stats.numPlayers(), Server::myServerInfo.stats.maxPlayers());
		nextBlip = teulTime() + 15;
	}
}

//////////////////////////////////////////////////////////////////////
// Save the sortie returns list

static void
_commitSorties()
{
	g_sortieReturns.Commit();
}

//////////////////////////////////////////////////////////////////////
// Fetch opinion polls

static void
_fetchOpinionPolls()
{
	mmFetchOpinionPolls(false);
}

//////////////////////////////////////////////////////////////////////
// Poll Checksums

#if !defined(DEV_BUILD)
static void
_pollChecksums()
{
	mapPollChecksums(false);
}
#endif

//////////////////////////////////////////////////////////////////////
// Iterate thru house keeping steps in a hosue keeping table

class Housekeeper
{
public:
	using FUNCTION = void(*)();

private:
	FUNCTION*		m_functions;
	UINT32			m_state;

public:
	Housekeeper(FUNCTION* functions)
		: m_functions(functions)
		, m_state(0)
		{
			assert( nullptr != m_functions );
		}

	bool poll()
	{
		m_functions[m_state++]();

		if ( nullptr == m_functions[m_state] )
		{
			m_state = 0;
			return false;
		}

		return true;
	}
};

static void
_logWWIIPlayerStats()
{
	static TeulTimeMS nextBlip = 0ULL;
	if ( teulClock64 < nextBlip )
		return;
	wwiiPlayer::logStats();
	nextBlip = teulClock64 + 60 * 1000;
}

//////////////////////////////////////////////////////////////////////
// Periodic "housekeeping" routines that are run every few seconds.

extern void mapExpireSorties();

static bool
_housekeeping()
{
	static Housekeeper::FUNCTION functions[] =
	{
		_fullCheck
	,	_commitSorties
	,	checkForTimedOutMissions
	,	_sendAuthHostStats
	,	_updateHostStatusTable
	,	mapCheckSideCounts
	,	_fetchOpinionPolls
#if !defined(DEV_BUILD)
	,	_pollChecksums
#endif
	,	Instance::RecordPopulations
	,	_logWWIIPlayerStats
	,	[](){ chatHousekeepGroupPermissions(); }
	,	mapExpireSorties
	,	mapRequestBrigadeDetailsUpdate
	,	_checkForUpdatedMissionLists
	,	Map::BroadcastSwingMeter
	, nullptr
	};
	static Housekeeper housekeeper(functions);

	return housekeeper.poll();
}

//////////////////////////////////////////////////////////////////////
// Free up memory, general clean up.
// "But unix will do that for us" - true, but that won't help you
// when you're looking for a memory leak will it, young padawan?

static void
_shutdown(SINT32 k)
{
	dbugLog(_FLA, "SIGINT: shutting down");
#if defined(SIGHUP)
	if ( g_initialized && k == SIGHUP )
	{
		teulCloseAllEndpoints(ETEUL_SERVER_SHUTDOWN);
	}
#endif
	wwiiIRCTerminate("[[ Shutdown cleanly ]]");
	strtReleaseSupercells();

	mapThreadsShutdown();

	mapCloseDB();
	teulCloseTransport();

	exit(0);
}

//////////////////////////////////////////////////////////////////////
// Map/Mission server main entry point
// The Map/Mission server is, effectively, the lobby server.
// The "map" part of it could quite easily be handled by the
// stratUpdater(s) instead, but according to legend the original
// idea was to save the number of network connections involved.
// [Why? Because network connections lead to interrupt time]

int
main(int argc, const char* argv[])
{
	Server::InitializeProcess(g_serverName, MAP_HOST, _errorFunction, nullptr, argc, argv, {
		{ "-teul", "Dump information about teulKit", false
			, [](const char*) {
				printf("sizeof(E) = %" PRSizeT ", sizeof(EI) = %" PRSizeT ", sizeof(wwiiPlayer) = %" PRSizeT ", sizeof(Player) = %" PRSizeT "\n", sizeof(ENDPOINT), sizeof(EndpointImpl), sizeof(wwiiPlayer), sizeof(Player));
				exit(0);
			}
			, nullptr
		}
	});
    dbugSetLogLevel(DEF_ERR_LEVEL | DEF_ERR_FLAGS);
    dbugSetWriteLevel(DEF_ERR_LEVEL | DEF_ERR_FLAGS);

	ABORT_IF(cv_appID.GetUint32() == 0 || cv_appID.GetUint32() > 255);

//	EF_ALLOW_MALLOC_0 = 1;
//	EF_PROTECT_FREE = 1;

	// Tell the instancing system how to handle
	Instance::RegisterHostInstanceImplFactory(Instance::MMHostInstanceImpl::Factory);

	// Utterly lock the arena.
	host_config.arena.lock = ~0;

	mapThreadsSetup();

	HostLanguage = cv_stratLanguage.GetString();

	dbugLog(_FLN, "Map/Mission Host starting. Compiled %s at %s. AppID is %u", __DATE__, __TIME__, cv_appID.GetUint32());

	_authHost = serverGetAuthIP();
	myMaxUsers = serverGetMaxClusterUsers();

	specLoader.loadSpecMap();

	wwiiIRCInitialize(Server::localHost, false);
	wwiiIRCActivate(false);

	// Set up signal handlers
#if defined(SIGINT)
	signal(SIGINT, _shutdown);
#endif
#if defined(SIGUSR1)
	signal(SIGUSR1, _handleSIGUSR1);
#endif

	teulSyncClock();

	mapPlayerInit();

	dbugLog(_FLI, "Initializing auth");
	SINT32 retval = -1;

	// Assume that auth is not up when the cluster is started (useful in development environment)
	while ( retval < 0 )
	{
		retval = authGameInit(
					Server::localHost
					, cv_publicIP.GetUint32()
					, WWII::Net::Ports::MapHost
					, _authHost
					, WWII::Net::Ports::InternalAuth
					, cv_appID.GetUint32()
					, PN_build_version_value
					, PN_build_version_value
					, Teul::MaxClusterUsers
					, _acceptLogin
					, _authErrorFunction
				);
		if ( retval < 0 )
		{
			// keep trying until we connect - at intervals
			dbugLog(_FLA, "%s authGameInit failed retval %d", __FUNCTION__, retval);
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	}

	authSetGameCapacity(0, 0, Server::myServerInfo.stats.arenaID, AL_INITIALIZING);

	dbugLog(_FLI, "auth initialized");

	//set things up
	missInit();

	//initalize time system
	wthrInitTime();

	if (!(mapInitDB() && strtInitDB()))
		LogAbort("Couldn't initialize database!");

	mapCPList = strtImportStratData();
	strtReportDataErrors(0);
	if (!mapCPList)
		LogAbort("Unable to initialize Strat System");

	dbugLog(_FLN,"loaded strat data");

	//broadcast stuff
	serverOpenBroadcastIndexes();

	Server::myServerInfo.stats.port = WWII::Net::Ports::MapHost;

	serverInit(PN_build_version_value
				, MAP_HOST, AUTH_HOST
				, _serverEnterCallback
				, _serverLeaveCallback
				, _serverStatusCallback
				,_playerCreate
				, mapReleasePlayer
				,_playerActivate
				,_playerExit
				, mapPlayerExitCluster
				,_playerErrorFunction);

	Server::myServerInfo.stats.m_maxPlayers = (SINT32)std::min((UINT32)host_config.arena.maxPlayers, (UINT32)8);
	Server::myServerInfo.flags = (Server::Flags)(Server::SVRFLAG_SEND_PSTAT | Server::SVRFLAG_RELOGIN);

	_installCallbacks();

	_connectToChat();
	_connectToStrat();

	dbugLog(_FLN,"listening for clients on TCP port %d",WWII::Net::Ports::MapHost);

	mapInitMapData();

	missionGetInstanceMission(Instance::_CAMPAIGN, WWII_ENGLAND, BRANCH_LAND);
	missionGetInstanceMission(Instance::_CAMPAIGN, WWII_FRANCE, BRANCH_LAND);
	missionGetInstanceMission(Instance::_CAMPAIGN, WWII_GERMANY, BRANCH_LAND);

	// Initialize the "big packet of vis level bits"
	mapInitializeHostSupercellData();

	mapThreadsEnable();

	dbugLog(_FLI, "connecting to authhost @ %s", teulGetAddress(_authHost));
	//register our ip and the port we're listening on

	teulUpdateTransport(50);

	Server::myServerInfo.stats.status = Server::SERVER_INITIALIZING;  //ready to receive missions
	Server::myServerInfo.sync();

	mapLoadWeaponIndexes();

	//set up the client port, where the FE will connect to on all ips for this interface
	if ( !teulOpenListenInterface(WWII::Net::Ports::MapHost, _acceptUserConnection, 45000, _errorFunction) )
		LogAbort("Error allocating endpoint!");

	authSetGameCapacity(0, 0, Server::myServerInfo.stats.arenaID, AL_SYNCING);
	wwiiIRCChannelMessage("-sys", "[[ Syncing ]]");

	dbugLog(_FLI, "entering update loop");

	g_initialized = true;

	TeulTimeMS nextHousekeeping = teulClock64 + 1000;
	Teul::housekeeper cpEventsHousekeeper(Strat::Events::Publish, 100);

	while (1)
	{
		teulUpdateTransport(50);

		// Make sure we're talking to the strat host
		if ( _stratIndex == -1 )
		{
			_connectToStrat();
			continue;
		}

		// Return to "accepting" status if necessary
		_checkAcceptingStatus();

		threadsCallNextCallback();

		static Housekeeper::FUNCTION functions[] =
		{
			mapSendChatUpdates
		,	mapProcessLogins
		,	mapSendHCUpdates
		,	nullptr
		};
		static Housekeeper housekeeper(functions);
		housekeeper.poll();

		// Send strat updates.
		cpEventsHousekeeper.poll();

		if ( teulClock64 >= nextHousekeeping )
		{
			// keep DB connection alive
			if (mysql_ping(dbh) != 0) {
				dbugLog(_FLA, "dbc:Main thread server connection failure - %d(%s)", mysql_errno(dbh), mysql_error(dbh));
				threadsDBReconnect(dbh);
			}       

			if ( _housekeeping() == false )
				nextHousekeeping = teulClock64 + 5000;
		}
	}

	// if we make the loop above break
//	_shutdown(0);
}

//////////////////////////////////////////////////////////////////////
// Force disconnection of a player (kick/smite)

void
mapSmiteCustomer(const playerid_t playerID)
{
	Player* player = PlayerInterface::Lookup(playerID);
	if ( player == nullptr )
		return;

	dbugLog(_FLN, "t:%d p:%u c:%s smiting", player->teulIndex(), player->playerID(), player->callsign());
	player->Release(player->teulIndex(), Server::ReleaseType::Exit);
}

//////////////////////////////////////////////////////////////////////
// Send a callback to the current chat host

SINT32
mmSendToChat(TEUL_CALLBACK* callback, void* data)
{
	if ( serverSendCallbackToType(CHAT_HOST, callback, data) == false )
		return -EINVAL;
	return 0;
}

