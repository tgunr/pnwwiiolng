#include <syscfg.h> // Path is project specific.
/////////////////////////////////////////////////////
// Module: fxQuad.cpp
// Description:
/////////////////////////////////////////////////////
#include "fx/fxUtl.h"
#include "fx/fxQuad.h"

#include "math/math3d.h"
#include "math/mathvect.h"
#include "math/mathnum.h"
#include "ld/ldpix.h"
#include "include/vars.h"
#include "rn/rnvis.h"
#include "mem/mem.h"
#include "math/mathnum.h"

#include <vector>
#include <algorithm>
using namespace std;

static vector<FXQUAD*> _quads;

#define DEF_MAX_QUADS   60000

static piLPMATRIX g_lpInvertView = NULL;

static FXQUAD* g_lpQuads = NULL;

static FXQUAD g_pclFreeHead;
static FXQUAD g_pclFreeTail;

static FXQUAD g_pclHead;
static FXQUAD g_pclTail;

static piLPCOLORVALUE g_lpColor = NULL;

//static FLOAT32 g_fPi = PI;

static SINT32 _QuadSort(const FXQUAD* const l, const FXQUAD* const r);

////////////////////////////
// static LOGICAL functions
////////////////////////////
static LOGICAL inline _AddParticle(FXQUAD* lpQuad);
static LOGICAL _CanDraw(FXQUAD* lpQuad);
static LOGICAL _CheckForParticle();
static LOGICAL _FreeParticle(FXQUAD* lpQuad, bool bIsPopulated);
static LOGICAL _FreeIfNecessary(FXQUAD* lpQuad);
static LOGICAL inline _GetParticle(FXQUAD** lpQuad);
static LOGICAL _StretchQuad(FXQUAD* lpQuad);

////////////////////////////
// static LOGICAL functions
////////////////////////////
static void _DrawQuads();
static void _InitQuad(FXQUAD* lpQuad);
static void _InitialRotate(FXQUAD* lpQuad);
static void _InitVertices(FXQUAD* lpQuad);
static void inline _Rotate(FXQUAD* lpQuad);
static void inline _SetMaterial(FXQUAD* lpQuad);
static void inline _SetupShape(FXQUAD* lpQuad);
static void inline _UpdateAlpha(FXQUAD* lpQuad);
static void inline _UpdateColor(FXQUAD* lpQuad);
static void inline _UpdateDrag(FXQUAD* lpQuad);
static void inline _UpdateQuad(FXQUAD* lpQuad);
static void inline _UpdateRadius(FXQUAD* lpQuad);
static void inline _UpdateRotate(FXQUAD* lpQuad);
static void inline _UpdateVelocity(FXQUAD* lpQuad);

//////////////////////////////////////////
// Function: fxQuadInit
// Description:
//////////////////////////////////////////
LOGICAL fxQuadInit()
{
    SINT32 i;

    g_lpQuads = (FXQUAD*)memAlloc(sizeof(FXQUAD) * DEF_MAX_QUADS);

    if(g_lpQuads == NULL)
        return FALSE;

	// memAlloc does this for us
//  memset(g_lpQuads, 0, sizeof(FXQUAD) * DEF_MAX_QUADS);

    memset(&g_pclHead, 0, sizeof(FXQUAD));
    memset(&g_pclTail, 0, sizeof(FXQUAD));

    g_pclHead.next = &g_pclTail;
    g_pclTail.prev = &g_pclHead;

    memset(&g_pclFreeHead, 0, sizeof(FXQUAD));
    memset(&g_pclFreeTail, 0, sizeof(FXQUAD));

    g_pclFreeHead.next = &g_pclFreeTail;
    g_pclFreeTail.prev = &g_pclFreeHead;

    for(i = 0; i < DEF_MAX_QUADS; i++)
    {
        g_lpQuads[i].iIndex = i;
        _FreeParticle(&g_lpQuads[i], false);
    }

    return TRUE;
}

//////////////////////////////////////////
// Function: fxQuadKill
// Description:
//////////////////////////////////////////
LOGICAL fxQuadKill()
{
    if(g_lpQuads != NULL)
    {
        memFree(g_lpQuads);
        g_lpQuads = NULL;
    }

    // Set to NULL in case of further updates.
    memset(&g_pclHead, 0, sizeof(FXQUAD));
    memset(&g_pclTail, 0, sizeof(FXQUAD));
    memset(&g_pclFreeHead, 0, sizeof(FXQUAD));
    memset(&g_pclFreeTail, 0, sizeof(FXQUAD));

    _quads.clear();

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: fxQuadGetNewParticle
// Description: Create empty particle
//////////////////////////////////////////////////////////////////////
SINT32 fxQuadGetNewParticle()
{
    FXQUAD* lpQuad = NULL;

    if(!_GetParticle(&lpQuad))
        return -1;

    if(!_AddParticle(lpQuad))
        return -1;

    return lpQuad->iIndex + 1;
}

//////////////////////////////////////////////////////////////////////
// Function: fxQuadInitParticle
// Description:
//////////////////////////////////////////////////////////////////////
void fxQuadInitParticle(SINT32 iID)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_QUADS)
        return;

    g_lpQuads[iIndex].bDraw = TRUE;

    fxUtlGetTextureUV(g_lpQuads[iIndex].uv, g_lpQuads[iIndex].uSpriteNum);

    _InitQuad(&g_lpQuads[iIndex]);

    // Needs to be set for initial velocity update
    g_lpQuads[iIndex].lastupdate = g_lpQuads[iIndex].quadTime.p1;

    if(g_lpQuads[iIndex].uFlags & DEF_FXFLAG_STOPATGROUND)
    {
        fxUtlUpdatePosition(&g_lpQuads[iIndex].relposition, &g_lpQuads[iIndex].worldpos);
        fxUtlGetGroundPos(&g_lpQuads[iIndex].relposition, &g_lpQuads[iIndex].groundpos);
    }

    // Update now, so we won't be one cycle behind.
    _UpdateQuad(&g_lpQuads[iIndex]);
}

//////////////////////////////////////////
// Function: _InitialRotate
// Description:
//////////////////////////////////////////
void _InitialRotate(FXQUAD* lpQuad)
{
    SINT32 iRotateIndex = abs(mathGetChaoticNumber() % 360);

    piMATRIX matRot = identity;

    matRot._11 = fxUtlGetCos(iRotateIndex);
    matRot._12 = fxUtlGetSin(iRotateIndex);

    matRot._21 = -matRot._12;
    matRot._22 =  matRot._11;

    fxUtlRotateZ(lpQuad->quad, &matRot);
}

