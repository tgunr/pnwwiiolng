#include <syscfg.h> // Path is project specific.
/////////////////////////////////////////////////////
// Module: fxHit.cpp
// Description: Handle all hit events.
/////////////////////////////////////////////////////
#include "fx/fxHit.h"
#include "fx/fxUtl.h"

#include "ml/mlutility.h"
#include "ld/ldpix.h"
#include "math/math3d.h"
#include "math/mathvect.h"
#include "math/matheul.h"
#include "trn/trnmain.h"
#include "rn/rnvis.h"
#include "mem/mem.h"
#include "pers/persmain.h"
#include "ogl/ogl.h"
#include "fx/DataFx.h"
#include "ogl/oglGlobalStates.h"

#include <vector>

#define DEF_MAX_NUM_OF_COLLISIONS   32
#define DEF_MAX_HITS                128

static FXHIT* g_lpHits = nullptr;

static FXHIT g_hitFreeHead;
static FXHIT g_hitFreeTail;

static FXHIT g_hitHead;
static FXHIT g_hitTail;

static piCOLORVALUE* g_lpColor = nullptr;

static std::multimap<void* , SINT32> g_mappedHits;
static std::vector<FXHIT*> drawlist;

//static FLOAT32 g_fPi = PI;
//static FLOAT32 g_fPOWEnergy = 0.0f;

static void _UnMap(void* pData);
static void _DrawDecals();

#if USE_STATESET
#include "ogl/States.h"

static ref_ptr<RenderStateSet> stateset;
#endif

////////////////////////////////////
// Function: fxHitInit
// Description: Initialize Hits.
////////////////////////////////////
LOGICAL fxHitInit()
{
    SINT32 i;

    g_lpHits = (FXHIT*)memAlloc(sizeof(FXHIT) * DEF_MAX_HITS);

    if(g_lpHits == nullptr)
        return FALSE;

//	Uneccessary -- memAlloc = calloc
//    memset(g_lpHits, 0, sizeof(FXHIT) * DEF_MAX_HITS);

    memset(&g_hitHead, 0, sizeof(FXHIT));
    memset(&g_hitTail, 0, sizeof(FXHIT));

    g_hitHead.next = &g_hitTail;
    g_hitTail.prev = &g_hitHead;

    memset(&g_hitFreeHead, 0, sizeof(FXHIT));
    memset(&g_hitFreeTail, 0, sizeof(FXHIT));

    g_hitFreeHead.next = &g_hitFreeTail;
    g_hitFreeTail.prev = &g_hitFreeHead;

    for(i = 0; i < DEF_MAX_HITS; i++)
    {
        g_lpHits[i].iIndex = i;
        fxHitFreeHit(&g_lpHits[i]);
    }

#if USE_STATESET
    stateset = new RenderStateSet;
    stateset->SetRenderState(new DepthMask(false));
    stateset->SetRenderState(new TextureUnit(0, FxGet().getTexture()));
    stateset->SetMode(GL_LIGHTING, 0);
    stateset->SetMode(GL_BLEND, 1);
#endif

    return TRUE;
}

//////////////////////////////////////////////
// Function: fxHitFreeAll
// Description: Release all hits
//////////////////////////////////////////////
void fxHitFreeAll()
{
    FXHIT* lpHit = g_hitHead.next;
    FXHIT* lpTail = &g_hitTail;
    FXHIT* lpFree = nullptr;

    if(lpHit == nullptr) return;

    while(lpHit != lpTail)
    {
        lpFree = lpHit;
        lpHit = lpHit->next;

        fxHitReleaseHit(lpFree);
        fxHitFreeHit(lpFree);
    }

    drawlist.clear();
}

