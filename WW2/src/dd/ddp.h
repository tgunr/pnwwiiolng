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

	using DDWND_DDINITFUNC = LOGICAL(*)(void);
	using DDWND_D3DINITFUNC = LOGICAL(*)(void);
	using DDWND_DDRENDERFUNC = LOGICAL(*)(void);
	using DDWND_D3DRENDERFUNC = LOGICAL(*)(void);

	using DDWND_DDKILLFUNC = void(*)(void);
	using DDWND_D3DKILLFUNC = void(*)(void);

	using DDWND_MOUSEFUNC = void(*)(UINT message, WPARAM wparam, LPARAM lparam);
	using DDWND_TIMERFUNC = void(*)(WPARAM wparam, LPARAM lparam);
	using DDWND_KEYBOARDFUNC = void(*)(UINT message, WPARAM wparam, LPARAM lparam);

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

	HINSTANCE	ddpGetInstance(void);
	void		ddpSetInstance(HINSTANCE hnewInst);

	LOGICAL		ddpInit(void);
	void		ddpKill(void);
	UINT32		ddpEngine(LPVOID dummy);
	UINT32		ddpEngineCycle(LPVOID dummy);

	LOGICAL		ddpCreateWnd(SINT32 myx, SINT32 myy, SINT32 mywidth, SINT32 myheight, SINT32 mybpp, UINT32 myflags, HWND parent);

	LOGICAL		ddpFlip();
	LOGICAL		ddpUpdate();
	LOGICAL		ddpGetDDRect(RECT* rect);
	LOGICAL		ddpGetAccess();
	void		ddpReleaseAccess();
	LOGICAL		ddpLockBackbuffer(UCHAR** pbyte);
	void		ddpUnlockBackbuffer();
	LPVOID		ddpGetBackbuffer();
	LOGICAL		ddpGetBackbufferSurface(void** lpdds);
	LOGICAL		ddpDrawToScreen(LPVOID lpdds);
	LOGICAL		ddpDraw();
	LOGICAL		ddpIsAccessed();
	LOGICAL		ddpIsLocked();
	LOGICAL		ddpDrawNotActive(LOGICAL newpna);
	LOGICAL		ddpDisableScreenSaver(LOGICAL newss);
	SINT32		ddpGetTotalPaints(void);
	FLOAT32		ddpGetFPS();
	FLOAT32		ddpGetInstantFPS();
	void		ddpResetFPS();
	void		ddpCapture();
	SINT32		ddpGetWidth();
	SINT32		ddpGetHeight();
	SINT32		ddpGetBPP();
	SINT32		ddpGetNumDrivers();
	LOGICAL		ddpSetDDInit(DDWND_DDINITFUNC newfunc);
	LOGICAL		ddpSetD3DInit(DDWND_D3DINITFUNC newfunc);
	LOGICAL		ddpSetDDRender(DDWND_DDRENDERFUNC newfunc);
	LOGICAL		ddpSetD3DRender(DDWND_D3DRENDERFUNC newfunc);
	LOGICAL		ddpSetDDKill(DDWND_DDKILLFUNC newfunc);
	LOGICAL		ddpSetD3DKill(DDWND_D3DKILLFUNC newfunc);
	LOGICAL		ddpSetMouse(DDWND_MOUSEFUNC newfunc);
	LOGICAL		ddpSetTimer(DDWND_TIMERFUNC newfunc);
	LOGICAL		ddpSetKeyboard(DDWND_KEYBOARDFUNC newfunc);

	// Set the screen to proper pixel depth and size requirements
	LOGICAL		ddpChooseScreenFormat(HDC hdc, PIXELFORMATDESCRIPTOR* pfd);
	LOGICAL		ddpSetDisplay(UINT32 flags);
	void		ddpResetDisplay(void);

	LOGICAL		ddcapStart(void);

	void		ddpShowCursor();
	void		ddpHideCursor();
	void		ddpPumpMessage();
	HWND		ddpGetHWND();
	void		ddpSaveImgAsBitmap();

	DWORD		GetMillisecondTick();
