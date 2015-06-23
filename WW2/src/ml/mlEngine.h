#pragma once

#include "ml/mlCompon.h"
#include "ml/mlobject.h"
#include "ml/mltypes.h"

enum
{
	ML_ENGINE_VARNAME_NONE,
	ML_ENGINE_VARNAME_TYPE,
	ML_ENGINE_VARNAME_BASEHORSEPOWER,
	ML_ENGINE_VARNAME_BASEHPALT,
	ML_ENGINE_VARNAME_BASEHPPRESS,
	ML_ENGINE_VARNAME_BASEHPRPM,
	ML_ENGINE_VARNAME_FUELPERHPSEC,
	ML_ENGINE_VARNAME_OILCONSUMPTIONPERSEC,
	ML_ENGINE_VARNAME_NEG_G_FUEL_CUTOUT,
	ML_ENGINE_VARNAME_NEG_G_TIME_LIMIT,
	ML_ENGINE_VARNAME_WEPMAXPRESS,
	ML_ENGINE_VARNAME_MILMAXPRESS,
	ML_ENGINE_VARNAME_CLIMBMAXPRESS,
	ML_ENGINE_VARNAME_ECONMAXPRESS,
	ML_ENGINE_VARNAME_MAXWEPTIME,
	ML_ENGINE_VARNAME_MAXMILTIME,
	ML_ENGINE_VARNAME_WEP_RPM,
	ML_ENGINE_VARNAME_MIL_RPM,
	ML_ENGINE_VARNAME_CLIMB_RPM,
	ML_ENGINE_VARNAME_ECON_RPM,
	ML_ENGINE_VARNAME_SUPERCHARGETYPE,
	ML_ENGINE_VARNAME_SPEED1CRITALT,
	ML_ENGINE_VARNAME_SPEED2CRITALT,
	ML_ENGINE_VARNAME_SHIFTALT,
	ML_ENGINE_VARNAME_NOTUSED,
	ML_ENGINE_VARNAME_ALSONOTUSED,
	ML_ENGINE_VARNAME_EXHAUSTTHRUSTANGLE,
	ML_ENGINE_VARNAME_WEPTYPE,
	ML_ENGINE_VARNAME_WEPCONSUMPTIONRATE,
	ML_ENGINE_VARNAME_PROPGEARRATIO,
	ML_ENGINE_VARNAME_IDLEPRESSURE,
	ML_ENGINE_VARNAME_FUELPERHPSECECON,
	ML_ENGINE_VARNAME_SUPERCHARGING_ALTFLOOR,
	ML_ENGINE_VARNAME_SPEED1HPEFFICIENCYTOP,
	ML_ENGINE_VARNAME_SPEED2HPEFFICIENCYTOP,
	ML_ENGINE_VARNAME_PROPSPINDIRECTION,
	ML_ENGINE_VARNAME_EXHAUSTTHRUST,
	ML_ENGINE_VARNAME_IDLERPM,
	ML_ENGINE_VARNAME_SPEED1HPEFFICIENCYBOTTOM,
	ML_ENGINE_VARNAME_SPEED2HPEFFICIENCYBOTTOM,
	ML_ENGINE_VARNAME_MANIFOLDUNITS,
	ML_ENGINE_VARNAME_IDLEPRESSUREATMAXRPM,
	ML_ENGINE_VARNAME_IDLETHROTTLEPOS,
	ML_ENGINE_VARNAME_BASEHPEXHAUSTTEMP,
	ML_ENGINE_VARNAME_CRITICALRPM,
	ML_ENGINE_VARNAME_STARTERRPM,
	ML_ENGINE_VARNAME_STARTERHP,
	ML_ENGINE_VARNAME_SPEED1MAXMILBOOST,
	ML_ENGINE_VARNAME_SPEED1MAXWEPBOOST,
	ML_ENGINE_VARNAME_SPEED2MAXMILBOOST,
	ML_ENGINE_VARNAME_SPEED2MAXWEPBOOST,
	ML_ENGINE_VARNAME_NEUTRALSHIFTALT,
	ML_ENGINE_VARNAME_NEUTRALMAXMILBOOST,
	ML_ENGINE_VARNAME_NEUTRALMAXWEPBOOST,
	ML_ENGINE_VARNAME_MAXPOWERCOOLSPEED,
	ML_ENGINE_VARNAME_ENGINETEMPSCALEFACTOR,
	ML_ENGINE_VARNAME_MAXSAFERPM,
	ML_ENGINE_VARNAME_SPEED1MAXCLIMBBOOST,
	ML_ENGINE_VARNAME_MAXINJWEPBOOST,
	ML_ENGINE_MAX_VARNAME
};

