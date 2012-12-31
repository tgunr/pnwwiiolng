#include <syscfg.h> // Path is project specific.
#include "fx/FXInterface.h"
#include "fx/FXApp.h"
#include "ui/uiMain.h"
#include "fx/fxMgr.h"

CFXApp FXApp;

LOGICAL FXInitialize()
{
    if(!fxMgrInit())
        return FALSE;

	return FXApp.Initialize();
}

LOGICAL FXUpdate()
{
    fxMgrUpdate();

    if(uiInGame())
        FXApp.Update();

    return TRUE;
}

LOGICAL FXKillAll()
{
    fxMgrKill();

    return FXApp.KillAll();
}

LOGICAL FXDetachFXFromObject(OBJECT* o)
{
    if(o->flag2 & OBJECT_HAS_FX)
    {
        fxMgrDetach(o);
        FXApp.DetachFXFromObject(o);
    }

    return TRUE;
}

//////////////////////////////////////////////////////////
//                      PARTICLES                       //
//////////////////////////////////////////////////////////
LOGICAL FXDisplayParticles(SINT32 DrawCockpitParticles)
{
	return FXApp.DisplayParticles(DrawCockpitParticles);
}

LOGICAL FXCreateNewParticle(FLOAT64VECTOR* fvectorPos,
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
	return FXApp.CreateNewParticle(fvectorPos,
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


LOGICAL FXCreateUserDrawParticle(FLOAT64VECTOR* fvectorPos,
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
							PARTICLEUSERDRAW UserDraw)
{
	return FXApp.CreateUserDrawParticle(fvectorPos,
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

//////////////////////////////////////////////////////////
//                      EFFECTS                         //
//////////////////////////////////////////////////////////
LOGICAL FXCreateExplosion(FLOAT64VECTOR* pfvecPosition)
{
    return FXApp.CreateExplosion(pfvecPosition);
}

LOGICAL FXCreateParticleFountain(UINT32 * EffectHandle,
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
	return FXApp.CreateParticleFountain(EffectHandle,
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

LOGICAL FXCreateSmokeTrail(UINT32 * EffectHandle,
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
	return FXApp.CreateSmokeTrail(EffectHandle,
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

LOGICAL FXCreateSmokeTrail2(UINT32* handle,
                            weapAmmo* ammo,
                            FLOAT32 drag,
                            FLOAT32 alpha1,
                            FLOAT32 alpha2,
                            FLOAT32 alpha3,
                            FLOAT32 alpha4,
                            FLOAT32 size1,
                            FLOAT32 size2,
                            FLOAT32 size3,
                            FLOAT32 size4,
                            SINT32 timeoffset1,
                            SINT32 timeoffset2,
                            SINT32 timeoffset3,
                            UINT32 objectType,
                            FLOAT32 maxRotateRate,
                            FLOAT32 spacing,
                            FLOAT32 variation,
                            FLOAT32 dispersion, // dispersion distance
                            FLOAT32 minParticleReleaseTime,
                            UINT32 flags)
{
	return FXApp.CreateSmokeTrail2(handle,
								   ammo,
								   drag,
								   alpha1,
					 			   alpha2,
								   alpha3,
								   alpha4,
								   size1,
								   size2,
								   size3,
								   size4,
								   timeoffset1,
								   timeoffset2,
								   timeoffset3,
								   objectType,
								   maxRotateRate,
								   spacing,
								   variation,
								   dispersion,
								   minParticleReleaseTime,
								   flags);
}

LOGICAL FXCreateMuzzleFlash(FLOAT64VECTOR const* Pos,
						   FLOAT64VECTOR const* Vel,
						   FLOAT64VECTOR const* ProjVel,
						   FLOAT32 Intensity,
						   SINT32 FireTime,
						   OBJECT * EmitterObject,
						   LOGICAL UseMuzzleSmoke)
{
		return FXApp.CreateMuzzleFlash(Pos,Vel,ProjVel,Intensity,FireTime,EmitterObject,UseMuzzleSmoke);

}

LOGICAL FXDoExplosiveEffect(const FLOAT64VECTOR* Pos,
							 FLOAT64 Energy)
{
	return FXApp.DoExplosiveEffect(Pos,Energy);
}


LOGICAL FXDoHitEffect(FLOAT64VECTOR const* Pos,
					 FLOAT64 Energy,
					 piVECTOR const* Norm,
					 OBJECT * CollideObject,
					 SINT32 PolyNum,
					 const SINT8 * TexName)
{
	return FXApp.DoHitEffect(Pos,Energy,Norm,CollideObject,PolyNum,TexName);
}

LOGICAL FXDoVehicleHitEffect(FLOAT64VECTOR* Pos,
					 FLOAT64 Energy,
					 piVECTOR const* Norm,
					 piVECTOR* Vel,
					 OBJECT * CollideObject)
{
	return FXApp.DoVehicleHitEffect(Pos,Energy,Norm,Vel,CollideObject);
}

LOGICAL FXDoHitSound(FLOAT64VECTOR * Pos,
					FLOAT32 Energy,
					const SINT8 * TexName)
{

	return FXApp.DoHitSound(Pos,Energy,TexName);
}

LOGICAL FXStopEffect(SINT32 HandleNum)
{

	return FXApp.StopEffect(HandleNum);
}
