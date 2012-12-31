#include "syscfg.h"
#include "systypes.h"

#include <math.h>

#include "mlAtmo.h"

// all units are in metric


// temp calc only good to about 20km
// Temp returned in Kelvin
UINT32 mlGetAltTemp(FLOAT64 *Temp,FLOAT64 Alt)
{
FLOAT64 ReturnTemp = 0;
FLOAT64 Theta = 0;

	Theta = 0.751861;

	if (Alt < 11000)
	{
		Theta = 1.0 - Alt/44330;
	}

	ReturnTemp = 288.15 * Theta;

	*Temp = ReturnTemp;

	return 0;
}

// pressure calc only good to about 20km
// Pressure returned in Pascals
UINT32 mlGetAltPressure(FLOAT64 *Pressure,FLOAT64 Alt)
{
FLOAT64 ReturnPressure = 0;
FLOAT64 Sigma = 0;

	Sigma = 0.0718645;

	if (Alt < 20000)
	{
		Sigma = 0.29705553 * exp((11000-Alt)/6341.36);
	}
	if (Alt < 11000)
	{
		Sigma = pow(1.0 - Alt/44328.6,4.255876);
	}

	ReturnPressure = 101327 * Sigma;

	*Pressure = ReturnPressure;

	return 0;
}

// atmospheric density calc only good to about 20km
// atmospheric density returned in kg/m^3
UINT32 mlGetAltDensity(FLOAT64 *Density, FLOAT64 Alt)
{
	static FLOAT64 DensityTable[202];
	static int tableInited  = 0;

	FLOAT64 ReturnDensity = 0;
	SINT32 i, j;
	FLOAT64 Delta = 0;
    FLOAT64 TableAlt = 0;

	if(!tableInited)
	{	
		for(i = 0; i <= 201; i++)
		{
			TableAlt = 100.0 * (FLOAT64)i;
			Delta = 0.05402977;

			if (TableAlt < 20000)
			{
				Delta = 0.2233422 * exp((11000-TableAlt)/6341.36);
			}
			if (TableAlt < 11000)
			{
				Delta = pow(1.0 - TableAlt/44328.6,5.255876);
			}

			DensityTable[i] = 1.22500 * Delta;			
		}
		tableInited = 1;
	}
	
	if(Alt < 0) Alt = 0;
	if(Alt > 20000) Alt = 20000;
	
	i = (int)(Alt / 100.0);
	j = (int)(Alt / 100.0) + 1;
	Delta = (Alt - 100.0 * (FLOAT64)i)  / 100.0;
	
	ReturnDensity = (1 - Delta) * DensityTable[i] +  Delta * DensityTable[j];
	*Density = ReturnDensity;

	return 0;
}

// speed of sound calc only good to about 20km
// speed of sound returned in m/s
UINT32 mlGetAltSpeedOfSound(FLOAT64 *Speed,FLOAT64 Alt)
{
FLOAT64 ReturnSpeedOfSound = 0;
FLOAT64 Temp = 0;

	mlGetAltTemp(&Temp,Alt);

	ReturnSpeedOfSound = sqrt(1.4*287.06*Temp);

	*Speed = ReturnSpeedOfSound;

	return 0;
}