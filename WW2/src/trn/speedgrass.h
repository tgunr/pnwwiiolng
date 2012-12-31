#pragma once

#include "math/Vec3d.h"
#include "math/Vec4d.h"
#include "trn/trnmain.h"
#include "ogl/oglVertexBuffer.h"
#include "ogl/oglMaterial.h"
#include "ogl/oglTextureObject.h"
#include "physics/colliderQuery.h"

#include "wwii/wwiiHash.h"
USING_EXT_NAMESPACE;

#if USE_STATESET
#include "ogl/States.h"
#endif

#define USE_GL_TRIANGLES    1

#include <map>
#include <queue>

struct CollideQue
{
    Vec3d pos;
    std::string type;
    size_t key;
    trnCDTerrainObject* to;
    OBJECT* o;
    int starttime;
    float fade;
    bool wantcollide;
};

#if platOS == OS_MACOS
namespace __gnu_cxx
{
    template<> struct hash<dGeomID>  { size_t operator()(dGeomID o) const  { return *(size_t*)(&o); } };

};
#endif

struct Patch
{
    float cullradius;

    int indiceoffset;
    int density;
    int vertexcount;
    int uvoffset;
    int zoffset;
    int h;
    int w;
    int type;

    size_t key;

    colliderQuery* collider;

    std::vector<float> params;
    std::vector<short> indices;

    std::vector<Vec2f> collide;
    stdext::hash_map<dGeomID, int> geommap;

    int vboid;

    Patch() :
        indiceoffset(0),
        vertexcount(0),
        density(0),
        h(0),
        w(0),
        uvoffset(0),
        zoffset(0),
        vboid(0)
    {
    }

    ~Patch()
    {
        if(collider)
        {
            delete collider;
            collider = NULL;
        }
    }
};

struct PatchInstance
{
    Vec3d center;
    Vec4f rot;

    unsigned int lastdrawtime;

    int primcount;
    int lod;
    int type;
    int indiceoffset;

#if USE_GL_TRIANGLES
    int offset;
#endif

    size_t key;

    bool collided;
    bool usematrix;

    float cullradius;
    float colorscale;
    float fadedistance;
    float distance;
    float alpha;
    float faderate;

    int splatid;
    int userwanteddensity;

    const Patch* instanceof;

    std::vector<float> params;
    std::vector<float> Z;

#if USE_GL_TRIANGLES
    std::vector<unsigned short> indices;
#endif

	PatchInstance() :
        indiceoffset(0),
        alpha(0),
        faderate(1.0f),
        lastdrawtime(0),
        lod(0),
        collided(false),
        usematrix(false),
        colorscale(1),
        cullradius(0),
        primcount(0),
        key(0),
        splatid(-1),
        instanceof(NULL),
        userwanteddensity(0)
	{
        rot.set(1, 0, 0, 0);
	}

    ~PatchInstance()
    {
    }
};

class SpeedGrassDeprecated
{
public:

    enum
    {
        type_radclutter,
        type_clutter,
        type_grass,
        type_cover,
        type_fltgrass,
        type_pigeon,
        type_userdefined,
        max_clutter_types
    };

    SpeedGrassDeprecated() :
        updatetime(0)
        {
        }

    virtual ~SpeedGrassDeprecated() {}

    virtual void Update();
    virtual void Cull();
    virtual void Draw();
    virtual void DrawCustom();
    virtual void DrawForShadows();
    virtual void DrawForCustomShadows();

    virtual void Init();

    void CreateGrassQuads(PatchInstance& instance, const Vec3d& pos, float scale, int texture = -1, float shadow = 1.0f);
    void CreateGrassQuads(Patch& instance, const Vec3d& pos, float scale, int texture = -1, float shadow = 1.0f);

    virtual bool speedgrassPut(const Vec3d& pos, PatchInstance& instance, float scale = 1.0f, int texture = -1, float shadow = 1.0f);

    virtual bool GrassFromHeightMap(CollideQue* que);
    virtual bool CollideWithTerrain(CollideQue* que);
    virtual bool CollideTheOldWay(CollideQue* que, const Patch& grass);

