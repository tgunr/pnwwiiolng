#include <syscfg.h> // Path is project specific.
//

#include <math.h>
#include <float.h>
#include <string.h>
#include "pi/pi.h"
#include "include/vars.h"
#include "mem/mem.h"
#include "ld/ldmdl.h"
#include "fio/fio.h"
#include "math/math3d.h"
#include "math/mathvect.h"
#include "ob/obc.h"
#include "pers/persmain.h"
#include "math/mathUnitConversion.h"
#include "math/SuperFastHash.h"
#include <assert.h>
#include <algorithm>
#include <deque>
#include "ogl/oglMesh.h"

template <typename T>
static inline void memRelease(T *&p) { memFree(reinterpret_cast<void *>(p)); p = nullptr; }

static bool	MergeObjectsIfPossible(OBJECT* main, OBJECT* o);
static void		ReduceBranch(OBJECT* node, OBJECT* branchtop);
static void		MergeObjectWithBranch(OBJECT* o, OBJECT* node);
static LOGICAL	IsMergingBranchTopNode(OBJECT* node);
static LOGICAL	IsMergableBranchTopNode(OBJECT* node);

static bool _reduceModel = false;

typedef struct normalsListItem
{
	normalsListItem*	next;
	OBJECT*		lpobj;
	OBJECT*		lpatt;
	SINT32			vcnt;

} *lpnormalsListItem;

normalsListItem*		normalsList;

// These functions differ from those in math3d in that they do not return a value.
// Further, some additional* attempted* optimizations were added to try to help
// the compiler create less assembly to implement the functions. With some limited
// testing, this has shown to be the case.

inline void Normalize(piVECTOR* lpa)
{
    float inv_mod = lpa->x * lpa->x + lpa->y * lpa->y + lpa->z * lpa->z;

	if ( inv_mod > 0 )
	{
		lpa->x *= (inv_mod = (float)(1.0 / sqrt(inv_mod)));
		lpa->y *= inv_mod;
		lpa->z *= inv_mod;
	}
}

// This function differs from the one in math3d in that all three parameters must
// be unique variables. Parameter lpd cannot be either of lpa or lpb.
inline void CrossProduct(piVECTOR* lpd, piVECTOR* lpa, piVECTOR* lpb)
{
	lpd->x = lpa->y * lpb->z - lpa->z * lpb->y;
	lpd->y = lpa->z * lpb->x - lpa->x * lpb->z;
	lpd->z = lpa->x * lpb->y - lpa->y * lpb->x;
}

static LOGICAL _UpdateGroupFilteringAndBlendStatus(OBJECT * ObjGrp);

// get the bounding elements
LOGICAL ldmdlGetBounds(OBJECT* lpobj)
{
	float		xmn, xmx;
	float		ymn, ymx;
	float		zmn, zmx;
	float       ax, ay, az;

#ifdef SPECIAL_LOG
__int64		rdcount, rdcount1, rdcount2;

	rdcount1 = piReadTimeStamp64();
#endif

	if (lpobj == nullptr)
	{
		return(FALSE);
	}

	if (lpobj->flags & OBJECT_NOTOBJECT)
	{
		return(TRUE);
	}

	if(!lpobj->numverts)
	{
		lpobj->pos.x = 0.0f;
		lpobj->pos.y = 0.0f;
		lpobj->pos.z = 0.0f;

		lpobj->bbox[0].x = 0.0f;
		lpobj->bbox[0].y = 0.0f;
		lpobj->bbox[0].z = 0.0f;

		lpobj->bbox[1].x = 0.0f;
		lpobj->bbox[1].y = 0.0f;
		lpobj->bbox[1].z = 0.0f;

		lpobj->minrad = 0.0f;
		lpobj->maxrad = 0.0f;

		return(TRUE);
	}

	piVectorConstPtr rpos = lpobj->RenderPositions();

	xmx = ymx = zmx = -FLT_MAX;
	xmn = ymn = zmn = FLT_MAX;

	for (int i=0; i < lpobj->numverts; i++)
	{
		if (xmn > rpos->x)
		{
			xmn = rpos->x;
		}

		if (xmx < rpos->x)
		{
			xmx = rpos->x;
		}

		if (ymn > rpos->y)
		{
			ymn = rpos->y;
		}

		if (ymx < rpos->y)
		{
			ymx = rpos->y;
		}

		if (zmn > rpos->z)
		{
			zmn = rpos->z;
		}

		if (zmx < rpos->z)
		{
			zmx = rpos->z;
		}

		rpos++;
	}

	// center
	ax = xmx - (xmx - xmn) * 0.5f;
	ay = ymx - (ymx - ymn) * 0.5f;
	az = zmx - (zmx - zmn) * 0.5f;

	lpobj->pos.x = ax;
	lpobj->pos.y = ay;
	lpobj->pos.z = az;

	xmn -= ax;
	ymn -= ay;
	zmn -= az;

	xmx -= ax;
	ymx -= ay;
	zmx -= az;

	// bounding box
	lpobj->bbox[0].x = xmn;
	lpobj->bbox[0].y = ymn;
	lpobj->bbox[0].z = zmn;

	lpobj->bbox[1].x = xmx;
	lpobj->bbox[1].y = ymx;
	lpobj->bbox[1].z = zmx;

	lpobj->minrad = xmn * xmn + ymn * ymn + zmn * zmn;
	lpobj->maxrad = xmx * xmx + ymx * ymx + zmx * zmx;

#ifdef SPECIAL_LOG
	rdcount2 = piReadTimeStamp64();
	rdcount = rdcount2 - rdcount1;
	Write_Debug("Cycles used for ldmdlGetBounds on object '%s' was %u", lpobj->name, rdcount);
#endif

	return(TRUE);
}


// get the object bounds
LOGICAL ldmdlGetObjectBounds(OBJECT* lpmain, piVECTOR* loc, piVECTOR* obox, float* minrad, float* maxrad)
{
int			i, num;
//float		mn=FLT_MAX, mx=-FLT_MAX;
float		xmn, xmx;
float		ymn, ymx;
float		zmn, zmx;
float       ax, ay, az;
OBJECT*	lpobj;

#ifdef SPECIAL_LOG
__int64		rdcount, rdcount1, rdcount2;

	rdcount1 = piReadTimeStamp64();
#endif

	if (lpmain == nullptr)
	{
		return(FALSE);
	}

	lpobj = lpmain;
	xmn = ymn = zmn = FLT_MAX;
	xmx = ymx = zmx = -FLT_MAX;
	while (lpobj)
	{
		if (!(lpobj->flags & OBJECT_NOTOBJECT))
		{
			num = lpobj->numverts;
			if (num > 0)
			{
				piVectorConstPtr rpos = lpobj->RenderPositions();
				for (i=0; i < num; i++, ++rpos)
				{
					if (xmn > rpos->x)
					{
						xmn = rpos->x;
					}

					if (xmx < rpos->x)
					{
						xmx = rpos->x;
					}

					if (ymn > rpos->y)
					{
						ymn = rpos->y;
					}

					if (ymx < rpos->y)
					{
						ymx = rpos->y;
					}

					if (zmn > rpos->z)
					{
						zmn = rpos->z;
					}

					if (zmx < rpos->z)
					{
						zmx = rpos->z;
					}
				}
			}
		}
		lpobj = lpobj->attached;
	}

	// center
	ax = xmx - (xmx - xmn) * 0.5f;
	ay = ymx - (ymx - ymn) * 0.5f;
	az = zmx - (zmx - zmn) * 0.5f;

	if (loc != nullptr)
	{
		loc->x = ax;
		loc->y = ay;
		loc->z = az;
	}

	xmn -= ax;
	ymn -= ay;
	zmn -= az;

	xmx -= ax;
	ymx -= ay;
	zmx -= az;

	// bounding box
	if (obox != nullptr)
	{
		obox[0].x = xmn;
		obox[0].y = ymn;
		obox[0].z = zmn;

		obox[1].x = xmx;
		obox[1].y = ymx;
		obox[1].z = zmx;
	}

	// min and max radii
	if (minrad != nullptr)
	{
		*minrad = xmn * xmn + ymn * ymn + zmn * zmn;
	}

	if (maxrad != nullptr)
	{
		*maxrad = xmx * xmx + ymx * ymx + zmx * zmx;
	}

#ifdef SPECIAL_LOG
	rdcount2 = piReadTimeStamp64();
	rdcount = rdcount2 - rdcount1;
	Write_Debug("Cycles used for ldmdlGetObjectBounds on object '%s' was %u", lpmain->name, rdcount);
#endif

	return(TRUE);
}


// update the object center locations
LOGICAL ldmdlUpdateLocation(OBJECT* lpobj)
{
piVECTOR		loc;

	if (lpobj == nullptr)
	{
		return(FALSE);
	}

	loc = lpobj->loc;

	do
	{
		lpobj->loc = loc;
		lpobj = lpobj->attached;
	}
	while (lpobj);

	return(TRUE);
}

