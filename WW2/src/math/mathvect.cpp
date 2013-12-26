#include <syscfg.h> // Path is project specific.

#include <math.h>

#include "math/mathvect.h"
#include "pi/pi.h"
#include "include/vars.h"
#include "math/matheul.h"
#include "math/math3d.h"
#include "math/Vec3d.h"

// for the modeling routines, pitch is the x axis, roll is the z axis and yaw is the y axis
// zero for all three is straight down with the "top" of the object pointing north

// rotation order is x,y,z axes in that order, or pitch,roll,yaw
// attitudes follow right hand rule

// rotate an attitude.
// the rotation is relative to the original attitude, so that
// if you were rolled 180 degrees, a positive pitch change
// would result in a "real" pitch change of a negative
void mathRotateAttByAtt(FLOAT64ATT* DestAtt,
						 const FLOAT64ATT* SourceAtt,
						 const FLOAT64ATT* RotateAtt)
{
	piMATRIX Mat,Mat1,Mat2;
	piVECTOR Att;

	Att.x = -(float)SourceAtt->Pitch;
	Att.y = -(float)SourceAtt->Roll;
	Att.z = -(float)SourceAtt->Yaw;

	mathEul_ToMatrix(&Mat1,EulOrdXYZr,&Att);

	Att.x = -(float)RotateAtt->Pitch;
	Att.y = -(float)RotateAtt->Roll;
	Att.z = -(float)RotateAtt->Yaw;

	mathEul_ToMatrix(&Mat2,EulOrdXYZr,&Att);

//	math3dMatrixRotate(&Mat1,&Att);
	math3dMatrixMultiply(&Mat,&Mat1,&Mat2);

	mathEul_FromMatrix(&Att,&Mat,EulOrdXYZr);

	DestAtt->Pitch = -Att.x;
	DestAtt->Roll = -Att.y;
	DestAtt->Yaw = -Att.z;
}


// haven't verified this function -hoof
void mathCRotateAttByAtt(FLOAT64ATT* DestAtt,
						 const FLOAT64ATT* SourceAtt,
						 const FLOAT64ATT* RotateAtt)
{
	piMATRIX Mat,Mat1,Mat2;
	piVECTOR Att;

	Att.x = -(float)SourceAtt->Pitch;
	Att.y = -(float)SourceAtt->Roll;
	Att.z = -(float)SourceAtt->Yaw;

	mathEul_ToMatrix(&Mat1,EulOrdXYZr,&Att);

	Att.x = (float)RotateAtt->Pitch;
	Att.y = (float)RotateAtt->Roll;
	Att.z = (float)RotateAtt->Yaw;

	mathEul_ToMatrix(&Mat2,EulOrdXYZs,&Att);

//	math3dMatrixRotate(&Mat1,&Att);
	math3dMatrixMultiply(&Mat,&Mat1,&Mat2);

	mathEul_FromMatrix(&Att,&Mat,EulOrdXYZr);

	DestAtt->Pitch = -Att.x;
	DestAtt->Roll = -Att.y;
	DestAtt->Yaw = -Att.z;
}

void mathRotateVectorByAtt(FLOAT64VECTOR* DestVec,
						   const FLOAT64VECTOR* SourceVec,
						   const FLOAT64ATT* RotateAtt)
{
	FLOAT64VECTOR TempVec;
	FLOAT64 CosValue,SinValue;
	FLOAT64 TempNum;

	TempVec = *SourceVec;

	// rotate vector along pitch axis
	CosValue = cos(-(RotateAtt->Pitch));
	SinValue = sin(-(RotateAtt->Pitch));

	TempNum = TempVec.z;
	TempVec.z = CosValue * TempVec.z + SinValue * TempVec.y;
	TempVec.y = -SinValue * TempNum + CosValue * TempVec.y;

	// rotate vector along roll axis
	CosValue = cos(-(RotateAtt->Roll));
	SinValue = sin(-(RotateAtt->Roll));

	TempNum = TempVec.z;
	TempVec.z = CosValue * TempVec.z - SinValue * TempVec.x;
	TempVec.x = SinValue * TempNum + CosValue * TempVec.x;

	// rotate vector along yaw axis (which is along the Z axis, counterclockwise)
	CosValue = cos(-(RotateAtt->Yaw));
	SinValue = sin(-(RotateAtt->Yaw));

	TempNum = TempVec.y;
	TempVec.y = CosValue * TempVec.y + SinValue * TempVec.x;
	TempVec.x = -SinValue * TempNum + CosValue * TempVec.x;

	*DestVec = TempVec;
}

