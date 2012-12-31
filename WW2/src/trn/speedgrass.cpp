#include <syscfg.h> // Path is project specific.
/*------------------------------------------------------------------
Module: speedgrass.cpp
Description:
-------------------------------------------------------------------*/
#include "trn/speedgrass.h"
#include "pers/persmain.h"
#include "math/math3d.h"
#include "trn/trnvars.h"
#include "math/Matrixf.h"
#include "ld/ldtexture.h"
#include "ogl/oglGlobalStates.h"
#include "math/Vec2f.h"
#include "rn/rnvis.h"
#include "physics/colliderMain.h"
#include "physics/physicsmain.h"
#include "ld/ldpix.h"
#include "trn/trnRadClutter.h"
#include "trn/trnclutter.h"
#include "spt/sptmain.h"
#include "ob/obMgr.h"
#include "math/mathnum.h"
#include "math/SuperFastHash.h"
#include "osgImport/ShadowMap.h"
#include "math/mathUnitConversion.h"
#include "ogl/GLCounter.h"

#include "scen/ScenarioManager.h"
#include "ogl/States.h"
#include "sdm/sdmDefines.h"

#include <map>

#include "ui/uiChat.h"

#define MAX_GRASS_WIDTH 64
#define QUE_TIME_LIMIT  5000
#define NUM_PARAMS_PER_VERTEX   5
#define NUMBER_OF_CORNERS       4

#define shadername "spdgrass"
#define shadowshadername "spdgrassshadow"

static bool wantspeedgrass = true;

static float whitescale = 0.0f;
static float redscale = 0.0f;
static float bluescale = 0.25f;
static float greenscale = 0.35f;

#ifdef DEBUG_WW2
static int totalclutter = 0;
static int totalradclutter = 0;
static int totalgrass = 0;
static int totaltriangles = 0;
#endif

#define DEFAULT_NUM_QUADS   2
#define CUSTOM_TERRAIN_NUM_QUADS    1

static int grassTexId = -1;
static int number_of_quads = DEFAULT_NUM_QUADS;

static std::vector<int> miscTexIds;
static std::vector<size_t> KillList;

static float sizescale[16] =
{
    1, 1, 1, 1,
    1, 1, 1, 1,
    1, 1, 1, 1,
    1, 1, 1, 1
};

typedef std::map<std::string, Patch*> GrassPatchMap;
typedef std::map<size_t, CollideQue*> GrassCollideQue;

static GrassCollideQue collideque;
static GrassPatchMap grasspatch;
static std::vector<std::string> clutterregister[SpeedGrassDeprecated::max_clutter_types];
static std::vector<PatchInstance*> drawlist;

static ref_ptr<oglTextureObject> grasstbo;

static bool initialized = false;
static int grass_tbo_size = 0;

static SpeedGrassDeprecated* Grass = NULL;

/*------------------------------------------------------------------
Function: speedgrassToggle
Description:
-------------------------------------------------------------------*/
void speedgrassToggle(bool want)
{
    drawlist.clear();
    wantspeedgrass = want;
}

/*------------------------------------------------------------------
Function: IsValid
Description:
-------------------------------------------------------------------*/
static inline bool IsValid(OBJECT* o)
{
    static std::set<int> texids;
    bool valid = false;

    if(o->flag3 & OBJECT_PLAYER_PLACED)
    {
        return false;
    }

    if(texids.find(o->texid) != texids.end())
    {
        valid = true;
    }
    else
    {
        int clutter[5];
        mlGetRadialClutterStuff(clutter, o->texname);

        if(clutter[0] || clutter[1] || clutter[2] || clutter[3] || clutter[4])
        {
            valid = true;
            texids.insert(o->texid);
        }
    }

    return valid;
}

/*------------------------------------------------------------------
Function: FindGrassParamsScenario
Description:
-------------------------------------------------------------------*/
static void FindGrassParamsScenario(std::vector<float>& params, int type, const Vec3d& pos, float scale = 1.0f, int tex = -1)
{
    int texture = tex;
    float scalefactor;

    unsigned int seed = SuperFastHash((const char*)&pos, sizeof(Vec3d));
    PNRandom::seedrandom(seed);

    static float grassscale[16] =
    {
        1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f
    };

    if(type == SpeedGrassDeprecated::type_grass)
    {
        static bool firsttime = true;
        static std::vector<int> grasschoices;

        if(firsttime)
        {
            firsttime = false;

            grasschoices.push_back(4);
            grasschoices.push_back(5);
            grasschoices.push_back(6);
            grasschoices.push_back(7);
        }

        if(texture == -1)
        {
            texture = PNRandom::random(0, grasschoices.size() - 1);
            texture = grasschoices[texture];
        }

        float height = grassscale[texture] * PNRandom::random(0.9f, 1.0f);

        scalefactor = height * scale;
    }
    else if(type == SpeedGrassDeprecated::type_fltgrass)
    {
        static bool firsttime = true;
        static std::vector<int> grasschoices;

        if(firsttime)
        {
            firsttime = false;

            grasschoices.push_back(4);
            grasschoices.push_back(5);
            grasschoices.push_back(6);
            grasschoices.push_back(7);
        }

        if(texture == -1)
        {
            texture = PNRandom::random(0, grasschoices.size() - 1);
            texture = grasschoices[texture];
        }

        float height = grassscale[texture];

        scalefactor = height * scale;
    }
    else if(type == SpeedGrassDeprecated::type_radclutter)
    {
        static bool firsttime = true;
        static std::vector<int> grasschoices;

        if(firsttime)
        {
            firsttime = false;

            grasschoices.push_back(0);
            grasschoices.push_back(1);
            grasschoices.push_back(2);
            grasschoices.push_back(3);
        }

        if(texture == -1)
        {
            texture = PNRandom::random(0, grasschoices.size() - 1);
            texture = grasschoices[texture];
        }

        float height = PNRandom::random(0.9f, 1.0f) * grassscale[texture];

        scalefactor = height * scale;
    }
    else if(type == SpeedGrassDeprecated::type_pigeon)
    {
        static bool firsttime = true;
        static std::vector<int> grasschoices;

        if(firsttime)
        {
            firsttime = false;

            grasschoices.push_back(8);
            grasschoices.push_back(9);
            grasschoices.push_back(10);
            grasschoices.push_back(11);
        }

        if(texture == -1)
        {
            texture = PNRandom::random(0, grasschoices.size() - 1);
            texture = grasschoices[texture];
        }

        scalefactor = grassscale[texture] * PNRandom::random(0.9f, 1.0f);
    }
    else if(type == SpeedGrassDeprecated::type_userdefined)
    {
        float height = PNRandom::random(0.9f, 1.0f) * grassscale[texture];
        scalefactor = height * scale;
    }

    params.push_back(scalefactor);
    params.push_back(PNRandom::random(0.0f, PI));
    params.push_back(0);
    params.push_back((float)(texture * 4)); // index into UV array. Each texture has 4 uv corners
}

/*------------------------------------------------------------------
Function: FindGrassParams2
Description:
-------------------------------------------------------------------*/
static void FindGrassParams2(std::vector<float>& params, int type, const Vec3d& pos, float scale = 1.0f)
{
    logTimerBeginMacro();

    int texture;
    float scalefactor;

    unsigned int seed = SuperFastHash((const char*)&pos, sizeof(Vec3d));

    if(type == SpeedGrassDeprecated::type_grass)
    {
        static float grassscale[16] =
        {
            0.50f, 1.00f, 1.00f, 1.00f,
            0.35f, 1.00f, 1.00f, 1.00f,
            1.00f, 1.00f, 1.00f, 1.00f,
            0.35f, 1.00f, 1.00f, 1.00f
        };

        static bool firsttime = true;
        static std::vector<int> grasschoices;

        if(firsttime)
        {
            firsttime = false;

            grasschoices.push_back(4);
            grasschoices.push_back(8);
            grasschoices.push_back(9);
            grasschoices.push_back(10);
            grasschoices.push_back(11);
            grasschoices.push_back(12);
        }

        texture = seed % (grasschoices.size() - 1);
        texture = grasschoices[texture];

        float height = 2.8f * grassscale[texture];

        scalefactor = height * scale;
    }
    else if(type == SpeedGrassDeprecated::type_fltgrass)
    {
        static float grassscale[16] =
        {
            0.50f, 1.00f, 1.00f, 1.00f,
            0.35f, 1.00f, 1.00f, 1.00f,
            1.00f, 1.00f, 1.00f, 1.00f,
            0.35f, 1.00f, 1.00f, 1.00f
        };

        static bool firsttime = true;
        static std::vector<int> grasschoices;

        if(firsttime)
        {
            firsttime = false;
            grasschoices.push_back(0);
            grasschoices.push_back(8);
            grasschoices.push_back(9);
            grasschoices.push_back(10);
            grasschoices.push_back(11);
        }

        texture = seed % (grasschoices.size() - 1);
        texture = grasschoices[texture];

        static float height = 1.25f;

        scalefactor = height * scale;
    }
    else if(type == SpeedGrassDeprecated::type_radclutter)
    {
        static bool firsttime = true;
        static std::vector<int> grasschoices;

        if(firsttime)
        {
            firsttime = false;

            grasschoices.push_back(1);
            grasschoices.push_back(2);
            grasschoices.push_back(12);
            grasschoices.push_back(3);
            grasschoices.push_back(4);
            grasschoices.push_back(5);
            grasschoices.push_back(7);
        }

        float s = PNRandom::random(-0.2f, 0.2f);

        // Add random tall clutter
        texture = seed % 128;

        if(texture == 2)
        {
            texture = seed % grasschoices.size() - 1;
        }
        else
        {
            texture = seed % 2;
        }

        texture = grasschoices[texture];

        float height = 0.8f - s;

        scalefactor = height * scale;
    }
    else if(type == SpeedGrassDeprecated::type_pigeon)
    {
        texture = 15;
        scalefactor = 3.0;
    }

    params.push_back(scalefactor);
    params.push_back(0.0f);
    params.push_back((float)frametime);
    params.push_back(texture * 4.0f); // index into UV array. Each texture has 4 uv corners

    logTimerEndMacro();
}

/*------------------------------------------------------------------
Function: GetUV
Description:
-------------------------------------------------------------------*/
static void GetUV(int r, float& u1, float& v1, float& u2, float& v2)
{
    static piUV uv[16][2] =
    {
        // 0
        0.0f, 0.0f,
        0.25f, 0.25f,
        // 1
        0.25f, 0.0f,
        0.5f, 0.25f,
        // 2
        0.5f, 0.0f,
        0.75f, 0.25f,
        // 3
        0.75f, 0.0f,
        1.0f, 0.25f,
        // 4
        0.0f, 0.25f,
        0.25f, 0.5f,
        // 5
        0.25f, 0.25f,
        0.5f, 0.5f,
        // 6
        0.5f, 0.25f,
        0.75f, 0.5f,
        // 7
        0.75f, 0.25f,
        1.0f, 0.5f,
        // 8
        0.0f, 0.5f,
        0.25f, 0.75f,
        // 9
        0.25f, 0.5f,
        0.5f, 0.75f,
        // 10
        0.5f, 0.5f,
        0.75f, 0.75f,
        // 11
        0.75f, 0.5f,
        1.0f, 0.75f,
        // 12
        0.0f, 0.75f,
        0.25f, 1.0f,
        // 13
        0.25f, 0.75f,
        0.5f, 1.0f,
        // 14
        0.5f, 0.75f,
        0.75f, 1.0f,
        // 15
        0.75f, 0.75f,
        1.0f, 1.0f,
    };

    static float offset = 0.0009765625f; // 1 / 1024

    u1 = uv[r][0].u;
    v1 = uv[r][0].v + offset;
    u2 = uv[r][1].u;
    v2 = uv[r][1].v + offset;
}

/*------------------------------------------------------------------
Function: CreatePatchGrass
Description:
-------------------------------------------------------------------*/
static void CreatePatchGrass(const char* key,
                             int cluttertype,
                             float w,
                             float h,
                             float bladeheight,
                             int density)
{
    Grass->CreatePatch(key, cluttertype, w, h, bladeheight, density);
}

/*------------------------------------------------------------------
Function: SpeedGrassDeprecated::CreatePatch()
Description:
-------------------------------------------------------------------*/
void SpeedGrassDeprecated::CreatePatch(const char* key,
                                       int cluttertype,
                                       float w,
                                       float h,
                                       float bladeheight,
                                       int density)
{
    float xx, yy;

    std::set<double> X;
    std::set<double> Y;

    static int seed = 3167;
    PNRandom::seedrandom(seed);

    struct Patch* grass = new Patch;
    struct Patch& patch = *grass;
    patch.type = cluttertype;

    patch.density = density;

    int positionoffset = 0;

    patch.vertexcount = 0;

    float maxdist = 0;

    // Positions
    for(int i = 0; i < density; i++)
    {
        xx = PNRandom::random(-w, w);
        yy = PNRandom::random(-h, h);

        patch.collide.push_back(Vec2f(xx, yy));

        float height = (bladeheight + PNRandom::random(0.0f, 0.2f));
        float radius = fabs(height) * 0.5f;

        // Find max dist from origin for culling radius
        float xxx = xx + radius;
        float yyy = yy + radius;
        float dist = math3dFastSqrt(xxx * xxx + yyy * yyy);

        maxdist = std::max(maxdist, dist);

        CreateGrassQuads(patch, Vec3d(xx, yy, 0.0), 1.0f);
    }

    int zsize = patch.density * number_of_quads * NUMBER_OF_CORNERS;

    patch.vboid = vbo->AddBuffer((void*)&patch.params[0], patch.params.size() * sizeof(float), true, oglVertexBuffer::MODE_SYSTEM_RAM);

    vbo->SetStride(oglVertexBuffer::OVB_POSITION, 4, GL_FLOAT, 20, 0, patch.vboid);
    vbo->SetStride(oglVertexBuffer::OVB_TEXCOORD0, 1, GL_FLOAT, 20, 16, patch.vboid);

    vbo->AddDrawElementsPrimitive(GL_TRIANGLES, patch.indices.size(), &patch.indices[0], patch.vboid);

    // a^2 + b^2 = c^2 (find hypotenuse for culling)
    patch.cullradius = math3dFastSqrt(maxdist * maxdist + maxdist * maxdist);

    size_t code = SuperFastHash(key, strlen(key));
    grass->key = code;

    grasspatch[key] = grass;
    clutterregister[cluttertype].push_back(key);
}

/*------------------------------------------------------------------
Function: speedgrassDraw
Description:
-------------------------------------------------------------------*/
void speedgrassDraw()
{
    if(ShadowMap::GetInstance().Enabled() && ShadowMap::GetInstance().Valid())
    {
        if(Grass)
        {
            if(gCustomTerrain)
                Grass->DrawForCustomShadows();
            else
                Grass->DrawForShadows();
        }
    }
    else
    {
        if(Grass)
        {
            if(gCustomTerrain)
                Grass->DrawCustom();
            else
                Grass->Draw();
        }
    }
}

