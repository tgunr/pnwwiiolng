@echo off

:: =====================================================================================================

:: _PrepareNewRelease.bat - Command Line:

:: _PrepareNewRelease.bat [ /rc | /alpha | /beta | /rcdb | /alphadb | /betadb ]


:: Default = /rc  (.\release-candidate -- ww2ExePath=..\.\build\client\Release\bin):

:: Available Options:
:: /rc = .\release-candidate -- ww2ExePath=..\.\build\client\Release\bin
:: /alpha = .\release-alpha -- ww2ExePath=..\.\build\client\Release\bin
:: /beta = .\release-beta -- ww2ExePath=..\.\build\client\Release\bin

:: /rcdb = .\release-candidate -- ww2ExePath=..\.\build\client\Debug\bin
:: /alphadb = .\release-alpha -- ww2ExePath=..\.\build\client\Debug\bin
:: /betadb = .\release-beta -- ww2ExePath=..\.\build\client\Debug\bin

:: Launching Directory = The directory where the BAT file is located when it was Launched.
:: NOTE: ALL of the other paths are relative to this directory within the < wwiiolng > directory tree.

:: EXIT Codes:
:: 1 = for some reason the Launching Directory is not set.
:: 2 = unable to locate the _PrepareNewRelease.bat (this) file. This file should be located in the Launching Directory.
:: 3 = unable to locate the wwiiolng Root Directory.
:: 4 = unable to locate the ww2mods Root Directory.
:: 5 = Invalaid Command Line Switch.
:: 6 = Insufficient Playarc files Present.
:: 7 = Completed - With Errors.

:: Mathew J Hogan (james10) 8-Apr-2018, 25-June-2019.

:: Disable / Enable = DebugBat
SET DebugBat=false
rem SET DebugBat=true

:: The main release.sh script uses the opt/ww2mods directory within the wwiiolng directory AND NOT a separate ww2mods directory.
:: SET DevSetupMODS=false = release.sh script use (wwiiolng/opt/ww2mods)
rem SET DevSetupMODS=true
SET DevSetupMODS=false

:: =====================================================================================================

@echo.
@echo.
@echo===================================================================================================
@echo.
@echo       Preparing the Source Directories for a NEW release. Please wait. . .
@echo.
@echo===================================================================================================

rem SET LaunchingDir=%CD% - Has < Run as Admin > issues.
SET launchingdir=%~dp0

IF NOT DEFINED LaunchingDir (
	@echo.
	@echo.
	@echo _PrepareNewRelease.bat: Launching Directory is not set - ABORTING.
	IF /I %DebugBat% == true @pause

	EXIT /B 1
)

CD /D "%LaunchingDir%"
@echo Bat File Launching Directory  :  %CD%
@echo.

rem If _PrepareNewRelease.bat file is not here, we are probably in the WRONG directory which can be VERY BAD.
IF /I NOT EXIST "_PrepareNewRelease.bat" (
	@echo.
	@echo.
	@echo _PrepareNewRelease.bat: file NOT found - ABORTING.
	IF /I %DebugBat% == true @pause

	EXIT /B 2
) else (
	@echo Located _PrepareNewRelease.bat file.
	@echo.
)

SET ReleaseVersion=%~1%

IF NOT DEFINED ReleaseVersion (
	@echo _PrepareNewRelease.bat: Using /rc command line option.
	@echo.
	SET ReleaseVersion=/rc
)

rem Locate and Set the Repos Root Directory.
CD /D "%LaunchingDir%"\..\..

SET RepoRootDIR=%CD%

CD /D "%RepoRootDIR%

rem Locate and Set the wwiiolng Root Directory.
IF /I NOT EXIST "wwiiolng\" (
	@echo.
	@echo wwiiolng: directory NOT found - ABORTING.
	IF /I %DebugBat% == true @pause

	EXIT /B 3
) else (
	@echo Located the wwiiolng Directory.
	SET wwiiolngRootDIR=%RepoRootDIR%\wwiiolng
)

rem Locate and Set the ww2mods Root Directory.
CD /D "%LaunchingDir%"

IF %DevSetupMODS% == true GOTO devMODS
IF %DevSetupMODS% == false GOTO scriptMODS

@echo ====  _PrepareNewRelease.bat: Invalaid DevSetupMODS setting - ABORTING.

IF /I %DebugBat% == true @pause

EXIT /B 5
rem ==========

:devMODS
CD /D "%RepoRootDIR%

