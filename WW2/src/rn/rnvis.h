#pragma once

#include "math/Vec3f.h"
#include "osgImport/BoundingBox.h"

#define LEFT_PLANE          0x01
#define RIGHT_PLANE         0x02
#define BOTTOM_PLANE        0x04
#define TOP_PLANE           0x08
#define NEAR_PLANE          0x10
#define FAR_PLANE           0x20

enum
{
    left_plane,
    right_plane,
    top_plane,
    bottom_plane,
    near_plane,
    far_plane
};

#define CLIP_NO_NEAR_FAR    LEFT_PLANE | RIGHT_PLANE | TOP_PLANE | BOTTOM_PLANE
#define CLIP_NO_FAR         NEAR_PLANE | LEFT_PLANE | RIGHT_PLANE | TOP_PLANE | BOTTOM_PLANE
#define CLIP_ALL_PLANES     NEAR_PLANE | FAR_PLANE | LEFT_PLANE | RIGHT_PLANE | TOP_PLANE | BOTTOM_PLANE

	#include "ob/obmain.h"

	// Internal 3d structures
	struct PLANE
	{
		FLOAT32	a;
		FLOAT32	b;
		FLOAT32	c;
		FLOAT32	d;

        float Z(float x, float y)
        {
            if(c < 0.00001f && c > -0.00001f)
            {
                return 0;
            }

            return -(d + a * x + b * y) / c;
        }

	};

	extern LOGICAL rnvisInit();
	extern void rnvisKill();
	extern void rnvisCreatePlane(piVECTOR* p0, piVECTOR* p1, piVECTOR* p2, PLANE* plane);
	extern void rnvisSetupClipPlanes(bool force = false);
    extern void rnvisSetupFrustum();

#ifndef RNVIS_INLINE

	extern LOGICAL rnvisVectorFOVCheck(piVECTOR* pos, FLOAT32 radius);
	extern LOGICAL rnvisVectorFOVCheckWithoutBack(piVECTOR* pos, FLOAT32 radius);
	extern LOGICAL rnvisFOVCheck(OBJECT* lpobj);
    extern bool rnvisVectorFOVCheck2(Vec3f& pos, float radius);

    extern bool rnvisObjectCull(unsigned int& mask, OBJECT* o);
    extern bool rnvisPointCull(unsigned int mask, const Vec3f& pos);
    extern bool rnvisSphereCull(unsigned int& mask, const Vec3f& pos, float radius);
    extern bool rnvisBoundBoxCull(unsigned int& mask, const BoundingBox& bb);

#else

#define RNVIS_INLINE
#define INLINE __inline
#include "rn/rnvis.cpp"
#undef INLINE
#undef RNVIS_INLINE

#endif
