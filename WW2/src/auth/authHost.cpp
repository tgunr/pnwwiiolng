#include <syscfg.h> // Path is project specific.
// Created: Thu Jun 15 15:26:30 2000
//
// The auth Host
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

#include <signal.h>  //to catch SIGHUPS with

#include "teul/teulKit.h"
#include "auth/authClientData.h"
#include "str/stringLib.h"
#include "auth/authHost.h"
#include "auth/authOTP.h"
#include "auth/authDB.h"
#include "auth/authAppIDs.h"
#include "auth/authPlaygateTypes.h"
#include "auth/authGame.h"
#include "auth/authHostMessages.h"
#include "teul/teulAuth.h"
#include "teul/teulClosingEnums.h"
#include "include/infoHostMessages.h"
#include "wwii/wwiiIRC.h"
#include "wwii/wwiiNetPorts.h"
#include "wwii/wwiiThreads.h"
#include "include/configVars.h"
#include "server/serverInitializeProcess.h"
#include "server/serverInterfaceDescription.h"
#include "teul/teulAuthServer.h"
#include "teul/teulEndpoint.h"

#ifdef DEBUG
	#define DEF_ERR_LEVEL Debug::ERR_DBUG
    #define DEF_ERR_FLAGS STD_ERR  // LOG_ALL_TO_STDERR
#else
	#define DEF_ERR_LEVEL Debug::ERR_INFO
    #define DEF_ERR_FLAGS STD_ERR
#endif

// Support this version at least.
#define COMPATABILITY_LEVEL 0x010120

#define VERSION_ARENA_ID 26  	//arenaID that the version host connects with
								//use this to look up gameEntry stuff for
								//version updates

#define PRESENTER_ARENA_ID 8  	//arenaID for the presenterHost, which is a dummy
								//host for now that only runs so that we can
								//get a version of presenter that the client
								//should be running.  Of course, if you define
								//HARDCODED_VERSIONS below, you don't need a
								//presenterHost


//if the following is defined, the authhost will not send an APP_LAUNCH
//with the appID of the version updater when presenter or an appVersion
//needs updating, but will look up a url based on the appID
//and WEB_ERR type

#define STRAIGHT_TO_WEB_DOWNLOAD
#define REDIRECT_TOO_MANY_PLAYERS -2

//if the following is defined, use these values to check if the following
//apps are up-to-date or not, skip checking their gameEntries.
//
//This allows you to not have a presenterHost running.  You will still need a
//versHost, though, unless you #define STRAIGHT_TO_WEB_DOWNLOAD above

//#define HARDCODED_VERSIONS

#ifdef HARDCODED_VERSIONS
#define PRESENTER_VERSION   "0.1.0.3"
#endif

//define the following if you don't want a dispatch to be sent to the game
//if the customerID that's passed is that of blitz, which is an acct used by
//watcher to check a game's status
#define NO_BLITZ_DISPATCH

#ifdef NO_BLITZ_DISPATCH
#define BLITZ_ID 17374
#endif

#define SECOND 1000
#define MINUTE 60000
#define PLAYER_TIMEOUT (30 * SECOND)
#define GAME_TIMEOUT (5 * MINUTE)

#define NUM_TOS_CHECKSUMS	16
#define MAX_CHECKSUM_LEN	64
#define TOS_FILENAME		"tos.txt"

struct TOS_CHECKSUMS
{
	UINT32	appID;
	UINT32	platform;
	UCHAR	checksum[MAX_CHECKSUM_LEN+1];
} _tosChecksums[NUM_TOS_CHECKSUMS];

static UINT32 numTOSChecksums = 0;

//global variables
static bool readDBRequired = false;

static bool _running = true;

#include <map>
using GameIndex = std::map<UINT32, std::unique_ptr<GAME>>;
static GameIndex gameIndex;

//////////////////////////////////////////////////////////////////////
// Get an Arena Name.

struct ArenaName
{
public:
	ArenaName(UINT32 arenaID)
		: arenaName(authdbGetArenaAlias(arenaID))
	{
		if ( arenaName == nullptr )
		{
			strprintf(arena, "arena:%u", arenaID);
			arenaName = arena;
		}
		else
			arena[0] = 0;
	}

	const char* name() const
	{
		return arenaName;
	}

public:
	const char* arenaName;
private:
	char		arena[21];
};

//////////////////////////////////////////////////////////////////////
// Constructor for a game object.

GAME::GAME()
	:	arenaID(0)
	,	curHostVersion(0)
	,	appID(0)
	,	currentUsers(0)
	,	maxUsers(0)
	,	realIndex(0)
	,	gameID(0)
	,	groupID(0)
	,	clusterID(0)
	,	m_lastReported(0)
	,	lock(AL_OFFLINE)
	,	m_server(nullptr)
	,	requiresMax(true)
	,	calcMaxUsersOnTheFly(true)
	,	m_lastIRCUsers(0)
	,	m_lastIRCReport(0)
{
	strsafecpy(description, "UNKNOWN GAME");
}

GAME::GAME(const AUTH_GAME_INIT& gInit)
	: arenaID(gInit.arenaID)
	, curHostVersion(gInit.hostVersion)
	, appID(gInit.appID)
	, currentUsers(0)
	, maxUsers(gInit.maxUsers)
	, realIndex(0)
	, gameID(0)
	, groupID(0)
	, clusterID(0)
	, m_lastReported(0)
	, lock(AL_OFFLINE)
	, m_server(nullptr)
	, requiresMax(true)
	, calcMaxUsersOnTheFly(true)
	, m_lastIRCUsers(0)
	, m_lastIRCReport(0)
{
	description[0] = 0;

	// Load arena information from the database.
	if ( !authInitGameEntry(*this) || description[0] == 0 )
	{
		strprintf(description, "Unknown Server #%u/%u", arenaID, appID);
	}

	char hostVer[64+1];
	teulGetAddress(curHostVersion, hostVer, sizeof(hostVer) - 1);

	const char* gameName = authdbGetArenaAlias(arenaID);
	if ( gameName == nullptr )
		gameName = description;

	wwiiIRCMessage("-sys,#servers", "AuthInit %s host:%s arena:%u app:%u game:%u grp:%u clu:%u maxu:%u"
					, gameName, hostVer, arenaID, appID, gameID, groupID, clusterID, maxUsers);
}

//////////////////////////////////////////////////////////////////////
// Name for a platform.

static const char*
_platformName(UINT32 platform)
{
	if ( platform == 1 ) return "Mac";
	return "PC";
}

//////////////////////////////////////////////////////////////////////
// Find a game in the index and return a pointer to it or NULL

static GAME*
_findGame(UINT32 arenaID)
{
	auto it = gameIndex.find(arenaID);
	return (it != gameIndex.end() ? it->second.get() : nullptr);
}

static void
_releaseConnection(const SINT32 teulIndex)
{
	Auth::ClientData* cdata = (Auth::ClientData*)teulGetPersonaData(teulIndex);
	if ( cdata == nullptr )
		return;

	teulSetPersonaData(teulIndex, nullptr);
	delete cdata;

	dbugLog(_FLD, "t:%d connection closed", teulIndex);
	teulClose(teulIndex, Teul::Closing::AuthRelease);

	return;
}

// -- helper functions

