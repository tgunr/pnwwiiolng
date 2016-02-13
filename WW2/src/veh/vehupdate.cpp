#include <syscfg.h> // Path is project specific.

#include "pi/pi.h"
#include "include/vars.h"

#include "veh/vehupdate.h"
#include "veh/vehmain.h"
#include "veh/vehanim.h"
#include "gr2/gr2Anim.h"
#include "ml/mlobject.h"
#include "ml/mlutility.h"
#include "ml/mlCompon.h"
#include "math/mathvect.h"
#include "math/math3d.h"
#include "math/matheul.h"
#include "math/mathnum.h"
#include "veh/vehdecal.h"
#include "client/GameClient.h"
#include "mm/mmSpawnTypes.h"
#include "weap/weapList.h"
#include "wwii/wwiiHCUnits.h"
#include "map/mapBrigadeSummary.h"
#include "comm/commClientMessages.h"
#include "wwii/wwiiClientMessages.h"
#include "dip.h"
#include "ogrp/ogrptype.h"
#include "comm/commPackers.h"
#include "updt/updtPackers.h"
#include "updt/updtTurrets.h"
#include "trn/trnmain.h"
#include "trn/trncd.h"
#include "trn/trnop.h"

#include "fx/fxMgr.h"
#include "fx/fxSnd.h"

#include <math.h>

#include "locl/loclMain.h"

#include "fp/fptypes.h"
#include "fp/fpMain.h"
#include "pers/persmain.h"
#include "fp/fpFPS.h"
#include "math/mathUnitConversion.h"
#include "fio/fio.h"
#include "chat/chatHandle.h"
#include "pers/pershud.h"

#include "veh/vehload.h"
#include "ob/obMgr.h"
#include "fp/fpTPSdata.h"
#include "miss/missionDetail.h"
#include "cfg/cfgPrefs.h"
#include "scen/ScenarioManager.h"

#define MAX_VIS_VEHICLES 128

#define TIMEOUT_MS		300000	// 30000	// 30 sec to 5 minutes
#define TIMEOUT_MS_HIDE	60000	// 15000	// 15 sec to 1 minute

#define INFANTRY_UPDATE_FACTOR 15

#define NEW_INF_PREDICTOR
//#define SIMULATE_LOW_UPDATE_RATE

#ifdef GR2_BLEND_EDITOR
#undef NEW_INF_PREDICTOR
#endif

bool _newpredictor = true;
int _simlowupdate = 0;

#ifdef BETA_RELEASE
UINT32 vehUpdatesSent = 0;
UINT32 vehMcrewUpdatesRecv = 0;
UINT32 vehNonMcrewUpdatesRecv = 0;
UINT32 vehMcommanderUpdatesIgn = 0;
UINT32 worldUpdatesReceived = 0;
UINT32 duplicateUpdatesIgnored = 0;
FLOAT64VECTOR vehMultiCrewAccumPerc = {0.0, 0.0, 0.0};
#endif

typedef struct
{
	const char* name;
	UINT32 Rank;
} IconRank;

static const IconRank _SpecialRanks[] =
{
#if !defined(CHINA) && !defined(HAPPYOO)
	{"killer", 		KILLER_RANK },
	{"martini", 	MARTINI_RANK },
	{"toto", 		TOTO_RANK },
	{"frytiger", 	FT_RANK },
	{"snail", 		SNAIL_RANK },
	{"lindir", 		LINDIR_RANK },
#endif
	{nullptr, 0}
};

vehVehicle* _UpdateVehicleList[MAX_VIS_VEHICLES];

static UINT32 _SendUpdate(vehVehicle&, mlVehicleData&);
static UINT32 _ExtractData(vehVehiclePositionData* ,vehVehicle* ,const VEHICLE_UPDATE_HEADER*);
static UINT32 _ResolveAverages(vehVehicle* Vehicle);
static UINT32 _InterpolateAttitudes(FLOAT64ATT * RetAtt,FLOAT64ATT * InAtt1,FLOAT64ATT * InAtt2,FLOAT64 Delta);
#if(0)
static UINT32 _InterpolateVectorsByAngles(piVECTOR* V3,piVECTOR* V1,piVECTOR* V2,FLOAT32 Delta);
#endif
static LOGICAL _GetSoundDistance(FLOAT32* pMin, FLOAT32* pMax, UINT32* pType);

static void vehUpdateRideChanged(vehVehicle*, UINT32 newRideVehicleIdx);
static void _processBuildingDamage (vehVehicle& Vehicle);
static void _playWeaponFireSound(SCHAR* sound, OBJECT* o, FLOAT64VECTOR* pos, UINT32 type);

UINT32 vehupdateCreatePlayerControlledVehicle(SINT32 VehicleNum,
											  SINT32 VehicleIdNum,
											  SINT32 Index,
											  SINT32 persVehicleIndex,
											  SINT32 MultiCrewPos)
{
	vehVehicle* Vehicle;

	// create the new vehicle
	Vehicle = vehFindVehicle(Index);
	if (Vehicle != nullptr)
		vehKillVehicle(Vehicle);

	if (vehCreateNewVehicle(&Vehicle,Index, VehicleNum) == 0)
		return 1;

	Vehicle->VehicleNum = VehicleIdNum;
	Vehicle->VehicleHostIndex = Index;
	Vehicle->persVehicleIndex = persVehicleIndex;

    // First person gets decal.
    Vehicle->persDecal = PlayerClient()->SpawnNotify().decal;

	// set bits from the spawn notify packet
	Vehicle->IsCommander = PlayerClient()->SpawnNotify().isHighCommander;
	Vehicle->IsMissionLeader = PlayerClient()->SpawnNotify().isMissionLeader;
	Vehicle->IsBuilder = PlayerClient()->SpawnNotify().isBuilder;
	Vehicle->persDecal = PlayerClient()->SpawnNotify().decal;
    Vehicle->unitDecal = PlayerClient()->BrigadeSelected().m_detail.brigadeID();

	*Vehicle->SquadHandle = 0;

	if(Vehicle->VehicleType->Type != VEH_TYPE_PERSON)
	{
		vehloadCreateVehicleInstance(Vehicle, TRUE);
	}
	else
	{
		mlobjectLoadVehicleObject(&Vehicle->VehicleObjectData,
								Vehicle->VehicleType->Shape,
								Vehicle->VehicleType->NumDamageObjects,
								Vehicle,
								0,
								(UCHAR*)(&Vehicle->VehicleType->DamageObjects[0][0]),
								TRUE);
	}

	vehSetVehicleToUserControlled(Vehicle,MultiCrewPos);
	if (Vehicle->VehicleObjectData != nullptr)
	{
		mlSetModelSpecialPoints(Vehicle->ModelData,Vehicle->VehicleObjectData);

		// set user controlled vehicles' objects to user controlled
		obmainSetFlag2InThisTree(Vehicle->VehicleObjectData->PlaneObject,OBJECT_USERVEHICLE);

		////picoCreateImmutableObjectTree(Vehicle->VehicleObjectData->PlaneObject);//// pico____
	}

	if (MultiCrewPos != 0)
	{
		vehupdateCreateVehicleLoadObjectPortion(Vehicle,1);
	}

	return 0;
}

UINT32 vehupdateCreateVehicle(vehVehicle** Vehicle, SINT32 vehicleType,SINT32 VehicleIdNum, SINT32 Index, ChatHandle ch)
{
	// create the new vehicle; 0 = failed, 1 = success + load it, 2 = success, already loaded
	UINT32 result = vehCreateNewVehicle(Vehicle,Index, vehicleType, ch);

	if (result == 0)
	{
		*Vehicle = nullptr;
		return 1;
	}

	(*Vehicle)->VehicleNum = VehicleIdNum;
	(*Vehicle)->VehicleHostIndex = Index;

	if(result == 1)
		(*Vehicle)->LoadVehicle = 1;

	return 0;
}

void vehupdateInitGhostFiring(vehVehicle*Vehicle)
{
	OBJECT * MountObject;
	OBJECT * TempObj;
	UINT32 i,j;

    vehLoadout* const Loadout = &Vehicle->VehicleType->Loadout[Vehicle->LoadoutNum];

	// set up weapon stuff for ghost firing
	for(i=0;i<VEH_MAX_MOUNTS;i++)
	{
		MountObject = nullptr;
		ogrpFindSubObjectByName(&MountObject,
								Vehicle->VehicleObjectData->PlaneObject,
								(char*)Loadout->MountObject[i]);
		if (MountObject == nullptr)
		{
			if (Vehicle->VehicleType->Type == VEH_TYPE_PERSON &&
				Loadout->WeaponTypes[i][0] != nullptr)
			{
				weapInitializeWeapon(&Vehicle->Weapons[i][0],Loadout->WeaponTypes[i][0]);
				Vehicle->Weapons[i][0].NumRoundsAmmoLeft[0] = -1;
			}
			continue;
		}
		for(j=0;j<VEH_MAX_WEAPONS_PER_MOUNT;j++)
		{
			if (Loadout->WeaponTypes[i][j] != nullptr)
			{
				weapInitializeWeapon(&Vehicle->Weapons[i][j],Loadout->WeaponTypes[i][j]);

				// find fire point
				TempObj = nullptr;
				ogrpFindSubObjectByDOFVarAndSubVar(
						&TempObj,
						MountObject,
						ML_DOF_CONNECT,
						j);

				if (TempObj != nullptr)
				{
					ogrpFindSubObjectByDOFVar(
							&Vehicle->VehicleObjectData->WeaponFirePoints[i][j],
							TempObj,
							ML_DOF_FIREPOINT);

					if (Vehicle->VehicleObjectData->WeaponFirePoints[i][j] != nullptr)
					{
						Vehicle->VehicleObjectData->WeaponFirePoints[i][j]->flag2 &= ~OBJECT_DOF_USECHILDMATRIX;
						Vehicle->Weapons[i][j].NumRoundsAmmoLeft[0] = -1;
					}
				}
			}
		}
	}
}

UINT32 vehupdateCreateVehicleLoadObjectPortion(vehVehicle* Vehicle, UINT32 IsMultiCrewVehicle)
{
	logTimerBeginMacro();

	if (IsMultiCrewVehicle == 0)
	{

		mlobjectLoadVehicleObject(&Vehicle->VehicleObjectData,
									Vehicle->VehicleType->Shape,
									Vehicle->VehicleType->NumDamageObjects,
									Vehicle,
									0,
									(UCHAR*)(&(Vehicle->VehicleType->DamageObjects[0][0])),
									FALSE);
	}

	if (Vehicle->VehicleObjectData != nullptr)
	{
		if (IsMultiCrewVehicle == 0)
		{
			mlSetModelSpecialPoints(Vehicle->ModelData,Vehicle->VehicleObjectData);
		}

        vehupdateInitGhostFiring(Vehicle);
	}

	logTimerEndMacro();

	return 0;
}

UINT32 vehupdateInitVehicle(vehVehicle*Vehicle)
{

	if (Vehicle == nullptr)
	{
		return 0;
	}

	if (Vehicle->ModelData != nullptr)
	{
		mlResetVehiclePos(Vehicle->ModelData);
	}

#if defined(TAFAT)
    // Leave static for debugging if necessary
    static float pitch = PI/4;

    // Assume for now that only a buzzard or 'special' vehicle can
    // be spawned.
    Vehicle->ModelData->VehicleRealAtt.Roll = 0;
    Vehicle->ModelData->VehicleRealAtt.Yaw = 0;
    Vehicle->ModelData->VehicleRealAtt.Pitch = pitch;
#endif

	return 0;
}


UINT32 vehupdateUpdate(UINT32 ForceSend)
{
SINT32 TempNum;
vehVehicle* Vehicle;
static SINT32 _LastUpdateTime = 0; // added specific initialization 5/28/01 hoss

	// send out updates for each vehicle at the update rate

	if(_LastUpdateTime == 0)
	{
		_LastUpdateTime = frametime;
	}

	TempNum = _LastUpdateTime + FE_UPDATE_INTERVAL;

	if (frametime - TempNum > 10000000)
	{
		_LastUpdateTime = frametime;
	}

	if (frametime - TempNum < -10000000)
	{
		_LastUpdateTime = frametime;
	}

	if (frametime - TempNum > 0 || ForceSend != 0)
	{

#ifdef BETA_RELEASE
		static SINT32 _LastReportTime = 0; // added specific initialization 5/28/01 hoss
										   /* Report our tracking numbers every second */
		SINT32 Periodic = _LastReportTime + 1000;
		if (frametime - Periodic > 0)
		{
			dbugLog(_FLI, "WUR: %u, DUI: %u, US: %u, MCUR: %u, MNMCUR: %u, MCUI: %u x:%f y:%f z:%f",
				worldUpdatesReceived, duplicateUpdatesIgnored, vehUpdatesSent, vehMcrewUpdatesRecv, 
				vehMultiCrewAccumPerc.x, vehMultiCrewAccumPerc.y, vehMultiCrewAccumPerc.z);
			vehUpdatesSent = vehMcrewUpdatesRecv = vehNonMcrewUpdatesRecv = vehMcommanderUpdatesIgn = 0;
			worldUpdatesReceived = duplicateUpdatesIgnored = 0;
			vehMultiCrewAccumPerc.x = vehMultiCrewAccumPerc.y = vehMultiCrewAccumPerc.z = 0.0;
			_LastReportTime = frametime;
		}
#endif

		_LastUpdateTime = frametime;

		vehGetVehicleHead(&Vehicle);

		for (;Vehicle != nullptr;Vehicle = Vehicle->getNext())
		{
			// find the player's vehicle, if it exists
			if (Vehicle->ModelData == nullptr)
				continue;

			// if player is in the process of exitting the vehicle, stop
			// sending updates

			if (Vehicle->VehicleDeadTime != 0 && ForceSend == 0)
				continue;

			if (Vehicle->ModelData->MultiCrewPos > 1)
			{
				Vehicle->PosData.NumSamples = 0;
			}
			else
			{
				_ResolveAverages(Vehicle);
			}

			_SendUpdate(*Vehicle, *Vehicle->ModelData);
			break;
		}
	}

	return 0;
}

