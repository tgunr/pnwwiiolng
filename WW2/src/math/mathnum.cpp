#include <syscfg.h> // Path is project specific.
//////////////////////////////////////////////////////////////////////
// Module: mathnum.cpp
// Description: Math Numeric. Handle miscellaneous math functions here.
// Not necessarily 3D.
//////////////////////////////////////////////////////////////////////
#include "math/mathnum.h"

#include "include/vars.h"

#include <math.h>
#include "math/math3d.h"
#include "spt/Random.h"

static CHAOTICSEED g_chaoticSeed;

#if platOS == OS_WIN32

union eco
{
    double d;
    struct
    {
        long i;
        long j;
    } n;
};
static union eco _eco;
#endif

#if platOS == OS_WIN32
const double _eco_m = (1048576L/0.693147180559945309417232121458177);
const double _eco_a = (1072693248L - 60801L);
#endif

/////////////////////////////////////////////////////////
// Function: mathGetChaoticSeed
// Description:
/////////////////////////////////////////////////////////
LOGICAL mathGetChaoticSeed(CHAOTICSEED* lpSeed)
{
    if(lpSeed == nullptr)
        return FALSE;

    *lpSeed = g_chaoticSeed;

    return TRUE;
}

/////////////////////////////////////////////////////////
// Function: mathSetChaoticSeed
// Description:
/////////////////////////////////////////////////////////
LOGICAL mathSetChaoticSeed(CHAOTICSEED* lpSeed)
{
    memset(&g_chaoticSeed, 0, sizeof(CHAOTICSEED));

    if(lpSeed == nullptr)
        return FALSE;

	g_chaoticSeed = *lpSeed;

	return TRUE;
}

