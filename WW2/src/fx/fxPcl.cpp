#include <syscfg.h> // Path is project specific.
///////////////////////////////////////////////////
// Module: fxPcl.cpp
// Description: Handles 3d particles.
///////////////////////////////////////////////////
#include "fx/fxPcl.h"
#include "fx/fxSnd.h"
#include "fx/fxUtl.h"
#include "fx/fxLgt.h"

#include "math/math3d.h"
#include "math/mathvect.h"
#include "math/mathnum.h"
#include "include/vars.h"
#include "rn/rnvis.h"
#include "ogrp/ogrptype.h"
#include "ob/obdraw.h"
#include "mem/mem.h"
#include "trn/trnmain.h"
#include "trn/trnvars.h"
#include "fx/TransformEffect.h"

#include <vector>

static std::multimap<void* , SINT32> g_mappedObjects;
static std::multimap<void* , SINT32> g_mappedVehicles;

static std::vector<SINT32> g_v3Ddraw;

#define DEF_FXPCL_MAX_PARTICLES     1024
#define DEF_FXPCL_MAX_PARTS         64

static FXPARTICLE* particles = nullptr;

static std::deque<int> freelist;

static FXPARTICLE g_pclHead;
static FXPARTICLE g_pclTail;

static piCOLORVALUE* g_lpColor = nullptr;

//static FLOAT32 g_fPi = PI;
static FLOAT32 g_fPiOver180 = PI / 180.0f;

static piMATRIX* g_lpInvertView = nullptr;

static FXINITFUNC g_fxInitFuncs[IDE_NUMBER_OF_PARTICLES];
static FXUPDATEFUNC g_fxUpdateFuncs[IDE_NUMBER_OF_PARTICLES];

extern LOGICAL fpFPSGetFirePoint(OBJECT* pObject, piMATRIX* pMatrix, bool visual);
extern LOGICAL gr2AnimGetFirePointMatrix(OBJECT* pObject, piMATRIX* pMatrix, bool visual);

/////////////////////////
// static void functions
/////////////////////////
static void _ApplyAmmo(FXPARTICLE* lpPcl);
static void _ApplyDOF(FXPARTICLE* lpPcl);
static void _FaceMe(piVECTOR vecOut[4], piVECTOR vecIn[4], FLOAT32 fScale, piVECTOR* pPos);
static void _InitVertices(FXPARTICLE* lpPcl, FLOAT32 fHalfRadius);
static void _SetInitFuncs();
#if(0)
static void _SetQuadDefaultValues(FXPARTICLE* lpPcl, SINT32 id, FLOAT32 fTimeOffset);
#endif
static void _SetUpdateFuncs();
static void _UpdateAlpha(FXPARTICLE* lpPcl);
static void _UpdateColor(FXPARTICLE* lpPcl);
static void _UpdateDebris(FXPARTICLE* lpPcl);
static void _UpdateDrag(FXPARTICLE* lpPcl);
static void _UpdateFireFountain(FXPARTICLE* lpPcl);
static void _UpdateFireTrail(FXPARTICLE* lpPcl);
static void _UpdateMuzzleFlash(FXPARTICLE* lpPcl);
static void _UpdateRadius(FXPARTICLE* lpPcl);
static void _UpdateParticle(FXPARTICLE* lpPcl);
static void _UpdateShells(FXPARTICLE* lpPcl);
static void _UpdateSmokeFountain(FXPARTICLE* lpPcl);
static void _UpdateSmokeTrail(FXPARTICLE* lpPcl);
static void _UpdateTrail(FXPARTICLE* lpPcl);
static void _UpdateVaporTrail(FXPARTICLE* lpPcl);
static void _updateVehicleFlash(FXPARTICLE* lpPcl);
static void _UpdateVelocity(FXPARTICLE* lpPcl);
static void _updateTroopFlash(FXPARTICLE* p);

////////////////////////////
// static LOGICAL functions
////////////////////////////
static LOGICAL _AddParticle(FXPARTICLE* lpPcl);
static LOGICAL _CanDraw(FXPARTICLE* lpPcl);
static LOGICAL _CheckForParticle();
static LOGICAL _CreateSmoke(FXPARTICLE* lpPcl, FLOAT32 fTimeOffset);
static LOGICAL _CreateFire(FXPARTICLE* lpPcl, FLOAT32 fTimeOffset);
static LOGICAL _CreateNewTrail(FXPARTICLE* lpPcl);
static LOGICAL _DrawDebris(FXPARTICLE* lpPcl);
static LOGICAL _DrawLVertices(FXPARTICLE* lpPcl);
#if(0)
static LOGICAL _DrawMuzzleFlash(FXPARTICLE* lpPcl);
#endif
static LOGICAL _DrawShells(FXPARTICLE* lpPcl);
static LOGICAL _DrawVaporTrail(FXPARTICLE* lpPcl);
static LOGICAL _FreeParticle(FXPARTICLE* lpPcl);
static LOGICAL _FreeParticleIfNecessary(FXPARTICLE* lpPcl);
static LOGICAL _GetParticle(FXPARTICLE** lpPcl);
static LOGICAL _ReleaseParticle(FXPARTICLE* lpPcl);
static LOGICAL _UpdatePCL(FXPARTICLE* lpPcl);
static bool _InitObjectParams(FXPARTICLE* lpPcl);
static bool _InitShells(FXPARTICLE* lpPcl);
static bool _InitDebris(FXPARTICLE* lpPcl);
static bool _InitFireTrail(FXPARTICLE* lpPcl);
static bool _InitSmokeTrail(FXPARTICLE* lpPcl);
static bool _InitMuzzleFlash(FXPARTICLE* lpPcl);
static bool _InitFireFountain(FXPARTICLE* lpPcl);
static bool _InitSmokeFountain(FXPARTICLE* lpPcl);
static bool _InitVaporTrail(FXPARTICLE* lpPcl);
static bool _InitCallback(FXPARTICLE* lpPcl);

#define USE_FX2 1

//////////////////////////////////////////
// Function: fxPclInit
// Description:
//////////////////////////////////////////
LOGICAL fxPclInit()
{
    SINT32 i;

    particles = (FXPARTICLE*)memAlloc(sizeof(FXPARTICLE) * DEF_FXPCL_MAX_PARTICLES);

    if(particles == nullptr)
        return FALSE;

//	Uneccessary -- memAlloc == calloc
//    memset(particles, 0, sizeof(FXPARTICLE) * DEF_FXPCL_MAX_PARTICLES);
    memset(g_fxInitFuncs, 0, sizeof(FXINITFUNC) * IDE_NUMBER_OF_PARTICLES);
    memset(g_fxUpdateFuncs, 0, sizeof(FXUPDATEFUNC) * IDE_NUMBER_OF_PARTICLES);

    memset(&g_pclHead, 0, sizeof(FXPARTICLE));
    memset(&g_pclTail, 0, sizeof(FXPARTICLE));

    g_pclHead.next = &g_pclTail;
    g_pclTail.prev = &g_pclHead;

    for(i = 0; i < DEF_FXPCL_MAX_PARTICLES; i++)
    {
        particles[i].ID = i + 1;
        _FreeParticle(&particles[i]);
    }

    _SetInitFuncs();
    _SetUpdateFuncs();

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _SetInitFuncs
// Description:
//////////////////////////////////////////////////////////////////////
void _SetInitFuncs()
{
    g_fxInitFuncs[IDE_PARTICLE_SMOKETRAIL].pFn = _InitSmokeTrail;
    g_fxInitFuncs[IDE_PARTICLE_DEBRIS].pFn = _InitDebris;
    g_fxInitFuncs[IDE_PARTICLE_FIRETRAIL].pFn = _InitFireTrail;
    g_fxInitFuncs[IDE_PARTICLE_SHELLS].pFn = _InitShells;
    g_fxInitFuncs[IDE_PARTICLE_MUZZLEFLASH].pFn = _InitMuzzleFlash;
    g_fxInitFuncs[IDE_PARTICLE_FIRE_FOUNTAIN].pFn = _InitFireFountain;
    g_fxInitFuncs[IDE_PARTICLE_SMOKE_FOUNTAIN].pFn = _InitSmokeFountain;
    g_fxInitFuncs[IDE_PARTICLE_VAPOR_TRAIL].pFn = _InitVaporTrail;
    g_fxInitFuncs[IDE_PARTICLE_CALLBACK].pFn = _InitCallback;
}

//////////////////////////////////////////////////////////////////////
// Function: _SetUpdateFuncs
// Description:
//////////////////////////////////////////////////////////////////////
void _SetUpdateFuncs()
{
    g_fxUpdateFuncs[IDE_PARTICLE_SMOKETRAIL].pFn = _UpdateSmokeTrail;
    g_fxUpdateFuncs[IDE_PARTICLE_DEBRIS].pFn = _UpdateDebris;
    g_fxUpdateFuncs[IDE_PARTICLE_FIRETRAIL].pFn = _UpdateFireTrail;
    g_fxUpdateFuncs[IDE_PARTICLE_SHELLS].pFn = _UpdateShells;
    g_fxUpdateFuncs[IDE_PARTICLE_MUZZLEFLASH].pFn = _UpdateMuzzleFlash;
    g_fxUpdateFuncs[IDE_PARTICLE_FIRE_FOUNTAIN].pFn = _UpdateFireFountain;
    g_fxUpdateFuncs[IDE_PARTICLE_SMOKE_FOUNTAIN].pFn = _UpdateSmokeFountain;
    g_fxUpdateFuncs[IDE_PARTICLE_VAPOR_TRAIL].pFn = _UpdateVaporTrail;
}

//////////////////////////////////////////
// Function: fxPclCreateSpark
// Description:
//////////////////////////////////////////
LOGICAL fxPclCreateSpark(FLOAT64VECTOR* pPos,
                         piVECTOR* pVel,
                         FLOAT32 fRadius,
                         FLOAT32 fTime,
                         UINT32 uFlags,
                         LOGICAL wantStretch,
                         LOGICAL fixStretch,
                         FLOAT32 drag)
{
#if(0)
    SINT32 ID = fxQuadGetNewParticle();

    if(ID == -1) return FALSE;

    uFlags |= DEF_FXFLAG_GLOW;
    uFlags |= DEF_FXFLAG_MYCOLOR;

    // Spark is number 29 on the sprite texture
    fxQuadSetSpriteNumber(ID, 29);
    fxQuadSetPt1Color(ID, 1.0f, 1.0f, 1.0f);
    fxQuadSet4PtAlpha(ID, 1.0f, 1.0f, 1.0f, 1.0f);
    fxQuadPushFlags(ID, uFlags);
    fxQuadSetPosition(ID, pPos->x, pPos->y, pPos->z);
    fxQuadSetVelocity(ID, pVel->x, pVel->y, pVel->z);
    fxQuadSetRadius(ID, fRadius);
    fxQuadSetScale(ID, 1.0f);
    fxQuadSetPt1Time(ID, (FLOAT32)frametime);
    fxQuadSetPt4Time(ID, (FLOAT32)frametime + fTime);
    fxQuadSetDragCoeff(ID, drag);
    fxQuadSetCanStretch(ID, wantStretch);

    if(fixStretch)
        fxQuadFixStretch(ID);

    // Required after settings
    fxQuadInitParticle(ID);
#endif
    return TRUE;
}

//////////////////////////////////////////
// Function: fxPclCreateExplosion
// Description:
//////////////////////////////////////////
LOGICAL fxPclCreateExplosion(FLOAT64VECTOR* pPos, FLOAT64 energy)
{
    Vec3d n(0, 0, -1);
    Vec3d pos(*pPos);
    pos.z() = -pos.z();

    // Not using energy for determining size, since our energy
    // values go infinite on certain occasions.

    TransformEffect* te = nullptr;

    if(energy > 300000000.0f)
    {
        te = TransformEffect::AddEffect("genericlargeexplosion", n, 0, nullptr);
    }
    else if(energy > 100000000.0f)
    {
        te = TransformEffect::AddEffect("genericmediumexplosion", n, 0, nullptr);
    }
    else if(energy > 1000000.0f)
    {
        te = TransformEffect::AddEffect("genericsmallexplosion", n, 0, nullptr);
    }
    else
    {
        te = TransformEffect::AddEffect("smallcannon", n, 0, nullptr);
    }

    if(te)
    {
        te->enableemitters(true);;
        te->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);
        te->setmatrix(Matrixf::translate(pos));
    }

    return TRUE;
}

//////////////////////////////////////////
// Function: fxPclCreateH2OExplosion
// Description:
//////////////////////////////////////////
LOGICAL fxPclCreateH2OExplosion(FLOAT64VECTOR* pPos, FLOAT64 fEnergy)
{
    Vec3f n(0, 0, -1);
    Vec3d p(*pPos);
    p.z() = -p.z();

    TransformEffect* te = nullptr;

    if(fEnergy > 38000000.0f)
    {
        te = TransformEffect::AddEffect("water_genericlargeexplosion", n, 0, nullptr);
    }
    else if(fEnergy > 10000000.0f)
    {
        te = TransformEffect::AddEffect("water_genericmediumexplosion", n, 0, nullptr);
    }
    else if(fEnergy > 100000.0f)
    {
        te = TransformEffect::AddEffect("water_genericsmallexplosion", n, 0, nullptr);
    }
    else
    {
        te = TransformEffect::AddEffect("water_smallcannon", n, 0, nullptr);
    }

    if(te)
    {
        te->enableemitters(true);;
        te->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);
        te->setmatrix(Matrixf::translate(p));
    }

    return TRUE;
}


