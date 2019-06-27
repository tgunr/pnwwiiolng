#include "syscfg.h" // Path is project specific.

#include <math.h>

#include "pi/pi.h"
#include "include/vars.h"

#include "veh/vehmain.h"
#include "veh/vehanim.h"
#include "veh/vehcntrl.h"
#include "ml/mlobject.h"
#include "ml/mlgauges.h"
#include "ml/mlutility.h"
#include "math/math3d.h"
#include "math/matheul.h"
#include "math/mathvect.h"
#include "math/mathnum.h"
#include "ob/obmain.h"
#include "ld/ldmdl.h"
#include "rn/rnmain.h"
#include "trn/trnmain.h"
#include "trn/trnop.h"
#include "trn/trncd.h"
#include "fio/fio.h"
#include "locl/loclMain.h"
#include "pers/persmain.h"
#include "gr2/gr2Anim.h"
#include "ogrp/ogrptype.h"
#include "fx/fxMgr.h"
#include "fx/fxExp.h"
#include "fx/fxSnd.h"
#include "fx/FXInterface.h"
#include "veh/vehVisDamage.h"
#include "veh/vehfx.h"
#include "ez/EZManager.h"
#include "veh/vehAnimEnum.h"

// UINT32 static functions
static UINT32 _UpdateVehicleTracks(vehVehicle* Vehicle,
								   UINT32 ActuateTracks);
static UINT32 _SetVehicleDOFState(vehVehicle* Vehicle,
								  vehanimVehicleStateType * StateType,
								  LOGICAL State, LOGICAL LastState);
static UINT32 _SetDOF(OBJECT * Object,
					  vehanimVehicleStateAnimations* StateAnims,
					  vehType * VehicleType,
					  vehVehicle* Vehicle);
static UINT32 _SetDOFSubObjectVals(OBJECT * Object,
								   vehanimVehicleStateAnimations* StateAnims,
								   vehType * VehicleType,
								   vehVehicle* Vehicle);
static UINT32 _UpdateSmokes(vehVehicle* Vehicle);
static UINT32 _HandleSound(vehVehicle* Vehicle,
							vehanimVehicleStateType * StateType,
							LOGICAL State,
							LOGICAL LastState);
static UINT32 _UpdateRecoils(vehVehicle*Vehicle);
static void _EngineSoundStopped(void* data);
static UINT32 _HandleVisDamage(vehVehicle*, vehanimVehicleStateType*, LOGICAL State, LOGICAL LastState);
static UINT32 _DoExhaustState(vehVehicle*pVeh, vehanimVehicleStateType* pStateType, LOGICAL bState, LOGICAL LastState);
static UINT32 _DoTreadDustState(vehVehicle*pVeh, vehanimVehicleStateType* pStateType, LOGICAL bState, LOGICAL LastState);
static UINT32 _DoWingVortexState(vehVehicle*pVeh, vehanimVehicleStateType* pStateType, LOGICAL bState, LOGICAL bLastState);

static LOGICAL _GetSoundDistance(SINT32 iID, const UINT32* pType);

static void _DoPlaneCrashFX(vehVehicle*pVehicle);
static void _Set3rdPersonPitch(vehVehicle*pVehicle, SINT32 iIndex);
#if(0)
static void _Set3rdPersonVolume(vehVehicle*pVehicle, SINT32 iIndex);
#endif
static LOGICAL _LeaveStub(vehVehicle* Vehicle, vehanimVehicleStateType* StateType);

#define USE_FX2 0
#if USE_FX2
#include "fx2/effect.h"
#endif

UINT32 vehAnimInit()
{
	return 0;
}

UINT32 vehAnimKill()
{
	return 0;
}

UINT32 vehInitVehicleAnimation(vehVehicle* Vehicle)
{
	return 0;
}

UINT32 vehUpdateVehicleAnimation(vehVehicle* Vehicle)
{
	// check to see that an object exists for the vehicle
	if (Vehicle->VehicleObjectData == nullptr)
	{
		return 0;
	}
	if (Vehicle->VehicleObjectData->PlaneObject == nullptr)
	{
		return 0;
	}

	mlGetTrackPosition(&Vehicle->StateAnimation.TrackPos[0],0);
	mlGetTrackPosition(&Vehicle->StateAnimation.TrackPos[1],1);

	_UpdateVehicleTracks(Vehicle,1);

	_UpdateRecoils(Vehicle);

	return 0;
}

UINT32 vehKillVehicleAnimation(vehVehicle* Vehicle)
{
	SINT32 i;
	// kill engine sound
	for(i = 0; i < ML_MAX_ENGINES; i++)
	{
		for(int j = 0; j < 4; j++)
		{
			if(Vehicle->StateAnimation.EngineSoundId[i][j] > 0)
				fxSndStopSound(Vehicle->StateAnimation.EngineSoundId[i][j]);
		}
	}

	if (Vehicle->StateAnimation.StukaSoundId)
		fxSndStopSound(Vehicle->StateAnimation.StukaSoundId);

	// kill weapon sounds
	for(i = 0; i < VEH_MAX_MOUNTS; i++)
	{
		if (Vehicle->StateAnimation.MountSoundId[i])
			fxSndStopSound(Vehicle->StateAnimation.MountSoundId[i]);
	}

	return 0;
}

UINT32 vehAnimUpdateHostControlledVehicleAnimations(vehVehicle* Vehicle)
{
FLOAT32 Speed;
piVECTOR TempVec;
piVECTOR Att1;
piMATRIX TempMat;
FLOAT32 Delta;
UINT32 i;
FLOAT32 TempNum;
FLOAT32 MultFactor;


	if (Vehicle == nullptr)
	{
		return 0;
	}

	if (Vehicle->VehicleObjectData == nullptr)
	{
		return 0;
	}

	if (Vehicle->VehicleType->Type == VEH_TYPE_PERSON)
	{
		return 0;
	}

	Vehicle->StateAnimation.UseCalculatedTrackAndWheel = 1;

	Delta = (frametime - Vehicle->StateAnimation.LastUpdateTime) / 1000.0f;

	TempVec.x = TempVec.z = 0.0f;
	TempVec.y = 1.0f;
	Att1.x = (FLOAT32)Vehicle->PosData.Att.Pitch;
	Att1.y = (FLOAT32)Vehicle->PosData.Att.Roll;
	Att1.z = (FLOAT32)Vehicle->PosData.Att.Yaw;
	mathEul_ToMatrixFast(&TempMat,EulOrdXYZs,&Att1);

	math3dVectorMultiply(&TempVec,&TempVec,&TempMat);

	Speed = (FLOAT32)mathVectDotProduct(&Vehicle->PosData.Vel,&TempVec);

	// update tracks
	TempNum = Vehicle->StateAnimation.TrackPos[0] += Speed * Delta;
	TempNum = (FLOAT32)(TempNum - floor(TempNum));
	Vehicle->StateAnimation.TrackPos[0] = TempNum;
	TempNum = Vehicle->StateAnimation.TrackPos[1] + Speed * Delta * 1.001f; // vary slightly
	TempNum = (FLOAT32)(TempNum - floor(TempNum));
	Vehicle->StateAnimation.TrackPos[1] = TempNum;

	_UpdateVehicleTracks(Vehicle, Vehicle->VehicleType->Type == VEH_TYPE_SEA);

	// update wheels
	for (i=0;i<ML_MAX_GEARNUM;i++)
	{
		TempNum = Vehicle->StateAnimation.WheelPos[i];
		if (Vehicle->VehicleObjectData->WheelRadius[i] > 0.0f)
		{
			TempNum += Speed * Delta / (2 * PI * Vehicle->VehicleObjectData->WheelRadius[i]);
		}
		TempNum = (FLOAT32)(TempNum - floor(TempNum));
		Vehicle->StateAnimation.WheelPos[i] = TempNum;
	}

	// control surfaces
	MultFactor = 1.0f;
	if (Speed < 10.0f)
	{
		MultFactor = Speed / 20.0f;
		MultFactor *= MultFactor;
	}
	if (MultFactor < 0.001)
	{
		MultFactor = 0.001f;
	}

	Vehicle->StateAnimation.LastAileronDeflection = Vehicle->StateAnimation.AileronDeflection;
	Vehicle->StateAnimation.AileronDeflection = (FLOAT32)Vehicle->PosData.AttVel.Roll * MultFactor;
	Vehicle->StateAnimation.AileronDeflection /= PI * 0.5f * 0.9f;
	if (Vehicle->StateAnimation.AileronDeflection > 1.0f)
	{
		Vehicle->StateAnimation.AileronDeflection = 1.0f;
	}
	if (Vehicle->StateAnimation.AileronDeflection < -1.0f)
	{
		Vehicle->StateAnimation.AileronDeflection = -1.0f;
	}
	math3dInvertMatrix(&TempMat,&TempMat);
	TempVec.x = (FLOAT32)Vehicle->PosData.Vel.x;
	TempVec.y = (FLOAT32)Vehicle->PosData.Vel.y;
	TempVec.z = (FLOAT32)Vehicle->PosData.Vel.z;

	if (TempVec.y <= 0.001f)
	{
		TempVec.y = 0.001f;
	}

	math3dVectorMultiply(&TempVec,&TempVec,&TempMat);
	Vehicle->StateAnimation.LastElevatorDeflection = Vehicle->StateAnimation.LastElevatorDeflection;

    if(TempVec.y < 0.001f)
    {
        Vehicle->StateAnimation.ElevatorDeflection = 0.0f;
    }
    else
    {
        Vehicle->StateAnimation.ElevatorDeflection = -TempVec.z / TempVec.y * 2.5f * MultFactor;
    }

    if (Vehicle->StateAnimation.ElevatorDeflection > 1.0f)
	{
		Vehicle->StateAnimation.ElevatorDeflection = 1.0f;
	}
	if (Vehicle->StateAnimation.ElevatorDeflection < -1.0f)
	{
		Vehicle->StateAnimation.ElevatorDeflection = -1.0f;
	}

	Vehicle->StateAnimation.LastRudderDeflection = Vehicle->StateAnimation.RudderDeflection;

    if(TempVec.y < 0.001f)
    {
        Vehicle->StateAnimation.RudderDeflection = 0.0f;
    }
    else
    {
	    Vehicle->StateAnimation.RudderDeflection = -TempVec.x / TempVec.y * 7.5f * MultFactor;
    }

    if (Vehicle->StateAnimation.RudderDeflection > 1.0f)
	{
		Vehicle->StateAnimation.RudderDeflection = 1.0f;
	}
	if (Vehicle->StateAnimation.RudderDeflection < -1.0f)
	{
		Vehicle->StateAnimation.RudderDeflection = -1.0f;
	}

	// just a bit of smoothing

    if(Delta < 0.0001f)
    {
        MultFactor = 0.0f;
    }
    else
    {
    	MultFactor = 1.0f / (Delta / 0.1f);
    }

    Vehicle->StateAnimation.AileronDeflection =
			(Vehicle->StateAnimation.AileronDeflection + MultFactor * Vehicle->StateAnimation.LastAileronDeflection) / (1.0f + MultFactor);
	Vehicle->StateAnimation.ElevatorDeflection =
			(Vehicle->StateAnimation.ElevatorDeflection + MultFactor * Vehicle->StateAnimation.LastElevatorDeflection) / (1.0f + MultFactor);
	Vehicle->StateAnimation.RudderDeflection =
			(Vehicle->StateAnimation.RudderDeflection + MultFactor * Vehicle->StateAnimation.LastRudderDeflection) / (1.0f + MultFactor);

	// update gun recoil animations
	if (Vehicle->ModelData == nullptr)
	{
		_UpdateRecoils(Vehicle);
	}

	return 0;
}

