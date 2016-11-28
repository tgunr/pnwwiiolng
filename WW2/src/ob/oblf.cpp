#include <syscfg.h> // Path is project specific.
//

#include <math.h>
#include "pi/pi.h"
#include "include/vars.h"
#include "ld/ldpix.h"
#include "ld/ldbmp.h"
#include "ob/obmain.h"
#include "ob/obcd.h"
#include "ob/obdlite.h"
#include "ob/oblf.h"
#include "math/math3d.h"

static int    flareid[NUMFLAREFX];
static int    numflares;
static FLARE	flare[20];


static FLARE SetFlare(int gfx, float location, float scale, float red, float green, float blue)
{
FLARE   ret;

	ret.gfx   = gfx;
	ret.loc   = location;
	ret.scale = scale;
	ret.red   = red;
	ret.green = green;
	ret.blue  = blue;

	return(ret);
}

LOGICAL oblfInit()
{
LOGICAL  retval=TRUE;
char  texname[256];
int   i;

	for (i=0 ; i < NUMFLAREGFX ; i++)
	{
		strprintf(texname, "flare%d.bmp", i+1);
		flareid[i] = ldpixLoadImageFileToSurface(texname, colorBLACK, LDPIX_NONE);
		if (!piIsTexture(flareid[i], 0))
		{
			Write_Debug("Flare texture file not found: %s", texname);
			flareid[i] = -1;
			retval = FALSE;
		}
	}

	numflares = 0;
	flare[numflares++] = SetFlare(2,  1.3f,	 0.04f, 1.0f,  0.75f, 0.75f);
	flare[numflares++] = SetFlare(3,  1.0f,	 0.1f,  1.0f,  1.0f,  1.0f);
	flare[numflares++] = SetFlare(0,  1.0f,  0.06f, 1.0f,  1.0f,  1.0f);
	flare[numflares++] = SetFlare(1,  0.5f,	 0.15f, 0.75f, 1.0f,  1.0f);
	flare[numflares++] = SetFlare(3,  0.2f,	 0.05f, 0.75f, 0.75f, 0.75f);
	flare[numflares++] = SetFlare(0,  0.0f,  0.04f, 1.0f,  1.0f,  1.0f);
	flare[numflares++] = SetFlare(5, -0.25f, 0.07f, 1.0f,  0.75f, 1.0f);
/*
	flare[numflares++] = SetFlare(5, -0.4f,	 0.02f, 1.0f,  1.0f,  1.0f);
	flare[numflares++] = SetFlare(5, -0.6f,	 0.04f, 1.0f,  1.0f,  1.0f);
	flare[numflares++] = SetFlare(5, -1.0f,	 0.03f, 1.0f,  1.0f,  1.0f);
*/
  return(retval);
}

