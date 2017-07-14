#include "syscfg.h" // Path is project specific.
#include "pi/pi.h"
#include "include/vars.h"

#include <math.h>

#include "mem/mem.h"

#include "veh/vehmain.h"
#include "ob/obmain.h"

#include "pers/persbombsight.h"
#include "pers/persmain.h"
#include "ml/mlCompon.h"
#include "ml/mltypes.h"
#include "ml/mlgauges.h"
#include "ml/mlweapons.h"
#include "ml/mlAtmo.h"
#include "ml/mlutility.h"

#include "math/mathvect.h"
#include "math/math3d.h"
#include "math/matheul.h"

#include "weap/weapammo.h"

#include "ogrp/ogrptype.h"

/************************************************************************************************
	persBombsight
*************************************************************************************************/

#define _MK9A_MIN_ALT 914.35f	// 3000ft AGL
#define _MK9A_MAX_ALT 6095.7f	// 20000ft AGL
#define _MK9A_MIN_SPEED 22.35f	// 50mph TAS
#define _MK9A_MAX_SPEED 134.11f	// 300mph TAS

#define _MK9A_ALT_CHANGE_RATE 400.0f // 1000 meters change per sec
#define _MK9A_SPEED_CHANGE_RATE 10.0f // 20 m/s change per sec

#define _LOTFEC7B_MIN_ALT 900.0f
#define _LOTFEC7B_MAX_ALT 6000.0f
#define _LOTFEC7B_MIN_SPEED 22.35f
#define _LOTFEC7B_MAX_SPEED 134.11f

#define _LOTFEC7B_ALT_CHANGE_RATE 400.0f // 1000 meters change per sec
#define _LOTFEC7B_SPEED_CHANGE_RATE 10.0f // 20 m/s change per sec

#define TIME_INCREMENT 0.1

struct BombSightData
{
	OBJECT* PivotDOF;
	OBJECT* AltPntDOF;
	OBJECT* SpeedPntDOF;
	OBJECT* ForeSightDOF;
	OBJECT* BackSightDOF;

	FLOAT32 CurrentAlt;
	FLOAT32 CurrentSpeed;

	weapAmmoType* AmmoType;

	FLOAT32 minAlt, maxAlt;
	FLOAT32 minSpeed, maxSpeed;
	FLOAT32 altRate, speedRate;

	LOGICAL useEnglish;
};

static LOGICAL _SetupBombsight(SINT8 * MountName, mlVehicleData * VehicleData, SINT32 type);
static FLOAT32 _CalcBombAngle(FLOAT32 Alt,FLOAT32 InSpeed,weapAmmoType * AmmoType);
static LOGICAL _UpdateBombsight();
static LOGICAL _PlaneAttitudeAdjustFactor(FLOAT32 * Pitch,FLOAT32 * Roll);

static SINT32 _BombsightType;
static BombSightData* _Data;

static SINT32 _AdjustSpeedDir;
static SINT32 _AdjustAltDir;

/************************************************************************************************
	persBombsightInit
*************************************************************************************************/

LOGICAL persBombsightInit()
{
	return TRUE;
}

/************************************************************************************************
	persBombsightUpdate
*************************************************************************************************/

LOGICAL persBombsightUpdate()
{
	switch(_BombsightType)
	{
		case BOMBSIGHT_TYPE_BRITISH_MK9A:
		case BOMBSIGHT_TYPE_GERMAN_LOTFEC7B:
			_UpdateBombsight();
		break;
	}

	return TRUE;
}

/************************************************************************************************
	persBombsightKill
*************************************************************************************************/

void persBombsightKill()
{
	persClearBombsight();
}

/************************************************************************************************
	persClearBombsight
*************************************************************************************************/

LOGICAL persClearBombsight()
{
	if (_Data != nullptr)
	{
		memFree(_Data);
		_Data = nullptr;
	}

	_BombsightType = 0;

	return TRUE;
}

/************************************************************************************************
	persBombsightSetup
*************************************************************************************************/

LOGICAL persBombsightSetup(SINT32 Type,SINT8* MountName, void*  InVehicleData)
{
mlVehicleData * VehicleData;

	if (MountName == nullptr || InVehicleData == nullptr || Type == 0)
	{
		return FALSE;
	}

	VehicleData = (mlVehicleData*)InVehicleData;

	if (VehicleData->ComponentList == nullptr)
	{
		return FALSE;
	}

	switch (Type)
	{
		case BOMBSIGHT_TYPE_BRITISH_MK9A:
		case BOMBSIGHT_TYPE_GERMAN_LOTFEC7B:
			_SetupBombsight(MountName,VehicleData, Type);
		break;
	}

	_BombsightType = Type;

	return TRUE;
}

