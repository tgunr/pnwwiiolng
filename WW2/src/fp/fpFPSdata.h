#pragma once

#define USE_NEW_DECALS
//#undef USE_NEW_DECALS

#include <string>
#include <vector>
#include "wwii/wwiiCovert.h"

#define	MAX_NUM_FPS_WEAPONS		10
#define FPS_MAX_NUM_SIGHT_DISTS	10

// tolua_begin
enum fpsInfantryTypes
{
	BRITISH_INFANTRY,
	GERMAN_INFANTRY,
	FRENCH_INFANTRY,
	BRITISH_HC,
	GERMAN_HC,
	FRENCH_HC,
	BRITISH_PARA_PACK,
	GERMAN_PARA_PACK,
	FRENCH_PARA_PACK,
	BRITISH_PARA_NOPACK,
	GERMAN_PARA_NOPACK,
	FRENCH_PARA_NOPACK,
	BRITISH_PARA_PACK_SELF,
	GERMAN_PARA_PACK_SELF,
	FRENCH_PARA_PACK_SELF,
	BRITISH_ML,
	GERMAN_ML,
	FRENCH_ML,
	AMERICAN_INFANTRY,
	NUM_INFANTRY_TYPES
};

enum fpsSpeedTypes
{
	FPS_SPEED_STILL = 0,
	FPS_SPEED_SPRINT,
	FPS_SPEED_TROT,
	FPS_SPEED_QUICK,
	FPS_SPEED_WALK,
	FPS_SPEED_CREEP,
	FPS_SPEED_CRAWL,
	FPS_SPEED_SWIM,
	FPS_SPEED_BACKWARDS,
	FPS_SPEED_BACKWARDS_CROUCH,
	FPS_NUM_SPEEDS
};

enum fpsWeaponTypes
{
	NO_WEAPON,
	THOMPSON,                   // BRITISH_SUB_MACHINE_GUN
	MP40,                       // GERMAN_SUB_MACHINE_GUN
	MAS,                        // FRENCH_SUB_MACHINE_GUN
	SATCHEL,
	KAR98K,			//5
	ENFIELD,
	LEBEL,
	M1935A,
	LUGER,
	MILLS36MK1,		//10
	FRBANG,
	STIGR,
	WEBLEY,
	BINOCS,
	KNIFE_AXIS,
	KNIFE_ALLIED,	//15
	BREN,           // BRITISH LMG
	MG34,           // GERMAN LMG
	FM29,           // FRENCH LMG
	HGR39,
	M18,			//20
	FRSMOKE,
	AMMO,
	MAS36,
	RG_G30,             // GERMAN GRENADIER
	RG_VB,              // FRENCH GRENADIER
	RG_MILLS,           // BRITISH GRENADIER
	BOYSMK1,            // BRITISH ATR
	PZB39,
	BRIDGE_REPAIR,
	MAP,
	ENFIELDMK4,         // BRITISH_SNIPER
	LEBELMLE,           // FRENCH_SNIPER
	KAR98KS,            // GERMAN_SNIPER
	SATCHEL_AX,			//DEG_CHANGE
	LTMORTAR_UK,		// British 2 in light mortar
	LTMORTAR_FR,		// French Brandt mle 37 light mortar
	LTMORTAR_DE,		// German GRW36 light mortar
	BUILD_TOOL,			// infantry build tool
	M1GARAND,
	BAR,
	COLT1911,
	SPRINGFIELD1903,
	GEW43,
	M1THOMPSON,
	MARK2,
	BAZOOKA,
	PZSCHRECK,
	M1903A3,
	USSMOKE,
	PIAT,
	NUM_WEAPONS
};
// tolua_end

extern SCHAR* fpFPSWeaponKeyStrings[NUM_WEAPONS];

