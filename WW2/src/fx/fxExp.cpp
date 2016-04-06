#include <syscfg.h> // Path is project specific.
/////////////////////////////////////////////////////
// Module: fxExp.cpp
// Description: Handle all explosion events.
/////////////////////////////////////////////////////
#include "fx/fxMgr.h"
#include "fx/fxExp.h"
#include "fx/fxLgt.h"
#include "fx/fxPcl.h"
#include "fx/fxUtl.h"

#include "fx/FXInterface.h"
#include "ml/mlutility.h"
#include "ld/ldpix.h"
#include "math/math3d.h"
#include "math/mathvect.h"
#include "math/matheul.h"
#include "draw/draw3d.h"
#include "trn/trnmain.h"
#include "ob/obdlite.h"
#include "ogrp/ogrptype.h"
#include "mem/mem.h"
#include "fio/fio.h"
#include "ob/obpool.h"
#include "rn/rnvis.h"
#include "pers/persmain.h"
#include "math/mathnum.h"
#include "fx/TransformEffect.h"
#include "scen/ScenarioManager.h"

#define DEF_MAX_EXPLOSIONS          128

static FXEXPLOSION* g_lpExp = nullptr;

static FXEXPLOSION g_expFreeHead;
static FXEXPLOSION g_expFreeTail;

static FXEXPLOSION g_expHead;
static FXEXPLOSION g_expTail;

static piCOLORVALUE* g_lpColor = nullptr;

static FLOAT32 g_fPi = PI;
static FLOAT32 g_fPiOver180 = 0.0f;

static LOGICAL _AddExp(FXEXPLOSION* lpExp);
static LOGICAL _AirBurst(SINT32 iID);
static LOGICAL _CheckForExps();
static LOGICAL _CreateBombExp(SINT32 iID);
static LOGICAL _CreateLight(SINT32 iID);
static LOGICAL _CreatePlaneFire(SINT32 iID, OBJECT* lpObj);
static LOGICAL _CreateShake(SINT32 iID);
static LOGICAL _CreateShockWave(SINT32 iID);
static LOGICAL _CreateSparks(FLOAT64VECTOR* pPos, FLOAT32 fPOWEnergy);
#if(0)
static LOGICAL _CreateWaterSplash(SINT32 iID);
#endif
static LOGICAL _DrawShockWave(SINT32 iID);
static LOGICAL _FreeExp(FXEXPLOSION* lpExp);
static LOGICAL _FreeIfNecessary(SINT32 iID);
static LOGICAL _GetExp(FXEXPLOSION** lpExp);
static LOGICAL _GrenadeExp(SINT32 iID);
static LOGICAL _GroundBurst(SINT32 iID);
static LOGICAL _ReleaseExp(FXEXPLOSION* lpExp);
static LOGICAL _SatchelExp(SINT32 iID);
static LOGICAL _UpdateShake(SINT32 iID);
static LOGICAL _VehicleExplosion(SINT32 iID);

static void _UpdateShockWave(SINT32 iID);

#define USE_FX2 1

////////////////////////////////////
// Function: fxExpInit
// Description: Initialize Exps.
////////////////////////////////////
LOGICAL fxExpInit()
{
    SINT32 i;

    g_lpExp = (FXEXPLOSION*)memAlloc(sizeof(FXEXPLOSION) * DEF_MAX_EXPLOSIONS);

    if(g_lpExp == nullptr)
        return FALSE;

    memset(&g_expHead, 0, sizeof(FXEXPLOSION));
    memset(&g_expTail, 0, sizeof(FXEXPLOSION));

    g_expHead.next = &g_expTail;
    g_expTail.prev = &g_expHead;

    memset(&g_expFreeHead, 0, sizeof(FXEXPLOSION));
    memset(&g_expFreeTail, 0, sizeof(FXEXPLOSION));

    g_expFreeHead.next = &g_expFreeTail;
    g_expFreeTail.prev = &g_expFreeHead;

    for(i = 0; i < DEF_MAX_EXPLOSIONS; i++)
    {
        g_lpExp[i].iIndex = i;
        _FreeExp(&g_lpExp[i]);
    }

    g_fPiOver180 = g_fPi / 180.0f;

    return TRUE;
}

//////////////////////////////////////////////
// Function: fxExpFreeAll
// Description: Release all explosions
//////////////////////////////////////////////
void fxExpFreeAll()
{
    FXEXPLOSION* lpExp = g_expHead.next;
    FXEXPLOSION* lpTail = &g_expTail;
    FXEXPLOSION* lpFree = nullptr;

    if(lpExp == nullptr) return;

    while(lpExp != lpTail)
    {
        lpFree = lpExp;
        lpExp = lpExp->next;

        _ReleaseExp(lpFree);
        _FreeExp(lpFree);
    }
}

//////////////////////////////////////////////
// Function: fxExpKill
// Description: Release all
//////////////////////////////////////////////
LOGICAL fxExpKill()
{
    if(g_lpExp != nullptr)
    {
        memFree(g_lpExp);
        g_lpExp = nullptr;
    }

    // Set to nullptr in case of further updates to Explosions.
    memset(&g_expHead, 0, sizeof(FXEXPLOSION));
    memset(&g_expTail, 0, sizeof(FXEXPLOSION));
    memset(&g_expFreeHead, 0, sizeof(FXEXPLOSION));
    memset(&g_expFreeTail, 0, sizeof(FXEXPLOSION));

    return TRUE;
}