/************************************************************************************************
	persBombsightSpeedInc
*************************************************************************************************/

UINT32 persBombsightSpeedInc(SINT32 Param,UINT32 Id,UINT32 Detach)
{
	_AdjustSpeedDir = (Param) ? 1 : 0;
	return 0;
}

/************************************************************************************************
	persBombsightSpeedDec
*************************************************************************************************/

UINT32 persBombsightSpeedDec(SINT32 Param,UINT32 Id,UINT32 Detach)
{
	_AdjustSpeedDir = (Param) ? -1 : 0;
	return 0;
}

/************************************************************************************************
	persBombsightAltInc
*************************************************************************************************/

UINT32 persBombsightAltInc(SINT32 Param,UINT32 Id,UINT32 Detach)
{
	_AdjustAltDir = (Param == 1) ? 1 : 0;
	return 0;
}

/************************************************************************************************
	persBombsightAltDec
*************************************************************************************************/

UINT32 persBombsightAltDec(SINT32 Param,UINT32 Id,UINT32 Detach)
{
	_AdjustAltDir =  (Param == 1) ? -1 : 0;
	return 0;
}

/************************************************************************************************
	persBombsightSpeed
*************************************************************************************************/

FLOAT32 persBombsightSpeed()
{
	return (_Data) ? _Data->CurrentSpeed : 0;
}

/************************************************************************************************
	persBombsightAltitude
*************************************************************************************************/

FLOAT32 persBombsightAltitude()
{
	return (_Data) ? _Data->CurrentAlt : 0;
}

/************************************************************************************************
	_SetupBombsight
*************************************************************************************************/

static LOGICAL _SetupBombsight(SINT8 * MountName, mlVehicleData * VehicleData, SINT32 type)
{
	BombSightData* BSD = nullptr;
	weapAmmoType * AmmoType = nullptr;
	mlComponent * Component = nullptr;
	mlWeaponMount * Mount = nullptr;
	OBJECT * TempObj1 = nullptr;
	OBJECT * PivotDOF = nullptr;
	OBJECT * AltPntDOF = nullptr;
	OBJECT * SpeedPntDOF = nullptr;
	OBJECT * ForeSightDOF = nullptr;
	OBJECT * BackSightDOF = nullptr;
	mlComponentListIterator it;

	if (_Data != nullptr)
	{
		piExitProgram(_FL,"Bombsight data being setup with non-null data pointer!");
	}

	mlComponentListBegin(VehicleData->ComponentList, &it);
	for(; it.comp; mlComponentListNext(&it))
	{
		Component = it.comp;

		if (Component->Type == ML_COMPONENT_WEAPONMOUNT)
		{
			Mount = (mlWeaponMount*)Component->SpecialData;

			if (Mount == nullptr)
			{
				continue;
			}

			if (strcmp((char*)Mount->MountName,MountName) != 0)
			{
				continue;
			}

			AmmoType = Mount->Weapons[0].WeaponType->AmmoType[0];
			break;
		}
	}

	if (AmmoType == nullptr)
	{
		WriteDebug("Unable to find ammo type %s for bombsight",MountName);
		return FALSE;
	}

	if (VehicleData->VehicleObjectData == nullptr ||
		VehicleData->VehicleObjectData->PlaneObject == nullptr)
	{
		return FALSE;
	}

	TempObj1 = VehicleData->VehicleObjectData->PlaneObject;
	PivotDOF = AltPntDOF = SpeedPntDOF = ForeSightDOF = BackSightDOF = nullptr;
	ogrpFindSubObjectByDOFVar(&PivotDOF,TempObj1,ML_DOF_BOMBSIGHT_PIVOT);
	if (PivotDOF == nullptr)
	{
		WriteDebug("Unable to find pivot point for bombsight");
		return FALSE;
	}
	ogrpFindSubObjectByDOFVar(&AltPntDOF,PivotDOF,ML_DOF_GAUGE_BOMBSIGHT_ALT);
	ogrpFindSubObjectByDOFVar(&SpeedPntDOF,PivotDOF,ML_DOF_GAUGE_BOMBSIGHT_SPEED);
	ogrpFindSubObjectByDOFVar(&ForeSightDOF,PivotDOF,ML_DOF_BOMBSIGHT_FORESIGHT);
	ogrpFindSubObjectByDOFVar(&BackSightDOF,PivotDOF,ML_DOF_BOMBSIGHT_BACKSIGHT);

	if (AltPntDOF == nullptr)
	{
		WriteDebug("Unable to find altitude point for bombsight");
		return FALSE;
	}
	if (SpeedPntDOF == nullptr)
	{
		WriteDebug("Unable to find speed point for bombsight");
		return FALSE;
	}
	if (ForeSightDOF == nullptr)
	{
		WriteDebug("Unable to find foresight point for bombsight");
		return FALSE;
	}
	if (BackSightDOF == nullptr)
	{
		WriteDebug("Unable to find backsight point for bombsight");
		return FALSE;
	}


	_Data = BSD = (BombSightData*)memAlloc(sizeof(BombSightData));

	BSD->PivotDOF = PivotDOF;
	BSD->AltPntDOF = AltPntDOF;
	BSD->SpeedPntDOF = SpeedPntDOF;
	BSD->ForeSightDOF = ForeSightDOF;
	BSD->BackSightDOF = BackSightDOF;
	BSD->ForeSightDOF->flag2 |= OBJECT_DOF_USECHILDMATRIX;
	BSD->BackSightDOF->flag2 |= OBJECT_DOF_USECHILDMATRIX;

	BSD->AmmoType = AmmoType;

	switch(type)
	{
		case BOMBSIGHT_TYPE_BRITISH_MK9A:
			BSD->minAlt 	= _MK9A_MIN_ALT;
			BSD->maxAlt 	= _MK9A_MAX_ALT;
			BSD->minSpeed 	= _MK9A_MIN_SPEED;
			BSD->maxSpeed 	= _MK9A_MAX_SPEED;
			BSD->altRate 	= _MK9A_ALT_CHANGE_RATE;
			BSD->speedRate 	= _MK9A_SPEED_CHANGE_RATE;
			BSD->useEnglish	= TRUE;
		break;

		case BOMBSIGHT_TYPE_GERMAN_LOTFEC7B:
			BSD->minAlt		= _LOTFEC7B_MIN_ALT;
			BSD->maxAlt		= _LOTFEC7B_MAX_ALT;
			BSD->minSpeed	= _LOTFEC7B_MIN_SPEED;
			BSD->maxSpeed	= _LOTFEC7B_MAX_SPEED;
			BSD->altRate	= _LOTFEC7B_ALT_CHANGE_RATE;
			BSD->speedRate	= _LOTFEC7B_SPEED_CHANGE_RATE;
			BSD->useEnglish = FALSE;
		break;
	}

	return TRUE;
}