#define ML_ENGINE_WEPTYPE_NORMAL 1
#define ML_ENGINE_WEPTYPE_MW 2
#define ML_ENGINE_WEPTYPE_NO2 3
#define ML_ENGINE_WEPTYPE_WATER 4

#define ML_ENGINE_MAX_WEPTYPE 5

#define ML_ENGINE_MANIFOLD_UNDEFINED 0
#define ML_ENGINE_MANIFOLD_CMHGBOOST 1
#define ML_ENGINE_MANIFOLD_INHG 2
#define ML_ENGINE_MANIFOLD_LBSBOOST 3
#define ML_ENGINE_MANIFOLD_ATM 4

#define ML_MAX_ENGINE_MANIFOLDTYPES 5

#define ML_ENGINE_NOSUPERCHARGING 0 // normally aspirated engine
#define ML_ENGINE_SINGLESPEED 1 // single-speed supercharger
#define ML_ENGINE_DOUBLESPEED 2 // 2 speed selectable supercharger
#define ML_ENGINE_TURBO 3 // turbosupercharger
#define ML_ENGINE_SINGLESPEEDWITHNEUTRAL 4 // single-speed supercharger with neutral blower
#define ML_ENGINE_DOUBLESPEEDWITHNEUTRAL 5 // 2 speed selectable supercharger with neutral blower

#define ML_ENGINE_MAX_SUPERCHARGETYPES 6

enum
{
	ML_ENGINE_MODE_OFF,
	ML_ENGINE_MODE_WEP,
	ML_ENGINE_MODE_MIL,
	ML_ENGINE_MODE_CLIMB,
	ML_ENGINE_MODE_ECON,
	ML_ENGINE_MODE_INJWEP
};

#define ML_ENGINE_MAX_MODE 5

#define ML_ENGINE_SUBTYPE_PROPELLOR 1
#define ML_ENGINE_SUBTYPE_ENGINE 2
#define ML_ENGINE_SUBTYPE_FUELTANK 3
#define ML_ENGINE_SUBTYPE_OILTANK 4
#define ML_ENGINE_SUBTYPE_RADIATOR 5
#define ML_ENGINE_SUBTYPE_WEPTANK 6
#define ML_ENGINE_SUBTYPE_TRANSMISSION 7
#define ML_ENGINE_SUBTYPE_TORQUEGENERATOR 8
#define ML_ENGINE_SUBTYPE_CLUTCH 9
#define ML_ENGINE_SUBTYPE_GEARBOX 10

#define ML_ENGINE_MAX_SUBTYPES 11

#define ML_ENGINE_MAX_ENGINETYPES 2

#define ML_ENGINECOM_VARNAME_ENGINETYPE 1
#define ML_ENGINECOM_VARNAME_PROPELLORCOM 2
#define ML_ENGINECOM_VARNAME_OILTANKCOM 3
#define ML_ENGINECOM_VARNAME_RADIATOR1COM 4
#define ML_ENGINECOM_VARNAME_RADIATOR2COM 5
#define ML_ENGINECOM_VARNAME_RADIATOR3COM 6
#define ML_ENGINECOM_VARNAME_RADIATOR4COM 7
#define ML_ENGINECOM_VARNAME_FUELTANK1COM 8
#define ML_ENGINECOM_VARNAME_FUELTANK2COM 9
#define ML_ENGINECOM_VARNAME_FUELTANK3COM 10
#define ML_ENGINECOM_VARNAME_FUELTANK4COM 11
#define ML_ENGINECOM_VARNAME_FUELTANK5COM 12
#define ML_ENGINECOM_VARNAME_FUELTANK6COM 13
#define ML_ENGINECOM_VARNAME_FUELTANK7COM 14
#define ML_ENGINECOM_VARNAME_FUELTANK8COM 15
#define ML_ENGINECOM_VARNAME_WEPTANK 16

