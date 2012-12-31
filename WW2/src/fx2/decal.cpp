#include <syscfg.h> // Path is project specific.
//////////////////////////////////////////////////////////////////////
// Module: decal.cpp
// Description:
//////////////////////////////////////////////////////////////////////
#include "pi/pi.h"
#include "include/vars.h"
#include "ld/ldpix.h"
#include "ob/obmain.h"
#include "random.h"
#include "rn/rnvis.h"
#include "trn/trnmain.h"
#include "math/math3d.h"
#include "math/mathvect.h"
#include "math/mathnum.h"
#include "ob/obdlite.h"
#include "ob/obcd.h"
#include "weap/weapammo.h"
#include "ml/mlutility.h"
#include "ob/obShader.h"

#define MAX_DECALS  128

#include <map>
#include <vector>
#include <queue>

#include "shader/Shader.h"
#include "fx2/fxdata.h"
#include "fx2/decal.h"

static std::deque<DECAL *> _freedecals;

static SINT32 _decalid = 1;

DecalMgr _decalmgr;

extern "C" SINT32 DefaultMaterialId;

//////////////////////////////////////////////////////////////////////
// Function: Decal
// Description:
//////////////////////////////////////////////////////////////////////
DecalMgr::DecalMgr()
{
}

//////////////////////////////////////////////////////////////////////
// Function: ~Decal
// Description:
//////////////////////////////////////////////////////////////////////
DecalMgr::~DecalMgr()
{
}

//////////////////////////////////////////////////////////////////////
// Function: init
// Description:
//////////////////////////////////////////////////////////////////////
void DecalMgr::init()
{
    _indices.push_back(0);
    _indices.push_back(1);
    _indices.push_back(2);

    _indices.push_back(0);
    _indices.push_back(2);
    _indices.push_back(3);

    for(SINT32 i = 0; i < MAX_DECALS; i++)
    {
        DECAL* d = new DECAL;
        d->id = -(i + 1); // Make negative to identity decals over other effects.

        if(i)
        {
            d->zdepth = zdepthLAYER0 * i;
        }
        else
        {
            d->zdepth = zdepthLAYER0;
        }

        _freedecals.push_back(d);
    }
}

//////////////////////////////////////////////////////////////////////
// Function: kill
// Description:
//////////////////////////////////////////////////////////////////////
void DecalMgr::kill()
{
    _indices.clear();

    std::map<SINT32, DECAL *>::iterator free;

    while(!_decals.empty())
    {
        free = _decals.begin();

        delete free->second;

        _decals.erase(free);
    }

    while(!_freedecals.empty())
    {
        DECAL* d = _freedecals.front();
        _freedecals.pop_front();

        delete d;
    }

    Write_Log(LOG_OLD, LOG_INFO, "[DecalMgr::kill]");
}

//////////////////////////////////////////////////////////////////////
// Function: remove
// Description:
//////////////////////////////////////////////////////////////////////
void DecalMgr::remove(SINT32 key)
{
    std::map<SINT32, DECAL *>::iterator find = _decals.find(key);

    if(find == _decals.end())
    {
        return;
    }

    find->second->dead = true;
}

//////////////////////////////////////////////////////////////////////
// Function: setupPoint
// Description:
//////////////////////////////////////////////////////////////////////
void DecalMgr::setupPoint(piVECTOR& v, FLOAT32 x, FLOAT32 y, FLOAT32 z, DECAL* d)
{
    // Put the point against the hit object
    piVECTOR pt = {x, y, z};

    math3dVectorMultiply(&v, &pt, &d->world);

    piMATRIX inverse;
    math3dInvertMatrix(&inverse, &d->o->mat);

    // ... then bring to object space
    math3dVectorMultiply(&v, &v, &inverse);
}

