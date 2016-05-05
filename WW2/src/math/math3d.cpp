#include <syscfg.h> // Path is project specific.

#include <math.h>
#include <float.h>
#include "pi/pi.h"
#include "include/vars.h"

#if !defined(NDEBUG)
  #define INLINE
#else
#if platOS == OS_MACOS
 #define INLINE static inline
#else
  #define INLINE static __forceinline
#endif
#endif

#define FP_ONE		0x3F800000

/*
 * Fast, Low Precision Reciprocal of float fx
 */
INLINE float math3dInvert(float fx)
{
    int i = 2 * FP_ONE - *(int*)&(fx);
    float r = *(float*)&i;
    r = r * (2.0f - (fx) * r);
	return(r);
}

/*
 * Fast, Low Precision Sqrt of float fx
 */
INLINE float math3dFastSqrt(float fx)
{
#if platOS == OS_LINUX
  union
  {
    FLOAT32	f;
    SINT32	d;
  }fd;
#else
struct
{
  union
  {
    FLOAT32	f;
    SINT32	d;
  };
}fd;
#endif

	fd.f = fx;
	fd.d -= FP_ONE;
	fd.d >>= 1;
	fd.d += FP_ONE;
	return(fd.f);
}

/*
 * Fast, Approximate Sqrt of float fx
 */
INLINE float math3dFastAppxSqrt(float fx)
{
#if platOS == OS_LINUX
  union
  {
    FLOAT32	f;
    SINT32	d;
  }fd;
#else
struct
{
  union
  {
    FLOAT32	f;
    SINT32	d;
	};
}fd;
#endif

	fd.f = fx;
	fd.d >>= 1;
	fd.d += 0x1fc00000;
	return(fd.f);
}

/*
 * Fast, Low Precision Normalize of vector v
 */
INLINE piVECTOR* math3dFastNormalize(piVECTOR* lpa)
{
float inv_mod;

	inv_mod = lpa->x * lpa->x + lpa->y * lpa->y + lpa->z * lpa->z;
	if (inv_mod)
	{
		//inv_mod = (float)(1.0f / math3dFastSqrt(inv_mod));
		inv_mod = math3dInvert(math3dFastAppxSqrt(inv_mod));

		lpa->x *= inv_mod;
		lpa->y *= inv_mod;
		lpa->z *= inv_mod;
	}

	return lpa;
}

/*
 * Normalizes the vector a
 */
#ifdef MATH3D_INLINE
#define math3dNormalize(lpa)										\
{																	\
float inv_mod;														\
																	\
	inv_mod = ((piVECTOR*)lpa)->x * ((piVECTOR*)lpa)->x +			\
              ((piVECTOR*)lpa)->y * ((piVECTOR*)lpa)->y +			\
              ((piVECTOR*)lpa)->z * ((piVECTOR*)lpa)->z;			\
	if (inv_mod)													\
	{																\
		inv_mod = (float)(1.0 / sqrt(inv_mod));						\
																	\
		((piVECTOR*)lpa)->x *= inv_mod;							\
		((piVECTOR*)lpa)->y *= inv_mod;							\
		((piVECTOR*)lpa)->z *= inv_mod;							\
	}																\
}
#else
INLINE piVECTOR* math3dNormalize(piVECTOR* lpa)
{
float inv_mod;

	inv_mod = lpa->x * lpa->x + lpa->y * lpa->y + lpa->z * lpa->z;
	if (inv_mod)
	{
		inv_mod = (float)(1.0 / sqrt(inv_mod));

		lpa->x *= inv_mod;
		lpa->y *= inv_mod;
		lpa->z *= inv_mod;
	}

	return lpa;
}
#endif	// MATH3D_INLINE

/*
 * Calculates cross product of a and b.
 */
INLINE piVECTOR* math3dCrossProduct(piVECTOR* lpd, piVECTOR* lpa, piVECTOR* lpb)
{
piVECTOR  d;

	d.x = lpa->y * lpb->z - lpa->z * lpb->y;
	d.y = lpa->z * lpb->x - lpa->x * lpb->z;
	d.z = lpa->x * lpb->y - lpa->y * lpb->x;

	*lpd = d;

	return lpd;
}


/*
 * Calculates dot product of a and b.
 */
#ifdef MATH3D_INLINE
#define math3dDotProduct(lpa, lpb)					\
(													\
	((piVECTOR*)lpa)->x * ((piVECTOR*)lpb)->x +	\
	((piVECTOR*)lpa)->y * ((piVECTOR*)lpb)->y +	\
	((piVECTOR*)lpa)->z * ((piVECTOR*)lpb)->z		\
)
#else
INLINE FLOAT32 math3dDotProduct(const piVECTOR* lpa, const piVECTOR* lpb)
{
	return(lpa->x * lpb->x + lpa->y * lpb->y + lpa->z * lpb->z);
}
#endif	// MATH3D_INLINE


/*
 * lpd = lpa * lpb
 * lpd can be equal to lpa or lpb
 */
INLINE piVECTOR* math3dMultiplyVectors(piVECTOR* lpd, piVECTOR* lpa, piVECTOR* lpb)
{
piVECTOR  d;

	d.x = lpa->x * lpb->x;
	d.y = lpa->y * lpb->y;
	d.z = lpa->z * lpb->z;

	*lpd = d;

	return lpd;
}


/*
 * lpd = lpa - lpb
 * lpd can be equal to lpa or lpb
 */
INLINE piVECTOR* math3dSubtractVectors(piVECTOR* lpd, const piVECTOR* lpa, const piVECTOR* lpb)
{
piVECTOR  d;

	d.x = lpa->x - lpb->x;
	d.y = lpa->y - lpb->y;
	d.z = lpa->z - lpb->z;

	*lpd = d;

	return lpd;
}


