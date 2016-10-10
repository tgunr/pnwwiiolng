#pragma once

extern LOGICAL rnlogoInit();
extern LOGICAL rnlogoRender();

extern void rnlogoKill();
extern void rnUpdateLoadingScreen ();
extern void rnUpdateLoadingEnd();
extern void rnlogoUpdateStatus(const char*, ...);
extern void rnlogoSplash();
