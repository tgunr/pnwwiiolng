#include <syscfg.h> // Path is project specific.
// Created: Mon Jul 02 18:32:26 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Marty "Vandal" Poulin

#include <queue>
#include <atomic>

#include "versionNo.h"

#include "server/serverFuncTypes.h"
#include "server/server.h"
#include "server/serverBase.h"
#include "server/serverConfigVars.h"
#include "server/serverIndexes.h"
#include "server/serverInterfaceDescription.h"
#include "server/serverInterface.h"
#include "server/serverMessages.h"
#include "server/serverPeers.h"
#include "server/serverPersonaExit.h"
#include "server/serverPersonaLogin.h"
#include "server/serverPersonaMirror.h"
#include "server/serverPersonaMirrorReply.h"
#include "server/serverPlayerStatusNode.h"
#include "server/serverPlayerStatusUpdate.h"
#include "server/serverServerInfo.h"
#include "server/serverTypes.h"
#include "teul/teulAuth.h"
#include "teul/teulAuthServer.h"
#include "teul/teulConfigVar.h"
#include "teul/teulConnectionInstruction.h"
#include "teul/teulConstants.h"
#include "teul/teulEndpoint.h"
#include "teul/teulMicroWebServer.h"
#include "teul/teulPacket.h"
#include "teul/teulPacketHeader.h"
#include "teul/teulServer.h"
#include "wwii/wwiiConfig.h"
#include "wwii/wwiiNetPorts.h"

// the time that a player may be on the exit server holder
// bofore he is sent back to the auth Host
static const UINT32 PLAYER_TIMEOUT = 3 * 60 * 1000;	// 4 minutes
static const SINT32 PLAYER_XFER_TIMEOUT = 30 * 1000;	// 30 seconds

CVAR_UINT(maxClusterUsers, "0", "MM_MAX_USERS"
		  , "Maxmimum number of players on the mmhost/cluster");
CVAR_ADDR(authHost, "auth.playnet.com", "AUTH_ADDRESS", true
			, "Host name or IP Address of the authentication server.");

extern void serverInitializePeering();
extern void serverConnectPeer(const UINT32 ip, const UINT32 type);
extern void serverDisconnectPeer(const UINT32 ip);

class ServerManager : public ServerInterface
{
public:
	////////////////////////////////////////////////////////////
	// Management functions -- intended for internal use of
	//
	static void SetPlayerTeulIndex(PlayerStatusNode* psn, SINT32 index);
	static void RegisterPlayer(PlayerStatusNode* psn);
	static void UnregisterPlayer(PlayerStatusNode* psn);
};

namespace Server
{
	InterfaceDescription myInterface;
	ServerInfo myServerInfo;
	static const TeulTimeMS CheckInterval_ms = 30000ULL;
	static const TeulTimeMS HeartbeatInterval_ms = 15000ULL;

	UINT32	broadcastNet;			// Base address of the broadcast network

	SINT32	broadcastIndex = -1;	// TeulIndex for broadcast (UDB) packets.
	SINT32	broadcastRxIndex = - 1;	// Where we receive broadcasts.

	UINT32	localHost;				// IP addr of local host
	UINT32	broadcastHost;			// IP address of the broadcast address.

	bool	broadcastConnected = false;
}

ERROR_FUNCTION* 	extErrorFn;

typedef std::deque<Server::Transfer*> TransferQueue;
static TransferQueue s_transferQueue;

static Server::ServerInfo* _exitServer = NULL;

static CREATE_PERSONA_FUNCTION* _createPersonaFn;
static RELEASE_PERSONA_FUNCTION* _releasePersonaFn;
static ACTIVATE_PLAYER_FUNCTION* _activatePlayerFn;
static EXIT_PERSONA_FUNCTION* _exitPersonaFn;
static EXIT_CLUSTER_FUNCTION* _exitClusterFn;

typedef std::map<UINT32, Server::ServerInfo*> SERVERS_BY_IP;
static SERVERS_BY_IP serversByIP;
typedef std::map<UINT64, Server::ServerInfo*> SERVERS_BY_ID;
static SERVERS_BY_ID serversByID;

PLAYERID_HASH ServerInterface::s_byPlayerID;
PLAYERIDX_MAP ServerInterface::s_byTeulIndex;

static Server::ServerInfo* _myServerInfo = NULL;
static TeulTimeMS _nextHeartbeat = 0ULL, _nextUpdate;

static SERVER_UPDATE* _enterCallback= NULL;
static SERVER_UPDATE* _leaveCallback = NULL;
static SERVER_UPDATE* _statusCallback= NULL;

static std::set<Server::ServerInfo*> s_activeServerList;

static SINT32 _playerDelete(PlayerStatusNode* const psn, Server::ReleaseType releaseType);

// move players from one server list to another and update state
extern ERROR_FUNCTION* 	extErrorFn;
static ENDPOINT* s_broadcastEndpoint = NULL;

#if defined(SERVER_AS_ENTRY_POINT)
UINT32 g_portOffset = 0;
extern const char* const g_serverName;
#endif

ServerBase* s_serverBase = nullptr;

//////////////////////////////////////////////////////////////////////
// Private functions.

static bool
_playerAddAuth(PlayerStatusNode* const psn)
{
	if (psn->inAuth == FALSE)
	{
		if ( authRecordDispatch(&(psn->dispatch()), false)  )
		{
			psn->inAuth = TRUE;
			return true;
		}
		else
			return false;
	}
	else
		return true;
}

static PlayerStatusNode*
_getPlayerWith(SINT32 teulIndex, UINT32 id)
{
	if (id != 0)
	{
		PlayerStatusNode* psn = ServerInterface::PlayerStatusNodeByPlayerID(id);
		if ( psn != NULL )
			return psn;
	}

	if ( teulIndex < 0 )
		return NULL;

	return ServerInterface::PlayerStatusNodeByTeulIndex(teulIndex);
}

SINT32
_playerExitTo(Server::ServerInfo* server, PlayerStatusNode* psn, Server::Transfer::XferReason /* reason */, UINT32 exitCode)
{
	if (server == NULL)
	{
		if(psn != NULL)
			dbugLog(_FLA, "p:%u cant send _playerExitTo, NULL server"
						, psn->dispatch().m_customerID);
		else
			dbugLog(_FLA, "cant send _playerExitTo for NULL everything!!");

		return -EINVAL;
	}

	if (psn == NULL)
	{
		dbugLog(_FLA, "cant send NULL _playerExitTo to server %s"
					, teulGetAddress(server->stats.address));
		return -EINVAL;
	}

	dbugLog(_FLI, "p:%u sending _playerExitTo to server %s exit code %u"
				, psn->dispatch().m_customerID
				, teulGetAddress(server->stats.address), exitCode);

	Server::PersonaExit xfer(exitCode, psn->data);

	if (_exitPersonaFn)
		(_exitPersonaFn)(psn->teulIndex, psn, xfer);

	return teulSendTo(Server::broadcastIndex, server->stats.address & DIRECT_MASK, 0, PERSONA_EXIT_CB, &xfer);
}

