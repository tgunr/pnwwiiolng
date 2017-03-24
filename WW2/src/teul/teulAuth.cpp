#include <syscfg.h> // Path is project specific.
// Created: Mon Oct 08 13:25:26 2000
//
// Auth game API
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Marty "Vandal" Poulin

#include "auth/authBypassOTP.h"
#include "auth/authGameMessages.h"
#include "server/serverServerInfo.h"
#include "teul/teulAuth.h"
#include "teul/teulAuthServer.h"
#include "teul/teulEndpoint.h"
#include "teul/teulTimeMS.h"
//#include "teul/teulEnums.h"

#define AUTH_BYPASS 1
#ifdef AUTH_BYPASS
#include <atomic>
#endif

static const TeulTimeMS LoginInterval_ms = 5 * 60 * 1000;

#define CONNECT_TIMEOUT	20000

#define MINUTE 60000

#define MAX_AUTH_DOWN_TIMEOUT (30 * MINUTE)

static SINT32 _authConnection;
static UINT32 _thisVersion;

static PLAYER_ACCEPT_FUNCTION _playerAcceptFn = nullptr;
static PLAYER_RELEASE_FUNCTION _playerReleaseFn = nullptr;

static bool _authIsOpen = false;

// The complete list of all current OTPs.
using OTPS = std::unordered_map<UINT32, OTP_NODE*>;
static OTPS _otps;
// OTPs that have logged in, by customerID.
static OTPS _custOtps;
// OTPs that haven't logged in yet.
static std::vector<OTP_NODE*> _pendingOtps;

static ERROR_FUNCTION _externalErrorFn;

static bool _authInited = false;

static SINT32 _bypassCustomerID = 1000000;

static bool _sendNotice = false;

static Teul::TimeMS _timeOpen(~0ULL);
static Teul::TimeMS _timeDown(0ULL);
static Teul::TimeMS _timeRetry(0ULL);

static UINT32 _authHost;
static UINT16 _authPort;

static AUTH_GAME_INIT game;

static  SINT32 _openSend(APID appID,
							UINT32 arenaID, UINT32 hostVersion,
							UINT32 authHost, UINT16 authPort,
							UINT32 localAddress, UINT32 extHost, UINT16 localPort,
							UINT32 maxUsers );

static void _authErrorFn(SINT32 teulIndex, const UCHAR* const data, SINT32 size, const TEUL_CALLBACK* const object, SINT32 error);

// This function initializes the game servers communications to the auth
//	server.  It takes all the information provided and establishes the
//	permanent link to the auth server that is used for authentication
//	purposes.
//	PARAMETERS:
//  	gameHost - the IP address the game server is running on
//		extHost - the IP address to tell the client to connect to
//		gamePort - the port the game server is listening for front ends on
//		authHost - the IP address the auth host is running on
//		authPort - the port to connect to the authHost on
//		appID - the application ID necessary to connect to this game server
//		appVersion - the minimum version of the application necessary to
//			connect to this server
//		hostVersion - what version to report to auth
//		maxUsers - the maximum users this server can handle
//		acceptFn - callback to notify you of a player login
//		externErrorFn - callback when errors occur on the auth connection
SINT32 authGameInit(UINT32 gameHost, UINT32 extHost, UINT16 gamePort
				, UINT32 authHost, UINT16 authPort
				, APID appID, UINT32 appVersion
				, UINT32 hostVersion
				, UINT32 maxUsers
				, PLAYER_ACCEPT_FUNCTION acceptFn
				, ERROR_FUNCTION externErrorFn)
{
	_externalErrorFn = externErrorFn;

	_playerAcceptFn = acceptFn;

	_sendNotice = true;

	if ( !_authInited )
	{
#ifdef DEBUG
		dbugWrite(_FLI,"installing game callbaks from init");
#endif

		_thisVersion = appVersion;

		SINT32 retval = authInstallGameCallbacks(_authErrorFn);
		if ( retval < 0 )
			return retval;

		_authInited = true;
	}

	if ( !useAuthServer )
		return 0;

	_authConnection = _openSend(appID, Server::myServerInfo.stats.arenaID, hostVersion, authHost, authPort, gameHost, extHost, gamePort, maxUsers);
	if ( _authConnection < 0 )
	{
		dbugWrite(_FLA, "couldn't open auth connection err:%d", _authConnection);
		return _authConnection;
	}

	SINT32 retval = authSendGameInit(_authConnection);
	if ( retval < 0 )
	{
		dbugWrite(_FLN, "t:%d auth init failed err:%d", _authConnection, retval);
		return retval;
	}

	return _authConnection;
}

