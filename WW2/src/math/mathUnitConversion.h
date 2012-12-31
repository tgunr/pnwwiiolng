#pragma once

/************************************************************************************************
	mathUnitConversion
*************************************************************************************************/
// Unit conversion macros

// angles
#define DEG_TO_RAD(x) 	((x) * 0.017453f)
#define DEG_TO_MIL(x) 	((x) * 17.777778f)

#define RAD_TO_DEG(x) 	((x) * 57.29578f)
#define RAD_TO_MIL(x) 	((x) * 1018.591636f)

#define MIL_TO_DEG(x) 	((x) * 0.05625f)
#define MIL_TO_RAD(x) 	((x) * 0.000982f)

// speed
#define KTS_TO_MPS(x) 	((x) * 0.514444f)
#define MPS_TO_KTS(x) 	((x) * 1.9438461717893491f)

// distance
#define METER_TO_FT(x) 	((x) * 3.28084f)
#define METER_TO_YD(x)	((x) * 1.093613f)
#define FT_TO_METER(x)  ((x) * 0.3048f)

// vol
#define GAL_TO_M3(x)	((x) * .003785)
#define LITER_TO_M3(x)	((x) * .001)

// "mass"/weight
#define LBS_TO_KG(x)	((x) * 0.4536f)

// time
#define	MS_TO_S(x)		((x) * 0.001f)
#define	S_TO_MS(x)		((x) * 1000.0f)

extern FLOAT32 mathUnitAsciiToMetric(char*);

