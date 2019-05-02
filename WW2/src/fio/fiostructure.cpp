
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
#include <chrono>
#include "fiostructure.h"
#include "platforms.h"

#if platOS == OS_WIN32
	#include <shellapi.h>
	#include <shlobj.h>
	#define errno (*_errno())
	#include <direct.h>  //  for _getcwd.
#endif

LOGICAL				WantedCopyOldSetup = TRUE;			//  DEF = TRUE -- Look for and copy the old BGE setup files (IF FOUND) to the New WWIIOnLine folder.
wwiiolpath_string	WantedDataPath = L"";				//  DEF = L"" -- SET by the command line - ENABLED by WantDataDir = True < OR > is a Nul String L"".
wwiiolpath_string	WWIIOnlineDirName = L"WWIIOnLine";	//  DEF = L"WWIIOnLine" -- The ROOT folder for the game install - .\WWIIOnLine

wwiiolpath_string	WW2_exePath;				//	Windows - EXE INSTALL ROOT folder:  \Program * Files -> CRS -> WWIIOnLine == *.exe, WWIIOnLine.ico, readme.*, unins*.* (uninstaller), *.tkt, *.script, bin(dir), DATA(dir), docs(dir), playarc(dir).
												//	macOS - EXE INSTALL ROOT folder:  /Applications -> World War II Online.app == Contents -> _CodeSignature(dir), Frameworks(dir), MacOS(dir -> exefile), Resources(dir) info.plist, PkgInfo.
wwiiolpath_string	WW2_exeDataPath;			//	Windows - EXE INSTALL ROOT Support Data folder:  \Program * Files -> CRS  -> WWIIOnLine -> data == DirectXRedist(dir), loc(dir), playgate(dir), RuntimeLibraries(dir).
                                                //	macOS - EXE INSTALL ROOT Support Data folder:  /Applications -> World War II Online.app -> Contents -> Resources.
wwiiolpath_string	WW2_exeDocsPath;			//	Windows - EXE INSTALL ROOT Docs Folder:  \Program * Files -> CRS  -> WWIIOnLine -> docs == eula.txt, license.*, regions.txt, tos.txt.
												//	macOS - EXE INSTALL ROOT Docs Folder:  /Applications -> World War II Online.app -> Contents -> Resources -> docs == eula.txt, license.*, regions.txt, tos.txt.
wwiiolpath_string	WW2_exePlayarcPath;			//	Windows - EXE INSTALL ROOT playarc folder:  \Program * Files -> CRS  -> WWIIOnLine -> playarc == playarc??.ww2  --OR--  == "-data" command line switch.
												//	macOS - EXE INSTALL ROOT playarc folder:  /Applications -> World War II Online.app -> Contents -> Resources == playarc.ww2.
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
	wwiiolpath_string exeDataPath = L"";
	wwiiolpath_string exeDocsPath = L"";
	wwiiolpath_string exePlayarcPath = L"";
	wwiiolpath_string exeWorkingPath = L"";
	wwiiolpath_string userDocumentsPath = L"";
	wwiiolpath_string userScreenShotsPath = L"";
	wwiiolpath_string userWorkingPath = L"";
	
	LOGICAL exePathSet = FALSE;
    LOGICAL exeDataPathSet = FALSE;
    LOGICAL exeDocsPathSet = FALSE;
    LOGICAL exePlayarcPathSet = FALSE;
    LOGICAL exeWorkingPathSet = FALSE;
    LOGICAL userDocumentsPathSet = FALSE;
	LOGICAL userScreenShotsPathSet = FALSE;
    LOGICAL userWorkingPathSet = FALSE;
    
    const wwiiolpath_string WW2filename = L"\\WW2.exe";			// \\WW2.exe
	const wwiiolpath_string WW2filenameMC = L"\\MultiClient.exe";			// \\MultiClient.exe
    const wwiiolpath_string DirNameBIN = L"\\bin";				//  -> bin
    const wwiiolpath_string DirNameDATA = L"\\data";			//  -> data
    const wwiiolpath_string DirNameDOCS = L"\\docs";			//  -> docs
	const wwiiolpath_string DirNameSSHOTS = L"\\screenshots";	//  -> screenshots
    const wwiiolpath_string DirNamePLAYARC = L"\\playarc";		//  -> playarc

