#pragma once

#include "ml/mltypes.h"
#include "sdm/StratDB.h"
#include "physics/colliderQuery.h"

#define	SUPERTROOP
#undef	SUPERTROOP

// tolua_begin
enum fpFPSPoses
{
	POSE_STAND,
	POSE_CROUCH,
	POSE_PRONE,
	POSE_TRANSITION,
	POSE_SIT,
	POSE_NUM_POSES
};

// control block flags; used by tutorials to block certain inputs
enum fpFPSControlBlocks
{
	CB_NONE				= 0,
	CB_MOVE				= 1<<0,
	CB_POSE				= 1<<1,
	CB_LOOK				= 1<<2,
	CB_FIRE				= 1<<3,
	CB_SELECT			= 1<<4,
	CB_DEPLOY			= 1<<5,
	CB_NUM_BLOCKS		= 6
};
// tolua_end

#define MAX_FPS_STEP .5f
#define FALL_COLLIDER_SPREAD .025f
#define FALL_COLLIDER COLLIDER_TILE|COLLIDER_TRN_GROUND|COLLIDER_MANMADE_MASK|COLLIDER_VEHICLE_MASK|COLLIDER_VEH_HULK|COLLIDER_CAPTURE|COLLIDER_NOFIRE
#define CAGE_COLLIDER COLLIDER_TILE|COLLIDER_TRN_GROUND|COLLIDER_TRN_TREE|COLLIDER_MANMADE_MASK|COLLIDER_VEHICLE_MASK|COLLIDER_VEH_HULK|COLLIDER_BLOCK|COLLIDER_VOLUME
#define WADE_COLLIDER COLLIDER_TRN_WATER
#define AREA_COLLIDER COLLIDER_STO

/*
// fps collider numbers; for tracking
enum fpFPSColliders
{
	FPC_NONE				= 0,

	FPC_RAY_CENTER		= 1<<0,
	FPC_RAY_FRONT		= 1<<1,
	FPC_RAY_RIGHT		= 1<<2,
	FPC_RAY_BACK		= 1<<3,
	FPC_RAY_LEFT		= 1<<4,

	FPC_STAND_FRONT		= 1<<5,
	FPC_STAND_BACK		= 1<<6,
	FPC_STAND_RAYLO		= 1<<7,
	FPC_STAND_RAYMID	= 1<<8,
	FPC_STAND_RAYHI		= 1<<9,

	FPC_CROUCH_FRONT	= 1<<10,
	FPC_CROUCH_BACK		= 1<<11,

	FPC_PRONE			= 1<<12,

	FPC_AREA			= 1<<13, // whats around me

	FPC_NUM				= 14
};

#define FPC_RAY			FPC_RAY_CENTER|FPC_RAY_FRONT|FPC_RAY_RIGHT|FPC_RAY_BACK|FPC_RAY_LEFT
#define FPC_STAND		FPC_STAND_FRONT|FPC_STAND_BACK|FPC_STAND_RAYLO|FPC_STAND_RAYMID|FPC_STAND_RAYHI
#define FPC_CROUCH		FPC_CROUCH_FRONT|FPC_CROUCH_BACK
*/

class FPSCollider : public colliderQuery
{
public:
	FPSCollider();
	virtual ~FPSCollider(){}
	virtual void Update(double deltaTime);
};

struct FPSColliderData
{
	FPSColliderData()
	{
		Reset();
	}

	virtual ~FPSColliderData()
	{
	}

	void Reset()
	{
		collider = nullptr;
		objectHit = nullptr;
		groundMisses = 0;
		groundHit.x = groundHit.y = groundHit.z = 0.0f;
		lastNBPos = groundHit;
		blocked = 0;
	}

	FPSCollider* collider;
	FLOAT64VECTOR groundHit;
	FLOAT64VECTOR lastNBPos; // non-blocked position
	OBJECT* objectHit;
	SINT32 groundMisses;
	SINT32 blocked;

};

// forward declare to avoid header pollution
struct trnCDTerrainObject;
struct vehVehicle;
struct PlayerPlacedObject;

extern struct fpPersonData* fpGetFPPersonDataPtr();
extern struct fpPersonData* fpGetFPPersonDataPtr(mlVehicleData* fromVehicleData);

