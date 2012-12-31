#pragma once

enum
{
	BOMBSIGHT_TYPE_BRITISH_MK9A = 1,
	BOMBSIGHT_TYPE_GERMAN_LOTFEC7B,
	BOMBSIGHT_TYPE_MAX
};

/************************************************************************************************
	persBombsight
*************************************************************************************************/

extern LOGICAL persBombsightInit(void);
extern LOGICAL persBombsightUpdate(void);
extern LOGICAL persBombsightKill(void);
extern LOGICAL persClearBombsight(void);
extern LOGICAL persBombsightSetup(SINT32 Type,SINT8* MountName, void*  InVehicleData);

extern UINT32 persBombsightSpeedInc(SINT32 Param,UINT32 Id,UINT32 Detach);
extern UINT32 persBombsightSpeedDec(SINT32 Param,UINT32 Id,UINT32 Detach);
extern UINT32 persBombsightAltInc(SINT32 Param,UINT32 Id,UINT32 Detach);
extern UINT32 persBombsightAltDec(SINT32 Param,UINT32 Id,UINT32 Detach);

extern FLOAT32 persBombsightSpeed();
extern FLOAT32 persBombsightAltitude();