#if platOS == OS_WIN32
	//  Returns true if NO errors were encounered.

	wchar_t wBuffer[MAX_PATH + 1];
	int returnResult = -10;
	wwiiolpath_string exeFullPath = L"";
	wwiiolpath_string exeName = L"";
	bool  WW2exe_Exists = false;
	bool  WW2exeMC_Exists = false;
	struct _stat buf;
	std::wstring filename = L"";



	//  exePath: Setup - START.
	//	exePath - Windows - EXE INSTALL ROOT folder:  \Program * Files -> CRS -> WWIIOnLine.
	//  This directory should be setup and populated by the installer.

    //  No Fallback - ABORT.
	
	if (GetExePath(exeFullPath, exeName) == true)
	{
		exePath = exeFullPath;
		exePathSet= TRUE;
	}
	else
	{
        //  Could not find the install folder based on the exe files actual location.
        //  This should NOT happen.
        
#if EXE_BUILD != BUILD_EXE_SETTINGS
        dbugLog(_FLA, "&HFFFF - Could not find the install folder based on the exe files actual location.  Aborting.  This should NOT happen.");
#endif
        
        exePath = L"";
		exePathSet = FALSE;
		fioStructureInit_Value = FALSE;
		
		//  NO FALLBACK
		goto ERR_EXIT_POINT;
	}

	//  Locate WW2.exe.
	filename = exePath + WW2filename;
	returnResult = -10;
	returnResult = _wstat(filename.c_str(), &buf);

	if (returnResult != 0)
	{
		switch (errno)
		{
		case ENOENT:
			//  printf("File %s not found.\n", filename);
			returnResult = -15;
            break;
		case EINVAL:
			//  printf("Invalid parameter to _stat.\n");
			returnResult = -20;
			break;
		default:
			//  Should never be reached.
			//  printf("Unexpected error in _stat.\n");
			returnResult = -25;
		}

        //  Could not locate WW2.exe file within the install folder at its detected location.
        //  This should NOT happen.

#if EXE_BUILD != BUILD_EXE_SETTINGS
        dbugLog(_FLA, "&HFFFE - Unable to locate WW2.exe - Aborting %d.", returnResult);
#endif
        WW2exe_Exists = false;
	}
	else
	{
		WW2exe_Exists = true;
	}

	//  Locate MultiClient.exe.
	filename = exePath + WW2filenameMC;
	returnResult = -10;
	returnResult = _wstat(filename.c_str(), &buf);

	if (returnResult != 0)
	{
		switch (errno)
		{
		case ENOENT:
			//  printf("File %s not found.\n", filename);
			returnResult = -15;
			break;
		case EINVAL:
			//  printf("Invalid parameter to _stat.\n");
			returnResult = -20;
			break;
		default:
			//  Should never be reached.
			//  printf("Unexpected error in _stat.\n");
			returnResult = -25;
		}

		//  Could not locate WW2.exe file within the install folder at its detected location.
		//  This should NOT happen.

#if EXE_BUILD != BUILD_EXE_SETTINGS
		dbugLog(_FLA, "&HFFFE - Unable to locate MultiClient.exe - Aborting %d.", returnResult);
#endif
		WW2exeMC_Exists = false;
	}
	else
	{
		WW2exeMC_Exists = true;
	}


	if (WW2exe_Exists == false && WW2exeMC_Exists == false)
	{
		wwiiolpath_string tmp = WWIIOnlineDirName + L" - File System."; 
		MessageBox(nullptr, L"Unable to locate WW2.exe. Aborting.", tmp.c_str(), MB_ICONSTOP | MB_OK);

		fioStructureInit_Value = FALSE;

		//  NO FALLBACK
		goto ERR_EXIT_POINT;
	}
	else
	{
		returnResult = -10;
		WW2exeMC_Exists = true;
	}
	//  exePath: Setup - END.



	//  userDocumentsPath - START.
    //  userDocumentsPath - Windows - USER Config ROOT folder:  \User -> My Documents -> WWIIOnLine.
    //  This directory is setup by the exe file at startup.
    
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
                //  Unable to create the WWIIOnline folder within the Users; My Documents folder.
                //  This should NOT happen.
                
#if EXE_BUILD != BUILD_EXE_SETTINGS
                dbugLog(_FLA, "&HFFFD - Unable to create the WWIIOnline folder within the Users; My Documents folder.  Aborting.  This should NOT Happen.");
