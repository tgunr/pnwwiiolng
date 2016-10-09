#include <syscfg.h> // Path is project specific.

#include "pi/pi.h"

#include "fio/fio.h"

#include "veh/vehmain.h"
#include "veh/vehcntrl.h"
#include "veh/vehanim.h"
#include "ml/mlutility.h"
#include "ml/mltypes.h"
#include "ml/mlhead.h"
#include "pers/persmain.h"
#include "pi/pi.h"
#include "fp/fpMain.h"
#include "pers/pershud.h"
#include "pers/persgunsight.h"
#include "pers/persbombsight.h"
#include "imap/imap.h"
#include "imap/ww2func.h"
#include "uimap/uiMap.h"
#include "fp/fpFPS.h"
#include "ui/uiChat.h"
#include "tut/TutorialManager.h"
#include "ui/uiDotCommands.h"
#include "scen/ScenarioManager.h"
#include "ez/EZManager.h"

struct vehCntrlCommandListElement
{
	UCHAR CommandName[32];
	UINT32 CommandId;
	imapControlCallback Control;
	UINT32 IsAxis;
	UINT32 ControlLossEffect;

	// next parameters have to be last
	UINT32 Enabled[ML_MAX_CREW_POSITIONS];
	UINT32 ExecutionTime;
};

UINT32 _ProcessCallback(SINT32 InParameter,UINT32 Id,UINT32 Detach);
static vehCntrlCommandListElement* _LookupElement(UINT32 Id);