static void
_fillReply(APP_LAUNCH& reply, SINT32 appID, UINT32 appVersion, const SCHAR* data
	, UINT32 serverIP, UINT32 serverPort, PROTOCOL protocol
	, UINT32 otp, APPDL_ACTION action, SINT32 dlAppID
	)
{
	reply.appID = appID;
	reply.appVersion = appVersion;
	strsafecpy(reply.data, data);
	reply.dataSize = strlen(reply.data);

	char* endURL = strchr((char*)reply.data, '\n');
	if(endURL)
		(*endURL) = '\0';

	reply.serverIP = serverIP;
	reply.serverPort = serverPort;
	reply.protocol = protocol;
	reply.otp = otp;
	reply.action = action;
	reply.dlAppID = dlAppID;
}

//////////////////////////////////////////////////////////////////////
// Generate a code from a WEB_ERR

static const char*
_authenticationErrorString(WEB_ERR errorType)
{
	switch(errorType)
	{
		case WEB_ERR_FULL: return "full";
		case WEB_ERR_LOGIN: return "login";
		case WEB_ERR_GAME: return "game";
		case WEB_ERR_INSTALL: return "install";
		case WEB_ERR_LOCKED: return "locked";
		case WEB_ERR_INTERNAL: return "internal";
		case WEB_ERR_USER: return "invuser";
		case WEB_ERR_PASSWORD: return "invpass";
		case WEB_ERR_SUB: return "inactive";
		case WEB_ERR_NONE: return "";
		case WEB_ERR_AUTH: return "auth";
		default: return "unknown";
	}
}

//////////////////////////////////////////////////////////////////////
// Construct the default error url

static int
__buildURL(char* into, size_t intoSize, WEB_ERR errorType, SINT32 appID, UINT32 appVersion)
{
	assert( intoSize > 0 );

	--intoSize;

	const char* errString = _authenticationErrorString(errorType);
	size_t size = snprintf(into, intoSize
				, "%s?error=%s&game=%u&with=%s"
				, cv_authErrorUrl.GetString()
				, errString, appID
				, teulGetAddress(appVersion));
	into[intoSize - 1] = 0;

	return size;
}
#define buildURL(into, errorType, appID, appVersion) __buildURL(into, sizeof(into), errorType, appID, appVersion)

// authSendArenaInfo()
//
static bool
_sendArenaInfo(Auth::ClientData& cdata, ARENA_REQ& request)
{
	//check to see if the game host has actually connected
	GAME* game = _findGame(request.arenaID);
	if ( game != nullptr )
	{
		dbugLog(_FLD, "t:%d app:%u plat:%u arena:%u got gameEntry", cdata.teulIndex, cdata.appID, cdata.platform, request.arenaID);

		APP_VERIFY reply;
		memset(&reply, 0, sizeof(reply));
		//we have a game, send them a reply

		reply.arenaID		= request.arenaID;
		reply.sessionID		= request.sessionID;

		//Populate game requirments
		reply.appID			= game->appID;
		//for now, the only version we have is what the game server reported
		//when it connected to the auth server.  Use that until we have a
		//database of mins & maxes for each app
		reply.appVersionMax	= game->appInfo[cdata.platform].max;
		reply.status		= game->currentUsers;

		char* appleID = game->appleID;
		if ( appleID )
			memcpy(reply.appleID, appleID, 4);

		if (!(teulSend(cdata.teulIndex, authAPP_VERIFY, &reply)))
			dbugLog(_FLA, "couldn't send APP_VERIFY");

		return true;
	}
	else if (request.infoOnly)
	{
		APP_VERIFY reply;

		reply.arenaID		= request.arenaID;
		reply.sessionID		= request.sessionID;

		//Populate game requirments
		reply.appID			= -1;
		reply.appVersionMax	= -1;
		reply.status		= -1;

		if (!(teulSend(cdata.teulIndex, authAPP_VERIFY, &reply)))
			dbugLog(_FLA,"couldn't send APP_VERIFY");

		return true;
	}

	// No game connected - Populate web redirection
	APP_LAUNCH launch;
	launch.arenaID		= request.arenaID;
	launch.sessionID	= request.sessionID;
	launch.appID		= Auth::Apps::WEB_REDIRECT;
	launch.appVersion	= -1;
	launch.status		= -AUTH_GAME_HOST_DOWN;

	//send the arenaID, since we can't find an appID
	launch.dataSize = buildURL(launch.data, WEB_ERR_GAME, request.arenaID, cdata.appVersion);

	dbugLog(_FLN, "t:%d c:%s ip:%s app:%u plat:%u arena:%u sess:%d redirecting to %s"
				, cdata.teulIndex, request.username, teulGetAddress(cdata.address)
				, cdata.appID, cdata.platform, request.arenaID, request.sessionID
				, launch.data);

	if (!(teulSend(cdata.teulIndex, authAPP_LAUNCH, &launch)))
		dbugLog(_FLA,"couldn't send APP_VERIFY");

	_releaseConnection(cdata.teulIndex);

	return false;
}

// Test whether a checksum is correct for a file for a given app/platform
static bool
_verifyChecksum(const SINT32 /*teulIndex*/, const UINT32 appID, const UINT32 platform, const char* const checksum)
{
	UINT32 i;

	if ( appID == 0 || checksum == nullptr )
		return false;

	for ( i = 0; i < numTOSChecksums; i++ )
	{
		if ( _tosChecksums[i].appID != appID )
			continue;
		if ( _tosChecksums[i].platform != platform )
			continue;
		// It has no checksum, we've already looked this up.
		if ( _tosChecksums[i].checksum[0] == 0 )
			return false;
		if ( strncasecmp(checksum, (char*)_tosChecksums[i].checksum, 64) == 0 )
			return true;
		return false;
	}

	if ( i >= NUM_TOS_CHECKSUMS )
		return false;

	// Assign parameters that will allow a subsequent match to match against us
	dbugLog(_FLI, "No checksum stored yet for %s:%d.%d; fetching", TOS_FILENAME, appID, platform);
	_tosChecksums[i].appID = appID;
	_tosChecksums[i].platform = platform;
	_tosChecksums[i].checksum[0] = 0;
	numTOSChecksums++;

	// Now fetch the actual checksum value from the database, it will be
	// cached next time around because we read it into this buffer.
	if ( !authdbGetFileMD5(appID, platform, TOS_FILENAME, (char*)_tosChecksums[i].checksum, MAX_CHECKSUM_LEN) )
		return false;

	// We found a checksum, now its time to do the match...
	if ( strncasecmp(checksum, (char*)_tosChecksums[i].checksum, MAX_CHECKSUM_LEN) == 0 )
		return true;

	// Didn't match, therefore your checksum is bad
	return false;
}

static void
_reloadGameEntries()
{
	for ( auto it = gameIndex.begin(); it != gameIndex.end(); ++it ) {
		authInitGameEntry(*it->second.get());
	}
}

static void
_reloadAuthDB()
{
	authdbReadArenaAliases();
	authInitApps();
	_reloadGameEntries();
	numTOSChecksums = 0;
}


// _findOptimumServer()
//
// find the server with the fewest number of players
static SERVER*
_findOptimumServer(GAME& game)
{
	SERVER* const best = game.server();

	dbugLog(_FLD, "game:%u(%p) best server: server %p numUsers %d maxUsers %u"
			, game.gameID, &game, best, best->currentUsers, best->maxUsers);

	return best;
}

#if defined(SIGHUP)
static void
_authSIGHUP(int /*signo*/)
{
	signal(SIGHUP, SIG_IGN);

	//Received a kill -HUP
	dbugLog(_FLW,"*** Recieved restart signal");

	readDBRequired = true;

	signal(SIGHUP, _authSIGHUP);
}
#endif

