#include <syscfg.h> // Path is project specific.

#include "gr2/gr2Anim.h"
#include "gr2/gr2layer.h"
#include "fp/fpTPSdata.h"
#include "pers/persmain.h"
#include "math/mathvect.h"
#include "math/math3d.h"
#include "math/matheul.h"
#include "math/mathnum.h"
#include "include/vars.h"
#include "draw/draw3d.h"
#include "weap/weapons.h"
#include "fp/fpTypes.h"
#include "veh/vehmain.h"
#include "fx/fxMgr.h"
#include "fx/TransformEffect.h"
#include "sn/sndsSoundCache.h"
#include "fio/fio.h"
#include "pers/persgunsight.h"
#include "ml/mlutility.h"
#include "trn/trncd.h"
#include "pers/persmsg.h"
#include "miss/missionUtils.h"
#include "comm/commClientMessages.h"
#include "math/mathUnitConversion.h"
#include "fp/fpFPS.h"
#include "fp/fpFPSdata.h"
#include "imap/ww2func.h"
#include "rn/rnmain.h"
#include "locl/loclApp.h"
#include "trn/trnvars.h"
#include "fp/fpFPSinternal.h"
#include "fp/fpMain.h"
#include "trn/trnmain.h"
#include "ml/mlhead.h"
#include "imap/imap.h"
#include "wwii/wwiiCovert.h"
#include "locl/loclLang.h"
#include "ui/uiMain.h"
#include "phui/DOM.h"
#include "ui/uiChat.h"
#include "client/GameClient.h"
#include "chat/chatHandle.h"
#include "str/stringLib.h"
#include "fp/fpHash.h"
#include "ui/uiGameEvents.h"
#include "uimap/uiMap.h"
#include "tut/TutorialManager.h"
#include "wwii/wwiiPermissions.h"
#include "ob/obdraw.h"
#include "fp/fpTPSPhysics.h"
#include "math/Matrixd.h"
#include "wwii/wwiiClientMessages.h"
#include "strt/strtCPFlags.h"
#include "client/clientSTO.h"
#include "ogrp/ogrptype.h"
#include "fp/fpPPO.h"
#include "fp/fpTPS.h"
#include "ob/obmain.h"
#include "physics/colliderQuery.h"
#include "spt/Random.h"
#include "scen/ScenarioManager.h"
#include "physics/colliderMain.h"
#include "ez/EZVehicles.h"
#include "fx/fxMisc.h"

static void             fpFPSAxisRotation (SINT32 param, UINT32 id);
static void             fpFPSSetHorizontalDeployedPosition (FLOAT64 newPos);
static void             fpFPSVerticalAxisLook (SINT32 param, UINT32 data);
static void             fpFPSHorizAxisLook (SINT32 param, UINT32 data);
static void             fpFPSAdjustAngles (FLOAT32 deltaTime, FLOAT64 minSpeed, FLOAT64 maxSpeed, FLOAT64* xAngle, FLOAT64 xTarget);


extern void fpPPOReset();

#define TESTING_SCOPE
#undef TESTING_SCOPE
//#define TESTING

#include "fx/fxSnd.h"

static float WEAPON_LAG_VMAX		= 6.0f; // max from center
static float WEAPON_LAG_HMAX		= 6.0f; // max from center

static float WEAPON_LAG_ACCEL		= 7.0f; // how much to lag based on mouse movement
static float WEAPON_LAG_BLEED		= 5.0f; // how much to bleed each frame

static float WEAPON_LAG_MOVE		= 4.0f; // amount of lag when strafing and moving backwards
static float WEAPON_LAG_JUMP		= 40.0f; // base value to calculate from
static float WEAPON_LAG_JUMPMAX		= 3.0f; // max jump between frames

#ifdef UNUSED
static float WEAPON_LAG_INTERVAL	= 2.0f;
static float WEAPON_LAG_AIMMOD		= .1f; // if aimed, reduce effects
#endif

static float ShakeCamMagnitude = 0.0f;
static int ShakeStart = 0;
static int ShakeDuration = 0;

#define MAX_WADING 1.3f
#define MAX_WADING_SPRINT .3f
#define MAX_WADING_PRONE .1f
#define MAX_WADING_KNEEL .9f

#define	AIM_VERTICAL_ANGLE	(aimVertAngle + aimVertAngleOffset)
//#define STRAT_COLLISION_FLAGS COLLIDER_CAPTURE|COLLIDER_NOFIRE
//#define GROUND_COLLISION_FLAGS COLLIDER_HARDTERRAIN_MASK|COLLIDER_TRN_WATER|COLLIDER_VEHICLE_MASK|COLLIDER_WALK|COLLIDER_STO
//#define AREA_COLLISION_FLAGS STRAT_COLLISION_FLAGS|GROUND_COLLISION_FLAGS

dGeomID _createFPSBall(dSpaceID space, double radius, double xoff, double yoff, double zoff);
dGeomID _createFPSCylinder(dSpaceID space, double length, double radius, double angle, int axis, double xoff, double yoff, double zoff);
dGeomID _createFPSCapsule(dSpaceID space, double length, double radius, double angle, int axis, double xoff, double yoff, double zoff);
dGeomID _createFPSRay(dSpaceID space, double length, double xoff, double yoff, double zoff, double dx, double dy, double dz);
dGeomID _createFPSBox(dSpaceID space, double lx, double ly, double lz);

struct FPSStandCollider : public FPSCollider
{
	FPSStandCollider() : FPSCollider()
	{
		dSpaceID s = space();

		// are we wading
		addGeom(_createFPSRay(s, 2.0f, 0, 0, 2.0f, 0, 0, -1.0f), COLLIDER_FPS, WADE_COLLIDER);

		// gravity collider
		addGeom(_createFPSRay(s, 10000.0f, 0, 0, 1.5f, 0, 0, -1.0f), COLLIDER_FPS, FALL_COLLIDER); // center
		addGeom(_createFPSRay(s, 3.0f, FALL_COLLIDER_SPREAD, FALL_COLLIDER_SPREAD, 1.5f, 0, 0, -1.0f), COLLIDER_FPS, FALL_COLLIDER); // front
		addGeom(_createFPSRay(s, 3.0f, FALL_COLLIDER_SPREAD, -FALL_COLLIDER_SPREAD, 1.5f, 0, 0, -1.0f), COLLIDER_FPS, FALL_COLLIDER); // right
		addGeom(_createFPSRay(s, 3.0f, -FALL_COLLIDER_SPREAD, FALL_COLLIDER_SPREAD, 1.5f, 0, 0, -1.0f), COLLIDER_FPS, FALL_COLLIDER); // back
		addGeom(_createFPSRay(s, 3.0f, -FALL_COLLIDER_SPREAD, -FALL_COLLIDER_SPREAD, 1.5f, 0, 0, -1.0f), COLLIDER_FPS, FALL_COLLIDER); // left

		//addGeom(_createFPSBall(s, .2f, 0, -.15f, 1.6f), COLLIDER_FPS, CAGE_COLLIDER); // head
		addGeom(_createFPSBall(s, .45f, 0, 0, 1.1f), COLLIDER_FPS, CAGE_COLLIDER); // hi
		addGeom(_createFPSBall(s, .3f, 0, 0, .8f), COLLIDER_FPS, CAGE_COLLIDER); // mid
		addGeom(_createFPSBall(s, .15f, 0, 0, .45f), COLLIDER_FPS, CAGE_COLLIDER); // lo
		addGeom(_createFPSRay(s, .8f, 0, 0, 1.0f, 0, -.2f, .8f), COLLIDER_FPS, CAGE_COLLIDER); // for head bumps
		//addGeom(_createFPSCylinder(s, 1.4f, .5f, 0, 1, 0, -.1f, .9f), COLLIDER_FPS, CAGE_COLLIDER); // front
		//addGeom(_createFPSCylinder(s, 1.4f, .5f, 0, 1, 0, .1f, .9f), COLLIDER_FPS, CAGE_COLLIDER); // back

		/*
		// cage collider capsules
		addGeom(_createFPSCapsule(s, 1.4f, .5f, 0, 1, 0, -.1f, .9f), COLLIDER_FPS, CAGE_COLLIDER); // front
		addGeom(_createFPSCapsule(s, 1.4f, .5f, 0, 1, 0, .1f, .9f), COLLIDER_FPS, CAGE_COLLIDER); // back

		const int rows = 3;
		const int rays = 10;
		const double base = .5f;
		const double gap = .3f;

		// cage collider backup rays
		for(int row = 0; row < rows; row++)
		{
			double rayZ		= base + double(row * gap);
			double length	= 1.0f;
			double minLen	= .3f;
			double maxLen	= .5f;
			double diff		= maxLen - minLen;

			Vec3d forward	= Vec3d(0, 1.0f, 0);
			Vec3d dir		= forward;

			Matrixd rotate;
			rotate.makeRotate(mlDEG_TO_RAD(360.0f/rays), 0, 0, 1.0f);

			for(int ray = 0; ray < rays; ray++)
			{
				// calculate our ray length
				// rays on towards the sides will be shorter
				length = minLen + (fabs(forward * dir) * diff);

				// add our ray collider
				addGeom(_createFPSRay(s, length, 0, 0, rayZ, dir.x(), dir.y(), dir.z()), COLLIDER_FPS, CAGE_COLLIDER);

				// rotate the next ray around
				dir = dir * rotate;
			}
		}
		*/

		// area collider
		addGeom(_createFPSBox(s, 2.0f, 2.0f, 2.0f), COLLIDER_FPS, AREA_COLLIDER);
	}

	//virtual void Update(double deltaTime);

	virtual ~FPSStandCollider()
	{
	}
};

struct FPSKneelCollider : public FPSCollider
{
	FPSKneelCollider() : FPSCollider()
	{
		/*
		dSpaceID s = space();

		// are we wading
		addGeom(_createFPSRay(s, 2.0f, 0, 0, 2.0f, 0, 0, -1.0f), COLLIDER_FPS, WADE_COLLIDER);

		// gravity collider
		addGeom(_createFPSRay(s, 50.0f, 0, 0, 1.0f, 0, 0, -1.0f), COLLIDER_FPS, FALL_COLLIDER); // center
		addGeom(_createFPSRay(s, 3.0f, FALL_COLLIDER_SPREAD, FALL_COLLIDER_SPREAD, 1.0f, 0, 0, -1.0f), COLLIDER_FPS, FALL_COLLIDER); // front
		addGeom(_createFPSRay(s, 3.0f, FALL_COLLIDER_SPREAD, -FALL_COLLIDER_SPREAD, 1.0f, 0, 0, -1.0f), COLLIDER_FPS, FALL_COLLIDER); // right
		addGeom(_createFPSRay(s, 3.0f, -FALL_COLLIDER_SPREAD, FALL_COLLIDER_SPREAD, 1.0f, 0, 0, -1.0f), COLLIDER_FPS, FALL_COLLIDER); // back
		addGeom(_createFPSRay(s, 3.0f, -FALL_COLLIDER_SPREAD, -FALL_COLLIDER_SPREAD, 1.0f, 0, 0, -1.0f), COLLIDER_FPS, FALL_COLLIDER); // left

		//addGeom(_createFPSBall(s, .2f, 0, -.15f, 1.4f), COLLIDER_FPS, CAGE_COLLIDER); // head
		addGeom(_createFPSBall(s, .4f, 0, -.1, .8f), COLLIDER_FPS, CAGE_COLLIDER); // hi
		addGeom(_createFPSBall(s, .2f, 0, -.1, .4f), COLLIDER_FPS, CAGE_COLLIDER); // lo
		//addGeom(_createFPSCylinder(s, 1.4f, .5f, 0, 1, 0, -.1f, .9f), COLLIDER_FPS, CAGE_COLLIDER); // front
		//addGeom(_createFPSCylinder(s, 1.4f, .5f, 0, 1, 0, .1f, .9f), COLLIDER_FPS, CAGE_COLLIDER); // back

		// area collider
		addGeom(_createFPSBox(s, 2.0f, 2.0f, 2.0f), COLLIDER_FPS, AREA_COLLIDER);
		*/

		dSpaceID s = space();

		// are we wading
		addGeom(_createFPSRay(s, 2.0f, 0, 0, 2.0f, 0, 0, -1.0f), COLLIDER_FPS, WADE_COLLIDER);

		// gravity collider
		addGeom(_createFPSRay(s, 10000.0f, 0, 0, 1.5f, 0, 0, -1.0f), COLLIDER_FPS, FALL_COLLIDER); // center
		addGeom(_createFPSRay(s, 3.0f, FALL_COLLIDER_SPREAD, FALL_COLLIDER_SPREAD, 1.5f, 0, 0, -1.0f), COLLIDER_FPS, FALL_COLLIDER); // front
		addGeom(_createFPSRay(s, 3.0f, FALL_COLLIDER_SPREAD, -FALL_COLLIDER_SPREAD, 1.5f, 0, 0, -1.0f), COLLIDER_FPS, FALL_COLLIDER); // right
		addGeom(_createFPSRay(s, 3.0f, -FALL_COLLIDER_SPREAD, FALL_COLLIDER_SPREAD, 1.5f, 0, 0, -1.0f), COLLIDER_FPS, FALL_COLLIDER); // back
		addGeom(_createFPSRay(s, 3.0f, -FALL_COLLIDER_SPREAD, -FALL_COLLIDER_SPREAD, 1.5f, 0, 0, -1.0f), COLLIDER_FPS, FALL_COLLIDER); // left

		addGeom(_createFPSBall(s, .45f, 0, -.2, 1.1f), COLLIDER_FPS, CAGE_COLLIDER); // hi
		addGeom(_createFPSBall(s, .3f, 0, -.2, .8f), COLLIDER_FPS, CAGE_COLLIDER); // mid
		addGeom(_createFPSBall(s, .15f, 0, -.2, .45f), COLLIDER_FPS, CAGE_COLLIDER); // lo
		addGeom(_createFPSRay(s, 1.0f, 0, -.2, .5f, 0, -.2f, .8f), COLLIDER_FPS, CAGE_COLLIDER); // for head bumps

		// area collider
		addGeom(_createFPSBox(s, 2.0f, 2.0f, 2.0f), COLLIDER_FPS, AREA_COLLIDER);
	}

	//virtual void Update(double deltaTime);

	virtual ~FPSKneelCollider()
	{
	}
};

struct FPSProneCollider : public FPSCollider
{
	FPSProneCollider() : FPSCollider()
	{
		dSpaceID s = space();

		// are we wading
		addGeom(_createFPSRay(s, 2.0f, 0, 0, 2.0f, 0, 0, -1.0f), COLLIDER_FPS, WADE_COLLIDER);
		addGeom(_createFPSRay(s, 2.0f, 0, .6f, 2.0f, 0, 0, -1.0f), COLLIDER_FPS, WADE_COLLIDER);

		// gravity collider
		addGeom(_createFPSRay(s, 10000.0f, 0, 0, 1.0f, 0, 0, -1.0f), COLLIDER_FPS, FALL_COLLIDER); // center
		addGeom(_createFPSRay(s, 3.0f, FALL_COLLIDER_SPREAD, FALL_COLLIDER_SPREAD, 1.0f, 0, 0, -1.0f), COLLIDER_FPS, FALL_COLLIDER); // front
		addGeom(_createFPSRay(s, 3.0f, FALL_COLLIDER_SPREAD, -FALL_COLLIDER_SPREAD, 1.0f, 0, 0, -1.0f), COLLIDER_FPS, FALL_COLLIDER); // right
		addGeom(_createFPSRay(s, 3.0f, -FALL_COLLIDER_SPREAD, FALL_COLLIDER_SPREAD, 1.0f, 0, 0, -1.0f), COLLIDER_FPS, FALL_COLLIDER); // back
		addGeom(_createFPSRay(s, 3.0f, -FALL_COLLIDER_SPREAD, -FALL_COLLIDER_SPREAD, 1.0f, 0, 0, -1.0f), COLLIDER_FPS, FALL_COLLIDER); // left

		addGeom(_createFPSBall(s, .15f, 0, -.7f, .25f), COLLIDER_FPS, CAGE_COLLIDER); // hi
		addGeom(_createFPSBall(s, .15f, 0, -.5f, .25f), COLLIDER_FPS, CAGE_COLLIDER); // lo
		addGeom(_createFPSBall(s, .15f, 0, -.3f, .25f), COLLIDER_FPS, CAGE_COLLIDER); // lo
		addGeom(_createFPSBall(s, .15f, 0, -.1f, .25f), COLLIDER_FPS, CAGE_COLLIDER); // lo
		addGeom(_createFPSBall(s, .15f, 0, .1f, .25f), COLLIDER_FPS, CAGE_COLLIDER); // lo
		addGeom(_createFPSRay(s, 1.0f, 0, 0, .25f, 0, -.2f, .8f), COLLIDER_FPS, CAGE_COLLIDER); // for head bumps

		// area collider
		addGeom(_createFPSBox(s, 2.0f, 2.0f, 2.0f), COLLIDER_FPS, AREA_COLLIDER);
	}

	//virtual void Update(double deltaTime);

	virtual ~FPSProneCollider()
	{
	}
};

// flagged to true if we determined that we ran into an enemy vehicle
// if it's true, fpMain will call mlDetermineInfantryCollisions again
// and kill the trooper if they're inside of an enemy vehicle after movement is complete
static UINT32				vehicleCollideCheckNeeded;
#ifdef STOP_FIRING_INSIDE
static UINT32				fpsHitVehicleThisFrame;
#endif
// and now the local stuff
static gr2ModelRef		armsModels[NUM_INFANTRY_TYPES];
static gr2ModelRef		weaponModels[NUM_WEAPONS];
static gr2ModelRef		secondaryModels[NUM_WEAPONS];
static gr2ModelRef		auxModels[NUM_WEAPONS][FPS_NUM_AUX_MODELS];
static gr2ModelRef		legsModels[NUM_INFANTRY_TYPES];
static gr2ModelRef		armTexModel;

static gr2ModelRef		skeletonModel, armsModel, weapModel, clipModel, legsModel, auxModel[FPS_NUM_AUX_MODELS];
static gr2ControlRef	currSkelControl, currWeapControl, currClipControl, currRGCupControl;

// these can be combined at some point
static piMATRIX				eyeRotation; // saves the current eyepoint rotation
static piVECTOR				eyePosition; // saves the current eyepoint position

static gr2Animation*		weapDefaultAnim[NUM_WEAPONS];
static gr2Animation*		weapEmptyDefaultAnim[NUM_WEAPONS];
static gr2Animation*		clipDefaultAnim[NUM_WEAPONS];

static gr2Animation*		armsAnims[NUM_WEAPONS][NUM_WEAP_STATES];
static gr2Animation*		weapAnims[NUM_WEAPONS][NUM_WEAP_STATES];
static gr2Animation*		clipAnims[NUM_WEAPONS][NUM_WEAP_STATES];


static fpsWeaponStates	currentFPSstate;	// this is the active animation state (index into animation arrays)
static UINT32			stateFlags;			// this is the master state repository
static UINT32			prevEyeStateFlags;
static UINT32			stat2Flags;
static UINT32			controlBlockFlags;
static UINT32			currentCollisionFlags; // what we hit
static bool				noFire;
static bool				snapView = false;
static bool				fireDown = false;

// uses the FPS_* defines defined above
static UINT32			controlStateFlags;

static fpsInfantryTypes	currentArms;
fpsWeaponTypes	currentWeaponModel;
static fpsWeaponTypes	lastWeaponModel;

static FLOAT64			aimVertAngle, aimVertAngleOffset;	// + is down, - is up.
static FLOAT64			vertWeaponLag, vertWeaponLagAcc;
static FLOAT64			horizWeaponLag, horizWeaponLagAcc;	// + is down, - is up.
#ifdef UNUSED
static FLOAT64			horizWeaponLagLast, vertWeaponLagLast;
#endif

static FLOAT64			aimVerticalDeployedPosition;	// + is down, - is up.  in imap units, so needs to be multiplied
														// with granularities to determine the actual angle in degrees
static FLOAT64			aimHorizontalDeployedPosition;	// + is down, - is up.  in imap units, so needs to be multiplied
														// with fpsTrooper.mouseGranularity to determine the actual angle in degrees

static FLOAT64			mouseLookHorizAngle, mouseLookVertAngle;
static SINT32			mouseLookOn;
static FLOAT64			grenadeApperLapse = 0.0;

static SINT32 _startDeathTimer = 0;

// movement controls
// these are the modifiers for the various poses
static fpFPSPositions	prevPose;
static fpFPSPositions	targetPose;
static FLOAT32			currentEyeHeight;
static FLOAT32          heightAtDeath;

static FLOAT32			currentForwardVectorLength;	// current y velocity
static FLOAT32			currentStrafeVectorLength;	// current x velocity

static weapWeapon*		currentGameWeapon;		// weapon currently being held

static UINT32			deathState;				// which death animation to play

// these prevent/permit drawing of the various components .. tried to use enable/disable in granny
// but that caused flashes
static bool				showArms;
static bool				showGun;
static bool				showClip;
static bool				showAux[NUM_WEAPONS][FPS_NUM_AUX_MODELS];
static bool				rgRoundFired;
static FLOAT64			fireStartTime;
static FLOAT64			weaponChangeStartTime;
static FLOAT32			deployStartTime;
static UINT32			fireUpdateNo;
static SINT32			fireDelay;
static SINT32			lastInput;

// when concentrating, no input is accepted
// things like when you are firing or deploying/undeploying
static bool				concentrating; // not yet used

// capture table stuff

static const StratDB::StratObject*	currentBuilderObject;
static FLOAT64VECTOR            builderPosition;

// emote stuff
static FLOAT64			emoteStartTime;
static FLOAT32			emoteLength;
static SINT32			emoteID;		// network ID for emote .. 0 is "no emote", 1-n is combat, n+1-m is body
static fpTPSEmoteType	emoteType;

static SINT32			shellEjectTime;

// deployed state stuff
static bool				deployedStateInitialized, deployNeedsDeployment, deployNeedsUndeployment;
static piVECTOR			deployPositionRaw, deployPosition;
//static piVECTOR			deployModelPosition;
static SINT32			groundMaterial;

// loadout dependent stuff
static FLOAT32			loadoutATPMultiplier;
static FLOAT32			fpFPSBulletATPEffect;
static FLOAT32			poseChangeATPEffect;

// resupply data
static UINT32			resupplyTarget;
static SINT32			resupplyTime;
static SINT32			primaryWeaponStartAmmo;

static bool				overSupplyWarning;
static piVECTOR			oversupplyPosition;

static FPSColliderData	_collider;
static FPSStandCollider _standCollider;
static FPSKneelCollider _kneelCollider;
static FPSProneCollider _proneCollider;

//static SINT32			currentSightNotch;

// debug stuff
static FLOAT32			animationSpeed = 1.0f;

static SINT32 fpsStartGrenadeThrowTime, fpsEndGrenadeThrowTime;

#define MIN_RIDE_DISTANCE2 9

// Sniper stuff

static bool				scopeUp;
static FLOAT64          scopeDelay;

static SINT32 _ppoLastStance = 0;
static UINT32 _ppoLastHit = 0;
static UINT32 _ppoCheck = 0;

// how steep a hill are we going UP
static FLOAT32 _steepness = 0.0f;

static ScenarioVolume* _volumeCollider = nullptr;

static bool _candeploy(int direction);
static void _calculateConvergenceAngle(int weaponIndex = -1); // 0 - 9
static void _selectBestAmmo(SINT32 weaponNumber, weapWeapon* weapon);

// weapon lag
static void _updateWeaponLag(float deltaTime);
//static float _getMaxWeaponLag(int lagPlane, float lagScale);
//static void _recoverWeaponLag();

static FLOAT32 _deployZ();

static void fpFPSAdjustTurretElevation (mlVehicleData* pVehicleData, FLOAT32 maxUpAngle, FLOAT32 maxDownAngle);

#ifdef DEBUG_DEPLOY_CRAP
static piVECTOR _obstructp1, _obstructp2;
static piVECTOR _centerp1, _centerp2;
static piVECTOR _rightp1, _rightp2;
static piVECTOR _leftp1,_leftp2;
#endif

#define HAS_ALL_BITS(value, mask) (((value) & (mask)) == (mask))
#define HAS_ANY_BITS(value, mask) (((value) & (mask)))

fpPersonData* fpGetFPPersonDataPtr()
{
	mlVehicleData* vehicleData = persGetPersonaModelDataPtr();
	return (vehicleData) ? (fpPersonData*) vehicleData->PersonData : nullptr;
}

// Save a call to persGetPersonaModelDataPtr if you already have this pointer
fpPersonData* fpGetFPPersonDataPtr(mlVehicleData* fromVehicleData)
{
	assert(fromVehicleData != nullptr);
	return (fpPersonData*)(fromVehicleData->PersonData);
}

void fpFPSInit(vehVehicle*vehicle)
{
	SCHAR*		pPtr = nullptr;

	SINT32		countryID;

	animationSpeed = 1.0f;

	persSetNormalView(fpsTrooper.defaultFOV);
	fpsTrooper.desiredFOV = fpsTrooper.defaultFOV;
    fpsTrooper.eyeHorizOffset = 0;

#ifdef TESTING_SCOPE
	fpsTrooper.sniperScope = TRUE;
	fpsTrooper.sniperScopeDelay = 1.0f;
#else
	fpsTrooper.sniperScope = FALSE;
	fpsTrooper.sniperScopeDelay = 0.0f;
#endif

	fpsTrooper.currentMuzzleRise = 0.0;
	fpsTrooper.targetMuzzleRise = 0.0;
	aimVertAngle = 0.0;
	aimVertAngleOffset = 0.0;

	currentGameWeapon = nullptr;

	persClearGunsights();
	// Load Binoculars
    persLoadGunsight ((char *)"vehicles\\ground\\groundman\\8x30binoc.gs");
	//persLoadInstGunsight("vehicles\\ground\\groundman\\4x_riflescope.gs");
	persGunSightSetBaseFOV(fpsTrooper.defaultFOV);

	scopeDelay = 0.0f;
	scopeUp = false;

#if AIMING_WEAPONS
	fpsTrooper.mouseGranularity = 0.01;
#endif

	memset (armsModels, 0, sizeof (armsModels));
	memset (weaponModels, 0, sizeof (weaponModels));
	memset (secondaryModels, 0, sizeof (secondaryModels));
	memset (auxModels, 0, sizeof (auxModels));
	memset (legsModels, 0, sizeof (legsModels));
	armTexModel = nullptr;
	memset (weapDefaultAnim, 0, sizeof (weapDefaultAnim));
	memset (weapEmptyDefaultAnim, 0, sizeof (weapEmptyDefaultAnim));
	memset (clipDefaultAnim, 0, sizeof (clipDefaultAnim));
	memset (armsAnims, 0, sizeof (gr2Animation*)*NUM_WEAPONS*NUM_WEAP_STATES);
	memset (weapAnims, 0, sizeof (gr2Animation*)*NUM_WEAPONS*NUM_WEAP_STATES);
	memset (clipAnims, 0, sizeof (gr2Animation*)*NUM_WEAPONS*NUM_WEAP_STATES);

	showArms = true;
	showGun = true;
	showClip = true;
	rgRoundFired = false;

	LOGICAL isParatroop = fpsTrooper.isParatroop.Get();
	LOGICAL binocsEnabled = fpsTrooper.binocsEnabled.Get();
	fpFPSDetermineTrooperLoadout (vehicle, vehicle->VehicleType->VehicleTypeNum, binocsEnabled, (SINT32*)(fpsTrooper.weaponPositionModel), &countryID, &fpsTrooper.weaponOfLastResort, &isParatroop);

	if (isParatroop)
	{
		fpsTrooper.isParatroop = TRUE;
		fpsTrooper.parachuteWorn = TRUE;
		fpsTrooper.parachuteOpen = FALSE;
		fpsTrooper.parachuteOKToOpen = FALSE;
	}
	else
	{
		fpsTrooper.isParatroop = FALSE;
		fpsTrooper.parachuteWorn = FALSE;
		fpsTrooper.parachuteOpen = FALSE;
		fpsTrooper.parachuteOKToOpen = FALSE;
	}

	UINT16 ammoTypeInUse = vehicle->Weapons[0]->AmmoTypeInUse.Get();
	primaryWeaponStartAmmo = vehicle->Weapons[0]->NumRoundsAmmoLeft[ammoTypeInUse];

	// calculate the convergence angles if needed, but only calculate once
	// FIXME - get rid of this 3
	for ( int i=0;i<=3;i++)
	{
		if(fpsTrooper.weaponPositionModel[i] == NO_WEAPON)
			continue;

		// this ensure we start with a valid convergence angle
		if (fpsWeapons[fpsTrooper.weaponPositionModel[i]].convergenceDistance > 0.0)
		{
			FLOAT32	angle;
			persGunsightCalcSightAngle(&angle,
				(char*) vehicle->VehicleType->Loadout[0].WeaponTypes[i][0]->AmmoType[0]->Name,
				fpsWeapons[fpsTrooper.weaponPositionModel[i]].convergenceDistance);

			fpsWeapons[fpsTrooper.weaponPositionModel[i]].convergenceAngle.Set(mlRAD_TO_DEG(angle));
		}

		// range table for rifle sights
		if (fpsWeapons[fpsTrooper.weaponPositionModel[i]].numSightDists > 0)
		{
			fpsWeapons[fpsTrooper.weaponPositionModel[i]].currentSightNotch = 0;

			for (int j = 0; j < fpsWeapons[fpsTrooper.weaponPositionModel[i]].numSightDists; j++)
			{
				if (fpsWeapons[fpsTrooper.weaponPositionModel[i]].getSightDist(j) == fpsWeapons[fpsTrooper.weaponPositionModel[i]].convergenceDistance)
				{
					fpsWeapons[fpsTrooper.weaponPositionModel[i]].currentSightNotch = j;
					break;
				}
			}
		}

		//_calculateConvergenceAngle(i);

		/* new code
		// range table for rifle sights
		if (fpsWeapons[fpsTrooper.weaponPositionModel[i]].numSightDists > 0)
		{
			currentSightNotch = 0;
			fpFPSWeaponData* wd = &fpsWeapons[fpsTrooper.weaponPositionModel[i]];

			wd->sightDistances	= new float[wd->numSightDists];
			wd->sightConvergances = new float[wd->numSightDists];

			for (int j = 0; j < wd->numSightDists; j++)
			{
				// init the sight distances table
				wd->sightDistances[j] = wd->startSightDists + (j * wd->incSightDists);

				// calculate the angle
				FLOAT32 angle;
				persGunsightCalcSightAngle(&angle,
					(char*)vehicle->VehicleType->Loadout[0].WeaponTypes[i][0]->AmmoType[0]->Name,
					wd->sightDistances[j]);

				wd->sightConvergences[j] = mlRAD_TO_DEG (angle);

				if (wd->sightDistances[j] == wd->convergenceDistance)
					currentSightNotch = j;
			}
		}
		*/
	}

	for(int i = 0;i <= MAX_NUM_FPS_WEAPONS; i++)
	{
		if(fpsTrooper.weaponPositionModel[i] == NO_WEAPON)
			continue;

		if (fpsWeapons[fpsTrooper.weaponPositionModel[i]].firepointDisplacement)
		{
			vehicle->Weapons[i][0].Displacement = fpsWeapons[fpsTrooper.weaponPositionModel[i]].firepointDisplacement;
		}

		if (fpsWeapons[fpsTrooper.weaponPositionModel[i]].reloadSound)
		{
			pPtr = fpsWeapons[fpsTrooper.weaponPositionModel[i]].reloadSound;
			sndsPreloadSoundCache(pPtr);
		}

		if (fpsWeapons[fpsTrooper.weaponPositionModel[i]].reload2Sound)
		{
			pPtr = fpsWeapons[fpsTrooper.weaponPositionModel[i]].reload2Sound;
			sndsPreloadSoundCache(pPtr);
		}

       if (fpsWeapons[fpsTrooper.weaponPositionModel[i]].reload3Sound)
		{
			pPtr = fpsWeapons[fpsTrooper.weaponPositionModel[i]].reload3Sound;
			sndsPreloadSoundCache(pPtr);
		}

		if (fpsWeapons[fpsTrooper.weaponPositionModel[i]].recycleSound)
		{
			pPtr = fpsWeapons[fpsTrooper.weaponPositionModel[i]].recycleSound;
			sndsPreloadSoundCache(pPtr);
		}

		if (fpsWeapons[fpsTrooper.weaponPositionModel[i]].rgReloadSound)
		{
			pPtr = fpsWeapons[fpsTrooper.weaponPositionModel[i]].rgReloadSound;
			sndsPreloadSoundCache(pPtr);
		}

		if (fpsWeapons[fpsTrooper.weaponPositionModel[i]].deploySound)
		{
			pPtr = fpsWeapons[fpsTrooper.weaponPositionModel[i]].deploySound;
			sndsPreloadSoundCache(pPtr);
		}

		if (fpsWeapons[fpsTrooper.weaponPositionModel[i]].undeploySound)
		{
			pPtr = fpsWeapons[fpsTrooper.weaponPositionModel[i]].undeploySound;
			sndsPreloadSoundCache(pPtr);
		}

		if (fpsWeapons[fpsTrooper.weaponPositionModel[i]].sightAdjustSound)
		{
			pPtr = fpsWeapons[fpsTrooper.weaponPositionModel[i]].sightAdjustSound;
			sndsPreloadSoundCache(pPtr);
		}
	}

	// make sure all weapon sounds are preloaded
	for(int i = 0; i < VEH_MAX_MOUNTSOUNDS; i++)
	{
		if(*vehicle->VehicleType->MountSounds[i])
			sndsPreloadSoundCache((const SCHAR*)vehicle->VehicleType->MountSounds[i]);
	}

	weaponChangeStartTime = 0;

	fireDelay = 0;

	fpsTrooper.teleportToGround = TRUE; // initial spawn in, put them on the ground
	fpsTrooper.doGravity = FALSE;
	fpsTrooper.readingMap = FALSE;
	fpsTrooper.activeWeaponNumber = 0;
	fpsTrooper.lastActiveWeaponNumber = 0;
	fpsTrooper.autoswitchWeaponNumber = -1;
	fpsTrooper.highlightedWeaponNumber = fpsTrooper.activeWeaponNumber;
	fpsTrooper.riverExit = FALSE;

	currentWeaponModel = fpsTrooper.weaponPositionModel[fpsTrooper.activeWeaponNumber];
	lastWeaponModel = currentWeaponModel;

	currentFPSstate = IDLE;

	stateFlags = STATE_DEFAULT_MASK;
	stat2Flags = 0;
	prevEyeStateFlags = stateFlags & STATE_EYE_MASK;
	controlStateFlags = 0;
	controlBlockFlags = 0;
	currentCollisionFlags = 0;
	noFire = false;

	prevPose = FPS_POSITION_STAND;
	currentEyeHeight = fpFPSGetEyeAdjust(FPS_POSITION_STAND);

	currSkelControl		= nullptr;
	currWeapControl		= nullptr;
	currClipControl		= nullptr;
	currRGCupControl	= nullptr;

	mouseLookOn = 0;
	_collider.Reset();

	// reset our collider
	_updateFPSColliders();

	// clear out any ppos states at spawn
	_ppoLastHit = 0;

	if (fpsTrooper.leanTime > 0)
	{
		fpsTrooper.leanMinSpeed = (FLOAT32) sqrt(fpsTrooper.leanLeftHoriz*fpsTrooper.leanLeftHoriz + fpsTrooper.leanLeftVert*fpsTrooper.leanLeftVert)/fpsTrooper.leanTime;
		fpsTrooper.leanMaxSpeed = fpsTrooper.leanMinSpeed * 5.0f;
	}
	else
	{
		fpsTrooper.leanMinSpeed = 1;
		fpsTrooper.leanMaxSpeed = 10;
	}

	concentrating = false;
	fireStartTime = 0.0f;
	fireUpdateNo = 0;

	// make sure ammo is initialized so the initial HUD update is correct
	for(int i = 0; i < MAX_NUM_FPS_WEAPONS; i++)
	{
		fpsWeapons[fpsTrooper.weaponPositionModel[i]].clipBullets = 0;

		// rockets start out unloaded
		if(fpsWeapons[fpsTrooper.weaponPositionModel[i]].weaponType == FPS_WEAPONTYPE_ROCKET)
			continue;

		fpFPSUpdateAmmo(i, &vehicle->Weapons[i][0]);
	}

	// reset auxillary stuff
	for(int i = 0; i < NUM_WEAPONS; i++)
	{
		//showAux[i][FPS_AUX_RGCUP]						= false;
		//showAux[i][FPS_AUX_RGGRENADE]					= false;
		fpsWeapons[i].auxEnable[FPS_AUX_RGCUP]			= 0;
		fpsWeapons[i].auxEnable[FPS_AUX_RGGRENADE]		= 0;
	}

	// always on for now
	fpFPSEnableBinoculars(1);

	// reset the ragdoll cam position and lookat
	fpTPSDeathCameraReset();
}

//
// IMPORTANT - this gets called for every instance of every trooper, not just FPS troopers
//   .. in other words, no setting fpsTrooper data in here!
//
void fpFPSDetermineTrooperLoadout (vehVehicle*vehicle,
								   UINT32 vehicleTypeNum,
								   SINT32 binocsEnabled,
								   SINT32* weaponArray,
								   SINT32* country,
								   SINT32* weaponOfLastResort,
								   LOGICAL* isParatroop)
{
	// reset the weapon array
	memset (weaponArray, NO_WEAPON, sizeof(fpsWeaponTypes)*MAX_NUM_FPS_WEAPONS);

	// set the default atp multiplier
	loadoutATPMultiplier = 1.0f;

	// setup the map
	weaponArray[MAP_WEAPON_NUMBER] = MAP;

	// setup the buildtool
	//weaponArray[BUILDTOOL_WEAPON_NUMBER] = BUILD_TOOL;

	// setup binocs if the player gets em
	if(binocsEnabled)
		weaponArray[BINOCS_WEAPON_NUMBER] = BINOCS;

	if(!vehicle || !vehicle->VehicleType) return;

	// lookup the trooper's loadout based on its vehicle id
	fpFPSTrooperLoadout* loadout = fpFPSGetTrooperLoadout((const char*)vehicle->VehicleType->Id);

	if(loadout)
	{
		*country = loadout->arms;

		loadoutATPMultiplier = loadout->atpMultiplier;

		for(int i = 0; i < (int)loadout->weapons.size(); i++)
		{
			fpFPSLoadoutWeapon& weapon = loadout->weapons.at(i);

			if(SCEN_MGR->IsActive() && (fpsWeaponTypes)weapon.weapon == BUILD_TOOL)
				continue;

			weaponArray[weapon.index] = (fpsWeaponTypes)weapon.weapon;

			if(weapon.aux)
			{
				showAux[weaponArray[weapon.index]][FPS_AUX_RGCUP]						= true;
				showAux[weaponArray[weapon.index]][FPS_AUX_RGGRENADE]					= true;
				fpsWeapons[weaponArray[weapon.index]].auxAvailable[FPS_AUX_RGCUP]		= 1;
				fpsWeapons[weaponArray[weapon.index]].auxAvailable[FPS_AUX_RGGRENADE]	= 1;
			}

			if(weapon.lastResort && weaponOfLastResort)
				*weaponOfLastResort = weapon.index;
		}

		if(weaponOfLastResort)
			currentArms = (fpsInfantryTypes)(*country);

		if(isParatroop)
			*isParatroop = loadout->isPara;
	}
}

LOGICAL fpFPSLoadModels (vehVehicle*vehicle)
{
	OBJECT*			fps_object;
	int				i, j;

	//fpFPSDataInit ();

	// first thing, prep the object for being a FPS
	fps_object = vehicle->VehicleObjectData->PlaneObject->child;

	vehicle->VehicleObjectData->PlaneObject->flag2 |= OBJECT_ISFPS;

	fps_object->flag2 |= OBJECT_ISFPS;
	// ok, now do the FPS specific stuff
	fpFPSInit(vehicle);

	skeletonModel = gr2Model::Load(skel_model, nullptr, nullptr, 1);
	armsModels[currentArms] = gr2Model::Load(arm_models[currentArms], skeletonModel, nullptr, 1);
	legsModels[currentArms] = gr2Model::Load(leg_models[currentArms], nullptr, nullptr, 1);

	if (default_arms_tex_names[currentArms])
	{
		SINT32		baseArmTexID, replacementArmTexID;

		armTexModel = gr2Model::Load(replacement_arms_file, nullptr, nullptr, 1);

		baseArmTexID = piGetNamedTexture (default_arms_tex_names[currentArms]);
		replacementArmTexID = piGetNamedTexture (replacement_arms_tex_names[currentArms]);

		if ((baseArmTexID > 0) && (replacementArmTexID > 0))
			gr2ModelFindAndReplaceTexture (armsModels[currentArms], baseArmTexID, replacementArmTexID);
	}

	for (i=0; i < MAX_NUM_FPS_WEAPONS; i++)
	{
		fpsWeaponTypes	weaponID;

		weaponID = fpsTrooper.weaponPositionModel[i];

		if (weaponID == NO_WEAPON)
			continue;

		if (weaponID == BINOCS)
			continue;

		if (weaponID == MAP)
			continue;

		if (fpsWeapons[weaponID].weaponModel)
		{
			//int modelIndex = (strstr(fpsWeapons[weaponID].weaponModel, "gunmesh-g30")) ? 1: 0;
            weaponModels[weaponID] = gr2Model::Load(fpsWeapons[weaponID].weaponModel, nullptr, nullptr, 1); //, modelIndex
            gr2ModelSetEventHandler(weaponModels[weaponID], (OBJECT*)nullptr, gr2AnimHandleEvent);

			// hide any hidden meshes
			if(fpsWeapons[weaponID].hiddenMeshes)
			{
				char line[128+1];
				strsafecpy(line, fpsWeapons[weaponID].hiddenMeshes);

				char* meshName = strtok(line," ,");
				while(meshName != nullptr)
				{
					gr2SetMeshActiveByName(weaponModels[weaponID], meshName, false);
					meshName = strtok(nullptr, " ,");
				}
			}
		}

		if (fpsWeapons[weaponID].clipModel)
		{
			secondaryModels[weaponID] = gr2Model::Load(fpsWeapons[weaponID].clipModel, nullptr, nullptr, 1);
        }

		for (j=0; j < FPS_NUM_AUX_MODELS; j++)
		{
			if (fpsWeapons[weaponID].auxModel[j])
			{
				// model index?
				auxModels[weaponID][j] = gr2Model::Load(fpsWeapons[weaponID].auxModel[j], nullptr, nullptr, 1);
                gr2SetModelActive(auxModels[weaponID][j], false);
			}
		}

		// all animations are by weapon, so load them here inside the weapon loop

		if (weap_default[weaponID][0])
		{
			weapDefaultAnim[weaponID] = gr2LoadAnimation (weap_default[weaponID], 1);
		}

		if (weap_empty_default[weaponID][0])
		{
			weapEmptyDefaultAnim[weaponID] = gr2LoadAnimation (weap_empty_default[weaponID], 1);
		}

		if (clip_default[weaponID][0])
		{
			clipDefaultAnim[weaponID] = gr2LoadAnimation (clip_default[weaponID], 1);
		}

		for (j=0; j < NUM_WEAP_STATES; j++)
		{
			if (arms_anim_files[weaponID][j][0] != 0)
			{
				armsAnims[weaponID][j] = gr2LoadAnimation (arms_anim_files[weaponID][j], 1);
			}

			if (weap_anim_files[weaponID][j][0] != 0)
			{
				weapAnims[weaponID][j] = gr2LoadAnimation (weap_anim_files[weaponID][j], 1);
			}

			if (clip_anim_files[weaponID][j][0] != 0)
			{
				clipAnims[weaponID][j] = gr2LoadAnimation (clip_anim_files[weaponID][j], 1);
			}
		}
	}

	armsModel = armsModels[currentArms];
	weapModel = weaponModels[currentWeaponModel];
	clipModel = secondaryModels[currentWeaponModel];
	for (i=0; i < FPS_NUM_AUX_MODELS; i++)
	{
		auxModel[i] = auxModels[currentWeaponModel][i];
	}
	legsModel = nullptr;

	if (fpsWeapons[currentWeaponModel].sniperGunSight)
	{
		fpsWeapons[currentWeaponModel].sniperScope = TRUE;
        persLoadInstGunsight(fpsWeapons[currentWeaponModel].sniperGunSight);
	}
	else fpsWeapons[currentWeaponModel].sniperScope = FALSE;

	if (!fpFPSAttachWeapon (currentWeaponModel))
	{
		char errorMessage[255];

		snprintf (errorMessage, 254, "fpFPSLoadModels: fpFPSAttachWeapon failed %d", currentWeaponModel);
		piExitProgram (_FL, errorMessage);
	}

	fpFPSStartAnimationState (currentWeaponModel, IDLE);

	fpsTrooper.initialized = TRUE;

	return TRUE;
}

LOGICAL fpFPSAttachWeapon (fpsWeaponTypes weapon)
{
	SINT32	i;

	if (!gr2AttachModel (weapModel, fpsWeapons[weapon].weaponConnect, skeletonModel, fpsWeapons[weapon].weaponConnectTo))
	{
		Write_Log (LOG_FPS, LOG_INFO, "Failed to attach %d to skeleton", weapon);
		return FALSE;
	}

	if (fpsWeapons[weapon].clipConnect && fpsWeapons[weapon].clipConnectTo)
	{
		if (!gr2AttachModel (clipModel, fpsWeapons[weapon].clipConnect, weapModel, fpsWeapons[weapon].clipConnectTo))
		{
			Write_Log (LOG_FPS, LOG_INFO, "Failed to attach clip to gun %d", weapon);
			return FALSE;
		}
	}

	for (i=0; i < FPS_NUM_AUX_MODELS; i++)
	{
		// we don't attache RGCUPs until they are deployed
		if (i != FPS_AUX_RGCUP && fpsWeapons[weapon].auxConnect[i] && fpsWeapons[weapon].auxConnectTo[i])
		{
			SCHAR* auxConnectToBone;

			if ((fpsWeapons[weapon].auxEnable[i]) && (fpsWeapons[weapon].auxAltConnectTime))
			{
				auxConnectToBone = fpsWeapons[weapon].auxConnectToAlt[i];
			}
			else
			{
				auxConnectToBone = fpsWeapons[weapon].auxConnectTo[i];
			}

			if (!gr2AttachModel (auxModel[i], fpsWeapons[weapon].auxConnect[i], weapModel, auxConnectToBone))
			{
				Write_Log (LOG_FPS, LOG_INFO, "Failed to attach aux %d to gun %d", i, weapon);
				return FALSE;
			}
		}
	}

	return TRUE;
}

LOGICAL fpFPSDetachWeapon (fpsWeaponTypes weapon)
{
	SINT32	i;

	if (fpsWeapons[weapon].clipConnect && fpsWeapons[weapon].clipConnectTo)
	{
		gr2DetachModel (clipModel);
	}

	for (i=0; i < FPS_NUM_AUX_MODELS; i++)
	{
		if (fpsWeapons[weapon].auxConnect[i] && fpsWeapons[weapon].auxConnectTo[i])
		{
			gr2DetachModel (auxModel[i]);
		}
	}

	gr2DetachModel (weapModel);

	return TRUE;
}

void fpFPSInitDeployedState()
{
	LOGICAL			pointFound;
	piMATRIX		deployMatrix;

	if (fpsWeapons[currentWeaponModel].deployPivotBone)
	{
		// if we're not deployed yet .. wait
		if (currentFPSstate != DEPLOYED)
			return;

		pointFound = gr2GetRawBonePosition (weapModel, fpsWeapons[currentWeaponModel].deployPivotBone, &deployMatrix);

		if (!pointFound)
		{
			Write_Log (LOG_FPS,LOG_INFO, "fpFPSInitDeployedState():  Failed to find deploy point for current model");
			return;
		}

		deployPositionRaw.x = deployMatrix._41;
		deployPositionRaw.y = deployMatrix._42;
		deployPositionRaw.z = deployMatrix._43;

		deployPosition.x = deployPositionRaw.x;
		deployPosition.y = deployPositionRaw.y;
		deployPosition.z = deployPositionRaw.z + fpFPSGetHeight();

		//aimVertAngle = 0;
		aimVerticalDeployedPosition = 0;
		aimHorizontalDeployedPosition = 0;

		stateFlags |= STATE_NO_ARM_SWAY;
	}
	else if (false && (fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_RIFLE) && (fpsWeapons[currentWeaponModel].auxAvailable[FPS_AUX_RGCUP]))
	{
		fpsWeapons[currentWeaponModel].auxEnable[FPS_AUX_RGGRENADE] = 0;
	}

	deployedStateInitialized = true;
}

static fpsWeaponStates _determineNextRifleState()
{
    switch(currentFPSstate)
    {
    case RELOAD:
        {
            if(gr2IsControlComplete(currSkelControl))
            {
                int clipbullets = fpsWeapons[currentWeaponModel].clipBullets.Get();
                int rounds = (int)currentGameWeapon->WeaponType->NumRoundsInClip;
				int remainder = currentGameWeapon->NumRoundsAmmoLeft[currentGameWeapon->AmmoTypeInUse.Get()];

                if(clipbullets < std::min(rounds, remainder))
                {
                    return RELOAD2;
                }
                else
                {
                    return RELOAD3;
                }
            }

            break;
        }
    case RELOAD2:
        {
            if(gr2IsControlComplete(currSkelControl))
            {
				if(currentGameWeapon->NumRoundsAmmoLeft[currentGameWeapon->AmmoTypeInUse.Get()] == -1)
                {
                    fpsWeapons[currentWeaponModel].clipBullets = currentGameWeapon->WeaponType->NumRoundsInClip;
                    return RELOAD3;
                }

                ++fpsWeapons[currentWeaponModel].clipBullets;

                int clipbullets = fpsWeapons[currentWeaponModel].clipBullets.Get();
                int rounds = (int)currentGameWeapon->WeaponType->NumRoundsInClip;
                int remainder = currentGameWeapon->NumRoundsAmmoLeft[currentGameWeapon->AmmoTypeInUse.Get()];

                if(clipbullets < std::min(rounds, remainder))
                {
                    fpFPSStartAnimationState(currentWeaponModel, RELOAD2);
                    return currentFPSstate;
                }
                else
                {
                    return RELOAD3;
                }
            }

            break;
        }
    case RELOAD3:
        {
            if(gr2IsControlComplete(currSkelControl))
            {
                currentGameWeapon->ReloadStartTime = 0;
                stateFlags &= ~STATE_RELOADING;
                return NUM_WEAP_STATES;
            }

            break;
        }
    default:
            // we only care about reload states
            break;
    }

    return currentFPSstate;
}

fpsWeaponStates fpFPSDetermineNextState ()
{
	mlVehicleData*	vehicleData = persGetPersonaModelDataPtr();
	assert( vehicleData != nullptr );

	fpsWeaponStates	newState;
	UINT32			animationStateFlags;
	FLOAT64			currentTime;
	FLOAT32			currentATP;

	currentTime = frametime / 1000.0;

	if (resupplyTarget)
	{
		if (frametime - resupplyTime > 2000)
		{
			resupplyTarget = 0;
			resupplyTime = 0;
		}
	}

	if (stateFlags & STATE_NOLEAN_MASK)
	{
		fpFPSLeanOff();
	}

	if (fpsWeapons[currentWeaponModel].suppressAim)
	{
		stateFlags &= ~STATE_AIM;
	}

	if (stateFlags & STATE_EMOTE_PLAYING)
	{
		if (currentTime >= (emoteStartTime + emoteLength))
		{
			if ( !(stateFlags & (STATE_DEAD | STATE_SIT)) )
			{
				stateFlags &= ~(STATE_EMOTE_PLAYING | STATE_NO_MOVEMENT);
			}
			else
				stateFlags &= ~STATE_EMOTE_PLAYING;

			emoteID = 0;
			emoteType = TPS_EMOTETYPE_NONE;
			emoteStartTime = 0;
			emoteLength = 0;
		}
		else
		{
			if (emoteType == TPS_EMOTETYPE_FULLBODY)
				return LOWER;
		}
	}

	if (fpsTrooper.parachuteOKToOpen)
	{
		stateFlags |= (STATE_NO_ARM_SWAY | STATE_NO_MOVEMENT);
		return LOWER;
	}

	bool isMoving = fpFPSIsMoving();

	// kick off binoculars if they're moving
	if (currentWeaponModel == BINOCS && fpsTrooper.autoswitchWeaponNumber == -1 && isMoving)
	{
		fpFPSWeaponChange(fpsTrooper.lastActiveWeaponNumber, FALSE);
		//fpFPSWeaponAutoSwitch();
	}

	if (stateFlags & STATE_KNIFE_ATTACKING)
	{
		if (currentTime > (fireStartTime + 1.0))
		{
			stateFlags &= ~STATE_KNIFE_ATTACKING;
		}
	}

	if (stateFlags & STATE_DEPLOYED)
	{
		if (!deployedStateInitialized)
		{
			fpFPSInitDeployedState();
		}
	}
	else
	{
		deployedStateInitialized = false;
	}

	// have to pay attention to the gun while deploying
	if (stateFlags & STATE_DEPLOYING)
	{
		stateFlags &= ~(STATE_LOOK_RIGHT | STATE_LOOK_LEFT | STATE_AIM);
	}

	// no head turn while throwing a grenade or priming a satchel or attacking with a knife
	if (stateFlags & STATE_HOLD_FIRE)
	{
		stateFlags &= ~(STATE_LOOK_RIGHT | STATE_LOOK_LEFT);
	}

	// when reloading a satchel and the weapon is down, allow sprinting keeping the
	// reload going until sprint stops
	if(fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_SATCHEL &&
		HAS_ALL_BITS(stateFlags, STATE_RELOADING | STATE_MOVE_FAST) &&
		targetPose != FPS_POSITION_PRONE)
	{
		return currentFPSstate;
	}
	else if(fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_BUILD &&
		HAS_ALL_BITS(stateFlags, STATE_RELOADING | STATE_MOVE_FAST) &&
		targetPose != FPS_POSITION_PRONE)
	{
		return currentFPSstate;
	}

	// no sprinting while doing the stuff specified in the nosprint mask
	if ((stateFlags & STATE_NOSPRINT_MASK) && (stateFlags & STATE_MOVE_FAST))
	{
		stateFlags &= ~(STATE_MOVE_FAST | STATE_NO_ARM_SWAY);
		stateFlags |= STATE_MOVE_SLOW;
	}

	// if we're swimming, force them to prone .. unless we're sitting (i.e. riding a boat)
	if ((stateFlags & STATE_SWIMMING) && !(stateFlags & STATE_SIT))
	{
		// Flags we need to remove
		static const UINT32 _FLAGS_TO_REMOVE = STATE_EYE_STAND | STATE_EYE_CROUCH | STATE_AIM;
		static const UINT32 _FLAGS_TO_ADD = STATE_EYE_PRONE | STATE_RELAXED;

		if (stateFlags & STATE_MOVE_FAST)
		{
			stateFlags &= ~(_FLAGS_TO_REMOVE | STATE_MOVE_FAST | STATE_NO_ARM_SWAY);
			stateFlags |= _FLAGS_TO_ADD | STATE_MOVE_SLOW;
		}
		else
		{
			stateFlags &= ~_FLAGS_TO_REMOVE;
			stateFlags |= _FLAGS_TO_ADD;
		}

		controlStateFlags &= ~FPS_CROUCH;
		controlStateFlags |= FPS_PRONE;
	}

	// handle the eye point state changes
	if ((prevEyeStateFlags != (stateFlags & STATE_EYE_MASK)) && !(stateFlags & STATE_SIT))
	{
		// crouch first
		if (stateFlags & STATE_EYE_CROUCH)
		{
			if (fpFPSCheckNewPose(POSE_CROUCH) == 0)
			{
				if (targetPose != FPS_POSITION_CROUCH)
					prevPose = targetPose;
				targetPose = FPS_POSITION_CROUCH;
//				Write_Log (LOG_FPS, LOG_INFO, "%d, %d", prevPose, targetPose);
				prevEyeStateFlags = stateFlags & STATE_EYE_MASK;
			}
			else
			{
				// if crouch not allowed, turn off crouch in control state
				controlStateFlags &= ~FPS_CROUCH;
				// use the previousEyeStateFlags to set the eye to what it belongs at
				stateFlags |= prevEyeStateFlags;
				// then turn off crouch
				stateFlags &= ~STATE_EYE_CROUCH;
				if (prevEyeStateFlags & STATE_EYE_PRONE)
				{
					controlStateFlags |= FPS_PRONE;
				}
				// and shake the screen (you bad player you!)
				persShakeHead();
			}
		}
		else if (stateFlags & STATE_EYE_STAND)
		{
			if (fpFPSCheckNewPose(POSE_STAND) == 0)
			{
				if (targetPose != FPS_POSITION_STAND)
					prevPose = targetPose;
				targetPose = FPS_POSITION_STAND;
//				Write_Log (LOG_FPS, LOG_INFO, "%d, %d", prevPose, targetPose);
				prevEyeStateFlags = stateFlags & STATE_EYE_MASK;
			}
			else
			{
				// stand failed, revert to where we were before
				stateFlags &= ~STATE_EYE_MASK;
				stateFlags |= prevEyeStateFlags & STATE_EYE_MASK;

				if (prevEyeStateFlags & STATE_EYE_PRONE)
				{
					controlStateFlags |= FPS_PRONE;

					// this is necessary to translate an attempt to sprint to a crawl
					if (stateFlags & STATE_MOVE_FAST)
					{
						stateFlags &= ~STATE_MOVE_FAST;
						stateFlags |= STATE_MOVE_SLOW;
						controlStateFlags &= ~FPS_SPRINT;
					}
				}
				else if (prevEyeStateFlags & STATE_EYE_CROUCH)
				{
					controlStateFlags |= FPS_CROUCH;
				}
				else
				{
					Write_Log (LOG_FPS, LOG_INFO, "Unexpected state?  %d %d %d", prevEyeStateFlags, stateFlags, controlStateFlags);
				}

				// and shake the screen (you bad player you!)
				persShakeHead();
			}
		}
		else if (stateFlags & STATE_EYE_PRONE)
		{
 			// swimming forces the prone state, regardless of collision check
            // Check for POSE_STAND to allow going prone on berms
			if ((stateFlags & STATE_SWIMMING) || (fpFPSCheckNewPose(POSE_STAND) == 0))
			{
				if (targetPose != FPS_POSITION_PRONE)
					prevPose = targetPose;
				targetPose = FPS_POSITION_PRONE;
//				Write_Log (LOG_FPS, LOG_INFO, "%d, %d", prevPose, targetPose);
				prevEyeStateFlags = stateFlags & STATE_EYE_MASK;
			}
			else
			{
				// prone failed, revert to where we were before
				stateFlags &= ~STATE_EYE_MASK;
				stateFlags |= prevEyeStateFlags & STATE_EYE_MASK;

				if (prevEyeStateFlags & STATE_EYE_STAND)
				{
					controlStateFlags &= ~FPS_PRONE;
				}
				else if (prevEyeStateFlags & STATE_EYE_CROUCH)
				{
					controlStateFlags |= FPS_CROUCH;
				}
				else
				{
					Write_Log (LOG_FPS, LOG_INFO, "Unexpected state?  %d %d %d", prevEyeStateFlags, stateFlags, controlStateFlags);
				}

				// and shake the screen (you bad player you!)
				persShakeHead();
			}
		}
		else
		{
			Write_Log (LOG_FPS, LOG_INFO, "*************Unidentified pose!**************");
			prevPose = targetPose;
			targetPose = FPS_POSITION_STAND;
			Write_Log (LOG_FPS, LOG_INFO, "%d, %d", prevPose, targetPose);
		}

		if(targetPose != prevPose)
			stat2Flags |= STAT2_POSE_CHANGE;
	}

	// check to see if we're trying to autoswitch to another weapon if fire isn't held down
	if (fpsTrooper.autoswitchWeaponNumber != -1 && (fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_GRENADE || !fireDown))
	{
        static fpsWeaponStates old = NUM_WEAP_STATES;

        if(currentFPSstate != old)
        {
            Write_Debug("%d = currentFPSstate", currentFPSstate);
        }

        old = currentFPSstate;

		// HACK: First check is to avoid infinite head-shaking when trying to switch from a loaded rocket launcher.
        if ( !(fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_ROCKET && fpsWeapons[currentWeaponModel].clipBullets != 0) &&
		     fpFPSIsWeaponSwitchOK(fpsTrooper.autoswitchWeaponNumber))
		{
			fpFPSWeaponChange (fpsTrooper.autoswitchWeaponNumber, 1);
			//return currentFPSstate; // go ahead and let it drop through; this fixes a binocs zoom issue
		}
//		Write_Log (LOG_FPS, LOG_INFO, "Failed to autoswitch to weapon %d", fpsTrooper.autoswitchWeaponNumber);
	}

	// special handling for sprint
	if (stateFlags & STATE_MOVE_FAST)
	{
		persGetHealthStats(nullptr, &currentATP, nullptr, nullptr);

		// if we're sprinting already, let them burn down to atpSprintMinimum ..
		// don't let them start sprinting, though, until they're at atpSprintAllowAgain
		if (currentFPSstate == SPRINT)
		{
			if (currentATP < fpsTrooper.atpSprintMinimum)
			{
				stateFlags &= ~( STATE_MOVE_FAST | STATE_NO_ARM_SWAY);
				stateFlags |= STATE_MOVE_SLOW;
				controlStateFlags &= ~FPS_SPRINT;
//				Write_Log (LOG_FPS, LOG_INFO, "Ok, I'm pooped!");
			}
			else
			{
				stateFlags &= ~STATE_AIM;
			}
		}
		else if (currentATP < fpsTrooper.atpSprintAllowAgain)
		{
			stateFlags &= ~STATE_MOVE_FAST;
			stateFlags |= STATE_MOVE_SLOW;
			controlStateFlags &= ~FPS_SPRINT;
//			Write_Log (LOG_FPS, LOG_INFO, "Not Yet!");
		}
		// ok, we're really sprinting, turn off aim
		else
		{
			stateFlags &= ~STATE_AIM;
		}
	}

	// no movement while prone with a primed grenade
	// also, no movement while deployed
	if (((stateFlags & STATE_EYE_PRONE) && (fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_GRENADE) && (stateFlags & (STATE_HOLD_FIRE | STATE_FIRE))) ||
		((stateFlags & STATE_DEPLOYED) && fpsWeapons[currentWeaponModel].staticDeploy))
	{
		stateFlags &= ~(STATE_MOVE_SLOW | STATE_STRAFE_LEFT | STATE_STRAFE_RIGHT | STATE_MOVE_BACKWARD);
		stateFlags |= STATE_MOVE_STILL;
	}

	// can't aim while moving prone, so check for prone & aim, then move states
	if (HAS_ALL_BITS(stateFlags, STATE_AIM | STATE_EYE_PRONE))
	{
		if (HAS_ANY_BITS(stateFlags, STATE_MOVE_SLOW | STATE_MOVE_BACKWARD | STATE_STRAFE_LEFT | STATE_STRAFE_RIGHT))
		{
			stateFlags &= ~STATE_AIM;
		}

		// rockets cant aim while prone
		if(fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_ROCKET && currentWeaponModel != PIAT)
		{
			stateFlags &= ~STATE_AIM;
			persShakeHead("common.headshake.noproneaim");
		}
	}
	// can't aim while moving prone, so check for prone & aim, then move states
	if (HAS_ALL_BITS(stateFlags, STATE_AIM | STATE_EYE_STAND))
	{
		// piats cant aim while standing
		if(currentWeaponModel == PIAT)
		{
			stateFlags &= ~STATE_AIM;
			persShakeHead("common.headshake.nostandaim");
		}
	}

	// if you are moving with a rocket, no aiming
	if(HAS_ANY_BITS(stateFlags, STATE_MOVE_SLOW | STATE_MOVE_BACKWARD | STATE_STRAFE_LEFT | STATE_STRAFE_RIGHT) &&
	   fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_ROCKET)
	{
		stateFlags &= ~STATE_AIM;
	}

	// LMGs have some special allowances for "aim"
	if (fpsWeapons[currentWeaponModel].weaponType != FPS_WEAPONTYPE_LMG)
	{
		// no aiming while looking left or right
		// can't aim while going backwards
		if (stateFlags & (STATE_LOOK_RIGHT | STATE_LOOK_LEFT | STATE_MOVE_BACKWARD))
		{
			stateFlags &= ~STATE_AIM;
		}

		// if aiming and moving forward, turn on walk
		if ((stateFlags & STATE_AIM) && (stateFlags & (STATE_MOVE_SLOW | STATE_MOVE_BACKWARD)))
		{
			stateFlags |= STATE_WALK;
		}
	}

	// can't aim while holding grenade or satchel
	if ((fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_SATCHEL) ||
		(fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_GRENADE) ||
		(fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_BUILD))
	{
		stateFlags &= ~STATE_AIM;
	}

	// can't aim while reloading either
	if (stateFlags & STATE_RELOADING)
	{
		stateFlags &= ~STATE_AIM;
	}

	// can't look through the scope while moving
	if((stateFlags & STATE_AIM) && fpsWeapons[currentWeaponModel].sniperScope && fpFPSIsMoving())
	{
		stateFlags &= ~STATE_AIM;
	}

#if(1)
	// lmg specific stuff here .. basically, handle the fact that aim becomes ready, and everything else is relaxed
	// also, kick off leans here if aim is on
	if (fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_LMG)
	{
		if((stateFlags & STATE_AIM))
			fpFPSLeanOff();
	}
#endif

	if (fpsTrooper.highlightTime > 0)
	{
		if (currentTime - fpsTrooper.highlightTime > 1.0)
		{
			if (fpsTrooper.highlightedWeaponNumber != fpsTrooper.activeWeaponNumber)
				fpFPSWeaponChange (fpsTrooper.highlightedWeaponNumber, FALSE);
			fpsTrooper.highlightTime = 0;
		}
	}

	if (!(vehicleData->PersonData->Flags & fpFLAGS_FORCE_EXTERNAL))
	{
        if ((currentWeaponModel == BINOCS)||( (fpsWeapons[currentWeaponModel].sniperScope) && ( currentFPSstate == AIM)))
		{
			if (stateFlags & STATE_WEAPON_SWITCHING || isMoving)
			{
				fpsTrooper.desiredFOV = fpsTrooper.defaultFOV;
				if (FieldOfView != fpsTrooper.desiredFOV)
				{
					FLOAT64			aimError;
					mlVehicleData*	pVehicleData = persGetPersonaModelDataPtr();
					aimError = (mathGetFloat32ChaoticNumber() - 0.5) * 6.0;
					pVehicleData->VehicleAtt.Yaw += aimError * PI/180.0;
					aimError = (mathGetFloat32ChaoticNumber() - 0.5) * 6.0;
					aimVertAngle += aimError;

					persSetNormalView (fpsTrooper.desiredFOV);
				}
			}
		}
		else if	(((stateFlags & STATE_AIM) && ((!fpsWeapons[currentWeaponModel].deployPivotBone) || currentWeaponModel == BAR || currentWeaponModel == FG42G)) ||
				(((fpsWeapons[currentWeaponModel].deployPivotBone) && ((stateFlags & (STATE_DEPLOYED|STATE_RELOADING|STATE_LOOK_LEFT|STATE_LOOK_RIGHT|STATE_DEPLOYING|STATE_NEEDS_RECYCLE|STAT2_NEEDS_RELOAD)) == STATE_DEPLOYED))))
		{
//			if ((currentFPSstate == RELOAD) || (currentFPSstate == RECYCLE) || (currentFPSstate == RELOAD2) || (currentFPSstate == RELOAD3) || (currentFPSstate == THROW))
			if (currentFPSstate == RECYCLE)
			{
				fpsTrooper.desiredFOV = fpsTrooper.defaultFOV;
			}
			else
			{
#if AIMING_WEAPONS
				fpsTrooper.desiredFOV = fpsWeapons[currentWeaponModel].aimFOV/10.0f;
#else
				fpsTrooper.desiredFOV = fpsWeapons[currentWeaponModel].aimFOV;
#endif
			}
		}
		else if (FieldOfView != fpsTrooper.defaultFOV)
		{
			fpsTrooper.desiredFOV = fpsTrooper.defaultFOV;
		}
	}

	// if we need to reload, try to reload; if we still need to reload (i.e. reload didn't happen)
	// deny state changes
	if (stat2Flags & STAT2_NEEDS_RELOAD)
		fpFPSReload(currentGameWeapon);

	if (stat2Flags & STAT2_NEEDS_RELOAD)
		return currentFPSstate;

	if (stateFlags & STATE_WEAPON_SWITCHING)
	{
		if (weaponChangeStartTime && (currentTime - weaponChangeStartTime) > fpsTrooper.weaponChangeTime)
		{
			weaponChangeStartTime = 0;

			if (currentWeaponModel != BINOCS)
			{
				// based on our current state, clean up things
				if (!fpFPSCleanupAnimationState ())
					return currentFPSstate;

				if (!fpFPSDetachWeapon (currentWeaponModel))
				{
					Write_Log (LOG_FPS, LOG_INFO, "Failed to detach weapon!");
					return currentFPSstate;
				}
			}

			fpPPOSet(nullptr);

			if (fpsTrooper.weaponPositionModel[fpsTrooper.nextWeaponNumber] != BINOCS)
			{
				currentGameWeapon = &(((vehVehicle*)(vehicleData->VehicleMasterPtr))->Weapons[fpsTrooper.nextWeaponNumber][0]);

				lastWeaponModel = currentWeaponModel;
				currentWeaponModel = fpsTrooper.weaponPositionModel[fpsTrooper.nextWeaponNumber];
				fpsTrooper.lastActiveWeaponNumber = fpsTrooper.activeWeaponNumber;
				fpsTrooper.activeWeaponNumber = fpsTrooper.nextWeaponNumber;
				fpsTrooper.nextWeaponNumber = -1;

				if (currentWeaponModel == BUILD_TOOL)
				{
					fpPPOReset();
				}

				if(fpsWeapons[currentWeaponModel].clipBullets == -1)
					fpFPSUpdateAmmo(fpsTrooper.activeWeaponNumber, currentGameWeapon);

				// calculate the convergence angle for the new weapon and ammo;
				_calculateConvergenceAngle();

				weapModel = weaponModels[currentWeaponModel];
				clipModel = secondaryModels[currentWeaponModel];
				for (SINT32 i = 0; i < FPS_NUM_AUX_MODELS; i++)
				{
					auxModel[i] = auxModels[currentWeaponModel][i];
				}

				showArms = true;
				showGun = true;
				showClip = true;

				if (!fpFPSAttachWeapon (currentWeaponModel))
				{
					Write_Log (LOG_FPS, LOG_INFO, "Failed to attach weapon!");
					return currentFPSstate;
				}

				if (fpsWeapons[currentWeaponModel].auxEnable[FPS_AUX_RGCUP])
				{
					// reattach the cup
					gr2AttachModel(auxModel[FPS_AUX_RGCUP],
								   fpsWeapons[currentWeaponModel].auxConnect[FPS_AUX_RGCUP],
								   weapModel,
								   fpsWeapons[currentWeaponModel].auxConnectToAlt[FPS_AUX_RGCUP]);
					stateFlags |= STATE_DEPLOYED;
				}

				if(lastWeaponModel == BINOCS &&
				   fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_LTMORTAR &&
				   (stateFlags & STATE_DEPLOYED))
				{
					// reset the view
					aimVertAngle = 0.0f;
					aimVerticalDeployedPosition = 0.0f;
				}

				// make sure this gets reset because different
				// weapons use it differently
				fireStartTime = 0.0f;

				// let the tutorial manager know we've switched weapons
				TUT_MGR->DoEvent(TUT_EVENT_WEAPON_SWITCH, currentGameWeapon->WeaponType, &currentWeaponModel);

				if (!fpFPSStartAnimationState (currentWeaponModel, currentFPSstate))
					return currentFPSstate;
			}
			else // binocs don't do any granny stuff
			{
				FLOAT64			aimError;
				mlVehicleData*	pVehicleData = persGetPersonaModelDataPtr();
				aimError = (mathGetFloat32ChaoticNumber() - 0.5) * 6.0;
				pVehicleData->VehicleAtt.Yaw += aimError * PI/180.0;
				aimError = (mathGetFloat32ChaoticNumber() - 0.5) * 6.0;
				aimVertAngle += aimError;

				showArms = false;
				showGun = false;
				showClip = false;

				lastWeaponModel = currentWeaponModel;
				currentWeaponModel = fpsTrooper.weaponPositionModel[fpsTrooper.nextWeaponNumber];
				fpsTrooper.lastActiveWeaponNumber = fpsTrooper.activeWeaponNumber;
				fpsTrooper.activeWeaponNumber = fpsTrooper.nextWeaponNumber;
				fpsTrooper.nextWeaponNumber = -1;
			}

			stateFlags &= ~STATE_WEAPON_SWITCHING;
		}
		else
			return currentFPSstate;
	}

	// reload and recycle and throw need to be allowed to finish
	// deploy states need to finish too
	if (currentFPSstate == RELOAD || currentFPSstate == RECYCLE || currentFPSstate == RELOAD2
		|| currentFPSstate == RELOAD3 || currentFPSstate == THROW || currentFPSstate == DEPLOYED_RELOAD)
	{
		if (fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_LTMORTAR)
		{
			return currentFPSstate;
		}
		else if (fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_SATCHEL)
		{
			if	((currentGameWeapon->ReloadStartTime == 0) ||
				((currentTime - currentGameWeapon->ReloadStartTime) > currentGameWeapon->WeaponType->ReloadTime))
			{
				currentGameWeapon->ReloadStartTime = 0;
				showGun = fpsWeapons[currentWeaponModel].clipBullets > 0;

				stateFlags &= ~STATE_RELOADING;
			}
			else
				return currentFPSstate;
		}
		else if (fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_BUILD)
		{
			if	((currentGameWeapon->ReloadStartTime == 0) ||
				((currentTime - currentGameWeapon->ReloadStartTime) > currentGameWeapon->WeaponType->ReloadTime))
			{
				currentGameWeapon->ReloadStartTime = 0;
				showGun = fpsWeapons[currentWeaponModel].clipBullets > 0;

				stateFlags &= ~STATE_RELOADING;
			}
			else
				return currentFPSstate;
		}
		else if (fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_GRENADE)
		{
			if (currentFPSstate == THROW)
			{
				if (gr2IsControlComplete(currSkelControl))// && !(stateFlags & STATE_FIRE_DELAYED_WEAP))
				{
					fpFPSForcedReload();
					return currentFPSstate;
				}
				else
				{
					return currentFPSstate;
				}
			}
			else // reload
			{
				if	((currentGameWeapon->ReloadStartTime == 0) ||
					((currentTime - currentGameWeapon->ReloadStartTime) > currentGameWeapon->WeaponType->ReloadTime))
				{
					currentGameWeapon->ReloadStartTime = 0;
					showGun = fpsWeapons[currentWeaponModel].clipBullets > 0;

					stateFlags &= ~STATE_RELOADING;
				}
				else
					return currentFPSstate;
			}
		}
		else if (fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_REVOLVER)
		{
			if (currentFPSstate == RELOAD)
			{
				if (gr2IsControlComplete(currSkelControl))
				{
					if ((UINT32)fpsWeapons[currentWeaponModel].clipBullets < std::min(currentGameWeapon->WeaponType->NumRoundsInClip,
																		  (UINT32)currentGameWeapon->NumRoundsAmmoLeft[currentGameWeapon->AmmoTypeInUse]) )
					{
						return RELOAD2;
					}
					else
					{
						return RELOAD3;
					}
				}
				else
					return currentFPSstate;
			}

			if (currentFPSstate == RELOAD2)
			{
				if (gr2IsControlComplete(currSkelControl))
				{
					if (currentGameWeapon->NumRoundsAmmoLeft[currentGameWeapon->AmmoTypeInUse] == -1)
					{
						fpsWeapons[currentWeaponModel].clipBullets = currentGameWeapon->WeaponType->NumRoundsInClip;
						return RELOAD3;
					}

					++fpsWeapons[currentWeaponModel].clipBullets;

					if ((UINT32)fpsWeapons[currentWeaponModel].clipBullets < std::min(currentGameWeapon->WeaponType->NumRoundsInClip,
																		  (UINT32)currentGameWeapon->NumRoundsAmmoLeft[currentGameWeapon->AmmoTypeInUse]) )
					{
						fpFPSStartAnimationState (currentWeaponModel, RELOAD2);
						return currentFPSstate;
					}
					else
					{
						return RELOAD3;
					}
				}
				else
					return currentFPSstate;
			}

			if (currentFPSstate == RELOAD3)
			{
				if (gr2IsControlComplete(currSkelControl))
				{
					currentGameWeapon->ReloadStartTime = 0;
					stateFlags &= ~STATE_RELOADING;
				}
				else
					return currentFPSstate;
			}
		}
		else
		{
			// hide the clip for the first half second of the reload animation for the rifle
			// this is to prevent the clip from being seen exiting the rifle and teleporting to the
			// hand
			if (fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_RIFLE)
			{
                // Had to hack this in here for single bullet reloads on the French dude.
                if(currentWeaponModel == RG_VB || currentWeaponModel == LEBELMLE || currentWeaponModel == SPRINGFIELD1903)
                {
                    fpsWeaponStates ws = _determineNextRifleState();

                    if(ws == RELOAD || ws == RELOAD2 || ws == RELOAD3)
                    {
                        return ws;
                    }
                }

				showClip = (currentTime - currentGameWeapon->ReloadStartTime) >= 0.5;
			}
			// else auto rifles fire as fast as you can pull the trigger

            if (gr2IsControlComplete(currSkelControl))
			{
				stateFlags &= ~STATE_NEEDS_RECYCLE;

				if ((currentFPSstate == RELOAD) || (currentFPSstate == DEPLOYED_RELOAD))
					currentGameWeapon->ReloadStartTime = 0;

				stateFlags &= ~STATE_RELOADING;

				if ((stateFlags & STATE_DEPLOYED) && (fpsWeapons[currentWeaponModel].auxEnable[FPS_AUX_RGCUP]) &&
					(fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_RIFLE) &&
					(currentFPSstate == DEPLOYED_RELOAD))
				{
					// we now have a RG round loaded
					fpsWeapons[currentWeaponModel].auxEnable[FPS_AUX_RGGRENADE] = 1;
				}

				if (currentWeaponModel == BOYSMK1)
				{
					fpFPSAttachWeapon (currentWeaponModel);

					if(currentFPSstate == DEPLOYED_RELOAD)
						stateFlags |= STATE_NEEDS_RECYCLE;
				}
			}
			else
				return currentFPSstate;
		}
	}

	// wait for fire animation to finish and then check to see if we need to recycle
	// this doesn't apply to satchels (and others) that never actually "finish"
	if (((currentFPSstate == STAND_FIRE) ||
		 (currentFPSstate == CROUCH_FIRE) ||
		 (currentFPSstate == AIM_FIRE) ||
		 (currentFPSstate == PRONE_FIRE)) &&
		 (fpsWeapons[currentWeaponModel].weaponType != FPS_WEAPONTYPE_SATCHEL) &&
		 (fpsWeapons[currentWeaponModel].weaponType != FPS_WEAPONTYPE_BUILD) &&
		 (fpsWeapons[currentWeaponModel].weaponType != FPS_WEAPONTYPE_SMG) &&
		 (fpsWeapons[currentWeaponModel].weaponType != FPS_WEAPONTYPE_LMG) &&
		 (fpsWeapons[currentWeaponModel].weaponType != FPS_WEAPONTYPE_KNIFE))
	{
		if (currSkelControl && !gr2IsControlComplete(currSkelControl))
		{
			return currentFPSstate;
		}
	}

	if (deployNeedsDeployment)
	{
		deployNeedsDeployment = false;

		/*
		if(stat2Flags & STAT2_POSE_CHANGE)
		{
			deployNeedsUndeployment = true;
		}
		else
		{
		*/
			// possiblity exists that we're switching back to an already deployed rifle grenade
			// so skip the whole deployment thing here in that case
			if (!(fpsWeapons[currentWeaponModel].auxEnable[FPS_AUX_RGCUP]))
			{
				stateFlags |= STATE_DEPLOYING;
				deployStartTime = frametime/1000.0f;

				if (fpsWeapons[currentWeaponModel].deploySound)
				{
					fpFPSPlaySound (fpsWeapons[currentWeaponModel].deploySound);
				}

				//if(auxModel[FPS_AUX_RGCUP])
				//	gr2SetModelActive(auxModel[FPS_AUX_RGCUP], true);

				aimVerticalDeployedPosition = 0;
				aimHorizontalDeployedPosition = 0;

				return DEPLOYING;
			}
		//}
	}

	if (deployNeedsUndeployment)
	{
		deployNeedsUndeployment = false;
		stateFlags |= STATE_DEPLOYING;
		deployStartTime = frametime/1000.0f;

		if (fpsWeapons[currentWeaponModel].undeploySound)
		{
			fpFPSPlaySound (fpsWeapons[currentWeaponModel].undeploySound);
		}

		//if(auxModel[FPS_AUX_RGCUP])
		//	gr2SetModelActive(auxModel[FPS_AUX_RGCUP], false);

		aimVerticalDeployedPosition = 0;
		aimHorizontalDeployedPosition = 0;

		return UNDEPLOYING;
	}

	if (stateFlags & STATE_DEPLOYING)
	{
		if (currentFPSstate == UNDEPLOYING)
			stateFlags &= ~STATE_DEPLOYED;

		if (!gr2IsControlComplete(currSkelControl))
			return currentFPSstate;

		stateFlags &= ~STATE_DEPLOYING;

		// level out view
		//aimVertAngle = 0.0f;
		//aimVerticalDeployedPosition = 0.0f;

		if (currentFPSstate == UNDEPLOYING)
		{
			stateFlags &= ~STATE_DEPLOYED;

			// this is actually a reload signal, but recycle works
			if (stateFlags & STATE_NEEDS_RECYCLE)
			{
				fpFPSForcedReload();
				return currentFPSstate;
			}
		}
	}

	// weapons that have hold times will set this flag upon fire
	// it prevents firing until sufficient time has passed
	//
	// satchel, grenade for starter
	if (stateFlags & STATE_HOLD_FIRE)
	{
		if (fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_SATCHEL)
		{
			if ((currentTime - fireStartTime) > fpsWeapons[currentWeaponModel].fireDelay)
			{
				stateFlags &= ~STATE_HOLD_FIRE;
			}
		}
		else if (fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_GRENADE)
		{
			if (!(controlStateFlags & FPS_FIRE))
			{
				stateFlags &= ~STATE_HOLD_FIRE;
				stateFlags |= STATE_FIRE_DELAYED_WEAP;

			}

			if ((currentTime - fireStartTime) > fpsWeapons[currentWeaponModel].fireDelay)
			{
				stateFlags &= ~STATE_HOLD_FIRE;
				stateFlags |= STATE_FIRE_DELAYED_WEAP;
			}
		}
		else if (fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_LTMORTAR)
		{
			if ((currentTime - fireStartTime) > (FLOAT64)fpsWeapons[currentWeaponModel].fireDelay)
			{
				Write_Debug("Mortar round leaving at %f, %f seconds after being fired.", currentTime, (currentTime - fireStartTime));
				stateFlags &= ~STATE_HOLD_FIRE;
				stateFlags |= STATE_FIRE_DELAYED_WEAP;
				fireStartTime = 0.0f;
			}
		}
		else if (fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_KNIFE)
		{
			if ((currentTime - fireStartTime) > fpsWeapons[currentWeaponModel].fireDelay)
			{
				stateFlags &= ~STATE_HOLD_FIRE;
				stateFlags |= STATE_FIRE_DELAYED_WEAP;
			}
		}

		return currentFPSstate;
	}

	// for pistols, make sure the player can't start the fire animation until the pistol is ready to fire again
	if ((fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_AUTO_PISTOL) ||
		(fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_REVOLVER))
	{
		if (currentGameWeapon && (currentGameWeapon->LastFireTime >= currentTime))
		{
			stateFlags &= ~STATE_FIRE;
//			controlStateFlags &= ~FPS_FIRE;
		}
	}

	// nobody gets to play a fire animation if you're out of ammo
	if (fpsWeapons[currentWeaponModel].weaponType != FPS_WEAPONTYPE_BUILD &&
		fpsWeapons[currentWeaponModel].clipBullets == 0 &&
		!fpsWeapons[currentWeaponModel].auxEnable[FPS_AUX_RGCUP])
	{
		if (stateFlags & STATE_FIRE)
		{
			static SINT32 lastDryFireTime;

			if (lastDryFireTime < (frametime - (SINT32)(fpsWeapons[currentWeaponModel].dryFireSoundLength * 1000.0f)))
			{
				fpFPSPlaySound (fpsWeapons[currentWeaponModel].dryFireSound);
				lastDryFireTime = frametime;
			}
		}

		stateFlags &= ~STATE_FIRE;
		controlStateFlags &= ~FPS_FIRE;
	}

	// no firing rockets from the hip
	if(fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_ROCKET && !(stateFlags & STATE_AIM))
	{
		stateFlags &= ~STATE_FIRE;
		controlStateFlags &= ~FPS_FIRE;
	}

	// handle relaxed state by disallowing it if any of the norelax flags are on
	if ((stateFlags & STATE_RELAXED) && (stateFlags & STATE_NORELAX_MASK))
	{
		stateFlags &= ~STATE_RELAXED;
	}

	if (stateFlags & STATE_SWIMMING)
	{
		stateFlags |= STATE_RELAXED;
	}

	// if we need to recycle the weapon, now is the time to do it
	if ( (stateFlags & STATE_NEEDS_RECYCLE) && !(controlStateFlags & FPS_FIRE))// && !(stateFlags & STATE_AIM) )//  | STATE_DEPLOYED)
	{
		if (fpsWeapons[currentWeaponModel].riseStopsReload && (fpsTrooper.currentMuzzleRise > 0 || fpsTrooper.targetMuzzleRise > 0))
			return currentFPSstate;
		else if (fpsWeapons[currentWeaponModel].delayAfterFire > 0)
		{
			SINT32	timeElapsed = frametime - fpsTrooper.fireTimeForDelay;

			if (timeElapsed < (fpsWeapons[currentWeaponModel].delayAfterFire * 1000.0))
				return currentFPSstate;
		}

		// if we get here, we're either going to recycle, or reload, or we're manually reloading and we'll wait for the manual reload signal
		// either way, we're done with the needs recycle flag
		stateFlags &= ~STATE_NEEDS_RECYCLE;

		// if we're out of ammo, do a reload (which includes an implicit recycle)
		if (fpsWeapons[currentWeaponModel].clipBullets <= 0)
		{
			if (fpsTrooper.autoReload)
			{
				fpFPSForcedReload();
				return currentFPSstate;
			}
		}
		else
		{
			fpFPSRecycle ();
			return RECYCLE;
		}
	}

	if (fpsTrooper.parachuteOpen)
	{
		return LOWER;
	}

	if ((fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_GRENADE) && (stateFlags & STATE_FIRE_DELAYED_WEAP))
	{
		return THROW;
	}

	if (stateFlags & STATE_RELAXED)
	{
		return LOWER;
	}

	if ((stateFlags & STATE_FIRE) && (!fpFPSStateWeaponReadyToFire()))
	{
		stateFlags &= ~STATE_FIRE;
	}

// states that currently affect animation are
	// moveState
	// eyeState
	// aimState
	// fireState
	animationStateFlags = stateFlags & STATE_ANIMATION_MASK;

	switch (animationStateFlags)
	{
		case STATE_MOVE_STILL	|STATE_EYE_STAND	|STATE_FIRE:
		case STATE_MOVE_SLOW	|STATE_EYE_STAND	|STATE_FIRE:
		case STATE_MOVE_BACKWARD|STATE_EYE_STAND	|STATE_FIRE:
			newState = STAND_FIRE;
			break;
		case STATE_MOVE_STILL	|STATE_EYE_CROUCH	|STATE_FIRE:
		case STATE_MOVE_SLOW	|STATE_EYE_CROUCH	|STATE_FIRE:
		case STATE_MOVE_BACKWARD|STATE_EYE_CROUCH	|STATE_FIRE:
			newState = CROUCH_FIRE;
			break;
		case STATE_MOVE_STILL	|STATE_EYE_STAND	|STATE_AIM	|STATE_FIRE:
		case STATE_MOVE_SLOW	|STATE_EYE_STAND	|STATE_AIM	|STATE_FIRE:
		case STATE_MOVE_BACKWARD|STATE_EYE_STAND	|STATE_AIM	|STATE_FIRE:
		case STATE_MOVE_STILL	|STATE_EYE_CROUCH	|STATE_AIM	|STATE_FIRE:
		case STATE_MOVE_SLOW	|STATE_EYE_CROUCH	|STATE_AIM	|STATE_FIRE:
		case STATE_MOVE_BACKWARD|STATE_EYE_CROUCH	|STATE_AIM	|STATE_FIRE:
		case STATE_MOVE_STILL	|STATE_EYE_PRONE	|STATE_AIM	|STATE_FIRE:
		case STATE_MOVE_SLOW	|STATE_EYE_PRONE	|STATE_AIM	|STATE_FIRE:
		case STATE_MOVE_BACKWARD|STATE_EYE_PRONE	|STATE_AIM	|STATE_FIRE:
		case STATE_MOVE_STILL	|STATE_EYE_PRONE	|STATE_FIRE:
		case STATE_MOVE_SLOW	|STATE_EYE_PRONE	|STATE_FIRE:
		case STATE_MOVE_BACKWARD|STATE_EYE_PRONE	|STATE_FIRE:
			newState = AIM_FIRE;
			break;
		case STATE_MOVE_STILL	|STATE_EYE_STAND:
			newState = IDLE;
			break;
		case STATE_MOVE_STILL	|STATE_EYE_CROUCH:
			newState = KNEEL;
			break;
		case STATE_MOVE_STILL	|STATE_EYE_PRONE:
			if(fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_ROCKET && (currentWeaponModel != PIAT || !(stateFlags & STATE_AIM)))
			{
				newState = IDLE;
				break;
			}
			else if(fpsWeapons[currentWeaponModel].sniperScope)
			{
				if (stateFlags & (STATE_STRAFE_LEFT | STATE_STRAFE_RIGHT))
				{
					stateFlags &= ~STATE_AIM;
					newState = CRAWL;
				}
				else newState = IDLE;
				break;
			}
		case STATE_MOVE_STILL	|STATE_EYE_PRONE	|STATE_AIM:
			if (stateFlags & (STATE_STRAFE_LEFT | STATE_STRAFE_RIGHT))
			{
				stateFlags &= ~STATE_AIM;
				newState = CRAWL;
			}
			else newState = AIM;
			break;
		case STATE_MOVE_STILL	|STATE_EYE_CROUCH	|STATE_AIM:
		case STATE_MOVE_STILL	|STATE_EYE_STAND	|STATE_AIM:
		case STATE_MOVE_SLOW	|STATE_EYE_STAND	|STATE_AIM:
		case STATE_MOVE_SLOW	|STATE_EYE_CROUCH	|STATE_AIM:
		case STATE_MOVE_BACKWARD|STATE_EYE_STAND	|STATE_AIM:
		case STATE_MOVE_BACKWARD|STATE_EYE_CROUCH	|STATE_AIM:
			newState = AIM;
			break;
		case STATE_MOVE_SLOW	|STATE_EYE_STAND:
		case STATE_MOVE_BACKWARD|STATE_EYE_STAND:
			newState = TROT;
			break;
		case STATE_MOVE_SLOW	|STATE_EYE_CROUCH:
		case STATE_MOVE_BACKWARD|STATE_EYE_CROUCH:
			newState = CREEP;
			break;
		case STATE_MOVE_SLOW	|STATE_EYE_PRONE:
		case STATE_MOVE_BACKWARD|STATE_EYE_PRONE:
			newState = CRAWL;
			stateFlags |= STATE_NO_ARM_SWAY;
			break;
		case STATE_MOVE_FAST	|STATE_EYE_CROUCH:
		case STATE_MOVE_FAST	|STATE_EYE_STAND:
			newState = SPRINT;
			if (!((fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_LMG) ||
				  (fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_ATR) ||
				  (fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_LTMORTAR)))
			{
				stateFlags |= STATE_NO_ARM_SWAY;
			}
			break;
		default:
			Write_Log (LOG_FPS, LOG_INFO, "Unknown state!  0x%x", animationStateFlags);
			newState = IDLE;
			break;
	}

	return newState;
}

// this takes a state, and, based on the weapon being held, modifies the state
// aka .. this is where the weapon specific handling should go
//
// this is where the weapon types should get their differentiation from the standard model
// even though a lot of it happens in determineNextState
//
// returns TRUE if no changes, FALSE if changed
LOGICAL fpFPSWeaponStateCheck (fpsWeaponStates* newState)
{
	fpsWeaponStates		currState;
	fpFPSWeaponData*	currWeapon;
	fpFPSWeaponType		currWeaponType;
	FLOAT64				currentTime;

	assert (newState);

	currState = *newState;
	currWeapon = &fpsWeapons[currentWeaponModel];
	currWeaponType = currWeapon->weaponType;
	currentTime = frametime / 1000.0;

	if (stateFlags & STATE_SIT)
	{
		if (!(stateFlags & STATE_RELOADING))
			*newState = SIT;
		return FALSE;
	}

	if (currWeaponType == FPS_WEAPONTYPE_SMG)
	{
		if (currState == AIM)
		{
			if (armsAnims[currentWeaponModel][AIM_SMG])
			{
				if ( HAS_ALL_BITS(stateFlags, STATE_EYE_STAND | STATE_MOVE_STILL) )
					*newState = AIM_SMG;
				else if ((stateFlags & STATE_EYE_CROUCH))
					*newState = AIM_SMG;
				else if ((stateFlags & STATE_EYE_PRONE))
					*newState = AIM_SMG;
			}
		}
		return TRUE;
	}
	else if (currWeaponType == FPS_WEAPONTYPE_RIFLE || currWeaponType == FPS_WEAPONTYPE_AUTO_RIFLE)
	{
		if (currState == DEPLOYING)
		{
			if ((currWeapon->auxAvailable[FPS_AUX_RGCUP]) && (currWeapon->auxAltConnectTime[FPS_AUX_RGCUP]> 0.0))
			{
				FLOAT64 playTime = currentTime - deployStartTime;

				gr2SetModelActive(auxModel[FPS_AUX_RGCUP], true);

				if(playTime > currWeapon->auxAltConnectTime[FPS_AUX_RGCUP])
				{
					gr2AttachModel(auxModel[FPS_AUX_RGCUP], currWeapon->auxConnect[FPS_AUX_RGCUP], weapModel, currWeapon->auxConnectToAlt[FPS_AUX_RGCUP]);
					currentGameWeapon->AmmoTypeInUse = 1;
					fpsWeapons[currentWeaponModel].auxEnable[FPS_AUX_RGCUP] = 1;
					fpsWeapons[currentWeaponModel].auxEnable[FPS_AUX_RGGRENADE] = 0;
					rgRoundFired = false;
				}
			}
		}
		else if (currState == UNDEPLOYING)
		{
			if ((currWeapon->auxAvailable[FPS_AUX_RGCUP]) && (currWeapon->auxAltConnectTime[FPS_AUX_RGCUP]))
			{
				FLOAT64 playTime = currentTime - deployStartTime;

				if(playTime >= currWeapon->auxAltDisconnectTime[FPS_AUX_RGCUP])
				{
					gr2DetachModel(auxModel[FPS_AUX_RGCUP]);
					currentGameWeapon->AmmoTypeInUse = 0;
					fpsWeapons[currentWeaponModel].auxEnable[FPS_AUX_RGCUP] = 0;
					fpsWeapons[currentWeaponModel].auxEnable[FPS_AUX_RGGRENADE] = 0;
					//gr2SetModelActive(auxModel[FPS_AUX_RGCUP], false);

					// set this so the main loop knows that it needs to recycle/reload
					if(rgRoundFired)
						stateFlags |= STATE_NEEDS_RECYCLE;
				}
			}
		}
		else if (currState == DEPLOYED_RELOAD)
		{
			if (currWeapon->auxAltConnectTime[FPS_AUX_RGGRENADE])
			{
				if ((currentTime - currentGameWeapon->ReloadStartTime) > currWeapon->auxAltConnectTime[FPS_AUX_RGGRENADE])
				{
					/*gr2DetachModel (auxModel[FPS_AUX_RGGRENADE]);

					if (!gr2AttachModel (auxModel[FPS_AUX_RGGRENADE], currWeapon.auxConnect[FPS_AUX_RGGRENADE], weapModel, currWeapon.auxConnectToAlt[FPS_AUX_RGGRENADE]))
					{
						Write_Log (LOG_FPS, LOG_INFO, "Failed to attach rggrenade to alt mount in gun");
						return FALSE;
					}*/
				}
			}
		}
		else if (stateFlags & STATE_DEPLOYED)
		{
			if (currState == KNEEL)
			{
				if (fpsWeapons[currentWeaponModel].rgFiringRules)
					*newState = DEPLOYED;
			}

			if (stateFlags & STATE_FIRE)
			{
				// if we're crouched and firing from a weapon marked with rifle grenade firing rules, go to the special fire state
				if ((fpsWeapons[currentWeaponModel].rgFiringRules) && (stateFlags & STATE_EYE_CROUCH))
					*newState = DEPLOYED_FIRE;

				/*gr2DetachModel (auxModel[FPS_AUX_RGGRENADE]);

				if (!gr2AttachModel (auxModel[FPS_AUX_RGGRENADE], currWeapon.auxConnect[FPS_AUX_RGGRENADE], weapModel, currWeapon.auxConnectTo[FPS_AUX_RGGRENADE]))
				{
					Write_Log (LOG_FPS, LOG_INFO, "Failed to attach rggrenade to alt mount in gun");
					return FALSE;
				}*/

				return FALSE;
			}
		}

		return TRUE;
	}
	// deploying weapons with pivot bones have special rules as follows:
	else if (fpsWeapons[currentWeaponModel].deployPivotBone)
	{
		if (stateFlags & STATE_DEPLOYED)
		{
			switch (currState)
			{
				case SIT:
				case DEPLOYED:
				case DEPLOYING:
				case UNDEPLOYING:
				case DEPLOYED_FIRE:
				case DEPLOYED_RELOAD:
				case RECYCLE:
					return TRUE;

				case LOWER:
				case RAISE:
				case IDLE:
				case KNEEL:
				case TROT:
				case CREEP:
				case CRAWL:
				case SPRINT:
				case AIM:
				case PRONE:
					*newState = DEPLOYED;
					return FALSE;
				case RELOAD:
					*newState = DEPLOYED_RELOAD;
					return FALSE;
				case STAND_FIRE:
				case CROUCH_FIRE:
				case AIM_FIRE:
				case PRONE_FIRE:
					*newState = DEPLOYED_FIRE;
					return FALSE;
				default:
					Write_Log (LOG_FPS, LOG_INFO, "Unknown state encountered in fpFPSWeaponStateCheck for satchel");
					*newState = IDLE;
					return FALSE;
			}
		}

		return TRUE;
	}
	else if ((currWeaponType == FPS_WEAPONTYPE_AUTO_PISTOL) || (currWeaponType == FPS_WEAPONTYPE_REVOLVER) )
	{
		// special handling for AIM here .. if prone, switch it to PRONE
		if (currState == AIM)
		{
			if (stateFlags & STATE_EYE_PRONE)
			{
				*newState = PRONE;
				return FALSE;
			}
		}
		else if (currState == AIM_FIRE)
		{
			if (stateFlags & STATE_EYE_PRONE)
			{
				*newState = PRONE_FIRE;
				return FALSE;
			}
		}
		return TRUE;
	}
	else if (currWeaponType == FPS_WEAPONTYPE_SATCHEL || currWeaponType == FPS_WEAPONTYPE_BUILD)
	{
		// satchel has all kinds of exclusions..
		switch (currState)
		{

			case STAND_FIRE:
			case LOWER:
			case RAISE:
			case IDLE:
			case KNEEL:
			case TROT:
			case CREEP:
			case CRAWL:
			case SPRINT:
			case RELOAD:
			case SIT:
				return TRUE;

			case AIM:
			case NUM_WEAP_STATES:
				*newState = IDLE;
				return FALSE;
			case CROUCH_FIRE:
			case AIM_FIRE:
				*newState = STAND_FIRE;
				return FALSE;
			default:
				Write_Log (LOG_FPS, LOG_INFO, "Unknown state encountered in fpFPSWeaponStateCheck for satchel");
				*newState = IDLE;
				return FALSE;
		}
	}
	else if ((currWeaponType == FPS_WEAPONTYPE_GRENADE) || (currWeaponType == FPS_WEAPONTYPE_KNIFE))
	{
		// grenade has all kinds of exclusions..
		switch (currState)
		{

			case STAND_FIRE:
			case LOWER:
			case RAISE:
			case IDLE:
			case KNEEL:
			case TROT:
			case CREEP:
			case CRAWL:
			case SPRINT:
			case RELOAD:
			case SIT:
			case THROW:
				return TRUE;

			case AIM:
			case NUM_WEAP_STATES:
				*newState = IDLE;
				return FALSE;
			case CROUCH_FIRE:
			case AIM_FIRE:
				*newState = STAND_FIRE;
				return FALSE;
			default:
				Write_Log (LOG_FPS, LOG_INFO, "Unknown state (%d) encountered in fpFPSWeaponStateCheck for knife or grenade", currState);
				*newState = IDLE;
				return FALSE;
		}
	}

	return TRUE;
}

LOGICAL fpFPSCleanupAnimationState ()
{
	if (currClipControl)
	{
		gr2FreeControl (currClipControl);
		currClipControl = nullptr;
	}

	if (currWeapControl)
	{
		gr2FreeControl (currWeapControl);
		currWeapControl = nullptr;
		gr2UpdateBlendEvents(weapModel);
	}

	if (currSkelControl)
	{
		gr2FreeControl (currSkelControl);
		currSkelControl = nullptr;
	}

	if(currRGCupControl)
	{
		gr2FreeControl(currRGCupControl);
		currRGCupControl = nullptr;
	}

	return TRUE;
}

//
// NOTE:  do NOT set currentFPSstate before calling this
//

LOGICAL fpFPSStartAnimationState (fpsWeaponTypes weapon, fpsWeaponStates state, FLOAT32 easeIn, FLOAT32 easeOut)
{
	SINT32			repeatCount;
	FLOAT32			outDuration, inDuration, animSpeed;

    outDuration = 0.3f;
	inDuration = 0.2f;
	animSpeed = 1.0f; //animationSpeed;

	fpFPSCleanupAnimationState ();

	Write_Log (LOG_FPS, LOG_INFO, "starting state %d, currentState %d, controlFlags = 0x%x, stateFlags = 0x%x", state, currentFPSstate, controlStateFlags, stateFlags);

	// let the tutorial manager know we've changed states
	TUT_MGR->DoEvent(TUT_EVENT_STATE_TROOP, &state, &weapon);

	// make the weapon not ready for a short period while rasing it (prevent firing)
	if(currentFPSstate != state && currentFPSstate == LOWER && fireDelay == 0)
	{
		fireDelay = frametime + 300;
	}
	else if(fireDelay && frametime >= fireDelay)
	{
		fireDelay = 0;
	}

	if (state == LOWER)
	{
		if (stateFlags & STATE_WEAPON_SWITCHING)
			outDuration = 0.1f;
		else
			outDuration = 0.3f;
		inDuration = 0.3f;
		repeatCount = GrannyForever;

		if((fpFPSIsSwimming() || currentFPSstate != state) && weaponChangeStartTime == 0)
			weaponChangeStartTime = frametime / 1000.0;
	}
	else if ((state == RELOAD) || (state == RECYCLE) || (state == RELOAD2) || (state == RELOAD3) ||
			 (state == THROW) || (state == DEPLOYED_RELOAD) || (state == DEPLOYING) || (state == UNDEPLOYING))
	{
		if (fpsWeapons[weapon].weaponType == FPS_WEAPONTYPE_SATCHEL || fpsWeapons[weapon].weaponType == FPS_WEAPONTYPE_BUILD)
		{
			outDuration = 0;
			inDuration = 1.0;
			repeatCount = GrannyForever;
			showGun = false;
		}
        // HACK! It's the same behavior as the revolver, but keeping it separate, since it may
        // change
        else if((weapon == RG_VB || weapon == LEBELMLE || weapon == SPRINGFIELD1903) &&
                (state == RELOAD || state == RELOAD2 || state == RELOAD3))
        {
            switch(state)
            {
            case RELOAD:
                {
                    repeatCount = 1;
                    break;
                }
            case RELOAD2:
                {
                    inDuration = 0;
                    outDuration = 0;

                    if(fpsWeapons[weapon].reload2Sound)
                    {
                        fpFPSfxPlayNormalSound(fpsWeapons[weapon].reload2Sound, IDE_FP_PRIORITY_HIGH, TYPE_FINITE_SOUND, nullptr, persGetPersonaVehicleObjectDataPtr());
                    }

				    repeatCount = 1;
                    break;
                }
            case RELOAD3:
                {
                    inDuration = 0;

                    if(fpsWeapons[weapon].reload3Sound)
                    {
                        fpFPSfxPlayNormalSound(fpsWeapons[weapon].reload3Sound, IDE_FP_PRIORITY_HIGH, TYPE_FINITE_SOUND, nullptr, persGetPersonaVehicleObjectDataPtr());
                    }

                    repeatCount = 1;
                    break;
                }
            default:
                // only care about reload states
                break;
            }
        }
		else if (fpsWeapons[weapon].weaponType == FPS_WEAPONTYPE_REVOLVER)
		{
			if (state == RELOAD)
				repeatCount = 1;
			else if (state == RELOAD2)
			{
				inDuration = 0;
				outDuration = 0;

				if (fpsWeapons[currentWeaponModel].reload2Sound)
				{
					fpFPSfxPlayNormalSound (fpsWeapons[currentWeaponModel].reload2Sound, IDE_FP_PRIORITY_HIGH, TYPE_FINITE_SOUND, nullptr, persGetPersonaVehicleObjectDataPtr());
				}

				repeatCount = 1;
			}
			else if (state == RELOAD3)
			{
				inDuration = 0;

				if (fpsWeapons[currentWeaponModel].reload3Sound)
				{
					fpFPSfxPlayNormalSound (fpsWeapons[currentWeaponModel].reload3Sound, IDE_FP_PRIORITY_HIGH, TYPE_FINITE_SOUND, nullptr, persGetPersonaVehicleObjectDataPtr());
                }

				repeatCount = 1;
			}
		}
		else if (fpsWeapons[weapon].weaponType == FPS_WEAPONTYPE_GRENADE)
		{
			// reload is a single frame .. but throw is an animation
			// in either case, keep the grenade hidden
			if (state == RELOAD)
			{
				repeatCount = GrannyForever;
				//showGun = false;
			}
			else
			{
				repeatCount = 1;
				grenadeApperLapse = (frametime/1000.0f);

			}
		}
		else if ((weapon == BOYSMK1)&& (state == RECYCLE))
		{
            repeatCount = 1;
			//outDuration = 0.0f;
            inDuration = 0.0001f;
		}
		else
		{
			repeatCount = 1;
			inDuration = 0.3f; // Fix this! DEG

			/* animation sound needs to be adjusted before we can do this
			if((weapon == ENFIELD || weapon == ENFIELDMK4) &&
			   (state == RELOAD || state == RELOAD2 || state == RELOAD3))
				animSpeed = 1.5f * animationSpeed;
			*/
		}
	}
	// lots of special cases for weapon firing, let's separate them out
	// into their own section
	else if ((state == STAND_FIRE) || (state == CROUCH_FIRE) || (state == AIM_FIRE) || (state == PRONE_FIRE) || (state == DEPLOYED_FIRE))
	{
		if ((fpsWeapons[weapon].weaponType == FPS_WEAPONTYPE_SMG) || (fpsWeapons[weapon].weaponType == FPS_WEAPONTYPE_LMG))
		{
			repeatCount = GrannyForever;
			outDuration = 0.2f;
			inDuration = 0.0;
		}
		else if (fpsWeapons[weapon].weaponType == FPS_WEAPONTYPE_SATCHEL)
		{
			if (!(stateFlags & STATE_HOLD_FIRE))
			{
				stateFlags |= STATE_HOLD_FIRE;
				fireStartTime = frametime / 1000.0;
				fpsTrooper.tpsStartAttack = 1;
			}
			else
				fireStartTime = 0.0;

			repeatCount = GrannyForever;
		}
		else if (fpsWeapons[weapon].weaponType == FPS_WEAPONTYPE_GRENADE)
		{
			if (!(stateFlags & STATE_HOLD_FIRE))
			{
				stateFlags |= STATE_HOLD_FIRE; //DEG Power Selector SP

				fireStartTime = frametime / 1000.0;
				fpsTrooper.tpsStartAttack = 1;
			}
			else
				fireStartTime = 0.0;

			repeatCount = 1;
		}
		else if (fpsWeapons[weapon].weaponType == FPS_WEAPONTYPE_LTMORTAR)
		{
			if (state == DEPLOYED_FIRE && !(stateFlags & STATE_HOLD_FIRE))
			{
				// need to unhide the correct ammo mesh
				gr2Model* weap = weaponModels[weapon];
				if(weap)
				{
					gr2SetMeshActiveByName(weap, "lmortar_he", !currentGameWeapon->AmmoTypeInUse);
					gr2SetMeshActiveByName(weap, "lmortar_smk", currentGameWeapon->AmmoTypeInUse);
				}

				stateFlags |= STATE_HOLD_FIRE;
				fireStartTime = frametime / 1000.0;
				fpsTrooper.tpsStartAttack = 1;
				Write_Debug("Firing mortar at %f", fireStartTime);

				// go ahead and play the reload sound for mortars here
				SCHAR* reloadSound = fpsWeapons[currentWeaponModel].reloadSound;
				if (reloadSound)
				{
					fpFPSfxPlayNormalSound (reloadSound, IDE_FP_PRIORITY_HIGH, TYPE_FINITE_SOUND, nullptr, persGetPersonaVehicleObjectDataPtr());
				}
			}
			else
				fireStartTime = 0.0;

			repeatCount = 1;
		}
		else if (fpsWeapons[weapon].weaponType == FPS_WEAPONTYPE_KNIFE)
		{
			if (!(stateFlags & STATE_HOLD_FIRE))
			{
				stateFlags |= STATE_HOLD_FIRE;

				fireStartTime = frametime / 1000.0;
				fpsTrooper.tpsStartAttack = 1;
			}
			else
				fireStartTime = 0.0;

			stateFlags |= STATE_KNIFE_ATTACKING;

			// set random knife stabs here DEG
			SINT32 random = abs(mathGetChaoticNumber()) % 3;
			if(random == 1)
			{
				state = PRONE_FIRE;
			}
			else if(random == 2)
			{
				state = CROUCH_FIRE;
			}
	        // End Random Stab  DEG


			repeatCount = 1;
		}
		else if (fpsWeapons[weapon].weaponType == FPS_WEAPONTYPE_RIFLE || fpsWeapons[weapon].weaponType == FPS_WEAPONTYPE_AUTO_RIFLE)
		{
			outDuration = 0.2f;
			inDuration = 0.0f;
			repeatCount = 1;
		}
		else if (fpsWeapons[weapon].weaponType == FPS_WEAPONTYPE_AUTO_PISTOL)
		{
			outDuration = 0.2f;
			inDuration = 0.0f;
			repeatCount = 1;
		}
		else if (fpsWeapons[weapon].weaponType == FPS_WEAPONTYPE_REVOLVER)
		{
          outDuration = 0.01f;
          inDuration = 0.1f;
		  repeatCount = 1;
		}
		/*else if (weapon == BOYSMK1)
		{
            repeatCount = 1;
			outDuration = 0.0f;
		}*/
		else repeatCount = 1;
	}
	else if (state == SPRINT)
	{
		if ((fpsWeapons[weapon].weaponType == FPS_WEAPONTYPE_ATR) ||
			(fpsWeapons[weapon].weaponType == FPS_WEAPONTYPE_LMG))
		{
			animSpeed = .8f;// * animationSpeed;
		}
		else if (fpsWeapons[weapon].weaponType == FPS_WEAPONTYPE_LTMORTAR)
		{
			animSpeed = .005f;// * animationSpeed;
		}
		else if ((fpsWeapons[weapon].weaponType == FPS_WEAPONTYPE_GRENADE) ||
				 (fpsWeapons[weapon].weaponType == FPS_WEAPONTYPE_AUTO_PISTOL) ||
				 (fpsWeapons[weapon].weaponType == FPS_WEAPONTYPE_REVOLVER) ||
				 (fpsWeapons[weapon].weaponType == FPS_WEAPONTYPE_KNIFE) ||
				 (fpsWeapons[weapon].weaponType == FPS_WEAPONTYPE_SATCHEL) ||
				 (fpsWeapons[weapon].weaponType == FPS_WEAPONTYPE_BUILD))
		{
			animSpeed = .7f;// * animationSpeed;
		}

		repeatCount = GrannyForever;
	}
	else
	{
		repeatCount = GrannyForever;
	}

	animSpeed *= animationSpeed;

	if(easeIn >= 0) inDuration = easeIn;
	if(easeOut >= 0) outDuration = easeOut;

// The Fallowing is to set-up the animation for the FPS weapon Clip - this is no longer used. DEG
	if (clipModel && (clip_anim_files[weapon][state][0]))
	{
		currClipControl = gr2PlayAnimation (clipModel, clipAnims[weapon][state]);
	}
	else if (clipModel && (clip_default[weapon][0]))
	{
		currClipControl = gr2PlayAnimation (clipModel, clipDefaultAnim[weapon]);
	}
	else
	{
		currClipControl = nullptr;
	}

	if (currClipControl)
	{
		gr2ControlSetLoopCount (currClipControl, repeatCount);

		if (inDuration > 0)
			gr2ControlSetEaseInDuration (currClipControl, inDuration);

		gr2ControlSetEaseOutDuration (currClipControl, outDuration);
		gr2ControlSetSpeed (currClipControl, animSpeed);
	}
// End of Clip Animation. DEG

// The fallowing is the Granny setup for the weapon animation controls for FPS. DEG
	if (weap_anim_files[weapon][state][0])
	{
		// the last round for the garand has a special animation
		// yay, another special case!
		if(weapon == M1GARAND && (state == STAND_FIRE || state == CROUCH_FIRE || state == AIM_FIRE || state == PRONE_FIRE) && fpsWeapons[weapon].clipBullets == 1)
		{
			currWeapControl = gr2PlayAnimation (weapModel, weapAnims[weapon][RELOAD2]);
		}
		else if(((state == DEPLOYING)||(state == UNDEPLOYING)) && (fpsWeapons[weapon].weaponType == FPS_WEAPONTYPE_LMG))
		{
			currWeapControl = gr2PlayAnimation (weapModel, weapAnims[weapon][DEPLOYING]);
		}
		else
		{
			currWeapControl = gr2PlayAnimation (weapModel, weapAnims[weapon][state]);
		}

		// Set up to play what we want for the weapon animations. DEG

		// if we're a riflegrenade, play the animation
		// the animation is in the same file as the weapon animation for DEPLOY and UNDEPLOY
		// if your a riflegrenade
		if((state == DEPLOYING || state == UNDEPLOYING) && auxModel[FPS_AUX_RGCUP])
			currRGCupControl = gr2PlayAnimation (auxModel[FPS_AUX_RGCUP], weapAnims[weapon][state]);
	}
	else if ((weap_empty_default[weapon][0]) && ((fpsWeapons[currentWeaponModel]).clipBullets == 0))
	{
		currWeapControl = gr2PlayAnimation (weapModel, weapEmptyDefaultAnim[weapon]);
		// If we are out of ammo we play this one for weapons. DEG
	}
	else if (weap_default[weapon][0])
	{
		currWeapControl = gr2PlayAnimation (weapModel, weapDefaultAnim[weapon]);
		// This is the animation we play for defult. DEG
	}
	else
	{
		currWeapControl = nullptr;
		gr2UpdateBlendEvents(weapModel);
	}

	if (currWeapControl)
	{
		gr2ControlSetLoopCount (currWeapControl, repeatCount);

		if (inDuration > 0)
			gr2ControlSetEaseInDuration (currWeapControl, inDuration);

		gr2ControlSetEaseOutDuration (currWeapControl, outDuration);

		// lmgs play the deploy animation for both deploy and undeploy
		// but for undeploy, they play it backwards
		if(((state == DEPLOYING)||(state == UNDEPLOYING)) && (fpsWeapons[weapon].weaponType == FPS_WEAPONTYPE_LMG))
		{
			if(state == UNDEPLOYING)
			{
				gr2ControlSetLastFrame(currWeapControl);
				gr2ControlSetSpeed(currWeapControl, -(animSpeed * LMG_DEPLOY_SPEED_FACTOR));
			}
			else
			{
				gr2ControlSetSpeed(currWeapControl, (animSpeed * LMG_DEPLOY_SPEED_FACTOR));
			}
		}
		else
		{
			gr2ControlSetSpeed (currWeapControl, animSpeed);
		}

		//If we have a proper animation, we set the controls for it here. DEG

		// this currently only fires for DEPLOYING and UNDEPLOYING
		if(currRGCupControl)
		{
			gr2ControlSetLoopCount (currRGCupControl, repeatCount);

			if (inDuration > 0)
				gr2ControlSetEaseInDuration (currRGCupControl, inDuration);

			gr2ControlSetEaseOutDuration (currRGCupControl, outDuration);
			gr2ControlSetSpeed (currRGCupControl, animSpeed);
		}

		gr2UpdateControlEvents(currWeapControl);
	}

	if (arms_anim_files[weapon][state][0])
	{
		if(((state == DEPLOYING)||(state == UNDEPLOYING)) && (fpsWeapons[weapon].weaponType == FPS_WEAPONTYPE_LMG))
		{
			currSkelControl = gr2PlayAnimation (skeletonModel, armsAnims[weapon][DEPLOYING]);
		}
		else
		{
			currSkelControl = gr2PlayAnimation (skeletonModel, armsAnims[weapon][state]);
		}
		// This is where the arm animation is set for FPS. DEG
	}
	else
	{
		currSkelControl = nullptr;
	}

	if (currSkelControl)
	{
		//DEG

		gr2ControlSetLoopCount (currSkelControl, repeatCount);

		if (inDuration > 0)
			gr2ControlSetEaseInDuration (currSkelControl, inDuration);

		gr2ControlSetEaseOutDuration (currSkelControl, outDuration);

		if(((state == DEPLOYING)||(state == UNDEPLOYING)) && (fpsWeapons[weapon].weaponType == FPS_WEAPONTYPE_LMG))
		{
			if(state == UNDEPLOYING)
			{
				gr2ControlSetLastFrame(currSkelControl);
				gr2ControlSetSpeed(currSkelControl, -(animSpeed * LMG_DEPLOY_SPEED_FACTOR));
			}
			else
			{
				gr2ControlSetSpeed(currSkelControl, (animSpeed * LMG_DEPLOY_SPEED_FACTOR));
			}
		}
		else
		{
			gr2ControlSetSpeed (currSkelControl, animSpeed);
		}

		// This is the controls for arms animation for FPS DEG

	}

	gr2UpdateAttachment(armsModel);
	gr2UpdateAttachment(weapModel);
	gr2UpdateAttachment(clipModel);

	for (int i = 0; i < FPS_NUM_AUX_MODELS; i++)
		gr2UpdateAttachment(auxModel[i]);

	return TRUE;
}

void generateRotationMatrix (int axis, FLOAT64 angle, piMATRIX* pMatrix)
{
	FLOAT32	trigValue;

	angle = angle * PI/180.0;

	trigValue = (FLOAT32)cos(angle);

	if (axis == AROUND_X)
	{
		*pMatrix = IdentityMatrix;
		pMatrix->_22 = trigValue;
		pMatrix->_33 = trigValue;

		trigValue = (FLOAT32)sin(angle);

		pMatrix->_23 = -trigValue;
		pMatrix->_32 = trigValue;
	}
	else if (axis == AROUND_Y)
	{
		*pMatrix = IdentityMatrix;
		pMatrix->_11 = trigValue;
		pMatrix->_33 = trigValue;

		trigValue = (FLOAT32)sin(angle);

		pMatrix->_13 = -trigValue;  // because our coord system has negative z up?
		pMatrix->_31 = trigValue;
	}
	else if (axis == AROUND_Z)
	{
		*pMatrix = IdentityMatrix;
		pMatrix->_11 = trigValue;
		pMatrix->_22 = trigValue;

		trigValue = (FLOAT32)sin(angle);

		pMatrix->_12 = -trigValue;
		pMatrix->_21 = trigValue;
	}

}

void fpFPSAdjustEyeToPose (FLOAT32 deltaTime)
{
    static SINT32 _fallTimer = 0;
	static FLOAT32 eyeHeight = 0;

	if(!eyeHeight)
		eyeHeight = TROOPER_STAND_EYE_HEIGHT;

	FLOAT32 targetEyeHeight	= fpFPSGetEyeAdjust(-1, -1, false);

    if(fpFPSIsDead())
    {
        if(TROOPER_STAND_EYE_HEIGHT - 0.26f <= -currentEyeHeight)
        {
            currentEyeHeight = -TROOPER_STAND_EYE_HEIGHT + 0.26f;
        }
        else
        {
            if(1)
            {
                heightAtDeath += -9.8f * deltaTime * 0.21f;
                currentEyeHeight = heightAtDeath;

                _fallTimer = frametime;
            }
        }
    }
	else if (targetEyeHeight != eyeHeight)
	{
		static FLOAT32 minMove = .1f;
		static FLOAT32 rate = 4.0f;

		if(snapView)
		{
			snapView = false;
			deltaTime = 5.0f;
		}

		FLOAT32 eyeDistance		= targetEyeHeight - eyeHeight;
		FLOAT32 eyeMovement		= deltaTime * std::max((FLOAT32)fabs(eyeDistance), minMove) * rate;

		bool down = (eyeDistance < 0);

		// make sure they're the same sign (take this out once the non-linear system is in)
		if ((eyeDistance * eyeMovement) < 0 )
			eyeMovement *= -1.0f;

		if ((eyeMovement * eyeMovement) < (eyeDistance * eyeDistance))
		{
			eyeHeight += eyeMovement;
			FLOAT64 amount = 5.0f;
			fpFPSAddWeaponLag(AROUND_X, (down) ? -amount: amount);
		}
		else
		{
			eyeHeight = targetEyeHeight;
		}

		// are out poses in sync?
		//bool poseSync = (prevPose == targetPose);

		if (eyeHeight == targetEyeHeight)
		{
//				Write_Log (LOG_FPS, LOG_INFO, "End reached");
			prevPose = targetPose;
			stat2Flags &= ~STAT2_POSE_CHANGE;
			TUT_MGR->DoEvent(TUT_EVENT_FPS_POSE, &targetPose);
			_updateFPSColliders();

			PlayerPlacedObject* ppo = fpPPOGet();

			if(!ppo || !ppo->buildAt)
				_ppoLastStance = fpFPSGetPlayerPose();
		}

		// are we in sync now? if so, update some stuff
		/*
		if(!poseSync && targetPose == prevPose)
		{
			stat2Flags &= ~STAT2_POSE_CHANGE;
			TUT_MGR->DoEvent(TUT_EVENT_FPS_POSE, &targetPose);
		}
		*/
	}
	else if(prevPose != targetPose)
	{
		prevPose = targetPose;
		stat2Flags &= ~STAT2_POSE_CHANGE;
		TUT_MGR->DoEvent(TUT_EVENT_FPS_POSE, &targetPose);
		_updateFPSColliders();

		PlayerPlacedObject* ppo = fpPPOGet();

		if(!ppo || !ppo->buildAt)
			_ppoLastStance = fpFPSGetPlayerPose();
	}

	currentEyeHeight = -TROOPER_STAND_EYE_HEIGHT + eyeHeight;
}

LOGICAL fpFPSMouseLookOK ()
{
	if (stateFlags & STATE_AIM)
		return FALSE;
	if (stateFlags & STATE_DEAD)
		return FALSE;
	if (stateFlags & STATE_RELOADING)
		return FALSE;
	if (stateFlags & STATE_EMOTE_PLAYING)
		return FALSE;
	if (stateFlags & STATE_KNIFE_ATTACKING)
		return FALSE;
	if (stateFlags & STATE_DEPLOYING)
		return FALSE;
	if ((fpsWeapons[currentWeaponModel].staticDeploy) && (stateFlags & STATE_DEPLOYED))
		return FALSE;
	if (stateFlags & STATE_WEAPON_SWITCHING)
		return FALSE;

	return TRUE;
}

void _doDeath(FLOAT32 deltaTime, piMATRIX* eye)
{
    static FLOAT64 horizEye = 0.0f;
    static FLOAT64 vertEye = 0.0f;
    static FLOAT64 yangle = 0.0f;
    static FLOAT64 step = 0.0f;
    static FLOAT32 x, y;

    if(_startDeathTimer == 0)
    {
        switch(gr2AnimGetDeathState())
        {
        case TPS_DEATH1:
            {
                horizEye = 0;
                vertEye = 45.0f;
                yangle = 0;
                break;
            }
        case TPS_DEATH2:
            {
                horizEye = 0;
                vertEye = 32.0f;
                yangle = 80.0f;
                break;
            }
        case TPS_DEATH3:
            {
                horizEye = -16.0f;
                vertEye = -32.0f;
                yangle = -80.0f;
                break;
            }
        case TPS_DEATH4:
            {
                horizEye = -16.0f;
                vertEye = 16.0f;
                yangle = 90.0f;
                break;
            }
        default:
            {
                horizEye = 0;
                vertEye = 16.0f;
                yangle = 0;
                break;
           }
        }

        _startDeathTimer = frametime;

        x = (mathGetFloat32ChaoticNumber() * 2.0f - 1.0f) * playdir.x;
        y = (mathGetFloat32ChaoticNumber() * 2.0f - 1.0f) * playdir.y;
    }

    FLOAT64 minspeed = 60.0f;
    FLOAT64 maxspeed = 60.0f;

    fpFPSAdjustAngles(deltaTime, minspeed, maxspeed, &fpsTrooper.eyeHorizOffset, horizEye);

    fpFPSAdjustAngles(deltaTime, minspeed, maxspeed, &fpsTrooper.eyeVertOffset, vertEye);

    // move the model so it sticks with the eye
    fpFPSMoveModel();

    eye->_41 = x;
    eye->_42 = y;
    eye->_43 = TROOPER_STAND_EYE_HEIGHT + fpFPSGetHeight() + (FLOAT32)fpsTrooper.leanVertCurrent;

    piMATRIX rot = IdentityMatrix;

    FLOAT64 eyeRotation = fpsTrooper.eyeHorizOffset + 180.0f;

    generateRotationMatrix(AROUND_Z, eyeRotation, &rot);
    math3dMatrixMultiply(eye, &rot, eye);

    eyeRotation = fpsTrooper.eyeVertOffset;

    generateRotationMatrix(AROUND_X, eyeRotation, &rot);
    math3dMatrixMultiply(eye, &rot, eye);

    step += deltaTime * yangle * 0.09f;

    if(yangle < 0)
    {
        if(step < yangle)
        {
            step = yangle;
        }
    }
    else
    {
        if(step > yangle)
        {
            step = yangle;
        }
    }

    generateRotationMatrix(AROUND_Y, yangle, &rot);
    math3dMatrixMultiply(eye, &rot, eye);

	fpFPSAdjustEyeToPose(deltaTime);
}

void fpFPSGetEyeMatrix (piMATRIX* pMatrix)
{
	static piMATRIX	eyeMatrix;
	static SINT32	lastUpdateTime;

	// if asked again on the same frame, return the cached value
	// note that this also prevents all the extra stuff from getting called more
	// than once per frame  (handy!)
	if (lastUpdateTime == frametime)
	{
		if (pMatrix)
		{
			*pMatrix = eyeMatrix;
		}
		return;
	}

	piMATRIX		rotMatrix;
	FLOAT64			eyeRotation;
	FLOAT32			deltaTime = (frametime - lastUpdateTime)/1000.0f;
	FLOAT32			horizWobble, vertWobble, horizAngle, vertAngle;
	FLOAT32			maxUpAngle, maxDownAngle;

	// do all of our updates that only need to happen once per frame .. including
	// eye height, eye matrix calculation, rotation

    if(fpFPSIsDead())
    {
        if(pMatrix && gr2AnimGetDeathState())
        {
            _doDeath(deltaTime, pMatrix);
            eyeMatrix = *pMatrix;

            lastUpdateTime = frametime;
        }

        return;
    }

	mlVehicleData* pVehicleData = persGetPersonaModelDataPtr();
	fpFPSGetEyeUpDownRange (&maxUpAngle, &maxDownAngle);

	mouseLookOn = (mlMouseLookEnabled() && fpFPSMouseLookOK());

	// first thing, update the eye height if necessary
	fpFPSAdjustEyeToPose (deltaTime);

	// handle muzzle climb
	if (fpsTrooper.targetMuzzleRise > 0.0)
	{
		fpsTrooper.currentMuzzleRise = fpsTrooper.currentMuzzleRise + (deltaTime * fpsWeapons[currentWeaponModel].muzzleClimbRate);

		if (fpsTrooper.currentMuzzleRise > fpsTrooper.targetMuzzleRise)
			fpsTrooper.currentMuzzleRise = fpsTrooper.targetMuzzleRise;

		fpsTrooper.targetMuzzleRise -= fpsWeapons[currentWeaponModel].muzzleReturn * deltaTime;
	}
	else
	{
		fpsTrooper.targetMuzzleRise = 0.0;
		fpsTrooper.currentMuzzleRise = 0.0;
	}

	// handle FOV
	if ((!fpsWeapons[currentWeaponModel].sniperScope || currentFPSstate != AIM) && currentWeaponModel != BINOCS && FieldOfView != fpsTrooper.desiredFOV && fpsTrooper.desiredFOV > 0 &&
		(!(pVehicleData->PersonData->Flags & fpFLAGS_FORCE_EXTERNAL)))
	{
		FLOAT32 deltaFOV;

		deltaFOV = deltaTime * fpsTrooper.FOVSpeed;

		// persSetNormalView recalculates LOD for terrain objects every time it's called
		// might want to figure out how to get around this and only do it once for the target
		// FOV
		if (FieldOfView < fpsTrooper.desiredFOV)
		{
			// produce an immediate fov change if coming out of the scope
			if(fpsWeapons[currentWeaponModel].sniperScope)
				deltaFOV = fpsTrooper.desiredFOV;

			if ((FieldOfView + deltaFOV) > fpsTrooper.desiredFOV)
				persSetNormalView (fpsTrooper.desiredFOV);
			else
				persSetNormalView (FieldOfView + deltaFOV);
		}
		else // FoV > desired
		{
			if ((FieldOfView - deltaFOV) < fpsTrooper.desiredFOV)
				persSetNormalView (fpsTrooper.desiredFOV);
			else
				persSetNormalView (FieldOfView - deltaFOV);
		}
	}

	//uiChatPrintLine("FOV is now %f, Desired is %f", FieldOfView, fpsTrooper.desiredFOV);

	// handle head turn
	if (1)
	{
		FLOAT64		horizEyeTarget, vertEyeTarget;
		FLOAT64		minSpeed, maxSpeed;

		minSpeed = fpsTrooper.minEyeMovement;
		maxSpeed = fpsTrooper.maxHorizEyeAngle * fpsTrooper.eyeMoveMultiplier;

		if (stateFlags & STATE_LOOK_LEFT)
		{
			horizEyeTarget = -fpsTrooper.maxHorizEyeAngle;
		}
		else if (stateFlags & STATE_LOOK_RIGHT)
		{
			horizEyeTarget = fpsTrooper.maxHorizEyeAngle;
		}
		else if (mouseLookOn)
		{
			horizEyeTarget = mouseLookHorizAngle;
			minSpeed = 120.0f;
		}
		else
			horizEyeTarget = 0;

		if (mouseLookOn)
		{
			// workaround for inverted pitch crap when riding
			/*
			if (stateFlags & STATE_SIT)
				vertEyeTarget = -mouseLookVertAngle;
			else
			*/
				vertEyeTarget = mouseLookVertAngle;
		}
		else
		{
			vertEyeTarget = 0;
		}

		fpFPSAdjustAngles (deltaTime, minSpeed, maxSpeed, &fpsTrooper.eyeHorizOffset, horizEyeTarget);
		fpFPSAdjustAngles (deltaTime, minSpeed, maxSpeed, &fpsTrooper.eyeVertOffset, vertEyeTarget);

		// if we're not mouselooking and the mouselook & movement are on the same keymap
		// blow away the mouselook axis .. this is so when you exit mouselook it's recentered
		// but, once again, only do this if their on the same keymap
		if (!mouseLookOn)
		{
			if (mlMouseLookAndTorsoUseSameKeymap())
			{
				if (mouseLookHorizAngle != 0)
					mouseLookHorizAngle = fpsTrooper.eyeHorizOffset;
				if (mouseLookVertAngle != 0)
					mouseLookVertAngle = fpsTrooper.eyeVertOffset;
			}
		}

		if (fpsTrooper.eyeVertOffset > 0)
			aimVertAngleOffset = fpsTrooper.eyeVertOffset;
		else
			aimVertAngleOffset = 0;
	}

	// lean (first person leans are only movement, no angles)
	if (stat2Flags & STAT2_LEAN_LEFT)
	{
		fpsTrooper.leanHorizTarget = fpsTrooper.leanLeftHoriz;
		fpsTrooper.leanVertTarget = -fpsTrooper.leanLeftVert;
	}
	else if (stat2Flags & STAT2_LEAN_RIGHT)
	{
		fpsTrooper.leanHorizTarget = -fpsTrooper.leanRightHoriz;
		fpsTrooper.leanVertTarget = -fpsTrooper.leanRightVert;
	}
	else
	{
		fpsTrooper.leanHorizTarget = 0;
		fpsTrooper.leanVertTarget = 0;
	}

	fpFPSAdjustAngles (deltaTime, fpsTrooper.leanMinSpeed, fpsTrooper.leanMaxSpeed, &fpsTrooper.leanHorizCurrent, fpsTrooper.leanHorizTarget);
	fpFPSAdjustAngles (deltaTime, fpsTrooper.leanMinSpeed, fpsTrooper.leanMaxSpeed, &fpsTrooper.leanVertCurrent, fpsTrooper.leanVertTarget);

	if(controlStateFlags & FPS_BACKWARD)
		fpFPSAddWeaponLag(AROUND_X, WEAPON_LAG_MOVE);

	if(controlStateFlags & FPS_STRAFE_RIGHT)
		fpFPSAddWeaponLag(AROUND_Z, -WEAPON_LAG_MOVE);

	if(controlStateFlags & FPS_STRAFE_LEFT)
		fpFPSAddWeaponLag(AROUND_Z, WEAPON_LAG_MOVE);

	_updateWeaponLag(deltaTime);

	//_updateWeaponLag(deltaTime);

	eyeMatrix = IdentityMatrix;

	eyeMatrix._41 = 0.0f + (FLOAT32)fpsTrooper.leanHorizCurrent;
	eyeMatrix._42 = 0.0f;
	eyeMatrix._43 = TROOPER_STAND_EYE_HEIGHT + fpFPSGetHeight() + (FLOAT32)fpsTrooper.leanVertCurrent;
     
    if (fpsTrooper.parachuteOpen) {
        eyeMatrix._42 = -0.16f;
        eyeMatrix._43 += 0.19f;
    }

	if (stateFlags & STATE_SIT)
		eyeMatrix._42 = -fpsTrooper.eyeRideDisplacement;
	// 0.05729576041450060899707537401941 is atan (1/1000) .. i.e. the granularity of the axis
	// is 1 meter at 1000 meters

	// YAW control
	// why the hell the troopers are backwards .. geez
	eyeRotation = fpsTrooper.eyeHorizOffset + 180.0f;

	generateRotationMatrix (AROUND_Z, eyeRotation, &rotMatrix);
	math3dMatrixMultiply(&eyeMatrix,&rotMatrix,&eyeMatrix);

	// make sure the mortar is stabilized in the verticle plane
	if(fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_LTMORTAR && stateFlags & STATE_DEPLOYED)
		aimVertAngle = 0.0f;

	// PITCH control
	// no muzzle rise for pistols .. this is instead done in the firing animation itself
	if ((fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_AUTO_PISTOL) ||
		(fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_REVOLVER))
	{
		eyeRotation = aimVertAngle + fpsTrooper.eyeVertOffset;
	}
	else
	{
		eyeRotation = aimVertAngle - fpsTrooper.currentMuzzleRise + fpsTrooper.eyeVertOffset;
	}

	// sanity check here
	if(stateFlags & STATE_SIT)
	{
		FLOAT64 temp = -eyeRotation;

		if (temp > maxDownAngle)
			temp = maxDownAngle;
		else if (temp < maxUpAngle)
			temp = maxUpAngle;

		eyeRotation = -temp;
	}
	else
	{
		if (eyeRotation > maxDownAngle)
			eyeRotation = maxDownAngle;
		else if (eyeRotation < maxUpAngle)
			eyeRotation = maxUpAngle;
	}

	generateRotationMatrix (AROUND_X, eyeRotation, &rotMatrix);
	math3dMatrixMultiply(&eyeMatrix,&rotMatrix,&eyeMatrix);

/*
	// ROLL control
	if (stateFlags & STATE_SIT)
	{
		eyeRotation = 2.0 * -((pVehicleData->VehicleAtt.Roll * 180.0)/PI - 180.0);
		generateRotationMatrix (AROUND_Y, eyeRotation, &rotMatrix);
		math3dMatrixMultiply(&eyeMatrix,&rotMatrix,&eyeMatrix);
	}
*/

	if (((fpsWeapons[currentWeaponModel].staticDeploy) && (stateFlags & STATE_DEPLOYED))||((fpsWeapons[currentWeaponModel].sniperScope)&& (( currentFPSstate == AIM))))
	{
		piMATRIX		tempMatrix;

		fpFPSGetWobble (&horizWobble, &vertWobble, &horizAngle, &vertAngle);

		// translate
		tempMatrix = IdentityMatrix;

		tempMatrix._41 -= deployPosition.x;
		if (fpsWeapons[currentWeaponModel].sniperScope)
			tempMatrix._41 -= horizWobble;

		tempMatrix._42 -= deployPosition.y;
        if (fpsWeapons[currentWeaponModel].sniperScope)
			tempMatrix._42 -= vertWobble;

		tempMatrix._43 -= _deployZ();

		math3dMatrixMultiply(&eyeMatrix,&tempMatrix,&eyeMatrix);

		// rotate

		generateRotationMatrix (AROUND_Z, (horizAngle), &tempMatrix);
		math3dMatrixMultiply(&eyeMatrix,&tempMatrix,&eyeMatrix);
		generateRotationMatrix (AROUND_X, -(vertAngle), &tempMatrix);
		math3dMatrixMultiply(&eyeMatrix,&tempMatrix,&eyeMatrix);

		// translate back
		tempMatrix = IdentityMatrix;
		tempMatrix._41 += deployPosition.x;
		tempMatrix._42 += deployPosition.y;
		tempMatrix._43 += _deployZ();

		math3dMatrixMultiply(&eyeMatrix,&tempMatrix,&eyeMatrix);
	}

	// move the model so it sticks with the eye
	fpFPSMoveModel();

	if (pMatrix)
	{
		*pMatrix = eyeMatrix;
	}

	lastUpdateTime = frametime;
}

FLOAT32 fpFPSGetEyeAdjust(int pose, int move, bool asDiff)
{
	vehVehicle* v = persGetPersonaVehiclePtr();

	// default
	FLOAT32 height		= TROOPER_STAND_EYE_HEIGHT + fpFPSGetHeight();
	int currentPose		= TPS_POSETYPE_STAND;
	int currentMove		= TPS_MOVETYPE_IDLE;
	int currentPoint	= TPS_POINTTYPE_RELAXED;

	if(v &&
	   v->VehicleObjectData &&
	   v->VehicleObjectData->PlaneObject &&
	   v->VehicleObjectData->PlaneObject->animModel)
	{
		currentPose		= (pose > -1) ? pose: v->VehicleObjectData->PlaneObject->animModel->currentPose;
		currentMove		= (move > -1) ? move: v->VehicleObjectData->PlaneObject->animModel->currentMove;
		currentPoint	= v->VehicleObjectData->PlaneObject->animModel->currentPoint;
	}

	if(fpFPSGetPlayerPose() == POSE_STAND)
	{
		height = fpGetZAdjust(TPS_POSETYPE_STAND, TPS_MOVETYPE_STILL);
	}
	else if(fpFPSIsDead())
	{
		if(fpsTrooper.parachuteOpen) // para dead
			height = fpGetZAdjust(TPS_POSETYPE_STAND, TPS_MOVETYPE_IDLE);
		else
			height = fpGetZAdjust(-2, -1); // normal dead
	}
	else if(emoteID && emoteType == TPS_EMOTETYPE_FULLBODY)
	{
		height = fpGetZAdjust(-3, -1); // full body emote
	}
	else if(pose == TPS_POSETYPE_PRONE)
	{
		fpsWeaponTypes weap = fpsTrooper.weaponPositionModel[fpsTrooper.activeWeaponNumber];

		if(fpFPSIsDeployed())
			height = fpGetZAdjust(TPS_POSETYPE_PRONE, TPS_MOVETYPE_DEPLOYED);
		else if(fpFPSIsMoving(.1f))
			height = fpGetZAdjust(TPS_POSETYPE_PRONE, TPS_MOVETYPE_IDLE);
		else if((weap == BINOCS || fpFPSIsAim()) && !fpFPSIsSwimming())
			height = fpGetZAdjust(TPS_POSETYPE_PRONE, TPS_MOVETYPE_IDLE); // TPS_POINTTYPE_AIM
		else
			height = fpGetZAdjust(TPS_POSETYPE_PRONE, TPS_MOVETYPE_IDLE);
	}
	else if(currentMove == TPS_MOVETYPE_TURN)
	{
		if(fpFPSIsAim())
			height = fpGetZAdjust(currentPose, TPS_MOVETYPE_IDLE); // TPS_POINTTYPE_AIM
		else
			height = fpGetZAdjust(currentPose, TPS_MOVETYPE_IDLE);
	}
	else if(currentPoint == TPS_POINTTYPE_PARA)
	{
		// move our eyepoint up into our tps body's face
		height = fpGetZAdjust(TPS_POSETYPE_STAND, TPS_MOVETYPE_WALK) + .15f;
	}
	else
	{
		height = fpGetZAdjust(currentPose, currentMove, 0);
	}

	//Write_Log (LOG_FPS, LOG_INFO, "EYE HEIGHT: height = %.2f", height);

	return (asDiff) ? -TROOPER_STAND_EYE_HEIGHT + height : height;
}

// this is the master height control
float fpFPSGetHeight()
{
	return currentEyeHeight;
}

// this is only to be used by those needing to know what collider box to use!!
fpFPSPoses fpFPSGetPlayerPose()
{
	if (prevPose != targetPose)
		return POSE_TRANSITION;
	else if (stateFlags & STATE_SIT)
		return POSE_SIT;
	else if (stateFlags & STATE_SWIMMING)
		return POSE_PRONE;
	else if (stateFlags & STATE_EYE_STAND)
		return POSE_STAND;
	else if (stateFlags & STATE_EYE_CROUCH)
		return POSE_CROUCH;
	else if (stateFlags & STATE_EYE_PRONE)
		return POSE_PRONE;

	return POSE_STAND;
}

// this adds error to the fire angle based on the speed of the trooper, their pose,
// and eventually their ATP level
//
// this returns the number of degrees of error induced by the human holding the gun

FLOAT32 fpFPSGetBulletDispersion()
{
	FLOAT32			speed;				// range is 0.0 to 3.0
	FLOAT32			speedFactor;
	FLOAT32			minDispersion;		// minimum human dispersion on this weapon
										// a 0 here is a declaration that there is NO dispersion
	FLOAT32			maxDispersion;		// top end of the possible dispersion given the various factors
										// of speed, ATP, and position
	FLOAT32			currentATP;			// duh
	FLOAT32			atpFactor;			// multiplier imposed by ATP (1.0 is full ATP)

	minDispersion = fpsWeapons[currentWeaponModel].minimumDispersion;

	if (minDispersion == 0)
		return 0.0f;

	if (stateFlags & STATE_EYE_CROUCH)
	{
		minDispersion *= 2.0f;
	}
	else if (stateFlags & STATE_EYE_STAND)
	{
		minDispersion *= 4.0f;
	}
	else if (stateFlags & STATE_SWIMMING)
	{
		minDispersion *= 10.0f;
	}

	// PRONE doesn't have a multiplier for position

	// if you are not aiming and your not deployed, you have greater dispersion
	if(!(stateFlags & STATE_AIM) && !(stateFlags & STATE_DEPLOYED))
	{
		FLOAT32 dmod = fpsWeapons[currentWeaponModel].unaimedDispersionMod;

		if(gEZMode && EZ_MGR)
		{
			EZTrooper* et = dynamic_cast<EZTrooper*>(EZ_MGR);
			dmod *= et->GetDispersionMod();
		}

		minDispersion *= dmod;
	}

	mlVehicleData* pVehicleData = persGetPersonaModelDataPtr();
	speed = (FLOAT32) (mathExactVectorLength(&(pVehicleData->VehicleVel)));
	speedFactor = std::min(1.0f, (fpsWeapons[currentWeaponModel].speedMultiplier * speed * speed * speed));

	persGetHealthStats(nullptr, &currentATP, nullptr, nullptr);

	// at 100%, 0% extra error
	// at 90%, 23% extra error
	// at 80%, 56% extra error
	// at 50%, 300% extra error
	if (currentATP > fpsTrooper.atpMinDispersionATP)
		atpFactor = 1.0f / (currentATP * currentATP * currentATP);
	else
		atpFactor = 1.0f / (fpsTrooper.atpMinDispersionATP * fpsTrooper.atpMinDispersionATP * fpsTrooper.atpMinDispersionATP);

	maxDispersion = minDispersion * speedFactor * atpFactor * fpsWeapons[currentWeaponModel].controlability;

	if (maxDispersion <= minDispersion)
		maxDispersion = minDispersion;

	/*
	static FLOAT32 _dis = 0;

	if(_dis != maxDispersion)
	{
		_dis = maxDispersion;
		uiChatPrintLine("Dispersion: %.2f", _dis);
	}
	*/

//	Write_Log (LOG_FPS, LOG_INFO, "MaxDispersion = %f, min = %f", maxDispersion, minDispersion);

#if AIMING_WEAPONS
	return 0.0f;
#else
	return (maxDispersion);
#endif
}

FLOAT32	fpFPSsinc (FLOAT32 x)
{
	if (x == 0)
		return 1.0f;
	else
		return (FLOAT32)(sin (x)/x);
}

//
// returns the gun wobble based on the players ATP, pose, breathing, and movement
//
// what a mess of variables this is ..
//
#define FPS_REST_PULSE	(fpsTrooper.restPulse/60.0f)
#define	FPS_MAX_PULSE	(fpsTrooper.maxPulse/60.0f)
#define	NUM_PULSES		5

void fpFPSGetWobble(FLOAT32* horizWobble, FLOAT32* vertWobble, FLOAT32*	horizAngle, FLOAT32* vertAngle)
{
	static	SINT32	lastCalcTime;
	static	FLOAT32	lastHorizWobble, lastVertWobble;
	static	SINT32	nextPulseSlot;
	static	FLOAT32	lastPulseTime;
	static	FLOAT32	pulseTimes[NUM_PULSES];
	static	FLOAT32	pulseHoriz[NUM_PULSES];
	static	FLOAT32	pulseStr[NUM_PULSES];
//	static	FLOAT32	prevBreathValhoriz;
	static	FLOAT32	prevBreathValvert;
	static	FLOAT32	totalAimTime;
	static	FLOAT32	armXtarget, armYtarget, armTargetTime, armXspeed, armYspeed, armXpos, armYpos;

	SINT32			i;
	FLOAT32			speed;
	FLOAT32			motionMultiplier;		// kinda misnamed .. 0.5 is 2x the motion of 1.0
	FLOAT32			moveHoriz = 0.0f, moveVert = 0.0f;
	FLOAT32			heartHoriz = 0.0f, heartVert = 0.0f;
	FLOAT32			breathHoriz = 0.0f, breathVert = 0.0f;
	FLOAT32			armsHoriz = 0.0f, armsVert = 0.0f;
	FLOAT32			newHorizWobble = 0.0f, newVertWobble = 0.0f;
	FLOAT32			currentTime = (frametime/1000.0f);
	FLOAT32			deltaTime;
//	FLOAT32			breathWhoriz, breathValhoriz;
	FLOAT32			breathWvert, breathValvert;
	FLOAT32			atpAimFactor, armAimFactor;
	FLOAT32			currentATP, pulse;
	FLOAT32			armPoseFactor, prevArmXtarget, prevArmYtarget;
	FLOAT32			maxAimTime;
	FLOAT64VECTOR	wobbleVelVector;		// player velocity w/o z axis

//	char			temp[255];

//	temp[0] = 0;

	if ((!horizWobble) || (!vertWobble) || (!horizAngle) || (!vertAngle))
		return;
#if GUN_WOBBLE
	if ((stateFlags & STATE_DEPLOYED) && (fpsWeapons[currentWeaponModel].staticDeploy))
#endif
	{
		*horizWobble = 0;
		*vertWobble = 0;
		*horizAngle = (FLOAT32) (aimHorizontalDeployedPosition * fpsTrooper.mouseGranularity * fpsTrooper.deployedGranularityMultiplier);
		*vertAngle = (FLOAT32) (aimVerticalDeployedPosition * fpsTrooper.mouseGranularity * fpsTrooper.vertGranularityMultiplier * fpsTrooper.deployedGranularityMultiplier);
		return;
	}

	if (frametime == lastCalcTime)
	{
		*horizWobble = lastHorizWobble;
		*vertWobble = lastVertWobble;

		*horizAngle = lastHorizWobble * fpsTrooper.horizWobbleAngleMultiplier;
		if (*horizAngle < 0)
		{
			*horizAngle = std::max(*horizAngle, -fpsTrooper.maxWobbleAngle);
		}
		else
		{
			*horizAngle = std::min(*horizAngle, fpsTrooper.maxWobbleAngle);
		}

		*vertAngle = lastVertWobble * fpsTrooper.vertWobbleAngleMultiplier;
		if (*vertAngle < 0)
		{
			*vertAngle = std::max(*vertAngle, -fpsTrooper.maxWobbleAngle);
		}
		else
		{
			*vertAngle = std::min(*vertAngle, fpsTrooper.maxWobbleAngle);
		}
		return;
	}

	// this is set for animations which already include arm bounce ..
	// i.e. sprint and crawl
	if (stateFlags & STATE_NO_ARM_SWAY)
	{
		lastHorizWobble = 0.0f;
		lastVertWobble = 0.0f;
		*horizWobble = 0.0f;
		*vertWobble = 0.0f;
		*horizAngle = 0.0f;
		*vertAngle = 0.0f;
		return;
	}

	deltaTime = (frametime - lastCalcTime) / 1000.0f;
	persGetHealthStats(nullptr, &currentATP, nullptr, nullptr);

	bool aiming = (stateFlags & STATE_AIM);

	// accumulate aim time .. if not aiming, take it off at the rest rate
	if (aiming)
	{
		if ((totalAimTime == 0) && (fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_AUTO_RIFLE || ((fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_RIFLE) && (!(fpsWeapons[currentWeaponModel].auxEnable[FPS_AUX_RGCUP])))))
		{
			if(!persIsTired())
				fpsTrooper.breathSoundID = fpFPSfxPlayNormalSound (fpsTrooper.aimBreathSound, IDE_FP_PRIORITY_HIGH, TYPE_FINITE_SOUND, nullptr, persGetPersonaVehicleObjectDataPtr());
		}

		totalAimTime += deltaTime;
		maxAimTime = fpsTrooper.aimBreathTime + (std::max(fpsTrooper.timeToMaxHeartBreathMultiplier, fpsTrooper.timeToMaxPoseMultiplier));
		if (totalAimTime > maxAimTime)
			totalAimTime = maxAimTime;
		if (totalAimTime < fpsTrooper.aimBreathTime)
		{
			FLOAT32		localTime = fpsTrooper.aimBreathTime - totalAimTime;

			atpAimFactor = std::min(fpsTrooper.maxHeartBreathMultiplier, fpsTrooper.maxHeartBreathMultiplier * fpsTrooper.aimStartValue * ((localTime)/fpsTrooper.aimBreathTime));
			armAimFactor = std::min(fpsTrooper.maxPoseMultiplier, fpsTrooper.maxPoseMultiplier * fpsTrooper.aimStartValue * (localTime/fpsTrooper.aimBreathTime));
		}
		else if (totalAimTime > fpsTrooper.aimBreathTime)
		{
			FLOAT32		localTime = totalAimTime - fpsTrooper.aimBreathTime;

			atpAimFactor = std::min(fpsTrooper.maxHeartBreathMultiplier, fpsTrooper.maxHeartBreathMultiplier * (localTime/fpsTrooper.timeToMaxHeartBreathMultiplier));
			armAimFactor = std::min(fpsTrooper.maxPoseMultiplier, fpsTrooper.maxPoseMultiplier * (localTime/fpsTrooper.timeToMaxPoseMultiplier));
		}
		else
		{
			atpAimFactor = 0;
			armAimFactor = 0;
		}
	}
	else
	{
        if(fpsTrooper.breathSoundID > 0)
        {
            fxSndStopSound(fpsTrooper.breathSoundID);
            fpsTrooper.breathSoundID = 0;
        }

		totalAimTime -= deltaTime * fpsTrooper.rejuvTimeMultiplier;
		if (totalAimTime < 0)
			totalAimTime = 0;
		atpAimFactor = 1.0f;
		armAimFactor = 1.0f;
	}

	//
	// MOVEMENT
	//
	if (stateFlags & STATE_EYE_STAND)
	{
		motionMultiplier = (aiming) ? 1.2f : 0.55f;
	}
	else if (stateFlags & STATE_EYE_CROUCH)
	{
		motionMultiplier = (aiming) ? 1.2f : .55f;
	}
	else
	{
		motionMultiplier = 1.0f;
	}

	mlVehicleData*	pVehicleData = persGetPersonaModelDataPtr();

    if(!pVehicleData)
        return;

	wobbleVelVector = pVehicleData->VehicleVel;
	wobbleVelVector.z = 0;

	speed = (FLOAT32) (mathExactVectorLength(&wobbleVelVector));

    // Reduce superwobble silliness on normal walk
	if(aiming && (stateFlags & (STATE_EYE_STAND|STATE_EYE_CROUCH)))
	{
		speed *= 0.1f;
	}
    else if(stateFlags & (STATE_EYE_STAND|STATE_EYE_CROUCH))
    {
		speed *= 0.05f;
    }

#ifdef BETA_RELEASE
	if(SuperTroop)
		speed = std::min(speed, 3.0f);
#endif
	// horizontal shift is 0.03m (3 cm)
	// vertical shift is 0.045m (4.5 cm)

	// these numbers are, well, arbitrary ..
	moveHoriz = (FLOAT32)sin (frametime/(367.0f*motionMultiplier)) * 0.0050f * speed * fpsTrooper.moveMultiplier * fpsWeapons[currentWeaponModel].weaponMoveMultiplier;
	moveVert  = (FLOAT32)sin (frametime/(127.0f*motionMultiplier)) * 0.0075f * speed * fpsTrooper.moveMultiplier * fpsWeapons[currentWeaponModel].weaponMoveMultiplier;

	// if we're aiming reduce the motion
	if (stateFlags & STATE_AIM)
	{
		moveHoriz *= 0.1f;
		moveVert  *= 0.3f;
	}

	//
	// HEART BOUNCE
	//
	pulse = std::min(FPS_MAX_PULSE, (FPS_REST_PULSE + (1.0f - currentATP)*(1.0f/(1.0f - fpsTrooper.atpSprintMinimum))*(FPS_MAX_PULSE - FPS_REST_PULSE)));

	// time for another beat?
	if ((currentTime - lastPulseTime) > (1.0f / pulse))
	{
		pulseTimes[nextPulseSlot] = currentTime;
		pulseHoriz[nextPulseSlot] = (mathGetChaoticNumber() % 6) / 5.0f;
		pulseStr[nextPulseSlot] = currentATP;

		nextPulseSlot++;
		if (nextPulseSlot == NUM_PULSES)
			nextPulseSlot = 0;
		lastPulseTime = currentTime;
	}

	for (i=0; i < NUM_PULSES; i++)
	{
		FLOAT32	pulseVal = fpFPSsinc ((currentTime - pulseTimes[i]) * 0.25f * PI);

		heartHoriz += (pulseVal * 0.00025f * pulseHoriz[i])/(std::max(pulseStr[i], 0.2f));
		heartVert += (pulseVal * 0.0005f)/(std::max(pulseStr[i], 0.2f));
	}

	heartHoriz *= fpsTrooper.heartMultiplier * fpsWeapons[currentWeaponModel].weaponHeartMultiplier;
	heartVert *= fpsTrooper.heartMultiplier * fpsWeapons[currentWeaponModel].weaponHeartMultiplier;

	//
	// BREATH
	//

	breathWvert = PI2 * (pulse/fpsTrooper.pulseBreathRatio);
	breathValvert = prevBreathValvert + (breathWvert * deltaTime);
	prevBreathValvert = breathValvert;
	if (prevBreathValvert > (PI2))
		prevBreathValvert -= PI2;

//	breathWhoriz = PI2 * (pulse/2.0f);
//	breathValhoriz = prevBreathValhoriz + (breathWhoriz * deltaTime);
//	prevBreathValhoriz = breathValhoriz;
//	if (prevBreathValhoriz > (PI2))
//		prevBreathValhoriz -= PI2;

//	breathHoriz = (FLOAT32) sin (breathValhoriz) * 0.0005f;
	breathHoriz = 0;
	breathVert = (FLOAT32) sin (breathValvert) * 0.001f;

	breathHoriz *= fpsTrooper.breathMultiplier * fpsWeapons[currentWeaponModel].weaponBreathMultiplier;
	breathVert *= fpsTrooper.breathMultiplier * fpsWeapons[currentWeaponModel].weaponBreathMultiplier;

	//
	// POSE
	//
	if (stateFlags & STATE_EYE_STAND)
	{
		armPoseFactor = fpsTrooper.standPoseWobbleMultiplier;
	}
	else if (stateFlags & STATE_EYE_CROUCH)
	{
		armPoseFactor = fpsTrooper.kneelPoseWobbleMultiplier;
	}
	else
	{
		armPoseFactor = fpsTrooper.pronePoseWobbleMultiplier;
	}

	if (currentTime > armTargetTime)
	{
		armXpos = armXtarget;
		armYpos = armYtarget;

		prevArmXtarget = armXtarget;
		prevArmYtarget = armYtarget;

		// random number from -1.0 to 1.0
		armXtarget = (mathGetFloat32ChaoticNumber() + mathGetFloat32ChaoticNumber() - 1.0f)/2.0f;
		armYtarget = (mathGetFloat32ChaoticNumber() + mathGetFloat32ChaoticNumber() - 1.0f)/2.0f;

		// random number from 0.5 to 1.0
		armTargetTime = fpsTrooper.minArmWobbleTime +
			((fpsTrooper.maxArmWobbleTime - fpsTrooper.minArmWobbleTime) * ((mathGetFloat32ChaoticNumber() + mathGetFloat32ChaoticNumber())/2.0f));
		armXspeed = (armXtarget - prevArmXtarget) / armTargetTime;
		armYspeed = (armYtarget - prevArmYtarget) / armTargetTime;

		armTargetTime += currentTime;
	}
	else
	{
		armXpos = armXpos + (deltaTime * armXspeed);
		armYpos = armYpos + (deltaTime * armYspeed);
	}

	armsHoriz = armXpos * armPoseFactor * 0.001f * fpsTrooper.poseMultiplier * fpsWeapons[currentWeaponModel].weaponPoseMultiplier;
	armsVert = armYpos * armPoseFactor * 0.00225f * fpsTrooper.poseMultiplier * fpsWeapons[currentWeaponModel].weaponPoseMultiplier;

	newHorizWobble = moveHoriz + ((heartHoriz + breathHoriz) * atpAimFactor) + (armsHoriz * armAimFactor);
	newVertWobble = moveVert + ((heartVert + breathVert) * atpAimFactor) + (armsVert * armAimFactor);

	*horizWobble = newHorizWobble;
	*vertWobble = newVertWobble;
	lastHorizWobble = newHorizWobble;
	lastVertWobble = newVertWobble;

	if ((currentWeaponModel == MAS36) || (currentWeaponModel == RG_VB))
	{
		*vertWobble += -0.0005f;
		*horizWobble += -0.0001f;
	}

	*horizAngle = newHorizWobble * fpsTrooper.horizWobbleAngleMultiplier;

	// if we're not aiming reduce the angles (otherwise they get way too large)
	if (!(stateFlags & STATE_AIM))
	{
		*horizAngle *= 0.1f;
	}
	if (*horizAngle < 0)
	{
		*horizAngle = std::max(*horizAngle, -fpsTrooper.maxWobbleAngle);
	}
	else
	{
		*horizAngle = std::min(*horizAngle, fpsTrooper.maxWobbleAngle);
	}

	*vertAngle = newVertWobble * fpsTrooper.vertWobbleAngleMultiplier;
	if (*vertAngle < 0)
	{
		*vertAngle = std::max(*vertAngle, -fpsTrooper.maxWobbleAngle);
	}
	else
	{
		*vertAngle = std::min(*vertAngle, fpsTrooper.maxWobbleAngle);
	}
	if (!(stateFlags & STATE_AIM))
	{
		*vertAngle *= 0.3f;
	}

	lastCalcTime = frametime;

//	sprintf (temp, "horiz = %f, vert = %f", newHorizWobble, newVertWobble);
//	sprintf (temp, "Xt = %f, Yt = %f, Xp = %f, Yp = %f, Xs = %f, Ys = %f", armXtarget, armYtarget, armXpos, armYpos, armXspeed, armYspeed);
//	uiChatPrintLine (temp);

	return;
}

// if we're deployed, we need to find the new position of the mount point, determine the difference between it and the default mount point position, and move the
// model so the mount point doesn't move
void fpFPSMoveDeployedModel(piMATRIX* armsMatrix, piVECTOR* pos)
{
	piMATRIX		tempMatrix;
	FLOAT32			horizWobble, vertWobble, horizAngle, vertAngle;

	fpFPSGetWobble (&horizWobble, &vertWobble, &horizAngle, &vertAngle);

	// translate
	tempMatrix = IdentityMatrix;
	tempMatrix._41 += deployPosition.x;
	tempMatrix._42 += deployPosition.y;
	tempMatrix._43 += _deployZ();

	math3dMatrixMultiply(armsMatrix,&tempMatrix,armsMatrix);

	// rotate

	generateRotationMatrix (AROUND_Z, (horizAngle), &tempMatrix);
	math3dMatrixMultiply(armsMatrix,&tempMatrix,armsMatrix);
	generateRotationMatrix (AROUND_X, (vertAngle), &tempMatrix);
	math3dMatrixMultiply(armsMatrix,&tempMatrix,armsMatrix);

	// translate back
	tempMatrix = IdentityMatrix;
	tempMatrix._41 -= deployPosition.x;
	tempMatrix._42 -= deployPosition.y;
	tempMatrix._43 -= _deployZ();

	math3dMatrixMultiply(armsMatrix,&tempMatrix,armsMatrix);
}

void fpFPSMoveModel()
{
	//piVECTOR		pos;
	//piMATRIX		armsMatrix;
	piMATRIX		tempMatrix;
	FLOAT64			horizShift, vertShift;
	FLOAT32			horizWobblePos, vertWobblePos;
	FLOAT32			horizWobbleAngle, vertWobbleAngle;

	//
	// ATP/Pose/Aim gun wobble
	//
	// uses the utility function fpFPSGetWobble (float *, float *, float *, float*) which returns relative horizontal and vertical wobble
	// factors.  These factors are used elsewhere in different ways, and range from -1.0 to 1.0
	//
	// here we scale them to make appearances correct for first person
	//

	if (((stateFlags & STATE_DEPLOYED) && (fpsWeapons[currentWeaponModel].staticDeploy)) || (stateFlags & STATE_SIT))
	{
		horizWobblePos = vertWobblePos = horizWobbleAngle = vertWobbleAngle = 0;
	}
	else
	{
		fpFPSGetWobble (&horizWobblePos, &vertWobblePos, &horizWobbleAngle, &vertWobbleAngle);
	}

	horizShift = horizWobblePos + fpsTrooper.leanHorizCurrent;
	vertShift = vertWobblePos + fpsTrooper.leanVertCurrent;

	// eyeRotation is needed because the origin is offset from the point we want to rotate around ..
	// so .. we translate to origin, do our rotations, and translate back to where we belong, then
	// use the resulting rotations and translations to move the granny model

	eyeRotation = IdentityMatrix;
	memset (&eyePosition, 0, sizeof (eyePosition));

	// translate
	tempMatrix = IdentityMatrix;
	tempMatrix._41 = 0.0f;
	tempMatrix._42 = 0.0f;
	tempMatrix._43 = TROOPER_STAND_EYE_HEIGHT;

	math3dMatrixMultiply(&eyeRotation,&tempMatrix,&eyeRotation);

	// don't do these rotations if we're sitting .. the model doesn't move based on your eyepoint any more
	// instead it moves based on the vehicle you're sitting on
	if (!(stateFlags & STATE_SIT))
	{
		generateRotationMatrix (AROUND_Z, (horizWobbleAngle + horizWeaponLag), &tempMatrix);
		math3dMatrixMultiply(&eyeRotation,&tempMatrix,&eyeRotation);

		generateRotationMatrix (AROUND_X,
			-(-vertWobbleAngle + (AIM_VERTICAL_ANGLE + vertWeaponLag) - fpsTrooper.currentMuzzleRise),
			&tempMatrix);

		math3dMatrixMultiply(&eyeRotation,&tempMatrix,&eyeRotation);
	}

	// and translate again
	tempMatrix = IdentityMatrix;
	tempMatrix._41 = 0.0f;
	tempMatrix._42 = 0.0f;
	tempMatrix._43 = -TROOPER_STAND_EYE_HEIGHT;

	math3dMatrixMultiply(&eyeRotation,&tempMatrix,&eyeRotation);

	if ((fpsWeapons[currentWeaponModel].staticDeploy) && (stateFlags & STATE_DEPLOYED))
	{
		fpFPSMoveDeployedModel (&eyeRotation, &eyePosition);
	}

	eyePosition.x = eyeRotation._41 + (FLOAT32)horizShift;

	// fixme .. this seems to do nothing?
	if (stateFlags & STATE_SIT)
	{
		eyePosition.y = eyeRotation._42 + fpsTrooper.eyeRideDisplacement;
	}
	else
	{
		eyePosition.y = eyeRotation._42;
	}

	eyePosition.z = eyeRotation._43 + fpFPSGetHeight() + (FLOAT32)vertShift;
	//eyePosition.y += -2; // use this to see the 1st person stuff work in 3rd person

	gr2SetModelRotation (skeletonModel, &eyeRotation);
	gr2SetModelPosition (skeletonModel, &eyePosition);

	//_recoverWeaponLag();

	return;
}

// this is called every PHYSICS loop (50 times a second) so keep processing in here
// as minimal as possible

LOGICAL fpFPSUpdatePerson(mlVehicleData* pVehicleData, FLOAT64 deltaTime)
{
	static FLOAT32 lastRequestedForward = 0;
	static SINT32 speedTransition = 0;

	FLOAT32			currentATP, minATP;
	UINT32			movementStateFlags;
	FLOAT32			requestedForward, requestedStrafe, maxSpeed;
	fpsSpeedTypes	forwardSpeedValue;

	// make sure we have a pointer to the weapon the trooper is holding
	if (!currentGameWeapon)
	{
		//FIXME .. somehow.  this is ugly, but can't do it in the init process as the weapons haven't been loaded yet
		Write_Log (LOG_FPS, LOG_INFO, "warning:  weapon identification hack in progress");
		currentGameWeapon = &(((vehVehicle*)(pVehicleData->VehicleMasterPtr))->Weapons[fpsTrooper.activeWeaponNumber][0]);

		if(fpsWeapons[currentWeaponModel].weaponType != FPS_WEAPONTYPE_ROCKET)
			fpFPSUpdateAmmo(fpsTrooper.activeWeaponNumber, currentGameWeapon);

		// FIXME!  - expose this in data!
		if (fpsWeapons[currentWeaponModel].auxAvailable[FPS_AUX_RGCUP])
		{
			/* these are now exposed in the jmm files
			if (currentWeaponModel == RG_G30)
				currentGameWeapon->NumRoundsAmmoLeft[1] = 10;
			else if (currentWeaponModel == RG_VB)
				currentGameWeapon->NumRoundsAmmoLeft[1] = 10;
			else if (currentWeaponModel == RG_MILLS)
				currentGameWeapon->NumRoundsAmmoLeft[1] = 6;
			*/
		}

	}

	if (stateFlags & STATE_TURN_RIGHT)
    {
		pVehicleData->VehicleAtt.Yaw += (fpsTrooper.keyRotationSpeed*PI/180.0) * deltaTime;
	}
	else if (stateFlags & STATE_TURN_LEFT)
	{
		pVehicleData->VehicleAtt.Yaw -= (fpsTrooper.keyRotationSpeed*PI/180.0) * deltaTime;
	}

	if (fpsTrooper.parachuteOpen && !fpFPSIsDead())
	{
		FLOAT64 scaler = .1;

		if (stateFlags & STATE_LOOK_LEFT)
		{
			pVehicleData->VehicleAtt.Yaw += scaler * (fpsTrooper.keyRotationSpeed*PI/180.0) * deltaTime;
		}
		else if (stateFlags & STATE_LOOK_RIGHT)
		{
			pVehicleData->VehicleAtt.Yaw -= scaler * (fpsTrooper.keyRotationSpeed*PI/180.0) * deltaTime;
		}
	}

	// check for delayed shell ejection
	if (shellEjectTime && (shellEjectTime < frametime))
	{
		if (shellEjectTime < 0)
		{
			if (currentFPSstate == RECYCLE || currentFPSstate == RELOAD)
			{
				shellEjectTime = frametime + (SINT32)(fpsWeapons[currentWeaponModel].ejectDelay * 1000.0);
			}
		}
		else
		{
			fpFPSForceShellEjection();
			shellEjectTime = 0;
		}
	}

	// adjust the eye up/down angle based on limitations as dictated by the weapon
	// if we have non-zero up and down angles for the given eye height, do it
	if ((fpsWeapons[currentWeaponModel].weapDeployUpAngle[targetPose]) && (fpsWeapons[currentWeaponModel].weapDeployDownAngle[targetPose]))
	{
		FLOAT32 maxUpAngle, maxDownAngle;
		FLOAT64	 currentAngle;

		fpFPSGetEyeUpDownRange (&maxUpAngle, &maxDownAngle);
		currentAngle = AIM_VERTICAL_ANGLE;

		if (currentAngle > maxDownAngle)
			aimVertAngle = (currentAngle - 1.0f);
		else if (currentAngle < maxUpAngle)
			aimVertAngle = (currentAngle + 1.0f);

		if (currentAngle != AIM_VERTICAL_ANGLE)
		{
			fpFPSAdjustTurretElevation(pVehicleData, maxUpAngle, maxDownAngle);
		}
	}

	// make us look forward?
	if(stat2Flags & STAT2_HEAD_FIXED)
	{
		aimVertAngle = 0;
		stateFlags &= ~(STATE_LOOK_LEFT|STATE_LOOK_RIGHT);
	}

	if(fpsTrooper.wading > MAX_WADING_PRONE && fpFPSGetPlayerPose() == POSE_PRONE)
	{
		// if we crawl into water too deep for crawling, popup to a crouch
		fpFPSControlInput(1, IMAP_HOLD_FP_PRONE, 0);
	}
	else if(fpsTrooper.wading > MAX_WADING_KNEEL && fpFPSGetPlayerPose() == POSE_CROUCH)
	{
		// if we crouch moved into water to deep to crouch move, stand up
		fpFPSControlInput(1, IMAP_HOLD_FP_CROUCH, 0);
	}

	if(fpsTrooper.wading > MAX_WADING_SPRINT)
	{
		// don't run either
		if(stateFlags & STATE_MOVE_FAST)
		{
			stateFlags = stateFlags & ~STATE_MOVE_FAST;
			stateFlags = stateFlags |= STATE_MOVE_SLOW;
		}
	}
	else if((controlStateFlags & FPS_SPRINT) && !(stateFlags & STATE_MOVE_FAST))
	{
		// we're no longer in water and want to run, so lets do it
		if(!(stateFlags & STATE_NOSPRINT_MASK) && !(stateFlags & STATE_MOVE_FAST))
		{
			stateFlags = stateFlags & ~STATE_MOVE_SLOW;
			stateFlags = stateFlags |= STATE_MOVE_FAST;
		}
	}

	// handle movement
	movementStateFlags = stateFlags & STATE_MOVEMENT_MASK;

	persGetHealthStats(nullptr, &currentATP, nullptr, nullptr);

	if (stateFlags & STATE_WALK)
		forwardSpeedValue = FPS_SPEED_WALK;
	else
		forwardSpeedValue = FPS_SPEED_TROT;

	switch (movementStateFlags)
	{
		///////// standing

		// stand forward
		case STATE_MOVE_SLOW|STATE_EYE_STAND:
		case STATE_MOVE_SLOW|STATE_EYE_STAND|STATE_STRAFE_LEFT:
		case STATE_MOVE_SLOW|STATE_EYE_STAND|STATE_STRAFE_RIGHT:
			requestedForward = fpsTrooper.movementSpeeds[forwardSpeedValue];
			requestedStrafe  = fpsTrooper.movementSpeeds[forwardSpeedValue];
			maxSpeed         = fpsTrooper.movementSpeeds[forwardSpeedValue];
			minATP			 = 0.0;
			break;

		// stand forward strafe/aim
		case STATE_MOVE_SLOW|STATE_EYE_STAND|STATE_AIM:
		case STATE_MOVE_SLOW|STATE_EYE_STAND|STATE_STRAFE_LEFT|STATE_AIM:
		case STATE_MOVE_SLOW|STATE_EYE_STAND|STATE_STRAFE_RIGHT|STATE_AIM:
			requestedForward = fpsTrooper.movementSpeeds[FPS_SPEED_CREEP];
			requestedStrafe  = fpsTrooper.movementSpeeds[FPS_SPEED_CREEP];
			maxSpeed         = fpsTrooper.movementSpeeds[FPS_SPEED_CREEP];
			minATP			 = 0.0;
			break;

		// stand strafe
		case STATE_MOVE_STILL|STATE_EYE_STAND|STATE_STRAFE_LEFT:
		case STATE_MOVE_STILL|STATE_EYE_STAND|STATE_STRAFE_RIGHT:
			requestedForward = fpsTrooper.movementSpeeds[FPS_SPEED_STILL];
			requestedStrafe  = fpsTrooper.movementSpeeds[FPS_SPEED_QUICK];
			maxSpeed         = fpsTrooper.movementSpeeds[FPS_SPEED_QUICK];
			minATP			 = 0.0f;
			break;

		// stand strafe aim
		case STATE_MOVE_STILL|STATE_EYE_STAND|STATE_STRAFE_LEFT|STATE_AIM:
		case STATE_MOVE_STILL|STATE_EYE_STAND|STATE_STRAFE_RIGHT|STATE_AIM:
			requestedForward = fpsTrooper.movementSpeeds[FPS_SPEED_STILL];
			requestedStrafe  = fpsTrooper.movementSpeeds[FPS_SPEED_CREEP];
			maxSpeed         = fpsTrooper.movementSpeeds[FPS_SPEED_CREEP];
			minATP			 = 0.0f;
			break;

		// stand sprint
		case STATE_MOVE_FAST|STATE_EYE_STAND:
		case STATE_MOVE_FAST|STATE_EYE_STAND|STATE_AIM:
		case STATE_MOVE_FAST|STATE_EYE_STAND|STATE_STRAFE_LEFT:
		case STATE_MOVE_FAST|STATE_EYE_STAND|STATE_STRAFE_RIGHT:
			requestedForward = fpsTrooper.movementSpeeds[FPS_SPEED_SPRINT];
			requestedStrafe  = fpsTrooper.movementSpeeds[FPS_SPEED_SPRINT];
			maxSpeed         = fpsTrooper.movementSpeeds[FPS_SPEED_SPRINT];
			minATP			 = fpsTrooper.atpSprintMinimum;
			break;

		// stand backward
		case STATE_MOVE_BACKWARD|STATE_EYE_STAND:
		case STATE_MOVE_BACKWARD|STATE_EYE_STAND|STATE_AIM:
		case STATE_MOVE_BACKWARD|STATE_EYE_STAND|STATE_STRAFE_LEFT:
		case STATE_MOVE_BACKWARD|STATE_EYE_STAND|STATE_STRAFE_RIGHT:
		case STATE_MOVE_BACKWARD|STATE_EYE_STAND|STATE_STRAFE_LEFT|STATE_AIM:
		case STATE_MOVE_BACKWARD|STATE_EYE_STAND|STATE_STRAFE_RIGHT|STATE_AIM:
			requestedForward = -fpsTrooper.movementSpeeds[FPS_SPEED_BACKWARDS];
			requestedStrafe  = fpsTrooper.movementSpeeds[FPS_SPEED_BACKWARDS];
			maxSpeed         = fpsTrooper.movementSpeeds[FPS_SPEED_BACKWARDS];
			minATP			 = fpsTrooper.atpBackwardMinimum;
			break;

		//////// crouching

		// crouch forward
		case STATE_MOVE_SLOW|STATE_EYE_CROUCH:
		case STATE_MOVE_SLOW|STATE_EYE_CROUCH|STATE_STRAFE_LEFT:
		case STATE_MOVE_SLOW|STATE_EYE_CROUCH|STATE_STRAFE_RIGHT:
			requestedForward = fpsTrooper.movementSpeeds[FPS_SPEED_WALK];
			requestedStrafe  = fpsTrooper.movementSpeeds[FPS_SPEED_WALK];
			maxSpeed         = fpsTrooper.movementSpeeds[FPS_SPEED_WALK];
			minATP			 = fpsTrooper.atpCreepMinimum;
			break;

		// crouch forward aim
		case STATE_MOVE_SLOW|STATE_EYE_CROUCH|STATE_AIM:
		case STATE_MOVE_SLOW|STATE_EYE_CROUCH|STATE_STRAFE_LEFT|STATE_AIM:
		case STATE_MOVE_SLOW|STATE_EYE_CROUCH|STATE_STRAFE_RIGHT|STATE_AIM:
			requestedForward = fpsTrooper.movementSpeeds[FPS_SPEED_CREEP];
			requestedStrafe  = fpsTrooper.movementSpeeds[FPS_SPEED_CREEP];
			maxSpeed         = fpsTrooper.movementSpeeds[FPS_SPEED_CREEP];
			minATP			 = fpsTrooper.atpCreepMinimum;
			break;

		// crouch strafe
		case STATE_MOVE_STILL|STATE_EYE_CROUCH|STATE_STRAFE_LEFT:
		case STATE_MOVE_STILL|STATE_EYE_CROUCH|STATE_STRAFE_RIGHT:
			requestedForward = fpsTrooper.movementSpeeds[FPS_SPEED_STILL];
			requestedStrafe  = fpsTrooper.movementSpeeds[FPS_SPEED_CREEP];
			maxSpeed         = fpsTrooper.movementSpeeds[FPS_SPEED_CREEP];
			minATP			 = 0.0f;
			break;

		// crouch strafe aim
		case STATE_MOVE_STILL|STATE_EYE_CROUCH|STATE_STRAFE_LEFT|STATE_AIM:
		case STATE_MOVE_STILL|STATE_EYE_CROUCH|STATE_STRAFE_RIGHT|STATE_AIM:
			requestedForward = fpsTrooper.movementSpeeds[FPS_SPEED_STILL];
			requestedStrafe  = fpsTrooper.movementSpeeds[FPS_SPEED_CREEP];
			maxSpeed         = fpsTrooper.movementSpeeds[FPS_SPEED_CREEP];
			minATP			 = 0.0f;
			break;

		// crouch sprint
		case STATE_MOVE_FAST|STATE_EYE_CROUCH:
		case STATE_MOVE_FAST|STATE_EYE_CROUCH|STATE_AIM:
		case STATE_MOVE_FAST|STATE_EYE_CROUCH|STATE_STRAFE_LEFT:
		case STATE_MOVE_FAST|STATE_EYE_CROUCH|STATE_STRAFE_RIGHT:
			requestedForward = fpsTrooper.movementSpeeds[FPS_SPEED_TROT];
			requestedStrafe  = fpsTrooper.movementSpeeds[FPS_SPEED_TROT];
			maxSpeed         = fpsTrooper.movementSpeeds[FPS_SPEED_TROT];
			minATP			 = fpsTrooper.atpSprintMinimum;
			break;

		// crouch backward
		case STATE_MOVE_BACKWARD|STATE_EYE_CROUCH:
		case STATE_MOVE_BACKWARD|STATE_EYE_CROUCH|STATE_AIM:
		case STATE_MOVE_BACKWARD|STATE_EYE_CROUCH|STATE_STRAFE_LEFT:
		case STATE_MOVE_BACKWARD|STATE_EYE_CROUCH|STATE_STRAFE_RIGHT:
		case STATE_MOVE_BACKWARD|STATE_EYE_CROUCH|STATE_STRAFE_LEFT|STATE_AIM:
		case STATE_MOVE_BACKWARD|STATE_EYE_CROUCH|STATE_STRAFE_RIGHT|STATE_AIM:
			requestedForward = -fpsTrooper.movementSpeeds[FPS_SPEED_WALK];
			requestedStrafe  = fpsTrooper.movementSpeeds[FPS_SPEED_WALK];
			maxSpeed         = fpsTrooper.movementSpeeds[FPS_SPEED_WALK];
			minATP			 = fpsTrooper.atpCreepMinimum;
			break;

		// prone strafe
		case STATE_MOVE_STILL|STATE_EYE_PRONE|STATE_STRAFE_LEFT:
		case STATE_MOVE_STILL|STATE_EYE_PRONE|STATE_STRAFE_RIGHT:
			requestedForward = fpsTrooper.movementSpeeds[FPS_SPEED_STILL];
			requestedStrafe  = fpsTrooper.movementSpeeds[FPS_SPEED_CRAWL];
			maxSpeed         = fpsTrooper.movementSpeeds[FPS_SPEED_CRAWL];
			minATP			 = 0.0f;
			break;

		// prone forward
		case STATE_MOVE_SLOW|STATE_EYE_PRONE:
		case STATE_MOVE_SLOW|STATE_EYE_PRONE|STATE_STRAFE_LEFT:
		case STATE_MOVE_SLOW|STATE_EYE_PRONE|STATE_STRAFE_RIGHT:
			requestedForward = fpsTrooper.movementSpeeds[FPS_SPEED_CRAWL];
			requestedStrafe  = fpsTrooper.movementSpeeds[FPS_SPEED_CRAWL];
			maxSpeed         = fpsTrooper.movementSpeeds[FPS_SPEED_CRAWL];
			minATP			 = fpsTrooper.atpCrawlMinimum;
			break;

		// prone backward
		case STATE_MOVE_BACKWARD|STATE_EYE_PRONE:
		case STATE_MOVE_BACKWARD|STATE_EYE_PRONE|STATE_STRAFE_LEFT:
		case STATE_MOVE_BACKWARD|STATE_EYE_PRONE|STATE_STRAFE_RIGHT:
			requestedForward = -fpsTrooper.movementSpeeds[FPS_SPEED_CRAWL];
			requestedStrafe  = fpsTrooper.movementSpeeds[FPS_SPEED_CRAWL];
			maxSpeed         = fpsTrooper.movementSpeeds[FPS_SPEED_CRAWL];
			minATP			 = fpsTrooper.atpCrawlMinimum;
			break;

		// still
		default:
			requestedForward = fpsTrooper.movementSpeeds[FPS_SPEED_STILL];
			requestedStrafe  = fpsTrooper.movementSpeeds[FPS_SPEED_STILL];
			maxSpeed         = fpsTrooper.movementSpeeds[FPS_SPEED_STILL];
			minATP			 = 0.0f;
			break;
	}

	// set sign on strafe if we're going left (left is negative strafe)
	if (stateFlags & STATE_STRAFE_LEFT)
	{
		requestedStrafe = -requestedStrafe;
	}
	else if (stateFlags & STATE_STRAFE_RIGHT)
	{
	}
	else
		requestedStrafe = 0.0f;


	if(!speedTransition && lastRequestedForward && requestedForward > lastRequestedForward)
	{
		FLOAT32 diff = requestedForward - lastRequestedForward;
		SINT32 duration = (SINT32)(800 * diff/6.0f);
		speedTransition = frametime + duration;

		if(SuperTroop && speedTransition > 1000)
			speedTransition = 1000;
	}

	// disable speed transitions for now; causing wierd delays
	speedTransition = 0;

	if(speedTransition && frametime < speedTransition)
	{
		requestedForward = lastRequestedForward;
	}
	else
	{
		lastRequestedForward = requestedForward;
		speedTransition = 0;
	}

	if ((stateFlags & STATE_DEPLOYED) && (fpsWeapons[currentWeaponModel].staticDeploy))
	{
		requestedForward = 0.0f;
		requestedStrafe = 0.0f;
	}

	if (stateFlags & STATE_SWIMMING)
	{
		minATP = fpsTrooper.atpSwimMinimum;
	}

	if (currentATP < minATP)
	{
		requestedForward /= 2.0f;
		requestedStrafe /= 2.0f;
//		Write_Log (LOG_FPS, LOG_INFO, "ATP Slowdown! %f < %f", currentATP, minATP);
	}

	// now we have requestedStrafe and requestedForward, figure our real velocities using acceleration
	// and then cap it based on requestedForward

	if (((currentForwardVectorLength - requestedForward) * (currentForwardVectorLength - requestedForward))
		< (MIN_MOVE_DIFFERENCE * MIN_MOVE_DIFFERENCE))
	{
		currentForwardVectorLength = requestedForward;
	}
	else if (currentForwardVectorLength < requestedForward)
	{
		FLOAT32 tmod = (fpFPSGetPlayerPose() == POSE_TRANSITION) ? .5f: 1.0f;

		currentForwardVectorLength += (fpsTrooper.forwardAccel * tmod) * (FLOAT32)deltaTime;
		currentForwardVectorLength = std::min(currentForwardVectorLength, requestedForward);
	}
	else if (currentForwardVectorLength > requestedForward)
	{
		currentForwardVectorLength -= fpsTrooper.forwardAccel * (FLOAT32)deltaTime;
		currentForwardVectorLength = std::max(currentForwardVectorLength, requestedForward);
	}

	if (((currentStrafeVectorLength - requestedStrafe) * (currentStrafeVectorLength - requestedStrafe))
		< (MIN_MOVE_DIFFERENCE * MIN_MOVE_DIFFERENCE))
	{
		currentStrafeVectorLength = requestedStrafe;
	}
	else if (currentStrafeVectorLength < requestedStrafe)
	{
		currentStrafeVectorLength += fpsTrooper.strafeAccel * (FLOAT32)deltaTime;
		currentStrafeVectorLength = std::min(currentStrafeVectorLength, requestedStrafe);
	}
	else if (currentStrafeVectorLength > requestedStrafe)
	{
		currentStrafeVectorLength -= fpsTrooper.strafeAccel * (FLOAT32)deltaTime;
		currentStrafeVectorLength = std::max(currentStrafeVectorLength, requestedStrafe);
	}

	// only cap if we're moving in both directions
	// cap based on requestedForward
	if ((currentForwardVectorLength != 0.0f) && (currentStrafeVectorLength != 0.0f))
	{
		FLOAT32 speed = currentForwardVectorLength * currentForwardVectorLength + currentStrafeVectorLength * currentStrafeVectorLength;

		// don't do sqrt unless we have to, so check speed (currently speed*speed) vs (max*max)
		if(maxSpeed && speed > (maxSpeed * maxSpeed))
		{
			speed = (FLOAT32) sqrt(speed);

			// we really ought to decelerate here instead of snap
			// causes an anomaly when sprinting forward with strafe
			currentForwardVectorLength = currentForwardVectorLength * (maxSpeed / speed);
			currentStrafeVectorLength  = currentStrafeVectorLength  * (maxSpeed / speed);
		}
	}

	// swimming fun!
	if (!gCustomTerrain && fpFPSIsSwimming())
	{
		pVehicleData->VehiclePos.z += fpsTrooper.swimDisplacement;
	}

//	Write_Log (LOG_FPS, LOG_INFO, "Forward = %f, Strafe = %f .. actual %f, %f", requestedForward, requestedStrafe, currentForwardVectorLength, currentStrafeVectorLength);

	return TRUE;
}

void fpFPSGetVelocity (FLOAT64VECTOR* velocity)
{
//	mlVehicleData*	vehicleData = persGetPersonaModelDataPtr();
	fpPersonData*	personData = fpGetFPPersonDataPtr();
	static FLOAT32	driftX, driftY;
	FLOAT32			fallTime;
	FLOAT64VECTOR	xyVector;
	FLOAT64			speed;

	// if we're a falling paratrooper who exited from a plane..
	if (personData && (personData->Flags & fpFLAGS_FALLING) && fpsTrooper.isParatroop && (fpsTrooper.parachuteOKToOpen || fpsTrooper.parachuteOpen))
	{
		fallTime = (ft10mscount - personData->StartFallTime10ms)/100.0f;

		if (fallTime >= fpsTrooper.parachuteDecelerationTime)
		{
			// clear inherited velocity
			fpsTrooper.inheritedVehicleVelocity.x = 0;
			fpsTrooper.inheritedVehicleVelocity.y = 0;
			fpsTrooper.inheritedVehicleVelocity.z = 0;

			velocity->x = 0;
			velocity->y = 0;
			velocity->z = 0;

			// add drift
			if(fallTime > fpsTrooper.nextDriftTime)
			{
				driftX = 5.0f * mathGetFloat32ChaoticNumber();
				driftY = 5.0f * mathGetFloat32ChaoticNumber();
				fpsTrooper.nextDriftTime += 10.0f + 3.0f * mathGetFloat32ChaoticNumber();
			}
		}
		else
		{
			xyVector.x = fpsTrooper.inheritedVehicleVelocity.x;
			xyVector.y = fpsTrooper.inheritedVehicleVelocity.y;
			xyVector.z = 0;

			speed = mathExactVectorLength(&xyVector) * (1.0 - fallTime / fpsTrooper.parachuteDecelerationTime);

			velocity->x = 0;
			velocity->y = speed;
			velocity->z = fpsTrooper.inheritedVehicleVelocity.z * (1.0 - fallTime / fpsTrooper.parachuteDecelerationTime);

			// no drift while decelerating
			driftX = driftY = 0.0f;
		}

		Write_Log (LOG_FPS, LOG_INFO, "Fall time %f (%d - %d)  (%f, (%f, %f, %f)", fallTime,  ft10mscount, personData->StartFallTime10ms, fpsTrooper.parachuteDecelerationTime,
			velocity->x, velocity->y, velocity->z);

		// add drift
		velocity->x += driftX;
		velocity->y += driftY;
	}
	else
	{
		velocity->x = currentStrafeVectorLength + fpsTrooper.pushBackVelocity.x;
		velocity->y = currentForwardVectorLength + fpsTrooper.pushBackVelocity.y;
		velocity->z = 0.0;
	}

	// slow them down going up hill
	if(gCustomTerrain && _steepness)
	{
		FLOAT32 reduction = 1.0f;
		reduction -= (.8f * _steepness);
		velocity->x *= reduction;
		velocity->y *= reduction;
	}
}

LOGICAL fpFPSUpdateState ()
{
	// this block shouldn't hit
	if(currentWeaponModel == BUILD_TOOL && !fpPPOGet())
		fpFPSWeaponChange(0, 0);

	if(!fpFPSHandleControls ())
		return (FALSE);

	// determine our "new" state (doesn't have to change, but give it a chance to)
	fpsWeaponStates newState = fpFPSDetermineNextState ();

	fpFPSWeaponStateCheck(&newState);

	// if it didn't change .. business as usual
	if (newState == currentFPSstate)
		return TRUE;

	Write_Log (LOG_FPS, LOG_INFO, "State change from %d to %d, control 0x%x, state 0x%x", currentFPSstate, newState, controlStateFlags, stateFlags);

	// based on our current state, clean up things
	if (!fpFPSCleanupAnimationState ())
		return FALSE;

	// based on the new state, set up and start animations
	if (!fpFPSStartAnimationState (currentWeaponModel, newState))
		return FALSE;

	currentFPSstate = newState;

	return TRUE;
}

FPSColliderData& fpFPSGetCollider()
{
	return _collider;
}

void fpFPSCancelPPO()
{
	vehVehicle* player = persGetPersonaVehiclePtr();

	if(player && player->VehicleType->Type == VEH_TYPE_PERSON)
	{
		if(_ppoLastStance == POSE_PRONE)
		{
			controlStateFlags &= ~FPS_CROUCH;
			controlStateFlags |= FPS_PRONE;
		}
		else if(_ppoLastStance == POSE_CROUCH)
		{
			controlStateFlags |= FPS_CROUCH;
			controlStateFlags &= ~FPS_PRONE;
		}
		else
		{
			controlStateFlags &= ~FPS_CROUCH;
			controlStateFlags &= ~FPS_PRONE;
		}

		// switch back to primary weapon
		fpFPSWeaponChange(0, 0);
		//fpFPSControlInput(1, IMAP_HOLD_FP_WEAPON_1, 0);
		fpFPSUpdateState();
	}
}

void fpFPSSetSteepness(FLOAT32 s)
{
	_steepness = s;
}

//#define SPAM_CHATBOX

void InitColliderGeometry(void) {
	static bool initialized = false;
	if ( ! initialized ) {
		_standCollider.addGeomToCollideWith((dGeomID)colliderWorldSpace());
		_kneelCollider.addGeomToCollideWith((dGeomID)colliderWorldSpace());
		_proneCollider.addGeomToCollideWith((dGeomID)colliderWorldSpace());

		_updateFPSColliders();

		initialized = true;
	}
}

void fpFPSAreaChecks(FLOAT64VECTOR* v, FLOAT64 DeltaTime, bool force)
{
	// no need to do this if we're not in game
	if(!force && !uiInGame())
		return;

	static int _lastCheck = 0;

	InitColliderGeometry();

	if(stateFlags & STATE_SWIMMING)
		fpsTrooper.riverExit = TRUE;

	// are we prone
	bool prone = (fpFPSGetPlayerPose() == POSE_PRONE);

	vehVehicle* player = persGetPersonaVehiclePtr();
	if(player == nullptr) return;

	UINT32 newCollisionFlags = 0;
	StratDB::StratObject* captureStrat = nullptr;
	StratDB::StratObject* nofireStrat = nullptr;

	// setup rays
	Vec3d up				= Vec3d(0,0,1.0f);
	Vec3d pos				= Vec3d(player->ModelData->VehiclePos);
	Vec3d lpos				= Vec3d(player->ModelData->LastVehiclePos);
	Matrixd attitude		= Matrixd(player->ModelData->VehicleAttMat);
	Vec3d velocity			= Vec3d(player->ModelData->VehicleVel);
	Vec3d dir				= velocity;

	if(!dir.length2())
	{
		dir.set(0.0f, 1.0f, 0.0f);
		dir = dir * attitude;
	}

	velocity *= DeltaTime;

	const double maxZ		= player->ModelData->LastVehiclePos.z + MAX_FPS_STEP;
	double myZ				= pos.z();
	UINT32 ppoHit			= 0;

	// init our collision data
	_collider.groundHit.x	= pos.x();
	_collider.groundHit.y	= pos.y();
	_collider.groundHit.z	= pos.z();
	_collider.objectHit		= nullptr;

	// convert to the physical world
	pos = physicsGameToPhysical(pos);
	lpos = physicsGameToPhysical(lpos);

	bool recentRider = (fpsTrooper.lastUnrideTime && ((frametime - fpsTrooper.lastUnrideTime) <= 3000));
	bool considerVehicles = !recentRider;

	// how much to push over this run (because of cage hits)
	Vec3d push;

	// zero our z because we'll use this to calculate push vectors in the xy plane below
	pos.z() = 0;
	lpos.z() = 0;

	Vec3d posDiff = pos - lpos;

	FLOAT64 moved = posDiff.length();
	FLOAT32 wadeZ = -10000;
	FLOAT32 wading = 0;

	ScenarioVolume* currentVolume = nullptr;
	trnCDTerrainObject* currentTO = nullptr;

	// resuse dir to indicate our normalized velocity vector in the xy plane
	dir.z() = 0;
	dir.normalize();

	static Vec3d lastDir;
	static Vec3d lastNormal;

	if(_collider.blocked)
	{
		if(dir != lastDir)
			--_collider.blocked;

		// no change, return early
		if(_collider.blocked)
			return;
	}

	lastDir.set(dir.x(), dir.y(), dir.z());

	// make sure we have a current collider
	if(_collider.collider)
	{
		FPSCollider* collider = _collider.collider;

		// update our positions
		collider->Update(DeltaTime);

		UINT32 tHitCount	= 0;
		UINT32 pHitCount	= 0;
		//UINT32 gHitCount	= 0;
		//UINT32 gHitCount	= 0;
		UINT32 cHitCount	= 0;

		bool activeScenario = SCEN_MGR->IsActive();

		// see what we hit, what we're on, collision flags, push us back, etc...
		if(collider->findCollisions(true))
		{
			// get all collisions and loop over them for inspection
			const colliderQuery::CollisionList& hits = collider->getCollisions();
			const colliderQuery::Collision* best = nullptr;
			const colliderQuery::Collision* walk = nullptr;

			tHitCount += hits.size();

			// push us back vector
			Vec3d push;

			for(size_t i = 0; i < hits.size(); i++)
			{
				const colliderQuery::Collision& hit = hits.at(i);

				// get our flags
				UINT32 collideBits	= dGeomGetCollideBits(hit.g1);
				UINT32 categoryBits = dGeomGetCategoryBits(hit.g2);

				// g1 is always our collider
				// g2 is what we collided with
				// and COLLIDER_AREA for the box area collider otherwise

				switch(collideBits)
				{
					case WADE_COLLIDER:
					{
						// no swimming while in a custom terrain
						if(!gCustomTerrain || wading)
							continue;

						if(myZ >= hit.pos.z())
							continue;

						wadeZ = (float)hit.pos.z();

						FLOAT32 depth = wadeZ - (float)myZ;

						if(depth > MAX_WADING)
							wading = MAX_WADING;
						else
							wading = depth;
					}
					break;
					case FALL_COLLIDER:
					{
						if(gCustomTerrain && colliderIsType(categoryBits, COLLIDER_TRN_WATER))
							continue;

						//if(SuperTroop && !colliderIsType(categoryBits, COLLIDER_TILE))
						//	continue;

						if(hit.pos.z() > maxZ)
							continue;

						// normal must be up
						if((up * hit.normal) < 0)
							continue;

						bool isVehicle = colliderIsType(categoryBits, COLLIDER_VEHICLE_MASK);

						if(isVehicle && !considerVehicles)
							continue;

						bool isPlane = colliderIsType(categoryBits, COLLIDER_VEH_AIR);

						if(isPlane && (fpsTrooper.parachuteOKToOpen || fpsTrooper.parachuteOpen))
							continue;

						bool isWalk = colliderIsType(categoryBits, COLLIDER_WALK);

						if(isWalk && !considerVehicles)
							continue;

						// we hit something
						pHitCount++;

						if(isWalk)
						{
							if(!walk || hit.pos.z() > walk->pos.z())
								walk = &hit;
						}
						else// if(!isVehicle) // let them kill you
						{
							// no best, or higher z
							if(!best || hit.pos.z() > best->pos.z())
								best = &hit;
						}

						if(gCustomTerrain)
							continue;

						// get our flags
						if(!nofireStrat && colliderIsType(categoryBits, COLLIDER_NOFIRE) && fabs(hit.pos.z() - myZ) <= 1.0f)
						{
							trnCDTerrainObject* TO = colliderSpaceData<trnCDTerrainObject>(hit.g2);
							nofireStrat	= (TO) ? TO->StratObj() : nullptr;
						}

						if(!captureStrat && colliderIsType(categoryBits, COLLIDER_CAPTURE) && fabs(hit.pos.z() - myZ) <= 1.0f)
						{
							trnCDTerrainObject* TO = colliderSpaceData<trnCDTerrainObject>(hit.g2);
							captureStrat = (TO) ? TO->StratObj() : nullptr;
						}
					}
					break;
					case CAGE_COLLIDER:
					{
						if(gCustomTerrain && colliderIsType(categoryBits, COLLIDER_TRN_WATER))
							continue;

						// capsule colliders are currently only used in
						// the custom scenario terrains
						if(SuperTroop)
							continue;

						if(colliderIsType(categoryBits, COLLIDER_BLOCK))
						{
							// disregard blocks if no scenario is running
							if(!activeScenario)
								continue;

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
						else if(colliderIsType(categoryBits, COLLIDER_VOLUME))
						{
							// disregard blocks if no scenario is running
							if(!activeScenario || currentVolume)
								continue;

							ScenarioVolume* v = (ScenarioVolume*)dGeomGetData(hit.g2);

							// see if this volume pertains to us
							if(v && v != currentVolume)
							{
								// if it doesn't pertain, dont use it
								if(!SCEN_MGR->PlayerRelevant(v))
									continue;

								currentVolume = v;
							}

							// volume colliders are simply for detecting areas
							// so never use them to actually collide
							continue;
						}
						else if(!gCustomTerrain)
						{
							continue;
						}

						// check for any head hits
						if(false && hit.normal.z() <= -.9f)
						{
							double eyeHeight = myZ + fpFPSGetEyeAdjust(-1, -1, false);

							if(hit.pos.z() + hit.depth >= eyeHeight)
								_collider.blocked = true;
						}

						Vec3d normal = hit.normal;
						double depth = hit.depth;
						double angle = dir * normal;

						// start out by throwing out contacts with normals
						// in the direction of our velocity
						bool throwOut = (angle > .1);

						// if we are prone, disregard relatively flat hills
						if(!throwOut && prone)
						{
							// lower is more forgiving
							// 1 means must be flat
							static double raThreshold = .5f;

							double upAngle = up * normal;

							if(upAngle >= raThreshold)
								throwOut = true;
						}

						// handle downward facing normals with care
						if(velocity.length2() && !throwOut && normal.z() < 0)
						{
							FLOAT64 downThreshold = -.8f;

							if(normal.z() < downThreshold)
							{
								normal = -velocity;
								normal.normalize();
							}
							else
							{
								normal.z() = 0;
								normal.normalize();
							}
						}

						// throw out really steep normals
						//if(fabs(normal.z()) >= .9f)
						//	throwOut = true;
						/*
						{
							// recalculate the normal to be hit pos minus my pos on the xy plane
							Vec3d p1 = hit.pos;
							p1.z() = 0;
							normal = p1 - lpos;
							depth = DeltaTime;
						}
						*/

						// throw this contact out?
						if(throwOut)
						{
							colliderDebugContact(hit.pos, normal, colorBLACKALPHA, nullptr, depth);
							continue;
						}
						else
						{
							// our wrapped geom
							dGeomID geom = dGeomTransformGetGeom(hit.g1);

							int klass = dGeomGetClass(geom);

							// start with the hit position
							Vec3d thisPush = normal;

							// bring it to 0 like our pos vector above
							thisPush.z() = 0;

							// calculate the diff
							thisPush.normalize();

							if(klass == dRayClass)
							{
								depth = moved;
								thisPush *= depth; //dGeomRayGetLength(geom) - depth;
							}
							else
							{
								thisPush *= depth;
							}

							if(fabs(thisPush.x()) > fabs(push.x()))
								push.x() = thisPush.x();

							if(fabs(thisPush.y()) > fabs(push.y()))
								push.y() = thisPush.y();

							// debug it
							if(klass == dRayClass)
							{
								colliderDebugContact(hit.pos, normal, colorBLUEALPHA, nullptr, depth);
								//colliderDebugContact(hit.pos, thisPush, colorCYANALPHA, nullptr, hit.depth);
							}
							else
							{
								colliderDebugContact(hit.pos, normal, colorREDALPHA, nullptr, depth);
								//colliderDebugContact(hit.pos, thisPush, colorMAGENTAALPHA, nullptr, hit.depth);
							}

							if(false && lastNormal.length2())
							{
								double perp = lastNormal * hit.normal;
								static SINT32 runs = 5;
								static double threshold = .001f;

								// if we come across perpendicular or more normals then we are blocked
								if(perp <= threshold)
									_collider.blocked = runs;
							}

							// keep a running track of this tick's normal
							// so we can check for blocked conditions
							lastNormal = hit.normal;

							if(!currentTO && colliderIsType(categoryBits, COLLIDER_TERRAIN_MASK))
								currentTO = colliderSpaceData<trnCDTerrainObject>(hit.g2);

							cHitCount++;
						}
					}
					break;
					case AREA_COLLIDER:
					{
						if(gCustomTerrain && colliderIsType(categoryBits, COLLIDER_TRN_WATER))
							continue;

						if(!colliderIsType(categoryBits, COLLIDER_STO) || !colliderIsType(categoryBits, COLLIDER_HARDTERRAIN_MASK))
							continue;

						// only do one per frame when online and no scenario is running
						if(Online && activeScenario)
							continue;

						// is this a terrain sto
						trnCDTerrainObject* TO = colliderSpaceData<trnCDTerrainObject>(hit.g2);

						if(TO && TO->IsSTO())
						{
							ServerTrackedObject* sto = (ServerTrackedObject*)TO->TerrainMasterObject->lpspare;

							if(sto && sto->type() == ServerTrackedObject::_PPO)
								ppoHit = sto->uid();
						}
					}
					break;
				}
			}

			// reset this each tick
			fpsTrooper.pushBackVelocity.x = 0;
			fpsTrooper.pushBackVelocity.y = 0;

			if(push.length2())
			{
				pos.set(player->ModelData->VehiclePos);
				pos += push;
				pos.get(player->ModelData->VehiclePos);
			}

			if(!best && walk)
				best = walk;

			// only allow walking on ships
			if(walk && best && walk != best && colliderIsType(best->g2, COLLIDER_VEH_SEA))
			{
				vehVehicle* bestVehicle = colliderSpaceData<vehVehicle>(best->g2);
				vehVehicle* walkVehicle = colliderSpaceData<vehVehicle>(walk->g2);

				if(walkVehicle && walkVehicle->InUse && bestVehicle == walkVehicle)
					best = walk;
			}

			if(best)
			{
				// get our flags
				newCollisionFlags = dGeomGetCategoryBits(best->g2);

				_collider.groundHit.z		= best->pos.z();
				_collider.objectHit			= colliderData<OBJECT>(best->g2);
				_collider.groundMisses		= 0;

				if(wading == MAX_WADING)
					_collider.groundHit.z = wadeZ - wading;

				// wait until we find a good best before we start doing gravity
				fpsTrooper.doGravity = TRUE;

				// turn off this flag once the bank shallows out
				static FLOAT64 bank = .8f;

				if(fpsTrooper.riverExit)
				{
					if(!(stateFlags & STATE_SWIMMING))
					{
						if(best->normal.z() >= bank)
							fpsTrooper.riverExit = FALSE;
					}
				}
			}
		}
        else
        {
            // When we have no collider hits we don't want system thinking our current position is ground
            // reset it.
            _collider.groundHit.z	= 0;
        }

		if(!cHitCount)
			lastNormal.set(0,0,0);
	}
	else
	{
		lastNormal.set(0,0,0);
	}

	// calculate which ppos we need to tell the host about
	if(Online && ppoHit != _ppoLastHit)
	{
		if(_ppoLastHit && (frametime - _ppoCheck) >= 1000)
		{
			ServerTrackedObject* sto = wwiiGetSTO(_ppoLastHit);

			if(sto != nullptr && sto->type() == ServerTrackedObject::_PPO)
			{
				PlayerPlacedSTO* ppo = dynamic_cast<PlayerPlacedSTO*>(sto);
				ppo->leave();
			}

			_ppoLastHit = 0;
			_ppoCheck = 0;
		}
		else if(!_ppoLastHit && ppoHit)
		{
			ServerTrackedObject* sto = wwiiGetSTO(ppoHit);

			if(sto != nullptr && sto->type() == ServerTrackedObject::_PPO)
			{
				PlayerPlacedSTO* ppo = dynamic_cast<PlayerPlacedSTO*>(sto);
				ppo->enter();

				_ppoLastHit = ppoHit;
				_ppoCheck = frametime;
			}
		}
	}
	else if(Online && _ppoLastHit && ppoHit == _ppoLastHit)
	{
		_ppoCheck = frametime;
	}

#ifdef SPAM_CHATBOX

	static FLOAT64 gz = 0;

	if(!gz || player->ModelData->VehiclePos.z < gz || gz != _collider.groundHit.z)
	{
		//uiChatPrintLine("New Ground Height: %.2f/%.2f", gz, player->ModelData->VehiclePos.z);
		gz = _collider.groundHit.z;
	}
#endif

	/*
	static StratDB::StratObject* lastNofireStrat = nullptr;

	if(lastNofireStrat != nofireStrat)
	{
		if(nofireStrat)
			uiChatPrintLine("Found no fire strat object!");
		else
			uiChatPrintLine("NO no fire strat object found!");

		lastNofireStrat = nofireStrat;
	}
	*/

	if(nofireStrat)
	{
		while(nofireStrat && nofireStrat->StratClass() != SDM_CLASS_FACILITY)
			nofireStrat = nofireStrat->ParentStratObject();

		if(nofireStrat && nofireStrat->Side() != playerGetPlayer()->GetSide() && (nofireStrat->StratFlags() & CP_STAT_OPEN))
		{
			noFire = true;
			newCollisionFlags |= COLLIDER_NOFIRE;
		}
		else
		{
			noFire = false;
		}
	}
	else
	{
		noFire = false;
	}

	if(captureStrat)
		newCollisionFlags |= COLLIDER_CAPTURE;

	if(wading && gCustomTerrain)
	{
		newCollisionFlags |= COLLIDER_TRN_WATER;
		fpsTrooper.wading = wading;
	}
	else
	{
		fpsTrooper.wading = 0.0f;
	}

	if(currentVolume != _volumeCollider)
	{
		if(_volumeCollider)
			SCEN_MGR->RunObject(_volumeCollider->name.c_str(), 1, "out");

		if(currentVolume)
			SCEN_MGR->RunObject(currentVolume->name.c_str(), 1, "in");
	}

	_volumeCollider = currentVolume;

	// alert the manage we hit something
	if(currentTO)
		TUT_MGR->DoEvent(TUT_EVENT_COLLISION, currentTO);

	unsigned int difference = currentCollisionFlags ^ newCollisionFlags;

	// save our full mask
	currentCollisionFlags = newCollisionFlags;

	// send a game alert the first time we enter the nofire zone
	if(Online && noFire && _lastCheck != frametime)
	{
		if((difference & COLLIDER_NOFIRE) && (newCollisionFlags & COLLIDER_NOFIRE))
			uiGameEvent(GAMEEVENT_NOFIRE);
	}

	_lastCheck = frametime;
}


/*
void fpFPSAreaChecks(FLOAT64VECTOR* v, FLOAT64 DeltaTime, bool force)
{
	// no need to do this if we're not in game
	if(!force && !uiInGame())
		return;

	static int _lastCheck = 0;

	// initialize some stuff
	if(_lastCheck == 0)
		_updateFPSColliders();

	// reset out nofire flag
	noFire = false;

	vehVehicle* player = persGetPersonaVehiclePtr();
	if(player == nullptr) return;

	UINT32 newCollisionFlags = 0;
	StratDB::StratObject* captureStrat = nullptr;
	StratDB::StratObject* nofireStrat = nullptr;

	// setup rays
	Vec3d velocity			= Vec3d(player->ModelData->VehicleVel);
	Vec3d dir				= velocity;
	Vec3d pos				= Vec3d(player->ModelData->VehiclePos);
	Matrixd attitude		= Matrixd(player->ModelData->VehicleAttMat);

	velocity *= DeltaTime;

	// init our collision data
	collider.groundHit.x	= pos.x();
	collider.groundHit.y	= pos.y();
	collider.groundHit.z	= badZ;
	collider.objectHit		= nullptr;

	// convert to the physical world
	pos = physicsGameToPhysical(pos);

	colliderQuery::dGeomList& geoms = collider.geoms();

	dMatrix3 dAttitude;
	MatrixTodMatrix(attitude, dAttitude);

	int fpsRays = 0;

	if(!dir.length2())
	{
		dir.set(0.0f, 1.0f, 0.0f);
		dir = dir * attitude;
	}

	// update our other than ray geoms
	for(size_t i = 5; i < geoms.size(); i++)
	{
		dGeomID geom = geoms.at(i);

		if(dGeomGetClass(geom) == dRayClass)
		{
			dGeomRaySetLength(geom, .7f);
			dGeomRaySet(geom, pos.x(), pos.y(), pos.z() + .5f + (fpsRays * .1f), dir.x(), dir.y(), dir.z());
			fpsRays++;
		}
		else
		{
			dGeomSetPosition(geom, pos.x(), pos.y(), pos.z());
			dGeomSetRotation(geom, dAttitude);
		}
	}

	bool recentRider = (fpsTrooper.lastUnrideTime && ((frametime - fpsTrooper.lastUnrideTime) <= 3000));
	bool considerVehicles = !recentRider;

	UINT32 ceilingFlags = (considerVehicles) ? COLLIDER_MANMADE_MASK|COLLIDER_DRIVABLE_MASK : COLLIDER_MANMADE_MASK;

	Vec3d up = Vec3d(0, 0, 1.0f);
	float ceiling = (float)collideRay(ceilingFlags, .75f, pos, up, true);
	double maxContactZ = pos.z() + std::max(ceiling, .75f) - .1f;

	dir.set(0, 0, -1.0f);

	// move it up slightly so we don't fall through the ground.
	pos.z() += (recentRider) ? 1.75f : .75f;

	// update our ray positions
	{
		// copy it; we modify it below
		Vec3d rayPos = pos;

		// center ray; center is long, so we can move it up even further
		dGeomRaySet(collider.geom(0), rayPos.x() + .1f, rayPos.y(), rayPos.z(), dir.x(), dir.y(), dir.z());

		Vec3d forward		= Vec3d(0.0f, 1.0f, 0.0f);
		Vec3d right			= Vec3d(1.0f, 0.0f, 0.0f);

		// make a 90 degree rotation matrix
		Matrixd rotate;
		rotate.makeRotate(forward, right);

		// update our rays, rotating each as we go
		forward = forward * .4f;
		forward = forward * attitude;

		rayPos = rayPos + forward;
		dGeomRaySet(collider.geom(1), rayPos.x(), rayPos.y(), rayPos.z(), dir.x(), dir.y(), dir.z());

		forward = forward * rotate;
		rayPos = pos + forward;
		dGeomRaySet(collider.geom(2), rayPos.x(), rayPos.y(), rayPos.z(), dir.x(), dir.y(), dir.z());

		forward = forward * rotate;
		rayPos = pos + forward;
		dGeomRaySet(collider.geom(3), rayPos.x(), rayPos.y(), rayPos.z(), dir.x(), dir.y(), dir.z());

		forward = forward * rotate;
		rayPos = pos + forward;
		dGeomRaySet(collider.geom(4), rayPos.x(), rayPos.y(), rayPos.z(), dir.x(), dir.y(), dir.z());
	}

	int tHitCount = 0;
	int pHitCount = 0;

	UINT32 ppoHit = 0;

	// resuse dir to indicate our normalized velocity vector in the xy plane
	dir = velocity;
	dir.z() = 0;
	dir.normalize();

	bool activeScenario = SCEN_MGR->IsActive();

	double maxCapDepth = 0.0f;

	// see what we hit, what we're on, collision flags, push us back, etc...
	if(collider.findCollisions(true))
	{
		// get all collisions and loop over them for inspection
		const colliderQuery::CollisionList& hits = collider.getCollisions();
		const colliderQuery::Collision* best = nullptr;
		const colliderQuery::Collision* walk = nullptr;

		tHitCount += (int)hits.size();

		// push us back vector
		//colliderQuery::CollisionList pushes;
		UINT32 pcount = 0;
		Vec3d push;

		for(int i = 0; i < (int)hits.size(); i++)
		{
			const colliderQuery::Collision& hit = hits.at(i);

			// get our flags
			UINT32 category1 = dGeomGetCategoryBits(hit.g1);
			UINT32 category2 = dGeomGetCategoryBits(hit.g2);

			// g1 is always our collider
			// g2 is what we collided with
			// category1 will be 0 if our geom collider is a ray
			// category1 will be either COLLIDER_FPS for capsules
			// and COLLIDER_AREA for the box area collider otherwise

			// capsule colliders are currently only used in
			// the custom scenario terrains
			if(!gCustomTerrain && category1 == COLLIDER_FPS)
				continue;

			//std::vector<std::string> c1desc = colliderGetDescription(category1);
			//std::vector<std::string> c2desc = colliderGetDescription(category2);

			// no swimming while in a custom terrain
			if(gCustomTerrain && colliderIsType(category2, COLLIDER_TRN_WATER))
				continue;

			// only do one per frame when online and no scenario is running
			if(Online && _lastCheck != frametime && !ppoHit && colliderIsType(category2, COLLIDER_STO) && !activeScenario)
			{
				if(colliderIsType(category2, COLLIDER_HARDTERRAIN_MASK))
				{
					// is this a terrain sto
					trnCDTerrainObject* TO = colliderSpaceData<trnCDTerrainObject>(hit.g2);

					if(TO && TO->IsSTO())
					{
						ServerTrackedObject* sto = (ServerTrackedObject*)TO->TerrainMasterObject->lpspare;

						if(sto && sto->type() == ServerTrackedObject::_PPO)
							ppoHit = sto->uid();
					}
				}
			}

			// when a ray, do some extra flag stuff (when not running custom terrain)
			if(!category1)
			{
				if(!gCustomTerrain)
				{
					// get our flags
					if(!nofireStrat && colliderIsType(category2, COLLIDER_NOFIRE) && fabs(hit.pos.z() - myZ) <= 1.0f)
					{
						trnCDTerrainObject* TO = colliderSpaceData<trnCDTerrainObject>(hit.g2);
						nofireStrat	= (TO) ? TO->StratObj() : nullptr;
					}

					if(!captureStrat && colliderIsType(category2, COLLIDER_CAPTURE) && fabs(hit.pos.z() - myZ) <= 1.0f)
					{
						trnCDTerrainObject* TO = colliderSpaceData<trnCDTerrainObject>(hit.g2);
						captureStrat = (TO) ? TO->StratObj() : nullptr;
					}
				}

				if(hit.pos.z() > maxContactZ)
					continue;
			}

			// handle our z collider
			// record our highest hit; ray colliders wont have a category
			if(!category1 && colliderIsType(category2, GROUND_COLLISION_FLAGS))
			{
				// only use center ray for debugging
				//if(hit.g1 != collider.geom(0))
				//	continue;

				// normal must be up
				if((up * hit.normal) < 0)
					continue;

				bool isVehicle = colliderIsType(category2, COLLIDER_VEHICLE_MASK);

				if(isVehicle && !considerVehicles)
					continue;

				bool isPlane = colliderIsType(category2, COLLIDER_VEH_AIR);

				if(isPlane && (fpsTrooper.parachuteOKToOpen || fpsTrooper.parachuteOpen))
					continue;

				bool isWalk = colliderIsType(category2, COLLIDER_WALK);

				if(isWalk && !considerVehicles)
					continue;

				// we hit something
				pHitCount++;

				if(isWalk)
				{
					if(!walk || hit.pos.z() > walk->pos.z())
						walk = &hit;
				}
				else// if(!isVehicle) // let them kill you
				{
					// no best, or higher z
					if(!best || hit.pos.z() > best->pos.z())
						best = &hit;
				}
			}
			else if(!SuperTroop && category1 == COLLIDER_FPS && gCustomTerrain)
			{
				//if((dir * hit.normal) < .1f)
				//	continue;

				if(dGeomGetClass(hit.g1) != dRayClass)
					continue;

				if(colliderIsType(category2, COLLIDER_BLOCK))
				{
					// disregard blocks if no scenario is running
					if(!activeScenario)
						continue;

					ScenarioBlock* block = (ScenarioBlock*)dGeomGetData(hit.g2);

					// see if this block pertains to our vehicle type
					if(block && !SCEN_MGR->BlockVehicle(block))
						continue;
				}

				// don't collide with water if we're in a custom terrain
				if(gCustomTerrain && colliderIsType(category2, COLLIDER_TRN_WATER))
					continue;

				//static double depthThreshold = 10.1f;

				double angle = dir * hit.normal;

				// throw out contacts with normals in the direction of our velocity
				if(angle > .1)
				{
					colliderDebugContact(hit, colorBLACKALPHA, nullptr, hit.depth);
					continue;
				}
				else
				{
					if(dGeomGetClass(hit.g1) == dRayClass)
					{
						Vec3d hitNormal = hit.normal;

						hitNormal.z() = 0;
						hitNormal.normalize();

						hitNormal *= .7f - hit.depth;
						push += hitNormal;
					}
					else
					{
						Vec3d hitNormal = hit.normal;

						hitNormal.z() = 0;
						hitNormal.normalize();

						hitNormal *= hit.depth;
						push += hitNormal;
					}

					pcount++;


					// red in front, magenta in back
					if(hit.g1 == collider.geom(5))
						colliderDebugContact(hit, colorREDALPHA, nullptr, hit.depth);
					else if(hit.g1 == collider.geom(6))
						colliderDebugContact(hit, colorMAGENTAALPHA, nullptr, hit.depth);
					else
						colliderDebugContact(hit, colorBLUEALPHA, nullptr, hit.depth);
				}

				//pcount++;

				if(hit.depth > maxCapDepth)
					maxCapDepth = hit.depth;
			}
		}

		if(pcount)
		{
			pos.set(player->ModelData->VehiclePos);
			pos = physicsGameToPhysical(pos);

			Vec3d diff = pos;

			push /= (double)pcount;
			pos += push;

			diff = pos - diff;

			pos = physicsPhysicalToGame(pos);
			pos.get(player->ModelData->VehiclePos);

			// make our next tick not move any further
			diff.get(fpsTrooper.pushBackVelocity);
		}
		else
		{
			// reset this each tick
			fpsTrooper.pushBackVelocity.x = 0;
			fpsTrooper.pushBackVelocity.y = 0;
		}

		if(!best && walk)
			best = walk;

		// only allow walking on ships
		if(walk && best && walk != best && colliderIsType(best->g2, COLLIDER_VEH_SEA))
		{
			vehVehicle* bestVehicle = colliderSpaceData<vehVehicle>(best->g2);
			vehVehicle* walkVehicle = colliderSpaceData<vehVehicle>(walk->g2);

			if(walkVehicle && walkVehicle->InUse && bestVehicle == walkVehicle)
				best = walk;
		}

		if(best)
		{
			// get our flags
			newCollisionFlags = dGeomGetCategoryBits(best->g2);

			collider.groundHit.z		= best->pos.z();
			collider.objectHit			= colliderData<OBJECT>(best->g2);
			collider.groundMisses		= 0;
		}
	}

	// calculate which ppos we need to tell the host about
	if(Online && _lastCheck != frametime && ppoHit != _ppoLastHit)
	{
		if(_ppoLastHit)
		{
			ServerTrackedObject* sto = wwiiGetSTO(_ppoLastHit);

			if(sto != nullptr && sto->type() == ServerTrackedObject::_PPO)
			{
				PlayerPlacedSTO* ppo = dynamic_cast<PlayerPlacedSTO*>(sto);
				ppo->leave();
			}
		}

		if(ppoHit)
		{
			ServerTrackedObject* sto = wwiiGetSTO(ppoHit);

			if(sto != nullptr && sto->type() == ServerTrackedObject::_PPO)
			{
				PlayerPlacedSTO* ppo = dynamic_cast<PlayerPlacedSTO*>(sto);
				ppo->enter();
			}
		}

		// now we save it
		_ppoLastHit = ppoHit;
	}

	if(collider.groundHit.z == badZ)
	{
		++collider.groundMisses;

		if(true || (collider.groundMisses && collider.groundMisses <= 10))
		{
			// don't start falling until after the third miss
			collider.groundHit.z = player->ModelData->VehiclePos.z;
#ifdef SPAM_CHATBOX
			uiChatPrintLine("!!!Nothing hit on FPS area check: positive(%d) of total(%d)", pHitCount, tHitCount);
#endif
		}
	}

	static FLOAT64 gz = 0;

	if(gz != collider.groundHit.z)
	{
#ifdef SPAM_CHATBOX
		uiChatPrintLine("New Ground Height: %.2f/%.2f", gz, player->ModelData->VehiclePos.z);
#endif
		gz = collider.groundHit.z;
	}

	if(nofireStrat)
		newCollisionFlags |= COLLIDER_NOFIRE;

	if(captureStrat)
		newCollisionFlags |= COLLIDER_CAPTURE;

	unsigned int difference = currentCollisionFlags ^ newCollisionFlags;

	// save our full mask
	currentCollisionFlags = newCollisionFlags;

	// send a game alert the first time we enter the nofire zone
	if(Online && nofireStrat && _lastCheck != frametime)
	{
		const StratDB::StratObject* facility = nofireStrat;

		while(facility && facility->StratClass() != SDM_CLASS_FACILITY)
			facility = facility->ParentStratObject();

		if(facility && facility->Side() != playerGetPlayer()->GetSide())
		{
			//uiChatPrintLine("You are now in a no fire zone!");

			if(facility->StratFlags() & CP_STAT_OPEN)
				noFire = true;

			if(noFire && difference & COLLIDER_NOFIRE && newCollisionFlags & COLLIDER_NOFIRE)
				uiGameEvent(GAMEEVENT_NOFIRE);
		}
	}


	_lastCheck = frametime;
}
*/

		/*
		if(hit.normal.z() >= 0 && (newCollisionFlags & COLLIDER_HARDTERRAIN_MASK))
		{
			double cosTheta = hit.normal * dir;

			// cap it
			cosTheta = (cosTheta < -1.0f) ? -1.0f : (cosTheta > 1.0f) ? 1.0f : cosTheta;

			double angle = mlRAD_TO_DEG(acos(cosTheta));
			double threshold = 45.0f;

			if(angle >= threshold)
			{
				static double delta = .1f;
				static double speed = 3.0f;

				Vec3d axis = hit.normal ^ dir;

				Matrixd rotator;
				rotator.makeRotate(90.0f, axis);

				// rotate the normal 90 downhill 90
				dir = hit.normal * rotator;
				dir *= (angle / 90.0f) * speed * delta;
				dir *= -1.0f;

				// move the player in that direction
				persGetPersonaVehiclePtr()->ModelData->VehiclePos.x += dir.x();
				persGetPersonaVehiclePtr()->ModelData->VehiclePos.y += dir.y();
				persGetPersonaVehiclePtr()->ModelData->VehiclePos.z += dir.z();
			}
		}
		*/

LOGICAL fpFPSUpdate ()
{
    if(ShakeCamMagnitude)
    {
        persShakeCamera(ShakeCamMagnitude, ShakeDuration);

        if(ShakeDuration >= 1.0)
        {
            float t = (float)(frametime - ShakeStart) / (float)ShakeDuration;
            t = t > 1.0f ? 1.0f : t;

            ShakeCamMagnitude *= 1.0f - t;
            ShakeCamMagnitude = ShakeCamMagnitude < 0.0f ? 0.0f : ShakeCamMagnitude;

            if(frametime - ShakeStart >= ShakeDuration)
                ShakeCamMagnitude = 0.0f;
        }
        else
        {
            ShakeCamMagnitude = 0.0f;
            ShakeDuration = 0;
            ShakeStart = 0;
        }
    }

	if (!fpsTrooper.initialized)
		return TRUE;

    if(currentBuilderObject)
    {
        if(!fpFPSTestBuilderValidity())
        {
            fpFPSBuilderEnd();
        }
    }

	//fpFPSAreaChecks();

	// state update
	// DEG
	int changed = fpFPSUpdateState();

	if(gEZMode && EZ_MGR)
	{
		FLOAT32 hl = (FLOAT32)fabs(horizWeaponLag / WEAPON_LAG_HMAX);
		FLOAT32 vl = (FLOAT32)fabs(vertWeaponLag / WEAPON_LAG_VMAX);

		EZTrooper* et = dynamic_cast<EZTrooper*>(EZ_MGR);
		et->SetDispersion((hl > vl) ? hl : vl);
	}

	if(!changed)
		return (FALSE);

	// go ahead and update the eye here too
	fpFPSGetEyeMatrix (nullptr);

	// Handle Weapon Firing
	fpFPSWeaponFire(persGetPersonaModelDataPtr());

#ifdef STOP_FIRING_INSIDE
	// end of frame, clear the "hit vehicle" flag
	fpsHitVehicleThisFrame = FALSE;
#endif

	return TRUE;
}

void fpFPSUpdateModels()
{
	if(skeletonModel)
		gr2UpdateModel(skeletonModel, true);

	if (armsModel)
		gr2UpdateModel(armsModel, true);

	//if(currWeapControl)
	//	gr2UpdateControlEvents(currWeapControl);

	if (weapModel)
		gr2UpdateModel(weapModel, true);

	if (clipModel)
		gr2UpdateModel(clipModel, true);

	if (legsModel)
		gr2UpdateModel(legsModel, true);

	for (int i=0; i < FPS_NUM_AUX_MODELS; i++)
	{
		if (auxModel[i])
		{
			// move this to update model
			if(i == FPS_AUX_RGCUP && gr2IsModelActive(auxModel[FPS_AUX_RGCUP]))
			{
				gr2SetModelRotation (auxModel[FPS_AUX_RGCUP], &eyeRotation);
				gr2SetModelPosition (auxModel[FPS_AUX_RGCUP], &eyePosition);
			}

			gr2UpdateModel(auxModel[i], true);
		}
	}
}

LOGICAL fpFPSIsExternalView()
{
	if (persIsInDeathCam())
		return true;
	return (fpGetFPPersonDataPtr()->Flags & fpFLAGS_FORCE_EXTERNAL);
}

LOGICAL fpFPSDraw(OBJECT* pObject, LOGICAL render)
{
	// the next few calls have internal checks so they don't run more than once a frame
	// we go ahead and let them run on both the shadow and non shadow runs so they
	// are updated as early as possible in the frame

	static SINT32 lastUpdate = 0;
	SINT32 i = 0;

	if(lastUpdate != frametime)//obdrawIsRenderPass(RENDER_SCENE))
	{
		if(!fpFPSUpdateState())
			return FALSE;

		if(skeletonModel)
			gr2UpdateModel(skeletonModel, true);

		if (armsModel)
			gr2UpdateModel(armsModel, true);

		if (weapModel)
			gr2UpdateModel(weapModel, true);

		if (clipModel)
			gr2UpdateModel(clipModel, true);

		if (legsModel)
			gr2UpdateModel(legsModel, true);

		// since at this point we've already handled all the input, and everything is ready to go, now
		// we can look at what we've collected and determine if we need to change animations

		for (i=0; i < FPS_NUM_AUX_MODELS; i++)
		{
			if (auxModel[i])
			{
				// test code
				/*
				if(currentWeaponModel == RG_G30 && i == FPS_AUX_RGCUP && gr2IsModelActive(auxModel[i]))
				{
					piMATRIX tempMatrix, finalMatrix;

					gr2GetModelFinalTransform(skeletonModel, &finalMatrix);

					tempMatrix		= IdentityMatrix;
					tempMatrix._41	= -finalMatrix._41;
					tempMatrix._42	= -finalMatrix._42;
					tempMatrix._43	= -finalMatrix._43;

					// translate
					math3dMatrixMultiply(&finalMatrix,&tempMatrix,&finalMatrix);

					gr2SetModelRotation(auxModel[i], &finalMatrix);

					*
					math3dMatrixMultiply(&finalMatrix,&tempMatrix,&finalMatrix);


					piVECTOR pos;

					gr2GetModelPosition(auxModel[i], &pos);

					tempMatrix = finalMatrix = IdentityMatrix;

					// translate to 0
					tempMatrix._41 = -pos.x;
					tempMatrix._42 = -pos.y;
					tempMatrix._43 = pos.z;

					math3dMatrixMultiply(&finalMatrix,&tempMatrix,&finalMatrix);

					tempMatrix = *skeletonModel->finalModelTransform;

					// don't do these rotations if we're sitting .. the model doesn't move based on your eyepoint any more
					// instead it moves based on the vehicle you're sitting on
					if (!(stateFlags & STATE_SIT))
					{
						generateRotationMatrix (AROUND_Z, (horizWobbleAngle), &tempMatrix);
						math3dMatrixMultiply(&armsMatrix,&tempMatrix,&armsMatrix);
						generateRotationMatrix (AROUND_X,
								-(-vertWobbleAngle + (AIM_VERTICAL_ANGLE) - fpsTrooper.currentMuzzleRise),
								&tempMatrix);
						math3dMatrixMultiply(&armsMatrix,&tempMatrix,&armsMatrix);
					}

					// and translate again
					tempMatrix = IdentityMatrix;
					tempMatrix._41 = 0.0f;
					tempMatrix._42 = 0.0f;
					tempMatrix._43 = -TROOPER_STAND_EYE_HEIGHT;

					math3dMatrixMultiply(&armsMatrix,&tempMatrix,&armsMatrix);
					*
				}
				*/

				// move this to update model
				if(i == FPS_AUX_RGCUP && gr2IsModelActive(auxModel[FPS_AUX_RGCUP]))
				{
					gr2SetModelRotation(auxModel[FPS_AUX_RGCUP], &eyeRotation);
					gr2SetModelPosition(auxModel[FPS_AUX_RGCUP], &eyePosition);
				}

				gr2UpdateModel(auxModel[i], true);
			}
		}

		lastUpdate = frametime;
	}

#ifdef TESTING_FPS
	const bool debugDraw = true;
#else
	const bool debugDraw = false;
#endif

	fpPersonData* personData = fpGetFPPersonDataPtr();

	if ((debugDraw) || ( (render) && (!persIsInDeathCam() ) && (!(personData->Flags & fpFLAGS_FORCE_EXTERNAL))))
	{
		if (skeletonModel)
		{
			if (!gr2RenderModel(skeletonModel))
				return (FALSE);
		}

		if (armsModel)
		{
			if (showArms)
			{
				if (!gr2RenderModel(armsModel))
					return (FALSE);
			}
		}

		if (weapModel)
		{
			if (showGun)
			{
				if (!gr2RenderModel(weapModel))
					return (FALSE);
			}
		}

		if (clipModel)
		{
			if (showClip)
			{
				if (!gr2RenderModel(clipModel))
					return (FALSE);
			}
		}

		if (legsModel)
		{
			if (!gr2RenderModel(legsModel))
				return (FALSE);
		}

		for (i=0; i < FPS_NUM_AUX_MODELS; i++)
		{
			if (auxModel[i] && showAux[currentWeaponModel][i])
			{
				if (!gr2RenderModel(auxModel[i]))
					return (FALSE);
			}
		}
	}

    if(obdrawIsRenderPass(RENDER_SCENE))
    {
		//if(!Online)
		//	fpFPSSetAnimationFlags();

	    // now, handle the local 3rd person stuff (troop in hud, external view require this)
//        gr2AnimProcessAnimationFlags(pObject, personData->AnimationFlags);
    }

	return (TRUE);
}

LOGICAL fpFPSIsWeaponSwitchOK (SINT32 weaponNumber)
{
	assert( weaponNumber < MAX_NUM_FPS_WEAPONS );

	if (stateFlags & (STATE_DEAD | STATE_NEEDS_RECYCLE | STATE_DEPLOYING))
		return FALSE;

	switch (currentFPSstate)
	{
		case RELOAD:
		case RELOAD2:
		case RELOAD3:
		case STAND_FIRE:
		case CROUCH_FIRE:
		case AIM_FIRE:
		case RECYCLE:
		case THROW:
		case PRONE_FIRE:
		case DEPLOYED_RELOAD:
		case DEPLOYING:
		case UNDEPLOYING:
			return FALSE;
		/*
		case LOWER:
			if (!(stateFlags & STATE_RELAXED))
				return FALSE;
		*/
        default:
            // we want all other states to fall through, switch is allowed
            break;
	}

	if (false && stateFlags & STATE_WEAPON_SWITCHING)
		return FALSE;

	// allow switching if your sitting except to binocs
	if ((stateFlags & STATE_SIT) && (fpsTrooper.weaponPositionModel[weaponNumber] == BINOCS))
		return FALSE;

	if (weaponNumber == fpsTrooper.activeWeaponNumber)
		return FALSE;

	if (fpsTrooper.weaponPositionModel[weaponNumber] == NO_WEAPON)
	{
		return FALSE;
	}

	if (fpsTrooper.weaponPositionModel[weaponNumber] == MAP)
	{
		return FALSE;
	}

	if(stateFlags & (STATE_HOLD_FIRE|STATE_FIRE_DELAYED_WEAP))
		return FALSE;

	// Cannot switch away from grenadier with the cup attached
	if ( fpsWeapons[currentWeaponModel].auxEnable[FPS_AUX_RGGRENADE] ||
	// Cannot switch away from loaded rocket launcher
	     (fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_ROCKET && fpsWeapons[currentWeaponModel].clipBullets != 0) )
	{
		persShakeHead();
		return FALSE;
	}

	if(fpsTrooper.weaponPositionModel[weaponNumber] == BUILD_TOOL)
	{
		/*
		// Don't allow PPO tool while swimming
		if(fpFPSIsSwimming())
		{
			//fpPPOAlert("ppos.swimming");
			return FALSE;
		}
		else return TRUE;
		*/

		return TRUE;
	}

	if (fpsTrooper.weaponPositionModel[weaponNumber] == BINOCS)
	{
		if (!fpsTrooper.binocsEnabled)
			return FALSE;

		// no binocs if any movement
		mlVehicleData*	vehicleData = persGetPersonaModelDataPtr();
		if ((vehicleData->VehicleVel.x * vehicleData->VehicleVel.x +
			vehicleData->VehicleVel.y * vehicleData->VehicleVel.y +
			vehicleData->VehicleVel.z * vehicleData->VehicleVel.z) > 0.0)
		{
			return FALSE;
		}

		return TRUE;
	}

	// deployed weapons have additional restrictions with these weapons
	if((stateFlags & STATE_DEPLOYED) &&
		(!fpFPSWeaponOutOfAmmo(currentGameWeapon) || fpsTrooper.weaponPositionModel[weaponNumber] == BUILD_TOOL) )
	{
		bool ok = false;
		fpsWeaponTypes weapType = NO_WEAPON;

		for(int i = 0; i < MAX_NUM_FPS_WEAPONS; i++)
		{
			weapType = fpsTrooper.weaponPositionModel[i];

			if(fpsWeapons[weapType].deployPivotBone)
			{
				if(weaponNumber == i)
					ok = true;

				break;
			}
			else if(fpsWeapons[weapType].auxEnable[FPS_AUX_RGCUP])
			{
				ok = true;
				break;
			}
		}

		if(!ok) return FALSE;
	}

	vehVehicle* playerVehicle = persGetPersonaVehiclePtr();

	if(playerVehicle && (fpFPSWeaponOutOfAmmo(playerVehicle->Weapons[weaponNumber]) && fpsTrooper.weaponPositionModel[weaponNumber] != BUILD_TOOL) )
	{
		if(fpsTrooper.autoswitchWeaponNumber == weaponNumber)
			fpFPSWeaponAutoSwitch();

		return FALSE;
	}

	return TRUE;
}

// weaponNumber will be from 0 to 9
void fpFPSWeaponChange (SINT32 weaponNumber, SINT32 override)
{
	// grab the vehicle data
	mlVehicleData* vehicleData = persGetPersonaModelDataPtr();

	// mortars use keys 1 and 2 to switch between ammo types
	// weapon 1 is alway the weapon in use though, it holds the ammo
	fpFPSWeaponType weaponType = fpsWeapons[fpsTrooper.weaponPositionModel[weaponNumber]].weaponType;

	if(weaponType == FPS_WEAPONTYPE_LTMORTAR && !(stateFlags & STATE_HOLD_FIRE))
	{
		weapWeapon* weap = &vehicleData->VehicleMasterPtr->Weapons[0][0];
		weap->AmmoTypeInUse = weaponNumber;

		/*
		if(weap->NumRoundsAmmoLeft[weaponNumber] > 0)
			fpsWeapons[fpsTrooper.weaponPositionModel[0]].clipBullets = 1;
		else
			fpsWeapons[fpsTrooper.weaponPositionModel[0]].clipBullets = 0;
		*/

		weaponNumber = 0;

		fpFPSUpdateAmmo(weaponNumber, weap);

		// we only calculate here if we're already the mortar
		// if a change happens because we're not currently the mortar,
		// code in fpFPSDetermineNextState() recalculates the angle
		// for the new weapon, so we dont need to
		if(weaponNumber == fpsTrooper.activeWeaponNumber)
			_calculateConvergenceAngle(weaponNumber);
	}

	if (!fpFPSIsWeaponSwitchOK(weaponNumber))
	{
		// fpFPSIsWeaponSwitchOK does check for out of ammo cases, but we check again here
		// due to the execution path, other things can block the weapon switch, but we want
		// the system to try again unless the weapon is really out of ammo
		if(!fpFPSWeaponOutOfAmmo(vehicleData->VehicleMasterPtr->Weapons[weaponNumber])
			|| fpsTrooper.weaponPositionModel[weaponNumber] == BUILD_TOOL) // build tool doesn't have ammo per se
			fpsTrooper.autoswitchWeaponNumber = weaponNumber;

		return;
	}

	// special handling here because lmgs don't have any memory
	// rifle grenades are smarter and store enough state in the weapondata structure
	// FIXME - really ought to fix this
	if (stateFlags & STATE_DEPLOYED)
	{
		if (fpsWeapons[currentWeaponModel].deployPivotBone)
		{
			// lmgs can ONLY switch to binocs if deployed
			if (override)
			{
				stateFlags &= ~STATE_DEPLOYED;
			}
			else if (fpsTrooper.weaponPositionModel[weaponNumber] != BINOCS)
			{
				return;
			}
		}
		else if (currentWeaponModel == BINOCS)
		{
			if (!fpsWeapons[fpsTrooper.weaponPositionModel[weaponNumber]].deployPivotBone)
				return;
			// binocs do nothing except switch back
		}
		else
		{
			stateFlags &= ~STATE_DEPLOYED;
		}
	}

	if ((!override) && (weaponNumber != BINOCS_WEAPON_NUMBER) && (weaponNumber != BUILDTOOL_WEAPON_NUMBER))
	{
		weapWeapon* newWeapon = &(((vehVehicle*)(vehicleData->VehicleMasterPtr))->Weapons[weaponNumber][0]);

		if (!newWeapon)
		{
			return;
		}

		if (fpFPSWeaponOutOfAmmo(newWeapon))
			return;
/*
		if (newWeapon->NumRoundsAmmoLeft[0] == 0)
		{
			if (fpsWeapons[fpsTrooper.weaponPositionModel[weaponNumber]].auxAvailable[FPS_AUX_RGCUP])
			{
				if (newWeapon->NumRoundsAmmoLeft[1] == 0)
					return;
			}
			else
				return;
		}
*/
	}

	stateFlags |= STATE_WEAPON_SWITCHING;

	if (stateFlags & STATE_RELAXED)
	{
		stateFlags &= ~STATE_RELAXED;
	}

	// make sure aim get's kill over weapon switches
	controlStateFlags = 	controlStateFlags & ~FPS_AIM;

	// based on our current state, clean up things
	if (!fpFPSCleanupAnimationState ())
		return;
	if (fpsWeapons[currentWeaponModel].sniperGunSight)
	{
		fpsWeapons[currentWeaponModel].sniperScope = TRUE;
        persLoadInstGunsight(fpsWeapons[currentWeaponModel].sniperGunSight);
	}
	else fpsWeapons[currentWeaponModel].sniperScope = FALSE;

	fpFPSStartAnimationState (currentWeaponModel, LOWER);

	currentFPSstate = LOWER;

	fpsTrooper.nextWeaponNumber = weaponNumber;
	fpsTrooper.autoswitchWeaponNumber = -1;

	fpsTrooper.highlightedWeaponNumber = weaponNumber;

	// handoff to DetermineNextState to actually DO the weapon switch
}

#define POSE_CHANGE_TIME 1000
#define POSE_CHANGE_MIN 1
#define POSE_CHANGE_MAX 5

static bool _poseCheck()
{
	static SINT32 _lastCheck = 0; // when we changed last
	static SINT32 _nextBleed = 0;  // next time to pop a change off
	static SINT32 _changes = 0; // number of changes in the
	static const FLOAT32 _atpHit = -10.0f; // max of 30 percent per try

	if(SuperFloat)
		return false;

	if(!_lastCheck)
	{
		_lastCheck = frametime;
		_nextBleed = frametime;
	}

	SINT32 diff = frametime - _lastCheck;

	if(diff <= POSE_CHANGE_TIME)
	{
		++_changes;

		if(_changes > POSE_CHANGE_MIN)
		{
			if(_changes > POSE_CHANGE_MAX)
				_changes = POSE_CHANGE_MAX;

			FLOAT32 delta = (FLOAT32)diff / (FLOAT32)POSE_CHANGE_TIME;
			FLOAT32 severity = (FLOAT32)_changes / (FLOAT32)POSE_CHANGE_MAX;

			// grab current level
			FLOAT32 currentATP = 0;
			persGetHealthStats(nullptr, &currentATP, nullptr, nullptr);

			// calculate hit
			poseChangeATPEffect = _atpHit * severity * delta;

			// if it's too much, don't hit the atp
			if((currentATP + poseChangeATPEffect) < .6f)
				poseChangeATPEffect = 0;
		}
	}
	else if(frametime > _nextBleed)
	{
		_changes = _changes - (SINT32)(diff / POSE_CHANGE_TIME);

		if(_changes < 0)
			_changes = 0;

		_nextBleed = frametime + POSE_CHANGE_TIME;
	}

	_lastCheck = frametime;

	return (_changes < POSE_CHANGE_MAX);
}

static bool _toProneCheck()
{
	bool prone = (fpFPSGetPlayerPose() == POSE_PRONE);

	if(prone && fpsTrooper.riverExit)
		return true;
	else if(!prone && fpsTrooper.riverExit)
		return false;

	// check timers
	if(!_poseCheck())
		return false;

	vehVehicle* player = persGetPersonaVehiclePtr();
	if(player == nullptr) return false;

	// which way are we moving
	Vec3d dir = Vec3d(player->ModelData->VehicleVel);

	// how fast are we moving
	FLOAT64 speed = dir.length();

	if(prone)
	{
		if(speed >= .1f)
		{
			// if we're moving, angle the ray a little towards our velocity
			dir.z() = 0;
			dir.normalize();

			Vec3d up = Vec3d(0, 0, 1.0f);
			Vec3d right = dir ^ up;

			Matrixd rotate;
			rotate.makeRotate(mlDEG_TO_RAD(75.0f), right);

			dir = dir * rotate;
		}
		else
		{
			// otherwise, just use straight up
			dir.set(0, 0, 1.0f);
		}

		// setup our ray start position
		Vec3d pos = Vec3d(player->ModelData->VehiclePos);

		// if we hit something, disallow the pose change
		if(collideRay(COLLIDER_HARDTERRAIN_MASK|COLLIDER_DRIVABLE_MASK, 2.0f, pos.x(), pos.y(), pos.z() + .15f, dir.x(), dir.y(), dir.z()) > 0)
			return false;
	}
	else
	{
		// if we're just moving slowly, go ahead and let the pose change happen
		if(speed <= 4.0f)
			return true;

		dir.normalize();

		// setup our ray start position
		Vec3d pos = Vec3d(player->ModelData->VehiclePos);

		// movit up slightly in case we're on a hill
		pos.z() += .5f;

		// move our start point back slightly
		dir *= .5f;
		pos += -dir;
		dir.normalize();

		// if we hit something, disallow the pose change
		if(collideRay(COLLIDER_HARDTERRAIN_MASK, 2.0f, pos.x(), pos.y(), pos.z(), dir.x(), dir.y(), dir.z()) > 0)
			return false;
	}

	// made it here, we're good
	return true;
}


// check control block flag; used by tutorials to block certain inputs
#define CBLOCK(x) (controlBlockFlags & (x))

UINT32 fpFPSControlInput (SINT32 param, UINT32 id, UINT32 reserve)
{
	lastInput = frametime;

	// allow nothing while reading a map
	if (fpsTrooper.readingMap)
		return TRUE;

	PlayerPlacedObject* ppo = fpPPOGet();

	if(ppo && ppo->buildAt > 0)
		return TRUE;

	// seperate this from above because I may want to add some
	// additional dependencies here
	if(concentrating)
		return TRUE;

	//static SINT32 lastProneChangeTime = 0; // prone only
	//static SINT32 lastPoseChangeTime = 0; // other poses
	SINT32	mobile			= 1;
	SINT32	deployed		= 0;
	SINT32	deployOK		= -1;
	SINT32	valid			= 1;
	SINT32	turnOK			= 1;
	//SINT32	poseOK		= 1;
	imap_callback_ext_data* extdata;

//	mlVehicleData*	pVehicleData = persGetPersonaModelDataPtr();
	extdata = (imap_callback_ext_data*)reserve;

	deployed = ((stateFlags & STATE_DEPLOYED) > 0);

	if(id != IMAP_HOLD_FP_DEPLOY && deployed && param)
	{
		switch(id)
		{
			case IMAP_HOLD_FP_PRONE:
			case IMAP_HOLD_FP_CROUCH:
			case IMAP_HOLD_FP_FORWARD:
			case IMAP_HOLD_FP_BACKWARD:
			case IMAP_HOLD_FP_AUTORUN:
			case IMAP_HOLD_FP_STRAFE_RIGHT:
			case IMAP_HOLD_FP_STRAFE_LEFT:
			case IMAP_HOLD_FP_WEAPON_1:
			case IMAP_HOLD_FP_WEAPON_2:
			case IMAP_HOLD_FP_WEAPON_3:
			case IMAP_HOLD_FP_WEAPON_4:
			case IMAP_HOLD_FP_WEAPON_5:
			case IMAP_HOLD_FP_WEAPON_6:
			case IMAP_HOLD_FP_WEAPON_7:
			case IMAP_HOLD_FP_WEAPON_8:
			case IMAP_HOLD_FP_WEAPON_9:
			{
				bool isMortar		= (fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_LTMORTAR);
				bool isGrenadier	= (fpsWeapons[currentWeaponModel].auxEnable[FPS_AUX_RGCUP]);

				if(!isGrenadier && (!isMortar || (id != IMAP_HOLD_FP_WEAPON_1 && id != IMAP_HOLD_FP_WEAPON_2)))
				{
					// undeploy us
					fpFPSControlInput(1, IMAP_HOLD_FP_DEPLOY, 0);
					deployed = !deployNeedsUndeployment;
				}
			}
		}
	}

	mobile = !(stateFlags & STATE_NO_MOVEMENT);

	// this sucks
	// attempted to prevent the flop-run but all it seems to do is annoy people
	//poseOK = (!SuperFloat && ((frametime - lastPoseChangeTime) >= POSE_CHANGE_TIME) ? 1 : 0);

	if (mobile)
	{
		turnOK = 1;

		if (fpsWeapons[currentWeaponModel].auxAvailable[FPS_AUX_RGCUP])
		{
			if (stateFlags & STATE_DEPLOYING)
				mobile = 0;
			else if (fpsWeapons[currentWeaponModel].auxEnable[FPS_AUX_RGCUP])
				if (false && stateFlags & STATE_RELOADING) // can move while reloading now
					mobile = 0;
		}
	}
	else
		turnOK = 0;

	if (fpsWeapons[currentWeaponModel].staticDeploy)
		valid = (param && mobile && !deployed);
	else
		valid = (param && mobile);

	//Write_Log (LOG_FPS, LOG_INFO, "frametime = %d, param = %d, id = %d", frametime, param, id);

    // kind of a HACK
	// could be moved to per weapon data as an aim override
	// but for now all we want all deployable weapons like this
	if(param && id == IMAP_HOLD_FP_GUNSIGHT && fpsWeapons[currentWeaponModel].deployPivotBone && (!gCustomTerrain || !fpsTrooper.wading))
	{
		// BAR & FG42 can aim OR deploy - it is up to players how to map this in the keymapper
		/*
		deployOK = (fpsCheckDeploy(currentWeaponModel != BAR)) ? 1 : 0;

		// lets see if we can deploy
		if(deployed || (currentWeaponModel != BAR || deployOK > 0))
			id = IMAP_HOLD_FP_DEPLOY;
		*/

		// lets see if we can deploy
		if ( deployed || (currentWeaponModel != BAR && currentWeaponModel != FG42G) )
			id = IMAP_HOLD_FP_DEPLOY;
	}

	// record the holddown states
	switch (id)
    {
		case IMAP_HOLD_FP_SPRINT:
			if (valid && !CBLOCK(CB_MOVE)) //  && (!(controlStateFlags & FPS_PRONE) ||  frametime >= lastProneChangeTime + (POSE_CHANGE_TIME*3))
			{
				bool prone = (fpFPSGetPlayerPose() == POSE_PRONE);
				bool poseCheck = (prone) ? _toProneCheck() : _poseCheck();

				if(!(controlStateFlags & FPS_PRONE) || (!(controlStateFlags & FPS_SPRINT) && poseCheck))
				{
					controlStateFlags = controlStateFlags | FPS_SPRINT;
					controlStateFlags = controlStateFlags | FPS_FORWARD;
				}
				else
					controlStateFlags = controlStateFlags & ~FPS_SPRINT;
				//lastPoseChangeTime = frametime;

				//if(controlStateFlags & FPS_PRONE)
				//	lastProneChangeTime = lastPoseChangeTime;
			}
			else
			{
				controlStateFlags = controlStateFlags & ~FPS_SPRINT;
			}
//			controlStateFlags = (valid ? controlStateFlags | FPS_SPRINT : controlStateFlags & ~FPS_SPRINT);
			break;
		case IMAP_HOLD_FP_CROUCH:
			{
				fpFPSPoses pose = fpFPSGetPlayerPose();

					bool prone = (pose == POSE_PRONE);
					bool poseCheck = (!valid) ? false : (prone) ? _toProneCheck() : _poseCheck();

					if (valid && poseCheck && !CBLOCK(CB_POSE) && ((controlStateFlags & FPS_CROUCH) || fpsTrooper.wading <= MAX_WADING_KNEEL))
					{
						controlStateFlags ^= FPS_CROUCH;
						controlStateFlags &= ~FPS_PRONE;
						//lastPoseChangeTime = frametime;
					}
//			controlStateFlags = ((param && mobile) ? controlStateFlags | FPS_CROUCH : controlStateFlags & ~FPS_CROUCH);
			}
			break;
		case IMAP_HOLD_FP_PRONE:
			{
					if(valid && _toProneCheck() && !CBLOCK(CB_POSE) && ((controlStateFlags & FPS_PRONE) || fpsTrooper.wading <= MAX_WADING_PRONE)) //  && frametime >= lastProneChangeTime + (POSE_CHANGE_TIME*3)
					{
						controlStateFlags &= ~FPS_CROUCH;
						controlStateFlags ^= FPS_PRONE;
						//lastPoseChangeTime = frametime; //lastProneChangeTime = frametime;
					}
			}
			break;
		case IMAP_HOLD_FP_AUTORUN:
			if (valid && !CBLOCK(CB_MOVE))
			{
				controlStateFlags ^= FPS_AUTORUN;

				// if we disabled autorun, make sure and disable sprint
				// otherwise you get animation anomalies
				if(!(controlStateFlags & FPS_AUTORUN))
					controlStateFlags = controlStateFlags & ~FPS_SPRINT;
			}
			break;
		case IMAP_HOLD_FP_RELAXED:
			if (valid)
				controlStateFlags ^= FPS_RELAXED;
			break;
		case IMAP_HOLD_FP_WALK:
			if (valid)
			{
				controlStateFlags ^= FPS_WALK;
			}
			break;
		case IMAP_HOLD_FP_DEPLOY:
            if (currentWeaponModel == BUILD_TOOL)
            {
                fpPPOInitiateBuildAction(stateFlags, controlStateFlags, _ppoLastStance);
            }
			else if (param && mobile && !CBLOCK(CB_DEPLOY) && fpsTrooper.wading <= MAX_WADING_PRONE)
			{
				if(deployOK == -1)
					deployOK = (fpsCheckDeploy()) ? 1 : 0;

				if (deployOK > 0)
				{
					controlStateFlags &= ~FPS_AUTORUN;
					fpFPSDeployChange();
				}
                
			}
			break;
		case IMAP_HOLD_FP_FORWARD:
		{
			if(valid && !CBLOCK(CB_MOVE))
			{
				controlStateFlags = controlStateFlags | FPS_FORWARD;

				if((controlStateFlags & FPS_PRONE) && (controlStateFlags && FPS_SPRINT) && !_poseCheck())
					controlStateFlags = controlStateFlags & ~FPS_SPRINT;
			}
			else
			{
				controlStateFlags = controlStateFlags & ~FPS_FORWARD;
				controlStateFlags = controlStateFlags & ~FPS_SPRINT;
			}

			if (param)
				controlStateFlags &= ~FPS_AUTORUN;
		}
		break;
		case IMAP_HOLD_FP_BACKWARD:
			controlStateFlags = ((valid && !CBLOCK(CB_MOVE)) ? controlStateFlags | FPS_BACKWARD : controlStateFlags & ~FPS_BACKWARD);
			controlStateFlags &= ~FPS_AUTORUN;
			controlStateFlags &= ~FPS_SPRINT;
			break;
		case IMAP_HOLD_FP_STRAFE_RIGHT:
			controlStateFlags = ((valid && !CBLOCK(CB_MOVE)) ? controlStateFlags | FPS_STRAFE_RIGHT : controlStateFlags & ~FPS_STRAFE_RIGHT);
			if(!(controlStateFlags & FPS_FORWARD)) controlStateFlags &= ~FPS_SPRINT;
			break;
		case IMAP_HOLD_FP_STRAFE_LEFT:
			controlStateFlags = ((valid && !CBLOCK(CB_MOVE)) ? controlStateFlags | FPS_STRAFE_LEFT : controlStateFlags & ~FPS_STRAFE_LEFT);
			if(!(controlStateFlags & FPS_FORWARD)) controlStateFlags &= ~FPS_SPRINT;
			break;
		case IMAP_HOLD_FP_TURN_RIGHT:
			controlStateFlags = ((valid && !CBLOCK(CB_LOOK)) ? controlStateFlags | FPS_TURN_RIGHT : controlStateFlags & ~FPS_TURN_RIGHT);
			break;
		case IMAP_HOLD_FP_TURN_LEFT:
			controlStateFlags = ((valid && !CBLOCK(CB_LOOK)) ? controlStateFlags | FPS_TURN_LEFT : controlStateFlags & ~FPS_TURN_LEFT);
			break;
		case IMAP_HOLD_FP_GUNSIGHT:
			controlStateFlags = ((valid) ? controlStateFlags | FPS_AIM : controlStateFlags & ~FPS_AIM);

			/*
			WEAPON_LAG_VMAX			*= (valid) ? .1f: 10.0f;
			WEAPON_LAG_HMAX			*= (valid) ? .1f: 10.0f;
			WEAPON_LAG_JUMP			*= (valid) ? .1f: 10.0f;
			WEAPON_LAG_JUMPMAX		*= (valid) ? .1f: 10.0f;
			WEAPON_LAG_JUMPMIN		*= (valid) ? .1f: 10.0f;
			WEAPON_LAG_ACCEL		*= (valid) ? .1f: 10.0f;
			WEAPON_LAG_BLEED		*= (valid) ? .1f: 10.0f;
			*/

			break;
		case IMAP_HOLD_FP_FIRE:

			if(fpsTrooper.highlightedWeaponNumber != fpsTrooper.activeWeaponNumber && !fpFPSWeaponOutOfAmmo(currentGameWeapon))
			{
				fpsTrooper.highlightedWeaponNumber = fpsTrooper.activeWeaponNumber;
				stateFlags &= ~STATE_WEAPON_SWITCHING;
			}

			if (controlStateFlags & FPS_RELAXED)
			{
				if (controlStateFlags & FPS_AIM)
				{
					controlStateFlags = ((valid && !CBLOCK(CB_FIRE)) ? controlStateFlags | FPS_FIRE : controlStateFlags & ~FPS_FIRE);
				}
				else if (param && mobile)
				{
					controlStateFlags &= ~FPS_RELAXED;
					controlStateFlags = ((!CBLOCK(CB_FIRE)) ? controlStateFlags | FPS_FIRE : controlStateFlags & ~FPS_FIRE);
				}
			}
			else
			{
				controlStateFlags = ((param && mobile && !CBLOCK(CB_FIRE)) ? controlStateFlags | FPS_FIRE : controlStateFlags & ~FPS_FIRE);
			}

			if(param)
			{
				fpsStartGrenadeThrowTime = frametime;
				fpsEndGrenadeThrowTime = 0;

				if((controlStateFlags & FPS_FIRE) && noFire)
				{
					fpsEndGrenadeThrowTime = frametime;
					controlStateFlags = controlStateFlags & ~FPS_FIRE;
					persShakeHead();
					uiGameEvent(GAMEEVENT_NOFIRE);
				}
			}
			else
			{
				fpsEndGrenadeThrowTime = frametime;
			}

			fireDown = (param > 0);

			break;
		case IMAP_HOLD_FP_LOOK_LEFT:
		{
			// not sure why this code was here, in all cases we want head turning
			//if (!fpsTrooper.isParatroop ||	!fpsTrooper.parachuteWorn || fpsTrooper.parachuteOpen)
				controlStateFlags = ((param && !CBLOCK(CB_LOOK)) ? controlStateFlags | FPS_LOOK_LEFT : controlStateFlags & ~FPS_LOOK_LEFT);
			//else
			//	controlStateFlags = (controlStateFlags & ~FPS_LOOK_LEFT);
		}
		break;
		case IMAP_HOLD_FP_LOOK_RIGHT:
		{
			// not sure why this code was here, in all cases we want head turning
			//if (!fpsTrooper.isParatroop ||	!fpsTrooper.parachuteWorn || fpsTrooper.parachuteOpen)
				controlStateFlags = ((param && !CBLOCK(CB_LOOK)) ? controlStateFlags | FPS_LOOK_RIGHT : controlStateFlags & ~FPS_LOOK_RIGHT);
			//else
			//	controlStateFlags = (controlStateFlags & ~FPS_LOOK_RIGHT);
		}
		break;

		/* Lean functions deprecated after 1.33

		case IMAP_HOLD_FP_LEAN_LEFT:
			if (valid)
			{
				controlStateFlags |= FPS_LEAN_LEFT;
				controlStateFlags &= ~FPS_LEAN_RIGHT;
			}
			else
			{
				controlStateFlags &= ~(FPS_LEAN_LEFT|FPS_LEAN_RIGHT);
			}
			break;
		case IMAP_HOLD_FP_LEAN_RIGHT:
			if (valid)
			{
				controlStateFlags |= FPS_LEAN_RIGHT;
				controlStateFlags &= ~FPS_LEAN_LEFT;
			}
			else
			{
				controlStateFlags &= ~(FPS_LEAN_RIGHT | FPS_LEAN_LEFT);
			}
			break;
		*/

		case IMAP_HOLD_FP_WEAPON_1:
		case IMAP_HOLD_FP_WEAPON_2:
		case IMAP_HOLD_FP_WEAPON_3:
		case IMAP_HOLD_FP_WEAPON_4:
		case IMAP_HOLD_FP_WEAPON_5:
		case IMAP_HOLD_FP_WEAPON_6:
		case IMAP_HOLD_FP_WEAPON_7:
		case IMAP_HOLD_FP_WEAPON_8:
		case IMAP_HOLD_FP_WEAPON_9: // Build tool
		case IMAP_HOLD_FP_WEAPON_10: // Binocs
		{
			fpFPSWeaponData& data = fpsWeapons[currentWeaponModel];

			// only allow weapon swaps if the rg is not loaded
			bool rgLoaded = (data.auxEnable[FPS_AUX_RGCUP] && data.auxEnable[FPS_AUX_RGGRENADE]);

			if (param)
			{
				if(!CBLOCK(CB_SELECT) && !rgLoaded)
					fpFPSWeaponChange (id - IMAP_HOLD_FP_WEAPON_1, 0);
				else
					persShakeHead();
			}
		}
		break;
		case IMAP_ASX_FP_TORSO_VERTICAL_AXIS:
			if(!CBLOCK(CB_LOOK))
			{
				if (mouseLookOn)
				{
					if (!mlMouseLookAndTorsoUseSameKeymap())
					{
						fpFPSAxisRotation (param, id);
					}
				}
				else
				{
					fpFPSAxisRotation (param, id);
				}
			}
			break;
		case IMAP_ASX_FP_TORSO_HORIZONTAL_AXIS:
			if (turnOK && !CBLOCK(CB_LOOK))
			{
				if (mouseLookOn)
				{
					if (!mlMouseLookAndTorsoUseSameKeymap())
					{
						fpFPSAxisRotation (param, id);
					}
				}
				else
				{
					fpFPSAxisRotation (param, id);
				}
			}
			break;
		case IMAP_MOUSE_LOOK_X:
			fpFPSHorizAxisLook (param, reserve);
			break;
		case IMAP_MOUSE_LOOK_Y:
			fpFPSVerticalAxisLook (param, reserve);
			break;
		case IMAP_MOM_FP_RIDE_UNRIDE_VEHICLE:
			if (param)
			{
				fpFPSRideUnrideVehicle ();
			}
			break;
		case IMAP_HOLD_FP_RELOAD:
			if ((param) && !((stateFlags & STATE_SWIMMING)) && !CBLOCK(CB_FIRE))
			{
				fpFPSManualReload ();
			}
			break;
		/* // disable emotes for now
		case IMAP_HOLD_FP_CE_DOUBLE:
			if (param && mobile)
			{
				fpFPSStartEmote(fpTPSCombatEmoteCmds[TPS_COMBATEMOTE_DOUBLETIME], "", FALSE);
			}
			break;
		case IMAP_HOLD_FP_CE_GETDOWN:
			if (param && mobile)
			{
				fpFPSStartEmote(fpTPSCombatEmoteCmds[TPS_COMBATEMOTE_GETDOWN], "", FALSE);
			}
			break;
		case IMAP_HOLD_FP_CE_HALT:
			if (param && mobile)
			{
				fpFPSStartEmote(fpTPSCombatEmoteCmds[TPS_COMBATEMOTE_HALT], "", FALSE);
			}
			break;
		case IMAP_HOLD_FP_CE_MOVEOUT:
			if (param && mobile)
			{
				fpFPSStartEmote(fpTPSCombatEmoteCmds[TPS_COMBATEMOTE_MOVEOUT], "", FALSE);
			}
			break;
		case IMAP_HOLD_FP_CE_AMMO:
			if (param && mobile)
			{
				fpFPSStartEmote(fpTPSCombatEmoteCmds[TPS_COMBATEMOTE_AMMO], "", FALSE);
				vehmainRequestAmmo();
			}
			break;
		case IMAP_HOLD_FP_CE_JUMP:
			if (param && mobile)
			{
				if (fpFPSOkToJumpEmote())
					fpFPSStartEmote(fpTPSCombatEmoteCmds[TPS_COMBATEMOTE_JUMP], "", FALSE);
			}
			break;
		*/
		case IMAP_HOLD_GUNSIGHT_RANGE_INC:
		case IMAP_HOLD_GUNSIGHT_RANGE_DEC:
			if (param && persIsCameraInternalMode())
			{
				if(id == IMAP_HOLD_GUNSIGHT_RANGE_INC)
				{
					controlStateFlags |= FPS_SIGHT_UP;
					controlStateFlags &= ~FPS_SIGHT_DOWN;
				}
				else
				{
					controlStateFlags |= FPS_SIGHT_DOWN;
					controlStateFlags &= ~FPS_SIGHT_UP;
				}
			}
			else
			{
				controlStateFlags &= ~(FPS_SIGHT_UP | FPS_SIGHT_DOWN);
			}
			break;
		case IMAP_HOLD_FP_NEXT_WEAPON:
			if (param && !CBLOCK(CB_SELECT))
			{
				fpFPSSelectNextWeapon();
			}
			break;
		case IMAP_HOLD_FP_PREV_WEAPON:
			if (param && !CBLOCK(CB_SELECT))
			{
				fpFPSSelectPreviousWeapon();
			}
			break;
		default:
			Write_Log (LOG_FPS, LOG_INFO, "Unrecognized input received %d", id);
			break;
	}

//	Write_Log (LOG_FPS, LOG_INFO, "control state flags 0x%08x", controlStateFlags);

	return TRUE;
}

static inline bool _updateTurrets()
{
	fpFPSWeaponType wtype = fpsWeapons[currentWeaponModel].weaponType;
	bool automaticWeapon = (wtype == FPS_WEAPONTYPE_LMG || wtype == FPS_WEAPONTYPE_SMG);

	// only update turrets after our fire turret position went out
	if(Online && !automaticWeapon && fireUpdateNo && wwiiGetUpdateNo(false) == fireUpdateNo)
		return false;

	return true;
}

static void fpFPSSetVerticalDeployedPosition (FLOAT64 newPos)
{
	FLOAT32		maxDeployedUpAngle = fpsTrooper.maxProneDeployedUpAngle;
	FLOAT32		maxDeployedDownAngle = fpsTrooper.maxProneDeployedDownAngle;
	FLOAT32		granularity = (float)(fpsTrooper.mouseGranularity * fpsTrooper.vertGranularityMultiplier * fpsTrooper.deployedGranularityMultiplier);

	mlVehicleData*	pVehicleData = persGetPersonaModelDataPtr();

	// mortars only rotate in the horizontal plane while deployed
	aimVerticalDeployedPosition = (fpsWeapons[currentWeaponModel].weaponType != FPS_WEAPONTYPE_LTMORTAR) ? newPos: 0;

	if ((aimVerticalDeployedPosition * granularity) > maxDeployedDownAngle)
		aimVerticalDeployedPosition = maxDeployedDownAngle / granularity;
	else if ((aimVerticalDeployedPosition * granularity) < -maxDeployedUpAngle)
		aimVerticalDeployedPosition = -maxDeployedUpAngle / granularity;

	// only update turrets after our fire turret position went out
	if(!_updateTurrets())
		return;

	// setup fire point turret info
	{
		FLOAT32 maxUpAngle = -80.0f;
		FLOAT32 maxDownAngle = 70.0f;
		FLOAT32 deployedAngle = (float)aimVerticalDeployedPosition * granularity;
		FLOAT32 convergance = fpsWeapons[fpsTrooper.weaponPositionModel[fpsTrooper.activeWeaponNumber]].convergenceAngle;

		// turret range is -1 to +1 .. -1 is all the way down, +1 is all the way up
		FLOAT32 angle = (float)(AIM_VERTICAL_ANGLE  - deployedAngle - convergance);
		pVehicleData->TurretElevate[1] = angle / maxUpAngle;

		// make sure we're still clamped
		if(pVehicleData->TurretElevate[1] > 1.0f)
			pVehicleData->TurretElevate[1] = 1.0f;
		else if(pVehicleData->TurretElevate[1] < -1.0f)
			pVehicleData->TurretElevate[1] = -1.0f;

		// the multiply by -1 is because maxup/maxdown is a negative number .. we don't want to flip the sign
		if (pVehicleData->TurretElevate[1] < 0)
			pVehicleData->TurretElevate[1] = (pVehicleData->TurretElevate[1] * maxUpAngle/maxDownAngle) * -1.0f;

		//uiChatPrintLine("VA(%.3f) + DAV(%.3f) = %.3f", -AIM_VERTICAL_ANGLE, deployedAngle, -AIM_VERTICAL_ANGLE + deployedAngle);
	}

	// setup visible turret info
	{
		pVehicleData->TurretElevate[2] = (FLOAT32)(aimVerticalDeployedPosition * granularity) / maxDeployedUpAngle;

		// make sure we're still clamped
		if(pVehicleData->TurretElevate[2] > 1.0f)
			pVehicleData->TurretElevate[2] = 1.0f;
		else if(pVehicleData->TurretElevate[2] < -1.0f)
			pVehicleData->TurretElevate[2] = -1.0f;

		//uiChatPrintLine("VA(%.3f) + DAV(%.3f) = %.3f", -AIM_VERTICAL_ANGLE, deployedAngle, -AIM_VERTICAL_ANGLE + deployedAngle);
	}
}

static void fpFPSSetHorizontalDeployedPosition (FLOAT64 newPos)
{
	FLOAT32		maxDeployedLeftAngle = fpsTrooper.maxProneDeployedLeftAngle;
	FLOAT32		maxDeployedRightAngle = fpsTrooper.maxProneDeployedRightAngle;
	mlVehicleData*	pVehicleData = persGetPersonaModelDataPtr();

	// mortars can't move while fired
	if(fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_LTMORTAR && fireStartTime)
		return;

	aimHorizontalDeployedPosition = newPos;

	if ((aimHorizontalDeployedPosition * fpsTrooper.mouseGranularity * fpsTrooper.deployedGranularityMultiplier) > maxDeployedRightAngle)
		aimHorizontalDeployedPosition = maxDeployedRightAngle / (fpsTrooper.mouseGranularity * fpsTrooper.deployedGranularityMultiplier);
	else if ((aimHorizontalDeployedPosition * fpsTrooper.mouseGranularity * fpsTrooper.deployedGranularityMultiplier) < -maxDeployedLeftAngle)
		aimHorizontalDeployedPosition = -maxDeployedLeftAngle / (fpsTrooper.mouseGranularity * fpsTrooper.deployedGranularityMultiplier);

	// only update turrets after our fire turret position went out
	if(!_updateTurrets())
		return;

	// setup fire turret info
	{
		// turret range is -1 to +1 .. -1 is all the way down, +1 is all the way up
		FLOAT32 angle = (float)(aimHorizontalDeployedPosition * fpsTrooper.mouseGranularity * fpsTrooper.deployedGranularityMultiplier);
		pVehicleData->TurretTraverse[1] = angle / maxDeployedRightAngle;

		// correct for up and down angle range being different (the scale is set to the up angle)
		// the multiply by -1 is because maxup/maxdown is a negative number .. we don't want to flip the sign
		if (pVehicleData->TurretTraverse[1] < 0)
			pVehicleData->TurretTraverse[1] = pVehicleData->TurretTraverse[1] * maxDeployedRightAngle/maxDeployedLeftAngle;
	}

	// setup visible turret info
	{
		pVehicleData->TurretTraverse[2] = (FLOAT32)(aimHorizontalDeployedPosition * fpsTrooper.mouseGranularity * fpsTrooper.deployedGranularityMultiplier) / maxDeployedRightAngle;

		// make sure we're still clamped
		if(pVehicleData->TurretTraverse[2] > 1.0f)
			pVehicleData->TurretTraverse[2] = 1.0f;
		else if(pVehicleData->TurretTraverse[2] < -1.0f)
			pVehicleData->TurretTraverse[2] = -1.0f;

		//uiChatPrintLine("VA(%.3f) + DAV(%.3f) = %.3f", -AIM_VERTICAL_ANGLE, deployedAngle, -AIM_VERTICAL_ANGLE + deployedAngle);
	}
}

void fpFPSAdjustTurretElevation (mlVehicleData* pVehicleData, FLOAT32 maxUpAngle, FLOAT32 maxDownAngle)
{
	// only update turrets after our fire turret position went out
	if(!_updateTurrets())
		return;

	// add in our sight adjustment
	FLOAT32 convergance = fpsWeapons[fpsTrooper.weaponPositionModel[fpsTrooper.activeWeaponNumber]].convergenceAngle;

	// turret range is -1 to +1 .. -1 is all the way down, +1 is all the way up
	// with aim vertical angle, negative is up for some reason, so we subtract convergance (it gets normalized below)
	FLOAT32 currentAngle = (FLOAT32)AIM_VERTICAL_ANGLE;
	maxUpAngle = fabs(maxUpAngle);
	maxDownAngle = fabs(maxDownAngle);

	FLOAT32 final = fabs(currentAngle - convergance);

	if(currentAngle > 0)
	{
		// pointing down
		final /= maxDownAngle;
		final *= -1.0f;
	}
	else
	{
		// point up
		final /= maxUpAngle;
	}

	// make sure we're still clamped between -1 and 1
	if(final > 1.0f) final = 1.0f;
	else if(final < -1.0f) final = -1.0f;

	pVehicleData->TurretElevate[0] = final;

	//uiChatPrintLine("Elevation: aim = %.2f convergance = %.2f min = %.2f max = %.2f equals %.2f", AIM_VERTICAL_ANGLE, convergance, maxUpAngle, maxDownAngle, pVehicleData->TurretElevate[0]);
}

static void fpFPSVerticalAxisLook (SINT32 param, UINT32 data)
{
	if (!mouseLookOn)
		return;

	imap_callback_ext_data*		extdata;
	FLOAT32						maxUpAngle, maxDownAngle;
    
    /*
     TBD = deal with turret elevation (currently commented out??)
     
	mlVehicleData*				pVehicleData = persGetPersonaModelDataPtr();
     */
    
	extdata = (imap_callback_ext_data*)data;

	fpFPSGetEyeUpDownRange(&maxUpAngle, &maxDownAngle);

	if(extdata && extdata->axisType == IMAP_DELTA_AXIS)
	{
		FLOAT64	angleChange = param * fpsTrooper.mouseGranularity * fpsTrooper.vertGranularityMultiplier;

		if(stateFlags & STATE_SIT)
		{
			maxUpAngle		= -maxUpAngle;
			maxDownAngle	= -maxDownAngle;
			angleChange		= -angleChange;

			if ((mouseLookVertAngle + (AIM_VERTICAL_ANGLE) + angleChange) > maxUpAngle && angleChange > 0)
				angleChange = 0;

			if ((mouseLookVertAngle + (AIM_VERTICAL_ANGLE) + angleChange) < maxDownAngle && angleChange < 0)
				angleChange = 0;
		}
		else
		{
			if ((mouseLookVertAngle + (AIM_VERTICAL_ANGLE) + angleChange) < maxUpAngle && angleChange < 0)
				angleChange = 0;

			if ((mouseLookVertAngle + (AIM_VERTICAL_ANGLE) + angleChange) > maxDownAngle && angleChange > 0)
				angleChange = 0;
		}

        /*
         
		FLOAT64 total = mouseLookVertAngle + (AIM_VERTICAL_ANGLE) + angleChange;

		uiChatPrintLine("mlookangle = %.2f avertangle = %.2f change = %.2f total = %.2f min = %.2f max = %.2f", mouseLookVertAngle, AIM_VERTICAL_ANGLE, angleChange, total, maxDownAngle, maxUpAngle);

         */
		if (angleChange)
		{
			mouseLookVertAngle += angleChange;

			// if we're looking down, push the gun down to move with the eye
			if (mouseLookVertAngle > 0)
			{
				aimVertAngleOffset = mouseLookVertAngle;
			}
			else
				aimVertAngleOffset = 0;
		}

		//fpFPSAdjustTurretElevation (pVehicleData, maxUpAngle, maxDownAngle);
	}
	else
	{
		if (param > 0)
		{
			mouseLookVertAngle = maxUpAngle * (param / 1000.0f);
			aimVertAngleOffset = 0;
		}
		else if (param < 0)
		{
			mouseLookVertAngle = -maxDownAngle * (param / 1000.0f);
			aimVertAngleOffset = mouseLookVertAngle;
		}
		else
		{
			mouseLookVertAngle = 0;
			aimVertAngleOffset = 0;
		}
	}
//	Write_Log (LOG_FPS, LOG_INFO, "VertAxisParam = %d", param);
}

static void fpFPSHorizAxisLook (SINT32 param, UINT32 data)
{
	if (!mouseLookOn)
		return;

	imap_callback_ext_data*		extdata;
	FLOAT32						maxUpAngle, maxDownAngle;

//	mlVehicleData*				pVehicleData = persGetPersonaModelDataPtr();

	extdata = (imap_callback_ext_data*)data;

	fpFPSGetEyeUpDownRange(&maxUpAngle, &maxDownAngle);

	if(extdata && extdata->axisType == IMAP_DELTA_AXIS)
	{
		mouseLookHorizAngle += param * fpsTrooper.mouseGranularity;
		if (mouseLookHorizAngle > fpsTrooper.maxHorizEyeAngle)
			mouseLookHorizAngle = fpsTrooper.maxHorizEyeAngle;
		else if (mouseLookHorizAngle < -fpsTrooper.maxHorizEyeAngle)
			mouseLookHorizAngle = -fpsTrooper.maxHorizEyeAngle;
	}
	else
	{
		mouseLookHorizAngle = fpsTrooper.maxHorizEyeAngle * (param / 1000.0f);
	}
//	Write_Log (LOG_FPS, LOG_INFO, "HorizAxisParam = %d", param);
}

static void fpFPSAxisRotation (SINT32 param, UINT32 id)
{
	mlVehicleData*	pVehicleData = persGetPersonaModelDataPtr();

	if(currentFPSstate == DEPLOYING)
		return;

	if (param)
	{
		if (id == IMAP_ASX_FP_TORSO_VERTICAL_AXIS)
		{
			FLOAT32 maxUpAngle, maxDownAngle;
			FLOAT64	 angleChange = param * fpsTrooper.mouseGranularity * fpsTrooper.vertGranularityMultiplier;

			// If we're aiming, use our separate aiming scalar
			if(stateFlags & STATE_AIM)
				angleChange *= (aimMouse * .6f); // Vertical aim is always "slower" than horizontal aim

			// this is because when we're sitting, our rotations are in the coordinate system of the
			// vehicle, and we need to flip this to get there
			// aka .. fixme, yuck
			if (stateFlags & STATE_SIT)
				angleChange = -angleChange;

			fpFPSGetEyeUpDownRange(&maxUpAngle, &maxDownAngle);

			if(HAS_ALL_BITS(stateFlags, STATE_DEPLOYED) && fpsWeapons[currentWeaponModel].staticDeploy) // | STATE_EYE_PRONE
			{
				fpFPSSetVerticalDeployedPosition (aimVerticalDeployedPosition - param);
				// this handles everything that needs to be handled, so we can return and skip a few instructions
				return;
			}
			else
			{
				aimVertAngle += angleChange;

				fpFPSAddWeaponLag(AROUND_X, -angleChange);

                bool deployed = (stateFlags & (STATE_DEPLOYED | STATE_EYE_PRONE)) == STATE_DEPLOYED;

                if(deployed)
                {
          //          maxDownAngle = 60.0f;
            //       maxUpAngle = -60.0f;
                }

				if (aimVertAngle > maxDownAngle)
					aimVertAngle = maxDownAngle;
				else if (aimVertAngle < maxUpAngle)
					aimVertAngle = maxUpAngle;
			}

			fpFPSAdjustTurretElevation(pVehicleData, maxUpAngle, maxDownAngle);
		}
		else if (id == IMAP_ASX_FP_TORSO_HORIZONTAL_AXIS)
		{
			if(fpsWeapons[currentWeaponModel].staticDeploy &&
               stateFlags & STATE_DEPLOYED)// && stateFlags & STATE_EYE_PRONE)
			{
				fpFPSSetHorizontalDeployedPosition (aimHorizontalDeployedPosition + param);
			}
			else
			{
				FLOAT64		rotationSpeed;
				FLOAT64		deltaTime;
				FLOAT64		maxRotationSpeed;
				FLOAT64		speedMOD = 1.0f;

				if(!SuperTroop && !SuperFloat && currentForwardVectorLength >= 4.0f)
				{
					static SINT32 speedModPeriod = 0;
					static FLOAT32 speedModSpeed = 0.0f;

					if(currentForwardVectorLength >= speedModSpeed)
					{
						speedModPeriod = frametime + 1000;
						speedModSpeed = currentForwardVectorLength;
					}

					if(frametime > speedModPeriod)
						speedModSpeed = currentForwardVectorLength;

					static FLOAT64 base = .65f;
					static FLOAT64 range = .5f;

					// mod is .65 to .5 from 4.0f to 6.0f speed range; turn speed decreases with forward speed
					speedMOD = base - (range * (1.0f - (6.0f - speedModSpeed)/2.0f));
				}

				rotationSpeed = (param * fpsTrooper.mouseGranularity) * speedMOD;
				deltaTime = ((frametime - lastframetime)/1000.0);

				if(deltaTime <= 0.0f)	deltaTime = FLT_MIN;

				if (param < 0)
					rotationSpeed *= -1.0;

				if ((stateFlags & STATE_TURN_LEFT) || (stateFlags & STATE_TURN_RIGHT))
				{
					maxRotationSpeed = fpsTrooper.maxRotationSpeed - fpsTrooper.keyRotationSpeed;
				}
				else
				{
					maxRotationSpeed = fpsTrooper.maxRotationSpeed;
				}

				if(stateFlags & STATE_EYE_PRONE)
					maxRotationSpeed *= .75f; // slow it down some

				if ((rotationSpeed/deltaTime) > maxRotationSpeed)
					rotationSpeed = maxRotationSpeed * deltaTime;

				if (param < 0)
					rotationSpeed *= -1.0;

				// If we're aiming, use our separate aiming scalar
				if(stateFlags & STATE_AIM)
					rotationSpeed *= aimMouse;


				stat2Flags |= (rotationSpeed < 0) ? STAT2_TURNING_LEFT: STAT2_TURNING_RIGHT;

				fpFPSAddWeaponLag(AROUND_Z, -rotationSpeed);

				/*
                bool deployed = !(stateFlags & STATE_EYE_PRONE) &&
                                 (stateFlags & STATE_DEPLOYED) &&
                                 fpsWeapons[currentWeaponModel].deployPivotBone;
				*/

                bool deployed = !(stateFlags & STATE_EYE_PRONE) &&
                                 (stateFlags & STATE_DEPLOYED);

				deployed = (currentWeaponModel == BINOCS) ?
							(deployed && fpsWeapons[lastWeaponModel].deployPivotBone):
							(deployed && fpsWeapons[currentWeaponModel].deployPivotBone);

				// set the yaw here (why not?)
                if(deployed && !_candeploy(param))
                {
                    pVehicleData->VehicleAtt = pVehicleData->LastVehicleAtt;
                }
                else
                {
				    // granularity converts to degrees, so multiplier needed for yaw, which is in radians
				    pVehicleData->VehicleAtt.Yaw -= rotationSpeed*PI/180.0;

				    // Make sure we don't go past the bounds of the Yaw
				    while(pVehicleData->VehicleAtt.Yaw > PI)
				    {
					    pVehicleData->VehicleAtt.Yaw -= 2 * PI;
				    }

				    while(pVehicleData->VehicleAtt.Yaw < -PI)
				    {
					    pVehicleData->VehicleAtt.Yaw += 2 * PI;
				    }
                }
			}
		}

		if(pVehicleData->VehicleMasterPtr->RideVehicleHostIndex > 0)
		{
			vehVehicle* FPSRideVehicle = vehFindVehicle(pVehicleData->VehicleMasterPtr->RideVehicleHostIndex);
			if(FPSRideVehicle)
				fpSaveRelRidePos(FPSRideVehicle, pVehicleData->VehicleMasterPtr);
		}
	}
	else
	{
#if defined( YOU_KNOW_WTF_THIS_IS_FOR )
		if (id == IMAP_ASX_FP_TORSO_VERTICAL_AXIS)
			int i = 0;
		else if (id == IMAP_ASX_FP_TORSO_HORIZONTAL_AXIS)
			int i = 0;
#else
	// Two less compiler warnings
#endif
	}
}

LOGICAL	fpFPSUpdateInfantry(FLOAT64 DeltaTime)
{
	return fpFPSUpdatePerson(persGetPersonaModelDataPtr(), DeltaTime);
}
void fpFPSGetInfantryData(fpFPSInfantryData& modelData)
{
	mlVehicleData* pVehicleData = persGetPersonaModelDataPtr();

	modelData.Yaw = pVehicleData->VehicleAtt.Yaw;
	pVehicleData->LastVehicleAtt = pVehicleData->VehicleAtt;
	fpFPSGetVelocity(&modelData.Velocity);
	modelData.AnimFlags = pVehicleData->PersonData->AnimationFlags;

	/*
	modelData.Velocity.x = -modelData.Velocity.x;
	modelData.Velocity.z = -modelData.Velocity.z;
	*/
}

UINT32 fpFPSCheckNewPose(fpFPSPoses poseToCheck)
{
	if(gCustomTerrain)
	{
		return 0;
	}
	else
	{
		//	Write_Log (LOG_FPS, LOG_INFO, "Checking pose %d", poseToCheck);
		mlVehicleData*	vehicleData = persGetPersonaModelDataPtr();
		FLOAT64VECTOR	zeroVel = { 0, 0, 0 };
		return (mlDetermineInfantryCollisions(vehicleData, &zeroVel, (UINT32)poseToCheck, FALSE));
	}
}

void fpFPSDeployChange ()
{
	if (!(stateFlags & STATE_DEPLOYED))
	{
		deployNeedsDeployment = true;
		stateFlags |= STATE_DEPLOYED;
	}
	else
	{
		deployNeedsUndeployment = true;
	}
}


LOGICAL fpFPSHandleControls ()
{
	UINT32 moveState = 0;
	UINT32 eyeState = 0;
	UINT32 strafeState = 0;
	UINT32 turnState = 0;
	UINT32 aimState = 0;
	UINT32 lookState = 0;
	UINT32 fireState = 0;
	UINT32 leanState = 0;

	if (stateFlags & STATE_SWIMMING)
	{
		controlStateFlags &= ~(FPS_CROUCH | FPS_SPRINT);
		controlStateFlags |= FPS_PRONE;
	}

	if(fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_ROCKET)
	{
		int cb = fpsWeapons[currentWeaponModel].clipBullets;
		FLOAT32	currentTime = (frametime/1000.0f);

		if((controlStateFlags & FPS_SPRINT) && cb > 0)
		{
			controlStateFlags &= ~FPS_SPRINT;
			persShakeHead("common.headshake.reloadnosprint");
		}
		else if((controlStateFlags & FPS_AIM) && cb == 0 && currentTime - currentGameWeapon->LastFireTime >= 1.0f)
		{
			controlStateFlags &= ~FPS_AIM;
			persShakeHead("common.headshake.reload");
		}
	}
	
	if (controlStateFlags & FPS_PRONE)
	{
		eyeState = EYE_PRONE;
	}
	else
	{
		eyeState = EYE_STAND;
	}

	if ((controlStateFlags & FPS_AIM) && (!(stateFlags & STATE_SIT)))
	{
		aimState = AIM_ON;
	}
	else
	{
		aimState = AIM_OFF;
	}

	if (controlStateFlags & (FPS_FORWARD | FPS_AUTORUN))
	{
		if (stateFlags & STATE_MOVE_BACKWARD)
		{
			// handle if forward and backwards are both held down ..
			// in this case, go backwards if already go backwards, otherwise
			// go forward
			if (!(controlStateFlags & FPS_BACKWARD))
			{
				moveState = SLOW;
			}
			else
			{
				moveState = BACKWARD;
			}
		}
		else
		{
			moveState = SLOW;
		}
	}
	else if (controlStateFlags & FPS_BACKWARD)
	{
		// forward isn't held down, so backwards we go
		moveState = BACKWARD;
	}
	else
	{
		moveState = STILL;
	}

	// turn off fire if we're leaning & wielding a weapon that can't fire while leaning
	// also turn off lean
	if ((controlStateFlags & FPS_FIRE)
		&& (controlStateFlags & (FPS_LEAN_RIGHT|FPS_LEAN_LEFT))
		&& !(fpsWeapons[currentWeaponModel].leanFireOK)
		)
	{
		controlStateFlags &= ~(FPS_FIRE|FPS_LEAN_RIGHT|FPS_LEAN_LEFT);
	}

	if (controlStateFlags & FPS_FIRE)
	{
		// can't fire if prone & moving
		if (!((eyeState == EYE_PRONE) && (moveState != STILL)))
			fireState = FIRE_ON;
		else
			fireState = FIRE_OFF;
	}
	else
	{
		fireState = FIRE_OFF;
	}

	if (controlStateFlags & FPS_STRAFE_RIGHT)
	{
		// right strafe gets priority this way
		// make sure we're not already strafing left and that the left button isn't held down still
		// before setting right strafe
		if (!((stateFlags & STATE_STRAFE_LEFT) && (controlStateFlags & FPS_STRAFE_LEFT)))
			strafeState = STRAFE_RIGHT;
	}
	else  if (controlStateFlags & FPS_STRAFE_LEFT)
	{
		// right strafe isn't being held down, so just strafe left
		strafeState = STRAFE_LEFT;
	}
	else
	{
		strafeState = STRAFE_NONE;
	}

	if ((controlStateFlags & (FPS_TURN_RIGHT | FPS_TURN_LEFT)) && (!((fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_LMG) && (stateFlags & STATE_DEPLOYED))))
	{
		// right turn gets priority this way
		if (controlStateFlags & FPS_TURN_RIGHT)
		{
			// make sure we're not already turning left and that the left button isn't held down still
			// before setting right turn
			if (!((stateFlags & STATE_TURN_LEFT) && (controlStateFlags & FPS_TURN_LEFT)))
				turnState = TURN_RIGHT;
		}
		else //(controlStateFlags & FPS_STRAFE_LEFT)
		{
			// right strafe isn't being held down, so just strafe left
			turnState = TURN_LEFT;
		}
	}
	else
	{
		turnState = TURN_NONE;
	}

	// Look left/right: if the player holds down both, look right gets priority.
	// right turn gets priority this way
	if (controlStateFlags & FPS_LOOK_RIGHT)
	{
		// make sure we're not already looking left and that the left button isn't held down still
		// before setting right strafe
		if (!((stateFlags & STATE_LOOK_LEFT) && (controlStateFlags & FPS_LOOK_LEFT)))
			lookState = LOOK_RIGHT;
	}
	else if (controlStateFlags & FPS_LOOK_LEFT)
	{
		// right strafe isn't being held down, so just strafe left
		lookState = LOOK_LEFT;
	}
	else
	{
		lookState = LOOK_AHEAD;
	}

	if (controlStateFlags & FPS_LEAN_RIGHT)
	{
		if (stat2Flags & STAT2_LEAN_LEFT)
		{
			fpFPSLeanOff();
			leanState = LEAN_NONE;
		}
		else
			leanState = LEAN_RIGHT;
	}
	else if (controlStateFlags & FPS_LEAN_LEFT)
	{
		if (stat2Flags & STAT2_LEAN_RIGHT)
		{
			fpFPSLeanOff();
			leanState = LEAN_NONE;
		}
		else
			leanState = LEAN_LEFT;
	}
	else
	{
		leanState = LEAN_NONE;
	}

	if (controlStateFlags & FPS_CROUCH)
	{
		eyeState = EYE_CROUCH;
	}

	// sprint isn't meaningful unless the forward button is held down
	if ((controlStateFlags & FPS_SPRINT) && controlStateFlags & (FPS_FORWARD | FPS_AUTORUN))
	{
		moveState = FAST;
		if (controlStateFlags & FPS_PRONE)
			eyeState = EYE_STAND;
		// can't aim or fire while sprinting
		if (aimState == AIM_ON)
		{
			aimState = AIM_OFF;
		}
		fireState = FIRE_OFF;
	}

	if (stateFlags & STATE_NO_MOVEMENT)
	{
		moveState = STILL;
		strafeState = STRAFE_NONE;
	}


	// some states need to be preserved across frames
	// the rest need to be cleared and reset appropriately
	//
	stateFlags &= (STATE_KEEPERS_MASK);
	stat2Flags &= (STAT2_KEEPERS_MASK);

	// Check for RELAXED without AIM.
	if ((controlStateFlags & (FPS_RELAXED | FPS_AIM)) == FPS_RELAXED)
		stateFlags |= STATE_RELAXED;

	if (controlStateFlags & FPS_WALK)
		stateFlags |= STATE_WALK;

	if (moveState == STILL)
		stateFlags |= STATE_MOVE_STILL;
	else if (moveState == SLOW)
		stateFlags |= STATE_MOVE_SLOW;
	else if (moveState == FAST)
		stateFlags |= (STATE_MOVE_FAST);
	else if (moveState == BACKWARD)
		stateFlags |= STATE_MOVE_BACKWARD;

	if (eyeState == EYE_STAND)
		stateFlags |= STATE_EYE_STAND;
	else if (eyeState == EYE_CROUCH)
		stateFlags |= STATE_EYE_CROUCH;
	else if (eyeState == EYE_PRONE)
		stateFlags |= (STATE_EYE_PRONE);

	if (strafeState == STRAFE_LEFT)
		stateFlags |= STATE_STRAFE_LEFT;
	else if (strafeState == STRAFE_RIGHT)
		stateFlags |= STATE_STRAFE_RIGHT;

	if (turnState == TURN_LEFT)
		stateFlags |= STATE_TURN_LEFT;
	else if (turnState == TURN_RIGHT)
		stateFlags |= STATE_TURN_RIGHT;

	if (lookState == LOOK_LEFT)
		stateFlags |= STATE_LOOK_LEFT;
	else if (lookState == LOOK_RIGHT)
		stateFlags |= STATE_LOOK_RIGHT;

	if (leanState == LEAN_LEFT)
		stat2Flags |= STAT2_LEAN_LEFT;
	else if (leanState == LEAN_RIGHT)
		stat2Flags |= STAT2_LEAN_RIGHT;

	if (aimState == AIM_ON)
		stateFlags |= (STATE_AIM);
	//else
	//	stateFlags &= (~STATE_AIM);

	if (fireState == FIRE_ON)
		stateFlags |= STATE_FIRE;

	// turn off the fire key for those weapons which are not holdfire
	if (!fpsWeapons[currentWeaponModel].holdfire)
	{
		controlStateFlags &= ~FPS_FIRE;
	}

	if(controlStateFlags & FPS_SIGHT_UP)
		fpFPSSightAdjust(1);
	else if(controlStateFlags & FPS_SIGHT_DOWN)
		fpFPSSightAdjust(0);

	return TRUE;
}

LOGICAL fpFPSGetFireDirection(OBJECT* pObject, piMATRIX* pMatrix)
{
	piMATRIX		rotMatrix = IdentityMatrix;
	piMATRIX		firePoint = IdentityMatrix;
	LOGICAL			fireFind = FALSE;
	FLOAT32			horizAngle = 0, vertAngle = 0;

	fireFind = gr2GetBonePosition (weapModel, "firepoint", &firePoint);

	if (!fireFind)
	{
		if(fpsWeapons[currentWeaponModel].firepointZDistance > 0.0f)
		{
			firePoint._41 = fpsWeapons[currentWeaponModel].firepointHorizOffset + IdentityMatrix._41;
			firePoint._42 = fpsWeapons[currentWeaponModel].firepointZDistance + IdentityMatrix._42;
			firePoint._43 = ((TROOPER_STAND_EYE_HEIGHT + fpFPSGetHeight())+ fpsWeapons[currentWeaponModel].firepointVertOffset)+ IdentityMatrix._43;
		}
		else
		{
#if(0) // Don't spam chat box
			Write_Log (LOG_FPS, LOG_INFO, "fpFPSGetFirePoint: Failed to find firepoint bone, using identity!");
#endif
			firePoint = IdentityMatrix;
		}
	}

	// special handling for knives
	if(currentWeaponModel == KNIFE_ALLIED || currentWeaponModel == KNIFE_AXIS)
	{
		firePoint._41 = 0.0f;
		firePoint._42 = 0.0f;
		firePoint._43 = TROOPER_STAND_EYE_HEIGHT + fpFPSGetHeight() - .1f;
	}

	if ((fpsWeapons[currentWeaponModel].rgFiringRules) && (fpsWeapons[currentWeaponModel].auxEnable[FPS_AUX_RGCUP]) && (currentFPSstate == DEPLOYED_FIRE))
	{
		vertAngle += fpsWeapons[currentWeaponModel].rgFiringAngle;
	}

	*pMatrix = IdentityMatrix;
	pMatrix->_11 = -1.0f;
	pMatrix->_22 = -1.0f;

	if ((fpsWeapons[currentWeaponModel].aimHorizontalCorrection != 0.0) || (horizAngle != 0.0) || (horizWeaponLag != 0.0))
	{
		generateRotationMatrix (AROUND_Z, horizAngle + fpsWeapons[currentWeaponModel].aimHorizontalCorrection + horizWeaponLag, &rotMatrix);

		math3dMatrixMultiply(pMatrix,&rotMatrix,pMatrix);
	}

	// RG rounds don't use the convergence angle calculated for the rifle
	// they just fire down the barrel
	FLOAT32 convergenceAngle = (fpsWeapons[currentWeaponModel].auxEnable[FPS_AUX_RGCUP])
										? 0.0f: (float)fpsWeapons[currentWeaponModel].convergenceAngle;

	// since we're not tied to the model, we need to add in muzzle rise here as well
	generateRotationMatrix (AROUND_X,
							((AIM_VERTICAL_ANGLE + vertWeaponLag) +
							-vertAngle +
							fpsWeapons[currentWeaponModel].aimVerticalCorrection -
							convergenceAngle -
							fpsTrooper.currentMuzzleRise),
							&rotMatrix);

	math3dMatrixMultiply(pMatrix,&rotMatrix,pMatrix);

	pMatrix->_41 = firePoint._41;
	pMatrix->_42 = firePoint._42;
	pMatrix->_43 = firePoint._43;

	return (TRUE);
}

LOGICAL	fpFPSGetFirePoint(OBJECT* pObject, piMATRIX* pMatrix, bool visual)
{
	fpPersonData* personData = fpGetFPPersonDataPtr();

	if(visual && personData->Flags & fpFLAGS_FORCE_EXTERNAL)
	{
		if(gr2AnimGetFirePointMatrix (pObject, pMatrix, visual))
			return TRUE;
	}
	
	piMATRIX		rotMatrix = IdentityMatrix;
	piMATRIX		firePoint = IdentityMatrix;
	LOGICAL			fireFind = FALSE;
	FLOAT32			horizWobble = 0, vertWobble = 0;
	FLOAT32			horizAngle = 0, vertAngle = 0;

	fireFind = gr2GetBonePosition (weapModel, "firepoint", &firePoint);

	if (!fireFind)
	{
		if(fpsWeapons[currentWeaponModel].firepointZDistance > 0.0f)
		{
			firePoint._41 = fpsWeapons[currentWeaponModel].firepointHorizOffset + IdentityMatrix._41;
			firePoint._42 = fpsWeapons[currentWeaponModel].firepointZDistance + IdentityMatrix._42;
			firePoint._43 = ((TROOPER_STAND_EYE_HEIGHT + fpFPSGetHeight())+ fpsWeapons[currentWeaponModel].firepointVertOffset)+ IdentityMatrix._43;
		}
		else
		{
#if(0) // Don't spam chat box
			Write_Log (LOG_FPS, LOG_INFO, "fpFPSGetFirePoint: Failed to find firepoint bone, using identity!");
#endif
			firePoint = IdentityMatrix;
		}
	}

	// special handling for knives
	if(currentWeaponModel == KNIFE_ALLIED || currentWeaponModel == KNIFE_AXIS)
	{
		firePoint._41 = 0.0f;
		firePoint._42 = 0.0f;
		firePoint._43 = TROOPER_STAND_EYE_HEIGHT + fpFPSGetHeight() - .1f;
	}

	fpFPSGetWobble (&horizWobble, &vertWobble, &horizAngle, &vertAngle);

	if ((fpsWeapons[currentWeaponModel].rgFiringRules) && (fpsWeapons[currentWeaponModel].auxEnable[FPS_AUX_RGCUP]) && (currentFPSstate == DEPLOYED_FIRE))
	{
		vertAngle += fpsWeapons[currentWeaponModel].rgFiringAngle;
	}

	*pMatrix = IdentityMatrix;
	pMatrix->_11 = -1.0f;
	pMatrix->_22 = -1.0f;

	FLOAT64 ahc = fpsWeapons[currentWeaponModel].aimHorizontalCorrection;
	FLOAT64 avc = fpsWeapons[currentWeaponModel].aimVerticalCorrection;

	// HACK: Override offsets for BAR when deployed.
	if(currentWeaponModel == BAR && fpFPSIsDeployed())
	{
		static FLOAT64 barAHC = .26;
		static FLOAT64 barAVC = -.5;

		ahc = barAHC;
		avc = barAVC;
	}

	if ((ahc != 0.0) || (horizAngle != 0.0) || (horizWeaponLag != 0.0))
	{
		generateRotationMatrix (AROUND_Z, horizAngle + ahc + horizWeaponLag, &rotMatrix);

		math3dMatrixMultiply(pMatrix,&rotMatrix,pMatrix);
	}

	// RG rounds don't use the convergence angle calculated for the rifle
	// they just fire down the barrel
	FLOAT32 convergenceAngle = (fpsWeapons[currentWeaponModel].auxEnable[FPS_AUX_RGCUP])
										? 0.0f: (float)fpsWeapons[currentWeaponModel].convergenceAngle;

	// since we're not tied to the model, we need to add in muzzle rise here as well
	generateRotationMatrix (AROUND_X,
							((AIM_VERTICAL_ANGLE + vertWeaponLag) +
							-vertAngle +
							avc -
							convergenceAngle -
							fpsTrooper.currentMuzzleRise),
							&rotMatrix);

	math3dMatrixMultiply(pMatrix,&rotMatrix,pMatrix);

	pMatrix->_41 = firePoint._41;
	pMatrix->_42 = firePoint._42;
	pMatrix->_43 = firePoint._43;

	//if(fpsWeapons[currentWeaponModel].sniperScope && scopeUp)
	//	pMatrix->_43 -= .4f;

	gr2AnimSetWeaponFireTime (pObject, frametime);

	return (TRUE);
}

LOGICAL	fpFPSGetEjectPoint (OBJECT* pObject, piMATRIX* pMatrix, LOGICAL firstPerson)
{
	if (!firstPerson)
	{
		return gr2AnimGetEjectPointMatrix (pObject, pMatrix, true);
	}

	piMATRIX		ejectPoint;
	LOGICAL			ejectFind;

	ejectFind = gr2GetBonePosition (weapModel, "ejectpoint", &ejectPoint);

	if (!ejectFind)
	{
		return FALSE;
	}

	*pMatrix = ejectPoint;

	return (TRUE);
}

UINT32 fpFPSGetCurrentWeapon()
{
	return fpsTrooper.activeWeaponNumber;
}

void fpFPSetHeadFixed(LOGICAL fixed)
{
	if(fixed)
		stat2Flags |= STAT2_HEAD_FIXED;
	else
		stat2Flags &= ~STAT2_HEAD_FIXED;
}

LOGICAL fpFPSStateWeaponReadyToFire(bool honorDelays)
{
	if (!currentGameWeapon)
		return FALSE;

	LOGICAL		weaponReady = TRUE;
#ifdef STOP_FIRING_INSIDE
	piVECTOR	eyePoint;
	mlVehicleData*	vehicleData = persGetPersonaModelDataPtr();
	eyePoint.x = 0;
	eyePoint.y = 0;
	eyePoint.z = currentEyeHeight;
#endif

	if(honorDelays && frametime < fireDelay)
		weaponReady = FALSE;

	if (currentWeaponModel == BINOCS)
		weaponReady = FALSE;

    switch(currentFPSstate)
    {
    case LOWER:
    case RELOAD:
    case RELOAD2:
    case RELOAD3:
    case RECYCLE:
    case DEPLOYED_RELOAD:
        {
            weaponReady = FALSE;
            break;
        }
    default:
        // all other states leave weaponReady true
        break;
    }

	if (stateFlags & STATE_NEEDS_RECYCLE)
		weaponReady = FALSE;

	if (honorDelays && (stateFlags & STATE_HOLD_FIRE))
		weaponReady = FALSE;

	if (stateFlags & STATE_DEPLOYING)
		weaponReady = FALSE;

	if (currentGameWeapon->NumRoundsAmmoLeft[fpFPSGetAmmoTypeNum()] == 0)
	{
		weaponReady = FALSE;
	}

	/*
	if (fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_ROCKET &&
		fpsWeapons[currentWeaponModel].clipBullets == 0)
		weaponReady = FALSE;

	UINT32 nric = currentGameWeapon->WeaponType->NumRoundsInClip;
	UINT32 cb = fpsWeapons[currentWeaponModel].clipBullets;
	*/

	if (currentWeaponModel != BUILD_TOOL &&
		fpsWeapons[currentWeaponModel].weaponType != FPS_WEAPONTYPE_ROCKET &&
		fpsWeapons[currentWeaponModel].clipBullets == 0 &&
		!fpsWeapons[currentWeaponModel].auxEnable[FPS_AUX_RGCUP])
	{
		// this should mean we still have some rounds left, but we're
		// out of sync with the underlying vehicle
        if(currentGameWeapon->WeaponType && currentGameWeapon->WeaponType->NumRoundsInClip == 1)
		    fpFPSUpdateAmmo(fpsTrooper.activeWeaponNumber, currentGameWeapon);

		// if we really don't have any rounds left, return false
		if(fpsWeapons[currentWeaponModel].clipBullets == 0)
			weaponReady = FALSE;
	}

	if (prevPose != targetPose)
	{
		weaponReady = FALSE;
	}

	// rules for rifle grenades .. no shooting w/o a grenade loaded (duh)
	// no shooting with any movement
	if (fpsWeapons[currentWeaponModel].auxEnable[FPS_AUX_RGCUP])
	{
		mlVehicleData*	vehicleData = persGetPersonaModelDataPtr();
		if (!vehicleData || !vehicleData->PersonData)
			return FALSE;

		if (!fpsWeapons[currentWeaponModel].auxEnable[FPS_AUX_RGGRENADE])
			weaponReady = FALSE;

		if ((vehicleData->VehicleVel.x * vehicleData->VehicleVel.x +
			vehicleData->VehicleVel.y * vehicleData->VehicleVel.y +
			vehicleData->VehicleVel.z * vehicleData->VehicleVel.z) > 0.0)
		{
			weaponReady = FALSE;
		}

		if (stateFlags & STATE_AIM)
			weaponReady = FALSE;
	}

	if (!(stateFlags & STATE_DEPLOYED) && fpsWeapons[currentWeaponModel].onlyFireDeployed)
	{
		weaponReady = FALSE;
	}
/*
	if (fpsWeapons[currentWeaponModel].rgFiringRules && fpsWeapons[currentWeaponModel].auxEnable[FPS_AUX_RGCUP])
	{
		if ((currentFPSstate != DEPLOYED) && (currentFPSstate != DEPLOYED_FIRE))
			weaponReady = FALSE;
	}
*/
	if (stateFlags & STATE_WEAPON_SWITCHING)
	{
		weaponReady = FALSE;
	}

	if ((fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_RIFLE) ||
		(fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_AUTO_RIFLE) ||
		(fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_AUTO_PISTOL) ||
		(fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_REVOLVER) )
	{
		FLOAT32	currentTime = (frametime/1000.0f);

		if (currentGameWeapon->LastFireTime >= currentTime)
		{
			weaponReady = FALSE;
		}
	}

	if ((currentWeaponModel == AMMO) && (resupplyTarget))
	{
		weaponReady = FALSE;
	}

#ifdef STOP_FIRING_INSIDE
	if ((fpsWeapons[currentWeaponModel].weaponType != FPS_WEAPONTYPE_SATCHEL) && (mlInfantryInsideVehicleCheck(vehicleData, &eyePoint)))
	{
		weaponReady = FALSE;
		Write_Log (LOG_FPS, LOG_INFO, "%d fpFPSStateWeaponReadyToFire: FALSE due to collision", frametime);
	}
#endif
/*
	if (weaponReady)
	{
		Write_Log (LOG_FPS, LOG_INFO, "%d fpFPSStateWeaponReadyToFire: TRUE", frametime);
	}
	else
	{
		Write_Log (LOG_FPS, LOG_INFO, "%d fpFPSStateWeaponReadyToFire: FALSE", frametime);
	}
*/
	return weaponReady;
}

LOGICAL fpFPSStateOKToFire ()
{
	LOGICAL			okToFire;
	static SINT32	grenadeStartTime;

	okToFire = FALSE;

	if (!fpFPSStateWeaponReadyToFire())
		return FALSE;

	// this lovely tidbit is to delay the grenade fire just a little, so it looks more in sync with the arms
	if (fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_GRENADE)
	{
		if (stateFlags & STATE_FIRE_DELAYED_WEAP)
		{
			if ((grenadeStartTime == 0) && (currentFPSstate != RELOAD) && (currentFPSstate != THROW))
			{
				grenadeStartTime = frametime;
			}

			if ((grenadeStartTime + fpsWeapons[currentWeaponModel].grenadeDelayTime) < frametime)
			{
				okToFire = TRUE;
				grenadeStartTime = 0;
				stateFlags &= ~STATE_FIRE_DELAYED_WEAP;
			}
		}
	}
	else if(fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_KNIFE ||
			fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_LTMORTAR)
	{
		if (stateFlags & STATE_FIRE_DELAYED_WEAP)
		{
			okToFire = TRUE;
			stateFlags &= ~STATE_FIRE_DELAYED_WEAP;
		}
	}
	// Goofy new rule to be still while placing a satchel
	else if(fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_SATCHEL)
	{
		if(stateFlags & STATE_FIRE && !fpFPSIsMoving())
		{
			okToFire = TRUE;
		}
	}
	else
	{
		// now look to see if the fire state is actually on
		if (stateFlags & STATE_FIRE)
		{
			okToFire = TRUE;
		}
	}
/*
	if (okToFire)
	{
		Write_Log (LOG_FPS, LOG_INFO, "fpFPSStateOKToFire: TRUE");
	}
	else
	{
		Write_Log (LOG_FPS, LOG_INFO, "fpFPSStateOKToFire: FALSE");
	}
*/
	return okToFire;
}

UINT32 fpFPSGetResupplyTarget ()
{
	UINT32 target = resupplyTarget;
	resupplyTarget = 0;
	return target;
}

void fpFPSFireResupply (mlVehicleData* pVehicleData, FLOAT64VECTOR* pFirePos, piMATRIX* pFireAtt)
{
	FLOAT64VECTOR	startPos, endPos;
	FLOAT64VECTOR	WorldOriginOffset;
	FLOAT64VECTOR	projVel;		// unit vector in Y
	piVECTOR		V1, V2;
	vehVehicle*		collideVehicle;
	vehVehicle*		playerVehicle = persGetPersonaVehiclePtr();

	trnGetWorldOriginOffset(&WorldOriginOffset);

	projVel.x = projVel.z = 0.0; projVel.y = 1.0;
	mathVectorRotate64(&projVel, &projVel, pFireAtt);

	// back 0.2m
	startPos.x = -0.2 * projVel.x + pFirePos->x;
	startPos.y = -0.2 * projVel.y + pFirePos->y;
	startPos.z = -0.2 * projVel.z + pFirePos->z;

	// forward 2.0
	endPos.x = 2.0 * projVel.x + pFirePos->x;
	endPos.y = 2.0 * projVel.y + pFirePos->y;
	endPos.z = 2.0 * projVel.z + pFirePos->z;

	startPos.z = -startPos.z;
	endPos.z = -endPos.z;

	//draw3dSetWorldDebugLine (&startPos, &endPos, "ResupplyTest", colorYELLOWALPHA);

	V1.x = (FLOAT32)(startPos.x - WorldOriginOffset.x);
	V1.y = (FLOAT32)(startPos.y - WorldOriginOffset.y);
	V1.z = (FLOAT32)(startPos.z);

	V2.x = (FLOAT32)(endPos.x - WorldOriginOffset.x);
	V2.y = (FLOAT32)(endPos.y - WorldOriginOffset.y);
	V2.z = (FLOAT32)(endPos.z);

	if (trncdCollideWithVehiclesAndTerrain(&V1, &V2, nullptr, nullptr, nullptr, &collideVehicle, nullptr, FALSE, FALSE, TRUE) == TRUE)
	{
		// check to see if we poked an infantry of the same country
		if ((collideVehicle) && (playerVehicle->Side == collideVehicle->Side) && (collideVehicle->VehicleType->Type == VEH_TYPE_PERSON))
		{
			// does this need to make use of oversupply?
			--currentGameWeapon->NumRoundsAmmoLeft[currentGameWeapon->AmmoTypeInUse];
			fpFPSFireRound (currentGameWeapon, pFireAtt);
			Write_Log (LOG_FPS, LOG_INFO, "Resupply vehicle <0x%x>, handle >%s<!", collideVehicle, collideVehicle->Handle);

			resupplyTarget = collideVehicle->VehicleHostIndex;
			resupplyTime = frametime;
		}
	}

	return;
}

//////////////////////////////////////////////////////////////////////
// Check a terrain object to see if it is a valid target for a repair
// package.

static bool _isValidRepairTarget(const trnCDTerrainObject* TO)
{
	if ( nullptr == TO )
		return false;

	// Bridges are valid, but we ought to check for a defend objective.
	// FIXME - check for objective here
	// need the new strat db to be loaded for this
	if ( TO->ObjId.objClass == CLASS_CP && TO->ObjId.objType == CP_BRIDGE )
		return true;

	// You can also repair auto-gunners
	if ( TO->ObjId.objClass == CLASS_BUILDING && TO->ObjId.objType == BUILDING_AUTOGUNNER )
	{
		// It must be destroyed
		if ( TO->DamageLevel < 1 )
			return false;

		vehVehicle*playerVehicle = persGetPersonaVehiclePtr();
		// And it oughta be friendly
		if ( nullptr == playerVehicle || wwiiSide(TO->Country) != (WWII_SIDE)playerVehicle->GetSide() )
			return false;
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////
// Fire one of the repair kits that was, originally, bridges only.

void fpFPSFireBridgeRepair (mlVehicleData* pVehicleData, FLOAT64VECTOR* pFirePos, piMATRIX* pFireAtt)
{
	FLOAT64VECTOR	startPos, endPos;
	FLOAT64VECTOR	WorldOriginOffset;
	FLOAT64VECTOR	projVel;		// unit vector in Y
	piVECTOR		V1, V2;
	trnCDTerrainObject* TO = nullptr;

	trnGetWorldOriginOffset(&WorldOriginOffset);

	projVel.x = projVel.z = 0.0; projVel.y = 1.0;
	mathVectorRotate64(&projVel, &projVel, pFireAtt);

	// back 0.2m
	startPos.x = -0.2 * projVel.x + pFirePos->x;
	startPos.y = -0.2 * projVel.y + pFirePos->y;
	startPos.z = -0.2 * projVel.z + pFirePos->z;

	// forward 2.0
	endPos.x = 2.0 * projVel.x + pFirePos->x;
	endPos.y = 2.0 * projVel.y + pFirePos->y;
	endPos.z = 2.0 * projVel.z + pFirePos->z;

	startPos.z = -startPos.z;
	endPos.z = -endPos.z;

	//draw3dSetWorldDebugLine (&startPos, &endPos, "BridgeRepair", colorMAGENTAALPHA);

	V1.x = (FLOAT32)(startPos.x - WorldOriginOffset.x);
	V1.y = (FLOAT32)(startPos.y - WorldOriginOffset.y);
	V1.z = (FLOAT32)(startPos.z);

	V2.x = (FLOAT32)(endPos.x - WorldOriginOffset.x);
	V2.y = (FLOAT32)(endPos.y - WorldOriginOffset.y);
	V2.z = (FLOAT32)(endPos.z);

	if (trncdCollideWithVehiclesAndTerrain(&V1, &V2, nullptr, nullptr, nullptr, nullptr, &TO, FALSE, FALSE, TRUE) == TRUE)
	{
		if ( _isValidRepairTarget(TO) )
		{
			// does this need to make use of oversupply?
			--currentGameWeapon->NumRoundsAmmoLeft[currentGameWeapon->AmmoTypeInUse];
			fpFPSFireRound (currentGameWeapon, pFireAtt);
			Write_Log (LOG_FPS, LOG_INFO, "Repair Bridge <0x%x>, name >%s<!", TO, TO->TerrainMasterObject->name);
			vehApplyDamageToObject(fpFPSGetBridgeRepair(), TO, nullptr, nullptr);
		}
	}

	return;
}

static void ApplyBackBlastConcussionEffect(const OBJECT* o)
{
#define MAX_DISTANCE_FOR_BACK_BLAST_EFFECT  3 // meters
    piVECTOR dir = {0, 1, 0};

    // This direction was calculated before this function, but if shadows are off (China),
    // the 3rd person model animations do not play which updates the troopers firepoint matrices. So
    // we have to hack it here to come up with a ray that will be "good enough".
    math3dMultiplyRotation(&dir, &dir, (piMATRIX*)&o->mat);

    piVECTOR backdir;

    backdir.set(dir.x, dir.y, dir.z);

    piVECTOR p1, p2;
    p1.set(o->mat._41, o->mat._42, o->mat._43 - 1.5f);

    // want this: p2 = p1 + MAX_DISTANCE_FOR_BACK_BLAST_EFFECT * backdir;
    math3dScaleVector(&backdir, &backdir, MAX_DISTANCE_FOR_BACK_BLAST_EFFECT);
    math3dAddVectors(&p2, &p1, &backdir);

    // Now collide with our ray
    COLLISION c;
    c.oldpos = p2;
    c.newpos = p1;
    FASTCOLLIDE list[32];
    unsigned int numcollides = 0;

    if(trncdCollideDownShortList(c, list, numcollides))
    {
        static int duration = 2000;
        static unsigned int legs_component = FP_HIT_LOCATION_LEGS + 1;

        float damage = 88.0f; // <-- arbitrary WAG

        if(currentGameWeapon)
        {
            int ammotype = currentGameWeapon->AmmoTypeInUse.Get();
            // Use 1 percent of explosive energy (arbitrary WAG) for damage
            damage = currentGameWeapon->WeaponType->AmmoType[ammotype]->ExplosiveEnergy * 0.001f;
        }

        const FASTCOLLIDE& collide = list[0];
        // Adjust effect by collide time
        fxMiscStartExpConcussion(MAX_DISTANCE_FOR_BACK_BLAST_EFFECT * (1.0f - collide.CollideTime), duration);

        ShakeCamMagnitude = 0.15f;
        ShakeStart = frametime;
        ShakeDuration = 15000;

        vehApplyDamageToVehicleComponent(persGetPersonaVehiclePtr(),
									     legs_component,
                                         damage * (1.0f - collide.CollideTime),
									     0);
    }
}

LOGICAL fpFPSWeaponFire (mlVehicleData* pVehicleData)
{
	if (fpFPSStateOKToFire())
	{
		OBJECT*			pObject = nullptr;
		vehVehicle*		pVehicle = nullptr;

		FLOAT64VECTOR	EmptyVector = {0, 0, 0}; // For placeholder vectors, etc.
		FLOAT64VECTOR	RecoilVector = {0, 0, 0};
		UINT32			flags = 0;
		piMATRIX		FirePointMatrix = IdentityMatrix, FireAttitudeMatrix = IdentityMatrix;
		FLOAT64VECTOR	FirePositionVector = {0, 0, 0};
		FLOAT64VECTOR	FlashPositionVector = {0, 0, 0};
		FLOAT64VECTOR	MuzzlePositionVector = {0, 0, 0}; // for relative muzzle flash positioning (if needed be)
		SINT32			weaponNumber = 0;
		SINT32			FireType = 0, SoundType = 0, TempID = 0, *pSoundID = nullptr;

		pObject = pVehicleData->VehicleObjectData->PlaneObject;

		EmptyVector.x = EmptyVector.y = EmptyVector.z = 0; // Make sure it's really empty

		// player cannot usually be hit by own weapons
		flags = WEAP_AMMOFLAG_USE_MEDCOLLIDELIST | WEAP_AMMOFLAG_CANTHITPLAYER | WEAP_AMMOFLAG_FPS_WEAPON;

		if(fpsWeapons[currentWeaponModel].sniperScope && scopeUp)
			flags |= WEAP_AMMOFLAG_NOMUZZLEFLASH;

		// instead of this, which is only here for testing
		weaponNumber = fpsTrooper.activeWeaponNumber;

		// Determine Point at which bullets should come out
		fpFPSGetFirePoint(pObject, &FirePointMatrix);

		FlashPositionVector.x = FirePointMatrix._41;
		FlashPositionVector.y = FirePointMatrix._42;
		FlashPositionVector.z = FirePointMatrix._43;

		FireAttitudeMatrix = FirePointMatrix;
		FireAttitudeMatrix._41 = 0;
		FireAttitudeMatrix._42 = 0;
		FireAttitudeMatrix._43 = 0;

		math3dMatrixMultiply(&FireAttitudeMatrix, &FireAttitudeMatrix, &pVehicleData->VehicleAttMat);

		mathVectorMultiply64(&FirePositionVector, &FlashPositionVector, &pVehicleData->VehicleAttMat);

		mathAddVectors(&FirePositionVector, &FirePositionVector, &(pVehicleData->VehiclePos));

		// special case for mortars since they are sto weapons and the muzzle flash
		// doesn't quite line up with the tps weapon in external view
		if(fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_LTMORTAR)
		{
			fpPersonData* pd = fpGetFPPersonDataPtr();

			// find the difference
			if(pd && (pd->Flags & fpFLAGS_FORCE_EXTERNAL))
			{
				MuzzlePositionVector.y = -.6f; // arbitrary
				mathVectorMultiply64(&MuzzlePositionVector, &MuzzlePositionVector, &pVehicleData->VehicleAttMat);
			}
		}

		pVehicle = (vehVehicle*)pVehicleData->VehicleMasterPtr;

#ifdef BETA_RELEASE
		// fixme - not sure this is a good idea for non beta builds, it might cause trouble
		//
		// done as a last second thing so I could test the crouch-strafe/firing interaction
		//
		// woops, it causes trouble (double firing) .. but it's still handy for testing
		/*
		if(!Online)
		{
			gr2AnimTrooper* anim = pVehicle->GetAnim();
			if(anim) anim->firing = 1;
		}
		*/
			//gr2AnimFireWeapon(pVehicle);
#endif
		if (currentWeaponModel == BUILD_TOOL)
		{
            fpPPOSwitchPPO(true);
		}
		else if ((currentWeaponModel == AMMO) && (!resupplyTarget))
		{
			Write_Log (LOG_FPS, LOG_INFO, "Ammo weapon fire!");
			fpFPSFireResupply (pVehicleData, &FirePositionVector, &FireAttitudeMatrix);
			//fpsTrooper.firedWeapon = 1;
		}
		else if (currentWeaponModel == BRIDGE_REPAIR)
		{
			fpFPSFireBridgeRepair (pVehicleData, &FirePositionVector, &FireAttitudeMatrix);
			//fpsTrooper.firedWeapon = 1;
		}
		else if (pVehicle != nullptr)
		{
			UINT32 fired = weapFireWeapon(&RecoilVector,
							  currentGameWeapon,
							  &FirePositionVector,
							  &EmptyVector,
							  &FireAttitudeMatrix,
							  (frametime - fpsTrooper.lastFireCheckTime) / 1000.0,
							  frametime / 1000.0,
							  1,
							  0,
							  flags,
							  nullptr,
							  &MuzzlePositionVector,
							  nullptr,
							  pVehicle);

			if (currentGameWeapon->WeaponType != nullptr)
			{
				FireType = currentGameWeapon->WeaponType->FireType;

				if(((RecoilVector.x * RecoilVector.x) > 0.00001 ||
					(RecoilVector.y * RecoilVector.y) > 0.00001 ||
					(RecoilVector.z * RecoilVector.z) > 0.00001))
				{
					//pVehicleData->MountIsFiring[0] = TRUE;

					if (FireType == WEAP_FIRETYPE_SINGLESHOT)
					{
						SoundType = TYPE_FINITE_SOUND;
						pSoundID = &TempID;
					}
					else
					{
						SoundType = TYPE_INFINITE_SOUND;
						pSoundID = &fpsTrooper.autoWeaponFireSoundId;
					}

					if (*pSoundID == 0)
					{
                        if(currentGameWeapon->WeaponType->IsPlacedWeapon)
                        {
                            fxSndAddFiniteObjectSound((char*)pVehicle->VehicleType->MountSounds[weaponNumber],
                                                      IDE_FP_PRIORITY_MANDATORY,
                                                      2,
                                                      20,
                                                      FlashPositionVector,
                                                      pObject);
                        }
                        else
                        {
						    *pSoundID = fpFPSfxPlayFireSound ((SCHAR*)pVehicle->VehicleType->MountSounds[weaponNumber],
                                                             IDE_FP_PRIORITY_MANDATORY,
                                                             SoundType,
                                                             &FlashPositionVector,
                                                             pObject);
                        }

                        if(fired && pObject && pObject->animModel)
                        {
                            if(pObject->animModel->currentWeaponType == TPS_WEAPONTYPE_ROCKET)
                            {
								TransformEffect* te = nullptr;
                                piMATRIX m = IdentityMatrix;
                                gr2AnimGetBoneMatrix(pObject, &m, "alt_firepoint");

								if(pObject->animModel->weaponArray[pObject->animModel->currentWeapon] != PIAT)
								{
									m._23 = -m._23;
									m._43 = -m._43;

									math3dMatrixMultiply(&m, &m, &pObject->mat);

									// Default direction along Y axis
									piVECTOR dir;
									dir.set(0, 1, 0);
									math3dMultiplyRotation(&dir, &dir, (piMATRIX*)&m);

									math3dNormalize(&dir);

									Matrixd mat(m);

									te = TransformEffect::AddEffect("bazookabackflash", Vec3f(dir), 0, nullptr);

									if(te)
									{
										te->enableemitters(true);;
										te->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);

										mat(3, 0) += WorldOriginOffset.x;
										mat(3, 1) += WorldOriginOffset.y;

										te->setmatrix(mat);
									}

	                                // Ticket # 5852. Add concussion/damage if backfire is close to an object
                                    if(persIsCameraInternalMode())
                                        ApplyBackBlastConcussionEffect(pObject);
								}

                                if(pObject->animModel->weaponArray[pObject->animModel->currentWeapon] == PIAT)
			                        te = TransformEffect::AddEffect("muzzlesmoke", Vec3f(0, 0, 0), 0, nullptr);
                                else
			                        te = TransformEffect::AddEffect("bazookafrontflash", Vec3f(0, 0, 0), 0, nullptr);

			                    if(te)
			                    {
				                    te->enableemitters(true);;
				                    te->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);

                                    gr2AnimGetBoneMatrix(pObject, &m, "firepoint");

                                    m._23 = -m._23;
                                    m._43 = -m._43;

                                    math3dMatrixMultiply(&m, &m, &pObject->mat);

									Matrixd mat;
                                    mat.set(m);

                                    mat(3, 0) += WorldOriginOffset.x;
                                    mat(3, 1) += WorldOriginOffset.y;

                                    te->setmatrix(mat);
			                    }
                            }
                        }
					}
				}
				else
				{
					if (FireType == WEAP_FIRETYPE_SINGLESHOT)
					{
						pVehicleData->PersonData->IsFiring = FALSE;
					}
					else
					{
						if (pVehicle->Weapons[weaponNumber][0].NumRoundsAmmoLeft[fpFPSGetAmmoTypeNum()] == 0)
						{
							if (fpsTrooper.autoWeaponFireSoundId != 0)
							{
								fxSndStopSound(fpsTrooper.autoWeaponFireSoundId);
								fpsTrooper.autoWeaponFireSoundId = 0;
							}
						}
					}
				}

				if (FireType != WEAP_FIRETYPE_SINGLESHOT)
				{
					if (weapIsWeaponReloading(&(pVehicle->Weapons[weaponNumber][0]), frametime / 1000.0) == TRUE)
					{
						if (fpsTrooper.autoWeaponFireSoundId != 0)
						{
							fxSndStopSoundDelayed(fpsTrooper.autoWeaponFireSoundId, 1);
                            fpsTrooper.autoWeaponFireSoundId = 0;
						}
					}

					if(fired)
						fpsTrooper.tpsAttack = frametime + 300;
				}
				else
				{
					if(fired)
					{
						switch(fpsWeapons[currentWeaponModel].weaponType)
						{
							case FPS_WEAPONTYPE_RIFLE:
							case FPS_WEAPONTYPE_AUTO_RIFLE:
							case FPS_WEAPONTYPE_AUTO_PISTOL:
							case FPS_WEAPONTYPE_REVOLVER:
							case FPS_WEAPONTYPE_SATCHEL:
							case FPS_WEAPONTYPE_ATR:
							case FPS_WEAPONTYPE_LTMORTAR:
							case FPS_WEAPONTYPE_ROCKET:
								fpsTrooper.tpsAttack = 1;
							break;
                            default:
                                // don't care about other weapon types
                                break;
						}
					}
				}

				// give mortars some fire concussion
				if(fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_LTMORTAR)
				{
					persShakeCamera(.01f, 300);
				}
			}
		}

	}
	else
	{
		if (fpsTrooper.autoWeaponFireSoundId != 0)
		{
			fxSndStopSound(fpsTrooper.autoWeaponFireSoundId);
			fpsTrooper.autoWeaponFireSoundId = 0;
		}
		fpsTrooper.autoWeaponFireSoundId = 0;
	}

	fpsTrooper.lastFireCheckTime = frametime;

	return TRUE;

}

void fpFPSEjectShell (OBJECT* pObject, piMATRIX* fireAtt, LOGICAL firstPerson)
{
	piVECTOR		gunDir, shellDir, troopVelocity;
	FLOAT32			shellSpeed;
	FLOAT64VECTOR	shellPosition;
	piMATRIX		shellMatrix;
	mlVehicleData*	pVehicleData = persGetPersonaModelDataPtr();
	LOGICAL			ejectAvailable;
	piMATRIX		attitudeMatrix;
	FLOAT64VECTOR	positionVector;
	FLOAT32			scale;


	// the gunDir has already been converted to world coordinates, so just change it from a matrix to a vector and we're done
	gunDir.x = fireAtt->_21;
	gunDir.y = fireAtt->_22;
	gunDir.z = fireAtt->_23;

	if (firstPerson)
	{
		scale = fpsWeapons[currentWeaponModel].shellScale;
	}
	else // FIXME - scale up 3rd person rifle shells
		scale = 0.8f;

	if ((firstPerson) || (!pObject))
	{
		attitudeMatrix = pVehicleData->VehicleAttMat;
		positionVector = pVehicleData->VehiclePos;
	}
	else
	{
		vehVehicle* veh = gr2AnimGetVehicle(pObject);

		if(!veh)
			return;

        if(veh->ModelData)
        {
            // If external view from own vehicle, use other data
            attitudeMatrix = pVehicleData->VehicleAttMat;
            positionVector = pVehicleData->VehiclePos;
        }
        else
        {
		    attitudeMatrix = veh->PosData.AttMat;
		    positionVector = veh->PosData.Pos;
        }
	}

	// get the gun-relative eject point
	ejectAvailable = fpFPSGetEjectPoint (pObject, &shellMatrix, firstPerson);

	if (!ejectAvailable)
	{
		Write_Debug ("No ejection point found");
		return;
	}

	// now rotate it so it's rotated with the trooper ..
	math3dMatrixMultiply(&shellMatrix, &shellMatrix, &attitudeMatrix);

	// and then break it up into two vectors, one for position, one for direction
	// shell ejecting out the Y axis
	shellDir.x = shellMatrix._21;
	shellDir.y = shellMatrix._22;
	shellDir.z = shellMatrix._23;

	shellPosition.x = shellMatrix._41;
	shellPosition.y = shellMatrix._42;
	shellPosition.z = shellMatrix._43;

	mathAddVectors(&shellPosition, &shellPosition, &positionVector);

	shellSpeed = 1.0f;

	if (firstPerson)
	{
		troopVelocity.x = (FLOAT32)pVehicleData->VehicleVel.x;
		troopVelocity.y = (FLOAT32)pVehicleData->VehicleVel.y;
		troopVelocity.z = (FLOAT32)pVehicleData->VehicleVel.z;
	}
	else
	{
		memset (&troopVelocity, 0, sizeof(troopVelocity));
	}

	fpFPSfxEjectShell(gunDir, shellDir, troopVelocity, shellPosition, shellSpeed, firstPerson, scale, positionVector);
}
// this gets called from the weapon code whenever a round is actually fired
// used for muzzle climb
// used to determine weapon cycling for rifles
// used to set state for grenades
// used to set state for stachels
void fpFPSFireRound (weapWeapon* weapon, piMATRIX* fireAtt, void* FireVehicle)
{
	// if we have a fire vehicle, try to do a visual attack
	if(FireVehicle)
	{
		vehVehicle* v = (vehVehicle*)FireVehicle;

		// do it; firearm attacks are always attack1 (at least for now)
		gr2AnimVisualAttack(v->GetAnim(), TPS_POINTTYPE_ATTACK1);
	}

	// save our update number
	fireUpdateNo = wwiiGetUpdateNo(false);

	bool autoSwitchWeapon = false;

	// handle rifle grenades
	if (fpsWeapons[currentWeaponModel].auxEnable[FPS_AUX_RGGRENADE])
	{
		fpsWeapons[currentWeaponModel].auxEnable[FPS_AUX_RGGRENADE] = 0;
		rgRoundFired = true;
		/*
		// update rifle clip ammo
		if(currentGameWeapon->NumRoundsAmmoLeft[0] > 0)
			--currentGameWeapon->NumRoundsAmmoLeft[0];

		if(currentGameWeapon->NumRoundsAmmoLeft[0] == 0)
			autoSwitchWeapon = true; // must have rifle rounds left

		--fpsWeapons[currentWeaponModel].clipBullets;
		int cb = fpsWeapons[currentWeaponModel].clipBullets;
		*/
	}
	else if (!SuperTroop && currentGameWeapon->NumRoundsAmmoLeft[currentGameWeapon->AmmoTypeInUse] >= 0) // don't take away bullets for infinite ammo (-1)
	{
		--fpsWeapons[currentWeaponModel].clipBullets;
	}

#ifdef TESTING
	int cb = fpsWeapons[currentWeaponModel].clipBullets;
#endif

	// garand's distinctive ping sound
	if(currentWeaponModel == M1GARAND && fpsWeapons[currentWeaponModel].clipBullets == 0)
	{
		SINT32 id = fpFPSfxPlayNormalSound("sounds/troop/garandping.wav", IDE_FP_PRIORITY_HIGH, TYPE_FINITE_SOUND, nullptr, persGetPersonaVehicleObjectDataPtr());
		if(id > -1) fxSndSetVolume(id, 2.0f);
	}

	// do we need to switch weapons?
	if (autoSwitchWeapon || fpFPSWeaponOutOfAmmo(currentGameWeapon))
		fpFPSWeaponAutoSwitch();

	// make sure we have an ammo type with rounds left in it
	_selectBestAmmo(fpsTrooper.activeWeaponNumber, currentGameWeapon);

	if (fpsWeapons[currentWeaponModel].ejectImmediate)
	{
		if (fpGetFPPersonDataPtr()->Flags & fpFLAGS_FORCE_EXTERNAL)
		{
			fpFPSEjectShell (persGetPersonaVehicleObjectDataPtr(), fireAtt, FALSE);
		}
		else
		{
			fpFPSEjectShell (persGetPersonaVehicleObjectDataPtr(), fireAtt, TRUE);
		}
	}
	else if ((fpsWeapons[currentWeaponModel].ejectDelay > 0) && (!(fpsWeapons[currentWeaponModel].auxEnable[FPS_AUX_RGCUP])))
	{
		shellEjectTime = -1;
	}

#if AIMING_WEAPONS
#else
	if (fpsWeapons[currentWeaponModel].muzzleRise)
	{
		FLOAT32		muzzleClimbAmount;
		FLOAT64		aimError;
		mlVehicleData*	pVehicleData = persGetPersonaModelDataPtr();

		muzzleClimbAmount = fpsWeapons[currentWeaponModel].muzzleRise;

		if ((fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_LMG) && (stateFlags & STATE_DEPLOYED))
		{
			muzzleClimbAmount *= fpsTrooper.proneDeployedClimbMultiplier;
		}
		fpsTrooper.targetMuzzleRise += muzzleClimbAmount;

		// horiz
		aimError = (mathGetFloat32ChaoticNumber() - 0.5) * fpsWeapons[currentWeaponModel].muzzleRise *	fpsTrooper.aimControl;

		aimError *= fpsWeapons[currentWeaponModel].bulletJitter * fpsWeapons[currentWeaponModel].horizJitterMult;

		fpFPSWeaponType wtype = fpsWeapons[currentWeaponModel].weaponType;

		if ((wtype == FPS_WEAPONTYPE_LMG || wtype == FPS_WEAPONTYPE_ATR) && (stateFlags & STATE_DEPLOYED))
		{
			aimError *= fpsTrooper.proneDeployedHorizShiftMultiplier;
			fpFPSSetHorizontalDeployedPosition (aimHorizontalDeployedPosition + (aimError / (fpsTrooper.mouseGranularity * fpsTrooper.deployedGranularityMultiplier)));
		}
		else
		{
			pVehicleData->VehicleAtt.Yaw += aimError * PI/180.0;
		}

		// vert
		aimError = (mathGetFloat32ChaoticNumber() - 0.5) * fpsWeapons[currentWeaponModel].muzzleRise *	fpsTrooper.aimControl;

		aimError *= fpsWeapons[currentWeaponModel].bulletJitter;

		if ((wtype == FPS_WEAPONTYPE_LMG || wtype == FPS_WEAPONTYPE_ATR) && (stateFlags & STATE_DEPLOYED))
		{
			aimError *= fpsTrooper.proneDeployedVertShiftMultiplier;
			fpFPSSetVerticalDeployedPosition (aimVerticalDeployedPosition + (aimError / (fpsTrooper.mouseGranularity * fpsTrooper.vertGranularityMultiplier * fpsTrooper.deployedGranularityMultiplier)));
		}
		else
		{
			aimVertAngle += aimError;
		}
	}
#endif

	// if we're not an holdfire weapon, clear the fire input
	// holdfire weapons leave it on so they continue to fire
	if (!fpsWeapons[currentWeaponModel].holdfire)
	{
		stateFlags &= ~STATE_FIRE;
	}

	// if we're a rifle (for recycle) and not a rifle with cup attached
	if ((fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_RIFLE) && !(fpsWeapons[currentWeaponModel].auxEnable[FPS_AUX_RGCUP]))
	{
		stateFlags |= STATE_NEEDS_RECYCLE;
	}

	// need handling for rg ammo here
	// the above block may need adjustment

	if (fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_ATR)
	{
		if (currentGameWeapon->NumRoundsAmmoLeft[currentGameWeapon->AmmoTypeInUse] > 0)
			stateFlags |= STATE_NEEDS_RECYCLE;
	}

	if (fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_ROCKET)
	{
		stateFlags &= ~STATE_FIRE;
		controlStateFlags &= ~FPS_FIRE;
	}

	// satchels immediately go to reload
	if (fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_SATCHEL ||
		fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_BUILD ||
		fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_LTMORTAR)
	{
        fpFPSForcedReload();
	}

	if (fpsWeapons[currentWeaponModel].perBulletATP)
	{
		fpFPSBulletATPEffect += fpsWeapons[currentWeaponModel].perBulletATP;
	}

	if (fpsWeapons[currentWeaponModel].delayAfterFire)
	{
		fpsTrooper.fireTimeForDelay = frametime;
	}

	if(gEZMode && EZ_MGR)
	{
		EZTrooper* et = dynamic_cast<EZTrooper*>(EZ_MGR);
		et->AddDispersion(.1f);
	}
}

// generic weapon cycle (currently only used for rifles)
void fpFPSRecycle ()
{
	stateFlags &= ~STATE_NEEDS_RECYCLE;

	controlStateFlags &= ~FPS_FIRE;

	if (fpsWeapons[currentWeaponModel].recycleSound)
	{
		fpFPSfxPlayNormalSound (fpsWeapons[currentWeaponModel].recycleSound, IDE_FP_PRIORITY_HIGH, TYPE_FINITE_SOUND, nullptr, persGetPersonaVehicleObjectDataPtr());
    }

	if (currentWeaponModel == BOYSMK1)
	{
	    fpFPSDetachWeapon (currentWeaponModel);
		if (!gr2AttachModel (weapModel, "atrsecondary-connect", skeletonModel, "rifle-control"))
		{
			Write_Log (LOG_FPS, LOG_INFO, "Failed to attach BOYSMK1 to skeleton");
		}
	}
}

// generic reload .. called directly from the weapon fire code
void fpFPSReload (weapWeapon* weapon)
{
	// no reload if this flag is on and there's muzzle rise
	if (fpsWeapons[currentWeaponModel].riseStopsReload && (fpsTrooper.currentMuzzleRise > 0 || fpsTrooper.targetMuzzleRise > 0))
	{
		stat2Flags |= STAT2_NEEDS_RELOAD;
		return;
	}

	if (fpsWeapons[currentWeaponModel].delayAfterFire > 0)
	{
		SINT32	timeElapsed = frametime - fpsTrooper.fireTimeForDelay;

		if (timeElapsed < (fpsWeapons[currentWeaponModel].delayAfterFire * 1000.0))
		{
			stat2Flags |= STAT2_NEEDS_RELOAD;
			return;
		}
	}

	if (fpsWeapons[currentWeaponModel].weaponType != FPS_WEAPONTYPE_REVOLVER)
	{
        // French Grenadier/Sniper HACKED!..since it reloads 'single bullet style'
        if(currentWeaponModel != RG_VB && currentWeaponModel != LEBELMLE && currentWeaponModel != SPRINGFIELD1903)
        {
			if(!fpsWeapons[currentWeaponModel].auxEnable[FPS_AUX_RGCUP])
				fpFPSUpdateAmmo(fpsTrooper.activeWeaponNumber, currentGameWeapon);

			/*
		    if (currentGameWeapon->NumRoundsAmmoLeft[currentGameWeapon->AmmoTypeInUse] == -1)
			    fpsWeapons[currentWeaponModel].clipBullets = currentGameWeapon->WeaponType->NumRoundsInClip;
		    // wee bit o' hackin' here for the rifle grenades
		    else if (fpFPSGetAmmoTypeNum() != 0)
			    fpsWeapons[currentWeaponModel].clipBullets = 1;
		    else
			    fpsWeapons[currentWeaponModel].clipBullets = std::min(currentGameWeapon->WeaponType->NumRoundsInClip,
																  (UINT32)currentGameWeapon->NumRoundsAmmoLeft[fpFPSGetAmmoTypeNum()]);
			*/
        }
    }
//	Write_Log (LOG_FPS, LOG_INFO, "Reloaded into clip: %d", fpsWeapons[currentWeaponModel].clipBullets);

	if(fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_LTMORTAR)
		return;

	// based on our current state, clean up things
	fpFPSCleanupAnimationState ();

	if (stateFlags & STATE_DEPLOYED)
	{
		currentFPSstate = DEPLOYED_RELOAD;
	}
	else
	{
		currentFPSstate = RELOAD;
	}

	// based on the new state, set up and start animations
	fpFPSStartAnimationState (currentWeaponModel, currentFPSstate);

	stateFlags |= STATE_RELOADING;
	stateFlags &= ~STATE_NEEDS_RECYCLE;
	stat2Flags &= ~STAT2_NEEDS_RELOAD;

	assert (currentGameWeapon == weapon);

	SCHAR* reloadSound = nullptr;

	if (fpsWeapons[currentWeaponModel].auxEnable[FPS_AUX_RGCUP])
		reloadSound = fpsWeapons[currentWeaponModel].rgReloadSound;
	else
		reloadSound = fpsWeapons[currentWeaponModel].reloadSound;

	if (reloadSound)
	{
		fpFPSfxPlayNormalSound (reloadSound, IDE_FP_PRIORITY_HIGH, TYPE_FINITE_SOUND, nullptr, persGetPersonaVehicleObjectDataPtr());
    }
}

// this is where auto-reload gets blocked (called from weapon system)
LOGICAL	fpFPSReloadCheck (weapWeapon* weapon, SINT32 ammoGroup)
{
//	Write_Log (LOG_FPS, LOG_INFO, "Left in clip: %d", fpsWeaponData[currentWeaponModel].clipBullets);

	if (!fpsTrooper.autoReload)
		return FALSE;

	// doesn't apply to single-fire weapons .. these get force reloaded when appropriate
	if ((fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_GRENADE) ||
		(fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_SATCHEL) ||
		(fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_ITEM) ||
		(fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_BUILD) ||
		(fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_LTMORTAR))
		return FALSE;

	// don't force reload if we're a deployed RG rifle
	if (fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_RIFLE)
	{
		if (fpsWeapons[currentWeaponModel].auxEnable[FPS_AUX_RGCUP] && (stateFlags & STATE_DEPLOYED))
			return FALSE;
	}

	if (fpsTrooper.parachuteOKToOpen)
		return FALSE;

	// garand can't reload with bullets still in the clip
	//if (fpsWeapons[currentWeaponModel].clipBullets > 0 && currentWeaponModel == M1GARAND)
	//	return FALSE;

	// no rocket reloading while moving
	//if (fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_ROCKET && fpFPSIsMoving(.1f))
	//	return FALSE;

	if ((fpsWeapons[currentWeaponModel].clipBullets <= 0) &&
			(weapon->NumRoundsAmmoLeft[ammoGroup] > 0))
		return TRUE;
	else
		return FALSE;
}

// manual reload
void fpFPSManualReload ()
{
	if ((fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_SATCHEL) ||
		(fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_GRENADE) ||
		(fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_ITEM) ||
		(fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_BUILD) ||
		(fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_KNIFE) ||
		(fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_LTMORTAR))
	{
		return;
	}

	if (fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_ATR)
		if (!(stateFlags & STATE_DEPLOYED))
			return;

	if (currentGameWeapon->NumRoundsAmmoLeft[fpFPSGetAmmoTypeNum()] == 0)
		return;


	fpFPSForcedReload();
}

// reload that's gotta happen if at all possible
void fpFPSForcedReload ()
{
	// check for weapons that don't reload
	if (currentWeaponModel == BINOCS)
		return;

	if ((currentFPSstate == RELOAD) || (currentFPSstate == RELOAD2) ||
		(currentFPSstate == RELOAD3) || (currentFPSstate == DEPLOYING) ||
		(currentFPSstate == UNDEPLOYING) || (currentFPSstate == DEPLOYED_RELOAD) ||
		(currentFPSstate == RECYCLE))
		return;

	if (stateFlags & STATE_NORELOAD_MASK)
		return;

	if (fpsTrooper.parachuteOKToOpen)
		return;

	// garand can't reload with bullets still in the clip
	/*
	if (fpsWeapons[currentWeaponModel].clipBullets > 0 && currentWeaponModel == M1GARAND)
	{
		persShakeHead();
		return;
	}
	*/

	// no rocket reloading while moving
	if (fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_ROCKET && fpFPSIsMoving(.01f))
	{
		persShakeHead("common.headshake.reloadstill");
		return;
	}

	if ((fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_RIFLE) && (fpsWeapons[currentWeaponModel].auxEnable[FPS_AUX_RGCUP]))
	{
		mlVehicleData*	vehicleData = persGetPersonaModelDataPtr();
		if (!vehicleData || !vehicleData->PersonData)
			return;

		bool ok = true;

		if(fpFPSWeaponOutOfAmmo(currentGameWeapon))
			ok = false;
		else if(fpsWeapons[currentWeaponModel].auxEnable[FPS_AUX_RGGRENADE] ||
				currentGameWeapon->NumRoundsAmmoLeft[1] <= 0)
			ok = false;
		else if(false && (vehicleData->VehicleVel.x * vehicleData->VehicleVel.x +
							vehicleData->VehicleVel.y * vehicleData->VehicleVel.y +
							vehicleData->VehicleVel.z * vehicleData->VehicleVel.z) > 0.0)
			ok = false;

		if(!ok)
		{
			persShakeHead();
			return;
		}
	}

	currentGameWeapon->ReloadStartTime = frametime / 1000.0;

	fpFPSReload(currentGameWeapon);
}

static bool _isReloadable(fpFPSWeaponData& weapon)
{
	if((weapon.weaponType == FPS_WEAPONTYPE_SMG) ||
		(weapon.weaponType == FPS_WEAPONTYPE_LMG) ||
		(weapon.weaponType == FPS_WEAPONTYPE_ATR) ||
		(weapon.weaponType == FPS_WEAPONTYPE_AUTO_PISTOL) ||
		(weapon.weaponType == FPS_WEAPONTYPE_RIFLE) ||
		(weapon.weaponType == FPS_WEAPONTYPE_AUTO_RIFLE) ||
		(weapon.weaponType == FPS_WEAPONTYPE_REVOLVER))
		return true;

	return false;
}

static void _sitFlags(UINT32& pFlags, fpFPSWeaponData& weapon)
{
	///////// pose
	UINT32 poseFlag = TPS_POSETYPE_SIT;

	//////// move
	UINT32 moveFlag = TPS_MOVETYPE_STILL;

	//////// point
	UINT32 pointFlag = TPS_POINTTYPE_RELAXED;

	if((stateFlags & STATE_RELOADING) && _isReloadable(weapon))
		pointFlag = TPS_POINTTYPE_RELOAD;
	else if(fpsTrooper.activeWeaponNumber == MAP_WEAPON_NUMBER)
		pointFlag = TPS_POINTTYPE_READY;

	// set the flags
	pFlags = pFlags << gr2AnimPoseBits;
	pFlags |= poseFlag;

	pFlags = pFlags << gr2AnimMoveBits;
	pFlags |= moveFlag;

	pFlags = pFlags << gr2AnimPointBits;
	pFlags |= pointFlag;
}

static void _swimFlags(UINT32& pFlags, fpFPSWeaponData& weapon)
{
	//////// pose
	UINT32 poseFlag = TPS_POSETYPE_SWIM;

	//////// move
	UINT32 moveFlag = TPS_MOVETYPE_STILL;

	// move
	if(fpFPSIsMoving(.2f) && !(stateFlags & STATE_MOVE_BACKWARD))
		moveFlag = TPS_MOVETYPE_FORWARD;

	//////// point
	UINT32 pointFlag = TPS_POINTTYPE_RELAXED;

	if(moveFlag == TPS_MOVETYPE_STILL && (fpsTrooper.activeWeaponNumber == MAP_WEAPON_NUMBER ||
										  fpsTrooper.activeWeaponNumber == BINOCS_WEAPON_NUMBER))
	{
		pointFlag = TPS_POINTTYPE_READY;
	}

	// set the flags
	pFlags = pFlags << gr2AnimPoseBits;
	pFlags |= poseFlag;

	pFlags = pFlags << gr2AnimMoveBits;
	pFlags |= moveFlag;

	pFlags = pFlags << gr2AnimPointBits;
	pFlags |= pointFlag;
}

static void _proneFlags(UINT32& pFlags, fpFPSWeaponData& weapon)
{
	//////// pose
	UINT32 poseFlag = TPS_POSETYPE_PRONE;

	//////// move
	UINT32 moveFlag = TPS_MOVETYPE_STILL;

	if(stateFlags & STATE_DEPLOYED && weapon.deployPivotBone)
	{
		moveFlag = TPS_MOVETYPE_DEPLOYED;
	}
	else if(fpFPSIsMoving(.01f))
	{
		if(stateFlags & STATE_MOVE_FAST)
		{
			poseFlag = TPS_POSETYPE_STAND;
			moveFlag = TPS_MOVETYPE_SPRINT;
		}
		else if(stateFlags & STATE_MOVE_BACKWARD)
		{
			moveFlag = TPS_MOVETYPE_BACKWARD;
		}
		else if(stateFlags & (STATE_MOVE_SLOW|STATE_STRAFE_LEFT|STATE_STRAFE_RIGHT))
		{
			moveFlag = TPS_MOVETYPE_FORWARD;
		}
	}

	//////// point
	UINT32 pointFlag = TPS_POINTTYPE_RELAXED;

	/*
	if(weapon.weaponType == FPS_WEAPONTYPE_GRENADE && (stateFlags & (STATE_FIRE|STATE_FIRE_DELAYED_WEAP)))
	{
		pointFlag = TPS_POINTTYPE_ATTACK1;
	}
	else if(weapon.weaponType == FPS_WEAPONTYPE_LTMORTAR && HAS_ALL_BITS(stateFlags, STATE_DEPLOYED|STATE_HOLD_FIRE))
	{
		pointFlag = TPS_POINTTYPE_ATTACK1;
	}
	else if(weapon.weaponType == FPS_WEAPONTYPE_KNIFE && (stateFlags & STATE_KNIFE_ATTACKING))
	{
		pointFlag = TPS_POINTTYPE_ATTACK1;
	}
	else
	*/
	if((stateFlags & STATE_RELOADING) && _isReloadable(weapon))
	{
		pointFlag = TPS_POINTTYPE_RELOAD;
	}

	switch(moveFlag)
	{
		case TPS_MOVETYPE_STILL:
		{
			if(currentWeaponModel == PIAT && (stateFlags & STATE_AIM))
			{
				pointFlag = TPS_POINTTYPE_AIM;
			}
			else if(weapon.weaponType != FPS_WEAPONTYPE_LTMORTAR && weapon.weaponType != FPS_WEAPONTYPE_ATR)
			{
				if(pointFlag == TPS_POINTTYPE_RELAXED)
					pointFlag = TPS_POINTTYPE_READY;
			}
		}
		break;
		case TPS_MOVETYPE_DEPLOYED:
		{
			if(pointFlag != TPS_POINTTYPE_RELOAD) // pointFlag != TPS_POINTTYPE_ATTACK1 &&
				pointFlag = TPS_POINTTYPE_READY;
		}
		break;
	}

	// can't do anything while prone with rockets
	if(weapon.weaponType == FPS_WEAPONTYPE_ROCKET && (currentWeaponModel != PIAT || moveFlag != TPS_MOVETYPE_STILL))
		pointFlag = TPS_POINTTYPE_RELAXED;

	// set the flags
	pFlags = pFlags << gr2AnimPoseBits;
	pFlags |= poseFlag;

	pFlags = pFlags << gr2AnimMoveBits;
	pFlags |= moveFlag;

	pFlags = pFlags << gr2AnimPointBits;
	pFlags |= pointFlag;
}

// crouch and stand are handled the same way
static void _standFlags(UINT32& pFlags, fpFPSWeaponData& weapon, UINT32 poseFlag)
{
	static SINT32 nextIdleCheck = frametime + 15000;
	static SINT32 currentIdle = -1;

	// how long after combat can we idle
	const SINT32 combatCheck = 15000; // 2 mins

	// how long between each idle check
	const SINT32 idleCheck = 10000;

	//////// pose is passed in

	//////// move
	UINT32 moveFlag = TPS_MOVETYPE_STILL;

	if(stateFlags & STATE_DEPLOYED && weapon.deployPivotBone)
	{
		moveFlag = TPS_MOVETYPE_DEPLOYED;
	}
	else if(fpFPSIsMoving(.01f))
	{
		// move
		if(stateFlags & STATE_MOVE_FAST)
		{
			if(poseFlag == TPS_POSETYPE_CROUCH)
			{
				poseFlag = TPS_POSETYPE_STAND;
				moveFlag = TPS_MOVETYPE_FORWARD;
			}
			else
			{
				moveFlag = TPS_MOVETYPE_SPRINT;
			}
		}
		else if(stateFlags & STATE_MOVE_BACKWARD)
		{
			moveFlag = TPS_MOVETYPE_BACKWARD;
		}
		else if(stateFlags & (STATE_MOVE_SLOW|STATE_STRAFE_LEFT|STATE_STRAFE_RIGHT))
		{
			moveFlag = TPS_MOVETYPE_FORWARD;
		}
	}

	//////// point
	UINT32 pointFlag = (poseFlag == TPS_POSETYPE_STAND) ? TPS_POINTTYPE_RELAXED : TPS_POINTTYPE_READY;

	if(poseFlag == TPS_POSETYPE_STAND && weapon.weaponType == FPS_WEAPONTYPE_ROCKET && moveFlag > TPS_MOVETYPE_STILL)
	{
		pointFlag = TPS_POINTTYPE_RELAXED;
	}
	else if((stateFlags & STATE_RELOADING) && _isReloadable(weapon))
	{
		pointFlag = TPS_POINTTYPE_RELOAD;
	}
	else if(weapon.weaponType == FPS_WEAPONTYPE_GRENADE ||
			weapon.weaponType == FPS_WEAPONTYPE_KNIFE ||
			weapon.weaponType == FPS_WEAPONTYPE_SATCHEL ||
			weapon.weaponType == FPS_WEAPONTYPE_BUILD)
	{
		pointFlag = TPS_POINTTYPE_READY;
	}
	else if(weapon.weaponType == FPS_WEAPONTYPE_LTMORTAR ||
			weapon.weaponType == FPS_WEAPONTYPE_ATR)
	{
		pointFlag = TPS_POINTTYPE_RELAXED;
	}
	else if((stateFlags & STATE_AIM) && moveFlag != TPS_MOVETYPE_BACKWARD && moveFlag != TPS_MOVETYPE_DEPLOYED)
	{
		pointFlag = TPS_POINTTYPE_AIM;
	}

	switch(moveFlag)
	{
		case TPS_MOVETYPE_SPRINT:
		{
			pointFlag = TPS_POINTTYPE_RELAXED;
		}
		break;
		case TPS_MOVETYPE_DEPLOYED:
		{
			if(pointFlag != TPS_POINTTYPE_RELOAD)
				pointFlag = TPS_POINTTYPE_READY;
		}
		break;
	}

	if(weapon.weaponType == FPS_WEAPONTYPE_ROCKET)
	{
		if(moveFlag != TPS_MOVETYPE_STILL || pointFlag == TPS_POINTTYPE_READY)
			pointFlag = TPS_POINTTYPE_RELAXED;
	}

	/*
	if(false) // parade
	{
		moveFlag = TPS_MOVETYPE_EMOTE;
		pointFlag = TPS_POINTTYPE_RELAXED;
	}
	else if(false) // canidle
	{
		moveFlag = TPS_MOVETYPE_IDLE;
		pointFlag = TPS_POINTTYPE_RELAXED;
	}
	*/



	if(moveFlag == TPS_MOVETYPE_STILL && (frametime - lastInput) >= idleCheck && !IN_COMBAT(combatCheck))
	{
		if(frametime >= nextIdleCheck)
		{
			// reset our current idle
			currentIdle = -1;

			// we're stationary, lets check for an idle to use
			// the point type could be any valid point type index if we had other idles besides the firs
			SINT32 idleCount = 0;
			fpTPSWeaponType weapType = TPS_WEAPONTYPE_EMPTY;

			if(fpsTrooper.activeWeaponNumber == BINOCS_WEAPON_NUMBER)
				weapType = TPS_WEAPONTYPE_BINOCS;
			else if(fpsTrooper.readingMap)
				weapType = TPS_WEAPONTYPE_MAP;
			else
				weapType = fpTPSConvertWeaponTypes(weapon.weaponType);

			for(int i = 0; i < TPS_POINTTYPE_SHARED; i++)
			{
				if(fpTPSGetBlend(poseFlag, TPS_MOVETYPE_IDLE, weapType, i))
					++idleCount;
			}

			//we have some idles, choose one and save it
			if(idleCount)
			{
				moveFlag	= TPS_MOVETYPE_IDLE;
				pointFlag	= GetRandom(0, idleCount - 1);
				currentIdle = pointFlag;
			}

			nextIdleCheck = frametime + idleCheck;
		}
		else if(currentIdle >= 0)
		{
			moveFlag = TPS_MOVETYPE_IDLE;
			pointFlag = currentIdle;
		}
	}
	else
	{
		currentIdle = -1;
		nextIdleCheck = frametime + idleCheck;
	}

	// handle ppo build
	if(poseFlag == TPS_POSETYPE_CROUCH &&
	   moveFlag == TPS_MOVETYPE_STILL &&
	   weapon.weaponType == FPS_WEAPONTYPE_BUILD)
	{
		PlayerPlacedObject* ppo = fpPPOGet();

		if(ppo && ppo->buildAt && frametime <= ppo->buildAt)
			pointFlag = TPS_POINTTYPE_ACTION1;
	}

	// set the flags
	pFlags = pFlags << gr2AnimPoseBits;
	pFlags |= poseFlag;

	pFlags = pFlags << gr2AnimMoveBits;
	pFlags |= moveFlag;

	pFlags = pFlags << gr2AnimPointBits;
	pFlags |= pointFlag;
}

LOGICAL fpFPSSetAnimationFlags()
{
	mlVehicleData*	vehicleData = persGetPersonaModelDataPtr();
	UINT32			pFlags = 0;

	fpFPSWeaponData& weapon = fpsWeapons[currentWeaponModel];

	// weapon
//	pFlags = pFlags << gr2AnimWeapBits;
	if (stateFlags & STATE_WEAPON_SWITCHING)
	{
		// special case for binocs since they switch away instantly before state changes happen
		if (fpsTrooper.activeWeaponNumber == BINOCS_WEAPON_NUMBER)
			pFlags |= fpsTrooper.nextWeaponNumber;
		else
			pFlags |= fpsTrooper.activeWeaponNumber;
	}
	else
	{
		if (fpsTrooper.readingMap && !(stateFlags & STATE_SWIMMING))
			pFlags |= MAP_WEAPON_NUMBER;
		else
			pFlags |= fpsTrooper.activeWeaponNumber;
	}

	// pose
	if (stateFlags & STATE_SIT)
	{
		_sitFlags(pFlags, weapon);
	}
	else if (stateFlags & STATE_SWIMMING)
	{
		_swimFlags(pFlags, weapon);
	}
	else if (stateFlags & STATE_EYE_PRONE)
	{
		_proneFlags(pFlags, weapon);
	}
	else
	{
		UINT32 poseFlag = TPS_POSETYPE_STAND;

		if(stateFlags & STATE_EYE_CROUCH)
			poseFlag = TPS_POSETYPE_CROUCH;

		_standFlags(pFlags, weapon, poseFlag);
	}

	// head
	pFlags = pFlags << gr2AnimHeadBits;
	if (stateFlags & STATE_LOOK_LEFT)
		pFlags |= 1;

	pFlags = pFlags << gr2AnimHeadBits;
	if (stateFlags & STATE_LOOK_RIGHT)
		pFlags |= 1;

	pFlags = pFlags << gr2AnimDeathBits;
	if (stateFlags & STATE_DEAD)
		pFlags |= 1;

	// emotes
	//pFlags = pFlags << gr2AnimEmoteBits;
	//pFlags |= emoteID;

	// alt weapon model
	if (gr2AnimAltWeapBits)
	{
		pFlags = pFlags << gr2AnimAltWeapBits;

		if (stateFlags & STATE_DEPLOYED)
		{
			if(fpsWeapons[currentWeaponModel].weaponType != FPS_WEAPONTYPE_LTMORTAR &&
			   fpsTrooper.activeWeaponNumber != BINOCS_WEAPON_NUMBER &&
			   fpsTrooper.activeWeaponNumber != MAP_WEAPON_NUMBER &&
			   !fpsTrooper.readingMap)
				pFlags |= 1;
		}
		else if(currentWeaponModel == PIAT)
		{
			//weapWeapon* hlGameWeapon = &(((vehVehicle*)(pVehicleData->VehicleMasterPtr))->Weapons[fpsTrooper.highlightedWeaponNumber][0]);
			if(weapon.clipBullets == 0)
				pFlags |= 1;
		}
	}

	if (gr2AnimParachuteBits)
	{
		bool justJumped = (fpsTrooper.lastUnrideTime && (frametime - fpsTrooper.lastUnrideTime) < 1000);

		pFlags = pFlags << 1;
		if (fpFPSIsTrooperFalling() || fpsTrooper.parachuteOKToOpen)
			pFlags |= 1;

		pFlags = pFlags << 1;
		if (!justJumped && (fpsTrooper.parachuteOpen || fpsTrooper.parachuteOKToOpen))
			pFlags |= 1;

		pFlags = pFlags << 1;
		if (fpsTrooper.isParatroop && !fpsTrooper.parachuteWorn)
			pFlags |= 1;
	}

	// send misc state info
	if (gr2AnimStateBits)
	{
		pFlags = pFlags << 1;
		if(controlStateFlags & FPS_WALK)
			pFlags |= 1;

		pFlags = pFlags << 1;
		if(!IN_COMBAT(120000) || currentFPSstate == LOWER)
			pFlags |= 1;

		pFlags = pFlags << 1;

		if(fpsTrooper.tpsStartAttack)
			pFlags |= 1;

		if(fpsTrooper.tpsAttack)
			vehicleData->MountIsFiring[0] = TRUE;
	}

	vehicleData->PersonData->AnimationFlags = pFlags;

	return TRUE;
}


void fpFPSUpdateSent()
{
	fpsTrooper.tpsStartAttack = 0;

	if(!fpsTrooper.tpsAttack || fpsTrooper.tpsAttack == 1 || frametime > fpsTrooper.tpsAttack)
	{
		mlVehicleData* vehicleData = persGetPersonaModelDataPtr();

		if(vehicleData)
			vehicleData->MountIsFiring[0] = FALSE;

		fpsTrooper.tpsAttack = 0;
	}
}

/*
LOGICAL fpFPSSetAnimationFlags()
{
	mlVehicleData*	vehicleData = persGetPersonaModelDataPtr();
	UINT32			pFlags = 0;
	UINT32			currentFlagValue;

	fpFPSWeaponData& weapon = fpsWeapons[currentWeaponModel];

	// weapon
//	pFlags = pFlags << gr2AnimWeapBits;
	if (stateFlags & STATE_WEAPON_SWITCHING)
	{
		// special case for binocs since they switch away instantly before state changes happen
		if (fpsTrooper.activeWeaponNumber == BINOCS_WEAPON_NUMBER)
			pFlags |= fpsTrooper.nextWeaponNumber;
		else
			pFlags |= fpsTrooper.activeWeaponNumber;
	}
	else
	{
		if (fpsTrooper.readingMap && !(stateFlags & STATE_SWIMMING))
			pFlags |= MAP_WEAPON_NUMBER;
		else
			pFlags |= fpsTrooper.activeWeaponNumber;
	}

	// pose
	if (stateFlags & STATE_SIT)
	{
		currentFlagValue = TPS_POSETYPE_SIT;
	}
	else if (stateFlags & STATE_SWIMMING)
	{
		currentFlagValue = TPS_POSETYPE_SWIM;
	}
	else if (stateFlags & STATE_EYE_CROUCH)
	{
		currentFlagValue = TPS_POSETYPE_CROUCH;
	}
	else if (stateFlags & STATE_EYE_PRONE)
	{
		currentFlagValue = TPS_POSETYPE_PRONE;
	}
	else
	{
		currentFlagValue = TPS_POSETYPE_STAND;
	}

	SINT32 pose = currentFlagValue;

	pFlags = pFlags << gr2AnimPoseBits;
	pFlags |= currentFlagValue;

	// move
	if (stateFlags & STATE_MOVE_FAST)
	{
		currentFlagValue = TPS_MOVETYPE_SPRINT;
	}
	else if (stateFlags & STATE_MOVE_BACKWARD)
	{
		// fixme .. get a backwards swim animation
		if (stateFlags & STATE_SWIMMING)
			currentFlagValue = TPS_MOVETYPE_FORWARD;
		else
			currentFlagValue = TPS_MOVETYPE_BACKWARD;
	}
	else if (stateFlags & (STATE_MOVE_SLOW|STATE_STRAFE_LEFT|STATE_STRAFE_RIGHT))
	{
		// Check for walk/eye-stand while not swimming.
		if ((stateFlags & (STATE_WALK|STATE_EYE_STAND|STATE_SWIMMING)) == (STATE_WALK|STATE_EYE_STAND))
			currentFlagValue = TPS_MOVETYPE_WALK;
		else
			currentFlagValue = TPS_MOVETYPE_FORWARD;
	}
	else if (stateFlags & STATE_DEPLOYED)
	{
		if (weapon.deployPivotBone)
			currentFlagValue = TPS_MOVETYPE_DEPLOYED;
		else if ((weapon.rgFiringRules) && (stateFlags & STATE_EYE_CROUCH))
			currentFlagValue = TPS_MOVETYPE_DEPLOYED;
		else
			currentFlagValue = TPS_MOVETYPE_IDLE;
	}
	else if (stat2Flags & STAT2_LEAN_LEFT)
	{
		currentFlagValue = TPS_MOVETYPE_LEAN_LEFT;
	}
	else if (stat2Flags & STAT2_LEAN_RIGHT)
	{
		currentFlagValue = TPS_MOVETYPE_LEAN_RIGHT;
	}
	else
	{
		currentFlagValue = TPS_MOVETYPE_IDLE;
	}

	SINT32 move = currentFlagValue;

	pFlags = pFlags << gr2AnimMoveBits;
	pFlags |= currentFlagValue;

	// point
	pFlags = pFlags << gr2AnimPointBits;

	if(stateFlags & STATE_IMPLICIT_RELAXED_MASK)
	{
		currentFlagValue = TPS_POINTTYPE_RELAXED;
	}
	else if ((stateFlags & STATE_RELOADING) && _isReloadable(weapon))
	{
		currentFlagValue = TPS_POINTTYPE_RELOAD;
	}
	else if((stateFlags & (STATE_AIM|STATE_EYE_PRONE)) == STATE_AIM && weapon.canAim)
	{
		currentFlagValue = TPS_POINTTYPE_AIM;
	}
	else if(weapon.weaponType == FPS_WEAPONTYPE_GRENADE && (stateFlags & STATE_HOLD_FIRE))
	{
		currentFlagValue = TPS_POINTTYPE_ATTACK;
	}
	else if(weapon.weaponType == FPS_WEAPONTYPE_LTMORTAR && HAS_ALL_BITS(stateFlags, STATE_DEPLOYED|STATE_HOLD_FIRE))
	{
		currentFlagValue = TPS_POINTTYPE_ATTACK;
	}
	else if(weapon.weaponType == FPS_WEAPONTYPE_KNIFE && (stateFlags & STATE_KNIFE_ATTACKING))
	{
		currentFlagValue = TPS_POINTTYPE_ATTACK;
	}
	else if((stateFlags & STATE_DEPLOYED) && fpsWeapons[currentWeaponModel].deployPivotBone)
	{
		currentFlagValue = TPS_POINTTYPE_READY;
	}
	else if(!IN_COMBAT(120000) || currentFPSstate == LOWER)
	{
		currentFlagValue = TPS_POINTTYPE_PARADE;
	}
	else
	{
		currentFlagValue = TPS_POINTTYPE_READY;
	}

	pFlags |= currentFlagValue;

	// head
	pFlags = pFlags << gr2AnimHeadBits;
	if (stateFlags & STATE_LOOK_LEFT)
		pFlags |= 1;

	pFlags = pFlags << gr2AnimHeadBits;
	if (stateFlags & STATE_LOOK_RIGHT)
		pFlags |= 1;

	pFlags = pFlags << gr2AnimDeathBits;
	if (stateFlags & STATE_DEAD)
		pFlags |= (deathState+1);

	// emotes
	pFlags = pFlags << gr2AnimEmoteBits;
	//pFlags |= emoteID;

	// alt weapon model
	if (gr2AnimAltWeapBits)
	{
		pFlags = pFlags << gr2AnimAltWeapBits;
		if (stateFlags & STATE_DEPLOYED)
		{
			if(fpsWeapons[currentWeaponModel].weaponType != FPS_WEAPONTYPE_LTMORTAR &&
			   fpsTrooper.activeWeaponNumber != BINOCS_WEAPON_NUMBER &&
			   fpsTrooper.activeWeaponNumber != MAP_WEAPON_NUMBER &&
			   !fpsTrooper.readingMap)
				pFlags |= 1;
		}
	}

	if (gr2AnimParachuteBits)
	{
		bool justJumped = (fpsTrooper.lastUnrideTime && (frametime - fpsTrooper.lastUnrideTime) < 1000);

		pFlags = pFlags << 1;
		if (fpFPSIsTrooperFalling() || fpsTrooper.parachuteOKToOpen)
			pFlags |= 1;
		pFlags = pFlags << 1;
		if (!justJumped && (fpsTrooper.parachuteOpen || fpsTrooper.parachuteOKToOpen))
			pFlags |= 1;
		pFlags = pFlags << 1;
		if (fpsTrooper.isParatroop && !fpsTrooper.parachuteWorn)
			pFlags |= 1;
	}

	// alt weapon model
	if (gr2AnimStateBits)
	{
		pFlags = pFlags << gr2AnimStateBits;

		if(!IN_COMBAT(120000) || currentFPSstate == LOWER)
			pFlags |= 1;
	}

	vehicleData->PersonData->AnimationFlags = pFlags;

	return TRUE;
}
*/

// kill the trooper
void fpFPSKillTrooper ()
{
	fpPersonData* personData = fpGetFPPersonDataPtr();
	if(!personData) return;

    showArms = false;
	showGun = false;
	showClip = false;

    heightAtDeath = fpFPSGetHeight();

//	fpFPSSit (0);

	personData->Flags |= fpFLAGS_NO_MOVEMENT;

	deathState = 1; //rand() % TPS_NUM_INF_DEATHS;

	stateFlags = STATE_DEFAULT_MASK|STATE_DEAD|STATE_NO_MOVEMENT;
	stat2Flags = 0;
	controlStateFlags = 0;

	// let the tutorial layer know we've been killed
	TUT_MGR->DoEvent(TUT_EVENT_DIED);

	// lets watch ourself die
	persSetDeathCam(2);
}

// kill the trooper SYSTEM
void fpFPSKill ()
{
	UINT32 i;

	currentFPSstate = IDLE;

	fpFPSSit (0);

    _startDeathTimer = 0;

	stateFlags = STATE_DEFAULT_MASK;
	stat2Flags = 0;
	controlStateFlags = 0;

	fpFPSCleanupAnimationState ();

	for (i=0; i < NUM_WEAPONS; i++)
	{
		UINT32 j;

		if (weapDefaultAnim[i])
		{
			gr2FreeAnimation (weapDefaultAnim[i]);
			weapDefaultAnim[i] = nullptr;
		}

		if (weapEmptyDefaultAnim[i])
		{
			gr2FreeAnimation (weapEmptyDefaultAnim[i]);
			weapEmptyDefaultAnim[i] = nullptr;
		}

		if (clipDefaultAnim[i])
		{
			gr2FreeAnimation (clipDefaultAnim[i]);
			clipDefaultAnim[i] = nullptr;
		}

		for (j=0; j < NUM_WEAP_STATES; j++)
		{
			if (armsAnims[i][j])
				gr2FreeAnimation (armsAnims[i][j]);

			if (weapAnims[i][j])
				gr2FreeAnimation (weapAnims[i][j]);

			if (clipAnims[i][j])
				gr2FreeAnimation (clipAnims[i][j]);

			armsAnims[i][j] = nullptr;
			weapAnims[i][j] = nullptr;
			clipAnims[i][j] = nullptr;
		}
	}

	gr2FreeModel (skeletonModel);
	skeletonModel = nullptr;

	for (i=0; i < NUM_INFANTRY_TYPES; i++)
	{
		if (armsModels[i])
		{
			gr2FreeModel (armsModels[i]);
			armsModels[i] = nullptr;
		}

		if (legsModels[i])
		{
			gr2FreeModel (legsModels[i]);
			legsModels[i] = nullptr;
		}
	}

	if (armTexModel)
	{
		gr2FreeModel (armTexModel);
		armTexModel = nullptr;
	}

	for (i=0; i < NUM_WEAPONS; i++)
	{
		if (weaponModels[i])
		{
			gr2FreeModel (weaponModels[i]);
			weaponModels[i] = nullptr;
		}

		if (secondaryModels[i])
		{
			gr2FreeModel (secondaryModels[i]);
			secondaryModels[i] = nullptr;
		}

		for(int j = 0; j < FPS_NUM_AUX_MODELS; j++)
		{
			if(auxModels[i][j])
			{
				gr2FreeModel (auxModels[i][j]);
				auxModels[i][j] = nullptr;
			}
		}
	}

	if (fpsTrooper.movementSoundID != 0)
	{
		fxSndStopSound(fpsTrooper.movementSoundID);
		fpsTrooper.movementSoundID = 0;
	}

	if (fpsTrooper.autoWeaponFireSoundId != 0)
	{
		fxSndStopSound(fpsTrooper.autoWeaponFireSoundId);
		fpsTrooper.autoWeaponFireSoundId = 0;
	}

	currentGameWeapon = nullptr;

	fpsTrooper.initialized = FALSE;
}

LOGICAL fpFPSGetHUDAmmoData (HUD_AMMO_DATA type, SINT32* data)
{
	// set default hud ammo data value;
	*data = -1;

	if (!currentGameWeapon)
		return FALSE;

	mlVehicleData* pVehicleData = persGetPersonaModelDataPtr();
	if (!pVehicleData)
		return FALSE;

	fpsWeaponTypes hlWeaponType = fpsTrooper.weaponPositionModel[fpsTrooper.highlightedWeaponNumber];

	if (hlWeaponType == BINOCS)
		return TRUE;

	weapWeapon* hlGameWeapon = &(((vehVehicle*)(pVehicleData->VehicleMasterPtr))->Weapons[fpsTrooper.highlightedWeaponNumber][0]);

	if (!hlGameWeapon)
		return TRUE;

	SINT32 ammoTypeNum = fpFPSGetAmmoTypeNumByWeapon((SINT32)hlWeaponType, hlGameWeapon);

	if(fpsWeapons[hlWeaponType].auxEnable[FPS_AUX_RGCUP])
		ammoTypeNum = 0;

	switch(type)
	{
		case HUD_AMMO_DATA_RANGE:
		{
			if(fpsWeapons[hlWeaponType].numSightDists > 0)
				*data = fpsWeapons[hlWeaponType].getSightDist();
		}
		break;
		case HUD_AMMO_DATA_CLIPTYPE:
		{
			*data = (hlGameWeapon->WeaponType) ? hlGameWeapon->WeaponType->ClipType: 0;
		}
		break;
		case HUD_AMMO_DATA_RGAMMO:
		{
			// number of rg rounds left if the cup is attached
			if(fpsWeapons[hlWeaponType].auxAvailable[FPS_AUX_RGCUP])
				*data = hlGameWeapon->NumRoundsAmmoLeft[1];
		}
		break;
		case HUD_AMMO_DATA_RGLOADED:
		{
			// number of rg rounds left if the cup is attached
			if(fpsWeapons[hlWeaponType].auxEnable[FPS_AUX_RGCUP])
			{
				if(fpsWeapons[hlWeaponType].auxEnable[FPS_AUX_RGGRENADE])
					*data = 1;
				else
					*data = 0;
			}
		}
		break;
		case HUD_AMMO_DATA_CLIPAMMO:
		{
			// number of rounds left in the clip
			*data = fpsWeapons[hlWeaponType].clipBullets;
		}
		break;
		case HUD_AMMO_DATA_CLIPCOUNT:
		{
			UINT32 clipSize	= (hlGameWeapon->WeaponType) ? hlGameWeapon->WeaponType->NumRoundsInClip: 0;

			// number of rounds or clips left
			// if clipsize > 1 its the number of clips
			// otherwise its the number of rounds
			if(clipSize > 1)
			{
				// clipped weapons
				SINT16 supply = hlGameWeapon->NumRoundsAmmoLeft[ammoTypeNum];

				if(supply >= 0)
				{
					if(hlGameWeapon->NumOverSupplyRounds[ammoTypeNum] > 0)
						*data = supply / clipSize;
					else
						*data = (supply - fpsWeapons[hlWeaponType].clipBullets + (clipSize - 1)) / clipSize;
				}
			}
			else if(clipSize == 1)
			{
				// single round ammo (grenades, satchels, etc...)
				*data = hlGameWeapon->NumRoundsAmmoLeft[ammoTypeNum];
			}
		}
		break;
		case HUD_AMMO_DATA_CLIPSIZE:
		{
			// the number of round an individual clip holds
			*data = (hlGameWeapon->WeaponType) ? hlGameWeapon->WeaponType->NumRoundsInClip: 0;
		}
		break;
		case HUD_AMMO_DATA_OVERSUPPLY:
		{
			*data = hlGameWeapon->NumOverSupplyRounds[ammoTypeNum];

			UINT32 clipSize	= (hlGameWeapon->WeaponType) ? hlGameWeapon->WeaponType->NumRoundsInClip: 0;

			// if we're not single rounds, but clips
			if(*data && clipSize > 1)
			{
				// add the clipammo back into the oversupply pool to give the illusion that the
				// oversupply pool only drains when a reload happens
				*data += (clipSize - fpsWeapons[hlWeaponType].clipBullets);

				// now turn the number into the number of clips
				*data = (SINT32)(*data / clipSize);
			}
		}
		break;
		default:
			return FALSE; // bad hud ammo data type
		break;
	}

	/*
	if(oversupply)
	{
		*oversupply = highlightedGameWeapon->NumOverSupplyRounds[ammoTypeNum] + (highlightedGameWeapon->WeaponType->NumRoundsInClip - fpsWeapons[highlightedWeaponType].clipBullets);
		*oversupply = (SINT32)(*oversupply / highlightedGameWeapon->WeaponType->NumRoundsInClip);
	}

	// FIXME - needs to be smarter at some point
	// currently only affects rifle grenades, who's clips are of size 1
	if (ammoTypeNum != 0 ||	highlightedGameWeapon->WeaponType->NumRoundsInClip == 0)
	{
		*bulletsInClip = fpsWeapons[highlightedWeaponType].clipBullets;
		*clipCount = highlightedGameWeapon->NumRoundsAmmoLeft[ammoTypeNum] - *bulletsInClip;
	}
	else if (highlightedGameWeapon->NumRoundsAmmoLeft[ammoTypeNum] < 0)
	{
		*bulletsInClip = -1;
		*clipCount = -1;
	}
	else
	{
		fpPersonData*	pPersonData;
		pPersonData = (fpPersonData*) pVehicleData->PersonData;

		if (pPersonData->Expendable[fpsTrooper.highlightedWeaponNumber])
		{
			*bulletsInClip = highlightedGameWeapon->NumRoundsAmmoLeft[ammoTypeNum];
			*clipCount = -1;
		}
		else
		{
			SINT32 at = ammoTypeNum;
			SINT16 al = highlightedGameWeapon->NumRoundsAmmoLeft[at];
			UINT32 nric = highlightedGameWeapon->WeaponType->NumRoundsInClip;

			*bulletsInClip = fpsWeapons[highlightedWeaponType].clipBullets;
			*clipCount = (al - *bulletsInClip + (nric - 1)) / nric;
		}
	}
	*/

	return TRUE;
}

LOGICAL fpFPSRideUnrideVehicle ()
{
	mlVehicleData*	vehicleData = persGetPersonaModelDataPtr();
	if (vehicleData == nullptr)
	{
		return FALSE;
	}
	if (vehicleData->VehicleMasterPtr == nullptr)
	{
		return FALSE;
	}

	vehVehicle*		vehicle = (vehVehicle*)(vehicleData->VehicleMasterPtr);;
	vehVehicle*		FPSRideVehicle = nullptr;
	FLOAT64			speed;
	FLOAT32			damage;

	vehVehicle*currentVehicle = nullptr;

#if 0
	if (stateFlags & STATE_SIT)
		fpFPSSit (0);
	else
		fpFPSSit (1);
	return TRUE;
#endif

	if(vehicle->RideVehicleHostIndex > 0)
	{
		FPSRideVehicle = vehFindVehicle(vehicle->RideVehicleHostIndex);

		// check for vehicle poofing on us .. if so, act like we jumped off
		if(FPSRideVehicle == nullptr)
		{
			FLOAT64VECTOR& pos = vehicle->ModelData->VehiclePos;
			const FLOAT32 rayLength = 10.0f;
			FLOAT32 hit = collideRay(COLLIDER_TILE, rayLength, pos.x, pos.y, pos.z);

			if (fpsTrooper.parachuteWorn && !hit)
			{
				fpsTrooper.parachuteOKToOpen = TRUE;
				fpFPSParadropStart();
			}

			vehicle->RideVehicleHostIndex = 0;
			fpFPSSit(0);

            // Set timer in case mobile spawn became a UMS
            fpsTrooper.lastUnrideTime = frametime;

			return TRUE;
		}
	}

	// if we're riding a vehicle, jump off
	if (FPSRideVehicle != nullptr && vehicle->RideSeatNum != VEH_TROOPER_ON_DECK_RIDE_SEAT)
	{
		// special handling for airplanes
		if (FPSRideVehicle->VehicleType->Type == VEH_TYPE_AIR)
		{
			FLOAT64VECTOR& pos = vehicle->ModelData->VehiclePos;
			const FLOAT32 rayLength = 10.0f;
			FLOAT32 hit = collideRay(COLLIDER_TILE, rayLength, pos.x, pos.y, pos.z);

			// if we're getting ejected, we're screwed?  test this..
//			if ((fpsTrooper.parachuteWorn) && (FPSRideVehicle->StateAnimation.EjectRiders != 1))
			if (fpsTrooper.parachuteWorn && !hit)
			{
				// only allow within 30 deg roll and pitch
				if(fabs(FPSRideVehicle->PosData.Att.Pitch) > DEG_TO_RAD(30.0)
				|| fabs(FPSRideVehicle->PosData.Att.Roll) > DEG_TO_RAD(30.0))
				{
					return FALSE;
				}

				// ignore max speed for planes for paratroops
				// little bit of a loophole, they can bug out while the plane is taxiing to quickly and so on
				fpsTrooper.parachuteOKToOpen = TRUE;
				fpFPSParadropStart();
				speed = 0.0;
				vehicleData->VehicleAtt.Yaw = -FPSRideVehicle->PosData.Att.Yaw + PI;
			}
			else
				speed = mathExactVectorLength(&FPSRideVehicle->PosData.Vel);
		}
		else
			speed = mathExactVectorLength(&FPSRideVehicle->PosData.Vel);

		// prevent non paratroops from bailing from a moving transport plane
		if (FPSRideVehicle->VehicleType->Type == VEH_TYPE_AIR)
		{
			// fixme - expose this as maxNonParatroopDisembarkPlaneSpeed or something
			// make sure we're not getting ejected (if we are, we go anyway)
			if ((speed > 0.5) && (FPSRideVehicle->StateAnimation.EjectRiders != 1))
			{
				uiChatPrintLine (uiLocString(IDE_LANG_UI, 700).c_str());
				return FALSE;
			}
		}

		if (speed < fpsTrooper.maxSafeVehicleSpeed)
		{
			vehicle->RideVehicleHostIndex = 0;
			fpFPSSit(0, FPSRideVehicle);
		}
		else
		{
			// too fast to unload from vehicle safely, damage rider
			speed = mathExactVectorLength(&FPSRideVehicle->PosData.LastLastUpdateVel);

			// sanity check
			// 30.0 m/s is about 110kph
			if (speed > 30.0)
			{
				Write_Log (LOG_FPS, LOG_INFO, "Absurd speed calculated on j:  %f %f %f", speed,
					FPSRideVehicle->PosData.Vel,
					FPSRideVehicle->PosData.LastLastUpdateVel);
			}
			else
			{
				vehicle->RideVehicleHostIndex = 0;

				fpFPSSit(0, FPSRideVehicle);

				damage = (FLOAT32)((speed - fpsTrooper.maxSafeVehicleSpeed) /
							(fpsTrooper.killVehicleSpeed - fpsTrooper.maxSafeVehicleSpeed));

				// No this is not a typo. The damage goes as the square of the velocity.
				damage *= damage * fpsTrooper.unrideJoules;

				vehVehicle*personaVehicle = (vehVehicle*)(vehicleData->VehicleMasterPtr);
				vehApplyDamageToVehicleComponent(personaVehicle,
												 4,
												 damage,
												 0);

				fpFPSPlayFallSound(vehicleData->VehicleObjectData->PlaneObject, FPS_FALL_NORMAL_HURT);
			}
		}

        // Don't want to track unride time on ships otherwise we won't collide with them when boarding!
        if (FPSRideVehicle->VehicleType->Type != VEH_TYPE_SEA)
            fpsTrooper.lastUnrideTime = frametime;
        else
            fpsTrooper.lastUnrideTime = 0;
        
		return TRUE;
	}

	// don't allow boarding of vehicles while falling

	if (vehicleData->PersonData->Flags & fpFLAGS_FALLING)
		return FALSE;

		// dont' allow boarding while deployed or deploying
	if (HAS_ANY_BITS(stateFlags, STATE_DEPLOYING|STATE_RELOADING|STATE_NEEDS_RECYCLE) || (stat2Flags & STAT2_NEEDS_RELOAD))
		return FALSE;

	if ((stateFlags & STATE_DEPLOYED) && (fpsWeapons[currentWeaponModel].staticDeploy))
		return FALSE;

	// start at the top of the vehicle list and find a vehicle to play with
	// scan through vehicle list to find someone within range
	// 2 conditions:  FPS must be within radii of the vehicle, and
	//   target vehicle cannot be moving faster than 1m/s when boarding

	vehGetVehicleHead(&currentVehicle);
	FPSRideVehicle = nullptr;

    for(;currentVehicle; currentVehicle = currentVehicle->getNext())
	{
		UINT32 i;

		if (currentVehicle == vehicle) continue;
		if (currentVehicle->VehicleObjectData == nullptr) continue;
		if (currentVehicle->VehicleObjectData->NumFPSSeats == 0) continue;
		if (currentVehicle->Side != vehicle->Side) continue;
        if (currentVehicle->isSTO()) continue;

		speed = mathExactVectorLength(&currentVehicle->PosData.Vel);
		if (speed > fpsTrooper.maxSafeVehicleSpeed)
			continue;

		for(i = 0; i < currentVehicle->VehicleObjectData->NumFPSSeats; i++)
		{
			OBJECT* RideObject = currentVehicle->VehicleObjectData->FPSSeats[i];
			if(RideObject == nullptr) break;
			FLOAT64VECTOR TrooperPos = {0,0,0}, RidePos, DeltaVec;
			FLOAT64VECTOR WorldOriginOffset;

			trnGetWorldOriginOffset(&WorldOriginOffset);
			mathVectorMultiply64(&TrooperPos, &TrooperPos, &vehicleData->VehicleAttMat);
			mathAddVectors(&TrooperPos, &TrooperPos, &vehicleData->AdjustedForCGPos);

			RidePos.x = RideObject->dof->dofmatrix2._41;
			RidePos.y = RideObject->dof->dofmatrix2._42;
			RidePos.z = RideObject->dof->dofmatrix2._43;

			mathVectorMultiply64(&RidePos, &RidePos, &RideObject->mat);

			RidePos.x += WorldOriginOffset.x;
			RidePos.y += WorldOriginOffset.y;
			RidePos.z = -(RidePos.z + WorldOriginOffset.z);
			mathSubVectors(&DeltaVec, &RidePos, &TrooperPos);

			if(mathExactVectorLength2(&DeltaVec) < MIN_RIDE_DISTANCE2)
			{
				FPSRideVehicle = currentVehicle;
				vehicle->RideVehicleHostIndex = FPSRideVehicle->VehicleHostIndex;
				vehicle->RideSeatNum = i;

				break;
			}
		}
	}

	// need to check if seat is occupied, if so, then pick another one
	if(FPSRideVehicle && FPSRideVehicle->VehicleType->Type != VEH_TYPE_SEA)
	{
		LOGICAL IsSeatOccupied[ML_MAX_FPS_SEATS];
		memset(IsSeatOccupied, 0x0, sizeof(IsSeatOccupied));

		vehGetVehicleHead(&currentVehicle);
		for(;currentVehicle; currentVehicle = currentVehicle->getNext())
		{
			if(currentVehicle->RideVehicleHostIndex == vehicle->RideVehicleHostIndex &&
				currentVehicle->VehicleType->Type == VEH_TYPE_PERSON &&
				currentVehicle != vehicle) // not me
			{
				if(currentVehicle->RideSeatNum < ML_MAX_FPS_SEATS)
					IsSeatOccupied[currentVehicle->RideSeatNum] = TRUE;
			}
		}

		if(IsSeatOccupied[vehicle->RideSeatNum])
		{
			for(int i = 0; i < (int)FPSRideVehicle->VehicleObjectData->NumFPSSeats; i++)
			{
				if(!IsSeatOccupied[i])
				{
					vehicle->RideSeatNum = i;
					break;
				}
			}
		}
	}

	if (vehicle->RideVehicleHostIndex > 0 && vehicle->RideSeatNum != VEH_TROOPER_ON_DECK_RIDE_SEAT)
	{
		fpsTrooper.parachuteOKToOpen = FALSE;
		fpFPSSit(1);
	}

	return TRUE;
}

SINT32 fpFPSGetLastUnrideTime()
{
	return fpsTrooper.lastUnrideTime;
}

// do NOT call fpFPSSit (1) from anywhere outside of the scope of fpFPSHandleControls
// calling fpFPSSit (0) is safe from anywhere though
LOGICAL fpFPSSit(SINT32 Param, vehVehicle* rideVehicle)
{
	snapView = true;

	if (Param)
	{

		if (currentWeaponModel == BINOCS)
		{
			// no binocs while riding
			fpFPSWeaponAutoSwitch();
		}

		stateFlags &=
				~(STATE_NEEDS_RECYCLE
				|STATE_HOLD_FIRE
				|STATE_FIRE_DELAYED_WEAP
				|STATE_SWIMMING
				|STATE_DEPLOYING
//				|STATE_WEAPON_SWITCHING
//				|STATE_DEPLOYED
				);

//		ogs - we're about to set stat2flags = 0, so this is redundant
//		stat2Flags &= ~STAT2_NEEDS_RELOAD;

		currentGameWeapon->ReloadStartTime = 0;
		showClip = true;
		showGun = true;

		stateFlags |= STATE_NO_MOVEMENT | STATE_SIT;
		stat2Flags = 0;

		// clear the falling flag (ok, so this lets people jump off of high buildings onto trucks)
		fpPersonData* personData = fpGetFPPersonDataPtr();
		personData->Flags &= ~fpFLAGS_FALLING;

		// turn off all of the controls that aren't allowed when riding
		controlStateFlags &= ~RIDE_MASK;

		// make sure the legs are straight and level by setting this to 0 then moving the model
		aimVertAngle = 0.0;
		aimVertAngleOffset = 0.0;
		targetPose = FPS_POSITION_SITTING;

		// since anything to sit is instant, we can pass in enough time to finish the eye move in one step
		fpFPSAdjustEyeToPose(0);
		fpFPSMoveModel();

		// attach legs
		legsModel = legsModels[currentArms];

		// this is added to fix a sit until we have all the set states. DEG
        // showGun = false;
		// end of temp fix DEG.

		if (!gr2AttachModel (legsModel, "Bip01", skeletonModel, "Bip01"))
		{
			Write_Log (LOG_FPS, LOG_INFO, "Failed to attach legs to skeleton");
			Write_Log (LOG_FPS, LOG_INFO, "0x%x 0x%x 0x%x .. legs = 0x%x", legsModels[0], legsModels[1], legsModels[2], legsModel);
			return FALSE;
		}

		if (!fpFPSCleanupAnimationState ())
			return FALSE;

		if (!fpFPSStartAnimationState (currentWeaponModel, SIT))
			return FALSE;

		currentFPSstate = SIT;
	}
	else
	{
		// move the player to the ride position

		stateFlags &= ~(STATE_NO_MOVEMENT|STATE_SIT);

		if (legsModel)
			gr2DetachModel (legsModel);
		legsModel = nullptr;

		targetPose = FPS_POSITION_STAND;
		// this is added to fix a sit until we have all the set states. DEG
        // showGun = true;
		// end of temp fix DEG.

		vehVehicle* player = persGetPersonaVehiclePtr();

		if(rideVehicle && player)
		{
			FLOAT64VECTOR offset = {0,0,0};

			// choose the best position to put the player
			switch(rideVehicle->VehicleType->Type)
			{
				case VEH_TYPE_AIR:
				{
					OBJECT* exit = nullptr;

					offset.x = -2.0f;
					offset.y = -4.0f;

					for(size_t i = 0; i < rideVehicle->VehicleObjectData->NumFPSSeats; i++)
					{
						OBJECT* seat = rideVehicle->VehicleObjectData->FPSSeats[i];
						if(seat == nullptr) continue;

						if(!exit || (exit->dof->dofmatrix2._41 <= 0 && seat->dof->dofmatrix2._42 < exit->dof->dofmatrix2._42))
							exit = seat;
					}

					if(exit)
					{
						offset.x += exit->dof->dofmatrix2._41;
						offset.y += exit->dof->dofmatrix2._42;
						offset.z += exit->dof->dofmatrix2._43;
					}
				}
				break;
				case VEH_TYPE_GROUND:
				{
					if(player->RideSeatNum < rideVehicle->VehicleObjectData->NumFPSSeats)
					{
						OBJECT* seat = rideVehicle->VehicleObjectData->FPSSeats[player->RideSeatNum];

						if(seat)
						{
							offset.x += seat->dof->dofmatrix2._41;
							offset.y += seat->dof->dofmatrix2._42;
							offset.z += seat->dof->dofmatrix2._43;

							if(seat->dof->dofmatrix2._41 < 0)
								offset.x += -1.0f;
							else
								offset.x += 1.0f;
						}
					}
				}
				break;
				case VEH_TYPE_SEA:
				{
                    offset.x = player->ModelData->VehiclePos.x - rideVehicle->PosData.Pos.x;
                    offset.y = player->ModelData->VehiclePos.y - rideVehicle->PosData.Pos.y;
					offset.z = player->ModelData->VehiclePos.z - rideVehicle->PosData.Pos.z;
				}
				break;
			}

			if(player)
			{
                mathVectorMultiply64(&offset, &offset, &rideVehicle->PosData.AttMat);

				player->ModelData->VehiclePos.x = rideVehicle->PosData.Pos.x + offset.x;
				player->ModelData->VehiclePos.y = rideVehicle->PosData.Pos.y + offset.y;
				player->ModelData->VehiclePos.z = rideVehicle->PosData.Pos.z + offset.z;
				player->ModelData->RenderPos	= player->ModelData->VehiclePos;

				// make us face down the vehicle's attitude
				player->ModelData->VehicleAtt.Yaw = -rideVehicle->PosData.Att.Yaw + PI;
			}
		}

		// since anything to sit is instant, we can pass in enough time to finish the eye move in one step
#if(0)
        fpFPSAdjustEyeToPose(0);
		fpFPSMoveModel();
#endif
	}

	return TRUE;
}

// this happens long after process inputs and the eye point calc, so it returns just the current frame's state
LOGICAL fpFPSGetATPGain(FLOAT32* pATPGain, FLOAT32 deltaTime)
{
	if (!pATPGain)
		return FALSE;

	if (deltaTime <= 0)
		return FALSE;

	FLOAT32			currentSpeed;
	FLOAT32			currentATP;
	FLOAT32			baseSpeed;
	fpsSpeedTypes	forwardSpeedValue;

	if (stateFlags & STATE_WALK)
		forwardSpeedValue = FPS_SPEED_WALK;
	else
		forwardSpeedValue = FPS_SPEED_TROT;

	persGetHealthStats(nullptr, &currentATP, nullptr, nullptr);

	currentSpeed = currentForwardVectorLength * currentForwardVectorLength +
		currentStrafeVectorLength * currentStrafeVectorLength;

	FLOAT32 recoverMultiplier = (persIsFatigued()) ? 1.0f : 12.0f;

	if (stateFlags & STATE_SIT)
	{
		*pATPGain = fpsTrooper.atpSitStill;
	}
	else if (stateFlags & STATE_SWIMMING)
	{
		if (currentSpeed == 0.0)
			*pATPGain = fpsTrooper.atpSwimStill;
		else
		{
			// if we're less than minimum, set to 0.0
			if (currentATP < fpsTrooper.atpSwimMinimum)
			{
				*pATPGain = 0.0;
			}
			else
			{
				baseSpeed = fpsTrooper.movementSpeeds[FPS_SPEED_SWIM];
				*pATPGain = fpsTrooper.atpSwimForward * (currentSpeed /  (baseSpeed * baseSpeed));
			}
		}
	}
	else if (stateFlags & STATE_EYE_PRONE)
	{
		if (currentSpeed == 0.0)
			*pATPGain = fpsTrooper.atpProneStill * recoverMultiplier;
		else
		{
			// if we're less than minimum, set to 0.0
			if (currentATP < fpsTrooper.atpCrawlMinimum)
			{
				*pATPGain = 0.0;
			}
			else
			{
				baseSpeed = fpsTrooper.movementSpeeds[FPS_SPEED_CRAWL];
				*pATPGain = fpsTrooper.atpProneForward * (currentSpeed /  (baseSpeed * baseSpeed));
			}
		}
	}
	else if (stateFlags & STATE_EYE_CROUCH)
	{
		if (stateFlags & STATE_RELAXED)
		{
			if (currentSpeed == 0.0)
				*pATPGain = fpsTrooper.atpCrouchStillRelaxed * recoverMultiplier;
			else
			{
				// if we're less than minimum, set to 0.0
				if (currentATP < fpsTrooper.atpCreepMinimum)
				{
					*pATPGain = 0.0;
				}
				else
				{
					baseSpeed = fpsTrooper.movementSpeeds[FPS_SPEED_QUICK];
					*pATPGain = fpsTrooper.atpCrouchForwardRelaxed * (currentSpeed /  (baseSpeed * baseSpeed));
				}
			}
		}
		else
		{
			if (currentSpeed == 0.0)
				*pATPGain = fpsTrooper.atpCrouchStill * recoverMultiplier;
			else
			{
				// if we're less than minimum, set to 0.0
				if (currentATP < fpsTrooper.atpCreepMinimum)
				{
					*pATPGain = 0.0;
				}
				else
				{
					baseSpeed = fpsTrooper.movementSpeeds[FPS_SPEED_QUICK];
					*pATPGain = fpsTrooper.atpCrouchForward * (currentSpeed /  (baseSpeed * baseSpeed));
				}
			}
		}

	}
	else if (stateFlags & STATE_EYE_STAND)
	{
		if (stateFlags & STATE_RELAXED)
		{
			// instead of trusting state flags, use the velocities to determine what state I'm really in
			if (currentSpeed == 0.0)
			{
				*pATPGain = fpsTrooper.atpStandStillRelaxed * recoverMultiplier;
			}
			else if (currentForwardVectorLength < 0)
			{
				// if we're less than minimum, set to 0.0
				if (currentATP < fpsTrooper.atpBackwardMinimum)
				{
					*pATPGain = 0.0;
				}
				else
				{
					baseSpeed = fpsTrooper.movementSpeeds[forwardSpeedValue];
					*pATPGain = fpsTrooper.atpStandBackwardRelaxed * (currentSpeed /  (baseSpeed * baseSpeed));
				}
			}
			// between 0 and walk speed
			// fudge because I'm paranoid about fp math multiplies
			else if ((currentSpeed - 0.00001) <= fpsTrooper.movementSpeeds[FPS_SPEED_WALK] * fpsTrooper.movementSpeeds[FPS_SPEED_WALK])
			{
				// for simplicity, just give them walk if they're moving here
				*pATPGain = fpsTrooper.atpStandForwardWalkRelaxed;
			}
			// between walk and trot speed
			// fudge because I'm paranoid about fp math multiplies
			else if ((currentSpeed - 0.00001) <= fpsTrooper.movementSpeeds[FPS_SPEED_TROT] * fpsTrooper.movementSpeeds[FPS_SPEED_TROT])
			{
				// for simplicity, just give them trot if they're moving here
				*pATPGain = fpsTrooper.atpStandForwardRelaxed;
			}
			// we're sprinting
			else
			{
				// for simplicity, just give them sprint if they're moving here
				*pATPGain = fpsTrooper.atpStandSprint;// * recoverMultiplier/3.0f;
			}
		}
		else
		{
			// instead of trusting state flags, use the velocities to determine what state I'm really in
			if (currentSpeed == 0.0)
			{
				*pATPGain = fpsTrooper.atpStandStill * recoverMultiplier;
			}
			else if (currentForwardVectorLength < 0)
			{
				// if we're less than minimum, set to 0.0
				if (currentATP < fpsTrooper.atpBackwardMinimum)
				{
					*pATPGain = 0.0;
				}
				else
				{
					baseSpeed = fpsTrooper.movementSpeeds[forwardSpeedValue];
					*pATPGain = fpsTrooper.atpStandBackward * (currentSpeed /  (baseSpeed * baseSpeed));
				}
			}
			// between 0 and walk speed
			// fudge because I'm paranoid about fp math multiplies
			else if ((currentSpeed - 0.00001) <= fpsTrooper.movementSpeeds[FPS_SPEED_WALK] * fpsTrooper.movementSpeeds[FPS_SPEED_WALK])
			{
				// for simplicity, just give them walk if they're moving here
				*pATPGain = fpsTrooper.atpStandForwardWalk;
			}
			// between walk and trot speed
			// fudge because I'm paranoid about fp math multiplies
			else if ((currentSpeed - 0.00001) <= fpsTrooper.movementSpeeds[FPS_SPEED_TROT] * fpsTrooper.movementSpeeds[FPS_SPEED_TROT])
			{
				// for simplicity, just give them trot if they're moving here
				*pATPGain = fpsTrooper.atpStandForward;
			}
			// we're sprinting
			else
			{
				// for simplicity, just give them sprint if they're moving here
				*pATPGain = fpsTrooper.atpStandSprint;// * recoverMultiplier/3.0f;
			}
		}
	}

	// loadout effects
	if (*pATPGain < 0)
	{
		if (fpsTrooper.parachuteWorn)
			*pATPGain = *pATPGain * fpsTrooper.parachuteWornATPMultiplier;
		else
			*pATPGain = *pATPGain * loadoutATPMultiplier;
	}

	*pATPGain += (fpFPSBulletATPEffect/deltaTime);
	fpFPSBulletATPEffect = 0;

	*pATPGain += poseChangeATPEffect;
	poseChangeATPEffect = 0;

	return TRUE;
}

void fpFPSStartSwimming (mlVehicleData* pVehicleData, LOGICAL swimming)
{
	if ((swimming) && (!(stateFlags & STATE_SIT)))
	{
		if (!fpsTrooper.parachuteOpen)
		{
			stateFlags |= STATE_SWIMMING;
			pVehicleData->VehiclePos.z -= fpsTrooper.swimDisplacement;
		}
	}
	else if (!(swimming) && (stateFlags & STATE_SWIMMING))
	{
		// we're not swimming anymore
		stateFlags &= ~STATE_SWIMMING;
	}

	if(fpsTrooper.riverExit && !(stateFlags & STATE_SWIMMING) && fpFPSGetPlayerPose() == POSE_PRONE)
	{
		controlStateFlags &= ~(FPS_CROUCH|FPS_PRONE);
		stateFlags |= STATE_EYE_STAND;
		targetPose = FPS_POSITION_STAND;
		fpFPSAdjustEyeToPose (0.0);
		fpFPSMoveModel();
	}
}

LOGICAL fpFPSIsReloading()
{
	return (stateFlags & STATE_RELOADING);
}

LOGICAL fpFPSIsSwimming()
{
	if (fpsTrooper.parachuteOpen)
		return FALSE;

	return ((stateFlags & (STATE_SWIMMING | STATE_SIT)) == STATE_SWIMMING);
}

LOGICAL fpFPSIsWading()
{
	return (fpsTrooper.wading) ? TRUE : FALSE;
}

LOGICAL fpFPSIsSilent()
{
	// we're silent if we're still and not strafing
	return ((stateFlags & (STATE_MOVE_STILL | STATE_STRAFE_MASK)) == STATE_MOVE_STILL);
}

LOGICAL fpFPSIsProne()
{
	return (stateFlags & STATE_EYE_PRONE);
}

LOGICAL fpFPSIsCrouch()
{
	return (stateFlags & STATE_EYE_CROUCH);
}

LOGICAL fpFPSIsAim()
{
	return (stateFlags & STATE_AIM);
}

LOGICAL fpFPSIsFast()
{
	return (stateFlags & STATE_MOVE_FAST);
}

LOGICAL fpFPSIsWalking()
{
	return (stateFlags & STATE_WALK);
}

bool fpFPSIsDead()
{
	return ((stateFlags & STATE_DEAD) != 0);
}

bool fpFPSIsNoFireZone()
{
	return noFire;
}

LOGICAL fpFPSIsCaptureZone(bool deathCheck)
{
	if((currentCollisionFlags & COLLIDER_CAPTURE) && (!deathCheck || !fpFPSIsDead()))
		return !vehmainWaitingOnLogoutTimer();

	return FALSE;
}

LOGICAL fpFPSIsMoving(float threshold)
{
	FLOAT64VECTOR velocity;
	fpFPSGetVelocity(&velocity);

	return (velocity.x * velocity.x +
			velocity.y * velocity.y +
			velocity.z * velocity.z) > threshold*threshold;
}

int fpFPSIsLeaning()
{
	if(stat2Flags & STAT2_LEAN_LEFT)
		return -1;
	else if(stat2Flags & STAT2_LEAN_RIGHT)
		return 1;

	return 0;
}

LOGICAL fpFPSIsBinocs()
{
	return (currentWeaponModel == BINOCS) ? TRUE: FALSE;
}

SINT32 fpFPSIsDeployed()
{
	if(stateFlags & STATE_DEPLOYED)
		return (fpFPSGetPlayerPose() == POSE_PRONE) ? 1: 2;

	return 0;
}

void fpFPSWeaponAutoSwitch()
{
	int				i;
	weapWeapon*		currentWeapon;
	mlVehicleData*	vehicleData = persGetPersonaModelDataPtr();
	int nextWeapon = fpsTrooper.weaponOfLastResort;

	for (i=0; i < MAX_NUM_FPS_WEAPONS; i++)
	{
		if ((i == fpsTrooper.activeWeaponNumber) || (fpsTrooper.weaponPositionModel[i] == NO_WEAPON))
			continue;

		// limitations in the weapWeapon structure
		if (i >= 4)
			continue;

		currentWeapon = &(((vehVehicle*)(vehicleData->VehicleMasterPtr))->Weapons[i][0]);

		if (!currentWeapon)
			continue;

		if ((currentWeapon->NumRoundsAmmoLeft[currentWeapon->AmmoTypeInUse] > 0) ||
			(currentWeapon->NumRoundsAmmoLeft[currentWeapon->AmmoTypeInUse] == -1))
		{
			nextWeapon = i;
			break;
		}
	}

	if (nextWeapon != fpsTrooper.activeWeaponNumber)
	{
		fpsTrooper.autoswitchWeaponNumber = nextWeapon;
	}
	else
	{
		fpsTrooper.autoswitchWeaponNumber = -1;
	}

	return;
}

void fpFPSAdjustLightForTrooper (piVECTOR* ld)
{
	static piMATRIX	rotMatrix;
	static SINT32	lastUpdateTime;

	if (lastUpdateTime != frametime)
	{
		mlVehicleData*	pVehicleData = persGetPersonaModelDataPtr();

		generateRotationMatrix (AROUND_Z, mlRAD_TO_DEG(pVehicleData->VehicleAtt.Yaw), &rotMatrix);
	}

	math3dMultiplyRotation (ld, ld, &rotMatrix);

	ld->z = -ld->z;
}



LOGICAL fpFPSIsObjectFPS (OBJECT* pObject)
{
	if (!pObject)
	{
		return FALSE;
	}

	if ((pObject->child) && (pObject->child->flag2 & OBJECT_ISFPS))
	{
		return TRUE;
	}

	return FALSE;
}

LOGICAL fpFPSIsTrooperSitting (OBJECT* pObject)
{
	// passing in nullptr means I already know I'm a fps object
	if (!pObject || fpFPSIsObjectFPS(pObject))
	{
		if (fpFPSGetPlayerPose() == POSE_SIT)
		{
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}
	else
	{
		if (gr2AnimIsTrooperSitting (pObject))
		{
			return TRUE;
		}
	}

	return FALSE;
}

#if USE_STATESET
void fpFPSDrawGunsight()
{
    static SINT32 startSniperDelay = 0;
	static SINT32 startSniperEndDelay = 0;

	mlVehicleData*	vehicleData = persGetPersonaModelDataPtr();

	if ( !(stateFlags & (STATE_WEAPON_SWITCHING|STATE_SIT)) &&
		 !(vehicleData->PersonData->Flags & fpFLAGS_FORCE_EXTERNAL) &&
		 !persIsInDeathCam() )
	{
		if (currentWeaponModel == BINOCS)
		{
            persGunsightDrawOverlay();
        }

		// Add the scope view function here   DEG
		// It will use the example above, but it will have a sight mark.
		// It will be called when the sniper wepon exists and a scope view is needed.

		// This is added to calulate the delay of any grenade for FPS. DEG
		if (fpsWeapons[currentWeaponModel].grenadeVisDelay)
		{
			if((fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_GRENADE) && 
				(currentFPSstate == THROW)&&
				(((frametime / 1000.0) - grenadeApperLapse) > fpsWeapons[currentWeaponModel].grenadeVisDelay ))
			{
				showGun = false;
			}
		}

		if(fpsWeapons[currentWeaponModel].sniperScope)
		{
            switch(currentFPSstate)
            {
            case AIM_FIRE:
            case STAND_FIRE:
            case CROUCH_FIRE:
            case PRONE_FIRE:
                {
                    if(scopeUp)
                    {
                        showArms = false;
				        showGun = false;
				        showClip = false;

                        persGunsightDrawFPSScope((int)currentArms);
                    }

                    break;
                }
            case AIM:
                {
                    if(!startSniperDelay)
                    {
                        startSniperDelay = frametime;
                    }

                    SINT32 delay = frametime - startSniperDelay;
                    FLOAT32 delayseconds = (FLOAT32)delay * 0.001f;

                    if(delayseconds >= fpsWeapons[currentWeaponModel].sniperScopeDelay)
				    {
					    showArms = false;
					    showGun = false;
					    showClip = false;

                        persGunsightDrawFPSScope((int)currentArms);

                        scopeUp = true;
				    }

					startSniperEndDelay = 0;

                    break;
                }
            default:
                {

					if (startSniperDelay && (fpsWeapons[currentWeaponModel].sniperScopeEndDelay))
					{
                      if(!startSniperEndDelay)
					  {
						startSniperEndDelay = frametime;
                      }

                      SINT32 delay = frametime - startSniperEndDelay;
                      FLOAT32 delayseconds = (FLOAT32)delay * 0.001f;

                      persGunsightDrawFPSScope((int)currentArms);

                      if(delayseconds >= fpsWeapons[currentWeaponModel].sniperScopeEndDelay)
				      {
					    scopeUp = false;
						showArms = true;
						showGun = true;
						showClip = true;
                        startSniperEndDelay = 0;
                        startSniperDelay = 0;
				     }

					}
					else
					{
						startSniperDelay = 0;
                        startSniperEndDelay = 0;
						scopeUp = false;
						showArms = true;
						showGun = true;
						showClip = true;
					}

					startSniperDelay = 0;
                }
            } // switch
		}// if sniper
	}
}
#else
void fpFPSDrawGunsight()
{
    static SINT32 startSniperDelay = 0;
	static SINT32 startSniperEndDelay = 0;

	mlVehicleData*	vehicleData = persGetPersonaModelDataPtr();

    UINT32 lighton = piGetEngineRenderState(piRENDERSTATE_LIGHTING);

    piSetRenderState(piRENDERSTATE_LIGHTING, FALSE);

	if ( !(stateFlags & (STATE_WEAPON_SWITCHING|STATE_SIT)) &&
		 !(vehicleData->PersonData->Flags & fpFLAGS_FORCE_EXTERNAL) &&
		 !persIsInDeathCam() )
	{
		if (currentWeaponModel == BINOCS)
		{
            persGunsightDrawOverlay();
        }

		// Add the scope view function here   DEG
		// It will use the example above, but it will have a sight mark.
		// It will be called when the sniper wepon exists and a scope view is needed.

		// This is added to calulate the delay of any grenade for FPS. DEG
		if (fpsWeapons[currentWeaponModel].grenadeVisDelay)
		{
			if((fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_GRENADE) && 
				(currentFPSstate == THROW)&&
				(((frametime / 1000.0) - grenadeApperLapse) > fpsWeapons[currentWeaponModel].grenadeVisDelay ))
			{
				showGun = false;
			}
		}

		if(fpsWeapons[currentWeaponModel].sniperScope)
		{
            switch(currentFPSstate)
            {
            case AIM_FIRE:
            case STAND_FIRE:
            case CROUCH_FIRE:
            case PRONE_FIRE:
                {
                    if(scopeUp)
                    {
                        showArms = false;
				        showGun = false;
				        showClip = false;

                        persGunsightDrawFPSScope((int)currentArms);
                    }

                    break;
                }
            case AIM:
                {
                    if(!startSniperDelay)
                    {
                        startSniperDelay = frametime;
                    }

                    SINT32 delay = frametime - startSniperDelay;
                    FLOAT32 delayseconds = (FLOAT32)delay * 0.001f;

                    if(delayseconds >= fpsWeapons[currentWeaponModel].sniperScopeDelay)
				    {
					    showArms = false;
					    showGun = false;
					    showClip = false;

                        persGunsightDrawFPSScope((int)currentArms);

                        scopeUp = true;
				    }

					startSniperEndDelay = 0;

                    break;
                }
            default:
                {

					if (startSniperDelay && (fpsWeapons[currentWeaponModel].sniperScopeEndDelay))
					{
                      if(!startSniperEndDelay)
					  {
						startSniperEndDelay = frametime;
                      }

                      SINT32 delay = frametime - startSniperEndDelay;
                      FLOAT32 delayseconds = (FLOAT32)delay * 0.001f;

                      persGunsightDrawFPSScope((int)currentArms);

                      if(delayseconds >= fpsWeapons[currentWeaponModel].sniperScopeEndDelay)
				      {
					    scopeUp = false;
						showArms = true;
						showGun = true;
						showClip = true;
                        startSniperEndDelay = 0;
                        startSniperDelay = 0;
				     }

					}
					else
					{
						startSniperDelay = 0;
                        startSniperEndDelay = 0;
						scopeUp = false;
						showArms = true;
						showGun = true;
						showClip = true;
					}

					startSniperDelay = 0;
                }
            } // switch
		}// if sniper
	}

    piSetRenderState(piRENDERSTATE_LIGHTING, lighton);
}
#endif

void fpFPSEnableBinoculars (SINT32 enableBinocs)
{
	if (!Online)
		fpsTrooper.binocsEnabled = TRUE;
	else
		fpsTrooper.binocsEnabled = enableBinocs;
}

void fpFPSMarkForVehicleCollideCheck(UINT32 flag)
{
	vehicleCollideCheckNeeded = flag;

#ifdef STOP_FIRING_INSIDE
	if (flag && !fpsHitVehicleThisFrame)
	{
		fpsHitVehicleThisFrame = TRUE;
		Write_Log (LOG_FPS, LOG_INFO, "%d fpsHitVehicleThisFrame set", frametime);
	}
#endif
}

UINT32 fpFPSVehicleCollideCheckNeeded()
{
	return (vehicleCollideCheckNeeded);
}

void fpFPSHitBuilderObject(const StratDB::StratObject* pStatue)
{
	if(pStatue == nullptr) return;

    if(currentBuilderObject == nullptr)
        fpFPSResetBuilderPosition();

	currentBuilderObject = pStatue;
	uiShowBuilderInfo(currentBuilderObject->DisplayName(), true);
}

void fpFPSBuilderEnd()
{
    currentBuilderObject = nullptr;
    uiShowBuilderInfo(nullptr, false);
}

void fpFPSResetBuilderPosition()
{
	mlVehicleData*	pVehicleData = persGetPersonaModelDataPtr();

	builderPosition.x = pVehicleData->VehiclePos.x;
	builderPosition.y = pVehicleData->VehiclePos.y;
	builderPosition.z = pVehicleData->VehiclePos.z;
}

UINT32 fpFPSTestBuilderValidity()
{
    if(!currentBuilderObject)
        return 0;

	FLOAT64VECTOR	deltaVec;

    //mathVectorMultiply64(&capturePosition, currentBuilderObject->
	// test 1 - distance
	mlVehicleData*	pVehicleData = persGetPersonaModelDataPtr();

	mathSubVectors (&deltaVec, &builderPosition, &(pVehicleData->VehiclePos));

	// capture tolerance distance, currently set to 1 meter
	if (((deltaVec.x * deltaVec.x) + (deltaVec.y*deltaVec.y) + (deltaVec.z*deltaVec.z)) > 1.0)
	{
		return 0;
	}

	return 1;
}
void fpFPSStartEmote (SCHAR* emote, SCHAR* handle, LOGICAL force)
{
	SINT32					i;
	SINT32					newEmoteID = -1;
	fpTPSEmoteType			newEmoteType;
	SINT32					combatEmoteID;
	SINT32					bodyEmoteID;
	vehVehicle*				targetVehicle;
	const WCHAR*				append;

	if (!emote)
		return;

	if (!emote[0])
		return;

	if (!fpsTrooper.initialized)
	{
		Write_Log(LOG_FPS, LOG_INFO, "FPS system not ready! (%s)", emote);
		return;
	}

	if ((stateFlags & STATE_EMOTE_PLAYING) && !force)
	{
		Write_Log(LOG_FPS, LOG_INFO, "Emote already playing! (%s)", emote);
		return;
	}

	if (stateFlags & STATE_DEAD)
	{
		Write_Log(LOG_FPS, LOG_INFO, "No emotes while dead! (%s)", emote);
		return;
	}

	if (stateFlags & STATE_SWIMMING)
	{
		Write_Log(LOG_FPS, LOG_INFO, "No emotes while swimming! (%s)", emote);
		return;
	}

	if (stateFlags & STATE_RELOADING)
	{
		Write_Log(LOG_FPS, LOG_INFO, "No emotes while reloading! (%s)", emote);
		return;
	}

	if (fpFPSIsTrooperFalling())
	{
		Write_Log(LOG_FPS, LOG_INFO, "No emotes while falling! (%s)", emote);
		return;
	}

	vehVehicle* playerVehicle = persGetPersonaVehiclePtr();
	append = nullptr;

	// ID the emote
	for (i=0; i < TPS_COMBATEMOTE_NUM_TYPES; i++)
	{
		if (!strncmp(fpTPSCombatEmoteCmds[i], emote, 4))
		{
			// remember, emoteID 0 is "no emote"
			newEmoteID = i + 1;
			newEmoteType = TPS_EMOTETYPE_COMBAT;
			combatEmoteID = i;
			break;
		}
	}

	if (newEmoteID < 0)
	{
		for (i=0; i < TPS_BODYEMOTE_NUM_TYPES; i++)
		{
			if (!strncasecmp(fpTPSBodyEmoteCmds[i], emote, 4))
			{
				// special case for salute
				if (!strncasecmp(emote, "salu", 4))
				{
					if (currentArms == GERMAN_INFANTRY)
					{
						i = TPS_BODYEMOTE_SALUTE_GERMAN;
					}
					else if (currentArms == FRENCH_INFANTRY)
					{
						i = TPS_BODYEMOTE_SALUTE_FRENCH;
					}
					else if (currentArms == BRITISH_INFANTRY)
					{
						i = TPS_BODYEMOTE_SALUTE_BRITISH;
					}
					else
					{
						i = TPS_BODYEMOTE_SALUTE_DEFAULT;
					}

				}

				// remember, emoteID 0 is "no emote"
				newEmoteID = i + TPS_COMBATEMOTE_NUM_TYPES + 1;
				newEmoteType = TPS_EMOTETYPE_FULLBODY;
				bodyEmoteID = i;
				break;
			}
		}
	}

	// bail if it's crap
	if (newEmoteID < 0)
	{
		Write_Log(LOG_FPS, LOG_INFO, "Bad emote %s passed to start emote", emote);
		return;
	}

	// fullybody emotes can only be played while not moving
	if (newEmoteType == TPS_EMOTETYPE_FULLBODY)
	{
		if ((!((stateFlags & STATE_MOVE_STILL) && (stateFlags & STATE_EYE_STAND))) && !force)
		{
			Write_Log(LOG_FPS, LOG_INFO, "Tried to play fullbody emote while moving (%s)", emote);
			return;
		}
		if (stateFlags & STATE_SIT)
		{
			Write_Log(LOG_FPS, LOG_INFO, "Tried to play fullbody emote while sitting (%s)", emote);
			return;
		}

		// now that we've excluded every case where we can't do a dressdown, do the dressdown specific stuff
		if (!strncasecmp(emote, "dres", 4))
		{
			if ((!handle) || (!handle[0]))
			{
				Write_Log(LOG_FPS, LOG_INFO, "Invalid handle passed to dressdown!");
				return;
			}
			ChatHandle target = chatGetHandle(handle);
			if(!target)
			{
				Write_Log(LOG_FPS, LOG_INFO, "Handle is not a real handle");
				return;
			}

			if (!((currentArms == BRITISH_HC) || (currentArms == GERMAN_HC) || (currentArms == FRENCH_HC)))
			{
				Write_Log(LOG_FPS, LOG_INFO, "Only HC can dress down!  (%s)", handle);
				return;
			}

			targetVehicle = vehFindVehicleByChatHandle(target);
			if (!targetVehicle)
			{
				Write_Log(LOG_FPS, LOG_INFO, "Failed to find target vehicle!  (%s)", handle);
				return;
			}
/*
			if (targetVehicle->Rank >= RAT_RANK)
			{
				// special rat functionality
			}
			else if (targetVehicle->VehicleType->Type == VEH_TYPE_FLOATER)
			{
				// special floater functionality
			}
			else */if (targetVehicle->Side == playerVehicle->Side)
			{
				resupplyTarget = targetVehicle->VehicleHostIndex;
				resupplyTime = frametime;
				append = chatGetWideCallsign(target);
			}
			else
			{
				Write_Log(LOG_FPS, LOG_INFO, "Invalid target for dressdown!  (%s)", handle);
				return;
			}
		}
	}
	// combat emotes, while prone, can only be started when not moving
	else if (stateFlags & STATE_EYE_PRONE)
	{
		mlVehicleData*	pVehicleData = persGetPersonaModelDataPtr();
		FLOAT32 speed = (FLOAT32) (mathExactVectorLength(&(pVehicleData->VehicleVel)));

		if (speed > 0.0)
		{
			Write_Log(LOG_FPS, LOG_INFO, "Tried to play prone combat emote while moving (%s) %f", emote, speed);
			return;
		}
	}

	// ok, emote start
	Write_Log(LOG_FPS, LOG_INFO, "Starting emote %s, id = %d, type = %d", emote, newEmoteID, newEmoteType);

	if (newEmoteType == TPS_EMOTETYPE_FULLBODY)
	{
		rnmainListEmoteText (IDE_LANG_UI, FPS_BODYEMOTE_OFFSET + i, colorGREENALPHA, nullptr, append);
	}
	else
	{
		rnmainListEmoteText (IDE_LANG_UI, FPS_COMBATEMOTE_OFFSET + i, colorGREENALPHA, nullptr, append);
	}

	// set the global stuff so determineNextState can figure things out
	if (newEmoteType == TPS_EMOTETYPE_COMBAT)
	{
		emoteLength = 3.0f;
	}
	else
	{
		emoteLength = fpTPSBodyEmoteLengths[bodyEmoteID];
		stateFlags |= STATE_NO_MOVEMENT;
	}

	emoteStartTime = frametime/1000.0f;
	emoteID = newEmoteID;
	emoteType = newEmoteType;
	stateFlags |= STATE_EMOTE_PLAYING;
}

void fpFPSForceShellEjection ()
{
	piMATRIX		fireAtt = IdentityMatrix;

	mlVehicleData* pVehicleData = persGetPersonaModelDataPtr();
	OBJECT* pObject = persGetPersonaVehicleObjectDataPtr();

	fpFPSGetFirePoint(pObject, &fireAtt, true);
	fireAtt._41 = 0;
	fireAtt._42 = 0;
	fireAtt._43 = 0;

	if (pVehicleData->PersonData->Flags & fpFLAGS_FORCE_EXTERNAL)
	{
		// should be FALSE
		fpFPSEjectShell (pObject, &fireAtt, TRUE);
	}
	else
	{
		fpFPSEjectShell (pObject, &fireAtt, TRUE);
	}
}

void fpFPSResupplySelf(vehVehicle*vehicle)
{
	if (!vehicle)
	{
		Write_Log (LOG_FPS, LOG_INFO, "Null vehicle tried to resupply me?!!");
		return;
	}

	fpFPSResupplySelf(chatGetWideCallsign(vehicle->Chat_Handle));
}

void fpFPSResupplySelf (const char* from)
{
	fpFPSResupplySelf(uiUTF8toUTF16(from));
}

void fpFPSResupplySelf (const wchar_t* from)
{
	static SINT32 lastResupplyTime;

	// FIXME .. this needs to be made more generic, but going in this way because I have 3 hours to do it
	// check to see if this is a HC officer .. if they are, I'm getting lashed, not resupplied
	//if (gr2AnimIsVehicleHC(vehicle))
	//{
		//fpFPSStartEmote ("attention", "", TRUE);
	//}
	//else
	if ((frametime - lastResupplyTime) > 5000)
	{
		vehVehicle*playerVehicle = persGetPersonaVehiclePtr();
		const char* utf8from = uiUTF16toUTF8(from);
		bool resupplied = false;

		Write_Log (LOG_FPS, LOG_INFO, "%s just resupplied me", utf8from);

		for(int i = 0; i < VEH_MAX_MOUNTS; i++)
		{
			weapWeapon* weapon = &playerVehicle->Weapons[i][0];

			if(weapon->WeaponType)
			{
				// we assume weapon 0 gets a full reload if InfResupply == 0
				// this is so we don't have to audit all of the loadouts at first
				// if InfResupply is set for weapon 0, we honor it though
				FLOAT32 infResupply = (i || weapon->WeaponType->InfResupply) ? weapon->WeaponType->InfResupply: 1.0f;

				if(infResupply && weapResupplyWeapon(weapon, infResupply, true))
				{
					// lets check for any new oversupply
					for(int j = 0; j < 4; j++)
					{
						if(weapon->NumOverSupplyRounds[j] > 0 && !oversupplyPosition.x && !oversupplyPosition.y)
						{
							// need to save oversupply position
							oversupplyPosition.x = (FLOAT32)playerVehicle->ModelData->VehiclePos.x;
							oversupplyPosition.y = (FLOAT32)playerVehicle->ModelData->VehiclePos.y;

							// reset the warning flag
							overSupplyWarning = false;

							// we're done
							break;
						}
					}

					if(!TUT_ACTIVE && !resupplied)
					{
						if(oversupplyPosition.x || oversupplyPosition.y)
							uiGameEvent(GAMEEVENT_OVERSUPPLY_RECEIVED, 1, from);
						else
							uiGameEvent(GAMEEVENT_INFSUPPLY_RECEIVED, 1, from);
					}

					resupplied = true;
				}

				// update this weapon's FPS clipbullets variable
				fpFPSUpdateAmmo(i, weapon);
			}
		}

		lastResupplyTime = frametime;

		if(resupplied)
			uiChatPrintLine(uiLocString("resupply.received").c_str(), from);
	}
	else
	{
		Write_Log (LOG_FPS, LOG_INFO, "Repeat resupply from %s suppressed", from);
	}
}

void fpFPSSetStandMaterial (SINT32 matid)
{
	groundMaterial = matid;
}

LOGICAL fpsCheckDeploy(bool doShake)
{
    mlVehicleData* pVehicleData = persGetPersonaModelDataPtr();
	if (!pVehicleData)
		return FALSE;

    vehVehicle*pVehicle = pVehicleData->VehicleMasterPtr;
	if (!pVehicle)
		return FALSE;

    LOGICAL retVal = TRUE;
    FLOAT64	 upDownAngle;

	if (fpsWeapons[currentWeaponModel].deployPivotBone)
	{
		// no deployment in trees
		if (groundMaterial == FoliageMatId)
			retVal = FALSE;
		else if (stateFlags & (STATE_WEAPON_SWITCHING|STATE_HOLD_FIRE|STATE_FIRE_DELAYED_WEAP|STATE_SWIMMING|STATE_RELOADING|STATE_NEEDS_RECYCLE))
			retVal = FALSE;
		else if (stat2Flags & (STAT2_NEEDS_RELOAD|STAT2_STANDING_ON_VEHICLE|STAT2_POSE_CHANGE))
			retVal = FALSE;
		else if(!(stateFlags & (STATE_EYE_PRONE | STATE_DEPLOYED)))
        {
			retVal = (fpsWeapons[currentWeaponModel].weaponType != FPS_WEAPONTYPE_LTMORTAR) ? _candeploy(0): FALSE;
        }
        else if (!(stateFlags & STATE_DEPLOYED))
        {
		    // this will only fail if we're trying to deploy; if we're already deployed, allow it
			static const FLOAT64 _MAX_VERTICAL_ANGLE2 = 25.0 * 25.0;
		    upDownAngle = AIM_VERTICAL_ANGLE;
		    if (upDownAngle * upDownAngle > _MAX_VERTICAL_ANGLE2)
		    {
			    retVal = FALSE;
		    }
        }

		if (!retVal && doShake && currentWeaponModel != BAR && currentWeaponModel != FG42G)
		{
			persShakeHead();
		}
	}
	else if (fpsWeapons[currentWeaponModel].auxAvailable[FPS_AUX_RGCUP])
	{
		mlVehicleData*	vehicleData = persGetPersonaModelDataPtr();
		if (!vehicleData || !vehicleData->PersonData)
			return FALSE;

		if (groundMaterial == FoliageMatId)
			retVal = FALSE;
		else if (stateFlags & (STATE_WEAPON_SWITCHING|STATE_SWIMMING|STATE_RELOADING|STATE_DEPLOYING|STATE_NEEDS_RECYCLE))
			retVal = FALSE;
		else if (stat2Flags & STAT2_NEEDS_RELOAD)
			retVal = FALSE;
		else if (fpsWeapons[currentWeaponModel].auxEnable[FPS_AUX_RGGRENADE])
			retVal = FALSE;
		else if (fpsWeapons[currentWeaponModel].auxEnable[FPS_AUX_RGCUP] && (currentGameWeapon->NumRoundsAmmoLeft[0] == 0))
			retVal = FALSE;
		else if (!(fpsWeapons[currentWeaponModel].auxEnable[FPS_AUX_RGCUP]) && (fpFPSWeaponOutOfSecondaryAmmo(currentGameWeapon)))
			retVal = FALSE;
		else if (currentFPSstate == RECYCLE)
		{
			retVal = FALSE;
		}
		else if ((vehicleData->VehicleVel.x * vehicleData->VehicleVel.x +
			vehicleData->VehicleVel.y * vehicleData->VehicleVel.y +
			vehicleData->VehicleVel.z * vehicleData->VehicleVel.z) > 0.0)
		{
			retVal = FALSE;
		}

		if (!retVal && doShake && currentWeaponModel != BAR && currentWeaponModel != FG42G)
		{
			persShakeHead();
		}
	}
	else
	{
		retVal = FALSE;
	}

	return retVal;
}

SINT32 fpFPSPlaySound (const SCHAR* sound)
{
	if (!sound)
		return -1;

	return (fpFPSfxPlayNormalSound (sound, IDE_FP_PRIORITY_HIGH, TYPE_FINITE_SOUND, nullptr, persGetPersonaVehicleObjectDataPtr()));
}

SINT32 fpFPSGetAmmoTypeNum (bool forType)
{
	// this should go away and simply be the last return statement
	/*
    if((stateFlags & STATE_DEPLOYED) && (fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_RIFLE) && (fpsWeapons[currentWeaponModel].auxEnable[FPS_AUX_RGCUP]))
    {
        return 1;
    }
	*/

	if(forType &&
	   fpsWeapons[currentWeaponModel].weaponType == FPS_WEAPONTYPE_LMG &&
	   currentGameWeapon->NumRoundsAmmoLeft[currentGameWeapon->AmmoTypeInUse] % 3 == 0 &&
	   currentGameWeapon->WeaponType->AmmoType[1])
	{
		return 1;
	}

	return currentGameWeapon->AmmoTypeInUse;
}

SINT32 fpFPSGetAmmoTypeNumByWeapon (SINT32 weaponType, weapWeapon* weapon)
{
	/*
	if ((fpsWeapons[weaponType].weaponType == FPS_WEAPONTYPE_RIFLE) && (fpsWeapons[weaponType].auxEnable[FPS_AUX_RGCUP]))
		return 1;
	else
	*/

	if(weapon)
		return weapon->AmmoTypeInUse;

	return 0;
}

LOGICAL fpFPSWeaponOutOfAmmo (weapWeapon* weapon)
{
	if (!weapon)
		return TRUE;

	if ((weapon->NumRoundsAmmoLeft[0]) ||
		(weapon->NumRoundsAmmoLeft[1]) ||
		(weapon->NumRoundsAmmoLeft[2]) ||
		(weapon->NumRoundsAmmoLeft[3]))
		return FALSE;

	return TRUE;
}

LOGICAL fpFPSWeaponOutOfSecondaryAmmo (weapWeapon* weapon)
{
	if (!weapon)
		return TRUE;

	if ((weapon->NumRoundsAmmoLeft[1]) ||
		(weapon->NumRoundsAmmoLeft[2]) ||
		(weapon->NumRoundsAmmoLeft[3]))
		return FALSE;

	return TRUE;
}

void fpFPSGetEyeUpDownRange(FLOAT32* upAngle, FLOAT32* downAngle)
{
	FLOAT32	maxUpAngle, maxDownAngle;

	if ((!upAngle) || (!downAngle))
		return;

	if (stateFlags & STATE_SIT)
	{
		maxUpAngle = -fpsTrooper.maxRidingEyeUpAngle;
		maxDownAngle = fpsTrooper.maxRidingEyeDownAngle;
	}
	else if ((fpsWeapons[currentWeaponModel].auxEnable[FPS_AUX_RGCUP]) &&
			(fpsWeapons[currentWeaponModel].weapDeployUpAngle[targetPose] != 0) &&
			(fpsWeapons[currentWeaponModel].weapDeployDownAngle[targetPose] != 0))
	{
		if ((fpsWeapons[currentWeaponModel].rgFiringRules) &&
			(targetPose == FPS_POSITION_CROUCH))
		{
			// in reality, the only example of this is the british RG at the moment
			// limited to +60 to +30 .. with a 45 degree offset
			// so could have just said up = 15, down = 15 and been done with it
			maxUpAngle = -1.0f*(fpsWeapons[currentWeaponModel].weapDeployKneelDeployUpAngle - fpsWeapons[currentWeaponModel].rgFiringAngle);
			maxDownAngle = -1.0f*(fpsWeapons[currentWeaponModel].weapDeployKneelDeployDownAngle - fpsWeapons[currentWeaponModel].rgFiringAngle);
		}
		else
		{
			maxUpAngle = -fpsWeapons[currentWeaponModel].weapDeployUpAngle[targetPose];
			maxDownAngle = fpsWeapons[currentWeaponModel].weapDeployDownAngle[targetPose];
		}
	}
	else
	{
		maxUpAngle = -fpsTrooper.maxEyeUpAngle;
		maxDownAngle = fpsTrooper.maxEyeDownAngle;
	}


	*upAngle = maxUpAngle;
	*downAngle = maxDownAngle;
}

void fpFPSSetInfantrySurfaceID (SINT32 surfaceID, struct trnCDTerrainObject* TO)
{
	LOGICAL	killTroop = FALSE;

	fpsTrooper.currentSurfaceID = surfaceID;

	trnCDTerrainObject* lastEnteredTO = nullptr;

	// override with vehmain's idea of the last entered TO if we found one
	if(vehmainLastEntered(&lastEnteredTO))
		TO = lastEnteredTO;

    vehVehicle*veh = persGetPersonaVehiclePtr();

	// Determine which building the player is in: don't overwrite the value in-place
	REL_OBJ_ID fpsBuildingID = { 0, 0, 0 };

	// check to see if the TO we're standing on has changed
	// if it hasn't, and it's damage level has
	// then we're likely going to die (building/bridge blowing up under us is bad)
	if (!TO)
	{
		// terra non firma? :)
		fpsTrooper.lastTO = nullptr;
		fpsTrooper.lastTODamageLevel = 0;
	}
	else if (fpsTrooper.lastTO != TO)
	{
        if(veh && TO->ObjId.objClass == CLASS_BUILDING)
            fpsBuildingID = TO->ObjId;

		// if we've changed terrain objects .. check the last one we were on
		// if it is a bridge, and wasn't destroyed, and now is .. you're falling into the river, and
		// need to die
		if (fpsTrooper.lastTO)
		{
            if(fpsTrooper.lastTO->ObjId.objClass == CLASS_CP && fpsTrooper.lastTO->ObjId.objType == CP_BRIDGE)
            {
			    // if the bridge was "repaired" toss you into the air and port you to the ground
			    if((fpsTrooper.lastTO->DamageLevel == 0) && (fpsTrooper.lastTO->DamageLevel != fpsTrooper.lastTODamageLevel))
				    fpsTrooper.teleportToGround = TRUE;
            }
		}

		fpsTrooper.lastTO = TO;
		fpsTrooper.lastTODamageLevel = TO->DamageLevel;
	}
	else
	{
        SCHAR objclass = TO->ObjId.objClass;

        if(veh && objclass == CLASS_BUILDING)
        {
            fpsBuildingID = TO->ObjId;
        }

		// if the building we're standing on is a bridge, and wasn't destroyed, and now is .. bye bye
		if (TO->ObjId.objClass == CLASS_CP && TO->ObjId.objType == CP_BRIDGE)
		{
			if(fpsTrooper.lastTODamageLevel != TO->DamageLevel)
                fpsTrooper.teleportToGround = TRUE;

			fpsTrooper.lastTODamageLevel = TO->DamageLevel;
		}
	}

	if ( memcmp(&fpsBuildingID, &veh->fpsBuildingID, sizeof(fpsBuildingID)) != 0 )
		veh->fpsBuildingID = fpsBuildingID;

	if (killTroop)
	{
#ifdef BETA_RELEASE
		uiChatPrintLine ("Bridge destroyed .. you're dead!");
#endif
		persKillCrew();
	}

	if (fpsTrooper.teleportToGround)
	{
#ifdef BETA_RELEASE
		uiChatPrintLine ("Bridge repaired .. popping you to top!");
#endif
		mlVehicleData* pVehicleData = persGetPersonaModelDataPtr();

		pVehicleData->VehiclePos.z += 100.0f;
		pVehicleData->LastVehiclePos.z = pVehicleData->VehiclePos.z;

		fpUpdatePerson (pVehicleData, 0.0);
	}
}

void fpFPSAdjustAngles (FLOAT32 deltaTime, FLOAT64 minSpeed, FLOAT64 maxSpeed, FLOAT64* xAngle, FLOAT64 xTarget)
{
	FLOAT64		xVel, xDelta;

	if ((!xAngle))
		return;

	if (minSpeed <= 0)
	{
		*xAngle = xTarget;
		return;
	}

	// smooth the horizontal eye movement
	xDelta = xTarget - *xAngle;

	xVel = xDelta * fpsTrooper.eyeMoveMultiplier;

	if (xVel > 0)
	{
		if (xVel > maxSpeed)
			xVel = maxSpeed;
		if (xVel < minSpeed)
			xVel = minSpeed;

		*xAngle += xVel * deltaTime;
		if (*xAngle > xTarget)
			*xAngle = xTarget;
	}
	else
	{
		if (xVel < -maxSpeed)
			xVel = -maxSpeed;
		if (xVel > minSpeed)
			xVel = minSpeed;

		*xAngle += xVel * deltaTime;
		if (*xAngle < xTarget)
			*xAngle = xTarget;
	}

}

void fpFPSLeanOff()
{
	controlStateFlags &= ~(FPS_LEAN_RIGHT|FPS_LEAN_LEFT);
	stat2Flags &= ~(STAT2_LEAN_LEFT|STAT2_LEAN_RIGHT);
}

LOGICAL fpFPSGetLeanPoints (piVECTOR* startPos, piVECTOR* endPos)
{
	if (!((stat2Flags & STAT2_LEAN_LEFT) || (stat2Flags & STAT2_LEAN_RIGHT)))
		return FALSE;

	if ((!startPos) || (!endPos)) return FALSE;

	startPos->x = 0.0f;
	startPos->y = 0.0f;
	startPos->z = TROOPER_STAND_EYE_HEIGHT + fpFPSGetHeight();

	endPos->x = (FLOAT32)fpsTrooper.leanHorizTarget * 2.0f;
	endPos->y = 0.0f;
	endPos->z = TROOPER_STAND_EYE_HEIGHT + fpFPSGetHeight() + (FLOAT32)(fpsTrooper.leanVertTarget * 2.0f);

	// ye olde fudge factor .. this is to prevent folks from viewing into walls by leaning into them

	return TRUE;
}

void fpFPSSetStandingOnVehicle(LOGICAL value)
{
	if (value)
		stat2Flags |= STAT2_STANDING_ON_VEHICLE;
	else
		stat2Flags &= ~STAT2_STANDING_ON_VEHICLE;
}

FLOAT64 fpFPSGetBridgeDamage()
{
	return fpsTrooper.bridgeDamageJoules;
}

FLOAT64 fpFPSGetBridgeRepair()
{
	return -fpsTrooper.bridgeRepairJoules;
}

FLOAT32 fpFPSGetConcussionMod()
{
	return (fpsTrooper.concussionMod) ? (FLOAT32)fpsTrooper.concussionMod: 1.0f;
}

// 1 is increase, 0 is decrease
void fpFPSSightAdjust (SINT32 sightParam)
{
	static int lastAdjustTime;

	if (fpsWeapons[currentWeaponModel].numSightDists <= 0)
		return;

	if ((frametime - lastAdjustTime) < (SINT32)(fpsWeapons[currentWeaponModel].sightAdjustTime * 1000.0f))
		return;

	lastAdjustTime = frametime;

	if (sightParam)
		fpsWeapons[currentWeaponModel].currentSightNotch++;
	else
		fpsWeapons[currentWeaponModel].currentSightNotch--;

	// -1 because it's 1 base vs 0 base
	if (fpsWeapons[currentWeaponModel].currentSightNotch > (fpsWeapons[currentWeaponModel].numSightDists - 1))
		fpsWeapons[currentWeaponModel].currentSightNotch = 0;
	else if (fpsWeapons[currentWeaponModel].currentSightNotch < 0)
		fpsWeapons[currentWeaponModel].currentSightNotch = fpsWeapons[currentWeaponModel].numSightDists - 1;

	_calculateConvergenceAngle();

	// old code
	//fpsWeapons[currentWeaponModel].convergenceAngle = fpsWeapons[currentWeaponModel].sightConvergences[currentSightNotch];

	/* new code
	vehVehicle*pPlayerVehicle	= persGetPersonaDataPtr();
	FLOAT32 angle				= 0.0f;
	int hwn						= fpsTrooper.highlightedWeaponNumber;
	int sightDist				= fpsWeapons[currentWeaponModel].getSightDist(currentSightNotch);

	if(pPlayerVehicle)
	{
		persGunsightCalcSightAngle(
			&angle,
			(char*)pPlayerVehicle->VehicleType->Loadout[0].WeaponTypes[hwn][0]->AmmoType[0]->Name,
			(FLOAT32)sightDist);

		fpsWeapons[currentWeaponModel].convergenceAngle = mlRAD_TO_DEG (angle);
	}
	*/

	if (fpsWeapons[currentWeaponModel].sightAdjustSound)
	{
		fpFPSPlaySound (fpsWeapons[currentWeaponModel].sightAdjustSound);
	}

	/* we now display this in the HUD, no need to show it in chat
	{
		char tString[255];
		sprintf (tString, "%.0dm", fpsWeapons[currentWeaponModel].getSightDist());
		rnmainListEmoteText (IDE_LANG_UI, 9000, colorWHITEALPHA, nullptr, tString);
	}
	*/

	return;
}

FLOAT32 fpFPSGetRedScreenTime ()
{
	if (fpsTrooper.redScreenFadeTime > 0)
		return fpsTrooper.redScreenFadeTime;
	else
		return 15.0f;
}

FLOAT32	fpFPSGetRedScreenMinAlpha()
{
	return fpsTrooper.redScreenMinAlpha;
}

bool fpFPSOnBridge()
{
    if(fpsTrooper.lastTO == nullptr)
    {
        return false;
    }

    if(fpsTrooper.lastTO->ObjId.objClass == CLASS_CP)
    {
        if(fpsTrooper.lastTO->ObjId.objType == CP_BRIDGE)
        {
            return true;
        }
    }

    return false;
}

void fpFPSParadropStart()
{
	if (!fpsTrooper.isParatroop)
		return;

	if (!fpsTrooper.parachuteWorn)
		return;

	if (!fpsTrooper.parachuteOKToOpen)
		return;

	bool dead = fpFPSIsDead();

	stateFlags = STATE_DEFAULT_MASK|STATE_NO_MOVEMENT|STATE_RELAXED|STATE_NO_ARM_SWAY;

	if(dead) stateFlags |= STATE_DEAD;

	currentEyeHeight = fpFPSGetEyeAdjust(FPS_POSITION_STAND);
}

void fpFPSParachuteOpened()
{
	if (fpsTrooper.parachuteOpen)
		return;

#ifdef BETA_RELEASE
	uiChatPrintLine (uiLocString(IDE_LANG_UI, 702).c_str());
#endif
	fpsTrooper.parachuteOpen = TRUE;
	fpsTrooper.parachuteOKToOpen = FALSE;

	bool dead = fpFPSIsDead();

	stateFlags = STATE_DEFAULT_MASK|STATE_NO_MOVEMENT|STATE_RELAXED;

	if(dead) stateFlags |= STATE_DEAD;

	currentEyeHeight = fpFPSGetEyeAdjust(FPS_POSITION_STAND);

	fpFPSPlaySound (fpsTrooper.parachuteOpeningSound);

	// allow us to look around again
	fpFPSetHeadFixed(FALSE);
}

void fpFPSParachuteLanded()
{
	fpsTrooper.parachuteOKToOpen = FALSE;

	showArms = true;
	showGun = true;
	showClip = true;

	stateFlags &= ~(STATE_NO_MOVEMENT | STATE_EYE_MASK);

	if (!fpsTrooper.parachuteOpen)
		return;

#ifdef BETA_RELEASE
	uiChatPrintLine (uiLocString(IDE_LANG_UI, 701).c_str());
#endif

	fpsTrooper.parachuteOpen = FALSE;
	fpsTrooper.parachuteWorn = FALSE;

	// if we died on the way down, make sure we stay that way
	if(fpFPSIsDead())
		fpFPSKillTrooper();

	// try to go prone, if that fails, try to crouch, only if that fails (and it shouldn't) .. stand
	if (!fpFPSCheckNewPose(POSE_PRONE))
	{
		controlStateFlags |= FPS_PRONE;
		controlStateFlags &= ~FPS_CROUCH;
		stateFlags |= STATE_EYE_PRONE;
		targetPose = FPS_POSITION_PRONE;
		fpFPSAdjustEyeToPose (0.0);
		fpFPSMoveModel();
	}
	else if (!fpFPSCheckNewPose(POSE_CROUCH))
	{
		controlStateFlags |= FPS_CROUCH;
		controlStateFlags &= ~FPS_PRONE;
		stateFlags |= STATE_EYE_CROUCH;
		targetPose = FPS_POSITION_CROUCH;
		fpFPSAdjustEyeToPose (0.0);
		fpFPSMoveModel();
	}
	else
	{
		controlStateFlags &= ~(FPS_CROUCH|FPS_PRONE);
		stateFlags |= STATE_EYE_STAND;
		targetPose = FPS_POSITION_STAND;
		fpFPSAdjustEyeToPose (0.0);
		fpFPSMoveModel();
	}

	prevEyeStateFlags = stateFlags & STATE_EYE_MASK;
	currentEyeHeight = fpFPSGetEyeAdjust();

	if (stateFlags & STATE_SWIMMING)
		fpFPSPlaySound (fpsTrooper.paratroopLandingWaterSound);
	else
		fpFPSPlaySound (fpsTrooper.paratroopLandingSound);

}

LOGICAL fpFPSIsTrooperFalling ()
{
	mlVehicleData* pVehicleData = persGetPersonaModelDataPtr();
	return  (pVehicleData && pVehicleData->PersonData && (pVehicleData->PersonData->Flags & fpFLAGS_FALLING));
}

LOGICAL fpFPSIsTrooperParachuteOpen()
{
	return fpsTrooper.parachuteOpen;
}

void fpFPSSetInheritedVelocity (FLOAT64VECTOR* velocity)
{
	if (!velocity)
		return;

	fpsTrooper.inheritedVehicleVelocity = *velocity;
}

void fpFPSDoRidingShake()
{
	static SINT32	nextShakeTime = 0;
	static FLOAT32	shakeStrength = 0;
	static SINT32	shakeDuration = 0;
	FLOAT32			speed;

	if (frametime > (nextShakeTime + shakeDuration))
	{
		speed = (FLOAT32)mathExactVectorLength (&fpsTrooper.inheritedVehicleVelocity);

		// 100m/s = 360kph
		if (speed > 100.0f)
			speed = 100.0f;

		shakeStrength = (FLOAT32)mathGetFloat32ChaoticNumber() * 0.01f * (speed / 100.0f);
		nextShakeTime = frametime + 10000 + (SINT32)(5000.0f * mathGetFloat32ChaoticNumber());

		if (speed > 30)
			nextShakeTime -= (SINT32) (9000.0f/(speed / 100.0f));

		shakeDuration = 250 + (SINT32)(500.0f * mathGetFloat32ChaoticNumber());
	}

	if (frametime > nextShakeTime && frametime < (nextShakeTime + shakeDuration)) // within the window
	{
		persShakeCamera(shakeStrength, shakeDuration); //DEG
	}
	else // does this ever hit?
	{
		speed = (FLOAT32)mathExactVectorLength2(&fpsTrooper.inheritedVehicleVelocity);

		if (speed > 100.0f)
		{
			persShakeCamera(0.00075f, shakeDuration);
		}
	}
}

FLOAT64 fpFPSGetParachuteFallSpeed ()
{
	fpPersonData*	pPersonData;
	FLOAT32			fallTime;

	mlVehicleData*	pVehicleData = persGetPersonaModelDataPtr();
	if (pVehicleData)
	{
		pPersonData = (fpPersonData*) pVehicleData->PersonData;
		if (!pPersonData)
			return 0.0;
	}
	else
		return 0.0;

	if (!((pPersonData && pPersonData->Flags & fpFLAGS_FALLING)))
		return 0.0;

	fallTime = (ft10mscount - pPersonData->StartFallTime10ms)/100.0f;

	if (fallTime >= fpsTrooper.parachuteDecelerationTime)
		return fpsTrooper.parachuteFallSpeed;

	if (fallTime <= fpsTrooper.parachuteDeployTime)
		return (9.8 * fallTime);

	return (9.8*fpsTrooper.parachuteDeployTime - (9.8 * fpsTrooper.parachuteDeployTime - fpsTrooper.parachuteFallSpeed) *
		(fallTime - fpsTrooper.parachuteDeployTime)/(fpsTrooper.parachuteDecelerationTime - fpsTrooper.parachuteDeployTime));
}

bool fpFPSControlBlocked(int flag)
{
	return (controlBlockFlags & flag) != 0;
}

void fpFPSControlBlockSet(int flag)
{
	controlBlockFlags |= flag;
}

void fpFPSControlBlockClear(int flag)
{
	controlBlockFlags &= ~flag;
}

void fpFPSControlBlockReset()
{
	controlBlockFlags = 0;
}

// have to be sitting on a plane to trigger this emote
LOGICAL fpFPSOkToJumpEmote()
{
	fpPersonData*	pPersonData = nullptr;
	vehVehicle*		FPSRideVehicle = nullptr;

	mlVehicleData*	pVehicleData = persGetPersonaModelDataPtr();
	if (pVehicleData)
	{
		pPersonData = (fpPersonData*) pVehicleData->PersonData;
		if (!pPersonData)
			return FALSE;
	}
	else
		return FALSE;

	// Check STATE_DEAD is set and STATE_SIT is not.
	if ((stateFlags & (STATE_SIT|STATE_DEAD)) == STATE_DEAD)
		return FALSE;

	vehVehicle* pPlayerVehicle = persGetPersonaVehiclePtr();
	if (!pPlayerVehicle)
		return FALSE;

	if(pPlayerVehicle->RideVehicleHostIndex > 0)
		FPSRideVehicle = vehFindVehicle(pPlayerVehicle->RideVehicleHostIndex);

	if (FPSRideVehicle == nullptr)
		return FALSE;

	if (FPSRideVehicle->VehicleType->Type != VEH_TYPE_AIR)
		return FALSE;

	return TRUE;
}

void fpFPSToggleAutoReload()
{
	fpsTrooper.autoReload = !fpsTrooper.autoReload;
	if (fpsTrooper.autoReload)
		uiChatPrintLine ("Infantry autoreload enabled");
	else
		uiChatPrintLine ("Infantry autoreload disabled");
}

void fpFPSSetAutoReload(LOGICAL state)
{
	fpsTrooper.autoReload = state;
}

void fpFPSMapState(LOGICAL state)
{
	if (fpsTrooper.initialized)
	{
		if (state)
		{
			//stateFlags |= STATE_NO_MOVEMENT;
			fpsTrooper.readingMap = TRUE;
			controlStateFlags &=
				~(//FPS_AUTORUN|
				 FPS_TURN_RIGHT
				|FPS_TURN_LEFT
				|FPS_STRAFE_RIGHT
				|FPS_STRAFE_LEFT
				//|FPS_FORWARD
				|FPS_BACKWARD
				|FPS_AIM
				|FPS_LOOK_LEFT
				|FPS_LOOK_RIGHT
				|FPS_LEAN_LEFT
				|FPS_LEAN_RIGHT
				|FPS_SPRINT
				);

			if(controlStateFlags & FPS_FORWARD)
			{
				// switch to auto run
				controlStateFlags &= ~FPS_FORWARD;
				controlStateFlags |= FPS_AUTORUN;
			}
		}
		else
		{
			if(!fpFPSIsDead())
			{
				if (!(stateFlags & (STATE_SIT |STATE_EMOTE_PLAYING)) &&
					!(fpsTrooper.parachuteOKToOpen) &&
					!(fpsTrooper.parachuteOpen))
				{
					stateFlags &= ~STATE_NO_MOVEMENT;
				}
			}

			fpsTrooper.readingMap = FALSE;
		}
	}

	return;
}

void fpFPSSelectNextWeapon ()
{
	fpFPSIterateHighlightedWeapon (1);
}

void fpFPSSelectPreviousWeapon ()
{
	fpFPSIterateHighlightedWeapon (-1);
}

void fpFPSIterateHighlightedWeapon (SINT32 direction)
{
	LOGICAL	done = FALSE;
	SINT32	foundWeaponNumber, weapNumIt;

	foundWeaponNumber = -1;

	weapNumIt = fpsTrooper.highlightedWeaponNumber;

 	while (!done)
	{
		weapNumIt += direction;

		if (weapNumIt >= MAX_NUM_FPS_WEAPONS)
			weapNumIt = 0;
		else if (weapNumIt == MAP_WEAPON_NUMBER)
			continue;
		else if (weapNumIt < 0)
			weapNumIt = BINOCS_WEAPON_NUMBER;

		if (weapNumIt == fpsTrooper.activeWeaponNumber || fpFPSIsWeaponSwitchOK(weapNumIt))
		{
			foundWeaponNumber = weapNumIt;
			done = TRUE;
		}
	}

	if (foundWeaponNumber < 0)
		return;

	fpFPSPlaySound("ui/sounds/select.wav");

	fpsTrooper.highlightedWeaponNumber = foundWeaponNumber;
	fpsTrooper.highlightTime = frametime / 1000.0;

	Write_Log (LOG_FPS, LOG_INFO, "Highlighting %d at %lf", fpsTrooper.highlightedWeaponNumber, fpsTrooper.highlightTime);
}

SINT32 fpFPSGetWeaponType()
{
	if(fpsTrooper.activeWeaponNumber >= 0)
		return fpsWeapons[fpsTrooper.weaponPositionModel[fpsTrooper.activeWeaponNumber]].weaponType;

	return FPS_WEAPONTYPE_ITEM;
}

UINT32 fpFPSGetHighlightedWeaponType()
{
	fpsWeaponTypes weaponType = fpsTrooper.weaponPositionModel[fpsTrooper.highlightedWeaponNumber];

	/*
	SINT32 ammoTypeNum = fpFPSGetAmmoTypeNumByWeapon((SINT32)weaponType, nullptr);

	if (ammoTypeNum == 0)
	{
		if (weaponType == RG_G30)
			weaponType = KAR98K;
		else if (weaponType == RG_VB)
			weaponType = LEBEL;
		else if (weaponType == RG_MILLS)
			weaponType = ENFIELD;
	}
	*/

	return (UINT32)weaponType;
}

#ifdef DEBUG_DEPLOY_CRAP
void _fpsdrawline(piMATRIX& world)
{
    piMATRIX w = world;
    w._41 = w._42 = w._43 = 0.0f;

    piPushMatrix(WORLD_MATRIX);
    piSetWorldMatrix(&IdentityMatrix);
    draw3dLine(&_obstructp2, &_obstructp1, colorYELLOWALPHA);
    draw3dLine(&_centerp2, &_centerp1, colorWHITEALPHA);
    draw3dLine(&_leftp2, &_leftp1, colorREDALPHA);
    draw3dLine(&_rightp2, &_rightp1, colorBLUEALPHA);

    piVECTOR pt2, pt3;
    mathSubVectors(&pt2, &_centerp2, &_centerp1);
    piMATRIX rotate = IdentityMatrix;

    for(int i = 1; i <= 90; i+=4)
    {
        rotate._11 =  cosf(DEG_TO_RAD(i));
        rotate._12 = -sinf(DEG_TO_RAD(i));
        rotate._21 = -rotate._12;
        rotate._22 =  rotate._11;

        math3dMultiplyRotation(&pt3, &pt2, &rotate);
        mathAddVectors(&pt3, &pt3, &_centerp1);

        draw3dLine(&pt3, &_centerp1, colorYELLOWALPHA);
    }

    for(int i = 1; i <= 90; i+=4)
    {
        rotate._11 =  cosf(DEG_TO_RAD(i));
        rotate._12 =  sinf(DEG_TO_RAD(i));
        rotate._21 = -rotate._12;
        rotate._22 =  rotate._11;

        math3dMultiplyRotation(&pt3, &pt2, &rotate);
        mathAddVectors(&pt3, &pt3, &_centerp1);

        draw3dLine(&pt3, &_centerp1, colorBLUEALPHA);
    }

    piPopMatrix(WORLD_MATRIX);
}
#endif

static inline void _getdeployboundry(double& s,
                                      double& e,
                                      piVECTOR& p1,
                                      piVECTOR& p2,
                                      bool topfan)
{
    piMATRIX rotate = IdentityMatrix;

    // Send through short list collider
    static FASTCOLLIDE collidelist[32];
    FASTCOLLIDE* listptr = collidelist;
    UINT32 numcollisions = 32;

    mlVehicleData* data = persGetPersonaModelDataPtr();

    piVECTOR pt2 = p2;

    piVECTOR pt3;
    piVECTOR pt4;

    // Translate to origin
    mathSubVectors(&pt2, &p2, &p1);

    double startangle = 0.0;

    while(startangle > -90.0)
    {
        startangle -= 4.0f;

        rotate._11 =  cosf((float)(DEG_TO_RAD(startangle)));
        rotate._21 = -rotate._12;
        rotate._12 = -sinf((float)(DEG_TO_RAD(startangle)));
        rotate._22 =  rotate._11;

        // Rotate
        math3dMultiplyRotation(&pt3, &pt2, &rotate);
        // Translate back
        math3dAddVectors(&pt4, &pt3, &p1);

        listptr = collidelist;
        numcollisions = 32;

 	    trncdCollideWithListOfTerrainObjects(&listptr,
                                             &numcollisions,
                                             data->ShortTrnList,
                                             ML_NUM_SHORT_LIST_ELEMENTS,
                                             &p1,
                                             &pt4,
                                             0);

        // We need this every run, in case there are no collisions.
        math3dNormalize(&pt3);
        s = atan2(pt3.x, pt3.y);

        if(topfan)
        {
            if(numcollisions - 32 > 0)
            {
                break;
            }
        }
        else if(numcollisions - 32 == 0)
        {
            break;
        }
    }

    startangle = 0.0f;

    while(startangle < 90.0)
    {
        startangle += 4.0f;

        rotate._11 =  cosf((float)(DEG_TO_RAD(startangle)));
        rotate._22 =  rotate._11;
        rotate._12 = -sinf((float)(DEG_TO_RAD(startangle)));
        rotate._21 = -rotate._12;

        math3dMultiplyRotation(&pt3, &pt2, &rotate);
        math3dAddVectors(&pt4, &pt3, &p1);

        listptr = collidelist;
        numcollisions = 32;

 	    trncdCollideWithListOfTerrainObjects(&listptr,
                                             &numcollisions,
                                             data->ShortTrnList,
                                             ML_NUM_SHORT_LIST_ELEMENTS,
                                             &p1,
                                             &pt4,
                                             0);

        // We need this every run, in case there are no collisions.
        math3dNormalize(&pt3);
        e = atan2(pt3.x, pt3.y);

        if(topfan)
        {
            if(numcollisions - 32 > 0)
            {
                break;
            }
        }
        else if(numcollisions - 32 == 0)
        {
            break;
        }
    }
}

static inline bool _candeploy(int direction)
{
    static double start = 0, end = 0;
    static double topstart = 0, topend = 0;

    mlVehicleData* data = persGetPersonaModelDataPtr();
    if(!data) return false;

	static const FLOAT64 _MAX_VERTICAL_ANGLE2 = 25.0 * 25.0;

    if(AIM_VERTICAL_ANGLE * AIM_VERTICAL_ANGLE > _MAX_VERTICAL_ANGLE2)
	    return false;

    piVECTOR pt1, pt2;

    piMATRIX firepoint = IdentityMatrix;
	fpFPSGetFirePoint (data->VehicleMasterPtr->VehicleObjectData->PlaneObject, &firepoint);

    // First, shoot a ray straight ahead. There cannot be any obstruction.
    FLOAT64VECTOR obstruct = {0, -1, 0};
    mathVectorMultiply64(&obstruct, &obstruct, &data->VehicleAttMat);

    double angle = atan2(obstruct.x, obstruct.y);

    // We have already calculated the boundries.
    if(direction != 0)
    {
        double end2 = end;

        if(start > end)
        {
            end2 += PI2;

            if(angle < 0)
            {
                angle += PI2;
            }
        }

        if(start > 0 && end > 0)
        {
            if(angle < 0)
            {
                angle += PI2;
            }
        }

        // Angle between boundries
        if(angle > start && angle < end2)
        {
            return true;
        }

        // Don't get stuck
        if(direction < 0 && angle >= end2)
        {
            return true;
        }

        // Don't get stuck
        if(direction > 0 && angle <= start)
        {
            return true;
        }

        return false;
    }

    // Get relative world position of the fire point
    mathAddVectors(&obstruct, &obstruct, &data->VehiclePos);

    obstruct.x -= WorldOriginOffset.x;
    obstruct.y -= WorldOriginOffset.y;

    // Convert to float
    math64To32Vec(&pt2, &obstruct);
    //pt2.z = -playpos.z + 0.1f; // Ray shoots out the neck

    // Get our pos
    pt1.x = (float)(data->VehiclePos.x - WorldOriginOffset.x);
    pt1.y = (float)(data->VehiclePos.y - WorldOriginOffset.y);
    //pt1.z =  pt2.z; // Have the height at neck level

	pt1.z = -((float)(data->VehiclePos.z - WorldOriginOffset.z));

	pt2.z = pt1.z + -firepoint._43 + .01f;

	pt1.z += -fpGetZAdjust(TPS_POSETYPE_STAND, TPS_MOVETYPE_DEPLOYED) + .01f;

    // Extend vector slightly by one unit
    piVECTOR dir;
    mathSubVectors(&dir, &pt2, &pt1);
    math3dNormalize(&dir);

    pt2.x += dir.x;
    pt2.y += dir.y;

    // Send through short list collider
    static FASTCOLLIDE collidelist[32];
    FASTCOLLIDE* listptr = collidelist;
    UINT32 numcollisions = 32;

    // Only check for obstruction for initial test
    if(direction == 0)
    {
 	    trncdCollideWithListOfTerrainObjects(&listptr,
                                            &numcollisions,
                                            data->ShortTrnList,
                                            ML_NUM_SHORT_LIST_ELEMENTS,
                                            &pt1,
                                            &pt2,
                                            0);

#ifdef DEBUG_DEPLOY_CRAP
        _obstructp1 = pt1;
        _obstructp2 = pt2;
#endif

        // Any collisions? If so, we are done. Cannot deploy.
        if(numcollisions - 32 > 0)
        {
            return false;
        }

        // Fan out the this ray and check for boundry collisions..such as a window boundry
        // This will limit our yaw.
        _getdeployboundry(topstart, topend, pt1, pt2, true);
    }

#ifdef DEBUG_DEPLOY_CRAP
    memset(&_centerp1, 0, sizeof(piVECTOR));
    memset(&_centerp2, 0, sizeof(piVECTOR));
#endif

    // If we are here, nothing is obstructing our view to place the gun.
    // Now check for ledge to place the weapon
    // Shoot a ray from naval.

    // We extended the obstruction vector, now bring it back for ledge collider.
    pt2.x -= (dir.x);
    pt2.y -= (dir.y);
    pt2.z += 0.4f; // Shoot ray at waist level

    pt1.z += 0.4f; // Waist level

    // Initial check for collision object.
    if(direction == 0)
    {
        listptr = collidelist;
        numcollisions = 32;

 	    trncdCollideWithListOfTerrainObjects(&listptr,
                                            &numcollisions,
                                            data->ShortTrnList,
                                            ML_NUM_SHORT_LIST_ELEMENTS,
                                            &pt1,
                                            &pt2,
                                            0);

#ifdef DEBUG_DEPLOY_CRAP
        _centerp1 = pt1;
        _centerp2 = pt2;
#endif

        // Any collisions? If not, there is no ledge. Cannot deploy.
        if(numcollisions - 32 == 0)
        {
            // Try at knee level if standing. If so, set trooper to kneel.
            if(stateFlags & STATE_EYE_STAND)
            {
                listptr = collidelist;
                numcollisions = 32;

                pt2.z += 0.3f; // Ray at knee level
                pt1.z += 0.3f; // Ray at knee level

 	            trncdCollideWithListOfTerrainObjects(&listptr,
                                                    &numcollisions,
                                                    data->ShortTrnList,
                                                    ML_NUM_SHORT_LIST_ELEMENTS,
                                                    &pt1,
                                                    &pt2,
                                                    0);
#ifdef DEBUG_DEPLOY_CRAP
                _centerp1 = pt1;
                _centerp2 = pt2;
#endif
                if(numcollisions - 32 == 0)
                {
                    return false;
                }

                // There is an object to deploy on. Set trooper to kneel.
                controlStateFlags |= FPS_CROUCH;
            }
            else
            {
                return false;
            }
        }

        _getdeployboundry(start, end, pt1, pt2, false);

        // Bound more if within a window frame...etc
        start = topstart > start ? topstart : start;
        end = topend < end ? topend : end;

        return true;
    }

    return true;
}


FLOAT32 fpFPSGetGrenadeThrowPower()
{
	if(fpsEndGrenadeThrowTime == 0) fpsEndGrenadeThrowTime = frametime;
	FLOAT32 maxPower = .25f + .75f * (fpsEndGrenadeThrowTime - fpsStartGrenadeThrowTime) / 2000.0f;
	if(maxPower < .25f) maxPower = .25f;
	else if(maxPower > 1.0f) maxPower = 1.0f;

	return maxPower;
}

#define SQR(x) ((x)*(x))

void fpFPSUpdateOverSupply(mlVehicleData* vd)
{
	static SINT32 lastCheck = 0;

	if((!oversupplyPosition.x && !oversupplyPosition.y) || frametime < (lastCheck + 200))
		return;

	float distance = math3dFastSqrt((float)SQR(vd->VehiclePos.x - oversupplyPosition.x) +
									(float)SQR(vd->VehiclePos.y - oversupplyPosition.y));

	if(distance > fpsTrooper.overSupplyRadius)
	{
		// alert the ui of the reset
		uiGameEvent(GAMEEVENT_OVERSUPPLY_LOST);

		// clear oversupply rounds
		for(int i = 0; i < VEH_MAX_MOUNTS; i++)
		{
			for(int j = 0; j < VEH_MAX_WEAPONS_PER_MOUNT; j++)
				vd->VehicleMasterPtr->Weapons[i][0].NumOverSupplyRounds[j] = 0;
		}

		// reset oversupply position
		oversupplyPosition.x = oversupplyPosition.y = 0.0f;
	}
	else if(distance >= fpsTrooper.overSupplyRadius * .2f)
	{
		// alert the ui we are close to the edge
		// but only if we haven't already sent one warning
		if(!overSupplyWarning)
		{
			uiGameEvent(GAMEEVENT_OVERSUPPLY_WARNING);
			overSupplyWarning = true;
		}
	}
	else // we're within range
	{
		if(overSupplyWarning)
		{
			uiGameEvent(GAMEEVENT_OVERSUPPLY_INRANGE);
			overSupplyWarning = false;
		}
	}

	lastCheck = frametime;
}

void fpFPSUpdateAmmo(SINT32 weaponNumber, weapWeapon* weapon, SINT32 forceAmmo)
{
	if(!weapon)
	{
		vehVehicle* vehicle = persGetPersonaVehiclePtr();
		if(vehicle)
			weapon = &vehicle->Weapons[weaponNumber][0];
	}

	if(!weapon || !weapon->WeaponType)
		return;

	// make sure we have some ammo
	_selectBestAmmo(weaponNumber, weapon);

	if(forceAmmo < 0 && fpsWeapons[fpsTrooper.weaponPositionModel[weaponNumber]].auxEnable[FPS_AUX_RGCUP])
		forceAmmo = 0; // force this to the rifle ammo

	UINT32 ammoIndex	= (forceAmmo >= 0 && forceAmmo < 4) ? (UINT32)forceAmmo : (UINT32)weapon->AmmoTypeInUse;
	UINT32 clipSize		= weapon->WeaponType->NumRoundsInClip;
	SINT32 ammoLeft		= (SINT32)weapon->NumRoundsAmmoLeft[ammoIndex];

	if(ammoLeft < 0)
		fpsWeapons[fpsTrooper.weaponPositionModel[weaponNumber]].clipBullets = clipSize;
	else
		fpsWeapons[fpsTrooper.weaponPositionModel[weaponNumber]].clipBullets = std::min((UINT32)ammoLeft, clipSize);
}

void fpFPSProcessTPSEmote(ChatHandle handle, SINT32 emoteID)
{
	uiMapProcessPlayerEventToAllMaps(handle, MAP_PLAYER_EVENT_EMOTE, emoteID);
}

// calculates the convergence angle for the current weapon, ammo and sight distance
void _calculateConvergenceAngle(int weaponIndex)
{
	if(weaponIndex < 0)
		weaponIndex = fpsTrooper.activeWeaponNumber;

	if(weaponIndex < 0 || weaponIndex >= NUM_WEAPONS)
		return;

	fpFPSWeaponData* weaponData = &fpsWeapons[fpsTrooper.weaponPositionModel[weaponIndex]];

	weaponData->convergenceAngle = 0;

	if(weaponData->numSightDists)
	{
		mlVehicleData* playerVehicle = persGetPersonaModelDataPtr();

		if(!playerVehicle)
			return;

		FLOAT32 angle				= 0.0f;
		int sightDist				= weaponData->getSightDist();
		weapWeapon* weapon			= &playerVehicle->VehicleMasterPtr->Weapons[weaponIndex][0];
		UINT16 ammoTypeIndex		= (weaponData->auxAvailable[FPS_AUX_RGCUP]) ? 0 : (UINT16)weapon->AmmoTypeInUse; // RG rifles always calculate for the rifle ammo
		const unsigned char* ammo	= weapon->WeaponType->AmmoType[ammoTypeIndex]->Name;

		persGunsightCalcSightAngle(&angle, (char*)ammo, (FLOAT32)sightDist);
		weaponData->convergenceAngle = mlRAD_TO_DEG(angle);
	}
}

void _selectBestAmmo(SINT32 weaponNumber, weapWeapon* weapon)
{
	// special case for automatic mortar ammo switching
	// probbaly should be generic, but RG rifles wouldn't work right
	if(fpsWeapons[fpsTrooper.weaponPositionModel[weaponNumber]].weaponType == FPS_WEAPONTYPE_LTMORTAR)
	{
		if(weapon->NumRoundsAmmoLeft[weapon->AmmoTypeInUse] == 0)
		{
			for(int i = 0; i < 4; i++)
			{
				if(weapon->NumRoundsAmmoLeft[i] > 0)
				{
					weapon->AmmoTypeInUse = i;
					break;
				}
			}
		}
	}
}

dGeomID _createFPSShape(dSpaceID space, dGeomID geom)
{
	// our shape transform
	dGeomID shape = dCreateGeomTransform(space);

	// init our transform geom
	dGeomTransformSetCleanup(shape, 1);
	dGeomTransformSetInfo(shape, 1);

	// save our capsule in the transform geom
	dGeomTransformSetGeom(shape, geom);

	// return it
	return shape;
}

dGeomID _createFPSBall(dSpaceID space, double radius, double xoff, double yoff, double zoff)
{
	// our actual collision capsule
	dGeomID ball = dCreateSphere(0, radius);

	// set our offset position
	dGeomSetPosition(ball, xoff, yoff, zoff);

	return _createFPSShape(space, ball);
}

dGeomID _createFPSCylinder(dSpaceID space, double length, double radius, double angle, int axis, double xoff, double yoff, double zoff)
{
	dGeomID cylinder = dCreateCylinder(0, radius, length);

	// arbitrary rotation passed in?
	if(angle)
	{
		Vec3d rotation;

		switch(axis)
		{
			case 1: rotation.x() = 1.0f; break;
			case 2: rotation.y() = 1.0f; break;
			case 3: rotation.z() = 1.0f; break;
		}

		// rotate it along the x so that it fits our body better
		Matrixd attitude = Matrixd::rotate(mlDEG_TO_RAD(angle), rotation);

		// convert it to an ode matrix
		dMatrix3 dAttitude;
		MatrixTodMatrix(attitude, dAttitude);

		// set the rotation for the capsule (relative to the transform geom
		dGeomSetRotation(cylinder, dAttitude); // should this be capsule and not shape?
	}

	// set our offset position
	dGeomSetPosition(cylinder, xoff, yoff, zoff);

	return _createFPSShape(space, cylinder);
}

// length is total length of capsule
dGeomID _createFPSCapsule(dSpaceID space, double length, double radius, double angle, int axis, double xoff, double yoff, double zoff)
{
	// capsules run along the Z axis
	double clen = length - (radius * 2.0f);
	if(clen <= 0) clen = .01f;

	// our actual collision capsule
	dGeomID capsule = dCreateCapsule(0, radius, clen);

	// arbitrary rotation passed in?
	if(angle)
	{
		Vec3d rotation;

		switch(axis)
		{
			case 1: rotation.x() = 1.0f; break;
			case 2: rotation.y() = 1.0f; break;
			case 3: rotation.z() = 1.0f; break;
		}

		// rotate it along the x so that it fits our body better
		Matrixd attitude = Matrixd::rotate(mlDEG_TO_RAD(angle), rotation);

		// convert it to an ode matrix
		dMatrix3 dAttitude;
		MatrixTodMatrix(attitude, dAttitude);

		// set the rotation for the capsule (relative to the transform geom
		dGeomSetRotation(capsule, dAttitude); // should this be capsule and not shape?
	}

	// set our offset position
	dGeomSetPosition(capsule, xoff, yoff, zoff);

	return _createFPSShape(space, capsule);
}

// length is total length of capsule
dGeomID _createFPSRay(dSpaceID space, double length, double xoff, double yoff, double zoff, double dx, double dy, double dz)
{
	// our actual collision capsule
	dGeomID ray = dCreateRay(0, length);
	dGeomRaySet(ray, xoff, yoff, zoff, dx, dy, dz);

	return _createFPSShape(space, ray);
}


dGeomID _createFPSBox(dSpaceID space, double lx, double ly, double lz)
{
	// our actual collision capsule
	dGeomID box = dCreateBox(0, lx * 2.0f, ly * 2.0f, lz * 2.0f);

	// set our offset position
	dGeomSetPosition(box, 0, 0, 0);

	return _createFPSShape(space, box);
}

// length is total length of capsule
dGeomID _createFPSCapsuleTest(dSpaceID space, double length, double radius, double angle = 0, int axis = 1, double xoff = 0, double yoff = 0, double zoff = 0)
{
	// capsules run along the Z axis

	// our shape transform
	double clen = length - (radius * 2.0f);
	if(clen <= 0) clen = .001f;

	// our actual collision capsule
	dGeomID capsule = dCreateCapsule(space, radius, clen);

	// arbitrary rotation passed in?
	if(angle)
	{
		Vec3d rotation;

		switch(axis)
		{
			case 1: rotation.x() = 1.0f; break;
			case 2: rotation.y() = 1.0f; break;
			case 3: rotation.z() = 1.0f; break;
		}

		// rotate it along the x so that it fits our body better
		Matrixf attitude = Matrixf::rotate((float)angle, rotation);

		// convert it to an ode matrix
		dMatrix3 dAttitude;
		MatrixTodMatrix(attitude, dAttitude);

		// set the rotation for the capsule (relative to the transform geom
		dGeomSetRotation(capsule, dAttitude); // should this be capsule and not shape?
	}

	// return it
	return capsule;
}

// !!!HACK!!! - vehInterpolator
fpFPSPositions HACK_forced_pose = FPS_NUM_POSITIONS;

void _updateFPSColliders(void)
{
	_collider.collider = nullptr;

	fpFPSPositions pose;
	if ( HACK_forced_pose == FPS_NUM_POSITIONS )
		pose = targetPose;
	else
		pose = HACK_forced_pose;

	switch(pose)
	{
		case POSE_STAND:
			_collider.collider = &_standCollider;
		break;
		case POSE_CROUCH:
			_collider.collider = &_kneelCollider;
		break;
		case POSE_PRONE:
		default:
			_collider.collider = &_proneCollider;
			break;
	}
}

/*
void _createFPSColliders()
{
	dSpaceID space = collider.space();

	// add our rays
	collider.addGeom(dCreateRay(space, 50.0f), 0, AREA_COLLISION_FLAGS); // center
	collider.addGeom(dCreateRay(space, 3.0f), 0, AREA_COLLISION_FLAGS); // front
	collider.addGeom(dCreateRay(space, 3.0f), 0, AREA_COLLISION_FLAGS); // right
	collider.addGeom(dCreateRay(space, 3.0f), 0, AREA_COLLISION_FLAGS); // back
	collider.addGeom(dCreateRay(space, 3.0f), 0, AREA_COLLISION_FLAGS); // left

	// dSpaceID space, double radius, double length, double angle = 0, int axis = 1, double xoff = 0, double yoff = 0, double zoff = 0
	const UINT32 capsuleFlags = (COLLIDER_VEHICLE_MASK|COLLIDER_MANMADE_MASK|COLLIDER_TRN_TREE|COLLIDER_VEH_HULK|COLLIDER_BLOCK);

	// stand collider
	collider.addGeom(_createFPSCapsule(space, 1.4f, .5f, 0, 1, 0, -.1f, 1.0f), COLLIDER_FPS, capsuleFlags); // front
	collider.addGeom(_createFPSCapsule(space, 1.4f, .5f, 0, 1, 0, .1f, 1.0f), COLLIDER_FPS, capsuleFlags); // back
	collider.addGeom(dCreateRay(space, 1.0f), COLLIDER_FPS, capsuleFlags); // hi
	collider.addGeom(dCreateRay(space, 1.0f), COLLIDER_FPS, capsuleFlags); // mid
	collider.addGeom(dCreateRay(space, 1.0f), COLLIDER_FPS, capsuleFlags); // lo

	// crouch collider
	collider.addGeom(_createFPSCapsule(space, .7f, .5f, 0, 1, 0, -.1f, .65f), COLLIDER_FPS, capsuleFlags); // front
	collider.addGeom(_createFPSCapsule(space, .7f, .5f, 0, 1, 0, .1f, .65f), COLLIDER_FPS, capsuleFlags); // back

	// prone collider
	collider.addGeom(_createFPSCapsule(space, .2f, .4f, PI2, 2, 0, .2f, .1f), COLLIDER_FPS, capsuleFlags); // prone

	// create a box collider here for sto collisions
	collider.addGeom(_createFPSBox(space, 2.0f, 2.0f, 2.0f), COLLIDER_AREA, COLLIDER_STO);

	// init our colliders
	_updateFPSColliders();
}
*/

FPSCollider::FPSCollider() : colliderQuery()
{
	//addGeomToCollideWith((dGeomID)colliderWorldSpace());
	setMaxContactsPerHit(10);
	//setNormalFilter(0.0f, 0.0f, 1.0f);
}

Vec3d* HACK_forced_pos = nullptr;

void FPSCollider::Update(double deltaTime)
{
	Vec3d pos;
	Matrixd attitude;

	if ( HACK_forced_pos == nullptr ) {
		vehVehicle* player = persGetPersonaVehiclePtr();
		if(player == nullptr) return;

		// setup rays
		pos				= Vec3d(player->ModelData->VehiclePos);
		attitude		= Matrixd(player->ModelData->VehicleAttMat);
	} else {
		pos = *HACK_forced_pos;
		attitude = IdentityMatrix;
	}

	// convert to the physical world
	pos = physicsGameToPhysical(pos);

	colliderQuery::dGeomList& colliders = geoms();

	dMatrix3 dAttitude;
	MatrixTodMatrix(attitude, dAttitude);

	// update our geoms
	for(size_t i = 0; i < colliders.size(); i++)
	{
		dGeomID geom = colliders.at(i);

		// set our position and roation
		// all colliders are wrapped by a transform geom
		dGeomSetPosition(geom, pos.x(), pos.y(), pos.z());
		dGeomSetRotation(geom, dAttitude);
	}
}

#define WEAPON_LAG_AVG_SAMPLES 3

void fpFPSAddWeaponLag(int plane, FLOAT64 amount, bool absolute)
{
	// no log while para is dropping
	if(fpsTrooper.isParatroop && (fpsTrooper.parachuteOKToOpen || fpsTrooper.parachuteOpen) && fpFPSIsTrooperFalling())
		return;

	if(controlStateFlags & FPS_AIM)
		amount *= .1f;

	// no new weapon lag while sitting or statically deployed
	if(!(stateFlags & STATE_SIT) && (!fpsWeapons[currentWeaponModel].staticDeploy || !(stateFlags & STATE_DEPLOYED)))
	{
		switch(plane)
		{
			case AROUND_Z: // horizontal
			{
				if(absolute)
					horizWeaponLag += amount;
				else
					horizWeaponLagAcc += amount;
			}
			break;
			case AROUND_X: // vertical
			{
				if(absolute)
					vertWeaponLag += amount;
				else
					vertWeaponLagAcc += amount;
			}
			break;
		}
	}
}

FLOAT32 _deployZ()
{
	/*
	switch(fpFPSGetPlayerPose())
	{
		case POSE_STAND:
		{
			static FLOAT32 z = 1.4f;
			return z;
		}
		break;
		case POSE_CROUCH:
		{
			static FLOAT32 z = 1.4f;
			return z;
		}
		break;
		case POSE_PRONE:
		{
			static FLOAT32 z = 1.4f;
			return z;
		}
		break;
	}
	*/

	return .85f;
}

void _updateWeaponLag(float deltaTime)
{
	static FLOAT64 horizSamples[WEAPON_LAG_AVG_SAMPLES];
	static FLOAT64 vertSamples[WEAPON_LAG_AVG_SAMPLES];

	static int horizIndex = 0;
	static int vertIndex = 0;

	FLOAT64 horizAvgAcc = 0;
	FLOAT64 vertAvgAcc = 0;

	// handle avg horizontal acceleration
	horizSamples[horizIndex++] = horizWeaponLagAcc;

	for(int i = 0; i < WEAPON_LAG_AVG_SAMPLES; i++)
		horizAvgAcc += horizSamples[i];

	horizAvgAcc = (horizAvgAcc/WEAPON_LAG_AVG_SAMPLES);

	if(horizIndex >= WEAPON_LAG_AVG_SAMPLES)
		horizIndex = 0;

	// handle avg vertical acceleration
	vertSamples[vertIndex++] = vertWeaponLagAcc;

	for(int i = 0; i < WEAPON_LAG_AVG_SAMPLES; i++)
		vertAvgAcc += vertSamples[i];

	vertAvgAcc = (vertAvgAcc/WEAPON_LAG_AVG_SAMPLES);

	if(vertIndex >= WEAPON_LAG_AVG_SAMPLES)
		vertIndex = 0;

	float WMOD		= (controlStateFlags & FPS_AIM) ? .1f: 1.0f;
	float WLVM		= WEAPON_LAG_VMAX * WMOD; // max from center
	float WLHM		= WEAPON_LAG_HMAX * WMOD; // max from center

	float WLA		= WEAPON_LAG_ACCEL * WMOD; // how much to lag based on mouse movement
	float WLB		= WEAPON_LAG_BLEED * WMOD; // how much to bleed each frame

	float WLJ		= WEAPON_LAG_JUMP * WMOD; // base value to calculate from
	float WLJMAX	= WEAPON_LAG_JUMPMAX * WMOD; // max jump between frames

	if(horizWeaponLag || horizAvgAcc)
	{
		FLOAT64 jump = 0.0f;

		if(horizAvgAcc && horizWeaponLag)
		{
			FLOAT64 bleed = WLJ * (std::min(fabs(horizWeaponLag)/WLB, 1.0));

			jump = (WLJ*2) * (std::min(fabs(horizAvgAcc)/WLA, 1.0));
			jump = (horizAvgAcc < 0) ? -jump: jump;
			jump += (horizWeaponLag < 0) ? bleed: -bleed;
		}
		else if(horizAvgAcc)
		{
			jump = WLJ * (std::min(fabs(horizAvgAcc)/WLA, 1.0));
			jump = (horizAvgAcc < 0) ? -jump: jump;
		}
		else if(horizWeaponLag)
		{
			jump = WLJ * (std::min(fabs(horizWeaponLag)/WLB, 1.0));
			jump = (horizWeaponLag < 0) ? jump: -jump;
		}

		if(jump)
		{
			jump *= deltaTime;

			//if(false && fabs(jump) < WEAPON_LAG_JUMPMIN)
			//	jump = (jump < 0) ? -WEAPON_LAG_JUMPMIN: WEAPON_LAG_JUMPMIN;

			if(fabs(jump) > WLJMAX)
				jump = (jump < 0) ? -WLJMAX: WLJMAX;

			if((horizWeaponLag < 0 && (horizWeaponLag + jump) > 0) ||
			   (horizWeaponLag > 0 && (horizWeaponLag + jump) < 0))
				horizWeaponLag = 0.0f;
			else
				horizWeaponLag	+= jump;

			if(fabs(horizWeaponLag) > WLHM)
				horizWeaponLag = (horizWeaponLag < 0) ? -WLHM: WLHM;
		}

		//Write_Debug("Horizontal Weapon Lag lag:%.4f acc:%.4f jmp:%.4f", horizWeaponLag, horizWeaponLagAcc, jump);
	}

	// for testing
	//horizWeaponLag = (stateFlags & STATE_EYE_PRONE) ? -WEAPON_LAG_HMAX: 0;

	if(vertWeaponLag || vertAvgAcc)
	{
		FLOAT64 jump = 0.0f;

		if(vertAvgAcc && vertWeaponLag)
		{
			FLOAT64 bleed = WLJ * (std::min(fabs(vertWeaponLag)/WLB, 1.0));

			jump = WLJ * (std::min(fabs(vertAvgAcc)/WLA, 1.0));
			jump = (vertAvgAcc < 0) ? -jump: jump;
			jump += (vertWeaponLag < 0) ? bleed: -bleed;
		}
		else if(vertAvgAcc)
		{
			jump = WLJ * (std::min(fabs(vertAvgAcc)/WLA, 1.0));
			jump = (vertAvgAcc < 0) ? -jump: jump;
		}
		else if(vertWeaponLag)
		{
			jump = WLJ * (std::min(fabs(vertWeaponLag)/WLB, 1.0));
			jump = (vertWeaponLag < 0) ? jump: -jump;
		}

		if(jump)
		{
			jump *= deltaTime;

			//if(false && fabs(jump) < WEAPON_LAG_JUMPMIN)
			//	jump = (jump < 0) ? -WEAPON_LAG_JUMPMIN: WEAPON_LAG_JUMPMIN;

			if(fabs(jump) > WLJMAX)
				jump = (jump < 0) ? -WLJMAX: WLJMAX;

			if((vertWeaponLag < 0 && (vertWeaponLag + jump) > 0) ||
			   (vertWeaponLag > 0 && (vertWeaponLag + jump) < 0))
				vertWeaponLag = 0.0f;
			else
				vertWeaponLag	+= jump;

			if(fabs(vertWeaponLag) > WLVM)
				vertWeaponLag = (vertWeaponLag < 0) ? -WLVM: WLVM;
		}
	}

	horizWeaponLagAcc = vertWeaponLagAcc = 0.0f;
}

#ifdef BETA_RELEASE
void fpFPSSetAnimationSpeed(float speed)
{
	animationSpeed = speed;
}

float fpFPSGetAnimationSpeed()
{
	return animationSpeed;
}

void fpFPSSetSuperTroop(LOGICAL st)
{
	SuperTroop = st;

	if(SuperTroop)
	{
		fpsTrooper.movementSpeeds[FPS_SPEED_SPRINT] = 100.0f;
		fpsTrooper.movementSpeeds[FPS_SPEED_SWIM] = 10.0f;
		fpsTrooper.movementSpeeds[FPS_SPEED_CRAWL] = 10.0f;
		fpsTrooper.forwardAccel = 100.0f;
		fpsTrooper.atpStandStill = 0.9f;
		fpsTrooper.atpSprintMinimum = 0.01f;
		fpsTrooper.atpSprintAllowAgain = 0.01f;
		fpsTrooper.atpStandSprint = 0.1f;
	}
	else
	{
		// force a reload of the fps data
		fpFPSDataReadFPSFile();
		/*
		fpsTrooper.movementSpeeds[FPS_SPEED_SPRINT] = 6.0f;
		fpsTrooper.movementSpeeds[FPS_SPEED_SWIM] = 0.75f;
		fpsTrooper.movementSpeeds[FPS_SPEED_CRAWL] = 0.75f;
		fpsTrooper.forwardAccel = 6.0f;
		fpsTrooper.atpStandStill = 0.004f;
		fpsTrooper.atpSprintMinimum = 0.8f;
		fpsTrooper.atpSprintAllowAgain = 0.85f;
		fpsTrooper.atpStandSprint = -0.00667f;
		*/
	}
}
#endif
