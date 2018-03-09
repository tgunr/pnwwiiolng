
//  fiostructure.cpp.
//  Date: Feb-27-2018
//
//  Copyright (c) 2018 Playnet, Inc., All Rights Reserved.
//  Not to be distributed in API form except as part of a
//  compiled library.
//
//  MJH - Feburary 27, 2018.


//  ==============================================================================================================

//  Notice:
//		The Paths as stored,   DO NOT   have the folder separater at the end of the last part of the pathname.
//		This   MUST   be added when appending further path segmints to a stored Paths.

//  ==============================================================================================================

#include "syscfg.h" // Path is project specific.

#include <string>
#include "fiostructure.h"

#if platOS == OS_WIN32
#include <shlobj.h>

#define errno (*_errno())
#endif

wwiiolpath_string	WWIIOnlineDirName = L"WWIIOnLine";	//  The ROOT folder for the game install -> WWIIOnLine

//  EXE_BUILD == BUILD_EXE_????:    is defined (win) in the Project Properties -> C/C++ -> Preprossor -> Preprossor Definitions. BUILD_EXE_SETTINGS = settings.exe, BUILD_EXE_PLAYGATE = Playgate.exe, BUILD_EXE_WW2 = WW2.exe.
#if EXE_BUILD == BUILD_EXE_SETTINGS
//  settings.exe
wwiiolpath_string ST_exePath = L"";				//	??_exePath - EXE ROOT folder:  \Program * Files -> WWIIOnLine == *.exe, WWIIOnLine.ico, readme.*, unins*.* (uninstaller), *.tkt, *.script, bin(dir), DATA(dir), docs(dir), playarc(dir).
wwiiolpath_string ST_exeBinPath = L"";			//	??_exeBinPath - EXE DLL folder:  \Program * Files -> WWIIOnLine -> bin == *.lib, steam_appid.txt.
wwiiolpath_string ST_exeDataPath = L"";			//	??_exeDataPath - EXE Support Data folder:  \Program * Files -> WWIIOnLine -> DATA == DirectXRedist(dir), loc(dir), playgate(dir), RuntimeLibraries(dir). 
wwiiolpath_string ST_exeDocsPath = L"";			//	??_exeDocsPath - EXE Docs Folder:  \Program * Files -> WWIIOnLine -> docs == eula.txt, license.*, regions.txt, tos.txt.
wwiiolpath_string ST_exePlayarcPath = L"";		//	??_exePlayarcPath - EXE playarc folder:  \Program * Files -> WWIIOnLine -> playarc == playarc??.ww2  --OR--  == "-data" command line switch.
wwiiolpath_string ST_exeWorkingPath = L"";		//	??_exeWorkingPath - EXE Working folder:  \ProgramData -> WWIIOnLine == ftplog.txt, downloaded patches, WWIIOL.log, *.log.
wwiiolpath_string ST_userConfigPath = L"";		//	??_userConfigPath - USER Config ROOT folder:  \User -> My Documents -> WWIIOnLine == wwiiol.xml, cfml(dir).
wwiiolpath_string ST_userWorkingPath = L"";		//	??_userWorkingPath - USER Working folder:  \User -> My Documents -> WWIIOnLine -> bin == lg.dat.

LOGICAL ST_exePathSet = FALSE;
LOGICAL ST_exeBinPathSet = FALSE;
LOGICAL ST_exeDataPathSet = FALSE;
LOGICAL ST_exeDocsPathSet = FALSE;
LOGICAL ST_exePlayarcPathSet = FALSE;
LOGICAL ST_exeWorkingPathSet = FALSE;
LOGICAL ST_userConfigPathSet = FALSE;
LOGICAL ST_userWorkingPathSet = FALSE;