// update the track animations
static UINT32 _UpdateVehicleTracks(vehVehicle* Vehicle,
								   UINT32 ActuateTracks)
{
mlObjectTrackSection* TrackSection;
mlObjectTrackAnimation* TrackAnimation;
piVECTOR const* Vertex1,*Vertex2;
piVECTOR* Vertex3;
FLOAT64VECTOR Point2;
UINT32 VertexNum1,VertexNum2;
UINT32 i,j;
piMATRIX Matrix;
FLOAT32 TrackPos;
OBJECT * Object;

	if (Vehicle == nullptr)
	{
		return 1;
	}

	if (Vehicle->VehicleObjectData == nullptr)
	{
		return 1;
	}

	if (ActuateTracks)
	{
		for (i=0;i<Vehicle->VehicleObjectData->TrackCount;i++)
		{
			TrackSection = &(Vehicle->VehicleObjectData->TrackSection[i]);

			if (TrackSection->NumVertices == 0 ||
				TrackSection->TrackSegment == nullptr ||
				TrackSection->TrackSegmentObjectToLinkTo == nullptr)
			{
				continue;
			}

			math3dInvertMatrix(&Matrix,&(TrackSection->TrackSegment->matint));

			piVectorConstPtr pos1 = TrackSection->TrackSegment->ImmutablePositions();
			piVectorConstPtr pos2 = TrackSection->TrackSegmentObjectToLinkTo->ImmutablePositions();
			piVectorPtr		 pos3 = TrackSection->TrackSegment->MutablePositions();

			for (j=0;j<TrackSection->NumVertices;j++)
			{
				VertexNum1 = TrackSection->LinkToVertex[j][0];
				VertexNum2 = TrackSection->LinkToVertex[j][1];

				Vertex1 = &(pos1[VertexNum1]);
				Vertex2 = &(pos2[VertexNum2]);


				Point2.x = Vertex2->x;
				Point2.y = Vertex2->y;
				Point2.z = Vertex2->z;

				mathVectorMultiply64(&Point2,&Point2,&(TrackSection->TrackSegmentObjectToLinkTo->matint));

				mathVectorMultiply64(&Point2,&Point2,&Matrix);

				Vertex3 = &(pos3[VertexNum1]);

				Vertex3->x = (FLOAT32)Point2.x;
				Vertex3->y = (FLOAT32)Point2.y;
				Vertex3->z = (FLOAT32)Point2.z;
			}

			//ldmdlCalcNormals1(TrackSection->TrackSegment);
		}
	}

	for (i=0;i<Vehicle->VehicleObjectData->TrackAnimationCount;i++)
	{
		TrackAnimation = &(Vehicle->VehicleObjectData->TrackAnimation[i]);

		Object = TrackAnimation->TrackAnimationObject;

		if (Object->flags & OBJECT_SKIPDRAWING ||
			!(Object->flags & OBJECT_VISIBLE))
		{
			continue;
		}

		TrackPos = Vehicle->StateAnimation.TrackPos[TrackAnimation->TrackNum-1];

		TrackPos *= TrackAnimation->TrackMoveRate;
		TrackPos = (FLOAT32)(TrackPos - floor(TrackPos));

        // HACK: fix in version 1.27. Reverses track movement for quick fix (Ticket #97)
        TrackPos = -TrackPos;

		if (Object != nullptr)
		{
			piTexCoordPtr mt = Object->MutableTexCoords();
			piTexCoordConstPtr it = Object->ImmutableTexCoords();

			for (j = 0; j < Object->numverts; j++, ++mt, ++it)
			{
				mt->v = it->v + TrackPos;
			}
		}
	}

	return 0;
}


UINT32 vehAnimResetVehicleStateAnimations(vehVehicle* Vehicle)
{
UINT32 i,j;
vehanimVehicleStateAnimations* StateAnims;

	if (Vehicle == nullptr)
	{
		return 0;
	}

	if (Vehicle->VehicleObjectData == nullptr)
	{
		return 0;
	}

	vehAnimUpdateVehicleStateAnimations(Vehicle);

	StateAnims = &Vehicle->StateAnimation;
	if (StateAnims->GearPos < 0.1)
	{
		for (i=0;i<Vehicle->VehicleObjectData->GearDownCount;i++)
		{
			ogrpHideObjectAndSubtree(Vehicle->VehicleObjectData->GearDownGroup[i]);
		}
		for (i=0;i<Vehicle->VehicleObjectData->GearUpCount;i++)
		{
			ogrpShowObjectAndSubtree(Vehicle->VehicleObjectData->GearUpGroup[i]);
		}
	}
	else
	{
		for (i=0;i<Vehicle->VehicleObjectData->GearDownCount;i++)
		{
			ogrpShowObjectAndSubtree(Vehicle->VehicleObjectData->GearDownGroup[i]);
		}
		for (i=0;i<Vehicle->VehicleObjectData->GearUpCount;i++)
		{
			ogrpHideObjectAndSubtree(Vehicle->VehicleObjectData->GearUpGroup[i]);
		}

	}

	if (StateAnims->FlapPos < 0.2)
	{
		for (i=0;i<Vehicle->VehicleObjectData->FlapsDownCount;i++)
		{
			ogrpHideObjectAndSubtree(Vehicle->VehicleObjectData->FlapsDownGroup[i]);
		}
		for (i=0;i<Vehicle->VehicleObjectData->FlapsUpCount;i++)
		{
			ogrpShowObjectAndSubtree(Vehicle->VehicleObjectData->FlapsUpGroup[i]);
		}

	}
	else
	{
		for (i=0;i<Vehicle->VehicleObjectData->FlapsDownCount;i++)
		{
			ogrpShowObjectAndSubtree(Vehicle->VehicleObjectData->FlapsDownGroup[i]);
		}
		for (i=0;i<Vehicle->VehicleObjectData->FlapsUpCount;i++)
		{
			ogrpHideObjectAndSubtree(Vehicle->VehicleObjectData->FlapsUpGroup[i]);
		}

	}

	for (i=0;i<ML_MAX_ENGINES;i++)
	{
		if (StateAnims->PropIsDiscSetting[i] == TRUE)
		{
			for (j=0;j<Vehicle->VehicleObjectData->EngineFastCount[i];j++)
			{
				ogrpShowObjectAndSubtree(Vehicle->VehicleObjectData->EngineFastGroup[i][j]);
			}
			for (j=0;j<Vehicle->VehicleObjectData->EngineSlowCount[i];j++)
			{
				ogrpHideObjectAndSubtree(Vehicle->VehicleObjectData->EngineSlowGroup[i][j]);
			}

		}
		else
		{
			for (j=0;j<Vehicle->VehicleObjectData->EngineFastCount[i];j++)
			{
				ogrpHideObjectAndSubtree(Vehicle->VehicleObjectData->EngineFastGroup[i][j]);
			}
			for (j=0;j<Vehicle->VehicleObjectData->EngineSlowCount[i];j++)
			{
				ogrpShowObjectAndSubtree(Vehicle->VehicleObjectData->EngineSlowGroup[i][j]);
			}
		}
	}

	return 0;
}

UINT32 vehAnimUpdateVehicleStateAnimations(vehVehicle* Vehicle)
{
UINT32 i,j;
FLOAT32 DeltaTime;
vehanimVehicleStateAnimations* StateAnims;
FLOAT32 LastFlapPos,LastGearPos;
FLOAT32 LastBombBayDoorPos;

	if (Vehicle == nullptr || Vehicle->VehicleType == nullptr ||
		Vehicle->VehicleObjectData  == nullptr || Vehicle->VehicleObjectData->PlaneObject == nullptr)
		return 0;

	StateAnims = &Vehicle->StateAnimation;

	DeltaTime = (frametime - StateAnims->LastUpdateTime) / 1000.0f;

	StateAnims->FlapSetting = 0;
	for (i=0;i<VEH_MAX_STATE_BITS;i++)
	{
		switch(Vehicle->VehicleType->StateType[i].StateType)
		{
			case VehAnim::STATE_TYPE_DOF:
			case VehAnim::STATE_TYPE_EFFECT:
			case VehAnim::STATE_TYPE_SMOKE:
				_SetVehicleDOFState(Vehicle,
									&Vehicle->VehicleType->StateType[i],
									Vehicle->PosData.States[i],
									Vehicle->PosData.LastStates[i]);
			break;
			case VehAnim::STATE_TYPE_SOUND:
				_HandleSound(Vehicle,
								&Vehicle->VehicleType->StateType[i],
								Vehicle->PosData.States[i],
								Vehicle->PosData.LastStates[i]);
			break;

			case VehAnim::STATE_TYPE_VISDAMAGE:
				_HandleVisDamage(Vehicle, &Vehicle->VehicleType->StateType[i],
								Vehicle->PosData.States[i],
								Vehicle->PosData.LastStates[i]);
			break;
		}
	}

	LastGearPos = StateAnims->GearPos;
	if (StateAnims->GearPos < StateAnims->GearSetting)
	{
		if(StateAnims->GearMoveTime != 0)	StateAnims->GearPos += 1000.0f * DeltaTime / StateAnims->GearMoveTime;
		if (StateAnims->GearPos > StateAnims->GearSetting)
		{
			StateAnims->GearPos = (FLOAT32)StateAnims->GearSetting;
		}
	}
	if (StateAnims->GearPos > StateAnims->GearSetting)
	{
		StateAnims->GearPos -= 1000.0f * DeltaTime / StateAnims->GearMoveTime;
		if (StateAnims->GearPos < StateAnims->GearSetting)
		{
			StateAnims->GearPos = (FLOAT32)StateAnims->GearSetting;
		}
	}

	LastBombBayDoorPos = StateAnims->BombBayDoorPos;
	if (StateAnims->BombBayDoorPos < StateAnims->BombBayDoorSetting)
	{
		StateAnims->BombBayDoorPos += 1000.0f * DeltaTime / StateAnims->BombBayDoorMoveTime;
		if (StateAnims->BombBayDoorPos > StateAnims->BombBayDoorSetting)
		{
			StateAnims->BombBayDoorPos = (FLOAT32)StateAnims->BombBayDoorSetting;
		}
	}
	if (StateAnims->BombBayDoorPos > StateAnims->BombBayDoorSetting)
	{
		StateAnims->BombBayDoorPos -= 1000.0f * DeltaTime / StateAnims->BombBayDoorMoveTime;
		if (StateAnims->BombBayDoorPos < StateAnims->BombBayDoorSetting)
		{
			StateAnims->BombBayDoorPos = (FLOAT32)StateAnims->BombBayDoorSetting;
		}
	}
	StateAnims->BombBayDoorDOFVar = StateAnims->BombBayDoorPos;

	if (StateAnims->DiveBrakePos < StateAnims->DiveBrakeSetting)
	{
		StateAnims->DiveBrakePos += 1000.0f * DeltaTime / StateAnims->DiveBrakeMoveTime;
		if (StateAnims->DiveBrakePos > StateAnims->DiveBrakeSetting)
		{
			StateAnims->DiveBrakePos = (FLOAT32)StateAnims->DiveBrakeSetting;
		}
	}
	if (StateAnims->DiveBrakePos > StateAnims->DiveBrakeSetting)
	{
		StateAnims->DiveBrakePos -= 1000.0f * DeltaTime / StateAnims->DiveBrakeMoveTime;
		if (StateAnims->DiveBrakePos < StateAnims->DiveBrakeSetting)
		{
			StateAnims->DiveBrakePos = (FLOAT32)StateAnims->DiveBrakeSetting;
		}
	}
	StateAnims->DiveBrakeDOFVar = StateAnims->DiveBrakePos;

	if (StateAnims->GearPos < 0.1 && LastGearPos >= 0.1)
	{
		for (i=0;i<Vehicle->VehicleObjectData->GearDownCount;i++)
		{
			ogrpHideObjectAndSubtree(Vehicle->VehicleObjectData->GearDownGroup[i]);
		}
		for (i=0;i<Vehicle->VehicleObjectData->GearUpCount;i++)
		{
			ogrpShowObjectAndSubtree(Vehicle->VehicleObjectData->GearUpGroup[i]);
		}

	}
	if (StateAnims->GearPos >= 0.1 && LastGearPos < 0.1)
	{
		for (i=0;i<Vehicle->VehicleObjectData->GearDownCount;i++)
		{
			ogrpShowObjectAndSubtree(Vehicle->VehicleObjectData->GearDownGroup[i]);
		}
		for (i=0;i<Vehicle->VehicleObjectData->GearUpCount;i++)
		{
			ogrpHideObjectAndSubtree(Vehicle->VehicleObjectData->GearUpGroup[i]);
		}

	}
	StateAnims->GearDOFVar = StateAnims->GearPos;

	LastFlapPos = StateAnims->FlapPos;
	if (StateAnims->FlapPos < StateAnims->FlapSetting)
	{
		StateAnims->FlapPos += 1000.0f * DeltaTime / StateAnims->FlapMoveTime;
		if (StateAnims->FlapPos > StateAnims->FlapSetting)
		{
			StateAnims->FlapPos = (FLOAT32)StateAnims->FlapSetting;
		}
	}
	if (StateAnims->FlapPos > StateAnims->FlapSetting)
	{
		StateAnims->FlapPos -= 1000.0f * DeltaTime / StateAnims->FlapMoveTime;
		if (StateAnims->FlapPos < StateAnims->FlapSetting)
		{
			StateAnims->FlapPos = (FLOAT32)StateAnims->FlapSetting;
		}
	}
	if (StateAnims->FlapPos < 0.2 && LastFlapPos >= 0.2)
	{
		for (i=0;i<Vehicle->VehicleObjectData->FlapsDownCount;i++)
		{
			ogrpHideObjectAndSubtree(Vehicle->VehicleObjectData->FlapsDownGroup[i]);
		}
		for (i=0;i<Vehicle->VehicleObjectData->FlapsUpCount;i++)
		{
			ogrpShowObjectAndSubtree(Vehicle->VehicleObjectData->FlapsUpGroup[i]);
		}

	}
	if (StateAnims->FlapPos >= 0.2 && LastFlapPos < 0.2)
	{
		for (i=0;i<Vehicle->VehicleObjectData->FlapsDownCount;i++)
		{
			ogrpShowObjectAndSubtree(Vehicle->VehicleObjectData->FlapsDownGroup[i]);
		}
		for (i=0;i<Vehicle->VehicleObjectData->FlapsUpCount;i++)
		{
			ogrpHideObjectAndSubtree(Vehicle->VehicleObjectData->FlapsUpGroup[i]);
		}

	}
	StateAnims->FlapDOFVar = StateAnims->FlapPos;

	for (i=0;i<ML_MAX_CREW_POSITIONS;i++)
	{
		if (StateAnims->CanopyPos[i] < StateAnims->CanopySetting[i])
		{
			StateAnims->CanopyPos[i] += 1000.0f * DeltaTime / StateAnims->CanopyMoveTime[i];
			if (StateAnims->CanopyPos[i] > StateAnims->CanopySetting[i])
			{
				StateAnims->CanopyPos[i] = (FLOAT32)StateAnims->CanopySetting[i];
			}
		}
		if (StateAnims->CanopyPos[i] > StateAnims->CanopySetting[i])
		{
			StateAnims->CanopyPos[i] -= 1000.0f * DeltaTime / StateAnims->CanopyMoveTime[i];
			if (StateAnims->CanopyPos[i] < StateAnims->CanopySetting[i])
			{
				StateAnims->CanopyPos[i] = (FLOAT32)StateAnims->CanopySetting[i];
			}
		}

		StateAnims->CanopyDOFVar[i] = StateAnims->CanopyPos[i];
		if (StateAnims->HatchPos[i] < StateAnims->HatchSetting[i])
		{
			StateAnims->HatchPos[i] += 1000.0f * DeltaTime / StateAnims->HatchMoveTime[i];
			if (StateAnims->HatchPos[i] > StateAnims->HatchSetting[i])
			{
				StateAnims->HatchPos[i] = (FLOAT32)StateAnims->HatchSetting[i];
			}
		}
		if (StateAnims->HatchPos[i] > StateAnims->HatchSetting[i])
		{
			StateAnims->HatchPos[i] -= 1000.0f * DeltaTime / StateAnims->HatchMoveTime[i];
			if (StateAnims->HatchPos[i] < StateAnims->HatchSetting[i])
			{
				StateAnims->HatchPos[i] = (FLOAT32)StateAnims->HatchSetting[i];
			}
		}
		StateAnims->HatchDOFVar[i] = StateAnims->HatchPos[i];
	}

	for (i=0;i<ML_MAX_ENGINES;i++)
	{
		if (StateAnims->PropIsDiscSetting[i] && !StateAnims->PropIsDisc[i])
		{
			for (j=0;j<Vehicle->VehicleObjectData->EngineFastCount[i];j++)
			{
				ogrpShowObjectAndSubtree(Vehicle->VehicleObjectData->EngineFastGroup[i][j]);
			}
			for (j=0;j<Vehicle->VehicleObjectData->EngineSlowCount[i];j++)
			{
				ogrpHideObjectAndSubtree(Vehicle->VehicleObjectData->EngineSlowGroup[i][j]);
			}

		}
		if (!StateAnims->PropIsDiscSetting[i] && StateAnims->PropIsDisc[i])
		{
			for (j=0;j<Vehicle->VehicleObjectData->EngineFastCount[i];j++)
			{
				ogrpHideObjectAndSubtree(Vehicle->VehicleObjectData->EngineFastGroup[i][j]);
			}
			for (j=0;j<Vehicle->VehicleObjectData->EngineSlowCount[i];j++)
			{
				ogrpShowObjectAndSubtree(Vehicle->VehicleObjectData->EngineSlowGroup[i][j]);
			}

		}
		StateAnims->PropIsDisc[i] = StateAnims->PropIsDiscSetting[i];


		// update prop position
		if (StateAnims->PropIsDisc[i] == TRUE)
		{
			StateAnims->PropSpinPosDOF[i] += StateAnims->PropDiscSpinRpm[i] / 60.0f * DeltaTime;
			if (StateAnims->PropSpinPosDOF[i] > 1.0f)
			{
				StateAnims->PropSpinPosDOF[i] -= 1.0f;
			}
		}
	}

	// update smokes
	_UpdateSmokes(Vehicle);
//	_UpdateTurrets(Vehicle);

	if (Vehicle->VehicleObjectData != nullptr)
	{
		vehAnimSetDOFObjectVals(Vehicle->VehicleObjectData->PlaneObject, StateAnims, Vehicle->VehicleType,Vehicle);
	}

	// if fps set turret elevation
	if (Vehicle->VehicleType->Type == VEH_TYPE_PERSON &&
		Vehicle->Flags & VEH_FLAGS_UPDATESYSTEM_CONTROLLED)
	{
	FLOAT32 TempNum;
	FLOAT32 TempNum1,TempNum2;
	FLOAT32 Delta;
	FLOAT32 Movement;
	UINT32 TurretNum;
	vehType * VehicleType;

		VehicleType = Vehicle->VehicleType;

		TurretNum = 0;
		if (frametime != StateAnims->LastElevUpdateTime[TurretNum])
		{
			TempNum1 = StateAnims->TurretElev[TurretNum];
			TempNum2 = StateAnims->LastTurretElev[TurretNum];

			TempNum = TempNum2;
			if (VehicleType->TurretElevateRate[TurretNum] > 0.0)
			{
				Movement = (frametime - StateAnims->LastUpdateTime) / 1000.0f *
								(0.9f / VehicleType->TurretElevateRate[TurretNum]);
			}
			else
			{
				Movement = 0.0f;
			}
			Delta = TempNum1 - TempNum2;
			if (Delta < 0.0f)
			{
				if (-Delta < Movement)
				{
					Movement = -Delta;
				}
				TempNum = TempNum2 - Movement;
				if (TempNum < TempNum1)
				{
				}
			}
			if (Delta > 0.0f)
			{
				if (Delta < Movement)
				{
					Movement = Delta;
				}
				TempNum = TempNum2 + Movement;
				if (TempNum > TempNum1)
				{
				}
			}

			StateAnims->LastTurretElev[TurretNum] = TempNum;
			StateAnims->LastElevUpdateTime[TurretNum] = frametime;
		}
	}

	for (i=0;i<VEH_MAX_STATE_BITS;i++)
		Vehicle->PosData.LastStates[i] = Vehicle->PosData.States[i];

	StateAnims->FirstUpdateProcessed = TRUE;

	return 0;
}