/*
 * lpd = lpa + lpb
 * lpd can be equal to lpa or lpb
 */
INLINE piVECTOR* math3dAddVectors(piVECTOR* lpd,  const piVECTOR* lpa,  const piVECTOR* lpb)
{
piVECTOR  d;

	d.x = lpa->x + lpb->x;
	d.y = lpa->y + lpb->y;
	d.z = lpa->z + lpb->z;

	*lpd = d;

	return lpd;
}


/*
 * lpd = lpa * b
 * lpd can be equal to lpa
 */
INLINE piVECTOR* math3dScaleVector(piVECTOR* lpd, const piVECTOR* lpa, float b)
{
	lpd->x = lpa->x * b;
	lpd->y = lpa->y * b;
	lpd->z = lpa->z * b;

	return lpd;
}


/*
 * lpd = lpa + lpb * c
 * lpd can be equal to lpa or lpb
 */
INLINE piVECTOR* math3dAddScaledVectors(piVECTOR* lpd, piVECTOR* lpa, piVECTOR* lpb, float c)
{
piVECTOR  d;

	d.x = lpa->x + (lpb->x * c);
	d.y = lpa->y + (lpb->y * c);
	d.z = lpa->z + (lpb->z * c);

	*lpd = d;

	return lpd;
}

/*
 * lpd = lpa * lpb
 * lpd can be equal to lpa or lpb
 */
// NOTE: this version can ONLY be used if matrix elements _14,_24,_34 are 0 and _44 is 1 for* BOTH* matricies
/*INLINE piMATRIX* math3dMatrixMultiplyOptimized(piMATRIX* lpd, piMATRIX* lpa, piMATRIX* lpb)
{
piMATRIX  d;

	d._11  = lpa->_11 * lpb->_11 + lpa->_12 * lpb->_21 + lpa->_13 * lpb->_31;// + lpa->_14 * lpb->_41;
	d._12  = lpa->_11 * lpb->_12 + lpa->_12 * lpb->_22 + lpa->_13 * lpb->_32;// + lpa->_14 * lpb->_42;
	d._13  = lpa->_11 * lpb->_13 + lpa->_12 * lpb->_23 + lpa->_13 * lpb->_33;// + lpa->_14 * lpb->_43;
//	d._14  = lpa->_11 * lpb->_14 + lpa->_12 * lpb->_24 + lpa->_13 * lpb->_34 + lpa->_14 * lpb->_44;
	d._14 = 0;

	d._21  = lpa->_21 * lpb->_11 + lpa->_22 * lpb->_21 + lpa->_23 * lpb->_31;// + lpa->_24 * lpb->_41;
	d._22  = lpa->_21 * lpb->_12 + lpa->_22 * lpb->_22 + lpa->_23 * lpb->_32;// + lpa->_24 * lpb->_42;
	d._23  = lpa->_21 * lpb->_13 + lpa->_22 * lpb->_23 + lpa->_23 * lpb->_33;// + lpa->_24 * lpb->_43;
//	d._24  = lpa->_21 * lpb->_14 + lpa->_22 * lpb->_24 + lpa->_23 * lpb->_34 + lpa->_24 * lpb->_44;
	d._24 = 0;

	d._31  = lpa->_31 * lpb->_11 + lpa->_32 * lpb->_21 + lpa->_33 * lpb->_31;// + lpa->_34 * lpb->_41;
	d._32  = lpa->_31 * lpb->_12 + lpa->_32 * lpb->_22 + lpa->_33 * lpb->_32;// + lpa->_34 * lpb->_42;
	d._33  = lpa->_31 * lpb->_13 + lpa->_32 * lpb->_23 + lpa->_33 * lpb->_33;// + lpa->_34 * lpb->_43;
//	d._34  = lpa->_31 * lpb->_14 + lpa->_32 * lpb->_24 + lpa->_33 * lpb->_34 + lpa->_34 * lpb->_44;
	d._34 = 0;

	d._41  = lpa->_41 * lpb->_11 + lpa->_42 * lpb->_21 + lpa->_43 * lpb->_31 + lpb->_41;// + lpa->_44 * lpb->_41;
	d._42  = lpa->_41 * lpb->_12 + lpa->_42 * lpb->_22 + lpa->_43 * lpb->_32 + lpb->_42;// + lpa->_44 * lpb->_42;
	d._43  = lpa->_41 * lpb->_13 + lpa->_42 * lpb->_23 + lpa->_43 * lpb->_33 + lpb->_43;// + lpa->_44 * lpb->_43;
//	d._44  = lpa->_41 * lpb->_14 + lpa->_42 * lpb->_24 + lpa->_43 * lpb->_34 + lpa->_44 * lpb->_44;
	d._44 = 1;

	*lpd = d;

	return lpd;
}
*/

