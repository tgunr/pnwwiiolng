#include "syscfg.h" // Path is project specific.
// player gauges data
//
// this file contains functions for accessing and creating the player's gauge data
// based on a vehicle data structure referenced in
//
// created by Hoof

#include <math.h>

#include "ml/mlgauges.h"
#include "ml/mltypes.h"
#include "ml/mlutility.h"
#include "ml/mlAtmo.h"
#include "ml/mlweapons.h"
#include "math/mathvect.h"
#include "veh/vehmain.h"
#include "veh/vehanim.h"
#include "ob/obdraw.h"
#include "trn/trnmain.h"
#include "trn/trnop.h"

#include "ogrp/ogrptype.h"
#include "rn/rnmain.h"
#include "veh/vehmain.h"
#include "math/math3d.h"

#include "ml/mlweapons.h"
#include "locl/loclMain.h"
#include "trn/trntod.h"
#include "trn/trncd.h"
#include "ogl/ogl.h"
#include "ogl/oglGlobalStates.h"
#include "ogl/oglMaterial.h"
#if USE_STATESET
#include "ogl/States.h"
#endif

static UINT32 _SetDOFObjects(OBJECT * Object);
static UINT32 _DoSpecialDOFs(mlVehicleData * VehicleData,OBJECT * Object);

static UINT32 _SetObjectTreeToAlpha(OBJECT * Object,UINT32 Alpha);
static UINT32 _SetObjectSubTreeToAlpha(OBJECT * Object, UINT32 Alpha);
static bool _DrawAlphadProp(OBJECT * Object);


mlVehicleVarData _VarData;

UINT32 _VarDataReference[ML_DOF_MAXNUM];
UCHAR mlDOFVarNameTable[ML_DOF_MAXNUM][32];

