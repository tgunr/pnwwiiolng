#include "syscfg.h" // Path is project specific.
//

#include <math.h>
#include "pi/pi.h"
#include "include/vars.h"
#include "ob/obslite.h"
#include "ob/obdlite.h"
#include "ob/obmain.h"		// For OBJECT
#include "math/math3d.h"

LOGICAL obsliteCalculate(int numobjects, OBJECT* object, int numlights, LIGHT* lightlist, UINT32 amblite)
{
int				i, j, k, matnum;
int				numdynalites;
UINT32			amb, color;
float			rlt, glt, blt, ro, go, bo, ao, rl, gl, bl;
float			x, y, z, d, atn, dp, sp, sr, sg, sb;
float			ar, ag, ab, tndl;
float			Kar, Kag, Kab;
float			Kdr, Kdg, Kdb;
float			Ksr, Ksg, Ksb;
float			Ker, Keg, Keb;
float			dvr;
DYNALITE*		dynalites;
piVECTOR		l, vv;
piLIGHT*		light;
MATERIAL*		lpmats;
piMATERIAL*		lpmaterial;
OBJECT*			lpobj;
DYNALITE*		lplight;

#ifdef EVENT_LOG_TIMES
static UINT64 count = 0;
static int number = 0;

UINT64 beginhere = GetMicroSeconds();
#endif

	amb = piGetAmbLight();
	ar = (float)piRGBA_GETRED(amb) * colorONEOVER255;
	ag = (float)piRGBA_GETGREEN(amb) * colorONEOVER255;
	ab = (float)piRGBA_GETBLUE(amb) * colorONEOVER255;

	numdynalites = obdliteGetLights(&dynalites);

	matnum = piGetMaterials(&lpmats);

	for (i=0 ; object != nullptr ; i++, object=(OBJECT*)object->attached)
	{
		lpobj = object;
		if (!(lpobj->flags & OBJECT_LITE))
		{
			continue;
		}

		if (piIsMaterial(lpobj->matid))
		{
			lpmaterial = &lpmats[lpobj->matid].material;
		}
		else
		{
			lpmaterial = nullptr;
		}

		if (lpmaterial != nullptr)
		{
			Kar = lpmaterial->ambient.r;
			Kag = lpmaterial->ambient.g;
			Kab = lpmaterial->ambient.b;
			Kdr = lpmaterial->diffuse.r * GammaRed;
			Kdg = lpmaterial->diffuse.g * GammaGreen;
			Kdb = lpmaterial->diffuse.b * GammaBlue;
			Ksr = lpmaterial->specular.r;
			Ksg = lpmaterial->specular.g;
			Ksb = lpmaterial->specular.b;
			Ker = lpmaterial->emissive.r;
			Keg = lpmaterial->emissive.g;
			Keb = lpmaterial->emissive.b;
		}
		else
		{
			Kar = 1.0f;
			Kag = 1.0f;
			Kab = 1.0f;
			Kdr = GammaRed;
			Kdg = GammaGreen;
			Kdb = GammaBlue;
			Ksr = 1.0f;
			Ksg = 1.0f;
			Ksb = 1.0f;
			Ker = 0.0f;
			Keg = 0.0f;
			Keb = 0.0f;
		}

		piVectorConstPtr rpos 		= lpobj->RenderPositions();
		piVectorConstPtr rnorm 		= lpobj->RenderNormals();
		piVertexColorConstPtr ic 	= lpobj->ImmutableColors();
		piVertexColorPtr mc 		= lpobj->MutableColors();

		// iterate through the vertices
		for (j=lpobj->numverts ; j ; j--, ++rpos, ++ic, ++mc, ++rnorm)
		{
			// init to the color of the object vertex
			color = ic->color;
			ro = (float)piRGBA_GETRED(color);
			go = (float)piRGBA_GETGREEN(color);
			bo = (float)piRGBA_GETBLUE(color);
			ao = (float)piRGBA_GETALPHA(color);

			// init the new color of the object vertex
			rlt = glt = blt = 0.0f;

			// we'll reuse this vertex' values several times
			x = rpos->x;
			y = rpos->y;
			z = rpos->z;

			if (!(lpobj->flags & OBJECT_NOSPECULAR))
			{
				// create the view vector
				vv.x = x + playpos.x;
				vv.y = y + playpos.y;
				vv.z = z + playpos.z;

				math3dNormalize(&vv);
			}

			lplight = dynalites;

			// initialize specular lighting
			sr = sg = sb = 0.0f;

			tndl = 0.0f;

			// iterate through each light source
			for (k=numdynalites ; k ; k--, lplight++)
			{
				if (!lplight->light.flags & LITE_ON)
				{
					continue;
				}

				// temporary pointer for light values
				light = &lplight->light.light;

				// if this is a spot light, early out
				if (piLIGHT_POINT != light->dltType)
				{
					continue;
				}

				// create a vector between light position and vertex position
				l.x = light->dvPosition.x - x;
				l.y = light->dvPosition.y - y;
				l.z = light->dvPosition.z - z;

				// resolve the distance
				d = l.x * l.x + l.y * l.y + l.z * l.z;

				// get angle between vertex/light vector and vertex normal
				atn = math3dDotProduct(&l, rnorm.ptr);
				if (0.0f >= atn)
				{
					continue;
				}

				dvr = light->dvRange * light->dvRange;

				// are we beyond the max distance for this light?
				if (d > dvr)
				{
					continue;
				}

				d = 1.0f / (float)sqrt(d);
				atn *= d;
				tndl += atn;

				rl = light->dcvColor.r;
				gl = light->dcvColor.g;
				bl = light->dcvColor.b;

				if (!(lpobj->flags & OBJECT_NOSPECULAR))
				{
					math3dScaleVector(&l, &l, d);

					// get angle between vertex/light vector and view vector
					dp = math3dDotProduct(&l, &vv);
					if (dp > 0.0f)
					{
						sp = dp * dp * dp * dp * dp * dp * dp * dp * dp * dp * dp * dp;//(float)pow(dp, 6 * InvScaleFactor);
						sr += sp * rl * 255.0f;
						sg += sp * gl * 255.0f;
						sb += sp * bl * 255.0f;
					}
				}

				// running light total color
				rlt += rl * atn;
				glt += gl * atn;
				blt += bl * atn;
			}

			// sum ambient, diffuse and emissive light and clip
			//rlt = ar * ro + Kar * ro * rlt + Kdr * ro * tndl + Ker * ro;
			rlt = ro * (ar + Kar * rlt + Kdr * tndl + Ker);
			if (rlt > 255.0f)
			{
				rlt = 255.0f;
			}

			// sum ambient, diffuse and emissive light and clip
			//glt = ag * go + Kag * go * glt + Kdg * go * tndl + Keg * go;
			glt = go * (ag + Kag * glt + Kdg * tndl + Keg);
			if (glt > 255.0f)
			{
				glt = 255.0f;
			}

			// sum ambient, diffuse and emissive light and clip
			//blt = ab * bo + Kab * bo * blt + Kdb * bo * tndl + Keb * bo;
			blt = bo * (ab + Kab * blt + Kdb * tndl + Keb);
			if (blt > 255.0f)
			{
				blt = 255.0f;
			}

			// save the vertex' light information
			mc->color = piRGBA_MAKE(FloatToInt(rlt), FloatToInt(glt), FloatToInt(blt), FloatToInt(ao));
			if (lpobj->flags & OBJECT_NOSPECULAR)
			{
				mc->specular = ic->specular;
			}
			else
			{
				sr = Ksr * sr;
				if (sr > 255.0f)
				{
					sr = 255.0f;
				}

				sg = Ksg * sg;
				if (sg > 255.0f)
				{
					sg = 255.0f;
				}

				sb = Ksb * sb;
				if (sb > 255.0f)
				{
					sb = 255.0f;
				}

				mc->specular = colorALPHA100 | (piRGB_MAKE(FloatToInt(sr), FloatToInt(sg), FloatToInt(sb)));
			}
		}
	}

#ifdef EVENT_LOG_TIMES
    count += GetMicroSeconds() - beginhere;
    if ((++number % 256) == 0)
    {
        WriteDebug("Average static lighting time: %lu us", count);
        count = 0;
    }
#endif

	return(TRUE);
}