INLINE void math3dMatrixMultiplyUnique(piMATRIX * lpd, piMATRIX const* lpa, piMATRIX const* lpb)
{
	lpd->_11  = lpa->_11 * lpb->_11 + lpa->_12 * lpb->_21 + lpa->_13 * lpb->_31 + lpa->_14 * lpb->_41;
	lpd->_12  = lpa->_11 * lpb->_12 + lpa->_12 * lpb->_22 + lpa->_13 * lpb->_32 + lpa->_14 * lpb->_42;
	lpd->_13  = lpa->_11 * lpb->_13 + lpa->_12 * lpb->_23 + lpa->_13 * lpb->_33 + lpa->_14 * lpb->_43;
	lpd->_14  = lpa->_11 * lpb->_14 + lpa->_12 * lpb->_24 + lpa->_13 * lpb->_34 + lpa->_14 * lpb->_44;

	lpd->_21  = lpa->_21 * lpb->_11 + lpa->_22 * lpb->_21 + lpa->_23 * lpb->_31 + lpa->_24 * lpb->_41;
	lpd->_22  = lpa->_21 * lpb->_12 + lpa->_22 * lpb->_22 + lpa->_23 * lpb->_32 + lpa->_24 * lpb->_42;
	lpd->_23  = lpa->_21 * lpb->_13 + lpa->_22 * lpb->_23 + lpa->_23 * lpb->_33 + lpa->_24 * lpb->_43;
	lpd->_24  = lpa->_21 * lpb->_14 + lpa->_22 * lpb->_24 + lpa->_23 * lpb->_34 + lpa->_24 * lpb->_44;

	lpd->_31  = lpa->_31 * lpb->_11 + lpa->_32 * lpb->_21 + lpa->_33 * lpb->_31 + lpa->_34 * lpb->_41;
	lpd->_32  = lpa->_31 * lpb->_12 + lpa->_32 * lpb->_22 + lpa->_33 * lpb->_32 + lpa->_34 * lpb->_42;
	lpd->_33  = lpa->_31 * lpb->_13 + lpa->_32 * lpb->_23 + lpa->_33 * lpb->_33 + lpa->_34 * lpb->_43;
	lpd->_34  = lpa->_31 * lpb->_14 + lpa->_32 * lpb->_24 + lpa->_33 * lpb->_34 + lpa->_34 * lpb->_44;

	lpd->_41  = lpa->_41 * lpb->_11 + lpa->_42 * lpb->_21 + lpa->_43 * lpb->_31 + lpa->_44 * lpb->_41;
	lpd->_42  = lpa->_41 * lpb->_12 + lpa->_42 * lpb->_22 + lpa->_43 * lpb->_32 + lpa->_44 * lpb->_42;
	lpd->_43  = lpa->_41 * lpb->_13 + lpa->_42 * lpb->_23 + lpa->_43 * lpb->_33 + lpa->_44 * lpb->_43;
	lpd->_44  = lpa->_41 * lpb->_14 + lpa->_42 * lpb->_24 + lpa->_43 * lpb->_34 + lpa->_44 * lpb->_44;
}

INLINE void math3dMatrixMultiply(piMATRIX* lpd, piMATRIX const* lpa, piMATRIX const* lpb)
{
piMATRIX  d;

	d._11  = lpa->_11 * lpb->_11 + lpa->_12 * lpb->_21 + lpa->_13 * lpb->_31 + lpa->_14 * lpb->_41;
	d._12  = lpa->_11 * lpb->_12 + lpa->_12 * lpb->_22 + lpa->_13 * lpb->_32 + lpa->_14 * lpb->_42;
	d._13  = lpa->_11 * lpb->_13 + lpa->_12 * lpb->_23 + lpa->_13 * lpb->_33 + lpa->_14 * lpb->_43;
	d._14  = lpa->_11 * lpb->_14 + lpa->_12 * lpb->_24 + lpa->_13 * lpb->_34 + lpa->_14 * lpb->_44;

	d._21  = lpa->_21 * lpb->_11 + lpa->_22 * lpb->_21 + lpa->_23 * lpb->_31 + lpa->_24 * lpb->_41;
	d._22  = lpa->_21 * lpb->_12 + lpa->_22 * lpb->_22 + lpa->_23 * lpb->_32 + lpa->_24 * lpb->_42;
	d._23  = lpa->_21 * lpb->_13 + lpa->_22 * lpb->_23 + lpa->_23 * lpb->_33 + lpa->_24 * lpb->_43;
	d._24  = lpa->_21 * lpb->_14 + lpa->_22 * lpb->_24 + lpa->_23 * lpb->_34 + lpa->_24 * lpb->_44;

	d._31  = lpa->_31 * lpb->_11 + lpa->_32 * lpb->_21 + lpa->_33 * lpb->_31 + lpa->_34 * lpb->_41;
	d._32  = lpa->_31 * lpb->_12 + lpa->_32 * lpb->_22 + lpa->_33 * lpb->_32 + lpa->_34 * lpb->_42;
	d._33  = lpa->_31 * lpb->_13 + lpa->_32 * lpb->_23 + lpa->_33 * lpb->_33 + lpa->_34 * lpb->_43;
	d._34  = lpa->_31 * lpb->_14 + lpa->_32 * lpb->_24 + lpa->_33 * lpb->_34 + lpa->_34 * lpb->_44;

	d._41  = lpa->_41 * lpb->_11 + lpa->_42 * lpb->_21 + lpa->_43 * lpb->_31 + lpa->_44 * lpb->_41;
	d._42  = lpa->_41 * lpb->_12 + lpa->_42 * lpb->_22 + lpa->_43 * lpb->_32 + lpa->_44 * lpb->_42;
	d._43  = lpa->_41 * lpb->_13 + lpa->_42 * lpb->_23 + lpa->_43 * lpb->_33 + lpa->_44 * lpb->_43;
	d._44  = lpa->_41 * lpb->_14 + lpa->_42 * lpb->_24 + lpa->_43 * lpb->_34 + lpa->_44 * lpb->_44;

	*lpd = d;
}

/*
 * lpd = lpa * lpb
 * lpd can be equal to lpa
 */