IF /I NOT EXIST "ww2mods\" (
	@echo.
	@echo ww2mods: directory NOT found - ABORTING.
	IF /I %DebugBat% == true @pause

	EXIT /B 4
) else (
	@echo Located the ww2mods Directory - Default Dev usage.
	SET modsRootDIR=%RepoRootDIR%\ww2mods
)

GOTO setupModsDone
rem ==========

:scriptMODS
CD /D "%wwiiolngRootDIR%"

IF /I NOT EXIST "opt\ww2mods\" (
	@echo.
	@echo ww2mods: directory NOT found - ABORTING.
	IF /I %DebugBat% == true @pause

	EXIT /B 4
) else (
	@echo Located the ww2mods Directory - Release Script usage.
	SET modsRootDIR=%wwiiolngRootDIR%\opt\ww2mods
)

:setupModsDone

CD /D "%LaunchingDir%"

SET ww2AssetsPath=%wwiiolngRootDIR%\Installer\ww2assets

rem Release EXE versions.
IF /I "%ReleaseVersion%" == "/RC" (
	SET TargetRootPath=%LaunchingDir%\release-candidate
	SET ww2ExePath=%wwiiolngRootDIR%\build\client\Release\bin
	SET ww2PlayarcPath="%ww2AssetsPath%"\oswin\archivefiles\release-candidate
	GOTO check1
)

IF /I "%ReleaseVersion%" == "/ALPHA" (
	SET TargetRootPath=%LaunchingDir%\release-alpha
	SET ww2ExePath=%wwiiolngRootDIR%\build\client\Debug\bin
	SET ww2PlayarcPath="%ww2AssetsPath%"\oswin\archivefiles\release-alpha
	GOTO check1
)

IF /I "%ReleaseVersion%" == "/BETA" (
	SET TargetRootPath=%LaunchingDir%\release-beta
	SET ww2ExePath=%wwiiolngRootDIR%\build\client\BetaRelease\bin
	SET ww2PlayarcPath="%ww2AssetsPath%"\oswin\archivefiles\release-beta
	GOTO check1
)

rem Debug EXE versions.
IF /I "%ReleaseVersion%" == "/RCDB" (
	SET TargetRootPath=%LaunchingDir%\release-candidate
	SET ww2ExePath=%wwiiolngRootDIR%\build\client\Debug\bin
	GOTO check1
)

IF /I "%ReleaseVersion%" == "/ALPHADB" (
	SET TargetRootPath=%LaunchingDir%\release-alpha
	SET ww2ExePath=%wwiiolngRootDIR%\build\client\Debug\bin
	GOTO check1
)

IF /I "%ReleaseVersion%" == "/BETADB" (
	SET TargetRootPath=%LaunchingDir%\release-beta
	SET ww2ExePath=%wwiiolngRootDIR%\build\client\Debug\bin
	GOTO check1
)

@echo.
@echo.
@echo.
@echo ====  _PrepareNewRelease.bat: Invalaid Command Line Switch - ABORTING.
IF /I %DebugBat% == true @pause

EXIT /B 5
rem ==========

:check1

IF /I %DebugBat% == true GOTO bypass1

CD /D "%ww2PlayarcPath%"
@echo.
rem set filesCount=0 & for %f in (*) do @(set /a filesCount+=1 > nul)
rem dir /b *.ww2 /s 2> nul | find "" /v /c > tmp && set /p count=<tmp && del tmp && echo %count%
dir /b *.ww2 /s 2> nul | find "" /v /c > tmp && set /p count=<tmp && del tmp
@echo Playarc Files Present: %count% required 10.
@echo.

IF NOT "%count%" == "10" (
	@echo Incorrect number of Playarc files present - ABORTING.
	@echo.
	IF /I %DebugBat% == true @pause

	EXIT /B 6
)

CD /D "%LaunchingDir%"

:bypass1

@echo.
@echo.
@echo===================================================================================================
@echo.
CD /D "%LaunchingDir%"
@echo Bat File Launching Directory  ==  :  %CD%
@echo.

rem CD /D "%RepoRootDIR%"
rem @echo Repo Root Directory  ===========  :  %CD%
rem @echo.

rem CD /D "%LaunchingDir%"
CD /D "%wwiiolngRootDIR%"
@echo wwiiolng Root Directory  =======  :  %CD%
@echo.

CD /D "%LaunchingDir%"
CD /D "%modsRootDIR%"
@echo ww2mods Root Directory  ========  :  %CD%
@echo.


CD /D "%TargetRootPath%"
@echo Target Root Path  ==============  :  %CD%
@echo.

