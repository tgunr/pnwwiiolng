#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#if !defined(AFX_BDIALOG_H__A303185C_4254_4F5F_B5A8_462531A59B83__INCLUDED_)
#define AFX_BDIALOG_H__A303185C_4254_4F5F_B5A8_462531A59B83__INCLUDED_

#include "label.h"
#include "loc.h"

#define StyleTile 0
#define StyleCenter 1
#define StyleStretch 2

class CBDialog : public CDialog
{
// Construction
public:
	int SetBitmap(UINT nIDResource);
	int SetBitmap(LPCTSTR bitmapName) ;
	int SetBitmap(HBITMAP bitmap) ;
	void SetBitmapStyle(int style);

	// Sets the WindowText of the specified
	// window element (itemID) to the specified
	// localization string (locID).
	void Localize(UINT itemID, UINT locID) const ;

// Overridable members for returning the LOC IDs for various buttons
	virtual UINT TitleLocID() const ;
	virtual UINT OkLocID() const ;
	virtual UINT CancelLocID() const ;
	virtual UINT HintLocID() const ;

	CBDialog(UINT nIDTemplate, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CBDialog)	
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBDialog)
	protected:
	virtual BOOL OnInitDialog() ;
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CBDialog)
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnPaint();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
protected:
	HICON m_hIcon;
	CBitmap m_bitmap;
private:
	int m_style;
public:
	CLabel m_hint ;
	virtual void WinHelp(DWORD dwData, UINT nCmd = HELP_CONTEXT);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BDIALOG_H__A303185C_4254_4F5F_B5A8_462531A59B83__INCLUDED_)