#define ML_ENGINECOM_VARNAME_TANKNUM 17

#define ML_ENGINECOM_FUELTANKTYPE_INTEGRAL 1
#define ML_ENGINECOM_FUELTANKTYPE_INTEGRAL_LIGHTSELFSEALING 2
#define ML_ENGINECOM_FUELTANKTYPE_BAG_NOTSELFSEALING 3
#define ML_ENGINECOM_FUELTANKTYPE_BAG_LIGHTSELFSEALING 4
#define ML_ENGINECOM_FUELTANKTYPE_BAG_GOODSELFSEALING 5
#define ML_ENGINECOM_FUELTANKTYPE_INTEGRALWITHEXTINGUISHER 6

#define ML_ENGINECOM_MAX_FUELTANKTYPE 7

#define ML_ENGINECOM_PROPDIAMETER 18
#define ML_ENGINECOM_PROPEFFICIENCY 19
#define ML_ENGINECOM_PROPAREA 20
#define ML_ENGINECOM_ENGINENUM 21
#define ML_ENGINECOM_VARNAME_PROPELLER_MDIV 22
#define ML_ENGINECOM_VARNAME_PROPELLER_MAXCL 23
#define ML_ENGINECOM_VARNAME_PROPELLER_MINCL 24
#define ML_ENGINECOM_VARNAME_PROPELLER_RESPONSE 25
#define ML_ENGINECOM_VARNAME_PROPELLER_MAXANGLE 26
#define ML_ENGINECOM_VARNAME_PROPELLER_MINANGLE 27
#define ML_ENGINECOM_VARNAME_PROPELLER_CD 28
#define ML_ENGINECOM_VARNAME_PROPELLER_MDIV_10DEG 29
#define ML_ENGINECOM_VARNAME_PROPELLER_EFFICIENCY 30

