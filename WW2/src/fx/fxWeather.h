#pragma once

#define WXSTATE_CLEAR_DAY 0x00000001
#define WXSTATE_CLOUDS    0x00000002
#define WXSTATE_RAIN      0x00000004
#define WXSTATE_SNOW      0x00000008

extern void fxWeatherUpdate();
extern void fxWeatherInit();
extern void fxWeatherDrawWindow(struct OBJECT* o);
extern void fxWeatherSetState(int weather, int seedindex);

extern bool fxWeatherIsCloudy();
extern bool fxWeatherIsRaining();
