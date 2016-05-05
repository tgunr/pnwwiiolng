#pragma once
// Copyright 2008 Playnet Inc, All rights reserved
// Not to be distributed in API form except as part of a compiled library
// under License or NDA

#include "fx/FXParticle.h"
#include "fx/FXEffects.h"
#include "fx/fxSnd.h"
#include "ob/obcd.h"

#define MAX_OBJECTS     10

class CFXApp
{
public:
	CFXApp();
	~CFXApp();


///////////////////////////////////////////////////////
//            SOUNDS/EFFECTS/PARTICLES               //
///////////////////////////////////////////////////////
public:
    LOGICAL DetachFXFromObject(struct OBJECT*);
	LOGICAL Initialize();
    LOGICAL Update();
    LOGICAL KillAll();

///////////////////////////////////////////////////////
//                  SOUNDS                           //
///////////////////////////////////////////////////////
public:
    LOGICAL PlaySound(SINT32 *,
                      SINT8 *,
                      SINT32,
                      SINT32,
                      SINT32,
                      FLOAT32,
                      FLOAT32);
    LOGICAL PlaySoundInHead(SINT32 *,
                            SINT8 *,
                            SINT32,
                            SINT32,
                            SINT32,
                            FLOAT32,
                            FLOAT32,
                            struct FLOAT64VECTOR*);
    LOGICAL PlaySoundInWorld(SINT32 *,
                             SINT8 *,
                             SINT32,
                             SINT32,
                             SINT32,
                             FLOAT32,
                             FLOAT32,
                             struct FLOAT64VECTOR*);
    LOGICAL PlaySoundAttachedToObject(SINT32 *,
                                      SINT8 *,
                                      SINT32,
                                      SINT32,
                                      SINT32,
                                      FLOAT32,
                                      FLOAT32,
                                      struct FLOAT64VECTOR *,
                                      struct OBJECT*);
	LOGICAL StopSound(SINT32);
	LOGICAL StopSoundDelayed(SINT32);
    LOGICAL ChangePitch(SINT32, FLOAT32);
    LOGICAL ChangeVolume(SINT32, FLOAT32);
    LOGICAL ChangePosition(SINT32, struct FLOAT64VECTOR*);
	LOGICAL SetSoundKillCallback(SINT32,
								UINT32 (*fnSoundKillCallback)(SINT32 iIndex,UINT32 iData),
								UINT32);

	LOGICAL SetSoundToFinite(SINT32);
	LOGICAL SetOverallVolume(FLOAT32 Volume);
	LOGICAL SetExternalVolume(FLOAT32 Volume);

///////////////////////////////////////////////////////
//                    PARTICLES                      //
///////////////////////////////////////////////////////
public:
	LOGICAL DisplayParticles(SINT32 DrawCockpitParticles);

	LOGICAL CreateNewParticle(FLOAT64VECTOR* fvectorPos,
							   piVECTOR* vecVel,
							   FLOAT32 fDragCo,
							   piMATRIX * pimatOrientation,
							   FLOAT32 fInitAlpha,
							   FLOAT32 fP1Alpha,
							   FLOAT32 fP2Alpha,
							   FLOAT32 fEndAlpha,
							   FLOAT32 fInitSize,
							   FLOAT32 fP1Size,
							   FLOAT32 fP2Size,
							   FLOAT32 fEndSize,
							   SINT32 iStartTime,
							   SINT32 iP1Time,
							   SINT32 iP2Time,
							   SINT32 iEndTime,
							   UINT32 uObjectType,
							   piVECTOR* vecRotation,
							   UINT32 uFlags,
							   OBJECT * EmitterObject);

	LOGICAL CreateUserDrawParticle(FLOAT64VECTOR* fvectorPos,
						   piVECTOR* vecVel,
						   FLOAT32 fDragCo,
						   piMATRIX * pimatOrientation,
						   FLOAT32 fInitAlpha,
						   FLOAT32 fP1Alpha,
						   FLOAT32 fP2Alpha,
						   FLOAT32 fEndAlpha,
						   FLOAT32 fInitSize,
						   FLOAT32 fP1Size,
						   FLOAT32 fP2Size,
						   FLOAT32 fEndSize,
						   SINT32 iStartTime,
						   SINT32 iP1Time,
						   SINT32 iP2Time,
						   SINT32 iEndTime,
						   UINT32 UserData,
						   piVECTOR * vecRotation,
						   UINT32 uFlags,
						   OBJECT * EmitterObject,
						   LOGICAL (*UserDraw)(FLOAT64VECTOR * Pos,
									FLOAT64 CurrentAlpha,
									FLOAT64 CurrentSize,
									UINT32 UserData));

///////////////////////////////////////////////////////
//                    EFFECTS                        //
///////////////////////////////////////////////////////
public:
    LOGICAL CreateExplosion(FLOAT64VECTOR*);

