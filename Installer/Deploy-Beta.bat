@echo off

:: =====================================================================================================

:: Deploy-Beta.bat - Command Line:

:: Deploy-Beta.bat [ /rc | /alpha | /beta | /rcdb | /alphadb | /betadb ]


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
:: 2 = unable to locate the Deploy-Beta.bat (this) file. This file should be located in the Launching Directory.
:: 3 = unable to locate the wwiiolng Root Directory.
:: 4 = unable to locate the ww2mods Root Directory.
:: 5 = unable to locate the iscc.exe Inno Command Line Compiler.
:: 6 = unable to locate the WWIIOL_Installer_Template.iss Inno Script file.
:: 7 = Invalaid Command Line Switch.
:: 8 = Completed - With Errors.

:: Mathew J Hogan (james10) 8-Apr-2018, 3-May-2019.

:: Disable / Enable = DebugBat
SET DebugBat="false"
rem SET DebugBat="true"

:: =====================================================================================================

@echo.
@echo.
@echo===================================================================================================
@echo.
@echo       Deploying a Beta Release. Please wait. . .
@echo.
@echo===================================================================================================

rem SET LaunchingDir=%CD% - Has < Run as Admin > issues.
SET launchingdir=%~dp0

IF NOT DEFINED LaunchingDir (
	@echo.
	@echo.
	@echo Deploy-Beta.bat: Launching Directory is not set - ABORTING.
	IF /I %DebugBat% == "true" @pause

	EXIT 1
)

CD /D "%LaunchingDir%"
@echo Bat File Launching Directory  :  %CD%
@echo.


rem If Deploy-Beta.bat file is not here, we are probably in the WRONG directory which can be VERY BAD.
IF /I NOT EXIST "Deploy-Beta.bat" (
	@echo.
	@echo.
	@echo Deploy-Beta.bat: file NOT found - ABORTING.
	IF /I %DebugBat% == "true" @pause

	EXIT 2
) else (
	@echo Located Deploy-Beta.bat file.
	@echo.
)


SET ReleaseVersion=%~1%

IF NOT DEFINED ReleaseVersion (
	@echo Deploy-Beta.bat: Using /beta command line option.
	@echo.
	SET ReleaseVersion=/beta
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

SET ww2AssetsPath=%wwiiolngRootDIR%\ww2assets


rem Setup for Inno Setup.
SET InnoAppPath=C:\Program Files (x86)\Inno Setup 5
SET InnoAppEXE=iscc.exe
SET InnoScriptPath=%wwiiolngRootDIR%\Installer
SET InnoScriptName=WWIIOL_Installer_Template.iss

rem iscc.exe is REQUIRED for this process.
IF /I NOT EXIST "%InnoAppPath%"\"%InnoAppEXE%" (
	@echo.
	@echo.
	@echo.
	@echo iscc.exe Inno Command Line Compiler: file NOT found - ABORTING.
	IF /I %DebugBat% == "true" @pause

	EXIT 5
) else (
	@echo Located iscc.exe Inno Command Line Compiler.
)

rem WWIIOL_Installer_Template.iss is REQUIRED for this process.
IF /I NOT EXIST "%InnoScriptPath%"\"%InnoScriptName%" (
	@echo.
	@echo.
	@echo.
	@echo WWIIOL_Installer_Template.iss Inno Script: file NOT found - ABORTING.
	IF /I %DebugBat% == "true" @pause

	EXIT 6
) else (
	@echo Located WWIIOL_Installer_Template.iss Inno Script.
)


rem Release EXE versions.
IF /I "%ReleaseVersion%" == "/RC" (
	GOTO check1
)

IF /I "%ReleaseVersion%" == "/ALPHA" (
	GOTO check1
)

IF /I "%ReleaseVersion%" == "/BETA" (
	GOTO check1
)

rem Debug EXE versions.
IF /I "%ReleaseVersion%" == "/RCDB" (
	GOTO check1
)

IF /I "%ReleaseVersion%" == "/ALPHADB" (
	GOTO check1
)

IF /I "%ReleaseVersion%" == "/BETADB" (
	GOTO check1
)

@echo.
@echo.
@echo.
@echo ====  Deploy-Beta.bat: Invalaid Command Line Switch - ABORTING.
IF /I %DebugBat% == "true" @pause

EXIT 7

:check1

@echo.
CD /D "%LaunchingDir%"
@echo Bat File Launching Directory  :  %CD%

CD /D "%InnoAppPath%"
@echo Application Path              :  %CD%
@echo Application File Name         :  %InnoAppEXE%

CD /D "%LaunchingDir%"

CD /D "%InnoScriptPath%"
@echo Script Path                   :  %CD%
@echo Script File Name              :  %InnoScriptName%

CD /D "%LaunchingDir%"

echo.
echo.

@echo on
CALL _PlayArchBuilder.bat /beta
@echo off

CD /D "%LaunchingDir%"
@echo.
@echo.

@echo on
CALL _PrepareNewRelease.bat /beta
@echo off

CD /D "%LaunchingDir%"
@echo.
@echo.

rem @echo on
rem CALL _MakeDiffs.bat /beta
rem @echo off

@echo Creating the Beta Release, Full Install.
@echo.

@echo on
"%InnoAppPath%\%InnoAppEXE%" "/dm_ReleaseType=beta" "/dm_BuildType=full" "%InnoScriptPath%\%InnoScriptName%"
@echo off

echo.
echo.
 
rem pause

rem @echo Creating the Beta Release, Patch Install.
rem @echo on
rem "%InnoAppPath%\%InnoAppEXE%" "/dm_ReleaseType=beta" "/dm_BuildType=patch" "%InnoScriptPath%\%InnoScriptName%"
rem @echo off

rem echo.
rem echo.

rem pause
