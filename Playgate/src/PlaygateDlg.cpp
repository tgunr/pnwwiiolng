// PlaygateDlg.cpp : implementation file
//

#include "Playgate.h"
#include "PlaygateDlg.h"
#include "LoginData.h"
#include "pnRegion.h"
#include "wwii/wwiiUTF.h"
#include "ftp.h"
#include "versionWin.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// ---------------------------------------- Private Variables

// List of games
Auth::GameList Auth::g_games;

// Descriptions of the lock states
static const int NUM_LOCK_STATES = 6;
const char* states[NUM_LOCK_STATES] =
{
	"Online"
,	"Offline"
,	"Closed"
,	"Locked"
,	"Loading"
,	"FULL"
};

// ---------------------------------------- Member Functions

static CPlaygateDlg* g_dlg;
static bool g_launchInProgress;

// CPlaygateDlg dialog

CPlaygateDlg::CPlaygateDlg(CWnd* pParent /*=nullptr*/)
	: CBDialog(CPlaygateDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPlaygateDlg)
	//}}AFX_DATA_INIT
	g_dlg = this;

	SetBitmap(pgLoadImage(L"bg.bmp"));
	SetBitmapStyle(StyleCenter);
}

CPlaygateDlg::~CPlaygateDlg()
{
	g_dlg = nullptr;
}

void CPlaygateDlg::DoDataExchange(CDataExchange* pDX)
{
	CBDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDOK, m_ok);
	DDX_Control(pDX, IDC_HINT, m_hint);
	DDX_Control(pDX, IDC_EXPLORER2, m_browser);
	DDX_Control(pDX, IDC_SERVER_LIST, m_servers);
}

BEGIN_MESSAGE_MAP(CPlaygateDlg, CBDialog)
	//{{AFX_MSG_MAP(CPlaygateDlg)
	//}}AFX_MSG_MAP
	ON_WM_DRAWITEM()
	ON_LBN_SELCHANGE(IDC_SERVER_LIST, &CPlaygateDlg::OnLbnSelchangeServerList)
	ON_BN_CLICKED(IDOK, &CPlaygateDlg::OnBnClickedOk)
	ON_WM_TIMER()
END_MESSAGE_MAP()


// CPlaygateDlg message handlers

BOOL CPlaygateDlg::OnInitDialog()
{
	CBDialog::OnInitDialog();

	Localize(IDS_SERVERS_LABEL, SERVSEL_SERVERS_LABEL );

	std::string url = "http://www";
	url += g_region.m_webSuffix;
	url += "/pgservers.html";

	std::wstring wideUrl(url.begin(), url.end());

	m_browser.Navigate(wideUrl.c_str(), nullptr, nullptr, nullptr, nullptr);

	playgate.m_nextGameListRequest = teulTime();

	return TRUE;  // return TRUE  unless you set the focus to a control
}

//////////////////////////////////////////////////////////////////////
// Populate the server-list box.

static const wchar_t* _lockDescriptionStringID(int lock, UINT32 population)
{
	if ( lock >= AL_ENTRY )
		return L"Maintenance";
	else if ( lock > AL_OFF )
		return L"Locked";
	else switch ( lock )
	{
		case AL_OFFLINE:
			return L"Offline";
		case AL_LOADING:
			return L"Loading";
		case AL_INITIALIZING:
			return L"Starting";
		case AL_SYNCING:
			return L"Syncing";
		case AL_OFF:
			if ( population >= 100 )
				return L"Full";
			return L"Open";
		default:
		break;
	}

	return L"Error";
}

COLORREF White = RGB(255, 255, 255);
COLORREF Background = RGB(222, 222, 206);

COLORREF brushes[] =
{
	RGB(0, 128, 0)		// Open
,	RGB(32, 144, 32)	// Locked
,	RGB(220, 96, 0)		// Loading
,	RGB(192, 0, 192)	// Closed
,	RGB(192, 0, 0)		// Offline
,	RGB(32, 144, 32)	// Full
,	RGB(0, 0, 0)		// Unknown
};

int _lockMode(int lock, UINT32 population)
{
	if ( lock == AL_OFF )
		return 0;
	if ( lock > AL_OFF && (lock & AL_ENTRY) == 0 )
	{
		if ( population >= 100 )
			return 5;
		return 1;
	}
	if ( lock >= AL_LOADING && lock <= AL_SYNCING )
		return 2;
	if ( lock > AL_OFF )
		return 3;
	if ( lock == AL_OFFLINE )
		return 4;
	return 6;
}

