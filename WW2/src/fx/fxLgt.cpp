#include <syscfg.h> // Path is project specific.
/////////////////////////////////////////////////
// Module: fxLgt.cpp
// Description: Handle light effects.
/////////////////////////////////////////////////
#include "fx/fxLgt.h"
#include "fx/fxUtl.h"

#include "math/math3d.h"
#include "math/mathvect.h"
#include "include/vars.h"
#include "ob/obdlite.h"
#include "ld/ldpix.h"
#include "math/mathnum.h"
#include "mem/mem.h"

#define DEF_FXLGT_MAX_LIGHTS    16

#define DEF_BOMB_NAME  "fxbomb"
#define DEF_MUZZLE_NAME "fxmuzzle"

static FXLIGHTS* g_lpLgts = nullptr;

static FXLIGHTS g_lgtFreeHead;
static FXLIGHTS g_lgtFreeTail;
static FXLIGHTS g_lgtHead;
static FXLIGHTS g_lgtTail;

//static FLOAT32 g_fPi = PI;
static FLOAT32 g_fMaxLightRange = 0.0f;

static LIGHT* g_lpLight = nullptr;
static LIGHT* g_lpMuzzle = nullptr;

static piMATRIX* g_lpInvertView = nullptr;

static LOGICAL _CheckForLight();
static LOGICAL _DrawBombLight(FXLIGHTS* lpLgt);
static LOGICAL _DrawMuzzleLight(FXLIGHTS* lpLgt);
static LOGICAL _FreeLight(FXLIGHTS* lpLgt);
static LOGICAL _FreeLightIfNecessary(FXLIGHTS* lpLgt);
static LOGICAL _InitLights();
static LOGICAL _ReleaseLight(FXLIGHTS* lpLgt);

static void _InterpolateAttenuation(FXLIGHTS* lpLgt);
static void _SetLightProperties(FXLIGHTS* lpLgt);
static void _UpdateLight(FXLIGHTS* lpLgt);

/////////////////////////////////////////////////
// Function: fxLgtInit
// Description:
/////////////////////////////////////////////////
LOGICAL fxLgtInit()
{
    SINT32 i;

    g_lpLgts = (FXLIGHTS*)memAlloc(sizeof(FXLIGHTS) * DEF_FXLGT_MAX_LIGHTS);

    if(g_lpLgts == nullptr)
        return FALSE;

    memset(&g_lgtHead, 0, sizeof(FXLIGHTS));
    memset(&g_lgtTail, 0, sizeof(FXLIGHTS));

    g_lgtHead.next = &g_lgtTail;
    g_lgtTail.prev = &g_lgtHead;

    memset(&g_lgtFreeHead, 0, sizeof(FXLIGHTS));
    memset(&g_lgtFreeTail, 0, sizeof(FXLIGHTS));

    g_lgtFreeHead.next = &g_lgtFreeTail;
    g_lgtFreeTail.prev = &g_lgtFreeHead;

    for(i = 0; i < DEF_FXLGT_MAX_LIGHTS; i++)
    {
        g_lpLgts[i].iIndex = i;
        _FreeLight(&g_lpLgts[i]);
    }

    if(!_InitLights())
        return FALSE;

    g_fMaxLightRange = (FLOAT32)sqrt(FLT_MAX);

    return TRUE;
}

//////////////////////////////////////////
// Function: fxLgtKill
// Description:
//////////////////////////////////////////
void fxLgtKill()
{
    if(g_lpLgts != nullptr)
    {
        memFree(g_lpLgts);
        g_lpLgts = nullptr;
    }

    // Set to nullptr in case of further updates.
    memset(&g_lgtHead, 0, sizeof(FXLIGHTS));
    memset(&g_lgtTail, 0, sizeof(FXLIGHTS));
    memset(&g_lgtFreeHead, 0, sizeof(FXLIGHTS));
    memset(&g_lgtFreeTail, 0, sizeof(FXLIGHTS));
}

