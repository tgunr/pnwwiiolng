@echo off

echo ==========================================================================
echo WWII Online Dev Cluster-in-a-Box Setup Script for Windows
echo.
echo This script:
echo - Checks for Python and MySQL Installs...
echo -- For configuring the MySQL setup, please see README-MySQL-Win.txt
echo - Builds the Server Binaries
echo - Checks for the Playnet Directories
echo - Runs the Configuration Script
echo - Creates the Start-Cluster Script
echo - Moves the Server and Client Scripts to C:\playnet\scripts
echo ==========================================================================
echo.
pause

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
echo === Complete ===

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
echo === Complete ===

echo.
echo Checking for the Playnet Directory Structure
IF NOT EXIST C:\playnet (
    echo Creating C:\playnet directory structure
	mkdir C:\playnet
	mkdir C:\playnet\bin
	mkdir C:\playnet\etc
	mkdir C:\playnet\lua
	mkdir c:\playnet\scripts
	mkdir c:\playnet\tmp
	mkdir C:\playnet\wwiiol
	mkdir c:\playnet\wwiiol\bin
	mkdir c:\playnet\wwiiol\data
	mkdir c:\playnet\wwiiol\lua
	mkdir c:\playnet\wwiiol\uweb
	mkdir c:\playnet\wwiiol\uweb\debug
) ELSE (
    echo C:\playnet already exists, not touching structure.
)
echo === Complete ===

echo.
echo Creating the Server Binaries, be sure to "Build" then after Visual Studio opens...
echo.
cd Scripts
call "Host Cmake Built.bat"
cd ..\Deployment\ciab
echo === Complete ===

echo.
echo ==========================================================================
echo Before proceeding be sure to build the Server Binaries
echo They should be located at wwiiolng\build\Debug...
echo ==========================================================================

echo.
pause

:Build

echo.
IF NOT EXIST ..\..\build\Debug\controller.exe (
    echo The controller.exe is missing from wwiiolng\build\Debug, try again...
	echo.
	pause
	GOTO Build
) ELSE (
    echo The Server Binaries seem to be in order
)

echo.
echo Moving the Server Binaries
move ..\..\build\Debug\* Server\ >nul
echo === Complete ===

echo.
echo ==========================================================================
echo During the next Configuration Script, be sure to add the locations for the below items:
echo - data_origin_path ([Your Path]\ww2mods\DATA)
echo - bin_origin_path ([Your Path]\wwiiolng\Deployment\ciab\Server)
echo ==========================================================================

:RunInstaller
echo.
call python bootstrap.py
if %errorlevel% NEQ 0 (
    echo.
    echo "Bootstrap failed. Please fix any problems and re-run."
    goto End
)

echo.
echo Creating Start-Cluster.bat Script
echo @echo off >Server\~Start-Cluster.bat
echo c: >>Server\~Start-Cluster.bat
echo cd \playnet\wwiiol\bin >>Server\~Start-Cluster.bat
echo start C:\playnet\tmp >>Server\~Start-Cluster.bat
echo controller.exe >>Server\~Start-Cluster.bat
echo pause >>Server\~Start-Cluster.bat
echo === Complete ===

echo.
echo Moving the Start-Cluster.bat Script
move Server\~Start-Cluster.bat C:\playnet\scripts\
echo === Complete ===

echo.
echo Copying the libmysql.dll file
copy Installers\libmysql.dll C:\playnet\wwiiol\bin\
echo === Complete ===

echo.
echo Moving the Client Scripts
move Client\*.bat C:\playnet\scripts\
move Client\*.tkt C:\playnet\scripts\
echo === Complete ===

echo.
echo ==========================================================================
echo.
echo SERVER:
echo To start the cluster: Double-click 'C:\playnet\scripts\~Start-Cluster.bat',
echo (The first time it runs, you will need to allow firewall access and restart)
echo.
echo When the 'Map' host says '++ Accepting logins ...' you can then start a client...
echo.
echo CLIENT:
echo Use the '.bat' files in the C:\playnet\script folder.
echo Each .bat represents a different account - Producer, Rat, GM, Trainer or Regular Player with different ranks.
echo - See the 'CiaB Accounts.xlsx' spreadsheet for a breakdown of the specific account details.
echo Use different .bat files if you want to run multiple simultaneous clients.
echo In order to utilize the Data directory you must use a Beta Client, contact the Development team for more info.
echo.
echo ==========================================================================
echo.

:End
pause

:FINISH