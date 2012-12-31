// Model Engine module
#include "syscfg.h"
#include "systypes.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mlEngine.h"
#include "mlutility.h"
#include "mlAtmo.h"
#include "mltypes.h"
#include "mlgauges.h"
#include "mathvect.h"
#include "mathnum.h"
#include "mlfoils.h"
#include "rnmain.h"
#include "fxMgr.h"
#include "vehmain.h"
#include "vehanim.h"
#include "weapons.h"
#include "mlgear.h"
#include "mlobject.h"
#include "mathUnitConversion.h"
#include "mem.h"
#include "fio.h"
#include "loclMain.h"
#include "sndsSoundControl.h"
#include "persmain.h"
#include "vehfx.h"

#if USE_OSG
#include "fxSound.h"
#else
#include "fxSnd.h"
#endif

#if USE_OSG
#undef PI
#define PI osg::PI
#endif

extern "C" LOGICAL IsFirstTic;

#define _CL_TO_DEG_RATIO 10.0 // # of degrees per whole cl number
#define OIL_DENSITY 920.0f // kg / m^2

static UINT32 _UpdateTank(mlEngineComponentData * TankData,
						  mlComponent * Component,
						  mlComponentList * ComponentList,
						  FLOAT64 DeltaTime,
						  vehVehicle * VehicleData);


static UINT32 _UpdateGearbox(mlEngineComponentData * EngineComponentData,
							 mlComponent * Component,
							 SINT32 GearChange,
							 SINT32 ReverseToggle,
							 UINT32 FailSafeMode,
							 FLOAT64 LastVehicleUpdateTime);

static UINT32 _UpdateClutch(mlEngineComponentData * EngineComponentData,
							mlComponent * Component,
							FLOAT64 ClutchPos,
							FLOAT64 DeltaTime,
							UINT32 FailSafeMode);

static UINT32 _UpdateTorqueGenerator(mlEngineComponentData * EngineComponentData,
									 mlComponent * Component,
									 FLOAT64 DeltaTime,
									 FLOAT64 ThrottlePos,
									 UINT32 FailSafeMode);


static UINT32 _UpdateTransmission(mlEngineComponentData * TransData,
								mlComponent * Component,
								FLOAT64 DeltaTime,
								FLOAT64 LeftTrackClutch,
								FLOAT64 RightTrackClutch);

static UINT32 _HandleFuelTankDamage(mlEngineComponentData * TankData,
									  mlComponent * Component,
									  mlComponentList * ComponentList,
									  FLOAT64 DeltaTime,
									  vehVehicle * VehicleData);
static UINT32 _HandleGenericTankDamage(mlEngineComponentData * TankData,
									  mlComponent * Component,
									  FLOAT64 DeltaTime,
									  UCHAR *StateName,
									  vehVehicle *VehicleData);

static UINT32 _GetManifoldInputPressure(FLOAT64 * ManifoldInDensity,
										FLOAT64 Alt,
										mlEngineComponentData * EngineComponentData,
										mlEngineData * EngineData,
										UINT32 FailSafeMode,
										SINT32 GearChange);

static void _EngineSoundStopped(void *data);
static void _IdleSoundStopped(void *data);
static void _StarterSoundStopped(void *data);

static FLOAT64 _FuelRateModifier(vehVehicle*);

static void _HandleEngineSounds(mlComponent*, mlEngineComponentData*, vehVehicle*, mlObjectData*, FLOAT32 VehicleG, UINT32 StarterOn);

UCHAR _EngineTypeVarNames[ML_ENGINE_MAX_VARNAME][32] = 
{
	"none",
	"type",
	"basehorsepower",
	"basehpalt",
	"basehppress",
	"basehprpm",
	"fuelperhphour",
	"oilconsumptionperhour",
	"neg_g_fuel_cutout",
	"neg_g_time_limit",
	"wepmaxpress",
	"milmaxpress",
	"climbmaxpress",
	"econmaxpress",
	"maxweptime",
	"maxmiltime",
	"weprpm",
	"milrpm",
	"climbrpm",
	"econrpm",
	"superchargetype",
	"speed1critalt",
	"speed2critalt",
	"shiftalt",
	"not_used",
	"not_used",
	"exhaustthrustangle",
	"weptype",
	"wepconsumptionratepersec",
	"propgearratio",
	"idlepressureatidle",
	"fuelperhphourecon",
	"superchargingaltfloor",
	"speed1efficiencytop",
	"speed2efficiencytop",
	"propspindirection",
	"exhaustthrustforce",
	"idlerpm",
	"speed1efficiencybottom",
	"speed2efficiencybottom",
	"manifoldunits",
	"idlepressureatmaxrpm",
	"idlethrottlepos",
	"basehpexhausttemp",
	"criticalrpm",
	"starterrpm",
	"starterhp",
	"speed1maxmilboost",
	"speed1maxwepboost",
	"speed2maxmilboost",
	"speed2maxwepboost",
	"neutralshiftalt",
	"neutralmaxmilboost",
	"neutralmaxwepboost",
	"enginemaxcoolspeed",
	"enginetempscalefactor",
	"maxsaferpm",
	"speed1maxclimbboost",
	"maxinjwepboost"
};

UCHAR _ManifoldPressureUnits[ML_MAX_ENGINE_MANIFOLDTYPES][32] = 
{
	"undefined",
	"cmhgboost",
	"inhg",
	"lbsboost",
	"atm",
};

UCHAR _WepTypeNames[ML_ENGINE_MAX_WEPTYPE][32] = 
{
	"none",
	"normal",
	"methanolwater",
	"nitrousoxide",
	"water",
};

UCHAR _SuperChargeTypeNames[ML_ENGINE_MAX_SUPERCHARGETYPES][32] = 
{
	"none",
	"singlespeed",
	"twospeed",
	"turbo",
	"singlespeedwithneutral",
	"twospeedwithneutral",
};

UCHAR _TransmissionTypeNames[ML_ENGINECOM_MAX_TRANSTYPE][32] = 
{
	"none",
	"limitedslip",
	"clutchandbrake",
	"epicyclic",
};

UCHAR _EngineTypeNames[ML_ENGINE_MAX_SUBTYPES][32] = 
{
	"none",
	"propeller",
	"engine",
	"fueltank",
	"oiltank",
	"radiator",
	"weptank",
	"transmission",
	"torquegenerator",
	"clutch",
	"gearbox",
};

UCHAR _FuelTankType[ML_ENGINECOM_MAX_FUELTANKTYPE][32] = 
{
	"none",
	"integral",
	"integral_with_lightselfsealing",
	"bag",
	"bag_with_lightselfsealing",
	"bag_with_goodselfsealing",
	"integral_with_extinguisher",


};

UCHAR _EngineComponentVarNames[ML_ENGINECOM_MAX_VARNAME][32] = 
{
	"none",
	"enginetype",
	"propellercomponent",
	"oiltankcomponent",
	"radiator1component",
	"radiator2component",
	"radiator3component",
	"radiator4component",
	"fueltank1component",
	"fueltank2component",
	"fueltank3component",
	"fueltank4component",
	"fueltank5component",
	"fueltank6component",
	"fueltank7component",
	"fueltank8component",
	"weptankcomponent",
	"tanknum",
	"propellerdiameter",
	"propellerefficiency",
	"propellerarea",
	"enginenumber",
	"propellermdiv",
	"propellermaxcl",
	"propellermincl",
	"propellerresponse",
	"propellermaxangle",
	"propellerminangle",
	"propellercd",
	"propellermdiv10deg",
	"propellerpowerefficiency",
	"transtype",
	"flywheelrotinertia",
	"numforwardgears",
	"numreversegears",
	"gear1ratio",
	"gear2ratio",
	"gear3ratio",
	"gear4ratio",
	"gear5ratio",
	"gear6ratio",
	"gear7ratio",
	"gear8ratio",
	"gear9ratio",
	"gear10ratio",
	"gear11ratio",
	"gear12ratio",
	"gear13ratio",
	"gear14ratio",
	"gear15ratio",
	"gear16ratio",
	"reverse1ratio",
	"reverse2ratio",
	"reverse3ratio",
	"reverse4ratio",
	"reverse5ratio",
	"reverse6ratio",
	"reverse7ratio",
	"reverse8ratio",
	"drivewheel1",
	"drivewheel2",
	"clutchforce",
	"clutchstart",
	"clutchend",
	"clutchdisengaged",
	"preselector",
	"epicyclicgearratio",
	"governedprop",
	"propcldragmult",
	"enginesound",
	"idlesound",
	"startersound",
	"propblastmult",
	"fueltanktype",
	"fuelburncomponent1",
	"fuelburncomponent2",
	"fuelburncomponent3",
	"fuelburncomponent4",
	"fuelburncomponent5",
	"fuelburncomponent6",
	"fuelburncomponent7",
	"fuelburncomponent8",
	"propairfoil",
	"proppowerefficiency",
	"propinduceddragmult",
};


UINT32 _DisableEngineThrust;

static char* ENGINE_DAMAGE_SOUND[4] = {"sounds\\enginedamage1.wav", "sounds\\enginedamage2.wav", "sounds\\enginedamage3.wav", "sounds\\enginedamage4.wav"};

UINT32 mlengineFreeEngineData(mlEngineList * EngineList)
{
	if (EngineList->EngineData != NULL)
	{
		memFree(EngineList->EngineData);
		EngineList->EngineData = NULL;
	}
	EngineList->NumEngines = 0;

	return 0;
}

UINT32 mlengineReadEngineFile(mlEngineList ** EngineList,
							   UCHAR * EngineListFilename,
							   UCHAR * DirName)
{
SINT32 EngineListFile;
UCHAR Buffer[256];
UCHAR * BufferPos;
UCHAR UseFileName[128];
SINT32 NumEngineTypes;
UINT32 VarNum;
UCHAR * VarName;
UCHAR * VarContents;
SINT32 EngineListPos;
mlEngineData * EngineData;
UINT32 Size;


	if(EngineListFilename == NULL || EngineListFilename[0] == '\0') return 1;

	strcpy((char *)UseFileName,(char *)DirName);
	strcat((char *)UseFileName,"\\");
	strcat((char *)UseFileName,(char *)EngineListFilename);

	EngineListFile = fioOpen((char *)UseFileName);

	if (EngineListFile == -1)
	{
		*EngineList = NULL;

		return 1;
	}

	NumEngineTypes = 0;
	while (fioGetString(EngineListFile,(char *)Buffer,255) != 0)
	{
		if (Buffer[0] == '[')
		{
			NumEngineTypes++;
		}
	}

	fioClose(EngineListFile);

	EngineListFile = fioOpen((char *)UseFileName);

	if (EngineListFile == -1)
	{
		*EngineList = NULL;

		return 1;
	}

	*EngineList = (mlEngineList*)memAlloc(sizeof(mlEngineList));
	memset(*EngineList,0,sizeof(mlEngineList));
	Size = NumEngineTypes * sizeof(mlEngineData);
	(*EngineList)->EngineData = (mlEngineData*)memAlloc(Size);
	memset((*EngineList)->EngineData,0,NumEngineTypes * sizeof(mlEngineData));
	(*EngineList)->NumEngines = NumEngineTypes;

	EngineListPos = -1;

	while (fioGetString(EngineListFile,(char *)Buffer,255) != 0)
	{
		if (Buffer[0] == '#')
		{
			continue;
		}

		if (Buffer[0] == '[')
		{
			// new engine
			// first strip off the "[" and "]" characters
			BufferPos = Buffer + 1;
            // Swapped the char * type cast around to the right of =
            // Caveman
			BufferPos = (UCHAR *)strtok((char *)BufferPos,"]");
			EngineListPos++;
			if (EngineListPos == NumEngineTypes)
			{
				break;
			}

			strcpy((char *)(*EngineList)->EngineData[EngineListPos].EngineName,(char *)BufferPos);		

			continue;
		}
		else
		{
			if (EngineListPos < 0)
			{
				continue;
			}

			mlUtilityGetVarnameAndContents(&VarName,&VarContents,Buffer);

			if (VarName == NULL || VarContents == NULL)
			{
				continue;
			}

			mlUtilityFindVarname(&VarNum,
								VarName,
								(UCHAR *)&_EngineTypeVarNames[0][0],
								32,
								ML_ENGINE_MAX_VARNAME);

			if (VarNum == 0)
			{
				Write_Debug("Unidentified variable %s in file %s\n",VarName,EngineListFilename);
				continue;
			}

			EngineData = &(*EngineList)->EngineData[EngineListPos];
			switch (VarNum)
			{
				case ML_ENGINE_VARNAME_TYPE:
						mlUtilityFindVarname(&EngineData->Type,
											VarContents,
											(UCHAR *)&_EngineTypeNames[0][0],
											32,
											ML_ENGINE_MAX_ENGINETYPES);

						if ((*EngineList)->EngineData[EngineListPos].Type == 0)
						{
							Write_Debug("Engine type %s not recognized in file %s\n",VarContents,UseFileName);
						}
					break;
				case ML_ENGINE_VARNAME_BASEHORSEPOWER:
						EngineData->BaseHorsePower = atof((char *)VarContents);
					break;
				case ML_ENGINE_VARNAME_BASEHPALT:
						EngineData->BaseHorsePowerAlt = atof((char *)VarContents);
					break;
				case ML_ENGINE_VARNAME_BASEHPPRESS:
						EngineData->BaseHpManifoldPress = atof((char *)VarContents);
					break;
				case ML_ENGINE_VARNAME_BASEHPRPM:
						EngineData->BaseHpRpm = atof((char *)VarContents);
						EngineData->MaxSafeRPM = EngineData->BaseHpRpm;
					break;
				case ML_ENGINE_VARNAME_FUELPERHPSEC:
						EngineData->FuelPerHpSec = atof((char *)VarContents) / 3600.0;
					break;
				case ML_ENGINE_VARNAME_FUELPERHPSECECON:
						EngineData->FuelPerHpSecEcon = atof((char *)VarContents) / 3600.0;
					break;
				case ML_ENGINE_VARNAME_OILCONSUMPTIONPERSEC:
						EngineData->OilConsumptionPerSec = atof((char *)VarContents) / 3600.0;
					break;
				case ML_ENGINE_VARNAME_NEG_G_FUEL_CUTOUT:
						EngineData->Neg_G_Fuel_Cutout = atoi((char *)VarContents);
					break;
				case ML_ENGINE_VARNAME_NEG_G_TIME_LIMIT:
						EngineData->Neg_G_Time_Limit = atof((char *)VarContents);
					break;
				case ML_ENGINE_VARNAME_WEPMAXPRESS:
						EngineData->WepMaxPressure = atof((char *)VarContents);
					break;
				case ML_ENGINE_VARNAME_MILMAXPRESS:
						EngineData->MilMaxPressure = atof((char *)VarContents);
					break;
				case ML_ENGINE_VARNAME_CLIMBMAXPRESS:
						EngineData->ClimbMaxPressure = atof((char *)VarContents);
					break;
				case ML_ENGINE_VARNAME_ECONMAXPRESS:
						EngineData->EconMaxPressure = atof((char *)VarContents);
					break;
				case ML_ENGINE_VARNAME_MAXWEPTIME:
						EngineData->MaxWepTime = atof((char *)VarContents);
					break;
				case ML_ENGINE_VARNAME_MAXMILTIME:
						EngineData->MaxMilTime = atof((char *)VarContents);
					break;
				case ML_ENGINE_VARNAME_WEP_RPM:
						EngineData->WepRpm = atof((char *)VarContents);
						EngineData->MaxSafeRPM = EngineData->WepRpm;
					break;
				case ML_ENGINE_VARNAME_MIL_RPM:
						EngineData->MilRpm = atof((char *)VarContents);
					break;
				case ML_ENGINE_VARNAME_CLIMB_RPM:
						EngineData->ClimbRpm = atof((char *)VarContents);
					break;
				case ML_ENGINE_VARNAME_ECON_RPM:
						EngineData->EconRpm = atof((char *)VarContents);
					break;
				case ML_ENGINE_VARNAME_SUPERCHARGETYPE:
						mlUtilityFindVarname(&EngineData->SuperChargerType,
											VarContents,
											(UCHAR *)&_SuperChargeTypeNames[0][0],
											32,
											ML_ENGINE_MAX_SUPERCHARGETYPES);
					break;
				case ML_ENGINE_VARNAME_SUPERCHARGING_ALTFLOOR:
						EngineData->SuperChargingAltFloor = atof((char *)VarContents);
					break;
				case ML_ENGINE_VARNAME_SPEED1CRITALT:
						EngineData->Speed1CriticalAlt = atof((char *)VarContents);
					break;
				case ML_ENGINE_VARNAME_SPEED2CRITALT:
						EngineData->Speed2CriticalAlt = atof((char *)VarContents);
					break;
				case ML_ENGINE_VARNAME_SHIFTALT:
						EngineData->ShiftAlt = atof((char *)VarContents);
					break;
				case ML_ENGINE_VARNAME_NEUTRALSHIFTALT:
						EngineData->NeutralShiftAlt = atof((char *)VarContents);
					break;
				case ML_ENGINE_VARNAME_SPEED1MAXMILBOOST:
						EngineData->Speed1MaxMilBoost = atof((char *)VarContents);
					break;
				case ML_ENGINE_VARNAME_SPEED1MAXWEPBOOST:
						EngineData->Speed1MaxWepBoost = atof((char *)VarContents);
					break;
				case ML_ENGINE_VARNAME_SPEED2MAXMILBOOST:
						EngineData->Speed2MaxMilBoost = atof((char *)VarContents);
					break;
				case ML_ENGINE_VARNAME_SPEED2MAXWEPBOOST:
						EngineData->Speed2MaxWepBoost = atof((char *)VarContents);
					break;
				case ML_ENGINE_VARNAME_NEUTRALMAXMILBOOST:
						EngineData->NeutralMaxMilBoost = atof((char *)VarContents);
					break;
				case ML_ENGINE_VARNAME_NEUTRALMAXWEPBOOST:
						EngineData->NeutralMaxWepBoost = atof((char *)VarContents);
					break;

				case ML_ENGINE_VARNAME_SPEED1HPEFFICIENCYTOP:
						EngineData->Speed1EfficiencyTop = atof((char *)VarContents);
					break;
				case ML_ENGINE_VARNAME_SPEED2HPEFFICIENCYTOP:
						EngineData->Speed2EfficiencyTop = atof((char *)VarContents);
					break;
				case ML_ENGINE_VARNAME_PROPSPINDIRECTION:
						EngineData->PropSpinDirection = atoi((char *)VarContents);
					break;
				case ML_ENGINE_VARNAME_EXHAUSTTHRUSTANGLE:
						EngineData->ExhaustThrustAngle = mlDEG_TO_RAD(atof((char *)VarContents));
					break;
				case ML_ENGINE_VARNAME_EXHAUSTTHRUST:
						EngineData->ExhaustThrust = atof((char *)VarContents);
					break;
				case ML_ENGINE_VARNAME_WEPTYPE:
						mlUtilityFindVarname(&EngineData->WepType,
											VarContents,
											(UCHAR *)&_WepTypeNames[0][0],
											32,
											ML_ENGINE_MAX_WEPTYPE);
					break;
				case ML_ENGINE_VARNAME_WEPCONSUMPTIONRATE:
						EngineData->WepConsumptionRate = atof((char *)VarContents);
					break;
				case ML_ENGINE_VARNAME_PROPGEARRATIO:
						EngineData->PropellerGearRatio = atof((char *)VarContents);
					break;
				case ML_ENGINE_VARNAME_IDLEPRESSURE:
						EngineData->IdlePressureAtIdle = atof((char *)VarContents);
					break;
				case ML_ENGINE_VARNAME_IDLEPRESSUREATMAXRPM:
						EngineData->IdlePressureAtMaxRpm = atof((char *)VarContents);
					break;
				case ML_ENGINE_VARNAME_IDLERPM:
						EngineData->IdleRpm = atof((char *)VarContents);
					break;
				case ML_ENGINE_VARNAME_SPEED1HPEFFICIENCYBOTTOM:
						EngineData->Speed1EfficiencyBottom = atof((char *)VarContents);
					break;
				case ML_ENGINE_VARNAME_SPEED2HPEFFICIENCYBOTTOM:
						EngineData->Speed2EfficiencyBottom = atof((char *)VarContents);
					break;
				case ML_ENGINE_VARNAME_MANIFOLDUNITS:
						mlUtilityFindVarname(&EngineData->ManifoldPressureUnits,
											 VarContents,
											 (UCHAR *)&_ManifoldPressureUnits[0][0],
											 32,
											 ML_MAX_ENGINE_MANIFOLDTYPES);
						break;
				case ML_ENGINE_VARNAME_IDLETHROTTLEPOS:
						EngineData->IdleThrottlePos = atof((char *)VarContents);
						break;

				case ML_ENGINE_VARNAME_BASEHPEXHAUSTTEMP:
						EngineData->BaseHpExhaustTemp = atof((char *)VarContents);
						break;

				case ML_ENGINE_VARNAME_CRITICALRPM:
						EngineData->CriticalRpm = atof((char *)VarContents);
						break;
				case ML_ENGINE_VARNAME_STARTERRPM:
						EngineData->StarterRpm = atof((char *)VarContents);
						break;
				case ML_ENGINE_VARNAME_STARTERHP:
						EngineData->StarterHp = atof((char *)VarContents);
						break;
				case ML_ENGINE_VARNAME_MAXPOWERCOOLSPEED:
						EngineData->MaxPowerCoolSpeed = atof((char *)VarContents);
						break;
				case ML_ENGINE_VARNAME_ENGINETEMPSCALEFACTOR:
						EngineData->EngineTempScaleFactor = atof((char *)VarContents);
						break;
				case ML_ENGINE_VARNAME_MAXSAFERPM:
						EngineData->MaxSafeRPM = atof((char *)VarContents);
						break;
				case ML_ENGINE_VARNAME_SPEED1MAXCLIMBBOOST:
						EngineData->Speed1MaxClimbBoost = atof((char *)VarContents);
				break;
				case ML_ENGINE_VARNAME_MAXINJWEPBOOST:
						EngineData->MaxInjWepBoost = atof((char *)VarContents);
				break;
				default:
						Write_Debug("Unidentified variable %s in file %s\n",VarContents,EngineListFilename);
						break;
			}
		}
	}
	
	if(EngineData->MaxWepTime > 0 && EngineData->WepConsumptionRate == 0)
		EngineData->WepConsumptionRate = 1/EngineData->MaxWepTime;
	
	fioClose (EngineListFile);
	
	return 0;
}