static bool
_presenterOutdated(UINT32 presenterVersion, UINT32 platform)
{
	//don't look up the version in a manifest somewhere
	//also we don't need a presenterHost running for
	//this one b/c we don't try to look up it's
	//version from a game entry

	//use cave's readmanifest stuff
	vers_manifest* vmatch = matchversion(presenterVersion, Auth::Apps::PRESENTER, platform);	// appID=5
	if ( vmatch == nullptr )
		return true;

	return (vmatch->isfile != ISFILE_NUN);
}

// _setupVersionUpdate
//
// an update is needed.  setup the launch to be sent to the client so that the
// appID, appVersion, etc are for the version client.  in the data field, put
// the appID, currentVersion, versionMin, versionMax, & requiresMax for the
// application that needs the update.
//
static bool
_setupVersionUpdate(APP_LAUNCH& launch, APP_LAUNCH_REQ& request, UINT32 platform)
{
	struct vers_manifest* vmatch;

	//this is a hack to just send them an APP_LAUNCH with an appID of
	//APP_WEB_DOWNLOAD and a url.  Presenter will recognize the appID
	//and fire off a browser.  You could use this if for some reason
	//presenter was being a little whore and didn't like talking to
	//the versionserver, like it is now.
	char url[256+1];

	vmatch = matchversion(request.appVersion, request.appID, platform);

	if( vmatch == nullptr)
	{
		dbugLog(_FLW, "unable to find web page for appVersion %x appID %d, platform %d redirecting to general game page"
					, request.appVersion, request.appID, platform);
		buildURL(url, WEB_ERR_INSTALL,request.appID, request.appVersion);	//specific game error
		wwiiIRCMessage("-err", "NO DATA FOR app:%u(%s) ver:%s. Redirecting to:%s"
								, request.appID, _platformName(platform), teulGetAddress(request.appVersion), url);
	}
	else if( vmatch->location[0] != 0) //found an ftp link
	{

		if ( vmatch->ftpURL[0])
			strprintf(url, "%s\n%s\n%s\n%s", vmatch->location, vmatch->ftpURL, vmatch->ftpDir, vmatch->ftpFile);
		else
			strsafecpy(url, vmatch->location);

		wwiiIRCMessage("-err", "%s REDIR %s", request.username, url);
		dbugLog(_FLI, "based on appID %d, sending APP_WEB_DOWNLOAD %s to \"%s\""
				, request.appID, request.username, url);

		_fillReply(launch, Auth::Apps::WEB_DOWNLOAD, 1, url, 0
					, vmatch->port, (PROTOCOL)vmatch->protocol, authGetOTP(), APPDL_INSTALL, request.appID);

		memcpy(launch.appleID, vmatch->appleID, 4);
		launch.status = 0;

		if(vmatch->protocol != DEF_PROTOCOL)
		{
			SCHAR homeDir[128];
			strprintf(homeDir, "appID%u/plat%u/", vmatch->appid, ((vmatch->newNo) >> 24));
			authDBInsertOTP(launch, request, homeDir);
		}
//		launch->status = -AUTH_GENERAL_ERROR;

		return true;
	}
	else
	{
		dbugLog(_FLW, "%s: giveurl didn't work, sending game error", __FUNCTION__);
		buildURL(url, WEB_ERR_INSTALL, request.appID, request.appVersion);	//game error
	}

	//fall through and send to web page
	dbugLog(_FLA, "based on appID %d, sending APP_WEB_REDIRECT %s to \"%s\""
			, request.appID, request.username, url);
	wwiiIRCMessage("-err", "%s WEBREDIR %s", request.username, url);
	_fillReply(launch, Auth::Apps::WEB_REDIRECT, 1, url, 0, 0, PROTOCOL_TCP, authGetOTP(), APPDL_INSTALL, request.appID);

	launch.status = -AUTH_GENERAL_ERROR;

	return true;
}

// _updatePresenter()
//
// send an APP_LAUNCH down to presenter to tell them to
static bool
_updatePresenter(const SINT32 teulIndex, ARENA_REQ& request, UINT32 platform)
{
	APP_LAUNCH_REQ launchRequest;
	memset(&launchRequest,0,sizeof(launchRequest));

	launchRequest.appID = Auth::Apps::PRESENTER;
	launchRequest.appVersion = request.presenterVersion;

	APP_LAUNCH launch;
	if ( _setupVersionUpdate(launch, launchRequest, platform) == FALSE )
	{
		dbugLog(_FLA,"life sucks, we cant update plagate version %d", request.presenterVersion);
		return false;
	}

	if (!(teulSend(teulIndex, authAPP_LAUNCH, &launch)))
	{
		dbugLog(_FLA,"error sending to versionserver for presenter upgrade");
		return false;
	}

	UINT32 destAddress = teulGetEndpointRemoteAddress(teulIndex);
	dbugLog(_FLI, "t:%d user on ip %s needs presenter upgrade from version %s.  sending them to %s"
				, teulIndex, teulGetAddress(destAddress), teulGetAddress(request.presenterVersion), launch.data);
	return true;
}

// _sendDispatch
//
// fill in the otp & clientIP and send the dispatch message
static void
_sendDispatch(Auth::Dispatch& dispatch, UINT32 otp, UINT32 clientIP, SINT32 teulIndex)
{
#ifdef NO_BLITZ_DISPATCH
	if(dispatch.m_customerID == BLITZ_ID)
		return; //don't actually send the dispatch, we were just checking to
				//see if the game host was up
#endif

	dispatch.m_OTP = otp;
	dispatch.m_clientIP = clientIP;

	std::string clientIPAddr = teulGetAddress(clientIP);
	dbugLog(_FLN, "p:%u c:%s ip:%s sending OTP %u version %s"
					, dispatch.m_customerID, dispatch.m_callsign, clientIPAddr.c_str()
					, dispatch.m_OTP, teulGetAddress(dispatch.m_appVersion)
					);

	if ( teulSend(teulIndex, authDISPATCH, &dispatch) > 0)
		dbugLog(_FLN," sent otp %u to game on %d", dispatch.m_OTP, teulIndex);
	else
		dbugLog(_FLA,"something went wrong with sending dispatch to game");
}

static void
_acceptUserConnection(ENDPOINT& endpoint)
{
	char remote[64], local[64];
	teulGetAddress(endpoint.fromAddress(), remote, sizeof(remote));
	teulGetAddress(endpoint.localAddress(), local, sizeof(local));
	dbugLog(_FLD, "t:%d user connection opened from %s:%hu->%s:%hu", endpoint.teulIndex(), remote, endpoint.fromPort(), local, endpoint.localPort());
}

static void
_acceptGameConnection(ENDPOINT& endpoint)
{
	char remote[64], local[64];
	teulGetAddress(endpoint.fromAddress(), remote, sizeof(remote));
	teulGetAddress(endpoint.localAddress(), local, sizeof(local));
	dbugLog(_FLD, "t:%d game connection opened from %s:%hu->%s:%hu", endpoint.teulIndex(), remote, endpoint.fromPort(), local, endpoint.localPort());
}

static void _errorFunction(const SINT32 teulIndex, const UCHAR* const /*data*/, SINT32 /*size*/, const TEUL_CALLBACK* const /*object*/, SINT32 error)
{
	//If the game host endpoint toasts up, then we need to hold off
	if(error == ENOTCONN)
	{
		_releaseConnection(teulIndex);
		return;
	}

#ifdef DEBUG_HEAVY
	dbugLog(_FLW, "Endpoint error - Index %d, error %d",teulIndex,error);
#endif
}


GAME::~GAME()
{
	dbugLog(_FLI, "deleting game %p", this);

	m_server = nullptr;

	authdbUpdateArenaStatus(arenaID, AL_OFFLINE, 0);
}

