// $Id: logon.cpp 102 2009-01-09 20:42:45Z osmith@CRS $
// Logon UI

// ------------------------------ Includes
#include "commctrl.h"
#include "logon.h"
#include "md5.h"
#include "arenaLockEnums.h"
#include "launchApp.h"
#include "pnRegion.h"
#include "authPlaygateTypes.h"
#include "wwii/wwiiUTF.h"
#include "LoginData.h"

//#include "MouseDrag.h"

#include <stdexcept>
#include <wchar.h>

// ------------------------------ Data

RECT   DialogRect;
BITMAP g_GameIcon;

HBITMAP launchBmp, nolaunchBmp ;

BOOL	g_inputEnabled = TRUE ;
bool	g_useChinese = false ;

static TCHAR g_szTOS[] = L"tos.txt";

//static SINT32 _LoadTOSListBox(HWND hWnd);

// We store an array of password mask characters so we can
// tell how long the password is... Playgate legacy.
static wchar_t g_passMask[MAX_PASSWORD_LENGTH + 1];
// We don't keep the actual password, just the MD5 sum.
static UCHAR g_passMD5[64+1];

// Regions define the domains we operate within,
// the default is to assume we are running Battleground Europe
// for playnet.com, but otherwise we obtain the data from
// the regions.txt file.
typedef std::vector<PNRegion> PN_REGIONS ;
static PN_REGIONS g_regions ;
PNRegion g_region(L"Playnet", ".playnet.com", ".battlegroundeurope.com") ;

UINT32 g_nextGameListRequest ;
Auth::GameList Auth::g_games ;

//////////////////////////////////////////////////////////////////////
// Server clusters

static const int AL_UNKNOWN = -1000 ;

static HWND serverList ;
static HWND playButton ; 

static const int NUM_LOCK_STATES = 5 ;
const char* states[NUM_LOCK_STATES] =
{
	"Online"
,	"Offline"
,	"Closed"
,	"Locked"
,	"Loading"
} ;

//////////////////////////////////////////////////////////////////////
// Load a bitmap into a button control

void LoadBmpToButton(HWND hWnd, int buttonID, int bmpID)
{
	HWND button = GetDlgItem(hWnd, buttonID) ;
	HBITMAP bmp = LoadBitmap(hMainInstance, MAKEINTRESOURCE(bmpID)) ;
	SendMessage(button, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)bmp) ;
}

static void _setItemText(HWND hWnd, int itemID, const wchar_t* text)
{
	HWND item = GetDlgItem(hWnd, itemID) ;
	if ( item )
		SetWindowTextW(item, text) ;
}

//////////////////////////////////////////////////////////////////////
// Region Selection dialog.