//////////////////////////////////////////
// Function: _InitLights
// Description:
//////////////////////////////////////////
LOGICAL _InitLights()
{
    LIGHT initBombLight;
    LIGHT initMuzzleLight;

    memset(&initBombLight, 0, sizeof(LIGHT));

    LOGICAL bReturn = TRUE;

    initBombLight.light.dltType = piLIGHT_POINT;
    initBombLight.light.dcvColor.r = 1.0f;
	initBombLight.light.dcvColor.g = 1.0f;
	initBombLight.light.dcvColor.b = 1.0f;
	initBombLight.light.dcvAmbient.r = 1.0f;
	initBombLight.light.dcvAmbient.g = 1.0f;
	initBombLight.light.dcvAmbient.b = 1.0f;
    initBombLight.light.dvRange = 100.0f;
    initBombLight.light.dvFalloff = 1.0f;

	strcpy(initBombLight.name, DEF_BOMB_NAME);

    if(!obdliteAddDynaLite(&initBombLight, fxLgtUpdateCallback))
        return FALSE;

    g_lpLight = obdliteFindDynaLight(DEF_BOMB_NAME);

    initMuzzleLight = initBombLight;

    // Set light initially to 'OFF'
    obdliteSetDynaState(g_lpLight, FALSE);

    strcpy(initMuzzleLight.name, DEF_MUZZLE_NAME);

    if(!obdliteAddDynaLite(&initMuzzleLight, fxLgtUpdateCallback))
        return FALSE;

    g_lpMuzzle = obdliteFindDynaLight(DEF_MUZZLE_NAME);

    // Set light initially to 'OFF'
    obdliteSetDynaState(g_lpMuzzle, FALSE);

    return bReturn;
}

//////////////////////////////////////////
// Function: fxLgtCreateMuzzleLight
// Description:
//////////////////////////////////////////
LOGICAL fxLgtCreateMuzzleLight(FLOAT64VECTOR* pPos, piVECTOR* dir)
{
    SINT32 iID = fxLgtGetNewLight();

    if(iID == -1)
        return FALSE;

    fxLgtSetType(iID, IDE_LIGHT_MUZZLEFLASH);
    fxLgtPushFlags(iID, DEF_FXFLAG_INTATTENUATION);
    fxLgtSetPosition(iID, pPos->x, pPos->y, pPos->z);

    if(g_wantNormalMaps) // Ticket# 5065
    {
        // ... for debugging values
        static float p1 = 5.0f;
        static float p2 = 5.0f;
        static float p3 = 5.0f;
        static float p4 = 5.0f;

        fxLgtSet4PtAttenuation(iID, p1, p2, p3, p4);
    }
    else
        fxLgtSet4PtAttenuation(iID, 0.6f, 0.6f, 0.8f, 10.0f);

    fxLgtSetPt1Time(iID, (FLOAT32)frametime);
    fxLgtSetPt2Time(iID, (FLOAT32)frametime + 50.0f);
    fxLgtSetPt3Time(iID, (FLOAT32)frametime + 100.0f);
    fxLgtSetPt4Time(iID, (FLOAT32)frametime + 150.0f);
    fxLgtSetPt1Color(iID, 1.0f, 1.0f, 1.0f);
    fxLgtSet4PtAlpha(iID, 1.0f, 1.0f, 1.0f, 1.0f);
    fxLgtSetDirection(iID, dir);

    // Required after settings.
    fxLgtInitLight(iID);

    return TRUE;
}

//////////////////////////////////////////
// Function: fxLgtUpdate
// Description:
//////////////////////////////////////////
void fxLgtUpdate()
{
    if(g_lgtHead.next == nullptr) return;

    if(g_lgtHead.next == &g_lgtTail)
    {
        if(g_lpLight->flags & LITE_ON)
        {
            obdliteSetDynaState(g_lpLight, FALSE);
        }

        if(g_lpMuzzle->flags & LITE_ON)
        {
            obdliteSetDynaState(g_lpMuzzle, FALSE);
        }

        return;
    }

    FXLIGHTS* lpLgt = g_lgtHead.next;
    FXLIGHTS* lpTail = &g_lgtTail;

    for(; lpLgt != lpTail; lpLgt = lpLgt->next)
    {
        switch(lpLgt->enumType)
        {
        case IDE_LIGHT_MUZZLEFLASH:
            {
                _UpdateLight(lpLgt);
                break;
            }
        case IDE_LIGHT_BOMB:
            {
                _UpdateLight(lpLgt);
                break;
            }
        default:
            // don't care about other light types
            break;
        }
    }
}

