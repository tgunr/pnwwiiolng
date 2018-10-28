
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

LOGICAL fioStructureInit();
LOGICAL fioStructureVerify();
unsigned long Wstring2Char(wwiiolpath_string sourceString, char *destinationString, const size_t sizeLimit);
unsigned long Wstring2Char(wwiiolpath_string sourceString, wchar_t *destinationString, const size_t sizeLimit);
unsigned long Char2Wstring(char * sourceString, wwiiolpath_string &destinationString, const size_t sizeLimit);
bool GetExePath(wwiiolpath_string &outFullPath, wwiiolpath_string &outExeName);
void GetWC(const char *sourceString, wwiiolpath_string &destinationString);
wwiiolpath_string fioGetCurrentWorkingPath();
LOGICAL fioSetCurrentWorkingPath(wwiiolpath_string targetPath);
void fioStructureForwadDeclare();  //  Dosnt "DO" anything.
wwiiolpath_string SQL_DateTimeStamp();  //  Returns an SQL Formatted Date/Time Stamp string. YYYYMMDD-HHMMSS
bool CopyOldSetupFiles_WIN();
bool SHFileFunctions_WC(wwiiolpath_string sourcePath, wwiiolpath_string destinationPath, unsigned int FO_Function);  //  Used within CopyOldSetupFiles_WIN().


#endif  //  __FileIOStructure__
