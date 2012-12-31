#pragma once

// Vehicle update system for the game

struct vehVehicle;
struct VEHICLE_UPDATE_HEADER;

extern UINT32 vehupdateCreatePlayerControlledVehicle(SINT32 VehicleNum,
											  SINT32 VehicleIdNum,
											  SINT32 Index,
											  SINT32 persVehicleIndex,
											  SINT32 MultiCrewPos);

extern UINT32 vehupdateInitVehicle(vehVehicle*Vehicle);
extern UINT32 vehupdateUpdate(UINT32 ForceSend);
extern UINT32 vehupdateCreateVehicle(vehVehicle** Vehicle, SINT32 vehicleType,SINT32 VehicleIdNum, SINT32 Index, ChatHandle ch = 0);
extern void vehicleRemoveVehicle(UINT32 index);

extern UINT32 vehupdateUpdateVehicle(vehVehicle* Vehicle);
extern UINT32 vehupdateDisplayVehicle(vehVehicle* Vehicle);
extern UINT32 vehupdateProcessPositionUpdateMsg(const VEHICLE_UPDATE_HEADER* VehicleHeader);

extern UINT32 vehupdateCreateVehicleLoadObjectPortion(vehVehicle* Vehicle, UINT32 IsMultiCrewVehicle);
extern UINT32 vehUpdateWeapons(vehVehicle* Vehicle);
extern UINT32 vehResupplyWeapons(vehVehicle* Vehicle);

extern void vehupdateInitGhostFiring(vehVehicle*Vehicle);
