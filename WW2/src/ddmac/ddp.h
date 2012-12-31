#pragma once

typedef LOGICAL (*DDWND_DDRENDERFUNC)(void);
typedef LOGICAL (*DDWND_D3DRENDERFUNC)(void);

#ifdef __cplusplus
extern "C"
{
#endif

LOGICAL	ddpInit();
void	ddpKill();
LOGICAL	ddpSetDDRender(DDWND_DDRENDERFUNC newfunc);
LOGICAL	ddpSetD3DRender(DDWND_D3DRENDERFUNC newfunc);
UINT32 	ddpEngine(void*);
LOGICAL ddpCreateWnd(int myx, int myy, int mywidth, int myheight, int mybpp, int fullscreen, int monitor, int refreshrate);
LOGICAL	ddpFlip();
LOGICAL	ddpUpdate();
LOGICAL	ddpDrawToScreen(LPVOID lpdds);
LOGICAL	ddpDraw();
long	ddpGetTotalPaints(void);
float	ddpGetFPS();
void	ddpResetFPS();
void	ddpCapture();
int		ddpGetWidth();
int		ddpGetHeight();
int	 	ddpGetBPP();
UINT32	ddpGetClockTime(UINT32 * Time);
UINT32	ddpGetTotTime(UINT32 * TotTime);

LOGICAL ddcapSave(UCHAR* bptr, UINT32 width, UINT32 height, char* bname);
LOGICAL ddcapStart();

void	ddpSetupGL(int w, int h);
void 	ddpShowCursor(void);
void 	ddpHideCursor(void);

void	ddpToggleWindowedMode();
void	ddpSetFullScreen(LOGICAL);
void	ddpHideApplication();
void	ddpShowApplication();
bool	ddpAppHidden();

void ddpHandleVsync();

#ifdef __cplusplus
}
#endif

