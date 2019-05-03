@echo off

:: =====================================================================================================

:: _PlayArchBuilder.bat - Command Line:

:: _PlayArchBuilder.bat  [ /rc | /alpha | /beta ]

:: Defaults = /rc (release candidate)
:: Playarc_Output = %wwiiolngRootDIR%\ww2assets\oswin\archivefiles\release-candidate

:: Launching Directory = The directory where the BAT file is located when it was Launched.
:: NOTE: ALL of the other paths are relative to this directory within the < wwiiolng > directory tree.

:: EXIT Codes:
:: 1 = for some reason the Launching Directory is not set.
:: 2 = unable to locate the _PlayArchBuilder.bat (this) file. This file should be located in the Launching Directory.
:: 3 = unable to locate the wwiiolng Root Directory.
:: 4 = unable to locate the ww2mods Root Directory.
:: 5 = Invalaid Command Line Switch.
:: 6 = Completed - With Errors.

:: Mathew J Hogan (james10) 8-Apr-2018, 3-May-2019.

:: Disable / Enable = DebugBat.
:: One option MUST be SET.
SET DebugBat="false"
rem SET DebugBat="true"

:: =====================================================================================================


@echo.
@echo.
@echo===================================================================================================
@echo.
@echo       Creating game playarc files. Please wait. . .
@echo.
@echo===================================================================================================
@echo.
@echo.

rem SET LaunchingDir=%CD% - Has < Run as Admin > issues.
SET launchingdir=%~dp0

IF NOT DEFINED LaunchingDir (
	@echo.
	@echo.
	@echo _PlayArchBuilder.bat: Launching Directory is not set - ABORTING.
	IF /I %DebugBat% == "true" @pause

	EXIT 1
)

CD /D "%LaunchingDir%"
@echo Bat File Launching Directory  :  %CD%
@echo.

rem If _PlayArchBuilder.bat file is not here, we are probably in the WRONG directory which can be VERY BAD.
IF /I NOT EXIST "_PlayArchBuilder.bat" (
	@echo.
	@echo.
	@echo _PlayArchBuilder.bat: file NOT found - ABORTING.
	IF /I %DebugBat% == "true" @pause

	EXIT 2
) else (
	@echo Located _PlayArchBuilder.bat file.
	@echo.
)

SET ReleaseVersion=%~1%

IF NOT DEFINED ReleaseVersion (
	@echo _PlayArchBuilder.bat: Using /rc command line option.
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
	IF /I %DebugBat% == "true" @pause

	EXIT 3
) else (
	@echo Located the wwiiolng Directory.
	SET wwiiolngRootDIR=%RepoRootDIR%\wwiiolng
)

rem Locate and Set the ww2mods Root Directory.
IF /I NOT EXIST "ww2mods\" (
	@echo.
	@echo ww2mods: directory NOT found - ABORTING.
	IF /I %DebugBat% == "true" @pause

	EXIT 4
) else (
	@echo Located the ww2mods Directory.
	SET modsRootDIR=%RepoRootDIR%\ww2mods
)

CD /D "%LaunchingDir%"

rem Archiving files - Source Files.
SET Playarc_BIN=%wwiiolngRootDIR%\Tools\PlayBuilder
SET Playarc_DATA=%modsRootDIR%\DATA

rem Archive files - Output Store.
IF /I "%ReleaseVersion%" == "/RC" (
	SET Playarc_Output=%wwiiolngRootDIR%\ww2assets\oswin\archivefiles\release-candidate
	GOTO check1
)

IF /I "%ReleaseVersion%" == "/ALPHA" (
	SET Playarc_Output=%wwiiolngRootDIR%\ww2assets\oswin\archivefiles\release-alpha
	GOTO check1
)

IF /I "%ReleaseVersion%" == "/BETA" (
	SET Playarc_Output=%wwiiolngRootDIR%\ww2assets\oswin\archivefiles\release-beta
	GOTO check1
)

@echo.
@echo.
@echo.
@echo ====  _PlayArchBuilder: Invalaid Command Line Switch - ABORTING.
IF /I %DebugBat% == "true" @pause

