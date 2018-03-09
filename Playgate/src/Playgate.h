// Playgate.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols
#include "BDialog.h"
#include "loc.h"

#include "include\systypes.h"  //  MJH - Feburary 27, 2018.
#include "fio\fiostructure.h"  //  MJH - Feburary 27, 2018.

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

//  MJH - Feburary 27, 2018. START ===========================

extern wwiiolpath_string		WWIIOnlineDirName;		//  The ROOT folder for the game install - .\WWIIOnLine

#if EXE_BUILD == BUILD_EXE_SETTINGS
//  settings.exe
extern wwiiolpath_string ST_exePath;			//	??_exePath - EXE ROOT folder:  \Program * Files -> WWIIOnLine == *.exe, WWIIOnLine.ico, readme.*, unins*.* (uninstaller), *.tkt, *.script, bin(dir), DATA(dir), docs(dir), playarc(dir).
extern wwiiolpath_string ST_exeBinPath;			//	??_exeBinPath - EXE DLL folder:  \Program * Files -> WWIIOnLine -> bin == *.lib, steam_appid.txt.
extern wwiiolpath_string ST_exeDataPath;		//	??_exeDataPath - EXE Support Data folder:  \Program * Files -> WWIIOnLine -> DATA == DirectXRedist(dir), loc(dir), playgate(dir), RuntimeLibraries(dir). 
extern wwiiolpath_string ST_exeDocsPath;		//	??_exeDocsPath - EXE Docs Folder:  \Program * Files -> WWIIOnLine -> docs == eula.txt, license.*, regions.txt, tos.txt.
extern wwiiolpath_string ST_exePlayarcPath;		//	??_exePlayarcPath - EXE playarc folder:  \Program * Files -> WWIIOnLine -> playarc == playarc??.ww2  --OR--  == "-data" command line switch.
extern wwiiolpath_string ST_exeWorkingPath;		//	??_exeWorkingPath - EXE Working folder:  \ProgramData -> WWIIOnLine == ftplog.txt, downloaded patches, WWIIOL.log, *.log.
extern wwiiolpath_string ST_userConfigPath;		//	??_userConfigPath - USER Config ROOT folder:  \User -> My Documents -> WWIIOnLine == wwiiol.xml, cfml(dir).
extern wwiiolpath_string ST_userWorkingPath;	//	??_userWorkingPath - USER Working folder:  \User -> My Documents -> WWIIOnLine -> bin == lg.dat.

extern LOGICAL ST_exePathSet;
extern LOGICAL ST_exeBinPathSet;
extern LOGICAL ST_exeDataPathSet;
extern LOGICAL ST_exeDocsPathSet;
extern LOGICAL ST_exePlayarcPathSet;
extern LOGICAL ST_exeWorkingPathSet;
extern LOGICAL ST_userConfigPathSet;
extern LOGICAL ST_userWorkingPathSet;

#elif EXE_BUILD == BUILD_EXE_PLAYGATE
//  Playgate.exe
extern wwiiolpath_string PG_exePath;			//	??_exePath - EXE ROOT folder:  \Program * Files -> WWIIOnLine == *.exe, WWIIOnLine.ico, readme.*, unins*.* (uninstaller), *.tkt, *.script, bin(dir), DATA(dir), docs(dir), playarc(dir).
extern wwiiolpath_string PG_exeBinPath;			//	??_exeBinPath - EXE DLL folder:  \Program * Files -> WWIIOnLine -> bin == *.lib, steam_appid.txt.
extern wwiiolpath_string PG_exeDataPath;		//	??_exeDataPath - EXE Support Data folder:  \Program * Files -> WWIIOnLine -> DATA == DirectXRedist(dir), loc(dir), playgate(dir), RuntimeLibraries(dir). 
extern wwiiolpath_string PG_exeDocsPath;		//	??_exeDocsPath - EXE Docs Folder:  \Program * Files -> WWIIOnLine -> docs == eula.txt, license.*, regions.txt, tos.txt.
extern wwiiolpath_string PG_exePlayarcPath;		//	??_exePlayarcPath - EXE playarc folder:  \Program * Files -> WWIIOnLine -> playarc == playarc??.ww2  --OR--  == "-data" command line switch.
extern wwiiolpath_string PG_exeWorkingPath;		//	??_exeWorkingPath - EXE Working folder:  \ProgramData -> WWIIOnLine == ftplog.txt, downloaded patches, WWIIOL.log, *.log.
extern wwiiolpath_string PG_userConfigPath;		//	??_userConfigPath - USER Config ROOT folder:  \User -> My Documents -> WWIIOnLine == wwiiol.xml, cfml(dir).
extern wwiiolpath_string PG_userWorkingPath;	//	??_userWorkingPath - USER Working folder:  \User -> My Documents -> WWIIOnLine -> bin == lg.dat.

