// src\PlayerLogon.cpp : implementation file
//

#include "stdafx.h"
#include "Playgate.h"
#include "PlayerLogon.h"
#include "pnRegion.h"
#include "LoginData.h"
#include "wwii/wwiiUTF.h"
#include "md5.h"
#include "auth/authPlaygateTypes.h"

// We don't keep the actual password, just the MD5 sum.
static UCHAR g_passMD5[64+1];
extern HANDLE hApplicationMutex;

// CPlayerLogon dialog

IMPLEMENT_DYNAMIC(CPlayerLogon, CDialog)

CPlayerLogon::CPlayerLogon(CWnd* pParent /*=nullptr*/)
	: CBDialog(CPlayerLogon::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTermsOfServiceDlg)
	//}}AFX_DATA_INIT

	SetBitmap(pgLoadImage(L"bg.bmp"));
	SetBitmapStyle(StyleCenter);

	std::string url = "http://www";
	url += g_region.m_webSuffix;
	url += "/pglogin.html";

	m_startUrl.assign(url.begin(), url.end());
}

CPlayerLogon::~CPlayerLogon()
{
}

void CPlayerLogon::DoDataExchange(CDataExchange* pDX)
{
	CBDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LOGIN_NAME, m_user);
	DDX_Control(pDX, IDC_PASSWORD, m_pass);
	DDX_Control(pDX, IDOK, m_ok);
	DDX_Control(pDX, IDC_HINT, m_hint);
	DDX_Control(pDX, IDC_EXPLORER2, m_browser);
}

BEGIN_MESSAGE_MAP(CPlayerLogon, CBDialog)
	//{{AFX_MSG_MAP(CPlayerLogon)
	ON_BN_CLICKED(IDOK, &CPlayerLogon::OnBnClickedOk)
	ON_EN_CHANGE(IDC_LOGIN_NAME, &CPlayerLogon::OnEnChangeInputs)
	ON_EN_CHANGE(IDC_PASSWORD, &CPlayerLogon::OnEnChangeInputs)
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
	ON_WM_CHAR()
	ON_WM_SYSKEYDOWN()
END_MESSAGE_MAP()


// CPlayerLogon message handlers

BOOL CPlayerLogon::OnInitDialog()
{
	CBDialog::OnInitDialog();

	Localize(IDS_LOGIN_NAME_LABEL, LOGIN_NAME_LABEL);
	Localize(IDS_PASSWORD_LABEL, LOGIN_PASSWORD_LABEL);

	if ( loginData.m_username[0] )
	{
		m_user.SetWindowTextW(loginData.m_username);
		if ( /*loginData.m_savePassword &&*/ loginData.m_passlen > 0 )
		{
			// Get the old MD5
			loginData.GetPasswordMD5(g_passMD5, sizeof(g_passMD5));

			// Populate the input field with a bogus password
			// we can match this later to tell if the user has
			// changed the password or not.
			wchar_t* passMask = new wchar_t[loginData.m_passlen + 1];
			loginData.GetPasswordMask(passMask);
			m_pass.SetWindowTextW(passMask);
			delete [] passMask;
		}
	}

	ToggleInput(false);

	Browse();

	ReleaseMutex(hApplicationMutex);

	return TRUE;
}

void CPlayerLogon::ToggleInput(bool enable, bool changeFocus)
{
	m_user.EnableWindow(enable);
	int userLen = m_user.GetWindowTextLengthW();
	m_pass.EnableWindow(enable);
	int passLen = m_pass.GetWindowTextLengthW();
//	m_savePass.EnableWindow(enable);

	bool allowLogin = (enable && userLen >= 2 && passLen >= 4);
	m_ok.EnableWindow(allowLogin);

	if ( changeFocus )
	{
		if ( userLen <= 2 )
			SetTimer('Fusr', 10, nullptr);
		else
			SetTimer('Fpwd', 10, nullptr);
	}
}

