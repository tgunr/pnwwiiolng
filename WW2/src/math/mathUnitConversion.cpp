#include "syscfg.h" // Path is project specific.
#include <ctype.h>
#include <string.h>

#include "math/mathUnitConversion.h"
#include "ml/mlutility.h"

enum
{
	UNIT_MPH = 1,
	UNIT_KPH,
	UNIT_METER,
	UNIT_FOOT,
	UNIT_MAX
};

static const char _UnitNames[UNIT_MAX][32] = { "none", "mph", "kph", "m", "ft"};

/************************************************************************************************
	mathUnitAsciiToMetric
*************************************************************************************************/

 FLOAT32 mathUnitAsciiToMetric(char* strval)
{
	FLOAT32 val = 0;
	SINT32 len, i;
	UINT32 units;
	char* unitstr;

	len = strlen(strval);

	// find units
	for(i = len - 1; i >= 0; i--)
	{
		if(isdigit(strval[i]))
		{
			i++; break;
		}
	}

	unitstr = &strval[i];
	mlUtilityFindVarname(&units, (UCHAR*)unitstr, (UCHAR*)&_UnitNames[0][0], 32, UNIT_MAX);

	strval[i] = '\0';
	val = (FLOAT32)atof(strval);

	switch(units)
	{
		case UNIT_MPH:
			val *= 1609.0f / 3600.0f;
		break;

		case UNIT_KPH:
			val *= 1000.0f / 3600.0f;
		break;

		case UNIT_FOOT:
			val *= .3048f;
		break;
	}

	return val;
}