static void
_playerReleasePersona(SINT32 teulIndex, PlayerStatusNode* psn, UINT32 exitCode, Server::ReleaseType releaseType)
{
	Server::ServerInfo* destination = NULL;

	if ( psn->localInfo != NULL && _releasePersonaFn )
	{

		if ((Server::myServerInfo.flags & Server::SVRFLAG_SEND_PSTAT)
			&& releaseType != Server::RELEASE_REDIRECTED
			&& Server::myServerInfo.exitDestType != 0 )
		{
			destination = Server::ServerInfo::GetBest(Server::myServerInfo.exitDestType);
			_playerExitTo(destination, psn, Server::Transfer::XferReason::Exit, exitCode);
		}

		(_releasePersonaFn)(teulIndex, psn, releaseType);

		psn->localInfo = NULL;
	}

	if ( psn->teulIndex >= 0 )
	{
		// We're about to clear this field, so remember it
		SINT32 wasIndex = psn->teulIndex;
		ServerManager::SetPlayerTeulIndex(psn, -1);
		///TODO: What if its not open? Check for ENOTCONN?
		teulClose(wasIndex, Teul::Closing::PlayerRelease);
	}
}

static void
_playerDeleteAuth(PlayerStatusNode* psn)
{
	if (psn->inAuth)
	{
		authDeleteDispatch(&psn->dispatch());
		psn->inAuth = FALSE;
	}
}

static void
_sendHeartBeat(void)
{
	if ( _myServerInfo == NULL || Server::broadcastIndex < 0 )
		return;

	Server::myServerInfo.availPlayers = Server::myServerInfo.stats.availPlayers();

#ifdef HEAVY_DEBUG
	dbugLog(_FLD, "t:%d sending heartbeat at %llu", Server::broadcastIndex, teulClock64);
#endif

	teulSend(Server::broadcastIndex, SERVER_HEARTBEAT_CB, &(Server::myServerInfo.stats));
	_nextHeartbeat = teulClock64 + Server::HeartbeatInterval_ms;
}

static PlayerStatusNode*
_extractPlayerFromServer(Server::ServerInfo* const server, PlayerStatusNode* const psn)
{
	if (server == NULL || psn == NULL)
	{
		dbugLog(_FLA, "cant extract player %p from server %p", psn, server);
		return NULL;
	}

	dbugLog(_FLD, "p:%d c:%s extracting player %p server %p ip %s id %Ld: "
				, psn->dispatch().m_customerID
				, psn->dispatch().m_callsign
				, psn
				, server
				, teulGetAddress(server->stats.address)
				, server->stats.id
				);

	server->m_players.erase(psn);

	if ( server->stats.address == Server::myServerInfo.stats.address )
		server->stats.removePlayer();

	return psn;
}

//////////////////////////////////////////////////////////////////////
// Construct a PlayerStatusUpdate from a PlayerStatusNode

static UINT32 _updateId = 0;
Server::PlayerStatusUpdate::PlayerStatusUpdate(const PlayerStatusNode& psn)
	: serverID(psn.curServer.id)
	, status(psn.status)
	, dispatch(psn.dispatch())
	, source(Server::myServerInfo.stats.address)
	, id(_updateId++)
	{}

//////////////////////////////////////////////////////////////////////
// Destructor for a Server::ServerInfo

Server::ServerInfo::~ServerInfo()
{
	this->stats.status = Server::SERVER_LOGIN;

	if ( this->localInfo != NULL )
	{
		memFree(this->localInfo);
		this->localInfo = NULL;
	}

	if ( !s_activeServerList.empty() )
		s_activeServerList.erase(this);
}

//////////////////////////////////////////////////////////////////////
// Send a PlayerStatusUpdate to the auth host for a given player.

static void
_sendPlayerStatusUpdate(const PlayerStatusNode& psn)
{
	if ( s_broadcastEndpoint == NULL )
		return;

	Teul::Packet outgoing(sizeof(PlayerStatusNode), Teul::Serv::CallbackX, PLAYER_UPDATE_Y);
	outgoing.setUnencrypted();
	Server::PlayerStatusUpdate* const update = CONSTRUCT_IN_PACKET(outgoing, Server::PlayerStatusUpdate, psn);

	dbugLog(_FLD, "p:%d c:%s sending update %d status %d server %Ld"
					, update->dispatch.m_customerID
					, update->dispatch.m_callsign
					, update->id
					, update->status
					, update->serverID
					);

	outgoing.send(*s_broadcastEndpoint);
}

//////////////////////////////////////////////////////////////////////
// Since we check the curSeverAddress here to determine some actions,
// it is important that curServer is not modified until after this function.
// Exception is _playerAdd which must initialize curServer prior to calling.
static SINT32
_pushPlayerToServer(Server::ServerInfo& server, PlayerStatusNode& psn)
{
	Server::ServerInfo* oldServer = NULL;
	SINT32 retval = 0;

	switch (psn.status)
	{
		case Server::PLAYER_FREE:
			LogAbort("invlid player node %p !!", &psn);
			return -EINVAL;
		case Server::PLAYER_NEW:
			psn.status = Server::PLAYER_INIT;
			break;
		default:
				oldServer = Server::ServerInfo::GetByID(psn.curServer.id);
				if (oldServer == &server)
					return 0;
				else if (oldServer)
				{
					if (_extractPlayerFromServer(oldServer, &psn) == NULL)
					{
						LogAbort("t:%d p:%u couldn't extract player from server %p!!"
								, psn.teulIndex, psn.dispatch().m_customerID, &server);
						return -EINVAL;
					}
				}
			break;
	}

	dbugLog(_FLD, "t:%d p:%d c:%s pushing player %p to server %p ip %s id %llu"
				, psn.teulIndex, psn.dispatch().m_customerID, psn.dispatch().m_callsign
				, &psn, &server, teulGetAddress(server.stats.address), server.stats.id);

	psn.curServer.address = server.stats.address;
	psn.curServer.type = 	server.stats.type;
	psn.curServer.id = server.stats.id;

	if (Server::myServerInfo.flags & Server::SVRFLAG_SEND_PSTAT)
	{
		if (&server == _exitServer && oldServer == &Server::myServerInfo)
		{
			psn.status = Server::PLAYER_IN_EXIT;
			_sendPlayerStatusUpdate(psn);
		}
	}

	if ( &server == _exitServer)
	{
		psn.status = Server::PLAYER_IN_EXIT;
		psn.timeStamp.makeCurrent();
	}
	else
		psn.timeStamp.makeNever();

	auto result = server.m_players.insert(&psn);
	if ( result.second == true && server.stats.address == Server::myServerInfo.stats.address )
	{
		server.stats.addPlayer();
		dbugLog(_FLWB, " numPlayers now %" PRSizeT ", m_players size %" PRSizeT "", server.stats.numPlayers(), server.m_players.size());
	}

	return retval;
}

static SINT32
_playerMove(PlayerStatusNode* const psn, Server::ServerInfo* const server)
{
	if ( server == NULL || psn == NULL)
		return -EINVAL;

	if ( psn->curServer.address == server->stats.address )
		return 0;

	dbugLog(_FLD, "p:%d c:%s moving player %p from server ip %s id %Ld to server %p ip %s id %Ld"
				, psn->dispatch().m_customerID, psn->dispatch().m_callsign, psn
				, teulGetAddress(psn->curServer.address), psn->curServer.id
				, server, teulGetAddress(server->stats.address), server->stats.id );

	return _pushPlayerToServer(*server, *psn);
}

