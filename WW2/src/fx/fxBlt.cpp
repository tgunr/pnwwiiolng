#include <syscfg.h> // Path is project specific.
/////////////////////////////////////////////////////
// Module: fxBlt.cpp
// Description: Handle all bullet events.
/////////////////////////////////////////////////////
#include "fx/fxBlt.h"
#include "fx/fxSnd.h"
#include "fx/fxMgr.h"
#include "fx/fxPcl.h"
#include "fx/fxUtl.h"

#include "ml/mlutility.h"
#include "ld/ldpix.h"
#include "math/math3d.h"
#include "math/mathvect.h"
#include "math/mathnum.h"
#include "math/matheul.h"
#include "draw/draw3d.h"
#include "trn/trnmain.h"
#include "rn/rnmain.h"
#include "rn/rnvis.h"
#include "ob/obdlite.h"
#include "fio/fio.h"
#include "pers/persmain.h"
#include "ob/obpool.h"
#include "ogrp/ogrptype.h"
#include "math/mathUnitConversion.h"
#include "trn/trnwtr.h"
#include "spt/Random.h"
#include "fx/TransformEffect.h"
#include "trn/trnvars.h"
#include "trn/trnScenario.h"
#include "ob/obMgr.h"

#include <algorithm>

#define MAX_NUM_OF_COLLISIONS   32
#define MAX_DECALS 128

#define GROUND_BLOOD_ANIM_SCALE 0.1f // How fast pool spreads

static FXBULLET* g_blts = NULL;

static std::multimap<void* , SINT32> g_mappedBullets;
static std::vector<FXBULLET*> drawlist;

static FXBULLET _freeH;
static FXBULLET _freeT;

static FXBULLET g_bltHead;
static FXBULLET g_bltTail;

static SINT32 g_iNumMat = 0;

static piCOLORVALUE* g_lpColor = NULL;

static SINT32 _SortBackToFront(FXBULLET* l, FXBULLET* r);
static void _UnMap(void* pData);
static void _DrawDecals();
static void _clear(FXBULLET* b);

#if USE_STATESET
static ref_ptr<RenderStateSet> stateset;
#endif

#define USE_FX2 1

