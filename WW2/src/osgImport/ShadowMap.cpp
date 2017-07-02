#include "syscfg.h" // Path is project specific.
#include "osgImport/ShadowMap.h"
#include "osgImport/Texture2D.h"
#include "osgImport/BoundingBox.h"
#include "osgImport/Polytope.h"
#include "scen/ScenarioManager.h"

#include "ob/obdlite.h"
#include "rn/rnmain.h"
#include "trn/trnvars.h"
#include "trn/trnmain.h"
#include "math/math3d.h"
#include "math/Matrixf.h"
#include "math/Vec2d.h"
#include "ld/ldtexture.h"
#include "fx/fxWeather.h"
#include "ob/obcs.h"
#include "ob/obcd.h"
#include "list/list.h"
#include "pers/persmain.h"
#include "trn/trncd.h"
#include "ob/obdraw.h"
#include "spt/sptmain.h"
#include "gr2/gr2Anim.h"
#include "gmlp/gmlpmain.h"
#include "ui/uiMain.h"
#include "ogl/States.h"
#include "physics/colliderQuery.h"
#include "sdm/sdmDefines.h"

#if DEBUG_SHADOW_MAP
#include "ui/uiChat.h"
#include "draw/draw3d.h"
#endif

#ifdef BETA_RELEASE
#include "ui/uiDotCommands.h"
#endif

#define SHADOW_UPDATE_TIME  5000

static ref_ptr<ShadowMap> _shadowmap;

/*---------------------------------------------------------------------------
Function: ShadowMap::GetInstance()
Description:
----------------------------------------------------------------------------*/
ShadowMap& ShadowMap::GetInstance()
{
    if(!_shadowmap.valid())
        _shadowmap = new ShadowMap;

    return *_shadowmap.get();
}

/*---------------------------------------------------------------------------
Function: ShadowMap::ShadowMap()
Description:
----------------------------------------------------------------------------*/
ShadowMap::ShadowMap():
    _reset(false),
    _enabled(false),
    _DynamicShadowDistance(40.0f),
    _StaticShadowDistance(800.0f),
    _light(nullptr),
    _baseTextureUnit(0),
    _shadowTextureUnit(0),
    _polyOffset(1.0f, 1.0f),
    _ambientBias(0.5f, 0.5f),
    DynamicTexWidth(1024),
    DynamicTexHeight(1024),
    _texwidth(2048),
    _texheight(2048),
    lastshadowcreate(0)
{
}

/*---------------------------------------------------------------------------
Function: ShadowMap::setTextureUnit()
Description:
----------------------------------------------------------------------------*/
void ShadowMap::setTextureUnit(unsigned int unit)
{
    _shadowTextureUnit = unit;
}

/*---------------------------------------------------------------------------
Function: ShadowMap::setPolygonOffset()
Description:
----------------------------------------------------------------------------*/
void ShadowMap::setPolygonOffset(const Vec2f& polyOffset)
{
    _polyOffset = polyOffset;
}

/*---------------------------------------------------------------------------
Function: ShadowMap::setAmbientBias()
Description:
----------------------------------------------------------------------------*/
void ShadowMap::setAmbientBias(const Vec2f& ambientBias)
{
    _ambientBias = ambientBias;
}

/*---------------------------------------------------------------------------
Function: ShadowMap::setTextureSize()
Description:
----------------------------------------------------------------------------*/
void ShadowMap::setTextureSize(int w, int h)
{
    _texwidth = w;
    _texheight = h;
}

/*---------------------------------------------------------------------------
Function: ShadowMap::setLight()
Description:
----------------------------------------------------------------------------*/
void ShadowMap::setLight(LIGHT* light)
{
    _light = light;
}

/*---------------------------------------------------------------------------
Function: ShadowMap::PostRenderUpdate()
Description:
----------------------------------------------------------------------------*/
void ShadowMap::PostRenderUpdate()
{
    // Reset shadow maps for each spawn.
    _reset = !uiInGame();
}

/*---------------------------------------------------------------------------
Function: CollideDown
Description:
----------------------------------------------------------------------------*/
static void CollideDown(Vec3d pos, colliderQuery::CollisionList& collisions)
{
    static colliderQuery collider;
    static bool firsttime = true;

    if(firsttime)
    {
        firsttime = false;
        collider.addGeomToCollideWith((dGeomID)colliderWorldSpace());
        dGeomID ray = dCreateRay(collider.space(), 10000.0f);
        dGeomRaySet(ray, 0, 0, 0, 0, 0, -1); // Updated when we collide
        collider.addGeom(ray, 0, COLLIDER_ALL);
    }

    collisions.clear();

    pos = physicsGameToPhysical(pos);
    pos.z() += 0.1f;

    colliderQuery::dGeomList& geoms = collider.geoms();
    dGeomRaySet(geoms[0], pos.x(), pos.y(), pos.z(), 0, 0, -1);

    collider.setDirty(true);

    if(collider.findCollisions(true))
    {
        collisions = collider.getCollisions();
    }
}

/*---------------------------------------------------------------------------
Function: ShadowMap::FindPlayerPosition
Description:
----------------------------------------------------------------------------*/
bool ShadowMap::FindPlayerPosition()
{
    const mlVehicleData* data = persGetPersonaModelDataPtr();

    if(!data)
        return false;

    if(data->SpawnTime == 0)
        return false;

    Vec3d pos(data->VehiclePos);
    pos.z() += 100.0f;

    float z = 0.0f;

    colliderQuery::CollisionList collisions;

    CollideDown(pos, collisions);

    if(!collisions.empty())
    {
        z = (float)-physicsPhysicalToGame(collisions[0].pos).z();
    }

    smoctet.center.set((float)pos.x(), (float)pos.y(), z);

    return true;
}

/*---------------------------------------------------------------------------
Function: ShadowMap::FindOctetCenter
Description:
----------------------------------------------------------------------------*/
bool ShadowMap::FindOctetCenter(int corner)
{
    const mlVehicleData* data = persGetPersonaModelDataPtr();

    if(!data)
        return false;

    if(data->SpawnTime == 0)
        return false;

    int sx = -1;
    int sy = -1;

    piVECTOR p;

    // We are going to find our supercell and octet for each bounding box point (4 of them)
    p.set(float(data->VehiclePos.x), float(data->VehiclePos.y), float(data->VehiclePos.z));

#if(0)
    short xx, yy;
    xx = (int)(p.x / SDM_SUPERCELL_SIZE);
    yy = (int)(p.y / SDM_SUPERCELL_SIZE);

    if(xx < 0) xx--;
    if(yy < 0) yy--;

    int cc = (int)((p.x - xx * SDM_SUPERCELL_SIZE) / SDM_OCTET_SIZE);
    int rr = (int)((p.y - yy * SDM_SUPERCELL_SIZE) / SDM_OCTET_SIZE);
#endif

    Vec2d v;
    int c, r;

    // Our bounding box is 400 radius
    switch(corner)
    {
    case 0:
        {
            p.x -= SDM_OCTET_SIZE_2;
            p.y -= SDM_OCTET_SIZE_2;
            break;
        }
    case 1:
        {
            p.x -= SDM_OCTET_SIZE_2;
            p.y += SDM_OCTET_SIZE_2;
            break;
        }
    case 2:
        {
            p.x += SDM_OCTET_SIZE_2;
            p.y += SDM_OCTET_SIZE_2;
            break;
        }
    case 3:
        {
            p.x += SDM_OCTET_SIZE_2;
            p.y -= SDM_OCTET_SIZE_2;
            break;
        }
    }

    trnmainGetSupercellAndOctet(Vec3d(p), sx, sy, r, c);

    if(!_reset)
    {
        // Is this aready set? If so, the shadow map has already been created for
        // this octet.
        if(_smOctet[corner].sx == sx && _smOctet[corner].sy == sy)
        {
            if(_smOctet[corner].r == r && _smOctet[corner].c == c)
            {
                // Stupid world origin offset
                if(!trnmainWorldOriginMoved())
                {
                    return false;
                }
            }
        }
    }

    // Collide down at center of octet to get z
    // SDM_OCTET_SIZE_2 is center of octet.
    float x = c * SDM_OCTET_SIZE + SDM_OCTET_SIZE_2 + sx * SDM_SUPERCELL_SIZE - float(WorldOriginOffset.x);
    float y = r * SDM_OCTET_SIZE + SDM_OCTET_SIZE_2 + sy * SDM_SUPERCELL_SIZE - float(WorldOriginOffset.y);
    float z = 0;

    // x and y should be relative to our offset now.
    COLLISION collide;

    collide.oldpos.x = x;
    collide.oldpos.y = y;
    collide.oldpos.z = -10000.0f;

    collide.newpos.x = x;
    collide.newpos.y = y;
    collide.newpos.z = 10000.0f;

    TERRAIN_DETAIL* dtl = nullptr;

    for(int i = 0; i < NUM_TERRAIN_RECTANGLES; i++)
	{
        if(activeTerrainDetail[i] && trndtl[i].scx == sx && trndtl[i].scy == sy)
        {
            dtl = &trndtl[i];
            break;
        }
	}

    if(!dtl)
        return false;

    // Find z height at center of this octet
    if(trncdCheckTerrainDownWithDTL(&collide, dtl))
    {
        z = collide.colpos.z;
    }

    // Set info for modelview creation
    _smOctet[corner].sx = sx;
    _smOctet[corner].sy = sy;
    _smOctet[corner].r = r;
    _smOctet[corner].c = c;
    _smOctet[corner].center.set(float(x + WorldOriginOffset.x), float(y + WorldOriginOffset.y), z);

    return true;
}

/*-------------------------------------------------------------------
Function: ShadowTexture::cull
Description:
--------------------------------------------------------------------*/
bool ShadowMap::cull(const BoundingSphere& bs)
{
    return !polytope.contains(bs);
}

