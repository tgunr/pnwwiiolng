#include "syscfg.h" // Path is project specific.
//////////////////////////////////////////////////////
// Module: vehfx.cpp
// Description: Handle vehicle effects. (exhaust..etc)
//////////////////////////////////////////////////////

#include "ez/EZManager.h"
#include "fx/fxBlt.h"
#include "fx/fxHit.h"
#include "fx/FXInterface.h"
#include "fx/fxPcl.h"
#include "fx/fxSnd.h"
#include "fx/fxUtl.h"
#include "fx/fxWeather.h"
#include "fx/TransformEffect.h"
#include "imaplib/imaptypes.h"
#include "include/vars.h"
#include "math/math3d.h"
#include "math/mathnum.h"
#include "math/mathvect.h"
#include "ob/obMgr.h"
#include "ob/obpool.h"
#include "ogrp/ogrptype.h"
#include "pers/persmain.h"
#include "spt/Random.h"
#include "trn/trnmain.h"
#include "trn/trnvars.h"
#include "trn/trnwtr.h"
#include "veh/vehanim.h"
#include "veh/vehfx.h"

#include <deque>

#define MAX_METAL_DEBRIS    32

static FXVEHCONTAINER* g_lpCon = nullptr;

static FXVEHCONTAINER g_conHead;
static FXVEHCONTAINER g_conTail;
static FXVEHCONTAINER g_conFreeHead;
static FXVEHCONTAINER g_conFreeTail;

static std::deque<OBJECT*> _freedebris;
static OBJECT* _debris[MAX_METAL_DEBRIS];

static std::set<SINT32> _updates;

static FLOAT32 g_fPiOver180 = 0.0f;

static LOGICAL _CreateDebris(struct FXVEHCONTAINER* lpCon, SINT32 i, FLOAT32 scale);
static LOGICAL _CreateExhaust(struct FXVEHCONTAINER* lpCon, SINT32 iEngine, SINT32 iSide);
static LOGICAL _CreateSmokeStack(struct FXVEHCONTAINER* lpCon, SINT32 iEngine, SINT32 iSide);
static LOGICAL _CreateSparks(struct FXVEHCONTAINER* lpCon, SINT32 iTank);
static LOGICAL _CreateTankFire(struct FXVEHCONTAINER* lpCon, SINT32 i);
static LOGICAL _CreateTreadDust(struct FXVEHCONTAINER* lpCon, SINT32 iSide);
static LOGICAL _CreateExplosion(struct FXVEHCONTAINER* lpCon, SINT32 i);
static LOGICAL _CreateVehicleFire(struct FXVEHCONTAINER* lpCon, SINT32 i);
#if(0)
static LOGICAL _CreateWingVortex(struct FXVEHCONTAINER* lpCon, SINT32 i);
#endif
static LOGICAL _UpdateSparks(struct FXVEHCONTAINER* lpCon);
static LOGICAL _UpdateDebris(struct FXVEHCONTAINER* lpCon);
static LOGICAL _UpdateExhaust(struct FXVEHCONTAINER* lpCon);
static LOGICAL _UpdateVehicleFire(struct FXVEHCONTAINER* lpCon);
static LOGICAL _UpdateTankFire(struct FXVEHCONTAINER* lpCon);
static LOGICAL _UpdateTreadDust(struct FXVEHCONTAINER* lpCon);
static LOGICAL _UpdateWingVortex(struct FXVEHCONTAINER* lpCon);

static void _BoatTrailCallback(struct FXPARTICLE* lpPcl);
static void _BoatWakeCallback(struct FXPARTICLE* lpPcl);
#if(0)
static void _CreateBoatWakesAndTrails(struct FXVEHCONTAINER* lpCon, SINT32 i);
static void _SmokeStackKill(void* pData);
#endif
static void _updateTurretSound(struct FXVEHCONTAINER* lpCon);
static void _updateTraverseSound(struct FXVEHCONTAINER* c, SINT32 i);
static void _updateElevateSound(struct FXVEHCONTAINER* c, SINT32 i);
static void _KillSoundCallback(void* data);

static OBJECT* _getFreeDebris();

static FLOAT32 _GetPitch(struct vehVehicle* lpVeh);

#define USE_FX2 0
#if USE_FX2
#include "fx2/effect.h"
#endif

///////////////////////////////////////////////////////
// Function: vehfxInit
// Description:
///////////////////////////////////////////////////////
LOGICAL vehfxInit()
{
    g_lpCon = (FXVEHCONTAINER*)memAlloc(sizeof(FXVEHCONTAINER) * MAX_POSSIBLE_VIS_VEHICLES);

    if(g_lpCon == nullptr)
        return FALSE;

    memset(&g_conFreeHead, 0, sizeof(FXVEHCONTAINER));
    memset(&g_conFreeTail, 0, sizeof(FXVEHCONTAINER));

    memset(&g_conHead, 0, sizeof(FXVEHCONTAINER));
    memset(&g_conTail, 0, sizeof(FXVEHCONTAINER));

    g_conFreeHead.next = &g_conFreeTail;
    g_conFreeTail.prev = &g_conFreeHead;

    g_conHead.next = &g_conTail;
    g_conTail.prev = &g_conHead;

    SINT32 i;

    for(i = 0; i < MAX_POSSIBLE_VIS_VEHICLES; i++)
    {
        g_lpCon[i].index = i;
        vehfxFreeVehicle(&g_lpCon[i]);
    }

    g_fPiOver180 = PI / 180.0f;

    if(!vehfxInitDebris())
    {
        return FALSE;
    }

    return TRUE;
}

//////////////////////////////////////////////////
// Function: vehfxInitDebris
// Description:
//////////////////////////////////////////////////
LOGICAL vehfxInitDebris()
{
    for(SINT32 i = 0; i < MAX_METAL_DEBRIS; i++)
    {
        if(!obmgrCreateModelInstance("debris2"))
        {
            return FALSE;
        }

        _debris[i] = obmainFindNamedObject("debris2");

        if(!_debris[i])
        {
            return FALSE;
        }

        // Draw double sided (if flat polygon)
        obmainSetFlagsInThisTree(_debris[i], OBJECT_NOBACKFACE);

        // Remove this object from the standard rendering system.
        obmainRemoveObjectTree(_debris[i]);

        _freedebris.push_back(_debris[i]);
    }

    return TRUE;
}

//////////////////////////////////////////////////
// Function: vehfxKill
// Description:
//////////////////////////////////////////////////
void vehfxKill()
{
    if(g_lpCon != nullptr)
    {
        memFree(g_lpCon);
        g_lpCon = nullptr;
    }

    SINT32 i;

    for(i = 0; i < MAX_METAL_DEBRIS; i++)
    {
        if(obmainValidateObject(_debris[i]))
        {
            ogrpFreeObjectTree(_debris[i]);
        }
    }

    _freedebris.clear();

    // Set to nullptr in case of further updates.
    memset(&g_conFreeHead, 0, sizeof(FXVEHCONTAINER));
    memset(&g_conFreeTail, 0, sizeof(FXVEHCONTAINER));
    memset(&g_conHead, 0, sizeof(FXVEHCONTAINER));
    memset(&g_conTail, 0, sizeof(FXVEHCONTAINER));
}

//////////////////////////////////////////////////
// Function: vehfxFreeVehicle
// Description: Add a fx vehicle to the free list.
//////////////////////////////////////////////////
LOGICAL vehfxFreeVehicle(FXVEHCONTAINER* lpCon)
{
    FXVEHCONTAINER* lpPrev = nullptr;
    SINT32 i;

    if(lpCon == nullptr)
        return FALSE;

    i = lpCon->index;

    memset(lpCon, 0, sizeof(FXVEHCONTAINER));

    lpCon->index = i;

    lpPrev = g_conFreeTail.prev;

    lpCon->prev = lpPrev;
    lpCon->next = &g_conFreeTail;

    lpPrev->next = lpCon;
    g_conFreeTail.prev = lpCon;

    return TRUE;
}

