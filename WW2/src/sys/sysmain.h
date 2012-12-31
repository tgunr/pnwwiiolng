#pragma once

#define TIME_STAMP_LENGTH 128

#if platOS == OS_WIN32

typedef void (*SYS_MOUSEFUNC)(UINT message, WPARAM wparam, LPARAM lparam);
typedef void (*SYS_TIMERFUNC)(WPARAM wparam, LPARAM lparam);
typedef void (*SYS_KEYBOARDFUNC)(UINT message, WPARAM wparam, LPARAM lparam);

extern LOGICAL sysmainGetExecutableStamp(SCHAR* szTimeStamp, SINT32 maxLength);
extern int sysmainExceptionHandler(int code, EXCEPTION_POINTERS* ep);
extern void sysmainQuit(void);

extern void sysmainSetKeyboard(SYS_KEYBOARDFUNC KeyboardRoute);
extern BOOL sysmainSetTimer(SYS_TIMERFUNC newfunc);
extern void sysmainSetMouse(SYS_MOUSEFUNC MouseRoute);
extern void sysGetMousePosition(int*, int*);
extern void sysSetMousePosition(int, int);
extern void SetupFPU(bool enableExceptions);

#endif