static void _gameErrorFunction (const SINT32 teulIndex, const UCHAR* const /*data*/, SINT32 /*size*/, const TEUL_CALLBACK* const /*object*/, SINT32 error)
{
	if ( error != ENOTCONN )
	{
		dbugLog(_FLA,"Endpoint error - Index %d, error %d",teulIndex,error);
		return;
	}

	//If the game host endpoint toasts up, then we need to hold off
	SERVER* server = (SERVER*)teulGetPersonaData(teulIndex);
	if ( server == nullptr )
	{
		dbugLog(_FLA, "lost unknown connection");
		return;
	}

	dbugLog(_FLN, "t:%d game:%u arena:%u game connection closed"
					, teulIndex, server->game->gameID, server->game->arenaID);

	GAME* game = server->game;
	if( game->calcMaxUsersOnTheFly )
		game->maxUsers -= server->maxUsers;

	//Ok, we do a lookup and delete using the deleteserver routine
	//	b/c it does all the error checking and such for us already.
	//	Problem is that it returns the address that we already have and
	//	duplicates a bit of the effort, but I'm not too worried about that
	//	at this point in time.
	//authPrintServers(game->servers);

	server->ipAddress = 0;
	if ( game->server() == server )
	{
		if ( _findGame(game->arenaID) == game ) {
			gameIndex.erase(game->arenaID);
		}
		server->game = nullptr;
	}

	if ( server->game == nullptr )
		delete server;

	teulSetPersonaData(teulIndex, nullptr);
}

// process the arena request
// Sends information about an arena or a web redirection
void authProcessArenaRequest(const SINT32 teulIndex, ARENA_REQ& request)
{
	/*  Very first thing, check the version of presenter that they have.
	 *  if it's not up to date, don't even send them an APP_VERIFY,
	 *  fill out an APP_LAUNCH with the info for version client and send
	 *  it down.  Presenter will check for this appID first thing in
	 *  its recv_APP_LAUNCH
	 */
	UINT32 platform = request.presenterVersion >> 24;

	dbugLog(_FLD, "t:%d plat:%d sess:%d arena:%d pv:%d user:%s", teulIndex, platform, request.sessionID, request.arenaID, request.presenterVersion, request.username);

	if( _presenterOutdated(request.presenterVersion, platform) )
	{
		//fill out an APP_LAUNCH pointing to version server and
		//send it
		if(!_updatePresenter(teulIndex, request, platform))
			dbugLog(_FLA, "unable to update presenter. no APP_VERIFY returned");
		return;
	}

	UINT32 destAddress = teulGetEndpointRemoteAddress(teulIndex);
	Auth::ClientData* cdata = new Auth::ClientData(teulIndex, platform, destAddress, request.oVersion, request.username, (char*)request.password);

	teulSetPersonaData(teulIndex, cdata);

	dbugLog(_FLI,"t:%d plat:%d sess:%d arena:%d", teulIndex, platform, request.sessionID, request.arenaID);

	_sendArenaInfo(*cdata, request);
}

static void
_runHost()
{
	// Check queues once a second.
	while ( _running )
	{
		teulUpdateTransport(25);

		if (readDBRequired)
		{
			wwiiIRCChannelMessage("-sys,#servers", "++ Reloading Data ++");
			_reloadAuthDB();
			readDBRequired = false;
		}

		static TeulTimeMS nextRead = 0;
		if ( teulClock64 >= nextRead )
		{
			authdbReadArenaAliases();
			nextRead = teulClock64 + 10 * 60 * 1000;	// 10 minutes
		}

		threadsCallNextCallback();
	}
}

//////////////////////////////////////////////////////////////////////
// Entry point

const char* const g_serverName = "authHost";

int
main(int argc, const char* argv[])
{
	Server::InitializeProcess(g_serverName, AUTH_HOST, _errorFunction, nullptr, argc, argv, {});
	dbugSetWriteLevel(DEF_ERR_LEVEL | DEF_ERR_FLAGS);
	dbugSetLogLevel(DEF_ERR_LEVEL | DEF_ERR_FLAGS);

	wwiiIRCInitialize(Server::myInterface.addr[0].ip, false);
	wwiiIRCAddInitialChannel("-err", "", "Authentication errors");
	wwiiIRCAddInitialChannel("-ok", "", "Authentication approvals");

	dbugLog(_FLN,"connecting to database");

	authInitDB();

    dbugLog(_FLN, "setting up database threads");
    
	threadsSetup();

	dbugLog(_FLN,"installing callbacks");

	authHostInstallCallbacks();

	dbugLog(_FLN, "Installing info callbacks");
	infoInstallHostCallbacks();

	_reloadAuthDB();

	if ( argc > 1 && strcmp(argv[1], "-test") == 0 )
	{
		printf("Test good.\n");
		exit(0);
	}

	wwiiIRCActivate(false);

	// Override with our own signal handler.
#if defined(SIGHUP)
	signal(SIGHUP, _authSIGHUP);
#endif
    
    SINT32 tcpconnection = -1;
    if (Server::IsBroadcastViaController()) {
        tcpconnection = teulOpenListen(WWII::Net::Ports::AltAuthHost, _acceptUserConnection, PLAYER_TIMEOUT, PROTOCOL_TCP, _errorFunction);
        dbugLog(_FLN, "listening for clients on TCP port %u", WWII::Net::Ports::AltAuthHost);
    } else {
        tcpconnection = teulOpenListen(WWII::Net::Ports::AuthHost, _acceptUserConnection, PLAYER_TIMEOUT, PROTOCOL_TCP, _errorFunction);
        dbugLog(_FLN, "listening for clients on TCP port %u", WWII::Net::Ports::AuthHost);
    }
	if (tcpconnection < 0)
		LogAbort("Error allocating endpoint!");

    SINT32 gameConnection = -1;
    
    gameConnection = teulOpenListen(WWII::Net::Ports::InternalAuth, _acceptGameConnection, GAME_TIMEOUT, PROTOCOL_TCP, _gameErrorFunction);
	if ( gameConnection < 0 )
		LogAbort("failed to open game listening socket");
	dbugLog(_FLN, "listening for games on port %u", WWII::Net::Ports::InternalAuth);

	threadsEnable();

	wwiiIRCChannelMessage("-sys,#servers", "++ Running ++");

	_runHost();

	wwiiIRCTerminate("[[ Normal shutdown ]]");

	threadsShutdown();
}

static const char* _describeLock(SINT32 lock)
{
	switch ( lock )
	{
	case AL_LOADING:
	case AL_INITIALIZING:
		return "Starting";
	case AL_SYNCING:
		return "Syncing";
	case AL_OFF:
		return "Open";
	case AL_ENTRY:
		return "Locked";
	default:
	break;
	}
	static std::string buffer;
	buffer = "";
	static const char* bits[] = { "NoCap", "NoSpawn", "NoKill", "CLOSED", nullptr };
	for ( UINT32 i = 0; bits[i] != nullptr; ++i )
	{
		if ( (lock & (1 << i)) == 0 )
			continue;
		if ( buffer.empty() == false )
			buffer += ",";
		buffer += bits[i];
	}

	return buffer.c_str();
}

