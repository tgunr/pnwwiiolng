#pragma once
//	Copyright (C) PlayNet Inc, 2001.
//	Vehicle module.  Handles all vehicles for the front end, sets up
//	callback functions for vehicle "modification", etc
//
//	Originating Programmer: Jonathan Hoof

#include "ml/mlgauges.h"
#include "ml/mltypes.h"
#include "ml/mlobject.h"
#include "comm/commTypes.h"
#include "weap/weapons.h"
#include "veh/vehIcon.h"
#include "veh/vehtype.h"
#include "wwii/wwiiTypes.h" 			// For REL_OBJ_ID
#include "updt/updtTypes.h" 			// For turret_t
#include "include/mobileSpawnEnums.h"	// For MSPAWN_RESULT
#include "draw/sneaky.h"

#include "physics/physicsMain.h"
#include "veh/vehInterpolator.h"
/************************************************************************************************
	vehmain
*************************************************************************************************/

#define MAX_POSSIBLE_VIS_VEHICLES 256

#define VEH_HOIST_RIDE_SEAT 128
#define VEH_TROOPER_ON_DECK_RIDE_SEAT 255
#define VEH_TOWED_VEHICLE 254

// vehicle flags
#define VEH_FLAGS_USERCONTROLLED		0x1 // is the vehicle controlled on the player's FE
#define VEH_FLAGS_USEROWNED			0x2 // is the vehicle owned by the player
#define VEH_FLAGS_UPDATESYSTEM_CONTROLLED 0x4 // the vehicle is updated by the update system
#define VEH_FLAGS_IS_CLONE 0x8
#define VEH_FLAGS_WET (1<<25)
#define VEH_FLAGS_SUBMERGED (1<<26)
#define VEH_FLAGS_AI_HIT (1<<27)
#define VEH_FLAGS_USE_OLD_INTERPOLATOR (1<<28)
#define VEH_FLAGS_HULKED (1<<29)
#define VEH_FLAGS_CACHED (1<<30)
#define VEH_FLAGS_ODE_CONTROLLED (1<<31)

#define VEH_MAX_VEHICLE_POOL 128
#define VEHANIM_DATA_MAX_SIZE 512
#define VEH_MAX_FIREPOINTS 8
#define VEH_MAX_BUILDINGS_HIT 16
#define VEH_MAX_DEBRIS_POINTS 16
#define VEH_MAX_TREAD_DUST  8

#define GENERIC_RAT_RANK	(1+VEHICON_RAT2)
#define KILLER_RANK			(1+VEHICON_KILLER)
#define THUNDER_RANK		(1+VEHICON_THUNDER)
#define MARTINI_RANK		(1+VEHICON_MARTINI)
#define TOTO_RANK			(1+VEHICON_TOTO)
#define FT_RANK				(1+VEHICON_FT)
#define SNAIL_RANK			(1+VEHICON_SNAIL)
#define LINDIR_RANK			(1+VEHICON_LINDIR)
#define FLUTH_RANK			(1+VEHICON_KICKME)

enum DEPLOYEDSTATE
{
	IDE_DEPLOYED_NOT_USED,
	IDE_VEHICLE_UNDEPLOYED,
	IDE_VEHICLE_DEPLOYING,
	IDE_VEHICLE_DEPLOYED,
	IDE_VEHICLE_UNDEPLOYING,
	NUMBER_OF_DEPLOYED_STATES

};

class VisualDamageBody;
class BailoutSTO;
struct PlayerPlacedObject;

struct vehanimVehicleStateAnimations
{
	SINT32 LastUpdateTime;

	UINT32 GearSetting;
	UINT32 FlapSetting;
	UINT32 CanopySetting[ML_MAX_CREW_POSITIONS];
	UINT32 HatchSetting[ML_MAX_CREW_POSITIONS];
	UINT32 BombBayDoorSetting;
	UINT32 DiveBrakeSetting;

	UINT32 GearMoveTime;
	UINT32 FlapMoveTime;
	UINT32 CanopyMoveTime[ML_MAX_CREW_POSITIONS];
	UINT32 HatchMoveTime[ML_MAX_CREW_POSITIONS];
	UINT32 BombBayDoorMoveTime;
	UINT32 DiveBrakeMoveTime;

	LOGICAL PropIsDiscSetting[ML_MAX_ENGINES];
	LOGICAL PropIsDisc[ML_MAX_ENGINES];

