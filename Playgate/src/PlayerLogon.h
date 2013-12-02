#pragma once

#include "BDialog.h"
#include "explorer2.h"
#include "afxwin.h"
#include "label.h"

// CPlayerLogon dialog

class CPlayerLogon : public CBDialog
{
	DECLARE_DYNAMIC(CPlayerLogon)

public:
	CPlayerLogon(CWnd* pParent = NULL);   // standard constructor
	virtual ~CPlayerLogon();
	virtual UINT TitleLocID() const { return LOGIN_TITLE; }
	virtual UINT OkLocID() const { return LOGIN_OK; }
	virtual UINT HintID() const { return LOGIN_HINT; }

	void ToggleInput(bool enable, bool changeFocus=false);

	static void Browse();

// Dialog Data
	enum { IDD = IDD_PLAYER_LOGON };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	//{{AFX_MSG(CPlayerLogon)
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
	//{{AFX_DATA(CPlayerLogon)
	//}}AFX_DATA

protected:
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPlayerLogon)
	virtual BOOL OnInitDialog();
	//}}AFX_VIRTUAL
public:
	CExplorer2 m_browser;
	afx_msg void OnBnClickedOk();
	CEdit m_user;
	CEdit m_pass;
	CButton m_ok;
	afx_msg void OnEnChangeInputs();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	std::wstring m_startUrl;
	virtual void WinHelp(DWORD dwData, UINT nCmd = HELP_CONTEXT);
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
};

extern void logonAttempt();