UINT32 vehUpdatePositionInfo(vehVehicle* Vehicle)
{
	vehVehiclePositionData* PosData;
	mlVehicleData* Data;

	if (Vehicle == nullptr) return 0;
	if (Vehicle->ModelData == nullptr) return 0;
	if (Vehicle->ModelData->MultiCrewPos > 1) return 0;

	PosData = &Vehicle->PosData;
	Data = Vehicle->ModelData;

	PosData->Vel.x = Data->VehicleRealVel.x;
	PosData->Vel.y = Data->VehicleRealVel.y;
	PosData->Vel.z = Data->VehicleRealVel.z;

	PosData->Acc.x = Data->VehicleAcc.x;
	PosData->Acc.y = Data->VehicleAcc.y;
	PosData->Acc.z = Data->VehicleAcc.z;

	PosData->AttVel.Pitch = Data->VehicleRealAttVel.Pitch;
	PosData->AttVel.Roll  = Data->VehicleRealAttVel.Roll;
	PosData->AttVel.Yaw   = Data->VehicleRealAttVel.Yaw;

	PosData->Att.Pitch = (FLOAT32)Data->VehicleRealAtt.Pitch;
	PosData->Att.Roll  = (FLOAT32)Data->VehicleRealAtt.Roll;
	PosData->Att.Yaw   = (FLOAT32)Data->VehicleRealAtt.Yaw;

	if (PosData->NumSamples == 0)
	{
		PosData->VelSum.x = PosData->VelSum.y = PosData->VelSum.z = 0;
		PosData->AccSum.x = PosData->AccSum.y = PosData->AccSum.z = 0;
		PosData->AttVelSum.Pitch = PosData->AttVelSum.Roll = PosData->AttVelSum.Yaw = 0;
	}

	PosData->VelSum.x += Data->VehicleRealVel.x;
	PosData->VelSum.y += Data->VehicleRealVel.y;
	PosData->VelSum.z += Data->VehicleRealVel.z;

	PosData->AccSum.x += Data->VehicleAcc.x;
	PosData->AccSum.y += Data->VehicleAcc.y;
	PosData->AccSum.z += Data->VehicleAcc.z;

	PosData->AttVelSum.Pitch += Data->VehicleRealAttVel.Pitch;
	PosData->AttVelSum.Roll  += Data->VehicleRealAttVel.Roll;
	PosData->AttVelSum.Yaw   += Data->VehicleRealAttVel.Yaw;

	PosData->NumSamples++;

	return 0;
}

static UINT32 _ResolveAverages(vehVehicle* Vehicle)
{
	vehVehiclePositionData* PosData;

	if (Vehicle == nullptr) return 0;
	if (Vehicle->ModelData == nullptr) return 0;

	PosData = &Vehicle->PosData;

	if (PosData->NumSamples == 0) return 0;

	PosData->Vel.x = PosData->VelSum.x / PosData->NumSamples;
	PosData->Vel.y = PosData->VelSum.y / PosData->NumSamples;
	PosData->Vel.z = PosData->VelSum.z / PosData->NumSamples;

	PosData->Acc.x = PosData->AccSum.x / PosData->NumSamples;
	PosData->Acc.y = PosData->AccSum.y / PosData->NumSamples;
	PosData->Acc.z = PosData->AccSum.z / PosData->NumSamples;

	PosData->AttVel.Pitch = PosData->AttVelSum.Pitch / PosData->NumSamples;
	PosData->AttVel.Roll  = PosData->AttVelSum.Roll  / PosData->NumSamples;
	PosData->AttVel.Yaw   = PosData->AttVelSum.Yaw   / PosData->NumSamples;

	PosData->NumSamples = 0;

	return 0;
}


static UINT32 _SendUpdate(vehVehicle& Vehicle, mlVehicleData& Data)
{
	static SHARED_VEHICLE_INFO VehicleInfo;
	static VEHICLE_UPDATE_HEADER VehicleHdr;
	FLOAT64VECTOR TempVec;
	FLOAT64ATT TempAtt;
	SINT32 i;
	turret_t TurretElev[ML_MAX_CREW_POSITIONS];
	turret_t TurretTrav[ML_MAX_CREW_POSITIONS];
	UINT32 TurretMask;
	UINT32 NumTurrets;
	SINT32 TurretNum;
	vehVehicle* RideVehicle = nullptr;

	const bool trooper = (Vehicle.VehicleType->Type == VEH_TYPE_PERSON);

	if (Vehicle.RideVehicleHostIndex > 0)
		RideVehicle = vehFindVehicle(Vehicle.RideVehicleHostIndex);

	// if I'm not an plane or if I'm ready to exit it, send off the PR_KILLED flag
	const bool sendKilled = (Vehicle.VehicleType->Type != VEH_TYPE_AIR || Vehicle.VehicleDeadTime > 0);

	if (Data.VehicleIsDead != 0 &&
		Data.LastVehicleIsDead == 0 &&
		sendKilled &&
		Data.MultiCrewPos <= 1)
	{
		VehicleHdr.present |= PR_KILLED;
		Data.LastVehicleIsDead = Data.VehicleIsDead;
		VehicleHdr.killedBy = Vehicle.ModelData->DeclaredKilled;
	}

	if (persIsCrewDead())
		VehicleHdr.present |= PR_DEAD;

	VehicleInfo.index = Vehicle.VehicleHostIndex;

	// position
	VehicleHdr.position.m_x = (SINT32)(Data.RenderPos.x * 100.0 + 0.5); // 1cm accuracy
	VehicleHdr.position.m_y = (SINT32)(Data.RenderPos.y * 100.0 + 0.5); // 1cm accuracy
	VehicleHdr.position.m_z = (UINT32)((Data.RenderPos.z + 2000.0) * 100.0 + 0.5); // 1cm accuracy

	// attitude
	TempAtt = Vehicle.PosData.Att;
	if (TempAtt.Pitch > PI) 	TempAtt.Pitch = fmod(TempAtt.Pitch + PI,PI*2.0) - PI;
	if (TempAtt.Pitch < -PI)	TempAtt.Pitch = -(fmod(-(TempAtt.Pitch - PI),PI*2.0) - PI);
	if (TempAtt.Roll > PI)		TempAtt.Roll = fmod(TempAtt.Roll + PI,PI*2.0) - PI;
	if (TempAtt.Roll < -PI)		TempAtt.Roll = -(fmod(-(TempAtt.Roll - PI),PI*2.0) - PI);
	if (TempAtt.Yaw > PI)		TempAtt.Yaw = fmod(TempAtt.Yaw + PI,PI*2.0) - PI;
	if (TempAtt.Yaw < -PI)		TempAtt.Yaw = -(fmod(-(TempAtt.Yaw - PI),PI*2.0) - PI);

	VehicleInfo.attitude.attX = (SINT16)(TempAtt.Pitch * 10000.0 + 0.5);
	VehicleInfo.attitude.attY = (SINT16)(TempAtt.Roll * 10000.0 + 0.5);
	VehicleInfo.attitude.attZ = (SINT16)(TempAtt.Yaw * 10000.0 + 0.5);

	// velocity
	TempVec = Vehicle.PosData.Vel;

	if (TempVec.x > 600.0) 	TempVec.x = 600.0;
	if (TempVec.x < -600.0)	TempVec.x = -600.0;
	if (TempVec.y > 600.0)	TempVec.y = 600.0;
	if (TempVec.y < -600.0)	TempVec.y = -600.0;
	if (TempVec.z > 600.0)	TempVec.z = 600.0;
	if (TempVec.z < -600.0)	TempVec.z = -600.0;

	VehicleInfo.velocity.velX = (SINT16)(TempVec.x * 50.0 + 0.5);
	VehicleInfo.velocity.velY = (SINT16)(TempVec.y * 50.0 + 0.5);
	VehicleInfo.velocity.velZ = (SINT16)(TempVec.z * 50.0 + 0.5);

	// accez
	TempVec = Vehicle.PosData.Acc;
	if (TempVec.x > 300.0)	TempVec.x = 300.0;
	if (TempVec.x < -300.0)	TempVec.x = -300.0;
	if (TempVec.y > 300.0)	TempVec.y = 300.0;
	if (TempVec.y < -300.0)	TempVec.y = -300.0;
	if (TempVec.z > 300.0)	TempVec.z = 300.0;
	if (TempVec.z < -300.0)	TempVec.z = -300.0;

	VehicleInfo.acceleration.accelX = (SINT16)(TempVec.x * 100.0 + 0.5);
	VehicleInfo.acceleration.accelY = (SINT16)(TempVec.y * 100.0 + 0.5);
	VehicleInfo.acceleration.accelZ = (SINT16)(TempVec.z * 100.0 + 0.5);

	TempAtt = Vehicle.PosData.AttVel;
	if (TempAtt.Pitch > 30.0)	TempAtt.Pitch = 30.0;
	if (TempAtt.Pitch < -30.0)	TempAtt.Pitch = -30.0;
	if (TempAtt.Roll > 30.0)	TempAtt.Roll = 30.0;
	if (TempAtt.Roll < -30.0)	TempAtt.Roll = -30.0;
	if (TempAtt.Yaw > 30.0)		TempAtt.Yaw = 30.0;
	if (TempAtt.Yaw < -30.0)	TempAtt.Yaw = -30.0;

	VehicleInfo.rotational.velX = (SINT16)(TempAtt.Pitch * 1000.0 + 0.5);
	VehicleInfo.rotational.velY = (SINT16)(TempAtt.Roll * 1000.0 + 0.5);
	VehicleInfo.rotational.velZ = (SINT16)(TempAtt.Yaw * 1000.0 + 0.5);

	VehicleInfo.spec() = Vehicle.VehicleType->WeaponSpec;
	VehicleInfo.timeStamp = (SINT16)(frametime);
	VehicleInfo.posTimeStamp = (SINT16)(frametime);

	// state information
	VehicleInfo.stateFlags0 = 0;
	VehicleInfo.stateFlags1 = 0;
	VehicleInfo.visBits = 0;

	for(i = 0; i < Vehicle.VehicleType->NumStates; i++)
	{
		// only send bits that are relavent to the player's multicrew position
		if (Data.MultiCrewPos != 0 && Data.MultiCrewPos != Vehicle.VehicleType->StateType[i].MultiCrewPos)
		{
			continue;
		}

		if (Vehicle.PosData.States[i])
		{
			if(i >= 32)
				VehicleInfo.visBits |= 1 << (i - 32);
			else if (i > 15 &&  i < 32)
				VehicleInfo.stateFlags1 |= 1 << (i - 16);
			else
				VehicleInfo.stateFlags0 |= 1 << i;
		}
	}

	/* although the vehicle defines this as UINT32 it is really an UPDT_STATION enum field and should not be hardcoded */
	if(static_cast<UPDT_STATION>(Data.MultiCrewPos) != UPDT_OTHER)
	{
		VehicleHdr.present |= PR_POSITION;
		if(Vehicle.VehicleType->NumStates > 0)
			VehicleHdr.present |= PR_STATE0;
		if(Vehicle.VehicleType->NumStates > 15)
			VehicleHdr.present |= PR_STATE1;
		if(Vehicle.VehicleType->NumStates > 31)
			VehicleHdr.present |= PR_VEH_VIS;

#ifdef BETA_RELEASE
			vehUpdatesSent++;
#endif

	}
	else
	{
		VehicleHdr.present &= ~(PR_STATE0 | PR_STATE1 | PR_VEH_VIS | PR_POSITION);
	}

	// mount firing
	VehicleInfo.fire.state = 0;

	for(i = 0; i < MAX_GUNS; i++)
	{
		if (Data.MountIsFiring[i])
			VehicleInfo.fire.state |= 1 << i;

		if(!trooper) // this flag is cleared below
			Data.MountIsFiring[i] = FALSE;
	}

	VehicleHdr.fireMask = VehicleInfo.fire.state;
	VehicleHdr.present |= PR_FIRE_FLAGS;

	// turret traverse
	TurretMask = 0;
	NumTurrets = 0;
	// trooper turrets are handled differently (much simpler)
	if(trooper)
	{
		for(i = 0; i <= 2; i++)
		{
#if defined(TURRET_GRANULARITY)
			TurretTrav[i] = (turret_t)((Data.TurretTraverse[i] + 1.0) / 2.0 * TURRET_GRANULARITY + 0.5);
			TurretElev[i] = (turret_t)((Data.TurretElevate[i] + 1.0) / 2.0 * TURRET_GRANULARITY + 0.5);
#else
			TurretTrav[i] = (turret_t)Data.TurretTraverse[i];
			TurretElev[i] = (turret_t)Data.TurretElevate[i];
#endif

			if (TurretTrav[i] != Vehicle.StateAnimation.LastSendTurretTrav[i] ||
				TurretElev[i] != Vehicle.StateAnimation.LastSendTurretElev[i])
			{
//				Write_Log (LOG_FPS, LOG_INFO, "XMIT %d: %d %d", i, TurretTrav[i], TurretElev[i]);
				//uiChatPrintLine("TURRET XMIT %d: %d %d", i, TurretTrav[i], TurretElev[i]);

				Vehicle.StateAnimation.LastSendTurretTrav[i] = TurretTrav[i];
				Vehicle.StateAnimation.LastSendTurretElev[i] = TurretElev[i];

				// add turrets to vehicleinfo, based on turretnum
				VehicleInfo.turret.pos[i].x = TurretTrav[i];
				VehicleInfo.turret.pos[i].y = TurretElev[i];

				TurretMask |= 1 << i;
				NumTurrets++;
			}
		}
	}
	else
	{
		for(i = 0; i < ML_MAX_CREW_POSITIONS; i++)
		{
			TurretElev[i] = 0;
			TurretTrav[i] = 0;

			// check for multicrew interlocks
			// don't send turret info for turrets not under persona's control
			if (Data.MultiCrewPos != 0 && Vehicle.VehicleType->MultiCrewPos[i] != Data.MultiCrewPos)
					continue;

			switch (Vehicle.VehicleType->TurretType[i])
			{
			case VEH_TURRET_TYPE_360:
#if defined(TURRET_GRANULARITY)
				if (Data.TurretTraverse[i] < 0.0)
					TurretTrav[i] = (turret_t)((Data.TurretTraverse[i] + 1.0) * TURRET_GRANULARITY + 0.5);
				else
					TurretTrav[i] = (turret_t)((Data.TurretTraverse[i]) * TURRET_GRANULARITY + 0.5);
				TurretElev[i] = (turret_t)((Data.TurretElevate[i] + 1.0) / 2.0 * TURRET_GRANULARITY + 0.5);
#else
				TurretTrav[i] = (turret_t)Data.TurretTraverse[i];
				TurretElev[i] = (turret_t)Data.TurretElevate[i];
#endif
			break;
			case VEH_TURRET_TYPE_LIMITED:
#if defined(TURRET_GRANULARITY)
				TurretTrav[i] = (turret_t)((Data.TurretTraverseWithStops[i] + 1.0) / 2.0 * TURRET_GRANULARITY + 0.5);
				TurretElev[i] = (turret_t)((Data.TurretElevate[i] + 1.0) / 2.0 * TURRET_GRANULARITY + 0.5);
#else
				TurretTrav[i] = (turret_t)Data.TurretTraverseWithStops[i];
				TurretElev[i] = (turret_t)Data.TurretElevate[i];
#endif
			break;
			case VEH_TURRET_TYPE_FLEXIBLE:
#if defined(TURRET_GRANULARITY)
				TurretTrav[i] = (turret_t)((Data.TurretTraverseRate[i] + 1.0) / 2.0 * TURRET_GRANULARITY + 0.5);
				TurretElev[i] = (turret_t)((Data.TurretElevateRate[i] + 1.0) / 2.0 * TURRET_GRANULARITY + 0.5);
#else
				TurretTrav[i] = (turret_t)Data.TurretTraverseRate[i];
				TurretElev[i] = (turret_t)Data.TurretElevateRate[i];
#endif
			break;
			default:
				continue;
			}

			if (TurretTrav[i] != Vehicle.StateAnimation.LastSendTurretTrav[i] ||
				TurretElev[i] != Vehicle.StateAnimation.LastSendTurretElev[i])
			{
				//Write_Log (LOG_FPS, LOG_INFO, "XMIT %d: %d %d", i, TurretTrav[i], TurretElev[i]);
				//uiChatPrintLine("TURRET XMIT %d: %d %d", i, TurretTrav[i], TurretElev[i]);

				// find out what the turret number is
				for (TurretNum = 0;TurretNum < MAX_TURRETS; TurretNum++)
				{
					// special case for trooper turret 0 and 1
					if (Vehicle.VehicleType->TurretNum[i] == TurretNum + 1)
					{
						Vehicle.StateAnimation.LastSendTurretTrav[i] = TurretTrav[i];
						Vehicle.StateAnimation.LastSendTurretElev[i] = TurretElev[i];

						// add turrets to vehicleinfo, based on turretnum
						VehicleInfo.turret.pos[TurretNum].x = TurretTrav[i];
						VehicleInfo.turret.pos[TurretNum].y = TurretElev[i];

						TurretMask |= 1 << TurretNum;
						NumTurrets++;
						break;
					}
				}
			}
		}
	}

	if(trooper)
	{
		VehicleInfo.visBits = fpFPSGetResupplyTarget();

		if(VehicleInfo.visBits)
			VehicleHdr.present |= PR_VEH_VIS;
	}

	if(Vehicle.RideVehicleHostIndex > 0)
	{
		VehicleHdr.present |= PR_RIDING;
		VehicleInfo.rideIndex = Vehicle.RideVehicleHostIndex;
		VehicleInfo.seatNumber = (UINT16)Vehicle.RideSeatNum;

		if(Vehicle.RideSeatNum == VEH_TROOPER_ON_DECK_RIDE_SEAT)
		{
			// send position relative to ship (in cm)
			VehicleHdr.present |= PR_RELPOS;
			VehicleInfo.relPos.relX = (SINT16)(Vehicle.RelRidePosition.x * 100.0);
			VehicleInfo.relPos.relY = (SINT16)(Vehicle.RelRidePosition.y * 100.0);
			VehicleInfo.relPos.relZ = (SINT16)(Vehicle.RelRidePosition.z * 100.0);
			VehicleInfo.relPos.yaw = (SINT8)(RAD_TO_DEG(Vehicle.RelRideYaw) * 255.0 / 360.0);
		}
	}
	else if(Vehicle.LastRideVehicleHostIndex != Vehicle.RideVehicleHostIndex)
	{
		// send it one more time when we get off
		VehicleHdr.present |= PR_RIDING;
		VehicleInfo.rideIndex = Vehicle.RideVehicleHostIndex;
		VehicleInfo.seatNumber = (UINT16)Vehicle.RideSeatNum;
	}

	// save it
	Vehicle.LastRideVehicleHostIndex = Vehicle.RideVehicleHostIndex;

	// add turretmask and numturrets to vehicleheader
	VehicleHdr.vt.numTurrets = NumTurrets;
	VehicleHdr.vt.turretMask = TurretMask;
	if (TurretMask != 0)
		 VehicleHdr.present |= PR_TURRET;

	// A little hack to make the man animate. By Jonathan...
	if(trooper)
	{
		VehicleHdr.present |= (PR_STATE0|PR_STATE1);

		const UINT32 animationFlags = ((fpPersonData*) Data.PersonData)->AnimationFlags;
		VehicleInfo.stateFlags0 = (UINT16)(animationFlags & 0xffff);
		VehicleInfo.stateFlags1 = (UINT16)(animationFlags >> 16);
	}

	if (vehmainWaitingOnLogoutTimer())
	{
		VehicleHdr.present |= PRX_DESPAWN_TIMER;
		VehicleInfo.timerInfo = 1;
	}
	else
	{
		VehicleHdr.present &= ~PRX_DESPAWN_TIMER;
		VehicleInfo.timerInfo = 0;
	}

	// Tell the host which building we are currently in
	VehicleInfo.fpsBuildingID = Vehicle.fpsBuildingID;

	// Tell the host if we think the building is a capture building
	//if(PlayerClient()->IsPermitted(CAN_CAPTURE))
		VehicleInfo.fpsInCaptureBuilding = fpFPSIsCaptureZone();
	//else
	//	VehicleInfo.fpsInCaptureBuilding = false;

	VehicleHdr.vehInfo = VehicleInfo;

	// FIXME - what the heck?
	// don't send first 3 updates
	if (Data.HasSentUpdate >= 3)
	{
		VehicleHdr.updateNo = wwiiGetUpdateNo(true);
		VehicleHdr.updatePrime = wwiiHitsPrime();
		COMM_UPDATE_BUFFER cub;
		cub.size = updtPackSHARED_VEHICLE_INFO(TEUL_PACK, cub.data, 0, &VehicleHdr, sizeof(cub.data));
		if ( cub.size >= 0 )
		{
			// Send them our teulClock64
			teulSyncClock();
			cub.size = teulPackUINT64(TEUL_PACK, cub.data, cub.size, &teulClock64, 0);
			teulSend(PlayerClient()->CellConnection(),commUPDATE,&cub);
		}
	}
	else
		++Data.HasSentUpdate;

	// after all else is sent send the building damage to the strat updater
	_processBuildingDamage(Vehicle);

	memset(&VehicleInfo, 0, sizeof(VehicleInfo));
	memset(&VehicleHdr, 0, sizeof(VEHICLE_UPDATE_HEADER));

	// let the fps system know to clear some stuff for this update
	fpFPSUpdateSent();

	return 0;
}

