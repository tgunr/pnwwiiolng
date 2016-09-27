#include <syscfg.h> // Path is project specific.
#include "ml/mlTypes.h"
#include "pers/persmain.h"
#include "veh/vehanim.h"
#include "rn/rnmain.h"
#include "fx/fxMgr.h"
#include "fx/FXInterface.h"
#include "imap/ww2func.h"
#include "locl/loclApp.h"
#include "locl/loclMain.h"
#include "math/math3d.h"
#include "math/mathvect.h"
#include "include/vars.h"
#include "imap/imapTypes.h"
#include "ml/mlHead.h"
#include "imap/ww2imap.h"
#include "trn/trncd.h"
#include "fp/fpPPO.h"
#include "ui/uiMain.h"
#include "ui/uiChat.h"
#include <iostream>
#include <sstream>
#include <math.h>
#include "wwii/wwiiPPOLoader.h"
#include "fx/fxSnd.h"
#include "client/GameClient.h"
#include "weap/weapSpec.h"

extern std::vector<PPOSpec *> activePPOList;
extern UINT16 activePPOIndex;
extern GameClient* PlayerClient();
extern PPOSpecLoader specLoader;

//static FLOAT64 _LastAutopilotThrottlePos;

// message loc id's
#define MAX_GEAR_OPEN_ID 		265
#define MAX_GEAR_CLOSE_ID 		266
#define MAX_CANOPY_OPEN_ID		267
#define MAX_CANOPY_CLOSE_ID		268
#define GEAR_JAMMED_ID			269
#define FLAPS_JAMMED_ID			270
#define SIREN_ENABLED_ID		271
#define SIREN_DISABLED_ID		272
#define DEPLOY_FOR_TOWING_ID	273
#define PREP_FOR_TOWING_ID		274
#define PREP_FOR_DEPLOY_ID		275
#define OUT_OF_TOW_ID			276
#define DEPLOY_DIVE_BRAKE_ID	277
#define DISABLE_DIVE_BRAKE_ID	278
#define TOGGLE_GEAR_ID 			279
#define DETERMINE_RANGE_ID		280
#define UNABLE_RANGE_ID			281
#define TAILWHEEL_LOCK_ID		282
#define TAILWHEEL_UNLOCK_ID		283
#define CANOPY_JAMMED_ID		284
#define HATCH_OPEN_ID			511
#define HATCH_CLOSE_ID			512
#define CANOPY_OPEN_ID			513
#define CANOPY_CLOSE_ID			514
#define AUTOPILOT_ON_ID			521
#define AUTOPILOT_OFF_ID		522
#define AUTOPILOT_MUSTLEVEL_ID	523
#define AUTOPILOT_OFF_LIMIT_ID	524
#define AUTOPILOT_OFFINPUT_ID	525

static SINT32 _PlaySound(mlVehicleData*, const char*, void* data);
static LOGICAL FlapJamSoundPlaying;

// this function only sets where the flaps are supposed to go
UINT32 mlAdjustFlapPosition(SINT32 Param,UINT32 Id,UINT32 Detach)
{
	mlVehicleData* VehicleData = persGetPersonaModelDataPtr();

	if(VehicleData == nullptr)  return 0;

//	mlAutopilotDisengage(VehicleData);

	if(VehicleData->FlapsJammed)
	{
		if(!FlapJamSoundPlaying)
		{
			FlapJamSoundPlaying = TRUE;
			rnmainListLocText(IDE_LANG_UI, FLAPS_JAMMED_ID);
			_PlaySound(VehicleData, *VehicleData->FlapJamSound ? VehicleData->FlapJamSound : "sounds\\flapdam.wav", &FlapJamSoundPlaying);
		}
		return 0;
	}

	VehicleData->FlapDesiredSetting = (FLOAT64)(Param + 1000) / 2000.0;
	VehicleData->FlapDesiredSetting *= VehicleData->NumberOfFlapPositions;
	mlMoveFlapSetting(Param,Id,Detach);

	return 0;
}

// this moves the flaps to the selected position
UINT32 mlMoveFlapSetting(SINT32 Param,UINT32 Id, UINT32 Detach)
{
	UINT32 FlapPos;
	UCHAR s[32];

	mlVehicleData * VehicleData = persGetPersonaModelDataPtr();
	if (Param == 0)
	{
		VehicleData->AnimationData.FlapsSettingMoveRate = 0.0;
	}
	else
	{
		VehicleData->AnimationData.FlapsSettingMoveRate = (FLOAT32)(1 / (Param / 1000.0));
	}
	FlapPos = (UINT32)(VehicleData->FlapSetting + 0.5);
	sprintf((char*)s,"flapstate%d",FlapPos);
	vehAnimChangeVehicleStateByName((vehVehicle*)VehicleData->VehicleMasterPtr, s, FALSE);

	VehicleData->FlapSetting = VehicleData->FlapDesiredSetting;

	FlapPos = (UINT32)(VehicleData->FlapSetting + 0.5);
	sprintf((char*)s,"flapstate%d",FlapPos);
	vehAnimChangeVehicleStateByName((vehVehicle*)VehicleData->VehicleMasterPtr, s, TRUE);

//	mlAutopilotDisengage(VehicleData);

	return 0;
}


UINT32 mlAdjustElevTrim(SINT32 Param,UINT32 Id,UINT32 Detach)
{
	mlVehicleData * VehicleData = persGetPersonaModelDataPtr();
	VehicleData->ElevatorTabPosition = (FLOAT64)Param / 1000;

	/*
	float val = (float)VehicleData->ElevatorTabPosition * 100;

	char buff[256] = {'\0'};
	if(val < 0)
		sprintf(buff, "%ls %ls %3.1f %ls", uiLocString("hud.readout.elevator").c_str(), uiLocString("hud.readout.trim").c_str(), fabs(val), uiLocString("hud.readout.down").c_str());
	else if(val > 0)
		sprintf(buff, "%ls %ls %3.1f %ls", uiLocString("hud.readout.elevator").c_str(), uiLocString("hud.readout.trim").c_str(), fabs(val), uiLocString("hud.readout.up").c_str());
	else
		sprintf(buff, "%ls %ls 0.0 %ls", uiLocString("hud.readout.elevator").c_str(), uiLocString("hud.readout.trim").c_str(), uiLocString("hud.readout.neutral").c_str());

	uiChatPrintLine(buff);
	*/

//	mlAutopilotDisengage(VehicleData);

	return 0;
}