mlGaugeTranslationTable mlDOFTransTable[ML_DOF_MAXNUM] =
{
	{"none",1, ML_DOF_NONE,nullptr},
	{"gearhandle",1,ML_DOF_CONTROL_GEARHANDLE,(void*)&_VarData.GearHandlePos,0,0},	// gear handle 0,1
	{"gearshiftx",1,ML_DOF_CONTROL_GEARSHIFTX,(void*)&_VarData.GearShiftX,0,1}, // h-type gearshift x pos, 0.0 = upper left (gear 1)
	{"gearshifty",1,ML_DOF_CONTROL_GEARSHIFTY,(void*)&_VarData.GearShiftY,0,1}, // h-type gearshift y pos, 0.0 = upper left (gear 1)
	{"lineargearshift",1,ML_DOF_CONTROL_LINEARGEARSHIFT,(void*)&_VarData.LinearGearShift,0,1}, // linear gearshift, 0.0 = max reverse, 1.0 = max forward
	{"rtrim",1,ML_DOF_CONTROL_RTRIM,(void*)&_VarData.RudderTrim,0,0},		// rudder trim -1,0,1
	{"etrim",1,ML_DOF_CONTROL_ETRIM,(void*)&_VarData.ElevatorTrim,0,0},		// elevator trim -1,0,1
	{"atrim",1,ML_DOF_CONTROL_ATRIM,(void*)&_VarData.AileronTrim,0,0},		// Aileron trim -1,0,1
	{"throttle",ML_MAX_ENGINES,ML_DOF_CONTROL_THROTTLE,(void*)&_VarData.Throttle,0,0},	// Throttle Pos 0,1
	{"mixture",ML_MAX_ENGINES,ML_DOF_CONTROL_MIXTURE,(void*)&_VarData.Mixture,0,0},		// mixture pos 0,1,2 (off, lean, rich)
	{"rpmlever",ML_MAX_ENGINES,ML_DOF_CONTROL_RPMLEVER,(void*)&_VarData.RpmLever,0,0},	// rpm position, rpmval
	{"pitchlever",ML_MAX_ENGINES,ML_DOF_CONTROL_PITCHLEVER,(void*)&_VarData.PitchLever,0,0},	// pitch position 0-90
	{"rpmrocker",ML_MAX_ENGINES,ML_DOF_CONTROL_RPMROCKER,(void*)&_VarData.RpmRocker,0,0},	// rpm adjuster rocker -1,0,1
	{"pitchrocker",ML_MAX_ENGINES,ML_DOF_CONTROL_PITCHROCKER,(void*)&_VarData.PitchRocker,0,0}, // pitch adjuster rocker -1,0,1
	{"supercharger",ML_MAX_ENGINES,ML_DOF_CONTROL_SUPERCHARGER,(void*)&_VarData.SuperChargerSetting,0,0},// supercharger position 0,1,2 (neutral,speed1,speed2)
	{"flaplever",1,ML_DOF_CONTROL_FLAPLEVER,(void*)&_VarData.FlapLever,0,0},	// flap lever, 0,maxflaps
	{"llswitch",1,ML_DOF_CONTROL_LLSWITCH,(void*)&_VarData.LandingLightSwitch,0,0},	// landing light switch, 0,1
	{"brakel",1,ML_DOF_CONTROL_BRAKELEFT,(void*)&_VarData.LeftBrake,0,0},	// left gear brakes 0,1
	{"braker",1,ML_DOF_CONTROL_BRAKERIGHT,(void*)&_VarData.RightBrake,0,0},	// right gear brakes 0,1
	{"brakeb",1,ML_DOF_CONTROL_BRAKEALL,(void*)&_VarData.GeneralBrake,0,0},	// general gear brakes 0,1
	{"ttraversewheel",ML_MAX_CREW_POSITIONS,ML_DOF_ELEMENT_TTRAVERSEWHEEL,(void*)&_VarData.TurretTraverse,1,1}, // turret traverse wheel control
	{"televatewheel",ML_MAX_CREW_POSITIONS,ML_DOF_ELEMENT_TELEVATEWHEEL,(void*)&_VarData.TurretElevate,1,1}, // turret elevate wheel control
	{"lefttrackclutch",1,ML_DOF_CONTROL_LEFTTRACKCLUTCH,(void*)&_VarData.LeftTrackClutch,0,0}, // left track clutch 0,1
	{"righttrackclutch",1,ML_DOF_CONTROL_RIGHTTRACKCLUTCH,(void*)&_VarData.RightTrackClutch,0,0}, // right track clutch 0,1
	{"fuelcock",ML_MAX_ENGINES,ML_DOF_CONTROL_FUELCOCK,(void*)&_VarData.FuelCock,0,0},	// fuel selector 0,maxtanks
	{"cflaps",ML_MAX_ENGINES,ML_DOF_CONTROL_COWLLEVER,(void*)&_VarData.CowlFlap,0,0},	// flap control 0,maxflapnum
	{"panellight",1,ML_DOF_CONTROL_PANELLIGHT,(void*)&_VarData.PanelLight,0,0},	// instrument panel light 0,1
	{"auxsight",1,ML_DOF_CONTROL_AUXSIGHT,(void*)&_VarData.AuxiliarySight,0,0},	// auxiliary sight control 0,1
	{"radio",1,ML_DOF_CONTROL_RADIO,(void*)&_VarData.Radio,0,0},		// radio switch 0,1
	{"hatchcontrol",ML_MAX_CREW_POSITIONS,ML_DOF_CONTROL_HATCHCONTROL,(void*)&_VarData.HatchControl,0,0},// hatch control/handle 0,1
	{"canopycontrol",ML_MAX_CREW_POSITIONS,ML_DOF_CONTROL_CANOPYCONTROL,(void*)&_VarData.CanopyControl,0,0},// canopy control/handle 0,1
	{"maintrigger",1,ML_DOF_CONTROL_MAINTRIGGER,(void*)&_VarData.MainTrigger,0,0},	// main weapon trigger 0,1
	{"secondtrigger",1,ML_DOF_CONTROL_SECONDTRIGGER,(void*)&_VarData.SecondTrigger,0,0},// secondary weapon trigger 0,1
	{"ordtrigger",1,ML_DOF_CONTROL_ORDTRIGGER,(void*)&_VarData.OrdTrigger,0,0},	// ordinance trigger 0,1
	{"ordselector",1,ML_DOF_CONTROL_ORDSELECTOR,(void*)&_VarData.OrdSelector,0,0},	// ordinance selector 0,maxhardpoints
	{"divebrake",1,ML_DOF_CONTROL_DIVEBRAKE,nullptr,0,0},	// dive brake control 0,1
	{"autodive",1,ML_DOF_CONTROL_AUTODIVE,(void*)&_VarData.AutoDive,0,0},	// autodive system control 0,1
	{"map",1,ML_DOF_CONTROL_MAP,(void*)&_VarData.Map,0,0},			// map mode 0,1
	{"starter",ML_MAX_ENGINES,ML_DOF_CONTROL_STARTER,(void*)&_VarData.StarterPos,0,0}, // starter pos 0,1
	{"laileron",1,ML_DOF_ELEMENT_LAILERON,(void*)&_VarData.LeftAileron,0,0},	// left aileron position -1,0,1
	{"raileron",1,ML_DOF_ELEMENT_RAILERON,(void*)&_VarData.RightAileron,0,0},	// right aileron position -1,0,1
	{"elevator",1,ML_DOF_ELEMENT_ELEVATOR,(void*)&_VarData.Elevator,0,0},	// elevator position -1,0,1
	{"rudder",1,ML_DOF_ELEMENT_RUDDER,(void*)&_VarData.Rudder,0,0},		// rudder position -1,0,1
	{"propspin",ML_MAX_ENGINES,ML_DOF_ELEMENT_PROPSPIN,(void*)&_VarData.PropSpin,0,0},	// propeller spin position 0,1
	{"proppitch",ML_MAX_ENGINES,ML_DOF_ELEMENT_PROPPITCH,(void*)&_VarData.PropPitch,0,1},	// propeller pitch 0,90
	{"proppitchdial",ML_MAX_ENGINES,ML_DOF_ELEMENT_PROPPITCHDIAL,(void*)&_VarData.PropPitch,1,0}, // prop pitch dial 0,90
	{"propeffect",ML_MAX_ENGINES,ML_DOF_ELEMENT_PROPEFFECT,(void*)&_VarData.PropEffect,0,0},	// propeller effect spin position 0,1
	{"flap",1,ML_DOF_ELEMENT_FLAP,nullptr,0,0},		// flap position 0,maxflapposition
	{"gear",1,ML_DOF_ELEMENT_GEAR,nullptr,0,0},		// gear position 0,1
	{"bombbaydoor",1,ML_DOF_ELEMENT_BOMBBAYDOOR,0,0},
	{"canopy",ML_MAX_CREW_POSITIONS,ML_DOF_ELEMENT_CANOPY,nullptr,0,0},		// canopy position 0,1
	{"lslot",1,ML_DOF_ELEMENT_LEFTSLOTOPEN,(void*)&_VarData.LeftSlotOpen,0,0}, // left slot open 0,1
	{"gearshift",1,ML_DOF_ELEMENT_GEARSHIFT,(void*)&_VarData.GearShiftState,0,0}, // gear shift, zero is neutral, negative is reverse
	{"clutch",1,ML_DOF_ELEMENT_CLUTCHSTATE,(void*)&_VarData.ClutchState,0,0}, // clutch state, 0.0 to 1.0
	{"rslot",1,ML_DOF_ELEMENT_RIGHTSLOTOPEN,(void*)&_VarData.RightSlotOpen,0,0}, // right slot open 0,1
	{"body",1,ML_DOF_ELEMENT_BODY,nullptr,0,0},		// body position dof (special case)
	{"eye",1,ML_DOF_ELEMENT_EYE,nullptr,0,0},			// eye position dof (special case)
	{"head",1,ML_DOF_ELEMENT_HEAD,nullptr,0,0},		// head position dof (special case)
	{"hatch",ML_MAX_CREW_POSITIONS,ML_DOF_ELEMENT_HATCH, nullptr,0,0},		// hatch position 0,1
	{"asimph",1,ML_DOF_GAUGE_ASIMPH,(void*)&_VarData.AirspeedMPH,1,0},		// airspeed indicator, mph
	{"asikph",1,ML_DOF_GAUGE_ASIKPH,(void*)&_VarData.AirspeedKPH,1,0},		// airspeed indicator, kph
	{"asikts",1,ML_DOF_GAUGE_ASIKTS,(void*)&_VarData.AirspeedKTS,1,0},		// airspeed indicator, knots
	{"asimphstop",1,ML_DOF_GAUGE_ASIMPHSTOP,(void*)&_VarData.AirspeedMPH,0,0},		// airspeed indicator, mph
	{"asikphstop",1,ML_DOF_GAUGE_ASIKPHSTOP,(void*)&_VarData.AirspeedKPH,0,0},		// airspeed indicator, kph
	{"asiktsstop",1,ML_DOF_GAUGE_ASIKTSSTOP,(void*)&_VarData.AirspeedKTS,0,0},		// airspeed indicator, knots
	{"vsift",1,ML_DOF_GAUGE_VSIFT,(void*)&_VarData.VSIFeeT,0,0},			// vertical speed indicator ft/min
	{"vsim",1,ML_DOF_GAUGE_VSIM,(void*)&_VarData.VSIMeters,0,0},			// vertical speed indicator m/s
	{"rpm",ML_MAX_ENGINES,ML_DOF_GAUGE_RPM,(void*)&_VarData.Rpm,0,0},			// engine RPM gauge
	{"proprpmsetting",ML_MAX_ENGINES,ML_DOF_GAUGE_PROPRPMSETTING,(void*)&_VarData.PropRpmSetting,0,0}, // the rpm setting
	{"propadjust",ML_MAX_ENGINES,ML_DOF_GAUGE_PROPADJUST,(void*)&_VarData.PropAdjust,0,0}, // -1 is decrease pitch, 0 is neutral, 1 is positive
	{"alt1ft",1,ML_DOF_GAUGE_ALT1FT,(void*)&_VarData.Alt1Feet,1,0},		// altimeter 1ft arm
	{"alt10ft",1,ML_DOF_GAUGE_ALT10FT,(void*)&_VarData.Alt10Feet,1,0},		// altimeter 10ft arm
	{"alt100ft",1,ML_DOF_GAUGE_ALT100FT,(void*)&_VarData.Alt100Feet,1,0},		// altimeter 100ft arm
	{"alt1m",1,ML_DOF_GAUGE_ALT1M,(void*)&_VarData.Alt1Meter,1,0},			// altimeter 1m arm
	{"alt10m",1,ML_DOF_GAUGE_ALT10M,(void*)&_VarData.Alt1Feet,1,0},		// altimeter 10m arm
	{"aglmeters",1,ML_DOF_GAUGE_AGLMETERS,(void*)&_VarData.AGLMeters,0,0}, // agl in meters
	{"aglfeet",1,ML_DOF_GAUGE_AGLFEET,(void*)&_VarData.AGLFeet,0,0},		// agl in feet
	{"slipdeg",1,ML_DOF_GAUGE_SLIPDEG,(void*)&_VarData.SlipDeg,0,0},		// slip indicator, deg
	{"turndeg",1,ML_DOF_GAUGE_TURNDEG,(void*)&_VarData.TurnDeg,0,0},		// turn indicator, deg
	{"bankdeg",1,ML_DOF_GAUGE_BANKDEG,(void*)&_VarData.BankDeg,0,0},		// bank indicator, deg
	{"clockhr",1,ML_DOF_GAUGE_CLOCKHR,(void*)&_VarData.ClockHour,1,1},		// hour hand of clock
	{"clockmin",1,ML_DOF_GAUGE_CLOCKMIN,(void*)&_VarData.ClockMin,1,1},		// minute hand of clock
	{"clocksec",1,ML_DOF_GAUGE_CLOCKSEC,(void*)&_VarData.ClockSec,1,1},		// second hand of clock
	{"oxygen",1,ML_DOF_GAUGE_OXYGEN,(void*)&_VarData.Oxygen,0,0},		// oxygen remaining indicator
	{"artificialhpitch",1,ML_DOF_GAUGE_ARTIFICIALHPITCH,(void*)&_VarData.ArtificialHorizonPitch,1,0},// artificial horizon pitch dof
	{"artificialhroll",1,ML_DOF_GAUGE_ARTIFICIALHROLL,(void*)&_VarData.ArtificialHorizonRoll,1,0},// artificial horizon roll dof
	{"dgyro",1,ML_DOF_GAUGE_DGYRO,(void*)&_VarData.DirectionalGyro,1,0},			// directional gyro
	{"magcompass",1,ML_DOF_GAUGE_MAGCOMPASS,(void*)&_VarData.MagneticCompass,1,0},	// magnetic compass
	{"fuelgauge",ML_MAX_FUELTANKS,ML_DOF_GAUGE_FUELGAUGE,(void*)&_VarData.FuelGauge,0,0},		// fuel gauge
	{"totalfuel",1,ML_DOF_GAUGE_TOTALFUEL,(void*)&_VarData.TotalFuel,0,0},			// total fuel gauge
	{"manpresscmhg",ML_MAX_ENGINES,ML_DOF_GAUGE_MANPRESSCMHG,(void*)&_VarData.ManifoldPressureBoostCmHg,0,0},	// manifold pressure boost, cm/hg
	{"manpressinhg",ML_MAX_ENGINES,ML_DOF_GAUGE_MANPRESSINHG,(void*)&_VarData.ManifoldPressureInHg,0,0},	// manifold pressure, in/hg
	{"manpresslbs",ML_MAX_ENGINES,ML_DOF_GAUGE_MANPRESSLBS,(void*)&_VarData.ManifoldPressureBoostLbs,0,0},	// manifold pressure boost, in lbs
	{"manpressatm",ML_MAX_ENGINES,ML_DOF_GAUGE_MANPRESSATM,(void*)&_VarData.ManifoldPressureAtm,0,0},	// manifold pressure, atmospheres
	{"oilpress",ML_MAX_ENGINES,ML_DOF_GAUGE_OILPRESS,(void*)&_VarData.OilPressure,0,0},		// oil pressure
	{"oillevel",ML_MAX_ENGINES,ML_DOF_GAUGE_OILLEVEL,(void*)&_VarData.OilLevel,0,0},			// oil level
	{"coolpress",ML_MAX_ENGINES,ML_DOF_GAUGE_COOLPRESS,(void*)&_VarData.CoolantPressure,0,0},		// coolant pressure
	{"engtemp",ML_MAX_ENGINES,ML_DOF_GAUGE_ENGTEMP,(void*)&_VarData.EngineTemp,0,0},		// engine temperature farenheit
	{"gearlup",1,ML_DOF_GAUGE_GEARLUP,(void*)&_VarData.LeftGearUp,0,0},		// gear left is up
	{"gearrup",1,ML_DOF_GAUGE_GEARRUP,(void*)&_VarData.RightGearUp,0,0},		// gear right is up
	{"gearcup",1,ML_DOF_GAUGE_GEARCUP,(void*)&_VarData.CenterGearUp,0,0},		// gear center is up
	{"gearldown",1,ML_DOF_GAUGE_GEARLDOWN,(void*)&_VarData.LeftGearDown,0,0},		// gear left is down
	{"gearrdown",1,ML_DOF_GAUGE_GEARRDOWN,(void*)&_VarData.RightGearDown,0,0},		// gear right is down
	{"gearcdown",1,ML_DOF_GAUGE_GEARCDOWN,(void*)&_VarData.CenterGearDown,0,0},		// gear center is down
	{"gearlmov",1,ML_DOF_GAUGE_GEARLMOV,(void*)&_VarData.LeftGearMoving,0,0},		// gear left is moving
	{"gearrmov",1,ML_DOF_GAUGE_GEARRMOV,(void*)&_VarData.RightGearMoving,0,0},		// gear right is moving
	{"gearcmov",1,ML_DOF_GAUGE_GEARCMOV,(void*)&_VarData.CenterGearMoving,0,0},		// gear center is moving
	{"ammocounter",ML_MAX_MOUNTS,ML_DOF_GAUGE_AMMOINDICATOR,(void*)&_VarData.AmmoCounters,0,0}, // num rounds
	{"totalammo",1,ML_DOF_GAUGE_TOTALAMMO,(void*)&_VarData.TotalAmmo,0,0}, // total ammo
	{"bspeed",1,ML_DOF_GAUGE_BOMBSIGHT_SPEED,(void*)&_VarData.BombSightSpeedSetting,0,1}, // bombsight speed setting gauge
	{"balt",1,ML_DOF_GAUGE_BOMBSIGHT_ALT,(void*)&_VarData.BombSightAltSetting,0,1}, // bombsight alt setting gauge
	{"bmpivot",1,ML_DOF_BOMBSIGHT_PIVOT,nullptr,0,0}, // british mk9a pivot point
	{"bforesight",1,ML_DOF_BOMBSIGHT_FORESIGHT,nullptr,0,0}, // british mk9a foresight point
	{"bbacksight",1,ML_DOF_BOMBSIGHT_BACKSIGHT,0,0}, // british mk9a backsight point
	{"ttraverse",ML_MAX_CREW_POSITIONS,ML_DOF_ELEMENT_TTRAVERSE,(void*)&_VarData.TurretTraverse,1,1}, // turret traverse position
	{"televate",ML_MAX_CREW_POSITIONS,ML_DOF_ELEMENT_TELEVATE,(void*)&_VarData.TurretElevate,0,1}, // turret elevation position
	{"ttraversestops",ML_MAX_CREW_POSITIONS,ML_DOF_ELEMENT_TRAVERSELIMITED,(void*)&_VarData.TurretTraverseWithStops,0,1}, // turret traverse with stops
	{"flextraverse",ML_MAX_CREW_POSITIONS,ML_DOF_ELEMENT_FLEXTRAVERSE,(void*)&_VarData.TurretTraverseWithStops,0,1}, // flexible traverse
	{"flexelevate",ML_MAX_CREW_POSITIONS,ML_DOF_ELEMENT_FLEXELEVATE,(void*)&_VarData.TurretElevate,0,1}, // flexible elevator
	{"recoil",VEH_MAX_MOUNTS,ML_DOF_RECOIL,nullptr,0,0},
	{"iklink1",1,ML_DOF_IK_LINK1,nullptr,0,0},
	{"iklink2",1,ML_DOF_IK_LINK2,nullptr,0,0},
	{"iklink3",1,ML_DOF_IK_LINK3,nullptr,0,0},
	{"ikchainend",1,ML_DOF_IK_CHAINEND,nullptr,0,0},
	{"ik",1,ML_DOF_IK_ENDEFFECTOR,nullptr,0,0},
	{"viewpoint",1,ML_DOF_VIEWPOINT,nullptr,0,0},
	{"spawn",TRN_MAX_SPAWN_LOCATIONS,ML_DOF_SPAWNLOCATION,nullptr,0,0},
	{"ailocate",1,ML_DOF_AG_LOCATE,0,0},
	{"aielevate",1,ML_DOF_AG_ELEVATE,0,0},
	{"aitraverse",1,ML_DOF_AG_TRAVERSE,0,0},
	{"aiview",1,ML_DOF_AG_VIEW,0,0},
	{"aifire",1,ML_DOF_AG_FIRE,0,0},
	{"wcontact",ML_MAX_GEARNUM,ML_DOF_WHEEL_CONTACTPOINT,nullptr,0,1}, // contact points for gear
	{"wrotate",ML_MAX_GEARNUM,ML_DOF_WHEEL_ROTATE,(void*)&_VarData.WheelRotate,0,1}, // wheel rotation value
	{"wstrut",ML_MAX_GEARNUM,ML_DOF_WHEEL_STRUT,(void*)&_VarData.WheelStrut,0,1}, // suspension compression
	{"wbogie",ML_MAX_GEARNUM,ML_DOF_WHEEL_BOGIE,(void*)&_VarData.WheelBogie,0,1}, // bogie tilt
	{"wcastor",ML_MAX_GEARNUM,ML_DOF_WHEEL_CASTOR,(void*)&_VarData.WheelCastor,1,1}, // bogie castor value
	{"wsteer",1,ML_DOF_WHEEL_STEER,(void*)&_VarData.WheelSteer,0,0}, // steering
	{"mount",ML_MAX_CONFIGURATIONS,ML_DOF_MOUNT,nullptr,0,0}, // weapon mount pnt
	{"connect",ML_MAX_WEAPONS_PER_MOUNT,ML_DOF_CONNECT,nullptr,0,0}, // weapon connection points
	{"firepoint",1,ML_DOF_FIREPOINT,nullptr,0,0}, // weapon fire point
	{"hitchlight",1,ML_DOF_TOWPOINT_LIGHT,nullptr,0,0}, // tow point
	{"hitchmedium",1,ML_DOF_TOWPOINT_MEDIUM,nullptr,0,0}, // tow point
	{"hitchheavy",1,ML_DOF_TOWPOINT_HEAVY,nullptr,0,0}, // tow point
	{"fpsseat",1,ML_DOF_FPS_SEAT,nullptr,0,0}, // fps seat
	{"crap",1,ML_DOF_CRAPPOINT,nullptr,0,0}, // weapon crap point
	{"pointcollide",1,ML_DOF_POINTCOLLIDE,nullptr,0,0}, // point collide point
	{"demovar",1,ML_DOF_DEMOVAR,(void*)&_VarData.DemoVar,0,0}, // demo variable
	{"ttraverseconverge1_",VEH_MAX_MOUNTS,ML_DOF_ELEMENT_TTRAVERSE1_CONV,(void*)&_VarData.TurretTraverseConv[0],1,1},
	{"ttraverseconverge2_",VEH_MAX_MOUNTS,ML_DOF_ELEMENT_TTRAVERSE2_CONV,(void*)&_VarData.TurretTraverseConv[1],1,1},
	{"ttraverseconverge3_",VEH_MAX_MOUNTS,ML_DOF_ELEMENT_TTRAVERSE3_CONV,(void*)&_VarData.TurretTraverseConv[2],1,1},
	{"ttraverseconverge4_",VEH_MAX_MOUNTS,ML_DOF_ELEMENT_TTRAVERSE4_CONV,(void*)&_VarData.TurretTraverseConv[3],1,1},
	{"ttraverseconverge5_",VEH_MAX_MOUNTS,ML_DOF_ELEMENT_TTRAVERSE5_CONV,(void*)&_VarData.TurretTraverseConv[4],1,1},
	{"ttraverseconverge6_",VEH_MAX_MOUNTS,ML_DOF_ELEMENT_TTRAVERSE6_CONV,(void*)&_VarData.TurretTraverseConv[5],1,1},
	{"ttraverseconverge7_",VEH_MAX_MOUNTS,ML_DOF_ELEMENT_TTRAVERSE7_CONV,(void*)&_VarData.TurretTraverseConv[6],1,1},
	{"ttraverseconverge8_",VEH_MAX_MOUNTS,ML_DOF_ELEMENT_TTRAVERSE8_CONV,(void*)&_VarData.TurretTraverseConv[7],1,1},
	{"ttraverseconverge9_",VEH_MAX_MOUNTS,ML_DOF_ELEMENT_TTRAVERSE9_CONV,(void*)&_VarData.TurretTraverseConv[8],1,1},
	{"ttraverseconverge10_",VEH_MAX_MOUNTS,ML_DOF_ELEMENT_TTRAVERSE10_CONV,(void*)&_VarData.TurretTraverseConv[9],1,1},
	{"ttraverseconverge11_",VEH_MAX_MOUNTS,ML_DOF_ELEMENT_TTRAVERSE11_CONV,(void*)&_VarData.TurretTraverseConv[10],1,1},
	{"ttraverseconverge12_",VEH_MAX_MOUNTS,ML_DOF_ELEMENT_TTRAVERSE12_CONV,(void*)&_VarData.TurretTraverseConv[11],1,1},
	{"ttraverseconverge13_",VEH_MAX_MOUNTS,ML_DOF_ELEMENT_TTRAVERSE13_CONV,(void*)&_VarData.TurretTraverseConv[12],1,1},
	{"ttraverseconverge14_",VEH_MAX_MOUNTS,ML_DOF_ELEMENT_TTRAVERSE14_CONV,(void*)&_VarData.TurretTraverseConv[13],1,1},
	{"ttraverseconverge15_",VEH_MAX_MOUNTS,ML_DOF_ELEMENT_TTRAVERSE15_CONV,(void*)&_VarData.TurretTraverseConv[14],1,1},
	{"ttraverseconverge16_",VEH_MAX_MOUNTS,ML_DOF_ELEMENT_TTRAVERSE16_CONV,(void*)&_VarData.TurretTraverseConv[15],1,1},
	{"televateconverge1_",VEH_MAX_MOUNTS,ML_DOF_ELEMENT_TELEVATE1_CONV,(void*)&_VarData.TurretElevateConv[0],1,1},
	{"televateconverge2_",VEH_MAX_MOUNTS,ML_DOF_ELEMENT_TELEVATE2_CONV,(void*)&_VarData.TurretElevateConv[1],1,1},
	{"televateconverge3_",VEH_MAX_MOUNTS,ML_DOF_ELEMENT_TELEVATE3_CONV,(void*)&_VarData.TurretElevateConv[2],1,1},
	{"televateconverge4_",VEH_MAX_MOUNTS,ML_DOF_ELEMENT_TELEVATE4_CONV,(void*)&_VarData.TurretElevateConv[3],1,1},
	{"televateconverge5_",VEH_MAX_MOUNTS,ML_DOF_ELEMENT_TELEVATE5_CONV,(void*)&_VarData.TurretElevateConv[4],1,1},
	{"televateconverge6_",VEH_MAX_MOUNTS,ML_DOF_ELEMENT_TELEVATE6_CONV,(void*)&_VarData.TurretElevateConv[5],1,1},
	{"televateconverge7_",VEH_MAX_MOUNTS,ML_DOF_ELEMENT_TELEVATE7_CONV,(void*)&_VarData.TurretElevateConv[6],1,1},
	{"televateconverge8_",VEH_MAX_MOUNTS,ML_DOF_ELEMENT_TELEVATE8_CONV,(void*)&_VarData.TurretElevateConv[7],1,1},
	{"televateconverge9_",VEH_MAX_MOUNTS,ML_DOF_ELEMENT_TELEVATE9_CONV,(void*)&_VarData.TurretElevateConv[8],1,1},
	{"televateconverge10_",VEH_MAX_MOUNTS,ML_DOF_ELEMENT_TELEVATE10_CONV,(void*)&_VarData.TurretElevateConv[9],1,1},
	{"televateconverge11_",VEH_MAX_MOUNTS,ML_DOF_ELEMENT_TELEVATE11_CONV,(void*)&_VarData.TurretElevateConv[10],1,1},
	{"televateconverge12_",VEH_MAX_MOUNTS,ML_DOF_ELEMENT_TELEVATE12_CONV,(void*)&_VarData.TurretElevateConv[11],1,1},
	{"televateconverge13_",VEH_MAX_MOUNTS,ML_DOF_ELEMENT_TELEVATE13_CONV,(void*)&_VarData.TurretElevateConv[12],1,1},
	{"televateconverge14_",VEH_MAX_MOUNTS,ML_DOF_ELEMENT_TELEVATE14_CONV,(void*)&_VarData.TurretElevateConv[13],1,1},
	{"televateconverge15_",VEH_MAX_MOUNTS,ML_DOF_ELEMENT_TELEVATE15_CONV,(void*)&_VarData.TurretElevateConv[14],1,1},
	{"televateconverge16_",VEH_MAX_MOUNTS,ML_DOF_ELEMENT_TELEVATE16_CONV,(void*)&_VarData.TurretElevateConv[15],1,1},
	{"hoist",1,ML_DOF_HOIST,nullptr,0,0},
	{"vehseat",1,ML_DOF_VEH_SEAT,nullptr,0,0}
};