EXIT 5

:check1

@echo.
@echo===================================================================================================
@echo.
CD /D "%LaunchingDir%"
@echo Bat File Launching Directory  ==  :  %CD%
@echo.

rem CD /D "%RepoRootDIR%"
rem @echo Repo Root Directory  =========  :  %CD%
rem @echo.

rem CD /D "%LaunchingDir%"
CD /D "%wwiiolngRootDIR%"
@echo wwiiolng Root Directory  =======  :  %CD%
@echo.

CD /D "%LaunchingDir%"
CD /D "%modsRootDIR%"
@echo ww2mods Root Directory  ========  :  %CD%
@echo.

@echo Release Version  ===============  :  %ReleaseVersion%
@echo.

CD /D "%LaunchingDir%"
CD /D "%Playarc_BIN%"
@echo Play Builder EXE Path  =========  :  %CD%
@echo.

CD /D "%LaunchingDir%"
CD /D "%Playarc_DATA%"
@echo Play Builder DATA Path  ========  :  %CD%
@echo.

CD /D "%LaunchingDir%"
CD /D "%Playarc_Output%"
@echo Play Builder OUTPUT Path:  =====  :  %CD%
@echo.
@echo===================================================================================================
@echo.
@echo.

IF /I %DebugBat% == "true" @pause

CD /D "%LaunchingDir%"

@echo.
@echo Playarc Files - Setup:
@echo.
@echo.

rem ===================================================================================================
rem   Directory / File Deletion (playarc files).

CD /D "%Playarc_Output%"

rem Delete All files contained within the target directory and sub-directories, with console echo.
@echo ====  Deleting Old Playarc Files in %CD%

@echo.
DEL playarc*.txt /S /Q

@echo.
DEL playarc*.ww2 /S /Q

CD /D "%LaunchingDir%"
rem ===================================================================================================

@echo.
@echo.
@echo ====  Creating New Playarc Files.
@echo on
%Playarc_BIN%\winbalancer.exe %Playarc_DATA% %Playarc_Output%
@echo off

@echo.
@echo.
@echo Playarc Files - Build:

@echo on
%Playarc_BIN%\win.exe %Playarc_Output%\playarc01.ww2 %Playarc_Output%\playarc01.txt
%Playarc_BIN%\win.exe %Playarc_Output%\playarc02.ww2 %Playarc_Output%\playarc02.txt
%Playarc_BIN%\win.exe %Playarc_Output%\playarc03.ww2 %Playarc_Output%\playarc03.txt
%Playarc_BIN%\win.exe %Playarc_Output%\playarc04.ww2 %Playarc_Output%\playarc04.txt
%Playarc_BIN%\win.exe %Playarc_Output%\playarc05.ww2 %Playarc_Output%\playarc05.txt
%Playarc_BIN%\win.exe %Playarc_Output%\playarc06.ww2 %Playarc_Output%\playarc06.txt
%Playarc_BIN%\win.exe %Playarc_Output%\playarc07.ww2 %Playarc_Output%\playarc07.txt
%Playarc_BIN%\win.exe %Playarc_Output%\playarc08.ww2 %Playarc_Output%\playarc08.txt
%Playarc_BIN%\win.exe %Playarc_Output%\playarc09.ww2 %Playarc_Output%\playarc09.txt
%Playarc_BIN%\win.exe %Playarc_Output%\playarc10.ww2 %Playarc_Output%\playarc10.txt
@echo off

@echo.
@echo.
@echo===================================================================================================
@echo.

IF %ERRORLEVEL% NEQ 0 (
	@echo       Game Playarc files Build  ==  Completed - With Errors.
	@echo.
	@echo===================================================================================================
	@echo.
	@echo.
	
	IF /I %DebugBat% == "true" @pause

	EXIT 6
) else (
	@echo       Game Playarc files Build  ==  Completed.
)

@echo.
@echo===================================================================================================
@echo.
@echo.

IF /I %DebugBat% == "true" @pause
