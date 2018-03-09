// $Id: launchApp.cpp 263 2009-07-06 22:50:28Z dchristopher@CRS $

#include "auth/authConstants.h"
#include "auth/authAppIDs.h"
#include "auth/authPlaygateTypes.h"
#include "comm/commPlaygateMessages.h"
#include "launchApp.h"
#include "launchAppWin.h"
#include "auth/authPack.h"
#include "teul/teulAuth.h"
#include "logon.h"
#include "versionWin.h"
#include "ftp.h"
#include "md5.h"
#include "LoginData.h"
#include "Playgate.h"

#ifdef IGNORE_REGISTRY
#include "wwii/wwiiUTF.h"

// HACK: In versionWin.cpp
extern bool GetFileVersion(TCHAR* const dest, const size_t destSize, const TCHAR* const filename);
// HACK: launchAppWin.cpp
extern SINT32 _FillInParamValues(const APP_LAUNCH *AppLaunch, const TCHAR* path, const TCHAR* Params, TCHAR* FilledInParams, size_t FIPSize);
bool launchApplication(const TCHAR* exeFile, const TCHAR* parameters, std::wstring path);
#endif

#define		MAX_REG_KEY		256
#define		MAX_URL			256

static void _CleanUpLaunchData(void);

//  static functions
static SINT32 _InstallCallbacks(void);
static SINT32 _SendArenaReq(ACTION_OBJECT *a);
static SINT32 _ReceiveAppLaunch(ACTION_OBJECT *Action);
static SINT32 _ReceiveAppVerify(ACTION_OBJECT *Action);
static SINT32 _SendLaunchReq(ACTION_OBJECT *Action);
static SINT32 _errorFunc(ACTION_OBJECT *Action);

//  static variables
static TEUL_CALLBACK*  _LoginReq;
static TEUL_CALLBACK*  _ArenaReq;
static TEUL_CALLBACK*  _ArenaReq2;
static TEUL_CALLBACK*  _LaunchReq;
static TEUL_CALLBACK*  _GetGameList;
static TEUL_CALLBACK*  _LoginXchg;

static APP_LAUNCH _AppLaunch;
#ifndef IGNORE_REGISTRY
static char		_SavedRegistryKey[MAX_REG_KEY];
#endif
static UINT32     _NumberOfOldVersions;
static char      _DownloadPage[MAX_URL];
const char* userName;

char  ftpURL[MAX_URL+1];
APP_LAUNCH  AppLaunch;

bool launchOpenConnection()
{
	static int attempt = 0;

	++attempt;
	playgate.SetStatus(GetLoc(AUTH_CONNECTING, g_authHostName, attempt), AppNote);

	for ( UINT32 i = 0; i < g_authHosts.size(); ++i )
	{
		if ( commOpenConnection(g_authHosts[i], g_authPort) )
		{
			playgate.SetStatus(GetLoc(AUTH_CONNECTED, g_authHostName), AppProgress);
			return true;
		}
	}

	playgate.SetStatus(GetLoc(AUTH_NO_CONNECT, dbugGetErrno()), AppError);

	return false;
}

void
Auth::LoginRequest::send()
{
	commSend(_LoginXchg, this);
}

LOGICAL launchArenaRequest()
{
	ARENA_REQ ArenaRequest;

	memset(&ArenaRequest, 0, sizeof(ArenaRequest));

	ArenaRequest.arenaID = loginData.m_arenaID;
	ArenaRequest.sessionID = 0;
	ArenaRequest.presenterVersion = g_playgateVersion;
	loginData.GetUsernameAsUTF8(ArenaRequest.username, sizeof(ArenaRequest.username));
	loginData.GetPasswordMD5(ArenaRequest.password, MAX_PASSWORD);

	if ( commSend(_ArenaReq2, &ArenaRequest) < 0 )
	{
		playgate.SetStatus(GetLoc(AUTH_ARENA_FAIL), AppError);
		return FALSE;
	}

//	playgate.SetStatus(_T("Sent Arena Request"));

	return TRUE;
}

static SINT32 _ReceivePlayGateOk(ACTION_OBJECT *a)
{
	playgate.SetStatus(GetLoc(AUTH_OK));
/*
	if (launchGetPassword(0, nullptr))
		launchArenaRequest(loginData.m_arenaID, g_sessionID);
*/

	return a->offset;
}