UINT32 mlGaugeInit()
{
UINT32 i,j;

	// set up reference table
	// this allows us to find out the appropriate DOFTransTable entry given only
	// the id number

	for (i=0;i<ML_DOF_MAXNUM;i++)
	{
		_VarDataReference[i] = 0;
		for (j=0;j<ML_DOF_MAXNUM;j++)
		{
			if (mlDOFTransTable[j].Id == i)
			{
				_VarDataReference[i] = j;
				break;
			}
		}

	}

	// now prep the name table for the DOF system
	for (i=0;i<ML_DOF_MAXNUM;i++)
	{
		j = _VarDataReference[i];

		strcpy((char*)mlDOFVarNameTable[i],(char*)mlDOFTransTable[j].Name);
	}

	return 0;
}

UINT32 mlGaugeUpdatePlayerSpeed(FLOAT64 Speed)
{
	_VarData.AirspeedKPH = (FLOAT32)(Speed * 3.6);
	_VarData.AirspeedMPH = (FLOAT32)(Speed * 2.237);
	_VarData.AirspeedKTS = (FLOAT32)(Speed * 1.969);

	return 0;
}

UINT32 _DoSpecialDOFs(mlVehicleData * VehicleData,OBJECT * Object)
{
UINT32 i;
FLOAT32 PropPos;
FLOAT32 WheelPos;
UINT32 DeltaTime;
UINT32 Alpha;
OBJECT* Object1;
char s[32];


	DeltaTime = frametime - VehicleData->AnimationData.LastAnimationTime;

	for (i=0;i<ML_MAX_ENGINES;i++)
	{

		// advance the prop
		PropPos = VehicleData->AnimationData.LastPropPosition[i];
		PropPos += (FLOAT32)(_VarData.PropRpm[i] / 60.0 *
				DeltaTime / 1000.0);


		PropPos = (FLOAT32)(PropPos - floor(PropPos));
		_VarData.PropSpin[i] = PropPos;
		_VarData.PropEffect[i] = (FLOAT32)((PropPos * 5) - floor(PropPos * 5));

		VehicleData->AnimationData.LastPropPosition[i] = PropPos;

		strprintf(s, "propstate%u", i + 1);
		if(VehicleData->AnimationData.PropFailed[i])
		{
			vehAnimChangeVehicleStateByName((vehVehicle*)VehicleData->VehicleMasterPtr, reinterpret_cast<UCHAR *>(s), FALSE);
		}else
		{
			if (_VarData.PropRpm[i] >= 300.0 &&
				_VarData.LastPropRpm[i] < 300.0)
			{
				vehAnimChangeVehicleStateByName((vehVehicle*)VehicleData->VehicleMasterPtr, reinterpret_cast<UCHAR *>(s), TRUE);
			}
			if (_VarData.PropRpm[i] < 300.0 &&
				_VarData.LastPropRpm[i] >= 300.0)
			{
				vehAnimChangeVehicleStateByName((vehVehicle*)VehicleData->VehicleMasterPtr, reinterpret_cast<UCHAR *>(s), FALSE);
			}
		}
		// set prop transparency
		sprintf((char*)s,"propa%u",i+1);
		ogrpFindSubObjectByName(&Object1,Object,(char*)s);
		if (Object1 != nullptr)
		{
			if(VehicleData->AnimationData.PropFailed[i])
			{
				Alpha = 255;
			}else
			{
				if (fabs(_VarData.PropRpm[i]) > 300.0)
				{
					Alpha = 10;
				}
				else
				{
					Alpha = (UINT32)((1.0 - (FLOAT32)fabs(_VarData.PropRpm[i] / 300.0)) * 245.0 + 10.5);
				}
			}
			_SetObjectTreeToAlpha(Object1,Alpha);
		}

		// set visdamage prop transparency
		sprintf((char*)s,"propa%u_vd",i+1);
		ogrpFindSubObjectByName(&Object1,Object,(char*)s);
		if (Object1 != nullptr)
		{
			if(VehicleData->AnimationData.PropFailed[i])
			{
				Alpha = 255;
			}else
			{
				if (fabs(_VarData.PropRpm[i]) > 300.0)
				{
					Alpha = 10;
				}
				else
				{
					Alpha = (UINT32)((1.0 - (FLOAT32)fabs(_VarData.PropRpm[i] / 300.0)) * 245.0 + 10.5);
				}
			}
			_SetObjectTreeToAlpha(Object1,Alpha);
		}
		_VarData.LastPropRpm[i] = _VarData.PropRpm[i];
	}

	for (i=0;i<ML_MAX_GEARNUM;i++)
	{
		// advance the wheels
		WheelPos = VehicleData->AnimationData.LastWheelPosition[i];
		WheelPos += (FLOAT32)(_VarData.WheelSpeed[i] * DeltaTime / 1000.0);
		WheelPos = (FLOAT32)(WheelPos - floor(WheelPos));
		_VarData.WheelRotate[i] = (FLOAT32)(1.0-WheelPos);

		VehicleData->AnimationData.LastWheelPosition[i] = WheelPos;
	}

	for (i=0;i<ML_MAX_TRACKS;i++)
	{
		// advance tracks
		WheelPos = _VarData.TrackPosition[i];
		WheelPos += (FLOAT32)(_VarData.TrackSpeed[i] * DeltaTime / 1000.0);
		if (WheelPos > 1000.0)
		{
			WheelPos -= 1000.0;
		}
		if (WheelPos < -1000.0)
		{
			WheelPos += 1000.0;
		}
		_VarData.TrackPosition[i] = WheelPos;
	}
	return 0;
}

