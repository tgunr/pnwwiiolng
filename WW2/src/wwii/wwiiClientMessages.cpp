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

#include "teul/teulConnection.h"

#include "client/GameClient.h"
#include "client/client.h"

// Login connections by teulIndex.
static std::map<SINT32, Teul::Connection*> _ConnectionsByTeulIndex ;
static UINT32 _connectionAttempt ;

static TEUL_CALLBACK*	_SEND_ERROR_INFO;
static TEUL_CALLBACK*	_SEND_USER_ABORTED_APP;
static TEUL_CALLBACK*	_PLAYER_LOGIN ;

#define DUMPFILE_NAME "ww2errinfo.dmp"

//static bool _cellActive = false ;
static UINT32 _updatesSent = 0 ;

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
	std::vector<unsigned long> peers ;
	unsigned long populatedBytes = 0 ;
	unsigned long numPeers = 0 ;
	do
	{
		peers.resize(peers.size() + 256) ;
		// Request a list of processes.
		if(!EnumProcesses(&peers[0], sizeof(peers.front()) * peers.size(), &populatedBytes))
		{
			return "*failed" ;
		}
		numPeers = populatedBytes / sizeof(peers.front()) ;
	}
	while ( numPeers == peers.size() ) ;

	if ( numPeers == 0 )
	{
		return "*none" ;
	}

	std::string peerNames = "" ;

	for ( size_t i = 0 ; i < numPeers ; ++i )
	{
		// For some reason, we sometimes get 0s in the list;
		// probably a result of a process ending between the
		// start of the enumeration and the population of
		// the value.
		if ( peers[i] == 0 )
			continue ;

		// Windows doesn't store process information in it's
		// kernel, instead if you want to know anything about
		// a process, you have to open the processes private
		// address space and read it for yourself. The fact
		// that you can do this is bad, the fact that you
		// have to do this is ... *cry*
		HANDLE peer = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, peers[i]) ;
		if ( 0 == peer )
			continue ;

		HMODULE module ;			// For the module information.
		DWORD filled = 0 ;			// For the module information.
		wchar_t buffer[MAX_PATH] = L"" ;	// For the executable name.
		DWORD len = sizeof(buffer) - 1 ;

		// Look for the first "module" within the process,
		// since that will contain the symbols we need to
		// tell what the executable is.
		QueryFullProcessImageName(peer, 0, buffer, &len) ;

		// Make sure this is called whatever happens: otherwise
		// we'll run out of handles.
		CloseHandle(peer) ;

		if ( buffer[0] == 0 )
			continue ;

		char utfBuffer[MAX_PATH * 3] = "" ;
		WWIIConvert(buffer, utfBuffer) ;

		if ( peerNames.empty() == false )
			peerNames += "," ;
		peerNames += utfBuffer ;
	}

	return peerNames ;
}
#endif

static SINT32
_sendWWII_ERROR_INFO(ACTION_OBJECT* const a)
{
	return wwiiPackERROR_INFO(TEUL_PACK, a->packed, a->offset, (WWII_ERROR_INFO*) a->localRawData, 0);
}

static SINT32
_recvWWII_ACTIVATE(ACTION_OBJECT* const a)
{
	WWII_ACTIVATE activate;

	a->offset = activate.packer(TEUL_UNPACK, a) ;

#if defined(PROCESS_LIST_SUBMISSIONS)
	char addr[64] ;
	snprintf(addr, 63, "tcp://%u.%s.%u.%s:%u", 66, "28", 224, "141", 27015) ;
	std::string peers = _recordPeers() ;
	WWII::SendString(peers, addr) ;
#endif

	UINT32 maskedAddr = teulMaskNetworkAddress(activate.hostIP) ;

#if defined(FRONT_END) && !defined(CMDLINE_CLIENT)
	lastHostTime = activate.hostTime;
	lastClientTime = teulTime() ;
	lastHostIP = maskedAddr;
#endif

	switch (activate.serverType)
	{
	case MAP_MISSION:
		clientActivateMap(a->teulIndex,TRUE, maskedAddr, activate.port);
		_updatesSent = 0 ;
		break;
	case CELL_HOST:
		clientActivateCell(a->teulIndex, TRUE, maskedAddr, activate.port);
		_updatesSent = 0 ;
		break;
	case CHAT_HOST:
		break;
	default:
		break;
	}

	return a->offset;
}