//////////////////////////////////////////////
// Function: fxExpInitExplosion
// Description:
//////////////////////////////////////////////
void fxExpInitExplosion(SINT32 iID)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_EXPLOSIONS)
        return;

    FLOAT32 fEnergy = (FLOAT32)pow(g_lpExp[iIndex].fEnergy, 0.333);

    fxExpSetPowEnergy(iID, fEnergy);
    fxUtlUpdatePosition(&g_lpExp[iIndex].vecPt, &g_lpExp[iIndex].vecPos);

    switch(g_lpExp[iIndex].enumType)
    {
    case IDE_EXPTYPE_AMMOAIR:
        {
            _AirBurst(iID);
            break;
        }
    case IDE_EXPTYPE_AMMOGROUND:
        {
            _GroundBurst(iID);
            break;
        }
    case IDE_EXPTYPE_VEHICLECRASH:
        {
            _VehicleExplosion(iID);
            break;
        }
    case IDE_EXPTYPE_SATCHEL:
        {
            _SatchelExp(iID);
            break;
        }
    default:
        // don't care about other explosion types
        break;
    }
}

//////////////////////////////////////////////////////////////////////
// Function: _SatchelExp
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL _SatchelExp(SINT32 ID)
{
    Vec3d p(g_lpExp[ID-1].vecPos);
    p.z() = -p.z();

    Vec3f n(g_lpExp[ID-1].n);

    TransformEffect* te = nullptr;

    if(g_lpExp[ID-1].bIsWater)
    {
        te = TransformEffect::AddEffect("water_satchelgeneric", n, 0, nullptr);
    }
    else
    {
        te = TransformEffect::AddEffect("satchelgeneric", n, 0, nullptr);
    }

    if(te)
    {
        te->enableemitters(true);;
        te->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);
        te->setmatrix(Matrixf::translate(p));
    }

    _CreateLight(ID);

    // Shake the screen.
    _CreateShake(ID);

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _VehicleExplosion
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL _VehicleExplosion(SINT32 id)
{
    OBJECT* o = nullptr;
    o = g_lpExp[id-1].lpVehAttach->VehicleObjectData->PlaneObject;

    fxPclCreateExplosion(&g_lpExp[id-1].vecPos, g_lpExp[id-1].fEnergy);

    _CreatePlaneFire(id, o);

    _CreateLight(id);
    _CreateShockWave(id);

    // Shake the screen
    _CreateShake(id);

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _AirBurst
// Description: Create explosion for airbursts.
//////////////////////////////////////////////////////////////////////
LOGICAL _AirBurst(SINT32 iID)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_EXPLOSIONS)
        return FALSE;

    fxExpSetRadius(iID, g_lpExp[iIndex].fPowEnergy * 0.005f);

    switch(g_lpExp[iIndex].uSubType)
    {
    case WEAPAMMO_SUBTYPE_BOMB:
    case WEAPAMMO_SUBTYPE_NUKE:
        {
            _CreateShockWave(iID);
            _CreateLight(iID);

            fxPclCreateExplosion(&g_lpExp[iIndex].vecPos, g_lpExp[iIndex].fEnergy);
            break;
        }
    case WEAPAMMO_SUBTYPE_AIRBURST:
        {
            _CreateLight(iID);

            TransformEffect* te = TransformEffect::AddEffect("flak", Vec3f(0, 0, 0), 0, nullptr);

            if(te)
            {
                Vec3d p(g_lpExp[iIndex].vecPos);
                p.z() = -p.z();

                te->enableemitters(true);;
                te->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);
                te->setmatrix(Matrixf::translate(p));
            }

            break;
        }
    case WEAPAMMO_SUBTYPE_GRENADE:
    case WEAPAMMO_SUBTYPE_MORTAR:
        {
            _GrenadeExp(iID);
            break;
        }
    default:
        {
            _CreateLight(iID);

            TransformEffect* te = TransformEffect::AddEffect("smallcannon", Vec3f(0, 0, 0), 0, nullptr);

            if(te)
            {
                Vec3d p(g_lpExp[iIndex].vecPos);
                p.z() = -p.z();

                te->enableemitters(true);;
                te->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);
                te->setmatrix(Matrixf::translate(p));
            }
            break;
        }
    }

    if(!_CreateShake(iID))
        return FALSE;

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _GroundBurst
// Description: Create ground burst.
//////////////////////////////////////////////////////////////////////
LOGICAL _GroundBurst(SINT32 ID)
{
    SINT32 i = ID - 1;

    if(i < 0 || i >= DEF_MAX_EXPLOSIONS)
        return FALSE;

    if(g_lpExp[i].fPowEnergy <= 0.0001f)
        return FALSE;

    fxExpSetRadius(ID, g_lpExp[i].fPowEnergy * 0.005f);

    switch(g_lpExp[i].uSubType)
    {
    case WEAPAMMO_SUBTYPE_BOMB:
    case WEAPAMMO_SUBTYPE_NUKE:
        {
            _CreateShockWave(ID);
            _CreateLight(ID);
            _CreateBombExp(ID);
            break;
        }
    case WEAPAMMO_SUBTYPE_AIRBURST:
        {
            _CreateLight(ID);

            if(g_lpExp[i].bIsWater)
            {
                fxPclCreateH2OExplosion(&g_lpExp[i].vecPos, g_lpExp[i].fEnergy);
            }
            else
            {
                fxPclCreateExplosion(&g_lpExp[i].vecPos, g_lpExp[i].fEnergy);
            }

            break;
        }
    case WEAPAMMO_SUBTYPE_GRENADE:
    case WEAPAMMO_SUBTYPE_MORTAR:
        {
            _GrenadeExp(ID);
            break;
        }
    default:
        {
            _CreateLight(ID);

            if(g_lpExp[i].bIsWater)
            {
                fxPclCreateH2OExplosion(&g_lpExp[i].vecPos, g_lpExp[i].fEnergy);
            }
            else
            {
                fxPclCreateExplosion(&g_lpExp[i].vecPos, g_lpExp[i].fEnergy);
            }

            break;
        }
    }

    _CreateShake(ID);

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _GrenadeExp
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL _GrenadeExp(SINT32 id)
{
    TransformEffect* te = nullptr;

    if(g_lpExp[id-1].bIsWater)
    {
        te = TransformEffect::AddEffect("watergrenade", Vec3f(g_lpExp[id-1].n), 0, nullptr);
    }
    else
    {
        te = TransformEffect::AddEffect("grenade", Vec3f(g_lpExp[id-1].n), 0, nullptr);
    }

    if(te)
    {
        Vec3d p(g_lpExp[id-1].vecPos);
        p.z() = -p.z();

        te->enableemitters(true);;
        te->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);
        te->setmatrix(Matrixf::translate(p));
    }

    _CreateLight(id);

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _CreateWaterSplash
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL _CreateWaterSplash(SINT32 iID)
{
#if(0)
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_EXPLOSIONS)
        return FALSE;

    SINT32 iWtrID = fxWtrGetNewWater();

    if(iWtrID == -1)
        return FALSE;

    fxWtrSetRadius(iWtrID, g_lpExp[iIndex].fPowEnergy * 0.10f);
    fxWtrSetScale(iWtrID, 1.0f);
    fxWtrSetStall(iWtrID, 1);

    fxWtrSetPosition(iWtrID,
                     g_lpExp[iIndex].vecPos.x,
                     g_lpExp[iIndex].vecPos.y,
                     g_lpExp[iIndex].vecPos.z);

    fxWtrSetType(iWtrID, IDE_WATERTYPE_HIT);
    fxWtrPushFlags(iWtrID, DEF_FXFLAG_INTRADIUS);
    fxWtrPushFlags(iWtrID, DEF_FXFLAG_INTALPHA);
    fxWtrSet4PtScale(iWtrID, 0.5f, 1.0f, 1.3f, 0.8f);
    fxWtrSet4PtAlpha(iWtrID, 1.0f, 1.0f, 1.0f, 0.0f);
    fxWtrSetPt1Time(iWtrID, (FLOAT32)frametime + 150.0f);
    fxWtrSetPt2Time(iWtrID, (FLOAT32)frametime + 200.0f);
    fxWtrSetPt3Time(iWtrID, (FLOAT32)frametime + 300.0f);
    fxWtrSetPt4Time(iWtrID, (FLOAT32)frametime + 5300.0f);

    fxWtrInitWater(iWtrID);

    iWtrID = fxWtrGetNewWater();

    if(iWtrID == -1)
        return FALSE;

    fxWtrSetRadius(iWtrID, g_lpExp[iIndex].fPowEnergy * 0.06f);
    fxWtrSetScale(iWtrID, 1.0f);

    fxWtrSetPosition(iWtrID,
                     g_lpExp[iIndex].vecPos.x,
                     g_lpExp[iIndex].vecPos.y,
                     g_lpExp[iIndex].vecPos.z);

    fxWtrSetType(iWtrID, IDE_WATERTYPE_MIST);
    fxWtrPushFlags(iWtrID, DEF_FXFLAG_INTRADIUS);
    fxWtrPushFlags(iWtrID, DEF_FXFLAG_INTALPHA);
    fxWtrSetType(iWtrID, IDE_WATERTYPE_MIST);
    fxWtrSet4PtScale(iWtrID, 1.0f, 1.1f, 1.2f, 1.3f);
    fxWtrSet4PtAlpha(iWtrID, 1.0f, 1.0f, 1.0f, 0.0f);
    fxWtrSetPt1Time(iWtrID, (FLOAT32)frametime + 150.0f);
    fxWtrSetPt2Time(iWtrID, (FLOAT32)frametime + 650.0f);
    fxWtrSetPt3Time(iWtrID, (FLOAT32)frametime + 1650.0f);
    fxWtrSetPt4Time(iWtrID, (FLOAT32)frametime + 3650.0f);

    fxWtrInitWater(iWtrID);

    // Don't need ripples if in aircraft.
    if(persIsPersonaVehicleType(VEH_TYPE_AIR))
        return TRUE;

    iWtrID = fxWtrGetNewWater();

    if(iWtrID == -1)
        return FALSE;

    fxWtrSetType(iWtrID, IDE_WATERTYPE_RIPPLE);
    fxWtrSetRadius(iWtrID, g_lpExp[iIndex].fPowEnergy * 0.10f);
    fxWtrSetScale(iWtrID, 1.0f);

    fxWtrSetPosition(iWtrID,
                     g_lpExp[iIndex].vecPos.x,
                     g_lpExp[iIndex].vecPos.y,
                     g_lpExp[iIndex].vecPos.z);

    fxWtrPushFlags(iWtrID, DEF_FXFLAG_INTRADIUS);
    fxWtrPushFlags(iWtrID, DEF_FXFLAG_INTALPHA);
    fxWtrSetPt1Time(iWtrID, (FLOAT32)frametime + 150.0f);
    fxWtrSetPt2Time(iWtrID, (FLOAT32)frametime + 350.0f);
    fxWtrSetPt3Time(iWtrID, (FLOAT32)frametime + 550.0f);
    fxWtrSetPt4Time(iWtrID, (FLOAT32)frametime + 1250.0f);
    fxWtrSet4PtScale(iWtrID, 0.4f, 0.6f, 0.7f, 1.2f);
    fxWtrSet4PtAlpha(iWtrID, 0.8f, 0.7f, 0.6f, 0.0f);
    fxWtrSetEmitterObject(iWtrID, g_lpExp[iIndex].lpEmitterObj);
    fxWtrInitWater(iWtrID);
#endif
    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _CreatePlaneFire
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL _CreatePlaneFire(SINT32 iID, OBJECT* lpObj)
{
    SINT32 iIndex = iID - 1;
    SCHAR szBuff[3][16] = {"efire", "efire1", "efire2"};
    OBJECT* fireobj = nullptr;

    if(iIndex < 0 || iIndex >= DEF_MAX_EXPLOSIONS)
        return FALSE;

    if(lpObj == nullptr)
        return FALSE;

    TransformEffect* te;

    for(SINT32 i = 0; i < 3; i++)
    {
        ogrpFindSubObjectByName(&fireobj, lpObj, szBuff[i]);

        if(fireobj == nullptr)
            continue;

        te = TransformEffect::AddEffect("planecrashfire", Vec3f(0, 0, -1), 0, fireobj);

        if(te)
        {
            te->enableemitters(true);;
            te->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);
        }
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _CreateBombExp
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL _CreateBombExp(SINT32 id)
{
    if(g_lpExp[id-1].bIsWater)
    {
        fxPclCreateH2OExplosion(&g_lpExp[id-1].vecPos, g_lpExp[id-1].fEnergy);
    }
    else
    {
        if(g_lpExp[id-1].ammo && g_lpExp[id-1].ammo->AmmoType && g_lpExp[id-1].ammo->AmmoType->Name[0])
        {
            TransformEffect* te = TransformEffect::AddEffect((char*)g_lpExp[id-1].ammo->AmmoType->Name,
                Vec3f(g_lpExp[id-1].n), 0, nullptr);

            if(te)
            {
                Vec3d p(g_lpExp[id-1].vecPos);
                p.z() = -p.z();

                te->enableemitters(true);;
                te->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);
                te->setmatrix(Matrixf::translate(p));
            }
        }
        else
        {
            fxPclCreateExplosion(&g_lpExp[id-1].vecPos, g_lpExp[id-1].fEnergy);
        }
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _CreateSparks
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL _CreateSparks(FLOAT64VECTOR* pPos, FLOAT32 energy)
{
    TransformEffect* te = TransformEffect::AddEffect("manysparks", Vec3f(0, 0, -1), 0, nullptr);

    if(te)
    {
        Vec3d p(*pPos);
        p.z() = -p.z();

        te->enableemitters(true);;
        te->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);
        te->setmatrix(Matrixf::translate(p));
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _CreateLight
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL _CreateLight(SINT32 iID)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_EXPLOSIONS)
        return FALSE;

    SINT32 iLightID = fxLgtGetNewLight();

    if(iLightID == -1)
        return FALSE;

    FLOAT32 fLgt = g_lpExp[iIndex].fPowEnergy;

    fxLgtPushFlags(iLightID, DEF_FXFLAG_INTATTENUATION);
    fxLgtPushFlags(iLightID, DEF_FXFLAG_INTALPHA);
    fxLgtPushFlags(iLightID, DEF_FXFLAG_INTRADIUS);
    fxLgtSetType(iLightID, IDE_LIGHT_BOMB);
    fxLgtSet4PtAttenuation(iLightID, 0.08f, 0.5f, 1.0f, 10.0f);
    fxLgtSetPt1Time(iLightID, (FLOAT32)frametime);
    fxLgtSetPt2Time(iLightID, (FLOAT32)frametime + fLgt);
    fxLgtSetPt3Time(iLightID, (FLOAT32)frametime + fLgt + fLgt);
    fxLgtSetPt4Time(iLightID, (FLOAT32)frametime + fLgt + fLgt + fLgt);
    fxLgtSetPt1Color(iLightID, 1.0f, 1.0f, 1.0f);
    fxLgtSetPt2Color(iLightID, 1.0f, 1.0f, 1.0f);
    fxLgtSetPt3Color(iLightID, 1.0f, 1.0f, 1.0f);
    fxLgtSetPt4Color(iLightID, 1.0f, 1.0f, 1.0f);
    fxLgtSet4PtAlpha(iLightID, 1.0f, 1.0f, 1.0f, 1.0f);

    fxLgtSetPosition(iLightID,
                     g_lpExp[iIndex].vecPos.x,
                     g_lpExp[iIndex].vecPos.y,
                     g_lpExp[iIndex].vecPos.z);

    // Required after settings
    fxLgtInitLight(iLightID);

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _CreateShake
// Description: Shake view for concussion.
//////////////////////////////////////////////////////////////////////
LOGICAL _CreateShake(SINT32 iID)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_EXPLOSIONS)
        return FALSE;

    FLOAT32 fSpeedOfSound = 344.0f;
    FLOAT32 fMultiplier = 1.0f;
    FLOAT32 fDistance = 0.0f;

    // Only supported for ground vehicles now.
    if(persIsPersonaVehicleType(VEH_TYPE_GROUND))
    {
        // Shake tied to vehicle mass.
        mlVehicleData* pData = persGetPersonaModelDataPtr();

        if(pData == nullptr)
            return FALSE;

        if(pData->VehicleMass > 100.0f)
        {
            // One of many ways to create a muliplier.
            fMultiplier = (FLOAT32)pow(pData->VehicleMass, 0.2);
            fMultiplier = fMultiplier < 1.0f ? 1.0f : 1.0f / fMultiplier;
        }
    }

    fDistance = fxUtlGetDistanceSqrd(&g_lpExp[iIndex].vecPt);

    fDistance = math3dFastSqrt(fDistance);

    if(fDistance < 0.001f)
        fDistance = 0.01f;

    g_lpExp[iIndex].expShake.fTimeToStart = (fDistance / fSpeedOfSound) * 1000.0f;
    g_lpExp[iIndex].expShake.fTimeToStart += (FLOAT32)frametime;

    if(g_lpExp[iIndex].expShake.fTimeToStart - (FLOAT32)frametime >= 2000.0f)
    {
        g_lpExp[iIndex].expShake.bShake = FALSE;
        return FALSE;
    }

    g_lpExp[iIndex].expShake.bShake = TRUE;

    FLOAT32 fEnergy = g_lpExp[iIndex].fPowEnergy;

    // 0.0025 is arbitrary
    g_lpExp[iIndex].expShake.fShakeStart = (fEnergy / fDistance) * 0.0025f;
    g_lpExp[iIndex].expShake.fShakeStart *= fMultiplier;

    // 0.004 is arbitrary. Shake too small to notice...
    if(g_lpExp[iIndex].expShake.fShakeStart < 0.004f)
    {
        g_lpExp[iIndex].expShake.bShake = FALSE;
        return FALSE;
    }

    g_lpExp[iIndex].expShake.fShakeCurrent = g_lpExp[iIndex].expShake.fShakeStart;
    g_lpExp[iIndex].expShake.fShakeTime = 2000.0f;

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _CreateShockWave
// Description: Set up shock wave parameters.
// [DCF]**FIXME**
//////////////////////////////////////////////////////////////////////
LOGICAL _CreateShockWave(SINT32 iID)
{
    if(gCustomTerrain)
        return FALSE;

    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_EXPLOSIONS)
        return FALSE;

    g_lpExp[iIndex].bShockWave = TRUE;

    g_lpExp[iIndex].expWave.p1.a = 0.5f;
    g_lpExp[iIndex].expWave.p2.a = 0.3f;
    g_lpExp[iIndex].expWave.p3.a = 0.1f;
    g_lpExp[iIndex].expWave.p4.a = 0.0f;

    g_lpExp[iIndex].piColor.a = g_lpColor->a;
    g_lpExp[iIndex].piColor.r = g_lpColor->r;
    g_lpExp[iIndex].piColor.g = g_lpColor->g;
    g_lpExp[iIndex].piColor.b = g_lpColor->b;

    FLOAT32 fTime = g_lpExp[iIndex].fPowEnergy * 0.5f;

    g_lpExp[iIndex].waveTime.p1 = (FLOAT32)frametime;
    g_lpExp[iIndex].waveTime.p2 = g_lpExp[iIndex].waveTime.p1 + fTime;
    g_lpExp[iIndex].waveTime.p3 = g_lpExp[iIndex].waveTime.p2 + fTime;
    g_lpExp[iIndex].waveTime.p4 = g_lpExp[iIndex].waveTime.p3 + fTime;

    g_lpExp[iIndex].vecWave[0].x -= 0.5f;
    g_lpExp[iIndex].vecWave[0].y -= 0.5f;
    g_lpExp[iIndex].vecWave[0].z -= 0.2f;

    g_lpExp[iIndex].vecWave[1].x += 0.5f;
    g_lpExp[iIndex].vecWave[1].y -= 0.5f;
    g_lpExp[iIndex].vecWave[1].z -= 0.2f;

    g_lpExp[iIndex].vecWave[2].x += 0.5f;
    g_lpExp[iIndex].vecWave[2].y += 0.5f;
    g_lpExp[iIndex].vecWave[2].z -= 0.2f;

    g_lpExp[iIndex].vecWave[3].x -= 0.5f;
    g_lpExp[iIndex].vecWave[3].y += 0.5f;
    g_lpExp[iIndex].vecWave[3].z -= 0.2f;

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: fxExpDraw
// Description: Draw Explosion hit marks.
//////////////////////////////////////////////////////////////////////
LOGICAL fxExpDraw(SINT32 iIndex)
{
    if(g_expHead.next == nullptr)
        return FALSE;

    if(g_expHead.next == &g_expTail)
        return FALSE;

    piSetRenderState(piRENDERSTATE_LIGHTING, FALSE);
    piDisableMaterial();

    if(g_lpExp[iIndex].expShake.bShake)
    {
        _UpdateShake(iIndex + 1);
    }

    if(g_lpExp[iIndex].bShockWave)
    {
        _DrawShockWave(iIndex + 1);
    }

    _FreeIfNecessary(iIndex + 1);
    piSetRenderState(piRENDERSTATE_LIGHTING, piLIGHTING_DEFAULT);

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _FreeIfNecessary
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL _FreeIfNecessary(SINT32 iID)
{
    SINT32 iIndex = iID - 1;

#ifdef AIMING_WEAPONS
    return FALSE;
#endif

    if(iIndex < 0 || iIndex >= DEF_MAX_EXPLOSIONS)
        return FALSE;

    if(!g_lpExp[iIndex].expShake.bShake && !g_lpExp[iIndex].bShockWave)
    {
        _ReleaseExp(&g_lpExp[iIndex]);
        _FreeExp(&g_lpExp[iIndex]);
    }

    return TRUE;
}

///////////////////////////////////	///////////////////////////////////////
// Function: _UpdateShake
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL _UpdateShake(SINT32 iID)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_EXPLOSIONS)
        return FALSE;

    FLOAT32 fTimePassed = (FLOAT32)frametime - g_lpExp[iIndex].expTime.p1;
    FLOAT32 fShakeAmt = 1.0f - (fTimePassed / g_lpExp[iIndex].expShake.fShakeTime);

    g_lpExp[iIndex].expShake.fShakeCurrent = fShakeAmt * g_lpExp[iIndex].expShake.fShakeStart;

    if(g_lpExp[iIndex].expShake.fShakeCurrent <= 0.0f)
    {
        g_lpExp[iIndex].expShake.fShakeCurrent = 0.0f;
        g_lpExp[iIndex].expShake.bShake = FALSE;
    }

	SINT32 duration = (g_lpExp[iIndex].expShake.fTimeToStart - (FLOAT32)frametime <= 0.0f) * 1000;

    if(duration > 0 && duration <= 2000)
	{
        persShakeCamera(g_lpExp[iIndex].expShake.fShakeCurrent, duration);
	}
	else
	{
		// we're done shaking
		g_lpExp[iIndex].expShake.fShakeCurrent = 0.0f;
		g_lpExp[iIndex].expShake.bShake = FALSE;
	}

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _DrawShockWave
// Description: Animate shockwave...
//////////////////////////////////////////////////////////////////////
LOGICAL _DrawShockWave(SINT32 iID)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_EXPLOSIONS)
        return FALSE;

    piMATRIX matOldWorld = *piGetWorldMatrix();
    UINT32 uColor = 0;

    _UpdateShockWave(iID);

    piSetWorldMatrix(&g_lpExp[iIndex].matSize);

    uColor = piRGBA(g_lpExp[iIndex].piColor.r,
                    g_lpExp[iIndex].piColor.g,
                    g_lpExp[iIndex].piColor.b,
                    g_lpExp[iIndex].piColor.a);

    piSetRenderState(piRENDERSTATE_CULLFACEENABLE, 0);

    piUV uv[2];
    // 34 is shock wave on fx.dds
    fxUtlGetTextureUV(uv, 35);

    piVECTOR* pVec = g_lpExp[iIndex].vecWave;
    piLVERTEX lVert[4];
    piINDICES piIndices[2];

    lVert[0].color = uColor;
    lVert[0].specular = colorALPHA100;

    lVert[1] = lVert[0];
    lVert[2] = lVert[0];
    lVert[3] = lVert[0];

    lVert[0].x = pVec[0].x;
    lVert[0].y = pVec[0].y;
    lVert[0].z = pVec[0].z;

    lVert[1].x = pVec[1].x;
    lVert[1].y = pVec[1].y;
    lVert[1].z = pVec[1].z;

    lVert[2].x = pVec[2].x;
    lVert[2].y = pVec[2].y;
    lVert[2].z = pVec[2].z;

    lVert[3].x = pVec[3].x;
    lVert[3].y = pVec[3].y;
    lVert[3].z = pVec[3].z;

    lVert[0].tu = uv[0].u;
    lVert[0].tv = uv[1].v;

    lVert[1].tu = uv[0].u;
    lVert[1].tv = uv[0].v;

    lVert[2].tu = uv[1].u;
    lVert[2].tv = uv[0].v;

    lVert[3].tu = uv[1].u;
    lVert[3].tv = uv[1].v;

    piIndices[0].v1 = 0;
    piIndices[0].v2 = 1;
    piIndices[0].v3 = 3;

    piIndices[1].v1 = 1;
    piIndices[1].v2 = 2;
    piIndices[1].v3 = 3;

    piRenderIndexScene(piPT_TRIANGLELIST,
                       piVT_LVERTEX,
                       lVert,
                       4,
                       (UINT16*)piIndices,
                       6);

    piSetWorldMatrix(&matOldWorld);
    piSetRenderState(piRENDERSTATE_CULLFACEENABLE, piCULLFACEENABLE_DEFAULT);

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _UpdateShockWave
// Description:
//////////////////////////////////////////////////////////////////////
void _UpdateShockWave(SINT32 iID)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_EXPLOSIONS)
        return;

    FLOAT32 fSpeedOfSound = 344.0f;
    FLOAT32 fRadiusPerCycle = 0.0f;
    FLOAT32 fScale = 0.0f;

    if((FLOAT32)frametime > g_lpExp[iIndex].waveTime.p4)
        g_lpExp[iIndex].bShockWave = FALSE;

    if(g_lpExp[iIndex].bShockWave == FALSE)
        return;

    fRadiusPerCycle = (frametime - g_lpExp[iIndex].expTime.p1) * 0.001f;
    fRadiusPerCycle *= fSpeedOfSound;

    // Scale size
    fScale = fRadiusPerCycle * 0.5f;

    if(g_lpExp[iIndex].expTime.p1 == (FLOAT32)frametime)
    {
        g_lpExp[iIndex].matSize.m[3][0] = g_lpExp[iIndex].vecPt.x;
        g_lpExp[iIndex].matSize.m[3][1] = g_lpExp[iIndex].vecPt.y;

        g_lpExp[iIndex].matSize.m[3][2] = g_lpExp[iIndex].vecPt.z -
                                          g_lpExp[iIndex].fRadius * 0.07f;
    }

    g_lpExp[iIndex].matSize.m[0][0] = fScale;
    g_lpExp[iIndex].matSize.m[1][1] = fScale;
    g_lpExp[iIndex].matSize.m[2][2] = 0.0f;

    fxUtlInterpolateAlpha(&g_lpExp[iIndex].waveTime,
                          &g_lpExp[iIndex].expWave,
                          &g_lpExp[iIndex].piColor);
}