static SINT32
_playerRelease(SINT32 teulIndex, PlayerStatusNode* psn, UINT32 exitCode, Server::ReleaseType releaseType)
{
	if (psn == NULL)
	{
		dbugLog(_FLD, "t:%d attempt to release null player", teulIndex);

		if ( teulIndex >= 0)
			teulClose(teulIndex, Teul::Closing::NullPlayerRelease);

		return -ENOENT;
	}

	switch (releaseType)
	{
	case Server::RELEASE_COLLISION:
	case Server::RELEASE_DISCO:
		teulSendError(teulIndex, Teul::Errors::LoginFail_Collision);

	case Server::RELEASE_EXIT:
	case Server::RELEASE_ERROR:
	case Server::RELEASE_DELETED:
		_playerDelete(psn, releaseType);

		break;

	case Server::RELEASE_ECONN:
		// connection is invalid, extract and move to exitserver
		_playerMove(psn, _exitServer);

		if ( (Server::myServerInfo.flags & Server::SVRFLAG_RELOGIN) == 0 )
		{
			_playerReleasePersona(teulIndex, psn, exitCode, releaseType);
			_playerDeleteAuth(psn);
		}
		else	if ( teulIndex >= 0)
		{
			ServerManager::SetPlayerTeulIndex(psn, -1);
			teulClose(teulIndex, Teul::Closing::ReleaseEconn);
		}

		break;

	case Server::RELEASE_REDIRECTED:
		// assumes player has been moved to new server before called
		_playerReleasePersona(teulIndex, psn, exitCode, releaseType);

		if ( ! ((Server::myServerInfo.flags & Server::SVRFLAG_RELOGIN) && Server::myServerInfo.exitDestType == 0 ) )
		{
			dbugLog(_FLD, "t:%d p:%u deleting auth after redirection", teulIndex, psn->dispatch().m_customerID);
			_playerDeleteAuth(psn);
		}

		break;

	default:
		// we better not ever get here, if so abort so I can see how
		LogAbort("should never get here releaseType == %d", releaseType);
		break;

	}
	return 0;
}

static SINT32
_playerDelete(PlayerStatusNode* const psn, Server::ReleaseType releaseType)
{
	if ( psn == NULL)
		return -ENOENT;

	Server::ServerInfo* server = NULL;

	if ( Server::myServerInfo.exitDestType == AUTH_HOST )
	{
		if ( psn->localInfo == NULL && _exitClusterFn )
		{
			dbugLog(_FLN, "t:%d p:%u exit cluster", psn->teulIndex, psn->dispatch().m_customerID);
			_exitClusterFn(*psn);
		}

		authExitPlayer(psn->dispatch(), Server::myServerInfo.stats.arenaID);
		dbugLog(_FLN, "t:%d p:%u authExitPlayer", psn->teulIndex, psn->dispatch().m_customerID);
	}

	_playerReleasePersona(psn->teulIndex, psn, 0, releaseType);
	_playerDeleteAuth(psn);
	ServerManager::UnregisterPlayer(psn);

	// With player tracking it is possible to have a player whos server is invalid
	// here we extract a player from a valid server if abvailabale
	server = Server::ServerInfo::GetInfo(psn->curServer.address);
	if ( server != NULL)
		 _extractPlayerFromServer(server, psn);
	else
		 _extractPlayerFromServer(_exitServer, psn);

	// alert other servers of the players demise but only if its news
	if ( psn->status != Server::PLAYER_DELETED && Server::myServerInfo.exitDestType == 0 && (Server::myServerInfo.flags & Server::SVRFLAG_SEND_PSTAT) )
	{
		psn->status = Server::PLAYER_DELETED;
		_sendPlayerStatusUpdate(*psn);
	}

	dbugLog(_FLD, "t:%d p:%u FREE %p", psn->teulIndex, psn->dispatch().m_customerID, psn);
	memFree(psn);

	if (	server == &Server::myServerInfo
		&& Server::myServerInfo.stats.status == Server::SERVER_FULL
		&& Server::myServerInfo.stats.availPlayers() > 0 )
	{
		Server::myServerInfo.stats.status = Server::SERVER_ACCEPTING;
		_sendHeartBeat();
	}

	return 0;

}

static void
_deleteAllPlayersOnServer(Server::ServerInfo* const server)
{
	for ( auto psn : server->m_players )
	{
		_playerDelete(psn, Server::RELEASE_ERROR);
	}
}

static void
_releaseServer(Server::ServerInfo* const server, Server::Code scode)
{
	if ( _leaveCallback)
		(_leaveCallback) (server, scode);

	_deleteAllPlayersOnServer(server);

	dbugLog(_FLN, "removing server %s type %d", teulGetAddress(server->stats.address), server->stats.type);

	serverDisconnectPeer(server->stats.address);

	serversByID.erase(server->stats.id);
	serversByIP.erase(Server::BroadcastMaskAddress(server->stats.address));

	delete server;
}

static bool
_playerActivate(SINT32 teulIndex, PlayerStatusNode* const psn)
{
	dbugLog(_FLD, "t:%d p:%u entered activate fn", teulIndex, psn->dispatch().m_customerID);

	if (_activatePlayerFn != NULL)
		(_activatePlayerFn)(teulIndex, psn);

	if (Server::myServerInfo.flags & Server::SVRFLAG_SEND_PSTAT)
	{
		psn->status = Server::PLAYER_INIT;
		_sendPlayerStatusUpdate(*psn);
	}

	return false;
}

static PlayerStatusNode*
_releaseNonMatchingPlayer(SINT32 teulIndex, const playerid_t playerID, UINT32 OTP, UINT32 exitCode, Server::ReleaseType releaseType)
{
	PlayerStatusNode* psn = _getPlayerWith(teulIndex, playerID);
	if ( psn == NULL )
		return NULL;

	if ( psn->dispatch().m_OTP == OTP )
		return psn;

	// Sanity check
	if ( psn->id != playerID && psn->id != 0 )
		dbugLog(_FLA, "t:%d p:%u otp:%u old psn has id:%u", teulIndex, playerID, psn->dispatch().m_OTP, psn->id);
	if ( psn->dispatch().m_customerID != playerID )
		dbugLog(_FLA, "t:%d p:%u otp:%u old psn has disp.p:%u", teulIndex, playerID, psn->dispatch().m_OTP, psn->dispatch().m_customerID);

	const char* const foundPlayer = (ServerInterface::PlayerStatusNodeByPlayerID(playerID) == NULL) ? "no" : "yes";
	const char* const foundTeul = (ServerInterface::PlayerStatusNodeByTeulIndex(teulIndex) == NULL) ? "no" : "yes";

	dbugLog(_FLI, "t:%d p:%u otp mismatch, was:%u is:%u ot:%d. found by player:%s, found by teul:%s", teulIndex, playerID, psn->dispatch().m_OTP, OTP, psn->teulIndex, foundPlayer, foundTeul);
	_playerRelease(psn->teulIndex, psn,  exitCode,  releaseType);

	return NULL;
}

static bool
_playerDestroy(PlayerStatusNode* const psn, bool createComplete)
{
	dbugLog(_FLN, "t:%d p:%u player destroy %p", psn->teulIndex, psn->dispatch().m_customerID, psn);
	if (createComplete)
	{
		_playerRelease(psn->teulIndex, psn, 0, Server::RELEASE_ERROR);
	}
	else
	{
		ServerManager::UnregisterPlayer(psn);
		dbugLog(_FLN, "t:%d p:%u FREE %p", psn->teulIndex, psn->dispatch().m_customerID, psn);
		memFree(psn);
	}

	return false;
}

