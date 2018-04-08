#ifndef __TEULKIT_SYSCFG_H__
#define __TEULKIT_SYSCFG_H__ 1
// Can't use a pragma guard because there are multiple syscfgs.

//  ============================
//
//  teulKit client - syscfg.h
//
//  ============================

// This is the teulKit client syscfg.h
// Copyright 1999 Playnet Inc, All rights reserved
// NOTE : There is NO corresponding source file, this is purely an abstract
// header file.
// Originating Programmer : Stephen W. Evans

#include "include/platforms.h"
#include "fio/fiostructure.h"

#include "teul/teulBasic.h"
#include "teul/teulClosingEnums.h"
#include "teul/teulEndpointDefs.h"
#include "teul/teulEndpoint.h"
#include "teul/teulPackers.h"
#include "teul/teulPacket.h"
#include "weap/weapSpec.h"

extern wwiiolpath_string WantedDataPath;		//  SET by the command line - ENABLED by WantDataDir = True < OR > is a Nul String L"".
extern wwiiolpath_string	WWIIOnlineDirName;	//  The ROOT folder for the game install - .\WWIIOnLine

extern wwiiolpath_string WW2_exePath;			//	??_exePath - Windows - EXE ROOT folder:  \Program * Files -> WWIIOnLine == *.exe, WWIIOnLine.ico, readme.*, unins*.* (uninstaller), *.tkt, *.script, bin(dir), DATA(dir), docs(dir), playarc(dir).
												//	??_exePath - macOS - EXE ROOT folder:  /Applications/WWIIOnline(package) == *.exe, WWIIOnLine.ico, readme.*, unins*.* (uninstaller), *.tkt, *.script, bin(dir), DATA(dir), docs(dir), playarc(dir).
extern wwiiolpath_string WW2_exeDataPath;		//	??_exeDataPath - EXE Support Data folder:  \Program * Files -> WWIIOnLine -> DATA == DirectXRedist(dir), loc(dir), playgate(dir), RuntimeLibraries(dir).
extern wwiiolpath_string WW2_exeDocsPath;		//	??_exeDocsPath - EXE Docs Folder:  \Program * Files -> WWIIOnLine -> docs == eula.txt, license.*, regions.txt, tos.txt.
extern wwiiolpath_string WW2_exePlayarcPath;	//	??_exePlayarcPath - EXE playarc folder:  \Program * Files -> WWIIOnLine -> playarc == playarc??.ww2  --OR--  == "-data" command line switch.
extern wwiiolpath_string WW2_exeWorkingPath;	//	??_exeWorkingPath - Windows - EXE Working folder:  \ProgramData -> WWIIOnLine == ftplog.txt, downloaded patches, WWIIOL.log, *.log.
												//	??_exeWorkingPath - macOS - EXE Working folder:  /Library/Application Support//WWIIOnline == ftplog.txt, downloaded patches, WWIIOL.log, *.log.
extern wwiiolpath_string WW2_userConfigPath;	//	??_userConfigPath - Windows - USER Config ROOT folder:  \User -> My Documents -> WWIIOnLine == wwiiol.xml, cfml(dir).
												//	??_userConfigPath - macOS - USER Config ROOT folder:  /Users/"currentuser"/Documents/WWIIOnline == wwiiol.xml, cfml(dir).
extern wwiiolpath_string WW2_userWorkingPath;	//	??_userWorkingPath - USER Working folder:  \User -> My Documents -> WWIIOnLine -> bin == lg.dat.

#endif  //__TEULKIT_SYSCFG_H__