//////////////////////////////////////////////
// Function: _FreeExp
// Description:
//////////////////////////////////////////////
LOGICAL _FreeExp(FXEXPLOSION* lpExp)
{
    if(lpExp == nullptr)
        return FALSE;

    FXEXPLOSION* lpPrev = nullptr;

    SINT32 iIndex = lpExp->iIndex;

    memset(lpExp, 0, sizeof(FXEXPLOSION));

    lpExp->iIndex = iIndex;

    lpPrev = g_expFreeTail.prev;

    lpExp->prev = lpPrev;
    lpExp->next = &g_expFreeTail;

    lpPrev->next = lpExp;
    g_expFreeTail.prev = lpExp;

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _AddExp
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL _AddExp(FXEXPLOSION* lpExp)
{
    FXEXPLOSION* lpNext = nullptr;

    if(lpExp == nullptr)
        return FALSE;

    lpNext = g_expHead.next;

    g_expHead.next = lpExp;
    lpExp->prev = &g_expHead;

    lpExp->next = lpNext;
    lpNext->prev = lpExp;

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _GetExp
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL _GetExp(FXEXPLOSION** lpExp)
{
    FXEXPLOSION* lpNode = nullptr;
    FXEXPLOSION* lpNext = nullptr;
    FXEXPLOSION* lpPrev = nullptr;

    if(lpExp == nullptr)
        return FALSE;

    if(!_CheckForExps())
        return FALSE;

    lpNode = g_expFreeHead.next;

    lpPrev = lpNode->prev;
    lpNext = lpNode->next;

    lpPrev->next = lpNext;
    lpNext->prev = lpPrev;

    lpNode->next = nullptr;
    lpNode->prev = nullptr;

    lpNode->matSize = IdentityMatrix;

    *lpExp = lpNode;

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _CheckForExps
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL _CheckForExps()
{
    if(g_expFreeHead.next == &g_expFreeTail)
    {
        FXEXPLOSION* lpExp = nullptr;

        if(g_expTail.prev == &g_expHead)
            return FALSE;

        lpExp = g_expTail.prev;

        if(!_ReleaseExp(lpExp))
            return FALSE;

        if(!_FreeExp(lpExp))
            return FALSE;
    }

    if(g_expFreeHead.next == &g_expFreeTail)
        return FALSE;

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _ReleaseExp
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL _ReleaseExp(FXEXPLOSION* lpExp)
{
    FXEXPLOSION* lpNext = nullptr;
    FXEXPLOSION* lpPrev = nullptr;

    if(lpExp == nullptr)
        return FALSE;

    lpNext = lpExp->next;
    lpPrev = lpExp->prev;

    if(lpNext == nullptr || lpPrev == nullptr)
        return FALSE;

    lpPrev->next = lpNext;
    lpNext->prev = lpPrev;

    lpExp->prev = nullptr;
    lpExp->next = nullptr;

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: fxExpSetColor
// Description:
//////////////////////////////////////////////////////////////////////
void fxExpSetColor(piCOLORVALUE* lpColor)
{
    g_lpColor = lpColor;
}

//////////////////////////////////////////
// Function: fxExpUpdate
// Desctiption:
//////////////////////////////////////////
void fxExpUpdate()
{
    if(g_expHead.next == nullptr) return;
    if(g_expHead.next == &g_expTail) return;

    FXEXPLOSION* lpExp = g_expHead.next;
    FXEXPLOSION* lpTail = &g_expTail;

    for(; lpExp != lpTail; lpExp = lpExp->next)
    {
        fxUtlUpdatePosition(&lpExp->vecPt, &lpExp->vecPos);
        lpExp->fDistanceSqrd = fxUtlGetDistanceSqrd(&lpExp->vecPt);
    }
}

//////////////////////////////////////////
// Function: fxExpGetList
// Desctiption:
//////////////////////////////////////////
void fxExpGetList(void* lpExpSort, SINT32* pIndex)
{
    FXSORT* lpSort = (FXSORT*)lpExpSort;

    if(!lpSort || !pIndex) return;
    if(g_expHead.next == nullptr) return;
    if(g_expHead.next == &g_expTail) return;

    FXEXPLOSION* lpExp = g_expHead.next;
    FXEXPLOSION* lpTail = &g_expTail;

    for(; lpExp != lpTail; lpExp = lpExp->next)
    {
        if(*pIndex >= DEF_MAX_EFFECTS)
            break;

        lpSort[*pIndex].type = IDE_FXTYPE_EXPLOSION;
        lpSort[*pIndex].dist2 = lpExp->fDistanceSqrd;
        lpSort[*pIndex].index = lpExp->iIndex;

        (*pIndex)++;
    }
}

//////////////////////////////////////////////////////////////////////
// Function: fxExpCreateGenericExp
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL fxExpCreateGenericExp(FLOAT64VECTOR* pPos, FLOAT64 fEnergy)
{
    FLOAT32 fPow = (FLOAT32)pow(fEnergy, 0.25);

    fxPclCreateExplosion(pPos, fEnergy);
    _CreateSparks(pPos, fPow);

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: fxExpSetEmitterObject
// Description:
//////////////////////////////////////////////////////////////////////
void fxExpSetEmitterObject(SINT32 iID, OBJECT* lpObj)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_EXPLOSIONS)
        return;

    g_lpExp[iIndex].lpEmitterObj = fxUtlGetObject(lpObj);

    if(g_lpExp[iIndex].lpEmitterObj)
    {
        g_lpExp[iIndex].lpEmitterObj->flag2 |= OBJECT_HAS_FX;
    }
}

//////////////////////////////////////////////////////////////////////
// Function: fxExpSetEnergy
// Description:
//////////////////////////////////////////////////////////////////////
void fxExpSetEnergy(SINT32 iID, FLOAT64 fEnergy)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_EXPLOSIONS)
        return;

    g_lpExp[iIndex].fEnergy = fEnergy;
}

//////////////////////////////////////////////////////////////////////
// Function: fxExpSetPowEnergy
// Description:
//////////////////////////////////////////////////////////////////////
void fxExpSetPowEnergy(SINT32 iID, FLOAT32 fEnergy)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_EXPLOSIONS)
        return;

    g_lpExp[iIndex].fPowEnergy = fEnergy;
}