#if USE_STATESET
/*------------------------------------------------------------------
Function: SpeedGrass::Draw()
Description:
-------------------------------------------------------------------*/
void SpeedGrass::Draw()
{
#ifdef DEBUG_WW2
    totaltriangles = 0;
#endif
    if(!wantspeedgrass)
        return;

    if(drawlist.empty())
        return;

    static std::vector<float> dd;

    oglSetWorldMatrix(&IdentityMatrix);

    Program* program = dynamic_cast<Program*>(grassMaterial->GetRenderState(RS_GLSLPROGRAM));
    glslProgram* pgm = program->GetProgram();

    pgm->GetUniform("origin")->Set(Vec3d(WorldOriginOffset));

    grassMaterial->Apply();

    int offset = 0;
    static std::vector<unsigned char> buffer;

    if(buffer.empty())
    {
        buffer.resize(grass_tbo_size);
    }

    int paramoffset = 0;
    int totalbytes = 0;
    int buffoffset = 0;

    int oldlod = 0;

    for(unsigned int i = 0; i < drawlist.size(); ++i)
    {
        PatchInstance& pi = *drawlist[i];

        int bytesize = (int)pi.params.size() * sizeof(float);

        // If buffer is bigger than TBO size
        if(bytesize > grass_tbo_size)
        {
            paramoffset = 0;

            while(bytesize > grass_tbo_size)
            {
                grasstbo->UpdateBuffer(grass_tbo_size, &pi.params[paramoffset], 0);
                vbo->DrawInstancedPrimitive(grass_tbo_size / 32);
#if DEBUG_WW2
                totaltriangles += (grass_tbo_size / 32) * number_of_quads * 2;
                totalgrass += grass_tbo_size / 32;
#endif
                paramoffset += grass_tbo_size / 32;
                bytesize -= grass_tbo_size;
            }
        }

        if(bytesize > 0)
        {
            if(bytesize + totalbytes < grass_tbo_size)
            {
                memcpy(&buffer[totalbytes], &pi.params[paramoffset], bytesize);
                totalbytes += bytesize;
                paramoffset = 0;
            }
            else
            {
                grasstbo->UpdateBuffer(totalbytes, &buffer[0], 0);
                vbo->DrawInstancedPrimitive(totalbytes / 32);

#if DEBUG_WW2
                totaltriangles += (totalbytes / 32) * number_of_quads * 2;
                totalgrass += totalbytes / 32;
#endif
                totalbytes = 0;
                paramoffset = 0;

                memcpy(&buffer[totalbytes], &pi.params[paramoffset], bytesize);
                totalbytes += bytesize;
            }
        }
    }

    if(totalbytes > 0)
    {
        grasstbo->UpdateBuffer(totalbytes, &buffer[0], 0);
        vbo->DrawInstancedPrimitive(totalbytes / 32);

#if DEBUG_WW2
        totaltriangles += (totalbytes / 32) * number_of_quads * 2;
        totalgrass += totalbytes / 32;
#endif
    }
}
#else
/*------------------------------------------------------------------
Function: SpeedGrass::Draw()
Description:
-------------------------------------------------------------------*/
void SpeedGrass::Draw()
{
#ifdef DEBUG_WW2
    totaltriangles = 0;
#endif
    if(!wantspeedgrass)
        return;

    if(drawlist.empty())
        return;

    ogl::Disable();

    oglSetRenderState(oglRENDERSTATE_CULLFACEENABLE, 0);

    static std::vector<float> dd;

    oglSetWorldMatrix(&IdentityMatrix);

    glslProgram* pgm = grassMaterial->GetProgram();

    pgm->GetUniform("origin")->Set(Vec3d(WorldOriginOffset));

    grassMaterial->Apply();

    int offset = 0;
    static std::vector<unsigned char> buffer;

    if(buffer.empty())
    {
        buffer.resize(grass_tbo_size);
    }

    int paramoffset = 0;
    int totalbytes = 0;
    int buffoffset = 0;

    int oldlod = 0;

    for(unsigned int i = 0; i < drawlist.size(); ++i)
    {
        PatchInstance& pi = *drawlist[i];

        int bytesize = (int)pi.params.size() * sizeof(float);

        // If buffer is bigger than TBO size
        if(bytesize > grass_tbo_size)
        {
            paramoffset = 0;

            while(bytesize > grass_tbo_size)
            {
                grasstbo->UpdateBuffer(grass_tbo_size, &pi.params[paramoffset], 0);
                vbo->DrawInstancedPrimitive(grass_tbo_size / 32);
#if DEBUG_WW2
                totaltriangles += (grass_tbo_size / 32) * number_of_quads * 2;
                totalgrass += grass_tbo_size / 32;
#endif

                paramoffset += grass_tbo_size / 32;
                bytesize -= grass_tbo_size;
            }
        }

        if(bytesize > 0)
        {
            if(bytesize + totalbytes < grass_tbo_size)
            {
                memcpy(&buffer[totalbytes], &pi.params[paramoffset], bytesize);
                totalbytes += bytesize;
                paramoffset = 0;
            }
            else
            {
                grasstbo->UpdateBuffer(totalbytes, &buffer[0], 0);
                vbo->DrawInstancedPrimitive(totalbytes / 32);

#if DEBUG_WW2
                totaltriangles += (totalbytes / 32) * number_of_quads * 2;
                totalgrass += totalbytes / 32;
#endif
                totalbytes = 0;
                paramoffset = 0;

                memcpy(&buffer[totalbytes], &pi.params[paramoffset], bytesize);
                totalbytes += bytesize;
            }
        }
    }

    if(totalbytes > 0)
    {
        grasstbo->UpdateBuffer(totalbytes, &buffer[0], 0);
        vbo->DrawInstancedPrimitive(totalbytes / 32);

#if DEBUG_WW2
        totaltriangles += (totalbytes / 32) * number_of_quads * 2;
        totalgrass += totalbytes / 32;
#endif
    }

    oglSetRenderState(oglRENDERSTATE_CULLFACEENABLE, piCULLFACEENABLE_DEFAULT);
}
#endif

#if USE_STATESET
/*------------------------------------------------------------------
Function: SpeedGrassDeprecated::DrawForShadows()
Description:
-------------------------------------------------------------------*/
void SpeedGrassDeprecated::DrawForShadows()
{
#ifdef DEBUG_WW2
    totaltriangles = 0;
#endif

#define MAX_LISTS_PER_DRAW  3

    if(!wantspeedgrass)
        return;

    if(grasspatch.empty())
        return;

    if(drawlist.empty())
        return;

    oglSetWorldMatrix(&IdentityMatrix);

    glslProgram* pgm = dynamic_cast<Program*>(grassShadowMaterial->GetRenderState(RS_GLSLPROGRAM))->GetProgram();

    if(!pgm)
        return;

    glslUniform* origin = pgm->GetUniform("origin");
    glslUniform* rotate = pgm->GetUniform("rot");

    origin->Set(Vec3d(WorldOriginOffset));

#ifdef DEBUG_WW2
    int count = 0;
#endif

    oglSetWorldMatrix(&IdentityMatrix);
    int createcount = 0;

    glslUniform* alphatest = pgm->GetUniform("alphatest");

    if(gCustomTerrain)
    {
        TextureUnit* tu = dynamic_cast<TextureUnit*>(grassShadowMaterial->GetRenderState(RS_TEXTURE_UNIT1));
        tu->SetTexture(ShadowMap::GetInstance().GetShadowMapTexture());

        TextureMatrix* tm = dynamic_cast<TextureMatrix*>(grassShadowMaterial->GetRenderState(RS_TEXTURE_MATRIX1));
        tm->SetMatrix(ShadowMap::GetInstance().GetInverseMVP());

        grassShadowMaterial->Apply();
    }

    for(unsigned int i = 0; i < drawlist.size(); i++)
    {
        PatchInstance& pi = *drawlist[i];

        if(!pi.collided)
            continue;

        if(!gCustomTerrain)
        {
            ShadowMap::GetInstance().SetShadowStuff(*grassShadowMaterial, pi.center, 1);
            grassShadowMaterial->Apply();
        }

        if(gCustomTerrain)
        {
            static int oldsplatid = -1;

            if(pi.splatid > -1 && oldsplatid != pi.splatid)
            {
                oldsplatid = pi.splatid;

                TextureUnit* tu = dynamic_cast<TextureUnit*>(grassShadowMaterial->GetRenderState(RS_TEXTURE_UNIT2));
                tu->SetTexture(pi.splatid);

                tu->apply();
                States::HaveAppliedAttribute(RS_TEXTURE_UNIT2);
            }
        }

        if(pi.lastdrawtime == 0)
            pi.lastdrawtime = frametime;

        float t = (float)(frametime - pi.lastdrawtime) * 0.001f * pi.faderate;

        if(t > 1.0f)
            t = 1.0f;

        pi.alpha = 1.0f - t * 0.670588235f; // 0.67 = 1 - (84/255)

        alphatest->Set(pi.alpha);
        pgm->Apply(*alphatest);

        if(pi.instanceof)
        {
            origin->Set(Vec3d(pi.center - WorldOriginOffset));
            rotate->Set(pi.rot);

            pgm->Apply(*origin);
            pgm->Apply(*rotate);

            ogl::SetTextureUnitCoordPointer(GL_TEXTURE1, 1, GL_FLOAT, 0, &pi.Z[0]);
            vbo->DrawPrimitive(0, pi.instanceof->vboid);
        }

#ifdef DEBUG_WW2
        totaltriangles += pi.primcount * number_of_quads * 2;
        totalgrass += pi.primcount;
#endif
    }

    oglSetWorldMatrix(&IdentityMatrix);
}

/*------------------------------------------------------------------
Function: SpeedGrassDeprecated::DrawForCustomShadows()
Description:
-------------------------------------------------------------------*/
void SpeedGrassDeprecated::DrawForCustomShadows()
{
#ifdef DEBUG_WW2
    totaltriangles = 0;
#endif

#define STRIDE  24

    if(!wantspeedgrass)
        return;

    if(grasspatch.empty())
        return;

    if(drawlist.empty())
        return;

    oglSetWorldMatrix(&IdentityMatrix);

    glslProgram* pgm = dynamic_cast<Program*>(grassShadowMaterial->GetRenderState(RS_GLSLPROGRAM))->GetProgram();

    if(!pgm)
        return;

    glslUniform* origin = pgm->GetUniform("origin");

    origin->Set(Vec3d(WorldOriginOffset));

#ifdef DEBUG_WW2
    int count = 0;
#endif

    oglSetWorldMatrix(&IdentityMatrix);
    int createcount = 0;

    glslUniform* alphatest = pgm->GetUniform("alphatest");

    TextureUnit* tu = dynamic_cast<TextureUnit*>(grassShadowMaterial->GetRenderState(RS_TEXTURE_UNIT1));
    tu->SetTexture(ShadowMap::GetInstance().GetShadowMapTexture());

    TextureMatrix* tm = dynamic_cast<TextureMatrix*>(grassShadowMaterial->GetRenderState(RS_TEXTURE_MATRIX1));
    tm->SetMatrix(ShadowMap::GetInstance().GetInverseMVP());

    grassShadowMaterial->Apply();

    for(unsigned int i = 0; i < drawlist.size(); i++)
    {
        PatchInstance& pi = *drawlist[i];

        if(!pi.collided)
            continue;

        static int oldsplatid = -1;

        if(pi.splatid > -1 && oldsplatid != pi.splatid)
        {
            oldsplatid = pi.splatid;

            TextureUnit* tu = dynamic_cast<TextureUnit*>(grassShadowMaterial->GetRenderState(RS_TEXTURE_UNIT2));
            tu->SetTexture(pi.splatid);

            tu->apply();
            States::HaveAppliedAttribute(RS_TEXTURE_UNIT2);
        }

        if(pi.lastdrawtime == 0)
            pi.lastdrawtime = frametime;

        float t = (float)(frametime - pi.lastdrawtime) * 0.001f * pi.faderate;

        if(t > 1.0f)
            t = 1.0f;

        pi.alpha = 1.0f - t * 0.670588235f; // 0.67 = 1 - (84/255)

        alphatest->Set(pi.alpha);
        pgm->Apply(*alphatest);

        ogl::SetVertexPointer(4, GL_FLOAT, STRIDE, &pi.params[0]); // x y z type
        ogl::SetTextureUnitCoordPointer(GL_TEXTURE0, 1, GL_FLOAT, STRIDE, &pi.params[4]); // shadow value
        ogl::SetTextureUnitCoordPointer(GL_TEXTURE1, 1, GL_FLOAT, STRIDE, &pi.params[5]); // z value

        ogl::DrawElements(GL_TRIANGLES, pi.indices.size(), GL_UNSIGNED_SHORT, &pi.indices[0]);

#ifdef DEBUG_WW2
        totaltriangles += pi.primcount * number_of_quads * 2;
        totalgrass += pi.primcount;
#endif
    }

    oglSetWorldMatrix(&IdentityMatrix);
}
#else
/*------------------------------------------------------------------
Function: SpeedGrassDeprecated::DrawForCustomShadows()
Description:
-------------------------------------------------------------------*/
void SpeedGrassDeprecated::DrawForCustomShadows()
{
#ifdef DEBUG_WW2
    totaltriangles = 0;
#endif

#define STRIDE  24

    if(!wantspeedgrass)
        return;

    if(grasspatch.empty())
        return;

    if(drawlist.empty())
        return;

    oglSetWorldMatrix(&IdentityMatrix);
    oglSetRenderState(oglRENDERSTATE_CULLFACEENABLE, 0);

    glslProgram* pgm = grassShadowMaterial->GetProgram();

    if(!pgm)
        return;

    pgm->GetUniform("origin")->Set(Vec3d(WorldOriginOffset));

    oglSetRenderState(oglRENDERSTATE_ALPHATESTENABLE, 1);
    oglSetRenderState(oglRENDERSTATE_ALPHABLENDENABLE, 0);
    oglSetRenderState(oglRENDERSTATE_LIGHTING, 0);

#ifdef DEBUG_WW2
    int count = 0;
#endif

    ogl::Disable();

    oglSetWorldMatrix(&IdentityMatrix);
    int createcount = 0;

    static glslUniform* alphatest = NULL;

    if(!alphatest)
        alphatest = pgm->GetUniform("alphatest");

    ShadowMap::GetInstance().SetShadowStuff(*grassShadowMaterial, 1);
    grassShadowMaterial->Apply();

    for(unsigned int i = 0; i < drawlist.size(); i++)
    {
        PatchInstance& pi = *drawlist[i];

        if(!pi.collided)
            continue;

        static int oldsplatid = -1;

        if(pi.splatid > -1 && oldsplatid != pi.splatid)
        {
            oldsplatid = pi.splatid;

            grassShadowMaterial->AddTexture(2, pi.splatid);
            grassShadowMaterial->ApplyTextures();
        }

        if(pi.lastdrawtime == 0)
            pi.lastdrawtime = frametime;

        float t = (float)(frametime - pi.lastdrawtime) * 0.001f * pi.faderate;

        if(t > 1.0f)
            t = 1.0f;

        pi.alpha = 1.0f - t * 0.670588235f; // 0.67 = 1 - (84/255)

        alphatest->Set(pi.alpha);
        pgm->Apply(*alphatest);

        ogl::SetVertexPointer(4, GL_FLOAT, STRIDE, &pi.params[0]); // x y z type
        ogl::SetTextureUnitCoordPointer(GL_TEXTURE0, 1, GL_FLOAT, STRIDE, &pi.params[4]); // shadow value
        ogl::SetTextureUnitCoordPointer(GL_TEXTURE1, 1, GL_FLOAT, STRIDE, &pi.params[5]); // z value

        ogl::DrawElements(GL_TRIANGLES, pi.indices.size(), GL_UNSIGNED_SHORT, &pi.indices[0]);

#ifdef DEBUG_WW2
        totaltriangles += pi.primcount * number_of_quads * 2;
        totalgrass += pi.primcount;
#endif
    }

    oglSetWorldMatrix(&IdentityMatrix);

    oglSetRenderState(oglRENDERSTATE_CULLFACEENABLE, piCULLFACEENABLE_DEFAULT);

    oglSetRenderState(oglRENDERSTATE_ALPHATESTENABLE, piALPHATESTENABLE_DEFAULT);
    oglSetRenderState(oglRENDERSTATE_ALPHABLENDENABLE, piALPHABLENDENABLE_DEFAULT);
    oglSetRenderState(oglRENDERSTATE_LIGHTING, piLIGHTING_DEFAULT);

    ShadowMap::GetInstance().UnApplyShadowMaps(1);
}

