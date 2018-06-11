
//  fiostructure.cpp.
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

#include "syscfg.h" // Path is project specific.

#include <string>
#include "fiostructure.h"

#if platOS == OS_WIN32
#include <shlobj.h>
#define errno (*_errno())
#endif

wwiiolpath_string WantedDataPath = L"";					//  SET by the command line - ENABLED by WantDataDir = True < OR > is a Nul String L"".
wwiiolpath_string WWIIOnlineDirName = L"WWIIOnLine";	//  The ROOT folder for the game install - .\WWIIOnLine

wwiiolpath_string WW2_exePath;			//	??_exePath - Windows - EXE ROOT folder:  \Program * Files -> WWIIOnLine == *.exe, WWIIOnLine.ico, readme.*, unins*.* (uninstaller), *.tkt, *.script, bin(dir), DATA(dir), docs(dir), playarc(dir).
										//	??_exePath - macOS - EXE ROOT folder:  /Applications/WWIIOnline(package) == *.exe, WWIIOnLine.ico, readme.*, *.tkt, *.script, bin(dir), DATA(dir), docs(dir), playarc(dir).
wwiiolpath_string WW2_exeDataPath;		//	??_exeDataPath - EXE Support Data folder:  \Program * Files -> WWIIOnLine -> DATA == DirectXRedist(dir), loc(dir), playgate(dir), RuntimeLibraries(dir). 
wwiiolpath_string WW2_exeDocsPath;		//	??_exeDocsPath - EXE Docs Folder:  \Program * Files -> WWIIOnLine -> docs == eula.txt, license.*, regions.txt, tos.txt.
wwiiolpath_string WW2_exePlayarcPath;	//	??_exePlayarcPath - EXE playarc folder:  \Program * Files -> WWIIOnLine -> playarc == playarc??.ww2  --OR--  == "-data" command line switch.
wwiiolpath_string WW2_exeWorkingPath;	//	??_exeWorkingPath - Windows - EXE Working folder:  \ProgramData -> WWIIOnLine == ftplog.txt, downloaded patches, WWIIOL.log, *.log.
										//	??_exeWorkingPath - macOS - EXE Working folder:  /Library -> Application Support -> WWIIOnline == ftplog.txt, downloaded patches, WWIIOL.log, *.log.
wwiiolpath_string WW2_userConfigPath;	//	??_userConfigPath - Windows - USER Config ROOT folder:  \User -> My Documents -> WWIIOnLine == wwiiol.xml, cfml(dir).
										//	??_userConfigPath - macOS - USER Config ROOT folder:  /Users -> "currentuser" -> Documents -> WWIIOnline == wwiiol.xml, cfml(dir).
wwiiolpath_string WW2_userWorkingPath;	//	??_userWorkingPath - USER Working folder:  \User -> My Documents -> WWIIOnLine -> bin == lg.dat.


/************************************************************************************************
fioStructureInit - sets up the data directory structure on clients
*************************************************************************************************/