UINT32 mlAdjustRudderTrim(SINT32 Param,UINT32 Id,UINT32 Detach)
{
	mlVehicleData * VehicleData = persGetPersonaModelDataPtr();
	VehicleData->RudderTabPosition = (FLOAT64)Param / 1000;

	/*
	float val = (float)VehicleData->RudderTabPosition * 100;

	char buff[256] = {'\0'};
	if(val < 0)
		sprintf(buff, "%ls %ls %3.1f %ls", uiLocString("hud.readout.rudder").c_str(), uiLocString("hud.readout.trim").c_str(), fabs(val), uiLocString("hud.readout.left").c_str());
	else if(val > 0)
		sprintf(buff, "%ls %ls %3.1f %ls", uiLocString("hud.readout.rudder").c_str(), uiLocString("hud.readout.trim").c_str(), fabs(val), uiLocString("hud.readout.right").c_str());
	else
		sprintf(buff, "%ls %ls 0.0 %ls", uiLocString("hud.readout.rudder").c_str(), uiLocString("hud.readout.trim").c_str(), uiLocString("hud.readout.neutral").c_str());

	uiChatPrintLine(buff);
	*/

//	mlAutopilotDisengage(VehicleData);

	return 0;
}

UINT32 mlAdjustAileronTrim(SINT32 Param,UINT32 Id,UINT32 Detach)
{
	mlVehicleData * VehicleData = persGetPersonaModelDataPtr();
	VehicleData->AileronTabPosition = (FLOAT64)Param / 1000;

	/*
	float val = (float)VehicleData->AileronTabPosition * 100;

	char buff[256] = {'\0'};
	if(val < 0)
		sprintf(buff, "%ls %ls %3.1f %ls", uiLocString("hud.readout.aileron").c_str(), uiLocString("hud.readout.trim").c_str(), fabs(val), uiLocString("hud.readout.left").c_str());
	else if(val > 0)
		sprintf(buff, "%ls %ls %3.1f %ls", uiLocString("hud.readout.aileron").c_str(), uiLocString("hud.readout.trim").c_str(), fabs(val), uiLocString("hud.readout.right").c_str());
	else
		sprintf(buff, "%ls %ls 0.0 %ls", uiLocString("hud.readout.aileron").c_str(), uiLocString("hud.readout.trim").c_str(), uiLocString("hud.readout.neutral").c_str());

	uiChatPrintLine(buff);
	*/

//	mlAutopilotDisengage(VehicleData);

	return 0;
}

UINT32 mlAdjustStickX(SINT32 Param,UINT32 Id,UINT32 Detach)
{
	mlSetStickXPos((FLOAT64)Param / 1000);
	return 0;
}

UINT32 mlAdjustClutch(SINT32 Param, UINT32 Id, UINT32 Detach)
{
	mlVehicleData * VehicleData = persGetPersonaModelDataPtr();

	if (VehicleData == nullptr)
	{
		return 0;
	}

	VehicleData->ClutchPos = (FLOAT64)((Param + 1000.0) / 2000.0);
	if (VehicleData->ClutchPos > 1.0)
	{
		VehicleData->ClutchPos = 1.0;
	}

	if (VehicleData->ClutchPos < 0.0)
	{
		VehicleData->ClutchPos = 0.0;
	}

	return 0;
}

UINT32 mlRpmStubFunction(SINT32 Param,UINT32 Id,UINT32 Detach)
{
	return 0;
}

UINT32 mlPrimaryTriggerDown(SINT32 Param,UINT32 Id,UINT32 Detach)
{
	mlVehicleData* VehicleData = persGetPersonaModelDataPtr();

	if (VehicleData == nullptr) return 0;
	if (VehicleData->VehicleMasterPtr->RideVehicleHostIndex > 0) return 0;

	VehicleData->PrimaryTriggerDown = Param;
	VehicleData->PicklePrimary = Param;

	return 0;
}

UINT32 mlSecondaryTriggerDown(SINT32 Param,UINT32 Id,UINT32 Detach)
{
	mlVehicleData* VehicleData = persGetPersonaModelDataPtr();

	if (VehicleData == nullptr) return 0;
	if (VehicleData->VehicleMasterPtr->RideVehicleHostIndex > 0) return 0;

	VehicleData->SecondaryTriggerDown = Param;
	VehicleData->PickleSecondary = Param;

	return 0;
}

UINT32 mlJettisonOrdnance(SINT32 Param,UINT32 Id,UINT32 Detach)
{
	mlVehicleData * VehicleData = persGetPersonaModelDataPtr();

	if (VehicleData)
		VehicleData->JettisonTriggerDown = Param;

//	mlAutopilotDisengage(VehicleData);

	return 0;
}

UINT32 mlATGunLeftRight(SINT32 Param,UINT32 Id, UINT32 Detach)
{
	mlVehicleData * VehicleData = persGetPersonaModelDataPtr();

	VehicleData->ATGunMovementLeftRight = (Param) / 1000.0;

	return 0;
}

UINT32 mlATGunForwardBackwards(SINT32 Param,UINT32 Id, UINT32 Detach)
{
	mlVehicleData * VehicleData = persGetPersonaModelDataPtr();

	VehicleData->ATGunMovementForwardsBackwards = (Param) / 1000.0;

	return 0;
}

UINT32 mlLeftBrakeFunction(SINT32 Param, UINT32 Id,UINT32 Detach)
{
	mlVehicleData * VehicleData = persGetPersonaModelDataPtr();
	VehicleData->BrakeForceLeft = (Param + 1000.0) / 2000.0;

	return 0;
}

UINT32 mlRightBrakeFunction(SINT32 Param, UINT32 Id,UINT32 Detach)
{
	mlVehicleData * VehicleData = persGetPersonaModelDataPtr();
	VehicleData->BrakeForceRight = (Param + 1000.0) / 2000.0;

	return 0;
}

UINT32 mlCenterBrakeFunction(SINT32 Param, UINT32 Id, UINT32 Detach)
{
	mlVehicleData * VehicleData = persGetPersonaModelDataPtr();
	VehicleData->BrakeForceCenter = (Param + 1000.0) / 2000.0;

	return 0;
}

UINT32 mlSteeringFunction(SINT32 Param, UINT32 Id, UINT32 Detach)
{
	mlVehicleData * VehicleData = persGetPersonaModelDataPtr();
	VehicleData->Steering = Param / 1000.0;

	return 0;
}

UINT32 mlLeftTrackClutchFunction(SINT32 Param,UINT32 Id, UINT32 Detach)
{
	mlVehicleData * VehicleData = persGetPersonaModelDataPtr();
	VehicleData->LeftTrackClutch = (Param + 1000.0) / 2000.0;


	return 0;
}

UINT32 mlRightTrackClutchFunction(SINT32 Param,UINT32 Id, UINT32 Detach)
{
	mlVehicleData * VehicleData = persGetPersonaModelDataPtr();
	VehicleData->RightTrackClutch = (Param + 1000.0) / 2000.0;

	return 0;
}

