@echo off

:: =================================================================

:: PrepareNewRelease.bat - Command Line:

:: PrepareNewRelease.bat [ /rc | /alpha | /beta ]


:: Default = /rc  (.\release-candidate):

:: Available Options:
:: \rc = .\release-candidate
:: \alpha = .\release-alpha
:: \beta = .\release-beta

:: Launching Directory = The directory where the BAT file is located when it was Launched.
:: NOTE: ALL of the other paths are relative to this directory within the < wwiiolng > directory tree.

:: EXIT Codes:
:: 1 = for some reason the Launching Directory is not set.
:: 2 = Invalaid Command Line Switch.
:: 3 = unable to locate the PrepareNewRelease.bat (this) file. This file should be located in the Launching Directory.
:: 4 = Completed - With Errors.

:: Mathew J Hogan (james10) 10-Apr-2018.

:: =================================================================


@echo.
@echo Preparing the Source Directories for a NEW release. Please wait. . .

rem SET LaunchingDir=%CD% - Has < Run as Admin > issues.
SET LaunchingDir=%~dp0
SET ReleaseVersion=%~1%

IF NOT DEFINED LaunchingDir (
	@echo.
	@echo.
	@echo PrepareNewRelease.bat: Launching Directory is not set - ABORTING.
	EXIT 1
)

IF NOT DEFINED ReleaseVersion (
	@echo.
	@echo.
	@echo PrepareNewRelease.bat: Using /rc command line option.
	SET ReleaseVersion=/rc
)

IF /I "%ReleaseVersion%" == "/RC" (
	SET TargetRootPath=.\release-candidate
	GOTO check1
)

IF /I "%ReleaseVersion%" == "/ALPHA" (
	SET TargetRootPath=.\release-alpha
	GOTO check1
)

IF /I "%ReleaseVersion%" == "/BETA" (
	SET TargetRootPath=.\release-beta
	GOTO check1
)

@echo.
@echo.
@echo ====  PrepareNewRelease.bat: Invalaid Command Line Switch - ABORTING.
EXIT 2

:check1

SET ww2AssetsPath=..\.\ww2assets
SET ww2PlayarcPath=..\.\WW2Archives
SET ww2ExePath=..\.\build\client\Debug\bin
rem SET ww2ExePath=..\.\build\client\Release\bin

@echo.
@echo==========================================================================================================
@echo.
CD /D "%LaunchingDir%"
@echo Bat File Launching Directory  :  %CD%
@echo.

CD /D "%TargetRootPath%"
@echo Target Root Path    ======    :  %CD%
@echo.

@echo Release Version    =======    :  %ReleaseVersion%
@echo.

CD /D "%LaunchingDir%"
CD /D "%ww2AssetsPath%"
@echo WW2 Assets Path    =======    :  %CD%
@echo.

CD /D "%LaunchingDir%"
CD /D "%ww2PlayarcPath%"
@echo WW2 Playarc Path    ======    :  %CD%
@echo.

CD /D "%LaunchingDir%"
CD /D "%ww2ExePath%"
@echo WW2 EXE Path:    =========    :  %CD%
@echo.
@echo==========================================================================================================
@echo.
@echo.

CD /D "%LaunchingDir%"
@echo ====  Current Working Directory: %CD%

rem If PrepareNewRelease.bat file is not here, we are probably in the WRONG directory which can be VERY BAD.
IF /I NOT EXIST "PrepareNewRelease.bat" (
	@echo.
	@echo PrepareNewRelease.bat: file NOT found - ABORTING.
	EXIT 3
) else (
	@echo Located PrepareNewRelease.bat file.
)

@echo.
@echo.
CD /D "%TargetRootPath%\previous"
@echo ====  Deleting the contents of: %CD%
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

DEL *.* /S /Q
RMDIR /S /Q "%CD%"

@echo.
@echo %CD% should now be Empty.
@echo.
@echo.

CD /D "%LaunchingDir%"

CD /D "%TargetRootPath%\diff"
@echo ====  Deleting the contents of: %CD%
@echo.