@echo Release Version  ===============  :  %ReleaseVersion%
@echo.

CD /D "%LaunchingDir%"
CD /D "%ww2AssetsPath%"
@echo WW2 Assets Path  ===============  :  %CD%
@echo.

CD /D "%LaunchingDir%"
CD /D "%ww2PlayarcPath%"
@echo WW2 Playarc Path  ==============  :  %CD%
@echo.

CD /D "%LaunchingDir%"
CD /D "%ww2ExePath%"
@echo WW2 EXE Path:  =================  :  %CD%

IF /I %DebugBat% == true ( 
	@echo.
	@echo Debug:  ========================  :  ON
)
@echo.
@echo===================================================================================================
@echo.
@echo.

IF /I %DebugBat% == true @pause

CD /D "%LaunchingDir%"
@echo ====  Current Working Directory  :  %CD%

@echo.
@echo.
@echo.
CD /D "%TargetRootPath%\previous"
@echo Deleting ALL files and directories in %CD%
@echo.

IF /I NOT EXIST "WW2.exe" (
	IF /I NOT EXIST "Settings.exe" (
		@echo WW2.exe file NOT found, Possible Empty directory.
	) ELSE (
		@echo Located Settings.exe file.
	)
) ELSE (
	@echo Located WW2.exe file.
)

@echo.

rem ===================================================================================================
rem   Directory / File Deletion (\previous).

rem Delete All files contained within the target directory and sub-directories, with console echo.
rem @echo Deleting Files.
rem DEL *.* /S /Q

rem Change to the Pearent Directory
CD ..

rem Delete / Remove the target directory tree, including any files.
@echo Deleting Directory Tree.
RMDIR /S /Q previous

rem Recreate the removed directory.
MD previous

rem Make the newly recreated directory the Current Directory and continue.
CD previous

rem ===================================================================================================

@echo.
@echo %CD% should now be Empty.
@echo.
@echo.
@echo.

CD /D "%LaunchingDir%"
CD /D "%TargetRootPath%\diff"
@echo ====  Deleting the contents of  :  %CD%
@echo.

IF /I NOT EXIST "WW2.exe.patch" (
	IF /I NOT EXIST "Settings.exe.patch" (
		@echo WW2.exe.patch file NOT found, Possible Empty directory.
		@echo.
	) ELSE (
		@echo Located Settings.exe.patch file.
		@echo.
	)
) ELSE (
	@echo Located WW2.exe.patch file.
	@echo.
)

rem ===================================================================================================
rem   Directory / File Deletion (\diff).

rem Delete All files contained within the target directory and sub-directories, with console echo.
rem @echo Deleting Files.
rem DEL *.* /S /Q

rem Change to the Pearent Directory
CD ..

rem Delete / Remove the target directory tree, including any files.
@echo Deleting Directory Tree.
RMDIR /S /Q diff

rem Recreate the removed directory.
MD diff

rem Make the newly recreated directory the Current Directory and continue.
CD diff

rem ===================================================================================================

@echo.
@echo %CD% should now be Empty.
@echo.
@echo.
@echo.

CD /D "%LaunchingDir%"
CD /D "%TargetRootPath%\current"
@echo ====  Moving files from the current to previous directory.
@echo.

IF /I NOT EXIST "WW2.exe" (
	IF /I NOT EXIST "Settings.exe" (
		@echo WW2.exe file NOT found, Possible Empty directory.
	) ELSE (
		@echo Located Settings.exe file.
	)
) ELSE (
	@echo Located WW2.exe file.
)

CD /D "%LaunchingDir%"

@echo.
@echo Copying files to the previous directory.
@echo.
XCOPY "%TargetRootPath%\current\*.*" "%TargetRootPath%\previous\" /S /E /V

@echo.
@echo.
@echo.
CD /D "%TargetRootPath%\current"
@echo Deleting ALL files within %CD%
@echo.

rem ===================================================================================================
rem   Directory / File Deletion (\current).

rem Delete All files contained within the target directory and sub-directories, with console echo.
rem @echo Deleting Files.
rem DEL *.* /S /Q

rem Change to the Pearent Directory
CD ..

rem Delete / Remove the target directory tree, including any files.
@echo Deleting Directory Tree.
RMDIR /S /Q current

rem Recreate the removed directory.
MD current

rem Make the newly recreated directory the Current Directory and continue.
CD current

rem ===================================================================================================

@echo.
@echo.
@echo %CD% should now be Empty.
@echo.
@echo.
@echo.