// tolua_begin
enum fpsWeaponStates
{
	RELOAD,
	STAND_FIRE,
	CROUCH_FIRE,
	AIM_FIRE,
	RECYCLE,
	LOWER,			//5
	RAISE,
	IDLE,
	KNEEL,
	AIM,
	TROT,			//10
	CREEP,
	CRAWL,
	SPRINT,
	PRONE,
	PRONE_FIRE,		//15
	SIT,
	RELOAD2,
	RELOAD3,
	THROW,
	DEPLOYED,		//20
	DEPLOYING,
	UNDEPLOYING,
	DEPLOYED_FIRE,
	DEPLOYED_RELOAD,
	DEPLOYED_KNEEL,
	AIM_SMG,
	NUM_WEAP_STATES
};

// if ever adding a weapon type, make sure to add a corresponding string in fpsData.cpp:fpFPSWeaponTypeStrings
enum fpFPSWeaponType
{
	FPS_WEAPONTYPE_SMG,
	FPS_WEAPONTYPE_SATCHEL,
	FPS_WEAPONTYPE_RIFLE,
	FPS_WEAPONTYPE_AUTO_PISTOL,
	FPS_WEAPONTYPE_GRENADE,
	FPS_WEAPONTYPE_REVOLVER,
	FPS_WEAPONTYPE_ITEM,
	FPS_WEAPONTYPE_KNIFE,
	FPS_WEAPONTYPE_LMG,
	FPS_WEAPONTYPE_RIFLE_GRENADE,
	FPS_WEAPONTYPE_ATR,
	FPS_WEAPONTYPE_LTMORTAR,
	FPS_WEAPONTYPE_BUILD,
	FPS_WEAPONTYPE_AUTO_RIFLE, // semiauto
	FPS_WEAPONTYPE_ROCKET, // bazooka/panzershrek/piat
	FPS_WEAPONTYPE_NUM_TYPES
};
// tolua_end

extern SCHAR* fpFPSWeaponTypeStrings[FPS_WEAPONTYPE_NUM_TYPES];

// if adding an icon type, make sure to add a string to fpsData.c:hud_icons
enum fpFPSIconType
{
	FPS_ICON_SMG,
	FPS_ICON_SATCHEL,
	FPS_ICON_RIFLE,
	FPS_ICON_PISTOL,
	FPS_ICON_GRENADE,
	FPS_ICON_SMOKE_GRENADE,
	FPS_ICON_BINOCS,
	FPS_ICON_KNIFE,
	FPS_ICON_LMG_BREN,
	FPS_ICON_LMG_MG34,
	FPS_ICON_LMG_FM29,
	FPS_ICON_RESUPPLY,
	FPS_ICON_RIFLE_GRENADE,
	FPS_ICON_ATR,
	FPS_ICON_LTMORTAR,
	FPS_ICON_BUILD,
	FPS_ICON_AUTO_RIFLE,
	FPS_ICON_ROCKET,
	NUM_ICONS
};

// tolua_begin
enum fpFPSPositions
{
	FPS_POSITION_STAND,
	FPS_POSITION_CROUCH,
	FPS_POSITION_PRONE,
	FPS_POSITION_SITTING,
	FPS_NUM_POSITIONS
};
// tolua_end

enum fpFPSSoundType
{
	FPS_SOUND_NONE,
	FPS_SOUND_CRAWL,
	FPS_SOUND_CREEP_AIM,
	FPS_SOUND_CREEP,
	FPS_SOUND_TROT_AIM,
	FPS_SOUND_TROT,
	FPS_SOUND_SPRINT,
	FPS_SOUND_WATER,
	FPS_SOUND_FALL_NORMAL,
	FPS_SOUND_FALL_NORMAL_HURT,
	FPS_SOUND_FALL_WATER,
	FPS_SOUND_FALL_WATER_HURT,
	FPS_SOUND_FREEFALL,
	FPS_SOUND_PARACHUTE_FALL,
	FPS_MAX_SOUNDS
};

enum fpFPSFallType
{
	FPS_FALL_NORMAL,
	FPS_FALL_NORMAL_HURT,
	FPS_FALL_WATER,
	FPS_FALL_WATER_HURT,
	FPS_MAX_FALL_TYPES
};

