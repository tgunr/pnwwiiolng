#include <syscfg.h> // Path is project specific.
//

#include <math.h>
#include <float.h>
#include "pi/pi.h"
#include "include/vars.h"
#include "ob/obcd.h"
#include "draw/draw3d.h"
#include "math/math3d.h"
#include "list/list.h"

#define CD_POLY_FENCE
//#undef CD_POLY_FENCE


#ifndef _DEBUG

#define OutlineTriangle(a, b, c)
#define HighliteView(a, b)

#else

static void OutlineTriangle(piVERTEX* pv, piINDICES* pt, DWORD color)
{
  draw3dLine((piVECTOR*)&pv[pt->v1], (piVECTOR*)&pv[pt->v2], color);
  draw3dLine((piVECTOR*)&pv[pt->v2], (piVECTOR*)&pv[pt->v3], color);
  draw3dLine((piVECTOR*)&pv[pt->v3], (piVECTOR*)&pv[pt->v1], color);
}

static void HighliteView(piVECTOR* pv, DWORD color)
{
  draw3dPoint(pv, color);
}

#endif


LOGICAL obcdCheckObjectCollision(OBJECT* lpobj, COLLISION* collision)
{
LOGICAL        retval=FALSE;
int         i, numtris;
float       d, d1, d2, d3, eps;
piVECTOR		tp, dir;
piVECTOR  pv1, pv2, pv3;
piVECTOR    m, q;
piVECTOR*	op;
piVECTOR*	np;
piVECTOR const* pn;
piVECTOR const*	pcn;

piINDICES*	pt;
piVECTOR*  lpcrnr;

#ifdef EVENT_LOG_TIMES
    static UINT64 count = 0;
    static int number = 0, npolys = 0;

    UINT64 beginhere = GetMicroSeconds();
#endif


	op = &collision->oldpos;
	np = &collision->newpos;

	math3dSubtractVectors(&dir, np, op);
	math3dNormalize(&dir);

	numtris = lpobj->numtris;
	piVectorConstPtr pos = lpobj->RenderPositions();

	pt      = lpobj->ptris;
	pn      = lpobj->psnorms;
	pcn     = lpobj->pscdn;
	lpcrnr  = lpobj->bbox;

	eps = FLT_EPSILON * 16.0f;
	for (i=0; i < numtris; i++, pt++, pn++, pcn+=3)
	{
		d = math3dDotProduct(&dir, pn);
		if (d >= 0.0f)
		{
			continue;
		}

#ifdef EVENT_LOG_TIMES
		npolys++;
#endif

		d1 = math3dDotProduct(&pos[pt->v1], pn);
		d2 = math3dDotProduct(op, pn);
		d3 = math3dDotProduct(np, pn);

		if (d2 <= (d1 + eps) && d2 >= (d1 - eps) && d3 <= d1)
		//if (d2 == d1 && d3 <= d1)// && (d1 - d3) > ScaleFactor)
		{
			//OutlineTriangle(pv, pt, colorWHITEALPHA);
			//HighliteView(&tp, colorWHITEALPHA);
			collision->colobj = lpobj;
			collision->colpoly = i;
			collision->colpos = *op;
			collision->colnormal = *pn;
			collision->colangle = d;
			retval = TRUE;
			break;
		}

		if (d2 >= d1 && d3 <= d1)
		{
			pv1 = pos[pt->v1];
			pv2 = pos[pt->v2];
			pv3 = pos[pt->v3];

			// create a vertex where the vector crosses the poly plane
			math3dScaleVector(&m, &dir, 1.0f / -d);
			math3dScaleVector(&q, &m, d2 - d1);
			math3dAddVectors(&tp, op, &q);

#ifdef CD_POLY_FENCE
			// polygon fence algorithm
			// used to determine whether the point is within the polygon bounds
			{
			float       da, db;

				da = math3dDotProduct(&pcn[0], &pv1);
				db = math3dDotProduct(&pcn[0], &tp);
				if (db > da)
				{
					continue;
				}

				da = math3dDotProduct(&pcn[1], &pv2);
				db = math3dDotProduct(&pcn[1], &tp);
				if (db > da)
				{
					continue;
				}

				da = math3dDotProduct(&pcn[2], &pv3);
				db = math3dDotProduct(&pcn[2], &tp);
				if (db > da)
				{
					continue;
				}
			}
#else
			// sum of the angles algorithm
			// used to determine whether the point is within the polygon bounds
			{
				piVECTOR		w1, w2, w3;

				math3dSubtractVectors(&w1, pv1, &tp);
				math3dSubtractVectors(&w2, pv2, &tp);
				math3dSubtractVectors(&w3, pv3, &tp);

				math3dNormalize(&w1);
				math3dNormalize(&w2);
				math3dNormalize(&w3);

				d1 = math3dDotProduct(&w1, &w2);
				d2 = math3dDotProduct(&w1, &w3);
				d3 = math3dDotProduct(&w2, &w3);
			}

			if (d1 + d2 + d3 <= -1.0f)
#endif
			{
				//OutlineTriangle(pv, pt, colorCYANALPHA);
				//HighliteView(&tp, colorCYANALPHA);
				collision->colobj = lpobj;
				collision->colpoly = i;
				collision->colpos = tp;
				collision->colnormal = *pn;
				collision->colangle = d;
				retval = TRUE;
				break;
			}
		}
	}

#ifdef EVENT_LOG_TIMES
    count += GetMicroSeconds() - beginhere;
    if ((++number % 1024) == 0)
    {
        Write_Debug("Average object polygon collision detect time: %lu us, polys = %d", count, npolys / 1024);
        number = 0;
        count = 0;
        npolys = 0;
    }
#endif

	return(retval);
}

