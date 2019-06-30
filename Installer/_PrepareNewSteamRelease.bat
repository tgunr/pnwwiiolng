@echo off

:: =====================================================================================================

:: _PrepareNewSteamRelease.bat - Command Line:

:: _PrepareNewSteamRelease.bat [ /rc | /alpha | /beta ]


:: Default = /rc  (.\release-candidate\steam\WWIIOnLine -- ww2ExePath=..\.\build\client\Release\bin):

:: Available Options:
:: /rc     = .\release-candidate\steam\WWIIOnLine    -- ww2ExePath=..\.\build\client\Release\bin       <---
:: /alpha  = .\release-alpha\steam\WWIIOnLine_Alpha  -- ww2ExePath=..\.\build\client\Debug\bin
::                                                   -- ww2ExePath=..\.\build\client\AlphaRelease\bin
::                                                   -- ww2ExePath=..\.\build\client\BetaRelease\bin   <---
:: /beta   = .\release-beta\steam\WWIIOnLine_Beta    -- ww2ExePath=..\.\build\client\BetaRelease\bin   <---

:: Launching Directory = The directory where the BAT file is located when it was Launched.
:: NOTE: ALL of the other paths are relative to this directory within the < wwiiolng > directory tree.

:: EXIT Codes:
:: 1 = for some reason the Launching Directory is not set.
:: 2 = unable to locate the _PrepareNewRelease.bat (this) file. This file should be located in the Launching Directory.
:: 3 = unable to locate the wwiiolng Root Directory.
:: 4 = unable to locate the wwiiolng\Installer Root Directory.
:: 5 = Invalaid Command Line Switch.
:: 6 = Insufficient Playarc files Present.
:: 7 = Completed - With Errors.

:: Mathew J Hogan (james10) 8-Apr-2018, 25-June-2019.

:: Disable / Enable = DebugBat
SET DebugBat=false
rem SET DebugBat=true

:: The main release.sh script uses the opt/ww2mods directory within the wwiiolng directory AND NOT a separate ww2mods directory.
:: SET DevSetupMODS=false = release.sh script use (wwiiolng/opt/ww2mods)
SET DevSetupMODS=false
rem SET DevSetupMODS=true

:: =====================================================================================================

@echo.
@echo.
@echo===================================================================================================
@echo.
@echo       Preparing the Source Directories for a NEW Steam release. Please wait. . .
@echo.
@echo===================================================================================================

rem SET LaunchingDir=%CD% - Has < Run as Admin > issues.
SET launchingdir=%~dp0

IF NOT DEFINED LaunchingDir (
	@echo.
	@echo.
	@echo _PrepareNewSteamRelease.bat: Launching Directory is not set - ABORTING.
	IF /I %DebugBat% == true @pause

	EXIT /B 1
)

CD /D "%LaunchingDir%"
@echo Bat File Launching Directory  :  %CD%
@echo.

rem If _PrepareNewSteamRelease.bat file is not here, we are probably in the WRONG directory which can be VERY BAD.
IF /I NOT EXIST "_PrepareNewSteamRelease.bat" (
	@echo.
	@echo.
	@echo _PrepareNewSteamRelease.bat: file NOT found - ABORTING.
	IF /I %DebugBat% == true @pause

	EXIT /B 2
) else (
	@echo Located _PrepareNewSteamRelease.bat file.
	@echo.
)

SET ReleaseVersion=%~1%

IF NOT DEFINED ReleaseVersion (
	@echo _PrepareNewSteamRelease.bat: Using /rc command line option.
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

rem Locate and Set the Installer Root Directory.
IF /I NOT EXIST "wwiiolng\Installer\" (
	@echo.
	@echo wwiiolng\Installer: directory NOT found - ABORTING.
	IF /I %DebugBat% == true @pause

	EXIT /B 4
) else (
	@echo Located the wwiiolng\Installer Directory.
	SET installerRootDIR=%RepoRootDIR%\wwiiolng\Installer
)

CD /D "%LaunchingDir%"

SET ww2AssetsPath=%installerRootDIR%\ww2assets

rem Release EXE versions.
IF /I "%ReleaseVersion%" == "/RC" (
	SET TargetRootPath=%LaunchingDir%\release-candidate\steam\WWIIOnLine
	SET ww2ExePath=%wwiiolngRootDIR%\build\client\Release\bin
	SET ww2PlayarcPath="%ww2AssetsPath%"\oswin\archivefiles\release-candidate
	GOTO check1
)

IF /I "%ReleaseVersion%" == "/ALPHA" (
	SET TargetRootPath=%LaunchingDir%\release-alpha\steam\WWIIOnLine_Alpha
	rem SET ww2ExePath=%wwiiolngRootDIR%\build\client\Debug\bin
	rem SET ww2ExePath=%wwiiolngRootDIR%\build\client\AlphaRelease\bin
	SET ww2ExePath=%wwiiolngRootDIR%\build\client\BetaRelease\bin
	SET ww2PlayarcPath="%ww2AssetsPath%"\oswin\archivefiles\release-alpha
	GOTO check1
)

IF /I "%ReleaseVersion%" == "/BETA" (
	SET TargetRootPath=%LaunchingDir%\release-beta\steam\WWIIOnLine_Beta
	SET ww2ExePath=%wwiiolngRootDIR%\build\client\BetaRelease\bin
	SET ww2PlayarcPath="%ww2AssetsPath%"\oswin\archivefiles\release-beta
	GOTO check1
)

@echo.
@echo.
@echo.
@echo ====  _PrepareNewSteamRelease.bat: Invalaid Command Line Switch - ABORTING.
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

CD /D "%wwiiolngRootDIR%"
@echo wwiiolng Root Directory  =======  :  %CD%
@echo.

