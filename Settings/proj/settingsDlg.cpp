// settingsDlg.cpp : implementation file
//

#include "syscfg.h" // Path is project specific.

#include "stdafx.h"
#include "settings.h"

#include <vector>
#include <sstream>
#include "settingsDlg.h"
#include "registry.h"

#include "load.h"
#include "language.h"
#include "cpuid.h"
#include "file.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "GL/glew.h"
#include <GL/gl.h>
#include "GL/wglew.h"

#include <algorithm>

// CsettingsDlg dialog
static HGLRC _hrc;

static int _curLanguage = 0;

CsettingsDlg::CsettingsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CsettingsDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDI_WWII_ICON);
}

CsettingsDlg::~CsettingsDlg()
{
}

void CsettingsDlg::_kill()
{
    loadKill();
}

void CsettingsDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_COMBO_LANGUAGES, _language);
    DDX_Control(pDX, IDC_COMBO_VIDEO_CARD, _videocard);
    DDX_Control(pDX, IDC_COMBO_RESOLUTION, _resolution);
    DDX_Control(pDX, IDC_COMBO_NETWORK, _network);
    DDX_Control(pDX, IDC_COMBO_SOUND_CARD, _soundcard);
}

BEGIN_MESSAGE_MAP(CsettingsDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_CBN_SELCHANGE(IDC_COMBO_LANGUAGES, OnCbnSelchangeComboLanguages)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_CBN_SELCHANGE(IDC_COMBO_VIDEO_CARD, OnCbnSelchangeComboVideoCard)
	ON_CBN_SELCHANGE(IDC_COMBO_RESOLUTION, OnCbnSelchangeComboResolution)
	ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
	ON_CBN_SELCHANGE(IDC_COMBO_NETWORK, OnCbnSelchangeComboNetwork)
	ON_CBN_SELCHANGE(IDC_COMBO_SOUND_CARD, OnCbnSelchangeComboSoundCard)
END_MESSAGE_MAP()


void CsettingsDlg::_chooseScreenFormat(CDC *dc)
{
    PIXELFORMATDESCRIPTOR pfd;
    int i, pixelformat;

    memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));

    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cRedBits = 0;
    pfd.cRedShift = 0;
    pfd.cGreenBits = 0;
    pfd.cGreenShift = 0;
    pfd.cBlueBits = 0;
    pfd.cBlueShift = 0;
    pfd.cAlphaBits = 0;
    pfd.cAlphaShift = 0;
    pfd.cAccumBits = 0;
    pfd.cAccumRedBits = 0;
    pfd.cAccumGreenBits	= 0;
    pfd.cAccumBlueBits = 0;
    pfd.cAccumAlphaBits	= 0;
    pfd.cDepthBits = 32;
    pfd.cStencilBits = 0;
    pfd.cAuxBuffers = 0;
    pfd.iLayerType = PFD_MAIN_PLANE;
    pfd.bReserved = 0;
    pfd.dwLayerMask = 0;
    pfd.dwVisibleMask = 0;
    pfd.dwDamageMask = 0;

    pixelformat = ChoosePixelFormat(dc->m_hDC, &pfd);

    for(i = 1; ; i++)
	{
        if(!DescribePixelFormat(dc->m_hDC, i, sizeof(PIXELFORMATDESCRIPTOR), &pfd))
		{
			break;
		}
	}

    if(!DescribePixelFormat(dc->m_hDC, pixelformat, sizeof(PIXELFORMATDESCRIPTOR), &pfd))
	{
        return;
	}

	// Set the pixel format selected
    if(!SetPixelFormat(dc->m_hDC, pixelformat, &pfd))
	{
		return;
	}
}

void CsettingsDlg::_adjustOK()
{
    CRect windowrect;
    GetClientRect(&windowrect); 

    CWnd *ok = GetDlgItem(IDOK);
    
    CRect okrect;
    ok->GetClientRect(&okrect);

    ok->SetWindowPos(&wndTopMost, 
                     0, 
                     windowrect.bottom - okrect.bottom, 
                     0, 
                     0, 
                     SWP_NOZORDER | SWP_NOSIZE);
}

void CsettingsDlg::_adjustCancel()
{
    CRect windowrect;
    GetClientRect(&windowrect); 

    CWnd *cancel = GetDlgItem(IDCANCEL);
    
    CRect cancelrect;
    cancel->GetClientRect(&cancelrect);

    cancel->SetWindowPos(&wndTopMost, 
                         windowrect.right - cancelrect.right, 
                         windowrect.bottom - cancelrect.bottom, 
                         0, 
                         0, 
                         SWP_NOZORDER | SWP_NOSIZE);
}

// CsettingsDlg message handlers