/*------------------------------------------------------------------
Function: SpeedGrassDeprecated::DrawForShadows()
Description:
-------------------------------------------------------------------*/
void SpeedGrassDeprecated::DrawForShadows()
{
#ifdef DEBUG_WW2
    totaltriangles = 0;
#endif

#define MAX_LISTS_PER_DRAW  3

    if(!wantspeedgrass)
        return;

    if(grasspatch.empty())
        return;

    if(drawlist.empty())
        return;

    oglSetWorldMatrix(&IdentityMatrix);
    oglSetRenderState(oglRENDERSTATE_CULLFACEENABLE, 0);

    glslProgram* pgm = grassShadowMaterial->GetProgram();

    if(!pgm)
        return;

    glslUniform* origin = pgm->GetUniform("origin");
    glslUniform* rotate = pgm->GetUniform("rot");

    oglSetRenderState(oglRENDERSTATE_ALPHATESTENABLE, 1);
    oglSetRenderState(oglRENDERSTATE_ALPHABLENDENABLE, 0);
    oglSetRenderState(oglRENDERSTATE_LIGHTING, 0);

#ifdef DEBUG_WW2
    int count = 0;
#endif

    ogl::Disable();

    oglSetWorldMatrix(&IdentityMatrix);
    int createcount = 0;

    static glslUniform* alphatest = NULL;

    if(!alphatest)
        alphatest = pgm->GetUniform("alphatest");

    for(unsigned int i = 0; i < drawlist.size(); i++)
    {
        PatchInstance& pi = *drawlist[i];

        if(!pi.collided)
            continue;

        ShadowMap::GetInstance().SetShadowStuff(*grassShadowMaterial, pi.center, 1);
        grassShadowMaterial->Apply();

        if(pi.lastdrawtime == 0)
            pi.lastdrawtime = frametime;

        float t = (float)(frametime - pi.lastdrawtime) * 0.001f * pi.faderate;

        if(t > 1.0f)
            t = 1.0f;

        pi.alpha = 1.0f - t * 0.670588235f; // 0.67 = 1 - (84/255)

        alphatest->Set(pi.alpha);
        pgm->Apply(*alphatest);

        if(pi.instanceof)
        {
            origin->Set(Vec3d(pi.center - WorldOriginOffset));
            rotate->Set(pi.rot);

            pgm->Apply(*origin);
            pgm->Apply(*rotate);

            ogl::SetTextureUnitCoordPointer(GL_TEXTURE1, 1, GL_FLOAT, 0, &pi.Z[0]);
            vbo->DrawPrimitive(0, pi.instanceof->vboid);
        }

#ifdef DEBUG_WW2
        totaltriangles += pi.primcount * number_of_quads * 2;
        totalgrass += pi.primcount;
#endif
    }

    oglSetWorldMatrix(&IdentityMatrix);

    oglSetRenderState(oglRENDERSTATE_CULLFACEENABLE, piCULLFACEENABLE_DEFAULT);

    oglSetRenderState(oglRENDERSTATE_ALPHATESTENABLE, piALPHATESTENABLE_DEFAULT);
    oglSetRenderState(oglRENDERSTATE_ALPHABLENDENABLE, piALPHABLENDENABLE_DEFAULT);
    oglSetRenderState(oglRENDERSTATE_LIGHTING, piLIGHTING_DEFAULT);

    ShadowMap::GetInstance().UnApplyShadowMaps(1);
    ProgramState::Bind(0);
}
#endif

#if USE_STATESET
/*------------------------------------------------------------------
Function: SpeedGrassDeprecated::Draw()
Description:
-------------------------------------------------------------------*/
void SpeedGrassDeprecated::Draw()
{
#ifdef DEBUG_WW2
    totaltriangles = 0;
#endif

#define MAX_LISTS_PER_DRAW  3

    if(!wantspeedgrass)
        return;

    if(grasspatch.empty())
        return;

    if(drawlist.empty())
        return;

    oglSetWorldMatrix(&IdentityMatrix);

    glslProgram* pgm = dynamic_cast<Program*>(grassMaterial->GetRenderState(RS_GLSLPROGRAM))->GetProgram();

    if(!pgm)
        return;

    glslUniform* origin = pgm->GetUniform("origin");
    glslUniform* rotate = pgm->GetUniform("rot");

    origin->Set(Vec3d(WorldOriginOffset));

    grassMaterial->Apply();

#ifdef DEBUG_WW2
    int count = 0;
#endif

    oglSetWorldMatrix(&IdentityMatrix);
    int createcount = 0;

    static glslUniform* alphatest = NULL;

    if(!alphatest)
        alphatest = pgm->GetUniform("alphatest");

    for(unsigned int i = 0; i < drawlist.size(); i++)
    {
        PatchInstance& pi = *drawlist[i];

        if(!pi.collided)
            continue;

        if(pi.lastdrawtime == 0)
            pi.lastdrawtime = frametime;

        float t = (float)(frametime - pi.lastdrawtime) * 0.001f * pi.faderate;

        if(t > 1.0f)
            t = 1.0f;

        pi.alpha = 1.0f - t * 0.670588235f; // 0.67 = 1 - (84/255)

        alphatest->Set(pi.alpha);
        pgm->Apply(*alphatest);

        if(pi.instanceof)
        {
            origin->Set(Vec3d(pi.center - WorldOriginOffset));
            rotate->Set(pi.rot);

            pgm->Apply(*origin);
            pgm->Apply(*rotate);

            ogl::SetTextureUnitCoordPointer(GL_TEXTURE1, 1, GL_FLOAT, 0, &pi.Z[0]);
            vbo->DrawPrimitive(0, pi.instanceof->vboid);
        }

#ifdef DEBUG_WW2
        totaltriangles += pi.primcount * number_of_quads * 2;
        totalgrass += pi.primcount;
#endif
    }

    oglSetWorldMatrix(&IdentityMatrix);
}

/*------------------------------------------------------------------
Function: SpeedGrassDeprecated::DrawCustom()
Description:
-------------------------------------------------------------------*/
void SpeedGrassDeprecated::DrawCustom()
{
#ifdef DEBUG_WW2
    totaltriangles = 0;
#endif

#define STRIDE 24

    if(!wantspeedgrass)
        return;

    if(grasspatch.empty())
        return;

    if(drawlist.empty())
        return;

    oglSetWorldMatrix(&IdentityMatrix);

    glslProgram* pgm = dynamic_cast<Program*>(grassMaterial->GetRenderState(RS_GLSLPROGRAM))->GetProgram();

    if(!pgm)
        return;

#ifdef DEBUG_WW2
    int count = 0;
#endif

    oglSetWorldMatrix(&IdentityMatrix);
    int createcount = 0;

    static glslUniform* alphatest = NULL;
    static glslUniform* origin = NULL;

    if(!origin)
        origin = pgm->GetUniform("origin");

    if(!alphatest)
        alphatest = pgm->GetUniform("alphatest");

    origin->Set(Vec3d(WorldOriginOffset));

    grassMaterial->Apply();

    for(unsigned int i = 0; i < drawlist.size(); i++)
    {
        PatchInstance& pi = *drawlist[i];

        if(!pi.collided)
            continue;

        if(pi.lastdrawtime == 0)
            pi.lastdrawtime = frametime;

        static int oldsplatid = -1;

        if(pi.splatid > -1 && oldsplatid != pi.splatid)
        {
            oldsplatid = pi.splatid;

            TextureUnit* tu = dynamic_cast<TextureUnit*>(grassMaterial->GetRenderState(RS_TEXTURE_UNIT1));
            tu->SetTexture(pi.splatid);

            tu->apply();
            States::HaveAppliedAttribute(RS_TEXTURE_UNIT1);
        }

        float t = (float)(frametime - pi.lastdrawtime) * 0.001f * pi.faderate;

        if(t > 1.0f)
            t = 1.0f;

        pi.alpha = 1.0f - t * 0.670588235f; // 0.67 = 1 - (84/255)

        alphatest->Set(pi.alpha);
        pgm->Apply(*alphatest);

        ogl::SetVertexPointer(4, GL_FLOAT, STRIDE, &pi.params[0]);
        ogl::SetTextureUnitCoordPointer(GL_TEXTURE0, 1, GL_FLOAT, STRIDE, &pi.params[4]);
        ogl::DrawElements(GL_TRIANGLES, pi.indices.size(), GL_UNSIGNED_SHORT, &pi.indices[0]);

#ifdef DEBUG_WW2
        totaltriangles += pi.primcount * number_of_quads * 2;
        totalgrass += pi.primcount;
#endif
    }

    oglSetWorldMatrix(&IdentityMatrix);
}
#else
/*------------------------------------------------------------------
Function: SpeedGrassDeprecated::DrawCustom()
Description:
-------------------------------------------------------------------*/
void SpeedGrassDeprecated::DrawCustom()
{
#ifdef DEBUG_WW2
    totaltriangles = 0;
#endif

#define STRIDE 24

    if(!wantspeedgrass)
        return;

    if(grasspatch.empty())
        return;

    if(drawlist.empty())
        return;

    oglSetWorldMatrix(&IdentityMatrix);
    oglSetRenderState(oglRENDERSTATE_CULLFACEENABLE, 0);

    glslProgram* pgm = grassMaterial->GetProgram();

    if(!pgm)
        return;

    pgm->GetUniform("origin")->Set(Vec3d(WorldOriginOffset));

    grassMaterial->Apply();

    oglSetRenderState(oglRENDERSTATE_ALPHATESTENABLE, 1);
    oglSetRenderState(oglRENDERSTATE_ALPHABLENDENABLE, 0);
    oglSetRenderState(oglRENDERSTATE_LIGHTING, 0);

#ifdef DEBUG_WW2
    int count = 0;
#endif

    ogl::Disable();

    oglSetWorldMatrix(&IdentityMatrix);
    int createcount = 0;

    static glslUniform* alphatest = NULL;

    if(!alphatest)
        alphatest = pgm->GetUniform("alphatest");

    for(unsigned int i = 0; i < drawlist.size(); i++)
    {
        PatchInstance& pi = *drawlist[i];

        if(!pi.collided)
            continue;

        if(pi.lastdrawtime == 0)
            pi.lastdrawtime = frametime;

        static int oldsplatid = -1;

        if(pi.splatid > -1 && oldsplatid != pi.splatid)
        {
            oldsplatid = pi.splatid;

            grassMaterial->AddTexture(1, pi.splatid);
            grassMaterial->ApplyTextures();
        }

        float t = (float)(frametime - pi.lastdrawtime) * 0.001f * pi.faderate;

        if(t > 1.0f)
            t = 1.0f;

        pi.alpha = 1.0f - t * 0.670588235f; // 0.67 = 1 - (84/255)

        alphatest->Set(pi.alpha);
        pgm->Apply(*alphatest);

        ogl::SetVertexPointer(4, GL_FLOAT, STRIDE, &pi.params[0]);
        ogl::SetTextureUnitCoordPointer(GL_TEXTURE0, 1, GL_FLOAT, STRIDE, &pi.params[4]);
        ogl::DrawElements(GL_TRIANGLES, pi.indices.size(), GL_UNSIGNED_SHORT, &pi.indices[0]);

#ifdef DEBUG_WW2
        totaltriangles += pi.primcount * number_of_quads * 2;
        totalgrass += pi.primcount;
#endif
    }

    oglSetWorldMatrix(&IdentityMatrix);

    oglSetRenderState(oglRENDERSTATE_CULLFACEENABLE, piCULLFACEENABLE_DEFAULT);

    oglSetRenderState(oglRENDERSTATE_ALPHATESTENABLE, piALPHATESTENABLE_DEFAULT);
    oglSetRenderState(oglRENDERSTATE_ALPHABLENDENABLE, piALPHABLENDENABLE_DEFAULT);
    oglSetRenderState(oglRENDERSTATE_LIGHTING, piLIGHTING_DEFAULT);
}

/*------------------------------------------------------------------
Function: SpeedGrassDeprecated::Draw()
Description:
-------------------------------------------------------------------*/
void SpeedGrassDeprecated::Draw()
{
#ifdef DEBUG_WW2
    totaltriangles = 0;
#endif

#define MAX_LISTS_PER_DRAW  3

    if(!wantspeedgrass)
        return;

    if(grasspatch.empty())
        return;

    if(drawlist.empty())
        return;

    oglSetWorldMatrix(&IdentityMatrix);
    oglSetRenderState(oglRENDERSTATE_CULLFACEENABLE, 0);

    glslProgram* pgm = grassMaterial->GetProgram();

    if(!pgm)
        return;

    glslUniform* origin = pgm->GetUniform("origin");
    glslUniform* rotate = pgm->GetUniform("rot");

    grassMaterial->Apply();

    oglSetRenderState(oglRENDERSTATE_ALPHATESTENABLE, 1);
    oglSetRenderState(oglRENDERSTATE_ALPHABLENDENABLE, 0);
    oglSetRenderState(oglRENDERSTATE_LIGHTING, 0);

#ifdef DEBUG_WW2
    int count = 0;
#endif

    ogl::Disable();

    oglSetWorldMatrix(&IdentityMatrix);
    int createcount = 0;

    static glslUniform* alphatest = NULL;

    if(!alphatest)
        alphatest = pgm->GetUniform("alphatest");

    for(unsigned int i = 0; i < drawlist.size(); i++)
    {
        PatchInstance& pi = *drawlist[i];

        if(!pi.collided)
            continue;

        if(pi.lastdrawtime == 0)
            pi.lastdrawtime = frametime;

        float t = (float)(frametime - pi.lastdrawtime) * 0.001f * pi.faderate;

        if(t > 1.0f)
            t = 1.0f;

        pi.alpha = 1.0f - t * 0.670588235f; // 0.67 = 1 - (84/255)

        alphatest->Set(pi.alpha);
        pgm->Apply(*alphatest);

        if(pi.instanceof)
        {
            origin->Set(Vec3d(pi.center - WorldOriginOffset));
            rotate->Set(pi.rot);

            pgm->Apply(*origin);
            pgm->Apply(*rotate);

            ogl::SetTextureUnitCoordPointer(GL_TEXTURE1, 1, GL_FLOAT, 0, &pi.Z[0]);
            vbo->DrawPrimitive(0, pi.instanceof->vboid);
        }

#ifdef DEBUG_WW2
        totaltriangles += pi.primcount * number_of_quads * 2;
        totalgrass += pi.primcount;
#endif
    }

    oglSetWorldMatrix(&IdentityMatrix);

    oglSetRenderState(oglRENDERSTATE_CULLFACEENABLE, piCULLFACEENABLE_DEFAULT);

    oglSetRenderState(oglRENDERSTATE_ALPHATESTENABLE, piALPHATESTENABLE_DEFAULT);
    oglSetRenderState(oglRENDERSTATE_ALPHABLENDENABLE, piALPHABLENDENABLE_DEFAULT);
    oglSetRenderState(oglRENDERSTATE_LIGHTING, piLIGHTING_DEFAULT);

    ProgramState::Bind(0);
}
#endif

/*------------------------------------------------------------------
Function: speedgrassRemove
Description:
-------------------------------------------------------------------*/
bool speedgrassRemove(const char* key)
{
    if(Grass)
    {
        size_t code = SuperFastHash(key, strlen(key));

        Grass->EraseInstance(code);

        // Still on que?
        GrassCollideQue::iterator itr2 = collideque.find(code);

        if(itr2 != collideque.end())
        {
            delete itr2->second;
            collideque.erase(itr2);
        }
    }

    return true;
}

