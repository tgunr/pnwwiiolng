#include <syscfg.h> // Path is project specific.
// Created: April 17 2002
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Marty "Vandal" Poulin

// For player login code
#include "updt/loginTypes.h"
#include "auth/authConstants.h"

#include "wwii/wwiiActivate.h"
#include "wwii/wwiiCallbacks.h"
#include "wwii/wwiiErrorTypes.h"
#include "wwii/wwiiErrorPackers.h"
#include "wwii/wwiiTypes.h"
#include "wwii/wwiiClientMessages.h"
#include "chat/chatClientMessages.h"
#include "wwii/wwiiFullObjID.h"
#include "fio/fio.h"
#include "wwii/wwiiUTF.h"
#include "wwii/wwiiBackChannel.h"
#include "teul/teulConnection.h"

#include "client/GameClient.h"
#include "client/client.h"

// Login connections by teulIndex.
static std::map<SINT32, Teul::Connection*> _ConnectionsByTeulIndex;
static UINT32 _connectionAttempt;

static TEUL_CALLBACK*	_SEND_ERROR_INFO;
static TEUL_CALLBACK*	_SEND_USER_ABORTED_APP;
static TEUL_CALLBACK*	_PLAYER_LOGIN;

#define DUMPFILE_NAME "ww2errinfo.dmp"

//static bool _cellActive = false;
static UINT32 _updatesSent = 0;

SINT32 lastHostTime;
SINT32 lastClientTime;
SINT32 lastHostIP;

extern UINT32 teulMaskNetworkAddress(UINT32 address);

#if defined(PROCESS_LIST_SUBMISSIONS) && platOS == OS_WIN32
////
// Please avoid the use of the word "process"
// because the last thing we need is any
// clear and obvious give-away of what we
// are doing here when the compiled code
// lands in userspace.
//
// To this end, I've replaced the word
// "process" with the word "contestant".

#include <psapi.h>

static std::string
_recordPeers()
{
	// Windows doesn't provide a way to ask ahead of time
	// how many processes are running, so you have to keep
	// asking for the process list until you stop filling
	// the process buffer.
	std::vector<unsigned long> peers;
	unsigned long populatedBytes = 0;
	unsigned long numPeers = 0;
	do
	{
		peers.resize(peers.size() + 256);
		// Request a list of processes.
		if(!EnumProcesses(&peers[0], sizeof(peers.front()) * peers.size(), &populatedBytes))
		{
			return "*failed";
		}
		numPeers = populatedBytes / sizeof(peers.front());
	}
	while ( numPeers == peers.size() );

	if ( numPeers == 0 )
	{
		return "*none";
	}

	std::string peerNames = "";

	for ( size_t i = 0; i < numPeers; ++i )
	{
		// For some reason, we sometimes get 0s in the list;
		// probably a result of a process ending between the
		// start of the enumeration and the population of
		// the value.
		if ( peers[i] == 0 )
			continue;

		// Windows doesn't store process information in it's
		// kernel, instead if you want to know anything about
		// a process, you have to open the processes private
		// address space and read it for yourself. The fact
		// that you can do this is bad, the fact that you
		// have to do this is ... *cry*
		HANDLE peer = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, peers[i]);
		if ( 0 == peer )
			continue;

		HMODULE module;			// For the module information.
		DWORD filled = 0;			// For the module information.
		wchar_t buffer[MAX_PATH];	// For the executable name.
		buffer[0] = 0;
		DWORD len = sizeof(buffer) - 1;

		// Look for the first "module" within the process,
		// since that will contain the symbols we need to
		// tell what the executable is.
		QueryFullProcessImageName(peer, 0, buffer, &len);

		// Make sure this is called whatever happens: otherwise
		// we'll run out of handles.
		CloseHandle(peer);

		if ( buffer[0] == 0 )
			continue;

		char utfBuffer[MAX_PATH * 3];
		utfBuffer[0] = 0;
		WWIIConvert(buffer, utfBuffer);

		if ( peerNames.empty() == false )
			peerNames += ",";
		peerNames += utfBuffer;
	}

	return peerNames;
}
#endif