LOGICAL obcdCheck(COLLISION* collision)
{
LOGICAL        retval=FALSE, indirection;
int         i, j, k, numobjects, numverts;
float       x, y, z, xo, yo, zo, xn, yn, zn, xc, yc, zc, d, dp;
float       mnx, mny, mnz, mxx, mxy, mxz;
piVECTOR    mn, mx;
OBJECT* object, *objects, *lpobj;
piVECTOR*  op;
piVECTOR*	np;
piVECTOR*	lpcrnr;
piVECTOR    dir, vt, pv;
LISTHEADER*	lplist;

#ifdef EVENT_LOG_TIMES
    static UINT64 count = 0;
    static int number = 0;

    UINT64 beginhere = GetMicroSeconds();
#endif


	object  = collision->object;
	op      = &collision->oldpos;
	np      = &collision->newpos;

	// cannot collide if standing still
	if (op->x == np->x &&
		op->y == np->y &&
		op->z == np->z)
	{
		return(FALSE);
	}

	math3dSubtractVectors(&dir, np, op);
	math3dNormalize(&dir);

	xo = op->x;
	yo = op->y;
	zo = op->z;
	xn = np->x;
	yn = np->y;
	zn = np->z;

	numobjects = obmainGetObjects(&lplist);
	if (numobjects < 0)
	{
		return(FALSE);
	}

	objects = (OBJECT*)lplist->next;
	for (lpobj=objects; lpobj != nullptr; lpobj=(OBJECT*)lpobj->next)
	{
		if (object != nullptr && lpobj != object)
		{
			continue;
		}

		if (!(lpobj->flags & OBJECT_COLLISION))
		{
			continue;
		}

		if (lpobj->flags & OBJECT_TERRAIN)
		{
			continue;
		}

		// center of this object
		xc = lpobj->pos.x;
		yc = lpobj->pos.y;
		zc = lpobj->pos.z;

		// very simple in-the-direction-of-movement check
		indirection = FALSE;
		if (lpobj->numverts <= 8)
		{
			piVectorConstPtr    pos = lpobj->RenderPositions();
			numverts  = lpobj->numverts;
			for (i=0; i < numverts; i++, ++pos)
			{
				math3dSubtractVectors(&vt, pos.ptr, op);
				dp = math3dDotProduct(&dir, &vt);
				if (dp > 0.0f)
				{
					indirection = TRUE;
					break;
				}
			}
		}
		else
		{
			lpcrnr = lpobj->bbox;
			for (i=0; i < 2 && !indirection; i++)
			{
				vt.x = xc + lpcrnr[i].x;
				for (j=0; j < 2 && !indirection; j++)
				{
					vt.y = yc + lpcrnr[j].y;
					for (k=0; k < 2; k++)
					{
						vt.z = zc + lpcrnr[k].z;
						math3dSubtractVectors(&pv, &vt, op);
						dp = math3dDotProduct(&dir, &pv);
						if (dp > 0.0f)
						{
							indirection = TRUE;
							break;
						}
					}
				}
			}
		}

		if (!indirection)
		{
			continue;
		}

		// FIX ME! This can save a bunch of time;
		// at the very edge of an object, dp goes negative
		// or the distance becomes too great (it shouldn't)
		if (0)
		{
			// Reject if path (a line) is completely
			// outside radius from center of object
			vt.x = x = xc - xo;
			vt.y = y = yc - yo;
			vt.z = z = zc - zo;
			math3dNormalize(&vt);

			d = x * x + y * y + z * z;
			dp = math3dDotProduct(&vt, &dir);
			if (dp <= 0.0f)
			{
				continue;
			}

			dp *= d;
			d = (float)sqrt(dp);	// extract sine from hypotenus and cosine

			x = xc - (dir.x * d + xo);
			y = yc - (dir.y * d + yo);
			z = zc - (dir.z * d + zo);

			d = x * x + y * y + z * z;
			if (d > lpobj->maxrad)
			{
				continue;
			}
		}

		// If inside bounding box, evaluate each polygon
		mn = lpobj->bbox[0];
		mnx = mn.x + xc;
		mny = mn.y + yc;
		mnz = mn.z + zc;
		mx = lpobj->bbox[1];
		mxx = mx.x + xc;
		mxy = mx.y + yc;
		mxz = mx.z + zc;

		if ((mnx <= xn &&
			mxx >= xn) ||
			(mnx <= xo &&
			mxx >= xo))
		{
			if ((mny <= yn &&
				mxy >= yn) ||
				(mny <= yo &&
				mxy >= yo))
			{
				if ((mnz <= zn &&
					mxz >= zn) ||
					(mnz <= zo &&
					mxz >= zo))
				{
					//Write_Debug("Non-flat BBox: %s", lpobj->name);
					//obdrawWireframeObject(lpobj, colorWHITEALPHA);
					if (obcdCheckObjectCollision(lpobj, collision))
					{
						retval = TRUE;
						break;
					}

					continue;
				}
			}
		}

		if ((mnx <= xn &&
		mxx >= xo) ||
		(mnx <= xo &&
		mxx >= xn))
		{
			if ((mny <= yn &&
				mxy >= yo) ||
				(mny <= yo &&
				mxy >= yn))
			{
				if ((mnz <= zn &&
					mxz >= zo) ||
					(mnz <= zo &&
					mxz >= zn))
				{
					//Write_Debug("Flat BBox: %s", lpobj->name);
					//obdrawWireframeObject(lpobj, colorREDALPHA);
					if (obcdCheckObjectCollision(lpobj, collision))
					{
						retval = TRUE;
						break;
					}

					continue;
				}
			}
		}

		continue;
	}

#ifdef EVENT_LOG_TIMES
    count += GetMicroSeconds() - beginhere;
    if ((++number % 1024) == 0)
    {
        Write_Debug("Average object collision detect time: %lu us", count);
        count = 0;
    }
#endif

	return(retval);
}

