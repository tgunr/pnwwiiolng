#include <syscfg.h> // Path is project specific.

#include <math.h>
#include "pi/pi.h"
#include "include/vars.h"
#include "rn/rnmain.h"
#include "ob/obmain.h"
#include "math/math3d.h"
#include "ogl/ogl.h"

#ifndef RNVIS_INLINE
#define RNVIS_CODE
#include "rn/rnvis.h"
#endif

#ifdef RNVIS_CODE

#include "osgImport/Polytope.h"
static Polytope polytope;

LOGICAL rnvisInit(void)
{
	return(TRUE);
}

void rnvisKill(void)
{
}

void rnvisCreatePlane(piVECTOR* p0, piVECTOR* p1, piVECTOR* p2, PLANE* plane)
{
piVECTOR	v0, v1;

	math3dSubtractVectors(&v0, p1, p0);
	math3dSubtractVectors(&v1, p2, p0);
	math3dCrossProduct(&v0, &v0, &v1);
	math3dNormalize(&v0);
	plane->a = v0.x;
	plane->b = v0.y;
	plane->c = v0.z;
	plane->d = -math3dDotProduct(&v0, p0);
}

// Once per frame
void rnvisSetupFrustum()
{
    polytope.setToUnitFrustum();

    const piMATRIX& mv = oglGetModelViewMatrix();

    Matrixf mvp = Matrixf(mv) * Matrixf(*piGetProjectionMatrix());

    polytope.transformProvidingInverse(mvp);
}

void rnvisSetupClipPlanes(bool force)
{
FLOAT32		angle, aspect;
piVECTOR	p0, p1;
piVECTOR	leftplane, topplane;

// Local statics
static FLOAT32 localfov=0.0f;
static SINT32 localwidth=0;
static SINT32 localheight=0;

	BackClip = piLongLikeFloat(FogEnd);

    if(!force)
    {
	    if (localfov == HorizontalFieldOfView &&
		    localwidth == piGetWidth() &&
		    localheight == piGetHeight())
	    {
		    return;
	    }
    }

	localfov = HorizontalFieldOfView;
	localwidth = piGetWidth();
	localheight = piGetHeight();

	angle = (FLOAT32)tan(HorizontalFieldOfView * 0.5f);
	aspect = angle * (FLOAT32)piGetHeight() / (FLOAT32)piGetWidth();

	// same for all vectors
	p0.z = p1.z = 1.0f;

	p0.x = -angle;
	p0.y = -aspect;
	p1.x = p0.x;
	p1.y = -p0.y;
	math3dCrossProduct(&leftplane, &p0, &p1);
	math3dNormalize(&leftplane);

	LeftPlaneX = leftplane.x;
	LeftPlaneZ = leftplane.z;

	p0.x = -angle;
	p0.y = aspect;
	p1.x = -p0.x;
	p1.y = p0.y;
	math3dCrossProduct(&topplane, &p0, &p1);
	math3dNormalize(&topplane);

	TopPlaneY = topplane.y;
	TopPlaneZ = topplane.z;

    rnvisSetupFrustum();
}

#endif // RNVIS_CODE

#ifdef RNVIS_INLINE
#define INLINE __inline
#else
#define INLINE
#endif

INLINE bool rnvisVectorFOVCheck2(Vec3f& pos, float radius)
{
    float x, y, z, zz;

	z = pos * Vec3f(playdir.x, playdir.y, playdir.z) + playtrans.z;

	// Front && Back
    if(z + radius < 0 || z - radius > BackClip)
    {
        return false;
    }

	y =  (pos * Vec3f(playup.x, playup.y, playup.z) + playtrans.y) * TopPlaneY;
	zz = z * TopPlaneZ;

	// Top && Bottom
	if((zz + y) > radius || (zz - y) > radius)
	{
		return false;
	}

	x = (pos * Vec3f(playright.x, playright.y, playright.z) + playtrans.x) * LeftPlaneX;
	zz = z * LeftPlaneZ;

	// Left && Right
	if((zz + x) > radius || (zz - x) > radius)
	{
		return false;
	}

	return true;
}

