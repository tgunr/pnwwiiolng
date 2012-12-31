#pragma once

#include "pi/pi.h"

#define DEF_MAX_PARTICLES               4096
#define NUM_PRIORITY_RESERVED_PARTICLES 512

// several elements in the particle struct are arrays, the following defines
// specify what each means
#define FXPARTICLE_INITPOS 0			// initial value at time zero
#define FXPARTICLE_GAINMAXPOS 1			// value after gain phase complete
#define FXPARTICLE_SUSTAINMAXPOS 2		// value after sustain phase complete
#define FXPARTICLE_DECAYMAXPOS 3		// value after decay phase complete

struct PARTICLEINFO
{
    FLOAT64 fDistance;
    SINT32 iIndex;

} ;

#define NUM_SINTABLE_ELEMENTS 1024 // sin/cos quick lookup table number of elements
//#define RAD_TO_SINTABLE_UNITS 1024.0f / PI / 2.0f // precalced lookup table converter
#define RAD_TO_SINTABLE_UNITS 162.9746617f // calculated out because mvc 6.0 doesn't optimize macros

struct PARTICLE
{
	PARTICLE* next;
	PARTICLE* prev;

	FLOAT64VECTOR	fvectorPosition; // position of the particle
	FLOAT64VECTOR	OrigPos; // original pos
	FLOAT64			DisplayDist; // distance to particle, squared
	piVECTOR		vecVelocity; // velocity of the particle
	FLOAT32			StretchVel; // used for gravity/rising effects on stretched particles
	FLOAT32			StretchLength; // stretch length for stretching particle
	FLOAT32			fDragCo; // drag coefficient of particle, if applicable
	FLOAT32			StartAlt; // starting altitude;
//	piMATRIX		pimatAttMat; // attitude matrix for particle
	piVECTOR		vecRotAtt; // the rotation speed for the particle
	FLOAT32			fAlpha[4]; // alpha for the different stages
	FLOAT32			fSize[4]; // size of the particle for the different stages
	SINT32			iTime[4]; // the time at which each stage occurs, in frametime
	FLOAT32			fUseSize; // the current size
	FLOAT32			fUseAlpha; // the current alpha
	struct OBJECT*	pObjPtr; // the base object to use
	struct OBJECT*	pAttachObject; // the object to spawn the particle from, if applicable
	UINT32			uParticleNum; // the particle number to use
	UINT32			uFlags;

	LOGICAL (*UserDraw)(FLOAT64VECTOR * Pos,
						FLOAT64 CurrentAlpha,
						FLOAT64 CurrentSize,
						UINT32 UserDraw); // particle user-draw function call

	SINT32			pParticleLastUpdateTime; // last time particle was updated

    SINT32 iIndex;

} ;

class CFXParticle
{
public:
	CFXParticle();
	~CFXParticle();

public:
	LOGICAL Initialize();
//    LOGICAL Create(SINT32) {return TRUE;}
    LOGICAL KillAll();
	LOGICAL Update();

	LOGICAL DisplayParticles(SINT32 DrawCockpitParticles);

	LOGICAL AddToFreeList(PARTICLE * pParticle);
	LOGICAL AddToActiveList(PARTICLE * pParticle);
	LOGICAL RemoveFromList(PARTICLE * pParticle);
	LOGICAL GetNewParticle(PARTICLE ** pReturnParticle,LOGICAL ParticleIsPriority);

	LOGICAL CreateNewParticle(const FLOAT64VECTOR* fvectorPos,
							   struct piVECTOR* vecVel,
							   FLOAT32 fDragCo,
							   union piMATRIX * pimatOrientation,
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
							   struct piVECTOR* vecRotation,
							   UINT32 uFlags,
							   struct OBJECT* EmitterObject);
	LOGICAL CreateUserDrawParticle(const FLOAT64VECTOR* fvectorPos,
						   struct piVECTOR* vecVel,
						   FLOAT32 fDragCo,
						   union piMATRIX * pimatOrientation,
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
						   struct piVECTOR * vecRotation,
						   UINT32 uFlags,
						   struct OBJECT* EmitterObject,
						   LOGICAL (*UserDraw)(FLOAT64VECTOR * Pos,
									FLOAT64 CurrentAlpha,
									FLOAT64 CurrentSize,
									UINT32 UserData));

	LOGICAL KillParticle(PARTICLE * pParticle);

	LOGICAL SortDisplayList(PARTICLEINFO aryInfo[DEF_MAX_PARTICLES], SINT32* pCount);
	LOGICAL GetUVMinMax(FLOAT32* Umin,FLOAT32* Vmin,FLOAT32* Umax,FLOAT32* Vmax,UINT32 InParticleNum);

	LOGICAL DetachFXFromObject(struct OBJECT * Object);

private:
    struct OBJECT* m_lpDummyObject;
    struct OBJECT* m_lpObject;

    PARTICLE m_Particles[DEF_MAX_PARTICLES];
	PARTICLE m_Head;
    PARTICLE m_Tail;
	PARTICLE m_FreeHead;
    PARTICLE m_FreeTail;

	PARTICLE m_ParticleDisplayHead;

	piLVERTEX m_Vertices[512];
	UINT32 Count;

	SINT32 iParticleTexId;		// contains the texture id for the particle system texture
	SINT32 iParticleTexIdGlow;	// contains the texture id for the particle system texture, glowing particles

	SINT32 iLastUpdateTime;
};