#define ML_ENGINECOM_VARNAME_TRANSTYPE 31
#define ML_ENGINECOM_VARNAME_FLYWHEEL_ROTINERTIA 32
#define ML_ENGINECOM_VARNAME_NUMFORWARDGEARS 33
#define ML_ENGINECOM_VARNAME_NUMREVERSEGEARS 34
#define ML_ENGINECOM_VARNAME_FORWARDGEAR1RATIO 35
#define ML_ENGINECOM_VARNAME_FORWARDGEAR2RATIO 36
#define ML_ENGINECOM_VARNAME_FORWARDGEAR3RATIO 37
#define ML_ENGINECOM_VARNAME_FORWARDGEAR4RATIO 38
#define ML_ENGINECOM_VARNAME_FORWARDGEAR5RATIO 39
#define ML_ENGINECOM_VARNAME_FORWARDGEAR6RATIO 40
#define ML_ENGINECOM_VARNAME_FORWARDGEAR7RATIO 41
#define ML_ENGINECOM_VARNAME_FORWARDGEAR8RATIO 42
#define ML_ENGINECOM_VARNAME_FORWARDGEAR9RATIO 43
#define ML_ENGINECOM_VARNAME_FORWARDGEAR10RATIO 44
#define ML_ENGINECOM_VARNAME_FORWARDGEAR11RATIO 45
#define ML_ENGINECOM_VARNAME_FORWARDGEAR12RATIO 46
#define ML_ENGINECOM_VARNAME_FORWARDGEAR13RATIO 47
#define ML_ENGINECOM_VARNAME_FORWARDGEAR14RATIO 48
#define ML_ENGINECOM_VARNAME_FORWARDGEAR15RATIO 49
#define ML_ENGINECOM_VARNAME_FORWARDGEAR16RATIO 50
#define ML_ENGINECOM_VARNAME_REVERSEGEAR1RATIO 51
#define ML_ENGINECOM_VARNAME_REVERSEGEAR2RATIO 52
#define ML_ENGINECOM_VARNAME_REVERSEGEAR3RATIO 53
#define ML_ENGINECOM_VARNAME_REVERSEGEAR4RATIO 54
#define ML_ENGINECOM_VARNAME_REVERSEGEAR5RATIO 55
#define ML_ENGINECOM_VARNAME_REVERSEGEAR6RATIO 56
#define ML_ENGINECOM_VARNAME_REVERSEGEAR7RATIO 57
#define ML_ENGINECOM_VARNAME_REVERSEGEAR8RATIO 58
#define ML_ENGINECOM_VARNAME_DRIVEWHEEL1 59
#define ML_ENGINECOM_VARNAME_DRIVEWHEEL2 60
#define ML_ENGINECOM_VARNAME_CLUTCHFORCE 61
#define ML_ENGINECOM_VARNAME_CLUTCHSTART 62
#define ML_ENGINECOM_VARNAME_CLUTCHEND 63
#define ML_ENGINECOM_VARNAME_CLUTCHDISENGAGED 64
#define ML_ENGINECOM_VARNAME_PRESELECT_GEARCHANGE 65
#define ML_ENGINECOM_VARNAME_EPICYCLICGEARRATIO 66
#define ML_ENGINECOM_VARNAME_PROPHASGOVERNOR 67
#define ML_ENGINECOM_VARNAME_PROPCLDRAGMULT 68
#define ML_ENGINECOM_VARNAME_ENGINESOUND 69
#define ML_ENGINECOM_VARNAME_IDLESOUND 70
#define ML_ENGINECOM_VARNAME_STARTERSOUND 71
#define ML_ENGINECOM_VARNAME_PROPBLASTMULT 72
#define ML_ENGINECOM_VARNAME_FUELTANKTYPE 73
#define ML_ENGINECOM_VARNAME_FUELTANKFIREDAMAGECOMPONENT1 74
#define ML_ENGINECOM_VARNAME_FUELTANKFIREDAMAGECOMPONENT2 75
#define ML_ENGINECOM_VARNAME_FUELTANKFIREDAMAGECOMPONENT3 76
#define ML_ENGINECOM_VARNAME_FUELTANKFIREDAMAGECOMPONENT4 77
#define ML_ENGINECOM_VARNAME_FUELTANKFIREDAMAGECOMPONENT5 78
#define ML_ENGINECOM_VARNAME_FUELTANKFIREDAMAGECOMPONENT6 79
#define ML_ENGINECOM_VARNAME_FUELTANKFIREDAMAGECOMPONENT7 80
#define ML_ENGINECOM_VARNAME_FUELTANKFIREDAMAGECOMPONENT8 81
#define ML_ENGINECOM_VARNAME_PROPAIRFOIL 82
#define ML_ENGINECOM_VARNAME_PROPPOWEREFFICIENCY 83
#define ML_ENGINECOM_VARNAME_PROPINDUCEDDRAGMULT 84

#define ML_ENGINECOM_MAX_VARNAME 85


#define ML_ENGINECOM_TRANSTYPE_LIMITEDSLIP 1
#define ML_ENGINECOM_TRANSTYPE_CLUTCHANDBRAKE 2
#define ML_ENGINECOM_TRANSTYPE_EPICYCLIC 3

#define ML_ENGINECOM_MAX_TRANSTYPE 4

struct mlEngineData
{
	UINT32 Type;
	UCHAR EngineName[32];

	// next 3 vars are used to determine engine "capacity"
	FLOAT64 BaseHorsePower;
	FLOAT64 BaseHorsePowerAlt; // alt hp was attained at
	FLOAT64 BaseHpManifoldPress; // manifold press hp was attained at
	FLOAT64 BaseHpRpm; // rpm hp was attained at

	FLOAT64 FuelPerHpSec; // fuel comsumption rate
	FLOAT64 FuelPerHpSecEcon; // fuel consumption rate in economy mode
	FLOAT64 OilConsumptionPerSec; // oil consumption rate
	FLOAT64 Neg_G_Fuel_Cutout; // does engine have neg-g fuel probs?
	FLOAT64 Neg_G_Time_Limit; // how long can you be inverted safely in seconds?

