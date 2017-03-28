// $Id: Playgate.cpp 486 2010-05-24 22:16:57Z rampster@CRS $
// Playgate.cpp : Defines the class behaviors for the application.
//

#include "Playgate.h"
#include "PlaygateDlg.h"
#include "SingleInstanceApp.h"		// For CLimitSingleInstance

#include "main.h"
#include "registry.h"
#include "teul/teulBasic.h"
#include "comm/commPlaygateMessages.h"
#include "ftp.h"
#include "TermsOfServiceDlg.h"
#include "RegionSelect.h"
#include "PlayerLogon.h"
#include "LoginData.h"
#include "pnRegion.h"
#include "launchApp.h"
#include "wwii/wwiiUTF.h"
#include "../WW2/src/auth/authAppIDs.h"
#include "steam_api.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Global variables

HINSTANCE	hMainInstance;
CBDialog*	hMainWnd;

HANDLE		hStatusMutex;
HANDLE		hExitMutex;
HANDLE		hApplicationMutex;
HANDLE		hLogonAttemptMutex;
bool		g_tryLogonAttempt = false;

std::wstring g_dataPath, g_exePath;
std::wstring g_exeName = L"ww2.exe", g_exeParams;

UINT32 g_gameAppID;
#ifndef IGNORE_REGISTRY
char g_registryKey[256]; // Where the registry values are located.
wchar_t g_registryKeyW[256];
#endif
UINT32 g_playgateVersion;
SINT32 g_arenaID, g_sessionID;

char g_authHostName[256] = "";

char szVersion[] = PG_VER_STRING;
char szVersionString[] = PG_COPYRIGHT;

IP_LIST	g_authHosts;
UINT16	g_authPort;
SINT32 webID; //used for web downloads
UINT32 webURL;

CPlaygateApp playgate;

static PlaygateState pgState;

//////////////////////////////////////////////////////////////////////
// Local variables

// Single-Instance-At-a-Time encapsulation
static CLimitSingleInstance g_SingleInstanceObj(TEXT("Global\\{0DC91C4C-AE4F-43d3-A007-EF89A8EBC432}"));

//////////////////////////////////////////////////////////////////////
// CPlaygateApp