static UINT32 _SetDOF(OBJECT * Object,
					  vehanimVehicleStateAnimations* StateAnims,
					  vehType * VehicleType,
					  vehVehicle* Vehicle)
{
FLOAT32 TempNum;
FLOAT32 TempNum1,TempNum2;
FLOAT32 Delta;
FLOAT32 Movement;
FLOAT32 Multiplier;
UINT32 TurretNum;
SINT32 DofVarnum;
SINT32 DofVarsubnum;

	DofVarnum = Object->dof->dofvarnum;
	DofVarsubnum = Object->dof->dofvarsubnum;

	if (DofVarnum == ML_DOF_NONE)
	{
		return 0;
	}


	switch (DofVarnum)
	{
		case ML_DOF_ELEMENT_GEAR:
			ogrpSetDOFVariable(Object,
								StateAnims->GearDOFVar,
								0,
								0);
			break;

		case ML_DOF_ELEMENT_FLAP:
			ogrpSetDOFVariable(Object,
								StateAnims->FlapDOFVar,
								0,
								0);
			break;

		case ML_DOF_ELEMENT_CANOPY:
			ogrpSetDOFVariable(Object,
								StateAnims->CanopyDOFVar[Object->dof->dofvarsubnum],
								0,
								0);
			break;

		case ML_DOF_ELEMENT_HATCH:
			ogrpSetDOFVariable(Object,
								StateAnims->HatchDOFVar[Object->dof->dofvarsubnum],
								0,
								0);
			break;

		case ML_DOF_ELEMENT_BOMBBAYDOOR:
			ogrpSetDOFVariable(Object,
								StateAnims->BombBayDoorDOFVar,
								0,
								0);
			break;

		case ML_DOF_CONTROL_DIVEBRAKE:
			ogrpSetDOFVariable(Object,
								StateAnims->DiveBrakeDOFVar,
								0,
								0);
			break;

		case ML_DOF_WHEEL_ROTATE:
			if (StateAnims->UseCalculatedTrackAndWheel == 1)
			{
				ogrpSetDOFVariable(Object,
									StateAnims->WheelPos[Object->dof->dofvarsubnum],
									0,
									1);
			}
			break;

		case ML_DOF_ELEMENT_LAILERON:
			if (StateAnims->UseCalculatedTrackAndWheel == 1)
			{
				ogrpSetDOFVariable(Object,
									StateAnims->AileronDeflection,
									0,
									0);
			}
			break;

		case ML_DOF_ELEMENT_RAILERON:
			if (StateAnims->UseCalculatedTrackAndWheel == 1)
			{
				ogrpSetDOFVariable(Object,
									StateAnims->AileronDeflection,
									0,
									0);
			}
			break;

		case ML_DOF_ELEMENT_ELEVATOR:
			if (StateAnims->UseCalculatedTrackAndWheel == 1)
			{
				ogrpSetDOFVariable(Object,
									StateAnims->ElevatorDeflection,
									0,
									0);
			}
			break;

		case ML_DOF_ELEMENT_RUDDER:
			if (StateAnims->UseCalculatedTrackAndWheel == 1)
			{
				ogrpSetDOFVariable(Object,
									StateAnims->RudderDeflection,
									0,
									0);
			}
			break;

		case ML_DOF_RECOIL:
			ogrpSetDOFVariable(Object,
								StateAnims->RecoilPosition[Object->dof->dofvarsubnum],
								0,
								0);

			break;
	}

	if (Vehicle->ModelData == nullptr ||
		Vehicle->ModelData->MultiCrewPos != 0)
	{
		// hack for turret control
		switch (DofVarnum)
		{
			case ML_DOF_ELEMENT_TELEVATE:
			case ML_DOF_ELEMENT_FLEXELEVATE:
			{
				TurretNum = Object->dof->dofvarsubnum;
				if (VehicleType->TurretType[TurretNum] == VEH_TURRET_TYPE_360 ||
					VehicleType->TurretType[TurretNum] == VEH_TURRET_TYPE_LIMITED)
				{
					if (frametime != StateAnims->LastElevUpdateTime[TurretNum])
					{
						TempNum1 = StateAnims->TurretElev[TurretNum];
						TempNum2 = StateAnims->LastTurretElev[TurretNum];

						Multiplier = (VehicleType->TurretElevateRate[TurretNum] - 0.75f) /
										VehicleType->TurretElevateRate[TurretNum];
						if (Multiplier < 0.75f)
						{
							Multiplier = 0.75f;
						}

						TempNum = TempNum2;
						if (VehicleType->TurretElevateRate[TurretNum] > 0.0)
						{
							Movement = (frametime - StateAnims->LastUpdateTime) / 1000.0f *
											(Multiplier / VehicleType->TurretElevateRate[TurretNum]);
						}
						else
						{
							Movement = 0.0f;
						}
						Delta = TempNum1 - TempNum2;
						if (Delta < 0.0f)
						{
							if (-Delta < Movement)
							{
								Movement = -Delta;
							}
							TempNum = TempNum2 - Movement;
							if (TempNum < TempNum1)
							{
				//				TempNum = TempNum1;
							}
						}
						if (Delta > 0.0f)
						{
							if (Delta < Movement)
							{
								Movement = Delta;
							}
							TempNum = TempNum2 + Movement;
							if (TempNum > TempNum1)
							{
				//				TempNum = TempNum1;
							}
						}

						if (TempNum > 1.0)
						{
							TempNum = 1.0;
						}
						if (TempNum < -1.0)
						{
							TempNum = -1.0;
						}

						if (Vehicle->ModelData == nullptr)
						{
							ogrpSetDOFVariable(Object,
												TempNum,
												0,
												1);
						}
						else
						{
							if (Vehicle->VehicleType->MultiCrewPos[TurretNum] !=
								Vehicle->ModelData->MultiCrewPos)
							{
								ogrpSetDOFVariable(Object,
													TempNum,
													0,
													1);
							}
						}
						StateAnims->LastTurretElev[TurretNum] = TempNum;
						StateAnims->LastElevUpdateTime[TurretNum] = frametime;
					}
					else
					{
						if (Vehicle->ModelData == nullptr)
						{
							ogrpSetDOFVariable(Object,
												StateAnims->LastTurretElev[TurretNum],
												0,
												1);
						}
						else
						{
							if (Vehicle->VehicleType->MultiCrewPos[TurretNum] !=
								Vehicle->ModelData->MultiCrewPos)
							{
								ogrpSetDOFVariable(Object,
													StateAnims->LastTurretElev[TurretNum],
													0,
													1);
							}
						}
					}
				}
			}
			break;

			case ML_DOF_ELEMENT_TTRAVERSE:
			{
				TurretNum = Object->dof->dofvarsubnum;
				if (VehicleType->TurretType[TurretNum] == VEH_TURRET_TYPE_360)
				{
					if (frametime != StateAnims->LastTravUpdateTime[TurretNum])
					{
						TempNum1 = StateAnims->TurretTraverse[TurretNum];
						TempNum2 = StateAnims->LastTurretTraverse[TurretNum];
						Multiplier = (VehicleType->TurretTraverseRate[TurretNum] - 0.75f) /
										VehicleType->TurretTraverseRate[TurretNum];
						if (Multiplier < 0.75f)
						{
							Multiplier = 0.75f;
						}

						TempNum = TempNum2;
						if (VehicleType->TurretTraverseRate[TurretNum] > 0.0)
						{
							Movement = (frametime - StateAnims->LastUpdateTime) / 1000.0f *
											(Multiplier / VehicleType->TurretTraverseRate[TurretNum]);
						}
						else
						{
							Movement = 0.0f;
						}

						if (TempNum1 < 0.0)
						{
							TempNum1 += 1.0;
						}
						if (TempNum2 < 0.0)
						{
							TempNum2 += 1.0;
						}

						Delta = TempNum1 - TempNum2;
						if (fabs(Delta) > 0.5)
						{
							if (Delta < 0.0)
							{
								Delta = (1.0f - -Delta);
							}
							else
							{
								Delta = -(1.0f - Delta);
							}
						}
						if (Delta < 0.0f)
						{
							if (-Delta < Movement)
							{
								Movement = -Delta;
							}
							TempNum = TempNum2 - Movement;
							if (TempNum < TempNum1)
							{
				//				TempNum = TempNum1;
							}
						}
						if (Delta > 0.0f)
						{
							if (Delta < Movement)
							{
								Movement = Delta;
							}
							TempNum = TempNum2 + Movement;
							if (TempNum > TempNum1)
							{
				//				TempNum = TempNum1;
							}
						}

						if (TempNum > 1.0)
						{
							TempNum -= 1.0;
							if (TempNum > 1.0)
							{
								TempNum = 1.0;
							}
						}
						if (TempNum < 0.0)
						{
							TempNum += 1.0;
							if (TempNum < 0.0)
							{
								TempNum = 0.0;
							}
						}

						if (Vehicle->ModelData == nullptr)
						{
							ogrpSetDOFVariable(Object,
												TempNum,
												1,
												1);
						}
						else
						{
							if (Vehicle->VehicleType->MultiCrewPos[TurretNum] !=
								Vehicle->ModelData->MultiCrewPos)
							{
								ogrpSetDOFVariable(Object,
													TempNum,
													1,
													1);
							}
						}
						StateAnims->LastTurretTraverse[TurretNum] = TempNum;
						StateAnims->LastTravUpdateTime[TurretNum] = frametime;
					}
					else
					{
						if (Vehicle->ModelData == nullptr)
						{
							ogrpSetDOFVariable(Object,
												StateAnims->LastTurretTraverse[TurretNum],
												1,
												1);
						}
						else
						{
							if (Vehicle->VehicleType->MultiCrewPos[TurretNum] !=
								Vehicle->ModelData->MultiCrewPos)
							{
								ogrpSetDOFVariable(Object,
													StateAnims->LastTurretTraverse[TurretNum],
													1,
													1);
							}
						}
					}
				}
			}
			break;

			case ML_DOF_ELEMENT_TRAVERSELIMITED:
			case ML_DOF_ELEMENT_FLEXTRAVERSE:
			{
				TurretNum = Object->dof->dofvarsubnum;
				if (VehicleType->TurretType[TurretNum] == VEH_TURRET_TYPE_LIMITED)
				{
					if (frametime != StateAnims->LastTravUpdateTime[TurretNum])
					{
						TempNum1 = StateAnims->TurretTraverse[TurretNum];
						TempNum2 = StateAnims->LastTurretTraverse[TurretNum];

						Multiplier = (VehicleType->TurretTraverseRate[TurretNum] - 0.75f) /
										VehicleType->TurretTraverseRate[TurretNum];
						if (Multiplier < 0.75f)
						{
							Multiplier = 0.75f;
						}

						TempNum = TempNum2;
						if (VehicleType->TurretTraverseRate[TurretNum] > 0.0)
						{
							Movement = (frametime - StateAnims->LastUpdateTime) / 1000.0f *
											(Multiplier / VehicleType->TurretTraverseRate[TurretNum]);
						}
						else
						{
							Movement = 0.0f;
						}
						Delta = TempNum1 - TempNum2;
						if (Delta < 0.0f)
						{
							if (-Delta < Movement)
							{
								Movement = -Delta;
							}
							TempNum = TempNum2 - Movement;
							if (TempNum < TempNum1)
							{
				//				TempNum = TempNum1;
							}
						}
						if (Delta > 0.0f)
						{
							if (Delta < Movement)
							{
								Movement = Delta;
							}
							TempNum = TempNum2 + Movement;
							if (TempNum > TempNum1)
							{
				//				TempNum = TempNum1;
							}
						}

						if (TempNum > 1.0)
						{
							TempNum = 1.0;
						}
						if (TempNum < -1.0)
						{
							TempNum = -1.0;
						}

						if (Vehicle->ModelData == nullptr)
						{
							ogrpSetDOFVariable(Object,
												TempNum,
												0,
												1);
						}
						else
						{
							if (Vehicle->VehicleType->MultiCrewPos[TurretNum] !=
								Vehicle->ModelData->MultiCrewPos)
							{
								ogrpSetDOFVariable(Object,
													TempNum,
													0,
													1);
							}
						}
						StateAnims->LastTurretTraverse[TurretNum] = TempNum;
						StateAnims->LastTravUpdateTime[TurretNum] = frametime;
					}
					else
					{
						if (Vehicle->ModelData == nullptr)
						{
							ogrpSetDOFVariable(Object,
												StateAnims->LastTurretTraverse[TurretNum],
												0,
												1);
						}
						else
						{
							if (Vehicle->VehicleType->MultiCrewPos[TurretNum] !=
								Vehicle->ModelData->MultiCrewPos)
							{
								ogrpSetDOFVariable(Object,
													StateAnims->LastTurretTraverse[TurretNum],
													0,
													1);
							}
						}
					}
				}
			}
			break;

			case ML_DOF_ELEMENT_PROPSPIN:
			case ML_DOF_ELEMENT_PROPEFFECT:
			{
				ogrpSetDOFVariable(Object,
									StateAnims->PropSpinPosDOF[DofVarsubnum],
									0,
									0);
			}
			break;
		}
	}
	return 0;
}