static bool
_playerProc(Server::PersonaXfer* const persXfer, PlayerStatusNode* const psn, bool createComplete)
{
	switch (persXfer->reason())
	{
		case Server::Transfer::XferReason::Reassign:
		case Server::Transfer::XferReason::Mirror:
			if (!_playerAddAuth(psn))
				return _playerDestroy(psn, createComplete);

			psn->logins = 0;
			// fall through and create persona
		case Server::Transfer::XferReason::Login:

			if (persXfer->reason() == Server::Transfer::XferReason::Login)
			{
				if (psn->logins > 0)
					psn->xferState = Server::Transfer::XferReason::Relogin;
				else
					_playerAddAuth(psn);

				psn->logins++;
			}

			if (psn->localInfo == NULL && ((_createPersonaFn)(*static_cast<Server::PersonaLogin*>(persXfer), *psn)) < 0)
				return _playerDestroy(psn, createComplete);

			break;
		case Server::Transfer::XferReason::Update:

			if ( (Server::myServerInfo.flags & Server::SVRFLAG_DETATCHED) == 0)
			{
				if ( Server::myServerInfo.exitDestType == 0)
				{
					if (!_playerAddAuth(psn))
						return _playerDestroy(psn, createComplete);
				}
				break;
			}
		default:
			LogAbort("shouldnt create player %d for persXfer->reason %d!!", psn->id, persXfer->reason());
			break;
	}

	return true;
}

static PlayerStatusNode*
_playerAdd(Server::PersonaXfer* const persXfer, Server::ServerInfo& server, Server::PlayerStatus status, void* const localInfo)
{
	PlayerStatusNode* psn = NULL;
	SINT32 retval;

	psn = _releaseNonMatchingPlayer(-1, persXfer->dispatch().m_customerID, persXfer->dispatch().m_OTP, 0, Server::RELEASE_DISCO);
	if (psn != NULL)
	{
		psn->xferState = persXfer->reason();

		if ( persXfer->reason() != Server::Transfer::XferReason::Update
			|| (Server::myServerInfo.flags & Server::SVRFLAG_DETATCHED) == 0)
		{
			if ( _playerMove(psn, &server) < 0)
			{
				_playerRelease(psn->teulIndex, psn, 0, Server::RELEASE_ERROR);
	//			_playerDelete(psn);
				return NULL;
			}
		}

		if ( ! _playerProc(persXfer, psn, true))
			return NULL;

		return psn;
	}

	if (_createPersonaFn == NULL)
	{
		dbugLog(_FLAS, "some brain surgeon didn't install the _create persona handler");
		return NULL;
	}

	psn = (PlayerStatusNode*)memCalloc(1, sizeof(PlayerStatusNode));

	psn->status = status;
	psn->localInfo = localInfo;
	psn->dispatch() = persXfer->dispatch();
	psn->teulIndex = -ENOENT;
	psn->host = 0;
	psn->inAuth = FALSE;
	psn->xferState = persXfer->reason();
	psn->status = Server::PLAYER_NEW;

	dbugLog(_FLD, "p:%u c:%s adding psn to server %lld"
				, psn->dispatch().m_customerID, psn->dispatch().m_callsign
				, server.stats.id);

	ServerManager::RegisterPlayer(psn);

	if ( ! _playerProc(persXfer, psn, true))
		return NULL;

	if (	&server == &Server::myServerInfo
		&& server.stats.status == Server::SERVER_ACCEPTING
		&& server.stats.availPlayers() <= 0 )
	{
		server.stats.status = Server::SERVER_FULL;
		_sendHeartBeat();
	}

	retval = _pushPlayerToServer(server, *psn);

	if (retval < 0)
		LogAbort("couldnt add player %d to server %Ld!!", psn->id, server.stats.id);

	return psn;
}

void
PlayerStatusNode::takeAuthorityForThisPlayer()
{
	_pushPlayerToServer(Server::myServerInfo, *this);

	if (Server::myServerInfo.flags & Server::SVRFLAG_SEND_PSTAT)
	{
		_sendPlayerStatusUpdate(*this);
	}
}

static SINT32
_acceptLoginFn(SINT32 teulIndex, Auth::Dispatch* const dispatch)
{
	dbugLog(_FLD, "t:%d p:%u c:%s accepting login otp:%u"
				, teulIndex
				, dispatch->m_customerID
				, dispatch->m_callsign
				, dispatch->m_OTP
				);

	Server::PersonaLogin login(*dispatch);

	PlayerStatusNode* psn = _playerAdd(&login, Server::myServerInfo, Server::PLAYER_INIT, NULL);
	if ( psn == NULL )
	{
		dbugLog(_FLA, "t:%d p:%u c:%s couldnt create persona, login denied!", teulIndex, dispatch->m_customerID, dispatch->m_callsign);
		return -Teul::Errors::LoginFail_AddPlayer;
	}

	ServerManager::SetPlayerTeulIndex(psn, teulIndex);

	_playerActivate(teulIndex, psn);

	return 0;
}

static SINT32
_releaseFn(SINT32 teulIndex, UINT32 customerID, UINT32 otp, Server::ReleaseType releaseType)
{
	dbugLog(_FLD, "t:%d p:%u otp:%u entered release fn", teulIndex, customerID, otp);

	serverReleasePlayer(teulIndex, customerID, otp, 0, releaseType);

	return 0;
}

static void
_timeOutPlayers(void)
{
	for ( auto psn : _exitServer->m_players )
	{
		const TeulDeltaMS elapsed = psn->timeStamp.elapsed();
		if ( elapsed < PLAYER_TIMEOUT )
			continue;

		dbugLog(_FLD, "t:%d p:%d c:%s timing out player after %lldms"
						, psn->teulIndex
						, psn->id
						, psn->dispatch().m_callsign
						, elapsed
						);

		_playerRelease(psn->teulIndex, psn, 0, Server::RELEASE_DISCO);
	}
}

static void
_timeOutXfers(void)
{
	const TeulTimeMS cutoff = teulClock64 - PLAYER_XFER_TIMEOUT;

	while ( s_transferQueue.empty() == false )
	{
		Server::Transfer* xfer = s_transferQueue.front();
		if ( xfer->timestamp() >= cutoff )
			break;

		dbugLog(_FLM, "xfer:%llu timing out", xfer->xferID());

		xfer->onComplete(-EAGAIN);

		delete xfer;

		s_transferQueue.pop_front();
	}
}

