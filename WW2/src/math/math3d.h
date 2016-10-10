#pragma once

#include <math.h>
#include "pi/pi.h"

// Find out who is calling sqrt with incorrect params
#define DEBUG_SQRT  0
#define DEBUG_POW   0
#define DEBUG_ATAN2 0
#define DEBUG_MATH3DNORMALIZE   0

#define isPowerOf2(x) !(x & (x - 1))

// Rotate Vec3f around z.
// rot = Vec2f
// rot.x() = cos
// rot.y() = sin
#define ROTATE_VECTOR_Z(rot, vec) \
    Vec3f(rot.x() * vec.x() + rot.y() * vec.y(), \
         -rot.y() * vec.x() + rot.x() * vec.y(), \
          vec.z())

//#define b2(x)   ((x) | ((x) >> 1))
#define b4(x)   (b2(x) | ( b2(x) >> 2))
#define b8(x)   (b4(x) | ( b4(x) >> 4))
#define b16(x)  (b8(x) | ( b8(x) >> 8))
#define b32(x)  (b16(x) | (b16(x) >>16))

#define next_power_of_2(x) (b32(x-1) + 1)

#if DEBUG_SQRT
	extern double callSqrt(const char* file, int line, const double& n);

	#define sqrt(n) callSqrt(_FL, n)
#endif

#if DEBUG_POW
	extern double callPow(const char* file, int line, const double& x, const double& y);

	#define pow(x, y) callPow(_FL, x, y)
#endif

#if DEBUG_ATAN2
	extern double callAtan2(const char* file, int line, const double& x, const double& y);

	#define atan2(x, y) callAtan2(_FL, x, y)
#endif

#if DEBUG_MATH3DNORMALIZE
    extern piVECTOR* callMath3Dnormalize(const char* file, int line, piVECTOR* v);

    #define math3dNormalize(v)  callMath3Dnormalize(_FL, v)
#endif

	inline void math3dMatrixTranslate(piMATRIX* targetMatrix, const piVECTOR& transVector)
	{
		targetMatrix->_41 += transVector.x;
		targetMatrix->_42 += transVector.y;
		targetMatrix->_43 += transVector.z;
	}

#if !defined(NDEBUG)


	/*
	 * Fast, Low Precision Reciprocal of float fx
	 */
	extern float math3dInvert(float fx);

	/*
	 * Fast, Approximate Sqrt of float fx
	 */
	extern float math3dFastAppxSqrt(float fx);

	/*
	 * Fast, Low Precision Sqrt of float fx
	 */
	extern float math3dFastSqrt(float fx);

	/*
	 * Very Fast Assembly, Low Precision Sqrt of float fx
	 */
	#if platOS == OS_WIN32
	extern float math3dFastAsmSqrt(float fx);
	#else
	#define math3dFastAsmSqrt(fx)	(float)sqrt(fx)
	#endif

	/*
	 * Fast, Low Precision Normalize of vector v
	 */
	extern piVECTOR* math3dFastNormalize(piVECTOR* lpa);

	/*
	 * Normalizes the vector v
	 */
	extern piVECTOR* math3dNormalize(piVECTOR* lpa);

	/*
	 * Calculates cross product of lpa and lpb.
	 */
	extern piVECTOR* math3dCrossProduct(piVECTOR* lpd, piVECTOR* lpa, piVECTOR* lpb);

	/*
	 * Calculates dot product of lpa and lpb.
	 */
	extern float math3dDotProduct(const piVECTOR* lpa, const piVECTOR* lpb);

	/*
	* lpd = lpa * lpb
	* lpd can be equal to lpa or lpb
	*/
	extern piVECTOR* math3dMultiplyVectors(piVECTOR* lpd, piVECTOR* lpa, piVECTOR* lpb);

	/*
	* lpd = lpa - lpb
	* lpd can be equal to lpa or lpb
	*/
	extern piVECTOR* math3dSubtractVectors(piVECTOR* lpd, const piVECTOR* lpa, const piVECTOR* lpb);

	/*
	* lpd = lpa + lpb
	* lpd can be equal to lpa or lpb
	*/
	extern piVECTOR* math3dAddVectors(piVECTOR* lpd, const piVECTOR* lpa,  const piVECTOR* lpb);

	/*
	* lpd = lpa * b
	* lpd can be equal to lpa
	*/
	extern piVECTOR* math3dScaleVector(piVECTOR* lpd, const piVECTOR* lpa, float b);

	/*
	* lpd = lpa + lpb * c
	* lpd can be equal to lpa or lpb
	*/
	extern piVECTOR* math3dAddScaledVectors(piVECTOR* lpd, piVECTOR* lpa, piVECTOR* lpb, float c);

	/*
	 * lpd = lpa * lpb
	 * lpd can be equal to lpa or lpb
	 */
	// NOTE: this version can ONLY be used if matrix elements _14,_24,_34 are 0 and _44 is 1
	// for* BOTH* matricies
	//piMATRIX* math3dMatrixMultiplyOptimized(piMATRIX* lpd, piMATRIX* lpa, piMATRIX* lpb);

	/*
	 * lpd = lpa * lpb
	 * lpd can be equal to lpa or lpb
	 */
  extern void math3dMatrixMultiply(piMATRIX* lpd, piMATRIX const* lpa, piMATRIX const* lpb);

