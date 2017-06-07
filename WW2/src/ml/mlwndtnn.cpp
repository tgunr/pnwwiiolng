#include "syscfg.h" // Path is project specific.
#ifdef BETA_RELEASE

#include <math.h>

#include "ml/mlwndtnn.h"
#include "mlatmo.h"
#include "ml/mltypes.h"
#include "ml/mlfoils.h"
#include "mlengine.h"
#include "ml/mlutility.h"
#include "pers/persmain.h"

static UINT32 _PrepSystemForWindTunnel(mlVehicleData * VehicleData);
static UINT32 _OpenWindTunnelFile(mlVehicleData * VehicleData);
static UINT32 _OutputBasicStats(mlVehicleData * VehicleData);
static UINT32 _GenerateNewThrustCurve(mlVehicleData * VehicleData);
static UINT32 _ReadThrustFile();
static UINT32 _GenerateEnginePowerChart(mlVehicleData * VehicleData);
static UINT32 _GenerateStallData(mlVehicleData * VehicleData);
static UINT32 _GenerateTopSpeedData(mlVehicleData * VehicleData);
static UINT32 _GenerateClimbData(mlVehicleData * VehicleData);
static UINT32 _GenerateRollRateAtSeaLevelData(mlVehicleData * VehicleData);
static UINT32 _GenerateClCdCurve(mlVehicleData * VehicleData);
static UINT32 _CloseWindTunnelFile(mlVehicleData * VehicleData);
static UINT32 _CleanUp(mlVehicleData * VehicleData);

#if(0)
static UINT32 _SetEnginesToMaxPower(mlVehicleData * VehicleData);
static UINT32 _SetPropsToAbsorbMaxPower(mlVehicleData * VehicleData);
#endif
static UINT32 _TunnelTurnEnginesOn(mlVehicleData * VehicleData);
static UINT32 _TunnelTurnEnginesOff(mlVehicleData * VehicleData);
static UINT32 _DetermineFirstEngineHp(mlVehicleData * VehicleData,FLOAT64 * Hp,FLOAT64 Alt);
static UINT32 _TunnelResetVehiclePos(mlVehicleData * VehicleData,FLOAT64 Alt);
static UINT32 _TunnelDetermineOneGAoA(mlVehicleData * VehicleData,
									  FLOAT64 * AoA,
									  FLOAT64 * YForce,
									  FLOAT64 Speed,
									  FLOAT64 MinAoA,
									  FLOAT64 MaxAoA,
									  FLOAT64 Step,
									  FLOAT64 Alt);
static UINT32 _SetPropsToCorrectPitchForSpeed(mlVehicleData * VehicleData,FLOAT64 Speed,FLOAT64 Alt);
static UINT32 _RunThrustCurve(FLOAT64 * ReturnThrust,FLOAT64 Speed,FLOAT64 Alt,
							  mlEngineComponentData * PropData,
							  mlComponent * Component);
static UINT32 _AddInPrecalculatedThrust(mlVehicleData * VehicleData,FLOAT64 Alt,FLOAT64 Speed);
static UINT32 _CountEngines(mlVehicleData * VehicleData);

extern UINT32 _ProcessControlInputs(UINT32 DeltaTime,mlVehicleData * VehicleData);

static UINT32 _TunnelTurnEnginesOnForClimb(mlVehicleData * VehicleData);

static FILE * TunnelFile;

extern UINT32 _NoHorizStab;
extern UINT32 _DisableEngineThrust;
extern UINT32 _DisableCollisions;
static UINT32 _iterations;

static UINT32 _EnginesAreOn;

#define MAX_ALT_INDEX 41
#define MAX_SPEED_INDEX 41
#define SPEED_INC 5.0
#define ALT_INC 500.0

static FLOAT64 _ThrustCurve[MAX_ALT_INDEX][MAX_SPEED_INDEX]; // first param is altitude * 500m, 2nd is speed * 5m/s

//FLOAT64 _AoAThrustModifier[31]; // AoA from -10 degrees to 20 degrees
static UINT32 _NumEngines;
static LOGICAL climbtest;
static UINT32 maxaltindex = 12;

extern UINT32 _AdjustVelAndRotVel(mlVehicleData*, FLOAT64VECTOR* Vel,
							   FLOAT64ATT* AttVel,
							   FLOAT64VECTOR* Acc,
							   FLOAT64ATT* AttAcc,
							   FLOAT64VECTOR* Force,
							   FLOAT64ATT* Torque,
							   FLOAT64 Mass,
							   FLOAT64ATT* RotInertia,
							   FLOAT64 DeltaTime, LOGICAL UseCorrectPhysics);

extern UINT32 _AdjustPosAndAccel(FLOAT64VECTOR* Pos,
								  FLOAT64ATT* Att,
								  FLOAT64VECTOR* Vel,
								  FLOAT64VECTOR* Acc,
								  FLOAT64VECTOR* LastAcc,
								  FLOAT64VECTOR* LastVel,
								  FLOAT64ATT* AttVel,
								  FLOAT64ATT* AttAcc,
								  FLOAT64ATT* LastAttAcc,
								  FLOAT64 DeltaTime);


UINT32 mlRunWindTunnel(LOGICAL DoStats, LOGICAL DoHp, LOGICAL DoStall,
						LOGICAL DoTopSpeed, LOGICAL DoClimb, LOGICAL DoRollRate,
						LOGICAL DoClCdCurves, LOGICAL DoNewThrustFile)
{
FILE * TlistFile;
char s[256];

UINT32 time1,time2;

	_DisableCollisions = 1;

	TlistFile = fopen("tlist.txt","r");
	if (TlistFile != nullptr)
	{
		do
		{
			fgets(s,255,TlistFile);
		}
		while (s[0] == '#');
		DoStats = atoi(s);

		do
		{
			fgets(s,255,TlistFile);
		}
		while (s[0] == '#');
		DoNewThrustFile = atoi(s);

		do
		{
			fgets(s,255,TlistFile);
		}
		while (s[0] == '#');
		DoHp = atoi(s);

		do
		{
			fgets(s,255,TlistFile);
		}
		while (s[0] == '#');
		DoStall = atoi(s);

		do
		{
			fgets(s,255,TlistFile);
		}
		while (s[0] == '#');
		DoTopSpeed = atoi(s);

		do
		{
			fgets(s,255,TlistFile);
		}
		while (s[0] == '#');
		DoClimb = atoi(s);

		do
		{
			fgets(s,255,TlistFile);
		}
		while (s[0] == '#');
		DoRollRate = atoi(s);

		do
		{
			fgets(s,255,TlistFile);
		}
		while (s[0] == '#');
		DoClCdCurves = atoi(s);

		do
		{
			fgets(s,255,TlistFile);
		}
		while (s[0] == '#');
		maxaltindex = atoi(s) ? 24 : 12;
	}

	_iterations = 0;

	time1 = GetMillisecondTick();

	mlVehicleData * VehicleData = persGetPersonaModelDataPtr();

	if (VehicleData == nullptr)
	{
		Write_Debug("No vehicle model data for player");
		return 0;
	}

	_PrepSystemForWindTunnel(VehicleData);

	_OpenWindTunnelFile(VehicleData);

	if (DoStats)
		_OutputBasicStats(VehicleData);


	if(!DoNewThrustFile)
		DoNewThrustFile = _ReadThrustFile();

	if (DoNewThrustFile)
		_GenerateNewThrustCurve(VehicleData);

	_DisableEngineThrust = 1;

	_CountEngines(VehicleData);

	if (DoHp)
		_GenerateEnginePowerChart(VehicleData);

	if (DoStall)
		_GenerateStallData(VehicleData);

	if (DoTopSpeed)
		_GenerateTopSpeedData(VehicleData);

	if (DoClimb)
	{
		climbtest = TRUE;
		_GenerateClimbData(VehicleData);
		climbtest = FALSE;
	}
	if (DoRollRate)
		_GenerateRollRateAtSeaLevelData(VehicleData);

	if (DoClCdCurves)
		_GenerateClCdCurve(VehicleData);

	_DisableEngineThrust = 0;
	_NoHorizStab = 0;

	time2 = GetMillisecondTick();

	Write_Debug("Wind tunnel processing time: %0.3lf s, iterations = %d",
		(FLOAT64)(time2 - time1) / 1000.0,
		(UINT32)(_iterations));
	fprintf(TunnelFile,"\nWind tunnel processing time: %0.3lf s, iterations = %d",
		(FLOAT64)(time2 - time1) / 1000.0,
		(UINT32)(_iterations));

	_CloseWindTunnelFile(VehicleData);

	_CleanUp(VehicleData);

	_DisableCollisions = 0;

	return 0;
}