static void ProcessInfantryAnimationFlags(vehVehicle* Vehicle, const VEHICLE_UPDATE_HEADER* VehicleHeader)
{
#ifdef NEW_INF_PREDICTOR
	bool useNewPredictor = (_newpredictor && !(Vehicle->Flags & VEH_FLAGS_USE_OLD_INTERPOLATOR));

	if(useNewPredictor)
	{
		if (VehicleHeader->present & PR_STATE0)
		{
			Vehicle->PosData.animationFlags &= 0xffff0000;
			Vehicle->PosData.animationFlags|= VehicleHeader->vehInfo.stateFlags0;
		}

		if (VehicleHeader->present & PR_STATE1)
		{
			Vehicle->PosData.animationFlags &= 0x0000ffff;
			Vehicle->PosData.animationFlags |= (UINT32)(VehicleHeader->vehInfo.stateFlags1) << 16;
		}
	}
	else
	{
#endif
		if (VehicleHeader->present & PR_STATE0)
		{
			Vehicle->AnimationBits &= 0xffff0000;
			Vehicle->AnimationBits |= VehicleHeader->vehInfo.stateFlags0;
		}

		if (VehicleHeader->present & PR_STATE1)
		{
			Vehicle->AnimationBits &= 0x0000ffff;
			Vehicle->AnimationBits |= (UINT32)(VehicleHeader->vehInfo.stateFlags1) << 16;
		}

		fpProcessVehicleAnimationFlags(Vehicle);
#ifdef NEW_INF_PREDICTOR
	}
#endif
}