void
handleGameCapacityUpdate(const AUTH_GAME_CAPACITY& message)
{
	GAME* const game = _findGame(message.arenaID);
	if ( game == nullptr )
	{
		// This can occur when the server is running ahead of the auth server
		// e.g. if the auth server is restarted.
		dbugLog(_FLW, "arena:%u no info yet to handle capacity update"
						, message.arenaID);
		return;
	}

	if ( game->currentUsers != message.totalPlayers
		|| game->maxUsers != message.capacity
		|| message.lock != game->lock )
	{
		if ( (SINT32)message.totalPlayers > (SINT32)(game->m_lastIRCUsers + 50)
			|| (SINT32)message.totalPlayers < (SINT32)(game->m_lastIRCUsers - 50)
			|| game->m_lastIRCReport < teulTime() - 120 )
		{
			wwiiIRCMessage("-sys", "%s update: lock:%u, players:%u/%u"
							, game->description, message.lock, message.totalPlayers, message.capacity);
			game->m_lastIRCUsers = message.totalPlayers;
			game->m_lastIRCReport = teulTime();
		}
	}

	game->currentUsers = message.totalPlayers;
	SINT32 lock = ( message.lock >= AL_ENTRY && (message.lock & AL_ENTRY) ) ? AL_ENTRY : message.lock;
	if ( lock != game->lock )
	{
		game->lock = lock;
		authdbUpdateArenaStatus(message.arenaID, lock, message.totalPlayers);
	}

	if ( game->maxUsers == message.capacity && !game->calcMaxUsersOnTheFly )
	{
		if ( game->m_lastReported < teulTime() - 60 )
		{
			dbugLog(_FLD, "game:%u arena:%u ping [%d/%u] %s"
						, game->gameID, message.arenaID, game->currentUsers, game->maxUsers, _describeLock(game->lock));
			game->m_lastReported = teulTime();
		}
		return;
	}

	game->maxUsers = message.capacity;
	game->calcMaxUsersOnTheFly = FALSE;

	dbugLog(_FLI, "arena:%u game:%d cap:%d players:%d state:%s(%d)"
					, message.arenaID
					, game->gameID
					, game->maxUsers
					, game->currentUsers
					, _describeLock(game->lock), message.lock
					);
	game->m_lastReported = teulTime();
}

void
handleServerLoadUpdate(const AUTH_SERVER_LOAD& message)
{
	GAME* game = _findGame(message.arenaID);
	if( game == nullptr )
	{
		dbugLog(_FLA, "update received from %s for arenaID %u, but we have no info for that server"
						, teulGetAddress(message.hostAddress)
						, message.arenaID);
		return;
	}

	SINT32 totalUsers = 0;
	bool changed = false;

	SERVER* const server = game->server();
	if ( server != nullptr )
	{
		if ( server->ipAddress == message.hostAddress && server->currentUsers != message.numPlayers )
		{
			server->currentUsers = message.numPlayers;
			changed = true;
		}

		totalUsers += server->currentUsers;
	}

	// Only bleat when the tally changes or 5 minutes passes
	if ( changed || game->m_lastReported < teulTime() - 300 )
	{
		dbugLog(_FLI,"arena:%u game:%d cap:%u tally:%u", message.arenaID, game->gameID, game->maxUsers, totalUsers);
		game->m_lastReported = teulTime();
	}

	authdbUpdateArenaStatus(message.arenaID, game->lock, totalUsers);
}

//////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////
//  this game is not running
//
//  we should rarely get here, as usually we should
//  bomb out in authSendArenaInfo()
////////////////////////////////////////////////////
static void
_noGameEntry(const SINT32 teulIndex, UINT32 platform, const char* ip, const APP_LAUNCH_REQ& request, APP_LAUNCH reply)
{
	char tmpStr[64+1];
	strsafecpy(tmpStr, teulGetAddress(request.appVersion));
	dbugLog(_FLW, "t:%d c:%s ip:%s arena:%u plat:%u app:%u ver:%s GAME entry NULL"
			, teulIndex, request.username, ip, request.arenaID, platform, request.appID, tmpStr);

	//send the arenaID, since we can't find an appID
	char url[MAX_PARAM];
	buildURL(url, WEB_ERR_GAME,request.arenaID, request.appVersion);

	_fillReply(reply
				, Auth::Apps::WEB_REDIRECT	//appID
				, 0					//appVersion
				, url 				//data
				, 0					//serverIP
				, 0					//serverPort
				, PROTOCOL_TCP		//protocol
				, 0					// otp
				, APPDL_NODL
				, request.appID
				);
	reply.status = -AUTH_GAME_HOST_DOWN;
}

/////////////////////////////////////////////
//  no info for user, login denied
/////////////////////////////////////////////
static void
_authValidateFailed(const SINT32 teulIndex, UINT32 platform, const char* ip, const APP_LAUNCH_REQ& request, const GAME& gameEntry, APP_LAUNCH& reply, UINT32 error)
{
	char appVer[64+1];
	teulGetAddress(request.appVersion, appVer, sizeof(appVer) - 1);

	ArenaName arena(request.arenaID);

	//Login denied, setup reply to say so
	wwiiIRCMessage("-err", "%s DENIED(%s) %s app:%u(%s) ver:%s"
							, request.username, _authenticationErrorString((WEB_ERR)error), arena.name(), request.appID, _platformName(platform), appVer);

	dbugLog(_FLN, "t:%d c:%s ip:%s arena:%u plat:%u app:%u ver:%s USER denied (validation:%u)"
			, teulIndex, request.username, ip, request.arenaID, platform, request.appID, appVer, error);

	char url[MAX_PARAM];
	buildURL(url, (WEB_ERR)error, gameEntry.appID, request.appVersion);  //which is 8
	_fillReply(reply, Auth::Apps::LOGIN_DENIED, 1, url, 0, 0, PROTOCOL_TCP, 0, APPDL_NODL, gameEntry.appID);

	reply.status = -AUTH_GENERAL_ERROR;
}

///////////////////////////////////////////////
// maxUsers == 0?
//
// game is not accepting, treat as game error
//////////////////////////////////////////////

static void
_gameNotAccepting(const SINT32 teulIndex, UINT32 platform, const char* ip, const APP_LAUNCH_REQ& request, const GAME& gameEntry, APP_LAUNCH& reply)
{
	char appVer[64+1];
	teulGetAddress(request.appVersion, appVer, sizeof(appVer) - 1);

	dbugLog(_FLN, "t:%d c:%s ip:%s arena:%u plat:%u app:%u ver:%s GAME not accepting"
			, teulIndex, request.username, ip, request.arenaID, platform, request.appID, appVer);

	char deniedUrl[MAX_PARAM];
	buildURL(deniedUrl, WEB_ERR_GAME, gameEntry.appID, request.appVersion);
	_fillReply(reply, Auth::Apps::WEB_REDIRECT, 1, deniedUrl, 0, 0, PROTOCOL_TCP, 0, APPDL_NODL, gameEntry.appID);

	reply.status = -AUTH_GAME_HOST_DOWN;
}

/////////////////////////////////////////////
// maxUsers exceeded? no room left in game
/////////////////////////////////////////////

static void
_gameFull(const SINT32 teulIndex, UINT32 platform, const char* ip, const APP_LAUNCH_REQ& request, const GAME& gameEntry, APP_LAUNCH& reply)
{
	char appVer[64+1];
	teulGetAddress(request.appVersion, appVer, sizeof(appVer) - 1);

	dbugLog(_FLN, "t:%d c:%s ip:%s arena:%u plat:%u app:%u ver:%s GAME full"
			, teulIndex, request.username, ip, request.arenaID, platform, request.appID, appVer);

	char deniedUrl[MAX_PARAM];
	buildURL(deniedUrl, WEB_ERR_FULL, gameEntry.appID, request.appVersion);
	_fillReply(reply, Auth::Apps::WEB_REDIRECT, 1, deniedUrl, 0, 0, PROTOCOL_TCP, 0, APPDL_NODL, gameEntry.appID);

	reply.status = -AUTH_GAME_FULL;
}