LOGICAL fioStructureInit()
{
    LOGICAL fioStructureInit_Value = TRUE;

    LOGICAL exePathSet = FALSE;
    //	LOGICAL exeBinPathSet = FALSE;
    LOGICAL exeDataPathSet = FALSE;
    LOGICAL exeDocsPathSet = FALSE;
    LOGICAL exePlayarcPathSet = FALSE;
    LOGICAL exeWorkingPathSet = FALSE;
    LOGICAL userConfigPathSet = FALSE;
    LOGICAL userWorkingPathSet = FALSE;
    
    const wwiiolpath_string WW2filename = L"\\WW2.exe";		// \\WW2.exe
    const wwiiolpath_string DirNameBIN = L"\\bin";			//  -> bin
    const wwiiolpath_string DirNameDATA = L"\\DATA";		//  -> DATA
    const wwiiolpath_string DirNameDOCS = L"\\docs";		//  -> docs
    const wwiiolpath_string DirNamePLAYARC = L"\\playarc";	//  -> playarc

#if platOS == OS_WIN32
	//  Returns true if NO errors were encounered.

	wchar_t wBuffer[MAX_PATH + 1];
	int returnResult = -10;
	wwiiolpath_string exeFullPath = L"";
	wwiiolpath_string exeName = L"";
	bool  WW2exeExists = false;
	struct _stat buf;
	std::wstring filename = L"";

	wwiiolpath_string exePath = L"";			//	??_exePath - EXE ROOT folder:  \Program * Files -> WWIIOnLine == *.exe, WWIIOnLine.ico, readme.*, unins*.* (uninstaller), *.tkt, *.script, bin(dir), DATA(dir), docs(dir), playarc(dir).
//	wwiiolpath_string exeBinPath = L"";			//	??_exeBinPath - EXE DLL folder:  \Program * Files -> WWIIOnLine -> bin == *.lib, steam_appid.txt.
	wwiiolpath_string exeDataPath = L"";		//	??_exeDataPath - EXE Support Data folder:  \Program * Files -> WWIIOnLine -> DATA == DirectXRedist(dir), loc(dir), playgate(dir), RuntimeLibraries(dir).
	wwiiolpath_string exeDocsPath = L"";		//	??_exeDocsPath - EXE Docs Folder:  \Program * Files -> WWIIOnLine -> docs == eula.txt, license.*, regions.txt, tos.txt.
	wwiiolpath_string exePlayarcPath = L"";		//	??_exePlayarcPath - EXE playarc folder:  \Program * Files -> WWIIOnLine -> playarc == playarc??.ww2  --OR--  == "-data" command line switch.
	wwiiolpath_string exeWorkingPath = L"";		//	??_exeWorkingPath - EXE Working folder:  \ProgramData -> WWIIOnLine == ftplog.txt, downloaded patches, WWIIOL.log, *.log.
	wwiiolpath_string userConfigPath = L"";		//	??_userConfigPath - USER Config ROOT folder:  \User -> My Documents -> WWIIOnLine == wwiiol.xml, cfml(dir).
	wwiiolpath_string userWorkingPath = L"";	//	??_userWorkingPath - USER Working folder:  \User -> My Documents -> WWIIOnLine -> bin == lg.dat.
	

	//  exePath: Setup - START.
	//	??_exePath - EXE ROOT folder:  \Program * Files -> WWIIOnLine == *.exe, WWIIOnLine.ico, readme.*, unins*.* (uninstaller), *.tkt, *.script, bin(dir), DATA(dir), docs(dir), playarc(dir).
	//  This folder should be setup by the installer.
	//  No Fallback.
	
	//  wBuffer[0] = 0;
//	if (GetCurrentDirectory(MAX_PATH, wBuffer) > 0)
	if (GetExePath(exeFullPath, exeName) == true)
	{
		//  exePath = wBuffer;
		exePath = exeFullPath;
		exePathSet= TRUE;
	}
	else
	{
		exePath = L"";
		exePathSet = FALSE;

#if EXE_BUILD != BUILD_EXE_SETTINGS
		dbugLog(_FLA, "EXE ROOT folder setup failed: &HFFFF.  This should NOT happen.");
#endif
		fioStructureInit_Value = FALSE;
		goto ERR_EXIT_POINT;
	}

	//  Locate WW2.exe.
	filename = exePath + WW2filename;
	returnResult = -10;
	returnResult = _wstat(filename.c_str(), &buf);

	if (returnResult != 0)
	{
		wwiiolpath_string tmp = WWIIOnlineDirName + L" - File System.";

		switch (errno)
		{
		case ENOENT:
			//  printf("File %s not found.\n", filename);
			MessageBox(nullptr, L"Unable to locate WW2.exe.", tmp.c_str(), MB_ICONSTOP | MB_OK);
			break;
		case EINVAL:
			//  printf("Invalid parameter to _stat.\n");
			//  MessageBox(nullptr, L"Invalid parameter to _stat.", tmp.c_str(), MB_ICONSTOP | MB_OK);
			returnResult = -10;
			break;
		default:
			/* Should never be reached. */
			//  printf("Unexpected error in _stat.\n");
			//  MessageBox(nullptr, L"Unexpected error in _stat.", tmp.c_str(), MB_ICONSTOP | MB_OK);
			returnResult = -10;
		}
		WW2exeExists = false;
		fioStructureInit_Value = FALSE;
		goto ERR_EXIT_POINT;
	}
	else
	{
		returnResult = -10;
		WW2exeExists = true;
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
		userConfigPath += L"\\" + WWIIOnlineDirName;

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

#if EXE_BUILD != BUILD_EXE_SETTINGS
				dbugLog(_FLW, "SHGetFolderPath(CSIDL_PERSONAL) failed.  This folder should be setup by the installer.");
#endif
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

#if EXE_BUILD != BUILD_EXE_SETTINGS
		dbugLog(_FLA, "SHGetFolderPath(CSIDL_PERSONAL) failed: &HFFFE.  This should NOT happen.");
#endif
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
		exeWorkingPath += L"\\" + WWIIOnlineDirName;

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
				if (userConfigPathSet == TRUE)
				{
					exeWorkingPath = userConfigPath + DirNameBIN;

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

#if EXE_BUILD != BUILD_EXE_SETTINGS
							dbugLog(_FLI, "SHGetFolderPath(CSIDL_PERSONAL) -> bin failed.  Setting to EXE ROOT folder.");
#endif
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

#if EXE_BUILD != BUILD_EXE_SETTINGS
					dbugLog(_FLI, "SHGetFolderPath(CSIDL_PERSONAL) -> bin failed.  Setting to EXE ROOT folder.");
#endif
					fioStructureInit_Value = FALSE;
				}
			}
			else
			{
				exeWorkingPath = exePath;
				exeWorkingPathSet = TRUE;

#if EXE_BUILD != BUILD_EXE_SETTINGS
				dbugLog(_FLI, "SHGetFolderPath(CSIDL_PERSONAL) -> bin failed.  Setting to EXE ROOT folder.");
#endif
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

#if EXE_BUILD != BUILD_EXE_SETTINGS
		dbugLog(_FLA, "SHGetFolderPath(CSIDL_COMMON_APPDATA) failed: &HFFFD.  This should NOT happen.");
#endif
		fioStructureInit_Value = FALSE;

		goto ERR_EXIT_POINT;
	}
	//  exeWorkingPath - END.


/*
==========================
	//  exeBinPath - START.
	//	??_exeBinPath - EXE DLL folder:  \Program * Files -> WWIIOnLine -> bin == *.lib, steam_appid.txt.
	//  This folder should be setup by the installer.
	//  Fallback to: EXE ROOT folder.

	exeBinPath = exePath + DirNameBIN;

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
			
#if EXE_BUILD != BUILD_EXE_SETTINGS
			dbugLog(_FLI, "EXE ROOT folder -> bin failed.  Setting to EXE ROOT folder.");
#endif
			fioStructureInit_Value = FALSE;
		}
	}
	else
	{
		exeBinPathSet = TRUE;
	}
	//  exeBinPath - END.
==========================
*/


	//  exeDataPath: Setup - START.
	//  This folder should be setup by the installer.

	//	??_exeDataPath - EXE Support Data folder:  \Program * Files -> WWIIOnLine -> DATA == DirectXRedist(dir), loc(dir), playgate(dir), RuntimeLibraries(dir).
	exeDataPath = exePath + DirNameDATA;

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

#if EXE_BUILD != BUILD_EXE_SETTINGS
			dbugLog(_FLI, "EXE ROOT folder -> DATA failed.  Setting to EXE ROOT folder.");
#endif
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
	exeDocsPath = exePath + DirNameDOCS;

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

#if EXE_BUILD != BUILD_EXE_SETTINGS
			dbugLog(_FLI, "EXE ROOT folder -> docs failed.  Setting to EXE ROOT folder.");
#endif
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
	exePlayarcPath = exePath + DirNamePLAYARC;

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

#if EXE_BUILD != BUILD_EXE_SETTINGS
			dbugLog(_FLI, "EXE ROOT folder -> playarc failed.  Setting to EXE ROOT folder.");
#endif
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
	userWorkingPath = userConfigPath + DirNameBIN;

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

#if EXE_BUILD != BUILD_EXE_SETTINGS
			dbugLog(_FLI, "EXE ROOT folder -> playarc failed.  Setting to EXE ROOT folder.");
#endif
			fioStructureInit_Value = FALSE;
		}
	}
	else
	{
		userWorkingPathSet = TRUE;
	}
	//  userWorkingPath: Setup - END.