static SINT32
_addServer(Server::ServerInfo* const server)
{
	Server::ServerInfo* oldServer = NULL;

	if (	server == NULL
		|| server->stats.type < 0
		|| server->stats.type > MAX_SERVER_TYPES)
		return -EINVAL;

	dbugLog(_FLD, "adding server %s", teulGetAddress(server->stats.address));

	oldServer = Server::ServerInfo::GetByID(server->stats.id);
	if (oldServer != NULL)
		_releaseServer(oldServer, Server::SCODE_REPLACED);

	oldServer = Server::ServerInfo::GetInfo(server->stats.address);
	if(oldServer != NULL)
		_releaseServer(oldServer, Server::SCODE_REPLACED);

	serversByID[server->stats.id] = server;
	serversByIP[Server::BroadcastMaskAddress(server->stats.address)] = server;

	// Make sure our server info gets sent ahead of our attempt to connect.
	_sendHeartBeat();
	serverConnectPeer(server->stats.address, server->stats.type);

	server->availPlayers = server->stats.availPlayers();

	s_activeServerList.insert(server);

	dbugLog(_FLN, "added server %s", teulGetAddress(server->stats.address));

	if (_enterCallback)
		(_enterCallback) (server, Server::SCODE_ADDED);

	dbugLog(_FLD, "leaving addServer");

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Tear down the debugging web server instance.

static void
_killDebugWebServer()
{
	if ( Teul::uWeb::s_debugServer != NULL )
	{
		Teul::uWeb::s_debugServer->cancel();
		OpenThreads::Thread::microSleep(100 * 1000);
		delete Teul::uWeb::s_debugServer;
		Teul::uWeb::s_debugServer = NULL;
	}
}

//////////////////////////////////////////////////////////////////////
// Tear up an instance of the debug web server.

void
serverEnableDebugWebServer(SERVER_TYPE serverType)
{
	if ( Teul::uWeb::s_debugServer != NULL )
		return;

	UINT16 port = WWII::Net::Ports::DebugWebServer + static_cast<UINT16>(serverType);

	Teul::uWeb::s_debugServer = new Teul::uWeb::Server(cv_teulWebPath.GetString(), port);
	Teul::uWeb::s_debugServer->start();

	atexit(_killDebugWebServer);
}

//////////////////////////////////////////////////////////////////////
// Simple URL to test the server is running.

#include "revision.h"

static void
_serverReportAlive(unsigned int fd, const char* /*path*/, const char* /*args*/)
{
	auto& stats = Server::myServerInfo.stats;

	char ip[32];
	teulGetAddress(stats.address, ip, sizeof(ip));

	char buffer[1024];
	const int len = sprintf(buffer, "{\"alive\":\"yes\",\"app\":\"%s\",\"ip\":\"%s\",\"arena\":%u,\"type\":%u,\"status\":%u,\"max_pl\":%u,\"cur_pl\":%u,\"src_rev\":\"" REPOSITORY_INFO "\"}\r\n"
						, Teul::GetAppName()
						, ip
						, stats.arenaID
						, stats.type
						, stats.status
						, stats.maxPlayers()
						, stats.numPlayers()
						);
	send(fd, buffer, len, 0);
}

//////////////////////////////////////////////////////////////////////
// Server functions.

SINT32
serverInit(UINT32 appVersion
				, SINT32 serverType, UINT32 exitDestType
				, SERVER_UPDATE* enterCallback
				, SERVER_UPDATE* leaveCallback
				, SERVER_UPDATE* statusCallback
				, CREATE_PERSONA_FUNCTION*	createPersonaFn
				, RELEASE_PERSONA_FUNCTION* releasePersonaFn
				, ACTIVATE_PLAYER_FUNCTION* activatePlayerFn
				, EXIT_PERSONA_FUNCTION* exitPersonaFn
				, EXIT_CLUSTER_FUNCTION* exitClusterFn
				, ERROR_FUNCTION* errorFunction)
{
	if ( Server::myServerInfo.stats.arenaID == 0 )
	{
		LogAbort("arenaID not specified [didn't call serverInitAddresses first?]");
		/*ABORT*/
	}

	SINT32 retval;

	_enterCallback = enterCallback;
	_leaveCallback = leaveCallback;
	_statusCallback = statusCallback;

	_myServerInfo = &Server::myServerInfo;

	dbugLog(_FLI, "serverInit: app:%u arena:%u servType:%u edt:%u server:{enter:%p, leave:%p, status:%p} persona:{cre:%p, rel:%p, act:%p} exit:{pers:%p, cluster:%p} err:%p"
					, appVersion, Server::myServerInfo.stats.arenaID, serverType, exitDestType, enterCallback, leaveCallback, statusCallback, createPersonaFn, releasePersonaFn, activatePlayerFn, exitPersonaFn, exitClusterFn, errorFunction);

	Server::myServerInfo.stats.address = Server::myInterface.addr[0].ip;
	Server::myServerInfo.stats.id = static_cast<UINT64>(Server::myServerInfo.stats.address) << 32 | Server::myInterface.pid;
	Server::myServerInfo.stats.type = (SINT16)serverType;

	Server::myServerInfo.stats.status = Server::SERVER_LOGIN;
	Server::myServerInfo.stats.m_numPlayers = 0;
	Server::myServerInfo.stats.m_maxPlayers = 0;
	Server::myServerInfo.exitDestType = exitDestType;

	_nextHeartbeat = teulClock64 + Server::HeartbeatInterval_ms;
	_nextUpdate = teulClock64 + Server::CheckInterval_ms;

	_createPersonaFn	= createPersonaFn;
	_releasePersonaFn	= releasePersonaFn;
	extErrorFn 		= errorFunction;
	_activatePlayerFn	= activatePlayerFn;
	_exitPersonaFn	= exitPersonaFn;
	_exitClusterFn = exitClusterFn;

	serverInitializePeering();

	authInitAccept(appVersion, _acceptLoginFn, _releaseFn, extErrorFn);

	if ( (retval = serverInitCallbacks()) < 0)
		return retval;

	_exitServer = new Server::ServerInfo;
	_addServer(_exitServer);
	_addServer(&Server::myServerInfo);

	if ( Teul::uWeb::s_debugServer != NULL )
	{
		Teul::uWeb::s_debugServer->addVirtualFile("Server Status", "/server-alive.json", _serverReportAlive);
	}

	return _myServerInfo->sync();
}

void
serverFree(void)
{
	_leaveCallback = NULL;

	for ( auto server : s_activeServerList )
	{
		_deleteAllPlayersOnServer(server);
		if ( server != &Server::myServerInfo )
			delete server;
	}

	s_activeServerList.clear();

	serversByID.clear();
	serversByIP.clear();
}

//////////////////////////////////////////////////////////////////////
// Get the ideal maximum number of cluster users.

UINT32
serverGetMaxClusterUsers()
{
	const UINT32 maxUsers = Teul::MaxClusterUsers;
	const UINT32 maxClusterUsers = cv_maxClusterUsers.GetUint32();
	return ( maxClusterUsers && maxClusterUsers <= maxUsers ) ? maxClusterUsers : maxUsers;
}

//////////////////////////////////////////////////////////////////////
// Convert a release type to a string

const char*
Server::GetReleaseType(Server::ReleaseType releaseType)
{
	static const char* releaseTypes[] =
	{
		"RELEASE_REDIRECTED"
	,	"RELEASE_DELETED"
	,	"RELEASE_DISCO"
	,	"RELEASE_COLLISION"
	,	"RELEASE_ERROR"
	,	"RELEASE_EXIT"
	,	"RELEASE_ECONN"
	};

	if ( releaseType >= Server::MAX_RELEASE_TYPES )
		return "???";
	return releaseTypes[releaseType];
}

Server::ServerInfo*
Server::ServerInfo::GetInfo(UINT32 baseIP)
{
	SERVERS_BY_IP::iterator it = serversByIP.find(Server::BroadcastMaskAddress(baseIP));
	return ( it != serversByIP.end() ) ? it->second : NULL;
}

Server::ServerInfo*
Server::ServerInfo::GetByID(const UINT64& id)
{
	SERVERS_BY_ID::iterator it = serversByID.find(id);
	return ( it != serversByID.end() ) ? it->second : NULL;
}

Server::ServerInfo*
Server::ServerInfo::GetBest(UINT32 type, bool allowFull/*=false*/)
{
	Server::ServerInfo* maxServer = NULL;

	for ( auto server : s_activeServerList )
	{
		if ( server->stats.type != type )
			continue;
		if ( allowFull == false && server->stats.status == SERVER_FULL )
			continue;
		if ( server->stats.status != SERVER_ACCEPTING )
			continue;
		if ( maxServer == NULL || server->availPlayers > maxServer->availPlayers )
			maxServer = server;
	}

	return maxServer;
}

bool
Server::ServerInfo::IsAvailable(const UINT32 type_)
{
	for ( auto server : s_activeServerList )
	{
		if ( server->stats.type == type_ && server->stats.status == SERVER_ACCEPTING )
			return true;
	}
	return false;
}

SINT32
Server::ServerInfo::sync()
{
	SINT32 retval = -EINVAL;

//	dbugLog(_FLI, "syncing server mode %d", Server::myServerInfo.stats.status);

	if ( _myServerInfo != NULL && Server::broadcastIndex > 0 )
	{
		Server::myServerInfo.availPlayers = Server::myServerInfo.stats.availPlayers();

#ifdef HEAVY_DEBUG
		dbugLog(_FLD, "t:%d sending heartbeat", Server::broadcastIndex);
#endif

		retval = teulSend(Server::broadcastIndex, SERVER_HEARTBEAT_CB, &(Server::myServerInfo.stats));
		if ( retval > 0)
			_nextHeartbeat = teulClock64 + Server::HeartbeatInterval_ms;
	}

	return retval;
}

SINT32
serverBroadcastClusterShutdown()
{
	if ( Server::broadcastIndex < 0 )
	{
		dbugLog(_FLW, "No connection to send shutdown on");
		return -EINVAL;
	}

	dbugLog(_FLW, "Sending cluster shutdown message");
	return teulSend(Server::broadcastIndex, CLUSTER_SHUTDOWN_CB, NULL);
}

void
serverUpdate(void)
{
	static bool inUpdate = false;
	if ((_myServerInfo == NULL) | (inUpdate))
		return;

	inUpdate = true;

	const TeulTimeMS cutoff = teulClock64 - SERVER_FAIL_INTERVAL;

	authExpirePendingOtps();

	if ( s_activeServerList.empty() == false && teulClock64 >= _nextUpdate )
	{
		for( auto server : s_activeServerList )
		{
			if (server == &Server::myServerInfo || server == _exitServer)
				continue;

			if ( server->m_lastRecv < cutoff )
			{
				dbugLog(_FLW, "serv:%s lastRecv too long, assuming server failed"
								, teulGetAddress(server->stats.address));
				_releaseServer(server, Server::SCODE_TIMED_OUT);
			}
		}

		_nextUpdate = teulClock64 + SERVER_FAIL_INTERVAL;
	}

	if ( teulClock64 >= _nextHeartbeat )
	{
		if ( (Server::myServerInfo.flags & Server::SVRFLAG_RELOGIN) && (Server::myServerInfo.exitDestType == 0 || (Server::myServerInfo.flags & Server::SVRFLAG_DETATCHED)) )
			_timeOutPlayers();

		_timeOutXfers();
		_sendHeartBeat();
	}

	inUpdate = false;
}

static void
_updateServerStats(Server::ServerInfo* const server)
{
	server->availPlayers = server->stats.availPlayers();
}

void
Server::HeartBeat::process()
{
	if ( _myServerInfo == NULL ) return;
	// Don't process heartbeat messages until we've sent out first hearbeat.
	// This ensures the server peering system doesn't get borked.
	

	Server::ServerInfo* server = Server::ServerInfo::GetByID(this->id);
	if ( server == NULL )
	{
		dbugWrite(_FLI, "adding new server %s", teulGetAddress(this->address));
		server = new Server::ServerInfo;
		server->stats = *this;
		_addServer(server);
	}
	else
	{
		const bool statusChange = server->stats.status != this->status;

		if (server->stats.type != this->type)
		{
			dbugLog(_FLI, "server:%s changed type %u->%u", teulGetAddress(server->stats.address), server->stats.type, this->type);
			serverDisconnectPeer(server->stats.address);
			if ( _leaveCallback)
				(_leaveCallback)(server, Server::SCODE_REPLACED);
			_sendHeartBeat();
			serverConnectPeer(this->address, this->type);
		}

		server->stats = *this;
		_updateServerStats(server);
		
		if ( statusChange )
		{
			dbugLog(_FLN, "status update for server %s status %d numPlayers %d maxPlayers %d"
						, teulGetAddress(server->stats.address)
						, server->stats.status
						, server->stats.numPlayers()
						, server->stats.maxPlayers()
						);
			if (_statusCallback)
				(_statusCallback)(server, Server::SCODE_STATUS);

			// Check if we've lost our connection to this guy.
			if ( serverPeeringWithIP(this->address) == false )
			{
				_sendHeartBeat();
				serverConnectPeer(this->address, this->type);
			}
		}
	}

#ifdef HEAVY_DEBUG
	dbugLog(_FLD, "updateing server %s status %d numPlayers %d maxPlayers %d"
				, teulGetAddress(server->stats.address)
				, server->stats.status
				, server->stats.numPlayers
				, server->stats.maxPlayers);
#endif

	server->m_lastRecv = teulClock64;
	dbugWrite(_FLD, "heartBeat service for %s", teulGetAddress(server->stats.address));

}

SINT32 serverReleasePlayer(SINT32 teulIndex, UINT32 id, UINT32 otp, UINT32 exitCode, Server::ReleaseType releaseType)
{
	PlayerStatusNode* psn = _getPlayerWith(teulIndex,  id);
	if ( psn == NULL )
		return -ENOENT;

	if ( otp != 0 && psn->dispatch().m_OTP != otp )
	{
		dbugLog(_FLM, "t:%d p:%u not releasing, otp mismatch: %u v %u", teulIndex, id, otp, psn->dispatch().m_OTP);
		return 0;
	}

	return _playerRelease(teulIndex, psn,  exitCode,  releaseType);
}

// move players from one server list to another and update state
// does not create a localinfo persona
SINT32 serverUpdatePlayer(const Server::PlayerStatusUpdate* const update)
{
	Server::PersonaXfer persXfer(Server::Transfer::XferReason::Update, update->dispatch, 0, NULL, update->serverID);

	dbugLog(_FLD, "p:%u c:%s updating player for server id %llu"
				, update->dispatch.m_customerID
				, update->dispatch.m_callsign
				, update->serverID);

	PlayerStatusNode* psn = _releaseNonMatchingPlayer(-1, update->dispatch.m_customerID, update->dispatch.m_OTP, 0, Server::RELEASE_DISCO);

	if ( update->status == Server::PLAYER_DELETED )
	{
		if (psn == NULL)
			return -ENOENT;

		 return _playerRelease(psn->teulIndex, psn, 0, Server::RELEASE_DELETED);
	}

	Server::ServerInfo* const server = Server::ServerInfo::GetByID(update->serverID);
	if ( server == NULL )
		psn = _playerAdd(&persXfer, *_exitServer, update->status, NULL);
	else
		psn = _playerAdd(&persXfer, *server, update->status, NULL);
	if (psn == NULL)
		return -ENOENT;

	psn->dispatch() = update->dispatch;
	psn->status = update->status;

	return 0;
}

SINT32
Server::PersonaExit::onReceive(SINT32 teulIndex)
{
	// Look this player up and see if they are local
	const playerid_t playerID = dispatch().m_customerID;
	PlayerStatusNode* const psn = ServerInterface::PlayerStatusNodeByPlayerID(playerID);

	if ( psn == NULL )
	{
		// Not local
		if (_exitPersonaFn)
			(_exitPersonaFn)(teulIndex, NULL, *this);
	}
	else
	{
		if ( dispatch().m_OTP != 0 && dispatch().m_OTP != psn->dispatch().m_OTP )
			return 0;

		dbugLog(_FLI, "t:%d p:%u c:%s remote PLAYER_EXIT for local player - releasing"
						, psn->teulIndex, playerID, dispatch().m_callsign);
		_playerRelease(psn->teulIndex, psn, 0, RELEASE_EXIT);
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Constructor for a real transfer that is going to be tracked
// locally.

Server::Transfer::Transfer(XferReason _reason, Server::ServerInfo* const _destination)
	: m_xferID(++g_xferID)
	, m_reason(_reason)
	, m_source(Server::myServerInfo.stats.id)
	, m_dest(_destination->stats.id)
	, m_timestamp(teulClock64)
{
	s_transferQueue.push_back(this);
	dbugLog(_FLD, "xfer:%llu added, reason:%u", xferID(), (UINT32)reason());

	// Temporarily boost the player count on that server.
	_destination->stats.addPlayer();

	// To avoid issues with legacy code, recalculate avail rather
	// than simply decrementing it. OGS
	_destination->availPlayers = _destination->stats.availPlayers();
}

//////////////////////////////////////////////////////////////////////
// Mirroring is used to clone a player to another server, specifically
// the chat host as a player logs in.

SINT32
Server::ServerInfo::mirrorPlayer(Server::PersonaMirror* const mirror)
{
	SINT32 destIndex = serverGetPeerIndex(stats.address);
	if ( destIndex < 0 )
	{
		dbugLog(_FLW, "p:%u unable to mirror to server %s: no peering connection", mirror->dispatch().m_customerID, teulGetAddress(stats.address));
		return -ENOTCONN;
	}

	dbugLog(_FLI, "p:%u mirroring to server t:%d ip:%s", mirror->dispatch().m_customerID, destIndex, teulGetAddress(stats.address));

	return teulSend(destIndex, PLAYER_MIRROR_CB, mirror);
}

SINT32
Server::PersonaMirror::onReceive(SINT32 teulIndex)
{
	dbugLog(_FLD, "t:%d p:%u c:%s processing player mirror request", teulIndex, dispatch().m_customerID, dispatch().m_callsign);
	if ( _playerAdd(this, Server::myServerInfo, PLAYER_INIT, NULL) == NULL )
		return -EINVAL;
	return 0;
}

static TransferQueue::iterator
_findTransferNode(Server::Transfer::xferid_t id)
{
	auto it = s_transferQueue.begin();
	while ( it != s_transferQueue.end() )
	{
		if ( (*it)->xferID() == id )
			break;
		++it;
	}
	return it;
}

static void
_destroyTransferNode(TransferQueue::iterator it)
{
	delete* it;
	s_transferQueue.erase(it);
}

SINT32
Server::PersonaMirrorReply::onReceive(SINT32 teulIndex)
{
	auto it = _findTransferNode(xferID());
	if ( it == s_transferQueue.end() )
	{
		dbugLog(_FLA, "xfer:%llu no xnode entry for mirror reply", xferID());
		return -ENOENT;
	}
	if ( (*it)->reason() != Server::Transfer::XferReason::Mirror )
	{
		dbugLog(_FLA, "xfer:%llu reply to a non-mirror", xferID());
		return -ENOENT;
	}

	auto* const mirrorRequest = (Server::PersonaMirror*)(*it);
	SINT32 retval = -ENOENT;
	PlayerStatusNode* const psn = ServerInterface::PlayerStatusNodeByPlayerID(mirrorRequest->dispatch().m_customerID);
	if ( psn == NULL )
	{
		mirrorRequest->onComplete(-ENOENT);
		retval = -ENOENT;
	}
	else
	{
		retval = mirrorRequest->onComplete(resultCode);
	}

	_destroyTransferNode(it);

	return retval;
}

//////////////////////////////////////////////////////////////////////
// Update the registration for a player as a result of their teulindex
// changing.

void
ServerManager::SetPlayerTeulIndex(PlayerStatusNode* const psn, SINT32 teulIndex)
{
#ifdef DEBUG
	if ( psn->teulIndex == teulIndex )
	{
		dbugLog(_FLD, "wasting CPU on setPlayerIndex");
	}
#endif

	if ( psn->teulIndex >= 0 )
	{
		PLAYERIDX_MAP::iterator it = s_byTeulIndex.find(psn->teulIndex);
		if ( it == s_byTeulIndex.end() )
			LogAbort("t:%d p:%u oldt:%d previous instance not registered", teulIndex, psn->id, psn->teulIndex);
		if ( it->second != psn )
			LogAbort("t:%d p:%u newt:%u oldt:%u oldp:%u previous instance registered to someone else"
							, teulIndex, psn->id, psn->teulIndex, (it->second)->teulIndex, (it->second)->id
							);

		s_byTeulIndex.erase(psn->teulIndex);
		teulSetPersonaData(psn->teulIndex, NULL);
	}

	psn->teulIndex = teulIndex;

	if ( teulIndex >= 0 )
	{
		teulSetPersonaData(teulIndex, psn->localInfo);
		s_byTeulIndex[teulIndex] = psn;
	}
}

//////////////////////////////////////////////////////////////////////
// Register a PlayerStatusNode

void
ServerManager::RegisterPlayer(PlayerStatusNode* const psn)
{
	if ( psn->teulIndex >= 0 )
	{
		teulSetPersonaData(psn->teulIndex, psn->localInfo);
		s_byTeulIndex[psn->teulIndex] = psn;
	}

	const playerid_t playerID = psn->dispatch().m_customerID;
	if ( playerID <= 0 )
		LogAbort("Attempted to register invalid playerID %u", playerID);

	PLAYERID_HASH::iterator it = s_byPlayerID.find(playerID);
	if ( it != s_byPlayerID.end() )
		LogAbort("Attempted to register another instance of playerID %u", playerID);

	s_byPlayerID[playerID] = psn;
}

//////////////////////////////////////////////////////////////////////
// Remove the registration of a given PlayerStatusNode

void
ServerManager::UnregisterPlayer(PlayerStatusNode* const psn)
{
	if ( psn->teulIndex >= 0 )
	{
		///TODO: Has it been freed?
		teulSetPersonaData(psn->teulIndex, NULL);
		s_byTeulIndex.erase(psn->teulIndex);
		psn->teulIndex = -1;
	}

	const UINT32 customerID = psn->dispatch().m_customerID;
	if ( customerID > 0 )
	{
		PLAYERID_HASH::iterator it = s_byPlayerID.find(customerID);
		if ( it != s_byPlayerID.end() )
		{
			if ( it->second == psn )
				s_byPlayerID.erase(it);
			else
				dbugLog(_FLA, "Removing customer with incorrect psn pointer");
		}
	}
}

//////////////////////////////////////////////////////////////////////
// Look up a player by PlayerID

wwiiPlayer*
ServerInterface::LookupByPlayerID(UINT32 id)
{
	PLAYERID_HASH::iterator it = s_byPlayerID.find(id);
	return ( it != s_byPlayerID.end() ) ? (wwiiPlayer*)it->second->localInfo : NULL;
}

//////////////////////////////////////////////////////////////////////
// Return the IP address of the auth host

UINT32
serverGetAuthIP()
{
	if ( useAuthServer == false )
		return 0;

	const char* authName = cv_authHost.GetString();
	if ( !authName && !*authName )
		LogAbort("Missing AUTH_ADDRESS");

	UINT32* const addresses = teulGetHostByName(authName);	// Returns malloc'd pointer!
	if ( addresses == NULL )
	{
		if ( useAuthServer == true )
		{
			LogAbort("Unable to resolve auth address '%s'", authName);
			/* ABORTED */
		}

		dbugLog(_FLN, "Couldn't resolve auth address '%s', ignoring", authName);
		return 0;
	}

	UINT32 address = *addresses;

	memFree(addresses);		// teulGetHostByName mallocs, grr.

	return address;
}

SINT32
Teul::ConnectionInstruction::receive(SINT32 teulIndex)
{
	dbugLog(_FLA, "t:%d sent us a connection instruction.", teulIndex);
	return -EINVAL;
}

//////////////////////////////////////////////////////////////////////
// Populate the Server: ... address fields

void
serverInitAddresses()
{
	Server::myServerInfo.stats.arenaID = cv_arenaID.GetUint32();

	Server::broadcastNet = cv_baseAddress.GetUint32();

	Server::localHost = Server::myInterface.addr[0].ip;
	Server::broadcastHost = Server::broadcastNet | (Server::localHost & 0x000000ff);
}

static bool _broadcastRxConnected = false;

static void
_broadcastConnect(ENDPOINT& endpoint)
{
	dbugLog(_FLD, "t:%d ip:%s:%u established broadcast index", endpoint.teulIndex(), teulGetAddress(endpoint.fromAddress()), endpoint.fromPort());
	Server::broadcastConnected = true;
	if ( Server::IsMulticast(Server::broadcastNet) )
		_broadcastRxConnected = true;
	s_broadcastEndpoint = &endpoint;
}

static void
_broadcastRxConnect(ENDPOINT& endpoint)
{
	dbugLog(_FLD, "t:%d ip:%s:%u established broadcast-rx index", endpoint.teulIndex(), teulGetAddress(endpoint.fromAddress()), endpoint.fromPort());
	_broadcastRxConnected = true;
}

void
serverOpenBroadcastIndexes()
{
	dbugLog(_FLD, "opening broadcast indexes");

	assert( Server::broadcastIndex == -1 );
	assert( Server::broadcastConnected == false );

	Server::broadcastIndex = teulOpenSend(Server::broadcastHost, WWII::Net::Ports::PrivateBcast, _broadcastConnect, 0, UDB, NULL);
	dbugLog(_FLN, "t:%d Server::broadcastIndex:%s", Server::broadcastIndex, teulGetAddress(Server::broadcastHost));
	if ( Server::broadcastIndex < 0 )
		LogAbort("unable to open server-broadcast index");

	if ( !Server::IsMulticast(Server::broadcastNet) )
	{
		UINT32 rxHost = Server::broadcastNet | 0x000000ff;
		Server::broadcastRxIndex = teulOpenSend(rxHost, WWII::Net::Ports::PrivateBcast, _broadcastRxConnect, 0, UDB, NULL);
		dbugLog(_FLN, "t:%d broadcastRxIndex:%s", Server::broadcastRxIndex, teulGetAddress(rxHost));
		if ( Server::broadcastRxIndex < 0 )
			LogAbort("unable to open server-broadcast-rx index");
	}
	else
		Server::broadcastRxIndex = Server::broadcastIndex;

	OpenThreads::Thread::YieldCurrentThread();

	for ( UINT32 i = 0; (Server::broadcastConnected == false || _broadcastRxConnected == false) && i < 10; ++i )
	{
		teulUpdateTransport(50);
	}

	if ( Server::broadcastConnected == false )
		LogAbort("timed out waiting for server-broadcast index tear up");
	if ( _broadcastRxConnected == false )
		LogAbort("timed out waiting for broadcast-rx index tear up");
}

void
serverBroadcastHostConfig()
{
	dbugLog(_FLD, "broadcasting host configuration");

	Teul::Packet outgoing(sizeof(host_config), Teul::Serv::CallbackX, HOST_CONFIG_Y);
	outgoing.setUnencrypted();
	outgoing.append(host_config);
	outgoing.send(Server::broadcastIndex);
}

//////////////////////////////////////////////////////////////////////
// Get the number of players on the cluster on servers of a given type.

size_t
serverGetPlayerCount(const UINT32 serverType)
{
	size_t total = 0;
	for ( auto server : s_activeServerList )
	{
		if ( server->stats.type == serverType )
			total += server->stats.numPlayers();
	}

	return total;
}

//////////////////////////////////////////////////////////////////////
// Get the current cluster capacity based on various rules.

size_t
serverGetClusterCapacity()
{
	// Try to cache the value.
	static size_t lastCapacity = 0;
	static TeulTimeMS nextCheck = 0;
	if ( teulClock64 < nextCheck )
		return lastCapacity;
	nextCheck = teulClock64 + 250;

	static const size_t environmentCapacity = serverGetMaxClusterUsers();
	const size_t configCapacity = (size_t)host_config.arena.maxPlayers;

	// Get a peak capacity for each server type.
	size_t clusterCapacity = 0;
	size_t typeCapacity[MAX_SERVER_TYPE] = { 0, 0, 0, 0, 0 };
	for ( auto server : s_activeServerList )
	{
		if ( server->stats.status < Server::SERVER_ACCEPTING )
			continue;

		if ( (UINT32)server->stats.type < (UINT32)MAX_SERVER_TYPE && server->stats.maxPlayers() > 0 )
			typeCapacity[server->stats.type] += server->stats.maxPlayers();
	}

	// Check all the essential servers.
	for ( size_t i = 1; i < (size_t) MAX_SERVER_TYPE; ++i )
	{
		if ( typeCapacity[i] == 0 )
			continue;
		if ( clusterCapacity == 0 || typeCapacity[i] < clusterCapacity )
			clusterCapacity = typeCapacity[i];
	}

	if ( environmentCapacity > 0 && environmentCapacity < clusterCapacity )
		clusterCapacity = environmentCapacity;

	if ( configCapacity > 0 && configCapacity < clusterCapacity )
		clusterCapacity = configCapacity;

	lastCapacity = clusterCapacity;

	return clusterCapacity;
}

#if defined(SERVER_AS_ENTRY_POINT)
//////////////////////////////////////////////////////////////////////
// EXPERIMENTAL: Entry point for all server processes.

int main(int argc, const char* argv[])
{
	if ( argc > 1 )
	{
		extern bool serverProcessCommandLine(int, const char**);
		if ( !serverProcessCommandLine(argc, argv) )
			LogAbort("Invalid command line!\n");
	}

	Teul::SetAppName(g_serverName);
	extern ERROR_FUNCTION serverErrorFunction;

	if ( teulInitTransport(LOG_TO_FILE, g_serverName, serverErrorFunction) < 0 )
		LogAbort("Can't init transport");

	serverEnableDebugWebServer(g_hostType);

	memInit();

	serverInitAddresses();

	extern void serverMain();
	serverMain();
}
#endif

ServerBase::ServerBase(SERVER_TYPE serverType_, const char* serverName_)
	: m_serverType(serverType_)
	, m_serverName(serverName_)
{
	// Singleton check.
	static std::atomic<UINT32> s_initializedCount = 0;
	ABORT_IF( ++s_initializedCount > 1 );

	s_serverBase = this;

	serverEnableDebugWebServer(serverType_);
	memInit();

	Teul::SetAppName(m_serverName);
	if ( teulInitTransport(LOG_TO_FILE, m_serverName, s_ErrorDelegate) < 0 )
		LogAbort("Can't initialize transport.");

	serverInitAddresses();
}

void ServerBase::s_ErrorDelegate(SINT32 teulIndex_, const UCHAR* const data_, SINT32 size_, const TEUL_CALLBACK* const object_, SINT32 error_)
{
	s_serverBase->OnError(teulIndex_, data_, size_, object_, error_);
}

