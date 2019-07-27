// Playgate.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols
#include "BDialog.h"
#include "loc.h"

#include "include\systypes.h"
#include "fio\fiostructure.h"

extern CBDialog* hMainWnd;

enum PlaygateState
{
	PgStarting = 0
	, PgTOS
	, PgRegionSelection
	, PgAuthentication
	, PgServerSelect
	, PgFinished
};

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
	void SetState(PlaygateState state);
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

extern wwiiolpath_string WantedDataPath;		//  SET by the command line - ENABLED by WantDataDir = True < OR > is a Nul String L"".
extern wwiiolpath_string WWIIOnlineDirName;		//  The ROOT folder for the game install - .\WWIIOnLine

extern wwiiolpath_string WW2_exePath;			//	??_exePath - Windows - EXE ROOT folder:  \Program * Files -> WWIIOnLine == *.exe, WWIIOnLine.ico, readme.*, unins*.* (uninstaller), *.tkt, *.script, bin(dir), DATA(dir), docs(dir), playarc(dir).
												//	??_exePath - macOS - EXE ROOT folder:  /Applications/WWIIOnline(package) == *.exe, WWIIOnLine.ico, readme.*, unins*.* (uninstaller), *.tkt, *.script, bin(dir), DATA(dir), docs(dir), playarc(dir).
extern wwiiolpath_string WW2_exeDataPath;		//	??_exeDataPath - EXE Support Data folder:  \Program * Files -> WWIIOnLine -> DATA == DirectXRedist(dir), loc(dir), playgate(dir), RuntimeLibraries(dir).
extern wwiiolpath_string WW2_exeDocsPath;		//	??_exeDocsPath - EXE Docs Folder:  \Program * Files -> WWIIOnLine -> docs == eula.txt, license.*, regions.txt, tos.txt.
extern wwiiolpath_string WW2_exePlayarcPath;	//	??_exePlayarcPath - EXE playarc folder:  \Program * Files -> WWIIOnLine -> playarc == playarc??.ww2  --OR--  == "-data" command line switch.
extern wwiiolpath_string WW2_exeWorkingPath;	//	??_exeWorkingPath - Windows - EXE Working folder:  \ProgramData -> WWIIOnLine == ftplog.txt, downloaded patches, WWIIOL.log, *.log.
												//	??_exeWorkingPath - macOS - EXE Working folder:  /Users -> "currentuser" -> Library -> Application Support -> com.corneredrats.wwiionline == ftplog.txt, downloaded patches, WWIIOL.log, *.log.
extern wwiiolpath_string WW2_userDocumentsPath;	//	??_userDocumentsPath - Windows - USER Config ROOT folder:  \User -> My Documents -> WWIIOnLine == wwiiol.xml, cfml(dir).
												//	??_userDocumentsPath - macOS - USER Config ROOT folder:  /Users/"currentuser"/Documents/WWIIOnline == wwiiol.xml, cfml(dir).
extern wwiiolpath_string WW2_userWorkingPath;	//	??_userWorkingPath - USER Working folder:  \User -> My Documents -> WWIIOnLine -> bin == lg.dat.