BEGIN_MESSAGE_MAP(CPlaygateApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()

static const int STEAM_APP_ID = 251950;
bool steamInitialized = false;

// CPlaygateApp construction

CPlaygateApp::CPlaygateApp()
: m_inputAllowed(true)
, m_nextGameListRequest(0)
, m_playnet(false)
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only CPlaygateApp object

static bool
_DoDialog(CBDialog& dlg)
{
	hMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	hMainWnd = nullptr;
	return(nResponse == IDOK);
}

static bool
_TosDialog()
{
	CTermsOfServiceDlg dlg;
	return _DoDialog(dlg);
}

static bool
_Login()
{
	CPlayerLogon dlg;
	return _DoDialog(dlg);
}

static bool
_ServerSelect()
{
	CPlaygateDlg dlg;
	return _DoDialog(dlg);
}

static void
_setInput(bool enable)
{
	CPlayerLogon* logon = (CPlayerLogon*)hMainWnd;
	logon->ToggleInput(enable, enable);
}

static void
_attemptConnection()
{
	if (loginData.m_steamAccountId == 0)
		_setInput(false);

	bool connected = false;
	bool firstTime = true;
	static bool firstAttempt = true;
	static UINT32 attempt = 0;

	while ( connected == false )
	{
		if ( firstAttempt == true )
			firstAttempt = false;
		else
		{
			// Report and retry in 6 seconds.
			playgate.SetStatus(GetLoc(ERROR_CONNECT), AppCaution);
			Sleep(6 * 1000);
		}

		// The first connection attempt often fails, so use logic-shortcircuit
		// to slip a second try in if the first fails.
		if ( launchOpenConnection() || launchOpenConnection() )
		{
			connected = true;
			break;
		}

		// We were unable to open a connection.
		if ( firstTime )
		{
			// This is the first time thru this loop, so inform the user we are
			// having trouble and give them a chance to confirm whether they want
			// us to continue trying.
			// This is mostly to let the user know we were having problems.
			if ( MessageBox(nullptr, GetLoc(ERROR_CONNECTION), GetLoc(AUTH_ERROR), MB_ICONEXCLAMATION|MB_YESNO) != IDYES )
			{
				// They said "no" to our "do you want to continue trying" question,
				// shutdown.
				playgate.StartBrowser(ERR_AUTH_CONNECT);
				playgate.Quit();
				ExitThread(0);
				return;
			}

			firstTime = false;
		}
	}

	// Ok, now you can start typing stuff.
	if (loginData.m_steamAccountId == 0)
		_setInput(true);
}

static DWORD WINAPI threadFunc(LPVOID dummyParam)
{
	if ( WaitForSingleObject(hApplicationMutex, INFINITE) == WAIT_ABANDONED )
		return 0;
	// Yield processing
	Sleep(0);
	ReleaseMutex(hApplicationMutex);
	// Yield processing
	Sleep(0);

	bool connected = false; 

	while ( WaitForSingleObject(hExitMutex, (connected ? 25 : 1)) == WAIT_TIMEOUT )
	{
		// Allow teul to do its stuff.
		if ( !commUpdate() )
		{
			// If we've already logged in, we're going to have to
			// bail out.
			if ( connected == true && pgState != PgAuthentication )
			{
				playgate.AlertOK(GetLoc(AUTH_LOST));
				playgate.Quit();
				ExitThread(0);
			}

			_attemptConnection();

			connected = true;

			continue;
		}

		// Check to see if the main thread has tasked us with
		// performing a login attempt.
		if ( WaitForSingleObject(hLogonAttemptMutex, 0) != WAIT_TIMEOUT )
		{
			bool tryAttempt = g_tryLogonAttempt;
			g_tryLogonAttempt = false;
			ReleaseMutex(hLogonAttemptMutex);
			if ( tryAttempt == true )
			{
				if ( pgState == PgAuthentication )
					logonAttempt();
				else if ( pgState == PgServerSelect )
					launchArenaRequest();
			}
		}

		if ( 0 != playgate.m_nextGameListRequest && teulTime() >= playgate.m_nextGameListRequest )
		{
			playgate.m_nextGameListRequest = teulTime() + 30;
			launchRequestGameList();
		}

		std::this_thread::yield();
	}

	ReleaseMutex(hExitMutex);

	ExitThread(0);
}


// CPlaygateApp initialization
extern char g_authHostName[256] ;

namespace {
	class CommandLineParser : public CCommandLineInfo {
	public:
		virtual void ParseParam(LPCTSTR lpszParam, BOOL bFlag, BOOL bLast) {
			// HACK: Not being particularly robust since only devs & GMs will ever use this.

			std::wstring param(lpszParam);

			std::wstring var(param.begin(), param.begin()+param.find_first_of('='));
			std::wstring val(param.begin()+param.find_first_of('=')+1, param.end());

            if (var == L"authserver")
                WWII::ConvertChars(val.c_str(), g_authHostName, sizeof(g_authHostName));
			else if ( var == L"path" )
				g_exePath = val;
			else if ( var == L"data" )
				g_dataPath = val;
			else if ( var == L"game" )
				g_exeName = val;
			else if ( var == L"params" )
				g_exeParams = val;
			else {
				std::wstring msg(L"Invalid parameter \'");
				msg += lpszParam;
				msg += L"\' passed into Playgate.";
				MessageBox(nullptr, L"Invalid Parameters", msg.c_str(), MB_ICONEXCLAMATION | MB_OK);
				exit(0);
			}
		}
	} ;
}

BOOL CPlaygateApp::InitInstance()
{
	CommandLineParser cmdParser ;
	ParseCommandLine(cmdParser) ;

	// OGS: First things first, determine what kind of client we are.
	// If the version number ends in .255 we are appID 1, otherwise we're beta.
	g_gameAppID = Auth::Apps::LIVE_APP_ID;
#ifndef IGNORE_REGISTRY
	if (strstr(STRVER, ".255") == nullptr)
		g_gameAppID = Auth::Apps::BETA_APP_ID;
	_snprintf(g_registryKey, 255, "Software\\Playnet\\apps\\appid%u\\instance1", g_gameAppID);
	WWII::ConvertChars(g_registryKey, g_registryKeyW, sizeof(g_registryKeyW));
#endif

	LocInit();
	
	if (g_SingleInstanceObj.IsAnotherInstanceRunning())
	{
		MessageBox(nullptr, GetLoc(ERROR_PLAYGATE), GetLoc(APP_TITLE), MB_ICONEXCLAMATION|MB_OK);
		return FALSE;
	}

	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();

	hMainInstance = AfxGetInstanceHandle();

	if (!AfxSocketInit())
	{
		AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
		return FALSE;
	}

	AfxEnableControlContainer();
	AfxInitRichEdit2();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T("Playnet"));

	// Turn our version number into a string.
	teulGetHost(PLAY_GATE_VERSION, &g_playgateVersion);

	// Move ourselves to the playgate home
	if ( registryGetPaths(g_exePath, g_dataPath) == false )
	{
		dbugLog(_FLA, "Unable to retrieve paths from registry.");
		MessageBox(nullptr, GetLoc(ERROR_INSTALL), GetLoc(ERROR_INSTALL_T), MB_ICONEXCLAMATION | MB_OK);
		exit(0);
	}

	_tchdir(g_dataPath.c_str());

	commInit();

	registryGetPaths(g_exePath, g_dataPath);
	dbugLog(_FLM, "Playgate version %s. Command Line: %s. ExePath: %ls. DataPath: %ls.", PG_COPYRIGHT, m_lpCmdLine, g_exePath.c_str(), g_dataPath.c_str());

	if ( _tchdir(g_exePath.c_str()) != 0 )
	{
		MessageBox(nullptr, GetLoc(ERROR_APP_FOLDER, g_exePath.c_str()), GetLoc(ERROR_INSTALL_T), MB_ICONEXCLAMATION | MB_OK);
		exit(0);
	}

#ifndef DEBUG_WW2
	if (!SteamAPI_RestartAppIfNecessary(STEAM_APP_ID))
	{
#endif

		// If they've logged in before, recover the data.
		loginData.Read();
		// Make sure it's not just or something.
		loginData.Validate();

		pgState = PgTOS;

		// Load the "region"s data.
		dbugLog(_FLI, "Regions");
		if ( !CRegionSelect::ReadRegionsFile() )
			return FALSE;

		// Show the Terms of Service file
		// (%exepath%\\data\\playgate\\tos.rtf)
		dbugLog(_FLI, "TOS");
		if ( !_TosDialog() )
			return FALSE;

		pgState = PgRegionSelection;

		// Prompt the user to select their region.
		dbugLog(_FLI, "Regions follow-up");
		if ( !RegionSelect() )
			return FALSE;

		// Update the region
		const wchar_t* regionName = g_region.name().c_str();
		if ( wcscmp(regionName, loginData.m_region) != 0 )
		{
			// Arena will change
			loginData.m_arenaID = 0;
			wcsncpy_s(loginData.m_region, regionName, sizeof(loginData.m_region));
			loginData.MarkForWrite();
		}

		// Verify access to the data directory.
		if ( _tchdir(g_dataPath.c_str()) != 0 )
		{
			dbugLog(_FLA, "Unable to access datapath [%ls]", g_dataPath.c_str());
			MessageBox(nullptr, GetLoc(ERROR_DATA_FOLDER, g_dataPath.c_str()), GetLoc(ERROR_INSTALL_T), MB_ICONEXCLAMATION | MB_OK);
			return FALSE;
		}

		hStatusMutex = CreateMutex(nullptr, FALSE, nullptr);
		hExitMutex = CreateMutex(nullptr, TRUE, nullptr);
		hApplicationMutex = CreateMutex(nullptr, TRUE, nullptr);

		pgState = PgAuthentication;

		DWORD threadId;
		::CreateThread(nullptr, 0, threadFunc, nullptr,0, &threadId);

		// let app close and restart if it must start through steam
		if (SteamAPI_Init())
		{
			if (SteamApps()->BIsAppInstalled(STEAM_APP_ID))
			{
				loginData.m_steamAccountId = SteamApps()->GetAppOwner().GetAccountID();
			}
		}

		if (loginData.m_steamAccountId == 0)
		{
			if (_Login())
			{
				loginData.MarkForWrite();

				pgState = PgServerSelect;

				if (_ServerSelect())
					loginData.MarkForWrite();
			}
		}
		else
		{
			pgState = PgAuthentication;
			playgate.QueueLogonAttempt();

			ReleaseMutex(hApplicationMutex);

			_ServerSelect();
		}

#ifndef DEBUG_WW2
	}
#endif

	pgState = PgFinished;

	ReleaseMutex(hExitMutex);

	Sleep(100);

	CloseHandle(hStatusMutex);
	CloseHandle(hLogonAttemptMutex);
	CloseHandle(hExitMutex);

	teulCloseTransport();

	commKill();

	return FALSE;
}