// calculate the surface normals for the face list
LOGICAL ldmdlCalcSurfaceNormals(OBJECT* lpobj)
{
UINT16			fst, sec, thd;
int				i, num;
piVECTOR		a, w1, w2, w3;
piVECTOR		pvn;
piVECTOR*		pscdn;
piVECTOR*		pscdon;
piINDICES*		pt;

#ifdef SPECIAL_LOG
__int64		rdcount, rdcount1, rdcount2;

	rdcount1 = piReadTimeStamp64();
#endif

	piVectorConstPtr rpos = lpobj->RenderPositions();
	pt = lpobj->ptris;

	pscdon = lpobj->pscdon;
	pscdn  = lpobj->pscdn;

	num = lpobj->numtris;
	for (i=0; i < num; i++, pt++)
	{
		// retrieve all three vectors numbers
		fst = pt->v1;
		sec = pt->v2;
		thd = pt->v3;

		w1.x = rpos[sec].x - rpos[fst].x;
		w1.y = rpos[sec].y - rpos[fst].y;
		w1.z = rpos[sec].z - rpos[fst].z;

		w2.x = rpos[thd].x - rpos[fst].x;
		w2.y = rpos[thd].y - rpos[fst].y;
		w2.z = rpos[thd].z - rpos[fst].z;

		CrossProduct(&a, &w1, &w2);
		Normalize(&a);

		// save the surface normal
		lpobj->psnorms[i] = a;
		lpobj->psnorig[i] = a;

		w2.x = rpos[thd].x - rpos[sec].x;
		w2.y = rpos[thd].y - rpos[sec].y;
		w2.z = rpos[thd].z - rpos[sec].z;

		w3.x = rpos[fst].x - rpos[thd].x;
		w3.y = rpos[fst].y - rpos[thd].y;
		w3.z = rpos[fst].z - rpos[thd].z;

		pvn.x = (rpos[sec].x + a.x) - rpos[fst].x;
		pvn.y = (rpos[sec].y + a.y) - rpos[fst].y;
		pvn.z = (rpos[sec].z + a.z) - rpos[fst].z;
		CrossProduct(pscdon, &w1, &pvn);
		Normalize(pscdon);
		*pscdn++ = *pscdon++;

		pvn.x = (rpos[thd].x + a.x) - rpos[sec].x;
		pvn.y = (rpos[thd].y + a.y) - rpos[sec].y;
		pvn.z = (rpos[thd].z + a.z) - rpos[sec].z;
		CrossProduct(pscdon, &w2, &pvn);
		Normalize(pscdon);
		*pscdn++ = *pscdon++;

		pvn.x = (rpos[fst].x + a.x) - rpos[thd].x;
		pvn.y = (rpos[fst].y + a.y) - rpos[thd].y;
		pvn.z = (rpos[fst].z + a.z) - rpos[thd].z;
		CrossProduct(pscdon, &w3, &pvn);
		Normalize(pscdon);
		*pscdn++ = *pscdon++;
	}

#ifdef SPECIAL_LOG
	rdcount2 = piReadTimeStamp64();
	rdcount = rdcount2 - rdcount1;
	Write_Debug("Cycles used for ldmdlCalcSurfaceNormals on object '%s' was %u", lpobj->name, rdcount);
#endif

	return(TRUE);
}


// calculate the vertex normals for the face list
LOGICAL ldmdlCalcCollisionNormals(OBJECT* lpobj)
{
UINT16			fst, sec, thd;
int				i, num;
piVECTOR		a, w1, w2, w3;
piVECTOR		pvn;
piVECTOR*		pscdn;
piVECTOR*		pscdon;
piINDICES*		pt;

#ifdef SPECIAL_LOG
__int64		rdcount, rdcount1, rdcount2;

	rdcount1 = piReadTimeStamp64();
#endif

	piVectorConstPtr rpos = lpobj->RenderPositions();
	pt = lpobj->ptris;

	if(!lpobj->numtris)	return(TRUE);

	if(!lpobj->pscdon)
	{
		lpobj->pscdon = (piVECTOR*)memAlloc(lpobj->numtris * 3 * sizeof(piVECTOR));
		lpobj->pscdn = lpobj->pscdon;
	}
	if(!lpobj->psnorms)
	{
		lpobj->psnorms = (piVECTOR*)memAlloc(lpobj->numtris * sizeof(piVECTOR));
		lpobj->psnorig = lpobj->psnorms;
	}
	pscdon = lpobj->pscdon;
	pscdn  = lpobj->pscdn;

	num = lpobj->numtris;
	for (i=0; i < num; i++, pt++)
	{
		// retrieve all three vectors numbers
		fst = pt->v1;
		sec = pt->v2;
		thd = pt->v3;

		w1.x = rpos[sec].x - rpos[fst].x;
		w1.y = rpos[sec].y - rpos[fst].y;
		w1.z = rpos[sec].z - rpos[fst].z;

		w2.x = rpos[thd].x - rpos[sec].x;
		w2.y = rpos[thd].y - rpos[sec].y;
		w2.z = rpos[thd].z - rpos[sec].z;

		CrossProduct(&a, &w1, &w2);
		Normalize(&a);
		lpobj->psnorms[i] = a;

		w3.x = rpos[fst].x - rpos[thd].x;
		w3.y = rpos[fst].y - rpos[thd].y;
		w3.z = rpos[fst].z - rpos[thd].z;

		pvn.x = (rpos[sec].x + a.x) - rpos[fst].x;
		pvn.y = (rpos[sec].y + a.y) - rpos[fst].y;
		pvn.z = (rpos[sec].z + a.z) - rpos[fst].z;
		CrossProduct(pscdon, &w1, &pvn);
		Normalize(pscdon);
		*pscdn++ = *pscdon++;

		pvn.x = (rpos[thd].x + a.x) - rpos[sec].x;
		pvn.y = (rpos[thd].y + a.y) - rpos[sec].y;
		pvn.z = (rpos[thd].z + a.z) - rpos[sec].z;
		CrossProduct(pscdon, &w2, &pvn);
		Normalize(pscdon);
		*pscdn++ = *pscdon++;

		pvn.x = (rpos[fst].x + a.x) - rpos[thd].x;
		pvn.y = (rpos[fst].y + a.y) - rpos[thd].y;
		pvn.z = (rpos[fst].z + a.z) - rpos[thd].z;
		CrossProduct(pscdon, &w3, &pvn);
		Normalize(pscdon);
		*pscdn++ = *pscdon++;
	}

#ifdef SPECIAL_LOG
	rdcount2 = piReadTimeStamp64();
	rdcount = rdcount2 - rdcount1;
	Write_Debug("Cycles used for ldmdlCalcCollisionNormals on object '%s' was %u", lpobj->name, rdcount);
#endif

	return(TRUE);
}