//////////////////////////////////////////
// Function: _UpdateLight
// Description:
//////////////////////////////////////////
void _UpdateLight(FXLIGHTS* lpLgt)
{
    if(!(lpLgt->uFlags & DEF_FXFLAG_INFINITE))
    {
        if((UINT32)frametime > lpLgt->lgtTime.p4)
            lpLgt->bDraw = FALSE;
    }

    fxUtlUpdatePosition(&lpLgt->vecPt, &lpLgt->vecPos);
    lpLgt->lgtLight.dvPosition = lpLgt->vecPt;

    lpLgt->fDistanceSqrd = fxUtlGetDistanceSqrd(&lpLgt->vecPt);

    if(lpLgt->uFlags & DEF_FXFLAG_INTATTENUATION)
    {
        _InterpolateAttenuation(lpLgt);
    }

    if(lpLgt->uFlags & DEF_FXFLAG_INTALPHA)
    {
        fxUtlInterpolateAlpha(&lpLgt->lgtTime, &lpLgt->lgtColor, &lpLgt->piColor);
    }

    _SetLightProperties(lpLgt);
}

//////////////////////////////////////////
// Function: _SetLightProperties
// Description:
//////////////////////////////////////////
void _SetLightProperties(FXLIGHTS* lpLgt)
{
    lpLgt->lgtLight.dltType = piLIGHT_POINT;

    lpLgt->lgtLight.dcvColor.r = lpLgt->piColor.r;
	lpLgt->lgtLight.dcvColor.g = lpLgt->piColor.g;
	lpLgt->lgtLight.dcvColor.b = lpLgt->piColor.b;
	lpLgt->lgtLight.dcvColor.a = lpLgt->piColor.a;

	lpLgt->lgtLight.dcvAmbient.r = lpLgt->piColor.r;
	lpLgt->lgtLight.dcvAmbient.g = lpLgt->piColor.g;
	lpLgt->lgtLight.dcvAmbient.b = lpLgt->piColor.b;
	lpLgt->lgtLight.dcvAmbient.a = lpLgt->piColor.a;

    lpLgt->lgtLight.dvAttenuation1 = lpLgt->fAttenuation;
}

//////////////////////////////////////////
// Function: fxLgtSetLights
// Description:
//////////////////////////////////////////
void fxLgtSetLights()
{
    // Set default
    obdliteSetDynaState(g_lpLight, FALSE);
    obdliteSetDynaState(g_lpMuzzle, FALSE);

    FXLIGHTS* light = g_lgtHead.next;

    for(; light != &g_lgtTail; light = light->next)
    {
        if(light->enumType == IDE_LIGHT_BOMB)
            _DrawBombLight(light);
        else if(light->enumType == IDE_LIGHT_MUZZLEFLASH)
            _DrawMuzzleLight(light);
    }

    light = g_lgtHead.next;
    FXLIGHTS* free;

    while(light != &g_lgtTail)
    {
        free = light;
        light = light->next;

        _FreeLightIfNecessary(free);
    }
}

//////////////////////////////////////////
// Function: fxLgtDraw
// Description:
//////////////////////////////////////////
LOGICAL fxLgtDraw(SINT32 iIndex)
{
    if(g_lgtHead.next == nullptr)
        return FALSE;

    if(g_lgtHead.next == &g_lgtTail)
    {
        obdliteSetDynaState(g_lpLight, FALSE);
        obdliteSetDynaState(g_lpMuzzle, FALSE);

        return TRUE;
    }

    FXLIGHTS* lpLgt = &g_lpLgts[iIndex];

    switch(lpLgt->enumType)
    {
    case IDE_LIGHT_BOMB:
        {
            _DrawBombLight(lpLgt);
            break;
        }
    case IDE_LIGHT_MUZZLEFLASH:
        {
            _DrawMuzzleLight(lpLgt);
            break;
        }
    default:
        // don't care about other light types
            break;
    }

    _FreeLightIfNecessary(lpLgt);

    return TRUE;
}

