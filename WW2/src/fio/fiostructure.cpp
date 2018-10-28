
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
	#include <shellapi.h>
	#include <shlobj.h>
	#define errno (*_errno())
	#include <direct.h>  //  for _getcwd.
#endif

LOGICAL				WantedCopyOldSetup = TRUE;			//  DEF = TRUE -- Look for and copy the old BGE setup files (IF FOUND) to the New WWIIOnLine folder.
wwiiolpath_string	WantedDataPath = L"";				//  DEF = L"" -- SET by the command line - ENABLED by WantDataDir = True < OR > is a Nul String L"".
wwiiolpath_string	WWIIOnlineDirName = L"WWIIOnLine";	//  DEF = L"WWIIOnLine" -- The ROOT folder for the game install - .\WWIIOnLine

wwiiolpath_string	WW2_exePath;				//	Windows - EXE ROOT folder:  \Program * Files -> CRS -> WWIIOnLine == *.exe, WWIIOnLine.ico, readme.*, unins*.* (uninstaller), *.tkt, *.script, bin(dir), DATA(dir), docs(dir), playarc(dir).
												//	macOS - EXE ROOT folder:  /Applications -> World War II Online.app == Contents -> _CodeSignature(dir), Frameworks(dir), MacOS(dir -> exefile), Resources(dir) info.plist, PkgInfo.
wwiiolpath_string	WW2_exeDataPath;			//	EXE Support Data folder:  \Program * Files -> CRS  -> WWIIOnLine -> DATA == DirectXRedist(dir), loc(dir), playgate(dir), RuntimeLibraries(dir). 
wwiiolpath_string	WW2_exeDocsPath;			//	Windows - EXE Docs Folder:  \Program * Files -> CRS  -> WWIIOnLine -> docs == eula.txt, license.*, regions.txt, tos.txt.
												//	macOS - EXE Docs Folder:  /Applications -> World War II Online.app -> Contents -> Resources -> docs == eula.txt, license.*, regions.txt, tos.txt.
wwiiolpath_string	WW2_exePlayarcPath;			//	Windoes - EXE playarc folder:  \Program * Files -> CRS  -> WWIIOnLine -> playarc == playarc??.ww2  --OR--  == "-data" command line switch.
												//	macOS - EXE playarc folder:  /Applications -> World War II Online.app -> Contents -> Resources == playarc.ww2.
wwiiolpath_string	WW2_exeWorkingPath;			//	Windows - EXE Working folder:  \ProgramData -> WWIIOnLine == ftplog.txt, downloaded patches, WWIIOL.log, *.log.
												//	macOS - EXE Working folder:  /Library -> Application Support -> WWIIOnline == ftplog.txt, downloaded patches, WWIIOL.log, *.log.
wwiiolpath_string	WW2_userDocumentsPath;		//	Windows - USER Config ROOT folder:  \User -> My Documents -> WWIIOnLine == wwiiol.xml, cfml(dir).
												//	macOS - USER Config ROOT folder:  /Users -> "currentuser" -> Documents -> WWIIOnline == wwiiol.xml, cfml(dir).
wwiiolpath_string	WW2_userScreenShotsPath;	//	Windows - USER Config ROOT folder:  \User -> My Documents -> WWIIOnLine -> screenshots == SShot*.bmp.
												//	macOS - USER Config ROOT folder:  /Users -> "currentuser" -> Documents -> WWIIOnline -> screenshots == SShot*.bmp.
wwiiolpath_string	WW2_userWorkingPath;		//	Windows - USER Working folder:  \User -> My Documents -> WWIIOnLine -> bin == lg.dat.
												//	macOS - USER Working folder:  /Users -> "currentuser" -> Documents -> WWIIOnline -> bin == lg.dat.


/************************************************************************************************
fioStructureInit - sets up the data directory structure on clients
*************************************************************************************************/

