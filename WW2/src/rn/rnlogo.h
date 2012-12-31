#pragma once

extern LOGICAL rnlogoInit(void);
extern LOGICAL rnlogoRender(void);

extern void rnlogoKill(void);
extern void rnUpdateLoadingScreen ();
extern void rnUpdateLoadingEnd();
extern void rnlogoUpdateStatus(const char*, ...);
extern void rnlogoSplash();
