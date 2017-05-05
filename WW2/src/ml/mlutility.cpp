#include <syscfg.h> // Path is project specific.
#include <ctype.h>

#include "pi/pi.h"
#include "include/vars.h"

#include "ml/mlutility.h"
#include "ml/mlCompon.h"
#include "math/matheul.h"
#include "math/mathvect.h"
#include "math/math3d.h"
#include "ob/obmain.h"
#include "ob/obcd.h"

UINT32 mlUtilityFindVarname(UINT32* VarNum,
							const UCHAR* VarName,
							const UCHAR* VehicleTypeVarNames,
							UINT32 VarNameSize,
							UINT32 MaxNames)
{
	const UCHAR* VehicleVarName = VehicleTypeVarNames;
	UINT32 i;

	while (*VarName == ' ' || *VarName == '=')
		VarName++;

	// Default value we may return is 0
	*VarNum = 0;

	if (*VarName == 0)
		return 0;

	VehicleVarName = VehicleTypeVarNames;

	for (i=0;i<MaxNames;i++, VehicleVarName += VarNameSize)
	{
		if (stricmp((char*)VarName, (char*)VehicleVarName) == 0)
		{
			*VarNum = i;
			return 0;
		}
	}

	///QUERY: If there's no match, just return 0? That doesn't seem smart.
	/// Since it effectively means the same as matching the first entry.

	return 0;
}

UINT32 mlUtilityGetVarnameAndContents(UCHAR ** Varname, UCHAR ** Varcontents,UCHAR * InputString)
{
	*Varname = (UCHAR*)strtok((char*)InputString,"= ");
	if (*Varname != nullptr)
	{
		*Varcontents = (UCHAR*)strtok(nullptr,"= ");
		*Varcontents = (UCHAR*)strtok((char*)*Varcontents,"\n"); // strip off the ending newline
	}
	else
	{
		*Varcontents = nullptr;
	}

	return 0;
}

UINT32 mlUtilityGetVarnameAndContents1(UCHAR ** Varname, UCHAR ** Varcontents,UCHAR * InputString)
{

	*Varname = (UCHAR*)strtok((char*)InputString,"= ");
	if (*Varname != nullptr)
	{
		*Varcontents = (UCHAR*)strtok(nullptr,"\0");
		*Varcontents = (UCHAR*)strtok((char*)*Varcontents,"\n"); // strip off the ending newline
	}
	else
	{
		*Varcontents = nullptr;
	}

	return 0;
}



// note: this busts up InputString so if you want Inputstring intact
// save it off before calling this function

UINT32 mlUtilityFindComponentByName(mlComponent** ComponentPtr,
									mlComponentList * ComponentList,
									UCHAR* ComponentName,
									mlComponent * Component)
{
	mlComponentListIterator it;

	const size_t compNameLen = strlen((const char*)ComponentName);
	mlComponentListBegin(ComponentList, &it);
	for(; it.comp; mlComponentListNext(&it))
	{
		if(strncmp((char*)ComponentName, (char*)it.comp->ComponentName, compNameLen) == 0)
		{
			*ComponentPtr = it.comp;
			return 0;
		}
	}

	*ComponentPtr = nullptr;
	WriteDebug("Unable to find component called %s stated in component %s\n",ComponentName,Component->ComponentName);

	return 0;
}

// returns an interpolation between Num1 and Num2
// detla is a number from 0.0 to 1.0
UINT32 mlInterpolate(FLOAT64* ReturnNum,FLOAT64 Num1,FLOAT64 Num2,FLOAT64 Delta)
{
FLOAT64 DeltaNum;

	DeltaNum = Num2-Num1;

	*ReturnNum = Num1 + DeltaNum * Delta;

	return 0;
}


UINT32 mlConvertModelAttitudeToWorldAttitude(piVECTOR * WorldAtt,piVECTOR * ModelAtt)
{
piMATRIX Mat1;
piVECTOR Att;

	if (WorldAtt == nullptr || ModelAtt == nullptr)
	{
		return 1;
	}

	Att.x = ModelAtt->x;
	Att.y = ModelAtt->y;
	Att.z = ModelAtt->z;
	mathEul_ToMatrix(&Mat1,EulOrdXYZs,&Att);
	mathEul_FromMatrix(&Att,&Mat1,EulOrdXYZr);
	WorldAtt->x = Att.x;
	WorldAtt->y = Att.y;
	WorldAtt->z = Att.z;

	return 0;
}