void
CPlaygateApp::SetState(PlaygateState state)
{
	pgState = state;
}

void
CPlaygateApp::SetStatus(const TCHAR* message, StatusType status) const
{
	if ( !hMainWnd ) return;
	// Lock the mutex so that we don't have multiple threads
	// trying to write to the same status object.
	WaitForSingleObject(hStatusMutex, INFINITE);
	if ( hMainWnd )
	{
		if (hMainWnd->m_hint.IsWindowVisible())
		{
			hMainWnd->m_hint.SetWindowText(message);
			hMainWnd->m_hint.SetTextColor((int)status);
		}
	}
	ReleaseMutex(hStatusMutex);
	Sleep(250);
}

void CPlaygateApp::Quit()
{
	// Stop anything the FTP connection is doing
	ftpQuit();

	// Post a message to the main window to tell it to shut down
	if ( hMainWnd )
		hMainWnd->SetTimer('QUIT', 100, nullptr);
}

static bool
_getDefaultBrowser(TCHAR* browserName, LONG nameSize)
{
	int res = RegQueryValue(HKEY_CLASSES_ROOT, _T("http\\shell\\open\\command")
							, browserName
							, &nameSize);
	if ( res != ERROR_SUCCESS )
		return false;

	TCHAR* p = browserName;
	// Normally the command is quoted
	if ( *p == '"' )
	{
		// Its quoted, find the end quote and snip.
		p = _tcschr(p + 1, '"');
		if ( p != nullptr )
			*(p + 1) = 0;
	}
	else
	{
		// Otherwise, snip at the first space
		p = _tcschr(browserName, ' ');
		if ( p != nullptr )
			*p = 0;
	}

	return true;
}