ERR_EXIT_POINT:    //  ==========    <<--    The goto target is HERE    -->>    ==========    <<<<<<<<========<<<<<<<<


	if (fioStructureInit_Value == FALSE)
	{
		wwiiolpath_string tmp = WWIIOnlineDirName + L" - File System.";

		MessageBox(nullptr, L"Please check your Client Install.\n\nShutting down.", tmp.c_str(), MB_ICONSTOP | MB_OK);
		exit(0);
	}
	else
	{
		WW2_exePath = exePath;
		//	WW2_exeBinPath = exeBinPath;
		WW2_exeDataPath = exeDataPath;
		WW2_exeDocsPath = exeDocsPath;
		WW2_exePlayarcPath = exePlayarcPath;
		WW2_exeWorkingPath = exeWorkingPath;
		WW2_userConfigPath = userConfigPath;
		WW2_userWorkingPath = userWorkingPath;
	}

#elif platOS == OS_MACOS
	// TODO: Need a Mac guy to verify this will work nicely
    
    const char *returnValue;
    fioStructureInit_Value = TRUE;
    
    wwiiolpath_string exePath = L"";			//	??_exePath - EXE ROOT folder:  /Applications -> World War II Online.app == Contents -> _CodeSignature(dir), Frameworks(dir), MacOS(dir -> exefile), Resources(dir) info.plist, PkgInfo and others.
    wwiiolpath_string exeDataPath = L"";		//	??_exeDataPath - EXE Support Data folder:  /Applications -> World War II Online.app -> Contents -> Resources == placeholder images (x2), playarc.ww2, steam_apid.txt, WWIIOnLine.ico, readme.*, eula.txt, license.*, regions.txt, tos.txt.
    wwiiolpath_string exePlayarcPath = L"";		//	??_exePlayarcPath - EXE playarc folder:  /Applications -> World War II Online.app -> Contents -> Resources -> playarc.ww2  --OR--  == "-data" command line switch.
    wwiiolpath_string exeWorkingPath = L"";		//	??_exeWorkingPath - EXE Working folder:  /Library -> Application Support -> WWIIOnline == ftplog.txt, downloaded patches, WWIIOL.log, *.log.
    wwiiolpath_string userConfigPath = L"";		//	??_userConfigPath - USER Config ROOT folder:  /Users -> "currentuser" -> Documents -> WWIIOnline == wwiiol.xml, cfml(dir).
    wwiiolpath_string userWorkingPath = L"";	//	??_userWorkingPath - USER Working folder:  /Users -> "currentuser" -> Documents -> WWIIOnline -> bin == lg.dat.
    
    returnValue = macAppPath();
    if (returnValue != NULL)
    {
        //  /Applications -> World War II Online.app
        wwiiolpath_string appPath(&returnValue[0], &returnValue[strlen(returnValue)]);
        exePath = appPath;
        exePathSet = TRUE;
    }
    else
    {
        fioStructureInit_Value = FALSE;
    }

    returnValue = macDataPath();
    if (returnValue != NULL)
    {
        //  /Applications -> World War II Online.app -> Contents -> Resources
        wwiiolpath_string dataPath(&returnValue[0], &returnValue[strlen(returnValue)]);
        exeDataPath = dataPath;
        exeDataPathSet = TRUE;
    }
    else
    {
        fioStructureInit_Value = FALSE;
    }

    returnValue = macPlayarcPath();
    if (returnValue != NULL)
    {
        //  /Applications -> World War II Online.app -> Contents -> Resources -> playarc.ww2
        wwiiolpath_string playarcPath(&returnValue[0], &returnValue[strlen(returnValue)]);
        exePlayarcPath = playarcPath;
        exePlayarcPathSet = TRUE;
    }
    else
    {
        fioStructureInit_Value = FALSE;
    }
    
    returnValue = macWorkingPathAPP();
    if (returnValue != NULL)
    {
        //  /Library -> Application Support -> WWIIOnline
        wwiiolpath_string workingPathAPP(&returnValue[0], &returnValue[strlen(returnValue)]);
        exeWorkingPath = workingPathAPP;
        exeWorkingPathSet = TRUE;
    }
    else
    {
        fioStructureInit_Value = FALSE;
    }
   
    returnValue = macCrashLogsPath();
    if (returnValue != NULL)
    {
        //  Creates Folder ONLY.
        
        //  /Library -> Application Support -> WWIIOnline -> CrashLogs
        wwiiolpath_string tempPathAPP(&returnValue[0], &returnValue[strlen(returnValue)]);
        //  exeWorkingPath = workingPathAPP;
    }
   
    returnValue = macUserDocumentsPath();
    if (returnValue != NULL)
    {
        //  /Users -> "currentuser" -> Documents -> WWIIOnline
        wwiiolpath_string userDocumentsPath(&returnValue[0], &returnValue[strlen(returnValue)]);
        userConfigPath = userDocumentsPath;
        userConfigPathSet = TRUE;
    }
    else
    {
        fioStructureInit_Value = FALSE;
    }

    returnValue = macUserWorkingPath();
    if (returnValue != NULL)
    {
        //  /Users -> "currentuser" -> Documents -> WWIIOnline -> bin
        wwiiolpath_string workingPathUSER(&returnValue[0], &returnValue[strlen(returnValue)]);
        userWorkingPath = workingPathUSER;
        userWorkingPathSet = TRUE;
    }
    else
    {
        fioStructureInit_Value = FALSE;
    }
    
    WW2_exePath = exePath;
    //	WW2_exeBinPath = exeBinPath;
    WW2_exeDataPath = exeDataPath;
    WW2_exePlayarcPath = exePlayarcPath;
    WW2_exeWorkingPath = exeWorkingPath;
    WW2_userConfigPath = userConfigPath;
    WW2_userWorkingPath = userWorkingPath;