#elif EXE_BUILD == BUILD_EXE_PLAYGATE
//  Playgate.exe
wwiiolpath_string PG_exePath = L"";				//	??_exePath - EXE ROOT folder:  \Program * Files -> WWIIOnLine == *.exe, WWIIOnLine.ico, readme.*, unins*.* (uninstaller), *.tkt, *.script, bin(dir), DATA(dir), docs(dir), playarc(dir).
wwiiolpath_string PG_exeBinPath = L"";			//	??_exeBinPath - EXE DLL folder:  \Program * Files -> WWIIOnLine -> bin == *.lib, steam_appid.txt.
wwiiolpath_string PG_exeDataPath = L"";			//	??_exeDataPath - EXE Support Data folder:  \Program * Files -> WWIIOnLine -> DATA == DirectXRedist(dir), loc(dir), playgate(dir), RuntimeLibraries(dir). 
wwiiolpath_string PG_exeDocsPath = L"";			//	??_exeDocsPath - EXE Docs Folder:  \Program * Files -> WWIIOnLine -> docs == eula.txt, license.*, regions.txt, tos.txt.
wwiiolpath_string PG_exePlayarcPath = L"";		//	??_exePlayarcPath - EXE playarc folder:  \Program * Files -> WWIIOnLine -> playarc == playarc??.ww2  --OR--  == "-data" command line switch.
wwiiolpath_string PG_exeWorkingPath = L"";		//	??_exeWorkingPath - EXE Working folder:  \ProgramData -> WWIIOnLine == ftplog.txt, downloaded patches, WWIIOL.log, *.log.
wwiiolpath_string PG_userConfigPath = L"";		//	??_userConfigPath - USER Config ROOT folder:  \User -> My Documents -> WWIIOnLine == wwiiol.xml, cfml(dir).
wwiiolpath_string PG_userWorkingPath = L"";		//	??_userWorkingPath - USER Working folder:  \User -> My Documents -> WWIIOnLine -> bin == lg.dat.

LOGICAL PG_exePathSet = FALSE;
LOGICAL PG_exeBinPathSet = FALSE;
LOGICAL PG_exeDataPathSet = FALSE;
LOGICAL PG_exeDocsPathSet = FALSE;
LOGICAL PG_exePlayarcPathSet = FALSE;
LOGICAL PG_exeWorkingPathSet = FALSE;
LOGICAL PG_userConfigPathSet = FALSE;
LOGICAL PG_userWorkingPathSet = FALSE;

#else
//  EXE_BUILD == BUILD_EXE_WW2
//  WW2.exe
wwiiolpath_string WW2_exePath = L"";			//	??_exePath - EXE ROOT folder:  \Program * Files -> WWIIOnLine == *.exe, WWIIOnLine.ico, readme.*, unins*.* (uninstaller), *.tkt, *.script, bin(dir), DATA(dir), docs(dir), playarc(dir).
wwiiolpath_string WW2_exeBinPath = L"";			//	??_exeBinPath - EXE DLL folder:  \Program * Files -> WWIIOnLine -> bin == *.lib, steam_appid.txt.
wwiiolpath_string WW2_exeDataPath = L"";		//	??_exeDataPath - EXE Support Data folder:  \Program * Files -> WWIIOnLine -> DATA == DirectXRedist(dir), loc(dir), playgate(dir), RuntimeLibraries(dir). 
wwiiolpath_string WW2_exeDocsPath = L"";		//	??_exeDocsPath - EXE Docs Folder:  \Program * Files -> WWIIOnLine -> docs == eula.txt, license.*, regions.txt, tos.txt.
wwiiolpath_string WW2_exePlayarcPath = L"";		//	??_exePlayarcPath - EXE playarc folder:  \Program * Files -> WWIIOnLine -> playarc == playarc??.ww2  --OR--  == "-data" command line switch.
wwiiolpath_string WW2_exeWorkingPath = L"";		//	??_exeWorkingPath - EXE Working folder:  \ProgramData -> WWIIOnLine == ftplog.txt, downloaded patches, WWIIOL.log, *.log.
wwiiolpath_string WW2_userConfigPath = L"";		//	??_userConfigPath - USER Config ROOT folder:  \User -> My Documents -> WWIIOnLine == wwiiol.xml, cfml(dir).
wwiiolpath_string WW2_userWorkingPath = L"";	//	??_userWorkingPath - USER Working folder:  \User -> My Documents -> WWIIOnLine -> bin == lg.dat.