BOOL CALLBACK RegionProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
//	MouseDragProc(hWnd, Msg, wParam, lParam) ;

	switch (Msg)
	{
		case WM_INITDIALOG :
		{
			mainSetIcon(hWnd) ;

			if ( g_useChinese == true )
			{
				SetWindowTextW(hWnd, L"请选择登录服务器") ;
				_setItemText(hWnd, IDOK, L"下一步") ;
				_setItemText(hWnd, IDCANCEL, L"退出") ;
				_setItemText(hWnd, ID_REGION_HELP, L"请选择你想要登录的地区。你必须先激活在该区的帐号才能登录") ;
			}

			HWND list = GetDlgItem(hWnd, ID_REGION_LIST) ;
			for ( UINT32 i = 0 ; i < g_regions.size() ; ++i )
			{
				SendMessage(list, LB_ADDSTRING, i, (LPARAM)g_regions[i].name().c_str());
			}
			SendMessage(list, LB_SETCURSEL, 0, 0) ;

			break ;
		}
		case WM_COMMAND :
		{
			switch (LOWORD(wParam))
			{
				case IDOK :
				{
					// Which did they select?
					HWND list = GetDlgItem(hWnd, ID_REGION_LIST) ;
					UINT32 selection = (UINT32)SendMessage(list, LB_GETCURSEL, 0, 0) ;
					g_region = g_regions[selection] ;

					EndDialog(hWnd, 1);
					return 1;
				}
				case IDCANCEL :
				{
					EndDialog(hWnd, 0);
					mainSetStatus(_T("Cancelled Region Selection.")) ;
					mainQuit() ;
					return 1;
				}
			}
			break;
		}
		case WM_NOTIFY :
		{
			LPNMHDR  pnmhdr = (LPNMHDR)lParam;
			switch (pnmhdr->code)
			{
				case NM_RETURN:
				case NM_CLICK:
				{
				PNMLINK link = (PNMLINK)lParam;
				ShellExecute(NULL, L"open", link->item.szUrl, NULL, NULL, SW_SHOWNORMAL);
				}
			}
			break;
		}
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Display the Region Select dialog

bool RegionDialog()
{
	int ret = DialogBox(hMainInstance, MAKEINTRESOURCE(REGION_DIALOG), NULL, RegionProc) ;
	if ( ret != 1 )
		return false ;

	return true ;
}

//////////////////////////////////////////////////////////////////////
// Remove the trailing spaces from the end of a string.

static void
_trimTrailingSpaces(std::wstring& text)
{
	while ( text.empty() == false )
	{
		if ( wcschr(L"\n \t\r", *text.rbegin()) == 0 )
			return ;
		text.erase(text.size() - 1) ;
	}
}

//////////////////////////////////////////////////////////////////////
// Find the next (comma separated) string into a std::string

static wchar_t* _readString(const char* name, wchar_t* p, std::wstring& into, bool allowSpaces)
{
	into.clear() ;
	if ( p == NULL )
		throw region_error(name, "No data for column") ;

	// Skip leading spaces
	while ( wcschr(L"\n \t\r", *p) )
		++p ;
	if ( *p == 0 || *p == ',' )
		throw region_error(name, "No data in column") ;

	// Chop the string at the first ','
	wchar_t* end = wcschr(p, ',') ;
	if ( end != NULL )
		*(end++) = 0 ;

	into = p ;

	// Trim trailing spaces.
	_trimTrailingSpaces(into) ;

	if ( allowSpaces == true )
		return end ;

	for ( UINT32 i = 0 ; i < into.size() ; ++i )
	{
		wchar_t wc = into[i] ;
		if ( wcschr(L"\n \t\r", wc) )
			throw region_error(name, "Spaces in column") ;
		if ( !isalnum(wc) && wc != '.' )
			throw region_error(name, "Invalid value") ;
	}

	return end ;
}

//////////////////////////////////////////////////////////////////////
// Read the regions.txt file

static bool _readRegionsFile()
{
	g_regions.clear() ;

	std::wstring filename = g_exePath + L"/regions.txt" ;
	FILE *fp = NULL;
	if ( _wfopen_s(&fp, filename.c_str(), L"r, ccs=UTF-8") != 0 || fp == NULL )
		return true ;

	static const size_t BUF_SIZE = 4095 ;
	wchar_t buffer[BUF_SIZE + 1] ;
	UINT32 line = 0 ;
	while ( fgetws(buffer, BUF_SIZE, fp) )
	{
		++line;

		wchar_t* p = buffer ;
		while ( wcschr(L"\n \t\r", *p) )
			++p ;

		wchar_t* comment = wcschr(p, '#') ;
		if ( comment != NULL )
		{
			if ( wcsncmp(L"#cn", p, 3) == 0 )
				g_useChinese = true ;
			*comment = 0 ;
		}

		// Empty lines (or lines that had comments removed)
		if ( *p == 0 )
			continue ;

		std::wstring name, service, web ;
		try
		{
			p = _readString("Region Name", p, name, true) ;
			p = _readString("Auth Suffix", p, service, false) ;
			p = _readString("Web Suffix", p, web, false) ;
		}
		catch ( const region_error& error )
		{
			dbugLog(_FLW, "regions.txt:%u:%s:%hs", error.name(), error.what()) ;
			continue ;
		}
		catch ( ... )
		{
			dbugLog(_FLW, "uncaught exception") ;
		}

		g_regions.push_back(PNRegion(name, std::string(service.begin(), service.end()), std::string(web.begin(), web.end()))) ;
	}

	return true ;
}

//////////////////////////////////////////////////////////////////////
// Get the user to select a region, if there is more than one available

bool RegionSelection()
{
	static bool haveReadRegions = false ;
	if ( haveReadRegions == false )
	{
		if ( _readRegionsFile() == false )
		{
			MessageBox(NULL, _T("Unable to determine which region to use - your installation may be corrupt. Please visit the game's website for additional assistance."), _T("Region Selection Failed"), MB_ICONSTOP) ;
			return false ;
		}

		haveReadRegions = true ;
	}

	// In the absence of a regions.txt, assume CRS.
	if ( g_regions.empty() )
		return true ;

	// If there is only one region, no selection.
	if ( g_regions.size() == 1 )
	{
		g_region = g_regions[0] ;
		return true ;
	}

	// Ok - resort to the dialog box.
	if ( !RegionDialog() )
		return false ;

	return true ;
}

//////////////////////////////////////////////////////////////////////
// Populate the TOS scroll-buffer with the terms of service.

SINT32 _LoadTOSListBox(HWND hWnd)
{
    HWND hWndTOS = GetDlgItem(hWnd, IDC_TOS_LIST);

    if(hWndTOS == NULL)
        return 1;

	// Make sure we have the TOS in the data folder.
	std::wstring exeTOS(g_exePath.begin(), g_exePath.end()) ;
	exeTOS += '\\' ;
	exeTOS += g_szTOS ;
	std::wstring dataTOS(g_dataPath.begin(), g_dataPath.end()) ;
	dataTOS += '\\' ;
	dataTOS += g_szTOS ;
	CopyFile(exeTOS.c_str(), dataTOS.c_str(), TRUE) ;

    FILE *fp = NULL;
    if ( _wfopen_s(&fp, g_szTOS, L"r") != 0 || NULL == fp )
    {
        return 1;
    }

    HDC hDC = GetDC(hWndTOS);

	TCHAR szLine[4096];
	TCHAR* p = NULL ;
    SIZE size;
    SINT32 x = 0;
    while(_fgetts(szLine, 4096, fp) != NULL)
    {
        p = _tcschr(szLine, 10);

        if(p) *p = 0;

        GetTextExtentPoint32(hDC, szLine, _tcslen(szLine), &size);

        if(size.cx > x)
            x = size.cx;

        SendMessage(hWndTOS, LB_INSERTSTRING, -1, (LPARAM)szLine);
    }

    // Set width of string extent to be 1 1/3 of largest string length
    // Follows SetColumnWidth for ListBox.
    SendMessage(hWndTOS, LB_SETHORIZONTALEXTENT, x * 4 / 3, (LPARAM)NULL);

    ReleaseDC(hWndTOS, hDC);

    fclose(fp);

    return 0;
}

//////////////////////////////////////////////////////////////////////
// TOS acceptance dialog.

BOOL CALLBACK TOSProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
//	MouseDragProc(hWnd, Msg, wParam, lParam) ;

	switch (Msg)
	{
		case WM_INITDIALOG :
		{
			mainSetIcon(hWnd) ;

			TCHAR windowTitle[512] ;
			LoadString(hMainInstance, IDS_TOS_TITLE, windowTitle, sizeof(windowTitle) - 1) ;
			SetWindowText(hWnd, windowTitle) ;

			LoadBmpToButton(hWnd, IDCANCEL, IDB_DECLINE) ;
			LoadBmpToButton(hWnd, IDOK, IDB_ACCEPT) ;

			if ( 0 != _LoadTOSListBox(hWnd) )
			{
				EndDialog(hWnd, 0);
				mainSetStatus(_T("Inaccessible/missing Terms of Service"));
				mainQuit() ;

				return 1;
			}
			break ;
		}
		case WM_COMMAND :
		{
			switch (LOWORD(wParam))
			{
				case IDOK :
				{
					EndDialog(hWnd, 1);
					return 1;
				}
				case IDCANCEL :
				{
					EndDialog(hWnd, 0);
					mainSetStatus(_T("Declined Terms of Service")) ;
					mainQuit() ;
					return 1;
				}
			}
			break;
		}
		case WM_NOTIFY :
		{
			LPNMHDR  pnmhdr = (LPNMHDR)lParam;
			switch (pnmhdr->code)
			{
				case NM_RETURN:
				case NM_CLICK:
				{
				PNMLINK link = (PNMLINK)lParam;
				ShellExecuteW(NULL, L"open", link->item.szUrl, NULL, NULL, SW_SHOWNORMAL);
				}
			}
			break;
		}
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Display the Terms of Service Dialog

bool TOSDialog()
{
	int ret = DialogBox(hMainInstance, MAKEINTRESOURCE(TOS_DIALOG), NULL, TOSProc) ;
	if ( ret != 1 )
		return false ;

	return true ;
}

//////////////////////////////////////////////////////////////////////

UCHAR *getPassWord(void)
{
	return g_passMD5;
}

//////////////////////////////////////////////////////////////////////
// Populate the server-list box.

static unsigned int _lockDescriptionStringID(int lock)
{
	if ( lock >= AL_ENTRY )
		return IDS_GAME_LOCK_CLOSED ;
	else if ( lock > AL_OFF )
		return IDS_GAME_LOCK_LOCKED ;
	else switch ( lock )
	{
		case AL_OFFLINE:
			return IDS_GAME_LOCK_OFFLINE ;
		case AL_LOADING:
			return IDS_GAME_LOCK_LOADING ;
		case AL_INITIALIZING:
			return IDS_GAME_LOCK_INITIALIZING ;
		case AL_SYNCING:
			return IDS_GAME_LOCK_SYNCING ;
		case AL_OFF:
			return IDS_GAME_LOCK_OPEN ;
		default:
		break ;
	}

	return IDS_GAME_LOCK_UNKNOWN ;
}

COLORREF White = RGB(255, 255, 255) ;
COLORREF Background = RGB(222, 222, 206) ;

COLORREF brushes[] =
{
	RGB(0, 128, 0)		// Open
,	RGB(32, 144, 32)	// Locked
,	RGB(220, 96, 0)		// Loading
,	RGB(192, 0, 192)	// Closed
,	RGB(192, 0, 0)		// Offline
,	RGB(0, 0, 0)		// Unknown
} ;

int _lockMode(int lock)
{
	if ( lock == AL_OFF )
		return 0 ;
	if ( lock > AL_OFF && (lock & AL_ENTRY) == 0 )
		return 1 ;
	if ( lock >= AL_LOADING && lock <= AL_SYNCING )
		return 2 ;
	if ( lock > AL_OFF )
		return 3 ;
	if ( lock == AL_OFFLINE )
		return 4 ;
	return 5 ;
}

static int _SelectArenaEntry()
{
	for ( UINT32 i = 0 ; i < Auth::g_games.size() ; ++i )
	{
		if ( Auth::g_games.m_games[i].m_arenaID == loginData.m_arenaID )
		{
			SendMessage(serverList, LB_SETCURSEL, i, 0) ;
			return i ;
		}
	}
	return -1 ;
}

//////////////////////////////////////////////////////////////////////
// Return the index of the server selected by the user.

UINT32 logonGetServerSelection()
{
	if ( serverList == NULL )
		return 0 ;
	UINT32 selection = (UINT32)SendMessage(serverList, LB_GETCURSEL, 0, 0) ;
	return selection ;
}

void logonUserSelection(int i)
{
	static int state = -1 ;
	int newState ;
	if ( i < 0 || i >= (int)Auth::g_games.size() )
	{
		newState = FALSE ;
	}
	else
	{
		Auth::GameEntry& server = Auth::g_games.m_games[i] ;
		if ( server.m_lockState >= AL_OFF && server.m_lockState <= AL_ENTRY )
			newState = TRUE ;
		else
			newState = FALSE ;

		loginData.m_arenaID = server.m_arenaID ;
		SendMessage(serverList, LB_SETCURSEL, i, 0) ;
	}

	if ( g_inputEnabled && state != newState )
	{
		SendMessage(playButton, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)(state ? launchBmp : nolaunchBmp)) ;
		EnableWindow(playButton, newState) ;
	}
}

void Auth::GameList::process()
{
	if ( serverList == NULL )
	{
		mainSetStatus(_T("no sl")) ;
		return ;
	}

	TCHAR buffer[128] ;
	LoadString(hMainInstance, IDS_GOT_GAME_LIST, buffer, sizeof(buffer) - 1) ;
	mainSetStatus(buffer) ;

	////
	// While unlikely, it is possible that our subscription may change
	// on the server and the list of servers we can access might go away.
	// For simplicity's sake, I therefore remove the entire list from the
	// comonent and then add it back, and re-select the one the user had
	// previously selected.
	////

	// Purge current entries
	while ( SendMessage(serverList, LB_DELETESTRING, 0, 0) > 0 )
		;

	int sel = -1 ;

	// Add these entries
	for ( unsigned int i = 0 ; i < size() ; ++i )
	{
		if ( m_games[i].m_arenaID == loginData.m_arenaID )
			sel = i ;
		SendMessage(serverList, LB_ADDSTRING, i, (LPARAM)"") ;
	}

	static bool runOnce = true ;
	if ( runOnce == true )
	{
		EnableWindow(serverList, true) ;

		sel = _SelectArenaEntry() ;
		logonUserSelection(sel) ;

		runOnce = false ;
	}

	g_nextGameListRequest = teulTime() + 8 ;

	logonUserSelection(sel) ;
}

//////////////////////////////////////////////////////////////////////
// Blip an error

static bool _MessageEntryError(HWND hWnd, int stringID)
{
	TCHAR title[128], message[640] ;
	LoadString(hMainInstance, IDS_LOGIN_ENTRY_ERROR, title, sizeof(title) - 1) ;
	LoadString(hMainInstance, stringID, message, sizeof(message) - 1) ;

	MessageBox(hWnd, message, title, MB_OK | MB_ICONASTERISK) ;

	return false ;
}

static bool _MessageArenaLocked(HWND hWnd, int lockState)
{
	TCHAR title[64], format[128], description[128], message[512] ;
	LoadString(hMainInstance, IDS_LOGIN_ENTRY_ERROR, title, sizeof(title) - 1) ;
	LoadString(hMainInstance, IDS_GAME_UNAVAILABLE, format, sizeof(format) - 1) ;
	LoadString(hMainInstance, _lockDescriptionStringID(lockState), description, sizeof(description) - 1) ;

	_stprintf_s(message, format, description) ;

	MessageBox(hWnd, message, title, MB_OK | MB_ICONASTERISK) ;

	return false ;
}

//////////////////////////////////////////////////////////////////////
//

static void _ReadLoginData()
{
	g_passMask[0] = g_passMD5[0] = '\0';

	// Read the login history from disk and
	LoginData fromDisk ;
	if ( fromDisk.Read() == false )
		return ;
	if ( fromDisk.Validate() == false )
		return ;

	loginData = fromDisk ;

	loginData.GetPasswordMask(g_passMask) ;
	loginData.GetPasswordMD5(g_passMD5, sizeof(g_passMD5)) ;
}

//////////////////////////////////////////////////////////////////////
// When the window is ready, populate and prepare it.

bool logonOnInitDialog(HWND hWnd)
{
	TCHAR windowTitle[256] ;

	LoadString(hMainInstance, IDS_LOGON_TITLE, windowTitle, sizeof(windowTitle) - 1) ;
	SetWindowText(hWnd, windowTitle) ;

	serverList = GetDlgItem(hWnd, IDC_SERVER_LIST) ;
	playButton = GetDlgItem(hWnd, IDOK) ;

	launchBmp = LoadBitmap(hMainInstance, MAKEINTRESOURCE(IDB_LAUNCH)) ;
	nolaunchBmp = LoadBitmap(hMainInstance, MAKEINTRESOURCE(IDB_NOLAUNCH)) ;

	SendMessage(playButton, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)nolaunchBmp) ;
	EnableWindow(playButton, false) ;

	LoadBmpToButton(hWnd, IDCANCEL, IDB_EXIT) ;

    HWND hLogin = GetDlgItem(hWnd, IDC_LOGIN_NAME);
    HWND hPassword = GetDlgItem(hWnd, IDC_PASSWORD);
    HWND hSavePassword = GetDlgItem(hWnd, IDC_SAVE_PASSWORD);

    HDC hDc = NULL;

	// Limit how many characters you can type
    SendMessage(hLogin, EM_LIMITTEXT, MAX_WCHAR_USERNAME, 0);
    SendMessage(hPassword, EM_LIMITTEXT, MAX_PASSWORD_LENGTH, 0);

	_ReadLoginData() ;

    if(g_passMask[0])
    {
        SetWindowTextW(hPassword, g_passMask);
    }
    else
    {
		SetWindowTextW(hPassword, L"") ;
        SetFocus(hPassword);
    }

    if(loginData.m_username[0])
    {
        SetWindowTextW(hLogin, loginData.m_username);
    }
    else
    {
		SetWindowTextW(hLogin, L"") ;
        SetFocus(hLogin);
    }

    if(loginData.m_username[0] && g_passMask[0])
    {
        SendMessage(hSavePassword, BM_SETCHECK, 1, 0);
        SetFocus(GetDlgItem(hWnd, IDOK));
    }
    else
    {
        SendMessage(hSavePassword, BM_SETCHECK, 0, 0);
    }
    
	return true ;
}

