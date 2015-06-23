#include <syscfg.h> // Path is project specific.
////////////////////////////////////////////////
// Module: fxMgr.cpp
// Description: Top level module for FX.
////////////////////////////////////////////////
#include "fx/fxMgr.h"
#include "fx/fxAmb.h"
#include "fx/fxLgt.h"
#include "fx/fxPcl.h"
#include "fx/fxHit.h"
#include "fx/fxExp.h"
#include "fx/fxBlt.h"
#include "fx/fxSnd.h"
#include "fx/fxUtl.h"

#include "math/math3d.h"
#include "math/mathnum.h"
#include "include/vars.h"
#include "ob/obdlite.h"
#include "mem/mem.h"
#include "trn/trncloud.h"
#include "trn/trnclutter.h"
#include "trn/trnmain.h"
#include "pers/persmain.h"
#include "veh/vehmain.h"
#include "wwii/wwiiTypes.h"
#include "trn/trnRadClutter.h"
#include "fx/fxMisc.h"

#define DEF_MAX_RED 180.0f * colorONEOVER255
#define DEF_MAX_BLU 180.0f * colorONEOVER255
#define DEF_MAX_GRN 180.0f * colorONEOVER255

#define DEF_MIN_RED 70.0f * colorONEOVER255
#define DEF_MIN_BLU 70.0f * colorONEOVER255
#define DEF_MIN_GRN 70.0f * colorONEOVER255

static piMATRIX g_matView;

static piVECTOR g_vecWind;

static FXSORT* g_lpSort = nullptr;

static SINT32 g_iMatID = -1;
static SINT32 g_iSortCount = 0;
static SINT32 _SortCompare(const void* pObj1, const void* pObj2);

struct FXDRAWFUNC
{
    LOGICAL (*pFn)(SINT32 iIndex);

};

static FXDRAWFUNC g_pfnDrawFunc[NUMBER_OF_FXTYPES];

#include "fx2/effect.h"

/////////////////////////////////////////////////
// Function: fxMgrInit
// Description: Top level initialization for FX.
/////////////////////////////////////////////////
LOGICAL fxMgrInit()
{
    //fxinit();

    if(!fxUtlInit()) return FALSE;

    fxMiscInit();

    if(bSoundEnable)
    {
        if(!fxSndInit())
            return FALSE;

        if(!fxAmbInit())
            return FALSE;
    }

    if(!fxBltInit()) return FALSE;
    if(!fxExpInit()) return FALSE;
    //if(!fxWtrInit()) return FALSE;
    if(!fxLgtInit()) return FALSE;
    if(!fxHitInit()) return FALSE;
    if(!fxPclInit()) return FALSE;

    //if(!fxQuadInit()) return FALSE;

    fxMgrSetInvertView();
    fxMgrSetLight();

    fxMgrCreateMaterial();

    g_lpSort = (FXSORT*)memAlloc(sizeof(FXSORT) * DEF_MAX_EFFECTS);

    if(g_lpSort == nullptr)
        return FALSE;

//	Uneccessary -- memAlloc = calloc
//    memset(g_lpSort, 0, sizeof(FXSORT) * DEF_MAX_EFFECTS);
    memset(g_pfnDrawFunc, 0, sizeof(FXDRAWFUNC) * NUMBER_OF_FXTYPES);

    //g_pfnDrawFunc[IDE_FXTYPE_WATER].pFn = fxWtrDraw;
    g_pfnDrawFunc[IDE_FXTYPE_EXPLOSION].pFn = fxExpDraw;
    g_pfnDrawFunc[IDE_FXTYPE_LIGHT].pFn = fxLgtDraw;
    g_pfnDrawFunc[IDE_FXTYPE_PARTICLE].pFn = fxPclDraw;

    fxMgrSetWind(0.0f, 0.0f, 0.0f);

    return TRUE;
}

