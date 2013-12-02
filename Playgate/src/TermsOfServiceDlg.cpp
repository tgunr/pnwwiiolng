// TermsOfServiceDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Playgate.h"
#include "TermsOfServiceDlg.h"


// CTermsOfServiceDlg dialog

IMPLEMENT_DYNAMIC(CTermsOfServiceDlg, CDialog)

CTermsOfServiceDlg::CTermsOfServiceDlg(CWnd* pParent /*=NULL*/)
	: CBDialog(CTermsOfServiceDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTermsOfServiceDlg)
	//}}AFX_DATA_INIT

	SetBitmap(pgLoadImage(L"tos-bg.bmp"));
	SetBitmapStyle(StyleCenter);
}

CTermsOfServiceDlg::~CTermsOfServiceDlg()
{
}

void CTermsOfServiceDlg::DoDataExchange(CDataExchange* pDX)
{
	CBDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_HINT, m_hint);
}


BEGIN_MESSAGE_MAP(CTermsOfServiceDlg, CBDialog)
	//{{AFX_MSG_MAP(CTermsOfServiceDlg)
	ON_NOTIFY(EN_LINK, IDC_TOS, &CTermsOfServiceDlg::OnEnLinkTos)
	ON_EN_SETFOCUS(IDC_TOS, &CTermsOfServiceDlg::OnEnSetfocusTos)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


// CTermsOfServiceDlg message handlers
static DWORD CALLBACK
_streamInCallback(DWORD dwCookie, LPBYTE pbBuff, LONG cb, LONG* pcb)
{
	CFile* file = (CFile*) dwCookie;
	*pcb = file->Read(pbBuff, cb);

	return 0;
}

static void
_loadTOS(CRichEditCtrl* const ctrl)
{
	ctrl->LimitText(64 * 1024);
	ctrl->SetAutoURLDetect(1);
	ctrl->SetTargetDevice(NULL, 0);
	ctrl->SetWordWrapMode(WBF_WORDWRAP | WBF_WORDBREAK | WBF_LEVEL2);

	// CRichEditCtrl beeps in read only mode.
	ctrl->SetReadOnly(FALSE); 
	ctrl->SetSel(0,-1); 
	ctrl->Clear(); 
	ctrl->SetReadOnly(TRUE); 

	try
	{
		std::wstring filename = g_exePath + _T("\\license.rtf");
		CFile cfile(filename.c_str(), CFile::modeRead);
		EDITSTREAM es;
		es.dwCookie = (DWORD)&cfile;
		es.pfnCallback = _streamInCallback;

		ctrl->StreamIn(SF_RTF, es);
	}
	catch ( ... )
	{
		ctrl->SetWindowText(GetLoc(ERROR_TOS));
	}
}

void CTermsOfServiceDlg::OnEnLinkTos(NMHDR* pNMHDR, LRESULT* pResult)
{
	ENLINK *pEnLink = reinterpret_cast<ENLINK *>(pNMHDR);
	// TODO: Add your control notification handler code here
	*pResult = 0;
}

void CTermsOfServiceDlg::OnEnSetfocusTos()
{
	// TODO: Add your control notification handler code here
	static bool firstTime = true;
	if ( firstTime == true )
	{
		firstTime = false;
		CWnd* ctrl = GetDlgItem(IDC_TOS);
		_loadTOS((CRichEditCtrl*)ctrl);
	}
}