/////////////////////////////////////////////
// wrong version, point them to a download
/////////////////////////////////////////////
static bool
_versionMismatch(const SINT32 teulIndex, UINT32 platform, const char* ip, APP_LAUNCH_REQ& request, const GAME& gameEntry, APP_LAUNCH& reply)
{
	char reqVer[64+1], minVer[64+1], maxVer[64+1], hostVer[64+1];
	teulGetAddress(request.appVersion, reqVer, sizeof(reqVer) - 1);
	teulGetAddress(gameEntry.appInfo[platform].min, minVer, sizeof(minVer) - 1);
	teulGetAddress(gameEntry.appInfo[platform].max, maxVer, sizeof(maxVer) - 1);
	teulGetAddress(gameEntry.curHostVersion, hostVer, sizeof(hostVer) - 1);

	ArenaName arena(request.arenaID);

	if ( strcmp(minVer, maxVer) == 0 )
		wwiiIRCMessage("-err", "%s DENIED(version) %s app:%u(%s) host:%s requires:%s user:%s"
								, request.username, arena.name(), request.appID, _platformName(platform), hostVer, minVer, reqVer);
	else
		wwiiIRCMessage("-err", "%s DENIED(version) %s app:%u(%s) host:%s requires:%s-%s user:%s"
								, request.username, arena.name(), request.appID, _platformName(platform), hostVer, minVer, maxVer, reqVer);

	dbugLog(_FLN, "t:%d c:%s ip:%s arena:%u plat:%u app:%u ver:%s USER version mismatch, host %s requires %s-%s"
			, teulIndex, request.username, ip, request.arenaID, platform, request.appID, reqVer, hostVer, minVer, maxVer);

	//use cave's checking function

	return _setupVersionUpdate(reply, request, platform);
}

/////////////////////////////////////////////
// Authenticated but couldn't find a server
/////////////////////////////////////////////
static void
_noServerAvailable(const SINT32 teulIndex, const APP_LAUNCH_REQ& request, GAME& gameEntry, APP_LAUNCH& reply)
{
	//this should never happen.  if there were no servers on the list,
	//we wouldn't have found a game entry.  probably the best thing to
	//do here would be to fill out a response, but I'm not sure what
	//to put in it, so I'm just going to log it

	dbugLog(_FLW, "t:%d unable to find a server for arenaID %u, even though gameEntry %p found", teulIndex, request.arenaID, &gameEntry);

	char deniedUrl[MAX_PARAM];
	buildURL(deniedUrl, WEB_ERR_FULL, gameEntry.appID, request.appVersion);
	_fillReply(reply, Auth::Apps::WEB_REDIRECT, 1, deniedUrl, 0, 0, PROTOCOL_TCP, 0, APPDL_NODL, gameEntry.appID);

	reply.status = -AUTH_GAME_FULL;
}

/////////////////////////////////////////////
// Populate the APP_LAUNCH to let them play
/////////////////////////////////////////////
static void
_okToLaunch(const SINT32 teulIndex, UINT32 platform, const char* ip, const APP_LAUNCH_REQ& request, const GAME& gameEntry
			, Auth::Dispatch& dispatch, SERVER& bestServer
			, APP_LAUNCH& reply)
{
	char appVer[64+1], ipStr[64+1];

	// Get the local network address excluding the last octet.
	const UINT32 localAddress = teulGetEndpointLocalAddress(teulIndex);

	UINT32 destAddress = bestServer.extAddress;

	//hack since teulGetAddress sucks
	teulGetAddress(request.appVersion, appVer, sizeof(appVer) - 1);
	teulGetAddress(destAddress, ipStr, sizeof(ipStr) - 1);

	const UINT32 otp = authGetOTP();

	ArenaName arena(request.arenaID);

	wwiiIRCMessage("-ok", "%s GRANT %s app:%u(%s) ver:%s server:%s"
							, request.username, arena.name(), request.appID, _platformName(platform), appVer, ipStr);

	dbugLog(_FLN, "t:%d c:%s ip:%s arena:%u plat:%u app:%u ver:%s USER granted to %s %hu OTP %u on %s"
			, teulIndex, request.username, ip
			, request.arenaID, platform, request.appID, appVer
			, ipStr, bestServer.port, otp, teulGetAddress(localAddress));

	//Login accepted, setup reply to say so
	_fillReply(reply, gameEntry.appID, request.appVersion, "Login Accepted"
				, destAddress, bestServer.port
				, PROTOCOL_TCP, otp, APPDL_NODL, gameEntry.appID);

	reply.status = gameEntry.currentUsers; //pass down the number of users in the game

	// get the appleID
	memcpy(reply.appleID, gameEntry.appleID, 4);

	//try and keep more accurate count between updates
	++bestServer.currentUsers;

	UINT32 clientMask;
	if ( platform == OS_MACOS
		&& ((gameEntry.appInfo[0].min) & 0xFF000000) == 0
		&& ((gameEntry.appInfo[0].max) & 0xFF000000) == 0 )
	{
		clientMask = 0x00FFFFFF;  //strip them of their platform
		dbugLog(_FLI,"setting client mask to %x for mac",clientMask);
	}
	else
		clientMask = 0xFFFFFFFF;

	dispatch.m_platform = platform;
	dispatch.m_appVersion = request.appVersion & clientMask;

	//send dispatch message
	UINT32 clientAddress = teulGetEndpointRemoteAddress(teulIndex);
	_sendDispatch(dispatch, reply.otp, clientAddress, bestServer.teulIndex);
}

//////////////////////////////////////////////////////////////////////
// Dtor for a server.

SERVER::~SERVER()
{
	if ( teulIndex >= 0 )
		teulSetPersonaData(teulIndex, nullptr);
}

//////////////////////////////////////////////////////////////////////
// Process app launch request and build an app launch reply
// Returns false if you should not send a reply

static bool
_processLaunchRequest(const SINT32 teulIndex, APP_LAUNCH_REQ& request, Auth::ClientData& cdata, APP_LAUNCH& reply)
{
	Auth::Dispatch dispatch;
	GAME* gameEntry = nullptr;
	UINT32 platform = 0;

	char ip[64+1];
	cdata.address = teulGetEndpointRemoteAddress(teulIndex);
	strsafecpy(ip, teulGetAddress(cdata.address));

	platform = cdata.platform;

	// trim trailing whitespace from the end of the username
	size_t length = strlen(request.username);
	while ( length > 0 && isspace(request.username[--length]) )
		request.username[length] = 0;

	gameEntry = _findGame(request.arenaID);

	dbugLog(_FLD, "t:%d c:%s app:%d plat:%d arena:%d oVersion:%u game:%u/%u"
					, teulIndex, request.username, cdata.appID, platform, (UINT32)request.arenaID, (UINT32)request.oVersion
					, (gameEntry ? gameEntry->currentUsers : 0), (gameEntry ? gameEntry->maxUsers : 0)
					);

	if( gameEntry == nullptr )
	{
		_noGameEntry(teulIndex, platform, ip, request, reply);
		return false;
	}

	// Fill in the dispatch
	UINT32 error = WEB_ERR_NONE;
    if (request.username[0] || request.steamID == 0)
    {
        error = authValidateUser(teulIndex, request, dispatch, cdata);
    }
    else
    {
        error = authValidateSteamUser(teulIndex, request, dispatch, cdata);
    }

	if ( error != WEB_ERR_NONE )
	{
		_authValidateFailed(teulIndex, platform, ip, request, *gameEntry, reply, error);
		return false;
	}

	if( gameEntry->maxUsers == 0 )  //not accepting yet
	{
		_gameNotAccepting(teulIndex, platform, ip, request, *gameEntry, reply);
		return false;
	}

	if( (SINT32)gameEntry->currentUsers >= (SINT32)gameEntry->maxUsers )
	{
		_gameFull(teulIndex, platform, ip, request, *gameEntry, reply);
		return false;
	}

	if ( request.appVersion < gameEntry->appInfo[platform].min || request.appVersion > gameEntry->appInfo[platform].max)
		//if( !_versionsMatch(gameEntry->appVersion,request->appVersion) )
	{
		///TODO:AUTH: Verify this is the right thing to do
		_versionMismatch(teulIndex, platform, ip, request, *gameEntry, reply);
		return false;
	}

	/////////////////////////////////////
	// everything's peachy
	/////////////////////////////////////
	SERVER* bestServer = _findOptimumServer(*gameEntry);
	if ( bestServer == nullptr )
	{
		_noServerAvailable(teulIndex, request, *gameEntry, reply);
		return false;
	}

	_okToLaunch(teulIndex, platform, ip, request, *gameEntry, dispatch, *bestServer, reply);

	return true;
}