vehCntrlCommandListElement vehCntrlCommandList[] =
{
	{"gearhandle",						IMAP_MOM_GEARHANDLE,							mlToggleGear,					VEH_CNTRL_ISNOTAXIS,		VEH_CONTROL_DISABLE},
//	{"mixtureoff",						IMAP_MOM_MIXTURE_OFF,							mlMixtureOff,					VEH_CNTRL_ISNOTAXIS,		VEH_CONTROL_DISABLE},
//	{"mixturelean",						IMAP_MOM_MIXTURE_LEAN,							mlMixtureLean,					VEH_CNTRL_ISNOTAXIS,		VEH_CONTROL_DISABLE},
//	{"mixturerich",						IMAP_MOM_MIXTURE_RICH,							mlMixtureRich,					VEH_CNTRL_ISNOTAXIS,		VEH_CONTROL_DISABLE},
	{"supercharger1",					IMAP_MOM_SUPERCHARGER_STAGE1,					nullptr,							VEH_CNTRL_ISNOTAXIS,		VEH_CONTROL_DISABLE},
	{"supercharger2",					IMAP_MOM_SUPERCHARGER_STAGE2,					nullptr,							VEH_CNTRL_ISNOTAXIS,		VEH_CONTROL_DISABLE},
	{"superchargern",					IMAP_MOM_SUPERCHARGER_NEUTRAL,					nullptr,							VEH_CNTRL_ISNOTAXIS,		VEH_CONTROL_DISABLE},
//	{"llswitch",						IMAP_MOM_LLSWITCH,								mlToggleLandingLight,			VEH_CNTRL_ISNOTAXIS,		VEH_CONTROL_DISABLE},
//	{"fuelcock",						IMAP_MOM_FUELCOCK_NEXTTANK,						mlCycleFuelCock,				VEH_CNTRL_ISNOTAXIS,		VEH_CONTROL_DISABLE},
//	{"cowlflaps",						IMAP_MOM_COWLFLAPS,								mlToggleCowlFlaps,				VEH_CNTRL_ISNOTAXIS,		VEH_CONTROL_DISABLE},
//	{"panellight",						IMAP_MOM_PANELLIGHT,							mlTogglePanelLight,				VEH_CNTRL_ISNOTAXIS,		VEH_CONTROL_DISABLE},
//	{"auxsight",						IMAP_MOM_AUXSIGHT,								mlToggleAuxSight,				VEH_CNTRL_ISNOTAXIS,		VEH_CONTROL_DISABLE},
//	{"radio",							IMAP_MOM_RADIO,									nullptr,							VEH_CNTRL_ISNOTAXIS,		VEH_CONTROL_UNAFFECTED},
	{"hatchcontrol",					IMAP_MOM_HATCHCONTROL,							mlToggleHatch,					VEH_CNTRL_ISNOTAXIS,		VEH_CONTROL_DISABLE},
	{"canopycontrol",					IMAP_MOM_CANOPYCONTROL,							mlToggleCanopy,					VEH_CNTRL_ISNOTAXIS,		VEH_CONTROL_DISABLE},
	{"ordtrigger",						IMAP_MOM_ORDTRIGGER,							nullptr,							VEH_CNTRL_ISNOTAXIS,		VEH_CONTROL_DISABLE},
	{"ordselector",						IMAP_MOM_ORDSELECTOR,							nullptr,							VEH_CNTRL_ISNOTAXIS,		VEH_CONTROL_DISABLE},
	{"divebrakes",						IMAP_MOM_DEPLOYDIVEBRAKES,						mlToggleDiveBrakes,				VEH_CNTRL_ISNOTAXIS,		VEH_CONTROL_DISABLE},
	{"autodive",						IMAP_MOM_AUTODIVE,								nullptr,							VEH_CNTRL_ISNOTAXIS,		VEH_CONTROL_DISABLE},
	{"map",								IMAP_MOM_MAP,									nullptr,							VEH_CNTRL_ISNOTAXIS,		VEH_CONTROL_UNAFFECTED},
	{"aileron",							IMAP_ASX_ROLL,									mlAdjustStickX,					VEH_CNTRL_ISAXIS,			VEH_CONTROL_REDUCE_BY_STRENGTH},
	{"elevator",						IMAP_ASX_PITCH,									mlAdjustStickY,					VEH_CNTRL_ISAXIS,			VEH_CONTROL_REDUCE_BY_STRENGTH},
	{"rudder",							IMAP_ASX_YAW,									mlAdjustStickZ,					VEH_CNTRL_ISAXIS,			VEH_CONTROL_REDUCE_BY_STRENGTH},
	{"throttle",						IMAP_ASX_THROTTLE,								mlAdjustThrottle,				VEH_CNTRL_ISAXIS,			VEH_CONTROL_DISABLE},
	{"winch",							IMAP_ASX_WINCH,									mlAdjustWinch,					VEH_CNTRL_ISAXIS,			VEH_CONTROL_DISABLE},
	{"gas",								IMAP_ASX_GAS,									mlAdjustGas,					VEH_CNTRL_ISAXIS,			VEH_CONTROL_AXIS_GOTONEG},
	{"clutch",							IMAP_ASX_CLUTCH,								mlAdjustClutch,					VEH_CNTRL_ISAXIS,			VEH_CONTROL_AXIS_GOTONEG},
	{"lefttrackclutch",					IMAP_ASX_LEFTTRACKCLUTCH,						mlLeftTrackClutchFunction,		VEH_CNTRL_ISAXIS},
	{"righttrackclutch",				IMAP_ASX_RIGHTTRACKCLUTCH,						mlRightTrackClutchFunction,		VEH_CNTRL_ISAXIS},
	{"flapcontrol",						IMAP_ASX_FLAPCONTROL,							mlAdjustFlapPosition,			VEH_CNTRL_ISAXIS,			VEH_CONTROL_DISABLE},
	{"flapexecute",						IMAP_MOM_FLAPEXECUTE,							mlMoveFlapSetting,				VEH_CNTRL_ISAXIS,			VEH_CONTROL_DISABLE},
	{"enginerpm",						IMAP_ASX_RPMLEVER,								mlRpmStubFunction,				VEH_CNTRL_ISAXIS,			VEH_CONTROL_DISABLE},
	{"primarytrigger",					IMAP_HOLD_MAINTRIGGER,							mlPrimaryTriggerDown,			VEH_CNTRL_ISHOLDDOWN,		VEH_CONTROL_DISABLE},
	{"secondarytrigger",				IMAP_HOLD_SECONDTRIGGER,						mlSecondaryTriggerDown,			VEH_CNTRL_ISHOLDDOWN,		VEH_CONTROL_DISABLE},
	{"leftbrake",						IMAP_ASX_LEFTBRAKE,								mlLeftBrakeFunction,			VEH_CNTRL_ISAXIS,			VEH_CONTROL_AXIS_GOTONEG},
	{"rightbrake",						IMAP_ASX_RIGHTBRAKE,							mlRightBrakeFunction,			VEH_CNTRL_ISAXIS,			VEH_CONTROL_AXIS_GOTONEG},
	{"bothbrake",						IMAP_ASX_CENTERBRAKE,							mlCenterBrakeFunction,			VEH_CNTRL_ISAXIS,			VEH_CONTROL_AXIS_GOTONEG},
	{"steering",						IMAP_ASX_STEERING,								mlSteeringFunction,				VEH_CNTRL_ISAXIS,			VEH_CONTROL_DISABLE},
	{"engstart",						IMAP_MOM_STARTER_ON,							mlStarterIsDepressed,			VEH_CNTRL_ISHOLDDOWN,		VEH_CONTROL_DISABLE},
	{"taillock",						IMAP_MOM_TOGGLE_TAILLOCK,						mlToggleTailWheelLock,			VEH_CNTRL_ISNOTAXIS,		VEH_CONTROL_DISABLE},
	{"shiftup",							IMAP_MOM_SHIFTUP,								mlShiftUp,						VEH_CNTRL_ISNOTAXIS,		VEH_CONTROL_DISABLE},
	{"shiftdown",						IMAP_MOM_SHIFTDOWN,								mlShiftDown,					VEH_CNTRL_ISNOTAXIS,		VEH_CONTROL_DISABLE},
	{"reverse",							IMAP_MOM_REVERSE,								mlReverse,						VEH_CNTRL_ISNOTAXIS,		VEH_CONTROL_DISABLE},
	{"togglewepmode",					IMAP_MOM_TOGGLEWEP,								mlToggleWepMode,				VEH_CNTRL_ISHOLDDOWN,		VEH_CONTROL_DISABLE},
	{"elevtrim",						IMAP_ASX_ELEV_TRIM,								mlAdjustElevTrim,				VEH_CNTRL_ISAXIS,			VEH_CONTROL_DISABLE},
	{"ailerontrim",						IMAP_ASX_AILERON_TRIM,							mlAdjustAileronTrim,			VEH_CNTRL_ISAXIS,			VEH_CONTROL_DISABLE},
	{"ruddertrim",						IMAP_ASX_RUDDER_TRIM,							mlAdjustRudderTrim,				VEH_CNTRL_ISAXIS,			VEH_CONTROL_DISABLE},
// turretraverse misspell
	{"turretraverse",					IMAP_ASX_TURRET_TRAVERSE,						mlTraverseTurretSpeed,			VEH_CNTRL_ISAXIS,			VEH_CONTROL_AXIS_GOTONEUTRAL},
	{"turretelevate",					IMAP_ASX_TURRET_ELEVATE,						mlElevateTurretSpeed,			VEH_CNTRL_ISAXIS,			VEH_CONTROL_AXIS_GOTONEUTRAL},
	{"turretraversesecondary",			IMAP_ASX_TURRET_TRAVERSE_SECONDARY,				mlTraverseTurretSpeed,			VEH_CNTRL_ISAXIS,			VEH_CONTROL_AXIS_GOTONEUTRAL},
	{"turretelevatesecondary",			IMAP_ASX_TURRET_ELEVATE_SECONDARY,				mlElevateTurretSpeed,			VEH_CNTRL_ISAXIS,			VEH_CONTROL_AXIS_GOTONEUTRAL},
	{"adjustpropup",					IMAP_MOM_ADJUST_PROP_UP,						mlPropPitchAdjust,				VEH_CNTRL_ISHOLDDOWN,		VEH_CONTROL_DISABLE},
	{"adjustpropdown",					IMAP_MOM_ADJUST_PROP_DOWN,						mlPropPitchAdjust,				VEH_CNTRL_ISHOLDDOWN,		VEH_CONTROL_DISABLE},
	{"atgunleftright",					IMAP_ASX_ATGUN_MOVEMENT_LEFTRIGHT,				mlATGunLeftRight,				VEH_CNTRL_ISAXIS,			VEH_CONTROL_AXIS_GOTONEUTRAL},
	{"atgunforwardbackwards",			IMAP_ASX_ATGUN_MOVEMENT_FORWARDBACKWARDS,		mlATGunForwardBackwards,		VEH_CNTRL_ISAXIS,			VEH_CONTROL_AXIS_GOTONEUTRAL},
	{"bombbaydoor",						IMAP_MOM_TOGGLE_BOMBBAYDOOR,					mlToggleBombBayDoor,			VEH_CNTRL_ISNOTAXIS,		VEH_CONTROL_DISABLE},
	{"deployweapon",					IMAP_MOM_TOGGLE_DEPLOYWEAPON,					mlToggleBombBayDoor,			VEH_CNTRL_ISNOTAXIS,		VEH_CONTROL_DISABLE},
	{"towbar",							IMAP_MOM_TOGGLE_TOWBAR,							mlToggleTowbar,					VEH_CNTRL_ISNOTAXIS,		VEH_CONTROL_DISABLE},
	{"ridetransport",					IMAP_RIDE_TRANSPORT,							mlRideTransport,				VEH_CNTRL_ISNOTAXIS,		VEH_CONTROL_DISABLE},
	{"stukasiren",						IMAP_MOM_STUKASIREN,							mlToggleStukaSiren,				VEH_CNTRL_ISNOTAXIS,		VEH_CONTROL_DISABLE},
	{"cycleammo",						IMAP_TMP_CYCLE_AMMO,							mlCycleAmmo,					VEH_CNTRL_ISNOTAXIS,		VEH_CONTROL_DISABLE},
	{"rangefind",						IMAP_TMP_RANGEFIND,								mlRangeFind,					VEH_CNTRL_ISNOTAXIS,		VEH_CONTROL_DISABLE},
	{"gunsightrangeinc",				IMAP_HOLD_GUNSIGHT_RANGE_INC,					persGunsightRangeInc,			VEH_CNTRL_ISHOLDDOWN,		VEH_CONTROL_DISABLE},
	{"gunsightrangedec",				IMAP_HOLD_GUNSIGHT_RANGE_DEC,					persGunsightRangeDec,			VEH_CNTRL_ISHOLDDOWN,		VEH_CONTROL_DISABLE},

	{"gunsightconvinc",					IMAP_GUNSIGHT_CONVERGENCE_INC,					persGunsightConvergenceInc,		VEH_CNTRL_ISHOLDDOWN,		VEH_CONTROL_DISABLE},
	{"gunsightconvdec",					IMAP_GUNSIGHT_CONVERGENCE_DEC,					persGunsightConvergenceDec,		VEH_CNTRL_ISHOLDDOWN,		VEH_CONTROL_DISABLE},
	{"bombsightspeedinc",				IMAP_HOLD_BOMBSIGHT_SPEED_INC,					persBombsightSpeedInc,			VEH_CNTRL_ISHOLDDOWN,		VEH_CONTROL_DISABLE},
	{"bombsightspeeddec",				IMAP_HOLD_BOMBSIGHT_SPEED_DEC,					persBombsightSpeedDec,			VEH_CNTRL_ISHOLDDOWN,		VEH_CONTROL_DISABLE},
	{"bombsightaltinc",					IMAP_HOLD_BOMBSIGHT_ALT_INC,					persBombsightAltInc,			VEH_CNTRL_ISHOLDDOWN,		VEH_CONTROL_DISABLE},
	{"bombsightaltdec",					IMAP_HOLD_BOMBSIGHT_ALT_DEC,					persBombsightAltDec,			VEH_CNTRL_ISHOLDDOWN,		VEH_CONTROL_DISABLE},
	{"targetspeedinc",					IMAP_TARGET_SPEED_INC,							persGunsightTargetSpeedInc,		VEH_CNTRL_ISHOLDDOWN,		VEH_CONTROL_DISABLE},
	{"targetspeeddec",					IMAP_TARGET_SPEED_DEC,							persGunsightTargetSpeedDec,		VEH_CNTRL_ISHOLDDOWN,		VEH_CONTROL_DISABLE},
	{"targetangleinc",					IMAP_TARGET_ANGLE_OFF_INC,						persGunsightTargetAngleInc,		VEH_CNTRL_ISHOLDDOWN,		VEH_CONTROL_DISABLE},
	{"targetangledec",					IMAP_TARGET_ANGLE_OFF_DEC,						persGunsightTargetAngleDec,		VEH_CNTRL_ISHOLDDOWN,		VEH_CONTROL_DISABLE},
	{"targetmarkbearing",				IMAP_TARGET_MARK_BEARING,						persGunsightTargetMarkBearing,	VEH_CNTRL_ISHOLDDOWN,		VEH_CONTROL_DISABLE},
	{"targetcalcsolution",				IMAP_TARGET_CALC_SOLUTION,						persGunsightCalcTargetSolution,	VEH_CNTRL_ISHOLDDOWN,		VEH_CONTROL_DISABLE},
	{"torpedodepthinc",					IMAP_TORPEDO_DEPTH_INCREASE,					persGunsightTorpedoDepthInc,	VEH_CNTRL_ISHOLDDOWN,		VEH_CONTROL_DISABLE},
	{"torpedodepthdec",					IMAP_TORPEDO_DEPTH_DECREASE,					persGunsightTorpedoDepthDec,	VEH_CNTRL_ISHOLDDOWN,		VEH_CONTROL_DISABLE},

	{"jettison",						IMAP_JETTISON_ORDNANCE,							mlJettisonOrdnance,				VEH_CNTRL_ISHOLDDOWN,		VEH_CONTROL_DISABLE},
	{"autopilot",						IMAP_AUTOPILOT,									mlAutopilot,				    VEH_CNTRL_ISHOLDDOWN,		VEH_CONTROL_DISABLE},
    {"mobilespawn",                     IMAP_MOBILE_SPAWN,                              mlToggleMobileSpawn,            VEH_CNTRL_ISNOTAXIS,		VEH_CONTROL_DISABLE},

	// FP Begin
	// Pose Functions
	{"fpcrouch",						IMAP_HOLD_FP_CROUCH,							fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN},
	{"fpfast",							IMAP_HOLD_FP_SPRINT,							fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN},
	{"fpautorun",						IMAP_HOLD_FP_AUTORUN,							fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN},
	{"fprelaxed",						IMAP_HOLD_FP_RELAXED,							fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN},
	{"fpwalk",							IMAP_HOLD_FP_WALK,								fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN},
	{"fpprone",							IMAP_HOLD_FP_PRONE,								fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN},

	// Movement Functions
	{"fpforward",						IMAP_HOLD_FP_FORWARD,							fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN},
	{"fpbackward",						IMAP_HOLD_FP_BACKWARD,							fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN},
	{"fpturnright",						IMAP_HOLD_FP_TURN_RIGHT,						fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN},
	{"fpturnleft",						IMAP_HOLD_FP_TURN_LEFT,							fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN},
	{"fpstraferight",					IMAP_HOLD_FP_STRAFE_RIGHT,						fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN},
	{"fpstrafeleft",					IMAP_HOLD_FP_STRAFE_LEFT,						fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN},
	{"fpleanright",						IMAP_HOLD_FP_LEAN_RIGHT,						fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN},
	{"fpleanleft",						IMAP_HOLD_FP_LEAN_LEFT,							fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN},

	// Action Functions
	{"fpgunsight",						IMAP_HOLD_FP_GUNSIGHT,							fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN},
	{"fpfire",							IMAP_HOLD_FP_FIRE,								fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN},
	{"fplookleft",						IMAP_HOLD_FP_LOOK_LEFT,							fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN},
	{"fplookright",						IMAP_HOLD_FP_LOOK_RIGHT,						fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN},
	{"fpreload",						IMAP_HOLD_FP_RELOAD,							fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN},
	{"fpdeploy",						IMAP_HOLD_FP_DEPLOY,							fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN},

	// Weapon Selection Functions
	{"fpweapon0",						IMAP_HOLD_FP_WEAPON_1,							fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN},
	{"fpweapon1",						IMAP_HOLD_FP_WEAPON_2,							fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN},
	{"fpweapon2",						IMAP_HOLD_FP_WEAPON_3,							fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN},
	{"fpweapon3",						IMAP_HOLD_FP_WEAPON_4,							fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN},
	{"fpweapon4",						IMAP_HOLD_FP_WEAPON_5,							fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN},
	{"fpweapon5",						IMAP_HOLD_FP_WEAPON_6,							fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN},
	{"fpweapon6",						IMAP_HOLD_FP_WEAPON_7,							fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN},
	{"fpweapon7",						IMAP_HOLD_FP_WEAPON_8,							fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN},
	{"fpweapon8",						IMAP_HOLD_FP_WEAPON_9,							fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN},
	{"fpweapon9",						IMAP_HOLD_FP_WEAPON_10,							fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN},

	{"fpnextweapon",					IMAP_HOLD_FP_NEXT_WEAPON,						fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN},
	{"fpprevweapon",					IMAP_HOLD_FP_PREV_WEAPON,						fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN},

	// combat emotes
	{"fpcedouble",						IMAP_HOLD_FP_CE_DOUBLE,							fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN},
	{"fpcegetdown",						IMAP_HOLD_FP_CE_GETDOWN,						fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN},
	{"fpcehalt",						IMAP_HOLD_FP_CE_HALT,							fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN},
	{"fpcemoveout",						IMAP_HOLD_FP_CE_MOVEOUT,						fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN},
	{"fpceammo",						IMAP_HOLD_FP_CE_AMMO,							fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN},
	{"fpcejump",						IMAP_HOLD_FP_CE_JUMP,							fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN},

	{"togglemouselook",					IMAP_TOGGLE_MOUSE_LOOK,							nullptr,							VEH_CNTRL_ISHOLDDOWN},
	{"mouselookx",						IMAP_MOUSE_LOOK_X,								nullptr,							VEH_CNTRL_ISAXIS},
	{"mouselooky",						IMAP_MOUSE_LOOK_Y,								nullptr,							VEH_CNTRL_ISAXIS},
	{"mouselookz",						IMAP_MOUSE_LOOK_Z,								nullptr,							VEH_CNTRL_ISAXIS},

	{"fptorsoverticalaxis",				IMAP_ASX_FP_TORSO_VERTICAL_AXIS,				fpFPSControlInput,				VEH_CNTRL_ISAXIS},
	{"fptorsohorizontalaxis",			IMAP_ASX_FP_TORSO_HORIZONTAL_AXIS,				fpFPSControlInput,				VEH_CNTRL_ISAXIS},
	{"fprideunridevehicle",				IMAP_MOM_FP_RIDE_UNRIDE_VEHICLE,				fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN},
	{"lookviewone",						IMAP_LOOK_FORWARD,								nullptr,							VEH_CNTRL_IS_SPECIAL},
	{"lookviewtwo",						IMAP_LOOK_BACK,									nullptr,							VEH_CNTRL_IS_SPECIAL},
	{"lookviewthree",					IMAP_LOOK_UP,									nullptr,							VEH_CNTRL_IS_SPECIAL},
	{"lookviewfour",					IMAP_LOOK_DOWN,									nullptr,							VEH_CNTRL_IS_SPECIAL},
	{"lookviewfive",					IMAP_LOOK_FORWARD_RIGHT,						nullptr,							VEH_CNTRL_IS_SPECIAL},
	{"lookviewsix",						IMAP_LOOK_FORWARD_LEFT,							nullptr,							VEH_CNTRL_IS_SPECIAL},
	{"lookviewseven",					IMAP_LOOK_RIGHT,								nullptr,							VEH_CNTRL_IS_SPECIAL},
	{"lookvieweight",					IMAP_LOOK_LEFT,									nullptr,							VEH_CNTRL_IS_SPECIAL},
	{"lookviewnine",					IMAP_LOOK_BACK_RIGHT,							nullptr,							VEH_CNTRL_IS_SPECIAL},
	{"lookviewten",						IMAP_LOOK_BACK_LEFT,							nullptr,							VEH_CNTRL_IS_SPECIAL},
	{"lookvieweleven",					IMAP_LOOK_FORWARD_UP,							nullptr,							VEH_CNTRL_IS_SPECIAL},
	{"lookviewtwelve",					IMAP_LOOK_FORWARD_RIGHT_UP,						nullptr,							VEH_CNTRL_IS_SPECIAL},
	{"lookviewthirteen",				IMAP_LOOK_FORWARD_LEFT_UP,						nullptr,							VEH_CNTRL_IS_SPECIAL},
	{"lookviewfourteen",				IMAP_LOOK_RIGHT_UP,								nullptr,							VEH_CNTRL_IS_SPECIAL},
	{"lookviewfifteen",					IMAP_LOOK_LEFT_UP,								nullptr,							VEH_CNTRL_IS_SPECIAL},
	{"lookviewsixteen",					IMAP_LOOK_BACK_RIGHT_UP,						nullptr,							VEH_CNTRL_IS_SPECIAL},
	{"lookviewseventeen",				IMAP_LOOK_BACK_LEFT_UP,							nullptr,							VEH_CNTRL_IS_SPECIAL},
	{"lookvieweightteen",				IMAP_LOOK_FORWARD_DOWN,							nullptr,							VEH_CNTRL_IS_SPECIAL},
	{"lookviewnineteen",				IMAP_LOOK_FORWARD_RIGHT_DOWN,					nullptr,							VEH_CNTRL_IS_SPECIAL},
	{"lookviewtwenty",					IMAP_LOOK_FORWARD_LEFT_DOWN,					nullptr,							VEH_CNTRL_IS_SPECIAL},
	{"lookviewtwentyone",				IMAP_LOOK_RIGHT_DOWN,							nullptr,							VEH_CNTRL_IS_SPECIAL},
	{"lookviewtwentytwo",				IMAP_LOOK_LEFT_DOWN,							nullptr,							VEH_CNTRL_IS_SPECIAL},
	{"lookviewtwentythree",				IMAP_LOOK_BACK_RIGHT_DOWN,						nullptr,							VEH_CNTRL_IS_SPECIAL},
	{"lookviewtwentyfour",				IMAP_LOOK_BACK_LEFT_DOWN,						nullptr,							VEH_CNTRL_IS_SPECIAL},
	{"gunsightview",					IMAP_TMP_GUNSIGHT_VIEW,							nullptr,							VEH_CNTRL_IS_SPECIAL},
	{"instrumentview",					IMAP_TMP_INSTRUMENT_VIEW,						nullptr,							VEH_CNTRL_IS_SPECIAL},
	{"positionone",						IMAP_MOM_POSITION_1,							nullptr,							VEH_CNTRL_IS_SPECIAL},
	{"positiontwo",						IMAP_MOM_POSITION_2,							nullptr,							VEH_CNTRL_IS_SPECIAL},
	{"positionthree",					IMAP_MOM_POSITION_3,							nullptr,							VEH_CNTRL_IS_SPECIAL},
	{"positionfour",					IMAP_MOM_POSITION_4,							nullptr,							VEH_CNTRL_IS_SPECIAL},
	{"positionfive",					IMAP_MOM_POSITION_5,							nullptr,							VEH_CNTRL_IS_SPECIAL},
	{"positionsix",						IMAP_MOM_POSITION_6,							nullptr,							VEH_CNTRL_IS_SPECIAL},
	{"positionseven",					IMAP_MOM_POSITION_7,							nullptr,							VEH_CNTRL_IS_SPECIAL},
	{"positioneight",					IMAP_MOM_POSITION_8,							nullptr,							VEH_CNTRL_IS_SPECIAL},
	{"positionnine",					IMAP_MOM_POSITION_9,							nullptr,							VEH_CNTRL_IS_SPECIAL},
	{"positionten",						IMAP_MOM_POSITION_10,							nullptr,							VEH_CNTRL_IS_SPECIAL},
	{"showgui",							IMAP_MOM_SHOW_GUI,								nullptr,							VEH_CNTRL_IS_SPECIAL},
	{"guicursor",						IMAP_MOM_SHOW_GUI_CURSOR,						nullptr,							VEH_CNTRL_IS_SPECIAL},
	{"guichatwindow",					IMAP_MOM_SHOW_GUI_CHAT_WINDOW,					nullptr,							VEH_CNTRL_IS_SPECIAL},
	{"showradiobuffer",					IMAP_MOM_SHOW_RADIO_BUFFER,						nullptr,							VEH_CNTRL_IS_SPECIAL},
	{"showingamemap",					IMAP_MOM_SHOW_IN_GAME_MAP,						nullptr,							VEH_CNTRL_IS_SPECIAL},
	{"changehudmode",					IMAP_MOM_TOGGLE_HUD,							nullptr,							VEH_CNTRL_IS_SPECIAL},
	{"resethud",						IMAP_MOM_RESET_HUD,								nullptr,							VEH_CNTRL_IS_SPECIAL},
	{"changechatheight",				IMAP_MOM_TOGGLE_CHATHEIGHT,						nullptr,							VEH_CNTRL_IS_SPECIAL},

	{"externallocked",					IMAP_MOM_EXTERNAL_LOCKED,						nullptr,							VEH_CNTRL_IS_SPECIAL},
	{"externalrubberband",				IMAP_MOM_EXTERNAL_RUBBERBAND,					nullptr,							VEH_CNTRL_IS_SPECIAL},
	{"externalfixed",					IMAP_MOM_EXTERNAL_FIXED,						nullptr,							VEH_CNTRL_IS_SPECIAL},
	{"setfixedcamera",					IMAP_MOM_SET_FIXED_CAMERA,						nullptr,							VEH_CNTRL_IS_SPECIAL},
	{"slewviewleft",					IMAP_HOLD_SLEWVIEWLEFT,							nullptr,							VEH_CNTRL_IS_SPECIAL},
	{"slewviewright",					IMAP_HOLD_SLEWVIEWRIGHT,						nullptr,							VEH_CNTRL_IS_SPECIAL},
	{"slewviewup",						IMAP_HOLD_SLEWVIEWUP,							nullptr,							VEH_CNTRL_IS_SPECIAL},
	{"slewviewdown",					IMAP_HOLD_SLEWVIEWDOWN,							nullptr,							VEH_CNTRL_IS_SPECIAL},
	{"slewviewin",						IMAP_HOLD_SLEWVIEWIN,							nullptr,							VEH_CNTRL_IS_SPECIAL},
	{"slewviewout",					IMAP_HOLD_SLEWVIEWOUT,							nullptr,							VEH_CNTRL_IS_SPECIAL},
	{"rollviewright",					IMAP_HOLD_ROLLVIEWRIGHT,						nullptr,							VEH_CNTRL_IS_SPECIAL},
	{"rollviewleft",					IMAP_HOLD_ROLLVIEWLEFT,							nullptr,							VEH_CNTRL_IS_SPECIAL},
	{"changefov",						IMAP_TMP_CHANGE_FOV,							nullptr,							VEH_CNTRL_IS_SPECIAL},
	{"toggleinternalexternalviews",		IMAP_MOM_TOGGLE_INTERNAL_EXTERNAL_VIEWS,		nullptr,							VEH_CNTRL_IS_SPECIAL},

    {"clonevehicle",		IMAP_CLONE_VEHICLE,	vehmainCloneVehicle,			VEH_CNTRL_ISHOLDDOWN},

	{"ezmove",				IMAP_EZ_MOVE,		nullptr,							VEH_CNTRL_ISAXIS},
	{"ezturn",				IMAP_EZ_TURN,		nullptr,							VEH_CNTRL_ISAXIS},
	{"ezaction1",			IMAP_EZ_ACTION1,	nullptr,							VEH_CNTRL_ISHOLDDOWN},
	{"ezaction2",			IMAP_EZ_ACTION2,	nullptr,							VEH_CNTRL_ISHOLDDOWN},
	{"ezfreelook",			IMAP_EZ_FREELOOK,	nullptr,							VEH_CNTRL_ISHOLDDOWN},
	{"ezcamerax",			IMAP_EZ_CAMERA_X,	nullptr,							VEH_CNTRL_ISAXIS},
	{"ezcameray",			IMAP_EZ_CAMERA_Y,	nullptr,							VEH_CNTRL_ISAXIS},
	{"eznextpos",			IMAP_EZ_NEXTPOS,	nullptr,							VEH_CNTRL_ISHOLDDOWN},
	{"ezprevpos",			IMAP_EZ_PREVPOS,	nullptr,							VEH_CNTRL_ISHOLDDOWN},
	{"ezengine",			IMAP_EZ_ENGINE,		nullptr,							VEH_CNTRL_ISHOLDDOWN},

	{"",								0,												nullptr,							0} // "null-terminated",

};

