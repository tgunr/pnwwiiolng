#include <syscfg.h> // Path is project specific.

#include <math.h>

#include "pi/pi.h"
#include "include/vars.h"

#include "math/math3d.h"
#include "math/matheul.h"
#include "math/mathvect.h"
#include "math/mathnum.h"

#include "ml/mltypes.h"
#include "ml/mlgear.h"
#include "ml/mlutility.h"
#include "ml/mlgauges.h"
#include "ml/mlCompon.h"

#include "pers/persmain.h"

#include "ogrp/ogrptype.h"

#include "ob/obcd.h"
#include "trn/trnop.h"
#include "trn/trncd.h"
#include "trn/trnmain.h"

#include "draw/draw3d.h"
#include "math/mathnum.h"

#if defined(USE_OSG)
#include <osgSim/DOFTransform>
#include "osgVehicle/VehicleBase.h"
#include "osg/Interpolate.h"
#include "osgTerrain/terrainMain.h"
#include "colliderMain.h"
#include "osgTools/debugTools.h"
#include "osgPhysics/osgODE.h"
#include "osgCollider/colliderSurface.h"
#include "osgVehicle/vehicleSpawn.h"
using namespace osg; // damn PI
#endif

static UINT32 _HandleCastor(mlGearComponentData * GearData,
							   FLOAT64 WheelMass,
							   FLOAT64VECTOR* GearSumForce,
							   FLOAT64 DeltaTime,
							   piMATRIX const* VehicleAttMat);

static UINT32 _HandleGearStrut(mlGearComponentData * GearData,
							   FLOAT64 WheelMass,
							   FLOAT64VECTOR* GearSumForce,
							   FLOAT64 DeltaTime,
							   piMATRIX const* VehicleAttMat,
							   UINT32 DamageLevel,
							   mlComponent * Component);

static UINT32 _DetermineGearCollision(FLOAT64VECTOR * ReturnForce,
									  FLOAT64VECTOR const * ContactPoint,
									  FLOAT64VECTOR const * PrevContactPoint,
									  FLOAT64VECTOR const * ContactVel,
									  FLOAT64VECTOR const * PrevContactVel,
									  FLOAT64 WheelRadius,
									  FLOAT64* WheelRotationSpeed,
									  FLOAT64 WheelRotationalInertia,
#if defined(USE_OSG)
									 void*  ContactPointDOF,
#else
									  OBJECT * ContactPointDOF,
#endif
									  FLOAT64 DeltaTime,
									  piMATRIX const* VehicleAttMat,
									  mlGearComponentData * GearData,
									 FLOAT64 BrakeLeft,
									 FLOAT64 BrakeRight,
									 FLOAT64 BrakeCenter,
									 FLOAT64 VehicleMass,
									 OBJECT ** CollideObjects,
									 trnCDTerrainObject ** CollideTOObjects,
									 OBJECT ** TerrainCollideObjects,
									 UINT32 * TerrainCollidePolys,
									 UINT32 NumTerrainCollidePolys,
									 UINT32 PassNum,
									FLOAT64VECTOR const* SavedVehicleForce,
									FLOAT64VECTOR* TempNormForceSum,
									UINT32 DamageLevel,
									FLOAT32 * StaticFriction,
									FLOAT32 * DynamicFriction,
									FLOAT32 * BumpDensity,
									FLOAT32 * BumpHeight,
									FLOAT32 * RollCoefficientMult,
									UINT32 CollisionsOff,
									mlVehicleData const* VehicleData,
									mlComponent const* Component);

#if defined(USE_OSG)
static UINT32 _DeterminePointCollision(FLOAT64* NormalDepth,
										FLOAT64VECTOR* NormalVector,
										FLOAT32* StaticFriction,
										FLOAT32* DynamicFriction,
										FLOAT32 * BumpDensity,
										FLOAT32 * BumpHeight,
										FLOAT32 * RollCoefficientMult,
										FLOAT64 CollideLength,
										mlGearComponentData const* GearData,
										mlVehicleData const* VehicleData);

#else
static UINT32 _DeterminePointCollision(FLOAT64 * NormalDepth,
										FLOAT64VECTOR * NormalVector,
										piMATRIX const* ContactPointAttMat,
										FLOAT64VECTOR const* ContactPoint,
										FLOAT64VECTOR const* PrevContactPoint,
										FLOAT64 DeltaTime,
										OBJECT ** CollideObjects,
										trnCDTerrainObject ** CollideTOObjects,
										OBJECT ** TerrainCollideObjects,
										UINT32* TerrainCollidePolys,
										UINT32 NumTerrainCollidePolys,
										FLOAT64 CollideLength,
										FLOAT32 * StaticFriction,
										FLOAT32 * DynamicFriction,
										FLOAT32 * BumpDensity,
										FLOAT32 * BumpHeight,
										FLOAT32 * RollCoefficientMult,
										UINT32 CollisionsOff,
										mlComponent const* Component,
										mlGearComponentData const* GearData,
										LOGICAL LimitLength,
										mlVehicleData const* VehicleData);
#endif

#if(0)
static UINT32 _GenerateSeededRandom(UINT32* Return,UINT32 Seed);
#endif

UINT32 NumTris;
UINT32 NumObjs;

static UCHAR _GearSubtypes[ML_GEAR_MAX_SUBTYPES][32] =
{
	"none",
	"wheel",
	"skid",
	"trackroad",
	"trackdrive",
	"transmission",
};

static UCHAR _GearVarnames[ML_GEAR_MAX_VARNAME][32] =
{
	"none",
	"wheelradius",
	"strutspringstrength",
	"strutlength",
	"attachedenginecomponent",
	"contactpointtype",
	"wheelrotationalinertia",
	"strutshockstrength",
	"gearnum",
	"castorrotinertia",
	"drivewheel",
	"braketype",
	"brakestrength",
	"rollcoefficient",
	"tracknum",
	"strutshockreturnstrength",
	"atspeedrollcoefficient",
	"atspeedkph",
	"collidelength",
};

static UCHAR _GearContactType[ML_GEAR_MAX_CONTACTTYPES][32] =
{
	"none",
	"tire",
	"metal",
	"wood",
};

static UCHAR _BrakeType[ML_GEAR_MAX_BRAKETYPE][32] =
{
	"none",
	"left",
	"right",
	"center",
};

UINT32 _DisableCollisions;

extern LOGICAL IsFirstTic;
extern LOGICAL DisplayCollideLines;

UINT32 mlgearSetComponentSubtype(void*  SpecialData, char * SubType)
{
mlGearComponentData * ComponentData;

	if (SpecialData == NULL || SubType == NULL)
	{
		return 1;
	}

	ComponentData = (mlGearComponentData*)SpecialData;

	mlUtilityFindVarname(&ComponentData->Subtype,
						(UCHAR*)SubType,
						(UCHAR*)&_GearSubtypes[0][0],
						32,
						ML_GEAR_MAX_SUBTYPES);

	return 0;
}