//////////////////////////////////////////////////////////////////////
//

BOOL logonFinishDialog(HWND hWnd, int result)
{
	serverList = NULL ;

	DeleteObject(launchBmp) ;
	DeleteObject(nolaunchBmp) ;
	EndDialog(hWnd, result) ;

	return 1 ;
}

void logonOnDrawItem(DRAWITEMSTRUCT* item)
{
	bool isSelected = (item->itemState & ODS_SELECTED) != 0 ;
	bool isValidItem = (item->itemID != -1) ;

	if ( isValidItem == false )
	{
		HBRUSH backbrush = CreateSolidBrush(White) ;
		FillRect(item->hDC, &item->rcItem, backbrush) ;
		DeleteObject(backbrush) ;
		return ;
	}

	const Auth::GameEntry& server = Auth::g_games.m_games[item->itemID] ;
	int mode = _lockMode(server.m_lockState) ;

	TCHAR description[24] ;
	LoadString(hMainInstance, _lockDescriptionStringID(server.m_lockState), description, sizeof(description)) ;

	COLORREF fore = (!isSelected) ? brushes[mode] : White ;
	COLORREF back = (!isSelected) ? White : brushes[mode] ;

	HBRUSH backbrush = CreateSolidBrush(back) ;

	RECT rc = item->rcItem ;
	int halfway = rc.left + (rc.right - rc.left) / 2 ;

	FillRect(item->hDC, &item->rcItem, backbrush) ;

	SetBkColor(item->hDC, back) ;
	SetTextColor(item->hDC, fore) ;

	rc = item->rcItem ;
	rc.left += 2 ;
	rc.top += 1 ;
	rc.right = halfway ;

	wchar_t wideName[256] ;
	WWII::ConvertChars(server.m_name.c_str(), wideName, sizeof(wideName)) ;
	DrawTextW(item->hDC, wideName, wcslen(wideName), &rc, DT_SINGLELINE) ;

	rc = item->rcItem ;
	rc.left = halfway + 2 ;
	rc.right -= 1 ;
	rc.top += 1 ;

	DrawText(item->hDC, description, _tcslen(description), &rc, DT_SINGLELINE) ;

	DeleteObject(backbrush) ;
}

