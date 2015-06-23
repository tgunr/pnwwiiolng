#pragma once
// all the internal defines, prototypes, and so on that are the FPS sytem
//
// externally useable stuff goes in fpFPS.h

// compile flags
#define	GUN_WOBBLE				1
//#define AIMING_WEAPONS			1
//#define	TESTING_FPS				1
#if AIMING_WEAPONS
#undef GUN_WOBBLE
#endif

#include "fp/fpFPS.h"
#include "fp/fpFPSdata.h"
//
// data
//

// this is the base height used .. changing this changes ALL heights, both
// eye and arms.  Default should be 1.7f
#define	TROOPER_STAND_EYE_HEIGHT	1.7f
#define MIN_MOVE_DIFFERENCE			0.01
#define	BINOCS_WEAPON_NUMBER		9
#define BUILDTOOL_WEAPON_NUMBER		8
#define MAP_WEAPON_NUMBER			7
#define GrannyForever				0

//this is some improved state stuff for some weapons

#define LMG_DEPLOY_SPEED_FACTOR    1.75f

// these are the states for the keys
#define	FPS_SPRINT			0x00000001	// holddown
#define	FPS_CROUCH			0x00000002	// toggle
#define	FPS_PRONE			0x00000004	// toggle
#define	FPS_FORWARD			0x00000008	// holddown
#define	FPS_BACKWARD		0x00000010	// holddown
#define	FPS_STRAFE_RIGHT	0x00000020	// holddown
#define	FPS_STRAFE_LEFT		0x00000040	// holddown
#define	FPS_TURN_RIGHT		0x00000080	// holddown
#define	FPS_TURN_LEFT		0x00000100	// holddown
#define	FPS_AIM				0x00000200	// holddown
#define	FPS_FIRE			0x00000400	// holddown
#define	FPS_LOOK_LEFT		0x00000800	// holddown
#define	FPS_LOOK_RIGHT		0x00001000	// holddown
#define	FPS_AUTORUN			0x00002000	// special
#define	FPS_RELAXED			0x00004000	// toggle
#define	FPS_WALK			0x00008000	// toggle
#define	FPS_DEPLOY			0x00010000	// toggle
#define	FPS_LEAN_LEFT		0x00020000	// holddown	(Deprecated after 1.33)
#define	FPS_LEAN_RIGHT		0x00040000	// holddown	(Deprecated after 1.33)
#define	FPS_SIGHT_UP		0x00080000	// holddown
#define	FPS_SIGHT_DOWN		0x00100000	// holddown

// this is used to turn all these off when riding a vehicle
#define RIDE_MASK			(FPS_SPRINT | FPS_CROUCH | FPS_PRONE | FPS_FORWARD | \
							FPS_BACKWARD | FPS_STRAFE_RIGHT | FPS_STRAFE_LEFT | \
							FPS_TURN_RIGHT | FPS_TURN_LEFT | \
							FPS_AUTORUN | FPS_RELAXED )


// these are the flags used by fpFPSControlInput to actually make state changes
#define	STATE_MOVE_STILL			0x00000001
#define	STATE_MOVE_SLOW				0x00000002
#define	STATE_MOVE_FAST				0x00000004
#define	STATE_MOVE_BACKWARD			0x00000008
#define	STATE_EYE_STAND				0x00000010
#define	STATE_EYE_CROUCH			0x00000020
#define	STATE_EYE_PRONE				0x00000040
#define	STATE_STRAFE_LEFT			0x00000080	// both strafes zero = no strafe
#define	STATE_STRAFE_RIGHT			0x00000100	// both strafes zero = no strafe
#define	STATE_TURN_RIGHT			0x00000200	// both turns zero = no turn
#define	STATE_TURN_LEFT				0x00000400	// both turns zero = no turn
#define	STATE_LOOK_RIGHT			0x00000800	// both looks zero = no look
#define	STATE_LOOK_LEFT				0x00001000	// both looks zero = no look
#define	STATE_AIM					0x00002000
#define	STATE_FIRE					0x00004000
#define	STATE_DEAD					0x00008000	// am I dead?
#define	STATE_NEEDS_RECYCLE			0x00010000	// weapon needs a recycle to be played next
#define	STATE_HOLD_FIRE				0x00020000	// don't fire immediately
#define	STATE_NO_ARM_SWAY			0x00040000	// prevent arm sway (set every frame)
#define	STATE_SIT					0x00080000
#define	STATE_NO_MOVEMENT			0x00100000
#define	STATE_FIRE_DELAYED_WEAP		0x00200000
#define	STATE_WEAPON_SWITCHING		0x00400000
#define	STATE_SWIMMING				0x00800000
#define	STATE_RELAXED				0x01000000
#define	STATE_WALK					0x02000000
#define	STATE_RELOADING				0x04000000
#define	STATE_EMOTE_PLAYING			0x08000000
#define	STATE_KNIFE_ATTACKING		0x10000000
#define	STATE_DEPLOYED				0x20000000
#define	STATE_DEPLOYING				0x40000000