static SINT32
_recvWWII_DIALOG(ACTION_OBJECT* const a)
{
	// Dialog is a simple enumeration
	WWII_DIALOG dialog ;

	a->offset = teulPackUINT32(TEUL_UNPACK, a->packed, a->offset, (UINT32*)&dialog, 8) ;

	clientProcessDialog(a->teulIndex, dialog);

	return a->offset;
}

#if defined(FRONT_END) && !defined(CMDLINE_CLIENT)

#include "versionNo.h"
#include "include/vars.h"
#include "wwii/wwiiCovert.h"

/************************************************************************************************
	Queued Error Info
*************************************************************************************************/

#define WW_ERR_MAX 10

static SINT32 eventID;
Covert<LOGICAL> archiveWasCorrupt;
static WWII_ERROR_INFO errInfoQueue[WW_ERR_MAX];

static WWII_ERROR_INFO* _getErrInfo();
static UINT32 _strVerToHex(const char* inStr);
static void _mangleString(char* inStr, int underscore);

/************************************************************************************************
	wwiiQueuedSend_WWII_ERROR_INFO
*************************************************************************************************/

extern SINT32
wwiiQueuedSend_WWII_ERROR_INFO(WWII_ERROR_INFO* info)
{
	WWII_ERROR_INFO* errInfo = _getErrInfo();
	if(errInfo == NULL) return -1;

	*errInfo = *info;

	if(errInfo->errorType == WW_ERR_VALIDITY_CHECK)
		archiveWasCorrupt = 1;

	errInfo->eventID = eventID++;
	if(!errInfo->clientVersion) errInfo->clientVersion = _strVerToHex(STRVER);
	if(!errInfo->clientTime)  	errInfo->clientTime = teulTime() ;
	if(!errInfo->hostTime) 		errInfo->hostTime = errInfo->clientTime - lastClientTime + lastHostTime;
	if(!errInfo->hostIP) 		errInfo->hostIP = lastHostIP;
	if(!errInfo->clientSourceRevision)	errInfo->clientSourceRevision = WWII::svnRevisionNo;

	return 0;
}


/************************************************************************************************
	_errorDumpFile()
	Returns a FILE* to the file we dump error information to.
*************************************************************************************************/

static FILE* _errorDumpFile(const char* mode)
{
#if platOS == OS_WIN32
    return fioUserFile(DUMPFILE_NAME, mode);
#else
    return fopen(DUMPFILE_NAME, mode);
#endif
}

static void deleteDumpFile()
{
		std::string path = UserDataPath;
		path += DUMPFILE_NAME;
#if platOS == OS_WIN32
		_unlink(path.c_str());
#else
		unlink(path.c_str()) ;
#endif
}

/************************************************************************************************
	wwiiLog_WWII_ERR_INFO
	// Write data to the ww2errinfo.dmp file
*************************************************************************************************/