/************************************************************************************************
	_CalcBombAngle
*************************************************************************************************/

static FLOAT32 _CalcBombAngle(FLOAT32 Alt,FLOAT32 InSpeed,weapAmmoType * AmmoType)
{
	FLOAT64 Mach,BaseMach;
	FLOAT64 Distance,LastDistance;
	FLOAT64 Time,LastTime;
	FLOAT64 Speed;
	FLOAT64 TempNum;
	SINT32 Index1,Index2;
	FLOAT64 Cd;
	FLOAT64 Delta;
	FLOAT64 Density;
	FLOAT64 NextRange;
	FLOAT64VECTOR Accel;
	FLOAT64VECTOR TempVec2;
	FLOAT64 DeltaTime;
	FLOAT64VECTOR LastPosition;
	FLOAT64VECTOR TempPos;
	FLOAT32 Angle;
	weapAmmo Ammo;
	SINT32 i;

	DeltaTime = TIME_INCREMENT;

	Distance = LastDistance = 0.0;
	Time = LastTime = 0.0;

	memset(&Ammo,0,sizeof(weapAmmo));
	Ammo.Velocity.y = InSpeed;

	NextRange = 0.0;
	i = 0;

	while (Ammo.Position.z > -Alt)
	{
		LastDistance = Distance;
		LastTime = Time;

		Time += TIME_INCREMENT;

		mlGetAltSpeedOfSound(&BaseMach,Ammo.Position.z + Alt);
		mlGetAltDensity(&Density,Ammo.Position.z + Alt);

		Speed = mathExactVectorLength(&(Ammo.Velocity));
		Mach = Speed / BaseMach;

		Index1 = (UINT32)(floor(Mach / G1_CHART_MACH_DELTA));
		Index2 = Index1+1;

		if (Index2 >= G1_CHART_NUM_DIVISIONS)
		{
			Cd = G1MachChart[G1_CHART_NUM_DIVISIONS - 1];
		}
		else
		{
			Delta = (Mach / G1_CHART_MACH_DELTA) - Index1;

			mlInterpolate(&Cd,G1MachChart[Index1],G1MachChart[Index2],Delta);
		}

	//	TempNum = Density / Density0 * 0.5 * Cd / Ammo->AmmoType->BallisticCoefficient;
		// NOTE: the 1790 is a constant that makes the Cd table work with
		// G1 ballistic coefficients and this particular formula
		TempNum = Density * Cd * Speed / (AmmoType->BallisticCoefficient * 1790.0f);
		Accel.x = TempNum * Ammo.Velocity.x;
		Accel.y = TempNum * Ammo.Velocity.y;
		Accel.z = TempNum * Ammo.Velocity.z;

		TempVec2.x = (Accel.x + Ammo.LastAcc.x) * 0.5;
		TempVec2.y = (Accel.y + Ammo.LastAcc.y) * 0.5;
		TempVec2.z = (Accel.z + Ammo.LastAcc.z) * 0.5;

		Ammo.Velocity.x -= TempVec2.x * DeltaTime;
		Ammo.Velocity.y -= TempVec2.y * DeltaTime;
		Ammo.Velocity.z -= TempVec2.z * DeltaTime;

		Ammo.LastAcc = Accel;

		// add in gravity
		Ammo.Velocity.z -= 9.8 * DeltaTime;

		LastPosition = Ammo.Position;
		// now apply velocity vector
		if (fabs(Ammo.LastVel.x) > fabs(Ammo.Velocity.x) ||
			fabs(Ammo.LastVel.y) > fabs(Ammo.Velocity.y))
		{
			Ammo.Position.x += Ammo.LastVel.x * DeltaTime + 0.5 * TempVec2.x * DeltaTime * DeltaTime;
			Ammo.Position.y += Ammo.LastVel.y * DeltaTime + 0.5 * TempVec2.y * DeltaTime * DeltaTime;
			Ammo.Position.z += Ammo.LastVel.z * DeltaTime + 0.5 * TempVec2.z * DeltaTime * DeltaTime;
		}
		else
		{
			Ammo.Position.x += Ammo.Velocity.x * DeltaTime;
			Ammo.Position.y += Ammo.Velocity.y * DeltaTime;
			Ammo.Position.z += Ammo.Velocity.z * DeltaTime;
		}

		Ammo.LastVel = Ammo.Velocity;

	}

	// ok figure out angle to bombpoint
	Delta = (Alt - -LastPosition.z);
	Delta /= (-Ammo.Position.z - -LastPosition.z);

	TempPos = Ammo.Position;
	TempPos.y = LastPosition.y + Delta * (Ammo.Position.y - LastPosition.y);
	TempPos.z = -Alt;

	Angle = (FLOAT32)(atan2(TempPos.z,TempPos.y));

	return Angle;

}