#endif

                userDocumentsPath = L"";
				userDocumentsPathSet = FALSE;
				fioStructureInit_Value = FALSE;
                
                //  NO FALLBACK
                goto ERR_EXIT_POINT;
            }
		}
		else
		{
			userDocumentsPathSet = TRUE;
		}
	}
	else  //  System Call failed, NO Fallbacks.
	{
        //  The system could not provide the location of the current users; My Documrnts folder.
        //  This should NOT happen.

#if EXE_BUILD != BUILD_EXE_SETTINGS
        dbugLog(_FLA, "&HFFFC - The system could not provide the location of the current users; My Documrnts folder.  Aborting.  This should NOT Happen.");
#endif
        userDocumentsPath = L"";
		userDocumentsPathSet = FALSE;
        fioStructureInit_Value = FALSE;
        
        //  NO FALLBACK
        goto ERR_EXIT_POINT;
	}
	//  userDocumentsPath - END.



	//  userWorkingPath: Setup - START.
    //	userWorkingPath - Windows - USER Working folder:  \User -> My Documents -> WWIIOnLine -> bin
    //  This directory is setup by the exe file at startup.
	//  Required by - exeWorkingPath fallback.

    //  Fallback to userDocumentsPath == \User -> My Documents -> WWIIOnLine
    //  Fallback to No Fallback - ABORT.

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
            //  Fallback to userDocumentsPath == \User -> My Documents -> WWIIOnLine
            
            if (userDocumentsPathSet = TRUE)
            {
#if EXE_BUILD != BUILD_EXE_SETTINGS
                dbugLog(_FLI, "Setting User Working path to User Documents path.");
#endif
                userWorkingPath = userDocumentsPath;
                userWorkingPathSet = TRUE;
            }
            else
            {
                //  Setting User Working path fallbacks failed.  Aborting.
#if EXE_BUILD != BUILD_EXE_SETTINGS
                dbugLog(_FLA, "&HFFFB - Setting User Working path fallbacks failed.  Aborting.");
#endif
                userWorkingPath = L"";
                userWorkingPathSet = FALSE;
                fioStructureInit_Value = FALSE;
                
                //  NO FALLBACK
                goto ERR_EXIT_POINT;
            }
		}
	}
	else
	{
		userWorkingPathSet = TRUE;
	}
	//  userWorkingPath: Setup - END.



	//  exeWorkingPath - START.
	//	exeWorkingPath - Windows - EXE Working folder:  \ProgramData -> WWIIOnLine.
	//  This folder should be setup by the installer.

    //  Fallback-1 to userWorkingPath - \User -> My Documents -> WWIIOnLine -> bin.
	//  Fallback-2 to userDocumentsPath - \User -> My Documents -> WWIIOnLine.
    //  Fallback to No Fallback - ABORT.

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
				//  Unable to create WWIIOnLine directory within the ProgramData directory.
                //  This is a Protected System directory. The installer should have created this directory.
				exeWorkingPathSet = FALSE;
                
#if EXE_BUILD != BUILD_EXE_SETTINGS
                dbugLog(_FLI, "Unable to create WWIIOnLine directory within the; ProgramData directory.  This is a Protected System directory.");
#endif
                
                //  Fallback-1 to userWorkingPath - \User -> My Documents -> WWIIOnLine -> bin.
				if (userWorkingPathSet == TRUE)
				{
#if EXE_BUILD != BUILD_EXE_SETTINGS
                    dbugLog(_FLI, "Fallback-1 - Setting Exe Working path to User Working path.");
#endif
                    exeWorkingPath = userWorkingPath;
                    exeWorkingPathSet = TRUE;
                }
                else
                {
                    //  Fallback-2 to userDocumentsPath - \User -> My Documents -> WWIIOnLine.
                    if (userDocumentsPathSet == TRUE)
                    {
#if EXE_BUILD != BUILD_EXE_SETTINGS
                        dbugLog(_FLI, "Fallback-2 - Setting Exe Working path to User Documents path.");
#endif
                        exeWorkingPath = userDocumentsPath;
                        exeWorkingPathSet = TRUE;
                    }
                    else
                    {
#if EXE_BUILD != BUILD_EXE_SETTINGS
                        dbugLog(_FLA, "&HFFFA - Unable to create WWIIOnLine directory within the ProgramData directory. Fallbacks Failed.");
#endif
                        exeWorkingPath = L"";
                        exeWorkingPathSet = FALSE;
                        fioStructureInit_Value = FALSE;
                        
                        //  NO FALLBACK
                        goto ERR_EXIT_POINT;
                    }
                }
            }
        }
    }
	else  //  System Call failed, attempting Fallbacks.
    {
        //  Unable to create WWIIOnLine directory within the; ProgramData directory.
        //  This is a Protected System directory. The installer should have created this directory.
#if EXE_BUILD != BUILD_EXE_SETTINGS
        dbugLog(_FLW, "&HFFF9 - Unable to create WWIIOnLine directory within the; ProgramData directory.  This is a Protected System directory.");
#endif
        //  Fallback-1 to userWorkingPath - \User -> My Documents -> WWIIOnLine -> bin.
        if (userWorkingPathSet == TRUE)
        {
#if EXE_BUILD != BUILD_EXE_SETTINGS
            dbugLog(_FLI, "Fallback-1 - Setting Exe Working path to User Working path.");
#endif
            exeWorkingPath = userWorkingPath;
            exeWorkingPathSet = TRUE;
        }
        else
        {
            //  Fallback-2 to userDocumentsPath - \User -> My Documents -> WWIIOnLine.
            if (userDocumentsPathSet == TRUE)
            {
#if EXE_BUILD != BUILD_EXE_SETTINGS
                dbugLog(_FLI, "Fallback-2 - Setting Exe Working path to User Documents path.");
#endif
                exeWorkingPath = userDocumentsPath;
                exeWorkingPathSet = TRUE;
            }
            else
            {
#if EXE_BUILD != BUILD_EXE_SETTINGS
                dbugLog(_FLA, "&HFFF8 - Unable to create WWIIOnLine directory within the; ProgramData directory.  Fallbacks Failed.");
#endif
                exeWorkingPath = L"";
                exeWorkingPathSet = FALSE;
                fioStructureInit_Value = FALSE;
                
                //  NO FALLBACK
                goto ERR_EXIT_POINT;
            }
        }
    }
    //  exeWorkingPath - END.



	//  exeDataPath: Setup - START.
    //	exeDataPath - Windows - EXE INSTALL ROOT Support Data folder:  \Program * Files -> CRS  -> WWIIOnLine -> data.
    
    //  WW2.exe REQUIRES the files contained within this directory to operate.
    //  This directory is created and populated by the installer.

    //  NO Fallback.

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
            //  This should NOT happen.
            //  Unable to locate the \Program * Files -> CRS -> WWIIOnLine -> data directory.
