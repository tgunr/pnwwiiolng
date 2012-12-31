#ifndef MLATMO_H
#define MLATMO_H

#include "syscfg.h"
#include "systypes.h"

// all altitude information is in metric
UINT32 mlGetAltTemp(FLOAT64 *Temp,FLOAT64 Alt);
UINT32 mlGetAltPressure(FLOAT64 *Pressure,FLOAT64 Alt);
UINT32 mlGetAltDensity(FLOAT64 *Density,FLOAT64 Alt);
UINT32 mlGetAltSpeedOfSound(FLOAT64 *Speed,FLOAT64 Alt);

#endif