// for 360 turret
static void FindMinDifferenceAndDirection(FLOAT64 from, FLOAT64 to, FLOAT64* diff, FLOAT64* dir)
{
	FLOAT64 diff1, diff2;
	if(to > from)
	{
		diff1 = to - from;
		diff2 = 1 - diff1;

		if(diff1 < diff2)
		{
			*diff = diff1;
			*dir = 1;
		}else
		{
			*diff = diff2;
			*dir = -1;
		 }
	}else
	{
		diff1 = from - to;
		diff2 = 1 - diff1;

		if(diff1 < diff2)
		{
			*diff = diff1;
			*dir = -1;
		}else
		{
			*diff = diff2;
			*dir = 1;
		}
	}
}

UINT32 mlTraverseTurretSpeed(SINT32 Param, UINT32 Id, UINT32 data)
{
	mlVehicleData * VehicleData = persGetPersonaModelDataPtr();
	if (VehicleData == nullptr) return 1;
	UINT32 Position;
	imap_callback_ext_data* extdata = (imap_callback_ext_data*)data;
	FLOAT64 Delta, diff, dir;

	//uiChatPrintLine("TurretTraverse: %d %u %u", Param, Id, data);

	persGetPersonaPosition(&Position);

	VehicleData->axisType = (extdata) ? extdata->axisType : IMAP_SYMMETRIC_AXIS;

	if(extdata && extdata->axisType == IMAP_DELTA_AXIS && !extdata->emulateJoystick)
	{
		if(Param && (!mlMouseLookEnabled() || !ww2imapTurretAndMouseLookUseSameKeymap()))
		{
			VehicleData->TurretTraverseDeltaInput[Position] = TRUE;

			Delta = extdata->deltaInput / 1000.0 / 10.0;
			if(VehicleData->VehicleMasterPtr->VehicleType->TurretType[Position] == VEH_TURRET_TYPE_360)
				Delta /= 10.0;

			if(Delta > .5) Delta = .5;
			else if(Delta < -.5) Delta = -.5;

			switch(VehicleData->VehicleMasterPtr->VehicleType->TurretType[Position])
			{
				case VEH_TURRET_TYPE_360:
				{
					// see if we've gone too far ahead
					FindMinDifferenceAndDirection(VehicleData->TurretTraverse[Position], VehicleData->WantedTurretTraverse[Position]+Delta, &diff, &dir);

					if(diff * dir > .2)
					{
						VehicleData->WantedTurretTraverse[Position] = VehicleData->TurretTraverse[Position] + .2f;
					}
					else if(diff * dir < -.2)
					{
						VehicleData->WantedTurretTraverse[Position] = VehicleData->TurretTraverse[Position] - .2f;
					}
					else
					{
						VehicleData->WantedTurretTraverse[Position] += (FLOAT32)Delta;
					}

					//FLOAT64 lastRate = VehicleData->TurretTraverseRate[Position];

					FindMinDifferenceAndDirection(VehicleData->TurretTraverse[Position], VehicleData->WantedTurretTraverse[Position], &diff, &dir);
					VehicleData->TurretTraverseRate[Position] = (FLOAT32)dir;

					if(VehicleData->WantedTurretTraverse[Position] > 0.5f)
						VehicleData->WantedTurretTraverse[Position] -= 1.0f;
					else if(VehicleData->WantedTurretTraverse[Position] < -0.5f)
						VehicleData->WantedTurretTraverse[Position] += 1.0f;

					/*
					if(lastRate < 0 && VehicleData->TurretTraverseRate[Position] > 0)
						VehicleData->WantedTurretTraverse[Position] = VehicleData->TurretTraverse[Position];
					else if(lastRate > 0 && VehicleData->TurretTraverseRate[Position] < 0)
						VehicleData->WantedTurretTraverse[Position] = VehicleData->TurretTraverse[Position];

					if(!Param)
					{
						VehicleData->WantedTurretTraverse[Position] = VehicleData->TurretTraverse[Position];
						VehicleData->TurretTraverseRate[Position] = 0.0;
					}
					*/
				}
				break;

				default:
				{
					VehicleData->WantedTurretTraverse[Position] += (FLOAT32)Delta;

					diff = VehicleData->WantedTurretTraverse[Position] - VehicleData->TurretTraverseWithStops[Position];

					//FLOAT64 lastRate = VehicleData->TurretTraverseRate[Position];

					if(diff > 0)
						VehicleData->TurretTraverseRate[Position] =  1.0;
					else if(diff < 0)
						VehicleData->TurretTraverseRate[Position] = -1.0;

					if(VehicleData->WantedTurretTraverse[Position] > 1.0)
						VehicleData->WantedTurretTraverse[Position] = 1.0;
					else if(VehicleData->WantedTurretTraverse[Position] < -1.0)
						VehicleData->WantedTurretTraverse[Position] = -1.0;

					/*
					if(lastRate < 0 && VehicleData->TurretTraverseRate[Position] > 0)
						VehicleData->WantedTurretTraverse[Position] = VehicleData->TurretTraverse[Position];
					else if(lastRate > 0 && VehicleData->TurretTraverseRate[Position] < 0)
						VehicleData->WantedTurretTraverse[Position] = VehicleData->TurretTraverse[Position];

					if(!Param)
					{
						VehicleData->WantedTurretTraverse[Position] = VehicleData->TurretTraverse[Position];
						VehicleData->TurretTraverseRate[Position] = 0.0;
					}
					*/
				}
				break;
			}
		}
		else
		{
			VehicleData->TurretTraverseDeltaInput[Position] = FALSE;
			VehicleData->TurretTraverseRate[Position] = 0;
		}
	}
	else
	{
		VehicleData->TurretTraverseDeltaInput[Position] = FALSE;
		VehicleData->TurretTraverseRate[Position] = Param / 1000.0f;
	}

	return 0;
}