#if EXE_BUILD != BUILD_EXE_SETTINGS
            dbugLog(_FLI, "&HFFF7 - Unable to locate the \\Program * Files -> CRS -> WWIIOnLine -> data directory.  Aborting.  This should NOT Happen.");
#endif
            exeDataPath = L"";
			exeDataPathSet = FALSE;
			fioStructureInit_Value = FALSE;
            
            //  NO FALLBACK
            goto ERR_EXIT_POINT;
        }
	}
	else
	{
		exeDataPathSet = TRUE;
	}
	//  exeDataPath: Setup - END.



	//  exeDocsPath: Setup - START.
    //	exeDocsPath - Windows - EXE INSTALL ROOT Docs Folder:  \Program * Files -> CRS  -> WWIIOnLine -> docs.
    //  This directory is created and populated by the installer.
	
    //  Fallback to userDocumentsPath - \User -> My Documents -> WWIIOnLine.

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
            if (userDocumentsPathSet == TRUE )
            {
#if EXE_BUILD != BUILD_EXE_SETTINGS
                dbugLog(_FLI, "Fallback - EXE Documents path set to User Documents path.");
#endif
                exeDocsPath = userDocumentsPath;
                exeDocsPathSet = TRUE;
            }
            else
            {
                //  This directory is created and populated by the installer.
#if EXE_BUILD != BUILD_EXE_SETTINGS
                dbugLog(_FLI, "EXE Documents path setup - Fallbacks Failed.");
#endif
                exeDocsPath = L"";
                exeDocsPathSet = FALSE;
                
                //  Will cause errors within Playgate Launcher
                //  . . . BUT . . .
                //  WW2.exe doesn't use the files contained within this directory.
                
                //  fioStructureInit_Value = FALSE;
            }
		}
	}
	else
	{
		exeDocsPathSet = TRUE;
	}
	//  exeDocsPath: Setup - END.



	//  exePlayarcPath: Setup - START.
    //	exePlayarcPath - Windows - EXE INSTALL ROOT playarc folder:  \Program * Files -> CRS  -> WWIIOnLine -> playarc.
    //  -- OR --
    //  == "-data" command line switch.
    
    //  WW2.exe REQUIRES the files contained within this directory to operate.
    //  This directory is created and populated by the installer.
    
    //  NO Fallback.
    
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
			//  This should NOT happen.
			//  Unable to locate: Program * Files -> CRS -> WWIIOnLine -> playarc directory.
#if EXE_BUILD != BUILD_EXE_SETTINGS
			dbugLog(_FLA, "&HFFF6 - Unable to locate: Program * Files -> CRS -> WWIIOnLine -> playarc directory.  Aborting.  This should NOT Happen.");