static UINT32 _PrepSystemForWindTunnel(mlVehicleData * VehicleData)
{
	VehicleData->CollisionsOff = 1;
	VehicleData->GearSetting = 0;
	VehicleData->GearPosition = 0.0;
	VehicleData->FlapPosition = 0.0;
	return 0;
}

static UINT32 _OpenWindTunnelFile(mlVehicleData * VehicleData)
{
	TunnelFile = fopen("tunnel.txt","w");

	return 0;
}

static UINT32 _OutputBasicStats(mlVehicleData * VehicleData)
{
	FLOAT64 Speed;
	FLOAT64 Q;
	FLOAT64 FrontalArea;
	mlComponent * Component;
	FLOAT64VECTOR DummyForce;
	FLOAT64VECTOR DummyForceSum;
	FLOAT64VECTOR SavedCG;
	FLOAT64 LowestTorque;
	FLOAT64 YPosLowestTorque;
	mlComponentListIterator it;

	mlCalcVehicleMass(VehicleData);
	mlCalcVehicleCenterOfMass(VehicleData);
	mlSetVehicleComponentsRelativePos(VehicleData);
	mlCalcVehicleRotationalInertia(VehicleData);
	fprintf(TunnelFile,"Vehicle Mass (kg) = %lf, (lb) = %lf\n",
				VehicleData->VehicleMass,
				VehicleData->VehicleMass * 2.205);
	fprintf(TunnelFile,"Vehicle Center of Mass (m) x = %lf, y = %lf, z = %lf\n",
				VehicleData->VehicleCenterOfMass.x,
				VehicleData->VehicleCenterOfMass.y,
				VehicleData->VehicleCenterOfMass.z);
	fprintf(TunnelFile,"Vehicle Rotational Inertia Pitch = %lf, Roll = %lf, Yaw = %lf",
				VehicleData->VehicleRotationalInertia.Pitch,
				VehicleData->VehicleRotationalInertia.Roll,
				VehicleData->VehicleRotationalInertia.Yaw);
	fprintf(TunnelFile,"\n");

	Write_Debug("Calculated vehicle mass and center of mass\n");

	_TunnelTurnEnginesOff(VehicleData);

	_TunnelResetVehiclePos(VehicleData,0.0);

	Speed = 100.0;

	VehicleData->VehicleAtt.Pitch = 0;
	VehicleData->VehicleAtt.Roll = VehicleData->VehicleAtt.Yaw = 0;
	VehicleData->VehicleVel.y = Speed;

	_SetPropsToCorrectPitchForSpeed(VehicleData,Speed,0.0);

	VehicleData->VehicleForce.x = 0;
	VehicleData->VehicleForce.y = 0;
	VehicleData->VehicleForce.z = 0;

	VehicleData->VehicleTorque.Roll = 0;
	VehicleData->VehicleTorque.Pitch = 0;
	VehicleData->VehicleTorque.Yaw = 0;


	// now compute component forces and torques and apply them to the whole
	mlComponentListBegin(VehicleData->ComponentList, &it);
	for(; it.comp; mlComponentListNext(&it))
	{
		Component = it.comp;

		mlUpdateComponent(Component,VehicleData,50,0,&DummyForce,&DummyForceSum);
	}

	Q = 100.0 * 100.0 * 1.225 * 1.11 * 0.5;

	FrontalArea = -VehicleData->VehicleForce.y / Q;

	fprintf(TunnelFile,"Frontal area = %0.2lf m^2,%0.2lf ft^2\n",
						FrontalArea,
						FrontalArea * 10.764);
	// now compute neutral point

	// first save off original center of mass
	SavedCG = VehicleData->VehicleCenterOfMass;

	_NoHorizStab = 0;
	_DisableEngineThrust = 1;
	VehicleData->ElevatorPos = 0.0;
	// now cycle through different positions of center of mass
	// and determine which causes the pitch torque to be smallest at 100m/s
	LowestTorque = 100000000.0;
	YPosLowestTorque = 0.0;
	for (VehicleData->VehicleCenterOfMass.y = 0.0;
		 VehicleData->VehicleCenterOfMass.y > -4.0;
		 VehicleData->VehicleCenterOfMass.y -= 0.01)
	{
		mlSetVehicleComponentsRelativePos(VehicleData);

		Speed = 100.0;

		VehicleData->VehicleAtt.Pitch = mlDEG_TO_RAD(5.0);;
		VehicleData->VehicleAtt.Roll = VehicleData->VehicleAtt.Yaw = 0;
		VehicleData->VehicleVel.y = Speed;

//		_SetPropsToCorrectPitchForSpeed(VehicleData,Speed,0.0);

		VehicleData->VehicleForce.x = 0;
		VehicleData->VehicleForce.y = 0;
		VehicleData->VehicleForce.z = 0;

		VehicleData->VehicleTorque.Roll = 0;
		VehicleData->VehicleTorque.Pitch = 0;
		VehicleData->VehicleTorque.Yaw = 0;


		// now compute component forces and torques and apply them to the whole
		mlComponentListBegin(VehicleData->ComponentList, &it);
		for(; it.comp; mlComponentListNext(&it))
		{
			Component = it.comp;

			mlUpdateComponent(Component,VehicleData,50,0,&DummyForce,&DummyForceSum);
		}


		if (fabs(VehicleData->VehicleTorque.Pitch) < LowestTorque)
		{
			LowestTorque = fabs(VehicleData->VehicleTorque.Pitch);
			YPosLowestTorque = VehicleData->VehicleCenterOfMass.y;
		}
	}
	VehicleData->VehicleCenterOfMass = SavedCG;

	fprintf(TunnelFile,"Neutral Point is Y = %lf behind CG\n\n",YPosLowestTorque);

	_TunnelTurnEnginesOn(VehicleData);


	return 0;
}