//////////////////////////////////////////////////////////////////////
// Function: getuv
// Description:
//////////////////////////////////////////////////////////////////////
void DecalMgr::getuv(OBJECT* o, SINT32 polygon, const piVECTOR& pos, piUV& uv)
{
    FLOAT32 L1,L2;
    FLOAT32 Delta;
    FLOAT32 N1, N2;
    FLOAT32 u1, v1, u2, v2;
    piUV Ver1, Ver2, Ver3;
    piVECTOR const* pcn;
    piINDICES* CurrentTri;

    piTexCoordConstPtr objUV = o->RenderTexCoords();
	piVectorConstPtr objPos = o->RenderPositions();
    CurrentTri = &o->ptris[polygon];
    pcn = &o->pscdn[CurrentTri->v1];

    Ver1 = objUV[CurrentTri->v1];
    Ver2 = objUV[CurrentTri->v2];
    Ver3 = objUV[CurrentTri->v3];

    piVECTOR V1 = objPos[CurrentTri->v1];
	piVECTOR V2 = objPos[CurrentTri->v2];
	piVECTOR V3 = objPos[CurrentTri->v3];

    piVECTOR Vec1, Vec2;
    piVECTOR P = pos;

	mathSubVectors(&Vec1,&V3,&V1);
	N1 = -math3dDotProduct(&pcn[0],&Vec1);

    FLOAT32 diff = math3dDotProduct(&Vec1, &Vec1) - N1 * N1;

    if(diff < 0.000001f)
    {
        L1 = 0.0f;
    }
    else
    {
		L1 = (FLOAT32)sqrt(diff);
    }

    mathSubVectors(&Vec2,&V2,&V1);
	L2 = (FLOAT32)mathExactVectorLength(&Vec2);
	Delta = L1 / L2;
	u1 = (Ver2.u - Ver1.u) * Delta + Ver1.u;
	v1 = (Ver2.v - Ver1.v) * Delta + Ver1.v;

	mathSubVectors(&Vec1,&P,&V1);
	N2 = -math3dDotProduct(&pcn[0],&Vec1);
	Delta = N2 / N1;
	u2 = (Ver3.u - u1) * Delta;
	v2 = (Ver3.v - v1) * Delta;

	math3dNormalize(&Vec2);
	mathSubVectors(&Vec1,&P,&V1);
	N1 = math3dDotProduct(&Vec1,&Vec2);
	Delta = N1 / L2;
	u1 = (Ver2.u - Ver1.u) * Delta + Ver1.u;
	v1 = (Ver2.v - Ver1.v) * Delta + Ver1.v;

	u1 += u2;
	v1 += v2;

    uv.u = u1;
    uv.v = v1;
}

