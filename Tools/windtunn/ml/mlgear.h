#ifndef MLGEAR_H
#define MLGEAR_H

#include "systypes.h"
#include "pi.h"

#include "obmain.h"
#include "trncd.h"

#define ML_GEAR_SUBTYPE_NONE 0
#define ML_GEAR_SUBTYPE_WHEEL 1
#define ML_GEAR_SUBTYPE_SKID 2
#define ML_GEAR_SUBTYPE_TRACK_ROADWHEEL 3
#define ML_GEAR_SUBTYPE_TRACK_DRIVEWHEEL 4
#define ML_GEAR_SUBTYPE_TRANSMISSION 5

#define ML_GEAR_MAX_SUBTYPES 6


#define ML_GEAR_VARNAME_NONE 0
#define ML_GEAR_VARNAME_WHEELRADIUS 1
#define ML_GEAR_VARNAME_STRUTSPRINGSTRENGTH 2
#define ML_GEAR_VARNAME_STRUTLENGTH 3
#define ML_GEAR_VARNAME_ATTACHEDENGINECOMPONENT 4
#define ML_GEAR_VARNAME_CONTACTPOINTTYPE 5
#define ML_GEAR_VARNAME_WHEELROTATIONALINERTIA 6
#define ML_GEAR_VARNAME_STRUTSHOCKSTRENGTH 7
#define ML_GEAR_VARNAME_GEARNUM 8
#define ML_GEAR_VARNAME_CASTOR_ROTINERTIA 9
#define ML_GEAR_VARNAME_DRIVEWHEEL_COMPONENT 10
#define ML_GEAR_VARNAME_BRAKETYPE 11
#define ML_GEAR_VARNAME_BRAKEFORCE 12
#define ML_GEAR_VARNAME_ROLLCOEFFICIENT 13
#define ML_GEAR_VARNAME_TRACKNUM 14
#define ML_GEAR_VARNAME_STRUTSHOCKRETURNSTRENGTH 15
#define ML_GEAR_VARNAME_ATSPEEDROLLCOEFFICIENT 16
#define ML_GEAR_VARNAME_ATSPEEDKPH 17
#define ML_GEAR_VARNAME_COLLIDELENGTH 18

#define ML_GEAR_MAX_VARNAME 19

#define ML_GEAR_CONTACTTYPE_TIRE 1
#define ML_GEAR_CONTACTTYPE_METAL 2
#define ML_GEAR_CONTACTTYPE_WOOD 3

#define ML_GEAR_MAX_CONTACTTYPES 4

#define ML_GEAR_BRAKETYPE_NONE 0
#define ML_GEAR_BRAKETYPE_LEFT 1
#define ML_GEAR_BRAKETYPE_RIGHT 2
#define ML_GEAR_BRAKETYPE_CENTER 3

#define ML_GEAR_MAX_BRAKETYPE 4

#define ML_MAX_GEARNUM 32
#define ML_MAX_TRACKS 4
#define ML_MAX_GEAR_CONTACTPOINTS 1
 
