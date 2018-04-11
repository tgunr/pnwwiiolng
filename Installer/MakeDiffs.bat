@echo off

:: =================================================================

:: MakeDiffs.bat - Command Line:

:: PreviousFilesDirectory  CurrentFilesDirectory  OutputDirectory

:: The order of appearence on the Command Line <<< IS CRITICAL >>>

:: Defaults = release candidate:
:: Launching Directory = The directory where the BAT file is located when it was Launched.
:: NOTE: ALL of the other default paths are relative to this directory.

:: PreviousFilesDirectory = .\release-candidate\previous
:: CurrentFilesDirectory = .\release-candidate\current
:: OutputDirectory = .\release-candidate\diff

:: ww2assetspath = Path to the Assets repository. ( ..\.\ww2assets )

:: Mathew J Hogan (james10) 8-Apr-2018.

:: =================================================================


@echo.
@echo Creating Differential Files. Please wait. . .

rem SET launchingdir=%CD% - Has < Run as Admin > issues.
SET launchingdir=%~dp0
SET previouspath=%~1%
SET currentpath=%~2%
SET outputpath=%~3%

IF NOT DEFINED launchingdir (EXIT 1)
IF NOT DEFINED previouspath (SET previouspath=.\release-candidate\previous)
IF NOT DEFINED currentpath (SET currentpath=.\release-candidate\current)
IF NOT DEFINED outputpath (SET outputpath=.\release-candidate\diff)

SET ww2assetspath=..\.\ww2assets

@echo.
@echo==========================================================================================================
@echo.
CD /D "%LaunchingDir%"
@echo Bat File Launching Directory  :  %CD%
@echo.

CD /D "%previouspath%"
@echo Previous Path    =========    :  %CD%
@echo.

CD /D "%LaunchingDir%"
CD /D "%currentpath%"
@echo Current Path    ==========    :  %CD%
@echo.

CD /D "%LaunchingDir%"
CD /D "%outputpath%"
@echo Output Path    ===========    :  %CD%
@echo.

CD /D "%LaunchingDir%"
CD /D "%ww2assetspath%"
@echo WW2 Assets Path    =======    :  %CD%
@echo.
@echo==========================================================================================================

@echo.
@echo.

CD /D %launchingdir%
@echo Current Working Directory: %CD%

rem If MakeDiffs.bat file is not here, we are probably in the WRONG directory which can be VERY BAD.
IF /I NOT EXIST "MakeDiffs.bat" (
	@echo.
	@echo MakeDiffs.bat file NOT found - ABORTING.
	EXIT 2
) else (
	@echo Located MakeDiffs.bat file.
)

@echo.
@echo.

CD %outputpath%
@echo Current Working Directory: %CD%
@echo.
@echo.

@echo Deleting ALL .patch files from %outputpath%.
DEL /Q *.patch

@echo.
@echo Done. . .
@echo.
@echo.

CD /D %launchingdir%
@echo Current Working Directory: %CD%

@echo.
@echo.
@echo.
@echo on
@echo Processing Requested Files.
@echo.

@echo ====  Playarc files:
%ww2assetspath%\xdelta3 -e -f -9 -s "%previouspath%\playarc\playarc01.ww2" "%currentpath%\playarc\playarc01.ww2" "%outputpath%\playarc01.ww2.patch"
%ww2assetspath%\xdelta3 -e -f -9 -s "%previouspath%\playarc\playarc02.ww2" "%currentpath%\playarc\playarc02.ww2" "%outputpath%\playarc02.ww2.patch"
%ww2assetspath%\xdelta3 -e -f -9 -s "%previouspath%\playarc\playarc03.ww2" "%currentpath%\playarc\playarc03.ww2" "%outputpath%\playarc03.ww2.patch"
%ww2assetspath%\xdelta3 -e -f -9 -s "%previouspath%\playarc\playarc04.ww2" "%currentpath%\playarc\playarc04.ww2" "%outputpath%\playarc04.ww2.patch"
%ww2assetspath%\xdelta3 -e -f -9 -s "%previouspath%\playarc\playarc05.ww2" "%currentpath%\playarc\playarc05.ww2" "%outputpath%\playarc05.ww2.patch"
%ww2assetspath%\xdelta3 -e -f -9 -s "%previouspath%\playarc\playarc06.ww2" "%currentpath%\playarc\playarc06.ww2" "%outputpath%\playarc06.ww2.patch"
%ww2assetspath%\xdelta3 -e -f -9 -s "%previouspath%\playarc\playarc07.ww2" "%currentpath%\playarc\playarc07.ww2" "%outputpath%\playarc07.ww2.patch"
%ww2assetspath%\xdelta3 -e -f -9 -s "%previouspath%\playarc\playarc08.ww2" "%currentpath%\playarc\playarc08.ww2" "%outputpath%\playarc08.ww2.patch"
%ww2assetspath%\xdelta3 -e -f -9 -s "%previouspath%\playarc\playarc09.ww2" "%currentpath%\playarc\playarc09.ww2" "%outputpath%\playarc09.ww2.patch"
%ww2assetspath%\xdelta3 -e -f -9 -s "%previouspath%\playarc\playarc10.ww2" "%currentpath%\playarc\playarc10.ww2" "%outputpath%\playarc10.ww2.patch"

@echo.
@echo.
@echo ====  Exe Files.
%ww2assetspath%\xdelta3 -e -f -9 -s "%previouspath%\WW2.exe"       "%currentpath%\WW2.exe"       "%outputpath%\WW2.exe.patch"
%ww2assetspath%\xdelta3 -e -f -9 -s "%previouspath%\Playgate.exe"  "%currentpath%\Playgate.exe"  "%outputpath%\Playgate.exe.patch"
%ww2assetspath%\xdelta3 -e -f -9 -s "%previouspath%\Settings.exe"  "%currentpath%\Settings.exe"  "%outputpath%\Settings.exe.patch"

@echo.
@echo.
@echo ====  Document Fles.
%ww2assetspath%\xdelta3 -e -f -9 -s "%previouspath%\docs\license.rtf"   "%currentpath%\docs\license.rtf"   "%outputpath%\license.rtf.patch"
%ww2assetspath%\xdelta3 -e -f -9 -s "%previouspath%\readme.rtf"    "%currentpath%\readme.rtf"    "%outputpath%\readme.rtf.patch"

@echo.
@echo.
@echo ====  Additional Files.
%ww2assetspath%\xdelta3 -e -f -9 -s "%previouspath%\DATA\playgate\bg-tos.bmp"    "%currentpath%\DATA\playgate\bg-tos.bmp"    "%outputpath%\bg-tos.bmp.patch"

@echo off

@echo.
@echo.
@echo.
IF %ERRORLEVEL% NEQ 0 (
	@echo ====  Completed - With Errors.
) else (
	@echo ====  Completed.
)
@echo.

@pause