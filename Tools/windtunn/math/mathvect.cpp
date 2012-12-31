#include "syscfg.h"
#include "systypes.h"

#include <math.h>

#include "mathvect.h"
#include "pi.h"
#include "vars.h"
#include "matheul.h"
#include "math3d.h"

// for the modelling routines, pitch is the x axis, roll is the z axis and yaw is the y axis
// zero for all three is straight down with the "top" of the object pointing north

// rotation order is x,y,z axes in that order, or pitch,roll,yaw
// attitudes follor right hand rule

// rotate an attitude.
// the rotation is relative to the original attitude, so that
// if you were rolled 180 degrees, a positive pitch change
// would result in a "real" pitch change of a negative
UINT32 mathRotateAttByAtt(FLOAT64ATT *DestAtt,
						 FLOAT64ATT *SourceAtt,
						 FLOAT64ATT *RotateAtt)
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

	return 0;

}


// haven't verified this function -hoof
UINT32 mathCRotateAttByAtt(FLOAT64ATT *DestAtt,
						 FLOAT64ATT *SourceAtt,
						 FLOAT64ATT *RotateAtt)
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

	return 0;

}



UINT32 mathRotateVectorByAtt(FLOAT64VECTOR *DestVec,
							FLOAT64VECTOR *SourceVec,
							FLOAT64ATT *RotateAtt)
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

	return 0;
}

UINT32 mathCRotateVectorByAtt(FLOAT64VECTOR *DestVec,
							 FLOAT64VECTOR *SourceVec,
							 FLOAT64ATT *RotateAtt)
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
	return 0;
}

UINT32 mathGetExactAttFromVec(FLOAT64ATT *DestAtt,
							 FLOAT64VECTOR *SourceVec)
{
FLOAT64VECTOR TempVec;
FLOAT64ATT TempAtt;

	TempVec = *SourceVec;

	// roll is always zero when getting attitudes from vectors
	TempAtt.Roll = TempAtt.Yaw = 0;

	// get yaw (a yaw of zero is along the y axis)
	TempAtt.Pitch = atan2(TempVec.y,TempVec.z);

	mathCRotateVectorByAtt(&TempVec,&TempVec,&TempAtt);

	// get pitch
	TempAtt.Yaw = atan2(TempVec.x,TempVec.z);

	*DestAtt = TempAtt;

	return 0;
}

UINT32 mathGetExactAttFromVec1(FLOAT64ATT *DestAtt,
							 FLOAT64VECTOR *SourceVec)
{
FLOAT64VECTOR TempVec;
FLOAT64ATT TempAtt;

	TempVec = *SourceVec;

	// roll is always zero when getting attitudes from vectors
	TempAtt.Roll = TempAtt.Pitch = 0;

	// get yaw (a yaw of zero is along the y axis)
	TempAtt.Yaw = atan2(TempVec.x,TempVec.y);

	mathCRotateVectorByAtt(&TempVec,&TempVec,&TempAtt);

	// get pitch
	TempAtt.Pitch = atan2(TempVec.y,TempVec.z);

	*DestAtt = TempAtt;

	return 0;
}

FLOAT64VECTOR* mathNormalize( FLOAT64VECTOR* vec )
{
	FLOAT64 inv_mod;

	inv_mod = vec->x * vec->x + vec->y * vec->y + vec->z * vec->z;
	if (inv_mod)
	{
		inv_mod = (float)( 1.0 / sqrt( inv_mod ) );

		vec->x *= inv_mod;
		vec->y *= inv_mod;
		vec->z *= inv_mod;
	}

	return vec;
}

UINT32 mathVectorMultiply64(FLOAT64VECTOR * lpd, FLOAT64VECTOR const* inlpa, piMATRIX const* lpb)
{
FLOAT64VECTOR lpa;

	lpa = *inlpa;

	lpd->x = lpa.x * lpb->_11 + lpa.y * lpb->_21 + lpa.z * lpb->_31 + lpb->_41;
	lpd->y = lpa.x * lpb->_12 + lpa.y * lpb->_22 + lpa.z * lpb->_32 + lpb->_42;
	lpd->z = lpa.x * lpb->_13 + lpa.y * lpb->_23 + lpa.z * lpb->_33 + lpb->_43;

  return 0;
}

UINT32 mathVectorRotate64(FLOAT64VECTOR * lpd, FLOAT64VECTOR const* inlpa, piMATRIX const* lpb)
{
FLOAT64VECTOR lpa;

	lpa = *inlpa;

	lpd->x = lpa.x * lpb->_11 + lpa.y * lpb->_21 + lpa.z * lpb->_31;
	lpd->y = lpa.x * lpb->_12 + lpa.y * lpb->_22 + lpa.z * lpb->_32;
	lpd->z = lpa.x * lpb->_13 + lpa.y * lpb->_23 + lpa.z * lpb->_33;

  return 0;
}