static LOGICAL DisplayFlares(LIGHT* light)
{
int       i, nflares;
UINT32     color;
float     d, ls, al;
piVECTOR  p, pd, c, ld, l, a, dx, dy, sx, sy;
piLVERTEX Vertices[4];
piCOLORVALUE  cv;
UINT16		  indices[6] = { 0, 1, 3, 1, 2, 3 };

	p.x = -playpos.x;
	p.y = -playpos.y;
	p.z = -playpos.z;

	// create a normal directed at the light
	ld = l = light->light.dvPosition;
	math3dSubtractVectors(&ld, &ld, &p);
	math3dNormalize(&ld);

	// find the angle to the light
	d = math3dDotProduct(&ld, &playdir);
	if (d <= 0.5f)  // more than 60 degrees, so don't show a flare
	{
		return(TRUE);
	}

	// if there are any obscuring objects, don't show a flare
	if (obcdObject(nullptr, &p, &l))
	{
		return(TRUE);
	}

	// locate the center of the flare axis
	math3dScaleVector(&pd, &playdir, 1.02f);
	math3dAddVectors(&c, &p, &pd);

	// find the axis vector perpendicular to the screen
	math3dScaleVector(&l, &ld, 1.0f / d);
	math3dScaleVector(&l, &l, 1.02f);
	math3dAddVectors(&l, &l, &p);
	math3dSubtractVectors(&a, &l, &c);

	// create a unit width and height
	dx = a;
	math3dNormalize(&dx);
	math3dCrossProduct(&dy, &dx, &playdir);

	if (light->flags & LITE_LFBRIGHT)
	{
		i = 0;
		nflares = numflares;
		ls = 1.0f;
		al = 0.5f;
	}
	else
	if (light->flags & LITE_LFMEDIUM)
	{
		i = 2;
		nflares = numflares - 2;
		ls = 0.95f;
		al = 0.25f;
	}
	else
	{
		i = 2;
		nflares = i + 1;
		ls = 0.9f;
		al = 0.0f;
	}

	cv = light->light.dcvColor;

	piDisableMaterial();
	for ( ; i < nflares ; i++)
	{
		math3dScaleVector(&sx, &dx, flare[i].scale * 1.2f);
		math3dScaleVector(&sy, &dy, flare[i].scale * 1.2f);

		math3dScaleVector(&l, &a, flare[i].loc);
		math3dAddVectors(&p, &c, &l);
		math3dScaleVector(&pd, &playdir, (float)(numflares - i) * 0.01f * ScaleFactor);
		math3dAddVectors(&p, &p, &pd);

		color = piRGBA(cv.r * flare[i].red * ls,
					   cv.g * flare[i].green * ls,
					   cv.b * flare[i].blue * ls,
					   d - al);

		Vertices[0].x = p.x + sx.x + sy.x;
		Vertices[0].y = p.y + sx.y + sy.y;
		Vertices[0].z = p.z + sx.z + sy.z;
		Vertices[0].color = color;
		Vertices[0].specular = colorALPHA100;
		Vertices[0].tu = 0.0f;
		Vertices[0].tv = 0.0f;

		Vertices[1].x = p.x - sx.x + sy.x;
		Vertices[1].y = p.y - sx.y + sy.y;
		Vertices[1].z = p.z - sx.z + sy.z;
		Vertices[1].color = color;
		Vertices[1].specular = colorALPHA100;
		Vertices[1].tu = 1.0f;
		Vertices[1].tv = 0.0f;

		Vertices[2].x = p.x - sx.x - sy.x;
		Vertices[2].y = p.y - sx.y - sy.y;
		Vertices[2].z = p.z - sx.z - sy.z;
		Vertices[2].color = color;
		Vertices[2].specular = colorALPHA100;
		Vertices[2].tu = 1.0f;
		Vertices[2].tv = 1.0f;

		Vertices[3].x = p.x + sx.x - sy.x;
		Vertices[3].y = p.y + sx.y - sy.y;
		Vertices[3].z = p.z + sx.z - sy.z;
		Vertices[3].color = color;
		Vertices[3].specular = colorALPHA100;
		Vertices[3].tu = 0.0f;
		Vertices[3].tv = 1.0f;

		piSetActiveTexture(flareid[flare[i].gfx], 0);
		if (!piRenderIndexScene(piPT_TRIANGLELIST, piVT_LVERTEX, Vertices, 4, indices, 6))
		{
			return(FALSE);
		}
	}

	return(TRUE);
}

void oblfUpdate()
{
	DYNALITE*  lplites;
	LIGHT*     light;

	const int numlites = obdliteGetLights(&lplites);
	if(!lplites || numlites <= 0)
		return;

	for (int i=0 ; i < numlites ; i++, lplites++)
	{
		light = &lplites->light;
		if (light->flags & LITE_ON &&
			light->flags & LITE_LENSFLARES)
		{
			DisplayFlares(light);
		}
	}
}

void oblfKill()
{
int   i;

  for (i=0 ; i < NUMFLAREGFX ; i++)
  {
	  piFreeMipTextures(flareid[i]);
    flareid[i] = -1;
  }
}