UINT32 mlGaugeUpdatePlayerGaugeData(mlVehicleData* VehicleData,OBJECT * Object)
{
FLOAT64VECTOR TempVec1;
FLOAT64VECTOR WorldOriginOffset;
FLOAT64ATT TempAtt,TempAtt1;
FLOAT32 TempNum;
piVECTOR TempPos;
piVECTOR V1;
FLOAT64 Speed;
UINT32 DeltaTime;
FLOAT32 Delta;
UINT32 i;
UINT32 TimeOfDay;

static piVECTOR LastDir;
static SINT32 LastUpdateTime;



	DeltaTime = frametime - _VarData.LastUpdate;

	_VarData.LastUpdate = frametime;

	Speed = mathExactVectorLength(&(VehicleData->VehicleVel));

	_VarData.RightAileron = -(FLOAT32)VehicleData->AileronPos;
	_VarData.LeftAileron = (FLOAT32)VehicleData->AileronPos;
	_VarData.Elevator = (FLOAT32)VehicleData->ElevatorPos;
	_VarData.Rudder = (FLOAT32)VehicleData->RudderPos;
	for (i=0;i<ML_MAX_ENGINES;i++)
	{
		_VarData.Throttle[i] = (FLOAT32)VehicleData->ThrottlePos;
	}

	_VarData.VSIMeters = -(FLOAT32)(VehicleData->VehicleVel.z);
	_VarData.VSIFeeT = -(FLOAT32)(VehicleData->VehicleVel.z * 3.281 * 60);

	_VarData.Alt1Meter = (FLOAT32)(VehicleData->VehiclePos.z);
	_VarData.Alt10Meter = (FLOAT32)(VehicleData->VehiclePos.z);
	_VarData.Alt1Feet = (FLOAT32)(VehicleData->VehiclePos.z * 3.281);
	_VarData.Alt10Feet = (FLOAT32)(VehicleData->VehiclePos.z * 3.281);
	_VarData.Alt100Feet = (FLOAT32)(VehicleData->VehiclePos.z * 3.281);

	// turn indicator
	if (frametime != LastUpdateTime)
	{

		Delta = (frametime - LastUpdateTime) / 1000.0f;
		LastUpdateTime = frametime;

		V1.x = V1.y = V1.z = 0.0f;
		V1.y = 1.0f;

		math3dVectorMultiply(&V1,&V1,&VehicleData->VehicleAttMat);
		V1.z = 0.0f;

		math3dCrossProduct(&TempPos,&V1,&LastDir);

		LastDir = V1;

		TempNum = (FLOAT32)asin(TempPos.z);

		_VarData.TurnDeg = (-mlRAD_TO_DEG(TempNum / Delta) + _VarData.TurnDeg * 4.0f) / 5.0f;
	}

	// bank angle
	TempAtt = VehicleData->VehicleRealAtt;
	TempAtt1.Roll = TempAtt1.Yaw = 0;
	TempAtt1.Pitch = -VehicleData->VehicleRealAtt.Pitch;
	mathRotateAttByAtt(&TempAtt,&TempAtt,&TempAtt1);
	_VarData.BankDeg = -(FLOAT32)(mlRAD_TO_DEG(TempAtt.Roll));

	// Slip angle
	TempVec1 = VehicleData->VehicleRealVel;
	mathVectorMultiply64(&TempVec1,&TempVec1,&VehicleData->VehicleAttInvMat);
	TempAtt.Yaw = -atan2(TempVec1.x,TempVec1.y);
	_VarData.SlipDeg = -(FLOAT32)(mlRAD_TO_DEG(TempAtt.Yaw));

	// "fade out" slip angle if really, really slow
	if (Speed < 10)
	{
		_VarData.SlipDeg = (FLOAT32)(_VarData.SlipDeg * Speed / 10.0);
	}

	// artificial horizon
	TempVec1.x = TempVec1.y = 0.0;
	TempVec1.z = 1.0;
	mathCRotateVectorByAtt(&TempVec1,&TempVec1,&VehicleData->VehicleRealAtt);

	TempAtt.Pitch = TempAtt.Roll = TempAtt.Yaw = 0;

	TempAtt.Roll = atan2(TempVec1.x,TempVec1.z);
	mathRotateVectorByAtt(&TempVec1,&TempVec1,&TempAtt);
	TempAtt.Pitch = atan2(TempVec1.y,TempVec1.z);
	_VarData.ArtificialHorizonPitch = (FLOAT32)mlRAD_TO_DEG(TempAtt.Pitch);
	_VarData.ArtificialHorizonRoll = (FLOAT32)mlRAD_TO_DEG(TempAtt.Roll);

	// compass
	TempVec1.x = TempVec1.z = 0;
	TempVec1.y = 1.0;
	mathVectorMultiply64(&TempVec1,&TempVec1,&VehicleData->VehicleAttMat);
	_VarData.DirectionalGyro = (FLOAT32)mlRAD_TO_DEG(atan2(TempVec1.x,TempVec1.y));
	_VarData.MagneticCompass = (FLOAT32)_VarData.DirectionalGyro;

	// Turn Indicator

	// gear indicator (will change when gear damage is modelled)
	if (VehicleData->GearPosition <= 0.001)
	{
		_VarData.LeftGearUp = 1;
		_VarData.RightGearUp = 1;
		_VarData.CenterGearUp = 1;
		_VarData.LeftGearDown = 0;
		_VarData.RightGearDown = 0;
		_VarData.CenterGearDown = 0;
		_VarData.LeftGearMoving = 0;
		_VarData.RightGearMoving = 0;
		_VarData.CenterGearMoving = 0;
	}
	else if(VehicleData->GearPosition >= 0.999)
	{
		_VarData.LeftGearUp = 0;
		_VarData.RightGearUp = 0;
		_VarData.CenterGearUp = 0;
		_VarData.LeftGearDown = 1;
		_VarData.RightGearDown = 1;
		_VarData.CenterGearDown = 1;
		_VarData.LeftGearMoving = 0;
		_VarData.RightGearMoving = 0;
		_VarData.CenterGearMoving = 0;
	}
	else
	{
		_VarData.LeftGearUp = 0;
		_VarData.RightGearUp = 0;
		_VarData.CenterGearUp = 0;
		_VarData.LeftGearDown = 0;
		_VarData.RightGearDown = 0;
		_VarData.CenterGearDown = 0;
		_VarData.LeftGearMoving = 1;
		_VarData.RightGearMoving = 1;
		_VarData.CenterGearMoving = 1;
	}

	_VarData.GearPos = (FLOAT32)VehicleData->GearPosition;
	_VarData.FlapPos = (FLOAT32)VehicleData->FlapPosition;

	_VarData.LandingLightSwitch = (FLOAT32)VehicleData->LandingLightOn;
	_VarData.PanelLight = (FLOAT32)VehicleData->PanelLightOn;
	_VarData.AuxiliarySight = (FLOAT32)VehicleData->AuxSightOn;

	_VarData.MainTrigger = (FLOAT32)VehicleData->PrimaryTriggerDown;
	_VarData.SecondTrigger = (FLOAT32)VehicleData->SecondaryTriggerDown;

	_VarData.WheelSteer = (FLOAT32)VehicleData->Steering;

	for (i=0;i<ML_MAX_CREW_POSITIONS;i++)
	{
		_VarData.HatchControl[i] = (FLOAT32)VehicleData->HatchOpen[i];
		_VarData.CanopyControl[i] = (FLOAT32)VehicleData->CanopyOpen[i];

		_VarData.HatchPos[i] = (FLOAT32)VehicleData->HatchPosition[i];
		_VarData.CanopyPos[i] = (FLOAT32)VehicleData->CanopyPosition[i];

	}

	// turret positions
	for (i=0;i<ML_MAX_CREW_POSITIONS;i++)
	{
		_VarData.TurretTraverse[i] = (FLOAT32)VehicleData->TurretTraverse[i];
		_VarData.TurretElevate[i] = (FLOAT32)VehicleData->TurretElevate[i];
		_VarData.TurretTraverseWithStops[i] = (FLOAT32)VehicleData->TurretTraverseWithStops[i];
		_VarData.FlexibleTraverse[i] = (FLOAT32)VehicleData->TurretTraverseRate[i];
		_VarData.FlexibleElevate[i] = (FLOAT32)VehicleData->TurretElevateRate[i];
	}
	_VarData.RudderTrim = (FLOAT32)VehicleData->RudderTabPosition;
	_VarData.ElevatorTrim = (FLOAT32)VehicleData->ElevatorTabPosition;
	_VarData.AileronTrim = (FLOAT32)VehicleData->AileronTabPosition;

	_VarData.MainTrigger = (FLOAT32)VehicleData->PrimaryTriggerDown;
	_VarData.SecondTrigger = (FLOAT32)VehicleData->SecondaryTriggerDown;
	_VarData.OrdTrigger = (FLOAT32)VehicleData->OrdnanceTriggerDown;
//	_VarData.DiveBrake = (FLOAT32)VehicleData->DiveBrakeOn;
	_VarData.AutoDive = (FLOAT32)VehicleData->AutoDiveOn;

	if (VehicleData->BrakeForceLeft > VehicleData->BrakeForceCenter)
	{
		_VarData.LeftBrake = (FLOAT32)VehicleData->BrakeForceLeft;
	}
	else
	{
		_VarData.LeftBrake = (FLOAT32)VehicleData->BrakeForceCenter;
	}
	if (VehicleData->BrakeForceRight > VehicleData->BrakeForceCenter)
	{
		_VarData.RightBrake = (FLOAT32)VehicleData->BrakeForceRight;
	}
	else
	{
		_VarData.RightBrake = (FLOAT32)VehicleData->BrakeForceCenter;
	}
	_VarData.GeneralBrake = (FLOAT32)VehicleData->BrakeForceCenter;

	_VarData.LeftTrackClutch = (FLOAT32)VehicleData->LeftTrackClutch;
	_VarData.RightTrackClutch = (FLOAT32)VehicleData->RightTrackClutch;



	// do timed sequences
	DeltaTime = frametime - VehicleData->AnimationData.LastAnimationTime;
	Delta = (FLOAT32)(DeltaTime / 1000.0);

	{
		if (_VarData.GearHandlePos < VehicleData->GearSetting)
		{
			_VarData.GearHandlePos += Delta * 2.0f;//VehicleData->AnimationData.GearHandleMoveRate;

			if (_VarData.GearHandlePos > VehicleData->GearSetting)
			{
				_VarData.GearHandlePos = (FLOAT32)VehicleData->GearSetting;
			}
		}
		if (_VarData.GearHandlePos > VehicleData->GearSetting)
		{
			_VarData.GearHandlePos -= Delta * 2.0f;//VehicleData->AnimationData.GearHandleMoveRate;

			if (_VarData.GearHandlePos < VehicleData->GearSetting)
			{
				_VarData.GearHandlePos = (FLOAT32)VehicleData->GearSetting;
			}
		}
	}

	if (VehicleData->AnimationData.CowlFlapMoveRate[0] == 0.0)
	{
		_VarData.CowlFlap[0] = (FLOAT32)VehicleData->CowlFlapSetting[0];
	}
	else
	{
		if (_VarData.CowlFlap[0] < VehicleData->CowlFlapSetting[0])
		{
			_VarData.CowlFlap[0] += Delta * VehicleData->AnimationData.CowlFlapMoveRate[0];

			if (_VarData.CowlFlap[0] > VehicleData->CowlFlapSetting[0])
			{
				_VarData.CowlFlap[0] = (FLOAT32)VehicleData->CowlFlapSetting[0];
			}
		}
		if (_VarData.CowlFlap[0] > VehicleData->CowlFlapSetting[0])
		{
			_VarData.CowlFlap[0] -= Delta * VehicleData->AnimationData.CowlFlapMoveRate[0];

			if (_VarData.CowlFlap[0] < VehicleData->CowlFlapSetting[0])
			{
				_VarData.CowlFlap[0] = (FLOAT32)VehicleData->CowlFlapSetting[0];
			}
		}
	}

	if (VehicleData->AnimationData.FlapsSettingMoveRate == 0.0)
	{
		_VarData.FlapLever = (FLOAT32)VehicleData->FlapSetting;
	}
	else
	{
		if (_VarData.FlapLever < VehicleData->FlapSetting)
		{
			_VarData.FlapLever += Delta * 2.0f;//VehicleData->AnimationData.FlapsSettingMoveRate;

			if (_VarData.FlapLever > VehicleData->FlapSetting)
			{
				_VarData.FlapLever = (FLOAT32)VehicleData->FlapSetting;
			}
		}
		if (_VarData.FlapLever > VehicleData->FlapSetting)
		{
			_VarData.FlapLever -= Delta * 2.0f;//VehicleData->AnimationData.FlapsSettingMoveRate;

			if (_VarData.FlapLever < VehicleData->FlapSetting)
			{
				_VarData.FlapLever = (FLOAT32)VehicleData->FlapSetting;
			}
		}
	}

	_VarData.TotalAmmo = 0.0f;
	for (i=0;i<ML_MAX_MOUNTS;i++)
	{
		_VarData.TotalAmmo += _VarData.AmmoCounters[i];
	}

	TempNum = 0.0f;
	 FLOAT32 TotalFuel = 0.0f;
	for (i=0;i<ML_MAX_FUELTANKS;i++)
	{
        if(_VarData.FuelTankInUse[i] > 0.0f)
        {
		    TotalFuel += _VarData.FuelGauge[i];
		    TempNum += _VarData.FuelTankInUse[i];
        }

		_VarData.FuelTankInUse[i] = 0.0f;
	}

    if(TempNum > 0.001f)
    {
       	_VarData.TotalFuel = TotalFuel / TempNum;
    }

	trnTODGetTimeOfDay(&TimeOfDay);
	_VarData.ClockHour = (FLOAT32)(fmod((FLOAT32)TimeOfDay / 3600.0,24.0));
	_VarData.ClockMin = (FLOAT32)(fmod((FLOAT32)TimeOfDay / 60.0,60.0));
	_VarData.ClockSec = (FLOAT32)(fmod((FLOAT32)TimeOfDay,60.0f));


	// do agl check
	_VarData.AGLMeters = 0.0f;

	trnGetWorldOriginOffset(&WorldOriginOffset);
	TempPos.x = (FLOAT32)(VehicleData->AdjustedForCGPos.x - WorldOriginOffset.x);
	TempPos.y = (FLOAT32)(VehicleData->AdjustedForCGPos.y - WorldOriginOffset.y);
	TempPos.z = -(FLOAT32)(VehicleData->AdjustedForCGPos.z - WorldOriginOffset.z);

	V1 = TempPos;
	V1.z += 1000.0f;

    trncdIgnoreTreeCollide(TRUE);

	if (trncdCollideDownMediumList(&TempPos,&V1) == TRUE)
	{
		TempPos.x = (FLOAT32)(V1.x + WorldOriginOffset.x);
		TempPos.y = (FLOAT32)(V1.y + WorldOriginOffset.y);
		TempPos.z = (FLOAT32)(V1.z + WorldOriginOffset.z);
		TempPos.z = -TempPos.z;

		_VarData.AGLMeters = (FLOAT32)(VehicleData->VehiclePos.z - TempPos.z);
	}
	else
	{
		_VarData.AGLMeters = 1000.0f;
	}

    trncdIgnoreTreeCollide(FALSE);

	_VarData.AGLFeet = _VarData.AGLMeters * 2.81f;

	_DoSpecialDOFs(VehicleData,Object);

	VehicleData->AnimationData.LastAnimationTime = frametime;

	_SetDOFObjects(Object);


	return 0;
}

