#pragma once

struct vehVehicle;
struct vehType;
struct vehanimVehicleStateAnimations;

extern UINT32 vehInitVehicleAnimation(vehVehicle*);
extern UINT32 vehUpdateVehicleAnimation(vehVehicle*);
extern UINT32 vehKillVehicleAnimation(vehVehicle*);

extern UINT32 vehAnimInit();
extern UINT32 vehAnimKill();

extern UINT32 vehAnimUpdateVehicleStateAnimations(vehVehicle*);

extern UINT32 vehAnimChangeVehicleStateByName(vehVehicle*, const UCHAR* StateName, LOGICAL State);
extern UINT32 vehAnimChangeVehicleStateByNumber(vehVehicle*, UCHAR StateNum, LOGICAL State);

extern UINT32 vehAnimResetVehicleStateAnimations(vehVehicle*);

extern UINT32 vehAnimUpdateHostControlledVehicleAnimations(vehVehicle*);

extern UINT32 vehAnimSetDOFObjectVals(struct OBJECT*, struct vehanimVehicleStateAnimations*, vehType*, vehVehicle*);

extern int vehAnimFindVehicleStateNumber(const vehVehicle* veh, const char* state);