#endif
			exePlayarcPath = L"";
			exePlayarcPathSet = FALSE;
			fioStructureInit_Value = FALSE;

			//  NO FALLBACK
			goto ERR_EXIT_POINT;
		}
    }
	else
	{
		exePlayarcPathSet = TRUE;
	}
	//  exePlayarcPath: Setup - END.



	//  ScreenShots: Setup - START.
    //	userScreenShotsPath - Windows - USER Config ROOT folder:  \User -> My Documents -> WWIIOnLine -> screenshots.
    
    //  Fallback to userDocumentsPath - \User -> My Documents -> WWIIOnLine.

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
            
            if (userDocumentsPathSet == TRUE)
            {
#if EXE_BUILD != BUILD_EXE_SETTINGS
                dbugLog(_FLI, "Fallback - User Screenshot path set to User Documents path.");
#endif
                
                userScreenShotsPath = userDocumentsPath;
                userScreenShotsPathSet = TRUE;
            }
            else
            {

#if EXE_BUILD != BUILD_EXE_SETTINGS
                dbugLog(_FLI, "User Screenshots path setup - Fallbacks Failed.");
#endif
                userScreenShotsPath = L"";
                userScreenShotsPathSet = FALSE;
                
                //  Will prevent the saving os F9 generated screenshots.
                //  . . . BUT . . .
                //  WW2.exe doesn't require any of the files contained within this directory.
                
                //  fioStructureInit_Value = FALSE;
            }
		}
	}
	else
	{
		userScreenShotsPathSet = TRUE;
	}
	//  ScreenShots: Setup - END.