// traverses the object tree and sets DOF data for each object
// note that this function does siblings of "Object" right off the bat
static UINT32 _SetDOFObjects(OBJECT * Object)
{
UINT32 DOFVarNum,DOFSubVarNum;
UINT32 TableEntry;
FLOAT32 * DataPtr;
FLOAT32 VarContents;
UINT32 IgnoreCenter;
OBJECT* ObjSib;

	if (Object == nullptr)
	{
		return 0;
	}

	ObjSib = Object;

	do
	{
			if (ObjSib->flags & OBJECT_DOF)
		{
			DOFVarNum = ObjSib->dof->dofvarnum;
			DOFSubVarNum = ObjSib->dof->dofvarsubnum;
			if (DOFVarNum > 0 && DOFVarNum < ML_DOF_MAXNUM)
			{
				TableEntry = _VarDataReference[DOFVarNum];
				DataPtr = (FLOAT32*)mlDOFTransTable[TableEntry].Variable;
				IgnoreCenter = mlDOFTransTable[TableEntry].IgnoreCurrent;
				if (DataPtr != nullptr)
				{
					if (DOFSubVarNum < mlDOFTransTable[TableEntry].NumVars)
					{
						VarContents = DataPtr[DOFSubVarNum];

						ogrpSetDOFVariable(ObjSib,
											VarContents,
											mlDOFTransTable[TableEntry].VarCycles,
											IgnoreCenter);

					}
				}
			}
		}

		if (ObjSib->child != nullptr)
		{
			_SetDOFObjects(ObjSib->child);
		}

		ObjSib = ObjSib->sibling;
	}
	while (ObjSib != nullptr);

	return 0;
}