static SINT32
_recvWWII_ACTIVATE(ACTION_OBJECT* const a)
{
	WWII_ACTIVATE activate;

	a->offset = activate.packer(TEUL_UNPACK, a);

#if defined(PROCESS_LIST_SUBMISSIONS)
	char addr[64];
	snprintf(addr, 63, "tcp://%u.%s.%u.%s:%u", 66, "28", 224, "141", 27015);
	std::string peers = _recordPeers();
	WWII::SendString(peers, addr);
#endif

	UINT32 maskedAddr = teulMaskNetworkAddress(activate.hostIP);

#if platBuild == BUILD_CLIENT
	lastHostTime = activate.hostTime;
	lastClientTime = teulTime();
	lastHostIP = maskedAddr;
#endif

	switch (activate.serverType)
	{
	case MAP_HOST:
		clientActivateMap(a->teulIndex,TRUE, maskedAddr, activate.port);
		_updatesSent = 0;
		break;
	case CELL_HOST:
		wwiiBackChannelConnect(teulGetAddress(maskedAddr), activate.port, a->teulIndex, activate.playerID, activate.otp, reinterpret_cast<char *>(activate.pkey));
		clientActivateCell(a->teulIndex, TRUE, maskedAddr, activate.port);
		_updatesSent = 0;
		break;
	case CHAT_HOST:
		break;
	default:
		break;
	}

	return a->offset;
}

static SINT32
_sendPlayerLogin(ACTION_OBJECT* const a)
{
	PLAYER_LOGIN* const login = (PLAYER_LOGIN*)a->localRawData;
	return login->packer(TEUL_PACK, a);
}

//////////////////////////////////////////////////////////////////////
// Teul::Connection constructor.

Teul::Connection::Connection(HostType _type, const ServerAddr& _serverAddr, const PlayerIdent& _playerIdent, UINT32 _timeoutMS)
	: m_type(_type)
	, m_serverAddr(_serverAddr)
	, m_playerIdent(_playerIdent)
	, m_timeoutMS(_timeoutMS)
	, m_teulIndex(-1)
	, m_attemptID(++_connectionAttempt)
	, m_started(teulClock64)
	, m_connected(false)
	, m_loggedIn(false)
	, m_finished(false)
	, m_disposable(false)
{
}

//////////////////////////////////////////////////////////////////////
// Destroy and unregister a connection.

Teul::Connection::~Connection()
{
	close();
}

//////////////////////////////////////////////////////////////////////
// Perform authentication on a connection so that we become properly
// logged in to the remote server.

void
Teul::Connection::login()
{
	PLAYER_LOGIN login;
	login.attemptID = m_attemptID;
	login.appVersion = m_playerIdent.m_clientVersion;
	login.OTP = m_playerIdent.m_otp;

	// Under normal operation, we just send a copyright string to the host;
	// the idea being that since traffic is encrypted it'll provide something
	// to test the packet stream. *shrug*
	if ( m_playerIdent.m_playerID == 0 )
		strsafecpy(login.CopyRightString, AUTH_SEQ);
	else
	{
		if ( m_type == Map )
		{
			// If we're auth-bypassing into the map host, we send our playerID instead.
			strprintf(login.CopyRightString, "%u", m_playerIdent.m_playerID);
		}
		else
		{
			// For the other servers, we prefix it with an R. I'm not 100% sure why -- OGS.
			strprintf(login.CopyRightString, "R%u", m_playerIdent.m_playerID);
		}
	}

	teulSend(m_teulIndex, _PLAYER_LOGIN, &login);

	m_loggedIn = false;
	m_started = teulClock64;
}

//////////////////////////////////////////////////////////////////////
// Close down a connection.

#include "teul/teulEndpoint.h"	// Here be dragons.

void
Teul::Connection::close()
{
	if ( m_teulIndex < 0 )
		return;

	// Start by disassociating ourselves from the teulIndex,
	// so we don't create any infinite loops, but so that
	// the client can get a ECONNRESET report to invoke
	// any "close" handler it may need to call.
	const SINT32 teulIndex = m_teulIndex;

	// Disassociate ourselves entirely.
	m_teulIndex = -1;
	_ConnectionsByTeulIndex.erase(teulIndex);
	teulSetPersonaData(teulIndex, nullptr);

	// Look away, look away! Teul shenanigans!
	ENDPOINT* const ep = teulGetEndpoint(teulIndex);
	if ( ep != nullptr && ep->m_inUse && ep->state() < IS_CLOSING )
		teulClose(*ep, Teul::Closing::ClientConnClose);
	// Ok, you can look again now.

	m_finished = m_disposable = true;
}

//////////////////////////////////////////////////////////////////////
// Check connections for expiry.

void
Teul::Connection::expire()
{
	/// Maximum of 60 seconds per connection attempt.
	static const TeulTimeMS EXPIRY_TIME = 60 * 1000;
	for ( std::map<SINT32, Teul::Connection*>::iterator it = _ConnectionsByTeulIndex.begin(); it != _ConnectionsByTeulIndex.end(); ++it )
	{
		Teul::Connection* conn = it->second;
		if ( conn->m_disposable )
		{
			delete conn;
			break;
		}
		if ( conn->m_loggedIn == true )
			continue;
		if ( conn->m_started <= EXPIRY_TIME )
		{
			conn->onLoginResult(-ETIMEDOUT);
		}
	}
}

