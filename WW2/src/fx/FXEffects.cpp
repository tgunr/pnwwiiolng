#include <syscfg.h> // Path is project specific.
//////////////////////////////////////////////////////////////////////
// Module: FXEffects.cpp
// Description:
//////////////////////////////////////////////////////////////////////
#include "fx/FXEffects.h"

#include "pi/pi.h"
#include "include/vars.h"

#include <math.h>

#include "fx/FXInterface.h"
#include "trn/trnmain.h"
#include "math/mathvect.h"
#include "math/math3d.h"
#include "math/matheul.h"
#include "math/mathnum.h"
#include "ml/mlAtmo.h"
#include "ld/ldpix.h"
#include "rn/rnmain.h"
#include "locl/loclMain.h"
#include "ml/mltypes.h"
#include "ml/mlutility.h"
#include "ob/obdlite.h"
#include "weap/weapammo.h"
#include "veh/vehmain.h"
#include "fx/fxMgr.h"
#include "fx/fxUtl.h"
#include "fx/fxSnd.h"

#define SMOKE_TRAIL_MIN_NUM_PARTICLES_PER_SEC       5
#define SMOKE_TRAIL_MAX_NUM_PARTICLES_PER_UPDATE    100
#define FOUNTAIN_MAX_NUM_PARTICLES_PER_UPDATE       100

CFXEffects::CFXEffects()
{
    m_pParticle = NULL;

    memset(m_EffectArray, 0, sizeof(m_EffectArray));
    memset(TexPockNameArray, 0, (sizeof(SCHAR) * MAX_POCKED_TEXTURES * 32));
    memset(TexPockMasterObjects, 4, (sizeof(OBJECT*) * MAX_POCKED_TEXTURES));
    memset(m_EffectArray, 0, (sizeof(EFFECT) * MAX_EFFECTS));

    memset(&m_effectHead, 0, sizeof(EFFECT));
    memset(&m_effectTail, 0, sizeof(EFFECT));

    m_effectHead.next = &m_effectTail;
    m_effectTail.prev = &m_effectHead;

    memset(&m_effectFreeHead, 0, sizeof(EFFECT));
    memset(&m_effectFreeTail, 0, sizeof(EFFECT));

    m_effectFreeHead.next = &m_effectFreeTail;
    m_effectFreeTail.prev = &m_effectFreeHead;

    memset(m_Vertices, 0, (sizeof(piLVERTEX) * 512));
}

CFXEffects::~CFXEffects()
{
}

LOGICAL CFXEffects::ReleaseEffect(EFFECT* pEffect)
{
    EFFECT* pNext = NULL;
    EFFECT* pPrev = NULL;

    if(pEffect == NULL)
        return FALSE;

    pNext = pEffect->next;
    pPrev = pEffect->prev;

    if(pNext == NULL || pPrev == NULL)
        return FALSE;

    pPrev->next = pNext;
    pNext->prev = pPrev;

    pEffect->prev = NULL;
    pEffect->next = NULL;

    return TRUE;
}

void CFXEffects::AddToFreeList(EFFECT* pNode)
{
    EFFECT* pPrev = NULL;
    SINT32 i;

    if(pNode == NULL)
        return;

    ReleaseEffect(pNode);

    i = pNode->iIndex;

    memset(pNode, 0, sizeof(EFFECT));

    pNode->iIndex = i;

    pPrev = m_effectFreeTail.prev;

    pNode->prev = pPrev;
    pNode->next = &m_effectFreeTail;

    pPrev->next = pNode;
    m_effectFreeTail.prev = pNode;
}

LOGICAL CFXEffects::AddToActiveList(EFFECT* pNode)
{
    EFFECT* pNext = NULL;

    if(pNode == NULL)
        return FALSE;

   /* if(pNode == &m_effectFreeTail)
    {
        int a = 0;
    }*/

    pNext = m_effectHead.next;

    m_effectHead.next = pNode;
    pNode->prev = &m_effectHead;

    pNode->next = pNext;
    pNext->prev = pNode;

    pNode = m_effectHead.next;

    return TRUE;
}

LOGICAL CFXEffects::Initialize()
{
    SINT32 i = 0;

    for(i = 0; i < MAX_EFFECTS; i++)
    {
        m_EffectArray[i].iIndex = i;
        AddToFreeList(&m_EffectArray[i]);
    }

	// set up chaotic random number seed
	RandomNumberGeneratorSeeded = 0;

	return TRUE;
}

LOGICAL CFXEffects::Update()
{
    EFFECT* pNode = m_effectHead.next;

    while(pNode != &m_effectTail)
    {
        if(pNode->pfnUpdateCallback == NULL)
        {
            EFFECT* pFree = pNode;
            pNode = pNode->next;
            AddToFreeList(pFree);
            continue;
        }

        if(!callMemberFunction(*this, pNode->pfnUpdateCallback)(pNode))
        {
            EFFECT* pFree = pNode;
            pNode = pNode->next;

            if(pFree->pfnKillCallback != NULL)
            {
                pFree->pfnKillCallback(pFree);
            }

            if(pFree->pfnOutsideCallback != NULL)
            {
                pFree->pfnOutsideCallback(pFree->uShutdownData);
            }

            AddToFreeList(pFree);
            continue;
        }

        pNode = pNode->next;
    }

    return TRUE;
}

LOGICAL CFXEffects::KillAll()
{
    EFFECT* pNode = m_effectHead.next;
    EFFECT* pFree = NULL;

    while(pNode != &m_effectTail)
    {
        pFree = pNode;
        pNode = pNode->next;

        if(pFree->pfnKillCallback != NULL)
        {
            pFree->pfnKillCallback(pFree);
        }

        if(pFree->pfnUpdateCallback != NULL)
        {
            pFree->pfnOutsideCallback(pFree->uShutdownData);
        }

        AddToFreeList(pFree);
    }

    return TRUE;
}

LOGICAL CFXEffects::callbackDetachParticleFountain(EFFECT* pEffect, OBJECT * Object)
{
FOUNTAIN * Fountain;

	if (pEffect == NULL)
	{
		return TRUE;
	}

	Fountain = (FOUNTAIN*)&pEffect->szData;
	if (Object != NULL && Fountain->lpDOFEmitter == Object)
	{
		return FALSE; // end fountain
	}
	if (Object != NULL && Fountain->lpVelocityObj == Object)
	{
		return FALSE; // end fountain
	}

	return TRUE;
}

LOGICAL CFXEffects::callbackDetachSmokeTrail(EFFECT* pEffect, OBJECT * Object)
{
SMOKETRAIL * SmokeTrailData;

	if (pEffect == NULL)
	{
		return TRUE;
	}

	SmokeTrailData = (SMOKETRAIL*)&pEffect->szData;
	if (Object != NULL && SmokeTrailData->lpObjAttachedTo == Object)
	{
		return FALSE; // end smoke trail
	}

	return TRUE;
}

LOGICAL CFXEffects::CreateParticleFountain(UINT32 * EffectHandle,
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
FOUNTAIN* Fountain = NULL;
SINT32 iIndex = -1;

    if(m_pParticle == NULL)
        return FALSE;

    if(sizeof(FOUNTAIN) > EFFECT_DATA_SIZE)
    {
        Write_Debug("[CFXEffects::CreateSmokeTrail] FOUNTAIN data structure too large.");
        return FALSE;
    }

	if (EmitterDOF == NULL)
	{
		return FALSE;
	}

	if (EmitterDOF->dof == NULL || !(EmitterDOF->flags & OBJECT_DOF))
	{
		return FALSE;
	}

	if (VelocityObject == NULL)
	{
		return FALSE;
	}

    iIndex = GetFreeNode();

    if(iIndex == -1)
        return FALSE;

	*EffectHandle = iIndex + 1;

	Fountain = (FOUNTAIN*)&m_EffectArray[iIndex].szData;
	m_EffectArray[iIndex].pfnUpdateCallback = &CFXEffects::callbackUpdateParticleFountain;
	m_EffectArray[iIndex].pfnDetachCallback = &CFXEffects::callbackDetachParticleFountain;


	Fountain->EmitVelocity = EmitVelocity;
    Fountain->lpDOFEmitter = EmitterDOF;

	Fountain->lpVelocityObj = fxUtlGetObject(VelocityObject);

    if(Fountain->lpVelocityObj == NULL)
        return FALSE;

    if(EmitterDOF)
        EmitterDOF->flag2 |= OBJECT_HAS_FX;

    Fountain->lpVelocityObj->flag2 |= OBJECT_HAS_FX;

	Fountain->DragCo = fDragCo;
	Fountain->InitAlpha = fInitAlpha;
	Fountain->P1Alpha = fP1Alpha;
	Fountain->P2Alpha = fP2Alpha;
	Fountain->EndAlpha = fEndAlpha;
	Fountain->InitSize = fInitSize;
	Fountain->P1Size = fP1Size;
	Fountain->P2Size = fP2Size;
	Fountain->EndSize = fEndSize;
	Fountain->P1TimeOffset = iP1TimeOffset;
	Fountain->P2TimeOffset = iP2TimeOffset;
	Fountain->EndTimeOffset = iEndTimeOffset;
	Fountain->SpriteType = uObjectType;
	Fountain->MaxRotationRate = MaxRotationRate;
	Fountain->ParticleFlags = ParticleFlags;
	Fountain->ParticlesPerSec = ParticlesPerSec;
	Fountain->Variation = Variation;
	Fountain->Dispersion = Dispersion;

	Fountain->LastUpdateTime = frametime;
	Fountain->LastParticleReleaseTime = frametime;

	return TRUE;
}