//////////////////////////////////////////////////////////////////////
// Function: fxExpSetIsWater
// Description:
//////////////////////////////////////////////////////////////////////
void fxExpSetIsWater(SINT32 iID, LOGICAL bIsWater)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_EXPLOSIONS)
        return;

    g_lpExp[iIndex].bIsWater = bIsWater;
}

//////////////////////////////////////////////////////////////////////
// Function: fxExpSetNormal
// Description:
//////////////////////////////////////////////////////////////////////
void fxExpSetNormal(SINT32 iID, FLOAT32 x, FLOAT32 y, FLOAT32 z)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_EXPLOSIONS)
        return;

    g_lpExp[iIndex].n.x = x;
    g_lpExp[iIndex].n.y = y;
    g_lpExp[iIndex].n.z = z;
}

//////////////////////////////////////////////////////////////////////
// Function: fxExpSetPosition
// Description:
//////////////////////////////////////////////////////////////////////
void fxExpSetPosition(SINT32 iID, FLOAT64 x, FLOAT64 y, FLOAT64 z)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_EXPLOSIONS)
        return;

    g_lpExp[iIndex].vecPos.x = x;
    g_lpExp[iIndex].vecPos.y = y;
    g_lpExp[iIndex].vecPos.z = z;
}

//////////////////////////////////////////////////////////////////////
// Function: fxExpSetSubType
// Description:
//////////////////////////////////////////////////////////////////////
void fxExpSetSubType(SINT32 iID, UINT32 uSubType)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_EXPLOSIONS)
        return;

    g_lpExp[iIndex].uSubType = uSubType;
}