/*

LOGICAL ldmdlCalcVertexNormals(OBJECT* lpobj)
{
UINT16			fst, sec, thd;
int				i, num;
piVECTOR		a;
piINDICES*		pt;

#ifdef SPECIAL_LOG
__int64		rdcount, rdcount1, rdcount2;

	rdcount1 = piReadTimeStamp64();
#endif

	piVectorPtr mnorm = lpobj->MutableNormals();

	num = lpobj->numverts;
	for (i=0; i < num; i++, ++mnorm)
	{
		mnorm->x = 0.0f;
		mnorm->y = 0.0f;
		mnorm->z = 0.0f;
	}

	pt = lpobj->ptris;

	num = lpobj->numtris;
	for (i=0; i < num; i++, pt++)
	{
		// retrieve all three vectors numbers
		fst = pt->v1;
		sec = pt->v2;
		thd = pt->v3;

		a = lpobj->psnorig[i];

		// average the vertex normals between adjacent
		// surfaces to produce better lighting
		mnorm[fst].x += a.x;
		mnorm[fst].y += a.y;
		mnorm[fst].z += a.z;

		mnorm[sec].x += a.x;
		mnorm[sec].y += a.y;
		mnorm[sec].z += a.z;

		mnorm[thd].x += a.x;
		mnorm[thd].y += a.y;
		mnorm[thd].z += a.z;
	}

	num = lpobj->numverts;
	for (i=0; i < num; i++, ++mnorm)
	{
		Normalize(mnorm.ptr);
	}

#ifdef SPECIAL_LOG
	rdcount2 = piReadTimeStamp64();
	rdcount = rdcount2 - rdcount1;
	Write_Debug("Cycles used for ldmdlCalcVertexNormals on object '%s' was %u", lpobj->name, rdcount);
#endif

	return(TRUE);
}



//////////////////////////////////////////////////////////////////////
// Function: ldmdlTangentArray
// Description: Generate tangent vectors.
// Retrieve binormal from B = (N x T) * T.w (x = cross product)
// N = normal
// T = tangent vector
// This function finds T. Send T into the vertex shader. N is passed
// by semantic. Find B using cross product as above.
//////////////////////////////////////////////////////////////////////
void ldmdlTangentArray(const piVECTOR* vertex,      // IN
                       const piVECTOR* normal,      // IN
                       const piUV* texcoord,        // IN
                       const piINDICES* triangle,   // IN
                       SINT32 vertexCount,          // IN
                       SINT32 triangleCount,        // IN
                       piQUAT** tangent)            // OUT
{
    piVECTOR* tan1 = (piVECTOR*)memAlloc(sizeof(piVECTOR) * vertexCount * 2);
    piVECTOR* tan2 = tan1 + vertexCount;

    memset(tan1, 0, vertexCount * sizeof(piVECTOR) * 2);

    *tangent = (piQUAT*)memAlloc(sizeof(piQUAT) * vertexCount);

    SINT32 indice = 0;

    UINT16 i1, i2, i3;

    for(SINT32 a = 0; a < triangleCount; a++)
    {
        if(triangle)
        {
            i1 = triangle->v1;
            i2 = triangle->v2;
            i3 = triangle->v3;
        }
        else
        {
            i1 = indice++;
            i2 = indice++;
            i3 = indice++;
        }

        const piVECTOR& v1 = vertex[i1];
        const piVECTOR& v2 = vertex[i2];
        const piVECTOR& v3 = vertex[i3];

        const piUV& w1 = texcoord[i1];
        const piUV& w2 = texcoord[i2];
        const piUV& w3 = texcoord[i3];

        FLOAT32 x1 = v2.x - v1.x;
        FLOAT32 x2 = v3.x - v1.x;

        FLOAT32 y1 = v2.y - v1.y;
        FLOAT32 y2 = v3.y - v1.y;

        FLOAT32 z1 = v2.z - v1.z;
        FLOAT32 z2 = v3.z - v1.z;

        FLOAT32 s1 = w2.u - w1.u;
        FLOAT32 s2 = w3.u - w1.u;

        FLOAT32 t1 = w2.v - w1.v;
        FLOAT32 t2 = w3.v - w1.v;

        FLOAT32 f = s1 * t2 - s2 * t1;

        if(f == 0)
        {
            triangle++;
            continue;
        }

        FLOAT32 r = 1.0f / f;

        piVECTOR sdir =
        {
            (t2 * x1 - t1 * x2) * r,
            (t2 * y1 - t1 * y2) * r,
            (t2 * z1 - t1 * z2) * r
        };

        piVECTOR tdir =
        {
            (s1 * x2 - s2 * x1) * r,
            (s1 * y2 - s2 * y1) * r,
            (s1 * z2 - s2 * z1) * r
        };

        mathAddVectors(&tan1[i1], &tan1[i1], &sdir);
        mathAddVectors(&tan1[i2], &tan1[i2], &sdir);
        mathAddVectors(&tan1[i3], &tan1[i3], &sdir);

        mathAddVectors(&tan2[i1], &tan2[i1], &tdir);
        mathAddVectors(&tan2[i2], &tan2[i2], &tdir);
        mathAddVectors(&tan2[i3], &tan2[i3], &tdir);

        triangle++;
    }

    for(SINT32 a = 0; a < vertexCount; a++)
    {
        const piVECTOR& n = normal[a];
        const piVECTOR& t = tan1[a];

        piVECTOR r;
        piVECTOR scale;

        FLOAT32 dot = mathVectDotProduct(&n, &t);
        mathScaleVector(&scale, &n, dot);
        mathSubVectors(&r, &t, &scale);

        math3dNormalize(&r);

        (*tangent)[a].x = r.x;
        (*tangent)[a].y = r.y;
        (*tangent)[a].z = r.z;

        math3dCrossProduct(&r, (piVECTOR*)&n, (piVECTOR*)&t);

       (*tangent)[a].w = math3dDotProduct(&r, &tan2[a]) < 0 ? -1.0f : 1.0f;
    }

    memFree(tan1);
}


//////////////////////////////////////////////////////////////////////
// Function: ldmdlTangentArray3
// Description: Generate tangent vectors.
// Retrieve binormal from B = (N x T) * T.w (x = cross product)
// N = normal
// T = tangent vector
// This function finds T. Send T into the vertex shader. N is passed
// by semantic. Find B using cross product as above.
//////////////////////////////////////////////////////////////////////
piQUAT* ldmdlTangentArray3(OBJECT* o)
{
    SINT32		numverts = o->numverts;
    piVECTOR*	tan1 = (piVECTOR*)memAlloc(sizeof(piVECTOR) * numverts * 2);
    piVECTOR*	tan2 = tan1 + numverts;
    piQUAT*		tangentData = (piQUAT*)memAlloc(sizeof(piQUAT) * numverts);


    memset(tan1, 0, numverts * sizeof(piVECTOR) * 2);

    UINT16 i1, i2, i3;

    SINT32 indice = 0;

    piVECTOR v1;
    piVECTOR v2;
    piVECTOR v3;

    piUV w1;
    piUV w2;
    piUV w3;

    piVectorConstPtr pos = o->RenderPositions();
    piTexCoordConstPtr tex = o->RenderTexCoords();

    for(SINT32 a = 0; a < o->numtris; a++)
    {
        if(o->ptris)
        {
            i1 = o->ptris[a].v1;
            i2 = o->ptris[a].v2;
            i3 = o->ptris[a].v3;
        }
        else
        {
            i1 = indice++;
            i2 = indice++;
            i3 = indice++;
        }

        v1.x = pos[i1].x;
        v1.y = pos[i1].y;
        v1.z = pos[i1].z;

        v2.x = pos[i2].x;
        v2.y = pos[i2].y;
        v2.z = pos[i2].z;

        v3.x = pos[i3].x;
        v3.y = pos[i3].y;
        v3.z = pos[i3].z;

        w1.u = tex[i1].u;
        w1.v = tex[i1].v;

        w2.u = tex[i2].u;
        w2.v = tex[i2].v;

        w3.u = tex[i3].u;
        w3.v = tex[i3].v;

        FLOAT32 x1 = v2.x - v1.x;
        FLOAT32 x2 = v3.x - v1.x;

        FLOAT32 y1 = v2.y - v1.y;
        FLOAT32 y2 = v3.y - v1.y;

        FLOAT32 z1 = v2.z - v1.z;
        FLOAT32 z2 = v3.z - v1.z;

        FLOAT32 s1 = w2.u - w1.u;
        FLOAT32 s2 = w3.u - w1.u;

        FLOAT32 t1 = w2.v - w1.v;
        FLOAT32 t2 = w3.v - w1.v;

        FLOAT32 f = s1 * t2 - s2 * t1;

        if(f == 0)
        {
            continue;
        }

        FLOAT32 r = 1.0f / f;

        piVECTOR sdir =
        {
            (t2 * x1 - t1 * x2) * r,
            (t2 * y1 - t1 * y2) * r,
            (t2 * z1 - t1 * z2) * r
        };

        piVECTOR tdir =
        {
            (s1 * x2 - s2 * x1) * r,
            (s1 * y2 - s2 * y1) * r,
            (s1 * z2 - s2 * z1) * r
        };

        mathAddVectors(&tan1[i1], &tan1[i1], &sdir);
        mathAddVectors(&tan1[i2], &tan1[i2], &sdir);
        mathAddVectors(&tan1[i3], &tan1[i3], &sdir);

        mathAddVectors(&tan2[i1], &tan2[i1], &tdir);
        mathAddVectors(&tan2[i2], &tan2[i2], &tdir);
        mathAddVectors(&tan2[i3], &tan2[i3], &tdir);
    }

    piVECTOR n;

    piVectorConstPtr norms = o->RenderNormals();

    for(SINT32 a = 0; a < numverts; a++)
    {
        n.x = norms[a].x;
        n.y = norms[a].y;
        n.z = norms[a].z;

        const piVECTOR& t = tan1[a];

        piVECTOR r;

        FLOAT32 dot = mathVectDotProduct(&n, &t);
        mathScaleVector(&n, &n, dot);
        mathSubVectors(&r, &t, &n);

        math3dNormalize(&r);

        tangentData[a].x = r.x;
        tangentData[a].y = r.y;
        tangentData[a].z = r.z;

        // Check if B = N x T
        math3dCrossProduct(&r, (piVECTOR*)&n, (piVECTOR*)&t);

        tangentData[a].w = math3dDotProduct(&r, &tan2[a]) < 0 ? -1.0f : 1.0f;
    }

    memFree(tan1);

	return tangentData;
}

//////////////////////////////////////////////////////////////////////
// Function: ldmdlTangentArray2
// Description: Generate tangent vectors.
// Retrieve binormal from B = (N x T) * T.w (x = cross product)
// N = normal
// T = tangent vector
// This function finds T. Send T into the vertex shader. N is passed
// by semantic. Find B using cross product as above.
//////////////////////////////////////////////////////////////////////
void ldmdlTangentArray2(const piCVERTEX* vertex,       // IN
                        SINT32 vertexCount,             // IN
                        SINT32 triangleCount,           // IN
                        piQUAT** tangent)               // OUT
{
    piVECTOR* tan1 = (piVECTOR*)memAlloc(sizeof(piVECTOR) * vertexCount * 2);
    piVECTOR* tan2 = tan1 + vertexCount;

    *tangent = (piQUAT*)memAlloc(sizeof(piQUAT) * vertexCount);

    memset(tan1, 0, vertexCount * sizeof(piVECTOR) * 2);

    SINT32 indice = 0;

    UINT16 i1, i2, i3;

    piVECTOR v1;
    piVECTOR v2;
    piVECTOR v3;

    piUV w1;
    piUV w2;
    piUV w3;

    for(SINT32 a = 0; a < triangleCount; a++)
    {
        i1 = indice++;
        i2 = indice++;
        i3 = indice++;

        v1.x = vertex[i1].x;
        v1.y = vertex[i1].y;
        v1.z = vertex[i1].z;

        v2.x = vertex[i2].x;
        v2.y = vertex[i2].y;
        v2.z = vertex[i2].z;

        v3.x = vertex[i3].x;
        v3.y = vertex[i3].y;
        v3.z = vertex[i3].z;

        w1.u = vertex[i1].tu;
        w1.v = vertex[i1].tv;

        w2.u = vertex[i2].tu;
        w2.v = vertex[i2].tv;

        w3.u = vertex[i3].tu;
        w3.v = vertex[i3].tv;

        FLOAT32 x1 = v2.x - v1.x;
        FLOAT32 x2 = v3.x - v1.x;

        FLOAT32 y1 = v2.y - v1.y;
        FLOAT32 y2 = v3.y - v1.y;

        FLOAT32 z1 = v2.z - v1.z;
        FLOAT32 z2 = v3.z - v1.z;

        FLOAT32 s1 = w2.u - w1.u;
        FLOAT32 s2 = w3.u - w1.u;

        FLOAT32 t1 = w2.v - w1.v;
        FLOAT32 t2 = w3.v - w1.v;

        FLOAT32 f = s1 * t2 - s2 * t1;

        if(f == 0)
        {
            continue;
        }

        FLOAT32 r = 1.0f / f;

        piVECTOR sdir =
        {
            (t2 * x1 - t1 * x2) * r,
            (t2 * y1 - t1 * y2) * r,
            (t2 * z1 - t1 * z2) * r
        };

        piVECTOR tdir =
        {
            (s1 * x2 - s2 * x1) * r,
            (s1 * y2 - s2 * y1) * r,
            (s1 * z2 - s2 * z1) * r
        };

        mathAddVectors(&tan1[i1], &tan1[i1], &sdir);
        mathAddVectors(&tan1[i2], &tan1[i2], &sdir);
        mathAddVectors(&tan1[i3], &tan1[i3], &sdir);

        mathAddVectors(&tan2[i1], &tan2[i1], &tdir);
        mathAddVectors(&tan2[i2], &tan2[i2], &tdir);
        mathAddVectors(&tan2[i3], &tan2[i3], &tdir);
    }

    piVECTOR n;

    for(SINT32 a = 0; a < vertexCount; a++)
    {
        n.x = vertex[a].nx;
        n.y = vertex[a].ny;
        n.z = vertex[a].nz;

        const piVECTOR& t = tan1[a];

        piVECTOR r;

        FLOAT32 dot = mathVectDotProduct(&n, &t);
        mathScaleVector(&n, &n, dot);
        mathSubVectors(&r, &t, &n);

        math3dNormalize(&r);

        (*tangent)[a].x = r.x;
        (*tangent)[a].y = r.y;
        (*tangent)[a].z = r.z;

        math3dCrossProduct(&r, (piVECTOR*)&n, (piVECTOR*)&t);

        (*tangent)[a].w = math3dDotProduct(&r, &tan2[a]) < 0 ? -1.0f : 1.0f;
    }

    memFree(tan1);
}
*/

