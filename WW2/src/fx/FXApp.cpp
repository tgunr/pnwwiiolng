#include <syscfg.h> // Path is project specific.

/*
	 What am i?
	 Copyright 2008 Playnet Inc, All rights reserved
	 Not to be distributed in API form except as part of a compiled library
	 under License or NDA

	Originating Programmer : Did You Write This?
	Maintaining Programmer : Have You Taken it Over?
	Supporting Programmer  : Have you had Someone Elses Messy Code frobbed off on you?

	Rev 1  c2001
*/

#include "fx/FXApp.h"
#include "mem/mem.h"

#include "weap/weapammo.h"

CFXApp::CFXApp()
{
}

CFXApp::~CFXApp()
{
}

//////////////////////////////////////////////////////////////////////
//                 SOUNDS/GRAPHICS/PARTICLES                 //
//////////////////////////////////////////////////////////////////////
LOGICAL CFXApp::Initialize()
{
    FXEffects.Initialize();
    FXParticle.Initialize();

    FXEffects.Link(&FXParticle);

	return TRUE;
}

LOGICAL CFXApp::Update()
{
    if(!FXEffects.Update())
        return FALSE;

    if(!FXParticle.Update())
        return FALSE;

    return TRUE;
}

LOGICAL CFXApp::KillAll()
{
    if(!FXEffects.KillAll())
        return FALSE;

    if(!FXParticle.KillAll())
        return FALSE;

    return TRUE;
}

LOGICAL CFXApp::DetachFXFromObject(struct OBJECT* lpObject)
{
	weapAmmoObjectCeasingToExist(lpObject);

	if (!FXParticle.DetachFXFromObject(lpObject))
		return FALSE;

	if(!FXEffects.DetachFXFromObject(lpObject))
		return FALSE;

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
//                      PARTICLES                            //
//////////////////////////////////////////////////////////////////////

LOGICAL CFXApp::DisplayParticles(SINT32 DrawCockpitParticles)
{
	return FXParticle.DisplayParticles(DrawCockpitParticles);
}

LOGICAL CFXApp::CreateNewParticle(FLOAT64VECTOR* fvectorPos,
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
								   piVECTOR * vecRotation,
								   UINT32 uFlags,
								   OBJECT * EmitterObject)
{
	return FXParticle.CreateNewParticle(fvectorPos,
										vecVel,
										fDragCo,
										pimatOrientation,
										fInitAlpha,
										fP1Alpha,
										fP2Alpha,
										fEndAlpha,
										fInitSize,
										fP1Size,
										fP2Size,
										fEndSize,
										iStartTime,
										iP1Time,
										iP2Time,
										iEndTime,
										uObjectType,
										vecRotation,
										uFlags,
										EmitterObject);
}

LOGICAL CFXApp::CreateUserDrawParticle(FLOAT64VECTOR* fvectorPos,
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
									UINT32 UserData))
{
	return FXParticle.CreateUserDrawParticle(fvectorPos,
								   vecVel,
								   fDragCo,
								   pimatOrientation,
								   fInitAlpha,
								   fP1Alpha,
								   fP2Alpha,
								   fEndAlpha,
								   fInitSize,
								   fP1Size,
								   fP2Size,
								   fEndSize,
								   iStartTime,
								   iP1Time,
								   iP2Time,
								   iEndTime,
								   UserData,
								   vecRotation,
								   uFlags,
								   EmitterObject,
								   UserDraw);
}

//////////////////////////////////////////////////////////////////////
//                      EFFECTS                              //
//////////////////////////////////////////////////////////////////////
LOGICAL CFXApp::CreateExplosion(FLOAT64VECTOR* pfvecPosition)
{
    return FXEffects.CreateExplosion(pfvecPosition);
}

