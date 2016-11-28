#pragma once

#include "veh/vehmain.h"

#define VEH_CNTRL_ISNOTAXIS 0
#define VEH_CNTRL_ISAXIS 1
#define VEH_CNTRL_ISHOLDDOWN 2
#define VEH_CNTRL_IS_SPECIAL 3

#define VEH_CONTROL_UNAFFECTED 0
#define VEH_CONTROL_DISABLE 1
#define VEH_CONTROL_AXIS_GOTONEUTRAL 2
#define VEH_CONTROL_AXIS_GOTOPOS 3
#define VEH_CONTROL_AXIS_GOTONEG 4
#define VEH_CONTROL_REDUCE_BY_STRENGTH 5

UINT32 vehCntrlInit();
UINT32 vehCntrlReadControlFile(vehType * VehicleType,UINT32 Position);
UINT32 vehCntrlInstallControlCallbacks();
UINT32 vehCntrlGetOperationNumber(UINT32* OperationNum,UCHAR * Name);
UINT32 vehCntrlEnableFunction(UINT32 Id);
UINT32 vehCntrlDisableFunction(UINT32 Id);
UINT32 vehIkTriggeredFunction(UINT32 Id,UINT32 Time);
UINT32 vehcntrlDisableAllControls();
SINT32 vehcntrlGetFunctionIndex(SCHAR* functionName);
UINT32 vehcntrlBailout();