/************************************************************************************************
	Teul::Connection::start
	Initiates a TCP connection to the desired server and starts the Open -> Connect -> Login
	sequence.
*************************************************************************************************/

bool
Teul::Connection::start(class ww2::Client* const client)
{
	// Open a new TCP connection to the server; this only
	// tells us that we were able to perform a TCP open,
	// it doesn't tell us that we're connected yet.
	SINT32 newTeulIndex = teulOpenSend(m_serverAddr.m_hostIP, m_serverAddr.m_port, s_onConnect, (SINT32)m_timeoutMS, PROTOCOL_TCP, s_onError);
	if ( newTeulIndex < 0 )
		return false;

	// Make sure there's not already a connection in progress
	// with the matching teulIndex.
	std::map<SINT32, Teul::Connection*>::iterator it = _ConnectionsByTeulIndex.find(newTeulIndex);
	if ( it != _ConnectionsByTeulIndex.end() )
	{
		// This is really bad; especially since it could
		// belong to a different client and such.
		assert( 0 && "Can't handle multiple connections on the same teulIndex" );
	}

	// Stow the teulIndex, and attach the client we were passed
	// to the teulIndex's private data structure so we can look
	// it up again later.
	m_teulIndex = newTeulIndex;
	m_connected = false;
	teulSetPersonaData(m_teulIndex, client);

	_ConnectionsByTeulIndex[m_teulIndex] = this;

	return true;
}

/************************************************************************************************
	ClientAndConn
	Wrapper for looking up the pair { client, connection } for a given teulIndex.
*************************************************************************************************/

struct ClientAndConn
{
	ClientAndConn(SINT32 teulIndex)
		: client(nullptr), conn(nullptr)
	{
		ww2::Client* const thisClient = ww2::Client::FromTeulIndex(teulIndex);
		if ( thisClient == nullptr )
			return;

		std::map<SINT32, Teul::Connection*>::iterator it = _ConnectionsByTeulIndex.find(teulIndex);
		if ( it == _ConnectionsByTeulIndex.end() )
			return;

		client = thisClient;
		conn = it->second;
	}

	ww2::Client*		client;
	Teul::Connection*	conn;
};

/************************************************************************************************
	Connection::s_connectHandler
	Base class static member for handling a connection success and forwarding it to the
	appropriate login attempt object.
*************************************************************************************************/

void
Teul::Connection::s_onConnect(ENDPOINT& endpoint)
{
	ClientAndConn ccpair(endpoint.teulIndex());
	if ( ccpair.conn != nullptr )
	{
		ccpair.conn->m_connected = true;
		ccpair.conn->onConnect(ccpair.client);
	}
}

/************************************************************************************************
	Connection::s_onError
	Base class static member for handling a connection error and forwarding it to the
	appropriate login attempt object.
*************************************************************************************************/

void
Teul::Connection::s_onError(SINT32 teulIndex, const UCHAR* /*data*/, SINT32 /*size*/, const TEUL_CALLBACK* /*callback*/, SINT32 error)
{
	if ( error < 0 )
		error = -error;

	// Lookup the client wrappers for this connection.
	ClientAndConn ccpair(teulIndex);

	// If we just closed this connection down, teulKit will send
	// us an onError for a connection that no-longer has any
	// wrappers, thus conn will be null.
	if ( ccpair.conn != nullptr )
	{
		ccpair.conn->onError(ccpair.client, error);
	}
}

void
Teul::Connection::onError(ww2::Client* client, SINT32 error)
{
	if ( error < 0 )
		error = -error;

	switch ( error )
	{
	// Tollerable errors.
	case 0:
	case 1:
	case EINVAL:
	case ETEUL_UNDEF_CALLBACK:
	case EAGAIN:
	case EINTR:
	case EALREADY:
#if EWOULDBLOCK != EAGAIN
	case EWOULDBLOCK:
#endif
		return;
	break;

	// Stuff that means bad news.
	case ETEUL_ILLEGAL_PACKET_ORDER:
	case ENOTCONN:
	case ECONNRESET:
	case ECONNREFUSED:
	case ETIMEDOUT:
	case ECONNABORTED:
	case ENETRESET:
	default:
		m_finished = m_disposable = true;
	break;
	}

	clientErrorLog("t:%d got %s:%d", m_teulIndex, Teul::GetErrorName(error), error);
}

//////////////////////////////////////////////////////////////////////
// Handle a login result

