#pragma once

/** This is the NON-threaded vehicle loading interface. */

struct vehVehicle;

extern void vehloadCreateVehicleInstance(struct vehVehicle*, LOGICAL hasCockpit);

#ifdef USE_SHADOWS
extern void vehloadCreateShadow(struct vehVehicle*);
#endif
