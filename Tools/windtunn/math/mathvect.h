#ifndef MATHVECT_H
#define MATHVECT_H

#include "systypes.h"
#include "pi.h"

// mathAddVectors is basically
// target = source1 + source2
// target and source1 can be the same
#define mathAddVectors(target,source1,source2) \
{ \
			(target)->x = (source1)->x + (source2)->x; \
			(target)->y = (source1)->y + (source2)->y; \
			(target)->z = (source1)->z + (source2)->z; \
}

// mathSubVectors is basically
// target = source1 - source2
// target and source1 can be the same
#define mathSubVectors(target,source1,source2) \
{ \
			(target)->x = (source1)->x - (source2)->x; \
			(target)->y = (source1)->y - (source2)->y; \
			(target)->z = (source1)->z - (source2)->z; \
}

// NOTE:  do NOT allow C to be either A or B or this will not WORK!
#define mathVectDoCrossProduct(C,A,B) \
{ \
			(C)->x = (A)->y*(B)->z - (A)->z*(B)->y; \
			(C)->y = (A)->z*(B)->x - (A)->x*(B)->z; \
			(C)->z = (A)->x*(B)->y - (A)->y*(B)->x; \
}

#define mathVectDotProduct(A,B) \
			((A)->x * (B)->x + (A)->y * (B)->y + (A)->z * (B)->z)

#define mathExactVectorLength(A) \
			(sqrt((A)->x*(A)->x + (A)->y*(A)->y + (A)->z*(A)->z))
  
#define mathExactVectorLength2(A) \
			((A)->x*(A)->x + (A)->y*(A)->y + (A)->z*(A)->z)

#define math64To32Vec(a,b) \
{ \
	(a)->x = (FLOAT32)(b)->x; \
	(a)->y = (FLOAT32)(b)->y; \
	(a)->z = (FLOAT32)(b)->z; \
}

#define math32To64Vec(a,b) \
{ \
	(a)->x = (b)->x; \
	(a)->y = (b)->y; \
	(a)->z = (b)->z; \
}

#define mathScaleVector(a,b,c) \
{ \
	(a)->x = (b)->x * c; \
	(a)->y = (b)->y * c; \
	(a)->z = (b)->z * c; \
}
extern UINT32 mathRotateAttByAtt(FLOAT64ATT *DestAtt,
								 FLOAT64ATT *SourceAtt,
								 FLOAT64ATT *RotateAtt);

extern UINT32 mathCRotateAttByAtt(FLOAT64ATT *DestAtt,
						 FLOAT64ATT *SourceAtt,
						 FLOAT64ATT *RotateAtt);

extern UINT32 mathRotateVectorByAtt(FLOAT64VECTOR *DestVec,
									FLOAT64VECTOR *SourceVec,
									FLOAT64ATT *RotateAtt);

extern UINT32 mathCRotateVectorByAtt(FLOAT64VECTOR *DestVec,
									 FLOAT64VECTOR *SourceVec,
									 FLOAT64ATT *RotateAtt);

extern UINT32 mathGetExactAttFromVec(FLOAT64ATT *DestAtt,
									 FLOAT64VECTOR *SourceVec);
extern UINT32 mathGetExactAttFromVec1(FLOAT64ATT *DestAtt,
									 FLOAT64VECTOR *SourceVec);

extern	FLOAT64VECTOR*	mathNormalize( FLOAT64VECTOR *vec );

extern UINT32 mathVectorMultiply64(FLOAT64VECTOR * lpd, FLOAT64VECTOR const* inlpa, piMATRIX const* lpb);
extern UINT32 mathVectorRotate64(FLOAT64VECTOR * lpd, FLOAT64VECTOR const* inlpa, piMATRIX const* lpb);

#endif