UINT32 mlengineSetComponentSubtype(void * SpecialData,
								   UCHAR * SubType,
								   mlEngineList * EngineList)
{
mlEngineComponentData * EngineComponentData;

	if (SpecialData == NULL || SubType == NULL || EngineList == NULL)
	{
		return 1;
	}
	
	EngineComponentData = (mlEngineComponentData *)(SpecialData);

	mlUtilityFindVarname(&EngineComponentData->SubType,
						SubType,
						(UCHAR *)&_EngineTypeNames[0][0],
						32,
						ML_ENGINE_MAX_SUBTYPES);

	return 0;
}

UINT32 mlengineAddSpecialVariable(void * SpecialData, 
								 char * VarName, 
								 char * VarContents, 
								 mlEngineList * EngineList,
								 mlComponentList * ComponentList,
								 mlComponent * Component,
								 mlAirfoilList * AirfoilList,
								 void * VehiclePtr)
{
mlEngineComponentData * ComponentData;
UINT32 VarNum;
UINT32 Found;
UINT32 i;
vehVehicle * Vehicle;

	if (SpecialData == NULL ||
		VarName == NULL ||
		VarContents == NULL ||
		EngineList == NULL)
	{
		return 1;
	}

	Vehicle = (vehVehicle *)VehiclePtr;

	ComponentData = (mlEngineComponentData *)(SpecialData);

	mlUtilityFindVarname(&VarNum,
						(UCHAR *)VarName,
						(UCHAR *)&_EngineComponentVarNames[0][0],
						32,
						ML_ENGINECOM_MAX_VARNAME);


	switch (VarNum)
	{
		case ML_ENGINECOM_VARNAME_ENGINETYPE:
				Found = 0;
				for(i=0;i<EngineList->NumEngines;i++)
				{
					if (strncmp((char *)VarContents,(char *)EngineList->EngineData[i].EngineName,strlen(VarContents)) == 0)
					{
						ComponentData->EngineData = &EngineList->EngineData[i];					
						ComponentData->OilInEngine = 1.0;
						strcpy((char*)ComponentData->EngineName,(char *)VarContents);

						Found = 1;
						break;
					}
				}

				if (Found == 0)
				{
					ComponentData->EngineData = NULL;
					strcpy((char *)ComponentData->EngineName,"");
					Write_Debug("Unable to find engine type %s in component %s\n",VarContents,Component->ComponentName);
				}
			break;
		case ML_ENGINECOM_VARNAME_PROPELLORCOM:
				mlUtilityFindComponentByName(&ComponentData->PropellorComponent,
											ComponentList,
											(UCHAR *)VarContents,
											Component);
			break;
		case ML_ENGINECOM_VARNAME_OILTANKCOM:
				mlUtilityFindComponentByName(&ComponentData->OilTankComponent,
											ComponentList,
											(UCHAR *)VarContents,
											Component);
			break;
		case ML_ENGINECOM_VARNAME_RADIATOR1COM:
				mlUtilityFindComponentByName(&ComponentData->RadiatorComponent[0],
											ComponentList,
											(UCHAR *)VarContents,
											Component);
			break;
		case ML_ENGINECOM_VARNAME_RADIATOR2COM:
				mlUtilityFindComponentByName(&ComponentData->RadiatorComponent[1],
											ComponentList,
											(UCHAR *)VarContents,
											Component);
			break;
		case ML_ENGINECOM_VARNAME_RADIATOR3COM:
				mlUtilityFindComponentByName(&ComponentData->RadiatorComponent[2],
											ComponentList,
											(UCHAR *)VarContents,
											Component);
			break;
		case ML_ENGINECOM_VARNAME_RADIATOR4COM:
				mlUtilityFindComponentByName(&ComponentData->RadiatorComponent[3],
											ComponentList,
											(UCHAR *)VarContents,
											Component);
			break;
		case ML_ENGINECOM_VARNAME_FUELTANK1COM:
				mlUtilityFindComponentByName(&ComponentData->FuelTankComponent[0],
											ComponentList,
											(UCHAR *)VarContents,
											Component);
			break;
		case ML_ENGINECOM_VARNAME_FUELTANK2COM:
				mlUtilityFindComponentByName(&ComponentData->FuelTankComponent[1],
											ComponentList,
											(UCHAR *)VarContents,
											Component);
			break;
		case ML_ENGINECOM_VARNAME_FUELTANK3COM:
				mlUtilityFindComponentByName(&ComponentData->FuelTankComponent[2],
											ComponentList,
											(UCHAR *)VarContents,
											Component);
			break;
		case ML_ENGINECOM_VARNAME_FUELTANK4COM:
				mlUtilityFindComponentByName(&ComponentData->FuelTankComponent[3],
											ComponentList,
											(UCHAR *)VarContents,
											Component);
			break;
		case ML_ENGINECOM_VARNAME_FUELTANK5COM:
				mlUtilityFindComponentByName(&ComponentData->FuelTankComponent[4],
											ComponentList,
											(UCHAR *)VarContents,
											Component);
			break;
		case ML_ENGINECOM_VARNAME_FUELTANK6COM:
				mlUtilityFindComponentByName(&ComponentData->FuelTankComponent[5],
											ComponentList,
											(UCHAR *)VarContents,
											Component);
			break;
		case ML_ENGINECOM_VARNAME_FUELTANK7COM:	
				mlUtilityFindComponentByName(&ComponentData->FuelTankComponent[6],
											ComponentList,
											(UCHAR *)VarContents,
											Component);
			break;
		case ML_ENGINECOM_VARNAME_FUELTANK8COM:
				mlUtilityFindComponentByName(&ComponentData->FuelTankComponent[7],
											ComponentList,
											(UCHAR *)VarContents,
											Component);
			break;
		case ML_ENGINECOM_VARNAME_WEPTANK:
				mlUtilityFindComponentByName(&ComponentData->WepTankComponent,
											ComponentList,
											(UCHAR *)VarContents,
											Component);
			break;

		case ML_ENGINECOM_VARNAME_TANKNUM:
				ComponentData->TankNum = atoi(VarContents);
			break;

		case ML_ENGINECOM_PROPDIAMETER:
				ComponentData->PropellorDiameter = atof(VarContents);
			break;
		
		case ML_ENGINECOM_PROPEFFICIENCY:
				ComponentData->PropellorEfficiency = atof(VarContents);
			break;

		case ML_ENGINECOM_PROPAREA:
				ComponentData->PropellorArea = atof(VarContents);
			break;
		case ML_ENGINECOM_ENGINENUM:
				ComponentData->EngineNumber = atoi(VarContents);
			break;
		case ML_ENGINECOM_VARNAME_PROPELLER_MDIV:
				ComponentData->PropellorMdiv = atof(VarContents);
			break;
		case ML_ENGINECOM_VARNAME_PROPELLER_MAXCL:
				ComponentData->PropellorMaxCl = atof(VarContents);
			break;
		case ML_ENGINECOM_VARNAME_PROPELLER_MINCL:
				ComponentData->PropellorMinCl = atof(VarContents);
			break;
		case ML_ENGINECOM_VARNAME_PROPELLER_RESPONSE:
				ComponentData->PropellorResponse = atof(VarContents);
			break;
		case ML_ENGINECOM_VARNAME_PROPELLER_MAXANGLE:
				ComponentData->PropellorMaxAngle = atof(VarContents);
			break;
		case ML_ENGINECOM_VARNAME_PROPELLER_MINANGLE:
				ComponentData->PropellorMinAngle = atof(VarContents);
			break;
		case ML_ENGINECOM_VARNAME_PROPELLER_CD:
				ComponentData->PropellorCd = atof(VarContents);
			break;
		case ML_ENGINECOM_VARNAME_PROPCLDRAGMULT:
				ComponentData->PropClDragMult = atof(VarContents);
			break;
		case ML_ENGINECOM_VARNAME_PROPELLER_MDIV_10DEG:
				ComponentData->PropellorMdiv10deg = atof(VarContents);
			break;
		case ML_ENGINECOM_VARNAME_PROPELLER_EFFICIENCY:
				ComponentData->PropellorPowerEfficiency = atof(VarContents);
			break;
		case ML_ENGINECOM_VARNAME_TRANSTYPE:
				mlUtilityFindVarname(&ComponentData->TransmissionType,
									(UCHAR *)VarContents,
									(UCHAR *)&_TransmissionTypeNames[0][0],
									32,
									ML_ENGINECOM_MAX_TRANSTYPE);
			break;
		case ML_ENGINECOM_VARNAME_FLYWHEEL_ROTINERTIA:
				ComponentData->FlyWheelRotInertia = atof(VarContents);
			break;
		case ML_ENGINECOM_VARNAME_NUMFORWARDGEARS:
				ComponentData->NumForwardGears = atoi(VarContents);
			break;
		case ML_ENGINECOM_VARNAME_NUMREVERSEGEARS:
				ComponentData->NumReverseGears = atoi(VarContents);
			break;
		case ML_ENGINECOM_VARNAME_PRESELECT_GEARCHANGE:
				ComponentData->PreselectorGearShift = atoi(VarContents);
			break;
		case ML_ENGINECOM_VARNAME_EPICYCLICGEARRATIO:
				ComponentData->EpicyclicGearRatio = atof(VarContents);
			break;
		case ML_ENGINECOM_VARNAME_FORWARDGEAR1RATIO:
				ComponentData->ForwardGearRatio[0] = atof(VarContents);
			break;
		case ML_ENGINECOM_VARNAME_FORWARDGEAR2RATIO:
				ComponentData->ForwardGearRatio[1] = atof(VarContents);
			break;
		case ML_ENGINECOM_VARNAME_FORWARDGEAR3RATIO:
				ComponentData->ForwardGearRatio[2] = atof(VarContents);
			break;
		case ML_ENGINECOM_VARNAME_FORWARDGEAR4RATIO:
				ComponentData->ForwardGearRatio[3] = atof(VarContents);
			break;
		case ML_ENGINECOM_VARNAME_FORWARDGEAR5RATIO:
				ComponentData->ForwardGearRatio[4] = atof(VarContents);
			break;
		case ML_ENGINECOM_VARNAME_FORWARDGEAR6RATIO:
				ComponentData->ForwardGearRatio[5] = atof(VarContents);
			break;
		case ML_ENGINECOM_VARNAME_FORWARDGEAR7RATIO:
				ComponentData->ForwardGearRatio[6] = atof(VarContents);
			break;
		case ML_ENGINECOM_VARNAME_FORWARDGEAR8RATIO:
				ComponentData->ForwardGearRatio[7] = atof(VarContents);
			break;
		case ML_ENGINECOM_VARNAME_FORWARDGEAR9RATIO:
				ComponentData->ForwardGearRatio[8] = atof(VarContents);
			break;
		case ML_ENGINECOM_VARNAME_FORWARDGEAR10RATIO:
				ComponentData->ForwardGearRatio[9] = atof(VarContents);
			break;
		case ML_ENGINECOM_VARNAME_FORWARDGEAR11RATIO:
				ComponentData->ForwardGearRatio[10] = atof(VarContents);
			break;
		case ML_ENGINECOM_VARNAME_FORWARDGEAR12RATIO:
				ComponentData->ForwardGearRatio[11] = atof(VarContents);
			break;
		case ML_ENGINECOM_VARNAME_FORWARDGEAR13RATIO:
				ComponentData->ForwardGearRatio[12] = atof(VarContents);
			break;
		case ML_ENGINECOM_VARNAME_FORWARDGEAR14RATIO:
				ComponentData->ForwardGearRatio[13] = atof(VarContents);
			break;
		case ML_ENGINECOM_VARNAME_FORWARDGEAR15RATIO:
				ComponentData->ForwardGearRatio[14] = atof(VarContents);
			break;
		case ML_ENGINECOM_VARNAME_FORWARDGEAR16RATIO:
				ComponentData->ForwardGearRatio[15] = atof(VarContents);
			break;
		case ML_ENGINECOM_VARNAME_REVERSEGEAR1RATIO:
				ComponentData->ReverseGearRatio[0] = atof(VarContents);
			break;
		case ML_ENGINECOM_VARNAME_REVERSEGEAR2RATIO:
				ComponentData->ReverseGearRatio[1] = atof(VarContents);
			break;
		case ML_ENGINECOM_VARNAME_REVERSEGEAR3RATIO:
				ComponentData->ReverseGearRatio[2] = atof(VarContents);
			break;
		case ML_ENGINECOM_VARNAME_REVERSEGEAR4RATIO:
				ComponentData->ReverseGearRatio[3] = atof(VarContents);
			break;
		case ML_ENGINECOM_VARNAME_REVERSEGEAR5RATIO:
				ComponentData->ReverseGearRatio[4] = atof(VarContents);
			break;
		case ML_ENGINECOM_VARNAME_REVERSEGEAR6RATIO:
				ComponentData->ReverseGearRatio[5] = atof(VarContents);
			break;
		case ML_ENGINECOM_VARNAME_REVERSEGEAR7RATIO:
				ComponentData->ReverseGearRatio[6] = atof(VarContents);
			break;
		case ML_ENGINECOM_VARNAME_REVERSEGEAR8RATIO:
				ComponentData->ReverseGearRatio[7] = atof(VarContents);
			break;
		case ML_ENGINECOM_VARNAME_DRIVEWHEEL1:
				mlUtilityFindComponentByName(&ComponentData->DriveWheel1,
											ComponentList,
											(UCHAR *)VarContents,
											Component);
			break;
		case ML_ENGINECOM_VARNAME_DRIVEWHEEL2:
				mlUtilityFindComponentByName(&ComponentData->DriveWheel2,
											ComponentList,
											(UCHAR *)VarContents,
											Component);
			break;
		case ML_ENGINECOM_VARNAME_CLUTCHFORCE:
				ComponentData->ClutchForce = atof(VarContents);
			break;
		case ML_ENGINECOM_VARNAME_CLUTCHSTART:
				ComponentData->ClutchStart = atof(VarContents);
			break;

		case ML_ENGINECOM_VARNAME_CLUTCHEND:
				ComponentData->ClutchEnd = atof(VarContents);
			break;

		case ML_ENGINECOM_VARNAME_CLUTCHDISENGAGED:
				ComponentData->ClutchDisengaged = atof(VarContents);
			break;

		case ML_ENGINECOM_VARNAME_PROPHASGOVERNOR:
				ComponentData->PropHasGovener = atoi(VarContents);
			break;

		case ML_ENGINECOM_VARNAME_ENGINESOUND:
				strcpy((char *)ComponentData->EngineSound,VarContents);
			break;
		case ML_ENGINECOM_VARNAME_IDLESOUND:
				strcpy((char *)ComponentData->IdleSound,VarContents);
			break;
		case ML_ENGINECOM_VARNAME_STARTERSOUND:
				strcpy((char *)ComponentData->StartupSound,VarContents);
			break;
		case ML_ENGINECOM_VARNAME_PROPBLASTMULT:
				ComponentData->PropBlastMult = atof(VarContents);
			break;
		case ML_ENGINECOM_VARNAME_FUELTANKTYPE:
				mlUtilityFindVarname(&ComponentData->FuelTankType,
									(UCHAR *)VarContents,
									(UCHAR *)&_FuelTankType[0][0],
									32,
									ML_ENGINECOM_MAX_FUELTANKTYPE);
			break;
		case ML_ENGINECOM_VARNAME_FUELTANKFIREDAMAGECOMPONENT1:
		case ML_ENGINECOM_VARNAME_FUELTANKFIREDAMAGECOMPONENT2:
		case ML_ENGINECOM_VARNAME_FUELTANKFIREDAMAGECOMPONENT3:
		case ML_ENGINECOM_VARNAME_FUELTANKFIREDAMAGECOMPONENT4:
		case ML_ENGINECOM_VARNAME_FUELTANKFIREDAMAGECOMPONENT5:
		case ML_ENGINECOM_VARNAME_FUELTANKFIREDAMAGECOMPONENT6:
		case ML_ENGINECOM_VARNAME_FUELTANKFIREDAMAGECOMPONENT7:
		case ML_ENGINECOM_VARNAME_FUELTANKFIREDAMAGECOMPONENT8:
		{
			UINT32 ComponentNum;
			
			strcpy((char*)ComponentData->FuelBurnDamageComponent[VarNum-ML_ENGINECOM_VARNAME_FUELTANKFIREDAMAGECOMPONENT1],VarContents);

			if (Vehicle != NULL && Vehicle->VehicleType != NULL)
			{
				vehFindHitmapObjectNumByName(&ComponentNum, (UCHAR*)VarContents, Vehicle->VehicleType->VehicleTypeNum);
				if (ComponentNum == 0)
					Write_Debug("Fuel fire damage component %s not found in %s", VarContents, Component->ComponentName);
			}
		}
		break;
		case ML_ENGINECOM_VARNAME_PROPAIRFOIL:
			if (AirfoilList == NULL)
			{
				Write_Debug("Attempting to add a propellor airfoil with no airfoils in vehicle!");
				break;
			}
			Found = 0;
			for (i=0;i<AirfoilList->NumAirfoils;i++)
			{
				if (strncmp((char *)VarContents,(char *)AirfoilList->AirfoilData[i].AirfoilName,strlen((char *)VarContents)) == 0)
				{
					ComponentData->PropAirfoil = &AirfoilList->AirfoilData[i];

					Found = 1;
					break;
				}
			}

			if (Found == 0)
			{
				ComponentData->PropAirfoil = NULL;
				Write_Debug("Airfoil type %s for component %s not found",VarContents,Component->ComponentName);
			}

			break;
		case ML_ENGINECOM_VARNAME_PROPPOWEREFFICIENCY:
			ComponentData->PropPowerEfficiency = (FLOAT32)atof(VarContents);
			break;
		case ML_ENGINECOM_VARNAME_PROPINDUCEDDRAGMULT:
			ComponentData->PropInducedDragMult = (FLOAT32)atof(VarContents);
			break;
		default:
			Write_Debug("Unidentified variable %s in component %s\n",VarName,Component->ComponentName);
			break;

	}

	return 0;
}

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
							   void * VehicleData,
							   FLOAT64 LastVehicleUpdateTime,
							   FLOAT32 VehicleG)
{
mlEngineComponentData * EngineComponentData;
UINT32 i;
UINT32 TempInt;

	if (Force == NULL || Torque == NULL)
	{
		return 1;

	}

	// resolve damage
	TempInt = 0;
	for (i=0;i<4;i++)
	{
		TempInt += Component->NewDamageNumHits[i];
	}

	Component->PrevDamage = Component->CurrentDamage;

	if (TempInt > 0)
	{
		for (i=0;i<4;i++)
		{
			Component->CurrentDamage += Component->NewDamage[i];
		}
	}

	Force->x = Force->y = Force->z = 0;
	Torque->Roll = Torque->Pitch = Torque->Yaw = 0;

	if (Component == NULL || ComponentVel == NULL)
	{
		return 1;
	}

	EngineComponentData = (mlEngineComponentData *)Component->SpecialData;

	if (EngineComponentData == NULL)
	{
		return 1;
	}

	switch (EngineComponentData->SubType)
	{

		case ML_ENGINE_SUBTYPE_PROPELLOR:
				mlUpdatePropellor(Force,
									Torque,
									EngineComponentData,
									Component,
									Alt,
									ComponentVel,
									DeltaTime,
									AttVel,
									PropPitchAdjust,
									FailSafeMode,
									VehicleObjectData);
			break;
		case ML_ENGINE_SUBTYPE_ENGINE:
				mlUpdateEngine(Force,
								Torque,
								EngineComponentData,
								Component,
								Alt,
								ComponentVel,
								DeltaTime,
								ThrottlePos,
								FailSafeMode,
								GearChange,
								ToggleWepMode,
								VehicleObjectData,
								VehicleData,
								VehicleG);
			break;
		case ML_ENGINE_SUBTYPE_FUELTANK:
		case ML_ENGINE_SUBTYPE_OILTANK:
		case ML_ENGINE_SUBTYPE_RADIATOR:
		case ML_ENGINE_SUBTYPE_WEPTANK:
				_UpdateTank(EngineComponentData,Component,ComponentList,DeltaTime,(vehVehicle *)VehicleData);
			break;
		case ML_ENGINE_SUBTYPE_TRANSMISSION:
				_UpdateTransmission(EngineComponentData,
									Component,
									DeltaTime,
									LeftTrackClutch,
									RightTrackClutch);
			break;
		case ML_ENGINE_SUBTYPE_TORQUEGENERATOR:
				_UpdateTorqueGenerator(EngineComponentData,
										Component,
										DeltaTime,
										ThrottlePos,
										FailSafeMode);
			break;
		case ML_ENGINE_SUBTYPE_CLUTCH:
				_UpdateClutch(EngineComponentData,
								Component,
								ClutchPos,
								DeltaTime,
								FailSafeMode);
			break;
		case ML_ENGINE_SUBTYPE_GEARBOX:
				_UpdateGearbox(EngineComponentData,
								Component,
								GearChange,
								ReverseToggle,
								FailSafeMode,
								LastVehicleUpdateTime);
			break;
		default:
			break;
	}

	return 0;

}