#define	STAT2_LEAN_RIGHT			0x00000001
#define	STAT2_LEAN_LEFT				0x00000002
#define	STAT2_NEEDS_RELOAD			0x00000004	// only used for weapons with riseStopsReload or delayAfterFire .. signals a reload event MUST occur
#define	STAT2_STANDING_ON_VEHICLE	0x00000008
#define	STAT2_POSE_CHANGE			0x00000010	// are we pose changing
#define	STAT2_TURNING_LEFT			0x00000020	// when turning left
#define	STAT2_TURNING_RIGHT			0x00000040	// when turning right
#define	STAT2_HEAD_FIXED				0x00000080	// lock our view forward?

#define STATE_ANIMATION_MASK		(STATE_MOVE_STILL | STATE_MOVE_SLOW | STATE_MOVE_FAST | STATE_MOVE_BACKWARD | \
									STATE_EYE_STAND | STATE_EYE_CROUCH | STATE_EYE_PRONE | \
									STATE_AIM | \
									STATE_FIRE )

#define	STATE_MOVEMENT_MASK			(STATE_MOVE_STILL | STATE_MOVE_SLOW | STATE_MOVE_FAST | STATE_MOVE_BACKWARD | \
									STATE_EYE_STAND | STATE_EYE_CROUCH | STATE_EYE_PRONE | STATE_AIM | \
									STATE_STRAFE_LEFT | STATE_STRAFE_RIGHT )


#define	STATE_EYE_MASK				(STATE_EYE_STAND | STATE_EYE_CROUCH | STATE_EYE_PRONE )

#define	STATE_STRAFE_MASK			(STATE_STRAFE_LEFT | STATE_STRAFE_RIGHT )

// these are persistent states .. the stuff not in here is reset every frame based on the controls by the control handler
#define STATE_KEEPERS_MASK			(STATE_DEAD | STATE_NEEDS_RECYCLE | STATE_HOLD_FIRE | \
									STATE_SIT | STATE_NO_MOVEMENT | \
									STATE_FIRE_DELAYED_WEAP | \
									STATE_WEAPON_SWITCHING | \
									STATE_RELOADING | \
									STATE_SWIMMING | \
									STATE_EMOTE_PLAYING | \
									STATE_KNIFE_ATTACKING | \
									STATE_DEPLOYING | \
									STATE_DEPLOYED )

#define	STAT2_KEEPERS_MASK			(STAT2_NEEDS_RELOAD | STAT2_STANDING_ON_VEHICLE | STAT2_POSE_CHANGE)

// any of these will prevent firing
#define	STATE_NOFIRE_MASK			(STATE_DEAD | STATE_NEEDS_RECYCLE | STATE_HOLD_FIRE | \
									STATE_SIT | STATE_NO_MOVEMENT | \
									STATE_FIRE_DELAYED_WEAP | \
									STATE_WEAPON_SWITCHING )

// any of these will prevent relaxed
#define	STATE_NORELAX_MASK			(STATE_DEAD | STATE_NEEDS_RECYCLE | STATE_HOLD_FIRE | \
									STATE_SIT | STATE_NO_MOVEMENT | \
									STATE_FIRE_DELAYED_WEAP | \
									STATE_WEAPON_SWITCHING | \
									STATE_EYE_PRONE | STATE_MOVE_FAST | STATE_AIM  )

#define	STATE_NORELOAD_MASK			(STATE_DEAD | STATE_WEAPON_SWITCHING | STATE_EMOTE_PLAYING | STATE_DEPLOYING)

#define	STATE_NOSPRINT_MASK			(STATE_DEAD | STATE_WEAPON_SWITCHING | STATE_EMOTE_PLAYING | STATE_DEPLOYING | STATE_RELOADING)

#define	STATE_NOLEAN_MASK			(STATE_DEAD | STATE_WEAPON_SWITCHING | STATE_EMOTE_PLAYING | STATE_DEPLOYING | STATE_RELOADING | \
									 STATE_MOVE_SLOW | STATE_MOVE_FAST | STATE_MOVE_BACKWARD | \
									 STATE_SIT | STATE_NO_MOVEMENT | STATE_EYE_PRONE | \
									 STATE_STRAFE_LEFT | STATE_STRAFE_RIGHT )

#define STATE_IMPLICIT_RELAXED_MASK	(STATE_MOVE_FAST | STATE_SIT | STATE_SWIMMING)