#elif platOS == OS_MACOS
    //  ======================================================================================
    
    const char *returnValue;
    fioStructureInit_Value = TRUE;
    

    
    //  exePath: Setup - START.
    //  exePath - macOS - EXE INSTALL ROOT folder:  /Applications -> World War II Online.app.
    //  This directory is created and populated by the app build process.
    
    //  No Fallback - ABORT.
    
    returnValue = macAppPath();
    if (returnValue != NULL)
    {
        wwiiolpath_string appPath(&returnValue[0], &returnValue[strlen(returnValue)]);
        exePath = appPath;
        exePathSet = TRUE;
    }
    else
    {
        //  Could not find World War II Online.app bundle.
        //  This should NOT happen.

        dbugLog(_FLA, "&HFFF5 - Could not find World War II Online.app bundle.  This should NOT happen.");
        
        exePath = L"";
        exePathSet = FALSE;
        fioStructureInit_Value = FALSE;
        
        //  NO FALLBACK.
        goto ERR_EXIT_POINT;
    }
    //  exePath: Setup - END.

    
    
    //  userDocumentsPath - START.
    //  userDocumentsPath - macOS - USER Config ROOT folder:  /Users -> "currentuser" -> Documents -> WWIIOnline.
    
    //  This directory is setup by the exe file at startup.
    
    //  No Fallback - ABORT.
    
    returnValue = macUserDocumentsPath();
    if (returnValue != NULL)
    {
        wwiiolpath_string macUserDocumentsPath(&returnValue[0], &returnValue[strlen(returnValue)]);
        userDocumentsPath = macUserDocumentsPath;
        userDocumentsPathSet = TRUE;
    }
    else
    {
        //  Unable to create the WWIIOnline folder within the /Users -> "currentuser" -> Documents older.
        //  This should NOT happen.
        
        dbugLog(_FLA, "&HFFF4 - Unable to create the WWIIOnline folder within the Current Users; Documents folder.  Aborting.  This should NOT Happen.");
        
        userDocumentsPath = L"";
        userDocumentsPathSet = FALSE;
        fioStructureInit_Value = FALSE;
        
        //  NO FALLBACK.
        goto ERR_EXIT_POINT;
    }
	//  userDocumentsPath - END.
    
    
    
    //  userWorkingPath: Setup - START.
    //  userWorkingPath - macOS - USER Working folder:  /Users -> "currentuser" -> Documents -> WWIIOnline -> bin.
    
    //  This directory is setup by the exe file at startup.
    //  Required by - exeWorkingPath fallback.
    
    //  Fallback to userDocumentsPath == /Users -> "currentuser" -> Documents -> WWIIOnline.
    //  Fallback to No Fallback - ABORT.
    
    returnValue = macUserWorkingPath();
    if (returnValue != NULL)
    {
        wwiiolpath_string workingPathUSER(&returnValue[0], &returnValue[strlen(returnValue)]);
        userWorkingPath = workingPathUSER;
        userWorkingPathSet = TRUE;
    }
    else
    {
        if (userDocumentsPathSet == TRUE)
        {
            //  Fallback to userDocumentsPath == /Users -> "currentuser" -> Documents -> WWIIOnline
            dbugLog(_FLI, "Setting User Working path to User Documents path.");
            userWorkingPath = userDocumentsPath;
            userWorkingPathSet = TRUE;
        }
        else
        {
            //  Setting User Working path fallbacks failed.  Aborting.
            dbugLog(_FLA, "&HFFF3 - Setting User Working path fallbacks failed.  Aborting.");
   
            userWorkingPath = L"";
            userWorkingPathSet = FALSE;
            fioStructureInit_Value = FALSE;
            
            //  NO FALLBACK
            goto ERR_EXIT_POINT;
        }
    }
    //  userWorkingPath: Setup - END.

    
    
    //  exeWorkingPath: Setup - START.
    //  exeWorkingPath - macOS - EXE Working folder:  /Library -> Application Support -> WWIIOnline.
    //  This folder should be setup by the installer.
    
    //  This folder  MUST  be setup by another porcess as the OS prevents axcess.
    //  The most likley process is the Mac DropDMG installer.
    
    //  Fallback-1 to userWorkingPath - /Users -> "currentuser" -> Documents -> WWIIOnline -> bin.
    //  Fallback-2 to userDocumentsPath - /Users -> "currentuser" -> Documents -> WWIIOnline.
    //  Fallback to No Fallback - ABORT.
    
    returnValue = macWorkingPathAPP();
    if (returnValue != NULL)
    {
        wwiiolpath_string workingPathAPP(&returnValue[0], &returnValue[strlen(returnValue)]);
        exeWorkingPath = workingPathAPP;
        exeWorkingPathSet = TRUE;
    }
    else
    {
        //  Unable to create WWIIOnLine directory within the; /Library -> Application Support directory.
        //  This is a Protected System directory. The installer should have created this directory.
        dbugLog(_FLI, "Unable to create WWIIOnLine directory within the; /Library -> Application Support directory.  This is a Protected System directory.");
        
        //  Fallback-1 to userWorkingPath - /Users -> "currentuser" -> Documents -> WWIIOnline -> bin.
        if (userWorkingPathSet == TRUE)
        {
            dbugLog(_FLI, "Fallback-1 - Setting Exe Working path to User Working path.");
            exeWorkingPath = userWorkingPath;
            exeWorkingPathSet = TRUE;
        }
        else
        {
            //  Fallback-2 to userDocumentsPath - /Users -> "currentuser" -> Documents -> WWIIOnline.
            if (userDocumentsPathSet == TRUE)
            {
                dbugLog(_FLI, "Fallback-2 - Setting Exe Working path to User Documents path.");
                exeWorkingPath = userDocumentsPath;
                exeWorkingPathSet = TRUE;
            }
            else
            {
                dbugLog(_FLA, "&HFFF1 - Unable to create WWIIOnLine directory within the /Library -> Application Support directory. Fallbacks Failed.");
                exeWorkingPath = L"";
                exeWorkingPathSet = FALSE;
                fioStructureInit_Value = FALSE;
                
                //  NO FALLBACK
                goto ERR_EXIT_POINT;
            }
        }
    }
    //  exeWorkingPath: Setup - END.
   
    
    
    //  exeDataPath: Setup - START.
    //  exeDataPath - macOS - EXE INSTALL ROOT Support Data folder:  /Applications -> World War II Online.app -> Contents -> Resources.
    
    //  WW2.exe REQUIRES the files contained within this directory to operate.
    //  This directory is created and populated by the app build process.

    //  NO Fallback.
    
    returnValue = macDataPath();
    if (returnValue != NULL)
    {
        wwiiolpath_string dataPath(&returnValue[0], &returnValue[strlen(returnValue)]);
        exeDataPath = dataPath;
        exeDataPathSet = TRUE;
    }
    else
    {
        //  This should NOT happen.
        //  Unable to locate the /Applications -> World War II Online.app -> Contents -> Resources directory.

        dbugLog(_FLI, "&HFFF7 - Unable to locate the /Applications -> World War II Online.app -> Contents -> Resources directory.  Aborting.  This should NOT Happen.");

        exeDataPath = L"";
        exeDataPathSet = FALSE;
        fioStructureInit_Value = FALSE;
        
        //  NO FALLBACK
        goto ERR_EXIT_POINT;
    }
    //  exeDataPath: Setup - START.

    
    
    //  exeDocsPath: Setup - START.
    //  exeDocsPath - macOS - EXE INSTALL ROOT Docs Folder:  /Applications -> World War II Online.app -> Contents -> Resources -> docs.
    //  This directory is created and populated by the app build process.

    //  Fallback to userDocumentsPath - /Users -> "currentuser" -> Documents -> WWIIOnline.

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
        if (userDocumentsPathSet == TRUE )
        {
            dbugLog(_FLI, "Fallback - EXE Documents path set to User Documents path.");

            exeDocsPath = userDocumentsPath;
            exeDocsPathSet = TRUE;
        }
        else
        {
            dbugLog(_FLI, "EXE Documents path setup - Fallbacks Failed.");

            exeDocsPath = L"";
            exeDocsPathSet = FALSE;
            
            //  WW2.exe doesn't use the files contained within this directory. Playgate Launcher only.
            
            //  fioStructureInit_Value = FALSE;
        }
    }
    //  exeDocsPath: Setup - END.

    
    
    //  exePlayarcPath: Setup - START.
    //  exePlayarcPath - macOS - EXE INSTALL ROOT playarc folder:  /Applications -> World War II Online.app -> Contents -> Resources == playarc.ww2.
    //  -- OR --  == "-data" command line switch.
    //  NOTE = On the MAC there is only a single file.
    
    //  WW2.exe REQUIRES the files contained within this directory to operate.
    //  This directory is created and populated by the app build process.

    //  NO Fallback.
    
    returnValue = macPlayarcPath();
    if (returnValue != NULL)
    {
        wwiiolpath_string playarcPath(&returnValue[0], &returnValue[strlen(returnValue)]);
        exePlayarcPath = playarcPath;
        exePlayarcPathSet = TRUE;
    }
    else
    {
        //  This should NOT happen.
        //  Unable to locate: Applications -> World War II Online.app -> Contents -> Resources directory.

        dbugLog(_FLA, "&HFFF6 - Unable to locate: Applications -> World War II Online.app -> Contents -> Resources directory.  Aborting.  This should NOT Happen.");

        exePlayarcPath = L"";
        exePlayarcPathSet = FALSE;
        fioStructureInit_Value = FALSE;
        
        //  NO FALLBACK
        goto ERR_EXIT_POINT;
    }
    //  exePlayarcPath: Setup - END.

    
    
    //  ScreenShots: Setup - START.
    //	userScreenShotsPath - macOS - USER Config ROOT folder:  /Users -> "currentuser" -> Documents -> WWIIOnline -> screenshots.

    //  Fallback to userDocumentsPath - /Users -> "currentuser" -> Documents -> WWIIOnline.
    
    returnValue = macUserScreenShotsPath();
    if (returnValue != NULL)
    {
        //  /Users -> "currentuser" -> Documents -> WWIIOnline -> screenshots
        //  Create Folder ONLY.
        wwiiolpath_string workingScreenShotsUSER(&returnValue[0], &returnValue[strlen(returnValue)]);
        userScreenShotsPath = workingScreenShotsUSER;
        userScreenShotsPathSet = TRUE;
    }
    else
    {
        if (userDocumentsPath != L"")
        {
            userScreenShotsPath = userDocumentsPath;
        }
        else
        {
            if (userDocumentsPathSet == TRUE)
            {
                dbugLog(_FLI, "Fallback - User Screenshot path set to User Documents path.");
                
                userScreenShotsPath = userDocumentsPath;
                userScreenShotsPathSet = TRUE;
            }
            else
            {
                dbugLog(_FLI, "User Screenshots path setup - Fallbacks Failed.");

                userScreenShotsPath = L"";
                userScreenShotsPathSet = FALSE;
                
                //  Will prevent the saving of F9 generated screenshots.
                //  . . . BUT . . .
                //  WW2.exe doesn't require any of the files contained within this directory.
                
                //  fioStructureInit_Value = FALSE;
            }
        }
    }
    //  ScreenShots: Setup - END.

