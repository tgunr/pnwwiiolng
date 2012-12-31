// math3d.h
//

#ifndef math3d_HEADER

	#define math3d_HEADER

	#include <math.h>
	#include "pi.h"

// Find out who is calling sqrt with incorrect params
#define DEBUG_SQRT  0
#define DEBUG_POW   0
#define DEBUG_ATAN2 0
#define DEBUG_MATH3DNORMALIZE   0

#if DEBUG_SQRT
	extern double callSqrt(const char *file, int line, const double& n);

	#define sqrt(n) callSqrt(__FILE__, __LINE__, n)
#endif

#if DEBUG_POW
	extern double callPow(const char *file, int line, const double& x, const double& y);

	#define pow(x, y) callPow(__FILE__, __LINE__, x, y)
#endif

#if DEBUG_ATAN2
	extern double callAtan2(const char *file, int line, const double& x, const double& y);

	#define atan2(x, y) callAtan2(__FILE__, __LINE__, x, y)
#endif

#if DEBUG_MATH3DNORMALIZE
    extern piVECTOR* callMath3Dnormalize(const char *file, int line, piVECTOR* v);

    #define math3dNormalize(v)  callMath3Dnormalize(__FILE__, __LINE__, v)
#endif

#if !defined(NDEBUG)


	/*
	 * Fast, Low Precision Reciprocal of float fx
	 */
	float math3dInvert(float fx);

	/*
	 * Fast, Approximate Sqrt of float fx
	 */
	float math3dFastAppxSqrt(float fx);

	/*
	 * Fast, Low Precision Sqrt of float fx
	 */
	float math3dFastSqrt(float fx);

	/*
	 * Very Fast Assembly, Low Precision Sqrt of float fx
	 */
	#if platOS == OS_WIN32
	float math3dFastAsmSqrt(float fx);
	#else
	#define math3dFastAsmSqrt(fx)	(float)sqrt(fx)
	#endif

	/*
	 * Fast, Low Precision Normalize of vector v
	 */
	piLPVECTOR math3dFastNormalize(piLPVECTOR lpa);

	/*

	/*
	 * Normalizes the vector v
	 */
	piLPVECTOR math3dNormalize(piLPVECTOR lpa);

	/*
	 * Calculates cross product of lpa and lpb.
	 */
	piLPVECTOR math3dCrossProduct(piLPVECTOR lpd, piLPVECTOR lpa, piLPVECTOR lpb);

	/*
	 * Calculates dot product of lpa and lpb.
	 */
	float math3dDotProduct(const piVECTOR* lpa, const piVECTOR* lpb);

	/*
	* lpd = lpa * lpb
	* lpd can be equal to lpa or lpb
	*/
	piLPVECTOR math3dMultiplyVectors(piLPVECTOR lpd, piLPVECTOR lpa, piLPVECTOR lpb);

	/*
	* lpd = lpa - lpb
	* lpd can be equal to lpa or lpb
	*/
	piLPVECTOR math3dSubtractVectors(piLPVECTOR lpd, const piVECTOR* lpa, const piVECTOR* lpb);

	/*
	* lpd = lpa + lpb
	* lpd can be equal to lpa or lpb
	*/
	piLPVECTOR math3dAddVectors(piLPVECTOR lpd, const piVECTOR* lpa,  const piVECTOR* lpb);

	/*
	* lpd = lpa * b
	* lpd can be equal to lpa
	*/
	piLPVECTOR math3dScaleVector(piVECTOR* lpd, const piVECTOR* lpa, float b);

	/*
	* lpd = lpa + lpb * c
	* lpd can be equal to lpa or lpb
	*/
	piLPVECTOR math3dAddScaledVectors(piLPVECTOR lpd, piLPVECTOR lpa, piLPVECTOR lpb, float c);

	/*
	 * lpd = lpa * lpb
	 * lpd can be equal to lpa or lpb
	 */
	// NOTE: this version can ONLY be used if matrix elements _14,_24,_34 are 0 and _44 is 1 
	// for *BOTH* matricies
	//piLPMATRIX math3dMatrixMultiplyOptimized(piLPMATRIX lpd, piLPMATRIX lpa, piLPMATRIX lpb);

	/*
	 * lpd = lpa * lpb
	 * lpd can be equal to lpa or lpb
	 */
	piLPMATRIX math3dMatrixMultiply(piLPMATRIX lpd, piMATRIX const* lpa, piMATRIX const* lpb);

	/*
	 * lpd = lpa * lpb
	 * lpd can be equal to lpa or lpb
	 */
	piLPVECTOR math3dVectorMultiply(piVECTOR* lpd, const piVECTOR* lpa, const piMATRIX* lpb);

	/*
	* lpd = lpa * lpb
	* lpd can be equal to lpa
	*/
	piLPVECTOR math3dMultiplyRotation(piVECTOR* lpd, const piVECTOR* lpa, piLPMATRIX lpb);

	/*
	* lpd = lpa * lpb
	* lpd can be equal to lpa
	*/
	piLPVECTOR math3dMultiplyTransformation(piLPVECTOR lpd, piLPVECTOR lpa, piLPMATRIX lpb);

	/*
	 */
	piLPVECTOR math3dMatrixToVector(piLPVECTOR lpd, piLPMATRIX lpa);

	/*
	* lpd = 1 / lpa
	*/
	piLPMATRIX math3dInvertMatrix(piLPMATRIX lpd, piLPMATRIX lpa);

	/*
	 * -1 lpa = lpa
	 */
	piLPMATRIX math3dInvert1(piLPMATRIX lpa);

	/*
	 * -1 lpd = lpa
	 */
	piLPMATRIX math3dInvert2(piLPMATRIX lpd, piLPMATRIX lpa);

	/*
	 * Set the rotation part of a matrix such that the vector lpa is the new
	 * z-axis and lpb is the new y-axis.
	 */
	piLPMATRIX math3dSetRotation(piLPMATRIX lpd, piLPVECTOR lpa, piLPVECTOR lpb);

	/*
	 * Apply one degree of rotation to matrix lpa.
	 */
	piLPMATRIX math3dConcatRotate(piLPMATRIX lpd, piLPMATRIX lpa, piLPMATRIX lpb);

	/*
	 * Rotate a matrix by a vector amount.
	 */
	piLPMATRIX math3dMatrixRotate(piLPMATRIX lpd, piLPVECTOR lpa);

	/*
	* Three degrees of rotation via the vector in lpa.
	*/
	piLPVECTOR math3dVectorRotate(piLPVECTOR lpd, piLPVECTOR lpa);

	/*
	* Calculate a normal based on a rotation vector
	*/
	piLPVECTOR math3dCalcNormalVector(piLPVECTOR lpd, piLPVECTOR lpa);

	/*
	 * Using the delta angles (pitch, yaw, roll) in lpa,
	 * update the angles in lpd to make the new direction.
	 */
	piLPVECTOR math3dOrientAngles(piLPVECTOR lpd, piLPVECTOR lpa);

	/*
	 * Re-Orthogonalize a matrix using Gram-Schmidt
	 */
	piLPMATRIX math3dReorthoMatrix(piLPMATRIX lpd, piLPMATRIX lpa);

	// quaternion operations
	piLPQUAT math3dMatrixToQuaternion(piLPQUAT quat, piLPMATRIX Mat);
	piLPMATRIX math3dQuaternionToMatrix(piLPMATRIX Mat, piLPQUAT inquat);
	piLPQUAT math3dMutliplyQuaternion(piLPQUAT res,piLPQUAT q1,piLPQUAT q2);
	piLPQUAT math3dQuatSlerp(piLPQUAT res,piLPQUAT from, piLPQUAT to, FLOAT32 t);

	// fast look-up table versions of cos/sin
	FLOAT32 mathSinViaTable(FLOAT32 InVal);
	FLOAT32 mathCosViaTable(FLOAT32 InVal);

	void mathInitCosSinLookupTables(void);
	void math3dInvertMatrixComplete(piMATRIX& inverse, const piMATRIX& matrix);
#else

#include "math3d.cpp"

#endif

#endif