LOGICAL ldmdlCalcNormals(OBJECT* lpobj)
{
UINT16			fst, sec, thd;
int				i, num, numv;
piVECTOR		a, w1, w2, w3;
piVECTOR		pvn;
piVECTOR*		pscdn;
piVECTOR*		pscdon;
piINDICES*		pt;

#ifdef SPECIAL_LOG
__int64		rdcount, rdcount1, rdcount2;

	rdcount1 = piReadTimeStamp64();
#endif

	piVectorConstPtr rpos = lpobj->RenderPositions();
	piVectorPtr mnorm = lpobj->StaticNormals();
	if(mnorm.ptr == nullptr)
	{
		return FALSE;
	}

	numv = lpobj->numverts;
	for (i=0; i < numv; i++, ++rpos, ++mnorm)
	{
		mnorm->x = 0.0f;
		mnorm->y = 0.0f;
		mnorm->z = 0.0f;
	}

 	rpos = lpobj->RenderPositions();
	mnorm = lpobj->StaticNormals();

	pt = lpobj->ptris;

	pscdon = lpobj->pscdon;
	pscdn  = lpobj->pscdn;

	num = lpobj->numtris;
	for (i=0; i < num; i++, pt++)
	{
		// retrieve all three vectors numbers
		fst = pt->v1;
		sec = pt->v2;
		thd = pt->v3;

		if (fst >= numv ||
			sec >= numv ||
			thd >= numv)
		{
			Write_Debug("Invalid vertex index in object!");
			return(FALSE);
		}

		w1.x = rpos[sec].x - rpos[fst].x;
		w1.y = rpos[sec].y - rpos[fst].y;
		w1.z = rpos[sec].z - rpos[fst].z;

		w2.x = rpos[thd].x - rpos[fst].x;
		w2.y = rpos[thd].y - rpos[fst].y;
		w2.z = rpos[thd].z - rpos[fst].z;

		CrossProduct(&a, &w1, &w2);
		Normalize(&a);

		// save the surface normal for collision detection
		lpobj->psnorms[i] = a;
		lpobj->psnorig[i] = a;

		w3.x = -w2.x;
		w3.y = -w2.y;
		w3.z = -w2.z;

		w2.x = rpos[thd].x - rpos[sec].x;
		w2.y = rpos[thd].y - rpos[sec].y;
		w2.z = rpos[thd].z - rpos[sec].z;

		pvn.x = (rpos[sec].x + a.x) - rpos[fst].x;
		pvn.y = (rpos[sec].y + a.y) - rpos[fst].y;
		pvn.z = (rpos[sec].z + a.z) - rpos[fst].z;
		CrossProduct(pscdon, &w1, &pvn);
		Normalize(pscdon);
		*pscdn++ = *pscdon++;

		pvn.x = (rpos[thd].x + a.x) - rpos[sec].x;
		pvn.y = (rpos[thd].y + a.y) - rpos[sec].y;
		pvn.z = (rpos[thd].z + a.z) - rpos[sec].z;
		CrossProduct(pscdon, &w2, &pvn);
		Normalize(pscdon);
		*pscdn++ = *pscdon++;

		pvn.x = (rpos[fst].x + a.x) - rpos[thd].x;
		pvn.y = (rpos[fst].y + a.y) - rpos[thd].y;
		pvn.z = (rpos[fst].z + a.z) - rpos[thd].z;
		CrossProduct(pscdon, &w3, &pvn);
		Normalize(pscdon);
		*pscdn++ = *pscdon++;

		// average the vertex normals between adjacent
		// surfaces to produce better lighting
		mnorm[fst].x += a.x;
		mnorm[fst].y += a.y;
		mnorm[fst].z += a.z;

		mnorm[sec].x += a.x;
		mnorm[sec].y += a.y;
		mnorm[sec].z += a.z;

		mnorm[thd].x += a.x;
		mnorm[thd].y += a.y;
		mnorm[thd].z += a.z;
	}

	num = lpobj->numverts;
	for (i=0; i < num; i++, ++mnorm)
	{
		Normalize(mnorm.ptr);
	}

	// We don't update the vertex data so I think this is a mistake!
	//lpobj->SyncMutableData();

#ifdef SPECIAL_LOG
	rdcount2 = piReadTimeStamp64();
	rdcount = rdcount2 - rdcount1;
	Write_Debug("Cycles used for ldmdlCalcNormals on object '%s' was %u", lpobj->name, rdcount);
#endif

	return(TRUE);
}

