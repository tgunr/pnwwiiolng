@ECHO ON
SETLOCAL

REM This is just utterly ridiculous.
REM Please make it go away.
REM    - Gadget

REM Pull in the VS macros
SET VSInstallDir=%~1
SET LOCALAPPDATA=%~2
SET ProgramW6432=%~3
SET ProgramFiles=%~4
SET HOMEDRIVE=%~5
SET SolutionDir=%~6
SET ProjectDir=%~7

REM Give PATH first dibs
FOR %%p IN (python.exe) DO (SET Python=%%~$PATH:p)
IF NOT DEFINED Python (
	REM Look for Python in all the default locations in descending order of version,
	REM prefer new 64-bit to new 32-bit but new 32-bit to old 64-bit.

	REM VS drops 3.5, 3.6, or 3.7 inside its install dir
	REM Official 3.5, 3.6, 3.7 default to AppData, Program Files x86, or Program Files
	REM Official 3.0, 3.1, 3.2, 3.3, 3.4 default to C:
	REM VS places 64-bit 2.7 in C: with a special name, 32-bit always goes to C:

	REM *****
	REM At the moment we're requiring Python 2 specifically due to a minor incompatibility
	REM with 3 caused by the script being lazy. This will be fixed soon.
	REM Gotta love how REM isn't really a comment so you can't put it inside the FOR...
	REM *****
	FOR %%p IN (
		"FAIL%VSInstallDir%..\..\Shared\Python37_64" "FAIL%VSInstallDir%..\..\Shared\Python37_86"
		"FAIL%VSInstallDir%..\..\Shared\Python36_64" "FAIL%VSInstallDir%..\..\Shared\Python36_86"
		"FAIL%VSInstallDir%..\..\Shared\Python35_64" "FAIL%VSInstallDir%..\..\Shared\Python35_86"
	
		"FAIL%LOCALAPPDATA%\Programs\Python\Python37"		"FAIL%LOCALAPPDATA%\Programs\Python\Python37-32"
		"FAIL%ProgramW6432%\Python37"						"FAIL%ProgramFiles%\Python37-32"
		"FAIL%LOCALAPPDATA%\Programs\Python\Python36"		"FAIL%LOCALAPPDATA%\Programs\Python\Python36-32"
		"FAIL%ProgramW6432%\Python36"						"FAIL%ProgramFiles%\Python36-32"
		"FAIL%LOCALAPPDATA%\Programs\Python\Python35"		"FAIL%LOCALAPPDATA%\Programs\Python\Python35-32"
		"FAIL%ProgramW6432%\Python35"						"FAIL%ProgramFiles%\Python35-32"
	
		"FAIL%HOMEDRIVE%\Python34"		"FAIL%HOMEDRIVE%\Python33"
		"FAIL%HOMEDRIVE%\Python32"		"FAIL%HOMEDRIVE%\Python31"
		"FAIL%HOMEDRIVE%\Python30"
	
		"%HOMEDRIVE%\Python27amd64"	"%HOMEDRIVE%\Python27"
	) DO (
		IF NOT DEFINED Python (
			IF EXIST %%~p\python.exe (SET Python=%%~p\python.exe)
		)
	)
)

IF NOT DEFINED Python (
	ECHO "BUILD_INFO.BAT: error ENOENT2: Unable to locate a Python interpreter. Please check your system %%PATH%%"
	EXIT /B
)

"%Python%" "%SolutionDir%WW2\src\build_info.py" client %ProjectDir%

ENDLOCAL
