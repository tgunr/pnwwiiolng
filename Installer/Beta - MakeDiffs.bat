@echo off

rem SET launchingdir=%CD% - Has < Run as Admin > issues.
SET launchingdir=%~dp0

@echo.
CD /D "%LaunchingDir%"
@echo Bat File Launching Directory  :  %CD%
@echo.

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
@echo Launching - MakeDiffs.bat - Beta Release. . .
@echo.
@echo.
@echo.

CALL MakeDiffs.bat .\release-beta\previous .\release-beta\current .\release-beta\diff

rem pause