/*
// difference between this and ldmdlCalcNormals is that this uses
// the pdlverts instead
LOGICAL ldmdlCalcNormals1(OBJECT* lpobj)
{
UINT16			fst, sec, thd;
int				i, num, numv;
piVECTOR		a, w1, w2, w3;
piVECTOR		pvn;
piVECTOR*		pscdn;
piINDICES*		pt;

#ifdef SPECIAL_LOG
__int64		rdcount, rdcount1, rdcount2;

	rdcount1 = piReadTimeStamp64();
#endif

	piVectorConstPtr rpos = lpobj->RenderPositions();
	piVectorPtr mnorm = lpobj->MutableNormals();
	if(mnorm.ptr == nullptr)
	{
		return FALSE;
	}

	numv = lpobj->numverts;
	for (i=0; i < numv; i++, ++mnorm)
	{
		mnorm->x = 0.0f;
		mnorm->y = 0.0f;
		mnorm->z = 0.0f;
	}

	rpos = lpobj->RenderPositions();
	mnorm = lpobj->MutableNormals();

	pt = lpobj->ptris;

	obmainCreateMutableSurfaceNormals(lpobj);
	obmainCreateMutableCollisionNormals(lpobj);

	pscdn  = lpobj->pscdn;

	num = lpobj->numtris;
	for (i=0; i < num; i++, pt++)
	{
		// retrieve all three vectors numbers
		fst = pt->v1;
		sec = pt->v2;
		thd = pt->v3;

		if (fst >= numv ||
			sec >= numv ||
			thd >= numv)
		{
			Write_Debug("Invalid vertex index in object!");
			return(FALSE);
		}

		w1.x = rpos[sec].x - rpos[fst].x;
		w1.y = rpos[sec].y - rpos[fst].y;
		w1.z = rpos[sec].z - rpos[fst].z;

		w2.x = rpos[thd].x - rpos[fst].x;
		w2.y = rpos[thd].y - rpos[fst].y;
		w2.z = rpos[thd].z - rpos[fst].z;

		CrossProduct(&a, &w1, &w2);
		Normalize(&a);

		// save the surface normal for collision detection
		lpobj->psnorms[i] = a;

		w3.x = -w2.x;
		w3.y = -w2.y;
		w3.z = -w2.z;

		w2.x = rpos[thd].x - rpos[sec].x;
		w2.y = rpos[thd].y - rpos[sec].y;
		w2.z = rpos[thd].z - rpos[sec].z;

		pvn.x = (rpos[sec].x + a.x) - rpos[fst].x;
		pvn.y = (rpos[sec].y + a.y) - rpos[fst].y;
		pvn.z = (rpos[sec].z + a.z) - rpos[fst].z;
		CrossProduct(pscdn, &w1, &pvn);
		Normalize(pscdn);
		pscdn++;

		pvn.x = (rpos[thd].x + a.x) - rpos[sec].x;
		pvn.y = (rpos[thd].y + a.y) - rpos[sec].y;
		pvn.z = (rpos[thd].z + a.z) - rpos[sec].z;
		CrossProduct(pscdn, &w2, &pvn);
		Normalize(pscdn);
		pscdn++;

		pvn.x = (rpos[fst].x + a.x) - rpos[thd].x;
		pvn.y = (rpos[fst].y + a.y) - rpos[thd].y;
		pvn.z = (rpos[fst].z + a.z) - rpos[thd].z;
		CrossProduct(pscdn, &w3, &pvn);
		Normalize(pscdn);
		pscdn++;

		// average the vertex normals between adjacent
		// surfaces to produce better lighting
		mnorm[fst].x += a.x;
		mnorm[fst].y += a.y;
		mnorm[fst].z += a.z;

		mnorm[sec].x += a.x;
		mnorm[sec].y += a.y;
		mnorm[sec].z += a.z;

		mnorm[thd].x += a.x;
		mnorm[thd].y += a.y;
		mnorm[thd].z += a.z;
	}

	numv = lpobj->numverts;
	for (i=0; i < numv; i++, ++mnorm)
	{
		Normalize(mnorm.ptr);
	}

#ifdef SPECIAL_LOG
	rdcount2 = piReadTimeStamp64();
	rdcount = rdcount2 - rdcount1;
	Write_Debug("Cycles used for ldmdlCalcNormals1 on object '%s' was %u", lpobj->name, rdcount);
#endif

	return(TRUE);
}
*/

// difference between this and ldmdlCalcNormals is that this uses
// the pdlverts instead and the vertex normals are left untouched
LOGICAL ldmdlCalcNormals2(OBJECT* lpobj)
{
UINT16			fst, sec, thd;
int				i, num, numv;
piVECTOR		a, w1, w2, w3;
piVECTOR		pvn;
piVECTOR*		pscdn;
piINDICES*		pt;

#ifdef SPECIAL_LOG
__int64		rdcount, rdcount1, rdcount2;

	rdcount1 = piReadTimeStamp64();
#endif

	piVectorConstPtr rpos = lpobj->RenderPositions();
	pt = lpobj->ptris;

	obmainCreateMutableSurfaceNormals(lpobj);
	obmainCreateMutableCollisionNormals(lpobj);

	pscdn  = lpobj->pscdn;

	numv = lpobj->numverts;
	num = lpobj->numtris;
	for (i=0; i < num; i++, pt++)
	{
		// retrieve all three vectors numbers
		fst = pt->v1;
		sec = pt->v2;
		thd = pt->v3;

		if (fst >= numv ||
			sec >= numv ||
			thd >= numv)
		{
			Write_Debug("Invalid vertex index in object!");
			return(FALSE);
		}

		w1.x = rpos[sec].x - rpos[fst].x;
		w1.y = rpos[sec].y - rpos[fst].y;
		w1.z = rpos[sec].z - rpos[fst].z;

		w2.x = rpos[thd].x - rpos[fst].x;
		w2.y = rpos[thd].y - rpos[fst].y;
		w2.z = rpos[thd].z - rpos[fst].z;

		CrossProduct(&a, &w1, &w2);
		Normalize(&a);

		// save the surface normal for collision detection
		lpobj->psnorms[i] = a;

        if ( pscdn )
        {
            w3.x = -w2.x;
            w3.y = -w2.y;
            w3.z = -w2.z;

            w2.x = rpos[thd].x - rpos[sec].x;
            w2.y = rpos[thd].y - rpos[sec].y;
            w2.z = rpos[thd].z - rpos[sec].z;

            pvn.x = (rpos[sec].x + a.x) - rpos[fst].x;
            pvn.y = (rpos[sec].y + a.y) - rpos[fst].y;
            pvn.z = (rpos[sec].z + a.z) - rpos[fst].z;
            CrossProduct(pscdn, &w1, &pvn);
            Normalize(pscdn);
            pscdn++;

            pvn.x = (rpos[thd].x + a.x) - rpos[sec].x;
            pvn.y = (rpos[thd].y + a.y) - rpos[sec].y;
            pvn.z = (rpos[thd].z + a.z) - rpos[sec].z;
            CrossProduct(pscdn, &w2, &pvn);
            Normalize(pscdn);
            pscdn++;

            pvn.x = (rpos[fst].x + a.x) - rpos[thd].x;
            pvn.y = (rpos[fst].y + a.y) - rpos[thd].y;
            pvn.z = (rpos[fst].z + a.z) - rpos[thd].z;
            CrossProduct(pscdn, &w3, &pvn);
            Normalize(pscdn);
            pscdn++;
        }
	}

#ifdef SPECIAL_LOG
	rdcount2 = piReadTimeStamp64();
	rdcount = rdcount2 - rdcount1;
	Write_Debug("Cycles used for ldmdlCalcNormals2 on object '%s' was %u", lpobj->name, rdcount);
#endif

	return(TRUE);
}

// reverse the vertex normals
LOGICAL ldmdlReverseNormals(OBJECT* lpobj)
{
SINT32			i, num, v;
piVECTOR*		pso;
piVECTOR*		psn;
piINDICES*		pt;

	if (lpobj == nullptr)
	{
		return(FALSE);
	}

	piVectorPtr snorm = lpobj->StaticNormals();
	if (snorm.ptr == nullptr)
	{
		return FALSE;
	}

	num = lpobj->numverts;
	for (i=0; i < num; i++, ++snorm)
	{
		snorm->x = -snorm->x;
		snorm->y = -snorm->y;
		snorm->z = -snorm->z;
	}

	// We don't update the vertex data so I think this is a mistake!
	//lpobj->SyncMutableData();

	pt = lpobj->ptris;
	pso = lpobj->psnorig;
	psn = lpobj->psnorms;

	num = lpobj->numtris;
	for (i=0; i < num; i++, pt++, pso++, psn++)
	{
		v = pt->v2;
		pt->v2 = pt->v3;
		pt->v3 = v;

		pso->x = psn->x = -pso->x;
		pso->y = psn->y = -pso->y;
		pso->z = psn->z = -pso->z;
	}

	lpobj->flags ^= OBJECT_REVERSENORMALS;

	return(TRUE);
}

// reverse the vertex normals
LOGICAL ldmdlReverseObjectNormals(OBJECT* lpobj)
{
	if (lpobj == nullptr)
	{
		return(FALSE);
	}

	do
	{
		ldmdlReverseNormals(lpobj);
		lpobj = lpobj->attached;
	}
	while (lpobj != nullptr);

	return(TRUE);
}