static UINT32 _UpdateTank(mlEngineComponentData * TankData,
						  mlComponent * Component,
						  mlComponentList * ComponentList,
						  FLOAT64 DeltaTime,
						  vehVehicle * VehicleData)
{
FLOAT64 TankFullPercent;

	if (TankData->TankIsSetUp == 0)
	{
		TankData->TankOriginalMass = Component->Mass;
		TankData->TankOriginalRotInertia = Component->RotInertia;

		TankData->CurrentTankContents = Component->Mass;
		
		TankData->TankIsSetUp = 1;
	}

	// calculate percent tank full
	TankFullPercent = TankData->CurrentTankContents / TankData->TankOriginalMass;

	Component->Mass = TankData->TankOriginalMass * TankFullPercent;
	Component->RotInertia.Pitch = TankData->TankOriginalRotInertia.Pitch * TankFullPercent;
	Component->RotInertia.Roll = TankData->TankOriginalRotInertia.Roll * TankFullPercent;
	Component->RotInertia.Yaw = TankData->TankOriginalRotInertia.Yaw * TankFullPercent;

	// update gauges
	if (TankData->TankNum != 0)
	{
		switch(TankData->SubType)
		{
			case ML_ENGINE_SUBTYPE_FUELTANK:
					mlUpdateFuelTankGauges(TankData->TankNum-1,TankFullPercent);
				break;
			case ML_ENGINE_SUBTYPE_OILTANK:
					mlUpdateOilTankLevel(TankData->TankNum-1,TankFullPercent);
				break;
			case ML_ENGINE_SUBTYPE_RADIATOR:
					mlUpdateRadiatorGauges(TankData->TankNum-1,TankFullPercent);
				break;
			case ML_ENGINE_SUBTYPE_WEPTANK:
					mlUpdateWepTankGauges(TankData->TankNum-1,TankFullPercent);
				break;
			default:
				break;


		}

		// handle damage stuff
		switch (TankData->SubType)
		{
			case ML_ENGINE_SUBTYPE_FUELTANK:
					_HandleFuelTankDamage(TankData,Component,ComponentList,DeltaTime,VehicleData);
				break;
			case ML_ENGINE_SUBTYPE_OILTANK:
					_HandleGenericTankDamage(TankData,Component,DeltaTime,(UCHAR *)"oilleak",VehicleData);
				break;
			case ML_ENGINE_SUBTYPE_RADIATOR:
					_HandleGenericTankDamage(TankData,Component,DeltaTime,(UCHAR *)"glycolleak",VehicleData);
				break;
			case ML_ENGINE_SUBTYPE_WEPTANK:
					_HandleGenericTankDamage(TankData,Component,DeltaTime,(UCHAR *)"wepleak",VehicleData);
				break;
			default:
				break;
		}
	}

	return 0;
}

static UINT32 _HandleGenericTankDamage(mlEngineComponentData * TankData,  mlComponent * Component,
									  	FLOAT64 DeltaTime,  UCHAR *StateName,  vehVehicle *VehicleData)
{
	static const FLOAT64 DamageLeakRates[4] = { 0.01, 0.1, 1.0, 20.0 };
	FLOAT64 LeakRate;
	SINT32 i;
	char s[64];
	
	LeakRate = 0.0;
	
	for(i = 3; i >= 0; i--)
	{
		if(Component->DamageLevels[i] > 0.001 && Component->CurrentDamage > Component->DamageLevels[i])
		{
			LeakRate = DamageLeakRates[i];
			break;
		}
	}
	
	if(LeakRate > 0.001 && TankData->CurrentTankContents > 0.01)
	{ 
		TankData->CurrentTankContents -= LeakRate * DeltaTime / 1000.0;
		TankData->TankIsLeaking = 1;
	}else
	{
		TankData->TankIsLeaking = 0;
	}

	if (TankData->CurrentTankContents < 0.0)
		TankData->CurrentTankContents = 0.0;
	
	if(IsFirstTic)
	{
		snprintf(s, 63, "%s%d", StateName, TankData->TankNum);

		if (!TankData->TankIsLeaking || TankData->CurrentTankContents < 0.01)
			vehAnimChangeVehicleStateByName(VehicleData, (UCHAR*)s, FALSE);
		else
			vehAnimChangeVehicleStateByName(VehicleData, (UCHAR*)s, TRUE);
	}
	
	return 0;
}

static UINT32 _HandleFuelTankDamage(mlEngineComponentData* TankData, mlComponent* Component, mlComponentList* ComponentList,
									  FLOAT64 DeltaTime,  vehVehicle* VehicleData)
{
	SINT32 DamageLevel;
	SINT32 LeakRate;
	FLOAT64 FlowRate;
	UINT32 NewHits;
	UINT32 i;
	FLOAT64 Energy;
	FLOAT64 DamPerHit;
	FLOAT32 Probability;
	SINT32 DamageComponentNum;
	UINT32 SealTime;
	UCHAR s[64];
	
	DamageLevel = 0;
	
	for(i = 0; i < 4; i++)
	{
		if(Component->DamageLevels[i] > 0 && Component->CurrentDamage > Component->DamageLevels[i])
			DamageLevel	= i + 1;
	}
	
	NewHits = 0;
	for(i = 0; i < 4; i++)
		NewHits += Component->NewDamageNumHits[i];

	if (NewHits > 0)
	{
		if (TankData->CurrentTankContents > 0.01)
		{
			// check for starting leaking fuel tank fire
			if (DamageLevel - TankData->FuelTankSealedLevel > 0 ||
				VehicleData->VehicleType->Type == VEH_TYPE_SEA)
			{
				LeakRate = DamageLevel - TankData->FuelTankSealedLevel;

				// basically this works out to be a leak level 1 hit by a .303 round
				// at 100 meters distance is about 1% likely to start a round
				// however a 20mm M-round fired from a MG 151/20 is about 30-50% likely
				// to start a fire (rising to 100% or more with higher leak levels)
				//
				// probabilities altered jan 2001
				DamPerHit = (Component->CurrentDamage - Component->PrevDamage) / NewHits;
				Probability = (FLOAT32)(sqrt(DamPerHit) / 600.0 * LeakRate); 
				for (i=0;i<NewHits;i++)
				{
					if (mathGetFloat32ChaoticNumber() < Probability)
					{
						TankData->TankOnFire = 1;
					}

					if (TankData->FuelTankType == ML_ENGINECOM_FUELTANKTYPE_INTEGRAL_LIGHTSELFSEALING ||
						TankData->FuelTankType == ML_ENGINECOM_FUELTANKTYPE_INTEGRAL)
					{
						// check for partially-full integral fuel tank fire
						// once a tank is holed, it's "ventilated" until it's 
						// sealed again
						if (DamageLevel - TankData->FuelTankSealedLevel <= 0)
						{
							Probability = (FLOAT32)((TankData->TankOriginalMass - TankData->CurrentTankContents) /
													 TankData->TankOriginalMass);

							Probability *= (FLOAT32)(DamPerHit / 2000.0 * LeakRate); 

							if (mathGetFloat32ChaoticNumber() < Probability)
							{
								TankData->TankOnFire = 1;
								if (Component->DamageLevels[3] > 0.001)
								{
									if (Component->CurrentDamage < Component->DamageLevels[3])
									{
										Component->CurrentDamage= Component->DamageLevels[3] + 1;
									}
								}
								else
								{
									if (Component->CurrentDamage < Component->DamageLevels[0])
									{
										Component->CurrentDamage = Component->DamageLevels[0] + 1;
									}
								}
							}
						}
					}
				}
			}


		}

		// adjust self-sealing level if applicable
		TankData->FuelTankLastHitTime = frametime;
		TankData->FuelTankSealedLevel -= NewHits;
		if (TankData->FuelTankSealedLevel < 0)
		{
			TankData->FuelTankSealedLevel = 0;
		}

	}

	// adjust self-sealing level if applicable
	if (DamageLevel > TankData->FuelTankSealedLevel)
	{
		if (TankData->FuelTankType == ML_ENGINECOM_FUELTANKTYPE_INTEGRAL_LIGHTSELFSEALING ||
			 TankData->FuelTankType == ML_ENGINECOM_FUELTANKTYPE_BAG_LIGHTSELFSEALING ||
			 TankData->FuelTankType == ML_ENGINECOM_FUELTANKTYPE_BAG_GOODSELFSEALING)
		{	
			if (VehicleData->VehicleType->Type == VEH_TYPE_SEA)
			{
				SealTime = 600000;
			}
			else
			{
				SealTime = 10000;
			}
			if (TankData->FuelTankSealedLevel == 0 && 
				frametime - TankData->FuelTankLastHitTime > SealTime)
			{
				TankData->FuelTankSealedLevel = 1;
			}

			if (TankData->FuelTankType == ML_ENGINECOM_FUELTANKTYPE_BAG_GOODSELFSEALING &&
				TankData->FuelTankSealedLevel == 1 &&
				frametime - TankData->FuelTankLastHitTime > SealTime * 2)
			{
				TankData->FuelTankSealedLevel = 2;
			}
		}
	}

	// leak tank
	// hack for boats and cheese, no fuel lost for cheese boats when on "fire"
	LeakRate = DamageLevel - TankData->FuelTankSealedLevel;
	if (VehicleData->VehicleType->Type != VEH_TYPE_SEA)
	{
		if ((LeakRate > 0 || DamageLevel > 3) &&
			TankData->CurrentTankContents > 0.01)
		{
			if (DamageLevel > 3)
			{
				FlowRate = TankData->TankOriginalMass * 0.2;
			}
			if (LeakRate == 1)
			{
				FlowRate = 0.01;
			}
			if (LeakRate == 2)
			{
				FlowRate = 0.1;
			}
			if (LeakRate == 3)
			{
				FlowRate = 1.0;
			}

			TankData->CurrentTankContents -= FlowRate * DeltaTime / 1000.0;
			TankData->TankIsLeaking = 1;
		}
		else
		{
			FlowRate = 0.0;
			TankData->TankIsLeaking = 0;
		}
		if (TankData->CurrentTankContents < 0.0)
		{
			TankData->CurrentTankContents = 0.0;
		}
	}

	// do burn damage if on fire
	if (TankData->CurrentTankContents < 0.01)
	{
		TankData->TankOnFire = 0;
	}

	if (TankData->TankOnFire == 1)
	{
		TankData->LastBurnDamageCounter += (FLOAT32)DeltaTime;

		if (TankData->LastBurnDamageCounter > 1000.0)
		{
			TankData->LastBurnDamageCounter -= 1000.0;
			Energy = 0.0f;
			if (DamageLevel > 3)
			{
				// catastrophic failure of tank
				Energy = 1000000.0f;
			}
			if (LeakRate == 1)
			{
				Energy = 200.0f;
			}
			if (LeakRate == 2)
			{
				Energy = 4000.0f;
			}
			if (LeakRate == 3)
			{
				Energy = 80000.0f;
			}
			
			if(IsFirstTic)
				vehAnimChangeVehicleStateByName(VehicleData, (UCHAR*)"tankfire", TRUE);

			for (i=0;i<8;i++)
			{
				if(TankData->FuelBurnDamageComponent[0])
				{
					DamageComponentNum = 0;
					vehFindHitmapObjectNumByName((UINT32*)&DamageComponentNum,
												TankData->FuelBurnDamageComponent[i],
												VehicleData->VehicleType->VehicleTypeNum);
					if (DamageComponentNum > 0)
					{
						vehApplyDamageToVehicleComponent(VehicleData,
												  DamageComponentNum,
												  Energy,
												  0);

						}
					else
					{
						Write_Debug("Attempting to ammo-damage hit location %s, component %s, hit location not found",
										TankData->FuelBurnDamageComponent[i],
										Component->ComponentName);
					}
				}
			}
		}
	}

	if(IsFirstTic) // 50/sec not good
	{
		// update the fuel tank fire/leak state
		// leak
		sprintf((char*)s,"fuelleak%d",TankData->TankNum);
		if (TankData->TankOnFire == 1 || DamageLevel - TankData->FuelTankSealedLevel <= 0 ||
			TankData->CurrentTankContents < 0.01)
		{
			vehAnimChangeVehicleStateByName(VehicleData, s, FALSE);
		}
		else
		{
			vehAnimChangeVehicleStateByName(VehicleData, s, TRUE);
		}

		// fire
		sprintf((char *)s,"fuelfire%d",TankData->TankNum);
		if (TankData->TankOnFire == 1)
		{
			vehAnimChangeVehicleStateByName(VehicleData, s, TRUE);
		}
		else
		{
			vehAnimChangeVehicleStateByName(VehicleData, s, FALSE);
		}
	}

	return 0;
}

