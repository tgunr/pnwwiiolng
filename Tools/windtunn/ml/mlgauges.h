// header file for mlgauges.c
//
// created by Hoof

#ifndef MLGAUGES_H
#define MLGAUGES_H


#include "mltypes.h"
#include "mlweapons.h"
#include "mlgear.h"

typedef enum TAGML_DOFLIST { // type min,mid,max
					ML_DOF_NONE = 0,
					ML_DOF_CONTROL_GEARHANDLE,	// gear handle 0,1
					ML_DOF_CONTROL_GEARSHIFTX,	// h-type gearshift x pos, 0.0 = upper left (gear 1)
					ML_DOF_CONTROL_GEARSHIFTY,	// h-type gearshift y pos, 0.0 = upper left (gear 1)
					ML_DOF_CONTROL_LINEARGEARSHIFT, // linear gearshift, 0.0 = max reverse, 1.0 = max forward
					ML_DOF_CONTROL_RTRIM,		// rudder trim -1,0,1
					ML_DOF_CONTROL_ETRIM,		// elevator trim -1,0,1
					ML_DOF_CONTROL_ATRIM,		// Aileron trim -1,0,1
					ML_DOF_CONTROL_THROTTLE,	// Throttle Pos 0,1
					ML_DOF_CONTROL_MIXTURE,		// mixture pos 0,1,2 (off, lean, rich)
					ML_DOF_CONTROL_RPMLEVER,	// rpm position, rpmval
					ML_DOF_CONTROL_PITCHLEVER,	// pitch position 0-90
					ML_DOF_CONTROL_RPMROCKER,	// rpm adjuster rocker -1,0,1
					ML_DOF_CONTROL_PITCHROCKER, // pitch adjuster rocker -1,0,1
					ML_DOF_CONTROL_SUPERCHARGER,// supercharger position 0,1,2 (neutral,speed1,speed2)
					ML_DOF_CONTROL_FLAPLEVER,	// flap lever, 0,maxflaps
					ML_DOF_CONTROL_LLSWITCH,	// landing light switch, 0,1
					ML_DOF_CONTROL_BRAKELEFT,	// left gear brakes 0,1
					ML_DOF_CONTROL_BRAKERIGHT,	// right gear brakes 0,1
					ML_DOF_CONTROL_BRAKEALL,	// general gear brakes 0,1
					ML_DOF_ELEMENT_TTRAVERSEWHEEL, // turret traverse wheel 0,1
					ML_DOF_ELEMENT_TELEVATEWHEEL, // turret elevate wheel 0,1
					ML_DOF_CONTROL_LEFTTRACKCLUTCH, // left track clutch 0,1
					ML_DOF_CONTROL_RIGHTTRACKCLUTCH, // right track clutch 0,1
					ML_DOF_CONTROL_FUELCOCK,	// fuel selector 0,maxtanks
					ML_DOF_CONTROL_COWLLEVER,	// flap control 0,maxflapnum
					ML_DOF_CONTROL_PANELLIGHT,	// instrument panel light 0,1
					ML_DOF_CONTROL_AUXSIGHT,	// auxiliary sight control 0,1
					ML_DOF_CONTROL_RADIO,		// radio switch 0,1
					ML_DOF_CONTROL_HATCHCONTROL,// hatch control/handle 0,1
					ML_DOF_CONTROL_CANOPYCONTROL,// canopy control/handle 0,1
					ML_DOF_CONTROL_MAINTRIGGER,	// main weapon trigger 0,1
					ML_DOF_CONTROL_SECONDTRIGGER,// secondary weapon trigger 0,1
					ML_DOF_CONTROL_ORDTRIGGER,	// ordinance trigger 0,1
					ML_DOF_CONTROL_ORDSELECTOR,	// ordinance selector 0,maxhardpoints
					ML_DOF_CONTROL_DIVEBRAKE,	// dive brake control 0,1
					ML_DOF_CONTROL_AUTODIVE,	// autodive system control 0,1
					ML_DOF_CONTROL_MAP,			// map mode 0,1
					ML_DOF_CONTROL_STARTER,		// starter 0,1
					ML_DOF_ELEMENT_LAILERON,	// left aileron position -1,0,1
					ML_DOF_ELEMENT_RAILERON,	// right aileron position -1,0,1
					ML_DOF_ELEMENT_ELEVATOR,	// elevator position -1,0,1
					ML_DOF_ELEMENT_RUDDER,		// rudder position -1,0,1
					ML_DOF_ELEMENT_PROPSPIN,	// propeller spin position 0,1
					ML_DOF_ELEMENT_PROPPITCH,	// propeller pitch 0,90
					ML_DOF_ELEMENT_PROPPITCHDIAL, // propeller pitch (cycles) 0,90
					ML_DOF_ELEMENT_PROPEFFECT,	// propeller effect spin position 0,1
					ML_DOF_ELEMENT_FLAP,		// flap position 0,maxflapposition
					ML_DOF_ELEMENT_GEAR,		// gear position 0,1
					ML_DOF_ELEMENT_CANOPY,		// canopy position 0,1
					ML_DOF_ELEMENT_BODY,		// body position dof (special case)
					ML_DOF_ELEMENT_EYE,			// eye position dof (special case)
					ML_DOF_ELEMENT_HEAD,		// head position dof (special case)
					ML_DOF_ELEMENT_HATCH,		// hatch position 0,1
					ML_DOF_ELEMENT_BOMBBAYDOOR, // bomb bay door position 0,1
					ML_DOF_ELEMENT_TTRAVERSE,	// turret traverse -1,0,1 for -180 to 180 deg
					ML_DOF_ELEMENT_TELEVATE,	// turret elevate -1,0,1 for min-max
					ML_DOF_ELEMENT_TRAVERSELIMITED, // turret traverse, with stops
					ML_DOF_ELEMENT_FLEXTRAVERSE,	// flexible mount traverse
					ML_DOF_ELEMENT_FLEXELEVATE,		// flexible mount elevation
					ML_DOF_ELEMENT_LEFTSLOTOPEN,// left slot is open, 0,1
					ML_DOF_ELEMENT_RIGHTSLOTOPEN,// right slot is open, 0,1
					ML_DOF_ELEMENT_GEARSHIFT,	// negative is reverse, zero is neutral
					ML_DOF_ELEMENT_CLUTCHSTATE,	// clutch state, 0.0 to 1.0
					ML_DOF_GAUGE_ASIMPH,		// airspeed indicator, mph
					ML_DOF_GAUGE_ASIKPH,		// airspeed indicator, kph
					ML_DOF_GAUGE_ASIKTS,		// airspeed indicator, knots
					ML_DOF_GAUGE_ASIMPHSTOP,	// airspeed indicator, mph, with stops
					ML_DOF_GAUGE_ASIKPHSTOP,	// airspeed indicator, kph, with stops
					ML_DOF_GAUGE_ASIKTSSTOP,	// airspeed indicator, kts, with stops
					ML_DOF_GAUGE_VSIFT,			// vertical speed indicator ft/min
					ML_DOF_GAUGE_VSIM,			// vertical speed indicator m/s
					ML_DOF_GAUGE_RPM,			// engine RPM gauge
					ML_DOF_GAUGE_PROPRPMSETTING,// propeller RPM setting
					ML_DOF_GAUGE_PROPADJUST,	// propeller adjust setting
					ML_DOF_GAUGE_ALT1FT,		// altimeter 1ft arm
					ML_DOF_GAUGE_ALT10FT,		// altimeter 10ft arm
					ML_DOF_GAUGE_ALT100FT,		// altimeter 100ft arm
					ML_DOF_GAUGE_ALT1M,			// altimeter 1m arm
					ML_DOF_GAUGE_ALT10M,		// altimeter 10m arm
					ML_DOF_GAUGE_AGLMETERS,		// above ground level in meters
					ML_DOF_GAUGE_AGLFEET,		// above ground level in feet
					ML_DOF_GAUGE_SLIPDEG,		// slip indicator, deg
					ML_DOF_GAUGE_TURNDEG,		// turn indicator, deg
					ML_DOF_GAUGE_BANKDEG,		// bank indicator, deg
					ML_DOF_GAUGE_CLOCKHR,		// hour hand of clock
					ML_DOF_GAUGE_CLOCKMIN,		// minute hand of clock
					ML_DOF_GAUGE_CLOCKSEC,		// second hand of clock
					ML_DOF_GAUGE_OXYGEN,		// oxygen remaining indicator
					ML_DOF_GAUGE_ARTIFICIALHPITCH,// artificial horizon pitch dof
					ML_DOF_GAUGE_ARTIFICIALHROLL,// artificial horizon roll dof
					ML_DOF_GAUGE_DGYRO,			// directional gyro
					ML_DOF_GAUGE_MAGCOMPASS,	// magnetic compass
					ML_DOF_GAUGE_FUELGAUGE,		// fuel gauge
					ML_DOF_GAUGE_TOTALFUEL,		// total fuel
					ML_DOF_GAUGE_MANPRESSCMHG,	// manifold pressure boost, cm/hg
					ML_DOF_GAUGE_MANPRESSINHG,	// manifold pressure, in/hg
					ML_DOF_GAUGE_MANPRESSLBS,	// manifold pressure boost, in lbs
					ML_DOF_GAUGE_MANPRESSATM,	// manifold pressure, atmospheres
					ML_DOF_GAUGE_OILPRESS,		// oil pressure
					ML_DOF_GAUGE_OILLEVEL,		// oil level
					ML_DOF_GAUGE_COOLPRESS,		// coolant pressure
					ML_DOF_GAUGE_ENGTEMP,		// engine temperature farenheit
					ML_DOF_GAUGE_GEARLUP,		// gear left is up
					ML_DOF_GAUGE_GEARRUP,		// gear right is up
					ML_DOF_GAUGE_GEARCUP,		// gear center is up
					ML_DOF_GAUGE_GEARLDOWN,		// gear left is down
					ML_DOF_GAUGE_GEARRDOWN,		// gear right is down
					ML_DOF_GAUGE_GEARCDOWN,		// gear center is down
					ML_DOF_GAUGE_GEARLMOV,		// gear left is moving
					ML_DOF_GAUGE_GEARRMOV,		// gear right is moving
					ML_DOF_GAUGE_GEARCMOV,		// gear center is moving
					ML_DOF_GAUGE_AMMOINDICATOR,	// mount ammo counter
					ML_DOF_GAUGE_TOTALAMMO,		// total ammo onboard counter
					ML_DOF_GAUGE_BOMBSIGHT_SPEED, // bombsight speed setting
					ML_DOF_GAUGE_BOMBSIGHT_ALT,	// bombsight alt setting
					ML_DOF_BOMBSIGHT_PIVOT,		// british mk9a pivot point
					ML_DOF_BOMBSIGHT_BACKSIGHT,	// british mk9a backsight point
					ML_DOF_BOMBSIGHT_FORESIGHT,	// british mk9a foresight point
					ML_DOF_IK_LINK1,
					ML_DOF_IK_LINK2,
					ML_DOF_IK_LINK3,
					ML_DOF_IK_CHAINEND,
					ML_DOF_IK_ENDEFFECTOR,
					ML_DOF_WHEEL_CONTACTPOINT,	// contact point for wheel vs other objects
					ML_DOF_WHEEL_ROTATE,		// rotation DOF to make wheel spin
					ML_DOF_WHEEL_STRUT,			// strut DOF for strut stuff
					ML_DOF_WHEEL_BOGIE,			
					ML_DOF_WHEEL_CASTOR,		// if the wheel castors freely it uses this DOF
					ML_DOF_WHEEL_STEER,			// steering DOF
					ML_DOF_RECOIL,				// gun recoil DOF

					ML_DOF_MOUNT,				// weapon mount DOF
					ML_DOF_CONNECT,				// weapon attachment point
					ML_DOF_FIREPOINT,			// weapon fire point
					ML_DOF_CRAPPOINT,			// weapon fire crap point (crap = ejected stuff like casings)
					ML_DOF_TOWPOINT_LIGHT,		// towing point for towing vehicles
					ML_DOF_TOWPOINT_MEDIUM,		// towing point for towing vehicles
					ML_DOF_TOWPOINT_HEAVY,		// towing point for towing vehicles
					ML_DOF_FPS_SEAT,			// fps seat on another vehicle
					ML_DOF_POINTCOLLIDE,		// point collider DOF
					ML_DOF_VIEWPOINT,			// used for positioning the camera specifically

					ML_DOF_SPAWNLOCATION,		// used to specify spawn location and orientation
					ML_DOF_AG_LOCATE,			// autogun location point
					ML_DOF_AG_ELEVATE,			// autogun elevation dof
					ML_DOF_AG_TRAVERSE,			// autogun traverse dof
					ML_DOF_AG_VIEW,				// autogun view point
					ML_DOF_AG_FIRE,				// autogun fire point

					ML_DOF_DEMOVAR,				// used for demos for positioning stuff fast
				
					// convergence DOF's
					ML_DOF_ELEMENT_TTRAVERSE1_CONV,
					ML_DOF_ELEMENT_TTRAVERSE2_CONV,
					ML_DOF_ELEMENT_TTRAVERSE3_CONV,
					ML_DOF_ELEMENT_TTRAVERSE4_CONV,
					ML_DOF_ELEMENT_TTRAVERSE5_CONV,
					ML_DOF_ELEMENT_TTRAVERSE6_CONV,
					ML_DOF_ELEMENT_TTRAVERSE7_CONV,
					ML_DOF_ELEMENT_TTRAVERSE8_CONV,
					ML_DOF_ELEMENT_TTRAVERSE9_CONV,
					ML_DOF_ELEMENT_TTRAVERSE10_CONV,
					ML_DOF_ELEMENT_TTRAVERSE11_CONV,
					ML_DOF_ELEMENT_TTRAVERSE12_CONV,
					ML_DOF_ELEMENT_TTRAVERSE13_CONV,
					ML_DOF_ELEMENT_TTRAVERSE14_CONV,
					ML_DOF_ELEMENT_TTRAVERSE15_CONV,
					ML_DOF_ELEMENT_TTRAVERSE16_CONV,

					ML_DOF_ELEMENT_TELEVATE1_CONV,
					ML_DOF_ELEMENT_TELEVATE2_CONV,
					ML_DOF_ELEMENT_TELEVATE3_CONV,
					ML_DOF_ELEMENT_TELEVATE4_CONV,
					ML_DOF_ELEMENT_TELEVATE5_CONV,
					ML_DOF_ELEMENT_TELEVATE6_CONV,
					ML_DOF_ELEMENT_TELEVATE7_CONV,
					ML_DOF_ELEMENT_TELEVATE8_CONV,
					ML_DOF_ELEMENT_TELEVATE9_CONV,
					ML_DOF_ELEMENT_TELEVATE10_CONV,
					ML_DOF_ELEMENT_TELEVATE11_CONV,
					ML_DOF_ELEMENT_TELEVATE12_CONV,
					ML_DOF_ELEMENT_TELEVATE13_CONV,
					ML_DOF_ELEMENT_TELEVATE14_CONV,
					ML_DOF_ELEMENT_TELEVATE15_CONV,
					ML_DOF_ELEMENT_TELEVATE16_CONV,
					
					ML_DOF_HOIST,
					ML_DOF_VEH_SEAT,
					
					ML_DOF_MAXNUM
} mlGaugeDOFList;

