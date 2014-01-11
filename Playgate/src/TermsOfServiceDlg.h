#pragma once

#include "BDialog.h"
#include "label.h"

// CTermsOfServiceDlg dialog

class CTermsOfServiceDlg : public CBDialog
{
	DECLARE_DYNAMIC(CTermsOfServiceDlg)

public:
	CTermsOfServiceDlg(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CTermsOfServiceDlg();
	virtual UINT TitleLocID() const { return TOS_TITLE ; }
	virtual UINT HintLocID() const { return TOS_HINT ; }
	virtual UINT OkLocID() const { return TOS_OK ; }

// Dialog Data
	enum { IDD = IDD_TERMS_OF_SERVICE };

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	// Generated message map functions
	//{{AFX_MSG(CTermsOfServiceDlg)
	afx_msg void OnEnLinkTos(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnEnSetfocusTos();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	//{{AFX_DATA(CTermsOfServiceDlg)
	//}}AFX_DATA

	protected:
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTermsOfServiceDlg)
	//}}AFX_VIRTUAL
public:
};