LOGICAL CFXEffects::CreateSmokeTrail(UINT32 * EffectHandle,
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
    SMOKETRAIL* SmokeTrail = NULL;
    SINT32 iIndex = -1;
  //  SINT32 iDummyIndex = -1;
//    SINT32 i = 0;
    FLOAT64VECTOR fvecWorldOriginOffset;

    if(m_pParticle == NULL)
        return FALSE;

    if(sizeof(SMOKETRAIL) > EFFECT_DATA_SIZE)
    {
        Write_Debug("[CFXEffects::CreateSmokeTrail] SMOKETRAIL data structure too large.");
        return FALSE;
    }

	if (ObjectAttachedTo == NULL)
	{
		return FALSE;
	}

	if (Position == NULL)
	{
		return FALSE;
	}

    iIndex = GetFreeNode();

    if(iIndex == -1)
        return FALSE;

	*EffectHandle = iIndex + 1;

    SmokeTrail = (SMOKETRAIL*)&m_EffectArray[iIndex].szData;

    m_EffectArray[iIndex].pfnUpdateCallback = &CFXEffects::callbackSmokeTrail;
	m_EffectArray[iIndex].pfnDetachCallback = &CFXEffects::callbackDetachSmokeTrail;

	SmokeTrail->lpObjAttachedTo = fxUtlGetObject(ObjectAttachedTo);

    if(SmokeTrail->lpObjAttachedTo == NULL)
        return FALSE;

    SmokeTrail->lpObjAttachedTo->flag2 |= OBJECT_HAS_FX;

	SmokeTrail->OffsetPos = *Position;
	SmokeTrail->DragCo = fDragCo;
	SmokeTrail->InitAlpha = fInitAlpha;
	SmokeTrail->P1Alpha = fP1Alpha;
	SmokeTrail->P2Alpha = fP2Alpha;
	SmokeTrail->EndAlpha = fEndAlpha;
	SmokeTrail->InitSize = fInitSize;
	SmokeTrail->P1Size = fP1Size;
	SmokeTrail->P2Size = fP2Size;
	SmokeTrail->EndSize = fEndSize;
	SmokeTrail->P1TimeOffset = iP1TimeOffset;
	SmokeTrail->P2TimeOffset = iP2TimeOffset;
	SmokeTrail->EndTimeOffset = iEndTimeOffset;
	SmokeTrail->SpriteType = uObjectType;
	SmokeTrail->MaxRotationRate = MaxRotationRate;
	SmokeTrail->ParticleFlags = uFlags;
	SmokeTrail->Spacing = Spacing;
	SmokeTrail->Variation = Variation;
	SmokeTrail->Dispersion = Dispersion;
	SmokeTrail->MinParticleReleaseTime = (UINT32)(MinParticleReleaseTime * 1000.0f + 0.5f);

	SmokeTrail->LastUpdateTime = frametime;

	// compute current position
    trnGetWorldOriginOffset(&fvecWorldOriginOffset);

	SmokeTrail->LastPos.x = SmokeTrail->OffsetPos.x;
	SmokeTrail->LastPos.y = SmokeTrail->OffsetPos.y;
	SmokeTrail->LastPos.z = SmokeTrail->OffsetPos.z;

    mathVectorMultiply64(&SmokeTrail->LastPos,
                         &SmokeTrail->LastPos,
                         &SmokeTrail->lpObjAttachedTo->mat);

    SmokeTrail->LastPos.z = -SmokeTrail->LastPos.z;

    mathAddVectors(&SmokeTrail->LastPos, &SmokeTrail->LastPos, &fvecWorldOriginOffset);

	return TRUE;
}

LOGICAL CFXEffects::CreateSmokeTrail2(UINT32 * EffectHandle,
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
    SMOKETRAIL* SmokeTrail = NULL;
    SINT32 iIndex = -1;
 //   SINT32 iDummyIndex = -1;
//SINT32 i = 0;

    if(m_pParticle == NULL)
        return FALSE;

    if(sizeof(SMOKETRAIL) > EFFECT_DATA_SIZE)
    {
        Write_Debug("[CFXEffects::CreateSmokeTrail] SMOKETRAIL data structure too large.");
        return FALSE;
    }

	if (ammo == NULL)
	{
		return FALSE;
	}

    iIndex = GetFreeNode();

    if(iIndex == -1)
        return FALSE;

	*EffectHandle = iIndex + 1;

    SmokeTrail = (SMOKETRAIL*)&m_EffectArray[iIndex].szData;

    m_EffectArray[iIndex].pfnUpdateCallback = &CFXEffects::callbackSmokeTrail;
	m_EffectArray[iIndex].pfnDetachCallback = &CFXEffects::callbackDetachSmokeTrail;

	SmokeTrail->ammo = ammo;

	SmokeTrail->DragCo = fDragCo;
	SmokeTrail->InitAlpha = fInitAlpha;
	SmokeTrail->P1Alpha = fP1Alpha;
	SmokeTrail->P2Alpha = fP2Alpha;
	SmokeTrail->EndAlpha = fEndAlpha;
	SmokeTrail->InitSize = fInitSize;
	SmokeTrail->P1Size = fP1Size;
	SmokeTrail->P2Size = fP2Size;
	SmokeTrail->EndSize = fEndSize;
	SmokeTrail->P1TimeOffset = iP1TimeOffset;
	SmokeTrail->P2TimeOffset = iP2TimeOffset;
	SmokeTrail->EndTimeOffset = iEndTimeOffset;
	SmokeTrail->SpriteType = uObjectType;
	SmokeTrail->MaxRotationRate = MaxRotationRate;
	SmokeTrail->ParticleFlags = uFlags;
	SmokeTrail->Spacing = Spacing;
	SmokeTrail->Variation = Variation;
	SmokeTrail->Dispersion = Dispersion;
	SmokeTrail->MinParticleReleaseTime = (UINT32)(MinParticleReleaseTime * 1000.0f + 0.5f);

	SmokeTrail->LastUpdateTime = frametime;

    FLOAT64 x, y, z;

    x = ammo->Position.x - trnGetWorldOriginOffsetPtr()->x;
    y = ammo->Position.y - trnGetWorldOriginOffsetPtr()->y;
    z = ammo->Position.z - trnGetWorldOriginOffsetPtr()->z;

	SmokeTrail->LastPos.x = x;
	SmokeTrail->LastPos.y = y;
	SmokeTrail->LastPos.z = z;

	return TRUE;
}

LOGICAL CFXEffects::DetachFXFromObject(OBJECT* Object)
{
    EFFECT* pNode = m_effectHead.next;
    EFFECT* pFree = NULL;

    if(pNode == NULL)
        return FALSE;

    while(pNode != &m_effectTail)
    {
        if(pNode->pfnDetachCallback == NULL)
        {
            pNode = pNode->next;
            continue;
        }

        if(!callMemberFunction(*this, pNode->pfnDetachCallback)(pNode, Object))
        {
            pFree = pNode;
            pNode = pNode->next;

            if(pFree->pfnKillCallback != NULL)
            {
                pFree->pfnKillCallback(pFree);
            }

            if(pFree->pfnOutsideCallback != NULL)
            {
                pFree->pfnOutsideCallback(pFree->uShutdownData);
            }

            AddToFreeList(pFree);
            continue;
        }

        pNode = pNode->next;
    }

    DetachTexturePockObject(Object);

	return TRUE;
}

