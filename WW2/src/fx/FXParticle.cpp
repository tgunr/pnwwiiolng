#include <syscfg.h> // Path is project specific.

/*
	 Manages particles within the Game such as hit sprites, tracers, muzzle flashes
	    smoke, fires, hit effects, darn near everything you see when any fire going.

	 Copyright 2008 Playnet Inc, All rights reserved
	 Not to be distributed in API form except as part of a compiled library
	 under License or NDA

	Originating Programmer : Hoof
	Maintaining Programmer : Caligula
	Supporting Programmer  : None

	Rev 1  c2001
*/

#include "pi/pi.h"
#include "include/vars.h"

#include <math.h>

#include "fx/FXParticle.h"
#include "fx/FXInterface.h"
#include "ld/ldpix.h"
#include "rn/rnmain.h"
#include "trn/trnmain.h"
#include "math/math3d.h"
#include "math/mathvect.h"
#include "math/matheul.h"
#include "ob/obdlite.h"
#include "locl/loclMain.h"

#include "draw/draw3d.h"

static SINT32 _SortCompare(const void* pObj1, const void* pObj2);

CFXParticle::CFXParticle()
{
	//  Insure all member variables are set to zero, although this MAY be done
	//  by a compilier no assumptions should be made concerning default initialization
	//  of any variable.

    m_lpDummyObject = NULL;
    m_lpObject = NULL;

    memset(m_Particles, 0, sizeof(PARTICLE) * DEF_MAX_PARTICLES);

    memset(&m_Head, 0, sizeof(PARTICLE));
    memset(&m_Tail, 0, sizeof(PARTICLE));
    memset(&m_FreeHead, 0, sizeof(PARTICLE));
    memset(&m_FreeTail, 0, sizeof(PARTICLE));

    m_Head.next = &m_Tail;
    m_Tail.prev = &m_Head;

    m_FreeHead.next = &m_FreeTail;
    m_FreeTail.prev = &m_FreeHead;

    memset(m_Vertices, 0, sizeof(piLVERTEX) * 512);

    Count = 0;
    iParticleTexId = 0;
    iParticleTexIdGlow = 0;
    iLastUpdateTime = 0;
}

CFXParticle::~CFXParticle()
{
}

LOGICAL CFXParticle::Initialize()
{
UINT32 i;

	for(i=0; i < DEF_MAX_PARTICLES; i++)
	{
        m_Particles[i].iIndex = i;
		AddToFreeList(&m_Particles[i]);
	}

	Count = 0;

	// load up the particle texture
	iParticleTexId = ldpixLoadImageFileToSurface("sprites1.bmp", colorBLACK, LDPIX_NONE);
	if (!piIsTexture(iParticleTexId, 0))
	{
		Write_Debug("Unable to load up the particle system texture map!");
		return(FALSE);
	}

	// load up the particle texture
	iParticleTexIdGlow = ldpixLoadImageFileToSurface("sprites1.bmp", colorBLACK, LDPIX_NONE);
	if (!piIsTexture(iParticleTexIdGlow, 0))
	{
		Write_Debug("Unable to load up the particle system texture map!");
		return(FALSE);
	}

	// lets ensure that the m_ParticleDisplayHead is always most distant object
	m_ParticleDisplayHead.DisplayDist = 1000000000.0;

	return TRUE;
}

LOGICAL CFXParticle::KillAll()
{
PARTICLE* Particle,*UseParticle;

	piFreeMipTextures(iParticleTexId);
	piFreeMipTextures(iParticleTexIdGlow);

	for (Particle = m_Head.next;Particle != NULL;)
	{
		UseParticle = Particle;
		Particle = Particle->next;
		AddToFreeList(UseParticle);
	}

	return TRUE;
}

LOGICAL CFXParticle::AddToFreeList(PARTICLE* pPart)
{
    PARTICLE* pPrev = NULL;
    SINT32 i;

    if(pPart == NULL)
        return FALSE;

    RemoveFromList(pPart);

    i = pPart->iIndex;

    memset(pPart, 0, sizeof(PARTICLE));

    pPart->iIndex = i;

    pPrev = m_FreeTail.prev;

    pPart->prev = pPrev;
    pPart->next = &m_FreeTail;

    pPrev->next = pPart;
    m_FreeTail.prev = pPart;

    return TRUE;
}

LOGICAL CFXParticle::AddToActiveList(PARTICLE* pPart)
{
    if(pPart == NULL)
        return FALSE;

    if(m_Head.next == NULL)
        return FALSE;

	pPart->prev = &m_Head;
	pPart->next = m_Head.next;

    m_Head.next->prev = pPart;
    m_Head.next = pPart;

	return TRUE;
}

LOGICAL CFXParticle::RemoveFromList(PARTICLE* pPart)
{
    PARTICLE* pNext = NULL;
    PARTICLE* pPrev = NULL;

    if(pPart == NULL)
        return FALSE;

    pNext = pPart->next;
    pPrev = pPart->prev;

    if(pNext == NULL || pPrev == NULL)
        return FALSE;

    pPrev->next = pNext;
    pNext->prev = pPrev;

    pPart->prev = NULL;
    pPart->next = NULL;

	return TRUE;
}