typedef struct tagMLGAUGETRANSLATIONTABLE
{
	UCHAR Name[32];
	UCHAR NumVars;
	UINT32 Id;
	void * Variable;
	UINT32 VarCycles;
	UINT32 IgnoreCurrent;
}	mlGaugeTranslationTable;

// all gauges are SINT32s so that the gauges can be referred to by number later
// no floats used because many gauges require state1/state2/etc settings that
// floats can't always do precisely, while rarely is a lot of precision (more than a few
// digits) required for gauges
typedef struct tagmlVehicleVarData
{
	// controls
	FLOAT32 GearHandlePos;
	FLOAT32 GearShiftX;
	FLOAT32 GearShiftY;
	FLOAT32 LinearGearShift;
	FLOAT32 RudderTrim;
	FLOAT32 ElevatorTrim;
	FLOAT32 AileronTrim;
	FLOAT32 Throttle[ML_MAX_ENGINES];
	FLOAT32 Mixture[ML_MAX_ENGINES];
	FLOAT32 RpmLever[ML_MAX_ENGINES];
	FLOAT32 PitchLever[ML_MAX_ENGINES];
	FLOAT32 RpmRocker[ML_MAX_ENGINES];
	FLOAT32 PitchRocker[ML_MAX_ENGINES];
	FLOAT32 SuperChargerSetting[ML_MAX_ENGINES];
	FLOAT32 FlapLever;
	FLOAT32 LandingLightSwitch;
	FLOAT32 LeftBrake;
	FLOAT32 RightBrake;
	FLOAT32 GeneralBrake;
	FLOAT32 LeftTrackClutch;
	FLOAT32 RightTrackClutch;
	FLOAT32 FuelCock[ML_MAX_ENGINES];
	FLOAT32 CowlFlap[ML_MAX_ENGINES];
	FLOAT32 PanelLight;
	FLOAT32 AuxiliarySight;
	FLOAT32 Radio;
	FLOAT32 HatchControl[ML_MAX_CREW_POSITIONS];
	FLOAT32 CanopyControl[ML_MAX_CREW_POSITIONS];
	FLOAT32 MainTrigger;
	FLOAT32 SecondTrigger;
	FLOAT32 OrdTrigger;
	FLOAT32 OrdSelector;
	FLOAT32 DiveBrake;
	FLOAT32 AutoDive;
	FLOAT32 Map;

	// elements
	FLOAT32 LeftAileron;
	FLOAT32 RightAileron;
	FLOAT32 Elevator;
	FLOAT32 Rudder;
	FLOAT32 PropSpin[ML_MAX_ENGINES];
	FLOAT32 PropPitch[ML_MAX_ENGINES];
	FLOAT32 PropEffect[ML_MAX_ENGINES];
	FLOAT32 PropRpm[ML_MAX_ENGINES];
	FLOAT32 LastPropRpm[ML_MAX_ENGINES];
	FLOAT32 PropRpmSetting[ML_MAX_ENGINES];
	FLOAT32 PropAdjust[ML_MAX_ENGINES];
	FLOAT32 FlapPos;
	FLOAT32 GearPos;
	FLOAT32 CanopyPos[ML_MAX_CREW_POSITIONS];
	FLOAT32 HatchPos[ML_MAX_CREW_POSITIONS];
	FLOAT32 LeftSlotOpen;
	FLOAT32 RightSlotOpen;

	// Instruments
	FLOAT32 AirspeedMPH;
	FLOAT32 AirspeedKPH;
	FLOAT32 AirspeedKTS;
	FLOAT32 VSIFeeT;
	FLOAT32 VSIMeters;
	FLOAT32 Rpm[ML_MAX_ENGINES];
	FLOAT32 Alt1Feet;
	FLOAT32 Alt10Feet;
	FLOAT32 Alt100Feet;
	FLOAT32 Alt1Meter;
	FLOAT32 Alt10Meter;
	FLOAT32 AGLMeters;
	FLOAT32 AGLFeet;
	FLOAT32 SlipDeg;
	FLOAT32 TurnDeg;
	FLOAT32 BankDeg;
	FLOAT32 ClockHour;
	FLOAT32 ClockMin;
	FLOAT32 ClockSec;
	FLOAT32 Oxygen;
	FLOAT32 ArtificialHorizonPitch;
	FLOAT32 ArtificialHorizonRoll;
	FLOAT32 DirectionalGyro;
	FLOAT32 MagneticCompass;
	FLOAT32 FuelGauge[ML_MAX_FUELTANKS];
	FLOAT32 FuelTankInUse[ML_MAX_FUELTANKS];
	FLOAT32 TotalFuel;
	FLOAT32 ManifoldPressureBoostCmHg[ML_MAX_ENGINES];
	FLOAT32 ManifoldPressureInHg[ML_MAX_ENGINES];
	FLOAT32 ManifoldPressureBoostLbs[ML_MAX_ENGINES];
	FLOAT32 ManifoldPressureAtm[ML_MAX_ENGINES];
	FLOAT32 OilPressure[ML_MAX_ENGINES];
	FLOAT32 OilLevel[ML_MAX_ENGINES];
	FLOAT32 CoolantPressure[ML_MAX_ENGINES];
	FLOAT32 EngineTemp[ML_MAX_ENGINES];
	FLOAT32 LeftGearUp;
	FLOAT32 RightGearUp;
	FLOAT32 CenterGearUp;
	FLOAT32 LeftGearDown;
	FLOAT32 RightGearDown;
	FLOAT32 CenterGearDown;
	FLOAT32 LeftGearMoving;
	FLOAT32 RightGearMoving;
	FLOAT32 CenterGearMoving;

	FLOAT32 TurretTraverse[ML_MAX_CREW_POSITIONS];
	FLOAT32 TurretElevate[ML_MAX_CREW_POSITIONS];
	FLOAT32 TurretTraverseWithStops[ML_MAX_CREW_POSITIONS];
	FLOAT32 FlexibleTraverse[ML_MAX_CREW_POSITIONS];
	FLOAT32 FlexibleElevate[ML_MAX_CREW_POSITIONS];

	FLOAT32 GearShiftState;
	FLOAT32 ClutchState;

	FLOAT32 StarterPos[ML_MAX_ENGINES];

	FLOAT32 AmmoCounters[ML_MAX_MOUNTS];
	FLOAT32 TotalAmmo;

	FLOAT32 BombSightSpeedSetting;
	FLOAT32 BombSightAltSetting;

	FLOAT32 WheelSpeed[ML_MAX_GEARNUM];
	FLOAT32 WheelRotate[ML_MAX_GEARNUM];
	FLOAT32 WheelStrut[ML_MAX_GEARNUM];
	FLOAT32 WheelBogie[ML_MAX_GEARNUM];
	FLOAT32 WheelCastor[ML_MAX_GEARNUM];
	FLOAT32 TrackSpeed[ML_MAX_TRACKS];
	FLOAT32 TrackPosition[ML_MAX_TRACKS];
	FLOAT32 WheelSteer;

	UINT32 LastUpdate;
	FLOAT32 LastDirectionGyro;

	FLOAT32 DemoVar; // used for demo purposes only, should always be zero
	
	FLOAT32 TurretTraverseConv[ML_MAX_CREW_POSITIONS][VEH_MAX_MOUNTS];
	FLOAT32 TurretElevateConv[ML_MAX_CREW_POSITIONS][VEH_MAX_MOUNTS];

} mlVehicleVarData;