//////////////////////////////////////////
// Function: _InitQuad
// Description:
//////////////////////////////////////////
void _InitQuad(FXQUAD* lpQuad)
{
    _InitVertices(lpQuad);

    if(lpQuad->bStretch == FALSE)
    {
        _InitialRotate(lpQuad);
    }
}

//////////////////////////////////////////
// Function: _InitVertices
// Description:
//////////////////////////////////////////
void _InitVertices(FXQUAD* lpQuad)
{
    FLOAT32 fHalradius = lpQuad->radius * 0.5f;

    lpQuad->quad[0].x = -fHalradius;
    lpQuad->quad[0].y = -fHalradius;

    lpQuad->quad[1].x = -fHalradius;
    lpQuad->quad[1].y =  fHalradius;

    lpQuad->quad[2].x =  fHalradius;
    lpQuad->quad[2].y =  fHalradius;

    lpQuad->quad[3].x =  fHalradius;
    lpQuad->quad[3].y = -fHalradius;
}

//////////////////////////////////////////
// Function: _FreeParticle
// Description:
// Pass 'bIsPopulated' as true if the
// lpQuad structure needs to be memset(0)
//////////////////////////////////////////
LOGICAL _FreeParticle(FXQUAD* lpQuad, bool bIsPopulated)
{
    FXQUAD* lpPrev = NULL;
    SINT32 i;

    if(lpQuad == NULL)
        return FALSE;

    i = lpQuad->iIndex;

	if(bIsPopulated)
	    memset(lpQuad, 0, sizeof(FXQUAD));

    lpQuad->iIndex = i;

    lpPrev = g_pclFreeTail.prev;

    lpQuad->prev = lpPrev;
    lpQuad->next = &g_pclFreeTail;

    lpPrev->next = lpQuad;
    g_pclFreeTail.prev = lpQuad;

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _ReleaseQuad
// Description: Release quad from active list...
//////////////////////////////////////////////////////////////////////
LOGICAL _ReleaseQuad(FXQUAD* lpQuad)
{
    FXQUAD* lpNext = NULL;
    FXQUAD* lpPrev = NULL;

    if(lpQuad == NULL)
        return FALSE;

    lpNext = lpQuad->next;
    lpPrev = lpQuad->prev;

    if(lpNext == NULL || lpPrev == NULL)
        return FALSE;

    lpPrev->next = lpNext;
    lpNext->prev = lpPrev;

    lpQuad->prev = NULL;
    lpQuad->next = NULL;

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _CheckForParticle
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL _CheckForParticle()
{
    if(g_pclFreeHead.next == &g_pclFreeTail)
    {
        FXQUAD* lpQuad = NULL;

        if(g_pclTail.prev == &g_pclHead)
            return FALSE;

        lpQuad = g_pclTail.prev;

        lpQuad->bDraw = FALSE;

        _FreeIfNecessary(lpQuad);
    }

    if(g_pclFreeHead.next == &g_pclFreeTail)
        return FALSE;

    return TRUE;
}

//////////////////////////////////////////
// Function: fxQuadPullParticles
// Desctiption:
//////////////////////////////////////////
SINT32 fxQuadPullParticles(SINT32 iPclID, FXQUAD* lpQuadParts[], SINT32 iMaxSize)
{
    SINT32 iSize = 0;
    FXQUAD* lpQuad = g_pclHead.next;
    FXQUAD* lpTail = &g_pclTail;

    if(lpQuad == NULL)
        return 0;

    while(lpQuad != lpTail)
    {
        if(lpQuad->iPclID == iPclID)
        {
            lpQuadParts[iSize++] = lpQuad;
        }

        if(iSize == iMaxSize)
            break;

        lpQuad = lpQuad->next;
    }

    return iSize;
}

//////////////////////////////////////////
// Function: fxQuadUpdate
// Desctiption:
//////////////////////////////////////////
void fxQuadUpdate()
{
    if(g_pclHead.next == NULL) return;
    if(g_pclHead.next == &g_pclTail) return;

    FXQUAD* lpQuad = g_pclHead.next;
    FXQUAD* lpTail = &g_pclTail;

    _quads.erase(_quads.begin(), _quads.end());

    FXQUAD* FREE = NULL;

    while(lpQuad != lpTail)
    {
        _UpdateQuad(lpQuad);

        if(!lpQuad->bDraw)
        {
            FREE = lpQuad;
            lpQuad = lpQuad->next;

            _FreeIfNecessary(FREE);
            continue;
        }

        lpQuad->lastupdate = (FLOAT32)frametime;
        lpQuad = lpQuad->next;
    }
}

//////////////////////////////////////////
// Function: fxQuadDraw
// Description:
//////////////////////////////////////////
LOGICAL fxQuadDraw()
{
    if(_quads.empty())
        return TRUE;

    sort(_quads.begin(), _quads.end(), _QuadSort);

    piDisableMaterial();

    UINT32 zwrite = piGetEngineRenderState(piRENDERSTATE_ZWRITEENABLE);
    UINT32 lighton = piGetEngineRenderState(piRENDERSTATE_LIGHTING);
    UINT32 alphatest = piGetEngineRenderState(piRENDERSTATE_ALPHATESTENABLE);

    piSetRenderState(piRENDERSTATE_ZWRITEENABLE, FALSE);
    piSetRenderState(piRENDERSTATE_LIGHTING, FALSE);
    piSetRenderState(piRENDERSTATE_ALPHATESTENABLE, FALSE);

    _DrawQuads();

    piSetRenderState(piRENDERSTATE_ZWRITEENABLE, zwrite);
    piSetRenderState(piRENDERSTATE_LIGHTING, lighton);
    piSetRenderState(piRENDERSTATE_ALPHATESTENABLE, alphatest);

    FXQUAD* B = g_pclHead.next;
    FXQUAD* E = &g_pclTail;
    FXQUAD* FREE = NULL;

    while(B != E)
    {
        FREE = B;
        B = B->next;

        _FreeIfNecessary(FREE);
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _DrawQuads
// Description:
//////////////////////////////////////////////////////////////////////
void _DrawQuads()
{
    FXQUAD* lpQuad = NULL;
    FXQUAD* lpLastQuad = NULL;

    static piLVERTEX lv[2048];
    static piINDICES indices[1024];

    vector<FXQUAD*>::iterator B = _quads.begin();
    vector<FXQUAD*>::iterator E = _quads.end();

    SINT16 i = 0;
    SINT32 q = 0;

    UINT32 uColor;

    LOGICAL glow = FALSE;
    LOGICAL flat = FALSE;

    for(B; B != E; ++B)
    {
        lpQuad = (*B);

        uColor = piRGBA(lpQuad->piColor.r,
                        lpQuad->piColor.g,
                        lpQuad->piColor.b,
                        lpQuad->piColor.a);

        if(flat)
        {
            if(!(lpQuad->uFlags & DEF_FXFLAG_FLAT))
            {
                // Send 'flat' particles
                if(q > 0)
                {
                    piRenderIndexScene(piPT_TRIANGLELIST,
                                       piVT_LVERTEX,
                                       lv,
                                       i,
                                       (UINT16*)indices,
                                       q * 3);

                    q = 0;
                    i = 0;

                    flat = FALSE;
                }

                glPolygonOffset(0.0f, 0.0f);
            }
        }

        if(glow)
        {
            if(!(lpQuad->uFlags & DEF_FXFLAG_GLOW))
            {
                // Send glowing particles
                if(q > 0)
                {
                    piRenderIndexScene(piPT_TRIANGLELIST,
                                       piVT_LVERTEX,
                                       lv,
                                       i,
                                       (UINT16*)indices,
                                       q * 3);

                    q = 0;
                    i = 0;

                    glow = FALSE;
                    piSetRenderState(piRENDERSTATE_DESTBLEND, DestBlend);
                }
            }
        }

        if(!flat && lpQuad->uFlags & DEF_FXFLAG_FLAT)
        {
            // Send non-flat particles before we change states
            if(q > 0)
            {
                piRenderIndexScene(piPT_TRIANGLELIST,
                                   piVT_LVERTEX,
                                   lv,
                                   i,
                                   (UINT16*)indices,
                                   q * 3);

                q = 0;
                i = 0;
            }

            flat = TRUE;
            glPolygonOffset(0.0f, -0.1f);
        }

        if(!glow && lpQuad->uFlags & DEF_FXFLAG_GLOW)
        {
            // Send non-glowing particles before we change states
            if(q > 0)
            {
                piRenderIndexScene(piPT_TRIANGLELIST,
                                   piVT_LVERTEX,
                                   lv,
                                   i,
                                   (UINT16*)indices,
                                   q * 3);

                q = 0;
                i = 0;
            }

            glow = TRUE;
            piSetRenderState(piRENDERSTATE_DESTBLEND, piBLEND_ONE);
        }

        lv[i + 0].color = uColor;
        lv[i + 0].specular = colorALPHA100;
        lv[i + 0].x = lpQuad->coords[0].x;
        lv[i + 0].y = lpQuad->coords[0].y;
        lv[i + 0].z = lpQuad->coords[0].z;
        lv[i + 0].tu = lpQuad->uv[0].u;
        lv[i + 0].tv = lpQuad->uv[1].v;

        lv[i + 1].color = uColor;
        lv[i + 1].specular = colorALPHA100;
        lv[i + 1].x = lpQuad->coords[1].x;
        lv[i + 1].y = lpQuad->coords[1].y;
        lv[i + 1].z = lpQuad->coords[1].z;
        lv[i + 1].tu = lpQuad->uv[0].u;
        lv[i + 1].tv = lpQuad->uv[0].v;

        lv[i + 2].color = uColor;
        lv[i + 2].specular = colorALPHA100;
        lv[i + 2].x = lpQuad->coords[2].x;
        lv[i + 2].y = lpQuad->coords[2].y;
        lv[i + 2].z = lpQuad->coords[2].z;
        lv[i + 2].tu = lpQuad->uv[1].u;
        lv[i + 2].tv = lpQuad->uv[0].v;

        lv[i + 3].color = uColor;
        lv[i + 3].specular = colorALPHA100;
        lv[i + 3].x = lpQuad->coords[3].x;
        lv[i + 3].y = lpQuad->coords[3].y;
        lv[i + 3].z = lpQuad->coords[3].z;
        lv[i + 3].tu = lpQuad->uv[1].u;
        lv[i + 3].tv = lpQuad->uv[1].v;

        indices[q + 0].v1 = i + 0;
        indices[q + 0].v2 = i + 1;
        indices[q + 0].v3 = i + 2;

        indices[q + 1].v1 = i + 2;
        indices[q + 1].v2 = i + 3;
        indices[q + 1].v3 = i + 0;

        q += 2;
        i += 4;

        if(q >= 1024)
        {
            piRenderIndexScene(piPT_TRIANGLELIST,
                               piVT_LVERTEX,
                               lv,
                               i,
                               (UINT16*)indices,
                               q * 3);

            q = 0;
            i = 0;
        }

        lpLastQuad = lpQuad;
    }

    // Send leftovers
    if(q > 0)
    {
        piRenderIndexScene(piPT_TRIANGLELIST,
                           piVT_LVERTEX,
                           lv,
                           i,
                           (UINT16*)indices,
                           q * 3);

        if(flat)
        {
            glPolygonOffset(0.0f, 0.0f);
        }

        if(glow)
        {
            piSetRenderState(piRENDERSTATE_DESTBLEND, DestBlend);
        }
    }
 }

//////////////////////////////////////////////////////////////////////
// Function: _QuadSort
// Description:
//////////////////////////////////////////////////////////////////////
SINT32 _QuadSort(const FXQUAD* const l, const FXQUAD* const r)
{
    return l->distancesqrd > r->distancesqrd;
}

//////////////////////////////////////////////////////////////////////
// Function: _CanDraw
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL _CanDraw(FXQUAD* lpQuad)
{
    if(!lpQuad->bDraw)
        return FALSE;

    if(!rnvisVectorFOVCheck(&lpQuad->relposition, lpQuad->radius))
        return FALSE;

    lpQuad->distancesqrd = fxUtlGetDistanceSqrd(&lpQuad->relposition);

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _FreeIfNecessary
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL _FreeIfNecessary(FXQUAD* lpQuad)
{
    if(lpQuad != NULL)
    {
        if(lpQuad->bDraw == FALSE)
        {
            if(lpQuad->pFnKill)
            {
                if(lpQuad->killData == NULL)
                {
                    lpQuad->pFnKill(lpQuad);
                }
                else
                {
                    lpQuad->pFnKill(lpQuad->killData);
                }
            }

            _ReleaseQuad(lpQuad);
            _FreeParticle(lpQuad, true);
        }
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _SetupShape
// Description: Final particle setup for display
//////////////////////////////////////////////////////////////////////
void _SetupShape(FXQUAD* q)
{
    if(q->bStretch)
    {
        _StretchQuad(q);
        return;
    }

// Too much to do if rotation involved
#if(0)
    FLOAT32 rad = q->radius * q->scale * 0.5f;

    piVECTOR X, Y;

    X.x = playright.x * rad;
    X.y = playright.y * rad;
    X.z = playright.z * rad;

    Y.x = playup.x * rad;
    Y.y = playup.y * rad;
    Y.z = playup.z * rad;
#endif

    piVECTOR v1, v2, v3, v4;

    FLOAT32 s = q->scale;
    FLOAT32 scaledrightx, scaledrighty;
    FLOAT32 scaledupx, scaledupy;
    FLOAT32 scaleddirx, scaleddiry;

    if(q->uFlags & DEF_FXFLAG_FLAT)
    {
        v1.x = q->quad[0].x * s;
        v1.y = q->quad[0].y * s;
        v1.z = q->quad[0].z - 0.01f;

        v2.x = q->quad[1].x * s;
        v2.y = q->quad[1].y * s;
        v2.z = q->quad[1].z - 0.01f;

        v3.x = q->quad[2].x * s;
        v3.y = q->quad[2].y * s;
        v3.z = q->quad[2].z - 0.01f;

        v4.x = q->quad[3].x * s;
        v4.y = q->quad[3].y * s;
        v4.z = q->quad[3].z - 0.01f;
    }
    else
    {
        scaledrightx = g_lpInvertView->_11 * s;
        scaledrighty = g_lpInvertView->_21 * s;

        scaledupx = g_lpInvertView->_12 * s;
        scaledupy = g_lpInvertView->_22 * s;

        scaleddirx = g_lpInvertView->_13 * s;
        scaleddiry = g_lpInvertView->_23 * s;

        v1.x = q->quad[0].x * scaledrightx + q->quad[0].y * scaledrighty;
        v1.y = q->quad[0].x * scaledupx    + q->quad[0].y * scaledupy;
        v1.z = q->quad[0].x * scaleddirx   + q->quad[0].y * scaleddiry;

        v2.x = q->quad[1].x * scaledrightx + q->quad[1].y * scaledrighty;
        v2.y = q->quad[1].x * scaledupx    + q->quad[1].y * scaledupy;
        v2.z = q->quad[1].x * scaleddirx   + q->quad[1].y * scaleddiry;

        v3.x = q->quad[2].x * scaledrightx + q->quad[2].y * scaledrighty;
        v3.y = q->quad[2].x * scaledupx    + q->quad[2].y * scaledupy;
        v3.z = q->quad[2].x * scaleddirx   + q->quad[2].y * scaleddiry;

        v4.x = q->quad[3].x * scaledrightx + q->quad[3].y * scaledrighty;
        v4.y = q->quad[3].x * scaledupx    + q->quad[3].y * scaledupy;
        v4.z = q->quad[3].x * scaleddirx   + q->quad[3].y * scaleddiry;
    }

    q->coords[0].x = q->relposition.x + v1.x;
    q->coords[0].y = q->relposition.y + v1.y;
    q->coords[0].z = q->relposition.z + v1.z;

    q->coords[1].x = q->relposition.x + v2.x;
    q->coords[1].y = q->relposition.y + v2.y;
    q->coords[1].z = q->relposition.z + v2.z;

    q->coords[2].x = q->relposition.x + v3.x;
    q->coords[2].y = q->relposition.y + v3.y;
    q->coords[2].z = q->relposition.z + v3.z;

    q->coords[3].x = q->relposition.x + v4.x;
    q->coords[3].y = q->relposition.y + v4.y;
    q->coords[3].z = q->relposition.z + v4.z;
}

//////////////////////////////////////////////////////////////////////
// Function: _StretchQuad
// Description: From my online bible...gamedev.net
//////////////////////////////////////////////////////////////////////
LOGICAL _StretchQuad(FXQUAD* q)
{
    FLOAT32 vel2 = math3dDotProduct(&q->vecVel, &q->vecVel);
    FLOAT32 invVel2 = 1.0f / math3dFastSqrt(vel2);

    piVECTOR forward;

    FLOAT32 rad = q->scale * 0.5f * q->radius;

    if(!q->stretchFixed && vel2 * 0.1f > rad * rad)
    {
        forward.x = -q->vecVel.x * 0.1f;
        forward.y = -q->vecVel.y * 0.1f;
        forward.z =  q->vecVel.z * 0.1f;
    }
    else if(q->stretchFixed)
    {
        // 8 is arbritrary
        forward.x = -q->vecVel.x * rad * invVel2 * 8.0f;
        forward.y = -q->vecVel.y * rad * invVel2 * 8.0f;
        forward.z =  q->vecVel.z * rad * invVel2 * 8.0f;
    }
    else
    {
        forward.x = -q->vecVel.x;
        forward.y = -q->vecVel.y;
        forward.z =  q->vecVel.z;

        forward.x *= invVel2;
        forward.y *= invVel2;
        forward.z *= invVel2;
    }

    piVECTOR right;

    right.x = -playpos.x - q->relposition.x;
    right.y = -playpos.y - q->relposition.y;
    right.z = -playpos.z - q->relposition.z;

    piVECTOR up;

    math3dCrossProduct(&up, &forward, &right);

    mathReallyFastNormalize(up);

    up.x *= rad;
    up.y *= rad;
    up.z *= rad;

    piVECTOR offset = {0.0f, 0.0f, 0.0f};

    piVECTOR normForward = forward;

    mathReallyFastNormalize(normForward);

    offset.x = normForward.x * rad;
    offset.y = normForward.y * rad;
    offset.z = normForward.z * rad;

    q->coords[0].x = q->relposition.x + (forward.x + offset.x) + up.x;
    q->coords[0].y = q->relposition.y + (forward.y + offset.y) + up.y;
    q->coords[0].z = q->relposition.z + (forward.z + offset.z) + up.z;

    q->coords[1].x = q->relposition.x + (forward.x + offset.x) - up.x;
    q->coords[1].y = q->relposition.y + (forward.y + offset.y) - up.y;
    q->coords[1].z = q->relposition.z + (forward.z + offset.z) - up.z;

    q->coords[2].x = q->relposition.x - (forward.x - offset.x) - up.x;
    q->coords[2].y = q->relposition.y - (forward.y - offset.y) - up.y;
    q->coords[2].z = q->relposition.z - (forward.z - offset.z) - up.z;

    q->coords[3].x = q->relposition.x - (forward.x - offset.x) + up.x;
    q->coords[3].y = q->relposition.y - (forward.y - offset.y) + up.y;
    q->coords[3].z = q->relposition.z - (forward.z - offset.z) + up.z;

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _UpdateQuad
// Description:
//////////////////////////////////////////////////////////////////////
void _UpdateQuad(FXQUAD* lpQuad)
{
    if((FLOAT32)frametime < lpQuad->quadTime.p1)
        return;

    if((FLOAT32)frametime > lpQuad->quadTime.p4)
    {
        lpQuad->bDraw = FALSE;
        return;
    }

    lpQuad->lastpos = lpQuad->worldpos;

    if(lpQuad->uFlags & DEF_FXFLAG_STOPATGROUND)
    {
        if(lpQuad->worldpos.z < lpQuad->groundpos.z)
        {
            lpQuad->bDraw = FALSE;
            return;
        }
    }

    _UpdateDrag(lpQuad);
    _UpdateVelocity(lpQuad);
    _UpdateColor(lpQuad);
    _UpdateAlpha(lpQuad);
    _UpdateRadius(lpQuad);

    if(lpQuad->bStretch == FALSE)
    {
        _UpdateRotate(lpQuad);
    }

    _SetupShape(lpQuad);

    if(_CanDraw(lpQuad))
    {
        _quads.push_back(lpQuad);
    }
}

//////////////////////////////////////////////////////////////////////
// Function: _UpdateRotate
// Description:
//////////////////////////////////////////////////////////////////////
void _UpdateRotate(FXQUAD* lpQuad)
{
    if(lpQuad->uFlags & DEF_FXFLAG_INTROTATE)
    {
        fxUtlInterpolateVector(&lpQuad->quadTime, &lpQuad->quadRotate, &lpQuad->vecRot);
    }

    if(lpQuad->uFlags & DEF_FXFLAG_ROTATE)
    {
        _Rotate(lpQuad);
    }
}

//////////////////////////////////////////////////////////////////////
// Function: _UpdateRadius
// Description:
//////////////////////////////////////////////////////////////////////
void _UpdateRadius(FXQUAD* lpQuad)
{
    if(lpQuad->uFlags & DEF_FXFLAG_INTRADIUS)
    {
        fxUtlInterpolateRadius(&lpQuad->quadTime, &lpQuad->quadScale, &lpQuad->scale);
    }
}

//////////////////////////////////////////////////////////////////////
// Function: _UpdateDrag
// Description:
//////////////////////////////////////////////////////////////////////
void _UpdateDrag(FXQUAD* lpQuad)
{
    if(lpQuad->uFlags & DEF_FXFLAG_USEDRAG)
    {
        fxUtlApplyDrag(&lpQuad->vecVel, &lpQuad->dragco, &lpQuad->lastupdate);
    }
}

//////////////////////////////////////////////////////////////////////
// Function: _UpdateVelocity
// Description:
//////////////////////////////////////////////////////////////////////
void _UpdateVelocity(FXQUAD* lpQuad)
{
    if(lpQuad->uFlags & DEF_FXFLAG_INTVELOCITY)
    {
        fxUtlInterpolateVector(&lpQuad->quadTime, &lpQuad->quadVel, &lpQuad->vecVel);
    }

    fxUtlApplyVelocity(&lpQuad->vecVel,
                       &lpQuad->worldpos,
                       &lpQuad->lastupdate,
                       lpQuad->uFlags);

    fxUtlUpdatePosition(&lpQuad->relposition, &lpQuad->worldpos);
}

//////////////////////////////////////////////////////////////////////
// Function: _UpdateColor
// Description:
//////////////////////////////////////////////////////////////////////
void _UpdateColor(FXQUAD* lpQuad)
{
    if(lpQuad->uFlags & DEF_FXFLAG_INTCOLOR)
    {
        fxUtlInterpolateColor(&lpQuad->quadTime, &lpQuad->quadColor, &lpQuad->piColor);

        if(!(lpQuad->uFlags & DEF_FXFLAG_GLOW))
        {
            lpQuad->piColor.r *= g_lpColor->r;
            lpQuad->piColor.g *= g_lpColor->g;
            lpQuad->piColor.b *= g_lpColor->b;
        }
    }
    else if(lpQuad->uFlags & DEF_FXFLAG_MYCOLOR)
    {
        lpQuad->piColor.a = lpQuad->quadColor.p1.a * g_lpColor->a;
        lpQuad->piColor.r = lpQuad->quadColor.p1.r * g_lpColor->r;
        lpQuad->piColor.g = lpQuad->quadColor.p1.g * g_lpColor->g;
        lpQuad->piColor.b = lpQuad->quadColor.p1.b * g_lpColor->b;
    }
    else
    {
        lpQuad->piColor.a = g_lpColor->a;
        lpQuad->piColor.r = g_lpColor->r;
        lpQuad->piColor.g = g_lpColor->g;
        lpQuad->piColor.b = g_lpColor->b;
    }
}

//////////////////////////////////////////
// Function: _UpdateAlpha
// Desctiption:
//////////////////////////////////////////
void _UpdateAlpha(FXQUAD* lpQuad)
{
    if(lpQuad->uFlags & DEF_FXFLAG_INTALPHA)
    {
        fxUtlInterpolateAlpha(&lpQuad->quadTime, &lpQuad->quadColor, &lpQuad->piColor);
    }
    else
    {
        lpQuad->piColor.a = 1.0f;
    }
}

//////////////////////////////////////////
// Function: _Rotate
// Desctiption:
//////////////////////////////////////////
void _Rotate(FXQUAD* lpQuad)
{
    // Rotate around Z.
    piMATRIX matRot = identity;

    matRot._11 = (FLOAT32)cos(lpQuad->vecRot.z);
    matRot._12 = (FLOAT32)sin(lpQuad->vecRot.z);

    matRot._21 = -matRot._12;
    matRot._22 =  matRot._11;

    fxUtlRotateZ(lpQuad->quad, &matRot);
}

//////////////////////////////////////////////////////////////////////
// Function: _AddParticle
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL _AddParticle(FXQUAD* lpQuad)
{
    FXQUAD* lpNext = NULL;

    if(lpQuad == NULL)
        return FALSE;

    lpNext = g_pclHead.next;

    g_pclHead.next = lpQuad;
    lpQuad->prev = &g_pclHead;

    lpQuad->next = lpNext;
    lpNext->prev = lpQuad;

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _GetParticle
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL _GetParticle(FXQUAD** lpQuad)
{
    FXQUAD* lpNode = NULL;
    FXQUAD* lpNext = NULL;
    FXQUAD* lpPrev = NULL;

    if(lpQuad == NULL)
        return FALSE;

    if(!_CheckForParticle())
        return FALSE;

    lpNode = g_pclFreeHead.next;

    lpPrev = lpNode->prev;
    lpNext = lpNode->next;

    lpPrev->next = lpNext;
    lpNext->prev = lpPrev;

    lpNode->next = NULL;
    lpNode->prev = NULL;

    *lpQuad = lpNode;

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: fxQuadSetColor
// Description: Set color from outside module.
//////////////////////////////////////////////////////////////////////
void fxQuadSetColor(piLPCOLORVALUE lpColor)
{
    g_lpColor = lpColor;
}

//////////////////////////////////////////////////////////////////////
// Function: fxQuadSetInvertView
// Description:
//////////////////////////////////////////////////////////////////////
void fxQuadSetInvertView(piLPMATRIX lpInvertView)
{
    g_lpInvertView = lpInvertView;
}

//////////////////////////////////////////////
// Function: fxQuadFreeAll
// Description: Release all.
//////////////////////////////////////////////
void fxQuadFreeAll()
{
    FXQUAD* lpQuad = g_pclHead.next;
    FXQUAD* lpTail = &g_pclTail;
    FXQUAD* lpFree = NULL;

    if(lpQuad == NULL) return;

    while(lpQuad != lpTail)
    {
        lpFree = lpQuad;
        lpQuad = lpQuad->next;

        lpFree->bDraw = FALSE;

        _FreeIfNecessary(lpFree);
    }

    _quads.erase(_quads.begin(), _quads.end());
}

//////////////////////////////////////////////////////////////////////
// Function: _SetMaterial
// Description:
//////////////////////////////////////////////////////////////////////
void _SetMaterial(FXQUAD* lpQuad)
{
    LPMATERIAL lpMat = NULL;

    piGetMaterials(&lpMat);

    if(lpMat == NULL) return;

    SINT32 iMatID = fxMgrGetMaterial();

    lpMat[iMatID].material.ambient.a = lpQuad->piColor.a;
    lpMat[iMatID].material.ambient.r = lpQuad->piColor.r;
    lpMat[iMatID].material.ambient.g = lpQuad->piColor.g;
    lpMat[iMatID].material.ambient.b = lpQuad->piColor.b;

    lpMat[iMatID].material.diffuse.a = lpQuad->piColor.a;
    lpMat[iMatID].material.diffuse.r = lpQuad->piColor.r;
    lpMat[iMatID].material.diffuse.g = lpQuad->piColor.g;
    lpMat[iMatID].material.diffuse.b = lpQuad->piColor.b;

    piSetActiveMaterial(iMatID);
}

//////////////////////////////////////////////////////////////////////
// Function: fxQuadPopFlags
// Description:
//////////////////////////////////////////////////////////////////////
void fxQuadPopFlags(SINT32 iID, UINT32 uFlag)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_QUADS)
        return;

    g_lpQuads[iIndex].uFlags &= ~uFlag;
}

//////////////////////////////////////////////////////////////////////
// Function: fxQuadPushFlags
// Description:
//////////////////////////////////////////////////////////////////////
void fxQuadPushFlags(SINT32 iID, UINT32 uFlag)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_QUADS)
        return;

    g_lpQuads[iIndex].uFlags |= uFlag;
}

//////////////////////////////////////////////////////////////////////
// Function: fxQuadSet4PtAlpha
// Description:
//////////////////////////////////////////////////////////////////////
void fxQuadSet4PtAlpha(SINT32 iID, FLOAT32 p1, FLOAT32 p2, FLOAT32 p3, FLOAT32 p4)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_QUADS)
        return;

    g_lpQuads[iIndex].quadColor.p1.a = p1;
    g_lpQuads[iIndex].quadColor.p2.a = p2;
    g_lpQuads[iIndex].quadColor.p3.a = p3;
    g_lpQuads[iIndex].quadColor.p4.a = p4;

    g_lpQuads[iIndex].piColor.a = p1;
}

//////////////////////////////////////////////////////////////////////
// Function: fxQuadSet4PtScale
// Description:
//////////////////////////////////////////////////////////////////////
void fxQuadSet4PtScale(SINT32 iID, FLOAT32 p1, FLOAT32 p2, FLOAT32 p3, FLOAT32 p4)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_QUADS)
        return;

    g_lpQuads[iIndex].quadScale.p1 = p1;
    g_lpQuads[iIndex].quadScale.p2 = p2;
    g_lpQuads[iIndex].quadScale.p3 = p3;
    g_lpQuads[iIndex].quadScale.p4 = p4;
}

//////////////////////////////////////////////////////////////////////
// Function: fxQuadSetDragCoeff
// Description:
//////////////////////////////////////////////////////////////////////
void fxQuadSetDragCoeff(SINT32 iID, FLOAT32 fCoeff)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_QUADS)
        return;

    g_lpQuads[iIndex].dragco = fCoeff;
}