UINT32 mlgearAddSpecialVariable(void*  SpecialData,
								 char * VarName,
								 char * VarContents,
								 mlComponentList * ComponentList,
								 mlComponent * Component)
{
mlGearComponentData * ComponentData;
UINT32 VarNum;

	if (SpecialData == NULL ||
		VarName == NULL ||
		VarContents == NULL)
	{
		return 1;
	}

	ComponentData = (mlGearComponentData*)SpecialData;

	mlUtilityFindVarname(&VarNum,
						(UCHAR*)VarName,
						(UCHAR*)&_GearVarnames[0][0],
						32,
						ML_GEAR_MAX_VARNAME);


	switch (VarNum)
	{
		case ML_GEAR_VARNAME_WHEELRADIUS:
			ComponentData->WheelRadius = atof(VarContents);
			break;
		case ML_GEAR_VARNAME_STRUTSPRINGSTRENGTH:
			ComponentData->StrutSpringStrength = atof(VarContents);
			break;
		case ML_GEAR_VARNAME_STRUTSHOCKSTRENGTH:
			ComponentData->StrutShockCompressStrength = atof(VarContents);
			if (ComponentData->StrutShockReturnStrength == 0.0)
			{
				ComponentData->StrutShockReturnStrength = ComponentData->StrutShockCompressStrength;
			}
			break;
		case ML_GEAR_VARNAME_STRUTSHOCKRETURNSTRENGTH:
			ComponentData->StrutShockReturnStrength = atof(VarContents);
		case ML_GEAR_VARNAME_STRUTLENGTH:
			ComponentData->StrutLength = atof(VarContents);
			break;
		case ML_GEAR_VARNAME_ATTACHEDENGINECOMPONENT:
			break;
		case ML_GEAR_VARNAME_CONTACTPOINTTYPE:
			mlUtilityFindVarname(&ComponentData->ContactPointType,
								 (UCHAR*)VarContents,
								 (UCHAR*)&_GearContactType[0][0],
								 32,
								 ML_GEAR_MAX_CONTACTTYPES);
			break;
		case ML_GEAR_VARNAME_WHEELROTATIONALINERTIA:
			ComponentData->WheelRotationalInertia = atof(VarContents);
			break;
		case ML_GEAR_VARNAME_GEARNUM:
			ComponentData->GearNum = atoi(VarContents);
			break;
		case ML_GEAR_VARNAME_CASTOR_ROTINERTIA:
			ComponentData->CastorRotInertia = atof(VarContents);
			break;
		case ML_GEAR_VARNAME_ATSPEEDROLLCOEFFICIENT:
			ComponentData->AtSpeedRollCoefficient = atof(VarContents);
			break;
		case ML_GEAR_VARNAME_ATSPEEDKPH:
			ComponentData->AtSpeedMetersPerSec = atof(VarContents) * (1.0/3.6);
			break;

		case ML_GEAR_VARNAME_DRIVEWHEEL_COMPONENT:
			mlUtilityFindComponentByName(&ComponentData->DriveWheelComponent,
										ComponentList,
										(UCHAR*)VarContents,
										Component);
			break;
		case ML_GEAR_VARNAME_BRAKETYPE:
			mlUtilityFindVarname(&ComponentData->BrakeType,
								 (UCHAR*)VarContents,
								 (UCHAR*)&_BrakeType[0][0],
								 32,
								 ML_GEAR_MAX_BRAKETYPE);

			break;
		case ML_GEAR_VARNAME_BRAKEFORCE:
			ComponentData->BrakeForce = atof(VarContents);
			break;
		case ML_GEAR_VARNAME_ROLLCOEFFICIENT:
			ComponentData->RollCoefficient = atof(VarContents);
			break;
		case ML_GEAR_VARNAME_TRACKNUM:
			ComponentData->TrackNum = atoi(VarContents);
			break;
		case ML_GEAR_VARNAME_COLLIDELENGTH:
			ComponentData->CollideLength = atof(VarContents);
			break;
		default:
			Write_Debug("Unidentified variable %s in component %s\n",VarName,Component->ComponentName);
			break;

	}

	return 0;
}

UINT32 mlUpdateGearComponent(FLOAT64VECTOR * Force, // OUT
							 FLOAT64ATT * Torque, // OUT
							 mlComponent * Component,
							 FLOAT64VECTOR const* VehiclePos,
							 FLOAT64VECTOR const* VehicleVel,
							 piMATRIX const* VehicleAttMat,
							 FLOAT64ATT const* VehicleAttVel,
							 FLOAT64 DeltaTime,
							 FLOAT64 BrakeLeft,
							 FLOAT64 BrakeRight,
							 FLOAT64 BrakeCenter,
							 FLOAT64 VehicleMass,
							 OBJECT ** CollideObjects,// IN
							 trnCDTerrainObject ** CollideTOObjects, // IN
							 OBJECT ** TerrainCollideObjects, // OUT don't care
							 UINT32 * TerrainCollidePolys, // OUT don't care
							 UINT32 NumTerrainCollidePolys,
							 UINT32 CollisionsOff,
							UINT32 PassNum,
							FLOAT64VECTOR const* SavedVehicleForce,
							FLOAT64VECTOR* NormForceSum, // OUT
							void*  InVehicleData)
{
piMATRIX Matrix1 = IdentityMatrix,Matrix2 = IdentityMatrix;
mlGearComponentData * GearData = NULL,*DriveWheel = NULL;
FLOAT64VECTOR GearSumForce = {0, 0, 0};
FLOAT64VECTOR TempForce = {0, 0, 0};
FLOAT64VECTOR ComponentVel = {0, 0, 0};
FLOAT64VECTOR TempVec1 = {0, 0, 0},TempVec2 = {0, 0, 0};
piVECTOR TempPiVec = {0, 0, 0};
UINT32 i;
UINT32 TempInt = 0;
UINT32 DamageLevel = 0;
mlVehicleData * VehicleData;

	VehicleData = (mlVehicleData*)InVehicleData;

	Force->x = Force->y = Force->z = 0;
	Torque->Roll = Torque->Pitch = Torque->Yaw = 0;

	if (Component == NULL) return 0;
	if (_DisableCollisions != 0) return 0;

	GearData = (mlGearComponentData*)(Component->SpecialData);
	if (GearData == NULL) return 0;


// determine damage to gear
	TempInt = 0;
	for (i=0;i<4;i++)
	{
		TempInt += Component->NewDamageNumHits[i];
	}
	if (TempInt > 0)
	{
		for (i=0;i<4;i++)
		{
			Component->CurrentDamage += Component->NewDamage[i];
		}
	}

	DamageLevel = 0;
	if (Component->CurrentDamage > Component->DamageLevels[0])
	{
		DamageLevel = 1;
	}
	if (Component->CurrentDamage > Component->DamageLevels[1])
	{
		DamageLevel = 2;
	}
	if (Component->CurrentDamage > Component->DamageLevels[2])
	{
		DamageLevel = 3;
	}

	{
		// hack for boats and cheese
		vehVehicle*Vehicle = persGetPersonaVehiclePtr();
		if (Vehicle != NULL && Vehicle->VehicleType->Type == VEH_TYPE_SEA && Vehicle->ModelData != NULL)
		{
			if (Component->CurrentDamage > Component->DamageLevels[3])
			{
				if (Vehicle->ModelData->ShipSinking == 0)
				{
					Vehicle->ModelData->ShipSinking = frametime;
				}
			}

		}
	}


	GearSumForce.x = GearSumForce.y = GearSumForce.z = 0;

	// compute contact point positions
	// first extract the position of the contact point right now in the universe
	if (PassNum == 0)
	{
		if (GearData->ContactPoint == NULL)
		{
			if (GearData->Subtype == ML_GEAR_SUBTYPE_TRACK_ROADWHEEL &&
				GearData->DriveWheelComponent != NULL)
			{
				DriveWheel = (mlGearComponentData*)GearData->DriveWheelComponent->SpecialData;
				mlUpdateWheelSpeed(GearData->GearNum - 1,
							-DriveWheel->WheelRotationSpeed * DriveWheel->WheelRadius / GearData->WheelRadius / PI /2);
			}
			return 0;
		}

#if defined(USE_OSG)
	    // doesn't take into account relative dof transforms
	    dGeomID contactdof = (dGeomID)GearData->ContactPoint;

        osg::Vec3d start, end;
	    osgGeomRayGet(dGeomTransformGetGeom(contactdof), start, end);

	    GearData->ContactPointCoords.x = start.x();
	    GearData->ContactPointCoords.y = start.y();
	    GearData->ContactPointCoords.z = start.z();

        // wstrut DOF
		osgSim::DOFTransform* shockdof = (osgSim::DOFTransform*)GearData->ShockDOF;

        if(shockdof)
        {
            // Attempt to mimic what is happening with non-osg. Have no idea what
            // is happening here, but this makes it work.

            // See ogrpmain (_MoveAndRotateSubObject)
            osg::Vec3d v = shockdof->getMaxTranslate() - shockdof->getMinTranslate();
            v *= GearData->LastCompression;
            v += shockdof->getMinTranslate();

            osg::Matrix m;

            m(3, 0) = v.x();
            m(3, 1) = v.y();
            m(3, 2) = v.z();

            osg::Matrix mat1 = shockdof->getInversePutMatrix();
            osg::Matrix mat2 = shockdof->getPutMatrix();

            // Make cumulative matrix for this dof (???)
            osg::Matrix cumulative;
            cumulative = mat1 * m;
            cumulative = cumulative * mat2;

	        GearData->ContactPointCoords.x = start.x() + cumulative(3, 0);
	        GearData->ContactPointCoords.y = start.y() + cumulative(3, 1);
	        GearData->ContactPointCoords.z = start.z() + cumulative(3, 2);
        }
#else
		OBJECT* ContactPoint = GearData->ContactPoint;
		Matrix1 = IdentityMatrix;
		Matrix1._41 = ContactPoint->dof->dofmatrix2._41;
		Matrix1._42 = ContactPoint->dof->dofmatrix2._42;
		Matrix1._43 = ContactPoint->dof->dofmatrix2._43;
		Matrix2 = ContactPoint->matint;
		math3dMatrixMultiply(&Matrix1,&Matrix1,&Matrix2);
		mlUtilityGetModelPositionFromObjectMatrix( &GearData->ContactPointCoords,&Matrix1);
#endif
		// calculate contact point velocity
		TempVec1 = GearData->ContactPointCoords;
		TempPiVec.x = -(FLOAT32)(VehicleAttVel->Pitch * 0.001);
		TempPiVec.y = -(FLOAT32)(VehicleAttVel->Roll * 0.001);
		TempPiVec.z = (FLOAT32)(VehicleAttVel->Yaw * 0.001);
		mathEul_ToMatrix(&Matrix1,EulOrdXYZs,&TempPiVec);

		mathVectorMultiply64(&TempVec2,&TempVec1,&Matrix1);
		ComponentVel.x = (TempVec2.x - TempVec1.x) * (1/0.001);
		ComponentVel.y = (TempVec2.y - TempVec1.y) * (1/0.001);
		ComponentVel.z = (TempVec2.z - TempVec1.z) * (1/0.001);

		// ok now take the component rotational velocity vector and
		// make it relative to the world
		mathVectorMultiply64(&ComponentVel,&ComponentVel,VehicleAttMat);

		// add to it the vehicle's velocity
		mathAddVectors(&ComponentVel,&ComponentVel,VehicleVel);

		// position force and mass point at this point
		// this is one of the few instances where the component position
		// from the component file is overridden
		if (!Component->OverridePos)
			Component->OriginalRelativePos = GearData->ContactPointCoords;

		// now factor in vehicle position
		GearData->WorldContactPoint = GearData->ContactPointCoords;

		mathVectorMultiply64(&(GearData->WorldContactPoint),
							 &(GearData->WorldContactPoint),
							 VehicleAttMat);
		mathAddVectors(&(GearData->WorldContactPoint),
					  &(GearData->WorldContactPoint),
					  VehiclePos);

		if (GearData->PrevWorldContactPoint.x == 0.0 &&
			GearData->PrevWorldContactPoint.y == 0.0 &&
			GearData->PrevWorldContactPoint.z == 0.0)
		{
			GearData->PrevWorldContactPoint = GearData->WorldContactPoint;
		}

		GearData->TempVel = ComponentVel;
		// now compute gear contact point "normal"
	}
	else
	{
		ComponentVel = GearData->TempVel;
	}

	// now take both and determine if a collision occured, if so, what force returned
	TempForce.x = TempForce.y = TempForce.z = 0;


	// note that TempForce is going to be perpendicular to the surface contacted
	// use the normal of the contact force as the "line" to check
	_DetermineGearCollision(&TempForce,
							&(GearData->WorldContactPoint),
							&(GearData->PrevWorldContactPoint),
							&ComponentVel,
							&(GearData->PrevComponentVel),
							GearData->WheelRadius,
							&(GearData->WheelRotationSpeed),
							GearData->WheelRotationalInertia,
							GearData->ContactPoint,
							DeltaTime,
							VehicleAttMat,
							GearData,
							BrakeLeft,
							BrakeRight,
							BrakeCenter,
							VehicleMass,
							CollideObjects,
							CollideTOObjects,
							TerrainCollideObjects,
							TerrainCollidePolys,
							NumTerrainCollidePolys,
							PassNum,
							SavedVehicleForce,
							NormForceSum,
							DamageLevel,
							&(GearData->StaticFriction),
							&(GearData->DynamicFriction),
							&(GearData->BumpDensity),
							&(GearData->BumpHeight),
							&(GearData->RollCoefficientMult),
							CollisionsOff,
							VehicleData,
							Component);
	if (PassNum == 1 && GearData->ContactPoint != NULL)
	{
		// wheel rotation speed is in radians, for the gauges it needs to be in rps
		mlUpdateWheelSpeed(GearData->GearNum - 1,-GearData->WheelRotationSpeed / PI / 2);

		if (GearData->TrackNum > 0)
		{
			mlUpdateTrackSpeed(GearData->TrackNum - 1,GearData->WheelRotationSpeed * GearData->WheelRadius);
		}
		// finally do shocks
		// determine z axis vector of shocks
		// return force

		GearSumForce = TempForce;

		_HandleGearStrut(GearData,Component->Mass,&GearSumForce,DeltaTime,VehicleAttMat,DamageLevel,Component);
		_HandleCastor(GearData,Component->Mass,&GearSumForce,DeltaTime,VehicleAttMat);
		*Force = GearSumForce;
	}
	else
	{
		GearData->PrevWorldContactPoint = GearData->WorldContactPoint;
		GearData->PrevComponentVel = ComponentVel;

		Force->x = Force->y = Force->z = 0;
	}


	return 0;
}