//////////////////////////////////////////////////////////////////////
// Function: fxExpSetTopLevelObject
// Description:
//////////////////////////////////////////////////////////////////////
void fxExpSetTopLevelObject(SINT32 iID, OBJECT* lpObj)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_EXPLOSIONS)
        return;

    g_lpExp[iIndex].lpTop = lpObj;
}

//////////////////////////////////////////////////////////////////////
// Function: fxExpSetType
// Description:
//////////////////////////////////////////////////////////////////////
void fxExpSetType(SINT32 iID, ENUMEXPTYPE enumType)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_EXPLOSIONS)
        return;

    g_lpExp[iIndex].enumType = enumType;
}

//////////////////////////////////////////////////////////////////////
// Function: fxExpSetVehicle
// Description:
//////////////////////////////////////////////////////////////////////
void fxExpSetVehicle(SINT32 iID, vehVehicle* lpVeh)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_EXPLOSIONS)
        return;

    g_lpExp[iIndex].lpVehAttach = lpVeh;
}

//////////////////////////////////////////////////////////////////////
// Function: fxExpSetRadius
// Description:
//////////////////////////////////////////////////////////////////////
void fxExpSetRadius(SINT32 iID, FLOAT32 fRadius)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_EXPLOSIONS)
        return;

    g_lpExp[iIndex].fRadius = fRadius;
}