extern LOGICAL PG_exePathSet;
extern LOGICAL PG_exeBinPathSet;
extern LOGICAL PG_exeDataPathSet;
extern LOGICAL PG_exeDocsPathSet;
extern LOGICAL PG_exePlayarcPathSet;
extern LOGICAL PG_exeWorkingPathSet;
extern LOGICAL PG_userConfigPathSet;
extern LOGICAL PG_userWorkingPathSet;

#else
//  EXE_BUILD == BUILD_EXE_WW2
//  WW2.exe
extern wwiiolpath_string WW2_exePath;			//	??_exePath - EXE ROOT folder:  \Program * Files -> WWIIOnLine == *.exe, WWIIOnLine.ico, readme.*, unins*.* (uninstaller), *.tkt, *.script, bin(dir), DATA(dir), docs(dir), playarc(dir).
extern wwiiolpath_string WW2_exeBinPath;		//	??_exeBinPath - EXE DLL folder:  \Program * Files -> WWIIOnLine -> bin == *.lib, steam_appid.txt.
extern wwiiolpath_string WW2_exeDataPath;		//	??_exeDataPath - EXE Support Data folder:  \Program * Files -> WWIIOnLine -> DATA == DirectXRedist(dir), loc(dir), playgate(dir), RuntimeLibraries(dir). 
extern wwiiolpath_string WW2_exeDocsPath;		//	??_exeDocsPath - EXE Docs Folder:  \Program * Files -> WWIIOnLine -> dosc == eula.txt, license.*, regions.txt, tos.txt.
extern wwiiolpath_string WW2_exePlayarcPath;	//	??_exePlayarcPath - EXE playarc folder:  \Program * Files -> WWIIOnLine -> playarc == playarc??.ww2  --OR--  == "-data" command line switch.
extern wwiiolpath_string WW2_exeWorkingPath;	//	??_exeWorkingPath - EXE Working folder:  \ProgramData -> WWIIOnLine == ftplog.txt, downloaded patches, WWIIOL.log, *.log.
extern wwiiolpath_string WW2_userConfigPath;	//	??_userConfigPath - USER Config ROOT folder:  \User -> My Documents -> WWIIOnLine == wwiiol.xml, cfml(dir).
extern wwiiolpath_string WW2_userWorkingPath;	//	??_userWorkingPath - USER Working folder:  \User -> My Documents -> WWIIOnLine -> bin == lg.dat.

extern LOGICAL WW2_exePathSet;
extern LOGICAL WW2_exeBinPathSet;
extern LOGICAL WW2_exeDataPathSet;
extern LOGICAL WW2_exeDocsPathSet;
extern LOGICAL WW2_exePlayarcPathSet;
extern LOGICAL WW2_exeWorkingPathSet;
extern LOGICAL WW2_userConfigPathSet;
extern LOGICAL WW2_userWorkingPathSet;

#endif
//  MJH - Feburary 27, 2018. END =============================