//////////////////////////////////////////
// Function: _DrawBombLight
// Description:
//////////////////////////////////////////
LOGICAL _DrawBombLight(FXLIGHTS* lpLgt)
{
    memcpy(&g_lpLight->light, &lpLgt->lgtLight, sizeof(piLIGHT));

    obdliteSetDynaLite(g_lpLight);
    obdliteSetDynaState(g_lpLight, TRUE);

    return TRUE;
}

//////////////////////////////////////////
// Function: _DrawMuzzleLight
// Description:
//////////////////////////////////////////
LOGICAL _DrawMuzzleLight(FXLIGHTS* lpLgt)
{
    memcpy(&g_lpMuzzle->light, &lpLgt->lgtLight, sizeof(piLIGHT));

    obdliteSetDynaLite(g_lpMuzzle);
    obdliteSetDynaState(g_lpMuzzle, TRUE);

    return TRUE;
}

//////////////////////////////////////////
// Function: _FreeLightIfNecessary
// Description:
//////////////////////////////////////////
LOGICAL _FreeLightIfNecessary(FXLIGHTS* lpLgt)
{
    if(lpLgt != nullptr)
    {
        switch(lpLgt->enumType)
        {
        case IDE_LIGHT_BOMB:
            {
                if(!lpLgt->bDraw)
                {
                    _ReleaseLight(lpLgt);
                    _FreeLight(lpLgt);
                }

                break;
            }
        case IDE_LIGHT_MUZZLEFLASH:
            {
                if(!lpLgt->bDraw)
                {
                    _ReleaseLight(lpLgt);
                    _FreeLight(lpLgt);
                }

                break;
            }
        default:
            // don't care about other light types
            break;
        }
    }

    return TRUE;
}