SINT32 vehcntrlGetFunctionIndex(SCHAR* functionName)
{
UINT32 count;

	if(functionName)
	{
		for(count = 0; vehCntrlCommandList[count].CommandId != IMAP_NO_FUNCTION; count++)
		{
			if((strcmp((char*)functionName, (char*)vehCntrlCommandList[count].CommandName)) == 0)
			{
				return vehCntrlCommandList[count].CommandId;
			}
		}
	}

	return -1;
}

UINT32 vehCntrlInstallControlCallbacks(void)
{
UINT32 i;

	for (i = 0; vehCntrlCommandList[i].CommandId != IMAP_NO_FUNCTION; i++)
	{
		if (vehCntrlCommandList[i].IsAxis != VEH_CNTRL_IS_SPECIAL)
		{
			imapInstallControlCallback(vehCntrlCommandList[i].CommandId,
										_ProcessCallback);
		}
	}

	if(!EZ_MGR)
	{
		imapInstallControlCallback(IMAP_TOGGLE_MOUSE_LOOK,	 	mlToggleMouseLook);
		imapInstallControlCallback(IMAP_MOUSE_LOOK_X, mlMouseLook);
		imapInstallControlCallback(IMAP_MOUSE_LOOK_Y, mlMouseLook);
		imapInstallControlCallback(IMAP_MOUSE_LOOK_Z, mlMouseLook);

		imapInstallControlCallback(IMAP_LOOK_FORWARD,	 		mlSetHeadViewCallback);
		imapInstallControlCallback(IMAP_LOOK_BACK,	 			mlSetHeadViewCallback);
		imapInstallControlCallback(IMAP_LOOK_UP,  				mlSetHeadViewCallback);
		imapInstallControlCallback(IMAP_LOOK_DOWN,  			mlSetHeadViewCallback);
		imapInstallControlCallback(IMAP_LOOK_FORWARD_RIGHT,  	mlSetHeadViewCallback);
		imapInstallControlCallback(IMAP_LOOK_FORWARD_LEFT, 	 	mlSetHeadViewCallback);
		imapInstallControlCallback(IMAP_LOOK_RIGHT,  			mlSetHeadViewCallback);
		imapInstallControlCallback(IMAP_LOOK_LEFT,  			mlSetHeadViewCallback);
		imapInstallControlCallback(IMAP_LOOK_BACK_RIGHT,  		mlSetHeadViewCallback);
		imapInstallControlCallback(IMAP_LOOK_BACK_LEFT, 		mlSetHeadViewCallback);
		imapInstallControlCallback(IMAP_LOOK_FORWARD_UP, 		mlSetHeadViewCallback);
		imapInstallControlCallback(IMAP_LOOK_FORWARD_RIGHT_UP, 	mlSetHeadViewCallback);
		imapInstallControlCallback(IMAP_LOOK_FORWARD_LEFT_UP, 	mlSetHeadViewCallback);
		imapInstallControlCallback(IMAP_LOOK_RIGHT_UP, 			mlSetHeadViewCallback);
		imapInstallControlCallback(IMAP_LOOK_LEFT_UP, 			mlSetHeadViewCallback);
		imapInstallControlCallback(IMAP_LOOK_BACK_RIGHT_UP, 	mlSetHeadViewCallback);
		imapInstallControlCallback(IMAP_LOOK_BACK_LEFT_UP, 		mlSetHeadViewCallback);
		imapInstallControlCallback(IMAP_LOOK_FORWARD_DOWN,	 	mlSetHeadViewCallback);
		imapInstallControlCallback(IMAP_LOOK_FORWARD_RIGHT_DOWN,mlSetHeadViewCallback);
		imapInstallControlCallback(IMAP_LOOK_FORWARD_LEFT_DOWN, mlSetHeadViewCallback);
		imapInstallControlCallback(IMAP_LOOK_RIGHT_DOWN, 		mlSetHeadViewCallback);
		imapInstallControlCallback(IMAP_LOOK_LEFT_DOWN, 		mlSetHeadViewCallback);
		imapInstallControlCallback(IMAP_LOOK_BACK_RIGHT_DOWN, 	mlSetHeadViewCallback);
		imapInstallControlCallback(IMAP_LOOK_BACK_LEFT_DOWN, 	mlSetHeadViewCallback);

		imapInstallControlCallback(IMAP_MOM_POSITION_1,			persSetPersonaPositionCallback);
		imapInstallControlCallback(IMAP_MOM_POSITION_2,			persSetPersonaPositionCallback);
		imapInstallControlCallback(IMAP_MOM_POSITION_3,			persSetPersonaPositionCallback);
		imapInstallControlCallback(IMAP_MOM_POSITION_4, 		persSetPersonaPositionCallback);
		imapInstallControlCallback(IMAP_MOM_POSITION_5,			persSetPersonaPositionCallback);
		imapInstallControlCallback(IMAP_MOM_POSITION_6,			persSetPersonaPositionCallback);
		imapInstallControlCallback(IMAP_MOM_POSITION_7,			persSetPersonaPositionCallback);
		imapInstallControlCallback(IMAP_MOM_POSITION_8,			persSetPersonaPositionCallback);
		imapInstallControlCallback(IMAP_MOM_POSITION_9,			persSetPersonaPositionCallback);
		imapInstallControlCallback(IMAP_MOM_POSITION_10,		persSetPersonaPositionCallback);
	}

	imapInstallControlCallback(IMAP_TMP_EXIT_VEHICLE,		vehExitVehicleTrigger);
	imapInstallControlCallback(IMAP_MOM_TOGGLE_HUD,			persHUDToggleHUD);
	imapInstallControlCallback(IMAP_MOM_RESET_HUD,			persHUDResetHUD);
	imapInstallControlCallback(IMAP_MOM_TOGGLE_CHATHEIGHT,	uiChatToggleHeight);
	imapInstallControlCallback(IMAP_MOM_SHOW_IN_GAME_MAP, 	uiMapToggleIngameMap);
	imapInstallControlCallback(IMAP_SHOW_STATS, 			persHUDToggleReadout);

	if(TUT_ACTIVE && !SCEN_MGR->IsActive())
		imapInstallControlCallback(IMAP_CONTINUE_TUTORIAL, TutorialManager::Input);
	else if(!Online)
		imapInstallControlCallback(IMAP_CLONE_VEHICLE, vehmainCloneVehicle);

	return 0;
}