BOOL CsettingsDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	wwiiolpath_string storedWorkingDir = L"";
	wchar_t wBuffer[MAX_PATH + 1];
	char buffer[512 + 1];

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

    //font.CreatePointFont(100, _T("arial"));
    
    BOOL success = font.CreateFont(14,                        // nHeight
                                   0,                         // nWidth
                                   0,                         // nEscapement
                                   0,                         // nOrientation
                                   FW_NORMAL,                 // nWeight
                                   FALSE,                     // bItalic
                                   FALSE,                     // bUnderline
                                   0,                         // cStrikeOut
                                   ANSI_CHARSET,              // nCharSet
                                   OUT_DEFAULT_PRECIS,        // nOutPrecision
                                   CLIP_DEFAULT_PRECIS,       // nClipPrecision
                                   DEFAULT_QUALITY,           // nQuality
                                   DEFAULT_PITCH | FF_SWISS,  // nPitchAndFamily
                                   _T("arial"));              // lpszFacename

    SetFont(&font);

    _language.SetFont(&font);
    _videocard.SetFont(&font);
    _resolution.SetFont(&font);
    _network.SetFont(&font);
    _soundcard.SetFont(&font);

    GetDlgItem(IDOK)->SetFont(&font);
    GetDlgItem(IDCANCEL)->SetFont(&font);
    GetDlgItem(IDC_STATIC_LANGUAGE)->SetFont(&font);
    GetDlgItem(IDC_STATIC_NETWORK)->SetFont(&font);
    GetDlgItem(IDC_STATIC_VIDEO)->SetFont(&font);
    GetDlgItem(IDC_STATIC_VIDEO_CARD)->SetFont(&font);
    GetDlgItem(IDC_STATIC_RESOLUTION)->SetFont(&font);
    GetDlgItem(IDC_STATIC_SOUND)->SetFont(&font);
    GetDlgItem(IDC_STATIC_SOUND_CARD)->SetFont(&font);

	fileInit();

	wBuffer[0] = 0;
	GetCurrentDirectory(MAX_PATH, wBuffer);
	storedWorkingDir = wBuffer;

	SetCurrentDirectory(WW2_exeDataPath.c_str());

	for(int i = 0; i < MAX_LANGUAGES; i++)
	{
		sprintf_s(buffer, 512, "loc/%s/settings.dtd", langNames[i]);

		if(!langInit(i, buffer))
	    {
            // Default to English
			sprintf_s(buffer, 512, "loc/%s/settings.dtd", langNames[i]);
			langInit(i, buffer);
		}
	}

	SetCurrentDirectory(storedWorkingDir.c_str());

    // Not used at the moment
#if(0)
    _adjustOK();
    _adjustCancel();
    _initGlew();
#endif

	if(!loadPreferences())
    {
        return FALSE;
    }

    _setDialog();

    // wait until here so localizations are loaded
    SetWindowText(getLoc(_curLanguage, "settings.text.title"));

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CsettingsDlg::_initGlew()
{
    CDC *dc = GetDC();

    _chooseScreenFormat(dc);

    _hrc = wglCreateContext(dc->m_hDC);
    
    wglMakeCurrent(dc->m_hDC, _hrc);

    GLenum error = glewInit();
    
    const GLubyte *s = glewGetErrorString(error);

    wglMakeCurrent(GetDC()->m_hDC, NULL);
    wglDeleteContext(_hrc);

    dc->ReleaseOutputDC();
}

void CsettingsDlg::_setLanguage()
{
    VALUE* value = getValue("app.language");
    
    if(value)
        value->get(_curLanguage);

    _language.ResetContent();
	
	if(region() == 5)
	{
		_language.InsertString(0, getLoc(_curLanguage, "settings.language.chinese"));
		_language.SetCurSel(0);
	}
	else
	{
		_language.InsertString(0, getLoc(_curLanguage, "settings.language.english"));
		_language.InsertString(1, getLoc(_curLanguage, "settings.language.italian"));
		_language.InsertString(2, getLoc(_curLanguage, "settings.language.french"));
		_language.InsertString(3, getLoc(_curLanguage, "settings.language.german"));
		_language.InsertString(4, getLoc(_curLanguage, "settings.language.spanish"));
		_language.InsertString(5, getLoc(_curLanguage, "settings.language.chinese"));
		_language.InsertString(6, getLoc(_curLanguage, "settings.language.russian"));

		_language.SetCurSel(_curLanguage);
	}
}