//////////////////////////////////////////////////////////////////////
// Function: findintersections
// Description:
//////////////////////////////////////////////////////////////////////
void DecalMgr::setup(DECAL* d)
{
    FLOAT32 radius = 0.07f;

    piVECTOR pt1;
    piVECTOR pt2;

    setupPoint(pt1, -radius, radius, -0.5f, d);
    setupPoint(pt2, -radius, radius,  0.5f, d);

    piMATRIX worldI;
    math3dInvertMatrix(&worldI, &d->world);

    UINT32 numcollisions = 32;

    FASTCOLLIDE collidelist[32];
    FASTCOLLIDE* list = collidelist;

    // Need 4 collisions for uv coords of basecolor
    trncdCollideWithSpecificTerrainObject(&list,
                                          &numcollisions,
                                          d->o,
                                          d->o,
                                          NULL,
                                          NULL,
                                          &pt2,
                                          &pt1,
                                          0);

    if(numcollisions == 32)
    {
        d->dead = true;
        return;
    }

    if(32 - numcollisions > 1)
    {
        mlUtilitySortCollideList(collidelist, 32 - numcollisions);
    }

    piUV uv;

    getuv(collidelist[0].CollideObject, collidelist[0].PolyNum, collidelist[0].CollidePnt, uv);

    UINT32 alpha;
    piGetTextureUVAlpha(&alpha, collidelist[0].CollideObject->texid, 0, uv.u, uv.v);

    if(alpha < 128)
    {
        d->dead = true;
        return;
    }

    d->_data.push_back(0.0f);       // u
    d->_data.push_back(1.0f);       // v
    d->_data.push_back(-radius);    // x
    d->_data.push_back(radius);     // y
    d->_data.push_back(d->zdepth);  // z
    d->_data.push_back(0);          // nx
    d->_data.push_back(0);          // ny
    d->_data.push_back(1);          // nz
    d->_data.push_back(1);          // tangent x
    d->_data.push_back(0);          // tangent y
    d->_data.push_back(0);          // tangent z
    d->_data.push_back(1);          // tangent w
    d->_data.push_back(uv.u);
    d->_data.push_back(uv.v);

    setupPoint(pt1, -radius, -radius, -0.5f, d);
    setupPoint(pt2, -radius, -radius,  0.5f, d);

    list = collidelist;
    numcollisions = 32;

    // Need 4 collisions for uv coords of basecolor
    trncdCollideWithSpecificTerrainObject(&list,
                                          &numcollisions,
                                          d->o,
                                          d->o,
                                          NULL,
                                          NULL,
                                          &pt2,
                                          &pt1,
                                          0);

    if(numcollisions == 32)
    {
        d->dead = true;
        return;
    }

    if(32 - numcollisions > 1)
    {
        mlUtilitySortCollideList(collidelist, 32 - numcollisions);
    }

    getuv(collidelist[0].CollideObject, collidelist[0].PolyNum, collidelist[0].CollidePnt, uv);

    piGetTextureUVAlpha(&alpha, collidelist[0].CollideObject->texid, 0, uv.u, uv.v);

    if(alpha < 128)
    {
        d->dead = true;
        return;
    }

    d->_data.push_back(0.0f);
    d->_data.push_back(0.0f);
    d->_data.push_back(-radius);
    d->_data.push_back(-radius);
    d->_data.push_back(d->zdepth);
    d->_data.push_back(0);
    d->_data.push_back(0);
    d->_data.push_back(1);
    d->_data.push_back(1);
    d->_data.push_back(0);
    d->_data.push_back(0);
    d->_data.push_back(1);
    d->_data.push_back(uv.u);
    d->_data.push_back(uv.v);

    setupPoint(pt1, radius, -radius, -0.5f, d);
    setupPoint(pt2, radius, -radius,  0.5f, d);

    list = collidelist;
    numcollisions = 32;

    // Need 4 collisions for uv coords of basecolor
    trncdCollideWithSpecificTerrainObject(&list,
                                          &numcollisions,
                                          d->o,
                                          d->o,
                                          NULL,
                                          NULL,
                                          &pt2,
                                          &pt1,
                                          0);

    if(numcollisions == 32)
    {
        d->dead = true;
        return;
    }

    if(32 - numcollisions > 1)
    {
        mlUtilitySortCollideList(collidelist, 32 - numcollisions);
    }

    getuv(collidelist[0].CollideObject, collidelist[0].PolyNum, collidelist[0].CollidePnt, uv);

    piGetTextureUVAlpha(&alpha, collidelist[0].CollideObject->texid, 0, uv.u, uv.v);

    if(alpha < 128)
    {
        d->dead = true;
        return;
    }

    d->_data.push_back(1.0f);
    d->_data.push_back(0.0f);
    d->_data.push_back(radius);
    d->_data.push_back(-radius);
    d->_data.push_back(d->zdepth);
    d->_data.push_back(0);
    d->_data.push_back(0);
    d->_data.push_back(1);
    d->_data.push_back(1);
    d->_data.push_back(0);
    d->_data.push_back(0);
    d->_data.push_back(1);
    d->_data.push_back(uv.u);
    d->_data.push_back(uv.v);

    setupPoint(pt1, radius, radius, -0.5f, d);
    setupPoint(pt2, radius, radius,  0.5f, d);

    list = collidelist;
    numcollisions = 32;

    // Need 4 collisions for uv coords of basecolor
    trncdCollideWithSpecificTerrainObject(&list,
                                          &numcollisions,
                                          d->o,
                                          d->o,
                                          NULL,
                                          NULL,
                                          &pt2,
                                          &pt1,
                                          0);

    if(numcollisions == 32)
    {
        d->dead = true;
        return;
    }

    if(32 - numcollisions > 1)
    {
        mlUtilitySortCollideList(collidelist, 32 - numcollisions);
    }

    getuv(collidelist[0].CollideObject, collidelist[0].PolyNum, collidelist[0].CollidePnt, uv);

    piGetTextureUVAlpha(&alpha, collidelist[0].CollideObject->texid, 0, uv.u, uv.v);

    if(alpha < 128)
    {
        d->dead = true;
        return;
    }

    d->_data.push_back(1.0f);
    d->_data.push_back(1.0f);
    d->_data.push_back(radius);
    d->_data.push_back(radius);
    d->_data.push_back(d->zdepth);
    d->_data.push_back(0);
    d->_data.push_back(0);
    d->_data.push_back(1);
    d->_data.push_back(1);
    d->_data.push_back(0);
    d->_data.push_back(0);
    d->_data.push_back(1);
    d->_data.push_back(uv.u);
    d->_data.push_back(uv.v);

    d->uvindex = 0;
    d->vertexindex = 2;
    d->normalindex = 5;
    d->tangentindex = 8;
    d->hitmapuv = 12;
    d->stride = 56;
}