enum fpFPSAuxModelType
{
	FPS_AUX_RGCUP,
	FPS_AUX_RGGRENADE,
	FPS_NUM_AUX_MODELS
};

enum fpFPSEyeHeights
{
	FPS_EYE_DEAD,
	FPS_EYE_PRONE,
	FPS_EYE_PRONEMOVE,
	FPS_EYE_PRONEAIM,
	FPS_EYE_PRONEDEPLOY,
	FPS_EYE_CROUCH,
	FPS_EYE_CROUCHMOVE,
	FPS_EYE_CROUCHAIM,
	FPS_EYE_CROUCHDEPLOY,
	FPS_EYE_STAND,
	FPS_EYE_STANDMOVE,
	FPS_EYE_STANDAIM,
	FPS_EYE_STANDDEPLOY,
	FPS_NUM_EYEHEIGHTS
};

enum HUD_AMMO_DATA
{
	HUD_AMMO_DATA_CLIPAMMO = 0, // number of bullets in the clip
	HUD_AMMO_DATA_CLIPCOUNT,	// number of clips in the primary bullet pool; -1 if clipsize is 1
	HUD_AMMO_DATA_CLIPSIZE,		// max rounds in each clip; 1 for single round fire types like grenades or satchels
	HUD_AMMO_DATA_OVERSUPPLY,	// number of clips or rounds in oversupply, if clipsize is > 1, its clips, otherwise its rounds
	HUD_AMMO_DATA_RGAMMO,		// number of RG rounds; -1 if no cup is attached or not a rg rifle
	HUD_AMMO_DATA_RGLOADED,		// do we have an rg round loaded; -1 is not applicable
	HUD_AMMO_DATA_CLIPTYPE,		// type of clip we use; clips, magazines, belts, etc..
	HUD_AMMO_DATA_RANGE			// current weapon's range setting
};

class CSAFE_fpFPSWeaponData
{
public:
	CSAFE_fpFPSWeaponData()
	{
		clear();
	}

	~CSAFE_fpFPSWeaponData(){}

	void clear()
	{
		memset(this, 0, sizeof(CSAFE_fpFPSWeaponData));
	}

	int getSightDist(int position)
	{
		return (numSightDists) ? startSightDists + (incSightDists * position): 0;
	}

	int getSightDist()
	{
		return getSightDist(currentSightNotch);
	}

	FLOAT32				muzzleRise;				// degrees of rise per bullet
	FLOAT32				muzzleReturn;			// degrees of return per physics frame
	FLOAT32				muzzleClimbRate;

	FLOAT64				bulletJitter;
	FLOAT64				horizJitterMult;
	FLOAT32				speedMultiplier;		// measure of how controllable gun is while moving

	fpFPSWeaponType		weaponType;				// type of weapon (determines animation states)

	FLOAT32				firepointDisplacement;	// round displacement from firepoint

	FLOAT32				firepointZDistance;     // z distance of firepoint when firepoint is not in the model
	FLOAT32             firepointHorizOffset;   // horizontal offset from eyepoint when firepoint is not in the model
	FLOAT32             firepointVertOffset;    // vertical offset from eyepoint when firepoint is not in the model
	FLOAT64             grenadeVisDelay;        // The amount of time that the grenade appears in the hand at toss.

	FLOAT32				fireDelay;				// how long to wait after pressing the fire button to actually shoot
	FLOAT32				minimumDispersion;		// minimum dispersion, in degrees, of a human firing this weapon
	FLOAT32				unaimedDispersionMod;	// dispersion mod for unaimed firing
	FLOAT32				controlability;			// how controllable the gun is .. direct multiplier on dispersion
	FLOAT32				aimFOV;

	SCHAR*				reloadSound;
	SCHAR*				reload2Sound;
	SCHAR*				reload3Sound;
	SCHAR*				recycleSound;
	SCHAR*				rgReloadSound;
	SCHAR*				deploySound;
	SCHAR*				undeploySound;