typedef struct tagmlGearComponentData
{
	UINT32 Subtype;
	UINT32 GearNum;
	UINT32 TrackNum; // track number for animation, 0 is no track
	FLOAT64 WheelRadius;
	FLOAT64 StrutSpringStrength;
	FLOAT64 StrutShockCompressStrength;
	FLOAT64 StrutShockReturnStrength;
	FLOAT64 StrutLength;
	UINT32 ContactPointType;
	FLOAT64 WheelRotationalInertia;
	mlComponent * AttachedEngineComponent;

	FLOAT64 WheelRotationSpeed;

	FLOAT64VECTOR ContactPointForce;
	
#if USE_OSG
	tagmlGearComponentData() { memset(this,0x0,sizeof(tagmlGearComponentData)); } // ok for now
	void* ContactPoint;
	void* ShockDOF;
	void* CastorDOF;
#else
	OBJECT *ContactPoint;
	OBJECT *ShockDOF;
	OBJECT *CastorDOF;
#endif
	UINT32 NumContactPoints;
	FLOAT64VECTOR ContactPointCoords;
	FLOAT64VECTOR WorldContactPoint;
	FLOAT64VECTOR PrevWorldContactPoint;
	FLOAT64VECTOR StrutForcePointCoords;
	FLOAT64VECTOR PrevComponentVel;
	FLOAT64VECTOR PrevFrictionForce;

	FLOAT64 GearStrutVel;
	FLOAT64 LastCompression;
	FLOAT64 LastCastorYaw;
	FLOAT64 PrevCastorYawSpeed;
	FLOAT64 CastorRotInertia;

	FLOAT64 LastWheelSpeed;

	mlComponent * DriveWheelComponent;
	FLOAT64 TrackCumulativeFrictionForce; // friction total from all wheels on track including drive
	FLOAT64 LastTrackCumulativeFrictionForce;
	FLOAT64 TrackSpeed; // the speed of the track, in meters of track per second
	FLOAT64 TrackCumulativeRotationalInertia; // track accel/decel resistance, including all wheels

	UINT32 BrakeType;
	FLOAT64 BrakeForce;

	FLOAT64 RollCoefficient; // friction coefficient for rolling
	FLOAT64 AtSpeedRollCoefficient; // friction coefficient for rolling at speed
	FLOAT64 AtSpeedMetersPerSec; // speed for the "at speed" part of "AtSpeedRollCoefficient"
	FLOAT64 LastAxleTorque; // torque wheel exerts on axle, if applicable
	FLOAT64 DriveTorque; // torque coming from drive shaft, if applicable

	FLOAT64VECTOR TempForce; // used for holding temporary force between cycle 1 and 2
	FLOAT64VECTOR TempVel; // used for holding temporary velocity info

	FLOAT64 LastWheelOrientation; // used for flat tire stuff

	FLOAT32 StaticFriction; // static friction of contact point when in contact
	FLOAT32 DynamicFriction; // dynamic friction of contact point when in contact
	FLOAT32 BumpDensity; // current terrain's bump density
	FLOAT32 BumpHeight; // current terrain's maximum bump height
	FLOAT32 RollCoefficientMult; // current terrain's rolling coefficient multiplier

	FLOAT64 LastCompressionForce; // last compression force
	FLOAT64 LastStrutVel; // last strut velocity

	FLOAT64 LastNormalDepth; // last normal depth
	FLOAT64 LastNormalLength;

	FLOAT64 CollideLength;

	FLOAT64VECTOR LastNormal;

	FLOAT64VECTOR LastNormalVel;
	
} mlGearComponentData;


#ifdef __cplusplus
extern "C" {
#endif

extern UINT32 mlgearSetComponentSubtype(void * SpecialData, char * SubType);

extern UINT32 mlgearAddSpecialVariable(void * SpecialData, 
										 char * VarName, 
										 char * VarContents,
										 mlComponentList * ComponentList,
										 mlComponent * Component);

extern UINT32 mlUpdateGearComponent(FLOAT64VECTOR * Force,
							 FLOAT64ATT * Torque,
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
							 OBJECT ** CollideObjects,
							 trnCDTerrainObject ** CollideTOObjects,
							 OBJECT ** TerrainCollideObjects,
							 UINT32 * TerrainCollidePolys,
							 UINT32 NumTerrainCollidePolys,
							 UINT32 CollisionsOff,
							UINT32 PassNum,
							FLOAT64VECTOR const *SavedVehicleForce,
							FLOAT64VECTOR * TempNormForceSum,
							void * InVehicleData);



extern UINT32 mlgearDetermineHeightVariance(FLOAT64 *HeightVariance,
											FLOAT64VECTOR const* Pos,
											FLOAT32 BumpDensity,
											FLOAT32 BumpHeight);

#ifdef __cplusplus
}
#endif

#endif