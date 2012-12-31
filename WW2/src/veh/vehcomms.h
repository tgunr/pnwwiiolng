#pragma once

extern UINT32 vehcommSendVehicleRequest(SINT32 VehicleNum,
                                 struct WEAPON_SERIAL const*pWeaponSerial);
extern UINT32 vehcommProcessVehicleRequestResponse(struct VEHICLE_REPLY const*Response);