void CsettingsDlg::_setVideoCard()
{
    _videocard.ResetContent();

	for (size_t i = 0; i < displayList.size(); ++i) {
		DEVMODE dm; dm.dmSize = sizeof(dm);
		if (EnumDisplaySettings(displayList[i].deviceName.c_str(), 0, &dm)) {
			_videocard.InsertString(i, displayList[i].displayName.c_str());
			_videocard.SetItemData(i, (DWORD_PTR)displayList[i].deviceName.c_str());
		}
	}

	// See if video card is one already in the config file.
    VALUE* value = getValue("app.video");
    
	std::wstring str;
    if(value)
        value->get(str);
	
    _videocard.SetCurSel(0);

	for (size_t i = 0; i < displayList.size(); ++i)
	{
		if (!str.compare(displayList[i].deviceName))
		{
			_videocard.SetCurSel(i);
			break;
		}
	}
}

void CsettingsDlg::_setResolution()
{
    _resolution.ResetContent();

    int adapter = _videocard.GetCurSel();

    wchar_t buffer[128];
    int index = 0;

    std::wstring res;
    
    VALUE* value = getValue("app.resolution");
    
    if(value)
        value->get(res);

    int cursor = 0;

	DEVMODE dm; dm.dmSize = sizeof(dm);
	for (DWORD i = 0; EnumDisplaySettings(displayList[adapter].deviceName.c_str(), i, &dm); ++i) {
		if (dm.dmPelsWidth >= 800 && dm.dmPelsHeight >= 600 && dm.dmBitsPerPel >= 32) {
			wsprintf(buffer, L"%dx%dx%d", dm.dmPelsWidth, dm.dmPelsHeight, dm.dmBitsPerPel);
			if (_resolution.FindString(0, buffer) >= 0)
				continue;
			_resolution.InsertString(index, buffer);
			if (!res.compare(buffer))
				cursor = index;
			++index;
		}
	}

    _resolution.SetCurSel(cursor);
}

void CsettingsDlg::_setNetwork()
{
    _network.ResetContent();

    _network.InsertString(0, getLoc(_curLanguage, "settings.net.usebest"));
    _network.InsertString(1, getLoc(_curLanguage, "settings.net.useprimary"));
    _network.InsertString(2, getLoc(_curLanguage, "settings.net.usesecondary"));

    long value = 0;
    
    VALUE* v = getValue("app.usenetworkroute");
    
    if(v)
        v->get(value);
    
    _network.SetCurSel(value);
}

void CsettingsDlg::_setSoundCard()
{
    _soundcard.ResetContent();

    int numDrivers = 0;
    DSOUNDDRIVERS *drivers;

    drivers = dx3dGetSoundInfo(numDrivers);

    int cursor = 0;

    std::wstring savedstr;
    
    VALUE* value = getValue("app.sound");
    
    if(value)
        value->get(savedstr);
	
    for(int i = 0; i < numDrivers; i++)
    {
		std::wstring& str = drivers[i].description;

		_soundcard.InsertString(i, str.c_str());
        _soundcard.SetItemData(i, (DWORD_PTR)drivers[i].guid);

        wchar_t ss[1024] = {0};
        
        StringFromGUID2(*drivers[i].guid, ss, 1024);

        if(!ss[0])
            continue;

        if(!_wcsnicmp(savedstr.c_str(), ss, savedstr.length()))
            cursor = i;
    }

    _soundcard.SetCurSel(cursor);
}

