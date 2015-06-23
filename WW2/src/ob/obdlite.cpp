#include <syscfg.h> // Path is project specific.
//

#include "pi/pi.h"
#include <math.h>
#include "mem/mem.h"
#include "ob/obmain.h"
#include "ob/obdlite.h"
#include "ob/obcs.h"
#include "ob/oblf.h"
#include "include/vars.h"
#include "math/math3d.h"
#include "fp/fpFPS.h"


static int          numdynalites;
static int          availdynalites;
static DYNALITE*    dynalites;


// Time for sun to travel in the sky 1 degree
#define UPDATE_TOD_DELTA	(4 * 60)


LOGICAL obdliteInit(void)
{
	numdynalites = 0;
	availdynalites = MINDYNALITES;
	dynalites = (DYNALITE*)memAlloc(availdynalites * sizeof(LIGHT));
	if (dynalites == nullptr)
	{
		return(FALSE);
	}

	if (!oblfInit())
	{
		return(FALSE);
	}

	return(TRUE);
}

int obdliteGetLights(DYNALITE** lplite)
{
	if (lplite == nullptr)
	{
		return(-1);
	}

	*lplite = dynalites;

	return(numdynalites);
}

void obdliteKill(void)
{
	if (dynalites != nullptr)
	{
		memFree(dynalites);
		dynalites = nullptr;
	}

    numdynalites = 0;

	oblfKill();
}

LIGHT* obdliteFindDynaLight(const char* name)
{
int   i;

	for (i=0 ; i < numdynalites ; i++)
	{
		if (lstrcmp(dynalites[i].light.name, name) == 0)
		{
			return(&dynalites[i].light);
		}
	}

	return(nullptr);
}

LOGICAL obdliteAddDynaLite(LIGHT* light, DYNALITE_UPDATE updfunc)
{
SINT32		i;

	if (numdynalites >= availdynalites)
	{
		return(FALSE);
	}

	for (i=0 ; i < numdynalites ; i++)
	{
		if (dynalites[i].light.light.dltType == 0)
		{
			break;
		}
	}

	if (i == numdynalites)
	{
		numdynalites += 1;
	}

	memset(&dynalites[i], 0, sizeof(DYNALITE));

	light->userdata1 = i;
	dynalites[i].light = *light;
	dynalites[i].updfunc = updfunc;

	if (light->light.dltType == piLIGHT_DIRECTIONAL)
	{
		dynalites[i].light.light.dvDirection = dynalites[i].light.light.dvPosition;
		math3dNormalize(&dynalites[i].light.light.dvDirection);
	}

#ifdef USE_3DAPI_LIGHTING
	dynalites[i].numlight = piCreateLight(light);
	if (dynalites[i].numlight == -1)
	{
		return(FALSE);
	}
#endif

	return(TRUE);
}

LOGICAL obdliteSetDynaLite(LIGHT* light)
{
SINT32	numlight;

	numlight = light->userdata1;
	if (numlight >= numdynalites)
	{
		return(FALSE);
	}

#if(0)
    if(!(light->flags & LITE_ON))
    {
        return FALSE;
    }
#endif

	dynalites[numlight].light = *light;

	if (light->light.dltType == piLIGHT_DIRECTIONAL)
	{
		dynalites[numlight].light.light.dvDirection = dynalites[numlight].light.light.dvPosition;
		math3dNormalize(&dynalites[numlight].light.light.dvDirection);
	}

#ifdef USE_3DAPI_LIGHTING
	if (!piSetLight(dynalites[numlight].numlight, light))
	{
		return(FALSE);
	}
#endif

	return(TRUE);
}

LOGICAL obdliteSetDynaState(LIGHT* light, LOGICAL state)
{
SINT32	numlight;

	numlight = light->userdata1;
	if (numlight >= numdynalites)
	{
		return(FALSE);
	}

	if (state)
	{
		light->flags |= LITE_ON;
	}
	else
	{
		light->flags &= ~LITE_ON;
	}

#ifdef USE_3DAPI_LIGHTING
	if (!piSetLightState(dynalites[numlight].numlight, state))
	{
		return(FALSE);
	}
#endif

	return(TRUE);
}

LOGICAL obdliteChangeFunction(const char* name, DYNALITE_UPDATE updfunc)
{
int   i;

	for (i=0 ; i < numdynalites ; i++)
	{
		if (lstrcmp(dynalites[i].light.name, name) == 0)
		{
			dynalites[i].updfunc = updfunc;
			return(TRUE);
		}
	}

	return(FALSE);
}

LOGICAL obdliteDynaLiteUpdate(void)
{
int   i;

	if (!DynamicLighting)
	{
		return(TRUE);
	}

	for (i=0 ; i < numdynalites ; i++)
	{
		if (dynalites[i].updfunc != nullptr)
		{
			dynalites[i].updfunc(&dynalites[i].light);
		}
	}

	return(TRUE);
}