#endif

	return fioStructureInit_Value;
}


LOGICAL fioStructureVerify()
{
	LOGICAL fioStructureVerify_Value = FALSE;

	return fioStructureVerify_Value;
}


unsigned long Wstring2Char(wwiiolpath_string sourceString, char *destinationString, const size_t sizeLimit)
{
	unsigned long retValue = 0;

	if (sizeLimit < 1)
		return 0;
    
    if (sourceString.length() == 0)
        return 0;

	retValue = wcstombs(destinationString, sourceString.c_str(), (sizeLimit - 1));
	
	if (retValue == (size_t)-1)
        return 0;

	if (retValue == (sizeLimit - 1))
	{
		destinationString[sizeLimit] = '\0';
	}
	else if (retValue == sourceString.size())
	{
		destinationString[sourceString.size()] = '\0';
        //  wcstombs return does not include the terminating NULL ( L"\0" ).
        retValue = retValue + 1;
	}

	return retValue;
}


unsigned long Char2Wstring(char *sourceString, wwiiolpath_string &destinationString, const size_t sizeLimit)
{
	unsigned long retValue = strlen(sourceString);
    unsigned long copyCount = 0;

	if (sizeLimit < 1 || retValue < 1)
		return 0;

    if(retValue > sizeLimit)
    {
        copyCount = sizeLimit;
    }
    else
    {
        copyCount = retValue;
    }

    //  wwiiolpath_string tmp(&sourceString[0], &sourceString[strlen(sourceString)]);
    wwiiolpath_string tmp(&sourceString[0], &sourceString[copyCount]);
    destinationString = tmp;
    
    
    //  destinationString.append(*sourceString, copyCount);
    retValue = destinationString.length();

	return retValue;
}