	SCHAR*				weaponModel;
	SCHAR*				weaponConnectTo;
	SCHAR*				weaponConnect;

	SCHAR*				clipModel;
	SCHAR*				clipConnectTo;
	SCHAR*				clipConnect;

	// these are meshes that are hidden on load
	// these hidden meshes are unhidden via events
	// when used in various animations
	// an example is the rifle grenade round, it only
	// needs to be visible during the RG round reload
	// animations
	// its a common delimited list of sub-mesh names
	SCHAR*				hiddenMeshes;

	// auxilliary models are those that are attached, but have no independent animations
	// currently just the rifle grenade cup, but should be extended to the clips

	SCHAR*				auxModel[FPS_NUM_AUX_MODELS];
	SCHAR*				auxConnectTo[FPS_NUM_AUX_MODELS];
	SCHAR*				auxConnect[FPS_NUM_AUX_MODELS];
	SCHAR*				auxConnectToAlt[FPS_NUM_AUX_MODELS];
	FLOAT32				auxAltConnectTime[FPS_NUM_AUX_MODELS];
	FLOAT32				auxAltDisconnectTime[FPS_NUM_AUX_MODELS];

	SINT32              grenadeDelayTime; // in ms

	FLOAT32				aimVerticalCorrection;
	FLOAT32				aimHorizontalCorrection;

	FLOAT32				convergenceDistance;

	fpFPSIconType		hudIconType;

	SINT32				holdfire;				// tells the control manager to allow the fire button to be held down
												// required for autofire weapons as well as recycling weapons

	FLOAT32				ejectDelay;				// for rifles only, how long after a recycle/reload animation starts before ejecting a shell
	FLOAT32				shellScale;

	FLOAT32				weaponMoveMultiplier;
	FLOAT32				weaponHeartMultiplier;
	FLOAT32				weaponBreathMultiplier;
	FLOAT32				weaponPoseMultiplier;

	SCHAR*				deployPivotBone;		// bone around which to pivot if deployed (nullptr if no pivot)

	SINT32				rgFiringRules;			// enable special rules for kneeling rifle grenade firing
	FLOAT32				rgFiringAngle;			// angle that the gun is pitched at

	// rg up/down angle limits
	FLOAT32				weapDeployUpAngle[FPS_NUM_POSITIONS];
	FLOAT32				weapDeployDownAngle[FPS_NUM_POSITIONS];
	FLOAT32				weapDeployKneelDeployUpAngle;
	FLOAT32				weapDeployKneelDeployDownAngle;

	SINT32				numSightDists;
	SINT32				startSightDists;
	SINT32				incSightDists;
	SINT32				currentSightNotch;
	FLOAT32				sightAdjustTime;
	SCHAR*				sightAdjustSound;

	FLOAT32				dryFireSoundLength;
	SCHAR*				dryFireSound;

	// snipper data

	SCHAR*               sniperGunSight;       // The file that has the sniper gunsight
	FLOAT64				sniperScopeDelay;      // The delay that the gunsight needs before it appers.
	FLOAT64             sniperScopeEndDelay;    // Added delay for the scope to come down if needed.
	LOGICAL             sniperScope:1;         // 1 if this is a sniper wepon

	// live data
	LOGICAL				auxAvailable[FPS_NUM_AUX_MODELS];
	LOGICAL				auxEnable[FPS_NUM_AUX_MODELS];

	LOGICAL				ejectImmediate:1;		// says whether to eject the shells immediately or to use the ..
	LOGICAL				onlyFireDeployed:1;		// 1 if weapon requires deployment for fire
	LOGICAL				leanFireOK:1;
	LOGICAL				riseStopsReload:1;		// any muzzle rise stops reloads & recycles
	LOGICAL				staticDeploy:1;			// 1 if trooper can't move while weapon is deployed
	LOGICAL				canAim:1;