void CsettingsDlg::_setDialog()
{
	videoDisplay tmpDisplay;
	displayList.clear();
	
	DISPLAY_DEVICE device; device.cb = sizeof(device);
    for ( DWORD i = 0 ; EnumDisplayDevices(NULL, i, &device, 0) != 0 ; ++i ) {

		tmpDisplay.indexNumber = i;
		tmpDisplay.deviceName = device.DeviceName;
		tmpDisplay.deviceString = device.DeviceString;

		tmpDisplay.displayName = L"Display " + std::to_wstring(tmpDisplay.indexNumber + 1) + L" - " + tmpDisplay.deviceString;
		displayList.push_back(tmpDisplay);
    }

	_setLanguage();
    _setVideoCard();
    _setResolution();
    _setSoundCard();
    _setNetwork();

    _p_info info;

    _cpuid(&info);

	// Set the Window title
	SetWindowText(getLoc(_curLanguage, "settings.text.title"));
	
	// Set localized dialog text
    CWnd *wnd = NULL;

    wnd = GetDlgItem(IDOK);
    wnd->SetWindowText(getLoc(_curLanguage, "settings.button.save"));

    wnd = GetDlgItem(IDCANCEL);
    wnd->SetWindowText(getLoc(_curLanguage, "settings.button.cancel"));

    wnd = GetDlgItem(IDC_STATIC_LANGUAGE);
    wnd->SetWindowText(getLoc(_curLanguage, "settings.text.language"));
    
    wnd = GetDlgItem(IDC_STATIC_NETWORK);
    wnd->SetWindowText(getLoc(_curLanguage, "settings.text.connection"));
    
    wnd = GetDlgItem(IDC_STATIC_VIDEO);
    wnd->SetWindowText(getLoc(_curLanguage, "settings.text.video"));
   
    wnd = GetDlgItem(IDC_STATIC_VIDEO_CARD);
    wnd->SetWindowText(getLoc(_curLanguage, "settings.text.videocard"));
    
    wnd = GetDlgItem(IDC_STATIC_RESOLUTION);
    wnd->SetWindowText(getLoc(_curLanguage, "settings.text.resolution"));
    
    wnd = GetDlgItem(IDC_STATIC_SOUND);
    wnd->SetWindowText(getLoc(_curLanguage, "settings.text.sound"));
    
    wnd = GetDlgItem(IDC_STATIC_SOUND_CARD);
    wnd->SetWindowText(getLoc(_curLanguage, "settings.text.soundcard"));
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CsettingsDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CsettingsDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CsettingsDlg::OnCbnSelchangeComboLanguages()
{
	int l = _language.GetCurSel();

	if(region() == 5)
		l = 5;

    VALUE* value = getValue("app.language");
    
    if(value)
        value->set(l);
    
    // Reset dialog with new language if necessary
    if(_curLanguage != l)
    {
        _setDialog();
    }
}

void CsettingsDlg::_saveDialogValues()
{
    // Force save of current dialog values
    OnCbnSelchangeComboLanguages();
    OnCbnSelchangeComboVideoCard();
    OnCbnSelchangeComboResolution();
    OnCbnSelchangeComboNetwork();
    OnCbnSelchangeComboSoundCard();
}

void CsettingsDlg::OnBnClickedOk()
{
    // Save file info
    // TODO: Add your control notification handler code here
    _saveDialogValues();
	loadSave();
    OnOK();
}

void CsettingsDlg::OnCbnSelchangeComboVideoCard()
{
	VALUE* value = getValue("app.video");
	if ( value ) {
		int selection = _videocard.GetCurSel();
		const wchar_t* data = (wchar_t*)_videocard.GetItemData(selection);
		value->set(data);
	}

    // Reset resolution for this display
    _setResolution();
}

void CsettingsDlg::OnCbnSelchangeComboResolution()
{    
    CString str;
   
    int selection = _resolution.GetCurSel();
    int length = _resolution.GetLBTextLen(selection);

    _resolution.GetLBText(selection, str.GetBuffer(length));

    VALUE* value = getValue("app.resolution");
    
    if(value)
        value->set(str.GetBuffer());
}

BOOL CsettingsDlg::DestroyWindow()
{
    _kill();
    return CDialog::DestroyWindow();
}

void CsettingsDlg::OnBnClickedCancel()
{
    // TODO: Add your control notification handler code here
    OnCancel();
}

void CsettingsDlg::OnCbnSelchangeComboNetwork()
{  
    int selection = _network.GetCurSel();
    
    VALUE* value = getValue("app.usenetworkroute");
    
    if(value)
        value->set(selection);
}

void CsettingsDlg::OnCbnSelchangeComboSoundCard()
{
    CString str;
   
    int selection = _soundcard.GetCurSel();

    GUID *guid = (GUID *)_soundcard.GetItemData(selection);
    wchar_t ss[1024] = {0};

    if(guid)
    {
        StringFromGUID2(*guid, ss, 1024);
    }
    else
    {
        wmemcpy(ss, L"Primary", 7); 
    }

    VALUE* value = getValue("app.sound");
    
    if(value)
        value->set(CW2A(ss));
}

int CsettingsDlg::getWindowsVersion()
{
	int returnValue = 0;
	HKEY hKey;
	LONG res;

	int major = 0;
	int minor = 0;
	std::wstring pname = L"";
	std::wstring temp = L"";
	std::vector<std::wstring> parts;

	res = RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"), 0, KEY_QUERY_VALUE, &hKey);
	if (res == ERROR_SUCCESS)
	{
		DWORD dwType = REG_SZ;
		char currentVersion[300];
		DWORD len = 300;
		res = RegQueryValueEx(hKey, _T("CurrentVersion"), NULL, &dwType, (LPBYTE)&currentVersion, &len);
		if (res == ERROR_SUCCESS)
		{
			pname = reinterpret_cast<LPWSTR>(currentVersion);

			std::wstringstream wss(pname);
			while (std::getline(wss, temp, L'.'))
				parts.push_back(temp);

			for (unsigned int it = 0; it <= parts.size(); ++it)
			{
				if (it == 0)
					major = std::stoi(parts[it]); // convert to int
				if (it == 1)
					minor = std::stoi(parts[it]); // convert to int
			}

			returnValue = major * 1000;
			returnValue = returnValue + minor;
		}
		else
		{
			returnValue = 0;
		}
	}
	else
	{
		returnValue = 0;
	}

	return returnValue;
}