//////////////////////////////////////////////////////////////////////
// Function: fxQuadSetDOFObject
// Description:
//////////////////////////////////////////////////////////////////////
void fxQuadSetDOFObject(SINT32 iID, LPOBJECTDOF lpDOF)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_QUADS)
        return;

    g_lpQuads[iIndex].lpDOF = lpDOF;
}

//////////////////////////////////////////////////////////////////////
// Function: fxQuadSetEnergy
// Description:
//////////////////////////////////////////////////////////////////////
void fxQuadSetEnergy(SINT32 iID, FLOAT64 fEnergy)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_QUADS)
        return;

    g_lpQuads[iIndex].fEnergy = fEnergy;
}

//////////////////////////////////////////////////////////////////////
// Function: fxQuadSetPosition
// Description:
//////////////////////////////////////////////////////////////////////
void fxQuadSetPosition(SINT32 iID, FLOAT64 x, FLOAT64 y, FLOAT64 z)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_QUADS)
        return;

    g_lpQuads[iIndex].worldpos.x = x;
    g_lpQuads[iIndex].worldpos.y = y;
    g_lpQuads[iIndex].worldpos.z = z;
}

//////////////////////////////////////////////////////////////////////
// Function: fxQuadSetGroundPosition
// Description:
//////////////////////////////////////////////////////////////////////
void fxQuadSetGroundPosition(SINT32 iID, FLOAT64 x, FLOAT64 y, FLOAT64 z)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_QUADS)
        return;

    g_lpQuads[iIndex].groundpos.x = x;
    g_lpQuads[iIndex].groundpos.y = y;
    g_lpQuads[iIndex].groundpos.z = z;
}

