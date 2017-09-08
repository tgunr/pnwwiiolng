// PlaygateDlg.h : header file
//

#pragma once

#include "BDialog.h"
#include "afxwin.h"
#include "label.h"
#include "explorer2.h"

// CPlaygateDlg dialog
class CPlaygateDlg : public CBDialog
{
	DECLARE_DYNAMIC(CPlaygateDlg)

	// Construction
public:
	CPlaygateDlg(CWnd* pParent = nullptr);	// standard constructor
	virtual ~CPlaygateDlg();
	virtual UINT TitleLocID() const { return SERVSEL_TITLE; }
	virtual UINT OkLocID() const { return SERVSEL_OK; }
	virtual UINT HintLocID() const { return SERVSEL_HINT; }

// Dialog Data
	enum { IDD = IDD_PLAYGATE_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

	//{{AFX_MSG(CPlaygateDlg)
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
	//{{AFX_DATA(CPlaygateDlg)	
	//}}AFX_DATA

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_VIRTUAL(CPlaygateDlg)
	virtual BOOL OnInitDialog();
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

public:
	CButton m_ok;
	CExplorer2 m_browser;
	CListBox m_servers;
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg void OnLbnSelchangeServerList();
	afx_msg void OnBnClickedOk();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);

};