//////////////////////////////////////////////////////////////////////
//

bool logonProcessOK(HWND hWnd)
{
	memset(loginData.m_username, 0, sizeof(loginData.m_username)) ;
	GetWindowTextW(GetDlgItem(hWnd, IDC_LOGIN_NAME), loginData.m_username, MAX_WCHAR_USERNAME);
	if ( loginData.m_username[0] == 0 )
		return _MessageEntryError(hWnd, IDS_ENTER_LOGIN_NAME) ;

	wchar_t wideInput[MAX_PASSWORD_LENGTH + 1] ;
	wideInput[0] = 0 ;
	GetWindowTextW(GetDlgItem(hWnd, IDC_PASSWORD), wideInput, MAX_PASSWORD_LENGTH);
	if ( wideInput[0] == 0 )
		return _MessageEntryError(hWnd, IDS_ENTER_PASSWORD) ;

	UINT32 len = 0 ;
	for ( len = 0 ; wideInput[len] != 0 ; ++len )
	{
		g_passMask[len] = '*' ;
	}
	g_passMask[len] = 0 ;

	// We populate the password field with the mask of the previous
	// password so that we can tell the user hasn't typed anything.
	// Remember: we store the MD5 sum not the actual password.
	if ( wcscmp(wideInput, g_passMask) != 0 )
	{
		// Convert to a UTF8 string so we can perform an MD5 sum
		char utf8Password[(MAX_PASSWORD_LENGTH * 4) + 1] ;
		WWII::ConvertChars(wideInput, utf8Password, sizeof(utf8Password)) ;

		UCHAR digest[256+1] ;
		memset(digest, 0 ,sizeof(digest));
		getMD5FromString(utf8Password, digest);
		convertMD5StringToHex(digest, g_passMD5, sizeof(digest));
	}

	// Update the arena we think we're launching
	const SINT32 serverID = logonGetServerSelection();
	const SINT32 lockState = Auth::g_games.m_games[serverID].m_lockState ;
	if ( lockState < AL_OFF || lockState >= AL_ENTRY )
		return _MessageArenaLocked(hWnd, lockState) ;

	bool savePassword = (SendMessage(GetDlgItem(hWnd, IDC_SAVE_PASSWORD), BM_GETCHECK, 0, 0) != 0) ;
	loginData.Write(savePassword, g_passMask, g_passMD5) ;

	return true ;
}

//////////////////////////////////////////////////////////////////////
//

SINT32 logonResetLogon(void)
{
	return 1;
}

void logonEnableInput(BOOL isEnabled)
{
	g_inputEnabled = isEnabled ;
	EnableWindow(GetDlgItem(hMainWnd, IDC_LOGIN_NAME), isEnabled) ;
	EnableWindow(GetDlgItem(hMainWnd, IDC_PASSWORD), isEnabled) ;
	EnableWindow(serverList, isEnabled) ;
	EnableWindow(playButton, isEnabled) ;
}