INLINE LOGICAL rnvisVectorFOVCheck(piVECTOR* pos, FLOAT32 radius)
{
    FLOAT32 x, y, z, zz;

	z = math3dDotProduct(pos, &playdir) + playtrans.z;

	// Front && Back
	if((z + radius) < 0)
        return(FALSE);	// FrontClip

    if(radius < 1000.0f)					// We don't want to back clip large objects like clouds
		if((z - radius) > BackClip)
            return(FALSE);

	y = (math3dDotProduct(pos, &playup) + playtrans.y) * TopPlaneY;
	zz = z * TopPlaneZ;

	// Top && Bottom
	if ((zz + y) > radius ||
		(zz - y) > radius)
	{
		return(FALSE);
	}

	x = (math3dDotProduct(pos, &playright) + playtrans.x) * LeftPlaneX;
	zz = z * LeftPlaneZ;

	// Left && Right
	if ((zz + x) > radius ||
		(zz - x) > radius)
	{
		return(FALSE);
	}

	return(TRUE);
}

// does not do far clip plane check
INLINE LOGICAL rnvisVectorFOVCheckWithoutBack(piVECTOR* pos, FLOAT32 radius)
{
FLOAT32		x, y, z, zz;

	z = math3dDotProduct(pos, &playdir) + playtrans.z;

	// Front && Back
	if ((z + radius) < 0) // FrontClip
	{
		return(FALSE);
	}

	y = (math3dDotProduct(pos, &playup) + playtrans.y) * TopPlaneY;
	zz = z * TopPlaneZ;

	// Top && Bottom
	if ((zz + y) > radius ||
		(zz - y) > radius)
	{
		return(FALSE);
	}

	x = (math3dDotProduct(pos, &playright) + playtrans.x) * LeftPlaneX;
	zz = z * LeftPlaneZ;

	// Left && Right
	if ((zz + x) > radius ||
		(zz - x) > radius)
	{
		return(FALSE);
	}

	return(TRUE);
}

/*------------------------------------------------------------------
Unlike the other cull functions here, this returns TRUE if culled,
not false.
A radius (sphere) cull
-------------------------------------------------------------------*/
INLINE bool rnvisObjectCull(unsigned int& mask, OBJECT* o)
{
    static piVECTOR pos;

    if(mask == 0)
    {
        return false;
    }

	if(o->flags & OBJECT_USEMATRIX)
	{
		math3dVectorMultiply(&pos, &o->pos, &o->mat);
	}
	else
	{
		pos = o->pos;
	}

    static float radius = 0;

    if(o->maxrad > 0)
    {
        radius = o->maxrad;
    }
    else
    {
        radius = math3dDotProduct(&o->bbox[1], &o->bbox[1]);
    }

    radius = math3dFastSqrt(radius);

	static float z = 0;

    z = math3dDotProduct(&pos, &playdir) + playtrans.z;

    if(mask & NEAR_PLANE)
    {
        if(z + radius < 0)
        {
            return true;
        }

        if(z > radius)
        {
            // Subsequent checks against this plane not necessary
            mask &= ~NEAR_PLANE;
        }
    }

    if(mask & FAR_PLANE)
    {
        if(z - radius > BackClip)
        {
            return true;
        }

        if(BackClip - z > radius)
        {
            // Subsequent checks against this plane not necessary
            mask &= ~FAR_PLANE;
        }
    }

    static float zz = 0;

    if((mask & LEFT_PLANE) | (mask & RIGHT_PLANE))
    {
	    static float x = 0;
        x = (math3dDotProduct(&pos, &playright) + playtrans.x) * LeftPlaneX;
	    zz = z * LeftPlaneZ;

        if(mask & LEFT_PLANE)
        {
            if(zz + x > radius)
            {
                return true;
            }

            if(-(zz + x) > radius)
            {
                // Subsequent checks against this plane not necessary
                mask &= ~LEFT_PLANE;
            }
        }

        if(mask & RIGHT_PLANE)
        {
            if(zz - x > radius)
            {
                return true;
            }

            if(-(zz - x) > radius)
            {
                // Subsequent checks against this plane not necessary
                mask &= ~RIGHT_PLANE;
            }
        }
    }

    if((mask & TOP_PLANE) | (mask & BOTTOM_PLANE))
    {
	    static float y = 0;

        y = (math3dDotProduct(&pos, &playup) + playtrans.y) * TopPlaneY;
	    zz = z * TopPlaneZ;

        if(mask & TOP_PLANE)
        {
            if(zz + y > radius)
            {
                return true;
            }

            if(-(zz + y) > radius)
            {
                // Subsequent checks against this plane not necessary
                mask &= ~TOP_PLANE;
            }
        }

        if(mask & BOTTOM_PLANE)
        {
            if(zz - y > radius)
            {
                return true;
            }

            if(-(zz - y) > radius)
            {
                // Subsequent checks against this plane not necessary
                mask &= ~BOTTOM_PLANE;
            }
        }
    }

    o->distsqr = math3dFastSqrt((Vec3f(pos) + Vec3f(playpos)).length2());

    return false;
}

