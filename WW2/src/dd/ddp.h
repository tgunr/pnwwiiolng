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

	using DDWND_DDINITFUNC = LOGICAL(*)();
	using DDWND_D3DINITFUNC = LOGICAL(*)();
	using DDWND_DDRENDERFUNC = LOGICAL(*)();
	using DDWND_D3DRENDERFUNC = LOGICAL(*)();

	using DDWND_DDKILLFUNC = void(*)();
	using DDWND_D3DKILLFUNC = void(*)();

	using DDWND_MOUSEFUNC = void(*)(UINT message, WPARAM wparam, LPARAM lparam);
	using DDWND_TIMERFUNC = void(*)(WPARAM wparam, LPARAM lparam);
	using DDWND_KEYBOARDFUNC = void(*)(UINT message, WPARAM wparam, LPARAM lparam);

	HINSTANCE	ddpGetInstance();
	void		ddpSetInstance(HINSTANCE hnewInst);

	LOGICAL		ddpInit();
	void		ddpKill();
	UINT32		ddpEngine(LPVOID dummy);
	UINT32		ddpEngineCycle(LPVOID dummy);

	bool ddpCreateWnd(void);

	LOGICAL		ddpFlip();
	LOGICAL		ddpUpdate();
	LOGICAL		ddpDraw();
	void		ddpDrawNotActive(bool newdna);
	void		ddpDisableScreenSaver(bool newss);
	SINT32		ddpGetTotalPaints();
	FLOAT32		ddpGetFPS();
	FLOAT32		ddpGetInstantFPS();
	void		ddpResetFPS();
	void		ddpCapture();
	SINT32 ddpGetWindowWidth(void);
	SINT32 ddpGetWindowHeight(void);
	SINT32		ddpGetWidth();
	SINT32		ddpGetHeight();
	LOGICAL		ddpSetDDInit(DDWND_DDINITFUNC newfunc);
	LOGICAL		ddpSetD3DInit(DDWND_D3DINITFUNC newfunc);
	LOGICAL		ddpSetDDRender(DDWND_DDRENDERFUNC newfunc);
	LOGICAL		ddpSetD3DRender(DDWND_D3DRENDERFUNC newfunc);
	LOGICAL		ddpSetDDKill(DDWND_DDKILLFUNC newfunc);
	LOGICAL		ddpSetD3DKill(DDWND_D3DKILLFUNC newfunc);
	LOGICAL		ddpSetMouse(DDWND_MOUSEFUNC newfunc);
	LOGICAL		ddpSetTimer(DDWND_TIMERFUNC newfunc);
	LOGICAL		ddpSetKeyboard(DDWND_KEYBOARDFUNC newfunc);

	GLuint ddpDefaultFramebuffer(void);

	LOGICAL		ddcapStart();

	void		ddpShowCursor();
	void		ddpHideCursor();
	void		ddpPumpMessage();
	HWND		ddpGetHWND();
	void		ddpSaveImgAsBitmap();

	DWORD		GetMillisecondTick();