LOGICAL WW2_exePathSet = FALSE;
LOGICAL WW2_exeBinPathSet = FALSE;
LOGICAL WW2_exeDataPathSet = FALSE;
LOGICAL WW2_exeDocsPathSet = FALSE;
LOGICAL WW2_exePlayarcPathSet = FALSE;
LOGICAL WW2_exeWorkingPathSet = FALSE;
LOGICAL WW2_userConfigPathSet = FALSE;
LOGICAL WW2_userWorkingPathSet = FALSE;

#endif


/************************************************************************************************
fioStructureInit - sets up the data directory structure on clients
*************************************************************************************************/

LOGICAL
fioStructureInit()
{
#if platOS == OS_WIN32
	//  Returns true if NO errors were encounered.

	LOGICAL fioStructureInit_Value = TRUE;
	wchar_t wBuffer[MAX_PATH + 1];
	int returnResult = -10;

	//												??_  =  WW2_: WW2.exe,    ST_: setup.exe,    PG_: Playgate.exe
	wwiiolpath_string exePath = L"";			//	??_exePath - EXE ROOT folder:  \Program * Files -> WWIIOnLine == *.exe, WWIIOnLine.ico, readme.*, unins*.* (uninstaller), *.tkt, *.script, bin(dir), DATA(dir), docs(dir), playarc(dir).
	wwiiolpath_string exeBinPath = L"";			//	??_exeBinPath - EXE DLL folder:  \Program * Files -> WWIIOnLine -> bin == *.lib, steam_appid.txt.
	wwiiolpath_string exeDataPath = L"";		//	??_exeDataPath - EXE Support Data folder:  \Program * Files -> WWIIOnLine -> DATA == DirectXRedist(dir), loc(dir), playgate(dir), RuntimeLibraries(dir).
	wwiiolpath_string exeDocsPath = L"";		//	??_exeDocsPath - EXE Docs Folder:  \Program * Files -> WWIIOnLine -> docs == eula.txt, license.*, regions.txt, tos.txt.
	wwiiolpath_string exePlayarcPath = L"";		//	??_exePlayarcPath - EXE playarc folder:  \Program * Files -> WWIIOnLine -> playarc == playarc??.ww2  --OR--  == "-data" command line switch.
	wwiiolpath_string exeWorkingPath = L"";		//	??_exeWorkingPath - EXE Working folder:  \ProgramData -> WWIIOnLine == ftplog.txt, downloaded patches, WWIIOL.log, *.log.
	wwiiolpath_string userConfigPath = L"";		//	??_userConfigPath - USER Config ROOT folder:  \User -> My Documents -> WWIIOnLine == wwiiol.xml, cfml(dir).
	wwiiolpath_string userWorkingPath = L"";	//	??_userWorkingPath - USER Working folder:  \User -> My Documents -> WWIIOnLine -> bin == lg.dat.
	
	LOGICAL exePathSet = FALSE;
	LOGICAL exeBinPathSet = FALSE;
	LOGICAL exeDataPathSet = FALSE;
	LOGICAL exeDocsPathSet = FALSE;
	LOGICAL exePlayarcPathSet = FALSE;
	LOGICAL exeWorkingPathSet = FALSE;
	LOGICAL userConfigPathSet = FALSE;
	LOGICAL userWorkingPathSet = FALSE;

	const wwiiolpath_string DirNameBIN = L"\\bin";			//  -> bin
	const wwiiolpath_string DirNameDATA = L"\\DATA";		//  -> DATA
	const wwiiolpath_string DirNameDOCS = L"\\docs";		//  -> docs
	const wwiiolpath_string DirNamePLAYARC = L"\\playarc";	//  -> playarc


	//  exePath: Setup - START.
	//	??_exePath - EXE ROOT folder:  \Program * Files -> WWIIOnLine == *.exe, WWIIOnLine.ico, readme.*, unins*.* (uninstaller), *.tkt, *.script, bin(dir), DATA(dir), docs(dir), playarc(dir).
	//  This folder should be setup by the installer.
	//  No Fallback.
	
	wBuffer[0] = 0;
	if (GetCurrentDirectory(MAX_PATH, wBuffer) > 0)
	{
		exePath = wBuffer;
		exePathSet= TRUE;
	}
	else
	{
		exePath = L"";
		exePathSet = FALSE;

		//  dbugLog(_FLA, "EXE ROOT folder setup failed: &HFFFF.  This should NOT happen.");
		fioStructureInit_Value = FALSE;

		goto ERR_EXIT_POINT;
	}
	//  exePath: Setup - END.



	//  userConfigPath - START.
	//  WW2_userConfigPath - \User -> My Documents -> WWIIOnLine == wwiiol.xml, lg.dat, cfml(dir).
	//  This folder should be setup by the installer.
	//  No Fallback.

	//  Ask Windows where the users documents folder is; oh, and create it if it doesn't exist
	//  because we'll be needing it.
	wBuffer[0] = 0;
	if (SHGetFolderPath(nullptr, CSIDL_PERSONAL | CSIDL_FLAG_CREATE, nullptr, SHGFP_TYPE_CURRENT, wBuffer) == S_OK)
	{
		userConfigPath = wBuffer;
		userConfigPath += L"\\";
		userConfigPath += WWIIOnlineDirName;

		//  make sure it exists
		//  Returns:
		//  0 = Directory created.
		//  -1 = Directory not created error state. [errno] set to the error code.
		returnResult = -10;
		_set_errno(0);
		returnResult = _wmkdir(userConfigPath.c_str());

		if (returnResult == -1)
		{
			if (errno == EEXIST)
			{
				//  File/Directory Already Exists.
				userConfigPathSet = TRUE;
			}
			else
			{
				//  errno = ENOENT - Path was not found.
				userConfigPath = L"";
				userConfigPathSet = FALSE;

				//  dbugLog(_FLW, "SHGetFolderPath(CSIDL_PERSONAL) failed.  This folder should be setup by the installer.");
				fioStructureInit_Value = FALSE;
			}
		}
		else
		{
			userConfigPathSet= TRUE;
		}
	}
	else
	{
		userConfigPath = L"";
		userConfigPathSet = FALSE;

		//  dbugLog(_FLA, "SHGetFolderPath(CSIDL_PERSONAL) failed: &HFFFE.  This should NOT happen.");
		fioStructureInit_Value = FALSE;

		goto ERR_EXIT_POINT;
	}
	//  userConfigPath - END.



	//  exeWorkingPath - START.
	//	??_exeWorkingPath - EXE Working folder:  \ProgramData -> WWIIOnLine == ftplog.txt, downloaded patches, WWIIOL.log, *.log.
	//  This folder should be setup by the installer.
	//  Fallback to: \User -> My Documents -> WWIIOnLine -> bin.

	//  Ask Windows where the ProgramData folder is; oh, and attempt create it if it doesn't exist
	//  because we'll be needing it.
	wBuffer[0] = 0;
	if (SHGetFolderPath(nullptr, CSIDL_COMMON_APPDATA | CSIDL_FLAG_CREATE, nullptr, SHGFP_TYPE_CURRENT, wBuffer) == S_OK)
	{
		exeWorkingPath = wBuffer;
		exeWorkingPath += L"\\";
		exeWorkingPath += WWIIOnlineDirName;

		//  make sure it exists
		//  Returns:
		//  0 = Directory created.
		//  -1 = Directory not created error state. [errno] set to the error code.
		returnResult = -10;
		_set_errno(0);
		returnResult = _wmkdir(exeWorkingPath.c_str());

		if (returnResult == -1)
		{
			if (errno == EEXIST)
			{
				//  File/Directory Already Exists.
				exeWorkingPathSet= TRUE;
			}
			else if (errno == ENOENT)
			{
				//  Path was not found, fallingback.
				//  Unable to create ProgramData directory. If the Installer (or someone else) has already created, all is ok.
				exeWorkingPathSet = FALSE;

				//  Attempting to use: WW2_userConfigPath - \User -> My Documents -> WWIIOnLine -> bin.
				//  Fallback to: EXE ROOT folder.
				if (userConfigPathSet = TRUE)
				{
					exeWorkingPath = userConfigPath;
					//  exeWorkingPath += L"\\Bin";
					exeWorkingPath += DirNameBIN;

					//  make sure it exists
					returnResult = -10;
					_set_errno(0);
					returnResult = _wmkdir(exeWorkingPath.c_str());
					if (returnResult == -1)
					{
						if (errno == EEXIST)
						{
							//  File/Directory Already Exists.
							exeWorkingPathSet = TRUE;
						}
						else
						{
							//  errno = ENOENT - Path was not found.

							exeWorkingPath = exePath;
							exeWorkingPathSet = TRUE;

							//  dbugLog(_FLI, "SHGetFolderPath(CSIDL_PERSONAL) -> bin failed.  Setting to EXE ROOT folder.");
							fioStructureInit_Value = FALSE;
						}
					}
					else
					{
						exeWorkingPathSet = TRUE;
					}
				}
				else
				{
					exeWorkingPath = exePath;
					exeWorkingPathSet = TRUE;

					//  dbugLog(_FLI, "SHGetFolderPath(CSIDL_PERSONAL) -> bin failed.  Setting to EXE ROOT folder.");
					fioStructureInit_Value = FALSE;
				}
			}
			else
			{
				exeWorkingPath = exePath;
				exeWorkingPathSet = TRUE;

				//  dbugLog(_FLI, "SHGetFolderPath(CSIDL_PERSONAL) -> bin failed.  Setting to EXE ROOT folder.");
				fioStructureInit_Value = FALSE;
			}
		}
		else
		{
			exeWorkingPathSet = TRUE;
		}
	}
	else
	{
		exeWorkingPath = L"";
		exeWorkingPathSet = FALSE;

		//  dbugLog(_FLA, "SHGetFolderPath(CSIDL_COMMON_APPDATA) failed: &HFFFD.  This should NOT happen.");
		fioStructureInit_Value = FALSE;

		goto ERR_EXIT_POINT;
	}
	//  exeWorkingPath - END.



	//  exeBinPath - START.
	//	??_exeBinPath - EXE DLL folder:  \Program * Files -> WWIIOnLine -> bin == *.lib, steam_appid.txt.
	//  This folder should be setup by the installer.
	//  Fallback to: EXE ROOT folder.

	exeBinPath = exePath;
	//  exeBinPath += L"\\bin";
	exeBinPath += DirNameBIN;

	//  make sure it exists
	//  Returns:
	//  0 = Directory created.
	//  -1 = Directory not created error state. [errno] set to the error code.
	returnResult = -10;
	_set_errno(0);
	returnResult = _wmkdir(exeBinPath.c_str());

	if (returnResult == -1)
	{
		if (errno == EEXIST)
		{
			//  File/Directory Already Exists.
			exeBinPathSet = TRUE;
		}
		else
		{
			//  errno = ENOENT - Path was not found.

			exeBinPath = exePath;
			exeBinPathSet = TRUE;
			
			//  dbugLog(_FLI, "EXE ROOT folder -> bin failed.  Setting to EXE ROOT folder.");
			fioStructureInit_Value = FALSE;
		}
	}
	else
	{
		exeBinPathSet = TRUE;
	}
	//  exeBinPath - END.



	//  exeDataPath: Setup - START.
	//  This folder should be setup by the installer.

	//	??_exeDataPath - EXE Support Data folder:  \Program * Files -> WWIIOnLine -> DATA == DirectXRedist(dir), loc(dir), playgate(dir), RuntimeLibraries(dir).
	exeDataPath = exePath;
	//  exeDataPath += L"\\DATA";
	exeDataPath += DirNameDATA;

	//  make sure it exists
	//  Returns:
	//  0 = Directory created.
	//  -1 = Directory not created error state. [errno] set to the error code.
	returnResult = -10;
	_set_errno(0);
	returnResult = _wmkdir(exeDataPath.c_str());

	if (returnResult == -1)
	{
		if (errno == EEXIST)
		{
			//  File/Directory Already Exists.
			exeDataPathSet = TRUE;
		}
		else
		{
			//  if errno = ENOENT - Path was not found.

			exeDataPath = exePath;
			exeDataPathSet = TRUE;

			//  dbugLog(_FLI, "EXE ROOT folder -> DATA failed.  Setting to EXE ROOT folder.");
			fioStructureInit_Value = FALSE;
		}
	}
	else
	{
		exeDataPathSet = TRUE;
	}
	//  exeDataPath: Setup - END.



	//  exeDocsPath: Setup - START.
	//  This folder should be setup by the installer.

	//	??_exeDocsPath - EXE Docs Folder:  \Program * Files -> WWIIOnLine -> doc == eula.txt, license.*, regions.txt, tos.txt.
	exeDocsPath = exePath;
	//  exeDocsPath += L"\\docs";
	exeDocsPath += DirNameDOCS;

	//  make sure it exists
	//  Returns:
	//  0 = Directory created.
	//  -1 = Directory not created error state. [errno] set to the error code.
	returnResult = -10;
	_set_errno(0);
	returnResult = _wmkdir(exeDocsPath.c_str());

	if (returnResult == -1)
	{
		if (errno == EEXIST)
		{
			//  File/Directory Already Exists.
			exeDocsPathSet = TRUE;
		}
		else
		{
			//  errno = ENOENT - Path was not found.

			exeDocsPath = exePath;
			exeDocsPathSet = TRUE;

			//  dbugLog(_FLI, "EXE ROOT folder -> docs failed.  Setting to EXE ROOT folder.");
			fioStructureInit_Value = FALSE;
		}
	}
	else
	{
		exeDocsPathSet = TRUE;
	}
	//  exeDocsPath: Setup - END.



	//  exePlayarcPath: Setup - START.
	//  This folder should be setup by the installer.

	//	??_exePlayarcPath - EXE playarc folder:  \Program * Files -> WWIIOnLine -> playarc == playarc??.ww2  -- OR --  == "-data" command line switch.
	exePlayarcPath = exePath;
	//  exePlayarcPath += L"\\playarc";
	exePlayarcPath += DirNamePLAYARC;

	//  make sure it exists
	//  Returns:
	//  0 = Directory created.
	//  -1 = Directory not created error state. [errno] set to the error code.
	returnResult = -10;
	_set_errno(0);
	returnResult = _wmkdir(exePlayarcPath.c_str());

	if (returnResult == -1)
	{
		if (errno == EEXIST)
		{
			//  File/Directory Already Exists.
			exePlayarcPathSet = TRUE;
		}
		else
		{
			//  if errno = ENOENT - Path was not found.

			exePlayarcPath = exePath;
			exePlayarcPathSet = TRUE;

			//  dbugLog(_FLI, "EXE ROOT folder -> playarc failed.  Setting to EXE ROOT folder.");
			fioStructureInit_Value = FALSE;
		}
	}
	else
	{
		exePlayarcPathSet = TRUE;
	}
	//  exePlayarcPath: Setup - END.



	//  userWorkingPath: Setup - START.
	//  This folder should be setup by the installer.

	//	??_userWorkingPath - USER Working folder:  \User -> My Documents -> WWIIOnLine -> bin == lg.dat.
	userWorkingPath = userConfigPath;
	//  userWorkingPath += L"\\bin";
	userWorkingPath += DirNameBIN;

	//  make sure it exists
	//  Returns:
	//  0 = Directory created.
	//  -1 = Directory not created error state. [errno] set to the error code.
	returnResult = -10;
	_set_errno(0);
	returnResult = _wmkdir(userWorkingPath.c_str());

	if (returnResult == -1)
	{
		if (errno == EEXIST)
		{
			//  File/Directory Already Exists.
			userWorkingPathSet = TRUE;
		}
		else
		{
			//  errno = ENOENT - Path was not found.

			userWorkingPath = exePath;
			userWorkingPathSet = TRUE;

			//  dbugLog(_FLI, "EXE ROOT folder -> playarc failed.  Setting to EXE ROOT folder.");
			fioStructureInit_Value = FALSE;
		}
	}
	else
	{
		userWorkingPathSet = TRUE;
	}
	//  userWorkingPath: Setup - END.


ERR_EXIT_POINT:    //  ==========    <<--    The goto target is HERE    -->>    ==========    <<<<<<<<========<<<<<<<<


#if EXE_BUILD == BUILD_EXE_SETTINGS
	ST_exePath = exePath;
	ST_exeBinPath = exeBinPath;
	ST_exeDataPath = exeDataPath;
	ST_exeDocsPath = exeDocsPath;
	ST_exePlayarcPath = exePlayarcPath;
	ST_exeWorkingPath = exeWorkingPath;
	ST_userConfigPath = userConfigPath;
	ST_userWorkingPath = userWorkingPath;

	ST_exePathSet = exePathSet;
	ST_exeBinPathSet = exeBinPathSet;
	ST_exeDataPathSet = exeDataPathSet;
	ST_exeDocsPathSet = exeDocsPathSet;
	ST_exePlayarcPathSet = exePlayarcPathSet;
	ST_exeWorkingPathSet = exeWorkingPathSet;
	ST_userConfigPathSet = userConfigPathSet;
	ST_userWorkingPathSet = userWorkingPathSet;

#elif EXE_BUILD == BUILD_EXE_PLAYGATE
	PG_exePath = exePath;
	PG_exeBinPath = exeBinPath;
	PG_exeDataPath = exeDataPath;
	PG_exeDocsPath = exeDocsPath;
	PG_exePlayarcPath = exePlayarcPath;
	PG_exeWorkingPath = exeWorkingPath;
	PG_userConfigPath = userConfigPath;
	PG_userWorkingPath = userWorkingPath;

	PG_exePathSet = exePathSet;
	PG_exeBinPathSet = exeBinPathSet;
	PG_exeDataPathSet = exeDataPathSet;
	PG_exeDocsPathSet = exeDocsPathSet;
	PG_exePlayarcPathSet = exePlayarcPathSet;
	PG_exeWorkingPathSet = exeWorkingPathSet;
	PG_userConfigPathSet = userConfigPathSet;
	PG_userWorkingPathSet = userWorkingPathSet;

#else
	//   EXE_BUILD == BUILD_EXE_WW2
	WW2_exePath = exePath;
	WW2_exeBinPath = exeBinPath;
	WW2_exeDataPath = exeDataPath;
	WW2_exeDocsPath = exeDocsPath;
	WW2_exePlayarcPath = exePlayarcPath;
	WW2_exeWorkingPath = exeWorkingPath;
	WW2_userConfigPath = userConfigPath;
	WW2_userWorkingPath = userWorkingPath;

	WW2_exePathSet = exePathSet;
	WW2_exeBinPathSet = exeBinPathSet;
	WW2_exeDataPathSet = exeDataPathSet;
	WW2_exeDocsPathSet = exeDocsPathSet;
	WW2_exePlayarcPathSet = exePlayarcPathSet;
	WW2_exeWorkingPathSet = exeWorkingPathSet;
	WW2_userConfigPathSet = userConfigPathSet;
	WW2_userWorkingPathSet = userWorkingPathSet;
#endif

	if (fioStructureInit_Value == FALSE)
	{
		wwiiolpath_string tmp = WWIIOnlineDirName;
		tmp += L" - File System.";

		MessageBox(nullptr, L"Please check your Client Install.\n\nShutting down.", tmp.c_str(), MB_ICONSTOP | MB_OK);
		exit(0);
	}

#elif platOS == OS_MACOS
	// TODO: Need a Mac guy to verify this will work nicely
	//  strsafecpy(UserDataPath, macDocumentsPath());  //  MJH - Feburary 27, 2018.
	strsafecpy(userConfigPath, macDocumentsPath());
#endif

	return fioStructureInit_Value;
}

LOGICAL
fioStructureVerify()
{
	LOGICAL fioStructureVerify_Value = FALSE;

	return fioStructureVerify_Value;
}

void
fioStructureForwadDeclare()
{

	return;

// Forwad Declare information store
/*
extern wwiiolpath_string	WWIIOnlineDirName;		//  The ROOT folder for the game install - .\WWIIOnLine

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
	extern wwiiolpath_string WW2_exeDocsPath;		//	??_exeDocsPath - EXE Docs Folder:  \Program * Files -> WWIIOnLine -> docs == eula.txt, license.*, regions.txt, tos.txt.
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
*/
}