#pragma once

#define USE_SHADOWS
#ifdef USE_SHADOWS

#define USE_NEW_SHADOWS
#undef USE_NEW_SHADOWS

extern LOGICAL obdshadInit();
extern LOGICAL obdshadCreate(struct OBJECT* lptop, struct OBJECT* lpobj);
extern LOGICAL obdshadProject(struct OBJECT* lpobj);
extern LOGICAL obdshadPosition(struct OBJECT* lpmain, struct OBJECT* lpobj);
extern LOGICAL obdshadUpdate();

extern void obdshadKill();
extern void obdBeginShadowMap();
extern void obdEndShadowMap();

#endif // USE_SHADOWS