////////////////////////////////////////////////
// Function: fxMgrCreateMaterial
// Description:
///////////////////////////////////////////////
LOGICAL fxMgrCreateMaterial()
{
    piMATERIAL matMaterial;

    memset(&matMaterial, 0, sizeof(piMATERIAL));

    matMaterial.diffuse.r = 1.0f;
    matMaterial.diffuse.g = 1.0f;
    matMaterial.diffuse.b = 1.0f;
    matMaterial.diffuse.a = 1.0f;

    matMaterial.ambient.r = 1.0f;
    matMaterial.ambient.g = 1.0f;
    matMaterial.ambient.b = 1.0f;
    matMaterial.ambient.a = 1.0f;

    matMaterial.specular.r = 0.0f;
    matMaterial.specular.g = 0.0f;
    matMaterial.specular.b = 0.0f;
    matMaterial.specular.a = 0.0f;

    matMaterial.emissive.r = 0.1f;
    matMaterial.emissive.g = 0.1f;
    matMaterial.emissive.b = 0.1f;
    matMaterial.emissive.a = 0.1f;

    matMaterial.power = 1.0f;

    g_iMatID = piCreateMaterial(&matMaterial);

    if(!piIsMaterial(g_iMatID))
        return FALSE;

    return TRUE;
}

/////////////////////////////////////////////////
// Function: fxMgrGetMaterial
// Description:
/////////////////////////////////////////////////
SINT32 fxMgrGetMaterial()
{
    return g_iMatID;
}

/////////////////////////////////////////////////
// Function: fxMgrFreeAll
// Description:
/////////////////////////////////////////////////
void fxMgrFreeAll()
{
    // Ensure all sounds are dead before returning to UI
    if(bSoundEnable)
    {
        fxSndKill();
        fxAmbStop(TRUE);
    }

    fxMiscResetAll();

    // Clear to avoid recurrence of concussion effect during spawn in.
    fxExpFreeAll();
    fxHitFreeAll();
    fxBltFreeAll();
    fxPclFreeAll();
    //fxQuadFreeAll();
}

#if USE_STATESET
/////////////////////////////////////////////////
// Function: fxMgrDraw
// Description: Top level draw for FX.
/////////////////////////////////////////////////
LOGICAL fxMgrDraw(LOGICAL beforesorted)
{
    fxMgrSetLight();

    if(g_lpSort == nullptr)
        return FALSE;

    fxBltDraw();
    fxHitDraw();
/*
    for(SINT32 i = 0; i < g_iSortCount; i++)
    {
        if(g_pfnDrawFunc[g_lpSort[i].type].pFn != nullptr)
        {
            g_pfnDrawFunc[g_lpSort[i].type].pFn(g_lpSort[i].index);
        }
    }
    */
    return TRUE;
}
#else
/////////////////////////////////////////////////
// Function: fxMgrDraw
// Description: Top level draw for FX.
/////////////////////////////////////////////////
LOGICAL fxMgrDraw(LOGICAL beforesorted)
{
    fxMgrSetLight();

    if(g_lpSort == nullptr)
        return FALSE;

    fxUtlSetTexture();

    fxBltDraw();
    fxHitDraw();

    for(SINT32 i = 0; i < g_iSortCount; i++)
    {
        if(g_pfnDrawFunc[g_lpSort[i].type].pFn != nullptr)
        {
            g_pfnDrawFunc[g_lpSort[i].type].pFn(g_lpSort[i].index);
        }
    }

    //fxQuadDraw();

    return TRUE;
}
#endif

/////////////////////////////////////////////////
// Function: fxMgrUpdate
// Description: Top level update for FX.
/////////////////////////////////////////////////
LOGICAL fxMgrUpdate()
{
    //fxupdate();

    logTimerBeginMacro();

    //logTimerCallVoid(fxMgrSetInvertView);
	logTimerCallVoid(fxMgrUpdateSound);
   // logTimerCallVoid(fxWtrUpdate);
    logTimerCallVoid(fxBltUpdate);
    logTimerCallVoid(fxExpUpdate);
    logTimerCallVoid(fxLgtUpdate);
    logTimerCallVoid(fxHitUpdate);
    logTimerCallVoid(fxPclUpdate);
//    logTimerCallVoid(fxQuadUpdate);
    logTimerCallVoid(fxMgrSortFX);

    //fxMiscUpdate();

    logTimerEndMacro();

    return TRUE;
}