void GetWC(const char *sourceString, wwiiolpath_string &destinationString)
{
	const size_t cSize = strlen(sourceString) + 1;
	std::wstring wc(cSize, L'#');
	mbstowcs(&wc[0], sourceString, cSize);
	destinationString = wc.data();
	return;
}


bool GetExePath(wwiiolpath_string &outFullPath, wwiiolpath_string &outExeName)
{
#if platOS == OS_WIN32
    wchar_t wBuffer[MAX_PATH + 1];
	std::wstring returnFullPath = L"";

	// Retrieve the full path for the current module. NULL = calling file (exe).
	wBuffer[0] = 0;
	if (GetModuleFileName(NULL, wBuffer, sizeof wBuffer) == 0)
		return false;

	returnFullPath = wBuffer;

	std::wstring::size_type found;
	found = returnFullPath.find_last_of(L"/\\");
	outFullPath = returnFullPath.substr(0, found);
	outExeName = returnFullPath.substr(found + 1);

	return true;
#else

    //  const char *returnValue;
    
    //  WW2_exePath = L"";
    //  returnValue = macAppPath();
    //  wwiiolpath_string exePath(returnValue[0], returnValue[strlen(returnValue)]);
    //  WW2_exePath = exePath;
    
    outFullPath = L"";
    outExeName = L"";
    
    return false;
#endif
}


wwiiolpath_string fioGetCurrentWorkingPath()
{
    char buffer[MAX_PATH];
    char *answer = getcwd(buffer, sizeof(buffer));
    wwiiolpath_string retVal;
    Char2Wstring(answer, retVal, strlen(answer));
    
    return retVal;
}


LOGICAL fioSetCurrentWorkingPath(wwiiolpath_string targetPath)
{
    LOGICAL retVal = FALSE;
    char buffer[MAX_PATH];
    
    if(targetPath == L"")
        return FALSE;
    
    Wstring2Char(targetPath, buffer, MAX_PATH);
    int result = chdir(buffer);
    
    if (result == 0)
        retVal = TRUE;
    
    return retVal;
}


void fioStructureForwadDeclare()
{

	return;

// Forwad Declare information store
/*
extern wwiiolpath_string WantedDataPath;		//  SET by the command line - ENABLED by WantDataDir = True < OR > is a Nul String L"".
extern wwiiolpath_string WWIIOnlineDirName;		//  The ROOT folder for the game install - .\WWIIOnLine

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

extern LOGICAL WW2_exePathSet;
//  extern LOGICAL WW2_exeBinPathSet;
extern LOGICAL WW2_exeDataPathSet;
extern LOGICAL WW2_exeDocsPathSet;
extern LOGICAL WW2_exePlayarcPathSet;
extern LOGICAL WW2_exeWorkingPathSet;
extern LOGICAL WW2_userConfigPathSet;
extern LOGICAL WW2_userWorkingPathSet;
*/
}
