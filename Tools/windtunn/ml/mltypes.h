#ifndef MLTYPES_H
#define MLTYPES_H

#include "syscfg.h"
#include "systypes.h"
#include "mlCompon.h"
#include "mlgear.h"
#include "obmain.h"
#include "trncd.h"
#include "trncd.h"
#include "updtTypes.h"
#include "pi.h"

#define ML_MAX_TRACK_VERTICES 32

#define ML_MAX_LODS 8

#define ML_MAX_DAMAGE_GROUPINGS 8
#define ML_MAX_DAMAGE_GROUP_DOFS 16

#define ML_MAX_ENGINES 8 // maximum number of engines

#define ML_TOWTYPE_LIGHT 1
#define ML_TOWTYPE_MEDIUM 2
#define ML_TOWTYPE_HEAVY 3

#define ML_MAX_FPS_SEATS 16
#define ML_MAX_VEH_SEATS 16

#define ML_MAX_GEARUPDOWN_GROUPS 8
#define ML_MAX_FLAPSUPDOWN_GROUPS 8
#define ML_MAX_ENGINEFASTSLOW_GROUPS 8

#define ML_MAX_CREW_POSITIONS 	16
#define VEH_MAX_MOUNTS 			16
#define ML_MAX_WEAPON_GROUPINGS	4 // used for selector secondary weapon groupings
#define ML_MAX_TURRETS 			8

#define ML_MAX_RADIATORS_PER_ENGINE 4
#define ML_MAX_FUEL_TANKS_PER_ENGINE 8
#define ML_MAX_FUELTANKS 8

#define ML_MED_TO_COLLIDE_DIST              2000.0 * ScaleFactor
#define ML_MED_TO_COLLIDE_DIST_DESTROYER    6000.0 * ScaleFactor
#define ML_SHORT_TO_COLLIDE_DIST            250.0 * ScaleFactor
#define ML_MED_FRONT_CONE_ANGLE             (mlDEG_TO_RAD(60.0))
#define ML_NUM_MED_LIST_ELEMENTS            2048 // was 512
#define ML_NUM_SHORT_LIST_ELEMENTS          128 // was 64
#define ML_NUM_MED_FRONT_CONE_LIST_ELEMENTS 32
#define ML_WATER_DENSITY                    1000.0
#define mlGRAVITY                           9.8 // 9.8m/s/s is earth's gravitational acceleration

#define MAX_SECONDARY_GROUPS 6

// arbitrary cut off for use with things constantly apply damage over time
#define MAX_DAMAGE 1000000000.0

// physics versions
#define HOOF_PHYSICS 0
#define TORQUE_FIX_PHYSICS 1
#define NEW_PHYSICS 100

struct tagvehVehicle;
struct mlAirfoilList;
struct mlEngineList;

typedef struct tagmlAnimation
{
	FLOAT32 LastPropPosition[ML_MAX_ENGINES]; // from 0 to 1
	LOGICAL PropFailed[ML_MAX_ENGINES];
	FLOAT32 LastFlapPosition;

	FLOAT32 LastGearPosition;

	FLOAT32 GearHandleMoveRate;
	FLOAT32 CowlFlapMoveRate[ML_MAX_ENGINES];
	FLOAT32 FlapsSettingMoveRate;

	FLOAT32 LastWheelPosition[ML_MAX_GEARNUM];

	UINT32 LastAnimationTime;

} mlAnimation;

typedef struct tagmlPositionData
{
	FLOAT32 CanopyClosedSoundVol[ML_MAX_CREW_POSITIONS];
	FLOAT32 CanopyOpenSoundVol[ML_MAX_CREW_POSITIONS];
	FLOAT32 HatchClosedSoundVol[ML_MAX_CREW_POSITIONS];
	FLOAT32 HatchOpenSoundVol[ML_MAX_CREW_POSITIONS];
	FLOAT32 EngineIdleVol[ML_MAX_CREW_POSITIONS];
	FLOAT32 EngineFullPowerVol[ML_MAX_CREW_POSITIONS];

	SINT8 CanopySound[ML_MAX_CREW_POSITIONS][32];
	SINT8 HatchSound[ML_MAX_CREW_POSITIONS][32];
	SINT8 CanopyCloseSound[ML_MAX_CREW_POSITIONS][32];
	SINT8 HatchCloseSound[ML_MAX_CREW_POSITIONS][32];

	UINT32 NumberOfSecondaryWeapons[ML_MAX_CREW_POSITIONS]; // number of secondary weapon groups available, minus 1
	SINT8 SecondaryWeaponMsg[ML_MAX_CREW_POSITIONS][ML_MAX_WEAPON_GROUPINGS][32]; // what message to display when selecting this weapon group

	LOGICAL DefaultCanopyPosition[ML_MAX_CREW_POSITIONS];
	LOGICAL DefaultHatchPosition[ML_MAX_CREW_POSITIONS];
	LOGICAL ExposedByCanopy[ML_MAX_CREW_POSITIONS];
	LOGICAL ExposedByHatch[ML_MAX_CREW_POSITIONS];
	LOGICAL AlwaysExposed[ML_MAX_CREW_POSITIONS];
	
	LOGICAL SetBombbayDoor[ML_MAX_CREW_POSITIONS];
	LOGICAL BombbayDoorState[ML_MAX_CREW_POSITIONS];

} mlPositionData;