LOGICAL CFXEffects::DoExplosiveEffect(const FLOAT64VECTOR* Pos,
								 FLOAT64 Energy)
{
SINT32 NumParticles;
piVECTOR Vel;
FLOAT32 TempNum,TempNum1;
FLOAT32 Speed;
piVECTOR RotationRate;
SINT32 i;
UINT32 Duration;
FLOAT32 Size;
SINT32 Sprite1,Sprite2,Sprite3;

	Sprite1 = 29; // fireball
	Sprite2 = 28; // flash
	Sprite3 = 11; // smoke

	if(Energy == 0) return TRUE;

	NumParticles = (UINT32)(log(Energy) / 2.0 + 0.5) - 1;
	if (NumParticles <= 0)
	{
		NumParticles = 1;
	}

	Speed = (FLOAT32)pow(Energy,0.25) * 0.25f;

	Duration = (UINT32)(25.0 * log(Energy));

	Size = (FLOAT32)(pow(Energy,0.25)) * 0.0625f;
	Size *= 4.0f;

	if (Sprite2 > 0)
	{
		TempNum = mathGetFloat32ChaoticNumber() * 0.5f + 0.5f;
		if (TempNum * Size > 1.0f)
		{
			TempNum1 = 1.0f;
		}
		else
		{
			TempNum1 = TempNum * Size;
		}
		m_pParticle->CreateNewParticle(Pos,
									   NULL,
									   0.2f,
									   NULL,
									   TempNum1,
									   TempNum1,
									   TempNum1,
									   TempNum1,
									   Size * TempNum * 2.0f,
									   Size, //Size * 0.5f * TempNum,
									   Size, //Size * 0.8666f * TempNum,
									   Size, //Size * 1.0f * TempNum,
									   frametime,
									   frametime + 25 + Duration / 16,
									   frametime + 75 + Duration / 8,
									   frametime + 125 + Duration / 4,
									   Sprite2,//28,
									   NULL,
									   PARTICLE_FLAG_GLOWS |
										 PARTICLE_FLAG_AT_LEAST_1_PIXEL |
										 PARTICLE_FLAG_DISPLAY_FOR_ONE_FRAME,
									   NULL);
	}

	for (i=0;i<NumParticles;i++)
	{
		if (Sprite1 > 0)
		{
			Vel.x = Speed * 1.2f * 0.2f * (mathGetFloat32ChaoticNumber() * 2.0f - 1.0f);
			Vel.y = Speed * 1.2f * 0.2f * (mathGetFloat32ChaoticNumber() * 2.0f - 1.0f);
			Vel.z = Speed * 1.2f * 1.0f * (mathGetFloat32ChaoticNumber());

			RotationRate.x = mathGetFloat32ChaoticNumber() * 2.0f * PI;
			RotationRate.y = mathGetFloat32ChaoticNumber() * 1.0f * 2.0f -
								1.0f;
			RotationRate.z = 0.0f;

			TempNum = mathGetFloat32ChaoticNumber() * 0.5f + 0.5f;
			m_pParticle->CreateNewParticle(Pos,
										   &Vel,
										   1.2f,
										   NULL,
										   0.75f,
										   0.55f,
										   0.35f,
										   0.0f,
										   0.0f,
										   Size * 1.5f * 0.5f * TempNum,
										   Size * 1.5f * 0.866f * TempNum,
										   Size * 1.5f * 1.0f * TempNum,
										   frametime,
										   frametime + (UINT32)(10 * Speed * 0.4),
										   frametime + (UINT32)(40 * Speed * 0.4),
										   frametime + (UINT32)(160 * Speed * 0.4),
										   Sprite1,
										   &RotationRate,
										   PARTICLE_FLAG_USESGRAVITY | PARTICLE_FLAG_USESDRAG |
												PARTICLE_FLAG_DISPLAY_AFTER_ONE_FRAME |
												PARTICLE_FLAG_AT_LEAST_1_PIXEL |
												PARTICLE_FLAG_RISES,
										   NULL);

		}

		if (Sprite3 > 0)
		{

			Vel.x = Speed * 0.3f * (mathGetFloat32ChaoticNumber() * 2.0f - 1.0f);
			Vel.y = Speed * 0.3f * (mathGetFloat32ChaoticNumber() * 2.0f - 1.0f);
			Vel.z = Speed * 0.3f * 0.1f * (mathGetFloat32ChaoticNumber() * 2.0f - 1.0f);
			TempNum = mathGetFloat32ChaoticNumber() * 0.5f + 0.5f;
			if (TempNum * Size > 1.0f)
			{
				TempNum1 = 1.0f;
			}
			else
			{
				TempNum1 = TempNum * Size;
			}
			m_pParticle->CreateNewParticle(Pos,
										   &Vel,
										   1.0f,
										   NULL,
										   0.35f * TempNum1,
										   0.35f * TempNum1,
										   0.15f * TempNum1,
										   0.0f * TempNum1,
										   0.0f,
										   Size * 2.0f * 0.5f * TempNum,
										   Size * 2.0f * 0.866f * TempNum,
										   Size * 2.0f * 1.0f * TempNum,
										   frametime,
										   frametime + (UINT32)(25 * Speed * 0.1),
										   frametime + (UINT32)(100 * Speed * 0.1),
										   frametime + (UINT32)(750 * sqrt(Speed) * 13.3 * 0.1),
										   Sprite3,
										   &RotationRate,
										   PARTICLE_FLAG_USESDRAG | PARTICLE_FLAG_DISPLAY_AFTER_ONE_FRAME |
												PARTICLE_FLAG_AT_LEAST_1_PIXEL | PARTICLE_FLAG_USESDRAG |
												PARTICLE_FLAG_RISES,
										   NULL);
		}
	}

	return TRUE;
}

LOGICAL CFXEffects::DoVehicleHitEffect(FLOAT64VECTOR* Pos,
								 FLOAT64 Energy,
								 piVECTOR const* Norm,
								 piVECTOR* InVel,
								 OBJECT * CollideObject)
{
FLOAT32 Size;
UINT32 Num;
UINT32 Duration;
UINT32 TimeOffset;
piVECTOR Vel;
piVECTOR RotationRate;
FLOAT32 DragCo;
UINT32 i;


	Size = (FLOAT32)(pow(Energy,0.3333333) / 25.0);

	Num = (UINT32)(pow(Energy,0.25) / 4.0);

	Duration = (UINT32)(25.0 * log(Energy)) + 200;

	RotationRate.x = mathGetFloat32ChaoticNumber() * 2.0f * PI;
	RotationRate.y = mathGetFloat32ChaoticNumber() * 10.0f * 2.0f -
						1.0f;
	RotationRate.z = 0.0f;

	TimeOffset = (UINT32)(mathGetFloat32ChaoticNumber() * Duration * 0.3);

	// do flash
	m_pParticle->CreateNewParticle(Pos,
								   InVel,
								   0.03f,
								   NULL,
								   1.0f,
								   1.0f,
								   1.0f,
								   0.0f,
								   Size * 0.0f,
								   Size * 0.3f,
								   Size * 1.0f,
								   Size * 1.25f,
								   frametime + TimeOffset,
								   frametime + TimeOffset + Duration / 4,
								   frametime + TimeOffset + Duration / 2,
								   frametime + TimeOffset + Duration,
								   30,
								   &RotationRate,
								   PARTICLE_FLAG_GLOWS | PARTICLE_FLAG_AT_LEAST_1_PIXEL,//PARTICLE_FLAG_USESDRAG,
								   CollideObject);

	for (i=0;i<Num;i++)
	{
		Vel = *InVel;

		Vel.x += Size * 0.2f * 0.2f * (mathGetFloat32ChaoticNumber() * 2.0f - 1.0f);
		Vel.y += Size * 0.2f * 0.2f * (mathGetFloat32ChaoticNumber() * 2.0f - 1.0f);
		Vel.z += Size * 0.2f * 0.2f * (mathGetFloat32ChaoticNumber() * 2.0f - 1.0f);

		DragCo = (mathGetFloat32ChaoticNumber() * 2.0f - 1.0f) * 0.5f + 0.7f;

		RotationRate.x = mathGetFloat32ChaoticNumber() * 2.0f * PI;
		RotationRate.y = mathGetFloat32ChaoticNumber() * 50.0f * 2.0f -
							25.0f;
		RotationRate.z = 0.0f;

		TimeOffset = (UINT32)(mathGetFloat32ChaoticNumber() * Duration * 0.3);

		if (mathGetFloat32ChaoticNumber() > 0.5)
		{
			m_pParticle->CreateNewParticle(Pos,
										   &Vel,
										   DragCo,
										   NULL,
										   0.15f,
										   0.10f,
										   0.05f,
										   0.0f,
										   Size * 2.0f * 0.5f,
										   Size * 2.0f * 0.866f,
										   Size * 2.0f * 1.0f,
										   Size * 2.0f * 1.0f,
										   frametime + TimeOffset + 100,
										   frametime + TimeOffset + Duration * 2 / 2 + 100,
										   frametime + TimeOffset + Duration * 2 + 100,
										   frametime + TimeOffset + Duration * 2 * 2 + 100,
										   12,
										   &RotationRate,
										   PARTICLE_FLAG_USESDRAG | PARTICLE_FLAG_AT_LEAST_1_PIXEL,
										   CollideObject);
		}
		else
		{
			m_pParticle->CreateNewParticle(Pos,
										   &Vel,
										   0.01f,
										   NULL,
										   1.0f,
										   0.25f,
										   0.25f,
										   0.0f,
										   0.0f,
										   Size * 3.0f * 0.25f,
										   Size * 3.0f * 0.5f,
										   Size * 3.0f * 1.0f,
										   frametime + TimeOffset + 100,
										   frametime + TimeOffset + Duration / 4 + 100,
										   frametime + TimeOffset + Duration / 2 + 100,
										   frametime + TimeOffset + Duration + 100,
										   31,
										   &RotationRate,
										   PARTICLE_FLAG_USESDRAG | PARTICLE_FLAG_GLOWS | PARTICLE_FLAG_AT_LEAST_1_PIXEL,
										   CollideObject);
		}


	}

	return TRUE;
}

LOGICAL CFXEffects::DoHitSound(FLOAT64VECTOR * Pos, FLOAT32 Energy,
								const SINT8 * TextureName)
{
SINT8 * HitSoundName;
FLOAT32 MinVolume;
FLOAT32 MaxVolumeEnergy;
FLOAT32 Volume;

	HitSoundName = NULL;
	mlGetRandomHitSound(&HitSoundName,
						&MinVolume,
						&MaxVolumeEnergy,
						TextureName);

	if (HitSoundName == NULL)
	{
		return 0;
	}

	if (Energy >= MaxVolumeEnergy)
	{
		Volume = 1.0f;
	}
	else
	{
		if (MaxVolumeEnergy == 0.0f)
		{
			Volume = 1.0f;
		}
		else
		{
			Volume = (1.0f - MinVolume) * Energy / MaxVolumeEnergy + MinVolume;
		}
	}

	if(Volume >= 0.01f)
	{
        SINT32 iID = fxSndGetNewSound();

        if(iID > 0)
        {
            fxSndSetSoundName(iID, HitSoundName);
            fxSndSetPosition(iID, Pos->x, Pos->y, Pos->z);
            fxSndSetPriority(iID, IDE_TP_PRIORITY_LOW);
            fxSndSetSoundType(iID, TYPE_FINITE_EXTERNAL_SOUND);
            fxSndSetVolume(iID, Volume);
            fxSndSetPitch(iID, 0.8f + 0.2f * (mathGetFloat32ChaoticNumber() * 2.0f - 1.0f));
            fxSndSetPlayType(iID, DEF_PLAYTYPE_WORLD);
            fxSndSetMaxDistance(iID, 50.0f);
            fxSndSetMinDistance(iID, 10.0f);

            fxSndInitSound(iID);
        }
    }

    return TRUE;
}


