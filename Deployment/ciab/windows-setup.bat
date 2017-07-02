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
    echo Creating C:\playnet directory structure.
    call make-win-playnet.bat
) ELSE (
    echo C:\playnet already exists, not touching structure.
)

:RunInstaller
echo.
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