#ifdef MATH3D_INLINE
#define math3dVectorMultiply(lpd, lpa, lpb)																	\
{																											\
piVECTOR	d;																								\
																											\
	d.x = ((const piVECTOR*)lpa)->x * ((piMATRIX*)lpb)->_11 + ((const piVECTOR*)lpa)->y * ((piMATRIX*)lpb)->_21 +	\
          ((const piVECTOR*)lpa)->z * ((piMATRIX*)lpb)->_31 + ((piMATRIX*)lpb)->_41;							\
	d.y = ((const piVECTOR*)lpa)->x * ((piMATRIX*)lpb)->_12 + ((const piVECTOR*)lpa)->y * ((piMATRIX*)lpb)->_22 +	\
	      ((const piVECTOR*)lpa)->z * ((piMATRIX*)lpb)->_32 + ((piMATRIX*)lpb)->_42;							\
	d.z = ((const piVECTOR*)lpa)->x * ((piMATRIX*)lpb)->_13 + ((const piVECTOR*)lpa)->y * ((piMATRIX*)lpb)->_23 +	\
          ((const piVECTOR*)lpa)->z * ((piMATRIX*)lpb)->_33 + ((piMATRIX*)lpb)->_43;							\
																											\
	*lpd = d;																								\
}
#else
INLINE piVECTOR* math3dVectorMultiply(piVECTOR* lpd, const piVECTOR* lpa, const piMATRIX* lpb)
{
piVECTOR	d;

	d.x = lpa->x * lpb->_11 + lpa->y * lpb->_21 + lpa->z * lpb->_31 + lpb->_41;
	d.y = lpa->x * lpb->_12 + lpa->y * lpb->_22 + lpa->z * lpb->_32 + lpb->_42;
	d.z = lpa->x * lpb->_13 + lpa->y * lpb->_23 + lpa->z * lpb->_33 + lpb->_43;

	*lpd = d;

	return lpd;
}
#endif	// MATH3D_INLINE

/*
 * lpd = lpa * lpb
 * lpd can be equal to lpa
 */
INLINE piVECTOR* math3dMultiplyRotation(piVECTOR* lpd, const piVECTOR* lpa, const piMATRIX* lpb)
{
piVECTOR	d;

	d.x = lpa->x * lpb->_11 + lpa->y * lpb->_21 + lpa->z * lpb->_31;
	d.y = lpa->x * lpb->_12 + lpa->y * lpb->_22 + lpa->z * lpb->_32;
	d.z = lpa->x * lpb->_13 + lpa->y * lpb->_23 + lpa->z * lpb->_33;

	*lpd = d;

	return lpd;
}


/*
 * lpd = lpa * lpb
 * lpd can be equal to lpa
 */
INLINE piVECTOR* math3dMultiplyTransformation(piVECTOR* lpd, piVECTOR* lpa, piMATRIX* lpb)
{
piVECTOR	d;

	d.x = lpa->x + lpb->_41;
	d.y = lpa->y + lpb->_42;
	d.z = lpa->z + lpb->_43;

	*lpd = d;

	return lpd;
}


/*
 */
INLINE piVECTOR* math3dMatrixToVector(piVECTOR* lpd, piMATRIX* lpa)
{
	lpd->x = lpa->_11 + lpa->_21 + lpa->_31 + lpa->_41;
	lpd->y = lpa->_12 + lpa->_22 + lpa->_32 + lpa->_42;
	lpd->z = lpa->_13 + lpa->_23 + lpa->_33 + lpa->_43;

	return lpd;
}


/*
 * -1 d = a
 */
INLINE piMATRIX* math3dInvert1(piMATRIX* lpa)
{
piMATRIX d;

	d._11 = lpa->_11;
	d._12 = lpa->_21;
	d._13 = lpa->_31;
	d._14 = lpa->_41;

	d._21 = lpa->_12;
	d._22 = lpa->_22;
	d._23 = lpa->_32;
	d._24 = lpa->_42;

	d._31 = lpa->_13;
	d._32 = lpa->_23;
	d._33 = lpa->_33;
	d._34 = lpa->_43;

	d._41 = lpa->_14;
	d._42 = lpa->_24;
	d._43 = lpa->_34;
	d._44 = lpa->_44;

	*lpa = d;

	return lpa;
}


/*
 * -1 lpd = lpa
 */
INLINE piMATRIX* math3dInvert2(piMATRIX* lpd, piMATRIX* lpa)
{
piMATRIX d;

	d._11 = lpa->_11;
	d._12 = lpa->_21;
	d._13 = lpa->_31;
	d._14 = lpa->_41;

	d._21 = lpa->_12;
	d._22 = lpa->_22;
	d._23 = lpa->_32;
	d._24 = lpa->_42;

	d._31 = lpa->_13;
	d._32 = lpa->_23;
	d._33 = lpa->_33;
	d._34 = lpa->_43;

	d._41 = lpa->_14;
	d._42 = lpa->_24;
	d._43 = lpa->_34;
	d._44 = lpa->_44;

	*lpd = d;

	return lpd;
}