#define	STATE_DEFAULT_MASK			(STATE_MOVE_STILL|STATE_EYE_STAND)

#define	STILL			0
#define	SLOW			1
#define	FAST			2
#define	BACKWARD		3
#define	NUM_MOVESTATES	4
#define	EYE_STAND		0
#define	EYE_CROUCH		1
#define	EYE_PRONE		2
#define	STRAFE_NONE		0
#define	STRAFE_RIGHT	1
#define	STRAFE_LEFT		2
#define	TURN_NONE		0
#define	TURN_RIGHT		1
#define	TURN_LEFT		2
#define	AIM_OFF			0
#define	AIM_ON			1
#define	LOOK_AHEAD		0
#define	LOOK_LEFT		1
#define	LOOK_RIGHT		2
#define	LEAN_NONE		0
#define	LEAN_LEFT		1
#define	LEAN_RIGHT		2
#define	FIRE_OFF		0
#define	FIRE_ON			1

//
// variables
//

extern SINT32 gr2AnimWeapBits, gr2AnimPoseBits, gr2AnimMoveBits, gr2AnimPointBits, gr2AnimHeadBits;
extern SINT32 gr2AnimDeathBits, gr2AnimAltWeapBits, gr2AnimParachuteBits, gr2AnimStateBits; // gr2AnimEmoteBits

//
// prototypes
//
extern void				fpFPSInit (vehVehicle*vehicle);
extern LOGICAL				fpFPSLoadModels (vehVehicle*vehicle);
extern LOGICAL				fpFPSAttachWeapon (fpsWeaponTypes weapon);
extern fpsWeaponStates		fpFPSDetermineNextState ();
extern LOGICAL				fpFPSCleanupAnimationState ();
extern LOGICAL				fpFPSStartAnimationState (fpsWeaponTypes weapon, fpsWeaponStates state, FLOAT32 easeIn = -1, FLOAT32 easeOut = -1);
extern void				fpFPSMoveModel ();
extern float				fpFPSGetHeight ();
extern LOGICAL			fpFPSHandleControls ();
extern void				fpFPSRecycle ();
extern void				fpFPSForcedReload ();
extern void				fpFPSManualReload ();
extern LOGICAL				fpFPSWeaponStateCheck (fpsWeaponStates* newState);
extern void				fpFPSWeaponChange (SINT32 weaponNumber, SINT32 override);
extern UINT32				fpFPSCheckNewPose(fpFPSPoses poseToCheck);
extern LOGICAL				fpFPSRideUnrideVehicle ();
extern LOGICAL				fpFPSSit(SINT32 Param, vehVehicle* rideVehicle = nullptr);
extern LOGICAL				fpFPSIsWeaponSwitchOK (SINT32 weaponNumber);
extern void				fpFPSWeaponAutoSwitch();
extern LOGICAL				fpFPSGetFirePoint (OBJECT* pObject, piMATRIX* pMatrix, bool visual);
extern void                fpFPSResetBuilderPosition();
extern UINT32				fpFPSTestBuilderValidity();
extern void                fpFPSBuilderEnd();
extern LOGICAL				fpFPSStateOKToFire();
extern void				fpFPSForceShellEjection();
extern void				fpFPSGetWobble(FLOAT32* horizWobble, FLOAT32* vertWobble, FLOAT32* horizAngle, FLOAT32* vertAngle);

extern LOGICAL				fpsCheckDeploy(bool doShake = true);
extern void				fpFPSDeployChange();

static void			fpFPSAxisRotation (SINT32 param, UINT32 id);
static void			fpFPSSetVerticalDeployedPosition (FLOAT64 newPos);
static void			fpFPSSetHorizontalDeployedPosition (FLOAT64 newPos);
static void			fpFPSVerticalAxisLook (SINT32 param, UINT32 data);
static void			fpFPSHorizAxisLook (SINT32 param, UINT32 data);

extern LOGICAL				fpFPSWeaponOutOfAmmo(weapWeapon* weapon);
extern LOGICAL				fpFPSWeaponOutOfSecondaryAmmo(weapWeapon* weapon);

extern void				fpFPSGetEyeUpDownRange (FLOAT32*	upAngle, FLOAT32* downAngle);
static void			fpFPSAdjustAngles (FLOAT32 deltaTime, FLOAT64 minSpeed, FLOAT64 maxSpeed, FLOAT64* xAngle, FLOAT64 xTarget);

extern void				fpFPSSightAdjust (SINT32 sightParam);
extern void				fpFPSParadropStart();
extern LOGICAL				fpFPSOkToJumpEmote();

extern void				fpFPSIterateHighlightedWeapon(SINT32 direction);