@echo===================================================================================================
@echo.
@echo ====  Preparing and Copying files into the Installer Source Directory.
@echo.
@echo===================================================================================================
@echo.
@echo.

rem CD /D "%TargetRootPath%\current"
@echo ====  Creating and populating the DATA directory under %CD%
MD data
MD data\screens

CD /D "%LaunchingDir%"

@echo.
@echo.
@echo Copying files to the Data directory.
@echo.
XCOPY "%ww2assetspath%\oswin\dependencies\*.*" "%TargetRootPath%\current\data\" /S /E /V

@echo.
@echo.
@echo Copying splash screen files to the Data directory.
@echo.
XCOPY "%ww2assetspath%\screens\*.*" "%TargetRootPath%\current\data\screens\" /S /E /V

@echo.
@echo.
CD /D "%TargetRootPath%\current"
@echo ====  Creating and populating the playarc directory under %CD%
MD playarc

@echo.
CD playarc
@echo Copying files to %CD%
CD /D "%LaunchingDir%"
@echo.
XCOPY "%ww2PlayarcPath%\*.ww2" "%TargetRootPath%\current\playarc\" /V

@echo.
@echo.
CD /D "%TargetRootPath%\current"
@echo ====  Copying the assets-docs directory to %CD%
MD docs
CD /D "%LaunchingDir%"

@echo.
XCOPY "%ww2assetspath%\docs\*.*" "%TargetRootPath%\current\docs\" /V 

@echo.
@echo.
CD /D "%TargetRootPath%\current"
@echo ====  Copying the the file Playgate.exe to %CD%
CD /D "%LaunchingDir%"
@echo.
XCOPY "%ww2ExePath%\Playgate.exe" "%TargetRootPath%\current\" /V

@echo.
@echo.
CD /D "%TargetRootPath%\current"
@echo ====  Copying the the file Settings.exe to %CD%
CD /D "%LaunchingDir%"
@echo.
XCOPY "%ww2ExePath%\Settings.exe" "%TargetRootPath%\current\" /V

@echo.
@echo.
CD /D "%TargetRootPath%\current"
@echo ====  Copying the the file WW2.exe to %CD%
CD /D "%LaunchingDir%"
@echo.
XCOPY "%ww2ExePath%\WW2.exe" "%TargetRootPath%\current\" /V

CD /D "%TargetRootPath%\current"
@echo.
@echo.
CD /D "%TargetRootPath%\current"
@echo ====  Copying the files readme.rtf and readme.txt to %CD%
CD /D "%LaunchingDir%"
@echo.
XCOPY "%ww2assetspath%\readme.*" "%TargetRootPath%\current\" /V 

rem The following files no longer appear to be required so will not be copied.
rem @echo.
rem @echo.
rem CD /D "%TargetRootPath%\current"
rem @echo ====  Copying the the files in the assets-libs directory to %CD%
rem CD /D "%LaunchingDir%"
rem @echo.
rem XCOPY "%ww2assetspath%\oswin\libs\*.*" "%TargetRootPath%\current\" /V

@echo.
@echo.
CD /D "%TargetRootPath%\current"
@echo ====  Copying Steam Files to %CD%
CD /D "%LaunchingDir%"
@echo.
XCOPY "%ww2assetspath%\steam\*.*" "%TargetRootPath%\current\" /V

@echo.
@echo.
CD /D "%TargetRootPath%\current"
@echo ====  Copying the the file WWIIOnLine.ico to %CD%
CD /D "%LaunchingDir%"
@echo.
XCOPY "%ww2assetspath%\oswin\WWIIOnLine.ico" "%TargetRootPath%\current\" /V

@echo off

@echo.
@echo.
@echo.
@echo===================================================================================================
@echo.
IF %ERRORLEVEL% NEQ 0 (
	@echo ====  Completed - With Errors.
	@echo Further investigation will be required.
	@echo.
	@echo===================================================================================================
	@echo.
	@echo.

	IF /I %DebugBat% == true @pause

	EXIT /B 7
	
) else (
	@echo ====  Completed.
	
	IF /I %DevSetupMODS% == "true" (
		@echo.
		@echo You can now run the appropriate _MakeDiffs.bat to populate the diff directory for use by the patch installer script.
		@echo.
		@echo The %TargetRootPath% directory should be sufficiently populated for the full installer script to be run.
	)
)
@echo.
@echo===================================================================================================
@echo.
@echo.

IF /I %DebugBat% == true @pause

