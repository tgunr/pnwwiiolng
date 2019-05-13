@echo off

:: =====================================================================================================

:: _buildMDLfiles.bat - Command Line:

:: _buildMDLfiles [ /args "command line args"]

:: Default = /args  "-BuildMDLOnly -window 1 -width 1024 -height 768 -data 1 -datadir %launchingdir%\DATA":

:: EXIT Codes:
:: 1 = for some reason the Launching Directory is not set.
:: 2 = unable to locate the _buildMDLfiles.bat (this) file. This file should be located in the Launching Directory.
:: 3 = Completed - With Errors.

:: Mathew J Hogan (james10) 8-Apr-2018, 3-May-2019.

:: Disable / Enable = DebugBat
SET DebugBat=false
rem SET DebugBat=true

:: =====================================================================================================

echo.
echo.
echo===================================================================================================
echo.
echo       Launching ww2.exe (Debug) to build the .mdl files. Please wait. . .
echo.
echo===================================================================================================

rem SET LaunchingDir=%CD% - Has < Run as Admin > issues at first lunch.
SET launchingdir=%~dp0

rem ReSET the launching Directory.
CD /D "%LaunchingDir%"
SET launchingdir=%CD%

IF NOT DEFINED LaunchingDir (
	echo.
	echo.
	echo _buildMDLfiles.bat: Launching Directory is not set - ABORTING.
	IF /I %DebugBat% == true @pause

	EXIT /B 1
)

CD /D "%LaunchingDir%"
echo Bat File Launching Directory  :  %CD%
echo.

SET defaulrCommandLineArg="-BuildMDLOnly -window 1 -width 1024 -height 768 -data 1 -datadir %launchingdir%\DATA"

rem If _buildMDLfiles file is not here, we are probably in the WRONG directory which can be VERY BAD.
IF /I NOT EXIST "_buildMDLfiles.bat" (
	echo.
	echo.
	echo _buildMDLfiles.bat: file NOT found - ABORTING.
	IF /I %DebugBat% == true @pause

	EXIT /B 2
) ELSE (
	echo Located _buildMDLfiles.bat file.
	echo.
)

SET argsSettingArgs=%~1%

IF /I "%argsSettingArgs%" == "/ARGS" (
	SET argsSetting="%~2%"

) ELSE (
	SET argsSetting=
)

Set _tempvar=false
If [%argsSetting%]==[] (
	Set _tempvar=true
)

If [%argsSetting%]==[""] (
	Set _tempvar=true
)

IF %_tempvar% == true (
	echo _buildMDLfiles.bat: Using the Default /args setting.
	echo.
	SET argsSetting=%defaulrCommandLineArg%
)

IF NOT DEFINED argsSetting (
	echo _buildMDLfiles.bat: Using the Default /args setting.
	echo.
	SET argsSetting=%defaulrCommandLineArg%
)

::  The " characters are required to pass the ww2 command line options as a single arg and not broken up into several.
::  Remove the " character from the args line as ww2 does not expect them to be included.
SET argsSetting=%argsSetting:"=%

echo.
echo.
echo==========================================================================================================
echo.
CD /D "%LaunchingDir%"
echo Bat File Launching Directory  ==  :  %CD%
echo.

echo Command Line Args  =============  :  %argsSetting%
echo.
echo==========================================================================================================

echo.
echo.

IF /I %DebugBat% == true @pause

echo on
ww2 %argsSetting%
@echo off

echo.
echo.
echo==========================================================================================================
echo.

IF %ERRORLEVEL% NEQ 0 (
	echo       Game .mdl files Build  ==  Completed - With Errors.
	echo.
	echo==========================================================================================================
	echo.
	echo.
	
	IF /I %DebugBat% == true @pause

	EXIT 3
) ELSE (
	echo       Game .mdl files Build  ==  Completed.
)

echo.
echo==========================================================================================================
echo.
echo.

IF /I %DebugBat% == true @pause
echo on

