#pragma once

extern void vehdecalSetDecals(struct vehVehicle* vehicle);
extern void vehdecalKill();

extern int vehdecalInit();
extern int vehdecalGetTrooperPersonalDecal(UINT32 id);

extern void vehdecalCreateUnitMap(class HCUnit* unit);
extern bool vehdecalGetUnitTextureForUI(class HCUnit* unit, int* texid, float* u1, float* v1, float* u2, float* v2);