LOGICAL CFXParticle::Update()
{
    FLOAT32 Speed,OrigSpeed;
    FLOAT32 DeltaTime;
    PARTICLE * pParticle;
    PARTICLE * pParticleNext;
    OBJECT * lpTempObject;
    FLOAT64VECTOR WorldOriginOffset;

	trnGetWorldOriginOffset(&WorldOriginOffset);

	iLastUpdateTime = frametime;

    pParticle = m_Head.next;

    if(pParticle == NULL)
        return FALSE;

    while(pParticle != &m_Tail)
	{
		DeltaTime = (frametime - pParticle->pParticleLastUpdateTime) / 1000.0f;
		pParticle->pParticleLastUpdateTime = frametime;
		// check to see if it's expired
		if (frametime - pParticle->iTime[FXPARTICLE_DECAYMAXPOS] > 0  &&
			!(pParticle->uFlags & PARTICLE_FLAG_DISPLAY_FOR_ONE_FRAME))
		{
			if (!(pParticle->uFlags & PARTICLE_FLAG_NEVERDIES))
			{
				pParticleNext = pParticle->next;
				KillParticle(pParticle);
				pParticle = pParticleNext;
				continue;
			}
		}

		if (pParticle->uFlags & PARTICLE_FLAG_DISPLAY_AFTER_ONE_FRAME &&
			pParticle->uFlags & PARTICLE_FLAG_ONE_FRAME_ALREADY)
		{
			pParticle->uFlags &= ~PARTICLE_FLAG_DISPLAY_AFTER_ONE_FRAME;
			pParticle->uFlags &= ~PARTICLE_FLAG_ONE_FRAME_ALREADY;
		}

		if (pParticle->uFlags & PARTICLE_FLAG_DISPLAY_AFTER_ONE_FRAME)
		{
			pParticle->uFlags |= PARTICLE_FLAG_ONE_FRAME_ALREADY;
		}

		if (pParticle->uFlags & PARTICLE_FLAG_DISPLAY_FOR_ONE_FRAME)
		{
			if (pParticle->uFlags & PARTICLE_FLAG_ONE_FRAME_ALREADY)
			{
				if (!(pParticle->uFlags & PARTICLE_FLAG_NEVERDIES))
				{
					pParticleNext = pParticle->next;
					KillParticle(pParticle);
					pParticle = pParticleNext;
					continue;
				}
			}
			pParticle->iTime[0] = frametime;
			pParticle->iTime[1] = frametime + 10000;
			pParticle->iTime[2] = frametime + 20000;
			pParticle->iTime[3] = frametime + 30000;
		}

		// check to see if we're starting relative to another object
		if (pParticle->pAttachObject != NULL)
		{
			lpTempObject = pParticle->pAttachObject;

			pParticle->fvectorPosition = pParticle->OrigPos;
			pParticle->fvectorPosition.z = -pParticle->fvectorPosition.z;
			mathVectorMultiply64(&pParticle->fvectorPosition,&pParticle->fvectorPosition,
								 &lpTempObject->mat);
			mathAddVectors(&pParticle->fvectorPosition,&pParticle->fvectorPosition,
							&WorldOriginOffset);
			pParticle->fvectorPosition.z = -pParticle->fvectorPosition.z;

			if (!(pParticle->uFlags & PARTICLE_FLAG_USE_VEL_FOR_STRETCH))
			{
				pParticle->fvectorPosition.x -= pParticle->vecVelocity.x * DeltaTime;
				pParticle->fvectorPosition.y -= pParticle->vecVelocity.y * DeltaTime;
				pParticle->fvectorPosition.z -= pParticle->vecVelocity.z * DeltaTime;
			}

			if (!(pParticle->uFlags & PARTICLE_FLAG_ATTACHED_TO_OBJECT))
			{
				pParticle->pAttachObject = NULL;
			}
		}

		// see if it's in the gain phase
		if ((pParticle->iTime[FXPARTICLE_INITPOS] < frametime) &&
			(pParticle->iTime[FXPARTICLE_GAINMAXPOS] >= frametime) &&
			(pParticle->iTime[FXPARTICLE_INITPOS] != pParticle->iTime[FXPARTICLE_GAINMAXPOS]))
		{
			pParticle->fUseAlpha =
				(pParticle->fAlpha[FXPARTICLE_GAINMAXPOS] - pParticle->fAlpha[FXPARTICLE_INITPOS]) *
				(FLOAT32)(frametime - pParticle->iTime[FXPARTICLE_INITPOS]) /
				(FLOAT32)(pParticle->iTime[FXPARTICLE_GAINMAXPOS] - pParticle->iTime[FXPARTICLE_INITPOS]) +
				pParticle->fAlpha[FXPARTICLE_INITPOS];

			pParticle->fUseSize =
				(pParticle->fSize[FXPARTICLE_GAINMAXPOS] - pParticle->fSize[FXPARTICLE_INITPOS]) *
				(FLOAT32)(frametime - pParticle->iTime[FXPARTICLE_INITPOS]) /
				(FLOAT32)(pParticle->iTime[FXPARTICLE_GAINMAXPOS] - pParticle->iTime[FXPARTICLE_INITPOS]) +
				pParticle->fSize[FXPARTICLE_INITPOS];

		}

		// see if it's in the sustain phase
		if ((pParticle->iTime[FXPARTICLE_GAINMAXPOS] < frametime) &&
			(pParticle->iTime[FXPARTICLE_SUSTAINMAXPOS] >= frametime) &&
			(pParticle->iTime[FXPARTICLE_GAINMAXPOS] != pParticle->iTime[FXPARTICLE_SUSTAINMAXPOS]))
		{
			pParticle->fUseAlpha =
				(pParticle->fAlpha[FXPARTICLE_SUSTAINMAXPOS] - pParticle->fAlpha[FXPARTICLE_GAINMAXPOS]) *
				(FLOAT32)(frametime - pParticle->iTime[FXPARTICLE_GAINMAXPOS]) /
				(FLOAT32)(pParticle->iTime[FXPARTICLE_SUSTAINMAXPOS] - pParticle->iTime[FXPARTICLE_GAINMAXPOS]) +
				pParticle->fAlpha[FXPARTICLE_GAINMAXPOS];

			pParticle->fUseSize =
				(pParticle->fSize[FXPARTICLE_SUSTAINMAXPOS] - pParticle->fSize[FXPARTICLE_GAINMAXPOS]) *
				(FLOAT32)(frametime - pParticle->iTime[FXPARTICLE_GAINMAXPOS]) /
				(FLOAT32)(pParticle->iTime[FXPARTICLE_SUSTAINMAXPOS] - pParticle->iTime[FXPARTICLE_GAINMAXPOS]) +
				pParticle->fSize[FXPARTICLE_GAINMAXPOS];

		}

		// see if it's in the decay phase
		if ((pParticle->iTime[FXPARTICLE_SUSTAINMAXPOS] < frametime) &&
			(pParticle->iTime[FXPARTICLE_DECAYMAXPOS] >= frametime) &&
			(pParticle->iTime[FXPARTICLE_SUSTAINMAXPOS] != pParticle->iTime[FXPARTICLE_DECAYMAXPOS]))
		{
			pParticle->fUseAlpha =
				(pParticle->fAlpha[FXPARTICLE_DECAYMAXPOS] - pParticle->fAlpha[FXPARTICLE_SUSTAINMAXPOS]) *
				(FLOAT32)(frametime - pParticle->iTime[FXPARTICLE_SUSTAINMAXPOS]) /
				(FLOAT32)(pParticle->iTime[FXPARTICLE_DECAYMAXPOS] - pParticle->iTime[FXPARTICLE_SUSTAINMAXPOS]) +
				pParticle->fAlpha[FXPARTICLE_SUSTAINMAXPOS];

			pParticle->fUseSize =
				(pParticle->fSize[FXPARTICLE_DECAYMAXPOS] - pParticle->fSize[FXPARTICLE_SUSTAINMAXPOS]) *
				(FLOAT32)(frametime - pParticle->iTime[FXPARTICLE_SUSTAINMAXPOS]) /
				(FLOAT32)(pParticle->iTime[FXPARTICLE_DECAYMAXPOS] - pParticle->iTime[FXPARTICLE_SUSTAINMAXPOS]) +
				pParticle->fSize[FXPARTICLE_SUSTAINMAXPOS];

		}

		// now update the physics of the particle
		if (!(pParticle->uFlags & PARTICLE_FLAG_USE_VEL_FOR_STRETCH))
		{
			if (pParticle->uFlags & PARTICLE_FLAG_USESGRAVITY)
			{
				pParticle->vecVelocity.z += -9.8f * DeltaTime;
			}
			if (pParticle->uFlags & PARTICLE_FLAG_RISES)
			{
				pParticle->vecVelocity.z -= -9.8f * DeltaTime;
			}

			if (pParticle->uFlags & PARTICLE_FLAG_USESDRAG)
			{
				// using drag accel = constant * velocity
				OrigSpeed = (FLOAT32)mathExactVectorLength(&pParticle->vecVelocity);
				Speed = (FLOAT32)(OrigSpeed * exp(-pParticle->fDragCo * DeltaTime));

				if (OrigSpeed > 0.00001f || OrigSpeed < -0.00001f)
				{
					pParticle->vecVelocity.x = Speed * pParticle->vecVelocity.x / OrigSpeed;
					pParticle->vecVelocity.y = Speed * pParticle->vecVelocity.y / OrigSpeed;
					pParticle->vecVelocity.z = Speed * pParticle->vecVelocity.z / OrigSpeed;
				}
				else
				{
					pParticle->vecVelocity.x = pParticle->vecVelocity.y = pParticle->vecVelocity.z = 0.0f;
				}
			}

			pParticle->fvectorPosition.x += pParticle->vecVelocity.x * DeltaTime;
			pParticle->fvectorPosition.y += pParticle->vecVelocity.y * DeltaTime;
			pParticle->fvectorPosition.z += pParticle->vecVelocity.z * DeltaTime;

			if (pParticle->uFlags & PARTICLE_FLAG_DONT_GO_BELOW_START_ALT)
			{
				if (!(pParticle->uFlags & PARTICLE_FLAG_NEVERDIES))
				{
					if (pParticle->fvectorPosition.z < pParticle->StartAlt - pParticle->fUseSize / 2)
					{
						pParticleNext = pParticle->next;
						KillParticle(pParticle);
						pParticle = pParticleNext;
						continue;
					}
				}
			}
		}

		// handle any rotation
		if (pParticle->uFlags & PARTICLE_FLAG_USESROTATION)
		{
			pParticle->vecRotAtt.x += pParticle->vecRotAtt.y * DeltaTime;
			pParticle->vecRotAtt.x = (FLOAT32)fmod(pParticle->vecRotAtt.x,(FLOAT32)(PI*2.0));
		}

        piVECTOR vecPos;

		vecPos.x = (FLOAT32)(pParticle->fvectorPosition.x - WorldOriginOffset.x);
		vecPos.y = (FLOAT32)(pParticle->fvectorPosition.y - WorldOriginOffset.y);
		vecPos.z = -(FLOAT32)(pParticle->fvectorPosition.z + WorldOriginOffset.z);

        piVECTOR vecDelta;

		vecDelta.x = vecPos.x + playpos.x;
		vecDelta.y = vecPos.y + playpos.y;
		vecDelta.z = vecPos.z + playpos.z;

		// This is* NOT* the distance squared, it is the distance along playdir
		pParticle->DisplayDist = math3dDotProduct(&vecDelta,&playdir);

		// all done! onto the next particle
		pParticle = pParticle->next;
	}

	return TRUE;
}