/*------------------------------------------------------------------
Function: speedgrassRemove
Description:
-------------------------------------------------------------------*/
bool speedgrassRemove(size_t key)
{
    if(Grass)
    {
        Grass->EraseInstance(key);
    }

    return true;
}

/*------------------------------------------------------------------
Function: CreateColliders
Description:
-------------------------------------------------------------------*/
void CreateColliders()
{
    GrassPatchMap::iterator gp = grasspatch.begin();

    for(; gp != grasspatch.end(); ++gp)
    {
        // Original patch in memory. Create instance from this
        Patch& patch = *gp->second;

         if(patch.density == 0)
            continue;

        patch.collider = new colliderQuery(0, false);
        patch.collider->addGeomToCollideWith((dGeomID)colliderTerrainSpace());
        patch.collider->setMaxContactsPerHit(1);

        int offset = number_of_quads * NUMBER_OF_CORNERS;

        if(!ogl::UseDeprecatedPath())
        {
            offset = number_of_quads * NUMBER_OF_CORNERS;
        }

		UINT32 col = COLLIDER_TILE|COLLIDER_BOX|COLLIDER_TRN_GROUND;

        for(int i = 0; i < patch.density; i++)
        {
            dGeomID ray = dCreateRay(patch.collider->space(), 10000.0f);
            dGeomRaySet(ray, 0, 0, 0, 0, 0, 1); // Updated when we collide
            patch.collider->addGeom(ray, 0, col);

            // Offset into data array for the grass coords
            patch.geommap[ray] = i * offset;
        }
    }
}

/*------------------------------------------------------------------
Function: LoadGrassBMP
Description:
-------------------------------------------------------------------*/
static void LoadGrassBMP(const char* filename,
                         const char* type,
                         int cluttertype,
                         float h,
                         float patchwidth,
                         float scalemin,
                         float scalemax,
                         const std::vector<int>& texchoices)
{
    Grass->LoadBMP(filename, type, cluttertype, h, patchwidth, scalemin, scalemax, texchoices);
}

/*------------------------------------------------------------------
Function: SpeedGrassDeprecated::LoadBMP()
Description:
-------------------------------------------------------------------*/
void SpeedGrassDeprecated::LoadBMP(const char* filename,
                                   const char* type,
                                   int cluttertype,
                                   float h,
                                   float patchwidth,
                                   float scalemin,
                                   float scalemax,
                                   const std::vector<int>& texchoices)
{
    int id = ldpixLoadImageFileToSurface(filename, colorMAGENTA, LDPIX_NONE);

	if(!piIsTexture(id, 0))
	{
		Write_Debug("ERROR: %s could not be found", filename);
		return;
	}

    miscTexIds.push_back(id);

    piSURFACEDESC pisd;
    piLockTextureMemorySurface(id, 0, &pisd);

    char* cptr;
    int bitsPitch = pisd.Width * 3;

    int density = 0;

    struct Patch* grass = new Patch;
    struct Patch& patch = *grass;
    patch.type = cluttertype;

    float scale = patchwidth / (float)pisd.Width;

    scale = scale > 1.0f ? 1.0f : scale;

    char key[32];
    strcpy(key, type);

    int halfw = pisd.Width / 2;
    int halfh = pisd.Height / 2;

    int positionoffset = 0;

    patch.w = pisd.Width;
    patch.h = pisd.Height;

    if(pisd.PixelFormat.dwRGBBitCount > 24)
        return;

    float s = 1.0f;

    int RedMask     = pisd.PixelFormat.dwRBitMask;
    int GreenMask   = pisd.PixelFormat.dwGBitMask;
    int BlueMask    = pisd.PixelFormat.dwBBitMask;
    int AlphaMask   = pisd.PixelFormat.dwRGBAlphaBitMask;

    int RedBits     = 8 - piFindBitCount(RedMask);
    int RedShift    = piFindBitShift(RedMask);
    int GreenBits   = 8 - piFindBitCount(GreenMask);
    int GreenShift  = piFindBitShift(GreenMask);
    int BlueBits    = 8 - piFindBitCount(BlueMask);
    int BlueShift   = piFindBitShift(BlueMask);
    int AlphaBits   = piFindBitCount(AlphaMask);
    int AlphaShift  = piFindBitShift(AlphaMask);

    unsigned int color = 0;
    unsigned int texsize = texchoices.size() - 1;

    std::vector<int> tex;

    PNRandom::seedrandom(3229);

    density = 0;
    patch.vertexcount = 0;
    tex.clear();

    float maxdist = 0;

    for(int row = 0; row < (int)pisd.Height; row++)
    {
        cptr = ((char*)pisd.Surface + row * pisd.Pitch);

        for(int col = 0; col < (int)pisd.Width; col++, cptr += 3)
        {
            int red = (int)(cptr[0] & 0xff);
            int grn = (int)(cptr[1] & 0xff);
            int blu = (int)(cptr[2] & 0xff);

            color = ((red >> RedBits) << RedShift) |
                    ((grn >> GreenBits) << GreenShift) |
                    ((blu >> BlueBits) << BlueShift);

            if(color == colorBLACK)
                continue;

            float xx = float(halfw - col);
            float yy = float(row - halfh);

            xx *= scale;
            yy *= scale;

            // Add a little offset to avoid grid like pattern
            float offsetx = PNRandom::random(-0.5f, 0.5f);
            float offsety = PNRandom::random(-0.5f, 0.5f);

            xx += offsetx;
            yy += offsety;

            // Sink into ground based on color
            switch(color)
            {
            case colorWHITE:
                s = whitescale;
                break;
            case colorRED:
                s = redscale;
                break;
            case colorBLUE:
                s = bluescale;
                break;
            case colorGREEN:
                s = greenscale;
                break;
            default:
                s = 0.0f;
            }

            s += PNRandom::random(scalemin, scalemax);

            patch.collide.push_back(Vec2f(xx, yy));

            int texture = texchoices[PNRandom::random(0, texsize)];

            float height = (h - s) * sizescale[texture];
            float radius = fabs(height) * 0.5f;

            // Find max dist from origin for culling radius
            float xxx = xx + radius;
            float yyy = yy + radius;
            float dist = math3dFastSqrt(xxx * xxx + yyy * yyy);

            maxdist = std::max(maxdist, dist);

            density++;

            CreateGrassQuads(patch, Vec3d(xx, yy, 0.0), 1.0f);
        }
    }

    if(density == 0)
    {
        patch.density = 0;
        return;
    }

    patch.density = density;

    int zsize = patch.density * number_of_quads * NUMBER_OF_CORNERS;

    patch.vboid = vbo->AddBuffer((void*)&patch.params[0], patch.params.size() * sizeof(float), true, oglVertexBuffer::MODE_SYSTEM_RAM);

    vbo->SetStride(oglVertexBuffer::OVB_POSITION, 4, GL_FLOAT, 20, 0, patch.vboid);
    vbo->SetStride(oglVertexBuffer::OVB_TEXCOORD0, 1, GL_FLOAT, 20, 16, patch.vboid);

    vbo->AddDrawElementsPrimitive(GL_TRIANGLES, patch.indices.size(), &patch.indices[0], patch.vboid);

    // a^2 + b^2 = c^2 (find hypotenuse and double it for culling)
    patch.cullradius = math3dFastSqrt(maxdist * maxdist + maxdist * maxdist);

    size_t code = SuperFastHash(key, strlen(key));
    grass->key = code;

    grasspatch[key] = grass;
    clutterregister[cluttertype].push_back(type);

    piUnlockTextureMemorySurface(id, 0);

    piFreeMipTextures(id);
}

/*------------------------------------------------------------------
Function: speedgrassRemoveAll
Description:
-------------------------------------------------------------------*/
void speedgrassRemoveAll()
{
    if(Grass)
        Grass->Kill();
}

/*------------------------------------------------------------------
Function: speedgrassKill
Description:
-------------------------------------------------------------------*/
void speedgrassKill()
{
    if(!Grass)
        return;

    initialized = false;

    oglFreeMipTextures(grassTexId);

    for(unsigned int i = 0; i < miscTexIds.size(); i++)
    {
        oglFreeMipTextures(miscTexIds[i]);
    }

    Grass->Kill();

    GrassPatchMap::iterator itr2 = grasspatch.begin();
    GrassPatchMap::iterator free2;

    while(itr2 != grasspatch.end())
    {
        free2 = itr2;
        ++itr2;

        delete free2->second;
        grasspatch.erase(free2);
    }

    GrassCollideQue::iterator itr3 = collideque.begin();
    GrassCollideQue::iterator free3;

    if(itr3 != collideque.end())
    {
        free3 = itr3;
        ++itr3;

        delete free3->second;
        collideque.erase(free3);
    }

    delete Grass;
    Grass = NULL;
}

/*------------------------------------------------------------------
Function: speedgrassInit
Description:
-------------------------------------------------------------------*/
void speedgrassInit()
{
    if(ogl::UseDeprecatedPath())
    {
        Grass = new SpeedGrassDeprecated;
    }
    else
    {
        Grass = new SpeedGrass;
    }

    Grass->InitVBO();
    Grass->Init();
    CreateColliders();
}

/*------------------------------------------------------------------
Function: SpeedGrassDeprecated::CreateGrassQuads
Description:
-------------------------------------------------------------------*/
void SpeedGrassDeprecated::CreateGrassQuads(Patch& instance, const Vec3d& pos, float scale, int texture, float shadow)
{
    logTimerBeginMacro();

    static std::vector<float> data;

    if(gCustomTerrain)
        FindGrassParamsScenario(data, instance.type, pos, scale, texture);
    else
        FindGrassParams2(data, instance.type, pos, scale);

    static Vec3f v1, v2, v3, v4;
    int type = (int)data[3];

    float height = -1.0f * data[0];
    float radius = 0.5f * data[0];

    v1.set(-radius, 0.0f, height);
    v2.set(-radius, 0.0f, 0.0f);
    v3.set(radius, 0.0f, 0.0f);
    v4.set(radius, 0.0f, height);

    int deg = PNRandom::random(0, 360);
    float rad = DEG_TO_RAD(deg);

    Vec2f rot;
    rot.x() = cosf(rad);
    rot.y() = sinf(rad);

    v1 = ROTATE_VECTOR_Z(rot, v1);
    v2 = ROTATE_VECTOR_Z(rot, v2);
    v3 = ROTATE_VECTOR_Z(rot, v3);
    v4 = ROTATE_VECTOR_Z(rot, v4);

    // Corner 1
    instance.params.push_back(v1.x() + (float)pos.x());
    instance.params.push_back(v1.y() + (float)pos.y());
    instance.params.push_back(v1.z() + (float)pos.z());
    instance.params.push_back((float)(0 + type));
    instance.params.push_back(shadow);

    // Corner 2
    instance.params.push_back(v2.x() + (float)pos.x());
    instance.params.push_back(v2.y() + (float)pos.y());
    instance.params.push_back(v2.z() + (float)pos.z());
    instance.params.push_back((float)(1 + type));
    instance.params.push_back(shadow);

    // Corner 3
    instance.params.push_back(v3.x() + (float)pos.x());
    instance.params.push_back(v3.y() + (float)pos.y());
    instance.params.push_back(v3.z() + (float)pos.z());
    instance.params.push_back((float)(2 + type));
    instance.params.push_back(shadow);

    // Corner 4
    instance.params.push_back(v4.x() + (float)pos.x());
    instance.params.push_back(v4.y() + (float)pos.y());
    instance.params.push_back(v4.z() + (float)pos.z());
    instance.params.push_back((float)(3 + type));
    instance.params.push_back(shadow);

    if(number_of_quads > 1)
    {
        float deg = PI / 2.0f;

        rot.x() = cosf(deg);
        rot.y() = sinf(deg);

        v1 = ROTATE_VECTOR_Z(rot, v1);
        v2 = ROTATE_VECTOR_Z(rot, v2);
        v3 = ROTATE_VECTOR_Z(rot, v3);
        v4 = ROTATE_VECTOR_Z(rot, v4);

        // Corner 1
        instance.params.push_back(v1.x() + (float)pos.x());
        instance.params.push_back(v1.y() + (float)pos.y());
        instance.params.push_back(v1.z() + (float)pos.z());
        instance.params.push_back((float)(0 + type));
        instance.params.push_back(shadow);

        // Corner 2
        instance.params.push_back(v2.x() + (float)pos.x());
        instance.params.push_back(v2.y() + (float)pos.y());
        instance.params.push_back(v2.z() + (float)pos.z());
        instance.params.push_back((float)(1 + type));
        instance.params.push_back(shadow);

        // Corner 3
        instance.params.push_back(v3.x() + (float)pos.x());
        instance.params.push_back(v3.y() + (float)pos.y());
        instance.params.push_back(v3.z() + (float)pos.z());
        instance.params.push_back((float)(2 + type));
        instance.params.push_back(shadow);

        // Corner 4
        instance.params.push_back(v4.x() + (float)pos.x());
        instance.params.push_back(v4.y() + (float)pos.y());
        instance.params.push_back(v4.z() + (float)pos.z());
        instance.params.push_back((float)(3 + type));
        instance.params.push_back(shadow);
    }

    if(number_of_quads > 2)
    {
        float deg = PI / 4.0f;

        rot.x() = cosf(deg);
        rot.y() = sinf(deg);

        v1 = ROTATE_VECTOR_Z(rot, v1);
        v2 = ROTATE_VECTOR_Z(rot, v2);
        v3 = ROTATE_VECTOR_Z(rot, v3);
        v4 = ROTATE_VECTOR_Z(rot, v4);

        // Corner 1
        instance.params.push_back(v1.x() + (float)pos.x());
        instance.params.push_back(v1.y() + (float)pos.y());
        instance.params.push_back(v1.z() + (float)pos.z());
        instance.params.push_back((float)(0 + type));
        instance.params.push_back(shadow);

        // Corner 2
        instance.params.push_back(v2.x() + (float)pos.x());
        instance.params.push_back(v2.y() + (float)pos.y());
        instance.params.push_back(v2.z() + (float)pos.z());
        instance.params.push_back((float)(1 + type));
        instance.params.push_back(shadow);

        // Corner 3
        instance.params.push_back(v3.x() + (float)pos.x());
        instance.params.push_back(v3.y() + (float)pos.y());
        instance.params.push_back(v3.z() + (float)pos.z());
        instance.params.push_back((float)(2 + type));
        instance.params.push_back(shadow);

        // Corner 4
        instance.params.push_back(v4.x() + (float)pos.x());
        instance.params.push_back(v4.y() + (float)pos.y());
        instance.params.push_back(v4.z() + (float)pos.z());
        instance.params.push_back((float)(3 + type));
        instance.params.push_back(shadow);
    }

    instance.indices.push_back(instance.indiceoffset + 0);
    instance.indices.push_back(instance.indiceoffset + 1);
    instance.indices.push_back(instance.indiceoffset + 2);
    instance.indices.push_back(instance.indiceoffset + 0);
    instance.indices.push_back(instance.indiceoffset + 2);
    instance.indices.push_back(instance.indiceoffset + 3);

    instance.indiceoffset += 4;

    if(number_of_quads > 1)
    {
        instance.indices.push_back(instance.indiceoffset + 0);
        instance.indices.push_back(instance.indiceoffset + 1);
        instance.indices.push_back(instance.indiceoffset + 2);
        instance.indices.push_back(instance.indiceoffset + 0);
        instance.indices.push_back(instance.indiceoffset + 2);
        instance.indices.push_back(instance.indiceoffset + 3);

        instance.indiceoffset += 4;
    }

    if(number_of_quads > 2)
    {
        instance.indices.push_back(instance.indiceoffset + 0);
        instance.indices.push_back(instance.indiceoffset + 1);
        instance.indices.push_back(instance.indiceoffset + 2);
        instance.indices.push_back(instance.indiceoffset + 0);
        instance.indices.push_back(instance.indiceoffset + 2);
        instance.indices.push_back(instance.indiceoffset + 3);

        instance.indiceoffset += 4;
    }

    data.clear();

    logTimerEndMacro();
}