//////////////////////////////////////////////////////////////////////
// Function: fxQuadSetPt1Color
// Description:
//////////////////////////////////////////////////////////////////////
void fxQuadSetPt1Color(SINT32 iID, FLOAT32 r, FLOAT32 g, FLOAT32 b)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_QUADS)
        return;

    g_lpQuads[iIndex].quadColor.p1.r = r;
    g_lpQuads[iIndex].quadColor.p1.g = g;
    g_lpQuads[iIndex].quadColor.p1.b = b;

    g_lpQuads[iIndex].piColor.r = r;
    g_lpQuads[iIndex].piColor.g = g;
    g_lpQuads[iIndex].piColor.b = b;
}

//////////////////////////////////////////////////////////////////////
// Function: fxQuadSetPt2Color
// Description:
//////////////////////////////////////////////////////////////////////
void fxQuadSetPt2Color(SINT32 iID, FLOAT32 r, FLOAT32 g, FLOAT32 b)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_QUADS)
        return;

    g_lpQuads[iIndex].quadColor.p2.r = r;
    g_lpQuads[iIndex].quadColor.p2.g = g;
    g_lpQuads[iIndex].quadColor.p2.b = b;
}

//////////////////////////////////////////////////////////////////////
// Function: fxQuadSetPt3Color
// Description:
//////////////////////////////////////////////////////////////////////
void fxQuadSetPt3Color(SINT32 iID, FLOAT32 r, FLOAT32 g, FLOAT32 b)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_QUADS)
        return;

    g_lpQuads[iIndex].quadColor.p3.r = r;
    g_lpQuads[iIndex].quadColor.p3.g = g;
    g_lpQuads[iIndex].quadColor.p3.b = b;
}