UINT32 mlForwardPredict(FLOAT64VECTOR * NewPos,
						FLOAT64ATT * NewAtt,
						FLOAT64VECTOR * CurrentPos,
						FLOAT64ATT * CurrentAtt,
						FLOAT64VECTOR * CurrentVel,
						FLOAT64ATT * CurrentAttVel,
						FLOAT64 LastUpdateTime)
{
FLOAT64 DeltaTime;
FLOAT64VECTOR TempPos;
FLOAT64ATT TempAtt;
FLOAT64ATT DeltaAtt;
piVECTOR Att1,AttVel1;
piMATRIX Mat1,Mat2,Mat3;

	if (LastUpdateTime == 0.0)
	{
		DeltaTime = 0.0;
	}
	else
	{
		DeltaTime = (frametime - LastUpdateTime) / 1000.0;
	}


	TempPos = *CurrentPos;
	TempAtt = *CurrentAtt;

	TempPos.x += CurrentVel->x * DeltaTime;
	TempPos.y += CurrentVel->y * DeltaTime;
	TempPos.z += CurrentVel->z * DeltaTime;

	DeltaAtt.Roll = CurrentAttVel->Roll * DeltaTime;
	DeltaAtt.Pitch = CurrentAttVel->Pitch * DeltaTime;
	DeltaAtt.Yaw = CurrentAttVel->Yaw * DeltaTime;

	Att1.x = (FLOAT32)TempAtt.Pitch;
	Att1.y = (FLOAT32)TempAtt.Roll;
	Att1.z = (FLOAT32)TempAtt.Yaw;

	AttVel1.x = -(FLOAT32)DeltaAtt.Pitch;
	AttVel1.y = -(FLOAT32)DeltaAtt.Roll;
	AttVel1.z = (FLOAT32)DeltaAtt.Yaw;

	mathEul_ToMatrix(&Mat1,EulOrdXYZs,&Att1);
	mathEul_ToMatrix(&Mat2,EulOrdXYZs,&AttVel1);
	math3dMatrixMultiply(&Mat3,&Mat2,&Mat1);
	mathEul_FromMatrix(&Att1,&Mat3,EulOrdXYZs);

	TempAtt.Pitch = Att1.x;
	TempAtt.Roll = Att1.y;
	TempAtt.Yaw = Att1.z;

	*NewPos = TempPos;
	*NewAtt = TempAtt;

	return 0;
}

UINT32 mlUtilityGetModelAttitudeFromObjectMatrix(FLOAT64ATT * Att,piMATRIX * Mat)
{
piVECTOR Att1;

	mathEul_FromMatrix(&Att1,Mat,EulOrdXYZs);

	Att->Pitch = -Att1.x;
	Att->Roll = -Att1.y;
	Att->Yaw = Att1.z;

	return 0;
}

UINT32 mlUtilityGetModelPositionFromObjectMatrix(FLOAT64VECTOR * Pos,piMATRIX * Mat)
{
	Pos->x = Mat->_41 / ScaleFactor;
	Pos->y = Mat->_42 / ScaleFactor;
	Pos->z = -Mat->_43 / ScaleFactor;

	return 0;
}

// the object/modelling system inverts the "z" axis, thus if we want to use
// an object matrix/DOF matrix in the modelling system we must negate the z line
UINT32 mlUtilityConvertMatrixToModelReference(piMATRIX * DestMat,piMATRIX * SourceMat)
{

	*DestMat = *SourceMat;

	DestMat->_13 = -DestMat->_13;
	DestMat->_23 = -DestMat->_23;
	DestMat->_33 = -DestMat->_33;
	DestMat->_43 = -DestMat->_43;

	return 0;
}