extern LOGICAL		fpFPSUpdate ();
extern void		fpFPSUpdateModels ();
extern LOGICAL		fpFPSLoadModels (struct vehVehicle*vehicle);
extern LOGICAL		fpFPSDraw (OBJECT* pObject, LOGICAL render);
extern fpFPSPoses	fpFPSGetPlayerPose(); // tolua_export
extern void		fpFPSGetEyeMatrix (piMATRIX* pMatrix);
//extern void		fpFPSGetEyePosition (piVECTOR* pPosition);
extern FLOAT32		fpFPSGetEyeAdjust (int pose = -1, int move = -1, bool asDiff = true); // pose, move, as diff
extern UINT32		fpFPSControlInput (SINT32 Param, UINT32 Id, UINT32 reserve); // tolua_export
extern LOGICAL		fpFPSUpdatePerson (mlVehicleData* pVehicleData, FLOAT64 DeltaTime);
extern void		fpFPSGetVelocity (FLOAT64VECTOR* velocity);
extern LOGICAL		fpFPSWeaponFire (mlVehicleData* pVehicleData);
extern void		fpFPSFireRound (struct weapWeapon* weapon, piMATRIX* fireAtt, void* FireVehicle = nullptr);
extern void		fpFPSReload (struct weapWeapon* weapon);
extern LOGICAL		fpFPSReloadCheck (struct weapWeapon* weapon, SINT32 ammoGroup);
extern LOGICAL		fpFPSSetAnimationFlags();
extern void			fpFPSUpdateSent(); // called when they are sent
extern void		fpFPSKillTrooper(); // tolua_export
extern void		fpFPSKill ();
extern FLOAT32		fpFPSGetBulletDispersion();
extern LOGICAL		fpFPSRideUnrideVehicle ();
extern UINT32		fpFPSGetCurrentWeapon(); // tolua_export
extern LOGICAL		fpFPSGetATPGain(FLOAT32* pATPGain, FLOAT32 deltaTime);
extern void		fpFPSStartSwimming (mlVehicleData* pVehicleData, LOGICAL swimming);
extern SINT32		fpFPSGetLastUnrideTime();
extern LOGICAL		fpFPSIsReloading(); // tolua_export
extern LOGICAL		fpFPSIsSwimming(); // tolua_export
extern LOGICAL		fpFPSIsWading(); // tolua_export
extern LOGICAL		fpFPSIsSilent();
extern LOGICAL		fpFPSIsProne(); // tolua_export
extern LOGICAL		fpFPSIsCrouch(); // tolua_export
extern LOGICAL		fpFPSIsAim(); // tolua_export
extern LOGICAL		fpFPSIsFast(); // tolua_export
extern LOGICAL		fpFPSIsWalking(); // tolua_export
extern bool			fpFPSIsDead(); // tolua_export
extern LOGICAL		fpFPSIsMoving(float threshold = 0.0f); // tolua_export
extern bool			fpFPSIsNoFireZone(); // can we fire here?
extern LOGICAL		fpFPSIsCaptureZone(bool deathCheck = true); // can we fire here?
extern LOGICAL		fpFPSIsBinocs();
extern int			fpFPSIsLeaning();
extern SINT32		fpFPSIsDeployed();
extern void		fpFPSAdjustLightForTrooper (piVECTOR* ld);
extern LOGICAL		fpFPSUpdateSounds(OBJECT* pObject);
extern LOGICAL		fpFPSPlayFallSound(OBJECT* pObject, SINT32 fallType);
extern LOGICAL		fpFPSIsTrooperSitting(OBJECT* pObject);
extern LOGICAL		fpFPSIsObjectFPS (OBJECT* pObject);
extern LOGICAL		fpFPSIsTrooperFalling ();
extern void		fpFPSDrawGunsight (void);
extern void		fpFPSEnableBinoculars (SINT32 enableBinocs);
extern void		fpFPSMarkForVehicleCollideCheck(UINT32 flag);
extern UINT32		fpFPSVehicleCollideCheckNeeded();
extern void		fpFPSDetermineTrooperLoadout (struct vehVehicle*vehicle, UINT32 weaponTypeNum, SINT32 binocsEnabled, SINT32* weaponArray, SINT32* country, SINT32* weaponOfLastResort, LOGICAL* isParatroop);
extern LOGICAL		fpFPSIsExternalView (void);
extern void		fpFPSStartEmote (SCHAR* emote, SCHAR* handle, LOGICAL force);
extern void		fpFPSEjectShell (OBJECT* pObject, piMATRIX* fireAtt, LOGICAL firstPerson);
extern UINT32		fpFPSGetResupplyTarget ();
extern void		fpFPSResupplySelf (struct vehVehicle*vehicle);
extern void		fpFPSResupplySelf (const char* from); // tolua_export
extern void		fpFPSResupplySelf (const wchar_t* from);
extern void		fpFPSSetStandMaterial (SINT32 matid);
extern SINT32		fpFPSPlaySound(const SCHAR* sound);
extern SINT32		fpFPSGetAmmoTypeNum (bool forType = false);
extern SINT32		fpFPSGetAmmoTypeNumByWeapon (SINT32 weaponType, weapWeapon* weapon = nullptr);
extern void		fpFPSLeanOff();
extern LOGICAL		fpFPSGetLeanPoints(piVECTOR* startPos, piVECTOR* endPos);
extern void		mlInfantryCheckLeanLines(mlVehicleData*	VehicleData);
extern LOGICAL		fpFPSStateWeaponReadyToFire(bool honorDelays = true);
extern void			fpFPSetHeadFixed(LOGICAL fixed); // set whether our head fixed looking forward or not
extern void			fpFPSSetSteepness(FLOAT32 s);
extern void			fpFPSAreaChecks(FLOAT64VECTOR* v = nullptr, FLOAT64 DeltaTime = 0.0f, bool force = false);
extern FPSColliderData& fpFPSGetCollider();
extern void fpFPSCancelPPO();