//////////////////////////////////////////////////////////////////////
// Function: add
// Description:
//////////////////////////////////////////////////////////////////////
SINT32 DecalMgr::add(OBJECT* o, const piVECTOR& pos, SINT32 polygon)
{
    if(_freedecals.empty())
    {
        return 0;
    }

    DECAL* d = _freedecals.front();
    _freedecals.pop_front();

    d->dead = false;

    math3dVectorMultiply(&d->pos, &pos, &o->mat);

    d->createtime = frametime;
    d->lifetime = 20000;
    d->textureindex = GetRandom(31, 32);
    d->alpha = 1.0f;
    d->o = o;
    d->polygon = polygon;

    math3dMultiplyRotation(&d->normal, &o->psnorms[polygon], &o->mat);

    // Create rotate matrix from normal
    getMatrixFromNormal(*d);

    setup(d);

    _decals.insert(std::make_pair(d->id, d));

    return d->id;
}

//////////////////////////////////////////////////////////////////////
// Function: getMatrixFromNormal
// Description:
//////////////////////////////////////////////////////////////////////
void DecalMgr::getMatrixFromNormal(DECAL& decal)
{
    // Find matrix to take normal and put in decal object space
    mathVectorToZAxis(&decal.normal, &decal.world);

    // Random rotate around Z
    piMATRIX rot = identity;
    FLOAT32 radian = GetRandom(0.0f, PI);

    rot._11 =  cosf(radian);
    rot._12 = -sinf(radian);
    rot._21 = -rot._12;
    rot._22 =  rot._11;

    math3dMatrixMultiply(&decal.world, &decal.world, &rot);

    // Invert it to find the world matrix to put object space decal in world space
    math3dInvertMatrix(&decal.world, &decal.world);

    decal.world._41 = decal.pos.x;
    decal.world._42 = decal.pos.y;
    decal.world._43 = decal.pos.z;
}

//////////////////////////////////////////////////////////////////////
// Function: _setstates
// Description:
//////////////////////////////////////////////////////////////////////
void _setstates(bool enable)
{
    static UINT32 lighton = 0;
    static UINT32 blendon = 0;
    static UINT32 zon = 0;
    static UINT32 dest = 0;
    static UINT32 src = 0;
    static UINT32 zwrite = 0;
    static UINT32 cull = 0;

    if(enable)
    {
        piDisableRenderStates();

        lighton = piGetEngineRenderState(piRENDERSTATE_LIGHTING);
        blendon = piGetEngineRenderState(piRENDERSTATE_ALPHABLENDENABLE);
        zon = piGetEngineRenderState(piRENDERSTATE_ZENABLE);
        dest = piGetEngineRenderState(piRENDERSTATE_DESTBLEND);
        src = piGetEngineRenderState(piRENDERSTATE_SRCBLEND);
        zwrite = piGetEngineRenderState(piRENDERSTATE_ZWRITEENABLE);
        cull = piGetEngineRenderState(piRENDERSTATE_CULLMODE);

        piSetRenderState(piRENDERSTATE_LIGHTING, TRUE);
        piSetRenderState(piRENDERSTATE_ALPHABLENDENABLE, TRUE);
 	    piSetRenderState(piRENDERSTATE_ZWRITEENABLE, zwrite);
        piSetRenderState(piRENDERSTATE_CULLMODE, piCULLMODE_DEFAULT);

        piSetRenderStageState(0, piTSS_ADDRESSU, piTADDRESS_CLAMP);
        piSetRenderStageState(0, piTSS_ADDRESSV, piTADDRESS_CLAMP);

        piSetRenderState(piRENDERSTATE_CLIENT_TEXTURE, 1);
        piSetRenderStageState(1, piTSS_ENABLE_TEXCOORD_ARRAY, 1);
        piSetRenderState(piRENDERSTATE_CLIENT_TEXTURE, 0);

        if(GLEW_ARB_fragment_program)
        {
            piSetRenderState(piRENDERSTATE_CLIENT_TEXTURE, 2);
            piSetRenderStageState(2, piTSS_ENABLE_TEXCOORD_ARRAY, 1);
            piSetRenderState(piRENDERSTATE_CLIENT_TEXTURE, 0);
        }

    }
    else
    {
        piSetRenderState(piRENDERSTATE_CLIENT_TEXTURE, 1);
        piSetRenderStageState(1, piTSS_ENABLE_TEXCOORD_ARRAY, 0);
        piSetRenderState(piRENDERSTATE_CLIENT_TEXTURE, 0);

        if(GLEW_ARB_fragment_program)
        {
            piSetRenderState(piRENDERSTATE_CLIENT_TEXTURE, 2);
            piSetRenderStageState(2, piTSS_ENABLE_TEXCOORD_ARRAY, 0);
            piSetRenderState(piRENDERSTATE_CLIENT_TEXTURE, 0);
        }

        piSetRenderStageState(0, piTSS_ADDRESSU, piTADDRESS_WRAP);
        piSetRenderStageState(0, piTSS_ADDRESSV, piTADDRESS_WRAP);

        piSetRenderState(piRENDERSTATE_LIGHTING, lighton);
        piSetRenderState(piRENDERSTATE_ALPHABLENDENABLE, blendon);
        piSetRenderState(piRENDERSTATE_ZENABLE, zon);
        piSetRenderState(piRENDERSTATE_SRCBLEND, src);
        piSetRenderState(piRENDERSTATE_DESTBLEND, dest);
        piSetRenderState(piRENDERSTATE_ZWRITEENABLE, zwrite);
        piSetRenderState(piRENDERSTATE_CULLMODE, cull);
    }
}

