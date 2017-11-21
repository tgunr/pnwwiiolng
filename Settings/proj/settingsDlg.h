// settingsDlg.h : header file
//

#pragma once
#include "afxwin.h"

#include "../src/dx/dx3dinfo.h"

// CsettingsDlg dialog
class CsettingsDlg : public CDialog
{
// Construction
public:
	CsettingsDlg(CWnd* pParent = NULL);	// standard constructor

    virtual ~CsettingsDlg();

// Dialog Data
	enum { IDD = IDD_SETTINGS_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
    virtual BOOL DestroyWindow();

	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

protected:
    void _setDialog();
    void _setLanguage();
    void _chooseScreenFormat(CDC *dc);
    void _initGlew();
    void _setVideoCard();
    void _setResolution();
    void _kill();
    void _adjustOK();
    void _adjustCancel();
    void _setNetwork();
    void _setSoundCard();
    void _saveDialogValues();

public:
    afx_msg void OnCbnSelchangeComboLanguages();
    afx_msg void OnBnClickedOk();
    afx_msg void OnCbnSelchangeComboVideoCard();
    afx_msg void OnCbnSelchangeComboResolution();
    afx_msg void OnDeleteItem(int controlID, DELETEITEMSTRUCT *item);
    afx_msg void OnBnClickedCancel();
    //afx_msg void OnBnClickedCheckRunSse2();
    afx_msg void OnCbnSelchangeComboNetwork();
    afx_msg void OnCbnSelchangeComboSoundCard();

private:
    CComboBox _language;
    CComboBox _videocard;
    CComboBox _resolution;
    CComboBox _network;
    CComboBox _soundcard;

    //CButton _runsse2;

    CBitmap _background;

    std::vector<std::wstring> displays;

    CFont font;
};