	LOGICAL CreateParticleFountain(UINT32 * EffectHandle,
									 OBJECT * EmitterDOF,
									 OBJECT * VelocityObject,
									 FLOAT32 EmitVelocity,
								   FLOAT32 fDragCo,
								   FLOAT32 fInitAlpha,
								   FLOAT32 fP1Alpha,
								   FLOAT32 fP2Alpha,
								   FLOAT32 fEndAlpha,
								   FLOAT32 fInitSize,
								   FLOAT32 fP1Size,
								   FLOAT32 fP2Size,
								   FLOAT32 fEndSize,
								   SINT32 iP1TimeOffset,
								   SINT32 iP2TimeOffset,
								   SINT32 iEndTimeOffset,
								   UINT32 uObjectType,
								   FLOAT32 MaxRotationRate,
								   FLOAT32 ParticlesPerSec,
								   FLOAT32 Variation,
									FLOAT32 Dispersion,
								   UINT32 ParticleFlags);

	LOGICAL CreateSmokeTrail(UINT32 * EffectHandle,
							   OBJECT * ObjectAttachedTo,
  							   piVECTOR * Position,
							   FLOAT32 fDragCo,
							   FLOAT32 fInitAlpha,
							   FLOAT32 fP1Alpha,
							   FLOAT32 fP2Alpha,
							   FLOAT32 fEndAlpha,
							   FLOAT32 fInitSize,
							   FLOAT32 fP1Size,
							   FLOAT32 fP2Size,
							   FLOAT32 fEndSize,
							   SINT32 iP1TimeOffset,
							   SINT32 iP2TimeOffset,
							   SINT32 iEndTimeOffset,
							   UINT32 uObjectType,
							   FLOAT32 MaxRotationRate,
							   FLOAT32 Spacing,
							   FLOAT32 Variation,
							   FLOAT32 Dispersion,
							   FLOAT32 MinParticleReleaseTime,
							   UINT32 uFlags);

	LOGICAL CreateSmokeTrail2(UINT32 * EffectHandle,
							   weapAmmo * ammo,
							   FLOAT32 fDragCo,
							   FLOAT32 fInitAlpha,
							   FLOAT32 fP1Alpha,
							   FLOAT32 fP2Alpha,
							   FLOAT32 fEndAlpha,
							   FLOAT32 fInitSize,
							   FLOAT32 fP1Size,
							   FLOAT32 fP2Size,
							   FLOAT32 fEndSize,
							   SINT32 iP1TimeOffset,
							   SINT32 iP2TimeOffset,
							   SINT32 iEndTimeOffset,
							   UINT32 uObjectType,
							   FLOAT32 MaxRotationRate,
							   FLOAT32 Spacing,
							   FLOAT32 Variation,
							   FLOAT32 Dispersion,
							   FLOAT32 MinParticleReleaseTime,
							   UINT32 uFlags);

	LOGICAL CreateMuzzleFlash(FLOAT64VECTOR const*Pos,
							   FLOAT64VECTOR const*Vel,
							   FLOAT64VECTOR const*ProjVel,
							   FLOAT32 Intensity,
							   SINT32 FireTime,
							   OBJECT * EmitterObject,
							   LOGICAL UseMuzzleSmoke);

	LOGICAL DoExplosiveEffect(const FLOAT64VECTOR* Pos,
									 FLOAT64 Energy);

	LOGICAL DoHitEffect(FLOAT64VECTOR const*Pos,
						 FLOAT64 Energy,
						 piVECTOR const* Norm,
						 OBJECT * CollideObject,
						 SINT32 PolyNum,
						 const SINT8 * TexName);

	LOGICAL DoHitSound(FLOAT64VECTOR * Pos,
						FLOAT32 Energy,
						const SINT8 * TexName);

	LOGICAL DoVehicleHitEffect(FLOAT64VECTOR* Pos,
						 FLOAT64 Energy,
						 piVECTOR const* Norm,
						 piVECTOR* Vel,
						 OBJECT * CollideObject);

	LOGICAL StopEffect(SINT32 HandleNum);

private:
	CFXParticle FXParticle;

private:
    CFXEffects FXEffects;
};
