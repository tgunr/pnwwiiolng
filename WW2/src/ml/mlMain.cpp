#include <syscfg.h> // Path is project specific.
#include "include/vars.h"
#include "fio/fio.h"
#include "mem/mem.h"
#include "math/mathvect.h"
#include "math/mathnum.h"
#include "ml/mltypes.h"
#include "ml/mlCompon.h"
#include "mlengine.h"
#include "ml/mlfoils.h"
#include "ml/mlutility.h"
#include "ml/mlgauges.h"
#include "mlatmo.h"
#include "strt/strtClientMessages.h"
#include "draw/draw3d.h"
#include "veh/vehmain.h"
#include "veh/vehanim.h"
#include "ogrp/ogrptype.h"
#include "ml/mlgear.h"
#include "ml/mlweapons.h"
#include "pers/persmain.h"
#include "pers/persgunsight.h"
#include "pers/persbombsight.h"
#include "trn/trnop.h"
#include "trn/trnmain.h"
#include "trn/trnldr.h"
#include "trn/trncd.h"
#include "dip.h"
#include "trn/trnmain.h"
#include "fx/FXInterface.h"
#include "fx/fxMgr.h"
#include "fx/fxSnd.h"
#include "trn/trnvars.h"
#include "trn/trnclutter.h"
#include "trn/trncloud.h"
#include "strt/strtDTL.h"
#include "ob/obdraw.h"
#include "imap/ww2func.h"
#include "locl/loclMain.h"
#include "weap/weapons.h"
#include "weap/weapammo.h"
#include "math/matheul.h"
#include "math/math3d.h"
#include "strt/strtClientMessages.h"
#include "imap/ww2func.h"
#include "rn/rnmain.h"
#include "draw/draw2d.h"
#include "veh/vehVisDamage.h"
#include "locl/loclApp.h"
#include "locl/loclLang.h"
#include "math/mathUnitConversion.h"
#include "veh/vehfx.h"
#include "client/GameClient.h"
#include "client/clientStrtQueue.h"
#include "fx/fxAmb.h"
#include "fp/fpFPS.h"
#include "ui/uiChat.h"
#include "ob/obpool.h"
#include "ob/obMgr.h"
#include "weap/weapList.h"
#include "physics/physicsMain.h"
#include "ui/uiMain.h"
#include "trn/trnwtr.h"
#include "ob/obMgr.h"
#include "client/clientSTO.h"
#include "veh/vehmain.h"
#include "inst/instDescriptor.h"
#include "chat/chatWaypoint.h"
#include "cfg/cfgPrefs.h"
#include "pers/pershud.h"
#include "miss/missionDetail.h"
#include "scen/ScenarioManager.h"
#include "trn/trnScenario.h"
#include "ez/EZManager.h"
#include "veh/vehAnimEnum.h"
#include "fp/fpFPSdata.h"
#include "fp/fpPPO.h"

#ifdef GR2_BLEND_EDITOR
#include "gr2/gr2Anim.h"
#endif

#include <math.h>
#include <map>
#include <string>
#include <algorithm>
#include <list>
#include <utility>

// note: deltatime is in milliseconds

// big note: HOOF
// the original flight model was "tweaked" to get it to work
// when I did it I wasn't familiar enough with rotations or matrices to
// understand half of what I was doing.  As a result, the
// original code has negated axes, and odd rotation orders/directions
// that are a pain to understand.
//
// Since then I've switched to a more sane system which
// uses the right hand rule for rotation, and has x positive pointing east,
// y positive pointing north, and z positive pointing up.  To solve
// the whole rotation* order* dillemma I've added VehicleRotMat and VehicleRotInvMat
// to the vehicle data structure, multiply any vector by this matrix to get
// the vector adjusted by the vehicle attitude.
//
// VehicleRealVel is correct with the new system, but VehicleRealAtt is not
// (you'll need to negate the roll axis to get it correct, I haven't verified this though)
// VehicleRealAttVel is correct with the new system.
//
// if you must create a routine that deals directly with VehicleTorque or VehicleForce
// note these sign conventions:
// Torque Pitch Positive pitches the vehicle* up* (along x axis)
// Torque Roll Positive rolls the vehicle* left* (along y axis) (backwards from right hand rule)
// Torque Yaw Positive yaws the vehicle* left* (along z axis) (backwards from right hand rule)
// Force positive x "pushes" west (backwards)
// Force positive y "pushes" north
// Force positive z "pushes" down (backwards)
// follow these conventions and the vehicle should act correctly
//
// "right hand rule" means if you take your right thumb and point it in the axis
// positive direction, the fingers will "curl" in the direction of rotation

// Currently the WorldOriginOffset.z is always zero. So most likely everything will break if
// it is ever non-zero as the code randomly (i.e inconsistently) flips its z component.

/************************************************************************************************
	defines
*************************************************************************************************/

#if defined(_DEBUG)
//#define SPAWN_AT_DINANT_OFFLINE 1
//#define SPAWN_AT_ANTWERP_OFFLINE 1
//#define SPAWN_AT_OLD_OFFLINE 1
#define SPAWN_AT_AREA_51 1
#endif

#include "ui/uiDotCommands.h"

// Offline Spawn Locations
#if defined(SPAWN_AT_DINANT_OFFLINE) 		// Dinant
	#define OFFLINE_SPAWN_SUPERCELL_X 	18
	#define OFFLINE_SPAWN_SUPERCELL_Y 	235
	#define OFFLINE_SPAWN_LOCID_PERSON 	1316
	#define OFFLINE_SPAWN_LOCID_AIR		1316
	#define OFFLINE_SPAWN_LOCID_GROUND 	1316
	#define OFFLINE_SPAWN_LOCID_SEA 	1316
#elif defined(SPAWN_AT_ANTWERP_OFFLINE) 	// Antwerp
	#define OFFLINE_SPAWN_SUPERCELL_X 	16
	#define OFFLINE_SPAWN_SUPERCELL_Y 	240
	#define OFFLINE_SPAWN_LOCID_PERSON 	553
	#define OFFLINE_SPAWN_LOCID_AIR		553
	#define OFFLINE_SPAWN_LOCID_GROUND 	553
	#define OFFLINE_SPAWN_LOCID_SEA 	553
#elif defined(SPAWN_AT_OLD_OFFLINE) // Old offline
	#define OFFLINE_SPAWN_SUPERCELL_X 	48
	#define OFFLINE_SPAWN_SUPERCELL_Y 	226
	#define OFFLINE_SPAWN_LOCID_PERSON 	8
	#define OFFLINE_SPAWN_LOCID_AIR		3
	#define OFFLINE_SPAWN_LOCID_GROUND 	7
	#define OFFLINE_SPAWN_LOCID_SEA 	21
#else // Area 51
	#define OFFLINE_SPAWN_SUPERCELL_X 	-25
	#define OFFLINE_SPAWN_SUPERCELL_Y 	281
	#define OFFLINE_SPAWN_LOCID_PERSON 	288
	#define OFFLINE_SPAWN_LOCID_AIR		286
	#define OFFLINE_SPAWN_LOCID_GROUND 	289
	#define OFFLINE_SPAWN_LOCID_SEA 	300
#endif // Offline Spawn Locations


#define _UPDATES_PER_SEC 50.0 // 50 updates per sec

#define _MS_PER_TIC 20.0 // 50 cycles per second model loop

#define _MAX_TICS 200 // 4 sec of backlogged updates
#define _TIME_SCALE 1.0 // flight model time scale, set to 1.0 for normal operation
#define _MAX_BUMPANDFRICTION_MATERIALS 2048
#define _TIME_BEFORE_FRIENDLY_COLLISIONS_ON 120000
#define _NO_COLLIDE_AFTER_UNRIDE_TIME 15000
#define MAX_HIT_SOUND 512
#define DEF_SIZE_OF_BAFDATA sizeof(BumpAndFrictionData)
#define _NUM_CP_LOOPS 100

#define MIN_VEHICLE_TOW_DISTANCE 3.0 // added to vehicle radius
#define MIN_TOW_DISTANCE 2.0
#define MIN_HOIST_XY_DISTANCE2 	9.0
#define MIN_HOIST_Z_DISTANCE2  	1.0
#define MIN_RIDE_XY_DISTANCE2 	9.0
#define MIN_RIDE_Z_DISTANCE2  	1.0
#define MAX_UNHOIST_Z 		 	1.0

#define UNHOIST_UI_LOC_ID 		500
#define HOIST_UI_LOC_ID	 		501
#define NO_HOIST_UI_LOC_ID		502
#define RIDING_UI_LOC_ID		503
#define HOISTFIRST_UI_LOC_ID 	504
#define NO_RIDE_UI_LOC_ID	 	505
#define RIDE_UI_LOC_ID			506
#define TOO_FAST_UI_LOC_ID		507
#define HOIST_POSITION_OCCUPIED_LOC_ID		515
#define	RIDE_POSITION_OCCUPIED_LOC_ID		516
#define	UNHOIST_TOO_HIGH_LOC_ID	517
#define AUTOPILOT_OFF_LIMIT_ID	524
#define TOW_NOHITCHINRANGE_LOC_ID	530
#define TOW_HITCHED_LOC_ID			531
#define TOW_UNHITCHED_LOC_ID		532
#define DEPLOY_FOR_TOWING_ID		273

#define AUTOPILOT_MAX_ATTACC2 16.0
#define AUTOPILOT_MAX_ACC2 16.0
#define AUTOPILOT_MAX_VELZ2 128.0
#define AUTOPILOT_MAX_ROLL2 ((16*PI/180.0)*(16*PI/180.0))

#define DRAG_DAMAGE 4000.0f

enum
{
	ML_VARNAME_NOVAR,
	ML_VARNAME_VEHICLETYPE,
	ML_VARNAME_VEHICLENAME,
	ML_VARNAME_COMPONENTFILE,
	ML_VARNAME_ENGINEFILE,
	ML_VARNAME_AIRFOILFILE,
	ML_VARNAME_AILERON_MAX_SPEED, // max speed for full deflection
	ML_VARNAME_RUDDER_MAX_SPEED, // max speed for full deflection
	ML_VARNAME_ELEVATOR_MAX_SPEED,// max speed for full deflection
	ML_VARNAME_AILERON_COMPRESS_SPEED,// compression speed
	ML_VARNAME_RUDDER_COMPRESS_SPEED,
	ML_VARNAME_ELEVATOR_COMPRESS_SPEED,
	ML_VARNAME_NUMBER_OF_FLAP_POSITIONS,
	ML_VARNAME_VEHICLE_OBJECTNAME,
	ML_VARNAME_AILERON_RESPONSETIME,
	ML_VARNAME_ELEVATOR_RESPONSETIME,
	ML_VARNAME_RUDDER_RESPONSETIME,
	ML_VARNAME_THROTTLE_RESPONSETIME,
	ML_VARNAME_AILERON_TAB_RATIO,
	ML_VARNAME_ELEVATOR_TAB_RATIO,
	ML_VARNAME_RUDDER_TAB_RATIO,
	ML_VARNAME_AILERON_TAB_STATIC,
	ML_VARNAME_ELEVATOR_TAB_STATIC,
	ML_VARNAME_RUDDER_TAB_STATIC,
	ML_VARNAME_AILERON_SPRINGTAB_RATIO,
	ML_VARNAME_ELEVATOR_SPRINGTAB_RATIO,
	ML_VARNAME_RUDDER_SPRINGTAB_RATIO,
	ML_VARNAME_FLAPSAREANALOG,
	ML_VARNAME_FLAPDEPLOYSPEED,
	ML_VARNAME_GEARDEPLOYSPEED,
	ML_VARNAME_HATCHOPENSPEED,
	ML_VARNAME_CANOPYOPENSPEED,
	ML_VARNAME_STALLSOUND,
	ML_VARNAME_GEARSOUND,
	ML_VARNAME_FLAPSSOUND,
	ML_VARNAME_CANOPYSOUND,
	ML_VARNAME_PRIMARYGUNSOUND,
	ML_VARNAME_SECONDARYGUNSOUND,
	ML_VARNAME_PRIMARYGUNISAUTOMATIC,
	ML_VARNAME_SECONDARYGUNISAUTOMATIC,
	ML_VARNAME_POSITIONFILE,
	ML_VARNAME_LEFTSLATSOUND,
	ML_VARNAME_RIGHTSLATSOUND,
	ML_VARNAME_WINDSOUND,
	ML_VARNAME_HATCHSOUND,
	ML_VARNAME_SLEDGEHAMMERSTABILIZER,
	ML_VARNAME_PHYSICSVERSION,
	ML_VARNAME_GEARJAMSOUND,
	ML_VARNAME_FLAPJAMSOUND,
	ML_VARNAME_CANOPYJAMSOUND,
	ML_VARNAME_STATICFRICTION,
	ML_VARNAME_DYNAMICFRICTION,
	ML_MAX_VEHICLEVARS
};

/************************************************************************************************
	variables
*************************************************************************************************/

static UCHAR _VehicleTypeVarNames[ML_MAX_VEHICLEVARS][32] = {
													"none",
													"vehicletype",
													"vehiclename",
													"componentfile",
													"enginefile",
													"airfoilfile",
													"aileronmaxspeed",
													"ruddermaxspeed",
													"elevatormaxspeed",
													"aileroncompressspeed",
													"ruddercompressspeed",
													"elevatorcompressspeed",
													"numberofflappositions",
													"vehicleobjectname",
													"aileronresponsetime",
													"elevatorresponsetime",
													"rudderresponsetime",
													"throttleresponsetime",
													"ailerontabratio",
													"elevatortabratio",
													"ruddertabratio",
													"ailerontabstatic",
													"elevatortabstatic",
													"ruddertabstatic",
													"aileronspringtabratio",
													"elevatorspringtabratio",
													"rudderspringtabratio",
													"flapsareanalog",
													"flapdeployspeed",
													"geardeployspeed",
													"HatchDeploySpeed",
													"canopyopenspeed",
													"stallsound",
													"gearsound",
													"flapssound",
													"canopysound",
													"primarygunsound",
													"secondarygunsound",
													"primaryisautomatic",
													"secondaryisautomatic",
													"positionfile",
													"leftslatsound",
													"rightslatsound",
													"windsound",
													"hatchsound",
													"sledgehammerstabilizer",
													"physicsversion",
													"gearjamsound",
													"flapjamsound",
													"canopyjamsound",
													"staticfriction",
													"dynamicfriction"
													};


FLOAT64 _StickXPos;
FLOAT64 _StickYPos; // -1.0 to 1.0
FLOAT64 _ThrottlePos; // 0 to 1.0
FLOAT64 _RudderPos; // -1.0 to 1.0
UINT32 _ThrottleDecreaseKeyIsDown;
UINT32 _ThrottleIncreaseKeyIsDown;
extern  fpsWeaponTypes   currentWeaponModel;

// hack for vehicle analysis input and floatere
extern char	buffer[256];
extern char lastbuffer[256];

char BuzzardFollowVehicle[256];
FLOAT32 BuzzardFollowDistance;

FLOAT32	_ReportRange;
SINT32 	_ReportTime;

LOGICAL IsFirstTic;

static UINT32 _NumTics;

static LOGICAL _RefreshList;
static BumpAndFrictionData _BAFData[_MAX_BUMPANDFRICTION_MATERIALS];
static UINT32 _NumBAF;

static UINT32 _LastTOUpdateTime;

// spawn stuff
static LOGICAL MobileSpawn;
static UINT32 MobileSpawnVehicle;
static FLOAT64VECTOR MobileSpawnLocation;
static FLOAT64 MobileSpawnYaw;
static SINT32 SpawnSuperCellX;
static SINT32 SpawnSuperCellY;
static SINT32 SpawnLocId;
static UINT32 SpawnNumber = 1;
static Vec3d SpawnPos;
static UINT64 SpawnInstance = 0;

#ifdef BETA_RELEASE
#define MAX_LEAKS 1000

SINT32 _CurrentLOD = 0; // LOD number - 1
SINT32 _CurrentHitmapDisplay = 0; // 0 = normal, 1 = inside, 2 = outside, 3 = both
SINT32 _CurrentHitmapObjectDisplay = 0;
SINT32 _DisplayContactPoints = 0;
SINT32 _DisplayWheelContactPoints = 0;
SINT32 _ExerciseWheelRotations = 0;
SINT32 _ExerciseWheelSuspension = 0;
SINT32 _FirstTime = 1;
UINT32 _StateFlags;
UINT32 _CycleDamageGroup[ML_MAX_DAMAGE_GROUPINGS];
FLOAT32 _DamageGroupLevel[ML_MAX_DAMAGE_GROUPINGS];
SINT32 _LastUpdateTime;
UINT32 _HighlightHitmapObjectNum;
SINT32 	LeakGridInc, LeakAngleInc, LeakYawStart, LeakYawEnd, LeakPitchStart, LeakPitchEnd;
LOGICAL	DoLeakTest;
SINT32 	NumLeaks;
piVECTOR Leaks[MAX_LEAKS][2];
SINT32	CurLeakYaw, CurLeakPitch;
LOGICAL DisplayFoils;
LOGICAL DisplayPhysics;
LOGICAL DisplayCollideLines;
LOGICAL DisableForces;
LOGICAL TurretConvergenceTraverseOff;
LOGICAL TurretConvergenceElevateOff;
#endif

static std::map<std::string, BumpAndFrictionData*> _hashBAF;

/************************************************************************************************
	functions
*************************************************************************************************/

static UINT32 _ClearVehicleData(mlVehicleData * VehicleData);
static UINT32 _ShutDownSoundsAndFx(mlVehicleData* VehicleData);
UINT32 _ReadBumpAndFrictionTable();

static UINT32 _UpdateAirfoilComponent(mlComponent * Component, mlVehicleData * VehicleData,FLOAT64 DeltaTime);
static UINT32 _UpdateHaulPoint(mlComponent * Component,
							   mlVehicleData * VehicleData,
							   FLOAT64 DeltaTime);
static UINT32 _UpdateTowBar(mlComponent * Component,
							mlVehicleData * VehicleData,
							FLOAT64 DeltaTime);
static UINT32 _UpdateEngineComponent(mlComponent * Component,
									 mlVehicleData * VehicleData,
									 FLOAT64 DeltaTime);
static UINT32 _UpdateCrew(mlComponent * Component,mlVehicleData * VehicleData);
static UINT32 _UpdatePitotTube(mlComponent * Component,mlVehicleData * VehicleData);
static UINT32 _UpdateGearComponent(mlComponent * Component,
								   mlVehicleData * VehicleData,
								   FLOAT64 DeltaTime,
									UINT32 PassNum,
									FLOAT64VECTOR* SavedVehicleForce,
									FLOAT64VECTOR* TempNormForceSum);
static UINT32 _UpdateBallast(mlComponent* Component,mlVehicleData * VehicleData);

static UINT32 _UpdateWeaponMount(mlComponent * Component,
								 mlVehicleData * VehicleData,
								 FLOAT64 DeltaTime,
								 UINT32 PassNum);

static void _SoundStopped(void* data);


static UINT32 _TunnelResetVehiclePos(mlVehicleData * VehicleData,FLOAT64 Alt);
static UINT32 _UpdateMovingParts(mlVehicleData * VehicleData,FLOAT64 DeltaTime);
static UINT32 _UpdateBBoxList(mlVehicleData * VehicleData);
static UINT32 _DetermineVehicleBoundingBoxIntersect(mlVehicleData * VehicleData,
													mlObjectData * VehicleObjectData,
													trnCDTerrainObject ** TOList,
													FLOAT64VECTOR AdjustedForCGPos,
													piMATRIX * AttMat);
static UINT32 _DetermineIfBoundingBoxIntersects(UINT32 * TrnCDPos,
												 UINT32 MaxTrnCDPos,
												 trnCDTerrainObject *** TOPtr,
												 OBJECT *** ObjPtr,
												 trnCDTerrainObject * TO,
												 OBJECT * Obj,
												 piVECTOR * BBox);

static UINT32 _UpdateTerrainCollideLists(mlVehicleData * VehicleData,UINT32 ForceRecalc);


static UINT32 mlApplyStaticForceAtRelativePointWithMat(mlVehicleData * VehicleData,
									     FLOAT64VECTOR * RelPoint,
									     FLOAT64VECTOR * ForceVec);

static UINT32 _DoLineCollision(mlVehicleData* VehicleData,
							   mlObjectData * ObjectData,
							   UINT32 PassNum,
							   FLOAT64VECTOR * SavedVehicleForce,
							   FLOAT64VECTOR * TempNormForceSum,
							   FLOAT64 DeltaTime);

static UINT32 _CheckSpecificObjectForLineCollision(mlVehicleData * VehicleData,
												   mlObjectCollidePnt * CollidePnt,
												   FLOAT64VECTOR* VehicleForce,
												   FLOAT64VECTOR* NormalForceSum,
												   UINT32 PassNum,
												   FLOAT64 DeltaTime);


static UINT32 _UpdateShock(FLOAT64 * ShockVel,FLOAT64 * ShockPos,FLOAT64 InForceMagnitude,FLOAT64 DeltaTime);

static UINT32 _UpdateVehicleSounds(mlVehicleData * VehicleData);
static UINT32 _ReadPositionFile(mlVehicleData * VehicleData,SINT8 * PositionFileName,SINT8 * DirName);

static UINT32 _DoVehicleCollision(mlVehicleData * VehicleData,
							FLOAT64 DeltaTime,
							piVECTOR * V1,
							piVECTOR * V2);

static UINT32 _DoODEVehicleCollision(mlVehicleData * VehicleData, FLOAT64 DeltaTime);

static UINT32 mlDetermineVehicleCollisions(mlVehicleData * VehicleData, FLOAT64 DeltaTime);

static SINT32 mlVehicleUpdateRide(mlVehicleData* VehicleData);

#if(0)
static SINT32 _radioStatic(trnCDTerrainObject* t);
#endif

static SINT32 mlVehicleHandleTurretConvergence(mlVehicleData*);
static SINT32 mlHoistCollisions(vehVehicle* Vehicle);
static vehVehicle* mlHoistCollideRoundWithVehicles(vehVehicle*, piVECTOR const* OldPos, piVECTOR const* NewPos);
static LOGICAL mlHoistLineCollideWithWorld(vehVehicle* Vehicle, UINT32 NumCollideObjects, mlObjectCollidePnt* CollidePntList, OBJECT* Hitmap);
static LOGICAL mlCollideWithVehicle(vehVehicle* CurrentVehicle, piVECTOR const* OldPos, piVECTOR const* NewPos, LOGICAL UseHitbox);
static LOGICAL mlHoistCollideRoundWithWorld(vehVehicle*, OBJECT* Hitmap, piVECTOR const* OldPos, piVECTOR const* NewPos);
static LOGICAL mlHoistCollideRoundWithHitmap(OBJECT* hitmap, piVECTOR const* OldPos, piVECTOR const* NewPos);
static LOGICAL mlHoistCollideRoundWithTerrain(piVECTOR const* OldPos, piVECTOR const* NewPos);

#ifdef BETA_RELEASE
static UINT32 _HandleVehicleAnalysisStuff(mlVehicleData* VehicleData);
static UINT32 _TraverseAndSetupColorCoding(OBJECT * Object,UINT32 UseAlpha,vehVehicle* Vehicle,
										   UINT32 CurrentHitmapObjectDisplay);
static void _LeakTest();
#endif

#ifdef GM_RELEASE
static UINT32 _DoEyeOperations(mlVehicleData* VehicleData);
#endif

extern UINT32 _AdjustVelAndRotVel(mlVehicleData*, FLOAT64VECTOR* Vel,
							   FLOAT64ATT* AttVel,
							   FLOAT64VECTOR* Acc,
							   FLOAT64ATT* AttAcc,
							   FLOAT64VECTOR* Force,
							   FLOAT64ATT* Torque,
							   FLOAT64 Mass,
							   FLOAT64ATT* RotInertia,
							   FLOAT64 DeltaTime, LOGICAL UseCorrectPhysics);

extern UINT32 _AdjustPosAndAccel(FLOAT64VECTOR* Pos,
								  FLOAT64ATT* Att,
								  FLOAT64VECTOR* Vel,
								  FLOAT64VECTOR* Acc,
								  FLOAT64VECTOR* LastAcc,
								  FLOAT64VECTOR* LastVel,
								  FLOAT64ATT* AttVel,
								  FLOAT64ATT* AttAcc,
								  FLOAT64ATT* LastAttAcc,
								  FLOAT64 DeltaTime);

extern UINT32 _ProcessControlInputs(UINT32 DeltaTime,mlVehicleData * VehicleData);

static void _HandleContactDamage(vehVehicle* Vehicle);

static void _DeclareVehicleKilled (mlVehicleData* pVehicleData, mlComponent* pComponent);
static void _CheckForSecondaryVehicleKill (mlVehicleData* pVehicleData, mlComponent* pComponent);

extern void fpPPOSwitchPPO(bool allowFrus);

/************************************************************************************************
	mlMain
*************************************************************************************************/
UINT32 mlInit()
{
	_ReadBumpAndFrictionTable();

	uiDotCommandsInit();
	OverrideSpawner = NO_OVERRIDE;

	return 0;
}

UINT32 mlUpdate(mlVehicleData * VehicleData,OBJECT * VehicleObject)
{
    static UINT32 _LastCallTime = 0;

    bool VehHasGaugeData = true;

	_ProcessControlInputs(frametime - _LastCallTime,VehicleData);

    if(VehicleData && VehicleData->VehicleMasterPtr)
    {
        if(VehicleData->VehicleMasterPtr->VehicleType->Type == VEH_TYPE_PERSON)
        {
            VehHasGaugeData = false;
        }
    }

	if(VehHasGaugeData)
        mlGaugeUpdatePlayerGaugeData(VehicleData,VehicleObject);

	// do stall sound

	_LastCallTime = frametime;


	// clear trigger states
	// if triggers are held down it'll get reset before next update

	if(_ReportTime != 0 && (frametime - _ReportTime) > 0)
	{
		uiChatPrintLine(loclGetLocalizationStringByID(838), (SINT32)_ReportRange, (int)METER_TO_YD(_ReportRange));
		_ReportTime = 0; // not sure how this got fubar'ed
	}

	return 0;
}

UINT32 mlKill()
{
	return 0;
}

UINT32 mlInitVehicleData(mlVehicleData * VehicleData)
{
	_ClearVehicleData(VehicleData);

	VehicleData->VehicleAttMat = VehicleData->VehicleAttInvMat = IdentityMatrix;

	// set our default per vehicle friction modifiers
	VehicleData->StaticFrictionMod = 1.0f;
	VehicleData->DynamicFrictionMod = 1.0f;

	return 0;
}

UINT32 mlKillVehicleData(mlVehicleData * VehicleData)
{
//	if(VehicleData->Body)
//		ptkFreeBody(VehicleData->Body);

	_ShutDownSoundsAndFx(VehicleData);

	_ClearVehicleData(VehicleData);

	return 0;
}

UINT32 _ClearVehicleData(mlVehicleData * VehicleData)
{

	if (VehicleData->AirfoilList != nullptr)
	{
		mlfoilsFreeAirfoilData(VehicleData->AirfoilList);
		memFree(VehicleData->AirfoilList);
		VehicleData->AirfoilList = nullptr;
	}
	if (VehicleData->EngineList != nullptr)
	{
		mlengineFreeEngineData(VehicleData->EngineList);
		memFree(VehicleData->EngineList);
		VehicleData->EngineList = nullptr;
	}
	if (VehicleData->ComponentList != nullptr)
	{
		mlcomponFreeComponentList(VehicleData->ComponentList);
		VehicleData->ComponentList = nullptr;
	}

	VehicleData->NumCageCollideLines = 0;
	if (VehicleData->CageLineV1 != nullptr)
	{
		memFree(VehicleData->CageLineV1);
		VehicleData->CageLineV1 = nullptr;
	}
	if (VehicleData->CageLineV2 != nullptr)
	{
		memFree(VehicleData->CageLineV2);
		VehicleData->CageLineV2 = nullptr;
	}
	if (VehicleData->CageLineObject != nullptr)
	{
		memFree(VehicleData->CageLineObject);
		VehicleData->CageLineObject = nullptr;
	}

	memset(VehicleData,0,sizeof(mlVehicleData));

	return 0;
}

UINT32 _ShutDownSoundsAndFx(mlVehicleData* VehicleData)
{
	UINT32 i, j;
	mlComponent * Component;
	mlEngineComponentData * EngineComponentData;
	mlWeaponMount * Mount;

	if (VehicleData == nullptr || VehicleData->ComponentList == nullptr)
		return 1;

	mlComponentList* compList = VehicleData->ComponentList;
	for(i = 0; i < compList->NumComponents;  i++)
	{
		Component = &compList->Components[i];

		if (Component->Type == ML_COMPONENT_ENGINE)
		{
			EngineComponentData = (mlEngineComponentData*)Component->SpecialData;

			if (EngineComponentData->EngineSoundLoaded == 1)
			{
				fxSndStopSound(EngineComponentData->EngineSoundId);
			}
			if (EngineComponentData->IdleSoundLoaded == 1)
			{
				fxSndStopSound(EngineComponentData->IdleSoundId);
			}
			if (EngineComponentData->StarterSoundLoaded == 1)
			{
				fxSndStopSound(EngineComponentData->StarterSoundId);
			}

            for(SINT32 q = 0; q < 4; q++)
            {
                if(EngineComponentData->DamageLevelSoundId[q] > 0)
                {
                    fxSndStopSound(EngineComponentData->DamageLevelSoundId[q]);
                }
            }
		}
		if (Component->Type == ML_COMPONENT_WEAPONMOUNT)
		{
			Mount = (mlWeaponMount*)Component->SpecialData;
//			hackUnlockBlock(Mount);
			for (j=0;j<Mount->NumberOfWeaponsInMount;j++)
			{
				if (Mount->WeaponSoundId[j] > 0)
				{
					fxSndStopSound(Mount->WeaponSoundId[j]);
				}
			}
//			hackLockBlock(Mount);
		}

		if (Component->Type == ML_COMPONENT_HAULPOINT)
		{
			if (Component->SoundId > 0)
			{
				fxSndStopSound(Component->SoundId);
			}
		}
	}

	if (VehicleData->WindSoundStarted == 1 &&
		VehicleData->WindSoundId > 0)
	{
		fxSndStopSound(VehicleData->WindSoundId);
	}

	if (VehicleData->StallSoundStarted == 1 &&
		VehicleData->StallSoundId > 0)
	{
		fxSndStopSound(VehicleData->StallSoundId);
	}

	if(VehicleData->FlapDamageSoundId > 0)
		fxSndStopSound(VehicleData->FlapDamageSoundId);

	if(VehicleData->GearDamageSoundId > 0)
		fxSndStopSound(VehicleData->GearDamageSoundId);

	return 0;
}

UINT32 mlReadVehicleFile(mlVehicleData * VehicleData,
						 UCHAR* FileName,
						 UCHAR* DirName,
						 UINT32 VehicleNum)
{
	static UCHAR Buffer[256];
	static UCHAR ComponentListFilename[128]="";
	static UCHAR EngineListFilename[128]="";
	static UCHAR AirfoilListFilename[128]="";
	static UCHAR PositionFileName[128]="";
	static UCHAR UseFileName[128];
	SINT32 VehicleFile;
	UCHAR * VarName;
	UCHAR * VarContents;
	UINT32 VarNum;
	mlComponent * Component;
	mlWeaponMount * Mount;

	snprintf((char*)UseFileName, 127, "%s\\%s",DirName, FileName);

	VehicleFile = fioOpen((char*)UseFileName);
	if (VehicleFile < 0)
  {
    Write_Debug("Failed to load vehicle file %s\n",UseFileName);
		return 1;
  }

	while (fioGetString(VehicleFile,(char*)Buffer,255) != 0)
	{
		if (Buffer[0] == '#')
		{
			continue;
		}

		mlUtilityGetVarnameAndContents(&VarName,&VarContents,Buffer);

		if (VarName == nullptr || VarContents == nullptr)
		{
			continue;
		}

		mlUtilityFindVarname(&VarNum,
							VarName,
							(UCHAR*)&_VehicleTypeVarNames[0][0],
							32,
							ML_MAX_VEHICLEVARS);

		if (VarNum == ML_VARNAME_NOVAR)
		{
			Write_Debug("Unidentified variable %s in file %s\n",VarName, FileName);
			continue;
		}

		switch (VarNum)
		{
			case ML_VARNAME_VEHICLETYPE:
			// unused
			break;

			case ML_VARNAME_VEHICLENAME:
					strcpy((char*)VehicleData->VehicleName,(char*)VarContents);
					break;

			case ML_VARNAME_VEHICLE_OBJECTNAME:
					strcpy((char*)VehicleData->VehicleObjectName,(char*)VarContents);
					break;

			case ML_VARNAME_COMPONENTFILE:
					strcpy((char*)ComponentListFilename,(char*)VarContents);
					break;

			case ML_VARNAME_ENGINEFILE:
					strcpy((char*)EngineListFilename,(char*)VarContents);
					break;

			case ML_VARNAME_AIRFOILFILE:
					strcpy((char*)AirfoilListFilename,(char*)VarContents);
					break;

			case ML_VARNAME_POSITIONFILE:
					strcpy((char*)PositionFileName,(char*)VarContents);
					break;

			case ML_VARNAME_AILERON_MAX_SPEED:
					VehicleData->AileronMaxSpeed = atof((char*)VarContents) * 1000.0 / 3600.0;	// in kph
					break;

			case ML_VARNAME_RUDDER_MAX_SPEED:
					VehicleData->RudderMaxSpeed = atof((char*)VarContents) * 1000.0 / 3600.0;
					break;

			case ML_VARNAME_ELEVATOR_MAX_SPEED:
					VehicleData->ElevatorMaxSpeed = atof((char*)VarContents) * 1000.0 / 3600.0;
					break;

			case ML_VARNAME_AILERON_COMPRESS_SPEED:
					VehicleData->AileronCompressSpeed = atof((char*)VarContents);
					break;

			case ML_VARNAME_RUDDER_COMPRESS_SPEED:
					VehicleData->RudderCompressSpeed = atof((char*)VarContents);
					break;

			case ML_VARNAME_ELEVATOR_COMPRESS_SPEED:
					VehicleData->ElevatorCompressSpeed = atof((char*)VarContents);
					break;

			case ML_VARNAME_NUMBER_OF_FLAP_POSITIONS:
					VehicleData->NumberOfFlapPositions = atoi((char*)VarContents);
					break;

			case ML_VARNAME_AILERON_RESPONSETIME:
					VehicleData->AileronResponseTime = atof((char*)VarContents);
					break;
			case ML_VARNAME_ELEVATOR_RESPONSETIME:
					VehicleData->ElevatorResponseTime = atof((char*)VarContents);
					break;
			case ML_VARNAME_RUDDER_RESPONSETIME:
					VehicleData->RudderResponseTime = atof((char*)VarContents);
					break;
			case ML_VARNAME_THROTTLE_RESPONSETIME:
					VehicleData->ThrottleResponseTime = atof((char*)VarContents);
					break;
			case ML_VARNAME_AILERON_TAB_RATIO:
					VehicleData->AileronTabRatio = atof((char*)VarContents);
					break;
			case ML_VARNAME_ELEVATOR_TAB_RATIO:
					VehicleData->ElevatorTabRatio = atof((char*)VarContents);
					break;
			case ML_VARNAME_RUDDER_TAB_RATIO:
					VehicleData->RudderTabRatio = atof((char*)VarContents);
					break;
			case ML_VARNAME_AILERON_TAB_STATIC:
					VehicleData->AileronTabStatic = atof((char*)VarContents);
					break;
			case ML_VARNAME_ELEVATOR_TAB_STATIC:
					VehicleData->ElevatorTabStatic = atof((char*)VarContents);
					break;
			case ML_VARNAME_RUDDER_TAB_STATIC:
					VehicleData->RudderTabStatic = atof((char*)VarContents);
					break;
			case ML_VARNAME_AILERON_SPRINGTAB_RATIO:
					VehicleData->AileronSpringTabRatio = atof((char*)VarContents);
					break;
			case ML_VARNAME_ELEVATOR_SPRINGTAB_RATIO:
					VehicleData->ElevatorSpringTabRatio = atof((char*)VarContents);
					break;
			case ML_VARNAME_RUDDER_SPRINGTAB_RATIO:
					VehicleData->RudderSpringTabRatio = atof((char*)VarContents);
					break;
			case ML_VARNAME_FLAPSAREANALOG:
					VehicleData->FlapsAreAnalog = atoi((char*)VarContents);
					break;
			case ML_VARNAME_FLAPDEPLOYSPEED:
					VehicleData->FlapDeploySpeed = atof((char*)VarContents);
					break;
			case ML_VARNAME_GEARDEPLOYSPEED:
					VehicleData->GearDeploySpeed = atof((char*)VarContents);
					break;
			case ML_VARNAME_HATCHOPENSPEED:
					VehicleData->HatchDeploySpeed = atof((char*)VarContents);
					break;
			case ML_VARNAME_CANOPYOPENSPEED:
					VehicleData->CanopyDeploySpeed = atof((char*)VarContents);
					break;
			case ML_VARNAME_STALLSOUND:
					strcpy((char*)VehicleData->StallSound,(char*)VarContents);
					break;
			case ML_VARNAME_GEARSOUND:
					strcpy((char*)VehicleData->GearSound,(char*)VarContents);
					break;
			case ML_VARNAME_FLAPSSOUND:
					strcpy((char*)VehicleData->FlapsSound,(char*)VarContents);
					break;
			case ML_VARNAME_CANOPYSOUND:
					strcpy((char*)VehicleData->CanopySound,(char*)VarContents);
					break;
			case ML_VARNAME_PRIMARYGUNSOUND:
					strcpy((char*)VehicleData->PrimaryGunSound,(char*)VarContents);
					break;
			case ML_VARNAME_SECONDARYGUNSOUND:
					strcpy((char*)VehicleData->SecondaryGunSound,(char*)VarContents);
					break;
			case ML_VARNAME_LEFTSLATSOUND:
					strcpy((char*)VehicleData->LeftSlatSound,(char*)VarContents);
					break;
			case ML_VARNAME_RIGHTSLATSOUND:
					strcpy((char*)VehicleData->RightSlatSound,(char*)VarContents);
					break;
			case ML_VARNAME_WINDSOUND:
					strcpy((char*)VehicleData->WindSound,(char*)VarContents);
					break;
			case ML_VARNAME_HATCHSOUND:
					strcpy((char*)VehicleData->HatchSound,(char*)VarContents);
					break;
			case ML_VARNAME_PRIMARYGUNISAUTOMATIC:
					VehicleData->PrimaryGunIsAutomatic = atoi((char*)VarContents);
					break;
			case ML_VARNAME_SECONDARYGUNISAUTOMATIC:
					VehicleData->SecondaryGunIsAutomatic = atoi((char*)VarContents);
					break;
			case ML_VARNAME_SLEDGEHAMMERSTABILIZER:
					VehicleData->UseKludgeStabilizer = atoi((char*)VarContents);
					break;
			case ML_VARNAME_PHYSICSVERSION:
				VehicleData->PhysicsVersion = atoi((char*)VarContents);
			break;
			case ML_VARNAME_GEARJAMSOUND:
				strncpy((char*)VehicleData->GearJamSound,(char*)VarContents, 63);
			break;
			case ML_VARNAME_FLAPJAMSOUND:
				strncpy((char*)VehicleData->FlapJamSound,(char*)VarContents, 63);
			break;
			case ML_VARNAME_CANOPYJAMSOUND:
				strncpy((char*)VehicleData->CanopyJamSound,(char*)VarContents, 63);
			break;
			case ML_VARNAME_STATICFRICTION:
				VehicleData->StaticFrictionMod = (FLOAT32)atof((char*)VarContents);
				break;
			case ML_VARNAME_DYNAMICFRICTION:
				VehicleData->DynamicFrictionMod = (FLOAT32)atof((char*)VarContents);
				break;
			default:
				Write_Debug("Unidentified variable %s in file %s",VarName,UseFileName);
				break;
		}

	}
	fioClose(VehicleFile);

	// now read airfoil file
	mlfoilsReadAirfoilFile(&VehicleData->AirfoilList,AirfoilListFilename,DirName);

	// now read engine file
	mlengineReadEngineFile(&VehicleData->EngineList,EngineListFilename,DirName);

	// clear the secondary component count arrays
	memset(VehicleData->TotalSecondaryCriticalComponentCount,0x0,sizeof(VehicleData->TotalSecondaryCriticalComponentCount));
	memset(VehicleData->SecondaryCriticalComponentCount,0x0,sizeof(VehicleData->SecondaryCriticalComponentCount));

	// now read component file
	mlcomponReadComponentFile(&VehicleData->ComponentList,
								ComponentListFilename,
								DirName,
								VehicleData->AirfoilList,
								VehicleData->EngineList,
								VehicleNum,
								VehicleData->VehicleMasterPtr);

	if(PositionFileName[0])
		_ReadPositionFile(VehicleData,(SCHAR*)PositionFileName,(SCHAR*)DirName);

	// update weapon mount numbers
	if (VehicleData->ComponentList != nullptr)
	{
		mlComponentListIterator it;

		mlComponentListBegin(VehicleData->ComponentList, &it);
		for(; it.comp; mlComponentListNext(&it))
		{
			Component = it.comp;
			if (Component != nullptr)
			{
				if (Component->Type == ML_COMPONENT_WEAPONMOUNT)
				{
					Mount = (mlWeaponMount*)Component->SpecialData;
//					hackUnlockBlock(Mount);
					if (Mount != nullptr)
					{
						Mount->MountSerialNum = VehicleData->NumberOfWeaponMounts;
						VehicleData->NumberOfWeaponMounts++;
					}
//					hackLockBlock(Mount);
				}
			}
		}
	}

	VehicleData->LastVehicleUpdateTime = 0;
 	if(VehicleData->VehicleMasterPtr == nullptr || VehicleData->VehicleMasterPtr->VehicleType->Type != VEH_TYPE_SEA)
		persResetGunsightAdjust(0);
	else
		persResetGunsightAdjust(1/8.0f); // since we now use for turret convergence

	return 0;
}

UINT32 mlCalcVehicleMass(mlVehicleData* VehicleData)
{
	mlComponent* CurrentComponent;
	FLOAT64 TotalMass;
	mlComponentListIterator it;

	if (VehicleData == nullptr || VehicleData->ComponentList == nullptr)
		return 1;

	TotalMass = 0;

	mlComponentListBegin(VehicleData->ComponentList, &it);
	for(; it.comp; mlComponentListNext(&it))
	{
		CurrentComponent = it.comp;
		TotalMass += CurrentComponent->Mass;
	}

	VehicleData->VehicleMass = TotalMass;

	return 0;
}

UINT32 mlSetVehicleComponentsRelativePos(mlVehicleData* VehicleData)
{
	mlComponent* CurrentComponent;
	mlComponentListIterator it;

	if (VehicleData == nullptr || VehicleData->ComponentList == nullptr)
		return 1;

	mlComponentListBegin(VehicleData->ComponentList, &it);
	for(; it.comp; mlComponentListNext(&it))
	{
		CurrentComponent = it.comp;

		CurrentComponent->CurrentRelativePos.x =
					CurrentComponent->OriginalRelativePos.x -
					VehicleData->VehicleCenterOfMass.x;
		CurrentComponent->CurrentRelativePos.y =
					CurrentComponent->OriginalRelativePos.y -
					VehicleData->VehicleCenterOfMass.y;
		CurrentComponent->CurrentRelativePos.z =
					CurrentComponent->OriginalRelativePos.z -
					VehicleData->VehicleCenterOfMass.z;
	}

	return 0;
}

UINT32 mlCalcVehicleCenterOfMass(mlVehicleData* VehicleData)
{
	mlComponent* CurrentComponent;
	FLOAT64 TotalMass;
	FLOAT64VECTOR Pos;
	mlComponentListIterator it;

	if (VehicleData == nullptr || VehicleData->ComponentList == nullptr)
		return 1;

	TotalMass = 0;
	Pos.x = Pos.y = Pos.z = 0;

	mlComponentListBegin(VehicleData->ComponentList, &it);
	for(; it.comp; mlComponentListNext(&it))
	{
		CurrentComponent = it.comp;
		TotalMass += CurrentComponent->Mass;

		Pos.x += CurrentComponent->Mass * CurrentComponent->OriginalRelativePos.x;
		Pos.y += CurrentComponent->Mass * CurrentComponent->OriginalRelativePos.y;
		Pos.z += CurrentComponent->Mass * CurrentComponent->OriginalRelativePos.z;
	}

	Pos.x /= TotalMass;
	Pos.y /= TotalMass;
	Pos.z /= TotalMass;

	VehicleData->VehicleCenterOfMass = Pos;

#ifdef BETA_RELEASE
	if(VehicleObjectAnalysis && DisplayPhysics && IsFirstTic) // CG and Model Position
	{
		FLOAT64VECTOR WorldOriginOffset;
		FLOAT64VECTOR Pos1;
		piVECTOR V1;

		trnGetWorldOriginOffset(&WorldOriginOffset);

		Pos1 = VehicleData->VehiclePos;
		V1.x = (FLOAT32)(Pos1.x - WorldOriginOffset.x);
		V1.y = (FLOAT32)(Pos1.y - WorldOriginOffset.y);
		V1.z = (FLOAT32)(-Pos1.z - WorldOriginOffset.z);

		draw3dDebugMarkPoint(&V1, colorWHITEALPHA, 1);

	// let's do the model center point while we're at it
		Pos1 = VehicleData->AdjustedForCGPos;
		V1.x = (FLOAT32)(Pos1.x - WorldOriginOffset.x);
		V1.y = (FLOAT32)(Pos1.y - WorldOriginOffset.y);
		V1.z = (FLOAT32)(-Pos1.z - WorldOriginOffset.z);

		draw3dDebugMarkPoint(&V1, colorREDALPHA, 1);
	}
#endif

	return 0;
}

UINT32 mlCalcVehicleRotationalInertia(mlVehicleData* VehicleData)
{
	mlComponent* CurrentComponent;
	FLOAT64ATT Inertia;
	FLOAT64ATT ComponentInertia;
	FLOAT64 RadiusSquared;
	FLOAT64VECTOR Pos;
	mlComponentListIterator it;

	if (VehicleData == nullptr || VehicleData->ComponentList == nullptr)
		return 1;

	Inertia.Roll = Inertia.Pitch = Inertia.Yaw = 0;

	mlComponentListBegin(VehicleData->ComponentList, &it);
	for(; it.comp; mlComponentListNext(&it))
	{
		CurrentComponent = it.comp;

		// get the position relative to the current center of mass
		Pos = CurrentComponent->CurrentRelativePos;

		// roll inertia  (along y axis)
		RadiusSquared = Pos.x * Pos.x + Pos.z * Pos.z;

		Inertia.Roll += RadiusSquared * CurrentComponent->Mass;

		// pitch inertia (along x axis)
		RadiusSquared = Pos.y * Pos.y + Pos.z * Pos.z;

		Inertia.Pitch += RadiusSquared * CurrentComponent->Mass;

		// yaw inertia   (along z axis)
		RadiusSquared = Pos.y * Pos.y + Pos.x * Pos.x;

		Inertia.Yaw += RadiusSquared * CurrentComponent->Mass;

		// now add in the component's inertia
		ComponentInertia = CurrentComponent->RotInertia;

		Inertia.Roll  += ComponentInertia.Roll;
		Inertia.Pitch += ComponentInertia.Pitch;
		Inertia.Yaw   += ComponentInertia.Yaw;

	}

	VehicleData->VehicleRotationalInertia = Inertia;

#ifdef BETA_RELEASE
	if(VehicleObjectAnalysis && DisplayPhysics && IsFirstTic) // Moment of Inertia
	{
		FLOAT64VECTOR p1, p2;

		p1.x = p1.y = p1.z = 0;

		p2.x = Inertia.Pitch / Inertia.Yaw; p2.y = 0; p2.z = 0;
		draw3dGizmoLine(&p1, &p2, &VehicleData->VehicleAttMat, &VehicleData->VehiclePos, colorMAGENTAALPHA);

		p2.x = 0; p2.y = Inertia.Roll / Inertia.Yaw; p2.z = 0;
		draw3dGizmoLine(&p1, &p2, &VehicleData->VehicleAttMat, &VehicleData->VehiclePos, colorMAGENTAALPHA);

		p2.x = 0; p2.y = 0; p2.z = 1.0;
		draw3dGizmoLine(&p1, &p2, &VehicleData->VehicleAttMat, &VehicleData->VehiclePos, colorMAGENTAALPHA);
	}
#endif
	return 0;
}

static inline void TornadoCheck(FLOAT64VECTOR& val, FLOAT64VECTOR const& lastval)
{
	if(_isnan(val.x) || !_finite(val.x)) val.x = lastval.x;
	if(_isnan(val.y) || !_finite(val.y)) val.y = lastval.y;
	if(_isnan(val.z) || !_finite(val.z)) val.z = lastval.z;
};

static inline void TornadoCheckAtt(FLOAT64ATT& val, FLOAT64ATT const& lastval)
{
	if(_isnan(val.Pitch)|| !_finite(val.Pitch)) val.Pitch	= lastval.Pitch;
	if(_isnan(val.Roll) || !_finite(val.Roll))	val.Roll	= lastval.Roll;
	if(_isnan(val.Yaw)	|| !_finite(val.Yaw))	val.Yaw		= lastval.Yaw;
};

void _checkForTornados(mlVehicleData* data)
{
	TornadoCheck(data->VehicleVel, data->LastVehicleVel);
	TornadoCheck(data->VehicleAcc, data->LastVehicleAcc);

	FLOAT64VECTOR LastRealVel = {-data->VehicleVel.x, data->VehicleVel.y, -data->VehicleVel.z};
	TornadoCheck(data->VehicleRealVel, LastRealVel);

	TornadoCheckAtt(data->VehicleAttVel, data->LastVehicleAttVel);
	TornadoCheckAtt(data->VehicleAttAcc, data->LastVehicleAttAcc);

	TornadoCheck(data->VehicleForce, data->LastVehicleForce);
	TornadoCheckAtt(data->VehicleTorque, data->LastVehicleTorque);
}

UINT32 mlUpdateVehicle(mlVehicleData* VehicleData)
{
    FLOAT64ATT AngularMomentum = {0, 0, 0};
	UINT32 NumTics = 0;
	UINT32 i,j;
	SINT32 TicTime = 0;
	mlComponent * Component = nullptr;
	FLOAT64VECTOR TempVec = {0, 0, 0};
	piVECTOR TempPiVec = {0, 0, 0};
	FLOAT64VECTOR SavedVehicleForce = {0, 0, 0};
	FLOAT64VECTOR TempNormForceSum = {0, 0, 0};
	vehVehicle* VehiclePtr = nullptr;
	FLOAT64 DeltaTime = 0;
	mlComponentListIterator it;

	logTimerBeginMacro();

    _checkForTornados(VehicleData);

	_UpdateVehicleSounds(VehicleData);

	// check for first update
	if (VehicleData->LastVehicleUpdateTime == 0)
	{
		VehicleData->LastVehicleUpdateTime = frametime;
	}

	if (VehicleData->GetStartTime == 1)
	{
		VehicleData->GetStartTime = 0;
		VehicleData->SpawnTime = frametime;
		VehicleData->LastVehicleUpdateTime = frametime;
	}

	if (VehicleData->LastResetTime == 0)
	{
		mlResetVehiclePos(VehicleData);
		VehicleData->LastResetTime = frametime;
	}

	DeltaTime = frametime - VehicleData->LastVehicleUpdateTime;

	if (DeltaTime < 0.0)
	{
		DeltaTime = 0.0;
		VehicleData->LastVehicleUpdateTime = frametime;
		Write_Debug("Deltatime in modelling loop was negative!!!");
	}

	NumTics = (UINT32)(DeltaTime / _MS_PER_TIC);
	if (NumTics > _MAX_TICS)
	{
		NumTics = _MAX_TICS;
		DeltaTime = _MAX_TICS * _MS_PER_TIC;
		VehicleData->LastVehicleUpdateTime = (SINT32)((FLOAT64)frametime - DeltaTime);

		Write_Debug("Number of modelling loop tics exceeded %d for one frame",_MAX_TICS);
	}

	TicTime = (SINT32)_MS_PER_TIC;

	vehVehicle* playerVehicle = VehicleData->VehicleMasterPtr;

#ifdef GM_RELEASE
	if(SuperFloat || playerVehicle->VehicleType->Type == VEH_TYPE_FLOATER)
	{
		_DoEyeOperations(VehicleData);
		// update the terrain object collider bounding box list
		_UpdateBBoxList(VehicleData);
		VehicleData->VehicleLastPos = VehicleData->VehiclePos;
		VehicleData->LastVehicleUpdateTime = frametime;
		if (VehicleData->VehicleMasterPtr)
			vehUpdatePositionInfo((vehVehicle*)VehicleData->VehicleMasterPtr);

		return 0;
	}
#endif // GM_RELEASE

	//if(NumTics)
	//	_UpdateMovingParts(VehicleData,(FLOAT64)(NumTics*TicTime));

	VehiclePtr = VehicleData->VehicleMasterPtr;

	if(VehiclePtr->VehicleObjectData->ProcessContactPointObject)
		mlProcessContactPointObject(VehiclePtr->VehicleObjectData);

	if (frametime - (VehicleData->SpawnTime + _TIME_BEFORE_FRIENDLY_COLLISIONS_ON) > 0 &&
		VehicleData->SpawnTime != 0)
	{
		VehicleData->CollisionSide = 0;
	}
	else
	{
		VehicleData->CollisionSide = VehiclePtr->Side;
	}

	if (VehiclePtr->RideVehicleHostIndex == 0 &&
		VehicleData->LastRideVehicle > 0)
	{
		VehicleData->LastUnRideTime = frametime;
	}

	VehicleData->LastRideVehicle = VehiclePtr->RideVehicleHostIndex;

	if (frametime - (VehicleData->LastUnRideTime + _NO_COLLIDE_AFTER_UNRIDE_TIME) < 0 &&
		VehicleData->SpawnTime != 0)
	{
		VehicleData->CollisionSide = VehiclePtr->Side;
	}

	if (VehicleData->ComponentList != nullptr || VehiclePtr->VehicleType->Type == VEH_TYPE_PERSON)
	{
		IsFirstTic = TRUE;

		// Update Vehicle Components and forces
		for (i=0;i<NumTics;i++)
		{
			for (j=0;j<ML_MAX_DAMAGE_GROUPINGS;j++)
			{
				VehicleData->DamagePossible[j] = 0.0f;
				VehicleData->DamageCurrent[j] = 0.0f;
			}

			_NumTics++;

			VehicleData->NumTics++;

			_UpdateMovingParts(VehicleData,(FLOAT64)TicTime);

			// update the terrain object collider bounding box list
			_UpdateBBoxList(VehicleData);

			// save old vehicle data
			_checkForTornados(VehicleData);
			VehicleData->LastVehicleRotationalInertia = VehicleData->VehicleRotationalInertia;
			VehicleData->LastVehicleCenterOfMass = VehicleData->VehicleCenterOfMass;
			VehicleData->LastVehiclePos = VehicleData->VehiclePos;
			VehicleData->LastVehicleVel = VehicleData->VehicleVel;
			VehicleData->LastVehicleForce = VehicleData->VehicleForce;
			VehicleData->LastVehicleAtt = VehicleData->VehicleAtt;
			VehicleData->LastVehicleAttVel = VehicleData->VehicleAttVel;
			VehicleData->LastVehicleTorque = VehicleData->VehicleTorque;
			VehicleData->LastVehicleAcc = VehicleData->VehicleAcc;
			VehicleData->LastVehicleAttAcc = VehicleData->VehicleAttAcc;
            VehicleData->VehicleLastPos = VehicleData->VehiclePos;
			VehicleData->GearBeingDamaged = 0;
			VehicleData->FlapsBeingDamaged = 0;

			VehicleData->ShutdownEngines = 0;
			if(VehicleData->PhysicsOff)
				VehicleData->ShutdownEngines = 1;

			VehicleData->GroupAmmoCount = -1;
			VehicleData->MaxGroupAmmoCount = 0;
			VehicleData->BombSightAmmoType = nullptr;

			// calculate old rotational momentum
			// first recalculate rotational inertia to remove any mass changes from the rotational
			// momentum
			mlCalcVehicleRotationalInertia(VehicleData);

			// now calc the rotational momentum
			AngularMomentum.Roll  = VehicleData->LastVehicleAttVel.Roll *  VehicleData->VehicleRotationalInertia.Roll;
			AngularMomentum.Pitch = VehicleData->LastVehicleAttVel.Pitch*  VehicleData->VehicleRotationalInertia.Pitch;
			AngularMomentum.Yaw   = VehicleData->LastVehicleAttVel.Yaw 	*  VehicleData->VehicleRotationalInertia.Yaw;

			// calculate new center of mass and rotational inertia
			mlCalcVehicleMass(VehicleData);
			mlCalcVehicleCenterOfMass(VehicleData);
			mlSetVehicleComponentsRelativePos(VehicleData);
			mlCalcVehicleRotationalInertia(VehicleData);

			// adjust position for CG change
			// this is done so that player doesn't see the vehicle "move" when the
			// CG changes radically
			mathSubVectors(&TempVec, &VehicleData->VehicleCenterOfMass, &VehicleData->LastVehicleCenterOfMass);
			mathVectorMultiply64(&TempVec,&TempVec,&VehicleData->VehicleAttMat);

			VehicleData->VehiclePos.x += TempVec.x;
			VehicleData->VehiclePos.y += TempVec.y;
			VehicleData->VehiclePos.z += TempVec.z;

			// now apply conservation of angular momentum
            if(VehicleData->VehicleRotationalInertia.Roll != 0.0f)
            {
			    VehicleData->VehicleAttVel.Roll  = AngularMomentum.Roll / VehicleData->VehicleRotationalInertia.Roll;
            }

            if(VehicleData->VehicleRotationalInertia.Pitch != 0.0f)
            {
			    VehicleData->VehicleAttVel.Pitch = AngularMomentum.Pitch/ VehicleData->VehicleRotationalInertia.Pitch;
            }

            if(VehicleData->VehicleRotationalInertia.Yaw != 0.0f)
            {
                VehicleData->VehicleAttVel.Yaw 	 = AngularMomentum.Yaw 	/ VehicleData->VehicleRotationalInertia.Yaw;
            }

			// repeat the following on a fixed frequency loop
			// clear sum forces;
			VehicleData->VehicleForce.x 	= VehicleData->VehicleForce.y 		=  VehicleData->VehicleForce.z = 0;
			VehicleData->VehicleTorque.Roll = VehicleData->VehicleTorque.Pitch  =  VehicleData->VehicleTorque.Yaw = 0;

			VehicleData->VehicleRealAtt = VehicleData->VehicleAtt;
			VehicleData->VehicleRealAtt.Pitch = -VehicleData->VehicleRealAtt.Pitch;
			VehicleData->VehicleRealAtt.Yaw = -VehicleData->VehicleRealAtt.Yaw;

			VehicleData->CurrentPrimaryAmmoCount = 0;
			VehicleData->CurrentSecondaryAmmoCount = 0;
			VehicleData->PrimaryAmmoType[0] = '\0';
			VehicleData->SecondaryAmmoType[0] = '\0';
			VehicleData->PrimaryClipCount = 0;
			VehicleData->SecondaryClipCount = 0;
			VehicleData->PrimaryClipType = 0;
			VehicleData->SecondaryClipType = 0;

			// groundpounders are handled differently than mechanical vehicles
			if(VehicleData->VehicleMasterPtr->VehicleType->Type == VEH_TYPE_PERSON)
			{
				mlInfantryUpdate(VehicleData, TicTime);
			}
			else
			{
				// Pass 0 is for non-contact point stuff
				// pass 1 is to resolve friction on contact point stuff

				TempNormForceSum.x = TempNormForceSum.y = TempNormForceSum.z = 0;
				SavedVehicleForce.x = SavedVehicleForce.y = SavedVehicleForce.z = 0;

                extern void mlengineNoDupsOn();
                extern void mlengineNoDupsOff();

                mlengineNoDupsOn();

				// now compute component forces and torques and apply them to the whole
				mlComponentListBegin(VehicleData->ComponentList, &it);
				for(; it.comp; mlComponentListNext(&it))
				{
					Component = it.comp;

					mlUpdateComponent(Component,
									VehicleData,
									TicTime / _TIME_SCALE,
									0,
									&SavedVehicleForce,
									&TempNormForceSum);
				}
                mlengineNoDupsOff();


				if (VehicleData->VehicleObjectData != nullptr)
				{
					if (VehicleData->VehicleObjectData->LineCollideObject != nullptr)
					{
						if (VehicleData->MultiCrewPos < 2)
						{
							_DoLineCollision(VehicleData,
											 VehicleData->VehicleObjectData,
											 0,
											 &SavedVehicleForce,
											 &TempNormForceSum,
											 TicTime / _TIME_SCALE / 1000.0);
						}
					}

				}


				SavedVehicleForce = VehicleData->VehicleForce;
				SavedVehicleForce.x = -SavedVehicleForce.x;
				SavedVehicleForce.z = -SavedVehicleForce.z;

				mlComponentListBegin(VehicleData->ComponentList, &it);
				for(; it.comp; mlComponentListNext(&it))
				{
					Component = it.comp;

					mlUpdateComponent(Component,
									VehicleData,
									TicTime / _TIME_SCALE,
									1,
									&SavedVehicleForce,
									&TempNormForceSum);
				}

				if (VehicleData->VehicleObjectData != nullptr)
				{
					if (VehicleData->VehicleObjectData->LineCollideObject != nullptr)
					{

						if (VehicleData->MultiCrewPos < 2)
						{
							_DoLineCollision(VehicleData,
											 VehicleData->VehicleObjectData,
											 1,
											 &SavedVehicleForce,
											 &TempNormForceSum,
											 TicTime / _TIME_SCALE / 1000.0);
						}
					}

				}

				VehicleData->VehicleTorque.Roll = VehicleData->VehicleTorque.Roll;
				VehicleData->VehicleTorque.Pitch = VehicleData->VehicleTorque.Pitch;
				VehicleData->VehicleTorque.Yaw = VehicleData->VehicleTorque.Yaw;


				if (VehicleData->MultiCrewPos < 2)
				{

					if(!VehicleData->PhysicsOff)
					{
						_checkForTornados(VehicleData);
						// with the new vel and attvel, compute new position and attitude
						_AdjustVelAndRotVel(VehicleData, &VehicleData->VehicleVel,
											&VehicleData->VehicleAttVel,
											&VehicleData->VehicleAcc,
											&VehicleData->VehicleAttAcc,
											&VehicleData->VehicleForce,
											&VehicleData->VehicleTorque,
											VehicleData->VehicleMass,
											&VehicleData->VehicleRotationalInertia,
											TicTime / _TIME_SCALE, VehicleData->PhysicsVersion > 2);

						_AdjustPosAndAccel(&VehicleData->VehiclePos,
										   &VehicleData->VehicleAtt,
										   &VehicleData->VehicleVel,
										   &VehicleData->LastVehicleVel,
										   &VehicleData->VehicleAcc,
										   &VehicleData->LastVehicleAcc,
										   &VehicleData->VehicleAttVel,
										   &VehicleData->VehicleAttAcc,
										   &VehicleData->LastVehicleAttAcc,
										   TicTime / _TIME_SCALE);
					}

#ifdef BETA_RELEASE
	if(VehicleObjectAnalysis && DisplayPhysics && IsFirstTic) // Total Force and Torque
	{
		FLOAT64VECTOR p1, p2;
		piVECTOR V1, V2;

		p1.x = p1.y = p1.z = 0;
		mathVectorMultiply64(&p1,&p1,&VehicleData->VehicleAttMat);
		mathAddVectors(&p1,&p1,&VehicleData->LastVehiclePos); p1.z = -p1.z;
		p1.x -= WorldOriginOffset.x; p1.y -= WorldOriginOffset.y; p1.z -= WorldOriginOffset.z;
		V1.x = (FLOAT32)p1.x; V1.y = (FLOAT32)p1.y; V1.z = (FLOAT32)p1.z;

		// Total Force
		p2 = p1;
		p2.x += -VehicleData->VehicleForce.x / 1000.0; p2.y += VehicleData->VehicleForce.y / 1000.0; p2.z += VehicleData->VehicleForce.z / 1000.0;
		V2.x = (FLOAT32)p2.x; V2.y = (FLOAT32)p2.y; V2.z = (FLOAT32)p2.z;
		draw3dLineDebug(&V1, &V2, colorGREEN50 | colorBLACKALPHA);

		// Total Torque
		p2 = p1;
		TempVec.x = VehicleData->VehicleTorque.Pitch; TempVec.y = VehicleData->VehicleTorque.Roll; TempVec.z = VehicleData->VehicleTorque.Yaw;
		mathVectorMultiply64(&TempVec,&TempVec,&VehicleData->VehicleAttMat);
		p2.x += TempVec.x / 1000.0; p2.y += TempVec.y / 1000.0; p2.z += -TempVec.z / 1000.0;
		V2.x = (FLOAT32)p2.x; V2.y = (FLOAT32)p2.y; V2.z = (FLOAT32)p2.z;
		draw3dLineDebug(&V1, &V2, colorBLUE50 | colorBLACKALPHA);

		// Velocity Vector
		p2 = p1;
		p2.x += -VehicleData->VehicleVel.x / 10.0; p2.y += VehicleData->VehicleVel.y / 10.0; p2.z += VehicleData->VehicleVel.z / 10.0;
		V2.x = (FLOAT32)p2.x; V2.y = (FLOAT32)p2.y; V2.z = (FLOAT32)p2.z;
		draw3dLineDebug(&V1, &V2, colorORANGEALPHA);
	}
#endif
					if (VehicleData->VehicleMasterPtr->VehicleType->Type == VEH_TYPE_GROUND ||
						VehicleData->VehicleMasterPtr->VehicleType->Type == VEH_TYPE_SEA)
					{
						if (VehicleData->TowBarEnabled == FALSE)
						{
							mlDetermineVehicleCollisions(VehicleData,(TicTime / _TIME_SCALE) * 0.001);
						}
					}

					// check for riding passenger ejection
					{
					FLOAT64 Acc;

						Acc = mathExactVectorLength(&VehicleData->VehicleAcc);
						if (Acc > 100.0)
						{
							VehicleData->EjectRiders = TRUE;
							if (VehicleData->EjectionStartTime == 0 ||
								frametime - VehicleData->EjectionStartTime > 1000)
							{
								VehicleData->EjectionStartTime = frametime;
							}
						}
					}
					if (frametime - VehicleData->EjectionStartTime > 1000)
					{
						VehicleData->EjectRiders = FALSE;
					}

					if (VehicleData->EjectRiders == TRUE)
					{
						vehAnimChangeVehicleStateByName((vehVehicle*)(VehicleData->VehicleMasterPtr), (UCHAR*)"crashejectriders", TRUE);
					}
					else
					{
						vehAnimChangeVehicleStateByName((vehVehicle*)(VehicleData->VehicleMasterPtr), (UCHAR*)"crashejectriders", FALSE);
					}


					mathAddVectors(&VehicleData->SavedAcc,&VehicleData->SavedAcc,&VehicleData->VehicleAcc);
					VehicleData->AccCount++;
				}
				// sledgehammer method to get the AT guns stable
				if (VehicleData->UseKludgeStabilizer == 1)
				{
					VehicleData->VehicleAttVel.Pitch *= 0.9;
					VehicleData->VehicleAttVel.Roll *= 0.9;
				}


				// clear out damage que list
				if (VehicleData->NewDamage == 1)
				{
					for (j=0;j<VEH_MAX_DAMAGE_OBJECTS;j++)
					{
						VehicleData->DamageAccrued[j] = 0.0;
						VehicleData->NumRoundsHit[j] = 0;
					}
					VehicleData->NewDamage = 0;
//					mlAutopilotDisengage(VehicleData);
				}


			}

			if (VehicleData->FreezeVehicle == 1)
			{
				VehicleData->VehiclePos = VehicleData->LastVehiclePos;
				VehicleData->VehicleAtt = VehicleData->LastVehicleAtt;

				VehicleData->VehicleAttVel.Pitch = VehicleData->VehicleAttVel.Roll = VehicleData->VehicleAttVel.Yaw = 0.0;
				VehicleData->VehicleRealAttVel.Pitch = VehicleData->VehicleRealAttVel.Roll = VehicleData->VehicleRealAttVel.Yaw = 0.0;

				VehicleData->VehicleVel.x = VehicleData->VehicleVel.y = VehicleData->VehicleVel.z = 0.0;
				VehicleData->VehicleRealVel.x = VehicleData->VehicleRealVel.y = VehicleData->VehicleRealVel.z = 0;

				VehicleData->VehicleAcc.x = VehicleData->VehicleAcc.y = VehicleData->VehicleAcc.z = 0;
			}

			VehicleData->PrevVehicleUpdateTime = VehicleData->LastVehicleUpdateTime;
			VehicleData->LastVehicleUpdateTime = VehicleData->LastVehicleUpdateTime + TicTime;

			// translate the attitude and attvel into "real" attitudes
			//
			// basically these are the real attitudes, the other attitudes
			// are what makes the flight model work
			// figure that it would take 1 week or more to switch the flight model over
			// and frankly it's not worth the time as this should allow usage
			// of the vehicle's attitude for other purposes
			VehicleData->VehicleRealAtt = VehicleData->VehicleAtt;
			VehicleData->VehicleRealAtt.Pitch = -VehicleData->VehicleRealAtt.Pitch;
			VehicleData->VehicleRealAtt.Yaw = -VehicleData->VehicleRealAtt.Yaw;

			// convert attitude to matrix format
			TempPiVec.x = (FLOAT32)VehicleData->VehicleRealAtt.Pitch;
			TempPiVec.y = (FLOAT32)VehicleData->VehicleRealAtt.Roll;
			TempPiVec.z = (FLOAT32)VehicleData->VehicleRealAtt.Yaw;
			mathEul_ToMatrix(&VehicleData->VehicleAttMat,EulOrdXYZs,&TempPiVec);
			math3dInvert2(&VehicleData->VehicleAttInvMat,&VehicleData->VehicleAttMat);

			VehicleData->VehicleRealAttVel = VehicleData->VehicleAttVel;
			VehicleData->VehicleRealAttVel.Roll = -VehicleData->VehicleRealAttVel.Roll;
			VehicleData->VehicleRealAttVel.Yaw = -VehicleData->VehicleRealAttVel.Yaw;

			VehicleData->VehicleRealVel.x = -VehicleData->VehicleVel.x;
			VehicleData->VehicleRealVel.y = VehicleData->VehicleVel.y;
			VehicleData->VehicleRealVel.z = -VehicleData->VehicleVel.z;

			// correct 3d position for CG for the model
			TempVec = VehicleData->VehicleCenterOfMass;
			mathVectorMultiply64(&TempVec,&TempVec,&VehicleData->VehicleAttMat);

			VehicleData->AdjustedForCGPos = VehicleData->VehiclePos;
			VehicleData->AdjustedForCGPos.x -= TempVec.x;
			VehicleData->AdjustedForCGPos.y -= TempVec.y;
			VehicleData->AdjustedForCGPos.z -= TempVec.z;

			// do impulses down here I guess, can't make sense of the co-ordinate systems
			/*VehicleData->VehicleRealVel.x += VehicleData->VehicleLinearImpulse.x / VehicleData->VehicleMass;
			VehicleData->VehicleRealVel.y += VehicleData->VehicleLinearImpulse.y / VehicleData->VehicleMass;
			VehicleData->VehicleRealVel.z += VehicleData->VehicleLinearImpulse.z / VehicleData->VehicleMass;*/

			VehicleData->VehicleLinearImpulse.x  = VehicleData->VehicleLinearImpulse.y  = VehicleData->VehicleLinearImpulse.z  = 0;
			VehicleData->VehicleAngularImpulse.x = VehicleData->VehicleAngularImpulse.y = VehicleData->VehicleAngularImpulse.z = 0;

			VehicleData->LastCollisionTime = VehicleData->LastVehicleUpdateTime;

			// clear temporary state variables
			VehicleData->GearShift = 0;
			VehicleData->ReverseToggle = 0;

			// smoothing nonsense
			if (VehicleData->VehicleMasterPtr != nullptr)
				vehUpdatePositionInfo((vehVehicle*)VehicleData->VehicleMasterPtr);

			// check for position switch or ordinance cycle and weapon groups active
			{
				SINT32 TempNum;

				persGetPersonaPosition((UINT32*)&TempNum);

				if (VehicleData->CycleAmmo != 0 ||
					TempNum != VehicleData->LastPlayerPos)
				{
                    // We need to check to see if a PPO is active
					if (fpPPOGet() != nullptr)
					{
						fpPPOSwitchPPO(true);
					}
					else
					{
						if (TempNum != VehicleData->LastPlayerPos)
						{
							VehicleData->OrdnanceSelected = persGetPositionOrdnanceNum (TempNum);
						}
						else
						{
							VehicleData->OrdnanceSelected++;
							if (VehicleData->OrdnanceSelected > VehicleData->CrewData.NumberOfSecondaryWeapons[TempNum])
							{
							VehicleData->OrdnanceSelected = 0;
							}
						}
					}
				}

				persSetPositionOrdnanceNum (TempNum, VehicleData->OrdnanceSelected);
				VehicleData->LastPlayerPos = TempNum;
			}

			// overrides any ammo count stuff
			if(VehicleData->CrewData.SecondaryWeaponMsg[VehicleData->LastPlayerPos][VehicleData->OrdnanceSelected][0])
			{
				/*if (VehicleData->GroupAmmoCount != -1)
				{
					VehicleData->CurrentClipCount = VehicleData->GroupAmmoCount;

					if (VehicleData->GroupAmmoCount == 0)
					{
						VehicleData->CurrentClipCount = 99998;
					}
				}
				else
				{
					VehicleData->CurrentClipCount = 99999;
				}*/
				strcpy(VehicleData->SecondaryAmmoType,VehicleData->CrewData.SecondaryWeaponMsg[VehicleData->LastPlayerPos][VehicleData->OrdnanceSelected]);
//				VehicleData->ClipIsMulti = 99999;
			}

			VehicleData->CycleAmmo = 0;
			VehicleData->LastKillCrew = VehicleData->KillCrew;
			VehicleData->KillCrew = FALSE;
			VehicleData->MasterStarterOn = 0;

			IsFirstTic = FALSE;

			if(EZ_MGR)
				EZ_MGR->TicUpdate((TicTime / _TIME_SCALE) * 0.001f);
		}
	}

	// handle hoisting and riding
	if(VehiclePtr->RideVehicleHostIndex > 0 && VehiclePtr->RideSeatNum != VEH_TOWED_VEHICLE)
		mlVehicleUpdateRide(VehicleData);

	// check for group damaged component vehicle kill
	for(int i = 0; i < MAX_SECONDARY_GROUPS; i++)
	{
		if (VehicleData->TotalSecondaryCriticalComponentCount[i] > 0)
		{
			if (VehicleData->SecondaryCriticalComponentCount[i] >=
				VehicleData->TotalSecondaryCriticalComponentCount[i])
			{
#ifdef BETA_RELEASE
				if(!VehicleData->VehicleIsDead)
					uiChatPrintLine("Vehicle Killed by critical secondary %d", i+1);
#endif
				VehicleData->VehicleIsDead = 1;
				break;
			}
		}
	}

	// check for underground vehicles
	if (!gCustomTerrain && VehicleData->MultiCrewPos < 2 && VehicleData->FreezeVehicle != 1)
	{
		piVECTOR TempVec1, TempVec2;

		TempVec1.x = (FLOAT32)(VehicleData->VehiclePos.x - WorldOriginOffset.x);
		TempVec1.y = (FLOAT32)(VehicleData->VehiclePos.y - WorldOriginOffset.y);
		TempVec1.z = (FLOAT32)(-VehicleData->VehiclePos.z);

		TempVec2 = TempVec1;
		TempVec2.z += 30000.0f;
		TempVec1.z -= 20.0f;

        trncdIgnoreTreeCollide(TRUE);

		if (trncdCollideDownMediumList(&TempVec1,&TempVec2) == FALSE)
		{
			// before we declare the crew dead, lets make sure there's something to collide with
			TempVec1.z -= 30000.0f;
			if (trncdCollideDownMediumList(&TempVec1,&TempVec2) == FALSE)
			{
				// only kill if below sea level
				if (VehicleData->VehiclePos.z < -100.0)
				{
					if(VehiclePtr->VehicleType->Type != VEH_TYPE_SEA || VehicleData->VehiclePos.z < -1000.0)
						VehicleData->FreezeVehicle = 1;

					persKillCrew();

					if(!VehicleData->VehicleIsDead)
					{
						vehAnimChangeVehicleStateByName(VehicleData->VehicleMasterPtr, (UCHAR*)"crashexplosion", TRUE);
						vehVisDamageHulkVehicle(VehicleData->VehicleMasterPtr, TRUE);
						VehicleData->VehicleIsDead = TRUE;
					}
				}
			}
			else
			{
				if(VehiclePtr->VehicleType->Type != VEH_TYPE_SEA || VehicleData->VehiclePos.z < -1000.0)
					VehicleData->FreezeVehicle = 1;

				persKillCrew();

				if(!VehicleData->VehicleIsDead)
				{
					vehAnimChangeVehicleStateByName(VehicleData->VehicleMasterPtr, (UCHAR*)"crashexplosion", TRUE);
					vehVisDamageHulkVehicle(VehicleData->VehicleMasterPtr, TRUE);
					VehicleData->VehicleIsDead = TRUE;
				}
			}
		}

        trncdIgnoreTreeCollide(FALSE);
	}

    // Freeze vehicle if deployed and ground. Vehicle cannot move until undeployed
    if(VehiclePtr->VehicleType->Type == VEH_TYPE_GROUND)
    {
        if(vehmainIsDeployed() && VehicleData->BombBayDoor)
        {
            VehicleData->FreezeVehicle = 1;
        }
        else
        {
            VehicleData->FreezeVehicle = 0;
        }
    }

	// set vehicle G
	TempVec = VehicleData->VehicleAcc;
	TempVec.x = -TempVec.x;
	TempVec.z = -TempVec.z;
	TempVec.z += 9.8;
	mathVectorMultiply64(&TempVec,&TempVec,&VehicleData->VehicleAttInvMat);
	FLOAT32 TempNum = (FLOAT32)(TempVec.z / 9.8);
	persSetVehicleG(TempNum,(vehVehicle*)VehicleData->VehicleMasterPtr);
	VehicleData->VehicleG = TempNum;

	if(VehicleData->HasFireDirector)
		mlVehicleHandleTurretConvergence(VehicleData);

	// tread dust, may do this differently in the future
	if(VehiclePtr->VehicleType->Type == VEH_TYPE_GROUND ||
       VehiclePtr->VehicleType->Type == VEH_TYPE_SEA)
	{
		FLOAT64VECTOR vel = VehicleData->VehicleVel;
		FLOAT64 speed2 = vel.x * vel.x + vel.y * vel.y + vel.z * vel.z;
		LOGICAL state = 0;

        static WEAPON_SPEC gun = NULL_WEAP_SPEC;

        static LOGICAL firsttime = 1;

        if(firsttime)
        {
            firsttime = 0;

            // Add others here
            gun = weapCreateSpec("GV_gun.1");
        }


        if(VehiclePtr->VehicleType->Type == VEH_TYPE_SEA)
        {
            state = (VehiclePtr->RideVehicleHostIndex > 0) ? 0 : ((speed2 > 2.0) ? 1 : 0);
        }
        else
        {
            if(VehiclePtr->WeaponSerial.spec.wClass == gun.wClass)
                state = 0;
            else
                state = (VehiclePtr->RideVehicleHostIndex > 0) ? 0 : ((speed2 > 2.0) ? 1 : 0);
        }

		for(UCHAR StateNum = 0; StateNum < VEH_MAX_STATE_BITS; StateNum++)
		{
			vehanimVehicleStateType* stateType = &VehiclePtr->VehicleType->StateType[StateNum];
			if(stateType->StateType == VehAnim::STATE_TYPE_SMOKE && stateType->StateSubType == VehAnim::STATE_SUBTYPE_TREADDUST)
				vehAnimChangeVehicleStateByNumber(VehiclePtr, StateNum, state);
		}
	}

	if(VehiclePtr->VehicleType->Type == VEH_TYPE_AIR)
		_HandleContactDamage(VehiclePtr);

	logTimerEndMacro();

#ifdef BETA_RELEASE
	_HandleVehicleAnalysisStuff(VehicleData);
#endif

	return 0;
}

#define RAD_CAP 4.2

UINT32 _AdjustVelAndRotVel(mlVehicleData* VehicleData, FLOAT64VECTOR* Vel,
								   FLOAT64ATT* AttVel,
								   FLOAT64VECTOR* Acc,
								   FLOAT64ATT* AttAcc,
								   FLOAT64VECTOR* Force,
								   FLOAT64ATT* Torque,
								   FLOAT64 Mass,
								   FLOAT64ATT* RotInertia,
								   FLOAT64 DeltaTime, LOGICAL UseCorrectPhysics)
{
FLOAT64VECTOR LastAcc;
FLOAT64ATT LastAttAcc;

	LastAcc = *Acc;
	LastAttAcc = *AttAcc;

	Acc->x = Force->x / Mass;
	Acc->y = Force->y / Mass;
	Acc->z = Force->z / Mass;

	if(UseCorrectPhysics)
	{ 	// still not completely correct since we don't have inertia tensor, but closer
		// co-ordinate system appears to be left handed so negative of ordinary euler equation
		AttAcc->Pitch = (Torque->Pitch - -(RotInertia->Roll  - RotInertia->Yaw  ) * AttVel->Roll  * AttVel->Yaw  ) / RotInertia->Pitch;
		AttAcc->Roll  = (Torque->Roll  - -(RotInertia->Yaw   - RotInertia->Pitch) * AttVel->Yaw   * AttVel->Pitch) / RotInertia->Roll;
		AttAcc->Yaw   = (Torque->Yaw   - -(RotInertia->Pitch - RotInertia->Roll ) * AttVel->Pitch * AttVel->Roll ) / RotInertia->Yaw;
	}else
	{
		AttAcc->Pitch = Torque->Pitch / RotInertia->Pitch;
		AttAcc->Roll  = Torque->Roll  / RotInertia->Roll;
		AttAcc->Yaw   = Torque->Yaw   / RotInertia->Yaw;
	}

	if(VehicleData->AutopilotOn)
	{
		if(mlAutopilotWithinLimits(VehicleData))
		{
			FLOAT64VECTOR dir = {0,1,0};
			mathVectorMultiply64(&dir,&dir,&VehicleData->VehicleAttMat);
			dir.x = -dir.x; // wtf?
			dir.z = 0; mathNormalize(&dir);
			Acc->z = 0;

			FLOAT64 yacc = mathVectDotProduct(&dir, Acc);
			mathScaleVector(Acc, &dir, yacc);

			LastAcc.x = Acc->x; LastAcc.y = Acc->y; LastAcc.z = Acc->z;

			AttAcc->Pitch = AttAcc->Roll = AttAcc->Yaw = 0;
			LastAttAcc.Pitch = LastAttAcc.Roll = LastAttAcc.Yaw = 0;

			AttVel->Pitch = AttVel->Roll = AttVel->Yaw = 0;
			Vel->z = 0;

			VehicleData->VehicleAtt.Roll = 0;
		}else
		{
			rnmainListLocText(IDE_LANG_UI,AUTOPILOT_OFF_LIMIT_ID);
			VehicleData->AutopilotOn = FALSE;
		}
	}

	Vel->x += (Acc->x + LastAcc.x) * 0.5 * (DeltaTime / 1000.0);
	Vel->y += (Acc->y + LastAcc.y) * 0.5 * (DeltaTime / 1000.0);
	Vel->z += (Acc->z + LastAcc.z) * 0.5 * (DeltaTime / 1000.0);

	AttVel->Pitch += (AttAcc->Pitch + LastAttAcc.Pitch) * 0.5 * (DeltaTime / 1000.0);
	AttVel->Roll  += (AttAcc->Roll  + LastAttAcc.Roll ) * 0.5 * (DeltaTime / 1000.0);
	AttVel->Yaw   += (AttAcc->Yaw   + LastAttAcc.Yaw  ) * 0.5 * (DeltaTime / 1000.0);

	// cheesy cap
	if(AttVel->Roll > RAD_CAP)
		AttVel->Roll = RAD_CAP;
	else if(AttVel->Roll < -RAD_CAP)
		AttVel->Roll = -RAD_CAP;

	if(AttVel->Pitch > RAD_CAP)
		AttVel->Pitch = RAD_CAP;
	else if(AttVel->Pitch < -RAD_CAP)
		AttVel->Pitch = -RAD_CAP;

	if(AttVel->Yaw > RAD_CAP)
		AttVel->Yaw = RAD_CAP;
	else if (AttVel->Yaw < -RAD_CAP)
		AttVel->Yaw = -RAD_CAP;

	return 0;
}

UINT32 _AdjustPosAndAccel(FLOAT64VECTOR* Pos,
								  FLOAT64ATT* Att,
								  FLOAT64VECTOR* Vel,
								  FLOAT64VECTOR* LastVel,
								  FLOAT64VECTOR* Acc,
								  FLOAT64VECTOR* LastAcc,
								  FLOAT64ATT* AttVel,
								  FLOAT64ATT* AttAcc,
								  FLOAT64ATT* LastAttAcc,
								  FLOAT64 DeltaTime)
{
FLOAT64ATT DeltaAtt;
FLOAT64VECTOR TempVec;
FLOAT64ATT TempAtt;


	TempVec.x = (Acc->x + LastAcc->x) * 0.5;
	TempVec.y = (Acc->y + LastAcc->y) * 0.5;
	TempVec.z = (Acc->z + LastAcc->z) * 0.5;


	Pos->x -= LastVel->x * DeltaTime / 1000.0 + 0.5 * TempVec.x * DeltaTime * DeltaTime / 1000.0 / 1000.0;
	Pos->y += LastVel->y * DeltaTime / 1000.0 + 0.5 * TempVec.y * DeltaTime * DeltaTime / 1000.0 / 1000.0;
	Pos->z -= LastVel->z * DeltaTime / 1000.0 + 0.5 * TempVec.z * DeltaTime * DeltaTime / 1000.0 / 1000.0;

	TempAtt.Pitch = (AttAcc->Pitch + LastAttAcc->Pitch) * 0.5;
	TempAtt.Roll = (AttAcc->Roll + LastAttAcc->Roll) * 0.5;
	TempAtt.Yaw = (AttAcc->Yaw + LastAttAcc->Yaw) * 0.5;

	DeltaAtt.Pitch = AttVel->Pitch * DeltaTime / 1000.0 + 0.5 * TempAtt.Pitch * DeltaTime * DeltaTime / 1000.0 / 1000.0;
	DeltaAtt.Roll = AttVel->Roll * DeltaTime / 1000.0 + 0.5 * TempAtt.Roll * DeltaTime * DeltaTime / 1000.0 / 1000.0;
	DeltaAtt.Yaw = AttVel->Yaw * DeltaTime / 1000.0 + 0.5 * TempAtt.Yaw * DeltaTime * DeltaTime / 1000.0 / 1000.0;

	mathRotateAttByAtt(Att,Att,&DeltaAtt);

	return 0;
}

// the following force is relative to the* vehicle*
UINT32 mlApplyStaticForceAtRelativePoint(mlVehicleData * VehicleData,
									     FLOAT64VECTOR * RelPoint,
									     FLOAT64VECTOR * ForceVec)
{
    FLOAT64VECTOR TorqueVector = {0, 0, 0};
    FLOAT64ATT Torque = {0, 0, 0};
    FLOAT64VECTOR RelativeForce = {0, 0, 0};

#ifdef BETA_RELEASE
	if(DisableForces) return 0;
#endif

	// first get relative force direction for torque calcs
	mathCRotateVectorByAtt(&RelativeForce,ForceVec,&VehicleData->VehicleAtt);

	// compute torque on whole
	// torque = distance x force
	mathVectDoCrossProduct(&TorqueVector,RelPoint, &RelativeForce);
	Torque.Pitch = TorqueVector.x;
	Torque.Roll = -TorqueVector.y;
	Torque.Yaw = TorqueVector.z;

	VehicleData->VehicleTorque.Pitch += Torque.Pitch;
	VehicleData->VehicleTorque.Roll += Torque.Roll;
	VehicleData->VehicleTorque.Yaw += Torque.Yaw;

	// now calc force upon center of mass
	// note: this is probably wrong if a significant part of the force goes into
	// rotation!
	VehicleData->VehicleForce.x -= ForceVec->x;
	VehicleData->VehicleForce.y -= ForceVec->y;
	VehicleData->VehicleForce.z -= ForceVec->z;

#ifdef BETA_RELEASE
	if(VehicleObjectAnalysis && DisplayPhysics && IsFirstTic) // Individual Force
	{
		FLOAT64VECTOR p1, p2;
		piVECTOR V1, V2;

		p1 = *RelPoint;
		mathVectorMultiply64(&p1,&p1,&VehicleData->VehicleAttMat);
		mathAddVectors(&p1,&p1,&VehicleData->LastVehiclePos); p1.z = -p1.z;
		p1.x -= WorldOriginOffset.x; p1.y -= WorldOriginOffset.y; p1.z -= WorldOriginOffset.z;
		V1.x = (FLOAT32)p1.x; V1.y = (FLOAT32)p1.y; V1.z = (FLOAT32)p1.z;

		p2 = p1;
		p2.x += ForceVec->x / 1000.0; p2.y += -ForceVec->y / 1000.0; p2.z += -ForceVec->z / 1000.0;
		V2.x = (FLOAT32)p2.x; V2.y = (FLOAT32)p2.y; V2.z = (FLOAT32)p2.z;
		draw3dLineDebug(&V1, &V2, colorGREENALPHA);
	}
#endif

	return 0;
}


// Input force is relative to the world
// positive x force pushes east
// positive y force pushes north
// positive z force pushes up
// RelPoint is the point relative to the vehicle that the force is applied
// x axis positive goes right
// y axis positive goes forward
// z axis positive goes up
static UINT32 mlApplyStaticForceAtRelativePointWithMat(mlVehicleData * VehicleData,
													FLOAT64VECTOR * InRelPoint,
													FLOAT64VECTOR * InForceVec)
{
	FLOAT64VECTOR TorqueVector;
	FLOAT64ATT Torque;
	FLOAT64VECTOR RelativeForce;
	FLOAT64VECTOR ForceVec;
	FLOAT64VECTOR RelPoint;

#ifdef BETA_RELEASE
	if(DisableForces) return 0;
#endif

	RelPoint = *InRelPoint;
	ForceVec = *InForceVec;

	// first get relative force direction for torque calcs
	mathVectorMultiply64(&RelativeForce,&ForceVec,&VehicleData->VehicleAttInvMat);

	// compute torque on whole
	// torque = distance x force
	mathVectDoCrossProduct(&TorqueVector,&RelPoint, &RelativeForce);
	Torque.Pitch = TorqueVector.x;
	Torque.Roll = -TorqueVector.y;
	Torque.Yaw = TorqueVector.z;

	VehicleData->VehicleTorque.Pitch += Torque.Pitch;
	VehicleData->VehicleTorque.Roll += Torque.Roll;
	VehicleData->VehicleTorque.Yaw += Torque.Yaw;

	// now calc force upon center of mass
	// note: this is probably wrong if a significant part of the force goes into
	// rotation!
	VehicleData->VehicleForce.x -= ForceVec.x;
	VehicleData->VehicleForce.y += ForceVec.y;
	VehicleData->VehicleForce.z -= ForceVec.z;

#ifdef BETA_RELEASE
	if(VehicleObjectAnalysis && DisplayPhysics && IsFirstTic) // Individual Force
	{
		FLOAT64VECTOR p1, p2;
		piVECTOR V1, V2;

		p1 = RelPoint;
		mathVectorMultiply64(&p1,&p1,&VehicleData->VehicleAttMat);
		mathAddVectors(&p1,&p1,&VehicleData->LastVehiclePos); p1.z = -p1.z;
		p1.x -= WorldOriginOffset.x; p1.y -= WorldOriginOffset.y; p1.z -= WorldOriginOffset.z;
		V1.x = (FLOAT32)p1.x; V1.y = (FLOAT32)p1.y; V1.z = (FLOAT32)p1.z;

		p2 = p1;
		p2.x += ForceVec.x / 1000.0; p2.y += ForceVec.y / 1000.0; p2.z += -ForceVec.z / 1000.0;
		V2.x = (FLOAT32)p2.x; V2.y = (FLOAT32)p2.y; V2.z = (FLOAT32)p2.z;
		draw3dLineDebug(&V1, &V2, colorGREENALPHA);
	}
#endif

	return 0;
}


UINT32 mlApplyTorqueAtRelativePoint(mlVehicleData * VehicleData,
									FLOAT64VECTOR * RelPoint,
									FLOAT64ATT * BaseTorque,
									FLOAT64ATT * TorqueAtt, LOGICAL UseCorrectPhysics)
{
	FLOAT64ATT RotInertiaAtPoint;
	FLOAT64ATT TorqueAtCM;
	FLOAT64ATT Torque;
	FLOAT64VECTOR TempVec;

#ifdef BETA_RELEASE
	if(DisableForces) return 0;
#endif

	TempVec.x = BaseTorque->Pitch;
	TempVec.y = BaseTorque->Roll;
	TempVec.z = BaseTorque->Yaw;

	// first translate the base torque by the attitude
	mathRotateVectorByAtt(&TempVec,&TempVec,TorqueAtt);

	Torque.Pitch = TempVec.x;
	Torque.Roll = TempVec.y;
	Torque.Yaw = TempVec.z;

	if(UseCorrectPhysics)
	{
		VehicleData->VehicleTorque.Pitch += Torque.Pitch;
		VehicleData->VehicleTorque.Roll += Torque.Roll;
		VehicleData->VehicleTorque.Yaw += Torque.Yaw;
	}else // THIS IS UTTERLY BOGUS
	{
		// torque at center of mass is the ratio of the rotational inertias of the
		// torque point and the center of mass or,
		//
		//	tp   =    tcm
		//  ---      -----
		//  RIp       RIcm
		//
		// where tp = torque at point, tcm = torque at center of mass
		// RIp = Rotational Inertia at point and
		// RIcm is the rotational inertia at the center of mass
		//
		// first calc RIcm
		// this is distance * mass + RIcm
		RotInertiaAtPoint = VehicleData->VehicleRotationalInertia;
		RotInertiaAtPoint.Pitch += sqrt(RelPoint->y * RelPoint->y + RelPoint->z * RelPoint->z) *
									VehicleData->VehicleMass;
		RotInertiaAtPoint.Roll += sqrt(RelPoint->x * RelPoint->x + RelPoint->z * RelPoint->z) *
									VehicleData->VehicleMass;
		RotInertiaAtPoint.Yaw += sqrt(RelPoint->x * RelPoint->x + RelPoint->y * RelPoint->y) *
									VehicleData->VehicleMass;

		// now calc torque based on ratio of two inertias
		if (RotInertiaAtPoint.Pitch != 0)
		{
			TorqueAtCM.Pitch = Torque.Pitch * (VehicleData->VehicleRotationalInertia.Pitch /
												RotInertiaAtPoint.Pitch);
		}
		else
		{
			TorqueAtCM.Pitch = 0;
		}

		if (RotInertiaAtPoint.Yaw != 0)
		{
			TorqueAtCM.Roll  = Torque.Roll  * (VehicleData->VehicleRotationalInertia.Roll /
												RotInertiaAtPoint.Roll);
		}
		else
		{
			TorqueAtCM.Roll = 0;
		}

		if (RotInertiaAtPoint.Roll != 0)
		{
			TorqueAtCM.Yaw   = Torque.Yaw   * (VehicleData->VehicleRotationalInertia.Yaw /
												RotInertiaAtPoint.Yaw);
		}
		else
		{
			TorqueAtCM.Yaw = 0;
		}

		VehicleData->VehicleTorque.Pitch += TorqueAtCM.Pitch;
		VehicleData->VehicleTorque.Roll += TorqueAtCM.Roll;
		VehicleData->VehicleTorque.Yaw += TorqueAtCM.Yaw;

		Torque = TorqueAtCM;
	}

#ifdef BETA_RELEASE
	if(VehicleObjectAnalysis && DisplayPhysics && IsFirstTic) // Individual Torque
	{
		FLOAT64VECTOR p1, p2;
		piVECTOR V1, V2;

		p1 = *RelPoint;
		mathVectorMultiply64(&p1,&p1,&VehicleData->VehicleAttMat);
		mathAddVectors(&p1,&p1,&VehicleData->LastVehiclePos); p1.z = -p1.z;
		p1.x -= WorldOriginOffset.x; p1.y -= WorldOriginOffset.y; p1.z -= WorldOriginOffset.z;
		V1.x = (FLOAT32)p1.x; V1.y = (FLOAT32)p1.y; V1.z = (FLOAT32)p1.z;

		TempVec.x = Torque.Pitch; TempVec.y = Torque.Roll; TempVec.z = Torque.Yaw;
		mathVectorMultiply64(&TempVec,&TempVec,&VehicleData->VehicleAttMat);

		p2 = p1;
		p2.x += TempVec.x / 1000.0; p2.y += TempVec.y / 1000.0; p2.z += -TempVec.z / 1000.0;
		V2.x = (FLOAT32)p2.x; V2.y = (FLOAT32)p2.y; V2.z = (FLOAT32)p2.z;
		draw3dLineDebug(&V1, &V2, colorBLUEALPHA);
	}
#endif

	return 0;
}

// returns velocity relative to the component
UINT32 mlCompCalcComponentVelocityVector(FLOAT64VECTOR* ReturnVel,
										 mlComponent * Component,
										 mlVehicleData * VehicleData)
{
FLOAT64VECTOR Velocity;
FLOAT64VECTOR Pos1,Pos2;
FLOAT64ATT DeltaAtt;
FLOAT64VECTOR VehicleVelocity;

	// first calculate the component's rotational velocity vector
	// we will use a "cheap" trick to figure this out
	// basically we will do a 1/25th second rotation of the point based
	// on the vehicle's velocity and then use the two positions to calc a velocity
	// the idea of using such a short segment is to minimize arc errors
	//
	// the "right" way to do this is to do this same proceedure, figure out
	// the exact angle difference between the two, and
	// apply the angle * radius rule to figure out the exact distance travelled
	// since none of the objects forseen will be spinning at rates larger than
	// about 15 times a second we should be ok
	Pos1 = Pos2 = Component->CurrentRelativePos;
	Pos1.x = -Pos1.x;
	Pos2.x = -Pos2.x;
	DeltaAtt.Pitch = VehicleData->VehicleAttVel.Pitch / 100.0;
	DeltaAtt.Roll = VehicleData->VehicleAttVel.Roll / 100.0;
	DeltaAtt.Yaw = VehicleData->VehicleAttVel.Yaw / 100.0;
	mathRotateVectorByAtt(&Pos2,&Pos2,&DeltaAtt);
	mathSubVectors(&Velocity,&Pos2,&Pos1);
	Velocity.x *= 100.0;
	Velocity.y *= 100.0;
	Velocity.z *= 100.0;

	// now rotate this velocity by the component's attitude
	mathCRotateVectorByAtt(&Velocity,&Velocity,&Component->Attitude);


	// now take the vehicle's velocity
	// and rotate it to the vehicle's frame of reference
	mathCRotateVectorByAtt(&VehicleVelocity,&VehicleData->VehicleVel,&VehicleData->VehicleAtt);

	// rotate it to the component's attitude
	mathCRotateVectorByAtt(&VehicleVelocity,&VehicleVelocity,&Component->Attitude);


	// finally add the two together
	ReturnVel->x = VehicleVelocity.x + Velocity.x;
	ReturnVel->y = VehicleVelocity.y + Velocity.y;
	ReturnVel->z = VehicleVelocity.z + Velocity.z;
//	*ReturnVel = VehicleVelocity;

	return 0;
}

void mlResetPositionState()
{
	// add stuff here to reset when a position change happens
	mlPrimaryTriggerDown(0, IMAP_HOLD_MAINTRIGGER, 0);
	mlSecondaryTriggerDown(0, IMAP_HOLD_SECONDTRIGGER, 0);
}


void mlResetControlStuff()
{
	_StickXPos = 0.0;
	_StickYPos = 0.0;
	_RudderPos = 0.0;
	_ThrottlePos = 0.0;
}

UINT32 _ProcessControlInputs(UINT32 DeltaTime,mlVehicleData * VehicleData)
{

FLOAT64 TempNum;
FLOAT64 Speed;
FLOAT64 Reduction;
FLOAT64 SpeedOfSound;
FLOAT64 MachNum;
FLOAT64 DensityRatio;
FLOAT64 TempNum1,TempNum2;
FLOAT64 Response;
FLOAT64 Delta;
FLOAT64 ForceFullDeflection,RealForceFullDeflection;
FLOAT64 TabForce,SurfaceForce;
FLOAT64 MaxSpeed;

if(VehicleData->VehicleMasterPtr->VehicleType->Type == VEH_TYPE_AIR)
{
	mlGetAltDensity(&TempNum1,0);
	mlGetAltDensity(&TempNum2,VehicleData->VehiclePos.z);
	DensityRatio = TempNum1 / TempNum2;
	Speed = mathExactVectorLength(&VehicleData->VehicleVel);
	if (Speed < 40.0)
	{
		Speed = 40.0;
	}
	mlGetAltSpeedOfSound(&SpeedOfSound,VehicleData->VehiclePos.z);
	MachNum = Speed / SpeedOfSound;


	// Elevator

	if (_StickYPos < 0)
	{
		TempNum = -_StickYPos * _StickYPos;
	}
	else
	{
		TempNum = _StickYPos * _StickYPos;
	}

	// now compensate stick position for airspeed and compressibility
	// first airspeed

	if (Speed > VehicleData->ElevatorMaxSpeed * DensityRatio)
	{
		Reduction = (VehicleData->ElevatorMaxSpeed * VehicleData->ElevatorMaxSpeed *
					 DensityRatio) /
					(Speed * Speed);
		TempNum *= Reduction;
		if (TempNum > 1.0)
		{
			TempNum = 1.0;
		}
		if (TempNum < -1.0)
		{
			TempNum = -1.0;
		}
	}

	if (DeltaTime > 0.0)
	{
		// limit movement by response rate for speed;
		ForceFullDeflection = (VehicleData->ElevatorMaxSpeed * VehicleData->ElevatorMaxSpeed *
						 DensityRatio) /
						(Speed * Speed);

		Response = ForceFullDeflection / VehicleData->ElevatorResponseTime;
		if ((VehicleData->ElevatorBasePos > 0.0 && TempNum > VehicleData->ElevatorBasePos) ||
			(VehicleData->ElevatorBasePos < 0.0 && TempNum < VehicleData->ElevatorBasePos))
		{
			// the further the deflection, the harder it is to move the controls
			Response *= (1.0 - VehicleData->ElevatorBasePos * VehicleData->ElevatorBasePos);
		}
		else
		{
			// going back to neutral is easier the more the deflection
			Response *= (VehicleData->ElevatorBasePos * VehicleData->ElevatorBasePos + 1.0);
		}
		Response = Response * DeltaTime / 1000.0;
		Delta = TempNum - VehicleData->ElevatorBasePos;
		if (fabs(Delta) > Response)
		{
			if (Delta < 0)
			{
				TempNum = VehicleData->ElevatorBasePos - Response;
			}
			else
			{
				TempNum = VehicleData->ElevatorBasePos + Response;
			}
		}
		// now compensate for compressiblity
		if (MachNum > VehicleData->ElevatorCompressSpeed)
		{
			TempNum *= 0.1; // just a guess but should work for government work
		}
		VehicleData->ElevatorPos = TempNum;
		VehicleData->ElevatorBasePos = TempNum;

		MaxSpeed = VehicleData->ElevatorMaxSpeed;
		// now adjust "center" for tab position
		if (fabs(Speed) > 0)
		{
			RealForceFullDeflection = Speed * Speed * DensityRatio /
										(MaxSpeed * MaxSpeed);

			TabForce = RealForceFullDeflection *
							VehicleData->ElevatorTabRatio *
							(VehicleData->ElevatorTabPosition +
							 VehicleData->ElevatorTabStatic);


			// determine force that the current position of the control surface requires
			SurfaceForce = RealForceFullDeflection * VehicleData->ElevatorPos;

			// now adjust position to the sum of the SurfaceForce and TabForce
			VehicleData->ElevatorPos = (TabForce + SurfaceForce) / RealForceFullDeflection;
		}

		// determine relative force to fully deflect control surface
		if (fabs(Speed) > 0)
		{
			RealForceFullDeflection = Speed * Speed * DensityRatio /
										(MaxSpeed * MaxSpeed);

			// determine force tab exerts
			TabForce = RealForceFullDeflection *
							VehicleData->ElevatorSpringTabRatio *
							_StickYPos;

			// determine force that the current position of the control surface requires
			SurfaceForce = RealForceFullDeflection * VehicleData->ElevatorPos;

			// now adjust position to the sum of the SurfaceForce and TabForce
			VehicleData->ElevatorPos = (TabForce + SurfaceForce) / RealForceFullDeflection;
		}

		// now check to ensure aileron hasn't exceeded bounds

		if (VehicleData->ElevatorPos > 1.0)
		{
			VehicleData->ElevatorPos = 1.0;
		}

		if (VehicleData->ElevatorPos < -1.0)
		{
			VehicleData->ElevatorPos = -1.0;
		}
	}

	// Rudder
	if (_RudderPos < 0)
	{
		TempNum = -_RudderPos * _RudderPos;
	}
	else
	{
		TempNum = _RudderPos * _RudderPos;
	}

	// now compensate stick position for airspeed and compressibility
	// first airspeed
	if (Speed > VehicleData->RudderMaxSpeed * DensityRatio)
	{
		Reduction = (VehicleData->RudderMaxSpeed * VehicleData->RudderMaxSpeed *
					 DensityRatio) /
					(Speed * Speed);
		TempNum *= Reduction;
		if (TempNum > 1.0)
		{
			TempNum = 1.0;
		}
		if (TempNum < -1.0)
		{
			TempNum = -1.0;
		}
	}

	if (DeltaTime > 0.0)
	{
		// limit movement by response rate for speed;
		ForceFullDeflection = (VehicleData->RudderMaxSpeed * VehicleData->RudderMaxSpeed *
						 DensityRatio) /
						(Speed * Speed);

		Response = ForceFullDeflection / VehicleData->RudderResponseTime;
		if ((VehicleData->RudderBasePos > 0.0 && TempNum > VehicleData->RudderBasePos) ||
			(VehicleData->RudderBasePos < 0.0 && TempNum < VehicleData->RudderBasePos))
		{
			// the further the deflection, the harder it is to move the controls
			Response *= (1.0 - VehicleData->RudderBasePos * VehicleData->RudderBasePos);
		}
		else
		{
			// going back to neutral is easier the more the deflection
			Response *= (VehicleData->RudderBasePos * VehicleData->RudderBasePos + 1.0);
		}
		Response = Response * DeltaTime / 1000.0;
		Delta = TempNum - VehicleData->RudderBasePos;
		if (fabs(Delta) > Response)
		{
			if (Delta < 0)
			{
				TempNum = VehicleData->RudderBasePos - Response;
			}
			else
			{
				TempNum = VehicleData->RudderBasePos + Response;
			}
		}
		// now compensate for compressiblity
		if (MachNum > VehicleData->RudderCompressSpeed)
		{
			TempNum *= 0.1; // just a guess but should work for government work
		}
		VehicleData->RudderPos = TempNum;
		VehicleData->RudderBasePos = TempNum;

		MaxSpeed = VehicleData->RudderMaxSpeed;
		// now adjust "center" for tab position
		if (fabs(Speed) > 0)
		{
			RealForceFullDeflection = Speed * Speed * DensityRatio /
										(MaxSpeed * MaxSpeed);

			TabForce = RealForceFullDeflection *
							VehicleData->RudderTabRatio *
							(VehicleData->RudderTabPosition +
							 VehicleData->RudderTabStatic);

			// determine force that the current position of the control surface requires
			SurfaceForce = RealForceFullDeflection * VehicleData->RudderPos;

			// now adjust position to the sum of the SurfaceForce and TabForce
			VehicleData->RudderPos = (TabForce + SurfaceForce) / RealForceFullDeflection;
		}

		// determine relative force to fully deflect control surface
		if (fabs(Speed) > 0)
		{
			RealForceFullDeflection = Speed * Speed * DensityRatio /
										(MaxSpeed * MaxSpeed);

			// determine force tab exerts
			TabForce = RealForceFullDeflection *
							VehicleData->RudderSpringTabRatio *
							_RudderPos;

			// determine force that the current position of the control surface requires
			SurfaceForce = RealForceFullDeflection * VehicleData->RudderPos;

			// now adjust position to the sum of the SurfaceForce and TabForce
			VehicleData->RudderPos = (TabForce + SurfaceForce) / RealForceFullDeflection;
		}

		// now check to ensure aileron hasn't exceeded bounds

		if (VehicleData->RudderPos > 1.0)
		{
			VehicleData->RudderPos = 1.0;
		}

		if (VehicleData->RudderPos < -1.0)
		{
			VehicleData->RudderPos = -1.0;
		}
	}

	// aileron

	if (_StickXPos < 0)
	{
		TempNum = - _StickXPos * _StickXPos;
	}
	else
	{
		TempNum = _StickXPos * _StickXPos;
	}

	// now compensate stick position for airspeed and compressibility
	// first airspeed
	MaxSpeed = VehicleData->AileronMaxSpeed;

	// adjust maxspeed for aileron tab effects
	// this is to ensure that the maximum deflection speed is the specified speed
	// regardless of tab activity

	if (Speed > MaxSpeed * DensityRatio)
	{
		Reduction = (MaxSpeed * MaxSpeed *
					 DensityRatio) /
					(Speed * Speed);
		TempNum *= Reduction;
		if (TempNum > 1.0)
		{
			TempNum = 1.0;
		}
		if (TempNum < -1.0)
		{
			TempNum = -1.0;
		}
	}

	if (DeltaTime > 0.0)
	{
		// limit movement by response rate for speed;
		ForceFullDeflection = (MaxSpeed * MaxSpeed *
						 DensityRatio) /
						(Speed * Speed);

		Response = ForceFullDeflection / VehicleData->AileronResponseTime;
		if ((VehicleData->AileronBasePos > 0.0 && TempNum > VehicleData->AileronBasePos) ||
			(VehicleData->AileronBasePos < 0.0 && TempNum < VehicleData->AileronBasePos))
		{
			// the further the deflection, the harder it is to move the controls
			Response *= (1.0 - VehicleData->AileronBasePos * VehicleData->AileronBasePos);
		}
		else
		{
			// going back to neutral is easier the more the deflection
			Response *= (VehicleData->AileronBasePos * VehicleData->AileronBasePos + 1.0);
		}
		Response = Response * DeltaTime / 1000.0;
		Delta = TempNum - VehicleData->AileronBasePos;
		if (fabs(Delta) > Response)
		{
			if (Delta < 0)
			{
				TempNum = VehicleData->AileronBasePos - Response;
			}
			else
			{
				TempNum = VehicleData->AileronBasePos + Response;
			}
		}
		// now compensate for compressiblity
		if (MachNum > VehicleData->AileronCompressSpeed)
		{
			TempNum *= 0.1; // just a guess but should work for government work
		}
		VehicleData->AileronPos = TempNum;
		VehicleData->AileronBasePos = TempNum;

		// now adjust "center" for tab position
		if (fabs(Speed) > 0)
		{
			RealForceFullDeflection = Speed * Speed * DensityRatio /
										(MaxSpeed * MaxSpeed);

			TabForce = RealForceFullDeflection *
							VehicleData->AileronTabRatio *
							(VehicleData->AileronTabPosition +
							 VehicleData->AileronTabStatic);


			// determine force that the current position of the control surface requires
			SurfaceForce = RealForceFullDeflection * VehicleData->AileronPos;

			// now adjust position to the sum of the SurfaceForce and TabForce
			VehicleData->AileronPos = (TabForce + SurfaceForce) / RealForceFullDeflection;
		}

		// determine relative force to fully deflect control surface
		if (fabs(Speed) > 0)
		{
			RealForceFullDeflection = Speed * Speed * DensityRatio /
										(MaxSpeed * MaxSpeed);

			// determine force tab exerts
			TabForce = RealForceFullDeflection *
							VehicleData->AileronSpringTabRatio *
							_StickXPos;

			// determine force that the current position of the control surface requires
			SurfaceForce = RealForceFullDeflection * VehicleData->AileronPos;

			// now adjust position to the sum of the SurfaceForce and TabForce
			VehicleData->AileronPos = (TabForce + SurfaceForce) / RealForceFullDeflection;
		}

//		VehicleData->AileronPos += TabForce / ForceFullDeflection;

		// now check to ensure aileron hasn't exceeded bounds

		if (VehicleData->AileronPos > 1.0)
		{
			VehicleData->AileronPos = 1.0;
		}

		if (VehicleData->AileronPos < -1.0)
		{
			VehicleData->AileronPos = -1.0;
		}

	}
}
	TempNum = _ThrottlePos;

	if (DeltaTime > 0.0)
	{
		if(VehicleData->ThrottleResponseTime > 0)
		{
			Response = 1.0 / VehicleData->ThrottleResponseTime;
			Response = Response * DeltaTime / 1000.0;
			Delta = TempNum - VehicleData->ThrottlePos;
			if (fabs(Delta) > Response)
			{
				if (Delta < 0)
				{
					TempNum = VehicleData->ThrottlePos - Response;
				}
				else
				{
					TempNum = VehicleData->ThrottlePos + Response;
				}
			}
		}
		VehicleData->ThrottlePos = TempNum;

		if(VehicleData->ThrottlePos < 0.0f)
			VehicleData->ThrottlePos = 0.0f;
		else if(VehicleData->ThrottlePos > 1.0f)
			VehicleData->ThrottlePos = 1.0f;
	}

	return 0;
}

#pragma mark -

UINT32 mlUpdateComponent(mlComponent* Component, mlVehicleData* VehicleData, FLOAT64 DeltaTime, UINT32 PassNum,
							FLOAT64VECTOR* SavedVehicleForce, FLOAT64VECTOR* TempNormForceSum)
{
	FLOAT64VECTOR Force;
	int i, ComponentNum, IsFlightModelController;

	IsFlightModelController = (VehicleData->MultiCrewPos < 2);

	if (PassNum == 0)
	{
		// accumulate new damage
		if (VehicleData->NewDamage)
		{
			for(i = 0; i < (Component->isPak38_6lb_57mm? 10 : 4); i++)
			{
				ComponentNum = Component->DamageComponentNum[i];
				Component->NewDamage[i] = VehicleData->DamageAccrued[ComponentNum];
				Component->NewDamageNumHits[i] = VehicleData->NumRoundsHit[ComponentNum];
			}
		}

		switch (Component->Type)
		{
			case ML_COMPONENT_AIRFOIL:
				if(IsFlightModelController)
					_UpdateAirfoilComponent(Component,VehicleData,DeltaTime);
			break;
			case ML_COMPONENT_ENGINE:
				if (IsFlightModelController)
					_UpdateEngineComponent(Component,VehicleData,DeltaTime);
			break;
			case ML_COMPONENT_PITOT:
					_UpdatePitotTube(Component,VehicleData);
			break;
			case ML_COMPONENT_GEAR:
				if(IsFlightModelController)
					_UpdateGearComponent(Component, VehicleData, DeltaTime, PassNum, SavedVehicleForce, TempNormForceSum);
			break;
			case ML_COMPONENT_WEAPONMOUNT:
				_UpdateWeaponMount(Component,VehicleData,DeltaTime,PassNum);
			break;
			case ML_COMPONENT_CREW:
				_UpdateCrew(Component,VehicleData);
			break;
			case ML_COMPONENT_TOWPOINT:
				if(IsFlightModelController)
					_UpdateTowBar(Component,VehicleData,DeltaTime);
			break;
			case ML_COMPONENT_HAULPOINT:
				if(IsFlightModelController)
					_UpdateHaulPoint(Component,VehicleData,DeltaTime);
			break;
			case ML_COMPONENT_BALLAST:
				_UpdateBallast(Component,VehicleData);
			break;
			default:
			break;
		}

		// all components "suffer" gravity
		Force.x = Force.y = 0;
		Force.z = -mlGRAVITY * Component->Mass;
		mlApplyStaticForceAtRelativePoint(VehicleData,&Component->CurrentRelativePos,&Force);
	}
	else
	{
		switch (Component->Type)
		{
			case ML_COMPONENT_GEAR:
				if(IsFlightModelController)
					_UpdateGearComponent(Component, VehicleData, DeltaTime, PassNum, SavedVehicleForce, TempNormForceSum);
			break;
			case ML_COMPONENT_WEAPONMOUNT:
				_UpdateWeaponMount(Component, VehicleData, DeltaTime, PassNum);
			break;
			default:
			break;
		}

		if(VehicleData->NewDamage)
		{
			for(i = 0; i < (Component->isPak38_6lb_57mm? 10 : 4); i++)
			{
				Component->NewDamage[i] = 0.0;
				Component->NewDamageNumHits[i] = 0;
			}
		}

		if(!Component->ComponentFailed &&
			Component->DamageLevelFail > 0 && Component->VisDamage[0][0] &&
			Component->CurrentDamage  > Component->DamageLevelFail )
		{
			int i;
			for(i = 0; i < MAX_DAMAGE_GROUPS; i++)
			{
				if(!Component->VisDamage[i][0]) break;
				vehVisDamageGroupDestroyed(VehicleData->VehicleMasterPtr, Component->VisDamage[i], TRUE, TRUE);
			}
		}
	}

	return 0;
}

void mlRemoveComponent(mlVehicleData* VehicleData, mlComponent* Component, LOGICAL ParentRemoved)
{
	Component->ComponentStatus = ML_STATUS_MISSING;
	Component->ComponentFailed = TRUE;

	if(Component->Type == ML_COMPONENT_AIRFOIL)
	{
		mlAirfoilComponentData* AirfoilComponentData = (mlAirfoilComponentData*)Component->SpecialData;
		if(AirfoilComponentData)
		{
			if(AirfoilComponentData->Subtype == ML_CANOPYDRAG_SECTION)
			{ // special case for canopy, we want to leave it around to produce drag (and deploy if not already open)

				Component->ComponentStatus = ML_STATUS_OK;
				VehicleData->CanopyOpen[AirfoilComponentData->Position] = 1;
				VehicleData->CanopyJammed[AirfoilComponentData->Position] = 1;
			}else if(AirfoilComponentData->VortexNum > 0) // turn off vortex
			{
				vehAnimChangeVehicleStateByName(VehicleData->VehicleMasterPtr,
						(UCHAR*)((AirfoilComponentData->VortexNum == 1) ? "wingtipvortexleft" : "wingtipvortexright"), 0);
			}
		}
	}else if (Component->Type == ML_COMPONENT_ENGINE) // turn off engine sound
	{
		mlEngineComponentData* EngineComponentData = (mlEngineComponentData*)Component->SpecialData;

		if (EngineComponentData->EngineSoundLoaded) 	fxSndStopSound(EngineComponentData->EngineSoundId);
		if (EngineComponentData->IdleSoundLoaded) 		fxSndStopSound(EngineComponentData->IdleSoundId);
		if (EngineComponentData->StarterSoundLoaded) 	fxSndStopSound(EngineComponentData->StarterSoundId);

        for(SINT32 q = 0; q < 4; q++)
        {
            if(EngineComponentData->DamageLevelSoundId[q] > 0)
      			fxSndStopSound(EngineComponentData->DamageLevelSoundId[q]);
        }
        if(!ParentRemoved && EngineComponentData->SubType == ML_ENGINE_SUBTYPE_PROPELLOR)
			Component->ComponentStatus = ML_STATUS_OK;
	}
}

static UINT32 _UpdateAirfoilComponent(mlComponent* Component, mlVehicleData* VehicleData, FLOAT64 DeltaTime)
{
    FLOAT64VECTOR ComponentVel = {0, 0, 0};
	FLOAT64 FlapPosition = 0;
	UINT32 PolyNum = 0;
    piVECTOR CollidePnt = {0, 0, 0};
	OBJECT * CollideObject = nullptr;
    piVECTOR OldPos = {0, 0, 0},NewPos = {0, 0, 0};
	FLOAT64VECTOR TempVec = {0, 0, 0},TempVec1 = {0, 0, 0};
	FLOAT64VECTOR WorldOffsetOrigin = {0, 0, 0};
	UINT32 RetVal = 0;
	UINT32 i;
	mlAirfoilComponentData* AirfoilComponentData = (mlAirfoilComponentData*)Component->SpecialData;
	LOGICAL foilActive = TRUE;


	// first off get component altitude above ground (for ground effect)
	trnGetWorldOriginOffset(&WorldOffsetOrigin);
	WorldOffsetOrigin.z = -WorldOffsetOrigin.z;

	TempVec = Component->CurrentRelativePos;
	mathVectorMultiply64(&TempVec,&TempVec,&VehicleData->VehicleAttMat);
	mathAddVectors(&TempVec1,&TempVec,&VehicleData->AdjustedForCGPos);
	mathSubVectors(&TempVec,&TempVec,&WorldOffsetOrigin);

	OldPos.x = (FLOAT32)TempVec.x; OldPos.y = (FLOAT32)TempVec.y; OldPos.z = -(FLOAT32)TempVec.z;
	NewPos = OldPos; NewPos.z += 10000.0;

	RetVal = trncdCollideWithPolyList(&OldPos, &NewPos, &CollidePnt, &CollideObject, &PolyNum,
				 VehicleData->TerrainAGLObjectList,	VehicleData->TerrainAGLPolyList, VehicleData->NumTerrainAGL);

// first calculate velocity of the component relative to the world
	mlCompCalcComponentVelocityVector(&ComponentVel,Component,VehicleData);

	FlapPosition = VehicleData->FlapPosition;
	const FLOAT64 Speed = mathExactVectorLength(&ComponentVel);

	// check if gear and canopy airfoils are deployed
	if(AirfoilComponentData->Subtype == ML_CANOPYDRAG_SECTION)
	{
		if(!VehicleData->CanopyOpen[AirfoilComponentData->Position])
			foilActive = FALSE;
	}else if(AirfoilComponentData->Subtype == ML_GEARDRAG_SECTION)
	{
		if(!VehicleData->GearSetting)
			foilActive = FALSE;
	}else if(AirfoilComponentData->Subtype == ML_DEPLOYDRAG_SECTION)
	{
		if(!VehicleData->BombBayDoor)
			foilActive = FALSE;
	}

	if(foilActive)
	{
		OBJECT* TempObj = VehicleData->VehicleObjectData ? VehicleData->VehicleObjectData->PlaneObject : nullptr;
		FLOAT64ATT Torque = {0, 0, 0};
		FLOAT64VECTOR Force = {0, 0, 0};
		FLOAT64VECTOR RelativePoint = {0, 0, 0};
		FLOAT64 ClMovement = 0; // center of lift movement
		FLOAT64 StallAmount = 0;

		FLOAT64 AGL = RetVal ? CollidePnt.z - OldPos.z : 20000;
		if(VehicleData->CollisionsOff)
			AGL = 20000;

		// now calculate airfoil segment forces/torques relative to the airfoil segment
		FLOAT64 ElevPos = 0, RudderPos = 0, AileronPos = 0;
		mlGetElevatorDeflection(&ElevPos, DeltaTime, VehicleData);
		mlGetRudderDeflection(&RudderPos, DeltaTime, VehicleData);
		mlGetAileronDeflection(&AileronPos, DeltaTime, VehicleData);

		mlGetAirfoilForcesAndTorques(&Force, &Torque, &ClMovement, Component, &ComponentVel,
									VehicleData->VehiclePos.z, AGL, FlapPosition, DeltaTime,
									ElevPos, RudderPos, AileronPos, &StallAmount,
									VehicleData->ElevatorTabPosition + VehicleData->ElevatorTabStatic,
									TempObj, VehicleData->LeftSlatSound, VehicleData->RightSlatSound,
									VehicleData->VehicleObjectData, VehicleData->DiveBrakeOn);

		RelativePoint.x = RelativePoint.y = RelativePoint.z = 0.0;
		if(VehicleData->PhysicsVersion > 0)
			RelativePoint.y = AirfoilComponentData->WingChord * .25;
		mathRotateVectorByAtt(&RelativePoint,&RelativePoint,&Component->Attitude);
		mathAddVectors(&RelativePoint,&RelativePoint,&Component->CurrentRelativePos);

		// adjust force to the world
		mathRotateVectorByAtt(&Force,&Force,&Component->Attitude);
		mathRotateVectorByAtt(&Force,&Force,&VehicleData->VehicleAtt);

		mlApplyStaticForceAtRelativePoint(VehicleData, &RelativePoint,&Force);
		mlApplyTorqueAtRelativePoint(VehicleData, &Component->CurrentRelativePos, &Torque, &Component->Attitude,
										VehicleData->PhysicsVersion > 1);

		FLOAT64 StallVolume = StallAmount;
		if(Speed < 50.0)
			StallVolume *= (Speed < 25.0) ? 0.0 : ((Speed-25.0) / 25.0) * ((Speed-25.0) / 25.0);

		if(VehicleData->StallSoundVolume < StallVolume && AirfoilComponentData->StallSound != 0)
			VehicleData->StallSoundVolume = StallVolume;

		if(AirfoilComponentData->VortexNum > 0 && AirfoilComponentData && IsFirstTic)
		{
			LOGICAL State = AirfoilComponentData->VortexOn;

			if(Component->OKIfFail) // don't display vortex if stub component
				State = FALSE;

			vehAnimChangeVehicleStateByName(VehicleData->VehicleMasterPtr,
				(UCHAR*)((AirfoilComponentData->VortexNum == 1) ? "wingtipvortexleft" : "wingtipvortexright"),
				State);

		}
	}
	// add in component damage effects
	if (Component->DamageGroupNum > 0)
	{
		FLOAT32 PossibleDamage = 0.0f;
		for(i = 0; i < 4; i++)
		{
			if (Component->DamageLevels[i] > PossibleDamage)
				PossibleDamage = (FLOAT32)Component->DamageLevels[i];
		}
		if (VehicleData->DamageCurrent[Component->DamageGroupNum-1] > PossibleDamage)
			VehicleData->DamageCurrent[Component->DamageGroupNum-1] += PossibleDamage;
		else
			VehicleData->DamageCurrent[Component->DamageGroupNum-1] += (FLOAT32)Component->CurrentDamage;

		VehicleData->DamagePossible[Component->DamageGroupNum-1] += PossibleDamage;
	}

	// handle vehicle dead stuff
	if (Component->CriticalSecondary != 0 || Component->CriticalPrimary != 0)
	{
		if (Component->CurrentDamage >= Component->DamageLevels[3])
		{
			if (Component->CriticalPrimary != 0)
			{
#ifdef BETA_RELEASE
				if(!VehicleData->VehicleIsDead)
					uiChatPrintLine("Vehicle Killed by critical primary");
#endif
				VehicleData->VehicleIsDead = 1;
				_DeclareVehicleKilled (VehicleData, Component);
			}
			if (Component->CriticalSecondary != 0)
				VehicleData->SecondaryCriticalComponentCount[Component->CriticalSecondary-1]++;
		}

		_CheckForSecondaryVehicleKill (VehicleData, Component);
	}

	// overspeed damage
	if(foilActive && AirfoilComponentData->MaxDamageSpeed > 0 && Speed > AirfoilComponentData->MaxDamageSpeed)
	{
		// jam open
		if(Component->DamageLevels[2] > 0 && Component->CurrentDamage > Component->DamageLevels[2])
			AirfoilComponentData->ControlSurfaceJammed = 1;

		switch(AirfoilComponentData->Subtype)
		{
			case ML_FLAP_SECTION:
				if(AirfoilComponentData->ControlSurfaceJammed)
				{
					VehicleData->FlapsJammed = 1;
				}else
				{
					if(FlapPosition >= .5 && Component->CurrentDamage < MAX_DAMAGE) // only damage flaps if greater than 50% (NOTE: some code seems to think FlapPosition can be from 0-4)
						Component->CurrentDamage += DRAG_DAMAGE * DeltaTime / 1000.0;
				}

				if(FlapPosition >= .5)
				{
					persShakeCamera(.003f);
					VehicleData->FlapsBeingDamaged = 1;

					if(!VehicleData->FlapsJammed && Component->DamageLevels[3] > 0 && Component->CurrentDamage < Component->DamageLevels[3])
						VehicleData->FlapDamageSoundVolume = (FLOAT32)(.25 + .75 * Component->CurrentDamage / Component->DamageLevels[3]);
					else
						VehicleData->FlapDamageSoundVolume = 1.0f;

				}
			break;

			case ML_GEARDRAG_SECTION:
				if(AirfoilComponentData->ControlSurfaceJammed)
				{
					VehicleData->GearJammed = 1;
				}else
				{
					if(VehicleData->GearPosition > .1)
					{
						if(Component->CurrentDamage < MAX_DAMAGE)
							Component->CurrentDamage += DRAG_DAMAGE * DeltaTime / 1000.0;

						if(AirfoilComponentData->DamageComponent == nullptr)
						{// find it if we haven't already
							mlComponentListIterator it;
							mlComponentListBegin(VehicleData->ComponentList, &it);

							for(; it.comp; mlComponentListNext(&it))
							{
								mlComponent* comp = it.comp;
								mlGearComponentData* gearData = (mlGearComponentData*)comp->SpecialData;
								if(comp->Type == ML_COMPONENT_GEAR && (SINT32)(gearData->GearNum) == AirfoilComponentData->Position)
								{
									AirfoilComponentData->DamageComponent = comp;
								}
							}
						}

						// and damage gear component (as long as it's below damagelevelfail (overspeed doesn't cause gear to rip off)
						if(AirfoilComponentData->DamageComponent)
						{
							mlComponent* gearComp = AirfoilComponentData->DamageComponent;
							FLOAT64 MaxDamage;

							MaxDamage = (gearComp->DamageLevelFail > 0.0) ? gearComp->DamageLevelFail : MAX_DAMAGE;

							if(gearComp->CurrentDamage < (MaxDamage - 2 * (DRAG_DAMAGE * DeltaTime / 1000.0))) // Stay away from DamageLevelFail
								gearComp->CurrentDamage += DRAG_DAMAGE * DeltaTime / 1000.0;
						}
					}
				}
				VehicleData->GearBeingDamaged = 1;

				if(!VehicleData->GearJammed && Component->DamageLevels[3] > 0 && Component->CurrentDamage < Component->DamageLevels[3])
					VehicleData->GearDamageSoundVolume = (FLOAT32)(.25 + .75 * Component->CurrentDamage / Component->DamageLevels[3]);
				else
					VehicleData->GearDamageSoundVolume = 1.0f;

				persShakeCamera(.003f);
			break;

			case ML_CANOPYDRAG_SECTION:
				if(AirfoilComponentData->ControlSurfaceJammed)
					VehicleData->CanopyJammed[AirfoilComponentData->Position] = 1;

				// keep applying damage 'til that sucker's ripped off.
				if(!Component->ComponentFailed && Component->CurrentDamage < MAX_DAMAGE)
					Component->CurrentDamage += DRAG_DAMAGE * DeltaTime / 1000.0;

				if(!Component->ComponentFailed) // if it's ripped off stop shaking
					persShakeCamera(.003f);
			break;
		}
	}

	// stall shake
	if(VehicleData->StallSoundVolume > .001)
		persShakeCamera((FLOAT32)(.01 * VehicleData->StallSoundVolume));

// draw airfoil sections in object analysis mode
#ifdef BETA_RELEASE
	if(IsFirstTic && DisplayFoils && foilActive && VehicleObjectAnalysis)
	{
		FLOAT64VECTOR p1, p2, p3, p4;
		piVECTOR V1;
		FLOAT64 InnerChord, OuterChord, Len;
		FLOAT64ATT att = Component->Attitude;
		UINT32 Color = (AirfoilComponentData->IsBlanked) ? colorREDALPHA : colorYELLOWALPHA;
		att.Pitch = -att.Pitch;

	// component position
		p1 = Component->OriginalRelativePos;
		mathVectorMultiply64(&p1,&p1,&VehicleData->VehicleAttMat);
		mathAddVectors(&p1,&p1,&VehicleData->AdjustedForCGPos); p1.z = -p1.z;
		p1.x -= WorldOriginOffset.x; p1.y -= WorldOriginOffset.y; p1.z -= WorldOriginOffset.z;
		V1.x = (FLOAT32)p1.x; V1.y = (FLOAT32)p1.y; V1.z = (FLOAT32)p1.z;

		draw3dDebugMarkPoint(&V1, Color, 0.3f);

	// display chord/wing area
		InnerChord = AirfoilComponentData->InnerChord ? AirfoilComponentData->InnerChord : AirfoilComponentData->WingChord;
		OuterChord = AirfoilComponentData->OuterChord ? AirfoilComponentData->OuterChord : AirfoilComponentData->WingChord;

		if(Component->OriginalRelativePos.x > 0)
		{
			FLOAT64 tmp = InnerChord;
			InnerChord = OuterChord;
			OuterChord = tmp;
		}

		Len = 2 * AirfoilComponentData->Area/ (InnerChord + OuterChord);

		p1.x = -Len / 2; p1.y =  OuterChord/2; p1.z = 0;
		p2.x = -Len / 2; p2.y = -OuterChord/2; p2.z = 0;
		p3.x =  Len / 2; p3.y = -InnerChord/2; p3.z = 0;
		p4.x =  Len / 2; p4.y =  InnerChord/2; p4.z = 0;

		mathRotateVectorByAtt(&p1,&p1,&att);
		mathRotateVectorByAtt(&p2,&p2,&att);
		mathRotateVectorByAtt(&p3,&p3,&att);
		mathRotateVectorByAtt(&p4,&p4,&att);

		mathAddVectors(&p1, &p1, &Component->OriginalRelativePos);
		mathAddVectors(&p2, &p2, &Component->OriginalRelativePos);
		mathAddVectors(&p3, &p3, &Component->OriginalRelativePos);
		mathAddVectors(&p4, &p4, &Component->OriginalRelativePos);

		draw3dGizmoLine(&p1, &p2, &VehicleData->VehicleAttMat, &VehicleData->AdjustedForCGPos, Color);
		draw3dGizmoLine(&p2, &p3, &VehicleData->VehicleAttMat, &VehicleData->AdjustedForCGPos, Color);
		draw3dGizmoLine(&p3, &p4, &VehicleData->VehicleAttMat, &VehicleData->AdjustedForCGPos, Color);
		draw3dGizmoLine(&p4, &p1, &VehicleData->VehicleAttMat, &VehicleData->AdjustedForCGPos, Color);
	}
#endif
	return 0;
}

static UINT32 _UpdateWeaponMount(mlComponent * Component,
								 mlVehicleData * VehicleData,
								 FLOAT64 DeltaTime,
								 UINT32 PassNum)
{
FLOAT64ATT Torque  = {0, 0, 0};
FLOAT64VECTOR Force  = {0, 0, 0};
UINT32 PrimaryIsFiring = 0,SecondaryIsFiring = 0;
OBJECT * TempObj = nullptr;
UINT32 MountFired = 0;
mlWeaponMount * Mount = nullptr;
weapWeapon * Weapon = nullptr;
vehVehicle* Vehicle = nullptr;
UINT32 Position = 0;

	if (VehicleData->VehicleObjectData == nullptr)
	{
		TempObj = nullptr;
	}
	else
	{
		if (VehicleData->VehicleObjectData->HitMap == nullptr)
		{
			TempObj = VehicleData->VehicleObjectData->PlaneObject;
		}
		else
		{
			TempObj = VehicleData->VehicleObjectData->HitMap;
		}
	}

	if (PassNum == 0)
	{

		MountFired = 0;
		mlUpdateWeaponMount(&Force,
							&Torque,
							Component,
							VehicleData,
							DeltaTime,
							VehicleData->LastVehicleUpdateTime,
							&PrimaryIsFiring,
							&SecondaryIsFiring,
							TempObj,
							VehicleData->ComponentList,
							persGunsightConvergenceRange(),
							&MountFired);

		Mount = (mlWeaponMount*)Component->SpecialData;
		Vehicle = (vehVehicle*)VehicleData->VehicleMasterPtr;

		if (MountFired != 0)
		{

			if (Mount != nullptr)
			{
				VehicleData->MountIsFiring[Mount->MountNum-1] = TRUE;

			}
		}

		// firemask specified what weapons the host vehicle update system may fire
		if (Vehicle != nullptr)
		{
			if (VehicleData->MultiCrewPos != 0 &&
				VehicleData->MultiCrewPos != Vehicle->VehicleType->MultiCrewPos[Mount->PositionNum])
			{
				VehicleData->FireMask |= 1 << (Mount->MountNum-1);
			}
		}

		if (PrimaryIsFiring == 1)
		{
			VehicleData->PrimaryIsFiring = 1;
		}
		if (SecondaryIsFiring == 1)
		{
			VehicleData->SecondaryIsFiring = 1;
		}

		mlApplyStaticForceAtRelativePointWithMat(VehicleData,&Component->CurrentRelativePos,&Force);
	}

	Mount = (mlWeaponMount*)Component->SpecialData;
	persGetPersonaPosition(&Position);

	if(PassNum == 1)
	{
		if ((Mount->TriggerType == ML_TRIGGERTYPE_PRIMARY ||
			Mount->TriggerType == ML_TRIGGERTYPE_BOTH) &&
			Mount->PositionNum == Position)
		{
			if (Mount->Weapons[0].WeaponType != nullptr)
			{
				Weapon = &Mount->Weapons[0];

				VehicleData->CurrentPrimaryAmmoCount += Weapon->NumRoundsAmmoLeft[Weapon->AmmoTypeInUse];
				VehicleData->PrimaryClipCount = 0;
				VehicleData->PrimaryClipType = Weapon->WeaponType->ClipType;

				if(Weapon->BeltName[Weapon->AmmoTypeInUse][0])
					strncpy(VehicleData->PrimaryAmmoType,Weapon->BeltName[Weapon->AmmoTypeInUse],31);

				if(Weapon->WeaponType->NumRoundsInClip > 1 && VehicleData->CurrentPrimaryAmmoCount > 1)
				{
					VehicleData->PrimaryClipCount = (VehicleData->CurrentPrimaryAmmoCount-1) / Weapon->WeaponType->NumRoundsInClip;
					VehicleData->CurrentPrimaryAmmoCount -= VehicleData->PrimaryClipCount * Weapon->WeaponType->NumRoundsInClip;
				}
			}
		}

		if ((Mount->TriggerType == ML_TRIGGERTYPE_SECONDARY ||
			Mount->TriggerType == ML_TRIGGERTYPE_BOTH) &&
			Mount->PositionNum == Position)
		{
			if (Mount->Weapons[0].WeaponType != nullptr)
			{
				Weapon = &Mount->Weapons[0];
				VehicleData->CurrentSecondaryAmmoCount = Weapon->NumRoundsAmmoLeft[Weapon->AmmoTypeInUse];
				VehicleData->SecondaryClipCount = 0;
				VehicleData->SecondaryClipType = Weapon->WeaponType->ClipType;

				if(Weapon->BeltName[Weapon->AmmoTypeInUse][0])
					strncpy(VehicleData->SecondaryAmmoType,Weapon->BeltName[Weapon->AmmoTypeInUse],31);

				if(Weapon->WeaponType->NumRoundsInClip > 1 && VehicleData->CurrentSecondaryAmmoCount > 1)
				{
					VehicleData->SecondaryClipCount = (VehicleData->CurrentSecondaryAmmoCount-1) / Weapon->WeaponType->NumRoundsInClip;
					VehicleData->CurrentSecondaryAmmoCount -= VehicleData->SecondaryClipCount * Weapon->WeaponType->NumRoundsInClip;
				}

				if (VehicleData->BombSightAmmoType == nullptr)
				{
					if (VehicleData->OrdnanceSelected + 1 == Mount->SelectorNum)
					{
					weapAmmoType * AmmoType;

						Weapon = &Mount->Weapons[0];
						AmmoType = Weapon->WeaponType->AmmoType[0];

						VehicleData->BombSightAmmoType = AmmoType;
					}
				}

				if (Mount->AddToGroupAmmoCount != 0 &&
					VehicleData->OrdnanceSelected + 1 == Mount->SelectorNum)
				{
					Weapon = &Mount->Weapons[0];
					if (Weapon != nullptr)
					{
						mlAddToGroupAmmoCount(Weapon->NumRoundsAmmoLeft[0],
											Weapon->NumTotalRounds[0]);
					}
				}
			}
		}
	}

	// handle vehicle dead stuff
	if (Component->CriticalSecondary != 0 || (Component->CriticalPrimary != 0 && PassNum == 1))
	{
		if (Mount->WeaponJammed != 0)
		{
			if (Component->CriticalPrimary != 0)
			{
#ifdef BETA_RELEASE
				if(!VehicleData->VehicleIsDead)
					uiChatPrintLine("Vehicle Killed by weapon mount critical primary");
#endif

				VehicleData->VehicleIsDead = 1;
				_DeclareVehicleKilled (VehicleData, Component);
			}
			if (Component->CriticalSecondary != 0)
			{
				VehicleData->SecondaryCriticalComponentCount[Component->CriticalSecondary-1]++;
			}
		}

		_CheckForSecondaryVehicleKill (VehicleData, Component);
	}

	return 0;
}

static UINT32 _UpdateBallast(mlComponent* Component,mlVehicleData * VehicleData)
{
SINT32 TempInt = 0;
SINT32 i = 0;
FLOAT32 PossibleDamage = 0;

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

	// add in component damage effects
	if (Component->DamageGroupNum > 0)
	{
		PossibleDamage = 0.0f;
		for (i=0;i<4;i++)
		{
			if (Component->DamageLevels[i] > PossibleDamage)
			{
				PossibleDamage = (FLOAT32)Component->DamageLevels[i];
			}
		}
		if (VehicleData->DamageCurrent[Component->DamageGroupNum-1] > PossibleDamage)
		{
			VehicleData->DamageCurrent[Component->DamageGroupNum-1] += PossibleDamage;
		}
		else
		{
			VehicleData->DamageCurrent[Component->DamageGroupNum-1] += (FLOAT32)Component->CurrentDamage;
		}

		VehicleData->DamagePossible[Component->DamageGroupNum-1] += PossibleDamage;
	}


	return 0;
}


static UINT32 _UpdateGearComponent(mlComponent * Component,
								   mlVehicleData * VehicleData,
								   FLOAT64 DeltaTime,
									UINT32 PassNum,
									FLOAT64VECTOR* SavedVehicleForce,
									FLOAT64VECTOR* TempNormForceSum)
{
FLOAT64ATT Torque = {0, 0, 0};
FLOAT64VECTOR Force = {0, 0, 0};
FLOAT64VECTOR Pos = {0, 0, 0};
FLOAT64 BrakeLeft = 0,BrakeRight = 0,BrakeCenter = 0;

	Pos = VehicleData->AdjustedForCGPos;

	if (VehicleData->NumTics < 50)
	{
		BrakeLeft = BrakeRight = BrakeCenter = 1.0;
	}
	else
	{
		BrakeLeft = VehicleData->BrakeForceLeft;
		BrakeRight = VehicleData->BrakeForceRight;
		BrakeCenter = VehicleData->BrakeForceCenter;
	}

	mlUpdateGearComponent(&Force,
						  &Torque,
						  Component,
						  &Pos,
						  &VehicleData->VehicleRealVel,
						  &VehicleData->VehicleAttMat,
						  &VehicleData->VehicleRealAttVel,
						  DeltaTime,
						  BrakeLeft,
						  BrakeRight,
						  BrakeCenter,
						  VehicleData->VehicleMass,
						  VehicleData->TrnCDListObject,
						  VehicleData->TrnCDListTOs,
						  VehicleData->TerrainAGLObjectList,
						  VehicleData->TerrainAGLPolyList,
						  VehicleData->NumTerrainAGL,
						  VehicleData->CollisionsOff,
						  PassNum,
						  SavedVehicleForce,
						  TempNormForceSum,
						  VehicleData);

	// correct force for flight model peculiarities

	// force is relative to world
	if (PassNum == 1)
	{
		mlApplyStaticForceAtRelativePointWithMat(VehicleData,&Component->CurrentRelativePos,&Force);
	}

	// handle vehicle dead stuff
	if (Component->CriticalSecondary != 0 || (Component->CriticalPrimary != 0 && PassNum == 1))
	{
		if (Component->CurrentDamage >= Component->DamageLevels[3])
		{
			if (Component->CriticalPrimary != 0)
			{
#ifdef BETA_RELEASE
				if(!VehicleData->VehicleIsDead)
					uiChatPrintLine("Vehicle Killed by gear critical primary");
#endif
				VehicleData->VehicleIsDead = 1;
				_DeclareVehicleKilled (VehicleData, Component);
			}
			if (Component->CriticalSecondary != 0)
			{
				VehicleData->SecondaryCriticalComponentCount[Component->CriticalSecondary-1]++;
			}
		}
		_CheckForSecondaryVehicleKill (VehicleData, Component);
	}

	return 0;
}

static UINT32 _UpdateHaulPoint(mlComponent * Component,
							   mlVehicleData * VehicleData,
							   FLOAT64 DeltaTime)
{
FLOAT64VECTOR TempVec1 = {0, 0, 0},TempVec2 = {0, 0, 0};
piMATRIX Mat = IdentityMatrix;
FLOAT64VECTOR ComponentVel = {0, 0, 0};
FLOAT64VECTOR Force = {0, 0, 0};
piVECTOR TempPiVec = {0, 0, 0};
LOGICAL ForceApplied = 0;

	// no hauling when deployed
	if (VehicleData->BombBayDoor == 1)
	{
        // Turn off sound if pushing AT gun
        if(VehicleData->VehicleMasterPtr->VehicleType->Type == VEH_TYPE_GROUND)
        {
            if(Component->SoundId > 0)
            {
                fxSndStopSound(Component->SoundId);
                Component->SoundId = 0;
            }
        }

		return 0;
	}

	// no hauling when towing
	if (VehicleData->TowBarEnabled == 1)
	{
		return 0;
	}

	if (VehicleData->VehicleObjectData == nullptr)
	{
		return 0;
	}

	// determine point's velocity vector, relative to vehicle
	{
		TempVec1 = Component->CurrentRelativePos;
		TempPiVec.x = -(FLOAT32)(VehicleData->VehicleRealAttVel.Pitch * 0.001);
		TempPiVec.y = -(FLOAT32)(VehicleData->VehicleRealAttVel.Roll * 0.001);
		TempPiVec.z = (FLOAT32)(VehicleData->VehicleRealAttVel.Yaw * 0.001);
		mathEul_ToMatrix(&Mat,EulOrdXYZs,&TempPiVec);

		mathVectorMultiply64(&TempVec2,&TempVec1,&Mat);
		ComponentVel.x = (TempVec2.x - TempVec1.x) * (1/0.001);
		ComponentVel.y = (TempVec2.y - TempVec1.y) * (1/0.001);
		ComponentVel.z = (TempVec2.z - TempVec1.z) * (1/0.001);

		mathVectorMultiply64(&ComponentVel,&ComponentVel,&VehicleData->VehicleAttMat);

		mathAddVectors(&ComponentVel,&ComponentVel,&VehicleData->VehicleRealVel);

		// finally convert velocity to vehicle's reference
		mathVectorMultiply64(&ComponentVel,&ComponentVel,&VehicleData->VehicleAttInvMat);
	}


	Force.x = Force.y = Force.z = 0.0f;
	ForceApplied = FALSE;

	// front/back

	Force.x = -Component->HaulAroundForce * ComponentVel.x / Component->HaulAroundSpeed;
	Force.x -= Component->HaulAroundForce * VehicleData->ATGunMovementLeftRight;
	if (VehicleData->ATGunMovementLeftRight > 0.1 ||
		VehicleData->ATGunMovementLeftRight < -0.1)
	{
		ForceApplied = TRUE;
	}

	Force.y = -Component->HaulAroundForce * ComponentVel.y / Component->HaulAroundSpeed;
	Force.y += Component->HaulAroundForce * VehicleData->ATGunMovementForwardsBackwards;
	if (VehicleData->ATGunMovementForwardsBackwards > 0.1 ||
		VehicleData->ATGunMovementForwardsBackwards < -0.1)
	{
		ForceApplied = TRUE;
	}


	if (ForceApplied == TRUE && Component->UserLogical == FALSE)
	{
		if (Component->SoundName[0] == 0)
		{
			Component->SoundId = -1;
		}

		if (Component->SoundId <= 0)
		{
		OBJECT * TempObj;

			if (VehicleData->VehicleObjectData->HitMap != nullptr)
			{
				TempObj = VehicleData->VehicleObjectData->HitMap;
			}
			else
			{
				TempObj = VehicleData->VehicleObjectData->PlaneObject;
			}

            SINT32 iID = fxSndGetNewSound();

            if(iID > 0)
            {
                fxSndSetSoundName(iID, (SCHAR*)Component->SoundName);
                fxSndSetPriority(iID, IDE_TP_PRIORITY_HIGH);
                fxSndSetSoundType(iID, TYPE_INFINITE_SOUND);
                fxSndSetPitch(iID, 1.0f);
                fxSndSetVolume(iID, 1.0f);
                fxSndSetPosition(iID,
                                 Component->OriginalRelativePos.x,
                                 Component->OriginalRelativePos.y,
                                 Component->OriginalRelativePos.z);
                fxSndSetEmitterObject(iID, TempObj);
                fxSndSetPlayType(iID, DEF_PLAYTYPE_OBJECT);
                fxSndSetKillCallback(iID, _SoundStopped);
                fxSndSetKillData(iID, &Component->SoundId);

                if(fxSndInitSound(iID))
                {
                    Component->SoundId = iID;
                }
                else
                {
                    Component->SoundId = -1;
                }
             }
		}
	}
	if (ForceApplied == FALSE && Component->UserLogical == TRUE)
	{
		if (Component->SoundId > 0)
		{
			fxSndStopSound(Component->SoundId);
			Component->SoundId = 0;
		}
	}

	Component->UserLogical = ForceApplied;

	mathVectorMultiply64(&Force,&Force,&VehicleData->VehicleAttMat);
	mlApplyStaticForceAtRelativePointWithMat(VehicleData,&Component->CurrentRelativePos,&Force);

	return 0;
}

static UINT32 _UpdateTowBar(mlComponent * Component,
							mlVehicleData * VehicleData,
							FLOAT64 DeltaTime)
{
	FLOAT64VECTOR Force = {0, 0, 0};
	FLOAT64VECTOR DeltaVec = {0, 0, 0};
	piVECTOR TempVec = {0, 0, 0};
	OBJECT* TowTarget = nullptr;
	vehVehicle* CurrentVehicle = nullptr;
	FLOAT64VECTOR WorldOriginOffset = {0, 0, 0};
	FLOAT64VECTOR HitchPos = {0, 0, 0},AttachPos = {0, 0, 0};

	DeltaVec.x = DeltaVec.y = DeltaVec.z = 0.0f;

	if (VehicleData->TowBarEnabled)
	{
		Force.x = Force.y = Force.z = 0.0;
		TowTarget = nullptr;
		trnGetWorldOriginOffset(&WorldOriginOffset);

		mathVectorMultiply64(&AttachPos,&Component->CurrentRelativePos,&VehicleData->VehicleAttMat);
		mathAddVectors(&AttachPos,&AttachPos,&VehicleData->AdjustedForCGPos);

		CurrentVehicle = vehFindVehicle(VehicleData->VehicleMasterPtr->RideVehicleHostIndex);
		if(!CurrentVehicle)
		{
			rnmainListLocText(IDE_LANG_UI, TOW_UNHITCHED_LOC_ID);
			VehicleData->VehicleVel.x = VehicleData->VehicleVel.y = VehicleData->VehicleVel.z = 0;
			VehicleData->VehicleLastVel.x = VehicleData->VehicleLastVel.y = VehicleData->VehicleLastVel.z = 0;
			VehicleData->VehicleRealVel.x = VehicleData->VehicleRealVel.y = VehicleData->VehicleRealVel.z = 0;
			VehicleData->TowBarEnabled = FALSE;
			VehicleData->VehicleMasterPtr->RideVehicleHostIndex = 0;
			VehicleData->VehicleMasterPtr->RideSeatNum = 0;
		}else
		{
			mathSubVectors(&DeltaVec,&CurrentVehicle->PosData.Pos,&AttachPos);
			TowTarget = CurrentVehicle->VehicleObjectData->TowObject;
			if (TowTarget != nullptr)
			{

				TempVec.x = TowTarget->dof->dofmatrix2._41;
				TempVec.y = TowTarget->dof->dofmatrix2._42;
				TempVec.z = TowTarget->dof->dofmatrix2._43;
				math3dVectorMultiply(&TempVec,&TempVec,&TowTarget->mat);

				HitchPos.x = TempVec.x + WorldOriginOffset.x;
				HitchPos.y = TempVec.y + WorldOriginOffset.y;
				HitchPos.z = -(TempVec.z + WorldOriginOffset.z);

				mathSubVectors(&DeltaVec,&HitchPos,&AttachPos);

				// ok if we're here, we're going to tow
				// velocity component
				TempVec.x = (FLOAT32)((DeltaVec.x - Component->UserFloat64.x) / (DeltaTime / 1000.0));
				TempVec.y = (FLOAT32)((DeltaVec.y - Component->UserFloat64.y) / (DeltaTime / 1000.0));
				TempVec.z = (FLOAT32)((DeltaVec.z - Component->UserFloat64.z) / (DeltaTime / 1000.0));

				Force.x = 4.0 * VehicleData->VehicleMass * DeltaVec.x / MIN_TOW_DISTANCE +
							TempVec.x * 10000.0;
				Force.y = 4.0 * VehicleData->VehicleMass * DeltaVec.y / MIN_TOW_DISTANCE +
							TempVec.y * 10000.0;
				Force.z = 4.0 * VehicleData->VehicleMass * DeltaVec.z / MIN_TOW_DISTANCE +
							TempVec.z * 10000.0;
				mlApplyStaticForceAtRelativePointWithMat(VehicleData,&Component->CurrentRelativePos,&Force);
			}

		}
	}
	Component->UserFloat64 = DeltaVec;

	return 0;
}


static UINT32 _UpdateEngineComponent(mlComponent * Component,
									 mlVehicleData * VehicleData,
									 FLOAT64 DeltaTime)
{
    FLOAT64VECTOR ComponentVel = {0, 0, 0};
FLOAT64ATT Torque = {0, 0, 0};
FLOAT64VECTOR Force = {0, 0, 0};
FLOAT64 Alt = 0;
FLOAT64 ThrottlePos = 0;
mlEngineComponentData * EngineData = nullptr;
UCHAR s[32];

	// first calculate velocity of the component relative to the world
	mlCompCalcComponentVelocityVector(&ComponentVel,Component,VehicleData);

	Alt = VehicleData->VehiclePos.z;

	// now calculate airfoil segment forces/torques relative to the airfoil segment
	mlGetThrottlePosition(&ThrottlePos,DeltaTime,VehicleData);
	mlUpdateEngineComponent(&Force,
							&Torque,
							Component,
							&ComponentVel,
							Alt,
							DeltaTime,
							&VehicleData->VehicleAttVel,
							ThrottlePos,
							VehicleData->ClutchPos,
							VehicleData->GearShift,
							VehicleData->ReverseToggle,
							VehicleData->LeftTrackClutch,
							VehicleData->RightTrackClutch,
							VehicleData->FailsafeMode,
							VehicleData->PropPitchAdjust,
							VehicleData->ToggleWepMode,
							VehicleData->VehicleObjectData,
							VehicleData->ComponentList,
							(void*)VehicleData->VehicleMasterPtr,
							VehicleData->LastVehicleUpdateTime,
							VehicleData->VehicleG);

	EngineData = (mlEngineComponentData*)Component->SpecialData;

	// check to see if a fire started .. if it did, declare myself killed
	if (!VehicleData->DeclaredKilled)
	{
		if (EngineData->TankOnFire)
		{
#ifdef BETA_RELEASE
			uiChatPrintLine("Fuel fire .. declaring myself killed");
#endif
			_DeclareVehicleKilled (VehicleData, Component);
		}
	}

	if (EngineData != nullptr)
	{
		if (EngineData->SubType == ML_ENGINE_SUBTYPE_ENGINE &&
			EngineData->EngineData != nullptr)
		{
			sprintf((char*)s,"exhaustsmoke%u",EngineData->EngineNumber);

			if(IsFirstTic)
			{
				if (EngineData->CurrentManifoldPress >= EngineData->EngineData->BaseHpManifoldPress * 0.8 &&
					EngineData->EngineCaught == 1)
				{
					vehAnimChangeVehicleStateByName((vehVehicle*)VehicleData->VehicleMasterPtr, s, TRUE);
				}
				else
				{
					vehAnimChangeVehicleStateByName((vehVehicle*)VehicleData->VehicleMasterPtr, s, FALSE);
				}
			}

			// handle vehicle dead stuff
			if (Component->CriticalSecondary != 0 || Component->CriticalPrimary != 0)
			{

				if (Component->CurrentDamage >= Component->DamageLevels[2])
				{
					if (Component->CriticalPrimary != 0)
					{
#ifdef BETA_RELEASE
						if(!VehicleData->VehicleIsDead)
							uiChatPrintLine("Vehicle Killed by engine critical primary");
#endif
						VehicleData->VehicleIsDead = 1;
						_DeclareVehicleKilled (VehicleData, Component);
					}
					if (Component->CriticalSecondary != 0)
					{
						VehicleData->SecondaryCriticalComponentCount[Component->CriticalSecondary-1]++;
					}
				}
				_CheckForSecondaryVehicleKill (VehicleData, Component);
			}

		}else if (EngineData->SubType == ML_ENGINE_SUBTYPE_PROPELLOR && EngineData->EngineData != nullptr)
		{
			VehicleData->AnimationData.PropFailed[EngineData->EngineNumber-1] = Component->ComponentFailed;
		}
	}


	// adjust force to the world
	mathRotateVectorByAtt(&Force,&Force,&Component->Attitude);
	mathRotateVectorByAtt(&Force,&Force,&VehicleData->VehicleAtt);

// BIG BIG HACK!!!!!!!!
// x axis is reversed until the yaw torque problem is resolved
{
FLOAT64VECTOR RelPos;

	RelPos = Component->CurrentRelativePos;
	RelPos.x = -RelPos.x;
	mlApplyStaticForceAtRelativePoint(VehicleData,&RelPos,&Force);
}

	mlApplyTorqueAtRelativePoint(VehicleData,
								&Component->CurrentRelativePos,
								&Torque,
								&Component->Attitude,
								VehicleData->PhysicsVersion > 1);

	return 0;
}

static UINT32 _UpdateCrew(mlComponent * Component,mlVehicleData * VehicleData)
{
	static const char* Location[] = {"combat.crew.head", "combat.crew.torso", "combat.crew.arms", "combat.crew.legs"};
	static char s[128];
	int i, NumHits = 0;
	UINT32 isDead = 0;

	UINT32 CrewPosition = Component->CrewPosition;
	if(CrewPosition >= ML_MAX_CREW_POSITIONS)
	{
		Write_Log(LOG_WEAP, LOG_WARN, "_UpdateCrew: Invalid Crew Position! %d", CrewPosition+1);
		return 0;
	}

	for (i=0;i<(Component->isPak38_6lb_57mm? 10 : 4);i++)
		NumHits += Component->NewDamageNumHits[i];

    if (VehicleData->NewDamage)
    {
        //printf("damage for component %s, hits %d\n",Component->ComponentName,NumHits);
    }

	// process new hits if the crew hasn't bailed out
	if (NumHits > 0 && !vehmainHasBailedOut())
	{
// at this point for the hacky atgs (component->isPak38_6lb_57mm is true ) convert the multiple parts into the 4 standard ones

        if ( Component->isPak38_6lb_57mm )
        {
            FLOAT64 damagecopy[10];
            size_t lll = sizeof(Component->NewDamage);
            memcpy( damagecopy, &Component->NewDamage[0], lll );
            Component->NewDamage[0] = Component->NewDamage[1] = Component->NewDamage[2] = Component->NewDamage[3] = 0;

            for (i=0;i<10;i++)
            {
                if (damagecopy[i] > 0.0001)
                {
                    int damagecomponentNumber = Component->DamageComponentNum[i]-1;
                    if ( damagecomponentNumber>=0 )
                    {
                        char* name = (char*) &VehicleData->VehicleMasterPtr->VehicleType->DamageObjects[damagecomponentNumber][0];
                        if ( strstr( name, "head" ) )
                        {
                            Component->NewDamage[0] += damagecopy[i];
                        }
                        else if ( strstr( name, "torso" ) )
                        {
                            Component->NewDamage[1] += damagecopy[i];
                        }
                        else if ( strstr( name, "arm" ) )
                        {
                            Component->NewDamage[2] += damagecopy[i];
                        }
                        else if ( strstr( name, "leg" ) )
                        {
                            Component->NewDamage[3] += damagecopy[i];
                        }
                        else
                            continue;
                        //printf("processing damage to %s with joules %lf\n",name,damagecopy[i]);
                    }
                }
            }
        }

		for (i=0;i<4;i++)
		{
			if (Component->NewDamage[i] > 0.0001)
			{
				isDead = persGetCrewIsDead(CrewPosition);

				persApplyDamage((FLOAT32)Component->NewDamage[i],CrewPosition,i, FALSE);

				if(!isDead) // if you're dead what do you care? :)
				{
					uiChatPrintLine(loclGetLocalizationStringByID(809), CrewPosition+1, uiLocString(Location[i]).c_str());
				}
				else
				{
                    //printf( "%s is dead crew #%d\n",VehicleData->VehicleName,CrewPosition+1 );
					// make sure they stop firing when they die
					VehicleData->PrimaryTriggerDown		= 0;
					VehicleData->PicklePrimary			= 0;
					VehicleData->SecondaryTriggerDown	= 0;
					VehicleData->PickleSecondary		= 0;
				}
			}
		}
	}

	LOGICAL* posDead = (LOGICAL*)Component->SpecialData;
	if(!*posDead && persGetCrewIsDead(CrewPosition))
	{
		snprintf(s, 127, "crew%u",CrewPosition+1);
		vehVisDamageGroupDestroyed(VehicleData->VehicleMasterPtr, s, TRUE, TRUE);
		*posDead = TRUE;
	}

	if (VehicleData->KillCrew == TRUE || VehicleData->LastKillCrew == TRUE)
	{
		if (Component->DamageComponentNum[0] > 0)
		{
			vehApplyDamageToVehicleComponent(VehicleData->VehicleMasterPtr,
									  Component->DamageComponentNum[0],
									  10000,
									  0);
		}
	}

	persSetPayAttentionFlag(CrewPosition);

	// handle vehicle dead stuff; if you haven't bailed out
	if (!vehmainHasBailedOut() && (Component->CriticalSecondary != 0 || Component->CriticalPrimary != 0))
	{
		if (persGetCrewDeadForKillScore(CrewPosition) == 1)
		{
			if (Component->CriticalPrimary != 0)
			{
#ifdef BETA_RELEASE
				if(!VehicleData->VehicleIsDead)
					uiChatPrintLine("Vehicle Killed by crew critical primary");
#endif
				VehicleData->VehicleIsDead = 1;
				_DeclareVehicleKilled (VehicleData, Component);
			}
			if (Component->CriticalSecondary != 0)
			{
				VehicleData->SecondaryCriticalComponentCount[Component->CriticalSecondary-1]++;
			}
		}
		_CheckForSecondaryVehicleKill (VehicleData, Component);
	}

	return 0;
}


UINT32 _UpdatePitotTube(mlComponent * Component,mlVehicleData * VehicleData)
{
FLOAT64VECTOR ComponentVel = {0, 0, 0};
FLOAT64 TempNum = 0;
FLOAT64 Density1 = 0,Density2 = 0;

	if (VehicleData->MultiCrewPos < 2)
	{
		mlCompCalcComponentVelocityVector(&ComponentVel,Component,VehicleData);
		TempNum = ComponentVel.y;

		mlGetAltDensity(&Density1,0);
		mlGetAltDensity(&Density2,VehicleData->VehiclePos.z);
		TempNum *= sqrt(fabs(Density2 / Density1));

		mlGaugeUpdatePlayerSpeed(TempNum);
	}
	else
	{
		TempNum = (FLOAT32)(mathExactVectorLength(&VehicleData->VehicleRealVel));

		mlGetAltDensity(&Density1,0);
		mlGetAltDensity(&Density2,VehicleData->VehiclePos.z);
		TempNum *= sqrt(fabs(Density2 / Density1));

		mlGaugeUpdatePlayerSpeed(TempNum);
	}

	return 0;
}

#pragma mark -

UINT32 mlGetElevatorDeflection(FLOAT64 * ReturnVal,FLOAT64 DeltaTime,mlVehicleData * VehicleData)
{
	*ReturnVal = VehicleData->ElevatorPos;
	return 0;
}

UINT32 mlGetRudderDeflection(FLOAT64 * ReturnVal,FLOAT64 DeltaTime,mlVehicleData * VehicleData)
{
	*ReturnVal = VehicleData->RudderPos;
	return 0;
}

UINT32 mlGetAileronDeflection(FLOAT64 * ReturnVal,FLOAT64 DeltaTime,mlVehicleData * VehicleData)
{
	*ReturnVal = VehicleData->AileronPos;
	return 0;
}

UINT32 mlGetThrottlePosition(FLOAT64 * ReturnVal,FLOAT64 DeltaTime,mlVehicleData * VehicleData)
{
	*ReturnVal = VehicleData->ThrottlePos;
	return 0;
}

UINT32 mlGetFlapPosition(FLOAT64 * ReturnVal,mlVehicleData * VehicleData)
{
	*ReturnVal = VehicleData->FlapPosition;
	return 0;
}

static UINT32 _TunnelResetVehiclePos(mlVehicleData * VehicleData,FLOAT64 Alt)
{

	VehicleData->VehicleAtt.Roll = VehicleData->VehicleAtt.Pitch = VehicleData->VehicleAtt.Yaw = 0;
	VehicleData->VehicleAttVel.Roll = VehicleData->VehicleAttVel.Pitch = VehicleData->VehicleAttVel.Yaw = 0;
	VehicleData->VehicleForce.x = VehicleData->VehicleForce.y = VehicleData->VehicleForce.z = 0;
	VehicleData->VehicleTorque.Roll = VehicleData->VehicleTorque.Pitch = VehicleData->VehicleTorque.Yaw = 0;
	VehicleData->VehicleVel.x = VehicleData->VehicleVel.y = VehicleData->VehicleVel.z = 0;
	VehicleData->VehiclePos.x = VehicleData->VehiclePos.y = 0;
	VehicleData->VehiclePos.z = Alt;
	VehicleData->AdjustedForCGPos.z = Alt;

	return 0;
}

UINT32 _UpdateMovingParts(mlVehicleData * VehicleData,FLOAT64 DeltaTime)
{
	FLOAT64 Delta;
	FLOAT64 Traverse,Elevate;
	FLOAT64 TempNum;
	FLOAT64 Rate;
	FLOAT64 CurrentPos;
	UINT32 i;
	UINT32 GotoNeutral;
	vehVehicle* Vehicle = VehicleData->VehicleMasterPtr;

	// Update Flaps
	if (VehicleData->FlapPosition < VehicleData->FlapSetting)
	{
		Delta = VehicleData->FlapSetting - VehicleData->FlapPosition;
		TempNum = VehicleData->FlapDeploySpeed * DeltaTime / 1000.0;

		if (Delta > TempNum)
			VehicleData->FlapPosition += TempNum;
		else
			VehicleData->FlapPosition = VehicleData->FlapSetting;
	}

	if (VehicleData->FlapPosition > VehicleData->FlapSetting)
	{
		Delta = VehicleData->FlapPosition - VehicleData->FlapSetting;
		TempNum = VehicleData->FlapDeploySpeed * DeltaTime / 1000.0;

		if (Delta > TempNum)
			VehicleData->FlapPosition -= TempNum;
		else
			VehicleData->FlapPosition = VehicleData->FlapSetting;
	}

	// Update Gear
	if (VehicleData->GearPosition < VehicleData->GearSetting)
	{
		Delta = VehicleData->GearSetting - VehicleData->GearPosition;
		TempNum = VehicleData->GearDeploySpeed * DeltaTime / 1000.0;

		if (Delta > TempNum)
			VehicleData->GearPosition += TempNum;
		else
			VehicleData->GearPosition = VehicleData->GearSetting;
	}

	if (VehicleData->GearPosition > VehicleData->GearSetting)
	{
		Delta = VehicleData->GearPosition - VehicleData->GearSetting;
		TempNum = VehicleData->GearDeploySpeed * DeltaTime / 1000.0;

		if (Delta > TempNum)
			VehicleData->GearPosition -= TempNum;
		else
			VehicleData->GearPosition = VehicleData->GearSetting;
	}

	// do a hoist collision check and reset hoist turrets to their last positions
	// if a collision occurs
	if(Vehicle->VehicleObjectData->HoistCollidePntList)
		mlHoistCollisions(Vehicle);

	bool ezModeCommandView = (gEZMode && EZ_MGR) ? EZ_MGR->IsCommandView() : false;

	// update turrets
	if (Vehicle && Vehicle->VehicleType && (Vehicle->VehicleType->Type != VEH_TYPE_PERSON))
	{
		for (i=0;i<ML_MAX_CREW_POSITIONS;i++)
		{
			if(ezModeCommandView)
				EZ_MGR->PreUpdateTurret(i);

			VehicleData->LastTurretTraverse[i] = VehicleData->TurretTraverse[i];
			VehicleData->LastTurretElevate[i]  = VehicleData->TurretElevate[i];
			VehicleData->LastTurretTraverseWithStops[i] = VehicleData->TurretTraverseWithStops[i];

			if (VehicleData->MultiCrewPos != 0)
			{
				if (Vehicle->VehicleType->MultiCrewPos[i] != VehicleData->MultiCrewPos)
				{
					VehicleData->TurretElevate[i] = Vehicle->StateAnimation.LastTurretElev[i];
					VehicleData->TurretElevateRate[i] = Vehicle->StateAnimation.LastTurretElev[i];
					VehicleData->TurretTraverse[i] = Vehicle->StateAnimation.LastTurretTraverse[i];
					VehicleData->TurretTraverseRate[i] = Vehicle->StateAnimation.LastTurretTraverse[i];
					VehicleData->TurretTraverseWithStops[i] = Vehicle->StateAnimation.LastTurretTraverse[i];

					continue;
				}
			}
			GotoNeutral = 0;
			if (Vehicle->VehicleType->TurretNeutral[i] != 0)
			{
				if (VehicleData->CanopyOpen[i])
				{
					if (Vehicle->VehicleType->TurretNeutral[i] == VEH_TURRET_NEUTRAL_ON_CANOPYOPEN)
						GotoNeutral = 1;
				}else
				{
					if (Vehicle->VehicleType->TurretNeutral[i] == VEH_TURRET_NEUTRAL_ON_CANOPYCLOSED)
						GotoNeutral = 1;
				}

				if (VehicleData->HatchOpen[i])
				{
					if (Vehicle->VehicleType->TurretNeutral[i] == VEH_TURRET_NEUTRAL_ON_HATCHOPEN)
						GotoNeutral = 1;
				}else
				{
					if (Vehicle->VehicleType->TurretNeutral[i] == VEH_TURRET_NEUTRAL_ON_HATCHCLOSED)
						GotoNeutral = 1;
				}

				if (VehicleData->BombBayDoor)
				{
					if (Vehicle->VehicleType->TurretNeutral[i] == VEH_TURRET_NEUTRAL_ON_BOMBBAYOPEN)
						GotoNeutral = 1;
				}else
				{
					if (Vehicle->VehicleType->TurretNeutral[i] == VEH_TURRET_NEUTRAL_ON_BOMBBAYCLOSED)
						GotoNeutral = 1;
				}
			}

			// traverse
			Delta = 0.0;
			if (Vehicle->VehicleType->TurretTraverseRate[i] > 0.0)
				Rate = (1.0 / Vehicle->VehicleType->TurretTraverseRate[i]);
			else
				Rate = 0.0;

			Traverse = VehicleData->TurretTraverseRate[i];

			if (Traverse > 0.0)
			{
				if (Traverse > 0.1)
					Delta = (Traverse - 0.1) / 0.9 *  Rate * DeltaTime / 1000.0;
			}else
			{
				if (Traverse < -0.1)
					Delta = (Traverse + 0.1) / 0.9 * Rate * DeltaTime / 1000.0;
			}

			if (GotoNeutral)
			{
				Delta = 0.0;
				if (Vehicle->VehicleType->TurretType[i] == VEH_TURRET_TYPE_LIMITED)
				{
					CurrentPos = VehicleData->TurretTraverseWithStops[i];
				} else
				{
					CurrentPos = VehicleData->TurretTraverse[i];

					if (CurrentPos > 0.5)
						CurrentPos -= 1.0;
				}
				if (CurrentPos > Vehicle->VehicleType->TurretTraverseNeutral[i])
				{
					if (CurrentPos > Vehicle->VehicleType->TurretTraverseNeutral[i] + Rate * DeltaTime / 1000.0)
						Delta = -Rate * DeltaTime / 1000.0;
					else
						Delta = Vehicle->VehicleType->TurretTraverseNeutral[i] - CurrentPos;
				}
				if (CurrentPos < Vehicle->VehicleType->TurretTraverseNeutral[i])
				{
					if (CurrentPos < Vehicle->VehicleType->TurretTraverseNeutral[i] - Rate * DeltaTime / 1000.0)
						Delta = Rate * DeltaTime / 1000.0;
					else
						Delta = Vehicle->VehicleType->TurretTraverseNeutral[i] - CurrentPos;
				}
			}
			VehicleData->TurretTraverse[i] += (FLOAT32)Delta;
			VehicleData->TurretTraverseWithStops[i] += (FLOAT32)Delta;

			switch(Vehicle->VehicleType->TurretType[i])
			{
				case VEH_TURRET_TYPE_360:
				{
					if(!GotoNeutral && VehicleData->TurretTraverseDeltaInput[i])
					{
						if(	(VehicleData->LastTurretTraverse[i] > VehicleData->WantedTurretTraverse[i] &&
							 VehicleData->WantedTurretTraverse[i] > VehicleData->TurretTraverse[i]) ||
							(VehicleData->LastTurretTraverse[i] < VehicleData->WantedTurretTraverse[i] &&
							 VehicleData->WantedTurretTraverse[i] < VehicleData->TurretTraverse[i]) )
						{
							VehicleData->TurretTraverse[i] = VehicleData->WantedTurretTraverse[i];
							VehicleData->TurretTraverseRate[i] = 0;
						}
					}else
					{
						VehicleData->WantedTurretTraverse[i] = VehicleData->LastTurretTraverse[i];
					}
				}
				break;

				default:
				{
					if(!GotoNeutral && VehicleData->TurretTraverseDeltaInput[i])
					{
						if(	(VehicleData->LastTurretTraverseWithStops[i] > VehicleData->WantedTurretTraverse[i] &&
							 VehicleData->WantedTurretTraverse[i] > VehicleData->TurretTraverseWithStops[i]) ||
							(VehicleData->LastTurretTraverseWithStops[i] < VehicleData->WantedTurretTraverse[i] &&
							 VehicleData->WantedTurretTraverse[i] < VehicleData->TurretTraverseWithStops[i]) )
						{
							VehicleData->TurretTraverseWithStops[i] = VehicleData->WantedTurretTraverse[i];
							VehicleData->TurretTraverseRate[i] = 0;
						}
					}else
					{
						VehicleData->WantedTurretTraverse[i] = VehicleData->LastTurretTraverseWithStops[i];
					}
				}
			}

			if (VehicleData->TurretTraverse[i] >  0.5) 	VehicleData->TurretTraverse[i] -= 1.0;
			if (VehicleData->TurretTraverse[i] < -0.5) 	VehicleData->TurretTraverse[i] += 1.0;
			if (VehicleData->TurretTraverseWithStops[i] >  1.0)	VehicleData->TurretTraverseWithStops[i] =  1.0;
			if (VehicleData->TurretTraverseWithStops[i] < -1.0)	VehicleData->TurretTraverseWithStops[i] = -1.0;

			// all turrets start at the neutral position
			if (!VehicleData->TurretNotFirstTime)
			{
				VehicleData->LastTurretTraverse[i] = VehicleData->TurretTraverse[i] = Vehicle->VehicleType->TurretTraverseNeutral[i];
				VehicleData->LastTurretTraverseWithStops[i] = VehicleData->TurretTraverseWithStops[i] = Vehicle->VehicleType->TurretTraverseNeutral[i];
			}

			// elevation
			Delta = 0.0;
			if (Vehicle->VehicleType->TurretElevateRate[i] > 0.0)
				Rate = (1.0 / Vehicle->VehicleType->TurretElevateRate[i]);
			else
				Rate = 0.0;

			Elevate = VehicleData->TurretElevateRate[i];
			if (Elevate > 0.0)
			{
				if (Elevate > 0.1)
					Delta = (Elevate - 0.1) / 0.9 * Rate * DeltaTime / 1000.0;
			} else
			{
				if (Elevate < -0.1)
					Delta = (Elevate + 0.1) / 0.9 * Rate * DeltaTime / 1000.0;
			}

			if (GotoNeutral)
			{
				Delta = 0.0;
				CurrentPos = VehicleData->TurretElevate[i];
				if (CurrentPos > Vehicle->VehicleType->TurretElevateNeutral[i])
				{
					if (CurrentPos > Vehicle->VehicleType->TurretElevateNeutral[i] + Rate * DeltaTime / 1000.0)
						Delta = -Rate * DeltaTime / 1000.0;
					else
						Delta = Vehicle->VehicleType->TurretElevateNeutral[i] - CurrentPos;
				}
				if (CurrentPos < Vehicle->VehicleType->TurretElevateNeutral[i])
				{
					if (CurrentPos < Vehicle->VehicleType->TurretElevateNeutral[i] - Rate * DeltaTime / 1000.0)
						Delta = Rate * DeltaTime / 1000.0;
					else
						Delta = Vehicle->VehicleType->TurretElevateNeutral[i] - CurrentPos;
				}
			}

			VehicleData->TurretElevate[i] += (FLOAT32)Delta;

			if (!VehicleData->TurretNotFirstTime)
			{
				VehicleData->LastTurretElevate[i] = VehicleData->TurretElevate[i] = Vehicle->VehicleType->TurretElevateNeutral[i];
			}

			// Handle mouse (delta) input
			if(!GotoNeutral && VehicleData->TurretElevateDeltaInput[i])
			{
				if(	(VehicleData->LastTurretElevate[i] > VehicleData->WantedTurretElevate[i] &&  VehicleData->WantedTurretElevate[i] > VehicleData->TurretElevate[i]) ||
					(VehicleData->LastTurretElevate[i] < VehicleData->WantedTurretElevate[i] &&  VehicleData->WantedTurretElevate[i] < VehicleData->TurretElevate[i]) )
				{
					 VehicleData->TurretElevate[i] = VehicleData->WantedTurretElevate[i];
					 VehicleData->TurretElevateRate[i] = 0;
				}
			}else
			{
				VehicleData->WantedTurretElevate[i] = VehicleData->TurretElevate[i];
			}

			if (VehicleData->TurretElevate[i] >  1.0)	VehicleData->TurretElevate[i] =  1.0;
			if (VehicleData->TurretElevate[i] < -1.0)	VehicleData->TurretElevate[i] = -1.0;

			if(ezModeCommandView)
				EZ_MGR->PostUpdateTurret(i);
		}
	}
	VehicleData->TurretNotFirstTime = 1;


	// update canopy
	for (i=0;i<ML_MAX_CREW_POSITIONS;i++)
	{
		if (VehicleData->CanopyPosition[i] < VehicleData->CanopyOpen[i])
		{
			Delta = VehicleData->CanopyOpen[i] - VehicleData->CanopyPosition[i];

			TempNum = VehicleData->CanopyDeploySpeed * DeltaTime / 1000.0;

			if (Delta > TempNum)
				VehicleData->CanopyPosition[i] += TempNum;
			else
				VehicleData->CanopyPosition[i] = VehicleData->CanopyOpen[i];
		}

		if (VehicleData->CanopyPosition[i] > VehicleData->CanopyOpen[i])
		{
			Delta = VehicleData->CanopyPosition[i] - VehicleData->CanopyOpen[i];

			TempNum = VehicleData->CanopyDeploySpeed * DeltaTime / 1000.0;

			if (Delta > TempNum)
				VehicleData->CanopyPosition[i] -= TempNum;
			else
				VehicleData->CanopyPosition[i] = VehicleData->CanopyOpen[i];
		}
	}

	return 0;
}

UINT32 mlengineGetComponents(mlVehicleData* VehicleData, mlComponentVector& list, SINT32 type, SINT32 subType)
{
	mlComponent* Component;
	mlEngineComponentData* Engine;
	//mlEngineData* EngineData;
	mlComponentListIterator it;

	mlComponentListBegin(VehicleData->ComponentList, &it);
	for(; it.comp; mlComponentListNext(&it))
	{
		Component = it.comp;

		if(Component->Type == type)
		{
			Engine = (mlEngineComponentData*)(Component->SpecialData);

			if(Engine->SubType == subType)
				list.push_back(Component);
		}
	}

	return (UINT32)list.size();
}

// pass -1 if you want the first found engine
mlEngineComponentData* mlGetEngine(mlVehicleData* VehicleData, SINT32 EngineNumber)
{
	mlComponent * Component;
	mlEngineComponentData * Engine;
	//mlEngineData * EngineData;
	//mlEngineComponentData * TorqueReceiver;
	mlComponentListIterator it;

	mlComponentListBegin(VehicleData->ComponentList, &it);
	for(; it.comp; mlComponentListNext(&it))
	{
		Component = it.comp;

		if(Component->Type == ML_COMPONENT_ENGINE)
		{
			Engine = (mlEngineComponentData*)(Component->SpecialData);

			if(Engine->SubType == ML_ENGINE_SUBTYPE_ENGINE)
			{
				if(EngineNumber == -1 || Engine->EngineNumber == (UINT32)EngineNumber)
					return Engine;
			}
		}
	}

	return nullptr;
}

// pass -1 if you want to know if any engine is on
UINT32 mlGetEnginesOn(mlVehicleData* VehicleData, SINT32 EngineNumber)
{
	mlComponent * Component;
	mlEngineComponentData * Engine;
	mlEngineData * EngineData;
	mlEngineComponentData * TorqueReceiver;
	mlComponentListIterator it;

	mlComponentListBegin(VehicleData->ComponentList, &it);
	for(; it.comp; mlComponentListNext(&it))
	{
		Component = it.comp;

		if(Component->Type == ML_COMPONENT_ENGINE)
		{
			Engine = (mlEngineComponentData*)(Component->SpecialData);

			if(Engine->SubType == ML_ENGINE_SUBTYPE_ENGINE)
			{
				if(EngineNumber == -1 || Engine->EngineNumber == (UINT32)EngineNumber)
				{
					EngineData = Engine->EngineData;
					TorqueReceiver = (mlEngineComponentData*)Engine->PropellorComponent->SpecialData;

					if(Engine->EngineCaught && TorqueReceiver->CurrentRpm >= EngineData->CriticalRpm)
						return 1;
				}
			}
		}
	}

	return 0;
}

UINT32 mlGetNumGears(mlVehicleData* VehicleData, UINT32* current)
{
	mlComponent* Component;
	mlEngineComponentData* EngineComponentData;
	mlComponentListIterator it;
	UINT32 numGears = 0;

	mlComponentListBegin(VehicleData->ComponentList, &it);

	for(; it.comp; mlComponentListNext(&it))
	{
		Component = it.comp;

		if(Component->Type == ML_COMPONENT_ENGINE)
		{
			EngineComponentData = (mlEngineComponentData*)Component->SpecialData;

			if(EngineComponentData->SubType == ML_ENGINE_SUBTYPE_GEARBOX)
				numGears += (EngineComponentData->NumForwardGears + EngineComponentData->NumReverseGears);
		}
	}

	return numGears;
}

UINT32 mlGetEngineRPMs(mlVehicleData* VehicleData, SINT32 EngineNumber)
{
	mlComponent * Component;
	mlEngineComponentData * Engine;
	mlEngineData * EngineData;
	mlEngineComponentData * TorqueReceiver;
	mlComponentListIterator it;

	UINT32 currentRPMS = 0;
	UINT32 numEngines = 0;

	mlComponentListBegin(VehicleData->ComponentList, &it);
	for(; it.comp; mlComponentListNext(&it))
	{
		Component = it.comp;

		if(Component->Type == ML_COMPONENT_ENGINE)
		{
			Engine = (mlEngineComponentData*)(Component->SpecialData);

			if(Engine->SubType == ML_ENGINE_SUBTYPE_ENGINE)
			{
				if(EngineNumber == -1 || Engine->EngineNumber == (UINT32)EngineNumber)
				{
					EngineData = Engine->EngineData;
					TorqueReceiver = (mlEngineComponentData*)Engine->PropellorComponent->SpecialData;

					if(Engine->EngineCaught && TorqueReceiver->CurrentRpm >= EngineData->CriticalRpm)
					{
						currentRPMS += (UINT32)TorqueReceiver->CurrentRpm;
						numEngines++;
					}
				}
			}
		}
	}

	return (numEngines) ? (UINT32)(currentRPMS / numEngines) : 0;
}

UINT32 mlGetStarterOnInfo(mlVehicleData* VehicleData, UINT32 * StarterOn,UINT32 EngineNumber)
{
	UINT32 j;
	mlComponent * Component;
	mlEngineComponentData * Engine;
	mlEngineData * EngineData;
	mlEngineComponentData * TorqueReceiver;
	mlEngineComponentData * TankData;
	LOGICAL AllEnginesOn;
	LOGICAL EngineHasFuel;
	mlComponentListIterator it;

	// first determine if all capable engines are running, if so, we're shutting down the engines
	AllEnginesOn = TRUE;

	mlComponentListBegin(VehicleData->ComponentList, &it);
	for(; it.comp; mlComponentListNext(&it))
	{
		Component =it.comp;

		if (Component->Type == ML_COMPONENT_ENGINE)
		{
			Engine = (mlEngineComponentData*)(Component->SpecialData);
			if (Engine->SubType == ML_ENGINE_SUBTYPE_ENGINE)
			{
				EngineData = Engine->EngineData;
				TorqueReceiver = (mlEngineComponentData*)Engine->PropellorComponent->SpecialData;

				if (Engine->EngineCaught == 1 &&
					TorqueReceiver->CurrentRpm > EngineData->CriticalRpm)
				{
					continue;
				}
				if (Component->CurrentDamage > Component->DamageLevels[2])
				{
					// we ignore damaged engines
					continue;
				}
				EngineHasFuel = FALSE;
				for (j=0;j<ML_MAX_FUEL_TANKS_PER_ENGINE;j++)
				{
					if (Engine->FuelTankComponent[j] != nullptr)
					{
						TankData = (mlEngineComponentData*)Engine->FuelTankComponent[j]->SpecialData;
						if (TankData->CurrentTankContents > 0.0)
						{
							EngineHasFuel = TRUE;
							break;
						}
					}
				}
				if (EngineHasFuel == FALSE)
				{
					continue;
				}

				AllEnginesOn = FALSE;
			}
		}
	}

	if (AllEnginesOn == TRUE)
	{
		VehicleData->StartersOn = 0;
		if (VehicleData->MasterStarterOn == 1)
		{
			VehicleData->ShutdownEngines = 1;
		}
		VehicleData->MasterStarterOn = 0;
	}
	else
	{
		if (VehicleData->MasterStarterOn == 1)
		{
			VehicleData->StartersOn = 1;
		}
	}

//	AllEnginesOn = TRUE;
	*StarterOn = 0;

	mlComponentListBegin(VehicleData->ComponentList, &it);
	for(; it.comp; mlComponentListNext(&it))
	{
		Component = it.comp;

		if (Component->Type == ML_COMPONENT_ENGINE)
		{
			Engine = (mlEngineComponentData*)(Component->SpecialData);
			if (Engine->SubType == ML_ENGINE_SUBTYPE_ENGINE)
			{
				if (Engine->EngineNumber == EngineNumber)
				{
					if (VehicleData->StartersOn == 1)
					{
						EngineData = Engine->EngineData;
						TorqueReceiver = (mlEngineComponentData*)Engine->PropellorComponent->SpecialData;
						if (Engine->EngineCaught == 0 || TorqueReceiver->CurrentRpm < EngineData->CriticalRpm)
						{
							*StarterOn = 1;
							break;
						}
						else
						{
							*StarterOn = 0;
							break;
						}
					}
					else
					{
						if (Engine->EngineCaught == 1 && VehicleData->ShutdownEngines == 1)
						{
							*StarterOn = 1;
							break;
						}
						else
						{
							*StarterOn = 0;
							break;
						}
					}
				}
			}
		}
	}

	return 0;
}

UINT32 mlSetStarterOff(UINT32 EngineNumber)
{
	mlVehicleData * VehicleData = persGetPersonaModelDataPtr();
	VehicleData->StarterOn[EngineNumber-1] = 0;

	return 0;
}


UINT32 mlIsTailWheelLocked()
{
	mlVehicleData * VehicleData = persGetPersonaModelDataPtr();

	return VehicleData->TailWheelLocked;
}

//#define TO_COLLIDE_LIST_DEBUG

struct TOPair
{
	trnCDTerrainObject* TO;
	FLOAT64 distance;
};

using TOList = std::list<TOPair>;

static bool to_sort(const TOPair& a, const TOPair& b)
{
	return a.distance < b.distance;
}

static void _createBoxCollider(trnCDTerrainObject* TO)
{
	if(!TO || !TO->InUse || !TO->CollisionObject || TO->physicsBoxCollider)
		return;

	const FLOAT64VECTOR& wo = physicsGetWorldOffset();
	//piMATRIX& m = TO->CollisionObject->mat;

    piVECTOR v1;
    piVECTOR v2;

    v1.x = TO->BBox1.x + TO->Center.x - TO->OrigCenter.x;
    v1.y = TO->BBox1.y + TO->Center.y - TO->OrigCenter.y;
    v1.z = TO->BBox1.z + TO->Center.z - TO->OrigCenter.z;

    v2.x = TO->BBox2.x + TO->Center.x - TO->OrigCenter.x;
    v2.y = TO->BBox2.y + TO->Center.y - TO->OrigCenter.y;
    v2.z = TO->BBox2.z + TO->Center.z - TO->OrigCenter.z;

	double xLen = fabs(v1.x - v2.x);
	double yLen = fabs(v1.y - v2.y);
	double zLen = fabs(v1.z - v2.z);

	// create our box collider
	dGeomID box = dCreateBox(colliderTerrainSimpleSpace(), xLen, yLen, zLen);

	// set its position to the TO's center point
	dGeomSetPosition(box
		,  WorldOriginOffset.x+TO->Center.x-wo.x
		,  WorldOriginOffset.y+TO->Center.y-wo.y
		,-(WorldOriginOffset.z+TO->Center.z-wo.z) );

	/* bbox is already oriented; dont need this
	dMatrix3 mat;
	mat[0] = m._11;
	mat[1] = m._21;
	mat[2] = m._31;
	mat[3] = 0;
	mat[4] = m._12;
	mat[5] = m._22;
	mat[6] = m._32;
	mat[7] = 0;
	mat[8] = -m._13;
	mat[9] = -m._23;
	mat[10]= -m._33;
	mat[11] = 0;

	// set the blank's rotation
	dGeomSetRotation(box, mat);
	*/

	// make our collision object the user data
	dGeomSetData(box, TO->CollisionObject);

	// set our collide category bits
	colliderSetBits(box, COLLIDER_BOX, 0);

	// save it so we can kill it later
	TO->physicsBoxCollider = box;
}

static bool _killODETOCollider(trnCDTerrainObject* TO)
{
	// if we have a blank collider, go ahead and kill it since it's simple
	if(TO->physicsBoxCollider)
	{
		// kill the blank collider
		dGeomDestroy((dGeomID)TO->physicsBoxCollider);
		TO->physicsBoxCollider = 0;
	}

	if(TO->physicsColliderSpace)
	{
		trncdRemoveODEColliderQueued(*TO);
		return true;
	}

	return false;
}

/*
    PARALLEL_BUILDER is the code (minus the #pragma omp parallel for) to support multi-threaded collider building.
    Current we need to upgrade to a newer version of ODE to use its multi-threaded support before this is usable
 */

#ifdef PARALLEL_BUILDER

struct Temp_Info
{
    dSpaceID space;
    bool addBoxCollider;
    trnCDTerrainObject* TO;
    UINT32 odecat;
    Temp_Info( trnCDTerrainObject*o, UINT32 cat, bool addbox, dSpaceID sid ) { TO=o; odecat=cat; addBoxCollider=addbox; space = sid; }
};

using TEMP_VECTOR = std::vector<Temp_Info>;

#endif

static UINT32 _UpdateTerrainCollideLists(mlVehicleData* VehicleData, UINT32 ForceRecalc)
{
	if (VehicleData == nullptr)
		return 0;

	trnCDTerrainObject* TO = nullptr;
	FLOAT64VECTOR Vec1;
	piVECTOR PosVec;
	FLOAT64 Dist1 = 0.0f;
	OBJECT* TempObj = nullptr;
	LOGICAL IgnoreZ = FALSE;
	WEAPON_SPEC Spec;
	WEAPON_SPEC SpecType;

#ifdef TO_COLLIDE_LIST_DEBUG
	int mediumCount = 0;
	int shortCount = 0;
	int odeAdds = 0;
	int odeRemoves = 0;
	int odeTotal = 0;
	int odeBlanks = 0;
	int totalTOs = 0;
	int timeDiff = 0;
	int rtCount = 0;
	FLOAT64 distanceDiff = 0.0f;
#endif

// Add for other vehicles
#define DESTROYER		0x0001
#define NO_ODE			0x0002

    // Special case these vehicles for long range weapon effects. Add if necessary
    static WEAPON_SPEC destroyer = NULL_WEAP_SPEC;

    static LOGICAL firsttime = 1;
    vehVehicle*v = persGetPersonaVehiclePtr();

    if(firsttime)
    {
        firsttime = 0;

        // Add others here
        destroyer = weapCreateSpec("SV_sea.1");
    }

    UINT32 type = 0;

    WEAPON_SPEC* s = nullptr;
	UINT32 vtype = VEH_TYPE_NONE;

    if(v)
    {
		if(v->VehicleType)
		{
			vtype = v->VehicleType->Type;
	        s = &v->VehicleType->WeaponSpec;
		}

        if(s && destroyer.wClass == s->wClass &&
           destroyer.wType == s->wType &&
           destroyer.wCategory == s->wCategory)
        {
            type |= DESTROYER;
        }
		/*
		else if(vtype == VEH_TYPE_AIR)
		{
			if(mathExactVectorLength2(&v->ModelData->VehicleVel) > 20.0f * 20.0f || v->isHulked())
				type |= NO_ODE;
		}
		*/
    }
	else
	{
		// no vehicle, no ode colliders
		type |= NO_ODE;
	}

	// we don't need to update colliders if we haven't moved far or if there have been no strat state changes
	static FLOAT64VECTOR lastPosition = {0,0,0};

	bool updateLists = (ForceRecalc == TRUE);

#ifdef TO_COLLIDE_LIST_DEBUG
	timeDiff = VehicleData->LastVehicleUpdateTime - _LastTOUpdateTime;
	distanceDiff = v->DistanceTo(lastPosition.x, lastPosition.y, v->ModelData->VehiclePos.z);
#endif

	// do this block's checks not more than once a second
	if(!updateLists && VehicleData->LastResetTime && (VehicleData->LastVehicleUpdateTime - _LastTOUpdateTime) >= 1000)
	{
		// first lets see if we need to update our collide lists because a strat event occurred (damage levels, etc)
		updateLists = trnCDUpdateCollideLists();

		// nope, now let's see if the we've moved far enough to recalculate the lists (only checks xy plane)
		if(!updateLists && v->DistanceTo(lastPosition.x, lastPosition.y, v->ModelData->VehiclePos.z) >= ML_MOVE_COLLIDE_CHECK)
			updateLists = true;

		// update the lists for the first x seconds after spawning in (in case we have ordering issues)
		if(!updateLists && (frametime - VehicleData->LastResetTime) <= 3000)
			updateLists = true;
	}

	// didn't find a reason to update, bailing!
	if(!updateLists)
		return 0;

	// save our new collider update position for the next check
	lastPosition = v->ModelData->VehiclePos;

	// reset our lists
    memset(VehicleData->MedTrnList,0,sizeof(trnCDTerrainObject*) * (ML_NUM_MED_LIST_ELEMENTS+1));
	memset(VehicleData->ShortTrnList,0,sizeof(trnCDTerrainObject*) * (ML_NUM_SHORT_LIST_ELEMENTS+1));

	// our temp TO list
	TOList toList;

	Spec = VehicleData->VehicleMasterPtr->VehicleType->WeaponSpec;
	SpecType = weapCreateSpec("AC_bomber.0");

	if (SpecType.wCategory == 0 && SpecType.wClass == 0)
	{
		IgnoreZ = FALSE;
	}
	else
	{
		if (Spec.wCategory == SpecType.wCategory && Spec.wClass == SpecType.wClass)
		{
			IgnoreZ = TRUE;
		}
		else
		{
			IgnoreZ = FALSE;
		}
	}

	if(vtype == VEH_TYPE_FLOATER)
		IgnoreZ = TRUE;

	LOGICAL onMedList = FALSE;
	TOPair toPair;

	// update the medium list
	trncdGetTOHead(&TO);

	// need to change this to a sorted list
	while (TO != nullptr)
	{
#ifdef TO_COLLIDE_LIST_DEBUG
		totalTOs++;
#endif
		// kill these flags by default
		TO->uFlags &= ~(TRN_OBJECT_ONMEDIUMLIST|TRN_OBJECT_ONSHORTLIST);

		onMedList = FALSE;

		// find an object with a matrix
		TempObj = TO->TerrainObject;//->child;

		while (TempObj != nullptr && TempObj->flags & OBJECT_NOTOBJECT)
		{
			TempObj = TempObj->child;
		}

        // Skip clouds and unused TO objects
        if(!TO->InUse || (TempObj->flag3 & (OBJECT_CLOUDS | OBJECT_RAINCLOUDS)) || (TO->uFlags & TRN_OBJECT_INACTIVE))
        {
			if(TO->uFlags & TRN_OBJECT_INACTIVE)
			{
#ifdef TO_COLLIDE_LIST_DEBUG
				if(TO->physicsColliderSpace)
					odeRemoves++;
#endif
				trncdRemoveODEColliderImmediate(*TO, true);
			}
			else if(_killODETOCollider(TO))
			{
#ifdef TO_COLLIDE_LIST_DEBUG
				odeRemoves++;
#endif
			}

            TO = TO->Next;
            continue;
        }

#ifdef TO_COLLIDE_LIST_DEBUG
		if(TO->physicsColliderSpace)
				odeTotal++;

		if(TO->physicsBoxCollider)
				odeBlanks++;
#endif

		if (TempObj != nullptr)
		{
			PosVec = TO->Center;
			PosVec.z = -PosVec.z;
			mathSubVectors(&Vec1,&PosVec,&VehicleData->VehiclePos);
			Vec1.z = -Vec1.z;
			mathAddVectors(&Vec1,&Vec1,&WorldOriginOffset);

			if (IgnoreZ == FALSE)
			{
				Dist1 = mathExactVectorLength(&Vec1) - TO->Radius;
			}
			else
			{
				Dist1 = sqrt(Vec1.x * Vec1.x + Vec1.y * Vec1.y) - TO->Radius;
			}

            // Special case destroyer so long range weapons will show effects
            if(type & DESTROYER)
            {
                if(Dist1 < ML_MED_TO_COLLIDE_DIST_DESTROYER)
					onMedList = TRUE;
            }
			else if (Dist1 < ML_MED_TO_COLLIDE_DIST)
			{
				onMedList = TRUE;
			}

			if(TO->uFlags & TRN_OBJECT_INACTIVE)
				onMedList = FALSE;

			// if we're on the medium list save it; otherwise queue the ODE collider for destruction (if we had one)
			if(onMedList)
			{
				toPair.TO = TO;
				toPair.distance = Dist1;

				toList.push_back(toPair);
			}
			else
			{
				if(_killODETOCollider(TO))
				{
#ifdef TO_COLLIDE_LIST_DEBUG
					odeRemoves++;
#endif
				}
			}
		}

		TO = TO->Next;
	}

	// now let's sort the list
	toList.sort(to_sort);

#ifdef PARALLEL_BUILDER
    TEMP_VECTOR saved;
#endif

    TOList::iterator lit = toList.begin();

	// now we'll populate our collide lists
	for(int toCount = 0; lit != toList.end(); ++lit, ++toCount)
	{
		// bail if we have reached the max medium list count
		if(toCount >= ML_NUM_MED_LIST_ELEMENTS)
			break;

#ifdef TO_COLLIDE_LIST_DEBUG
		mediumCount++;
#endif

		TOPair& p = *lit;

		TO = p.TO;
		Dist1 = p.distance;

		// note it
		TO->uFlags |= TRN_OBJECT_ONMEDIUMLIST;

		// place it on our list
		VehicleData->MedTrnList[toCount] = TO;

		if(Dist1 < ML_SHORT_TO_COLLIDE_DIST && toCount < ML_NUM_SHORT_LIST_ELEMENTS)
		{
#ifdef TO_COLLIDE_LIST_DEBUG
			shortCount++;
#endif
			// note it
			TO->uFlags |= TRN_OBJECT_ONSHORTLIST;

			// place it on our list
			VehicleData->ShortTrnList[toCount] = TO;
		}

		// now let's determine what kind of ODE collider to use
		UINT32 odeCat = 0; // ode collider category

		bool addBoxCollider		= false; // do we add a simple blank collider for this TO?
		bool isTerrainTile		= false;

		if(!(type & NO_ODE) && Dist1 <= ML_MED_TO_COLLIDE_DIST)
		{
			odeCat = physicsCollider::DetermineCategory(COLLIDER_TRN_GROUND, TO->CollisionObject);

			isTerrainTile	= colliderIsType(odeCat, COLLIDER_TILE);
			addBoxCollider	= colliderIsType(odeCat, COLLIDER_ENTERABLE);

#ifdef TO_COLLIDE_LIST_DEBUG
			if(isTerrainTile)
				rtCount++;
#endif

			if(vtype == VEH_TYPE_AIR)
			{
				if(!isTerrainTile)
					odeCat = 0; // only add ode colliders for terrain tiles within 2000 meters of you
			}
			/* all objects on the medium collide list now get ode colliders
			else
			{
				bool onShortList = ((TO->uFlags & TRN_OBJECT_ONSHORTLIST) > 0);

				// if we're not on the short, minimize far out colliders
				if(!isTerrainTile && !onShortList)
					odeCat = 0;
			}
			*/
		}

		if(odeCat)
		{
			// for calculating the detailed has space position
			FLOAT64VECTOR pos = {TO->Center.x+WorldOriginOffset.x,TO->Center.y+WorldOriginOffset.y,0};

			// put tiles in the terrain tile space
			dSpaceID space = (isTerrainTile) ? colliderTerrainTileSpace() : colliderForPosition(pos);

			if(space)
			{
				if(!TO->physicsColliderSpace)
				{
					if(TO->DamageLevel != FB_CLOSED_VISLEVEL)
					{
#ifdef TO_COLLIDE_LIST_DEBUG
						odeAdds++;
#endif

						if(TO->DamageLevel == 0 || (TO->CollisionObject == TO->DamageCollisionObject) )
						{
#ifdef PARALLEL_BUILDER
							saved.push_back (Temp_Info( TO, odeCat, addBoxCollider,space ) );
							addBoxCollider = false;
#else
							TO->physicsColliderSpace = physicsRecurseAndCreateObjectCollider(TO->CollisionObject, space,odeCat,0,false,true,true,TO); // slow path
#endif
						}
						else
							TO->physicsColliderSpace = physicsRecurseAndCreateObjectCollider(TO->DamageCollisionObject,space,odeCat,0,false,true,true,TO);
					}
				}
				else if(!(TO->uFlags & TRN_OBJECT_INACTIVE))
				{
					if(TO->DamageLevel == FB_CLOSED_VISLEVEL)
					{
						// damage state is not collidable anymore
						if(TO->physicsColliderSpace)
						{
#ifdef TO_COLLIDE_LIST_DEBUG
							odeRemoves++;
#endif
							trncdRemoveODEColliderQueued(*TO);
						}
					}
					else if((TO->CollisionObject != TO->DamageCollisionObject)
						&&	(TO->physicsColliderDamageLevel != TO->DamageLevel)
						&&	(TO->physicsColliderDamageLevel == 0 || TO->DamageLevel == 0) )
					{
						if(TO->DamageLevel == 0)
						{
							physicsRecurseAndFreeObjectCollider(TO->DamageCollisionObject);
							//dSpaceDestroy((dSpaceID)TO->physicsColliderSpace);
							//TO->physicsColliderSpace = physicsRecurseAndCreateObjectCollider(TO->CollisionObject,space,odeCat,0,false,true,true,TO);
							physicsRecurseAndCreateObjectCollider(TO->CollisionObject,(dSpaceID)TO->physicsColliderSpace,odeCat,0,false,true,false,TO);
						}
						else
						{
							physicsRecurseAndFreeObjectCollider(TO->CollisionObject);
							//dSpaceDestroy((dSpaceID)TO->physicsColliderSpace);
							//TO->physicsColliderSpace = physicsRecurseAndCreateObjectCollider(TO->DamageCollisionObject,space,odeCat,0,false,true,true,TO);
							physicsRecurseAndCreateObjectCollider(TO->DamageCollisionObject,(dSpaceID)TO->physicsColliderSpace,odeCat,0,false,true,false,TO);
						}

#ifdef TO_COLLIDE_LIST_DEBUG
							odeAdds++;
#endif
					}
				}
			}
			else
			{
				// Andrew, want to log something here?
			}

			TO->physicsColliderDamageLevel = TO->DamageLevel;
		}
		else if(TO->physicsColliderSpace)
		{
#ifdef TO_COLLIDE_LIST_DEBUG
			odeRemoves++;
#endif

			// we've determined our current collider flags and distance to the object means we don't need
			// this collider; queue it for destruction
			trncdRemoveODEColliderQueued(*TO);
		}

		if(addBoxCollider)
			_createBoxCollider(TO);
	}

#ifdef PARALLEL_BUILDER
    int tempcount = saved.size();
    for( TEMP_VECTOR::iterator it = saved.begin(); it!=saved.end(); it++)
    {
        Temp_Info&info = *it;
        info.TO->physicsColliderSpace
        = physicsRecurseAndCreateObjectCollider(info.TO->CollisionObject, info.space,info.odecat,0,false,true,true,info.TO);
        TO->physicsColliderDamageLevel = TO->DamageLevel;
        if(info.addBoxCollider)
            _createBoxCollider(info.TO);
    }
#endif

	_LastTOUpdateTime = VehicleData->LastVehicleUpdateTime;

	// create non-replacement tile collider
	for(int i = 0; i < NUM_TERRAIN_RECTANGLES; i++)
	{
		if(!activeTerrainDetail[i]) continue;

		TERRAIN_DETAIL* td = &trndtl[i];
		//FLOAT64VECTOR pos = {(td->scx+.5)*SUPERCELL_SIZE_M,(td->scy+.5)*SUPERCELL_SIZE_M,0};

		for(int j=0; j < MAX_TERRAIN_TEXTURES; j++)
		{
			OBJECT* lpobj=td->nonreplacementtiles[j];

			if(!lpobj) continue;
			if(lpobj->texid < 0 || lpobj->texid == trncdReplacementTileTextureID()) continue;

			if(!lpobj->collider)
			{
				physicsCreateObjectCollider(lpobj, colliderTerrainTileSpace(),COLLIDER_TRN_GROUND|COLLIDER_TILE,0);
			}
		}
	}

#ifdef TO_COLLIDE_LIST_DEBUG
	uiChatPrintLine("Collide List Update %d: time:%.2f dist:%.2f total:%d med:%d short:%d tiles:%d odet:%d odei:%d odeo:%d odeb: %d",
		frametime,
		FLOAT32(timeDiff/1000.0f),
		distanceDiff,
		totalTOs,
		mediumCount,
		shortCount,
		rtCount,
		odeTotal,
		odeAdds,
		odeRemoves,
		odeBlanks);
#endif

	return 0;
}

// Play radio static sound if necessary
SINT32 _radioStatic(trnCDTerrainObject* t)
{
    OBJECT* o = t->TerrainObject;

    while(o->flags & OBJECT_NOTOBJECT)
    {
        o = o->child;

        if(!o) return 0;
    }

    if(o->flags & OBJECT_SKIPDRAWING)
    {
        if(o->soundID > 0)
            fxSndStopSound(o->soundID);

        return 0;
    }

    // Already playing
    if(o->soundID > 0)
        return 0;

    vehVehicle*veh = persGetPersonaVehiclePtr();

    // Create static sound for these types
    if(veh && veh->VehicleType)
    {
        switch(veh->VehicleType->Type)
        {
        case VEH_TYPE_FLOATER:
        case VEH_TYPE_PERSON:
        case VEH_TYPE_GROUND:
            {
                SINT32 ID = fxSndGetNewSound();

                if(ID == -1) return 1;

                FLOAT64VECTOR v64;

                v64.x = t->Center.x + WorldOriginOffset.x;
                v64.y = t->Center.y + WorldOriginOffset.y;
                v64.z = t->Center.z;

                fxSndSetSoundName(ID, (SCHAR*)"sounds\\radioxmit.wav");
                fxSndSetSoundType(ID, TYPE_INFINITE_SOUND);
                fxSndSetPitch(ID, 1.0f);
                fxSndSetVolume(ID, 1.0f);
                fxSndSetCreateTime(ID, frametime);
                fxSndSetPlayType(ID, DEF_PLAYTYPE_WORLD);
                fxSndSetPriority(ID, IDE_FP_PRIORITY_MEDIUM);
                fxSndSetPosition(ID, v64.x, v64.y, -v64.z);
                fxSndSetMinDistance(ID, 2.0f);
                fxSndSetMaxDistance(ID, 10.0f);
                fxSndSetKillCallback(ID, _SoundStopped);
                fxSndSetKillData(ID, &o->soundID);

                if(!fxSndInitSound(ID))
                    return 1;

                o->soundID = ID;
            }
        }
    }

    return 0;
}

UINT32 mlCheckRemovedObjectOnCollisionList(trnCDTerrainObject * TO)
{
	if (TO == nullptr)
	{
		return 0;
	}

	mlVehicleData * VehicleData = persGetPersonaModelDataPtr();
	if (VehicleData == nullptr) return 0;

	if(TO->uFlags & TRN_OBJECT_ONMEDIUMLIST)
		_RefreshList = TRUE;

	return 0;
}

UINT32 mlCheckForCollisionListUpdate(bool force)
{
	mlVehicleData * VehicleData = persGetPersonaModelDataPtr();
	if (VehicleData == nullptr)
	{
		return 0;
	}

	if (_RefreshList != FALSE || force)
	{
		_UpdateTerrainCollideLists(VehicleData,1);
	}

	_RefreshList = FALSE;

	return 0;
}

static UINT32 _DoLineCollision(mlVehicleData* VehicleData,
							   mlObjectData * ObjectData,
							   UINT32 PassNum,
							   FLOAT64VECTOR * SavedVehicleForce,
							   FLOAT64VECTOR * TempNormForceSum,
							   FLOAT64 DeltaTime)
{
FLOAT64VECTOR Pos;
FLOAT64VECTOR WorldOffsetOrigin;
OBJECT * LineCollideObject;
UINT32 i;

	if (VehicleData->CollisionsOff == 1)
	{
		return 0;
	}

	if (ObjectData == nullptr)
	{
		return 0;
	}

	LineCollideObject = ObjectData->LineCollideObject;

	if (LineCollideObject == nullptr)
	{
		return 0;
	}

	Pos = VehicleData->AdjustedForCGPos;

	trnGetWorldOriginOffset(&WorldOffsetOrigin);
	WorldOffsetOrigin.z = -WorldOffsetOrigin.z;
	mathSubVectors(&Pos,&Pos,&WorldOffsetOrigin);

	// ensure collide object is up to date
	mlMoveAndRotateObject(LineCollideObject,
						  &Pos,
						  &VehicleData->VehicleRealAtt);


	for (i=0;i<ObjectData->NumCollideObjects;i++)
	{
		_CheckSpecificObjectForLineCollision(VehicleData,
												&ObjectData->CollidePntList[i],
												SavedVehicleForce,
												TempNormForceSum,
												PassNum,
												DeltaTime);

	}

	return 0;
}

static UINT32 _CheckSpecificObjectForLineCollision(mlVehicleData * VehicleData,
												   mlObjectCollidePnt * CollidePnt,
												   FLOAT64VECTOR* VehicleForce,
												   FLOAT64VECTOR* NormalForceSum,
												   UINT32 PassNum,
												   FLOAT64 DeltaTime)
{
FLOAT64VECTOR Force;
FLOAT64VECTOR ForceArm;
OBJECT * Object;
OBJECT * MasterObject;

//return 0;
	Force.x = Force.y = Force.z = 0;
	ForceArm.x = ForceArm.y = ForceArm.z = 0;

	if (CollidePnt == nullptr)
	{
		return 0;
	}

	Object = CollidePnt->CollideObject;
	MasterObject = CollidePnt->CollideObject;

	mlCheckSpecificObjectForLineCollision1(VehicleData,
										  Object,
										  MasterObject,
										  &Force,
										  &ForceArm,
										  VehicleForce,
										  NormalForceSum,
										  &(CollidePnt->RatioForce),
										  &(CollidePnt->LastContactPnt),
										  &(CollidePnt->Force),
										  &(CollidePnt->SlideVel),
										  &(CollidePnt->FrictionCo),
										  &(CollidePnt->BumpDensity),
										  &(CollidePnt->BumpHeight),
										  &(CollidePnt->ShockDisplacement),
										  &(CollidePnt->ShockVel),
										  &(CollidePnt->ForceDir),
										  &(CollidePnt->Length),
										  PassNum,
										  DeltaTime,
										  0,
										   &(CollidePnt->RollFrictionCo),
										   CollidePnt->IsSpike);

	if (PassNum == 0)
	{
		// save off force/sliding vel for component damage purposes
		CollidePnt->Force.x = (FLOAT32)Force.x;
		CollidePnt->Force.y = (FLOAT32)Force.y;
		CollidePnt->Force.z = (FLOAT32)Force.z;

		mathSubVectors(&ForceArm, &ForceArm, &VehicleData->VehicleCenterOfMass);

		CollidePnt->ForceArm.x = (FLOAT32)ForceArm.x;
		CollidePnt->ForceArm.y = (FLOAT32)ForceArm.y;
		CollidePnt->ForceArm.z = (FLOAT32)ForceArm.z;

		mathAddVectors(NormalForceSum,NormalForceSum,&Force);
	}
	else
	{
		// save off force/sliding vel for component damage purposes

		CollidePnt->Force.x = (FLOAT32)Force.x;
		CollidePnt->Force.y = (FLOAT32)Force.y;
		CollidePnt->Force.z = (FLOAT32)Force.z;

		ForceArm.x = CollidePnt->ForceArm.x;
		ForceArm.y = CollidePnt->ForceArm.y;
		ForceArm.z = CollidePnt->ForceArm.z;

		if(	VehicleData->VehicleMasterPtr->VehicleType->Type == VEH_TYPE_SEA)
		{
			mlApplyStaticForceAtRelativePointWithMat(VehicleData,&ForceArm,&Force);
		}else if( (VehicleData->VehicleMasterPtr->VehicleType->Type == VEH_TYPE_AIR && mathExactVectorLength2(&VehicleData->VehicleVel) > 100)
				|| !(CollidePnt->RollFrictionCo > 198.999f &&  CollidePnt->RollFrictionCo < 199.001f)) // not on water
		{
			mlApplyStaticForceAtRelativePointWithMat(VehicleData,&ForceArm,&Force);
		}
	}

	return 0;
}

UINT32 mlCheckSpecificObjectForLineCollision1(mlVehicleData * VehicleData,
												   OBJECT * Object,
												   OBJECT * MasterObject,
												   FLOAT64VECTOR* ReturnForce,
												   FLOAT64VECTOR* ReturnForceArm,
												   FLOAT64VECTOR* VehicleForce,
												   FLOAT64VECTOR* NormalForceSum,
												   FLOAT64VECTOR* ReturnRatioForce,
												   FLOAT64VECTOR* LastContactPnt,
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
													LOGICAL IsSpike)
{
FASTCOLLIDE CollisionList[32];
FASTCOLLIDE * CollideListPtr = nullptr;
UINT32 TempNum = 0;
UINT32 NumCollides = 0;
UINT32 CollideNum = 0;
OBJECT * ColObj = nullptr;
FLOAT32 Length = 0.0f;
piVECTOR ForceDir = {0, 0, 0};
FLOAT64VECTOR Force = {0, 0, 0};
FLOAT64VECTOR ForcePnt = {0, 0, 0};
FLOAT64 ForceMagnitude = 0.0f;
piMATRIX Mat=IdentityMatrix;
FLOAT32 L1 = 0.0f;
FLOAT64VECTOR TempVec1 = {0, 0, 0},TempVec2 = {0, 0, 0};
piVECTOR TempPiVec = {0, 0, 0};
FLOAT64VECTOR ComponentVel = {0, 0, 0};
FLOAT64 Length1 = 0.0f;
FLOAT64VECTOR ForceUnit = {0, 0, 0};
FLOAT64 Vel = 0.0f;
FLOAT64 Friction = 0.0f;
UINT32 RetVal = 0;
FLOAT64VECTOR StaticFriction = {0, 0, 0};
FLOAT64 Percent = 0.0f;
FLOAT64VECTOR Ratio = {0, 0, 0};
FLOAT64 RatioLength = 0.0f;

FLOAT64 Variance = 0.0f;

FLOAT64VECTOR ContactPoint = {0, 0, 0};
piMATRIX ContactPointAttMat=IdentityMatrix,InvContactPointAttMat=IdentityMatrix;
piMATRIX Matrix1=IdentityMatrix,Matrix2=IdentityMatrix;
piVECTOR OldPos = {0, 0, 0},NewPos = {0, 0, 0};
FLOAT32 DOFLength = 0.0f;
FLOAT64VECTOR Pos = {0, 0, 0};
FLOAT64VECTOR WorldOffsetOrigin = {0, 0, 0};

FLOAT64 TempCompress = 0.0f,TempCompress1 = 0.0f;
FLOAT64 TempVel = 0.0f;
FLOAT64 TempVel1 = 0.0f;
FLOAT64 ForceSum = 0.0f;
FLOAT64 VelRotInertia = 0.0f;
FLOAT64 ConstantForce = 0.0f;
FLOAT64 SpringCo = 0.0f,ShockCo = 0.0f;
FLOAT64 ContactDeltaLength = 0.0f;
FLOAT64VECTOR TempContact = {0, 0, 0};
vehVehicle* Vehicle = nullptr;
LOGICAL IsSeaVehicle = 0;
UINT32 i = 0;


	if (PassNum == 0)
	{
		*InLength = 0.0;

		if (Object == nullptr)
			return 0;

		if (!(Object->flags & OBJECT_DOF))
			return 0;

		if (!(Object->dof->dofvarnum == ML_DOF_POINTCOLLIDE ||
			  Object->dof->dofvarnum == ML_DOF_WHEEL_CONTACTPOINT))
			return 0;

		DOFLength = Object->dof->dofneutralval; // neutral comment val is the line length
		if (DOFLength == 0.0)
			DOFLength = 1.0;

		// determine mat and inv mat for DOF contact point
		math3dMatrixMultiply(&ContactPointAttMat, &(Object->dof->dofmatrix2), &(Object->matint));
		ContactPointAttMat._41 = ContactPointAttMat._42 = ContactPointAttMat._43 = 0;
		mlUtilityConvertMatrixToModelReference(&ContactPointAttMat,&ContactPointAttMat);
		math3dMatrixMultiply(&ContactPointAttMat,&ContactPointAttMat,&VehicleData->VehicleAttMat);
		math3dInvert2(&InvContactPointAttMat,&ContactPointAttMat);

		// determine position of DOF contact point
		Matrix1 = IdentityMatrix;
		Matrix1._41 = Object->dof->dofmatrix2._41;
		Matrix1._42 = Object->dof->dofmatrix2._42;
		Matrix1._43 = Object->dof->dofmatrix2._43;
		Matrix2 = Object->matint;
		math3dMatrixMultiply(&Matrix1,&Matrix1,&Matrix2);


		mlUtilityGetModelPositionFromObjectMatrix(&ContactPoint,&Matrix1);

		ForcePnt = ContactPoint;

		mathVectorMultiply64(&ContactPoint, &ContactPoint, &VehicleData->VehicleAttMat);

		Pos = VehicleData->AdjustedForCGPos;

		trnGetWorldOriginOffset(&WorldOffsetOrigin);
		WorldOffsetOrigin.z = -WorldOffsetOrigin.z;

		mathAddVectors(&ContactPoint, &ContactPoint, &Pos);

		mathSubVectors(&TempContact,&ContactPoint,LastContactPnt);
		ContactDeltaLength = mathExactVectorLength(&TempContact);
		ContactDeltaLength *= DeltaTime;
		*LastContactPnt = ContactPoint;
		mathSubVectors(&ContactPoint,&ContactPoint,&WorldOffsetOrigin);

		// determine bumpiness variance
		mlgearDetermineHeightVariance(&Variance,&ContactPoint,*BumpDensity,*BumpHeight);
		DOFLength += (FLOAT32)Variance;

		NewPos.x = (FLOAT32)ContactPoint.x;
		NewPos.y = (FLOAT32)ContactPoint.y;
		NewPos.z = (FLOAT32)ContactPoint.z;
		TempPiVec.x = 0;
		TempPiVec.y = 0;
		TempPiVec.z = -(FLOAT32)Variance;
		math3dVectorMultiply(&TempPiVec,&TempPiVec,&ContactPointAttMat);
		mathAddVectors(&NewPos,&NewPos,&TempPiVec);

		OldPos.x = 0;
		OldPos.y = 0;
		OldPos.z = DOFLength;
		math3dVectorMultiply(&OldPos,&OldPos,&ContactPointAttMat);
		mathAddVectors(&OldPos,&OldPos,&NewPos);
// This was somehow related to the bizzaro self forward prediction code in _UpdateVehicleObject
	/*	OldPos.x -= (FLOAT32)(VehicleData->VehicleRealVel.x * DeltaTime);
		OldPos.y -= (FLOAT32)(VehicleData->VehicleRealVel.y * DeltaTime);
		OldPos.z -= (FLOAT32)(VehicleData->VehicleRealVel.z * DeltaTime);*/
		mathSubVectors(&TempPiVec,&NewPos,&OldPos);
		RatioLength = fabs(DOFLength / mathExactVectorLength(&TempPiVec));

		OldPos.z = -OldPos.z;
		NewPos.z = -NewPos.z;

#ifdef BETA_RELEASE
if(VehicleObjectAnalysis && DisplayCollideLines && IsFirstTic) // Vehicle line collider
{
	piVECTOR TempVec;
	draw3dLineDebug(&OldPos,&NewPos,colorGREENALPHA);

	mathSubVectors(&TempVec,&OldPos,&NewPos);
	TempVec.x *= 0.1f;
	TempVec.y *= 0.1f;
	TempVec.z *= 0.1f;
	mathAddVectors(&TempVec,&TempVec,&NewPos);
	draw3dLineDebug(&TempVec,&NewPos,colorWHITEALPHA);
}
#endif
		TempNum = 32;
		CollideListPtr = CollisionList;

		trncdCollideWithListOfTerrainObjects1(&CollideListPtr,
											 &TempNum,
											 VehicleData->TrnCDListObject,
											 VehicleData->TrnCDListTOs,
											 ML_NUM_MED_LIST_ELEMENTS,
											 &OldPos,
											 &NewPos,
											 0);
		NumCollides = 32 - TempNum;

		if (NumCollides < 32)
		{

			RetVal = trncdCollideWithPolyList(&OldPos, &NewPos, &CollisionList[NumCollides].CollidePnt,
									&CollisionList[NumCollides].CollideObject, &CollisionList[NumCollides].PolyNum,
									VehicleData->TerrainAGLObjectList, VehicleData->TerrainAGLPolyList, VehicleData->NumTerrainAGL);

			if (RetVal != 0)
			{
				math3dVectorMultiply(&CollisionList[NumCollides].CollidePnt, &CollisionList[NumCollides].CollidePnt, &CollisionList[NumCollides].CollideObject->mat);

				CollisionList[NumCollides].CollideTime = (FLOAT32)((CollisionList[NumCollides].CollidePnt.z - NewPos.z) / (OldPos.z - NewPos.z));
				CollisionList[NumCollides].CollideTime = (FLOAT32)(1.0 - CollisionList[NumCollides].CollideTime);
				CollisionList[NumCollides].Alpha = 1.0;
				NumCollides++;
				TempNum--;
				CollideListPtr = &CollisionList[NumCollides];
			}
		}

		// throw in vehicle collision, air only.
		if (VehicleData->VehicleMasterPtr->VehicleType->Type == VEH_TYPE_AIR)
		{
			if (NumCollides < 32)
			{
				weapCollideRoundWithVehicles(&CollideListPtr,
									 &TempNum,
									 &OldPos,
									 &NewPos,
									 VehicleData->LastVehicleUpdateTime / 1000.0,
									 VehicleData->LastVehicleUpdateTime / 1000.0 + DeltaTime,
									 0,
									 0,
									 &WorldOffsetOrigin,
									 1,
									 VehicleData->VehicleMasterPtr->Side);

			}
			NumCollides = 32 - TempNum;
		}

		if (NumCollides == 0)
		{
			_UpdateShock(ShockVel,ShockPos,0.0,DeltaTime);

			return 0;
		}


		mlUtilitySortCollideList(CollisionList,NumCollides);

		CollideNum = 0;
		Vehicle = VehicleData->VehicleMasterPtr;
		if (Vehicle->VehicleType->Type == VEH_TYPE_SEA)
			IsSeaVehicle = TRUE;
		else
			IsSeaVehicle = FALSE;

		mlGetObjectBumpinessAndFriction(	nullptr,
											nullptr,
											nullptr,
											nullptr,
											RollFrictionCo,
											nullptr,
											CollisionList[CollideNum].CollideObject);

		while (CollideNum < NumCollides && (CollisionList[CollideNum].Alpha < 0.5 ||
				(*RollFrictionCo > 0.99f && *RollFrictionCo < 1.01f && IsSeaVehicle)))
		{
			CollideNum++;
			if (CollideNum < NumCollides && IsSeaVehicle)
			{
				mlGetObjectBumpinessAndFriction(nullptr,
												nullptr,
												nullptr,
												nullptr,
												RollFrictionCo,
												nullptr,
												CollisionList[CollideNum].CollideObject);
			}
		}


		if (CollideNum == NumCollides)
		{
			_UpdateShock(ShockVel,ShockPos,0.0,DeltaTime);
			return 0;
		}

		ColObj = CollisionList[CollideNum].CollideObject;

		if(ColObj == nullptr)
		{
			Write_Log(LOG_VEHICLE, LOG_WARN, "mlCheckSpecificObjectForLineCollision1: Null collide object!");
			return 0;
		}

		// determine bumpiness
		mlGetObjectBumpinessAndFriction(BumpDensity, // dummy var
									   BumpHeight, // dummy var
									   nullptr,
									   DynamicFriction,
									   nullptr,
									   nullptr,
									   ColObj);
		if(IsSpike)
			*DynamicFriction = 1;

		Length = (FLOAT32)(1.0 - CollisionList[CollideNum].CollideTime);
		if (Length < 0.0)
		{
			_UpdateShock(ShockVel,ShockPos,0.0,DeltaTime);
			return 0;
		}

		if (Length < ContactDeltaLength)
		{
			Length = (FLOAT32)(Length * Length / ContactDeltaLength);
		}

		ForceDir.x = ColObj->psnorms[CollisionList[CollideNum].PolyNum].x;
		ForceDir.y = ColObj->psnorms[CollisionList[CollideNum].PolyNum].y;
		ForceDir.z = ColObj->psnorms[CollisionList[CollideNum].PolyNum].z;

		mathSubVectors(&TempPiVec,&OldPos,&NewPos);
		math3dNormalize(&TempPiVec);
		Length = mathVectDotProduct(&ForceDir,&TempPiVec) * Length * DOFLength;
		Length -= (FLOAT32)*ShockPos;

		*InLength = Length;
		TempCompress = Length * RatioLength;
		TempCompress1 = TempCompress;

		// calculate velocity along force vector
		TempVec1 = ForcePnt;
		TempPiVec.x = -(FLOAT32)(VehicleData->VehicleRealAttVel.Pitch * 0.001);
		TempPiVec.y = -(FLOAT32)(VehicleData->VehicleRealAttVel.Roll * 0.001);
		TempPiVec.z = (FLOAT32)(VehicleData->VehicleRealAttVel.Yaw * 0.001);
		mathEul_ToMatrix(&Mat,EulOrdXYZs,&TempPiVec);

		mathVectorMultiply64(&TempVec2,&TempVec1,&Mat);
		ComponentVel.x = (TempVec2.x - TempVec1.x) * (1/0.001);
		ComponentVel.y = (TempVec2.y - TempVec1.y) * (1/0.001);
		ComponentVel.z = (TempVec2.z - TempVec1.z) * (1/0.001);

		// ok now take the component rotational velocity vector and
		// make it relative to the world
		mathVectorMultiply64(&ComponentVel,&ComponentVel,&VehicleData->VehicleAttMat);

		// add to it the vehicle's velocity
		mathAddVectors(&ComponentVel,&ComponentVel,&VehicleData->VehicleRealVel);

//		if (mathExactVectorLength(&ComponentVel) > 30.0)
		if (mathVectDotProduct(&ComponentVel,&ForceDir) > 30.0)
		{
			vehAnimChangeVehicleStateByName(VehicleData->VehicleMasterPtr, (UCHAR*)"crashexplosion", TRUE);
			persKillCrew();
			VehicleData->FreezeVehicle = 1;
			vehVisDamageHulkVehicle(VehicleData->VehicleMasterPtr, TRUE);
		}

		// determine component of velocity relative to force
		ForceUnit.x = ForceDir.x;
		ForceUnit.y = ForceDir.y;
		ForceUnit.z = ForceDir.z;

		Length1 = mathVectDotProduct(&ComponentVel,&ForceUnit);


		Vel = mathVectDotProduct(&VehicleData->VehicleVel,
						&ColObj->psnorms[CollisionList[CollideNum].PolyNum]);

		Vel += *ShockVel;

		InForceDir->x = ForceDir.x;
		InForceDir->y = ForceDir.y;
		InForceDir->z = ForceDir.z;

		TempVel = Vel;

		ForceSum = 0.0;

		TempVec2 = ForceUnit;
		TempVec2.z = -TempVec2.z;
		mathVectorMultiply64(&TempVec2,&TempVec2,&VehicleData->VehicleAttInvMat);
		mathVectDoCrossProduct(&TempVec1,&TempVec2,&ForcePnt);
		Length = (FLOAT32)mathExactVectorLength(&ForcePnt);
		TempVec1.x = TempVec1.x / Length * VehicleData->VehicleRotationalInertia.Pitch;
		TempVec1.y = TempVec1.y / Length * VehicleData->VehicleRotationalInertia.Roll;
		TempVec1.z = TempVec1.z / Length * VehicleData->VehicleRotationalInertia.Yaw;
		VelRotInertia = mathExactVectorLength(&TempVec1);
		VelRotInertia /= Length;

		ConstantForce = mathVectDotProduct(&ForceUnit,ReturnRatioForce);

		if (IsWheel == 0)
		{
			SpringCo = 10.0 * VehicleData->VehicleMass * 9.8; //400.0
		}
		else
		{
			SpringCo = 10.0 * VehicleData->VehicleMass * 9.8;
		}


		for (i=0;i<_NUM_CP_LOOPS;i++)
		{
			if (IsWheel == 0)
			{
				ShockCo = 10.0 * VehicleData->VehicleMass; // 1000.0
			}
			else
			{
				ShockCo = 10.0 * VehicleData->VehicleMass;
			}

			ForceMagnitude = -ShockCo * TempVel;

			TempVel1 = TempVel + ForceMagnitude * DeltaTime / _NUM_CP_LOOPS / VehicleData->VehicleMass;
			TempVel1 += ForceMagnitude * DeltaTime / _NUM_CP_LOOPS / VelRotInertia;
			if (TempVel1 > 0.0 && TempVel < 0.0)
			{
				ForceMagnitude *= -TempVel / (TempVel1 - TempVel);
			}
			if (TempVel1 < 0.0 && TempVel > 0.0)
			{
				ForceMagnitude *= -TempVel / (TempVel1 - TempVel);

			}

			if (ForceMagnitude > VehicleData->VehicleMass * 9.8 * 1.5)
			{
				ForceMagnitude += VehicleData->VehicleMass * 9.8 * 1.5;
			}
			else
			{
				ForceMagnitude += SpringCo * TempCompress;
			}

			ForceSum += ForceMagnitude;

			// do gross movement
			TempVel += (ForceMagnitude) * DeltaTime / _NUM_CP_LOOPS / VehicleData->VehicleMass;
			// now do rotation factor
			TempVel += ForceMagnitude * DeltaTime / _NUM_CP_LOOPS / VelRotInertia;

			TempCompress -= TempVel * DeltaTime / _NUM_CP_LOOPS;

			if (TempCompress < 0)
			{
				break;
			}

			ForceMagnitude = SpringCo * TempCompress;
			ForceMagnitude += -ShockCo * TempVel;
		}

		ForceMagnitude = ForceSum / _NUM_CP_LOOPS;

		Force.x = ForceMagnitude * ForceUnit.x;
		Force.y = ForceMagnitude * ForceUnit.y;
		Force.z = -ForceMagnitude * ForceUnit.z;

		// now adjust shock effects
		ForceMagnitude = mathVectDotProduct(&Force,&ForceUnit);

		_UpdateShock(ShockVel,ShockPos,ForceMagnitude,DeltaTime);

		PntVel->x = (FLOAT32)ComponentVel.x;
		PntVel->y = (FLOAT32)ComponentVel.y;
		PntVel->z = (FLOAT32)ComponentVel.z;
		*ReturnForceArm = ForcePnt;
		*ReturnForce = Force;

		if(!IsSeaVehicle && *RollFrictionCo > 198.999f && *RollFrictionCo < 199.001f)
		{
			if(Vehicle->VehicleType->Type != VEH_TYPE_AIR)
			{
				ReturnForce->x = ReturnForce->y = ReturnForce->z = 0;
				ReturnForceArm->x = ReturnForceArm->y = ReturnForceArm->z = 0;
			}
			return 0;
		}
	}
	else
	{
		Force.x = PntForce->x;
		Force.y = PntForce->y;
		Force.z = PntForce->z;

		if (NormalForceSum->x != 0.0)
		{
			Ratio.x = Force.x / NormalForceSum->x;
		}
		else
		{
			Ratio.x = 0;
		}
		if (NormalForceSum->y != 0.0)
		{
			Ratio.y = Force.y / NormalForceSum->y;
		}
		else
		{
			Ratio.y = 0;
		}
		if (NormalForceSum->z != 0.0)
		{
			Ratio.z = Force.z / NormalForceSum->z;
		}
		else
		{
			Ratio.z = 0;
		}

		mathSubVectors(&TempVec1,VehicleForce,NormalForceSum);

		ForceUnit.x = TempVec1.x * Ratio.x;
		ForceUnit.y = TempVec1.y * Ratio.y;
		ForceUnit.z = TempVec1.z * Ratio.z;

		*ReturnRatioForce = ForceUnit;

		TempVec1 = ForceUnit;

		mathVectDoCrossProduct(&TempVec2,&Force,&ForceUnit);

		mathVectDoCrossProduct(&ForceUnit,&Force,&TempVec2);
		L1 = (FLOAT32)mathExactVectorLength(&ForceUnit);

		if (L1 > 0.00001)
		{
			ForceUnit.x /= L1;
			ForceUnit.y /= L1;
			ForceUnit.z /= L1;
		}
		else
		{
			ForceUnit.x = ForceUnit.y = ForceUnit.z = 0;
		}

		L1 = (FLOAT32)mathVectDotProduct(&TempVec1,&ForceUnit);

		Length = (FLOAT32)mathExactVectorLength(&Force);
		if (L1 > 0.7 * Length)
		{
			L1 = (FLOAT32)0.7 * Length;
		}
		if (L1 < -0.7 * Length)
		{
			L1 = (FLOAT32)(-0.7 * Length);
		}

		StaticFriction.x = -ForceUnit.x * L1;
		StaticFriction.y = -ForceUnit.y * L1;
		StaticFriction.z = -ForceUnit.z * L1;

		// now figure out friction
		// first determine unit vector in direction of velocity perpendicular to current force vector
		// if the vel is large enough, only kinetic friction applies, otherwise
		// interpolate with static friction
		ComponentVel.x = PntVel->x;
		ComponentVel.y = PntVel->y;
		ComponentVel.z = PntVel->z;

		mathVectDoCrossProduct(&TempVec2,&Force,&ComponentVel);
		mathVectDoCrossProduct(&ForceUnit,&Force,&TempVec2);
		Length1 = mathExactVectorLength(&ForceUnit);
		if (Length1 > 0.00001)
		{
			ForceUnit.x /= Length1;
			ForceUnit.y /= Length1;
			ForceUnit.z /= Length1;

			// now determine velocity component in this direction
			Vel = mathVectDotProduct(&ForceUnit,&ComponentVel);
			ForceMagnitude = mathExactVectorLength(&Force);
			if (Vel > 0.0)
			{
				Friction = -*DynamicFriction * ForceMagnitude;
				if (Vel > 0.2)
				{
					Percent = 1.0;
				}
				else
				{
					Percent = Vel / 0.2;
				}
			}
			else
			{
				Friction = *DynamicFriction * ForceMagnitude;
				if (Vel < -0.2)
				{
					Percent = 1.0;
				}
				else
				{
					Percent = -Vel / 0.2;
				}
			}

			// apply friction
			Force.x += (ForceUnit.x * Friction * Percent) + (StaticFriction.x * (1.0 - Percent));
			Force.y += (ForceUnit.y * Friction * Percent) + (StaticFriction.y * (1.0 - Percent));
			Force.z += (ForceUnit.z * Friction * Percent) + (StaticFriction.z * (1.0 - Percent));

			PntVel->x = (FLOAT32)(Vel * ForceUnit.x);
			PntVel->y = (FLOAT32)(Vel * ForceUnit.y);
			PntVel->z = (FLOAT32)(Vel * ForceUnit.z);
		}

		*ReturnForce = Force;
	}

	return 0;
}

#define SHOCK_FORCE 100000
#define SHOCK_RETURN_FORCE 50000
#define SPRING_FORCE 1000000
#define SHOCK_INERTIA 500

static UINT32 _UpdateShock(FLOAT64 * ShockVel,FLOAT64 * ShockPos,FLOAT64 InForceMagnitude,FLOAT64 DeltaTime)
{
	FLOAT64 ForceMagnitude;

	ForceMagnitude = InForceMagnitude;

	ForceMagnitude -= *ShockPos * SPRING_FORCE;
	if (*ShockVel > 0)
	{
		ForceMagnitude -= SHOCK_FORCE * *ShockVel;
	}
	else
	{
		ForceMagnitude -= SHOCK_RETURN_FORCE * *ShockVel;
	}

	*ShockVel = ((*ShockVel + ForceMagnitude * DeltaTime / 1000.0 / SHOCK_INERTIA) + *ShockVel) / 2.0;
	*ShockPos += *ShockVel * DeltaTime / 1000.0;

	*ShockVel = 0.0;
	*ShockPos = 0.0;

	return 0;
}

// routine to create a list of objects from the short terrain list to check
// for line colliders
static UINT32 _UpdateBBoxList(mlVehicleData * VehicleData)
{
FLOAT64VECTOR DeltaVec;
FLOAT64 TempNum;
UINT32 ForceRecalc;
static SINT32 LastUpdateTime;

	// check to see if we've moved too far between frames
	ForceRecalc = 0;

	if (VehicleData != nullptr && frametime != LastUpdateTime)
	{
		mathSubVectors(&DeltaVec,&VehicleData->VehiclePos,&VehicleData->VehicleLastPos);
		DeltaVec.z = 0.0;

		TempNum = mathExactVectorLength(&DeltaVec);

		if (TempNum / ((frametime - LastUpdateTime) / 1000.0) > 400.0)
		{
			ForceRecalc = 1;
		}

		LastUpdateTime = frametime;
	}

	_UpdateTerrainCollideLists(VehicleData,ForceRecalc);

	// now determine list of objects that intersect bounding box of vehicle
	_DetermineVehicleBoundingBoxIntersect(VehicleData,
										  VehicleData->VehicleObjectData,
										  VehicleData->ShortTrnList,
										  VehicleData->AdjustedForCGPos,
										  &VehicleData->VehicleAttMat);
	return 0;
}

static UINT32 _DetermineVehicleBoundingBoxIntersect(mlVehicleData * VehicleData,
													mlObjectData * VehicleObjectData,
													trnCDTerrainObject ** TOList,
													FLOAT64VECTOR AdjustedForCGPos,
													piMATRIX * AttMat)
{
piVECTOR BBox[8];
piVECTOR bbox[2];
UINT32 i;
FLOAT64VECTOR WorldOffsetOrigin;
trnCDTerrainObject * TO;
UINT32 Count;
UINT32 TrnCDPos;
trnCDTerrainObject** TOPtr;
OBJECT ** ObjPtr;
FLOAT32 mnx,mny,mnz, mxx,mxy,mxz;
UINT32 First;
UINT32 TempNum;


	if (VehicleObjectData != nullptr)
	{
		bbox[0] = VehicleObjectData->BBox1;
		bbox[1] = VehicleObjectData->BBox2;
	}
	else
	{
		bbox[0].x = bbox[0].y = bbox[0].z = bbox[1].x = bbox[1].y = bbox[1].z = 0.0f;
	}

	bbox[0].x -= 2.0f;
	bbox[0].y -= 2.0f;
	bbox[0].z -= 2.0f;
	bbox[1].x += 2.0f;
	bbox[1].y += 2.0f;
	bbox[1].z += 2.0f;

	BBox[0].x = bbox[0].x;
	BBox[0].y = bbox[0].y;
	BBox[0].z = bbox[0].z;
	BBox[1].x = bbox[0].x;
	BBox[1].y = bbox[1].y;
	BBox[1].z = bbox[0].z;
	BBox[2].x = bbox[1].x;
	BBox[2].y = bbox[1].y;
	BBox[2].z = bbox[0].z;
	BBox[3].x = bbox[1].x;
	BBox[3].y = bbox[0].y;
	BBox[3].z = bbox[0].z;
	BBox[4].x = bbox[0].x;
	BBox[4].y = bbox[0].y;
	BBox[4].z = bbox[1].z;
	BBox[5].x = bbox[0].x;
	BBox[5].y = bbox[1].y;
	BBox[5].z = bbox[1].z;
	BBox[6].x = bbox[1].x;
	BBox[6].y = bbox[1].y;
	BBox[6].z = bbox[1].z;
	BBox[7].x = bbox[1].x;
	BBox[7].y = bbox[0].y;
	BBox[7].z = bbox[1].z;

	trnGetWorldOriginOffset(&WorldOffsetOrigin);
	WorldOffsetOrigin.z = -WorldOffsetOrigin.z;

	First = 1;
	for (i=0;i<8;i++)
	{
		math3dVectorMultiply(&BBox[i],&BBox[i],AttMat);
		BBox[i].x += (FLOAT32)(AdjustedForCGPos.x - WorldOffsetOrigin.x);
		BBox[i].y += (FLOAT32)(AdjustedForCGPos.y - WorldOffsetOrigin.y);
		BBox[i].z -= (FLOAT32)(AdjustedForCGPos.z - WorldOffsetOrigin.z);

		if (First == 1)
		{
			bbox[0] = bbox[1] = BBox[i];
			First = 0;
		}
		else
		{
			if (BBox[i].x < bbox[0].x)
			{
				bbox[0].x = BBox[i].x;
			}
			if (BBox[i].y < bbox[0].y)
			{
				bbox[0].y = BBox[i].y;
			}
			if (BBox[i].z < bbox[0].z)
			{
				bbox[0].z = BBox[i].z;
			}

			if (BBox[i].x > bbox[1].x)
			{
				bbox[1].x = BBox[i].x;
			}
			if (BBox[i].y > bbox[1].y)
			{
				bbox[1].y = BBox[i].y;
			}
			if (BBox[i].z > bbox[1].z)
			{
				bbox[1].z = BBox[i].z;
			}
		}
	}

	VehicleData->NumTerrainCollides = 0;

	// now get the list of terrain polys below vehicle
	bbox[0].z = -10000.0;
	bbox[1].z = 20000.0;

	TempNum = 8;

	trncdGetListOfTerrainPolysNearBoundingBox(bbox,
											  VehicleData->TerrainAGLObjectList,
											  VehicleData->TerrainAGLPolyList,
											  &TempNum);
	VehicleData->NumTerrainAGL = 8 - TempNum;

	memset(VehicleData->TrnCDListObject,0,sizeof(OBJECT*) * (ML_NUM_MED_LIST_ELEMENTS+1));
	memset(VehicleData->TrnCDListTOs,0,sizeof(trnCDTerrainObject*) * (ML_NUM_MED_LIST_ELEMENTS+1));

	Count = 0;
	TrnCDPos = 0;
	TOPtr = &VehicleData->TrnCDListTOs[0];
	ObjPtr = &VehicleData->TrnCDListObject[0];

	while (Count < ML_NUM_SHORT_LIST_ELEMENTS && TrnCDPos < ML_NUM_MED_LIST_ELEMENTS)
	{
		TO = TOList[Count];

		if (TO == nullptr)
		{
			break;
		}

		if (TO->TerrainObject == nullptr || TO->DamageLevel == FB_CLOSED_VISLEVEL)
		{
            Count++;
			continue;
		}

		if(TO->IsSTO())
		{
			TerrainSTO* sto = static_cast<TerrainSTO*>(TO->TerrainMasterObject->lpspare);

			if(sto && !sto->isCollidable())
			{
				Count++;
				continue;
			}
		}

		// check general bounding box
		mnx = (FLOAT32)(TO->BBox1.x + TO->Center.x - TO->OrigCenter.x);
		mny = (FLOAT32)(TO->BBox1.y + TO->Center.y - TO->OrigCenter.y);
		mnz = (FLOAT32)(TO->BBox1.z + TO->Center.z - TO->OrigCenter.z);

		mxx = (FLOAT32)(TO->BBox2.x + TO->Center.x - TO->OrigCenter.x);
		mxy = (FLOAT32)(TO->BBox2.y + TO->Center.y - TO->OrigCenter.y);
		mxz = (FLOAT32)(TO->BBox2.z + TO->Center.z - TO->OrigCenter.z);

		if ((bbox[0].x > mnx &&
			 bbox[0].x < mxx) ||
			(bbox[1].x > mnx &&
			 bbox[1].x < mxx) ||
			(mnx > bbox[0].x &&
			 mnx < bbox[1].x) ||
			(mxx > bbox[0].x &&
			 mxx < bbox[1].x))
		{
			if ((bbox[0].y > mny &&
				 bbox[0].y < mxy) ||
				(bbox[1].y > mny &&
				 bbox[1].y < mxy) ||
				(mny > bbox[0].y &&
				 mny < bbox[1].y) ||
				(mxy > bbox[0].y &&
				 mxy < bbox[1].y))
			{
				if ((bbox[0].z > mnz &&
					 bbox[0].z < mxz) ||
					(bbox[1].z > mnz &&
					 bbox[1].z < mxz) ||
					(mnz > bbox[0].z &&
					 mnz < bbox[1].z) ||
					(mxz > bbox[0].z &&
					 mxz < bbox[1].z))
				{
					if (TO->CollisionObject->child != nullptr &&
						TO->DamageLevel != FB_CLOSED_VISLEVEL)
					{
						if (TO->DamageLevel == 0)
						{

							_DetermineIfBoundingBoxIntersects(&TrnCDPos,
															  ML_NUM_MED_LIST_ELEMENTS,
															  &TOPtr,
															  &ObjPtr,
															  TO,
															  //TO->TerrainObject->child,
															  TO->CollisionObject->child,
															  &BBox[0]);
						}
						else
						{
							_DetermineIfBoundingBoxIntersects(&TrnCDPos,
															  ML_NUM_MED_LIST_ELEMENTS,
															  &TOPtr,
															  &ObjPtr,
															  TO,
															  //TO->TerrainObject->child,
															  TO->DamageCollisionObject->child,
															  &BBox[0]);
						}
					}
				}
			}
		}

		Count++;
	}
	return 0;
}

static UINT32 _DetermineIfBoundingBoxIntersects(UINT32 * TrnCDPos,
												 UINT32 MaxTrnCDPos,
												 trnCDTerrainObject *** TOPtr,
												 OBJECT *** ObjPtr,
												 trnCDTerrainObject * TO,
												 OBJECT * Obj,
												 piVECTOR * BBoxIn)
{
OBJECT * ObjSib,*ObjAtt;
UINT32 i;
piVECTOR bbox[2],bbox1[2];
piVECTOR BBox;
UINT32 First;
piMATRIX MatInv;
UINT32 Intersects;

	if (Obj == nullptr)
	{
		return 0;
	}

	if (*TrnCDPos >= MaxTrnCDPos)
	{
		return 0;
	}

	ObjSib = Obj;

	do
	{
		if (!(ObjSib->flags & OBJECT_NOTOBJECT) &&
			 (ObjSib->flags & OBJECT_VISIBLE))
		{
			ObjAtt = ObjSib;

			do
			{
				math3dInvertMatrix(&MatInv,&ObjAtt->mat);
				First = 1;

				for (i=0;i<8;i++)
				{
					math3dVectorMultiply(&BBox,&BBoxIn[i],&MatInv);

					if (First == 1)
					{
						bbox[0] = bbox[1] = BBox;

						First = 0;
					}
					else
					{
						if (BBox.x < bbox[0].x)
						{
							bbox[0].x = BBox.x;
						}
						if (BBox.y < bbox[0].y)
						{
							bbox[0].y = BBox.y;
						}
						if (BBox.z < bbox[0].z)
						{
							bbox[0].z = BBox.z;
						}

						if (BBox.x > bbox[1].x)
						{
							bbox[1].x = BBox.x;
						}
						if (BBox.y > bbox[1].y)
						{
							bbox[1].y = BBox.y;
						}
						if (BBox.z > bbox[1].z)
						{
							bbox[1].z = BBox.z;
						}
					}
				}

				Intersects = 0;
				// the actual check

				bbox1[0] = ObjAtt->bbox[0];
				mathAddVectors(&bbox1[0],&bbox1[0],&ObjAtt->pos);
				bbox1[1] = ObjAtt->bbox[1];
				mathAddVectors(&bbox1[1],&bbox1[1],&ObjAtt->pos);
				if ((bbox[0].x > bbox1[0].x &&
					 bbox[0].x < bbox1[1].x) ||
					(bbox[1].x > bbox1[0].x &&
					 bbox[1].x < bbox1[1].x) ||
					(bbox1[0].x > bbox[0].x &&
					 bbox1[0].x < bbox[1].x) ||
					(bbox1[1].x > bbox[0].x &&
					 bbox1[1].x < bbox[1].x))
				{
					if ((bbox[0].y > bbox1[0].y &&
						 bbox[0].y < bbox1[1].y) ||
						(bbox[1].y > bbox1[0].y &&
						 bbox[1].y < bbox1[1].y) ||
						(bbox1[0].y > bbox[0].y &&
						 bbox1[0].y < bbox[1].y) ||
						(bbox1[1].y > bbox[0].y &&
						 bbox1[1].y < bbox[1].y))
					{
						if ((bbox[0].z > bbox1[0].z &&
							 bbox[0].z < bbox1[1].z) ||
							(bbox[1].z > bbox1[0].z &&
							 bbox[1].z < bbox1[1].z) ||
							(bbox1[0].z > bbox[0].z &&
							 bbox1[0].z < bbox[1].z) ||
							(bbox1[1].z > bbox[0].z &&
							 bbox1[1].z < bbox[1].z))
						{
							Intersects = 1;
						}
					}
				}

				if (Intersects == 1)
				{
					**TOPtr = TO;
					**ObjPtr = ObjAtt;

					(*TrnCDPos)++;
					(*TOPtr)++;
					(*ObjPtr)++;
				}

				ObjAtt = ObjAtt->attached;
			}
			while (ObjAtt != nullptr && *TrnCDPos < MaxTrnCDPos);

		}

		if (ObjSib->child != nullptr)
		{
			_DetermineIfBoundingBoxIntersects(TrnCDPos,
											  MaxTrnCDPos,
											  TOPtr,
											  ObjPtr,
											  TO,
											  ObjSib->child,
											  BBoxIn);
		}

		ObjSib = ObjSib->sibling;
	}
	while (ObjSib != nullptr && *TrnCDPos < MaxTrnCDPos);

	return 0;
}

UINT32 _ReadBumpAndFrictionTable()
{
	static UCHAR Buffer[512];
	SINT32 FileNum;
	UCHAR* BufferPtr;
	SINT32 Position;

	FileNum = fioOpen("vehicles\\ground.csv");
	if(FileNum < 0)
	{
		FileNum = fioOpen("vehicles\\ground.txt");
		if (FileNum < 0)
		{
			Write_Debug("Can't find ground modelling data file vehicles\\ground.txt");
			return 1;
		}
	}

    std::string szStr;

	Position = -1;
	while (fioGetString(FileNum,(char*)Buffer,511) != 0)
	{
		if (Buffer[0] == '#')
		{
			continue;
		}

		if (strlen((SCHAR*)Buffer) < 3)
		{
			continue;
		}

		Position++;

		BufferPtr = (UCHAR*)strtok((char*)Buffer,",");
		if (BufferPtr == nullptr)
		{
			continue;
		}
		strcpy((char*)_BAFData[Position].Name,(char*)BufferPtr);

        //--
        _hashBAF[(const char*)_BAFData[Position].Name] = &_BAFData[Position];
        //--

		BufferPtr = (UCHAR*)strtok(nullptr,",");
		if (BufferPtr == nullptr)
		{
			continue;
		}
		_BAFData[Position].BumpDensity = (FLOAT32)atof((char*)BufferPtr);

		BufferPtr = (UCHAR*)strtok(nullptr,",");
		if (BufferPtr == nullptr)
		{
			continue;
		}
		_BAFData[Position].BumpHeight = (FLOAT32)atof((char*)BufferPtr);

		BufferPtr = (UCHAR*)strtok(nullptr,",");
		if (BufferPtr == nullptr)
		{
			continue;
		}
		_BAFData[Position].StaticFriction = (FLOAT32)atof((char*)BufferPtr);

		BufferPtr = (UCHAR*)strtok(nullptr,",");
		if (BufferPtr == nullptr)
		{
			continue;
		}
		_BAFData[Position].DynamicFriction = (FLOAT32)atof((char*)BufferPtr);

		BufferPtr = (UCHAR*)strtok(nullptr,",");
		if (BufferPtr == nullptr)
		{
			_BAFData[Position].RollCoefficientMult = 1.0;
			continue;
		}
		_BAFData[Position].RollCoefficientMult = (FLOAT32)atof((char*)BufferPtr);
//
		BufferPtr = (UCHAR*)strtok(nullptr,",");
		if (BufferPtr == nullptr)
		{
			_BAFData[Position].GroundClutterType[0] = 0;
			continue;
		}
		_BAFData[Position].GroundClutterType[0] = atoi((char*)BufferPtr);

		BufferPtr = (UCHAR*)strtok(nullptr,",");
		if (BufferPtr == nullptr)
		{
			_BAFData[Position].GroundClutterType[1] = 0;
			continue;
		}
		_BAFData[Position].GroundClutterType[1] = atoi((char*)BufferPtr);

		BufferPtr = (UCHAR*)strtok(nullptr,",");
		if (BufferPtr == nullptr)
		{
			_BAFData[Position].GroundClutterType[2] = 0;
			continue;
		}
		_BAFData[Position].GroundClutterType[2] = atoi((char*)BufferPtr);

		BufferPtr = (UCHAR*)strtok(nullptr,",");
		if (BufferPtr == nullptr)
		{
			_BAFData[Position].GroundClutterWidth = 1.0f;
			continue;
		}
		_BAFData[Position].GroundClutterWidth = (FLOAT32)atof((char*)BufferPtr);

		BufferPtr = (UCHAR*)strtok(nullptr,",");
		if (BufferPtr == nullptr)
		{
			_BAFData[Position].GroundClutterHeight = 1.0f;
			continue;
		}
		_BAFData[Position].GroundClutterHeight = (FLOAT32)atof((char*)BufferPtr);
//

		BufferPtr = (UCHAR*)strtok(nullptr,",");
		if (BufferPtr == nullptr)
		{
			continue;
		}
		sscanf((SCHAR*)BufferPtr,"%s,",(SCHAR*)_BAFData[Position].HitSoundEffect[0]);

		BufferPtr = (UCHAR*)strtok(nullptr,",");
		if (BufferPtr == nullptr)
		{
			continue;
		}
		sscanf((SCHAR*)BufferPtr,"%s,",(SCHAR*)_BAFData[Position].HitSoundEffect[1]);

		BufferPtr = (UCHAR*)strtok(nullptr,",");
		if (BufferPtr == nullptr)
		{
			continue;
		}
		sscanf((SCHAR*)BufferPtr,"%s,",(SCHAR*)_BAFData[Position].HitSoundEffect[2]);

		BufferPtr = (UCHAR*)strtok(nullptr,",");
		if (BufferPtr == nullptr)
		{
			_BAFData[Position].MinHitEffectVolume = 1.0;
			continue;
		}
		_BAFData[Position].MinHitEffectVolume = (FLOAT32)atof((char*)BufferPtr);

		BufferPtr = (UCHAR*)strtok(nullptr,",");
		if (BufferPtr == nullptr)
		{
			_BAFData[Position].MaxVolumeEnergy = 1.0;
			continue;
		}
		_BAFData[Position].MaxVolumeEnergy = (FLOAT32)atof((char*)BufferPtr);

		BufferPtr = (UCHAR*)strtok(nullptr,",");
		if (BufferPtr == nullptr)
		{
			_BAFData[Position].HitSpriteNum[0] = 0;
			continue;
		}
		_BAFData[Position].HitSpriteNum[0] = atoi((char*)BufferPtr);

		BufferPtr = (UCHAR*)strtok(nullptr,",");
		if (BufferPtr == nullptr)
		{
			_BAFData[Position].HitSpriteNum[1] = 0;
			continue;
		}
		_BAFData[Position].HitSpriteNum[1] = atoi((char*)BufferPtr);

		BufferPtr = (UCHAR*)strtok(nullptr,",");
		if (BufferPtr == nullptr)
		{
			_BAFData[Position].HitSpriteNum[2] = 0;
			continue;
		}
		_BAFData[Position].HitSpriteNum[2] = atoi((char*)BufferPtr);

		BufferPtr = (UCHAR*)strtok(nullptr,",");
		if (BufferPtr == nullptr)
		{
			_BAFData[Position].InfSurfaceID = 0;
			continue;
		}
		_BAFData[Position].InfSurfaceID = atoi((char*)BufferPtr);

		if (Position >= _MAX_BUMPANDFRICTION_MATERIALS)
		{
			break;
		}

        for(SINT32 q = 0; q < 5; q++)
        {
            BufferPtr = (UCHAR*)strtok(nullptr, ",");

            if(BufferPtr == nullptr)
            {
                _BAFData[Position].RadialClutter[q] = 0;
            }
            else
            {
                _BAFData[Position].RadialClutter[q] = atoi((char*)BufferPtr);
            }
        }

		BufferPtr = (UCHAR*)strtok(nullptr,",");
		if(BufferPtr == nullptr)
		{
			_BAFData[Position].GroundClutterDensity = 1;
			continue;
		}

		_BAFData[Position].GroundClutterDensity = atoi((char*)BufferPtr);
		if(_BAFData[Position].GroundClutterDensity > 4)
		{
			_BAFData[Position].GroundClutterDensity = 4;
			Write_Log(LOG_VEHICLE, LOG_TERRAIN, "Do not set GroundClutterDensity > 4");
		}
		if(_BAFData[Position].GroundClutterDensity < 1)
		{
			_BAFData[Position].GroundClutterDensity = 1;
		}
	}

	_NumBAF = Position;

	fioClose(FileNum);

	return 0;
}

UINT32 mlGetGroundClutterStuff(SINT32* ClutterType1,
							   SINT32* ClutterType2,
							   SINT32* ClutterType3,
							   FLOAT32* ClutterWidth,
							   FLOAT32* ClutterHeight,
							   SINT32* ClutterDensity,
							   const SINT8 * TextureName)
{
	if (ClutterType1 == nullptr ||
		ClutterType2 == nullptr ||
		ClutterType3 == nullptr ||
		ClutterWidth == nullptr ||
		ClutterHeight == nullptr ||
		TextureName == nullptr)
	{
		return 1;
	}

    std::map<std::string, BumpAndFrictionData*>::iterator hashFind;

    SCHAR szCopy[32];
    SCHAR* p = nullptr;

    strncpy(szCopy, TextureName, 31);

    // Remove '.bmp'
    p = strchr(szCopy, '.');

    if(p) *p = 0;

    SCHAR* p2 = nullptr;
    p = szCopy;
    p2 = p;

    // Remove directory path if name includes it.
    while(*p != 0)
    {
        if(*p == '\\') p2 = p + 1;
        if(*p == '/') p2 = p + 1;

        p++;
    }

    hashFind = _hashBAF.find(p2);

    if(hashFind == _hashBAF.end())
    {
        hashFind = _hashBAF.find("default");

        if(hashFind == _hashBAF.end())
        {
            return 1;
        }
    }

    BumpAndFrictionData* lpBAFData = hashFind->second;

	*ClutterType1 = lpBAFData->GroundClutterType[0];
	*ClutterType2 = lpBAFData->GroundClutterType[1];
	*ClutterType3 = lpBAFData->GroundClutterType[2];
	*ClutterWidth = lpBAFData->GroundClutterWidth;
	*ClutterHeight = lpBAFData->GroundClutterHeight;
	*ClutterDensity = lpBAFData->GroundClutterDensity;

	return 0;
}

UINT32 mlGetRadialClutterStuff(SINT32 iClutter[5], const SCHAR szTextureName[])
{
    std::map<std::string, BumpAndFrictionData*>::iterator hashFind;

    SCHAR szCopy[32];
    SCHAR* p = nullptr;

    strncpy(szCopy, szTextureName, 31);

    // Remove '.bmp'
    p = strchr(szCopy, '.');

    if(p) *p = 0;

    SCHAR* p2 = nullptr;
    p = szCopy;
    p2 = p;

    // Remove directory path if name includes it.
    while(*p != 0)
    {
        if(*p == '\\') p2 = p + 1;
        if(*p == '/') p2 = p + 1;

        p++;
    }

    hashFind = _hashBAF.find(p2);

    if(hashFind == _hashBAF.end())
    {
        hashFind = _hashBAF.find("default");

        if(hashFind == _hashBAF.end())
        {
            return 1;
        }
    }

    BumpAndFrictionData* lpBAFData = hashFind->second;

    iClutter[0] = lpBAFData->RadialClutter[0];
    iClutter[1] = lpBAFData->RadialClutter[1];
    iClutter[2] = lpBAFData->RadialClutter[2];
    iClutter[3] = lpBAFData->RadialClutter[3];
    iClutter[4] = lpBAFData->RadialClutter[4];

	return 0;
}

UINT32 mlGetHitSpriteNumbers(SINT32* Sprite1,
                             SINT32* Sprite2,
                             SINT32* Sprite3,
                             const SINT8 * TextureName)
{
	if (Sprite1 == nullptr ||
        Sprite2 == nullptr ||
        Sprite3 == nullptr ||
        TextureName == nullptr)
	{
		return 1;
	}

    std::map<std::string, BumpAndFrictionData*>::iterator hashFind;

    SCHAR szCopy[32];
    SCHAR* p = nullptr;

    strncpy(szCopy, TextureName, 31);

    // Remove '.bmp'
    p = strchr(szCopy, '.');

    if(p) *p = 0;

    SCHAR* p2 = nullptr;
    p = szCopy;
    p2 = p;

    // Remove directory path if name includes it.
    while(*p != 0)
    {
        if(*p == '\\') p2 = p + 1;
        if(*p == '/') p2 = p + 1;

        p++;
    }

    hashFind = _hashBAF.find(p2);

    if(hashFind == _hashBAF.end())
    {
        hashFind = _hashBAF.find("default");

        if(hashFind == _hashBAF.end())
        {
            return 1;
        }
    }

    BumpAndFrictionData* lpBAFData = hashFind->second;

    *Sprite1 = lpBAFData->HitSpriteNum[0];
    *Sprite2 = lpBAFData->HitSpriteNum[1];
    *Sprite3 = lpBAFData->HitSpriteNum[2];

	return 0;
}

UINT32 mlGetRandomHitSound(const char ** HitSoundName,
						   FLOAT32 * MinVolume,
						   FLOAT32 * MaxVolumeEnergy,
						   const char * TextureName)
{
	if (HitSoundName == nullptr ||
        TextureName == nullptr ||
        MinVolume == nullptr ||
        MaxVolumeEnergy == nullptr)
	{
		return 1;
	}

    std::map<std::string, BumpAndFrictionData*>::iterator hashFind;

    UINT32 i = (UINT32)(mathGetChaoticNumber()) % 3;

    SCHAR szCopy[32];
    SCHAR* p = nullptr;

    strncpy(szCopy, TextureName, 31);

    // Remove '.bmp'
    p = strchr(szCopy, '.');

    if(p) *p = 0;

    SCHAR* p2 = nullptr;
    p = szCopy;
    p2 = p;

    // Remove directory path if name includes it.
    while(*p != 0)
    {
        if(*p == '\\') p2 = p + 1;
        if(*p == '/') p2 = p + 1;

        p++;
    }

    hashFind = _hashBAF.find(p2);

    if(hashFind == _hashBAF.end())
    {
        hashFind = _hashBAF.find("default");

        if(hashFind == _hashBAF.end())
        {
            return 1;
        }
    }

    BumpAndFrictionData* lpBAFData = hashFind->second;

    *HitSoundName = lpBAFData->HitSoundEffect[i];
    *MinVolume = lpBAFData->MinHitEffectVolume;
    *MaxVolumeEnergy = lpBAFData->MaxVolumeEnergy;

	return 0;
}

BumpAndFrictionData* mlGetScenarioBumpinessAndFriction(double x, double y, int id)
{
    const char* material = TerrainScenario::GetInstance().GetMaterial(x, y, id);

    std::map<std::string, BumpAndFrictionData*>::iterator itr;

    itr = _hashBAF.find(material);

    if(itr == _hashBAF.end())
    {
        itr = _hashBAF.find("default");
    }

    return itr->second;
}

UINT32 mlGetObjectBumpinessAndFriction(FLOAT32 * BumpDensity,
									   FLOAT32 * BumpHeight,
									   FLOAT32 * StaticFriction,
									   FLOAT32 * DynamicFriction,
									   FLOAT32 * RollCoefficientMult,
									   SINT32 * InfSurfID,
									   OBJECT * Object,
									   LOGICAL FrictionMods)
{
	LOGICAL		usingDefaultSurface = FALSE;

	if (Object == nullptr)
	{
		return 1;
	}

    UCHAR * StringPtr = nullptr;
	if(!obmainValidateObject(Object)) return 1;

	if(Object->texname == nullptr || Object->texname[0] == '\0')
	{
		if(Object->matname == nullptr || Object->matname[0] == '\0')
		{
			// Do nothing
		}
		else
		{
			StringPtr = (UCHAR*)Object->matname;
		}
	}
	else
	{
		StringPtr = (UCHAR*)Object->texname;
	}

#ifdef BETA_RELEASE
	if(OverrideSpawner == NOSPAWN_OVERRIDE)
	{
		vehVehicle* Vehicle = persGetPersonaVehiclePtr();
		if(Vehicle && Vehicle->VehicleType->Type != VEH_TYPE_SEA)
        {
            if(StringPtr)
            {
                if(!strncmp((char*)StringPtr, "water1", 6))
                {
			        StringPtr = (UCHAR*)"asphault1";
                }
            }
        }
	}
#endif

    std::map<std::string, BumpAndFrictionData*>::iterator hashFind;

    BumpAndFrictionData* lpBAFData = nullptr;

    if(StringPtr == nullptr)
    {
		usingDefaultSurface = TRUE;

        hashFind = _hashBAF.find("default");

        if(hashFind == _hashBAF.end())
        {
            return 1;
        }
    }
    else if(StringPtr[0])
    {
        SCHAR szCopy[64];

        strcpy(szCopy, (SCHAR*)StringPtr);

        // Remove '.bmp'
        SCHAR* p = strchr(szCopy, '.');

        if(p) *p = 0;

        hashFind = _hashBAF.find(szCopy);

        if(hashFind == _hashBAF.end())
        {
			usingDefaultSurface = TRUE;

            hashFind = _hashBAF.find("default");

            if(hashFind == _hashBAF.end())
            {
                return 1;
            }
        }
    }

    lpBAFData = hashFind->second;

	vehVehicle* Vehicle = persGetPersonaVehiclePtr();

 	if (BumpDensity) *BumpDensity = lpBAFData->BumpDensity;
	if (BumpHeight) *BumpHeight = lpBAFData->BumpHeight;
	if (StaticFriction) *StaticFriction = lpBAFData->StaticFriction;
	if (DynamicFriction) *DynamicFriction = lpBAFData->DynamicFriction;
	if (RollCoefficientMult) *RollCoefficientMult = lpBAFData->RollCoefficientMult;
	if (InfSurfID)
	{
		if (usingDefaultSurface)
			*InfSurfID = -1;
		else
			*InfSurfID = lpBAFData->InfSurfaceID;
	}

	if (Vehicle != nullptr && Vehicle->VehicleType != nullptr)
	{
		if (Vehicle->VehicleType->Type == VEH_TYPE_SEA)
		{
			if ((RollCoefficientMult) && (*RollCoefficientMult > 198.99f) && (*RollCoefficientMult < 199.01f))
			{
				if (StaticFriction) *StaticFriction = 0.2f;
				if (DynamicFriction) *DynamicFriction = 0.2f;
				if (BumpDensity) *BumpDensity = 0.1f;
				if (BumpHeight) *BumpHeight = 0.0f;
				if (RollCoefficientMult) *RollCoefficientMult = 1.0f;
			}
			else
			{
				if (StaticFriction) *StaticFriction = 0.2f;
				if (DynamicFriction) *DynamicFriction = 0.2f;
				if (BumpDensity) *BumpDensity = 0.1f;
				if (BumpHeight) *BumpHeight = 0.0f;
				if (RollCoefficientMult) *RollCoefficientMult = 100.0f;
			}
		}
		else
		{
			if ((RollCoefficientMult) && (*RollCoefficientMult > 198.99f) && (*RollCoefficientMult < 199.01f))
			{
				if (StaticFriction) *StaticFriction = 0.5f;
				if (DynamicFriction) *DynamicFriction = 0.5f;
				if (BumpDensity) *BumpDensity = 0.1f;
				if (BumpHeight) *BumpHeight = 0.0f;
				if (RollCoefficientMult) *RollCoefficientMult = 199.0f;
			}

			// if we are rolling, don't apply friction mods because it
			// causes odd behavior
			if(Vehicle->ModelData)
			{
				const FLOAT64 threshold = .8f;

				if(FrictionMods &&
				   fabs(Vehicle->ModelData->VehicleAtt.Roll) < threshold &&
				   fabs(Vehicle->ModelData->VehicleAtt.Pitch) < threshold)
				{
					if (StaticFriction) *StaticFriction *= Vehicle->ModelData->StaticFrictionMod;
					if (DynamicFriction) *DynamicFriction *= Vehicle->ModelData->DynamicFrictionMod;
				}
			}
		}
	}

	return 0;
}

////////////////////////////////////////////////
// Function: _SoundStopped
// Description: Callback to set sound ID to -1.
////////////////////////////////////////////////
void _SoundStopped(void* data)
{
    SINT32* pID = (SINT32*)data;

    if(pID == nullptr)
        return;

    *pID = -1;
}

static void _expireSTO(ServerTrackedObject& sto)
{
	sto.setExpired();
}

UINT32 mlResetVehiclePos(mlVehicleData * VehicleData)
{
	vehanimVehicleStateAnimations* StateAnims;
	vehVehicle* Vehicle;
	FLOAT64VECTOR Pos;
	piVECTOR TempPos;
	FLOAT64VECTOR WorldOffsetOrigin;
	//UINT32 Count = 100;
	piMATRIX AttMat = IdentityMatrix;
	piVECTOR Att;
	OBJECT* SpawnPtr = nullptr;

	//uiConsoleprintf("SCENARIO: Vehicle reset pos called!");

	Vehicle = (vehVehicle*)(VehicleData->VehicleMasterPtr);

#ifdef BETA_RELEASE
    switch(OverrideSpawner)
    {
    case NOSPAWN_OVERRIDE:
        {
            SpawnSuperCellX = 0;
            SpawnSuperCellY = 0;
            SpawnLocId = 1;
            SpawnNumber = 1;

            break;
        }
    case OLDSPAWN_OVERRIDE:
        {
            SpawnSuperCellX = 48;
            SpawnSuperCellY = 226;

            if(Vehicle->VehicleType->Type == VEH_TYPE_PERSON)
                SpawnLocId = 8;
            else if(Vehicle->VehicleType->Type == VEH_TYPE_AIR)
                SpawnLocId = 3;
            else if(Vehicle->VehicleType->Type == VEH_TYPE_GROUND)
                SpawnLocId = 7;
            else if(Vehicle->VehicleType->Type == VEH_TYPE_SEA)
                SpawnLocId = 21;
            else
                SpawnLocId = 8;

            break;
        }
    case AREA_51_OVERRIDE:
        {
	        SpawnSuperCellX = -25;
	        SpawnSuperCellY = 281;

            if(Vehicle->VehicleType->Type == VEH_TYPE_PERSON)
                SpawnLocId = 288;
            else if(Vehicle->VehicleType->Type == VEH_TYPE_AIR)
                SpawnLocId = 286;
            else if(Vehicle->VehicleType->Type == VEH_TYPE_GROUND)
                SpawnLocId = 289;
            else if(Vehicle->VehicleType->Type == VEH_TYPE_SEA)
                SpawnLocId = 300;
            else
                SpawnLocId = 288;

            break;
        }
    default:
        {
            if(SpawnLocId == -1)
            {
 		        SpawnSuperCellX = -25;
		        SpawnSuperCellY = 281;

                if(Vehicle->VehicleType->Type == VEH_TYPE_PERSON)
                    SpawnLocId = 288;
                else if(Vehicle->VehicleType->Type == VEH_TYPE_AIR)
                    SpawnLocId = 286;
				else if(Vehicle->VehicleType->Type == VEH_TYPE_GROUND)
                    SpawnLocId = 289;
                else if(Vehicle->VehicleType->Type == VEH_TYPE_SEA)
                    SpawnLocId = 300;
                else
                    SpawnLocId = 288;
            }

            break;
        }
   }
#else
    if(!Online && SpawnLocId < 0)
    {
        SpawnSuperCellX = -25;
        SpawnSuperCellY = 281;

        if(Vehicle->VehicleType->Type == VEH_TYPE_PERSON)
            SpawnLocId = 288;
        else if(Vehicle->VehicleType->Type == VEH_TYPE_AIR)
            SpawnLocId = 286;
        else if(Vehicle->VehicleType->Type == VEH_TYPE_GROUND)
            SpawnLocId = 289;
        else if(Vehicle->VehicleType->Type == VEH_TYPE_SEA)
            SpawnLocId = 300;
        else
            SpawnLocId = 288;
    }
#endif

	if(MobileSpawn)
	{
		Pos = MobileSpawnLocation;
		SpawnSuperCellX = (SINT32)Pos.x / SUPERCELL_SIZE_M;
		SpawnSuperCellY = (SINT32)Pos.y / SUPERCELL_SIZE_M;
	}

    bool SpecialOverride = false;

    switch(OverrideSpawner)
    {
#if RELEASE_CLIENT
    case BENCHANTWERP_OVERRIDE:
        {
            Pos.x = 211414.0f;
            Pos.y = 3070984.0f;
            Pos.z = 300.0f;

            SpawnSuperCellX = 211414 / SUPERCELL_SIZE_M;
            SpawnSuperCellY = 3070984 / SUPERCELL_SIZE_M;

            SpecialOverride = true;

            break;
        }
    case BENCHRAMAGEN_OVERRIDE:
        {
            Pos.x = 347533.33f;
            Pos.y = 3034163.975f;
            Pos.z = 70.0f;

            SpawnSuperCellX = 347533 / SUPERCELL_SIZE_M;
            SpawnSuperCellY = 3034163 / SUPERCELL_SIZE_M;

            SpecialOverride = true;

            break;
        }
#endif
    case SCENARIO_OVERRIDE:
        {
			SpawnSuperCellX = 0;
            SpawnSuperCellY = 0;
            SpawnLocId = 1;
            SpawnNumber = 1;

 			Pos = MobileSpawnLocation;
			SpawnSuperCellX = (SINT32)Pos.x / SUPERCELL_SIZE_M;
			SpawnSuperCellY = (SINT32)Pos.y / SUPERCELL_SIZE_M;

            SpecialOverride = true;

            break;
        }
    }

    trnGetWorldOriginOffset(&WorldOffsetOrigin);

	if(!MobileSpawn && !SpecialOverride)
	{
#ifdef BETA_RELEASE
		if(OverrideSpawner != NOSPAWN_OVERRIDE)
        {
#endif
		Pos.x = (SpawnSuperCellX * SUPERCELL_WIDTH + SUPERCELL_WIDTH * 0.5f);
		Pos.y = (SpawnSuperCellY * SUPERCELL_HEIGHT + SUPERCELL_HEIGHT * 0.5f);
		Pos.z = 0.0;
#ifdef BETA_RELEASE
		}
        else
        {
			Pos.x  = Pos.y = Pos.z = 0;
		}
#endif
	}

	if(OverrideSpawner == SCENARIO_OVERRIDE)
	{
		FLOAT64VECTOR offset = { 0, 0, 0};
		physicsSetWorldOffset(offset);
	}
	else
	{
		FLOAT64VECTOR offset = { 18*SUPERCELL_WIDTH, 235*SUPERCELL_HEIGHT, 0};
		physicsSetWorldOffset(offset);
	}

	Write_Debug("Pos x=%lf, y=%lf, z=%lf", Pos.x, Pos.y, Pos.z);

	TempPos.x = -(FLOAT32)(Pos.x - WorldOffsetOrigin.x);
	TempPos.y = -(FLOAT32)(Pos.y - WorldOffsetOrigin.y);
	TempPos.z = -(FLOAT32)(Pos.z - WorldOffsetOrigin.z);

	Write_Debug("TempPos x=%lf, y=%lf, z=%lf", TempPos.x, TempPos.y, TempPos.z);
	trnCheckAndAdjustCameraPositionWithOriginOffset(&TempPos);

	// move the world origin offset if necessary
	trnUpdateTerrainOriginOffset(FALSE);
	trnUpdateTerrainOriginOffset(FALSE); // 2nd call forces terrain to load

	playpos.x = playpos.y = playpos.z = 0.0f;

	trnldrFinishUpdate();			// Terrain system is now configured to handle this
	trnmainDoRawUpdate(FALSE);
	trnldrFinishUpdateRelocate();	// Terrain system is now configured to handle this

	if(!MobileSpawn && !SpecialOverride)
	{
#ifdef BETA_RELEASE
		if(OverrideSpawner != NOSPAWN_OVERRIDE)
        {
#endif
		if (!trnldrFindStratObjectPosition(SpawnSuperCellX,  SpawnSuperCellY, SpawnLocId, &Pos))
		{
			char s[128];
			sprintf(s, "Cannot find strat spawn object for scx = %d, scy = %d, id = %d",
						SpawnSuperCellX, SpawnSuperCellY, SpawnLocId);
			piExitProgram(_FL,s);
		}
#ifdef BETA_RELEASE
		}
#endif
	}

	TempPos.x = -(FLOAT32)(Pos.x - WorldOffsetOrigin.x);
	TempPos.y = -(FLOAT32)(Pos.y - WorldOffsetOrigin.y);
	TempPos.z = -(FLOAT32)(Pos.z - WorldOffsetOrigin.z);

	Write_Debug("TempPos x=%lf, y=%lf, z=%lf", TempPos.x, TempPos.y, TempPos.z);
	trnCheckAndAdjustCameraPositionWithOriginOffset(&TempPos);

	trnUpdateTerrainOriginOffset(FALSE); // 2nd call forces terrain to load

	playpos.x = playpos.y = playpos.z = 0.0f;

	trnldrFinishUpdate();			// Terrain system is now configured to handle this
	trnmainDoRawUpdate(FALSE);
	trnldrFinishUpdateRelocate();

#if(1)
//////////////////////////////////////////////////////////////////////
// One more time for final terrain adjustment. Some areas are not full adjusted
// and results in a piExitProgram
//////////////////////////////////////////////////////////////////////
    trnGetWorldOriginOffset(&WorldOffsetOrigin);
	if(!MobileSpawn && !SpecialOverride)
	{
#ifdef BETA_RELEASE
	    if(OverrideSpawner != NOSPAWN_OVERRIDE)
	    {
#endif
		if (!trnldrFindStratObjectPosition(SpawnSuperCellX,  SpawnSuperCellY, SpawnLocId, &Pos))
		{
			char s[128];
			sprintf(s, "Cannot find strat spawn object for scx = %d, scy = %d, id = %d",
						SpawnSuperCellX, SpawnSuperCellY, SpawnLocId);
			piExitProgram(_FL,s);
		}
#ifdef BETA_RELEASE
		}
#endif
	}
    TempPos.x = -(FLOAT32)(Pos.x - WorldOffsetOrigin.x);
	TempPos.y = -(FLOAT32)(Pos.y - WorldOffsetOrigin.y);
	TempPos.z = -(FLOAT32)(Pos.z - WorldOffsetOrigin.z);

	Write_Debug("TempPos x=%lf, y=%lf, z=%lf", TempPos.x, TempPos.y, TempPos.z);
	trnCheckAndAdjustCameraPositionWithOriginOffset(&TempPos);

	trnUpdateTerrainOriginOffset(FALSE);

	trnldrFinishUpdate();
	trnmainDoRawUpdate(FALSE);
	trnldrFinishUpdateRelocate();

    // Force rebuild of inverse matrices because of our @#$!*#@ back-ass-wards terrain
    // loading!!!!!
    trnUpdateTerrainOriginOffset(TRUE);

#endif // #if(1)

#if(1)
//////////////////////////////////////////////////////////////////////
// Ticket: 6055
// Do another update to see if this fixes the collider offset problem
//////////////////////////////////////////////////////////////////////
    trnGetWorldOriginOffset(&WorldOffsetOrigin);
	if(!MobileSpawn && !SpecialOverride)
	{
#ifdef BETA_RELEASE
	    if(OverrideSpawner != NOSPAWN_OVERRIDE)
	    {
#endif
		if (!trnldrFindStratObjectPosition(SpawnSuperCellX,  SpawnSuperCellY, SpawnLocId, &Pos))
		{
			char s[128];
			sprintf(s, "Cannot find strat spawn object for scx = %d, scy = %d, id = %d",
						SpawnSuperCellX, SpawnSuperCellY, SpawnLocId);
			piExitProgram(_FL,s);
		}
#ifdef BETA_RELEASE
		}
#endif
	}
    TempPos.x = -(FLOAT32)(Pos.x - WorldOffsetOrigin.x);
	TempPos.y = -(FLOAT32)(Pos.y - WorldOffsetOrigin.y);
	TempPos.z = -(FLOAT32)(Pos.z - WorldOffsetOrigin.z);

	Write_Debug("TempPos x=%lf, y=%lf, z=%lf", TempPos.x, TempPos.y, TempPos.z);
	trnCheckAndAdjustCameraPositionWithOriginOffset(&TempPos);

	trnUpdateTerrainOriginOffset(FALSE);

	trnldrFinishUpdate();
	trnmainDoRawUpdate(FALSE);
	trnldrFinishUpdateRelocate();

    // Force rebuild of inverse matrices because of our @#$!*#@ back-ass-wards terrain
    // loading!!!!!
    trnUpdateTerrainOriginOffset(TRUE);

#endif // #if(1)

    trnGetWorldOriginOffset(&WorldOffsetOrigin);

	Write_Debug("mlResetVehiclePos is trying to spawn at: scx=%d, scy=%d, id=%d, num=%d",
					SpawnSuperCellX, SpawnSuperCellY, SpawnLocId, SpawnNumber);

	if(!MobileSpawn && !SpecialOverride)
	{
#ifdef BETA_RELEASE
		if(OverrideSpawner != NOSPAWN_OVERRIDE){
#endif
		if (SpawnNumber == 0)
		{
			Write_Debug("INVALID SPAWN NUMBER!!!!!!!!");
			SpawnNumber = 1;
		}

		if (SpawnNumber > 0)
		{
			trnGetSpawnObject(&SpawnPtr,
                              SpawnSuperCellX,
                              SpawnSuperCellY,
                              SpawnLocId,
                              SpawnNumber - 1,
                              Vehicle->VehicleType->WeaponSpec);

			if (SpawnPtr == nullptr)
			{
				char s[128];

				sprintf(s,"Cannot find spawn object for scx = %d, scy = %d, id = %d",
							SpawnSuperCellX, SpawnSuperCellY, SpawnLocId);
				piExitProgram(_FL,s);
			}
		}

		// determine where to go
		AttMat = SpawnPtr->dof->dofmatrix2;
		math3dMatrixMultiply(&AttMat,&AttMat,&SpawnPtr->child->mat);
		AttMat._13 = -AttMat._13;
		AttMat._23 = -AttMat._23;
		AttMat._33 = -AttMat._33;
		AttMat._43 = -AttMat._43;
		Pos.x = AttMat._41;
		Pos.y = AttMat._42;
		Pos.z = AttMat._43;

		Pos.x += WorldOffsetOrigin.x;
		Pos.y += WorldOffsetOrigin.y;
#ifdef BETA_RELEASE
		}
#endif
}
	if (VehicleData->VehicleObjectData != nullptr)
	{
		Pos.z += WorldOffsetOrigin.z - VehicleData->VehicleObjectData->LowestPoint;
	}
	else
	{
		Pos.z += WorldOffsetOrigin.z + 1.0f;
	}

	if (Vehicle && Vehicle->VehicleType && Vehicle->VehicleType->Type == VEH_TYPE_PERSON)
		Pos.z += 1.0f;

	if(!MobileSpawn && !SpecialOverride)
	{
	// adjust height based on place mode
#ifdef BETA_RELEASE
		if(OverrideSpawner != NOSPAWN_OVERRIDE){
#endif
		Pos.z += -(SpawnPtr->child->MutablePositions()[0].z - SpawnPtr->child->ImmutablePositions()[0].z);
#ifdef BETA_RELEASE
		}
#endif
	}
	AttMat._41 = AttMat._42 = AttMat._43 = 0.0f;

	Write_Debug("Spawning': scx=%d, scy=%d, id=%d, num=%d", SpawnSuperCellX, SpawnSuperCellY, SpawnLocId, SpawnNumber);
	Write_Debug("Spawn Position: x=%lf, y=%lf, z=%lf", Pos.x, Pos.y, Pos.z);

	_TunnelResetVehiclePos(VehicleData,Pos.z);
	VehicleData->VehiclePos.x = Pos.x;
	VehicleData->AdjustedForCGPos.x = Pos.x;
	VehicleData->VehiclePos.y = Pos.y;
	VehicleData->AdjustedForCGPos.y = Pos.y;
	VehicleData->LastVehicleUpdateTime = frametime;

	mathEul_FromMatrix(&Att,&AttMat,EulOrdXYZr);

	VehicleData->VehicleAtt.Pitch = -Att.x;
	VehicleData->VehicleAtt.Roll = -Att.y;
	VehicleData->VehicleAtt.Yaw = -Att.z;

	if((MobileSpawn && !SpecialOverride) || SCEN_MGR->IsActive())
		VehicleData->VehicleAtt.Yaw = MobileSpawnYaw;

	VehicleData->FailsafeMode = 1;
	VehicleData->GearSetting = VehicleData->LastGearSetting = 1.0;
	VehicleData->GearPosition = 1.0;
	VehicleData->TailWheelLocked = (VehicleData->VehicleMasterPtr->VehicleType->Type == VEH_TYPE_AIR);
	VehicleData->LastResetTime = frametime;
	VehicleData->LastVehicleUpdateTime = frametime;

	//uiConsoleprintf("SCENARIO: Spawn position is %.2f, %.2f, %.2f", &Pos.x, &Pos.y, &Pos.z);
	//uiConsoleprintf("SCENARIO: Vehicle position is %.2f, %.2f, %.2f", &VehicleData->VehiclePos.x, &VehicleData->VehiclePos.y, &VehicleData->VehiclePos.z);

	_LastTOUpdateTime = 0;

	if (Vehicle)
	{
		if(Vehicle->VehicleType->Type == VEH_TYPE_FLOATER)
		{
			VehicleData->GearSetting = VehicleData->LastGearSetting = 0;
			VehicleData->GearPosition = 0;
		}else
		{
			vehAnimChangeVehicleStateByName(Vehicle, (UCHAR*)"gearstate", TRUE);

			StateAnims = &Vehicle->StateAnimation;
			if (StateAnims != nullptr)
			{
				StateAnims->GearDOFVar = 1.0f;
				StateAnims->GearPos = 1.0f;
				StateAnims->GearSetting = 1;

				vehAnimResetVehicleStateAnimations(Vehicle);
			}
		}
	}

	if (VehicleData && VehicleData->VehicleObjectData && VehicleData->VehicleObjectData->PlaneObject)
	{
		mlMoveAndRotateObject(VehicleData->VehicleObjectData->PlaneObject,
								&VehicleData->VehiclePos, &VehicleData->VehicleAtt);
	}

	trnldrFinishUpdate();

	if(SCEN_MGR->IsActive())
		SCEN_MGR->SetTerrainReadyToLoad();

	//uiConsoleprintf("SCENARIO: Updating terrain collide lists!");
	_UpdateTerrainCollideLists(VehicleData,1);

	fxAmbStop(FALSE);

	trnmainSetLightingUpdate(TRUE);
	obdrawUpdateTerrainObjectLighting(TRUE);

	VehicleData->GetStartTime = 1;
	VehicleData->HasSentUpdate = 0;

	// setup any bomb sights
	persClearBombsight();
	persSetPersonaPositionCallback(1, IMAP_MOM_POSITION_1, 0);

	if (Vehicle && Vehicle->VehicleType && Vehicle->VehicleType->BombSightPosition && Vehicle->VehicleType->BombSightType)
	{
		persBombsightSetup(Vehicle->VehicleType->BombSightType,
							Vehicle->VehicleType->BombSightMount, (void*)Vehicle->ModelData);
	}

    if(MobileSpawn && !SpecialOverride)
    {
        // Temporary for now so Oli can test.
        // (Eventually need to collide with short collide list)
        if((SINT32)Pos.z == 1)
        {
			COLLISION c{};

            c.newpos.x = (FLOAT32)(Pos.x - WorldOriginOffset.x);
            c.newpos.y = (FLOAT32)(Pos.y - WorldOriginOffset.y);

            c.oldpos = c.newpos;

            c.oldpos.z = -10000.0f;
            c.newpos.z =  10000.0f;

            if(trncdCheckTerrainDown(&c))
            {
                VehicleData->VehiclePos.z = -c.colpos.z + 1.0f;
                VehicleData->AdjustedForCGPos.z = -c.colpos.z + 1.0f;
            }
        }
    }

	Vec3d newSpawnPos = Vec3d(Pos);
	Vec3d diff = SpawnPos - newSpawnPos;
	const Instance::Descriptor& instance = PlayerClient()->GetInstance();

	// expire all sto's if we spawned far enough away
	if(instance.id() != SpawnInstance || (!SCEN_MGR->IsActive() && diff.length() > STO_SPAWN_THRESHOLD))
	{
		wwiiForEachSTO(_expireSTO);
		wwiiExpireSTOs();
	}

	// save our current spawn pos
	SpawnPos.set(newSpawnPos);
	SpawnInstance = instance.id();

	if(!SCEN_MGR->IsActive() && cfgPrefsGetBool("app.missionwaypoints", true))
	{
		// offset the origin position slightly because using an exact
		// position causes an anomalie with the screen position lookup function
		// which in turn causes the HUD to be clipped out onspawn and only shows
		// after you move a little
		FLOAT64VECTOR opos = Vehicle->ModelData->VehiclePos;

		opos.x += .5f;
		opos.y += .5f;

		HUD_MGR->AddObject(new PersHUDManager::HUDWaypoint(opos, "origin", 1.0f));

		MissionDetail& mission = PlayerClient()->SelectedMission();

		StratDB::StratObject* target = StratDB::FindStratObject(mission.m_target);

		if(target)
			HUD_MGR->AddObject(new PersHUDManager::HUDWaypoint(target, "target", 1.5f));
	}

	return 0;
}

UINT32 _UpdateVehicleSounds(mlVehicleData * VehicleData)
{
	FLOAT64VECTOR Pos;
	FLOAT64 Speed;
	FLOAT32 Volume,Pitch;
	LOGICAL hasBailed = vehmainHasBailedOut();

	//UINT32 PlayType;

    //[DCF] HACK!
    if(VehicleData->WindSoundId <= 0)
        VehicleData->WindSoundStarted = 0;

	if (VehicleData->WindSoundStarted == 0)
	{
		VehicleData->WindSoundStarted = 1;
		VehicleData->WindSoundId = -1;

		if(VehicleData->WindSound[0])
		{
			Pos.x = Pos.y = Pos.z = 0.0;

            SINT32 iID = fxSndGetNewSound();

            if(iID > 0)
            {
                fxSndSetSoundName(iID, (SCHAR*)VehicleData->WindSound);
                fxSndSetPriority(iID, IDE_FP_PRIORITY_LOW);
                fxSndSetSoundType(iID, TYPE_INFINITE_EXTERNAL_SOUND);
                fxSndSetPitch(iID, 1.0f);
                fxSndSetVolume(iID, 0.0f);
                fxSndSetPosition(iID, Pos.x, Pos.y, Pos.z);
                fxSndSetEmitterObject(iID, VehicleData->VehicleObjectData->PlaneObject);
                fxSndSetPlayType(iID, DEF_PLAYTYPE_OBJECT);
                fxSndSetKillCallback(iID, _SoundStopped);
                fxSndSetKillData(iID, &VehicleData->WindSoundId);

                if(fxSndInitSound(iID))
                {
                    VehicleData->WindSoundId = iID;
                }
                else
                {
                    VehicleData->WindSoundId = -1;
                }
            }
		}
	}

	if(VehicleData->WindSoundId > 0)
	{
		Speed = mathExactVectorLength(&VehicleData->VehicleVel);

		Volume = 0.0f;
		Pitch = 1.0f;
		if (Speed > 25.0)
		{
			if (Speed < 50.0)
			{
				Volume = (FLOAT32)((Speed - 25.0) / 25.0);
			}
			else
			{
				Volume = 1.0f;
			}
			Pitch = (FLOAT32)((Speed - 25.0) / 200.0);
			Pitch += 0.5f;
		}

		if(hasBailed)
			Volume = 0.0f;

        fxSndChangeVolume(VehicleData->WindSoundId, Volume);
		fxSndChangePitch(VehicleData->WindSoundId, Pitch);
	}

    //[DCF] HACK!
    if(VehicleData->StallSoundId <= 0)
        VehicleData->StallSoundStarted = 0;

	if (VehicleData->StallSoundStarted == 0 &&
		VehicleData->VehicleObjectData != nullptr &&
		VehicleData->VehicleObjectData->PlaneObject != nullptr &&
		VehicleData->StallSound[0])
	{
		Pos.x = Pos.y = Pos.z = 0.0;

        SINT32 iID = fxSndGetNewSound();

        if(iID > 0)
        {
            fxSndSetSoundName(iID, (SCHAR*)VehicleData->StallSound);
            fxSndSetPriority(iID, IDE_FP_PRIORITY_MANDATORY);
            fxSndSetSoundType(iID, TYPE_INFINITE_SOUND);
            fxSndSetVolume(iID, 0.0f);
            fxSndSetPitch(iID, 1.0f);
            fxSndSetPosition(iID, Pos.x, Pos.y, Pos.z);
            fxSndSetEmitterObject(iID, VehicleData->VehicleObjectData->PlaneObject);
            fxSndSetPlayType(iID, DEF_PLAYTYPE_OBJECT);
            fxSndSetKillCallback(iID, _SoundStopped);
            fxSndSetKillData(iID, &VehicleData->StallSoundId);

            if(fxSndInitSound(iID))
            {
                VehicleData->StallSoundId = iID;
                VehicleData->StallSoundStarted = 1;
            }
            else
            {
                VehicleData->StallSoundId = -1;
            }
        }
	}

	if (VehicleData->StallSoundVolume > 1.0)
		VehicleData->StallSoundVolume = 1.0;

	if(hasBailed)
		VehicleData->StallSoundVolume = 0;

	if (VehicleData->StallSoundId > 0)
		fxSndChangeVolume(VehicleData->StallSoundId,(FLOAT32)(VehicleData->StallSoundVolume));

	VehicleData->StallSoundVolume = 0.0;

	if(!hasBailed && VehicleData->LastGearSetting != VehicleData->GearSetting)
	{
		if(VehicleData->GearSound[0])
		{
            if(VehicleData->gearSoundId > 0)
            {
                fxSndStopSound(VehicleData->gearSoundId);
                VehicleData->gearSoundId = 0;
            }

			Pos.x = Pos.y = Pos.z = 0.0;

            SINT32 id = fxSndGetNewSound();

            if(id > 0)
            {
                fxSndSetSoundName(id, (SCHAR*)VehicleData->GearSound);
                fxSndSetPriority(id, IDE_FP_PRIORITY_HIGH);
                fxSndSetSoundType(id, TYPE_FINITE_SOUND);
                fxSndSetVolume(id, 1.0f);
                fxSndSetPitch(id, 1.0f);
                fxSndSetPosition(id, Pos.x, Pos.y, Pos.z);
                fxSndSetEmitterObject(id, VehicleData->VehicleObjectData->PlaneObject);
                fxSndSetPlayType(id, DEF_PLAYTYPE_OBJECT);

                fxSndInitSound(id);

                VehicleData->gearSoundId = id;
            }
		}
	}

	if(!hasBailed && VehicleData->LastFlapSetting != VehicleData->FlapSetting)
	{
		if (VehicleData->FlapsSound[0])
		{
            if(VehicleData->flapSoundId > 0)
            {
                fxSndStopSound(VehicleData->flapSoundId);
                VehicleData->flapSoundId = 0;
            }

			Pos.x = Pos.y = Pos.z = 0.0;

            SINT32 id = fxSndGetNewSound();

            if(id > 0)
            {
                fxSndSetSoundName(id, (SCHAR*)VehicleData->FlapsSound);
                fxSndSetPriority(id, IDE_FP_PRIORITY_HIGH);
                fxSndSetSoundType(id, TYPE_FINITE_SOUND);
                fxSndSetVolume(id, 1.0f);
                fxSndSetPitch(id, 1.0f);
                fxSndSetPosition(id, Pos.x, Pos.y, Pos.z);
                fxSndSetEmitterObject(id, VehicleData->VehicleObjectData->PlaneObject);
                fxSndSetPlayType(id, DEF_PLAYTYPE_OBJECT);

                fxSndInitSound(id);

                VehicleData->flapSoundId = id;
            }
		}
	}

	if(!hasBailed && VehicleData->GearBeingDamaged && VehicleData->GearDamageSoundId < 1)
	{
		Pos.x = Pos.y = Pos.z = 0.0;

        SINT32 iID = fxSndGetNewSound();

        if(iID > 0)
        {
            fxSndSetSoundName(iID, "sounds\\rattle.wav");
            fxSndSetPriority(iID, IDE_FP_PRIORITY_MANDATORY);
            fxSndSetSoundType(iID, TYPE_INFINITE_SOUND);
            fxSndSetVolume(iID, 0.0f);
            fxSndSetPitch(iID, 1.0f);
            fxSndSetPosition(iID, Pos.x, Pos.y, Pos.z);
            fxSndSetEmitterObject(iID, VehicleData->VehicleObjectData->PlaneObject);
            fxSndSetPlayType(iID, DEF_PLAYTYPE_OBJECT);
            fxSndSetKillCallback(iID, _SoundStopped);
            fxSndSetKillData(iID, &VehicleData->GearDamageSoundId);

            if(fxSndInitSound(iID))
            {
	            VehicleData->GearDamageSoundId = iID;
            }
        }
	}

	if((hasBailed && VehicleData->GearDamageSoundId > 0) || (!VehicleData->GearBeingDamaged && VehicleData->GearDamageSoundId > 0))
		fxSndStopSound(VehicleData->GearDamageSoundId);

	if(!hasBailed && VehicleData->FlapsBeingDamaged && VehicleData->FlapDamageSoundId < 1)
	{
		Pos.x = Pos.y = Pos.z = 0.0;

        SINT32 iID = fxSndGetNewSound();

        if(iID > 0)
        {
            fxSndSetSoundName(iID, "sounds\\flapwind.wav");
            fxSndSetPriority(iID, IDE_FP_PRIORITY_MANDATORY);
            fxSndSetSoundType(iID, TYPE_INFINITE_SOUND);
            fxSndSetVolume(iID, 0.0f);
            fxSndSetPitch(iID, 1.0f);
            fxSndSetPosition(iID, Pos.x, Pos.y, Pos.z);
            fxSndSetEmitterObject(iID, VehicleData->VehicleObjectData->PlaneObject);
            fxSndSetPlayType(iID, DEF_PLAYTYPE_OBJECT);
            fxSndSetKillCallback(iID, _SoundStopped);
            fxSndSetKillData(iID, &VehicleData->FlapDamageSoundId);

            if(fxSndInitSound(iID))
            {
			    VehicleData->FlapDamageSoundId = iID;
            }
        }
	}

	if((hasBailed && VehicleData->FlapDamageSoundId > 0) || (!VehicleData->FlapsBeingDamaged && VehicleData->FlapDamageSoundId > 0))
		fxSndStopSound(VehicleData->FlapDamageSoundId);

	if(!hasBailed && !VehicleData->LastGearJammed && VehicleData->GearJammed)
	{
		Pos.x = Pos.y = Pos.z = 0.0;

        SINT32 iID = fxSndGetNewSound();

        if(iID > 0)
        {
            fxSndSetSoundName(iID, "sounds\\geardmg.wav");
            fxSndSetPriority(iID, IDE_FP_PRIORITY_MANDATORY);
            fxSndSetSoundType(iID, TYPE_FINITE_SOUND);
            fxSndSetVolume(iID, 1.0f);
            fxSndSetPitch(iID, 1.0f);
            fxSndSetPosition(iID, Pos.x, Pos.y, Pos.z);
            fxSndSetEmitterObject(iID, VehicleData->VehicleObjectData->PlaneObject);
            fxSndSetPlayType(iID, DEF_PLAYTYPE_HEAD);

            fxSndInitSound(iID);
        }
	}

	if(hasBailed)
	{
		VehicleData->GearDamageSoundVolume = 0.0f;
		VehicleData->FlapDamageSoundVolume = 0.0f;
	}

	if(VehicleData->GearDamageSoundId > 0)
		fxSndChangeVolume(VehicleData->GearDamageSoundId, VehicleData->GearDamageSoundVolume);

	if (VehicleData->FlapDamageSoundId > 0)
		fxSndChangeVolume(VehicleData->FlapDamageSoundId, VehicleData->FlapDamageSoundVolume);

	// tank tread/brake
	if(	VehicleData->VehicleMasterPtr->VehicleType->Type == VEH_TYPE_GROUND)
	{
		FLOAT64 speed2 = mathExactVectorLength2(&VehicleData->VehicleVel);
		LOGICAL playTreadSound = speed2 > 1.0;
		vehfxPlayTankTreadInternal(VehicleData->VehicleMasterPtr->iFXVehicleID, playTreadSound);

        LOGICAL playBrakeSound = (VehicleData->BrakeForceCenter > .5 && speed2 > 1.0f);
		vehfxPlayTankBrake(VehicleData->VehicleMasterPtr->iFXVehicleID, playBrakeSound);
	}

	VehicleData->LastFlapSetting = VehicleData->FlapSetting;
	VehicleData->LastGearSetting = VehicleData->GearSetting;
	VehicleData->LastGearJammed = VehicleData->GearJammed;

	return 0;
}


UINT32 mlDetermineVehicleCollisions(mlVehicleData * VehicleData, FLOAT64 DeltaTime)
{
	if (PN_UNLIKELY(VehicleData == nullptr || VehicleData->VehicleObjectData == nullptr))
		return 0;
	OBJECT* HitBox = VehicleData->VehicleObjectData->HitBox;
	if (PN_UNLIKELY(HitBox == nullptr || HitBox->child == nullptr))
		return 0;

	piVECTOR TempPos;
	FLOAT64VECTOR WorldOffsetOrigin;
	UINT32 i;
	piVECTOR V1,V2;

	SINT32 T2;

	trnGetWorldOriginOffset(&WorldOffsetOrigin);
	WorldOffsetOrigin.z = -WorldOffsetOrigin.z;

	TempPos.x = (FLOAT32)(VehicleData->AdjustedForCGPos.x - WorldOffsetOrigin.x);
	TempPos.y = (FLOAT32)(VehicleData->AdjustedForCGPos.y - WorldOffsetOrigin.y);
	TempPos.z = (FLOAT32)(VehicleData->AdjustedForCGPos.z - WorldOffsetOrigin.z);

	for (i=0;i<VehicleData->NumCageCollideLines;i++)
	{
		V1 = VehicleData->CageLineV1[i];
		V2 = VehicleData->CageLineV2[i];
		math3dVectorMultiply(&V1,&V1,&VehicleData->CageLineObject[i]->matint);
		math3dVectorMultiply(&V2,&V2,&VehicleData->CageLineObject[i]->matint);
		V1.z = -V1.z;
		V2.z = -V2.z;
		math3dVectorMultiply(&V1,&V1,&VehicleData->VehicleAttMat);
		math3dVectorMultiply(&V2,&V2,&VehicleData->VehicleAttMat);
		mathAddVectors(&V1,&V1,&TempPos);
		mathAddVectors(&V2,&V2,&TempPos);
		V1.z = -V1.z;
		V2.z = -V2.z;
		_DoVehicleCollision(VehicleData,DeltaTime,&V1,&V2);
	}

	_DoODEVehicleCollision(VehicleData, DeltaTime);

	T2 = piReadTimeStamp32();

	return 0;
}

/************************************************************************************************
	_DoVehicleCollision
*************************************************************************************************/

#define NUM_COLLISION_SLOTS 32
#define BOUNCE_BACK .9f

static UINT32 _DoVehicleCollision(mlVehicleData* VehicleData, FLOAT64 DeltaTime,
									piVECTOR* V2, piVECTOR* V1)
{
    piVECTOR CollisionPnt = {0, 0, 0};
	FLOAT64VECTOR WorldOffsetOrigin = {0, 0, 0};
	FLOAT64 Len = 0;
	FLOAT64VECTOR Vel = {0, 0, 0};
    FLOAT64VECTOR DeltaVel = {0, 0, 0};
	piVECTOR Norm = {0, 0, 0};
	piVECTOR TempPos = {0, 0, 0};
	FASTCOLLIDE CollisionList[NUM_COLLISION_SLOTS]{};
	FASTCOLLIDE* CollideListPtr = nullptr;
	UINT32 NumCollides = 0;
	UINT32 CollideNum = 0;
	LOGICAL RetVal = 0;
	piMATRIX TempMat = IdentityMatrix;
	FLOAT32 RollFrictionCo = 0;
	vehVehicle* Vehicle = nullptr;
	UINT32 collisionSlotsLeft = 0;
	UINT32 i = 0;

#ifdef BETA_RELEASE // Collider Cage
	if(VehicleObjectAnalysis && DisplayCollideLines && IsFirstTic)
		draw3dLineDebug(V1, V2, colorBLUEALPHA);
#endif

	Vehicle = VehicleData->VehicleMasterPtr;
	trnGetWorldOriginOffset(&WorldOffsetOrigin);
	WorldOffsetOrigin.z = -WorldOffsetOrigin.z;
	collisionSlotsLeft = NUM_COLLISION_SLOTS;
	CollideListPtr = CollisionList;

	// do boundary collisions
	/*
	if(SCEN_MGR->IsActive())
	{
		trncdCollideUsingODE(Vec3d(*V1),
								Vec3d(*V2),
								&CollideListPtr,
								*collisionSlotsLeft,
								*CurrentObject,
								*TopObject,
								*TO,
								COLLIDER_BLOCK);

	}
	*/

	trncdCollideWithListOfTerrainObjects1(&CollideListPtr, &collisionSlotsLeft,
										 VehicleData->TrnCDListObject,  VehicleData->TrnCDListTOs,
										 ML_NUM_MED_LIST_ELEMENTS,  V1,  V2,  1);

	NumCollides = NUM_COLLISION_SLOTS - collisionSlotsLeft;

	if (NumCollides < NUM_COLLISION_SLOTS)
	{
		RetVal = trncdCollideWithPolyList(V1, V2, &CollisionList[NumCollides].CollidePnt,
								&CollisionList[NumCollides].CollideObject, &CollisionList[NumCollides].PolyNum,
								VehicleData->TerrainAGLObjectList, VehicleData->TerrainAGLPolyList, VehicleData->NumTerrainAGL);

		if (RetVal != 0)
		{
			CollisionList[NumCollides].CollideTime = (FLOAT32)((CollisionList[NumCollides].CollidePnt.z - V2->z) /  (V1->z - V2->z));
			CollisionList[NumCollides].CollideTime = (FLOAT32)(1.0 - CollisionList[NumCollides].CollideTime);
			CollisionList[NumCollides].Alpha = 1.0;
			NumCollides++;
			collisionSlotsLeft--;
			CollideListPtr = &CollisionList[NumCollides];
		}
	}

	// throw in vehicle collision
	if (NumCollides < NUM_COLLISION_SLOTS)
	{
		weapCollideRoundWithVehicles(&CollideListPtr,  &collisionSlotsLeft,  V1, V2, VehicleData->LastVehicleUpdateTime / 1000.0,
							 VehicleData->LastVehicleUpdateTime / 1000.0 + DeltaTime, 0, 0, &WorldOffsetOrigin, 1, VehicleData->CollisionSide);

		for (i = NumCollides; i < NUM_COLLISION_SLOTS - collisionSlotsLeft; i++)
		{
			CollisionList[i].Flags |= _AMMOCD_FLAG_ISAVEHICLE;
		}

	}

	NumCollides = NUM_COLLISION_SLOTS - collisionSlotsLeft;

	if (NumCollides == 0) return 0;

	mlUtilitySortCollideList(CollisionList,NumCollides);

	// Determine which collision we're actually gonna use
	for(CollideNum = 0; CollideNum < NumCollides; CollideNum++)
	{
		if(Vehicle->VehicleType->Type != VEH_TYPE_AIR)
		{// ignore air collisions if we're a ground vehicle, to prevent "tank flipping"
			if(CollisionList[CollideNum].Flags & _AMMOCD_FLAG_ISAVEHICLE)
			{
				vehVehicle* collideVehicle = (vehVehicle*)(CollisionList[CollideNum].TopLevelObject); // <--ugh

				if(collideVehicle && collideVehicle->VehicleType->Type == VEH_TYPE_AIR)
					continue;

				if(Vehicle->VehicleType->Type == VEH_TYPE_SEA && collideVehicle->VehicleType->Type != VEH_TYPE_SEA)
					continue; // ship ain't stopping for no one
			}
		}

		if(Vehicle->VehicleType->Type == VEH_TYPE_SEA)
		{
			mlGetObjectBumpinessAndFriction(nullptr, nullptr, nullptr, nullptr,
											&RollFrictionCo, nullptr, CollisionList[CollideNum].CollideObject);

			if(RollFrictionCo <= 0.99f || RollFrictionCo >= 1.01f)
				break;

		}
		else if(CollisionList[CollideNum].Alpha >= 0.5)
		{
			/*
			if(gCustomTerrain &&
				CollisionList[CollideNum].CollideObject &&
				SCEN_MGR->IsActive() &&
				trnwtrIsWaterTexture(CollisionList[CollideNum].CollideObject->texid))
			{
				continue;
			}
			*/

			break;
		}
	}

	if (CollideNum == NumCollides) return 0;

	CollisionPnt = CollisionList[CollideNum].CollidePnt;

    if(CollisionList[CollideNum].CollideObject)
        math3dVectorMultiply(&CollisionPnt,&CollisionPnt,&CollisionList[CollideNum].CollideObject->mat);

	//if (CollisionList[CollideNum].CollideObject->flag2 & OBJECT_HASALPHA)
	if (1)
	{
		TempPos.x = (FLOAT32)(VehicleData->AdjustedForCGPos.x - WorldOffsetOrigin.x);
		TempPos.y = (FLOAT32)(VehicleData->AdjustedForCGPos.y - WorldOffsetOrigin.y);
		TempPos.z = -(FLOAT32)(VehicleData->AdjustedForCGPos.z - WorldOffsetOrigin.z);

		mathSubVectors(&Norm,&CollisionPnt,&TempPos);

		Norm.x = Norm.x;
		Norm.y = Norm.y;
		Norm.z = Norm.z;

		math3dNormalize(&Norm);
	}
	else
	{
		Norm = CollisionList[CollideNum].CollideObject->psnorms[CollisionList[CollideNum].PolyNum];

		TempMat = CollisionList[CollideNum].CollideObject->mat;
		TempMat._41 = TempMat._42 = TempMat._43 = 0.0f;
		math3dVectorMultiply(&Norm,&Norm,&TempMat);
	}

	Vel.x = -VehicleData->VehicleVel.x;
	Vel.y = VehicleData->VehicleVel.y;
	Vel.z = VehicleData->VehicleVel.z;

	Len = mathVectDotProduct(&Vel,&Norm);
	if (Len >= 0.0f)
	{
		if (Vehicle->VehicleType->Type == VEH_TYPE_SEA)
		{
			DeltaVel.x = Norm.x * Len * 0.5;
			DeltaVel.y = Norm.y * Len * 0.5;
			DeltaVel.z = Norm.z * Len * 0.5;
		}
		else
		{
			DeltaVel.x = Norm.x * Len * BOUNCE_BACK;
			DeltaVel.y = Norm.y * Len * BOUNCE_BACK;
			DeltaVel.z = Norm.z * Len * BOUNCE_BACK;
		}

		Vel.x -= DeltaVel.x;
		Vel.y -= DeltaVel.y;
		Vel.z -= DeltaVel.z;

		VehicleData->VehicleVel.x = -Vel.x;
		VehicleData->VehicleVel.y = Vel.y;
		VehicleData->VehicleVel.z = Vel.z;

		VehicleData->VehicleAcc.x -= -DeltaVel.x / DeltaTime;
		VehicleData->VehicleAcc.y -= DeltaVel.y / DeltaTime;
		VehicleData->VehicleAcc.z -= DeltaVel.z / DeltaTime;

		if (Len > 30.0)
		{
			vehAnimChangeVehicleStateByName(VehicleData->VehicleMasterPtr, (UCHAR*)"crashexplosion", TRUE);
			persKillCrew();
			VehicleData->FreezeVehicle = 1;
			vehVisDamageHulkVehicle(VehicleData->VehicleMasterPtr, TRUE);
		}
	}

	return 0;
}

static UINT32 _DoODEVehicleCollision(mlVehicleData* VehicleData, FLOAT64 DeltaTime)
{
	vehVehicle* vehicle = VehicleData->VehicleMasterPtr;
	if(!vehicle || !vehicle->collider) return 0;

	if(!SCEN_MGR->IsActive()) return 0;

    piVECTOR CollisionPnt = {0, 0, 0};
	FLOAT64VECTOR WorldOffsetOrigin = {0, 0, 0};
	FLOAT64 Len = 0;
	FLOAT64VECTOR Vel = {0, 0, 0};
    FLOAT64VECTOR DeltaVel = {0, 0, 0};
	piVECTOR Norm = {0, 0, 0};
	piVECTOR TempPos = {0, 0, 0};

	trnGetWorldOriginOffset(&WorldOffsetOrigin);
	WorldOffsetOrigin.z = -WorldOffsetOrigin.z;

	Vec3d wo = WorldOffsetOrigin;

	if(vehicle->collider->findCollisions(true))
	{
		colliderQuery::CollisionList& hits = vehicle->collider->getCollisions();

		for(size_t i = 0; i < hits.size(); i++)
		{
			const colliderQuery::Collision& hit = hits.at(i);

			if(colliderIsType(hit.g2, COLLIDER_BLOCK))
			{
				ScenarioBlock* block = (ScenarioBlock*)dGeomGetData(hit.g2);

				// see if this block pertains to our vehicle type
				if(block)
				{
					// if it doesn't pertain, dont use it
					if(!SCEN_MGR->PlayerRelevant(block))
						continue;

					// otherwise it does pertain to us
					// try running any other events with the same name as this block
					SCEN_MGR->RunObject(block->name.c_str(), 1);
				}
			}

			Vec3d pos = physicsPhysicalToGame(hit.pos);

			// world offset it
			pos = pos - wo;
			pos.z() = -pos.z();

			// put into a piVECTOR
			pos.get(CollisionPnt);

			// from here below is the same as above
			TempPos.x = (FLOAT32)(VehicleData->AdjustedForCGPos.x - WorldOffsetOrigin.x);
			TempPos.y = (FLOAT32)(VehicleData->AdjustedForCGPos.y - WorldOffsetOrigin.y);
			TempPos.z = -(FLOAT32)(VehicleData->AdjustedForCGPos.z - WorldOffsetOrigin.z);

			mathSubVectors(&Norm,&CollisionPnt,&TempPos);

			Norm.x = Norm.x;
			Norm.y = Norm.y;
			Norm.z = Norm.z;

			math3dNormalize(&Norm);

			/*
			Vec3d normal	= hit.normal;
			Vec3d dir		= VehicleData->VehicleVel;

			dir.normalize();
			dir.x() = -dir.x();

			double angle	= dir * normal;

			// start out by throwing out contacts with normals
			// in the direction of our velocity
			if(angle < 0)
			{
				colliderDebugContact(hit.pos, normal, colorBLACKALPHA, nullptr, hit.depth);
				continue;
			}
			else
			{
				colliderDebugContact(hit.pos, normal, colorREDALPHA, nullptr, hit.depth);
			}

			Norm.x = hit.normal.x();
			Norm.y = hit.normal.y();
			Norm.z = hit.normal.z();
			*/

			Vel.x = -VehicleData->VehicleVel.x;
			Vel.y = VehicleData->VehicleVel.y;
			Vel.z = VehicleData->VehicleVel.z;

			Len = mathVectDotProduct(&Vel,&Norm);

			if (Len >= 0.0f)
			{
				if (vehicle->VehicleType->Type == VEH_TYPE_SEA)
				{
					DeltaVel.x = Norm.x * Len * 0.5;
					DeltaVel.y = Norm.y * Len * 0.5;
					DeltaVel.z = Norm.z * Len * 0.5;
				}
				else
				{
					DeltaVel.x = Norm.x * Len * BOUNCE_BACK;
					DeltaVel.y = Norm.y * Len * BOUNCE_BACK;
					DeltaVel.z = Norm.z * Len * BOUNCE_BACK;
				}

				Vel.x -= DeltaVel.x;
				Vel.y -= DeltaVel.y;
				Vel.z -= DeltaVel.z;

				VehicleData->VehicleVel.x = -Vel.x;
				VehicleData->VehicleVel.y = Vel.y;
				VehicleData->VehicleVel.z = Vel.z;

				VehicleData->VehicleAcc.x -= -DeltaVel.x / DeltaTime;
				VehicleData->VehicleAcc.y -= DeltaVel.y / DeltaTime;
				VehicleData->VehicleAcc.z -= DeltaVel.z / DeltaTime;

				if (Len > 30.0)
				{
					vehAnimChangeVehicleStateByName(VehicleData->VehicleMasterPtr, (UCHAR*)"crashexplosion", TRUE);
					persKillCrew();
					VehicleData->FreezeVehicle = 1;
					vehVisDamageHulkVehicle(VehicleData->VehicleMasterPtr, TRUE);
					return 0;
				}
			}
		}
	}

	return 0;
}

UINT32 mlSetSpawnStuff(SINT32 SpawnCellX,SINT32 SpawnCellY,SINT32 SpawnId,UINT32 SpawnNum)
{
	Write_Debug("SetSpawnStuff called, scx = %d, scy = %d, sid = %d, n = %d",
					SpawnCellX, SpawnCellY, SpawnId, SpawnNum);
	MobileSpawn = FALSE;
	SpawnSuperCellX = SpawnCellX;
	SpawnSuperCellY = SpawnCellY;
	SpawnLocId = SpawnId;
	SpawnNumber = SpawnNum;

	return 0;
}

extern UINT32 mlSetMobileSpawnStuff(UINT32 VehicleHostIndex, FLOAT64 x,FLOAT64 y,FLOAT64 z,FLOAT64 yaw)
{
	MobileSpawn = TRUE;
	MobileSpawnVehicle = VehicleHostIndex;
	MobileSpawnLocation.x = x;
	MobileSpawnLocation.y = y;
	MobileSpawnLocation.z = z;
	MobileSpawnYaw = yaw;

	return 0;
}

static UINT32 _ReadPositionFile(mlVehicleData * VehicleData,SINT8 * PositionFileName,SINT8 * DirName)
{
	static UCHAR UseFileName[128];
	static UCHAR Buffer[256];
	SINT32 PositionFile;
	SINT32 Position;
	UCHAR * VarName;
	UCHAR * VarContents;
	mlPositionData * CrewData;
	SINT32 TempNum;

	if (VehicleData == nullptr)
	{
		return 1;
	}

	if (PositionFileName == nullptr)
	{
		return 1;
	}

	if (DirName == nullptr)
	{
		return 1;
	}

	CrewData = &VehicleData->CrewData;

	strcpy((char*)UseFileName,(char*)DirName);
	strcat((char*)UseFileName,"\\");
	strcat((char*)UseFileName,(char*)PositionFileName);

	PositionFile = fioOpen((char*)UseFileName);

	if (PositionFile == -1)
	{
		return 1;
	}

	Position = -1;

	while (fioGetString(PositionFile,(char*)Buffer,255) != 0)
	{
		if (Buffer[0] == '#' ||
			strlen((SCHAR*)Buffer) < 2)
		{
			continue;
		}

		if (strncmp((SCHAR*)Buffer,"groupmsg",8) == 0)
		{
			if (Position == -1)
			{
				Write_Debug("Attempted to specify a weapon grouping message before a position has been specified");
				continue;
			}

			VarName = VarContents = nullptr;
			mlUtilityGetVarnameAndContents1(&VarName,&VarContents,Buffer);

			if (VarContents == nullptr)
			{
				Write_Debug("Weapon grouping message has no parameters");
				continue;
			}

			VarName = nullptr;
			mlUtilityGetVarnameAndContents1(&VarName,&VarContents,VarContents);

			if (VarContents == nullptr)
			{
				Write_Debug("Weapon grouping message has insufficient parameters");
				continue;
			}

			TempNum = atoi((SCHAR*)VarName);

			if (TempNum >= ML_MAX_WEAPON_GROUPINGS)
			{
				Write_Debug("Weapon grouping message has invalid weapon group number");
				continue;
			}

			strcpy(CrewData->SecondaryWeaponMsg[Position][TempNum-1],(SCHAR*)VarContents);

			continue;
		}

		VarName = VarContents = nullptr;
		mlUtilityGetVarnameAndContents(&VarName,&VarContents,Buffer);
		if (VarName == nullptr || VarContents == nullptr)
		{
			continue;
		}

		if (strcmp((SCHAR*)VarName,"position") == 0)
		{
			Position = atoi((SCHAR*)VarContents) - 1;
		}

		if (Position >= 0)
		{
			if (strcmp((SCHAR*)VarName,"hatchclosedsoundvolume") == 0)
			{
				CrewData->HatchClosedSoundVol[Position] = 1.0f - (FLOAT32)atof((SCHAR*)VarContents);
			}else if (strcmp((SCHAR*)VarName,"hatchopensoundvolume") == 0)
			{
				CrewData->HatchOpenSoundVol[Position] = 1.0f - (FLOAT32)atof((SCHAR*)VarContents);
			}else if (strcmp((SCHAR*)VarName,"canopyclosedsoundvolume") == 0)
			{
				CrewData->CanopyClosedSoundVol[Position] = 1.0f - (FLOAT32)atof((SCHAR*)VarContents);
			}else if (strcmp((SCHAR*)VarName,"canopyopensoundvolume") == 0)
			{
				CrewData->CanopyOpenSoundVol[Position] = 1.0f - (FLOAT32)atof((SCHAR*)VarContents);
			}else if (strcmp((SCHAR*)VarName,"engineidlevol") == 0)
			{
				CrewData->EngineIdleVol[Position] = 1.0f - (FLOAT32)atof((SCHAR*)VarContents);
			}else if (strcmp((SCHAR*)VarName,"enginefullpowervol") == 0)
			{
				CrewData->EngineFullPowerVol[Position] = 1.0f - (FLOAT32)atof((SCHAR*)VarContents);
			}else if (strcmp((SCHAR*)VarName,"canopysound") == 0)
			{
				strcpy(CrewData->CanopySound[Position],(SCHAR*)VarContents);
			}else if (strcmp((SCHAR*)VarName,"hatchsound") == 0)
			{
				strcpy(CrewData->HatchSound[Position],(SCHAR*)VarContents);
			}else if (strcmp((SCHAR*)VarName,"canopyclosesound") == 0)
			{
				strcpy(CrewData->CanopyCloseSound[Position],(SCHAR*)VarContents);
			}else if (strcmp((SCHAR*)VarName,"hatchclosesound") == 0)
			{
				strcpy(CrewData->HatchCloseSound[Position],(SCHAR*)VarContents);
			}else if (strcmp((SCHAR*)VarName,"numberofsecondarygroups") == 0)
			{
				CrewData->NumberOfSecondaryWeapons[Position] = atoi((SCHAR*)VarContents) - 1;
			}else if (strcmp((SCHAR*)VarName,"defaultcanopypos") == 0)
			{
				CrewData->DefaultCanopyPosition[Position] = atoi((SCHAR*)VarContents);
			}else if (strcmp((SCHAR*)VarName,"defaulthatchpos") == 0)
			{
				CrewData->DefaultHatchPosition[Position] = atoi((SCHAR*)VarContents);
			}else if (strcmp((SCHAR*)VarName,"exposedbycanopy") == 0)
			{
				CrewData->ExposedByCanopy[Position] = atoi((SCHAR*)VarContents);
			}else if (strcmp((SCHAR*)VarName,"exposedbyhatch") == 0)
			{
				CrewData->ExposedByHatch[Position] = atoi((SCHAR*)VarContents);
			}else if (strcmp((SCHAR*)VarName,"alwaysexposed") == 0)
			{
				CrewData->AlwaysExposed[Position] = atoi((SCHAR*)VarContents);
			}else if (strcmp((SCHAR*)VarName,"bombbaydoor") == 0)
			{
				CrewData->SetBombbayDoor[Position] = true;
				CrewData->BombbayDoorState[Position] = atoi((SCHAR*)VarContents);
			}
		}
	}

	fioClose(PositionFile);

	return 0;
}


#pragma mark -

/************************************************************************************************
	mlVehicleHandleTurretConvergence
*************************************************************************************************/

static SINT32 mlVehicleHandleTurretConvergence(mlVehicleData* VehicleData)
{
	FLOAT32  Range = persGunsightGetRange();
	if(Range < 10.0f) return 0;

	for(int i = 0; i < ML_MAX_CREW_POSITIONS; i++)
	{
		piMATRIX mat;
		OBJECT* FireDirectorObj = VehicleData->VehicleObjectData->FireDirectorObj[i];
		if(FireDirectorObj == nullptr) continue;
		if(FireDirectorObj->dof == nullptr || FireDirectorObj->child == nullptr) break; // should be a DOF object

		// Fire Director position and direction
		mat = IdentityMatrix;
		mat._41 = FireDirectorObj->dof->dofmatrix2._41;		// all this matrix foo copied from code that gets fire position in mlweapons and vehupdate
		mat._42 = FireDirectorObj->dof->dofmatrix2._42;
		mat._43 = FireDirectorObj->dof->dofmatrix2._43;
		math3dMatrixMultiply(&mat,&mat,&FireDirectorObj->matint);

		piVECTOR FireDirectorPos = {mat._41, mat._42, -mat._43};

		mat = FireDirectorObj->dof->dofmatrix2;
		math3dMatrixMultiply(&mat,&mat,&FireDirectorObj->child->matint);
		mat._13 = -mat._13; mat._23 = -mat._23; mat._33 = -mat._33; mat._43 = -mat._43;
		mat._41 = mat._42 = mat._43 = 0.0f;

		piVECTOR  FireDirectorDir 	= {0, 1, 0};
		piVECTOR  FireDirectorRight = {1, 0, 0};
		piVECTOR  FireDirectorUp	= {0, 0, 1};
		math3dVectorMultiply(&FireDirectorDir,  &FireDirectorDir,  	&mat);
		math3dVectorMultiply(&FireDirectorRight,&FireDirectorRight,	&mat);
		math3dVectorMultiply(&FireDirectorUp,	&FireDirectorUp, 	&mat);

		piVECTOR FireDirectorRange = FireDirectorDir;
		FireDirectorRange.x *= Range;
		FireDirectorRange.z *= Range;
		FireDirectorRange.y *= Range;

		// Target
		piVECTOR TargetPoint;
		math3dAddVectors(&TargetPoint, &FireDirectorPos, &FireDirectorRange);

		for(int j = 0; j < VEH_MAX_MOUNTS; j++)
		{
			OBJECT* TurretConvObj = VehicleData->VehicleObjectData->TurretConvObj[i][j];
			if(	TurretConvObj == nullptr || TurretConvObj->dof == nullptr)  break;

			// Turret position and direction (inexplicable matrix foo ahead)
			mat = IdentityMatrix;
			mat._41 = TurretConvObj->dof->dofmatrix2._41;
			mat._42 = TurretConvObj->dof->dofmatrix2._42;
			mat._43 = TurretConvObj->dof->dofmatrix2._43;
			math3dMatrixMultiply(&mat,&mat,&TurretConvObj->matint);

			piVECTOR TurretPos = {mat._41, mat._42, -mat._43};

			piVECTOR TurretRange;
			math3dSubtractVectors(&TurretRange, &TargetPoint, &TurretPos);

			piVECTOR TurretDir = TurretRange;
			math3dNormalize(&TurretDir);

			// Calculate traverse convergence
			piVECTOR FireDirectorTraverseDir = FireDirectorDir; FireDirectorTraverseDir.z = 0; math3dNormalize(&FireDirectorTraverseDir);
			piVECTOR TurretTraverseDir = TurretDir; TurretTraverseDir.z = 0; math3dNormalize(&TurretTraverseDir);
			piVECTOR FireDirectorTraverseRight = FireDirectorRight; FireDirectorTraverseRight.z = 0;

			FLOAT32 convTraverseDotProduct = math3dDotProduct(&TurretTraverseDir, &FireDirectorTraverseDir);
			if(convTraverseDotProduct < -1) 	convTraverseDotProduct = -1;
			else if(convTraverseDotProduct > 1)	convTraverseDotProduct =  1;

			FLOAT32 convTraverseAngle = acos(convTraverseDotProduct);

			piVECTOR TurretTraverseRight;
			math3dSubtractVectors(&TurretTraverseRight, &TurretTraverseDir, &FireDirectorTraverseDir);

			if(math3dDotProduct(&TurretTraverseRight, &FireDirectorTraverseRight) < 0)
				convTraverseAngle = -convTraverseAngle;

			if(VehicleData->VehicleObjectData->TraverseReversed[i])
				convTraverseAngle = -convTraverseAngle;

			// Calculate elevate convergence
			piVECTOR FireDirectorElevateDir;
			// rotate in traverse plane
			if(VehicleData->VehicleObjectData->TraverseReversed[i])
			{
				FireDirectorElevateDir.y = FireDirectorDir.y * cos(convTraverseAngle) + FireDirectorDir.x * sin(convTraverseAngle);
				FireDirectorElevateDir.x = FireDirectorDir.x * cos(convTraverseAngle) - FireDirectorDir.y * sin(convTraverseAngle);
			}else
			{
				FireDirectorElevateDir.y = FireDirectorDir.y * cos(convTraverseAngle) - FireDirectorDir.x * sin(convTraverseAngle);
				FireDirectorElevateDir.x = FireDirectorDir.x * cos(convTraverseAngle) + FireDirectorDir.y * sin(convTraverseAngle);
			}
			FireDirectorElevateDir.z = FireDirectorDir.z;

			piVECTOR TurretElevateDir = TurretDir;
			piVECTOR FireDirectorElevateUp = FireDirectorUp;

			FLOAT32 convElevateDotProduct = math3dDotProduct(&TurretElevateDir, &FireDirectorElevateDir);
			if(convElevateDotProduct < -1) 		convElevateDotProduct = -1;
			else if(convElevateDotProduct > 1)	convElevateDotProduct =  1;

			FLOAT32 convElevateAngle = acos(convElevateDotProduct);

			piVECTOR TurretElevateUp;
			math3dSubtractVectors(&TurretElevateUp, &TurretElevateDir, &FireDirectorElevateDir);

			if(math3dDotProduct(&TurretElevateUp, &FireDirectorElevateUp) < 0)
				convElevateAngle = -convElevateAngle;

#ifdef BETA_RELEASE
			if(TurretConvergenceTraverseOff) { convTraverseAngle = 0; convElevateAngle = 0; }
			if(TurretConvergenceElevateOff) convElevateAngle  = 0;
#endif
			// finally, set the convergence
			mlSetTurretConvergence(i+1, j+1, convTraverseAngle / PI, convElevateAngle /  PI);

// bunch o' debugging
#ifdef BETA_RELEASE
		if(VehicleObjectAnalysis)
		{
			FLOAT64VECTOR p1 = {FireDirectorPos.x, FireDirectorPos.y, FireDirectorPos.z};
			FLOAT64VECTOR p2 = {TargetPoint.x, TargetPoint.y, TargetPoint.z};
			draw3dGizmoLine(&p1, &p2, &VehicleData->VehicleAttMat, &VehicleData->AdjustedForCGPos, colorMAGENTAALPHA);
		}

		if(VehicleObjectAnalysis)
		{
			FLOAT64VECTOR p1 = {TurretPos.x, TurretPos.y, TurretPos.z};
			FLOAT64VECTOR p2 = {TargetPoint.x, TargetPoint.y, TargetPoint.z};
			draw3dGizmoLine(&p1, &p2, &VehicleData->VehicleAttMat, &VehicleData->AdjustedForCGPos, colorREDALPHA);
		}

		if(VehicleObjectAnalysis)
		{
			OBJECT* FirePoint = VehicleData->VehicleObjectData->FirePoint[i][j];
			mat = IdentityMatrix;
			mat._41 = FirePoint->dof->dofmatrix2._41;
			mat._42 = FirePoint->dof->dofmatrix2._42;
			mat._43 = FirePoint->dof->dofmatrix2._43;
			math3dMatrixMultiply(&mat,&mat,&FirePoint->matint);

			FLOAT64VECTOR p1 = {mat._41, mat._42, -mat._43};

			mat = FirePoint->dof->dofmatrix2;
			math3dMatrixMultiply(&mat,&mat,&FirePoint->child->matint);
			mat._13 = -mat._13; mat._23 = -mat._23; mat._33 = -mat._33; mat._43 = -mat._43;
			mat._41 = mat._42 = mat._43 = 0.0f;

			piVECTOR  FirePointDir 	= {0, 1, 0};
			math3dVectorMultiply(&FirePointDir, &FirePointDir, &mat);

			FLOAT64VECTOR p2 = {FirePointDir.x, FirePointDir.y, FirePointDir.z};
			p2.x *= Range; p2.z *= Range; p2.y *= Range;
			mathAddVectors(&p2, &p2, &p1);

			draw3dGizmoLine(&p1, &p2, &VehicleData->VehicleAttMat, &VehicleData->AdjustedForCGPos, colorYELLOWALPHA);
		}

		if(VehicleObjectAnalysis)
		{
			FLOAT64VECTOR p1 = {TurretPos.x, TurretPos.y, TurretPos.z};
			FLOAT64VECTOR p2 = {FireDirectorElevateDir.x, FireDirectorElevateDir.y, FireDirectorElevateDir.z};
			p2.x *= Range; p2.y *= Range; p2.z *= Range;
			mathAddVectors(&p2, &p2, &p1);
			draw3dGizmoLine(&p1, &p2, &VehicleData->VehicleAttMat, &VehicleData->AdjustedForCGPos, colorBLUEALPHA);
		}

#endif // BETA_RELEASE

		}
	}

	return 0;
}

/************************************************************************************************
	mlVehicleUpdateRide
*************************************************************************************************/

SINT32 mlVehicleUpdateRide(mlVehicleData* VehicleData)
{
	vehVehicle* Vehicle = VehicleData->VehicleMasterPtr;

	VehicleData->VehicleTorque.Pitch =  VehicleData->VehicleTorque.Roll = VehicleData->VehicleTorque.Yaw = 0;
	VehicleData->VehicleForce.x = VehicleData->VehicleForce.y = VehicleData->VehicleForce.z = 0;

	if(vehAttachToRidePoint(Vehicle) != 0 && Vehicle->VehicleType->Type != VEH_TYPE_PERSON)
	{
		if(Vehicle->RideSeatNum >= VEH_HOIST_RIDE_SEAT)
			uiChatPrintLine(uiLocString(IDE_LANG_UI, UNHOIST_UI_LOC_ID).c_str(), L"vehicle");

		Vehicle->RideVehicleHostIndex = 0;
		Vehicle->RideSeatNum = 0;
		VehicleData->PhysicsOff = FALSE;
	}
	return 0;
}

/************************************************************************************************
	mlSafeToUnhoist
*************************************************************************************************/

static LOGICAL mlSafeToUnhoist(vehVehicle* Vehicle)
{
	OBJECT* CollideObject = nullptr;
	piVECTOR V1,V2;
 	FLOAT32 Height = 0;
	FLOAT64VECTOR WorldOriginOffset;
	FLOAT64VECTOR Pos;
	piVECTOR CollidePos;
	FLOAT32 RollCoefficient;
	if(Vehicle->ModelData == nullptr) return 0;

	trnGetWorldOriginOffset(&WorldOriginOffset);
	Pos = (Vehicle->ModelData) ? Vehicle->ModelData->AdjustedForCGPos : Vehicle->PosData.RenderPos;

	V1.x = (FLOAT32)(Pos.x - WorldOriginOffset.x);
	V1.y = (FLOAT32)(Pos.y - WorldOriginOffset.y);
	V1.z = (FLOAT32)(Pos.z - WorldOriginOffset.z); V1.z = -V1.z;
	V2 = V1; V2.z += 10000.0; V1.z -= 10000.0;

	trncdCollideWithVehiclesAndTerrain(&V1,&V2,&CollidePos,&CollideObject,nullptr,nullptr,nullptr,FALSE,TRUE, FALSE);

	if(CollideObject)
	{
		FLOAT64VECTOR WorldCollidePos;
		WorldCollidePos.x = CollidePos.x; WorldCollidePos.y = CollidePos.y; WorldCollidePos.z = CollidePos.z;
		mathVectorMultiply64(&WorldCollidePos, &WorldCollidePos, &CollideObject->mat);
		mathAddVectors(&WorldCollidePos, &WorldCollidePos, &WorldOriginOffset); WorldCollidePos.z = -WorldCollidePos.z;

		Height = (FLOAT32)(Pos.z - WorldOriginOffset.z);
		if(Vehicle->VehicleObjectData)
			Height += Vehicle->VehicleObjectData->LowestPoint;

		Height += (FLOAT32)CollidePos.z;
		mlGetObjectBumpinessAndFriction(nullptr, nullptr, nullptr, nullptr, &RollCoefficient, nullptr, CollideObject);
	}

	return  (Height < MAX_UNHOIST_Z && !(RollCoefficient > 198.999f && RollCoefficient < 199.001f));
}

/************************************************************************************************
	mlVehicleToggleHoist
*************************************************************************************************/

SINT32 mlVehicleToggleHoist(mlVehicleData* VehicleData)
{
	vehVehicle* CurrentVehicle;
	vehVehicle* PlayerVehicle = VehicleData->VehicleMasterPtr;

	if(PlayerVehicle->RideVehicleHostIndex > 0 && PlayerVehicle->RideSeatNum >= VEH_HOIST_RIDE_SEAT) // detach from hoist
	{
		// check to make sure it's safe
		if(!mlSafeToUnhoist(VehicleData->VehicleMasterPtr))
		{
			uiChatPrintLine(uiLocString(IDE_LANG_UI, UNHOIST_TOO_HIGH_LOC_ID).c_str());
			return 0;
		}else
		{
			CurrentVehicle = vehFindVehicle(PlayerVehicle->RideVehicleHostIndex);
			uiChatPrintLine(uiLocString(IDE_LANG_UI, UNHOIST_UI_LOC_ID).c_str(), CurrentVehicle ? chatGetWideCallsign(CurrentVehicle->Chat_Handle) : L"vehicle");
			PlayerVehicle->RideVehicleHostIndex = 0;
			PlayerVehicle->RideSeatNum = 0;
			VehicleData->PhysicsOff = FALSE;
		}
	}else // try to attach to hoist
	{
		vehGetVehicleHead(&CurrentVehicle);

		FLOAT64VECTOR AttachPos = {0,0,PlayerVehicle->VehicleObjectData->HighestPoint};
		mathVectorMultiply64(&AttachPos, &AttachPos, &VehicleData->VehicleAttMat);
		mathAddVectors(&AttachPos, &AttachPos, &VehicleData->AdjustedForCGPos);

		FLOAT64VECTOR WorldOriginOffset;
		trnGetWorldOriginOffset(&WorldOriginOffset);

		// find a hoist point in range
		for(;CurrentVehicle; CurrentVehicle = CurrentVehicle->getNext())
		{
			if (CurrentVehicle == PlayerVehicle) continue;
			if (CurrentVehicle->VehicleObjectData == nullptr) continue;
			if (CurrentVehicle->Side != PlayerVehicle->Side) continue;

			OBJECT* Object = CurrentVehicle->VehicleObjectData->HoistObject;
			if (Object != nullptr)
			{
				piVECTOR TempVec;
				TempVec.x = Object->dof->dofmatrix2._41;
				TempVec.y = Object->dof->dofmatrix2._42;
				TempVec.z = Object->dof->dofmatrix2._43;
				math3dVectorMultiply(&TempVec, &TempVec, &Object->mat);

				FLOAT64VECTOR DeltaVec, HoistPos;
				HoistPos.x = TempVec.x + WorldOriginOffset.x;
				HoistPos.y = TempVec.y + WorldOriginOffset.y;
				HoistPos.z = -(TempVec.z + WorldOriginOffset.z);

				mathSubVectors(&DeltaVec, &HoistPos, &AttachPos);
				if (DeltaVec.x * DeltaVec.x + DeltaVec.y * DeltaVec.y < MIN_HOIST_XY_DISTANCE2 &&
					DeltaVec.z * DeltaVec.z < MIN_HOIST_Z_DISTANCE2)
				{
					// do a line collide with the vehicle
					piVECTOR NewPos, OldPos = TempVec;
					NewPos = OldPos; NewPos.z += 10.0;

					if(mlCollideWithVehicle(PlayerVehicle, &OldPos, &NewPos, TRUE))
						break; // got it
				}
			}
		}

		if(CurrentVehicle)
		{
			if(mathExactVectorLength2(&CurrentVehicle->PosData.Vel) > 25.0)
			{
				uiChatPrintLine(uiLocString(IDE_LANG_UI, TOO_FAST_UI_LOC_ID).c_str(), CurrentVehicle ? chatGetWideCallsign(CurrentVehicle->Chat_Handle) : L"????");
			}else
			{
				if(vehFindRidingVehicle(CurrentVehicle->VehicleHostIndex, VEH_HOIST_RIDE_SEAT, VEH_TYPE_GROUND))
				{
					uiChatPrintLine(uiLocString(IDE_LANG_UI, HOIST_POSITION_OCCUPIED_LOC_ID).c_str());
				}else
				{
					PlayerVehicle->RideVehicleHostIndex = CurrentVehicle->VehicleHostIndex;
					PlayerVehicle->RideSeatNum = VEH_HOIST_RIDE_SEAT;
					VehicleData->PhysicsOff = TRUE;
					uiChatPrintLine(uiLocString(IDE_LANG_UI, HOIST_UI_LOC_ID).c_str(), CurrentVehicle ? chatGetWideCallsign(CurrentVehicle->Chat_Handle) : L"????");

					PlayerVehicle->ModelData->TowBarEnabled = FALSE;
					PlayerVehicle->ModelData->PrimaryTriggerDown = FALSE;
					PlayerVehicle->ModelData->PicklePrimary = FALSE;
					PlayerVehicle->ModelData->SecondaryTriggerDown = FALSE;
					PlayerVehicle->ModelData->PickleSecondary = FALSE;
					vehAnimChangeVehicleStateByName(PlayerVehicle, (UCHAR*)"bombbaydoor", FALSE);
				}
			}
		}else
		{
			uiChatPrintLine(uiLocString(IDE_LANG_UI, NO_HOIST_UI_LOC_ID).c_str());
		}
	}

	return 0;
}


/************************************************************************************************
	mlFindShipUnderVehicle
*************************************************************************************************/

static vehVehicle* mlFindShipUnderVehicle(vehVehicle* Vehicle)
{
	FLOAT64VECTOR WorldOffsetOrigin;
	FLOAT64VECTOR Pos;
	piVECTOR OldPos, NewPos;

	trnGetWorldOriginOffset(&WorldOffsetOrigin); WorldOffsetOrigin.z = -WorldOffsetOrigin.z;
	Pos = (Vehicle->ModelData) ? Vehicle->ModelData->AdjustedForCGPos :  Vehicle->PosData.RenderPos;

	OldPos.x = (FLOAT32)(Pos.x - WorldOffsetOrigin.x);
	OldPos.y = (FLOAT32)(Pos.y - WorldOffsetOrigin.y);
	OldPos.z =-(FLOAT32)(Pos.z - WorldOffsetOrigin.z);

	NewPos = OldPos; NewPos.z += 100;
	draw3dLineDebug(&OldPos,&NewPos,colorYELLOWALPHA);

	vehVehicle* CurrentVehicle = nullptr;
	vehGetVehicleHead(&CurrentVehicle);

	for(;CurrentVehicle; CurrentVehicle = CurrentVehicle->getNext())
	{
		if (CurrentVehicle == Vehicle) continue;
		if (CurrentVehicle->VehicleType->Type != VEH_TYPE_SEA) continue;
		if (CurrentVehicle->Side != Vehicle->Side) continue;
		if (CurrentVehicle->VehicleObjectData == nullptr) continue;

		if(mlCollideWithVehicle(CurrentVehicle, &OldPos, &NewPos, FALSE))
			return CurrentVehicle;
	}

	return nullptr;
}

/************************************************************************************************
	mlVehicleRideTransport
*************************************************************************************************/

SINT32 mlVehicleRideTransport(mlVehicleData* VehicleData)
{
	vehVehicle* RideVehicle;
	vehVehicle* PlayerVehicle = VehicleData->VehicleMasterPtr;

	if(	PlayerVehicle->RideVehicleHostIndex == 0 ||
	   (PlayerVehicle->RideVehicleHostIndex > 0 && PlayerVehicle->RideSeatNum < VEH_HOIST_RIDE_SEAT) )
	{
		mlVehicleToggleHoist(VehicleData);
		return 0;
	}

	// OK if we got here then we need to determine if we're gonna try to ride or fall off
	RideVehicle = mlFindShipUnderVehicle(VehicleData->VehicleMasterPtr);

	if(RideVehicle == nullptr)
	{// detach
		mlVehicleToggleHoist(VehicleData);
		return 0;
	}else // OK over ship try to ride it
	{
		FLOAT64VECTOR AttachPos = {0,0,PlayerVehicle->VehicleObjectData->LowestPoint};
		mathVectorMultiply64(&AttachPos, &AttachPos, &VehicleData->VehicleAttMat);
		mathAddVectors(&AttachPos, &AttachPos, &VehicleData->AdjustedForCGPos);

		FLOAT64VECTOR WorldOriginOffset;
		trnGetWorldOriginOffset(&WorldOriginOffset);

		if(RideVehicle->VehicleObjectData == nullptr) return 0;

		for(UINT32 i = 0; i < RideVehicle->VehicleObjectData->NumVehSeats; i++)
		{
			OBJECT* Object = RideVehicle->VehicleObjectData->VehSeats[i];
			if (Object != nullptr)
			{
				piVECTOR TempVec;
				TempVec.x = Object->dof->dofmatrix2._41;
				TempVec.y = Object->dof->dofmatrix2._42;
				TempVec.z = Object->dof->dofmatrix2._43;
				math3dVectorMultiply(&TempVec, &TempVec, &Object->mat);

				FLOAT64VECTOR DeltaVec, SeatPos;
				SeatPos.x = TempVec.x + WorldOriginOffset.x;
				SeatPos.y = TempVec.y + WorldOriginOffset.y;
				SeatPos.z = -(TempVec.z + WorldOriginOffset.z);

				mathSubVectors(&DeltaVec, &SeatPos, &AttachPos);
				if( DeltaVec.x * DeltaVec.x + DeltaVec.y * DeltaVec.y < MIN_RIDE_XY_DISTANCE2 &&
					DeltaVec.z * DeltaVec.z < MIN_RIDE_Z_DISTANCE2)
				{
					if(vehFindRidingVehicle(RideVehicle->VehicleHostIndex, i, VEH_TYPE_GROUND))
					{
						uiChatPrintLine(uiLocString(IDE_LANG_UI, RIDE_POSITION_OCCUPIED_LOC_ID).c_str());
					}else
					{
						PlayerVehicle->RideSeatNum = i;
						break;
					}
				}
			}
		}
	}

	if(PlayerVehicle->RideSeatNum >= VEH_HOIST_RIDE_SEAT)
		uiChatPrintLine(uiLocString(IDE_LANG_UI, NO_RIDE_UI_LOC_ID).c_str());
	else
		uiChatPrintLine(UTF8(uiLocString(IDE_LANG_UI, RIDE_UI_LOC_ID).c_str()), UTF8(chatGetWideCallsign(RideVehicle->Chat_Handle))); // bad

	return 0;
}

/************************************************************************************************
	mlHoistCollisions
*************************************************************************************************/
// prevents hoist from clipping into ship by colliding with hoist collide object

static SINT32 mlHoistCollisions(vehVehicle* Vehicle)
{
	LOGICAL Collided = FALSE;
	vehVehicle* HoistedVehicle;
	FLOAT64VECTOR WorldOffsetOrigin;
	piVECTOR OldPos, NewPos;
	LOGICAL UpdateWinch = FALSE;
	OBJECT* HoistObject;

	trnGetWorldOriginOffset(&WorldOffsetOrigin); WorldOffsetOrigin.z = -WorldOffsetOrigin.z;

	HoistedVehicle = vehFindRidingVehicle(Vehicle->VehicleHostIndex, VEH_HOIST_RIDE_SEAT, VEH_TYPE_GROUND);
	if(HoistedVehicle)
	{
		if(HoistedVehicle->VehicleHostIndex != Vehicle->ModelData->HoistVehicleHostIndex)
		{
			Vehicle->ModelData->HoistCleanCollideState = FALSE;
			Vehicle->ModelData->HoistVehicleHostIndex = HoistedVehicle->VehicleHostIndex;
		}
	}else
	{
		Vehicle->ModelData->HoistCleanCollideState = FALSE;
		Vehicle->ModelData->HoistVehicleHostIndex = 0;
	}

	// programmatic vertical lines
	HoistObject = Vehicle->VehicleObjectData->HoistObject;
	if(Vehicle->VehicleObjectData == nullptr) return 0;

	if (HoistObject)
	{
		OldPos.x = HoistObject->dof->dofmatrix2._41;
		OldPos.y = HoistObject->dof->dofmatrix2._42;
		OldPos.z = HoistObject->dof->dofmatrix2._43;
		math3dVectorMultiply(&OldPos, &OldPos, &HoistObject->mat);
		NewPos = OldPos;

		// line from hoist down
		// if no vehicle on hoist, it tries to collide with a vehicle if it finds one it will
		// automatically start raising the winch
		// if vehicle is on hoist moves line down to the lowest point of the vehicle
		if(HoistedVehicle && HoistedVehicle->VehicleObjectData)
			NewPos.z += HoistedVehicle->VehicleObjectData->HighestPoint - HoistedVehicle->VehicleObjectData->LowestPoint;
		else
			NewPos.z +=10;

#ifdef BETA_RELEASE
		draw3dLineDebug(&OldPos,&NewPos,colorGREENALPHA);
#endif
		if(Vehicle->ModelData->HoistTurretNum > 0)
		{
			Vehicle->ModelData->TurretTraverseRate[Vehicle->ModelData->HoistTurretNum-1] = Vehicle->ModelData->LastHoistRate;

			if(HoistedVehicle == nullptr)
			{
				vehVehicle* CollideVehicle = mlHoistCollideRoundWithVehicles(nullptr, &OldPos, &NewPos);
				if(CollideVehicle)
				{
					if(-OldPos.z < CollideVehicle->PosData.RenderPos.z + CollideVehicle->VehicleObjectData->HighestPoint - WorldOriginOffset.z)
					{
						UpdateWinch = TRUE;
						Vehicle->ModelData->TurretTraverseRate[Vehicle->ModelData->HoistTurretNum-1] = -1;
						Vehicle->ModelData->LastHoistRate = 0;
					}
				}
			}else
			{
				Collided = mlHoistCollideRoundWithWorld(HoistedVehicle, Vehicle->VehicleObjectData->HoistHitBox, &OldPos, &NewPos);
				if(Collided)
				{
					// retract winch
					if(Vehicle->ModelData->HoistTurretNum > 0)
					{
						UpdateWinch = TRUE;
						if(Vehicle->ModelData->LastHoistRate > 0)
						{
							Vehicle->ModelData->TurretTraverseRate[Vehicle->ModelData->HoistTurretNum-1] = -.1f;
							Vehicle->ModelData->LastHoistRate = 0;
						}
					}
				}
			}
		}
	}

	// collide lines of hoist only if hoisting vehicle
	if(!Collided && HoistedVehicle && HoistedVehicle && HoistedVehicle->VehicleObjectData)
	{
		Collided = mlHoistLineCollideWithWorld(Vehicle, Vehicle->VehicleObjectData->NumHoistCollideObjects,
											 Vehicle->VehicleObjectData->HoistCollidePntList, Vehicle->VehicleObjectData->HoistHitBox);

		OBJECT* LineCollideObject = HoistedVehicle->VehicleObjectData->LineCollideObject;
		if(!Collided && LineCollideObject)
		{
			FLOAT64VECTOR Pos = HoistedVehicle->PosData.RenderPos;
			mathSubVectors(&Pos,&Pos,&WorldOffsetOrigin);

			LineCollideObject->flag2 &= ~OBJECT_DONT_UPDATE;
			mlMoveAndRotateObject(LineCollideObject,  &Pos,  &HoistedVehicle->PosData.RenderAtt);
			LineCollideObject->flag2 |= OBJECT_DONT_UPDATE;

			Collided = mlHoistLineCollideWithWorld(HoistedVehicle, HoistedVehicle->VehicleObjectData->NumCollideObjects,
											 	HoistedVehicle->VehicleObjectData->CollidePntList, Vehicle->VehicleObjectData->HoistHitBox);
		}
	}

	if(!Collided && HoistObject)
	{
		// line from hoist up
		NewPos = OldPos; NewPos.z -= 10;
		Collided = mlHoistCollideRoundWithHitmap(Vehicle->VehicleObjectData->HoistHitBox, &OldPos, &NewPos);
#ifdef BETA_RELEASE
		draw3dLineDebug(&OldPos,&NewPos,colorGREENALPHA);
#endif
	}

	if(HoistedVehicle && !Vehicle->ModelData->HoistCleanCollideState)
	{
		if(!Collided)
			Vehicle->ModelData->HoistCleanCollideState = TRUE;

		Collided = FALSE;
	}

	// reset to last position
	if(Collided)
	{
		UINT32 i;
		for (i = 0; i < ML_MAX_CREW_POSITIONS; i++)
		{
			SINT32 HoistTurret = Vehicle->VehicleType->TurretAux[i] - 1;

			if(HoistTurret >= 0 && HoistTurret < ML_MAX_CREW_POSITIONS)
			{
				mlVehicleData* VehicleData = Vehicle->ModelData;
				VehicleData->TurretTraverse[i] = VehicleData->LastTurretTraverse[i];
				VehicleData->TurretElevate[i]  = VehicleData->LastTurretElevate[i];
				VehicleData->TurretTraverseWithStops[i] = VehicleData->LastTurretTraverseWithStops[i];

				if(!UpdateWinch)
				{
					VehicleData->TurretTraverse[HoistTurret] = VehicleData->LastTurretTraverse[HoistTurret];
					VehicleData->TurretElevate[HoistTurret]  = VehicleData->LastTurretElevate[HoistTurret];
					VehicleData->TurretTraverseWithStops[HoistTurret] = VehicleData->LastTurretTraverseWithStops[HoistTurret];
				}
			}
		}
	}

	return 0;
}

/************************************************************************************************
	mlHoistLineCollideWithWorld
*************************************************************************************************/
// big mess copied from line collide crap

static LOGICAL mlHoistLineCollideWithWorld(vehVehicle* Vehicle, UINT32 NumCollideObjects, mlObjectCollidePnt* CollidePntList, OBJECT* Hitmap)
{
	piMATRIX Matrix1, Matrix2;
	piMATRIX ContactPointAttMat,InvContactPointAttMat;
	FLOAT64VECTOR ContactPoint,Pos,WorldOffsetOrigin,TempContact;
	FLOAT64 ContactDeltaLength;
	piVECTOR OldPos, NewPos;

	for(UINT32 i = 0; i < NumCollideObjects; i++)
	{
		OBJECT* Object = CollidePntList[i].CollideObject;
		FLOAT64VECTOR* LastContactPnt = &CollidePntList[i].LastContactPnt;

		FLOAT32 DOFLength = Object->dof->dofneutralval; // neutral comment val is the line length
		if (DOFLength == 0.0)
			DOFLength = 1.0;

		// determine mat and inv mat for DOF contact point
		math3dMatrixMultiply(&ContactPointAttMat, &(Object->dof->dofmatrix2), &(Object->matint));
		ContactPointAttMat._41 = ContactPointAttMat._42 = ContactPointAttMat._43 = 0;
		mlUtilityConvertMatrixToModelReference(&ContactPointAttMat,&ContactPointAttMat);
		if(Vehicle->ModelData)
			math3dMatrixMultiply(&ContactPointAttMat,&ContactPointAttMat,&Vehicle->ModelData->VehicleAttMat);
		else
			math3dMatrixMultiply(&ContactPointAttMat,&ContactPointAttMat,&Vehicle->PosData.AttMat);

		math3dInvert2(&InvContactPointAttMat,&ContactPointAttMat);

		// determine position of DOF contact point
		Matrix1 = IdentityMatrix;
		Matrix1._41 = Object->dof->dofmatrix2._41;
		Matrix1._42 = Object->dof->dofmatrix2._42;
		Matrix1._43 = Object->dof->dofmatrix2._43;
		Matrix2 = Object->matint;
		math3dMatrixMultiply(&Matrix1,&Matrix1,&Matrix2);

		mlUtilityGetModelPositionFromObjectMatrix(&ContactPoint,&Matrix1);
		if(Vehicle->ModelData)
			mathVectorMultiply64(&ContactPoint, &ContactPoint, &Vehicle->ModelData->VehicleAttMat);
		else
			mathVectorMultiply64(&ContactPoint, &ContactPoint, &Vehicle->PosData.AttMat);

		if(Vehicle->ModelData)
			Pos = Vehicle->ModelData->AdjustedForCGPos;
		else
			Pos = Vehicle->PosData.RenderPos;

		trnGetWorldOriginOffset(&WorldOffsetOrigin);
		WorldOffsetOrigin.z = -WorldOffsetOrigin.z;

		mathAddVectors(&ContactPoint, &ContactPoint, &Pos);
		mathSubVectors(&TempContact,&ContactPoint,LastContactPnt);
		ContactDeltaLength = mathExactVectorLength(&TempContact);
		*LastContactPnt = ContactPoint;
		mathSubVectors(&ContactPoint,&ContactPoint,&WorldOffsetOrigin);

		NewPos.x = (FLOAT32)ContactPoint.x;
		NewPos.y = (FLOAT32)ContactPoint.y;
		NewPos.z = (FLOAT32)ContactPoint.z;
		OldPos.x = 0;
		OldPos.y = 0;
		OldPos.z = DOFLength;
		math3dVectorMultiply(&OldPos,&OldPos,&ContactPointAttMat);
		mathAddVectors(&OldPos,&OldPos,&NewPos);
		OldPos.z = -OldPos.z;
		NewPos.z = -NewPos.z;

#ifdef BETA_RELEASE
		draw3dLineDebug(&OldPos,&NewPos,colorGREENALPHA);
#endif
		if(mlHoistCollideRoundWithWorld(Vehicle, Hitmap, &OldPos, &NewPos))
			return TRUE;
	}

	return FALSE;
}

/************************************************************************************************
	mlHoistCollideRoundWithWorld
*************************************************************************************************/

static LOGICAL mlHoistCollideRoundWithWorld(vehVehicle* Vehicle, OBJECT* Hitmap, piVECTOR const* OldPos, piVECTOR const* NewPos)
{
	LOGICAL Collided = FALSE;

	// collide with hoistbox
	Collided = mlHoistCollideRoundWithHitmap(Hitmap, OldPos, NewPos);

	// collide with vehicles
	if(!Collided)
		Collided = (mlHoistCollideRoundWithVehicles(Vehicle, OldPos, NewPos) != nullptr);

	// collide with terrain
	if(!Collided)
		Collided = mlHoistCollideRoundWithTerrain(OldPos, NewPos);

	return Collided;
}

/************************************************************************************************
	mlHoistCollideRoundWithTerrain
*************************************************************************************************/

static LOGICAL mlHoistCollideRoundWithTerrain(piVECTOR const* OldPos, piVECTOR const* NewPos)
{
	vehVehicle* PlayerVehicle = persGetPersonaVehiclePtr();

	//LOGICAL Collided = FALSE;

	FASTCOLLIDE CollisionList[32];
	FASTCOLLIDE* CollisionListPtr = CollisionList;
	UINT32 NumCollisionSlotsLeft = 32;

	trncdCollideWithListOfTerrainObjects1(&CollisionListPtr,  &NumCollisionSlotsLeft, PlayerVehicle->ModelData->TrnCDListObject,
										 PlayerVehicle->ModelData->TrnCDListTOs,  ML_NUM_MED_LIST_ELEMENTS,
										 (piVECTOR*)OldPos,  (piVECTOR*)NewPos, 0);
	if(NumCollisionSlotsLeft != 32)
		return TRUE;

	return trncdCollideWithPolyList((piVECTOR*)OldPos,(piVECTOR*) NewPos, &CollisionList[0].CollidePnt,
									&CollisionList[0].CollideObject, &CollisionList[0].PolyNum,
									PlayerVehicle->ModelData->TerrainAGLObjectList, PlayerVehicle->ModelData->TerrainAGLPolyList,
									PlayerVehicle->ModelData->NumTerrainAGL);

}

/************************************************************************************************
	mlHoistCollideRoundWithHitmap
*************************************************************************************************/

static LOGICAL mlHoistCollideRoundWithHitmap(OBJECT* hitmap, piVECTOR const* OldPos, piVECTOR const* NewPos)
{
	FASTCOLLIDE CollisionList[32];
	FASTCOLLIDE* CollisionListPtr = CollisionList;
	UINT32 NumCollisionSlotsLeft = 32;

	weapCollideWithHitmapObject(&CollisionListPtr, &NumCollisionSlotsLeft,  hitmap,
								nullptr, OldPos, NewPos,  1, 1);

	return (NumCollisionSlotsLeft != 32);
}

/************************************************************************************************
	mlHoistCollideRoundWithVehicles
*************************************************************************************************/

static vehVehicle* mlHoistCollideRoundWithVehicles(vehVehicle* IgnoreVehicle, piVECTOR const* OldPos, piVECTOR const* NewPos)
{
	vehVehicle* CurrentVehicle = nullptr;
	vehVehicle* PlayerVehicle = persGetPersonaVehiclePtr();
	vehGetVehicleHead(&CurrentVehicle);

	for(;CurrentVehicle; CurrentVehicle = CurrentVehicle->getNext())
	{
		if (CurrentVehicle == PlayerVehicle) continue;
		if (CurrentVehicle == IgnoreVehicle) continue;
		if (CurrentVehicle->VehicleType->Type == VEH_TYPE_PERSON) continue;
		if (CurrentVehicle->VehicleObjectData == nullptr) continue;

		if (CurrentVehicle->RideVehicleHostIndex == PlayerVehicle->VehicleHostIndex && CurrentVehicle->RideSeatNum == VEH_HOIST_RIDE_SEAT)
			continue;

		if(mlCollideWithVehicle(CurrentVehicle, OldPos, NewPos, TRUE))
			return CurrentVehicle;
	}

	return nullptr;
}

/************************************************************************************************
	mlCollideWithVehicle
*************************************************************************************************/

static LOGICAL mlCollideWithVehicle(vehVehicle* CurrentVehicle, piVECTOR const* OldPos, piVECTOR const* NewPos, LOGICAL UseHitbox)
{
	FASTCOLLIDE CollisionList[32];
	FASTCOLLIDE* CollisionListPtr = CollisionList;
	UINT32 NumCollisionSlotsLeft = 32;
	piVECTOR		SphereCenter;
	UINT32			Collision;
	FLOAT64VECTOR 	WorldOriginOffset;

	trnGetWorldOriginOffset(&WorldOriginOffset);
	WorldOriginOffset.z = -WorldOriginOffset.z;

	SphereCenter.x =  (FLOAT32)(CurrentVehicle->PosData.RenderPos.x - WorldOriginOffset.x);
	SphereCenter.y =  (FLOAT32)(CurrentVehicle->PosData.RenderPos.y - WorldOriginOffset.y);
	SphereCenter.z = -(FLOAT32)(CurrentVehicle->PosData.RenderPos.z - WorldOriginOffset.z);

	mlUtilityCheckLineSphereCollide(&Collision, &SphereCenter, OldPos, NewPos, CurrentVehicle->VehicleObjectData->Radius);

	if (Collision)
	{
	//	weapUpdateVehicleHitmap(CurrentVehicle);
		weapCollideWithHitmapObject(&CollisionListPtr, &NumCollisionSlotsLeft,
									UseHitbox ? CurrentVehicle->VehicleObjectData->HitBox : CurrentVehicle->VehicleObjectData->CollideExternObject,
									nullptr, OldPos, NewPos, 1,1);
	}

	return (NumCollisionSlotsLeft != 32);
}

#pragma mark -

#ifdef BETA_RELEASE

// now does* all* vehicles
static UINT32 _HandleVehicleAnalysisStuff(mlVehicleData* VehicleData)
{
	FLOAT32 DeltaTime;
	SINT32 i;
	SINT32 VisualStateChange;
	vehVehicle* Vehicle;
	mlObjectData* ObjectData;
	OBJECT* Object;

#if RELEASE_CLIENT != 0
	if(!HasKeyFile) return 0;
#endif

	if(!VehicleObjectAnalysis) return 0;

	if(DoLeakTest)
		_LeakTest();

	for(i = 0; i < NumLeaks; i++)
		draw3dLineDebug(&Leaks[i][0], &Leaks[i][1], colorREDALPHA); // leaks

	if (_FirstTime == 1)
	{
		// initialize stuff

		_CurrentHitmapDisplay = 0;
		_CurrentLOD = 0;

		VisualStateChange = 1;
		_FirstTime = 0;
	}

	// handle state changes
	VisualStateChange = 0;

	if (dipKEYDOWN(buffer,DIK_LCONTROL))
	{
		if (dipKEYDOWN(buffer,DIK_MINUS) && !dipKEYDOWN(lastbuffer,DIK_MINUS))
		{
			_CurrentHitmapDisplay = 0;
			_CurrentHitmapObjectDisplay = 0;
			if (_CurrentLOD > 0)
				_CurrentLOD--;
			VisualStateChange = 1;
		}
		if (dipKEYDOWN(buffer,DIK_EQUALS) && !dipKEYDOWN(lastbuffer,DIK_EQUALS))
		{
			_CurrentHitmapDisplay = 0;
			_CurrentHitmapObjectDisplay = 0;
			if (_CurrentLOD < ML_MAX_LODS - 1)
			{
				if (VehicleData->VehicleObjectData->LODObjects[_CurrentLOD+1] != nullptr)
					_CurrentLOD++;
			}
			VisualStateChange = 1;
		}

		for (i=DIK_1;i<DIK_1 + ML_MAX_DAMAGE_GROUPINGS;i++)
		{
			if (dipKEYDOWN(buffer,i) && !dipKEYDOWN(lastbuffer,i))
			{
				if (_CycleDamageGroup[i - DIK_1] == 0)
				{
					_CycleDamageGroup[i - DIK_1] = 1;
				}
				else
				{
					_CycleDamageGroup[i - DIK_1] = 0;
					_DamageGroupLevel[i - DIK_1] = 0;
				}
			}
		}

		if (dipKEYDOWN(buffer,DIK_LBRACKET) && !dipKEYDOWN(lastbuffer,DIK_LBRACKET))
		{
			_CurrentLOD = 0;
			_CurrentHitmapObjectDisplay = 0;
			if (_CurrentHitmapDisplay > 0)
				_CurrentHitmapDisplay--;
			VisualStateChange = 1;
		}
		if (dipKEYDOWN(buffer,DIK_RBRACKET) && !dipKEYDOWN(lastbuffer,DIK_RBRACKET))
		{
			_CurrentLOD = 0;
			_CurrentHitmapObjectDisplay = 0;
			if (_CurrentHitmapDisplay < 3)
				_CurrentHitmapDisplay++;
			VisualStateChange = 1;
		}
		if (dipKEYDOWN(buffer,DIK_SEMICOLON) && !dipKEYDOWN(lastbuffer,DIK_SEMICOLON))
		{
			_CurrentLOD = 0;
			_CurrentHitmapDisplay = 0;

			if (_CurrentHitmapObjectDisplay > 0)
				_CurrentHitmapObjectDisplay--;
			VisualStateChange = 1;
		}
		if (dipKEYDOWN(buffer,DIK_APOSTROPHE) && !dipKEYDOWN(lastbuffer,DIK_APOSTROPHE))
		{
			_CurrentLOD = 0;
			_CurrentHitmapDisplay = 0;

			if (_CurrentHitmapObjectDisplay < VEH_MAX_DAMAGE_OBJECTS-1)
				_CurrentHitmapObjectDisplay++;
			VisualStateChange = 1;
		}
		if (dipKEYDOWN(buffer,DIK_C) && !dipKEYDOWN(lastbuffer,DIK_C))
		{
			_DisplayContactPoints = !_DisplayContactPoints;
			VisualStateChange = 1;
		}
		if (dipKEYDOWN(buffer,DIK_W) && !dipKEYDOWN(lastbuffer,DIK_W))
		{
			_DisplayWheelContactPoints = !_DisplayWheelContactPoints;
			VisualStateChange = 1;
		}
		if (dipKEYDOWN(buffer,DIK_R) && !dipKEYDOWN(lastbuffer,DIK_R))
		{
			_ExerciseWheelRotations = !_ExerciseWheelRotations;
			VisualStateChange = 1;
		}
		if (dipKEYDOWN(buffer,DIK_S) && !dipKEYDOWN(lastbuffer,DIK_S))
		{
			_ExerciseWheelRotations = !_ExerciseWheelRotations;
			VisualStateChange = 1;
		}
	}


		// adjust visuals if necessary
	if (VisualStateChange != 0)
	{
		vehGetVehicleHead(&Vehicle);

		for(; Vehicle; Vehicle = Vehicle->getNext())
		{
			ObjectData = Vehicle->VehicleObjectData;
			if(ObjectData == nullptr)
				continue;

	//		weapUpdateVehicleHitmap(Vehicle);

			// lod stuff
			// first reset all LOD's visual ranges
			for (i=0;i<ML_MAX_LODS;i++)
			{
				Object = ObjectData->LODObjects[i];
				if (Object != nullptr)
				{
					while (Object->flags & OBJECT_NOTOBJECT)
					{
						Object = Object->child;
						if (Object == nullptr)
							break;
					}
				}

				if (i == _CurrentLOD && _CurrentHitmapDisplay == 0 && _CurrentHitmapObjectDisplay == 0)
				{
					if (Object != nullptr && Object->flag2)
					{
						Object->minlod = 0.0f;
						Object->maxlod = 100000000.0f;
					}
				}
				else
				{
					if (Object != nullptr)
					{
						Object->minlod = 0.0f;
						Object->maxlod = 0.0f;
					}
				}
			}

			Object = ObjectData->LoadoutLODObject;
			if (Object != nullptr)
			{
				while (Object->flags & OBJECT_NOTOBJECT)
				{
					Object = Object->child;
					if (Object == nullptr)
						break;
				}
			}

			if (_CurrentLOD == 0 && _CurrentHitmapDisplay == 0 && _CurrentHitmapObjectDisplay == 0)
			{
				if (Object != nullptr && Object->flag2)
				{
					Object->minlod = 0.0f;
					Object->maxlod = 100000000.0f;
				}
			}
			else
			{
				if (Object != nullptr)
				{
					Object->minlod = 0.0f;
					Object->maxlod = 0.0f;
				}
			}


			// handle hitmap visuals
			if (_CurrentHitmapDisplay == 1 || _CurrentHitmapDisplay == 3)
			{
				if (ObjectData->CollideInternObject != nullptr)
					ogrpShowObjectAndSubtree(ObjectData->CollideInternObject);
			}
			else
			{
				if (ObjectData->CollideExternObject != nullptr)
					ogrpHideObjectAndSubtree(ObjectData->CollideInternObject);
			}
			if (_CurrentHitmapDisplay == 2 || _CurrentHitmapDisplay == 3)
			{
				if (ObjectData->CollideInternObject != nullptr)
					ogrpShowObjectAndSubtree(ObjectData->CollideExternObject);
			}
			else
			{
				if (ObjectData->CollideExternObject != nullptr)
					ogrpHideObjectAndSubtree(ObjectData->CollideExternObject);
			}

			if (_CurrentHitmapObjectDisplay != 0)
			{
				if (ObjectData->CollideInternObject != nullptr)
					ogrpShowObjectAndSubtree(ObjectData->CollideInternObject);
				if (ObjectData->CollideInternObject != nullptr)
					ogrpShowObjectAndSubtree(ObjectData->CollideExternObject);
			}

			// setup color-coding;
			if (ObjectData->CollideInternObject != nullptr)
			{
				if (_CurrentHitmapDisplay == 3)
				{
					_TraverseAndSetupColorCoding(ObjectData->CollideInternObject->child, 0,
													Vehicle, _CurrentHitmapObjectDisplay);
				}
				else
				{
					_TraverseAndSetupColorCoding(ObjectData->CollideInternObject->child, 1,
													Vehicle, _CurrentHitmapObjectDisplay);
				}
			}
			if (ObjectData->CollideExternObject != nullptr)
			{
				_TraverseAndSetupColorCoding(ObjectData->CollideExternObject->child, 1,
												Vehicle, _CurrentHitmapObjectDisplay);

			}
		}
	}

	DeltaTime = (FLOAT32)(frametime - _LastUpdateTime) / 1000.0f;
	_HighlightHitmapObjectNum = _CurrentHitmapObjectDisplay;
	_LastUpdateTime = frametime;

	return 0;
}

static UINT32 _TraverseAndSetupColorCoding(OBJECT * Object,UINT32 UseAlpha,vehVehicle* Vehicle,
										   UINT32 CurrentHitmapObjectDisplay)
{
OBJECT * ObjSib;
OBJECT * ObjAtt;
FLOAT64 Thickness;
UINT32 Color;
UINT32 Red,Green,Blue,Alpha;
UINT32 i;

	if (Object == nullptr)
	{
		return 0;
	}

	ObjSib = Object;

	do
	{
		if (!(ObjSib->flags & OBJECT_NOTOBJECT))
		{
			{
				Thickness = 0.0f;
				vehGetHitmapComponentThickness(&Thickness,Vehicle,ObjSib->trnum);

				if (UseAlpha == 0)
				{
					Alpha = 255;
				}
				else
				{
					Alpha = 128;
				}

				Red = Green = Blue = 0;
				if (Thickness > 0.03f)
				{
					Red = 255;
				}
				else if (Thickness > 0.02f)
				{
					Red = 255;
					Green = 128;
				}
				else if (Thickness > 0.015)
				{
					Red = 255;
					Green = 255;
				}
				else if (Thickness > 0.009)
				{
					Green = 255;
				}
				else if (Thickness > 0.005)
				{
					Green = 255;
					Blue = 255;
				}
				else if (Thickness > 0.002)
				{
					Blue = 255;
				}
				else if (Thickness > 0.000001)
				{
					Blue = 255;
					Red = 255;
				}
				else
				{
					Red = 255;
					Green = 255;
					Blue = 255;
				}
				Color = piRGBA_MAKE(Red,Green,Blue,Alpha);
			}

			if (CurrentHitmapObjectDisplay != 0)
			{
				UseAlpha = 1;

				if (ObjSib->trnum == (SINT32)CurrentHitmapObjectDisplay)
				{
					Color |= piRGBA_MAKE(0,0,0,0xff);
				}
				else
				{
					Color &= piRGBA_MAKE(0xff,0xff,0xff,0);
				}
			}


			ObjAtt = ObjSib;
			do
			{
				ObjAtt->flags &= ~OBJECT_LITE;

				if (CurrentHitmapObjectDisplay != 0)
				{
					ObjAtt->flags |= OBJECT_NOBACKFACE;
				}
				else
				{
					ObjAtt->flags &= ~OBJECT_NOBACKFACE;
				}

				if (UseAlpha == 1)
				{
					ObjAtt->flag2 |= OBJECT_HASALPHA;
				}
				else
				{
					ObjAtt->flag2 &= ~OBJECT_HASALPHA;
				}

				if (ObjAtt->texid != -1)
				{
					piFreeMipTextures(ObjAtt->texid);
					ObjAtt->texid = -1;
				}

				for (i=0;i<ObjAtt->numverts;i++)
				{
					ObjAtt->puverts[i].color = Color;
				}

				ObjAtt = ObjAtt->attached;
			}
			while (ObjAtt != nullptr);
		}

		if (ObjSib->child != nullptr)
		{
			_TraverseAndSetupColorCoding(ObjSib->child,UseAlpha,Vehicle,CurrentHitmapObjectDisplay);
		}

		ObjSib = ObjSib->sibling;
	}
	while (ObjSib != nullptr);



	return 0;
}

/************************************************************************************************
	_LeakTest
*************************************************************************************************/

static void _LeakTest()
{
	FLOAT64VECTOR WorldOriginOffset;
	FASTCOLLIDE CollideList[32], *CollideListPtr;
	UINT32 NumCollisions;
	piVECTOR start, end, att;
	mlVehicleData* VehicleData = persGetPersonaModelDataPtr();
	FLOAT64  x, z, dim, maxdim;
	piMATRIX mat;
	SINT32 size;

	trnGetWorldOriginOffset(&WorldOriginOffset);

	maxdim = -1;

	dim = fabs(VehicleData->VehicleObjectData->BBox1.x - VehicleData->VehicleObjectData->BBox2.x);
	if(dim > maxdim) maxdim = dim;
	dim = fabs(VehicleData->VehicleObjectData->BBox1.y - VehicleData->VehicleObjectData->BBox2.y);
	if(dim > maxdim) maxdim = dim;
	dim = fabs(VehicleData->VehicleObjectData->BBox1.z - VehicleData->VehicleObjectData->BBox2.z);
	if(dim > maxdim) maxdim = dim;
	size = (SINT32)ceil(maxdim / (LeakGridInc / 100.0f));

	att.x = -CurLeakPitch * PI / 180.0f;
	att.y = 0.0f;
	att.z = CurLeakYaw * PI / 180.0f;
	mathEul_ToMatrix(&mat,EulOrdXYZs,&att);

	for(x = -maxdim/2; x <= maxdim/2 && NumLeaks < MAX_LEAKS; x += LeakGridInc/100.0f)
	{
		for(z = -maxdim/2; z <= maxdim/2 && NumLeaks < MAX_LEAKS; z += LeakGridInc/100.0f)
		{
			start.x = (FLOAT32)x; start.y = 30.0f; start.z = (FLOAT32)z;
			end.x = (FLOAT32)x; end.y = 0.0f; end.z = (FLOAT32)z;

			// rotate
			 math3dVectorMultiply(&start, &start, &mat);
			 math3dVectorMultiply(&end, &end, &mat);

			// translate
			start.x += (FLOAT32)(VehicleData->VehiclePos.x - WorldOriginOffset.x);
			start.y += (FLOAT32)(VehicleData->VehiclePos.y - WorldOriginOffset.y);
			start.z += (FLOAT32)(VehicleData->VehiclePos.z - WorldOriginOffset.z);
			end.x += (FLOAT32)(VehicleData->VehiclePos.x - WorldOriginOffset.x);
			end.y += (FLOAT32)(VehicleData->VehiclePos.y - WorldOriginOffset.y);
			end.z += (FLOAT32)(VehicleData->VehiclePos.z - WorldOriginOffset.z);

			// randomly flip z's (sigh)
			start.z = -start.z;
			end.z = -end.z;

			draw3dLineDebug(&start, &end, colorGREENALPHA); // leaks

			memset(CollideList, 0x0, 32 * sizeof(FASTCOLLIDE));
			CollideListPtr = CollideList; NumCollisions = 32;
			weapCollideRoundWithVehicles(&CollideListPtr, &NumCollisions,
											&start, &end, frametime/1000.0f, frametime/1000.0f,
											TRUE, TRUE, &WorldOriginOffset, FALSE, 0);
			NumCollisions = 32 - NumCollisions;

			if(NumCollisions > 0)
			{
				mlUtilitySortCollideList(CollideList,NumCollisions);

				if(CollideList[0].Flags & _AMMOCD_FLAG_INTERNALCOMPONENT)
				{
					uiChatPrintLine("Collided with internal component: %s", CollideList[0].CollideObject->name);
					Leaks[NumLeaks][0] = start;
					Leaks[NumLeaks][1] = end;
					NumLeaks++;
				}
			}
		}
	}

	CurLeakPitch += LeakAngleInc;
	if(CurLeakPitch > LeakPitchEnd)
	{
		CurLeakPitch = LeakPitchStart;
		CurLeakYaw += LeakAngleInc;
	}

	if(NumLeaks == MAX_LEAKS)
	{
		uiChatPrintLine("Max Leak Limit Reached. Leak Test Finished");
		DoLeakTest = FALSE;
	}

	if(CurLeakYaw > LeakYawEnd)
	{
		uiChatPrintLine("Leak Test Finished");
		DoLeakTest = FALSE;
	}
}

#endif

#pragma mark -

// change!
// now returns true if it was able to tell, false if it wasn't
//
LOGICAL mlIsFPSOnWater(mlVehicleData * VehicleData, LOGICAL* onWater)
{
FLOAT64VECTOR WorldOriginOffset;
piVECTOR V1,V2;
FLOAT32 RollCoefficientMult;
OBJECT * Object;
SINT32	InfSurfaceID = 0;
trnCDTerrainObject*	TO;

	if (VehicleData == nullptr)
	{
		// no vehicle data is an error, we can't tell, so return false
		return FALSE;
	}

	if (onWater == nullptr)
		// bad pointer is an error, we can't tell, so return false
		return FALSE;

	TO = nullptr;

	trnGetWorldOriginOffset(&WorldOriginOffset);

	V1.x = (FLOAT32)(VehicleData->VehiclePos.x - WorldOriginOffset.x);
	V1.y = (FLOAT32)(VehicleData->VehiclePos.y - WorldOriginOffset.y);
	V1.z = -(FLOAT32)(VehicleData->VehiclePos.z);

	V2 = V1;
	V2.z += 10000.0f;
	V1.z -= 0.25f;

    trncdIgnoreTreeCollide(TRUE);
    LOGICAL collided = trncdCollideWithVehiclesAndTerrain(&V1,&V2,nullptr,&Object,nullptr,nullptr,&TO,FALSE,TRUE, FALSE);
    trncdIgnoreTreeCollide(FALSE);

    if(collided)
	{
		RollCoefficientMult = 1.0f;

        if(gCustomTerrain)
        {
            BumpAndFrictionData* data = mlGetScenarioBumpinessAndFriction(VehicleData->VehiclePos.x, VehicleData->VehiclePos.y, Object->soundID);

            if(data)
            {
                InfSurfaceID = data->InfSurfaceID;
            }
        }
        else
        {
		    mlGetObjectBumpinessAndFriction(nullptr,nullptr,nullptr,nullptr,&RollCoefficientMult,&InfSurfaceID,Object);
        }

		// just set this here, no reason not to
		fpFPSSetInfantrySurfaceID (InfSurfaceID, TO);

		if (RollCoefficientMult > 198.999f && RollCoefficientMult < 199.001f)
		{
			*onWater = TRUE;
			return TRUE;
		}
	}
	// no terrain collision is an error, we can't tell, so return false
	else
		return FALSE;

	*onWater = FALSE;
	return TRUE;
}

SINT32 mlGetPrimaryPickleSetting()
{
	mlVehicleData * VehicleData = persGetPersonaModelDataPtr();
	return VehicleData->PicklePrimary;
}

SINT32 mlGetSecondaryPickleSetting()
{
	mlVehicleData * VehicleData = persGetPersonaModelDataPtr();

	return VehicleData->PickleSecondary;
}

void mlResetPrimaryPickleSetting()
{
	mlVehicleData * VehicleData = persGetPersonaModelDataPtr();
	VehicleData->PicklePrimary = 0;
}

void mlResetSecondaryPickleSetting()
{
	mlVehicleData * VehicleData = persGetPersonaModelDataPtr();
	VehicleData->PickleSecondary = 0;
}

LOGICAL mlAddToGroupAmmoCount(SINT32 Count,SINT32 Total)
{
	mlVehicleData * VehicleData = persGetPersonaModelDataPtr();

	if (VehicleData->GroupAmmoCount == -1)
	{
		VehicleData->GroupAmmoCount = 0;
		VehicleData->MaxGroupAmmoCount = 0;
	}

	VehicleData->GroupAmmoCount += Count;
	VehicleData->MaxGroupAmmoCount += Total;

	return TRUE;
}

#ifdef GM_RELEASE

// hacked floating eye controller
static UINT32 _DoEyeOperations(mlVehicleData* VehicleData)
{
#if RELEASE_CLIENT
    switch(OverrideSpawner)
    {
    case BENCHANTWERP_OVERRIDE:
    case BENCHRAMAGEN_OVERRIDE:
        {
            return 0;
        }
    }
#endif

	static UINT32 _LastCallTime;
	static FLOAT64 cruiseSpeed = 10.0;
	static FLOAT64VECTOR freeVel;
	static FLOAT64ATT freeAttVel;
	static LOGICAL freeMove;

	vehVehicle* vehicle = VehicleData->VehicleMasterPtr;
	bool trooper = (vehicle->VehicleType->Type == VEH_TYPE_PERSON);

	FLOAT64 DeltaTime, Speed;
	FLOAT64VECTOR Delta;
	FLOAT64ATT Att;
	piVECTOR attVec;

	if (_LastCallTime == 0)
	{
		_LastCallTime = frametime;
		return 0;
	}

	DeltaTime = (frametime - _LastCallTime) / 1000.0;
	_LastCallTime = frametime;

	Delta.x = Delta.y = Delta.z = 0;
	Att.Pitch = Att.Roll = Att.Yaw = 0;

	if(!uiFocused())
	{
		// cruise speeds
		Speed = cruiseSpeed;

#ifdef GR2_BLEND_EDITOR
		if(getEditModel())
			Speed * .5f;
#endif

		if (dipKEYDOWN(buffer,DIK_DELETE) && !dipKEYDOWN(lastbuffer,DIK_DELETE))
		{
			freeMove = !freeMove;
			freeVel.x = freeVel.y = freeVel.z = 0;
			freeAttVel.Pitch = freeAttVel.Roll = freeAttVel.Yaw = 0;
		}
		if (dipKEYDOWN(buffer,DIK_SLASH)) cruiseSpeed += 1.0;
		if (dipKEYDOWN(buffer,DIK_PERIOD))
		{
			cruiseSpeed -= 1.0;
			if(cruiseSpeed < 10.0)  cruiseSpeed = 10.0;
		}
		if (dipKEYDOWN(buffer,DIK_COMMA)) cruiseSpeed = 10.0;
		if (dipKEYDOWN(buffer,DIK_LSHIFT) || dipKEYDOWN(buffer,DIK_RSHIFT)) Speed = 1000.0;
		if (dipKEYDOWN(buffer,DIK_LCONTROL) || dipKEYDOWN(buffer,DIK_RCONTROL)) Speed = 111.0;

		// translation
		if (dipKEYDOWN(buffer,DIK_F)) 	Delta.z += Speed;
		if (dipKEYDOWN(buffer,DIK_V)) 	Delta.z -= Speed;
		if (dipKEYDOWN(buffer,DIK_D)) 	(trooper) ? Delta.x -= Speed: Delta.x += Speed;
		if (dipKEYDOWN(buffer,DIK_A)) 	(trooper) ? Delta.x += Speed: Delta.x -= Speed;
		if (dipKEYDOWN(buffer,DIK_W)) 	(trooper) ? Delta.y -= Speed: Delta.y += Speed;
		if (dipKEYDOWN(buffer,DIK_UP)) 	(trooper) ? Delta.y -= Speed: Delta.y += Speed;
		if (dipKEYDOWN(buffer,DIK_S))	(trooper) ? Delta.y += Speed: Delta.y -= Speed;
		if (dipKEYDOWN(buffer,DIK_DOWN)) (trooper) ? Delta.y += Speed: Delta.y -= Speed;

		FLOAT64 rotation = 145.0f;

#ifdef GR2_BLEND_EDITOR
		if(getEditModel())
			rotation * .5f;
#endif

		if(!freeMove)
		{
			// attitude control
			if (dipKEYDOWN(buffer,DIK_LEFT)) 	Att.Yaw -= mlDEG_TO_RAD(rotation) * DeltaTime;
			if (dipKEYDOWN(buffer,DIK_RIGHT)) 	Att.Yaw += mlDEG_TO_RAD(rotation) * DeltaTime;
			if (dipKEYDOWN(buffer,DIK_Q)) 		Att.Pitch -= mlDEG_TO_RAD(rotation) * DeltaTime;
			if (dipKEYDOWN(buffer,DIK_Z)) 		Att.Pitch += mlDEG_TO_RAD(rotation) * DeltaTime;
			if (dipKEYDOWN(buffer,DIK_A) && (dipKEYDOWN(buffer,DIK_LMENU)))	Att.Roll += mlDEG_TO_RAD(rotation) * DeltaTime;
			if (dipKEYDOWN(buffer,DIK_D) && (dipKEYDOWN(buffer,DIK_LMENU))) Att.Roll -= mlDEG_TO_RAD(rotation) * DeltaTime;

		}else
		{
			if (dipKEYDOWN(buffer,DIK_LEFT)) 	freeAttVel.Yaw -= mlDEG_TO_RAD(rotation) * DeltaTime;
			if (dipKEYDOWN(buffer,DIK_RIGHT)) 	freeAttVel.Yaw += mlDEG_TO_RAD(145.0) * DeltaTime;
			if (dipKEYDOWN(buffer,DIK_Q)) 		freeAttVel.Pitch -= mlDEG_TO_RAD(rotation) * DeltaTime;
			if (dipKEYDOWN(buffer,DIK_Z)) 		freeAttVel.Pitch += mlDEG_TO_RAD(rotation) * DeltaTime;
			if (dipKEYDOWN(buffer,DIK_A) && (dipKEYDOWN(buffer,DIK_LMENU)))	freeAttVel.Roll += mlDEG_TO_RAD(rotation) * DeltaTime;
			if (dipKEYDOWN(buffer,DIK_D) && (dipKEYDOWN(buffer,DIK_LMENU)))	freeAttVel.Roll -= mlDEG_TO_RAD(rotation) * DeltaTime;
		}

		// cool stuff
		if (dipKEYDOWN(buffer,DIK_O) && !dipKEYDOWN(lastbuffer,DIK_O)) mlToggleGear(1,IMAP_MOM_GEARHANDLE,0);
		if (dipKEYDOWN(buffer,DIK_J) && !dipKEYDOWN(lastbuffer,DIK_J)) mlToggleBombBayDoor(1,IMAP_MOM_TOGGLE_DEPLOYWEAPON, 0);
		if (dipKEYDOWN(buffer,DIK_H) && !dipKEYDOWN(lastbuffer,DIK_H)) mlToggleCanopy(1,IMAP_MOM_CANOPYCONTROL,0);
		if (dipKEYDOWN(buffer,DIK_X) && !dipKEYDOWN(lastbuffer,DIK_X)) persToggleInternalExternalViews(1,IMAP_MOM_TOGGLE_INTERNAL_EXTERNAL_VIEWS,1);
	}

	if(!freeMove)
	{
		VehicleData->VehicleRealAtt.Pitch += Att.Pitch;
		VehicleData->VehicleRealAtt.Yaw   += Att.Yaw;
		VehicleData->VehicleRealAtt.Roll   += Att.Roll;
	}else
	{
		VehicleData->VehicleRealAtt.Pitch += freeAttVel.Pitch * DeltaTime;
		VehicleData->VehicleRealAtt.Yaw   += freeAttVel.Yaw * DeltaTime;
		VehicleData->VehicleRealAtt.Roll  += freeAttVel.Roll * DeltaTime;
	}

	if( VehicleData->VehicleRealAtt.Pitch >  PI)	VehicleData->VehicleRealAtt.Pitch = -(PI - fmod(VehicleData->VehicleRealAtt.Pitch, (FLOAT64)PI));
	if( VehicleData->VehicleRealAtt.Pitch < -PI) 	VehicleData->VehicleRealAtt.Pitch =  (PI + fmod(VehicleData->VehicleRealAtt.Pitch, (FLOAT64)PI));

 	if( VehicleData->VehicleRealAtt.Yaw >  PI)		VehicleData->VehicleRealAtt.Yaw = -(PI - fmod(VehicleData->VehicleRealAtt.Yaw, (FLOAT64)PI));
	if( VehicleData->VehicleRealAtt.Yaw < -PI) 		VehicleData->VehicleRealAtt.Yaw =  (PI + fmod(VehicleData->VehicleRealAtt.Yaw, (FLOAT64)PI));
 	if( VehicleData->VehicleRealAtt.Roll >  PI)		VehicleData->VehicleRealAtt.Roll = -(PI - fmod(VehicleData->VehicleRealAtt.Roll, (FLOAT64)PI));
	if( VehicleData->VehicleRealAtt.Roll < -PI) 	VehicleData->VehicleRealAtt.Roll =  (PI + fmod(VehicleData->VehicleRealAtt.Roll, (FLOAT64)PI));

	Att.Roll = Att.Pitch = 0;
	Att.Yaw  = VehicleData->VehicleRealAtt.Yaw;

	mathRotateVectorByAtt(&Delta,&Delta,&Att);

	if(freeMove || *BuzzardFollowVehicle)
	{
		freeVel.x += Delta.x / 10.0f;
		freeVel.y += Delta.y / 10.0f;
		freeVel.z += Delta.z / 10.0f;
		Delta = freeVel;
	}

	Delta.x *= DeltaTime;
	Delta.y *= DeltaTime;
	Delta.z *= DeltaTime;

	mathAddVectors(&VehicleData->VehiclePos,&VehicleData->VehiclePos,&Delta);

	VehicleData->VehicleLastVel = VehicleData->VehicleVel;
	VehicleData->VehicleVel = VehicleData->VehicleRealVel = freeVel;

	// cooler stuff vehicle. following
	if(*BuzzardFollowVehicle && BuzzardFollowDistance > 0)
	{
		vehVehicle* FollowVehicle = nullptr;
		vehGetVehicleHead(&FollowVehicle);
		while(FollowVehicle)
		{
			if(!strcmp((char*)FollowVehicle->Handle, BuzzardFollowVehicle))
				break;

			FollowVehicle = FollowVehicle->getNext();
		}

		if(FollowVehicle)
		{
			FLOAT64VECTOR DeltaVec;
			mathSubVectors(&DeltaVec, &VehicleData->VehiclePos, &FollowVehicle->PosData.RenderPos);
			mathNormalize(&DeltaVec);

			VehicleData->VehiclePos.x = FollowVehicle->PosData.RenderPos.x + BuzzardFollowDistance * DeltaVec.x;
			VehicleData->VehiclePos.y = FollowVehicle->PosData.RenderPos.y + BuzzardFollowDistance * DeltaVec.y;
			VehicleData->VehiclePos.z = FollowVehicle->PosData.RenderPos.z + BuzzardFollowDistance * DeltaVec.z;
			VehicleData->VehicleVel = VehicleData->VehicleRealVel = FollowVehicle->PosData.Vel;
		}else if(!freeMove)
		{
			freeVel.x = freeVel.y = freeVel.z = 0;
		}
	}else if(!freeMove)
	{
		freeVel.x = freeVel.y = freeVel.z = 0;
	}

// Set all of the attitudes and positions in all of the appoximately one billion places
// where this info is stored and used by who knows what and who knows how.
	vehicle->ModelData->RenderPos = VehicleData->VehiclePos;
	vehicle->ModelData->RenderAtt = VehicleData->VehicleRealAtt;

	vehicle->PosData.RenderPos = VehicleData->VehiclePos;
	vehicle->PosData.RenderAtt = VehicleData->VehicleRealAtt;

	vehicle->PosData.Pos = VehicleData->VehiclePos;
	vehicle->PosData.Att = VehicleData->VehicleRealAtt;

	VehicleData->VehicleAtt   = VehicleData->VehicleRealAtt;
	if(vehicle->VehicleType->Type != VEH_TYPE_FLOATER) // usual random sign flips
	{
		VehicleData->VehicleAtt.Pitch = -VehicleData->VehicleAtt.Pitch;
		VehicleData->VehicleAtt.Yaw = -VehicleData->VehicleAtt.Yaw;
	}
	attVec.x = (FLOAT32)VehicleData->VehicleRealAtt.Pitch;
	attVec.y = (FLOAT32)VehicleData->VehicleRealAtt.Roll;
	attVec.z = (FLOAT32)VehicleData->VehicleRealAtt.Yaw;
	mathEul_ToMatrix(&VehicleData->VehicleAttMat,EulOrdXYZs,&attVec);
	math3dInvert2(&VehicleData->VehicleAttInvMat,&VehicleData->VehicleAttMat);

	FLOAT64VECTOR TempVec = VehicleData->VehicleCenterOfMass;
	mathVectorMultiply64(&TempVec,&TempVec,&VehicleData->VehicleAttMat);

	VehicleData->AdjustedForCGPos = VehicleData->VehiclePos;
	VehicleData->AdjustedForCGPos.x -= TempVec.x;
	VehicleData->AdjustedForCGPos.y -= TempVec.y;
	VehicleData->AdjustedForCGPos.z -= TempVec.z;

	return 0;
}

#endif // GM_RELEASE

#pragma mark -

// handles the magical mystery rotational co-ordinate conversion

/*SINT32 mlQuaternionToVehAttMat(ptkQuaternion const* q, FLOAT64ATT* att64, piMATRIX* mat32)
{
	ptkReal mat64[3][3];
	ptkQuaternion o = *q;
	piVECTOR att32;

	o.z = -o.z;
	ptkQuaternionToRotationMatrix(mat64, &o);

	// could we convert back and forth between double and float enough times?
	mat32->m[0][0] = (FLOAT32)mat64[0][0];
	mat32->m[0][1] = (FLOAT32)mat64[0][1];
	mat32->m[0][2] = (FLOAT32)mat64[0][2];
	mat32->m[1][0] = (FLOAT32)mat64[1][0];
	mat32->m[1][1] = (FLOAT32)mat64[1][1];
	mat32->m[1][2] = (FLOAT32)mat64[1][2];
	mat32->m[2][0] = (FLOAT32)mat64[2][0];
	mat32->m[2][1] = (FLOAT32)mat64[2][1];
	mat32->m[2][2] = (FLOAT32)mat64[2][2];

	mathEul_FromMatrix(&att32, mat32, EulOrdXYZs);

	att64->Pitch = -att32.x;
	att64->Roll = -att32.y;
	att64->Yaw = att32.z;

	return 0;
}
// handles the magical mystery rotational co-ordinate conversion

SINT32 mlVehAttToQuaternion(ptkQuaternion* q, FLOAT64ATT const* att64)
{
	piVECTOR att32;
	piMATRIX mat32;
	ptkReal mat64[3][3];

	att32.x = -(FLOAT32)att64->Pitch;
	att32.y = -(FLOAT32)att64->Roll;
	att32.z = (FLOAT32)att64->Yaw;

	mathEul_ToMatrix(&mat32, EulOrdXYZs, &att32);

	mat64[0][0] = mat32.m[0][0];
	mat64[0][1] = mat32.m[0][1];
	mat64[0][2] = mat32.m[0][2];
	mat64[1][0] = mat32.m[1][0];
	mat64[1][1] = mat32.m[1][1];
	mat64[1][2] = mat32.m[1][2];
	mat64[2][0] = mat32.m[2][0];
	mat64[2][1] = mat32.m[2][1];
	mat64[2][2] = mat32.m[2][2];

	ptkRotationMatrixToQuaternion(q, mat64);
	q->z = -q->z;

	return 0;
}

// handles the magical mystery rotational co-ordinate conversion

SINT32 mlObjMatToQuaternion(ptkQuaternion* q, piMATRIX const* mat)
{
  	piVECTOR att32;
	piMATRIX mat32 = *mat;
	ptkReal mat64[3][3];

	mathEul_FromMatrix(&att32, &mat32,EulOrdXYZr);
	mathEul_ToMatrix(&mat32, EulOrdXYZr, &att32);
	mathEul_FromMatrix(&att32, &mat32, EulOrdXYZs);
	mathEul_ToMatrix(&mat32, EulOrdXYZs, &att32);

	mat64[0][0] = mat32.m[0][0];
	mat64[0][1] = mat32.m[0][1];
	mat64[0][2] = mat32.m[0][2];
	mat64[1][0] = mat32.m[1][0];
	mat64[1][1] = mat32.m[1][1];
	mat64[1][2] = mat32.m[1][2];
	mat64[2][0] = mat32.m[2][0];
	mat64[2][1] = mat32.m[2][1];
	mat64[2][2] = mat32.m[2][2];

	ptkRotationMatrixToQuaternion(q, mat64);
	q->z = -q->z;


	return 0;
}

// probably will break all these functions out into a separate file


void mlCalcObjectRotationalInertia(OBJECT* obj, FLOAT64 mass, FLOAT64 I[3][3], FLOAT64VECTOR* cm, FLOAT64* vol)
{
	SINT32* index;
	FLOAT64* verts;
	SINT32 numverts;
	SINT32 numtris;
	SINT32 i;

	if(obj && !(obj->flags & OBJECT_NOTOBJECT))
	{
		numverts = obj->numverts;
		numtris = obj->numtris;

	  	verts = (FLOAT64*)memAlloc(numverts*3*sizeof(FLOAT64));
	  	index = (SINT32*)memAlloc(numtris*3*sizeof(SINT32));

	  	for(i = 0; i <  numverts; i++)
	  	{
	  		verts[3*i] 	 =  obj->puverts[i].x;
	  		verts[3*i+1] =  obj->puverts[i].y;
	  		verts[3*i+2] = -obj->puverts[i].z; // convert the stupid z back to normal co-ordinates
	  	}

	  	for(i = 0; i <  numtris; i++)
	  	{
	  		// and correct handedness...
	  		index[3*i] 	 = obj->ptris[i].v3;
	  		index[3*i+1] = obj->ptris[i].v2;
	  		index[3*i+2] = obj->ptris[i].v1;
	  	}
		ptkRotationalInertiaIndexedBody(mass, numverts, numtris, (ptkReal*)verts, (ptkInt*)index, I, (ptkVector*)cm, vol);

		memFree(verts);
		memFree(index);
	}
}*/


void _HandleContactDamage(vehVehicle* Vehicle)
{
	mlObjectData* ObjectData = Vehicle->VehicleObjectData;
	if(ObjectData == nullptr || Vehicle->ModelData == nullptr) return;

	mlComponentList* compList = Vehicle->ModelData->ComponentList;

	for(UINT32 i = 0; i < ObjectData->NumCollideObjects; i++)
	{
		mlObjectCollidePnt* collidePnt = &ObjectData->CollidePntList[i];
		if(!*collidePnt->VisDamage) continue;

		if(mathExactVectorLength2(&collidePnt->Force) > 1.0)
		{
			for(UINT32 j = 0; j < compList->NumComponents;  j++)
			{
				mlComponent* comp = &compList->Components[j];
				if(!*comp->VisDamage[0] || comp->ComponentStatus != ML_STATUS_OK) continue;

				if(strcmp(collidePnt->VisDamage, comp->VisDamage[0]) != 0) continue;

				if(comp->Type == ML_COMPONENT_ENGINE)
				{
					mlEngineComponentData* EngineComponentData = (mlEngineComponentData*)comp->SpecialData;
					if(EngineComponentData->SubType == ML_ENGINE_SUBTYPE_PROPELLOR)
					{
						vehApplyDamageToVehicleComponent(Vehicle,
									  					comp->DamageComponentNum[0],
									  					comp->DamageLevelFail+1000, 0);
//						vehVisDamageGroupDestroyed(Vehicle, collidePnt->VisDamage, TRUE, TRUE);
					}
				}
			}
		}
	}
}

/************************************************************************************************
	mlAutopilotWithinLimits
*************************************************************************************************/

LOGICAL mlAutopilotWithinLimits(mlVehicleData* VehicleData)
{
	LOGICAL withinLimits = FALSE;

	FLOAT64VECTOR dir = {0,1,0};
	mathVectorMultiply64(&dir,&dir,&VehicleData->VehicleAttMat);

	FLOAT64 yacc = mathVectDotProduct(&dir, &VehicleData->VehicleAcc);
	FLOAT64VECTOR yaccvec;
	mathScaleVector(&yaccvec, &dir, yacc);

	FLOAT64VECTOR xzaccvec;
	mathSubVectors(&xzaccvec, &VehicleData->VehicleAcc, &yaccvec);

	FLOAT64 acc2 = mathExactVectorLength2(&xzaccvec);
	FLOAT64 attacc2 = mathExactVectorLength2((FLOAT64VECTOR*)&VehicleData->VehicleAttAcc);

	if(	acc2 < AUTOPILOT_MAX_ACC2 && attacc2 < AUTOPILOT_MAX_ATTACC2 &&
	 	VehicleData->VehicleVel.z*VehicleData->VehicleVel.z 		< AUTOPILOT_MAX_VELZ2 &&
		VehicleData->VehicleAtt.Roll*VehicleData->VehicleAtt.Roll 	< AUTOPILOT_MAX_ROLL2	)
	{
		withinLimits = TRUE;
	}

	return withinLimits;
}

/************************************************************************************************
	mlHitchVehicle
*************************************************************************************************/

void mlHitchVehicle(mlVehicleData* VehicleData)
{
	if(!VehicleData->TowBarEnabled)
	{
		if (VehicleData->BombBayDoor)
		{
			rnmainListLocText(IDE_LANG_UI, DEPLOY_FOR_TOWING_ID);
		}
		else // hitch
		{
			mlComponent* Component = nullptr;
			mlComponentListIterator it;
			mlComponentListBegin(VehicleData->ComponentList, &it);
			for(; it.comp; mlComponentListNext(&it))
			{
				if(it.comp->Type == ML_COMPONENT_TOWPOINT)
				{
					Component = it.comp;
					break;
				}
			}

			if(Component == nullptr) return;

			FLOAT64VECTOR HitchPos,AttachPos;
			FLOAT64VECTOR DeltaVec;
			piVECTOR TempVec;
			OBJECT * TowTarget;
			OBJECT * Object;
			vehVehicle* CurrentVehicle;
			FLOAT64VECTOR WorldOriginOffset;
			UINT32 OkToContinue;
			DeltaVec.x = DeltaVec.y = DeltaVec.z = 0.0f;
			trnGetWorldOriginOffset(&WorldOriginOffset);

			mathVectorMultiply64(&AttachPos,&Component->CurrentRelativePos,&VehicleData->VehicleAttMat);
			mathAddVectors(&AttachPos,&AttachPos,&VehicleData->AdjustedForCGPos);

			// first find a vehicle within range
			TowTarget = nullptr;
			vehGetVehicleHead(&CurrentVehicle);

			for(;CurrentVehicle; CurrentVehicle = CurrentVehicle->getNext())
			{
				if (CurrentVehicle == VehicleData->VehicleMasterPtr) continue;
				if (CurrentVehicle->VehicleObjectData == nullptr) continue;
				if (CurrentVehicle->Side != VehicleData->VehicleMasterPtr->Side)  continue;

				mathSubVectors(&DeltaVec,&CurrentVehicle->PosData.Pos,&AttachPos);
				if (mathExactVectorLength(&DeltaVec) <  MIN_VEHICLE_TOW_DISTANCE + CurrentVehicle->VehicleObjectData->Radius)
				{
					// next, determine if vehicle has a tow point
					// if so, determine if tow point in range
					Object = CurrentVehicle->VehicleObjectData->TowObject;

					OkToContinue = 0;
					if (Object != nullptr)
					{
						if (Object->dof->dofvarnum == ML_DOF_TOWPOINT_LIGHT &&
							Component->TowType == ML_TOWTYPE_LIGHT)
						{
							OkToContinue = 1;
						}

						if (Object->dof->dofvarnum == ML_DOF_TOWPOINT_MEDIUM &&
							(Component->TowType == ML_TOWTYPE_MEDIUM ||
							 Component->TowType == ML_TOWTYPE_LIGHT))
						{
							OkToContinue = 1;
						}

						if (Object->dof->dofvarnum == ML_DOF_TOWPOINT_HEAVY &&
							(Component->TowType == ML_TOWTYPE_LIGHT ||
							 Component->TowType == ML_TOWTYPE_MEDIUM ||
							 Component->TowType == ML_TOWTYPE_HEAVY))
						{
							OkToContinue = 1;
						}

						if (OkToContinue == 0) continue;

						if (CurrentVehicle->VehicleObjectData->TowObject != nullptr)
						{
							TempVec.x = Object->dof->dofmatrix2._41;
							TempVec.y = Object->dof->dofmatrix2._42;
							TempVec.z = Object->dof->dofmatrix2._43;
							math3dVectorMultiply(&TempVec,&TempVec,&Object->mat);

							HitchPos.x = TempVec.x + WorldOriginOffset.x;
							HitchPos.y = TempVec.y + WorldOriginOffset.y;
							HitchPos.z = -(TempVec.z + WorldOriginOffset.z);

							mathSubVectors(&DeltaVec,&HitchPos,&AttachPos);

							if (mathExactVectorLength(&DeltaVec) < MIN_TOW_DISTANCE)
							{
								TowTarget = Object;
								break;
							}
						}
					}
				}
			}

			if(TowTarget)
			{
				VehicleData->TowBarEnabled = TRUE;
				rnmainListLocText(IDE_LANG_UI, TOW_HITCHED_LOC_ID);

				VehicleData->VehicleMasterPtr->RideVehicleHostIndex = CurrentVehicle->VehicleHostIndex;
				VehicleData->VehicleMasterPtr->RideSeatNum = VEH_TOWED_VEHICLE;
			}else
			{
				rnmainListLocText(IDE_LANG_UI, TOW_NOHITCHINRANGE_LOC_ID);
			}
		}
	}
	else // unhitch
	{
		if(VehicleData->VehicleMasterPtr->RideVehicleHostIndex > 0)
		{
			rnmainListLocText(IDE_LANG_UI, TOW_UNHITCHED_LOC_ID);
			VehicleData->VehicleVel.x = VehicleData->VehicleVel.y = VehicleData->VehicleVel.z = 0;
			VehicleData->VehicleLastVel.x = VehicleData->VehicleLastVel.y = VehicleData->VehicleLastVel.z = 0;
			VehicleData->VehicleRealVel.x = VehicleData->VehicleRealVel.y = VehicleData->VehicleRealVel.z = 0;
		}

		VehicleData->TowBarEnabled = FALSE;
		VehicleData->VehicleMasterPtr->RideVehicleHostIndex = 0;
		VehicleData->VehicleMasterPtr->RideSeatNum = 0;
	}
}

static void _DeclareVehicleKilled (mlVehicleData* pVehicleData, mlComponent* pComponent)
{
	if (pVehicleData->DeclaredKilled)
		return;

	//pVehicleData->DeclaredKilled = 1;
	pVehicleData->DeclaredKilled = vehmainDetermineComponentKiller(pComponent);//vehmainHandleVehicleKilled(pComponent);
	pVehicleData->componentDeath = pComponent;

	//vehmainHandleVehicleKilled(pComponent);
}

static void _CheckForSecondaryVehicleKill (mlVehicleData* pVehicleData, mlComponent* pComponent)
{
	// quick exit
	if (pComponent->CriticalSecondary == 0)
		return;

	if (pVehicleData->DeclaredKilled)
		return;

	if (pVehicleData->TotalSecondaryCriticalComponentCount[pComponent->CriticalSecondary-1] > 0)
	{
		if (pVehicleData->SecondaryCriticalComponentCount[pComponent->CriticalSecondary-1] >=
			pVehicleData->TotalSecondaryCriticalComponentCount[pComponent->CriticalSecondary-1])
		{
#ifdef BETA_RELEASE
			uiChatPrintLine("Declaring vehicle killed by secondary component %d", pComponent->CriticalSecondary);
#endif
			_DeclareVehicleKilled (pVehicleData, pComponent);
		}
	}

}

void mlSetStickXPos(FLOAT64 pos)
{
	_StickXPos = pos;
}
void mlSetStickYPos(FLOAT64 pos)
{
	_StickYPos = pos;
}

void mlSetRudderPosition(FLOAT64 pos)
{
	_RudderPos = pos;
}

void mlSetThrottlePosition(FLOAT64 pos)
{
	_ThrottlePos = pos;
	if (_ThrottlePos < 0.0)
		_ThrottlePos = 0.0;
}

void mlSetReportRangeAndTime(FLOAT32 range, SINT32 time)
{
	_ReportRange = (FLOAT32)floor(range / 50.0 + 0.5f) * 50.0f;;
	_ReportTime = time;
}