LOGICAL fioStructureInit()
{
    LOGICAL fioStructureInit_Value = TRUE;

	wwiiolpath_string exePath = L"";
	//  wwiiolpath_string exeBinPath = L"";
	wwiiolpath_string exeDataPath = L"";
	wwiiolpath_string exeDocsPath = L"";
	wwiiolpath_string exePlayarcPath = L"";
	wwiiolpath_string exeWorkingPath = L"";
	wwiiolpath_string userDocumentsPath = L"";
	wwiiolpath_string userScreenShotsPath = L"";
	wwiiolpath_string userWorkingPath = L"";
	
	LOGICAL exePathSet = FALSE;
    //	LOGICAL exeBinPathSet = FALSE;
    LOGICAL exeDataPathSet = FALSE;
    LOGICAL exeDocsPathSet = FALSE;
    LOGICAL exePlayarcPathSet = FALSE;
    LOGICAL exeWorkingPathSet = FALSE;
    LOGICAL userDocumentsPathSet = FALSE;
	LOGICAL userScreenShotsPathSet = FALSE;
    LOGICAL userWorkingPathSet = FALSE;
    
    const wwiiolpath_string WW2filename = L"\\WW2.exe";			// \\WW2.exe
    const wwiiolpath_string DirNameBIN = L"\\bin";				//  -> bin
    const wwiiolpath_string DirNameDATA = L"\\DATA";			//  -> DATA
    const wwiiolpath_string DirNameDOCS = L"\\docs";			//  -> docs
	const wwiiolpath_string DirNameSSHOTS = L"\\screenshots";	//  -> screenshots
    const wwiiolpath_string DirNamePLAYARC = L"\\playarc";		//  -> playarc

#if platOS == OS_WIN32
	//  Returns true if NO errors were encounered.

	wchar_t wBuffer[MAX_PATH + 1];
	int returnResult = -10;
	wwiiolpath_string exeFullPath = L"";
	wwiiolpath_string exeName = L"";
	bool  WW2exeExists = false;
	struct _stat buf;
	std::wstring filename = L"";



	//  exePath: Setup - START.
	//	??_exePath - EXE ROOT folder:  \Program * Files -> CRS -> WWIIOnLine == *.exe, WWIIOnLine.ico, readme.*, unins*.* (uninstaller), *.tkt, *.script, bin(dir), DATA(dir), docs(dir), playarc(dir).
	//  This folder should be setup by the installer.
	//  No Fallback - ABORT.
	
	//  wBuffer[0] = 0;
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
		
		//  NO FALLBACK.
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
		
		//  NO FALLBACK.
		goto ERR_EXIT_POINT;
	}
	else
	{
		returnResult = -10;
		WW2exeExists = true;
	}
	//  exePath: Setup - END.



	//  userDocumentsPath - START.
	//  WW2_userDocumentsPath - \User -> My Documents -> WWIIOnLine == wwiiol.xml, lg.dat, cfml(dir).
	//  This folder should be setup by the installer.
	//  No Fallback - ABORT.

	//  Ask Windows where the users documents folder is; oh, and create it if it doesn't exist
	//  because we'll be needing it.
	wBuffer[0] = 0;
	if (SHGetFolderPath(nullptr, CSIDL_PERSONAL | CSIDL_FLAG_CREATE, nullptr, SHGFP_TYPE_CURRENT, wBuffer) == S_OK)
	{
		userDocumentsPath = wBuffer;
		userDocumentsPath += L"\\" + WWIIOnlineDirName;

		//  make sure it exists
		//  Returns:
		//  0 = Directory created.
		//  -1 = Directory not created error state. [errno] set to the error code.
		returnResult = -10;
		_set_errno(0);
		returnResult = _wmkdir(userDocumentsPath.c_str());

		if (returnResult == -1)
		{
			if (errno == EEXIST)
			{
				//  File/Directory Already Exists.
				userDocumentsPathSet = TRUE;
			}
			else
			{
				//  errno = ENOENT - Path was not found.
				userDocumentsPath = L"";
				userDocumentsPathSet = FALSE;

#if EXE_BUILD != BUILD_EXE_SETTINGS
				dbugLog(_FLW, "SHGetFolderPath(CSIDL_PERSONAL) failed.  This folder should be setup by the installer.");
#endif
				fioStructureInit_Value = FALSE;
			}
		}
		else
		{
			userDocumentsPathSet = TRUE;
		}
	}
	else
	{
		userDocumentsPath = L"";
		userDocumentsPathSet = FALSE;

#if EXE_BUILD != BUILD_EXE_SETTINGS
		dbugLog(_FLA, "SHGetFolderPath(CSIDL_PERSONAL) failed: &HFFFE.  This should NOT happen.");
#endif
		fioStructureInit_Value = FALSE;

		//  NO FALLBACK.
		goto ERR_EXIT_POINT;
	}
	//  userDocumentsPath - END.



	//  userWorkingPath: Setup - START.
	//  This folder should be setup by the installer.
	//  Required by - exeWorkingPath fallback.
	//  Fallback to exePath == \Program * Files -> CRS -> WWIIOnLine

	//	??_userWorkingPath - USER Working folder:  \User -> My Documents -> WWIIOnLine -> bin == lg.dat.
	userWorkingPath = userDocumentsPath + DirNameBIN;

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

			//  Fallback to exePath == \Program * Files -> CRS -> WWIIOnLine
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



	//  exeWorkingPath - START.
	//	??_exeWorkingPath - EXE Working folder:  \ProgramData -> WWIIOnLine == ftplog.txt, downloaded patches, WWIIOL.log, *.log.
	//  This folder should be setup by the installer.
	//  Fallback-1 to userDocumentsPath - \User -> My Documents -> WWIIOnLine -> bin.
	//  Fallback-2 to exePath == \Program * Files -> CRS -> WWIIOnLine.

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
				exeWorkingPathSet = TRUE;
			}
			else if (errno == ENOENT)
			{
				//  Path was not found, fallingback.
				//  Unable to create ProgramData directory. If the Installer (or someone else) has already created, all is ok.
				exeWorkingPathSet = FALSE;

				//  Fallback-1 to userDocumentsPath - \User -> My Documents -> WWIIOnLine -> bin.
				if (userDocumentsPathSet == TRUE)
				{
					exeWorkingPath = userDocumentsPath + DirNameBIN;

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
							//  Fallback-2 to exePath == \Program * Files -> CRS -> WWIIOnLine.
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



	/* ==========================
	//  exeBinPath - START.
	//	??_exeBinPath - EXE DLL folder:  \Program * Files -> CRS -> WWIIOnLine -> bin == *.lib, steam_appid.txt.
	//  This folder should be setup by the installer.
	//  Fallback to exePath == \Program * Files -> CRS -> WWIIOnLine

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
	========================== */



	//  exeDataPath: Setup - START.
	//  This folder should be setup by the installer.
	//  Fallback to exePath == \Program * Files -> CRS -> WWIIOnLine

	//	??_exeDataPath - EXE Support Data folder:  \Program * Files -> CRS -> WWIIOnLine -> DATA == DirectXRedist(dir), loc(dir), playgate(dir), RuntimeLibraries(dir).
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
	//  Fallback to exePath == \Program * Files -> CRS -> WWIIOnLine

	//	??_exeDocsPath - EXE Docs Folder:  \Program * Files -> CRS -> WWIIOnLine -> doc == eula.txt, license.*, regions.txt, tos.txt.
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

	//	??_exePlayarcPath - EXE playarc folder:  \Program * Files -> CRS -> WWIIOnLine -> playarc == playarc??.ww2  -- OR --  == "-data" command line switch.
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



	//  ScreenShots: Setup - START.
	//  This folder should be setup by the installer.

	//	Screen Shots Folder: \User -> My Documents -> WWIIOnLine -> screenshots.
	userScreenShotsPath = userDocumentsPath + DirNameSSHOTS;

	//  make sure it exists
	//  Returns:
	//  0 = Directory created.
	//  -1 = Directory not created error state. [errno] set to the error code.
	returnResult = -10;
	_set_errno(0);
	returnResult = _wmkdir(userScreenShotsPath.c_str());

	if (returnResult == -1)
	{
		if (errno == EEXIST)
		{
			//  File/Directory Already Exists.
			userScreenShotsPathSet = TRUE;
		}
		else
		{
			//  errno = ENOENT - Path was not found.
			userScreenShotsPath = userDocumentsPath;
			userScreenShotsPathSet = FALSE;

#if EXE_BUILD != BUILD_EXE_SETTINGS
			dbugLog(_FLI, "Could NOT create the User Screen Shots folder.");
#endif
		}
	}
	else
	{
		userScreenShotsPathSet = TRUE;
	}
	//  ScreenShots: Setup - END.



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
		WW2_userDocumentsPath = userDocumentsPath;
		WW2_userScreenShotsPath = userScreenShotsPath;
		WW2_userWorkingPath = userWorkingPath;
	}

#elif platOS == OS_MACOS
    
    const char *returnValue;
    fioStructureInit_Value = TRUE;
    
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
    
    returnValue = macDocsPath();
    if (returnValue != NULL)
    {
        //  /Applications -> World War II Online.app -> Contents -> Resources -> docs
        wwiiolpath_string docsPath(&returnValue[0], &returnValue[strlen(returnValue)]);
        exeDocsPath = docsPath;
        exeDocsPathSet = TRUE;
    }
    else
    {
        fioStructureInit_Value = FALSE;
    }

    returnValue = macPlayarcPath();
    if (returnValue != NULL)
    {
        //  /Applications -> World War II Online.app -> Contents -> Resources -> playarc.ww2 ( NOTE = On the MAC it is a single file. )
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
        //  This folder  MUST  be setup by another porcess as the OS prevents axcess.
        //  The most likley process is the Mac DropDMG installer.

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
        wwiiolpath_string macUserDocumentsPath(&returnValue[0], &returnValue[strlen(returnValue)]);
        userDocumentsPath = macUserDocumentsPath;
        userDocumentsPathSet = TRUE;
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

	returnValue = macUserScreenShotsPath();
	if (returnValue != NULL)
	{
		//  /Users -> "currentuser" -> Documents -> WWIIOnline -> screenshots
		//  Create Folder ONLY.
		wwiiolpath_string workingScreenShotsUSER(&returnValue[0], &returnValue[strlen(returnValue)]);
		userScreenShotsPath = workingScreenShotsUSER;
		userScreenshotsPathSet = TRUE;
	}
	else
	{
		fioStructureInit_Value = FALSE;
	}

    WW2_exePath = exePath;
    //	WW2_exeBinPath = exeBinPath;
    WW2_exeDataPath = exeDataPath;
    WW2_exeDocsPath = exeDocsPath;
    WW2_exePlayarcPath = exePlayarcPath;
    WW2_exeWorkingPath = exeWorkingPath;
    WW2_userDocumentsPath = userDocumentsPath;
	WW2_userScreenShotsPath = userScreenShotsPath;
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


unsigned long Wstring2Char(wwiiolpath_string sourceString, wchar_t *destinationString, const size_t sizeLimit)
{
	unsigned long retValue = 0;

	if (sizeLimit < 1)
		return 0;

	if (sourceString.length() == 0)
		return 0;

	wcscpy(destinationString, sourceString.c_str());
	retValue = sourceString.length();

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

	wwiiolpath_string tmp(&sourceString[0], &sourceString[copyCount]);
    destinationString = tmp;

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
    
#if platOS == OS_WIN32
	char *answer = _getcwd(buffer, sizeof(buffer));
#else
    char *answer = getcwd(buffer, sizeof(buffer));
#endif
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
    
#if platOS == OS_WIN32
    int result = _chdir(buffer);
#else
    int result = chdir(buffer);
#endif
    
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
extern wwiiolpath_string WW2_userDocumentsPath;	//	??_userDocumentsPath - Windows - USER Config ROOT folder:  \User -> My Documents -> WWIIOnLine == wwiiol.xml, cfml(dir).
												//	??_userDocumentsPath - macOS - USER Config ROOT folder:  /Users/"currentuser"/Documents/WWIIOnline == wwiiol.xml, cfml(dir).
extern wwiiolpath_string WW2_userWorkingPath;	//	??_userWorkingPath - USER Working folder:  \User -> My Documents -> WWIIOnLine -> bin == lg.dat.

extern LOGICAL WW2_exePathSet;
//  extern LOGICAL WW2_exeBinPathSet;
extern LOGICAL WW2_exeDataPathSet;
extern LOGICAL WW2_exeDocsPathSet;
extern LOGICAL WW2_exePlayarcPathSet;
extern LOGICAL WW2_exeWorkingPathSet;
extern LOGICAL WW2_userDocumentsPathSet;
extern LOGICAL WW2_userWorkingPathSet;
*/
}


wwiiolpath_string SQL_DateTimeStamp() {

	//  Create a SQL formatted date/time stamp.
	wwiiolpath_string returnValue = L"";
	int n = 0;
	time_t rawtime;
	struct tm * timeinfo;
	wchar_t timeStamp[256] = L"";

	time(&rawtime);
	timeinfo = localtime(&rawtime);
	n = swprintf(timeStamp, 256, L"%.4d%.2d%.2d-%.2d%.2d%.2d", timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

	returnValue = timeStamp;
	timeStamp[0] = 0;

	return returnValue;
}


bool CopyOldSetupFiles_WIN()
{	
	bool copyFiles = false;

	if (WantedCopyOldSetup == FALSE) { return false; }

#if platOS == OS_WIN32
	int charCount = 0;
	int YN = 0;

	wchar_t wBuffer[MAX_PATH + 1];
	wwiiolpath_string userDocumentsPath = L""; //  \User -> My Documents
	wwiiolpath_string BGEFolder = L"Battleground Europe";  //  L"Battleground Europe"
	wwiiolpath_string bgePath = L"";
	wwiiolpath_string ww2Path = WW2_userDocumentsPath;  //  \User -> My Documents -> WWIIOnLine
	wwiiolpath_string folderRename = L"";
	wwiiolpath_string reNamePath = L"";
	wwiiolpath_string title1 = L"World War II Online - Old Game settings Located.";
	wwiiolpath_string title2 = L"World War II Online - Old Game settings Saved.";
	wwiiolpath_string message1 = L"An older version of the Game Settings has been found.\n\nDo you wish to re-use them?";
	wwiiolpath_string message2 = L"";  //  To be set later.

#if EXE_BUILD != BUILD_EXE_SETTINGS
	dbugLog(_FLI, "Old Settings - Checking for the presence of Old Game Settings.");
#endif


	//  Create a folder copy date/time stamp.  =================================================
	folderRename = L"_Copied_" + SQL_DateTimeStamp();


	//  Ask Windows where the users documents folder is?  ======================================
	wBuffer[0] = 0;
	if (SHGetFolderPath(nullptr, CSIDL_PERSONAL | CSIDL_FLAG_CREATE, nullptr, SHGFP_TYPE_CURRENT, wBuffer) == S_OK)
	{
		userDocumentsPath = wBuffer;  //  \User -> My Documents
		bgePath = userDocumentsPath + L"\\" + BGEFolder;  //  \User -> My Documents -> Battleground Europe
		wBuffer[0] = 0;
	}
	else
	{
		wwiiolpath_string tmp = WWIIOnlineDirName + L" - File System.";
		MessageBox(nullptr, L"Unable to locate the My Documents directory.\n - This should not happen.", tmp.c_str(), MB_ICONSTOP | MB_OK);

#if EXE_BUILD != BUILD_EXE_SETTINGS
		dbugLog(_FLI, "Old Settings - Unable to locate the My Documents directory. This should not happen.");
#endif

		return false;
	}


	//  Locate: \User -> My Documents -> Battleground Europe.  =================================
	if ((GetFileAttributes(bgePath.c_str())) == INVALID_FILE_ATTRIBUTES)
	{
#if EXE_BUILD != BUILD_EXE_SETTINGS
		dbugLog(_FLI, "Old Settings - Old Game Settings NOT found.");
#endif
		return false;
	}

	//  Located: \User -> My Documents -> Battleground Europe.  ================================
#if EXE_BUILD != BUILD_EXE_SETTINGS
	dbugLog(_FLI, "Old Settings - Old Game Settings found.");
#endif


	//  Verify the User wants to copy the old Setup Files.  ====================================
	YN  = MessageBox(nullptr, message1.c_str(), title1.c_str(), MB_ICONASTERISK | MB_YESNO);
	
	if (YN == IDNO)
	{
		// the User clicked: No.
#if EXE_BUILD != BUILD_EXE_SETTINGS
		dbugLog(_FLW, "Old Settings - Old Game Settings found -> the user declined the copy option.");
#endif
		copyFiles = false;
	
	}
	else
	{
		//  Copy the ENTIRE Contents of:  ======================================================
		//   \User -> My Documents -> Battleground Europe
		//   TO
		//   \User -> My Documents -> WWIIOnline.

#if EXE_BUILD != BUILD_EXE_SETTINGS
		dbugLog(_FLI, "Old Settings - Old Game Settings found -> copying files.");
#endif

		wwiiolpath_string tmpSource = bgePath + L"\\*.*";
		wwiiolpath_string tmpdestination = WW2_userDocumentsPath + L"\\";
		if (SHFileFunctions_WC(tmpSource, tmpdestination, FO_COPY))
		{
			//  Copy Completed.


			//  Re-Name:  ======================================================================
			//   \User -> My Documents -> "Battleground Europe"
			//   TO
			//   \User -> My Documents -> "Battleground Europe_Copied_XXXXXXXXXX-XXXXXX".

			reNamePath = bgePath + folderRename;
			message2 = L"The Directory : \n" + bgePath + L"\n\nhas been re-named to : \n" + reNamePath;

#if EXE_BUILD != BUILD_EXE_SETTINGS
			dbugLog(_FLI, "Old Settings - Old Game Settings found -> re-naming the Original Battleground Europe Directory.");
#endif

			if (SHFileFunctions_WC(bgePath, reNamePath, FO_RENAME))
			{
				//  Rename Succeeded.
#if EXE_BUILD != BUILD_EXE_SETTINGS
				dbugLog(_FLI, "Old Settings - Old Game Settings found -> Old Game Settings file copy and re-naming completed.");
#endif
				YN = MessageBox(nullptr, message2.c_str(), title2.c_str(), MB_ICONASTERISK | MB_OK);
				copyFiles = true;
			}
			else
			{
				//  Rename Failed.
#if EXE_BUILD != BUILD_EXE_SETTINGS
				char source[MAX_PATH] = "";
				char destination[MAX_PATH] = "";
				Wstring2Char(bgePath, source, MAX_PATH);
				Wstring2Char(reNamePath, destination, MAX_PATH);

				dbugLog(_FLW, "Old Settings - Could not rename:-\n\nfrom path : \n%s,\n\nto path : \n%s.", &source, &destination);
#endif
				copyFiles = false;
			}
		}
		else
		{
			//  Copy Failed.
#if EXE_BUILD != BUILD_EXE_SETTINGS
			char source[MAX_PATH] = "";
			char destination[MAX_PATH] = "";
			Wstring2Char(bgePath, source, MAX_PATH);
			Wstring2Char(WW2_userDocumentsPath, destination, MAX_PATH);

			dbugLog(_FLW, "Old Settings - Could not copy:-\n\nfrom path : \n%s,\n\nto path : \n%s.", &source, &destination);
#endif
			copyFiles = false;
		}
	}

#else
	copyFiles = false;

#endif

	return copyFiles;
}


bool SHFileFunctions_WC(wwiiolpath_string sourcePath, wwiiolpath_string destinationPath, unsigned int FO_Function = 2U)
{
	bool retValue = false;

#if platOS == OS_WIN32

#if EXE_BUILD != BUILD_EXE_SETTINGS
	char source[MAX_PATH] = "";
	char destination[MAX_PATH] = "";
	Wstring2Char(sourcePath, source, MAX_PATH);
	Wstring2Char(destinationPath, destination, MAX_PATH);

	dbugLog(_FLI, "Old Settings - File operations: %d from %s to %s", FO_Function, &source, &destination);
#endif

	//  FO_Function defines.
	//  FO_MOVE		0x0001 - Move files from sourcePath, to destinationPath.
	//  FO_COPY		0x0002 - Copy files from sourcePath, to destinationPath.
	//  FO_DELETE	0x0003 - Delete files from sourcePath.
	//  FO_RENAME	0x0004 - ReName a file from sourcePath. Single file ONLY.

	//  ALL Paths are Fully Qualified Paths.

	//  FO_Function Out of Range.
	if (FO_Function < 1 || FO_Function > 4) { return false; }

	//  Disable Deletion.
	if (FO_Function == FO_DELETE) { return false; }

	//  Source Pathe required for Delete and Rename.
	if (sourcePath == L"" && (FO_Function == FO_DELETE || FO_Function == FO_RENAME)) { return false; }

	//  No Source and Destination.
	if (sourcePath == L"" && destinationPath == L"" ) { return false; }

	int result = -1;
	SHFILEOPSTRUCT fileOp = { 0 };
	fileOp.wFunc = FO_Function;

	TCHAR newSourcePath[MAX_PATH];
	wcscpy_s(newSourcePath, sourcePath.c_str());
	newSourcePath[wcslen(sourcePath.c_str()) + 1] = NULL;
	fileOp.pFrom = newSourcePath;

	TCHAR newDestinationPath[MAX_PATH];
	wcscpy_s(newDestinationPath, destinationPath.c_str());
	newDestinationPath[wcslen(destinationPath.c_str()) + 1] = NULL;
	fileOp.pTo = newDestinationPath;

	fileOp.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_NOCONFIRMMKDIR;

	result = SHFileOperationW(&fileOp);

	if (result == 0 && fileOp.fAnyOperationsAborted == false)
	{
		retValue = true;
	}
	else
	{
#if EXE_BUILD != BUILD_EXE_SETTINGS
		dbugLog(_FLI, "Old Settings - SHFileOperation return code: 0x%x", result);
#endif
		retValue = false;
	}

#else
	retValue = false;

#endif

	return retValue;
}