UINT32 mlTraverseTurret(UINT32 Position)
{
	mlVehicleData * VehicleData = persGetPersonaModelDataPtr();
	if (VehicleData == nullptr) return 1;

	VehicleData->TurretTraverseDeltaInput[Position] = TRUE;

	FLOAT64 diff, dir;

	switch(VehicleData->VehicleMasterPtr->VehicleType->TurretType[Position])
	{
		case VEH_TURRET_TYPE_360:
		{
			// see if we've gone too far ahead
			FindMinDifferenceAndDirection(VehicleData->TurretTraverse[Position], VehicleData->WantedTurretTraverse[Position], &diff, &dir);

			if(diff * dir > .2)
			{
				VehicleData->WantedTurretTraverse[Position] = VehicleData->TurretTraverse[Position] + .2f;
			}
			else if(diff * dir < -.2)
			{
				VehicleData->WantedTurretTraverse[Position] = VehicleData->TurretTraverse[Position] - .2f;
			}

			FindMinDifferenceAndDirection(VehicleData->TurretTraverse[Position], VehicleData->WantedTurretTraverse[Position], &diff, &dir);
			VehicleData->TurretTraverseRate[Position] = (FLOAT32)dir;

			if(VehicleData->WantedTurretTraverse[Position] > 0.5)
				VehicleData->WantedTurretTraverse[Position] -= 1.0;
			else if(VehicleData->WantedTurretTraverse[Position] < -0.5)
				VehicleData->WantedTurretTraverse[Position] += 1.0;
		}
		break;

		default:
		{
			diff = VehicleData->WantedTurretTraverse[Position] - VehicleData->TurretTraverseWithStops[Position];

			if(diff > 0)
				VehicleData->TurretTraverseRate[Position] =  1.0;
			else if(diff < 0)
				VehicleData->TurretTraverseRate[Position] = -1.0;

			if(VehicleData->WantedTurretTraverse[Position] > 1.0)
				VehicleData->WantedTurretTraverse[Position] = 1.0;
			else if(VehicleData->WantedTurretTraverse[Position] < -1.0)
				VehicleData->WantedTurretTraverse[Position] = -1.0;
		}
		break;
	}

	return 0;
}

UINT32 mlElevateTurretSpeed(SINT32 Param, UINT32 Id, UINT32 data)
{
	mlVehicleData* VehicleData = persGetPersonaModelDataPtr();
	if (VehicleData == nullptr) return 1;
	UINT32 Position;
	imap_callback_ext_data* extdata = (imap_callback_ext_data*)data;

	//uiChatPrintLine("TurretElevate: %d %u %u", Param, Id, data);

	persGetPersonaPosition(&Position);

	VehicleData->axisType = (extdata) ? extdata->axisType : IMAP_SYMMETRIC_AXIS;

	if(extdata && extdata->axisType == IMAP_DELTA_AXIS && !extdata->emulateJoystick)
	{
		if(Param && (!mlMouseLookEnabled() || !ww2imapTurretAndMouseLookUseSameKeymap()))
		{
			VehicleData->TurretElevateDeltaInput[Position] = TRUE;
			VehicleData->WantedTurretElevate[Position] += (FLOAT32)-extdata->deltaInput / 1000.0f / 10.0f;

			if(VehicleData->WantedTurretElevate[Position] > 1.0)
				VehicleData->WantedTurretElevate[Position] = 1.0;
			else if(VehicleData->WantedTurretElevate[Position] < -1.0)
				VehicleData->WantedTurretElevate[Position] = -1.0;

			//FLOAT64 lastRate = VehicleData->TurretElevateRate[Position];

			if(VehicleData->WantedTurretElevate[Position] - VehicleData->TurretElevate[Position] > 0)
				VehicleData->TurretElevateRate[Position] = 1.0;
			else if(VehicleData->WantedTurretElevate[Position] - VehicleData->TurretElevate[Position] < 0)
				VehicleData->TurretElevateRate[Position] = -1.0;

			/*
			if(lastRate < 0 && VehicleData->TurretElevateRate[Position] > 0)
				VehicleData->WantedTurretElevate[Position] = VehicleData->TurretElevate[Position];
			else if(lastRate > 0 && VehicleData->TurretElevateRate[Position] < 0)
				VehicleData->WantedTurretElevate[Position] = VehicleData->TurretElevate[Position];

			if(!Param)
			{
				VehicleData->WantedTurretElevate[Position] = VehicleData->TurretElevate[Position];
				VehicleData->TurretElevateRate[Position] = 0.0;
			}
			*/
		}
		else
		{
			VehicleData->TurretElevateDeltaInput[Position] = FALSE;
			VehicleData->TurretElevateRate[Position] = 0;
		}
	}else
	{
		VehicleData->TurretElevateDeltaInput[Position] = FALSE;
		VehicleData->TurretElevateRate[Position] = Param / 1000.0f;
	}

	return 0;
}

UINT32 mlAdjustStickY(SINT32 Param,UINT32 Id,UINT32 Detach)
{
	mlSetStickYPos(-(FLOAT64)Param / 1000);

	return 0;
}

UINT32 mlAdjustStickZ(SINT32 Param,UINT32 Id,UINT32 Detach)
{
	mlSetRudderPosition((FLOAT64)Param / 1000);

	return 0;
}

UINT32 mlAdjustThrottle(SINT32 Param,UINT32 Id,UINT32 Detach)
{
	mlSetThrottlePosition((FLOAT64)(Param + 1000) / 2000);

/*	mlVehicleData* VehicleData = persGetPersonaModelDataPtr();

	if(VehicleData->AutopilotOn)
	{
		if(fabs(_LastAutopilotThrottlePos-_ThrottlePos) > .05) // guard against spikey input
		{
			_LastAutopilotThrottlePos = _ThrottlePos;
			mlAutopilotDisengage(VehicleData);
		}
	}*/

	return 0;
}

UINT32 mlAdjustGas(SINT32 Param,UINT32 Id,UINT32 Detach)
{
	mlSetThrottlePosition((FLOAT64)(Param + 1000) / 2000);

	return 0;
}

UINT32 mlToggleStukaSiren(SINT32 Param, UINT32 Id, UINT32 Detach)
{
	mlVehicleData * VehicleData = persGetPersonaModelDataPtr();

	if (VehicleData->StukaSirenOn == 0)
	{
		if(VehicleData->DiveBrakeOn)
		{
			rnmainListLocText(IDE_LANG_UI, SIREN_ENABLED_ID);
			VehicleData->StukaSirenOn = 1;
			vehAnimChangeVehicleStateByName((vehVehicle*)VehicleData->VehicleMasterPtr, (UCHAR*)"stukasiren", TRUE);
		}
	}
	else
	{
		rnmainListLocText(IDE_LANG_UI, SIREN_DISABLED_ID);
		VehicleData->StukaSirenOn = 0;
		vehAnimChangeVehicleStateByName((vehVehicle*)VehicleData->VehicleMasterPtr, (UCHAR*)"stukasiren", FALSE);
	}

	return 0;
}

UINT32 mlToggleTowbar(SINT32 Param,UINT32 Id,UINT32 Detach)
{
	mlVehicleData* VehicleData = persGetPersonaModelDataPtr();

	if (VehicleData->VehicleMasterPtr->RideVehicleHostIndex > 0 &&
		VehicleData->VehicleMasterPtr->RideSeatNum != VEH_TOWED_VEHICLE) return 0;

	mlHitchVehicle(VehicleData);

	return 0;
}