//////////////////////////////////////////////////////////////////////
// Function: draw
// Description:
//////////////////////////////////////////////////////////////////////
void DecalMgr::draw()
{
    update();

    if(_drawlist.empty())
    {
        return;
    }

    static LPMATERIAL mat = NULL;

    if(!mat)
    {
        piGetMaterials(&mat);
    }

    _setstates(true);

    piPushMatrix(WORLD_MATRIX);

    SINT32 matid;

    for(UINT32 i = 0; i < _drawlist.size(); i++)
    {
        DECAL* d = _drawlist[i];

        if(piIsMaterial(d->o->matid))
        {
            matid = d->o->matid;
        }
        else
        {
            matid = DefaultMaterialId;
        }

        if(!GLEW_ARB_fragment_program)
        {
            piSetWorldMatrix(&d->world);
            piSetActiveMaterial(matid);
            piSetActiveTexture(_fxdata._bullettex, 0);

            piDrawScene(piPT_TRIANGLELIST,
                        &d->_data[d->vertexindex],
                        &d->_data[d->normalindex],
                        &d->_data[d->uvindex],
                        NULL,
                        &_indices[0],
                        d->stride,
                        4,
                        6);

            continue;
        }

//FIXME******************
        piSetRenderState(piRENDERSTATE_CLIENT_TEXTURE, 1);
        piSetRenderStageState(1, piTSS_ENABLE_TEXCOORD_ARRAY, 1);
        glTexCoordPointer(4, GL_FLOAT, d->stride, (GLvoid*)&d->_data[d->tangentindex]);
        piSetRenderState(piRENDERSTATE_CLIENT_TEXTURE, 0);

        piSetRenderState(piRENDERSTATE_CLIENT_TEXTURE, 2);
        piSetRenderStageState(2, piTSS_ENABLE_TEXCOORD_ARRAY, 1);
        glTexCoordPointer(2, GL_FLOAT, d->stride, (GLvoid*)&d->_data[d->hitmapuv]);
        piSetRenderState(piRENDERSTATE_CLIENT_TEXTURE, 0);

        // Send to generic 'parallax any' function.
        obShaderDoParallaxMapping(&d->_data[d->vertexindex],
                                  &d->_data[d->normalindex],
                                  &d->_data[d->uvindex],
                                  NULL,
                                  &_indices[0],
                                  1.0f,
                                  &d->world,
                                  matid,
                                  _fxdata._bullettex,
                                  d->stride,
                                  4,
                                  6);
    }

    _setstates(false);

    piPopMatrix(WORLD_MATRIX);
}

//////////////////////////////////////////////////////////////////////
// Function: update
// Description:
//////////////////////////////////////////////////////////////////////
void DecalMgr::update()
{
    std::map<SINT32, DECAL *>::iterator b = _decals.begin();
    std::map<SINT32, DECAL *>::iterator free;

    _drawlist.erase(_drawlist.begin(), _drawlist.end());

    while(b != _decals.end())
    {
        if(b->second->expired())
        {
            b->second->dead = true;
        }

        if(b->second->dead)
        {
            free = b;
            ++b;

            _freedecals.push_back(free->second);
            _decals.erase(free);

            continue;
        }

        if(rnvisVectorFOVCheck(&b->second->pos, 0.5f))
        {
            _drawlist.push_back(b->second);
        }

        ++b;
    }
}