#endif
    
    
    
ERR_EXIT_POINT:    //  ==========    <<--    The goto target is HERE    -->>    ==========    <<<<<<<<========<<<<<<<<
    
    

    //  Warn/Exit if issues were encountered.
    if (fioStructureInit_Value == FALSE)
    {
        wwiiolpath_string title = WWIIOnlineDirName + L" - File System.";
        wwiiolpath_string message = L"Please check your Client Install.\n\nShutting down.";

#if platOS == OS_WIN32
        MessageBox(nullptr, message.c_str(), title.c_str(), MB_ICONSTOP | MB_OK);
        
#elif platOS == OS_MACOS
        MessageBoxMAC(message.c_str(), title.c_str(), 1);
        
#endif
        exit(0);

    }
    else
    {
        WW2_exePath = exePath;
        WW2_exeDataPath = exeDataPath;
        WW2_exeDocsPath = exeDocsPath;
        WW2_exePlayarcPath = exePlayarcPath;
        WW2_exeWorkingPath = exeWorkingPath;
        WW2_userDocumentsPath = userDocumentsPath;
        WW2_userScreenShotsPath = userScreenShotsPath;
        WW2_userWorkingPath = userWorkingPath;
    }
    
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

extern wwiiolpath_string WW2_exePath;			//	exePath - Windows - EXE ROOT folder:  \Program * Files -> WWIIOnLine == *.exe, WWIIOnLine.ico, readme.*, unins*.* (uninstaller), *.tkt, *.script, bin(dir), DATA(dir), docs(dir), playarc(dir).
												//	exePath - macOS - EXE ROOT folder:  /Applications/WWIIOnline(package) == *.exe, WWIIOnLine.ico, readme.*, unins*.* (uninstaller), *.tkt, *.script, bin(dir), DATA(dir), docs(dir), playarc(dir).