/*------------------------------------------------------------------
Function: SpeedGrassDeprecated::CreateGrassQuads
Description:
-------------------------------------------------------------------*/
void SpeedGrassDeprecated::CreateGrassQuads(PatchInstance& instance, const Vec3d& pos, float scale, int texture, float shadow)
{
    logTimerBeginMacro();

    static std::vector<float> data;

    if(gCustomTerrain)
        FindGrassParamsScenario(data, instance.type, pos, scale, texture);
    else
        FindGrassParams2(data, instance.type, pos, scale);

    static Vec3f v1, v2, v3, v4;
    int type = (int)data[3];

    float height = -1.0f * data[0];
    float radius = 0.5f * data[0];

    v1.set(-radius, 0.0f, height);
    v2.set(-radius, 0.0f, 0.0f);
    v3.set(radius, 0.0f, 0.0f);
    v4.set(radius, 0.0f, height);

    int deg = PNRandom::random(0, 360);
    float rad = DEG_TO_RAD(deg);

    Vec2f rot;
    rot.x() = cosf(rad);
    rot.y() = sinf(rad);

    v1 = ROTATE_VECTOR_Z(rot, v1);
    v2 = ROTATE_VECTOR_Z(rot, v2);
    v3 = ROTATE_VECTOR_Z(rot, v3);
    v4 = ROTATE_VECTOR_Z(rot, v4);

    // Corner 1
    instance.params.push_back(v1.x() + (float)pos.x());
    instance.params.push_back(v1.y() + (float)pos.y());
    instance.params.push_back(v1.z() + (float)pos.z());
    instance.params.push_back((float)(0 + type));
    instance.params.push_back(shadow);
    instance.params.push_back(0);

    // Corner 2
    instance.params.push_back(v2.x() + (float)pos.x());
    instance.params.push_back(v2.y() + (float)pos.y());
    instance.params.push_back(v2.z() + (float)pos.z());
    instance.params.push_back((float)(1 + type));
    instance.params.push_back(shadow);
    instance.params.push_back(0);

    // Corner 3
    instance.params.push_back(v3.x() + (float)pos.x());
    instance.params.push_back(v3.y() + (float)pos.y());
    instance.params.push_back(v3.z() + (float)pos.z());
    instance.params.push_back((float)(2 + type));
    instance.params.push_back(shadow);
    instance.params.push_back(0);

    // Corner 4
    instance.params.push_back(v4.x() + (float)pos.x());
    instance.params.push_back(v4.y() + (float)pos.y());
    instance.params.push_back(v4.z() + (float)pos.z());
    instance.params.push_back((float)(3 + type));
    instance.params.push_back(shadow);
    instance.params.push_back(0);

#if USE_GL_TRIANGLES
    instance.indices.push_back(instance.indiceoffset + 0);
    instance.indices.push_back(instance.indiceoffset + 1);
    instance.indices.push_back(instance.indiceoffset + 2);
    instance.indices.push_back(instance.indiceoffset + 0);
    instance.indices.push_back(instance.indiceoffset + 2);
    instance.indices.push_back(instance.indiceoffset + 3);

    instance.indiceoffset += 4;
#endif

    if(number_of_quads > 1)
    {
        float deg = PI / 2.0f;

        rot.x() = cosf(deg);
        rot.y() = sinf(deg);

        v1 = ROTATE_VECTOR_Z(rot, v1);
        v2 = ROTATE_VECTOR_Z(rot, v2);
        v3 = ROTATE_VECTOR_Z(rot, v3);
        v4 = ROTATE_VECTOR_Z(rot, v4);

        // Corner 1
        instance.params.push_back(v1.x() + (float)pos.x());
        instance.params.push_back(v1.y() + (float)pos.y());
        instance.params.push_back(v1.z() + (float)pos.z());
        instance.params.push_back((float)(0 + type));
        instance.params.push_back(shadow);
        instance.params.push_back(0);

        // Corner 2
        instance.params.push_back(v2.x() + (float)pos.x());
        instance.params.push_back(v2.y() + (float)pos.y());
        instance.params.push_back(v2.z() + (float)pos.z());
        instance.params.push_back((float)(1 + type));
        instance.params.push_back(shadow);
        instance.params.push_back(0);

        // Corner 3
        instance.params.push_back(v3.x() + (float)pos.x());
        instance.params.push_back(v3.y() + (float)pos.y());
        instance.params.push_back(v3.z() + (float)pos.z());
        instance.params.push_back((float)(2 + type));
        instance.params.push_back(shadow);
        instance.params.push_back(0);

        // Corner 4
        instance.params.push_back(v4.x() + (float)pos.x());
        instance.params.push_back(v4.y() + (float)pos.y());
        instance.params.push_back(v4.z() + (float)pos.z());
        instance.params.push_back((float)(3 + type));
        instance.params.push_back(shadow);
        instance.params.push_back(0);

#if USE_GL_TRIANGLES
        instance.indices.push_back(instance.indiceoffset + 0);
        instance.indices.push_back(instance.indiceoffset + 1);
        instance.indices.push_back(instance.indiceoffset + 2);
        instance.indices.push_back(instance.indiceoffset + 0);
        instance.indices.push_back(instance.indiceoffset + 2);
        instance.indices.push_back(instance.indiceoffset + 3);

        instance.indiceoffset += 4;
#endif
    }

    if(number_of_quads > 2)
    {
        float deg = PI / 4.0f;

        rot.x() = cosf(deg);
        rot.y() = sinf(deg);

        v1 = ROTATE_VECTOR_Z(rot, v1);
        v2 = ROTATE_VECTOR_Z(rot, v2);
        v3 = ROTATE_VECTOR_Z(rot, v3);
        v4 = ROTATE_VECTOR_Z(rot, v4);

        // Corner 1
        instance.params.push_back(v1.x() + (float)pos.x());
        instance.params.push_back(v1.y() + (float)pos.y());
        instance.params.push_back(v1.z() + (float)pos.z());
        instance.params.push_back((float)(0 + type));
        instance.params.push_back(shadow);
        instance.params.push_back(0);

        // Corner 2
        instance.params.push_back(v2.x() + (float)pos.x());
        instance.params.push_back(v2.y() + (float)pos.y());
        instance.params.push_back(v2.z() + (float)pos.z());
        instance.params.push_back((float)(1 + type));
        instance.params.push_back(shadow);
        instance.params.push_back(0);

        // Corner 3
        instance.params.push_back(v3.x() + (float)pos.x());
        instance.params.push_back(v3.y() + (float)pos.y());
        instance.params.push_back(v3.z()+ (float)pos.z());
        instance.params.push_back((float)(2 + type));
        instance.params.push_back(shadow);
        instance.params.push_back(0);

        // Corner 4
        instance.params.push_back(v4.x() + (float)pos.x());
        instance.params.push_back(v4.y() + (float)pos.y());
        instance.params.push_back(v4.z() + (float)pos.z());
        instance.params.push_back((float)(3 + type));
        instance.params.push_back(shadow);
        instance.params.push_back(0);

#if USE_GL_TRIANGLES
        instance.indices.push_back(instance.indiceoffset + 0);
        instance.indices.push_back(instance.indiceoffset + 1);
        instance.indices.push_back(instance.indiceoffset + 2);
        instance.indices.push_back(instance.indiceoffset + 0);
        instance.indices.push_back(instance.indiceoffset + 2);
        instance.indices.push_back(instance.indiceoffset + 3);

        instance.indiceoffset += 4;
#endif
    }

    data.clear();

    logTimerEndMacro();
}

/*------------------------------------------------------------------
Function: SpeedGrass::CreateGrassVBO
Description:
-------------------------------------------------------------------*/
void SpeedGrass::CreateGrassVBO(float radius, float height)
{
    static piVECTOR v1, v2, v3, v4;

    v1.set(-radius, 0.0f, height);
    v2.set(-radius, 0.0f, 0.0f);
    v3.set(radius, 0.0f, 0.0f);
    v4.set(radius, 0.0f, height);

    std::vector<float> data;

    // Corner 1
    data.push_back(v1.x);
    data.push_back(v1.y);
    data.push_back(v1.z);
    data.push_back(0);

    // Corner 2
    data.push_back(v2.x);
    data.push_back(v2.y);
    data.push_back(v2.z);
    data.push_back(1);

    // Corner 3
    data.push_back(v3.x);
    data.push_back(v3.y);
    data.push_back(v3.z);
    data.push_back(2);

    // Corner 4
    data.push_back(v4.x);
    data.push_back(v4.y);
    data.push_back(v4.z);
    data.push_back(3);

    float deg = PI / 2.0f;

    piMATRIX rot = IdentityMatrix;

    rot._11 = cosf(deg);
    rot._12 = sinf(deg);
    rot._21 = -rot._12;
    rot._22 =  rot._11;

    math3dVectorMultiply(&v1, &v1, &rot);
    math3dVectorMultiply(&v2, &v2, &rot);
    math3dVectorMultiply(&v3, &v3, &rot);
    math3dVectorMultiply(&v4, &v4, &rot);

    // Corner 1
    data.push_back(v1.x);
    data.push_back(v1.y);
    data.push_back(v1.z);
    data.push_back(0);

    // Corner 2
    data.push_back(v2.x);
    data.push_back(v2.y);
    data.push_back(v2.z);
    data.push_back(1);

    // Corner 3
    data.push_back(v3.x);
    data.push_back(v3.y);
    data.push_back(v3.z);
    data.push_back(2);

    // Corner 4
    data.push_back(v4.x);
    data.push_back(v4.y);
    data.push_back(v4.z);
    data.push_back(3);

    if(number_of_quads > 2)
    {
        float deg = PI / 4.0f;

        piMATRIX rot = IdentityMatrix;

        rot._11 = cosf(deg);
        rot._12 = sinf(deg);
        rot._21 = -rot._12;
        rot._22 =  rot._11;

        math3dVectorMultiply(&v1, &v1, &rot);
        math3dVectorMultiply(&v2, &v2, &rot);
        math3dVectorMultiply(&v3, &v3, &rot);
        math3dVectorMultiply(&v4, &v4, &rot);

        // Corner 1
        data.push_back(v1.x);
        data.push_back(v1.y);
        data.push_back(v1.z);
        data.push_back(0);

        // Corner 2
        data.push_back(v2.x);
        data.push_back(v2.y);
        data.push_back(v2.z);
        data.push_back(1);

        // Corner 3
        data.push_back(v3.x);
        data.push_back(v3.y);
        data.push_back(v3.z);
        data.push_back(2);

        // Corner 4
        data.push_back(v4.x);
        data.push_back(v4.y);
        data.push_back(v4.z);
        data.push_back(3);
    }

    if(!ogl::UseDeprecatedPath())
    {
        int stride = 0;

        int id = vbo->AddBuffer((void*)&data[0], data.size() * sizeof(float), true, oglVertexBuffer::MODE_ARB_VAO);
        vbo->SetStride(oglVertexBuffer::ATTRIBUTE_POSITION, 4, GL_FLOAT, stride, 0, id);
        vbo->AddDrawArraysPrimitive(GL_QUADS, 0, data.size() / 4, id);
    }
}

/*------------------------------------------------------------------
Function: SpeedGrass::Init()
Description:
-------------------------------------------------------------------*/
void SpeedGrass::Init()
{
    if(initialized)
        return;

    if(gCustomTerrain)
    {
        grassTexId = ldTextureLoad(SCEN_MGR->MapPath("radialclutter.dds"));
        number_of_quads = CUSTOM_TERRAIN_NUM_QUADS;
    }
    else
    {
        grassTexId = ldTextureLoad("terrain/grass/radialclutter.dds");
        number_of_quads = DEFAULT_NUM_QUADS;
    }

    std::vector<int> texchoices;

    if(gCustomTerrain)
    {
        texchoices.push_back(0);
        texchoices.push_back(1);
        texchoices.push_back(2);
        texchoices.push_back(3);
    }
    else
    {
        texchoices.push_back(8);
        texchoices.push_back(9);
        texchoices.push_back(10);
        texchoices.push_back(11);
        texchoices.push_back(12);
        texchoices.push_back(4);

        sizescale[4] = 0.35f;
        sizescale[12] = 0.35f;
    }

    float bladeheight = -3.0f;
    float patchwidth = MAX_GRASS_WIDTH;
    float scalemin = 0.0f;
    float scalemax = 0.0f;

    LoadGrassBMP("terrain/grass/grass1.bmp",
                 "grass1",
                 type_grass,
                 bladeheight,
                 patchwidth,
                 scalemin,
                 scalemax,
                 texchoices);

    LoadGrassBMP("terrain/grass/grass2.bmp",
                 "grass2",
                 type_grass,
                 bladeheight,
                 patchwidth,
                 scalemin,
                 scalemax,
                 texchoices);

    LoadGrassBMP("terrain/grass/grass3.bmp",
                 "grass3",
                 type_grass,
                 bladeheight,
                 patchwidth,
                 scalemin,
                 scalemax,
                 texchoices);

    texchoices.clear();

    if(gCustomTerrain)
        texchoices.push_back(0);
    else
    {
        sizescale[4] = 1.0f;
        sizescale[12] = 1.0f;

        texchoices.push_back(1);
        texchoices.push_back(2);
    }

    patchwidth = 16.0f;
    bladeheight = -0.80f;
    scalemin = -0.2f;
    scalemax = 0.2f;

    LoadGrassBMP("terrain/grass/grass_pos.bmp",
                 "radclutter",
                 type_radclutter,
                 bladeheight,
                 patchwidth,
                 scalemin,
                 scalemax,
                 texchoices);

    LoadGrassBMP("terrain/grass/grass_pos.bmp",
                 "radclutter2",
                 type_radclutter,
                 bladeheight,
                 patchwidth,
                 scalemin,
                 scalemax,
                 texchoices);

    LoadGrassBMP("terrain/grass/grass_pos.bmp",
                 "radclutter3",
                 type_radclutter,
                 bladeheight,
                 patchwidth,
                 scalemin,
                 scalemax,
                 texchoices);

    bluescale = 0.00f;
    greenscale = 0.00f;

    whitescale = 0.0f;

    texchoices.clear();

    if(gCustomTerrain)
    {
        texchoices.push_back(0);
        texchoices.push_back(1);
        texchoices.push_back(2);
        texchoices.push_back(3);
    }
    else
    {
        texchoices.push_back(12);
        texchoices.push_back(3);
        texchoices.push_back(4);
        texchoices.push_back(4);
        texchoices.push_back(5);
        texchoices.push_back(5);
        texchoices.push_back(7);
        texchoices.push_back(7);
    }

    bladeheight = -0.6f;

    CreatePatchGrass("radclutter4", type_radclutter, 2.0f, 2.0f, bladeheight, 4);
    CreatePatchGrass("radclutter5", type_radclutter, 2.0f, 2.0f, bladeheight, 8);
    CreatePatchGrass("radclutter6", type_radclutter, 2.0f, 2.0f, bladeheight, 4);
    CreatePatchGrass("radclutter7", type_radclutter, 2.0f, 2.0f, bladeheight, 5);
    CreatePatchGrass("radclutter8", type_radclutter, 2.0f, 2.0f, bladeheight, 5);

    bluescale = 0.00f;
    greenscale = 0.00f;

    whitescale = 0.0f;

    texchoices.clear();

    if(gCustomTerrain)
    {
        texchoices.push_back(0);
        texchoices.push_back(1);
        texchoices.push_back(2);
        texchoices.push_back(3);
    }
    else
    {
        texchoices.push_back(12);
        texchoices.push_back(3);
        texchoices.push_back(4);
        texchoices.push_back(4);
        texchoices.push_back(5);
        texchoices.push_back(5);
        texchoices.push_back(7);
        texchoices.push_back(7);
    }

    bladeheight = -0.6f;

    CreatePatchGrass("radclutter4", type_radclutter, 2.0f, 2.0f, bladeheight, 4);
    CreatePatchGrass("radclutter5", type_radclutter, 2.0f, 2.0f, bladeheight, 8);
    CreatePatchGrass("radclutter6", type_radclutter, 2.0f, 2.0f, bladeheight, 4);
    CreatePatchGrass("radclutter7", type_radclutter, 2.0f, 2.0f, bladeheight, 5);
    CreatePatchGrass("radclutter8", type_radclutter, 2.0f, 2.0f, bladeheight, 5);

#if USE_STATESET
    grassMaterial = new RenderStateSet;
    grassMaterial->SetMode(GL_LIGHTING, 0);
    grassMaterial->SetMode(GL_CULL_FACE, 0);
    grassMaterial->SetRenderState(new TextureUnit(0, grassTexId));
    grassMaterial->SetRenderState(new Program(shadername));
#else
    grassMaterial = new oglMaterial;
    grassMaterial->AddTexture(0, grassTexId);
    grassMaterial->SetShader(shadername);
#endif

    int id = -1;

    if(gCustomTerrain)
        id = oglFindMaterialName("grassca2");
    else
        id = oglFindMaterialName("grass");

#if USE_STATESET
    grassMaterial->SetRenderState(new Material(id));
#else
    grassMaterial->SetMaterial(id);
#endif

    if(vbo.valid())
    {
        CreateGrassVBO(0.5f, -1.0f);

#if USE_STATESET
        glslProgram* pgm = dynamic_cast<Program*>(grassMaterial->GetRenderState(RS_GLSLPROGRAM))->GetProgram();
#else
        glslProgram* pgm = grassMaterial->GetProgram();
#endif
        glslUniform* u = pgm->GetUniform("uv");

        int idx = 0;

        float u1, v1, u2, v2;

        for(int i = 0; i < 16; i++)
        {
            GetUV(i, u1, v1, u2, v2);

            u->SetElement(idx++,  Vec2f(u1, v1));
            u->SetElement(idx++,  Vec2f(u1, v2));
            u->SetElement(idx++,  Vec2f(u2, v2));
            u->SetElement(idx++,  Vec2f(u2, v1));
        }

        pgm->Apply();
        ProgramState::Bind(0);

        vbo->Buffer();

        grasstbo = oglTextureObject::GetOrCreateTextureObject("speedgrass", 32768 * sizeof(float) * 8, GL_RGBA32F_ARB, GL_DYNAMIC_DRAW);

        grass_tbo_size = grasstbo->GetSize();

#if USE_STATESET
        grassMaterial->SetRenderState(new TextureUnit(1, GL_TEXTURE_BUFFER, grasstbo->GetTextureObjectName()));
#else
        grassMaterial->AddTexture(1, GL_TEXTURE_BUFFER, grasstbo->GetTextureObjectName());
#endif
    }

    initialized = true;
}

