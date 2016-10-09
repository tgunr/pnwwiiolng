#pragma once

#include "pi/pi.h"

#define LITE_ON           0x00000001
#define LITE_LENSFLARES   0x00000002
#define LITE_LFMEDIUM     0x00000004
#define LITE_LFBRIGHT     0x00000008

using DYNALITE_UPDATE = LOGICAL(*)(struct LIGHT* light);

struct DYNALITE
{
	LIGHT				light;
	DYNALITE_UPDATE		updfunc;
	SINT32				numlight;
};

extern LOGICAL obdliteInit(void);
extern int obdliteGetLights(DYNALITE** lplite);
extern void obdliteKill(void);
extern LIGHT* obdliteFindDynaLight(const char* name);
extern LOGICAL obdliteAddDynaLite(LIGHT* light, DYNALITE_UPDATE updfunc);
extern LOGICAL obdliteSetDynaLite(LIGHT* light);
extern LOGICAL obdliteSetDynaState(LIGHT* light, LOGICAL state);
extern LOGICAL obdliteChangeFunction(const char* name, DYNALITE_UPDATE updfunc);
extern LOGICAL obdliteDynaLiteUpdate(void);