SINT32 launchAppVerify(SINT32 Connection, APP_VERIFY *AppVerify)
{
APP_LAUNCH_REQ LaunchRequest;
#ifndef IGNORE_REGISTRY
SINT32         NumberOfInstances;
UINT32         CurrentVer;
#endif

	loginData.GetUsernameAsUTF8(LaunchRequest.username, sizeof(LaunchRequest.username));
	loginData.GetPasswordMD5(LaunchRequest.password, sizeof(LaunchRequest.password) - 1);

	LaunchRequest.status = 0;
	LaunchRequest.cdKeySize = 0;
	LaunchRequest.arenaID = AppVerify->arenaID;
	LaunchRequest.sessionID = AppVerify->sessionID;
	LaunchRequest.appID = AppVerify->appID;
	LaunchRequest.appVersion = 0;//AppVerify->appVersionMax;
	LaunchRequest.oVersion = 2;

#ifdef IGNORE_REGISTRY
	// Access Window's version information for the executable.
	TCHAR buffer[256+1];
	if ( !GetFileVersion(buffer, sizeof(buffer), (PG_exePath + g_exeName).c_str()) ) {
		LaunchRequest.status = -1;
		return 0;
	}

	char verStr[25 + 1];
	WWII::ConvertChars(buffer, verStr, sizeof(verStr));
	teulGetHost(verStr, &LaunchRequest.appVersion);

	if ( commSend(_LaunchReq, &LaunchRequest) < 0 ) {
		playgate.SetStatus(GetLoc(AUTH_LAUNCH_FAIL), AppError);
		return 0;
	}

	return 1;
#else
	_NumberOfOldVersions = 0;

	if (_SavedRegistryKey[0] != '\0')
		//  User has already been through selection process.  The saved key
		//  will indicate which instance to run
		NumberOfInstances = 1;
	else
		//  First time through.  User hasn't selected an instance of the
		//  game to run.
		NumberOfInstances = versionWinGetVersion(AppVerify->appID, 
											 AppVerify->appVersionMax,
											 &CurrentVer,
											 _SavedRegistryKey);

	//  NumberOfInstances will return both the number of matching versions
	//  AND the number of older versions.
	switch (NumberOfInstances)
	{
		case 0:  // Uh-oh. Registry's messed up, no version, fatal error -
				 //   send them to download page to get fixed.
		{
//			mainWinMessage(_T("No current version has been installed."));
//			httpBringUpWebPage("@bv/download.jsp");
//			break;
			LaunchRequest.status = -1;
		}
		case 1:  // Everything's cool - only one up-to-date copy on user's box
		default:
			LaunchRequest.appVersion = CurrentVer;//AppVerify->appVersionMax;
			if ( commSend(_LaunchReq, &LaunchRequest) < 0 )
			{
				playgate.SetStatus(GetLoc(AUTH_LAUNCH_FAIL), AppError);
				return 0;
			}
			break;
	}

	return 1;
#endif
}

bool launchRequestGameList()
{
	UINT32 dummy = 0;
	if ( commSend(_GetGameList, &dummy) < 0 )
	{
		playgate.SetStatus(GetLoc(AUTH_SERVER_FAIL), AppError);
		return false;
	}

	return true;
}

static SINT32 _RequestGameList(ACTION_OBJECT* a)
{
	return a->offset;
}

static SINT32 _ReceiveGameList(ACTION_OBJECT *a)
{
	Auth::g_games.packer(TEUL_UNPACK, a);
	if ( a->offset >= 0 )
		Auth::g_games.process();
	return a->offset;
}

static SINT32 _SendArenaReq2(ACTION_OBJECT *a)
{
	//StatusSetStatus("Sending arena request.");
	return authPackARENA_REQ2(TEUL_PACK, a->packed, a->offset,(ARENA_REQ *)a->localRawData, 0);
}

static SINT32 _SendArenaReq(ACTION_OBJECT *a)
{
	//StatusSetStatus("Sending arena request.");
	return authPackARENA_REQ(TEUL_PACK, a->packed, a->offset,(ARENA_REQ *)a->localRawData, 0);
}

static SINT32 _SendLaunchReq(ACTION_OBJECT *a)
{
	APP_LAUNCH_REQ* AppLaunchReq = (APP_LAUNCH_REQ*)a->localRawData;

	AppLaunchReq->status = 0;

	//StatusSetStatus("Sending launch request.");
	return authPackAPP_LAUNCH_REQ2(TEUL_PACK, a->packed, a->offset,AppLaunchReq, 0);
}

static SINT32 _SendLoginRequest(ACTION_OBJECT* a)
{
	return ((Auth::LoginRequest*)a->localRawData)->packer(TEUL_PACK, a);
}