void mathCRotateVectorByAtt(FLOAT64VECTOR* DestVec,
							const FLOAT64VECTOR* SourceVec,
							const FLOAT64ATT* RotateAtt)
{
	FLOAT64VECTOR TempVec;
	FLOAT64 CosValue,SinValue;
	FLOAT64 TempNum;

	TempVec = *SourceVec;

	// rotate vector along yaw axis (which is along the Z axis, counterclockwise)
	CosValue = cos(RotateAtt->Yaw);
	SinValue = sin(RotateAtt->Yaw);

	TempNum = TempVec.y;
	TempVec.y = CosValue * TempVec.y + SinValue * TempVec.x;
	TempVec.x = -SinValue * TempNum + CosValue * TempVec.x;

	// rotate vector along roll axis
	CosValue = cos(RotateAtt->Roll);
	SinValue = sin(RotateAtt->Roll);

	TempNum = TempVec.z;
	TempVec.z = CosValue * TempVec.z - SinValue * TempVec.x;
	TempVec.x = SinValue * TempNum + CosValue * TempVec.x;

	// rotate vector along pitch axis
	CosValue = cos(RotateAtt->Pitch);
	SinValue = sin(RotateAtt->Pitch);

	TempNum = TempVec.z;
	TempVec.z = CosValue * TempVec.z + SinValue * TempVec.y;
	TempVec.y = -SinValue * TempNum + CosValue * TempVec.y;

	*DestVec = TempVec;
}

void mathGetExactAttFromVec(FLOAT64ATT* DestAtt,
							const FLOAT64VECTOR* SourceVec)
{
	FLOAT64VECTOR TempVec = *SourceVec;

	// roll is always zero when getting attitudes from vectors
	DestAtt->Roll = DestAtt->Yaw = 0;

	// get yaw (a yaw of zero is along the y axis)
	DestAtt->Pitch = atan2(TempVec.y, TempVec.z);

	mathCRotateVectorByAtt(&TempVec, &TempVec, DestAtt);

	// get pitch
	DestAtt->Yaw = atan2(TempVec.x, TempVec.z);
}

void mathGetExactAttFromVec1(FLOAT64ATT* DestAtt,
							 const FLOAT64VECTOR* SourceVec)
{
	FLOAT64VECTOR TempVec;
	TempVec = *SourceVec;

	// roll is always zero when getting attitudes from vectors
	DestAtt->Roll = DestAtt->Pitch = 0;

	// get yaw (a yaw of zero is along the y axis)
	DestAtt->Yaw = atan2(TempVec.x, TempVec.y);

	mathCRotateVectorByAtt(&TempVec, &TempVec, DestAtt);

	// get pitch
	DestAtt->Pitch = atan2(TempVec.y, TempVec.z);
}

FLOAT64VECTOR* mathNormalize( FLOAT64VECTOR* vec )
{
	const FLOAT64 magnitude = sqrt(vec->x * vec->x + vec->y * vec->y + vec->z * vec->z);
	if (magnitude)
	{
		FLOAT64 inv_mod = (1.0 / magnitude);
		vec->x *= inv_mod;
		vec->y *= inv_mod;
		vec->z *= inv_mod;
	}
	else
	{
		vec->x = vec->y = vec->z = 0.0;
	}

	return vec;
}

void mathVectorMultiply64(FLOAT64VECTOR * lpd, FLOAT64VECTOR const* inlpa, piMATRIX const* lpb)
{
	FLOAT64VECTOR lpa = *inlpa;

	lpd->x = lpa.x * lpb->_11 + lpa.y * lpb->_21 + lpa.z * lpb->_31 + lpb->_41;
	lpd->y = lpa.x * lpb->_12 + lpa.y * lpb->_22 + lpa.z * lpb->_32 + lpb->_42;
	lpd->z = lpa.x * lpb->_13 + lpa.y * lpb->_23 + lpa.z * lpb->_33 + lpb->_43;
}

void mathVectorRotate64(FLOAT64VECTOR * lpd, FLOAT64VECTOR const* inlpa, piMATRIX const* lpb)
{
	FLOAT64VECTOR lpa = *inlpa;

	lpd->x = lpa.x * lpb->_11 + lpa.y * lpb->_21 + lpa.z * lpb->_31;
	lpd->y = lpa.x * lpb->_12 + lpa.y * lpb->_22 + lpa.z * lpb->_32;
	lpd->z = lpa.x * lpb->_13 + lpa.y * lpb->_23 + lpa.z * lpb->_33;
}