	SINT32 RecoilStartTime[VEH_MAX_MOUNTS];

	FLOAT32 RecoilPosition[VEH_MAX_MOUNTS];
	FLOAT32 RecoilTime[VEH_MAX_MOUNTS];

	FLOAT32 GearPos;
	FLOAT32 FlapPos;
	FLOAT32 CanopyPos[ML_MAX_CREW_POSITIONS];
	FLOAT32 HatchPos[ML_MAX_CREW_POSITIONS];
	FLOAT32 BombBayDoorPos;
	FLOAT32 DiveBrakePos;

	FLOAT32 CanopyDOFVar[ML_MAX_CREW_POSITIONS];
	FLOAT32 HatchDOFVar[ML_MAX_CREW_POSITIONS];
	FLOAT32 GearDOFVar;
	FLOAT32 FlapDOFVar;
	FLOAT32 BombBayDoorDOFVar;
	FLOAT32 DiveBrakeDOFVar;

	FLOAT32 PropDiscSpinRpm[ML_MAX_ENGINES];
	FLOAT32 PropSpinPosDOF[ML_MAX_ENGINES];
	UINT32 ExhaustSmokeHandle[ML_MAX_ENGINES][2];
	UINT32 OilLeakSmokeHandle[ML_MAX_ENGINES];
	UINT32 FuelLeakSmokeHandle[ML_MAX_FUELTANKS];
	UINT32 GlycolLeakSmokeHandle[ML_MAX_ENGINES][ML_MAX_RADIATORS_PER_ENGINE];
	UINT32 EngineFireHandle[ML_MAX_ENGINES];
	UINT32 FuelFireHandle[ML_MAX_FUELTANKS];
	UINT32 FuelFireSmokeHandle[ML_MAX_FUELTANKS];
	UINT32 FuelFire2Handle[ML_MAX_FUELTANKS];
	UINT32 WingtipVortexSmokeHandle[2];
	UINT32 TreadDustHandle[VEH_MAX_TREAD_DUST];

	SINT32 ExhaustSmokeStartTime; // hack to tie 3rd person exaust to enginesound
	UINT32 ExhaustSmokeOn[ML_MAX_ENGINES];
	UINT32 OilLeakSmokeOn[ML_MAX_ENGINES];
	UINT32 FuelLeakSmokeOn[ML_MAX_FUELTANKS];
	UINT32 GlycolLeakSmokeOn[ML_MAX_ENGINES][ML_MAX_RADIATORS_PER_ENGINE];
	UINT32 EngineFireOn[ML_MAX_ENGINES];
	UINT32 FuelFireOn[ML_MAX_FUELTANKS];
	UINT32 WingtipVortexSmokeOn[2];
	UINT32 DebrisOn[VEH_MAX_DEBRIS_POINTS];
	UINT32 ExplosionOn[VEH_MAX_DEBRIS_POINTS];
	UINT32 TreadDustOn[VEH_MAX_TREAD_DUST];
    UINT32 WaterEffectOn[VEH_MAX_TREAD_DUST];

    OBJECT* ExhaustSmokeEmitter[ML_MAX_ENGINES][2];
	OBJECT* OilLeakSmokeEmitter[ML_MAX_ENGINES];
	OBJECT* FuelLeakSmokeEmitter[ML_MAX_FUELTANKS];
	OBJECT* GlycolLeakSmokeEmitter[ML_MAX_ENGINES][ML_MAX_RADIATORS_PER_ENGINE];
	OBJECT* EngineFireEmitter[ML_MAX_ENGINES];
	OBJECT* FuelFireEmitter[ML_MAX_FUELTANKS];
	OBJECT* WingtipVortexEmitter[2];
	OBJECT* DebrisEmitter[VEH_MAX_DEBRIS_POINTS];
	OBJECT* TreadDustEmitter[VEH_MAX_TREAD_DUST];

	// wheel rolling stuff
	UINT32 UseCalculatedTrackAndWheel;
	FLOAT32 TrackPos[2]; // left and right track animation position
	FLOAT32 WheelPos[ML_MAX_GEARNUM]; // whee position
	FLOAT32 VehicleSpeed; // vehicle wheel speed

	// control surface stuff
	FLOAT32 AileronDeflection;
	FLOAT32 ElevatorDeflection;
	FLOAT32 RudderDeflection;
	FLOAT32 LastAileronDeflection;
	FLOAT32 LastElevatorDeflection;
	FLOAT32 LastRudderDeflection;

