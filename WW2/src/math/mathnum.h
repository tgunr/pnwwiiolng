#ifndef __MATHNUM_H__
#define __MATHNUM_H__

#include "pi/pi.h"
#include "math/Rand.h"

#define ABS(x) x < 0 ? -x : x

class Vec3f;

namespace PNRandom
{
    // Return min <= x <= max
    inline int random(int mn, int mx)
    {
        int diff = mx - mn + 1;
        return mn + (randomMT() % diff);
    }

    inline float random(float mn, float mx)
    {
        return mn + frandomMT() * (mx - mn);
    }

    inline void seedrandom(unsigned int seed)
    {
        seedMT(seed);
    }
};

struct CHAOTICSEED
{
	FLOAT64 X;
	FLOAT64 Y;
	FLOAT64 Z;

	FLOAT64 dt;
	FLOAT64 A;
	FLOAT64 B;
	FLOAT64 C;

};

extern LOGICAL mathGetChaoticSeed(CHAOTICSEED* lpSeed);
extern LOGICAL mathSeedRandomNumberGenerator();
extern LOGICAL mathSetChaoticSeed(CHAOTICSEED* lpSeed);

extern SINT16 mathGetChaoticNumber();

extern FLOAT32 mathGetFloat32ChaoticNumber();
extern FLOAT32 mathFastInvSqrt(FLOAT32 value);

extern FLOAT64 fexp(FLOAT64 arg);

extern SINT8 mathGetRandomNumberFromSeed(SINT32* pSeed);

extern void mathVectorToYAxis(piVECTOR* lpInV, piMATRIX* lpOutMatrix);
extern void mathVectorToZAxis(piVECTOR* lpInV, piMATRIX* lpOutMatrix);
extern void mathTransposeMatrix(piMATRIX& m);
extern void mathReallyFastNormalize(piVECTOR& in);
extern unsigned char GetEightPartCartesianOrdinal(const Vec3f& dir);
extern const Vec3f& GetEightPartOrdinalDirection(unsigned char ordinal);

extern UINT32 closestPowerOfTwo(UINT32 x);

extern void mathFloatToInt(SINT32* int_pointer, FLOAT32 f);

#endif