static UINT32 _ExtractData(vehVehiclePositionData * PosData,vehVehicle* Vehicle,const VEHICLE_UPDATE_HEADER * VehicleHeader)
{
	SINT16 Delta;
	UINT32 i;
	LOGICAL Rollover = FALSE;

	if (VehicleHeader == nullptr) return 0;
	const SHARED_VEHICLE_INFO*VehicleInfo = &VehicleHeader->vehInfo;

	Delta = (SINT16)((UINT16)VehicleInfo->posTimeStamp) - (SINT16)((UINT16)PosData->CurrentSenderUpdateTime);
	if (Delta < 0 && !(VehicleHeader->present & PR_ONE_TIME))
	{
		Rollover = TRUE;
	}

	// states
	if (VehicleHeader->present & PR_STATE0)
	{
		for (i=0;i<16;i++)
		{
			// only update state bits on other multicrew players update
			if (Vehicle->ModelData != nullptr)
			{
				if (Vehicle->ModelData->MultiCrewPos ==
							Vehicle->VehicleType->StateType[i].MultiCrewPos)
				{
					continue;
				}
			}

			if (VehicleInfo->stateFlags0 & 1 << i)
				PosData->States[i] = TRUE;
			else
				PosData->States[i] = FALSE;
		}
	}
	if (VehicleHeader->present & PR_STATE1)
	{
		for (i=16;i<32;i++)
		{
			// only update state bits on other multicrew players update
			if (Vehicle->ModelData != nullptr)
			{
				if (Vehicle->ModelData->MultiCrewPos ==
							Vehicle->VehicleType->StateType[i].MultiCrewPos)
				{
					continue;
				}
			}

			if (VehicleInfo->stateFlags1 & 1 << (i - 16))
				PosData->States[i] = TRUE;
			else
				PosData->States[i] = FALSE;
		}
	}

	if (Vehicle->VehicleType->Type != VEH_TYPE_PERSON && VehicleHeader->present & PR_VEH_VIS)
	{
		for (i=32;i<64;i++)
		{
			// only update state bits on other multicrew players update
			if (Vehicle->ModelData != nullptr)
			{
				if (Vehicle->ModelData->MultiCrewPos ==
							Vehicle->VehicleType->StateType[i].MultiCrewPos)
				{
					continue;
				}
			}

			if (VehicleInfo->visBits & 1 << (i - 32))
				PosData->States[i] = TRUE;
			else
				PosData->States[i] = FALSE;
		}
	}

	// position
	// if multicrew, only crew gets this data
	if (!Rollover && (VehicleHeader->present & PR_POSITION))
	{
		if (Vehicle->ModelData == nullptr || Vehicle->ModelData->MultiCrewPos > 1)
		{
			if (Online == 0)
			{
				PosData->Pos.x = VehicleHeader->position.m_x / 100.0;
				PosData->Pos.y = VehicleHeader->position.m_y / 100.0;
				PosData->Pos.z = VehicleHeader->position.m_z / 100.0 - 2000.0;
			}
			else
			{
				PosData->Pos.x = VehicleInfo->position.m_x / 100.0;
				PosData->Pos.y = VehicleInfo->position.m_y / 100.0;
				PosData->Pos.z = VehicleInfo->position.m_z / 100.0 - 2000.0;

#ifdef BETA_RELEASE
				if (Vehicle->ModelData != nullptr)
				{
					/* Track how far off our predictions are off and accumulate perc diff */
					vehMcrewUpdatesRecv++;
					FLOAT64 xPerc = abs(PosData->Pos.x - PosData->LastUpdatePos.x) / PosData->LastUpdatePos.x;
					FLOAT64 yPerc = abs(PosData->Pos.y - PosData->LastUpdatePos.y) / PosData->LastUpdatePos.y;
					FLOAT64 zPerc = abs(PosData->Pos.z - PosData->LastUpdatePos.z) / PosData->LastUpdatePos.z;
					vehMultiCrewAccumPerc.x += xPerc;
					vehMultiCrewAccumPerc.y += yPerc;
					vehMultiCrewAccumPerc.z += zPerc;
				}
				else
				{
					vehNonMcrewUpdatesRecv++;
				}
#endif
			}

			// attitude
			PosData->Att.Pitch = VehicleInfo->attitude.attX / 10000.0;
			PosData->Att.Roll = VehicleInfo->attitude.attY / 10000.0;
			PosData->Att.Yaw = VehicleInfo->attitude.attZ / 10000.0;
			// velocity
			PosData->Vel.x = VehicleInfo->velocity.velX / 50.0;
			PosData->Vel.y = VehicleInfo->velocity.velY / 50.0;
			PosData->Vel.z = VehicleInfo->velocity.velZ / 50.0;

			// acceleration
			PosData->Acc.x = VehicleInfo->acceleration.accelX / 100.0;
			PosData->Acc.y = VehicleInfo->acceleration.accelY / 100.0;
			PosData->Acc.z = VehicleInfo->acceleration.accelZ / 100.0;

			// attvel
			PosData->AttVel.Pitch = VehicleInfo->rotational.velX / 1000.0;
			PosData->AttVel.Roll = VehicleInfo->rotational.velY / 1000.0;
			PosData->AttVel.Yaw = VehicleInfo->rotational.velZ / 1000.0;
		}
		else
		{
#ifdef BETA_RELEASE
			vehMcommanderUpdatesIgn++;
#endif
		}
	}

	if (!Rollover && VehicleHeader->present & PR_RELPOS)
	{
		Vehicle->PosData.LastRelUpdateTime = Vehicle->PosData.CurRelUpdateTime;
		Vehicle->PosData.CurRelUpdateTime = frametime;
		Vehicle->PosData.LastRelPos = Vehicle->RelRidePosition;
		Vehicle->PosData.LastRelYaw = Vehicle->RelRideYaw;

		Vehicle->PosData.CurRelPos.x = VehicleInfo->relPos.relX / 100.0;
		Vehicle->PosData.CurRelPos.y = VehicleInfo->relPos.relY / 100.0;
		Vehicle->PosData.CurRelPos.z = VehicleInfo->relPos.relZ / 100.0;
		Vehicle->PosData.CurRelYaw = -DEG_TO_RAD(VehicleInfo->relPos.yaw * 360.0 / 255.0); // has to be reversed for some reason

		// handle angle "flip"
		if(fabs(Vehicle->PosData.CurRelYaw - Vehicle->PosData.LastRelYaw) > PI)
		{
			if(Vehicle->PosData.LastRelYaw > 0)
				Vehicle->PosData.CurRelYaw =  PI2 + Vehicle->PosData.CurRelYaw;
			else
				Vehicle->PosData.CurRelYaw = -PI2 + Vehicle->PosData.CurRelYaw;
		}
	}

	if (Vehicle->VehicleType && Vehicle->VehicleType->Type == VEH_TYPE_PERSON)
		ProcessInfantryAnimationFlags(Vehicle, VehicleHeader);

	if (!Rollover && (VehicleHeader->present & (PR_POSITION | PR_RELPOS)))
	{
		PosData->LastSenderUpdateTime = PosData->CurrentSenderUpdateTime;
		PosData->CurrentSenderUpdateTime = VehicleInfo->posTimeStamp;

		PosData->LastUpdateTime = PosData->CurrentUpdateTime;
		PosData->CurrentUpdateTime = frametime;
	}


	if ((VehicleHeader->present & (PR_POSITION | VehicleHeader->present & PR_RELPOS)) &&
		Vehicle->VehicleType && Vehicle->VehicleType->Type == VEH_TYPE_PERSON)
	{
		if (Vehicle->VehicleObjectData != nullptr)
		{
			if (Vehicle->VehicleObjectData->PlaneObject != nullptr)
			{
				fpHandleProneTrooperPosition (Vehicle->VehicleObjectData->PlaneObject, PosData);
			}
		}
	}

#ifdef NEW_INF_PREDICTOR
	bool useNewPredictor = (_newpredictor && !(Vehicle->Flags & VEH_FLAGS_USE_OLD_INTERPOLATOR));

	if(useNewPredictor && Vehicle->VehicleType->Type == VEH_TYPE_PERSON)
	{
		Vehicle->AnimationBits = PosData->animationFlags;
		if(VehicleHeader->present & PR_POSITION)
			Vehicle->interpolator.NewPosDataUpdate(VehicleInfo->timeStamp, frametime);
	}
#endif

	return 0;
}

void vehicleRemoveVehicle(UINT32 index)
{
	vehVehicle* const Vehicle = vehFindVehicle(index);

	Write_Debug("v:%u removal indicated", index);

	if (Vehicle != nullptr)
	{
		//uiChatPrintLine("Removing Vehicle %s -> %u", (const char*)Vehicle->Handle, index);
		vehKillVehicle(Vehicle);
	}
	/*
	else
	{
 		uiChatPrintLine("Trying to remove nullptr vehicle with index %u!!!", index);
	}
	*/
}

// Set the vehicle info bits corresponding to onetime flags
void _setVehicleOneTimeBits(vehVehicle* Vehicle, UINT32 oneTimeBits)
{
	// set squaddie bit
	if (oneTimeBits & OTB_IN_SQUAD)
		Vehicle->VehicleIsSquaddie = TRUE;

	if (oneTimeBits & OTB_HIGH_COMMAND)
		Vehicle->IsCommander = TRUE;

	if (oneTimeBits & OTB_BUILDER)
		Vehicle->IsBuilder = TRUE;

	if (oneTimeBits & OTB_IN_BRIGADE)
		Vehicle->SameBrigade = TRUE;

	if (oneTimeBits & OTB_IN_MISSION)
		Vehicle->SameMission = TRUE;

	if (oneTimeBits & OTB_MISSION_LEADER)
		Vehicle->IsMissionLeader = TRUE;

	if (oneTimeBits & OTB_TRAINER)
		Vehicle->IsTrainer = TRUE;

	// always use the poster's name in the selected mission
	// because it can change and this may not get updated on the host
	//if (Vehicle->SameMission && Vehicle->IsMissionLeader)
	//	Vehicle->IsMyMissionLeader = TRUE;
	if(Vehicle->SameMission)
	{
		MissionDetail& mission = PlayerClient()->SelectedMission();

		if(Vehicle->Chat_Handle == mission.m_poster)
		{
			Vehicle->IsMyMissionLeader = TRUE;

			if(cfgPrefsGetBool("app.missionwaypoints", true))
				HUD_MGR->AddObject(new PersHUDManager::HUDLeaderWaypoint(Vehicle));
		}
	}

	// init the vehicle's icon pulse flag to not pulse
	vehIconSetPulsate(Vehicle, 0);
}