//////////////////////////////////////////
// Function: _FreeLight
// Description:
//////////////////////////////////////////
LOGICAL _FreeLight(FXLIGHTS* lpLgt)
{
    FXLIGHTS* lpPrev = nullptr;

    if(lpLgt == nullptr)
        return FALSE;

    lpPrev = g_lgtFreeTail.prev;

    lpLgt->prev = lpPrev;
    lpLgt->next = &g_lgtFreeTail;

    lpPrev->next = lpLgt;
    g_lgtFreeTail.prev = lpLgt;

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _ReleaseLight
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL _ReleaseLight(FXLIGHTS* lpLgt)
{
    FXLIGHTS* lpNext = nullptr;
    FXLIGHTS* lpPrev = nullptr;

    if(lpLgt == nullptr)
        return FALSE;

    lpNext = lpLgt->next;
    lpPrev = lpLgt->prev;

    if(lpNext == nullptr || lpPrev == nullptr)
        return FALSE;

    lpPrev->next = lpNext;
    lpNext->prev = lpPrev;

    lpLgt->prev = nullptr;
    lpLgt->next = nullptr;

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _CheckForLight
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL _CheckForLight()
{
    if(g_lgtFreeHead.next == &g_lgtFreeTail)
    {
        FXLIGHTS* lpLgt = nullptr;

        if(g_lgtTail.prev == &g_lgtHead)
            return FALSE;

        lpLgt = g_lgtTail.prev;

        if(!_ReleaseLight(lpLgt))
            return FALSE;

        if(!_FreeLight(lpLgt))
            return FALSE;
    }

    if(g_lgtFreeHead.next == &g_lgtFreeTail)
        return FALSE;

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: fxLgtAddLight
// Description: Add to head of list.
//////////////////////////////////////////////////////////////////////
LOGICAL fxLgtAddLight(FXLIGHTS* lpLgt)
{
    FXLIGHTS* lpNext = nullptr;

    if(lpLgt == nullptr)
        return FALSE;

    lpNext = g_lgtHead.next;

    g_lgtHead.next = lpLgt;
    lpLgt->prev = &g_lgtHead;

    lpLgt->next = lpNext;
    lpNext->prev = lpLgt;

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: fxLgtGetLight
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL fxLgtGetLight(FXLIGHTS** lpLgt)
{
    FXLIGHTS* lpNode = nullptr;
    FXLIGHTS* lpNext = nullptr;
    FXLIGHTS* lpPrev = nullptr;

    if(lpLgt == nullptr)
        return FALSE;

    if(!_CheckForLight())
        return FALSE;

    lpNode = g_lgtFreeHead.next;

    lpPrev = lpNode->prev;
    lpNext = lpNode->next;

    lpPrev->next = lpNext;
    lpNext->prev = lpPrev;

    lpNode->next = nullptr;
    lpNode->prev = nullptr;

    *lpLgt = lpNode;

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: fxLgtUpdateCallback
// Desctiption: Update the most recent light.
//////////////////////////////////////////////////////////////////////
LOGICAL fxLgtUpdateCallback(LIGHT* lpLight)
{
    return TRUE;
}

//////////////////////////////////////////
// Function: fxLgtGetList
// Desctiption:
//////////////////////////////////////////
void fxLgtGetList(void* lpLgtSort, SINT32* pIndex)
{
    FXSORT* lpSort = (FXSORT*)lpLgtSort;

    if(!lpSort || !pIndex) return;
    if(g_lgtHead.next == nullptr) return;
    if(g_lgtHead.next == &g_lgtTail) return;

    FXLIGHTS* lpLgt = g_lgtHead.next;
    FXLIGHTS* lpTail = &g_lgtTail;

    for(; lpLgt != lpTail; lpLgt = lpLgt->next)
    {
        if(*pIndex >= DEF_MAX_EFFECTS)
            break;

        lpSort[*pIndex].type = IDE_FXTYPE_LIGHT;
        lpSort[*pIndex].dist2 = lpLgt->fDistanceSqrd;
        lpSort[*pIndex].index = lpLgt->iIndex;

        (*pIndex)++;
    }
}

//////////////////////////////////////////
// Function: fxLgtSetInvertView
// Desctiption:
//////////////////////////////////////////
void fxLgtSetInvertView(piMATRIX* lpView)
{
    g_lpInvertView = lpView;
}

//////////////////////////////////////////
// Function: _InterpolateAttenuation
// Desctiption:
//////////////////////////////////////////
void _InterpolateAttenuation(FXLIGHTS* lpLgt)
{
    FLOAT32 t = 0.0f;
    FLOAT32 fAtt = 0.0f;

    INTFLOAT32* lpTime = &lpLgt->lgtTime;
    INTFLOAT32* lpAtt = &lpLgt->lgtAttn;

    if((FLOAT32)frametime < lpTime->p2)
    {
        FLOAT32 fNow = (FLOAT32)frametime;
        FLOAT32 fEnd = lpTime->p2;
        FLOAT32 fStart = lpTime->p1;

        t = (fNow - fStart) / (fEnd - fStart);

        fAtt = lpAtt->p2 - lpAtt->p1;
        lpLgt->fAttenuation = fAtt * t + lpAtt->p1;
    }
    else if((FLOAT32)frametime < lpTime->p3)
    {
        FLOAT32 fNow = (FLOAT32)frametime;
        FLOAT32 fEnd = (FLOAT32)lpTime->p3;
        FLOAT32 fStart = (FLOAT32)lpTime->p2;

        t = (fNow - fStart) / (fEnd - fStart);

        fAtt = lpAtt->p3 - lpAtt->p2;
        lpLgt->fAttenuation = fAtt * t + lpAtt->p2;
    }
    else if((FLOAT32)frametime < lpTime->p4)
    {
        FLOAT32 fNow = (FLOAT32)frametime;
        FLOAT32 fEnd = (FLOAT32)lpTime->p4;
        FLOAT32 fStart = (FLOAT32)lpTime->p3;

        t = (fNow - fStart) / (fEnd - fStart);

        fAtt = lpAtt->p4 - lpAtt->p3;
        lpLgt->fAttenuation = fAtt * t + lpAtt->p3;
    }
    else
    {
        lpLgt->fAttenuation = lpAtt->p4;
    }
}

////////////////////////////////////////////////////
// Function: fxLgtSet4PtAttenuation
// Description:
////////////////////////////////////////////////////
void fxLgtSet4PtAttenuation(SINT32 iID, FLOAT32 p1, FLOAT32 p2, FLOAT32 p3, FLOAT32 p4)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_FXLGT_MAX_LIGHTS)
        return;

    g_lpLgts[iIndex].lgtAttn.p1 = p1;
    g_lpLgts[iIndex].lgtAttn.p2 = p2;
    g_lpLgts[iIndex].lgtAttn.p3 = p3;
    g_lpLgts[iIndex].lgtAttn.p4 = p4;

    g_lpLgts[iIndex].fAttenuation = p1;
}

////////////////////////////////////////////////////
// Function: fxLgtPopFlags
// Description:
////////////////////////////////////////////////////
void fxLgtPopFlags(SINT32 iID, UINT32 uFlags)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_FXLGT_MAX_LIGHTS)
        return;

    g_lpLgts[iIndex].uFlags &= ~uFlags;
}

////////////////////////////////////////////////////
// Function: fxLgtPushFlags
// Description:
////////////////////////////////////////////////////
void fxLgtPushFlags(SINT32 iID, UINT32 uFlags)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_FXLGT_MAX_LIGHTS)
        return;

    g_lpLgts[iIndex].uFlags |= uFlags;
}