// basic theory is this:  The engine spins up/down the flywheel.  The Flywheel
// spins up/down the drive wheels.  
static UINT32 _UpdateTransmission(mlEngineComponentData * TransData,
								mlComponent * Component,
								FLOAT64 DeltaTime,
								FLOAT64 InLeftTrackClutch,
								FLOAT64 InRightTrackClutch)
{
FLOAT64 Power;
FLOAT64 Speed;
FLOAT64 WheelSpeed;
FLOAT64 Ratio;
mlGearComponentData * DriveWheel1;
mlGearComponentData * DriveWheel2;
FLOAT64 Rpm1,Rpm2;
FLOAT64 RightTrackClutch,LeftTrackClutch;
FLOAT64 TempNum;

	DriveWheel1 = DriveWheel2 = NULL;
	if (TransData->DriveWheel1 != NULL)
	{
		DriveWheel1 = (mlGearComponentData *)TransData->DriveWheel1->SpecialData;
	}
	if (TransData->DriveWheel2 != NULL)
	{
		DriveWheel2 = (mlGearComponentData *)TransData->DriveWheel2->SpecialData;
	}

	Power = TransData->CurrentPower;

	if (TransData->TransmissionType == ML_ENGINECOM_TRANSTYPE_LIMITEDSLIP)
	{
		if (DriveWheel1 != NULL && DriveWheel2 != NULL)
		{
			if (DriveWheel1->LastAxleTorque == 0 && DriveWheel2->LastAxleTorque == 0)
			{
				Ratio = -1.0;
			}
			else
			{
				Ratio = fabs(DriveWheel1->LastAxleTorque) / 
					(fabs(DriveWheel1->LastAxleTorque) + fabs(DriveWheel2->LastAxleTorque));
			}
		}
		else
		{
			Ratio = -1.0;
		}
		if (DriveWheel1 != NULL)
		{	
			WheelSpeed = fabs(DriveWheel1->WheelRotationSpeed);

			if (WheelSpeed < 1.0)
			{
				WheelSpeed = 1.0;
			}

			// /2 because there are 2 wheels feeding off same powerplant
			if (Ratio < 0.0)
			{
				DriveWheel1->DriveTorque = Power / WheelSpeed;
			}
			else
			{
				DriveWheel1->DriveTorque = Power / WheelSpeed * (1.0 -Ratio);
			}

			Rpm1 = DriveWheel1->WheelRotationSpeed / PI / 2 * 60.0;
		}

		if (DriveWheel2 != NULL)
		{
			WheelSpeed = fabs(DriveWheel2->WheelRotationSpeed);

			if (WheelSpeed < 1.0)
			{
				WheelSpeed = 1.0;
			}

			// /2 because there are 2 wheels feeding off same powerplant
			if (Ratio < 0.0)
			{
				DriveWheel2->DriveTorque = Power / WheelSpeed;
			}
			else
			{
				DriveWheel2->DriveTorque = Power / WheelSpeed * (Ratio);
			}

			Rpm2 = DriveWheel2->WheelRotationSpeed / PI / 2 * 60.0;
		}

		if (DriveWheel1 != NULL)
		{
			if (DriveWheel2 != NULL)
			{
				TransData->CurrentRpm = (Rpm1 + Rpm2) / 2.0;
			}
			else
			{
				TransData->CurrentRpm = Rpm1;
			}
		}
		else
		{
			if (DriveWheel2 != NULL)
			{
				TransData->CurrentRpm = Rpm2;
			}
			else
			{
				TransData->CurrentRpm = 0.0;
			}
		}
	}

	// this particular clutch and brake model assumes that
	// you always have one track clutched in, and are varying the other
	//
	// to do this like real life would require coding in 
	// a rotational inertia model and independant rpm with clutch code
	// to transfer power
	if (TransData->TransmissionType == ML_ENGINECOM_TRANSTYPE_CLUTCHANDBRAKE)
	{
		RightTrackClutch = 1.0 - InRightTrackClutch * 10.0 / 3.0;
		LeftTrackClutch = 1.0 - InLeftTrackClutch * 10.0 / 3.0;
		if (RightTrackClutch < 0.0)
		{
			RightTrackClutch = 0.0;
		}
		if (LeftTrackClutch < 0.0)
		{
			LeftTrackClutch = 0.0;
		}

		if (LeftTrackClutch < 0.001 && RightTrackClutch < 0.001)
		{
			Ratio = 0.5;
		}
		else
		{
			if (LeftTrackClutch < RightTrackClutch)
			{
				Ratio = LeftTrackClutch / RightTrackClutch / 2;
			}
			else
			{
				Ratio = 1.0 - RightTrackClutch / LeftTrackClutch / 2;
			}
		}

		if (DriveWheel1 != NULL)
		{
			if (DriveWheel1->WheelRotationSpeed > 1.0)
			{
				DriveWheel1->DriveTorque = TransData->CurrentPower * Ratio / fabs(DriveWheel1->WheelRotationSpeed);
			}
			else
			{
				DriveWheel1->DriveTorque = TransData->CurrentPower * Ratio;
			}
		}

		if (DriveWheel2 != NULL)
		{
			if (DriveWheel2->WheelRotationSpeed > 1.0)
			{
				DriveWheel2->DriveTorque = TransData->CurrentPower * (1.0 - Ratio) / fabs(DriveWheel2->WheelRotationSpeed);
			}
			else
			{
				DriveWheel2->DriveTorque = TransData->CurrentPower * (1.0 - Ratio);
			}
		}

// fix rpm
		Rpm1 = Rpm2 = 0.0;
		if (DriveWheel1 != NULL)
		{
			Rpm1 = DriveWheel1->WheelRotationSpeed / PI / 2 * 60.0;
		}
		if (DriveWheel2 != NULL)
		{
			Rpm2 = DriveWheel2->WheelRotationSpeed / PI / 2 * 60.0;
		}

		if (DriveWheel1 != NULL)
		{
			if (DriveWheel2 != NULL)
			{
				TransData->CurrentRpm = Rpm1 * Ratio + Rpm2 * (1.0 - Ratio);
			}
			else
			{
				TransData->CurrentRpm = Rpm1;
			}
		}
		else
		{
			if (DriveWheel2 != NULL)
			{
				TransData->CurrentRpm = Rpm2;
			}
			else
			{
				TransData->CurrentRpm = 0.0;
			}
		}
	}


	TransData->EpicyclicNeutralEngagedPercent = 0.0;
	if (TransData->TransmissionType == ML_ENGINECOM_TRANSTYPE_EPICYCLIC)
	{
		if (TransData->InNeutral == 1)
		{
			RightTrackClutch = 1.0 - InRightTrackClutch;
			LeftTrackClutch = 1.0 - InLeftTrackClutch;

			if (LeftTrackClutch < 0.001 && RightTrackClutch < 0.001)
			{
				Ratio = 0.0;
			}
			else
			{
				if (LeftTrackClutch < RightTrackClutch)
				{
					Ratio = (LeftTrackClutch - RightTrackClutch)/(RightTrackClutch) * 0.5;
				}
				else
				{
					Ratio = (LeftTrackClutch - RightTrackClutch)/(LeftTrackClutch) * 0.5;
				}
			}
			
			TempNum = fabs(DriveWheel1->WheelRotationSpeed) + fabs(DriveWheel2->WheelRotationSpeed);

			if (fabs(TempNum) > 1.0)
			{
				DriveWheel1->DriveTorque = TransData->CurrentPower * Ratio / TempNum;
			}
			else
			{
				DriveWheel1->DriveTorque = TransData->CurrentPower * Ratio;
			}

			if (fabs(TempNum) > 1.0)
			{
				DriveWheel2->DriveTorque = TransData->CurrentPower * (-Ratio) / TempNum;
			}
			else
			{
				DriveWheel2->DriveTorque = TransData->CurrentPower * (-Ratio);
			}

			TransData->EpicyclicNeutralEngagedPercent = fabs(Ratio) * 2.0;
		}
		else
		{
			RightTrackClutch = 1.0 - InRightTrackClutch;
			LeftTrackClutch = 1.0 - InLeftTrackClutch;

			if (LeftTrackClutch < 0.001 && RightTrackClutch < 0.001)
			{
				Ratio = 0.5;
			}
			else
			{
				if (LeftTrackClutch < RightTrackClutch)
				{
					Ratio = LeftTrackClutch / RightTrackClutch / 2;
				}
				else
				{
					Ratio = 1.0 - RightTrackClutch / LeftTrackClutch / 2;
				}
			}
			if (DriveWheel1->WheelRotationSpeed > 1.0)
			{
				DriveWheel1->DriveTorque = TransData->CurrentPower * Ratio / fabs(DriveWheel1->WheelRotationSpeed);
			}
			else
			{
				DriveWheel1->DriveTorque = TransData->CurrentPower * Ratio;
			}

			if (DriveWheel2->WheelRotationSpeed > 1.0)
			{
				DriveWheel2->DriveTorque = TransData->CurrentPower * (1.0 - Ratio) / fabs(DriveWheel2->WheelRotationSpeed);
			}
			else
			{
				DriveWheel2->DriveTorque = TransData->CurrentPower * (1.0 - Ratio);
			}
		}

		Rpm1 = Rpm2 = 0.0;
		if (DriveWheel1 != NULL)
		{
			Rpm1 = DriveWheel1->WheelRotationSpeed / PI / 2 * 60.0;
		}
		if (DriveWheel2 != NULL)
		{
			Rpm2 = DriveWheel2->WheelRotationSpeed / PI / 2 * 60.0;
		}

		TransData->CurrentRpm = (Rpm1 + Rpm2) / 2.0;
	}

	Speed = 0.0;
	if (DriveWheel1 == NULL)
	{
		if (DriveWheel2 != NULL)
		{
			Speed = DriveWheel2->WheelRotationSpeed * DriveWheel2->WheelRadius;
		}
	}
	else if (DriveWheel2 == NULL)
	{
		if (DriveWheel1 != NULL)
		{
			Speed = DriveWheel1->WheelRotationSpeed * DriveWheel1->WheelRadius;
		}
	}
	else
	{
		Speed = DriveWheel1->WheelRotationSpeed * DriveWheel1->WheelRadius +
				DriveWheel2->WheelRotationSpeed * DriveWheel2->WheelRadius;
		Speed /= 2;
	}

	mlGaugeUpdatePlayerSpeed(Speed);

	return 0;
}

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
								mlObjectData * VehicleObjectData)
{
	FLOAT64 Torque;
	FLOAT64 Force;
	FLOAT64 Thrust;
	FLOAT64 Lift,Drag;
	mlEngineData * EngineData;
	FLOAT64 CenterOfLiftDrag;
	FLOAT64 AspectRatio;
	FLOAT64 Density;
	FLOAT64 ComponentVelocity;
	FLOAT64 ComponentSavedVelocity;
	FLOAT64 PropVel;
	FLOAT64 RelativeVelocity;
	FLOAT64 Angle;
	FLOAT64 Q;
	FLOAT64 AngleAttained;
	FLOAT64 Delta;
	FLOAT64 TempNum;
	FLOAT64 Cl,Cd,Cm;
	FLOAT64 RotInertia;
	FLOAT64 AngularMomentum;
	FLOAT64 ZeroAoASpeed;
	FLOAT64 YawTorque;
	FLOAT64 PitchTorque;
	FLOAT64 TempThrust;
	FLOAT64 PowerEfficiency;
	FLOAT64 BackWashVelocity;
	FLOAT64 PropArea;
	FLOAT64 UseRpm;
	FLOAT64 Rotate;
	FLOAT64 ClDragMult,ClDrag;
	FLOAT32 Volume,Pitch;


	ReturnForce->x = ReturnForce->y = ReturnForce->z = 0.0;
	ReturnTorque->Roll = ReturnTorque->Pitch = ReturnTorque->Yaw = 0.0;

	EngineData = PropellerData->EngineData;

	if(EngineData == NULL) return 1;
	if(PropellerData->PropellorArea == 0) return 1;

	PropArea = PropellerData->PropellorArea;
	if (Component->CurrentDamage > Component->DamageLevels[0] && Component->DamageLevels[0] > 0.001)
		PropArea *= 0.8;
		
	if (Component->CurrentDamage > Component->DamageLevels[2] && Component->DamageLevels[2] > 0.001)
		PropArea *= 0.5;
		
	if (Component->CurrentDamage > Component->DamageLevels[3] && Component->DamageLevels[3] > 0.001)
		PropArea = 0.0001;

	if (PropellerData->PropInducedDragMult < 0.001f)
		PropellerData->PropInducedDragMult = 1.0f;
		
	if (PropellerData->PropPowerEfficiency < 0.001f)
		PropellerData->PropPowerEfficiency = 1.0f;

	// autoprop pitch adjuster
	if (PropellerData->PropHasGovener == 1 && FailSafeMode == 0)
	{
		if (PropellerData->RpmSetting < 10.0)
			PropellerData->RpmSetting = EngineData->ClimbRpm;

		if (PropPitchAdjustor > 0)
			PropellerData->RpmSetting += 500.0 * DeltaTime / 1000.0;

		if (PropPitchAdjustor < 0)
			PropellerData->RpmSetting -= 500.0 * DeltaTime / 1000.0;
	}

	if (FailSafeMode == 1)
	{
		if (PropellerData->RpmSetting < EngineData->EconRpm)
			PropellerData->RpmSetting = EngineData->EconRpm;

		if (PropPitchAdjustor > 0 && PropellerData->LastPropPitchAdjustor <= 0)
		{
			if (PropellerData->RpmSetting >= EngineData->ClimbRpm)
			{
				PropellerData->RpmSetting = EngineData->MilRpm;
				rnmainListText(loclGetLocalizationStringByID(loclSET_MAX_RPM_MSG));
			}
			else
			{
				PropellerData->RpmSetting = EngineData->ClimbRpm;
				rnmainListText(loclGetLocalizationStringByID(loclSET_CONTINUOUS_RPM_MSG));
			}
		}
		if (PropPitchAdjustor < 0 && PropellerData->LastPropPitchAdjustor >= 0)
		{
			if (PropellerData->RpmSetting <= EngineData->ClimbRpm)
			{
				PropellerData->RpmSetting = EngineData->EconRpm;
				rnmainListText(loclGetLocalizationStringByID(loclSET_ECONOMY_RPM_MSG));
			}
			else
			{
				PropellerData->RpmSetting = EngineData->ClimbRpm;
				rnmainListText(loclGetLocalizationStringByID(loclSET_CONTINUOUS_RPM_MSG));
			}
		}
	}
	PropellerData->LastPropPitchAdjustor = PropPitchAdjustor;

	if (PropellerData->RpmSetting < EngineData->IdleRpm)
		PropellerData->RpmSetting = EngineData->IdleRpm;

	if (PropellerData->RpmSetting > EngineData->MilRpm)
		PropellerData->RpmSetting = EngineData->MilRpm;

	UseRpm = PropellerData->CurrentRpm * EngineData->PropellerGearRatio;

	// cube root of 1/2 = 0.7937
	CenterOfLiftDrag = 0.7937 * PropellerData->PropellorDiameter / 2;
	AspectRatio = PropellerData->PropellorDiameter * PropellerData->PropellorDiameter / 4 /
					PropellerData->PropellorArea;

	if (AspectRatio == 0)
	{
		return 1;
	}

	PropellerData->LastPropAngle = PropellerData->CurrentPropAngle;

	Delta = 0;

	if (PropellerData->PropHasGovener == 1 ||
		FailSafeMode == 1)
	{
		// auto pitch ajustor
		if (FailSafeMode == 1 && PropellerData->PropHasGovener == 0)
		{
			TempNum = 0.5; // manual prop pitch planes in failsafe mode have 1/2 pitch response
		}
		else
		{
			TempNum = 1.0;
		}

		if (PropellerData->CurrentRpm < PropellerData->RpmSetting - 5.0)
		{
			Delta = -PropellerData->PropellorResponse * DeltaTime / 1000.0 * TempNum;
		}

		if (PropellerData->CurrentRpm > PropellerData->RpmSetting + 5.0)
		{
			Delta = PropellerData->PropellorResponse * DeltaTime / 1000.0 * TempNum;
		}

	}
	else
	{
		// manual prop pitch adjustor
		if (PropPitchAdjustor < 0)
		{
			Delta = -PropellerData->PropellorResponse * DeltaTime / 1000.0;
		}
		if (PropPitchAdjustor > 0)
		{
			Delta = PropellerData->PropellorResponse * DeltaTime / 1000.0;
		}
	}

	if (Component->CurrentDamage > Component->DamageLevels[1] &&
		Component->DamageLevels[1] > 0.001)
	{
		Delta = -PropellerData->PropellorResponse * DeltaTime / 1000.0;
	}


	AngleAttained = PropellerData->CurrentPropAngle + Delta;
	if (AngleAttained > PropellerData->PropellorMaxAngle)
	{
		AngleAttained = PropellerData->PropellorMaxAngle;
	}

	if (AngleAttained < PropellerData->PropellorMinAngle)
	{
		AngleAttained = PropellerData->PropellorMinAngle;
	}

	PropellerData->CurrentPropAngle = AngleAttained;

	Thrust = 0.0;
	Force = 0.0;
	YawTorque = 0.0;
	PitchTorque = 0.0;

	if (PropellerData->PropClDragMult > 0.001)
	{
		ClDragMult = PropellerData->PropClDragMult;
	}
	else
	{
		ClDragMult = 1.0;
	}

	ComponentVelocity = ComponentVel->y;
	ComponentSavedVelocity = ComponentVel->y;
	ComponentVelocity = ComponentVel->y + PropellerData->PropSeesVelocity;

	// ok we calculate forces on the left/right/up/down quarters of the prop disc
	// this allows P factor to come out on its own
	mlGetAltDensity(&Density,Alt);

	// top quarter
	PropVel = CenterOfLiftDrag * 2 * PI * UseRpm / 60.0;
	PropVel += EngineData->PropSpinDirection * ComponentVel->x;

	Rotate = Angle = atan2(ComponentVelocity,PropVel);
	Angle -= mlDEG_TO_RAD(AngleAttained);
	PropellerData->LastPropAoA = (FLOAT32)Rotate;
	RelativeVelocity = sqrt(ComponentVelocity * ComponentVelocity + PropVel * PropVel);

	Q = 0.5 * Density * RelativeVelocity * RelativeVelocity;

	if (PropellerData->PropAirfoil != NULL)
	{
		mlFoilsGetBaseClCdCm(&Cl,&Cd,&Cm,-Angle,PropellerData->PropAirfoil);
	}
	else
	{
		Cl = Cd = Cm = 0.0;
	}
	PropellerData->LastWindAoA = mlRAD_TO_DEG(Rotate);

	TempNum = 1.0f;

	ClDrag = Cl * ClDragMult;
	Lift = Cl * Q * PropArea;
	Drag = Cd * Q * PropArea; // parasitic drag
	Drag += ClDrag * ClDrag * PropellerData->PropellorEfficiency / AspectRatio * Q * PropArea *
				PropellerData->PropInducedDragMult * TempNum;

	// calculate countertorque and thrust
	TempThrust = 0.25 * (cos(Rotate) * Lift - sin(Rotate) * Drag);
	Force += 0.25 * (sin(Rotate) * Lift + cos(Rotate) * Drag);

	PitchTorque = TempThrust * CenterOfLiftDrag;
	Thrust += TempThrust;


	// bottom quarter
	PropVel = CenterOfLiftDrag * 2 * PI * UseRpm / 60.0;
	PropVel -= EngineData->PropSpinDirection * ComponentVel->x;

	Rotate = Angle = atan2(ComponentVelocity,PropVel);
	Angle -= mlDEG_TO_RAD(AngleAttained);
	RelativeVelocity = sqrt(ComponentVelocity * ComponentVelocity + PropVel * PropVel);

	Q = 0.5 * Density * RelativeVelocity * RelativeVelocity;

	if (PropellerData->PropAirfoil != NULL)
	{
		mlFoilsGetBaseClCdCm(&Cl,&Cd,&Cm,-Angle,PropellerData->PropAirfoil);
	}
	else
	{
		Cl = Cd = Cm = 0.0;
	}

	TempNum = 1.0f;

	ClDrag = Cl * ClDragMult;
	Lift = Cl * Q * PropArea;
	Drag = Cd * Q * PropArea; // parasitic drag
	Drag += ClDrag * ClDrag * PropellerData->PropellorEfficiency / AspectRatio * Q * PropArea *
				PropellerData->PropInducedDragMult * TempNum;

	// calculate countertorque and thrust
	TempThrust = 0.25 * (cos(Rotate) * Lift - sin(Rotate) * Drag);
	Force += 0.25 * (sin(Rotate) * Lift + cos(Rotate) * Drag);

	PitchTorque -= TempThrust * CenterOfLiftDrag;
	Thrust += TempThrust;


	// left side
	PropVel = CenterOfLiftDrag * 2 * PI * UseRpm / 60.0;
	PropVel -= EngineData->PropSpinDirection * ComponentVel->z;

	Rotate = Angle = atan2(ComponentVelocity,PropVel);
	Angle -= mlDEG_TO_RAD(AngleAttained);
	RelativeVelocity = sqrt(ComponentVelocity * ComponentVelocity + PropVel * PropVel);

	Q = 0.5 * Density * RelativeVelocity * RelativeVelocity;

	if (PropellerData->PropAirfoil != NULL)
	{
		mlFoilsGetBaseClCdCm(&Cl,&Cd,&Cm,-Angle,PropellerData->PropAirfoil);
	}
	else
	{
		Cl = Cd = Cm = 0.0;
	}
	TempNum = 1.0f;
	
	ClDrag = Cl * ClDragMult;
	Lift = Cl * Q * PropArea;
	Drag = Cd * Q * PropArea; // parasitic drag
	Drag += ClDrag * ClDrag * PropellerData->PropellorEfficiency / AspectRatio * Q * PropArea *
				PropellerData->PropInducedDragMult * TempNum;

	// calculate countertorque and thrust
	TempThrust = 0.25 * (cos(Rotate) * Lift - sin(Rotate) * Drag);
	Force += 0.25 * (sin(Rotate) * Lift + cos(Rotate) * Drag);

	YawTorque += TempThrust * CenterOfLiftDrag;
	Thrust += TempThrust;


	// right side
	PropVel = CenterOfLiftDrag * 2 * PI * UseRpm / 60.0;
	PropVel += EngineData->PropSpinDirection * ComponentVel->z;

	Rotate = Angle = atan2(ComponentVelocity,PropVel);
	Angle -= mlDEG_TO_RAD(AngleAttained);
	RelativeVelocity = sqrt(ComponentVelocity * ComponentVelocity + PropVel * PropVel);

	Q = 0.5 * Density * RelativeVelocity * RelativeVelocity;

	if (PropellerData->PropAirfoil != NULL)
	{
		mlFoilsGetBaseClCdCm(&Cl,&Cd,&Cm,-Angle,PropellerData->PropAirfoil);
	}
	else
	{
		Cl = Cd = Cm = 0.0;
	}

	TempNum = 1.0f;

	ClDrag = Cl * ClDragMult;
	Lift = Cl * Q * PropArea;
	Drag = Cd * Q * PropArea; // parasitic drag
	Drag += ClDrag * ClDrag * PropellerData->PropellorEfficiency / AspectRatio * Q * PropArea *
				PropellerData->PropInducedDragMult * TempNum;

	// calculate countertorque and thrust
	TempThrust = 0.25 * (cos(Rotate) * Lift - sin(Rotate) * Drag);
	Force += 0.25 * (sin(Rotate) * Lift + cos(Rotate) * Drag);

	YawTorque += -TempThrust * CenterOfLiftDrag;
	Thrust += TempThrust;


	ReturnTorque->Pitch = PitchTorque;
	ReturnTorque->Yaw = YawTorque;

	// update countertorque and gauges
	PowerEfficiency = PropellerData->PropellorPowerEfficiency;
	if (PowerEfficiency == 0.0)
	{
		PowerEfficiency = 1.0;
	}

	PropellerData->PropCounterTorque = Force * CenterOfLiftDrag;

	ComponentVelocity = ComponentSavedVelocity;

	mlUpdatePropGauges(PropellerData->EngineNumber - 1, //.com labels engines starting at 1
					   PropellerData->CurrentPropAngle,
					   Thrust,
					   PropellerData->RpmSetting,
					   UseRpm,
					   PropPitchAdjustor);


	// propwash effects
	ZeroAoASpeed = RelativeVelocity;
	PropellerData->VortexBackWashVelocity = ZeroAoASpeed * Cl / PI /
											AspectRatio;

	BackWashVelocity = PropellerData->VortexBackWashVelocity;

	if (PropellerData->VortexBackWashVelocity <= ZeroAoASpeed &&
		EngineData->PropSpinDirection != 0.0)
	{
		PropellerData->VortexRotationSpeed = ZeroAoASpeed - sqrt(
				ZeroAoASpeed * ZeroAoASpeed - 
				PropellerData->VortexBackWashVelocity * PropellerData->VortexBackWashVelocity);

	}
	else
	{
		PropellerData->VortexRotationSpeed = 0;
	}
	// now adjust it for zero aoa angle
	TempNum = PropellerData->VortexBackWashVelocity;
	PropellerData->VortexBackWashVelocity = PropellerData->VortexBackWashVelocity * cos(mlDEG_TO_RAD(AngleAttained)) +
											PropellerData->VortexRotationSpeed * sin(mlDEG_TO_RAD(AngleAttained));
	PropellerData->VortexRotationSpeed = TempNum * sin(mlDEG_TO_RAD(AngleAttained)) +
										PropellerData->VortexRotationSpeed * cos(mlDEG_TO_RAD(AngleAttained));

	// now adjust vortexrotationspeed to an angular speed
	// sqrt(2) = 1.414213562
	// cube root of 2 is 1.2599
	PropellerData->VortexRotationSpeed *= 1.2599; // adjust for prop tip position;
	PropellerData->VortexRotationSpeed /= PropellerData->PropellorDiameter / 2; // now convert to angular velocity

	if (PropellerData->PropBlastMult > 0.01)
	{
		PropellerData->VortexRotationSpeed *= PropellerData->PropBlastMult;
		PropellerData->VortexBackWashVelocity *= PropellerData->PropBlastMult;
	}


	if (EngineData->PropSpinDirection > 0.0)
	{
		PropellerData->VortexRotationSpeed = -PropellerData->VortexRotationSpeed;
	}

	if (PropellerData->VortexBackWashVelocity < 0.0)
	{
		PropellerData->VortexRotationSpeed = 0.0;
		PropellerData->VortexBackWashVelocity = 0.0;
	}

	// now put in real prop backwash speed and velocity
	{
	FLOAT64 PropArea;
	FLOAT64 PressureDelta;
	FLOAT64 NewSpeed;
	FLOAT64 TempNum;
	FLOAT64 DeltaSpeed;
	FLOAT64 Ratio;
	FLOAT64 Orig;
	FLOAT64 PropVel;
	FLOAT64 TotalSpeed;

		Q = 0.5 * Density * ComponentVelocity * ComponentVelocity;
		
		PropArea = PropellerData->PropellorDiameter * 
						PropellerData->PropellorDiameter * 0.25 * PI;
		PressureDelta = Thrust / PropArea;

		TempNum = ((PressureDelta + Q) / (0.5 * Density));
		if (TempNum < 0.0)
		{
			NewSpeed = ComponentVelocity;
			PropVel = -sqrt(-TempNum);
		}
		else
		{
			PropVel = NewSpeed = sqrt(TempNum);
		}
		DeltaSpeed = NewSpeed - ComponentVelocity;
		Orig = PropellerData->VortexBackWashVelocity;
		
		if (PropellerData->VortexBackWashVelocity > 0.01 &&
			DeltaSpeed > 0.0)
		{
			Ratio = DeltaSpeed / PropellerData->VortexBackWashVelocity;
			PropellerData->VortexBackWashVelocity = DeltaSpeed;
		
			if (Rotate < 1.5 && Rotate > -1.5)
			{
				TotalSpeed = DeltaSpeed / cos(Rotate);
				
				NewSpeed = DeltaSpeed * sin(Rotate);

				if (EngineData->PropSpinDirection > 0.0)
				{
					NewSpeed = -NewSpeed;
				}
			}
			else
			{
				DeltaSpeed = 0.0;
			}
			PropellerData->VortexRotationSpeed = NewSpeed;
		}

		DeltaSpeed = PropVel - ComponentVelocity;

		PropellerData->PropSeesVelocity = (DeltaSpeed * 0.5 + PropellerData->PropSeesVelocity) * 0.5;
	}


	RotInertia = 0.0;
	if (EngineData->PropSpinDirection != 0.0)
	{
		// calculate gyroscopic effect
		//
		// general theory is that a gyro will precess at attvel = T / L
		// where L is the angular momentum of the gyro, and T is the torque imparted
		//
		// inverting this we get T = attvel * L, which is what we use
		// this is the "countertorque" off the gyro in respect to the vehicle's rotation velocity
		//
		// RotInertia = M * R^2 / 8
		if (PropellerData->FlyWheelRotInertia > 0.0001)
		{
			RotInertia = PropellerData->FlyWheelRotInertia;
		}
		else
		{
			RotInertia = Component->Mass * (PropellerData->PropellorDiameter*PropellerData->PropellorDiameter) / 8;
		}

		AngularMomentum = (UseRpm / 60.0) * RotInertia;

		if (EngineData->PropSpinDirection > 0)
		{
			ReturnTorque->Pitch += AttVel->Yaw * AngularMomentum;
			ReturnTorque->Yaw += -AttVel->Pitch * AngularMomentum;
		}
		else
		{
			ReturnTorque->Pitch += -AttVel->Yaw * AngularMomentum;
			ReturnTorque->Yaw += AttVel->Pitch * AngularMomentum;
		}

	}

	// now compute new rpm
	if (fabs(PropellerData->CurrentRpm) < 50.0)
	{
		Torque = PropellerData->CurrentPower;
	}
	else
	{
		Torque = PropellerData->CurrentPower / (PropellerData->CurrentRpm / 60 );
		Torque /= 2 * PI;
	}


	// crankshaft hp coming to prop gearbox
	Torque *= PropellerData->PropPowerEfficiency; 

{
FLOAT64 TempNum,TempNum1;
FLOAT64 Delta;
	TempNum = Torque / EngineData->PropellerGearRatio;

	TempNum1 = PropellerData->PropCounterTorque;

	Delta = TempNum - TempNum1;

	Torque = Delta * EngineData->PropellerGearRatio;
}

//****************************
	// now compute crankshaft hp coming from prop gearbox
//	Torque -= PropellerData->PropCounterTorque / EngineData->PropellerGearRatio;


	PropellerData->CurrentRpm += Torque / RotInertia * DeltaTime / 1000.0;
	
	if(PropellerData->CurrentRpm < 0) // eh, fixes 1.#R
		PropellerData->CurrentRpm = 0;
		
	ReturnForce->y = (_DisableEngineThrust == 0) ? -Thrust : 0.0;

	// do sounds
	if (!PropellerData->EngineSoundLoaded && PropellerData->EngineSound[0])
	{
		if (VehicleObjectData != NULL)
		{
			if (VehicleObjectData->PlaneObject != NULL)
			{
			OBJECT * TempObj;

				if (VehicleObjectData->HitMap != NULL)
				{
					TempObj = VehicleObjectData->HitMap;
				}
				else
				{
					TempObj = VehicleObjectData->PlaneObject;
				}

#if USE_OSG
                osg::Vec3d pos = osg::Vec3d(Component->OriginalRelativePos.x, Component->OriginalRelativePos.y, Component->OriginalRelativePos.z);
                osg::Group* o = ((osg::Node*)TempObj->osgNode)->asGroup();

                int id = fxSoundPlayObjectSound((char*)PropellerData->EngineSound,
                                                pos,
                                                infinite_sound,
                                                sound_mandatory,
                                                10.0f,
                                                3218.0f,
                                                o);

                PropellerData->EngineSoundId = id;

                if(id > 0)
                {
 					PropellerData->EngineSoundLoaded = 1;
                    fxSoundSetKillCallback(id, _EngineSoundStopped);
                    fxSoundSetKillData(id, PropellerData);
                }

#else
                SINT32 iID = fxSndGetNewSound();

                if(iID > 0)
                {
                    fxSndSetSoundName(iID, (SCHAR *)PropellerData->EngineSound);
                    fxSndSetPriority(iID, IDE_FP_PRIORITY_MANDATORY);
                    fxSndSetSoundType(iID, TYPE_INFINITE_SOUND);
                    fxSndSetPitch(iID, 1.0f);
                    fxSndSetVolume(iID, 1.0f);
                    fxSndSetPosition(iID, Component->OriginalRelativePos.x, Component->OriginalRelativePos.y, Component->OriginalRelativePos.z);
                    fxSndSetEmitterObject(iID, TempObj);
                    fxSndSetKillData(iID, PropellerData);
                    fxSndSetKillCallback(iID, _EngineSoundStopped);
                    fxSndSetPlayType(iID, DEF_PLAYTYPE_OBJECT);
                    fxSndSetMinDistance(iID, 10.0f);
                    fxSndSetMaxDistance(iID, 3218.0f);
                    fxSndPushFlags(iID, DEF_SOUND_STOPS_AMBIENCE);

                    if(fxSndInitSound(iID))
                    {
                        PropellerData->EngineSoundId = iID;
					    PropellerData->EngineSoundLoaded = 1;
                    }
                }
#endif
			}
		}
	}

	// if the sound wasn't loaded before but is now, we want to adjust it
	// this is why it isn't an "else" clause
	if (PropellerData->EngineSoundLoaded)
	{
		Volume = (FLOAT32)(fabs(BackWashVelocity) / 7.5);
		if (Volume > 1.0)
		{
			Volume = 1.0;
		}

        if(PropellerData->CurrentRpm < 0.001f)
            Volume = 0.0f;

#if USE_OSG
        fxSoundChangeVolume(PropellerData->EngineSoundId, Volume);
#else
		fxSndChangeVolume(PropellerData->EngineSoundId,Volume);
#endif

		Pitch = (FLOAT32)(PropellerData->CurrentRpm / EngineData->MilRpm);
		if (Pitch < 0.05)
		{
			Pitch = (FLOAT32)0.05;
		}
		if (Pitch > 2.0)
		{
			Pitch = (FLOAT32)2.0;
		}

#if USE_OSG
        fxSoundChangePitch(PropellerData->EngineSoundId, Pitch);
#else
		fxSndChangePitch(PropellerData->EngineSoundId,Pitch);
#endif
	}


	return 0;
}