LOGICAL CFXEffects::DoHitEffect(FLOAT64VECTOR const*Pos,
								 FLOAT64 InEnergy,
								 piVECTOR const* Norm,
								 OBJECT * CollideObject,
								SINT32 PolyNum,
								const SINT8 * TexName)
{
SINT32 NumParticles;
piVECTOR BaseVel;
piVECTOR Vel;
piVECTOR TempVec;
FLOAT32 TempNum,TempNum1;
FLOAT32 Speed;
piMATRIX Mat;
piVECTOR RotationRate;
SINT32 i;
UINT32 Duration;
FLOAT32 Size;
FLOAT64 Energy;
SINT32 Sprite1,Sprite2,Sprite3;

	if (CollideObject == NULL)
	{
		return FALSE;
	}

	if (InEnergy < 200.0)
	{
		Energy = 200.0;
	}
	else
	{
		Energy = InEnergy;
	}

    memset(&RotationRate, 0, sizeof(piVECTOR));

	if (TexName == NULL || strlen(TexName) == 0)
	{
		mlGetHitSpriteNumbers(&Sprite1,&Sprite2,&Sprite3,"default");
	}
	else
	{
		mlGetHitSpriteNumbers(&Sprite1,&Sprite2,&Sprite3,TexName);
	}

	NumParticles = (UINT32)(log(Energy) / 2.0 + 0.5) - 1;
	if (NumParticles <= 0)
	{
		NumParticles = 1;
	}

	Speed = (FLOAT32)pow(Energy,0.33) * 0.25f;

	if (Norm == NULL)
	{
		BaseVel.x = 0.0f;
		BaseVel.y = 0.0f;
		BaseVel.z = 1.0f;
	}
	else
	{
		BaseVel = *Norm;
	}

	Mat = CollideObject->mat;
	Mat.m[3][0]/*._41*/ = Mat.m[3][1]/*._42*/ = Mat.m[3][2]/*._43*/ = 0;

	math3dVectorMultiply(&BaseVel,&BaseVel,&Mat);

	BaseVel.x *= Speed * 0.1f;
	BaseVel.y *= Speed * 0.1f;
	BaseVel.z *= -Speed * 0.05f;

	Duration = (UINT32)(25.0 * log(Energy));

	Size = (FLOAT32)(pow(Energy,0.33)) * 0.0625f;

	if (Sprite1 > 0)
	{
		TempNum = mathGetFloat32ChaoticNumber() * 0.5f + 0.5f;
		if (TempNum * Size > 1.0f)
		{
			TempNum1 = 1.0f;
		}
		else
		{
			TempNum1 = TempNum * Size;
		}
		m_pParticle->CreateNewParticle(Pos,
									   NULL,
									   0.2f,
									   NULL,
									   TempNum1,
									   TempNum1,
									   TempNum1,
									   TempNum1,
									   Size * TempNum,
									   Size, //Size * 0.5f * TempNum,
									   Size, //Size * 0.8666f * TempNum,
									   Size, //Size * 1.0f * TempNum,
									   frametime,
									   frametime + 25 + Duration / 16,
									   frametime + 75 + Duration / 8,
									   frametime + 125 + Duration / 4,
									   Sprite1 - 1,//28,
									   NULL,
									   PARTICLE_FLAG_GLOWS |
										 PARTICLE_FLAG_AT_LEAST_1_PIXEL |
										 PARTICLE_FLAG_DISPLAY_FOR_ONE_FRAME,
									   NULL);
	}

	for (i=0;i<NumParticles;i++)
	{
		if (Sprite2 > 0)
		{
			Vel = BaseVel;

			Vel.x += Speed * 0.4f * 0.2f * (mathGetFloat32ChaoticNumber() * 2.0f - 1.0f);
			Vel.y += Speed * 0.4f * 0.2f * (mathGetFloat32ChaoticNumber() * 2.0f - 1.0f);
			Vel.z += Speed * 0.4f * 1.0f * (mathGetFloat32ChaoticNumber() * 2.0f - 1.0f);

			RotationRate.x = mathGetFloat32ChaoticNumber() * 2.0f * PI;
			RotationRate.y = mathGetFloat32ChaoticNumber() * 1.0f * 2.0f -
								1.0f;
			RotationRate.y *= 3.0f / Size;
			RotationRate.z = 0.0f;

			TempNum = mathGetFloat32ChaoticNumber() * 0.5f + 0.5f;
			m_pParticle->CreateNewParticle(Pos,
										   &Vel,
										   3.0f,
										   NULL,
										   0.75f,
										   0.55f,
										   0.35f,
										   0.0f,
										   0.0f,
										   Size * 1.5f * 0.5f * TempNum,
										   Size * 1.5f * 0.866f * TempNum,
										   Size * 1.5f * 1.0f * TempNum,
										   frametime,
										   frametime + (UINT32)(25 * Speed * 0.4),
										   frametime + (UINT32)(100 * Speed * 0.4),
										   frametime + (UINT32)(450 * Speed * 0.4),
										   Sprite2 - 1,
										   &RotationRate,
										   PARTICLE_FLAG_USESGRAVITY | PARTICLE_FLAG_USESDRAG |
												PARTICLE_FLAG_USE_ALPHAREF |
												PARTICLE_FLAG_DISPLAY_AFTER_ONE_FRAME |
												PARTICLE_FLAG_AT_LEAST_1_PIXEL |
												PARTICLE_FLAG_DONT_GO_BELOW_START_ALT,
										   NULL);

			Vel = BaseVel;

			Vel.x += Speed * 0.4f * 0.2f * (mathGetFloat32ChaoticNumber() * 2.0f - 1.0f);
			Vel.y += Speed * 0.4f * 0.2f * (mathGetFloat32ChaoticNumber() * 2.0f - 1.0f);
			Vel.z += Speed * 0.4f * 1.0f * (mathGetFloat32ChaoticNumber() * 2.0f - 1.0f);

			if (Norm != NULL)
			{
				TempVec = CollideObject->psnorms[PolyNum];
			}
			else
			{
				TempVec.x = TempVec.y = 0.0f;
				TempVec.z = 1.0f;
			}
			Mat = CollideObject->mat;
			Mat.m[3][0]/*_41*/ = Mat.m[3][1]/*_42*/ = Mat.m[3][2]/*_43*/ = 0.0f;
			math3dVectorMultiply(&TempVec,&TempVec,&Mat);
			TempVec.z = -TempVec.z;
			TempNum = 40.0f * (FLOAT32)(pow(Energy,0.25) * 0.02) * mathGetFloat32ChaoticNumber();
			Vel.x += TempNum * TempVec.x;
			Vel.y += TempNum * TempVec.y;
			Vel.z += TempNum * TempVec.z;

			RotationRate.x = mathGetFloat32ChaoticNumber() * 2.0f * PI;
			RotationRate.y = mathGetFloat32ChaoticNumber() * 1.0f * 2.0f -
								1.0f;
			RotationRate.y *= 3.0f / Size;
			RotationRate.z = 0.0f;

			TempNum = mathGetFloat32ChaoticNumber() * 0.5f + 0.5f;

			m_pParticle->CreateNewParticle(Pos,
										   &Vel,
										   3.0f,
										   NULL,
										   0.0f,
										   0.5f,
										   0.5f,
										   0.0f,
										   0.0f,
										   Size * 0.75f * 0.5f * TempNum,
										   Size * 0.75f * 0.866f * TempNum,
										   Size * 0.75f * 1.0f * TempNum,
										   frametime,
										   frametime + (UINT32)(25 * Speed * 0.4),
										   frametime + (UINT32)(100 * Speed * 0.4),
										   frametime + (UINT32)(850 * Speed * 0.4),
										   Sprite2 - 1,
										   &RotationRate,
										   PARTICLE_FLAG_USESGRAVITY | PARTICLE_FLAG_USESDRAG |
												PARTICLE_FLAG_USE_ALPHAREF |
												PARTICLE_FLAG_DISPLAY_AFTER_ONE_FRAME |
												PARTICLE_FLAG_AT_LEAST_1_PIXEL |
												PARTICLE_FLAG_DONT_GO_BELOW_START_ALT,
										   NULL);
		}

		if (Sprite3 > 0)
		{
			Vel = BaseVel;

			Vel.x += Speed * 0.1f * (mathGetFloat32ChaoticNumber() * 2.0f - 1.0f);
			Vel.y += Speed * 0.1f * (mathGetFloat32ChaoticNumber() * 2.0f - 1.0f);
			Vel.z += Speed * 0.1f * 0.1f * (mathGetFloat32ChaoticNumber() * 2.0f - 1.0f);
			TempNum = mathGetFloat32ChaoticNumber() * 0.5f + 0.5f;
			if (TempNum * Size > 1.0f)
			{
				TempNum1 = 1.0f;
			}
			else
			{
				TempNum1 = TempNum * Size;
			}
			m_pParticle->CreateNewParticle(Pos,
										   &Vel,
										   0.5f,
										   NULL,
										   0.35f * TempNum1,
										   0.35f * TempNum1,
										   0.15f * TempNum1,
										   0.0f * TempNum1,
										   0.0f,
										   Size * 2.0f * 0.5f * TempNum,
										   Size * 2.0f * 0.866f * TempNum,
										   Size * 2.0f * 1.0f * TempNum,
										   frametime,
										   frametime + (UINT32)(25 * Speed * 0.1),
										   frametime + (UINT32)(100 * Speed * 0.1),
										   frametime + (UINT32)(750 * sqrt(Speed) * 13.3 * 0.1),
										   Sprite3 - 1,
										   &RotationRate,
										   PARTICLE_FLAG_USESDRAG | PARTICLE_FLAG_DISPLAY_AFTER_ONE_FRAME |
												PARTICLE_FLAG_AT_LEAST_1_PIXEL,
										   NULL);
		}
	}

	return TRUE;
}