static UINT32 _GenerateNewThrustCurve(mlVehicleData * VehicleData)
{
UINT32 i,j;
FLOAT64 Alt,Speed;
mlComponent * Component;
mlEngineComponentData * EngineComponentData;
mlEngineComponentData * PropData;
FLOAT64VECTOR DummyForce;
FLOAT64ATT DummyTorque;
FLOAT64VECTOR TempVec;
FILE * ThrustFile;
mlComponentListIterator it;

	_TunnelTurnEnginesOn(VehicleData);

	for (i=0;i<=maxaltindex;i++)
	{
		Alt = i * ALT_INC;

		for (j=0;j<MAX_SPEED_INDEX;j++)
		{
			Speed = j * SPEED_INC;


			// clear sum forces;
			VehicleData->VehicleForce.x = 0;
			VehicleData->VehicleForce.y = 0;
			VehicleData->VehicleForce.z = 0;

			VehicleData->VehicleTorque.Roll = 0;
			VehicleData->VehicleTorque.Pitch = 0;
			VehicleData->VehicleTorque.Yaw = 0;


			// now compute component forces and torques and apply them to the whole
			mlComponentListBegin(VehicleData->ComponentList, &it);
			for(; it.comp; mlComponentListNext(&it))
			{
				Component = it.comp;
				if (Component->Type == ML_COMPONENT_ENGINE)
				{
					EngineComponentData = (mlEngineComponentData*)Component->SpecialData;
					if (EngineComponentData->SubType == ML_ENGINE_SUBTYPE_ENGINE)
					{


						EngineComponentData->EngineMode = (climbtest) ? ML_ENGINE_MODE_CLIMB : ML_ENGINE_MODE_MIL;

						PropData = (mlEngineComponentData*)EngineComponentData->PropellorComponent->SpecialData;
						PropData->CurrentRpm = EngineComponentData->EngineData->MilRpm;
						PropData->RpmSetting = PropData->CurrentRpm;

						EngineComponentData->EngineCaught = 1;


						TempVec.x = TempVec.y = TempVec.z = 0;

						mlUpdateEngine(&DummyForce,
									   &DummyTorque,
									   EngineComponentData,
									   Component,
									   Alt,
									   &TempVec,
									   1,
									   1.0,
									   1,
									   0,
									   0,
									   VehicleData->VehicleObjectData,
									   (void*)VehicleData->VehicleMasterPtr,
									   1.0f);
					}
				}
			}

			mlComponentListBegin(VehicleData->ComponentList, &it);
			for(; it.comp; mlComponentListNext(&it))
			{
				Component = it.comp;
				if (Component->Type == ML_COMPONENT_ENGINE)
				{
					EngineComponentData = (mlEngineComponentData*)Component->SpecialData;
					if (EngineComponentData->SubType == ML_ENGINE_SUBTYPE_PROPELLOR)
					{
						_SetPropsToCorrectPitchForSpeed(VehicleData,Speed,Alt);
						_RunThrustCurve(&_ThrustCurve[i][j],Speed,Alt,EngineComponentData,Component);

						break;
					}

				}
			}

		}
	}

	ThrustFile = fopen("thrustfile.bin","wb");
	if (ThrustFile == nullptr)
	{
		return 1;
	}

	fseek(ThrustFile,0,0);

	fwrite(&_ThrustCurve[0][0],sizeof(FLOAT64) * MAX_SPEED_INDEX * MAX_ALT_INDEX,1,ThrustFile);
	fclose(ThrustFile);

	return 0;
}

static UINT32 _ReadThrustFile()
{
FILE* ThrustFile;

	ThrustFile = fopen("thrustfile.bin","rb");
	if (ThrustFile == nullptr)
	{
		Write_Debug("Cannot find thrust file");
		return 1;
	}

	fseek(ThrustFile,0,0);

	fread(&_ThrustCurve[0][0],sizeof(FLOAT64) * MAX_SPEED_INDEX * MAX_ALT_INDEX,1,ThrustFile);
	fclose(ThrustFile);

	return 0;
}

static UINT32 _RunThrustCurve(FLOAT64 * ReturnThrust,FLOAT64 Speed,FLOAT64 Alt,
							  mlEngineComponentData * PropData,
							  mlComponent * Component)
{
FLOAT64VECTOR ComponentVel;
FLOAT64VECTOR ReturnForce;
FLOAT64ATT DummyTorque;
FLOAT64ATT TempAtt;
FLOAT64 SumThrust;
UINT32 i;

	_DisableEngineThrust = 0;

	DummyTorque.Pitch = DummyTorque.Roll = DummyTorque.Yaw = 0.0;

	ComponentVel.x = ComponentVel.z = 0;
	ComponentVel.y = Speed;

	TempAtt.Pitch = TempAtt.Roll = TempAtt.Yaw = 0.0;

	// run 10 seconds of prop flight loop to stabilize prop
	for (i=0;i<50;i++)
	{
		mlUpdatePropellor(&ReturnForce,
						  &DummyTorque,
						  PropData,
						  Component,
						  Alt,
						  &ComponentVel,
						  50,
						  &DummyTorque,
						  0,
						  1,
						  nullptr);
	}

	// now 20 minutes of the actual prop running to get average thrust
	SumThrust = 0.0;
	for (i=0;i<100;i++)
	{
		mlUpdatePropellor(&ReturnForce,
						  &DummyTorque,
						  PropData,
						  Component,
						  Alt,
						  &ComponentVel,
						  50,
						  &TempAtt,
						  0,
						  1,
						  nullptr);

		SumThrust += ReturnForce.y;
	}

	*ReturnThrust = -SumThrust / 100.0;

	_DisableEngineThrust = 1;

	return 0;
}

static UINT32 _GenerateEnginePowerChart(mlVehicleData * VehicleData)
{
FLOAT64 Alt;
FLOAT64 Hp;
FLOAT64 BaseHp;
FLOAT64 Speed;
UINT32 i,j;

	// first, generate the horsepower chart
	fprintf(TunnelFile,"\nEngine brake horsepower output\n");
	fprintf(TunnelFile,"\nAlt m    ft     hp\n");

	for (Alt = 0;Alt <= maxaltindex*ALT_INC /*6000*/; Alt+= ALT_INC/2.0 /*250.0*/)
	{
		fprintf(TunnelFile,"%5.0lf %5.0lf  ",Alt,Alt*3.281);

		_TunnelTurnEnginesOn(VehicleData);
		_TunnelResetVehiclePos(VehicleData,0);

		_DetermineFirstEngineHp(VehicleData,&Hp,Alt);

		fprintf(TunnelFile,"%7.3lf ",Hp);

		fprintf(TunnelFile,"\n");

	}

	fprintf(TunnelFile,"\n");

	fprintf(TunnelFile,"\n prop thrust chart:  base hp = %lf\n",VehicleData->EngineList->EngineData[0].BaseHorsePower);
	fprintf(TunnelFile,"          m/s ");
	for (Speed = 20;Speed <= 200.0; Speed += 20.0)
	{
		fprintf(TunnelFile,"%7.1lf ",Speed);
	}
	fprintf(TunnelFile,"\n          kph ");
	for (Speed = 20;Speed <= 200; Speed += 20.0)
	{
		fprintf(TunnelFile,"%7.1lf ",Speed * 3.6);
	}
	fprintf(TunnelFile,"\nAlt m ft  mph ");
	for (Speed = 20;Speed <= 200; Speed += 20.0)
	{
		fprintf(TunnelFile,"%7.1lf ",Speed * 2.237);
	}
	fprintf(TunnelFile,"\n\n");

	// next generate thrust chart
	for (i = 0;i<=maxaltindex;i++)
	{
		fprintf(TunnelFile,"%5.0lf %5.0lf  ",i*ALT_INC,i*ALT_INC*3.281);
		for (j=4;j<MAX_SPEED_INDEX;j+=4)
		{
			fprintf(TunnelFile,"%7.0lf ",_ThrustCurve[i][j]);
		}
		fprintf(TunnelFile,"\n");

	}


	// finally generate the thrust efficiency chart
	fprintf(TunnelFile,"\n");

	fprintf(TunnelFile,"\n prop efficiency chart:  base hp = %lf\n",VehicleData->EngineList->EngineData[0].BaseHorsePower);
	fprintf(TunnelFile,"          m/s ");
	for (Speed = 20;Speed <= 200.0; Speed += 20.0)
	{
		fprintf(TunnelFile,"%7.1lf ",Speed);
	}
	fprintf(TunnelFile,"\n          kph ");
	for (Speed = 20;Speed <= 200; Speed += 20.0)
	{
		fprintf(TunnelFile,"%7.1lf ",Speed * 3.6);
	}
	fprintf(TunnelFile,"\nAlt m ft  mph ");
	for (Speed = 20;Speed <= 200; Speed += 20.0)
	{
		fprintf(TunnelFile,"%7.1lf ",Speed * 2.237);
	}
	fprintf(TunnelFile,"\n\n");

	// next generate thrust chart
	for (i = 0;i<=maxaltindex;i++)
	{
		Alt = i * ALT_INC;
		_DetermineFirstEngineHp(VehicleData,&BaseHp,Alt);
		fprintf(TunnelFile,"%5.0lf %5.0lf  ",i*ALT_INC,i*ALT_INC*3.281);
		for (j=4;j<MAX_SPEED_INDEX;j+=4)
		{
			Hp = _ThrustCurve[i][j];
			Hp *= j * 5.0;
			Hp /= 745.7;
			fprintf(TunnelFile,"%7.3lf ",Hp / BaseHp);
		}
		fprintf(TunnelFile,"\n");

	}

	fprintf(TunnelFile,"\n\n");

	return 0;
}