////////////////////////////////////
// Function: fxBltInit
// Description: Initialize bullets.
////////////////////////////////////
LOGICAL fxBltInit()
{
    SINT32 i;
    MATERIAL* lpMat = NULL;

    g_blts = new FXBULLET[MAX_DECALS];

    if(g_blts == NULL)
        return FALSE;

    g_iNumMat = piGetMaterials(&lpMat);

    memset(&g_bltHead, 0, sizeof(FXBULLET));
    memset(&g_bltTail, 0, sizeof(FXBULLET));

    g_bltHead.next = &g_bltTail;
    g_bltTail.prev = &g_bltHead;

    memset(&_freeH, 0, sizeof(FXBULLET));
    memset(&_freeT, 0, sizeof(FXBULLET));

    _freeH.next = &_freeT;
    _freeT.prev = &_freeH;

    for(i = 0; i < MAX_DECALS; i++)
    {
        _clear(&g_blts[i]);

        g_blts[i].index = i;

        fxBltFreeBullet(&g_blts[i]);
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
// Function: _clear
// Description:
//////////////////////////////////////////////
inline void _clear(FXBULLET* b)
{
    b->isStreak = 0;
    b->isWater = 0;

    b->lpVehAttach = NULL;
    b->o = NULL;
    b->lpTop = NULL;

    b->uVehicleType = 0;
    b->uAmmoSubType = 0;

    b->collidepoly = 0;
    b->penetrate = 0;

    b->type = IDE_BULLET_CONCRETE;

    b->radius = 0.0f;
    b->dis2 = 0.0f;
    b->starttime = 0;
    b->lastupdate = 0;
    b->scale = 0.0f;

    memset(b->original, 0, sizeof(b->original));
    memset(b->decal, 0, sizeof(b->decal));
}

//////////////////////////////////////////////
// Function: fxBltFreeAll
// Description: Release all bullets
//////////////////////////////////////////////
void fxBltFreeAll()
{
    FXBULLET* blt = g_bltHead.next;
    FXBULLET* lpTail = &g_bltTail;
    FXBULLET* lpFree = NULL;

    if(blt == NULL) return;

    while(blt != lpTail)
    {
        lpFree = blt;
        blt = blt->next;

        fxBltReleaseBullet(lpFree);
        fxBltFreeBullet(lpFree);
    }

    drawlist.clear();
}

//////////////////////////////////////////////
// Function: fxBltKill
// Description: Release all
//////////////////////////////////////////////
LOGICAL fxBltKill()
{
    if(g_blts != NULL)
    {
        delete [] g_blts;
        g_blts = NULL;
    }

    // Set to NULL in case other bullet update routines are called.
    memset(&g_bltHead, 0, sizeof(FXBULLET));
    memset(&g_bltTail, 0, sizeof(FXBULLET));
    memset(&_freeH, 0, sizeof(FXBULLET));
    memset(&_freeT, 0, sizeof(FXBULLET));

    return TRUE;
}

//////////////////////////////////////////////
// Function: fxBltCreate
// Description: Add a bullet to the free list.
//////////////////////////////////////////////
LOGICAL fxBltCreate(FXBULLETCREATE* create)
{
    if(PN_UNLIKELY(create == NULL))
        return FALSE;

	return fxBltCreateBulletDecal(create);
}

//////////////////////////////////////////////////////////////////////
// Function: fxBltCreateBulletDecal
// Description: Top level create for the actual bullet.
//////////////////////////////////////////////////////////////////////
LOGICAL fxBltCreateBulletDecal(FXBULLETCREATE* create)
{
	if (PN_UNLIKELY(create == NULL))
		return FALSE;

	bool blood = create->flags & (blt_decal_is_blood_splat | blt_decal_is_ground_blood);

    if(blood)
    {
        if(!WantBloodEffects)
            return FALSE;
    }

    FXBULLET* blt = NULL;
    if(!fxBltGetBullet(&blt))
        return FALSE;

    if(!fxBltAddBullet(blt))
        return FALSE;

    if(!fxBltSetBltValues(blt, create))
    {
        // Throw up hit smoke, even if texture placement failed.
        if(!blood && !blt->isWater && blt->uAmmoSubType != WEAPAMMO_SUBTYPE_KNIFE)
        {
            fxBltCreateSmoke(blt);
        }

        fxBltReleaseBullet(blt);
        fxBltFreeBullet(blt);

        return FALSE;
    }

    if(!blt->isWater && blt->uAmmoSubType != WEAPAMMO_SUBTYPE_KNIFE && !blood)
    {
        fxBltCreateSmoke(blt);
    }

    if(!blood)
        fxBltPlayHitSound(blt);

    if(blt->isWater)
    {
        // Don't need this anymore, since it's a water hit.
        fxBltReleaseBullet(blt);
        fxBltFreeBullet(blt);
    }

    if(blt->uAmmoSubType == WEAPAMMO_SUBTYPE_KNIFE)
    {
        fxBltReleaseBullet(blt);
        fxBltFreeBullet(blt);
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: fxBltCreateSmoke
// Description: Create smoke sprite for bullet hit.
//////////////////////////////////////////////////////////////////////
LOGICAL fxBltCreateSmoke(FXBULLET* b)
{
    if(b->o == NULL)
        return FALSE;

    if(!b->o->psnorms)
        return FALSE;

    piVECTOR n;

    n.x = b->o->psnorms[b->collidepoly].x;
    n.y = b->o->psnorms[b->collidepoly].y;
    n.z = b->o->psnorms[b->collidepoly].z;

    math3dMultiplyRotation(&n, &n, &b->o->mat);

    const char* effect = NULL;

    if(gCustomTerrain && b->o && b->o->modelInstance)
    {
        if(b->o->modelInstance->GetModelType() == ModelTypes::TERRAIN_TILE)
        {
            effect = (char*)TerrainScenario::GetInstance().GetMaterial(b->worldpos.x, b->worldpos.y, b->o->soundID);

            if(effect)
                effect = fxUtlGetBulletEffectFromMaterial(effect);
        }
    }

    if(!effect)
        effect = fxUtlGetBulletEffectFromMaterial(b->o->matname);

    TransformEffect* te = NULL;

    if(effect)
    {
        te = TransformEffect::AddEffect(effect, Vec3f(n), 0, NULL);
    }
    else
    {
        te = TransformEffect::AddEffect("genericbullethit", Vec3f(n), 0, NULL);
    }

    if(te)
    {
        Vec3d p(b->worldpos);
        p.z() = -p.z();

        te->enableemitters(true);;
        te->setmatrix(Matrixf::translate(p));
        te->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: fxBltSetBltValues
// Description: Set bullet original values.
//////////////////////////////////////////////////////////////////////
LOGICAL fxBltSetBltValues(FXBULLET* blt, FXBULLETCREATE* create)
{
    blt->starttime = frametime;
    blt->lastupdate = frametime;
    blt->radius = create->fRadius;
    blt->collidepoly = create->iPolyNum;
    blt->penetrate = create->penetrate;
    blt->uAmmoSubType = create->uAmmoSubType;
    blt->flags = create->flags;

    if(create->pVehicle != NULL)
    {
        blt->lpVehAttach = (vehVehicle*)create->pVehicle;
        blt->uVehicleType = blt->lpVehAttach->VehicleType->Type;
    }
    else
    {
        blt->lpVehAttach = NULL;
    }

    blt->o = create->lpAttachedObj;
    blt->lpTop = create->lpTopLevelObj;

    if(!fxBltGetObjectForBullet(blt, blt->o))
        return FALSE;

    blt->worldpos = create->vecPos;

    blt->velocity.x = (FLOAT32)create->vecVel.x;
    blt->velocity.y = (FLOAT32)create->vecVel.y;
    blt->velocity.z = (FLOAT32)create->vecVel.z;

    fxUtlUpdatePosition(&blt->relpos, &blt->worldpos);

    if(blt->o)
    {
        // No bullet hits on flags
        if(blt->o->flag2 & OBJECT_FLAG)
        {
            return FALSE;
        }
    }

    if((blt->isWater = fxBltIsWater(blt)) == TRUE)
        return TRUE;

    if(blt->uAmmoSubType != WEAPAMMO_SUBTYPE_KNIFE)
    {
        if(!fxBltClip(blt, &create->vecDelta))
            return FALSE;
    }

    if(blt->flags & blt_decal_is_ground_blood)
    {
        blt->scale = 0.1f;
    }

    if(!fxBltGetBulletTexture(blt))
        return FALSE;

    if(blt->uAmmoSubType != WEAPAMMO_SUBTYPE_KNIFE)
    {
        float x = create->vecDelta.x * 0.005f;
        float y = create->vecDelta.y * 0.005f;
        float z = create->vecDelta.z * 0.005f;

        blt->original[0].x -= x;
        blt->original[0].y -= y;
        blt->original[0].z -= z;
        blt->original[1].x -= x;
        blt->original[1].y -= y;
        blt->original[1].z -= z;
        blt->original[2].x -= x;
        blt->original[2].y -= y;
        blt->original[2].z -= z;
        blt->original[3].x -= x;
        blt->original[3].y -= y;
        blt->original[3].z -= z;
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: fxBltGetObjectForBullet
// Description: Make sure we have the right object for the bullet placement.
//////////////////////////////////////////////////////////////////////
LOGICAL fxBltGetObjectForBullet(FXBULLET* blt, OBJECT* o)
{
    OBJECT* lpRetObj = fxUtlGetObject(o);

    blt->o = lpRetObj;

    if(blt->o)
    {
        blt->o->flag2 |= OBJECT_HAS_FX;
    }

    typedef std::pair<void* , SINT32> ins;

    blt->mapIter = g_mappedBullets.insert(ins((void*)o, blt->index + 1));

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: fxBltCreateRotationMatrix
// Description: Create matrix for rotation of texture.
//////////////////////////////////////////////////////////////////////
LOGICAL fxBltCreateRotationMatrix(FXBULLET* blt,
                                  piMATRIX* lpMat,
                                  piVECTOR* pvecShot)
{
    piVECTOR u, v;

    ////////////////////////////////////////////////////////////////
    // If angle of shot is >= 45 degrees, display richochet texture.
    ////////////////////////////////////////////////////////////////
    v = *pvecShot;

    v.x = -v.x;
    v.y = -v.y;
    v.z = -v.z;

    u = blt->o->psnorms[blt->collidepoly];

    blt->n = u;

    math3dVectorMultiply(&u, &u, &blt->o->mat);

    u.x -= blt->o->mat.m[3][0];
    u.y -= blt->o->mat.m[3][1];
    u.z -= blt->o->mat.m[3][2];

    u.x = -u.x;
    u.y = -u.y;
    u.z = -u.z;

    LOGICAL isGlass = FALSE;

    if(blt->o->matid != -1)
    {
        if(blt->o->matname[0])
        {
            if(strstr(blt->o->matname, "glas"))
            {
                isGlass = TRUE;
            }
        }
    }

    if(isGlass)
    {
        // Do nothing
    }
    else
    {
        FLOAT32 costheta = math3dDotProduct(&v, &u);
        costheta = costheta < -1.0f ? -1.0f : costheta > 1.0f ? 1.0f : costheta;

        float angle = PI - (FLOAT32)acos(costheta);

        if(angle >= PI * 0.25f && !(blt->flags & blt_decal_is_blood_splat))
            blt->isStreak = TRUE;
    }

    blt->vecAtt.Roll = 0.0f;
    blt->vecAtt.Yaw = atan2(v.x, v.y);
    blt->vecAtt.Pitch = atan2(-sqrt(v.y * v.y + v.x * v.x), -v.z);

    piVECTOR vecAtt;

    vecAtt.x = (FLOAT32)blt->vecAtt.Pitch;
    vecAtt.y = (FLOAT32)blt->vecAtt.Roll;
    vecAtt.z = (FLOAT32)blt->vecAtt.Yaw;

    mathEul_ToMatrix(lpMat, EulOrdXYZs, &vecAtt);

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: fxBltClip
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL fxBltClip(FXBULLET* blt, piVECTOR* pvecShot)
{
    piVECTOR v1, v2, v3, v4, v;

    if(blt->uVehicleType == VEH_TYPE_PERSON)
        return TRUE;

    FASTCOLLIDE cldArray[MAX_NUM_OF_COLLISIONS + 2];

    piMATRIX matAtt = IdentityMatrix;

    FLOAT32 fDistance = 1.0f;

    UINT32 uNumCollisions = 0;

    if(!fxBltCreateRotationMatrix(blt, &matAtt, pvecShot))
    {
        Write_Debug("[fxBltClip] fxBltCreateRotationMatrix** FAILED**");
        return FALSE;
    }

    FLOAT32 radius = 0.0f;

    if(blt->isStreak)
    {
        radius = blt->radius;
    }
    else
    {
        radius = blt->radius * 2.0f;
        blt->radius = radius;
    }

    v1.x =  radius; v1.y =  radius; v1.z = fDistance;
    v2.x =  radius; v2.y = -radius; v2.z = fDistance;
    v3.x = -radius; v3.y = -radius; v3.z = fDistance;
    v4.x = -radius; v4.y =  radius; v4.z = fDistance;

    ///////////////////////////////////
    // Random rotate here if necessary.
    ///////////////////////////////////

    v = v1; v.z = -v.z;

    fxBltSetCollisionVectors(&v1, &v1, &matAtt, blt);
    fxBltSetCollisionVectors(&v, &v, &matAtt, blt);

    if(!fxBltCollideObject(&v1, &v, blt, &uNumCollisions, cldArray))
    {
        return FALSE;
    }

    if(!fxBltHandleCollision(cldArray, uNumCollisions, blt, 0))
    {
        return FALSE;
    }

    v = v2; v.z = -v.z;

    fxBltSetCollisionVectors(&v2, &v2, &matAtt, blt);
    fxBltSetCollisionVectors(&v, &v, &matAtt, blt);

    if(!fxBltCollideObject(&v2, &v, blt, &uNumCollisions, cldArray))
    {
        return FALSE;
    }

    if(!fxBltHandleCollision(cldArray, uNumCollisions, blt, 1))
    {
        return FALSE;
    }

    v = v3; v.z = -v.z;

    fxBltSetCollisionVectors(&v3, &v3, &matAtt, blt);
    fxBltSetCollisionVectors(&v, &v, &matAtt, blt);

    if(!fxBltCollideObject(&v3, &v, blt, &uNumCollisions, cldArray))
    {
        return FALSE;
    }

    if(!fxBltHandleCollision(cldArray, uNumCollisions, blt, 2))
    {
        return FALSE;
    }

    v = v4;v.z = -v.z;

    fxBltSetCollisionVectors(&v4, &v4, &matAtt, blt);
    fxBltSetCollisionVectors(&v, &v, &matAtt, blt);

    if(!fxBltCollideObject(&v4, &v, blt, &uNumCollisions, cldArray))
    {
        return FALSE;
    }

    if(!fxBltHandleCollision(cldArray, uNumCollisions, blt, 3))
    {
        return FALSE;
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: fxBltSetCollisionVectors
// Description: Set vectors for old and new vector positions.
//////////////////////////////////////////////////////////////////////
LOGICAL fxBltSetCollisionVectors(piVECTOR* pv1,
                                 piVECTOR* pv2,
                                 piMATRIX* lpMat,
                                 FXBULLET* blt)
{
    math3dVectorMultiply(pv1, pv2, lpMat);
    mathAddVectors(pv1, pv2, &blt->relpos);

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: fxBltCollideObject
// Description: Can't avoid this if we don't want the overlap
// of bullet holes on windows, off edges, etc...
//////////////////////////////////////////////////////////////////////
LOGICAL fxBltCollideObject(piVECTOR* pV1,
                           piVECTOR* pV2,
                           FXBULLET* blt,
                           UINT32* pNumCls,
                           FASTCOLLIDE cldArray[])
{
    FASTCOLLIDE* pList = NULL;

    UINT32 uNumCollisions = MAX_NUM_OF_COLLISIONS;

    pList = cldArray;

    weapCollideWithHitmapObject(&pList,
                                &uNumCollisions,
                                blt->o,
                                blt->o,
                                pV2,
                                pV1,
                                0,
                                1);

    uNumCollisions = MAX_NUM_OF_COLLISIONS - uNumCollisions;

    if(uNumCollisions == 0)
        return FALSE;

    *pNumCls = uNumCollisions;

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: fxBltHandleCollision
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL fxBltHandleCollision(FASTCOLLIDE cldArray[],
                             UINT32 uNumCollisions,
                             FXBULLET* blt,
                             SINT32 iIndex)
{
    UINT32 i;

    mlUtilitySortCollideList(cldArray, uNumCollisions);

    for(i = 0; i < uNumCollisions; i++)
    {
        if(cldArray[i].Alpha > 0.5f)
        {
            fxBltSetBulletVector(blt, &cldArray[i], iIndex);
            return TRUE;
        }
    }

    ////////////////////////////////////////////////////////////////////
    // If we are here, then one of the collisions hit an 'alpha window'.
    ////////////////////////////////////////////////////////////////////

    return FALSE;
}

//////////////////////////////////////////////////////////////////////
// Function: fxBltSetBulletVector
// Description: Set the bullet vector if we have collided. Called from
// fxBltHandleCollision.
//////////////////////////////////////////////////////////////////////
LOGICAL fxBltSetBulletVector(FXBULLET* blt, FASTCOLLIDE* lpCld, SINT32 i)
{
    if(blt->lpVehAttach == NULL)
    {
        math3dVectorMultiply(&blt->original[i],
                             &lpCld->CollidePnt,
                             &lpCld->CollideObject->mat);
    }
    ///////////////////////////////////
    // Update vehicle bullets on draw.
    ///////////////////////////////////
    else
    {
        mathAddVectors(&blt->original[i],
                       &blt->original[i],
                       &lpCld->CollidePnt);
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: fxBltPlayHitSound
// Description: Play sound associated with texture.
//////////////////////////////////////////////////////////////////////
LOGICAL fxBltPlayHitSound(FXBULLET* bullet)
{
    if(bullet->uVehicleType == VEH_TYPE_PERSON)
        return FALSE;

    static SCHAR szName[128];
	szName[0] = 0;

    FLOAT64VECTOR vPos;

    vPos.x = bullet->relpos.x + WorldOriginOffset.x;
    vPos.y = bullet->relpos.y + WorldOriginOffset.y;
    vPos.z = (-bullet->relpos.z);

    if(!fxBltGetHitSound(bullet, szName))
        return FALSE;

    SINT32 id = fxSndGetNewSound();

    if(id == -1)
        return FALSE;

    fxSndSetSoundName(id, szName);
    fxSndSetPosition(id, vPos.x, vPos.y, vPos.z);
    fxSndSetPriority(id, IDE_TP_PRIORITY_LOW);
    fxSndSetSoundType(id, TYPE_FINITE_EXTERNAL_SOUND);
    fxSndSetVolume(id, 1.0f);
    fxSndSetPlayType(id, DEF_PLAYTYPE_WORLD);
    fxSndSetMaxDistance(id, 400.0f);
    fxSndSetMinDistance(id, 10.0f);

    if(bullet->uAmmoSubType == WEAPAMMO_SUBTYPE_KNIFE)
    {
        fxSndSetPitch(id, 1.0f);
    }
    else
    {
        fxSndSetPitch(id, GetRandom(0.8f, 1.0f));
    }

    fxSndInitSound(id);

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: fxBltGetHitSound
// Description: Get the name of the bullet hit sound.
//////////////////////////////////////////////////////////////////////
LOGICAL fxBltGetHitSound(FXBULLET* blt, SCHAR hitname[])
{
    SCHAR* hitsound = NULL;

    if(blt->uAmmoSubType == WEAPAMMO_SUBTYPE_KNIFE)
	{
        strncpy(hitname, "sounds\\genericknife.wav", 79);
        return TRUE;
    }

    if(!blt->o)
    {
        return FALSE;
    }

    if(gCustomTerrain && blt->o && blt->o->modelInstance)
    {
        if(blt->o->modelInstance->GetModelType() == ModelTypes::TERRAIN_TILE)
        {
            hitsound = (char*)TerrainScenario::GetInstance().GetMaterial(blt->worldpos.x, blt->worldpos.y, blt->o->soundID);

            char* sound;

            if(fxUtlGetRandomHitSound(hitsound, &sound, NULL, NULL))
            {
                strncpy(hitname, sound, 79);
                return true;
            }
        }
    }

    if(hitsound)
    {
        float trash;

        mlGetRandomHitSound(&hitsound,
                            &trash,
                            &trash,
                            hitsound);

        if(hitsound)
        {
            strncpy(hitname, hitsound, 79);
            return true;
        }

        return false;
    }

    if(blt->isWater)
    {
        hitsound = "sounds\\hitwater1.wav";
    }

    if(hitsound != NULL)
    {
        if(!hitsound[0])
        {
            return FALSE;
        }

        strncpy(hitname, hitsound, 79);

        return TRUE;
    }
    else if(blt->o->texname[0])
    {
        // One last try the old way...using grounds.csv
        FLOAT32 trash;

        mlGetRandomHitSound(&hitsound,
                            &trash,
                            &trash,
                            blt->o->texname);

        if(hitsound)
        {
            if(!hitsound[0])
            {
                return FALSE;
            }

            strncpy(hitname, hitsound, 79);

            return TRUE;
        }
    }

    return FALSE;
}

//////////////////////////////////////////////////////////////////////
// Function: fxBltPlaySound
// Description: Play hit sound.
//////////////////////////////////////////////////////////////////////
bool fxBltPlaySound(FLOAT64VECTOR* pos, const OBJECT* o)
{
    if(!o)
        return false;

    SCHAR* sound = NULL;

    if(oglIsMaterial(o->matid))
    {
        // Valid matid does not imply valid o->matname
        MATERIAL* mat;
        piGetMaterials(&mat);

        fxUtlGetRandomHitSound(mat[o->matid].name,
                               &sound,
                               NULL,
                               NULL);
    }
    else
    {
        FLOAT32 trash;

        mlGetRandomHitSound(&sound,
                            &trash,
                            &trash,
                            o->texname);
    }

    if(!sound)
        return false;

    int id = fxSndGetNewSound();

    if(id == -1)
    {
        return false;
    }

    fxSndSetSoundName(id, sound);
    fxSndSetPosition(id, pos->x, pos->y, pos->z);
    fxSndSetPriority(id, IDE_TP_PRIORITY_LOW);
    fxSndSetSoundType(id, TYPE_FINITE_EXTERNAL_SOUND);
    fxSndSetVolume(id, 1.0f);
    fxSndSetPitch(id, 0.8f + 0.2f * (mathGetFloat32ChaoticNumber() * 2.0f - 1.0f));
    fxSndSetPlayType(id, DEF_PLAYTYPE_WORLD);
    fxSndSetMaxDistance(id, 50.0f);
    fxSndSetMinDistance(id, 10.0f);

    if(!fxSndInitSound(id))
    {
        return false;
    }

    return true;
}

//////////////////////////////////////////////////////////////////////
// Function: fxBltDraw
// Description: Draw bullets.
//////////////////////////////////////////////////////////////////////
LOGICAL fxBltDraw()
{
    if(drawlist.empty())
        return TRUE;

    oglSetWorldMatrix(&IdentityMatrix);

#if USE_STATESET
    stateset->Apply();

    std::vector<FXBULLET*>::iterator itr = drawlist.begin();

    FXBULLET* blt;

    for(; itr != drawlist.end(); ++itr)
    {
        blt = (*itr);

        if(blt->lpVehAttach != NULL)
        {
            fxBltUpdateVehicleBullet(blt);
        }
    }

    _DrawDecals();

#else
    piDisableMaterial();

    std::vector<FXBULLET*>::iterator B = drawlist.begin();
    std::vector<FXBULLET*>::iterator E = drawlist.end();

    FXBULLET* blt;

    for(B; B != E; ++B)
    {
        blt = (*B);

        if(blt->lpVehAttach != NULL)
        {
            fxBltUpdateVehicleBullet(blt);
        }
    }

    piSetRenderState(piRENDERSTATE_ZWRITEENABLE, FALSE);
    piSetRenderState(piRENDERSTATE_LIGHTING, FALSE);

    _DrawDecals();

    piSetRenderState(piRENDERSTATE_ZWRITEENABLE, bZWrite);
    piSetRenderState(piRENDERSTATE_LIGHTING, piLIGHTING_DEFAULT);

#endif // USE_STATESET

    // DEBUG
#if(0)
    B = drawlist.begin();
    E = drawlist.end();

    for(B; B != E; ++B)
    {
        blt = (*B);

        if(blt->lpVehAttach != NULL)
        {
            piVECTOR p1 = blt->decal[0];
            piVECTOR p2 = p1;
            p2.x -= 10.0f;

            draw3dLine(&p1, &p2, colorBLUEALPHA);

            p1 = blt->relpos;
            p2 = p1;
            p2.z -= 10.0f;
            draw3dLine(&p1, &p2, colorREDALPHA);
        }
    }
#endif

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: _DrawDecals
// Description:
//////////////////////////////////////////////////////////////////////
void _DrawDecals()
{
    FXBULLET* blt = NULL;

    // 128 max * 4
    static piLVERTEX lv[512];
    static piINDICES indices[256];

    SINT16 i = 0;
    SINT16 q = 0;

    UINT32 color = piRGBA(g_lpColor->r,
                          g_lpColor->g,
                          g_lpColor->b,
                          g_lpColor->a);

    std::vector<FXBULLET*>::iterator itr = drawlist.begin();

    for(; itr != drawlist.end(); ++itr)
    {
        blt = (*itr);

        lv[i + 0].color = color;
        lv[i + 0].specular = colorALPHA100;
        lv[i + 0].x = blt->decal[0].x;
        lv[i + 0].y = blt->decal[0].y;
        lv[i + 0].z = blt->decal[0].z;
        lv[i + 0].tu = blt->uv[0].u;
        lv[i + 0].tv = blt->uv[1].v;

        lv[i + 1].color = color;
        lv[i + 1].specular = colorALPHA100;
        lv[i + 1].x = blt->decal[1].x;
        lv[i + 1].y = blt->decal[1].y;
        lv[i + 1].z = blt->decal[1].z;
        lv[i + 1].tu = blt->uv[0].u;
        lv[i + 1].tv = blt->uv[0].v;

        lv[i + 2].color = color;
        lv[i + 2].specular = colorALPHA100;
        lv[i + 2].x = blt->decal[2].x;
        lv[i + 2].y = blt->decal[2].y;
        lv[i + 2].z = blt->decal[2].z;
        lv[i + 2].tu = blt->uv[1].u;
        lv[i + 2].tv = blt->uv[0].v;

        lv[i + 3].color = color;
        lv[i + 3].specular = colorALPHA100;
        lv[i + 3].x = blt->decal[3].x;
        lv[i + 3].y = blt->decal[3].y;
        lv[i + 3].z = blt->decal[3].z;
        lv[i + 3].tu = blt->uv[1].u;
        lv[i + 3].tv = blt->uv[1].v;

        indices[q + 0].v1 = i + 0;
        indices[q + 0].v2 = i + 1;
        indices[q + 0].v3 = i + 2;

        indices[q + 1].v1 = i + 2;
        indices[q + 1].v2 = i + 3;
        indices[q + 1].v3 = i + 0;

        i += 4;
        q += 2;
    }

    ogl::SetVertexPointer(3, GL_FLOAT, sizeof(piLVERTEX), &lv[0].x);
    ogl::SetTextureUnitCoordPointer(GL_TEXTURE0, 2, GL_FLOAT, sizeof(piLVERTEX), &lv[0].tu);
    ogl::SetColorPointer(4, GL_UNSIGNED_BYTE, sizeof(piLVERTEX), &lv[0].color);

    ogl::DrawElements(GL_TRIANGLES, q * 3, GL_UNSIGNED_SHORT, indices);
}

//////////////////////////////////////////////////////////////////////
// Function: fxBltUpdateVehicleBullet
// Description: Update position of bullet texture on a vehicle.
//////////////////////////////////////////////////////////////////////
LOGICAL fxBltUpdateVehicleBullet(FXBULLET* blt)
{
    if(blt == NULL)
        return FALSE;

    if(blt->lpVehAttach == NULL)
        return FALSE;

    if(blt->lpVehAttach->VehicleObjectData == NULL)
        return FALSE;

    weapUpdateVehicleHitmap(blt->lpVehAttach);

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: fxBltIsWater
// Description: Since we use a #@@#2#$ collide object for non-replacement
// collisions, it does not contain the material id. The id cannot be
// copied, since the collide object may represent different material objects
//////////////////////////////////////////////////////////////////////
LOGICAL fxBltIsWater(FXBULLET* b)
{
    if(b->isWater)
    {
        return TRUE;
    }

    if(b->o == NULL)
    {
        return FALSE;
    }

    // Check material first
    static SINT32 water1;
    static SINT32 water2;
    static bool firsttime = true;

    if(firsttime)
    {
        water1 = piFindMaterialName("water1");
        water2 = piFindMaterialName("dkwater1");

        firsttime = false;
    }

    if(piIsMaterial(b->o->matid))
    {
        if(b->o->matid == water1 || b->o->matid == water2)
        {
            return TRUE;
        }
    }

    // Legacy crap for determining water
    FLOAT32 rollcoeff = 0.0f;
    vehVehicle*veh = persGetPersonaVehiclePtr();

    mlGetObjectBumpinessAndFriction(NULL, NULL, NULL, NULL, &rollcoeff, NULL, b->o);

    if(veh != NULL && veh->VehicleType != NULL)
    {
        if(veh->VehicleType->Type == VEH_TYPE_SEA)
        {
            if(rollcoeff > 0.999f && rollcoeff < 1.001f)
            {
                return TRUE;
            }
        }
        else if(rollcoeff > 198.99f && rollcoeff < 199.001f)
        {
            return TRUE;
        }
    }
    else if(rollcoeff > 198.99f && rollcoeff < 199.001f)
    {
        return TRUE;
    }

    return FALSE;
}

//////////////////////////////////////////////////////////////////////
// Function: fxBltGetBulletTexture
// Description: Test for bullet hitting water else get bullet texture.
//////////////////////////////////////////////////////////////////////
LOGICAL fxBltGetBulletTexture(FXBULLET* blt)
{
    if(blt->isWater)
        return TRUE;

    if(blt->uVehicleType == VEH_TYPE_PERSON)
        return TRUE;

    if(blt->flags & blt_decal_is_blood_splat)
    {
        blt->type = BLOOD_SPAT_DECAL;
    }
    else if(blt->flags & blt_decal_is_ground_blood)
    {
        blt->type = GROUND_BLOOD_DECAL;
    }
    else if(piIsMaterial(blt->o->matid))
    {
        // Nice to find out that even if an object has a valid matid, the name
        // may be blank! So, get the name the old fashion way. @#!@$!@
        MATERIAL* mat;
        piGetMaterials(&mat);

        SINT32 texture = fxUtlGetBulletType(mat[blt->o->matid].name);

        if(texture <= 0)
        {
            texture = 1;
        }

        blt->type = (ENUMBULLETTEXTURES)(texture - 1);

        if(blt->penetrate && blt->type == IDE_BULLET_METALDENT)
        {
            blt->type = IDE_BULLET_METALHOLE;
        }
    }
    else
    {
        blt->type = IDE_BULLET_WALLHOLE;
    }

    if(blt->isStreak && !blt->penetrate)
    {
        fxUtlGetTextureUV(blt->uv, "metaldent1");
    }
    else
    {
        static char buff[IDE_MAX_BULLET_TEXTURES][32] =
        {
            "concrete bullet hole",
            "dirtbullethit",
            "metaldent1",
            "metalhole",
            "woodbullethit",
            "metaldent1",
            "glassbullethit",
            "bloodbullethit",
            "error",
            "corpse ground blood"
        };

        static char buff2[3][16] =
        {
            "blood spurt 1",
            "blood spurt 2",
            "blood spurt 3"
        };

        if(blt->type != BLOOD_SPAT_DECAL)
        {
            fxUtlGetTextureUV(blt->uv, buff[blt->type]);
        }
        else
        {
            fxUtlGetTextureUV(blt->uv, buff2[GetRandom(0, 2)]);
        }
    }

    return TRUE;
}

//////////////////////////////////////////////
// Function: fxBltFreeBullet
// Description: Add a bullet to the free list.
//////////////////////////////////////////////
LOGICAL fxBltFreeBullet(FXBULLET* blt)
{
    FXBULLET* lpPrev = NULL;

    if(blt == NULL)
        return FALSE;

    if(blt->o)
    {
        g_mappedBullets.erase(blt->mapIter);
    }

    _clear(blt);

    lpPrev = _freeT.prev;

    blt->prev = lpPrev;
    blt->next = &_freeT;

    lpPrev->next = blt;
    _freeT.prev = blt;

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: fxBltAddBullet
// Description: Add a bullet to the active list. This function is called after
// a successful call to fxBltGetBullet.
//////////////////////////////////////////////////////////////////////
LOGICAL fxBltAddBullet(FXBULLET* blt)
{
    FXBULLET* lpNext = NULL;

    if(blt == NULL)
        return FALSE;

    lpNext = g_bltHead.next;

    g_bltHead.next = blt;
    blt->prev = &g_bltHead;

    blt->next = lpNext;
    lpNext->prev = blt;

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: fxBltGetBullet
// Description: Get a bullet off the free list. If we are out of bullets, it will
// take the oldest bullet off the active list.
//////////////////////////////////////////////////////////////////////
LOGICAL fxBltGetBullet(FXBULLET** blt)
{
    FXBULLET* lpNode = NULL;
    FXBULLET* lpNext = NULL;
    FXBULLET* lpPrev = NULL;

    if(blt == NULL)
        return FALSE;

    if(!fxBltCheckForBullets())
        return FALSE;

    lpNode = _freeH.next;

    lpPrev = lpNode->prev;
    lpNext = lpNode->next;

    lpPrev->next = lpNext;
    lpNext->prev = lpPrev;

    lpNode->next = NULL;
    lpNode->prev = NULL;

    *blt = lpNode;

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: fxBltCheckForBullets
// Description: Check for free bullets. If there are none, take the oldest bullet
// off the active list.
//////////////////////////////////////////////////////////////////////
LOGICAL fxBltCheckForBullets()
{
    if(_freeH.next == &_freeT)
    {
        FXBULLET* blt = NULL;

        if(g_bltTail.prev == &g_bltHead)
            return FALSE;

        blt = g_bltTail.prev;

        if(!fxBltReleaseBullet(blt))
            return FALSE;

        if(!fxBltFreeBullet(blt))
            return FALSE;
    }

    if(_freeH.next == &_freeT)
        return FALSE;

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: fxBltReleaseBullet
// Description: Release bullet from active list...do before fxBltFreeBullet.
//////////////////////////////////////////////////////////////////////
LOGICAL fxBltReleaseBullet(FXBULLET* blt)
{
    FXBULLET* lpNext = NULL;
    FXBULLET* lpPrev = NULL;

    if(blt == NULL)
        return FALSE;

    lpNext = blt->next;
    lpPrev = blt->prev;

    if(lpNext == NULL || lpPrev == NULL)
        return FALSE;

    lpPrev->next = lpNext;
    lpNext->prev = lpPrev;

    blt->prev = NULL;
    blt->next = NULL;

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: fxBltDetachBlt
// Description: Detach bullets from object.
//////////////////////////////////////////////////////////////////////
LOGICAL fxBltDetachBlt(OBJECT* lpObj)
{
    if(g_blts == NULL)
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
void _UnMap(void* key)
{
    std::multimap<void* , SINT32>::iterator I;
    std::multimap<void* , SINT32>::iterator FREE;

    if(g_mappedBullets.empty())
        return;

    if(!key)
    {
        return;
    }

    I = g_mappedBullets.find(key);

    if(I == g_mappedBullets.end())
    {
        return;
    }

    std::vector<SINT32> removeList;

    while(I != g_mappedBullets.end())
    {
        if(I->first != key)
            break;

        FREE = I;
        ++I;

        SINT32 iIndex = FREE->second - 1;

        removeList.push_back(iIndex);
        g_mappedBullets.erase(FREE);
    }

    for(UINT32 u = 0; u < removeList.size(); u++)
    {
        g_blts[removeList[u]].o = NULL;

        fxBltReleaseBullet(&g_blts[removeList[u]]);
        fxBltFreeBullet(&g_blts[removeList[u]]);
    }
}

//////////////////////////////////////////////////////////////////////
// Function: fxBltSetColor
// Description: Set color one time per draw from fxMgr.
//////////////////////////////////////////////////////////////////////
void fxBltSetColor(piCOLORVALUE* lpColor)
{
    g_lpColor = lpColor;
}

//////////////////////////////////////////
// Function: fxBltUpdate
// Description:
//////////////////////////////////////////
void fxBltUpdate()
{
    if(g_bltHead.next == NULL) return;
    if(g_bltHead.next == &g_bltTail) return;

    FXBULLET* blt = g_bltHead.next;
    FXBULLET* lpTail = &g_bltTail;
    FXBULLET* free = NULL;

    piVECTOR vecDiff;
    FLOAT32 len2 = 0.0f;

    drawlist.clear();

    while(blt != lpTail)
    {
        if(!rnvisVectorFOVCheck(&blt->relpos, blt->radius * 2.0f))
        {
            blt = blt->next;
            continue;
        }

        // Damaged building, for example
        if(blt->o->flags & OBJECT_SKIPDRAWING)
        {
            free = blt;
            blt = blt->next;

            fxBltReleaseBullet(free);
            fxBltFreeBullet(free);

            continue;
        }

        // Free if over 1000 meters away
        if(blt->dis2 > 1000000.0f)
        {
            free = blt;
            blt = blt->next;

            fxBltReleaseBullet(free);
            fxBltFreeBullet(free);

            continue;
        }

        blt->oldrelpos = blt->relpos;

        if(blt->lpVehAttach != NULL)
        {
            fxBltUpdateVehicleBullet(blt);
        }

        fxUtlUpdatePosition(&blt->relpos, &blt->worldpos);
        blt->dis2 = fxUtlGetDistanceSqrd(&blt->relpos);

        // If this is not zero, then world offset had change. I'm not even
        // going to bother with this crap. Just free the bullet.
        mathSubVectors(&vecDiff, &blt->oldrelpos, &blt->relpos);

        len2 = vecDiff.x * vecDiff.x +
               vecDiff.y * vecDiff.y +
               vecDiff.z * vecDiff.z;

        if(len2 > 0.0001f)
        {
            free = blt;
            blt = blt->next;

            fxBltReleaseBullet(free);
            fxBltFreeBullet(free);

            continue;
        }

        if(blt->flags & blt_decal_is_ground_blood && blt->scale <= 1.0f)
        {
            piVECTOR v;

            for(int i = 0; i < 4; i++)
            {
                v = blt->original[i];

                mathSubVectors(&v, &blt->original[i], &blt->relpos);
                v.x *= blt->scale;
                v.y *= blt->scale;
                mathAddVectors(&v, &v, &blt->relpos);

                blt->decal[i].x = v.x;
                blt->decal[i].y = v.y;
                blt->decal[i].z = blt->original[i].z;
            }

            // Translate to origin, scale, translate back
            blt->scale += (frametime - blt->lastupdate) * 0.001f * GROUND_BLOOD_ANIM_SCALE;
        }
        else
        {
            if(blt->lpVehAttach)
            {
                math3dVectorMultiply(&blt->decal[0], &blt->original[0], &blt->o->mat);
                math3dVectorMultiply(&blt->decal[1], &blt->original[1], &blt->o->mat);
                math3dVectorMultiply(&blt->decal[2], &blt->original[2], &blt->o->mat);
                math3dVectorMultiply(&blt->decal[3], &blt->original[3], &blt->o->mat);
            }
            else
            {
                memcpy(&blt->decal, &blt->original, sizeof(piVECTOR) * 4);
            }
        }

        drawlist.push_back(blt);
        blt->lastupdate = frametime;

        blt = blt->next;
    }

    if(!drawlist.empty())
    {
        sort(drawlist.begin(), drawlist.end(), _SortBackToFront);
    }
}

//////////////////////////////////////////
// Function: _SortBackToFront
// Description:
//////////////////////////////////////////
SINT32 _SortBackToFront(FXBULLET* l, FXBULLET* r)
{
    return l->dis2 > r->dis2;
}
