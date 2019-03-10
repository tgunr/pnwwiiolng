#ifndef PN_PROJECT_SYSCFG_H
#define PN_PROJECT_SYSCFG_H 1
// Created: Sun Nov 19 12:50:55 CST 2006
//
//! Host-project specific defines, includes, etc.
//! This file is force-included by the compiler.
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "config.h"
#include "hostproj/hostBuildOptions.h"
#include "include/platforms.h"	// includes precompile_includes.h
#include "fio/fiostructure.h"


#define NOGDI
#define WIN32_LEAN_AND_MEAN

#if platBuild != BUILD_SERVER
#error "Why are you building the host project without AS_SERVER?"
#endif

#include <signal.h>

#include "inst/instDescriptor.h"
#include "wwii/wwiiPlayer.h"
#include "server/serverIndexes.h"
#include "server/serverPeers.h"
#include "wwii/wwiiServerTypeEnums.h"
#include "teul/teulConfigVar.h"

extern LOGICAL				WantedCopyOldSetup;			//  DEF = TRUE -- Look for and copy the old BGE setup files (IF FOUND) to the New WWIIOnLine folder.
extern wwiiolpath_string	WantedDataPath;				//  DEF = L"" -- SET by the command line - ENABLED by WantDataDir = True < OR > is a Nul String L"".
extern wwiiolpath_string	WWIIOnlineDirName;			//  DEF = L"WWIIOnLine" -- The ROOT folder for the game install - .\WWIIOnLine

extern wwiiolpath_string	WW2_exePath;				//	Windows - EXE INSTALL ROOT folder:  \Program * Files -> CRS -> WWIIOnLine == *.exe, WWIIOnLine.ico, readme.*, unins*.* (uninstaller), *.tkt, *.script, bin(dir), DATA(dir), docs(dir), playarc(dir).
														//	macOS - EXE INSTALL ROOT folder:  /Applications -> World War II Online.app == Contents -> _CodeSignature(dir), Frameworks(dir), MacOS(dir -> exefile), Resources(dir) info.plist, PkgInfo.
extern wwiiolpath_string	WW2_exeDataPath;			//	Windows - EXE INSTALL ROOT Support Data folder:  \Program * Files -> CRS  -> WWIIOnLine -> data == DirectXRedist(dir), loc(dir), playgate(dir), RuntimeLibraries(dir).
														//	macOS - EXE INSTALL ROOT Support Data folder:  /Applications -> World War II Online.app -> Contents -> Resources.
extern wwiiolpath_string	WW2_exeDocsPath;			//	Windows - EXE INSTALL ROOT Docs Folder:  \Program * Files -> CRS  -> WWIIOnLine -> docs == eula.txt, license.*, regions.txt, tos.txt.
														//	macOS - EXE INSTALL ROOT Docs Folder:  /Applications -> World War II Online.app -> Contents -> Resources -> docs == eula.txt, license.*, regions.txt, tos.txt.
extern wwiiolpath_string	WW2_exePlayarcPath;			//	Windows - EXE INSTALL ROOT playarc folder:  \Program * Files -> CRS  -> WWIIOnLine -> playarc == playarc??.ww2  --OR--  == "-data" command line switch.
														//	macOS - EXE INSTALL ROOT playarc folder:  /Applications -> World War II Online.app -> Contents -> Resources == playarc.ww2.
extern wwiiolpath_string	WW2_exeWorkingPath;			//	Windows - EXE Working folder:  \ProgramData -> WWIIOnLine == ftplog.txt, downloaded patches, WWIIOL.log, *.log.
														//	macOS - EXE Working folder:  /Library -> Application Support -> WWIIOnline == ftplog.txt, downloaded patches, WWIIOL.log, *.log.
extern wwiiolpath_string	WW2_userDocumentsPath;		//	Windows - USER Config ROOT folder:  \User -> My Documents -> WWIIOnLine == wwiiol.xml, cfml(dir).
														//	macOS - USER Config ROOT folder:  /Users -> "currentuser" -> Documents -> WWIIOnline == wwiiol.xml, cfml(dir).
extern wwiiolpath_string	WW2_userScreenShotsPath;	//	Windows - USER Config ROOT folder:  \User -> My Documents -> WWIIOnLine -> screenshots == SShot*.bmp.
														//	macOS - USER Config ROOT folder:  /Users -> "currentuser" -> Documents -> WWIIOnline -> screenshots == SShot*.bmp.
extern wwiiolpath_string	WW2_userWorkingPath;		//	Windows - USER Working folder:  \User -> My Documents -> WWIIOnLine -> bin == lg.dat.
														//	macOS - USER Working folder:  /Users -> "currentuser" -> Documents -> WWIIOnline -> bin == lg.dat.


#endif // PN_PROJECT_SYSCFG_H