UINT32 vehAnimSetDOFObjectVals(OBJECT * Object,
								vehanimVehicleStateAnimations* StateAnims,
								vehType * VehicleType,
								vehVehicle* Vehicle)
{
	if (Object->flags & OBJECT_DOF)
	{
		_SetDOF(Object,StateAnims,VehicleType,Vehicle);
	}

	if (Object->child != nullptr)
	{
		_SetDOFSubObjectVals(Object->child,StateAnims,VehicleType,Vehicle);
	}

	return 0;
}

static UINT32 _SetDOFSubObjectVals(OBJECT * Object,
								   vehanimVehicleStateAnimations* StateAnims,
								   vehType * VehicleType,
								   vehVehicle* Vehicle)
{
OBJECT * ObjSib;

	ObjSib = Object;
	do
	{
		if (ObjSib->flags & OBJECT_DOF)
		{
			_SetDOF(ObjSib,StateAnims,VehicleType,Vehicle);
		}

		if (ObjSib->child != nullptr)
		{
			// don't recurse into hidden LODs that are not special LODs and aren't user's vehicle

			if (!(ObjSib->flags & OBJECT_SKIPDRAWING &&
				  ObjSib->flags & OBJECT_LOD &&
				 !(ObjSib->flag2 & OBJECT_USERVEHICLE) &&
				 !(ObjSib->flag2 & OBJECT_SPECIAL_LOD)) &&
				 !(ObjSib->flag2 & OBJECT_DONT_UPDATE &&
				   !(ObjSib->flag2 & OBJECT_USERVEHICLE)))

			{
				_SetDOFSubObjectVals(ObjSib->child,StateAnims,VehicleType,Vehicle);
			}
		}

		ObjSib = ObjSib->sibling;
	}
	while (ObjSib != nullptr);

	return 0;
}

static UINT32 _SetVehicleDOFState(vehVehicle* Vehicle,
								  vehanimVehicleStateType * StateType,
								  LOGICAL State, LOGICAL LastState)
{
	if (StateType == nullptr)
	{
		return 0;
	}

	if (StateType->StateType == VehAnim::STATE_TYPE_SMOKE &&
		StateType->Data == 0)
	{
		return 0;
	}

	switch(StateType->StateSubType)
	{
		case VehAnim::STATE_SUBTYPE_GEAR:
			if (State == TRUE)
			{
				Vehicle->StateAnimation.GearSetting = 1;
			}
			else
			{
				Vehicle->StateAnimation.GearSetting = 0;
			}
			Vehicle->StateAnimation.GearMoveTime = StateType->Data;
			break;
		case VehAnim::STATE_SUBTYPE_FLAPS:
			if (State == TRUE)
			{
				Vehicle->StateAnimation.FlapSetting = (UINT32)(StateType->Data + 0.5);
			}
			Vehicle->StateAnimation.FlapMoveTime = StateType->Data1;
			break;
		case VehAnim::STATE_SUBTYPE_PROP:
			Vehicle->StateAnimation.PropIsDiscSetting[StateType->Data-1] = State;
			Vehicle->StateAnimation.PropDiscSpinRpm[StateType->Data-1] = (FLOAT32)StateType->Data1;

			break;
		case VehAnim::STATE_SUBTYPE_CANOPY:
			if (State == TRUE)
			{
				Vehicle->StateAnimation.CanopySetting[StateType->Data-1] = 1;
			}
			else
			{
				Vehicle->StateAnimation.CanopySetting[StateType->Data-1] = 0;
			}
			if(EZ_MGR)
				Vehicle->StateAnimation.CanopyMoveTime[StateType->Data-1] = EZ_MGR->GetHatchTime();
			else
				Vehicle->StateAnimation.CanopyMoveTime[StateType->Data-1] = StateType->Data1;
			break;
		case VehAnim::STATE_SUBTYPE_HATCH:
			if (State == TRUE)
			{
				Vehicle->StateAnimation.HatchSetting[StateType->Data-1] = 1;
			}
			else
			{
				Vehicle->StateAnimation.HatchSetting[StateType->Data-1] = 0;
			}
			Vehicle->StateAnimation.HatchMoveTime[StateType->Data-1] = StateType->Data1;
			break;
		case VehAnim::STATE_SUBTYPE_BOMBBAYDOOR:
			Vehicle->StateAnimation.BombBayDoorSetting = (State) ? 1 : 0;
			Vehicle->StateAnimation.BombBayDoorMoveTime = StateType->Data;
			if(Vehicle->ModelData)
				Vehicle->ModelData->BombBayDoor = (State) ? 1 : 0;
			break;
		case VehAnim::STATE_SUBTYPE_DIVEBRAKE:
			if (State == TRUE)
			{
				Vehicle->StateAnimation.DiveBrakeSetting = 1;
			}
			else
			{
				Vehicle->StateAnimation.DiveBrakeSetting = 0;
			}
			Vehicle->StateAnimation.DiveBrakeMoveTime = StateType->Data;
			break;
		case VehAnim::STATE_SUBTYPE_EXHAUST:
            {
                _DoExhaustState(Vehicle, StateType, State, LastState);
                break;
            }
        case VehAnim::STATE_SUBTYPE_TREADDUST:
            {
                _DoTreadDustState(Vehicle, StateType, State, LastState);
                break;
            }
		case VehAnim::STATE_SUBTYPE_WINGTIPVORTEX:
            {
                _DoWingVortexState(Vehicle, StateType, State, LastState);
                break;
            }
			break;
		case VehAnim::STATE_SUBTYPE_FUELFIRE:
			if(State && Vehicle->ModelData) Vehicle->ModelData->HulkOnDespawn = TRUE;
			Vehicle->StateAnimation.FuelFireOn[StateType->Data-1] = State;
			if (Vehicle->StateAnimation.FuelFireEmitter[StateType->Data-1] == nullptr)
			{
				ogrpFindSubObjectByName(&Vehicle->StateAnimation.FuelFireEmitter[StateType->Data-1],
										Vehicle->VehicleObjectData->EffectsObject,
										(char*)StateType->Emitter);
				if (Vehicle->StateAnimation.FuelFireEmitter[StateType->Data-1] == nullptr)
				{
//					Vehicle->StateAnimation.FuelFireEmitter[StateType->Data-1] = (OBJECT*)-1;
					char s[256];
					strprintf(s,"State %s, fuel fire emitter smoke, unable to find dof %s, Vehicle type = %s",
							StateType->Name,
							(char*)StateType->Emitter,
							Vehicle->VehicleType->Description);
					WriteLog(LOG_VEHICLE,LOG_WARN,s);

					piExitProgram(_FL,s);

				}
			}
			break;
		case VehAnim::STATE_SUBTYPE_FUELLEAK:
			Vehicle->StateAnimation.FuelLeakSmokeOn[StateType->Data-1] = State;
			if (Vehicle->StateAnimation.FuelLeakSmokeEmitter[StateType->Data-1] == nullptr)
			{
				ogrpFindSubObjectByName(&Vehicle->StateAnimation.FuelLeakSmokeEmitter[StateType->Data-1],
										Vehicle->VehicleObjectData->EffectsObject,
										(char*)StateType->Emitter);
				if (Vehicle->StateAnimation.FuelLeakSmokeEmitter[StateType->Data-1] == nullptr)
				{
//					Vehicle->StateAnimation.FuelLeakSmokeEmitter[StateType->Data-1] = (OBJECT*)-1;
					char s[256];
					strprintf(s,"State %s, fuel leak smoke, unable to find dof %s, Vehicle type = %s",
							StateType->Name,
							(char*)StateType->Emitter,
							Vehicle->VehicleType->Description);
					WriteLog(LOG_VEHICLE,LOG_WARN,s);

					piExitProgram(_FL,s);

				}
			}
			break;
		case VehAnim::STATE_SUBTYPE_OILLEAK:
			Vehicle->StateAnimation.OilLeakSmokeOn[StateType->Data-1] = State;
			if (Vehicle->StateAnimation.OilLeakSmokeEmitter[StateType->Data-1] == nullptr)
			{
				ogrpFindSubObjectByName(&Vehicle->StateAnimation.OilLeakSmokeEmitter[StateType->Data-1],
										Vehicle->VehicleObjectData->EffectsObject,
										(char*)StateType->Emitter);
				if (Vehicle->StateAnimation.OilLeakSmokeEmitter[StateType->Data-1] == nullptr)
				{
//					Vehicle->StateAnimation.OilLeakSmokeEmitter[StateType->Data-1] = (OBJECT*)-1;
					char s[256];
					strprintf(s,"State %s, oil leak smoke, unable to find dof %s, Vehicle type = %s",
							StateType->Name,
							(char*)StateType->Emitter,
							Vehicle->VehicleType->Description);
					WriteLog(LOG_VEHICLE,LOG_WARN,s);

					piExitProgram(_FL,s);



				}
			}
			break;
		case VehAnim::STATE_SUBTYPE_GLYCOLLEAK:
			if (StateType->Data1 > 0)
			{
				Vehicle->StateAnimation.GlycolLeakSmokeOn[StateType->Data-1][StateType->Data1-1] = State;
				if (Vehicle->StateAnimation.GlycolLeakSmokeEmitter[StateType->Data-1][StateType->Data1-1] == nullptr)
				{
					ogrpFindSubObjectByName(&Vehicle->StateAnimation.GlycolLeakSmokeEmitter[StateType->Data-1][StateType->Data1-1],
											Vehicle->VehicleObjectData->EffectsObject,
											(char*)StateType->Emitter);
					if (Vehicle->StateAnimation.GlycolLeakSmokeEmitter[StateType->Data-1][StateType->Data1-1] == nullptr)
					{
//						Vehicle->StateAnimation.GlycolLeakSmokeEmitter[StateType->Data-1][StateType->Data1-1] = (OBJECT*)-1;
						char s[256];
						strprintf(s,"State %s, glycol leak smoke, unable to find dof %s, Vehicle type = %s",
								StateType->Name,
								(char*)StateType->Emitter,
								Vehicle->VehicleType->Description);
						WriteLog(LOG_VEHICLE,LOG_WARN,s);

						piExitProgram(_FL,s);

					}
				}
			}
			break;
		case VehAnim::STATE_SUBTYPE_ENGINEFIRE:
			if(State && Vehicle->IsMe() && Vehicle->VehicleType->Type != VEH_TYPE_AIR)
				Vehicle->ModelData->HulkOnDespawn = TRUE;
			Vehicle->StateAnimation.EngineFireOn[StateType->Data-1] = State;
			if (Vehicle->StateAnimation.EngineFireEmitter[StateType->Data-1] == nullptr)
			{
				ogrpFindSubObjectByName(&Vehicle->StateAnimation.EngineFireEmitter[StateType->Data-1],
										Vehicle->VehicleObjectData->EffectsObject,
										(char*)StateType->Emitter);
				if (Vehicle->StateAnimation.EngineFireEmitter[StateType->Data-1] == nullptr)
				{
//					Vehicle->StateAnimation.EngineFireEmitter[StateType->Data-1] = (OBJECT*)-1;
					char s[256];
					strprintf(s,"State %s, engine fire smoke, unable to find dof %s, Vehicle type = %s",
							StateType->Name,
							(char*)StateType->Emitter,
							Vehicle->VehicleType->Description);
					WriteLog(LOG_VEHICLE,LOG_WARN,s);

					piExitProgram(_FL,s);

				}
			}
			break;
		case VehAnim::STATE_SUBTYPE_TANKFIRE:
			if (Vehicle->StateAnimation.TankOnFire == 0 && State == TRUE)
			{
				if(Vehicle->ModelData) Vehicle->ModelData->HulkOnDespawn = TRUE;
				Vehicle->StateAnimation.TankOnFire = 1;
				Vehicle->StateAnimation.TankOnFireStartTime = frametime;
				Vehicle->StateAnimation.TankBurnFlareupTime = StateType->Data;

#if(0)
				// find the tank fire emitter points
				for (i=0;i<VEH_MAX_FIREPOINTS;i++)
				{
					char s[256];
					strprintf(s,"tfire%d",i+1);
					ogrpFindSubObjectByName(&Vehicle->StateAnimation.TankFireEmitterPoint[i],
											Vehicle->VehicleObjectData->EffectsObject,
											s);;
				}
#endif
			}
			break;
		case VehAnim::STATE_SUBTYPE_CRASHEXPLOSION:
			if (State == TRUE)
			{
				if(Vehicle->ModelData) Vehicle->ModelData->HulkOnDespawn = TRUE;
				Vehicle->StateAnimation.PlayCrash = 1;
			}
			break;
		case VehAnim::STATE_SUBTYPE_CRASHEJECTRIDERS:
			if (State == TRUE)
			{
				Vehicle->StateAnimation.EjectRiders = 1;
			}
			else
			{
				Vehicle->StateAnimation.EjectRiders = 0;
			}
			break;
		default:
			break;
	}


	if (StateType->StateType == VehAnim::STATE_TYPE_SMOKE)
	{

	}

	return 0;
}

