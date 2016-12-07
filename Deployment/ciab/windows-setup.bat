@echo off

echo WWIIOnline Cluster-in-a-Box Setup Script for Windows.
echo.
echo Checks for Python and MySQL installs,
echo For configuring the MySQL setup, please see README-MySQL-Win.txt.
echo.

echo Checking for Python:
python --version >NUL 2>NUL
IF %errorlevel% == 0 (
    echo Great! Python is already installed.
) ELSE (
    echo.
    echo ========== Attention ==========
    echo.
    echo 'Python' needs to be installed, hit enter to launch the installer.
    echo Re-run this script after the install is complete.
    echo.
    pause

    Installers\python-3.5.2-amd64-webinstall.exe /passive SimpleInstall=1 ^
     PrependPath=1 InstallAllUsers=1 InstallLauncherAllUsers=1 ^
     AssociateFiles=1 Shortcuts=1 ^
     Include_doc=1 Include_exe=1 Include_launcher=1 Include_lib=1 Include_pip=1 ^
     Include_tcltk=1 Include_tools=1 ^
     Include_debug=0 Include_dev=0 Include_symbols=0 Include_test=0 CompileAll=0
    GOTO FINISH
)

echo.
echo Checking Python is up-to-date:
python -m pip install --upgrade pip
pip install --upgrade setuptools

echo.
echo Checking for MySQL:
python -m lib.findmysql
IF %errorlevel% == 0 (
    echo Great! MySQL is already installed.
) ELSE (
    echo.
    echo ========== Attention ==========
    echo.
    echo 'MySQL' needs to be installed, hit enter to launch the installer.
    echo Re-run this script after the install is complete.
    echo NOTE: This tooling was written around MySQL 5.7, if you are installing
    echo a different version or using a non-standard configuration and the tools
    echo cannot find your MySQL, please talk to the dev team!
    echo.
    pause

    Installers\mysql-installer-web-community-5.7.16.0.msi
    TIMEOUT 5 /NOBREAK >NUL
    start notepad README-MySQL-Win.txt
    GOTO FINISH
)

echo.
IF NOT EXIST C:\playnet (
    echo C:\playnet already exists, making sure it's populated correctly...
) ELSE (
    echo Creating C:\playnet directories.
)
call make-win-playnet.bat

:CheckData
set data_dst=C:\Playnet\wwiiol\data
set test_file=blitzWor.csv
echo.
echo Checking for blitzWor:
IF EXIST "%data_dst%\%test_file%" (
    echo Great - you appear to have %data_dst% ready to go.
    GOTO CopyFiles
)
echo . Not found: %data_dst%\%test_file%
IF EXIST WW2Mods\DATA\%test_file% (
    set data_src=WW2Mods\DATA
    GOTO SetupDataDir
)
echo . Not FOund: WW2Mods\Data\%test_file%
IF EXIST DATA\%test_file% (
    set data_src=DATA
    GOTO SetupDataDir
)

echo.
echo ========== PROBLEM ==========
echo.
echo ERROR: Could not find a DATA directory.
echo.

:GetDataDir
set /P data_src="Enter full path to DATA directory: "
if EXIST "%data_src%\%test_file%" GOTO DecideCopyMoveOrLink
if NOT EXIST "%data_src%\DATA\%test_file%" (
    echo ERROR: Could not find %data_src%\%test_file%.
    GOTO GetDataDir
)
set data_src=%data_src%\DATA

:DecideCopyMoveOrLink
@rem check if 'mklink' is available
mkdir t1
mklink /J t2 t1 >NUL 2>NUL
set mklink_result=%errorlevel%
rmdir /S /Q t1 t2

echo.
echo Found blitzWor.csv in %data_src%.
echo.
echo ========== Attention ==========
echo.
echo Please choose whether to COPY, MOVE or LINK %data_src% to %data_dst%.
echo 'COPY' will take a while and use more disk space,
echo 'MOVE' is very quick,
if %mklink_result% == 0 (
    echo 'LINK' is cheap but breaks if the source directory is deleted/moved.
    set op_prompt=COPY, MOVE or LINK
) ELSE (
    echo 'LINK' is not available on your system.
    set op_prompt=COPY or MOVE
)
echo.

:GetOpType
set /P op_type="Enter %op_prompt%: "
if %op_type% == COPY (
    echo Copying %data_src% to %data_dst%... Please wait.
    xcopy "%data_src%" "%data_dst%" /S /E /K /V /Q /R /Y /I
    GOTO OpDone
)
if %op_type% == MOVE (
    echo Moving %data_src% to %data_dst%.
    rmdir /S /Q "%data_dst%"
    move "%data_src%" "%data_dst%"
    GOTO OpDone
)
if %mklink_result% == 0 if %op_type% == LINK (
    rmdir /S /Q "%data_dst%"
    mklink /J "%data_dst%" "%data_src%"
    GOTO OpDone
)
echo ERROR: Unrecognized/unavailable option '%op_type%' (case matters).
GOTO GetOpType

:OpDone
if %errorlevel% NEQ 0 (
    echo ERROR: %op_type% operation failed.
    echo Either try a different operation or press CTRL-C to terminate.
    GOTO GetOpType
)

:CopyFiles
echo.
echo Copying Server Binaries to C:\playnet\wwiiol\bin
xcopy /QRY Server\*.exe C:\playnet\wwiiol\bin
xcopy /QRY Server\*.dll C:\playnet\wwiiol\bin
echo Copying Lua files to C:\playnet\wwiiol\lua
xcopy /QRY Lua\*.lua C:\playnet\wwiiol\lua


:RunInstaller
echo.
echo And now ... Running the CiaB bootstrap script.
call python bootstrap.py
if %errorlevel% NEQ 0 (
    echo.
    echo "Bootstrap failed. Please fix any problems and re-run."
    goto End
)

echo.
echo Creating 'start-cluster.bat' script.
echo @echo off >Server\start-cluster.bat
echo c: >>Server\start-cluster.bat
echo cd \playnet\wwiiol\bin >>Server\start-cluster.bat
echo start C:\playnet\tmp >>Server\start-cluster.bat
echo controller.exe >>Server\start-cluster.bat
echo pause >>Server\start-cluster.bat

echo.
echo ==========================================================================
echo.
echo SERVER:
echo To start the cluster: double-click 'Server\start-cluster.bat',
echo ( first time it runs, you need to allow firewall access and restart)
echo.
echo When the 'map' host says '++ Accepting logins ...' you can start a client,
echo.
echo CLIENT:
echo Use the '.bat' files in the 'Client' folder.
echo Each .bat represents a different account - rat or regular player.
echo Use different .bat files if you want to run multiple simultaneous clients.
echo.
echo ==========================================================================
echo.

:End
pause

:FINISH