// note: WheelSpeed is in rotations per sec, watch for input values of radians/sec
// these are* NOT* the same thing (divide radians/sec by PI*2 to get rps)
UINT32 mlUpdateWheelSpeed(UINT32 GearNum, FLOAT64 WheelSpeed)
{
    if(_isnan(WheelSpeed))
    {
        WheelSpeed = 0.0f;
    }

	_VarData.WheelSpeed[GearNum] = (FLOAT32)WheelSpeed;

	return 0;
}


UINT32 mlUpdateTrackSpeed(UINT32 TrackNum, FLOAT64 TrackSpeed)
{
	_VarData.TrackSpeed[TrackNum] = (FLOAT32)TrackSpeed;

	return 0;
}

UINT32 mlGetTrackPosition(FLOAT32 * TrackPosition,UINT32 TrackNum)
{
	*TrackPosition = _VarData.TrackPosition[TrackNum];

	return 0;
}

// -1 is -180 degrees, 1 is 180 degrees, 0 is neutral
// neutral is the attitude where the vehicle is moving forward
UINT32 mlUpdateCastorAttitude(UINT32 GearNum, FLOAT64 CastorAtt)
{
	_VarData.WheelCastor[GearNum] = (FLOAT32)CastorAtt;

	return 0;
}

UINT32 mlUpdateEngineGauges(SINT32 EngineNum,
								   FLOAT64 Rpm,
								   FLOAT64 ManifoldPress,
								   UINT32 Mode,
								   UINT32 StarterPressed,
								   FLOAT64 EngTemp)
{
	if (EngineNum < 0 || EngineNum >= ML_MAX_ENGINES)
	{
		return 1;
	}
	_VarData.ManifoldPressureAtm[EngineNum] = (FLOAT32)ManifoldPress;
	_VarData.ManifoldPressureBoostCmHg[EngineNum] = (FLOAT32)(ManifoldPress * 76 - 76);
	_VarData.ManifoldPressureBoostLbs[EngineNum] = (FLOAT32)(ManifoldPress * 14.7 - 14.7);
	_VarData.ManifoldPressureInHg[EngineNum] = (FLOAT32)(ManifoldPress * 29.92);
	_VarData.Rpm[EngineNum] = (FLOAT32)Rpm;

	_VarData.StarterPos[EngineNum] = (FLOAT32)StarterPressed;
	_VarData.EngineTemp[EngineNum] = (FLOAT32)EngTemp;

	return 0;
}