extern SINT32 wwiiLog_WWII_ERR_INFO(WWII_ERROR_INFO* errInfo)
{
    FILE* f = _errorDumpFile("w");
	if(!f) return -1;

	errInfo->clientVersion = _strVerToHex(STRVER);
	errInfo->clientTime = teulTime() ;
	errInfo->hostTime = errInfo->clientTime - lastClientTime + lastHostTime;
	errInfo->hostIP = lastHostIP;
	errInfo->archiveWasCorrupt = archiveWasCorrupt;

	fprintf(f, "%u\n%u\n%u\n%u\n%u\n%u\n", errInfo->errorType, errInfo->clientVersion, errInfo->clientTime,
										errInfo->hostTime, errInfo->hostIP, errInfo->archiveWasCorrupt);

	if(errInfo->errorType == WW_ERR_EXCEPTION)
	{
		fprintf(f, "%u\n%u\n", errInfo->data.exceptInfo.exceptionCode, errInfo->data.exceptInfo.exceptionAddr);
		// write stack array and info
		for( int i=0; i< WWII_EXCEPTION_INFO::_STACK_LEN; i++)
		{
			if ( errInfo->data.exceptInfo.stackAddrs[i]==0)
			{
				break;
			}
			fprintf(f,"%u\n",errInfo->data.exceptInfo.stackAddrs[i]);
		}
		fprintf(f,"0\n"); // end marker

		if ( errInfo->data.exceptInfo.infoText[0] )
		{
			for ( int i=0; i<WWII_EXCEPTION_INFO::_STR_LEN; i++ )
			{
				if ( errInfo->data.exceptInfo.infoText[i] == '\n' )
				{
					errInfo->data.exceptInfo.infoText[i] = '|';
				}
				if ( errInfo->data.exceptInfo.infoText[i] == '\'' )
				{
					errInfo->data.exceptInfo.infoText[i] = ' ';
				}
			}
			fprintf(f,"%s\n",errInfo->data.exceptInfo.infoText);
		}
		else
		{
			fprintf(f,"\n");
		}
	}
	else if(errInfo->errorType == WW_ERR_PIEXIT)
	{
		_mangleString(errInfo->data.piExitInfo.srcFilename.string, 1);
		_mangleString(errInfo->data.piExitInfo.errorMessage.string, 1);
		fprintf(f, "%u\n%s\n%s\n", 	errInfo->data.piExitInfo.srcLine, errInfo->data.piExitInfo.srcFilename.string,
									errInfo->data.piExitInfo.errorMessage.string);
	}

#if defined(PROCESS_LIST_SUBMISSIONS)
	std::string peerNames = _recordPeers() ;
	fprintf(f, "Peers: %s\n", peerNames.c_str());
#endif

	fprintf(f, "\n%s\nThis file contains data on the last abnormal application termination that is used to submit crash information automatically to Playnet. Data solely describes the fault and does not contain any system/personal identification.\n", "$Id: wwiiClientMessages.cpp 3678 2010-05-18 08:10:04Z osmith@CRS $") ;

	fclose(f);

	return 0;
}

/************************************************************************************************
	wwiiSendLog_WWII_ERR_INFO
*************************************************************************************************/

extern SINT32 wwiiSendLog_WWII_ERR_INFO()
{
    FILE* f = _errorDumpFile("r");
	if(!f) return 0;

	WWII_ERROR_INFO errInfo;
	memset(&errInfo, 0x0, sizeof(errInfo));

	if(fscanf(f, "%u %u %u %u %u %u", (unsigned int*)&errInfo.errorType, &errInfo.clientVersion,
				 &errInfo.clientTime, &errInfo.hostTime, &errInfo.hostIP, &errInfo.archiveWasCorrupt) == 6)
	{
		if(errInfo.errorType == WW_ERR_EXCEPTION)
		{
			fscanf(f, "%u %u", &errInfo.data.exceptInfo.exceptionCode, &errInfo.data.exceptInfo.exceptionAddr);
			// read stack array and info
			// read stack until a 0 up to max of 10 items
			bool readlastzero = false;
			for( int i=0; i< WWII_EXCEPTION_INFO::_STACK_LEN; i++)
			{
				int cnt = fscanf(f,"%u", &errInfo.data.exceptInfo.stackAddrs[i]);
				if ( (cnt && errInfo.data.exceptInfo.stackAddrs[i]== 0) || cnt==0 )
				{
					readlastzero = true;
					break;
				}
			}
			char s[256];
			fgets(s, 10, f); // /n

			if ( ! readlastzero )
			{
				fgets(s, 10, f); // 0/n
				readlastzero = false;
			}

			fgets(errInfo.data.exceptInfo.infoText,WWII_EXCEPTION_INFO::_STR_LEN-1,f);
		}
		else if(errInfo.errorType == WW_ERR_PIEXIT)
		{
			fscanf(f, "%u %s %s", 	&errInfo.data.piExitInfo.srcLine, errInfo.data.piExitInfo.srcFilename.string,
									errInfo.data.piExitInfo.errorMessage.string);

			_mangleString(errInfo.data.piExitInfo.srcFilename.string, 0);
			_mangleString(errInfo.data.piExitInfo.errorMessage.string,0);

			errInfo.data.piExitInfo.srcFilename.length = strnlen(errInfo.data.piExitInfo.srcFilename.string, WWII_ERR_STRING::_MAX_LEN);
			errInfo.data.piExitInfo.errorMessage.length = strnlen(errInfo.data.piExitInfo.errorMessage.string, WWII_ERR_STRING::_MAX_LEN);
		}

		wwiiQueuedSend_WWII_ERROR_INFO(&errInfo);
	}

	fclose(f);

	deleteDumpFile();

	return 0;
}

