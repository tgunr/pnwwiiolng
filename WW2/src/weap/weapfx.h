#pragma once
#include "weap/weapammo.h"
#include "ml/mlweapons.h"

/************************************************************************************************
	weapfx
*************************************************************************************************/

// weapAmmo
extern UINT32 weapfxAmmoHit(weapAmmo const*, FASTCOLLIDE const* lpFast, piVECTOR const* OldPos,
						piVECTOR const* NewPos, void* pVehicle, SINT32 iPenetration, FLOAT64 KE);
extern UINT32 weapfxAmmoExplosion(weapAmmo const* ,FLOAT64VECTOR const* Pos, FASTCOLLIDE const* lpFast, SINT32 iPenetration);
extern UINT32 weapfxAmmoTracer(weapAmmo*, FLOAT64VECTOR const* LastPos);
extern UINT32 weapfxAmmoFlyby(weapAmmo*);
extern UINT32 weapfxAmmoMuzzleFlash(weapAmmo const*, FLOAT64VECTOR const* RelativePnt, OBJECT* EmitterObject,
							FLOAT64VECTOR const* Vel, FLOAT64 LaunchTime, const weapWeapon* weapon);
extern UINT32 weapfxAmmoWaterSplash(weapAmmo const*, FLOAT64VECTOR const* CollidePos, OBJECT* CollideObject,
								SINT32 PolyNum, FLOAT64 KE);
extern UINT32 weapfxAmmoAttachSound(weapAmmo*);
extern UINT32 weapfxAmmoDetachAmmo(weapAmmo const*);
extern UINT32 weapfxAttachSmokeGrenadeToAmmo(weapAmmo const*);
extern UINT32 weapfxDetonateSmokeGrenade(FLOAT64VECTOR* pPos, const weapAmmo* ammo);

// weapMount
extern UINT32 weapfxMountFireSound(mlWeaponMount*, UINT32 MountNum, SINT32 RoundsLeft, FLOAT64VECTOR const* Pos, OBJECT* PlaneObject);
extern UINT32 weapfxMountReloadSound(mlWeaponMount*, UINT32 MountNum, FLOAT64VECTOR const* Pos, OBJECT* PlaneObject);
extern UINT32 weapfxMountStopFireSound(mlWeaponMount*, UINT32 MountNum, SINT32 RoundsLeft);

// Initializaion
extern LOGICAL weapfxInit();

extern void weapfxKill();
extern void weapfxGetObjectPosition(FASTCOLLIDE const* pFast, FLOAT64VECTOR* pPos);
extern void weapfxGetVehicleVelocity(void* pVehicle, piVECTOR* lpVel);
extern void weapfxTorpedo(weapAmmo* ammo);
extern void weapfxRicochetEffect(const weapAmmo& Ammo, const FASTCOLLIDE& collide, const char* effect);