UINT32 vehupdateProcessPositionUpdateMsg(const VEHICLE_UPDATE_HEADER* VehicleHeader)
{
	if(!VehicleHeader) return 0;

	vehVehicle* Vehicle;
	SINT32 Delta;
	SINT32 Delta1;
	UINT32 UpdateMult;
	UINT32 Error;
	SINT32 TurretNum;
	UINT32 i,j;

#ifdef BETA_RELEASE
	worldUpdatesReceived++;
#endif

	const SHARED_VEHICLE_INFO* const VehicleInfo = &VehicleHeader->vehInfo;

	const UINT32 Position = VehicleInfo->index;

	// check for change of vehicle for this position
	Vehicle = vehFindVehicle(Position);


#ifdef VEHICLES_GAME_INFO
	if(Vehicle)
		vehRecordVehicleUpdate(Vehicle->VehicleHostIndex);
#endif

	if (VehicleHeader->present & PR_ONE_TIME)
	{
		if (Vehicle != nullptr && Vehicle->ModelData == nullptr)
		{
			Vehicle->BumpedOff = 0;
		}
		else
		{
			ChatHandle ch = chatGetHandle(VehicleInfo->callsign);

			if (VehicleInfo->country > MAX_COUNTRIES)
			{
				Write_Debug("Invalid countries specified for vehicle %s, host index %d",VehicleInfo->callsign,
								VehicleInfo->index);
				return 0;
			}
			{
				WEAPON* weapon = weapLookup(VehicleInfo->spec());
				if(weapon == nullptr)
				{
					return 0;
				}

				vehupdateCreateVehicle(&Vehicle,weapon->localIndex(),0,VehicleInfo->index, ch);
			}

			if (Vehicle == nullptr)
			{
				// we're out of vehicle slots
				Write_Debug("Out of free vehicle slots for new vehicle");
				return 0;
			}

			Vehicle->GotOneTime = true;
			Vehicle->LastUpdate = frametime;
			Vehicle->PosData.LastReceivedTime = frametime;

			Vehicle->Side = wwiiSide(VehicleInfo->country);
			Vehicle->Country = VehicleInfo->country;
			strncpy((char*)Vehicle->Handle,VehicleInfo->callsign,MAX_HANDLE_LEN);

			if(*VehicleInfo->squadName)
				strncpy((char*)Vehicle->SquadHandle,VehicleInfo->squadName,MAX_HANDLE_LEN); // squadname

			Vehicle->Chat_Handle = ch;
			Vehicle->Rank = VehicleInfo->rank;
			if (VehicleInfo->oneTimeBits & OTB_USE_DECALS)
				Vehicle->persDecal = VehicleInfo->decal;
			else
				Vehicle->persDecal = 0;

            Vehicle->unitDecal = VehicleInfo->brigadeID;

			// Special Rat Icon Rank stuff
			if(isupper(Vehicle->Handle[0]))
				Vehicle->Rank = GENERIC_RAT_RANK;

			for(i = 0; _SpecialRanks[i].name; i++)
			{
				if(!strcasecmp((char*)Vehicle->Handle, _SpecialRanks[i].name))
				{
					Vehicle->Rank = _SpecialRanks[i].Rank;
					break;
				}
			}
		}

		///TODO: May need to change where this goes to support gophurs forced loadout system
		_setVehicleOneTimeBits(Vehicle, VehicleInfo->oneTimeBits);
	}

#ifdef SIMULATE_LOW_UPDATE_RATE
if(_simlowupdate)
{
// simulate low update rate
static int lastLowUpdateTime;
if(Vehicle && frametime - lastLowUpdateTime < _simlowupdate)
{
	ProcessInfantryAnimationFlags(Vehicle, VehicleHeader); // process these regardless to avoid missing a state change
	return 0;
}
lastLowUpdateTime = frametime;
}
#endif

	if (Vehicle == nullptr)
	{
		// vehicle doesn't exist in list
		Write_Debug("Received an update for vehicle not being tracked, index = %d",Position);
		return 0;
	}

    if(VehicleInfo->timeStamp == Vehicle->PosData.LastAnyUpdateTime)
	{
#ifdef BETA_RELEASE
		duplicateUpdatesIgnored++;
#endif
		Write_Debug("Duplicate update received for vehicle index %d (%s)", Position, Vehicle->Handle);
		return 0;
	}

	Vehicle->LastUpdateDelta = frametime - Vehicle->LastUpdate;
	Vehicle->LastUpdate = frametime;

	Vehicle->BumpedOff = 0;

	const SINT32 frameDelta = frametime - Vehicle->PosData.LastReceivedTime;
	if (frameDelta >= 5000)
	{
		Write_Debug("Vehicle index %d %s, delta time between updates = %d",
						Vehicle->VehicleHostIndex,
						Vehicle->Handle,
						frameDelta);
	}

	Vehicle->PosData.LastAnyUpdateTime = VehicleInfo->timeStamp;

	// if there's modeldata, it isn't updatesystem controlled
	if (Vehicle->ModelData == nullptr)
		Vehicle->Flags |= VEH_FLAGS_UPDATESYSTEM_CONTROLLED;

	if (VehicleHeader->present & (PR_RIDING | PR_ONE_TIME))
	{
 		Vehicle->RideSeatNum = VehicleInfo->seatNumber;
		vehUpdateRideChanged(Vehicle, VehicleInfo->rideIndex);
	}

	Vehicle->PosData.LastUpdatePos = Vehicle->PosData.RenderPos;
	Vehicle->PosData.LastUpdateAtt = Vehicle->PosData.RenderAtt;
	Vehicle->PosData.LastLastUpdateVel = Vehicle->PosData.LastUpdateVel;
	Vehicle->PosData.LastUpdateVel = Vehicle->PosData.Vel;
	Vehicle->PosData.LastUpdateAcc = Vehicle->PosData.Acc;
	Vehicle->PosData.LastUpdateAttVel = Vehicle->PosData.AttVel;

	Vehicle->PosData.CumulativeDelta -= frametime - Vehicle->PosData.CurrentUpdateTime;
	if (Vehicle->PosData.CumulativeDelta < 0)
	{
		Vehicle->PosData.CumulativeDelta = 0;
	}
	if (_ExtractData(&Vehicle->PosData,Vehicle,VehicleHeader) != 0)
	{
		// vehicle has been killed
		return 0;
	}
	Vehicle->PosData.CumulativeDelta +=
			(Vehicle->PosData.CurrentUpdateTime - Vehicle->PosData.LastUpdateTime);

	Vehicle->LastUpdateType = VehicleInfo->resolution;

	if (Vehicle->PosData.UpdateMult == 0)
	{
		UpdateMult = 1;
	}
	else
	{
		UpdateMult = Vehicle->PosData.UpdateMult;
	}
	Error = 500;

	if (Vehicle->PosData.CumulativeDelta > (SINT32)g_hostUpdateInterval)
	{
		Vehicle->PosData.CumulativeDelta -= g_hostUpdateInterval * UpdateMult + Error;
		Vehicle->PosData.CumulativeDelta /= 2;
		Vehicle->PosData.CumulativeDelta += g_hostUpdateInterval * UpdateMult + Error;
	}

	if (Vehicle->PosData.CumulativeDelta < (SINT32)g_hostUpdateInterval)
	{
		Vehicle->PosData.CumulativeDelta += g_hostUpdateInterval * UpdateMult + Error;
		Vehicle->PosData.CumulativeDelta /= 2;
		Vehicle->PosData.CumulativeDelta -= g_hostUpdateInterval * UpdateMult + Error;
	}

	if (Vehicle->PosData.CumulativeDelta > 10000)
	{
		Vehicle->PosData.CumulativeDelta = 10000;
	}
	if (Vehicle->PosData.LastUpdateTime == 0)
	{
		Vehicle->PosData.CumulativeDelta = 0;
		Vehicle->PosData.LastPredictorUpdateTime = frametime;
	}
	else
	{
		Delta =	(SINT16)((SINT16)((SINT16)(Vehicle->PosData.CurrentUpdateTime) - (SINT16)(Vehicle->PosData.LastUpdateTime)) -
				(SINT16)((SINT16)(Vehicle->PosData.CurrentSenderUpdateTime) - (SINT16)(Vehicle->PosData.LastSenderUpdateTime)));

		Delta += Vehicle->PosData.LastDelta;


		Vehicle->PosData.LastDelta = Delta;

		Delta1 = abs((SINT32)g_hostUpdateInterval - (SINT32)FE_UPDATE_INTERVAL);
		Delta1 *= 2;

		if (Vehicle->PosData.LastDelta > Delta1)
		{
			Vehicle->PosData.LastDelta = Vehicle->PosData.LastDelta * 8 / 10;
		}

		if (Vehicle->PosData.LastDelta < -Delta1)
		{
			Vehicle->PosData.LastDelta = Vehicle->PosData.LastDelta * 8 / 10;
		}

		Vehicle->PosData.LastPredictorUpdateTime = frametime - Delta;
	}

	bool trooper = (Vehicle->VehicleType->Type == VEH_TYPE_PERSON);

	// weapon fire
	if (VehicleHeader->present & PR_FIRE_FLAGS)
	{
		for (i=0;i<VEH_MAX_MOUNTS;i++)
		{
			if (VehicleHeader->fireMask & 1<<i)
			{
				Vehicle->MountIsFiring[i] = TRUE;
				for (j=0;j<4;j++)
				{
					if (Vehicle->Weapons[i][j].WeaponType != nullptr)
					{
						if (Vehicle->Weapons[i][j].WeaponType->HideOnLastDrop == 1 &&
							VehicleHeader->present & PR_ONE_TIME)
						{
							Vehicle->WeaponIsHidden[i][j] = 1;
							if (Vehicle->VehicleObjectData && Vehicle->VehicleObjectData->WeaponConnectPoints[i][j] != nullptr)
							{
								ogrpHideObjectAndSubtree(Vehicle->VehicleObjectData->WeaponConnectPoints[i][j]);
							}
						}
					}
				}
			}
			else
			{
				Vehicle->MountIsFiring[i] = FALSE;
			}
		}
	}

	// turret stuff
	// skip the first update because it contains bad data...not sure why..can't find where the client sends the bad data
	// maybe the host data isn't init'ed correctly or something
	if (VehicleHeader->present & PR_TURRET && Vehicle->VehicleType != nullptr)// && Vehicle->PosData.LastUpdateTime)
	{
		if (trooper)
		{
			for (i=0; i<=2; i++)
			{
				if (VehicleHeader->vt.turretMask & (1 << i))
				{
#if defined(TURRET_GRANULARITY)
					Vehicle->StateAnimation.TurretTraverse[i] = ((VehicleInfo->turret.pos[i].x / TURRET_GRANULARITY) * 2.0f) - 1.0f;
					Vehicle->StateAnimation.TurretElev[i] = ((VehicleInfo->turret.pos[i].y / TURRET_GRANULARITY) * 2.0f) - 1.0f;
#else
					Vehicle->StateAnimation.TurretTraverse[i] = VehicleInfo->turret.pos[i].x;
					Vehicle->StateAnimation.TurretElev[i] = VehicleInfo->turret.pos[i].y;
#endif
//					Write_Log (LOG_FPS, LOG_INFO, "RCV %d:  %d %d %f %f", i,VehicleInfo->turret.pos[i].x,VehicleInfo->turret.pos[i].y, Vehicle->StateAnimation.TurretTraverse[i] , Vehicle->StateAnimation.TurretElev[i]);
					//uiChatPrintLine("TURRET RCV %d:  %d %d %f %f", i,VehicleInfo->turret.pos[i].x,VehicleInfo->turret.pos[i].y, Vehicle->StateAnimation.TurretTraverse[i] , Vehicle->StateAnimation.TurretElev[i]);
				}

			}
		}
		else
		{
			for (TurretNum=0;TurretNum<MAX_TURRETS;TurretNum++)
			{
				if (VehicleHeader->vt.turretMask & (1 << TurretNum))
				{
					for (i=0;i<ML_MAX_CREW_POSITIONS;i++)
					{
						if ((Vehicle->VehicleType->TurretNum[i] == TurretNum + 1) || ((trooper) && (TurretNum <= 1)))
						{
							// ignore turret updates that the player controls
							if (Vehicle->ModelData != nullptr)
							{
								if (Vehicle->ModelData->MultiCrewPos != 0)
								{
									if (Vehicle->ModelData->MultiCrewPos == Vehicle->VehicleType->MultiCrewPos[i])
									{
										continue;
									}
								}
							}

							if (trooper)
							{
#if defined(TURRET_GRANULARITY)
								Vehicle->StateAnimation.TurretTraverse[i] = ((VehicleInfo->turret.pos[TurretNum].x / TURRET_GRANULARITY) * 2.0f) - 1.0f;
#else
								Vehicle->StateAnimation.TurretTraverse[i] = VehicleInfo->turret.pos[TurretNum].x;
#endif
							}
							else if (Vehicle->VehicleType->TurretType[i] == VEH_TURRET_TYPE_360)
							{
#if defined(TURRET_GRANULARITY)
								Vehicle->StateAnimation.TurretTraverse[i] = (VehicleInfo->turret.pos[TurretNum].x / TURRET_GRANULARITY);
#else
								Vehicle->StateAnimation.TurretTraverse[i] = VehicleInfo->turret.pos[TurretNum].x;
#endif
							}
							else
							{
#if defined(TURRET_GRANULARITY)
								Vehicle->StateAnimation.TurretTraverse[i] = ((VehicleInfo->turret.pos[TurretNum].x / TURRET_GRANULARITY) * 2.0f) - 1.0f;
#else
								Vehicle->StateAnimation.TurretTraverse[i] = VehicleInfo->turret.pos[TurretNum].x;
#endif
							}

#if defined(TURRET_GRANULARITY)
							Vehicle->StateAnimation.TurretElev[i] = ((VehicleInfo->turret.pos[TurretNum].y / TURRET_GRANULARITY) * 2.0f) - 1.0f;
#else
							Vehicle->StateAnimation.TurretElev[i] = VehicleInfo->turret.pos[TurretNum].y;
#endif
							//Write_Log (LOG_FPS, LOG_INFO, "RCV %d:  %d %d %f %f", i,VehicleInfo->turret.pos[i].x,VehicleInfo->turret.pos[i].y, Vehicle->StateAnimation.TurretTraverse[i] , Vehicle->StateAnimation.TurretElev[i]);
							//uiChatPrintLine("TURRET RCV %d:  %d %d %f %f", i,VehicleInfo->turret.pos[i].x,VehicleInfo->turret.pos[i].y, Vehicle->StateAnimation.TurretTraverse[i] , Vehicle->StateAnimation.TurretElev[i]);

							break;
						}
					}
				}
			}
		}
	}

	// check fire flag for turrets
	if ((VehicleHeader->present & PR_FIRE_FLAGS) && VehicleHeader->fireMask != 0)
	{
		for (i=0;i<ML_MAX_CREW_POSITIONS;i++)
		{
			Vehicle->StateAnimation.LastTurretTraverse[i] =
						Vehicle->StateAnimation.TurretTraverse[i];
			Vehicle->StateAnimation.LastTurretElev[i] =
						Vehicle->StateAnimation.TurretElev[i];

		}
	}

	Vehicle->PosData.LastReceivedDelta = frametime - Vehicle->PosData.LastReceivedTime;
	Vehicle->PosData.LastReceivedTime = frametime;

	return 0;
}


static UINT32 vehupdateInfantry(vehVehicle* Vehicle)
{
	gr2AnimTrooper* anim = Vehicle->GetAnim();
	if(!anim) return 0;

	vehVehiclePositionData* PosData = &Vehicle->PosData;

	// interpolate from the last position
	InterpolatorBase::Update const& update	= Vehicle->interpolator.Interpolate(frametime);

    // For .vspawn infantry
    if(!Vehicle->isSTO())
    {
	    // update magical mystery variables
	    PosData->RenderPos = PosData->Pos = update.pos;
	    PosData->Att.Pitch = PosData->Att.Roll = PosData->RenderAtt.Pitch = PosData->RenderAtt.Roll = 0;
    }

	// do yaw smoothing as needed
	//anim->twist.UpdateYaw(Vehicle);

	// update our position variables
	PosData->RenderAtt.Yaw	= anim->twist.vehicleYaw;
	PosData->Att.Yaw		= anim->twist.vehicleYaw;
	PosData->Vel			= update.vel;
	PosData->Vel.x			= -PosData->Vel.x; // usual bullshit

	Vehicle->AnimationBits	= update.animationFlags;

	fpProcessVehicleAnimationFlags(Vehicle);

	return 0;
}