/*-------------------------------------------------------------------
Function: ShadowTexture::setupCull
Description:
--------------------------------------------------------------------*/
void ShadowMap::setupCull()
{
    polytope.setToUnitFrustum();

    Matrixf mvp = camera * projection;

    polytope.transformProvidingInverse(mvp);
}

/*---------------------------------------------------------------------------
Function: ShadowMap::RenderShadowCasters()
Description:
----------------------------------------------------------------------------*/
void ShadowMap::RenderShadowCasters()
{
    if(!_enabled)
        return;

    if(!FindPlayerPosition())
        return;

    bool oldnormal = g_wantNormalMaps;
    bool oldspec = WantSpecularMaps;

    g_wantNormalMaps = false;
    WantSpecularMaps = false;

    obdrawSetRenderPass(RENDER_SHADOW_MAP);

    oglSetWorldMatrix(&IdentityMatrix);

    BeginShadowMap();
    smoctet.valid = true;

    if(ogl::UseDeprecatedPath())
        sptDisplayDeprecated();

    obdrawDrawShadowCasters();

    EndShadowMap();

    g_wantNormalMaps = oldnormal;
    WantSpecularMaps = oldspec;

    obdrawSetRenderPass(RENDER_SCENE);

    lastshadowcreate = frametime;
}

/*---------------------------------------------------------------------------
Function: ShadowMap::RenderStaticShadowCasters()
Description:
----------------------------------------------------------------------------*/
void ShadowMap::RenderStaticShadowCasters()
{
    if(!_enabled)
        return;

    if(gCustomTerrain)
    {
        if(!FindPlayerPosition())
            return;

        bool oldnormal = g_wantNormalMaps;
        bool oldspec = WantSpecularMaps;

        g_wantNormalMaps = false;
        WantSpecularMaps = false;

        obdrawSetRenderPass(RENDER_SHADOW_MAP);

        oglSetWorldMatrix(&IdentityMatrix);

        BeginShadowMap();
        smoctet.valid = true;

        if(ogl::UseDeprecatedPath())
            sptDisplayDeprecated();

        obdrawDrawShadowCasters();

        EndShadowMap();

        g_wantNormalMaps = oldnormal;
        WantSpecularMaps = oldspec;

        obdrawSetRenderPass(RENDER_SCENE);

        lastshadowcreate = frametime;
    }
    else
    {
        if(playpos.x == 0.0f && playpos.y == 0.0f && playpos.z == 0.0f)
        {
            return;
        }

        unsigned int flag = 0;

#if DEBUG_SHADOW_MAP
        _reset = true;
#endif

        // We intersect at for corners (a 400 meter box around us)
        if(FindOctetCenter(0))
            flag = 0x1;
        if(FindOctetCenter(1))
            flag |= 0x2;
        if(FindOctetCenter(2))
            flag |= 0x4;
        if(FindOctetCenter(3))
            flag |= 0x8;

        // Each vehicle spawn may distort shadow maps, so make sure they are fresh.
        _reset = false;

        // Nothing has changed
        if(flag == 0)
        {
#if DEBUG_SHADOW_MAP
#else
            return;
#endif
        }

        bool oldnormal = g_wantNormalMaps;
        g_wantNormalMaps = false;

        unsigned int mask = 1;

        obdrawSetRenderPass(RENDER_SHADOW_MAP);

        // Reset playright, playpos, etc...
        piVECTOR oldplayright = playright;
        piVECTOR oldplayup = playup;
        piVECTOR oldplaydir = playdir;
        piVECTOR oldplaytrans = playtrans;
        piVECTOR oldplaypos = playpos;

        oglSetWorldMatrix(&IdentityMatrix);

        for(int i = 0; i < 4; i++)
        {
            if(flag & mask)
            {
                // This octet needs a shadow map
                BeginStaticShadowMap(i);
                _smOctet[i].valid = false;

                float factor = -_polyOffset[0];
                float units =  -_polyOffset[1];

                glPolygonOffset(factor, units);

                if(sptDrawTreeShadowCasters())
                {
                    _smOctet[i].valid = true;
                }

                EndStaticShadowMap(i);
            }

            mask <<= 1;
        }

        oglSetViewMatrix(&IdentityMatrix);

        // Reset playright, playpos, etc...
        playright = oldplayright;
        playup = oldplayup;
        playdir = oldplaydir;
        playtrans = oldplaytrans;
        playpos = oldplaypos;

        obdrawSetRenderPass(RENDER_SCENE);

        g_wantNormalMaps = oldnormal;
    }
}

#if DEBUG_SHADOW_MAP
/*---------------------------------------------------------------------------
Function: ShadowMap::Debug()
Description:
----------------------------------------------------------------------------*/
void ShadowMap::Debug()
{
    static bool showlightview = false;
    static bool drawoctets = true;
    static int c = 0;

    oglSetWorldMatrix(&IdentityMatrix);

    oglLockViewMatrix(0);
    oglLockUnscaledViewMatrix(0);
    oglLockProjectionMatrix(0);
    oglLockCockpitMatrix(0);

    if(showlightview)
    {
        rnmainSetViewMatrix2(&_smOctet[c].mv);
        oglSetProjectionMatrix(&_smOctet[c].proj);

        oglLockViewMatrix(1);
        oglLockUnscaledViewMatrix(1);
        oglLockProjectionMatrix(1);
        oglLockCockpitMatrix(1);
    }

    if(drawoctets)
    {
        for(int i = 0; i < 4; i++)
        {
            piVECTOR p1, p2;
            piVECTOR c = _smOctet[i].center;

            c.x -= (float)WorldOriginOffset.x;
            c.y -= (float)WorldOriginOffset.y;

            p1 = c;
            p2 = p1;
            p2.z -= 100.0f;

            draw3dLine(&p1, &p2, colorWHITEALPHA);

            p1.x = c.x - SDM_OCTET_SIZE_2;
            p1.y = c.y - SDM_OCTET_SIZE_2;
            p1.z = c.z - 1.0f;

            p2.x = c.x - SDM_OCTET_SIZE_2;
            p2.y = c.y + SDM_OCTET_SIZE_2;
            p2.z = c.z - 1.0f;

            draw3dLine(&p1, &p2, colorBLUEALPHA);

            p1.x = c.x - SDM_OCTET_SIZE_2;
            p1.y = c.y + SDM_OCTET_SIZE_2;
            p1.z = c.z - 1.0f;

            p2.x = c.x + SDM_OCTET_SIZE_2;
            p2.y = c.y + SDM_OCTET_SIZE_2;
            p2.z = c.z - 1.0f;

            draw3dLine(&p1, &p2, colorBLUEALPHA);

            p1.x = c.x + SDM_OCTET_SIZE_2;
            p1.y = c.y + SDM_OCTET_SIZE_2;
            p1.z = c.z - 1.0f;

            p2.x = c.x + SDM_OCTET_SIZE_2;
            p2.y = c.y - SDM_OCTET_SIZE_2;
            p2.z = c.z - 1.0f;

            draw3dLine(&p1, &p2, colorBLUEALPHA);

            p1.x = c.x + SDM_OCTET_SIZE_2;
            p1.y = c.y - SDM_OCTET_SIZE_2;
            p1.z = c.z - 1.0f;

            p2.x = c.x - SDM_OCTET_SIZE_2;
            p2.y = c.y - SDM_OCTET_SIZE_2;
            p2.z = c.z - 1.0f;

            draw3dLine(&p1, &p2, colorBLUEALPHA);
        }
    }
}
#endif