/*------------------------------------------------------------------
Function: SpeedGrassDeprecated::Init()
Description:
-------------------------------------------------------------------*/
void SpeedGrassDeprecated::Init()
{
    if(gCustomTerrain)
    {
        grassTexId = ldTextureLoad(SCEN_MGR->MapPath("radialclutter.dds"));
        number_of_quads = CUSTOM_TERRAIN_NUM_QUADS;
    }
    else
    {
        grassTexId = ldTextureLoad("terrain/grass/radialclutter.dds");
        number_of_quads = DEFAULT_NUM_QUADS;
    }

    std::vector<int> texchoices;

    // Sometimes the height need to be adjusted based on the type of
    // patch we use. 'heightsscale' will scale the height based on this type.
    // Each height is for a texture on radialclutter.dds

    // texture uv selection from radclutter.dds
    texchoices.push_back(8);
    texchoices.push_back(9);
    texchoices.push_back(10);
    texchoices.push_back(11);
    texchoices.push_back(12);
    texchoices.push_back(4);

    sizescale[4] = 0.35f;
    sizescale[12] = 0.35f;

    float bladeheight = -3.0f;
    float patchwidth = MAX_GRASS_WIDTH;
    float scalemin = 0.0f;
    float scalemax = 0.0f;

    LoadGrassBMP("terrain/grass/grass1.bmp",
                 "grass1",
                 type_grass,
                 bladeheight,
                 patchwidth,
                 scalemin,
                 scalemax,
                 texchoices);

    LoadGrassBMP("terrain/grass/grass2.bmp",
                 "grass2",
                 type_grass,
                 bladeheight,
                 patchwidth,
                 scalemin,
                 scalemax,
                 texchoices);

    LoadGrassBMP("terrain/grass/grass3.bmp",
                 "grass3",
                 type_grass,
                 bladeheight,
                 patchwidth,
                 scalemin,
                 scalemax,
                 texchoices);

    sizescale[4] = 1.0f;
    sizescale[12] = 1.0f;

    texchoices.clear();

    texchoices.push_back(1);
    texchoices.push_back(2);

    patchwidth = 16.0f;
    bladeheight = -0.80f;
    scalemin = -0.2f;
    scalemax = 0.2f;

    LoadGrassBMP("terrain/grass/grass_pos.bmp",
                 "radclutter",
                 type_radclutter,
                 bladeheight,
                 patchwidth,
                 scalemin,
                 scalemax,
                 texchoices);

    LoadGrassBMP("terrain/grass/grass_pos.bmp",
                 "radclutter2",
                 type_radclutter,
                 bladeheight,
                 patchwidth,
                 scalemin,
                 scalemax,
                 texchoices);

    LoadGrassBMP("terrain/grass/grass_pos.bmp",
                 "radclutter3",
                 type_radclutter,
                 bladeheight,
                 patchwidth,
                 scalemin,
                 scalemax,
                 texchoices);

    bluescale = 0.00f;
    greenscale = 0.00f;

    whitescale = 0.0f;

    texchoices.clear();
    texchoices.push_back(12);
    texchoices.push_back(3);
    texchoices.push_back(4);
    texchoices.push_back(4);
    texchoices.push_back(5);
    texchoices.push_back(5);
    texchoices.push_back(7);
    texchoices.push_back(7);

    bladeheight = -0.6f;

    CreatePatchGrass("radclutter4", type_radclutter, 2.0f, 2.0f, bladeheight, 4);
    CreatePatchGrass("radclutter5", type_radclutter, 2.0f, 2.0f, bladeheight, 8);
    CreatePatchGrass("radclutter6", type_radclutter, 2.0f, 2.0f, bladeheight, 4);
    CreatePatchGrass("radclutter7", type_radclutter, 2.0f, 2.0f, bladeheight, 5);
    CreatePatchGrass("radclutter8", type_radclutter, 2.0f, 2.0f, bladeheight, 5);

    // Initialize grass type 1..n of width and height
    srand(3229);

    CreatePatchGrass("grass1c", type_fltgrass, 5, 5, -3.0f, 35);
    CreatePatchGrass("grass2c", type_fltgrass, 5, 5, -3.0f, 35);
    CreatePatchGrass("medgrass1c", type_fltgrass, 4, 50, -3.0f, 250);
    CreatePatchGrass("biggrass1c", type_fltgrass, 6, 82, -3.0f, 400);

#if USE_STATESET
    grassMaterial = new RenderStateSet;
    grassMaterial->SetMode(GL_CULL_FACE, 0);
    grassMaterial->SetMode(GL_BLEND, 0);
    grassMaterial->SetMode(GL_ALPHA_TEST, 1);
    grassMaterial->SetMode(GL_LIGHTING, 0);

    grassMaterial->SetRenderState(new TextureUnit(0, grassTexId));
    grassMaterial->SetRenderState(new TextureUnit(1));
#else
    grassMaterial = new oglMaterial;
    grassMaterial->AddTexture(0, grassTexId);
#endif

    int id = -1;
    id = oglFindMaterialName("grass");

#if USE_STATESET
    grassMaterial->SetRenderState(new Material(id));
    grassMaterial->SetRenderState(new Program(shadername));
#else
    grassMaterial->SetMaterial(id);
    grassMaterial->SetShader(shadername);
#endif

    // Have to load grass shaders programmatically since each scenario may require a unique shader
    if(gCustomTerrain)
    {
        std::string name = SCEN_MGR->MapPath(shadername);

        if(GLSL.getProgram(name.c_str()))
        {
#if USE_STATESET
            dynamic_cast<Program*>(grassMaterial->GetRenderState(RS_GLSLPROGRAM))->SetShader(name);
#else
            grassMaterial->SetShader(name);
#endif
        }
        else
        {
            glslProgram* pgm = GLSL.loadProgram(name.c_str());

            ref_ptr<glslShader> s1 = GLSL.loadVertexShader("fx/shaders/GLSL/grass/spdgrassscenario.vert");

            std::string str = SCEN_MGR->MapPath(shadername, ".frag");

            ref_ptr<glslShader> s2 = GLSL.loadFragmentShader(str.c_str());

            if(!s2.valid())
            {
                s2 = GLSL.loadFragmentShader("fx/shaders/GLSL/grass/spdgrass.frag");
            }

            ref_ptr<glslShader> s3 = GLSL.loadFragmentShader("fx/shaders/GLSL/utility/fogutility.frag");
            ref_ptr<glslShader> s4 = GLSL.loadFragmentShader("fx/shaders/GLSL/utility/lightutility.frag");

            pgm->AddShader(s1.get());
            pgm->AddShader(s2.get());
            pgm->AddShader(s3.get());
            pgm->AddShader(s4.get());

            pgm->GetOrCreateUniform("src1", glslUniform::SAMPLER_2D);
            pgm->GetOrCreateUniform("splat", glslUniform::SAMPLER_2D);
            pgm->GetOrCreateUniform("alphatest", glslUniform::FLOAT);
            pgm->GetOrCreateUniform("uv", glslUniform::FLOAT_VEC2, 64);
            pgm->GetOrCreateUniform("origin", glslUniform::FLOAT_VEC3);

            pgm->GetUniform("src1")->Set(0);
            pgm->GetUniform("splat")->Set(1);

            pgm->GetUniform("src1")->SetOneTimeApply(true);
            pgm->GetUniform("splat")->SetOneTimeApply(true);
            pgm->GetUniform("uv")->SetOneTimeApply(true);

            if(ogl::UseDeprecatedPath())
                glslLoadGlobalUniforms(pgm);

            pgm->Apply();

            if(GLSL.getProgram(name.c_str()))
            {
#if USE_STATESET
                dynamic_cast<Program*>(grassMaterial->GetRenderState(RS_GLSLPROGRAM))->SetShader(name);
#else
                grassMaterial->SetShader(name);
#endif
            }
        }
    }

#if USE_STATESET
    grassShadowMaterial = new RenderStateSet;
    grassShadowMaterial->SetMode(GL_CULL_FACE, 0);
    grassShadowMaterial->SetMode(GL_BLEND, 0);
    grassShadowMaterial->SetMode(GL_ALPHA_TEST, 1);
    grassShadowMaterial->SetMode(GL_LIGHTING, 0);
    grassShadowMaterial->SetRenderState(new TextureUnit(0, grassTexId));
    grassShadowMaterial->SetRenderState(new TextureUnit(1));
    grassShadowMaterial->SetRenderState(new TextureUnit(2));
    grassShadowMaterial->SetRenderState(new TextureMatrix(1));
    grassShadowMaterial->SetRenderState(new TextureMatrix(2));
    grassShadowMaterial->SetRenderState(new Program(shadowshadername));
    grassShadowMaterial->SetRenderState(new Material(id));
#else
    grassShadowMaterial = new oglMaterial;
    grassShadowMaterial->AddTexture(0, grassTexId);
    grassShadowMaterial->SetShader(shadowshadername);
    grassShadowMaterial->SetMaterial(id);
#endif

    if(gCustomTerrain)
    {
        std::string name = SCEN_MGR->MapPath(shadowshadername);

        if(GLSL.getProgram(name.c_str()))
        {
#if USE_STATESET
            dynamic_cast<Program*>(grassShadowMaterial->GetRenderState(RS_GLSLPROGRAM))->SetShader(name);
#else
            grassShadowMaterial->SetShader(name);
#endif
        }
        else
        {
            glslProgram* pgm = GLSL.loadProgram(name.c_str());

            ref_ptr<glslShader> s1 = GLSL.loadVertexShader("fx/shaders/GLSL/grass/spdgrassshadowscenario.vert");

            std::string str = SCEN_MGR->MapPath(shadowshadername, ".frag");

            ref_ptr<glslShader> s2 = GLSL.loadFragmentShader(str.c_str());

            if(!s2.valid())
            {
                s2 = GLSL.loadFragmentShader("fx/shaders/GLSL/grass/spdgrassshadow.frag");
            }

            ref_ptr<glslShader> s3 = GLSL.loadFragmentShader("fx/shaders/GLSL/utility/fogutility.frag");
            ref_ptr<glslShader> s4 = GLSL.loadFragmentShader("fx/shaders/GLSL/utility/lightutility.frag");

            pgm->AddShader(s1.get());
            pgm->AddShader(s2.get());
            pgm->AddShader(s3.get());
            pgm->AddShader(s4.get());

            pgm->GetOrCreateUniform("src1", glslUniform::SAMPLER_2D);
            pgm->GetOrCreateUniform("splat", glslUniform::SAMPLER_2D);
            pgm->GetOrCreateUniform("src2", glslUniform::SAMPLER_2D_SHADOW);
            pgm->GetOrCreateUniform("origin", glslUniform::FLOAT_VEC3);
            pgm->GetOrCreateUniform("rot", glslUniform::FLOAT_VEC4);
            pgm->GetOrCreateUniform("alphatest", glslUniform::FLOAT);
            pgm->GetOrCreateUniform("uv", glslUniform::FLOAT_VEC2, 64);
            pgm->GetOrCreateUniform("scale", glslUniform::FLOAT);
            pgm->GetOrCreateUniform("shadowambient", glslUniform::FLOAT_VEC3);

            pgm->GetUniform("src1")->SetOneTimeApply(true);
            pgm->GetUniform("src2")->SetOneTimeApply(true);
            pgm->GetUniform("splat")->SetOneTimeApply(true);
            pgm->GetUniform("uv")->SetOneTimeApply(true);
            pgm->GetUniform("rot")->SetOneTimeApply(true);

            pgm->GetUniform("src1")->Set(0);
            pgm->GetUniform("src2")->Set(1);
            pgm->GetUniform("splat")->Set(2);
            pgm->GetUniform("rot")->Set(Vec4f(1, 0, 0, 0));

            if(ogl::UseDeprecatedPath())
                glslLoadGlobalUniforms(pgm);

            pgm->Apply();

            if(GLSL.getProgram(name.c_str()))
            {
#if USE_STATESET
                dynamic_cast<Program*>(grassShadowMaterial->GetRenderState(RS_GLSLPROGRAM))->SetShader(name);
#else
                grassShadowMaterial->SetShader(name);
#endif
            }
        }
    }

#if USE_STATESET
    glslProgram* pgm = dynamic_cast<Program*>(grassMaterial->GetRenderState(RS_GLSLPROGRAM))->GetProgram();
#else
    glslProgram* pgm = grassMaterial->GetProgram();
#endif

    glslUniform* u = pgm->GetUniform("uv");

    int idx = 0;

    float u1, v1, u2, v2;

    for(int i = 0; i < 16; i++)
    {
        GetUV(i, u1, v1, u2, v2);

        u->SetElement(idx++,  Vec2f(u1, v1));
        u->SetElement(idx++,  Vec2f(u1, v2));
        u->SetElement(idx++,  Vec2f(u2, v2));
        u->SetElement(idx++,  Vec2f(u2, v1));
    }

#if USE_STATESET
    u = dynamic_cast<Program*>(grassShadowMaterial->GetRenderState(RS_GLSLPROGRAM))->GetProgram()->GetUniform("uv");
#else
    u = grassShadowMaterial->GetProgram()->GetUniform("uv");
#endif

    idx = 0;

    for(int i = 0; i < 16; i++)
    {
        GetUV(i, u1, v1, u2, v2);

        u->SetElement(idx++,  Vec2f(u1, v1));
        u->SetElement(idx++,  Vec2f(u1, v2));
        u->SetElement(idx++,  Vec2f(u2, v2));
        u->SetElement(idx++,  Vec2f(u2, v1));
    }

#if USE_STATESET
    Program* program = dynamic_cast<Program*>(grassShadowMaterial->GetRenderState(RS_GLSLPROGRAM));
    program->GetProgram()->GetUniform("scale")->Set(ShadowMap::GetInstance().GetScale());
#else
    grassShadowMaterial->GetProgram()->GetUniform("scale")->Set(ShadowMap::GetInstance().GetScale());
#endif

    vbo->Buffer();

    initialized = true;
}