static void _buildFRU(JSContext* ctx, UINT32 argc, jsval* argv)
{
	PlayerPlacedObject* ppo = fpPPOGet();

	if(ppo && ppo->valid && argc > 0)
	{
		JSBool build = false;
		JS_ValueToBoolean(ctx, argv[0], &build);

		if(!build || !ppo->Build())
			fpPPOSet(nullptr);
	}
}

UINT32 mlToggleMobileSpawn(SINT32 Param, UINT32 Id, UINT32 Detach)
{
        static WEAPON_SPEC troop = NULL_WEAP_SPEC;
        static bool firsttime = true;

        if(firsttime)
        {
                firsttime = false;
                troop = weapCreateSpec("GV_troop.1");
        }

        mlVehicleData* VehicleData = persGetPersonaModelDataPtr();
        vehType* vtype = VehicleData->VehicleMasterPtr->VehicleType;

        PlayerPlacedObject* ppo = fpPPOGet();

        if(ppo)
        {
        	if(ppo->valid)
		{
			uiCallJS("buildirp", _buildFRU, 0);
		}
		else
		{
			fpPPOAlert();
			fpPPOSet(nullptr);
		}
	}
	else
    {
        // only allow FRUs for infrantry when mission leader
        bool allowFrus = vtype->WeaponSpec.wClass != troop.wClass || (Online && PlayerClient()->IsMissionLeader());

		// make sure this class/type has ppo types available to it
		if (specLoader.getAllowedPPOs(vtype->WeaponSpec.wClass, vtype->WeaponSpec.wType, activePPOList, !allowFrus))
        {
            vehmainFRUBuild();
        }
    }

	return 0;
}

static void _soundkill(void* data)
{
	SINT32* id = (SINT32*)data;

	if(id == nullptr)
		return;

	*id = 0;
}

UINT32 mlToggleBombBayDoor(SINT32 Param,UINT32 Id,UINT32 Detach)
{
	mlVehicleData* VehicleData = persGetPersonaModelDataPtr();
	if(VehicleData->VehicleMasterPtr->RideVehicleHostIndex > 0) return 0;

	// determine if we can do it
	FLOAT64 Speed = mathExactVectorLength(&VehicleData->VehicleVel);

	if(VehicleData->BombBayMaxCloseSpeed && VehicleData->BombBayDoor && (Speed > VehicleData->BombBayMaxCloseSpeed))
	{
		return 0;
	}
	if(VehicleData->BombBayMaxOpenSpeed && !VehicleData->BombBayDoor && (Speed > VehicleData->BombBayMaxOpenSpeed))
	{
		return 0;
	}

	VehicleData->BombBayDoor = !VehicleData->BombBayDoor;

	if (VehicleData->BombBayDoor == 1)
	{
		if (VehicleData->TowBarEnabled == TRUE)
		{
			rnmainListLocText(IDE_LANG_UI, OUT_OF_TOW_ID);
			VehicleData->BombBayDoor = !VehicleData->BombBayDoor;
		}
		else
		{
			vehAnimChangeVehicleStateByName(VehicleData->VehicleMasterPtr, (UCHAR*)"bombbaydoor", TRUE);
		}
	}
	else
	{
		vehAnimChangeVehicleStateByName(VehicleData->VehicleMasterPtr, (UCHAR*)"bombbaydoor", FALSE);
	}

	if(VehicleData->VehicleMasterPtr)
	{
		if(VehicleData->VehicleMasterPtr->VehicleType->Type == VEH_TYPE_AIR)
		{
			if(VehicleData->bombbayDoorSoundId > 0)
			{
				fxSndStopSound(VehicleData->bombbayDoorSoundId);
				VehicleData->bombbayDoorSoundId = 0;
			}

			SINT32 id = fxSndGetNewSound();

			if(id > 0)
			{
				fxSndSetSoundName(id, "sounds/bombbaydoor.wav");
				fxSndSetPriority(id, IDE_FP_PRIORITY_LOW);
				fxSndSetSoundType(id, TYPE_FINITE_SOUND);
				fxSndSetVolume(id, 1.0f);
				fxSndSetPitch(id, 1.0f);
				fxSndSetPlayType(id, DEF_PLAYTYPE_HEAD);
				fxSndSetKillCallback(id, _soundkill);
				fxSndSetKillData(id, &VehicleData->bombbayDoorSoundId);

				if(fxSndInitSound(id))
				{
					VehicleData->bombbayDoorSoundId = id;
				}
			}
		}
	}

	return 0;
}

UINT32 mlToggleLandingLight(SINT32 Param,UINT32 Id,UINT32 Detach)
{
	mlVehicleData* VehicleData = persGetPersonaModelDataPtr();
	VehicleData->LandingLightOn = !VehicleData->LandingLightOn;

	return 0;
}

UINT32 mlTogglePanelLight(SINT32 Param,UINT32 Id,UINT32 Detach)
{
	mlVehicleData * VehicleData = persGetPersonaModelDataPtr();
	VehicleData->PanelLightOn = !VehicleData->PanelLightOn;

	return 0;
}

UINT32 mlToggleAuxSight(SINT32 Param,UINT32 Id,UINT32 Detach)
{
	mlVehicleData * VehicleData = persGetPersonaModelDataPtr();
	VehicleData->AuxSightOn = !VehicleData->AuxSightOn;

	return 0;
}



