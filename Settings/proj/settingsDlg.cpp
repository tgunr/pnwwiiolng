// settingsDlg.cpp : implementation file
//
#include "stdafx.h"
#include "settings.h"

#include <vector>
#include "settingsDlg.h"

#include "load.h"
#include "language.h"
#include "cpuid.h"
#include "file.h"

#include ".\settingsdlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "glew/glew.h"
#include <GL/gl.h>
#include "glew/wglew.h"

#define WW2OL_DIR "\\Battleground Europe"

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
//    DDX_Control(pDX, IDC_CHECK_RUN_SSE2, _runsse2);
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
//ON_BN_CLICKED(IDC_CHECK_RUN_SSE2, OnBnClickedCheckRunSse2)
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
//    GetDlgItem(IDC_CHECK_RUN_SSE2)->SetFont(&font);

	fileInit();

	char buffer[512+1];

	for(int i = 0; i < MAX_LANGUAGES; i++)
	{
		sprintf_s(buffer, 512, "data/loc/%s/settings.dtd", langNames[i]);

		if(!langInit(i, buffer))
	    {
            // Default to English
		    sprintf_s(buffer, 512, "data/loc/%s/settings.dtd", langNames[i]);
			langInit(i, buffer);
		}
	}

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
    SetWindowText(getLoc(_curLanguage, "settings.title"));

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
		_language.InsertString(0, getLoc(_curLanguage, "settings.chinese"));
		_language.SetCurSel(0);
	}
	else
	{
		_language.InsertString(0, getLoc(_curLanguage, "settings.english"));
		_language.InsertString(1, getLoc(_curLanguage, "settings.italian"));
		_language.InsertString(2, getLoc(_curLanguage, "settings.french"));
		_language.InsertString(3, getLoc(_curLanguage, "settings.german"));
		_language.InsertString(4, getLoc(_curLanguage, "settings.spanish"));
		_language.InsertString(5, getLoc(_curLanguage, "settings.chinese"));

		_language.SetCurSel(_curLanguage);
	}
}

void CsettingsDlg::_setVideoCard()
{
    _videocard.ResetContent();

    DWORD i;
	std::wstring str = _T("");

    for(i = 0; i < _numAdapters; i++)
    {
		str = utf8_to_utf16((const unsigned char*)_adapterModes[i]->identifier.Description);
		_videocard.InsertString(i, str.c_str());
        _videocard.SetItemData(i, (DWORD_PTR)&_adapterModes[i]->identifier.DeviceIdentifier);
    }

    // See if video card is one already in the config file.
    VALUE* value = getValue("app.video");
    
    if(value)
        value->get(str);
	
    _videocard.SetCurSel(0);

    static wchar_t ss[1024];

    for(i = 0; i < _numAdapters; i++)
    {
        StringFromGUID2(_adapterModes[i]->identifier.DeviceIdentifier, ss, 1024);

		if(!str.compare(ss))
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

    UINT w, h;

    wchar_t buffer[128];
    int index = 0;

    std::wstring res;
    
    VALUE* value = getValue("app.resolution");
    
    if(value)
        value->get(res);

    int cursor = 0;

    for(unsigned int i = 0; i < _adapterModes[adapter]->_modes.size(); i++)
    {
        w = _adapterModes[adapter]->_modes[i]->Width;
        h = _adapterModes[adapter]->_modes[i]->Height;

        int bpp = dx3dGetBPP(_adapterModes[adapter]->_modes[i]->Format);

        if(w >= 800 && h >= 600 && bpp >= 32)
        {
            wsprintf(buffer, L"%dx%dx%d", w, h, bpp);

			if(!res.compare(buffer))
                cursor = index;
			
            _resolution.InsertString(index++, buffer);
        }
    }

    _resolution.SetCurSel(cursor);
}

void CsettingsDlg::_setNetwork()
{
    _network.ResetContent();

    _network.InsertString(0, getLoc(_curLanguage, "settings.usebest"));
    _network.InsertString(1, getLoc(_curLanguage, "settings.useprimary"));
    _network.InsertString(2, getLoc(_curLanguage, "settings.usesecondary"));

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
    dx3dinfoGetAdaptersAndModes(&_numAdapters, _adapterModes);

    _setLanguage();
    _setVideoCard();
    _setResolution();
    _setSoundCard();
    _setNetwork();

    _p_info info;

    _cpuid(&info);

	/*
    if(info.checks & _CPU_FEATURE_SSE2 && info.os_support & _CPU_FEATURE_SSE2)
    {
        _runsse2.EnableWindow(1);

        bool check = false;

        getValue("app.wantsse2")->get(check);
        _runsse2.SetCheck(check);
    }
    else
    {
        _runsse2.EnableWindow(0);
        _runsse2.SetCheck(false);
    }
	*/

    // Set localized dialog text
    CWnd *wnd = NULL;

    wnd = GetDlgItem(IDOK);
    wnd->SetWindowText(getLoc(_curLanguage, "settings.save"));

    wnd = GetDlgItem(IDCANCEL);
    wnd->SetWindowText(getLoc(_curLanguage, "settings.cancel"));

    wnd = GetDlgItem(IDC_STATIC_LANGUAGE);
    wnd->SetWindowText(getLoc(_curLanguage, "settings.language"));
    
    wnd = GetDlgItem(IDC_STATIC_NETWORK);
    wnd->SetWindowText(getLoc(_curLanguage, "settings.connection"));
    
    wnd = GetDlgItem(IDC_STATIC_VIDEO);
    wnd->SetWindowText(getLoc(_curLanguage, "settings.video"));
   
    wnd = GetDlgItem(IDC_STATIC_VIDEO_CARD);
    wnd->SetWindowText(getLoc(_curLanguage, "settings.videocard"));
    
    wnd = GetDlgItem(IDC_STATIC_RESOLUTION);
    wnd->SetWindowText(getLoc(_curLanguage, "settings.resolution"));
    
    wnd = GetDlgItem(IDC_STATIC_SOUND);
    wnd->SetWindowText(getLoc(_curLanguage, "settings.sound"));
    
    wnd = GetDlgItem(IDC_STATIC_SOUND_CARD);
    wnd->SetWindowText(getLoc(_curLanguage, "settings.soundcard"));
    
//    wnd = GetDlgItem(IDC_CHECK_RUN_SSE2);
//    wnd->SetWindowText(getLoc(_curLanguage, "settings.wantsse2"));
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
//    OnBnClickedCheckRunSse2();
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
    CString str;

    int selection = _videocard.GetCurSel();
   
    GUID *guid = (GUID *)_videocard.GetItemData(selection);
    wchar_t ss[1024] = {0};

    StringFromGUID2(*guid, ss, 1024);

    // Reset resolution for this display
    _setResolution();

    VALUE* value = getValue("app.video");
    
    if(value)
        value->set(CW2A(ss));
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

/*
void CsettingsDlg::OnBnClickedCheckRunSse2()
{
    bool check = _runsse2.GetCheck() ? true : false;
    getValue("app.wantsse2")->set(check);
}
*/

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
