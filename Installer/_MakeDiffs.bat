@echo off

:: =====================================================================================================

:: _MakeDiffs.bat - Command Line:

:: _MakeDiffs.bat  [ /rc | /alpha | /beta ]

:: Defaults = /rc (release candidate)
:: PreviousFilesDirectory = .\release-candidate\previous
:: CurrentFilesDirectory = .\release-candidate\current
:: OutputDirectory = .\release-candidate\diff

:: xdelta3path = Path to the xdelta3. ( ..\ww2assets\oswin )

:: Launching Directory = The directory where the BAT file is located when it was Launched.
:: NOTE: ALL of the other paths are relative to this directory within the < wwiiolng > directory tree.

:: EXIT Codes:
:: 1 = for some reason the Launching Directory is not set.
:: 2 = unable to locate the _MakeDiffs.bat (this) file. This file should be located in the Launching Directory.
:: 3 = unable to locate the wwiiolng Root Directory.
:: 4 = unable to locate the ww2mods Root Directory.
:: 5 = Invalaid Command Line Switch.
:: 6 = unable to locate the xdelta3.exe file. This file is REQUIRED.
:: 7 = Completed - With Errors.

:: Mathew J Hogan (james10) 8-Apr-2018, 25-June-2019.

:: Disable / Enable = DebugBat.
SET DebugBat="false"
rem SET DebugBat="true"

:: =====================================================================================================


@echo.
@echo.
@echo===================================================================================================
@echo.
@echo Creating Differential Files. Please wait. . .
@echo.
@echo===================================================================================================
@echo.
@echo.



rem SET launchingdir=%CD% - Has < Run as Admin > issues.
SET launchingdir=%~dp0

IF NOT DEFINED LaunchingDir (
	@echo.
	@echo.
	@echo.
	@echo _MakeDiffs.bat: Launching Directory is not set - ABORTING.
	IF /I %DebugBat% == "true" @pause

	EXIT /B 1
)

CD /D "%LaunchingDir%"
@echo Bat File Launching Directory  :  %CD%

rem If _MakeDiffs.bat file is not here, we are probably in the WRONG directory which can be VERY BAD.
IF /I NOT EXIST "_MakeDiffs.bat" (
	@echo.
	@echo.
	@echo.
	@echo _MakeDiffs.bat: file NOT found - ABORTING.
	IF /I %DebugBat% == "true" @pause

	EXIT /B 2
) else (
	@echo.
	@echo Located _MakeDiffs.bat file.
	@echo.
)

SET ReleaseVersion=%~1%

IF NOT DEFINED ReleaseVersion (
	@echo _MakeDiffs.bat: Using /rc command line option.
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

	EXIT /B 3
) else (
	@echo Located the wwiiolng Directory.
	SET wwiiolngRootDIR=%RepoRootDIR%\wwiiolng
)

rem Locate and Set the ww2mods Root Directory.
IF /I NOT EXIST "ww2mods\" (
	@echo.
	@echo ww2mods: directory NOT found - ABORTING.
	IF /I %DebugBat% == "true" @pause

	EXIT /B 4
) else (
	@echo Located the ww2mods Directory.
	SET modsRootDIR=%RepoRootDIR%\ww2mods
)

CD /D "%LaunchingDir%"

rem Release EXE versions.
IF /I "%ReleaseVersion%" == "/RC" (
	SET previouspath=%LaunchingDir%\release-candidate\previous
	SET currentpath=%LaunchingDir%\release-candidate\current
	SET outputpath=%LaunchingDir%\release-candidate\diff
	GOTO check1
)

IF /I "%ReleaseVersion%" == "/ALPHA" (
	SET previouspath=%LaunchingDir%\release-alpha\previous
	SET currentpath=%LaunchingDir%\release-alpha\current
	SET outputpath=%LaunchingDir%\release-alpha\diff
	GOTO check1
)

IF /I "%ReleaseVersion%" == "/BETA" (
	SET previouspath=%LaunchingDir%\release-beta\previous
	SET currentpath=%LaunchingDir%\release-beta\current
	SET outputpath=%LaunchingDir%\release-beta\diff
	GOTO check1
)

@echo.
@echo.
@echo.
@echo ====  _MakeDiffs.bat: Invalaid Command Line Switch - ABORTING.
IF /I %DebugBat% == "true" @pause

EXIT /B 5

:check1

SET xdelta3path=%wwiiolngRootDIR%\Installer\ww2assets\oswin
CD /D "%xdelta3path%"

rem xdelta3.exe is REQUIRED for this process.
IF /I NOT EXIST "xdelta3.exe" (
	@echo.
	@echo.
	@echo.
	@echo xdelta3.exe: file NOT found - ABORTING.
	IF /I %DebugBat% == "true" @pause

	EXIT /B 6

) else (
	@echo Located xdelta3.exe file.
)

CD /D "%LaunchingDir%"

@echo.
@echo.
@echo==========================================================================================================
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
CD /D "%previouspath%"
@echo Previous Path  =================  :  %CD%
@echo.

CD /D "%LaunchingDir%"
CD /D "%currentpath%"
@echo Current Path  ==================  :  %CD%
@echo.

CD /D "%LaunchingDir%"
CD /D "%outputpath%"
@echo Output (diff) Path  ============  :  %CD%
@echo.