	LOGICAL				placedObject;				// is this weapon type the build tool

private:
	CSAFE_fpFPSWeaponData(const CSAFE_fpFPSWeaponData&);			// Prevent copy ctor
	CSAFE_fpFPSWeaponData& operator=(const CSAFE_fpFPSWeaponData&);	// Prevent copy operator
};

class fpFPSWeaponData : public CSAFE_fpFPSWeaponData
{
public:
	fpFPSWeaponData()
	{
		clear();
	}

	~fpFPSWeaponData(){}

	void clear();

public:
	Covert<SINT32>		clipBullets;			// number of bullets in current clip
	Covert<FLOAT32>		convergenceAngle;
	Covert<LOGICAL>		suppressAim;
	Covert<FLOAT32>		perBulletATP;			// ATP drain per bullet
	Covert<FLOAT32>		delayAfterFire;			// how long after the weapon is fired to delay reload & recycle animations
};

// "C" safe trooper data
class CSAFE_fpFPSTrooperData {
public:
	CSAFE_fpFPSTrooperData() {
		clear();
	}
	~CSAFE_fpFPSTrooperData() {}

	void clear() {
		memset(this, 0, sizeof(CSAFE_fpFPSTrooperData));
	}

public:
	// preloaded data
	// eye height controls
	FLOAT64				mouseGranularity;
	FLOAT32				vertGranularityMultiplier;
	FLOAT32				deployedGranularityMultiplier;

	//FLOAT32				transitionTimes[FPS_NUM_POSITIONS][FPS_NUM_POSITIONS];
	FLOAT32				eyeHeight[FPS_NUM_EYEHEIGHTS];

	// fov adjust speed
	FLOAT32				FOVSpeed;

	// per bullet error multiplier
	FLOAT32				aimControl;

	// riding controls
	FLOAT32				maxSafeVehicleSpeed;
	FLOAT32				killVehicleSpeed;
	FLOAT32				unrideJoules;
	// weapon change timer
	FLOAT32				weaponChangeTime;
	// eye angle controls
	FLOAT64				maxHorizEyeAngle;
	FLOAT64				minEyeMovement;
	FLOAT64				eyeMoveMultiplier;
	FLOAT32				maxEyeUpAngle;
	FLOAT32				maxEyeDownAngle;
	FLOAT32				maxRidingEyeUpAngle;
	FLOAT32				maxRidingEyeDownAngle;

	FLOAT32				movementSpeeds[FPS_NUM_SPEEDS];

	FLOAT32				atpMinDispersionATP;

	FLOAT64				maxRotationSpeed;
	FLOAT64				keyRotationSpeed;

	FLOAT32				eyeRideDisplacement;

	FLOAT32				swimDisplacement;

	// gun wobble data
	FLOAT32				moveMultiplier;
	FLOAT32				heartMultiplier;
	FLOAT32				breathMultiplier;
	FLOAT32				poseMultiplier;

	FLOAT32				restPulse;
	FLOAT32				maxPulse;

	FLOAT32				maxHeartBreathMultiplier;
	FLOAT32				timeToMaxHeartBreathMultiplier;
	FLOAT32				maxPoseMultiplier;
	FLOAT32				timeToMaxPoseMultiplier;
	FLOAT32				rejuvTimeMultiplier;
	FLOAT32				pulseBreathRatio;
	FLOAT32				standPoseWobbleMultiplier;
	FLOAT32				kneelPoseWobbleMultiplier;
	FLOAT32				pronePoseWobbleMultiplier;
	FLOAT32				minArmWobbleTime;
	FLOAT32				maxArmWobbleTime;
	FLOAT32				aimBreathTime;		// time to get to zero
	FLOAT32				aimStartValue;		// what to start at

	FLOAT32				horizWobbleAngleMultiplier;
	FLOAT32				vertWobbleAngleMultiplier;
	FLOAT32				maxWobbleAngle;

	FLOAT32				fallShortTime, fallFarTime;