static int _SelectArenaEntry(CListBox* serverList)
{
	for ( UINT32 i = 0; i < Auth::g_games.size(); ++i )
	{
		if ( Auth::g_games.m_games[i].m_arenaID == loginData.m_arenaID )
		{
			serverList->SetCurSel(i);
			return i;
		}
	}
	return -1;
}

static void _ServerSelection(CListBox* serverList, int i)
{
	static int state = -1;
	int newState;
	if ( playgate.m_inputAllowed == false || playgate.m_nextGameListRequest == 0 || g_launchInProgress )
	{
		newState = FALSE;
	}
	else if ( i < 0 || i >= (int)Auth::g_games.size() )
	{
		newState = FALSE;
	}
	else
	{
		Auth::GameEntry& server = Auth::g_games.m_games[i];
		std::string info = "Server: v";
		if ( server.m_minVer != 0 && server.m_minVer != server.m_maxVer )
		{
			info += teulGetAddress(server.m_minVer);
			info += "-";
		}
		info += teulGetAddress(server.m_maxVer);
		info += "\nLocal: ";

		UINT32 localVersion = 0;
		SINT32 installs = versionWinGetVersion(server.m_appID, ~0, &localVersion, nullptr);
		if ( installs <= 0 )
			info += "Not Installed";
		else
		{
			info += teulGetAddress(localVersion);
			info += "\nStatus: ";
			if ( localVersion >= server.m_minVer && localVersion <= server.m_maxVer )
				info += "Ready";
			else if ( localVersion < server.m_minVer )
				info += "Patch Reqd";
			else if ( localVersion > server.m_maxVer )
				info += "Wrong Version";
		}

		std::wstring infoW(info.begin(), info.end());
		CWnd* stateText = hMainWnd->GetDlgItem(IDS_SERVER_NOTE);

#if(0)
		// I can't see a way to clear the text, wtf?
		if (stateText != nullptr )
		{
			CPlaygateDlg* pgdialog = (CPlaygateDlg*)hMainWnd->GetDlgItem(IDD_PLAYGATE_DIALOG);
			
			stateText->SetWindowText(L"");
			stateText->SetWindowText(infoW.c_str());
			stateText->RedrawWindow();
			
			std::string url = "http://www";
			url += g_region.m_webSuffix;
			url += "/pgservers.html";
			std::wstring wideUrl(url.begin(), url.end());
			pgdialog->m_browser.Navigate(wideUrl.c_str(), nullptr, nullptr, nullptr, nullptr);

		}
#endif

		if ( server.m_lockState < AL_OFF )
		{
			playgate.SetStatus(GetLoc(ERROR_SERVER), AppError);
			newState = FALSE;
		}
		else if ( server.m_populationPctg >= 100 )
		{
			playgate.SetStatus(GetLoc(ERROR_SERVER_FULL), AppCaution);
			newState = FALSE;
		}
		else if ( server.m_lockState >= AL_ENTRY )
		{
			playgate.SetStatus(GetLoc(ERROR_SERVER_CLOSED), AppCaution);
			newState = (playgate.m_playnet) ? TRUE : FALSE;
		}
		else
		{
			if ( server.m_lockState > 0 )
				playgate.SetStatus(GetLoc(ERROR_SERVER_HELP), AppCaution);
			else
				playgate.SetStatus(GetLoc(ERROR_SERVER_READY), AppProgress); // POORLY NAMED
			newState = TRUE;
		}

		loginData.m_arenaID = server.m_arenaID;
		serverList->SetCurSel(i);
	}

	if ( state != newState )
	{
		g_dlg->m_ok.EnableWindow(newState);
	}
}