static UINT32 _GenerateStallData(mlVehicleData * VehicleData)
{
FLOAT64 Speed;
FLOAT64 AoA;
FLOAT64 YForce;
FLOAT64 StallSpeed;
UINT32 Result;

	// now calc stall speed
	_NoHorizStab = 1;

	// power off, clean
	VehicleData->GearSetting = 0;
	VehicleData->GearPosition = 0.0;
	_TunnelTurnEnginesOff(VehicleData);
	_TunnelResetVehiclePos(VehicleData,0);
	VehicleData->FlapPosition = 0;
	for (Speed=10.0;Speed<100.0;Speed += 0.25)
	{
//SINT32 T1,T2;

//		T1 = piReadTimeStamp32();

		Result = _TunnelDetermineOneGAoA(VehicleData,&AoA,&YForce,Speed,10.0,20.0,0.25,0);
		if (Result != 1)
		{
			break;
		}

//		T2 = piReadTimeStamp32();

//		Write_Debug("Stall test speed = %lf,time = %d",Speed,T2-T1);
	}
	StallSpeed = Speed;

	fprintf(TunnelFile,"Stall, clean, power off (m/s) = %lf, (kph) = %lf, (mph) = %lf\n",
		Speed,
		Speed * 3.6,
		Speed * 2.237);
	// power off, dirty

	VehicleData->GearSetting = 1;
	VehicleData->GearPosition = 1.0;
	_TunnelResetVehiclePos(VehicleData,0);
	VehicleData->FlapPosition = VehicleData->NumberOfFlapPositions;
	for (Speed=10.0;Speed<100.0;Speed += 0.25)
	{
		Result = _TunnelDetermineOneGAoA(VehicleData,&AoA,&YForce,Speed,10.0,20.0,0.25,0);
		if (Result != 1)
		{
			break;
		}
	}
	fprintf(TunnelFile,"Stall, dirty, power off (m/s) = %lf, (kph) = %lf, (mph) = %lf\n",
		Speed,
		Speed * 3.6,
		Speed * 2.237);

	// power on, clean
	VehicleData->GearSetting = 0;
	VehicleData->GearPosition = 0.0;
	_TunnelTurnEnginesOn(VehicleData);
	_TunnelResetVehiclePos(VehicleData,0);
	VehicleData->FlapPosition = 0;
	for (Speed=10.0;Speed<100.0;Speed += 0.25)
	{
		Result = _TunnelDetermineOneGAoA(VehicleData,&AoA,&YForce,Speed,10.0,20.0,0.25,0);
		if (Result != 1)
		{
			break;
		}
	}
	fprintf(TunnelFile,"Stall, clean, power on (m/s) = %lf, (kph) = %lf, (mph) = %lf\n",
		Speed,
		Speed * 3.6,
		Speed * 2.237);

	// power on, dirty
	VehicleData->GearSetting = 1;
	VehicleData->GearPosition = 1.0;
	_TunnelResetVehiclePos(VehicleData,0);
	VehicleData->FlapPosition = VehicleData->NumberOfFlapPositions;
	for (Speed=10.0;Speed<100.0;Speed += 0.25)
	{
		Result = _TunnelDetermineOneGAoA(VehicleData,&AoA,&YForce,Speed,10.0,20.0,0.25,0);
		if (Result != 1)
		{
			break;
		}
	}
	fprintf(TunnelFile,"Stall, dirty, power on (m/s) = %lf, (kph) = %lf, (mph) = %lf\n",
		Speed,
		Speed * 3.6,
		Speed * 2.237);
	Write_Debug("Done calculating stall speeds\n");

	VehicleData->GearSetting = 0;
	VehicleData->GearPosition = 0.0;
	VehicleData->FlapPosition = 0.0;

	return 0;
}

static UINT32 _GenerateTopSpeedData(mlVehicleData * VehicleData)
{
FLOAT64 YForce,Speed,Alt;
FLOAT64 LastYForce;
UINT32 Result;
FLOAT64 AoA;

	// top speed chart
	_NoHorizStab = 0;
	fprintf(TunnelFile,"\n");
	fprintf(TunnelFile,"Top Speed:\n");
	fprintf(TunnelFile,"Alt (m) (ft) :  m/s  kph  mph\n");
	VehicleData->FlapPosition = 0;
	_TunnelTurnEnginesOn(VehicleData);

	for (Alt = 0.0;Alt <=  maxaltindex*ALT_INC /*6000.0*/; Alt += ALT_INC)
	{
		YForce = 0;

		fprintf(TunnelFile,"%6.0lf %6.0lf  ",Alt,Alt * 3.281);
		for (Speed = 200.0; Speed > 50.0; Speed -= 1.0)
		{
			LastYForce = YForce;
			_TunnelTurnEnginesOn(VehicleData);
			Result = _TunnelDetermineOneGAoA(VehicleData,&AoA,&YForce,Speed,-4.0,6.0,0.1,Alt);
			if (YForce > 0.0 && Result == 0)
			{
				break;
			}
		}

		if (Speed <= 50.0)
		{
			fprintf(TunnelFile,"N/A\n");
		}
		else
		{
			fprintf(TunnelFile,"%4.0lf %4.0lf %4.0lf\n",Speed,Speed * 3.6,Speed * 2.237);
		}
	}

	Write_Debug("Done calculating top speeds\n");

	return 0;
}