#if USE_STATESET
/*---------------------------------------------------------------------------
Function: ShadowMap::RenderDynamicShadowCasters()
Description:
----------------------------------------------------------------------------*/
void ShadowMap::RenderDynamicShadowCasters()
{
    if(gCustomTerrain)
        return;

    extern void obcsGetShadowBin(std::vector<OBJECT*>& bin);

    if(!_enabled)
        return;

    if(playpos.x == 0.0f && playpos.y == 0.0f && playpos.z == 0.0f)
    {
        return;
    }

    struct LISTHEADER* list = nullptr;
	int numobjects = obmainGetObjects(&list);
	OBJECT* o = (OBJECT*)list->next;

    obdrawSetRenderPass(RENDER_SHADOW_MAP);

    int num = obcsCullAndSortShadowObjects(numobjects, o, &playdir);
    DynShadowMap.valid = false;

    if(num == 0)
    {
        FrameBufferObject* fbo = dynamic_cast<FrameBufferObject*>(DynShadowMap.stateset->GetRenderState(RS_FRAMEBUFFEROBJECT));

        fbo->apply();
        oglSetViewport(0, 0, DynamicTexWidth, DynamicTexHeight);
        glClear(GL_DEPTH_BUFFER_BIT);
        fbo->UnApply();

        oglSetViewport(0, 0, WantedWidth, WantedHeight);
        obdrawSetRenderPass(RENDER_SCENE);

        return;
    }

    bool oldnormal = g_wantNormalMaps;
    g_wantNormalMaps = false;

    // Reset playright, playpos, etc...
    piVECTOR oldplayright = playright;
    piVECTOR oldplayup = playup;
    piVECTOR oldplaydir = playdir;
    piVECTOR oldplaytrans = playtrans;
    piVECTOR oldplaypos = playpos;

    // This octet needs a shadow map
    BeginDynamicShadowMap();

    std::vector<OBJECT*> bin;
    obcsGetShadowBin(bin);

    // Only FPS now
    for(unsigned int i = 0; i < bin.size(); i++)
    {
        o = bin[i];

        if(!o) continue;

        if(o->flag3 & OBJECT_IS_INTERIOR)
            continue;

        if(o->flags & OBJECT_USERDRAW)
        {
            // Userdraw deprecated
            if(o->drawcallback)
            {
                DynShadowMap.valid = true;
                o->drawcallback(o);
            }
        }
    }

    if(gr2AnimDrawCorpses())
    {
        DynShadowMap.valid = true;
    }

    EndDynamicShadowMap();

    // Reset playright, playpos, etc...
    playright = oldplayright;
    playup = oldplayup;
    playdir = oldplaydir;
    playtrans = oldplaytrans;
    playpos = oldplaypos;

    obdrawSetRenderPass(RENDER_SCENE);

    g_wantNormalMaps = oldnormal;
}
#else
/*---------------------------------------------------------------------------
Function: ShadowMap::RenderDynamicShadowCasters()
Description:
----------------------------------------------------------------------------*/
void ShadowMap::RenderDynamicShadowCasters()
{
    if(gCustomTerrain)
        return;

    extern void obcsGetShadowBin(std::vector<OBJECT*>& bin);

    if(!_enabled)
        return;

    if(playpos.x == 0.0f && playpos.y == 0.0f && playpos.z == 0.0f)
    {
        return;
    }

    struct LISTHEADER* list = nullptr;
	int numobjects = obmainGetObjects(&list);
	OBJECT* o = (OBJECT*)list->next;

    obdrawSetRenderPass(RENDER_SHADOW_MAP);

    int num = obcsCullAndSortShadowObjects(numobjects, o, &playdir);
    DynShadowMap.valid = false;

    if(num == 0)
    {
        DynShadowMap.fbo->apply();
        oglSetViewport(0, 0, DynamicTexWidth, DynamicTexHeight);
        glClear(GL_DEPTH_BUFFER_BIT);
        DynShadowMap.fbo->UnApply();
        oglSetViewport(0, 0, WantedWidth, WantedHeight);
        obdrawSetRenderPass(RENDER_SCENE);

        return;
    }

    bool oldnormal = g_wantNormalMaps;
    g_wantNormalMaps = false;

    // Reset playright, playpos, etc...
    piVECTOR oldplayright = playright;
    piVECTOR oldplayup = playup;
    piVECTOR oldplaydir = playdir;
    piVECTOR oldplaytrans = playtrans;
    piVECTOR oldplaypos = playpos;

    // This octet needs a shadow map
    BeginDynamicShadowMap();

    std::vector<OBJECT*> bin;
    obcsGetShadowBin(bin);

    // Only FPS now
    for(unsigned int i = 0; i < bin.size(); i++)
    {
        o = bin[i];

        if(!o) continue;

        if(o->flag3 & OBJECT_IS_INTERIOR)
            continue;

        if(o->flags & OBJECT_USERDRAW)
        {
            // Userdraw deprecated
            if(o->drawcallback)
            {
                DynShadowMap.valid = true;
                o->drawcallback(o);
            }
        }
    }

    if(gr2AnimDrawCorpses())
    {
        DynShadowMap.valid = true;
    }

    EndDynamicShadowMap();

    // Reset playright, playpos, etc...
    playright = oldplayright;
    playup = oldplayup;
    playdir = oldplaydir;
    playtrans = oldplaytrans;
    playpos = oldplaypos;

    obdrawSetRenderPass(RENDER_SCENE);

    g_wantNormalMaps = oldnormal;
}
#endif

/*-------------------------------------------------------------------
Function: computeOrthogonalVector
Description:
--------------------------------------------------------------------*/
Vec3f computeOrthogonalVector(const Vec3f& direction)
{
    float length = direction.length();
    static Vec3f right = Vec3f(1.0f, 0.0f, 0.0f);

    Vec3f orthogonalVector = direction ^ right;

    if(orthogonalVector.normalize() < length * 0.5f)
    {
        orthogonalVector = direction ^ Vec3f(0.0f, 0.0f, 1.0f);
        orthogonalVector.normalize();
    }

    return orthogonalVector;
}

#if USE_STATESET
/*---------------------------------------------------------------------------
Function: ShadowMap::BeginStaticShadowMap()
Description:
----------------------------------------------------------------------------*/
bool ShadowMap::BeginStaticShadowMap(int corner)
{
    // This phase hard-codes light source to be 400+ meters directly
    // above the octet center looking straight down.
    _smOctet[corner].stateset->Apply();

    oglSetViewport(0, 0, _texwidth, _texheight);
    glClear(GL_DEPTH_BUFFER_BIT);

    // Lock to prevent others from changing these states
    States::LockState(RS_SHADEMODEL, 1);
    States::LockState(RS_COLORMASK, 1);
    States::LockMode(GL_FOG, 1);
    States::LockMode(GL_LIGHTING, 1);

    static float z = SDM_OCTET_SIZE_2;

    float height = -z;

    _StaticShadowDistance = -height * 2.0f;
    float znear = 1.0f;

    piVECTOR eye = _smOctet[corner].center;
    eye.x -= (float)WorldOriginOffset.x;
    eye.y -= (float)WorldOriginOffset.y;

    eye.z += height;

    float top = -z;
    float bottom = z;
    float right = -z;
    float left = z;

    _smOctet[corner].proj = IdentityMatrix;

    // Left handed ortho (because our game is actually an upside down
    // left handed coordinate system, even though we are OpenGL
    // right handed. (stupid)

    _smOctet[corner].proj._11 = 2.0f / SDM_OCTET_SIZE;
    _smOctet[corner].proj._22 = 2.0f / SDM_OCTET_SIZE;
    _smOctet[corner].proj._33 = 1.0f / (_StaticShadowDistance - znear);
    _smOctet[corner].proj._43 = znear / (znear - _StaticShadowDistance);

    piVECTOR center = _smOctet[corner].center;
    center.x -= (float)WorldOriginOffset.x;
    center.y -= (float)WorldOriginOffset.y;

    piVECTOR up;

    Vec3f playerup = (Vec3f(center) - Vec3f(eye));
    playerup.normalize();
    playerup = playerup ^ Vec3f(1, 0, 0);
    playerup.get(up);

    oglCreateModelView(&_smOctet[corner].mv, &eye, &center, &up, nullptr);

    // This also sets playright, playup, etc...
    rnmainSetViewMatrix2(&_smOctet[corner].mv);

    oglSetProjectionMatrix(&_smOctet[corner].proj);

    static Matrixf bias(0.5f, 0.0f, 0.0f, 0.0f,
                        0.0f, 0.5f, 0.0f, 0.0f,
                        0.0f, 0.0f, 0.5f, 0.0f,
                        0.5f, 0.5f, 0.5f, 1.0f);

    Matrixf mvp = Matrixf(_smOctet[corner].mv) * Matrixf(_smOctet[corner].proj) * bias;
    mvp.get(_smOctet[corner].mvp);

    // Orthogonal culling planes
    LeftPlaneX = -1.0f;
    LeftPlaneZ = -1.0f;
    TopPlaneY = 1.0f;
    TopPlaneZ = -1.0f;

    oglLockViewMatrix(1);
    oglLockUnscaledViewMatrix(1);
    oglLockProjectionMatrix(1);
    oglLockCockpitMatrix(1);

    return true;
}
#else
/*---------------------------------------------------------------------------
Function: ShadowMap::BeginStaticShadowMap()
Description:
----------------------------------------------------------------------------*/
bool ShadowMap::BeginStaticShadowMap(int corner)
{
    // This phase hard-codes light source to be 400+ meters directly
    // above the octet center looking straight down.
    _smOctet[corner].fbo->apply();

    oglSetViewport(0, 0, _texwidth, _texheight);
    glClear(GL_DEPTH_BUFFER_BIT);

    float factor = -_polyOffset[0];
    float units =  -_polyOffset[1];

    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(factor, units);

    // Turn off all bells and whistles
    oglSetRenderState(oglRENDERSTATE_LIGHTING, FALSE);
    oglSetRenderState(oglRENDERSTATE_FOGENABLE, FALSE);
    oglSetRenderState(oglRENDERSTATE_SHADEMODE, GL_FLAT);
    oglSetRenderState(oglRENDERSTATE_COLORMASK, piRGBA_MAKE(0, 0, 0, 0));

    // Lock to prevent others from changing these states
    oglLockState(oglRENDERSTATE_LIGHTING, 1);
    oglLockState(oglRENDERSTATE_FOGENABLE, 1);
    oglLockState(oglRENDERSTATE_SHADEMODE, 1);
    oglLockState(oglRENDERSTATE_COLORMASK, 1);

    static float z = SDM_OCTET_SIZE_2;

    float height = -z;

    _StaticShadowDistance = -height * 2.0f;
    float znear = 1.0f;

    piVECTOR eye = _smOctet[corner].center;
    eye.x -= (float)WorldOriginOffset.x;
    eye.y -= (float)WorldOriginOffset.y;

    eye.z += height;

    _smOctet[corner].proj = IdentityMatrix;

    // Left handed ortho (because our game is actually an upside down
    // left handed coordinate system, even though we are OpenGL
    // right handed. (stupid)

    _smOctet[corner].proj._11 = 2.0f / SDM_OCTET_SIZE;
    _smOctet[corner].proj._22 = 2.0f / SDM_OCTET_SIZE;
    _smOctet[corner].proj._33 = 1.0f / (_StaticShadowDistance - znear);
    _smOctet[corner].proj._43 = znear / (znear - _StaticShadowDistance);

    piVECTOR center = _smOctet[corner].center;
    center.x -= (float)WorldOriginOffset.x;
    center.y -= (float)WorldOriginOffset.y;

    piVECTOR up;

    Vec3f playerup = (Vec3f(center) - Vec3f(eye));
    playerup.normalize();
    playerup = playerup ^ Vec3f(1, 0, 0);
    playerup.get(up);

    oglCreateModelView(&_smOctet[corner].mv, &eye, &center, &up, nullptr);

    // This also sets playright, playup, etc...
    rnmainSetViewMatrix2(&_smOctet[corner].mv);

    oglSetProjectionMatrix(&_smOctet[corner].proj);

    static Matrixf bias(0.5f, 0.0f, 0.0f, 0.0f,
                        0.0f, 0.5f, 0.0f, 0.0f,
                        0.0f, 0.0f, 0.5f, 0.0f,
                        0.5f, 0.5f, 0.5f, 1.0f);

    Matrixf mvp = Matrixf(_smOctet[corner].mv) * Matrixf(_smOctet[corner].proj) * bias;
    mvp.get(_smOctet[corner].mvp);

    // Orthogonal culling planes
    LeftPlaneX = -1.0f;
    LeftPlaneZ = -1.0f;
    TopPlaneY = 1.0f;
    TopPlaneZ = -1.0f;

    oglLockViewMatrix(1);
    oglLockUnscaledViewMatrix(1);
    oglLockProjectionMatrix(1);
    oglLockCockpitMatrix(1);

    return true;
}
#endif