extern void        fpFPSHitBuilderObject(const StratDB::StratObject* pStatue);

extern SINT32		fpFPSfxPlayFireSound(char* soundName, SINT32 soundPriority, SINT32 soundType, FLOAT64VECTOR* soundPosition, OBJECT* soundObject);
extern SINT32		fpFPSfxPlayNormalSound(const char* soundName, SINT32 soundPriority, SINT32 soundType, FLOAT64VECTOR* soundPosition, OBJECT* soundObject);
extern SINT32		fpFPSfxPlayTPSMovementSound(char* soundName, OBJECT* soundObject, FLOAT32 fMin, FLOAT32 fMax, void* model, void (*pFn)(void* data));
extern SINT32		fpFPSfxPlayTPSSingleSound(const char* soundName, OBJECT* soundObject, FLOAT32 fMin, FLOAT32 fMax);

extern void		fpFPSSetInfantrySurfaceID (SINT32 infSurfID, struct trnCDTerrainObject* TO);
extern void		fpFPSSetStandingOnVehicle (LOGICAL value);

extern FLOAT64		fpFPSGetBridgeDamage (void);
extern FLOAT64		fpFPSGetBridgeRepair (void);

extern FLOAT32		fpFPSGetConcussionMod (void);

extern FLOAT32		fpFPSGetRedScreenTime ();
extern FLOAT32		fpFPSGetRedScreenMinAlpha ();

extern void		fpFPSParachuteOpened ();
extern void		fpFPSParachuteLanded ();
extern LOGICAL		fpFPSIsTrooperParachuteOpen ();

extern void		fpFPSSetInheritedVelocity (FLOAT64VECTOR* velocity);
extern void		fpFPSDoRidingShake ();
extern FLOAT64		fpFPSGetParachuteFallSpeed  ();

extern void		fpFPSToggleAutoReload();
extern void		fpFPSSetAutoReload(LOGICAL state);

extern void		fpFPSMapState(LOGICAL state);

extern void		fpFPSSelectNextWeapon(); // tolua_export
extern void		fpFPSSelectPreviousWeapon(); // tolua_export

extern UINT32		fpFPSGetHighlightedWeaponType();
extern SINT32		fpFPSGetWeaponType();

// fixes up clipBullets and ammo; last argument is for convenience, quite often we know this
extern void		fpFPSUpdateAmmo(SINT32 weaponNumber, weapWeapon* weapon = nullptr, SINT32 forceAmmo = -1);
extern void		fpFPSUpdateOverSupply(mlVehicleData* vd);
extern void		fpFPSAddWeaponLag(int plane, FLOAT64 amount, bool absolute = false);
extern void		fpFPSProcessTPSEmote(ChatHandle handle, SINT32 emoteID); // tolua_export

extern bool		fpFPSControlBlocked(int flag); // tolua_export
extern void		fpFPSControlBlockSet(int flag); // tolua_export
extern void		fpFPSControlBlockClear(int flag); // tolua_export
extern void		fpFPSControlBlockReset(); // tolua_export

extern bool fpFPSOnBridge();
extern LOGICAL	fpFPSGetFirePoint(OBJECT* pObject, piMATRIX* pMatrix, bool visual = false);
extern LOGICAL	fpFPSGetFireDirection(OBJECT* pObject, piMATRIX* pMatrix);

#ifdef BETA_RELEASE
extern void		fpFPSSetAnimationSpeed(float speed);
extern float		fpFPSGetAnimationSpeed();
extern void		fpFPSSetSuperTroop(LOGICAL st);
#endif

extern void generateRotationMatrix (int axis, FLOAT64 angle, piMATRIX* pMatrix);

extern void fpFPSfxKill();
extern LOGICAL fpFPSfxInit();
extern UINT32 fpFPSfxEjectShell(piVECTOR& gundir,
						 piVECTOR& ejectdir,
						 piVECTOR& troopspeed,
						 FLOAT64VECTOR& shellpos,
						 FLOAT32 shellspeed,
						 LOGICAL firstperson,
						 FLOAT32 scale,
						 FLOAT64VECTOR& trooppos);

struct fpFPSInfantryData
{
	FLOAT64VECTOR Velocity;
	FLOAT64 Yaw;
	UINT32 AnimFlags;
};
extern void fpFPSGetInfantryData(fpFPSInfantryData& modelData);
extern LOGICAL	fpFPSUpdateInfantry(FLOAT64 DeltaTime);

extern FLOAT32 fpFPSGetGrenadeThrowPower();
extern bool fpFPSCanProjectGrenade();

//--------------
// !!!HACK!!! - for vehInterpolator
void InitColliderGeometry(void);
void _updateFPSColliders(void);
extern fpFPSPoses HACK_forced_pose;
extern Vec3d* HACK_forced_pos;