//////////////////////////////////////////////////////////////////////
// Function: fxQuadSetPt4Color
// Description:
//////////////////////////////////////////////////////////////////////
void fxQuadSetPt4Color(SINT32 iID, FLOAT32 r, FLOAT32 g, FLOAT32 b)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_QUADS)
        return;

    g_lpQuads[iIndex].quadColor.p4.r = r;
    g_lpQuads[iIndex].quadColor.p4.g = g;
    g_lpQuads[iIndex].quadColor.p4.b = b;
}

//////////////////////////////////////////////////////////////////////
// Function: fxQuadSetPt1Rotate
// Description:
//////////////////////////////////////////////////////////////////////
void fxQuadSetPt1Rotate(SINT32 iID, FLOAT32 x, FLOAT32 y, FLOAT32 z)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_QUADS)
        return;

    g_lpQuads[iIndex].quadRotate.p1.x = x;
    g_lpQuads[iIndex].quadRotate.p1.y = y;
    g_lpQuads[iIndex].quadRotate.p1.z = z;
}

//////////////////////////////////////////////////////////////////////
// Function: fxQuadSetPt2Rotate
// Description:
//////////////////////////////////////////////////////////////////////
void fxQuadSetPt2Rotate(SINT32 iID, FLOAT32 x, FLOAT32 y, FLOAT32 z)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_QUADS)
        return;

    g_lpQuads[iIndex].quadRotate.p2.x = x;
    g_lpQuads[iIndex].quadRotate.p2.y = y;
    g_lpQuads[iIndex].quadRotate.p2.z = z;
}

