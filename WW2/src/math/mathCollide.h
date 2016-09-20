#pragma once

#include "pi/pi.h"

extern SINT32 mathRayToPlane(piVECTOR* p1,
                      piVECTOR* p2,
                      FLOAT32 a,
                      FLOAT32 b,
                      FLOAT32 c,
                      FLOAT32 d,
                      FLOAT32* t);


// Prevent link errors with inline
#ifndef MATHCOLLIDE_INLINE

extern SINT32 mathRayToCylinder(piVECTOR const*startP,      // IN
                         piVECTOR const*endP,        // IN
                         piVECTOR* cylinderPos, // IN
                         FLOAT32 radius,        // IN
                         FLOAT32 height,        // IN
                         LOGICAL checkCaps,     // IN
                         FLOAT32* time,         // OUT
                         piVECTOR* cp);         // OUT

#else // MATHCOLLIDE_INLINE

#define MATHCOLLIDE_INLINE

#define INLINE inline

#include "math/mathCollide.cpp"

#undef INLINE
#undef MATHCOLLIDE_INLINE

#endif