#if USE_STATESET
/*---------------------------------------------------------------------------
Function: ShadowMap::BeginDynamicShadowMap()
Description:
----------------------------------------------------------------------------*/
bool ShadowMap::BeginDynamicShadowMap()
{
    // This phase hard-codes light source to be 400+ meters directly
    // above the octet center looking straight down.
    const mlVehicleData* data = persGetPersonaModelDataPtr();

    if(!data)
        return false;

    if(data->SpawnTime == 0)
        return false;

    DynShadowMap.center.set(float(data->VehiclePos.x - WorldOriginOffset.x),
                            float(data->VehiclePos.y - WorldOriginOffset.y),
                            float(-data->VehiclePos.z));

    DynShadowMap.stateset->Apply();

    oglSetViewport(0, 0, DynamicTexWidth, DynamicTexHeight);

    // Lock to prevent others from changing these states
    States::LockState(RS_SHADEMODEL, 1);
    States::LockState(RS_COLORMASK, 1);
    States::LockState(RS_FRONTFACE, 1);
    States::LockMode(GL_FOG, 1);
    States::LockMode(GL_LIGHTING, 1);

    static float z = 20.0f;

    float height = -z;

    _DynamicShadowDistance = -height * 2.0f;
    float znear = 1.0f;

    piVECTOR eye;
    eye = DynShadowMap.center;

    eye.z += height;

    float top = -z;
    float bottom = z;
    float right = -z;
    float left = z;

    DynShadowMap.proj = IdentityMatrix;

    DynShadowMap.proj._11 = 2.0f / (right - left);
    DynShadowMap.proj._22 = 2.0f / (top - bottom);
    DynShadowMap.proj._33 = 1.0f / (_DynamicShadowDistance - znear);
    DynShadowMap.proj._43 = znear / (znear - _DynamicShadowDistance);

    piVECTOR center = DynShadowMap.center;

    piVECTOR up;

    Vec3f playerup = (Vec3f(center) - Vec3f(eye));
    playerup.normalize();
    playerup = playerup ^ Vec3f(1, 0, 0);
    playerup.get(up);

    oglCreateModelView(&DynShadowMap.mv, &eye, &center, &up, nullptr);

    // This also sets playright, playup, etc...
    rnmainSetViewMatrix2(&DynShadowMap.mv);

    oglSetProjectionMatrix(&DynShadowMap.proj);

    // Do this once here:
    static Matrixf bias(0.5f, 0.0f, 0.0f, 0.0f,
                        0.0f, 0.5f, 0.0f, 0.0f,
                        0.0f, 0.0f, 0.5f, 0.0f,
                        0.5f, 0.5f, 0.5f, 1.0f);

    Matrixf mvp = Matrixf(DynShadowMap.mv) * Matrixf(DynShadowMap.proj) * bias;
    mvp.get(DynShadowMap.mvp);

    // Orthogonal culling planes
    LeftPlaneX = -1.0f;
    LeftPlaneZ = -1.0f;
    TopPlaneY = 1.0f;
    TopPlaneZ = -1.0f;

    oglLockViewMatrix(1);
    oglLockUnscaledViewMatrix(1);
    oglLockCockpitMatrix(1);
    oglLockProjectionMatrix(1);

    return true;
}
#else
/*---------------------------------------------------------------------------
Function: ShadowMap::BeginDynamicShadowMap()
Description:
----------------------------------------------------------------------------*/
bool ShadowMap::BeginDynamicShadowMap()
{
    // This phase hard-codes light source to be 400+ meters directly
    // above the octet center looking straight down.
    const mlVehicleData* data = persGetPersonaModelDataPtr();

    if(!data)
        return false;

    if(data->SpawnTime == 0)
        return false;

    DynShadowMap.center.set(float(data->VehiclePos.x - WorldOriginOffset.x),
                            float(data->VehiclePos.y - WorldOriginOffset.y),
                            float(-data->VehiclePos.z));

    DynShadowMap.fbo->apply();

    oglSetViewport(0, 0, DynamicTexWidth, DynamicTexHeight);
    glClear(GL_DEPTH_BUFFER_BIT);

    float factor = -_polyOffset[0];
    float units =  -_polyOffset[1];

    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(factor, units);

    // Turn off all bells and whistles
    oglSetRenderState(oglRENDERSTATE_LIGHTING, FALSE);
    oglSetRenderState(oglRENDERSTATE_FOGENABLE, FALSE);
    oglSetRenderState(oglRENDERSTATE_SHADEMODE, GL_FLAT);
    oglSetRenderState(oglRENDERSTATE_COLORMASK, piRGBA_MAKE(0, 0, 0, 0));
    oglSetRenderState(oglRENDERSTATE_CULLFACE, GL_BACK);

    // Lock to prevent others from changing these states
    oglLockState(oglRENDERSTATE_LIGHTING, 1);
    oglLockState(oglRENDERSTATE_FOGENABLE, 1);
    oglLockState(oglRENDERSTATE_SHADEMODE, 1);
    oglLockState(oglRENDERSTATE_COLORMASK, 1);

    static float z = 20.0f;

    float height = -z;

    _DynamicShadowDistance = -height * 2.0f;
    float znear = 1.0f;

    piVECTOR eye;
    eye = DynShadowMap.center;

    eye.z += height;

    float top = -z;
    float bottom = z;
    float right = -z;
    float left = z;

    DynShadowMap.proj = IdentityMatrix;

    DynShadowMap.proj._11 = 2.0f / (right - left);
    DynShadowMap.proj._22 = 2.0f / (top - bottom);
    DynShadowMap.proj._33 = 1.0f / (_DynamicShadowDistance - znear);
    DynShadowMap.proj._43 = znear / (znear - _DynamicShadowDistance);

    piVECTOR center = DynShadowMap.center;

    piVECTOR up;

    Vec3f playerup = (Vec3f(center) - Vec3f(eye));
    playerup.normalize();
    playerup = playerup ^ Vec3f(1, 0, 0);
    playerup.get(up);

    oglCreateModelView(&DynShadowMap.mv, &eye, &center, &up, nullptr);

    // This also sets playright, playup, etc...
    rnmainSetViewMatrix2(&DynShadowMap.mv);

    oglSetProjectionMatrix(&DynShadowMap.proj);

    // Do this once here:
    static Matrixf bias(0.5f, 0.0f, 0.0f, 0.0f,
                        0.0f, 0.5f, 0.0f, 0.0f,
                        0.0f, 0.0f, 0.5f, 0.0f,
                        0.5f, 0.5f, 0.5f, 1.0f);

    Matrixf mvp = Matrixf(DynShadowMap.mv) * Matrixf(DynShadowMap.proj) * bias;
    mvp.get(DynShadowMap.mvp);

    // Orthogonal culling planes
    LeftPlaneX = -1.0f;
    LeftPlaneZ = -1.0f;
    TopPlaneY = 1.0f;
    TopPlaneZ = -1.0f;

    oglLockViewMatrix(1);
    oglLockUnscaledViewMatrix(1);
    oglLockCockpitMatrix(1);
    oglLockProjectionMatrix(1);

    return true;
}
#endif

/*---------------------------------------------------------------------------
Function: ShadowMap::Valid()
Description:
----------------------------------------------------------------------------*/
bool ShadowMap::Valid()
{
    if(gCustomTerrain)
        return smoctet.valid;

    return _smOctet[0].valid ||
           _smOctet[1].valid ||
           _smOctet[2].valid ||
           _smOctet[3].valid ||
           DynShadowMap.valid;
}

/*---------------------------------------------------------------------------
Function: ShadowMap::ApplyShadowMaps()
Description:
----------------------------------------------------------------------------*/
void ShadowMap::ApplyShadowMaps()
{
    if(!_enabled)
        return;

    oglSetWorldMatrix(&IdentityMatrix);

    _program->Apply();

    static bool iscloudy = false;

    GLSL.getProgram("shadow")->GetUniform("ambient")->Set(Vec3f(0.5f, 0.5f, 0.5f));

    if(iscloudy != fxWeatherIsCloudy())
    {
        iscloudy = fxWeatherIsCloudy();

        if(iscloudy)
            GLSL.getProgram("shadow")->GetUniform("ambient")->Set(Vec3f(0.75f, 0.75f, 0.75f));
    }

    _program->ApplyUniformMap();

    // Have to do inverse model view, since we are bypassing the pipeline.
    _invModelView = *oglGetViewMatrix();
    math3dInvertMatrix(&_invModelView, &_invModelView);

    if(!gCustomTerrain)
    {
        if(_smOctet[0].valid)
            math3dMatrixMultiply(&_smOctet[0].texmat, &_invModelView, &_smOctet[0].mvp);

        if(_smOctet[1].valid)
            math3dMatrixMultiply(&_smOctet[1].texmat, &_invModelView, &_smOctet[1].mvp);

        if(_smOctet[2].valid)
            math3dMatrixMultiply(&_smOctet[2].texmat, &_invModelView, &_smOctet[2].mvp);

        if(_smOctet[3].valid)
            math3dMatrixMultiply(&_smOctet[3].texmat, &_invModelView, &_smOctet[3].mvp);

        if(DynShadowMap.valid)
            math3dMatrixMultiply(&DynShadowMap.texmat, &_invModelView, &DynShadowMap.mvp);

        _lastcorner = -1;
    }
}