//////////////////////////////////////////
// Function: _InitShells
// Description:
//////////////////////////////////////////
bool _InitShells(FXPARTICLE* lpPcl)
{
    // Particle is an OBJECT*
    if(lpPcl->uFlags & DEF_FXFLAG_OBJECT)
    {
        if(!_InitObjectParams(lpPcl))
        {
            return false;
        }
    }
    else
    {
        FLOAT32 fHalfRadius = lpPcl->fRadius * 0.5f * lpPcl->fScale;
        _InitVertices(lpPcl, fHalfRadius);

        lpPcl->vecPcl[0].y *= 2.0f;
        lpPcl->vecPcl[1].y *= 2.0f;
        lpPcl->vecPcl[2].y *= 2.0f;
        lpPcl->vecPcl[3].y *= 2.0f;
    }

    fxUtlUpdatePosition(&lpPcl->vecPt, &lpPcl->vecPos);
    fxUtlGetGroundPos(&lpPcl->vecPt, &lpPcl->vecOrig);

    return true;
}

//////////////////////////////////////////
// Function: _InitDebris
// Description:
//////////////////////////////////////////
bool _InitDebris(FXPARTICLE* lpPcl)
{
    // Particle is an OBJECT*
    if(lpPcl->uFlags & DEF_FXFLAG_OBJECT)
    {
        if(!_InitObjectParams(lpPcl))
        {
            return false;
        }
    }

    return true;
}

//////////////////////////////////////////
// Function: _InitCallback
// Description:
//////////////////////////////////////////
bool _InitCallback(FXPARTICLE* lpPcl)
{
    return true;
}

//////////////////////////////////////////
// Function: _InitVaporTrail
// Description:
//////////////////////////////////////////
bool _InitVaporTrail(FXPARTICLE* lpPcl)
{
    lpPcl->vecPcl[0].x = -lpPcl->fRadius * 0.25f;
    lpPcl->vecPcl[0].y =  0.0f;
    lpPcl->vecPcl[0].z =  0.0f;

    lpPcl->vecPcl[1].x = -lpPcl->fRadius * 0.25f;
    lpPcl->vecPcl[1].y =  lpPcl->fRadius;
    lpPcl->vecPcl[1].z =  0.0f;

    lpPcl->vecPcl[2].x = lpPcl->fRadius * 0.25f;
    lpPcl->vecPcl[2].y = lpPcl->fRadius;
    lpPcl->vecPcl[2].z = 0.0f;

    lpPcl->vecPcl[3].x = lpPcl->fRadius * 0.25f;
    lpPcl->vecPcl[3].y = 0.0f;
    lpPcl->vecPcl[3].z = 0.0f;

    return true;
}

//////////////////////////////////////////
// Function: _InitFireTrail
// Description:
//////////////////////////////////////////
bool _InitFireTrail(FXPARTICLE* lpPcl)
{
    fxUtlUpdatePosition(&lpPcl->vecPt, &lpPcl->vecPos);
    fxUtlGetGroundPos(&lpPcl->vecPt, &lpPcl->vecOrig);
    lpPcl->fLastQuadCreate = (FLOAT32)frametime;
    _InitVertices(lpPcl, lpPcl->fRadius * 0.5f);

    return true;
}

//////////////////////////////////////////
// Function: _InitFireFountain
// Description:
//////////////////////////////////////////
bool _InitFireFountain(FXPARTICLE* lpPcl)
{
    lpPcl->uFlags |= DEF_FXFLAG_NOPHYSICS;
    fxUtlUpdatePosition(&lpPcl->vecPt, &lpPcl->vecPos);
    lpPcl->fLastQuadCreate = (FLOAT32)frametime;
    _InitVertices(lpPcl, lpPcl->fRadius * 0.5f);

    return true;
}

//////////////////////////////////////////
// Function: _InitSmokeFountain
// Description:
//////////////////////////////////////////
bool _InitSmokeFountain(FXPARTICLE* lpPcl)
{
    lpPcl->uFlags |= DEF_FXFLAG_NOPHYSICS;

    fxUtlUpdatePosition(&lpPcl->vecPt, &lpPcl->vecPos);

    if(lpPcl->uFlags & DEF_FXFLAG_STOPATGROUND)
    {
        fxUtlGetGroundPos(&lpPcl->vecPt, &lpPcl->vecOrig);
    }

    _InitVertices(lpPcl, lpPcl->fRadius * 0.5f);
    lpPcl->fLastQuadCreate = (FLOAT32)frametime;

    return true;
}

//////////////////////////////////////////
// Function: _InitSmokeTrail
// Description:
//////////////////////////////////////////
bool _InitSmokeTrail(FXPARTICLE* lpPcl)
{
    fxUtlUpdatePosition(&lpPcl->vecPt, &lpPcl->vecPos);

    if(lpPcl->uFlags & DEF_FXFLAG_STOPATGROUND)
    {
        fxUtlGetGroundPos(&lpPcl->vecPt, &lpPcl->vecOrig);
    }

    lpPcl->fLastQuadCreate = (FLOAT32)frametime;
    _InitVertices(lpPcl, lpPcl->fRadius * 0.5f);

    return true;
}

//////////////////////////////////////////
// Function: _InitMuzzleFlash
// Description:
//////////////////////////////////////////
bool _InitMuzzleFlash(FXPARTICLE* lpPcl)
{
    if(lpPcl->lpFxObj)
    {
        if(!_InitObjectParams(lpPcl))
        {
            return false;
        }
    }

    // Get latest DOF position
	_UpdateMuzzleFlash(lpPcl);

    return true;
}

//////////////////////////////////////////
// Function: _InitObjectParams
// Description:
//////////////////////////////////////////
bool _InitObjectParams(FXPARTICLE* lpPcl)
{
    piMATRIX matOrient = IdentityMatrix;

    float dot = math3dDotProduct(&lpPcl->vecDir, &lpPcl->vecDir);

    if(dot > -0.0001f && dot < 0.0001f)
    {
        return false;
    }

    if(_isnan(dot) || !_finite(dot))
    {
        return false;
    }

    piVECTOR vecDir = lpPcl->vecDir;

    vecDir.z = -vecDir.z;

    mathVectorToYAxis(&vecDir, &matOrient);
    math3dInvertMatrix(&matOrient, &matOrient);

    piMATRIX matScale = IdentityMatrix;

    // In case user has rotated object prior to this call
    piMATRIX matObj = IdentityMatrix;

    if(lpPcl->lpFxObj)
    {
        matObj = lpPcl->lpFxObj->mat;
    }

    matObj._41 = 0.0f;
    matObj._42 = 0.0f;
    matObj._43 = 0.0f;

    matScale._11 = lpPcl->fScale;
    matScale._22 = lpPcl->fScale;
    matScale._33 = lpPcl->fScale;

    math3dMatrixMultiply(&matOrient, &matOrient, &matScale);
    math3dMatrixMultiply(&matObj, &matObj, &matOrient);

    if(lpPcl->lpFxObj)
    {
        fxUtlSetMatrixInTree(lpPcl->lpFxObj, &matObj);
    }

    lpPcl->matWorld = matObj;

    return true;
}

//////////////////////////////////////////////
// Function: _InitVertices
// Description:
//////////////////////////////////////////////
void _InitVertices(FXPARTICLE* lpPcl, FLOAT32 fHalfRadius)
{
    lpPcl->vecPcl[0].x = -fHalfRadius;
    lpPcl->vecPcl[0].y = -fHalfRadius;

    lpPcl->vecPcl[1].x = -fHalfRadius;
    lpPcl->vecPcl[1].y =  fHalfRadius;

    lpPcl->vecPcl[2].x =  fHalfRadius;
    lpPcl->vecPcl[2].y =  fHalfRadius;

    lpPcl->vecPcl[3].x =  fHalfRadius;
    lpPcl->vecPcl[3].y = -fHalfRadius;
}

////////////////////////////////////////////////////
// Function: fxPclFreeAll
// Description: Release all particles on game to UI
////////////////////////////////////////////////////
void fxPclFreeAll()
{
    FXPARTICLE* lpPcl = g_pclHead.next;
    FXPARTICLE* lpTail = &g_pclTail;
    FXPARTICLE* lpFree = nullptr;

    if(lpPcl == nullptr) return;

    while(lpPcl != lpTail)
    {
        lpFree = lpPcl;
        lpPcl = lpPcl->next;

        lpFree->bDraw = FALSE;

        _FreeParticleIfNecessary(lpFree);
    }
}

//////////////////////////////////////////
// Function: fxPclKill
// Description:
//////////////////////////////////////////
LOGICAL fxPclKill()
{
    if(particles != nullptr)
    {
        memFree(particles);
        particles = nullptr;
    }

    // Set to nullptr in case of further updates.
    memset(&g_pclHead, 0, sizeof(FXPARTICLE));
    memset(&g_pclTail, 0, sizeof(FXPARTICLE));

    return TRUE;
}