static UINT32 _GenerateClimbData(mlVehicleData * VehicleData)
{
FLOAT64 LowestDrag,LowestSpeed;
FLOAT64 Speed;
FLOAT64 StallSpeed;
UINT32 Result;
FLOAT64 AoA;
FLOAT64 YForce;
FLOAT64 MaxClimbSpeed;
FLOAT64 Alt;
FLOAT64 ClimbRate,HighestClimbRate;
UINT32 FoundStall = 0;

	// climb speed chart
	fprintf(TunnelFile,"\n");
	_NoHorizStab = 0;
//	_DisableEngineThrust = 1;

	// first determine best raw climb speed
	// go from stall speed to 400mph and determine lowest 1g drag, engine off
	// this will determine sea level best climb speed, which is also best
	// IAS for climb rate
	_TunnelTurnEnginesOff(VehicleData);
	_TunnelResetVehiclePos(VehicleData,0);
	VehicleData->FlapPosition = 0;
	LowestDrag = -1000000;
	LowestSpeed = 0; // stallspeed
	FoundStall = 0;
	for (Speed = 10.0; Speed < 200;Speed += 1.0)
	{
		Result = _TunnelDetermineOneGAoA(VehicleData,&AoA,&YForce,Speed,-2.0,20.0,0.5,0);
		if (Result == 0)
		{
			if (FoundStall == 0)
			{
				FoundStall = 1;
				StallSpeed = Speed;
			}
			if (YForce > LowestDrag)
			{
				LowestSpeed = Speed;
				LowestDrag = YForce;
			}
		}

	}
	fprintf(TunnelFile,"Best Climb Rate Speed (IAS), (m/s) = %lf, (kph) = %lf, (mph) = %lf\n",
				LowestSpeed,LowestSpeed * 2.6,LowestSpeed * 2.237);
	MaxClimbSpeed = LowestSpeed;

	// next determine best climb speed angle
	LowestDrag = -1000000;
	LowestSpeed = 0; // stallspeed
	for (Speed = StallSpeed; Speed < 200;Speed += 1.0)
	{
		Result = _TunnelDetermineOneGAoA(VehicleData,&AoA,&YForce,Speed,-2.0,20.0,0.5,0);
		if (Result == 0)
		{
			if (YForce / Speed > LowestDrag)
			{
				LowestSpeed = Speed;
				LowestDrag = YForce / Speed;
			}
		}

	}
	fprintf(TunnelFile,"Best Climb Angle/Glide Speed (IAS), (m/s) = %lf, (kph) = %lf, (mph) = %lf\n",
				LowestSpeed,LowestSpeed * 2.6,LowestSpeed * 2.237);

	// climb rate chart
	fprintf(TunnelFile,"\n");
	fprintf(TunnelFile,"Climb rate chart, max climb rate\n");
	fprintf(TunnelFile,"Alt (m)  (ft) spd (m/s) (kph) (mph)   Climb Rate (m/s)  (ft/min)\n");

	_TunnelTurnEnginesOnForClimb(VehicleData);
	_TunnelResetVehiclePos(VehicleData,0);
	VehicleData->FlapPosition = 0;

	for (Alt = 0;Alt <=  maxaltindex*ALT_INC /*6000.0*/; Alt += ALT_INC)
	{

		// determine best climb speed
		_TunnelTurnEnginesOnForClimb(VehicleData);
		_TunnelResetVehiclePos(VehicleData,0);
		VehicleData->FlapPosition = 0;
		LowestDrag = -1000000;
		LowestSpeed = 0; // stallspeed
		HighestClimbRate = -10000.0;
		for (Speed = StallSpeed; Speed < 200;Speed += 1.0)
		{
			Result = _TunnelDetermineOneGAoA(VehicleData,&AoA,&YForce,Speed,-2.0,20.0,0.5,Alt);
			if (Result == 0)
			{
				ClimbRate = YForce * Speed * 1.0;
				ClimbRate /= VehicleData->VehicleMass * mlGRAVITY;
				if (ClimbRate > HighestClimbRate)
				{
					HighestClimbRate = ClimbRate;
					LowestSpeed = Speed;
					LowestDrag = YForce;
				}
//				if (YForce > LowestDrag)
//				{
//					LowestSpeed = Speed;
//					LowestDrag = YForce;
//				}
			}

		}

		Speed = LowestSpeed;
		MaxClimbSpeed = Speed;

		fprintf(TunnelFile,"%6.0lf %6.0lf  %4.0lf %4.0lf      %4.0lf ",
				Alt, Alt * 3.281,
				Speed,
				Speed * 3.6,
				Speed * 2.237);

		_TunnelTurnEnginesOnForClimb(VehicleData);
		_TunnelResetVehiclePos(VehicleData,0);
		Result = _TunnelDetermineOneGAoA(VehicleData,&AoA,&YForce,Speed,-2.0,20.0,0.5,Alt);
		if (Result == 0)
		{
			// work = force * distance * time
			ClimbRate = YForce * Speed * 1.0;
			ClimbRate /= VehicleData->VehicleMass * mlGRAVITY;

//			_DisableEngineThrust = 1;
//			Result = _TunnelDetermineOneGAoA(VehicleData,&AoA,&YForce,Speed,-2.0,20.0,0.5,Alt);
//			_DisableEngineThrust = 0;

//			ClimbRate += YForce * Speed * 1.0 / VehicleData->VehicleMass / mlGRAVITY;

			fprintf(TunnelFile,"%5.2lf     %4.0lf\n",ClimbRate, ClimbRate * 60.0 * 3.281);
		}
		else
		{
			fprintf(TunnelFile,"N/A\n");
		}

	}
	Write_Debug("Calculated climb data\n");
	_NoHorizStab = 0;

	return 0;
}
extern FLOAT64 _StickXPos;
static UINT32 _GenerateRollRateAtSeaLevelData(mlVehicleData * VehicleData)
{
FLOAT64 Speed;
UINT32 i;
mlComponent * Component;
FLOAT64VECTOR DummyForce,DummyForceSum;
mlComponentListIterator it;

	// roll rates
	// general theory is this:
	// we set the plane to a particular speed and attitude, then deflect the ailerons
	// the flight model runs for the equivalent of 10 seconds, holding everything
	// constant except for rollvel
	// then we record the angular velocity to determine sustained roll rate
	_NoHorizStab = 0;
	fprintf(TunnelFile,"\nRoll Rates: (Engine off, left hand roll)\n");
	fprintf(TunnelFile,"   m/s     kph     mph      rate (deg/s)\n");
	for (Speed = 50 / 2.237;Speed <= 501.0 / 2.237; Speed += 10.0 / 2.237)
	{
		fprintf(TunnelFile,"%7.1lf %7.1lf %7.1lf",Speed, Speed * 3.6, Speed * 2.237);
		_TunnelTurnEnginesOff(VehicleData);
		_TunnelResetVehiclePos(VehicleData,0);
		VehicleData->FlapPosition = 0;

		_StickXPos = -1.0;

		for (i=0;i<200;i++)
		{
//			_Iterations++;

			// clear sum forces;
			VehicleData->VehicleForce.x = 0;
			VehicleData->VehicleForce.y = 0;
			VehicleData->VehicleForce.z = 0;

			VehicleData->VehicleTorque.Roll = 0;
			VehicleData->VehicleTorque.Pitch = 0;
			VehicleData->VehicleTorque.Yaw = 0;

			VehicleData->VehiclePos.x = 0;
			VehicleData->VehiclePos.y = 0;
			VehicleData->VehiclePos.z = 0;

			VehicleData->VehicleVel.x = 0;
			VehicleData->VehicleVel.y = Speed;
			VehicleData->VehicleVel.z = 0;

			VehicleData->VehicleAtt.Pitch = 0;
			VehicleData->VehicleAtt.Yaw = 0;

			VehicleData->VehicleAttVel.Pitch = 0;
			VehicleData->VehicleAttVel.Yaw = 0;


			_ProcessControlInputs(50,VehicleData);

			// now compute component forces and torques and apply them to the whole
			mlComponentListBegin(VehicleData->ComponentList, &it);
			for(; it.comp; mlComponentListNext(&it))
			{
				Component = it.comp;
				mlUpdateComponent(Component,VehicleData,50,0,&DummyForce,&DummyForceSum);
			}
			// with the new vel and attvel, compute new position and attitude

			_AdjustVelAndRotVel(VehicleData, &VehicleData->VehicleVel,
								&VehicleData->VehicleAttVel,
								&VehicleData->VehicleAcc,
								&VehicleData->VehicleAttAcc,
								&VehicleData->VehicleForce,
								&VehicleData->VehicleTorque,
								VehicleData->VehicleMass,
								&VehicleData->VehicleRotationalInertia,
								50.0,0);

			_AdjustPosAndAccel(&VehicleData->VehiclePos,
							   &VehicleData->VehicleAtt,
							   &VehicleData->VehicleVel,
							   &VehicleData->LastVehicleVel,
							   &VehicleData->VehicleAcc,
							   &VehicleData->LastVehicleAcc,
							   &VehicleData->VehicleAttVel,
							   &VehicleData->VehicleAttAcc,
							   &VehicleData->LastVehicleAttAcc,
							   50.0);

			VehicleData->LastVehicleVel = VehicleData->VehicleVel;
			VehicleData->LastVehicleAcc = VehicleData->VehicleAcc;
			VehicleData->LastVehicleAttAcc = VehicleData->VehicleAttAcc;

		}

		fprintf(TunnelFile,"    %7.1lf",mlRAD_TO_DEG(VehicleData->VehicleAttVel.Roll));
		fprintf(TunnelFile,"\n");
	}
	_StickXPos = 0.0;
	_ProcessControlInputs(5000,VehicleData);

	return 0;
}