/*---------------------------------------------------------------------------
Function: ShadowMap::GetShadowMapTexture()
Description:
----------------------------------------------------------------------------*/
Texture2D* ShadowMap::GetShadowMapTexture()
{
    if(!smoctet.valid)
    {
        return _white.get();
    }

    return smoctet.map.get();
}

/*---------------------------------------------------------------------------
Function: ShadowMap::GetInverseMVP()
Description:
----------------------------------------------------------------------------*/
const Matrixf& ShadowMap::GetInverseMVP()
{
    invmvp.identity();

    if(!smoctet.valid)
        return invmvp;

    // Have to do inverse model view, since we are bypassing the pipeline.
    invmvp.set(*oglGetViewMatrix());
    invmvp.invert(invmvp);

    invmvp = invmvp * modelviewproj;

    return invmvp;
}

/*---------------------------------------------------------------------------
Function: ShadowMap::SetShadowStuff()
Description:
----------------------------------------------------------------------------*/
bool ShadowMap::SetShadowStuff(oglMaterial& material, int unit)
{
    if(smoctet.valid || DynShadowMap.valid)
    {
        oglSetActiveTextureStage(unit);

        if(!smoctet.valid || !Enabled())
        {
            material.AddTexture(unit, _white.get());
        }
        else
        {
            material.AddTexture(unit, smoctet.map.get());

            // Have to do inverse model view, since we are bypassing the pipeline.
            Matrixf view(*oglGetViewMatrix());
            view.invert(view);

            Matrixf mvp = view * modelviewproj;
            oglSetTextureMatrix(mvp);
        }

#if(0)
        // Place holder until dynamic shadow implemented.
        material.AddTexture(unit + 1, _white.get());
#endif

        static bool iscloudy = false;
        glslUniform* u = material.GetProgram()->GetUniform("shadowambient");

        if(u)
            u->Set(Vec3f(0.7f, 0.7f, 0.7f));

        if(u && (iscloudy != fxWeatherIsCloudy()))
        {
            iscloudy = fxWeatherIsCloudy();

            if(iscloudy)
                u->Set(Vec3f(0.75f, 0.75f, 0.75f));
            else
                u->Set(Vec3f(0.7f, 0.7f, 0.7f));
        }
    }

    return true;
}

/*---------------------------------------------------------------------------
Function: ShadowMap::GetShadowAmbient()
Description:
----------------------------------------------------------------------------*/
void ShadowMap::GetShadowAmbient(Vec3f& ambient)
{
    static bool iscloudy = false;
    ambient.set(0.7f, 0.7f, 0.7f);

    if(iscloudy != fxWeatherIsCloudy())
    {
        iscloudy = fxWeatherIsCloudy();

        if(iscloudy)
            ambient.set(0.75f, 0.75f, 0.75f);
        else
            ambient.set(0.7f, 0.7f, 0.7f);
    }
}

/*---------------------------------------------------------------------------
Function: ShadowMap::SetShadowStuff()
Description:
----------------------------------------------------------------------------*/
bool ShadowMap::SetShadowStuff(OBJECT* o)
{
    if(gCustomTerrain)
    {
        if(o)
        {
            static float offset = -0.025f;

            Matrixf m(o->mat);
            // Lower a bit to avoid flickering anomalies
            m(3, 2) += offset;
            oglSetWorldMatrix(m);
        }

        if(smoctet.valid || DynShadowMap.valid)
        {
            oglSetActiveTextureStage(_shadowTextureUnit);
            TextureState::Enable(GL_TEXTURE_2D);

            if(!smoctet.valid)
            {
                _white->apply();
            }
            else
            {
                smoctet.map->apply();
                Matrixf mvp = Matrixf(_invModelView) * modelviewproj;
                oglSetTextureMatrix(mvp);
            }

            oglSetActiveTextureStage(_shadowTextureUnit + 1);
            TextureState::Enable(GL_TEXTURE_2D);
            _white->apply();
        }
    }
    else
    {
        piVECTOR p;
        p.set(0, 0, 0);

        if(o)
        {
            math3dVectorMultiply(&p, &o->pos, &o->mat);
        }

        p.x += (float)WorldOriginOffset.x;
        p.y += (float)WorldOriginOffset.y;

        int sx, sy, r, c;
        trnmainGetSupercellAndOctet(Vec3d(p), sx, sy, r, c);

        int corner = -1;

        for(int i = 0; i < 4; i++)
        {
            if(_smOctet[i].sx == sx && _smOctet[i].sy == sy && _smOctet[i].r == r && _smOctet[i].c == c)
            {
                if(!_smOctet[i].valid && !DynShadowMap.valid)
                {
                    return false;
                }

                corner = i;
            }
        }

        if(corner == -1 && !DynShadowMap.valid)
        {
            return false;
        }

        if(o)
        {
            oglSetWorldMatrix(&o->mat);
        }

        if(_lastcorner != corner || DynShadowMap.valid)
        {
            _lastcorner = corner;

            oglSetActiveTextureStage(_shadowTextureUnit);
            TextureState::Enable(GL_TEXTURE_2D);

            if(corner == -1)
            {
                _white->apply();
            }
            else
            {
                _smOctet[corner].map->apply();
                oglSetTextureMatrix(&_smOctet[corner].texmat);
            }

            oglSetActiveTextureStage(_shadowTextureUnit + 1);
            TextureState::Enable(GL_TEXTURE_2D);

            if(DynShadowMap.valid && o->distsqr <= 800.0f)
            {
                DynShadowMap.map->apply();
                oglSetTextureMatrix(&DynShadowMap.texmat);
            }
            else
            {
                _white->apply();
            }
        }

        // Don't want FPS to shadow themselves
        // Ragdoll
        if(o->flag2 & (OBJECT_ISFPS | OBJECT_ANIMATION))
        {
            oglSetActiveTextureStage(_shadowTextureUnit + 1);
            _white->apply();
            _lastcorner = -1;
        }
        // Other
        else if(o->child)
        {
            if((o->child->flag2 & (OBJECT_ISFPS | OBJECT_ANIMATION)))
            {
                oglSetActiveTextureStage(_shadowTextureUnit + 1);
                _white->apply();
                _lastcorner = -1;

            }
        }
    }

    return true;
}

/*---------------------------------------------------------------------------
Function: ShadowMap::SetShadowStuff()
Description: For non replacement octets
----------------------------------------------------------------------------*/
bool ShadowMap::SetShadowStuff(const piVECTOR& point, unsigned int flags)
{
    if(gCustomTerrain)
        return false;

    piVECTOR p = point;

    p.x += (float)WorldOriginOffset.x;
    p.y += (float)WorldOriginOffset.y;

    int sx, sy, r, c;
    trnmainGetSupercellAndOctet(Vec3d(p), sx, sy, r, c);

    int corner = -1;

    for(int i = 0; i < 4; i++)
    {
        if(_smOctet[i].sx == sx && _smOctet[i].sy == sy && _smOctet[i].r == r && _smOctet[i].c == c)
        {
            if(!_smOctet[i].valid && !DynShadowMap.valid)
            {
                return false;
            }

            corner = i;
        }
    }

    if(corner == -1 && !DynShadowMap.valid)
    {
        return false;
    }

    if(_lastcorner != corner || DynShadowMap.valid)
    {
        _lastcorner = corner;

        oglSetActiveTextureStage(_shadowTextureUnit);

        if(corner == -1)
        {
            _white->apply();
        }
        else
        {
            _smOctet[corner].map->apply();
            oglSetTextureMatrix(&_smOctet[corner].texmat);
        }

        oglSetActiveTextureStage(_shadowTextureUnit + 1);
        TextureState::Enable(GL_TEXTURE_2D);

        if(DynShadowMap.valid)
        {
            DynShadowMap.map->apply();
            oglSetTextureMatrix(&DynShadowMap.texmat);
        }
        else
        {
            _white->apply();
        }
    }

    // Don't want corpse to shadow themselves
    if(flags & SHADOWED_CORPSE)
    {
        oglSetActiveTextureStage(_shadowTextureUnit + 1);
        _white->apply();
        _lastcorner = -1;
    }

    return true;
}