	// deploy data
	FLOAT32				maxProneDeployedUpAngle;
	FLOAT32				maxProneDeployedDownAngle;
	FLOAT32				maxProneDeployedLeftAngle;
	FLOAT32				maxProneDeployedRightAngle;
	FLOAT32				proneDeployedClimbMultiplier;
	FLOAT32				proneDeployedHorizShiftMultiplier;
	FLOAT32				proneDeployedVertShiftMultiplier;

	// sound files
	SCHAR*				moveSounds[FPS_MAX_SOUNDS];
	SCHAR*				aimBreathSound;
	SCHAR*				fallShortSound;
	SCHAR*				fallFarSound;

	// lean movements
	FLOAT32				leanLeftHoriz, leanLeftVert;
	FLOAT32				leanRightHoriz, leanRightVert;
	FLOAT32				leanTime, leanMinSpeed, leanMaxSpeed;

	// bridge specials
	FLOAT64				bridgeDamageJoules, bridgeRepairJoules;

	FLOAT32				redScreenFadeTime;
	FLOAT32				redScreenMinAlpha;

	// paratroop data
	FLOAT32				parachuteDeployTime;
	FLOAT32				parachuteWornATPMultiplier;
	FLOAT32				paratroopATPModifier;
	FLOAT32				parachuteFallSpeed;
	FLOAT32				minSafeFallDistance;
	SCHAR*				parachuteOpeningSound;
	SCHAR*				paratroopLandingSound;
	SCHAR*				paratroopLandingWaterSound;

	// live data
	FLOAT32				currentMuzzleRise, targetMuzzleRise;
	fpsWeaponTypes		weaponPositionModel[MAX_NUM_FPS_WEAPONS];
	SINT32				activeWeaponNumber, weaponOfLastResort, autoswitchWeaponNumber, nextWeaponNumber, lastActiveWeaponNumber;
	SINT32				highlightedWeaponNumber;
	FLOAT64				highlightTime;
	FLOAT64				eyeHorizOffset;
	FLOAT64				eyeVertOffset;
	SINT32				lastUnrideTime;

	// tps fire flags
	SINT32				tpsStartAttack;
	SINT32				tpsAttack;

	// when we last fired
	SINT32				lastFireCheckTime;

	SINT32				movementSound;			// Enum for the actual files
	SINT32				movementSoundID;		// The actual FX Sound Handle
	SINT32				autoWeaponFireSoundId;	// The actual FX Sound Handle
	SINT32              breathSoundID;

	SINT32				currentSurfaceID;

	FLOAT64				leanVertTarget, leanHorizTarget, leanHorizCurrent, leanVertCurrent;

	SINT32				fireTimeForDelay;

	struct trnCDTerrainObject* lastTO;
	SINT32				lastTODamageLevel;

	LOGICAL				teleportToGround;

	LOGICAL				sniperScope;
	FLOAT64				sniperScopeDelay;

	FLOAT32				parachuteDecelerationTime;
	FLOAT64VECTOR		inheritedVehicleVelocity;
	FLOAT64VECTOR		pushBackVelocity;
	FLOAT32				nextDriftTime;

	FLOAT32				overSupplyRadius;

	// are we wading (only used in custom terrains)
	// it is the depth of water we are wading in
	FLOAT32				wading;

	// are exiting a river?
	// used to determine which cage collider to use
	LOGICAL				riverExit;
};

class fpFPSTrooperData : public CSAFE_fpFPSTrooperData {
public:
	fpFPSTrooperData() : CSAFE_fpFPSTrooperData() {
		clear();
	}
	~fpFPSTrooperData() {}

	void clear();

public:
	Covert<LOGICAL>		initialized;
	Covert<LOGICAL>		binocsEnabled;
	Covert<LOGICAL>		isParatroop;
	Covert<LOGICAL>		parachuteWorn;
	Covert<LOGICAL>		parachuteOpen;
	Covert<LOGICAL>		parachuteOKToOpen;
	Covert<LOGICAL>		readingMap;
	Covert<LOGICAL>		autoReload;

	Covert<FLOAT32>		forwardAccel;			// (m/s2)
	Covert<FLOAT32>		strafeAccel;			// (m/s2)

