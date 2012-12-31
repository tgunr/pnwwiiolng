#pragma once

#include "veh/vehmain.h"

/************************************************************************************************
	vehVisDamage
*************************************************************************************************/

extern SINT32 vehVisDamageGroupDestroyed(vehVehicle*, const char* visdamgroup, LOGICAL LeaveStub, LOGICAL CreateCrap);
extern SINT32 vehVisDamageGroupRepaired(vehVehicle*, const char* visdamgroup);
extern SINT32 vehVisDamageHulkVehicle(vehVehicle*, LOGICAL CreateCrap);
extern SINT32 vehVisDamageUnHulkVehicle(vehVehicle*);
extern SINT32 vehVisDamageUpdateCrap();
extern SINT32 vehVisDamageRemoveAllCrap(vehVehicle*);
extern SINT32 vehVisDamageGroupDestroyed(vehVehicle*, const char* visdamgroup, LOGICAL LeaveStub, LOGICAL CreateCrap);
extern SINT32 vehVisDamageGroupRepaired(vehVehicle*, const char* visdamgroup);
extern SINT32 vehVisDamageHulkVehicle(vehVehicle*, LOGICAL CreateCrap);
extern SINT32 vehVisDamageUnHulkVehicle(vehVehicle*);
extern SINT32 vehVisDamageUpdateCrap();
extern SINT32 vehVisDamageRemoveAllCrap(vehVehicle*);
extern bool vehVisDamageCamera(piVECTOR* pos, piVECTOR* aim);