	// hack variables
	FLOAT32 TurretElev[ML_MAX_CREW_POSITIONS];
	FLOAT32 TurretTraverse[ML_MAX_CREW_POSITIONS];
	FLOAT32 LastTurretElev[ML_MAX_CREW_POSITIONS];
	FLOAT32 LastTurretTraverse[ML_MAX_CREW_POSITIONS];
	SINT32 LastElevUpdateTime[ML_MAX_CREW_POSITIONS];
	SINT32 LastTravUpdateTime[ML_MAX_CREW_POSITIONS];

	turret_t LastSendTurretElev[ML_MAX_CREW_POSITIONS];
	turret_t LastSendTurretTrav[ML_MAX_CREW_POSITIONS];

	// sound stuff
	SINT32 EngineSoundId[ML_MAX_ENGINES][4];
	SINT32 EngineSpeedRange;
	SINT32 LastSpeedRangeChangeTime;
	SINT32 MountSoundId[VEH_MAX_MOUNTS];
	LOGICAL MountSoundPlaying[VEH_MAX_MOUNTS];
	SINT32 MountSoundSemaphore[VEH_MAX_MOUNTS];
	SINT32 StukaSoundId;

	LOGICAL TankOnFire;

	bool	bVehicleDestroyed : 1;
	bool	FirstUpdateProcessed : 1;

	SINT32 TankOnFireStartTime;
	SINT32 TankBurnFlareupTime;
	OBJECT * TankFireEmitterPoint[VEH_MAX_FIREPOINTS];
	SINT32 TankFireSmokeEmitter[VEH_MAX_FIREPOINTS];

	SINT32 PlayCrash;
	SINT32 CrashHasPlayed;

	SINT32 EjectRiders;

	SINT32 MainGunLastUpdateTime;

	DEPLOYEDSTATE enumDeployedState;

	FLOAT32 LastAnimationVehicleSpeed;
	FLOAT32 LastTankFireDeltaTime;
	FLOAT32 LastMainGunSmokeLevel;
	FLOAT32 MainGunSmokeLevel;
	FLOAT32 CockpitSmokeLevel;
};

struct vehVehiclePositionData
{
	FLOAT64VECTOR RenderPos;
	FLOAT64ATT RenderAtt;

	FLOAT64VECTOR LastUpdatePos;
	FLOAT64ATT LastUpdateAtt;
	FLOAT64VECTOR LastUpdateVel;
	FLOAT64VECTOR LastLastUpdateVel;
	FLOAT64VECTOR LastUpdateAcc;
	FLOAT64ATT LastUpdateAttVel;

	FLOAT64VECTOR Vel;
	FLOAT64VECTOR Acc;

	FLOAT64ATT Att;
	FLOAT64ATT AttVel;
	piMATRIX AttMat;

	FLOAT64VECTOR VelSum;
	FLOAT64VECTOR AccSum;
	FLOAT64ATT AttVelSum;

// walking on vehicle interpolationg
	FLOAT64VECTOR LastRelPos;
	FLOAT64VECTOR CurRelPos;
	FLOAT64 LastRelYaw;
	FLOAT64 CurRelYaw;

	SINT32 LastRelUpdateTime;
	SINT32 CurRelUpdateTime;
	SINT32 NumSamples;

	SINT32 LastUpdatePredictorUpdateTime;
	SINT32 CumulativeDelta;

	SINT32 LastSenderUpdateTime;
	SINT32 CurrentSenderUpdateTime;
	SINT32 LastAnyUpdateTime;

	SINT32 LastUpdateTime;
	SINT32 CurrentUpdateTime;

	SINT32 LastPredictorUpdateTime;

	SINT32 LastFireTime;

	SINT32 LastDelta;

	LOGICAL States[VEH_MAX_STATE_BITS];
	LOGICAL LastStates[VEH_MAX_STATE_BITS];

	UINT32 UpdateMult;

	SINT32 LastReceivedTime;
	SINT32 LastReceivedDelta;

	UINT32 animationFlags;

	FLOAT64VECTOR Pos;
};


/************************************************************************************************
	vehDamageQue
*************************************************************************************************/