void
CPlaygateApp::StartBrowser(std::string WebPage)
{
	TCHAR browserName[256];
	if ( !_getDefaultBrowser(browserName, sizeof(browserName)) )
		return;

	TCHAR parameters[512];
	if ( WebPage[0] == '@' )
	{
		_stprintf(parameters
					, L"http://www%hs/%hs"
						, g_region.webSuffix().c_str()
						, WebPage.c_str() + 1);
	}
	else
	{
		_stprintf(parameters, L"%hs", WebPage.c_str());
	}

	ShellExecute(nullptr, _T("open"), parameters, nullptr, nullptr, SW_SHOWNORMAL);

	return;
}

void
CPlaygateApp::QueueLogonAttempt()
{
	WaitForSingleObject(hLogonAttemptMutex, INFINITE);
	g_tryLogonAttempt = true;
	ReleaseMutex(hLogonAttemptMutex);
}

void
CPlaygateApp::ftping(bool actively)
{
	if ( playgate.m_progress )
	{
		playgate.m_progress->ShowWindow(actively);
//		playgate.m_progress->EnableWindow(actively);
	}

	if ( actively )
	{
		playgate.m_progress->SetRange(0, 100);
		m_inputAllowed = false;
		hMainWnd->SetTimer('FTP', 50, nullptr);
	}
	else
	{
		m_inputAllowed = true;
		hMainWnd->KillTimer('FTP');
	}

	hMainWnd->GetDlgItem(IDC_SERVER_LIST)->EnableWindow(!actively);
	hMainWnd->GetDlgItem(IDOK)->EnableWindow(!actively);
}

void CPlaygateApp::SetWindowText(CWnd* wnd, const char* text)
{
	if ( wnd != nullptr )
	{
		wchar_t buffer[512];
		WWII::ConvertChars(text, buffer, sizeof(buffer));
		wnd->SetWindowTextW(buffer);
	}
}

//////////////////////////////////////////////////////////////////////
// Load a bitmap from disk

HBITMAP pgLoadImage(const std::wstring& filename)
{
	std::wstring fullName(g_exePath.begin(), g_exePath.end());
	fullName += L"\\data\\playgate\\";
	fullName += filename;
	return (HBITMAP)::LoadImage(hMainInstance, fullName.c_str(), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION | LR_LOADFROMFILE);
}