/*------------------------------------------------------------------
Unlike the other cull functions here, this returns TRUE if culled,
not false.
-------------------------------------------------------------------*/
INLINE bool rnvisSphereCull(unsigned int& mask, const Vec3f& pos, float radius)
{
    if(mask == 0)
    {
        return false;
    }

    polytope.setResultMask(mask);
    polytope.pushCurrentMask();

    static BoundingSphere bs;

    bs.set(pos, radius);

    bool cull = !polytope.contains(bs);
    mask = polytope.getResultMask();

    polytope.popCurrentMask();

    return cull;
}

/*------------------------------------------------------------------
Unlike the other cull functions here, this returns TRUE if culled,
not false.
-------------------------------------------------------------------*/
INLINE bool rnvisPointCull(unsigned int mask, const Vec3f& pos)
{
	static float z = 0;

    if(mask == 0)
    {
        return false;
    }

    z = pos * Vec3f(playdir) + playtrans.z;

    if(mask & NEAR_PLANE)
    {
        if(z < 0)
        {
            return true;
        }
    }

    if(mask & FAR_PLANE)
    {
        if(z > BackClip)
        {
            return true;
        }
    }

    static float zz = 0;

    if((mask & LEFT_PLANE) | (mask & RIGHT_PLANE))
    {
	    float x = 0;
        x = (pos * Vec3f(playright) + playtrans.x) * LeftPlaneX;
	    zz = z * LeftPlaneZ;

        if(mask & LEFT_PLANE)
        {
            if(zz + x > 0)
            {
                return true;
            }
        }

        if(mask & RIGHT_PLANE)
        {
            if(zz - x > 0)
            {
                return true;
            }
        }
    }

    if((mask & TOP_PLANE) | (mask & BOTTOM_PLANE))
    {
	    float y = 0;

        y = (pos * Vec3f(playup) + playtrans.y) * TopPlaneY;
	    zz = z * TopPlaneZ;

        if(mask & TOP_PLANE)
        {
            if(zz + y > 0)
            {
                return true;
            }
        }

        if(mask & BOTTOM_PLANE)
        {
            if(zz - y > 0)
            {
                return true;
            }
        }
    }

    return false;
}

/*------------------------------------------------------------------
Unlike the other cull functions here, this returns TRUE if culled,
not false.
Bound box is in relative world space
-------------------------------------------------------------------*/
INLINE bool rnvisBoundBoxCull(unsigned int& mask, const BoundingBox& bb)
{
    if(mask == 0)
    {
        return false;
    }

    polytope.setResultMask(mask);
    polytope.pushCurrentMask();

    bool cull = !polytope.contains(bb);
    mask = polytope.getResultMask();

    polytope.popCurrentMask();

    return cull;
}

INLINE LOGICAL rnvisFOVCheck(OBJECT* lpobj)
{
    static FLOAT32 x, y, z, zz, radius;
    static piVECTOR pos;

    math3dVectorMultiply(&pos, &lpobj->pos, &lpobj->mat);

    radius = math3dFastSqrt(lpobj->maxrad);

	z = math3dDotProduct(&pos, &playdir) + playtrans.z;

	// Front && Back
	if((z + radius) < 0)
        return(FALSE);	// FrontClip

	if(radius < 1000.0f)					// We don't want to back clip large objects like clouds
		if((z - radius) > BackClip)
            return(FALSE);

	y = (math3dDotProduct(&pos, &playup) + playtrans.y) * TopPlaneY;
	zz = z * TopPlaneZ;

	// Top && Bottom
	if((zz + y) > radius || (zz - y) > radius)
	{
		return(FALSE);
	}

	x = (math3dDotProduct(&pos, &playright) + playtrans.x) * LeftPlaneX;
	zz = z * LeftPlaneZ;

	// Left && Right
	if((zz + x) > radius || (zz - x) > radius)
	{
		return(FALSE);
	}

    lpobj->distsqr = z;

	return(TRUE);
}

#undef INLINE

#ifndef RNVIS_INLINE
#undef RNVIS_CODE
#endif