UINT32 mlToggleHatch(SINT32 Param,UINT32 Id,UINT32 Detach)
{
	UINT32 Position;
	FLOAT64VECTOR TempPos;
	UCHAR s[32];

	mlVehicleData * VehicleData = persGetPersonaModelDataPtr();
	persGetPersonaPosition(&Position);
	VehicleData->HatchOpen[Position] = !VehicleData->HatchOpen[Position];

	sprintf((char*)s,"hatchstate%d",Position+1);
	TempPos.x = TempPos.y = TempPos.z = 0.0f;
	if (VehicleData->HatchOpen[Position] == 1)
	{
		if(VehicleData->CrewData.HatchSound[Position][0])
		{
			SINT32 iID = fxSndGetNewSound();

			if(iID > 0)
			{
				fxSndSetSoundName(iID, VehicleData->CrewData.HatchSound[Position]);
				fxSndSetPriority(iID, IDE_FP_PRIORITY_MEDIUM);
				fxSndSetSoundType(iID, TYPE_FINITE_SOUND);
				fxSndSetPitch(iID, 1.0f);
				fxSndSetVolume(iID, 1.0f);
				fxSndSetPosition(iID, TempPos.x, TempPos.y, TempPos.z);
				fxSndSetPlayType(iID, DEF_PLAYTYPE_HEAD);

				fxSndInitSound(iID);
			}
		}
	}
	else
	{
		if(VehicleData->CrewData.HatchCloseSound[Position][0])
		{
			SINT32 iID = fxSndGetNewSound();

			if(iID > 0)
			{
				fxSndSetSoundName(iID, VehicleData->CrewData.HatchCloseSound[Position]);
				fxSndSetPriority(iID, IDE_FP_PRIORITY_MEDIUM);
				fxSndSetSoundType(iID, TYPE_FINITE_SOUND);
				fxSndSetPitch(iID, 1.0f);
				fxSndSetVolume(iID, 1.0f);
				fxSndSetPosition(iID, TempPos.x, TempPos.y, TempPos.z);
				fxSndSetPlayType(iID, DEF_PLAYTYPE_HEAD);

				fxSndInitSound(iID);
			}
		}
		else
		{
			if(VehicleData->CrewData.HatchSound[Position][0])
			{
				SINT32 iID = fxSndGetNewSound();

				if(iID > 0)
				{
					fxSndSetSoundName(iID, VehicleData->CrewData.HatchSound[Position]);
					fxSndSetPriority(iID, IDE_FP_PRIORITY_MEDIUM);
					fxSndSetSoundType(iID, TYPE_FINITE_SOUND);
					fxSndSetPitch(iID, 1.0f);
					fxSndSetVolume(iID, 1.0f);
					fxSndSetPosition(iID, TempPos.x, TempPos.y, TempPos.z);
					fxSndSetPlayType(iID, DEF_PLAYTYPE_HEAD);

					fxSndInitSound(iID);
				}
			}
		}
	}
	vehAnimChangeVehicleStateByName((vehVehicle*)VehicleData->VehicleMasterPtr, s, VehicleData->HatchOpen[Position]);
//	rnmainListLocText(IDE_LANG_UI,VehicleData->CanopyOpen[Position] ? HATCH_OPEN_ID : HATCH_CLOSE_ID);

	return 0;
}

UINT32 mlToggleCanopy(SINT32 Param,UINT32 Id,UINT32 Detach)
{
	mlVehicleData * VehicleData = persGetPersonaModelDataPtr();
	if(VehicleData == nullptr) return 0;
	UINT32 Position;
	FLOAT64VECTOR TempPos;
	FLOAT64 Speed;

	persGetPersonaPosition(&Position);

	if(VehicleData->CanopyJammed[Position])
	{
		rnmainListLocText(IDE_LANG_UI,CANOPY_JAMMED_ID);
		_PlaySound(VehicleData, *VehicleData->CanopyJamSound ? VehicleData->CanopyJamSound : "sounds\\canopyjam.wav", nullptr);
		return 0;
	}

	// determine if we can do it
	Speed = mathExactVectorLength(&VehicleData->VehicleVel);

	if(VehicleData->CanopyMaxCloseSpeed[Position] &&
		VehicleData->CanopyOpen[Position] && (Speed > VehicleData->CanopyMaxCloseSpeed[Position]))
	{
		rnmainListLocText(IDE_LANG_UI,MAX_CANOPY_CLOSE_ID);
		return 0;
	}

	if(VehicleData->CanopyMaxOpenSpeed[Position]&&
		!VehicleData->CanopyOpen[Position] && (Speed > VehicleData->CanopyMaxOpenSpeed[Position]))
	{
		rnmainListLocText(IDE_LANG_UI,MAX_CANOPY_OPEN_ID);
		return 0;
	}

	VehicleData->CanopyOpen[Position] = !VehicleData->CanopyOpen[Position];
//	rnmainListLocText(IDE_LANG_UI,VehicleData->CanopyOpen[Position] ? CANOPY_OPEN_ID : CANOPY_CLOSE_ID);

	char s[32];
	strprintf(s, "canopystate%d", Position + 1);

	TempPos.x = TempPos.y = TempPos.z = 0.0f;
	if (VehicleData->CanopyOpen[Position] == 1)
	{
		if(VehicleData->CrewData.CanopySound[Position][0])
		{
			if(VehicleData->canopySoundId[Position] > 0)
			{
				fxSndStopSound(VehicleData->canopySoundId[Position]);
			}

			SINT32 id = fxSndGetNewSound();

			if(id > 0)
			{
				fxSndSetSoundName(id, VehicleData->CrewData.CanopySound[Position]);
				fxSndSetPriority(id, IDE_FP_PRIORITY_MEDIUM);
				fxSndSetSoundType(id, TYPE_FINITE_SOUND);
				fxSndSetPitch(id, 1.0f);
				fxSndSetVolume(id, 1.0f);
				fxSndSetPosition(id, TempPos.x, TempPos.y, TempPos.z);
				fxSndSetPlayType(id, DEF_PLAYTYPE_HEAD);
				fxSndSetKillCallback(id, _soundkill);
				fxSndSetKillData(id, &VehicleData->canopySoundId[Position]);

				if(fxSndInitSound(id))
				{
					VehicleData->canopySoundId[Position] = id;
				}
			}
		}
	}
	else
	{
		if(VehicleData->CrewData.CanopyCloseSound[Position][0])
		{
			if(VehicleData->canopySoundId[Position] > 0)
			{
				fxSndStopSound(VehicleData->canopySoundId[Position]);
			}

			SINT32 id = fxSndGetNewSound();

			if(id > 0)
			{
				fxSndSetSoundName(id, VehicleData->CrewData.CanopyCloseSound[Position]);
				fxSndSetPriority(id, IDE_FP_PRIORITY_MEDIUM);
				fxSndSetSoundType(id, TYPE_FINITE_SOUND);
				fxSndSetPitch(id, 1.0f);
				fxSndSetVolume(id, 1.0f);
				fxSndSetPosition(id, TempPos.x, TempPos.y, TempPos.z);
				fxSndSetPlayType(id, DEF_PLAYTYPE_HEAD);
				fxSndSetKillCallback(id, _soundkill);
				fxSndSetKillData(id, &VehicleData->canopySoundId[Position]);

				if(fxSndInitSound(id))
				{
					VehicleData->canopySoundId[Position] = id;
				}
			}
		}
		else
		{
			if(VehicleData->CrewData.CanopySound[Position][0])
			{
				if(VehicleData->canopySoundId[Position] > 0)
				{
					fxSndStopSound(VehicleData->canopySoundId[Position]);
				}

				SINT32 id = fxSndGetNewSound();

				if(id > 0)
				{
					fxSndSetSoundName(id, VehicleData->CrewData.CanopySound[Position]);
					fxSndSetPriority(id, IDE_FP_PRIORITY_MEDIUM);
					fxSndSetSoundType(id, TYPE_FINITE_SOUND);
					fxSndSetPitch(id, 1.0f);
					fxSndSetVolume(id, 1.0f);
					fxSndSetPosition(id, TempPos.x, TempPos.y, TempPos.z);
					fxSndSetPlayType(id, DEF_PLAYTYPE_HEAD);
					fxSndSetKillCallback(id, _soundkill);
					fxSndSetKillData(id, &VehicleData->canopySoundId[Position]);

					if(fxSndInitSound(id))
					{
						VehicleData->canopySoundId[Position] = id;
					}
				}
			}
		}
	}

	vehAnimChangeVehicleStateByName((vehVehicle*)VehicleData->VehicleMasterPtr, (const UCHAR*)s, VehicleData->CanopyOpen[Position]);
//	mlAutopilotDisengage(VehicleData);

	return 0;
}