UINT32 mlUtilityConvertAttToMatrix(piMATRIX * Mat,FLOAT64ATT * Att)
{
piVECTOR TempPiVec;

	TempPiVec.x = (FLOAT32)Att->Pitch;
	TempPiVec.y = (FLOAT32)Att->Roll;
	TempPiVec.z = (FLOAT32)Att->Yaw;
	mathEul_ToMatrix(Mat,EulOrdXYZs,&TempPiVec);

	return 0;
}

// sorts from smallest to largest
UINT32 mlUtilitySortCollideList(FASTCOLLIDE * CollideList,UINT32 NumElements)
{
FLOAT32 CurrentTime;
SINT32 Index;
UINT32 i,j;
FASTCOLLIDE TempCollide;

	// inefficient as hell collider could be optimized
	for (i=0;i<NumElements;i++)
	{
		CurrentTime = 100000000.0;
		Index = -1;
		for (j=i;j<NumElements;j++)
		{
			if (CollideList[j].CollideTime < CurrentTime)
			{
				CurrentTime = CollideList[j].CollideTime;
				Index = j;
			}
		}
		if (Index >= 0)
		{
			// swap i and index
			TempCollide = CollideList[Index];
			CollideList[Index] = CollideList[i];
			CollideList[i] = TempCollide;
		}

	}

	return 0;
}

SINT32 mlUtilityGetPosAndAttFromDOF(OBJECT* dofObj, piVECTOR const* relpos, piVECTOR* pos, piVECTOR* att)
{
	piMATRIX const* dofmatrix2 = nullptr;
	piMATRIX AttMat;

	if(dofObj == nullptr || dofObj->dof == nullptr) return -1;
	dofmatrix2 = &dofObj->dof->dofmatrix2;

	if(pos)
	{
		pos->x = dofmatrix2->_41; pos->y = dofmatrix2->_42; pos->z = dofmatrix2->_43;
		if(relpos)
			mathAddVectors(pos, pos, relpos);

		math3dVectorMultiply(pos, pos, &dofObj->mat);
	}

	if(att)
	{
		AttMat = *dofmatrix2;
		AttMat._41 = AttMat._42 = AttMat._43 = 0.0f;
		math3dMatrixMultiply(&AttMat, &AttMat, &dofObj->mat);
		mathEul_FromMatrix(att, &AttMat, EulOrdXYZs);
	}

	return 0;
}

UINT32 mlUtilityCheckLineSphereCollide(UINT32 * Collision, piVECTOR const* SphereCenter,
									piVECTOR const* UseOldPos, piVECTOR const* UseNewPos, FLOAT32 Radius)
{
	FLOAT32 a,b,c;
	FLOAT32 c1,c2,c3;
	FLOAT32 A,B,C;
	FLOAT64 Det;
	FLOAT64 T1,T2;

	*Collision = 0;

	c1 = UseOldPos->x - SphereCenter->x;
	c2 = UseOldPos->y - SphereCenter->y;
	c3 = UseOldPos->z - SphereCenter->z;
	a = UseNewPos->x - UseOldPos->x;
	b = UseNewPos->y - UseOldPos->y;
	c = UseNewPos->z - UseOldPos->z;

	A = a * a + b * b + c * c;
	B = 2 * (a * c1 + b * c2 + c * c3);
	C = c1 * c1 + c2 * c2 + c3 * c3 - Radius * Radius;

	// determinate check
	Det = B * B - 4 * A * C;
	if (Det < 0)
	{
		return 0;
	}

	Det = sqrt(Det);

	T1 = (-B + Det) / (2 * A);

	// check T1
	if (T1 >= 0.0 && T1 <= 1.0)
	{
		*Collision = 1;
		return 0;
	}

	// check T2
	T2 = (-B - Det) / (2 * A);
	if (T2 >= 0.0 && T2 <= 1.0)
	{
		*Collision = 1;
		return 0;
	}

	// check if T1 and T2 surround time zero to 1
	if ((T1 < 0.0 && T2 > 1.0) || (T1 > 1.0 && T2 < 0.0))
	{
		*Collision = 1;
		return 0;
	}

	return 0;

}