static SINT32 _ReceiveAppLaunch(ACTION_OBJECT *Action)
{
	Action->offset = authPackAPP_LAUNCH(TEUL_UNPACK, Action->packed, Action->offset, &AppLaunch, 0);

	switch(AppLaunch.appID)
	{
	case Auth::Apps::LOGIN_DENIED:

		strcpy_s(_DownloadPage, AppLaunch.data);
		playgate.PostCommand(MSG_DOWNLOAD, (UINT32)_DownloadPage);
		_CleanUpLaunchData();
		playgate.Quit();

		return 0;

	break;

	case Auth::Apps::BROWSER:
	case Auth::Apps::WEB_DOWNLOAD:
		{
		SINT32 len;
		SCHAR *ftpData;

			len = sizeof(_DownloadPage);
			memset(_DownloadPage,0,sizeof(_DownloadPage));
			memset(ftpURL,0,sizeof(ftpURL));

			strcpy_s(_DownloadPage, AppLaunch.data);
			len = strlen(_DownloadPage);
			if (AppLaunch.dataSize > len)
			{
				ftpData = AppLaunch.data + len + 1;
				
				strcpy_s(ftpURL, ftpData);
			}
//			else
//				strcpy(_DownloadPage, "www." AUTH_DOMAIN "/bv/download.jsp");

			//winsSendMessage(MSG_DOWNLOAD, (UINT32)_DownloadPage);
			webID = MSG_DOWNLOAD;
			webURL = (UINT32)_DownloadPage;
			playgate.SetStatus(GetLoc(UPDATING), AppProgress);
			ftpStart();

			return 0;
		}
	case Auth::Apps::WEB_REDIRECT:
		{
			strcpy_s(_DownloadPage, AppLaunch.data);
			playgate.PostCommand(MSG_DOWNLOAD, (UINT32)_DownloadPage);

			_CleanUpLaunchData();

			playgate.Quit();

			return 0;
		}
	}

	playgate.SetStatus(GetLoc(INFO_LAUNCH), AppProgress);

#ifdef IGNORE_REGISTRY
	std::wstring param_pattern = L"-serverip $QUAD -serverport $PORT -serverotp $OTP -side $SESSIONID";
	TCHAR c_params[500]; // HACK
	_FillInParamValues(&AppLaunch, PG_exePath.c_str(), param_pattern.c_str(), c_params, sizeof(c_params) / sizeof(TCHAR));
	std::wstring params(c_params + (L" " + g_exeParams));

	launchApplication((PG_exePath + L"/" + g_exeName).c_str(), params.c_str(), PG_exePath.c_str());
#else
	launchAppWin(&AppLaunch, _SavedRegistryKey);
#endif

	_CleanUpLaunchData();

	playgate.Quit();

	return 0;
}

static SINT32 _ReceiveAppVerify(ACTION_OBJECT *Action)
{
	APP_VERIFY AppVerify;

	//playgate.SetStatus("Received app verify");

	Action->offset = authPackAPP_VERIFY(TEUL_UNPACK, Action->packed, Action->offset, &AppVerify, 0);

	if (!launchAppVerify(Action->teulIndex, &AppVerify))
	{
		//_CleanUpLaunchData();
		playgate.SetStatus(GetLoc(INFO_ABORTED), AppError);
		return 0;
	}

	return Action->offset;
}

static SINT32 _RecvLoginReply(ACTION_OBJECT* a)
{
	Auth::LoginReply reply(a);
	if ( a->offset >= 0 )
		reply.process();
	return a->offset;
}

extern SINT32 launchInstallCallbacks(void)
{
	try
	{
		_ArenaReq = commInstallMessage(Auth::CallbackX, ARENA_REQ_Y, _SendArenaReq, nullptr);
		if ( _ArenaReq == nullptr )
			throw 1;
		_ArenaReq2 = commInstallMessage(Auth::CallbackX, ARENA_REQ2_Y, _SendArenaReq2, nullptr);
		if ( _ArenaReq2 == nullptr )
			throw 2;
		_LaunchReq = commInstallMessage(Auth::CallbackX, APP_LAUNCH_REQ_Y, _SendLaunchReq, _errorFunc);
		if ( _LaunchReq == nullptr )
			throw 3;
		if ( commInstallMessage(Auth::CallbackX, APP_LAUNCH_Y, nullptr, _ReceiveAppLaunch) == nullptr )
			throw 4;
		if ( commInstallMessage(Auth::CallbackX, APP_VERIFY_Y, nullptr, _ReceiveAppVerify) == nullptr)
			throw 5;
		if ( commInstallMessage(Auth::CallbackX, APP_PLAYGATE_OK_Y, nullptr, _ReceivePlayGateOk) == nullptr )
			throw 6;
		_GetGameList = commInstallMessage(Auth::CallbackX, APP_XCHG_GAME_LIST_Y, _RequestGameList, _ReceiveGameList);
		if ( nullptr == _GetGameList )
			throw 7;
		_LoginXchg = commInstallMessage(Auth::CallbackX, AUTH_XCHG_LOGIN_Y, _SendLoginRequest, _RecvLoginReply);
		if ( nullptr == _LoginXchg )
			throw 8;
	}
	catch ( int n )
	{
		playgate.SetStatus(GetLoc(COMMS_FAILED, n), AppError);
		return 0;
	}

	return 1;
}

static void _CleanUpLaunchData(void)
{
#ifndef IGNORE_REGISTRY
	_SavedRegistryKey[0] = '\0';
#endif
}

static SINT32 _errorFunc(ACTION_OBJECT *Action)
{
	return 1;
}