LOGICAL CFXParticle::DisplayParticles(SINT32 DrawCockpitParticles)
{
    PARTICLE * pParticle;
    DYNALITE* DynaLite;
    piLIGHT* light;

    piMATRIX OrigWorldMat;
    piMATRIX Mat1,Mat2,Mat3;
    piMATRIX ViewAttMatrix;
    piMATRIX SavedViewMatrix;

    piVECTOR vecPos;
    piVECTOR vecCorner1,vecCorner2,vecCorner3,vecCorner4;
    piVECTOR TempVec,TempVec1;
    piVECTOR V1,V2;

    FLOAT32 size;
    FLOAT32 StretchLength;
    FLOAT32 Ratio;
    FLOAT32 OneOverHypotenuse;
    FLOAT32 Len,Len1,Len2;
    FLOAT32 Cos;
    FLOAT32 TempNum;
    FLOAT32 OnePixelRatio;
    FLOAT32 Dist;
    FLOAT32 DaylightBrightness;
    FLOAT32 fMinU,fMinV,fMaxU,fMaxV;

    UINT32 Color;
    UINT32 AmbColor;
    UINT32 Red,Green,Blue,Alpha;
    UINT32 uVCount;

    LOGICAL LastParticleWasGlow;

    piCOLORVALUE SunColor;
    piCOLORVALUE UseBaseColor;

    SINT32 PassNum;
    SINT32 CockpitOffset;

    FLOAT64VECTOR fvectorWorldOriginOffset;

	if (DrawCockpitParticles == 0)
	{
		CockpitOffset = 0;
	}
	else
	{
		CockpitOffset = 2;
	}

	AmbColor = piGetAmbLight();

	obdliteGetLights(&DynaLite);
	light = &(DynaLite->light.light);
	SunColor = light->dcvColor;

	TempVec.z = -1.0f;
	TempVec.x = TempVec.y = 0.0f;
	DaylightBrightness = -math3dDotProduct(&TempVec, &light->dvDirection);

	if (DaylightBrightness < 0.0f)
	{
		DaylightBrightness = 0.0f;
	}

	UseBaseColor.r = SunColor.r * 255.0f * DaylightBrightness + 0.5f;
	UseBaseColor.g = SunColor.g * 255.0f * DaylightBrightness + 0.5f;
	UseBaseColor.b = SunColor.b * 255.0f * DaylightBrightness + 0.5f;

	ViewAttMatrix = *piGetUnscaledViewMatrix();
	ViewAttMatrix.m[3][0] = ViewAttMatrix.m[3][1] = ViewAttMatrix.m[3][2] = 0.0f;

    static PARTICLEINFO arySort[DEF_MAX_PARTICLES];
    SINT32 iAryCount = 0;

	// first sort them by distance
	SortDisplayList(arySort, &iAryCount);

    if(iAryCount == 0)
        return TRUE;

	// now display them
	trnGetWorldOriginOffset(&fvectorWorldOriginOffset);

	OrigWorldMat = *piGetWorldMatrix();

	piSetWorldMatrix(&identity);

	piSetRenderState(piRENDERSTATE_ZBIAS, 3);
	piSetActiveTexture(iParticleTexId, 0);
	piDisableMaterial();
	piSetRenderStageState(0, piTSS_MIPFILTER, piTFP_NONE);
    piSetRenderState(piRENDERSTATE_LIGHTING, FALSE);

	// One Pixel Size calculations
	OnePixelRatio = FieldOfView * PI / 180.0f / piGetWidth() * 20.0f;

	// pass number 0 is alpharef particles
	// pass number 1 is normal particles
	// pass number 2 is cockpit alpharef particles
	// pass number 3 is cockpit normal particles
	piSetRenderState(piRENDERSTATE_DESTBLEND, DestBlend);

	for (PassNum = 0 + CockpitOffset;PassNum < 2 + CockpitOffset; PassNum++)
	{
		if (PassNum == 0 || PassNum == 2)
		{
			piSetRenderState(piRENDERSTATE_ALPHAREF, 128);
			piSetRenderState(piRENDERSTATE_SRCBLEND, piBLEND_ONE);
			piSetRenderState(piRENDERSTATE_DESTBLEND, piBLEND_ZERO);
		}

		if (PassNum == 2 || PassNum == 3)
		{
			SavedViewMatrix = *piGetViewMatrix();

			piSetViewMatrix(piGetCockpitViewMatrix());
		}

		uVCount = 0;
		LastParticleWasGlow = FALSE;

		for(SINT32 i = 0; i < iAryCount; i++)
		{
            pParticle = &m_Particles[arySort[i].iIndex];

            if (pParticle->uFlags & PARTICLE_FLAG_ONE_FRAME_ALREADY)
			{
				continue;
			}

			if (pParticle->uFlags & PARTICLE_FLAG_CALL_USERDRAW)
			{
				if (pParticle->UserDraw != NULL)
				{
					pParticle->UserDraw(&pParticle->fvectorPosition,
										pParticle->fUseAlpha,
										pParticle->fUseSize,
										pParticle->uParticleNum);
				}

				continue;
			}

			if (PassNum == 2 || PassNum == 3)
			{
				if (!(pParticle->uFlags & PARTICLE_FLAG_INCOCKPIT_PARTICLE))
				{
					continue;
				}
			}
			else
			{
				if (pParticle->uFlags & PARTICLE_FLAG_INCOCKPIT_PARTICLE)
				{
					continue;
				}
			}
			if (PassNum == 0 || PassNum == 2)
			{
				if (!(pParticle->uFlags & PARTICLE_FLAG_USE_ALPHAREF))
				{
					continue;
				}
			}
			else
			{
				if (pParticle->uFlags & PARTICLE_FLAG_USE_ALPHAREF)
				{
					continue;
				}
			}

			if (uVCount > 511 - 6)
			{
				if (!piRenderScene(piPT_TRIANGLELIST, piVT_LVERTEX,
									&m_Vertices, uVCount))
				{
					return(FALSE);
				}
				// draw vertecies
				uVCount = 0;
			}

			if (pParticle->uFlags & PARTICLE_FLAG_GLOWS &&
				LastParticleWasGlow == FALSE)
			{
				if (uVCount > 0)
				{
					if (!piRenderScene(piPT_TRIANGLELIST, piVT_LVERTEX,
										&m_Vertices, uVCount))
					{
						return(FALSE);
					}
					// draw vertecies
					uVCount = 0;
				}
				piSetActiveTexture(iParticleTexIdGlow, 0);

				piSetRenderState(piRENDERSTATE_DESTBLEND, piBLEND_ONE);

				UseBaseColor.r = 255.0f;
				UseBaseColor.g = 255.0f;
				UseBaseColor.b = 255.0f;

				LastParticleWasGlow = TRUE;
			}

			if (!(pParticle->uFlags & PARTICLE_FLAG_GLOWS) &&
				LastParticleWasGlow == TRUE)
			{
				if (uVCount > 0)
				{
					if (!piRenderScene(piPT_TRIANGLELIST, piVT_LVERTEX,
										&m_Vertices, uVCount))
					{
						return(FALSE);
					}
					// draw vertecies
					uVCount = 0;
				}
				piSetActiveTexture(iParticleTexId, 0);

				piSetRenderState(piRENDERSTATE_DESTBLEND, DestBlend);

				UseBaseColor.r = SunColor.r * 255.0f * DaylightBrightness + 0.5f;
				UseBaseColor.g = SunColor.g * 255.0f * DaylightBrightness + 0.5f;
				UseBaseColor.b = SunColor.b * 255.0f * DaylightBrightness + 0.5f;

				LastParticleWasGlow = FALSE;
			}

			if (pParticle->uFlags & PARTICLE_FLAG_DISPLAY_FOR_ONE_FRAME)
			{
				pParticle->uFlags |= PARTICLE_FLAG_ONE_FRAME_ALREADY;
			}

			vecPos.x = (FLOAT32)(pParticle->fvectorPosition.x - fvectorWorldOriginOffset.x);
			vecPos.y = (FLOAT32)(pParticle->fvectorPosition.y - fvectorWorldOriginOffset.y);
			vecPos.z = -(FLOAT32)(pParticle->fvectorPosition.z + fvectorWorldOriginOffset.z);

			// compute min/max
			GetUVMinMax(&fMinU,&fMinV,&fMaxU,&fMaxV,pParticle->uParticleNum);

			// compute alpha and scale
			Red = FloatToInt(UseBaseColor.r) + piRGBA_GETRED(AmbColor);
			if (Red > 255)
			{
				Red = 255;
			}
			Green = FloatToInt(UseBaseColor.g) + piRGBA_GETGREEN(AmbColor);
			if (Green > 255)
			{
				Green = 255;
			}
			Blue = FloatToInt(UseBaseColor.b) + piRGBA_GETBLUE(AmbColor);
			if (Blue > 255)
			{
				Blue = 255;
			}
			if (PassNum == 0 || PassNum == 2)
			{
				Alpha = FloatToInt((pParticle->fUseAlpha * 0.5f + 0.5f) * 255.0f + 0.5f);
			}
			else
			{
				Alpha = FloatToInt(pParticle->fUseAlpha * 255.0f + 0.5f);
			}
			if (Alpha > 255)
			{
				Alpha = 255;
			}
			Color = piRGBA_MAKE(Red,Green,Blue,Alpha);

			size = 0.5f * pParticle->fUseSize;

			if (pParticle->uFlags & PARTICLE_FLAG_AT_LEAST_1_PIXEL)
			{
				// FIXME - This is* NOT* correct, the distance is not squared in _SortDisplayList
				// plus this is probably not correct distance to use for size calculation

                Dist = 0.0f;

                if(pParticle->DisplayDist > 0.0f)
                {
                    Dist = (FLOAT32)sqrt(pParticle->DisplayDist);
                }

				if (size < OnePixelRatio * Dist)
				{
					size = OnePixelRatio * Dist;
				}
			}

			// compute corners
			vecCorner1.x = vecPos.x - playright.x * size - playup.x * size;
			vecCorner1.y = vecPos.y - playright.y * size - playup.y * size;
			vecCorner1.z = vecPos.z - playright.z * size - playup.z * size;

			vecCorner2.x = vecPos.x + playright.x * size - playup.x * size;
			vecCorner2.y = vecPos.y + playright.y * size - playup.y * size;
			vecCorner2.z = vecPos.z + playright.z * size - playup.z * size;

			vecCorner3.x = vecPos.x + playright.x * size + playup.x * size;
			vecCorner3.y = vecPos.y + playright.y * size + playup.y * size;
			vecCorner3.z = vecPos.z + playright.z * size + playup.z * size;

			vecCorner4.x = vecPos.x - playright.x * size + playup.x * size;
			vecCorner4.y = vecPos.y - playright.y * size + playup.y * size;
			vecCorner4.z = vecPos.z - playright.z * size + playup.z * size;

			// apply rotation if applicable
			if (pParticle->uFlags & PARTICLE_FLAG_USESROTATION)
			{

				// set up displacement and conversion to view space
				TempVec1.x = -vecPos.x;
				TempVec1.y = -vecPos.y;
				TempVec1.z = -vecPos.z;
				math3dVectorMultiply(&TempVec1,&TempVec1,&ViewAttMatrix);
				Mat2 = ViewAttMatrix;
				Mat2.m[3][0]/*_41*/ = TempVec1.x;
				Mat2.m[3][1]/*_42*/ = TempVec1.y;
				Mat2.m[3][2]/*_43*/ = TempVec1.z;

				// rotation
				Mat1 = identity;
				Mat1.m[0][0] = Mat1.m[1][1] = (FLOAT32)cos(pParticle->vecRotAtt.x);
				Mat1.m[0][1] = (FLOAT32)sin(pParticle->vecRotAtt.x);
				Mat1.m[1][0] = -Mat1.m[0][1];
				math3dMatrixMultiply(&Mat3,&Mat2,&Mat1);

				// undo back to render space
				math3dInvertMatrix(&Mat2,&Mat2);
				math3dMatrixMultiply(&Mat2,&Mat3,&Mat2);

				math3dVectorMultiply(&vecCorner1,&vecCorner1,&Mat2);
				math3dVectorMultiply(&vecCorner2,&vecCorner2,&Mat2);
				math3dVectorMultiply(&vecCorner3,&vecCorner3,&Mat2);
				math3dVectorMultiply(&vecCorner4,&vecCorner4,&Mat2);
			}
			// stretch the particle if necessary
			// must occur after any rotation
			if (pParticle->uFlags & PARTICLE_FLAG_USE_VEL_FOR_STRETCH && size > 0.0001f)
			{
				Ratio = pParticle->StretchLength / size;
				TempVec.x = pParticle->vecVelocity.x * Ratio;
				TempVec.y = pParticle->vecVelocity.y * Ratio;
				TempVec.z = -pParticle->vecVelocity.z * Ratio;

				TempVec1.x = pParticle->vecVelocity.x * pParticle->StretchLength;
				TempVec1.y = pParticle->vecVelocity.y * pParticle->StretchLength;
				TempVec1.z = -pParticle->vecVelocity.z * pParticle->StretchLength;
				mathAddVectors(&TempVec1,&vecPos,&TempVec1);
				mathAddVectors(&TempVec1,&TempVec1,&playpos);
				Len = mathVectDotProduct(&playdir,&TempVec1);
				if (Len <= 0.0)//size * 2.0f)
				{
					continue;
				}

				TempVec1.x = pParticle->vecVelocity.x * pParticle->StretchLength;
				TempVec1.y = pParticle->vecVelocity.y * pParticle->StretchLength;
				TempVec1.z = -pParticle->vecVelocity.z * pParticle->StretchLength;
				mathSubVectors(&TempVec1,&vecPos,&TempVec1);
				mathAddVectors(&TempVec1,&TempVec1,&playpos);
				Len = mathVectDotProduct(&playdir,&TempVec1);
				if (Len <= 0.0f)//size * 2.0f)
				{
					continue;
				}

				// we must project this point onto the view plane
				// and then compensate for perspective

				// determine direction to point
				mathAddVectors(&V1,&vecPos,&playpos);
				Len = mathVectDotProduct(&playdir,&TempVec);

				if (Len < 0.0f)
				{
					mathSubVectors(&V2,&V1,&TempVec);
				}
				else
				{
					mathAddVectors(&V2,&V1,&TempVec);
				}

				Len1 = (FLOAT32)mathExactVectorLength(&V1);
				Len2 = (FLOAT32)mathExactVectorLength(&V2);
				if (Len2 < Len1)
				{
					Len1 = Len2;
					V1 = V2;
				}
				TempNum = 1.0f / Len1;
				V1.x *= TempNum;
				V1.y *= TempNum;
				V1.z *= TempNum;

				// now determine amount to move point
				Cos = mathVectDotProduct(&playdir,&V1);

				if (Cos > 0.001f || Cos < -0.001f)
				{
					Len = Len / Cos;

					// move point
					TempVec.x -= V1.x * Len;
					TempVec.y -= V1.y * Len;
					TempVec.z -= V1.z * Len;
				}

				math3dVectorMultiply(&TempVec,&TempVec,&ViewAttMatrix);

				Len = StretchLength = (FLOAT32)sqrt(TempVec.x * TempVec.x + TempVec.y * TempVec.y);

				if (StretchLength > 1.0f)
				{

					OneOverHypotenuse = 1.0f / Len;

					// set up displacement and conversion to view space
					TempVec1.x = -vecPos.x;
					TempVec1.y = -vecPos.y;
					TempVec1.z = -vecPos.z;
					math3dVectorMultiply(&TempVec1,&TempVec1,&ViewAttMatrix);
					Mat2 = ViewAttMatrix;
					Mat2.m[3][0]/*_41*/ = TempVec1.x;
					Mat2.m[3][1]/*_42*/ = TempVec1.y;
					Mat2.m[3][2]/*_43*/ = TempVec1.z;

					// now rotate so that we're aligned with the y axis for stretching
					Mat1 = identity;

					Mat1.m[0][0] = Mat1.m[1][1] = TempVec.y * OneOverHypotenuse; // cos
					Mat1.m[0][1] = TempVec.x * OneOverHypotenuse; // sin
					Mat1.m[1][0] = -Mat1.m[0][1];
					math3dMatrixMultiply(&Mat1,&Mat2,&Mat1);

					// now STREEEEEEEEETTTTTCCCCCHHHHHH!!!!!! :)
					Mat3 = identity;
					Mat3.m[1][1]/*_22*/ = StretchLength;
					math3dMatrixMultiply(&Mat2,&Mat1,&Mat3);

					// invert the setup matrix
					math3dInvertMatrix(&Mat1,&Mat1);

					// and reverse everything so we're back in render space
					math3dMatrixMultiply(&Mat2,&Mat2,&Mat1);

					math3dVectorMultiply(&vecCorner1,&vecCorner1,&Mat2);
					math3dVectorMultiply(&vecCorner2,&vecCorner2,&Mat2);
					math3dVectorMultiply(&vecCorner3,&vecCorner3,&Mat2);
					math3dVectorMultiply(&vecCorner4,&vecCorner4,&Mat2);
				}

			}

			// now the vertices

			m_Vertices[uVCount].x = vecCorner3.x;
			m_Vertices[uVCount].y = vecCorner3.y;
			m_Vertices[uVCount].z = vecCorner3.z;
			m_Vertices[uVCount].color = Color;
			m_Vertices[uVCount].specular = colorALPHA100;
			m_Vertices[uVCount].tu = fMaxU;
			m_Vertices[uVCount++].tv = fMaxV;

			m_Vertices[uVCount].x = vecCorner2.x;
			m_Vertices[uVCount].y = vecCorner2.y;
			m_Vertices[uVCount].z = vecCorner2.z;
			m_Vertices[uVCount].color = Color;
			m_Vertices[uVCount].specular = colorALPHA100;
			m_Vertices[uVCount].tu = fMaxU;
			m_Vertices[uVCount++].tv = fMinV;

			m_Vertices[uVCount].x = vecCorner1.x;
			m_Vertices[uVCount].y = vecCorner1.y;
			m_Vertices[uVCount].z = vecCorner1.z;
			m_Vertices[uVCount].color = Color;
			m_Vertices[uVCount].specular = colorALPHA100;
			m_Vertices[uVCount].tu = fMinU;
			m_Vertices[uVCount++].tv = fMinV;

			m_Vertices[uVCount].x = vecCorner1.x;
			m_Vertices[uVCount].y = vecCorner1.y;
			m_Vertices[uVCount].z = vecCorner1.z;
			m_Vertices[uVCount].color = Color;
			m_Vertices[uVCount].specular = colorALPHA100;
			m_Vertices[uVCount].tu = fMinU;
			m_Vertices[uVCount++].tv = fMinV;

			m_Vertices[uVCount].x = vecCorner4.x;
			m_Vertices[uVCount].y = vecCorner4.y;
			m_Vertices[uVCount].z = vecCorner4.z;
			m_Vertices[uVCount].color = Color;
			m_Vertices[uVCount].specular = colorALPHA100;
			m_Vertices[uVCount].tu = fMinU;
			m_Vertices[uVCount++].tv = fMaxV;

			m_Vertices[uVCount].x = vecCorner3.x;
			m_Vertices[uVCount].y = vecCorner3.y;
			m_Vertices[uVCount].z = vecCorner3.z;
			m_Vertices[uVCount].color = Color;
			m_Vertices[uVCount].specular = colorALPHA100;
			m_Vertices[uVCount].tu = fMaxU;
			m_Vertices[uVCount++].tv = fMaxV;

		}
		if (uVCount > 0)
		{
			if (!piRenderScene(piPT_TRIANGLELIST, piVT_LVERTEX,
								&m_Vertices, uVCount))
			{
				return(FALSE);
			}
			// draw vertecies
			uVCount = 0;
		}

		if (PassNum == 2 || PassNum == 3)
		{
			piSetViewMatrix(&SavedViewMatrix);
		}

		if (PassNum == 0 || PassNum == 2)
		{
			piSetRenderState(piRENDERSTATE_SRCBLEND, SrcBlend);
			piSetRenderState(piRENDERSTATE_DESTBLEND, DestBlend);
			piSetRenderState(piRENDERSTATE_ALPHAREF, piALPHAREF_DEFAULT);
		}

		piSetRenderState(piRENDERSTATE_DESTBLEND, DestBlend);
		LastParticleWasGlow = FALSE;
	}

	piSetRenderState(piRENDERSTATE_ZBIAS, 0);
    piSetRenderState(piRENDERSTATE_LIGHTING, piLIGHTING_DEFAULT);

	piSetRenderStageState(0, piTSS_MIPFILTER, TextureMipFilter);
	piSetWorldMatrix(&OrigWorldMat);

	return TRUE;
}