UINT32 vehCntrlInit(void)
{
	vehCntrlInstallControlCallbacks();

	return 0;
}

UINT32 vehcntrlDisableAllControls(void)
{
	// disable all controls
	for (UINT32 i = 0; vehCntrlCommandList[i].CommandId != IMAP_NO_FUNCTION; i++)
	{
		for (UINT32 j = 0; j < ML_MAX_CREW_POSITIONS; j++)
			vehCntrlCommandList[i].Enabled[j] = 0;

		vehCntrlCommandList[i].ExecutionTime = 0;
	}

	return 0;
}

UINT32 vehcntrlBailout(void)
{
	for (UINT32 i = 0; vehCntrlCommandList[i].CommandId != IMAP_NO_FUNCTION; i++)
	{
		for (UINT32 j = 0; j < ML_MAX_CREW_POSITIONS; j++)
		{
			if(vehCntrlCommandList[i].Enabled[j] && vehCntrlCommandList[i].Control)
			{
				switch(vehCntrlCommandList[i].CommandId)
				{
					case IMAP_ASX_ROLL:
					case IMAP_ASX_PITCH:
						vehCntrlCommandList[i].Control(0,vehCntrlCommandList[i].CommandId,0);
					break;
				}
			}
		}
	}

	vehcntrlDisableAllControls();

	return 0;
}