//////////////////////////////////////////////////////////////////////
// Function: fxQuadSetPt3Rotate
// Description:
//////////////////////////////////////////////////////////////////////
void fxQuadSetPt3Rotate(SINT32 iID, FLOAT32 x, FLOAT32 y, FLOAT32 z)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_QUADS)
        return;

    g_lpQuads[iIndex].quadRotate.p3.x = x;
    g_lpQuads[iIndex].quadRotate.p3.y = y;
    g_lpQuads[iIndex].quadRotate.p3.z = z;
}

//////////////////////////////////////////////////////////////////////
// Function: fxQuadSetPt4Rotate
// Description:
//////////////////////////////////////////////////////////////////////
void fxQuadSetPt4Rotate(SINT32 iID, FLOAT32 x, FLOAT32 y, FLOAT32 z)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_QUADS)
        return;

    g_lpQuads[iIndex].quadRotate.p4.x = x;
    g_lpQuads[iIndex].quadRotate.p4.y = y;
    g_lpQuads[iIndex].quadRotate.p4.z = z;
}

//////////////////////////////////////////////////////////////////////
// Function: fxQuadSetPt1Velocity
// Description:
//////////////////////////////////////////////////////////////////////
void fxQuadSetPt1Velocity(SINT32 iID, FLOAT32 x, FLOAT32 y, FLOAT32 z)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_QUADS)
        return;

    g_lpQuads[iIndex].quadVel.p1.x = x;
    g_lpQuads[iIndex].quadVel.p1.y = y;
    g_lpQuads[iIndex].quadVel.p1.z = z;
}