static UINT32 _GenerateClCdCurve(mlVehicleData * VehicleData)
{
FLOAT64 Angle;
FLOAT64 Speed;
mlComponent * Component;
mlEngineComponentData * EngineData;
UINT32 i;
FLOAT64VECTOR DummyForce,DummyForceSum;
FLOAT64 Q,Density;
mlComponentListIterator it;

	_NoHorizStab = 1;
	fprintf(TunnelFile,"\nCl/Cd curves: (Prop removed)\n");
	fprintf(TunnelFile,"   AoA     Cl*S    Cd*S     ");

	_DisableEngineThrust = 1;

	_TunnelTurnEnginesOff(VehicleData);
	_TunnelResetVehiclePos(VehicleData,0);
	VehicleData->FlapPosition = 0;

	mlGetAltDensity(&Density,0);
	Speed = 50.0;
	Q = 0.5 * Speed * Speed * Density;

	for (Angle = -4.0; Angle < 20.0; Angle += 0.25)
	{
		for (i=0;i<4;i++)
		{
			VehicleData->VehicleForce.x = 0;
			VehicleData->VehicleForce.y = 0;
			VehicleData->VehicleForce.z = 0;

			VehicleData->VehicleTorque.Roll = 0;
			VehicleData->VehicleTorque.Pitch = 0;
			VehicleData->VehicleTorque.Yaw = 0;

			VehicleData->VehiclePos.x = 0;
			VehicleData->VehiclePos.y = 0;
			VehicleData->VehiclePos.z = 0;

			VehicleData->VehicleVel.x = 0;
			VehicleData->VehicleVel.y = Speed;
			VehicleData->VehicleVel.z = 0;

			VehicleData->VehicleAtt.Pitch = mlDEG_TO_RAD(Angle);
			VehicleData->VehicleAtt.Yaw = 0;
			VehicleData->VehicleAtt.Roll = 0;

			VehicleData->VehicleAttVel.Pitch = 0;
			VehicleData->VehicleAttVel.Yaw = 0;
			VehicleData->VehicleAttVel.Roll = 0;

			mlComponentListBegin(VehicleData->ComponentList, &it);
			for(; it.comp; mlComponentListNext(&it))
			{
				Component = it.comp;

				if (Component->Type == ML_COMPONENT_ENGINE)
				{

					EngineData = (mlEngineComponentData*)Component->SpecialData;
					if (EngineData->SubType == ML_ENGINE_SUBTYPE_PROPELLOR)
					{
//						EngineData->CurrentRpm = 0.0;
					}
				}

				mlUpdateComponent(Component,VehicleData,50,0,&DummyForce,&DummyForceSum);
			}

		}

		fprintf(TunnelFile,"%6.2lf %6.3lf %6.3lf\n",Angle,
				(-VehicleData->VehicleForce.z + VehicleData->VehicleMass * 9.8) / Q / 23.4,
						-VehicleData->VehicleForce.y / Q / 23.4);
	}

	_DisableEngineThrust = 0;
	_NoHorizStab = 0;

	fprintf(TunnelFile,"\n");

	return 0;
}

static UINT32 _CloseWindTunnelFile(mlVehicleData * VehicleData)
{
	fclose(TunnelFile);

	return 0;
}

static UINT32 _CleanUp(mlVehicleData * VehicleData)
{
	VehicleData->CollisionsOff = 0;
	mlResetVehiclePos(VehicleData);

	return 0;
}

#if(0)
static UINT32 _SetEnginesToMaxPower(mlVehicleData * VehicleData)
{


	return 0;
}

static UINT32 _SetPropsToAbsorbMaxPower(mlVehicleData * VehicleData)
{


	return 0;
}
#endif

static UINT32 _TunnelTurnEnginesOn(mlVehicleData * VehicleData)
{
mlComponent * Component;
mlEngineComponentData * EngineComponentData;
mlEngineComponentData * PropData;
mlComponentListIterator it;
	VehicleData->ThrottlePos = 1.0;

	mlComponentListBegin(VehicleData->ComponentList, &it);
	for(; it.comp; mlComponentListNext(&it))
	{
		Component = it.comp;

		if (Component->Type == ML_COMPONENT_ENGINE)
		{
			EngineComponentData = (mlEngineComponentData*)Component->SpecialData;
			if (EngineComponentData->SubType == ML_ENGINE_SUBTYPE_ENGINE)
			{

				EngineComponentData->EngineMode = ML_ENGINE_MODE_MIL;

				PropData = (mlEngineComponentData*)EngineComponentData->PropellorComponent->SpecialData;
//				PropData->CurrentRpm = EngineComponentData->EngineData->MilRpm *
//										EngineComponentData->EngineData->PropellerGearRatio;
				PropData->CurrentRpm = EngineComponentData->EngineData->MilRpm;
				PropData->LastPropAngle = PropData->CurrentPropAngle = 45.0;

				EngineComponentData->EngineCaught = 1;
			}
		}

	}

	_EnginesAreOn = 1;

	return 0;
}

static UINT32 _TunnelTurnEnginesOff(mlVehicleData * VehicleData)
{
mlComponent * Component;
mlEngineComponentData * EngineComponentData;
mlEngineComponentData * PropData;
mlComponentListIterator it;

	VehicleData->ThrottlePos = 1.0;


	mlComponentListBegin(VehicleData->ComponentList, &it);
	for(; it.comp; mlComponentListNext(&it))
	{
		Component = it.comp;

		if (Component->Type == ML_COMPONENT_ENGINE)
		{
			EngineComponentData = (mlEngineComponentData*)Component->SpecialData;
			if (EngineComponentData->SubType == ML_ENGINE_SUBTYPE_ENGINE)
			{

				EngineComponentData->EngineMode = ML_ENGINE_MODE_MIL;

				PropData = (mlEngineComponentData*)EngineComponentData->PropellorComponent->SpecialData;
//				PropData->CurrentRpm = EngineComponentData->EngineData->MilRpm *
//										EngineComponentData->EngineData->PropellerGearRatio;
				PropData->CurrentRpm = 0;
				PropData->LastPropAngle = PropData->CurrentPropAngle =
								PropData->PropellorMaxAngle;

				EngineComponentData->EngineCaught = 0;
			}
		}

	}

	_EnginesAreOn = 1;

	return 0;
}

UINT32 _DetermineFirstEngineHp(mlVehicleData * VehicleData,FLOAT64 * Hp,FLOAT64 Alt)
{
mlComponent * Component;
mlEngineComponentData * EngineComponentData;
mlEngineComponentData * PropData;
FLOAT64VECTOR DummyForce;
FLOAT64ATT DummyTorque;
mlComponentListIterator it;
	*Hp = 0.0;

	mlComponentListBegin(VehicleData->ComponentList, &it);
	for(; it.comp; mlComponentListNext(&it))
	{
		Component = it.comp;

		if (Component->Type == ML_COMPONENT_ENGINE)
		{
			EngineComponentData = (mlEngineComponentData*)Component->SpecialData;
			if (EngineComponentData->SubType == ML_ENGINE_SUBTYPE_ENGINE)
			{
				PropData = (mlEngineComponentData*)EngineComponentData->PropellorComponent->SpecialData;

				PropData->CurrentRpm = EngineComponentData->EngineData->MilRpm;

				mlUpdateEngine(&DummyForce,
							   &DummyTorque,
							   EngineComponentData,
							   Component,
							   Alt,
							   &VehicleData->VehicleVel,
							   0.040,
							   1.0,
							   1,
							   0,
							   0,
							   VehicleData->VehicleObjectData,
							   (void*)VehicleData->VehicleMasterPtr,
							   1.0f);

				*Hp = PropData->CurrentPower / 745.7;

				break;
			}
		}
	}


	return 0;
}