struct vehDamageQue
{
	UINT32 LastSendTime;
	UINT32 FirstRoundTimer;
	UINT32 LastCheckTime;
	UINT32 SendMore;

// Vehicle damage
	LOGICAL	isImportant		[VEH_MAX_DAMAGE_OBJECTS+1];
	UINT32	NumRoundsHit	[VEH_MAX_DAMAGE_OBJECTS+1];
	UINT32	Counter			[VEH_MAX_DAMAGE_OBJECTS+1];	// used for combining 2 hits on same check into 1

// Building damage
	SINT32 SuperCellX	[VEH_MAX_BUILDINGS_HIT];
	SINT32 SuperCellY	[VEH_MAX_BUILDINGS_HIT];
	UINT32 Id			[VEH_MAX_BUILDINGS_HIT];
	UINT32 Count;

	FLOAT64	DamageAccrued	[VEH_MAX_DAMAGE_OBJECTS+1];	// Vehicle Damage
	FLOAT64	Damage		[VEH_MAX_BUILDINGS_HIT];		// Building Damage
};

/************************************************************************************************
	vehVehicle
*************************************************************************************************/
struct vehVehicle
{
	weapWeapon Weapons			[VEH_MAX_MOUNTS][VEH_MAX_WEAPONS_PER_MOUNT];
	vehInterpolator interpolator;

	SINT32	VehicleNum; 		// used by the host to refer to a particular vehicle (on host it is "ident")
	UINT32	VehicleHostIndex; 	// used for updates sent to host
	SINT32	persVehicleIndex; 	// used by the host to refer to the vehicle also
	vehType*	VehicleType; 	// pointer to the vehicle type (needs to be const)
	WEAPON_SERIAL WeaponSerial;	// vehicle's host weapon serial

	UINT32 Flags;
	mlVehicleData* 	ModelData; 			// "flight" modelling data
	mlObjectData* 	VehicleObjectData; 	// 3d object data

	VisualDamageBody* ChunksOfCrap;
	AltPointer<struct vehVehicle> xNext;
// FPS and vehicle riding
	UINT32	RideSeatNum; // special values VEH_HOIST_RIDE_SEAT, VEH_TROOPER_ON_DECK_RIDE_SEAT, VEH_TOWED_VEHICLE
	UINT32	RideVehicleHostIndex;
	UINT32	LastRideVehicleHostIndex;
	UINT32	InfantryTarget;
	FLOAT64VECTOR RelRidePosition; // for troopers walking on deck
	FLOAT64 RelRideYaw;

// Update
	SINT32 	VehicleHasBeenHit;
	SINT32 	VehicleInitTime;
	SINT32 	VehicleDeadTime;
	SINT32 	MinVehicleLOD;
	SINT32 	HideVehicle;
	SINT32 	BumpedOff;
	UINT32 	AnimationBits;
	UINT32 	LoadVehicle;
	SINT32 	LastUpdateType; 		// hi/med/low/vlow res update
	SINT32 	LastUpdateDelta;
	UINT32 	LastUpdate;
	SINT32 	LastHitmapUpdateTime;
	SINT32 	LastFPSWeaponNum;
	SINT32	LastFireTime; // when we last fired any weapon
	vehVehiclePositionData PosData;	// used by smoothing code

// Damage
	vehDamageQue DamageQue;
	vehanimVehicleStateAnimations StateAnimation;

// Weapon
	UINT32 	LoadoutNum;
	LOGICAL	MountIsFiring		[VEH_MAX_MOUNTS];
	UINT32 	WeaponIsHidden		[VEH_MAX_MOUNTS][VEH_MAX_WEAPONS_PER_MOUNT];

// Engine sound
	FLOAT32 EngineIdleVolume;
	FLOAT32 EnginePowerVolume;

// Icon
	FLOAT32 IconTransparency;
	FLOAT32 IconRingSize;
	FLOAT32 LastVisTransparency;
	ChatHandle	Chat_Handle;
	UINT32 	IconColor;
	SINT32  IconPulse;
	SINT32 	LastVisChangeTime;

	bool	DrawIconHandle:1;
	bool	DrawRangeCircle:1;
	bool	DrawIconId:1;
	bool	DrawIconRank:1;
	bool	DrawIconSquad:1;

// Affliation
	bool	VehicleIsSquaddie:1;
	bool	IsCommander:1;
	bool	IsBuilder:1;
	bool	SameBrigade:1;
	bool	SameMission:1;
	bool	IsMissionLeader:1;
	bool	IsMyMissionLeader:1;
	bool	IsTrainer:1;