void CPlayerLogon::OnBnClickedOk()
{
	memset(loginData.m_username, 0, sizeof(loginData.m_username));
	m_user.GetWindowTextW(loginData.m_username, MAX_WCHAR_USERNAME);

	wchar_t wideInput[MAX_PASSWORD_LENGTH + 1];
	wideInput[0] = 0;
	m_pass.GetWindowTextW(wideInput, MAX_PASSWORD_LENGTH);
	if ( loginData.m_username[0] == 0 || wideInput[0] == 0 )
	{
		MessageBox(GetLoc(AUTH_ENTRY), GetLoc(ERROR_GENERAL), MB_ICONEXCLAMATION|MB_OK);
		if ( loginData.m_username[0] == 0 )
			m_user.SetFocus();
		else
			m_pass.SetFocus();
		return;
	}

	// Create a password mask based on the input
	wchar_t passMask[MAX_PASSWORD_LENGTH + 1];
	UINT32 len = 0;
	for ( len = 0; wideInput[len] != 0; ++len )
	{
		passMask[len] = L'*';
	}
	passMask[len] = 0;

	// We populate the password field with the mask of the previous
	// password so that we can tell the user hasn't typed anything.
	// Remember: we store the MD5 sum not the actual password.
	if ( wcscmp(wideInput, passMask) != 0 )
	{
		// Convert to a UTF8 string so we can perform an MD5 sum
		char utf8Password[(MAX_PASSWORD_LENGTH * 4) + 1];
		WWII::ConvertChars(wideInput, utf8Password, sizeof(utf8Password));

		UCHAR digest[256+1];
		memset(digest, 0 ,sizeof(digest));
		getMD5FromString(utf8Password, digest);
		convertMD5StringToHex(digest, g_passMD5, sizeof(digest));
	}

	bool savePassword = false; // m_savePass.GetCheck() != 0;

	loginData.SetPassword(savePassword, len, g_passMD5);

	ToggleInput(false, false);

	playgate.QueueLogonAttempt();
}

// Called from the background thread: attempt a login.
void logonAttempt()
{
	playgate.SetStatus(GetLoc(AUTH_AUTHENTICATION));

	char username[sizeof(loginData.m_username) + 1];
	WWII::ConvertChars(loginData.m_username, username, sizeof(username));
	dbugLog(_FLI, "Login request \"%s\"", username);
	Auth::LoginRequest request(username, g_passMD5);
	request.send();
}

void Auth::LoginReply::process()
{
	dbugLog(_FLI, "Login reply %s[%c]", m_code.c_str(), (m_playnet) ? 'y' : 'n');
	if ( wasSuccessful() )
	{
		playgate.SetState(PgServerSelect);
		playgate.SetStatus(GetLoc(AUTH_LOGIN), AppProgress);
		playgate.m_playnet = m_playnet;
		hMainWnd->SetTimer('AOK', 1500, nullptr);
		return;
	}
	const char* p = m_code.c_str();
	const TCHAR* message = nullptr;
	if ( strcmp(p, "full") == 0 )
		message = GetLoc(AUTH_REGION_FULL);
	else if ( strcmp(p, "login") == 0 || strcmp(p, "internal") == 0 || strcmp(p, "unknown") == 0 )
		message = GetLoc(ERROR_SYSTEM);
	else if ( strcmp(p, "game") == 0 )
		message = GetLoc(ERROR_NETWORK);
	else if ( strcmp(p, "install") == 0 )
	{
		message = GetLoc(ERROR_VERSION);
		playgate.StartBrowser(ERR_GAME_REINSTALL);
	}
	else if ( strcmp(p, "locked") == 0 )
		message = GetLoc(ERROR_SERVER_LOCKED);
	else if ( strcmp(p, "invuser") == 0 )
		message = GetLoc(ERROR_USERNAME);
	else if ( strcmp(p, "invpass") == 0 )
		message = GetLoc(ERROR_PASSWORD);
	else if ( strcmp(p, "inactive") == 0 )
		message = GetLoc(ERROR_SUBSCRIPTION);
	else
		message = GetLoc(ERROR_AUTHENTICATION);

	MessageBeep(MB_ICONEXCLAMATION);
	playgate.SetStatus(message, AppError);

	// Re-enable input in 1 second.
	hMainWnd->SetTimer('AERR', 1000, nullptr);
}

void CPlayerLogon::OnEnChangeInputs()
{
	ToggleInput(true, false);
	//m_browser.Refresh(); /* This causes a blank error upon accepting TOS */
	
}

void CPlayerLogon::OnTimer(UINT_PTR nIDEvent)
{
	switch ( nIDEvent )
	{
	case 'AERR':
		ToggleInput(true, true);
	break;
	case 'AOK':
		OnOK();
	break;
	case 'Fusr':
		m_user.SetFocus();
	break;
	case 'Fpwd':
		m_pass.SetFocus();
	break;
	default:
		CBDialog::OnTimer(nIDEvent);
		return;
	break;
	}
	KillTimer(nIDEvent);
}

void CPlayerLogon::Browse()
{
	CPlayerLogon* logon = (CPlayerLogon*)hMainWnd;
	logon->m_browser.Navigate(logon->m_startUrl.c_str(), nullptr, nullptr, nullptr, nullptr);
}

void CPlayerLogon::WinHelp(DWORD dwData, UINT nCmd)
{
	// TODO: Add your specialized code here and/or call the base class

	//CBDialog::WinHelp(dwData, nCmd);
}

void CPlayerLogon::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: Add your message handler code here and/or call default

	CBDialog::OnChar(nChar, nRepCnt, nFlags);
}

void CPlayerLogon::OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: Add your message handler code here and/or call default

	CBDialog::OnSysKeyDown(nChar, nRepCnt, nFlags);
}