// it is assumed that the castor rotates around the z axis of the DOF
// and the "vehicle moving forward" position is y = 1, x = 0
static UINT32 _HandleCastor(mlGearComponentData * GearData,
							   FLOAT64 WheelMass,
							   FLOAT64VECTOR* GearSumForce,
							   FLOAT64 DeltaTime,
							   piMATRIX const*VehicleAttMat)
{
	if (GearData->CastorDOF == NULL) return 0;
	if (GearData->ContactPoint == NULL) return 0;

	GearData->LastCastorYaw = 0.0;

	mlUpdateCastorAttitude(GearData->GearNum-1, GearData->LastCastorYaw / PI);

#if !defined(USE_OSG)
	piMATRIX Matrix1,Matrix2;

    Matrix2 = GearData->CastorDOF->matint;
	math3dInvertMatrix(&Matrix2,&Matrix2);
	Matrix1 = GearData->CastorDOF->mat;
	math3dMatrixMultiply(&Matrix2,&Matrix2,&Matrix1);
	ogrpSetDOFVariable(GearData->CastorDOF, (FLOAT32)GearData->LastCastorYaw / PI, 0, 1);
	ogrpMoveAndRotateSubObject(GearData->CastorDOF, &Matrix1, &Matrix2);
#endif
	return 0;
}


#define STRUT_NUM_LOOPS 50
#define _SINK_TIME 60.0 // 60 seconds to sink ship
static UINT32 _HandleGearStrut(mlGearComponentData * GearData,
							   FLOAT64 WheelMass,
							   FLOAT64VECTOR* GearSumForce,
							   FLOAT64 DeltaTime,
							   piMATRIX const* VehicleAttMat,
							   UINT32 DamageLevel,
							   mlComponent * Component)
{
	FLOAT64 Compression;
	FLOAT64 Velocity;
	FLOAT64 CompressionForce,CompressionForceBase;
	FLOAT64 BumpStrength,ReturnStrength;
	UINT32 i;
	FLOAT64 TempAcc;
	FLOAT64 TimeDelta;
	FLOAT64 StrengthAdjust;

    vehVehicle* Vehicle = persGetPersonaVehiclePtr();

	if (GearData->ShockDOF != NULL)
	{
		// hack for boats and cheese
			StrengthAdjust = 1.0;
		if (Vehicle != NULL && Vehicle->VehicleType->Type == VEH_TYPE_SEA && Vehicle->ModelData != NULL)
		{
			StrengthAdjust = Component->DamageLevels[3];
			if (StrengthAdjust > 0.001)
			{
				StrengthAdjust = 1.0 - (Component->CurrentDamage / StrengthAdjust);
				if (StrengthAdjust < 0.0)
				{
					StrengthAdjust = 0.0;
				}
			}
			else
			{
				StrengthAdjust = 1.0;
			}

			if (Vehicle->ModelData->ShipSinking != 0)
			{
			FLOAT64 Delta;

				Delta = (frametime - Vehicle->ModelData->ShipSinking) / 1000.0;

				if (Delta > _SINK_TIME)
				{
					StrengthAdjust = 0.0;
				}
				else
				{
					StrengthAdjust *= 1.0f - (Delta / _SINK_TIME);
				}
			}
		}



#if defined(USE_OSG)
		osgSim::DOFTransform* shockdof = (osgSim::DOFTransform*)GearData->ShockDOF;
		osg::Matrix attmat;
		Vehicle->vehicle->Attitude().get(attmat);
		osg::Matrix shockmat = shockdof->getInversePutMatrix();
		shockmat(3,0) = shockmat(3,1) = shockmat(3,2) =
		shockmat(0,3) = shockmat(1,3) = shockmat(2,3) = 0;

		CompressionForceBase = (attmat * shockmat * osg::Vec3d(0,0,1)) * osg::Vec3d(GearSumForce->x,GearSumForce->y,GearSumForce->z);
#else
		FLOAT64VECTOR TempVec1,TempVec2;
		piMATRIX Matrix1;

		TempVec1 = *GearSumForce;
		math3dMatrixMultiply(&Matrix1,&GearData->ShockDOF->dof->dofmatrix2,&GearData->ShockDOF->matint);
		Matrix1._41 = Matrix1._42 = Matrix1._43 = 0;
		mlUtilityConvertMatrixToModelReference(&Matrix1,&Matrix1);
		math3dMatrixMultiply(&Matrix1,&Matrix1,VehicleAttMat);
		TempVec2.x = TempVec2.y = 0;
		TempVec2.z = 1.0;
		mathVectorMultiply64(&TempVec2,&TempVec2,&Matrix1);

		CompressionForceBase = mathVectDotProduct(&TempVec2,&TempVec1);
#endif
		TimeDelta = DeltaTime / 1000.0 / STRUT_NUM_LOOPS;

		for (i=0;i<STRUT_NUM_LOOPS;i++)
		{
			CompressionForce = CompressionForceBase; // force from ground
			CompressionForce -= GearData->LastCompression * GearData->StrutSpringStrength * StrengthAdjust; // spring
			Velocity = GearData->GearStrutVel;

			BumpStrength = GearData->StrutShockCompressStrength;
			ReturnStrength = GearData->StrutShockReturnStrength;

			// damage to gear results in shock damage


			// added hack for boats and cheese
			if (Vehicle != NULL && Vehicle->VehicleType->Type == VEH_TYPE_SEA)
			{

			}
			else
			{
				if (DamageLevel >= 2)
				{
					if (BumpStrength > 100.0)
					{
						BumpStrength = 100.0;
					}

					if (ReturnStrength > 100.0)
					{
						ReturnStrength = 100.0;
					}
				}
			}

			if (Velocity > 0.0)
			{
				CompressionForce -= Velocity * GearData->StrutShockCompressStrength;
			}
			else
			{
				CompressionForce -= Velocity * GearData->StrutShockReturnStrength;
			}

			TempAcc = (CompressionForce / WheelMass + GearData->LastCompressionForce / WheelMass) * 0.5;
			GearData->GearStrutVel += TempAcc * TimeDelta;


			GearData->GearStrutVel *= 0.99;
			Compression = GearData->LastCompression +
					GearData->LastStrutVel * TimeDelta +
					0.5 * TempAcc * TimeDelta * TimeDelta;
			if (Compression < 0)
			{
				Compression = 0;
				GearData->GearStrutVel = 0;
			}
			if (Compression > 1.0)
			{
				Compression = 1.0;
				GearData->GearStrutVel = 0;
			}

			GearData->LastCompression = Compression;
			GearData->LastStrutVel = GearData->GearStrutVel;
			GearData->LastCompressionForce = CompressionForce;
		}
		mlUpdateStrutCompression(GearData->GearNum - 1, Compression);
#if !defined(USE_OSG)
		piMATRIX Matrix2 = GearData->ShockDOF->matint;
		math3dInvertMatrix(&Matrix2,&Matrix2);
		Matrix1 = GearData->ShockDOF->mat;
		math3dMatrixMultiply(&Matrix2,&Matrix2,&Matrix1);
		ogrpSetDOFVariable(GearData->ShockDOF, (FLOAT32)Compression, 0, 1);
		ogrpMoveAndRotateSubObject(GearData->ShockDOF, &Matrix1, &Matrix2);

#endif
		}


	return 0;
}