// particles 0-7 occupy the top half of the texture map
// with 0-3 being top row, 4-7 being next
//
// particles 8-39 occupy the bottom half,
// with 8-15 being top row, 16-23 being next, 24-31 next, followed by 32-39
//
// all are left to right
LOGICAL CFXParticle::GetUVMinMax(FLOAT32* Umin,
								 FLOAT32* Vmin,
								 FLOAT32* Umax,
								 FLOAT32* Vmax,
								 UINT32 InParticleNum)
{
UINT32 X,Y;
UINT32 ParticleNum;

	ParticleNum = InParticleNum & 0xff;

	if (ParticleNum < 8)
	{
		// we're a large particle
		X = ParticleNum & 3;	// Fast mod 4
		Y = ParticleNum / 4;

		*Umin = X * 0.25f;
		*Umax = *Umin + 0.25f;

		*Vmax = 1.0f - (Y * 0.25f);
		*Vmin = *Vmax - 0.25f;
	}
	else
	{
		X = ParticleNum & 7;	// Fast mod 4
		Y = (ParticleNum - 8) / 8;

		*Umin = X * 0.125f;
		*Umax = *Umin + 0.125f;

		*Vmax = 1.0f - (Y * 0.125f + 0.5f);
		*Vmin = *Vmax - 0.125f;
	}
	return TRUE;
}


