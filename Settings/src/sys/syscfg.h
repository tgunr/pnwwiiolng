
#pragma once

#ifndef __SETTINGS_SYSCFG_H__
#define __SETTINGS_SYSCFG_H__ 1

//  ============================
//
//  Settings.exe - syscfg.h
//
//  ============================

//	------------------------------	Public Definitions

#if platOS == OS_WIN32

// Windows defines
//  #define AUTH_CLIENT
#define platEndian platLittleEndian
#define platBuild TEUL_CLIENT
#define _CRT_RAND_S
//  #define OPENGL3D	1
#define WIN32_LEAN_AND_MEAN
#define _GDI32_
#define NO_DEBUG_WRITE

#include <cwchar>
#include <stdexcept>
#include <cstdlib>

#include "stdafx.h"
#include <stdint.h>

#include "include\systypes.h"
#include "fio\fiostructure.h"

#include <windows.h>
#undef PostMessage

// Project defines - Non-Specific
using SYS_DDINITFUNC = LOGICAL(*)();
using SYS_D3DINITFUNC = LOGICAL(*)();
using SYS_DDRENDERFUNC = LOGICAL(*)();
using SYS_D3DRENDERFUNC = LOGICAL(*)();
using SYS_DDKILLFUNC = void(*)();
using SYS_D3DKILLFUNC = void(*)();
using SYS_MOUSEFUNC = void(*)(UINT message, WPARAM wparam, LPARAM lparam);
using SYS_TIMERFUNC = void(*)(WPARAM wparam, LPARAM lparam);
using SYS_KEYBOARDFUNC = void(*)(UINT message, WPARAM wparam, LPARAM lparam);

#define MAXENUMDRIVERS		32
#define MAXMIPLEVELS		12
#define MINSOUNDS			32
#define MINDYNALITES		16

#define SYS_TITLE				"WWIIOL"
#define SYS_COMPANY_NAME		"Playnet"
#define SYS_REGISTRY_NAME		_T(SYS_COMPANY_NAME) _T("\\") _T(SYS_TITLE)

#define TERRAIN_PREPATH "Terrain\\"
#define GR2DATA_PREPATH "gr2Data\\"

#define HIGHSPEED_LIGHTING

// Use the 3d API lighting mechanism (may be hardware)
#define USE_3DAPI_LIGHTING

// Project defines - DEBUG
#ifdef _DEBUG
#ifndef NO_LOGGING
#define EVENT_LOG
#define EVENT_LOG_TIMES
//		#define EVENT_LOG_EXTREME
#endif
// #define MEMORY_DEBUG_EXTREME
#elif defined(BETA_RELEASE)
#ifndef EVENT_LOG
# define EVENT_LOG
#endif
#endif	// _DEBUG

#define NO_DEBUG_WRITE

#include "log/log.h"

#pragma warning (disable: 4800) // shut the #$!@ up

#ifndef snprintf
# define snprintf _snprintf
#endif
#ifndef usleep
# define usleep(n) Sleep((n))
#endif

#ifndef strcasecmp
#define strcasecmp _stricmp
#endif

// we undefine these so we can redefine them below
// we need to redefine them because the game was
// never coded for unicode, but we need to turn on
// unicode for chinese IMEs; redefining these
// saves a ton of work
#undef lstrcpy
#undef lstrcat
#undef lstrcmp
#undef lstrlen

#define lstrcpy strcpy
#define lstrcat strcat
#define lstrcmp strcmp
#define lstrlen strlen

// these defines replace the above
// defines so can can still conditionally
// compile the unicode flag in certain
// string handling situations (if needed)
#ifdef _UNICODE
#define ustrcpy lstrcpyW
#define ustrcat lstrcatW
#define ustrcmp lstrcmpW
#define ustrlen lstrlenW
#define usprintf swprintf
#else
#define ustrcpy lstrcpyA
#define ustrcat lstrcatA
#define ustrcmp lstrcmpA
#define ustrlen lstrlenA
#define usprintf sprintf
#endif

//  #include "include/precompile_includes.h"
//  #include <deque>
//  #include "wwii/wwiiString.h"

//  #include "math/mathvect.h"
//  #include "math/math3d.h"
//  #include "math/mathnum.h"
//  #include "math/Matrixd.h"
//  #include "math/Matrixf.h"
//  #include "spt/random.h"

//  #include "include/vars.h"
//  #include "ddp.h"

//  #include "wwii/wwiiOwnershipEnums.h"
//  #include "wwii/wwiiFullObjID.h"
//  #include "wwii/wwiiCovert.h"
//  #include "weap/weapDefs.h"
//  #include "weap/weapSpec.h"
//  #include "inst/instDescriptor.h"

//  #include "phui/Javascript.h"

//  #include "pi/pi.h"

extern UINT64 GetMicroSeconds(void);

extern wwiiolpath_string WantedDataPath;		//  SET by the command line - ENABLED by WantDataDir = True < OR > is a Nul String L"".
extern wwiiolpath_string WWIIOnlineDirName;		//  The ROOT folder for the game install -> WWIIOnLine

extern wwiiolpath_string WW2_exePath;			//	??_exePath - Windows - EXE ROOT folder:  \Program * Files -> WWIIOnLine == *.exe, WWIIOnLine.ico, readme.*, unins*.* (uninstaller), *.tkt, *.script, bin(dir), DATA(dir), docs(dir), playarc(dir).
												//	??_exePath - macOS - EXE ROOT folder:  /Applications/WWIIOnline(package) == *.exe, WWIIOnLine.ico, readme.*, unins*.* (uninstaller), *.tkt, *.script, bin(dir), DATA(dir), docs(dir), playarc(dir).
extern wwiiolpath_string WW2_exeDataPath;		//	??_exeDataPath - EXE Support Data folder:  \Program * Files -> WWIIOnLine -> DATA == DirectXRedist(dir), loc(dir), playgate(dir), RuntimeLibraries(dir).
extern wwiiolpath_string WW2_exeDocsPath;		//	??_exeDocsPath - EXE Docs Folder:  \Program * Files -> WWIIOnLine -> docs == eula.txt, license.*, regions.txt, tos.txt.
extern wwiiolpath_string WW2_exePlayarcPath;	//	??_exePlayarcPath - EXE playarc folder:  \Program * Files -> WWIIOnLine -> playarc == playarc??.ww2  --OR--  == "-data" command line switch.
extern wwiiolpath_string WW2_exeWorkingPath;	//	??_exeWorkingPath - Windows - EXE Working folder:  \ProgramData -> WWIIOnLine == ftplog.txt, downloaded patches, WWIIOL.log, *.log.
												//	??_exeWorkingPath - macOS - EXE Working folder:  /Library/Application Support//WWIIOnline == ftplog.txt, downloaded patches, WWIIOL.log, *.log.
extern wwiiolpath_string WW2_userDocumentsPath;	//	??_userDocumentsPath - Windows - USER Config ROOT folder:  \User -> My Documents -> WWIIOnLine == wwiiol.xml, cfml(dir).
												//	??_userDocumentsPath - macOS - USER Config ROOT folder:  /Users/"currentuser"/Documents/WWIIOnline == wwiiol.xml, cfml(dir).
extern wwiiolpath_string WW2_userWorkingPath;	//	??_userWorkingPath - USER Working folder:  \User -> My Documents -> WWIIOnLine -> bin == lg.dat.

#endif  //  platOS == OS_WIN32

#endif // __SETTINGS_SYSCFG_H__