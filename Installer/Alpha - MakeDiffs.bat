@echo off
@echo.

rem @echo ====  Launching:  _MakeDiffs.bat  (Release Candidate).

@echo ====  Launching:  _MakeDiffs.bat  (Alpha Release).

rem @echo ====  Launching:  _MakeDiffs.bat  (Beta Release).

@echo.
@echo.

rem SET launchingdir=%CD% - Has < Run as Admin > issues.
SET launchingdir=%~dp0

IF NOT DEFINED LaunchingDir (
	@echo.
	@echo.
	@echo _MakeDiffs.bat: Launching Directory is not set - ABORTING.
	
	EXIT 1
)

CD /D "%LaunchingDir%"

@echo Bat File Launching Directory  :  %CD%
@echo.

rem If _MakeDiffs.bat file is not here, we are probably in the WRONG directory which can be VERY BAD.
IF /I NOT EXIST "_MakeDiffs.bat" (
	@echo.
	@echo _MakeDiffs.bat file NOT found - ABORTING.
	
	EXIT 2
) else (
	@echo Located MakeDiffs.bat file.
)

@echo.
@echo.

rem CALL _MakeDiffs.bat /rc

CALL _MakeDiffs.bat /alpha

rem CALL _MakeDiffs.bat /beta

@echo.
@echo.

rem @pause
