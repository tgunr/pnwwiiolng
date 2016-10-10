#pragma once

#define TIME_STAMP_LENGTH 128

#if platOS == OS_WIN32

using SYS_MOUSEFUNC = void(*)(UINT message, WPARAM wparam, LPARAM lparam);
using SYS_TIMERFUNC = void(*)(WPARAM wparam, LPARAM lparam);
using SYS_KEYBOARDFUNC = void(*)(UINT message, WPARAM wparam, LPARAM lparam);

extern LOGICAL sysmainGetExecutableStamp(SCHAR* szTimeStamp, SINT32 maxLength);
extern int sysmainExceptionHandler(int code, EXCEPTION_POINTERS* ep);
extern void sysmainQuit();

extern void sysmainSetKeyboard(SYS_KEYBOARDFUNC KeyboardRoute);
extern BOOL sysmainSetTimer(SYS_TIMERFUNC newfunc);
extern void sysmainSetMouse(SYS_MOUSEFUNC MouseRoute);
extern void sysGetMousePosition(int*, int*);
extern void sysSetMousePosition(int, int);
extern void SetupFPU(bool enableExceptions);

#endif