#if USE_STATESET
/*---------------------------------------------------------------------------
Function: ShadowMap::SetShadowStuff()
Description: For non replacement octets
----------------------------------------------------------------------------*/
bool ShadowMap::SetShadowStuff(RenderStateSet& material, const Vec3d& point, int unit)
{
    int sx, sy, r, c;
    trnmainGetSupercellAndOctet(Vec3d(point), sx, sy, r, c);

    int corner = -1;

    for(int i = 0; i < 4; i++)
    {
        if(_smOctet[i].sx == sx && _smOctet[i].sy == sy && _smOctet[i].r == r && _smOctet[i].c == c)
        {
            if(!_smOctet[i].valid && !DynShadowMap.valid)
            {
                return false;
            }

            corner = i;
        }
    }

    if(corner == -1 && !DynShadowMap.valid)
    {
        TextureUnit* tu;
        tu = dynamic_cast<TextureUnit*>(material.GetRenderState(RS_TEXTURE_UNIT0 + unit));
        tu->SetTexture(_white.get());

        tu = dynamic_cast<TextureUnit*>(material.GetRenderState(RS_TEXTURE_UNIT0 + unit + 1));
        tu->SetTexture(_white.get());

        TextureMatrix* tm;
        tm = dynamic_cast<TextureMatrix*>(material.GetRenderState(RS_TEXTURE_MATRIX0 + unit));
        tm->SetMatrix(Matrixf());

        tm = dynamic_cast<TextureMatrix*>(material.GetRenderState(RS_TEXTURE_MATRIX0 + unit + 1));
        tm->SetMatrix(Matrixf());

        return true;
    }

    if(corner == -1)
    {
        TextureUnit* tu;
        tu = dynamic_cast<TextureUnit*>(material.GetRenderState(RS_TEXTURE_UNIT0 + unit));
        tu->SetTexture(_white.get());
    }
    else
    {
        TextureUnit* tu;
        tu = dynamic_cast<TextureUnit*>(material.GetRenderState(RS_TEXTURE_UNIT0 + unit));
        tu->SetTexture(_smOctet[corner].map.get());

        TextureMatrix* tm;
        tm = dynamic_cast<TextureMatrix*>(material.GetRenderState(RS_TEXTURE_MATRIX0 + unit));
        tm->SetMatrix(Matrixf(_smOctet[corner].texmat));
    }

    if(DynShadowMap.valid)
    {
         // Have to do inverse model view, since we are bypassing the pipeline.
        piMATRIX mv = *oglGetUnscaledViewMatrix();
        math3dInvertMatrix(&mv, &mv);

        math3dMatrixMultiply(&mv, &mv, &DynShadowMap.mvp);

        TextureUnit* tu;
        tu = dynamic_cast<TextureUnit*>(material.GetRenderState(RS_TEXTURE_UNIT0 + unit + 1));
        tu->SetTexture(DynShadowMap.map.get());

        TextureMatrix* tm;
        tm = dynamic_cast<TextureMatrix*>(material.GetRenderState(RS_TEXTURE_MATRIX0 + unit + 1));
        tm->SetMatrix(Matrixf(Matrixf(mv)));
    }
    else
    {
        TextureUnit* tu;
        tu = dynamic_cast<TextureUnit*>(material.GetRenderState(RS_TEXTURE_UNIT0 + unit + 1));
        tu->SetTexture(_white.get());
    }

    static bool iscloudy = false;
    glslUniform* u = dynamic_cast<Program*>(material.GetRenderState(RS_GLSLPROGRAM))->GetProgram()->GetUniform("shadowambient");
    u->Set(Vec3f(0.5f, 0.5f, 0.5f));

    if(iscloudy != fxWeatherIsCloudy())
    {
        iscloudy = fxWeatherIsCloudy();

        if(iscloudy)
            u->Set(Vec3f(0.75f, 0.75f, 0.75f));
    }

    return true;
}
#else
/*---------------------------------------------------------------------------
Function: ShadowMap::SetShadowStuff()
Description: For non replacement octets
----------------------------------------------------------------------------*/
bool ShadowMap::SetShadowStuff(oglMaterial& material, const Vec3d& point, int unit)
{
    int sx, sy, r, c;
    trnmainGetSupercellAndOctet(Vec3d(point), sx, sy, r, c);

    int corner = -1;

    for(int i = 0; i < 4; i++)
    {
        if(_smOctet[i].sx == sx && _smOctet[i].sy == sy && _smOctet[i].r == r && _smOctet[i].c == c)
        {
            if(!_smOctet[i].valid && !DynShadowMap.valid)
            {
                return false;
            }

            corner = i;
        }
    }

    if(corner == -1 && !DynShadowMap.valid)
    {
        material.AddTexture(unit, _white.get());
        material.AddTexture(unit + 1, _white.get());

        oglSetActiveTextureStage(unit);
        oglSetTextureMatrix(&IdentityMatrix);
        oglSetActiveTextureStage(unit + 1);
        oglSetTextureMatrix(&IdentityMatrix);

        return true;
    }

    oglSetActiveTextureStage(unit);

    if(corner == -1)
    {
        material.AddTexture(unit, _white.get());
    }
    else
    {
        material.AddTexture(unit, _smOctet[corner].map.get());
        oglSetTextureMatrix(&_smOctet[corner].texmat);
    }

    oglSetActiveTextureStage(unit + 1);
    TextureState::Enable(GL_TEXTURE_2D);

    if(DynShadowMap.valid)
    {
         // Have to do inverse model view, since we are bypassing the pipeline.
        piMATRIX mv = *oglGetUnscaledViewMatrix();
        math3dInvertMatrix(&mv, &mv);

        math3dMatrixMultiply(&mv, &mv, &DynShadowMap.mvp);

        material.AddTexture(unit + 1, DynShadowMap.map.get());
        oglSetTextureMatrix(&mv);
    }
    else
    {
        material.AddTexture(unit + 1, _white.get());
    }

    static bool iscloudy = false;
    material.GetProgram()->GetUniform("shadowambient")->Set(Vec3f(0.5f, 0.5f, 0.5f));

    if(iscloudy != fxWeatherIsCloudy())
    {
        iscloudy = fxWeatherIsCloudy();

        if(iscloudy)
            material.GetProgram()->GetUniform("shadowambient")->Set(Vec3f(0.75f, 0.75f, 0.75f));
    }

    return true;
}
#endif

/*---------------------------------------------------------------------------
Function: ShadowMap::UnApplyShadowMaps()
Description:
----------------------------------------------------------------------------*/
void ShadowMap::UnApplyShadowMaps(int unit)
{
    if(!_enabled)
        return;

    ProgramState::Bind(0);

    oglSetActiveTextureStage(unit);
    oglSetTextureMatrix(&IdentityMatrix);
    TextureState::Disable();

    oglSetActiveTextureStage(unit + 1);
    oglSetTextureMatrix(&IdentityMatrix);
    TextureState::Disable();

    oglSetWorldMatrix(&IdentityMatrix);

    oglSetActiveTextureStage(0);
}

/*---------------------------------------------------------------------------
Function: ShadowMap::UnApplyShadowMapNoShader()
Description:
----------------------------------------------------------------------------*/
void ShadowMap::UnApplyShadowMapNoShader()
{
    oglSetTextureMatrix(&IdentityMatrix);
    oglSetWorldMatrix(&IdentityMatrix);

    oglSetActiveTextureStage(_shadowTextureUnit);
    oglDisableTexture();

    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);
    glDisable(GL_TEXTURE_GEN_R);
    glDisable(GL_TEXTURE_GEN_Q);

    oglSetActiveTextureStage(0);
}

#if USE_STATESET
/*---------------------------------------------------------------------------
Function: ShadowMap::BeginShadowMap()
Description:
----------------------------------------------------------------------------*/
bool ShadowMap::BeginShadowMap()
{
    LIGHT* sun = obdliteFindDynaLight("sun");

    if(!sun)
        return false;

    smoctet.stateset->Apply();

    oglSetViewport(0, 0, _texwidth, _texheight);
    glClear(GL_DEPTH_BUFFER_BIT);

    States::LockState(RS_SHADEMODEL, 1);
    States::LockState(RS_COLORMASK, 1);
    States::LockMode(GL_LIGHTING, 1);
    States::LockMode(GL_FOG, 1);

    static float z = 64.0f;

    float height = -z;

    _StaticShadowDistance = -height * 2.0f;
    float znear = 1.0f;

    Vec3f lightdir(sun->light.dvDirection);

    Vec3f eye = smoctet.center;
    eye.x() -= (float)WorldOriginOffset.x;
    eye.y() -= (float)WorldOriginOffset.y;
    eye += lightdir * height;

    float top = z;
    float bottom = -z;
    float right = z;
    float left = -z;

    oglSetOrthoProjectionMatrix(right, left, top, bottom, znear, _StaticShadowDistance);

    Vec3f playerup = computeOrthogonalVector(lightdir);

    Vec3d center = Vec3d(smoctet.center) - WorldOriginOffset;

    camera.makeLookAt(eye, center, playerup);

    // Stupid coordinate system stuff
    Matrixf rot;
    rot(1, 1) = -1.0f;
    rot(2, 2) = -1.0f;

    camera = camera * rot;

    oldview.set(*oglGetViewMatrix());

    projection = *oglGetProjectionMatrix();
    oglSetViewMatrix(camera);

    oglLockProjectionMatrix(1);
    oglLockViewMatrix(1);

    static Matrixf bias(0.5f, 0.0f, 0.0f, 0.0f,
                        0.0f, 0.5f, 0.0f, 0.0f,
                        0.0f, 0.0f, 0.5f, 0.0f,
                        0.5f, 0.5f, 0.5f, 1.0f);

    modelviewproj = camera * projection * bias;

    return true;
}
#else
/*---------------------------------------------------------------------------
Function: ShadowMap::BeginShadowMap()
Description:
----------------------------------------------------------------------------*/
bool ShadowMap::BeginShadowMap()
{
    LIGHT* sun = obdliteFindDynaLight("sun");

    if(!sun)
        return false;

    smoctet.fbo->apply();

    oglSetViewport(0, 0, _texwidth, _texheight);
    glClear(GL_DEPTH_BUFFER_BIT);

    float factor = _polyOffset[0];
    float units =  _polyOffset[1];

    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(factor, units);

    // Turn off all bells and whistles
    oglSetRenderState(oglRENDERSTATE_LIGHTING, FALSE);
    oglSetRenderState(oglRENDERSTATE_FOGENABLE, FALSE);
    oglSetRenderState(oglRENDERSTATE_SHADEMODE, GL_FLAT);
    oglSetRenderState(oglRENDERSTATE_COLORMASK, piRGBA_MAKE(0, 0, 0, 0));

    // Lock to prevent others from changing these states
    oglLockState(oglRENDERSTATE_LIGHTING, 1);
    oglLockState(oglRENDERSTATE_FOGENABLE, 1);
    oglLockState(oglRENDERSTATE_SHADEMODE, 1);
    oglLockState(oglRENDERSTATE_COLORMASK, 1);

    static float z = 64.0f;

    float height = -z;

    _StaticShadowDistance = -height * 2.0f;
    float znear = 1.0f;

    Vec3f lightdir(sun->light.dvDirection);

    Vec3f eye = smoctet.center;
    eye.x() -= (float)WorldOriginOffset.x;
    eye.y() -= (float)WorldOriginOffset.y;
    eye += lightdir * height;

    float top = z;
    float bottom = -z;
    float right = z;
    float left = -z;

    oglSetOrthoProjectionMatrix(right, left, top, bottom, znear, _StaticShadowDistance);

    Vec3f playerup = computeOrthogonalVector(lightdir);

    Vec3d center = Vec3d(smoctet.center) - WorldOriginOffset;

    camera.makeLookAt(eye, center, playerup);

    // Stupid coordinate system stuff
    Matrixf rot;
    rot(1, 1) = -1.0f;
    rot(2, 2) = -1.0f;

    camera = camera * rot;

    oldview.set(*oglGetViewMatrix());

    projection = *oglGetProjectionMatrix();
    oglSetViewMatrix(camera);

    oglLockProjectionMatrix(1);
    oglLockViewMatrix(1);

    static Matrixf bias(0.5f, 0.0f, 0.0f, 0.0f,
                        0.0f, 0.5f, 0.0f, 0.0f,
                        0.0f, 0.0f, 0.5f, 0.0f,
                        0.5f, 0.5f, 0.5f, 1.0f);

    modelviewproj = camera * projection * bias;

    return true;
}
#endif