/************************************************************************************************
	_UpdateBombsight
*************************************************************************************************/

static LOGICAL _UpdateBombsight()
{
	BombSightData* BSD;
	FLOAT32 DeltaTime;
	FLOAT32 Adjust;
	piVECTOR V1,V2;
	piVECTOR Att;
	piMATRIX Mat1;
	FLOAT32 ViewAngle,Angle;
	FLOAT32 Pitch,Roll;
	static SINT32 _LastUpdateTime;

	if (_Data == nullptr)
	{
		return FALSE;
	}
	BSD = _Data;

	DeltaTime = (frametime - _LastUpdateTime) / 1000.0f;
	_LastUpdateTime = frametime;

	// speed
	Adjust = (_AdjustAltDir * DeltaTime * BSD->altRate);
	BSD->CurrentAlt += Adjust;

	// alt
	Adjust = (_AdjustSpeedDir * DeltaTime * BSD->speedRate);
	BSD->CurrentSpeed += Adjust;


	if (BSD->CurrentAlt < BSD->minAlt)
	{
		BSD->CurrentAlt = BSD->minAlt;
	}
	if (BSD->CurrentAlt > BSD->maxAlt)
	{
		BSD->CurrentAlt = BSD->maxAlt;
	}
	if (BSD->CurrentSpeed < BSD->minSpeed)
	{
		BSD->CurrentSpeed = BSD->minSpeed;
	}
	if (BSD->CurrentSpeed > BSD->maxSpeed)
	{
		BSD->CurrentSpeed = BSD->maxSpeed;
	}

	if(BSD->useEnglish)
		mlSetBombsightReadoutEnglish(BSD->CurrentAlt,BSD->CurrentSpeed);
	else
		mlSetBombsightReadoutMKS(BSD->CurrentAlt,BSD->CurrentSpeed);

	mlUpdateBombSightGauges(BSD->CurrentSpeed,BSD->CurrentAlt);

	// tilt the view to line up the two points
	V1.x = BSD->ForeSightDOF->dof->dofmatrix2._41;
	V1.y = BSD->ForeSightDOF->dof->dofmatrix2._42;
	V1.z = BSD->ForeSightDOF->dof->dofmatrix2._43;

	math3dVectorMultiply(&V1,&V1,&BSD->ForeSightDOF->matint);

	V2.x = BSD->BackSightDOF->dof->dofmatrix2._41;
	V2.y = BSD->BackSightDOF->dof->dofmatrix2._42;
	V2.z = BSD->BackSightDOF->dof->dofmatrix2._43;

	math3dVectorMultiply(&V2,&V2,&BSD->BackSightDOF->matint);

	mathSubVectors(&V1,&V1,&V2);

	V2.x = V2.z = 0.0f;
	V2.y = 1.0f;
	Mat1 = BSD->BackSightDOF->matint;
	Mat1._41 = Mat1._42 = Mat1._43 = 0.0f;
	math3dVectorMultiply(&V2,&V2,&Mat1);
	math3dNormalize(&V1);
	math3dNormalize(&V2);
	Att.x = Att.y = Att.z = 0.0f;
	ViewAngle = Att.x = (FLOAT32)(acos(math3dDotProduct(&V1,&V2)));

	mathEul_ToMatrix(&Mat1,EulOrdXYZr,&Att);

	BSD->BackSightDOF->dof->dofposmatrix = Mat1;
	BSD->BackSightDOF->dof->dofflags |= DOF_USE_DOFPOSMATRIX;

	// calculate the overall angle needed

	mlVehicleData * VehicleData = persGetPersonaModelDataPtr();
	if (VehicleData == nullptr)
	{
		Angle = 0.0;
	}
	else
	{
		if (VehicleData->BombSightAmmoType == nullptr)
		{
			Angle = 0.0;
		}
		else
		{
			Angle = -_CalcBombAngle(BSD->CurrentAlt,BSD->CurrentSpeed,(weapAmmoType*)VehicleData->BombSightAmmoType);
		}
	}
	Angle -= ViewAngle;

	// compensate for plane attitude
	_PlaneAttitudeAdjustFactor(&Pitch,&Roll);

	if (Pitch > 0.25)
	{
		Pitch = 0.25;
	}
	if (Pitch < -0.25)
	{
		Pitch = -0.25;
	}
	if (Roll > 0.25)
	{
		Roll = 0.25;
	}
	if (Roll < -0.25)
	{
		Roll = -0.25;
	}

	Att.x = Att.y = Att.z = 0.0f;
	Att.x = Angle + Pitch;
	Att.y = -Roll;

	mathEul_ToMatrix(&Mat1,EulOrdXYZr,&Att);

	BSD->PivotDOF->dof->dofposmatrix = Mat1;
	BSD->PivotDOF->dof->dofflags |= DOF_USE_DOFPOSMATRIX;

	return TRUE;
}

/************************************************************************************************
	_PlaneAttitudeAdjustFactor
*************************************************************************************************/

static LOGICAL _PlaneAttitudeAdjustFactor(FLOAT32 * Pitch,FLOAT32 * Roll)
{
	mlVehicleData* VehicleData = persGetPersonaModelDataPtr();
	if (VehicleData == nullptr)
	{
		return FALSE;
	}

	FLOAT64VECTOR V1;
	FLOAT32 Pitch1,Roll1;
	static FLOAT32 _LastPitch,_LastRoll;

	V1 = VehicleData->VehicleAcc;
	V1.x = -V1.x;
	V1.z = -V1.z;
	V1.z += 9.8f;
	mathVectorMultiply64(&V1,&V1,&VehicleData->VehicleAttInvMat);

	Pitch1 = (FLOAT32)(atan2(V1.y, V1.z));
	Roll1 = (FLOAT32)(atan2(V1.x, V1.z));

	_LastPitch = *Pitch = (Pitch1 + _LastPitch * 20.0f) / 21.0f;
	_LastRoll = *Roll = (Roll1 + _LastRoll * 20.0f) / 21.0f;

	return TRUE;
}