UINT32 vehupdateUpdateVehicle(vehVehicle* Vehicle)
{
	FLOAT64					Delta,Delta1;
	SINT32					LastUpdateTime;
	vehVehiclePositionData*	PosData;
	piMATRIX				Mat1,Mat2,Mat3;
	piVECTOR				Att1,Att2;
	piVECTOR				TempPiVec;
	FLOAT64VECTOR			TempVec;
	FLOAT64VECTOR			PrevVel,PrevVel1;
	FLOAT64ATT				VAtt1,VAtt2;
	FLOAT64VECTOR			WorldOriginOffset;
	OBJECT*					pObject;

	if (Vehicle == nullptr) return 0;
	PosData = &Vehicle->PosData;

#ifdef NEW_INF_PREDICTOR
	bool useNewPredictor = (_newpredictor && !(Vehicle->Flags & VEH_FLAGS_USE_OLD_INTERPOLATOR));

	if(useNewPredictor && Vehicle->VehicleType->Type == VEH_TYPE_PERSON)
	{
		vehupdateInfantry(Vehicle);
	}
	else
	{
#endif

	LastUpdateTime = frametime - PosData->LastReceivedTime;

	// time out old vehicles
	//
	// ONLY applies to host-controlled vehicles that the player doesn't occupy
	if (Online &&
        Vehicle->ModelData == nullptr &&
        LastUpdateTime > TIMEOUT_MS_HIDE &&
        // STO and .vspawn vehicles have negative vehicle numbers (don't want
        // to bump them off)
        Vehicle->VehicleNum > 0)
    {
		Vehicle->BumpedOff = 1;
    }

	// determine if update rate on this vehicle is too slow
	Delta  = (FLOAT64)(frametime - PosData->LastPredictorUpdateTime) / 1000.0;
	Delta1 = (FLOAT64)(frametime - PosData->LastUpdatePredictorUpdateTime) / 1000.0;

	if (Vehicle->VehicleType != nullptr &&
		Vehicle->VehicleType->Type == VEH_TYPE_GROUND &&
		!Vehicle->VisDamageNoStick &&
		(Online || !Vehicle->isSTO())) // offline STO's do not stick to ground so we can position accurately for marketing
	{ // ground vehicles always stick to ground unless riding or hoisting
		Vehicle->StickVehicleToGround = (Vehicle->RideVehicleHostIndex == 0);
	}
	else
	{
		Vehicle->StickVehicleToGround = FALSE;
	}

	// scenario stos don't need to be stuck to ground
	if(SCEN_MGR->IsActive() && Vehicle->isSTO())
		Vehicle->StickVehicleToGround = FALSE;

	Att1.x = (FLOAT32)PosData->Att.Pitch;
	Att1.y = (FLOAT32)PosData->Att.Roll;
	Att1.z = (FLOAT32)PosData->Att.Yaw;
	Att2.x = -(FLOAT32)(PosData->AttVel.Pitch * Delta);
	Att2.y = -(FLOAT32)(PosData->AttVel.Roll * Delta);
	Att2.z = (FLOAT32)(PosData->AttVel.Yaw * Delta);
	mathEul_ToMatrix(&Mat1,EulOrdXYZs,&Att1);
	mathEul_ToMatrix(&Mat2,EulOrdXYZs,&Att2);
	math3dMatrixMultiply(&Mat3,&Mat2,&Mat1);
	mathEul_FromMatrix(&Att1,&Mat3,EulOrdXYZs);
	PosData->Att.Pitch = Att1.x;
	PosData->Att.Roll = Att1.y;
	PosData->Att.Yaw = Att1.z;

	Att1.x = (FLOAT32)PosData->LastUpdateAtt.Pitch;
	Att1.y = (FLOAT32)PosData->LastUpdateAtt.Roll;
	Att1.z = (FLOAT32)PosData->LastUpdateAtt.Yaw;
	Att2.x = -(FLOAT32)(PosData->LastUpdateAttVel.Pitch * Delta1);
	Att2.y = -(FLOAT32)(PosData->LastUpdateAttVel.Roll * Delta1);
	Att2.z = (FLOAT32)(PosData->LastUpdateAttVel.Yaw * Delta1);
	mathEul_ToMatrix(&Mat1,EulOrdXYZs,&Att1);
	mathEul_ToMatrix(&Mat2,EulOrdXYZs,&Att2);
	math3dMatrixMultiply(&Mat3,&Mat2,&Mat1);
	mathEul_FromMatrix(&Att1,&Mat3,EulOrdXYZs);
	PosData->LastUpdateAtt.Pitch = Att1.x;
	PosData->LastUpdateAtt.Roll = Att1.y;
	PosData->LastUpdateAtt.Yaw = Att1.z;

	// accel
	// first rotate accel to vehicle's frame of reference

	// Velocity
	PrevVel = PosData->Vel;
	PosData->Vel.x += -PosData->Acc.x * Delta;
	PosData->Vel.y += PosData->Acc.y * Delta;
	PosData->Vel.z += -PosData->Acc.z * Delta;

	PrevVel1 = PosData->LastUpdateVel;
	PosData->LastUpdateVel.x += -PosData->LastUpdateAcc.x * Delta1;
	PosData->LastUpdateVel.y += PosData->LastUpdateAcc.y * Delta1;
	PosData->LastUpdateVel.z += -PosData->LastUpdateAcc.z * Delta1;

	// accel
	TempVec = PrevVel;
	mathGetExactAttFromVec1(&VAtt1,&TempVec);
	TempVec = PosData->Vel;
	mathGetExactAttFromVec1(&VAtt2,&TempVec);
	TempVec = PosData->Acc;
	TempVec.x = -TempVec.x;
	TempVec.z = -TempVec.z;
	mathCRotateVectorByAtt(&TempVec,&TempVec,&VAtt1);
	mathRotateVectorByAtt(&TempVec,&TempVec,&VAtt2);
	TempVec.x = -TempVec.x;
	TempVec.z = -TempVec.z;
	PosData->Acc = TempVec;

	TempVec = PrevVel1;
	mathGetExactAttFromVec1(&VAtt1,&TempVec);
	TempVec = PosData->LastUpdateVel;
	mathGetExactAttFromVec1(&VAtt2,&TempVec);
	TempVec = PosData->LastUpdateAcc;
	TempVec.x = -TempVec.x;
	TempVec.z = -TempVec.z;
	mathCRotateVectorByAtt(&TempVec,&TempVec,&VAtt1);
	mathRotateVectorByAtt(&TempVec,&TempVec,&VAtt2);
	TempVec.x = -TempVec.x;
	TempVec.z = -TempVec.z;
	PosData->LastUpdateAcc = TempVec;


	// Position
// Absolutely no idea why it works like this
// Seems to be due to some retardation in the FPS math, but fixing it is
// a real potential can of worms
if(Vehicle->VehicleType->Type == VEH_TYPE_PERSON)
	PosData->Pos.x += -PosData->Vel.x * Delta;
else
	PosData->Pos.x += PosData->Vel.x * Delta;

	PosData->Pos.y += PosData->Vel.y * Delta;
	PosData->Pos.z += PosData->Vel.z * Delta;

if(Vehicle->VehicleType->Type == VEH_TYPE_PERSON)
	PosData->LastUpdatePos.x += -PosData->LastUpdateVel.x * Delta1;
else
	PosData->LastUpdatePos.x += PosData->LastUpdateVel.x * Delta1;

	PosData->LastUpdatePos.y += PosData->LastUpdateVel.y * Delta1;
	PosData->LastUpdatePos.z += PosData->LastUpdateVel.z * Delta1;


	// override position if "stuck on ground" flag set
	if (Vehicle->StickVehicleToGround && Vehicle->VehicleObjectData != nullptr)
	{
	piVECTOR V1,V2;
	SINT32 DummyNum;

		trnGetWorldOriginOffset(&WorldOriginOffset);

		V1.x = (FLOAT32)(PosData->Pos.x - WorldOriginOffset.x);
		V1.y = (FLOAT32)(PosData->Pos.y - WorldOriginOffset.y);
		V1.z = -(FLOAT32)(PosData->Pos.z - -WorldOriginOffset.z) - 0.3f;

		V2 = V1;
		V2.z += 10000.0;

		if (trncdCollideDownMediumList(&V1,&V2) == TRUE)
		{

			V2.z += (FLOAT32)WorldOriginOffset.z;
			V2.z = -V2.z;

			PosData->LastUpdatePos.z = PosData->Pos.z = V2.z - (Vehicle->VehicleObjectData->LowestPoint);
		}
		else
		{
			if (trnopCollideDown(&V2, &DummyNum, &V1) == TRUE)
			{
				V2.z += (FLOAT32)WorldOriginOffset.z;
				V2.z = -V2.z;

				PosData->LastUpdatePos.z = PosData->Pos.z = V2.z - (Vehicle->VehicleObjectData->LowestPoint);

			}
		}

	}

	// smoothing
	if (frametime - PosData->CurrentUpdateTime > PosData->CumulativeDelta ||
		PosData->CumulativeDelta == 0)
	{
		PosData->RenderPos = PosData->Pos;

		if (Online == 1)
		{
			if (Vehicle->BumpedOff == 1)
			{
				PosData->RenderPos.z -= 20000.0;
			}
		}

		PosData->RenderAtt = PosData->Att;
	}
	else
	{
		if (PosData->CumulativeDelta == 0)
		{
			Delta = 1.0;
		}else
		{
			if(Vehicle->VehicleType->Type == VEH_TYPE_PERSON)
			{
			 	// Move the infantry to the correct position quicker.
				//OGS- This doesn't look healthy,
				// INFANTRY_UPDATE_FACTOR hasn't changed since we were
				// only sending 2 updates a second
				Delta = INFANTRY_UPDATE_FACTOR * (FLOAT64)(frametime - PosData->CurrentUpdateTime) / (FLOAT64)PosData->CumulativeDelta;
				if(Delta > 1.0)
					Delta = 1.0;
			}else
			{
				Delta = (FLOAT64)(frametime - PosData->CurrentUpdateTime) / (FLOAT64)PosData->CumulativeDelta;
			}
		}

		PosData->RenderPos.x = PosData->LastUpdatePos.x * (1.0 - Delta) + PosData->Pos.x * Delta;
		PosData->RenderPos.y = PosData->LastUpdatePos.y * (1.0 - Delta) + PosData->Pos.y * Delta;
		PosData->RenderPos.z = PosData->LastUpdatePos.z * (1.0 - Delta) + PosData->Pos.z * Delta;

		if (Delta <= 0.5)
		{
			Delta = 2 * (Delta * Delta);
		}
		else
		{
			Delta = (FLOAT32)(1.0 - Delta);
			Delta = (FLOAT32)(1.0 - 2 * (Delta * Delta));
		}
		_InterpolateAttitudes(&PosData->RenderAtt,&PosData->LastUpdateAtt,&PosData->Att,Delta);
	}

	if (!_finite(PosData->RenderPos.x) || !_finite(PosData->RenderPos.y) || !_finite(PosData->RenderPos.z))
	{
		PosData->RenderPos.x = PosData->Pos.x;
		PosData->RenderPos.y = PosData->Pos.y;
		PosData->RenderPos.z = PosData->Pos.z;
	}

#ifdef NEW_INF_PREDICTOR
}
#endif

	TempPiVec.x = (FLOAT32)PosData->RenderAtt.Pitch;
	TempPiVec.y = (FLOAT32)PosData->RenderAtt.Roll;
	TempPiVec.z = (FLOAT32)PosData->RenderAtt.Yaw;
	mathEul_ToMatrix(&PosData->AttMat,EulOrdXYZs,&TempPiVec);

	if (Vehicle->ModelData == nullptr || Vehicle->ModelData->MultiCrewPos != 0)
		vehUpdateWeapons(Vehicle);

	PosData->LastPredictorUpdateTime = frametime;
	PosData->LastUpdatePredictorUpdateTime = frametime;

	// relative position interpolation, quick and dirty, butt simple, here to there
	if(Vehicle->RideVehicleHostIndex > 0)
	{
		if(PosData->CurRelUpdateTime - PosData->LastRelUpdateTime > 0)
			Delta = (frametime - PosData->CurRelUpdateTime) / (FLOAT64)(PosData->CurRelUpdateTime - PosData->LastRelUpdateTime);
		else
			Delta = 1.0;

		if(Delta > 1.0)
			Delta = 1.0;
		else if(Delta < 0.0)
			Delta = 0.0;

		Vehicle->RelRidePosition.x = PosData->LastRelPos.x * (1 - Delta) + PosData->CurRelPos.x * Delta;
		Vehicle->RelRidePosition.y = PosData->LastRelPos.y * (1 - Delta) + PosData->CurRelPos.y * Delta;
		Vehicle->RelRidePosition.z = PosData->LastRelPos.z * (1 - Delta) + PosData->CurRelPos.z * Delta;
		Vehicle->RelRideYaw = PosData->LastRelYaw * (1 - Delta) + PosData->CurRelYaw * Delta;

		if(Vehicle->RelRideYaw >  PI) Vehicle->RelRideYaw = -PI2 + Vehicle->RelRideYaw;
		if(Vehicle->RelRideYaw < -PI) Vehicle->RelRideYaw =  PI2 + Vehicle->RelRideYaw;
	}

	// routine to stick fps butt in seat when riding vehicle
	if (Vehicle->VehicleType->Type == VEH_TYPE_PERSON &&
		Vehicle->VehicleObjectData != nullptr &&
		Vehicle->VehicleObjectData->PlaneObject != nullptr)
	{
		pObject = Vehicle->VehicleObjectData->PlaneObject;

		bool trooperOnMyLap = false;

		if(Vehicle->RideVehicleHostIndex > 0)
		{
			if(Vehicle->RideSeatNum == VEH_TROOPER_ON_DECK_RIDE_SEAT)
			{
				vehAttachToRidePoint(Vehicle);
			}
			else if (fpFPSIsTrooperSitting (pObject))
			{
				// check to make sure we're not sitting in a FPS's lap
				// first, make sure we're not the FPS
				if (!fpFPSIsObjectFPS(pObject))
				{
					// get the self vehicle, see if we're a trooper, if we are, get my seat number, and
					// make sure it's not the same as the one we're about to draw
					vehVehicle*selfVehicle = persGetPersonaVehiclePtr();
					if ((selfVehicle != nullptr) &&
						(selfVehicle->VehicleType != nullptr) &&
						(selfVehicle->VehicleType->Type == VEH_TYPE_PERSON) &&
						(selfVehicle->RideVehicleHostIndex > 0) &&
						(selfVehicle->RideSeatNum == Vehicle->RideSeatNum))
					{
						trooperOnMyLap = true;
					}
				}

				if(trooperOnMyLap || vehAttachToRidePoint(Vehicle) < 0)
					Vehicle->PosData.Pos.z = -3000.0;
			}
		}
	}

	// vehicle riding
	if (Vehicle->VehicleType->Type != VEH_TYPE_PERSON && Vehicle->RideVehicleHostIndex > 0)
		vehAttachToRidePoint(Vehicle);

	return 0;
}

#if(0)
static UINT32 _InterpolateVectorsByAngles(piVECTOR* V3,piVECTOR* V1,piVECTOR* V2,FLOAT32 Delta)
{
piVECTOR Normal;
piVECTOR Att;
piVECTOR TempVec;
piMATRIX Mat,MatInv;
FLOAT32 Angle1,Angle2;
FLOAT32 Angle;
FLOAT32 D;

	mathVectDoCrossProduct(&Normal,V1,V2);

	Att.x = Att.y = Att.z = 0.0f;
	Att.z = (FLOAT32)atan2(Normal.x,Normal.y);
	Att.x = (FLOAT32)atan2(sqrt(Normal.y * Normal.y + Normal.x * Normal.x),Normal.z);
	mathEul_ToMatrixFast(&Mat,EulOrdXYZs,&Att);
	math3dInvertMatrix(&MatInv,&Mat);

	math3dVectorMultiply(&TempVec,V1,&MatInv);
	Angle1 = (FLOAT32)atan2(TempVec.x,TempVec.y);

	math3dVectorMultiply(&TempVec,V2,&MatInv);
	Angle2 = (FLOAT32)atan2(TempVec.x,TempVec.y);

	// adjust for overlap discrepancy
	D = Angle2 - Angle1;
	if (D > PI)
	{
		D -= 2 * PI;
	}
	if (D < -PI)
	{
		D += 2 * PI;
	}

	Angle = Angle1 + D * (1.0f - Delta);

	TempVec.z = 0.0f;
	TempVec.x = (FLOAT32)sin(Angle);
	TempVec.y = (FLOAT32)cos(Angle);

	math3dVectorMultiply(V3,&TempVec,&Mat);


	return 0;
}
#endif

// interpolates
// delta of 0 means all Att1
// delta of 1.0 means all Att2
static UINT32 _InterpolateAttitudes(FLOAT64ATT * RetAtt,FLOAT64ATT * InAtt1,FLOAT64ATT * InAtt2,FLOAT64 Delta)
{

piQUAT quat1,quat2,quat3;
piVECTOR Att1,Att2,Att3;
piMATRIX Mat1,Mat2,Mat3;


	Att1.x = (FLOAT32)InAtt1->Pitch;
	Att1.y = (FLOAT32)InAtt1->Roll;
	Att1.z = (FLOAT32)InAtt1->Yaw;

	Att2.x = (FLOAT32)InAtt2->Pitch;
	Att2.y = (FLOAT32)InAtt2->Roll;
	Att2.z = (FLOAT32)InAtt2->Yaw;

	mathEul_ToMatrix(&Mat1,EulOrdXYZs,&Att1);
	mathEul_ToMatrix(&Mat2,EulOrdXYZs,&Att2);

	math3dMatrixToQuaternion(&quat1,&Mat1);
	math3dMatrixToQuaternion(&quat2,&Mat2);

	math3dQuatSlerp(&quat3,&quat1,&quat2,(FLOAT32)Delta);

	math3dQuaternionToMatrix(&Mat3,&quat3);

	mathEul_FromMatrix(&Att3,&Mat3,EulOrdXYZs);

	RetAtt->Pitch = Att3.x;
	RetAtt->Roll = Att3.y;
	RetAtt->Yaw = Att3.z;


	return 0;
}