	// for each of the 4 engine states, max pressures
	// and rpm settings
	// minimum pressure is whatever it takes to maintain rpm
	//
	// note that WEP is max power period, including addetives
	// MIL power is maximum "normal" power, usually time limited
	// Climb is maximum continuous power, no time limit
	// Econ is economy power, used for range and cruise
	FLOAT64 WepMaxPressure;
	FLOAT64 MilMaxPressure;
	FLOAT64 ClimbMaxPressure;
	FLOAT64 EconMaxPressure;
	UINT32 ManifoldPressureUnits;
	FLOAT64 WepRpm;
	FLOAT64 MilRpm;
	FLOAT64 ClimbRpm;
	FLOAT64 EconRpm;
	FLOAT64 IdleRpm;
	FLOAT64 MaxWepTime;
	FLOAT64 MaxMilTime;

	UINT32 SuperChargerType; // the class of supercharger
	FLOAT64 SuperChargingAltFloor; // min alt for supercharger effects
	FLOAT64 Speed1CriticalAlt; // max alt for max pressure, speed 1
	FLOAT64 Speed2CriticalAlt; // max alt for max pressure, speed 2
	FLOAT64 ShiftAlt; // altitude when the blower shifts
	FLOAT64 NeutralShiftAlt; // altitude where the blower shifts to neutral, if applicable
	FLOAT64 Speed1MaxMilBoost; // max regulated boost at military power
	FLOAT64 Speed1MaxWepBoost; // max regulated boost at wep power
	FLOAT64 Speed2MaxMilBoost;
	FLOAT64 Speed2MaxWepBoost;
	FLOAT64 NeutralMaxMilBoost;
	FLOAT64 NeutralMaxWepBoost;
	FLOAT64 Speed1MaxClimbBoost;
	FLOAT64 MaxInjWepBoost;

	// note that at whatever altitude hp of engine is given at
	// in the above params the efficiency must be 100% below
	// otherwise you're double-penalizing the engine!
	FLOAT64 Speed1EfficiencyTop; // % of horsepower left after supercharging low blower
	FLOAT64 Speed1EfficiencyBottom;
	FLOAT64 Speed2EfficiencyTop; // % of horsepower left after supercharging high blower
	FLOAT64 Speed2EfficiencyBottom;

	SINT32 PropSpinDirection; // positive for clockwise relative to pilot if pilot behind prop

	FLOAT64 ExhaustThrustAngle; // used to determine exaust thrust
	FLOAT64 ExhaustThrust; // force of gases coming out of exhausts

	UINT32 WepType; // used to determine if addetives consumed
	FLOAT64 WepConsumptionRate; // if wep consumes something how much per sec?

	FLOAT64 PropellerGearRatio;
	FLOAT64 IdlePressureAtIdle;
	FLOAT64 IdlePressureAtMaxRpm;

	FLOAT64 IdleThrottlePos; // throttle position at idle
	FLOAT64 BaseHpExhaustTemp;

	FLOAT64 CriticalRpm; // rpm at which below this the engine can cut out
	FLOAT64 StarterRpm; // the rpm at which the starter will spin the engine at
	FLOAT64 StarterHp; // the power of the starter motor
	FLOAT64 MaxPowerCoolSpeed; // at maximum power, how fast must vehicle go to sufficiently cool
	FLOAT64 EngineTempScaleFactor; // scaling factor for temp gauge

	FLOAT64 MaxSafeRPM; // rpm at which peak torque occurs

};

struct mlEngineList
{
	UINT32 NumEngines;
	mlEngineData * EngineData;
};

struct mlEngineComponentData
{
	UINT32 SubType;

	UCHAR EngineName[32];
	mlEngineData * EngineData; // used to quickreference engine data

	mlComponent* PropellorComponent;
	mlComponent* OilTankComponent;
	mlComponent* RadiatorComponent[ML_MAX_RADIATORS_PER_ENGINE];
	mlComponent* FuelTankComponent[ML_MAX_FUEL_TANKS_PER_ENGINE];
	mlComponent* WepTankComponent;

	// next var is for fuel tanks, oil tanks, WEP tanks,
	// radiators, etc.
	//
	// weight is calculated as the component weight
	// times the ratio of "fullness" of the tank
	FLOAT64 CurrentTankContents; // current contents in kg (zero = empty)
	FLOAT64 TankOriginalMass; // original mass of tank, or original contents
	FLOAT64ATT TankOriginalRotInertia; // original rotational inertia
	UINT32 TankIsSetUp;
	UINT32 TankNum;