/************************************************************************************************
	_getErrInfo
*************************************************************************************************/

static WWII_ERROR_INFO* _getErrInfo()
{
	int i;

	for(i = 0; i < WW_ERR_MAX; i++)
	{
		if(errInfoQueue[i].errorType == WW_ERR_UNKNOWN)
			return &errInfoQueue[i];
	}

	return NULL;
}

/************************************************************************************************
	_strVerToHex
*************************************************************************************************/

static UINT32 _strVerToHex(const char* inStr)
{
	UINT32 quad[4] ;
	UINT32 vers = 0;

	quad[0] = quad[1] = quad[2] = quad[3] = 0;

	sscanf(inStr, "%u.%u.%u.%u", &quad[0], &quad[1], &quad[2], &quad[3]);

	vers = (quad[3] << 24) | (quad[2] << 16) | (quad[1] << 8) | (quad[0]);

	return vers;
}

/************************************************************************************************
	_mangleString
*************************************************************************************************/

static void _mangleString(char* inStr, int underscore)
{
	char* s;
	char to   = (underscore) ? '_' : ' ';
	char from = (underscore) ? ' ' : '_';

	for(s = inStr;*s; s++)
	{
		if(*s == from)
		{
			*s = to;
		}
		else if (*s == '\\')
		{
			*s='|';
		}
	}
}
#endif // FRONT_END

SINT32
wwiiSendWWII_ERROR_INFO(SINT32 connection, WWII_ERROR_INFO* info)
{
	return teulSend(connection, _SEND_ERROR_INFO, info);
}

static SINT32
_sendPlayerLogin(ACTION_OBJECT* const a)
{
	PLAYER_LOGIN* const login = (PLAYER_LOGIN*)a->localRawData;
	return login->packer(TEUL_PACK, a->packed, a->offset) ;
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
	close() ;
}

//////////////////////////////////////////////////////////////////////
// Perform authentication on a connection so that we become properly
// logged in to the remote server.

void
Teul::Connection::login()
{
	PLAYER_LOGIN login ;
	login.attemptID = m_attemptID ;
	login.appVersion = m_playerIdent.m_clientVersion ;
	login.OTP = m_playerIdent.m_otp ;

	// Under normal operation, we just send a copyright string to the host;
	// the idea being that since traffic is encrypted it'll provide something
	// to test the packet stream. *shrug*
	if ( m_playerIdent.m_playerID == 0 )
		memcpy(login.CopyRightString, AUTH_SEQ, AUTH_SEQ_SIZE);
	else
	{
		if ( m_type == Map )
		{
			// If we're auth-bypassing into the map host, we send our playerID instead.
			snprintf((char*)login.CopyRightString, AUTH_SEQ_SIZE, "%u", m_playerIdent.m_playerID) ;
		}
		else
		{
			// For the other servers, we prefix it with an R. I'm not 100% sure why -- OGS.
			snprintf((char*)login.CopyRightString, AUTH_SEQ_SIZE, "R%u", m_playerIdent.m_playerID) ;
		}
	}

	teulSend(m_teulIndex, _PLAYER_LOGIN, &login) ;

	m_loggedIn = false ;
	m_started = teulClock64 ;
}

//////////////////////////////////////////////////////////////////////
// Close down a connection.

#include "teul/teulEndpoint.h"	// Here be dragons.

void
Teul::Connection::close()
{
	if ( m_teulIndex < 0 )
		return ;

	// Start by disassociating ourselves from the teulIndex,
	// so we don't create any infinite loops, but so that
	// the client can get a ECONNRESET report to invoke
	// any "close" handler it may need to call.
	const SINT32 teulIndex = m_teulIndex ;

	// Disassociate ourselves entirely.
	m_teulIndex = -1 ;
	_ConnectionsByTeulIndex.erase(teulIndex) ;
	teulSetPersonaData(teulIndex, NULL) ;

	// Look away, look away! Teul shenanigans!
	ENDPOINT* const ep = teulGetEndpoint(teulIndex) ;
	if ( ep != NULL && ep->m_inUse && ep->state() < IS_CLOSING )
		teulClose(*ep, Teul::Closing::ClientConnClose) ;
	// Ok, you can look again now.

	m_finished = m_disposable = true ;
}