IF /I NOT EXIST "WW2.exe.patch" (
	IF /I NOT EXIST "Settings.exe.patch" (
		@echo WW2.exe.patch file NOT found, Possible Empty directory.
	) ELSE (
		@echo Located Settings.exe.patch file.
	)
) ELSE (
	@echo Located WW2.exe.patch file.
)

@echo.

DEL *.* /S /Q
RMDIR /S /Q "%CD%"

@echo.
@echo %CD% should now be Empty.
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
XCOPY "%TargetRootPath%\current\*.*" "%TargetRootPath%\previous\" /S /E /V

@echo.
CD /D "%TargetRootPath%\current"
@echo Deleting files from the current directory.

DEL *.* /S /Q
RMDIR /S /Q "%CD%"

@echo.
@echo %CD% should now be Empty.
@echo.
@echo.

rem CD /D "%TargetRootPath%\current"
@echo ====  Creating and populating the DATA directory under %CD%.
MD DATA
MD DATA\screens

CD /D "%LaunchingDir%"

@echo Copying files to the DATA directory.
XCOPY "%ww2assetspath%\oswin\dependencies\*.*" "%TargetRootPath%\current\DATA\" /S /E /V

@echo.
@echo Copying splash screen files to the DATA directory.
XCOPY "%ww2assetspath%\screens\*.*" "%TargetRootPath%\current\DATA\screens\" /S /E /V

@echo.
@echo.
CD /D "%TargetRootPath%\current"
@echo ====  Creating and populating the playarc directory under %CD%.
MD playarc

CD /D "%LaunchingDir%"

@echo Copying files to the playarc directory.
XCOPY "%ww2PlayarcPath%\*.ww2" "%TargetRootPath%\current\playarc\" /V

CD /D "%TargetRootPath%\current"

@echo.
@echo.
@echo ====  Copying the assets-docs directory to %CD%.
MD docs

CD /D "%LaunchingDir%"

XCOPY "%ww2assetspath%\docs\*.*" "%TargetRootPath%\current\docs\" /V 

@echo.
@echo.
@echo ====  Copying the the file Playgate.exe to %CD%.
XCOPY "%ww2ExePath%\Playgate.exe" "%TargetRootPath%\current\" /V

@echo.
@echo.
@echo ====  Copying the the file Settings.exe to %CD%.
XCOPY "%ww2ExePath%\Settings.exe" "%TargetRootPath%\current\" /V

@echo.
@echo.
@echo ====  Copying the the file WW2.exe to %CD%.
XCOPY "%ww2ExePath%\WW2.exe" "%TargetRootPath%\current\" /V

@echo.
@echo.
@echo ====  Copying the files readme.rtf and readme.txt to %TargetRootPath%\current\.
rem XCOPY "%ww2assetspath%\readme.rtf" "%TargetRootPath%\current\" /V 
rem XCOPY "%ww2assetspath%\readme.txt" "%TargetRootPath%\current\" /V
XCOPY "%ww2assetspath%\readme.*" "%TargetRootPath%\current\" /V 

@echo.
@echo.
@echo ====  Copying the the files in the assets-libs directory to %TargetRootPath%\current\.
XCOPY "%ww2assetspath%\oswin\libs\*.*" "%TargetRootPath%\current\" /V

@echo.
@echo.
@echo ====  Copying Steam Files to %TargetRootPath%\current\.
XCOPY "%ww2assetspath%\steam_appid.txt" "%TargetRootPath%\current\" /V
XCOPY "%ww2assetspath%\oswin\steam_api.dll" "%TargetRootPath%\current\" /V

@echo.
@echo.
@echo ====  Copying the the file WWIIOnLine.ico to %TargetRootPath%\current\.
XCOPY "%ww2assetspath%\oswin\WWIIOnLine.ico" "%TargetRootPath%\current\" /V

@echo off

@echo.
@echo.
@echo.
IF %ERRORLEVEL% NEQ 0 (
	@echo ====  Completed - With Errors.
	@echo Further investigation will be required.
	EXIT 4
) else (
	@echo ====  Completed.
	@echo You can now run the appropriate MakeDiffs.bat to populate the diff directory for use by the patch installer script.
	@echo.
	@echo The %TargetRootPath% directory should be sufficiently populated for the installer script to be run.
)
@echo.

rem @pause