#define SHIFT_TIME 500.0
static UINT32 _UpdateGearbox(mlEngineComponentData * EngineComponentData,
							 mlComponent * Component,
							 SINT32 GearChange,
							 SINT32 ReverseToggle,
							 UINT32 FailSafeMode,
							 FLOAT64 LastVehicleUpdateTime)
{
//WCHAR s[128],s1[128];
mlEngineComponentData * TorqueReceiverData;
//UINT32 WriteSomething;
mlEngineData * EngineData;
FLOAT32 NewGearXPos,NewGearYPos;
UINT32 NumCrosses;
FLOAT32 DeltaTime;

	if (EngineComponentData == NULL ||
		EngineComponentData->PropellorComponent == NULL)
	{
		return 1;
	}

	TorqueReceiverData = (mlEngineComponentData *)EngineComponentData->PropellorComponent->SpecialData;

	if (TorqueReceiverData == NULL)
	{
		return 1;
	}

	EngineData = EngineComponentData->EngineData;

	if (EngineData == NULL)
	{
		return 1;
	}

	if (FailSafeMode == 1)
	{
		if (EngineComponentData->AutoShift != 0 &&
			EngineComponentData->UseGear > 0)
		{
			GearChange = EngineComponentData->AutoShift;
		}

	}

	/*
	if (EngineComponentData->PreselectorGearShift == 1 && FailSafeMode != 1)
	{
		strcpy((char *)s,"Selected gear ");
	}
	else
	{
		strcpy((char *)s,"Shifted to gear ");
	}
	WriteSomething = 0;
	*/

	// check for gear shift
	if (EngineComponentData->CanShift == 0 && GearChange != 0 && 
		EngineComponentData->PreselectorGearShift == 0 &&
		FailSafeMode != 1)
	{
		rnmainListText(loclGetLocalizationStringByID(loclCLUTCH_MUST_BE_DISENGAGED_MSG));
	}
	else
	{
		if (GearChange != 0)
		{
			EngineComponentData->SelectedGear += GearChange;
			
			if (EngineComponentData->SelectedGear < 0)
			{
				EngineComponentData->SelectedGear = 0;
			}

			if (EngineComponentData->InReverse == 1)
			{
				if (EngineComponentData->SelectedGear > EngineComponentData->NumReverseGears)
				{
					EngineComponentData->SelectedGear = EngineComponentData->NumReverseGears;
				}
			}
			else
			{
				if (EngineComponentData->SelectedGear > EngineComponentData->NumForwardGears)
				{
					EngineComponentData->SelectedGear = EngineComponentData->NumForwardGears;
				}
			}
			
			/*
			if (EngineComponentData->SelectedGear == 0)
			{
				strcat((char *)s,"neutral");
				WriteSomething = 1;
			}
			else
			{
				sprintf((char *)s1,"%d",EngineComponentData->SelectedGear);
				strcat((char *)s,(char *)s1);
				WriteSomething = 1;
				
				if (EngineComponentData->InReverse == 1)
				{
					strcat((char *)s,loclGetLocalizationStringByID(loclGEAR_IN_REVERSE_MSG));
				}
			}
			*/
		}
	}

	// check for reverse toggle
	if (ReverseToggle != 0)
	{
		if (EngineComponentData->SelectedGear != 0)
		{
			EngineComponentData->SelectedGear = 0;
			EngineComponentData->UseGear = 0;
			EngineComponentData->ShiftTime = LastVehicleUpdateTime + SHIFT_TIME;
			EngineComponentData->StartGearX = EngineComponentData->CurrentGearX;
			EngineComponentData->StartGearY = EngineComponentData->CurrentGearY;
			EngineComponentData->StartLinearGearPos = EngineComponentData->CurrentLinearGearPos;			
		}

		if (EngineComponentData->InReverse == 1)
		{
			EngineComponentData->InReverse = 0;
			rnmainListText(loclGetLocalizationStringByID(loclSHIFTED_TO_FORWARD_GEARS_MSG));
		}
		else
		{
			EngineComponentData->InReverse = 1;
			rnmainListText(loclGetLocalizationStringByID(loclSHIFTED_TO_REVERSE_GEARS_MSG));
		}
	}

	if (EngineComponentData->InReverse == 1)
	{
		mlUpdateGearShiftState(-(SINT32)EngineComponentData->SelectedGear);
	}
	else
	{
		mlUpdateGearShiftState((SINT32)EngineComponentData->SelectedGear);
	}

	// if we're not a preselector gear shift mechanism, set gear now
	if (EngineComponentData->PreselectorGearShift == 0)
	{
		if (EngineComponentData->UseGear != EngineComponentData->SelectedGear)
		{
			EngineComponentData->ShiftTime = LastVehicleUpdateTime + SHIFT_TIME;
			EngineComponentData->StartGearX = EngineComponentData->CurrentGearX;
			EngineComponentData->StartGearY = EngineComponentData->CurrentGearY;
			EngineComponentData->StartLinearGearPos = EngineComponentData->CurrentLinearGearPos;			
		}
		EngineComponentData->UseGear = EngineComponentData->SelectedGear;
	}
	else
	{
		// otherwise shift when clutch comes out
		if (EngineComponentData->CanShift == 0 &&
			EngineComponentData->LastCanShift != 0 ||
			FailSafeMode == 1)
		{
			if (EngineComponentData->UseGear != EngineComponentData->SelectedGear)
			{
				EngineComponentData->ShiftTime = LastVehicleUpdateTime + SHIFT_TIME;
				EngineComponentData->StartGearX = EngineComponentData->CurrentGearX;
				EngineComponentData->StartGearY = EngineComponentData->CurrentGearY;
				EngineComponentData->StartLinearGearPos = EngineComponentData->CurrentLinearGearPos;			
			}

			EngineComponentData->UseGear = EngineComponentData->SelectedGear;
		}
	}

	EngineComponentData->LastCanShift = EngineComponentData->CanShift;
	if (LastVehicleUpdateTime > EngineComponentData->ShiftTime)
	{
		EngineComponentData->CurrentGear = EngineComponentData->UseGear;
	}
	
	// do gear shift stuff
	NumCrosses = (EngineComponentData->NumForwardGears + EngineComponentData->NumReverseGears + 1) / 2 - 1;
	// determine where we're going
	if (EngineComponentData->UseGear == 0)
	{
		NewGearXPos = 0.5f;
		NewGearYPos = 0.5f;
	}
	else
	{
		if (EngineComponentData->InReverse)
		{
			if ((EngineComponentData->NumForwardGears & 1) == 0)	// Fast modulo 2
			{
				NewGearYPos = (FLOAT32)((EngineComponentData->UseGear-1 + EngineComponentData->NumForwardGears + 1) & 1);
				NewGearXPos = (FLOAT32)((EngineComponentData->UseGear-1 + EngineComponentData->NumForwardGears + 1) / 2) / NumCrosses;	
			}
			else
			{
				NewGearYPos = (FLOAT32)((EngineComponentData->UseGear-1 + EngineComponentData->NumForwardGears) & 1);
				NewGearXPos = (FLOAT32)((EngineComponentData->UseGear-1 + EngineComponentData->NumForwardGears) / 2) / NumCrosses;	
			}
		}
		else
		{
			NewGearYPos = (FLOAT32)((EngineComponentData->UseGear-1) & 1);	// fast modulo 2
			NewGearXPos = (FLOAT32)((EngineComponentData->UseGear-1) / 2) / (FLOAT32)NumCrosses;
		}
	}

	// now move the gearshift based on how much time has passed
	if (LastVehicleUpdateTime >= EngineComponentData->ShiftTime)
	{
		EngineComponentData->CurrentGearX = NewGearXPos;
		EngineComponentData->CurrentGearY = NewGearYPos;
	}
	else
	{
		DeltaTime = (FLOAT32)(EngineComponentData->ShiftTime - LastVehicleUpdateTime) / (FLOAT32)SHIFT_TIME;

		if (DeltaTime > 1.0)
		{
			EngineComponentData->CurrentGearX = EngineComponentData->StartGearX;
			EngineComponentData->CurrentGearY = EngineComponentData->StartGearY;
		}
		if (DeltaTime <= 1.0 && DeltaTime > 0.75)
		{
			EngineComponentData->CurrentGearX = EngineComponentData->StartGearX;
			EngineComponentData->CurrentGearY = (DeltaTime - 0.75f) / 0.25f * 
							(EngineComponentData->StartGearY - 0.5f) + 0.5f;
		}
		if (DeltaTime <= 0.75 && DeltaTime > 0.25)
		{
			EngineComponentData->CurrentGearY = 0.5f;
			EngineComponentData->CurrentGearX = (DeltaTime - 0.25f) / 0.5f *
							(EngineComponentData->StartGearX - NewGearXPos) + NewGearXPos;
		}
		if (DeltaTime <= 0.25)
		{
			EngineComponentData->CurrentGearX = NewGearXPos;
			EngineComponentData->CurrentGearY = (1.0f - DeltaTime / 0.25f) * 
							(NewGearYPos - 0.5f) + 0.5f;
		}
	}
	// linear shifter
	NumCrosses = EngineComponentData->NumForwardGears + EngineComponentData->NumReverseGears + 1;
	if (EngineComponentData->UseGear == 0)
	{
		NewGearXPos = (FLOAT32)(EngineComponentData->NumReverseGears + 1);
	}
	else
	{
		if (EngineComponentData->InReverse)
		{
			NewGearXPos = (FLOAT32)(EngineComponentData->NumReverseGears - EngineComponentData->UseGear);
		}
		else
		{
			NewGearXPos = (FLOAT32)(EngineComponentData->NumReverseGears + 1 + EngineComponentData->UseGear);
		}
	}
	NewGearXPos /= NumCrosses;
	if (LastVehicleUpdateTime >= EngineComponentData->ShiftTime)
	{
		EngineComponentData->CurrentLinearGearPos = NewGearXPos;
	}
	else
	{
		DeltaTime = (FLOAT32)(EngineComponentData->ShiftTime - LastVehicleUpdateTime) / (FLOAT32)SHIFT_TIME;
		if (DeltaTime > 1.0f)
		{
			EngineComponentData->CurrentLinearGearPos = EngineComponentData->StartLinearGearPos;
		}
		else
		{
			EngineComponentData->CurrentLinearGearPos = DeltaTime *
							(EngineComponentData->StartLinearGearPos - NewGearXPos) + NewGearXPos;
		}
	}

	mlUpdateGearShifter(EngineComponentData->CurrentGearX,
						EngineComponentData->CurrentGearY,
						EngineComponentData->CurrentLinearGearPos);

	// now do rpm stuff
	if (EngineComponentData->CurrentGear == 0)
	{
		if (TorqueReceiverData->TransmissionType == ML_ENGINECOM_TRANSTYPE_EPICYCLIC)
		{
			TorqueReceiverData->CurrentPower *= 3;
			TorqueReceiverData->CurrentPower += EngineComponentData->CurrentPower;
			TorqueReceiverData->CurrentPower /= 4;

			EngineComponentData->CurrentRpm *= 3;
			EngineComponentData->CurrentRpm += TorqueReceiverData->CurrentRpm * 
					EngineComponentData->EpicyclicGearRatio;
			EngineComponentData->CurrentRpm /= 4;
		}
//		else
//		{
			EngineComponentData->InNeutral = 1;
//		}
		TorqueReceiverData->CurrentPower = 0.0;
		TorqueReceiverData->InNeutral = 1;
	}
	else
	{
		EngineComponentData->InNeutral = 0;
		TorqueReceiverData->InNeutral = 0;

		if (EngineComponentData->InReverse == 0)
		{
			EngineComponentData->CurrentRpm *= 3;
			EngineComponentData->CurrentRpm += TorqueReceiverData->CurrentRpm * 
					EngineComponentData->ForwardGearRatio[EngineComponentData->CurrentGear-1];
			EngineComponentData->CurrentRpm /= 4;

			TorqueReceiverData->CurrentPower *= 3;
			TorqueReceiverData->CurrentPower += EngineComponentData->CurrentPower;
			TorqueReceiverData->CurrentPower /= 4;
		}
		else
		{
			EngineComponentData->CurrentRpm *= 3;
			EngineComponentData->CurrentRpm += -TorqueReceiverData->CurrentRpm * 
					EngineComponentData->ReverseGearRatio[EngineComponentData->CurrentGear-1];
			EngineComponentData->CurrentRpm /= 4;

			TorqueReceiverData->CurrentPower *= 3;
			TorqueReceiverData->CurrentPower = -EngineComponentData->CurrentPower;
			TorqueReceiverData->CurrentPower /= 4;
		}
	}

	EngineComponentData->EpicyclicNeutralEngagedPercent = 
				TorqueReceiverData->EpicyclicNeutralEngagedPercent;

	return 0;
}


static UINT32 _UpdateClutch(mlEngineComponentData * EngineComponentData,
							mlComponent * Component,
							FLOAT64 InClutchPos,
							FLOAT64 DeltaTime,
							UINT32 FailSafeMode)
{
mlEngineComponentData * TorqueReceiverData;
mlEngineData * EngineData;
FLOAT64 Torque,MaxTorque,TorqueDelta,Torque1;
FLOAT64 TempNum;
FLOAT64 SumTorque;
UINT32 i;
FLOAT64 ClutchPos;
FLOAT64 RetTorque;
FLOAT64 LastRpm;

	if (EngineComponentData == NULL ||
		EngineComponentData->PropellorComponent == NULL)
	{
		return 1;
	}

	TorqueReceiverData = (mlEngineComponentData *)EngineComponentData->PropellorComponent->SpecialData;

	if (TorqueReceiverData == NULL)
	{
		return 1;
	}

	EngineData = EngineComponentData->EngineData;

	if (EngineData == NULL)
	{
		return 1;
	}

	LastRpm = EngineComponentData->CurrentRpm;

	TorqueReceiverData->EngineData = EngineData;
	TorqueReceiverData->AutoShift = EngineComponentData->AutoShift;

	if (FailSafeMode == 1)
	{
		ClutchPos = InClutchPos;

		if (TorqueReceiverData->UseGear > 1)
		{
			if (EngineComponentData->CurrentRpm < EngineData->IdleRpm * 0.6)
			{
				ClutchPos = 0.0;
			}
			if (EngineComponentData->CurrentRpm > EngineData->IdleRpm * 0.8)
			{
				ClutchPos = 1.0;
			}
			if (EngineComponentData->CurrentRpm >= EngineData->IdleRpm * 0.6 &&
				EngineComponentData->CurrentRpm <= EngineData->IdleRpm * 0.8)
			{
				ClutchPos = 1.0 - (EngineData->IdleRpm * 0.8 - EngineComponentData->CurrentRpm) /
								(EngineData->IdleRpm * 0.8 - EngineData->IdleRpm * 0.6);
			}
		}
		else
		{
			if (EngineComponentData->CurrentRpm < EngineData->IdleRpm * 1.1)
			{
				ClutchPos = 0.0;
			}
			if (EngineComponentData->CurrentRpm > EngineData->IdleRpm * 1.4)
			{
				ClutchPos = 1.0;
			}
			if (EngineComponentData->CurrentRpm >= EngineData->IdleRpm * 1.1 &&
				EngineComponentData->CurrentRpm <= EngineData->IdleRpm * 1.4)
			{
				ClutchPos = 1.0 - (EngineData->IdleRpm * 1.4 - EngineComponentData->CurrentRpm) /
								(EngineData->IdleRpm * 1.4 - EngineData->IdleRpm * 1.1);
			}
		}
	}
	else
	{
		ClutchPos = InClutchPos;

		if (ClutchPos < EngineComponentData->ClutchDisengaged)
		{
			TorqueReceiverData->CanShift = 1;
		}
		else
		{
			TorqueReceiverData->CanShift = 0;
		}
	}

	mlUpdateClutchState((FLOAT32)ClutchPos);

	if (ClutchPos < EngineComponentData->ClutchStart)
	{
		MaxTorque = 0;
	}

	if (ClutchPos > EngineComponentData->ClutchEnd)
	{
		MaxTorque = EngineComponentData->ClutchForce;
	}

	if (ClutchPos >= EngineComponentData->ClutchStart &&
		ClutchPos <= EngineComponentData->ClutchEnd)
	{
		MaxTorque = (ClutchPos - EngineComponentData->ClutchStart) /
			(EngineComponentData->ClutchEnd - EngineComponentData->ClutchStart);

		MaxTorque *= EngineComponentData->ClutchForce;
	}

	if (TorqueReceiverData->InNeutral == 1)
	{
		MaxTorque *= TorqueReceiverData->EpicyclicNeutralEngagedPercent;
	}

	SumTorque = 0.0;
	for (i=0;i<20;i++)
	{

		if (EngineComponentData->CurrentRpm > 50.0)
		{
			Torque = EngineComponentData->CurrentPower / (EngineComponentData->CurrentRpm / 60.0);
		}
		else
		{
			Torque = EngineComponentData->CurrentPower / (50.0 / 60.0);
		}


		if (EngineComponentData->CurrentRpm < TorqueReceiverData->CurrentRpm)
		{
			if (TorqueReceiverData->CurrentRpm - EngineComponentData->CurrentRpm < 100.0)
			{
				TempNum = MaxTorque * 
						(TorqueReceiverData->CurrentRpm - EngineComponentData->CurrentRpm) /
						100.0;
			}
			else
			{
				TempNum = MaxTorque;
			}

			Torque1 = -TempNum;
			RetTorque = TempNum;
		}
		else
		{
			if (EngineComponentData->CurrentRpm - TorqueReceiverData->CurrentRpm < 100.0)
			{
				TempNum = MaxTorque * 
						(EngineComponentData->CurrentRpm - TorqueReceiverData->CurrentRpm) /
						100.0;
			}
			else
			{
				TempNum = MaxTorque;
			}

			Torque1 = TempNum;
			RetTorque = -TempNum;
		}

		// first compute torque to the rest of the drivetrain
		SumTorque += Torque1;

		// now accelerate the flywheel
		TorqueDelta = Torque + RetTorque;
		TempNum = EngineComponentData->LastTorqueDelta;
		EngineComponentData->LastTorqueDelta = TorqueDelta;
		TorqueDelta = (TorqueDelta + TempNum) * 0.5;

		TempNum = TorqueDelta / EngineComponentData->FlyWheelRotInertia * DeltaTime / 1000.0 / 20.0;
		EngineComponentData->CurrentRpm *= 4;
		EngineComponentData->CurrentRpm += (TempNum * 60.0);
		EngineComponentData->CurrentRpm /= 4;

	}

	// transmit power
	TorqueReceiverData->CurrentPower = (SumTorque / 20) * (EngineComponentData->CurrentRpm / 60.0);

	return 0;
}