	Covert<FLOAT32>		atpStandStill;
	Covert<FLOAT32>		atpStandForward;
	Covert<FLOAT32>		atpStandForwardWalk;
	Covert<FLOAT32>		atpStandBackward;
	Covert<FLOAT32>		atpStandBackwardWalk;
	Covert<FLOAT32>		atpStandStillRelaxed;
	Covert<FLOAT32>		atpStandForwardRelaxed;
	Covert<FLOAT32>		atpStandForwardWalkRelaxed;
	Covert<FLOAT32>		atpStandBackwardRelaxed;
	Covert<FLOAT32>		atpStandBackwardWalkRelaxed;
	Covert<FLOAT32>		atpStandSprint;
	Covert<FLOAT32>		atpCrouchStill;
	Covert<FLOAT32>		atpCrouchForward;
	Covert<FLOAT32>		atpCrouchStillRelaxed;
	Covert<FLOAT32>		atpCrouchForwardRelaxed;
	Covert<FLOAT32>		atpProneStill;
	Covert<FLOAT32>		atpProneForward;
	Covert<FLOAT32>		atpSwimStill;
	Covert<FLOAT32>		atpSwimForward;
	Covert<FLOAT32>		atpSitStill;

	Covert<FLOAT32>		atpSprintMinimum;
	Covert<FLOAT32>		atpSprintAllowAgain;
	Covert<FLOAT32>		atpCreepMinimum;
	Covert<FLOAT32>		atpCrawlMinimum;
	Covert<FLOAT32>		atpSwimMinimum;
	Covert<FLOAT32>		atpBackwardMinimum;

	Covert<FLOAT32>		concussionMod;

	Covert<LOGICAL>		doGravity; // make sure terrain is loaded

	Covert<FLOAT32>		defaultFOV;
	Covert<FLOAT32>		desiredFOV;
};

struct fpFPSLoadoutWeapon
{
	fpFPSLoadoutWeapon() : weapon(-1), index(-1), lastResort(false), aux(false)
	{
	}

	int weapon;
	int index;
	bool lastResort;
	bool aux;

};

struct fpFPSTrooperLoadout
{
	fpFPSTrooperLoadout() : id(""), arms(-1), isPara(false), atpMultiplier(1.0f)
	{
	}

	std::string id;
	int arms;
	bool isPara;
	float atpMultiplier;
	std::vector<fpFPSLoadoutWeapon> weapons;

};

// prototypes
void					fpFPSDataInit();
void					fpFPSDataReadFPSFile();
fpFPSTrooperLoadout*	fpFPSGetTrooperLoadout(const char* id);
LOGICAL					fpFPSGetHUDAmmoData(HUD_AMMO_DATA type, SINT32* data);
UINT32					fpFPSDataGetVarnameAndContents(SCHAR ** varname, SCHAR ** varcontents, SCHAR * inputString);

// data
extern SCHAR*				skel_model;
extern SCHAR*				arm_models[NUM_INFANTRY_TYPES];
extern SCHAR*				leg_models[NUM_INFANTRY_TYPES];

extern SCHAR*				arms_anim_files[NUM_WEAPONS][NUM_WEAP_STATES];
extern SCHAR*				weap_anim_files[NUM_WEAPONS][NUM_WEAP_STATES];
extern SCHAR*				clip_anim_files[NUM_WEAPONS][NUM_WEAP_STATES];
extern SCHAR*				weap_default[NUM_WEAPONS];
extern SCHAR*				weap_empty_default[NUM_WEAPONS];
extern SCHAR*				clip_default[NUM_WEAPONS];
extern SCHAR*				replacement_arms_file;
extern SCHAR*				default_arms_tex_names[NUM_INFANTRY_TYPES];
extern SCHAR*				replacement_arms_tex_names[NUM_INFANTRY_TYPES];

extern fpFPSTrooperData		fpsTrooper;
extern fpFPSWeaponData		fpsWeapons[NUM_WEAPONS];
