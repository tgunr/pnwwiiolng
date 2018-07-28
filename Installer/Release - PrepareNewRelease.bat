@echo off
@echo.
@echo ====  Launching:  PrepareNewRelease.bat  (release-candidate).
@echo.
@echo.

SET LaunchingDir=%~dp0

IF NOT DEFINED LaunchingDir (
	@echo.
	@echo.
	@echo Release - PrepareNewRelease.bat: Launching Directory is not set - ABORTING.
	EXIT 1
)

CD /D "%LaunchingDir%"

@echo ====  Current Working Directory: %CD%

rem If PrepareNewRelease.bat file is not here, we are probably in the WRONG directory which can be VERY BAD.
IF /I NOT EXIST "PrepareNewRelease.bat" (
	@echo.
	@echo PrepareNewRelease.bat: file NOT found - ABORTING.
	EXIT 2
) else (
	@echo Located PrepareNewRelease.bat file.
)

@echo.
@echo.
@echo.

CALL PrepareNewRelease.bat /rc

@echo.