// generic power/torque generator
// 
// uses the principal that an engine generates power based on the throttle position
// up to the engine's max power
//
// this is "transmitted" to the torque receiver component
//
// the rule of thumb is the next component on the chain controls the RPM,
// while this component generates the power/torque the next component receives
static UINT32 _UpdateTorqueGenerator(mlEngineComponentData * EngineComponentData,
									 mlComponent * Component,
									 FLOAT64 DeltaTime,
									 FLOAT64 ThrottlePos,
									 UINT32 FailSafeMode)
{
mlEngineComponentData * TorqueReceiverData;
mlEngineData * EngineData;
FLOAT64 Power;

	EngineData = EngineComponentData->EngineData;

	if (EngineData == NULL)
	{
		return 1;
	}

	if (EngineComponentData->PropellorComponent == NULL)
	{
		return 1;
	}

	TorqueReceiverData = (mlEngineComponentData *)EngineComponentData->PropellorComponent->SpecialData;

	if (TorqueReceiverData == NULL)
	{
		return 1;
	}

	TorqueReceiverData->EngineData = EngineData;

	// for now, rpm governer enabled whether failsave mode enabled or not
	if (TorqueReceiverData->CurrentRpm > EngineData->WepRpm)
	{
		ThrottlePos *= (EngineData->WepRpm * 1.1 - TorqueReceiverData->CurrentRpm) /
						(EngineData->WepRpm * 1.1 - EngineData->WepRpm);
	}

	if (TorqueReceiverData->CurrentRpm <= EngineData->IdleRpm + 400 &&
		TorqueReceiverData->CurrentRpm > EngineData->IdleRpm)
	{
		ThrottlePos += 0.3 * ((EngineData->IdleRpm + 400) - TorqueReceiverData->CurrentRpm) /
								400.0;
	}

	if (TorqueReceiverData->CurrentRpm <= EngineData->IdleRpm)
	{
		ThrottlePos += 0.3 + 0.1 * (EngineData->IdleRpm - TorqueReceiverData->CurrentRpm) /
								EngineData->IdleRpm;
	}

	if (ThrottlePos > 1.0)
	{
		ThrottlePos = 1.0;
	}

	Power = EngineData->BaseHorsePower * 745.7 * ((ThrottlePos - 0.3)/0.7);

	if (TorqueReceiverData->CurrentRpm < 50.0)
	{
		Power *= 50.0 / EngineData->MilRpm;
	}
	else
	{
		if (TorqueReceiverData->CurrentRpm <= EngineData->MilRpm)
		{
			Power *= TorqueReceiverData->CurrentRpm / EngineData->MilRpm;
		}
	}

	TorqueReceiverData->CurrentPower = Power;

	TorqueReceiverData->AutoShift = 0;

	if (TorqueReceiverData->CurrentRpm > EngineData->MaxSafeRPM * 1.2)
	{
		TorqueReceiverData->AutoShift = 1;
	}

	if (TorqueReceiverData->CurrentRpm < EngineData->IdleRpm * 0.5)
	{
		TorqueReceiverData->AutoShift = -1;
	}

	return 0;
}