UINT32 mlStarterIsDepressed(SINT32 Param, UINT32 Id, UINT32 Detach)
{
	mlVehicleData* VehicleData = persGetPersonaModelDataPtr();

	if (VehicleData->VehicleMasterPtr->RideVehicleHostIndex > 0) return 0;

	VehicleData->MasterStarterOn = Param ? 1 : 0;
//	mlAutopilotDisengage(VehicleData);

	return 0;
}

UINT32 mlStarterIsNotDepressed(SINT32 Param, UINT32 Id, UINT32 Detach)
{
	mlVehicleData * VehicleData = persGetPersonaModelDataPtr();
	VehicleData->MasterStarterOn = 0;
//	mlAutopilotDisengage(VehicleData);

	return 0;
}

UINT32 mlShiftUp(SINT32 Param,UINT32 Id,UINT32 Detach)
{
	mlVehicleData * VehicleData = persGetPersonaModelDataPtr();

	VehicleData->GearShift = 1;

	return 0;
}

UINT32 mlShiftDown(SINT32 Param,UINT32 Id,UINT32 Detach)
{
	mlVehicleData * VehicleData = persGetPersonaModelDataPtr();

	VehicleData->GearShift = -1;

	return 0;
}

UINT32 mlPropPitchAdjust(SINT32 Param, UINT32 Id, UINT32 Detach)
{
static SINT32 pitch;

	mlVehicleData * VehicleData = persGetPersonaModelDataPtr();
	if(Param == 1)
	{

		if(Id == IMAP_MOM_ADJUST_PROP_UP)
		{
			pitch = 1;
		}
		else if(Id == IMAP_MOM_ADJUST_PROP_DOWN)
		{
			pitch = -1;
		}
		VehicleData->PropPitchAdjust = pitch;
	}
	else
	{
		VehicleData->PropPitchAdjust = 0;
	}
//	mlAutopilotDisengage(VehicleData);

	return 0;
}

UINT32 mlToggleWepMode(SINT32 Param, UINT32 Id, UINT32 Detach)
{
	mlVehicleData * VehicleData = persGetPersonaModelDataPtr();
	VehicleData->ToggleWepMode = Param;

//	mlAutopilotDisengage(VehicleData);

	return 0;
}

UINT32 mlReverse(SINT32 Param,UINT32 Id,UINT32 Detach)
{
	mlVehicleData * VehicleData = persGetPersonaModelDataPtr();

	VehicleData->ReverseToggle = 1;

	return 0;
}
UINT32 mlToggleDiveBrakes(SINT32 Param, UINT32 Id, UINT32 Detach)
{
	mlVehicleData * VehicleData = persGetPersonaModelDataPtr();
	if (VehicleData == nullptr)
	{
		return 0;
	}

	VehicleData->DiveBrakeOn = !VehicleData->DiveBrakeOn;

	if (VehicleData->DiveBrakeOn != 0)
	{
		vehAnimChangeVehicleStateByName((vehVehicle*)VehicleData->VehicleMasterPtr, (UCHAR*)"divebrake", TRUE);
		rnmainListLocText(IDE_LANG_UI,DEPLOY_DIVE_BRAKE_ID);
	}
	else
	{
		vehAnimChangeVehicleStateByName((vehVehicle*)VehicleData->VehicleMasterPtr, (UCHAR*)"divebrake", FALSE);
		rnmainListLocText(IDE_LANG_UI,DISABLE_DIVE_BRAKE_ID);

		if(VehicleData->StukaSirenOn)
		{
			mlToggleStukaSiren(1, IMAP_MOM_STUKASIREN, 0);
		}
	}


	return 0;
}

UINT32 mlToggleGear(SINT32 Param,UINT32 Id,UINT32 Detach)
{
	mlVehicleData * VehicleData = persGetPersonaModelDataPtr();
	if (VehicleData == nullptr) return 0;

	FLOAT64 Speed;

	if(VehicleData->GearJammed)
	{
		if(VehicleData->gearJamSoundId == 0)
		{
			rnmainListLocText(IDE_LANG_UI,GEAR_JAMMED_ID);
			int id = _PlaySound(VehicleData, *VehicleData->GearJamSound ? VehicleData->GearJamSound : "sounds\\hgeardam.wav", &VehicleData->gearJamSoundId);
			VehicleData->gearJamSoundId = id;
		}

		return 0;
	}

	// determine if we can do it
	Speed = mathExactVectorLength(&VehicleData->VehicleVel);

	if(VehicleData->GearMaxCloseSpeed && VehicleData->GearSetting && (Speed > VehicleData->GearMaxCloseSpeed))
	{
		rnmainListLocText(IDE_LANG_UI,MAX_GEAR_CLOSE_ID);
		return 0;
	}
	if(VehicleData->GearMaxOpenSpeed && !VehicleData->GearSetting && (Speed > VehicleData->GearMaxOpenSpeed))
	{
		rnmainListLocText(IDE_LANG_UI,MAX_GEAR_OPEN_ID);
		return 0;
	}

	if (VehicleData->GearSetting == 0.0)
	{
		VehicleData->GearSetting = 1.0;
		if (VehicleData->VehicleMasterPtr != nullptr)
		{
			vehAnimChangeVehicleStateByName((vehVehicle*)VehicleData->VehicleMasterPtr, (UCHAR*)"gearstate", TRUE);
		}

	}
	else
	{
		if (VehicleData->VehicleMasterPtr != nullptr)
		{
			vehAnimChangeVehicleStateByName((vehVehicle*)VehicleData->VehicleMasterPtr, (UCHAR*)"gearstate", FALSE);
		}
		VehicleData->GearSetting = 0.0;
	}

	if (Param == 0)
	{
		VehicleData->AnimationData.GearHandleMoveRate = 0.0;
	}
	else
	{
		VehicleData->AnimationData.GearHandleMoveRate = (FLOAT32)(1 / (Param / 1000.0));
	}

	rnmainListLocText(IDE_LANG_UI, TOGGLE_GEAR_ID);


//	mlAutopilotDisengage(VehicleData);

	return 0;
}