//////////////////////////////////////////////////////////////////////
// Function: fxQuadSetPt2Velocity
// Description:
//////////////////////////////////////////////////////////////////////
void fxQuadSetPt2Velocity(SINT32 iID, FLOAT32 x, FLOAT32 y, FLOAT32 z)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_QUADS)
        return;

    g_lpQuads[iIndex].quadVel.p2.x = x;
    g_lpQuads[iIndex].quadVel.p2.y = y;
    g_lpQuads[iIndex].quadVel.p2.z = z;
}

//////////////////////////////////////////////////////////////////////
// Function: fxQuadSetPt3Velocity
// Description:
//////////////////////////////////////////////////////////////////////
void fxQuadSetPt3Velocity(SINT32 iID, FLOAT32 x, FLOAT32 y, FLOAT32 z)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_QUADS)
        return;

    g_lpQuads[iIndex].quadVel.p3.x = x;
    g_lpQuads[iIndex].quadVel.p3.y = y;
    g_lpQuads[iIndex].quadVel.p3.z = z;
}

//////////////////////////////////////////////////////////////////////
// Function: fxQuadSetPt4Velocity
// Description:
//////////////////////////////////////////////////////////////////////
void fxQuadSetPt4Velocity(SINT32 iID, FLOAT32 x, FLOAT32 y, FLOAT32 z)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_QUADS)
        return;

    g_lpQuads[iIndex].quadVel.p4.x = x;
    g_lpQuads[iIndex].quadVel.p4.y = y;
    g_lpQuads[iIndex].quadVel.p4.z = z;
}

