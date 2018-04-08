
//  fiostructure.h.
//  Date: Feb-27-2018
//
//  Copyright (c) 2018 Playnet, Inc., All Rights Reserved.
//  Not to be distributed in API form except as part of a
//  compiled library.
//
//  MJH - April 8, 2018.


//  ==============================================================================================================

//  Notice:
//		The Paths as stored,   DO NOT   have the folder separater at the end of the last part of the pathname.
//		This   MUST   be added when appending further path segmints to a stored Paths.

//  ==============================================================================================================


#pragma once

#ifndef __FileIOStructure__
#define __FileIOStructure__

//  Selected exe Build.
//  REQUIRES EXE_BUILD to be set. ( #define EXE_BUILD BUILD_EXE_WW2 )
#define BUILD_EXE_WW2		0	//  WW2.exe
#define BUILD_EXE_SETTINGS	1	//  settings.exe
#define BUILD_EXE_PLAYGATE	2	//  Playgate.exe


using wwiiolpath_string = std::wstring;
//  using wwiiolpath_string = std::u16string;

enum enumfiostructure
{
							//  ??_ = WW2_: WW2.exe,    ST_: setup.exe,    PG_: Playgate.exe
	IS_EXE_PATH,			//	??_exePath - Windows - EXE ROOT folder:  \Program * Files -> WWIIOnLine == *.exe, WWIIOnLine.ico, readme.*, unins*.* (uninstaller), *.tkt, *.script, bin(dir), DATA(dir), docs(dir), playarc(dir).
							//	??_exePath - macOS - EXE ROOT folder:  /Applications/WWIIOnline(package) == *.exe, WWIIOnLine.ico, readme.*, unins*.* (uninstaller), *.tkt, *.script, bin(dir), DATA(dir), docs(dir), playarc(dir).
//	IS_EXE_BIN_PATH,		//	??_exeBinPath - EXE DLL folder:  \Program * Files -> WWIIOnLine -> bin == *.lib, steam_appid.txt.;
	IS_EXE_DATA_PATH,		//	??_exeDataPath - EXE Support Data folder:  \Program * Files -> WWIIOnLine -> DATA == DirectXRedist(dir), loc(dir), playgate(dir), RuntimeLibraries(dir).
	IS_EXE_DOCS_PATH,		//	??_exeDocsPath - EXE Docs Folder:  \Program * Files -> WWIIOnLine -> doc == eula.txt, license.*, regions.txt, tos.txt.
	IS_EXE_PLAYARC_PATH,	//	??_exePlayarcPath - EXE playarc folder:  \Program * Files -> WWIIOnLine -> playarc == playarc??.ww2  --OR--  == "-data" command line switch.
	IS_EXE_WORKING_PATH,	//	??_exeWorkingPath - Windows - EXE Working folder:  \ProgramData -> WWIIOnLine == ftplog.txt, downloaded patches, WWIIOL.log, *.log.
							//	??_exeWorkingPath - macOS - EXE Working folder:  /Library/Application Support//WWIIOnline == ftplog.txt, downloaded patches, WWIIOL.log, *.log.
	IS_USER_CONFIG_PATH,	//	??_userConfigPath - Windows - USER Config ROOT folder:  \User -> My Documents -> WWIIOnLine == wwiiol.xml, cfml(dir).
							//	??_userConfigPath - macOS - USER Config ROOT folder:  /Users/"currentuser"/Documents/WWIIOnline == wwiiol.xml, cfml(dir).
	IS_USER_WORKING_PATH,	//	??_userWorkingPath - USER Working folder:  \User -> My Documents -> WWIIOnLine -> bin == lg.dat.
	IS_ALL_PATHS			//  All of the folders.
};

LOGICAL fioStructureInit();
LOGICAL fioStructureVerify();
unsigned int Wstring2Char(wwiiolpath_string sourceString, char *destinationString, const size_t sizeLimit);
unsigned int Char2Wstring(char * sourceString, wwiiolpath_string &destinationString, const size_t sizeLimit);
bool GetExePath(wwiiolpath_string &outFullPath, wwiiolpath_string &outExeName);
void GetWC(const char *sourceString, wwiiolpath_string &destinationString);


#endif  //  __FileIOStructure__