static OBJECT* _reduceTrees(OBJECT* o)
{
    OBJECT* sibling = o;

    SINT32 tris = 0;

    OBJECT* commentobjects = nullptr;
    OBJECT* head = nullptr;

    while(sibling)
    {
        // If this has a comment, leave, since it may be for a specific tree
        if(!sibling->comment)
        {
            tris += sibling->numtris;
        }
        else
        {
            if(!commentobjects)
            {
                commentobjects = sibling;
                head = commentobjects;
            }
            else
            {
                head->sibling = sibling;
                head = head->sibling;
            }
        }

        sibling = sibling->sibling;
    }

	if(tris == 0)
	{
		return o;
	}

    if(tris == 1)
    {
        o->psnorms = (piVECTOR*)memAlloc(sizeof(piVECTOR));
        return o;
    }

    OBJECT* newobj = nullptr;

    // One triangle old object for one vertice new object
    obmainCreateObjectFull(&newobj, o->name, tris, tris);

    // Copy only necessary data over for trees

    newobj->flags |= o->flags;
    newobj->flag2 |= o->flag2;
    newobj->flag3 |= o->flag3;

    newobj->flag3 |= OBJECT_TREE_CLUSTER;

    newobj->minlod = o->minlod;
    newobj->maxlod = o->maxlod;

    sibling = o;

    piVectorPtr pos = newobj->StaticPositions();
    piVectorPtr nrm = newobj->StaticNormals();

    SINT32 q = 0;

    while(sibling)
    {
        // If this has a comment, leave, since it may be for a specific tree
        if(!sibling->comment)
        {
            pos->x = sibling->pos.x;
            pos->y = sibling->pos.y;
            pos->z = sibling->pos.z;

            ++pos;

            nrm->x = sibling->ImmutableNormals()->x;
            nrm->y = sibling->ImmutableNormals()->y;
            nrm->z = sibling->ImmutableNormals()->z;

            ++nrm;

			if(sibling->psnorms)
			{
				newobj->psnorms[q].x = sibling->psnorms[0].x;
				newobj->psnorms[q].y = sibling->psnorms[0].y;
				newobj->psnorms[q].z = sibling->psnorms[0].z;
			}
            q++;

            OBJECT* free = sibling;
            sibling = sibling->sibling;
            obmainFreeObject(free);

            continue;
        }

        sibling = sibling->sibling;
    }

    if(commentobjects)
    {
        head->sibling = nullptr;
        newobj->sibling = commentobjects;
    }

    ldmdlGetBounds(newobj);

    return newobj;
}

LOGICAL ldmdlReduceSpeedTreeObjects(OBJECT* root)
{
    if(!root)
    {
        return TRUE;
    }

    if(root->flags & OBJECT_GROUP)
    {
        if(root->flag2 & OBJECT_SPEEDTREE)
        {
            if(root->child && !(root->child->flags & OBJECT_LOD))
            {
                root->child = _reduceTrees(root->child);
            }
        }
    }

    if(root->child && (root->child->flags & OBJECT_GROUP || root->child->flags & OBJECT_LOD))
    {
        ldmdlReduceSpeedTreeObjects(root->child);
    }

    if(root->sibling && (root->sibling->flags & OBJECT_GROUP || root->sibling->flags & OBJECT_LOD))
    {
        ldmdlReduceSpeedTreeObjects(root->sibling);
    }

    return TRUE;
}

#if(0)
static LOGICAL CheckObjectCollision(OBJECT* lpobj, piVECTOR const* op, piVECTOR const* np, SINT32 id, piINDICES* pt)
{
LOGICAL		retval=FALSE;
float		d, d1, d2, d3;
float		da, db, eps;
piVECTOR	tp, dir;
piVECTOR const* pv1, *pv2, *pv3;
piVECTOR	m, q;
piVECTOR const*	pn, *pcn;


	math3dSubtractVectors(&dir, np, op);
	math3dNormalize(&dir);

	piVectorConstPtr rpos = lpobj->RenderPositions();
	pn      = &lpobj->psnorms[id];
	pcn     = &lpobj->pscdn[id*3];

	do
	{
		d = math3dDotProduct(&dir, pn);
		if (d >= 0.0f)
		{
			continue;
		}

		d1 = math3dDotProduct(&rpos[pt->v1], pn);
		d2 = math3dDotProduct(op, pn);
		d3 = math3dDotProduct(np, pn);

		eps = 0.01f * ScaleFactor;
		if (d2 >= d1+eps && d3 <= d1-eps)
		{
			pv1 = &rpos[pt->v1];
			pv2 = &rpos[pt->v2];
			pv3 = &rpos[pt->v3];

			// create a vertex where the vector crosses the poly plane
			math3dScaleVector(&m, &dir, 1.0f / -d);
			math3dScaleVector(&q, &m, d2 - d1);
			math3dAddVectors(&tp, op, &q);

			// polygon fence algorithm
			// used to determine whether the point is within the polygon bounds
			da = math3dDotProduct(&pcn[0], pv1);
			db = math3dDotProduct(&pcn[0], &tp);
			if (db > da)
			{
				continue;
			}

			da = math3dDotProduct(&pcn[1], pv2);
			db = math3dDotProduct(&pcn[1], &tp);
			if (db > da)
			{
				continue;
			}

			da = math3dDotProduct(&pcn[2], pv3);
			db = math3dDotProduct(&pcn[2], &tp);
			if (db > da)
			{
				continue;
			}

			retval = TRUE;
			break;
		}
	}
	while (FALSE);

	return(retval);
}
#endif

#if(0)
static LOGICAL PolygonsIntersect(OBJECT* lpobj, SINT32 id1, SINT32 id2, piINDICES* pt1, piINDICES* pt2)
{
	piVECTOR const* va1, *va2, *va3;
	piVectorConstPtr rpos = lpobj->RenderPositions();

	// do first poly against second
	va1 = &rpos[pt1->v1];
	va2 = &rpos[pt1->v2];
	va3 = &rpos[pt1->v3];


	if (CheckObjectCollision(lpobj, va1, va2, id2, pt2))
	{
		return(TRUE);
	}

	if (CheckObjectCollision(lpobj, va2, va1, id2, pt2))
	{
		return(TRUE);
	}

	if (CheckObjectCollision(lpobj, va1, va3, id2, pt2))
	{
		return(TRUE);
	}

	if (CheckObjectCollision(lpobj, va3, va1, id2, pt2))
	{
		return(TRUE);
	}

	if (CheckObjectCollision(lpobj, va2, va3, id2, pt2))
	{
		return(TRUE);
	}

	if (CheckObjectCollision(lpobj, va3, va2, id2, pt2))
	{
		return(TRUE);
	}


	// do second poly against first
	va1 = &rpos[pt2->v1];
	va2 = &rpos[pt2->v2];
	va3 = &rpos[pt2->v3];


	if (CheckObjectCollision(lpobj, va1, va2, id1, pt1))
	{
		return(TRUE);
	}

	if (CheckObjectCollision(lpobj, va2, va1, id1, pt1))
	{
		return(TRUE);
	}

	if (CheckObjectCollision(lpobj, va1, va3, id1, pt1))
	{
		return(TRUE);
	}

	if (CheckObjectCollision(lpobj, va3, va1, id1, pt1))
	{
		return(TRUE);
	}

	if (CheckObjectCollision(lpobj, va2, va3, id1, pt1))
	{
		return(TRUE);
	}

	if (CheckObjectCollision(lpobj, va3, va2, id1, pt1))
	{
		return(TRUE);
	}


	return(FALSE);
}
#endif

LOGICAL ldmdlSortObjectTriangles(OBJECT* lpobj)
{
SINT32		i, j, num;
FLOAT32		d1, d2;
piVECTOR	pv1, pv2, pv3, pp;
piINDICES	ndx;
piINDICES*	pt;


	if (lpobj == nullptr)
	{
		return(FALSE);
	}

	if (lpobj->flags & OBJECT_NOTOBJECT)
	{
		return(TRUE);
	}

	if (!(lpobj->flags & OBJECT_INDEXED) ||
		!(lpobj->flags & OBJECT_USEVERTEXBSP))
	{
		return(TRUE);
	}

	pp = lpobj->bbox[0];
	//pp.z = 0.0f;

	do
	{
		piVectorConstPtr rpos = lpobj->RenderPositions();
		num = lpobj->numtris;
		pt = lpobj->ptris;
		for (i=0; i < num-1; i++)
		{
			math3dSubtractVectors(&pv1, &rpos[pt[i].v1], &pp);
			math3dSubtractVectors(&pv2, &rpos[pt[i].v2], &pp);
			math3dSubtractVectors(&pv3, &rpos[pt[i].v3], &pp);
			math3dAddVectors(&pv1, &pv1, &pv2);
			math3dAddVectors(&pv1, &pv1, &pv3);
			d1 = math3dDotProduct(&pv1, &pv1);

			for (j=i+1; j < num; j++)
			{
				math3dSubtractVectors(&pv1, &rpos[pt[j].v1], &pp);
				math3dSubtractVectors(&pv2, &rpos[pt[j].v2], &pp);
				math3dSubtractVectors(&pv3, &rpos[pt[j].v3], &pp);
				math3dAddVectors(&pv1, &pv1, &pv2);
				math3dAddVectors(&pv1, &pv1, &pv3);
				d2 = math3dDotProduct(&pv1, &pv1);

				if (d1 < d2)
				{
					ndx = pt[i];
					pt[i] = pt[j];
					pt[j] = ndx;
				}
			}
		}

		lpobj = lpobj->attached;
	}
	while (lpobj);

	return(TRUE);
}