void Auth::GameList::process()
{
	if ( g_dlg == nullptr ) return;
	CListBox* serverList = (CListBox*)g_dlg->GetDlgItem(IDC_SERVER_LIST);
	if ( serverList == nullptr ) return;

	int sel = serverList->GetCurSel();
	UINT32 arenaID = (sel >= 0 && sel < (int)m_games.size()) ? m_games[sel].m_arenaID : loginData.m_arenaID;

	////
	// While unlikely, it is possible that our subscription may change
	// on the server and the list of servers we can access might go away.
	// For simplicity's sake, I therefore remove the entire list from the
	// comonent and then add it back, and re-select the one the user had
	// previously selected.
	////

	// Purge current entries
	while ( serverList->DeleteString(0) > 0 )
		;

	// Add these entries
	for ( unsigned int i = 0; i < size(); ++i )
	{
		if ( m_games[i].m_arenaID == arenaID )
			sel = i;
		serverList->AddString(L"");
	}

	static bool runOnce = true;
	if ( runOnce == true )
	{
		serverList->EnableWindow(true);

		sel = _SelectArenaEntry(serverList);

		runOnce = false;
	}

	_ServerSelection(serverList, sel);

	playgate.m_nextGameListRequest = teulTime() + 10;
}


void CPlaygateDlg::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT item)
{
	if ( nIDCtl != IDC_SERVER_LIST )
	{
		CBDialog::OnDrawItem(nIDCtl, item);
		return;
	}

	bool isSelected = (item->itemState & ODS_SELECTED) != 0;
	bool isValidItem = (item->itemID != -1);

	if ( isValidItem == false )
	{
		HBRUSH backbrush = CreateSolidBrush(White);
		FillRect(item->hDC, &item->rcItem, backbrush);
		DeleteObject(backbrush);
		return;
	}

	const Auth::GameEntry& server = Auth::g_games.m_games[item->itemID];
	int mode = _lockMode(server.m_lockState, server.m_populationPctg);

	const TCHAR* description = _lockDescriptionStringID(server.m_lockState, server.m_populationPctg);

	COLORREF fore = (!isSelected) ? brushes[mode] : White;
	COLORREF back = (!isSelected) ? White : brushes[mode];

	HBRUSH backbrush = CreateSolidBrush(back);

	RECT rc = item->rcItem;
	int halfway = rc.left + (rc.right - rc.left) / 2;

	FillRect(item->hDC, &item->rcItem, backbrush);

	SetBkColor(item->hDC, back);
	SetTextColor(item->hDC, fore);

	rc = item->rcItem;
	rc.left += 2;
	rc.top += 1;
	rc.right = halfway;

	wchar_t wideName[256];
	const char* arenaName;
	std::string arenaPrefix = "ARENA_";

	/* Future work: pulling arena names from translation tool
	if(GetLoc(arenaPrefix.append((char*)server.m_arenaID).compare("?")))
		arenaName = server.m_name.c_str(); // Default to hard-coded server name
	else
		arenaName = GetLoc(arenaPrefix.append((char*)server.m_arenaID)).c_str(); // Localized name
	*/

	arenaName = server.m_name.c_str();

	WWII::ConvertChars(arenaName, wideName, sizeof(wideName));
	DrawTextW(item->hDC, wideName, wcslen(wideName), &rc, DT_SINGLELINE);

	rc = item->rcItem;
	rc.left = halfway + 2;
	rc.right -= 1;
	rc.top += 1;

	DrawText(item->hDC, description, _tcslen(description), &rc, DT_SINGLELINE);

	DeleteObject(backbrush);
}

void CPlaygateDlg::OnLbnSelchangeServerList()
{
	_ServerSelection(&m_servers, m_servers.GetCurSel());
}

void CPlaygateDlg::OnBnClickedOk()
{
	playgate.m_nextGameListRequest = 0;
	g_launchInProgress = true;
	m_servers.EnableWindow(false);
	m_ok.EnableWindow(false);

	playgate.QueueLogonAttempt();
}

BOOL CPlaygateDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam))
	{
	case MSG_DOWNLOAD:
		playgate.StartBrowser((const char*)lParam);
		PostMessage(WM_DESTROY, 0, 0);
		return 0;
	break;
	default:
		return CBDialog::OnCommand(wParam, lParam);
	break;
	}
}
void CPlaygateDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: Add your message handler code here and/or call default
	if ( nIDEvent == 'FTP' )
	{
		hMainWnd->KillTimer('FTP');
		int delay = ftpProcess();
		if ( delay > 0 )
			hMainWnd->SetTimer('FTP', delay, nullptr);
		return;
	}

	CBDialog::OnTimer(nIDEvent);
}