// ReturnForce is relative to the world
// ContactPoint is also
// ContactVel is also
#define _NUM_GEAR_SUB_LOOPS 15
#define _STATIC_FRICTION_SPEED 0.2
#define _INV_STATIC_FRICTION_SPEED (1.0 / _STATIC_FRICTION_SPEED)

static UINT32 _DetermineGearCollision(FLOAT64VECTOR * ReturnForce, // OUT
									  FLOAT64VECTOR const * ContactPoint,
									  FLOAT64VECTOR const * PrevContactPoint,
									  FLOAT64VECTOR const * ContactVel,
									  FLOAT64VECTOR const *,
									  FLOAT64 WheelRadius,
									  FLOAT64* WheelRotationSpeed,
									  FLOAT64 WheelRotationalInertia,
#if defined(USE_OSG)
									 void*  ContactPointDOF,
#else
									  OBJECT * ContactPointDOF,
#endif
									  FLOAT64 DeltaTime,
									  piMATRIX const* VehicleAttMat,
									  mlGearComponentData * GearData, // OUT
									 FLOAT64 BrakeLeft,
									 FLOAT64 BrakeRight,
									 FLOAT64 BrakeCenter,
									 FLOAT64 VehicleMass,
									 OBJECT ** CollideObjects,
									 trnCDTerrainObject ** CollideTOObjects,
									 OBJECT ** TerrainCollideObjects,
									 UINT32 * TerrainCollidePolys,
									 UINT32 NumTerrainCollidePolys,
									 UINT32 PassNum,
									FLOAT64VECTOR const* SavedVehicleForce,
									FLOAT64VECTOR* NormForceSum, // OUT
									UINT32 DamageLevel,
									FLOAT32* StaticFriction, // OUT
									FLOAT32* DynamicFriction, // OUT
									FLOAT32* BumpDensity,// OUT
									FLOAT32* BumpHeight,// OUT
									FLOAT32* RollCoefficientMult,// OUT
									UINT32 CollisionsOff,
									mlVehicleData const* VehicleData,
									mlComponent const* Component)
{
	if (ContactPointDOF == NULL)
	{
		ReturnForce->x = ReturnForce->y = ReturnForce->z = 0;
		return 0;
	}

	FLOAT64 Speed = 0.0f,OneOverSpeed = 0.0f;
	FLOAT64 NormalDepth = 0.0f;
	FLOAT64VECTOR NormalVector = {0, 0, 0},NormalVectorSave = {0, 0, 0};
	FLOAT64VECTOR NormalVel = {0, 0, 0};
	FLOAT64VECTOR NormalForce = {0, 0, 0};
	FLOAT64VECTOR SumNormalForce = {0, 0, 0};
	FLOAT64 Force = 0.0f;
	FLOAT64 OneOverWheelRadius = 0.0f;
	FLOAT64 NewWheelSpeed = 0.0f;
	UINT32 i = 0;
	FLOAT64 Pitch = 0.0f,Roll = 0.0f;
	FLOAT64 TempNum = 0.0f;
	FLOAT64 CosValue = 0.0f,SinValue = 0.0f;
	FLOAT64 WheelRotationForce = 0.0f;
	FLOAT64 BrakeForce = 0.0f;
	FLOAT64 EffectiveBrakeForce = 0.0f;
	mlGearComponentData * DriveWheel = NULL;
	FLOAT64 RotationalInertia = 0.0f;
	FLOAT64 OneOverRotationalInertia = 0.0f;
	UINT32 NumLoops = 0;
	FLOAT64 OneOverNumLoops = 0.0f;
	FLOAT64 RollCoefficient = 0.0f;
	FLOAT64 Length = 0.0f,L1 = 0.0f;
	FLOAT64VECTOR Ratio = {0, 0, 0};
	FLOAT64VECTOR TempForce = {0, 0, 0};
	FLOAT64VECTOR ForceUnit = {0, 0, 0};
	FLOAT64VECTOR TempVec1 = {0, 0, 0},TempVec2 = {0, 0, 0};
	FLOAT64VECTOR StaticFrictionVec = {0, 0, 0};
	FLOAT64 Percent = 0.0f;
	FLOAT64 Vely = 0.0f;
	FLOAT64 WheelRotationalForceSum = 0.0f;
	FLOAT64 TempRotationForce = 0.0f;
	FLOAT64 Movement = 0.0f;
	FLOAT64 BrakePercent = 0.0f;
	FLOAT64 LastWheelSpeed = 0.0f;
	FLOAT64 SpringCo = 0.0f;
	FLOAT64 CollideLength = 0.0f;
	UINT32 SetNormalLengthToZero = 0;

	piMATRIX InvContactPointAttMat = IdentityMatrix;

#if defined(USE_OSG)
    dGeomID dof = (dGeomID)(ContactPointDOF);

    osg::Matrixd m;
    // Get cumulative matrix for this dof
    osgGeomGetRotationMatrix(dof, m);

    piMATRIX ContactPointAttMat;
    ContactPointAttMat._11 = m(0, 0);
    ContactPointAttMat._12 = m(0, 1);
    ContactPointAttMat._13 = m(0, 2);
    ContactPointAttMat._14 = m(0, 3);
    ContactPointAttMat._21 = m(1, 0);
    ContactPointAttMat._22 = m(1, 1);
    ContactPointAttMat._23 = m(1, 2);
    ContactPointAttMat._24 = m(1, 3);
    ContactPointAttMat._31 = m(2, 0);
    ContactPointAttMat._32 = m(2, 1);
    ContactPointAttMat._33 = m(2, 2);
    ContactPointAttMat._34 = m(2, 3);
    ContactPointAttMat._41 = m(3, 0);
    ContactPointAttMat._42 = m(3, 1);
    ContactPointAttMat._43 = m(3, 2);
    ContactPointAttMat._44 = m(3, 3);

 	math3dInvert2(&InvContactPointAttMat, &ContactPointAttMat);
#else
	piMATRIX ContactPointAttMat = IdentityMatrix;
	math3dMatrixMultiply(&ContactPointAttMat, &(ContactPointDOF->dof->dofmatrix2), &(ContactPointDOF->matint));
	ContactPointAttMat._41 = ContactPointAttMat._42 = ContactPointAttMat._43 = 0;
	mlUtilityConvertMatrixToModelReference(&ContactPointAttMat,&ContactPointAttMat);
	math3dMatrixMultiply(&ContactPointAttMat,&ContactPointAttMat,VehicleAttMat);
	math3dInvert2(&InvContactPointAttMat,&ContactPointAttMat);
#endif

	if (PassNum == 0)
	{
		if (GearData->CollideLength > 0.01)
		{
			CollideLength = GearData->CollideLength;
		}
		else
		{
			CollideLength = WheelRadius;
		}
#if defined(USE_OSG)
		_DeterminePointCollision(&NormalDepth,
								 &NormalVector,
								 StaticFriction,
								 DynamicFriction,
								 BumpDensity,
								 BumpHeight,
								 RollCoefficientMult,
								 CollideLength,
								 GearData,
								 VehicleData);
#else
		_DeterminePointCollision(&NormalDepth,
								 &NormalVector,
								 &ContactPointAttMat,
								 ContactPoint,
								 PrevContactPoint,
								 DeltaTime,
								 CollideObjects,
								 CollideTOObjects,
								 TerrainCollideObjects,
								 TerrainCollidePolys,
								 NumTerrainCollidePolys,
								 CollideLength,
								 StaticFriction,
								 DynamicFriction,
								 BumpDensity,
								 BumpHeight,
								 RollCoefficientMult,
								 CollisionsOff,
								 Component,
								 GearData,
								VehicleData->VehicleMasterPtr->VehicleType->Type != VEH_TYPE_SEA,
								VehicleData);
#endif
		if(VehicleData->VehicleMasterPtr->VehicleType->Type != VEH_TYPE_SEA && *RollCoefficientMult > 198.999f && *RollCoefficientMult < 199.001f)
		{
			ReturnForce->x = ReturnForce->y = ReturnForce->z = 0;
			GearData->TempForce.x = GearData->TempForce.y = GearData->TempForce.z = 0;
			return 0;
		}

		{
			vehVehicle* Vehicle = persGetPersonaVehiclePtr();
			if (Vehicle != NULL && Vehicle->VehicleType->Type == VEH_TYPE_SEA)
			{

			}
			else
			{
				if (DamageLevel >= 1 &&
					(GearData->Subtype == ML_GEAR_SUBTYPE_WHEEL ||
					 GearData->Subtype == ML_GEAR_SUBTYPE_TRACK_DRIVEWHEEL)) // tire flat
				{
					NormalDepth += (WheelRadius * 0.2);

					Movement = *WheelRotationSpeed * DeltaTime / 1000.0;
					GearData->LastWheelOrientation += Movement;
					GearData->LastWheelOrientation = fmod((FLOAT32)GearData->LastWheelOrientation,(FLOAT32)(PI*2.0f));
					NormalDepth += cos(GearData->LastWheelOrientation) * WheelRadius * 0.075;
				}
			}
		}

		if (NormalDepth > 0.0)
		{
			NormalDepth = GearData->LastNormalDepth * 0.5;
			GearData->LastNormalDepth = 0.0;
			SetNormalLengthToZero = 1;
			NormalVector = GearData->LastNormal;
		}
		else
		{
			TempNum = NormalDepth;
			NormalDepth = NormalDepth * 0.5 + GearData->LastNormalDepth * 0.5;
			GearData->LastNormalDepth = TempNum;
			SetNormalLengthToZero = 0;
		}

		if (NormalDepth < 0.0)
		{
			Length = mathVectDotProduct(&NormalVector,ContactVel);

			SpringCo = 250000.0;
			Force = -SpringCo * NormalDepth;

			NormalForce.x = NormalVector.x * Force;
			NormalForce.y = NormalVector.y * Force;
			NormalForce.z = NormalVector.z * Force;

			if (Length > 0.0)
			{
				NormalVel.x = NormalVector.x * Length;
				NormalVel.y = NormalVector.y * Length;
				NormalVel.z = NormalVector.z * Length;


				NormalForce.x -= NormalVel.x * 15000.0;
				NormalForce.y -= NormalVel.y * 15000.0;
				NormalForce.z -= NormalVel.z * 15000.0;
			}
			else
			{
				NormalVel.x = NormalVector.x * Length;
				NormalVel.y = NormalVector.y * Length;
				NormalVel.z = NormalVector.z * Length;


				NormalForce.x -= NormalVel.x * 15000.0;
				NormalForce.y -= NormalVel.y * 15000.0;
				NormalForce.z -= NormalVel.z * 15000.0;
			}

			GearData->LastNormalVel = NormalVel;

			mathAddVectors(NormForceSum, NormForceSum, &NormalForce);

			GearData->TempForce = NormalForce;

			GearData->LastNormal = NormalVector;
		}
		else
		{
			NormalForce.x = NormalForce.y = NormalForce.z = 0;
			GearData->TempForce = NormalForce;
		}

		ReturnForce->x = ReturnForce->y = ReturnForce->z = 0;
	}
	else
	{
		NormalForce = GearData->TempForce;

		// calculate static friction force

		if (NormForceSum->x != 0.0)
		{
			Ratio.x = NormalForce.x / NormForceSum->x;
		}
		else
		{
			Ratio.x = 0;
		}
		if (NormForceSum->y != 0.0)
		{
			Ratio.y = NormalForce.y / NormForceSum->y;
		}
		else
		{
			Ratio.y = 0;
		}
		if (NormForceSum->z != 0.0)
		{
			Ratio.z = NormalForce.z / NormForceSum->z;
		}
		else
		{
			Ratio.z = 0;
		}

		mathSubVectors(&TempForce, SavedVehicleForce, NormForceSum);

		ForceUnit.x = TempForce.x * Ratio.x;
		ForceUnit.y = TempForce.y * Ratio.y;
		ForceUnit.z = TempForce.z * Ratio.z;

		TempVec1 = ForceUnit;
		mathVectDoCrossProduct(&TempVec2,&NormalForce,&ForceUnit);
		mathVectDoCrossProduct(&ForceUnit,&NormalForce,&TempVec2);
		Length = (FLOAT32)mathExactVectorLength(&ForceUnit);

		if (Length > 0.00001)
		{
			ForceUnit.x /= Length;
			ForceUnit.y /= Length;
			ForceUnit.z /= Length;
		}
		else
		{
			ForceUnit.x = ForceUnit.y = ForceUnit.z = 0;
		}

		L1 = mathVectDotProduct(&TempVec1,&ForceUnit);

		Length = mathExactVectorLength(&NormalForce);
		if (L1 > *StaticFriction * Length)
		{
			L1 = *StaticFriction * Length;
		}
		if (L1 < -*StaticFriction * Length)
		{
			L1 = (-*StaticFriction * Length);
		}

		StaticFrictionVec.x = -ForceUnit.x * L1;
		StaticFrictionVec.y = -ForceUnit.y * L1;
		StaticFrictionVec.z = -ForceUnit.z * L1;

		DriveWheel = NULL;
		if (GearData->Subtype == ML_GEAR_SUBTYPE_TRACK_ROADWHEEL)
		{
			if (GearData->DriveWheelComponent != NULL)
			{
				DriveWheel = (mlGearComponentData*)GearData->DriveWheelComponent->SpecialData;
			}
		}

		RollCoefficient = 0.02;

		if (DriveWheel != NULL)
		{
			if (DriveWheel->RollCoefficient != 0.0)
			{
				RollCoefficient = DriveWheel->RollCoefficient;
			}
			if (DriveWheel->AtSpeedMetersPerSec > 0.001)
			{
				Speed =  WheelRadius * *WheelRotationSpeed;
				RollCoefficient += fabs(Speed /DriveWheel->AtSpeedMetersPerSec) *
								(DriveWheel->AtSpeedRollCoefficient - DriveWheel->RollCoefficient);
			}
		}
		else
		{
			if (GearData->RollCoefficient != 0.0)
			{
				RollCoefficient = GearData->RollCoefficient;
			}
			if (GearData->AtSpeedMetersPerSec > 0.001)
			{
				Speed =  WheelRadius * *WheelRotationSpeed;
				RollCoefficient += fabs(Speed /GearData->AtSpeedMetersPerSec) *
								(GearData->AtSpeedRollCoefficient - GearData->RollCoefficient);
			}
		}

		if (*RollCoefficientMult > 0.0001)
		{
			RollCoefficient *= *RollCoefficientMult;
		}

		BrakeForce = 0.0;
		if (GearData->BrakeType == ML_GEAR_BRAKETYPE_LEFT)
		{
			if (BrakeLeft > BrakeCenter)
			{
				BrakePercent = (BrakeLeft - 0.3) / 0.7;
				if (BrakePercent < 0.0)
				{
					BrakePercent = 0.0;
				}
				BrakeForce = BrakePercent * GearData->BrakeForce;
			}
			else
			{
				BrakePercent = (BrakeCenter - 0.3) / 0.7;
				if (BrakePercent < 0.0)
				{
					BrakePercent = 0.0;
				}
				BrakeForce = BrakePercent * GearData->BrakeForce;
			}
		}

		if (GearData->BrakeType == ML_GEAR_BRAKETYPE_RIGHT)
		{
			if (BrakeRight > BrakeCenter)
			{
				BrakePercent = (BrakeRight - 0.3) / 0.7;
				if (BrakePercent < 0.0)
				{
					BrakePercent = 0.0;
				}
				BrakeForce = BrakePercent * GearData->BrakeForce;
			}
			else
			{
				BrakePercent = (BrakeCenter - 0.3) / 0.7;
				if (BrakePercent < 0.0)
				{
					BrakePercent = 0.0;
				}
				BrakeForce = BrakePercent * GearData->BrakeForce;
			}
		}
		if (GearData->BrakeType == ML_GEAR_BRAKETYPE_CENTER)
		{
			BrakePercent = (BrakeCenter - 0.3) / 0.7;
			if (BrakePercent < 0.0)
			{
				BrakePercent = 0.0;
			}
			BrakeForce = BrakePercent * GearData->BrakeForce;
		}

		{
			vehVehicle* Vehicle = persGetPersonaVehiclePtr();
			if (Vehicle != NULL && Vehicle->VehicleType->Type == VEH_TYPE_SEA)
			{

			}
			else
			{
				if (DamageLevel > 2) // jammed wheel
				{
					BrakeForce = VehicleMass * 0.25 + GearData->BrakeForce;
					RollCoefficient = 2.0;
				}
			}
		}

		// rotate relative to wheel contact point
		NormalVel = GearData->TempVel;
		NormalVector = NormalForce;
		NormalVectorSave = NormalVector;

		mathVectorMultiply64(&NormalVector,&NormalVector,&InvContactPointAttMat);
		mathVectorMultiply64(&NormalVel,ContactVel,&InvContactPointAttMat);
		mathVectorMultiply64(&StaticFrictionVec,&StaticFrictionVec,&InvContactPointAttMat);

		Pitch = atan2(NormalVector.y,NormalVector.z);

		CosValue = cos(Pitch);
		SinValue = sin(Pitch);
		TempNum = NormalVector.z;
		NormalVector.z = CosValue * NormalVector.z + SinValue * NormalVector.y;
		NormalVector.y = -SinValue * TempNum + CosValue * NormalVector.y;

		TempNum = NormalVel.z;
		NormalVel.z = CosValue * NormalVel.z + SinValue * NormalVel.y;
		NormalVel.y = -SinValue * TempNum + CosValue * NormalVel.y;

		TempNum = StaticFrictionVec.z;
		StaticFrictionVec.z = CosValue * StaticFrictionVec.z + SinValue * StaticFrictionVec.y;
		StaticFrictionVec.y = -SinValue * TempNum + CosValue * StaticFrictionVec.y;


		Roll = atan2(NormalVector.x,NormalVector.z);

		CosValue = cos(Roll);
		SinValue = sin(Roll);
		TempNum = NormalVector.z;
		NormalVector.z = CosValue * NormalVector.z + SinValue * NormalVector.x;
		NormalVector.x = -SinValue * TempNum + CosValue * NormalVector.x;

		TempNum = NormalVel.z;
		NormalVel.z = CosValue * NormalVel.z + SinValue * NormalVel.x;
		NormalVel.x = -SinValue * TempNum + CosValue * NormalVel.x;

		TempNum = StaticFrictionVec.z;
		StaticFrictionVec.z = CosValue * StaticFrictionVec.z + SinValue * StaticFrictionVec.x;
		StaticFrictionVec.x = -SinValue * TempNum + CosValue * StaticFrictionVec.x;

		NumLoops = _NUM_GEAR_SUB_LOOPS;
		OneOverNumLoops = 1.0 / _NUM_GEAR_SUB_LOOPS;

		WheelRotationalForceSum = 0;
		SumNormalForce.x = SumNormalForce.y = 0;
		GearData->LastAxleTorque = 0;
		LastWheelSpeed = 0.0;

		if (GearData->Subtype == ML_GEAR_SUBTYPE_TRACK_DRIVEWHEEL)
		{
			RotationalInertia = GearData->TrackCumulativeRotationalInertia +
							WheelRotationalInertia;
		}
		else
		{
			RotationalInertia = WheelRotationalInertia;
		}

		if (RotationalInertia == 0.0)
		{
			RotationalInertia = 1.0;
		}

		OneOverRotationalInertia = 1.0 / RotationalInertia;
		OneOverWheelRadius = 1.0 / WheelRadius;

		for (i=0;i<NumLoops;i++)
		{

			// factor in wheels
			Vely = NormalVel.y;
			if (GearData->Subtype == ML_GEAR_SUBTYPE_TRACK_ROADWHEEL && DriveWheel != NULL)
			{
				Vely -= DriveWheel->WheelRadius * DriveWheel->WheelRotationSpeed;
			}
			else
			{
				Vely -= WheelRadius * *WheelRotationSpeed;
			}

			// apply friction
			Speed = math3dFastAppxSqrt((FLOAT32)(NormalVel.x * NormalVel.x + Vely * Vely));

			if (Speed < _STATIC_FRICTION_SPEED)
			{
				Percent = Speed * _INV_STATIC_FRICTION_SPEED;
			}
			else
			{
				Percent = 1.0;
			}

			if (Speed == 0.0)
			{
				ForceUnit.x = 0;
				ForceUnit.y = 0;
			}
			else
			{
				OneOverSpeed = 1.0 / Speed;
				ForceUnit.x = NormalVel.x * OneOverSpeed;
				ForceUnit.y = Vely * OneOverSpeed;
			}

			NormalForce = NormalVector;
			NormalForce.x -= Percent * *DynamicFriction * ForceUnit.x * NormalVector.z;
			NormalForce.y -= Percent * *DynamicFriction * ForceUnit.y * NormalVector.z;

			NormalForce.x += (1.0 - Percent) * StaticFrictionVec.x;
			NormalForce.y += (1.0 - Percent) * (StaticFrictionVec.y);


			WheelRotationForce = 0.0;

			if (GearData->Subtype == ML_GEAR_SUBTYPE_TRACK_DRIVEWHEEL)
			{
				WheelRotationForce -= GearData->TrackCumulativeFrictionForce;
			}

			// now factor in power to wheels
			if (WheelRadius != 0.0)
			{
				WheelRotationForce += GearData->DriveTorque * OneOverWheelRadius;
			}
			else
			{
				WheelRotationForce = 0.0;
			}

			TempNum = WheelRotationForce;

			WheelRotationForce -= NormalForce.y;

			TempRotationForce = WheelRotationForce;
			EffectiveBrakeForce = BrakeForce + RollCoefficient * NormalForce.z + RotationalInertia * 0.1;

			// brakes
			if (*WheelRotationSpeed > 0.0)
			{
				if (*WheelRotationSpeed > 1.0)
				{
					WheelRotationForce -= EffectiveBrakeForce;
				}
				else
				{
					if (WheelRotationForce > EffectiveBrakeForce ||
						WheelRotationForce < -EffectiveBrakeForce)
					{
						WheelRotationForce -= EffectiveBrakeForce;
					}
					else
					{
						WheelRotationForce -= EffectiveBrakeForce;
						WheelRotationForce *= *WheelRotationSpeed / 1.0;
					}
				}
			}
			else
			{
				if (*WheelRotationSpeed < -1.0)
				{
					WheelRotationForce += EffectiveBrakeForce;
				}
				else
				{
					if (WheelRotationForce > EffectiveBrakeForce ||
						WheelRotationForce < -EffectiveBrakeForce)
					{
						WheelRotationForce += EffectiveBrakeForce;
					}
					else
					{
						WheelRotationForce += EffectiveBrakeForce;
						WheelRotationForce *= -*WheelRotationSpeed / 1.0;
					}
				}
			}

			WheelRotationalForceSum -= WheelRotationForce;


			if (GearData->Subtype != ML_GEAR_SUBTYPE_TRACK_ROADWHEEL || DriveWheel == NULL)
			{
				NewWheelSpeed = *WheelRotationSpeed + WheelRotationForce * WheelRadius * OneOverRotationalInertia *
								DeltaTime * 0.001 * OneOverNumLoops;
				GearData->LastAxleTorque += WheelRotationForce * WheelRadius;

				if (GearData->Subtype != ML_GEAR_SUBTYPE_WHEEL)
				{
					GearData->LastWheelSpeed = (GearData->LastWheelSpeed * 20.0 + NewWheelSpeed) * (1.0 / 21.0);
				}
				else
				{
					GearData->LastWheelSpeed = NewWheelSpeed;
				}
				*WheelRotationSpeed = GearData->LastWheelSpeed;

			}
			else
			{
				if (WheelRadius != 0.0)
				{
					NewWheelSpeed = DriveWheel->WheelRadius * DriveWheel->WheelRotationSpeed *
											OneOverWheelRadius;
					NewWheelSpeed += *WheelRotationSpeed + WheelRotationForce * WheelRadius * OneOverRotationalInertia *
									DeltaTime * 0.001 * OneOverNumLoops;
					NewWheelSpeed *= 0.5;
					GearData->LastWheelSpeed = (GearData->LastWheelSpeed * 20.0 + NewWheelSpeed) * (1.0 / 21.0);
					*WheelRotationSpeed = GearData->LastWheelSpeed;

				}
			}

			SumNormalForce.x += NormalForce.x;
			SumNormalForce.y += NormalForce.y;
			LastWheelSpeed += *WheelRotationSpeed;
		}


		GearData->LastAxleTorque /= NumLoops;
		NormalForce.x = SumNormalForce.x / NumLoops;
		NormalForce.y = SumNormalForce.y / NumLoops;

		// hack to remove friction for tailwheels
		if (GearData->CastorRotInertia > 0.001 &&
			VehicleData->TailWheelLocked == 0)
		{
			NormalForce.x = 0;
//			NormalForce.y = 0;
		}

		if (GearData->Subtype == ML_GEAR_SUBTYPE_TRACK_ROADWHEEL && DriveWheel != NULL)
		{

			DriveWheel->TrackCumulativeFrictionForce += WheelRotationalForceSum / NumLoops;
			DriveWheel->TrackCumulativeRotationalInertia += GearData->WheelRotationalInertia;
		}
		else
		{
			*WheelRotationSpeed = LastWheelSpeed / NumLoops;
		}

		if (GearData->Subtype == ML_GEAR_SUBTYPE_TRACK_DRIVEWHEEL)
		{
			GearData->LastTrackCumulativeFrictionForce = GearData->TrackCumulativeFrictionForce;

			GearData->TrackCumulativeFrictionForce = 0;
			GearData->TrackCumulativeRotationalInertia = 0;
		}


		// counterrotate back
		CosValue = cos(-Roll);
		SinValue = sin(-Roll);
		TempNum = NormalForce.z;
		NormalForce.z = CosValue * NormalForce.z + SinValue * NormalForce.x;
		NormalForce.x = -SinValue * TempNum + CosValue * NormalForce.x;

		CosValue = cos(-Pitch);
		SinValue = sin(-Pitch);
		TempNum = NormalForce.z;
		NormalForce.z = CosValue * NormalForce.z + SinValue * NormalForce.y;
		NormalForce.y = -SinValue * TempNum + CosValue * NormalForce.y;

		mathVectorMultiply64(&NormalForce,&NormalForce,&ContactPointAttMat);


		*ReturnForce = NormalForce;
	}

	if(VehicleData->PhysicsOff)
	{
		ReturnForce->x = ReturnForce->y = ReturnForce->z = 0;
		*WheelRotationSpeed = 0;
	}
	return 0;
}