UINT32 vehupdateDisplayVehicle(vehVehicle* Vehicle)
{
	if (Vehicle->VehicleObjectData == nullptr || Vehicle->VehicleObjectData->PlaneObject == nullptr)
		return 0;

	/*
	if(Vehicle->spaceID)
	{
		bool enabled = (bool)dGeomIsEnabled((dGeomID)Vehicle->spaceID);

		if(Vehicle->VehicleObjectData->PlaneObject->distsqr > 1500.0f && enabled)
			dGeomDisable((dGeomID)Vehicle->spaceID);
		else if(Vehicle->VehicleObjectData->PlaneObject->distsqr <= 1500.0f && !enabled)
			dGeomEnable((dGeomID)Vehicle->spaceID);
	}
	*/

	mlMoveAndRotateObject(Vehicle->VehicleObjectData->PlaneObject, &Vehicle->PosData.RenderPos, &Vehicle->PosData.RenderAtt);

	if (Vehicle->VehicleType->Type != VEH_TYPE_PERSON)
	{
		mlApplyVelocityToObject(Vehicle->VehicleObjectData->HitMap ? Vehicle->VehicleObjectData->HitMap : Vehicle->VehicleObjectData->PlaneObject,
							&Vehicle->PosData.Vel);
	}

	return 0;
}