CD /D "%LaunchingDir%"
CD /D "%installerRootDIR%"
@echo Installer Root Directory  ======  :  %CD%
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
CD /D "%TargetRootPath%"
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
rem   Directory / File Deletion (\WWIIOnLine - ???).

IF /I "%ReleaseVersion%" == "/RC" (
	rem Delete All files contained within the target directory and sub-directories, with console echo.
	rem @echo Deleting Files.
	rem DEL *.* /S /Q

	rem Change to the Pearent Directory
	CD ..

	rem Delete / Remove the target directory tree, including any files.
	@echo Deleting Directory Tree - RC.
	RMDIR /S /Q WWIIOnLine

	rem Recreate the removed directory.
	MD WWIIOnLine

	rem Make the newly recreated directory the Current Directory and continue.
	CD WWIIOnLine

	GOTO check2
)

IF /I "%ReleaseVersion%" == "/ALPHA" (
	rem Delete All files contained within the target directory and sub-directories, with console echo.
	rem @echo Deleting Files.
	rem DEL *.* /S /Q

	rem Change to the Pearent Directory
	CD ..

	rem Delete / Remove the target directory tree, including any files.
	@echo Deleting Directory Tree - Alpha.
	RMDIR /S /Q WWIIOnLine_Alpha

	rem Recreate the removed directory.
	MD WWIIOnLine_Alpha

	rem Make the newly recreated directory the Current Directory and continue.
	CD WWIIOnLine_Alpha

	GOTO check2
)

IF /I "%ReleaseVersion%" == "/BETA" (
	rem Delete All files contained within the target directory and sub-directories, with console echo.
	rem @echo Deleting Files.
	rem DEL *.* /S /Q

	rem Change to the Pearent Directory
	CD ..

	rem Delete / Remove the target directory tree, including any files.
	@echo Deleting Directory Tree - Beta.
	RMDIR /S /Q WWIIOnLine_Beta

	rem Recreate the removed directory.
	MD WWIIOnLine_Beta

	rem Make the newly recreated directory the Current Directory and continue.
	CD WWIIOnLine_Beta

	GOTO check2
)

rem ===================================================================================================


:check2

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

CD /D "%TargetRootPath%"
@echo ====  Creating and populating the DATA directory under %CD%
MD data
MD data\loc
MD data\screens

CD /D "%LaunchingDir%"

@echo.
@echo.
@echo Copying Localisation files to the Data directory.
@echo.
XCOPY "%ww2assetspath%\oswin\dependencies\loc\*.*" "%TargetRootPath%\data\loc\" /S /E /V

@echo.
@echo.
@echo Copying splash screen files to the Data directory.
@echo.
XCOPY "%ww2assetspath%\screens\*.*" "%TargetRootPath%\data\screens\" /S /E /V

@echo.
@echo.
CD /D "%TargetRootPath%"
@echo ====  Creating and populating the playarc directory under %CD%
MD playarc

@echo.
CD playarc
@echo Copying files to %CD%
CD /D "%LaunchingDir%"
@echo.
XCOPY "%ww2PlayarcPath%\*.ww2" "%TargetRootPath%\playarc\" /V

@echo.
@echo.
CD /D "%TargetRootPath%"
@echo ====  Copying the assets-docs directory to %CD%
MD docs
CD /D "%LaunchingDir%"

@echo.
XCOPY "%ww2assetspath%\docs\*.*" "%TargetRootPath%\docs\" /V 
DEL "%TargetRootPath%\docs\regions.txt" /Q

@echo.
@echo.
CD /D "%TargetRootPath%"
@echo ====  Copying the the file Playgate.exe to %CD%
CD /D "%LaunchingDir%"
@echo.
XCOPY "%ww2ExePath%\Playgate.exe" "%TargetRootPath%\" /V

@echo.
@echo.
CD /D "%TargetRootPath%"
@echo ====  Copying the the file Settings.exe to %CD%
CD /D "%LaunchingDir%"
@echo.
XCOPY "%ww2ExePath%\Settings.exe" "%TargetRootPath%\" /V

@echo.
@echo.
CD /D "%TargetRootPath%"
@echo ====  Copying the the file WW2.exe to %CD%
CD /D "%LaunchingDir%"
@echo.
XCOPY "%ww2ExePath%\WW2.exe" "%TargetRootPath%\" /V

@echo.
@echo.
CD /D "%TargetRootPath%"
@echo ====  Copying the files readme.rtf and readme.txt to %CD%
CD /D "%LaunchingDir%"
@echo.
XCOPY "%ww2assetspath%\readme.*" "%TargetRootPath%\" /V 

@echo.
@echo.
CD /D "%TargetRootPath%"
@echo ====  Copying Steam Files to %CD%
CD /D "%LaunchingDir%"
@echo.
XCOPY "%ww2assetspath%\steam\steam_api.dll" "%TargetRootPath%\" /V

@echo.
@echo.
CD /D "%TargetRootPath%"
@echo ====  Copying the file WWIIOnLine.ico to %CD%
CD /D "%LaunchingDir%"
@echo.
XCOPY "%ww2assetspath%\oswin\WWIIOnLine.ico" "%TargetRootPath%\" /V

@echo off

@echo.
@echo.
@echo.
@echo===================================================================================================
@echo.
IF %ERRORLEVEL% NEQ 0 (
	@echo ====  New Steam Release Completed - With Errors.
	@echo Further investigation will be required.
	@echo.
	@echo===================================================================================================
	@echo.
	@echo.

	IF /I %DebugBat% == true @pause

	EXIT /B 7
	
) else (
	@echo ====  New Steam Release Completed.
)
@echo.
@echo===================================================================================================
@echo.
@echo.

IF /I %DebugBat% == true @pause