/////////////////////////////////////////////////
// Function: fxMgrUpdateSound
// Description:
/////////////////////////////////////////////////
void fxMgrUpdateSound()
{
    if(!bSoundEnable) return;

    UINT32 position = 0;
    persGetPersonaPosition(&position);

    // We are dead.
    if(persGetCrewIsDead(position))
    {
        fxSndStopSoundsForThisPosition(position);
        fxAmbStop(TRUE);
    }
    else
    {
        // Well, it turns out we could be dead, but not really! &*#&@*#!
        fxSndResetSoundsForThisPosition(position);
        fxAmbStop(FALSE);
    }

    fxSndUpdate();
    fxAmbUpdate();
}

/////////////////////////////////////////////////
// Function: fxMgrSortFX
// Description: Sort all effects for drawing.
/////////////////////////////////////////////////
void fxMgrSortFX()
{
    g_iSortCount = 0;

   // fxWtrGetList(g_lpSort, &g_iSortCount);
    fxExpGetList(g_lpSort, &g_iSortCount);
    fxLgtGetList(g_lpSort, &g_iSortCount);
    fxPclGetList(g_lpSort, &g_iSortCount);

    if(g_iSortCount > 0)
    {
        g_iSortCount = g_iSortCount > DEF_MAX_EFFECTS ? DEF_MAX_EFFECTS : g_iSortCount;

        qsort(g_lpSort, g_iSortCount, sizeof(FXSORT), _SortCompare);
    }
}

//////////////////////////////////////////
// Function: SortCompare
// Desctiption:
//////////////////////////////////////////
SINT32 _SortCompare(const void* o1, const void* o2)
{
    const FXSORT* const left = (FXSORT*)o1;
    const FXSORT* const right = (FXSORT*)o2;

    if(left->dist2 < right->dist2)
        return 1;

    if(left->dist2 > right->dist2)
        return -1;

    return 0;
}

/////////////////////////////////////////////////
// Function: fxMgrKill
// Description: Top level kill for FX.
/////////////////////////////////////////////////
LOGICAL fxMgrKill()
{
   // fxkill();

    if(bSoundEnable)
    {
        fxSndKill();
        fxAmbKill();
    }

    fxMiscKill();

    fxBltKill();
    fxExpKill();
   // fxWtrKill();
    fxLgtKill();
    fxHitKill();
    fxPclKill();
    fxUtlKill();

//    fxQuadKill();

    if(g_lpSort)
    {
        memFree(g_lpSort);
        g_lpSort = nullptr;
    }

    Write_Log(LOG_OLD, LOG_INFO, "[fxMgrKill]");

    return TRUE;
}

/////////////////////////////////////////////////
// Function: fxMgrDetach
// Description: Top level detach for FX.
/////////////////////////////////////////////////
LOGICAL fxMgrDetach(OBJECT* lpObj)
{
    if(lpObj == nullptr) return FALSE;

    if(!(lpObj->flag2 & OBJECT_HAS_FX))
        return FALSE;

    logTimerBeginMacro();

    if(bSoundEnable)
    {
        fxSndDetachSnd(lpObj);
    }

    fxBltDetachBlt(lpObj);
    fxHitDetachHit(lpObj);
    fxPclDetachObj(lpObj);

    lpObj->flag2 &= ~OBJECT_HAS_FX;

    logTimerEndMacro();

    return TRUE;
}

/////////////////////////////////////////////////
// Function: fxMgrDetachAmmo
// Description: Detach effect from ammo.
/////////////////////////////////////////////////
void fxMgrDetachAmmo(void const* pAmmo)
{
    if(pAmmo == nullptr) return;

    if(bSoundEnable)
    {
        fxSndDetachAmmo(pAmmo);
    }

    fxPclDetachAmmo(pAmmo);
}