#if USE_STATESET
/*---------------------------------------------------------------------------
Function: ShadowMap::EndShadowMap()
Description:
----------------------------------------------------------------------------*/
void ShadowMap::EndShadowMap()
{
    oglSetViewport(0, 0, WantedWidth, WantedHeight);

    States::LockState(RS_SHADEMODEL, 0);
    States::LockState(RS_COLORMASK, 0);
    States::LockMode(GL_LIGHTING, 0);
    States::LockMode(GL_FOG, 0);

    oglLockViewMatrix(0);
    oglLockUnscaledViewMatrix(0);
    oglLockProjectionMatrix(0);
    oglLockCockpitMatrix(0);

    oglSetViewMatrix(oldview);
    gmlpHandleFieldOfView();
    rnvisSetupClipPlanes(true);
}
#else
/*---------------------------------------------------------------------------
Function: ShadowMap::EndShadowMap()
Description:
----------------------------------------------------------------------------*/
void ShadowMap::EndShadowMap()
{
    smoctet.fbo->UnApply();

    oglSetViewport(0, 0, WantedWidth, WantedHeight);

    glPolygonOffset(0, 0);
    glDisable(GL_POLYGON_OFFSET_FILL);

    oglLockState(oglRENDERSTATE_LIGHTING, 0);
    oglLockState(oglRENDERSTATE_FOGENABLE, 0);
    oglLockState(oglRENDERSTATE_SHADEMODE, 0);
    oglLockState(oglRENDERSTATE_COLORMASK, 0);

    oglSetRenderState(oglRENDERSTATE_LIGHTING, piLIGHTING_DEFAULT);
    oglSetRenderState(oglRENDERSTATE_FOGENABLE, piFOGENABLE_DEFAULT);
    oglSetRenderState(oglRENDERSTATE_SHADEMODE, ShadeMode);
    oglSetRenderState(oglRENDERSTATE_COLORMASK, piRGBA_MAKE(1, 1, 1, 1));

    oglLockViewMatrix(0);
    oglLockUnscaledViewMatrix(0);
    oglLockProjectionMatrix(0);
    oglLockCockpitMatrix(0);

    oglSetViewMatrix(oldview);
    gmlpHandleFieldOfView();
    rnvisSetupClipPlanes(true);
}
#endif

#if USE_STATESET
/*---------------------------------------------------------------------------
Function: ShadowMap::EndStaticShadowMap()
Description:
----------------------------------------------------------------------------*/
void ShadowMap::EndStaticShadowMap(int corner)
{
    oglSetViewport(0, 0, WantedWidth, WantedHeight);

    States::LockState(RS_SHADEMODEL, 0);
    States::LockState(RS_COLORMASK, 0);
    States::LockMode(GL_LIGHTING, 0);
    States::LockMode(GL_FOG, 0);

    oglLockViewMatrix(0);
    oglLockUnscaledViewMatrix(0);
    oglLockProjectionMatrix(0);
    oglLockCockpitMatrix(0);

    gmlpHandleFieldOfView();
    rnvisSetupClipPlanes(true);
}
#else
/*---------------------------------------------------------------------------
Function: ShadowMap::EndStaticShadowMap()
Description:
----------------------------------------------------------------------------*/
void ShadowMap::EndStaticShadowMap(int corner)
{
    _smOctet[corner].fbo->UnApply();

    oglSetViewport(0, 0, WantedWidth, WantedHeight);

    glPolygonOffset(0, 0);
    glDisable(GL_POLYGON_OFFSET_FILL);

    oglLockState(oglRENDERSTATE_LIGHTING, 0);
    oglLockState(oglRENDERSTATE_FOGENABLE, 0);
    oglLockState(oglRENDERSTATE_SHADEMODE, 0);
    oglLockState(oglRENDERSTATE_COLORMASK, 0);

    oglSetRenderState(oglRENDERSTATE_LIGHTING, piLIGHTING_DEFAULT);
    oglSetRenderState(oglRENDERSTATE_FOGENABLE, piFOGENABLE_DEFAULT);
    oglSetRenderState(oglRENDERSTATE_SHADEMODE, ShadeMode);
    oglSetRenderState(oglRENDERSTATE_COLORMASK, piRGBA_MAKE(1, 1, 1, 1));

    oglLockViewMatrix(0);
    oglLockUnscaledViewMatrix(0);
    oglLockProjectionMatrix(0);
    oglLockCockpitMatrix(0);

    gmlpHandleFieldOfView();
    rnvisSetupClipPlanes(true);
}
#endif

#if USE_STATESET
/*---------------------------------------------------------------------------
Function: ShadowMap::EndDynamicShadowMap()
Description:
----------------------------------------------------------------------------*/
void ShadowMap::EndDynamicShadowMap()
{
    oglSetViewport(0, 0, WantedWidth, WantedHeight);

    States::LockState(RS_SHADEMODEL, 1);
    States::LockState(RS_COLORMASK, 1);
    States::LockState(RS_FRONTFACE, 1);
    States::LockMode(GL_FOG, 1);
    States::LockMode(GL_LIGHTING, 1);

    oglLockViewMatrix(0);
    oglLockUnscaledViewMatrix(0);
    oglLockProjectionMatrix(0);
    oglLockCockpitMatrix(0);

    gmlpHandleFieldOfView();
    rnvisSetupClipPlanes(true);
}
#else
/*---------------------------------------------------------------------------
Function: ShadowMap::EndDynamicShadowMap()
Description:
----------------------------------------------------------------------------*/
void ShadowMap::EndDynamicShadowMap()
{
    DynShadowMap.fbo->UnApply();
    oglSetViewport(0, 0, WantedWidth, WantedHeight);
    glPolygonOffset(0, 0);

    oglLockState(oglRENDERSTATE_LIGHTING, 0);
    oglLockState(oglRENDERSTATE_FOGENABLE, 0);
    oglLockState(oglRENDERSTATE_SHADEMODE, 0);
    oglLockState(oglRENDERSTATE_COLORMASK, 0);

    oglSetRenderState(oglRENDERSTATE_LIGHTING, piLIGHTING_DEFAULT);
    oglSetRenderState(oglRENDERSTATE_FOGENABLE, piFOGENABLE_DEFAULT);
    oglSetRenderState(oglRENDERSTATE_SHADEMODE, ShadeMode);
    oglSetRenderState(oglRENDERSTATE_COLORMASK, piRGBA_MAKE(1, 1, 1, 1));
    oglSetRenderState(oglRENDERSTATE_CULLFACE, GL_FRONT);

    glDisable(GL_POLYGON_OFFSET_FILL);

    oglLockViewMatrix(0);
    oglLockUnscaledViewMatrix(0);
    oglLockProjectionMatrix(0);
    oglLockCockpitMatrix(0);

    gmlpHandleFieldOfView();
    rnvisSetupClipPlanes(true);
}
#endif

/*---------------------------------------------------------------------------
Function: ShadowMap::Kill()
Description:
----------------------------------------------------------------------------*/
void ShadowMap::Kill()
{
    for(int i = 0; i < 4; i++)
    {
		// TODO: It's hideously wrong to forcibly kill all existing refcounts
        while(_smOctet[i].fbo && _smOctet[i].fbo->referenceCount() > 1)
            _smOctet[i].fbo->unref();

        _smOctet[i].fbo = nullptr;

        while(_smOctet[i].map && _smOctet[i].map->referenceCount() > 1)
            _smOctet[i].map->unref();

        _smOctet[i].map = nullptr;
    }

    if(DynShadowMap.fbo.valid())
    {
        while(DynShadowMap.fbo->referenceCount() > 1)
            DynShadowMap.fbo->unref();

        DynShadowMap.fbo = nullptr;
    }

#if USE_STATESET
#else
    DynShadowMap.map = nullptr;
#endif

#if USE_STATESET
#else
    smoctet.fbo = nullptr;
#endif

    if(_white.valid())
    {
        while(_white->referenceCount() > 1)
            _white->unref();

        _white = nullptr;
    }

    smoctet.map = nullptr;
    _shadowmap = nullptr;
}