UINT32 mlToggleCowlFlaps(SINT32 Param,UINT32 Id,UINT32 Detach)
{
	mlVehicleData * VehicleData = persGetPersonaModelDataPtr();
	if (VehicleData == nullptr)
	{
		return 0;
	}
//	mlAutopilotDisengage(VehicleData);

	if (VehicleData->CowlFlapSetting[0] == 0.0)
	{
		VehicleData->CowlFlapSetting[0] = 1.0;
	}
	else
	{
		VehicleData->CowlFlapSetting[0] = 0.0;
	}

	if (Param == 0)
	{
		VehicleData->AnimationData.CowlFlapMoveRate[0] = 0.0;
	}
	else
	{
		VehicleData->AnimationData.CowlFlapMoveRate[0] = (FLOAT32)(1 / (Param / 1000.0));
	}

	return 0;
}


UINT32 mlCycleAmmo(SINT32 Param,UINT32 Id,UINT32 Detach)
{
	mlVehicleData * VehicleData = persGetPersonaModelDataPtr();
	if (VehicleData)
		VehicleData->CycleAmmo = 1;

	return 0;
}


UINT32 mlRangeFind(SINT32 Param,UINT32 Id,UINT32 Detach)
{
	piVECTOR V1,V2;

	V1.x = -playpos.x;
	V1.y = -playpos.y;
	V1.z = -playpos.z;

	V2 = playdir;
	math3dScaleVector(&V2,&V2,10000.0f);
	mathAddVectors(&V2,&V2,&V1);

	if (trncdCollideWithVehiclesAndTerrain(
					&V1,
					&V2,
					nullptr,
					nullptr,
					nullptr,
					nullptr,
					nullptr,
					TRUE,
					FALSE,
					FALSE) == TRUE)
	{
		mathAddVectors(&V2,&V1,&playpos);
		mlSetReportRangeAndTime((FLOAT32)(mathExactVectorLength(&V2)), frametime + 8000);
		rnmainListLocText(IDE_LANG_UI,DETERMINE_RANGE_ID);
	}
	else
	{
		rnmainListLocText(IDE_LANG_UI,UNABLE_RANGE_ID);
	}

	return 0;
}

UINT32 mlToggleTailWheelLock(SINT32 Param, UINT32 Id, UINT32 Detach)
{
	mlVehicleData * VehicleData = persGetPersonaModelDataPtr();
	if (VehicleData->TailWheelLocked == 0)
	{
		rnmainListLocText(IDE_LANG_UI, TAILWHEEL_LOCK_ID);
		VehicleData->TailWheelLocked = 1;
	}
	else
	{
		rnmainListLocText(IDE_LANG_UI, TAILWHEEL_UNLOCK_ID);
		VehicleData->TailWheelLocked = 0;
	}

	return 0;
}

static void _PlaySoundFinish(void* data)
{
	if(data)
	{
		*(LOGICAL*)data = FALSE;
	}
}

static SINT32 _PlaySound(mlVehicleData* VehicleData, const char* SoundName, void* data)
{
	SINT32 id = fxSndGetNewSound();

	if(id > 0)
	{
		fxSndSetSoundName(id, (char*)SoundName);
		fxSndSetPriority(id, IDE_FP_PRIORITY_HIGH);
		fxSndSetSoundType(id, TYPE_FINITE_SOUND);
		fxSndSetVolume(id, 1.0f);
		fxSndSetPitch(id, 1.0f);
		fxSndSetPosition(id, 0.0f, 0.0f, 0.0f);
		fxSndSetEmitterObject(id, VehicleData->VehicleObjectData->PlaneObject);
		fxSndSetPlayType(id, DEF_PLAYTYPE_HEAD);
	}

	if(data)
	{
		fxSndSetKillCallback(id, _PlaySoundFinish);
		fxSndSetKillData(id, data);
	}

	if(!fxSndInitSound(id))
	{
		fxSndStopSound(id);
		id = 0;
	}

	return id;
}

UINT32 mlToggleHoist(SINT32 Param,UINT32 Id,UINT32 Detach)
{
	mlVehicleData* VehicleData = persGetPersonaModelDataPtr();
	if(VehicleData)
		mlVehicleToggleHoist(VehicleData);

	return 0;
}

UINT32 mlAdjustWinch(SINT32 Param,UINT32 Id,UINT32 Detach)
{
	mlVehicleData * VehicleData = persGetPersonaModelDataPtr();
	if (VehicleData == nullptr) return 1;
	UINT32 Position;
	SINT32 TurretNum;

	persGetPersonaPosition(&Position);

	TurretNum = VehicleData->VehicleMasterPtr->VehicleType->TurretAux[Position];
	if(TurretNum > 0)
	{
		VehicleData->TurretTraverseRate[TurretNum-1] = Param / 1000.0f;
		VehicleData->HoistTurretNum = TurretNum; // cheesy
		VehicleData->LastHoistRate = Param / 1000.0f;
	}

	return 0;
}

UINT32 mlRideTransport(SINT32 Param,UINT32 Id,UINT32 Detach)
{
	mlVehicleData* VehicleData = persGetPersonaModelDataPtr();

	if(VehicleData)
		mlVehicleRideTransport(VehicleData);

	return 0;
}

UINT32 mlAutopilot(SINT32 Param,UINT32 Id,UINT32 Detach)
{
	if(Param) return 0;

	mlVehicleData* VehicleData = persGetPersonaModelDataPtr();

	if(!VehicleData || VehicleData->VehicleMasterPtr->VehicleType->Type != VEH_TYPE_AIR) return 0;

	if(VehicleData->AutopilotOn)
	{
			rnmainListLocText(IDE_LANG_UI,AUTOPILOT_OFF_ID);
			VehicleData->AutopilotOn = FALSE;
	}else
	{
		if(!mlAutopilotWithinLimits(VehicleData))
		{
			rnmainListLocText(IDE_LANG_UI,AUTOPILOT_MUSTLEVEL_ID);
		}else
		{
			rnmainListLocText(IDE_LANG_UI,AUTOPILOT_ON_ID);
			VehicleData->AutopilotOn = TRUE;
//			_LastAutopilotThrottlePos = _ThrottlePos;
		}
	}

	return 0;
}

void mlAutopilotDisengage(mlVehicleData* VehicleData)
{
	if(!VehicleData || !VehicleData->AutopilotOn) return;

	rnmainListLocText(IDE_LANG_UI,AUTOPILOT_OFF_LIMIT_ID);
	VehicleData->AutopilotOn = FALSE;
}
