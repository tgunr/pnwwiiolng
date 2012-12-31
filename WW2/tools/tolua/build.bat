
@rem Build ww2 lua bindings

@echo off
@setlocal
@set TOLUA=..\..\tools\tolua\tolua++

cd ..\..\src\cfg

FOR /f "tokens=*" %%G IN ('dir /AD /D /b ..') DO (
    cd ..
    cd %%G
    @echo "Building %%G"
    %TOLUA% -n %%G -H %%GLua.h -o %%GLua.cpp %%G.pkg)
    
PAUSE