//////////////////////////////////////////
// Function: _FreeParticle
// Description:
//////////////////////////////////////////
LOGICAL _FreeParticle(FXPARTICLE* pcl)
{
    FXPARTICLE* lpPrev = nullptr;

    if(pcl == nullptr)
        return FALSE;

    std::multimap<void* , SINT32>::iterator I;

    if(pcl->lpEmitterObj)
    {
        I = g_mappedObjects.find(pcl->lpEmitterObj);

        if(I != g_mappedObjects.end())
        {
            g_mappedObjects.erase(I);
        }
    }

    if(pcl->lpVehicle)
    {
        I = g_mappedVehicles.find(pcl->lpVehicle);

        if(I != g_mappedVehicles.end())
        {
            g_mappedVehicles.erase(I);
        }
    }

    int id = pcl->ID;

    memset(pcl, 0, sizeof(FXPARTICLE));

    pcl->ID = id;

    freelist.push_back(id);

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _GetParticle
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL _GetParticle(FXPARTICLE** pcl)
{
    if(!_CheckForParticle())
        return FALSE;

    int id = freelist.front();
    freelist.pop_front();

    *pcl = &particles[id - 1];

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _CheckForParticle
// Description: Check for free particle. If there are none, take the oldest
// particle off the active list.
//////////////////////////////////////////////////////////////////////
LOGICAL _CheckForParticle()
{
    if(freelist.empty())
    {
        FXPARTICLE* lpPcl = nullptr;

        if(g_pclTail.prev == &g_pclHead)
            return FALSE;

        lpPcl = g_pclTail.prev;

        if(!_ReleaseParticle(lpPcl))
            return FALSE;

        if(!_FreeParticle(lpPcl))
            return FALSE;
    }

    if(freelist.empty())
        return FALSE;

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _ReleaseParticle
// Description: Release particle from active list...do before fxPclFreeParticle.
//////////////////////////////////////////////////////////////////////
LOGICAL _ReleaseParticle(FXPARTICLE* lpPcl)
{
    FXPARTICLE* lpNext = nullptr;
    FXPARTICLE* lpPrev = nullptr;

    if(lpPcl == nullptr)
        return FALSE;

    lpNext = lpPcl->next;
    lpPrev = lpPcl->prev;

    if(lpNext == nullptr || lpPrev == nullptr)
        return FALSE;

    lpPrev->next = lpNext;
    lpNext->prev = lpPrev;

    lpPcl->prev = nullptr;
    lpPcl->next = nullptr;

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _AddParticle
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL _AddParticle(FXPARTICLE* lpPcl)
{
    FXPARTICLE* lpNext = nullptr;

    if(lpPcl == nullptr)
        return FALSE;

    lpNext = g_pclHead.next;

    g_pclHead.next = lpPcl;
    lpPcl->prev = &g_pclHead;

    lpPcl->next = lpNext;
    lpNext->prev = lpPcl;

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: fxPclSetColor
// Description: Set color from outside module.
//////////////////////////////////////////////////////////////////////
void fxPclSetColor(piCOLORVALUE* lpColor)
{
    g_lpColor = lpColor;
}

//////////////////////////////////////////////////////////////////////
// Function: fxPclSetInvertView
// Description:
//////////////////////////////////////////////////////////////////////
void fxPclSetInvertView(piMATRIX* lpInvertView)
{
    g_lpInvertView = lpInvertView;
}

//////////////////////////////////////////
// Function: fxPclUpdate
// Description:
//////////////////////////////////////////
void fxPclUpdate()
{
    if(g_pclHead.next == nullptr) return;
    if(g_pclHead.next == &g_pclTail) return;

    FXPARTICLE* lpPcl = g_pclHead.next;
    FXPARTICLE* lpTail = &g_pclTail;

    while(lpPcl != lpTail)
    {
        if(lpPcl->lpEmitterObj)
        {
            if(g_mappedObjects.find(lpPcl->lpEmitterObj) == g_mappedObjects.end())
            {
                lpPcl->bDraw = FALSE;
            }
        }

        _UpdatePCL(lpPcl);

        if(lpPcl->pFnUserUpdate && lpPcl->bDraw)
        {
            lpPcl->pFnUserUpdate(lpPcl);
        }

        if(!lpPcl->bDraw)
        {
            FXPARTICLE* FREE = lpPcl;
            lpPcl = lpPcl->next;

            _FreeParticleIfNecessary(FREE);
            continue;
        }

        lpPcl = lpPcl->next;
    }
}

//////////////////////////////////////////
// Function: _UpdatePCL
// Description:
//////////////////////////////////////////
LOGICAL _UpdatePCL(FXPARTICLE* lpPcl)
{
    if(!lpPcl->bDraw)
        return FALSE;

    if(lpPcl->enumType <= IDE_PARTICLE_NOTUSED)
        return FALSE;

    if(lpPcl->enumType >= IDE_NUMBER_OF_PARTICLES)
        return FALSE;

    if(g_fxUpdateFuncs[lpPcl->enumType].pFn == nullptr)
        return FALSE;

    g_fxUpdateFuncs[lpPcl->enumType].pFn(lpPcl);

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _UpdateMuzzleFlash
// Description:
//////////////////////////////////////////////////////////////////////
void _UpdateMuzzleFlash(FXPARTICLE* lpPcl)
{
    if(lpPcl->lpFxObj->updatecallback)
    {
        lpPcl->lpFxObj->updatecallback(lpPcl->lpFxObj);

        if(!(lpPcl->uFlags & DEF_FXFLAG_INFINITE))
        {
            if((FLOAT32)frametime > lpPcl->pclTime.p4)
            {
                lpPcl->bDraw = FALSE;
            }
        }

        return;
    }

    vehVehicle*pVeh = lpPcl->lpVehicle;

    if(pVeh == nullptr)
        return;

    if(!pVeh->InUse)
        return;

    // Something bad has happened here.
    if(pVeh->VehicleType == nullptr)
    {
        lpPcl->bDraw = FALSE;
        return;
    }

    if(pVeh->VehicleType->Type == VEH_TYPE_PERSON)
    {
        _updateTroopFlash(lpPcl);
        return;
    }
    else
    {
        _updateVehicleFlash(lpPcl);
        return;
    }

    fxUtlUpdatePosition(&lpPcl->vecPt, &lpPcl->vecPos);

    if(lpPcl->lpFxObj)
    {
        piMATRIX matObj = lpPcl->lpFxObj->mat;

        matObj._41 = lpPcl->vecPt.x;
        matObj._42 = lpPcl->vecPt.y;
        matObj._43 = lpPcl->vecPt.z;

        fxUtlSetMatrixInTree(lpPcl->lpFxObj, &matObj);
    }
}

//////////////////////////////////////////////////////////////////////
// Function: _updateVehicleFlash
// Description:
//////////////////////////////////////////////////////////////////////
void _updateVehicleFlash(FXPARTICLE* p)
{
    OBJECT* o = p->lpFxObj;
    vehVehicle* veh = p->lpVehicle;

    if(veh == nullptr) return;
    if(!veh->InUse) return;

    if(veh->VehicleObjectData == nullptr) return;
    if(veh->VehicleObjectData->PlaneObject == nullptr) return;

    if((FLOAT32)frametime < p->pclTime.p1)
        return;

    if(!(p->uFlags & DEF_FXFLAG_INFINITE))
    {
        if((FLOAT32)frametime > p->pclTime.p4)
        {
            p->bDraw = FALSE;
            return;
        }
    }

    piVECTOR dir;
    FLOAT64VECTOR gunpos;

    piMATRIX m = IdentityMatrix;

    if(p->lpDOF)
    {
        piMATRIX mat;

        if(p->lpEmitterObj)
            mat = p->lpEmitterObj->mat;
        else
            mat = IdentityMatrix;

        // Get current direction of flash so drawing does not appear a frame behind.
        fxUtlGetDOFDirection(p->lpDOF, &mat, &dir);
        fxUtlGetDOFPos(p->lpDOF, &mat, &gunpos);

        // Create matrix to orient object to gun direction
        mathVectorToYAxis(&dir, &m);

        piMATRIX scale = IdentityMatrix;

        o->vel.set(dir.x, dir.y, dir.z);

        // Scale values saved in rot.x
        scale._11 = o->rot.x;
        scale._22 = o->rot.x;
        scale._33 = o->rot.x;

        // Scale orientation matrix
        math3dMatrixMultiply(&m, &m, &scale);

#if(1)
        // Random rotate around y
        piMATRIX roty = IdentityMatrix;

        roty._11 = cosf(o->rot.y);
        roty._13 = sinf(o->rot.y);
        roty._31 = -roty._13;
        roty._33 = roty._11;

        math3dMatrixMultiply(&m, &m, &roty);
#endif

        math3dInvertMatrix(&m, &m);

        // Translate to gun position.
        m._41 = float(gunpos.x - WorldOriginOffset.x);
        m._42 = float(gunpos.y - WorldOriginOffset.y);
        m._43 = float(-gunpos.z);

        // Apply matrix to object tree
        fxUtlSetMatrixInTree(o, &m);
    }

    p->vecPt.x = m._41;
    p->vecPt.y = m._42;
    p->vecPt.z = m._43;

    p->vecPos.x = m._41 + WorldOriginOffset.x;
    p->vecPos.y = m._42 + WorldOriginOffset.y;
    p->vecPos.z = -m._43;
}

//////////////////////////////////////////////////////////////////////
// Function: _UpdateDrag
// Description:
//////////////////////////////////////////////////////////////////////
void _UpdateDrag(FXPARTICLE* lpPcl)
{
    if(lpPcl->uFlags & DEF_FXFLAG_USEDRAG)
    {
        if(lpPcl->uFlags & DEF_FXFLAG_NOPHYSICS)
            return;

        fxUtlApplyDrag(&lpPcl->vecPVel, &lpPcl->fDragCo, &lpPcl->fLastUpdate);
    }
}

//////////////////////////////////////////////////////////////////////
// Function: _UpdateVelocity
// Description:
//////////////////////////////////////////////////////////////////////
void _UpdateVelocity(FXPARTICLE* lpPcl)
{
    if(lpPcl->uFlags & DEF_FXFLAG_NOPHYSICS)
        return;

    if(lpPcl->uFlags & DEF_FXFLAG_INTVELOCITY)
    {
        fxUtlInterpolateVector(&lpPcl->pclTime, &lpPcl->pclVel, &lpPcl->vecPVel);
        return;
    }

    fxUtlApplyVelocity(&lpPcl->vecPVel,
                       &lpPcl->vecPos,
                       &lpPcl->fLastUpdate,
                       lpPcl->uFlags);

    fxUtlUpdatePosition(&lpPcl->vecPt, &lpPcl->vecPos);
    lpPcl->fDistanceSqrd = fxUtlGetDistanceSqrd(&lpPcl->vecPt);
}

//////////////////////////////////////////////////////////////////////
// Function: _UpdateColor
// Description:
//////////////////////////////////////////////////////////////////////
void _UpdateColor(FXPARTICLE* lpPcl)
{
    if(lpPcl->uFlags & DEF_FXFLAG_INTCOLOR)
    {
        fxUtlInterpolateColor(&lpPcl->pclTime, &lpPcl->pclColor, &lpPcl->piColor);
    }
    else if(lpPcl->uFlags & DEF_FXFLAG_MYCOLOR)
    {
        lpPcl->piColor.a = lpPcl->pclColor.p1.a;
        lpPcl->piColor.r = lpPcl->pclColor.p1.r;
        lpPcl->piColor.g = lpPcl->pclColor.p1.g;
        lpPcl->piColor.b = lpPcl->pclColor.p1.b;
    }
    else
    {
        lpPcl->piColor.a = g_lpColor->a;
        lpPcl->piColor.r = g_lpColor->r;
        lpPcl->piColor.g = g_lpColor->g;
        lpPcl->piColor.b = g_lpColor->b;
    }
}

//////////////////////////////////////////
// Function: _UpdateAlpha
// Desctiption:
//////////////////////////////////////////
void _UpdateAlpha(FXPARTICLE* lpPcl)
{
    if(lpPcl->uFlags & DEF_FXFLAG_INTALPHA)
    {
        fxUtlInterpolateAlpha(&lpPcl->pclTime, &lpPcl->pclColor, &lpPcl->piColor);
    }
    else
    {
        lpPcl->piColor.a = 1.0f;
    }
}

//////////////////////////////////////////////////////////////////////
// Function: _UpdateRadius
// Description:
//////////////////////////////////////////////////////////////////////
void _UpdateRadius(FXPARTICLE* lpPcl)
{
    if(lpPcl->uFlags & DEF_FXFLAG_INTRADIUS)
    {
        fxUtlInterpolateRadius(&lpPcl->pclTime, &lpPcl->pclScale, &lpPcl->fScale);
    }
}

//////////////////////////////////////////////////////////////////////
// Function: _updateTroopFlash
// Description:
//////////////////////////////////////////////////////////////////////
void _updateTroopFlash(FXPARTICLE* p)
{
    piMATRIX firept = IdentityMatrix;

    OBJECT* o = p->lpFxObj;
    vehVehicle* veh = p->lpVehicle;

    if(veh == nullptr) return;
    if(!veh->InUse) return;

    if(veh->VehicleObjectData == nullptr) return;
    if(veh->VehicleObjectData->PlaneObject == nullptr) return;

    if((FLOAT32)frametime < p->pclTime.p1)
        return;

    if(!(p->uFlags & DEF_FXFLAG_INFINITE))
    {
        if((FLOAT32)frametime > p->pclTime.p4)
        {
            p->bDraw = FALSE;
            return;
        }
    }

    if(o->feature) // Using this variable to declare ghost rounds
    {
        gr2AnimGetFirePointMatrix(veh->VehicleObjectData->PlaneObject, &firept, true);
    }
    else
    {
        fpFPSGetFirePoint(veh->VehicleObjectData->PlaneObject, &firept, true);
    }

    FLOAT64VECTOR gunpos, player;

    // Get position at end of gun
    gunpos.x = firept._41;
    gunpos.y = firept._42;
    gunpos.z = firept._43;

    piMATRIX fireatt = IdentityMatrix;

    // Get player attitude and position
    if(veh->ModelData == nullptr)
    {
        fireatt = veh->PosData.AttMat;
        player = veh->PosData.Pos;
    }
    else
    {
        fireatt = veh->ModelData->VehicleAttMat;
        player = veh->ModelData->VehiclePos;
    }

    // Orient fire point to player attitude
    mathVectorMultiply64(&gunpos, &gunpos, &fireatt);
    // Transform to player pos
    mathAddVectors(&gunpos, &gunpos, &player);

    math3dMatrixMultiply(&firept, &fireatt, &firept);

    piVECTOR dir;
    // x and z reversed (whatever works)
    dir.set(-firept._12, firept._22, -firept._32);

    piMATRIX m = IdentityMatrix;

    // Create matrix to orient object to gun direction
    mathVectorToYAxis(&dir, &m);

    // Scale (values saved in rot.x)
    piMATRIX scale = IdentityMatrix;

    scale._11 = o->rot.x;
    scale._22 = o->rot.x;
    scale._33 = o->rot.x;

    // Scale orientation matrix
    math3dMatrixMultiply(&m, &m, &scale);

#if(1)
    // Random rotate around y
    piMATRIX roty = IdentityMatrix;

    roty._11 = cosf(o->rot.y);
    roty._13 = sinf(o->rot.y);
    roty._31 = -roty._13;
    roty._33 = roty._11;

    math3dMatrixMultiply(&m, &m, &roty);
#endif

    // Invert it so we can move object space flash to gun direction
    math3dInvertMatrix(&m, &m);

    // Translate to gun position.
    m._41 = float(gunpos.x - WorldOriginOffset.x);
    m._42 = float(gunpos.y - WorldOriginOffset.y);
    m._43 = float(-gunpos.z);

    // Apply matrix to object tree
    fxUtlSetMatrixInTree(o, &m);

    p->vecPt.x = m._41;
    p->vecPt.y = m._42;
    p->vecPt.z = m._43;

    p->vecPos.x = m._41 + WorldOriginOffset.x;
    p->vecPos.y = m._42 + WorldOriginOffset.y;
    p->vecPos.z = -m._43;
}

//////////////////////////////////////////////////////////////////////
// Function: _UpdateDebris
// Description:
//////////////////////////////////////////////////////////////////////
void _UpdateDebris(FXPARTICLE* lpPcl)
{
    if(lpPcl->uFlags & DEF_FXFLAG_OBJECT)
    {
        if(lpPcl->lpFxObj == nullptr)
            return;

        if(lpPcl->uFlags & DEF_FXFLAG_ROTATE)
            fxPclRotateObject(lpPcl);
    }
}

//////////////////////////////////////////////////////////////////////
// Function: _UpdateVaporTrail
// Description:
//////////////////////////////////////////////////////////////////////
void _UpdateVaporTrail(FXPARTICLE* lpPcl)
{
    if((FLOAT32)frametime < lpPcl->pclTime.p1)
        return;

    if(lpPcl->uFlags & DEF_FXFLAG_INFINITE)
    {
        _CreateNewTrail(lpPcl);
    }

    _UpdateParticle(lpPcl);
    _UpdateColor(lpPcl);
    _UpdateAlpha(lpPcl);
    _UpdateRadius(lpPcl);

    lpPcl->fLastUpdate = (FLOAT32)frametime;
}

//////////////////////////////////////////////////////////////////////
// Function: _CreateNewTrail
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL _CreateNewTrail(FXPARTICLE* lpPcl)
{
    FLOAT64VECTOR vecDiff;

    if(lpPcl->fSpacing <= 0.01f) return FALSE;

    mathSubVectors(&vecDiff, &lpPcl->vecPos, &lpPcl->vecLastPos);

    FLOAT32 fLength = (FLOAT32)mathExactVectorLength(&vecDiff);

    if(fLength < 0.01f) return FALSE;

    SINT32 iNumParticles = FloatToInt(fLength / lpPcl->fSpacing);

    if(iNumParticles <= 0)
    {
        iNumParticles = 1;
    }

    if(iNumParticles > 32) return FALSE;

    INTCOLOR* lpClr = &lpPcl->pclColor;
    INTFLOAT32* lpScl = &lpPcl->pclScale;

    FLOAT32 fTimeStep = 0.0f;
    FLOAT32 fDeltaTime = (FLOAT32)frametime - lpPcl->fLastUpdate;

    fTimeStep = fDeltaTime / (FLOAT32)iNumParticles;

    piVECTOR vecDir;

    vecDir.x = -(FLOAT32)vecDiff.x;
    vecDir.y = -(FLOAT32)vecDiff.y;
    vecDir.z =  (FLOAT32)vecDiff.z;

    piVECTOR vecUnit;

    vecUnit.x = (FLOAT32)vecDiff.x / fLength;
    vecUnit.y = (FLOAT32)vecDiff.y / fLength;
    vecUnit.z = (FLOAT32)vecDiff.z / fLength;

    piMATRIX matWorld = IdentityMatrix;

    FLOAT64VECTOR vecPos = lpPcl->vecPos;

    FLOAT32 fTimeOffset = 0.0f;

    mathVectorToYAxis(&vecDir, &matWorld);

    math3dInvertMatrix(&matWorld, &matWorld);

    piMATRIX matShear = IdentityMatrix;

    FLOAT32 fShear = fLength * lpPcl->fRadius;

    matShear._22 = fShear < 1.0f ? 1.0f : fShear;

    math3dMatrixMultiply(&matWorld, &matShear, &matWorld);

    for(SINT32 i = 0; i < iNumParticles; i++)
    {
        SINT32 id = fxPclGetNewParticle();

        fxPclSetWorldMatrix(id, &matWorld);
        fxPclPushFlags(id, lpPcl->uFlags);
        fxPclPopFlags(id, DEF_FXFLAG_INFINITE);
        fxPclPopFlags(id, DEF_FXFLAG_NOPHYSICS);
        fxPclSetType(id, IDE_PARTICLE_VAPOR_TRAIL);
        fxPclSetDragCoeff(id, 4.0f);
        fxPclSetSpriteNumber(id, 7);
        fxPclSetParticleVel(id, lpPcl->vecPVel.x, lpPcl->vecPVel.y, lpPcl->vecPVel.z);

        FLOAT32 fTime1 = (FLOAT32)frametime;
        FLOAT32 fTime2 = fTime1 + lpPcl->pclOffset.p2;
        FLOAT32 fTime3 = fTime2 + lpPcl->pclOffset.p3;
        FLOAT32 fTime4 = fTime3 + lpPcl->pclOffset.p4;

        fxPclSetPt1Time(id, fTime1 + fTimeOffset);
        fxPclSetPt2Time(id, fTime2 + fTimeOffset);
        fxPclSetPt3Time(id, fTime3 + fTimeOffset);
        fxPclSetPt4Time(id, fTime4 + fTimeOffset);

        fxPclSetRadius(id, lpPcl->fRadius);
        fxPclSetScale(id, lpPcl->pclScale.p1);

        fxPclSetPt1Color(id, lpClr->p1.r, lpClr->p1.g, lpClr->p1.b);
        fxPclSetPt2Color(id, lpClr->p2.r, lpClr->p2.g, lpClr->p2.b);
        fxPclSetPt3Color(id, lpClr->p3.r, lpClr->p3.g, lpClr->p3.b);
        fxPclSetPt4Color(id, lpClr->p4.r, lpClr->p4.g, lpClr->p4.b);
        fxPclSet4PtAlpha(id, lpClr->p1.a, lpClr->p2.a, lpClr->p3.a, lpClr->p4.a);
        fxPclSet4PtScale(id, lpScl->p1, lpScl->p2, lpScl->p3, lpScl->p4);
        fxPclSetPosition(id, vecPos.x, vecPos.y, vecPos.z);

        vecPos.x += vecUnit.x * lpPcl->fSpacing;
        vecPos.y += vecUnit.y * lpPcl->fSpacing;
        vecPos.z += vecUnit.z * lpPcl->fSpacing;

        fTimeOffset += fTimeStep;

        if(!fxPclInitParticle(id))
        {
            fxPclDetachByID(id);
        }
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _UpdateShells
// Description:
//////////////////////////////////////////////////////////////////////
void _UpdateShells(FXPARTICLE* lpPcl)
{
    if(lpPcl->uFlags & DEF_FXFLAG_OBJECT)
    {
        if(lpPcl->lpFxObj == nullptr)
            return;

        if(lpPcl->uFlags & DEF_FXFLAG_ROTATE)
        {
            fxPclRotateObject(lpPcl);
        }
        else
        {
            piMATRIX matObj = IdentityMatrix;
            OBJECT* lpObj = lpPcl->lpFxObj;

            matObj = lpObj->mat;

            matObj._41 = lpPcl->vecPt.x;
            matObj._42 = lpPcl->vecPt.y;
            matObj._43 = lpPcl->vecPt.z;

            fxUtlSetMatrixInTree(lpObj, &matObj);
        }
    }
    else
    {
        if(lpPcl->uFlags & DEF_FXFLAG_ROTATE)
        {
            piMATRIX matRot = IdentityMatrix;

            // Rotate Z
            matRot._11 = (FLOAT32)cos(lpPcl->vecRot.z);
            matRot._12 = (FLOAT32)sin(lpPcl->vecRot.z);

            matRot._21 = -matRot._12;
            matRot._22 =  matRot._11;

            fxUtlRotateZ(lpPcl->vecPcl, &matRot);
        }
    }

    // Rotate to face if a sprite.
    if(!(lpPcl->uFlags & DEF_FXFLAG_OBJECT))
    {
         _FaceMe(lpPcl->vecDraw,
                 lpPcl->vecPcl,
                 lpPcl->fScale,
                 &lpPcl->vecPt);
    }

    if(lpPcl->vecPos.z >= lpPcl->vecOrig.z)
        return;

    if(lpPcl->bDraw == FALSE)
        return;

    fxPclPlayShellSound(lpPcl);

    lpPcl->bDraw = FALSE;
}

//////////////////////////////////////////////////////////////////////
// Function: fxPclRotateObject
// Description: Rotate 3D particle object.
//////////////////////////////////////////////////////////////////////
void fxPclRotateObject(FXPARTICLE* lpPcl)
{
    piMATRIX matRotX = IdentityMatrix;
    piMATRIX matRotY = IdentityMatrix;
    piMATRIX matRotZ = IdentityMatrix;
    piMATRIX matObj = IdentityMatrix;

    OBJECT* lpObj = lpPcl->lpFxObj;

    matObj = lpObj->mat;

    // Translate to origin
    matObj._41 = 0.0f;
    matObj._42 = 0.0f;
    matObj._43 = 0.0f;

    // Rotate around X
    matRotX._22 = (FLOAT32)cos(lpPcl->vecRot.x);
    matRotX._23 = (FLOAT32)sin(lpPcl->vecRot.x);
    matRotX._32 = -matRotX._23;
    matRotX._33 =  matRotX._22;

    math3dMatrixMultiply(&matObj, &matObj, &matRotX);

    // Rotate around Y
    matRotY._11 = (FLOAT32)cos(lpPcl->vecRot.y);
    matRotY._13 = (FLOAT32)-sin(lpPcl->vecRot.y);
	matRotY._31 = -matRotY._13;
	matRotY._33 =  matRotY._11;

    math3dMatrixMultiply(&matObj, &matObj, &matRotY);

    // Rotate around Z
    matRotZ._11 = (FLOAT32)cos(lpPcl->vecRot.z);
    matRotZ._12 = (FLOAT32)sin(lpPcl->vecRot.z);
	matRotZ._21 = -matRotZ._12;
	matRotZ._22 =  matRotZ._11;

    math3dMatrixMultiply(&matObj, &matObj, &matRotZ);

    matObj._41 = lpPcl->vecPt.x;
    matObj._42 = lpPcl->vecPt.y;
    matObj._43 = lpPcl->vecPt.z;

    fxUtlSetMatrixInTree(lpObj, &matObj);
}

//////////////////////////////////////////////////////////////////////
// Function: fxPclPlayDirtSound
// Description:
//////////////////////////////////////////////////////////////////////
void fxPclPlayDirtSound(void* posAddress)
{
    FLOAT64VECTOR* pPos = (FLOAT64VECTOR*)posAddress;

    if(pPos == nullptr)
        return;

    SINT32 id = fxSndGetNewSound();

    if(id == -1)
        return;

    static const char* const szName = "sounds\\troop\\satchel-place.wav";
    fxSndSetSoundName(id, szName);
    fxSndSetPosition(id, pPos->x, pPos->y, pPos->z);
    fxSndSetPriority(id, IDE_TP_PRIORITY_LOW);
    fxSndSetSoundType(id, TYPE_FINITE_EXTERNAL_SOUND);
    fxSndSetVolume(id, 0.5f);
    fxSndSetPlayType(id, DEF_PLAYTYPE_WORLD);
    fxSndSetMaxDistance(id, 100.0f);
    fxSndSetMinDistance(id, 10.0f);

    fxSndInitSound(id);
}

//////////////////////////////////////////////////////////////////////
// Function: fxPclPlayShellSound
// Description:
//////////////////////////////////////////////////////////////////////
void fxPclPlayShellSound(FXPARTICLE* lpPcl)
{
    SINT32 id = fxSndGetNewSound();

    if(id == -1)
        return;

    static const char* const szName = "sounds\\casing.wav";
	fxSndSetSoundName(id, szName);
    fxSndSetPosition(id, lpPcl->vecPos.x, lpPcl->vecPos.y, lpPcl->vecPos.z);
    fxSndSetPriority(id, IDE_TP_PRIORITY_LOW);
    fxSndSetSoundType(id, TYPE_FINITE_EXTERNAL_SOUND);
    fxSndSetVolume(id, 0.5f);
    fxSndSetPitch(id, 1.0f);
    fxSndSetPlayType(id, DEF_PLAYTYPE_WORLD);
    fxSndSetMaxDistance(id, 50.0f);
    fxSndSetMinDistance(id, 5.0f);

    fxSndInitSound(id);
}

//////////////////////////////////////////////////////////////////////
// Function: _UpdateTrail
// Description: Stop playing when below ground pos.
//////////////////////////////////////////////////////////////////////
void _UpdateTrail(FXPARTICLE* lpPcl)
{
    if(lpPcl->vecPos.z >= lpPcl->vecOrig.z)
        return;

    if(lpPcl->bDraw == FALSE)
        return;

    lpPcl->bDraw = FALSE;
}

//////////////////////////////////////////////////////////////////////
// Function: _UpdateParticle
// Description:
//////////////////////////////////////////////////////////////////////
void _UpdateParticle(FXPARTICLE* lpPcl)
{
    if(lpPcl->lpEmitterObj)
    {
        if(g_mappedObjects.find(lpPcl->lpEmitterObj) == g_mappedObjects.end())
        {
            lpPcl->bDraw = FALSE;
            return;
        }
    }

    lpPcl->vecLastPos = lpPcl->vecPos;

    // Movement updated by object DOF
    if(lpPcl->uFlags & DEF_FXFLAG_DOF)
    {
        _ApplyDOF(lpPcl);
    }

    if(lpPcl->uFlags & DEF_FXFLAG_ATTACHAMMO)
    {
        _ApplyAmmo(lpPcl);
    }

    _UpdateDrag(lpPcl);
    _UpdateVelocity(lpPcl);

    if(!(lpPcl->uFlags & DEF_FXFLAG_INFINITE))
    {
        if((FLOAT32)frametime > lpPcl->pclTime.p4)
        {
            lpPcl->bDraw = FALSE;
        }
    }

    lpPcl->fLastUpdate = (FLOAT32)frametime;
}

//////////////////////////////////////////
// Function: _ApplyDOF
// Desctiption:
//////////////////////////////////////////
void _ApplyDOF(FXPARTICLE* lpPcl)
{
    OBJECTDOF* lpDOF = lpPcl->lpDOF;
    OBJECT* lpObj = lpPcl->lpEmitterObj;
    piVECTOR vecDir;
    piVECTOR vecVel;

    if(lpDOF == nullptr) return;
    if(lpObj == nullptr) return;

    fxUtlGetDOFPos(lpDOF, &lpObj->mat, &lpPcl->vecPos);
    fxUtlUpdatePosition(&lpPcl->vecPt, &lpPcl->vecPos);

    fxUtlGetDOFDirection(lpDOF, &lpObj->mat, &vecDir);

    // Move FX particle with vehicle velocity.
    if(lpPcl->lpVehicle)
    {
        if(!lpPcl->lpVehicle->InUse)
            return;

        FLOAT32 fSpeed = 0.0f;

        if(lpPcl->lpVehicle->ModelData)
        {
            FLOAT64VECTOR vecVehVel = lpPcl->lpVehicle->ModelData->VehicleVel;

            fSpeed = (FLOAT32)mathExactVectorLength(&vecVehVel);

            vecVel.x = vecDir.x * fSpeed;
            vecVel.y = vecDir.y * fSpeed;
            vecVel.z = vecDir.z * fSpeed;
        }
        else
        {
            fSpeed = (FLOAT32)mathExactVectorLength(&lpPcl->lpVehicle->PosData.Vel);

            vecVel.x = vecDir.x * fSpeed;
            vecVel.y = vecDir.y * fSpeed;
            vecVel.z = vecDir.z * fSpeed;
        }
    }
    else
    {
        vecVel.x = vecDir.x * lpPcl->vecPVel.x;
        vecVel.y = vecDir.y * lpPcl->vecPVel.y;
        vecVel.z = vecDir.z * lpPcl->vecPVel.z;
    }

    lpPcl->vecFXVel = vecVel;
}

//////////////////////////////////////////
// Function: _ApplyAmmo
// Desctiption:
//////////////////////////////////////////
void _ApplyAmmo(FXPARTICLE* lpPcl)
{
    weapAmmo* pAmmo = (weapAmmo*)lpPcl->pAmmo;

    if(pAmmo == nullptr)
        return;

    lpPcl->vecPos = pAmmo->Position;

    lpPcl->vecPVel.x = (FLOAT32)pAmmo->Velocity.x;
    lpPcl->vecPVel.y = (FLOAT32)pAmmo->Velocity.y;
    lpPcl->vecPVel.z = (FLOAT32)pAmmo->Velocity.z;
}

////////////////////////////////////////////////////
// Function: _UpdateSmokeTrail
// Description:
////////////////////////////////////////////////////
void _UpdateSmokeTrail(FXPARTICLE* lpPcl)
{
    if((FLOAT32)frametime < lpPcl->pclTime.p1)
        return;

    if(lpPcl->lpEmitterObj)
    {
        if(lpPcl->lpEmitterObj->flags & OBJECT_SKIPDRAWING)
        {
            _UpdateParticle(lpPcl);
            return;
        }
    }

    if(lpPcl->uFlags & DEF_FXFLAG_STOPATGROUND)
    {
        // Below ground?
        _UpdateTrail(lpPcl);
    }

    _UpdateParticle(lpPcl);

    if((FLOAT32)frametime - lpPcl->fLastQuadCreate < lpPcl->fStallTime)
    {
        return;
    }

    FLOAT64VECTOR vecDiff;

    mathSubVectors(&vecDiff, &lpPcl->vecLastPos, &lpPcl->vecPos);

    FLOAT64 fLength = mathExactVectorLength(&vecDiff);

    if(fLength < 0.00001f)
        return;

    SINT32 iNumParts = FloatToInt((FLOAT32)fLength / lpPcl->fSpacing);

    if(iNumParts == 0)
        iNumParts = 1;

    FLOAT64VECTOR vecUnitLength;

    vecUnitLength.x = vecDiff.x / (FLOAT32)iNumParts;
    vecUnitLength.y = vecDiff.y / (FLOAT32)iNumParts;
    vecUnitLength.z = vecDiff.z / (FLOAT32)iNumParts;

    FLOAT32 fDeltaTime = (FLOAT32)frametime - lpPcl->fLastQuadCreate;

    fDeltaTime /= (FLOAT32)iNumParts;

    FLOAT32 fTimeStep = 0.0f;

    FLOAT64VECTOR vecSave = lpPcl->vecPos;
    FLOAT64VECTOR vecPos = lpPcl->vecPos;

    for(SINT32 i = 0; i < iNumParts; i++)
    {
        lpPcl->vecPos = vecPos;

        _CreateSmoke(lpPcl, fTimeStep);

        vecPos.x += vecUnitLength.x;
        vecPos.y += vecUnitLength.y;
        vecPos.z += vecUnitLength.z;

        fTimeStep -= fDeltaTime;
    }

    lpPcl->vecPos = vecSave;
}

////////////////////////////////////////////////////
// Function: _CreateSmoke
// Description:
////////////////////////////////////////////////////
LOGICAL _CreateSmoke(FXPARTICLE* lpPcl, FLOAT32 fTimeOffset)
{
#if(0)
    SINT32 id = fxQuadGetNewParticle();

    if(id == -1) return FALSE;

    fxQuadSetSpriteNumber(id, fxUtlGetBillowSmoke());

    _SetQuadDefaultValues(lpPcl, id, fTimeOffset);

    // Required after settings
    fxQuadInitParticle(id);

    lpPcl->fLastQuadCreate = (FLOAT32)frametime;
#endif
    return TRUE;
}

////////////////////////////////////////////////////
// Function: _UpdateFireFountain
// Description:
////////////////////////////////////////////////////
void _UpdateFireFountain(FXPARTICLE* lpPcl)
{
    if((FLOAT32)frametime < lpPcl->pclTime.p1)
        return;

    // Keep updated even if skip drawing
    _UpdateParticle(lpPcl);

    if(lpPcl->lpEmitterObj)
    {
        if(lpPcl->lpEmitterObj->flags & OBJECT_SKIPDRAWING)
        {
            return;
        }
    }

    FLOAT32 fTimeDiff = (FLOAT32)frametime - lpPcl->fLastQuadCreate;

    SINT32 iNumFire = FloatToInt(fTimeDiff * lpPcl->fNumPerSec * 0.001f);

    if(iNumFire > DEF_FXPCL_MAX_PARTS)
    {
        lpPcl->fLastQuadCreate = (FLOAT32)frametime;
        return;
    }

    FLOAT32 fDeltaTime = 0.0f;
    FLOAT32 fTimeOffset = 0.0f;

    if(iNumFire > 0)
        fDeltaTime = fTimeDiff / (FLOAT32)iNumFire;

    for(SINT32 i = 0; i < iNumFire; i++)
    {
        _CreateFire(lpPcl, fTimeOffset);
        fTimeOffset -= fDeltaTime;
    }
}

////////////////////////////////////////////////////
// Function: _UpdateSmokeFountain
// Description:
////////////////////////////////////////////////////
void _UpdateSmokeFountain(FXPARTICLE* lpPcl)
{
    if((FLOAT32)frametime < lpPcl->pclTime.p1)
        return;

    if(lpPcl->lpEmitterObj)
    {
        if(lpPcl->lpEmitterObj->flags & OBJECT_SKIPDRAWING)
        {
            _UpdateParticle(lpPcl);
            return;
        }
    }

    if(lpPcl->uFlags & DEF_FXFLAG_STOPATGROUND)
    {
        // Below ground?
        _UpdateTrail(lpPcl);
    }

    _UpdateParticle(lpPcl);

    FLOAT32 fDeltaTime = (FLOAT32)frametime - lpPcl->fLastQuadCreate;

    if(fDeltaTime < lpPcl->fStallTime)
        return;

    SINT32 iNumSmoke = FloatToInt(fDeltaTime * lpPcl->fNumPerSec * 0.001f);

    if(iNumSmoke > DEF_FXPCL_MAX_PARTS)
    {
        lpPcl->fLastQuadCreate = (FLOAT32)frametime;
        return;
    }

    if(iNumSmoke == 0)
        return;

    FLOAT32 fTimeStep = 0.0f;

    fDeltaTime /= (FLOAT32)iNumSmoke;

    for(SINT32 i = 0; i < iNumSmoke; i++)
    {
        _CreateSmoke(lpPcl, fTimeStep);
        fTimeStep -= fDeltaTime;
    }
}

////////////////////////////////////////////////////
// Function: _UpdateFireTrail
// Description: Add a new 'fire' to the trail.
////////////////////////////////////////////////////
void _UpdateFireTrail(FXPARTICLE* lpPcl)
{
    if((FLOAT32)frametime < lpPcl->pclTime.p1)
        return;

    if(lpPcl->lpEmitterObj)
    {
        if(lpPcl->lpEmitterObj->flags & OBJECT_SKIPDRAWING)
        {
            _UpdateParticle(lpPcl);
            return;
        }
    }

    if(lpPcl->uFlags & DEF_FXFLAG_STOPATGROUND)
    {
        // Below ground?
        _UpdateTrail(lpPcl);
    }

    _UpdateParticle(lpPcl);

    if((FLOAT32)frametime - lpPcl->fLastQuadCreate < lpPcl->fStallTime)
    {
        return;
    }

    FLOAT64VECTOR vecDiff;

    mathSubVectors(&vecDiff, &lpPcl->vecLastPos, &lpPcl->vecPos);

    FLOAT64 fLength = mathExactVectorLength(&vecDiff);

    if(fLength < 0.00001f)
        return;

    SINT32 iNumParts = FloatToInt((FLOAT32)fLength / lpPcl->fSpacing);

    if(iNumParts == 0)
        iNumParts = 1;

    FLOAT64VECTOR vecUnitLength;

    vecUnitLength.x = vecDiff.x / (FLOAT32)iNumParts;
    vecUnitLength.y = vecDiff.y / (FLOAT32)iNumParts;
    vecUnitLength.z = vecDiff.z / (FLOAT32)iNumParts;

    FLOAT32 fDeltaTime = (FLOAT32)frametime - lpPcl->fLastQuadCreate;

    fDeltaTime /= (FLOAT32)iNumParts;

    FLOAT32 fTimeStep = 0.0f;

    FLOAT64VECTOR vecSave = lpPcl->vecPos;
    FLOAT64VECTOR vecPos = lpPcl->vecPos;

    for(SINT32 i = 0; i < iNumParts; i++)
    {
        lpPcl->vecPos = vecPos;

        _CreateFire(lpPcl, fTimeStep);

        vecPos.x += vecUnitLength.x;
        vecPos.y += vecUnitLength.y;
        vecPos.z += vecUnitLength.z;

        fTimeStep -= fDeltaTime;
    }

    lpPcl->vecPos = vecSave;
}

//////////////////////////////////////////
// Function: _CreateFire
// Description:
//////////////////////////////////////////
LOGICAL _CreateFire(FXPARTICLE* lpPcl, FLOAT32 fTimeOffset)
{
#if(0)
    SINT32 id = fxQuadGetNewParticle();

    if(id == -1) return FALSE;

    fxQuadSetSpriteNumber(id, fxUtlGetFire());

    // Random glow flame
    if(!(lpPcl->uFlags & DEF_FXFLAG_GLOW))
    {
        if((abs(mathGetChaoticNumber() % 10) == 4))
        {
            fxQuadPushFlags(id, DEF_FXFLAG_GLOW);
            fxQuadPushFlags(id, DEF_FXFLAG_USEDRAG);
            fxQuadSetDragCoeff(id, 8.0f);
            fxQuadSetPt2Color(id, 1.0f, 1.0f, 1.0f);
            fxQuadSetPt3Color(id, 1.0f, 1.0f, 1.0f);
            fxQuadSetPt4Color(id, 1.0f, 1.0f, 1.0f);
            fxQuadPopFlags(id, DEF_FXFLAG_ROTATE);
        }
    }

    _SetQuadDefaultValues(lpPcl, id, fTimeOffset);

    // Required after settings
    fxQuadInitParticle(id);

    // C shape texture
    id = fxQuadGetNewParticle();

    if(id == -1) return FALSE;

    fxQuadSetSpriteNumber(id, fxUtlGetFireC());
    fxQuadPushFlags(id, DEF_FXFLAG_GLOW);

    _SetQuadDefaultValues(lpPcl, id, fTimeOffset);

    fxQuadInitParticle(id);

    lpPcl->fLastQuadCreate = (FLOAT32)frametime;
#endif
    return TRUE;
}

//////////////////////////////////////////
// Function: _SetQuadDefaultValues
// Description:
//////////////////////////////////////////
void _SetQuadDefaultValues(FXPARTICLE* lpPcl, SINT32 id, FLOAT32 fTimeOffset)
{
#if(0)
    INTCOLOR* lpClr = &lpPcl->pclColor;
    INTFLOAT32* lpScl = &lpPcl->pclScale;

    fxQuadPushFlags(id, lpPcl->uFlags);
    fxQuadSetPosition(id, lpPcl->vecPos.x, lpPcl->vecPos.y, lpPcl->vecPos.z);
    fxQuadSetRadius(id, lpPcl->fRadius);
    fxQuadSetScale(id, lpPcl->fScale);
    fxQuadSetDragCoeff(id, lpPcl->fDragCo);

    if(lpPcl->uFlags & DEF_FXFLAG_USESPRITENUM)
    {
        fxQuadSetSpriteNumber(id, lpPcl->uSpriteNum);
    }

    fxQuadSetPt1Velocity(id, lpPcl->pclVel.p1.x, lpPcl->pclVel.p1.y, lpPcl->pclVel.p1.z);
    fxQuadSetPt2Velocity(id, lpPcl->pclVel.p2.x, lpPcl->pclVel.p2.y, lpPcl->pclVel.p2.z);
    fxQuadSetPt3Velocity(id, lpPcl->pclVel.p3.x, lpPcl->pclVel.p3.y, lpPcl->pclVel.p3.z);
    fxQuadSetPt4Velocity(id, lpPcl->pclVel.p4.x, lpPcl->pclVel.p4.y, lpPcl->pclVel.p4.z);

    piVECTOR vecVel = lpPcl->vecFXVel;

    if(lpPcl->uFlags & DEF_FXFLAG_VARYVELOCITY)
    {
        vecVel.x += lpPcl->fVariance * (mathGetFloat32ChaoticNumber() * 2.0f - 1.0f);
        vecVel.y += lpPcl->fVariance * (mathGetFloat32ChaoticNumber() * 2.0f - 1.0f);
        vecVel.z += lpPcl->fVariance * mathGetFloat32ChaoticNumber();
    }

    fxQuadSetVelocity(id, vecVel.x, vecVel.y, vecVel.z);

    fxQuadSet4PtScale(id, lpScl->p1, lpScl->p2, lpScl->p3, lpScl->p4);
    fxQuadSetPt1Color(id, lpClr->p1.r, lpClr->p1.g, lpClr->p1.b);
    fxQuadSetPt2Color(id, lpClr->p2.r, lpClr->p2.g, lpClr->p2.b);
    fxQuadSetPt3Color(id, lpClr->p3.r, lpClr->p3.g, lpClr->p3.b);
    fxQuadSetPt4Color(id, lpClr->p4.r, lpClr->p4.g, lpClr->p4.b);
    fxQuadSet4PtAlpha(id, lpClr->p1.a, lpClr->p2.a, lpClr->p3.a, lpClr->p4.a);

    FLOAT32 fTime1 = (FLOAT32)frametime + lpPcl->pclOffset.p1;
    FLOAT32 fTime2 = fTime1 + lpPcl->pclOffset.p2;
    FLOAT32 fTime3 = fTime2 + lpPcl->pclOffset.p3;
    FLOAT32 fTime4 = fTime3 + lpPcl->pclOffset.p4;

    fxQuadSetPt1Time(id, fTime1 + fTimeOffset);
    fxQuadSetPt2Time(id, fTime2 + fTimeOffset);
    fxQuadSetPt3Time(id, fTime3 + fTimeOffset);
    fxQuadSetPt4Time(id, fTime4 + fTimeOffset);

    if(lpPcl->uFlags & DEF_FXFLAG_INTROTATE)
    {
        FLOAT32 z1, z2, z3, z4;

        // Rotate slowly around z.
        z1 = (mathGetFloat32ChaoticNumber() * 2.0f - 1.0f) * g_fPiOver180;

        z1 *= lpPcl->vecRot.z;

        z2 = z1 * 0.5f;
        z3 = z2 * 0.5f;
        z4 = 0.0f;

        fxQuadSetPt1Rotate(id, 0.0f, 0.0f, z1);
        fxQuadSetPt2Rotate(id, 0.0f, 0.0f, z2);
        fxQuadSetPt3Rotate(id, 0.0f, 0.0f, z3);
        fxQuadSetPt4Rotate(id, 0.0f, 0.0f, z4);
    }
    else if(lpPcl->uFlags & DEF_FXFLAG_ROTATE)
    {
        FLOAT32 z;

        // Rotate slowly around z.
        z = (mathGetFloat32ChaoticNumber() * 2.0f - 1.0f) * g_fPiOver180;

        z *= lpPcl->vecRot.z;

        fxQuadSetRotate(id, 0.0f, 0.0f, z);
    }
#endif
}

//////////////////////////////////////////
// Function: fxPclGetList
// Description:
//////////////////////////////////////////
void fxPclGetList(void* lpParticleSort, SINT32* pIndex)
{
    FXSORT* lpSort = (FXSORT*)lpParticleSort;

    g_v3Ddraw.clear();

    if(!lpSort || !pIndex) return;
    if(g_pclHead.next == nullptr) return;
    if(g_pclHead.next == &g_pclTail) return;

    FXPARTICLE* lpPcl = g_pclHead.next;
    FXPARTICLE* lpTail = &g_pclTail;

    for(lpPcl; lpPcl != lpTail; lpPcl = lpPcl->next)
    {
        if(*pIndex >= DEF_MAX_EFFECTS)
            break;

        if(lpPcl->lpFxObj)
        {
            g_v3Ddraw.push_back(lpPcl->ID);
        }
        else
        {
            lpSort[*pIndex].type = IDE_FXTYPE_PARTICLE;
            lpSort[*pIndex].dist2 = lpPcl->fDistanceSqrd;
            lpSort[*pIndex].index = lpPcl->ID;

            (*pIndex)++;
        }
    }
}

//////////////////////////////////////////
// Function: fxPclDraw3D
// Description:
//////////////////////////////////////////
LOGICAL fxPclDraw3D()
{
    if(g_v3Ddraw.empty())
        return TRUE;

    std::vector<SINT32>::iterator B = g_v3Ddraw.begin();
    std::vector<SINT32>::iterator E = g_v3Ddraw.end();

    for(B; B != E; ++B)
    {
        fxPclDraw((*B));
    }

    return TRUE;
}

//////////////////////////////////////////
// Function: fxPclDraw
// Description:
//////////////////////////////////////////
LOGICAL fxPclDraw(SINT32 ID)
{
    if(g_pclHead.next == nullptr)
        return FALSE;

    if(g_pclHead.next == &g_pclTail)
        return FALSE;

    SINT32 i = ID - 1;

    FXPARTICLE* lpPcl = &particles[i];

    if(lpPcl->enumType == IDE_PARTICLE_CALLBACK)
        return TRUE;

    if((FLOAT32)frametime < lpPcl->pclTime.p1)
        return TRUE;

    piDisableMaterial();

    // Hack for now. Make sure all particles are current in position
    if(lpPcl->enumType != IDE_PARTICLE_VAPOR_TRAIL)
    {
        _UpdateParticle(lpPcl);
    }

    if(_CanDraw(lpPcl))
    {
        switch(lpPcl->enumType)
        {
        case IDE_PARTICLE_SHELLS:
            {
                _DrawShells(lpPcl);
                break;
            }
        case IDE_PARTICLE_DEBRIS:
            {
                _DrawDebris(lpPcl);
                break;
            }
        case IDE_PARTICLE_MUZZLEFLASH:
            {
                //_UpdateMuzzleFlash(lpPcl);
                //_DrawMuzzleFlash(lpPcl);
                break;
            }
        case IDE_PARTICLE_VAPOR_TRAIL:
            {
                _DrawVaporTrail(lpPcl);
                break;
            }
        }
    }

    _FreeParticleIfNecessary(lpPcl);

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _DrawVaporTrail
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL _DrawVaporTrail(FXPARTICLE* lpPcl)
{
    piMATRIX matOldWorld = *piGetWorldMatrix();

    piMATRIX matWorld = lpPcl->matWorld;
    piVECTOR vecScale[4];

    math3dScaleVector(&vecScale[0], &lpPcl->vecPcl[0], lpPcl->fScale);
    math3dScaleVector(&vecScale[1], &lpPcl->vecPcl[1], lpPcl->fScale);
    math3dScaleVector(&vecScale[2], &lpPcl->vecPcl[2], lpPcl->fScale);
    math3dScaleVector(&vecScale[3], &lpPcl->vecPcl[3], lpPcl->fScale);

    matWorld._41 = lpPcl->vecPt.x;
    matWorld._42 = lpPcl->vecPt.y;
    matWorld._43 = lpPcl->vecPt.z;

    if(lpPcl->uFlags & DEF_FXFLAG_GLOW)
    {
        piSetRenderState(piRENDERSTATE_DESTBLEND, piBLEND_ONE);
    }

    piSetWorldMatrix(&matWorld);

    piSetRenderState(piRENDERSTATE_CULLFACEENABLE, 0);

    lpPcl->vecDraw[0] = vecScale[0];
    lpPcl->vecDraw[1] = vecScale[1];
    lpPcl->vecDraw[2] = vecScale[2];
    lpPcl->vecDraw[3] = vecScale[3];

    _DrawLVertices(lpPcl);

    piMATRIX matRotY = IdentityMatrix;

    matRotY._11 =  fxUtlGetCos(45);
    matRotY._13 =  fxUtlGetSin(45);
    matRotY._31 = -matRotY._13;
    matRotY._33 =  matRotY._11;

    math3dVectorMultiply(&lpPcl->vecDraw[0], &vecScale[0], &matRotY);
    math3dVectorMultiply(&lpPcl->vecDraw[1], &vecScale[1], &matRotY);
    math3dVectorMultiply(&lpPcl->vecDraw[2], &vecScale[2], &matRotY);
    math3dVectorMultiply(&lpPcl->vecDraw[3], &vecScale[3], &matRotY);

    _DrawLVertices(lpPcl);

    matRotY = IdentityMatrix;

    matRotY._11 =  fxUtlGetCos(90);
    matRotY._13 =  fxUtlGetSin(90);
    matRotY._31 = -matRotY._13;
    matRotY._33 =  matRotY._11;

    math3dVectorMultiply(&lpPcl->vecDraw[0], &vecScale[0], &matRotY);
    math3dVectorMultiply(&lpPcl->vecDraw[1], &vecScale[1], &matRotY);
    math3dVectorMultiply(&lpPcl->vecDraw[2], &vecScale[2], &matRotY);
    math3dVectorMultiply(&lpPcl->vecDraw[3], &vecScale[3], &matRotY);

    _DrawLVertices(lpPcl);

    matRotY = IdentityMatrix;

    matRotY._11 =  fxUtlGetCos(135);
    matRotY._13 =  fxUtlGetSin(135);
    matRotY._31 = -matRotY._13;
    matRotY._33 =  matRotY._11;

    math3dVectorMultiply(&lpPcl->vecDraw[0], &vecScale[0], &matRotY);
    math3dVectorMultiply(&lpPcl->vecDraw[1], &vecScale[1], &matRotY);
    math3dVectorMultiply(&lpPcl->vecDraw[2], &vecScale[2], &matRotY);
    math3dVectorMultiply(&lpPcl->vecDraw[3], &vecScale[3], &matRotY);

    _DrawLVertices(lpPcl);

    piSetWorldMatrix(&matOldWorld);

    piSetRenderState(piRENDERSTATE_CULLFACEENABLE, piCULLFACEENABLE_DEFAULT);
    piSetRenderState(piRENDERSTATE_DESTBLEND, DestBlend);

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _FaceMe
// Description: Have particle face the player.
//////////////////////////////////////////////////////////////////////
void _FaceMe(piVECTOR vecOut[4], piVECTOR vecIn[4], FLOAT32 fScale, piVECTOR* pPos)
{
	math3dScaleVector(&vecOut[0], &vecIn[0], fScale);
	math3dScaleVector(&vecOut[1], &vecIn[1], fScale);
	math3dScaleVector(&vecOut[2], &vecIn[2], fScale);
	math3dScaleVector(&vecOut[3], &vecIn[3], fScale);

    math3dVectorMultiply(&vecOut[0], &vecOut[0], g_lpInvertView);
    math3dVectorMultiply(&vecOut[1], &vecOut[1], g_lpInvertView);
    math3dVectorMultiply(&vecOut[2], &vecOut[2], g_lpInvertView);
    math3dVectorMultiply(&vecOut[3], &vecOut[3], g_lpInvertView);

    math3dAddVectors(&vecOut[0], &vecOut[0], pPos);
    math3dAddVectors(&vecOut[1], &vecOut[1], pPos);
    math3dAddVectors(&vecOut[2], &vecOut[2], pPos);
    math3dAddVectors(&vecOut[3], &vecOut[3], pPos);
}

//////////////////////////////////////////////////////////////////////
// Function: _DrawDebris
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL _DrawDebris(FXPARTICLE* lpPcl)
{
    if(lpPcl->uFlags & DEF_FXFLAG_OBJECT)
    {
        obdrawDrawTree(lpPcl->lpFxObj);
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _DrawMuzzleFlash
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL _DrawMuzzleFlash(FXPARTICLE* pcl)
{
    if(pcl->lpFxObj)
    {
        if(pcl->lpFxObj->drawcallback)
        {
            pcl->lpFxObj->drawcallback(pcl->lpFxObj);
            return TRUE;
        }
    }

    UINT32 zwrite = piGetEngineRenderState(piRENDERSTATE_ZWRITEENABLE);

    piSetRenderState(piRENDERSTATE_ZWRITEENABLE, FALSE);

    if(pcl->uFlags & DEF_FXFLAG_OBJECT)
    {
        obdrawDrawTree(pcl->lpFxObj);
    }

    piSetRenderState(piRENDERSTATE_ZWRITEENABLE, zwrite);

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _DrawShells
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL _DrawShells(FXPARTICLE* lpPcl)
{
    if(lpPcl->uFlags & DEF_FXFLAG_OBJECT)
    {
        obdrawDrawTree(lpPcl->lpFxObj);
    }
    else
    {
        piDisableMaterial();
        _DrawLVertices(lpPcl);
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _DrawLVertices
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL _DrawLVertices(FXPARTICLE* lpPcl)
{
    piLVERTEX LVert[4];
    piINDICES piIndices[2];

	// We are going to copy LVert[0] into 1, 2 and 3,
	// so we only need to memset the first one.
    memset(LVert, 0, sizeof(piLVERTEX) * 1);

    LVert[0].color = piRGBA(lpPcl->piColor.r,
                            lpPcl->piColor.g,
                            lpPcl->piColor.b,
                            lpPcl->piColor.a);

    LVert[0].specular = colorALPHA100;

    LVert[1] = LVert[0];
    LVert[2] = LVert[0];
    LVert[3] = LVert[0];

    LVert[0].x = lpPcl->vecDraw[0].x;
    LVert[0].y = lpPcl->vecDraw[0].y;
    LVert[0].z = lpPcl->vecDraw[0].z;

    LVert[1].x = lpPcl->vecDraw[1].x;
    LVert[1].y = lpPcl->vecDraw[1].y;
    LVert[1].z = lpPcl->vecDraw[1].z;

    LVert[2].x = lpPcl->vecDraw[2].x;
    LVert[2].y = lpPcl->vecDraw[2].y;
    LVert[2].z = lpPcl->vecDraw[2].z;

    LVert[3].x = lpPcl->vecDraw[3].x;
    LVert[3].y = lpPcl->vecDraw[3].y;
    LVert[3].z = lpPcl->vecDraw[3].z;

    LVert[0].tu = lpPcl->uv[0].u;
    LVert[0].tv = lpPcl->uv[1].v;

    LVert[1].tu = lpPcl->uv[0].u;
    LVert[1].tv = lpPcl->uv[0].v;

    LVert[2].tu = lpPcl->uv[1].u;
    LVert[2].tv = lpPcl->uv[0].v;

    LVert[3].tu = lpPcl->uv[1].u;
    LVert[3].tv = lpPcl->uv[1].v;

    piIndices[0].v1 = 0;
    piIndices[0].v2 = 1;
    piIndices[0].v3 = 2;

    piIndices[1].v1 = 2;
    piIndices[1].v2 = 3;
    piIndices[1].v3 = 0;

    piRenderIndexScene(piPT_TRIANGLELIST,
                       piVT_LVERTEX,
                       LVert,
                       4,
                       (UINT16*)piIndices,
                       6);

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _CanDraw
// Description: Should we draw the particle?
//////////////////////////////////////////////////////////////////////
LOGICAL _CanDraw(FXPARTICLE* lpPcl)
{
    if(!lpPcl->bDraw)
        return FALSE;

    FLOAT32 fRadius = lpPcl->fRadius;

    if(!rnvisVectorFOVCheck(&lpPcl->vecPt, fRadius))
        return FALSE;

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _FreeParticleIfNecessary
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL _FreeParticleIfNecessary(FXPARTICLE* lpPcl)
{
    if(lpPcl == nullptr)
        return FALSE;

    if(lpPcl->bDraw == TRUE)
        return FALSE;

    switch(lpPcl->enumType)
    {
    case IDE_PARTICLE_SHELLS:
    case IDE_PARTICLE_DEBRIS:
    case IDE_PARTICLE_MUZZLEFLASH:
        {
            if(lpPcl->uFlags & DEF_FXFLAG_OBJECT)
            {
                if(lpPcl->pFnUserKill)
                {
                    lpPcl->pFnUserKill(lpPcl->pKillData);
                    lpPcl->pFnUserKill = nullptr;
                }
                else if(lpPcl->lpFxObj)
                {
                    ogrpFreeObjectTree(lpPcl->lpFxObj);
                    lpPcl->lpFxObj = nullptr;
                }
            }

            break;
        }
    }

    if(lpPcl->pFnUserKill)
    {
        lpPcl->pFnUserKill(lpPcl->pKillData);
    }

    _ReleaseParticle(lpPcl);
    _FreeParticle(lpPcl);

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: fxPclDetachByID
// Description: Detach particle from object.
//////////////////////////////////////////////////////////////////////
void fxPclDetachByID(SINT32 id)
{
    SINT32 index = id - 1;

    if(index < 0 || index >= DEF_FXPCL_MAX_PARTICLES)
    {
        return;
    }

    if(particles == nullptr)
    {
        return;
    }

    if(particles[index].bDraw == FALSE)
        return;

    particles[index].bDraw = FALSE;

    _FreeParticleIfNecessary(&particles[index]);
}

//////////////////////////////////////////////////////////////////////
// Function: fxPclDetachObj
// Description: Detach particle from object.
//////////////////////////////////////////////////////////////////////
void fxPclDetachObj(OBJECT* lpObj)
{
    if(particles == nullptr)
        return;

    if(!lpObj)
        return;

    if(!(lpObj->flag2 & OBJECT_HAS_FX))
        return;

    std::multimap<void* , SINT32>::iterator I = g_mappedObjects.find(lpObj);
    std::multimap<void* , SINT32>::iterator FREE;

    std::vector<SINT32> removeList;

    while(I != g_mappedObjects.end())
    {
        if(I->first != lpObj)
            break;

        FREE = I;
        ++I;

        removeList.push_back(FREE->second);
        g_mappedObjects.erase(FREE);
    }

    for(UINT32 u = 0; u < removeList.size(); u++)
    {
         particles[removeList[u] - 1].bDraw = FALSE;
         particles[removeList[u] - 1].lpEmitterObj = nullptr;

        _FreeParticleIfNecessary(&particles[removeList[u] - 1]);
   }
}

//////////////////////////////////////////////////////////////////////
// Function: fxPclDetachVehicle
// Description: Detach particle from object.
//////////////////////////////////////////////////////////////////////
void fxPclDetachVehicle(vehVehicle* v)
{
    if(particles == nullptr)
        return;

    std::multimap<void* , SINT32>::iterator I = g_mappedVehicles.find(v);
    std::multimap<void* , SINT32>::iterator FREE;

    std::vector<SINT32> removeList;

    while(I != g_mappedVehicles.end())
    {
        if(I->first != v)
            break;

        FREE = I;
        ++I;

        removeList.push_back(FREE->second);
        g_mappedVehicles.erase(FREE);
    }

    for(UINT32 u = 0; u < removeList.size(); u++)
    {
         particles[removeList[u] - 1].bDraw = FALSE;
         particles[removeList[u] - 1].lpVehicle = nullptr;

        _FreeParticleIfNecessary(&particles[removeList[u] - 1]);
   }
}

//////////////////////////////////////////////////////////////////////
// Function: fxPclDetachAmmo
// Description: Detach particle from ammo. It's possible that one ammo may
// be associated with many particles.
//////////////////////////////////////////////////////////////////////
void fxPclDetachAmmo(void const* pAmmo)
{
    FXPARTICLE* lpPcl = g_pclHead.next;

    if(lpPcl == nullptr) return;

    weapAmmo* ammo = (weapAmmo*)pAmmo;

    if(ammo->iParticleID)
    {
        fxPclDetachByID(ammo->iParticleID);
        ammo->iParticleID = 0;
    }
}

//////////////////////////////////////////////////////////////////////
// Function: fxPclGetNewParticle
// Description: Create empty particle
//////////////////////////////////////////////////////////////////////
SINT32 fxPclGetNewParticle()
{
    FXPARTICLE* pcl = nullptr;

    if(!_GetParticle(&pcl))
        return -1;

    if(!_AddParticle(pcl))
        return -1;

    return pcl->ID;
}

//////////////////////////////////////////////////////////////////////
// Function: fxPclSetType
// Description:
//////////////////////////////////////////////////////////////////////
void fxPclSetType(SINT32 id, ENUMPARTICLETYPE enumType)
{
    SINT32 index = id - 1;

    if(index < 0 || index >= DEF_FXPCL_MAX_PARTICLES)
        return;

    particles[index].enumType = enumType;
}

//////////////////////////////////////////////////////////////////////
// Function: fxPclSetPosition
// Description:
//////////////////////////////////////////////////////////////////////
void fxPclSetPosition(SINT32 id, FLOAT64 x, FLOAT64 y, FLOAT64 z)
{
    SINT32 index = id - 1;

    if(index < 0 || index >= DEF_FXPCL_MAX_PARTICLES)
        return;

    particles[index].vecPos.x = x;
    particles[index].vecPos.y = y;
    particles[index].vecPos.z = z;
}

//////////////////////////////////////////////////////////////////////
// Function: fxPclPushFlags
// Description:
//////////////////////////////////////////////////////////////////////
void fxPclPushFlags(SINT32 id, UINT32 uFlags)
{
    SINT32 index = id - 1;

    if(index < 0 || index >= DEF_FXPCL_MAX_PARTICLES)
        return;

    particles[index].uFlags |= uFlags;
}

//////////////////////////////////////////////////////////////////////
// Function: fxPclPopFlags
// Description:
//////////////////////////////////////////////////////////////////////
void fxPclPopFlags(SINT32 id, UINT32 uFlags)
{
    SINT32 index = id - 1;

    if(index < 0 || index >= DEF_FXPCL_MAX_PARTICLES)
        return;

    particles[index].uFlags &= ~uFlags;
}

//////////////////////////////////////////////////////////////////////
// Function: fxPclSetPt1Time
// Description:
//////////////////////////////////////////////////////////////////////
void fxPclSetPt1Time(SINT32 id, FLOAT32 fTime)
{
    SINT32 index = id - 1;

    if(index < 0 || index >= DEF_FXPCL_MAX_PARTICLES)
        return;

    particles[index].pclTime.p1 = fTime;
}

//////////////////////////////////////////////////////////////////////
// Function: fxPclSetPt2Time
// Description:
//////////////////////////////////////////////////////////////////////
void fxPclSetPt2Time(SINT32 id, FLOAT32 fTime)
{
    SINT32 index = id - 1;

    if(index < 0 || index >= DEF_FXPCL_MAX_PARTICLES)
        return;

    particles[index].pclTime.p2 = fTime;
}

//////////////////////////////////////////////////////////////////////
// Function: fxPclSetPt3Time
// Description:
//////////////////////////////////////////////////////////////////////
void fxPclSetPt3Time(SINT32 id, FLOAT32 fTime)
{
    SINT32 index = id - 1;

    if(index < 0 || index >= DEF_FXPCL_MAX_PARTICLES)
        return;

    particles[index].pclTime.p3 = fTime;
}

//////////////////////////////////////////////////////////////////////
// Function: fxPclSetPt4Time
// Description:
//////////////////////////////////////////////////////////////////////
void fxPclSetPt4Time(SINT32 id, FLOAT32 fTime)
{
    SINT32 index = id - 1;

    if(index < 0 || index >= DEF_FXPCL_MAX_PARTICLES)
        return;

    particles[index].pclTime.p4 = fTime;
}

//////////////////////////////////////////////////////////////////////
// Function: fxPclSet4PtTimeOffset
// Description:
//////////////////////////////////////////////////////////////////////
void fxPclSet4PtTimeOffset(SINT32 id, FLOAT32 p1, FLOAT32 p2, FLOAT32 p3, FLOAT32 p4)
{
    SINT32 index = id - 1;

    if(index < 0 || index >= DEF_FXPCL_MAX_PARTICLES)
        return;

    particles[index].pclOffset.p1 = p1;
    particles[index].pclOffset.p2 = p2;
    particles[index].pclOffset.p3 = p3;
    particles[index].pclOffset.p4 = p4;
}

//////////////////////////////////////////////////////////////////////
// Function: fxPclSet4PtScale
// Description:
//////////////////////////////////////////////////////////////////////
void fxPclSet4PtScale(SINT32 id, FLOAT32 p1, FLOAT32 p2, FLOAT32 p3, FLOAT32 p4)
{
    SINT32 index = id - 1;

    if(index < 0 || index >= DEF_FXPCL_MAX_PARTICLES)
        return;

    particles[index].pclScale.p1 = p1;
    particles[index].pclScale.p2 = p2;
    particles[index].pclScale.p3 = p3;
    particles[index].pclScale.p4 = p4;
}

//////////////////////////////////////////////////////////////////////
// Function: fxPclSetPt1Color
// Description:
//////////////////////////////////////////////////////////////////////
void fxPclSetPt1Color(SINT32 id, FLOAT32 r, FLOAT32 g, FLOAT32 b)
{
    SINT32 index = id - 1;

    if(index < 0 || index >= DEF_FXPCL_MAX_PARTICLES)
        return;

    particles[index].pclColor.p1.r = r;
    particles[index].pclColor.p1.g = g;
    particles[index].pclColor.p1.b = b;

    particles[index].piColor.r = r;
    particles[index].piColor.g = g;
    particles[index].piColor.b = b;
}

//////////////////////////////////////////////////////////////////////
// Function: fxPclSetPt2Color
// Description:
//////////////////////////////////////////////////////////////////////
void fxPclSetPt2Color(SINT32 id, FLOAT32 r, FLOAT32 g, FLOAT32 b)
{
    SINT32 index = id - 1;

    if(index < 0 || index >= DEF_FXPCL_MAX_PARTICLES)
        return;

    particles[index].pclColor.p2.r = r;
    particles[index].pclColor.p2.g = g;
    particles[index].pclColor.p2.b = b;
}

//////////////////////////////////////////////////////////////////////
// Function: fxPclSetPt3Color
// Description:
//////////////////////////////////////////////////////////////////////
void fxPclSetPt3Color(SINT32 id, FLOAT32 r, FLOAT32 g, FLOAT32 b)
{
    SINT32 index = id - 1;

    if(index < 0 || index >= DEF_FXPCL_MAX_PARTICLES)
        return;

    particles[index].pclColor.p3.r = r;
    particles[index].pclColor.p3.g = g;
    particles[index].pclColor.p3.b = b;
}

//////////////////////////////////////////////////////////////////////
// Function: fxPclSetPt4Color
// Description:
//////////////////////////////////////////////////////////////////////
void fxPclSetPt4Color(SINT32 id, FLOAT32 r, FLOAT32 g, FLOAT32 b)
{
    SINT32 index = id - 1;

    if(index < 0 || index >= DEF_FXPCL_MAX_PARTICLES)
        return;

    particles[index].pclColor.p4.r = r;
    particles[index].pclColor.p4.g = g;
    particles[index].pclColor.p4.b = b;
}

//////////////////////////////////////////////////////////////////////
// Function: fxPclSet4PtAlpha
// Description:
//////////////////////////////////////////////////////////////////////
void fxPclSet4PtAlpha(SINT32 id, FLOAT32 p1, FLOAT32 p2, FLOAT32 p3, FLOAT32 p4)
{
    SINT32 index = id - 1;

    if(index < 0 || index >= DEF_FXPCL_MAX_PARTICLES)
        return;

    particles[index].pclColor.p1.a = p1;
    particles[index].pclColor.p2.a = p2;
    particles[index].pclColor.p3.a = p3;
    particles[index].pclColor.p4.a = p4;

    particles[index].piColor.a = p1;
}

//////////////////////////////////////////////////////////////////////
// Function: fxPclSetPt1Rotate
// Description:
//////////////////////////////////////////////////////////////////////
void fxPclSetPt1Rotate(SINT32 id, FLOAT32 x, FLOAT32 y, FLOAT32 z)
{
    SINT32 index = id - 1;

    if(index < 0 || index >= DEF_FXPCL_MAX_PARTICLES)
        return;

    particles[index].pclRotate.p1.x = x;
    particles[index].pclRotate.p1.y = y;
    particles[index].pclRotate.p1.z = z;
}

//////////////////////////////////////////////////////////////////////
// Function: fxPclSetPt2Rotate
// Description:
//////////////////////////////////////////////////////////////////////
void fxPclSetPt2Rotate(SINT32 id, FLOAT32 x, FLOAT32 y, FLOAT32 z)
{
    SINT32 index = id - 1;

    if(index < 0 || index >= DEF_FXPCL_MAX_PARTICLES)
        return;

    particles[index].pclRotate.p2.x = x;
    particles[index].pclRotate.p2.y = y;
    particles[index].pclRotate.p2.z = z;
}

//////////////////////////////////////////////////////////////////////
// Function: fxPclSetPt3Rotate
// Description:
//////////////////////////////////////////////////////////////////////
void fxPclSetPt3Rotate(SINT32 id, FLOAT32 x, FLOAT32 y, FLOAT32 z)
{
    SINT32 index = id - 1;

    if(index < 0 || index >= DEF_FXPCL_MAX_PARTICLES)
        return;

    particles[index].pclRotate.p3.x = x;
    particles[index].pclRotate.p3.y = y;
    particles[index].pclRotate.p3.z = z;
}

//////////////////////////////////////////////////////////////////////
// Function: fxPclSetPt4Rotate
// Description:
//////////////////////////////////////////////////////////////////////
void fxPclSetPt4Rotate(SINT32 id, FLOAT32 x, FLOAT32 y, FLOAT32 z)
{
    SINT32 index = id - 1;

    if(index < 0 || index >= DEF_FXPCL_MAX_PARTICLES)
        return;

    particles[index].pclRotate.p4.x = x;
    particles[index].pclRotate.p4.y = y;
    particles[index].pclRotate.p4.z = z;
}

//////////////////////////////////////////////////////////////////////
// Function: fxPclSetRotate
// Description:
//////////////////////////////////////////////////////////////////////
void fxPclSetRotate(SINT32 id, FLOAT32 x, FLOAT32 y, FLOAT32 z)
{
    SINT32 index = id - 1;

    if(index < 0 || index >= DEF_FXPCL_MAX_PARTICLES)
        return;

    particles[index].vecRot.x = x;
    particles[index].vecRot.y = y;
    particles[index].vecRot.z = z;
}

//////////////////////////////////////////////////////////////////////
// Function: fxPclSetPt1Velocity
// Description:
//////////////////////////////////////////////////////////////////////
void fxPclSetPt1Velocity(SINT32 id, FLOAT32 x, FLOAT32 y, FLOAT32 z)
{
    SINT32 index = id - 1;

    if(index < 0 || index >= DEF_FXPCL_MAX_PARTICLES)
        return;

    particles[index].pclVel.p1.x = x;
    particles[index].pclVel.p1.y = y;
    particles[index].pclVel.p1.z = z;
}

//////////////////////////////////////////////////////////////////////
// Function: fxPclSetPt2Velocity
// Description:
//////////////////////////////////////////////////////////////////////
void fxPclSetPt2Velocity(SINT32 id, FLOAT32 x, FLOAT32 y, FLOAT32 z)
{
    SINT32 index = id - 1;

    if(index < 0 || index >= DEF_FXPCL_MAX_PARTICLES)
        return;

    particles[index].pclVel.p2.x = x;
    particles[index].pclVel.p2.y = y;
    particles[index].pclVel.p2.z = z;
}

//////////////////////////////////////////////////////////////////////
// Function: fxPclSetPt3Velocity
// Description:
//////////////////////////////////////////////////////////////////////
void fxPclSetPt3Velocity(SINT32 id, FLOAT32 x, FLOAT32 y, FLOAT32 z)
{
    SINT32 index = id - 1;

    if(index < 0 || index >= DEF_FXPCL_MAX_PARTICLES)
        return;

    particles[index].pclVel.p3.x = x;
    particles[index].pclVel.p3.y = y;
    particles[index].pclVel.p3.z = z;
}

//////////////////////////////////////////////////////////////////////
// Function: fxPclSetPt4Velocity
// Description:
//////////////////////////////////////////////////////////////////////
void fxPclSetPt4Velocity(SINT32 id, FLOAT32 x, FLOAT32 y, FLOAT32 z)
{
    SINT32 index = id - 1;

    if(index < 0 || index >= DEF_FXPCL_MAX_PARTICLES)
        return;

    particles[index].pclVel.p4.x = x;
    particles[index].pclVel.p4.y = y;
    particles[index].pclVel.p4.z = z;
}

//////////////////////////////////////////////////////////////////////
// Function: fxPclSetDragCoeff
// Description:
//////////////////////////////////////////////////////////////////////
void fxPclSetDragCoeff(SINT32 id, FLOAT32 fDragCoeff)
{
    SINT32 index = id - 1;

    if(index < 0 || index >= DEF_FXPCL_MAX_PARTICLES)
        return;

    particles[index].fDragCo = fDragCoeff;
}

//////////////////////////////////////////////////////////////////////
// Function: fxPclSetParticleVel
// Description:
//////////////////////////////////////////////////////////////////////
void fxPclSetParticleVel(SINT32 id, FLOAT32 x, FLOAT32 y, FLOAT32 z)
{
    SINT32 index = id - 1;

    if(index < 0 || index >= DEF_FXPCL_MAX_PARTICLES)
        return;

    particles[index].vecPVel.x = x;
    particles[index].vecPVel.y = y;
    particles[index].vecPVel.z = z;
}

//////////////////////////////////////////////////////////////////////
// Function: fxPclSetFXVel
// Description:
//////////////////////////////////////////////////////////////////////
void fxPclSetFXVel(SINT32 id, FLOAT32 x, FLOAT32 y, FLOAT32 z)
{
    SINT32 index = id - 1;

    if(index < 0 || index >= DEF_FXPCL_MAX_PARTICLES)
        return;

    particles[index].vecFXVel.x = x;
    particles[index].vecFXVel.y = y;
    particles[index].vecFXVel.z = z;
}

//////////////////////////////////////////////////////////////////////
// Function: fxPclSetSpriteNumber
// Description:
//////////////////////////////////////////////////////////////////////
void fxPclSetSpriteNumber(SINT32 id, UINT32 uNum)
{
    SINT32 index = id - 1;

    if(index < 0 || index >= DEF_FXPCL_MAX_PARTICLES)
        return;

    particles[index].uSpriteNum = uNum;
}

//////////////////////////////////////////////////////////////////////
// Function: fxPclSetStallTime
// Description:
//////////////////////////////////////////////////////////////////////
void fxPclSetStallTime(SINT32 id, FLOAT32 fStall)
{
    SINT32 index = id - 1;

    if(index < 0 || index >= DEF_FXPCL_MAX_PARTICLES)
        return;

    particles[index].fStallTime = fStall;
}

//////////////////////////////////////////////////////////////////////
// Function: fxPclSetAmmoFlags
// Description:
//////////////////////////////////////////////////////////////////////
void fxPclSetAmmoFlags(SINT32 id, UINT32 uFlags)
{
    SINT32 index = id - 1;

    if(index < 0 || index >= DEF_FXPCL_MAX_PARTICLES)
        return;

    particles[index].uAmmoFlags |= uFlags;
}

//////////////////////////////////////////////////////////////////////
// Function: fxPclSetDirection
// Description:
//////////////////////////////////////////////////////////////////////
void fxPclSetDirection(SINT32 id, FLOAT32 x, FLOAT32 y, FLOAT32 z)
{
    SINT32 index = id - 1;

    if(index < 0 || index >= DEF_FXPCL_MAX_PARTICLES)
        return;

    particles[index].vecDir.x = x;
    particles[index].vecDir.y = y;
    particles[index].vecDir.z = z;
}

//////////////////////////////////////////////////////////////////////
// Function: fxPclSetVehicle
// Description:
//////////////////////////////////////////////////////////////////////
void fxPclSetVehicle(SINT32 id, vehVehicle* lpVeh)
{
    SINT32 index = id - 1;

    if(index < 0 || index >= DEF_FXPCL_MAX_PARTICLES)
        return;

    g_mappedVehicles.insert(std::make_pair(lpVeh, id));

    particles[index].lpVehicle = lpVeh;
}

//////////////////////////////////////////////////////////////////////
// Function: fxPclSetTopLevelObject
// Description:
//////////////////////////////////////////////////////////////////////
void fxPclSetTopLevelObject(SINT32 id, OBJECT* lpObj)
{
    SINT32 index = id - 1;

    if(index < 0 || index >= DEF_FXPCL_MAX_PARTICLES)
        return;

    particles[index].lpTopLevelObj = lpObj;
}

//////////////////////////////////////////////////////////////////////
// Function: fxPclSetEmitterObject
// Description:
//////////////////////////////////////////////////////////////////////
void fxPclSetEmitterObject(SINT32 id, OBJECT* lpObj)
{
    SINT32 index = id - 1;

    if(index < 0 || index >= DEF_FXPCL_MAX_PARTICLES)
        return;

    FXPARTICLE* lpPcl = &particles[index];

    lpPcl->lpEmitterObj = fxUtlGetObject(lpObj);

    if(lpPcl->lpEmitterObj)
    {
        g_mappedObjects.insert(std::make_pair((void*)lpPcl->lpEmitterObj, id));
        lpPcl->lpEmitterObj->flag2 |= OBJECT_HAS_FX;
    }
}

//////////////////////////////////////////////////////////////////////
// Function: fxPclSetDOFObject
// Description:
//////////////////////////////////////////////////////////////////////
void fxPclSetDOFObject(SINT32 id, OBJECTDOF* lpDOF)
{
    SINT32 index = id - 1;

    if(index < 0 || index >= DEF_FXPCL_MAX_PARTICLES)
        return;

    particles[index].lpDOF = lpDOF;
}

//////////////////////////////////////////////////////////////////////
// Function: fxPclSetFXObject
// Description:
//////////////////////////////////////////////////////////////////////
void fxPclSetFXObject(SINT32 id, OBJECT* lpObj)
{
    SINT32 index = id - 1;

    if(index < 0 || index >= DEF_FXPCL_MAX_PARTICLES)
        return;

    particles[index].lpFxObj = fxUtlGetObject(lpObj);
}

//////////////////////////////////////////////////////////////////////
// Function: fxPclSetFXObject
// Description:
//////////////////////////////////////////////////////////////////////
void fxPclSetKillData(SINT32 id, void* pData)
{
    SINT32 index = id - 1;

    if(index < 0 || index >= DEF_FXPCL_MAX_PARTICLES)
        return;

    particles[index].pKillData = pData;
}

//////////////////////////////////////////////////////////////////////
// Function: fxPclSetRadius
// Description:
//////////////////////////////////////////////////////////////////////
void fxPclSetRadius(SINT32 id, FLOAT32 fRadius)
{
    SINT32 index = id - 1;

    if(index < 0 || index >= DEF_FXPCL_MAX_PARTICLES)
        return;

    particles[index].fRadius = fRadius;
}

//////////////////////////////////////////////////////////////////////
// Function: fxPclSetScale
// Description:
//////////////////////////////////////////////////////////////////////
void fxPclSetScale(SINT32 id, FLOAT32 fScale)
{
    SINT32 index = id - 1;

    if(index < 0 || index >= DEF_FXPCL_MAX_PARTICLES)
        return;

    particles[index].fScale = fScale;
}

//////////////////////////////////////////////////////////////////////
// Function: fxPclSetUserKillCallback
// Description:
//////////////////////////////////////////////////////////////////////
void fxPclSetUserKillCallback(SINT32 id, void (*pFn)(void*))
{
    SINT32 index = id - 1;

    if(index < 0 || index >= DEF_FXPCL_MAX_PARTICLES)
        return;

    particles[index].pFnUserKill = pFn;
}

//////////////////////////////////////////////////////////////////////
// Function: fxPclSetUserUpdate
// Description:
//////////////////////////////////////////////////////////////////////
void fxPclSetUserUpdate(SINT32 id, void (*pFn)(FXPARTICLE*))
{
    SINT32 index = id - 1;

    if(index < 0 || index >= DEF_FXPCL_MAX_PARTICLES)
        return;

    particles[index].pFnUserUpdate = pFn;
}

//////////////////////////////////////////////////////////////////////
// Function: fxPclSetVelocityVariance
// Description:
//////////////////////////////////////////////////////////////////////
void fxPclSetVelocityVariance(SINT32 id, FLOAT32 fVariance)
{
    SINT32 index = id - 1;

    if(index < 0 || index >= DEF_FXPCL_MAX_PARTICLES)
        return;

    particles[index].fVariance = fVariance;
}

//////////////////////////////////////////////////////////////////////
// Function: fxPclSetAmmo
// Description:
//////////////////////////////////////////////////////////////////////
void fxPclSetAmmo(SINT32 id, void* pAmmo)
{
    SINT32 index = id - 1;

    if(index < 0 || index >= DEF_FXPCL_MAX_PARTICLES)
        return;

    weapAmmo* ammo = (weapAmmo*)pAmmo;

    ammo->iParticleID = id;

    particles[index].pAmmo = pAmmo;
}

//////////////////////////////////////////////////////////////////////
// Function: fxPclSetNumPerSec
// Description:
//////////////////////////////////////////////////////////////////////
void fxPclSetNumPerSec(SINT32 id, FLOAT32 fNum)
{
    SINT32 index = id - 1;

    if(index < 0 || index >= DEF_FXPCL_MAX_PARTICLES)
        return;

    particles[index].fNumPerSec = fNum;
}

//////////////////////////////////////////////////////////////////////
// Function: fxPclSetSpacing
// Description:
//////////////////////////////////////////////////////////////////////
void fxPclSetSpacing(SINT32 id, FLOAT32 fSpace)
{
    SINT32 index = id - 1;

    if(index < 0 || index >= DEF_FXPCL_MAX_PARTICLES)
        return;

    particles[index].fSpacing = fSpace;
}

//////////////////////////////////////////////////////////////////////
// Function: fxPclSetWorldMatrix
// Description:
//////////////////////////////////////////////////////////////////////
void fxPclSetWorldMatrix(SINT32 id, piMATRIX* lpMatWorld)
{
    SINT32 index = id - 1;

    if(index < 0 || index >= DEF_FXPCL_MAX_PARTICLES)
        return;

    particles[index].matWorld = *lpMatWorld;
}

//////////////////////////////////////////////////////////////////////
// Function: fxPclInitParticle
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL fxPclInitParticle(SINT32 id)
{
    SINT32 index = id - 1;

    if(index < 0 || index >= DEF_FXPCL_MAX_PARTICLES)
        return FALSE;

    FXPARTICLE* lpPcl = &particles[index];

    lpPcl->bDraw = TRUE;

    if(lpPcl->enumType <= IDE_PARTICLE_NOTUSED)
        return FALSE;

    if(lpPcl->enumType >= IDE_NUMBER_OF_PARTICLES)
        return FALSE;

    if(g_fxInitFuncs[lpPcl->enumType].pFn == nullptr)
        return FALSE;

    if(!g_fxInitFuncs[lpPcl->enumType].pFn(lpPcl))
    {
        return FALSE;
    }

    fxUtlGetTextureUV(lpPcl->uv, lpPcl->uSpriteNum);

    // Needs to be set for initial velocity update
    lpPcl->fLastUpdate = (FLOAT32)frametime;
    lpPcl->vecLastPos = lpPcl->vecPos;

    if(lpPcl->vecRot.z < 0.0001f)
        lpPcl->vecRot.z = 1.0f;

    if(lpPcl->fVariance < 0.00001f)
        lpPcl->fVariance = 1.0f;

    if(lpPcl->fSpacing < 0.00001f)
        lpPcl->fSpacing = 1.0f;

    return _UpdatePCL(lpPcl);
}
