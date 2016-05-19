echo on
rem DATA should point to the DATA full path
rem RESULT is anywhere you want the result to go full path

SET parent=%~dp0
SET DATA=%parent%..\..\opt\ww2mods\DATA\
SET BIN=%parent%
SET RESULT=%parent%..\..\WW2Archives\

%BIN%winbalancer.exe %DATA% %RESULT%

%BIN%win.exe %RESULT%\playarc01.ww2 %RESULT%\playarc01.txt
%BIN%win.exe %RESULT%\playarc02.ww2 %RESULT%\playarc02.txt
%BIN%win.exe %RESULT%\playarc03.ww2 %RESULT%\playarc03.txt
%BIN%win.exe %RESULT%\playarc04.ww2 %RESULT%\playarc04.txt
%BIN%win.exe %RESULT%\playarc05.ww2 %RESULT%\playarc05.txt
%BIN%win.exe %RESULT%\playarc06.ww2 %RESULT%\playarc06.txt
%BIN%win.exe %RESULT%\playarc07.ww2 %RESULT%\playarc07.txt
%BIN%win.exe %RESULT%\playarc08.ww2 %RESULT%\playarc08.txt
%BIN%win.exe %RESULT%\playarc09.ww2 %RESULT%\playarc09.txt
%BIN%win.exe %RESULT%\playarc10.ww2 %RESULT%\playarc10.txt
%BIN%win.exe %RESULT%\playarc11.ww2 %RESULT%\playarc11.txt

echo Completed
echo on