struct ptkBody;

typedef struct mlBuoyancyData
{
	struct mlBuoyancyData* next;
	FLOAT64 x,y;
	FLOAT64 top, bottom;
	FLOAT64 width, len;
} mlBuoyancyData;

struct fpPersonData;

typedef struct tagmlVehicleData
{
	UCHAR 	VehicleName[80];
	UCHAR 	VehicleObjectName[80];

	LOGICAL	PhysicsVersion;
	
	struct tagvehVehicle* 	VehicleMasterPtr;
	struct mlObjectData*	VehicleObjectData;
	mlComponentList*		ComponentList;
	struct ptkBody*			Body;
	struct mlBuoyancyData*	BuoyancyData;
	struct mlEngineList*	EngineList;
	struct mlAirfoilList*	AirfoilList;
	mlPositionData 			CrewData; 
	mlAnimation 			AnimationData;
	fpPersonData*			PersonData; 
	void* 					BombSightAmmoType;

	UINT32 	PersonaVehicle;
	UINT32 	MultiCrewPos;
	
	UINT32 	HasSentUpdate;
	UINT32	NumTics;

// Linear
	FLOAT64VECTOR 	AdjustedForCGPos;
	FLOAT64VECTOR	VehiclePos;
	FLOAT64VECTOR 	VehicleLastPos;
	FLOAT64VECTOR 	VehicleVel;
	FLOAT64VECTOR 	VehicleLastVel;
	FLOAT64VECTOR 	VehicleRealVel; 	// use this for any non-flightmodel velocity needs
	FLOAT64VECTOR 	VehicleAcc;
	FLOAT64VECTOR 	LastVehicleAcc;

// Rotational
	FLOAT64ATT 		VehicleAtt;
	piMATRIX 		VehicleAttMat; 		// attitude in matrix format
	piMATRIX 		VehicleAttInvMat; 	// inverse of attitude matrix
	FLOAT64ATT 		VehicleRealAtt; 	// use this for any non-flightmodel attitude needs
	FLOAT64ATT 		VehicleAttVel;
	FLOAT64ATT 		VehicleAttAcc;
	FLOAT64ATT 		VehicleRealAttVel;	// use this for any non-flightmodel attitude needs
	
// Force
	FLOAT64VECTOR 	VehicleForce;
	FLOAT64ATT 		VehicleTorque;

// Impulse
	FLOAT64VECTOR	VehicleLinearImpulse;
	FLOAT64VECTOR	VehicleAngularImpulse;
		
// Previous Values
	FLOAT64VECTOR	LastVehiclePos;
	FLOAT64VECTOR 	LastVehicleVel;
	FLOAT64ATT 		LastVehicleAtt;
	FLOAT64ATT 		LastVehicleAttAcc;
	FLOAT64ATT 		LastVehicleAttVel;
	FLOAT64VECTOR 	LastVehicleForce;
	FLOAT64ATT 		LastVehicleTorque;
	FLOAT64 		LastYaw;
	SINT32 			LastPlayerPos; 	// last cycle player position number, used to determine if the player moved
	FLOAT64VECTOR 	SavedAcc;

// Render
	FLOAT64VECTOR 	RenderPos; 				// these are where the player is *drawn*, do not use for calcs!
	FLOAT64ATT		RenderAtt; 				//
	
// Physical Properties
	FLOAT64 		VehicleMass;
	FLOAT64VECTOR 	VehicleCenterOfMass;
	FLOAT64ATT 		VehicleRotationalInertia;

	FLOAT64VECTOR 	LastVehicleCenterOfMass;
	FLOAT64ATT 		LastVehicleRotationalInertia;

// Times
	SINT32 		PrevVehicleUpdateTime;
	SINT32 		LastVehicleUpdateTime;
	SINT32 		LastCollisionTime;
	SINT32 		SpawnTime;
	UINT32		LastResetTime;
	LOGICAL 	GetStartTime;
	
// Control Surfaces
	FLOAT64 AileronMaxSpeed;
	FLOAT64 RudderMaxSpeed;
	FLOAT64 ElevatorMaxSpeed;

	FLOAT64 AileronCompressSpeed;
	FLOAT64 RudderCompressSpeed;
	FLOAT64 ElevatorCompressSpeed;

	FLOAT64 AileronTabRatio; 	// ratio of tab torque to control surface torque
	FLOAT64 ElevatorTabRatio;
	FLOAT64 RudderTabRatio;

	FLOAT64 AileronTabStatic; 	// static tab position, adds to user tab position
	FLOAT64 ElevatorTabStatic;
	FLOAT64 RudderTabStatic;

	FLOAT64 AileronSpringTabRatio; 	// opposing tab movement ratio, same as reg tabs
	FLOAT64 ElevatorSpringTabRatio;	// note that spring tabs extend based on stick force
	FLOAT64 RudderSpringTabRatio;

	FLOAT64 AileronTabPosition; 	// -1.0 to 1.0
	FLOAT64 ElevatorTabPosition; 	// -1.0 to 1.0
	FLOAT64 RudderTabPosition; 		// -1.0 to 1.0

	FLOAT64 AileronResponseTime; 	// time for full deflection at aileronmaxspeed
	FLOAT64 ElevatorResponseTime; 	// time for full deflection at elevatormaxspeed
	FLOAT64 RudderResponseTime; 	// time for full deflection at ruddermaxspeed

	FLOAT64 ElevatorPos; 	// -1.0 to 1.0
	FLOAT64 RudderPos; 		// -1.0 to 1.0
	FLOAT64 AileronPos; 	// -1.0 to 1.0
	FLOAT64 ThrottlePos;

	FLOAT64 ElevatorBasePos;	// position unmodified by tabs
	FLOAT64 RudderBasePos;
	FLOAT64 AileronBasePos;

// Engine
	FLOAT64 ThrottleResponseTime; 	// time for full deflection of throttle
	FLOAT64 ClutchPos; 				// position of the clutch
	SINT32 	GearShift; 				// gear shift flag -1 = shift down, 1 = shift up, 0 = no shift
	SINT32 	ReverseToggle; 			// reverse toggle, 1 = toggle reverse state, 0 = no toggle

	FLOAT64	CowlFlapSetting[ML_MAX_ENGINES];
	UINT32 	StarterOn[ML_MAX_ENGINES];
	UINT32 	StartersOn;
	UINT32 	ShutdownEngines;
	UINT32 	MasterStarterOn;

// Flaps
	FLOAT64	FlapDesiredSetting;
	FLOAT64	FlapSetting;
	FLOAT64	LastFlapSetting;
	FLOAT64	FlapPosition;
	UINT32 	NumberOfFlapPositions;
	UINT32 	FlapsAreAnalog;
	FLOAT64 FlapDeploySpeed;
	LOGICAL FlapsJammed;
	LOGICAL FlapsBeingDamaged;

// Gear
	FLOAT64	GearSetting;
	FLOAT64 LastGearSetting;
	FLOAT64 GearPosition;
	FLOAT64 GearDeploySpeed;
	FLOAT64 GearMaxOpenSpeed;
	FLOAT64 GearMaxCloseSpeed;
	LOGICAL GearJammed;
	LOGICAL GearBeingDamaged;
	LOGICAL LastGearJammed;
	
// Canopy
	UINT32 	CanopyOpen[ML_MAX_CREW_POSITIONS];
	FLOAT64 CanopyPosition[ML_MAX_CREW_POSITIONS];
	FLOAT64 CanopyDeploySpeed;
	FLOAT64 CanopyMaxOpenSpeed[ML_MAX_CREW_POSITIONS];
	FLOAT64 CanopyMaxCloseSpeed[ML_MAX_CREW_POSITIONS];
	LOGICAL CanopyJammed[ML_MAX_CREW_POSITIONS];
	
// Hatch
	UINT32 	HatchOpen[ML_MAX_CREW_POSITIONS];
	FLOAT64 HatchPosition[ML_MAX_CREW_POSITIONS];
	FLOAT64 HatchDeploySpeed;

// Toggle States
	UINT32 LandingLightOn;
	UINT32 PanelLightOn;
	UINT32 AuxSightOn;
	UINT32 PrimaryTriggerDown;
	UINT32 SecondaryTriggerDown;
	UINT32 OrdnanceTriggerDown;
	UINT32 OrdnanceSelected;
	UINT32 DiveBrakeOn;
	UINT32 AutoDiveOn;
	UINT32 JettisonTriggerDown;
	SINT32 PicklePrimary;
	SINT32 PickleSecondary;

	FLOAT64 BrakeForceLeft;
	FLOAT64 BrakeForceRight;
	FLOAT64 BrakeForceCenter;

	FLOAT64 ATGunMovementLeftRight;
	FLOAT64 ATGunMovementForwardsBackwards;

	FLOAT64 Steering;

	FLOAT64 LeftTrackClutch;
	FLOAT64 RightTrackClutch;

	UINT32 FailsafeMode;
	UINT32 ViewMode;
	UINT32 TailWheelLocked;
	SINT32 PropPitchAdjust;
	SINT32 StukaSirenOn;
	UINT32 ToggleWepMode;

	SINT32 	BombBayDoor;
	FLOAT64 BombBayMaxOpenSpeed;
	FLOAT64 BombBayMaxCloseSpeed;
	LOGICAL BombBayammed;

	LOGICAL	TowBarEnabled;

	UINT32 	PrimaryGunIsAutomatic;
	UINT32 	SecondaryGunIsAutomatic;

	UINT32 	PrimaryIsFiring;
	UINT32 	SecondaryIsFiring;

// Turret
	FLOAT64 TurretTraverseRate[ML_MAX_CREW_POSITIONS];
	FLOAT64 TurretElevateRate[ML_MAX_CREW_POSITIONS];
	FLOAT64 TurretTraverse[ML_MAX_CREW_POSITIONS];
	FLOAT64 TurretElevate[ML_MAX_CREW_POSITIONS];
	FLOAT64	TurretTraverseWithStops[ML_MAX_CREW_POSITIONS];
	FLOAT64 LastTurretTraverse[ML_MAX_CREW_POSITIONS];
	FLOAT64 LastTurretElevate[ML_MAX_CREW_POSITIONS];
	FLOAT64	LastTurretTraverseWithStops[ML_MAX_CREW_POSITIONS];
	SINT32  HoistTurretNum;
	FLOAT64 LastHoistRate;
	UINT32 	TurretNotFirstTime;
	LOGICAL TurretElevateDeltaInput[ML_MAX_CREW_POSITIONS];
	LOGICAL TurretTraverseDeltaInput[ML_MAX_CREW_POSITIONS];
	FLOAT64 WantedTurretElevate[ML_MAX_CREW_POSITIONS];
	FLOAT64 WantedTurretTraverse[ML_MAX_CREW_POSITIONS];

// Damage
	UINT32 	NewDamage;
	FLOAT32 DamageAccrued[VEH_MAX_DAMAGE_OBJECTS];
	UINT32 	NumRoundsHit[VEH_MAX_DAMAGE_OBJECTS];
	FLOAT32 DamageCurrent[ML_MAX_DAMAGE_GROUPINGS];
	FLOAT32 DamagePossible[ML_MAX_DAMAGE_GROUPINGS];

// Kill
	LOGICAL KillCrew; 		// go ahead and kill the crew?
	LOGICAL LastKillCrew; 	// go ahead and kill the crew last frame?
	LOGICAL VehicleIsDead;
	LOGICAL LastVehicleIsDead;
	UINT32 SecondaryCriticalComponentCount[MAX_SECONDARY_GROUPS];
	UINT32 TotalSecondaryCriticalComponentCount[MAX_SECONDARY_GROUPS];

// Sounds
	SCHAR StallSound[64];
	SCHAR GearSound[64];
	SCHAR FlapsSound[64];
	SCHAR CanopySound[64];
	SCHAR PrimaryGunSound[64];
	SCHAR SecondaryGunSound[64];
	SCHAR LeftSlatSound[64];
	SCHAR RightSlatSound[64];
	SCHAR WindSound[64];
	SCHAR HatchSound[64];
	SCHAR GearJamSound[64];
	SCHAR FlapJamSound[64];
	SCHAR CanopyJamSound[64];
	
	UINT32 	StallSoundStarted;
	SINT32 	StallSoundId;
	FLOAT64 StallSoundVolume;
	SINT32 	WindSoundStarted;
	SINT32 	WindSoundId;
	SINT32 	PrimaryGunSoundId;
	SINT32 	SecondaryGunSoundId;
	SINT32	FlapDamageSoundId;
	FLOAT32 FlapDamageSoundVolume;
	SINT32  GearDamageSoundId;
	FLOAT32 GearDamageSoundVolume;
    SINT32  gearSoundId;
    SINT32  flapSoundId;
    SINT32  bombbayDoorSoundId;
    SINT32  gearJamSoundId;
    SINT32  canopySoundId[ML_MAX_CREW_POSITIONS];

// Collision
	trnCDTerrainObject* 	MedTrnList[ML_NUM_MED_LIST_ELEMENTS+1]; // anything within 2km of vehicle
	trnCDTerrainObject* 	ShortTrnList[ML_NUM_SHORT_LIST_ELEMENTS+1]; // anything within 250 meters of vehicle
	trnCDTerrainObject * 	MedFront60DegList[ML_NUM_MED_FRONT_CONE_LIST_ELEMENTS+1]; // anything within 2km and front 60deg cone
	trnCDTerrainObject* 	TrnCDListTOs[ML_NUM_MED_LIST_ELEMENTS+1];
	FLOAT32 				ShortTrnListDist[ML_NUM_SHORT_LIST_ELEMENTS+1]; // distances for short list

	UINT32 		CollisionsOff;
	SINT32		 CollisionSide; // used by colliders to facilitate friendly collisions
	OBJECT* 	TrnCDListObject[ML_NUM_MED_LIST_ELEMENTS+1];
	OBJECT* 	TerrainCollideObjectList[8];
	UINT32 		TerrainCollidePolyList[8];
	UINT32 		NumTerrainCollides;
	OBJECT* 	TerrainAGLObjectList[8];
	UINT32 		TerrainAGLPolyList[8];
	UINT32 		NumTerrainAGL;

	UINT32 NumCageCollideLines;
	piVECTOR * CageLineV1;
	piVECTOR * CageLineV2;
	OBJECT ** CageLineObject;

// Riding
	SINT32 	LastUnRideTime;
	UINT32	LastRideVehicle;

	LOGICAL	EjectRiders;
	SINT32 	EjectionStartTime;

// Weap
	UINT32 	CycleAmmo;
	SINT32 	GroupAmmoCount;
	SINT32	MaxGroupAmmoCount;
	LOGICAL	MountIsFiring[VEH_MAX_MOUNTS];
	UINT32 	FireMask; // used by multicrew vehicles
	UINT32 	NumberOfWeaponMounts;
	UINT32 	CurrentPrimaryAmmoCount;
	UINT32 	CurrentSecondaryAmmoCount;
	char	PrimaryAmmoType[32];
	char	SecondaryAmmoType[32];
	UINT32	PrimaryClipCount;
	UINT32	SecondaryClipCount;
	SINT32	PrimaryClipType;
	SINT32	SecondaryClipType;

// Misc
	FLOAT32	VehicleG;
	UINT32 	AccCount;
	LOGICAL FreezeVehicle;
	LOGICAL ShipSinking;
	LOGICAL	UseKludgeStabilizer;
	LOGICAL HasFireDirector;
	LOGICAL PhysicsOff;
	
    SINT32 axisType;

	UINT32  HoistVehicleHostIndex;
	LOGICAL HoistCleanCollideState;
	LOGICAL AutopilotOn;

	LOGICAL	DeclaredKilled;
	mlComponent	*componentDeath;
	UINT32	KillerVehicleID;
	
	FULL_OBJ_ID	closestCapTO;
	FULL_OBJ_ID	closestCp;

#if FRES
	SINT32 PTZZoomLevel;
#endif

#if USE_OSG
	tagmlVehicleData();
	~tagmlVehicleData();
#endif

} mlVehicleData, *mlVehicleRef;