LOGICAL CFXEffects::CreateExplosion(FLOAT64VECTOR* pfvecPosition)
{
    EXPLOSION* pExplosion = NULL;
    SINT32 iIndex = -1;
//    SINT32 iDummyIndex = -1;
    SINT32 i = 0;
    piVECTOR PIvelocity;

    if(m_pParticle == NULL)
        return FALSE;

    if(sizeof(EXPLOSION) > EFFECT_DATA_SIZE)
    {
        Write_Debug("[CFXEffects::CreateExplosion] Explosion data structure too large.");
        return FALSE;
    }

    SINT32 iID = fxSndGetNewSound();

    if(iID > 0)
    {
        fxSndSetSoundName(iID, "sounds\\explosion.wav");
        fxSndSetPitch(iID, 1.0f);
        fxSndSetVolume(iID, 1.0f);
        fxSndSetPriority(iID, IDE_PRIORITY_NOT_USED);
        fxSndSetSoundType(iID, TYPE_FINITE_EXTERNAL_SOUND);
        fxSndSetPosition(iID, pfvecPosition->x, pfvecPosition->y, pfvecPosition->z);
        fxSndSetPlayType(iID, DEF_PLAYTYPE_WORLD);

        fxSndInitSound(iID);
    }

    iIndex = GetFreeNode();

    if(iIndex == -1)
        return FALSE;

    pExplosion = (EXPLOSION*)&m_EffectArray[iIndex].szData;

    pExplosion->fvecPosition = *pfvecPosition;
    pExplosion->iStartFrameTime = frametime;

    m_pParticle->CreateNewParticle(pfvecPosition,
                                   NULL,
                                   0.0f,
                                   NULL,
                                   1.0f,
                                   1.0f,
                                   0.5f,
                                   0.0f,
                                   0.0f,
                                   10.0f,
                                   15.0f,
                                   20.0f,
                                   frametime,
                                   frametime + 5000,
                                   frametime + 20000,
                                   frametime + 40000,
                                   16,
                                   NULL,
                                   0,
								   NULL);

    for(i = 0; i < 10; i++)
    {
            PIvelocity.x = (2 * (mathGetChaoticNumber()) / (FLOAT32)(32768) - 1) * 3;
            PIvelocity.y = (2 * (mathGetChaoticNumber()) / (FLOAT32)(32768) - 1) * 3;
            PIvelocity.z = ((UINT16)mathGetChaoticNumber()) / (FLOAT32)(65536) * 30;


        m_pParticle->CreateNewParticle(pfvecPosition,
                                       &PIvelocity,
                                       0.2f,               // Drag Coefficient
                                       NULL,
                                       0.0f,
                                       0.0f,
                                       0.5f,
                                       0.0f,
                                       0.0f,
                                       6.0f,
                                       12.0f,
                                       20.0f,
                                       frametime,
                                       frametime,
                                       frametime + 500,
                                       frametime + 10000,
                                       12,
                                       NULL,
                                       PARTICLE_FLAG_USESDRAG,
									   NULL);

    }
//    m_EffectArray[iIndex].pfnUpdateCallback = callbackExplosion;

    return TRUE;
}

LOGICAL CFXEffects::callbackUpdateParticleFountain(EFFECT* pEffect)
{
FOUNTAIN * Fountain;
FLOAT64VECTOR fvecWorldOriginOffset;
FLOAT64VECTOR CurrentPos;
FLOAT64VECTOR Temp64Vec;
piVECTOR CurrentVel;
piVECTOR TempVec;
piVECTOR RotationRate;
piVECTOR Dispersion;
SINT32 DeltaTime;
FLOAT32 fDeltaTime;
FLOAT32 Variable;
FLOAT32 MaxSize;
FLOAT32 Dist;
FLOAT32 OnePixelRatio;
UINT32 NumParticles;
piMATRIX TempMat;
SINT32 TempNum;
OBJECT * Object;
UINT32 i;

	if (pEffect == NULL)
	{
		return FALSE;
	}

	Fountain = (FOUNTAIN*)&pEffect->szData;

	// compute current position
    trnGetWorldOriginOffset(&fvecWorldOriginOffset);

	if (Fountain->LastParticleReleaseTime == 0)
	{
		Fountain->LastParticleReleaseTime = frametime;
	}

	DeltaTime = frametime - Fountain->LastParticleReleaseTime;

	// determine number of particles
	NumParticles = (SINT32)(DeltaTime * Fountain->ParticlesPerSec / 1000.0f);
	if (NumParticles > FOUNTAIN_MAX_NUM_PARTICLES_PER_UPDATE)
	{
		NumParticles = 0;
		Fountain->LastParticleReleaseTime = frametime;
		Fountain->LastUpdateTime = frametime;
		return TRUE;
	}

	if (NumParticles <= 0)
	{
		Fountain->LastUpdateTime = frametime;
		return TRUE;
	}

	Object = Fountain->lpVelocityObj;

	// calculate currentpos
	CurrentPos.x = Fountain->lpDOFEmitter->dof->dofmatrix2.m[3][0];//_41;
	CurrentPos.y = Fountain->lpDOFEmitter->dof->dofmatrix2.m[3][1];//_42;
	CurrentPos.z = Fountain->lpDOFEmitter->dof->dofmatrix2.m[3][2];//_43;

    mathVectorMultiply64(&CurrentPos,
                         &CurrentPos,
                         &Fountain->lpDOFEmitter->mat);

    mathAddVectors(&CurrentPos, &CurrentPos, &fvecWorldOriginOffset);
	CurrentPos.z = -CurrentPos.z;

	// sanity check for distance
	if (Fountain->InitSize > Fountain->P1Size)
	{
		MaxSize = Fountain->InitSize;
	}
	else
	{
		MaxSize = Fountain->P1Size;
	}
	if (Fountain->P2Size > MaxSize)
	{
		MaxSize = Fountain->P2Size;
	}
	if (Fountain->EndSize > MaxSize)
	{
		MaxSize = Fountain->EndSize;
	}

	if (!(Fountain->ParticleFlags & PARTICLE_FLAG_AT_LEAST_1_PIXEL))
	{
		mathSubVectors(&Temp64Vec,&fvecWorldOriginOffset,&playpos);
		Temp64Vec.z = -Temp64Vec.z;
		mathSubVectors(&Temp64Vec,&CurrentPos,&Temp64Vec);
		Dist = (FLOAT32)mathExactVectorLength(&Temp64Vec);
		OnePixelRatio = FieldOfView * PI / 180.0f / piGetWidth() * 0.25f;
		if (MaxSize < OnePixelRatio * Dist)
		{
			// smoke particles would be too small to see, so don't
			// generate smoke
			Fountain->LastUpdateTime = frametime;
			Fountain->LastParticleReleaseTime = frametime;
			return TRUE;
		}
	}

	for (i=0;i<NumParticles;i++)
	{
		// calculate velocity vector
		TempVec.x = TempVec.z = 0.0f;
		TempVec.y = Fountain->EmitVelocity;

		// calculate dispersion ( 80% of rounds will be within the dispersion)
		Dispersion.x = mathGetFloat32ChaoticNumber() * mathGetFloat32ChaoticNumber();
		Dispersion.x *= (FLOAT32)(1.6 * Fountain->Dispersion);
		Dispersion.y = (mathGetFloat32ChaoticNumber() * 2.0f - 1.0f) * PI;
		Dispersion.x = (FLOAT32)mlDEG_TO_RAD(Dispersion.x);
		Dispersion.z = 0;
		mathEul_ToMatrix(&TempMat,EulOrdXYZs,&Dispersion);
		math3dVectorMultiply(&TempVec,&TempVec,&TempMat);

		// calculate final velocity
		TempMat = Fountain->lpDOFEmitter->dof->dofmatrix2;
		TempMat.m[3][0]/*_41*/ = TempMat.m[3][1]/*_42*/ = TempMat.m[3][2]/*_43*/ = 0.0f;
		math3dVectorMultiply(&CurrentVel,&TempVec,&TempMat);
		TempMat = Object->mat;
		TempMat.m[3][0]/*_41*/ = TempMat.m[3][1]/*_42*/ = TempMat.m[3][2]/*_43*/ = 0.0f;
		math3dVectorMultiply(&CurrentVel,&CurrentVel,&TempMat);
		CurrentVel.z = -CurrentVel.z;
		mathAddVectors(&CurrentVel,&CurrentVel,&Object->vel);

		// rotation
		RotationRate.x = mathGetFloat32ChaoticNumber() * 2.0f * PI;
		RotationRate.y = mathGetFloat32ChaoticNumber() * Fountain->MaxRotationRate * 2.0f -
							Fountain->MaxRotationRate;
		RotationRate.z = 0.0f;

		TempNum = Fountain->LastParticleReleaseTime;

		fDeltaTime = (frametime - TempNum) / 1000.0f;
		if (Fountain->Variation > 0.0001)
		{
			Variable = 1.0f + (mathGetFloat32ChaoticNumber() * 2.0f - 1.0f) * Fountain->Variation;
			TempVec.x = CurrentVel.x * Variable;
			TempVec.y = CurrentVel.y * Variable;
			TempVec.z = CurrentVel.z * Variable;

			Temp64Vec.x = CurrentPos.x - fDeltaTime * TempVec.x;
			Temp64Vec.y = CurrentPos.y - fDeltaTime * TempVec.y;
			Temp64Vec.z = CurrentPos.z - fDeltaTime * TempVec.z;

			Variable = 1.0f + (mathGetFloat32ChaoticNumber() * 2.0f - 1.0f) * Fountain->Variation;

			m_pParticle->CreateNewParticle(&Temp64Vec,
											&TempVec,
											Fountain->DragCo,
											NULL,
											Fountain->InitAlpha,
											Fountain->P1Alpha,
											Fountain->P2Alpha,
											Fountain->EndAlpha,
											Fountain->InitSize,
											Fountain->P1Size,
											Fountain->P2Size,
											Fountain->EndSize,
											TempNum,
											TempNum + (SINT32)(Fountain->P1TimeOffset * Variable + 0.5f),
											TempNum + (SINT32)(Fountain->P2TimeOffset * Variable + 0.5f),
											TempNum + (SINT32)(Fountain->EndTimeOffset * Variable + 0.5f),
											Fountain->SpriteType,
											&RotationRate,
											Fountain->ParticleFlags,
											NULL);
		}
		else
		{
			Temp64Vec.x = CurrentPos.x - fDeltaTime * CurrentVel.x;
			Temp64Vec.y = CurrentPos.y - fDeltaTime * CurrentVel.y;
			Temp64Vec.z = CurrentPos.z - fDeltaTime * CurrentVel.z;

			m_pParticle->CreateNewParticle(&Temp64Vec,
											&CurrentVel,
											Fountain->DragCo,
											NULL,
											Fountain->InitAlpha,
											Fountain->P1Alpha,
											Fountain->P2Alpha,
											Fountain->EndAlpha,
											Fountain->InitSize,
											Fountain->P1Size,
											Fountain->P2Size,
											Fountain->EndSize,
											TempNum,
											TempNum + Fountain->P1TimeOffset,
											TempNum + Fountain->P2TimeOffset,
											TempNum + Fountain->EndTimeOffset,
											Fountain->SpriteType,
											&RotationRate,
											Fountain->ParticleFlags,
											NULL);
		}

		Fountain->LastParticleReleaseTime += (SINT32)(1000.0f / Fountain->ParticlesPerSec);
	}

	Fountain->LastUpdateTime = frametime;

	return TRUE;
}

