#pragma once

#include "ob/obcd.h"
#include "weap/weapammo.h"

#define PARTICLE_FLAG_USESORIENTATIONMAT	    0x00000001 // not currently used
#define PARTICLE_FLAG_USESROTATION			    0x00000002 // particle rotates
#define PARTICLE_FLAG_USESOBJECT			    0x00000004 // particle connected to an object
#define PARTICLE_FLAG_USESGRAVITY			    0x00000008 // particle has gravity acceleration
#define PARTICLE_FLAG_USESDRAG				    0x00000010 // particle uses a velocity-based drag acceleration
#define PARTICLE_FLAG_RISES					    0x00000020 // particle has a rising acceleration
#define PARTICLE_FLAG_GLOWS					    0x00000040 // particle uses additive render mode
#define PARTICLE_FLAG_USE_VEL_FOR_STRETCH	    0x00000080 // particle stretches (vel used for stretch)
#define PARTICLE_FLAG_INCOCKPIT_PARTICLE	    0x00000100 // particle displays inside cockpit only
#define PARTICLE_FLAG_USE_ALPHAREF			    0x00000200 // uses alpharef to solidify edges
#define PARTICLE_FLAG_PRIORITY				    0x00000400 // particle has priority over other particles
#define PARTICLE_FLAG_AT_LEAST_1_PIXEL          0x00000800 // ensures that particle size at least 1 pixel
#define PARTICLE_FLAG_DISPLAY_FOR_ONE_FRAME	    0x00001000 // display particle, at original settings, for one frame
#define PARTICLE_FLAG_ONE_FRAME_ALREADY		    0x00002000 // used with the previous flag, to indicate time to remove particle
#define PARTICLE_FLAG_DISPLAY_AFTER_ONE_FRAME   0x00004000 // start displaying after one frame
#define PARTICLE_FLAG_DONT_GO_BELOW_START_ALT   0x00008000 // prevents falling below ground
#define PARTICLE_FLAG_CALL_USERDRAW			    0x00010000 // calls a userdraw function for this particle's draw
#define PARTICLE_FLAG_NEVERDIES				    0x00020000 // particle never goes away
#define PARTICLE_FLAG_ATTACHED_TO_OBJECT	    0x00040000 // particle is attached to object
#define PARTICLE_FLAG_ATTACHED_AMMO             0x00080000 // particle uses ammo object for positions.

//////////////////////////////////////////////////////////////////////
//                 SOUNDS/PARTICLES/GRAPHICS                   //
//////////////////////////////////////////////////////////////////////
extern LOGICAL FXInitialize();
extern LOGICAL FXUpdate();
extern LOGICAL FXKillAll();
extern LOGICAL FXDetachFXFromObject(OBJECT*); //[IN] Object to detach FX

//////////////////////////////////////////////////////////////////////
//                       PARTICLES                             //
//////////////////////////////////////////////////////////////////////

using PARTICLEUSERDRAW = LOGICAL(*)(FLOAT64VECTOR * Pos,
									FLOAT64 CurrentAlpha,
									FLOAT64 CurrentSize,
									UINT32 UserData);


extern LOGICAL FXDisplayParticles(SINT32 DrawCockpitParticles);

// note: for billboard particles, the vecRotation variable is as follows:
// vecRotation->x = initial orientation
// vecRotation->y = rotation rate
//
// if EmitterObject is specified, the particle will get it's starting point
// by assuming fvectorPos is a relative position, and calculate where that position is
// relative to the vehicle, in world space
extern LOGICAL FXCreateNewParticle(FLOAT64VECTOR* fvectorPos,
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
								   OBJECT * EmitterObject);

// next function sets up a particle placeholder object, that will call UserDraw whenever
// this particle is drawn
//
// this guarantees that the particle is sorted with the other particles
// however, if the PARTICLE_FLAG_USE_ALPHAREF flag is set, it won't sort with the transparant
// particles, so only set the PARTICLE_FLAG_USE_ALPHAREF if particle sort order isn't necessary
// (as in no transparancy on anything drawn)
extern LOGICAL FXCreateUserDrawParticle(FLOAT64VECTOR* fvectorPos,
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
								   PARTICLEUSERDRAW UserDraw);

//////////////////////////////////////////////////////////////////////
//                       EFFECTS                               //
//////////////////////////////////////////////////////////////////////
extern LOGICAL FXCreateExplosion(FLOAT64VECTOR*);
extern LOGICAL FXCreateParticleFountain(UINT32 * EffectHandle,
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
									FLOAT32 Dispersion, // dispersion in degrees
								   UINT32 ParticleFlags);
extern LOGICAL FXCreateSmokeTrail(UINT32 * EffectHandle,
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
							   FLOAT32 Dispersion, // dispersion distance
							   FLOAT32 MinParticleReleaseTime,
							   UINT32 uFlags);

extern LOGICAL FXCreateSmokeTrail2(UINT32* handle,
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
							       UINT32 flags);

// if an emitterobject is specified, the muzzle flash will assume "Pos" is a relative
// position otherwise it's an absolute
extern LOGICAL FXCreateMuzzleFlash(FLOAT64VECTOR const*Pos,
								   FLOAT64VECTOR const*Vel,
								   FLOAT64VECTOR const*ProjVel,
								   FLOAT32 Intensity,
								   SINT32 FireTime,
								   OBJECT * EmitterObject,
								   LOGICAL UseMuzzleSmoke);

extern LOGICAL FXDoHitEffect(FLOAT64VECTOR const* Pos,
							 FLOAT64 Energy,
							 piVECTOR const* Norm,
							 OBJECT * CollideObject,
							 SINT32 PolyNum,
							 const SINT8 * TexName);

extern LOGICAL FXDoVehicleHitEffect(FLOAT64VECTOR* Pos,
							 FLOAT64 Energy,
							 piVECTOR const* Norm,
							 piVECTOR* Vel,
							 OBJECT * CollideObject);

extern LOGICAL FXDoExplosiveEffect(const FLOAT64VECTOR* Pos,
								 FLOAT64 Energy);

extern LOGICAL FXDoHitSound(FLOAT64VECTOR * Pos,
							FLOAT32 Energy,
							const SINT8 * TexName);

extern LOGICAL FXStopEffect(SINT32 HandleNum);