// version which does no wholesale copy of the result, ie ldp != lpa || lpb
  extern void math3dMatrixMultiplyUnique(piMATRIX * lpd, piMATRIX const* lpa, piMATRIX const* lpb);

	/*
	 * lpd = lpa * lpb
	 * lpd can be equal to lpa or lpb
	 */
	extern piVECTOR* math3dVectorMultiply(piVECTOR* lpd, const piVECTOR* lpa, const piMATRIX* lpb);

	/*
	* lpd = lpa * lpb
	* lpd can be equal to lpa
	*/
	extern piVECTOR* math3dMultiplyRotation(piVECTOR* lpd, const piVECTOR* lpa, const piMATRIX* lpb);

	/*
	* lpd = lpa * lpb
	* lpd can be equal to lpa
	*/
	extern piVECTOR* math3dMultiplyTransformation(piVECTOR* lpd, piVECTOR* lpa, piMATRIX* lpb);

	/*
	 */
	extern piVECTOR* math3dMatrixToVector(piVECTOR* lpd, piMATRIX* lpa);

	/*
	* lpd = 1 / lpa
	*/
	extern piMATRIX* math3dInvertMatrix(piMATRIX* lpd, piMATRIX* lpa);

	/*
	 * -1 lpa = lpa
	 */
	extern piMATRIX* math3dInvert1(piMATRIX* lpa);

	/*
	 * -1 lpd = lpa
	 */
	extern piMATRIX* math3dInvert2(piMATRIX* lpd, piMATRIX* lpa);

	/*
	 * Set the rotation part of a matrix such that the vector lpa is the new
	 * z-axis and lpb is the new y-axis.
	 */
	extern piMATRIX* math3dSetRotation(piMATRIX* lpd, piVECTOR* lpa, piVECTOR* lpb);

	/*
	 * Apply one degree of rotation to matrix lpa.
	 */
	extern piMATRIX* math3dConcatRotate(piMATRIX* lpd, piMATRIX* lpa, piMATRIX* lpb);

	/*
	 * Rotate a matrix by a vector amount.
	 */
	extern piMATRIX* math3dMatrixRotate(piMATRIX* lpd, piVECTOR* lpa);

	/*
	* Three degrees of rotation via the vector in lpa.
	*/
	extern piVECTOR* math3dVectorRotate(piVECTOR* lpd, piVECTOR* lpa);

	/*
	* Calculate a normal based on a rotation vector
	*/
	extern piVECTOR* math3dCalcNormalVector(piVECTOR* lpd, piVECTOR* lpa);

	/*
	 * Using the delta angles (pitch, yaw, roll) in lpa,
	 * update the angles in lpd to make the new direction.
	 */
	extern piVECTOR* math3dOrientAngles(piVECTOR* lpd, piVECTOR* lpa);

	/*
	 * Re-Orthogonalize a matrix using Gram-Schmidt
	 */
	extern piMATRIX* math3dReorthoMatrix(piMATRIX* lpd, piMATRIX* lpa);

	// quaternion operations
	extern piQUAT* math3dMatrixToQuaternion(piQUAT* quat, piMATRIX* Mat);
	extern piMATRIX* math3dQuaternionToMatrix(piMATRIX* Mat, piQUAT* inquat);
	extern piQUAT* math3dMutliplyQuaternion(piQUAT* res,piQUAT* q1,piQUAT* q2);
	extern piQUAT* math3dQuatSlerp(piQUAT* res,piQUAT* from, piQUAT* to, FLOAT32 t);

	extern void math3dInvertMatrixComplete(piMATRIX& inverse, const piMATRIX& matrix);
#else

#include "math/math3d.cpp"

#endif