UINT32 vehCntrlReadControlFile(vehType * VehicleType,UINT32 Position)
{
SINT32 ControlFile;
UINT32 i,j;
UCHAR Buffer[256];
UINT32 FoundPosition;
UCHAR * VarName;
UCHAR * VarContents;
UCHAR ControlFilename[128];

	if (VehicleType == nullptr)
	{
		return 1;
	}

	strcpy((char*)ControlFilename,"vehicles\\");
	strcat((char*)ControlFilename,(char*)VehicleType->ModelDirectory);
	strcat((char*)ControlFilename,"\\");
	strcat((char*)ControlFilename,(char*)VehicleType->ControlFile);

	// disable all controls
	for (i = 0; vehCntrlCommandList[i].CommandId != IMAP_NO_FUNCTION; i++)
	{
		for (j=0;j<ML_MAX_CREW_POSITIONS;j++)
		{
			vehCntrlCommandList[i].Enabled[j] = 0;
		}
		vehCntrlCommandList[i].ExecutionTime = 0;
	}

	ControlFile = fioOpen((char*)ControlFilename);
	if (ControlFile == -1)
	{
		return 1;
	}

	FoundPosition = 0;

	// first locate the particular position
	while(fioGetString(ControlFile,(char*)Buffer,255) != 0)
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

		if (strcmp((char*)VarName,"position") == 0)
		{
				FoundPosition = 1;
				Position = atoi((char*)VarContents);
				break;
		}
	}

	if (FoundPosition == 0)
	{
		fioClose(ControlFile);
		return 1;
	}

	// ok read in enable list
	while(fioGetString(ControlFile,(char*)Buffer,255) != 0)
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

		if (strcmp((char*)VarName,"position") == 0)
		{
			Position = atoi((char*)VarContents);
			continue;
		}

		if(gEZMode && EZ_MGR && VehicleType->Type != VEH_TYPE_PERSON)
		{
			if(!EZ_MGR->EasyControl(Position, (const char*)VarContents))
				continue;
		}
		else
		{
			if(strcmp((char*)VarName,"control") != 0 && strcmp((char*)VarName,"ezcontrol") != 0)
			{
				continue;
			}
		}

		// locate string
		for (i = 0; vehCntrlCommandList[i].CommandId != IMAP_NO_FUNCTION; i++)
		{
			if (strcmp((char*)vehCntrlCommandList[i].CommandName,(char*)VarContents) == 0)
			{
				vehCntrlCommandList[i].Enabled[Position-1] = 1;
				break;
			}
		}

	}

	fioClose(ControlFile);

	return 0;
}