#define _CONTACT_POINT_LENGTH 1.0
#define _MAX_NUM_OF_COLLISIONS 32

// NormalDepth is the depth into the surface the contact point is
// NormalVector is the normal vector to the surface, scaled to a unit vector
#if defined(USE_OSG)

static UINT32 _DeterminePointCollision(FLOAT64* NormalDepth,
										FLOAT64VECTOR* NormalVector,
										FLOAT32* StaticFriction,
										FLOAT32* DynamicFriction,
										FLOAT32 * BumpDensity,
										FLOAT32 * BumpHeight,
										FLOAT32 * RollCoefficientMult,
										FLOAT64 CollideLength,
										mlGearComponentData const* GearData,
										mlVehicleData const* VehicleData)
{
	*NormalDepth = 1;

	NormalVector->x = 0;
    NormalVector->y = 0;
    NormalVector->z = 1;

	colliderQuery::CollisionList collisions;
	VehicleData->VehicleObjectData->vehicleCollider->getCollisions((dGeomID)GearData->ContactPoint, collisions, true);

    if(!collisions.empty())
	{
        // Since our collisions now go ground up, take the last one
        unsigned int q = collisions.size() - 1;

        NormalVector->x = collisions[q].normal.x();
		NormalVector->y = collisions[q].normal.y();
		NormalVector->z = collisions[q].normal.z();

		*NormalDepth = -collisions[q].depth;

        colliderGeom* geom = (colliderGeom*)dGeomGetData(collisions[q].g2);

        if(geom)
        {
            getBumpinessAndFriction(geom->surface().c_str(),
                                    BumpDensity,
                                    BumpHeight,
                                    StaticFriction,
                                    DynamicFriction,
                                    RollCoefficientMult,
                                    NULL);
        }
    }

	return 0;
}