/////////////////////////////////////////////////////////
// Function: mathSeedRandomNumberGenerator
// Description: Seed only once.
/////////////////////////////////////////////////////////
LOGICAL mathSeedRandomNumberGenerator()
{
    CHAOTICSEED chaoticSeed;
    UINT16 uSeed = 0;
    UINT32 uTime = 0;
    UINT32 i;

    static LOGICAL bSeeded = FALSE;

    if(bSeeded)
        return TRUE;

    bSeeded = TRUE;

    uTime = timeGetTime();

    memset(&chaoticSeed, 0, sizeof(CHAOTICSEED));

    chaoticSeed.X = 1.0f;
    chaoticSeed.Y = 1.0f;
    chaoticSeed.Z = 1.0f;

    chaoticSeed.A = 5.0f;
    chaoticSeed.B = 15.0f;
    chaoticSeed.C = 1.0f;
    chaoticSeed.dt = 0.02f;

	uSeed = (UINT16)uTime;

	*(UINT16*)((UCHAR*)&chaoticSeed.Y + 3) = uSeed;

	mathSetChaoticSeed(&chaoticSeed);

    for(i = 0; i < (uTime & 0xFF) + ((uTime & 0xFF00) >> 8); i++)
    {
		srand(mathGetChaoticNumber()+rand());
	}

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: mathGetChaoticNumber
// Description:
// HOOF NOTE:
// real life effects aren't ever truly random, in fact what looks like a random
// cloud of stuff is often a chaotic cloud of stuff (as per the Chaos theory
// definition of "chaotic") therefore a chaotic number generator will serve us
// much better than a random-number generator, so that's what we use this
// particular one is the Lorenz equations. Returns a value from the range
// of SINT16.
//////////////////////////////////////////////////////////////////////
SINT16 mathGetChaoticNumber()
{
    SINT32 number = GetRandom(0, 1<<30);

    return (SINT16)number;

#ifdef DONT_USE_THIS
    FLOAT64 X,Y,Z;
    SINT16 nReturn;

	X = g_chaoticSeed.X;
	Y = g_chaoticSeed.Y;
	Z = g_chaoticSeed.Z;

    g_chaoticSeed.X += -g_chaoticSeed.A * X * g_chaoticSeed.dt +
                        g_chaoticSeed.A * Y * g_chaoticSeed.dt;

    g_chaoticSeed.Y += g_chaoticSeed.B * X * g_chaoticSeed.dt -
                       Y * g_chaoticSeed.dt -
                       Z * X * g_chaoticSeed.dt;

    g_chaoticSeed.Z += -g_chaoticSeed.C * Z * g_chaoticSeed.dt +
                        X * Y * g_chaoticSeed.dt;

	nReturn = (SINT16)*(SINT16*)((UCHAR*)&g_chaoticSeed.Y + 3);

	return (SINT32)nReturn;
#endif
}

/////////////////////////////////////////////////////////
// Function: mathGetFloat32ChaoticNumber
// Description: returns a chaotic number from 0.0 to 1.0
/////////////////////////////////////////////////////////
FLOAT32 mathGetFloat32ChaoticNumber()
{
	return GetRandom(0.0f, 1.0f);
}

//////////////////////////////////////////////////////////////////////
// Function: mathGetFloat32ChaoticNumber
// Description: Simple, but predictable random number generator
//////////////////////////////////////////////////////////////////////
SINT8 mathGetRandomNumberFromSeed(SINT32* pSeed)
{
    *pSeed = *pSeed * 1103515245 + 12345;
    return (*pSeed >> 16) & 0xFF;
}

/*
    Fastest exponent function with 11-bit precision.

    Home page: www.imach.uran.ru/exp

    Copyright 2001-2002 by Dr. Raul N.Shakirov, IMach of RAS(UB),
    Phillip S. Pang, Ph.D. Biochemistry and Molecular Biophysics.
    Columbia University. NYC. All Rights Reserved.

    Permission has been granted to copy, distribute and modify
    software in any context without fee, including a commercial
    application, provided that the aforesaid copyright statement
    is present here as well as exhaustive description of changes.

    THE SOFTWARE IS DISTRIBUTED "AS IS". NO WARRANTY OF ANY KIND
    IS EXPRESSED OR IMPLIED. YOU USE AT YOUR OWN RISK. THE AUTHOR
    WILL NOT BE LIABLE FOR DATA LOSS, DAMAGES, LOSS OF PROFITS OR
    ANY OTHER KIND OF LOSS WHILE USING OR MISUSING THIS SOFTWARE.

    Method:     A Fast, Compact Approximation
                of the Exponential Function
                Technical Report IDSIA-07-98
                to appear in Neural Computation 11(4)
                Nicol N. Schraudolph
                ftp://ftp.idsia.ch/pub/nic/exp.ps.gz

    Name:       fexp

    Purpose:    11-bit precision exponent for Intel x86.

    Usage:      fexp (arg)

    Domain:     Same as for standard exp() function
                (approximately -709 <= arg <= 709).

    Result:     Approximate exp of arg, if within domain,
                otherwise undefined.
*/
FLOAT64 fexp(FLOAT64 arg)
{
#if platOS == OS_WIN32

    // Disable compile warning for size mismatch.
    #pragma warning(push)
    #pragma warning(disable: 4410)

    __asm fld   _eco_m
    __asm fmul  arg
    __asm fadd  _eco_a
    __asm fistp _eco.n.j

    return _eco.d;

    #pragma warning(pop)

#else

    return exp(arg);

#endif
}

//////////////////////////////////////////////////////////////////////
// Function: floatToInt
// Description: From NVIDIA fastmath.cpp. Handles negatives too.
//////////////////////////////////////////////////////////////////////
void mathFloatToInt(SINT32* int_pointer, FLOAT32 f)
{
#if platOS == OS_WIN32
  __asm  fld  f
  __asm  mov  edx,int_pointer
  __asm  FRNDINT
  __asm  fistp dword ptr [edx];
#else
    *int_pointer = FloatToInt(f);
#endif
}

//////////////////////////////////////////////////////////////////////
// Function: mathVectorToYAxis
// Description: Orient an arbitrary vector to the Y axis.
//////////////////////////////////////////////////////////////////////
void mathVectorToYAxis(piVECTOR* lpInV, piMATRIX* lpOutMatrix)
{
    if(lpInV == nullptr) return;
    if(lpOutMatrix == nullptr) return;

    piVECTOR n = *lpInV;
    piMATRIX matOrient = IdentityMatrix;

    math3dNormalize(&n);

    // First, rotate around Z to get on X/Z plane
    FLOAT32 d = math3dFastSqrt(n.y * n.y + n.x * n.x);

    if(d > 0.0001f)
    {
        FLOAT32 fCosTheta = n.x / d;
        FLOAT32 fSinTheta = n.y / d;

        piMATRIX matRotZ = IdentityMatrix;

        matRotZ._11 =  fCosTheta;
        matRotZ._12 = -fSinTheta;
        matRotZ._21 =  fSinTheta;
        matRotZ._22 =  fCosTheta;

        math3dVectorMultiply(&n, &n, &matRotZ);
        math3dMatrixMultiply(&matOrient, &matOrient, &matRotZ);
    }

    // Rotate around Y to get on  Z/Y plane
    d = math3dFastSqrt(n.z * n.z + n.x * n.x);

    if(d > 0.0001f)
    {
        FLOAT32 fSinPhi = n.x / d;
        FLOAT32 fCosPhi = n.z / d;

        piMATRIX matRotY = IdentityMatrix;
        matRotY._11 =  fCosPhi;
        matRotY._13 =  fSinPhi;
        matRotY._31 = -fSinPhi;
        matRotY._33 =  fCosPhi;

        math3dVectorMultiply(&n, &n, &matRotY);
        math3dMatrixMultiply(&matOrient, &matOrient, &matRotY);
    }

    // Rotate around X to align with Y axis
    d = math3dFastSqrt(n.z * n.z);

    if(d > 0.0001f)
    {
        FLOAT32 fSinAlpha = n.z / d;
        FLOAT32 fCosAlpha = n.y / d;

        piMATRIX matRotX = IdentityMatrix;

        matRotX._22 =  fCosAlpha;
        matRotX._23 = -fSinAlpha;
        matRotX._32 =  fSinAlpha;
        matRotX._33 =  fCosAlpha;

        math3dVectorMultiply(&n, &n, &matRotX);
        math3dMatrixMultiply(&matOrient, &matOrient, &matRotX);
    }

    *lpOutMatrix = matOrient;
}

//////////////////////////////////////////////////////////////////////
// Function: mathVectorToZAxis
// Description: Orient an arbitrary vector to the Z axis.
//////////////////////////////////////////////////////////////////////
void mathVectorToZAxis(piVECTOR* lpInV, piMATRIX* lpOutMatrix)
{
    if(lpInV == nullptr) return;
    if(lpOutMatrix == nullptr) return;

    piVECTOR n = *lpInV;
    piMATRIX matOrient = IdentityMatrix;
    FLOAT32 fCos, fSin;

    math3dNormalize(&n);

    // Rotate around Y.
    FLOAT32 d = math3dFastSqrt(n.x * n.x + n.z * n.z);

    if(d > 0.0000001f)
    {
        fCos = n.x / d;
        fSin = n.z / d;

        piMATRIX matRotY = IdentityMatrix;

        matRotY._11 =  fCos;
        matRotY._13 = -fSin;
        matRotY._31 =  fSin;
        matRotY._33 =  fCos;

        math3dVectorMultiply(&n, &n, &matRotY);
        math3dMatrixMultiply(&matOrient, &matOrient, &matRotY);
    }

    // Rotate Z
    d = math3dFastSqrt(n.x * n.x + n.y * n.y);

    if(d > 0.000001f)
    {
        fCos = n.y / d;
        fSin = n.x / d;

        piMATRIX matRotZ = IdentityMatrix;

        matRotZ._11 =  fCos;
        matRotZ._12 =  fSin;
        matRotZ._21 = -fSin;
        matRotZ._22 =  fCos;

        math3dVectorMultiply(&n, &n, &matRotZ);
        math3dMatrixMultiply(&matOrient, &matOrient, &matRotZ);
    }

    // Rotate X
    d = math3dFastSqrt(n.z * n.z + n.y * n.y);

    if(d > 0.000001f)
    {
        fCos = n.z / d;
        fSin = n.y / d;

        piMATRIX matRotX = IdentityMatrix;

        matRotX._22 =  fCos;
        matRotX._23 =  fSin;
        matRotX._32 = -fSin;
        matRotX._33 =  fCos;

        math3dVectorMultiply(&n, &n, &matRotX);
        math3dMatrixMultiply(&matOrient, &matOrient, &matRotX);
    }

    *lpOutMatrix = matOrient;
}

//////////////////////////////////////////////////////////////////////
// Function: closestPowerOfTwo
// Description: From humus@rogers.com
//////////////////////////////////////////////////////////////////////
UINT32 closestPowerOfTwo(UINT32 x)
{
    UINT32 i = 1;

    while(i < x)
    {
        i <<= 1;
    }

    if(4 * x < 3 * i)
    {
        i >>= 1;
    }

    return i;
}

//////////////////////////////////////////////////////////////////////
// Function: mathTransposeMatrix
// Description:
//////////////////////////////////////////////////////////////////////
void mathTransposeMatrix(piMATRIX& m)
{
    piMATRIX original = m;

    m._11 = original._11;
    m._12 = original._21;
    m._13 = original._31;
    m._14 = original._41;

    m._21 = original._12;
    m._22 = original._22;
    m._23 = original._32;
    m._24 = original._42;

    m._31 = original._13;
    m._32 = original._23;
    m._33 = original._33;
    m._34 = original._43;

    m._41 = original._14;
    m._42 = original._24;
    m._43 = original._34;
    m._44 = original._44;
}

//////////////////////////////////////////////////////////////////////
// Function: mathFastInvSqrt
// Description:
//////////////////////////////////////////////////////////////////////
FLOAT32 mathFastInvSqrt(FLOAT32 value)
{
    static FLOAT32 half = 0.0f;
    static int i = 0;

    half = 0.5f * value;

    i  = *(SINT32*)&value;
    i = 0x5f3759df - (i >> 1);
    value = *(FLOAT32*)&i;
    value = value * (1.5f - half * value * value);

    return value;
}

//////////////////////////////////////////////////////////////////////
// Function: mathReallyFastNormalize
// Description: '...when close enough is good enough...'
// Avoids the sqrt.
//////////////////////////////////////////////////////////////////////
void mathReallyFastNormalize(piVECTOR& in)
{
    static float invsqrt = 0.0f;

    invsqrt = mathFastInvSqrt(in.x * in.x + in.y * in.y + in.z * in.z);
    in.x *= invsqrt;
    in.y *= invsqrt;
    in.z *= invsqrt;
}
