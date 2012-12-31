#pragma once

// all altitude information is in metric
extern UINT32 mlGetAltTemp(FLOAT64* Temp,FLOAT64 Alt);
extern UINT32 mlGetAltPressure(FLOAT64* Pressure,FLOAT64 Alt);
extern UINT32 mlGetAltDensity(FLOAT64* Density,FLOAT64 Alt);
extern UINT32 mlGetAltSpeedOfSound(FLOAT64* Speed,FLOAT64 Alt);