#else
static UINT32 _DeterminePointCollision(FLOAT64 * NormalDepth, // OUT
										FLOAT64VECTOR * NormalVector, // OUT
										piMATRIX const* ContactPointAttMat,
										FLOAT64VECTOR const * ContactPoint,
										FLOAT64VECTOR const * PrevContactPoint,
										FLOAT64 DeltaTime,
										OBJECT ** CollideObjects,
										trnCDTerrainObject ** CollideTOObjects,
										OBJECT ** TerrainCollideObjects,
										UINT32* TerrainCollidePolys,
										UINT32 NumTerrainCollidePolys,
										FLOAT64 CollideLength,
										FLOAT32 * StaticFriction,// OUT
										FLOAT32 * DynamicFriction,// OUT
										FLOAT32 * BumpDensity,// OUT
										FLOAT32 * BumpHeight,// OUT
										FLOAT32 * RollCoefficientMult,// OUT
										UINT32 CollisionsOff,
										mlComponent const* Component,
										mlGearComponentData const* GearData,
										LOGICAL LimitLength,
										mlVehicleData const* VehicleData)
{
	FLOAT64VECTOR TempVec;
	piVECTOR OldPos,NewPos;
	piVECTOR CollidePnt;
	OBJECT * CollideObject;
	SINT32 PolyNum;
	FLOAT64 Variance = 0;
	UINT32 RetVal;
	FASTCOLLIDE CollideList[_MAX_NUM_OF_COLLISIONS];
	FASTCOLLIDE * CollideListPtr;
	UINT32 TempNum;
	SINT32 NumCollides;
	SINT32 CollideNum;
	FLOAT64VECTOR WorldOffsetOrigin;
	piMATRIX TempMat;

	trnGetWorldOriginOffset(&WorldOffsetOrigin);
	WorldOffsetOrigin.z = -WorldOffsetOrigin.z;

	// first compute contact point vector
	TempVec.x = 0;
	TempVec.y = 0;
	TempVec.z = CollideLength;

	if(LimitLength && TempVec.z < _CONTACT_POINT_LENGTH)
		TempVec.z = _CONTACT_POINT_LENGTH;

	mathVectorMultiply64(&TempVec,&TempVec,ContactPointAttMat);
	mathAddVectors(&TempVec,&TempVec,ContactPoint);
	mathSubVectors(&TempVec,&TempVec,&WorldOffsetOrigin);

	OldPos.x = (FLOAT32)TempVec.x * ScaleFactor;
	OldPos.y = (FLOAT32)TempVec.y * ScaleFactor;
	OldPos.z = -(FLOAT32)TempVec.z * ScaleFactor;

	mlgearDetermineHeightVariance(&Variance,ContactPoint,*BumpDensity,*BumpHeight);

	Variance += 0.05;
	TempVec.x = 0;
	TempVec.y = 0;
	TempVec.z = -Variance;
	mathVectorMultiply64(&TempVec,&TempVec,ContactPointAttMat);
	mathAddVectors(&TempVec,&TempVec,ContactPoint);
	mathSubVectors(&TempVec,&TempVec,&WorldOffsetOrigin);

	NewPos.x = (FLOAT32)TempVec.x * ScaleFactor;
	NewPos.y = (FLOAT32)TempVec.y * ScaleFactor;
	NewPos.z = -(FLOAT32)TempVec.z * ScaleFactor;

#ifdef BETA_RELEASE
if(VehicleObjectAnalysis && IsFirstTic && DisplayCollideLines)
{
	piVECTOR TempVec;
  	draw3dLineDebug(&NewPos, &OldPos, colorREDALPHA);
	mathSubVectors(&TempVec,&OldPos,&NewPos);
	TempVec.x *= 0.1f;
	TempVec.y *= 0.1f;
	TempVec.z *= 0.1f;
	mathAddVectors(&TempVec,&TempVec,&NewPos);
	draw3dLineDebug(&TempVec,&NewPos,colorWHITEALPHA);
}
#endif

	// first check object collision
	TempNum = _MAX_NUM_OF_COLLISIONS;
	CollideListPtr = CollideList;

{
static int LastTime;
static int Count;
		trncdCollideWithListOfTerrainObjects1(&CollideListPtr,
											 &TempNum,
											 CollideObjects,
											 CollideTOObjects,
											 ML_NUM_MED_LIST_ELEMENTS,
											 &OldPos,
											 &NewPos,
											 0);
	Count++;

	if (frametime - LastTime > 1000)
	{

		LastTime = frametime;
		NumTris = 0;
		NumObjs = 0;
		Count = 0;
	}
}

	NumCollides = _MAX_NUM_OF_COLLISIONS - TempNum;
	if (NumCollides == 0)
	{
		// if no collision, then check terrain
		RetVal = trncdCollideWithPolyList(
								&OldPos,
								&NewPos,
								&CollidePnt,
								&CollideObject,
								(UINT32*)&PolyNum,
								TerrainCollideObjects,
								TerrainCollidePolys,
								NumTerrainCollidePolys);

		if (RetVal != 0)
		{
			math3dVectorMultiply(&CollidePnt,&CollidePnt,&CollideObject->mat);
		}
	}
	else
	{
		mlUtilitySortCollideList(CollideList,NumCollides);

		CollideNum = 0;
		while (CollideNum < NumCollides && CollideList[CollideNum].Alpha < 0.5)
		{
			CollideNum++;
		}

		if (CollideNum < NumCollides)
		{
			CollideObject = CollideList[CollideNum].CollideObject;
			PolyNum = CollideList[CollideNum].PolyNum;

			CollidePnt.x = (NewPos.x - OldPos.x) * CollideList[CollideNum].CollideTime + OldPos.x;
			CollidePnt.y = (NewPos.y - OldPos.y) * CollideList[CollideNum].CollideTime + OldPos.y;
			CollidePnt.z = (NewPos.z - OldPos.z) * CollideList[CollideNum].CollideTime + OldPos.z;
			RetVal = 1;
		}
		else
		{

			RetVal = trncdCollideWithPolyList(
									&OldPos,
									&NewPos,
									&CollidePnt,
									&CollideObject,
									(UINT32*)&PolyNum,
									TerrainCollideObjects,
									TerrainCollidePolys,
									NumTerrainCollidePolys);

			if (RetVal != 0)
			{
				math3dVectorMultiply(&CollidePnt,&CollidePnt,&CollideObject->mat);
			}
		}
	}

	*StaticFriction = *DynamicFriction = 0.0;

	if (CollisionsOff == 1)
	{
		RetVal = 0;
	}

	vehVehicle* Vehicle = persGetPersonaVehiclePtr();

	if (*RollCoefficientMult > 198.99 && *RollCoefficientMult < 199.01)
	{
		if (!(Vehicle != NULL && Vehicle->VehicleType->Type != VEH_TYPE_GROUND))
		{
			RetVal = 0;
		}

	}

	{
		// sinking hack
		if (Vehicle != NULL && Vehicle->VehicleType->Type == VEH_TYPE_SEA)
		{
			if (GearData->LastCompression > 0.9f)
			{
				RetVal = 0;
			}
		}

	}

	if (!RetVal)
	{
		*NormalDepth = 1.0;
	}
	else
	{
		// compute depth into ground
		TempVec.x = (NewPos.x - CollidePnt.x) / ScaleFactor;
		TempVec.y = (NewPos.y - CollidePnt.y) / ScaleFactor;
		TempVec.z = (NewPos.z - CollidePnt.z) / ScaleFactor;

		*NormalDepth = -mathExactVectorLength(&TempVec);



		// compute normal
		if (CollideObject == NULL)
		{
			NormalVector->x = NormalVector->y = 0;
			NormalVector->z = 1.0;
		}
		else
		{
			NormalVector->x = CollideObject->psnorms[PolyNum].x;
			NormalVector->y = CollideObject->psnorms[PolyNum].y;
			NormalVector->z = CollideObject->psnorms[PolyNum].z;
			TempMat = CollideObject->mat;
			TempMat._41 = TempMat._42 = TempMat._43 = 0.0f;
			mathVectorMultiply64(NormalVector,NormalVector,&TempMat);
			NormalVector->z = -NormalVector->z;
			mlGetObjectBumpinessAndFriction(BumpDensity, // dummy var
										   BumpHeight, // dummy var
										   StaticFriction,
										   DynamicFriction,
										   RollCoefficientMult,
										   NULL,
										   CollideObject,
										   TRUE);
		}

	}
	return 0;
}
#endif