CD /D "%LaunchingDir%"
CD /D "%xdelta3path%"
@echo xdelta3.exe Path  ==============  :  %CD%
@echo.
@echo==========================================================================================================

@echo.
@echo.

IF /I %DebugBat% == "true" @pause

CD /D "%LaunchingDir%"
CD /D "%outputpath%"
@echo File Output Directory: %CD%
@echo.
@echo.

@echo Deleting ALL .patch files from %CD%
@echo.
DEL /Q /S *.patch

@echo.
@echo Done. . .
@echo.
@echo.

CD /D "%launchingdir%"
@echo Current Working Directory: %CD%

@echo.
@echo.
@echo.
@echo Processing Requested Files.
@echo.

@echo ====  Playarc files:
@echo on
%xdelta3path%\xdelta3.exe -e -f -9 -s "%previouspath%\playarc\playarc01.ww2" "%currentpath%\playarc\playarc01.ww2" "%outputpath%\playarc01.ww2.patch"
%xdelta3path%\xdelta3.exe -e -f -9 -s "%previouspath%\playarc\playarc02.ww2" "%currentpath%\playarc\playarc02.ww2" "%outputpath%\playarc02.ww2.patch"
%xdelta3path%\xdelta3.exe -e -f -9 -s "%previouspath%\playarc\playarc03.ww2" "%currentpath%\playarc\playarc03.ww2" "%outputpath%\playarc03.ww2.patch"
%xdelta3path%\xdelta3.exe -e -f -9 -s "%previouspath%\playarc\playarc04.ww2" "%currentpath%\playarc\playarc04.ww2" "%outputpath%\playarc04.ww2.patch"
%xdelta3path%\xdelta3.exe -e -f -9 -s "%previouspath%\playarc\playarc05.ww2" "%currentpath%\playarc\playarc05.ww2" "%outputpath%\playarc05.ww2.patch"
%xdelta3path%\xdelta3.exe -e -f -9 -s "%previouspath%\playarc\playarc06.ww2" "%currentpath%\playarc\playarc06.ww2" "%outputpath%\playarc06.ww2.patch"
%xdelta3path%\xdelta3.exe -e -f -9 -s "%previouspath%\playarc\playarc07.ww2" "%currentpath%\playarc\playarc07.ww2" "%outputpath%\playarc07.ww2.patch"
%xdelta3path%\xdelta3.exe -e -f -9 -s "%previouspath%\playarc\playarc08.ww2" "%currentpath%\playarc\playarc08.ww2" "%outputpath%\playarc08.ww2.patch"
%xdelta3path%\xdelta3.exe -e -f -9 -s "%previouspath%\playarc\playarc09.ww2" "%currentpath%\playarc\playarc09.ww2" "%outputpath%\playarc09.ww2.patch"
%xdelta3path%\xdelta3.exe -e -f -9 -s "%previouspath%\playarc\playarc10.ww2" "%currentpath%\playarc\playarc10.ww2" "%outputpath%\playarc10.ww2.patch"

@echo off
@echo.
@echo.
@echo ====  EXE Files.
@echo on
%xdelta3path%\xdelta3.exe -e -f -9 -s "%previouspath%\WW2.exe"       "%currentpath%\WW2.exe"       "%outputpath%\WW2.exe.patch"
%xdelta3path%\xdelta3.exe -e -f -9 -s "%previouspath%\Playgate.exe"  "%currentpath%\Playgate.exe"  "%outputpath%\Playgate.exe.patch"
%xdelta3path%\xdelta3.exe -e -f -9 -s "%previouspath%\Settings.exe"  "%currentpath%\Settings.exe"  "%outputpath%\Settings.exe.patch"

@echo off
@echo.
@echo.
@echo ====  Document Fles.
@echo on
%xdelta3path%\xdelta3.exe -e -f -9 -s "%previouspath%\docs\license.rtf"  "%currentpath%\docs\license.rtf"  "%outputpath%\license.rtf.patch"
%xdelta3path%\xdelta3.exe -e -f -9 -s "%previouspath%\readme.rtf"        "%currentpath%\readme.rtf"        "%outputpath%\readme.rtf.patch"
%xdelta3path%\xdelta3.exe -e -f -9 -s "%previouspath%\readme.txt"        "%currentpath%\readme.txt"        "%outputpath%\readme.txt.patch"

@echo off
@echo.
@echo.
@echo ====  Additional Files.
@echo       No Additional Files.

rem Template for additional files.
rem @echo on
rem %xdelta3path%\xdelta3.exe -e -f -9 -s "%previouspath%\data\playgate\bg-tos.bmp"  "%currentpath%\data\playgate\bg-tos.bmp"  "%outputpath%\bg-tos.bmp.patch"

@echo off

@echo.
@echo.
@echo===================================================================================================
@echo.
IF %ERRORLEVEL% NEQ 0 (
	@echo       Create - Differential [Patching] files  ==  Completed - With Errors.
	@echo.
	@echo===================================================================================================
	@echo.
	@echo.

	IF /I %DebugBat% == "true" @pause

	EXIT /B 7
) else (
	@echo       Create - Differential [Patching] files  ==  Completed.
)
@echo.
@echo===================================================================================================
@echo.
@echo.

IF /I %DebugBat% == "true" @pause