static UINT32 _GetManifoldInputPressure(FLOAT64 * ManifoldInDensity,
										FLOAT64 Alt,
										mlEngineComponentData * EngineComponentData,
										mlEngineData * EngineData,
										UINT32 FailSafeMode,
										SINT32 GearChange)
{
FLOAT64 AltDensity,SeaLevelDensity,CritAltDensity1,CritAltDensity2;
FLOAT64 MaxBoost,Boost;
FLOAT64 Ratio;
SINT32 LastBlowerSpeed;

	// handle supercharging stuff
	mlGetAltDensity(&SeaLevelDensity,0);
	if (EngineData->SuperChargerType != ML_ENGINE_NOSUPERCHARGING)
	{
		mlGetAltDensity(&CritAltDensity1,EngineData->Speed1CriticalAlt);
		if (EngineData->SuperChargerType == ML_ENGINE_DOUBLESPEED ||
			EngineData->SuperChargerType == ML_ENGINE_DOUBLESPEEDWITHNEUTRAL)
		{
			mlGetAltDensity(&CritAltDensity2,EngineData->Speed2CriticalAlt);
		}
	}
	mlGetAltDensity(&AltDensity,Alt);

	// handle any shifting of supercharger state
	FailSafeMode = 1;

	LastBlowerSpeed = EngineComponentData->SelectedGear;
	if (FailSafeMode == 0)
	{
		if (GearChange < 0)
		{
			if (EngineComponentData->SelectedGear > 0)
			{
				EngineComponentData->SelectedGear--;
			}
		}
		if (GearChange > 0)
		{
			EngineComponentData->SelectedGear++;
		}
	}
	else
	{
		if (EngineData->SuperChargerType != ML_ENGINE_NOSUPERCHARGING)
		{
			EngineComponentData->SelectedGear = 1;
		}

		if (EngineData->SuperChargerType == ML_ENGINE_SINGLESPEEDWITHNEUTRAL ||
			EngineData->SuperChargerType == ML_ENGINE_DOUBLESPEEDWITHNEUTRAL)
		{
			if (Alt < EngineData->NeutralShiftAlt)
			{
				EngineComponentData->SelectedGear = 0;
			}
		}

		if (EngineData->SuperChargerType == ML_ENGINE_DOUBLESPEED ||
			EngineData->SuperChargerType == ML_ENGINE_DOUBLESPEEDWITHNEUTRAL)
		{
			if (Alt > EngineData->ShiftAlt)
			{
				EngineComponentData->SelectedGear = 2;
			}
		}
	}

	if (EngineData->SuperChargerType == ML_ENGINE_NOSUPERCHARGING)
	{
		EngineComponentData->SelectedGear = 0;
	}

	if (EngineData->SuperChargerType == ML_ENGINE_SINGLESPEED ||
		EngineData->SuperChargerType == ML_ENGINE_DOUBLESPEED)
	{
		if (EngineComponentData->SelectedGear < 1)
		{
			EngineComponentData->SelectedGear = 1;
		}
	}

	if (EngineData->SuperChargerType == ML_ENGINE_SINGLESPEED ||
		EngineData->SuperChargerType == ML_ENGINE_SINGLESPEEDWITHNEUTRAL ||
		EngineData->SuperChargerType == ML_ENGINE_TURBO)
	{
		if (EngineComponentData->SelectedGear > 1)
		{
			EngineComponentData->SelectedGear = 1;
		}
	}

	if (EngineData->SuperChargerType == ML_ENGINE_DOUBLESPEED ||
		EngineData->SuperChargerType == ML_ENGINE_DOUBLESPEEDWITHNEUTRAL)
	{
		if (EngineComponentData->SelectedGear > 2)
		{
			EngineComponentData->SelectedGear = 2;
		}
	}

	if (EngineComponentData->SelectedGear != LastBlowerSpeed)
	{
		if (EngineComponentData->SelectedGear == 0)
		{
			rnmainListText(loclGetLocalizationStringByID(loclSHIFTED_INTO_NEUTRAL_BLOWER_MSG));
		}
		if (EngineComponentData->SelectedGear == 1)
		{
			rnmainListText(loclGetLocalizationStringByID(loclSUPERCHARGER_PRIMARY_SPEED_SETTING_MSG));
		}
		if (EngineComponentData->SelectedGear == 2)
		{
			rnmainListText(loclGetLocalizationStringByID(loclSUPERCHARGER_SECOND_SPEED_SETTING_MSG));
		}
	}

	if (EngineComponentData->SelectedGear == 0)
	{
		MaxBoost = EngineData->NeutralMaxMilBoost * SeaLevelDensity;
		Boost = MaxBoost * AltDensity / SeaLevelDensity;
	}
	if (EngineComponentData->SelectedGear == 1)
	{
		MaxBoost = EngineData->Speed1MaxMilBoost * SeaLevelDensity;
		Boost = MaxBoost * AltDensity / CritAltDensity1;
	}
	if (EngineComponentData->SelectedGear == 2)
	{
		MaxBoost = EngineData->Speed2MaxMilBoost * SeaLevelDensity;
		Boost = MaxBoost * AltDensity / CritAltDensity2;
	}

	if (EngineComponentData->EngineMode == ML_ENGINE_MODE_WEP)
	{
		if (EngineComponentData->SelectedGear == 0)
		{
			MaxBoost *= EngineData->NeutralMaxWepBoost / EngineData->NeutralMaxMilBoost;
		}
		if (EngineComponentData->SelectedGear == 1)
		{
			MaxBoost *= EngineData->Speed1MaxWepBoost / EngineData->Speed1MaxMilBoost;
		}
		if (EngineComponentData->SelectedGear == 2)
		{
			MaxBoost *= EngineData->Speed2MaxWepBoost / EngineData->Speed2MaxMilBoost;
		}
	}


	if (EngineComponentData->EngineMode == ML_ENGINE_MODE_INJWEP)
	{
		if (EngineComponentData->SelectedGear == 0)
			MaxBoost *= EngineData->MaxInjWepBoost / EngineData->NeutralMaxMilBoost;

		if (EngineComponentData->SelectedGear == 1)
			MaxBoost *= EngineData->MaxInjWepBoost / EngineData->Speed1MaxMilBoost;

		if (EngineComponentData->SelectedGear == 2)
			MaxBoost *= EngineData->MaxInjWepBoost / EngineData->Speed2MaxMilBoost;
	}

	if (EngineComponentData->EngineMode == ML_ENGINE_MODE_CLIMB)
	{
		if (EngineComponentData->SelectedGear == 0)
			MaxBoost *= EngineData->Speed1MaxClimbBoost / EngineData->NeutralMaxMilBoost;

		if (EngineComponentData->SelectedGear == 1)
			MaxBoost *= EngineData->Speed1MaxClimbBoost / EngineData->Speed1MaxMilBoost;

		if (EngineComponentData->SelectedGear == 2)
			MaxBoost *= EngineData->Speed1MaxClimbBoost / EngineData->Speed2MaxMilBoost;	
	}
	
	Ratio = EngineComponentData->CurrentRpm / EngineData->MilRpm;
	Boost = Ratio * Ratio * (Boost - AltDensity) + AltDensity;

	if (Boost > MaxBoost)
	{
		Boost = MaxBoost;
	}

	if (Boost < AltDensity)
	{
		Boost = AltDensity;
	}

	*ManifoldInDensity = Boost;

	return 0;
}


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
							 void * VehicleData,
							 FLOAT32 VehicleG)
{
	mlEngineData * EngineData;
	mlEngineComponentData * TorqueReceiverData;
    LPVEHICLE lpVeh = NULL;
	FLOAT64 AltDensity;
	FLOAT64 ManifoldInDensity;
	FLOAT64 FullThrottleCapacity;
	FLOAT64 SuctionCapacity;
	FLOAT64 Capacity;
	FLOAT64 AirConsumed;
	FLOAT64 CylinderAirDensity;
	FLOAT64 CompressionPower;
	FLOAT64 EngineVolume;
	FLOAT64 TempNum,TempNum1;
	FLOAT64 EnginePower;
	FLOAT64 SeaLevelDensity;
	FLOAT64 EngineExhaustTempFactorPower,EngineExhaustTempFactorCompression;
	FLOAT64 ExhaustTemp;
	FLOAT64 OutsideTemp,BaseHpAltTemp;
	UINT32 StarterOn;
	FLOAT64 StarterPower;
	FLOAT64 SpeedEfficiency,Top,Bottom;
	FLOAT64 SuperchargerPowerLoss;
	FLOAT64 GeneratedPower;
	FLOAT64 IdleThrottlePos;
	mlEngineComponentData * TankData;
	UINT32 i,j;
	UINT32 Found;
	FLOAT64 FuelConsumed;
	FLOAT64 HorsepowerLoss;
	FLOAT64 Percent;
	UINT32 DamageLevel;
	FLOAT64 DamagePerHit;
	FLOAT64 Ratio;
	FLOAT32 ItemHit;
	UINT32 NumHits;
	UINT32 AirCooled;
	UINT32 Count;
	FLOAT64 Heat,Cooling;
	UCHAR s[128];

	EngineData = EngineComponentData->EngineData;
	if (EngineData == NULL || EngineComponentData->PropellorComponent == NULL) return 1;

	TorqueReceiverData = (mlEngineComponentData *)EngineComponentData->PropellorComponent->SpecialData;
	if (TorqueReceiverData == NULL) return 1;
	

	NumHits = 0;
	for(i = 0; i < 4; i++)
		NumHits += Component->NewDamageNumHits[i];
	
	DamagePerHit = (NumHits > 0) ? (Component->CurrentDamage - Component->PrevDamage) / NumHits : 0;
		
	Component->PrevDamage = Component->CurrentDamage;

	// check for random oil leak/glycol leak/fuel leak+fire
	for(i = 0; i < NumHits; i++)
	{
// change percentile
		if (mathGetFloat32ChaoticNumber() < (sqrt(DamagePerHit) / 447.0))
		{
			ItemHit = mathGetFloat32ChaoticNumber();
			if (ItemHit < 0.1f)
			{
				// fuel line hit
				EngineComponentData->TankIsLeaking = 1;
				EngineComponentData->TankOnFire = 1;
// start tank leaking a bit
			}
			else if (ItemHit < 0.3f)
			{
				// oil leak
				if (EngineComponentData->OilTankComponent != NULL)
				{
					if (EngineComponentData->OilTankComponent->SpecialData != NULL)
					{
						TankData = (mlEngineComponentData *)
								EngineComponentData->OilTankComponent->SpecialData;

						if (EngineComponentData->OilTankComponent->CurrentDamage <=
								EngineComponentData->OilTankComponent->DamageLevels[0])
						{
							EngineComponentData->OilTankComponent->CurrentDamage =
									EngineComponentData->OilTankComponent->DamageLevels[0] + 1.0f;
						}
						TankData->TankIsLeaking = 1;
					}
				}
			}
			else if (ItemHit < 0.6f)
			{
				// radiator damage
				for (j=0;j<ML_MAX_RADIATORS_PER_ENGINE;j++)
				{
					if (EngineComponentData->RadiatorComponent[j] != NULL)
					{
						if (EngineComponentData->RadiatorComponent[j]->SpecialData != NULL)
						{
							TankData = (mlEngineComponentData *)
									EngineComponentData->RadiatorComponent[j]->SpecialData;

							if (EngineComponentData->RadiatorComponent[j]->CurrentDamage <
									EngineComponentData->RadiatorComponent[j]->DamageLevels[0])
							{
								EngineComponentData->RadiatorComponent[j]->CurrentDamage =
										EngineComponentData->RadiatorComponent[j]->DamageLevels[0] + 1.0f;
							}
							TankData->TankIsLeaking = 1;
						}
					}

				}
			}
		}
	}

	DamageLevel = 0;
	for(i = 0; i < 4; i++)
	{
		if (Component->CurrentDamage > Component->DamageLevels[i] && Component->DamageLevels[i] > 0.001)
			DamageLevel = i+1;
	}
	
	// damage screen shake
	if(DamageLevel >= 1 && ((vehVehicle*)VehicleData)->VehicleType->Type == VEH_TYPE_AIR) // aircraft only for now
	{
		if(EngineComponentData->CurrentRpm >= .1 && EngineComponentData->EngineCaught)
		{
			FLOAT32 shake = (FLOAT32)(0.01 * (Component->CurrentDamage - Component->DamageLevels[0]) / (Component->DamageLevels[3] - Component->DamageLevels[0]));
	     	if(shake < 0.0)			shake = 0.0f;
			else if(shake > 0.01)	shake = 0.01f;
			persShakeCamera(shake);
		}
	}
	
	HorsepowerLoss = 0.0;
	if (DamageLevel == 4)
	{
		HorsepowerLoss = EngineData->BaseHorsePower * 10.0 * TorqueReceiverData->CurrentRpm /
						EngineData->BaseHpRpm;
		EngineComponentData->EngineCaught = 0;
		if (EngineComponentData->DamagedEngineRotFlag == 0)
		{
			TorqueReceiverData->FlyWheelRotInertia *= 10.0;
			EngineComponentData->DamagedEngineRotFlag = 1;
		}
	}
	if (DamageLevel == 3)
	{
		HorsepowerLoss = EngineData->BaseHorsePower * 1.5 * TorqueReceiverData->CurrentRpm /
						EngineData->BaseHpRpm;
	}
	if (DamageLevel == 2)
	{
		Percent = Component->DamageLevels[2] - Component->CurrentDamage - Component->DamageLevels[1];
		Percent /= Component->DamageLevels[2] - Component->DamageLevels[1];
		Percent = 1.0 - Percent;
		HorsepowerLoss = Percent * 0.2 * EngineData->BaseHorsePower * TorqueReceiverData->CurrentRpm /
						EngineData->BaseHpRpm;
	}
	if (DamageLevel == 1)
	{
		Percent = Component->DamageLevels[1] - Component->CurrentDamage - Component->DamageLevels[0];
		Percent /= Component->DamageLevels[1] - Component->DamageLevels[0];
		Percent = 1.0 - Percent;
		HorsepowerLoss = Percent * 0.05 * EngineData->BaseHorsePower * TorqueReceiverData->CurrentRpm /
						EngineData->BaseHpRpm;
	}

	TorqueReceiverData->EngineData = EngineData;

	// toggle wep if applicable
	if((ToggleWepMode == 1) && (EngineComponentData->LastWepToggle == 0))
	{
		if(	EngineComponentData->WepTankComponent && 
			((mlEngineComponentData*)EngineComponentData->WepTankComponent->SpecialData)->CurrentTankContents > 0 )
		{
			EngineComponentData->WepStage++;
			if(EngineComponentData->WepStage > 3) EngineComponentData->WepStage = 0;
		}else
		{
			EngineComponentData->WepStage = !EngineComponentData->WepStage;
		}
		
		if(EngineComponentData->WepStage == 0 || !EngineComponentData->EngineCaught)
		{
			EngineComponentData->EngineMode = ML_ENGINE_MODE_MIL;
			rnmainListText(loclGetLocalizationStringByID(847));
		}else if(EngineComponentData->WepStage == 1) 
		{
				EngineComponentData->EngineMode = ML_ENGINE_MODE_WEP;
			rnmainListText(loclGetLocalizationStringByID(846));
		}else if(EngineComponentData->WepStage == 2)
		{
			EngineComponentData->EngineMode = ML_ENGINE_MODE_INJWEP;
			rnmainListText("Injected Wep On");
		}else if(EngineComponentData->WepStage == 3)
		{
			EngineComponentData->EngineMode = ML_ENGINE_MODE_WEP;	
			rnmainListText("Injected WEP Off, WEP On");
		}
	}
	EngineComponentData->LastWepToggle = ToggleWepMode;

	// failsafe mode has an rpm govener to reduce overspeed problems
	if (TorqueReceiverData->CurrentRpm > EngineData->WepRpm + 15.0 && FailSafeMode == 1)
	{
		ThrottlePos *= (EngineData->MaxSafeRPM * 1.1 - TorqueReceiverData->CurrentRpm) /
						(EngineData->MaxSafeRPM * 1.1 - (EngineData->MaxSafeRPM + 15.0));
	}

    lpVeh = (vehVehicle *)VehicleData;

#if !USE_OSG
   // HACK! Turn off first person smoke stack for ships if engine is shut off.
    if(lpVeh->VehicleType->Type == VEH_TYPE_SEA)
    {
        if(!EngineComponentData->EngineCaught)
        {
            if(EngineComponentData->EngineLastCaught)
            {
                vehfxStopSmokeStack(lpVeh->iFXVehicleID);
            }
        }
    }
#endif
	// do engine starting stuff;
	EngineComponentData->EngineLastCaught = EngineComponentData->EngineCaught;
	mlGetStarterOnInfo(((vehVehicle *)VehicleData)->ModelData, &StarterOn,EngineComponentData->EngineNumber);
	StarterPower = 0.0;
	if (StarterOn && DamageLevel < 4)
	{
		if (TorqueReceiverData->CurrentRpm < EngineData->StarterRpm)
		{
			StarterPower = EngineData->StarterHp * 745.7;
			StarterPower *= (EngineData->StarterRpm - TorqueReceiverData->CurrentRpm) / 
									EngineData->StarterRpm;
		}

		// check for engine start
		if (EngineComponentData->EngineCaught == 0 && 
			TorqueReceiverData->CurrentRpm < EngineData->CriticalRpm)
		{
			TempNum = DeltaTime / 1000.0;
			TempNum *= TorqueReceiverData->CurrentRpm / EngineData->CriticalRpm;
			if (EngineComponentData->EngineCaughtOnce == 1)
			{
				TempNum *= 5.0;
			}
	
			TempNum1 = (FLOAT64)rand() / (FLOAT64)RAND_MAX;
			if (TempNum1 > 1.0 - TempNum)
			{
				EngineComponentData->EngineCaught = 1;
				EngineComponentData->EngineCaughtOnce = 1;
			}
		}
	}

	// check for engine stall
	if (EngineComponentData->EngineCaught == 1 && TorqueReceiverData->CurrentRpm < EngineData->CriticalRpm)
	{
		TempNum = DeltaTime / 1000.0;
		TempNum *= TorqueReceiverData->CurrentRpm / EngineData->StarterRpm;
	
		TempNum1 = (FLOAT64)rand() / (FLOAT64)RAND_MAX;
		if (TempNum1 > 1.0 - TempNum)
			EngineComponentData->EngineCaught = 0;
	}

	// check for engine cutoff
	if (EngineComponentData->LastStarterState == 0 && StarterOn == 1)
	{
		if (EngineComponentData->EngineCaught)
		{
			EngineComponentData->EngineCaught = 0;
		} else
		{
			if (TorqueReceiverData->CurrentRpm >= EngineData->CriticalRpm)
				EngineComponentData->EngineCaught = 1;
		}
	}

	EngineComponentData->LastStarterState = StarterOn;

	mlGetAltDensity(&AltDensity,Alt);
	mlGetAltDensity(&SeaLevelDensity,0);
	EngineComponentData->CurrentRpm = TorqueReceiverData->CurrentRpm;

	_GetManifoldInputPressure(&ManifoldInDensity,Alt,EngineComponentData,EngineData,FailSafeMode,GearChange);

	// compute ratio of engine temperature during cylinder firing vs normal atmospheric pressure
	ExhaustTemp = EngineData->BaseHpExhaustTemp;

	if (ExhaustTemp < 0.1)
		ExhaustTemp = 1100.0; // default of 1100 degrees kelvin, about 800 degrees C

	mlGetAltTemp(&OutsideTemp,Alt);
	mlGetAltTemp(&BaseHpAltTemp,EngineData->BaseHorsePowerAlt);
	EngineExhaustTempFactorPower = OutsideTemp / ExhaustTemp; // used to calc power generated
	EngineExhaustTempFactorCompression = BaseHpAltTemp / ExhaustTemp; // used to calc compression effects

	// precalculate effective engine volume
	TempNum = EngineData->BaseHorsePower;
	TempNum *= 745.7;
	TempNum /= (EngineData->BaseHpManifoldPress * SeaLevelDensity);
	EngineVolume = TempNum / EngineData->BaseHpRpm * 60.0;

	// fudge factor because the air doing this work is at a much higher temp than
	// the outside air, thus isn't directly translatable to volume
//	EngineVolume *= EngineExhaustTempFactorCompression;
	
	// this is the assumed maximum airflow capacity at full throttle
	// this value controls throttle position effects as well as overspeed compression
	FullThrottleCapacity = EngineData->BaseHpManifoldPress * SeaLevelDensity * EngineData->BaseHpRpm / 60.0;
	

	IdleThrottlePos = EngineData->IdleThrottlePos;
	if (TorqueReceiverData->CurrentRpm < EngineData->IdleRpm)
	{
		IdleThrottlePos = (TorqueReceiverData->CurrentRpm - EngineData->IdleRpm) /
								 (EngineData->CriticalRpm - EngineData->IdleRpm);
		if (IdleThrottlePos >= 0.0)
			IdleThrottlePos = sqrt(IdleThrottlePos);
		else
			IdleThrottlePos = 0.0;
		
		IdleThrottlePos = (EngineData->IdleThrottlePos + IdleThrottlePos) / 
						  (1.0 + IdleThrottlePos);
	}

	IdleThrottlePos = (IdleThrottlePos + 5.0 * EngineComponentData->LastIdleThrottlePos) / 6.0;
	EngineComponentData->LastIdleThrottlePos = IdleThrottlePos;

	// this is the current throttle position max airflow
	Capacity = FullThrottleCapacity * ((1.0 - IdleThrottlePos) * ThrottlePos + IdleThrottlePos)
				* (ManifoldInDensity / (SeaLevelDensity * EngineData->BaseHpManifoldPress));

	// this is the maximum air the engine can take
	if (TorqueReceiverData->CurrentRpm > EngineData->WepRpm)
		SuctionCapacity = ManifoldInDensity * EngineData->WepRpm / 60.0;
	else
		SuctionCapacity = ManifoldInDensity * TorqueReceiverData->CurrentRpm / 60.0;

	// if the throttle max airflow is less than the maximum the engine can take
	// restrict airflow accordingly
	if (SuctionCapacity > Capacity)
		AirConsumed = Capacity;
	else
		AirConsumed = SuctionCapacity;

	// now determine compression effects, if any
	if (TorqueReceiverData->CurrentRpm < 50.0)
		CylinderAirDensity = AirConsumed / 50.0 * 60.0;
	else
		CylinderAirDensity = AirConsumed / TorqueReceiverData->CurrentRpm * 60.0;

	if (CylinderAirDensity <= AltDensity)
	{
		TempNum = AltDensity - CylinderAirDensity;
		CompressionPower = EngineVolume * TempNum * TorqueReceiverData->CurrentRpm / 60.0;
	}
	else
	{
		CompressionPower = 0.0;
	}

	EngineComponentData->CurrentManifoldPress = CylinderAirDensity / SeaLevelDensity;

	// compute power loss due to supercharger needs
	if (EngineComponentData->SelectedGear != 0)
	{
		SpeedEfficiency = 0;
		if (EngineComponentData->SelectedGear == 1)
		{
			if (EngineData->Speed1EfficiencyBottom > 0.0)
			{
				Top = EngineData->Speed1CriticalAlt;
				Bottom = EngineData->SuperChargingAltFloor;
				mlInterpolate(&SpeedEfficiency,
							  EngineData->Speed1EfficiencyBottom,
							  EngineData->Speed1EfficiencyTop,
							  (Alt - Bottom) / (Top - Bottom));
			}
			else
			{
				SpeedEfficiency = EngineData->Speed1EfficiencyTop;
			}
		}

		if (EngineComponentData->SelectedGear == 2)
		{
			if (EngineData->Speed2EfficiencyBottom > 0.0)
			{
				Top = EngineData->Speed2CriticalAlt;
				Bottom = EngineData->ShiftAlt;
				mlInterpolate(&SpeedEfficiency,
							  EngineData->Speed2EfficiencyBottom,
							  EngineData->Speed2EfficiencyTop,
							  (Alt - Bottom) / (Top - Bottom));
			}
			else
			{
				SpeedEfficiency = EngineData->Speed1EfficiencyTop;
			}
		}

		if (SpeedEfficiency > 1.0)
			SpeedEfficiency = 1.0;
			
		if (SpeedEfficiency < 0.0)
			SpeedEfficiency = 0.0;

		TempNum = 1.0 - SpeedEfficiency;
		SuperchargerPowerLoss = TempNum * EngineData->BaseHorsePower * 745.7;
		SuperchargerPowerLoss *= EngineComponentData->CurrentRpm / EngineData->BaseHpRpm;
	}
	else
	{
		SuperchargerPowerLoss = 0.0;
	}

	// now compute engine power generated
	EnginePower = 0.0;
	if (EngineComponentData->EngineCaught == 1)
	{
		if (EngineData->Neg_G_Fuel_Cutout == 0 ||
			VehicleG > -0.2f)
		{
			EnginePower = CylinderAirDensity * EngineVolume * TorqueReceiverData->CurrentRpm / 60.0;
		}
	}
	if (!StarterOn)
	{
		if (CompressionPower < EngineData->BaseHorsePower * 0.01 * 745.7 &&
			EnginePower < EngineData->BaseHorsePower * 0.01 * 745.7)
		{
			CompressionPower = EngineData->BaseHorsePower * 0.01 * 745.7;
			if (TorqueReceiverData->CurrentRpm < EngineData->CriticalRpm)
			{
				CompressionPower *= TorqueReceiverData->CurrentRpm / EngineData->CriticalRpm;
			}

			if (TorqueReceiverData->CurrentRpm < 0.0 && DamageLevel < 3)
			{
				CompressionPower = EngineData->BaseHorsePower * 5 * 745.7 * TorqueReceiverData->CurrentRpm /
									EngineData->CriticalRpm;
			}
		}
	}

	GeneratedPower = EnginePower;

	EnginePower -= CompressionPower;
	EnginePower -= SuperchargerPowerLoss;
	EnginePower += StarterPower;
	if (TorqueReceiverData->CurrentRpm > 10.0)
		EnginePower -= HorsepowerLoss * 745.7;

	TorqueReceiverData->CurrentPower = EnginePower;

	// do autoshift stuff
	TorqueReceiverData->AutoShift = 0;

	if (TorqueReceiverData->CurrentRpm > EngineData->MaxSafeRPM * 1.2)
		TorqueReceiverData->AutoShift = 1;

	if (TorqueReceiverData->CurrentRpm < EngineData->IdleRpm * 0.5)
		TorqueReceiverData->AutoShift = -1;

	// gauges
	TempNum = EngineComponentData->CurrentManifoldPress;
	// now convert to atmospheres
	// if undefined we assume it's already atmospheres
	if (EngineData->ManifoldPressureUnits == ML_ENGINE_MANIFOLD_CMHGBOOST)
		TempNum /= 76;
	else if (EngineData->ManifoldPressureUnits == ML_ENGINE_MANIFOLD_INHG)
		TempNum /= 29.92;
	else if (EngineData->ManifoldPressureUnits == ML_ENGINE_MANIFOLD_LBSBOOST)
		TempNum /= 14.7;

	TempNum1 = EngineComponentData->EngineTemp;

	mlUpdateEngineGauges(EngineComponentData->EngineNumber - 1, // in the .com file first eng is 1
						 EngineComponentData->CurrentRpm,
						 TempNum,
						 EngineComponentData->EngineMode,
						 StarterOn,
						 TempNum1);


	// first consume WEP if applicable
	if(EngineComponentData->EngineMode == ML_ENGINE_MODE_INJWEP && EngineComponentData->EngineCaught)
	{
		if(!EngineComponentData->WepTankComponent)
		{
			EngineComponentData->EngineMode = ML_ENGINE_MODE_WEP;
			EngineComponentData->WepStage = 1;
		}
		else
		{
			TankData = (mlEngineComponentData*)EngineComponentData->WepTankComponent->SpecialData;
			TankData->CurrentTankContents -= EngineData->WepConsumptionRate * DeltaTime / 1000.0 *
						EngineComponentData->CurrentRpm / EngineData->BaseHpRpm;
			if (TankData->CurrentTankContents <= 0.0)
			{
				TankData->CurrentTankContents = 0.0;
				EngineComponentData->EngineMode = ML_ENGINE_MODE_WEP;
				EngineComponentData->WepStage = 1;
			}
		}
	}

	// check for lack of oil
	if(EngineComponentData->OilTankComponent && EngineComponentData->EngineCaught)
	{
		TankData = (mlEngineComponentData *)EngineComponentData->OilTankComponent->SpecialData;
		if(TankData)
		{
			// now based on oil in engine, if below 30%, start taking proportionally more damage
#ifdef BETA_RELEASE
if(!VehicleObjectAnalysis){
#endif
			if(EngineComponentData->OilInEngine < .3)
				Component->CurrentDamage += 5000.0 * DeltaTime / 1000.0 * (.3 - EngineComponentData->OilInEngine) / .3;

#ifdef BETA_RELEASE
}
#endif
			// update oil pressure gauge
			mlUpdateOilTankPress(EngineComponentData->EngineNumber - 1, EngineComponentData->OilInEngine);
		}
	}


	// now consume fuel
	if(EngineComponentData->EngineCaught)
	{
		if(EngineData->Neg_G_Fuel_Cutout == 0 || VehicleG > -0.2f)
		{
		
			Found = 0;
			// find a fuel tank to drain
			for (i=0;i<ML_MAX_FUEL_TANKS_PER_ENGINE;i++)
			{
				if (EngineComponentData->FuelTankComponent[i] != NULL)
				{
					TankData = (mlEngineComponentData *)EngineComponentData->FuelTankComponent[i]->SpecialData;
					if (TankData->CurrentTankContents > 0.0)
					{
						Found = 1;
						break;
					}
				}
			}

			if (Found == 0)
			{
				EngineComponentData->EngineMode = EngineComponentData->EngineCaught = 0;
			}
			else
			{
				FuelConsumed = EngineData->FuelPerHpSec;// * _FuelRateModifier((vehVehicle*)VehicleData);

				// fuel line leak
				if (EngineComponentData->TankIsLeaking == 1)
				{
					FuelConsumed += 0.1f;
				}

				FuelConsumed *= GeneratedPower / 745.7;
				FuelConsumed = FuelConsumed * DeltaTime / 1000.0;

				if (EngineComponentData->TankIsLeaking != 0)
				{
					Component->CurrentDamage += 40000000 * FuelConsumed * 0.001;
					FuelConsumed *= 10.0;
				}

				if (FuelConsumed < 0.0)
				{
					FuelConsumed = 0.0;
				}

				TankData->CurrentTankContents -= FuelConsumed;
				if (TankData->CurrentTankContents < 0.0)
				{
					TankData->CurrentTankContents = 0.0;
				}

			}
		}
	}

	// handle engine cooling/heat
	TempNum = EngineData->BaseHorsePower * 745.7;
	TempNum = GeneratedPower / TempNum;
	
	// determine if we're a liquid-cooled engine
	AirCooled = 1;
	for (i=0;i<ML_MAX_RADIATORS_PER_ENGINE;i++)
	{
		if (EngineComponentData->RadiatorComponent[i] != NULL)
		{
			AirCooled = 0;
		}
	}
	if (AirCooled == 0)
	{
		Count = 0;
		TempNum = 0.0;
		for (i=0;i<ML_MAX_RADIATORS_PER_ENGINE;i++)
		{
			if (EngineComponentData->RadiatorComponent[i] != NULL)
			{
				TankData = (mlEngineComponentData *)EngineComponentData->RadiatorComponent[i]->SpecialData;
				if (TankData->TankOriginalMass > 0.01)
				{
					TempNum += TankData->CurrentTankContents / TankData->TankOriginalMass;
					Count++;
				}
			}
		}
		if (Count == 0)
			Ratio = 1.0;
		else
			Ratio = TempNum / Count;
	}
	else
	{
		Ratio = 1.0;
	}

	// heating
	Heat = GeneratedPower;

	// engine fire equivelant to about 2000hp of engine heat
	// should fry the engine unless it's got a really good cooling system
	if (EngineComponentData->TankIsLeaking)
		Heat += 1500000.0f;

	// add heat caused by engine damage
/*	if(Component->DamageLevels[1] > 0 && Component->CurrentDamage > Component->DamageLevels[1])
	{
	}
*/	
	// cooling
	Cooling = EngineComponentData->EngineTemp * EngineData->BaseHorsePower * 745.7 * Ratio;

	// modify cooling by airspeed
	TempNum1 = mathExactVectorLength(ComponentVel);
	if (EngineData->MaxPowerCoolSpeed > 0.01)
	{
		if (TempNum1 > EngineData->MaxPowerCoolSpeed)
			Cooling = Cooling * TempNum1 / (EngineData->MaxPowerCoolSpeed);
		else
			Cooling = Cooling * (0.5 + 0.5 * TempNum1 / EngineData->MaxPowerCoolSpeed);
	}

	Heat -= Cooling;
	Heat /= EngineData->BaseHorsePower * 745.7;

	// adjust engine temp
	EngineComponentData->EngineTemp += Heat * DeltaTime / 1000.0 / 300.0;
	if (EngineComponentData->EngineTemp < 0.0)
		EngineComponentData->EngineTemp = 0.0;

	// damage engine if too hot
	if (EngineComponentData->EngineTemp > 1.0)
		Component->CurrentDamage += (EngineComponentData->EngineTemp - 1.0) * 20000.0 * DeltaTime / 1000.0;
	
	// if on fire degrade engine
	if (EngineComponentData->TankIsLeaking == 1)
		Component->CurrentDamage += 5000.0 * DeltaTime / 1000.0f;

	// now consume oil
	if(EngineComponentData->OilTankComponent)
	{
		TankData = (mlEngineComponentData*)EngineComponentData->OilTankComponent->SpecialData;
		if(TankData)
		{
			TankData->CurrentTankContents -= EngineData->OilConsumptionPerSec * DeltaTime / 1000.0
											* TorqueReceiverData->CurrentRpm / EngineData->BaseHpRpm;		
			
			// let's say it can transfer everything in 15 seconds at 1 G
			EngineComponentData->OilInEngine += 1.0 / 15.0 * DeltaTime / 1000.0 * VehicleG;

			// clamp appropriately
			if(EngineComponentData->OilInEngine > 1.0)
				 EngineComponentData->OilInEngine = 1.0;
			else if(EngineComponentData->OilInEngine < 0.0)
				 EngineComponentData->OilInEngine = 0.0;
			
			// no oil in tank, no oil in engine
			if(TankData->CurrentTankContents <= 0.01)
				EngineComponentData->OilInEngine = 0.0;
		}
	}

	// exhaust thrust
	if (EngineData->BaseHorsePower != 0 && _DisableEngineThrust == 0)
	{
		Force->y = -GeneratedPower / 745.7 / EngineData->BaseHorsePower * EngineData->ExhaustThrust *
						cos(EngineData->ExhaustThrustAngle);
	}

	// engine torque
	// torque
	Torque->Roll = EnginePower - StarterPower;

	if (fabs(TorqueReceiverData->CurrentRpm) > 0.0001)
		Torque->Roll /= (2 * PI * TorqueReceiverData->CurrentRpm / 60.0);
	else
		Torque->Roll = 0;

	if (EngineData->PropSpinDirection < 0)
		Torque->Roll = -Torque->Roll;

	// some engines are attached to dual counterrotating props, thus no torque
	// very late model spits did this primarily to eliminate the griffon engine's
	// torque problems when taxiing and taking off
	if (EngineData->PropSpinDirection == 0)
		Torque->Roll = 0;

	if (EngineComponentData->EngineCaught == 0)
	{
		EngineComponentData->EngineMode = ML_ENGINE_MODE_MIL;
		mlAutopilotDisengage(((vehVehicle*)VehicleData)->ModelData);
	}
	_HandleEngineSounds(Component, EngineComponentData, (vehVehicle*)VehicleData, VehicleObjectData, VehicleG, StarterOn);

	// engine smoke effect
	if(IsFirstTic)
	{
		sprintf((char *)s,"enginefire%d",EngineComponentData->EngineNumber);
		if (DamageLevel < 3 && EngineComponentData->TankOnFire == 0)
			vehAnimChangeVehicleStateByName((vehVehicle *)VehicleData, s, FALSE);
		else
			vehAnimChangeVehicleStateByName((vehVehicle *)VehicleData, s, TRUE);

		// ghost engine sound
		sprintf((char *)s,"enginesound%d",EngineComponentData->EngineNumber);
		if (EngineComponentData->EngineCaught == 1)
			vehAnimChangeVehicleStateByName((vehVehicle *)VehicleData, s, TRUE);
		else
			vehAnimChangeVehicleStateByName((vehVehicle *)VehicleData, s, FALSE);
		
		// exhaust
		sprintf((char *)s,"exhaust%d",EngineComponentData->EngineNumber);
		if (((vehVehicle*)VehicleData)->ModelData->StartersOn && TorqueReceiverData->CurrentRpm < 0.9 * EngineData->IdleRpm)
			vehAnimChangeVehicleStateByName((vehVehicle*)VehicleData, s, TRUE);
		else
			vehAnimChangeVehicleStateByName((vehVehicle*)VehicleData, s, FALSE);
	}

	// engine sound reduction stuff
	TempNum = TorqueReceiverData->CurrentRpm / EngineData->BaseHpRpm;
	if (TempNum > 1.0)
		TempNum = 1.0;
	if (TempNum < 0.0)
		TempNum = 0.0;
	
	if (VehicleData != NULL)
		((vehVehicle *)VehicleData)->EngineIdleVolume = (FLOAT32)TempNum;

	if (EngineComponentData->EngineCaught)
	{
		TempNum = ManifoldInDensity / (SeaLevelDensity * EngineData->BaseHpManifoldPress);
		TempNum *= (1.0 - IdleThrottlePos) * ThrottlePos + IdleThrottlePos;
		if (TempNum > 1.0)
			TempNum = 1.0;
		if (TempNum < 0.0)
			TempNum = 0.0;

		if (VehicleData != NULL)
			((vehVehicle *)VehicleData)->EnginePowerVolume = (FLOAT32)TempNum;
	}

	EngineComponentData->LastRpm = TorqueReceiverData->CurrentRpm;
	
	return 0;
}