//////////////////////////////////////////////////
// Function: vehfxAttachVehicle
// Description:
//////////////////////////////////////////////////
LOGICAL vehfxAttachVehicle(vehVehicle* const lpVehicle, SINT32* pIndex)
{
    if(lpVehicle == nullptr) return FALSE;
    if(lpVehicle->VehicleType == nullptr) return FALSE;

    switch(lpVehicle->VehicleType->Type)
    {
    case VEH_TYPE_FLOATER:
        {
            break;
        }
    case VEH_TYPE_PERSON:
        {
            vehfxAttachTrooperVehicle(lpVehicle, pIndex);
            break;
        }
    case VEH_TYPE_AIR:
        {
            vehfxAttachAirVehicle(lpVehicle, pIndex);
            break;
        }
    case VEH_TYPE_GROUND:
        {
            vehfxAttachGroundVehicle(lpVehicle, pIndex);
            break;
        }
    case VEH_TYPE_SEA:
        {
            vehfxAttachSeaVehicle(lpVehicle, pIndex);
            break;
        }
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: vehfxAttachTrooperVehicle
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL vehfxAttachTrooperVehicle(vehVehicle* lpVehicle, SINT32* pIndex)
{
    FXVEHCONTAINER* lpCon = nullptr;

    if(!lpVehicle)
        return FALSE;

    if(!lpVehicle->VehicleType)
        return FALSE;

    if(!vehfxGrabAVehicle(&lpCon))
        return FALSE;

    if(!vehfxAddVehicle(lpCon))
        return FALSE;

    lpCon->lpVeh = lpVehicle;

    *pIndex = lpCon->index + 1;

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: vehfxAttachAirVehicle
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL vehfxAttachAirVehicle(vehVehicle* lpVehicle, SINT32* pIndex)
{
    FXVEHCONTAINER* lpCon = nullptr;

    if(!lpVehicle)
        return FALSE;

    if(!lpVehicle->VehicleType)
        return FALSE;

    if(!vehfxGrabAVehicle(&lpCon))
        return FALSE;

    if(!vehfxAddVehicle(lpCon))
        return FALSE;

    lpCon->lpVeh = lpVehicle;

    *pIndex = lpCon->index + 1;

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: vehfxAttachSeaVehicle
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL vehfxAttachSeaVehicle(vehVehicle* lpVehicle, SINT32* pIndex)
{
    FXVEHCONTAINER* lpCon = nullptr;

    if(!lpVehicle)
        return FALSE;

    if(!lpVehicle->VehicleType)
        return FALSE;

    if(!vehfxGrabAVehicle(&lpCon))
        return FALSE;

    if(!vehfxAddVehicle(lpCon))
        return FALSE;

    lpCon->lpVeh = lpVehicle;

    *pIndex = lpCon->index + 1;

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: vehfxAttachGroundVehicle
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL vehfxAttachGroundVehicle(vehVehicle* lpVehicle, SINT32* pIndex)
{
    FXVEHCONTAINER* lpCon = nullptr;

    if(!lpVehicle)
        return FALSE;

    if(!lpVehicle->VehicleType)
        return FALSE;

    if(!vehfxGrabAVehicle(&lpCon))
        return FALSE;

    if(!vehfxAddVehicle(lpCon))
        return FALSE;

    lpCon->lpVeh = lpVehicle;

    *pIndex = lpCon->index + 1;

    return TRUE;
}

//////////////////////////////////////////////////
// Function: vehfxDetachVehicle
// Description:
//////////////////////////////////////////////////
LOGICAL vehfxDetachVehicle(SINT32 iID)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= MAX_POSSIBLE_VIS_VEHICLES)
    {
        return FALSE;
    }

    if(g_lpCon == nullptr)
    {
        return FALSE;
    }

    logTimerBeginMacro();

    vehfxRemoveAllEffects(iID);

    fxPclDetachVehicle(g_lpCon[iIndex].lpVeh);

    vehfxReleaseVehicle(&g_lpCon[iIndex]);
    vehfxFreeVehicle(&g_lpCon[iIndex]);

    logTimerEndMacro();

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: vehfxReleaseVehicle
// Description:
//////////////////////////////////////////////////////////////////////
void vehfxReleaseVehicle(FXVEHCONTAINER* lpCon)
{
    FXVEHCONTAINER* lpNext = nullptr;
    FXVEHCONTAINER* lpPrev = nullptr;

    if(lpCon == nullptr) return;

    lpNext = lpCon->next;
    lpPrev = lpCon->prev;

    if(lpNext == nullptr || lpPrev == nullptr)
        return;

    lpPrev->next = lpNext;
    lpNext->prev = lpPrev;

    lpCon->prev = nullptr;
    lpCon->next = nullptr;
}

//////////////////////////////////////////////////////////////////////
// Function: vehfxGrabAVehicle
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL vehfxGrabAVehicle(FXVEHCONTAINER** lpCon)
{
    FXVEHCONTAINER* lpNode = nullptr;
    FXVEHCONTAINER* lpNext = nullptr;
    FXVEHCONTAINER* lpPrev = nullptr;

    if(lpCon == nullptr)
        return FALSE;

    if(!vehfxCheckForVehicles())
        return FALSE;

    lpNode = g_conFreeHead.next;

    lpPrev = lpNode->prev;
    lpNext = lpNode->next;

    lpPrev->next = lpNext;
    lpNext->prev = lpPrev;

    lpNode->next = nullptr;
    lpNode->prev = nullptr;

    *lpCon = lpNode;

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: vehfxCheckForVehicles
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL vehfxCheckForVehicles()
{
    // Are we out of free vehicles?
    if(g_conFreeHead.next == &g_conFreeTail)
    {
        FXVEHCONTAINER* lpCon = nullptr;

        if(g_conTail.prev == &g_conHead)
            return FALSE;

        // Pop oldest one
        lpCon = g_conTail.prev;

        vehfxReleaseVehicle(lpCon);
        vehfxFreeVehicle(lpCon);
    }

    // Double check.
    if(g_conFreeHead.next == &g_conFreeTail)
        return FALSE;

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: vehfxAddVehicle
// Description: Add to head of list.
//////////////////////////////////////////////////////////////////////
LOGICAL vehfxAddVehicle(FXVEHCONTAINER* lpCon)
{
    FXVEHCONTAINER* lpNext = nullptr;

    if(lpCon == nullptr)
        return FALSE;

    lpNext = g_conHead.next;

    g_conHead.next = lpCon;
    lpCon->prev = &g_conHead;

    lpCon->next = lpNext;
    lpNext->prev = lpCon;

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _UpdateTankFire
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL _UpdateTankFire(FXVEHCONTAINER* lpCon)
{
    if(lpCon->lpVeh == nullptr) return FALSE;

    SINT32 i;

    vehanimVehicleStateAnimations* p = &lpCon->lpVeh->StateAnimation;

    if(!p->TankOnFire || p->bVehicleDestroyed)
    {
        for(i = 0; i < VEH_MAX_FIREPOINTS; i++)
        {
            if(p->TankFireSmokeEmitter[i] > 0)
            {
                fxPclDetachByID(p->TankFireSmokeEmitter[i]);
                p->TankFireSmokeEmitter[i] = 0;
            }
        }

		if(lpCon->cookOffSoundID > 0)
		{
			fxSndStopSound(lpCon->cookOffSoundID);
		}

        return TRUE;
    }

    for(i = 0; i < VEH_MAX_FIREPOINTS; i++)
    {
        if(p->TankFireSmokeEmitter[i] == 0)
        {
            _CreateTankFire(lpCon, i);
        }
    }

    if(lpCon->cookOffSoundID == 0)
    {
        // Set emitter to first fire point. If this is 0, the fire has not been created.
        if(p->TankFireSmokeEmitter[0] == 0)
            return TRUE;

        // Create cookoff sound.
        SINT32 ID = fxSndGetNewSound();

        if(ID == -1)
            return FALSE;

        FLOAT64VECTOR pos;

        pos.x = p->TankFireEmitterPoint[0]->dof->dofmatrix1._41;
        pos.y = p->TankFireEmitterPoint[0]->dof->dofmatrix1._42;
        pos.z = p->TankFireEmitterPoint[0]->dof->dofmatrix1._43;

        fxSndSetSoundName(ID, "sounds/tank_burn.wav");
        fxSndSetSoundType(ID, TYPE_INFINITE_SOUND);
        fxSndSetPitch(ID, 1.0f);
        fxSndSetVolume(ID, 1.0f);
        fxSndSetCreateTime(ID, frametime);
        fxSndSetPlayType(ID, DEF_PLAYTYPE_OBJECT);
        fxSndSetEmitterObject(ID, p->TankFireEmitterPoint[0]);
        fxSndSetPriority(ID, IDE_FP_PRIORITY_LOW);
        fxSndSetPosition(ID, pos.x, pos.y, pos.z);
        fxSndSetMinDistance(ID, 10.0f);
        fxSndSetMaxDistance(ID, 100.0f);
        fxSndSetKillCallback(ID, _KillSoundCallback);
        fxSndSetKillData(ID, &lpCon->cookOffSoundID);

        if(!fxSndInitSound(ID))
        {
            fxSndStopSound(ID);
        }
        else
        {
            lpCon->cookOffSoundID = ID;
        }
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _UpdateTreadDust
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL _UpdateTreadDust(FXVEHCONTAINER* lpCon)
{
    if(lpCon->lpVeh == nullptr) return FALSE;

    for(SINT32 i = 0; i < VEH_MAX_TREAD_DUST; i++)
    {
        if(!lpCon->lpVeh->StateAnimation.TreadDustOn[i])
        {
            if(lpCon->lpVeh->StateAnimation.TreadDustHandle[i] > 0)
            {
#if USE_FX2
                fxremove(lpCon->lpVeh->StateAnimation.TreadDustHandle[i]);
#else
                fxPclDetachByID(lpCon->lpVeh->StateAnimation.TreadDustHandle[i]);
#endif
                lpCon->lpVeh->StateAnimation.TreadDustHandle[i] = 0;

                // HACK until more state bits allowed in the update system
                if(lpCon->lpVeh->VehicleType->Type == VEH_TYPE_SEA)
                {
                    if(lpCon->iShipScrewsID[0] > 0)
                    {
#if USE_FX2
                        fxremove(lpCon->iShipScrewsID[0]);
#else
                        fxPclDetachByID(lpCon->iShipScrewsID[0]);
#endif
                        lpCon->iShipScrewsID[0] = -1;
                    }

                    if(lpCon->iShipScrewsID[1] > 0)
                    {
#if USE_FX2
                        fxremove(lpCon->iShipScrewsID[1]);
#else
                        fxPclDetachByID(lpCon->iShipScrewsID[1]);
#endif
                        lpCon->iShipScrewsID[1] = -1;
                    }
                }
            }

            continue;
        }

        _CreateTreadDust(lpCon, i);
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _UpdateWingVortex
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL _UpdateWingVortex(FXVEHCONTAINER* lpCon)
{
#if(0)
    if(lpCon->lpVeh == nullptr) return FALSE;

    for(SINT32 i = 0; i < 2; i++)
    {
        if(!lpCon->lpVeh->StateAnimation.WingtipVortexSmokeOn[i])
        {
            if(lpCon->lpVeh->StateAnimation.WingtipVortexSmokeHandle[i] > 0)
            {
#if USE_FX2
                fxremove(lpCon->lpVeh->StateAnimation.WingtipVortexSmokeHandle[i]);
#else
                //FXStopEffect(lpCon->lpVeh->StateAnimation.WingtipVortexSmokeHandle[i]);
#endif
                lpCon->lpVeh->StateAnimation.WingtipVortexSmokeHandle[i] = 0;
            }

            continue;
        }

        _CreateWingVortex(lpCon, i);
    }
#endif

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _UpdateExhaust
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL _UpdateExhaust(FXVEHCONTAINER* lpCon)
{
    if(lpCon->lpVeh == nullptr) return FALSE;

    for(SINT32 i = 0; i < ML_MAX_ENGINES; i++)
    {
        if(!lpCon->lpVeh->StateAnimation.ExhaustSmokeOn[i])
        {
            if(lpCon->lpVeh->StateAnimation.ExhaustSmokeHandle[i][0] > 0)
            {
#if USE_FX2
                fxremove(lpCon->lpVeh->StateAnimation.ExhaustSmokeHandle[i][0]);
#else
                fxPclDetachByID(lpCon->lpVeh->StateAnimation.ExhaustSmokeHandle[i][0]);
#endif
                lpCon->lpVeh->StateAnimation.ExhaustSmokeHandle[i][0] = 0;

                if(lpCon->lpVeh->VehicleType->Type == VEH_TYPE_SEA)
                    _CreateSmokeStack(lpCon, i, 0);
            }

            if(lpCon->lpVeh->StateAnimation.ExhaustSmokeHandle[i][1] > 0)
            {
#if USE_FX2
                fxremove(lpCon->lpVeh->StateAnimation.ExhaustSmokeHandle[i][1]);
#else
                fxPclDetachByID(lpCon->lpVeh->StateAnimation.ExhaustSmokeHandle[i][1]);
#endif
                lpCon->lpVeh->StateAnimation.ExhaustSmokeHandle[i][1] = 0;

                if(lpCon->lpVeh->VehicleType->Type == VEH_TYPE_SEA)
                    _CreateSmokeStack(lpCon, i, 1);
            }

            continue;
        }

        _CreateExhaust(lpCon, i, 0);
        _CreateExhaust(lpCon, i, 1);
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _UpdateExplosion
// Description: Currently set off with debris state. We will probably need to
// add another state (explosion state) to the vehicle structure.
//////////////////////////////////////////////////////////////////////
LOGICAL _UpdateExplosion(FXVEHCONTAINER* lpCon)
{
    if(lpCon->lpVeh == nullptr) return FALSE;

    SINT32 i;

    for(i = 0; i < VEH_MAX_DEBRIS_POINTS; i++)
    {
        if(lpCon->lpVeh->StateAnimation.ExplosionOn[i])
        {
            _CreateExplosion(lpCon, i);
        }
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _UpdateDebris
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL _UpdateDebris(FXVEHCONTAINER* lpCon)
{
    if(lpCon->lpVeh == nullptr) return FALSE;

    SINT32 i;

    for(i = 0; i < VEH_MAX_DEBRIS_POINTS; i++)
    {
        if(lpCon->lpVeh->StateAnimation.DebrisOn[i])
        {
            _CreateDebris(lpCon, i, 0.75f);
        }
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _UpdateVehicleFire
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL _UpdateVehicleFire(FXVEHCONTAINER* lpCon)
{
    if(lpCon->lpVeh == nullptr) return FALSE;

    _CreateVehicleFire(lpCon, 0);

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _UpdateSparks
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL _UpdateSparks(FXVEHCONTAINER* lpCon)
{
    if(lpCon->lpVeh == nullptr) return FALSE;

    if(lpCon->lpVeh->VehicleType->Type != VEH_TYPE_AIR)
        return TRUE;

    for(SINT32 i = 0; i < ML_MAX_FUELTANKS; i++)
    {
        if(lpCon->lpVeh->StateAnimation.FuelFireEmitter[i] == nullptr)
            continue;

        if(lpCon->lpVeh->StateAnimation.FuelFireOn[i])
        {
            if(lpCon->iSparkID[i] <= 0)
            {
                _CreateSparks(lpCon, i);
            }
        }
        else if(lpCon->iSparkID[i] > 0)
        {
            if(lpCon->lpVeh->StateAnimation.FuelFireOn[i] == 0)
            {
#if USE_FX2
                fxremove(lpCon->iSparkID[i]);
#else
                fxPclDetachByID(lpCon->iSparkID[i]);
#endif
                lpCon->iSparkID[i] = 0;
            }
        }
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _updateTurretSound
// Description:
//////////////////////////////////////////////////////////////////////
void _updateTurretSound(FXVEHCONTAINER* c)
{
    vehVehicle* v = c->lpVeh;

    if(v && v->VehicleType)
    {
        if(v->VehicleType->Type == VEH_TYPE_GROUND || v->VehicleType->Type == VEH_TYPE_SEA)
        {
            for(SINT32 i = 0; i < ML_MAX_CREW_POSITIONS; i++)
            {
                if(!v->VehicleType->turretSound[i].empty())
                {
                    _updateTraverseSound(c, i);
                    _updateElevateSound(c, i);
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////
// Function: _updateTraverseSound
// Description:
//////////////////////////////////////////////////////////////////////
void _updateTraverseSound(FXVEHCONTAINER* c, SINT32 i)
{
    vehVehicle* v = c->lpVeh;

    if(!v)
        return;

    if(!v->VehicleType)
        return;

    FLOAT64 rate = 0.0f;
    FLOAT64 move = 0.0f;
    FLOAT64 lastRate = 0.0f;

    SINT32 playType = 0;

    // #$@#$! damn turret crap
    FLOAT64 almostzero = 1e-4;

    // We have too many variables for turrets!
    if(v->ModelData)
    {
        // HACK! for mouse input $#@&*#
        if(v->ModelData->axisType == IMAP_DELTA_AXIS)
        {
            almostzero = 1e-10;
        }

        // If player position does not equal i, continue..stop the sound.
        // Don't want annoying sound playing when player changes positions
        UINT32 position = persGetPersonaPosition();

        if(position != (UINT32)i)
        {
            if(c->turretTraverse[i][0] > 0)
            {
                fxSndPushFlags(c->turretTraverse[i][0], SOUND_PLAY_NO_TRAIL);
                fxSndStopSound(c->turretTraverse[i][0]);
                c->turretTraverse[i][0] = 0;
            }

            if(c->turretTraverse[i][1] > 0)
            {
                fxSndPushFlags(c->turretTraverse[i][0], SOUND_PLAY_NO_TRAIL);
                fxSndStopSound(c->turretTraverse[i][1]);
                c->turretTraverse[i][1] = 0;
            }

            return;
        }

        rate = v->ModelData->TurretTraverse[i];
        FLOAT64 lastrate = v->ModelData->LastTurretTraverse[i];

        rate -= lastrate;

        rate = fabs(rate);
        rate = rate < almostzero ? 0.0f : rate;

//		static FLOAT64 oldrate = 0.0f;

        move = rate;

        if(move > almostzero)
        {
            playType = DEF_PLAYTYPE_HEAD;
        }
    }
    else
    {
        rate = fabs(v->StateAnimation.TurretTraverse[i]);
        lastRate = fabs(v->StateAnimation.LastTurretTraverse[i]);

        rate -= lastRate;

        rate = fabs(rate);
        rate = rate < almostzero ? 0.0f : rate;

        move = rate;

        if(move > almostzero)
        {
            playType = DEF_PLAYTYPE_OBJECT;
        }
    }

    // Try to avoid the rapid on/off sound
    if(move > almostzero)
    {
        // Load if no sound playing
        if(c->turretTraverse[i][0] == 0)
        {
            if(!v->VehicleType->turretSound[i]["turrettraverse"].empty())
            {
                OBJECT* o = v->VehicleObjectData->PlaneObject;

                if(!o) return;

                FLOAT64 x = o->pos.x;
                FLOAT64 y = o->pos.y;
                FLOAT64 z = o->pos.z;

                std::string str = "sounds\\";
                str += v->VehicleType->turretSound[i]["turrettraverse"];

                SCHAR* handsound = nullptr;

                // Don't play hand move sounds if 3rd person
                if(playType == DEF_PLAYTYPE_OBJECT)
                {
                    handsound = (char*)strstr(str.c_str(), "hand_turret_move");
                }

                if(handsound == nullptr)
                {
                    SINT32 ID = fxSndGetNewSound();

                    if(ID == -1) return;

                    fxSndSetSoundName(ID, (char*)str.c_str());
                    fxSndSetSoundType(ID, TYPE_INFINITE_SOUND);
                    fxSndSetPitch(ID, 0.9f);
                    fxSndSetVolume(ID, 1.0f);
                    fxSndSetCreateTime(ID, frametime);
                    fxSndSetPlayType(ID, playType);
                    fxSndSetEmitterObject(ID, o);
                    fxSndSetPriority(ID, IDE_FP_PRIORITY_MEDIUM);
                    fxSndSetPosition(ID, x, y, z);
                    fxSndSetMinDistance(ID, 10.0f);
                    fxSndSetMaxDistance(ID, 100.0f);
                    fxSndSetKillCallback(ID, _KillSoundCallback);
                    fxSndSetKillData(ID, &c->turretTraverse[i][0]);

                    if(!fxSndInitSound(ID))
                        return;

                    c->turretTraverse[i][0] = ID;
                }
            }
        }

        // Load if no sound playing
        if(c->turretTraverse[i][1] == 0)
        {
            if(!v->VehicleType->turretSound[i]["turretgeneral"].empty())
            {
                OBJECT* o = v->VehicleObjectData->PlaneObject;

                if(!o) return;

                SINT32 ID = fxSndGetNewSound();

                if(ID == -1) return;

                FLOAT64 x = o->pos.x;
                FLOAT64 y = o->pos.y;
                FLOAT64 z = o->pos.z;

                std::string str = "sounds\\";
                str += v->VehicleType->turretSound[i]["turretgeneral"];

                fxSndSetSoundName(ID, (char*)str.c_str());
                fxSndSetSoundType(ID, TYPE_INFINITE_SOUND);
                fxSndSetPitch(ID, 1.0f);
                fxSndSetVolume(ID, 1.0f);
                fxSndSetCreateTime(ID, frametime);
                fxSndSetPlayType(ID, playType);
                fxSndSetEmitterObject(ID, o);
                fxSndSetPriority(ID, IDE_FP_PRIORITY_MEDIUM);
                fxSndSetPosition(ID, x, y, z);
                fxSndSetMinDistance(ID, 10.0f);
                fxSndSetMaxDistance(ID, 100.0f);
                fxSndSetKillCallback(ID, _KillSoundCallback);
                fxSndSetKillData(ID, &c->turretTraverse[i][1]);

                if(!fxSndInitSound(ID))
                    return;

                c->turretTraverse[i][1] = ID;
            }
        }
    }
    else
    {
        if(c->turretTraverse[i][0] > 0)
        {
            fxSndStopSound(c->turretTraverse[i][0]);
            c->turretTraverse[i][0] = 0;
        }

        if(c->turretTraverse[i][1] > 0)
        {
            fxSndStopSound(c->turretTraverse[i][1]);
            c->turretTraverse[i][1] = 0;
        }
    }
}

//////////////////////////////////////////////////////////////////////
// Function: _updateElevateSound
// Description:
//////////////////////////////////////////////////////////////////////
void _updateElevateSound(FXVEHCONTAINER* c, SINT32 i)
{
    vehVehicle* v = c->lpVeh;

    if(!v)
        return;

    if(!v->VehicleType)
        return;

    FLOAT64 rate = 0.0f;
    FLOAT64 move = 0.0f;
    FLOAT64 lastRate = 0.0f;

    SINT32 playType = 0;

    // #$@#$! damn turret crap
    FLOAT64 almostzero = 1e-3;

    // We have too many variables for turrets!
    if(v->ModelData)
    {
        // HACK! for mouse input $#@&*#
        if(v->ModelData->axisType == IMAP_DELTA_AXIS)
        {
            almostzero = 1e-10;
        }

        // If player position does not equal i, continue..stop the sound.
        // Don't want annoying sound playing when player changes positions
        UINT32 position = persGetPersonaPosition();

        if(position != (UINT32)i)
        {
            if(c->turretElevate[i] > 0)
            {
                fxSndPushFlags(c->turretTraverse[i][0], SOUND_PLAY_NO_TRAIL);
                fxSndStopSound(c->turretElevate[i]);
                c->turretElevate[i] = 0;
            }

            return;
        }

        rate = v->ModelData->TurretElevate[i];
        FLOAT64 lastrate = v->ModelData->LastTurretElevate[i];

        rate -= lastrate;

        rate = fabs(rate);
        rate = rate < almostzero ? 0.0f : rate;

//		static FLOAT64 oldrate = 0.0f;

        move = rate;

        if(move >= almostzero)
        {
            playType = DEF_PLAYTYPE_HEAD;
        }
    }
    else
    {
        rate = fabs(v->StateAnimation.TurretElev[i]);
        lastRate = fabs(v->StateAnimation.LastTurretElev[i]);

        rate -= lastRate;

        rate = fabs(rate);
        rate = rate < almostzero ? 0.0f : rate;

        move = rate;

        if(move > almostzero)
        {
            playType = DEF_PLAYTYPE_OBJECT;
        }
    }

    if(move > almostzero)
    {
        if(c->turretElevate[i] == 0)
        {
            if(!v->VehicleType->turretSound[i]["turretelevate"].empty())
            {
                OBJECT* o = v->VehicleObjectData->PlaneObject;

                if(!o) return;

                FLOAT64 x = o->pos.x;
                FLOAT64 y = o->pos.y;
                FLOAT64 z = o->pos.z;

                std::string str = "sounds\\";
                str += v->VehicleType->turretSound[i]["turretelevate"];

                SCHAR* handsound = nullptr;

                // Don't play hand move sounds if 3rd person
                if(playType == DEF_PLAYTYPE_OBJECT)
                {
                    handsound = (char*)strstr(str.c_str(), "hand_turret_move");
                }

                if(handsound == nullptr)
                {
                    SINT32 ID = fxSndGetNewSound();

                    if(ID == -1) return;

                    fxSndSetSoundName(ID, (char*)str.c_str());
                    fxSndSetSoundType(ID, TYPE_INFINITE_SOUND);
                    fxSndSetPitch(ID, 1.0f);
                    fxSndSetVolume(ID, 1.0f);
                    fxSndSetCreateTime(ID, frametime);
                    fxSndSetPlayType(ID, playType);
                    fxSndSetEmitterObject(ID, o);
                    fxSndSetPriority(ID, IDE_FP_PRIORITY_MEDIUM);
                    fxSndSetPosition(ID, x, y, z);
                    fxSndSetMinDistance(ID, 10.0f);
                    fxSndSetMaxDistance(ID, 100.0f);
                    fxSndSetKillCallback(ID, _KillSoundCallback);
                    fxSndSetKillData(ID, &c->turretElevate[i]);

                    if(!fxSndInitSound(ID))
                        return;

                    c->turretElevate[i] = ID;
                }
            }
        }
    }
    else
    {
        if(c->turretElevate[i] > 0)
        {
            fxSndStopSound(c->turretElevate[i]);
            c->turretElevate[i] = 0;
        }
    }
}

//////////////////////////////////////////////////////////////////////
// Function: _CreateSparks
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL _CreateSparks(FXVEHCONTAINER* lpCon, SINT32 iTank)
{
#if(0)
    if(lpCon->lpVeh == nullptr) return FALSE;

    OBJECT* lpObj = lpCon->lpVeh->StateAnimation.FuelFireEmitter[iTank];

    if(lpObj == nullptr) return FALSE;
    if(lpObj->dof == nullptr) return FALSE;

#if USE_FX2
    piVECTOR n = {0, 0, 1};
    int id = fxaddeffectdof("fuelsparks", lpObj, n, 0);
    lpCon->iSparkID[iTank] = id;
#else
    SINT32 iID = fxPclGetNewParticle();

    if(iID == -1) return FALSE;

    fxPclSetType(iID, IDE_PARTICLE_CALLBACK);
    fxPclPushFlags(iID, DEF_FXFLAG_INFINITE);
    fxPclSetPt1Time(iID, (FLOAT32)frametime);
    fxPclSetUserUpdate(iID, vehfxFuelEmitterSparkCallback);
    fxPclSetEmitterObject(iID, lpObj);
    fxPclSetDOFObject(iID, lpObj->dof);

    // Required after settings.
    if(!fxPclInitParticle(iID))
    {
        fxPclDetachByID(iID);
        iID = -1;
    }

    lpCon->iSparkID[iTank] = iID;
#endif
#endif

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _BoatTrailCallback
// Description:
//////////////////////////////////////////////////////////////////////
void _BoatTrailCallback(FXPARTICLE* p)
{
#ifndef USE_FX2
    if(p->lpVehicle == nullptr)
        return;

    if(p->lpEmitterObj == nullptr)
        return;

    if(p->lpDOF == nullptr)
        return;

    SINT32 sprites[3] = {21, 44, 20};
    SINT32 sprite = abs(mathGetChaoticNumber() % 3);

    FLOAT32 starttime = (FLOAT32)frametime;

    FLOAT64VECTOR vecVel;
    fxUtlGetVehicleVel(p->lpVehicle, &vecVel);

    FLOAT32 speed = (FLOAT32)mathExactVectorLength(&vecVel);

    if(FloatToInt(speed) == 0)
    {
        return;
    }

    fxUtlGetDOFPos(p->lpDOF, &p->lpEmitterObj->mat, &p->vecPos);

    FLOAT64VECTOR diff;
    mathSubVectors(&diff, &p->vecPos, &p->vecLastPos);

    FLOAT64 dist2 = mathVectDotProduct(&diff, &diff);

    if(math3dFastSqrt((FLOAT32)dist2) < p->fSpacing)
    {
        return;
    }

    p->vecLastPos = p->vecPos;

    SINT32 iID = fxQuadGetNewParticle();

    // Add 0.01 to help reduce z-fighting
    fxQuadSetPosition(iID, p->vecPos.x, p->vecPos.y, p->vecPos.z + 0.01f);

    fxQuadPushFlags(iID, DEF_FXFLAG_INTRADIUS);
    fxQuadSet4PtScale(iID, p->pclScale.p1, p->pclScale.p2, p->pclScale.p3, p->pclScale.p4);

    fxQuadPushFlags(iID, DEF_FXFLAG_INTALPHA);

    // Scale based on speed...10 is arbitrary.
    FLOAT32 s = speed / 10.0f;

    s = s > 1.0f ? 1.0f : s;

    fxQuadSet4PtAlpha(iID,
                      p->pclColor.p1.a * s,
                      p->pclColor.p2.a * s,
                      p->pclColor.p3.a * s,
                      p->pclColor.p4.a * s);

    if(sprites[sprite] != 44)
    {
        fxQuadPushFlags(iID, DEF_FXFLAG_FLAT);

        fxQuadSetPt1Time(iID, starttime);
        starttime += 500;
        fxQuadSetPt2Time(iID, starttime);
        starttime += 4000;
        fxQuadSetPt3Time(iID, starttime);
        starttime += 10000;
        fxQuadSetPt4Time(iID, starttime);

        fxQuadSetRadius(iID, speed < 10.0f ? 10.0f : speed);
    }
    else
    {
        fxQuadSetPt1Time(iID, starttime);
        starttime += 200;
        fxQuadSetPt2Time(iID, starttime);
        starttime += 200;
        fxQuadSetPt3Time(iID, starttime);
        starttime += 200;
        fxQuadSetPt4Time(iID, starttime);

        fxQuadSetRadius(iID, speed);
    }

    fxQuadSetSpriteNumber(iID, sprites[sprite]);

    fxQuadSetScale(iID, 1.0f);

    fxQuadPushFlags(iID, DEF_FXFLAG_MYCOLOR);
    fxQuadSetPt1Color(iID, p->pclColor.p1.r, p->pclColor.p1.g, p->pclColor.p1.b);

    fxQuadInitParticle(iID);
#endif
}

//////////////////////////////////////////////////////////////////////
// Function: _BoatWakeCallback
// Description:
//////////////////////////////////////////////////////////////////////
void _BoatWakeCallback(FXPARTICLE* p)
{
#if(0)
    if(p->lpVehicle == nullptr)
        return;

    if(p->lpEmitterObj == nullptr)
        return;

    if(p->lpDOF == nullptr)
        return;

    FLOAT32 starttime = (FLOAT32)frametime;

    FLOAT64VECTOR vecVel;
    fxUtlGetVehicleVel(p->lpVehicle, &vecVel);

    FLOAT32 speed = (FLOAT32)mathExactVectorLength(&vecVel);

    if(speed <= 1.0f)
        return;

    fxUtlGetDOFPos(p->lpDOF, &p->lpEmitterObj->mat, &p->vecPos);

    FLOAT64VECTOR diff;
    mathSubVectors(&diff, &p->vecPos, &p->vecLastPos);

    FLOAT64 dist2 = mathVectDotProduct(&diff, &diff);

    if(math3dFastSqrt((FLOAT32)dist2) < p->fSpacing)
    {
        return;
    }

    p->vecLastPos = p->vecPos;

    piVECTOR vecDir;

    fxUtlGetDOFDirection(p->lpDOF, &p->lpEmitterObj->mat, &vecDir);

    speed *= 2.0f;

    vecDir.x *= speed;
    vecDir.y *= speed;
    vecDir.z *= speed;

    speed *= 0.5f;

    SINT32 iID = fxQuadGetNewParticle();

    fxQuadSetVelocity(iID, vecDir.x, vecDir.y, vecDir.z);
    fxQuadSetPosition(iID, p->vecPos.x, p->vecPos.y, p->vecPos.z);

    fxQuadPushFlags(iID, DEF_FXFLAG_INTRADIUS);
    fxQuadSet4PtScale(iID, 0.1f, 0.5f, 0.8f, 1.0f);

    fxQuadPushFlags(iID, DEF_FXFLAG_INTALPHA);
    fxQuadSet4PtAlpha(iID, 1.0f, 0.4f, 0.2f, 0.0f);

    fxQuadPushFlags(iID, DEF_FXFLAG_ROTATE);

    fxQuadPushFlags(iID, DEF_FXFLAG_USEDRAG);
    fxQuadSetDragCoeff(iID, 4.0f);

    SINT32 gravity = abs(mathGetChaoticNumber() % 3);

    if(gravity)
        fxQuadPushFlags(iID, DEF_FXFLAG_USEGRAVITY);

    fxQuadSetSpriteNumber(iID, 21);

    fxQuadPushFlags(iID, DEF_FXFLAG_MYCOLOR);
    fxQuadSetPt1Color(iID, p->pclColor.p1.r - 0.1f, p->pclColor.p1.g - 0.1f, p->pclColor.p1.b - 0.1f);

    fxQuadSetRadius(iID, speed * 0.5f);
    fxQuadSetScale(iID, 1.0f);
    fxQuadSetPt1Color(iID, 1.0f, 1.0f, 1.0f);
    fxQuadSetPt1Time(iID, starttime);
    fxQuadSetPt2Time(iID, starttime + 100);
    fxQuadSetPt3Time(iID, starttime + 500);
    fxQuadSetPt4Time(iID, starttime + 2000);

    fxQuadInitParticle(iID);

    iID = fxQuadGetNewParticle();

    fxQuadPushFlags(iID, DEF_FXFLAG_INTRADIUS);
    fxQuadPushFlags(iID, DEF_FXFLAG_INTALPHA);

    fxQuadPushFlags(iID, DEF_FXFLAG_FLAT);

    fxQuadSetPosition(iID, p->vecPos.x, p->vecPos.y, p->vecPos.z + 0.1f);
    fxQuadSetVelocity(iID, vecDir.x, vecDir.y, vecDir.z);

    // Arbitrary
    fxQuadSetRadius(iID, speed < 10.0f ? 10.0f : speed);

    FLOAT32 s = speed / 8.0f;

    s = s > 1.0f ? 1.0f : s;

    fxQuadSet4PtAlpha(iID, 0.3f * s, 0.3f * s, 0.1f * s, 0.0f);
    fxQuadSet4PtScale(iID, 0.3f, 0.4f, 0.7f, 1.5f);

    fxQuadPushFlags(iID, DEF_FXFLAG_MYCOLOR);
    fxQuadSetPt1Color(iID, p->pclColor.p1.r, p->pclColor.p1.g, p->pclColor.p1.b);

    fxQuadSetPt1Time(iID, starttime);
    starttime += 500;
    fxQuadSetPt2Time(iID, starttime);
    starttime += 800;
    fxQuadSetPt3Time(iID, starttime);
    starttime += 3000;
    fxQuadSetPt4Time(iID, starttime);

    fxQuadSetPt1Color(iID, 1.0f, 1.0f, 1.0f);

    fxQuadPushFlags(iID, DEF_FXFLAG_USEDRAG);
    fxQuadSetDragCoeff(iID, 4.0f);

    fxQuadSetSpriteNumber(iID, 21);

    fxQuadSetScale(iID, 1.0f);

    fxQuadInitParticle(iID);
#endif
}

//////////////////////////////////////////////////////////////////////
// Function: vehfxFuelEmitterSparkCallback
// Description:
//////////////////////////////////////////////////////////////////////
void vehfxFuelEmitterSparkCallback(FXPARTICLE* lpPcl)
{
    OBJECT* lpEmitObj = lpPcl->lpEmitterObj;

    if(lpEmitObj == nullptr) return;
    if(lpPcl->lpDOF == nullptr) return;

    // Random occurance
    if(abs(mathGetChaoticNumber() & 15) != 2)	// fast modulo 16
        return;

    piVECTOR vecVel = {2.0f, 2.0f, 2.0f};
    FLOAT64VECTOR vecPos;

    fxUtlGetDOFPos(lpPcl->lpDOF, &lpEmitObj->mat, &vecPos);

    piVECTOR vecVelAdd;

    vecVelAdd.x = 10.0f * (mathGetFloat32ChaoticNumber() * 2.0f - 1.0f);
    vecVelAdd.y = 10.0f * (mathGetFloat32ChaoticNumber() * 2.0f - 1.0f);
    vecVelAdd.z = 20.0f * mathGetFloat32ChaoticNumber();

    vecVel.x += vecVelAdd.x;
    vecVel.y += vecVelAdd.y;
    vecVel.z += vecVelAdd.z;

    FLOAT32 fRadius = 0.25f * mathGetFloat32ChaoticNumber();

    UINT32 uFlags = DEF_FXFLAG_USEGRAVITY | DEF_FXFLAG_USEDRAG;

    fxPclCreateSpark(&vecPos, &vecVel, fRadius, 4000.0f, uFlags, FALSE, TRUE, 1.0f);
}

//////////////////////////////////////////////////////////////////////
// Function: vehfxSparkNoDragCallback
// Description:
//////////////////////////////////////////////////////////////////////
void vehfxSparkNoDragCallback(FXPARTICLE* lpPcl)
{
    OBJECT* lpEmitObj = lpPcl->lpEmitterObj;

    if(lpEmitObj == nullptr) return;
    if(lpPcl->lpDOF == nullptr) return;

    // Random occurance
    if(abs(mathGetChaoticNumber() % 17) != 2)
        return;

    SINT32 iID = fxPclGetNewParticle();

    if(iID == -1) return;

    piVECTOR vecVel = {2.0f, 2.0f, 2.0f};
    FLOAT64VECTOR vecPos;

    fxUtlGetDOFPos(lpPcl->lpDOF, &lpEmitObj->mat, &vecPos);

    piVECTOR vecVelAdd;

    vecVelAdd.x = 10.0f * (mathGetFloat32ChaoticNumber() * 2.0f - 1.0f);
    vecVelAdd.y = 10.0f * (mathGetFloat32ChaoticNumber() * 2.0f - 1.0f);
    vecVelAdd.z = 20.0f * mathGetFloat32ChaoticNumber();

    vecVel.x += vecVelAdd.x;
    vecVel.y += vecVelAdd.y;
    vecVel.z += vecVelAdd.z;

    FLOAT32 fRadius = 0.5f * mathGetFloat32ChaoticNumber();

    fxPclCreateSpark(&vecPos, &vecVel, fRadius, 4000.0f, DEF_FXFLAG_USEGRAVITY, FALSE, TRUE, 0.0f);
}

//////////////////////////////////////////////////////////////////////
// Function: vehfxStopSmokeStack
// Description:
//////////////////////////////////////////////////////////////////////
void vehfxStopSmokeStack(SINT32 iID)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= MAX_POSSIBLE_VIS_VEHICLES)
        return;

    if(g_lpCon[iIndex].lpVeh->VehicleType->Type != VEH_TYPE_SEA)
        return;

    for(SINT32 i = 0; i < ML_MAX_ENGINES; i++)
    {
        if(g_lpCon[iIndex].iShipStackSmoke[i][0] > 0)
        {
#if USE_FX2
            fxremove(g_lpCon[iIndex].iShipStackSmoke[i][0]);
            g_lpCon[iIndex].iShipStackSmoke[i][0] = 0;
#else
            fxPclDetachByID(g_lpCon[iIndex].iShipStackSmoke[i][0]);
#endif
        }

        if(g_lpCon[iIndex].iShipStackSmoke[i][1] > 0)
        {
#if USE_FX2
            fxremove(g_lpCon[iIndex].iShipStackSmoke[i][1]);
            g_lpCon[iIndex].iShipStackSmoke[i][1] = 0;
#else
            fxPclDetachByID(g_lpCon[iIndex].iShipStackSmoke[i][1]);
#endif
        }
    }
}

//////////////////////////////////////////////////////////////////////
// Function: _CreateTankFire
// Description: Create a callback to produce fire.
//////////////////////////////////////////////////////////////////////
LOGICAL _CreateTankFire(FXVEHCONTAINER* lpCon, SINT32 i)
{
#if(0)
    if(lpCon->lpVeh == nullptr)
        return FALSE;

    if(lpCon->lpVeh->StateAnimation.TankFireSmokeEmitter[i] > 0)
        return FALSE;

    OBJECT* lpObj = lpCon->lpVeh->StateAnimation.TankFireEmitterPoint[i];

    if(lpObj == nullptr) return FALSE;
    if(lpObj->dof == nullptr) return FALSE;

#ifdef USE_FX2
    piVECTOR n = {0, 0, -1};
    int id = fxaddeffectdof("tankfire", lpObj, n, 0);
    lpCon->lpVeh->StateAnimation.TankFireSmokeEmitter[i] = id;
#else
    SINT32 iID = fxPclGetNewParticle();

    if(iID == -1) return FALSE;

    FLOAT32 z = mathGetFloat32ChaoticNumber() * g_fPiOver180;

    piVECTOR vecVel;
    FLOAT64VECTOR vecPos;

    fxUtlGetDOFDirection(lpObj->dof, &lpObj->mat, &vecVel);
    fxUtlGetDOFPos(lpObj->dof, &lpObj->mat, &vecPos);

    fxPclSetType(iID, IDE_PARTICLE_FIRE_FOUNTAIN);

    fxPclPushFlags(iID, DEF_FXFLAG_INFINITE);
    fxPclPushFlags(iID, DEF_FXFLAG_INTALPHA);
    fxPclPushFlags(iID, DEF_FXFLAG_INTCOLOR);
    fxPclPushFlags(iID, DEF_FXFLAG_INTRADIUS);
    fxPclPushFlags(iID, DEF_FXFLAG_ROTATE);
    fxPclPushFlags(iID, DEF_FXFLAG_DOF);
    fxPclPushFlags(iID, DEF_FXFLAG_RISE);
    fxPclPushFlags(iID, DEF_FXFLAG_USEDRAG);

    fxPclSetDragCoeff(iID, 3.0f);

    fxPclPushFlags(iID, DEF_FXFLAG_VARYVELOCITY);
    fxPclSetVelocityVariance(iID, 0.4f);

    fxPclSetParticleVel(iID, 3.0f, 3.0f, -3.0f);
    fxPclSetPt1Time(iID, (FLOAT32)frametime);
    fxPclSetPt4Time(iID, (FLOAT32)frametime + 500);
    fxPclSetRotate(iID, 0.0f, 0.0f, z);
    fxPclSetEmitterObject(iID, lpObj);
    fxPclSetDOFObject(iID, lpObj->dof);
    fxPclSet4PtScale(iID, 1.0f, 1.0f, 1.0f, 1.0f);
    fxPclSet4PtAlpha(iID, 1.0f, 1.0f, 0.5f, 0.0f);
    fxPclSetRadius(iID, 0.5f);
    fxPclSetScale(iID, 1.0f);
    fxPclSetPt1Color(iID, 1.0f, 1.0f, 1.0f);
    fxPclSetPt2Color(iID, 1.0f, 1.0f, 1.0f);
    fxPclSetPt3Color(iID, 0.5f, 0.5f, 0.5f);
    fxPclSetPt4Color(iID, 0.0f, 0.0f, 0.0f);
    fxPclSet4PtTimeOffset(iID, 0.0f, 100.0f, 100.0f, mathGetFloat32ChaoticNumber() * 400.0f);
    fxPclSetPosition(iID, vecPos.x, vecPos.y, vecPos.z);
    fxPclSetNumPerSec(iID, 32);

    if(!fxPclInitParticle(iID))
    {
        fxPclDetachByID(iID);
        return FALSE;
    }

    lpCon->lpVeh->StateAnimation.TankFireSmokeEmitter[i] = iID;

    iID = fxPclGetNewParticle();

    if(iID == -1) return FALSE;

    fxPclSetType(iID, IDE_PARTICLE_SMOKE_FOUNTAIN);
    fxPclSetPosition(iID, vecPos.x, vecPos.y, vecPos.z);

    fxPclPushFlags(iID, DEF_FXFLAG_INTRADIUS);
    fxPclPushFlags(iID, DEF_FXFLAG_INTALPHA);
    fxPclPushFlags(iID, DEF_FXFLAG_DOF);
    fxPclPushFlags(iID, DEF_FXFLAG_INFINITE);
    fxPclPushFlags(iID, DEF_FXFLAG_RISE);

    fxPclPushFlags(iID, DEF_FXFLAG_VARYVELOCITY);
    fxPclSetVelocityVariance(iID, 2.0f);

    fxPclPushFlags(iID, DEF_FXFLAG_USEDRAG);
    fxPclSetDragCoeff(iID, mathGetFloat32ChaoticNumber() * 4.0f);

    fxPclSet4PtScale(iID, 0.0f, 0.4f, 0.4f, 1.5f);
    fxPclSet4PtAlpha(iID, 0.0f, 0.0f, 0.5f, 0.0f);
    fxPclSetRadius(iID, 4.0f);
    fxPclSetStallTime(iID, 100.0f);
    fxPclSetParticleVel(iID, 1.0f, 1.0f, -1.0f);

    fxPclSetEmitterObject(iID, lpObj);
    fxPclSetDOFObject(iID, lpObj->dof);
    fxPclSetPt1Time(iID, (FLOAT32)frametime);
    fxPclSetScale(iID, 1.0f);
    fxPclSet4PtTimeOffset(iID, 0.0f, 300.0f, 500.0f, 3000.0f);
    fxPclSetNumPerSec(iID, 16.0f);

    // Required after settings.
    if(!fxPclInitParticle(iID))
    {
        fxPclDetachByID(iID);
        return FALSE;
    }

    lpCon->_smokeid[i] = iID;
#endif
#endif

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _CreateTreadDust
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL _CreateTreadDust(FXVEHCONTAINER* c, SINT32 i)
{
#if(0)
    if(c->lpVeh->StateAnimation.TreadDustHandle[i] > 0)
        return FALSE;

    if(c->lpVeh->StateAnimation.TreadDustEmitter[i] == nullptr)
        return FALSE;

    OBJECT* o = c->lpVeh->StateAnimation.TreadDustEmitter[i];

    if(o == nullptr) return FALSE;
    if(o->dof == nullptr) return FALSE;

    if(c->uType == VEH_TYPE_SEA)
    {
        _CreateBoatWakesAndTrails(c, i);
    }
    else
    {
#if USE_FX2
        piVECTOR n = {0, 0, 0};
        int id = fxaddeffectdof("treaddust", o, n, 0);
        c->lpVeh->StateAnimation.TreadDustHandle[i] = id;
#else
#endif
    }
#endif

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _CreateBoatWakesAndTrails
// Description:
//////////////////////////////////////////////////////////////////////
void _CreateBoatWakesAndTrails(FXVEHCONTAINER* c, SINT32 i)
{
    OBJECT* o = c->lpVeh->StateAnimation.TreadDustEmitter[i];

#if(0)//= USE_FX2
    FLOAT64VECTOR velocity;
    fxUtlGetVehicleVel(c->lpVeh, &velocity);

    float speed = math3dFastSqrt((float)mathVectDotProduct(&velocity, &velocity));

    if(speed <= 1.0f)
        return;

    piVECTOR n = {0, 0, 0};
    int id = fxaddeffectdof("boattrail", o, n, 0);
    c->lpVeh->StateAnimation.TreadDustHandle[i] = id;
#else
    SINT32 iID = fxPclGetNewParticle();

    if(iID == -1)
        return;

    // Wake
    fxPclSetType(iID, IDE_PARTICLE_CALLBACK);
    fxPclPushFlags(iID, DEF_FXFLAG_INFINITE);
    fxPclSetVehicle(iID, c->lpVeh);
    fxPclSetEmitterObject(iID, o);
    fxPclSetDOFObject(iID, o->dof);
    fxPclSetUserUpdate(iID, _BoatWakeCallback);
    fxPclSet4PtAlpha(iID, 0.25f, 0.2f, 0.1f, 0.0f);
    fxPclSet4PtScale(iID, 0.4f, 0.4f, 1.0f, 2.0f);
    fxPclSetSpacing(iID, 0.5f);
    fxPclSetPt1Color(iID, 0.13725f + 0.6f, 0.1333f + 0.6f, 0.14118f + 0.6f);

    // Required after settings.
    if(!fxPclInitParticle(iID))
    {
        fxPclDetachByID(iID);
        iID = -1;
    }

    c->lpVeh->StateAnimation.TreadDustHandle[i] = iID;

    if(i >= 2)
        return;
#endif

    // Screws
    // HACK until state system allows for more bits.
    const SCHAR* pStr = (i == 0) ? "lscrew" : "rscrew";
    ogrpFindSubObjectByName(&o,
                            c->lpVeh->VehicleObjectData->EffectsObject,
                            pStr);

    if(o == nullptr)
        return;

#if USE_FX2
    piVECTOR n = {0, 0, 0};
    int id = fxaddeffectdof("boattrail", o, n, 0);
    c->iShipScrewsID[i] = id;
#else

    iID = fxPclGetNewParticle();

    if(iID == -1)
        return;

    fxPclSetType(iID, IDE_PARTICLE_CALLBACK);
    fxPclPushFlags(iID, DEF_FXFLAG_INFINITE);
    fxPclSetVehicle(iID, c->lpVeh);
    fxPclSetEmitterObject(iID, o);
    fxPclSetDOFObject(iID, o->dof);
    fxPclSetUserUpdate(iID, _BoatTrailCallback);
    fxPclSetSpriteNumber(iID, 44);
    fxPclSetSpacing(iID, 0.5f);
    fxPclSet4PtAlpha(iID, 0.5f, 0.3f, 0.1f, 0.0f);
    fxPclSet4PtScale(iID, 0.4f, 0.4f, 1.0f, 2.0f);
    fxPclSetPt1Color(iID, 0.13725f + 0.8f, 0.1333f + 0.8f, 0.14118f + 0.8f);

    // Required after settings.
    if(!fxPclInitParticle(iID))
    {
        fxPclDetachByID(iID);
        iID = -1;
    }

    c->iShipScrewsID[i] = iID;
#endif
}

//////////////////////////////////////////////////////////////////////
// Function: _CreateWingVortex
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL _CreateWingVortex(FXVEHCONTAINER* lpCon, SINT32 i)
{
#if(0)
    if(lpCon->lpVeh->StateAnimation.WingtipVortexSmokeHandle[i] > 0)
        return FALSE;

    if(lpCon->lpVeh->StateAnimation.WingtipVortexEmitter[i] == nullptr)
        return FALSE;

    OBJECT* lpObj = lpCon->lpVeh->StateAnimation.WingtipVortexEmitter[i];

    if(lpObj == nullptr) return FALSE;
    if(lpObj->dof == nullptr) return FALSE;

    piVECTOR pos;

    pos.x = lpObj->dof->dofmatrix2._41;
    pos.y = lpObj->dof->dofmatrix2._42;
    pos.z = lpObj->dof->dofmatrix2._43;

#if USE_FX2
    piVECTOR n = {0, 0, 0};
    int id = fxaddeffectdof("wingvortex", lpObj, n, 0);
    lpCon->lpVeh->StateAnimation.WingtipVortexSmokeHandle[i] = id;
#else
    // The old vortex trail looks much better than any new stuff.
    FXCreateSmokeTrail(&lpCon->lpVeh->StateAnimation.WingtipVortexSmokeHandle[i],
                       lpObj,
                       &pos,
                       0.0f,
                       0.0f,
                       0.25f,
                       0.15f,
                       0.0f,
                       0.15f,
                       0.25f,
                       0.3f,
                       0.4f,
                       80,
                       300,
                       450,
                       10,
                       2.0f,
                       1.0f, // spacing
                       0.1f,
                       0.01f,
                       0.0f,
                       PARTICLE_FLAG_USESROTATION | PARTICLE_FLAG_USE_VEL_FOR_STRETCH);
#endif
#endif
    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _CreateExhaust
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL _CreateExhaust(FXVEHCONTAINER* lpCon, SINT32 iEngine, SINT32 iSide)
{
#if(0)
    OBJECT* lpObj = nullptr;

    // Already have exhaust created for this engine.
    if(lpCon->lpVeh->StateAnimation.ExhaustSmokeHandle[iEngine][iSide] > 0)
    {
        return FALSE;
    }

    lpObj = lpCon->lpVeh->StateAnimation.ExhaustSmokeEmitter[iEngine][iSide];

    if(lpObj == nullptr) return FALSE;
    if(lpObj->dof == nullptr) return FALSE;

#if USE_FX2
    piVECTOR n = {0, 0, -1};
    int id = fxaddeffectdof("exhaust", lpObj, n, 0);
    lpCon->lpVeh->StateAnimation.ExhaustSmokeHandle[iEngine][iSide] = id;
#else
    SINT32 iID = fxPclGetNewParticle();

    if(iID == -1) return FALSE;

    FLOAT64VECTOR vecPos;

    fxUtlGetDOFPos(lpObj->dof, &lpObj->mat, &vecPos);

    fxPclSetType(iID, IDE_PARTICLE_SMOKE_FOUNTAIN);
    fxPclSetPosition(iID, vecPos.x, vecPos.y, vecPos.z);
    fxPclPushFlags(iID, DEF_FXFLAG_INTRADIUS);
    fxPclPushFlags(iID, DEF_FXFLAG_INTALPHA);
    fxPclPushFlags(iID, DEF_FXFLAG_DOF);
    fxPclPushFlags(iID, DEF_FXFLAG_INFINITE);
    fxPclPushFlags(iID, DEF_FXFLAG_USESPRITENUM);
    fxPclSetEmitterObject(iID, lpObj);
    fxPclSetDOFObject(iID, lpObj->dof);
    fxPclSetPt1Time(iID, (FLOAT32)frametime);
    fxPclSetPt2Time(iID, (FLOAT32)frametime + 500.0f);
    fxPclSetRadius(iID, 5.0f);
    fxPclSetScale(iID, 1.0f);
    fxPclSet4PtScale(iID, 0.0f, 0.4f, 0.6f, 0.8f);
    fxPclSet4PtAlpha(iID, 0.8f, 0.5f, 0.15f, 0.0f);
    fxPclSet4PtTimeOffset(iID, 0.0f, 100.0f, 500.0f, 500.0f);
    fxPclSetNumPerSec(iID, 20.0f);

    // Initial velocity for stationary vehicle.
    switch(lpCon->uType)
    {
    case VEH_TYPE_AIR:
        {
            fxPclSetParticleVel(iID, 8.0f, 8.0f, -8.0f);
            fxPclPushFlags(iID, DEF_FXFLAG_INTCOLOR);
            fxPclSetPt1Color(iID, 1.0f, 1.0f, 1.0f);
            fxPclSetPt2Color(iID, 1.0f, 1.0f, 1.0f);
            fxPclSetPt3Color(iID, 0.0f, 0.0f, 0.0f);
            fxPclSetPt4Color(iID, 0.0f, 0.0f, 0.0f);
            fxPclSetSpriteNumber(iID, 42);
            break;
        }
    case VEH_TYPE_GROUND:
    case VEH_TYPE_SEA:
        {
            fxPclSetParticleVel(iID, 4.0f, 4.0f, -4.0f);
            break;
        }
    }

    lpCon->lpVeh->StateAnimation.ExhaustSmokeHandle[iEngine][iSide] = iID;

    // Required after settings.
    if(!fxPclInitParticle(iID))
    {
        fxPclDetachByID(iID);
    }
#endif
#endif

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _CreateSmokeStack
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL _CreateSmokeStack(FXVEHCONTAINER* lpCon, SINT32 iEngine, SINT32 iSide)
{
#if(0)

    // TBD - smoke stack
    OBJECT* lpObj = nullptr;

    if(lpCon->iShipStackSmoke[iEngine][iSide] > 0)
        return FALSE;

    lpObj = lpCon->lpVeh->StateAnimation.ExhaustSmokeEmitter[iEngine][iSide];

    if(lpObj == nullptr) return FALSE;
    if(lpObj->dof == nullptr) return FALSE;

#if USE_FX2
    piVECTOR n = {0, 0, 0};
    int id = 0;
    static WEAPON_SPEC ws = NULL_WEAP_SPEC;
    static bool firsttime = true;

    if(firsttime)
    {
        ws = weapFindClass("riverine");
        firsttime = false;
    }

    if(lpCon->lpVeh->VehicleType->WeaponSpec.wClass == ws.wClass)
    {
        id = fxaddeffectdof("smokestacksmall", lpObj, n, 0);
    }
    else
    {
        id = fxaddeffectdof("smokestacklarge", lpObj, n, 0);
    }

    lpCon->iShipStackSmoke[iEngine][iSide] = id;
#else
    SINT32 iID = fxPclGetNewParticle();

    if(iID == -1) return FALSE;

    FLOAT64VECTOR vecPos;

    fxUtlGetDOFPos(lpObj->dof, &lpObj->mat, &vecPos);

    fxPclSetType(iID, IDE_PARTICLE_SMOKE_FOUNTAIN);
    fxPclSetPosition(iID, vecPos.x, vecPos.y, vecPos.z);
    fxPclPushFlags(iID, DEF_FXFLAG_INTRADIUS);
    fxPclPushFlags(iID, DEF_FXFLAG_INTALPHA);
    fxPclPushFlags(iID, DEF_FXFLAG_DOF);
    fxPclPushFlags(iID, DEF_FXFLAG_INFINITE);
    fxPclPushFlags(iID, DEF_FXFLAG_VARYVELOCITY);

    // In case boat sinks.
    fxPclPushFlags(iID, DEF_FXFLAG_STOPATGROUND);

    fxPclSetParticleVel(iID, 0.0f, 0.0f, -5.0f);

    fxPclSetEmitterObject(iID, lpObj);
    fxPclSetDOFObject(iID, lpObj->dof);
    fxPclSetPt1Time(iID, (FLOAT32)frametime);
    fxPclSetRadius(iID, 15.0f);
    fxPclSetScale(iID, 1.0f);
    fxPclSet4PtScale(iID, 0.1f, 0.4f, 0.4f, 1.0f);
    fxPclSet4PtAlpha(iID, 0.0f, 0.05f, 0.01f, 0.0f);
    fxPclSet4PtTimeOffset(iID, 0.0f, 500.0f, 500.0f, 2000.0f);
    fxPclSetNumPerSec(iID, 2.0f);
    lpCon->iShipStackSmoke[iEngine][iSide] = iID;

    fxPclSetUserKillCallback(iID, _SmokeStackKill);
    fxPclSetKillData(iID, &lpCon->iShipStackSmoke[iEngine][iSide]);

    // Required after settings.
    if(!fxPclInitParticle(iID))
    {
        fxPclDetachByID(iID);
    }
#endif
#endif

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _SmokeStackKill
// Description:
//////////////////////////////////////////////////////////////////////
#ifndef USE_FX2
void _SmokeStackKill(void* pData)
{
    SINT32* pID = (SINT32*)pData;

    if(pID == nullptr)
        return;

    *pID = 0;
}
#endif

//////////////////////////////////////////////////////////////////////
// Function: vehfxCreateShockSmoke
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL vehfxCreateShockSmoke(SINT32 id)
{
    if(fxWeatherIsRaining())
        return FALSE;

    SINT32 idx = id - 1;

    if(idx < 0 || idx >= MAX_POSSIBLE_VIS_VEHICLES)
        return FALSE;

    if(!g_lpCon[idx].lpVeh)
        return FALSE;

    if(!g_lpCon[idx].lpVeh->VehicleType)
        return FALSE;

    // Only want gun shock smoke from ground vehicles
    if(g_lpCon[idx].lpVeh->VehicleType->Type != VEH_TYPE_GROUND)
        return FALSE;

    // Use tread dust emitters for dust positions.
    for(SINT32 i = 0; i < VEH_MAX_TREAD_DUST; i++)
    {
        OBJECT* lpObj = g_lpCon[idx].lpVeh->StateAnimation.TreadDustEmitter[i];

        if(lpObj == nullptr) continue;
        if(lpObj->dof == nullptr) continue;

        TransformEffect* te = TransformEffect::AddEffect("shockdust", Vec3f(0, 0, 0), 0, lpObj);

        if(te)
        {
            te->enableemitters(true);;
            te->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);
        }
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _CreateDebris
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL _CreateDebris(FXVEHCONTAINER* lpCon, SINT32 i, FLOAT32 scale)
{
	if(lpCon->lpVeh == nullptr) return FALSE;

	OBJECT* lpObj = lpCon->lpVeh->StateAnimation.DebrisEmitter[i];

	if(lpObj == nullptr) return FALSE;
	if(lpObj->dof == nullptr) return FALSE;

	SINT32 iID;

	piVECTOR dir;
	FLOAT64VECTOR vecPos;
	FLOAT64VECTOR vecVel;
	FLOAT32 fLength, x, y, z;

	fxUtlGetVehicleVel(lpCon->lpVeh, &vecVel);
	fxUtlGetDOFPos(lpObj->dof, &lpObj->mat, &vecPos);

	fLength = (FLOAT32)mathExactVectorLength(&vecVel);

	for(SINT32 j = 0; j < 10; j++)
	{
		OBJECT* o = _getFreeDebris();

		// No more debris to display
		if(!o)
			return FALSE;

		iID = fxPclGetNewParticle();

		if(iID == -1) return FALSE;

		fxPclSetType(iID, IDE_PARTICLE_DEBRIS);
		fxPclPushFlags(iID, DEF_FXFLAG_OBJECT);
		fxPclPushFlags(iID, DEF_FXFLAG_ROTATE);
		fxPclPushFlags(iID, DEF_FXFLAG_USEGRAVITY);
		fxPclSetUserKillCallback(iID, vehfxFXOBJKillCallback);
		fxPclSetPt1Time(iID, (FLOAT32)frametime);
		fxPclSetPt4Time(iID, (FLOAT32)frametime + 5000.0f);
		fxPclSetDirection(iID, 0, 1, 0);
		fxPclSetPosition(iID, vecPos.x, vecPos.y, vecPos.z);

		fxPclSetFXObject(iID, o);
		fxPclSetKillData(iID, o);

		fxPclSetScale(iID, mathGetFloat32ChaoticNumber() * scale);
		fxPclSetRadius(iID, 1.0f);

		// Need random rotation.
		x = (FLOAT32)(mathGetChaoticNumber() % 20) * g_fPiOver180;
		y = (FLOAT32)(mathGetChaoticNumber() % 20) * g_fPiOver180;
		z = (FLOAT32)(mathGetChaoticNumber() % 20) * g_fPiOver180;

		fxPclSetRotate(iID, x, y, z);

		// Assign random direction.
		dir.x = mathGetFloat32ChaoticNumber() * 2.0f - 1.0f;
		dir.y = mathGetFloat32ChaoticNumber() * 2.0f - 1.0f;
		dir.z = mathGetFloat32ChaoticNumber() * 2.0f - 1.0f;

		mathReallyFastNormalize(dir);

		x = dir.x * 10.0f;
		y = dir.y * 10.0f;
		z = dir.z * 10.0f;

		x += (FLOAT32)vecVel.x;
		y -= (FLOAT32)vecVel.y;
		z += (FLOAT32)vecVel.z;

		fxPclSetParticleVel(iID, x, y, z);

		// Required after settings.
		if(!fxPclInitParticle(iID))
		{
			fxPclDetachByID(iID);
		}
	}

	return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _CreateExplosion
// Description: Use debris emitters for now on explosions
//////////////////////////////////////////////////////////////////////
LOGICAL _CreateExplosion(FXVEHCONTAINER* lpCon, SINT32 i)
{
    if(lpCon->lpVeh == nullptr) return FALSE;

    OBJECT* lpObj = lpCon->lpVeh->StateAnimation.DebrisEmitter[i];

    if(lpObj == nullptr) return FALSE;
    if(lpObj->dof == nullptr) return FALSE;

    FLOAT64VECTOR vecPos;

    fxUtlGetDOFPos(lpObj->dof, &lpObj->mat, &vecPos);

    fxPclCreateExplosion(&vecPos, 5000000.0f);

    piVECTOR vPos;

    vPos.x = lpObj->dof->dofmatrix2._41;
    vPos.y = lpObj->dof->dofmatrix2._42;
    vPos.z = lpObj->dof->dofmatrix2._43;

    // Do this for aircraft only, in case we need a fire trail.
    if(lpCon->lpVeh->VehicleType->Type != VEH_TYPE_AIR)
        return TRUE;

    TransformEffect* te = TransformEffect::AddEffect("fuelfire1", Vec3f(0, 0, 0), 0, lpObj);

    if(te)
    {
        te->enableemitters(true);;
        te->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);
    }

#if(0)
    UINT32 uTrash;
    // Use old FX for now in creating a smoke trail
    FXCreateSmokeTrail(&uTrash,
                       lpObj,
                       &vPos,
                       0.3f, // Drag
                       0.3f, // Init Alpha
                       0.2f,
                       0.1f,
                       0.0f,
                       1.5f,
                       2.0f,
                       4.5f,
                       6.0f,
                       100,
                       200,
                       300,
                       11,
                       PI*2,
                       2.0f,
                       0.3f,
                       0.3f,
                       0.1f,
                       PARTICLE_FLAG_USESDRAG |
                       PARTICLE_FLAG_GLOWS |
	                   PARTICLE_FLAG_USESGRAVITY |
	                   PARTICLE_FLAG_USE_VEL_FOR_STRETCH);
#endif
    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _CreateVehicleFire
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL _CreateVehicleFire(FXVEHCONTAINER* lpCon, SINT32 i)
{
    if(lpCon->lpVeh == nullptr)
        return FALSE;

    if(lpCon->_fireid[i] > 0)
        return FALSE;

    OBJECT* lpObj = nullptr;

    // Currently all vehicle fire stems from hfire1. If more are needed
    // in the future, add an emitter array to the state animation structure.
    ogrpFindSubObjectByName(&lpObj,
                            lpCon->lpVeh->VehicleObjectData->EffectsObject,
                            "hfire1");

    if(lpObj == nullptr) return FALSE;
    if(lpObj->dof == nullptr) return FALSE;

    TransformEffect* te = TransformEffect::AddEffect("vehiclefire", Vec3f(0, 0, -1), 0, lpObj);

    if(te)
    {
        te->enableemitters(true);;
        te->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: vehfxGetFreeDebris
// Description:
//////////////////////////////////////////////////////////////////////
OBJECT* _getFreeDebris()
{
    if(_freedebris.empty())
    {
        return nullptr;
    }

    OBJECT* o = _freedebris.front();
    _freedebris.pop_front();

    return o;
}

//////////////////////////////////////////////////////////////////////
// Function: vehfxFXOBJKillCallback
// Description:
//////////////////////////////////////////////////////////////////////
void vehfxFXOBJKillCallback(void* data)
{
    OBJECT* o = (OBJECT*)data;

    if(o == nullptr)
    {
        return;
    }

    fxUtlSetMatrixInTree(o, &IdentityMatrix);

    _freedebris.push_back(o);
}

//////////////////////////////////////////////////////////////////////
// Function: vehfxCheckExhaustState
// Description:
//////////////////////////////////////////////////////////////////////
void vehfxCheckExhaustState(SINT32 iID)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= MAX_POSSIBLE_VIS_VEHICLES)
    {
        return;
    }

    FXVEHCONTAINER* lpCon = &g_lpCon[iIndex];

    _UpdateExhaust(lpCon);
}

//////////////////////////////////////////////////////////////////////
// Function: vehfxSetDebrisState
// Description:
//////////////////////////////////////////////////////////////////////
void vehfxSetDebrisState(SINT32 iID)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= MAX_POSSIBLE_VIS_VEHICLES)
    {
        return;
    }

    _UpdateDebris(&g_lpCon[iIndex]);
}

//////////////////////////////////////////////////////////////////////
// Function: vehfxCheckSparkState
// Description: Shoot off some sparks during burning aircraft
//////////////////////////////////////////////////////////////////////
void vehfxCheckSparkState(SINT32 iID)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= MAX_POSSIBLE_VIS_VEHICLES)
    {
        return;
    }

    _UpdateSparks(&g_lpCon[iIndex]);
}

//////////////////////////////////////////////////////////////////////
// Function: vehfxCheckTankFireState
// Description: Tank fire
//////////////////////////////////////////////////////////////////////
void vehfxCheckTankFireState(SINT32 iID)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= MAX_POSSIBLE_VIS_VEHICLES)
        return;

    _UpdateTankFire(&g_lpCon[iIndex]);
}

//////////////////////////////////////////////////////////////////////
// Function: vehfxSetVehicleFireState
// Description: Tank fire
//////////////////////////////////////////////////////////////////////
void vehfxSetVehicleFireState(SINT32 iID)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= MAX_POSSIBLE_VIS_VEHICLES)
        return;

    _UpdateVehicleFire(&g_lpCon[iIndex]);
}

//////////////////////////////////////////////////////////////////////
// Function: vehfxCheckTreadDustState
// Description: Vehicle tread dust
//////////////////////////////////////////////////////////////////////
void vehfxCheckTreadDustState(SINT32 iID)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= MAX_POSSIBLE_VIS_VEHICLES)
        return;

    _UpdateTreadDust(&g_lpCon[iIndex]);
}

//////////////////////////////////////////////////////////////////////
// Function: vehfxCheckWingVortexState
// Description:
//////////////////////////////////////////////////////////////////////
void vehfxCheckWingVortexState(SINT32 iID)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= MAX_POSSIBLE_VIS_VEHICLES)
        return;

    _UpdateWingVortex(&g_lpCon[iIndex]);
}

//////////////////////////////////////////////////////////////////////
// Function: vehfxSetExplosionState
// Description:
//////////////////////////////////////////////////////////////////////
void vehfxSetExplosionState(SINT32 iID)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= MAX_POSSIBLE_VIS_VEHICLES)
        return;

    _UpdateExplosion(&g_lpCon[iIndex]);
}

//////////////////////////////////////////////////////////////////////
// Function: vehfxUpdateTurretSound
// Description:
//////////////////////////////////////////////////////////////////////
void vehfxUpdateTurretSound(SINT32 ID)
{
    SINT32 i = ID - 1;

    if(i < 0 || i >= MAX_POSSIBLE_VIS_VEHICLES)
        return;

    _updateTurretSound(&g_lpCon[i]);
}

//////////////////////////////////////////////////////////////////////
// Function: vehfxPlayTankTreadExternal
// Description: Play tank tread sound for external view / 3rd person
//////////////////////////////////////////////////////////////////////
LOGICAL vehfxPlayTankTreadExternal(SINT32 iID, LOGICAL bOn)
{
    LOGICAL bSuccess = FALSE;

    // External sound is now the same as internal.
    bSuccess = vehfxPlayTankTreadInternal(iID, bOn);

    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= MAX_POSSIBLE_VIS_VEHICLES)
        return FALSE;

    FXVEHCONTAINER* lpCon = &g_lpCon[iIndex];

    // Change to external sound to muffle if inside a vehicle.
    for(SINT32 i = 0; i < VEH_MAX_TREAD_DUST; i++)
    {
        if(lpCon->iTankTreadSnd[i] > 0)
        {
            fxSndSetSoundType(lpCon->iTankTreadSnd[i],
                              TYPE_INFINITE_EXTERNAL_SOUND);
        }
    }

    return bSuccess;
}

//////////////////////////////////////////////////////////////////////
// Function: vehfxPlayTankTreadInternal
// Description: Play tank tread sound if internal to tank / 1st person
//////////////////////////////////////////////////////////////////////
LOGICAL vehfxPlayTankTreadInternal(SINT32 id, LOGICAL bOn)
{
    SINT32 iIndex = id - 1;
    SINT32 i;

    if(iIndex < 0 || iIndex >= MAX_POSSIBLE_VIS_VEHICLES)
        return FALSE;

    FXVEHCONTAINER* lpCon = &g_lpCon[iIndex];

    if(lpCon->lpVeh == nullptr)
        return FALSE;

    if(!lpCon->lpVeh->VehicleType)
        return FALSE;

    if(!lpCon->lpVeh->VehicleType->bHasTreadSound)
        return FALSE;

    // First person vehicles only
    // HACK! Prevent tread sound starting on initial spawn in.
    if(lpCon->lpVeh->ModelData)
    {
        if(lpCon->lpVeh->ModelData->SpawnTime == 0)
            return FALSE;

        // Stall 3 seconds before starting sounds.
        if(frametime - lpCon->lpVeh->ModelData->SpawnTime < 3000)
        {
            return FALSE;
        }
    }

    if(bOn == FALSE)
    {
        for(i = 0; i < VEH_MAX_TREAD_DUST; i++)
        {
            if(lpCon->iTankTreadSnd[i] > 0)
            {
                fxSndStopSoundDelayed(lpCon->iTankTreadSnd[i], 1);
                lpCon->iTankTreadSnd[i] = 0;
            }
        }

        return TRUE;
    }

    // Attach sound to tread dust emitters
    for(i = 0; i < VEH_MAX_TREAD_DUST; i++)
    {
        if(lpCon->iTankTreadSnd[i] > 0)
        {
            fxSndChangePitch(lpCon->iTankTreadSnd[i], _GetPitch(lpCon->lpVeh));
            continue;
        }

        if(lpCon->lpVeh->StateAnimation.TreadDustEmitter[i] == nullptr)
            continue;

        OBJECT* lpObj = lpCon->lpVeh->StateAnimation.TreadDustEmitter[i];

        if(lpObj == nullptr) return FALSE;
        if(lpObj->dof == nullptr) return FALSE;

        FLOAT64VECTOR vecPos;

        vecPos.x =  lpObj->dof->dofmatrix2._41;
        vecPos.y =  lpObj->dof->dofmatrix2._42;
        vecPos.z = -lpObj->dof->dofmatrix2._43;

        SINT32 iID = fxSndGetNewSound();

        if(iID == -1)
            return FALSE;

        fxSndSetSoundName(iID, (SCHAR*)lpCon->lpVeh->VehicleType->szTreadSound);
        fxSndSetSoundType(iID, TYPE_INFINITE_SOUND);
        fxSndSetPitch(iID, 1.0f);
        fxSndSetVolume(iID, 1.0f);
        fxSndSetCreateTime(iID, frametime);
        fxSndSetPlayType(iID, DEF_PLAYTYPE_OBJECT);
        fxSndSetEmitterObject(iID, lpObj);
        fxSndSetPriority(iID, IDE_FP_PRIORITY_LOW);
        fxSndSetPosition(iID, vecPos.x, vecPos.y, vecPos.z);
        fxSndSetMinDistance(iID, 10.0f);
        fxSndSetMaxDistance(iID, 100.0f);
        fxSndSetKillCallback(iID, _KillSoundCallback);
        fxSndSetKillData(iID, &lpCon->iTankTreadSnd[i]);

        if(!fxSndInitSound(iID))
            continue;

        lpCon->iTankTreadSnd[i] = iID;
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: vehfxPlayTankBrake
// Description: Play tank brake sound
//////////////////////////////////////////////////////////////////////
LOGICAL vehfxPlayTankBrake(SINT32 id, LOGICAL bOn)
{
    SINT32 iIndex = id - 1;
    SINT32 i;

    if(iIndex < 0 || iIndex >= MAX_POSSIBLE_VIS_VEHICLES)
        return FALSE;

    FXVEHCONTAINER* lpCon = &g_lpCon[iIndex];

    if(lpCon->lpVeh == nullptr)
        return FALSE;

    if(!lpCon->lpVeh->VehicleType)
        return FALSE;

    if(!lpCon->lpVeh->VehicleType->bHasBrakeSound)
        return FALSE;

    if(bOn == FALSE)
    {
        for(i = 0; i < VEH_MAX_TREAD_DUST; i++)
        {
            if(lpCon->iTankBrakeSnd[i] > 0)
            {
                fxSndStopSoundDelayed(lpCon->iTankBrakeSnd[i], 1);
                lpCon->iTankBrakeSnd[i] = 0;
            }
        }

        return TRUE;
    }

    // Attach sound to tread dust emitters
    for(i = 0; i < VEH_MAX_TREAD_DUST; i++)
    {
        if(lpCon->iTankBrakeSnd[i] > 0)
        {
            fxSndChangePitch(lpCon->iTankBrakeSnd[i], _GetPitch(lpCon->lpVeh));
            continue;
        }

        if(lpCon->lpVeh->StateAnimation.TreadDustEmitter[i] == nullptr)
            return FALSE;

        OBJECT* lpObj = lpCon->lpVeh->StateAnimation.TreadDustEmitter[i];

        if(lpObj == nullptr) return FALSE;
        if(lpObj->dof == nullptr) return FALSE;

        FLOAT64VECTOR vecPos;

        vecPos.x =  lpObj->dof->dofmatrix2._41;
        vecPos.y =  lpObj->dof->dofmatrix2._42;
        vecPos.z = -lpObj->dof->dofmatrix2._43;

        SINT32 iID = fxSndGetNewSound();

        if(iID == -1)
            return FALSE;

        fxSndSetSoundName(iID, (SCHAR*)lpCon->lpVeh->VehicleType->szBrakeSound);
        fxSndSetSoundType(iID, TYPE_INFINITE_SOUND);
        fxSndSetPitch(iID, 1.0f);
        fxSndSetVolume(iID, 1.0f);
        fxSndSetCreateTime(iID, frametime);
        fxSndSetPlayType(iID, DEF_PLAYTYPE_OBJECT);
        fxSndSetEmitterObject(iID, lpObj);
        fxSndSetPriority(iID, IDE_FP_PRIORITY_LOW);
        fxSndSetPosition(iID, vecPos.x, vecPos.y, vecPos.z);
        fxSndSetMinDistance(iID, 10.0f);
        fxSndSetMaxDistance(iID, 100.0f);
        fxSndSetKillCallback(iID, _KillSoundCallback);
        fxSndSetKillData(iID, &lpCon->iTankBrakeSnd[i]);

        if(!fxSndInitSound(iID))
            continue;

        lpCon->iTankBrakeSnd[i] = iID;
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _GetPitch
// Description:
//////////////////////////////////////////////////////////////////////
FLOAT32 _GetPitch(vehVehicle* lpVeh)
{
    if(lpVeh == nullptr)
        return 1.0f;

    if(lpVeh->VehicleType == nullptr)
        return 1.0f;

    FLOAT32 fSpeedEnd = lpVeh->VehicleType->EngineSpeedEnd[0];
    FLOAT32 fPitchEnd = lpVeh->VehicleType->EnginePitchEnd[0];
    FLOAT32 fSpeedStart = lpVeh->VehicleType->EngineSpeedStart[0];
    FLOAT32 fPitchStart = lpVeh->VehicleType->EnginePitchStart[0];

    if(fSpeedEnd - fSpeedStart < 0.0001f)
        return 1.0f;

    FLOAT32 fSpeed = (FLOAT32)mathExactVectorLength(&lpVeh->PosData.Vel);

	FLOAT32 fPitch = 1.0f - (fSpeedEnd - fSpeed) / (fSpeedEnd - fSpeedStart);
	fPitch = fPitch * (fPitchEnd  - fPitchStart) + fPitchStart;

    return fPitch;
}

//////////////////////////////////////////////////////////////////////
// Function: _KillSoundCallback
// Description:
//////////////////////////////////////////////////////////////////////
void _KillSoundCallback(void* data)
{
    SINT32* pHandle = (SINT32*)data;

    if(pHandle == nullptr)
        return;

    *pHandle = 0;
}

//////////////////////////////////////////////////////////////////////
// Function: vehfxRemoveAllEffects
// Description:
//////////////////////////////////////////////////////////////////////
void vehfxRemoveAllEffects(SINT32 iID)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= MAX_POSSIBLE_VIS_VEHICLES)
        return;

    FXVEHCONTAINER* lpCon = &g_lpCon[iIndex];

    for(SINT32 i = 0; i < VEH_MAX_FIREPOINTS; i++)
    {
        if(lpCon->_fireid[i] > 0)
        {
#if USE_FX2
            fxremove(lpCon->_fireid[i]);
#else
            fxPclDetachByID(lpCon->_fireid[i]);
#endif
            lpCon->_fireid[i] = 0;
        }

        if(lpCon->_smokeid[i] > 0)
        {
#if USE_FX2
            fxremove(lpCon->_smokeid[i]);
#else
            fxPclDetachByID(lpCon->_smokeid[i]);
#endif
            lpCon->_smokeid[i] = 0;
        }

        if(lpCon->lpVeh != nullptr && lpCon->lpVeh->StateAnimation.TankFireSmokeEmitter[i] > 0)
        {
#if USE_FX2
            fxremove(lpCon->lpVeh->StateAnimation.TankFireSmokeEmitter[i]);
#else
            fxPclDetachByID(lpCon->lpVeh->StateAnimation.TankFireSmokeEmitter[i]);
#endif
            lpCon->lpVeh->StateAnimation.TankFireSmokeEmitter[i] = 0;
        }

#if USE_FX2
        if(lpCon->lpVeh->StateAnimation.FuelFireHandle[i] > 0)
        {
            fxremove(lpCon->lpVeh->StateAnimation.FuelFireHandle[i]);
            lpCon->lpVeh->StateAnimation.FuelFireHandle[i] = 0;
        }

        if(lpCon->lpVeh->StateAnimation.FuelFireSmokeHandle[i] > 0)
        {
            fxremove(lpCon->lpVeh->StateAnimation.FuelFireSmokeHandle[i]);
            lpCon->lpVeh->StateAnimation.FuelFireSmokeHandle[i] = 0;
        }

        if(lpCon->lpVeh->StateAnimation.OilLeakSmokeHandle[i] > 0)
        {
            fxremove(lpCon->lpVeh->StateAnimation.OilLeakSmokeHandle[i]);
            lpCon->lpVeh->StateAnimation.OilLeakSmokeHandle[i] = 0;
        }

        if(lpCon->lpVeh->StateAnimation.FuelLeakSmokeHandle[i] > 0)
        {
            fxremove(lpCon->lpVeh->StateAnimation.FuelLeakSmokeHandle[i]);
            lpCon->lpVeh->StateAnimation.FuelLeakSmokeHandle[i] = 0;
        }

        if(lpCon->lpVeh->StateAnimation.EngineFireHandle[i] > 0)
        {
            fxremove(lpCon->lpVeh->StateAnimation.EngineFireHandle[i]);
            lpCon->lpVeh->StateAnimation.EngineFireHandle[i] = 0;
        }

        for(int j = 0; j < ML_MAX_RADIATORS_PER_ENGINE; j++)
        {
            if(lpCon->lpVeh->StateAnimation.GlycolLeakSmokeHandle[i][j] > 0)
            {
                fxremove(lpCon->lpVeh->StateAnimation.GlycolLeakSmokeHandle[i][j]);
                lpCon->lpVeh->StateAnimation.GlycolLeakSmokeHandle[i][j] = 0;
            }
        }
#endif
    }
}

//////////////////////////////////////////////////////////////////////
// Function: vehfxWheelsOffEffect
// Description:
//////////////////////////////////////////////////////////////////////
void vehfxWheelsOffEffect(SINT32 id, SINT32 debrisindex)
{
    SINT32 index = id - 1;

    if(index < 0 || index >= MAX_POSSIBLE_VIS_VEHICLES)
    {
        return;
    }

    FXVEHCONTAINER* container = &g_lpCon[index];

    if(!container->lpVeh)
    {
        return;
    }

    _CreateDebris(container, debrisindex, 0.2f);

    OBJECT* o = container->lpVeh->StateAnimation.DebrisEmitter[debrisindex];

    if(!o)
    {
        return;
    }

    if(!o->dof)
    {
        return;
    }

    FLOAT64VECTOR pos;

    fxUtlGetDOFPos(o->dof, &o->mat, &pos);

    SINT32 numparts = 16;

    piVECTOR vel, u;

    FLOAT32 fortyfive = PI * 0.25f;
    FLOAT32 radius;

    UINT32 flags = DEF_FXFLAG_USEGRAVITY | DEF_FXFLAG_USEDRAG;

    for(SINT32 i = 0; i < numparts; i++)
    {
        radius = 0.05f * mathGetFloat32ChaoticNumber();

        u.x = fortyfive * (mathGetFloat32ChaoticNumber() * 2.0f - 1.0f);
        u.y = fortyfive * (mathGetFloat32ChaoticNumber() * 2.0f - 1.0f);
        u.z = fortyfive * (mathGetFloat32ChaoticNumber() * 2.0f - 1.0f);

        vel.x = u.x * 10.0f;
        vel.y = u.y * 10.0f;
        vel.z = u.z * 10.0f;

        fxPclCreateSpark(&pos,
                         &vel,
                         radius,
                         500.0f,
                         flags,
                         TRUE,
                         FALSE,
                         0.0f);
    }

    SINT32 ID = fxPclGetNewParticle();

    if(ID == -1)
    {
        return;
    }

    fxPclSetType(ID, IDE_PARTICLE_SMOKE_FOUNTAIN);
    fxPclSetPosition(ID, pos.x, pos.y, pos.z);
    fxPclPushFlags(ID, DEF_FXFLAG_INTRADIUS);
    fxPclPushFlags(ID, DEF_FXFLAG_INTALPHA);
    fxPclSetPt1Time(ID, (FLOAT32)frametime);
    fxPclSetPt4Time(ID, (FLOAT32)frametime + 300.0f);
    fxPclSetRadius(ID, 8.0f);
    fxPclSetScale(ID, 1.0f);
    fxPclSet4PtScale(ID, 0.0f, 0.4f, 0.6f, 1.0f);
    fxPclSet4PtAlpha(ID, 0.4f, 0.2f, 0.1f, 0.0f);
    fxPclSet4PtTimeOffset(ID, 0.0f, 100.0f, 500.0f, 2000.0f);
    fxPclSetFXVel(ID, 0.0f, 0.0f, 1.0f);
    fxPclSetNumPerSec(ID, 10.0f);

    // Required after settings.
    if(!fxPclInitParticle(ID))
    {
        fxPclDetachByID(ID);
    }
}

//////////////////////////////////////////////////////////////////////
// Function: vehfxCreateHulkSmoke
// Description:
//////////////////////////////////////////////////////////////////////
void vehfxCreateHulkSmoke(SINT32 id)
{
    if(id - 1 <= 0)
    {
        return;
    }

    FXVEHCONTAINER* container = &g_lpCon[id - 1];

    if(container->_fireid[0] > 0)
    {
        return;
    }

    vehVehicle*veh = container->lpVeh;
    OBJECT* emitter = nullptr;

    if(!veh->VehicleObjectData)
    {
        return;
    }

    // Currently all vehicle fire stems from hfire1. If more are needed
    // in the future, add an emitter array to the state animation structure.
    ogrpFindSubObjectByName(&emitter,
                            veh->VehicleObjectData->EffectsObject,
                            "hfire1");

    if(!emitter)
    {
        return;
    }

    if(!emitter->dof)
    {
        return;
    }

    TransformEffect* te = TransformEffect::AddEffect("hulksmoke", Vec3f(0, 0, -1), 0, emitter);

    if(te)
    {
        te->enableemitters(true);;
        te->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);

        // HACK.
        container->_fireid[0] = INT_MAX;
    }

#if(0)
    SINT32 ID = fxPclGetNewParticle();

    if(ID == -1)
        return;

    fxPclSetType(ID, IDE_PARTICLE_SMOKE_FOUNTAIN);

    fxPclPushFlags(ID, DEF_FXFLAG_INFINITE);
    fxPclPushFlags(ID, DEF_FXFLAG_VARYVELOCITY);
    fxPclPushFlags(ID, DEF_FXFLAG_INTRADIUS);
    fxPclPushFlags(ID, DEF_FXFLAG_INTALPHA);
    fxPclPushFlags(ID, DEF_FXFLAG_ROTATE);
    fxPclPushFlags(ID, DEF_FXFLAG_MYCOLOR);

    fxPclSetFXVel(ID, 0.0f, 0.0f, 0.7f);

    fxPclSetVelocityVariance(ID, 0.4f);
    fxPclSetPt1Time(ID, (FLOAT32)frametime);
    fxPclSetRadius(ID, 8.0f);
    fxPclSetScale(ID, 1.0f);
    fxPclSet4PtScale(ID, 0.1f, 0.6f, 0.8f, 1.0f);
    fxPclSet4PtAlpha(ID, 0.2f, 0.1f, 0.05f, 0.0f);
    fxPclSet4PtTimeOffset(ID, 0.0f, 2500.0f, 2500.0f, 2500.0f);
    fxPclSetNumPerSec(ID, 3.0f);
    fxPclSetEmitterObject(ID, emitter);

    FLOAT64VECTOR pos = veh->PosData.Pos;

    fxPclSetPosition(ID, pos.x, pos.y, pos.z - 0.5f);

    if(!fxPclInitParticle(ID))
    {
        fxPclDetachByID(ID);
        ID = -1;
    }

    container->_fireid[0] = ID;
#endif
}

//////////////////////////////////////////////////////////////////////
// Function: _soundhulkupdate
// Description:
//////////////////////////////////////////////////////////////////////
void _soundhulkupdate(FXSOUND* s)
{
    if(s->starttime && frametime - s->updatetimer > 100)
    {
        s->updatetimer = frametime;
        fxSndChangePitch(s->ID, s->originalPitch + 0.005f);
    }
}

//////////////////////////////////////////////////////////////////////
// Function: vehfxSinkVehicle
// Description:
//////////////////////////////////////////////////////////////////////
void vehfxSinkVehicle(SINT32 id, SINT32 duration, FLOAT32 rate)
{
    FXVEHCONTAINER* container = &g_lpCon[id - 1];

    if(container->_fireid[0] > 0)
    {
        fxPclDetachByID(container->_fireid[0]);
    }

    container->startsinktime = frametime;
    container->duration = duration;
    container->effectrate = rate;
    container->updateflags |= VEHFX_SINK_VEHICLE;

    _updates.insert(id);

    vehVehicle*veh = container->lpVeh;
    FLOAT64VECTOR pos = veh->PosData.Pos;

    piVECTOR relpos;

    relpos.x = (FLOAT32)(pos.x - WorldOriginOffset.x);
    relpos.y = (FLOAT32)(pos.y - WorldOriginOffset.y);
    relpos.z = (FLOAT32)(pos.z);

    id = fxSndGetNewSound();

    if(id == -1)
    {
        return;
    }

    fxSndSetPitch(id, GetRandom(0.7f, 1.0f));
    fxSndSetVolume(id, 1.0f);
    fxSndSetPriority(id, IDE_TP_PRIORITY_LOW);
    fxSndSetSoundType(id, TYPE_FINITE_EXTERNAL_SOUND);
    fxSndSetSoundName(id, "sounds/hulksink.wav");
    fxSndSetPosition(id, pos.x, pos.y, pos.z);
    fxSndSetPlayType(id, DEF_PLAYTYPE_WORLD);
    fxSndSetMinDistance(id, 10.0f);
    fxSndSetMaxDistance(id, 40.0f);
    fxSndSetUserUpdate(id, _soundhulkupdate);

    // Required after settings
    fxSndInitSound(id);
}

//////////////////////////////////////////////////////////////////////
// Function: vehfxHulkExplosion
// Description:
//////////////////////////////////////////////////////////////////////
void vehfxHulkExplosion(SINT32 ID)
{
    FXVEHCONTAINER* container = &g_lpCon[ID - 1];

    vehVehicle*veh = container->lpVeh;
    FLOAT64VECTOR pos = veh->PosData.Pos;

    piVECTOR relpos;

    relpos.x = (FLOAT32)(pos.x - WorldOriginOffset.x);
    relpos.y = (FLOAT32)(pos.y - WorldOriginOffset.y);
    relpos.z = (FLOAT32)(pos.z);

    FLOAT32 distance = playdir.x * relpos.x +
                       playdir.y * relpos.y +
                       playdir.z * relpos.z +
                       playtrans.z;

    // Don't display explosion if over 500 meters
    if(distance > 500.0f)
    {
        return;
    }

    pos.z -= 0.5f;

    fxPclCreateExplosion(&pos, 5000000.0f);

    SINT32 id = fxSndGetNewSound();

    if(id == -1)
    {
        return;
    }

    fxSndSetPitch(id, 1.0f);
    fxSndSetVolume(id, 1.0f);
    fxSndSetPriority(id, IDE_TP_PRIORITY_LOW);
    fxSndSetSoundType(id, TYPE_FINITE_EXTERNAL_SOUND);
    fxSndSetSoundName(id, "sounds\\medcloseexp.wav");
    fxSndSetPosition(id, pos.x, pos.y, pos.z);
    fxSndSetPlayType(id, DEF_PLAYTYPE_WORLD);
    fxSndSetMinDistance(id, 100.0f);
    fxSndSetMaxDistance(id, 3218.0f);

    // Required after settings
    fxSndInitSound(id);
}

//////////////////////////////////////////////////////////////////////
// Function: _updatecontainer
// Description:
//////////////////////////////////////////////////////////////////////
static void _updatecontainer(SINT32 id)
{
    FXVEHCONTAINER* container = &g_lpCon[id - 1];

    if(container->updateflags & VEHFX_SINK_VEHICLE)
    {
        if(frametime - container->startsinktime >= container->duration)
        {
            container->killupdate = true;
        }
        else
        {
            // Sink vehicle approx 4 meters / 30 seconds
            SINT32 diff = frametime - lastframetime;
            container->lpVeh->PosData.Pos.z -= container->effectrate * diff;
        }
    }
}

//////////////////////////////////////////////////////////////////////
// Function: vehfxUpdate
// Description:
//////////////////////////////////////////////////////////////////////
void vehfxUpdate()
{
    if(_updates.empty())
    {
        return;
    }

    std::set<SINT32>::iterator itr = _updates.begin();
    std::set<SINT32>::iterator free;

    SINT32 id = 0;

    while(itr != _updates.end())
    {
        id = *itr;

        _updatecontainer(id);

        if(g_lpCon[id - 1].killupdate)
        {
            free = itr;
            ++itr;

            _updates.erase(free);
            vehKillVehicle(g_lpCon[id - 1].lpVeh);

            continue;
        }

        ++itr;
    }
}