UINT32 mlUpdatePropGauges(SINT32 EngineNum, FLOAT64 Angle, FLOAT64 Thrust, FLOAT64 RpmSetting, FLOAT64 Rpm, SINT32 PropAdjust)
{
	_VarData.PropPitch[EngineNum] = (FLOAT32)Angle;
	_VarData.PropRpmSetting[EngineNum] = (FLOAT32)RpmSetting;
	_VarData.PropRpm[EngineNum] = (FLOAT32)Rpm;
	_VarData.PropAdjust[EngineNum] = (FLOAT32)PropAdjust;

	return 0;
}

UINT32 mlUpdateRadiatorGauges(SINT32 TankNum, FLOAT64 Percent)
{
	_VarData.CoolantPressure[TankNum] = (FLOAT32)Percent;

	return 0;
}

UINT32 mlUpdateFuelTankGauges(SINT32 TankNum, FLOAT64 Percent)
{
	_VarData.FuelGauge[TankNum] = (FLOAT32)Percent;
	_VarData.FuelTankInUse[TankNum] = 1.0f;

	return 0;
}

UINT32 mlUpdateOilTankLevel(SINT32 TankNum, FLOAT64 Percent)
{
	_VarData.OilLevel[TankNum] = (FLOAT32)Percent;

	return 0;
}