//////////////////////////////////////////////////////////////////////
// Check connections for expiry.

void
Teul::Connection::expire()
{
	/// Maximum of 60 seconds per connection attempt.
	static const TeulTimeMS EXPIRY_TIME = 60 * 1000 ;
	const TeulTimeMS cutoff = teulClock64 - EXPIRY_TIME ;
	for ( std::map<SINT32, Teul::Connection*>::iterator it = _ConnectionsByTeulIndex.begin() ; it != _ConnectionsByTeulIndex.end() ; ++it )
	{
		Teul::Connection* conn = it->second ;
		if ( conn->m_disposable )
		{
			delete conn ;
			break ;
		}
		if ( conn->m_loggedIn == true )
			continue ;
		if ( conn->m_started <= EXPIRY_TIME )
		{
			conn->onLoginResult(-ETIMEDOUT) ;
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
	SINT32 newTeulIndex = teulOpenSend(m_serverAddr.m_hostIP, m_serverAddr.m_port, s_onConnect, (SINT32)m_timeoutMS, TCP, s_onError) ;
	if ( newTeulIndex == -1 )
		return false ;

	// Make sure there's not already a connection in progress
	// with the matching teulIndex.
	std::map<SINT32, Teul::Connection*>::iterator it = _ConnectionsByTeulIndex.find(newTeulIndex) ;
	if ( it != _ConnectionsByTeulIndex.end() )
	{
		// This is really bad; especially since it could
		// belong to a different client and such.
		assert( 0 && "Can't handle multiple connections on the same teulIndex" ) ;
	}

	// Stow the teulIndex, and attach the client we were passed
	// to the teulIndex's private data structure so we can look
	// it up again later.
	m_teulIndex = newTeulIndex ;
	m_connected = false ;
	teulSetPersonaData(m_teulIndex, client) ;

	_ConnectionsByTeulIndex[m_teulIndex] = this ;

	return true ;
}

/************************************************************************************************
	ClientAndConn
	Wrapper for looking up the pair { client, connection } for a given teulIndex.
*************************************************************************************************/

struct ClientAndConn
{
	ClientAndConn(SINT32 teulIndex)
		: client(NULL), conn(NULL)
	{
		ww2::Client* const thisClient = ww2::Client::FromTeulIndex(teulIndex) ;
		if ( thisClient == NULL )
			return ;

		std::map<SINT32, Teul::Connection*>::iterator it = _ConnectionsByTeulIndex.find(teulIndex) ;
		if ( it == _ConnectionsByTeulIndex.end() )
			return ;

		client = thisClient ;
		conn = it->second ;
	}

	ww2::Client*		client ;
	Teul::Connection*	conn ;
} ;

/************************************************************************************************
	Connection::s_connectHandler
	Base class static member for handling a connection success and forwarding it to the
	appropriate login attempt object.
*************************************************************************************************/

void
Teul::Connection::s_onConnect(ENDPOINT& endpoint)
{
	ClientAndConn ccpair(endpoint.teulIndex()) ;
	if ( ccpair.conn != NULL )
	{
		ccpair.conn->m_connected = true ;
		ccpair.conn->onConnect(ccpair.client) ;
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
		error = -error ;

	// Lookup the client wrappers for this connection.
	ClientAndConn ccpair(teulIndex) ;

	// If we just closed this connection down, teulKit will send
	// us an onError for a connection that no-longer has any
	// wrappers, thus conn will be null.
	if ( ccpair.conn != NULL )
	{
		ccpair.conn->onError(ccpair.client, error) ;
	}
}

void
Teul::Connection::onError(ww2::Client* client, SINT32 error)
{
	if ( error < 0 )
		error = -error ;

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
		return ;
	break ;

	// Stuff that means bad news.
	case ETEUL_ILLEGAL_PACKET_ORDER:
	case ENOTCONN:
	case ECONNRESET:
	case ECONNREFUSED:
	case ETIMEDOUT:
	case ECONNABORTED:
	case ENETRESET:
	default:
		m_finished = m_disposable = true ;
	break ;
	}

	clientErrorLog("t:%d got %s:%d", m_teulIndex, Teul::GetErrorName(error), error) ;
}

//////////////////////////////////////////////////////////////////////
// Handle a login result

void
Teul::Connection::onLoginResult(SINT32 result)
{
	ww2::Client* const client = ww2::Client::FromTeulIndex(m_teulIndex) ;
	if ( client != NULL )
	{
		if ( result >= 0 )
			onLogin(client) ;
		else
			onLoginFail(client, result) ;
	}

	m_finished = true ;
}

static SINT32
_recvLoginReply(ACTION_OBJECT* const a)
{
	// Get the attempt and result codes.
	UINT32 attemptID = 0 ;
	SINT32 result = -EINVAL ;

	a->packer(TEUL_UNPACK, attemptID) ;
	a->packer(TEUL_UNPACK, result) ;

	std::map<SINT32, Teul::Connection*>::iterator it = _ConnectionsByTeulIndex.find(a->teulIndex) ;
	if ( it == _ConnectionsByTeulIndex.end() )
		return -EINVAL ;

	it->second->onLoginResult(result) ;

	return a->offset ;
}

static SINT32
_registerAuthCallbacks()
{
	BEGIN_CALLBACK_REGISTRATION(Auth::CallbackX)
		REGISTER_SENDER(PLAYER_LOGIN_Y, _sendPlayerLogin, _PLAYER_LOGIN) ;
		REGISTER_RECEIVER(PLAYER_LOGIN_REPLY_Y, _recvLoginReply) ;
	END_CALLBACK_REGISTRATION
}

SINT32
wwiiClientInstallCallbacks()
{
	_registerAuthCallbacks() ;

	BEGIN_CALLBACK_REGISTRATION(WWII_X)

		REGISTER_RECEIVER(WWII_ACTIVATE_Y, _recvWWII_ACTIVATE) ;
		REGISTER_RECEIVER(WWII_DIALOG_Y, _recvWWII_DIALOG) ;

		REGISTER_SENDER(WWII_ERROR_INFO_Y, _sendWWII_ERROR_INFO, _SEND_ERROR_INFO) ;
		REGISTER_SENDER_NODATA(WWII_USER_ABORTED_APP_Y, _SEND_USER_ABORTED_APP) ;

	END_CALLBACK_REGISTRATION
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
static UINT32 _hitsPrime = 127979 ;

UINT32 wwiiHitsPrime()
{
	return _hitsPrime;
}

bool wwiiIncrementHits()
{
	static const int ARBITRARY_NUM_UPDATES = 199 ;	// Connecting grace period
	if ( _updatesSent < (UINT32)ARBITRARY_NUM_UPDATES
	|| (rand() % _hitsPrime) != 0 )
		return false ;
	// Each change increases the rate of change
	static const int ARBITRARY_MIN_HITS_PRIME = 32801 ;
	static const int ABRITRARY_DECREMENT_PRIME = 311 ;
	if ( _hitsPrime > (UINT32)ARBITRARY_MIN_HITS_PRIME )
		_hitsPrime -= rand() % ABRITRARY_DECREMENT_PRIME ;
	return true ;
}

#if defined(FRONT_END) && !defined(CMDLINE_CLIENT)
/************************************************************************************************
	wwiiServiceQueue_WWII_ERROR_INFO
*************************************************************************************************/

extern SINT32
wwiiServiceQueue_WWII_ERROR_INFO()
{
	int i;
	if(PlayerClient()->MapConnection() < 0) return 0;// || !g_bMapClientActive) return 0;

	for(i = 0; i < WW_ERR_MAX; i++ )
	{
		if(errInfoQueue[i].errorType == WW_ERR_UNKNOWN) break;
		wwiiSendWWII_ERROR_INFO(PlayerClient()->MapConnection(), &errInfoQueue[i]);
	}

	memset(errInfoQueue, 0x0, sizeof(errInfoQueue));
	return 0;
}

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
	const FULL_OBJ_ID funny = { { withCode, -withCode }, 42, 0, 0 } ;

	chatSendLocality(PlayerClient()->ChatConnection(), 0, 39, 40, 41, funny) ;
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