//////////////////////////////////////////////////////////////////////
// Function: fxMgrSetLight
// Description: Set color for fx lighting.
//////////////////////////////////////////////////////////////////////
void fxMgrSetLight()
{
    static LIGHT* s = nullptr;
    static LIGHT* m = nullptr;

    if(!s)
    {
        s = obdliteFindDynaLight("sun");
    }

    if(!m)
    {
        m = obdliteFindDynaLight("moon");
    }

    if(!s || !m)
    {
        Write_Log(LOG_OLD, LOG_INFO, "ERROR: Cannot find sun/moon object");
        return;
    }

    static piCOLORVALUE color = {0, 0, 0, 1};

    piVECTOR dir;

    UINT32 ambient;

    FLOAT32 daylight;

    ambient = piGetAmbLight();

    dir.z = -1.0f;
    dir.x = 0.0f;
    dir.y = 0.0f;

    daylight = -math3dDotProduct(&dir, (s->flags & LITE_ON) ? &s->light.dvDirection : &m->light.dvDirection);

    if(daylight < 0.0f)
    {
        daylight = 0.0f;
    }

    piCOLORVALUE* cv = s->flags & LITE_ON ? &s->light.dcvColor : &m->light.dcvColor;

    color.r = cv->r * daylight;
    color.r += piRGBA_GETRED(ambient) * colorONEOVER255;

    color.g = cv->g * daylight;
    color.g += piRGBA_GETGREEN(ambient) * colorONEOVER255;

    color.b = cv->b * daylight;
    color.b += piRGBA_GETBLUE(ambient) * colorONEOVER255;

    color.a = 0.94118f; //Arbitrary Alpha 0xF0 = 240; 240 / 255 = 0.94;

    ////////////////////////////
    // Clip for some visiblity.
    ////////////////////////////
    color.r = color.r > DEF_MAX_RED ? DEF_MAX_RED : color.r;
    color.g = color.g > DEF_MAX_GRN ? DEF_MAX_GRN : color.g;
    color.b = color.b > DEF_MAX_BLU ? DEF_MAX_BLU : color.b;

    color.r = color.r < DEF_MIN_RED ? DEF_MIN_RED : color.r;
    color.g = color.g < DEF_MIN_GRN ? DEF_MIN_GRN : color.g;
    color.b = color.b < DEF_MIN_BLU ? DEF_MIN_BLU : color.b;

    fxBltSetColor(&color);
    fxExpSetColor(&color);
   // fxWtrSetColor(&color);
    fxHitSetColor(&color);
    fxPclSetColor(&color);
//    fxQuadSetColor(&color);

 //   fxsetcolor(color);
}

//////////////////////////////////////////////////////////////////////
// Function: fxMgrSetInvertView
// Description:
//////////////////////////////////////////////////////////////////////
void fxMgrSetInvertView()
{
    g_matView = *piGetViewMatrix();

    g_matView.m[3][0] = 0.0f;
    g_matView.m[3][1] = 0.0f;
    g_matView.m[3][2] = 0.0f;

    math3dInvertMatrix(&g_matView, &g_matView);

   // fxWtrSetInvertView(&g_matView);
    fxLgtSetInvertView(&g_matView);
    fxPclSetInvertView(&g_matView);
//    fxQuadSetInvertView(&g_matView);
}

//////////////////////////////////////////////////////////////////////
// Function: fxMgrSetWind
// Description: Set wind vector in meters/sec
//////////////////////////////////////////////////////////////////////
void fxMgrSetWind(FLOAT32 x, FLOAT32 y, FLOAT32 z)
{
    g_vecWind.x = x;
    g_vecWind.y = y;
    g_vecWind.z = z;
}

//////////////////////////////////////////////////////////////////////
// Function: fxMgrGetWind
// Description: Retrieve wind vector.
//////////////////////////////////////////////////////////////////////
piVECTOR* fxMgrGetWind()
{
    return &g_vecWind;
}
