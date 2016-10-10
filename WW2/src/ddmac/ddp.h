#pragma once

using DDWND_DDRENDERFUNC = LOGICAL(*)();
using DDWND_D3DRENDERFUNC = LOGICAL(*)();

extern LOGICAL cursorHidden;

#ifdef __cplusplus
extern "C"
{
#endif

LOGICAL	ddpInit();
void	ddpKill();
LOGICAL	ddpSetDDRender(DDWND_DDRENDERFUNC newfunc);
LOGICAL	ddpSetD3DRender(DDWND_D3DRENDERFUNC newfunc);
UINT32 	ddpEngine(void*);
LOGICAL ddpCreateWnd(int x, int y, int width, int height, int bpp, int fullscreen, int displayMask, int refreshrate);
LOGICAL	ddpFlip();
LOGICAL	ddpUpdate();
LOGICAL	ddpDrawToScreen(LPVOID lpdds);
LOGICAL	ddpDraw();
long	ddpGetTotalPaints();
float	ddpGetFPS();
void	ddpResetFPS();
void	ddpCapture();
int		ddpGetWidth();
int		ddpGetHeight();
int     ddpGetCenterX();
int     ddpGetCenterY();
UINT32	ddpGetClockTime(UINT32 * Time);
UINT32	ddpGetTotTime(UINT32 * TotTime);

void	ddpSetupGL(int w, int h);
void 	ddpShowCursor();
void 	ddpHideCursor();

void	ddpToggleWindowedMode();
void	ddpSetFullScreen(LOGICAL);
void	ddpHideApplication();
void	ddpShowApplication();

void ddpHandleVsync();

#ifdef __cplusplus
}
#endif

