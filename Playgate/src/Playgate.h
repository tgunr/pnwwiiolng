// Playgate.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols
#include "BDialog.h"
#include "loc.h"

extern CBDialog* hMainWnd;

enum StatusType
{
	AppError = RGB(255, 0, 0)
,	AppProgress = RGB(0, 0, 255)
,	AppNote = RGB(0, 0, 0)
,	AppCaution = RGB(255, 128, 0)
};

// CPlaygateApp:
// See Playgate.cpp for the implementation of this class
//

class CPlaygateApp : public CWinApp
{
public:
	CPlaygateApp();

// Overrides
	public:
	virtual BOOL InitInstance();

// Own Functions
	public:
	static const TCHAR* AppName() { return GetLoc(APP_TITLE); }
	void AlertOK(const TCHAR* message) const
	{
		if ( hMainWnd )
			hMainWnd->MessageBox(message, AppName(), MB_OK);
	}
	void SetStatus(const TCHAR* message, StatusType=AppNote) const;
	static void SetWindowText(CWnd* wnd, const char* text);
	void PostCommand(UINT32 messageID, LPARAM lParam) const
	{
		if ( hMainWnd )
			hMainWnd->PostMessage(WM_COMMAND, messageID, lParam);
	}
	void StartBrowser(std::string WebPage);
	void QueueLogonAttempt();
	void Quit();

	void ftping(bool actively);

	void Launch();

// Implementation

	DECLARE_MESSAGE_MAP()
protected:

public:
	bool		m_inputAllowed : 1;
	bool		m_playnet : 1;
	UINT32		m_nextGameListRequest;
	CProgressCtrl* m_progress;
};

extern CPlaygateApp playgate;
extern char g_authHostName[256];
extern HBITMAP pgLoadImage(const std::wstring&);