//////////////////////////////////////////////
// Function: fxHitKill
// Description: Release all
//////////////////////////////////////////////
LOGICAL fxHitKill()
{
    if(g_lpHits != nullptr)
    {
        memFree(g_lpHits);
        g_lpHits = nullptr;
    }

    // Set to nullptr in case of further updates.
    memset(&g_hitHead, 0, sizeof(FXHIT));
    memset(&g_hitTail, 0, sizeof(FXHIT));
    memset(&g_hitFreeHead, 0, sizeof(FXHIT));
    memset(&g_hitFreeTail, 0, sizeof(FXHIT));

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: fxHitGetNewHit
// Description:
//////////////////////////////////////////////////////////////////////
SINT32 fxHitGetNewHit()
{
    FXHIT* lpHit = nullptr;

    if(!fxHitGetHit(&lpHit))
        return -1;

    if(!fxHitAddHit(lpHit))
        return -1;

    return lpHit->iIndex + 1;
}

//////////////////////////////////////////////////////////////////////
// Function: fxHitGetObjectForHit
// Description: Make sure we have the right object for the hitmark placement.
//////////////////////////////////////////////////////////////////////
LOGICAL fxHitGetObjectForHit(FXHIT* lpHit, OBJECT* lpObj)
{
    OBJECT* lpRetObj = fxHitGetObject(lpObj);

    lpHit->lpObj = lpRetObj;

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: fxHitGetObject
// Description: Parse through non-objects.
//////////////////////////////////////////////////////////////////////
OBJECT* fxHitGetObject(OBJECT* lpObj)
{
    if(lpObj == nullptr)
        return nullptr;

    if(!(lpObj->flags & OBJECT_NOTOBJECT))
    {
        return lpObj;
    }
    else
    {
        fxHitGetObject(lpObj->child);
    }

    return lpObj;
}

//////////////////////////////////////////////////////////////////////
// Function: fxHitCreateRotationMatrix
// Description: Create matrix for rotation of texture.
//////////////////////////////////////////////////////////////////////
LOGICAL fxHitCreateRotationMatrix(FXHIT* lpHit, piMATRIX* lpMat)
{
    piVECTOR u;
    piVECTOR vecAtt;

    u = lpHit->n;

    vecAtt.y = 0.0f;
    vecAtt.z = (FLOAT32)atan2(u.x, u.y);
    vecAtt.x = (FLOAT32)atan2(-sqrt(u.x * u.x + u.y * u.y), -u.z);

    mathEul_ToMatrix(lpMat, EulOrdXYZs, &vecAtt);

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: fxHitClip
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL fxHitClip(FXHIT* lpHit, piVECTOR* pvecShot)
{
    piVECTOR v1, v2, v3, v4, v;

    FASTCOLLIDE cldArray[DEF_MAX_NUM_OF_COLLISIONS + 2];

    piMATRIX matAtt = IdentityMatrix;

    FLOAT32 fDistance = 1.0f;

    UINT32 uNumCollisions = 0;

    fxHitCreateRotationMatrix(lpHit, &matAtt);

    FLOAT32 fRadius = 0.0f;

    fRadius = lpHit->fRadius * 2.0f;
    lpHit->fRadius = fRadius;

    v1.x = -fRadius; v1.y = -fRadius; v1.z = fDistance;
    v2.x = -fRadius; v2.y =  fRadius; v2.z = fDistance;
    v3.x =  fRadius; v3.y =  fRadius; v3.z = fDistance;
    v4.x =  fRadius; v4.y = -fRadius; v4.z = fDistance;

    ///////////////////////////////////
    // Random rotate here if necessary.
    ///////////////////////////////////

    v = v1; v.z = -v.z;

    fxHitSetCollisionVectors(&v1, &v1, &matAtt, lpHit);
    fxHitSetCollisionVectors(&v, &v, &matAtt, lpHit);

    if(!fxHitCollideObject(&v1, &v, lpHit, &uNumCollisions, cldArray))
    {
        return FALSE;
    }

    if(!fxHitHandleCollision(cldArray, uNumCollisions, lpHit, 0))
    {
        return FALSE;
    }

    v = v2; v.z = -v.z;

    fxHitSetCollisionVectors(&v2, &v2, &matAtt, lpHit);
    fxHitSetCollisionVectors(&v, &v, &matAtt, lpHit);

    if(!fxHitCollideObject(&v2, &v, lpHit, &uNumCollisions, cldArray))
    {
        return FALSE;
    }

    if(!fxHitHandleCollision(cldArray, uNumCollisions, lpHit, 1))
    {
        return FALSE;
    }

    v = v3; v.z = -v.z;

    fxHitSetCollisionVectors(&v3, &v3, &matAtt, lpHit);
    fxHitSetCollisionVectors(&v, &v, &matAtt, lpHit);

    if(!fxHitCollideObject(&v3, &v, lpHit, &uNumCollisions, cldArray))
    {
        return FALSE;
    }

    if(!fxHitHandleCollision(cldArray, uNumCollisions, lpHit, 2))
    {
        return FALSE;
    }

    v = v4; v.z = -v.z;

    fxHitSetCollisionVectors(&v4, &v4, &matAtt, lpHit);
    fxHitSetCollisionVectors(&v, &v, &matAtt, lpHit);

    if(!fxHitCollideObject(&v4, &v, lpHit, &uNumCollisions, cldArray))
    {
        return FALSE;
    }

    if(!fxHitHandleCollision(cldArray, uNumCollisions, lpHit, 3))
    {
        return FALSE;
    }

    // Necessary to do when we have vehicles.
    v1.x = v1.y = v1.z = 0.0f;
    v1.z = fDistance;
    v = v1; v.z = -v.z;

    fxHitSetCollisionVectors(&v1, &v1, &matAtt, lpHit);
    fxHitSetCollisionVectors(&v, &v, &matAtt, lpHit);

    if(!fxHitCollideObject(&v1, &v, lpHit, &uNumCollisions, cldArray))
    {
        return FALSE;
    }

    if(!fxHitHandleCollision(cldArray, uNumCollisions, lpHit, 4))
    {
        return FALSE;
    }

    for(SINT32 i = 0; i < 5; i++)
    {
        lpHit->hitPt[i].x += lpHit->n.x * 0.05f;
        lpHit->hitPt[i].y += lpHit->n.y * 0.05f;
        lpHit->hitPt[i].z += lpHit->n.z * 0.05f;
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: fxHitSetCollisionVectors
// Description: Set vectors for old and new vector positions.
//////////////////////////////////////////////////////////////////////
LOGICAL fxHitSetCollisionVectors(piVECTOR* pv1,
                                 piVECTOR* pv2,
                                 piMATRIX* lpMat,
                                 FXHIT* lpHit)
{
    math3dVectorMultiply(pv1, pv2, lpMat);
    mathAddVectors(pv1, pv2, &lpHit->vecPt);

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: fxHitCollideObject
// Description: Can't avoid this if we don't want the overlap
// of Explosion holes on windows, off edges, etc...
//////////////////////////////////////////////////////////////////////
LOGICAL fxHitCollideObject(piVECTOR* pV1,
                           piVECTOR* pV2,
                           FXHIT* lpHit,
                           UINT32* pNumCls,
                           FASTCOLLIDE cldArray[])
{
    FASTCOLLIDE* pList = nullptr;

    UINT32 uNumCollisions = DEF_MAX_NUM_OF_COLLISIONS;

    *pNumCls = 0;

    pList = cldArray;

    weapCollideWithHitmapObject(&pList,
                                &uNumCollisions,
                                lpHit->lpObj,
                                lpHit->lpObj,
                                pV2,
                                pV1,
                                0,
                                1);

    uNumCollisions = DEF_MAX_NUM_OF_COLLISIONS - uNumCollisions;

    if(uNumCollisions == 0)
        return FALSE;

    *pNumCls = uNumCollisions;

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: fxHitHandleCollision
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL fxHitHandleCollision(FASTCOLLIDE cldArray[],
                             UINT32 uNumCollisions,
                             FXHIT* lpHit,
                             SINT32 iIndex)
{
    UINT32 i;

    mlUtilitySortCollideList(cldArray, uNumCollisions);

    for(i = 0; i < uNumCollisions; i++)
    {
        if(cldArray[i].Alpha > 0.5f)
        {
            fxHitSetHitVector(lpHit, &cldArray[i], iIndex);
            return TRUE;
        }
    }

    ////////////////////////////////////////////////////////////////////
    // If we are here, then one of the collisions hit an 'alpha window'.
    ////////////////////////////////////////////////////////////////////

    return FALSE;
}

//////////////////////////////////////////////////////////////////////
// Function: fxHitSetHitVector
// Description: Set the Explosion vector if we have collided. Called from
// fxHitHandleCollision.
//////////////////////////////////////////////////////////////////////
LOGICAL fxHitSetHitVector(FXHIT* lpHit, FASTCOLLIDE* lpCld, SINT32 i)
{
    if(lpHit->lpVehAttach == nullptr)
    {
        math3dVectorMultiply(&lpHit->hitPt[i],
                             &lpCld->CollidePnt,
                             &lpCld->CollideObject->mat);
    }
    ///////////////////////////////////
    // Update vehicle bullets on draw.
    ///////////////////////////////////
    else
    {
        mathAddVectors(&lpHit->hitPt[i],
                       &lpHit->hitPt[i],
                       &lpCld->CollidePnt);
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: fxHitDraw
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL fxHitDraw()
{
    if(drawlist.empty())
        return TRUE;

    oglSetWorldMatrix(&IdentityMatrix);

#if USE_STATESET
    stateset->Apply();

    std::vector<FXHIT*>::iterator itr = drawlist.begin();

    FXHIT* hit;

    for(; itr != drawlist.end(); ++itr)
    {
        hit = (*itr);

        if(hit->lpVehAttach != nullptr)
        {
            fxHitUpdateVehicleHit(hit);
        }
    }

    _DrawDecals();
#else
    piDisableMaterial();

    std::vector<FXHIT*>::iterator B = drawlist.begin();
    std::vector<FXHIT*>::iterator E = drawlist.end();

    FXHIT* lpHit;

    for(; B != E; ++B)
    {
        lpHit = (*B);

        if(lpHit->lpVehAttach != nullptr)
        {
            fxHitUpdateVehicleHit(lpHit);
        }
     }

    piSetRenderState(piRENDERSTATE_LIGHTING, FALSE);
    piSetRenderState(piRENDERSTATE_ZWRITEENABLE, FALSE);

    _DrawDecals();

    piSetRenderState(piRENDERSTATE_ZWRITEENABLE, bZWrite);
    piSetRenderState(piRENDERSTATE_LIGHTING, piLIGHTING_DEFAULT);
#endif

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _DrawDecals
// Description:
//////////////////////////////////////////////////////////////////////
void _DrawDecals()
{
    FXHIT* lpHit = nullptr;

    // 128 max * 5
    static piLVERTEX lv[640];
    // 128 * 12 indices
    static piINDICES indices[1536];

    SINT16 i = 0;
    SINT32 q = 0;

    FLOAT32 fHalfU = 0;
    FLOAT32 fHalfV = 0;

    FLOAT32 u0;
    FLOAT32 u1;
    FLOAT32 v0;
    FLOAT32 v1;

    UINT32 color = piRGBA(g_lpColor->r,
                          g_lpColor->g,
                          g_lpColor->b,
                          g_lpColor->a);

    std::vector<FXHIT*>::iterator itr = drawlist.begin();

    for(; itr != drawlist.end(); ++itr)
    {
        lpHit = (*itr);

        fHalfU = (lpHit->uv[0].u + lpHit->uv[1].u) * 0.5f;
        fHalfV = (lpHit->uv[0].v + lpHit->uv[1].v) * 0.5f;

        u0 = lpHit->uv[0].u;
        u1 = lpHit->uv[1].u;
        v0 = lpHit->uv[0].v;
        v1 = lpHit->uv[1].v;

        lv[i + 0].color = color;
        lv[i + 0].specular = colorALPHA100;
        lv[i + 0].x = lpHit->hitPt[0].x;
        lv[i + 0].y = lpHit->hitPt[0].y;
        lv[i + 0].z = lpHit->hitPt[0].z;
        lv[i + 0].tu = u0;
        lv[i + 0].tv = v1;

        lv[i + 1].color = color;
        lv[i + 1].specular = colorALPHA100;
        lv[i + 1].x = lpHit->hitPt[1].x;
        lv[i + 1].y = lpHit->hitPt[1].y;
        lv[i + 1].z = lpHit->hitPt[1].z;
        lv[i + 1].tu = u0;
        lv[i + 1].tv = v0;

        lv[i + 2].color = color;
        lv[i + 2].specular = colorALPHA100;
        lv[i + 2].x = lpHit->hitPt[2].x;
        lv[i + 2].y = lpHit->hitPt[2].y;
        lv[i + 2].z = lpHit->hitPt[2].z;
        lv[i + 2].tu = u1;
        lv[i + 2].tv = v0;

        lv[i + 3].color = color;
        lv[i + 3].specular = colorALPHA100;
        lv[i + 3].x = lpHit->hitPt[3].x;
        lv[i + 3].y = lpHit->hitPt[3].y;
        lv[i + 3].z = lpHit->hitPt[3].z;
        lv[i + 3].tu = u1;
        lv[i + 3].tv = v1;

        lv[i + 4].color = color;
        lv[i + 4].specular = colorALPHA100;
        lv[i + 4].x = lpHit->hitPt[4].x;
        lv[i + 4].y = lpHit->hitPt[4].y;
        lv[i + 4].z = lpHit->hitPt[4].z;
        lv[i + 4].tu = fHalfU;
        lv[i + 4].tv = fHalfV;

        indices[q + 0].v1 = i + 0;
        indices[q + 0].v2 = i + 1;
        indices[q + 0].v3 = i + 4;

        indices[q + 1].v1 = i + 1;
        indices[q + 1].v2 = i + 2;
        indices[q + 1].v3 = i + 4;

        indices[q + 2].v1 = i + 2;
        indices[q + 2].v2 = i + 3;
        indices[q + 2].v3 = i + 4;

        indices[q + 3].v1 = i + 3;
        indices[q + 3].v2 = i + 0;
        indices[q + 3].v3 = i + 4;

        q += 4;
        i += 5;
    }

    ogl::SetVertexPointer(3, GL_FLOAT, sizeof(piLVERTEX), &lv[0].x);
    ogl::SetTextureUnitCoordPointer(GL_TEXTURE0, 2, GL_FLOAT, sizeof(piLVERTEX), &lv[0].tu);
    ogl::SetColorPointer(4, GL_UNSIGNED_BYTE, sizeof(piLVERTEX), &lv[0].color);

    ogl::DrawElements(GL_TRIANGLES, q * 3, GL_UNSIGNED_SHORT, indices);
}

//////////////////////////////////////////////////////////////////////
// Function: fxHitUpdateVehicleHit
// Description: Update position of bomb hit texture on a vehicle.
//////////////////////////////////////////////////////////////////////
LOGICAL fxHitUpdateVehicleHit(FXHIT* lpHit)
{
    if(lpHit == nullptr)
        return FALSE;

    if(lpHit->lpVehAttach == nullptr)
        return FALSE;

    if(lpHit->lpVehAttach->VehicleObjectData == nullptr)
        return FALSE;

    weapUpdateVehicleHitmap(lpHit->lpVehAttach);

    math3dVectorMultiply(&lpHit->hitPt[0], &lpHit->hitPt[0], &lpHit->lpObj->mat);
    math3dVectorMultiply(&lpHit->hitPt[1], &lpHit->hitPt[1], &lpHit->lpObj->mat);
    math3dVectorMultiply(&lpHit->hitPt[2], &lpHit->hitPt[2], &lpHit->lpObj->mat);
    math3dVectorMultiply(&lpHit->hitPt[3], &lpHit->hitPt[3], &lpHit->lpObj->mat);
    math3dVectorMultiply(&lpHit->hitPt[4], &lpHit->hitPt[4], &lpHit->lpObj->mat);

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: fxHitDrawTriangle
// Description: Draw piece of the explostion hit mark.
//////////////////////////////////////////////////////////////////////
LOGICAL fxHitDrawTriangle(piVECTOR* v1,
                          piVECTOR* v2,
                          piVECTOR* v3,
                          FLOAT32 u[],
                          FLOAT32 v[],
                          UINT32 uColor)
{
    piLVERTEX lVert[3];

	lVert[0].color = uColor;
	lVert[0].specular = colorALPHA100;

    lVert[1] = lVert[0];
    lVert[2] = lVert[0];

    lVert[0].x = v1->x;
	lVert[0].y = v1->y;
	lVert[0].z = v1->z;
	lVert[0].tu = u[0];
	lVert[0].tv = v[0];

	lVert[1].x = v2->x;
	lVert[1].y = v2->y;
	lVert[1].z = v2->z;
	lVert[1].tu = u[1];
	lVert[1].tv = v[1];

	lVert[2].x = v3->x;
	lVert[2].y = v3->y;
	lVert[2].z = v3->z;
	lVert[2].tu = u[2];
	lVert[2].tv = u[2];

    if(!piRenderScene(piPT_TRIANGLELIST, piVT_LVERTEX, lVert, 3))
	{
		return FALSE;
	}

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: fxHitCanDraw
// Description: Should we draw the hitmarks?
//////////////////////////////////////////////////////////////////////
LOGICAL fxHitCanDraw(FXHIT* lpHit)
{
    if(lpHit->enumTex == IDE_HIT_NOTUSED)
        return FALSE;

    if(!rnvisVectorFOVCheck(&lpHit->vecPt, lpHit->fRadius * 2.0f))
        return FALSE;

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: fxHitGetHitTexture
// Description: Test for hitmark hitting water else get Bomb texture.
//////////////////////////////////////////////////////////////////////
LOGICAL fxHitGetHitTexture(FXHIT* lpHit)
{
    if(fxHitIsWater(lpHit))
    {
        return FALSE;
    }

    if(lpHit->lpVehAttach != nullptr)
    {
        // Use scorch mark for now.
        lpHit->enumTex = IDE_HIT_TERRAIN;
    }
    else
    {
        lpHit->enumTex = IDE_HIT_TERRAIN;
    }

    return TRUE;
}

//////////////////////////////////////////////
// Function: fxHitIsWater
// Description:
//////////////////////////////////////////////
LOGICAL fxHitIsWater(FXHIT* lpHit)
{
    FLOAT32 fRollCoef = 0.0f;

    vehVehicle*pVehicle = persGetPersonaVehiclePtr();

    mlGetObjectBumpinessAndFriction(nullptr,
									nullptr,
									nullptr,
									nullptr,
									&fRollCoef,
									nullptr,
                                    lpHit->lpObj);

    if(pVehicle != nullptr)
    {
        if(pVehicle->VehicleType != nullptr)
        {
            if(pVehicle->VehicleType->Type == VEH_TYPE_SEA)
            {
                if(fRollCoef > 0.999f && fRollCoef < 1.001f)
                {
                    return TRUE;
                }
            }
            else if(fRollCoef > 198.99f && fRollCoef < 199.001f)
            {
                return TRUE;
            }
        }
    }
    else if(fRollCoef > 198.99f && fRollCoef < 199.001f)
    {
        return TRUE;
    }

    return FALSE;
}

//////////////////////////////////////////////
// Function: fxHitFreeHit
// Description: Add a hitmark to the free list.
//////////////////////////////////////////////
LOGICAL fxHitFreeHit(FXHIT* lpHit)
{
    FXHIT* lpPrev = nullptr;
    SINT32 i;

    if(lpHit == nullptr)
        return FALSE;

    if(lpHit->lpObj)
    {
        g_mappedHits.erase(lpHit->mapIter);
    }

    i = lpHit->iIndex;

    memset(lpHit, 0, sizeof(FXHIT));

    lpHit->iIndex = i;

    lpPrev = g_hitFreeTail.prev;

    lpHit->prev = lpPrev;
    lpHit->next = &g_hitFreeTail;

    lpPrev->next = lpHit;
    g_hitFreeTail.prev = lpHit;

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: fxHitAddHit
// Description: Add a hitmark to the active list. This function is called after
// a successful call to fxExpGetExp.
//////////////////////////////////////////////////////////////////////
LOGICAL fxHitAddHit(FXHIT* lpHit)
{
    FXHIT* lpNext = nullptr;

    if(lpHit == nullptr)
        return FALSE;

    lpNext = g_hitHead.next;

    g_hitHead.next = lpHit;
    lpHit->prev = &g_hitHead;

    lpHit->next = lpNext;
    lpNext->prev = lpHit;

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: fxHitGetHit
// Description: Get a hitmark off the free list. If we are out of Bombs, it will
// take the oldest Bomb off the active list.
//////////////////////////////////////////////////////////////////////
LOGICAL fxHitGetHit(FXHIT** lpHit)
{
    FXHIT* lpNode = nullptr;
    FXHIT* lpNext = nullptr;
    FXHIT* lpPrev = nullptr;

    if(lpHit == nullptr)
        return FALSE;

    if(!fxHitCheckForHits())
        return FALSE;

    lpNode = g_hitFreeHead.next;

    lpPrev = lpNode->prev;
    lpNext = lpNode->next;

    lpPrev->next = lpNext;
    lpNext->prev = lpPrev;

    lpNode->next = nullptr;
    lpNode->prev = nullptr;

    lpNode->matSize = IdentityMatrix;

    *lpHit = lpNode;

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: fxHitCheckForHits
// Description: Check for free hitmarks. If there are none, take the oldest
// hitmark off the active list.
//////////////////////////////////////////////////////////////////////
LOGICAL fxHitCheckForHits()
{
    if(g_hitFreeHead.next == &g_hitFreeTail)
    {
        FXHIT* lpHit = nullptr;

        if(g_hitTail.prev == &g_hitHead)
            return FALSE;

        lpHit = g_hitTail.prev;

        if(!fxHitReleaseHit(lpHit))
            return FALSE;

        if(!fxHitFreeHit(lpHit))
            return FALSE;
    }

    if(g_hitFreeHead.next == &g_hitFreeTail)
        return FALSE;

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: fxHitReleaseHit
// Description: Release hitmark from active list...do before fxExpFreeExp.
//////////////////////////////////////////////////////////////////////
LOGICAL fxHitReleaseHit(FXHIT* lpHit)
{
    FXHIT* lpNext = nullptr;
    FXHIT* lpPrev = nullptr;

    if(lpHit == nullptr)
        return FALSE;

    lpNext = lpHit->next;
    lpPrev = lpHit->prev;

    if(lpNext == nullptr || lpPrev == nullptr)
        return FALSE;

    lpPrev->next = lpNext;
    lpNext->prev = lpPrev;

    lpHit->prev = nullptr;
    lpHit->next = nullptr;

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: fxHitDetachHit
// Description: Detach hitmark from object.
//////////////////////////////////////////////////////////////////////
LOGICAL fxHitDetachHit(OBJECT* lpObj)
{
    if(g_lpHits == nullptr)
        return TRUE;

    if(lpObj)
    {
        if(!(lpObj->flag2 & OBJECT_HAS_FX))
            return FALSE;
    }

    _UnMap((void*)lpObj);

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _UnMap
// Description:
//////////////////////////////////////////////////////////////////////
void _UnMap(void* pKey)
{
    std::multimap<void* , SINT32>::iterator I;
    std::multimap<void* , SINT32>::iterator FREE;

    if(g_mappedHits.empty())
        return;

    I = g_mappedHits.find(pKey);

    std::vector<SINT32> removeList;

    while(I != g_mappedHits.end())
    {
        if(I->first != pKey)
            break;

        FREE = I;
        ++I;

        SINT32 iIndex = FREE->second - 1;

        removeList.push_back(iIndex);
        g_mappedHits.erase(FREE);
    }

    for(UINT32 u = 0; u < removeList.size(); u++)
    {
        g_lpHits[removeList[u]].lpObj = nullptr;

        fxHitReleaseHit(&g_lpHits[removeList[u]]);
        fxHitFreeHit(&g_lpHits[removeList[u]]);
    }
}

//////////////////////////////////////////////////////////////////////
// Function: fxHitSetColor
// Description:
//////////////////////////////////////////////////////////////////////
void fxHitSetColor(piCOLORVALUE* lpColor)
{
    g_lpColor = lpColor;
}

//////////////////////////////////////////
// Function: fxHitUpdate
// Desctiption:
//////////////////////////////////////////
void fxHitUpdate()
{
    if(g_hitHead.next == nullptr) return;
    if(g_hitHead.next == &g_hitTail) return;

    FXHIT* lpHit = g_hitHead.next;
    FXHIT* tail = &g_hitTail;
    FXHIT* free = nullptr;

    piVECTOR vecDiff;
    FLOAT32 fLenSqrd = 0.0f;

    drawlist.clear();

    while(lpHit != tail)
    {
        if(!fxHitCanDraw(lpHit))
        {
            lpHit = lpHit->next;
            continue;
        }

        // Damaged building, for example
        if(lpHit->lpObj->flags & OBJECT_SKIPDRAWING)
        {
            free = lpHit;
            lpHit = lpHit->next;

            fxHitReleaseHit(free);
            fxHitFreeHit(free);

            continue;
        }

        // Free if over 2000 meters away
        if(lpHit->fDistanceSqrd > 4000000.0f)
        {
            free = lpHit;
            lpHit = lpHit->next;

            fxHitReleaseHit(free);
            fxHitFreeHit(free);

            continue;
        }

        lpHit->vecOldPt = lpHit->vecPt;

        fxUtlUpdatePosition(&lpHit->vecPt, &lpHit->vecPos);
        lpHit->fDistanceSqrd = fxUtlGetDistanceSqrd(&lpHit->vecPt);

        // If this is not zero, then world offset had change. I'm not even
        // going to bother with this crap. Just free the hit mark.
        mathSubVectors(&vecDiff, &lpHit->vecOldPt, &lpHit->vecPt);

        fLenSqrd = vecDiff.x * vecDiff.x +
                   vecDiff.y * vecDiff.y +
                   vecDiff.z * vecDiff.z;

        if(fLenSqrd > 0.0001f)
        {
            free = lpHit;
            lpHit = lpHit->next;

            fxHitReleaseHit(free);
            fxHitFreeHit(free);

            continue;
        }

        drawlist.push_back(lpHit);

        lpHit = lpHit->next;
    }
}

//////////////////////////////////////////
// Function: fxHitSetCreateTime
// Desctiption:
//////////////////////////////////////////
void fxHitSetCreateTime(SINT32 iID, FLOAT32 fTime)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_HITS)
        return;

    g_lpHits[iIndex].fCreateTime = fTime;
}

//////////////////////////////////////////
// Function: fxHitSetAttachedObject
// Desctiption:
//////////////////////////////////////////
void fxHitSetAttachedObject(SINT32 iID, OBJECT* lpObj)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_HITS)
        return;

    FXHIT* lpHit = &g_lpHits[iIndex];

    lpHit->lpObj = fxUtlGetObject(lpObj);

    if(lpHit->lpObj)
    {
        typedef std::pair<void* , SINT32> ins;

        lpHit->mapIter = g_mappedHits.insert(ins((void*)lpHit->lpObj, iID));

        lpHit->lpObj->flag2 |= OBJECT_HAS_FX;
    }
}

//////////////////////////////////////////
// Function: fxHitSetNormal
// Desctiption:
//////////////////////////////////////////
void fxHitSetNormal(SINT32 iID, FLOAT32 x, FLOAT32 y, FLOAT32 z)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_HITS)
        return;

    g_lpHits[iIndex].n.x = x;
    g_lpHits[iIndex].n.y = y;
    g_lpHits[iIndex].n.z = z;
}

//////////////////////////////////////////
// Function: fxHitSetPosition
// Desctiption:
//////////////////////////////////////////
void fxHitSetPosition(SINT32 iID, FLOAT64 x, FLOAT64 y, FLOAT64 z)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_HITS)
        return;

    g_lpHits[iIndex].vecPos.x = x;
    g_lpHits[iIndex].vecPos.y = y;
    g_lpHits[iIndex].vecPos.z = z;
}

//////////////////////////////////////////
// Function: fxHitSetRadius
// Desctiption:
//////////////////////////////////////////
void fxHitSetRadius(SINT32 iID, FLOAT32 fRadius)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_HITS)
        return;

    g_lpHits[iIndex].fRadius = fRadius;
}

//////////////////////////////////////////
// Function: fxHitSetEnergy
// Desctiption:
//////////////////////////////////////////
void fxHitSetEnergy(SINT32 iID, FLOAT32 fEnergy)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_HITS)
        return;

    g_lpHits[iIndex].fEnergy = fEnergy;
}

//////////////////////////////////////////
// Function: fxHitSetTopLevelObject
// Desctiption:
//////////////////////////////////////////
void fxHitSetTopLevelObject(SINT32 iID, OBJECT* lpObj)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_HITS)
        return;

    g_lpHits[iIndex].lpTop = lpObj;
}

//////////////////////////////////////////
// Function: fxHitSetVehicle
// Desctiption:
//////////////////////////////////////////
void fxHitSetVehicle(SINT32 iID, vehVehicle* lpVeh)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_HITS)
        return;

    g_lpHits[iIndex].lpVehAttach = lpVeh;
}

//////////////////////////////////////////
// Function: fxHitInitHit
// Desctiption:
//////////////////////////////////////////
void fxHitInitHit(SINT32 id)
{
    SINT32 index = id - 1;

    if(index < 0 || index >= DEF_MAX_HITS)
        return;

    if(g_lpHits[index].lpObj == nullptr)
    {
        fxHitReleaseHit(&g_lpHits[index]);
        fxHitFreeHit(&g_lpHits[index]);
        return;
    }

    if(g_lpHits[index].lpObj->flag2 & OBJECT_SPEEDTREE)
    {
        fxHitReleaseHit(&g_lpHits[index]);
        fxHitFreeHit(&g_lpHits[index]);
        return;
    }

    if(fxHitIsWater(&g_lpHits[index]))
    {
        fxHitReleaseHit(&g_lpHits[index]);
        fxHitFreeHit(&g_lpHits[index]);
        return;
    }

    if(g_lpHits[index].lpObj)
    {
        if(g_lpHits[index].lpObj->flag2 & OBJECT_FLAG)
        {
            fxHitReleaseHit(&g_lpHits[index]);
            fxHitFreeHit(&g_lpHits[index]);

            return;
        }
    }

    // 30 is scorch texture on fx.dds
    fxUtlGetTextureUV(g_lpHits[index].uv, "scorch");

    fxUtlUpdatePosition(&g_lpHits[index].vecPt, &g_lpHits[index].vecPos);

    if(g_lpHits[index].lpVehAttach != nullptr)
    {
        // Hack for now so we can see scorch marks on vehicles.
        g_lpHits[index].fRadius = g_lpHits[index].fEnergy * 0.001f;
    }

    if(!fxHitClip(&g_lpHits[index], &g_lpHits[index].n))
    {
        fxHitReleaseHit(&g_lpHits[index]);
        fxHitFreeHit(&g_lpHits[index]);
        return;
    }

    if(!fxHitGetHitTexture(&g_lpHits[index]))
    {
        fxHitReleaseHit(&g_lpHits[index]);
        fxHitFreeHit(&g_lpHits[index]);
        return;
    }
}