UINT32 _ProcessCallback(SINT32 InParameter,UINT32 Id,UINT32 Detach)
{
	vehCntrlCommandListElement* Element = _LookupElement(Id);
	if(Element == nullptr) return 0;

	UINT32 Position;
	FLOAT32 Strength;
	LOGICAL ControlsDisabled;

	SINT32 Parameter = InParameter;
	// first check to see if the IK system has "taken over" this function
	// if it's an axis function for the IK system we still execute it
	// this is to allow axis operations to trigger
//	vehVehicle* Vehicle = persGetPersonaVehiclePtr();
	persGetPersonaPosition(&Position);
	persGetControlStatus(&Strength,&ControlsDisabled);

	if(EZ_MGR && EZ_MGR->InputBlock(Id))
		return 0;

	if (Element->Enabled[Position] == 0)
	{
		return 0;
	}
	if (Element->Control == nullptr)
	{
		return 0;
	}

	if (ControlsDisabled)
	{
		if (Element->ControlLossEffect == VEH_CONTROL_DISABLE &&
			Parameter == 1)
		{
			return 0;
		}

		if (Element->IsAxis == VEH_CNTRL_ISAXIS)
		{
			if (Element->ControlLossEffect == VEH_CONTROL_AXIS_GOTONEUTRAL)
			{
				Parameter = 0;
			}
			if (Element->ControlLossEffect == VEH_CONTROL_AXIS_GOTOPOS)
			{
				Parameter = 1000;
			}
			if (Element->ControlLossEffect == VEH_CONTROL_AXIS_GOTONEG)
			{
				Parameter = -1000;
			}
			if (Element->ControlLossEffect == VEH_CONTROL_REDUCE_BY_STRENGTH)
			{
				Parameter = 0;
			}
			if (Element->ControlLossEffect == VEH_CONTROL_DISABLE)
			{
				return 0;
			}
		}
	}

	if (Element->ControlLossEffect == VEH_CONTROL_REDUCE_BY_STRENGTH)
	{
		Parameter = (SINT32)(Parameter * Strength + 0.5f);
	}

	if (Element->IsAxis == VEH_CNTRL_ISNOTAXIS)
	{
		if (Parameter == TRUE)
		{
			int p = 1;
			TUT_MGR->DoEvent(TUT_EVENT_INPUT, &Id, &p);
			Element->Control(0,Id,Detach);
		}
	}
	else if (Element->IsAxis == VEH_CNTRL_ISAXIS || Element->IsAxis == VEH_CNTRL_ISHOLDDOWN)
	{
		TUT_MGR->DoEvent(TUT_EVENT_INPUT, &Id, &Parameter);
		Element->Control(Parameter,Id,Detach);
	}

	return 0;
}