LOGICAL CFXParticle::SortDisplayList(PARTICLEINFO arySort[DEF_MAX_PARTICLES], SINT32* pCount)
{
    PARTICLE* pPart = m_Head.next;
    SINT32 iCount = 0;

    if(pPart == NULL)
        return FALSE;

    while(pPart != &m_Tail)
    {
        arySort[iCount].iIndex = pPart->iIndex;
        arySort[iCount].fDistance = pPart->DisplayDist;

        iCount++;

        pPart = pPart->next;
    }

    if(iCount > 0)
    {
        qsort(arySort, iCount, sizeof(PARTICLEINFO), _SortCompare);
    }

    *pCount = iCount;

    return TRUE;
}

SINT32 _SortCompare(const void* pObj1, const void* pObj2)
{
    PARTICLEINFO* pLeft = NULL;
    PARTICLEINFO* pRight = NULL;

    pLeft = (PARTICLEINFO*)pObj1;
    pRight = (PARTICLEINFO*)pObj2;

    if(pLeft->fDistance < pRight->fDistance)
        return 1;

    if(pLeft->fDistance > pRight->fDistance)
        return -1;

    return 0;
}

LOGICAL CFXParticle::GetNewParticle(PARTICLE ** pReturnParticle, LOGICAL ParticleIsPriority)
{
PARTICLE * pParticle;

	if(pReturnParticle == NULL)
	{
		return FALSE;
	}

	// if there are insufficient particles left and this new particle doesn't have priority
	// then return no priority
	if (ParticleIsPriority == FALSE)
	{
		if(DEF_MAX_PARTICLES - Count < NUM_PRIORITY_RESERVED_PARTICLES)
		{
			*pReturnParticle = NULL;
			return FALSE;
		}
	}

	pParticle = m_FreeHead.next;

    if(pParticle ==  NULL)
    {
        *pReturnParticle = NULL;
        return FALSE;
    }

    if(pParticle == &m_FreeTail)
    {
        *pReturnParticle = NULL;
        return FALSE;
    }

    PARTICLE* pHead = &m_FreeHead;
    PARTICLE* pNext = pParticle->next;

    pHead->next = pNext;
    pNext->prev = pHead;

    pParticle->next = NULL;
    pParticle->prev = NULL;

    AddToActiveList(pParticle);

	*pReturnParticle = pParticle;

	return TRUE;
}