LOGICAL obcdPlayer(piVECTOR* oldpos, piVECTOR* newpos)
{
LOGICAL      retval;
COLLISION collision;

	memset(&collision, 0, sizeof(collision));
	collision.oldpos.x = -oldpos->x;
	collision.oldpos.y = -oldpos->y;
	collision.oldpos.z = -oldpos->z;
	collision.newpos.x = -newpos->x;
	collision.newpos.y = -newpos->y;
	collision.newpos.z = -newpos->z;

	retval = obcdCheck(&collision);

	if (retval)
	{
		newpos->x = -collision.colpos.x;
		newpos->y = -collision.colpos.y;
		newpos->z = -collision.colpos.z;
	}
	else
	{
		newpos->x = -collision.newpos.x;
		newpos->y = -collision.newpos.y;
		newpos->z = -collision.newpos.z;
	}

	return(retval);
}

LOGICAL obcdObject(OBJECT* object, piVECTOR* oldpos, piVECTOR* newpos)
{
LOGICAL      retval;
COLLISION collision;

	memset(&collision, 0, sizeof(collision));
	collision.object   = object;
	collision.oldpos.x = oldpos->x;
	collision.oldpos.y = oldpos->y;
	collision.oldpos.z = oldpos->z;
	collision.newpos.x = newpos->x;
	collision.newpos.y = newpos->y;
	collision.newpos.z = newpos->z;

	retval = obcdCheck(&collision);

	if (retval)
	{
		newpos->x = collision.colpos.x;
		newpos->y = collision.colpos.y;
		newpos->z = collision.colpos.z;
	}
	else
	{
		newpos->x = collision.newpos.x;
		newpos->y = collision.newpos.y;
		newpos->z = collision.newpos.z;
	}

	return(retval);
}