////////////////////////////////////////////////////////
// Function: _DoExhaustState
// Description:
////////////////////////////////////////////////////////
UINT32 _DoExhaustState(vehVehicle*Vehicle,
                       vehanimVehicleStateType* pStateType,
                       LOGICAL bState, LOGICAL bLastState)
{
    vehanimVehicleStateAnimations* pAnim = nullptr;
    SCHAR szDOFName[64];
    SINT32 q = pStateType->Data - 1;

    if(q < 0) return 1;

    pAnim = &Vehicle->StateAnimation;

    OBJECT* lpLeft = pAnim->ExhaustSmokeEmitter[q][0];
    OBJECT* lpRight = pAnim->ExhaustSmokeEmitter[q][1];

    if(lpLeft == nullptr)
    {
        strprintf(szDOFName, "%s", (SCHAR*)pStateType->Emitter);
	    ogrpFindSubObjectByName(&lpLeft, Vehicle->VehicleObjectData->EffectsObject, szDOFName);

        if(lpLeft == nullptr)
        {
	        SCHAR szBuff[128];
	        const SCHAR* pStr = "State %s, exhaust smoke, unable to find dof %s, Vehicle type = %s";

	        strprintf(szBuff,
	                pStr,
	                pStateType->Name,
	                (SCHAR*)pStateType->Emitter,
	                Vehicle->VehicleType->Description);

	        WriteLog(LOG_VEHICLE,LOG_WARN, szBuff);
	    }

	    pAnim->ExhaustSmokeEmitter[q][0] = lpLeft;
        return 0;
	}

    if(lpRight == nullptr)
    {
        strprintf(szDOFName, "%s", (SCHAR*)pStateType->Emitter);
	    ogrpFindSubObjectByName(&lpRight, Vehicle->VehicleObjectData->EffectsObject, szDOFName);

        if(lpRight == nullptr)
        {
	        SCHAR szBuff[128];
	        const SCHAR* pStr = "State %s, exhaust smoke, unable to find dof %s, Vehicle type = %s";

	        strprintf(szBuff,
	                pStr,
	                pStateType->Name,
	                (SCHAR*)pStateType->Emitter,
	                Vehicle->VehicleType->Description);

	        WriteLog(LOG_VEHICLE,LOG_WARN, szBuff);
	    }

	    pAnim->ExhaustSmokeEmitter[q][1] = lpRight;
        return 0;
	}

	// tie 3rd person exhaust to engine sound
	if(pAnim->ExhaustSmokeStartTime != 0)
	{
		if(frametime - pAnim->ExhaustSmokeStartTime > 2000) // on for 2 secs
		{
			pAnim->ExhaustSmokeOn[q] = 0;
			vehfxCheckExhaustState(Vehicle->iFXVehicleID);

            LOGICAL bSmokeOn = FALSE;

            for(SINT32 j = 0; j < 8; j++)
            {
                bSmokeOn |= pAnim->ExhaustSmokeOn[j];
            }

            // Only set start time to 0 if all exhaust smoke states are off.
            // Otherwise, we only shut one off when we set start time to 0.
            if(!bSmokeOn)
            {
                pAnim->ExhaustSmokeStartTime = 0;
            }
		}else
		{
			if(!pAnim->ExhaustSmokeOn[q])
			{
				if(pAnim->ExhaustSmokeEmitter[q] != nullptr)
					pAnim->ExhaustSmokeOn[q] = 1;

				vehfxCheckExhaustState(Vehicle->iFXVehicleID);
			}
		}
	}
	else // normal first person operation
	{
		if(bState != bLastState)
		{
			if(pAnim->ExhaustSmokeEmitter[q] != nullptr)
				pAnim->ExhaustSmokeOn[q] = bState;

			vehfxCheckExhaustState(Vehicle->iFXVehicleID);
		}
	}

    return 0;
}

////////////////////////////////////////////////////////
// Function: _DoWingVortexState
// Description:
////////////////////////////////////////////////////////
UINT32 _DoWingVortexState(vehVehicle*pVeh,
                          vehanimVehicleStateType* pStateType,
                          LOGICAL bState,
                          LOGICAL bLastState)
{
    SINT32 q = pStateType->Data - 1;

    if(q < 0 || q >= 2)
        return 1;

    pVeh->StateAnimation.WingtipVortexSmokeOn[q] = bState;

    if(pVeh->StateAnimation.WingtipVortexEmitter[q] == nullptr)
    {
        ogrpFindSubObjectByName(&pVeh->StateAnimation.WingtipVortexEmitter[q],
                                pVeh->VehicleObjectData->EffectsObject,
                                (SCHAR*)pStateType->Emitter);

        if(pVeh->StateAnimation.WingtipVortexEmitter[q] == nullptr)
        {
            SCHAR s[256];
            strprintf(s, "State %s, wingtip vortex smoke, unable to find dof %s, Vehicle type = %s",
                    pStateType->Name,
                    (SCHAR*)pStateType->Emitter,
                    pVeh->VehicleType->Description);

			WriteLog(LOG_VEHICLE,LOG_WARN, s);

			piExitProgram(_FL, s);
        }
    }

    if(bState != bLastState)
    {
        vehfxCheckWingVortexState(pVeh->iFXVehicleID);
    }

    return 0;
}

////////////////////////////////////////////////////////
// Function: _DoExhaustState
// Description:
////////////////////////////////////////////////////////
UINT32 _DoTreadDustState(vehVehicle*Vehicle,
                         vehanimVehicleStateType* pStateType,
                         LOGICAL bState,
                         LOGICAL bLastState)
{
    vehanimVehicleStateAnimations* pAnim = nullptr;
    static SCHAR szDOFName[64];
    SINT32 q = pStateType->Data - 1;

    if(q < 0) return 1;
    if(q >= VEH_MAX_TREAD_DUST) return 1;

    pAnim = &Vehicle->StateAnimation;

    OBJECT* lpEmit = pAnim->TreadDustEmitter[q];

    if(lpEmit == nullptr)
    {
        strprintf(szDOFName, "%s", (SCHAR*)pStateType->Emitter);
	    ogrpFindSubObjectByName(&lpEmit, Vehicle->VehicleObjectData->EffectsObject, szDOFName);

        if(lpEmit == nullptr)
        {
	        SCHAR szBuff[128];
	        const char* pStr = "State %s, exhaust smoke, unable to find dof %s, Vehicle type = %s";

	        strprintf(szBuff,
	                pStr,
	                pStateType->Name,
	                (SCHAR*)pStateType->Emitter,
	                Vehicle->VehicleType->Description);

	        WriteLog(LOG_VEHICLE,LOG_WARN, szBuff);
	    }

        pAnim->TreadDustEmitter[q] = lpEmit;
        return 0;
	}

	if(bState != bLastState)
	{
        if(pAnim->TreadDustEmitter[q] != nullptr)
            pAnim->TreadDustOn[q] = bState;
	}

    return 0;
}

UINT32 vehAnimChangeVehicleStateByName(vehVehicle* Vehicle, const UCHAR* StateName, LOGICAL State)
{
UCHAR StateNum;
UINT32 i;

	if (Vehicle == nullptr)
	{
		return 0;
	}

	StateNum = 255;
	for (i=0;i<VEH_MAX_STATE_BITS;i++)
	{
		if (strcmp((char*)StateName,(char*)Vehicle->VehicleType->StateType[i].Name) == 0)
		{
			StateNum = i;
			break;
		}
	}

	if (StateNum == 255)
	{
		// state not found
		return 0;
	}

	// only single-crewed vehicles or correct crewmember can change
	// states via this function
	if (Vehicle->ModelData != nullptr)
	{
		if (Vehicle->ModelData->MultiCrewPos != 0 &&
			Vehicle->VehicleType->StateType[StateNum].MultiCrewPos !=
					Vehicle->ModelData->MultiCrewPos)
		{
			return 0;
		}
	}

// this should get handled in updatestateanimations
//	_SetVehicleDOFState(Vehicle, &Vehicle->VehicleType->StateType[StateNum], State);

	Vehicle->PosData.States[StateNum] = State;

	return 0;
}

