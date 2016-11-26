@echo off

echo WWIIOnline Cluster-in-a-Box Setup Script for Windows.
echo.
echo Checks for Python and MySQL installs,
echo For configuring the MySQL setup, please see README-MySQL-Win.txt.
echo.

:CheckPython
echo Checking for Python:
python --version
if %errorlevel% == 0 GOTO PythonInstalled

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

:PythonInstalled
echo Great! Python is already installed.

:CheckMysql
echo.
echo Checking for MySQL:
python -m lib.findmysql
if %errorlevel% == 0 GOTO MySQLInstalled

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
start README-MySQL-Win.txt
GOTO End

:MySQLInstalled
echo Great! MySQL is already installed.

:CheckPlaynet
echo.
IF NOT EXIST C:\playnet GOTO PlaynetExists
echo Creating C:\playnet directories.
GOTO :MakePlaynet
:PlaynetExists
echo C:\playnet already exists, making sure it's populated correctly...
:MakePlaynet
call make-win-playnet.bat

:CheckData
echo.
echo Checking for blitzWor:
IF EXIST C:\playnet\wwiiol\data\blitzWor.csv GOTO PNDataExists
rmdir /S /Q C:\playnet\wwiiol\data
echo There's no blitzWor.csv in C:\playnet\wwiiol\data, checking for WW2Mods:
IF EXIST WW2Mods\DATA\blitzWor.csv GOTO ModsDataExists
echo Not there, checking for DATA\BlitzWor.csv:
IF EXIST DATA\blitzWor.csv GOTO DataExists
echo.
echo ========== PROBLEM ==========
echo.
echo ERROR: Could not find a DATA directory with blitzWor.csv.
echo.
echo If you have a WW2Mods directory please copy the DATA directory to
echo C:\Playnet\wwiiol so that 'blitzWor.csv' is in C:\Playnet\wwiiol\data\
goto End

:PNDataExists
echo Great - you appear to have \playnet\wwiiol\data\ ready to go.
GOTO CopyFiles

:ModsDataExists
set data_src=WW2Mods\DATA
GOTO LinkData

:DataExists
set data_src=DATA
GOTO LinkData

:LinkData
echo Found blitzWor.csv in %data_src%.
echo.
echo ========== NOTE ==========
echo.
echo Creating a link from %data_src% to \playnet\wwiiol\data
echo.
echo You may want to move or copy your data to the playnet\wwiiol folder
echo since this link will break if you delete or move the current folder.
echo.

pause

mklink /D C:\playnet\wwiiol\data %cd%\%data_src%
GOTO CopyFiles

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

pause

start .

goto FINISH

:End
pause

:FINISH