#include "trn/trnvars.h"
UINT32 mlgearDetermineHeightVariance(FLOAT64* HeightVariance,
									 FLOAT64VECTOR const* Pos,
									 FLOAT32 BumpDensity,
									 FLOAT32 BumpHeight)
{
	piVECTOR Vec1,Vec2,Vec3,Vec4;
	FLOAT32 LL,LR,LT,LB; // lengths
	FLOAT32 ILL,ILR,ILT,ILB; // inverse lengths
	FLOAT64 NL,NR,NT,NB; // left, right, top, bottom
	piVECTOR Pos1;

	*HeightVariance = 0.0;

	if (BumpDensity < 0.0001)
	{
		return 0;
	}

	if (BumpHeight < 0.0001)
	{
		return 0;
	}

	Pos1.x = float(Pos->x - WorldOriginOffset.x);
	Pos1.y = float(Pos->y - WorldOriginOffset.y);
	Pos1.x /= BumpDensity;
	Pos1.y /= BumpDensity;

	Vec1.x = floor(Pos1.x);
	Vec1.y = floor(Pos1.y);

	Vec2.x = Vec1.x + 1.0f;
	Vec2.y = Vec1.y;

	Vec3.x = Vec1.x;
	Vec3.y = Vec1.y + 1.0f;

	Vec4.x = Vec1.x + 1.0f;
	Vec4.y = Vec1.y + 1.0f;

    PNRandom::seedrandom((FloatToInt(Vec1.x) << 8) + (FloatToInt(Vec1.y) << 8));
    Vec1.z = PNRandom::random(0.0f, BumpHeight);

    PNRandom::seedrandom((FloatToInt(Vec2.x) << 8) + (FloatToInt(Vec2.y) << 8));
    Vec2.z = PNRandom::random(0.0f, BumpHeight);

    PNRandom::seedrandom((FloatToInt(Vec3.x) << 8) + (FloatToInt(Vec3.y) << 8));
    Vec3.z = PNRandom::random(0.0f, BumpHeight);

    PNRandom::seedrandom((FloatToInt(Vec4.x) << 8) + (FloatToInt(Vec4.y) << 8));
    Vec4.z = PNRandom::random(0.0f, BumpHeight);

	mlInterpolate(&NL,Vec1.z,Vec3.z,Pos1.y - Vec1.y);
	mlInterpolate(&NR,Vec2.z,Vec4.z,Pos1.y - Vec2.y);
	mlInterpolate(&NT,Vec3.z,Vec4.z,Pos1.x - Vec3.x);
	mlInterpolate(&NB,Vec1.z,Vec2.z,Pos1.x - Vec1.x);

	LL = Pos1.x - Vec1.x;
	LR = Vec2.x - Pos1.x;
	LT = Vec3.y - Pos1.y;
	LB = Pos1.y - Vec1.y;

	if (LL == 0)
	{
		*HeightVariance = NL;
		return 0;
	}
	if (LR == 0)
	{
		*HeightVariance = NR;
		return 0;
	}
	if (LT == 0)
	{
		*HeightVariance = NT;
		return 0;
	}
	if (LB == 0)
	{
		*HeightVariance = NB;
		return 0;
	}

	ILL = 1/LL;
	ILR = 1/LR;
	ILT = 1/LT;
	ILB = 1/LB;

	*HeightVariance = (NL * ILL + NR * ILR + NT * ILT + NB * ILB) / (ILL + ILR + ILT + ILB);
	return 0;
}


#if(0)
static UINT32 _GenerateSeededRandom(UINT32* Return,UINT32 Seed)
{
	UINT32 Num = Seed;

	Num *= 1103515245;
	Num += 12345;
	*Return = (UINT32)(Num >> 16) & 2047; // Very fast (n/65536) % 2048

	Num *= 1103515245;
	Num += 12345;
	*Return <<= 11;
	*Return ^= (UINT32)(Num >> 16) & 2047;	// Very fast (n/65536) % 2048

	Num *= 1103515245;
	Num += 12345;
	*Return <<= 10;
	*Return ^= (UINT32)(Num >> 16) & 2047;	// Very fast (n/65536) % 2048

	return 0;
}
#endif