//////////////////////////////////////////////////////////////////////
//	math3dInvertMatrixComplete
//
//	Performs Cramers rule of inverting a square and invertible matrix (which isn't being checked).
//////////////////////////////////////////////////////////////////////
INLINE void math3dInvertMatrixComplete(piMATRIX& inverse, const piMATRIX& matrix)
{
	const	FLOAT32*	mat	= &matrix._11;
			FLOAT32*	dst	= &inverse._11;
			FLOAT32 tmp[12];								// temp array for pairs
			FLOAT32 src[16];								// array of transpose source matrix
			FLOAT32 det;									// determinant


	// transpose matrix
	for(UINT32 i = 0; i < 4; i++)
	{
		src[i] = mat[i*4];
		src[i + 4] = mat[i*4 + 1];
		src[i + 8] = mat[i*4 + 2];
		src[i + 12] = mat[i*4 + 3];
	}

	// calculate pairs for first 8 elements (cofactors)
	tmp[0] = src[10] * src[15];
	tmp[1] = src[11] * src[14];
	tmp[2] = src[9] * src[15];
	tmp[3] = src[11] * src[13];
	tmp[4] = src[9] * src[14];
	tmp[5] = src[10] * src[13];
	tmp[6] = src[8] * src[15];
	tmp[7] = src[11] * src[12];
	tmp[8] = src[8] * src[14];
	tmp[9] = src[10] * src[12];
	tmp[10] = src[8] * src[13];
	tmp[11] = src[9] * src[12];

	// calculate first 8 elements (cofactors)
	dst[0] = tmp[0]*src[5] + tmp[3]*src[6] + tmp[4]*src[7];
	dst[0] -= tmp[1]*src[5] + tmp[2]*src[6] + tmp[5]*src[7];
	dst[1] = tmp[1]*src[4] + tmp[6]*src[6] + tmp[9]*src[7];
	dst[1] -= tmp[0]*src[4] + tmp[7]*src[6] + tmp[8]*src[7];
	dst[2] = tmp[2]*src[4] + tmp[7]*src[5] + tmp[10]*src[7];
	dst[2] -= tmp[3]*src[4] + tmp[6]*src[5] + tmp[11]*src[7];
	dst[3] = tmp[5]*src[4] + tmp[8]*src[5] + tmp[11]*src[6];
	dst[3] -= tmp[4]*src[4] + tmp[9]*src[5] + tmp[10]*src[6];
	dst[4] = tmp[1]*src[1] + tmp[2]*src[2] + tmp[5]*src[3];
	dst[4] -= tmp[0]*src[1] + tmp[3]*src[2] + tmp[4]*src[3];
	dst[5] = tmp[0]*src[0] + tmp[7]*src[2] + tmp[8]*src[3];
	dst[5] -= tmp[1]*src[0] + tmp[6]*src[2] + tmp[9]*src[3];
	dst[6] = tmp[3]*src[0] + tmp[6]*src[1] + tmp[11]*src[3];
	dst[6] -= tmp[2]*src[0] + tmp[7]*src[1] + tmp[10]*src[3];
	dst[7] = tmp[4]*src[0] + tmp[9]*src[1] + tmp[10]*src[2];
	dst[7] -= tmp[5]*src[0] + tmp[8]*src[1] + tmp[11]*src[2];

	// calculate pairs for second 8 elements (cofactors)
	tmp[0] = src[2]*src[7];
	tmp[1] = src[3]*src[6];
	tmp[2] = src[1]*src[7];
	tmp[3] = src[3]*src[5];
	tmp[4] = src[1]*src[6];
	tmp[5] = src[2]*src[5];
	tmp[6] = src[0]*src[7];
	tmp[7] = src[3]*src[4];
	tmp[8] = src[0]*src[6];
	tmp[9] = src[2]*src[4];
	tmp[10] = src[0]*src[5];
	tmp[11] = src[1]*src[4];

	// calculate second 8 elements (cofactors)
	dst[8] = tmp[0]*src[13] + tmp[3]*src[14] + tmp[4]*src[15];
	dst[8] -= tmp[1]*src[13] + tmp[2]*src[14] + tmp[5]*src[15];
	dst[9] = tmp[1]*src[12] + tmp[6]*src[14] + tmp[9]*src[15];
	dst[9] -= tmp[0]*src[12] + tmp[7]*src[14] + tmp[8]*src[15];
	dst[10] = tmp[2]*src[12] + tmp[7]*src[13] + tmp[10]*src[15];
	dst[10]-= tmp[3]*src[12] + tmp[6]*src[13] + tmp[11]*src[15];
	dst[11] = tmp[5]*src[12] + tmp[8]*src[13] + tmp[11]*src[14];
	dst[11]-= tmp[4]*src[12] + tmp[9]*src[13] + tmp[10]*src[14];
	dst[12] = tmp[2]*src[10] + tmp[5]*src[11] + tmp[1]*src[9];
	dst[12]-= tmp[4]*src[11] + tmp[0]*src[9] + tmp[3]*src[10];
	dst[13] = tmp[8]*src[11] + tmp[0]*src[8] + tmp[7]*src[10];
	dst[13]-= tmp[6]*src[10] + tmp[9]*src[11] + tmp[1]*src[8];
	dst[14] = tmp[6]*src[9] + tmp[11]*src[11] + tmp[3]*src[8];
	dst[14]-= tmp[10]*src[11] + tmp[2]*src[8] + tmp[7]*src[9];
	dst[15] = tmp[10]*src[10] + tmp[4]*src[8] + tmp[9]*src[9];
	dst[15]-= tmp[8]*src[9] + tmp[11]*src[10] + tmp[5]*src[8];

	// calculate determinant
	det=src[0]*dst[0]+src[1]*dst[1]+src[2]*dst[2]+src[3]*dst[3];

	// calculate matrix inverse
	det = 1/det;
	for(int j = 0; j < 16; j++)
	{
		dst[j] *= det;
	}
}


/*
 * lpd = 1 / lpa
 */
INLINE piMATRIX* math3dInvertMatrix(piMATRIX* lpd, piMATRIX* lpa)
{
piMATRIX	d;

	d = IdentityMatrix;

	d._11 = lpa->_11;
	d._12 = lpa->_21;
	d._13 = lpa->_31;

	d._21 = lpa->_12;
	d._22 = lpa->_22;
	d._23 = lpa->_32;

	d._31 = lpa->_13;
	d._32 = lpa->_23;
	d._33 = lpa->_33;

	d._41 = d._11 * -lpa->_41 + d._21 * -lpa->_42 + d._31 * -lpa->_43;
	d._42 = d._12 * -lpa->_41 + d._22 * -lpa->_42 + d._32 * -lpa->_43;
	d._43 = d._13 * -lpa->_41 + d._23 * -lpa->_42 + d._33 * -lpa->_43;

	*lpd = d;

	return lpd;
}