LOGICAL CFXEffects::callbackSmokeTrail(EFFECT* pEffect)
{
    SMOKETRAIL* SmokeTrail;

    FLOAT64VECTOR CurrentPos;
    FLOAT64VECTOR UnitDeltaVec;
    FLOAT64VECTOR DeltaVec;
    FLOAT64VECTOR TempPos;

    FLOAT64 Length;
    FLOAT64 UseSpacing;
    FLOAT64 FloatCurrentTime;

    SINT32 NumParticles;
    SINT32 i;
    SINT32 CurrentTime;

    FLOAT32 TimeStep = 0.0f;
    FLOAT32 DeltaTime;
    FLOAT32 Variable;

    piVECTOR RotationRate;
    piVECTOR Dispersion;
    piVECTOR Stretch;

	if (pEffect == NULL)
	{
		return FALSE;
	}

    SmokeTrail = (SMOKETRAIL*)&pEffect->szData;

	DeltaTime = (frametime - SmokeTrail->LastUpdateTime) / 1000.0f;

    if(!GetCurrentPosition(SmokeTrail, &CurrentPos))
        return FALSE;

    if(ParticleTooSmallToSee(SmokeTrail, &CurrentPos))
        return FALSE;

	mathSubVectors(&DeltaVec,&CurrentPos,&SmokeTrail->LastPos);

	CurrentTime = (SINT32)SmokeTrail->LastUpdateTime;
	FloatCurrentTime = (FLOAT64)CurrentTime;

	Length = mathExactVectorLength(&DeltaVec);

	if (Length < 0.01)
	{
		Length = 1.0f;
        memset(&DeltaVec, 0, sizeof(FLOAT64VECTOR));
	}

	if (SmokeTrail->Spacing == 0.0)
	{
		Write_Debug("Smoke trail with spacing of zero detected");
		return FALSE;
	}

	NumParticles = (UINT32)(Length / SmokeTrail->Spacing) - 2;

	if (NumParticles == -2 || NumParticles == -1)
	{
		NumParticles = 0;
	}

	if (NumParticles > SMOKE_TRAIL_MAX_NUM_PARTICLES_PER_UPDATE)
	{
		SmokeTrail->LastPos = CurrentPos;
		SmokeTrail->LastUpdateTime = frametime;

		return TRUE;
	}
	else
	{
		UseSpacing = SmokeTrail->Spacing;
	}

	if (NumParticles == 0 && SmokeTrail->MinParticleReleaseTime > 1 &&
				frametime - SmokeTrail->LastParticleReleaseTime > SmokeTrail->MinParticleReleaseTime &&
				Length < SmokeTrail->Spacing * 3.0f)
	{
		if (SmokeTrail->LastSpecialDropTime == 0)
		{
			SmokeTrail->LastSpecialDropTime = frametime;
		}
		if (frametime - SmokeTrail->LastSpecialDropTime > (SINT32)SmokeTrail->MinParticleReleaseTime)
		{
			RotationRate.x = mathGetFloat32ChaoticNumber() * 2.0f * PI;
			RotationRate.y = mathGetFloat32ChaoticNumber() * SmokeTrail->MaxRotationRate * 2.0f -
								SmokeTrail->MaxRotationRate;
			RotationRate.z = 0.0f;

			m_pParticle->CreateNewParticle(&CurrentPos,
											NULL,//&CurrentVel,
											SmokeTrail->DragCo,// Drag coefficient
											NULL, // Matrix orientation
											SmokeTrail->InitAlpha, // Initial Alpha
											SmokeTrail->P1Alpha, // Point 1 Alpha
											SmokeTrail->P2Alpha, // Point 2 Alpha
											SmokeTrail->EndAlpha, // End Alpha
											SmokeTrail->InitSize, // Initial Size
											SmokeTrail->P1Size, // Point 1 Size
											SmokeTrail->P2Size, // Point 2 Size
											SmokeTrail->EndSize, // End Size
											SmokeTrail->LastSpecialDropTime, // Initial time
											SmokeTrail->LastSpecialDropTime + SmokeTrail->P1TimeOffset, // Point 1 time
											SmokeTrail->LastSpecialDropTime + SmokeTrail->P2TimeOffset, // Point 2 time
											SmokeTrail->LastSpecialDropTime + SmokeTrail->EndTimeOffset, // End time
											SmokeTrail->SpriteType,
											&RotationRate,
											SmokeTrail->ParticleFlags & ~PARTICLE_FLAG_USE_VEL_FOR_STRETCH,
											NULL);

		    SmokeTrail->LastSpecialDropTime = frametime;
		}
	}

	CurrentPos = SmokeTrail->LastPos;

    if(NumParticles > 0)
    {
	    TimeStep = DeltaTime / NumParticles * 1000.0f;
    }

	if (Length > 0.00001)
	{
		UnitDeltaVec.x = DeltaVec.x / Length;
		UnitDeltaVec.y = DeltaVec.y / Length;
		UnitDeltaVec.z = DeltaVec.z / Length;
	}
	else
	{
		UnitDeltaVec.x = UnitDeltaVec.y = UnitDeltaVec.z = 0.0;
	}


	for (i=0;i<NumParticles;i++)
	{
		Variable = 1.0f + (mathGetFloat32ChaoticNumber() * 2.0f - 1.0f) * SmokeTrail->Variation;

		CurrentPos.x += UnitDeltaVec.x * UseSpacing;
		CurrentPos.y += UnitDeltaVec.y * UseSpacing;
		CurrentPos.z += UnitDeltaVec.z * UseSpacing;
		TempPos = CurrentPos;
		TempPos.x += UnitDeltaVec.x * UseSpacing * Variable;
		TempPos.y += UnitDeltaVec.y * UseSpacing * Variable;
		TempPos.z += UnitDeltaVec.z * UseSpacing * Variable;
		Dispersion.x = (mathGetFloat32ChaoticNumber() * 2.0f - 1.0f);
		Dispersion.y = (mathGetFloat32ChaoticNumber() * 2.0f - 1.0f);
		Dispersion.z = (mathGetFloat32ChaoticNumber() * 2.0f - 1.0f);
		math3dFastNormalize(&Dispersion);
		TempPos.x += Dispersion.x * SmokeTrail->Dispersion / 2.0f;
		TempPos.y += Dispersion.y * SmokeTrail->Dispersion / 2.0f;
		TempPos.z += Dispersion.z * SmokeTrail->Dispersion / 2.0f;


		RotationRate.x = mathGetFloat32ChaoticNumber() * 2.0f * PI;
		RotationRate.y = mathGetFloat32ChaoticNumber() * SmokeTrail->MaxRotationRate * 2.0f -
							SmokeTrail->MaxRotationRate;
		RotationRate.z = 0.0f;

		Variable = 1.0f + (mathGetFloat32ChaoticNumber() * 2.0f - 1.0f) * SmokeTrail->Variation;

		if (SmokeTrail->ParticleFlags & PARTICLE_FLAG_USE_VEL_FOR_STRETCH)
		{
			Stretch.x = (FLOAT32)(UnitDeltaVec.x * SmokeTrail->Spacing * 2.0);
			Stretch.y = (FLOAT32)(UnitDeltaVec.y * SmokeTrail->Spacing * 2.0);
			Stretch.z = (FLOAT32)(UnitDeltaVec.z * SmokeTrail->Spacing * 2.0);

			m_pParticle->CreateNewParticle(&TempPos,
										   &Stretch,//&CurrentVel,
										   SmokeTrail->DragCo,// Drag coefficient
										   NULL, // Matrix orientation
										   SmokeTrail->InitAlpha, // Initial Alpha
										   SmokeTrail->P1Alpha, // Point 1 Alpha
										   SmokeTrail->P2Alpha, // Point 2 Alpha
										   SmokeTrail->EndAlpha, // End Alpha
										   SmokeTrail->InitSize, // Initial Size
										   SmokeTrail->P1Size, // Point 1 Size
										   SmokeTrail->P2Size, // Point 2 Size
										   SmokeTrail->EndSize, // End Size
										   CurrentTime, // Initial time
										   CurrentTime + (SINT32)(SmokeTrail->P1TimeOffset * Variable + 0.5f), // Point 1 time
										   CurrentTime + (SINT32)(SmokeTrail->P2TimeOffset * Variable + 0.5f), // Point 2 time
										   CurrentTime + (SINT32)(SmokeTrail->EndTimeOffset * Variable + 0.5f), // End time
										   SmokeTrail->SpriteType,
										   &RotationRate,
										   SmokeTrail->ParticleFlags,
										   NULL);
		}
		else
		{
			m_pParticle->CreateNewParticle(&TempPos,
										   NULL,//&CurrentVel,
										   SmokeTrail->DragCo,// Drag coefficient
										   NULL, // Matrix orientation
										   SmokeTrail->InitAlpha, // Initial Alpha
										   SmokeTrail->P1Alpha, // Point 1 Alpha
										   SmokeTrail->P2Alpha, // Point 2 Alpha
										   SmokeTrail->EndAlpha, // End Alpha
										   SmokeTrail->InitSize, // Initial Size
										   SmokeTrail->P1Size, // Point 1 Size
										   SmokeTrail->P2Size, // Point 2 Size
										   SmokeTrail->EndSize, // End Size
										   CurrentTime, // Initial time
										   CurrentTime + (SINT32)(SmokeTrail->P1TimeOffset * Variable + 0.5f), // Point 1 time
										   CurrentTime + (SINT32)(SmokeTrail->P2TimeOffset * Variable + 0.5f), // Point 2 time
										   CurrentTime + (SINT32)(SmokeTrail->EndTimeOffset * Variable + 0.5f), // End time
										   SmokeTrail->SpriteType,
										   &RotationRate,
										   SmokeTrail->ParticleFlags,
										   NULL);
		}

		FloatCurrentTime += TimeStep;
		CurrentTime = (SINT32)(FloatCurrentTime);
		SmokeTrail->LastParticleReleaseTime = frametime;
	}

	SmokeTrail->LastPos = CurrentPos;
    SmokeTrail->LastUpdateTime = CurrentTime;

	return TRUE;
}