// authExitPlayer notifies the auth server that a player has left the game
//	PARAMETERS:
//		player - the pointer to the Auth::Dispatch struct that was returned by
//			authValidatePlayer above
SINT32 authExitPlayer(const Auth::Dispatch& dispatch, UINT32 arenaID)
{
	if ( !useAuthServer )
		return 0;

	if ( memcmp(dispatch.m_callsign, "CRS", 3) == 0 )
		return -EINVAL;

	authSendClientAction(_authConnection,  arenaID, dispatch.m_customerID, CLIENT_LEAVE, dispatch.m_clientIP);

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Fill the dispatch data

static inline void
_populateDispatch(UINT32 ipAddress, const PLAYER_LOGIN& login, const OTP_NODE* otpNode, Auth::Dispatch& dispatch)
{
	dispatch.m_customerID = otpNode->customerID;
	dispatch.m_clientIP = ipAddress;
	dispatch.m_OTP = login.OTP;
	dispatch.m_appVersion = login.appVersion;
	strsafecpy(dispatch.m_callsign, otpNode->handle);
}

//////////////////////////////////////////////////////////////////////
// Handle a regular initial login and populate the data as necessary

static SINT32
_processLoginData(const UINT32 ipAddress, const PLAYER_LOGIN login, const OTP_NODE* otpNode, Auth::Dispatch& dispatch)
{
	char otpVersion[32], loginVersion[32], thisVersion[32];
	teulGetAddress(otpNode->appVersion, otpVersion, 32);
	teulGetAddress(login.appVersion, loginVersion, 32);
	teulGetAddress(_thisVersion, thisVersion, 32);

	dbugLog(_FLI, "LoginData OTP:%u otpV:%s loginV:%s appV:%s crs:%s"
				, login.OTP, otpVersion, loginVersion, thisVersion, login.CopyRightString);

	if ( login.appVersion != otpNode->appVersion )
	{
		dbugLog(_FLW, "version mismatch: this version = %x, otp appVersion is %x, login version is %x"
				, _thisVersion, otpNode->appVersion, login.appVersion);
		return -Teul::Errors::LoginFail_Client;
	}

	const char* crs = (const char*)login.CopyRightString;
	if ( *crs == 'R' )
		++crs;

	if ( isdigit(*crs) )
	{
		UINT32 custID = (UINT32)atol(crs);
		if ( custID != 0 && custID != otpNode->customerID )
		{
			dbugLog(_FLW,"login cookie doesn't match : R%d", otpNode->customerID);
			dbugLog(_FLW,"player sent this cookie       : %s", login.CopyRightString);
			return -Teul::Errors::LoginFail_SessionOwn;
		}
	}
	else if ( memcmp(AUTH_SEQ, login.CopyRightString, AUTH_SEQ_SIZE-1) != 0)
	{
		dbugLog(_FLW,"login cookie doesn't match : %s",AUTH_SEQ);
		dbugLog(_FLW,"player sent this cookie       : %s", login.CopyRightString);
		return -Teul::Errors::LoginFail_Validation;
	}

	_populateDispatch(ipAddress, login, otpNode, dispatch);

	return 0;
}

static OTP_NODE*
_findOTP(const UINT32 OTP)
{
	OTPS::iterator it = _otps.find(OTP);
	return PN_LIKELY( it != _otps.end() ) ? it->second : nullptr;
}

#if defined(AUTH_BYPASS)
static OTP_NODE *
_findCustomer(const UINT32 customerID)
{
	OTPS::iterator it = _custOtps.find(customerID);
	return PN_LIKELY( it != _custOtps.end() ) ? it->second : nullptr;
}

static void
_processBackDoor(PLAYER_LOGIN& login, Auth::Dispatch& dispatch)
{
	static std::atomic<UINT32> backGameCallsign { 0 };

	if ( memcmp(AUTH_SEQ, login.CopyRightString, AUTH_SEQ_SIZE-1) != 0 )
	{
		strprintf(dispatch.m_callsign, "STAFF%05u", backGameCallsign++);

		const char* crs = (const char*)login.CopyRightString;

		if ( *crs == 'R')
		{
			++crs;

			dispatch.m_customerID = (UINT32)atol(crs);
			const OTP_NODE* otpNode = _findCustomer(dispatch.m_customerID);
			if ( otpNode != nullptr )
			{
				login.OTP = otpNode->OTP;
				// For some reason ipAddr needs to be zero here
				_populateDispatch(0, login, otpNode, dispatch);
			}
		}

		dispatch.m_customerID = (UINT32)atol(crs);
	}
	else
	{
		dispatch.m_customerID = _bypassCustomerID;
		strprintf(dispatch.m_callsign, "CRS%07u", backGameCallsign++);
	}

	login.OTP = dispatch.m_OTP = _bypassCustomerID++;
	dispatch.m_clientIP = 0;
	dispatch.m_appVersion = login.appVersion;
}
#endif

//////////////////////////////////////////////////////////////////////
// Checks to see if the auth server has assigned the incoming
// user to the cookie they claim to have. If so, it populates
// the Auth::Dispatch supplied and tells the auth server that
// the player has entered the game.
// Returns 0 on success, -error on failure.
//	PARAMETERS:
//		ipAddress - the IP Address the player has connected using
//		login - the login data supplied by the connecting user
//		dispatch - reference to the dispatch you want populated

SINT32
authValidatePlayer(const UINT32 ipAddress, PLAYER_LOGIN& login, Auth::Dispatch& dispatch)
{
	memset(&dispatch, 0, sizeof(dispatch));

	// Playgate logs the user in to auth and gets a session cookie or One Time Pass (otp).
	// The client then provides the customer:otp pair for each connection to the cluster.
	// In theory:
	// . a cust+otp match with no session match = new login,
	// . a cust+otp match matching an existing session = client-dropped connection recovery,
	// . a cust+otp non-match indicates a server-dropped recovery or a stale login.
	// We also catch the case where a player launches a new client either as a result of a
	// crash or by logging in to their account from another window/session/machine.
	// If a man-in-the-middle sees the exchange with auth, they can simply launch the client
	// with the appropriate "-otp ${value}" argument and seize the session. Presumably
	// this wouldn't be worth much as the player would simply log in again causing auth to
	// invalidate the previous otp session and kick the mitm out of the system.
	const OTP_NODE* const otpNode = _findOTP(login.OTP);
	if ( otpNode != nullptr )
	{
		return _processLoginData(ipAddress, login, otpNode, dispatch);
	}
	else
#ifdef AUTH_BYPASS
	if ( login.OTP == AUTH_BYPASS_OTP )
	{
		_processBackDoor(login, dispatch);
		return 0;
	}
	else
#endif
	{
		char address[65];
		teulGetAddress(ipAddress, address, sizeof(address));
		dbugLog(_FLM, "otp:%u ip:%s otp not registered, session probably expired", login.OTP, address);
		return -Teul::Errors::LoginFail_NoSession;
	}
}

SINT32
authInitAccept(UINT32 appVersion
				, PLAYER_ACCEPT_FUNCTION loginAcceptFn
				, PLAYER_RELEASE_FUNCTION releaseFn
				, ERROR_FUNCTION externErrorFn)
{
	_playerAcceptFn = loginAcceptFn;
	_playerReleaseFn = releaseFn;

	if (  !_authInited )
	{
		_thisVersion = appVersion;

		const SINT32 authVal = authInstallGameCallbacks(externErrorFn);
		if ( authVal < 0)
			return authVal;

		dbugLog(_FLI, "registered appVersion = %x", appVersion);

		_authInited = true;
	}

	return 0;
}

SINT32
authAcceptPlayer(const SINT32 teulIndex, const playerid_t playerIP, PLAYER_LOGIN& loginData)
{
	Auth::Dispatch dispatch;

	const SINT32 validation = authValidatePlayer(playerIP, loginData, dispatch);
	if ( validation < 0 )
	{
		dbugLog(_FLA, "t:%d p:%u c:%s ip:%s otp:%u authValidatePlayer failed [%d]", teulIndex, dispatch.m_customerID, dispatch.m_callsign, teulGetAddress(playerIP), loginData.OTP, -validation);
		return validation;
	}

	SINT32 retval = -EINVAL;
	if (_playerAcceptFn != nullptr)
		retval = (_playerAcceptFn) (teulIndex, &dispatch);

	if ( !useAuthServer || _sendNotice == false || retval < 0 )
	{
		dbugLog(_FLD, "t:%d p:%u c:%s otp:%u validated", teulIndex, dispatch.m_customerID, dispatch.m_callsign, dispatch.m_OTP);
		return retval;
	}

	if ( memcmp(dispatch.m_callsign,"CRS",3) == 0 )
	{
		dbugLog(_FLD, "t:%d p:%u c:%s otp:%u staff-validated", teulIndex, dispatch.m_customerID, dispatch.m_callsign, dispatch.m_OTP);
		return retval;
	}

	OTP_NODE* otpNode = _findOTP(loginData.OTP);
	if ( otpNode == nullptr || otpNode->fromAuth )
	{
		dbugLog(_FLD, "t:%d p:%u c:%s otp:%u non-auth validated", teulIndex, dispatch.m_customerID, dispatch.m_callsign, dispatch.m_OTP);
		return retval;
	}

    char playerAddress[32];
    teulGetAddress(playerIP, playerAddress, sizeof(playerAddress));
    dbugLog(_FLD, "t:%d p:%u c:%s otp:%u validated, sending auth CLIENT_ENTER ar:%u ip:%u(%s)"
                    , teulIndex
                    , dispatch.m_customerID
                    , dispatch.m_callsign
                    , dispatch.m_OTP
                    , Server::myServerInfo.stats.arenaID
                    , playerIP
                    , playerAddress
                    );

	authSendClientAction(_authConnection, Server::myServerInfo.stats.arenaID, dispatch.m_customerID, CLIENT_ENTER, playerIP);

	return retval;
}

void
_authOpened(ENDPOINT& endpoint)
{
	_authIsOpen = true;
	_authConnection = endpoint.teulIndex();

	if ( _timeDown.time() != 0 )
	{
		if ( _externalErrorFn != nullptr)
			(_externalErrorFn) (endpoint.teulIndex(),  nullptr, 0, nullptr, ETEUL_AUTH_RESTARTED);

	}
	_timeDown.makeZero();

	const SINT32 retval = teulSend(_authConnection, authGAME_INIT, &game);	///TODO:Optimize.
	if ( retval < 0 )
		dbugLog(_FLA,"Something went wrong in sending auth: %d.",_authConnection);
	else
		dbugLog(_FLN,"Sending auth game.");
}

static void _authErrorFn(SINT32 teulIndex, const UCHAR* const data, SINT32 size, const TEUL_CALLBACK* const object, SINT32 error)
{
	static SINT32 errorCount = 0;

	errorCount++;

	if ( !useAuthServer || error != ENOTCONN || !_authIsOpen )
	{
		dbugWrite(_FLA,"t:%d err:%d error on auth connection", teulIndex, error);
		if ( _externalErrorFn != nullptr)
			(_externalErrorFn) (teulIndex,  data, size, object, error);
		return;
	}

	if ( _timeDown.time() == 0)
	{
		_timeDown.makeCurrent();
	}

	if ( _timeDown.elapsed() <= MAX_AUTH_DOWN_TIMEOUT )
	{
		dbugLog(_FLA,"Auth connection error count %d, waiting to try restart", errorCount);

		// auth is not connected - lets reset a wait timer before we try to reconnect and let the main loop handle it
		if ( _timeRetry.time() == 0)
		{
			_timeRetry.makeCurrent();
	        }

	}
	else
	{
		// NOTE: this is an extreme case of auth being down for a 1/2 hour or more

		dbugLog(_FLA,"can't restart auth connection time down: %" PR64BitT "", _timeDown.elapsed());
		if ( _externalErrorFn != nullptr)
			(_externalErrorFn) (teulIndex,  data, size, object, ETEUL_AUTH_DOWN);
	}
}

// opens a Game connection to an Auth Server.
// <appID> is the application ID needed to connect to the server
// <arenaID> is the arena ID the game services
// <hostVersion> is the version of the arena the game is running
// <authHost> host address of an Auth Server.
// <authPort> port address of an Auth Server.
// <localPort> local port game is listening for new users on.
SINT32 _openSend(APID appID, UINT32 arenaID, UINT32 hostVersion, UINT32 authHost, UINT16 authPort, UINT32 localAddress, UINT32 extAddress, UINT16 localPort, UINT32 maxUsers)
{
//static AUTH_GAME_INIT game;
	_authIsOpen = false;

	if (authHost == 0)
		return -EINVAL;

	_authHost = authHost;
	_authPort = authPort;

	game.arenaID = arenaID;
	game.hostVersion = hostVersion;
	game.appID = appID;
	game.maxUsers = maxUsers;
	game.port = localPort;
	game.ipAddress = localAddress;
	game.extAddress = extAddress;
	strsafecpy(game.hello, "Hello!");

	_authConnection = teulOpenSend(_authHost, _authPort, _authOpened, CONNECT_TIMEOUT, PROTOCOL_TCP, _authErrorFn);
	_timeOpen.makeCurrent();

	if (_authConnection < 0)
	{
		dbugWrite(_FLA,"Couldn't connect to auth server!");
		return _authConnection;
	}

	teulSyncClock();
	const TeulTimeMS startTime = teulClock64;
	static const TeulTimeMS timeout_ms = 60 * 1000;
	const TeulTimeMS cutoff    = startTime + timeout_ms;

	while ( ! _authIsOpen )
	{
		if ( teulClock64 >= cutoff )
		{
			dbugLog(_FLA, "couldnt open auth connection to %s after %" PR64BitUT "ms", teulGetAddress(authHost), teulClock64 - startTime);
			return -EINVAL;
		}
		teulUpdateTransport(1000);
	}

	return _authConnection;
}

OTP_NODE::OTP_NODE(Auth::Dispatch& dispatch_, bool fromAuth_)
	: timeOut(teulClock64 + LoginInterval_ms)
	, OTP(dispatch_.m_OTP)
	, customerID(dispatch_.m_customerID)
	, address(dispatch_.m_clientIP)
	, appVersion(dispatch_.m_appVersion)
	, fromAuth(fromAuth_)
{
	strsafecpy(handle, dispatch_.m_callsign);

	assert( _otps.find(OTP) == _otps.end() );
	_otps[OTP] = this;

    static const size_t MAX_IP_LEN = 64;
    char ip[MAX_IP_LEN+1], version[MAX_IP_LEN];
    teulGetAddress(address, ip, MAX_IP_LEN);
    teulGetAddress(appVersion, version, MAX_IP_LEN);
    dbugLog(_FLD, "p:%u c:%s otp:%u ptr:%p ip:%s appVersion:%s +ALLOC {%" PRSizeT ",%" PRSizeT ",%" PRSizeT "}", customerID, handle, OTP, this, ip, version, _otps.size(), _pendingOtps.size(), _custOtps.size());
}

static void
_removeFromOtps(OTP_NODE* node)
{
	auto it = _otps.find(node->OTP);
	if ( it != _otps.end() && it->second == node )
		_otps.erase(it);
}

static void
_removeFromPendingOtps(OTP_NODE* node)
{
	auto it = std::find(_pendingOtps.begin(), _pendingOtps.end(), node);
	if ( it != _pendingOtps.end() )
	{
		*it = _pendingOtps.back();
		_pendingOtps.pop_back();
	}
}

static void
_removeFromCustomerOtps(OTP_NODE* node)
{
	auto it = _custOtps.find(node->customerID);
	if ( it != _custOtps.end() && it->second == node )
		_custOtps.erase(it);
}

OTP_NODE::~OTP_NODE()
{
	_removeFromOtps(this);
	_removeFromPendingOtps(this);
	_removeFromCustomerOtps(this);

	static const size_t MAX_IP_LEN = 64;
	char ip[MAX_IP_LEN+1], version[MAX_IP_LEN];
	teulGetAddress(address, ip, MAX_IP_LEN);
	teulGetAddress(appVersion, version, MAX_IP_LEN);

	dbugLog(_FLN, "p:%u c:%s otp:%u ptr:%p ip:%s appVersion:%s -REL {%" PRSizeT ",%" PRSizeT ",%" PRSizeT "}", customerID, handle, OTP, this, ip, version, _otps.size(), _pendingOtps.size(), _custOtps.size());
}

static bool
_registerLocalDispatch(Auth::Dispatch& dispatch, const bool fromAuth)
{
	OTP_NODE* node = _findOTP(dispatch.m_OTP);
	if ( node == nullptr )
	{
		node = new OTP_NODE(dispatch, false);
	}
	else
	{
		// Transfer from pending list.
		_removeFromPendingOtps(node);
		node->fromAuth = fromAuth;
	}

	dbugLog(_FLD, "p:%u otp:%u -> custOtps", node->customerID, node->OTP);

	_custOtps[dispatch.m_customerID] = node;

	return true;
}

static bool
_registerNewDispatch(Auth::Dispatch& dispatch)
{
	OTP_NODE* node = _findOTP(dispatch.m_OTP);
	if ( node != nullptr )
	{
		dbugLog(_FLW, "otp:%u duplicate Auth::Dispatch for this OTP", dispatch.m_OTP);
		return false;
	}

	node = new OTP_NODE(dispatch, true);

	_pendingOtps.push_back(node);

	dbugLog(_FLD, "p:%u otp:%u -> pendingOtps [%" PR64BitUT "]", node->customerID, node->OTP, node->timeOut);

	return true;
}

void
authExpirePendingOtps()
{
	static TeulTimeMS nextCheck = 0ULL;
	if ( _pendingOtps.empty() || teulClock64 < nextCheck )
		return;
	nextCheck = teulClock64 + 500;

	std::vector<OTP_NODE*> expiredNodes;
	for (auto it = _pendingOtps.begin(), end = _pendingOtps.end(); it != end; ++it)
	{
		OTP_NODE* node = *it;
		if ( node && node->timeOut <= teulClock64 )
			expiredNodes.push_back(node);
	}

	for (auto node : expiredNodes)
	{
		dbugLog(_FLM, "p:%u otp:%u OTP has expired", node->customerID, node->OTP);
		delete node;
	}
}

//This function handles the cookie when we get it
bool
authRecordDispatch(Auth::Dispatch* dispatch, bool fromAuth)
{
	ABORT_IF( dispatch == nullptr );

	dbugLog(_FLD,"p:%u c:%s otp:%u ip:%s new cookie %s"
				, dispatch->m_customerID
				, dispatch->m_callsign
				, dispatch->m_OTP
				, teulGetAddress(dispatch->m_clientIP)
				, ((fromAuth) ? "from auth" : "non-auth")
				);

	if ( fromAuth )
		return _registerNewDispatch(*dispatch);
	else
		return _registerLocalDispatch(*dispatch, fromAuth);
}

SINT32 authDeleteDispatch(Auth::Dispatch* dispatch)
{
	ABORT_IF( dispatch == nullptr );

	auto it = _otps.find(dispatch->m_OTP);
	if ( PN_UNLIKELY(it == _otps.end()) )
		return -ENOENT;

	delete it->second;

	return 0;
}

//
// verifyAuthConnected()
//
//    Returns true when auth is connected.
//    Returns true when auth is in the process of connecting
//    Returns false when auth is not connected or is in the process of connecting
//
//    This function should be used before any transmisions to auth to ensure a valid connection exists.  If one does not
//    it will attempt a connection to Auth after a respectible grace period (10 seconds).  Teul is non-blocking I/O so
//    a connection is not always instant so it may still return false even if Auth is available for connection.
//
//    NOTE: This function depends on the certainty of being called to maintain an auth connection.  Any periodic transmissions
//    to auth should be calling this function first
//
bool verifyAuthConnected()
{
	bool bConnected = false;

	// Is there no connection ID? Or, is there no endpoint configured?  Or, is the endpoint unusable due to state?
	ENDPOINT *endPoint = nullptr;
	if (_authConnection < 0 || ((endPoint = teulGetEndpoint(_authConnection)) == nullptr) ||
		endPoint->state() == IS_CLOSED || endPoint->state() == IS_DEAD)
	{
		// No valid auth connection exists.  Has 10 seconds elapsed since last attempt?
		if ( _timeRetry.elapsed() >= 10000 )
		{
			// Grace period elapsed, attempt a connection
			dbugLog(_FLI,"Auth connection down attempting restart");

			// try restarting
			_authConnection = teulOpenSend(_authHost, _authPort, _authOpened, CONNECT_TIMEOUT, PROTOCOL_TCP, _authErrorFn);
			if (_authConnection >= 0)
			{
				// Connection attempt is at least in progress, check and see if its open yet
				endPoint = teulGetEndpoint(_authConnection);
				if (endPoint != nullptr && (endPoint->state() == IS_OPEN || endPoint->state() == IS_LISTENING))
					// yes, tell the caller auth connection is usable
					bConnected = true;
			}
			else
			{
				dbugLog(_FLA, "Unable to restart Auth connection");
			}

			// reset the timer for another wait period so we don't spam attempts
			_timeRetry.makeCurrent();
		}
		// no, then don't do anything and tell the caller there is no connection
	}
	else
	{
		if (endPoint->state() == IS_OPEN || endPoint->state() == IS_LISTENING)
			bConnected = true;
	}

	return bConnected;

}

SINT32 authUpdateServerLoad(SINT32 numPlayers, UINT32 hostAddress, UINT32 arenaID)
{
	if ( !useAuthServer )
		return 0;

	// This is called periodically so make sure we have an auth connection before we send it
	if (verifyAuthConnected())
	{

		AUTH_SERVER_LOAD loadMsg;

		loadMsg.numPlayers = numPlayers;
		loadMsg.hostAddress = hostAddress;
		loadMsg.arenaID = arenaID;

		const SINT32 retval = teulSend(_authConnection, authSERVER_LOAD, &loadMsg);
		if ( retval < 0 )
		{
#ifdef DEBUG
			dbugWrite(_FLA,"Something went wrong in sending auth load msg: %d.",_authConnection);
#endif
			return retval;
		}
#ifdef DEBUG
		dbugWrite(_FLN,"Sending AUTH_SERVER_LOAD %d players on arena %d.",numPlayers,arenaID);
#endif
	}

	return 0;
}

SINT32 authSetGameCapacity(SINT32 capacity, SINT32 totalPlayers, UINT32 arenaID, SINT32 lock)
{
	if ( !useAuthServer )
		return 0;

	// This is called periodically so make sure we have an auth connection before we send it
	if (verifyAuthConnected())
	{
		AUTH_GAME_CAPACITY message;

		message.capacity = capacity;
		message.totalPlayers = totalPlayers;
		message.arenaID = arenaID;
		message.lock = lock;

		const SINT32 retval = teulSend(_authConnection, authGAME_CAPACITY, &message);
		if ( retval < 0 )
		{
#ifdef DEBUG
			dbugWrite(_FLA,"Something went wrong in sending game capacity msg: %d.",_authConnection);
#endif
			return retval;
		}
#ifdef DEBUG
		dbugWrite(_FLN,"Sending AUTH_GAME_CAPACITY %d.", capacity);
#endif
	}

	return 0;
}