UINT32 vehUpdateWeapons(vehVehicle* Vehicle)
{
SINT32 i,j;
FLOAT64VECTOR RecoilForce;
FLOAT64VECTOR FirePos;
FLOAT64VECTOR FireVel;
piMATRIX Matrix1;
piVECTOR TempVec;
FLOAT64 FireDelta;
FLOAT64 CurrentTime;
UINT32 AmmoFlags;
FLOAT64VECTOR RelativePos;
FLOAT64VECTOR WorldOriginOffset;
LOGICAL WeaponFired;
SINT32 SoundNum;
OBJECT * TempObj;

// Weapon Firing Stuff
FLOAT64VECTOR	vecVehicleVel; // For recoil, etc.
piMATRIX		FirePointMatrix, TempMatrix;
FLOAT64VECTOR	FirePositionVector;
FLOAT64VECTOR	FlashPositionVector;
FLOAT64			LastFireTime = 0.0f;
SINT32			RecoilNum = 0;

    memset(&vecVehicleVel, 0, sizeof(FLOAT64VECTOR));
    memset(&FirePositionVector, 0, sizeof(FLOAT64VECTOR));
    memset(&FlashPositionVector, 0, sizeof(FLOAT64VECTOR));

    FirePointMatrix = IdentityMatrix;
    TempMatrix = IdentityMatrix;

	if (Vehicle == nullptr)
	{
		return 0;
	}

	if (Vehicle->VehicleObjectData == nullptr)
	{
		return 0;
	}

	trnGetWorldOriginOffset(&WorldOriginOffset);
	WorldOriginOffset.z = -WorldOriginOffset.z;

	if (Vehicle->VehicleType == nullptr)
	{
		return 0;
	}

	vehVehicle* PersonaVehicle = persGetPersonaVehiclePtr();
	if (PersonaVehicle == nullptr)
	{
		return 0;
	}

	if (Vehicle->VehicleType->Type == VEH_TYPE_PERSON)
		return 0;

	//else
	{
		for (i=0;i<VEH_MAX_MOUNTS;i++)
		{
			// ghost fire can only occur for player's vehicle if he doesn't control that position
			if (Vehicle->ModelData != nullptr)
			{
				if (!(Vehicle->ModelData->FireMask & (1 << i)))
				{
					continue;
				}
			}

			WeaponFired = FALSE;
			LastFireTime = 0.0;
			if (Vehicle->MountIsFiring[i] == TRUE)
			{
				for (j=0;j<VEH_MAX_WEAPONS_PER_MOUNT;j++)
				{
					if (Vehicle->VehicleObjectData->WeaponFirePoints[i][j] != nullptr)
//					 && Vehicle->WeaponIsHidden[i][j] == 0)
					{
						// calculate fire position
						Matrix1 = IdentityMatrix;
						TempVec.x = Matrix1._41 = Vehicle->VehicleObjectData->WeaponFirePoints[i][j]->dof->dofmatrix2._41;
						TempVec.y = Matrix1._42 = Vehicle->VehicleObjectData->WeaponFirePoints[i][j]->dof->dofmatrix2._42;
						TempVec.z = Matrix1._43 = Vehicle->VehicleObjectData->WeaponFirePoints[i][j]->dof->dofmatrix2._43;

						mlUtilityGetModelPositionFromObjectMatrix(&RelativePos,&Matrix1);
						mathVectorMultiply64(&FirePos,&RelativePos,&Vehicle->PosData.AttMat);

						mathAddVectors(&FirePos,&FirePos,&Vehicle->PosData.RenderPos);

						// fire velocity
						if (Vehicle == PersonaVehicle && PersonaVehicle->ModelData != nullptr &&
							PersonaVehicle->ModelData->MultiCrewPos < 2)
						{
							FireVel = PersonaVehicle->ModelData->VehicleRealVel;
						}
						else
						{
							FireVel = Vehicle->PosData.Vel;
						}

						// fire attitude
						Matrix1 = Vehicle->VehicleObjectData->WeaponFirePoints[i][j]->dof->dofmatrix2;
						math3dMatrixMultiply(&Matrix1,&Matrix1,&(Vehicle->VehicleObjectData->WeaponFirePoints[i][j]->mat));
						Matrix1._13 = -Matrix1._13;
						Matrix1._23 = -Matrix1._23;
						Matrix1._33 = -Matrix1._33;
						Matrix1._43 = -Matrix1._43;

						Matrix1._41 = Matrix1._42 = Matrix1._43 = 0.0f;

						// flags
						AmmoFlags = 0;
						AmmoFlags |= WEAP_AMMOFLAG_USE_SHORTCOLLIDELIST;
						AmmoFlags |= WEAP_AMMOFLAG_DONT_SEND_DAMAGE;
//						AmmoFlags |= WEAP_AMMOFLAG_CANTHITPLAYER;
//        				AmmoFlags |= WEAP_AMMOFLAG_DONTCHECKVEHICLES;

						// other stuff
						FireDelta = (frametime - Vehicle->PosData.LastFireTime) / 1000.0;
						if(Vehicle->PosData.LastFireTime == 0) FireDelta = 0.0f;
						CurrentTime = frametime / 1000.0;
						// fire the weapon, using ghost state
						RecoilForce.x = RecoilForce.y = RecoilForce.z = 0.0f;
						weapFireWeapon(&RecoilForce,
										&Vehicle->Weapons[i][j],
										&FirePos,
										&FireVel,
										&Matrix1,
										FireDelta,
										CurrentTime,
										0, // ghost can't damage stuff, only bullet holes
										1, // this firing is from a ghost
										AmmoFlags,
										Vehicle->VehicleObjectData->WeaponFirePoints[i][j],
										&RelativePos,
										&LastFireTime,
										Vehicle);

						if (Vehicle->Weapons[i][j].WeaponType->HideOnLastDrop == 1)
						{
							Vehicle->WeaponIsHidden[i][j] = 1;
							if (Vehicle->VehicleObjectData->WeaponConnectPoints[i][j] != nullptr)
							{
								ogrpHideObjectAndSubtree(Vehicle->VehicleObjectData->WeaponConnectPoints[i][j]);
							}
						}

						if ((RecoilForce.x * RecoilForce.x) > 0.00001 ||
							(RecoilForce.y * RecoilForce.y) > 0.00001 ||
							(RecoilForce.z * RecoilForce.z) > 0.00001)
						{
							WeaponFired = TRUE;
						}
					}
				}
				// if the first weapon in the mount is single-shot
				// then we stop firing after this round

				if (Vehicle->VehicleObjectData->WeaponFirePoints[i][0] != nullptr)
				{
					if (Vehicle->Weapons[i][0].WeaponType->FireType == WEAP_FIRETYPE_SINGLESHOT)
					{
						if ((RecoilForce.x * RecoilForce.x) > 0.00001 ||
							(RecoilForce.y * RecoilForce.y) > 0.00001 ||
							(RecoilForce.z * RecoilForce.z) > 0.00001)
						{
							//Vehicle->MountIsFiring[i] = (!Online && Vehicle->isSTO()) ? 1 : 0; // allow sto marketing vehicles to continue to fire
							Vehicle->MountIsFiring[i] = 0;

							// play fire sound
							RelativePos.x = Vehicle->VehicleObjectData->WeaponFirePoints[i][0]->dof->dofmatrix2._41;
							RelativePos.y = Vehicle->VehicleObjectData->WeaponFirePoints[i][0]->dof->dofmatrix2._42;
							RelativePos.z = Vehicle->VehicleObjectData->WeaponFirePoints[i][0]->dof->dofmatrix2._43;
							mathVectorMultiply64(&RelativePos,&RelativePos,&Vehicle->VehicleObjectData->WeaponFirePoints[i][0]->matint);
							SoundNum = Vehicle->VehicleType->Loadout[Vehicle->LoadoutNum].MountSoundNum[i] - 1;
							if (Vehicle->VehicleObjectData->HitMap == nullptr)
							{
								TempObj = Vehicle->VehicleObjectData->PlaneObject;
							}
							else
							{
								TempObj = Vehicle->VehicleObjectData->HitMap;
							}
							if (Vehicle->ModelData != nullptr && SoundNum >= 0)
							{
                                FLOAT32 fMin, fMax;

                                SINT32 iID = fxSndGetNewSound();

                                if(iID > 0)
                                {
                                    fxSndSetSoundName(iID, (SCHAR*)Vehicle->VehicleType->MountSounds[SoundNum]);
                                    fxSndSetPriority(iID, IDE_TP_PRIORITY_HIGH);
                                    fxSndSetSoundType(iID, TYPE_FINITE_SOUND);
                                    fxSndSetPitch(iID, 1.0f);
                                    fxSndSetVolume(iID, 1.0f);
                                    fxSndSetPosition(iID, RelativePos.x, RelativePos.y, RelativePos.z);
                                    fxSndSetEmitterObject(iID, TempObj);
                                    fxSndSetPlayType(iID, DEF_PLAYTYPE_OBJECT);
                                    fxSndPushFlags(iID, DEF_SOUND_STOPS_AMBIENCE);

                                    _GetSoundDistance(&fMin, &fMax, &Vehicle->VehicleType->Type);

                                    fxSndSetMinDistance(iID, fMin);
                                    fxSndSetMaxDistance(iID, fMax);
                                    fxSndInitSound(iID);
                                }
							}
							else if(SoundNum >= 0)
							{
                                _playWeaponFireSound((SCHAR*)Vehicle->VehicleType->MountSounds[SoundNum],
                                                     TempObj,
                                                     &RelativePos,
                                                     Vehicle->VehicleType->Type);
                            }
						}
					}
					else
					{
						if (Vehicle->StateAnimation.MountSoundPlaying[i] == FALSE)
						{
							SoundNum = Vehicle->VehicleType->Loadout[Vehicle->LoadoutNum].MountSoundNum[i] - 1;
							if (SoundNum >= 0)
							{
								if (WeaponFired == TRUE)
								{
									if (Vehicle->StateAnimation.MountSoundId[SoundNum] == 0)
									{
										RelativePos.x = Vehicle->VehicleObjectData->WeaponFirePoints[i][0]->dof->dofmatrix2._41;
										RelativePos.y = Vehicle->VehicleObjectData->WeaponFirePoints[i][0]->dof->dofmatrix2._42;
										RelativePos.z = Vehicle->VehicleObjectData->WeaponFirePoints[i][0]->dof->dofmatrix2._43;
										mathVectorMultiply64(&RelativePos,&RelativePos,&Vehicle->VehicleObjectData->WeaponFirePoints[i][0]->matint);
										if (Vehicle->VehicleObjectData->HitMap == nullptr)
										{
											TempObj = Vehicle->VehicleObjectData->PlaneObject;
										}
										else
										{
											TempObj = Vehicle->VehicleObjectData->HitMap;
										}
										if (Vehicle->ModelData != nullptr && SoundNum >= 0)
										{
                                            FLOAT32 fMin, fMax;
                                            SINT32 iID = fxSndGetNewSound();

                                            if(iID > 0)
                                            {
                                                fxSndSetSoundName(iID, (SCHAR*)Vehicle->VehicleType->MountSounds[SoundNum]);
                                                fxSndSetPriority(iID, IDE_TP_PRIORITY_HIGH);
                                                fxSndSetSoundType(iID, TYPE_INFINITE_SOUND);
                                                fxSndSetPitch(iID, 1.0f);
                                                fxSndSetVolume(iID, 1.0f);
                                                fxSndSetPosition(iID, RelativePos.x, RelativePos.y, RelativePos.z);
                                                fxSndSetEmitterObject(iID, TempObj);
                                                fxSndSetPlayType(iID, DEF_PLAYTYPE_OBJECT);
                                                fxSndPushFlags(iID, DEF_SOUND_STOPS_AMBIENCE);

                                                _GetSoundDistance(&fMin, &fMax, &Vehicle->VehicleType->Type);

                                                fxSndSetMinDistance(iID, fMin);
                                                fxSndSetMaxDistance(iID, fMax);

                                                if(fxSndInitSound(iID))
                                                {
                                                    Vehicle->StateAnimation.MountSoundId[SoundNum] = iID;
                                                }
                                            }
										}
										else if(SoundNum >= 0)
										{
                                            FLOAT32 fMin, fMax;
                                            SINT32 iID = fxSndGetNewSound();

                                            if(iID > 0)
                                            {
                                                fxSndSetSoundName(iID, (SCHAR*)Vehicle->VehicleType->MountSounds[SoundNum]);
                                                fxSndSetPriority(iID, IDE_TP_PRIORITY_HIGH);
                                                fxSndSetSoundType(iID, TYPE_INFINITE_EXTERNAL_SOUND);
                                                fxSndSetPitch(iID, 1.0f);
                                                fxSndSetVolume(iID, 1.0f);
                                                fxSndSetPosition(iID, RelativePos.x, RelativePos.y, RelativePos.z);
                                                fxSndSetEmitterObject(iID, TempObj);
                                                fxSndSetPlayType(iID, DEF_PLAYTYPE_OBJECT);
                                                fxSndPushFlags(iID, DEF_SOUND_STOPS_AMBIENCE);

                                                _GetSoundDistance(&fMin, &fMax, &Vehicle->VehicleType->Type);

                                                fxSndSetMinDistance(iID, fMin);
                                                fxSndSetMaxDistance(iID, fMax);

                                                if(fxSndInitSound(iID))
                                                {
                                                    Vehicle->StateAnimation.MountSoundId[SoundNum] = iID;
                                                }
                                            }
										}
									}
									Vehicle->StateAnimation.MountSoundSemaphore[SoundNum]++;
									Vehicle->StateAnimation.MountSoundPlaying[i] = TRUE;
								}
							}
						}
					}
				}
			}
			else
			{
				if (Vehicle->Weapons[i][0].WeaponType != nullptr)
				{
					if (Vehicle->Weapons[i][0].WeaponType->FireType != WEAP_FIRETYPE_SINGLESHOT)
					{
						if (Vehicle->StateAnimation.MountSoundPlaying[i] == TRUE)
						{
							SoundNum = Vehicle->VehicleType->Loadout[Vehicle->LoadoutNum].MountSoundNum[i] - 1;
							Vehicle->StateAnimation.MountSoundPlaying[i] = FALSE;
							Vehicle->StateAnimation.MountSoundSemaphore[SoundNum]--;
							if (Vehicle->StateAnimation.MountSoundSemaphore[SoundNum] <= 0)
							{
								fxSndStopSoundDelayedFromStart(Vehicle->StateAnimation.MountSoundId[SoundNum]);
								Vehicle->StateAnimation.MountSoundSemaphore[SoundNum] = 0;
								Vehicle->StateAnimation.MountSoundId[SoundNum] = 0;
							}
						}
					}
				}
			}

			// recoil animation
			if (WeaponFired == TRUE)
			{
				// trigger recoil
				if (Vehicle != nullptr)
				{
					RecoilNum = Vehicle->VehicleType->Loadout[Vehicle->LoadoutNum].RecoilNum[i]-1;
					if (RecoilNum >= 0)
					{
						Vehicle->StateAnimation.RecoilStartTime[RecoilNum] =
									(SINT32)((LastFireTime) * 1000.0);
						Vehicle->StateAnimation.RecoilTime[RecoilNum] =
									Vehicle->VehicleType->Loadout[Vehicle->LoadoutNum].RecoilTime[i];
					}
				}
			}
		}
	}

	Vehicle->PosData.LastFireTime = frametime;

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Function: vehResupplyWeapons
// Description: resupplies all of the vehicle's mounted weapons
// limited to clipsize resupply per call (called per "tick" elsewhere)
//////////////////////////////////////////////////////////////////////
UINT32 vehResupplyWeapons(struct vehVehicle* Vehicle)
{
	UINT32 numResupplied = 0;

	if(Vehicle->VehicleType->Type == VEH_TYPE_PERSON)
	{
		// handle infantry
		for (int i = 0; i < VEH_MAX_MOUNTS; i++)
		{
			for (int j = 0; j < VEH_MAX_WEAPONS_PER_MOUNT; j++)
			{
				if(weapResupplyWeapon(&Vehicle->Weapons[i][j]))
					++numResupplied;
			}
		}
	}
	else
	{
		// handle vehicles
		mlComponent* Component;
		mlComponentListIterator it;
		FLOAT32 percentOfTotal = (Vehicle->VehicleType->Type == VEH_TYPE_AIR) ? 1.0f: 0; // hardcoded for now

		mlComponentListBegin(Vehicle->ModelData->ComponentList, &it);
		for(; it.comp; mlComponentListNext(&it))
		{
			Component = it.comp;

			if(Component->Type == ML_COMPONENT_WEAPONMOUNT)
			{
				mlWeaponMount* Mount = (mlWeaponMount*)(Component->SpecialData);

				if(Mount)
				{
					for(int i = 0; i < ML_MAX_WEAPONS_PER_MOUNT; i++)
					{
						if(weapResupplyWeapon(&Mount->Weapons[i], percentOfTotal))
						{
							// make sure the weapon is again shown
							ogrpShowObjectAndSubtree(Mount->WeaponConnectPoints[i]);

							// increment the number of weapons resupplied
							++numResupplied;
						}
					}
				}
			}
		}
	}

	return numResupplied;
}

//////////////////////////////////////////////////////////////////////
// Function: _playWeaponFireSound
// Description:
//////////////////////////////////////////////////////////////////////
void _playWeaponFireSound(SCHAR* sound,
                          OBJECT* o,
                          FLOAT64VECTOR* pos,
                          UINT32 type)
{
    SINT32 ID = fxSndGetNewSound();

    if(ID < 0) return;

    FLOAT32 fMin, fMax;

    // Check if distance file exists.
    SCHAR* p = nullptr;
    static SCHAR distSound[128];
    LOGICAL hasDistant = FALSE;
  //  LOGICAL canPlayDistantSound = FALSE;

    strncpy(distSound, sound, sizeof(distSound) - 1);
	distSound[sizeof(distSound)-1] = 0;

    p = strstr(distSound, ".wav");

    if(p)
    {
        *p = 0;
        strcat(distSound, "_distant.wav");

        if(fioAccess(distSound))
        {
            hasDistant = TRUE;
        }
    }

    _GetSoundDistance(&fMin, &fMax, &type);

    if(hasDistant)
    {
        piVECTOR v;

        v.x = (FLOAT32)pos->x;
        v.y = (FLOAT32)pos->y;
        v.z = (FLOAT32)pos->z;

        math3dVectorMultiply(&v, &v, &o->mat);

        FLOAT32 z = math3dDotProduct(&v, &playdir) + playtrans.z;

        if(z > fMax) return;

        if(z > fMax/4 && z <= fMax)
        {
            fMin = 150.0f;
            sound = distSound;
        }
    }

    fxSndSetSoundName(ID, sound);
    fxSndSetPriority(ID, IDE_TP_PRIORITY_HIGH);
    fxSndSetSoundType(ID, TYPE_FINITE_EXTERNAL_SOUND);
    fxSndSetPitch(ID, 1.0f);
    fxSndSetVolume(ID, 1.0f);
    fxSndSetPosition(ID, pos->x, pos->y, pos->z);
    fxSndSetEmitterObject(ID, o);
    fxSndSetPlayType(ID, DEF_PLAYTYPE_OBJECT);
    fxSndPushFlags(ID, DEF_SOUND_STOPS_AMBIENCE);

    fxSndSetMinDistance(ID, fMin);
    fxSndSetMaxDistance(ID, fMax);
    fxSndInitSound(ID);
}

//////////////////////////////////////////////////////////////////////
// Function: _GetSoundDistance
// Description: Retrieve the min and max distance for mount sounds.
//////////////////////////////////////////////////////////////////////
LOGICAL _GetSoundDistance(FLOAT32* pMin, FLOAT32* pMax, UINT32* pType)
{
    switch(*pType)
    {
    case VEH_TYPE_PERSON:
        {
            *pMax = 1609.0f;
            *pMin = 25.0f;
            break;
        }
    case VEH_TYPE_GROUND:
        {
            *pMax = 1609.0f;
            *pMin = 50.0f;
            break;
        }
    case VEH_TYPE_AIR:
        {
            *pMax = 1609.0f;
            *pMin = 50.0f;
            break;
        }
    case VEH_TYPE_SEA:
        {
            *pMax = 1609.0f;
            *pMin = 50.0f;
            break;
        }
    default:
        {
            return FALSE;
        }
    }

    return TRUE;
}

// called whenever the riding vehicle is changed so we can reinitialize
// the appropriate interpolation code
static void vehUpdateRideChanged(vehVehicle* Vehicle, UINT32 newRideVehicleIdx)
{
	if(newRideVehicleIdx == 0) // unriding
	{
// Thank you Oliver
/*
		// reset abs pos interpolation god knows what all these variables do
		Vehicle->PosData.CumulativeDelta = 0;
		Vehicle->PosData.LastSenderUpdateTime = frametime;
		Vehicle->PosData.CurrentSenderUpdateTime = frametime;
		Vehicle->PosData.LastAnyUpdateTime = frametime;
		Vehicle->PosData.LastUpdateTime = frametime;
		Vehicle->PosData.CurrentUpdateTime = frametime;
		Vehicle->PosData.LastPredictorUpdateTime = frametime;
		Vehicle->PosData.LastUpdatePredictorUpdateTime = frametime;
		Vehicle->PosData.LastDelta = 0;

		Vehicle->PosData.LastUpdateAtt = Vehicle->PosData.Att = Vehicle->PosData.RenderAtt;
		Vehicle->PosData.LastUpdatePos = Vehicle->PosData.Pos = Vehicle->PosData.RenderPos;
		Vehicle->PosData.LastUpdateVel = Vehicle->PosData.LastLastUpdateVel = Vehicle->PosData.Vel;
		Vehicle->PosData.LastUpdateAcc.x = Vehicle->PosData.LastUpdateAcc.y = Vehicle->PosData.LastUpdateAcc.z = 0;
		Vehicle->PosData.Acc.x = Vehicle->PosData.Acc.y = Vehicle->PosData.Acc.z = 0;

		Vehicle->PosData.LastUpdateAttVel.Roll = Vehicle->PosData.LastUpdateAttVel.Pitch = Vehicle->PosData.LastUpdateAttVel.Yaw = 0;
		Vehicle->PosData.AttVel.Roll = Vehicle->PosData.AttVel.Pitch = Vehicle->PosData.AttVel.Yaw = 0;
*/
	}else
	{
		// reset rel pos interpolation
		Vehicle->PosData.LastRelUpdateTime = Vehicle->PosData.CurRelUpdateTime = frametime;
		Vehicle->PosData.LastRelPos.x = Vehicle->PosData.LastRelPos.y = Vehicle->PosData.LastRelPos.z = 0;
		Vehicle->PosData.CurRelPos.x = Vehicle->PosData.CurRelPos.y = Vehicle->PosData.CurRelPos.z = 0;
		Vehicle->PosData.LastRelYaw = Vehicle->PosData.CurRelYaw = 0;
	}

	Vehicle->RideVehicleHostIndex = newRideVehicleIdx;
}

void _processBuildingDamage (vehVehicle& Vehicle)
{
	CP_HIT_HEADER	damagePacket;
	CP_OBJ_HIT		damageList[VEH_MAX_BUILDINGS_HIT];
	SINT32 i;

	// building hit stuff
	if (Vehicle.DamageQue.Count <= 0)
		return;

	memset (damageList, 0, sizeof (damageList));

//	damagePacket.country = Vehicle->
	damagePacket.list = damageList;
	if(Vehicle.DamageQue.Count > VEH_MAX_BUILDINGS_HIT)
	{
		damagePacket.numHits = VEH_MAX_BUILDINGS_HIT;
		Write_Debug("DamageQue.count = %d, exceeded VEH_MAX_BUILDINGS_HIT = %d", Vehicle.DamageQue.Count, VEH_MAX_BUILDINGS_HIT);
	}
	else
		damagePacket.numHits = Vehicle.DamageQue.Count;

	for (i = 0; i < damagePacket.numHits; i++)
	{
		damagePacket.list[i].object.cell.ox = Vehicle.DamageQue.SuperCellX[i] << 4;
		damagePacket.list[i].object.cell.oy = Vehicle.DamageQue.SuperCellY[i] << 4;
		damagePacket.list[i].object.id = Vehicle.DamageQue.Id[i];
		damagePacket.list[i].object.objType = 0;

		// used to be 4000000000.0, now 2147483647 due to change from UINT to SINT
		SINT64 damage = (SINT64)Vehicle.DamageQue.Damage[i];
		static const SINT64 cutoffJoules = 2100000000LL;
		if ( damage > 0 && damage > cutoffJoules )
			damage = cutoffJoules;
		else if ( damage < 0 && damage < -cutoffJoules )
			damage = -cutoffJoules;
		damagePacket.list[i].damage = damage;

		Write_Debug("WEAP Sending building hit message, scx = %d, scy = %d, id = %d, damage = %lf",
							Vehicle.DamageQue.SuperCellX[i],
							Vehicle.DamageQue.SuperCellY[i],
							Vehicle.DamageQue.Id[i],
							Vehicle.DamageQue.Damage[i]);
	}

	teulSend (PlayerClient()->CellConnection(),commSTRAT_DAMAGE,&damagePacket);

	// clear the damage queue once sent
	Vehicle.DamageQue.Count = 0;
	memset(&Vehicle.DamageQue.Damage[0],0,sizeof(FLOAT32) * VEH_MAX_BUILDINGS_HIT);
}