/*------------------------------------------------------------------
Function: speedgrassAddPatch
Description: Usually for non-replacement tiles
-------------------------------------------------------------------*/
bool speedgrassAddPatch(const Vec3d& pos, const char* type, const char* key, float fade)
{
    if(!wantspeedgrass)
        return false;

    if(type == NULL || key == NULL)
        return false;

    struct CollideQue* que = new CollideQue;
    que->pos = pos;
    que->type = type;
    que->key = SuperFastHash(key, strlen(key));
    que->fade = fade;
    que->wantcollide = false;
    que->starttime = frametime;

    collideque[que->key] = que;

    return true;
}

/*------------------------------------------------------------------
Function: HeightGrass
Description:
-------------------------------------------------------------------*/
static bool HeightGrass(CollideQue* que)
{
    if(Grass)
        return Grass->GrassFromHeightMap(que);

    return false;
}

/*------------------------------------------------------------------
Function: SpeedGrassDeprecated::GrassFromHeightMap
Description:
-------------------------------------------------------------------*/
bool SpeedGrassDeprecated::GrassFromHeightMap(CollideQue* que)
{
    GrassPatchMap::iterator gp = grasspatch.find(que->type);

    if(gp == grasspatch.end())
        return false;

    // First find the supercell and then determine the row/colum of our octet
    int sx = (int)(que->pos.x() / SDM_SUPERCELL_SIZE);
    int sy = (int)(que->pos.y() / SDM_SUPERCELL_SIZE);

    sx = sx < 0 ? sx - 1 : sx;
    sy = sy < 0 ? sy - 1 : sy;

    TERRAIN_DETAIL* td = NULL;

    for(int q = 0; q < NUM_TERRAIN_RECTANGLES; q++)
    {
        if(activeTerrainDetail[q] && trndtl[q].scx == sx && trndtl[q].scy == sy)
        {
            td = &trndtl[q];
            break;
        }
    }

    if(!td)
        return false;

    // Original patch in memory. Create instance from this
    const Patch& patch = *gp->second;

    que->pos.z() = trnmainHeightAt(td, que->pos.x(), que->pos.y());

    int index = 0;

    float x, y;

    int coordx = 0;
    int coordy = 1;

    index = 0;

    size_t code = que->key;

    bool created = false;
    PatchInstance& instance = Grass->GetOrCreateInstance(code, created);

    int userwanteddensity;

    if(gCustomTerrain)
        userwanteddensity = patch.density;
    else
    {
        float scale = g_fRadialDensity > 0.0 ? g_fRadialDensity : 0.1f;
        userwanteddensity = FloatToInt(patch.density * scale);
    }

    if(created)
    {
        instance.center = que->pos;
        instance.collided = true;
        instance.cullradius = patch.cullradius;
        instance.fadedistance = que->fade;
        instance.type = patch.type;
        instance.instanceof = &patch;

        instance.Z.resize(patch.density * number_of_quads * NUMBER_OF_CORNERS);

        if(instance.type == type_radclutter)
            instance.faderate = 0.20f;
    }
    else
    {
        if(!gCustomTerrain)
        {
            if(ogl::UseDeprecatedPath())
            {
                return false;
            }
        }
    }

    Vec3d pos;

    PNRandom::seedrandom((int)(que->pos.x() + que->pos.y() + 7793) % 360);

    int q = 0;
    int size = number_of_quads * NUMBER_OF_CORNERS;

    for(int i = 0; i < patch.density; i ++)
    {
        x = patch.collide[i].x();
        y = patch.collide[i].y();

        float z = trnmainHeightAt(NULL, x + que->pos.x(), y + que->pos.y());

        for(int r = 0; r < size; r++)
        {
            instance.Z[q++] = z;
        }
    }

    return true;
}

/*------------------------------------------------------------------
Function: IsSheared
Description:
-------------------------------------------------------------------*/
static inline bool IsSheared(OBJECT* o)
{
	return false;

    if(!o->modelInstance)
        return false;

    return o->modelInstance->IsShearPlacement();
}

/*------------------------------------------------------------------
Function: SpeedGrassDeprecated::CollideTheOldWay
Description:
-------------------------------------------------------------------*/
bool SpeedGrassDeprecated::CollideTheOldWay(CollideQue* que, const Patch& grass)
{
    if(!que)
        return false;

    if(!que->o)
        return false;

    bool created = false;
    PatchInstance& instance = Grass->GetOrCreateInstance(que->key, created);

    instance.center = que->pos;
    instance.collided = true;
    instance.cullradius = grass.cullradius;
    instance.fadedistance = que->fade;
    instance.type = grass.type;
    instance.instanceof = &grass;
    instance.userwanteddensity = grass.density;

    int totalsize = grass.density * number_of_quads * NUMBER_OF_CORNERS;
    instance.Z.resize(totalsize);

    for(int i = 0; i < totalsize; i++)
        instance.Z[i] = 10000.0f;

    piVECTOR pp;
    Vec3d center = que->pos - WorldOriginOffset;
    piVECTOR c;
    center.get(c);

    COLLISION col;
    OBJECT* o = que->o;

    float x, y;
    int offset = 0;

    if(o->flag2 & OBJECT_IS_ROTATED)
    {
        instance.rot.x() = o->mat._11;
        instance.rot.y() = -o->mat._12;
        instance.rot.z() = -o->mat._21;

        instance.usematrix = true;
    }

    center.get(pp);

    if(o->flag2 & OBJECT_IS_ROTATED)
    {
        x = pp.x * o->mat._11 +
            pp.y * o->mat._21 +
            pp.z * o->mat._31 +
            o->mat._41;

        y = pp.x * o->mat._12 +
            pp.y * o->mat._22 +
            pp.z * o->mat._32 +
            o->mat._42;
    }
    else
    {
        x = pp.x + o->mat._41;
        y = pp.y + o->mat._42;
    }

    col.oldpos.set(x, y, 10000.0f);
    col.newpos.set(x, y, -10000.0f);

    if(trncdCheckTerrainDown(&col))
    {
        instance.center.z() = col.colpos.z;
    }

   // Need to collide to get z values for this object with this grass type
    for(int i = 0; i < grass.density; i++)
    {
        pp.set(grass.collide[i].x(), grass.collide[i].y(), 0.0f);

        if(o->flag2 & OBJECT_IS_ROTATED)
        {
            x = pp.x * o->mat._11 +
                pp.y * o->mat._21 +
                pp.z * o->mat._31 +
                o->mat._41;

            y = pp.x * o->mat._12 +
                pp.y * o->mat._22 +
                pp.z * o->mat._32 +
                o->mat._42;
        }
        else
        {
            x = pp.x + o->mat._41;
            y = pp.y + o->mat._42;
        }

        col.oldpos.set(x, y, 10000.0f);
        col.newpos.set(x, y, -10000.0f);

        if(trncdCheckTerrainDown(&col))
        {
            instance.Z[offset + 0] = col.colpos.z;
            instance.Z[offset + 1] = col.colpos.z;
            instance.Z[offset + 2] = col.colpos.z;
            instance.Z[offset + 3] = col.colpos.z;
            instance.Z[offset + 4] = col.colpos.z;
            instance.Z[offset + 5] = col.colpos.z;
            instance.Z[offset + 6] = col.colpos.z;
            instance.Z[offset + 7] = col.colpos.z;
        }

        offset += 8;
    }

    return true;
}

/*------------------------------------------------------------------
Function: SpeedGrassDeprecated::CollideWithTerrain()
Description:
-------------------------------------------------------------------*/
#if(0)
static void CollideDown(Vec3d pos, colliderQuery::CollisionList& collisions, const unsigned int bits)
{
    /*
    static colliderQuery collider;
    static bool firsttime = true;

    if(firsttime)
    {
        firsttime = false;
        collider.addGeomToCollideWith((dGeomID)colliderTerrainSpace());
        dGeomID ray = dCreateRay(collider.space(), 10000.0f);
        dGeomRaySet(ray, 0, 0, 0, 0, 0, -1); // Updated when we collide
        collider.addGeom(ray, 0, 0);
    }

    collisions.clear();

    pos = physicsGameToPhysical(pos);

    pos.z() += 0.1f;

    colliderQuery::dGeomList& geoms = collider.geoms();
    dGeomRaySet(geoms[0], pos.x(), pos.y(), pos.z(), 0, 0, -1);
    colliderSetBits(geoms[0], 0, bits);

    collider.setDirty(true);

    if(!collider.findCollisions(true))
    {
        uiChatPrintLine("No Collisions");
    }
    else
    {
        colliderQuery::CollisionList& c = collider.getCollisions();

        dGeomID current = NULL;

        for(unsigned int i = 0; i < c.size(); i++)
        {
            if(current == c[i].g2)
                continue;

            current = c[i].g2;

			// don't use blocks
			if(colliderIsType(c[i].g2, COLLIDER_BLOCK))
				continue;

            if(colliderIsType(c[i].g2, bits))
            {
                collisions.push_back(c[i]);
            }
        }
    }
    */
}
#endif

/*------------------------------------------------------------------
Function: SpeedGrassDeprecated::CollideWithTerrain()
Description:
-------------------------------------------------------------------*/
bool SpeedGrassDeprecated::CollideWithTerrain(CollideQue* que)
{
	static Vec3d down(0.0f, 0.0f, -1.0f);
    Vec3d p = que->pos;
    p.z() = -p.z();

    GrassPatchMap::iterator gp = grasspatch.find(que->type);

    if(gp == grasspatch.end())
        return false;

    // Original patch in memory. Create instance from this
    const Patch& patch = *gp->second;

    // ODE colliders at far distances may not be created. Can't rely on them.
    if(patch.type == type_fltgrass)
    {
        return CollideTheOldWay(que, patch);
    }

    p = physicsGameToPhysical(p);
    p.z() += 1000.0f;

	// use static ray collider instead of creating one each pass
	const colliderQuery::CollisionList& list = collideRayList(COLLIDER_TILE, 10000.0f, p, down);

	if(list.empty())
		return false;

    float centerz = 0;

    PNRandom::seedrandom((int)(que->pos.x() + que->pos.y() + 7793) % 360);

    for(unsigned int j = 0; j < list.size(); j++)
    {
        const colliderQuery::Collision& c = list[j];

        OBJECT* o = (OBJECT*)dGeomGetData(c.g2);

        if(!o)
            continue;

        Vec3d start, end;
        osgGeomRayGet(c.g1, start, end);
        float z = -(float)(start.z() - c.depth);

        que->pos.z() = z;

        break;
    }

    Vec3d pp;

    colliderQuery::dGeomList& geoms = patch.collider->geoms();

    if((int)geoms.size() != patch.density)
    {
        return false;
    }

    int userwanteddensity;

    if(gCustomTerrain)
        userwanteddensity = patch.density;
    else
    {
        float scale = g_fRadialDensity > 0.0 ? g_fRadialDensity : 0.1f;
        userwanteddensity = FloatToInt(patch.density * scale);
    }

    for(int i = 0; i < patch.density; i++)
    {
        pp.x() = patch.collide[i].x();
        pp.y() = patch.collide[i].y();
        pp.z() = 0.0f;

        pp += p;

        dGeomRaySet(geoms[i], pp.x(), pp.y(), pp.z(), 0, 0, -1);
    }

    patch.collider->setDirty(true);

    dGeomID current = NULL;
    bool good = false;

    if(!patch.collider->findCollisions(true))
    {
        return false;
    }

    colliderQuery::CollisionList& collisions = patch.collider->getCollisions();

    current = NULL;

    size_t code = que->key;

    bool created = false;
    PatchInstance& instance = Grass->GetOrCreateInstance(code, created);

    int size = number_of_quads * NUMBER_OF_CORNERS;
    int count = 0;

    if(created)
    {
        instance.center = que->pos;
        instance.collided = true;
        instance.cullradius = patch.cullradius;
        instance.fadedistance = que->fade;
        instance.type = patch.type;
        instance.instanceof = &patch;
        instance.userwanteddensity = userwanteddensity;

        int totalsize = patch.density * number_of_quads * NUMBER_OF_CORNERS;
        instance.Z.resize(totalsize);

        for(int i = 0; i < totalsize; i++)
            instance.Z[i] = 10000.0f;

        int datasize = number_of_quads * NUMBER_OF_CORNERS;

        for(unsigned int j = 0; j < collisions.size(); j++)
        {
            struct colliderQuery::Collision& c = collisions[j];

            if(current == c.g1)
                continue;

            current = c.g1;

            if(colliderIsType(c.g2, COLLIDER_BOX))
                continue;

            OBJECT* o = (OBJECT*)dGeomGetData(c.g2);

            Vec3d pos = physicsPhysicalToGame(c.pos);

            if(o)
            {
                good = IsValid(o);

                if(good)
                {
                    count++;
                    int offset = patch.geommap.find(current)->second;

                    for(int r = 0; r < datasize; r++)
                    {
                        instance.Z[offset + r] = (float)-c.pos.z();
                    }
                }
            }
        }

        if(instance.type == type_radclutter)
            instance.faderate = 0.20f;
    }
    else
    {
        if(!gCustomTerrain)
        {
            if(ogl::UseDeprecatedPath())
            {
                return false;
            }
        }
    }

    return count != 0;
}

