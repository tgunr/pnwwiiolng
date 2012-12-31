#include <syscfg.h> // Path is project specific.
//////////////////////////////////////////////////////////////////////
// Module: mathCollide.cpp
// Description: Module to contain routine collision functions.
//////////////////////////////////////////////////////////////////////
#include "math/mathCollide.h"
#include "math/mathvect.h"
#include "math/math3d.h"
#include "math/mathnum.h"

#include "trn/trnmain.h"
#include "include/vars.h"

#define IS_SMALL(x) x < 1e-7 && x > -1e-7

#ifdef MATHCOLLIDE_INLINE
    #if platOS == OS_MACOS
        #define INLINE inline
    #else
        #define INLINE __forceinline
    #endif
#else
    #define INLINE
#endif

//////////////////////////////////////////////////////////////////////
// Function: mathRayToCylinder
// Description: Assume cylinder oriented along the Z axis.
//////////////////////////////////////////////////////////////////////
INLINE SINT32 mathRayToCylinder(piVECTOR const*startP,      // IN
                                piVECTOR const*endP,        // IN
                                piVECTOR* cylinderPos, // IN
                                FLOAT32 radius,        // IN
                                FLOAT32 height,        // IN
                                LOGICAL checkCaps,     // IN
                                FLOAT32* time,         // OUT
                                piVECTOR* cp)          // OUT
{
    piVECTOR p1, p2;
    FLOAT32 t = 0;

    // Translate ray points to origin
    mathSubVectors(&p1, startP, cylinderPos);
    mathSubVectors(&p2, endP, cylinderPos);

    piVECTOR dir;

    // x^2 + y^2 = r^2 (if circular cylinder)
    // (Sx + tVx)^2 + (Sy + tVy)^2 = r^2 (where S = start point, V = direction)
    // (Vx^2 + Vy^2)t^2 + (SxVx + SyVy)t + Sx^2 + Sy^2 - r^2 = 0

    // Use quadratic formula to solve.

    mathSubVectors(&dir, &p2, &p1);

    FLOAT32 a = dir.x * dir.x + dir.y * dir.y;

    // Line may be vertical
    if(a == 0.0f)
    {
        // Make direction from origin to the start point
        dir.x = -p1.x;
        dir.y = -p1.y;
        dir.z = -p1.z;

        mathReallyFastNormalize(dir);

        a = dir.x * dir.x + dir.y * dir.y;
    }

    FLOAT32 b = 0.0f;
    FLOAT32 c = 0.0f;

    b = 2.0f * (p1.x * dir.x + p1.y * dir.y);
    c = p1.x * p1.x + p1.y * p1.y - radius * radius;

    FLOAT32 d = b * b - 4 * a * c;

    if(d < 0)
    {
        // No intersection
        return -1;
    }

    t = (-b - math3dFastSqrt(d)) / (2.0f * a);

    // Intersect with infinte cylinder?
    if(t >= 0 && t <= 1)
    {
        *time = t;

        // Return collide point
        cp->x = startP->x + t * (endP->x - startP->x);
        cp->y = startP->y + t * (endP->y - startP->y);
        cp->z = startP->z + t * (endP->z - startP->z);

        if(checkCaps)
        {
            // Check cylinder caps
            if(cp->z <= cylinderPos->z && cp->z >= cylinderPos->z - height)
            {
                return 0;
            }
        }
        else
        {
            return 0;
        }
    }

    return -1;
}

//////////////////////////////////////////////////////////////////////
// Function: mathRayToPlane
// Description:
//////////////////////////////////////////////////////////////////////
SINT32 mathRayToPlane(piVECTOR* p1,
                      piVECTOR* p2,
                      FLOAT32 a,
                      FLOAT32 b,
                      FLOAT32 c,
                      FLOAT32 d,
                      FLOAT32* t)
{
    piVECTOR dir;

    mathSubVectors(&dir, p1, p2);

    piVECTOR n;
    n.x = a;
    n.y = b;
    n.z = c;

    FLOAT32 fDot = math3dDotProduct(&dir, &n);

    // Line does not intersect plane.
    if(fDot >= 0.0f)
        return 1;

    // a(x0 - tDx) + b(y0 - tDy) + c(z0 - tDz) + d = 0.
    FLOAT32 time = (d - math3dDotProduct(&n, p1)) / fDot;

    if(IS_SMALL(time))
        time = 0.0f;

    if(time < 0.0f)
        return 2;

    if(time > 1.0f)
        return 3;

    *t = time;

    return 0;
}

#undef INLINE