LOGICAL CFXParticle::CreateNewParticle(const FLOAT64VECTOR* fvectorPos,
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
	return CreateUserDrawParticle(fvectorPos,
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
							EmitterObject,
							NULL);
}


LOGICAL CFXParticle::CreateUserDrawParticle(const FLOAT64VECTOR* fvectorPos,
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
    PARTICLE* p = NULL;

	if (fvectorPos == NULL)
	{
		return FALSE;
	}

    if(vecVel)
    {
        if(_isnan(vecVel->x) || _isnan(vecVel->y) || _isnan(vecVel->z))
        {
            return FALSE;
        }
    }

	if (uFlags & PARTICLE_FLAG_PRIORITY)
	{
		GetNewParticle(&p,TRUE);
	}
	else
	{
		GetNewParticle(&p,FALSE);
	}

	// not getting a particle isn't an error, but we can't proceed unless we get one
	if (p == NULL)
	{
		return TRUE;
	}

	p->uFlags = uFlags;

	p->OrigPos = p->fvectorPosition = *fvectorPos;
	if (vecVel != NULL)
	{
		p->vecVelocity = *vecVel;
	}
	p->fDragCo = fDragCo;
	if (pimatOrientation != NULL)
	{
		p->uFlags |= PARTICLE_FLAG_USESORIENTATIONMAT;
//		pParticle->pimatAttMat = *pimatOrientation;
	}
	else
	{
		p->uFlags &= ~(PARTICLE_FLAG_USESORIENTATIONMAT);
	}

	p->fAlpha[0] = fInitAlpha;
	p->fAlpha[1] = fP1Alpha;
	p->fAlpha[2] = fP2Alpha;
	p->fAlpha[3] = fEndAlpha;
	p->fSize[0] = fInitSize;
	p->fSize[1] = fP1Size;
	p->fSize[2] = fP2Size;
	p->fSize[3] = fEndSize;
	p->iTime[0] = iStartTime;
	p->iTime[1] = iP1Time;
	p->iTime[2] = iP2Time;
	p->iTime[3] = iEndTime;

	p->StartAlt = (FLOAT32)fvectorPos->z;

	p->fUseAlpha = fInitAlpha;
	p->fUseSize = fInitSize;

	p->pAttachObject = EmitterObject;
	if (p->pAttachObject != NULL)
	{
        EmitterObject->flag2 |= OBJECT_HAS_FX;
		p->uFlags |= PARTICLE_FLAG_ATTACHED_TO_OBJECT;
	}

	p->pParticleLastUpdateTime = iStartTime;

	p->UserDraw = UserDraw;

	if (vecRotation != NULL)
	{
		p->uFlags |= PARTICLE_FLAG_USESROTATION;
		p->vecRotAtt = *vecRotation;
	}
	else
	{
		p->uFlags &= ~(PARTICLE_FLAG_USESROTATION);
	}

	// now select the object, if applicable
	if (p->uFlags & PARTICLE_FLAG_USESOBJECT)
	{
		// fill in stuff here later
	}
	else
	{
		p->uParticleNum = UserData & 0xff;
//		if (uObjectType & 0xff00 != 0)
//		{
//			pParticle->uFlags |= PARTICLE_FLAG_GLOWS;
//		}
	}

	if (p->uFlags & PARTICLE_FLAG_USE_VEL_FOR_STRETCH)
	{
		p->StretchLength = (FLOAT32)mathExactVectorLength(&p->vecVelocity);

        // Prevent 'tornado' variables
        if(p->StretchLength > 0.00001f)
        {
            p->vecVelocity.x = (FLOAT32)(p->vecVelocity.x / p->StretchLength);
		    p->vecVelocity.y = (FLOAT32)(p->vecVelocity.y / p->StretchLength);
		    p->vecVelocity.z = (FLOAT32)(p->vecVelocity.z / p->StretchLength);
        }
	}

	return TRUE;
}

LOGICAL CFXParticle::KillParticle(PARTICLE * pParticle)
{

	AddToFreeList(pParticle);

	return TRUE;
}

LOGICAL CFXParticle::DetachFXFromObject(OBJECT * Object)
{
PARTICLE * Particle;
PARTICLE * NextParticle;

	if (Object == NULL || m_Head.next == NULL)
	{
		return TRUE;
	}

	for (Particle = m_Head.next; Particle != NULL;)
	{
		NextParticle = Particle->next;

		if (Particle->uFlags & PARTICLE_FLAG_ATTACHED_TO_OBJECT &&
			Particle->pAttachObject == Object)
		{
			KillParticle(Particle);
		}

		Particle = NextParticle;
	}

	return TRUE;
}