#ifdef __cplusplus
extern "C" {
#endif

extern UINT32 mlInit(void);
extern UINT32 mlUpdate(mlVehicleData * VehicleData,OBJECT * VehicleObject);
extern UINT32 mlKill(void);

extern UINT32 mlInitVehicleData(mlVehicleData * VehicleData);
extern UINT32 mlKillVehicleData(mlVehicleData * VehicleData);
extern UINT32 mlReadVehicleFile(mlVehicleData * VehicleData, 
								 UCHAR *FileName, 
								 UCHAR *DirName,
								 UINT32 VehicleNum);

extern UINT32 mlSetVehicleComponentsRelativePos(mlVehicleData *VehicleData);
extern UINT32 mlCalcVehicleMass(mlVehicleData *VehicleData);
extern UINT32 mlCalcVehicleCenterOfMass(mlVehicleData *_VehicleData);
extern UINT32 mlCalcVehicleRotationalInertia(mlVehicleData *VehicleData);

extern UINT32 mlApplyStaticForceAtRelativePoint(mlVehicleData * VehicleData,
											     FLOAT64VECTOR * RelPoint,
											     FLOAT64VECTOR * ForceVec);

extern UINT32 mlApplyTorqueAtRelativePoint(mlVehicleData * VehicleData,
											FLOAT64VECTOR * RelPoint,
											FLOAT64ATT * Torque,
											FLOAT64ATT * TorqueAtt, LOGICAL UseCorrectPhysics);

// returns velocity relative to the vehicle
extern UINT32 mlCompCalcComponentVelocityVector(FLOAT64VECTOR *ReturnVel,
												 mlComponent * Component, 
												 mlVehicleData * VehicleData);


extern UINT32 mlGetElevatorDeflection(FLOAT64 * ReturnVal,FLOAT64 DeltaTime,mlVehicleData * VehicleData);
extern UINT32 mlGetRudderDeflection(FLOAT64 * ReturnVal,FLOAT64 DeltaTime,mlVehicleData * VehicleData);
extern UINT32 mlGetAileronDeflection(FLOAT64 * ReturnVal,FLOAT64 DeltaTime,mlVehicleData * VehicleData);
extern UINT32 mlGetThrottlePosition(FLOAT64 * ReturnVal,FLOAT64 DeltaTime,mlVehicleData * VehicleData);
extern UINT32 mlGetFlapPosition(FLOAT64 * ReturnVal,mlVehicleData * VehicleData);

extern UINT32 mlUpdateVehicle(mlVehicleData *VehicleData);

// I/O stuff
extern UINT32 mlAdjustElevTrim(SINT32 Param,UINT32 Id,UINT32 Detach);
extern UINT32 mlAdjustRudderTrim(SINT32 Param,UINT32 Id,UINT32 Detach);
extern UINT32 mlAdjustAileronTrim(SINT32 Param,UINT32 Id,UINT32 Detach);
extern UINT32 mlAdjustStickX(SINT32 Param,UINT32 Id,UINT32 Detach);
extern UINT32 mlAdjustStickY(SINT32 Param,UINT32 Id,UINT32 Detach);
extern UINT32 mlAdjustStickZ(SINT32 Param,UINT32 Id,UINT32 Detach);
extern UINT32 mlAdjustThrottle(SINT32 Param,UINT32 Id,UINT32 Detach);
extern UINT32 mlAdjustGas(SINT32 Param,UINT32 Id,UINT32 Detach);
extern UINT32 mlToggleDiveBrakes(SINT32 Param, UINT32 Id, UINT32 Detach);
extern UINT32 mlToggleStukaSiren(SINT32 Param, UINT32 Id, UINT32 Detach);
extern UINT32 mlToggleCanopy(SINT32 Param,UINT32 Id,UINT32 Detach);
extern UINT32 mlToggleHatch(SINT32 Param,UINT32 Id,UINT32 Detach);
extern UINT32 mlToggleAuxSight(SINT32 Param,UINT32 Id,UINT32 Detach);
extern UINT32 mlTogglePanelLight(SINT32 Param,UINT32 Id,UINT32 Detach);
extern UINT32 mlToggleLandingLight(SINT32 Param,UINT32 Id,UINT32 Detach);
extern UINT32 mlCenterBrakeFunction(SINT32 Param, UINT32 Id, UINT32 Detach);
extern UINT32 mlSteeringFunction(SINT32 Param, UINT32 Id, UINT32 Detach);
extern UINT32 mlStarterIsDepressed(SINT32 Param, UINT32 Id, UINT32 Detach);
extern UINT32 mlStarterIsNotDepressed(SINT32 Param, UINT32 Id, UINT32 Detach);
extern UINT32 mlShiftUp(SINT32 Param,UINT32 Id,UINT32 Detach);
extern UINT32 mlShiftDown(SINT32 Param,UINT32 Id,UINT32 Detach);
extern UINT32 mlReverse(SINT32 Param,UINT32 Id,UINT32 Detach);
extern UINT32 mlAdjustClutch(SINT32 Param, UINT32 Id, UINT32 Detach);
extern UINT32 mlLeftTrackClutchFunction(SINT32 Param,UINT32 Id, UINT32 Detach);
extern UINT32 mlRightTrackClutchFunction(SINT32 Param,UINT32 Id, UINT32 Detach);
extern UINT32 mlToggleWepMode(SINT32 Param,UINT32 Id,UINT32 Detach);
extern UINT32 mlRangeFind(SINT32 Param,UINT32 Id,UINT32 Detach);

extern UINT32 mlSetStarterOff(UINT32 EngineNumber);

extern UINT32 mlATGunLeftRight(SINT32 Param,UINT32 Id, UINT32 Detach);
extern UINT32 mlATGunForwardBackwards(SINT32 Param,UINT32 Id, UINT32 Detach);

extern UINT32 mlToggleGear(SINT32 Param,UINT32 Id,UINT32 Detach);
extern UINT32 mlToggleCowlFlaps(SINT32 Param,UINT32 Id,UINT32 Detach);
extern UINT32 mlAdjustFlapPosition(SINT32 Param,UINT32 Id,UINT32 Detach);
extern UINT32 mlMoveFlapSetting(SINT32 Param,UINT32 Id, UINT32 Detach);
extern UINT32 mlMixtureRich(SINT32 Param,UINT32 Id,UINT32 Detach);
extern UINT32 mlMixtureLean(SINT32 Param,UINT32 Id,UINT32 Detach);
extern UINT32 mlMixtureOff(SINT32 Param,UINT32 Id,UINT32 Detach);
extern UINT32 mlCycleFuelCock(SINT32 Param,UINT32 Id,UINT32 Detach);
extern UINT32 mlRpmStubFunction(SINT32 Param,UINT32 Id,UINT32 Detach);
extern UINT32 mlPrimaryTriggerDown(SINT32 Param,UINT32 Id,UINT32 Detach);
extern UINT32 mlSecondaryTriggerDown(SINT32 Param,UINT32 Id,UINT32 Detach);
extern UINT32 mlJettisonOrdnance(SINT32 Param,UINT32 Id,UINT32 Detach);
extern UINT32 mlPropPitchAdjust(SINT32 Param, UINT32 Id, UINT32 Detach);
extern UINT32 mlToggleBombBayDoor(SINT32 Param,UINT32 Id,UINT32 Detach);
extern UINT32 mlToggleMobileSpawn(SINT32 Param, UINT32 Id, UINT32 Detach);

extern UINT32 mlTraverseTurretSpeed(SINT32 Param, UINT32 Id, UINT32 Detach);
extern UINT32 mlElevateTurretSpeed(SINT32 Param, UINT32 Id, UINT32 Detach);



extern UINT32 mlGetStarterOnInfo(mlVehicleData*, UINT32 * StarterOn,UINT32 EngineNumber);

extern UINT32 mlResetControlStuff(void);

extern UINT32 mlResetVehiclePos(mlVehicleData * VehicleData);
extern UINT32 mlLeftBrakeFunction(SINT32 Param, UINT32 Id,UINT32 Detach);
extern UINT32 mlRightBrakeFunction(SINT32 Param, UINT32 Id,UINT32 Detach);

extern UINT32 mlToggleTailWheelLock(SINT32 Param, UINT32 Id, UINT32 Detach);

extern UINT32 mlIsTailWheelLocked(void);

extern UINT32 mlGetRandomHitSound(SINT8 ** HitSoundName,
								   FLOAT32 * MinVolume,
								   FLOAT32 * MaxVolumeEnergy,
								   const SINT8 * TextureName);
extern UINT32 mlGetObjectBumpinessAndFriction(FLOAT32 * BumpDensity,
											   FLOAT32 * BumpHeight,
											   FLOAT32 * StaticFriction,
											   FLOAT32 * DynamicFriction,
											   FLOAT32 * RollCoefficientMult,
											   SINT32 * InfSurfaceID,
											   OBJECT * Object);
extern UINT32 mlGetHitSpriteNumbers(SINT32 *Sprite1,SINT32 *Sprite2,SINT32 *Sprite3, const SINT8 * TextureName);
extern UINT32 mlGetGroundClutterStuff(SINT32 *ClutterType1,
									   SINT32 *ClutterType2,
									   SINT32 *ClutterType3,
									   FLOAT32 *ClutterWidth,
									   FLOAT32 *ClutterHeight,
									   SINT32* ClutterDensity,
									   const SINT8 * TextureName);

extern UINT32 mlGetRadialClutterStuff(SINT32 iClutter[], const SCHAR szTextureName[]);

extern UINT32 mlUpdateComponent(mlComponent * Component,
								mlVehicleData * VehicleData,
								FLOAT64 DeltaTime,
								UINT32 PassNum,
								FLOAT64VECTOR *SavedVehicleForce,
								FLOAT64VECTOR *TempNormForceSum);

extern UINT32 mlSetSpawnStuff(SINT32 SpawnCellX,SINT32 SpawnCellY,SINT32 SpawnId,UINT32 SpawnNum);
extern UINT32 mlSetMobileSpawnStuff(UINT32 VehicleHostIndex, FLOAT64 x,FLOAT64 y,FLOAT64 z);

extern UINT32 mlToggleTowbar(SINT32 Param,UINT32 Id,UINT32 Detach);

extern  UINT32 mlCheckSpecificObjectForLineCollision1(mlVehicleData * VehicleData,
												   OBJECT * Object,
												   OBJECT * MasterObject,
												   FLOAT64VECTOR *ReturnForce,
												   FLOAT64VECTOR *ReturnForceArm,
												   FLOAT64VECTOR *VehicleForce,
												   FLOAT64VECTOR *NormalForceSum,
												   FLOAT64VECTOR *ReturnRatioForce,
												   FLOAT64VECTOR *LastContactPnt,
												   piVECTOR * PntForce,
												   piVECTOR * PntVel,
													FLOAT32 * DynamicFriction,
													FLOAT32 * BumpDensity,
													FLOAT32 * BumpHeight,
													FLOAT64 * ShockPos,
													FLOAT64 * ShockVel,
													FLOAT64VECTOR * InForceDir,
													FLOAT64 * InLength,
												   UINT32 PassNum,
												   FLOAT64 DeltaTime,
												   UINT32 IsWheel,
												   FLOAT32* RollFrictionCo,
												   LOGICAL IsSpike);

extern UINT32 mlCycleAmmo(SINT32 Param,UINT32 Id,UINT32 Detach);

extern LOGICAL mlIsFPSOnWater(mlVehicleData * VehicleData, LOGICAL *onWater);

extern UINT32 mlCheckForCollisionListUpdate(void);
extern UINT32 mlCheckRemovedObjectOnCollisionList(trnCDTerrainObject * TO);

extern LOGICAL mlGetPrimaryPickleSetting(SINT32 * Pickle);
extern LOGICAL mlGetSecondaryPickleSetting(SINT32 * Pickle);
extern LOGICAL mlResetPrimaryPickleSetting(void);
extern LOGICAL mlResetSecondaryPickleSetting(void);
extern LOGICAL mlAddToGroupAmmoCount(SINT32 Count,SINT32 Total);

#ifdef VEHICLE_OBJECT_ANALYSIS
extern UINT32 _AdjustVelAndRotVel(FLOAT64VECTOR *Vel,
							   FLOAT64ATT *AttVel,
							   FLOAT64VECTOR *Acc,
							   FLOAT64ATT *AttAcc,
							   FLOAT64VECTOR *Force,
							   FLOAT64ATT *Torque,
							   FLOAT64 Mass,
							   FLOAT64ATT *RotInertia,
							   FLOAT64 DeltaTime);

extern UINT32 _AdjustPosAndAccel(FLOAT64VECTOR *Pos,
								  FLOAT64ATT *Att,
								  FLOAT64VECTOR *Vel,
								  FLOAT64VECTOR *Acc,
								  FLOAT64VECTOR *LastAcc,
								  FLOAT64VECTOR *LastVel,
								  FLOAT64ATT *AttVel,
								  FLOAT64ATT *AttAcc,
								  FLOAT64ATT *LastAttAcc,
								  FLOAT64 DeltaTime);
#endif

#ifdef TERRAIN_EDITOR
LOGICAL mlCountColliderStats(UINT32 *TwoKmTLOG, UINT32 *FiftyMeterTLOG, UINT32 *NumTrisChecked);
#endif

void mlCalcObjectRotationalInertia(OBJECT* obj, FLOAT64 mass, FLOAT64 I[3][3], FLOAT64VECTOR* cm, FLOAT64* vol);


// mlReadout
LOGICAL mlSetTorpedoReadout(FLOAT32 relBearing, FLOAT32 angleOff, FLOAT32 range, FLOAT32 speed, FLOAT32 depth, 
							FLOAT32 fireAngle, FLOAT32 leadAngle, FLOAT32 travelTime, LOGICAL ConvertToEnglishUnits);
extern SINT32 mlSetNormalReadout(void);
extern SINT32 mlSetRangeReadout(FLOAT32 Range);
extern SINT32 mlSetBombsightReadoutEnglish(FLOAT32 Alt, FLOAT32 TAS);
extern SINT32 mlSetBombsightReadoutMKS(FLOAT32 Alt, FLOAT32 TAS);

SINT32 mlVehicleAddImpulse(mlVehicleRef, FLOAT64VECTOR const* Pos, FLOAT64VECTOR const* Impulse);

UINT32 mlToggleHoist(SINT32 Param,UINT32 Id,UINT32 Detach);
UINT32 mlAdjustWinch(SINT32 Param,UINT32 Id,UINT32 Detach);
UINT32 mlRideTransport(SINT32 Param,UINT32 Id,UINT32 Detach);

SINT32 mlVehicleToggleHoist(mlVehicleRef);
SINT32 mlVehicleRideTransport(mlVehicleRef);

void mlRemoveComponent(mlVehicleData*, mlComponent*, LOGICAL ParentRemoved);

UINT32 mlDetermineInfantryCollisions(mlVehicleData	*VehicleData,
											FLOAT64VECTOR	*Vel,
											UINT32			poseToCheck,
											 UINT32			vehicleCollideCheck);
UINT32 mlInfantryUpdate(mlVehicleData* VehicleData, SINT32 TicTime);
SINT32 mlInfantryInsideVehicleCheck (mlVehicleData* pVehicleData, piVECTOR *eyePoint);

SINT32 mlVehicleUpdate(mlVehicleRef vehRef);

struct ptkQuaternion;
SINT32 mlQuaternionToVehAttMat(struct ptkQuaternion const*, FLOAT64ATT*,piMATRIX*);
SINT32 mlVehAttToQuaternion(struct ptkQuaternion*, FLOAT64ATT const*);
SINT32 mlObjMatToQuaternion(struct ptkQuaternion*, piMATRIX const*);

UINT32 mlAutopilot(SINT32 Param,UINT32 Id,UINT32 Detach);
LOGICAL mlAutopilotWithinLimits(mlVehicleData*);
void mlAutopilotDisengage(mlVehicleData*);

void mlHitchVehicle(mlVehicleData*);

void mlSetStickXPos(FLOAT64);
void mlSetStickYPos(FLOAT64);
void mlSetRudderPosition(FLOAT64);
void mlSetThrottlePosition(FLOAT64);
void mlSetReportRangeAndTime(FLOAT32, SINT32);

void mlSetProfileFlight();

#ifdef __cplusplus
}
#endif

#endif