LOGICAL CFXApp::CreateParticleFountain(UINT32 * EffectHandle,
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
								   UINT32 ParticleFlags)
{

	return FXEffects.CreateParticleFountain(EffectHandle,
											EmitterDOF,
											VelocityObject,
											EmitVelocity,
											fDragCo,
											fInitAlpha,
											fP1Alpha,
											fP2Alpha,
											fEndAlpha,
											fInitSize,
											fP1Size,
											fP2Size,
											fEndSize,
											iP1TimeOffset,
											iP2TimeOffset,
											iEndTimeOffset,
											uObjectType,
											MaxRotationRate,
											ParticlesPerSec,
											Variation,
											Dispersion,
											ParticleFlags);
}

LOGICAL CFXApp::CreateSmokeTrail(UINT32 * EffectHandle,
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
							   UINT32 uFlags)
{
	return FXEffects.CreateSmokeTrail(EffectHandle,
									  ObjectAttachedTo,
									  Position,
									  fDragCo,
									  fInitAlpha,
									  fP1Alpha,
									  fP2Alpha,
									  fEndAlpha,
									  fInitSize,
									  fP1Size,
									  fP2Size,
									  fEndSize,
									  iP1TimeOffset,
									  iP2TimeOffset,
									  iEndTimeOffset,
									  uObjectType,
									  MaxRotationRate,
									  Spacing,
									  Variation,
									  Dispersion,
									  MinParticleReleaseTime,
									  uFlags);
}

LOGICAL CFXApp::CreateSmokeTrail2(UINT32 * EffectHandle,
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
							   UINT32 uFlags)
{
	return FXEffects.CreateSmokeTrail2(EffectHandle,
									  ammo,
									  fDragCo,
									  fInitAlpha,
									  fP1Alpha,
									  fP2Alpha,
									  fEndAlpha,
									  fInitSize,
									  fP1Size,
									  fP2Size,
									  fEndSize,
									  iP1TimeOffset,
									  iP2TimeOffset,
									  iEndTimeOffset,
									  uObjectType,
									  MaxRotationRate,
									  Spacing,
									  Variation,
									  Dispersion,
									  MinParticleReleaseTime,
									  uFlags);
}

LOGICAL CFXApp::DoExplosiveEffect(const FLOAT64VECTOR* Pos,
								 FLOAT64 Energy)
{
	return FXEffects.DoExplosiveEffect(Pos,Energy);
}


LOGICAL CFXApp::CreateMuzzleFlash(FLOAT64VECTOR const*Pos,
							   FLOAT64VECTOR const*Vel,
							   FLOAT64VECTOR const*ProjVel,
							   FLOAT32 Intensity,
							   SINT32 FireTime,
							   OBJECT * EmitterObject,
							   LOGICAL UseMuzzleSmoke)
{
	return FXEffects.CreateMuzzleFlash(Pos,Vel,ProjVel,Intensity,FireTime,EmitterObject,UseMuzzleSmoke);
}

LOGICAL CFXApp::DoHitEffect(FLOAT64VECTOR const*Pos,
						 FLOAT64 Energy,
						 piVECTOR const* Norm,
						 OBJECT * CollideObject,
						 SINT32 PolyNum,
						 const SINT8 * TexName)
{

	return FXEffects.DoHitEffect(Pos,Energy,Norm,CollideObject,PolyNum,TexName);
}

LOGICAL CFXApp::DoVehicleHitEffect(FLOAT64VECTOR* Pos,
							 FLOAT64 Energy,
							 piVECTOR const*Norm,
							 piVECTOR* Vel,
							 OBJECT * CollideObject)
{

	return FXEffects.DoVehicleHitEffect(Pos,Energy,Norm,Vel,CollideObject);
}

LOGICAL CFXApp::DoHitSound(FLOAT64VECTOR * Pos,
				FLOAT32 Energy,
				const SINT8 * TexName)
{

	return FXEffects.DoHitSound(Pos,Energy,TexName);
}

LOGICAL CFXApp::StopEffect(SINT32 HandleNum)
{
	return FXEffects.StopEffect(HandleNum);
}