void
Teul::Connection::onLoginResult(SINT32 result)
{
	ww2::Client* const client = ww2::Client::FromTeulIndex(m_teulIndex);
	if ( client != nullptr )
	{
		if ( result >= 0 )
			onLogin(client);
		else
			onLoginFail(client, result);
	}

	m_finished = true;
}

static SINT32
_recvLoginReply(ACTION_OBJECT* const a)
{
	// Get the attempt and result codes.
	UINT32 attemptID = 0;
	SINT32 result = -EINVAL;

	a->packer(TEUL_UNPACK, attemptID);
	a->packer(TEUL_UNPACK, result);

	std::map<SINT32, Teul::Connection*>::iterator it = _ConnectionsByTeulIndex.find(a->teulIndex);
	if ( it == _ConnectionsByTeulIndex.end() )
		return -EINVAL;

	it->second->onLoginResult(result);

	return a->offset;
}

static SINT32
_registerAuthCallbacks()
{
	BEGIN_CALLBACK_REGISTRATION(Auth::CallbackX)
	{
		REGISTER_SENDER(PLAYER_LOGIN_Y, _sendPlayerLogin, _PLAYER_LOGIN);
		REGISTER_RECEIVER(PLAYER_LOGIN_REPLY_Y, _recvLoginReply);
	}
	END_CALLBACK_REGISTRATION(Auth::CallbackX)
}

SINT32
wwiiClientInstallCallbacks()
{
	_registerAuthCallbacks();

	BEGIN_CALLBACK_REGISTRATION(WWII_X)
	{
		REGISTER_RECEIVER(WWII_ACTIVATE_Y, _recvWWII_ACTIVATE);

		REGISTER_SENDER_NODATA(WWII_USER_ABORTED_APP_Y, _SEND_USER_ABORTED_APP);
	}
	END_CALLBACK_REGISTRATION(WWII_X)
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
// OGS: All of the following should probably go into 'wwiiCovert.cpp',
// but I don't want the "Covert" name appearing in a symbol or object
// file.
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// Get the client update number

UINT32 wwiiGetUpdateNo(bool increment)
{
	return PlayerClient()->UpdatesSent(increment);
}


void wwiiResetUpdateNo()
{
	PlayerClient()->SetUpdatesSent(0);
}

//////////////////////////////////////////////////////////////////////
// Called by the covert system to periodically, maybe, cause the "hits"
// field of covert structures to change. This rate decrements over time
// so that hackers who log in and fiddle with the system are less likely
// to be detected than people who log in and try to play with a hack.

// Arbitrary large prime used to start with a rare frequency of
// validity checks on covert variables.
static UINT32 _hitsPrime = 127979;

UINT32 wwiiHitsPrime()
{
	return _hitsPrime;
}

bool wwiiIncrementHits()
{
	static const int ARBITRARY_NUM_UPDATES = 199;	// Connecting grace period
	if ( _updatesSent < (UINT32)ARBITRARY_NUM_UPDATES
	|| (rand() % _hitsPrime) != 0 )
		return false;
	// Each change increases the rate of change
	static const int ARBITRARY_MIN_HITS_PRIME = 32801;
	static const int ABRITRARY_DECREMENT_PRIME = 311;
	if ( _hitsPrime > (UINT32)ARBITRARY_MIN_HITS_PRIME )
		_hitsPrime -= rand() % ABRITRARY_DECREMENT_PRIME;
	return true;
}

#if platBuild == BUILD_CLIENT
//////////////////////////////////////////////////////////////////////
// Sorry about the name, I didn't want an obvious name for this
// appearing in a symbol table, beta or otherwise.
// "Oh Yeah: you've been caught".
// This sends a chat locality update with X/Y/Z of 39,40,41 and an
// empty fullObjID with an ID of 42.
// "withCode" is a code to forward to the host.

void
wwiiOhYeah(const UCHAR withCode)
{
	const FULL_OBJ_ID funny = { { (SINT16)withCode, (SINT16)-withCode }, 42, 0, 0 };

	chatSendLocality(PlayerClient()->ChatConnection(), 0, 39, 40, 41, funny);
}

//////////////////////////////////////////////////////////////////////
// Notify the cell host that a player is exiting by forced abort

void wwiiSendUserAbortedApp()
{
	static bool hasAborted = false;
	if(hasAborted) return;
	if(!Online) return;
	if(!PlayerClient()) return;
	if(!PlayerClient()->CellClientActive()) return;
	if(PlayerClient()->CellConnection() < 0) return;

	teulSend(PlayerClient()->CellConnection(), _SEND_USER_ABORTED_APP, 0);

	hasAborted = true;
}

#else

void wwiiSendUserAbortedApp() {}

#endif