//////////////////////////////////////////////////////////////////////
//this is the function that handles the app_launch_req that we get from
//the client.  It does lots of checks and validations and sends down an
//APP_LAUNCH.
//
//It seems to be growing exponentially.
//
SINT32 authLaunchRequest(const SINT32 teulIndex, APP_LAUNCH_REQ& request)
{
	Auth::ClientData* cdata = (Auth::ClientData*)teulGetPersonaData(teulIndex);
	if (cdata == nullptr)
		return -EINVAL;

	cdata->teulIndex = teulIndex;

	APP_LAUNCH reply;
	memset(&reply,0,sizeof(reply));

	_processLaunchRequest(teulIndex, request, *cdata, reply);

	//send applaunch message
	if (!(teulSend(teulIndex, authAPP_LAUNCH, &reply)))
		dbugLog(_FLA,"Something went wrong with reply");

	return TRUE;
}

//the Hello handler
bool
authEstablishServer(const SINT32 teulIndex, AUTH_GAME_INIT& gInit)
{
	AUTH_GAME_ACK ack;
	char tmpStr[64+1];

	//Setup the server information
	SERVER* newServer = new SERVER;
	newServer->teulIndex = teulIndex;
	newServer->ipAddress = gInit.ipAddress;
	newServer->extAddress = gInit.extAddress;
	newServer->port = gInit.port;
	newServer->sourcePort = 0;	///TODO:AUTH: Should use either local or dest port.
	newServer->maxUsers = gInit.maxUsers;
	dbugLog(_FLN, "t:%d newServer source port is %hu", teulIndex, newServer->sourcePort);

	//Try to find out if this is a new game
	GAME* game = _findGame(gInit.arenaID);
	if ( game == nullptr )
	{
		//No game entry found, we must create a new one
		dbugLog(_FLN,"Creating a new game entry for this arena type");

		try
		{
			auto newGame = std::make_unique<GAME>(gInit);
			game = newGame.get();
			gameIndex[game->arenaID] = std::move(newGame);
		}
		catch ( std::exception& e )
		{
			LogAbort("%s", e.what());
		}
	}
	else //found servers already here for this game
	{
		///TODO:AUTH: Check the server isn't already on the list!
		//increase the total capacity of the game
		if ( game->calcMaxUsersOnTheFly )
			game->maxUsers += gInit.maxUsers;
	}

	if ( game->server() == newServer )
		dbugLog(_FLM, "game already has server assigned");

	//Set the game entry and attach this information to the connection
	newServer->game = game;

	teulSetPersonaData(teulIndex, newServer);

	//Log the connection
	strsafecpy(tmpStr, teulGetAddress(game->curHostVersion));

	//Place the server onto the server list for the game entry we either found
	//	or created
	SERVER* oldServer = game->server();
	if ( oldServer && oldServer != newServer )
	{
		delete oldServer;
	}

	game->server(newServer);

	dbugLog(_FLN,"game host running arenaID %d and version %s is listening on %s:%d"
				,game->arenaID,tmpStr
				,teulGetAddress(newServer->ipAddress),newServer->port);

	//authPrintServers(gameEntry->servers);
	//authPrintGames(gameList);

	ack.gotcha = 1;
	if (!teulSend(teulIndex, authGAME_ACK, &ack))
		dbugLog(_FLA,"something went wrong with sending ack.");

	return true;
}

bool
authVerifyChecksum(const SINT32 teulIndex, const UINT32 appID, const UINT32 platform, const char* const checksum)
{
	SCHAR url[200+1];
	APP_LAUNCH launcher;

	if ( !_verifyChecksum(teulIndex, appID, platform, checksum) )
	{
		dbugLog(_FLD, "t:%d app:%d plat:%u sending TOS appdl_repair", teulIndex, appID, platform);

		// Please don't try to understand these fields without a bottle of advil
		// "\n" is used as a separator since the client isn't actually doing
		// URI parsing. These things* look* like parts of URLs, but it's just an
		// appearance designed to confuse you.
		strprintf(url, "%s\n%s\n%s\n%s"
					, "http://www.playnet.com/bv/wwiiol/downloads.jsp?tos.txt"
					, "downloads.wwiionline.com"
					, "/"
					, "tos.txt");

		_fillReply(launcher, Auth::Apps::WEB_DOWNLOAD, 1, url, 0, 21, PROTOCOL_TCP, 0, APPDL_REPAIR, appID);
		launcher.status = 0;

		teulSend(teulIndex, authAPP_LAUNCH, &launcher);
		return FALSE;
	}

	dbugLog(_FLD, "t:%d app:%u plat:%u sending appPlaygateOk", teulIndex, appID, platform);

	authSendAppPlaygateOk(teulIndex);

	return TRUE;
}

// keep teulKit happy this is only available with teulAuth.so
SINT32
authValidatePlayer(UINT32 /*ipAddress*/, PLAYER_LOGIN& /*loginData*/, Auth::Dispatch& /*dispatch*/)
{
	return -EINVAL;
}

SINT32
authInitAccept(UINT32 /*appVersion*/
				, PLAYER_ACCEPT_FUNCTION /*loginAcceptFn*/
				, PLAYER_RELEASE_FUNCTION /*releaseFn*/
				, ERROR_FUNCTION /*externErrorFn*/)
{
	return -EINVAL;
}

SINT32
authExitPlayer(const Auth::Dispatch& /*player*/, UINT32 /*arenaID*/)
{
	return -EINVAL;
}

bool
authRecordDispatch(Auth::Dispatch* /*dispatch*/, bool /*fromAuth*/)
{
	return false;
}

SINT32
authDeleteDispatch(Auth::Dispatch* /*dispatch*/)
{
	return -ENOENT;
}

//////////////////////////////////////////////////////////////////////
// [Deprecated] Process request for a game list for alpha 1.1.20 playgates

void AppGameList::process()
{
	static const UINT32 arenaIDs[_MAX_GAMES] = { 1, 22, 222, 0, 0, 0, 0, 0 };
	for ( numGames = 0; numGames < _MAX_GAMES && arenaIDs[numGames] != 0; ++numGames )
	{
		GAME* game =_findGame(arenaIDs[numGames]);
		arenaID[numGames] = arenaIDs[numGames];
		lockState[numGames] = (nullptr != game) ? game->lock : AL_OFFLINE;
	}
}