////////////////////////////////////////////////////
// Function: fxLgtSetDirection
// Description:
////////////////////////////////////////////////////
void fxLgtSetDirection(SINT32 ID, piVECTOR* dir)
{
    SINT32 i = ID - 1;

    if(i < 0 || i >= DEF_FXLGT_MAX_LIGHTS)
        return;

    g_lpLgts[i].direction.x = dir->x;
    g_lpLgts[i].direction.y = dir->y;
    g_lpLgts[i].direction.z = dir->z;
}

////////////////////////////////////////////////////
// Function: fxLgtSetPosition
// Description:
////////////////////////////////////////////////////
void fxLgtSetPosition(SINT32 iID, FLOAT64 x, FLOAT64 y, FLOAT64 z)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_FXLGT_MAX_LIGHTS)
        return;

    g_lpLgts[iIndex].vecPos.x = x;
    g_lpLgts[iIndex].vecPos.y = y;
    g_lpLgts[iIndex].vecPos.z = z;
}

////////////////////////////////////////////////////
// Function: fxLgtSet4PtAlpha
// Description:
////////////////////////////////////////////////////
void fxLgtSet4PtAlpha(SINT32 iID, FLOAT32 p1, FLOAT32 p2, FLOAT32 p3, FLOAT32 p4)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_FXLGT_MAX_LIGHTS)
        return;

    g_lpLgts[iIndex].lgtColor.p1.a = p1;
    g_lpLgts[iIndex].lgtColor.p2.a = p2;
    g_lpLgts[iIndex].lgtColor.p3.a = p3;
    g_lpLgts[iIndex].lgtColor.p4.a = p4;

    g_lpLgts[iIndex].piColor.a = p1;
}

////////////////////////////////////////////////////
// Function: fxLgtSetPt1Color
// Description:
////////////////////////////////////////////////////
void fxLgtSetPt1Color(SINT32 iID, FLOAT32 r, FLOAT32 g, FLOAT32 b)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_FXLGT_MAX_LIGHTS)
        return;

    g_lpLgts[iIndex].lgtColor.p1.r = r;
    g_lpLgts[iIndex].lgtColor.p1.g = g;
    g_lpLgts[iIndex].lgtColor.p1.b = b;

    g_lpLgts[iIndex].piColor.r = r;
    g_lpLgts[iIndex].piColor.g = g;
    g_lpLgts[iIndex].piColor.b = b;
}

////////////////////////////////////////////////////
// Function: fxLgtSetPt2Color
// Description:
////////////////////////////////////////////////////
void fxLgtSetPt2Color(SINT32 iID, FLOAT32 r, FLOAT32 g, FLOAT32 b)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_FXLGT_MAX_LIGHTS)
        return;

    g_lpLgts[iIndex].lgtColor.p2.r = r;
    g_lpLgts[iIndex].lgtColor.p2.g = g;
    g_lpLgts[iIndex].lgtColor.p2.b = b;
}

