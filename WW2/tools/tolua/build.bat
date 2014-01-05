
@rem Build ww2 lua bindings

@echo off
@setlocal
@set TOLUA=..\tools\tolua\tolua++

cd ..\..\src

FOR /f "tokens=*" %%G IN ('dir /AD /D /b .') DO (
    if exist %%G\%%G.pkg (
      @echo Building %%G
      %TOLUA% -n %%G -H %%G\%%GLua.h -o %%G\%%GLua.cpp %%G\%%G.pkg
    )
)

PAUSE