LOGICAL CFXEffects::callbackExplosion(EFFECT* pEffect)
{
    SINT32 i = 0;
    piVECTOR PIvelocity;

    if(pEffect == NULL)
        return FALSE;

    EXPLOSION* pExplosion = (EXPLOSION*)pEffect->szData;

    if(pExplosion == NULL)
        return FALSE;

    if(frametime > pExplosion->iStartFrameTime + 500)
    {
        for(i = 0; i < 18; i++)
        {
            PIvelocity.x = (2 * (mathGetChaoticNumber()) / (FLOAT32)(32768) - 1) * 3;
            PIvelocity.y = (2 * (mathGetChaoticNumber()) / (FLOAT32)(32768) - 1) * 3;
            PIvelocity.z = ((UINT16)mathGetChaoticNumber()) / (FLOAT32)(65536) * 30;

            m_pParticle->CreateNewParticle(&pExplosion->fvecPosition,
                                           &PIvelocity,
                                           0.05f,// Drag coefficient
                                           NULL, // Matrix orientation
                                           1.0f, // Initial Alpha
                                           1.0f, // Point 1 Alpha
                                           1.0f, // Point 2 Alpha
                                           1.0f, // End Alpha
                                           3.0f, // Initial Size
                                           9.0f, // Point 1 Size
                                           18.0f, // Point 2 Size
                                           27.0f, // End Size
                                           frametime, // Initial time
                                           frametime, // Point 1 time
                                           frametime + 500, // Point 2 time
                                           frametime + 10000, // End time
                                           23,
                                           NULL,
                                           PARTICLE_FLAG_USESDRAG |
                                           PARTICLE_FLAG_USESGRAVITY,
										   NULL);
        }

        return FALSE;
    }

    return TRUE;
}

SINT32 CFXEffects::GetFreeNode()
{
    if(m_effectFreeHead.next == &m_effectFreeTail)
        return -1;

    if(m_effectFreeHead.next == NULL)
        return -1;

    EFFECT* pNode = m_effectFreeHead.next;

    m_effectFreeHead.next = pNode->next;
    pNode->next->prev = &m_effectFreeHead;

    pNode->next = NULL;
    pNode->prev = NULL;

    AddToActiveList(pNode);

    return pNode->iIndex;
}

LOGICAL CFXEffects::CreateMuzzleFlash(FLOAT64VECTOR const*Pos,
								   FLOAT64VECTOR const*Vel,
								   FLOAT64VECTOR const*ProjVel,
								   FLOAT32 Intensity,
								   SINT32 FireTime,
								   OBJECT * EmitterObject,
								   LOGICAL UseMuzzleSmoke)
{
SINT32 Duration;
FLOAT32 Size;
FLOAT64 Length;
FLOAT64 Density1,Density2;
FLOAT32 TempNum;
piVECTOR RotationRate;
piVECTOR UseVel;
piVECTOR TempVec;

    if(m_pParticle == NULL)
        return FALSE;

	UseVel.x = (FLOAT32)Vel->x;
	UseVel.y = (FLOAT32)Vel->y;
	UseVel.z = (FLOAT32)Vel->z;

	Duration = (SINT32)(pow(Intensity,(FLOAT32)0.333333) * 2.0);
	if (Duration == 0)
	{
		Duration = 1;
	}
	Size = (FLOAT32)(pow(Intensity,(FLOAT32)0.333333) * 2.0 / 100.0);

	// create main flash
	RotationRate.x = mathGetFloat32ChaoticNumber() * 2.0f * PI;
	RotationRate.y = mathGetFloat32ChaoticNumber() * 2.0f * 2.0f -
						2.0f;
	RotationRate.z = 0.0f;

	Length = mathExactVectorLength(ProjVel);
	TempVec.x = (FLOAT32)((ProjVel->x - Vel->x) / Length * Size * 2.0);
	TempVec.y = (FLOAT32)((ProjVel->y - Vel->y) / Length * Size * 2.0);
	TempVec.z = (FLOAT32)((ProjVel->z - Vel->z) / Length * Size * 2.0);

	UseVel.x = TempVec.x;
	UseVel.y = TempVec.y;
	UseVel.z = TempVec.z;

	UseVel.x += (FLOAT32)Vel->x;
	UseVel.y += (FLOAT32)Vel->y;
	UseVel.z += (FLOAT32)Vel->z;

	m_pParticle->CreateNewParticle(Pos,
								   &UseVel,
								   0.03f,
								   NULL,
								   0.85f,
								   0.85f,
								   0.55f,
								   0.0f,
								   Size * 0.0f,
								   Size * 0.3f,
								   Size * 1.0f,
								   Size * 1.25f,
								   FireTime,
								   FireTime + Duration / 2,
								   FireTime + Duration,
								   FireTime + Duration * 2,
								   29,
								   &RotationRate,
								   PARTICLE_FLAG_GLOWS,//PARTICLE_FLAG_USESDRAG,
								   EmitterObject);


	mlGetAltDensity(&Density1,0);
	mlGetAltDensity(&Density2,Pos->z);
	if (Size > 1.0f)
	{
		TempNum = 1.0f;
	}
	else
	{
		TempNum = Size;
	}
	if (UseMuzzleSmoke == TRUE && SuppressMuzzleSmoke == FALSE)
	{
		m_pParticle->CreateNewParticle(Pos,
									   &UseVel,
									   0.7f * (FLOAT32)(Density2 / Density1),
									   NULL,
									   0.0f * TempNum,
									   0.5f * TempNum,
									   0.10f * TempNum,
									   0.0f * TempNum,
									   0.0f,
									   Size * 1.5f,
									   Size * 2.5f,
									   Size * 4.0f,
									   FireTime,
									   FireTime + Duration / 2,
									   FireTime + Duration,
									   FireTime + Duration * 16,
									   27,
									   &RotationRate,
									   PARTICLE_FLAG_USESDRAG,
									   EmitterObject);
	}
	return TRUE;
}

LOGICAL CFXEffects::DetachTexturePockObject(OBJECT * Object)
{
UINT32 i;

	for (i=0;i<MAX_POCKED_TEXTURES;i++)
	{
		if (TexPockMasterObjects[i] == Object)
		{
			TexPockMasterObjects[i] = NULL;
			TexPockNameArray[i][0] = '\0';
		}
	}

	return TRUE;
}

LOGICAL CFXEffects::UpdatePockTextureUpdateTime(OBJECT * TopLevelObject,OBJECT * CollideObject)
{
UINT32 i;

	for (i=0;i<MAX_POCKED_TEXTURES;i++)
	{
		if (TexPockMasterObjects[i] == TopLevelObject && CollideObject->texname != NULL)
		{
			if (strlen(TexPockNameArray[i]) == strlen(CollideObject->texname) &&
				strcmp(TexPockNameArray[i],CollideObject->texname) == 0)
			{
				LastPockUpdateTime[i] = frametime;
				break;
			}
		}
	}
	return TRUE;
}


SINT32 CFXEffects::GetTextureId(OBJECT * TopLevelObject,OBJECT * CollideObject)
{
UINT32 i;
SINT32 SlotNum;
SINT32 OldTexId,NewTexId;

	if (TopLevelObject == NULL || CollideObject == NULL)
	{
		return -1;
	}

	if (CollideObject->flag2 & OBJECT_ALTEREDTEXTURE)
	{
		return CollideObject->texid;
	}

	// first scan list of texture substitutions to see if we already have a duplicate texture
	for (i=0;i<MAX_POCKED_TEXTURES;i++)
	{
		if (TexPockMasterObjects[i] == TopLevelObject && CollideObject->texname != NULL)
		{
			if (strlen(TexPockNameArray[i]) == strlen(CollideObject->texname) &&
				strcmp(TexPockNameArray[i],CollideObject->texname))
			{
				return CollideObject->texid;
			}
		}
	}

	// otherwise we're working on a new texture substitution
	// first see if there's an empty slot
	SlotNum = -1;
	for (i=0;i<MAX_POCKED_TEXTURES;i++)
	{
		if (TexPockMasterObjects[i] == NULL)
		{
			SlotNum = i;
			break;
		}
	}

	if (SlotNum == -1)
	{
		SlotNum = RemoveEarliestPockEntry();
		if (SlotNum == -1)
		{
			return -1;
		}
	}

	// ok now that we have a valid slot, lets create the new texture
	OldTexId = CollideObject->texid;
	NewTexId = piDuplicateRawTexture(OldTexId,0);
	CollideObject->texid = NewTexId;
	piFreeMipTextures(OldTexId);
	CollideObject->flag2 |= OBJECT_ALTEREDTEXTURE;

	TraverseObjectTreeAndSubstitudeTextures(TopLevelObject,OldTexId,NewTexId);
	TexPockMasterObjects[SlotNum] = TopLevelObject;
	strcpy(TexPockNameArray[SlotNum],CollideObject->texname);
	LastPockUpdateTime[SlotNum] = frametime;

	return NewTexId;
}