////////////////////////////////////////////////////
// Function: fxLgtSetPt3Color
// Description:
////////////////////////////////////////////////////
void fxLgtSetPt3Color(SINT32 iID, FLOAT32 r, FLOAT32 g, FLOAT32 b)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_FXLGT_MAX_LIGHTS)
        return;

    g_lpLgts[iIndex].lgtColor.p3.r = r;
    g_lpLgts[iIndex].lgtColor.p3.g = g;
    g_lpLgts[iIndex].lgtColor.p3.b = b;
}

////////////////////////////////////////////////////
// Function: fxLgtSetPt4Color
// Description:
////////////////////////////////////////////////////
void fxLgtSetPt4Color(SINT32 iID, FLOAT32 r, FLOAT32 g, FLOAT32 b)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_FXLGT_MAX_LIGHTS)
        return;

    g_lpLgts[iIndex].lgtColor.p4.r = r;
    g_lpLgts[iIndex].lgtColor.p4.g = g;
    g_lpLgts[iIndex].lgtColor.p4.b = b;
}

////////////////////////////////////////////////////
// Function: fxLgtSetPt1Time
// Description:
////////////////////////////////////////////////////
void fxLgtSetPt1Time(SINT32 iID, FLOAT32 fTime)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_FXLGT_MAX_LIGHTS)
        return;

    g_lpLgts[iIndex].lgtTime.p1 = fTime;
}

////////////////////////////////////////////////////
// Function: fxLgtSetPt2Time
// Description:
////////////////////////////////////////////////////
void fxLgtSetPt2Time(SINT32 iID, FLOAT32 fTime)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_FXLGT_MAX_LIGHTS)
        return;

    g_lpLgts[iIndex].lgtTime.p2 = fTime;
}

////////////////////////////////////////////////////
// Function: fxLgtSetPt3Time
// Description:
////////////////////////////////////////////////////
void fxLgtSetPt3Time(SINT32 iID, FLOAT32 fTime)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_FXLGT_MAX_LIGHTS)
        return;

    g_lpLgts[iIndex].lgtTime.p3 = fTime;
}

////////////////////////////////////////////////////
// Function: fxLgtSetPt4Time
// Description:
////////////////////////////////////////////////////
void fxLgtSetPt4Time(SINT32 iID, FLOAT32 fTime)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_FXLGT_MAX_LIGHTS)
        return;

    g_lpLgts[iIndex].lgtTime.p4 = fTime;
}

////////////////////////////////////////////////////
// Function: fxLgtSetType
// Description:
////////////////////////////////////////////////////
void fxLgtSetType(SINT32 iID, LIGHTTYPE enumType)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_FXLGT_MAX_LIGHTS)
        return;

    g_lpLgts[iIndex].enumType = enumType;
}

////////////////////////////////////////////////////
// Function: fxLgtInitLight
// Description:
////////////////////////////////////////////////////
void fxLgtInitLight(SINT32 iID)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_FXLGT_MAX_LIGHTS)
        return;

    fxUtlUpdatePosition(&g_lpLgts[iIndex].vecPt, &g_lpLgts[iIndex].vecPos);

    g_lpLgts[iIndex].lgtLight.dvRange = g_fMaxLightRange;
    g_lpLgts[iIndex].lgtLight.dvPosition = g_lpLgts[iIndex].vecPt;
    g_lpLgts[iIndex].lgtLight.dvDirection = g_lpLgts[iIndex].direction;

    g_lpLgts[iIndex].bDraw = TRUE;
}

////////////////////////////////////////////////////
// Function: fxLgtGetNewLight
// Description:
////////////////////////////////////////////////////
SINT32 fxLgtGetNewLight()
{
    FXLIGHTS* lpLgt = nullptr;

    if(!fxLgtGetLight(&lpLgt))
        return -1;

    if(!fxLgtAddLight(lpLgt))
        return -1;

    lpLgt->lgtLight.dvFalloff = 1.0f;

    return lpLgt->iIndex + 1;
}
