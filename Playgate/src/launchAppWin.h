#pragma once
// $Id: launchAppWin.h 102 2009-01-09 20:42:45Z osmith@CRS $

#include "auth/authTypes.h"

extern bool launchApplication(const TCHAR* exeFile, const TCHAR* parameters, std::wstring path) ;
extern bool launchAppWin(const APP_LAUNCH *AppLaunch, const char* RegKey);
extern bool launchAppWinGetHost(char* HostIP, char *HostPort);