SINT32 CFXEffects::RemoveEarliestPockEntry()
{
UINT32 i;
SINT32 EarliestTime;
SINT32 SlotNum;

	EarliestTime = frametime + 1;
	SlotNum = -1;

	for (i=0;i<MAX_POCKED_TEXTURES;i++)
	{
		if (LastPockUpdateTime[i] < EarliestTime)
		{
			EarliestTime = LastPockUpdateTime[i];
			SlotNum = i;
		}
	}

	if (SlotNum == -1)
	{
		return -1;
	}

	ResetTextureToOriginalTree(TexPockMasterObjects[SlotNum],TexPockNameArray[SlotNum]);

	LastPockUpdateTime[SlotNum] = 0;
	TexPockMasterObjects[SlotNum] = NULL;
	TexPockNameArray[SlotNum][0] = '\0';


	return SlotNum;
}

LOGICAL CFXEffects::ResetTextureToOriginalTree(OBJECT * Object,SCHAR* TexName)
{
OBJECT * ObjAtt;

	if (Object == NULL || TexName == NULL)
	{
		return TRUE;
	}

	ObjAtt = Object;

	do
	{
		if (ObjAtt->texname != NULL)
		{
			if (strlen(ObjAtt->texname) == strlen(TexName) &&
				strcmp(ObjAtt->texname,TexName) == 0)
			{
				piFreeMipTextures(ObjAtt->texid);
				ObjAtt->texid = piGetNamedTexture(TexName);

				if (ObjAtt->texid < 0)
				{
					ObjAtt->texid = ldpixLoadImageFileToSurface(TexName,0,LDPIX_MIPMAP);
				}
				else
				{
					piAddTexRef(ObjAtt->texid);
				}
				ObjAtt->flag2 &= ~(OBJECT_ALTEREDTEXTURE);
			}
		}

		ObjAtt = ObjAtt->attached;
	}
	while (ObjAtt != NULL);

	if (Object->child != NULL)
	{
		ResetTextureToOriginalSubTree(Object->child,TexName);
	}

	return TRUE;
}

LOGICAL CFXEffects::ResetTextureToOriginalSubTree(OBJECT * Object,SCHAR* TexName)
{
OBJECT * ObjAtt;
OBJECT * ObjSib;

	if (Object == NULL || TexName == NULL)
	{
		return TRUE;
	}

	ObjSib = Object;

	do
	{
		ObjAtt = ObjSib;

		do
		{
			if (ObjAtt->texname != NULL)
			{
				if (strlen(ObjAtt->texname) == strlen(TexName) &&
					strcmp(ObjAtt->texname,TexName) == 0)
				{
					piFreeMipTextures(ObjAtt->texid);
					ObjAtt->texid = piGetNamedTexture(TexName);

					if (ObjAtt->texid < 0)
					{
						ObjAtt->texid = ldpixLoadImageFileToSurface(TexName,0,LDPIX_MIPMAP);
					}
					else
					{
						piAddTexRef(ObjAtt->texid);
					}
					ObjAtt->flag2 &= ~(OBJECT_ALTEREDTEXTURE);
				}
			}

			ObjAtt = ObjAtt->attached;
		}
		while (ObjAtt != NULL);

		if (ObjSib->child != NULL)
		{
			ResetTextureToOriginalSubTree(ObjSib->child,TexName);
		}

		ObjSib = Object->sibling;
	}
	while (ObjSib != NULL);

	return TRUE;
}


LOGICAL CFXEffects::TraverseObjectTreeAndSubstitudeTextures(OBJECT * Object,SINT32 OldTexId,SINT32 NewTexId)
{
OBJECT * ObjAtt;

	if (Object == NULL)
	{
		return TRUE;
	}

	ObjAtt = Object;

	do
	{
		if (ObjAtt->texid == OldTexId)
		{
			if (ObjAtt->flag2 & OBJECT_ALTEREDTEXTURE)
			{
				Write_Debug("Object with altered texture is being texture-substituted again!");
			}
			piFreeMipTextures(OldTexId);
			piAddTexRef(NewTexId);
			ObjAtt->texid = NewTexId;
			ObjAtt->flag2 |= OBJECT_ALTEREDTEXTURE;
		}

		ObjAtt = ObjAtt->attached;
	}
	while (ObjAtt != NULL);

	if (Object->child != NULL)
	{
		TraverseObjectSubTreeAndSubstitudeTextures(Object->child,OldTexId,NewTexId);
	}

	return TRUE;
}

LOGICAL CFXEffects::TraverseObjectSubTreeAndSubstitudeTextures(OBJECT * Object,SINT32 OldTexId,SINT32 NewTexId)
{
OBJECT * ObjAtt;
OBJECT * ObjSib;

	if (Object == NULL)
	{
		return TRUE;
	}

	ObjSib = Object;

	do
	{
		ObjAtt = ObjSib;

		do
		{
			if (ObjAtt->texid == OldTexId)
			{
				if (ObjAtt->flag2 & OBJECT_ALTEREDTEXTURE)
				{
					Write_Debug("Object with altered texture is being texture-substituted again!");
				}
				piFreeMipTextures(OldTexId);
				piAddTexRef(NewTexId);
				ObjAtt->texid = NewTexId;
				ObjAtt->flag2 |= OBJECT_ALTEREDTEXTURE;
			}

			ObjAtt = ObjAtt->attached;
		}
		while (ObjAtt != NULL);

		if (ObjSib->child != NULL)
		{
			TraverseObjectSubTreeAndSubstitudeTextures(ObjSib->child,OldTexId,NewTexId);
		}

		ObjSib = ObjSib->sibling;
	}
	while (ObjSib != NULL);

	return TRUE;
}

LOGICAL CFXEffects::StopEffect(SINT32 HandleNum)
{
    EFFECT * pTemp;

    SINT32 iIndex = HandleNum - 1;

	if (iIndex < 0 || iIndex >= MAX_EFFECTS)
	{
		Write_Debug("Tried to stop an effect for an invalid effect handle! handle = %x",iIndex);
		return TRUE;
	}

	pTemp = &m_EffectArray[iIndex];

    if(pTemp->pfnKillCallback != NULL)
        pTemp->pfnKillCallback(pTemp);

    if(pTemp->pfnOutsideCallback != NULL)
        pTemp->pfnOutsideCallback(pTemp->uShutdownData);

    AddToFreeList(pTemp);

	return TRUE;
}

LOGICAL CFXEffects::GetCurrentPosition(SMOKETRAIL* trail, FLOAT64VECTOR* pos)
{
    OBJECT* lpObj = NULL;
    weapAmmo* ammo = NULL;

    if(trail->ammo) ammo = trail->ammo;
    if(trail->lpObjAttachedTo) lpObj = trail->lpObjAttachedTo;

    if(!ammo && !lpObj)
        return FALSE;

    const FLOAT64VECTOR* pOffset = trnGetWorldOriginOffsetPtr();

    pos->x = trail->OffsetPos.x;
    pos->y = trail->OffsetPos.y;
    pos->z = trail->OffsetPos.z;

    if(lpObj)
    {
        mathVectorMultiply64(pos, pos, &lpObj->mat);
    }
    else
    {
        FLOAT64 x = ammo->Position.x - trnGetWorldOriginOffsetPtr()->x;
        FLOAT64 y = ammo->Position.y - trnGetWorldOriginOffsetPtr()->y;
        FLOAT64 z = ammo->Position.z - trnGetWorldOriginOffsetPtr()->z;

        pos->x = (FLOAT32)x;
        pos->y = (FLOAT32)y;
        pos->z = (FLOAT32)z;
    }

    mathAddVectors(pos, pos, pOffset);

    pos->z = -pos->z;

    return TRUE;
}

LOGICAL CFXEffects::ParticleTooSmallToSee(SMOKETRAIL* lpTrail, FLOAT64VECTOR* pPos)
{
    if(lpTrail->ParticleFlags & PARTICLE_FLAG_AT_LEAST_1_PIXEL)
        return FALSE;

    FLOAT32 fMaxSize = 0;

    fMaxSize = lpTrail->InitSize > lpTrail->P1Size ? lpTrail->InitSize : lpTrail->P1Size;
    fMaxSize = lpTrail->P2Size > fMaxSize ? lpTrail->P2Size : fMaxSize;
    fMaxSize = lpTrail->EndSize > fMaxSize ? lpTrail->EndSize : fMaxSize;

    const FLOAT64VECTOR* pOffset = trnGetWorldOriginOffsetPtr();

    piVECTOR vecPlayerPos;
    piVECTOR vecOffset;

    vecOffset.x = (FLOAT32)pOffset->x;
    vecOffset.y = (FLOAT32)pOffset->y;
    vecOffset.z = (FLOAT32)pOffset->z;

	mathSubVectors(&vecPlayerPos, &vecOffset, &playpos);
	vecPlayerPos.z = -vecPlayerPos.z;

    piVECTOR vecParticleToPlayer;

    piVECTOR vecCurrentPos;

    vecCurrentPos.x = (FLOAT32)pPos->x;
    vecCurrentPos.y = (FLOAT32)pPos->y;
    vecCurrentPos.z = (FLOAT32)pPos->z;

    mathSubVectors(&vecParticleToPlayer, &vecCurrentPos, &vecPlayerPos);

    FLOAT32 fDistance = (FLOAT32)mathExactVectorLength(&vecParticleToPlayer);

	FLOAT32 fOnePixelRatio = FieldOfView * PI / 180.0f / piGetWidth() * 0.25f;

	if(fMaxSize < fOnePixelRatio * fDistance)
	{
		// Particles would be too small to see
		lpTrail->LastPos = *pPos;
		lpTrail->LastUpdateTime = frametime;
		return TRUE;
	}

    return FALSE;
}
