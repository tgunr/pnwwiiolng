#pragma once
// $Id: RegionSelect.h 185 2009-03-18 19:08:28Z osmith@CRS $

#include "BDialog.h"
#include "label.h"

// CRegionSelect dialog

class CRegionSelect : public CBDialog
{
	DECLARE_DYNAMIC(CRegionSelect)

public:
	CRegionSelect(CWnd* pParent = NULL);   // standard constructor
	virtual ~CRegionSelect();
	virtual UINT TitleLocID() const { return REGION_TITLE; }
	virtual UINT HintLocID() const { return REGION_HINT; }

	static bool ReadRegionsFile();

// Dialog Data
	enum { IDD = IDD_REGION_SELECT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	// Generated message map functions
	//{{AFX_MSG(CRegionSelect)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	//{{AFX_DATA(CRegionSelect)
	//}}AFX_DATA

	protected:
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRegionSelect)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_VIRTUAL
public:
	afx_msg void OnBnClickedOk();
};

extern bool RegionSelect();