// traverses tree, if "nofiltering" found, sets it and subtree to no filter
LOGICAL ldmdlUpdateGroupFilteringAndBlendStatus(OBJECT * ObjGrp)
{
OBJECT * ObjAtt;
SINT8*		ptr;

	if (ObjGrp == nullptr)
	{
		return (TRUE);
	}

	ObjAtt = ObjGrp;

	do
	{
		if (obmainFindCommentName(&ptr, ObjAtt->comment, "nofiltering"))
		{
			obmainSetFlagsInThisTree(ObjAtt, OBJECT_NOFILTER);
		}
		if (obmainFindCommentName(&ptr, ObjAtt->comment, "blendadditive"))
		{
			obmainSetFlag2InThisTree(ObjAtt, OBJECT_BLENDADDITIVE);
		}

		ObjAtt = ObjAtt->attached;
	}
	while (ObjAtt != nullptr);

	if (ObjGrp->child != nullptr)
	{
		_UpdateGroupFilteringAndBlendStatus(ObjGrp->child);
	}

	return (TRUE);
}


static LOGICAL _UpdateGroupFilteringAndBlendStatus(OBJECT * ObjGrp)
{
OBJECT* ObjAtt,*ObjSib;
SINT8*		ptr;

	if (ObjGrp == nullptr)
	{
		return (TRUE);
	}

	ObjSib = ObjGrp;

	do
	{
		ObjAtt = ObjSib;

		do
		{
			if (obmainFindCommentName(&ptr, ObjAtt->comment, "nofiltering"))
			{
				obmainSetFlagsInThisTree(ObjAtt, OBJECT_NOFILTER);
			}
			if (obmainFindCommentName(&ptr, ObjAtt->comment, "blendadditive"))
			{
				obmainSetFlag2InThisTree(ObjAtt, OBJECT_BLENDADDITIVE);
			}

			ObjAtt = ObjAtt->attached;
		}
		while (ObjAtt != nullptr);

		if (ObjSib->child != nullptr)
		{
			_UpdateGroupFilteringAndBlendStatus(ObjSib->child);
		}

		ObjSib = ObjSib->sibling;
	}
	while (ObjSib != nullptr);

	return (TRUE);
}

//////////////////////////////////////////////////////////////////////
// Return true if two objects can be merged.
//////////////////////////////////////////////////////////////////////