int vehAnimFindVehicleStateNumber(const vehVehicle* veh, const char* state)
{
	for(int i = 0; i < VEH_MAX_STATE_BITS; i++)
	{
		if(strcmp(state, (char*)veh->VehicleType->StateType[i].Name) == 0)
		{
			return i;
		}
	}

    return -1;
}

UINT32 vehAnimChangeVehicleStateByNumber(vehVehicle* Vehicle, UCHAR StateNum, LOGICAL State)
{
	// only single-crewed vehicles or correct crewmember can change
	// states via this function
	if (Vehicle->ModelData != nullptr)
	{
		if (Vehicle->ModelData->MultiCrewPos != 0 &&
			Vehicle->VehicleType->StateType[StateNum].MultiCrewPos !=
					Vehicle->ModelData->MultiCrewPos)
		{
			return 0;
		}
	}

//	_SetVehicleDOFState(Vehicle, &Vehicle->VehicleType->StateType[StateNum], State);

	Vehicle->PosData.States[StateNum] = State;

	return 0;
}

// hack for remote turret control
#if(0)
static UINT32 _UpdateTurrets(vehVehicle*Vehicle)
{
//	Vehicle->StateAnimation.TurretElev = Vehicle->ModelData


	return 0;
}
#endif


static UINT32 _UpdateSmokes(vehVehicle* Vehicle)
{
    UINT32 i, j;
    vehanimVehicleStateAnimations* StateAnims = nullptr;
    piVECTOR Pos;
    FLOAT64VECTOR UsePos;
    OBJECT* Object = nullptr;
    FLOAT64VECTOR worldOriginOffset;

	if (Vehicle == nullptr)
	{
		return 0;
	}
	if (Vehicle->VehicleObjectData == nullptr)
	{
		return 0;
	}
	if (Vehicle->VehicleObjectData->PlaneObject == nullptr)
	{
		return 0;
	}

    if(Vehicle->hulksto)
    {
        vehfxCreateHulkSmoke(Vehicle->iFXVehicleID);
        return 0;
    }

    trnGetWorldOriginOffset(&worldOriginOffset);

	StateAnims = &Vehicle->StateAnimation;

#if(0)
    vehfxCheckTankFireState(Vehicle->iFXVehicleID);
#endif

	// check exhaust smokes,
	// oil leak
    for(i = 0; i < ML_MAX_ENGINES; i++)
    {
		if (StateAnims->EngineFireHandle[i] != 0 && StateAnims->EngineFireOn[i] == 0)
		{
#if USE_FX2
            fxremove(StateAnims->EngineFireHandle[i]);
#else
#endif
			StateAnims->EngineFireHandle[i] = 0;
		}

		if (StateAnims->EngineFireHandle[i] == 0 && StateAnims->EngineFireOn[i] != 0)
		{
			if (Vehicle->VehicleObjectData->HitMap == nullptr)
			{
				Object = Vehicle->VehicleObjectData->PlaneObject;
			}
			else
			{
				Object = Vehicle->VehicleObjectData->HitMap;
			}

#if USE_FX2
            {
                piVECTOR n = {0, 0, 0};
                int id = fxaddeffectdof("oilsmoke", StateAnims->EngineFireEmitter[i], n, 0);
                StateAnims->EngineFireHandle[i] = id;
            }
#else
#endif
			if (StateAnims->EngineFireHandle[i] == 0)
			{
				StateAnims->EngineFireHandle[i] = (UINT32)-1;
			}
		}

		if (StateAnims->OilLeakSmokeHandle[i] != 0 && StateAnims->OilLeakSmokeOn[i] == 0)
		{
#if USE_FX2
            fxremove(StateAnims->OilLeakSmokeHandle[i]);
#else
#endif
			StateAnims->OilLeakSmokeHandle[i] = 0;
		}

		if (StateAnims->OilLeakSmokeHandle[i] == 0 && StateAnims->OilLeakSmokeOn[i] != 0)
		{
			Object = StateAnims->OilLeakSmokeEmitter[i];
			if (Object == nullptr || (SINT32)Object == -1 || !(Object->flags & OBJECT_DOF))
			{
				Pos.x = Pos.y = Pos.z = 0.0f;
				Object = Vehicle->VehicleObjectData->PlaneObject;
			}
			else
			{
				Pos.x = Object->dof->dofmatrix2._41;
				Pos.y = Object->dof->dofmatrix2._42;
				Pos.z = Object->dof->dofmatrix2._43;
			}
#if USE_FX2
            {
                piVECTOR n = {0, 0, 0};
                int id = fxaddeffectdof("oilleak", Object, n, 0);
                StateAnims->OilLeakSmokeHandle[i] = id;
            }
#else
#endif
		}

		// glycol system
		for (j = 0; j < ML_MAX_RADIATORS_PER_ENGINE; j++)
		{
			if (StateAnims->GlycolLeakSmokeHandle[i][j] != 0 && StateAnims->GlycolLeakSmokeOn[i][j] == 0)
			{
#if USE_FX2
                fxremove(StateAnims->GlycolLeakSmokeHandle[i][j]);
#else
#endif
				StateAnims->GlycolLeakSmokeHandle[i][j] = 0;
			}

			if (StateAnims->GlycolLeakSmokeHandle[i][j] == 0 && StateAnims->GlycolLeakSmokeOn[i][j] != 0)
			{
				Object = StateAnims->GlycolLeakSmokeEmitter[i][j];
				if (Object == nullptr || (SINT32)Object == -1 || !(Object->flags & OBJECT_DOF))
				{
					Pos.x = Pos.y = Pos.z = 0.0f;
					Object = Vehicle->VehicleObjectData->PlaneObject;
				}
				else
				{
					Pos.x = Object->dof->dofmatrix2._41;
					Pos.y = Object->dof->dofmatrix2._42;
					Pos.z = Object->dof->dofmatrix2._43;
				}
#if USE_FX2
                {
                    piVECTOR n = {0, 0, 0};
                    int id = fxaddeffectdof("glycolleak", Object, n, 0);
                    StateAnims->GlycolLeakSmokeHandle[i][j] = id;
                }
#else
#endif
				if (StateAnims->GlycolLeakSmokeHandle[i][j] == 0)
				{
					StateAnims->GlycolLeakSmokeHandle[i][j] = (UINT32)-1;
				}
			}
		} // END for (j = 0; j < ML_MAX_RADIATORS_PER_ENGINE; j++)
	} //END for (i=0; i < ML_MAX_ENGINES; i++)


	// crashed airplane
	if (StateAnims->PlayCrash == 1 && StateAnims->CrashHasPlayed == 0)
	{
		StateAnims->CrashHasPlayed = 1;

		// big kaboom!  But only for non-boats
        UsePos = Vehicle->PosData.RenderPos;

		if (Vehicle->VehicleType != nullptr)
		{
			if (Vehicle->VehicleType->Type != VEH_TYPE_SEA)
			{
                _DoPlaneCrashFX(Vehicle);
            }
		}
	}
	if (StateAnims->PlayCrash == 1)
	{
		// hide vehicle
		Vehicle->HideVehicle = 1;

		// hide lodloadout
		if(Vehicle->VehicleObjectData && Vehicle->VehicleObjectData->LoadoutLODObject)
			ogrpHideObjectAndSubtree(Vehicle->VehicleObjectData->LoadoutLODObject);
	}

	// fuel fire/leak
	for (i=0;i<ML_MAX_FUELTANKS;i++)
	{
		if (StateAnims->FuelLeakSmokeHandle[i] != 0 && StateAnims->FuelLeakSmokeOn[i] == 0)
		{
#if USE_FX2
            fxremove(StateAnims->FuelLeakSmokeHandle[i]);
#else
#endif
			StateAnims->FuelLeakSmokeHandle[i] = 0;
		}

		if (StateAnims->FuelLeakSmokeHandle[i] == 0 && StateAnims->FuelLeakSmokeOn[i] != 0)
		{
			Object = StateAnims->FuelLeakSmokeEmitter[i];
			if (Object == nullptr || (SINT32)Object == -1 || !(Object->flags & OBJECT_DOF))
			{
				Pos.x = Pos.y = Pos.z = 0.0f;
				Object = Vehicle->VehicleObjectData->PlaneObject;
			}
			else
			{
				Pos.x = Object->dof->dofmatrix2._41;
				Pos.y = Object->dof->dofmatrix2._42;
				Pos.z = Object->dof->dofmatrix2._43;
			}
#if USE_FX2
            {
                piVECTOR n = {0, 0, 0};
                int id = fxaddeffectdof("fuelleak", Object, n, 0);
                StateAnims->FuelLeakSmokeHandle[i] = id;
            }
#else
#endif
			if (StateAnims->FuelLeakSmokeHandle[i] == 0)
			{
				StateAnims->FuelLeakSmokeHandle[i] = (UINT32)-1;
			}
		}

		if (StateAnims->FuelFireHandle[i] != 0 && StateAnims->FuelFireOn[i] == 0)
		{
#if USE_FX2
            fxremove(StateAnims->FuelFireHandle[i]);
            fxremove(StateAnims->FuelFireSmokeHandle[i]);
#else
#endif
			StateAnims->FuelFireHandle[i] = 0;
			StateAnims->FuelFireSmokeHandle[i] = 0;
			StateAnims->FuelFire2Handle[i] = 0;

            vehfxCheckSparkState(Vehicle->iFXVehicleID);
		}

		if (StateAnims->FuelFireHandle[i] == 0 && StateAnims->FuelFireOn[i] != 0)
		{
			Object = StateAnims->FuelFireEmitter[i];
			if (Object == nullptr || (SINT32)Object == -1 || !(Object->flags & OBJECT_DOF))
			{
				Pos.x = Pos.y = Pos.z = 0.0f;
				Object = Vehicle->VehicleObjectData->PlaneObject;
			}
			else
			{
				Pos.x = Object->dof->dofmatrix2._41;
				Pos.y = Object->dof->dofmatrix2._42;
				Pos.z = Object->dof->dofmatrix2._43;
			}

#if USE_FX2
            {
                // Shoot off random sparks for vehicle fires
                vehfxCheckSparkState(Vehicle->iFXVehicleID);

                piVECTOR n = {0, 0, 0};
                int id = -1;

                // HACK!
                if(Vehicle->VehicleType->Type == VEH_TYPE_SEA)
                {
                    id = fxaddeffectdof("shipfuelfire", Object, n, 0);
                }
                else
                {
                    id = fxaddeffectdof("fuelfire", Object, n, 0);
                }

                StateAnims->FuelFireHandle[i] = id;
            }
#else
#endif
			if (StateAnims->FuelFireHandle[i] == 0)
			{
				StateAnims->FuelFireHandle[i] = (UINT32)-1;
			}

#if !USE_FX2
#endif
			if (StateAnims->FuelFire2Handle[i] == 0)
			{
				StateAnims->FuelFire2Handle[i] = (UINT32)-1;
			}

#if USE_FX2
            {
                piVECTOR n = {0, 0, 0};

                int id = -1;

                // HACK!
                if(Vehicle->VehicleType->Type == VEH_TYPE_SEA)
                {
                    id = fxaddeffectdof("shipfuelsmoke", Object, n, 0);
                }
                else
                {
                    id = fxaddeffectdof("fuelsmoke", Object, n, 0);
                }

                StateAnims->FuelFireSmokeHandle[i] = id;
            }
#else
#endif
			if (StateAnims->FuelFireSmokeHandle[i] == 0)
			{
				StateAnims->FuelFireSmokeHandle[i] = (UINT32)-1;
			}
		}
	} // END for (i=0;i<ML_MAX_FUELTANKS;i++)

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Function: _DoPlaneCrashFX
// Description:
//////////////////////////////////////////////////////////////////////
void _DoPlaneCrashFX(vehVehicle*pVehicle)
{
    piVECTOR vecVel;
    LOGICAL bPlayNewExp = FALSE;

    fxSndStopVehicleSounds(pVehicle);

    if(pVehicle->VehicleType->Type == VEH_TYPE_AIR)
    {
        if(pVehicle->ModelData != nullptr)
        {
            vecVel.x = (FLOAT32)pVehicle->ModelData->LastVehicleVel.x;
            vecVel.y = (FLOAT32)pVehicle->ModelData->LastVehicleVel.y;
            vecVel.z = (FLOAT32)pVehicle->ModelData->LastVehicleVel.z;

            bPlayNewExp = pVehicle->ModelData->VehicleRealVel.x <= 0.0001f &&
                          pVehicle->ModelData->VehicleRealVel.y <= 0.0001f &&
                          pVehicle->ModelData->VehicleRealVel.z <= 0.0001f;
        }
        else
        {
            piVECTOR v1, v2;
            FLOAT64VECTOR vecOriginOffset;

            trnGetWorldOriginOffset(&vecOriginOffset);

            v1.x = (FLOAT32)( pVehicle->PosData.RenderPos.x - vecOriginOffset.x);
            v1.y = (FLOAT32)( pVehicle->PosData.RenderPos.y - vecOriginOffset.y);
            v1.z = (FLOAT32)(-pVehicle->PosData.RenderPos.z + vecOriginOffset.z);

            v2 = v1;

            v1.z -= 100.0f;
            v2.z += 100.0f;

            bPlayNewExp = trncdCollideDownMediumList(&v1, &v2);

            vecVel.x = (FLOAT32)pVehicle->PosData.LastUpdateVel.x;
            vecVel.y = (FLOAT32)pVehicle->PosData.LastUpdateVel.y;
            vecVel.z = (FLOAT32)pVehicle->PosData.LastUpdateVel.z;
        }
    }
    else
    {
        if(pVehicle->ModelData)
        {
            vecVel.x = (FLOAT32)pVehicle->ModelData->LastVehicleVel.x;
            vecVel.y = (FLOAT32)pVehicle->ModelData->LastVehicleVel.y;
            vecVel.z = (FLOAT32)pVehicle->ModelData->LastVehicleVel.z;
        }
        else
        {
            vecVel.x = (FLOAT32)pVehicle->PosData.LastUpdateVel.x;
            vecVel.y = (FLOAT32)pVehicle->PosData.LastUpdateVel.y;
            vecVel.z = (FLOAT32)pVehicle->PosData.LastUpdateVel.z;
        }
    }

    if(bPlayNewExp)
    {
        SINT32 iID = fxExpGetNewExplosion();

        if(iID == -1)
            return;

        fxExpSetEnergy(iID, 250000000.0f);
        fxExpSetVehicle(iID, pVehicle);
        fxExpSetType(iID, IDE_EXPTYPE_VEHICLECRASH);

        fxExpSetPosition(iID,
                         pVehicle->PosData.RenderPos.x,
                         pVehicle->PosData.RenderPos.y,
                         pVehicle->PosData.RenderPos.z);

        fxExpSetVelocity(iID, vecVel.x, vecVel.y, vecVel.z);

        fxExpInitExplosion(iID);
    }
    else
    {
        // Do nothing
    }

	// not our vehicle or we haven't bailed out
	if(!pVehicle->ModelData || !vehmainHasBailedOut())
	{
		SINT32 iID = fxSndGetNewSound();

		if(iID > 0)
		{
			fxSndSetSoundName(iID, "sounds\\crashexp.wav");
			fxSndSetPriority(iID, IDE_TP_PRIORITY_HIGH);
			fxSndSetSoundType(iID, TYPE_FINITE_EXTERNAL_SOUND);
			fxSndSetPitch(iID, 1.0f);
			fxSndSetVolume(iID, 1.0f);
			fxSndSetPosition(iID,
							 pVehicle->PosData.RenderPos.x,
							 pVehicle->PosData.RenderPos.y,
							 pVehicle->PosData.RenderPos.z);
			fxSndPushFlags(iID, DEF_SOUND_STOPS_AMBIENCE);
			fxSndSetPlayType(iID, DEF_PLAYTYPE_WORLD);
			fxSndSetMaxDistance(iID, 10000.0f);
			fxSndSetMinDistance(iID, 100.0f);

			fxSndInitSound(iID);
		}
	}
}

//////////////////////////////////////////////////////////////////////
// Function: _EngineSoundStopped
// Description: Callback for engine sound stop. Resets the ID to -1.
// The update should try to restart the engine sound if it is not
// playing.
//////////////////////////////////////////////////////////////////////
void _EngineSoundStopped(void* data)
{
    SINT32* pID = (SINT32*)data;

    if(pID == nullptr)
        return;

    *pID = -1;
}

static UINT32 _HandleSound(vehVehicle* Vehicle,
							vehanimVehicleStateType * StateType,
							LOGICAL State,
							LOGICAL LastState)
{
	FLOAT64VECTOR Pos;
	OBJECT* Object;
	OBJECT* TempObj;

	if (Vehicle->Flags & VEH_FLAGS_UPDATESYSTEM_CONTROLLED ||
		(Vehicle->ModelData != nullptr && Vehicle->ModelData->MultiCrewPos >= 2))
	{
		if(Vehicle->HideVehicle)
			State = FALSE;

		if (StateType->StateSubType == VehAnim::STATE_SUBTYPE_ENGINESOUND)
		{

			if (Vehicle->ModelData != nullptr)
			{
				if (State == TRUE)
				{
					Vehicle->EngineIdleVolume = 1.0f;
					Vehicle->EnginePowerVolume = 1.0f;
				}
				else
				{
					Vehicle->EngineIdleVolume = 0.0f;
					Vehicle->EnginePowerVolume = 0.0f;
				}
			}

            // If sound has not been loaded...keep trying.
			if(State == TRUE)
			{
				for(int i = 0; i < 4; i++)
				{
					if(Vehicle->VehicleType->EngineSound[i][0] != '\0' && Vehicle->StateAnimation.EngineSoundId[StateType->Data-1][i] <= 0)
					{
						Object = Vehicle->StateAnimation.EngineFireEmitter[StateType->Data-1];
						if (Object == nullptr || (SINT32)Object == -1 || !(Object->flags & OBJECT_DOF))
						{
							Pos.x = Pos.y = Pos.z = 0.0f;
							Object = Vehicle->VehicleObjectData->PlaneObject;
						}
						else
						{
							Pos.x = Object->dof->dofmatrix2._41;
							Pos.y = Object->dof->dofmatrix2._42;
							Pos.z = Object->dof->dofmatrix2._43;
							mathVectorMultiply64(&Pos,&Pos,&Object->matint);
						}

                        // Don't load if position has not been set.
                        FLOAT32 fPosValid = 0.0f;

                        fPosValid = (FLOAT32)mathVectDotProduct(&Pos, &Pos);

                        if(fPosValid <= 0.0001f)
                            continue;

						TempObj = Vehicle->VehicleObjectData->HitMap ? Vehicle->VehicleObjectData->HitMap : Vehicle->VehicleObjectData->PlaneObject;

                        SINT32 iID = fxSndGetNewSound();

                        if(iID == -1)
                            continue;

                        fxSndSetPitch(iID, 1.0f);
                        fxSndSetVolume(iID, 1.0f);
                        fxSndSetPosition(iID, Pos.x, Pos.y, Pos.z);
                        fxSndSetSoundType(iID, (Vehicle->ModelData) ? TYPE_INFINITE_SOUND : TYPE_INFINITE_EXTERNAL_SOUND);
	                    fxSndSetPriority(iID, IDE_TP_PRIORITY_HIGH);
                        fxSndSetEmitterObject(iID, TempObj);
                        fxSndSetPlayType(iID, DEF_PLAYTYPE_OBJECT);
                        fxSndSetKillCallback(iID, _EngineSoundStopped);
                        fxSndPushFlags(iID, DEF_SOUND_STOPS_AMBIENCE);
                        fxSndSetSoundName(iID, (SCHAR*)Vehicle->VehicleType->EngineSound[i]);
                        fxSndSetKillData(iID, &Vehicle->StateAnimation.EngineSoundId[StateType->Data-1][i]);

                        _GetSoundDistance(iID, &Vehicle->VehicleType->Type);

                        if(fxSndInitSound(iID))
                        {
						    Vehicle->StateAnimation.EngineSoundId[StateType->Data-1][i] = iID;
                        }
					}
				}
			}

            if(!State && LastState)
			{
				for(int i = 0; i < 4; i++)
				{
					if (Vehicle->StateAnimation.EngineSoundId[StateType->Data-1][i] > 0)
					{
                        // HACK!
                        // Turn off smoke stack for ships
                        if(Vehicle->VehicleType->Type == VEH_TYPE_SEA)
                        {
                            vehfxStopSmokeStack(Vehicle->iFXVehicleID);
                        }

        	            fxSndStopSoundDelayed(Vehicle->StateAnimation.EngineSoundId[StateType->Data-1][i], 1);
						Vehicle->StateAnimation.EngineSoundId[StateType->Data-1][i] = 0;
					}
				}
			}

			if(State && Vehicle->StateAnimation.EngineSoundId[StateType->Data-1][0] > 0)
                _Set3rdPersonPitch(Vehicle, StateType->Data - 1);
		}

		// rigamorale tied to 3rd person exhaust to enginesound
		if (State && !LastState && Vehicle->StateAnimation.FirstUpdateProcessed)
			Vehicle->StateAnimation.ExhaustSmokeStartTime = frametime;

		// tank tread noise
		if(	Vehicle->VehicleType->Type == VEH_TYPE_GROUND)
		{
			FLOAT64 speed2 = mathExactVectorLength2(&Vehicle->PosData.Vel);
			LOGICAL playTreadSound = speed2 > 1.0;

			if(Vehicle->ModelData) // multicrew
				vehfxPlayTankTreadInternal(Vehicle->iFXVehicleID, playTreadSound);
			else
				vehfxPlayTankTreadExternal(Vehicle->iFXVehicleID, playTreadSound);
		}
	}

	if (StateType->StateSubType == VehAnim::STATE_SUBTYPE_STUKASOUND)
	{
		if (State && !LastState)
		{
			if (Vehicle->StateAnimation.StukaSoundId <= 0)
			{
				Pos.x = Pos.y = Pos.z = 0.0f;

				if (Vehicle->VehicleObjectData->HitMap == nullptr)
				{
					TempObj = Vehicle->VehicleObjectData->PlaneObject;
				}
				else
				{
					TempObj = Vehicle->VehicleObjectData->HitMap;
				}

                SINT32 iID = fxSndGetNewSound();

                if(iID != -1)
                {
                    fxSndSetSoundName(iID, (SCHAR*)Vehicle->VehicleType->StukaSound);
                    fxSndSetPriority(iID, IDE_TP_PRIORITY_HIGH);
                    fxSndSetSoundType(iID, TYPE_INFINITE_EXTERNAL_SOUND);
                    fxSndSetPosition(iID, Pos.x, Pos.y, Pos.z);
                    fxSndSetEmitterObject(iID, TempObj);
                    fxSndSetPlayType(iID, DEF_PLAYTYPE_OBJECT);
                    fxSndSetMaxDistance(iID, 1609.0f);
                    fxSndSetMinDistance(iID, 100.0f);
                    fxSndSetKillCallback(iID, _EngineSoundStopped);
                    fxSndSetKillData(iID, &Vehicle->StateAnimation.StukaSoundId);
                    fxSndPushFlags(iID, DEF_SOUND_STOPS_AMBIENCE);
                    fxSndSetVolume(iID, 1.0f);
                    fxSndSetPitch(iID, 1.0f);

                    if(fxSndInitSound(iID))
                    {
                        Vehicle->StateAnimation.StukaSoundId = iID;
                    }
                }
			}
		}
		if (!State && LastState)
		{
			if (Vehicle->StateAnimation.StukaSoundId > 0)
			{
				fxSndStopSoundDelayed(Vehicle->StateAnimation.StukaSoundId, 1);
				Vehicle->StateAnimation.StukaSoundId = 0;
			}
		}

		if (State == TRUE && Vehicle->StateAnimation.StukaSoundId > 0)
		{
            /*
            // Corn says volume is max at all times.
			if (Vehicle->ModelData == nullptr ||
				Vehicle->ModelData->MultiCrewPos >= 2)
			{
				TempVel.x = 0.0;
				TempVel.y = 0.0;
				TempVel.z = 1.0;
				mathVectorMultiply64(&TempVel,&TempVel,&Vehicle->PosData.AttMat);

				Speed = (FLOAT32)mathExactVectorLength(&Vehicle->PosData.Vel);
				// extract out the z component of velocity relative to vehicle
				TempVel.x = Vehicle->PosData.Vel.x + Speed * TempVel.x;
				TempVel.y = Vehicle->PosData.Vel.y + Speed * TempVel.y;
				TempVel.z = Vehicle->PosData.Vel.z + Speed * TempVel.z;
			}
			else
			{
				TempVel.x = 0.0;
				TempVel.y = 0.0;
				TempVel.z = 1.0;
				mathVectorMultiply64(&TempVel,&TempVel,&Vehicle->PosData.AttMat);

				Speed = (FLOAT32)mathExactVectorLength(&Vehicle->ModelData->VehicleRealVel);
				// extract out the z component of velocity relative to vehicle
				TempVel.x = Vehicle->ModelData->VehicleRealVel.x + Speed * TempVel.x;
				TempVel.y = Vehicle->ModelData->VehicleRealVel.y + Speed * TempVel.y;
				TempVel.z = Vehicle->ModelData->VehicleRealVel.z + Speed * TempVel.z;
			}
			Speed = (FLOAT32)mathExactVectorLength(&TempVel);
	        Volume = 0.0f;
			if (Speed > 100.0f)
			{
				Volume = 1.0f;
			}
			if (Speed > 30.0f && Speed <= 100.0f)
			{
				Volume = (Speed - 30.0f) / 70.0f;
			}

			fxMgrChangeVolume(Vehicle->StateAnimation.StukaSoundId, Volume);
            */
		}
	}

    vehfxUpdateTurretSound(Vehicle->iFXVehicleID);

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Function: _Set3rdPersonVolume
//////////////////////////////////////////////////////////////////////
void _Set3rdPersonVolume(vehVehicle*pVehicle, SINT32 iIndex)
{
    if(pVehicle == nullptr) return;

    FLOAT64VECTOR vecVel = {0.0f, 0.0f, 1.0f};
    FLOAT32 fSpeed = 0.0f;
    FLOAT32 fVolume = 0.75f;
    FLOAT32 fAcc = 0.0f;
    SINT32 iTimeDiff = 0;

    mathVectorMultiply64(&vecVel, &vecVel, &pVehicle->PosData.AttMat);

    fSpeed = (FLOAT32)mathExactVectorLength(&pVehicle->PosData.Vel);

    // extract out the z component of velocity relative to vehicle
    vecVel.x = pVehicle->PosData.Vel.x + fSpeed * vecVel.x;
    vecVel.y = pVehicle->PosData.Vel.y + fSpeed * vecVel.y;
    vecVel.z = pVehicle->PosData.Vel.z + fSpeed * vecVel.z;

    fSpeed = (FLOAT32)mathExactVectorLength(&vecVel);

    iTimeDiff = frametime - pVehicle->StateAnimation.LastUpdateTime;

    FLOAT32 fLastAnimVehSpeed = pVehicle->StateAnimation.LastAnimationVehicleSpeed;

    fSpeed = (fSpeed * (FLOAT32)iTimeDiff + fLastAnimVehSpeed * 5000.0f) /
             (FLOAT32)(iTimeDiff + 5000);

    pVehicle->StateAnimation.LastAnimationVehicleSpeed = fSpeed;

    fSpeed = (FLOAT32)mathExactVectorLength(&pVehicle->PosData.Vel);

    fAcc = fSpeed < 0.001f ? 0.0f :
           (FLOAT32)mathVectDotProduct(&vecVel, &pVehicle->PosData.Acc);

    fVolume = fAcc > 5.0f ? 1.0f :
              fAcc < -2.0f ? 0.5f :
              fAcc > 0.0f ? fAcc / 5.0f * 0.25f + 0.74f :
              fAcc / 2.0f * 0.25f + 0.74f;

	for(int i = 0; i < 4; i++)
	{
    	SINT32 iID = pVehicle->StateAnimation.EngineSoundId[iIndex][i];
	    fxSndChangeVolume(iID, fVolume);
	}
}

//////////////////////////////////////////////////////////////////////
// Function: _Set3rdPersonPitch
//////////////////////////////////////////////////////////////////////
void _Set3rdPersonPitch(vehVehicle*pVehicle, SINT32 iIndex)
{
    if(pVehicle == nullptr) return;
    if(iIndex < 0) return;

  //  FLOAT64VECTOR vecVel = {0.0f, 0.0f, 1.0f};
    FLOAT32 fSpeed = 0.0f;
    FLOAT32 fPitch = 1.0f;
 //   SINT32 iTimeDiff = 0;
    SINT32 iLastSpeedRangeChangeTime = 0;

    // Assuming there are 2 settings only here. A start and end for both
    // sound and pitch.
    FLOAT32 fSpeedEnd = pVehicle->VehicleType->EngineSpeedEnd[0];
    FLOAT32 fPitchEnd = pVehicle->VehicleType->EnginePitchEnd[0];
    FLOAT32 fSpeedStart = pVehicle->VehicleType->EngineSpeedStart[0];
    FLOAT32 fPitchStart = pVehicle->VehicleType->EnginePitchStart[0];

    fSpeed = (FLOAT32)mathExactVectorLength(&pVehicle->PosData.Vel);

    iLastSpeedRangeChangeTime = pVehicle->StateAnimation.LastSpeedRangeChangeTime;

	for(int i = 0; i < 4; i++)
	{
    	SINT32 iID = pVehicle->StateAnimation.EngineSoundId[iIndex][i];
	    pVehicle->StateAnimation.LastSpeedRangeChangeTime = frametime;

	    if(fSpeed < fSpeedStart)
	    {
	        fxSndChangePitch(iID, fPitchStart);
	        return;
	    }

	    // Don't want to keep setting pitch here, so just return.
	    if(fSpeed >= fSpeedEnd)
	    {
	        return;
	    }

	    if(fSpeedEnd - fSpeedStart < 0.0001f)
	        return;

	    fPitch = 1.0f - (fSpeedEnd - fSpeed) / (fSpeedEnd - fSpeedStart);
	    fPitch = fPitch * (fPitchEnd  - fPitchStart) + fPitchStart;
	    fxSndChangePitch(iID, fPitch);
	}
}

// recoil equation is
// x = k1 * t * exp(-(k2 * t))
static UINT32 _UpdateRecoils(vehVehicle*Vehicle)
{
UINT32 i;
FLOAT32 K1,K2;
FLOAT32 TempNum;
SINT32 DeltaTime;
vehLoadout * Loadout;

	if (Vehicle == nullptr)
	{
		return 0;
	}

	Loadout = &Vehicle->VehicleType->Loadout[Vehicle->LoadoutNum];

	for (i=0;i<VEH_MAX_MOUNTS;i++)
	{
		if (Vehicle->StateAnimation.RecoilStartTime[i] != 0)
		{
			K2 = 1.0f / Vehicle->StateAnimation.RecoilTime[i];
			K1 = K2 * E_;
			DeltaTime = frametime - Vehicle->StateAnimation.RecoilStartTime[i];
			if (DeltaTime < 0)
			{
				DeltaTime = 0;
			}
			TempNum = (FLOAT32)(K1 * (DeltaTime * 0.001) * exp(-K2 * (DeltaTime * 0.001)));
			if (TempNum < 0.001f && DeltaTime > 0.001f)
			{
				Vehicle->StateAnimation.RecoilStartTime[i] = 0;
				Vehicle->StateAnimation.RecoilPosition[i] = 0.0f;
			}
			else
			{
				Vehicle->StateAnimation.RecoilPosition[i] = TempNum;
			}
		}
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Function: vehanimGetSoundDistance
// Description: Retrieve the min and max distance for mount sounds.
//////////////////////////////////////////////////////////////////////
LOGICAL _GetSoundDistance(SINT32 iID, const UINT32* pType)
{
    if(pType == nullptr)
        return FALSE;

    switch(*pType)
    {
    case VEH_TYPE_GROUND:
        {
            fxSndSetMaxDistance(iID, 1609.0f);
            fxSndSetMinDistance(iID, 25.0f);
            break;
        }
    case VEH_TYPE_AIR:
        {
            fxSndSetMaxDistance(iID, 3218.0f);
            fxSndSetMinDistance(iID, 100.0f);
            break;
        }
    case VEH_TYPE_SEA:
        {
            fxSndSetMaxDistance(iID, 1609.0f);
            fxSndSetMinDistance(iID, 25.0f);
            break;
        }
    default:
        {
            return FALSE;
        }
    }

    return TRUE;
}

/************************************************************************************************
	_HandleVisDamage
*************************************************************************************************/

static UINT32 _HandleVisDamage(vehVehicle* Vehicle, vehanimVehicleStateType* StateType, LOGICAL State, LOGICAL LastState)
{
	if (Vehicle->Flags & VEH_FLAGS_UPDATESYSTEM_CONTROLLED || (Vehicle->ModelData && Vehicle->ModelData->MultiCrewPos > 1))
	{
		if(Vehicle->HideVehicle)
			State = FALSE;

		if(State && !LastState)
			vehVisDamageGroupDestroyed(Vehicle, StateType->Name, _LeaveStub(Vehicle, StateType), Vehicle->StateAnimation.FirstUpdateProcessed);
		//else if(!State && LastState)
		//	vehVisDamageGroupRepaired(Vehicle, StateType->Name);
	}

	if((UINT32)StateType->Data >= VEH_MAX_DEBRIS_POINTS)
		return 0;

	if(State && !LastState)
	{
		if(Vehicle->StateAnimation.DebrisEmitter[StateType->Data] == nullptr)
		{
			ogrpFindSubObjectByName(&Vehicle->StateAnimation.DebrisEmitter[StateType->Data],
									Vehicle->VehicleObjectData->EffectsObject, (char*)StateType->Emitter);
		}

		if(Vehicle->StateAnimation.DebrisEmitter[StateType->Data])
		{
            switch(StateType->StateSubType)
            {
            case VehAnim::STATE_SUBTYPE_CRASHEXPLOSION:
                {
			        Vehicle->StateAnimation.DebrisOn[StateType->Data] = TRUE;
			        vehfxSetDebrisState(Vehicle->iFXVehicleID);
			        Vehicle->StateAnimation.DebrisOn[StateType->Data] = FALSE;

                    Vehicle->StateAnimation.ExplosionOn[StateType->Data] = TRUE;
                    vehfxSetExplosionState(Vehicle->iFXVehicleID);
                    Vehicle->StateAnimation.ExplosionOn[StateType->Data] = FALSE;
                    break;
                }
            case VehAnim::STATE_SUBTYPE_WHEELEJECT:
                {
                    vehfxWheelsOffEffect(Vehicle->iFXVehicleID, StateType->Data);
                    break;
                }
            default:
                {
                    if(StateType->Name)
                    {
                        WriteLog(LOG_OLD, LOG_INFO, "[HandleVisDamage] **FAILED** to find StateSubType for %s", StateType->Name);
                    }

                    Vehicle->StateAnimation.ExplosionOn[StateType->Data] = TRUE;
                    vehfxSetExplosionState(Vehicle->iFXVehicleID);
                    Vehicle->StateAnimation.ExplosionOn[StateType->Data] = FALSE;

                    break;
                }
            }

            const vehanimVehicleStateType* p = nullptr;

            p = vehFindStateByName(Vehicle->VehicleType, "turret");

            // Only set fire if turret blew off
            if(p == StateType)
            {
                vehfxRemoveAllEffects(Vehicle->iFXVehicleID);
                vehfxSetVehicleFireState(Vehicle->iFXVehicleID);
                Vehicle->StateAnimation.bVehicleDestroyed = TRUE;
            }
		}
	}
	else if(!State && LastState)
	{
		if(Vehicle->StateAnimation.DebrisEmitter[StateType->Data] == nullptr)
		{
			ogrpFindSubObjectByName(&Vehicle->StateAnimation.DebrisEmitter[StateType->Data],
									Vehicle->VehicleObjectData->EffectsObject, (char*)StateType->Emitter);
		}

		if(Vehicle->StateAnimation.DebrisEmitter[StateType->Data])
		{
            switch(StateType->StateSubType)
            {
            case VehAnim::STATE_SUBTYPE_CRASHEXPLOSION:
                {
			        Vehicle->StateAnimation.DebrisOn[StateType->Data] = FALSE;
			        vehfxSetDebrisState(Vehicle->iFXVehicleID);
			        //Vehicle->StateAnimation.DebrisOn[StateType->Data] = FALSE;

                    Vehicle->StateAnimation.ExplosionOn[StateType->Data] = FALSE;
                    vehfxSetExplosionState(Vehicle->iFXVehicleID);
                    //Vehicle->StateAnimation.ExplosionOn[StateType->Data] = FALSE;
                    break;
                }
            case VehAnim::STATE_SUBTYPE_WHEELEJECT:
                {
                    //vehfxWheelsOnEffect(Vehicle->iFXVehicleID, StateType->Data);
                    break;
                }
            default:
                {
                    if(StateType->Name)
                    {
                        WriteLog(LOG_OLD, LOG_INFO, "[HandleVisDamage] **FAILED** to find StateSubType for %s", StateType->Name);
                    }

                    Vehicle->StateAnimation.ExplosionOn[StateType->Data] = FALSE;
                    vehfxSetExplosionState(Vehicle->iFXVehicleID);
                    //Vehicle->StateAnimation.ExplosionOn[StateType->Data] = FALSE;

                    break;
                }
            }

            const vehanimVehicleStateType* p = nullptr;

            p = vehFindStateByName(Vehicle->VehicleType, "turret");

            // Only set fire if turret blew off
            if(p == StateType)
            {
				/*
                vehfxRemoveAllEffects(Vehicle->iFXVehicleID);
                vehfxSetVehicleFireState(Vehicle->iFXVehicleID);
                Vehicle->StateAnimation.bVehicleDestroyed = TRUE;
				*/
            }
		}
	}

	return 0;
}

/************************************************************************************************
	_LeaveStub
*************************************************************************************************/
// Tricky bit of business to determine whether or not we need to leave a stub

static LOGICAL _LeaveStub(vehVehicle* Vehicle, vehanimVehicleStateType* StateType)
{
	LOGICAL LeaveStub = TRUE;
	int i;

	// only need to go through this rigamorale if it has a parent visdamage group
	if(StateType->ParentVisDamage[0])
	{
		for(i = 0; i < VEH_MAX_STATE_BITS; i++)
		{
			vehanimVehicleStateType* StateTypeParent = &Vehicle->VehicleType->StateType[i];

			if(!StateTypeParent->Name[0])
				break;

			if(!strcmp(StateTypeParent->Name, StateType->ParentVisDamage))
			{
				if(Vehicle->PosData.States[i])
				{
					LeaveStub = FALSE;
				}else if(StateTypeParent->ParentVisDamage[0]) // go further up the chain if the parent has a parent
				{
					LeaveStub = _LeaveStub(Vehicle, StateTypeParent);
				}

				break;
			}
		}
	}

	return LeaveStub;
}