#ifdef __cplusplus
extern "C"
{
#endif

extern mlVehicleVarData _VarData;

extern UINT32 mlGaugeInit(void);

extern UINT32 mlGaugeUpdatePlayerSpeed(FLOAT64 Speed);
extern UINT32 mlGaugeUpdatePlayerGaugeData(mlVehicleData *VehicleData,OBJECT * Object);
extern UINT32 mlGetGaugeData(FLOAT32 * ReturnData,UINT32 GaugeNum,UINT32 SubGaugeNum);
extern UINT32 mlUpdateEngineGauges(SINT32 EngineNum, 
								   FLOAT64 Rpm, 
								   FLOAT64 ManifoldPress, 
								   UINT32 Mode,
								   UINT32 StarterPressed,
								   FLOAT64 EngTemp);
extern UINT32 mlUpdatePropGauges(SINT32 EngineNum, FLOAT64 Angle, FLOAT64 Thrust, FLOAT64 RpmSetting, FLOAT64 Rpm, SINT32 PropAdjust);
extern UINT32 mlUpdateRadiatorGauges(SINT32 TankNum, FLOAT64 Percent);
extern UINT32 mlUpdateFuelTankGauges(SINT32 TankNum, FLOAT64 Percent);
extern UINT32 mlUpdateOilTankLevel(SINT32 TankNum, FLOAT64 Percent);
extern UINT32 mlUpdateOilTankPress(SINT32 TankNum, FLOAT64 Percent);
extern UINT32 mlUpdateWepTankGauges(SINT32 TankNum, FLOAT64 Percent);
extern UINT32 mlUpdateStrutCompression(SINT32 GearNum, FLOAT64 Percent);
extern UINT32 mlUpdateWheelSpeed(UINT32 GearNum, FLOAT64 WheelSpeed);
extern UINT32 mlUpdateCastorAttitude(UINT32 GearNum, FLOAT64 CastorAtt);
extern UINT32 mlUpdateTrackSpeed(UINT32 TrackNum, FLOAT64 TrackSpeed);
extern UINT32 mlUpdateLeftSlotStatus(FLOAT32 SlotStatus);
extern UINT32 mlUpdateRightSlotStatus(FLOAT32 SlotStatus);
extern UINT32 mlUpdateGearShiftState(SINT32 GearPos);
extern UINT32 mlUpdateClutchState(FLOAT32 ClutchState);
extern UINT32 mlUpdateGearShifter(FLOAT32 GearShiftX,FLOAT32 GearShiftY,FLOAT32 LinearGearShift);
extern UINT32 mlUpdateAmmoCounter(UINT32 Ammo, UINT32 MountNum);
extern UINT32 mlUpdateBombSightGauges(FLOAT32 Speed,FLOAT32 Alt);

extern UCHAR mlDOFVarNameTable[ML_DOF_MAXNUM][32];

extern UINT32 mlGetTrackPosition(FLOAT32 * TrackPosition,UINT32 TrackNum);

// one based turretnum
SINT32 mlSetTurretConvergence(SINT32 TurretNum, SINT32 TurretSubNum,
								FLOAT32 dofTraversePos, FLOAT32 dofElevatePos);
#ifdef __cplusplus
}
#endif

#endif