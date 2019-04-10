@echo off
@echo.

@echo ====  Launching:  _PrepareNewRelease.bat  (Release Candidate).

rem @echo ====  Launching:  _PrepareNewRelease.bat  (Alpha Release).

rem @echo ====  Launching:  _PrepareNewRelease.bat  (Beta Release).

@echo.
@echo.

rem SET launchingdir=%CD% - Has < Run as Admin > issues.
SET LaunchingDir=%~dp0

IF NOT DEFINED LaunchingDir (
	@echo.
	@echo.
	@echo _PrepareNewRelease.bat: Launching Directory is not set - ABORTING.
	
	EXIT 1
)

CD /D "%LaunchingDir%"

@echo Bat File Launching Directory  :  %CD%
@echo.

rem If _PrepareNewRelease.bat file is not here, we are probably in the WRONG directory which can be VERY BAD.
IF /I NOT EXIST "_PrepareNewRelease.bat" (
	@echo.
	@echo _PrepareNewRelease.bat: file NOT found - ABORTING.

	EXIT 2
) else (
	@echo Located _PrepareNewRelease.bat file.
)

@echo.
@echo.

CALL _PrepareNewRelease.bat /rc

rem CALL _PrepareNewRelease.bat /alpha

rem CALL _PrepareNewRelease.bat /beta

@echo.
@echo.

rem @pause
