#pragma once

#include "pi/pi.h"
#include "veh/vehmain.h"	// For VEH_MAX_FIREPOINTS etc

#define MAX_TURRET_SOUNDS 4

#define VEHFX_SINK_VEHICLE  0x00000001

struct FXVEHCONTAINER
{
    FXVEHCONTAINER* next;
    FXVEHCONTAINER* prev;

    unsigned short index;

    // To be removed
    SINT32 _fireid[VEH_MAX_FIREPOINTS];
    SINT32 _smokeid[VEH_MAX_FIREPOINTS];

    // To be removed
    SINT32 iSparkID[ML_MAX_FUELTANKS];

    SINT32 iShipStackSmoke[ML_MAX_ENGINES][2];
    SINT32 iShipScrewsID[2];
    SINT32 iTankTreadSnd[VEH_MAX_TREAD_DUST];
    SINT32 iTankBrakeSnd[VEH_MAX_TREAD_DUST];

    SINT32 cookOffSoundID;
    SINT32 startsinktime;
    SINT32 duration;

    // Turret stuff
    SINT32 turretTraverse[ML_MAX_CREW_POSITIONS][2];
    SINT32 turretElevate[ML_MAX_CREW_POSITIONS];

    FLOAT32 effectrate;

    unsigned char updateflags;

    struct vehVehicle* lpVeh;

    bool killupdate;

};

////////////////////////////
// LOGICAL return functions
////////////////////////////
extern LOGICAL vehfxAddVehicle(FXVEHCONTAINER* lpCon);
extern LOGICAL vehfxAttachAirVehicle(struct vehVehicle* lpVehicle, SINT32* pIndex);
extern LOGICAL vehfxAttachGroundVehicle(struct vehVehicle* lpVehicle, SINT32* pIndex);
extern LOGICAL vehfxAttachSeaVehicle(struct vehVehicle* lpVehicle, SINT32* pIndex);
extern LOGICAL vehfxAttachTrooperVehicle(struct vehVehicle* lpVehicle, SINT32* pIndex);
extern LOGICAL vehfxAttachVehicle(struct vehVehicle* const lpVeh, SINT32* pIndex);
extern LOGICAL vehfxCheckForVehicles();
extern LOGICAL vehfxCreateShockSmoke(SINT32 iID);
extern LOGICAL vehfxDetachVehicle(SINT32 iID);
extern LOGICAL vehfxFreeVehicle(FXVEHCONTAINER* lpCon);
extern LOGICAL vehfxGrabAVehicle(FXVEHCONTAINER** lpCon);
extern LOGICAL vehfxInit();
extern LOGICAL vehfxInitDebris();
extern LOGICAL vehfxPlayTankTreadExternal(SINT32 iID, LOGICAL bOn);
extern LOGICAL vehfxPlayTankTreadInternal(SINT32 iID, LOGICAL bOn);
extern LOGICAL vehfxPlayTankBrake(SINT32 iID, LOGICAL bOn);

////////////////////////////
// void return functions
////////////////////////////
extern void vehfxCheckExhaustState(SINT32 iID);
extern void vehfxCheckSparkState(SINT32 iID);
extern void vehfxCheckTankFireState(SINT32 iID);
extern void vehfxCheckTreadDustState(SINT32 iID);
extern void vehfxCheckWingVortexState(SINT32 iID);
extern void vehfxFXOBJKillCallback(void* pData);
extern void vehfxKill();
extern void vehfxReleaseVehicle(FXVEHCONTAINER* lpCon);
extern void vehfxSetDebrisState(SINT32 iID);
extern void vehfxSetExplosionState(SINT32 iID);
extern void vehfxSetVehicleFireState(SINT32 iID);
extern void vehfxStopSmokeStack(SINT32 iID);
extern void vehfxFuelEmitterSparkCallback(struct FXPARTICLE* lpPcl);
extern void vehfxSparkNoDragCallback(struct FXPARTICLE* lpPcl);
extern void vehfxRemoveAllEffects(SINT32 iID);
extern void vehfxUpdateTurretSound(SINT32 ID);
extern void vehfxWheelsOffEffect(SINT32 id, SINT32 debrisindex);
extern void vehfxCreateHulkSmoke(SINT32 id);
extern void vehfxHulkExplosion(SINT32 id);
extern void vehfxSinkVehicle(SINT32 id, SINT32 duration, FLOAT32 rate);
extern void vehfxUpdate();
