#pragma once

#undef EXTERN
#ifndef  GLOBALS
	#define EXTERN extern
#else
	#define EXTERN
#endif

	/*
		Constants and typedefs
	*/

	typedef LOGICAL (*DDWND_DDINITFUNC)(void);
	typedef LOGICAL (*DDWND_D3DINITFUNC)(void);
	typedef LOGICAL (*DDWND_DDRENDERFUNC)(void);
	typedef LOGICAL (*DDWND_D3DRENDERFUNC)(void);

	typedef void (*DDWND_DDKILLFUNC)(void);
	typedef void (*DDWND_D3DKILLFUNC)(void);

	typedef void (*DDWND_MOUSEFUNC)(UINT message, WPARAM wparam, LPARAM lparam);
	typedef void (*DDWND_TIMERFUNC)(WPARAM wparam, LPARAM lparam);
	typedef void (*DDWND_KEYBOARDFUNC)(UINT message, WPARAM wparam, LPARAM lparam);

	// flags to indicate mode of operation
	#define DDWND_FULLSCREEN    0x00000001
	#define DDWND_WINDOW        0x00000002
	#define DDWND_CAPTION       0x00000004
	#define DDWND_SYSMENU       0x00000008
	#define DDWND_SIZEABLE      0x00000010
	#define DDWND_FULLWINDOW    (DDWND_WINDOW|DDWND_CAPTION|DDWND_SYSMENU|DDWND_SIZEABLE)


	// flags to control status display
	#define DDWND_SHOW_NONE     0x00000000
	#define DDWND_SHOW_SIZE     0x00000001
	#define DDWND_SHOW_FPS      0x00000002
	#define DDWND_SHOW_ALL      DDWND_SHOW_SIZE|DDWND_SHOW_FPS


	// flags to allow certain devices to be used
	#define DDWND_ALLOW_TNL		0x00000001
	#define DDWND_ALLOW_HAL		0x00000002
	#define DDWND_ALLOW_RGB		0x00000004
	#define DDWND_ALLOW_MMX		0x00000008
	#define DDWND_ALLOW_RAMP	0x00000010
	#define DDWND_ALLOW_REF		0x00000020
	#define DDWND_ALLOW_NULL	0x00000040
	#define DDWND_ALLOW_ANY		0x0000007f

	extern HINSTANCE	ddpGetInstance(void);
	extern void		ddpSetInstance(HINSTANCE hnewInst);

	extern LOGICAL		ddpInit(void);
	extern void			ddpKill(void);
	extern UINT32		ddpEngine(LPVOID dummy);
	extern UINT32		ddpEngineCycle(LPVOID dummy);

	extern LOGICAL		ddpCreateWnd(SINT32 myx, SINT32 myy, SINT32 mywidth, SINT32 myheight, SINT32 mybpp, UINT32 myflags, HWND parent);

	extern LOGICAL		ddpFlip();
	extern LOGICAL		ddpUpdate();
	extern LOGICAL		ddpGetDDRect(RECT* rect);
	extern LOGICAL		ddpGetAccess();
	extern void		ddpReleaseAccess();
	extern LOGICAL		ddpLockBackbuffer(UCHAR** pbyte);
	extern void		ddpUnlockBackbuffer();
	extern LPVOID		ddpGetBackbuffer();
	extern LOGICAL		ddpGetBackbufferSurface(void** lpdds);
	extern LOGICAL		ddpDrawToScreen(LPVOID lpdds);
	extern LOGICAL		ddpDraw();
	extern LOGICAL		ddpIsAccessed();
	extern LOGICAL		ddpIsLocked();
	extern LOGICAL		ddpDrawNotActive(LOGICAL newpna);
	extern LOGICAL		ddpDisableScreenSaver(LOGICAL newss);
	extern SINT32		ddpGetTotalPaints(void);
	extern FLOAT32		ddpGetFPS();
	extern FLOAT32		ddpGetInstantFPS();
	extern void		ddpResetFPS();
	extern void		ddpCapture();
	extern SINT32		ddpGetWidth();
	extern SINT32		ddpGetHeight();
	extern SINT32		ddpGetBPP();
	extern SINT32		ddpGetNumDrivers();
	extern LOGICAL		ddpSetDDInit(DDWND_DDINITFUNC newfunc);
	extern LOGICAL		ddpSetD3DInit(DDWND_D3DINITFUNC newfunc);
	extern LOGICAL		ddpSetDDRender(DDWND_DDRENDERFUNC newfunc);
	extern LOGICAL		ddpSetD3DRender(DDWND_D3DRENDERFUNC newfunc);
	extern LOGICAL		ddpSetDDKill(DDWND_DDKILLFUNC newfunc);
	extern LOGICAL		ddpSetD3DKill(DDWND_D3DKILLFUNC newfunc);
	extern LOGICAL		ddpSetMouse(DDWND_MOUSEFUNC newfunc);
	extern LOGICAL		ddpSetTimer(DDWND_TIMERFUNC newfunc);
	extern LOGICAL		ddpSetKeyboard(DDWND_KEYBOARDFUNC newfunc);

	// Set the screen to proper pixel depth and size requirements
	extern LOGICAL		ddpChooseScreenFormat(HDC hdc, PIXELFORMATDESCRIPTOR* pfd);
	extern LOGICAL		ddpSetDisplay(UINT32 flags);
	extern void		ddpResetDisplay(void);

	extern LOGICAL ddcapStart(void);

	extern void ddpShowCursor();
	extern void ddpHideCursor();
	extern void ddpPumpMessage();
	extern HWND ddpGetHWND();
	extern void ddpSaveImgAsBitmap();

	extern DWORD GetMillisecondTick();