/*------------------------------------------------------------------
Function: SpeedGrass::CollideWithTerrain()
Description:
-------------------------------------------------------------------*/
bool SpeedGrass::CollideWithTerrain(CollideQue* que)
{
    GrassPatchMap::iterator gp = grasspatch.find(que->type);

    if(gp == grasspatch.end())
        return false;

    // Original patch in memory. Create instance from this
    const Patch& patch = *gp->second;

    // Place in physics world

    if(patch.density == 0)
    {
        return true;
    }

    if(patch.type == type_grass)
    {
        PNRandom::seedrandom(3229);
    }
    else
    {
        PNRandom::seedrandom(3137);
    }

    Vec3d p = physicsGameToPhysical(que->pos);
    p.z() += 1000.0f;

    colliderQuery::dGeomList& geoms = patch.collider->geoms();

    if((int)geoms.size() != patch.density)
    {
        return false;
    }

    Vec3d pp;

    for(int i = 0; i < patch.density; i++)
    {
        pp.x() = patch.collide[i].x();
        pp.y() = patch.collide[i].y();
        pp.z() = 0.0f;

        pp += p;

        dGeomRaySet(geoms[i], pp.x(), pp.y(), pp.z(), 0, 0, -1);
    }

    patch.collider->setDirty(true);

    dGeomID current = NULL;
    bool good = false;

    if(!patch.collider->findCollisions(true))
    {
        return false;
    }

    colliderQuery::CollisionList& collisions = patch.collider->getCollisions();

    current = NULL;

    size_t code = que->key;

    bool created = false;
    PatchInstance& instance = Grass->GetOrCreateInstance(code, created);

    if(created)
    {
        instance.center = que->pos;
        instance.collided = true;
        instance.cullradius = patch.cullradius;
        instance.fadedistance = que->fade;
        instance.type = patch.type;

        if(instance.type == type_radclutter)
            instance.faderate = 0.20f;
    }

    // Second pass
    for(unsigned int j = 0; j < collisions.size(); j++)
    {
        struct colliderQuery::Collision& c = collisions[j];

        if(current == c.g1)
            continue;

        if(colliderIsType(c.g2, COLLIDER_BOX))
            return false;

        current = c.g1;

        OBJECT* o = (OBJECT*)dGeomGetData(c.g2);

        if(!o)
            continue;

        good = IsValid(o);

        if(good)
        {
            Vec3d pos = physicsPhysicalToGame(c.pos);
            pos.z() = -pos.z();
            Grass->speedgrassPut(pos, instance);
        }
    }

    if(instance.params.empty())
    {
        Grass->EraseInstance(code);
        return false;
    }

    return true;
}

/*------------------------------------------------------------------
Function: speedgrassAddAndCollide
Description: Usually for replacement tiles
-------------------------------------------------------------------*/
bool speedgrassAddAndCollide(const Vec3d& pos, const char* type, const char* key, float fade, trnCDTerrainObject* to, OBJECT* o)
{
    if(!wantspeedgrass)
        return false;

    if(type == NULL || key == NULL)
        return false;

    struct CollideQue* que = new CollideQue;
    que->pos = pos;
    que->type = type;
    que->key = SuperFastHash(key, strlen(key));
    que->fade = fade;
    que->wantcollide = true;
    que->to = to;
    que->starttime = frametime;
    que->o = o;

    collideque[que->key] = que;

    return true;
}

/*------------------------------------------------------------------
Function: speedgrassAddAndCollide
Description: Usually for replacement tiles
-------------------------------------------------------------------*/
bool speedgrassAddAndCollide(const Vec3d& pos, const char* type, size_t key, float fade, trnCDTerrainObject* to, OBJECT* o)
{
    if(!wantspeedgrass)
        return false;

    if(type == NULL || key == 0)
        return false;

    struct CollideQue* que = new CollideQue;
    que->pos = pos;
    que->type = type;
    que->key = key;
    que->fade = fade;
    que->wantcollide = true;
    que->to = to;
    que->starttime = frametime;
    que->o = o;

    collideque[key] = que;

    return true;
}

/*------------------------------------------------------------------
Function: speedgrassGetOrCreateInstance
Description:
-------------------------------------------------------------------*/
PatchInstance* speedgrassGetOrCreateInstance(unsigned int key, const char* name, bool& created)
{
    GrassPatchMap::iterator gp = grasspatch.find(name);

    if(gp == grasspatch.end())
        return NULL;

    PatchInstance& instance = Grass->GetOrCreateInstance(key, created);
    instance.instanceof = gp->second;

    int totalsize = gp->second->density * number_of_quads * NUMBER_OF_CORNERS;
    instance.Z.resize(totalsize);

    return &instance;
}

/*------------------------------------------------------------------
Function: speedgrassAddCoverGrass
Description:
-------------------------------------------------------------------*/
bool speedgrassAddCoverGrass(const Vec3d& pos, PatchInstance& instance, int type, float scale, int texture)
{
    Grass->speedgrassPut(pos, instance, scale, texture);
    return true;
}

/*------------------------------------------------------------------
Function: speedgrassAddCoverGrass
Description:
-------------------------------------------------------------------*/
bool speedgrassAddCoverGrass(const Vec3d& pos, int type, unsigned int key, float scale, int texture, float shadow, int splatid)
{
    size_t code = key;
    Vec3d center;

    if(key == 0)
    {
        int keys[5];

        keys[0] = FloatToInt((float)pos.x() / SDM_SUPERCELL_SIZE);
        keys[1] = FloatToInt((float)pos.y() / SDM_SUPERCELL_SIZE);

        float x = (float)(pos.x() - WorldOriginOffset.x + SDM_OCTET_SIZE_2);
        float y = (float)(pos.y() - WorldOriginOffset.y + SDM_OCTET_SIZE_2);

#define GRID_WIDTH  100.0f
#define GRID_SIZE   8

        keys[2] = (int)(y / GRID_WIDTH);
        keys[3] = (int)(x / GRID_WIDTH);
        keys[4] = type;

        center.x() = -350.0f + keys[3] * 100.0f;
        center.y() = -350.0f + keys[2] * 100.0f;
        center.z() = pos.z();

        center.x() += WorldOriginOffset.x;
        center.y() += WorldOriginOffset.y;

        code = SuperFastHash((const char*)keys, sizeof(int) * 5);
    }

    bool created = false;
    PatchInstance& instance = Grass->GetOrCreateInstance(code, created);

#define CULL_RADIUS 150.0f // 100x100 grid cell. Cull with hypotenuse of 150.0f
#define FADE_DISTANCE 1000.0f

    if(created)
    {
        instance.center = center;
        instance.collided = true;
        instance.cullradius = CULL_RADIUS;
        instance.fadedistance = FADE_DISTANCE;
        instance.type = type;
        instance.splatid = splatid;
    }

    Grass->speedgrassPut(pos, instance, scale, texture, shadow);

    return true;
}

/*------------------------------------------------------------------
Function: SpeedGrassDeprecated::speedgrassPut
Description:
-------------------------------------------------------------------*/
bool SpeedGrassDeprecated::speedgrassPut(const Vec3d& pos, PatchInstance& instance, float scale, int texture, float shadow)
{
    Grass->CreateGrassQuads(instance, pos, scale, texture, shadow);
    instance.primcount++;

    return true;
}

/*------------------------------------------------------------------
Function: SpeedGrass::speedgrassPut
Description:
-------------------------------------------------------------------*/
bool SpeedGrass::speedgrassPut(const Vec3d& pos, PatchInstance& instance, float scale, int texture)
{
    instance.params.push_back((float)pos.x());
    instance.params.push_back((float)pos.y());
    instance.params.push_back((float)pos.z());
    instance.params.push_back(0);

    if(gCustomTerrain)
        FindGrassParamsScenario(instance.params, instance.type, pos, scale, texture);
    else
        FindGrassParams2(instance.params, instance.type, pos, scale);

    instance.primcount++;

    return true;
}

/*------------------------------------------------------------------
Function: CollideGrass
Description:
-------------------------------------------------------------------*/
static bool CollideGrass(const CollideQue* que)
{
    if(!Grass->CollideWithTerrain((CollideQue*)que))
    {
        return false;
    }

    return true;
}

/*------------------------------------------------------------------
Function: speedgrassGetGrassByPos
Description:
-------------------------------------------------------------------*/
const char* speedgrassGetGrassByPos(const Vec4d& pos)
{
    unsigned int size = clutterregister[SpeedGrassDeprecated::type_grass].size();

    if(size == 0)
        return NULL;

	unsigned int id = (int)(7159 * (pos.x() - (int)pos.x()) + 15137 * (pos.y() - (int)pos.y())) % size;

    return clutterregister[SpeedGrassDeprecated::type_grass][id].c_str();
}

/*------------------------------------------------------------------
Function: speedgrassGetClutterByPos
Description:
-------------------------------------------------------------------*/
const char* speedgrassGetClutterByPos(const Vec4d& pos)
{
    unsigned int size = clutterregister[SpeedGrassDeprecated::type_clutter].size();

    if(size == 0)
        return NULL;

	unsigned int id = (unsigned int)(7159 * (pos.x() - (int)pos.x()) + 15137 * (pos.y() - (int)pos.y())) % size;

    return clutterregister[SpeedGrassDeprecated::type_clutter][id].c_str();
}

/*------------------------------------------------------------------
Function: speedgrassGetRandomRadialType
Description:
-------------------------------------------------------------------*/
const char* speedgrassGetRandomRadialType()
{
    unsigned int size = clutterregister[SpeedGrassDeprecated::type_radclutter].size();

    if(size == 0)
        return NULL;

    int idx = rand() % size;

    return clutterregister[SpeedGrassDeprecated::type_radclutter][idx].c_str();
}

/*------------------------------------------------------------------
Function: FreeAttribute
Description:
-------------------------------------------------------------------*/
static void FreeAttribute(const CollideQue* que)
{
    radclutterFreeAttributeIfNecessary(que->key);
    trnClutterFreeAttributeIfNecessary(que->key);
}

/*------------------------------------------------------------------
Function: speedgrassCull
Description:
-------------------------------------------------------------------*/
void speedgrassCull()
{
    if(Grass)
        Grass->Cull();
}

/*------------------------------------------------------------------
Function: SortByKey
Description:
-------------------------------------------------------------------*/
static bool SortByKey(PatchInstance* l, PatchInstance* r)
{
    return l->instanceof->key < r->instanceof->key;
}

/*------------------------------------------------------------------
Function: SortFrontToBack
Description:
-------------------------------------------------------------------*/
static bool SortFrontToBack(PatchInstance* l, PatchInstance* r)
{
    return l->distance < r->distance;
}

/*------------------------------------------------------------------
Function: SpeedGrassDeprecated::Cull()
Description:
-------------------------------------------------------------------*/
void SpeedGrassDeprecated::Cull()
{
    if(!wantspeedgrass)
        return;

    drawlist.clear();

#ifdef DEBUG_WW2
    totalclutter = 0;
    totalradclutter = 0;
    totalgrass = 0;
#endif

    float dis = 0;
    // Scale to field of view
    float distratio = FieldOfView / 90.0f;

    piVECTOR v;

    GrassMap::iterator itr = grassMap.begin();

    for(; itr != grassMap.end(); ++itr)
    {
        PatchInstance& pi = *itr->second;

        if(!pi.collided)
            continue;

        v.x = (float)(pi.center.x() - WorldOriginOffset.x);
        v.y = (float)(pi.center.y() - WorldOriginOffset.y);
        v.z = (float)pi.center.z();

        if(!rnvisVectorFOVCheck(&v, pi.cullradius))
        {
            continue;
        }

        dis = distratio * (math3dDotProduct(&v, &playdir) + playtrans.z);

        if(dis > pi.fadedistance + 5.0f)
        {
            continue;
        }

#ifdef DEBUG_WW2
        if(pi.type == type_clutter)
            totalclutter++;
        else if(pi.type == type_radclutter)
            totalradclutter++;
        else if(pi.type == type_grass)
            totalgrass++;
#endif

        pi.distance = dis;

        drawlist.push_back(&pi);
    }

    std::sort(drawlist.begin(), drawlist.end(), gCustomTerrain ? SortFrontToBack : SortByKey);
}

/*------------------------------------------------------------------
Function: SpeedGrass::Cull()
Description:
-------------------------------------------------------------------*/
void SpeedGrass::Cull()
{
    if(!wantspeedgrass)
        return;

    drawlist.clear();

#ifdef DEBUG_WW2
    totalclutter = 0;
    totalradclutter = 0;
    totalgrass = 0;
#endif

    float dis = 0;
    // Scale to field of view
    float distratio = FieldOfView / 90.0f;

    piVECTOR v;

    GrassMap::iterator itr = grassMap.begin();

    unsigned int mask = CLIP_ALL_PLANES;
    static BoundingBox bb;

    for(; itr != grassMap.end(); ++itr)
    {
        PatchInstance& pi = *itr->second;

        if(!pi.collided)
            continue;

        if(pi.params.empty())
            continue;

        v.x = (float)(pi.center.x() - WorldOriginOffset.x);
        v.y = (float)(pi.center.y() - WorldOriginOffset.y);
        v.z = (float)pi.center.z();

        bb.set(v.x - pi.cullradius, v.y - pi.cullradius, v.z - 2.0f, v.x + pi.cullradius, v.y + pi.cullradius, v.z + 2.0f);

        mask = CLIP_ALL_PLANES;

        if(rnvisBoundBoxCull(mask, bb))
        {
            continue;
        }

        dis = distratio * (math3dDotProduct(&v, &playdir) + playtrans.z);

        if(dis > pi.fadedistance + 5.0f)
        {
            continue;
        }

        pi.distance = dis;
        pi.lod = 0;

        drawlist.push_back(&pi);
    }

    std::sort(drawlist.begin(), drawlist.end(), SortFrontToBack);
}

/*------------------------------------------------------------------
Function: speedgrassClear
Description:
-------------------------------------------------------------------*/
void speedgrassClear()
{
    drawlist.clear();
    collideque.clear();
}

/*------------------------------------------------------------------
Function: speedgrassUpdate
Description:
-------------------------------------------------------------------*/
void speedgrassUpdate()
{
    Grass->Update();
}

/*------------------------------------------------------------------
Function: SpeedGrassDeprecated::Update
Description:
-------------------------------------------------------------------*/
void SpeedGrassDeprecated::Update()
{
    Grass->SetUpdateTime(frametime);

    const mlVehicleData* data = persGetPersonaModelDataPtr();

    if(!data)
        return;

    if(data->SpawnTime == 0)
        return;

    if(!KillList.empty())
    {
        for(unsigned int i = 0; i < KillList.size(); i++)
            EraseInstance(KillList[i]);

        KillList.clear();
    }

#define max_collisions_per_frame    6
    int count = 0;

    GrassCollideQue::iterator itr = collideque.begin();
    std::vector<GrassCollideQue::iterator> freelist;

    for(; itr != collideque.end(); ++itr)
    {
        CollideQue* que = itr->second;

        if(que->wantcollide && que->to)
        {
            if((frametime - que->starttime) > QUE_TIME_LIMIT)
            {
                que->wantcollide = false;
            }

            // Make sure terrain is loaded first
            if(!que->to->physicsColliderSpace)
                continue;
        }

        if(que->wantcollide)
        {
            if(wantspeedgrass && !CollideGrass(que))
                FreeAttribute(que);
        }
        else
        {
            if(wantspeedgrass && !HeightGrass(que))
                FreeAttribute(que);
        }

        freelist.push_back(itr);
        count++;

        if(count == max_collisions_per_frame)
            break;
    }

    for(unsigned int i = 0; i < freelist.size(); i++)
    {
        itr = freelist[i];
        delete itr->second;

        collideque.erase(itr);
    }
}

/*------------------------------------------------------------------
Function: speedgrassGetStats
Description:
-------------------------------------------------------------------*/
void speedgrassGetStats(char buff[])
{
#ifdef DEBUG_WW2
    sprintf(buff, "SpeedGrass Total: %d (Grass: %d Clutter: %d Radial Clutter: %d Triangles: %d)",
           (int)drawlist.size(), totalgrass, totalclutter, totalradclutter, totaltriangles);
#endif
}