//////////////////////////////////////////////////////////////////////
// Cache structure used by Game List request handler to minimize the
// number of times we have to build server lists.

struct GameListCache
{
	GameListCache() : m_goodUntil(0), m_forSubs(), m_list() {}
	UINT32				m_goodUntil;
	std::vector<UCHAR>	m_forSubs;
	Auth::GameList		m_list;
};

//////////////////////////////////////////////////////////////////////
// [Deprecated] Create a fake game list for alpha clients that don't
// log in before requesting server status lists.

#if COMPATABILITY_LEVEL >= 0x010120
static void
_initializeClientSubs(std::vector<UCHAR>& clientSubs)
{
	// List of the default game IDs we'll pretend we know about.
	unsigned char defaultGameList[] = { 1, 22, 222 };
	clientSubs.reserve(sizeof(defaultGameList));
	for ( UINT32 i = 0; i < clientSubs.capacity(); ++i )
	{
		clientSubs.push_back(defaultGameList[i]);
	}
}
#endif

//////////////////////////////////////////////////////////////////////
// Process a request to get a list of game servers available for the
// given teulIndex. Contains legacy support for alpha 1.2.0s that
// should be removed since this alpha was not made publicly
// available.

Auth::GameList*
authGetGameList(const SINT32 teulIndex)
{
	// Look to see if we have login data for this client.
	Auth::ClientData* cdata = (Auth::ClientData*)teulGetPersonaData(teulIndex);

#if COMPATABILITY_LEVEL >= 0x010120
	// [Deprecated] Legacy for clients that didn't log in.
	static std::vector<UCHAR> defaultClientSubs;
	if ( defaultClientSubs.empty() )
		_initializeClientSubs(defaultClientSubs);

	// [Deprecated] If there is login data, use it otherwise use the
	// statics.
	const std::vector<UCHAR>& clientSubs = (cdata) ? cdata->m_subscribedArenaIDs : defaultClientSubs;
#else
	///TODO: Notify the client of this problem.
	// No data until you are logged in.
	if (cdata == NULL)
	{
		dbugLog(_FLW, "t:%d GetGameList with no cdata", teulIndex);
		return NULL;
	}
	const std::vector<UCHAR>& clientSubs = data->m_subscribedArenaIDs;
#endif

	using CACHES = std::vector<GameListCache>;
	static CACHES caches;

	CACHES::iterator it = caches.begin();
	while ( it != caches.end() )
	{
		GameListCache& cache = *it;
		// Check for expired or not populated first
		if ( cache.m_goodUntil < teulTime() )
		{
				dbugLog(_FLD, "t:%d expiring gamelist cache entry", teulIndex);
			*it = caches.back();
			caches.pop_back();
			continue;
		}

		if ( cache.m_forSubs != clientSubs )
		{
			++it;
			continue;
		}

			dbugLog(_FLD, "t:%d found cached gamelist entry", teulIndex);
		return &cache.m_list;
	}

	caches.resize(caches.size() + 1);

	dbugLog(_FLD, "t:%d populating new gamelist", teulIndex);

	GameListCache& cache = caches.back();

	cache.m_list.populate(clientSubs);
	cache.m_goodUntil = teulTime() + 5;
	cache.m_forSubs = clientSubs;

	return &cache.m_list;
}

void
Auth::GameList::populate(const std::vector<UCHAR>& clientSubs)
{
	const UINT32 plat = 0;

	m_games.clear();
	m_games.reserve(8);

	GameEntry entry;
	for ( UINT32 i = 0; i < clientSubs.size(); ++i )
	{
		entry.m_arenaID = clientSubs[i];
		const char* alias = authdbGetArenaAlias(entry.m_arenaID);
		if ( alias == nullptr )
			continue;
		entry.m_name = alias;
		GAME* game = _findGame(clientSubs[i]);
		if ( game == nullptr )
		{
			entry.m_appID = 0;
			entry.m_lockState = AL_OFFLINE;
			entry.m_populationPctg = 0;
			entry.m_maxVer = entry.m_minVer = 0;
		}
		else
		{
			entry.m_appID = game->appID;
			entry.m_lockState = game->lock;
			if ( game->maxUsers == 0 )
				entry.m_populationPctg = 100;
			else
				entry.m_populationPctg = game->currentUsers * 100 / game->maxUsers;
			entry.m_maxVer = game->appInfo[plat].max;
			entry.m_minVer = ( game->requiresMax ) ? entry.m_maxVer : game->appInfo[plat].min;
		}
		m_games.push_back(entry);
	}
}

SINT32
Auth::LoginSteamRequest::process(const SINT32 teulIndex)
{
	Auth::ClientData* cdata = (Auth::ClientData*)teulGetPersonaData(teulIndex);
	if (cdata != nullptr)
	{
		dbugLog(_FLA, "t:%d c:%s ignoring multiple login requests from client", teulIndex, cdata->m_username.c_str());
		return -EINVAL;
	}

	// Get teul's information about this connection such as IP address etc.
	const UINT32 destAddress = teulGetEndpointRemoteAddress(teulIndex);
	dbugLog(_FLI, "t:%d auth login request plat:%u ip:%s ov:%u s:%u"
					, teulIndex
					, m_platform
					, teulGetAddress(destAddress)
					, 0
					, m_account
					);

	cdata = new Auth::ClientData(teulIndex
									, m_platform			// Windows/Mac?
									, destAddress			// IP address
									, 0						// oVersion
									, m_account);	    	// steamID

	// Translate this into a response:
	WEB_ERR err = (WEB_ERR)validate(teulIndex, *cdata);
	// Text
	const char* code = _authenticationErrorString(err);
	// Filter the playnet status out until you succeed
	bool isPlaynet = (err == WEB_ERR_NONE) ? cdata->isPlaynet : false;

	Auth::LoginReply response(isPlaynet, code);
	response.send(teulIndex);

	if ( err == WEB_ERR_NONE )
		teulSetPersonaData(teulIndex, cdata);
	else
		delete cdata;

	return 0;
}

SINT32
Auth::LoginRequest::process(const SINT32 teulIndex)
{
	Auth::ClientData* cdata = (Auth::ClientData*)teulGetPersonaData(teulIndex);
	if (cdata != nullptr)
	{
		dbugLog(_FLA, "t:%d c:%s ignoring multiple login requests from client", teulIndex, cdata->m_username.c_str());
		return -EINVAL;
	}

	// Get teul's information about this connection such as IP address etc.
	const UINT32 destAddress = teulGetEndpointRemoteAddress(teulIndex);
	dbugLog(_FLI, "t:%d auth login request plat:%u ip:%s ov:%u c:%s p:%s"
					, teulIndex
					, m_platform
					, teulGetAddress(destAddress)
					, 0
					, m_account.c_str()
					, m_passwordMD5.c_str()
					);

	cdata = new Auth::ClientData(teulIndex
									, m_platform			// Windows/Mac?
									, destAddress			// IP address
									, 0						// oVersion
									, m_account.c_str()		// Username
									, m_passwordMD5.c_str());

	// Translate this into a response:
	WEB_ERR err = (WEB_ERR)validate(teulIndex, *cdata);
	// Text
	const char* code = _authenticationErrorString(err);
	// Filter the playnet status out until you succeed
	bool isPlaynet = (err == WEB_ERR_NONE) ? cdata->isPlaynet : false;

	Auth::LoginReply response(isPlaynet, code);
	response.send(teulIndex);

	if ( err == WEB_ERR_NONE )
		teulSetPersonaData(teulIndex, cdata);
	else
		delete cdata;

	return 0;
}

void
authExpirePendingOtps()
{
}