static UINT32 _TunnelResetVehiclePos(mlVehicleData * VehicleData,FLOAT64 Alt)
{

	VehicleData->VehicleAtt.Roll = VehicleData->VehicleAtt.Pitch = VehicleData->VehicleAtt.Yaw = 0;
	VehicleData->VehicleAttVel.Roll = VehicleData->VehicleAttVel.Pitch = VehicleData->VehicleAttVel.Yaw = 0;
	VehicleData->VehicleForce.x = VehicleData->VehicleForce.y = VehicleData->VehicleForce.z = 0;
	VehicleData->VehicleTorque.Roll = VehicleData->VehicleTorque.Pitch = VehicleData->VehicleTorque.Yaw = 0;
	VehicleData->VehicleVel.x = VehicleData->VehicleVel.y = VehicleData->VehicleVel.z = 0;
	VehicleData->VehiclePos.x = VehicleData->VehiclePos.y = 0;
	VehicleData->VehiclePos.z = Alt;

	return 0;
}

static UINT32 _TunnelDetermineOneGAoA(mlVehicleData * VehicleData,
									  FLOAT64 * AoA,
									  FLOAT64 * YForce,
									  FLOAT64 Speed,
									  FLOAT64 MinAoA,
									  FLOAT64 MaxAoA,
									  FLOAT64 Step,
									  FLOAT64 Alt)
{
FLOAT64 CurrentAoA;
UINT32 i;
mlComponent * Component;
FLOAT64 Min,Max;
FLOAT64VECTOR DummyForce,DummyForceSum;
mlComponentListIterator it;

//	_SetEnginePitchResponseToInfinity(VehicleData);

	mlCalcVehicleMass(VehicleData);
	mlCalcVehicleCenterOfMass(VehicleData);
	mlSetVehicleComponentsRelativePos(VehicleData);
	mlCalcVehicleRotationalInertia(VehicleData);

	VehicleData->ElevatorPos = 0.0;
	VehicleData->AileronPos = 0.0;
	VehicleData->RudderPos = 0.0;


	if (MinAoA < MaxAoA)
	{
		Min = MinAoA;
		Max = MaxAoA;
	}
	else
	{
		Min = MaxAoA;
		Max = MinAoA;
	}

	for (CurrentAoA = Min;CurrentAoA <= Max;CurrentAoA += Step)
	{
		// set attitude and position
		_TunnelResetVehiclePos(VehicleData,Alt);

		VehicleData->VehicleAtt.Pitch = mlDEG_TO_RAD(CurrentAoA);
		VehicleData->VehicleAtt.Roll = VehicleData->VehicleAtt.Yaw = 0;
		VehicleData->VehicleVel.y = Speed;

		_SetPropsToCorrectPitchForSpeed(VehicleData,Speed,Alt);

//		for (i=0;i<15;i++)
		for (i=0;i<5;i++)
		{
			_iterations++;

			// clear sum forces;
			VehicleData->VehicleForce.x = 0;
			VehicleData->VehicleForce.y = 0;
			VehicleData->VehicleForce.z = 0;

			VehicleData->VehicleTorque.Roll = 0;
			VehicleData->VehicleTorque.Pitch = 0;
			VehicleData->VehicleTorque.Yaw = 0;

			VehicleData->VehicleAtt.Pitch = mlDEG_TO_RAD(CurrentAoA);
			VehicleData->VehicleAtt.Roll = VehicleData->VehicleAtt.Yaw = 0;
			VehicleData->VehicleVel.y = Speed;
			VehicleData->VehicleVel.x = VehicleData->VehicleVel.z = 0.0;

			// now compute component forces and torques and apply them to the whole
			mlComponentListBegin(VehicleData->ComponentList, &it);
			for(; it.comp; mlComponentListNext(&it))
			{
				Component = it.comp;

				mlUpdateComponent(Component,VehicleData,5,0,&DummyForce,&DummyForceSum);
			}
			// with the new vel and attvel, compute new position and attitude

			if (_EnginesAreOn == 1)
			{
				_AddInPrecalculatedThrust(VehicleData,Alt,Speed);
			}

			_AdjustVelAndRotVel(VehicleData, &VehicleData->VehicleVel,
								&VehicleData->VehicleAttVel,
								&VehicleData->VehicleAcc,
								&VehicleData->VehicleAttAcc,
								&VehicleData->VehicleForce,
								&VehicleData->VehicleTorque,
								VehicleData->VehicleMass,
								&VehicleData->VehicleRotationalInertia,
								5,0);

			_AdjustPosAndAccel(&VehicleData->VehiclePos,
							   &VehicleData->VehicleAtt,
							   &VehicleData->VehicleVel,
							   &VehicleData->LastVehicleVel,
							   &VehicleData->VehicleAcc,
							   &VehicleData->LastVehicleAcc,
							   &VehicleData->VehicleAttVel,
							   &VehicleData->VehicleAttAcc,
							   &VehicleData->LastVehicleAttAcc,
							   5);

			VehicleData->LastVehicleVel = VehicleData->VehicleVel;
			VehicleData->LastVehicleAcc = VehicleData->VehicleAcc;
			VehicleData->LastVehicleAttAcc = VehicleData->VehicleAttAcc;
		}

		if (VehicleData->VehicleForce.z < 0)
		{
			break;
		}
	}

	if (CurrentAoA > Max)
	{
		*YForce = VehicleData->VehicleForce.y;
		*AoA = CurrentAoA;
		return 1;
	}
	else
	{
		*YForce = VehicleData->VehicleForce.y;
		*AoA = CurrentAoA;
		return 0;
	}

	return 0;
}

static UINT32 _AddInPrecalculatedThrust(mlVehicleData * VehicleData,FLOAT64 Alt,FLOAT64 Speed)
{
UINT32 AltIndex1,AltIndex2;
UINT32 SpeedIndex1,SpeedIndex2;
FLOAT64 AltDelta,SpeedDelta;
FLOAT64 ThrustAlt1,ThrustAlt2;
FLOAT64 ThrustSpeed1,ThrustSpeed2;
FLOAT64 Thrust;

	AltIndex1 = (UINT32)(Alt / ALT_INC);
	if (AltIndex1 > 39)
	{
		AltIndex1 = 39;
	}
	AltIndex2 = AltIndex1+1;

	SpeedIndex1 = (UINT32)(Speed / SPEED_INC);
	if (SpeedIndex1 > 39)
	{
		SpeedIndex1 = 39;
	}
	SpeedIndex2 = SpeedIndex1+1;

	AltDelta = Alt - AltIndex1 * ALT_INC;
	SpeedDelta = Speed - SpeedIndex1 * SPEED_INC;

	ThrustAlt1 = _ThrustCurve[AltIndex1][SpeedIndex1];
	ThrustAlt2 = _ThrustCurve[AltIndex2][SpeedIndex1];
	mlInterpolate(&ThrustSpeed1,ThrustAlt1,ThrustAlt2,AltDelta / ALT_INC);

	ThrustAlt1 = _ThrustCurve[AltIndex1][SpeedIndex2];
	ThrustAlt2 = _ThrustCurve[AltIndex2][SpeedIndex2];
	mlInterpolate(&ThrustSpeed2,ThrustAlt1,ThrustAlt2,AltDelta / ALT_INC);

	mlInterpolate(&Thrust,ThrustSpeed1,ThrustSpeed2,SpeedDelta / SPEED_INC);

	VehicleData->VehicleForce.y += Thrust * _NumEngines;

	return 0;
}