    virtual void LoadBMP(const char* filename,
                         const char* type,
                         int cluttertype,
                         float h,
                         float patchwidth,
                         float scalemin,
                         float scalemax,
                         const std::vector<int>& texchoices);

    virtual void CreatePatch(const char* key,
                             int cluttertype,
                             float w,
                             float h,
                             float bladeheight,
                             int density);

    void SetUpdateTime(int time) {updatetime = time;}
    int GetUpdateTime() {return updatetime;}

    PatchInstance& GetOrCreateInstance(unsigned int key, bool& created)
    {
        GrassMap::iterator itr = grassMap.find(key);

        if(itr != grassMap.end())
        {
            created = false;
            return *itr->second;
        }
        else
        {
            PatchInstance* instance = new PatchInstance;
            instance->key = key;
            grassMap[key] = instance;
            created = true;

            return *instance;
        }
    }

    void EraseInstance(unsigned int key)
    {
        if(!grassMap.empty())
        {
            GrassMap::iterator itr = grassMap.find(key);

            if(itr != grassMap.end())
            {
                delete itr->second;
                grassMap.erase(itr);
            }
        }
    }

    void Kill()
    {
        if(grassMap.empty())
            return;

        GrassMap::iterator itr = grassMap.begin();

        for(; itr != grassMap.end(); ++itr)
        {
            delete itr->second;
        }

        grassMap.clear();
    }

    void InitVBO() {vbo = new oglVertexBuffer;}

    ref_ptr<oglVertexBuffer> VBO() {return vbo;}

protected:
#if USE_STATESET
    ref_ptr<RenderStateSet> grassMaterial;
    ref_ptr<RenderStateSet> grassShadowMaterial;
#else
    ref_ptr<oglMaterial> grassMaterial;
    ref_ptr<oglMaterial> grassShadowMaterial;
#endif
    int updatetime;

    std::queue<int> listids;

    typedef std::map<unsigned int, PatchInstance*> GrassMap;
    GrassMap grassMap;
    ref_ptr<oglVertexBuffer> vbo;
};

class SpeedGrass : public SpeedGrassDeprecated
{
public:
    SpeedGrass() : SpeedGrassDeprecated()
    {
    }

    virtual ~SpeedGrass()
    {
    }

    virtual void Init();
    virtual void Draw();
    virtual void Cull();

    void CreateGrassVBO(float radius, float height);

    virtual bool CollideWithTerrain(CollideQue* que);

    bool speedgrassPut(const Vec3d& pos, PatchInstance& instance, float scale = 1.0f, int texture = -1);
};

void speedgrassUpdate();
void speedgrassCull();
void speedgrassDrawDeprecated();
void speedgrassDraw();
void speedgrassInit();
void speedgrassToggle(bool want);
void speedgrassKill();
void speedgrassClear();
void speedgrassGetStats(char buff[]);
void speedgrassRemoveAll();
bool speedgrassRemove(const char* key);
bool speedgrassRemove(size_t key);
bool speedgrassAddAndCollide(const Vec3d& pos, const char* type, const char* key, float fade, trnCDTerrainObject* to = NULL, OBJECT* o = NULL);
bool speedgrassAddAndCollide(const Vec3d& pos, const char* type, size_t key, float fade, trnCDTerrainObject* to = NULL, OBJECT* o = NULL);
bool speedgrassAddCoverGrass(const Vec3d& pos,
                             int type = SpeedGrassDeprecated::type_grass,
                             unsigned int key = 0,
                             float scale = 1.0f,
                             int texture = -1,
                             float alpha = 1.0f,
                             int splatid = -1);
bool speedgrassAddCoverGrass(const Vec3d& pos, PatchInstance& instance, int type = SpeedGrassDeprecated::type_grass, float scale = 1.0f, int texture = -1);

PatchInstance* speedgrassGetOrCreateInstance(unsigned int key, bool& created);

#if USE_HEIGHT_AT
bool speedgrassAddPatch(const Vec3d& pos, const char* type, const char* key, float fade);
#endif
const char* speedgrassGetGrassByPos(const Vec4d& pos);
const char* speedgrassGetClutterByPos(const Vec4d& pos);
const char* speedgrassGetRandomRadialType();