/*
 * Three degrees of rotation via the vector in lpa.
 */
INLINE piMATRIX* math3dMatrixRotate(piMATRIX* lpd, piVECTOR* lpa)
{
float	s, c;
piMATRIX p, r, y, t;

	/*
	 * Init the vector and apply x, y and z rotations.
	 */

	// pitch
	c = (float)cos(lpa->x);
	s = (float)sin(lpa->x);

	p = IdentityMatrix;
	p._22 = c;
	p._33 = c;
	p._23 = -s;
	p._32 = s;

	// roll
	c = (float)cos(lpa->y);
	s = (float)sin(lpa->y);

	r = IdentityMatrix;
	r._11 = c;
	r._33 = c;
	r._31 = -s;
	r._13 = s;
	math3dMatrixMultiply(&t, &r, &p);

	// yaw
	c = (float)cos(lpa->z);
	s = (float)sin(lpa->z);

	y = IdentityMatrix;
	y._11 = c;
	y._22 = c;
	y._12 = -s;
	y._21 = s;

	math3dMatrixMultiply(&t, &y, &t);

	math3dMatrixMultiply(lpd, lpd, &t);

	return lpd;
}


/*
 * Three degrees of rotation via the vector in lpa.
 */
INLINE piVECTOR* math3dVectorRotate(piVECTOR* lpd, piVECTOR* lpa)
{
float   t1, t2;
float   c, s;
float   x = lpd->x;
float   y = lpd->y;
float   z = lpd->z;

	// Roll
	t1 = x;
	t2 = y;
	c = (float)cos(lpa->y);
	s = (float)sin(lpa->y);
	x =  t1 * c + t2 * s;
	y = -t1 * s + t2 * c;

	// Pitch
	t1 = y;
	t2 = z;
	c = (float)cos(lpa->x);
	s = (float)sin(lpa->x);
	y =  t1 * c + t2 * s;
	z = -t1 * s + t2 * c;

	// Yaw
	t1 = x;
	t2 = z;
	c = (float)cos(lpa->z);
	s = (float)sin(lpa->z);
	x =  t1 * c + t2 * s;
	z = -t1 * s + t2 * c;

	lpd->x = x;
	lpd->y = y;
	lpd->z = z;

	return(lpd);
}


/*
 * Calculate a normal based on a rotation vector
 */
INLINE piVECTOR* math3dCalcNormalVector(piVECTOR* lpd, piVECTOR* lpa)
{
float cx, cy, cz, sx, sy, sz;

	// precalc intrinsic functions
	cx = (float)cos(lpa->x);
	cy = (float)cos(lpa->y);
	cz = (float)cos(lpa->z);
	sx = (float)sin(lpa->x);
	sy = (float)sin(lpa->y);
	sz = (float)sin(lpa->z);

	// normal vector straight ahead
	lpd->x =  (cx*sy);
	lpd->z = -(sx);
	lpd->y = -(cx*cy);

	return(lpd);
}

/*
Line equation: p = org + u * dir                             (1)
Plane equation: p * normal - k = 0.                          (2)

Substituting (1) into (2) and rearranging we get:

(org + u * dir) * normal - k = 0

ie  u * dir * normal = k - org * normal
ie  u = (k - org * normal) / (dir * normal)

If (dir * normal) = 0 then the line runs parallel to the plane and no intersection occurs.
The exact point at which intersection does occur can be found by plugging u back into the
line equation in (1).

Notes:

a) Both u and k are scalars.
b) u is the length of a line.
c) k is the location of a plane on it's normal and is the same for all points on the plane.
*/



/*
	equatorial to alt-az
	altitude = arcsin(sin(dec)*sin(lat) + cos(dec)*cos(lat)*cos(ha))
	azimuth = arccos((sin(dec) - sin(lat)*sin(alt))/(cos(lat)*cos(alt)))

	alt-az to equatorial
	dec = arcsin(sin(alt)*sin(lat) + cos(alt)*cos(lat)*cos(az))
	ha = arccos((sin(alt) - sin(lat)*sin(dec))/(cos(lat)*cos(dec)))
*/

/*
	Note: The co-ordinate system I use here is:
	with xrot = 0, yrot = 0, and zrot = 0

	x points right, y points up, and z points into the screen.
	+pitch pulls up on the stick, +yaw turns right, and +roll roll to the right.
	zrot, xrot, and yrot follow the directions of roll, pitch, and yaw.
	In other words, zrot is right handed, xrot is right handed, and yrot
	is lefthanded.

	This is a little bit oddball, since most texts tell you to pick one hand
	and stick with it.  To change the handedness of a rotation variable,
	the simplest way is to reverse the sign of the variable at the beginning
	and end of the orientship procedure, the way I did with yaw.


	By the way, if you want to move your object in the direction it is
	pointing, you will need to convert the rotation angles to vectors.


	x += speed*cos(xrot)*sin(yrot));
	y += speed*sin(xrot));
	z += speed*cos(xrot)*cos(yrot);
*/

static void alt_az_to_eq(float alt, float az, float* dec, float* ha, float lat)
{
float sin_dec, cos_ha, sinalt, cosalt, sinlat, coslat;

	sinalt = (float)sin(alt);
	cosalt = (float)cos(alt);
	sinlat = (float)sin(lat);
	coslat = (float)cos(lat);

	sin_dec = sinalt * sinlat + cosalt * coslat * (float)cos(az);
	if (sin_dec == -1.0f)
	{
		*dec = -(float)PI * 0.5f;;
	}
	else
	{
		*dec = (float)asin(sin_dec);
	}

	if (sin_dec == 1)
	{
		cos_ha = 1;
	}
	else
	{
		cos_ha = (sinalt - sinlat * sin_dec) / (coslat * (float)cos(*dec));
	}

	if (cos_ha > 1)
	{
		cos_ha = 1;
	}

	if (cos_ha < -1)
	{
		cos_ha = -1;
	}

	*ha = (float)acos(cos_ha);
	if (sin(az) > 0)
	{
		*ha = PI2 - *ha;
	}
}