/*---------------------------------------------------------------------------
Function: ShadowMap::init()
Description:
----------------------------------------------------------------------------*/
void ShadowMap::init()
{
    smoctet.valid = false;
    smoctet.center.set(0, 0, 0);

    smoctet.map = new Texture2D;
    smoctet.map->setTextureSize(_texwidth, _texheight);

    if(GLEW_ARB_depth_buffer_float)
        smoctet.map->setInternalFormat(GL_DEPTH_COMPONENT32F);
    else
        smoctet.map->setInternalFormat(GL_DEPTH_COMPONENT24);

    smoctet.map->setSourceFormat(GL_DEPTH_COMPONENT);
    smoctet.map->setSourceType(GL_FLOAT);
    smoctet.map->setShadowComparison(true);
    smoctet.map->setShadowTextureMode(Texture2D::LUMINANCE);
    smoctet.map->setFilter(Texture2D::MIN_FILTER, Texture2D::LINEAR);
    smoctet.map->setFilter(Texture2D::MAG_FILTER, Texture2D::LINEAR);

    // The shadow comparison should fail if object is outside the texture
    smoctet.map->setWrap(Texture2D::WRAP_S, Texture2D::CLAMP_TO_BORDER);
    smoctet.map->setWrap(Texture2D::WRAP_T, Texture2D::CLAMP_TO_BORDER);
    smoctet.map->setBorderColor(Vec4f(1.0f, 1.0f, 1.0f, 1.0f));

	{
		ref_ptr<FrameBufferObject> fbo = new FrameBufferObject;
		fbo->setAttachment(FrameBufferObject::DEPTH_BUFFER, FrameBufferAttachment(smoctet.map.get()));

#if USE_STATESET
		smoctet.stateset = new RenderStateSet;
		smoctet.stateset->SetRenderState(new PolygonOffset(-_polyOffset[0], -_polyOffset[1]));
		smoctet.stateset->SetRenderState(new ColorMask(false, false, false, false));
		smoctet.stateset->SetRenderState(new ShadeModel(GL_FLAT));
		smoctet.stateset->SetRenderState(fbo);
		smoctet.stateset->SetMode(GL_FOG, 0);
		smoctet.stateset->SetMode(GL_LIGHTING, 0);
#else
		smoctet.fbo = fbo;
#endif
	}

    _white = new Texture2D;
    _white->setTextureSize(4, 4);
    _white->setInternalFormat(GL_DEPTH_COMPONENT);
    _white->setShadowComparison(true);
    _white->setShadowTextureMode(Texture2D::LUMINANCE);
    _white->setFilter(Texture2D::MIN_FILTER, Texture2D::LINEAR);
    _white->setFilter(Texture2D::MAG_FILTER, Texture2D::LINEAR);

    // The shadow comparison should fail if object is outside the texture
    _white->setWrap(Texture2D::WRAP_S, Texture2D::CLAMP_TO_BORDER);
    _white->setWrap(Texture2D::WRAP_T, Texture2D::CLAMP_TO_BORDER);
    _white->setBorderColor(Vec4f(1.0f, 1.0f, 1.0f, 1.0f));

	{
		ref_ptr<FrameBufferObject> fbo = new FrameBufferObject;
		fbo->setAttachment(FrameBufferObject::DEPTH_BUFFER, FrameBufferAttachment(_white.get()));

		// Make texture 'white'
		fbo->apply();
		oglSetViewport(0, 0, 4, 4);
		glClear(GL_DEPTH_BUFFER_BIT);
		fbo->UnApply();
	}

    oglSetViewport(0, 0, WantedWidth, WantedHeight);

    if(ogl::UseDeprecatedPath())
    {
        // Keep 4 octets of shadow mappings
        for(int i = 0; i < 4; i++)
        {
            _smOctet[i].valid = false;
            _smOctet[i].sx = -1;
            _smOctet[i].sy = -1;
            _smOctet[i].r = -1;
            _smOctet[i].c = -1;
            _smOctet[i].center.set(0, 0, 0);
            _smOctet[i].mvp = IdentityMatrix;

            _smOctet[i].map = new Texture2D;
            _smOctet[i].map->setTextureSize(_texwidth, _texheight);
            _smOctet[i].map->setInternalFormat(GL_DEPTH_COMPONENT);
            _smOctet[i].map->setShadowComparison(true);
            _smOctet[i].map->setShadowTextureMode(Texture2D::LUMINANCE);
            _smOctet[i].map->setFilter(Texture2D::MIN_FILTER, Texture2D::LINEAR);
            _smOctet[i].map->setFilter(Texture2D::MAG_FILTER, Texture2D::LINEAR);

            // The shadow comparison should fail if object is outside the texture
            _smOctet[i].map->setWrap(Texture2D::WRAP_S, Texture2D::CLAMP_TO_BORDER);
            _smOctet[i].map->setWrap(Texture2D::WRAP_T, Texture2D::CLAMP_TO_BORDER);
            _smOctet[i].map->setBorderColor(Vec4f(1.0f, 1.0f, 1.0f, 1.0f));

			{
				ref_ptr<FrameBufferObject> fbo = new FrameBufferObject;
				ref_ptr<FrameBufferAttachment> fba = new FrameBufferAttachment(_smOctet[i].map.get());
				fbo->setAttachment(FrameBufferObject::DEPTH_BUFFER, *(fba.get()));

#if USE_STATESET
				_smOctet[i].stateset = new RenderStateSet;
				_smOctet[i].stateset->SetRenderState(new PolygonOffset(-_polyOffset[0], -_polyOffset[1]));
				_smOctet[i].stateset->SetRenderState(new ColorMask(false, false, false, false));
				_smOctet[i].stateset->SetRenderState(new ShadeModel(GL_FLAT));
				_smOctet[i].stateset->SetRenderState(fbo);
				_smOctet[i].stateset->SetMode(GL_FOG, 0);
				_smOctet[i].stateset->SetMode(GL_LIGHTING, 0);
#else
				_smOctet[i].fbo = fbo;
#endif
			}
        }

        DynShadowMap.valid = false;
        DynShadowMap.center.set(0, 0, 0);
        DynShadowMap.mvp = IdentityMatrix;

        DynShadowMap.map = new Texture2D;
        DynShadowMap.map->setTextureSize(DynamicTexWidth, DynamicTexHeight);
        DynShadowMap.map->setInternalFormat(GL_DEPTH_COMPONENT);
        DynShadowMap.map->setShadowComparison(true);
        DynShadowMap.map->setShadowTextureMode(Texture2D::LUMINANCE);
        DynShadowMap.map->setFilter(Texture2D::MIN_FILTER, Texture2D::LINEAR);
        DynShadowMap.map->setFilter(Texture2D::MAG_FILTER, Texture2D::LINEAR);

        // The shadow comparison should fail if object is outside the texture
        DynShadowMap.map->setWrap(Texture2D::WRAP_S, Texture2D::CLAMP_TO_BORDER);
        DynShadowMap.map->setWrap(Texture2D::WRAP_T, Texture2D::CLAMP_TO_BORDER);
        DynShadowMap.map->setBorderColor(Vec4f(1.0f, 1.0f, 1.0f, 1.0f));

		{
			ref_ptr<FrameBufferObject> fbo = new FrameBufferObject;
			ref_ptr<FrameBufferAttachment> fba = new FrameBufferAttachment(DynShadowMap.map.get());
			fbo->setAttachment(FrameBufferObject::DEPTH_BUFFER, *fba.get());

#if USE_STATESET
			DynShadowMap.stateset = new RenderStateSet;
			DynShadowMap.stateset->SetRenderState(new PolygonOffset(-_polyOffset[0], -_polyOffset[1]));
			DynShadowMap.stateset->SetRenderState(new ColorMask(false, false, false, false));
			DynShadowMap.stateset->SetRenderState(new ShadeModel(GL_FLAT));
			DynShadowMap.stateset->SetRenderState(new FrontFace(GL_BACK));
			DynShadowMap.stateset->SetRenderState(fbo);
			DynShadowMap.stateset->SetMode(GL_FOG, 0);
			DynShadowMap.stateset->SetMode(GL_LIGHTING, 0);
#else
			DynShadowMap.fbo = fbo;
#endif
		}

#if(0)
        _white = new Texture2D;
        _white->setTextureSize(4, 4);
        _white->setInternalFormat(GL_DEPTH_COMPONENT);
        _white->setShadowComparison(true);
        _white->setShadowTextureMode(Texture2D::LUMINANCE);
        _white->setFilter(Texture2D::MIN_FILTER, Texture2D::LINEAR);
        _white->setFilter(Texture2D::MAG_FILTER, Texture2D::LINEAR);

        // The shadow comparison should fail if object is outside the texture
        _white->setWrap(Texture2D::WRAP_S, Texture2D::CLAMP_TO_BORDER);
        _white->setWrap(Texture2D::WRAP_T, Texture2D::CLAMP_TO_BORDER);
        _white->setBorderColor(Vec4f(1.0f, 1.0f, 1.0f, 1.0f));
#endif

		{
			FrameBufferObject* fbo = new FrameBufferObject;
			ref_ptr<FrameBufferAttachment> fba = new FrameBufferAttachment(_white.get());
			fbo->setAttachment(FrameBufferObject::DEPTH_BUFFER, *fba.get());

			// Make texture 'white'
			fbo->apply();
			oglSetViewport(0, 0, 4, 4);
			glClear(GL_DEPTH_BUFFER_BIT);
			fbo->UnApply();
		}

        oglSetViewport(0, 0, WantedWidth, WantedHeight);
        _program = GLSL.getProgram("shadow");
    }
}