	FLOAT64 PropellorDiameter;
	FLOAT64 PropellorEfficiency;
	FLOAT64 PropellorArea;
	FLOAT64 PropellorMdiv; // used for supersonic prop drag
	FLOAT64 PropellorMdiv10deg;
	FLOAT64 PropellorMaxCl; // used for limiting prop's bite
	FLOAT64 PropellorMinCl;
	FLOAT64 PropellorResponse; // how many deg/sec the prop can adjust
	FLOAT64 PropellorMaxAngle; // absolute limit on prop angle
	FLOAT64 PropellorMinAngle;
	FLOAT64 PropellorCd; // parasitic drag
	FLOAT64 PropClDragMult; // scaling factor for CD based on Cl
	FLOAT64 PropellorPowerEfficiency;

	FLOAT64 PropBlastMult; // multiplier of prop blast

	UINT32 PropHasGovener; // does the propeller have an RPM govener?
	SINT32 LastPropPitchAdjustor;

	UINT32 LastWepToggle;
	UINT32 WepStage;

	UINT32 SuperchargerSpeed; // which speed setting is the supercharger in

	FLOAT64 CurrentRpm; // engine's current rpm
	FLOAT64 LastRpm;
	FLOAT64 CurrentAngle;
	FLOAT64 CurrentManifoldPress;
	FLOAT64 LastManifoldPress;
	FLOAT64 CurrentPower;
	FLOAT64 LastPower;
	FLOAT64 RpmSettingPower;
	FLOAT64 RpmSetting;
	FLOAT64 CurrentPropAngle;
	FLOAT64 LastPropAngle;
	FLOAT64 PropCounterTorque;
	FLOAT64 LastTorqueDelta;
	FLOAT64 LastWindAoA;
	UINT32 EngineMode; // current engine mode
	UINT32 EngineNumber; // engine number, used for gauges

	FLOAT64 VortexBackWashVelocity;
	FLOAT64 VortexRotationSpeed;

	// ground model stuff
	UINT32 TransmissionType;
	FLOAT64 FlyWheelRotInertia;
	SINT32 NumForwardGears;
	SINT32 NumReverseGears;
	UINT32 InReverse;
	FLOAT64 ForwardGearRatio[16];
	FLOAT64 ReverseGearRatio[8];
	FLOAT64 EpicyclicGearRatio;
	SINT32 SelectedGear;
	SINT32 UseGear;
	SINT32 CurrentGear;
	FLOAT64 ShiftTime;
	SINT32 PreselectorGearShift;
	SINT32 CanShift;
	SINT32 LastCanShift;
	FLOAT32 CurrentGearX,CurrentGearY;
	FLOAT32 StartGearX,StartGearY;
	FLOAT32 CurrentLinearGearPos;
	FLOAT32 StartLinearGearPos;
	SINT32 InNeutral; // gear box is in neutral gear
	FLOAT64 ClutchPercentEngaged;
	FLOAT64 ClutchForce;
	FLOAT64 ClutchStart; // clutch percentage where it starts to catch
	FLOAT64 ClutchEnd; // clutch percentage where it's fully engaged
	FLOAT64 ClutchDisengaged; // clutch percentage where shifting is allowedS
	FLOAT64 EpicyclicNeutralEngagedPercent;
	SINT32 AutoShift;

	FLOAT64 EngineTemp; // 1.0 = running temp, 0.0 = ambient temp

// engine
	UINT32 EngineSoundLoaded;
	SINT32 EngineSoundId;
	UINT32 IdleSoundLoaded;
	SINT32 IdleSoundId;
	UINT32 StarterSoundLoaded;
	SINT32 StarterSoundId;
	UINT32 DamageLevelSoundLoaded[4];
	SINT32 DamageLevelSoundId[4];

	UCHAR EngineSound[64]; // engine/prop sound filename
	UCHAR IdleSound[64]; // engine/prop idle sound
	UCHAR StartupSound[64]; // engine/prop startup sound