	bool	hulksto:1;
	bool	isInside:1;			//!< FPS inside building?

	UCHAR   persDecal:8;

	bool InUse : 1;  			//!< is the vehicle struct in use?
	bool StickVehicleToGround : 1;
	bool VisDamageNoStick : 1;
	bool GotOneTime : 1;		//!< True when we have received the one-time update.

	REL_OBJ_ID fpsBuildingID;   //!< ID of building FPS is in/on.

	SINT32	Side;
	UINT16  unitDecal;
	UINT16	Rank;
	SINT32	Country;

// FX
	SINT32 iFXVehicleID;
	SINT32 splashTime;

// Timers
	SINT32 deployingTimeout;
	SINT32 deployedTimeout;
	SINT32 interiorCheck;

	dSpaceID spaceID;

	UCHAR 	Handle[MAX_HANDLE_SIZE]; // SIZE includes null byte
	UCHAR 	SquadHandle[MAX_HANDLE_SIZE];

	colliderQuery* collider;

#ifdef __cplusplus

	void clear();
	bool isHulked();

	struct vehVehicle* getNext() { return xNext.get(); }
	void setNext(struct vehVehicle*x) { xNext.set(x); }

	LOGICAL isSTO() {return (SINT32)VehicleHostIndex < 0;}

	vehVehicle() { clear(); }

	// start lua compatibility functions with OSG
	void SetPosition(double x, double y, double z)
	{
		if(ModelData)
		{
			ModelData->VehiclePos.x = x;
			ModelData->VehiclePos.y = y;
			ModelData->VehiclePos.z = z;
		}
	}

	void SetYaw(double y)
	{
		if(ModelData)
		{
			ModelData->VehicleAtt.Yaw = y;
		}
	}

	double DistanceTo(double x, double y, double z);
	bool InRange(double x, double y, double z, double distance);
	vehType* GetType() const { return VehicleType; }
	std::string GetHandle() const { return std::string((char*)Handle); }
	unsigned long long GetChatHandle() const { return Chat_Handle; }
	unsigned int GetRank() const { return Rank; }
	unsigned int GetSide() const { return (unsigned int)Side; }
	unsigned int GetCountry() const { return (unsigned int)Country; }
	int GetDecal() const { return (int)persDecal; }
	double GetYaw() const;
	double GetPos(int axis) const;
	double GetTraverse(int turret, bool world = false) const;
	double GetElevation(int turret, bool world = false) const;

	bool IsMyMissLeader() const { return (bool)IsMyMissionLeader; }
	bool IsMissLeader() const { return (bool)IsMissionLeader; }
	bool IsMissMate() const { return (bool)SameMission; }
	bool IsSquadMate() const { return (bool)VehicleIsSquaddie; }
	bool IsBrigMate() const { return (bool)SameBrigade; }
	bool IsBldr() const { return (bool)IsBuilder; }
	bool IsTrnr() const { return (bool)IsTrainer; }
	bool IsHC() const { return (bool)IsCommander; }
	bool IsMe() const {	return (ModelData) ? true : false; }
	// end lua compatibility

	gr2AnimTrooper* GetAnim();
	bool IsFiring(SINT32 index) const;

private:
	// Prevent copy ctor/operator
	vehVehicle(const vehVehicle&);
	vehVehicle& operator=(const vehVehicle&);
#endif
};


/************************************************************************************************
	vehmain
*************************************************************************************************/

UINT32 	vehInit(void);
UINT32 	vehKill(void);

//void vehmainInitModelInstance(OBJECT* root);

UINT32 vehmainCloneVehicle(SINT32 keyState, UINT32 functionNum, UINT32 alwaysZero);

UINT32 	vehCreateNewVehicle(vehVehicle**, UINT32 HostIndex, SINT32 VehicleType, ChatHandle ch = 0);
UINT32 	vehSetVehicleToUserControlled(vehVehicle*,SINT32 MultiCrewPos);
UINT32 	vehKillVehicle(vehVehicle*);

UINT32 	vehUpdatePositionInfo(vehVehicle*);
UINT32	vehApplyDamageToVehicleComponent(vehVehicle* InVehicle, UINT32 ComponentNum, FLOAT64 Energy, UINT32 Counter, weapAmmo* Ammo = nullptr);
UINT32	vehGetHitmapComponentThickness(FLOAT64* Thickness,vehVehicle* Vehicle, UINT32 PartNum);
UINT32	vehFindHitmapObjectNumByName(UINT32* ComponentNum, UCHAR* ComponentName, UINT32 VehicleNum);