static UINT32 _SetPropsToCorrectPitchForSpeed(mlVehicleData * VehicleData,FLOAT64 Speed,FLOAT64 Alt)
{
UINT32 j;
mlComponent * Component;
mlEngineComponentData * EngineComponentData;
mlEngineComponentData * PropData;
FLOAT64VECTOR DummyForce;
FLOAT64ATT DummyTorque,TempAtt;
FLOAT64VECTOR TempVel;
FLOAT64 Upper,Lower;
FLOAT64 TempAngle,TempAngle1;
FLOAT64 BestAngle,BestAngle1;
FLOAT64 Delta,BestDelta;
mlComponentListIterator it;

	VehicleData->ThrottlePos = 1.0;

	mlComponentListBegin(VehicleData->ComponentList, &it);
	for(; it.comp; mlComponentListNext(&it))
	{
		Component = it.comp;

		if (Component->Type == ML_COMPONENT_ENGINE)
		{
			EngineComponentData = (mlEngineComponentData*)Component->SpecialData;
			if (EngineComponentData->SubType == ML_ENGINE_SUBTYPE_ENGINE)
			{

			EngineComponentData->EngineMode = (climbtest) ? ML_ENGINE_MODE_CLIMB : ML_ENGINE_MODE_MIL;

				PropData = (mlEngineComponentData*)EngineComponentData->PropellorComponent->SpecialData;
				if (EngineComponentData->EngineCaught == 0)
				{
					PropData->CurrentPropAngle = PropData->PropellorMaxAngle;
					if (PropData->CurrentPropAngle > 90.0)
					{
						PropData->CurrentPropAngle = 90.0;
					}
					continue;
				}

				PropData->CurrentRpm = EngineComponentData->EngineData->MilRpm;

				TempVel.x = TempVel.z = 0;
				TempVel.y = Speed;
				mlUpdateEngine(&DummyForce,
							   &DummyTorque,
							   EngineComponentData,
							   Component,
							   Alt,
							   &TempVel,
							   0.040,
							   1.0,
							   1,
							   0,
							   0,
							   VehicleData->VehicleObjectData,
							   (void*)VehicleData->VehicleMasterPtr,
							   1.0f);

				PropData->CurrentPropAngle = (PropData->PropellorMaxAngle + PropData->PropellorMinAngle) / 2.0;
				TempAtt.Roll = TempAtt.Pitch = TempAtt.Yaw = 0.0;

				PropData->CurrentPropAngle = 0.0f;
				mlUpdatePropellor(&DummyForce,
								  &DummyTorque,
								  PropData,
								  Component,
								  Alt,
								  &TempVel,
								  0.040,
								  &TempAtt,
								  0,
								  1,
								  VehicleData->VehicleObjectData);

				Upper = mlRAD_TO_DEG(PropData->LastPropAoA) + 15.0;
				Lower = mlRAD_TO_DEG(PropData->LastPropAoA) - 5.0;

				PropData->CurrentPropAngle = (Upper + Lower) / 2.0;
				mlUpdatePropellor(&DummyForce,
								  &DummyTorque,
								  PropData,
								  Component,
								  Alt,
								  &TempVel,
								  0.040,
								  &TempAtt,
								  0,
								  1,
								  VehicleData->VehicleObjectData);

				TempAngle1 = PropData->LastWindAoA;

				BestDelta = 10000000000.0;
				BestAngle = 0.0;
				for (TempAngle = TempAngle1 - 5.0;TempAngle < PropData->LastWindAoA + 15.0;
					 TempAngle += 1.0)
				 {
					PropData->CurrentPropAngle = TempAngle;

					for (j=0;j<5;j++)
					{
						mlUpdatePropellor(&DummyForce,
										  &DummyTorque,
										  PropData,
										  Component,
										  Alt,
										  &TempVel,
										  0.040,
										  &TempAtt,
										  0,
										  1,
										  VehicleData->VehicleObjectData);
					}

					Delta = fabs(PropData->CurrentPower / (PropData->CurrentRpm / 60) / PI / 2 -
							PropData->PropCounterTorque*EngineComponentData->EngineData->PropellerGearRatio);

					if (Delta < BestDelta)
					{
						BestDelta = Delta;
						BestAngle = TempAngle;
					}
				 }

				BestAngle1 = BestAngle - 1.0;
				BestDelta = 1000000000000.0;
				for (TempAngle = BestAngle - 1.0; TempAngle < BestAngle + 1.0;TempAngle += 0.1)
				{
					PropData->CurrentPropAngle = TempAngle;

					for (j=0;j<5;j++)
					{
						mlUpdatePropellor(&DummyForce,
										  &DummyTorque,
										  PropData,
										  Component,
										  Alt,
										  &TempVel,
										  0.040,
										  &TempAtt,
										  0,
										  1,
										  VehicleData->VehicleObjectData);
					}

					Delta = fabs(PropData->CurrentPower / (PropData->CurrentRpm / 60) / PI / 2 -
							PropData->PropCounterTorque*EngineComponentData->EngineData->PropellerGearRatio);

					if (Delta < BestDelta)
					{
						BestDelta = Delta;
						BestAngle1 = TempAngle;
					}
				}

				PropData->CurrentPropAngle = BestAngle1;
			}
		}

	}
	return 0;
}

static UINT32 _CountEngines(mlVehicleData * VehicleData)
{
mlComponent * Component;
mlEngineComponentData * EngineComponentData;
mlComponentListIterator it;

	_NumEngines = 0;

	mlComponentListBegin(VehicleData->ComponentList, &it);
	for(; it.comp; mlComponentListNext(&it))
	{
		Component = it.comp;
		if (Component->Type == ML_COMPONENT_ENGINE)
		{
			EngineComponentData = (mlEngineComponentData*)Component->SpecialData;
			if (EngineComponentData->SubType == ML_ENGINE_SUBTYPE_ENGINE)
			{
				_NumEngines++;
			}
		}
	}

	return 0;
}

static UINT32 _TunnelTurnEnginesOnForClimb(mlVehicleData * VehicleData)
{
mlComponent * Component;
mlEngineComponentData * EngineComponentData;
mlEngineComponentData * PropData;
mlComponentListIterator it;
	VehicleData->ThrottlePos = 1.0;

	mlComponentListBegin(VehicleData->ComponentList, &it);
	for(; it.comp; mlComponentListNext(&it))
	{
		Component = it.comp;

		if (Component->Type == ML_COMPONENT_ENGINE)
		{
			EngineComponentData = (mlEngineComponentData*)Component->SpecialData;
			EngineComponentData->EngineCaught = 1;

			if (EngineComponentData->SubType == ML_ENGINE_SUBTYPE_ENGINE)
			{
				if(EngineComponentData->PropellorComponent)
				{
					PropData = (mlEngineComponentData*)EngineComponentData->PropellorComponent->SpecialData;
					if(PropData)
					{
						PropData->LastPropAngle = PropData->CurrentPropAngle = 45.0;

						if(EngineComponentData->EngineData->ClimbRpm > 0)
						{
							EngineComponentData->EngineMode = ML_ENGINE_MODE_CLIMB;
							PropData->CurrentRpm = EngineComponentData->EngineData->ClimbRpm;
						}else
						{
							EngineComponentData->EngineMode = ML_ENGINE_MODE_MIL;
							PropData->CurrentRpm = EngineComponentData->EngineData->MilRpm;

						}
					}
				}
			}
		}

	}

	_EnginesAreOn = 1;

	return 0;
}


#endif // BETA_RELEASE