//////////////////////////////////////////////////////////////////////
// Function: fxQuadSetPt1Time
// Description:
//////////////////////////////////////////////////////////////////////
void fxQuadSetPt1Time(SINT32 iID, FLOAT32 fTime)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_QUADS)
        return;

    g_lpQuads[iIndex].quadTime.p1 = fTime;
}

//////////////////////////////////////////////////////////////////////
// Function: fxQuadSetPt2Time
// Description:
//////////////////////////////////////////////////////////////////////
void fxQuadSetPt2Time(SINT32 iID, FLOAT32 fTime)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_QUADS)
        return;

    g_lpQuads[iIndex].quadTime.p2 = fTime;
}

//////////////////////////////////////////////////////////////////////
// Function: fxQuadSetPt3Time
// Description:
//////////////////////////////////////////////////////////////////////
void fxQuadSetPt3Time(SINT32 iID, FLOAT32 fTime)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_QUADS)
        return;

    g_lpQuads[iIndex].quadTime.p3 = fTime;
}

//////////////////////////////////////////////////////////////////////
// Function: fxQuadSetPt4Time
// Description:
//////////////////////////////////////////////////////////////////////
void fxQuadSetPt4Time(SINT32 iID, FLOAT32 fTime)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_QUADS)
        return;

    g_lpQuads[iIndex].quadTime.p4 = fTime;
}

//////////////////////////////////////////////////////////////////////
// Function: fxQuadSetRadius
// Description:
//////////////////////////////////////////////////////////////////////
void fxQuadSetRadius(SINT32 iID, FLOAT32 radius)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_QUADS)
        return;

    g_lpQuads[iIndex].radius = radius;
}

//////////////////////////////////////////////////////////////////////
// Function: fxQuadSetScale
// Description:
//////////////////////////////////////////////////////////////////////
void fxQuadSetScale(SINT32 iID, FLOAT32 scale)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_QUADS)
        return;

    g_lpQuads[iIndex].scale = scale;
}

//////////////////////////////////////////////////////////////////////
// Function: fxQuadSetSpriteNumber
// Description:
//////////////////////////////////////////////////////////////////////
void fxQuadSetSpriteNumber(SINT32 iID, UINT32 uSpriteNum)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_QUADS)
        return;

    g_lpQuads[iIndex].uSpriteNum = uSpriteNum;
}

//////////////////////////////////////////////////////////////////////
// Function: fxQuadSetCanStretch
// Description:
//////////////////////////////////////////////////////////////////////
void fxQuadSetCanStretch(SINT32 iID, LOGICAL bStretch)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_QUADS)
        return;

    g_lpQuads[iIndex].bStretch = bStretch;
}

//////////////////////////////////////////////////////////////////////
// Function: fxQuadSetVelocity
// Description:
//////////////////////////////////////////////////////////////////////
void fxQuadSetVelocity(SINT32 iID, FLOAT32 x, FLOAT32 y, FLOAT32 z)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_QUADS)
        return;

    g_lpQuads[iIndex].vecVel.x = x;
    g_lpQuads[iIndex].vecVel.y = y;
    g_lpQuads[iIndex].vecVel.z = z;
}

//////////////////////////////////////////////////////////////////////
// Function: fxQuadSetRotate
// Description:
//////////////////////////////////////////////////////////////////////
void fxQuadSetRotate(SINT32 iID, FLOAT32 x, FLOAT32 y, FLOAT32 z)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_QUADS)
        return;

    g_lpQuads[iIndex].vecRot.x = x;
    g_lpQuads[iIndex].vecRot.y = y;
    g_lpQuads[iIndex].vecRot.z = z;
}

//////////////////////////////////////////////////////////////////////
// Function: fxQuadSetParticleID
// Description:
//////////////////////////////////////////////////////////////////////
void fxQuadSetParticleID(SINT32 iID, SINT32 iPclID)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_QUADS)
        return;

    g_lpQuads[iIndex].iPclID = iPclID;
}

//////////////////////////////////////////////////////////////////////
// Function: fxQuadSetUserKillCallback
// Description:
//////////////////////////////////////////////////////////////////////
void fxQuadSetUserKillCallback(SINT32 iID, void (*pFn)(void*))
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_QUADS)
        return;

    g_lpQuads[iIndex].pFnKill = pFn;
}

//////////////////////////////////////////////////////////////////////
// Function: fxQuadSetKillData
// Description:
//////////////////////////////////////////////////////////////////////
void fxQuadSetKillData(SINT32 iID, void* killData)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_QUADS)
        return;

    g_lpQuads[iIndex].killData = killData;
}

//////////////////////////////////////////////////////////////////////
// Function: fxQuadGetQuad
// Description:
//////////////////////////////////////////////////////////////////////
FXQUAD* fxQuadGetQuad(SINT32 iID)
{
    SINT32 iIndex = iID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_QUADS)
        return NULL;

    return &g_lpQuads[iIndex];
}

//////////////////////////////////////////////////////////////////////
// Function: fxQuadFixStretch
// Description: Stretch of particle is at fixed length.
//////////////////////////////////////////////////////////////////////
void fxQuadFixStretch(SINT32 ID)
{
    SINT32 iIndex = ID - 1;

    if(iIndex < 0 || iIndex >= DEF_MAX_QUADS)
        return;

    g_lpQuads[iIndex].stretchFixed = 1;
}