static void _EngineSoundStopped(void *data)
{
	mlEngineComponentData* EngineComponentData = (mlEngineComponentData*)data;
	
	if(EngineComponentData)
		EngineComponentData->EngineSoundLoaded = 0;
}

static void _IdleSoundStopped(void *data)
{
	mlEngineComponentData* EngineComponentData = (mlEngineComponentData*)data;

	if(EngineComponentData)
		EngineComponentData->IdleSoundLoaded = 0;
}

static void _StarterSoundStopped(void *data)
{
	mlEngineComponentData* EngineComponentData = (mlEngineComponentData*)data;
	
	if(EngineComponentData)
		EngineComponentData->StarterSoundLoaded = 0;
}

static void _DamageSoundStopped(void *data)
{
	SINT32* idx = (SINT32*)data;
	
	if(idx)
		*idx = 0;
}
/************************************************************************************************
	_FuelRateModifier
*************************************************************************************************/
// Scale fuel burn rate to approximate "real" unscaled distance traveled

static FLOAT64 _FuelRateModifier(vehVehicle* veh)
{
	/*mlVehicleData* modelData = veh->ModelData;
	FLOAT64VECTOR vel;
	FLOAT64 modifier = 1.0, speed;
//	FLOAT64 lat;
	
	if(!modelData) return 1.0;

	if(veh->VehicleType->Type == VEH_TYPE_AIR) // eventually handle ships in a similar manner
	{
		speed = mathExactVectorLength(&modelData->VehicleVel);
		if(speed > 1.0)
		{
			vel = modelData->VehicleVel;		

// couple of different ways we can do this
// completely "real world" range with lat/lon scaling
//			lat = modelData->VehiclePos.y / WORLD_HEIGHT * WORLD_ARC_HEIGHT * PI / 180.0;
//			vel.x *= 2.0 * cos(lat) / cos(37.0 * PI / 180.0); // scale based on latitude. 2:1 starts at about 37 lat (should be defined somewhere)
//			vel.y *= 2.0; // 2:1 (also really should've been defined somewhere)

// fixed reduction for horizontal components
			vel.x *= 1.65;
			vel.y *= 1.65;
			
			modifier = mathExactVectorLength(&vel) / speed;
		}
	}
	else // ground vehicles, flat reduction for gameplay purposes
	{
		modifier *= 1.65;	
	}
	*/
	return 1.65;
}

/************************************************************************************************
	_HandleEngineSounds
*************************************************************************************************/

static void _HandleEngineSounds(mlComponent* Component, mlEngineComponentData* EngineComponentData, 
								vehVehicle* veh, mlObjectData* VehicleObjectData, FLOAT32 VehicleG, UINT32 StarterOn)
{
	FLOAT32 Pitch,Volume;
	mlEngineData* EngineData = EngineComponentData->EngineData;
	OBJECT* obj;
	int i;
	
	if(veh->VehicleType->Type == VEH_TYPE_AIR) // only for aircraft at the moment
	{
		for(i = 0; i < 3; i++)
		{
			if(	EngineComponentData->EngineCaught && EngineComponentData->DamageLevelSoundId[i] == 0 && Component->DamageLevels[i] > 0 && 
				Component->CurrentDamage > Component->DamageLevels[i] && !EngineComponentData->DamageLevelSoundLoaded[i])
			{
				obj = (VehicleObjectData->HitMap != NULL) ? VehicleObjectData->HitMap : VehicleObjectData->PlaneObject;
			
#if USE_OSG
                osg::Vec3d pos = osg::Vec3d(Component->OriginalRelativePos.x, Component->OriginalRelativePos.y, Component->OriginalRelativePos.z);
                osg::Group* o = ((osg::Node*)obj->osgNode)->asGroup();
                
                int id = fxSoundPlayObjectSound(ENGINE_DAMAGE_SOUND[i],
                                                pos,
                                                infinite_sound,
                                                sound_mandatory,
                                                10.0f,
                                                3218.0f,
                                                o);

                EngineComponentData->DamageLevelSoundId[i] = id;

                if(id > 0)
                {
                    fxSoundSetKillCallback(id, _DamageSoundStopped);
                    fxSoundSetKillData(id, &EngineComponentData->DamageLevelSoundId[i]);
                    EngineComponentData->DamageLevelSoundLoaded[i] = 1;
                }
#else
		        SINT32 iID = fxSndGetNewSound();

                if(iID > 0)
                {
		            fxSndSetSoundName(iID, ENGINE_DAMAGE_SOUND[i]);
		            fxSndSetPriority(iID, IDE_FP_PRIORITY_MANDATORY);
		            fxSndSetSoundType(iID, TYPE_INFINITE_SOUND);
		            fxSndSetPitch(iID, 1.0f);
		            fxSndSetVolume(iID, 1.0f);
		            fxSndSetPosition(iID, Component->OriginalRelativePos.x, Component->OriginalRelativePos.y, Component->OriginalRelativePos.z);
		            fxSndSetEmitterObject(iID, obj);
		            fxSndSetKillData(iID, &EngineComponentData->DamageLevelSoundId[i]);
		            fxSndSetKillCallback(iID, _DamageSoundStopped);
		            fxSndSetPlayType(iID, DEF_PLAYTYPE_OBJECT);
		            fxSndSetMinDistance(iID, 10.0f);
		            fxSndSetMaxDistance(iID, 3218);
		            fxSndPushFlags(iID, DEF_SOUND_STOPS_AMBIENCE);

		            if(fxSndInitSound(iID))
		            {
		                EngineComponentData->DamageLevelSoundId[i] = iID;
		                EngineComponentData->DamageLevelSoundLoaded[i] = 1;
		            }
                }
#endif
	        }
		}
		
		// play "damage level 4" sound
		// this is a bit hacked as we want the sound to play as soon as the player sees the prop stop rotating
		if(	EngineComponentData->CurrentRpm < .1 &&
			Component->CurrentDamage > Component->DamageLevels[2] && EngineComponentData->DamageLevelSoundId[3] == 0
			&& !EngineComponentData->DamageLevelSoundLoaded[3])
		{
			obj = (VehicleObjectData->HitMap != NULL) ? VehicleObjectData->HitMap : VehicleObjectData->PlaneObject;
		
#if USE_OSG
            osg::Vec3d pos = osg::Vec3d(Component->OriginalRelativePos.x, Component->OriginalRelativePos.y, Component->OriginalRelativePos.z);
            osg::Group* o = ((osg::Node*)obj->osgNode)->asGroup();
            
            int id = fxSoundPlayObjectSound(ENGINE_DAMAGE_SOUND[3],
                                            pos,
                                            finite_sound,
                                            sound_mandatory,
                                            10.0f,
                                            3218.0f,
                                            o);

            EngineComponentData->DamageLevelSoundId[3] = id;

            if(id > 0)
            {
                 EngineComponentData->DamageLevelSoundLoaded[3] = 1;
            }
#else
	        SINT32 iID = fxSndGetNewSound();

            if(iID > 0)
            {
	            fxSndSetSoundName(iID, ENGINE_DAMAGE_SOUND[3]);
	            fxSndSetPriority(iID, IDE_FP_PRIORITY_MANDATORY);
	            fxSndSetSoundType(iID, TYPE_FINITE_SOUND);
	            fxSndSetPitch(iID, 1.0f);
	            fxSndSetVolume(iID, 1.0f);
	            fxSndSetPosition(iID, Component->OriginalRelativePos.x, Component->OriginalRelativePos.y, Component->OriginalRelativePos.z);
	            fxSndSetEmitterObject(iID, obj);
	            fxSndSetKillData(iID, 0);
	            fxSndSetKillCallback(iID, NULL);
	            fxSndSetPlayType(iID, DEF_PLAYTYPE_OBJECT);
	            fxSndSetMinDistance(iID, 10.0f);
	            fxSndSetMaxDistance(iID, 3218);
	            fxSndPushFlags(iID, DEF_SOUND_STOPS_AMBIENCE);
    	
			    EngineComponentData->DamageLevelSoundLoaded[3] = 1;
	        
	            if(fxSndInitSound(iID))
	            {
	                EngineComponentData->DamageLevelSoundId[3] = iID;
	                fxSndChangeVolume(EngineComponentData->DamageLevelSoundId[3], 1.0f);
				    fxSndChangePitch(EngineComponentData->DamageLevelSoundId[3], 1.0f);
	            }
            }
#endif

	        for(i = 0; i < 3; i++)
	        {
	        	if(EngineComponentData->DamageLevelSoundLoaded[i] && EngineComponentData->DamageLevelSoundId[i] != 0)
	        	{
#if USE_OSG
                    fxSoundStopSound(EngineComponentData->DamageLevelSoundId[i]);
#else
	        		fxSndStopSound(EngineComponentData->DamageLevelSoundId[i]);
#endif
	        	}
	        }
		}
	}
		
	if (!EngineComponentData->EngineSoundLoaded && EngineComponentData->EngineSound[0])
	{
		if (VehicleObjectData && VehicleObjectData->PlaneObject)
		{
			obj = (VehicleObjectData->HitMap != NULL) ? VehicleObjectData->HitMap : VehicleObjectData->PlaneObject;
		
#if USE_OSG
            osg::Vec3d pos = osg::Vec3d(Component->OriginalRelativePos.x, Component->OriginalRelativePos.y, Component->OriginalRelativePos.z);
            osg::Group* o = ((osg::Node*)obj->osgNode)->asGroup();

            int id = fxSoundPlayObjectSound((char*)EngineComponentData->EngineSound,
                                            pos,
                                            infinite_sound,
                                            sound_mandatory,
                                            10.0f,
                                            3218.0f,
                                            o);

            EngineComponentData->EngineSoundId = id;

            if(id > 0)
            {
			    EngineComponentData->EngineSoundLoaded = 1;
                fxSoundSetKillCallback(id, _EngineSoundStopped);
                fxSoundSetKillData(id, EngineComponentData);
            }                                
#else
	        SINT32 iID = fxSndGetNewSound();

            if(iID > 0)
            {
	            fxSndSetSoundName(iID, (SCHAR*)EngineComponentData->EngineSound);
	            fxSndSetPriority(iID, IDE_FP_PRIORITY_MANDATORY);
	            fxSndSetSoundType(iID, TYPE_INFINITE_SOUND);
	            fxSndSetPitch(iID, 1.0f);
	            fxSndSetVolume(iID, 1.0f);
	            fxSndSetPosition(iID, Component->OriginalRelativePos.x, Component->OriginalRelativePos.y, Component->OriginalRelativePos.z);
	            fxSndSetEmitterObject(iID, obj);
	            fxSndSetKillData(iID, EngineComponentData);
	            fxSndSetKillCallback(iID, _EngineSoundStopped);
	            fxSndSetPlayType(iID, DEF_PLAYTYPE_OBJECT);
	            fxSndSetMinDistance(iID, 10.0f);
	            fxSndSetMaxDistance(iID, 3218);
	            fxSndPushFlags(iID, DEF_SOUND_STOPS_AMBIENCE);

	            if(fxSndInitSound(iID))
	            {
	                EngineComponentData->EngineSoundId = iID;
				    EngineComponentData->EngineSoundLoaded = 1;
	            }
            }
#endif
        }
	}
	
	if (!EngineComponentData->IdleSoundLoaded && EngineComponentData->IdleSound[0])
	{
		if (VehicleObjectData && VehicleObjectData->PlaneObject)
		{
			obj = (VehicleObjectData->HitMap != NULL) ? VehicleObjectData->HitMap : VehicleObjectData->PlaneObject;

#if USE_OSG
            osg::Vec3d pos = osg::Vec3d(Component->OriginalRelativePos.x, Component->OriginalRelativePos.y, Component->OriginalRelativePos.z);
            osg::Group* o = ((osg::Node*)obj->osgNode)->asGroup();

            int id = fxSoundPlayObjectSound((char*)EngineComponentData->IdleSound,
                                            pos,
                                            infinite_sound,
                                            sound_mandatory,
                                            10.0f,
                                            3218.0f,
                                            o);

            EngineComponentData->IdleSoundId = id;

            if(id > 0)
            {
			    EngineComponentData->IdleSoundLoaded = 1;
                fxSoundSetKillCallback(id, _IdleSoundStopped);
                fxSoundSetKillData(id, EngineComponentData);
            }
#else
            SINT32 iID = fxSndGetNewSound();

            if(iID > 0)
            {
                fxSndSetSoundName(iID, (SCHAR*)EngineComponentData->IdleSound);
                fxSndSetPriority(iID, IDE_FP_PRIORITY_MANDATORY);
                fxSndSetSoundType(iID, TYPE_INFINITE_SOUND);
                fxSndSetPitch(iID, 1.0f);
                fxSndSetVolume(iID, 1.0f);
                fxSndSetPosition(iID, Component->OriginalRelativePos.x, Component->OriginalRelativePos.y, Component->OriginalRelativePos.z);
                fxSndSetEmitterObject(iID, obj);
                fxSndSetKillData(iID, EngineComponentData);
                fxSndSetKillCallback(iID, _IdleSoundStopped);
                fxSndSetPlayType(iID, DEF_PLAYTYPE_OBJECT);
                fxSndSetMinDistance(iID, 10.0f);
                fxSndSetMaxDistance(iID, 3218.0f);
                fxSndPushFlags(iID, DEF_SOUND_STOPS_AMBIENCE);

                if(fxSndInitSound(iID))
                {
                    EngineComponentData->IdleSoundId = iID;
				    EngineComponentData->IdleSoundLoaded = 1;
			    }
            }
#endif
		}
	}

	if (!EngineComponentData->StarterSoundLoaded && StarterOn && 
		!EngineComponentData->EngineLastCaught &&
		 EngineComponentData->StartupSound[0])
	{
		if (VehicleObjectData && VehicleObjectData->PlaneObject)
		{
			obj = (VehicleObjectData->HitMap != NULL) ? VehicleObjectData->HitMap : VehicleObjectData->PlaneObject;

#if USE_OSG
            osg::Vec3d pos = osg::Vec3d(Component->OriginalRelativePos.x, Component->OriginalRelativePos.y, Component->OriginalRelativePos.z);
            osg::Group* o = ((osg::Node*)obj->osgNode)->asGroup();

            int id = fxSoundPlayObjectSound((char*)EngineComponentData->StartupSound,
                                            pos,
                                            infinite_sound,
                                            sound_mandatory,
                                            10.0f,
                                            3218.0f,
                                            o);

            EngineComponentData->StarterSoundId = id;

            if(id > 0)
            {
			    EngineComponentData->StarterSoundLoaded = 1;
                fxSoundSetKillCallback(id, _StarterSoundStopped);
                fxSoundSetKillData(id, EngineComponentData);
            }
#else
            SINT32 iID = fxSndGetNewSound();

            if(iID > 0)
            {
                fxSndSetSoundName(iID, (SCHAR*)EngineComponentData->StartupSound);
                fxSndSetPriority(iID, IDE_FP_PRIORITY_MANDATORY);
                fxSndSetSoundType(iID, TYPE_INFINITE_SOUND);
                fxSndSetPitch(iID, 1.0f);
                fxSndSetVolume(iID, 1.0f);
                fxSndSetPosition(iID, Component->OriginalRelativePos.x, Component->OriginalRelativePos.y, Component->OriginalRelativePos.z);
                fxSndSetEmitterObject(iID, obj);
                fxSndSetKillData(iID, EngineComponentData);
                fxSndSetKillCallback(iID, _StarterSoundStopped);
                fxSndSetPlayType(iID, DEF_PLAYTYPE_OBJECT);
                fxSndSetMinDistance(iID, 10.0f);
                fxSndSetMaxDistance(iID, 3218.0f);
                fxSndPushFlags(iID, DEF_SOUND_STOPS_AMBIENCE);

                if(fxSndInitSound(iID))
                {
                    EngineComponentData->StarterSoundId = iID;
                    EngineComponentData->StarterSoundLoaded = 1;
			    }
            }
#endif
		}
	}

	if (EngineComponentData->StarterSoundLoaded && 
		(!StarterOn || EngineComponentData->CurrentRpm > EngineData->CriticalRpm))
	{
#if USE_OSG
        fxSoundStopSound(EngineComponentData->StarterSoundId);
#else
		fxSndStopSound(EngineComponentData->StarterSoundId);
#endif
		EngineComponentData->StarterSoundLoaded = 0;
	}

	if (EngineComponentData->EngineSoundLoaded)
	{
		if (EngineComponentData->EngineCaught)
		{
			Volume = (FLOAT32)(EngineComponentData->CurrentManifoldPress / EngineData->BaseHpManifoldPress);
			if (Volume > 1.0)
				Volume = 1.0;
		}else
		{
			Volume = 0.0;
		}

		if (EngineData->Neg_G_Fuel_Cutout != 0 && VehicleG <= -0.2f)
			Volume = 0.0;

#if USE_OSG
        fxSoundChangeVolume(EngineComponentData->EngineSoundId, Volume);
#else
		fxSndChangeVolume(EngineComponentData->EngineSoundId,Volume);
#endif
		Pitch = (FLOAT32)(EngineComponentData->CurrentRpm / EngineData->MilRpm);

		if (Pitch < 0.05)
			Pitch = (FLOAT32)0.05;
		else if (Pitch > 2.0)
			Pitch = (FLOAT32)2.0;

#if USE_OSG
        fxSoundChangePitch(EngineComponentData->EngineSoundId, Pitch);
#else
		fxSndChangePitch(EngineComponentData->EngineSoundId,Pitch);
#endif
	}
	
	if (EngineComponentData->IdleSoundLoaded)
	{
		if (EngineComponentData->CurrentRpm > EngineData->StarterRpm)
			Volume = 1.0;
		else
			Volume = (FLOAT32)(EngineComponentData->CurrentRpm / EngineData->StarterRpm);
		
#if USE_OSG
        fxSoundChangeVolume(EngineComponentData->IdleSoundId, Volume);
#else
		fxSndChangeVolume(EngineComponentData->IdleSoundId,Volume);
#endif
		Pitch = (FLOAT32)(EngineComponentData->CurrentRpm / EngineData->MilRpm);

		if (Pitch < 0.05)
			Pitch = (FLOAT32)0.05;
		else if (Pitch > 2.0)
			Pitch = (FLOAT32)2.0;

#if USE_OSG
        fxSoundChangePitch(EngineComponentData->IdleSoundId, Pitch);
#else
		fxSndChangePitch(EngineComponentData->IdleSoundId,Pitch);
#endif
	}
	
	if(veh->VehicleType->Type == VEH_TYPE_AIR) // only for aircraft at the moment
	{
		for(i = 0; i < 3; i++)
		{
			if(EngineComponentData->DamageLevelSoundId[i] > 0 && EngineComponentData->DamageLevelSoundLoaded[i])
			{
				Volume = (FLOAT32)((Component->CurrentDamage - Component->DamageLevels[i]) / (Component->DamageLevels[3] - Component->DamageLevels[i]));

				if(Volume < 0.0)
					Volume = 0.0f;
				else if(Volume > 1.0)
					Volume = 1.0f;
					
				if(!EngineComponentData->EngineCaught || EngineComponentData->CurrentRpm < .1)
					Volume = 0.0f;
					
#if USE_OSG
                fxSoundChangeVolume(EngineComponentData->DamageLevelSoundId[i], Volume);
#else
				fxSndChangeVolume(EngineComponentData->DamageLevelSoundId[i], Volume);
#endif
				Pitch = (FLOAT32)(EngineComponentData->CurrentRpm / EngineData->MilRpm);

				if (Pitch < 0.05)		Pitch = 0.05f;
				else if (Pitch > 2.0) 	Pitch = 2.0f;

#if USE_OSG
                fxSoundChangePitch(EngineComponentData->DamageLevelSoundId[i], Pitch);
#else
				fxSndChangePitch(EngineComponentData->DamageLevelSoundId[i],Pitch);
#endif
			}
		}
	}
}