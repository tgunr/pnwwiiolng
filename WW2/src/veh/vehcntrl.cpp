#include "syscfg.h" // Path is project specific.

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
#include "imaplib/imap.h"
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

#define VCEID  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 0 // VEH_CONTROL_END_INITIALIZER_DATA
vehCntrlCommandListElement vehCntrlCommandList[] =
{
	{"gearhandle",						IMAP_MOM_GEARHANDLE,						mlToggleGear,					VEH_CNTRL_ISNOTAXIS,	VEH_CONTROL_DISABLE, VCEID},
//	{"mixtureoff",						IMAP_MOM_MIXTURE_OFF,						mlMixtureOff,					VEH_CNTRL_ISNOTAXIS,	VEH_CONTROL_DISABLE, VCEID},
//	{"mixturelean",						IMAP_MOM_MIXTURE_LEAN,						mlMixtureLean,					VEH_CNTRL_ISNOTAXIS,	VEH_CONTROL_DISABLE, VCEID},
//	{"mixturerich",						IMAP_MOM_MIXTURE_RICH,						mlMixtureRich,					VEH_CNTRL_ISNOTAXIS,	VEH_CONTROL_DISABLE, VCEID},
	{"supercharger1",					IMAP_MOM_SUPERCHARGER_STAGE1,				nullptr,                        VEH_CNTRL_ISNOTAXIS,	VEH_CONTROL_DISABLE, VCEID},
	{"supercharger2",					IMAP_MOM_SUPERCHARGER_STAGE2,				nullptr,						VEH_CNTRL_ISNOTAXIS,	VEH_CONTROL_DISABLE, VCEID},
	{"superchargern",					IMAP_MOM_SUPERCHARGER_NEUTRAL,				nullptr,						VEH_CNTRL_ISNOTAXIS,	VEH_CONTROL_DISABLE, VCEID},
//	{"llswitch",						IMAP_MOM_LLSWITCH,							mlToggleLandingLight,			VEH_CNTRL_ISNOTAXIS,	VEH_CONTROL_DISABLE, VCEID},
//	{"fuelcock",						IMAP_MOM_FUELCOCK_NEXTTANK,					mlCycleFuelCock,				VEH_CNTRL_ISNOTAXIS,	VEH_CONTROL_DISABLE, VCEID},
//	{"cowlflaps",						IMAP_MOM_COWLFLAPS,							mlToggleCowlFlaps,				VEH_CNTRL_ISNOTAXIS,	VEH_CONTROL_DISABLE, VCEID},
//	{"panellight",						IMAP_MOM_PANELLIGHT,						mlTogglePanelLight,				VEH_CNTRL_ISNOTAXIS,	VEH_CONTROL_DISABLE, VCEID},
//	{"auxsight",						IMAP_MOM_AUXSIGHT,							mlToggleAuxSight,				VEH_CNTRL_ISNOTAXIS,	VEH_CONTROL_DISABLE, VCEID},
//	{"radio",							IMAP_MOM_RADIO,								nullptr,						VEH_CNTRL_ISNOTAXIS,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"hatchcontrol",					IMAP_MOM_HATCHCONTROL,						mlToggleHatch,					VEH_CNTRL_ISNOTAXIS,	VEH_CONTROL_DISABLE, VCEID},
	{"canopycontrol",					IMAP_MOM_CANOPYCONTROL,						mlToggleCanopy,					VEH_CNTRL_ISNOTAXIS,	VEH_CONTROL_DISABLE, VCEID},
	{"ordtrigger",						IMAP_MOM_ORDTRIGGER,						nullptr,						VEH_CNTRL_ISNOTAXIS,	VEH_CONTROL_DISABLE, VCEID},
	{"ordselector",						IMAP_MOM_ORDSELECTOR,						nullptr,						VEH_CNTRL_ISNOTAXIS,	VEH_CONTROL_DISABLE, VCEID},
	{"divebrakes",						IMAP_MOM_DEPLOYDIVEBRAKES,					mlToggleDiveBrakes,				VEH_CNTRL_ISNOTAXIS,	VEH_CONTROL_DISABLE, VCEID},
	{"autodive",						IMAP_MOM_AUTODIVE,							nullptr,						VEH_CNTRL_ISNOTAXIS,	VEH_CONTROL_DISABLE, VCEID},
	{"map",								IMAP_MOM_MAP,								nullptr,						VEH_CNTRL_ISNOTAXIS,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"aileron",							IMAP_ASX_ROLL,								mlAdjustStickX,					VEH_CNTRL_ISAXIS,		VEH_CONTROL_REDUCE_BY_STRENGTH, VCEID},
	{"elevator",						IMAP_ASX_PITCH,								mlAdjustStickY,					VEH_CNTRL_ISAXIS,		VEH_CONTROL_REDUCE_BY_STRENGTH, VCEID},
	{"rudder",							IMAP_ASX_YAW,								mlAdjustStickZ,					VEH_CNTRL_ISAXIS,		VEH_CONTROL_REDUCE_BY_STRENGTH, VCEID},
	{"throttle",						IMAP_ASX_THROTTLE,							mlAdjustThrottle,				VEH_CNTRL_ISAXIS,		VEH_CONTROL_DISABLE, VCEID},
	{"winch",							IMAP_ASX_WINCH,								mlAdjustWinch,					VEH_CNTRL_ISAXIS,		VEH_CONTROL_DISABLE, VCEID},
	{"gas",								IMAP_ASX_GAS,								mlAdjustGas,					VEH_CNTRL_ISAXIS,		VEH_CONTROL_AXIS_GOTONEG, VCEID},
	{"clutch",							IMAP_ASX_CLUTCH,							mlAdjustClutch,					VEH_CNTRL_ISAXIS,		VEH_CONTROL_AXIS_GOTONEG, VCEID},
	{"lefttrackclutch",					IMAP_ASX_LEFTTRACKCLUTCH,					mlLeftTrackClutchFunction,		VEH_CNTRL_ISAXIS,		VEH_CONTROL_UNAFFECTED, VCEID},
	{"righttrackclutch",				IMAP_ASX_RIGHTTRACKCLUTCH,					mlRightTrackClutchFunction,		VEH_CNTRL_ISAXIS,		VEH_CONTROL_UNAFFECTED, VCEID},
	{"flapcontrol",						IMAP_ASX_FLAPCONTROL,						mlAdjustFlapPosition,			VEH_CNTRL_ISAXIS,		VEH_CONTROL_DISABLE, VCEID},
	{"flapexecute",						IMAP_MOM_FLAPEXECUTE,						mlMoveFlapSetting,				VEH_CNTRL_ISAXIS,		VEH_CONTROL_DISABLE, VCEID},
	{"enginerpm",						IMAP_ASX_RPMLEVER,							mlRpmStubFunction,				VEH_CNTRL_ISAXIS,		VEH_CONTROL_DISABLE, VCEID},
	{"primarytrigger",					IMAP_HOLD_MAINTRIGGER,						mlPrimaryTriggerDown,			VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_DISABLE, VCEID},
	{"secondarytrigger",				IMAP_HOLD_SECONDTRIGGER,					mlSecondaryTriggerDown,			VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_DISABLE, VCEID},
	{"leftbrake",						IMAP_ASX_LEFTBRAKE,							mlLeftBrakeFunction,			VEH_CNTRL_ISAXIS,		VEH_CONTROL_AXIS_GOTONEG, VCEID},
	{"rightbrake",						IMAP_ASX_RIGHTBRAKE,						mlRightBrakeFunction,			VEH_CNTRL_ISAXIS,		VEH_CONTROL_AXIS_GOTONEG, VCEID},
	{"bothbrake",						IMAP_ASX_CENTERBRAKE,						mlCenterBrakeFunction,			VEH_CNTRL_ISAXIS,		VEH_CONTROL_AXIS_GOTONEG, VCEID},
	{"steering",						IMAP_ASX_STEERING,							mlSteeringFunction,				VEH_CNTRL_ISAXIS,		VEH_CONTROL_DISABLE, VCEID},
	{"engstart",						IMAP_MOM_STARTER_ON,						mlStarterIsDepressed,			VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_DISABLE, VCEID},
	{"taillock",						IMAP_MOM_TOGGLE_TAILLOCK,					mlToggleTailWheelLock,			VEH_CNTRL_ISNOTAXIS,	VEH_CONTROL_DISABLE, VCEID},
	{"shiftup",							IMAP_MOM_SHIFTUP,							mlShiftUp,						VEH_CNTRL_ISNOTAXIS,	VEH_CONTROL_DISABLE, VCEID},
	{"shiftdown",						IMAP_MOM_SHIFTDOWN,							mlShiftDown,					VEH_CNTRL_ISNOTAXIS,	VEH_CONTROL_DISABLE, VCEID},
	{"reverse",							IMAP_MOM_REVERSE,							mlReverse,						VEH_CNTRL_ISNOTAXIS,	VEH_CONTROL_DISABLE, VCEID},
	{"togglewepmode",					IMAP_MOM_TOGGLEWEP,							mlToggleWepMode,				VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_DISABLE, VCEID},
	{"elevtrim",						IMAP_ASX_ELEV_TRIM,							mlAdjustElevTrim,				VEH_CNTRL_ISAXIS,		VEH_CONTROL_DISABLE, VCEID},
	{"ailerontrim",						IMAP_ASX_AILERON_TRIM,						mlAdjustAileronTrim,			VEH_CNTRL_ISAXIS,		VEH_CONTROL_DISABLE, VCEID},
	{"ruddertrim",						IMAP_ASX_RUDDER_TRIM,						mlAdjustRudderTrim,				VEH_CNTRL_ISAXIS,		VEH_CONTROL_DISABLE, VCEID},
// turretraverse misspell
	{"turretraverse",					IMAP_ASX_TURRET_TRAVERSE,					mlTraverseTurretSpeed,			VEH_CNTRL_ISAXIS,		VEH_CONTROL_AXIS_GOTONEUTRAL, VCEID},
	{"turretelevate",					IMAP_ASX_TURRET_ELEVATE,					mlElevateTurretSpeed,			VEH_CNTRL_ISAXIS,		VEH_CONTROL_AXIS_GOTONEUTRAL, VCEID},
	{"turretraversesecondary",			IMAP_ASX_TURRET_TRAVERSE_SECONDARY,			mlTraverseTurretSpeed,			VEH_CNTRL_ISAXIS,		VEH_CONTROL_AXIS_GOTONEUTRAL, VCEID},
	{"turretelevatesecondary",			IMAP_ASX_TURRET_ELEVATE_SECONDARY,			mlElevateTurretSpeed,			VEH_CNTRL_ISAXIS,		VEH_CONTROL_AXIS_GOTONEUTRAL, VCEID},
	{"adjustpropup",					IMAP_MOM_ADJUST_PROP_UP,					mlPropPitchAdjust,				VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_DISABLE, VCEID},
	{"adjustpropdown",					IMAP_MOM_ADJUST_PROP_DOWN,					mlPropPitchAdjust,				VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_DISABLE, VCEID},
	{"atgunleftright",					IMAP_ASX_ATGUN_MOVEMENT_LEFTRIGHT,			mlATGunLeftRight,				VEH_CNTRL_ISAXIS,		VEH_CONTROL_AXIS_GOTONEUTRAL, VCEID},
	{"atgunforwardbackwards",			IMAP_ASX_ATGUN_MOVEMENT_FORWARDBACKWARDS,	mlATGunForwardBackwards,		VEH_CNTRL_ISAXIS,		VEH_CONTROL_AXIS_GOTONEUTRAL, VCEID},
	{"bombbaydoor",						IMAP_MOM_TOGGLE_BOMBBAYDOOR,				mlToggleBombBayDoor,			VEH_CNTRL_ISNOTAXIS,	VEH_CONTROL_DISABLE, VCEID},
	{"deployweapon",					IMAP_MOM_TOGGLE_DEPLOYWEAPON,				mlToggleBombBayDoor,			VEH_CNTRL_ISNOTAXIS,	VEH_CONTROL_DISABLE, VCEID},
	{"towbar",							IMAP_MOM_TOGGLE_TOWBAR,						mlToggleTowbar,					VEH_CNTRL_ISNOTAXIS,	VEH_CONTROL_DISABLE, VCEID},
	{"ridetransport",					IMAP_RIDE_TRANSPORT,						mlRideTransport,				VEH_CNTRL_ISNOTAXIS,	VEH_CONTROL_DISABLE, VCEID},
	{"stukasiren",						IMAP_MOM_STUKASIREN,						mlToggleStukaSiren,				VEH_CNTRL_ISNOTAXIS,	VEH_CONTROL_DISABLE, VCEID},
	{"cycleammo",						IMAP_TMP_CYCLE_AMMO,						mlCycleAmmo,					VEH_CNTRL_ISNOTAXIS,	VEH_CONTROL_DISABLE, VCEID},
	{"rangefind",						IMAP_TMP_RANGEFIND,							mlRangeFind,					VEH_CNTRL_ISNOTAXIS,	VEH_CONTROL_DISABLE, VCEID},
	{"gunsightrangeinc",				IMAP_HOLD_GUNSIGHT_RANGE_INC,				persGunsightRangeInc,			VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_DISABLE, VCEID},
	{"gunsightrangedec",				IMAP_HOLD_GUNSIGHT_RANGE_DEC,				persGunsightRangeDec,			VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_DISABLE, VCEID},
	{"gunsightconvinc",					IMAP_GUNSIGHT_CONVERGENCE_INC,				persGunsightConvergenceInc,		VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_DISABLE, VCEID},
	{"gunsightconvdec",					IMAP_GUNSIGHT_CONVERGENCE_DEC,				persGunsightConvergenceDec,		VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_DISABLE, VCEID},
	{"bombsightspeedinc",				IMAP_HOLD_BOMBSIGHT_SPEED_INC,				persBombsightSpeedInc,			VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_DISABLE, VCEID},
	{"bombsightspeeddec",				IMAP_HOLD_BOMBSIGHT_SPEED_DEC,				persBombsightSpeedDec,			VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_DISABLE, VCEID},
	{"bombsightaltinc",					IMAP_HOLD_BOMBSIGHT_ALT_INC,				persBombsightAltInc,			VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_DISABLE, VCEID},
	{"bombsightaltdec",					IMAP_HOLD_BOMBSIGHT_ALT_DEC,				persBombsightAltDec,			VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_DISABLE, VCEID},
	{"targetspeedinc",					IMAP_TARGET_SPEED_INC,						persGunsightTargetSpeedInc,		VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_DISABLE, VCEID},
	{"targetspeeddec",					IMAP_TARGET_SPEED_DEC,						persGunsightTargetSpeedDec,		VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_DISABLE, VCEID},
	{"targetangleinc",					IMAP_TARGET_ANGLE_OFF_INC,					persGunsightTargetAngleInc,		VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_DISABLE, VCEID},
	{"targetangledec",					IMAP_TARGET_ANGLE_OFF_DEC,					persGunsightTargetAngleDec,		VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_DISABLE, VCEID},
	{"targetmarkbearing",				IMAP_TARGET_MARK_BEARING,					persGunsightTargetMarkBearing,	VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_DISABLE, VCEID},
	{"targetcalcsolution",				IMAP_TARGET_CALC_SOLUTION,					persGunsightCalcTargetSolution,	VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_DISABLE, VCEID},
	{"torpedodepthinc",					IMAP_TORPEDO_DEPTH_INCREASE,				persGunsightTorpedoDepthInc,	VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_DISABLE, VCEID},
	{"torpedodepthdec",					IMAP_TORPEDO_DEPTH_DECREASE,				persGunsightTorpedoDepthDec,	VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_DISABLE, VCEID},

	{"jettison",						IMAP_JETTISON_ORDNANCE,						mlJettisonOrdnance,				VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_DISABLE, VCEID},
	{"autopilot",						IMAP_AUTOPILOT,								mlAutopilot,				    VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_DISABLE, VCEID},
    {"mobilespawn",                     IMAP_MOBILE_SPAWN,							mlToggleMobileSpawn,			VEH_CNTRL_ISNOTAXIS,	VEH_CONTROL_DISABLE, VCEID},

	// FP Begin
	// Pose Functions
	{"fpcrouch",						IMAP_HOLD_FP_CROUCH,						fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"fpfast",							IMAP_HOLD_FP_SPRINT,						fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"fpautorun",						IMAP_HOLD_FP_AUTORUN,						fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"fprelaxed",						IMAP_HOLD_FP_RELAXED,						fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"fpwalk",							IMAP_HOLD_FP_WALK,							fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"fpprone",							IMAP_HOLD_FP_PRONE,							fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_UNAFFECTED, VCEID},

	// Movement Functions
	{"fpforward",						IMAP_HOLD_FP_FORWARD,						fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"fpbackward",						IMAP_HOLD_FP_BACKWARD,						fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"fpturnright",						IMAP_HOLD_FP_TURN_RIGHT,					fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"fpturnleft",						IMAP_HOLD_FP_TURN_LEFT,						fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"fpstraferight",					IMAP_HOLD_FP_STRAFE_RIGHT,					fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"fpstrafeleft",					IMAP_HOLD_FP_STRAFE_LEFT,					fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"fpleanright",						IMAP_HOLD_FP_LEAN_RIGHT,					fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"fpleanleft",						IMAP_HOLD_FP_LEAN_LEFT,						fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_UNAFFECTED, VCEID},

	// Action Functions
	{"fpgunsight",						IMAP_HOLD_FP_GUNSIGHT,						fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"fpfire",							IMAP_HOLD_FP_FIRE,							fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"fplookleft",						IMAP_HOLD_FP_LOOK_LEFT,						fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"fplookright",						IMAP_HOLD_FP_LOOK_RIGHT,					fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"fpreload",						IMAP_HOLD_FP_RELOAD,						fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"fpdeploy",						IMAP_HOLD_FP_DEPLOY,						fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_UNAFFECTED, VCEID},

	// Weapon Selection Functions
	{"fpweapon0",						IMAP_HOLD_FP_WEAPON_1,						fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"fpweapon1",						IMAP_HOLD_FP_WEAPON_2,						fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"fpweapon2",						IMAP_HOLD_FP_WEAPON_3,						fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"fpweapon3",						IMAP_HOLD_FP_WEAPON_4,						fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"fpweapon4",						IMAP_HOLD_FP_WEAPON_5,						fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"fpweapon5",						IMAP_HOLD_FP_WEAPON_6,						fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"fpweapon6",						IMAP_HOLD_FP_WEAPON_7,						fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"fpweapon7",						IMAP_HOLD_FP_WEAPON_8,						fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"fpweapon8",						IMAP_HOLD_FP_WEAPON_9,						fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"fpweapon9",						IMAP_HOLD_FP_WEAPON_10,						fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_UNAFFECTED, VCEID},

	{"fpnextweapon",					IMAP_HOLD_FP_NEXT_WEAPON,					fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"fpprevweapon",					IMAP_HOLD_FP_PREV_WEAPON,					fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_UNAFFECTED, VCEID},

	// combat emotes
	{"fpcedouble",						IMAP_HOLD_FP_CE_DOUBLE,						fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"fpcegetdown",						IMAP_HOLD_FP_CE_GETDOWN,					fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"fpcehalt",						IMAP_HOLD_FP_CE_HALT,						fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"fpcemoveout",						IMAP_HOLD_FP_CE_MOVEOUT,					fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"fpceammo",						IMAP_HOLD_FP_CE_AMMO,						fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"fpcejump",						IMAP_HOLD_FP_CE_JUMP,						fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_UNAFFECTED, VCEID},

	{"togglemouselook",					IMAP_TOGGLE_MOUSE_LOOK,						nullptr,						VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"mouselookx",						IMAP_MOUSE_LOOK_X,							nullptr,						VEH_CNTRL_ISAXIS,		VEH_CONTROL_UNAFFECTED, VCEID},
	{"mouselooky",						IMAP_MOUSE_LOOK_Y,							nullptr,						VEH_CNTRL_ISAXIS,		VEH_CONTROL_UNAFFECTED, VCEID},
	{"mouselookz",						IMAP_MOUSE_LOOK_Z,							nullptr,						VEH_CNTRL_ISAXIS,		VEH_CONTROL_UNAFFECTED, VCEID},

	{"fptorsoverticalaxis",				IMAP_ASX_FP_TORSO_VERTICAL_AXIS,			fpFPSControlInput,				VEH_CNTRL_ISAXIS,		VEH_CONTROL_UNAFFECTED, VCEID},
	{"fptorsohorizontalaxis",			IMAP_ASX_FP_TORSO_HORIZONTAL_AXIS,			fpFPSControlInput,				VEH_CNTRL_ISAXIS,		VEH_CONTROL_UNAFFECTED, VCEID},
	{"fprideunridevehicle",				IMAP_MOM_FP_RIDE_UNRIDE_VEHICLE,			fpFPSControlInput,				VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"lookviewone",						IMAP_LOOK_FORWARD,							nullptr,						VEH_CNTRL_IS_SPECIAL,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"lookviewtwo",						IMAP_LOOK_BACK,								nullptr,						VEH_CNTRL_IS_SPECIAL,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"lookviewthree",					IMAP_LOOK_UP,								nullptr,						VEH_CNTRL_IS_SPECIAL,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"lookviewfour",					IMAP_LOOK_DOWN,								nullptr,						VEH_CNTRL_IS_SPECIAL,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"lookviewfive",					IMAP_LOOK_FORWARD_RIGHT,					nullptr,						VEH_CNTRL_IS_SPECIAL,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"lookviewsix",						IMAP_LOOK_FORWARD_LEFT,						nullptr,						VEH_CNTRL_IS_SPECIAL,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"lookviewseven",					IMAP_LOOK_RIGHT,							nullptr,						VEH_CNTRL_IS_SPECIAL,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"lookvieweight",					IMAP_LOOK_LEFT,								nullptr,						VEH_CNTRL_IS_SPECIAL,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"lookviewnine",					IMAP_LOOK_BACK_RIGHT,						nullptr,						VEH_CNTRL_IS_SPECIAL,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"lookviewten",						IMAP_LOOK_BACK_LEFT,						nullptr,						VEH_CNTRL_IS_SPECIAL,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"lookvieweleven",					IMAP_LOOK_FORWARD_UP,						nullptr,						VEH_CNTRL_IS_SPECIAL,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"lookviewtwelve",					IMAP_LOOK_FORWARD_RIGHT_UP,					nullptr,						VEH_CNTRL_IS_SPECIAL,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"lookviewthirteen",				IMAP_LOOK_FORWARD_LEFT_UP,					nullptr,						VEH_CNTRL_IS_SPECIAL,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"lookviewfourteen",				IMAP_LOOK_RIGHT_UP,							nullptr,						VEH_CNTRL_IS_SPECIAL,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"lookviewfifteen",					IMAP_LOOK_LEFT_UP,							nullptr,						VEH_CNTRL_IS_SPECIAL,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"lookviewsixteen",					IMAP_LOOK_BACK_RIGHT_UP,					nullptr,						VEH_CNTRL_IS_SPECIAL,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"lookviewseventeen",				IMAP_LOOK_BACK_LEFT_UP,						nullptr,						VEH_CNTRL_IS_SPECIAL,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"lookvieweightteen",				IMAP_LOOK_FORWARD_DOWN,						nullptr,						VEH_CNTRL_IS_SPECIAL,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"lookviewnineteen",				IMAP_LOOK_FORWARD_RIGHT_DOWN,				nullptr,						VEH_CNTRL_IS_SPECIAL,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"lookviewtwenty",					IMAP_LOOK_FORWARD_LEFT_DOWN,				nullptr,						VEH_CNTRL_IS_SPECIAL,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"lookviewtwentyone",				IMAP_LOOK_RIGHT_DOWN,						nullptr,						VEH_CNTRL_IS_SPECIAL,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"lookviewtwentytwo",				IMAP_LOOK_LEFT_DOWN,						nullptr,						VEH_CNTRL_IS_SPECIAL,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"lookviewtwentythree",				IMAP_LOOK_BACK_RIGHT_DOWN,					nullptr,						VEH_CNTRL_IS_SPECIAL,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"lookviewtwentyfour",				IMAP_LOOK_BACK_LEFT_DOWN,					nullptr,						VEH_CNTRL_IS_SPECIAL,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"gunsightview",					IMAP_TMP_GUNSIGHT_VIEW,						nullptr,						VEH_CNTRL_IS_SPECIAL,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"instrumentview",					IMAP_TMP_INSTRUMENT_VIEW,					nullptr,						VEH_CNTRL_IS_SPECIAL,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"positionone",						IMAP_MOM_POSITION_1,						nullptr,						VEH_CNTRL_IS_SPECIAL,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"positiontwo",						IMAP_MOM_POSITION_2,						nullptr,						VEH_CNTRL_IS_SPECIAL,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"positionthree",					IMAP_MOM_POSITION_3,						nullptr,						VEH_CNTRL_IS_SPECIAL,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"positionfour",					IMAP_MOM_POSITION_4,						nullptr,						VEH_CNTRL_IS_SPECIAL,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"positionfive",					IMAP_MOM_POSITION_5,						nullptr,						VEH_CNTRL_IS_SPECIAL,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"positionsix",						IMAP_MOM_POSITION_6,						nullptr,						VEH_CNTRL_IS_SPECIAL,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"positionseven",					IMAP_MOM_POSITION_7,						nullptr,						VEH_CNTRL_IS_SPECIAL,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"positioneight",					IMAP_MOM_POSITION_8,						nullptr,						VEH_CNTRL_IS_SPECIAL,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"positionnine",					IMAP_MOM_POSITION_9,						nullptr,						VEH_CNTRL_IS_SPECIAL,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"positionten",						IMAP_MOM_POSITION_10,						nullptr,						VEH_CNTRL_IS_SPECIAL,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"showgui",							IMAP_MOM_SHOW_GUI,							nullptr,						VEH_CNTRL_IS_SPECIAL,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"guicursor",						IMAP_MOM_SHOW_GUI_CURSOR,					nullptr,						VEH_CNTRL_IS_SPECIAL,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"guichatwindow",					IMAP_MOM_SHOW_GUI_CHAT_WINDOW,				nullptr,						VEH_CNTRL_IS_SPECIAL,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"showradiobuffer",					IMAP_MOM_SHOW_RADIO_BUFFER,					nullptr,						VEH_CNTRL_IS_SPECIAL,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"showingamemap",					IMAP_MOM_SHOW_IN_GAME_MAP,					nullptr,						VEH_CNTRL_IS_SPECIAL,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"changehudmode",					IMAP_MOM_TOGGLE_HUD,						nullptr,						VEH_CNTRL_IS_SPECIAL,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"resethud",						IMAP_MOM_RESET_HUD,							nullptr,						VEH_CNTRL_IS_SPECIAL,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"changechatheight",				IMAP_MOM_TOGGLE_CHATHEIGHT,					nullptr,						VEH_CNTRL_IS_SPECIAL,	VEH_CONTROL_UNAFFECTED, VCEID},

	{"externallocked",					IMAP_MOM_EXTERNAL_LOCKED,					nullptr,						VEH_CNTRL_IS_SPECIAL,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"externalrubberband",				IMAP_MOM_EXTERNAL_RUBBERBAND,				nullptr,						VEH_CNTRL_IS_SPECIAL,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"externalfixed",					IMAP_MOM_EXTERNAL_FIXED,					nullptr,						VEH_CNTRL_IS_SPECIAL,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"setfixedcamera",					IMAP_MOM_SET_FIXED_CAMERA,					nullptr,						VEH_CNTRL_IS_SPECIAL,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"slewviewleft",					IMAP_HOLD_SLEWVIEWLEFT,						nullptr,						VEH_CNTRL_IS_SPECIAL,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"slewviewright",					IMAP_HOLD_SLEWVIEWRIGHT,					nullptr,						VEH_CNTRL_IS_SPECIAL,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"slewviewup",						IMAP_HOLD_SLEWVIEWUP,						nullptr,						VEH_CNTRL_IS_SPECIAL,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"slewviewdown",					IMAP_HOLD_SLEWVIEWDOWN,						nullptr,						VEH_CNTRL_IS_SPECIAL,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"slewviewin",						IMAP_HOLD_SLEWVIEWIN,						nullptr,						VEH_CNTRL_IS_SPECIAL,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"slewviewout",						IMAP_HOLD_SLEWVIEWOUT,						nullptr,						VEH_CNTRL_IS_SPECIAL,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"rollviewright",					IMAP_HOLD_ROLLVIEWRIGHT,					nullptr,						VEH_CNTRL_IS_SPECIAL,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"rollviewleft",					IMAP_HOLD_ROLLVIEWLEFT,						nullptr,						VEH_CNTRL_IS_SPECIAL,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"changefov",						IMAP_TMP_CHANGE_FOV,						nullptr,						VEH_CNTRL_IS_SPECIAL,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"toggleinternalexternalviews",		IMAP_MOM_TOGGLE_INTERNAL_EXTERNAL_VIEWS,	nullptr,						VEH_CNTRL_IS_SPECIAL,	VEH_CONTROL_UNAFFECTED, VCEID},

    {"clonevehicle",					IMAP_CLONE_VEHICLE,							vehmainCloneVehicle,			VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_UNAFFECTED, VCEID},

	{"ezmove",							IMAP_EZ_MOVE,								nullptr,						VEH_CNTRL_ISAXIS,		VEH_CONTROL_UNAFFECTED, VCEID},
	{"ezturn",							IMAP_EZ_TURN,								nullptr,						VEH_CNTRL_ISAXIS,		VEH_CONTROL_UNAFFECTED, VCEID},
	{"ezaction1",						IMAP_EZ_ACTION1,							nullptr,						VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"ezaction2",						IMAP_EZ_ACTION2,							nullptr,						VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"ezfreelook",						IMAP_EZ_FREELOOK,							nullptr,						VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"ezcamerax",						IMAP_EZ_CAMERA_X,							nullptr,						VEH_CNTRL_ISAXIS,		VEH_CONTROL_UNAFFECTED, VCEID},
	{"ezcameray",						IMAP_EZ_CAMERA_Y,							nullptr,						VEH_CNTRL_ISAXIS,		VEH_CONTROL_UNAFFECTED, VCEID},
	{"eznextpos",						IMAP_EZ_NEXTPOS,							nullptr,						VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"ezprevpos",						IMAP_EZ_PREVPOS,							nullptr,						VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_UNAFFECTED, VCEID},
	{"ezengine",						IMAP_EZ_ENGINE,								nullptr,						VEH_CNTRL_ISHOLDDOWN,	VEH_CONTROL_UNAFFECTED, VCEID},

    // Experiments
    {"gvalueinc",                       IMAP_HOLD_GVALUE_INC,                       persGValueInc,                  VEH_CNTRL_ISHOLDDOWN,   VEH_CONTROL_UNAFFECTED, VCEID},
    {"gvaluedec",                       IMAP_HOLD_GVALUE_DEC,                       persGValueDec,                  VEH_CNTRL_ISHOLDDOWN,   VEH_CONTROL_UNAFFECTED, VCEID},
    {"gtoggle1",                        IMAP_TOGGLE1,                               persGValueT,                   VEH_CNTRL_ISHOLDDOWN,   VEH_CONTROL_UNAFFECTED, VCEID},

	{"",								0,											nullptr,						0,						0, VCEID} // "null-terminated",

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

UINT32 vehCntrlInstallControlCallbacks()
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

UINT32 vehCntrlInit()
{
	vehCntrlInstallControlCallbacks();

	return 0;
}

UINT32 vehcntrlDisableAllControls()
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

UINT32 vehcntrlBailout()
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

	FLOAT32 Strength;
	LOGICAL ControlsDisabled;

	SINT32 Parameter = InParameter;
	// first check to see if the IK system has "taken over" this function
	// if it's an axis function for the IK system we still execute it
	// this is to allow axis operations to trigger
//	vehVehicle* Vehicle = persGetPersonaVehiclePtr();
	UINT32 Position = persGetPersonaPosition();
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
		if(vehCntrlCommandList[i].CommandId == Id)
            return &vehCntrlCommandList[i];
	}
	return nullptr;
}