UINT32	vehUpdate(void);
namespace Vehicle { struct Hit; }
UINT32 	vehProcessVehicleHit(const Vehicle::Hit&);
UINT32 	vehAdjustAnimationTime(void);

UINT32 		vehGetVehicleHead(vehVehicle**);
vehVehicle*	vehFindVehicle(UINT32 HostIndex);
vehVehicle*	vehFindVehicleByChatHandle(const ChatHandle& chathandle);
LOGICAL vehValid(vehVehicle*);
LOGICAL vehVehicleInList(vehVehicle*);
int vehVehicleSpawned(const vehType* vtype, bool onlyPlayers = true); // how many of this vehicle type is spawned

vehInertiaComponent const* vehFindInertiaComponent(vehType*, const char* name);
vehanimVehicleStateType const* vehFindStateByName(vehType*, const char* name);

// shouldn't be here
UINT32 vehApplyDamageToObject(const FLOAT64& Energy, struct trnCDTerrainObject*, vehVehicle* InShooterVehicle, struct weapAmmo* Ammo);

UINT32 	vehExitVehicle(SINT32 Param,UINT32 Id,UINT32 Detach);
UINT32 	vehExitVehicleTrigger(SINT32 Param, UINT32 Id, UINT32 Detach);

SINT32 	vehAttachToRidePoint(vehVehicle*);
vehVehicle*	vehFindRidingVehicle(UINT32 RideVehicleHostIndex, UINT32 RideSeatNum, UINT32 VehicleType);

// Infantry resupply point
//bool vehmainDropInfantryResupplyPoint();
PlayerPlacedObject* vehmainFRUBuild();
//bool vehmainIRPCheck();
bool vehmainFRUExecute();

// Mobile spawn
SINT32 vehmainDeployVehicleForMobileSpawn();
void vehHandleMobileSpawnUpdate(MSPAWN_RESULT enumResult);
void vehmainUndeployVehicleForMobileSpawn();

LOGICAL vehmainIsMobileSpawn(); // am I the mobile spawn?
LOGICAL vehmainCanSTO();

// vehicle sto tutorial support
UINT32 vehmainAddSTO(const vehType* type, double x, double y, double z, float att, bool relative = false, const char* icon = ""); // utf8 icon
UINT32 vehmainAddSTO(const char* type, double x, double y, double z, float att, bool relative = false, const char* icon = ""); // utf8 icon
bool vehmainRemoveSTO(UINT32 id);

void vehmainReportCache(int& rcount, int &fcount, bool update = true);
void vehmainClearVehicleCache();
bool vehmainIsVehicleCached(ChatHandle ch);
//bool vehmainCachePrintDiff();

LOGICAL vehmainIsDeploying();
LOGICAL vehmainIsDeployed();
LOGICAL vehmainIsUndeploying();
LOGICAL vehmainIsUndeployed();

LOGICAL vehmainIsOverSupplied(vehVehicle* vehicle);
SINT32 vehmainRequestAmmo(const char* input = nullptr); // sends a "I need ammo" message to everyone on mission

LOGICAL vehmainWaitingOnLogoutTimer();
void vehmainBailout(BailoutSTO* pilot = nullptr);
LOGICAL vehmainHasBailedOut();

UINT32 vehmainDetermineKiller();
UINT32 vehmainDeterminePlinkKiller();
UINT32 vehmainDetermineComponentKiller(mlComponent* killComponent);
void vehmainSpawnVehicle(VEHICLE_REPLY const*);

UINT32 vehDetermineExitStatus(SINT32* ExitStatus, FULL_OBJ_ID* id);
void vehmainCreateODECollider(vehVehicle* vehicle);
namespace StratDB { class StratObject; }
StratDB::StratObject* vehmainLastEntered(trnCDTerrainObject** TO = nullptr);
ChatHandle vehLastInfantryDamaged();

#ifdef VEHICLES_GAME_INFO
std::string& vehVehiclesGameInfo();
void vehSaveLastWorldUpdate(WORLD_UPDATE& wu);
const char* vehGetHistoryStringFor(const UINT32 vehicleIndex, char* const into, size_t intoSize_bytes);
void vehRecordVehicleUpdate(const UINT32 vehicleIndex);

#endif
