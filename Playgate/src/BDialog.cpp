// BDialog.cpp : implementation file
//

#include "stdafx.h"
#include "BDialog.h"
#include "Playgate.h"
#include "../res/locIDs.h"
#include "../ww2/proj/versionNo.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBDialog dialog

CBDialog::CBDialog(UINT nIDTemplate, CWnd* pParent /*=nullptr*/)
	: CDialog(nIDTemplate, pParent)
{
	//{{AFX_DATA_INIT(CBDialog)
		// NOTE: the ClassWizard will add member initialization here
	m_style=StyleCenter;
	//}}AFX_DATA_INIT

	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CBDialog::Localize(UINT itemId, UINT locID) const
{
	CWnd* item = GetDlgItem(itemId);
	if ( item != nullptr )
	{
		if ( locID )
			item->SetWindowTextW(GetLoc(locID));
		else
		{
			wchar_t versionNo[81];
			swprintf(versionNo, 80, L"%s", _T(STRVER));
			item->SetWindowText(versionNo);
		}
	}
}

BOOL CBDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	SetWindowText(GetLoc(TitleLocID()));
#ifdef SHOW_VERSIONS
	Localize(IDC_VERSION_NO, 0);
#endif
	Localize(IDOK, OkLocID());
	Localize(IDCANCEL, CancelLocID());
	Localize(IDC_HINT, HintLocID());

	return TRUE;
}

BEGIN_MESSAGE_MAP(CBDialog, CDialog)
	//{{AFX_MSG_MAP(CBDialog)
	ON_WM_CTLCOLOR()
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Default LocIDs for buttons

UINT CBDialog::TitleLocID() const { return APP_TITLE; }
UINT CBDialog::OkLocID() const { return DIALOG_OK; }
UINT CBDialog::CancelLocID() const { return DIALOG_CANCEL; }
UINT CBDialog::HintLocID() const { return DIALOG_HINT; }

/////////////////////////////////////////////////////////////////////////////
// CBDialog message handlers

BOOL CBDialog::OnEraseBkgnd(CDC* pDC) 
{
	CDialog::OnEraseBkgnd(pDC);	
	if(!m_bitmap.m_hObject)
		return true;
	
	CRect rect;
	GetClientRect(&rect);
	CDC dc;
	dc.CreateCompatibleDC(pDC);
	CBitmap* pOldBitmap = dc.SelectObject(&m_bitmap);
	int bmw, bmh;
	BITMAP bmap;
	m_bitmap.GetBitmap(&bmap);
	bmw = bmap.bmWidth;
	bmh = bmap.bmHeight;
	int xo=0, yo=0;

	if(m_style == StyleTile)
	{
		for (yo = 0; yo < rect.Height(); yo += bmh)
		{
			for (xo = 0; xo < rect.Width(); xo += bmw)
			{
				pDC->BitBlt (xo, yo, rect.Width(),
					rect.Height(), &dc,
					0, 0, SRCCOPY);
			}
		}

	}

	if(m_style == StyleCenter)
	{
		if(bmw < rect.Width())
			xo = (rect.Width() - bmw)/2;
		else 
			xo=0;
		if(bmh < rect.Height())
			yo = (rect.Height()-bmh)/2;
		else
			yo=0;
		pDC->BitBlt (xo, yo, rect.Width(),
			rect.Height(), &dc,
			0, 0, SRCCOPY);
	}

	if(m_style == StyleStretch)
	{
		pDC->StretchBlt(xo, yo, rect.Width(),
			rect.Height(), &dc,
			0, 0,bmw,bmh, SRCCOPY);
	}

	
	dc.SelectObject(pOldBitmap);


	return true;
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CBDialog::OnPaint()
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

//when a Static control is being drawn, change the background mode and brush to transparent
HBRUSH CBDialog::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
	if( nCtlColor == CTLCOLOR_STATIC )
	{
		pDC->SetBkMode(TRANSPARENT);
		hbr = (HBRUSH)GetStockObject( NULL_BRUSH );
	}
	return hbr;
}

void CBDialog::SetBitmapStyle(int style)
{
	if((style==StyleTile)||
		(style==StyleCenter)||
		(style==StyleStretch))
	{	
		m_style = style;
	}

}

int CBDialog::SetBitmap(UINT nIDResource)
{
	if(m_bitmap.LoadBitmap(nIDResource))
		return 0;
	else
		return 1;//error
}

int CBDialog::SetBitmap(LPCTSTR name)
{
	int res = m_bitmap.LoadBitmap(name);
	if(res)
		return 0;	
	else
		return 1;//error
}

int CBDialog::SetBitmap(HBITMAP bitmap)
{
	int res = m_bitmap.Attach(bitmap);
	return ( res ) ? 0 : 1;
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CBDialog::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CBDialog::OnTimer(UINT_PTR nIDEvent)
{
	if ( nIDEvent == 'QUIT' )
		OnCancel();
	else
		CDialog::OnTimer(nIDEvent);
}
void CBDialog::WinHelp(DWORD dwData, UINT nCmd)
{
	// TODO: Add your specialized code here and/or call the base class

	//CDialog::WinHelp(dwData, nCmd);
}