UINT32 mlUpdateOilTankPress(SINT32 TankNum, FLOAT64 Percent)
{
	_VarData.OilPressure[TankNum] = (FLOAT32)Percent;

	return 0;
}

UINT32 mlUpdateWepTankGauges(SINT32 TankNum, FLOAT64 Percent)
{
	return 0;
}

UINT32 mlUpdateLeftSlotStatus(FLOAT32 SlotStatus)
{
	_VarData.LeftSlotOpen = SlotStatus;

	return 0;
}

UINT32 mlUpdateRightSlotStatus(FLOAT32 SlotStatus)
{
	_VarData.RightSlotOpen = SlotStatus;

	return 0;
}

UINT32 mlUpdateGearShiftState(SINT32 GearPos)
{
	_VarData.GearShiftState = (FLOAT32)GearPos;

	return 0;
}

UINT32 mlUpdateClutchState(FLOAT32 ClutchState)
{
	_VarData.ClutchState = (FLOAT32)ClutchState;

	return 0;
}

UINT32 mlUpdateGearShifter(FLOAT32 GearShiftX,FLOAT32 GearShiftY,FLOAT32 LinearGearShift)
{
	_VarData.GearShiftX = GearShiftX;
	_VarData.GearShiftY = GearShiftY;
	_VarData.LinearGearShift = LinearGearShift;

	return 0;
}

UINT32 mlUpdateAmmoCounter(UINT32 Ammo, UINT32 MountNum)
{
	if (MountNum >= ML_MAX_MOUNTS)
	{
		return 0;
	}

	_VarData.AmmoCounters[MountNum] = (FLOAT32)(Ammo);

	return 0;
}

UINT32 mlUpdateBombSightGauges(FLOAT32 Speed,FLOAT32 Alt)
{

	_VarData.BombSightSpeedSetting = Speed;
	_VarData.BombSightAltSetting = Alt;

	return 0;
}

UINT32 mlUpdateStrutCompression(SINT32 GearNum, FLOAT64 Percent)
{
	_VarData.WheelStrut[GearNum] = (FLOAT32)Percent;

	return 0;
}

UINT32 mlGetGaugeData(FLOAT32 * ReturnData,UINT32 GaugeNum,UINT32 SubGaugeNum)
{
	// Unsigned cast so negatives would go large
	if ((UINT32)GaugeNum >= ML_DOF_MAXNUM || (int)GaugeNum < 0)
	{
		*ReturnData = 0;
		return 1;
	}

	FLOAT32* VarPtr = (FLOAT32*)(mlDOFTransTable[_VarDataReference[GaugeNum]].Variable);
	if (VarPtr == nullptr)
	{
		*ReturnData = 0.0f;
	}
	else
	{
		*ReturnData = VarPtr[SubGaugeNum];
	}

	return 0;
}

static UINT32 _SetObjectTreeToAlpha(OBJECT * Object,UINT32 Alpha)
{
OBJECT * ObjAtt;

	if (Object == nullptr)
	{
		return 0;
	}

	ObjAtt = Object;

	if (!(ObjAtt->flags & OBJECT_NOTOBJECT))
	{
		do
		{
			ObjAtt->lpspare = (void*)Alpha;
			ObjAtt->flags |= OBJECT_USERDRAW;
			ObjAtt->flag2 |= OBJECT_HASALPHA;
            ObjAtt->drawcallback = _DrawAlphadProp;

			ObjAtt = ObjAtt->attached;
		}
		while (ObjAtt != nullptr);
	}

	if (Object->child != nullptr)
	{
		_SetObjectSubTreeToAlpha(Object->child,Alpha);
	}

	return 0;
}

static UINT32 _SetObjectSubTreeToAlpha(OBJECT * Object, UINT32 Alpha)
{
OBJECT * ObjSib, * ObjAtt;

	if (Object == nullptr)
	{
		return 0;
	}

	ObjSib = Object;

	do
	{
		ObjAtt = ObjSib;

		if (!(ObjAtt->flags & OBJECT_NOTOBJECT))
		{
			do
			{
				ObjAtt->lpspare = (void*)Alpha;
				ObjAtt->flags |= OBJECT_USERDRAW;
				ObjAtt->flag2 |= OBJECT_HASALPHA;
				ObjAtt->drawcallback = _DrawAlphadProp;


				ObjAtt = ObjAtt->attached;
			}
			while (ObjAtt != nullptr);
		}

		if (ObjSib->child != nullptr)
		{
			_SetObjectSubTreeToAlpha(ObjSib->child,Alpha);
		}

		ObjSib = ObjSib->sibling;
	}
	while (ObjSib != nullptr);


	return 0;
}

#if USE_STATESET
static bool _DrawAlphadProp(OBJECT* o)
{
    o->material->Apply();

    oglSetWorldMatrix(&o->mat);
    obdrawDrawMesh(o);

	return TRUE;
}
#else
static bool _DrawAlphadProp(OBJECT * Object)
{
#if(0)
	FLOAT32 savedAlpha;
	MATERIAL* propMat;

	// set material transparency
	if(piIsMaterial(Object->matid))
	{
		MATERIAL* mats;
		piGetMaterials(&mats);

		propMat = &mats[Object->matid];

		savedAlpha = propMat->material.diffuse.a;
		propMat->material.diffuse.a = (UINT32)Object->lpspare / 255.0f;
	}
#endif

    if(ogl::UseDeprecatedPath())
    {
        oglSetActiveMaterial(Object->matid);
        oglSetActiveTexture(0, Object->texid);
    }
    else
    {
        Object->material->Apply();
    }

    oglSetWorldMatrix(&Object->mat);
    obdrawDrawMesh(Object);

#if(0)
	// reset alpha
	if(propMat)
	{
		propMat->material.diffuse.a = savedAlpha;
	}
#endif

	return TRUE;
}
#endif

/************************************************************************************************
	mlSetTurretConvergence
*************************************************************************************************/

SINT32 mlSetTurretConvergence(SINT32 TurretNum, SINT32 TurretSubNum, FLOAT32 dofTraversePos, FLOAT32 dofElevatePos)
{
	if(TurretNum > 0 && TurretNum <= ML_MAX_CREW_POSITIONS && TurretSubNum > 0 && TurretSubNum <= VEH_MAX_MOUNTS)
	{
		_VarData.TurretTraverseConv[TurretNum-1][TurretSubNum-1] = dofTraversePos;
		_VarData.TurretElevateConv [TurretNum-1][TurretSubNum-1] = dofElevatePos;
	}

	return 0;
}