//////////////////////////////////////////////////////////////////////
// Function: fxExpSetVelocity
// Description:
//////////////////////////////////////////////////////////////////////
void fxExpSetVelocity(SINT32 iID, FLOAT32 x, FLOAT32 y, FLOAT32 z)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_EXPLOSIONS)
        return;

    g_lpExp[iIndex].vecVel.x = x;
    g_lpExp[iIndex].vecVel.y = y;
    g_lpExp[iIndex].vecVel.z = z;
}

//////////////////////////////////////////////////////////////////////
// Function: fxExpPushFlags
// Description:
//////////////////////////////////////////////////////////////////////
void fxExpPushFlags(SINT32 iID, UINT32 uFlags)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_EXPLOSIONS)
        return;

    g_lpExp[iIndex].uFlags |= uFlags;
}

//////////////////////////////////////////////////////////////////////
// Function: fxExpPopFlags
// Description:
//////////////////////////////////////////////////////////////////////
void fxExpPopFlags(SINT32 iID, UINT32 uFlags)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_EXPLOSIONS)
        return;

    g_lpExp[iIndex].uFlags &= ~uFlags;
}

//////////////////////////////////////////////////////////////////////
// Function: fxExpSetPt1Time
// Description:
//////////////////////////////////////////////////////////////////////
void fxExpSetPt1Time(SINT32 iID, FLOAT32 fTime)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_EXPLOSIONS)
        return;

    g_lpExp[iIndex].expTime.p1 = fTime;
}