bool CanObjectsMerge(OBJECT& lhs, OBJECT& rhs)
{
	const bool lhsHasAlpha = (lhs.flag2 & OBJECT_HASALPHA);
	const bool rhsHasAlpha = (rhs.flag2 & OBJECT_HASALPHA);

	if (PN_LIKELY(&lhs != &rhs))
	{
		if (_reduceModel && (lhsHasAlpha || rhsHasAlpha))
			return false;
		if (lhs.matid != rhs.matid || lhs.texid != rhs.texid || lhsHasAlpha != rhsHasAlpha)
			return false;
		if ((lhs.flag2 & OBJECT_HIDDEN) != (rhs.flag2 & OBJECT_HIDDEN))
			return false;
		if ((lhs.flags & OBJECT_INDEXED) && (rhs.flags & OBJECT_INDEXED))
			return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////
//	ldmdlReduceAttachments
//
//Merges all attachments to an object
//////////////////////////////////////////////////////////////////////

void ldmdlReduceAttachments(OBJECT* node)
{
	if(PN_UNLIKELY(!node))
		return;

	std::vector<OBJECT*> candidates;
	for (OBJECT* curNode = node; curNode; curNode = curNode->attached)
	{
		candidates.clear();
		size_t resultNumVerts = curNode->numverts;
		size_t resultNumTris = curNode->numtris;

		// Try to consume as many nodes as we can
		for (OBJECT* attached = curNode->attached; attached; attached = attached->attached)
		{
			if (CanObjectsMerge(*curNode, *attached))
			{
				candidates.push_back(attached);
				resultNumVerts += attached->numverts;
				resultNumTris += attached->numtris;
			}
		}

		if (candidates.empty())
			continue;

		curNode->flags &= ~OBJECT_TRIANGLEFAN;
		curNode->flags |= (OBJECT_TRIANGLELIST | OBJECT_INDEXED);

		curNode->puverts = (piUNIFIED_VERTEX*)memRealloc(curNode->puverts, resultNumVerts * sizeof(piUNIFIED_VERTEX));
		curNode->ptris = (piINDICES*)memRealloc(curNode->ptris, resultNumTris * sizeof(piINDICES));

		if (curNode->psnorms != curNode->psnorig)
			memRelease(curNode->psnorms);
		memRelease(curNode->psnorig);
		if (curNode->pscdn != curNode->pscdon)
			memRelease(curNode->pscdn);
		memRelease(curNode->pscdon);
		memRelease(curNode->ptverts);

		for (auto it = candidates.begin(), end = candidates.end(); it != end; ++it)
		{
			auto addVerts = (*it)->numverts;
			memcpy(curNode->puverts + curNode->numverts, (*it)->puverts, addVerts * sizeof(piUNIFIED_VERTEX));
			curNode->numverts += addVerts;
			obmainDeleteNode(*it);
		}

		auto i = curNode->numtris;
		for (auto it = curNode->ptris + i, end = curNode->ptris + resultNumTris; it != end; ++it)
		{
			// Funky useless indices
			it->v1 = i++;
			it->v2 = i++;
			it->v3 = i++;
		}

		curNode->numtris = resultNumTris;
		ABORT_IF(curNode->numverts != resultNumVerts);
	}
	return;
}

//////////////////////////////////////////////////////////////////////
//	MergeObjectsIfPossible
//
//Merges two objects if they are compatible
//////////////////////////////////////////////////////////////////////

bool MergeObjectsIfPossible(OBJECT* main, OBJECT* o)
{
	if (PN_UNLIKELY(!main || !o))
		return FALSE;

	if (!CanObjectsMerge(*main, *o))
		return FALSE;

	UINT16				old_numverts = main->numverts;
	UINT16				old_numtris = main->numtris;

	// Append o onto main
	main->numverts += o->numverts;
	main->numtris += o->numtris;

	main->flags &= ~OBJECT_TRIANGLEFAN;
	main->flags |= (OBJECT_TRIANGLELIST | OBJECT_INDEXED);

	if(main->psnorms != main->psnorig)
		memRelease(main->psnorms);
	memRelease(main->psnorig);
	if(main->pscdn != main->pscdon)
		memRelease(main->pscdn);
	memRelease(main->pscdon);
	memRelease(main->ptverts);

	main->psnorig		= nullptr;
	main->psnorms		= nullptr;
	main->pscdon		= nullptr;
	main->pscdn			= nullptr;
	main->ptverts		= nullptr;

	main->puverts		= (piUNIFIED_VERTEX*)memRealloc(main->puverts, main->numverts * sizeof(piUNIFIED_VERTEX));
	main->ptris			= (piINDICES*)memRealloc(main->ptris, main->numtris * sizeof(piINDICES));

	memcpy(main->puverts + old_numverts, o->puverts, o->numverts*sizeof(piUNIFIED_VERTEX));

	main->PopulateIndexes(old_numtris);

	obmainDeleteNode(o);

	return true;
}

//////////////////////////////////////////////////////////////////////
//	ldmdlReduceModel
//
//Find every branch of mergable objects for reduction
//////////////////////////////////////////////////////////////////////
void ldmdlReduceModel(OBJECT* node)
{
    _reduceModel = true;

	assert(node);

	if(IsMergingBranchTopNode(node) && IsMergableBranchTopNode(node))
        ReduceBranch(node->child, node->child);	// Start a new merging branch here

    if(node->child && IsMergableBranchTopNode(node))
        ldmdlReduceModel(node->child);

    if(node->attached)
        ldmdlReduceModel(node->attached);

    if(node->sibling)
        ldmdlReduceModel(node->sibling);

    _reduceModel = false;

	return;
}


//////////////////////////////////////////////////////////////////////
//	ReduceBranch
//
//Find every object with a mesh for possible merging in the current branch
//////////////////////////////////////////////////////////////////////
static void ReduceBranch(OBJECT* node, OBJECT* branchtop)
{
	if(!node) return;

	if(	!IsMergingBranchTopNode(node) && IsMergableBranchTopNode(node) )
	{
		if(	!(node->flags & OBJECT_NOTOBJECT) &&
			!(node->flags & OBJECT_LINESTRIP) &&
			!(node->flags & OBJECT_LINECLOSED) &&
			node->flags & OBJECT_VISIBLE )
		{
			// Object with possibly mergeable mesh found
			MergeObjectWithBranch(node, branchtop);
		}
		if(node->child) ReduceBranch(node->child, branchtop);
	}
	if(node->attached) ReduceBranch(node->attached, branchtop);
	if(node->sibling) ReduceBranch(node->sibling, branchtop);
	return;
}


//////////////////////////////////////////////////////////////////////
//	MergeObjectWithBranch
//
//Find every object with a mergeable mesh for merging in the current branch
//////////////////////////////////////////////////////////////////////
static void MergeObjectWithBranch(OBJECT* o, OBJECT* node)
{
	if(!o || !node) return;

	OBJECT*				child			= node->child;
	OBJECT*				attached		= node->attached;
	OBJECT*				sibling			= node->sibling;

	if(!IsMergingBranchTopNode(node) && IsMergableBranchTopNode(node))
	{
		if(	!(node->flags & OBJECT_NOTOBJECT) &&
			!(node->flags & OBJECT_LINESTRIP) &&
			!(node->flags & OBJECT_LINECLOSED) &&
			node->flags & OBJECT_VISIBLE )
		{
			// Object with possibly mergeable mesh found
			if(MergeObjectsIfPossible(o, node))
			{
				// attached takes over sibling pointer if node is deleted
				if(attached) sibling = nullptr;
			}
		}
		if(child) MergeObjectWithBranch(o, child);
	}
	if(attached) MergeObjectWithBranch(o, attached);
	if(sibling) MergeObjectWithBranch(o, sibling);
}


//////////////////////////////////////////////////////////////////////
//	IsMergingBranchTopNode
//
//Find out if a new branch of object merging starts from this node
//////////////////////////////////////////////////////////////////////
static LOGICAL IsMergingBranchTopNode(OBJECT* node)
{
	if(!node) return FALSE;

	if( (node->flags & OBJECT_LOD)		||
		(node->flags & OBJECT_DOF) )
	{
		return TRUE;
	}

	if(node->name)
	{
		if( strstr(node->name, "left wing")		||											// The cockpit view is reffering to the ext. view model objects
			strstr(node->name, "right wing")	||
			strstr(node->name, "both")			||
			strstr(node->name, "bottombase")	||
			strstr(node->name, "extern") )
		{
			return TRUE;
		}
	}

	if(node->comment)
	{
		if(	strstr(node->comment, "bottombase")	||
			strstr(node->comment, "visdamage")	)
		{
			return true;
		}
	}
	return FALSE;
}

//////////////////////////////////////////////////////////////////////
//	IsMergableBranchTopNode
//
//Find out if it is possible that something below this node can be merged
//////////////////////////////////////////////////////////////////////
static LOGICAL IsMergableBranchTopNode(OBJECT* node)
{
	if(!node) return FALSE;

	if(node->name)
	{
		// Group names which sub-objects cannot be merged. Add here if necessary.
		if( strstr(node->name, "vcpit")			||									// Cockpit handles
            strstr(node->name, "topbase")       ||
            strstr(node->name, "bottombase")    ||
			strstr(node->name, "lodc")			||									// Non-visible vehicle data
			strstr(node->name, "fx")			||									// FX handles
			strstr(node->name, "origin1")		||									// origins for flags
			strstr(node->name, "origin2")		||									// origins for flags
			strstr(node->name, "flagtable")		||									// For flagtables1.flt
			strstr(node->name, "lodloadout")    ||
			strstr(node->name, "upperblock")    ||
			strstr(node->name, "cargotop")		||
			strstr(node->name, "cargohook")		||
			strstr(node->name, "cargoboom")		||
            strstr(node->name, "lgearup")       ||
            strstr(node->name, "rgearup")       ||
			strstr(node->name, "decal") )											// Vehicle decals
		{
			return FALSE;
		}
	}

	if(node->comment)
	{
		if( strstr(node->comment, "speedtree")		||
			strstr(node->comment, "connect")	    ||
            strstr(node->comment, "topbase")        ||
            strstr(node->comment, "bottombase")     ||
			strstr(node->comment, "flapdn")			||
			strstr(node->comment, "flapup")			||
			strstr(node->comment, "nofire")			||
			strstr(node->comment, "ode=")			||
			strstr(node->comment, "capture")		||
			strstr(node->comment, "seperate")	    ||								// Seperated collider mesh for highly merged buildings
			strstr(node->comment, "separate"))										// in kase it's speld korectly...
		{
			return FALSE;
		}
	}

	return TRUE;
}

inline bool operator==(const piUNIFIED_VERTEX &a, const piUNIFIED_VERTEX &b) { return !memcmp(&a, &b, sizeof(piUNIFIED_VERTEX)); }

typedef std::multimap<size_t,ogl::VertexIdx> UniVertexIndexer;

//////////////////////////////////////////////////////////////////////
//	ldmdlIndexVertices
//////////////////////////////////////////////////////////////////////
void ldmdlIndexVertices(OBJECT* o)
{
//	std::vector<piUNIFIED_VERTEX>::iterator	existingElement;
	piUNIFIED_VERTEX*						crtVertex			= nullptr;

	if(o->numverts)
	{
		UniVertexIndexer indexer;
		ogl::VertexIdx existingElementIndex;
		std::vector<ogl::VertexIdx>		indices;
		std::vector<piUNIFIED_VERTEX>	uniqueVertices;
		indices.reserve(o->numverts);
		uniqueVertices.reserve(o->numverts);
		crtVertex = o->puverts;
        if(o->numverts > 3 && (o->numverts % 3) == 0 && (o->numverts / 3) == o->numtris)
		{
			for(int i=0; i < o->numverts; i++, crtVertex++)
			{
                size_t code = SuperFastHash((const char*)crtVertex,sizeof(piUNIFIED_VERTEX));
                std::pair<UniVertexIndexer::iterator,UniVertexIndexer::iterator> rvi = indexer.equal_range(code);
                bool newVertex = true;

                if ( rvi.first != rvi.second ) // possible match (might be more than 1)
                {
                    for (UniVertexIndexer::iterator it=rvi.first; it!=rvi.second; ++it)
                    {
                        // [DCF] '.second' has no relation to puverts and should not be used.
                        //piUNIFIED_VERTEX* test = &o->puverts[ (*it).second ];
                        // Verify by 'code' value should be ok(?) If not, then this whole routine is invalid.
                        if (code == it->first)
                        {
                            newVertex = false;
                            existingElementIndex = (*it).second;
                            break;
                        }
                    }
                }

                if ( newVertex )
                {
				    indices.push_back((ogl::VertexIdx)uniqueVertices.size());
				    uniqueVertices.push_back(*crtVertex);
                    indexer.insert( std::pair<size_t,ogl::VertexIdx>( code, (ogl::VertexIdx)uniqueVertices.size()-1) );
                }
                else
                {
                    indices.push_back(existingElementIndex);
                }
			}

			o->numverts = uniqueVertices.size();
			memFree(o->puverts);
			o->puverts = (piUNIFIED_VERTEX*)memAlloc(o->numverts * sizeof(piUNIFIED_VERTEX));
			memcpy(o->puverts, &uniqueVertices[0], o->numverts * sizeof(piUNIFIED_VERTEX));
			memcpy(o->ptris, &indices[0], o->numtris * 3 * sizeof(ogl::VertexIdx));
		}
	}

	if(o->child)	ldmdlIndexVertices(o->child);
	if(o->attached)	ldmdlIndexVertices(o->attached);
	if(o->sibling)	ldmdlIndexVertices(o->sibling);
}


//////////////////////////////////////////////////////////////////////
//	ldmdlProcessBounds
//
//Create bounding boxes for each object
//////////////////////////////////////////////////////////////////////
void ldmdlProcessBounds(OBJECT* o)
{
	assert(o);

	if(!(o->comment && strstr(o->comment, "speedtree")))
	{
		if(!(o->flags & OBJECT_NOTOBJECT))
		{
			ldmdlGetBounds(o);
		}
		if(o->child)	ldmdlProcessBounds(o->child);
	}
	if(o->attached)	ldmdlProcessBounds(o->attached);
	if(o->sibling)	ldmdlProcessBounds(o->sibling);
}


//////////////////////////////////////////////////////////////////////
//	ldmdlProcessBranchCollisionData
//
//Creates the collision data for the object tree branch
//////////////////////////////////////////////////////////////////////
void ldmdlProcessBranchCollisionData(OBJECT* o)
{
	assert(o);

	if(	!(o->comment && strstr(o->comment, "speedtree"))	&&
		!(o->comment && strstr(o->comment, "smoke"))		&&
		!(o->comment && strstr(o->comment, "entity"))		&&
		!(o->comment && strstr(o->comment, "instance"))	)
	{
		if(!(o->flags & OBJECT_NOTOBJECT))
		{
			o->flags |= OBJECT_COLLISION;
			ldmdlCalcCollisionNormals(o);
		}

		if(o->child)	ldmdlProcessBranchCollisionData(o->child);
		if(o->attached)	ldmdlProcessBranchCollisionData(o->attached);
	}
	if(o->sibling)	ldmdlProcessBranchCollisionData(o->sibling);
}