extern wwiiolpath_string WW2_exeDataPath;		//	exeDataPath - EXE Support Data folder:  \Program * Files -> WWIIOnLine -> DATA == DirectXRedist(dir), loc(dir), playgate(dir), RuntimeLibraries(dir).
extern wwiiolpath_string WW2_exeDocsPath;		//	exeDocsPath - EXE Docs Folder:  \Program * Files -> WWIIOnLine -> docs == eula.txt, license.*, regions.txt, tos.txt.
extern wwiiolpath_string WW2_exePlayarcPath;	//	exePlayarcPath - EXE playarc folder:  \Program * Files -> WWIIOnLine -> playarc == playarc??.ww2  --OR--  == "-data" command line switch.
extern wwiiolpath_string WW2_exeWorkingPath;	//	exeWorkingPath - Windows - EXE Working folder:  \ProgramData -> WWIIOnLine == ftplog.txt, downloaded patches, WWIIOL.log, *.log.
												//	exeWorkingPath - macOS - EXE Working folder:  /Library/Application Support//WWIIOnline == ftplog.txt, downloaded patches, WWIIOL.log, *.log.
extern wwiiolpath_string WW2_userDocumentsPath;	//	userDocumentsPath - Windows - USER Config ROOT folder:  \User -> My Documents -> WWIIOnLine == wwiiol.xml, cfml(dir).
												//	userDocumentsPath - macOS - USER Config ROOT folder:  /Users/"currentuser"/Documents/WWIIOnline == wwiiol.xml, cfml(dir).
extern wwiiolpath_string WW2_userWorkingPath;	//	userWorkingPath - USER Working folder:  \User -> My Documents -> WWIIOnLine -> bin == lg.dat.

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


// Retrieve the current time to millisecond resolution in portable fashion.
// The extra math is needed thanks to design flaws in C++'s chrono library.
// From dbug.cpp line : 496.
static void _getCurrentTime(struct tm &tm, uint32_t &millis)
{
	auto now = std::chrono::system_clock::now();
	time_t nowTime = std::chrono::system_clock::to_time_t(now);
	auto nowRounded = std::chrono::system_clock::from_time_t(nowTime);

	// If the time value was rounded upward instead of truncated or rounded down,
	// adjust for the rounding.
	if (nowRounded > now) {
		--nowTime;
		nowRounded -= std::chrono::seconds(1);
	}

	localtime_r(&nowTime, &tm);
	millis = std::chrono::duration_cast<std::chrono::duration<uint32_t, std::milli>>(now - nowRounded).count();
}


//  Returns an SQL Formatted Date/Time Stamp string. YYYY-MM-DD HH:MM:SS.MS
wwiiolpath_string SQL_DateTimeStamp(bool compact)
{

	//  Create a SQL formatted date/time stamp.
	wwiiolpath_string returnValue = L"";
	int n = 0;
	struct tm timeinfo;
	uint32_t milisec;
	wchar_t timeStamp[256] = L"";
	std::wstring formatString = L"";

	if (compact == true)
	{
		formatString = L"%.4d%.2d%.2d-%.2d%.2d%.2d%.3d";
	}
	else
	{
		formatString = L"%.4d-%.2d-%.2d %.2d:%.2d:%.2d.%.3d";
	}

//  time_t rawtime;
//	time(&rawtime);
//	timeinfo = localtime(&rawtime);
	//  n = swprintf(timeStamp, 256, L"%.4d%.2d%.2d-%.2d%.2d%.2d"

	_getCurrentTime(timeinfo, milisec);

	//  n = swprintf(timeStamp, 256, L"%.4d-%.2d-%.2d %.2d:%.2d:%.2d.%.3d"
	n = swprintf(timeStamp, 256, formatString.c_str()
		, timeinfo.tm_year + 1900
		, timeinfo.tm_mon + 1
		, timeinfo.tm_mday
		, timeinfo.tm_hour
		, timeinfo.tm_min
		, timeinfo.tm_sec
		, milisec);

	returnValue = timeStamp;

	timeStamp[0] = 0;
	formatString = L"";

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
	folderRename = L"_Copied_" + SQL_DateTimeStamp(true);


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
			//   \User -> My Documents -> "Battleground Europe_Copied_YYYY-MM-DD HH:MM:SS.MS".

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

				dbugLog(_FLW, "Old Settings - Could not rename:-\n\nfrom path : \n<< %s >>\n\nto path : \n<< %s >>", &source, &destination);
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


bool SHFileFunctions_WC(wwiiolpath_string sourcePath, wwiiolpath_string destinationPath, unsigned int FO_Function)
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