UINT32 vehIkTriggeredFunction(UINT32 Id,UINT32 Time)
{
	vehCntrlCommandListElement* Element;

	Element = _LookupElement(Id);

	if (Element == nullptr || Element->Control == nullptr)
	{
		return 0;
	}

	if (Element->IsAxis != VEH_CNTRL_ISNOTAXIS)
	{
		Element->Control(0,Id,0);
	}

	Element->Control(Time,Id,0);

	return 0;
}

UINT32 vehCntrlGetOperationNumber(UINT32* OperationNum,UCHAR * Name)
{
	UINT32 i;

	if (Name == nullptr)
	{
		return 0;
	}


	*OperationNum = 0;

	for (i = 0; vehCntrlCommandList[i].CommandId != IMAP_NO_FUNCTION; i++)
	{
		if (strcmp((char*)vehCntrlCommandList[i].CommandName,(char*)Name) == 0)
		{
			*OperationNum = vehCntrlCommandList[i].CommandId;
			break;
		}
	}

	return 0;
}

UINT32 vehCntrlEnableFunction(UINT32 Id)
{
	return 0;
}

UINT32 vehCntrlDisableFunction(UINT32 Id)
{
	return 0;
}

vehCntrlCommandListElement* _LookupElement(UINT32 Id)
{
	int i;

	for(i = 0; vehCntrlCommandList[i].CommandId != IMAP_NO_FUNCTION; i++)
	{
		if(vehCntrlCommandList[i].CommandId == Id) return &vehCntrlCommandList[i];
	}
	return nullptr;
}