	mlComponent* DriveWheel1;
	mlComponent* DriveWheel2;

	UINT32 EngineCaught; // has the engine caught?
	UINT32 EngineLastCaught;
	UINT32 EngineCaughtOnce;
	UINT32 EngineStarved; // is the engine starved of fuel for some reason?
	UINT32 LastStarterState; // the state of the starter motor last
	FLOAT64 LastIdleThrottlePos;

	UINT32 FuelTankType; // fuel tank type
	UCHAR FuelBurnDamageComponent[8][64];

	UINT32 FuelTankLastHitTime;
	SINT32 FuelTankSealedLevel;
	UINT32 TankIsLeaking;
	UINT32 TankOnFire;

	FLOAT32 LastPropAoA;
	FLOAT32 PropPowerEfficiency; // what percentage of engine power is applied
	FLOAT32 PropInducedDragMult; // induced drag multiplication factor

	UINT32 DamagedEngineRotFlag;
	FLOAT32 LastBurnDamageCounter;

	FLOAT64 PropSeesVelocity; // velocity the prop sees

	struct mlAirfoilData * PropAirfoil;

	FLOAT64 OilInEngine; // amount of oil currently in engine (%)

};

extern UINT32 mlengineFreeEngineData(mlEngineList * EngineList);

extern UINT32 mlengineReadEngineFile(mlEngineList ** EngineList,
									   UCHAR * EngineListFilename,
									   UCHAR * DirName);

extern UINT32 mlengineSetComponentSubtype(void*  Component,
										   UCHAR * SubType,
										   mlEngineList * EngineList);

extern UINT32 mlengineAddSpecialVariable(void*  SpecialData,
										 char * VarName,
										 char * VarContents,
										 mlEngineList * EngineList,
										 mlComponentList * ComponentList,
										 mlComponent * Component,
										struct mlAirfoilList * AirfoilList,
										void*  VehiclePtr);


UINT32 mlUpdateEngineComponent(FLOAT64VECTOR * Force,
							   FLOAT64ATT * Torque,
							   mlComponent * Component,
							   FLOAT64VECTOR * ComponentVel,
							   FLOAT64 Alt,
							   FLOAT64 DeltaTime,
							   FLOAT64ATT * AttVel,
							   FLOAT64 ThrottlePos,
							   FLOAT64 ClutchPos,
							   SINT32 GearChange,
							   UINT32 ReverseToggle,
							   FLOAT64 LeftTrackClutch,
							   FLOAT64 RightTrackClutch,
							   UINT32 FailSafeMode,
							   SINT32 PropPitchAdjust,
							   UINT32 ToggleWepMode,
							   mlObjectData * VehicleObjectData,
							   mlComponentList * ComponentList,
							   void*  VehicleData,
							   FLOAT64 LastVehicleUpdateTime,
							   FLOAT32 VehicleG);

UINT32 mlUpdatePropellor(FLOAT64VECTOR * ReturnForce,
								FLOAT64ATT * ReturnTorque,
								mlEngineComponentData * PropellerData,
								mlComponent * Component,
								FLOAT64 Alt,
								FLOAT64VECTOR * ComponentVel,
								FLOAT64 DeltaTime,
								FLOAT64ATT * AttVel,
								SINT32 PropPitchAdjustor,
								SINT32 FailSafeMode,
								mlObjectData * VehicleObjectData);

UINT32 mlUpdateEngine(FLOAT64VECTOR * Force,
							 FLOAT64ATT * Torque,
							 mlEngineComponentData * EngineComponentData,
							 mlComponent * Component,
							 FLOAT64 Alt,
							 FLOAT64VECTOR * ComponentVel,
							 FLOAT64 DeltaTime,
							 FLOAT64 ThrottlePos,
							 UINT32 FailSafeMode,
							 SINT32 GearChange,
							 UINT32 ToggleWepMode,
							 mlObjectData * VehicleObjectData,
							 void*  VehicleData,
							 FLOAT32 VehicleG);

UINT32 mlengineGetComponents(mlVehicleData* VehicleData,
							mlComponentVector& list,
							SINT32 type,
							SINT32 subType);