/*
 * Using the delta angles (pitch, yaw, roll) in lpa,
 * update the angles in lpd to make the new direction.
 */
INLINE piVECTOR* math3dOrientAngles(piVECTOR* lpd, piVECTOR* lpa)
{
float ha, ha2, dec, lat, lon, alt, az;

	// locate axis of rotation
	if (lpa->x == 0 && lpa->y == 0)
	{
		lpd->z += lpa->z;
		while (lpd->z < 0)
		{
			lpd->z += PI2;
		}

		while (lpd->z >= PI2)
		{
			lpd->z -= PI2;
		}

		return(lpd);
	}

	// correct for reverse yaw angle(turn right = +)
	// now all inputs are right hand angles
	lpa->y = -lpa->y;

	az = lpd->z + (float)atan2(lpa->x, lpa->y);
	lpd->z -= az;

	// locate nose of ship
	lat = lpd->x;
	lon = lpd->y;

	alt = (float)atan2(lpa->z, (float)sqrt(lpa->x * lpa->x + lpa->y * lpa->y));
	alt_az_to_eq(alt, az, &dec, &ha, lat);

	// locate az of ship
	lat = dec;

	alt_az_to_eq(lpd->x, -ha, &alt, &az, lat);

	az += (float)sqrt(lpa->x * lpa->x + lpa->y * lpa->y + lpa->z * lpa->z);

	alt_az_to_eq(alt, az, &dec, &ha2, lat);

	alt_az_to_eq(lat, ha2, &alt, &az, dec);

	lpd->x = dec;

#if platOS == OS_MACOS
	if (!std::isnan(lpd->x) && isfinite(lpd->x))
#else
	if (!_isnan(lpd->x) && _finite(lpd->x))
#endif
	{
		while (lpd->x < 0)
		{
			lpd->x += PI2;
		}

		while (lpd->x >= PI2)
		{
			lpd->x -= PI2;
		}
	}
	else
	{
		lpd->x = 0;
	}

	lpd->y = lon - ha - ha2;
#if platOS == OS_MACOS
	if (!std::isnan(lpd->y) && isfinite(lpd->y))
#else
	if (!_isnan(lpd->y) && _finite(lpd->y))
#endif
	{
		while(lpd->y < 0)
		{
			lpd->y += PI2;
		}

		while (lpd->y >= PI2)
		{
			lpd->y -= PI2;
		}
	}
	else
	{
		lpd->y = 0;
	}

	lpd->z -= az;
#if platOS == OS_MACOS
	if (!std::isnan(lpd->z) && isfinite(lpd->z))
#else
	if (!_isnan(lpd->z) && _finite(lpd->z))
#endif
	{
		while (lpd->z < 0)
		{
			lpd->z += PI2;
		}

		while (lpd->z >= PI2)
		{
			lpd->z -= PI2;
		}
	}
	else
	{
		lpd->z = 0;
	}

	return(lpd);
}

// Spherical Linear Interpolation (Slerp)
// t is the 0.0 to 1.0 interpolation value
// with 0.0 returning from, and 1.0 returning to

// this code is a modified version of the code found at
// http://www.gamasutra.com/features/programming/19980703/quaternions_09.htm
INLINE piQUAT* math3dQuatSlerp(piQUAT* res,piQUAT* from, piQUAT* to, FLOAT32 t)
{
FLOAT32           to1[4];
FLOAT64        omega, cosom, sinom, scale0, scale1;

	// calc cosine
	cosom = from->x * to->x + from->y * to->y + from->z * to->z
						  + from->w * to->w;

	// adjust signs (if necessary)
	if ( cosom <0.0 )
	{
		cosom = -cosom; to1[0] = - to->x;
		to1[1] = - to->y;
		to1[2] = - to->z;
		to1[3] = - to->w;
	}
	else
	{
		to1[0] = to->x;
		to1[1] = to->y;
		to1[2] = to->z;
		to1[3] = to->w;
	}

	// calculate coefficients

	if ( (1.0 - cosom) > 0.001 )
	{
		// standard case (slerp)
		omega = acos(cosom);
		sinom = sin(omega);
		scale0 = sin((1.0 - t) * omega) / sinom;
		scale1 = sin(t * omega) / sinom;

	}
	else
	{
	// "from" and "to" quaternions are very close
	//  ... so we can do a linear interpolation
		scale0 = 1.0 - t;
		scale1 = t;
	}

	// calculate final values
	res->x = (FLOAT32)(scale0 * from->x + scale1 * to1[0]);
	res->y = (FLOAT32)(scale0 * from->y + scale1 * to1[1]);
	res->z = (FLOAT32)(scale0 * from->z + scale1 * to1[2]);
	res->w = (FLOAT32)(scale0 * from->w + scale1 * to1[3]);

	return res;
}

// this code is a modified version of the code found at
// http://www.gamasutra.com/features/programming/19980703/quaternions_08.htm
INLINE piQUAT* math3dMutliplyQuaternion(piQUAT* res,piQUAT* q1,piQUAT* q2)
{

FLOAT32 A, B, C, D, E, F, G, H;

	A = (q1->w + q1->x)*(q2->w + q2->x);
	B = (q1->z - q1->y)*(q2->y - q2->z);
	C = (q1->x - q1->w)*(q2->y + q2->z);
	D = (q1->y + q1->z)*(q2->x - q2->w);
	E = (q1->x + q1->z)*(q2->x + q2->y);
	F = (q1->x - q1->z)*(q2->x - q2->y);
	G = (q1->w + q1->y)*(q2->w - q2->z);
	H = (q1->w - q1->y)*(q2->w + q2->z);

	res->w = B + (-E - F + G + H) /2;
	res->x = A - (E + F + G + H)/2;
	res->y = -C + (E - F + G - H)/2;
	res->z = - D + (E - F - G + H)/2;

	return res;
}

