#pragma once

#include "fx/FXParticle.h"
#include "fx/FXInterface.h"

#define MAX_EFFECTS         512
#define EFFECT_DATA_SIZE    232
#define MAX_POCKED_TEXTURES 16

#define MAX_BULLETHOLES		512

class CFXEffects;

#define callMemberFunction(CObject, pfnMemberFunction) ((CObject).*(pfnMemberFunction))

struct EFFECT
{
    EFFECT* next;
    EFFECT* prev;

    LOGICAL (CFXEffects::*pfnUpdateCallback)(EFFECT*);
	LOGICAL (CFXEffects::*pfnDetachCallback)(EFFECT *, struct OBJECT * Object);
    LOGICAL (*pfnKillCallback)(EFFECT*);
    LOGICAL (*pfnOutsideCallback)(UINT32);

    SINT8 szData[EFFECT_DATA_SIZE];
    SINT32 iIndex;
    UINT32 uShutdownData;
};

struct EXPLOSION
{
    FLOAT64VECTOR fvecPosition;
    SINT32 iStartFrameTime;
};

struct SMOKETRAIL
{
    struct OBJECT* lpObjAttachedTo;
    struct weapAmmo* ammo;

    piVECTOR OffsetPos;
    piVECTOR LastVel;

    FLOAT32 DragCo;
    FLOAT32 InitAlpha;
    FLOAT32 P1Alpha;
    FLOAT32 P2Alpha;
    FLOAT32 EndAlpha;
    FLOAT32 InitSize;
    FLOAT32 P1Size;
    FLOAT32 P2Size;
    FLOAT32 EndSize;
    FLOAT32 Spacing;
    FLOAT32 Variation;
    FLOAT32 Dispersion;
    FLOAT32 MaxRotationRate;

    SINT32 P1TimeOffset;
    SINT32 P2TimeOffset;
    SINT32 EndTimeOffset;
    SINT32 LastSpecialDropTime;

    UINT32 SpriteType;
    UINT32 ParticleFlags;
    UINT32 MinParticleReleaseTime;
    UINT32 LastUpdateTime;
    UINT32 LastParticleReleaseTime;

    FLOAT64VECTOR LastPos;

};

struct FOUNTAIN
{
    struct OBJECT* lpDOFEmitter;
    struct OBJECT* lpVelocityObj;

    FLOAT32 EmitVelocity;
    FLOAT32 DragCo;
    FLOAT32 InitAlpha;
    FLOAT32 P1Alpha;
    FLOAT32 P2Alpha;
    FLOAT32 EndAlpha;
    FLOAT32 InitSize;
    FLOAT32 P1Size;
    FLOAT32 P2Size;
    FLOAT32 EndSize;
    FLOAT32 MaxRotationRate;
    FLOAT32 ParticlesPerSec;
    FLOAT32 Variation;
    FLOAT32 Dispersion;

    SINT32 P1TimeOffset;
    SINT32 P2TimeOffset;
    SINT32 EndTimeOffset;
    SINT32 LastParticleReleaseTime;

    UINT32 SpriteType;
    UINT32 ParticleFlags;
    UINT32 LastUpdateTime;

};

class CFXEffects
{
public:
	CFXEffects();
	~CFXEffects();

public:
	LOGICAL Initialize();
    LOGICAL Create(SINT32) {return TRUE;}
    LOGICAL Update();
    LOGICAL KillAll();
	LOGICAL DetachFXFromObject(OBJECT * Object);
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

	LOGICAL DoHitSound(FLOAT64VECTOR * Pos,
						FLOAT32 Energy,
						const SINT8 * TextureName);


	LOGICAL DoHitEffect(FLOAT64VECTOR const*Pos,
						 FLOAT64 InEnergy,
						 piVECTOR const* Norm,
						 OBJECT * CollideObject,
						 SINT32 PolyNum,
						 const SINT8 * TexName);

	LOGICAL DoVehicleHitEffect(FLOAT64VECTOR* Pos,
						 FLOAT64 Energy,
						 piVECTOR const* Norm,
						 piVECTOR* InVel,
						 OBJECT * CollideObject);

    LOGICAL AddToActiveList(EFFECT* pNode);
    LOGICAL callbackExplosion(EFFECT*);
	LOGICAL callbackDetachSmokeTrail(EFFECT* pEffect,OBJECT * Object);
	LOGICAL callbackSmokeTrail(EFFECT* pEffect);
	LOGICAL callbackUpdateParticleFountain(EFFECT* pEffect);
	LOGICAL callbackDetachParticleFountain(EFFECT* pEffect, OBJECT * Object);
    LOGICAL ParticleTooSmallToSee(SMOKETRAIL* lpTrail, FLOAT64VECTOR* pPos);
    LOGICAL ReleaseEffect(EFFECT* pEffect);

public:
    void Link(CFXParticle* p) {m_pParticle = p;}
    void AddToFreeList(EFFECT*);

public:
    SINT32 GetFreeNode();
	SINT32 GetTextureId(OBJECT * TopLevelObject,OBJECT * CollideObject);
	SINT32 RemoveEarliestPockEntry();

	LOGICAL StopEffect(SINT32 HandleNum);
	LOGICAL ResetTextureToOriginalTree(OBJECT * Object,SCHAR* TexName);
	LOGICAL UpdatePockTextureUpdateTime(OBJECT * TopLevelObject,OBJECT * CollideObject);
    LOGICAL GetCurrentPosition(SMOKETRAIL* lpTrail, FLOAT64VECTOR* pPos);
	LOGICAL TraverseObjectTreeAndSubstitudeTextures(OBJECT * Object,SINT32 OldTexId,SINT32 NewTexId);
	LOGICAL TraverseObjectSubTreeAndSubstitudeTextures(OBJECT * Object,SINT32 OldTexId,SINT32 NewTexId);
	LOGICAL DetachTexturePockObject(OBJECT * Object);
	LOGICAL ResetTextureToOriginalSubTree(OBJECT * Object,SCHAR* TexName);
private:
    CFXParticle* m_pParticle;

private:
	UINT32 RandomNumberGeneratorSeeded;

private:

	SCHAR TexPockNameArray[MAX_POCKED_TEXTURES][32];
	OBJECT* TexPockMasterObjects[MAX_POCKED_TEXTURES];
	SINT32 LastPockUpdateTime[MAX_POCKED_TEXTURES];

    EFFECT m_EffectArray[MAX_EFFECTS];

    EFFECT m_effectHead;
    EFFECT m_effectTail;
    EFFECT m_effectFreeHead;
    EFFECT m_effectFreeTail;

    piLVERTEX m_Vertices[512];
};