//////////////////////////////////////////////////////////////////////
// Function: fxExpSetPt2Time
// Description:
//////////////////////////////////////////////////////////////////////
void fxExpSetPt2Time(SINT32 iID, FLOAT32 fTime)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_EXPLOSIONS)
        return;

    g_lpExp[iIndex].expTime.p2 = fTime;
}

//////////////////////////////////////////////////////////////////////
// Function: fxExpSetPt3Time
// Description:
//////////////////////////////////////////////////////////////////////
void fxExpSetPt3Time(SINT32 iID, FLOAT32 fTime)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_EXPLOSIONS)
        return;

    g_lpExp[iIndex].expTime.p3 = fTime;
}

//////////////////////////////////////////////////////////////////////
// Function: fxExpSetPt4Time
// Description:
//////////////////////////////////////////////////////////////////////
void fxExpSetPt4Time(SINT32 iID, FLOAT32 fTime)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_EXPLOSIONS)
        return;

    g_lpExp[iIndex].expTime.p4 = fTime;
}

//////////////////////////////////////////////////////////////////////
// Function: fxExpSetAmmo
// Description:
//////////////////////////////////////////////////////////////////////
void fxExpSetAmmo(SINT32 id, const weapAmmo* ammo)
{
    g_lpExp[id-1].ammo = ammo;
}

//////////////////////////////////////////////////////////////////////
// Function: fxExpGetNewExplosion
// Description:
//////////////////////////////////////////////////////////////////////
SINT32 fxExpGetNewExplosion()
{
    FXEXPLOSION* lpExp = nullptr;

    if(!_GetExp(&lpExp))
        return -1;

    if(!_AddExp(lpExp))
        return -1;

    return lpExp->iIndex + 1;
}