// this code is a modified version of the code found at
// http://www.gamasutra.com/features/programming/19980703/quaternions_08.htm
INLINE piMATRIX* math3dQuaternionToMatrix(piMATRIX* Mat, piQUAT* quat)
{

FLOAT32 wx, wy, wz, xx, yy, yz, xy, xz, zz, x2, y2, z2;

	// calculate coefficients
	x2 = quat->x + quat->x;
	y2 = quat->y + quat->y;
	z2 = quat->z + quat->z;
	xx = quat->x * x2;
	xy = quat->x * y2;
	xz = quat->x * z2;
	yy = quat->y * y2;
	yz = quat->y * z2;
	zz = quat->z * z2;
	wx = quat->w * x2;
	wy = quat->w * y2;
	wz = quat->w * z2;

	Mat->m[0][0] = 1.0f - (yy + zz);
	Mat->m[0][1] = xy - wz;
	Mat->m[0][2] = xz + wy;
	Mat->m[0][3] = 0.0f;

	Mat->m[1][0] = xy + wz;
	Mat->m[1][1] = 1.0f - (xx + zz);
	Mat->m[1][2] = yz - wx;
	Mat->m[1][3] = 0.0f;

	Mat->m[2][0] = xz - wy;
	Mat->m[2][1] = yz + wx;
	Mat->m[2][2] = 1.0f - (xx + yy);
	Mat->m[2][3] = 0.0f;

	Mat->m[3][0] = 0.0f;
	Mat->m[3][1] = 0.0f;
	Mat->m[3][2] = 0.0f;
	Mat->m[3][3] = 1.0f;

//	math3dInvertMatrix(Mat,Mat);

	return Mat;
}

// this code is a modified version of the code found at
// http://www.gamasutra.com/features/programming/19980703/quaternions_07.htm
INLINE piQUAT* math3dMatrixToQuaternion(piQUAT* quat, piMATRIX* InMat)
{

//   MatToQuat(float m[4][4], QUAT * quat)
//   {
FLOAT32  tr, s, q[4];
SINT32    i, j, k;
SINT32 nxt[3] = {1, 2, 0};
piMATRIX TempMat;
piMATRIX * Mat;

	if (PN_UNLIKELY(InMat == nullptr))
	{
		quat->x = quat->y = quat->z = quat->w = 0;
		return quat;
	}

	TempMat = *InMat;
	Mat = &TempMat;
	math3dInvertMatrix(Mat,Mat);

	tr = Mat->m[0][0] + Mat->m[1][1] + Mat->m[2][2];

     // check the diagonal
	if (tr > 0.0)
	{
		s = (FLOAT32)sqrt (tr + 1.0);
		quat->w = s / 2.0f;
		s = 0.5f / s;
		quat->x = (Mat->m[1][2] - Mat->m[2][1]) * s;
		quat->y = (Mat->m[2][0] - Mat->m[0][2]) * s;
		quat->z = (Mat->m[0][1] - Mat->m[1][0]) * s;
	}
	else
	{
		// diagonal is negative
		i = 0;
		if (Mat->m[1][1] > Mat->m[0][0])
		{
			i = 1;
		}
		if (Mat->m[2][2] > Mat->m[i][i])
		{
			i = 2;
		}
		j = nxt[i];
		k = nxt[j];

		s = (FLOAT32)(sqrt ((Mat->m[i][i] - (Mat->m[j][j] + Mat->m[k][k])) + 1.0f));

		q[i] = s * 0.5f;

		if (s != 0.0f)
		{
			s = 0.5f / s;
		}

		q[3] = (Mat->m[j][k] - Mat->m[k][j]) * s;
		q[j] = (Mat->m[i][j] + Mat->m[j][i]) * s;
		q[k] = (Mat->m[i][k] + Mat->m[k][i]) * s;

		quat->x = q[0];
		quat->y = q[1];
		quat->z = q[2];
		quat->w = q[3];
	}

	return quat;
}



/*
 * Re-Orthogonalize a matrix using Gram-Schmidt
 */
INLINE piMATRIX* math3dReorthoMatrix(piMATRIX* lpd, piMATRIX* lpa)
{
FLOAT32		dp;
piVECTOR	v1, v2, v3, vt;
piMATRIX	d;

	d._41 = lpa->_41;
	d._42 = lpa->_42;
	d._43 = lpa->_43;
	d._14 = d._24 = d._34 = 0.0f;
	d._44 = 1.0f;

	v1.x = lpa->_13;
	v1.y = lpa->_23;
	v1.z = lpa->_33;

	math3dNormalize(&v1);
	d._13 = v1.x;
	d._23 = v1.y;
	d._33 = v1.z;

	v2.x = lpa->_12;
	v2.y = lpa->_22;
	v2.z = lpa->_32;

	dp = math3dDotProduct(&v2, &v1);
	math3dScaleVector(&vt, &v1, dp);
	math3dSubtractVectors(&v2, &v2, &vt);
	math3dNormalize(&v2);
	d._12 = v2.x;
	d._22 = v2.y;
	d._32 = v2.z;

	math3dCrossProduct(&v3, &v2, &v1);
	d._11 = v3.x;
	d._21 = v3.y;
	d._31 = v3.z;

	*lpd = d;

	return(lpd);
}

#undef INLINE
