#include <syscfg.h> // Path is project specific.
/*--------------------------------------------------------------------------------------
Module: glslWater.cpp
Description:
---------------------------------------------------------------------------------------*/
#include "gl/glew.h"
#include "shader/glslWater.h"
#include "shader/glsl.h"
#include "shader/glslProgram.h"
#include "ogl/oglGlobalStates.h"
#include "ogl/oglMesh.h"
#include "ld/ldtex.h"
#include "ld/ldtexture.h"
#include "include/vars.h"
#include "math/math3d.h"
#include "ld/ldpix.h"
#include "osgImport/Texture2D.h"
#include "ob/obdraw.h"
#include "fx/fxWeather.h"
#include "ogl/States.h"
#include "ogl/oglMaterial.h"
#include "ogl/oglTextureObject.h"
#include "fx/postRender.h"
#include "sdm/sdm.h"
#include "ui/uiDotCommands.h"

#define MAX_WATER_PRIMITIVES 1000

static int _waternormal = -1;
static int _watertex = -1;
static int _waterfoam = -1;
static int _waterheight = -1;
static int _waterreflection = -1;
static int _watermaterial = -1;
static int _watermaterialcloudy = -1;

static bool _initfailed = true;
static bool reset = true;
static bool waterdrawn = false;

static float distance2 = 0.0f;
static float waterheight = 0.0f;
static float watermindistance = FLT_MAX;

#include "scen/ScenarioManager.h"
#include "osgImport/ShadowMap.h"

#if USE_STATESET
static ref_ptr<RenderStateSet> h2oshadowmaterial;
static ref_ptr<RenderStateSet> h2omaterial;
static ref_ptr<RenderStateSet> scenariocoast;
static ref_ptr<RenderStateSet> scenarioriver;
static ref_ptr<RenderStateSet> scenarioocean;
#else
static ref_ptr<oglMaterial> h2oshadowmaterial;
static ref_ptr<oglMaterial> h2omaterial;
static ref_ptr<oglMaterial> scenariocoast;
static ref_ptr<oglMaterial> scenarioriver;
static ref_ptr<oglMaterial> scenarioocean;
#endif

static std::vector<OBJECT*> waterbin;
static std::vector<OBJECT*> scenariocoastbin;
static std::vector<OBJECT*> scenarioriverbin;
static std::vector<OBJECT*> scenariooceanbin;

extern Texture2D* obdrawGetWaterReflectionTexture();
extern Texture2D* obdrawGetWaterRefractionTexture();
extern Texture2D* obdrawGetSurfaceTexture();

static void DrawScenarioCoastBin();
static void DrawScenarioRiverBin();
static void DrawScenarioOceanBin();
static void DrawScenarioCoastEffectBin();

/*--------------------------------------------------------------------------------------
Function: glslWaterCanReflect
Description: Must be called before glslWaterGetClosestWaterHeight
---------------------------------------------------------------------------------------*/
bool glslWaterCanReflect()
{
    return OverrideSpawner == NOSPAWN_OVERRIDE || (waterdrawn && (fabs(watermindistance) < SDM_OCTET_SIZE));
}

/*--------------------------------------------------------------------------------------
Function: glslWaterDraw
Description:
---------------------------------------------------------------------------------------*/
bool glslWaterDraw()
{
    return waterdrawn;
}

/*--------------------------------------------------------------------------------------
Function: glslWaterReset
Description:
---------------------------------------------------------------------------------------*/
void glslWaterReset()
{
    reset = true;
    waterdrawn = false;
    watermindistance = FLT_MAX;
}

/*--------------------------------------------------------------------------------------
Function: glslWaterGetClosestWaterHeight
Description:
---------------------------------------------------------------------------------------*/
float glslWaterGetClosestWaterHeight()
{
    return waterheight;
}

/*--------------------------------------------------------------------------------------
Function: glslWaterUpdate
Description:
---------------------------------------------------------------------------------------*/
void glslWaterUpdate()
{
    if(_initfailed)
    {
        return;
    }

    // Since programs share data, only need to do this once per frame
    static int firsttime = frametime;

    float dtime = (float)(frametime - firsttime) * 0.001f;

#if USE_STATESET
    glslProgram* pgm = dynamic_cast<Program*>(h2omaterial->GetRenderState(RS_GLSLPROGRAM))->GetProgram();
#else
    glslProgram* pgm = h2omaterial->GetProgram();
#endif

    pgm->GetUniform("time")->Set(dtime);

    if(ogl::UseDeprecatedPath())
    {
        if(gCustomTerrain)
        {
#if USE_STATESET
            glslProgram* pgm = dynamic_cast<Program*>(h2oshadowmaterial->GetRenderState(RS_GLSLPROGRAM))->GetProgram();
            pgm->GetUniform("time")->Set(dtime);
#else
            h2oshadowmaterial->GetProgram()->GetUniform("time")->Set(dtime);
#endif
        }
    }
}

/*--------------------------------------------------------------------------------------
Function: OceanDrawCallback
Description:
---------------------------------------------------------------------------------------*/
static bool OceanDrawCallback(OBJECT* o)
{
    waterbin.push_back(o);
    return true;
}

/*--------------------------------------------------------------------------------------
Function: glslWaterClearBin
Description:
---------------------------------------------------------------------------------------*/
void glslWaterClearBin()
{
    waterbin.clear();
    scenariocoastbin.clear();
    scenarioriverbin.clear();
    scenariooceanbin.clear();
}

#if USE_STATESET
/*--------------------------------------------------------------------------------------
Function: glslWaterDrawBinDeprecated
Description:
---------------------------------------------------------------------------------------*/
void glslWaterDrawBinDeprecated(bool fbodraw)
{
    if(fbodraw)
    {
        if(!scenariocoastbin.empty())
        {
            DrawScenarioCoastEffectBin();
        }

        return;
    }

    if(!scenariocoastbin.empty())
    {
        DrawScenarioCoastBin();
    }

    if(!scenarioriverbin.empty())
    {
        DrawScenarioRiverBin();
    }

    if(!scenariooceanbin.empty())
    {
        DrawScenarioOceanBin();
    }

    if(waterbin.empty())
        return;

    waterdrawn = true;

    const unsigned int size = waterbin.size();
    OBJECT* o = nullptr;

    int matid;

    if(fxWeatherIsCloudy())
        matid = _watermaterialcloudy;
    else
        matid = _watermaterial;

    dynamic_cast<Material*>(h2omaterial->GetRenderState(RS_MATERIAL))->SetMaterialID(matid);

    if(!gWaterShadersEnabled)
    {
        h2omaterial->GetRenderState(RS_GLSLPROGRAM)->SetEnabled(false);
        h2omaterial->GetRenderState(RS_TEXTURE_UNIT1)->SetEnabled(false);
        h2omaterial->GetRenderState(RS_TEXTURE_UNIT2)->SetEnabled(false);
        h2omaterial->GetRenderState(RS_TEXTURE_UNIT3)->SetEnabled(false);

        for(unsigned int i = 0; i < size; ++i)
        {
            o = waterbin[i];
            oglSetWorldMatrix(&o->mat);

            h2omaterial->Apply();

            if(o->ptverts)
            {
                piRenderScene(piPT_TRIANGLELIST, piVT_CVERTEX, o->ptverts, o->numtverts);
            }
            else
            {
                obdrawDrawMesh(o);
            }
        }

        waterbin.clear();

        return;
    }

    h2omaterial->GetRenderState(RS_GLSLPROGRAM)->SetEnabled(true);
    h2omaterial->GetRenderState(RS_TEXTURE_UNIT1)->SetEnabled(true);
    h2omaterial->GetRenderState(RS_TEXTURE_UNIT2)->SetEnabled(true);
    h2omaterial->GetRenderState(RS_TEXTURE_UNIT3)->SetEnabled(true);

    if(g_wantWaterReflections)
    {
        Texture2D* t = obdrawGetWaterReflectionTexture();
        dynamic_cast<TextureUnit*>(h2omaterial->GetRenderState(RS_TEXTURE_UNIT2))->SetTexture(t);

        if(gCustomTerrain)
        {
            dynamic_cast<TextureUnit*>(h2oshadowmaterial->GetRenderState(RS_TEXTURE_UNIT2))->SetTexture(t);
        }
    }
    else
    {
        dynamic_cast<TextureUnit*>(h2omaterial->GetRenderState(RS_TEXTURE_UNIT2))->SetTexture(_waterreflection);

        if(gCustomTerrain)
        {
            dynamic_cast<TextureUnit*>(h2oshadowmaterial->GetRenderState(RS_TEXTURE_UNIT2))->SetTexture(_waterreflection);
        }
    }

    Texture2D* t = obdrawGetSurfaceTexture();
    dynamic_cast<TextureUnit*>(h2omaterial->GetRenderState(RS_TEXTURE_UNIT3))->SetTexture(t);

    if(gCustomTerrain)
    {
        dynamic_cast<TextureUnit*>(h2oshadowmaterial->GetRenderState(RS_TEXTURE_UNIT3))->SetTexture(t);
    }

    if(gCustomTerrain)
    {
        dynamic_cast<Material*>(h2oshadowmaterial->GetRenderState(RS_MATERIAL))->SetMaterialID(matid);

        if(ShadowMap::GetInstance().Enabled() && ShadowMap::GetInstance().Valid())
        {
            dynamic_cast<TextureUnit*>(h2oshadowmaterial->GetRenderState(RS_TEXTURE_UNIT4))->SetTexture(ShadowMap::GetInstance().GetShadowMapTexture());
            h2oshadowmaterial->Apply();
        }
        else
            h2omaterial->Apply();
    }
    else
        h2omaterial->Apply();

    for(unsigned int i = 0; i < size; ++i)
    {
        o = waterbin[i];
        oglSetWorldMatrix(&o->mat);

        watermindistance = std::min(watermindistance, o->distsqr);

        if(o->ptverts)
        {
            piRenderScene(piPT_TRIANGLELIST, piVT_CVERTEX, o->ptverts, o->numtverts);
        }
        else
        {
            obdrawDrawMesh(o);
        }
    }

    oglSetWorldMatrix(&IdentityMatrix);

    waterbin.clear();
};
#else
/*--------------------------------------------------------------------------------------
Function: glslWaterDrawBinDeprecated
Description:
---------------------------------------------------------------------------------------*/
void glslWaterDrawBinDeprecated(bool fbodraw)
{
    if(!scenariocoastbin.empty())
    {
        DrawScenarioCoastBin();
        scenariocoastbin.clear();
    }

    if(!scenarioriverbin.empty())
    {
        DrawScenarioRiverBin();
        scenarioriverbin.clear();
    }

    if(!scenariooceanbin.empty())
    {
        DrawScenarioOceanBin();
        scenariooceanbin.clear();
    }

    if(waterbin.empty())
        return;

    waterdrawn = true;

    const unsigned int size = waterbin.size();
    OBJECT* o = nullptr;

    int matid;

    if(fxWeatherIsCloudy())
        matid = _watermaterialcloudy;
    else
        matid = _watermaterial;

    h2omaterial->SetMaterial(matid);

    if(!gWaterShadersEnabled)
    {
        ProgramState::Bind(0);

        for(unsigned int i = 0; i < size; ++i)
        {
            o = waterbin[i];
            oglSetWorldMatrix(&o->mat);

            h2omaterial->ApplyMaterial();

            TextureState::Start();
            h2omaterial->ApplyTexture(0);
            TextureState::End();

            if(o->ptverts)
            {
                piRenderScene(piPT_TRIANGLELIST, piVT_CVERTEX, o->ptverts, o->numtverts);
            }
            else
            {
                obdrawDrawMesh(o);
            }
        }

        waterbin.clear();
        return;
    }

    if(g_wantWaterReflections)
    {
        Texture2D* t = obdrawGetWaterReflectionTexture();
        h2omaterial->AddTexture(2, t);

        if(gCustomTerrain)
            h2oshadowmaterial->AddTexture(2, t);
    }
    else
    {
        h2omaterial->AddTexture(2, _waterreflection);

        if(gCustomTerrain)
            h2oshadowmaterial->AddTexture(2, _waterreflection);
    }

    Texture2D* t = obdrawGetSurfaceTexture();
    h2omaterial->AddTexture(3, t);

    if(gCustomTerrain)
        h2oshadowmaterial->AddTexture(3, t);

    if(gCustomTerrain)
    {
        h2oshadowmaterial->SetMaterial(matid);

        if(ShadowMap::GetInstance().Enabled() && ShadowMap::GetInstance().Valid())
        {
            ShadowMap::GetInstance().SetShadowStuff(*h2oshadowmaterial, 4);
            h2oshadowmaterial->Apply();
        }
        else
            h2omaterial->Apply();
    }
    else
        h2omaterial->Apply();

    piSetRenderState(piRENDERSTATE_LIGHTING, 0);
	piSetRenderState(piRENDERSTATE_ZFUNC, ZFunction);

    for(unsigned int i = 0; i < size; ++i)
    {
        o = waterbin[i];

        watermindistance = std::min(watermindistance, o->distsqr);

        oglSetWorldMatrix(&o->mat);

        if(o->ptverts)
        {
            piRenderScene(piPT_TRIANGLELIST, piVT_CVERTEX, o->ptverts, o->numtverts);
        }
        else
        {
            obdrawDrawMesh(o);
        }
    }

    piSetRenderState(piRENDERSTATE_LIGHTING, piLIGHTING_DEFAULT);
    oglSetWorldMatrix(&IdentityMatrix);

    if(gCustomTerrain)
    {
        if(ShadowMap::GetInstance().Enabled() && ShadowMap::GetInstance().Valid())
        {
            ShadowMap::GetInstance().UnApplyShadowMaps(4);
        }
        else
            ProgramState::Bind(0);
    }
    else
        ProgramState::Bind(0);

    waterbin.clear();
};
#endif

/*--------------------------------------------------------------------------------------
Function: AddWaterInstance
Description:
---------------------------------------------------------------------------------------*/
static void AddWaterInstance(const OBJECT* o, bool push = false)
{
    static std::vector<Vec4f> data;
    static ref_ptr<oglTextureObject> tbo;

    if(!tbo.valid())
    {
        tbo = oglTextureObject::GetTextureObject("dynamicglobal");
    }

    if(push)
    {
        if(!data.empty())
        {
            tbo->UpdateBuffer(data.size() * sizeof(Vec4f), &data[0]);

            if(data.size() > 1)
                obdrawDrawObjectInstanced(o, 0, data.size());
            else
                obdrawDrawObject(o);
        }

        data.clear();
    }
    else
    {
        data.push_back(Vec4f(o->mat._41, o->mat._42, o->mat._43, o->att.z));

        if(data.size() == MAX_WATER_PRIMITIVES)
        {
            tbo->UpdateBuffer(data.size() * sizeof(Vec4f), &data[0]);

            obdrawDrawObjectInstanced(o, 0, data.size());

            data.clear();
        }
    }
}

#if USE_STATESET
/*--------------------------------------------------------------------------------------
Function: glslWaterDrawBin
Description:
---------------------------------------------------------------------------------------*/
void glslWaterDrawBin()
{
    if(waterbin.empty())
        return;

    waterdrawn = true;

    oglSetWorldMatrix(&IdentityMatrix);

    const unsigned int size = waterbin.size();
    OBJECT* o = nullptr;

    if(g_wantWaterReflections)
    {
        Texture2D* t = obdrawGetWaterReflectionTexture();
        dynamic_cast<TextureUnit*>(h2omaterial->GetRenderState(RS_TEXTURE_UNIT2))->SetTexture(t);
    }
    else
    {
        dynamic_cast<TextureUnit*>(h2omaterial->GetRenderState(RS_TEXTURE_UNIT2))->SetTexture(_waterreflection);
    }

    Texture2D* t = obdrawGetSurfaceTexture();
    dynamic_cast<TextureUnit*>(h2omaterial->GetRenderState(RS_TEXTURE_UNIT3))->SetTexture(t);

    int matid;

    if(fxWeatherIsCloudy())
        matid = _watermaterialcloudy;
    else
        matid = _watermaterial;

    dynamic_cast<Material*>(h2omaterial->GetRenderState(RS_MATERIAL))->SetMaterialID(matid);
    h2omaterial->Apply();

    const piUNIFIED_VERTEX* old = nullptr;
    const OBJECT* last = nullptr;

    for(unsigned int i = waterbin.size(); i != 0; --i)
    {
        o = waterbin[i - 1];

        if(old != o->RenderData())
        {
            if(last)
            {
                AddWaterInstance(last, true);
            }
        }

        old = o->RenderData();
        last = o;

        AddWaterInstance(o);
    }

    AddWaterInstance(last, true);
    waterbin.clear();
};
#else
/*--------------------------------------------------------------------------------------
Function: glslWaterDrawBin
Description:
---------------------------------------------------------------------------------------*/
void glslWaterDrawBin()
{
    if(waterbin.empty())
        return;

    waterdrawn = true;

    oglSetWorldMatrix(&IdentityMatrix);

    const unsigned int size = waterbin.size();
    OBJECT* o = nullptr;

    if(g_wantWaterReflections)
    {
        Texture2D* t = obdrawGetWaterReflectionTexture();
        h2omaterial->AddTexture(2, t);
    }
    else
    {
        h2omaterial->AddTexture(2, _waterreflection);
    }

    Texture2D* t = obdrawGetSurfaceTexture();
    h2omaterial->AddTexture(3, t);

    int matid;

    if(fxWeatherIsCloudy())
        matid = _watermaterialcloudy;
    else
        matid = _watermaterial;

    h2omaterial->SetMaterial(matid);
    h2omaterial->Apply();

    const piUNIFIED_VERTEX* old = nullptr;
    const OBJECT* last = nullptr;

#if DEBUG_WW2
    int same = 0;
#endif

    for(unsigned int i = waterbin.size(); i != 0; --i)
    {
        o = waterbin[i - 1];

        if(old != o->RenderData())
        {
            if(last)
            {
                AddWaterInstance(last, true);
            }
        }
#if DEBUG_WW2
        else
        {
            same++;
        }
#endif

        old = o->RenderData();
        last = o;

        AddWaterInstance(o);
    }

    AddWaterInstance(last, true);
    waterbin.clear();
};
#endif

/*--------------------------------------------------------------------------------------
Function: ScenarioCoastDrawCallback
Description:
---------------------------------------------------------------------------------------*/
static bool ScenarioCoastDrawCallback(OBJECT* o)
{
    if(!gWaterShadersEnabled)
        waterbin.push_back(o);
    else
        scenariocoastbin.push_back(o);

    return true;
}

/*--------------------------------------------------------------------------------------
Function: ScenarioRiverDrawCallback
Description:
---------------------------------------------------------------------------------------*/
static bool ScenarioRiverDrawCallback(OBJECT* o)
{
    if(!gWaterShadersEnabled)
        waterbin.push_back(o);
    else
        scenarioriverbin.push_back(o);

    return true;
}

/*--------------------------------------------------------------------------------------
Function: ScenarioOceanDrawCallback
Description:
---------------------------------------------------------------------------------------*/
static bool ScenarioOceanDrawCallback(OBJECT* o)
{
    if(!gWaterShadersEnabled)
        waterbin.push_back(o);
    else
        scenariooceanbin.push_back(o);

    return true;
}

#if USE_STATESET
/*--------------------------------------------------------------------------------------
Function: DrawScenarioCoastEffectBin
Description: HACK
---------------------------------------------------------------------------------------*/
static void DrawScenarioCoastEffectBin()
{
   if(scenariocoastbin.empty())
       return;

    static int firsttime = frametime;

    float dtime = (float)(frametime - firsttime) * 0.001f;

    dynamic_cast<Program*>(scenariocoast->GetRenderState(RS_GLSLPROGRAM))->GetProgram()->GetUniform("time")->Set(dtime);

    static ref_ptr<FrameBufferObject> surfaceFBO;
    static ref_ptr<Texture2D> rtt;
    static ref_ptr<RenderStateSet> surface;
    static ref_ptr<ClearNode> clear;

    static int foam = -1;

    if(!surface.valid())
    {
        clear = new ClearNode(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT, true, Vec4f(0, 0, 0, 1));

        foam = ldTextureLoad(SCEN_MGR->MapPath("seafoam.dds"), 0);

        if(!oglIsTexture(foam, 0))
        {
            Write_Debug("Coast scenarios need seafoam.dds");
        }

        surface = new RenderStateSet;
        surface->SetRenderState(new Program("coasteffect"));
        surface->SetRenderState(new TextureUnit(0, obdrawGetSurfaceTexture()));

        TextureUnit* tu = dynamic_cast<TextureUnit*>(scenariocoast->GetRenderState(RS_TEXTURE_UNIT4));
        surface->SetRenderState(new TextureUnit(1, tu->GetTextureTarget(), tu->GetTexture()));

        surface->SetRenderState(new TextureUnit(2, foam));
        surface->SetRenderState(new Viewport(0, 0, 256, 256));

        surfaceFBO = new FrameBufferObject;
        rtt = new Texture2D;

        rtt->setSourceFormat(GL_RGBA);
        rtt->setSourceType(GL_UNSIGNED_BYTE);
        rtt->setInternalFormat(GL_RGBA8);
        rtt->setTextureWidth(256);
        rtt->setTextureHeight(256);

        rtt->setFilter(Texture::MAG_FILTER, Texture::LINEAR);
        rtt->setFilter(Texture::MIN_FILTER, Texture::LINEAR);
        rtt->setWrap(Texture::WRAP_S, Texture::CLAMP);
        rtt->setWrap(Texture::WRAP_T, Texture::CLAMP);

        FrameBufferAttachment* fba = new FrameBufferAttachment(rtt.get());
        surfaceFBO->setAttachment(FrameBufferObject::COLOR_BUFFER0, *fba);

        surface->SetRenderState(surfaceFBO.get());
        surface->SetMode(GL_LIGHTING, 0);

        dynamic_cast<TextureUnit*>(scenariocoast->GetRenderState(RS_TEXTURE_UNIT3))->SetTexture(rtt.get());
    }

    OBJECT* o = scenariocoastbin[0];

    piMATRIX m = o->mat;
    static float speed = 0.6f;

    float offset = (float)sin(dtime * speed);

    float alpha = (1.0f - offset) / 2.0f;
    dynamic_cast<Program*>(surface->GetRenderState(RS_GLSLPROGRAM))->GetProgram()->GetUniform("alpha")->Set(alpha);

    m._42 += offset;

    oglSetWorldMatrix(&m);

    surface->Apply();
    clear->apply();

    obdrawDrawMesh(o);

    States::ApplyRenderStates(nullptr);
}

/*--------------------------------------------------------------------------------------
Function: DrawScenarioCoastBin
Description: HACK
---------------------------------------------------------------------------------------*/
static void DrawScenarioCoastBin()
{
   if(scenariocoastbin.empty())
       return;

    waterdrawn = true;

    for(unsigned int i = 0; i < scenariocoastbin.size(); i++)
    {
        OBJECT* o = scenariocoastbin[i];

        if(g_wantWaterReflections)
        {
            // Need the height of the water plane closest to the user for mirroring
            // the view matrix.
            if(reset)
            {
                reset = false;
                distance2 = o->distsqr > 0.0f ? o->distsqr : 0.0f;
                waterheight = o->pos.z;
            }
            else
            {
                float dist = Vec3f(o->bbox[0]).length2();

                if(dist < distance2)
                {
                    waterheight = o->pos.z;
                    distance2 = dist;
                }

                dist = Vec3f(o->bbox[1]).length2();

                if(dist < distance2)
                {
                    waterheight = o->pos.z;
                    distance2 = dist;
                }

                if(o->distsqr < distance2)
                {
                    waterheight = o->pos.z;
                    distance2 = o->distsqr;
                }
            }
        }

        if(g_wantWaterReflections)
        {
            Texture2D* t = obdrawGetWaterReflectionTexture();
            dynamic_cast<TextureUnit*>(scenariocoast->GetRenderState(RS_TEXTURE_UNIT2))->SetTexture(t);
        }
        else
        {
            dynamic_cast<TextureUnit*>(scenariocoast->GetRenderState(RS_TEXTURE_UNIT2))->SetTexture(_waterreflection);
        }

        int matid;

        if(fxWeatherIsCloudy())
            matid = _watermaterialcloudy;
        else
            matid = _watermaterial;

        dynamic_cast<Material*>(scenariocoast->GetRenderState(RS_MATERIAL))->SetMaterialID(matid);

        scenariocoast->Apply();

        oglSetWorldMatrix(&o->mat);

        if(o->ptverts)
        {
            piRenderScene(piPT_TRIANGLELIST, piVT_CVERTEX, o->ptverts, o->numtverts);
        }
        else
        {
            obdrawDrawMesh(o);
        }
    }

    oglSetWorldMatrix(&IdentityMatrix);

    States::ApplyRenderStates(nullptr);
    States::ApplyRenderModes(nullptr);
}
#else
/*--------------------------------------------------------------------------------------
Function: DrawScenarioCoastBin
Description: HACK
---------------------------------------------------------------------------------------*/
static void DrawScenarioCoastBin()
{
   oglSetRenderState(oglRENDERSTATE_LIGHTING, 0);

   if(scenariocoastbin.empty())
       return;

    waterdrawn = true;

    static int firsttime = frametime;

    float dtime = (float)(frametime - firsttime) * 0.001f;

    scenariocoast->GetProgram()->GetUniform("time")->Set(dtime);

    static ref_ptr<FrameBufferObject> surfaceFBO;
    static ref_ptr<Texture2D> rtt;
    static ref_ptr<oglMaterial> surface;
    static int foam = -1;

    if(!surfaceFBO.valid())
    {
        foam = ldTextureLoad(SCEN_MGR->MapPath("seafoam.dds"), 0);

        if(!oglIsTexture(foam, 0))
        {
            Write_Debug("Coast scenarios need seafoam.dds");
        }

        surface = new oglMaterial;
        surface->SetShader("coasteffect");

        surfaceFBO = new FrameBufferObject;
        rtt = new Texture2D;

        rtt->setSourceFormat(GL_RGBA);
        rtt->setSourceType(GL_UNSIGNED_BYTE);
        rtt->setInternalFormat(GL_RGBA8);
        rtt->setTextureWidth(256);
        rtt->setTextureHeight(256);

        rtt->setFilter(Texture::MAG_FILTER, Texture::LINEAR);
        rtt->setFilter(Texture::MIN_FILTER, Texture::LINEAR);
        rtt->setWrap(Texture::WRAP_S, Texture::CLAMP);
        rtt->setWrap(Texture::WRAP_T, Texture::CLAMP);

        FrameBufferAttachment* fba = new FrameBufferAttachment(rtt.get());
        surfaceFBO->setAttachment(FrameBufferObject::COLOR_BUFFER0, *fba);
    }

    Texture2D* t = obdrawGetSurfaceTexture();

    surface->AddTexture(0, t);
    surface->AddTexture(1, GL_TEXTURE_2D, scenariocoast->GetTexture(4));
    surface->AddTexture(2, foam);

    surfaceFBO->apply();

    oglSetViewport(0, 0, 256, 256);

    oglSetRenderState(oglRENDERSTATE_CLEARCOLOR, piRGBA(0, 0, 0, 1));
    oglClearTargets(oglCLEAR_VIEWPORT);
    oglSetRenderState(oglRENDERSTATE_CLEARCOLOR, FogColor);

    OBJECT* o = scenariocoastbin[0];

    piMATRIX m = o->mat;
    static float speed = 0.6f;

    float offset = (float)sin(dtime * speed);

    float alpha = (1.0f - offset) / 2.0f;
    surface->GetProgram()->GetUniform("alpha")->Set(alpha);

    m._42 += offset;

    oglSetWorldMatrix(&m);

    if(g_wantWaterReflections)
    {
        Texture2D* t = obdrawGetWaterReflectionTexture();
        scenariocoast->AddTexture(2, t);
    }
    else
    {
        scenariocoast->AddTexture(2, _waterreflection);
    }

    scenariocoast->AddTexture(3, rtt.get());

    if(ShadowMap::GetInstance().Enabled() && ShadowMap::GetInstance().Valid())
    {
        ShadowMap::GetInstance().SetShadowStuff(*scenariocoast, 5);
    }
    else
    {
        static int white = -1;

        if(white == -1)
        {
            white = ldTextureLoad("white.dds", 0);
        }

        scenariocoast->AddTexture(5, white);
    }

    int matid;

    if(fxWeatherIsCloudy())
        matid = _watermaterialcloudy;
    else
        matid = _watermaterial;

    scenariocoast->SetMaterial(matid);

    surface->Apply();
    obdrawDrawMesh(o);
    surfaceFBO->UnApply();

    scenariocoast->Apply();

    oglSetViewport(0, 0, WantedWidth, WantedHeight);

    for(unsigned int i = 0; i < scenariocoastbin.size(); i++)
    {
        o = scenariocoastbin[i];
        watermindistance = std::min(watermindistance, o->distsqr);

        if(g_wantWaterReflections)
        {
            // Need the height of the water plane closest to the user for mirroring
            // the view matrix.
            if(reset)
            {
                reset = false;
                distance2 = o->distsqr > 0.0f ? o->distsqr : 0.0f;
                waterheight = o->pos.z;
            }
            else
            {
                float dist = Vec3f(o->bbox[0]).length2();

                if(dist < distance2)
                {
                    waterheight = o->pos.z;
                    distance2 = dist;
                }

                dist = Vec3f(o->bbox[1]).length2();

                if(dist < distance2)
                {
                    waterheight = o->pos.z;
                    distance2 = dist;
                }

                if(o->distsqr < distance2)
                {
                    waterheight = o->pos.z;
                    distance2 = o->distsqr;
                }
            }
        }

        oglSetWorldMatrix(&o->mat);

        if(o->ptverts)
        {
            piRenderScene(piPT_TRIANGLELIST, piVT_CVERTEX, o->ptverts, o->numtverts);
        }
        else
        {
            obdrawDrawMesh(o);
        }
    }

    piSetRenderState(piRENDERSTATE_LIGHTING, piLIGHTING_DEFAULT);
    oglSetWorldMatrix(&IdentityMatrix);

    TextureState::SetTextureUnit(0);
    oglSetTextureMatrix(&IdentityMatrix);

    if(ShadowMap::GetInstance().Enabled() && ShadowMap::GetInstance().Valid())
    {
        ShadowMap::GetInstance().UnApplyShadowMaps(5);
    }
    else
        ProgramState::Bind(0);
}
#endif

#if USE_STATESET
/*--------------------------------------------------------------------------------------
Function: DrawScenarioRiverBin
Description: HACK
---------------------------------------------------------------------------------------*/
static void DrawScenarioRiverBin()
{
   if(scenarioriverbin.empty())
       return;

    waterdrawn = true;

    static int firsttime = frametime;

    float dtime = (float)(frametime - firsttime) * 0.001f;

    glslProgram* pgm = dynamic_cast<Program*>(scenarioriver->GetRenderState(RS_GLSLPROGRAM))->GetProgram();
    pgm->GetUniform("time")->Set(dtime);

    if(ShadowMap::GetInstance().Enabled())
    {
        Vec3f ambient;
        ShadowMap::GetInstance().GetShadowAmbient(ambient);

        pgm->GetUniform("shadowambient")->Set(ambient);
    }

    TextureUnit* tu;

    if(g_wantWaterReflections)
    {
        Texture2D* t = obdrawGetWaterReflectionTexture();
        tu = dynamic_cast<TextureUnit*>(scenarioriver->GetRenderState(RS_TEXTURE_UNIT2));
        tu->SetTexture(t);
    }
    else
    {
        tu = dynamic_cast<TextureUnit*>(scenarioriver->GetRenderState(RS_TEXTURE_UNIT2));
        tu->SetTexture(_waterreflection);
    }

    tu = dynamic_cast<TextureUnit*>(scenarioriver->GetRenderState(RS_TEXTURE_UNIT3));
    tu->SetTexture(obdrawGetSurfaceTexture());

    tu = dynamic_cast<TextureUnit*>(scenarioriver->GetRenderState(RS_TEXTURE_UNIT6));

    if(ShadowMap::GetInstance().Enabled() && ShadowMap::GetInstance().Valid())
    {
        tu->SetTexture(ShadowMap::GetInstance().GetShadowMapTexture());

        TextureMatrix* tm = dynamic_cast<TextureMatrix*>(scenarioriver->GetRenderState(RS_TEXTURE_MATRIX6));
        tm->SetMatrix(ShadowMap::GetInstance().GetInverseMVP());
    }
    else
    {
        static int white = -1;

        if(white == -1)
        {
            white = ldTextureLoad("white.dds", 0);
        }

        tu->SetTexture(white);
    }

    int matid;

    if(fxWeatherIsCloudy())
        matid = _watermaterialcloudy;
    else
        matid = _watermaterial;

    dynamic_cast<Material*>(scenarioriver->GetRenderState(RS_MATERIAL))->SetMaterialID(matid);

    scenarioriver->Apply();

    OBJECT* o;

    for(unsigned int i = 0; i < scenarioriverbin.size(); i++)
    {
        o = scenarioriverbin[i];
        watermindistance = std::min(watermindistance, o->distsqr);

        if(g_wantWaterReflections)
        {
            // Need the height of the water plane closest to the user for mirroring
            // the view matrix.
            if(reset)
            {
                reset = false;
                distance2 = o->distsqr > 0.0f ? o->distsqr : 0.0f;
                waterheight = o->pos.z;
            }
            else
            {
                float dist = Vec3f(o->bbox[0]).length2();

                if(dist < distance2)
                {
                    waterheight = o->pos.z;
                    distance2 = dist;
                }

                dist = Vec3f(o->bbox[1]).length2();

                if(dist < distance2)
                {
                    waterheight = o->pos.z;
                    distance2 = dist;
                }

                if(o->distsqr < distance2)
                {
                    waterheight = o->pos.z;
                    distance2 = o->distsqr;
                }
            }
        }

        oglSetWorldMatrix(&o->mat);

        if(o->ptverts)
        {
            piRenderScene(piPT_TRIANGLELIST, piVT_CVERTEX, o->ptverts, o->numtverts);
        }
        else
        {
            obdrawDrawMesh(o);
        }
    }
}
#else
/*--------------------------------------------------------------------------------------
Function: DrawScenarioRiverBin
Description: HACK
---------------------------------------------------------------------------------------*/
static void DrawScenarioRiverBin()
{
   if(scenarioriverbin.empty())
       return;

   oglSetRenderState(oglRENDERSTATE_LIGHTING, 0);

    waterdrawn = true;

    static int firsttime = frametime;

    float dtime = (float)(frametime - firsttime) * 0.001f;

    scenarioriver->GetProgram()->GetUniform("time")->Set(dtime);

    Texture2D* t = obdrawGetSurfaceTexture();

    if(g_wantWaterReflections)
    {
        Texture2D* t = obdrawGetWaterReflectionTexture();
        scenarioriver->AddTexture(2, t);
    }
    else
    {
        scenarioriver->AddTexture(2, _waterreflection);
    }

    scenarioriver->AddTexture(3, t);

    if(ShadowMap::GetInstance().Enabled() && ShadowMap::GetInstance().Valid())
    {
        ShadowMap::GetInstance().SetShadowStuff(*scenarioriver, 6);
    }
    else
    {
        static int white = -1;

        if(white == -1)
        {
            white = ldTextureLoad("white.dds", 0);
        }

        scenarioriver->AddTexture(6, white);
    }

    int matid;

    if(fxWeatherIsCloudy())
        matid = _watermaterialcloudy;
    else
        matid = _watermaterial;

    scenarioriver->SetMaterial(matid);
    scenarioriver->Apply();

    OBJECT* o;

    for(unsigned int i = 0; i < scenarioriverbin.size(); i++)
    {
        o = scenarioriverbin[i];
        watermindistance = std::min(watermindistance, o->distsqr);

        if(g_wantWaterReflections)
        {
            // Need the height of the water plane closest to the user for mirroring
            // the view matrix.
            if(reset)
            {
                reset = false;
                distance2 = o->distsqr > 0.0f ? o->distsqr : 0.0f;
                waterheight = o->pos.z;
            }
            else
            {
                float dist = Vec3f(o->bbox[0]).length2();

                if(dist < distance2)
                {
                    waterheight = o->pos.z;
                    distance2 = dist;
                }

                dist = Vec3f(o->bbox[1]).length2();

                if(dist < distance2)
                {
                    waterheight = o->pos.z;
                    distance2 = dist;
                }

                if(o->distsqr < distance2)
                {
                    waterheight = o->pos.z;
                    distance2 = o->distsqr;
                }
            }
        }

        oglSetWorldMatrix(&o->mat);

        if(o->ptverts)
        {
            piRenderScene(piPT_TRIANGLELIST, piVT_CVERTEX, o->ptverts, o->numtverts);
        }
        else
        {
            obdrawDrawMesh(o);
        }
    }

    piSetRenderState(piRENDERSTATE_LIGHTING, piLIGHTING_DEFAULT);
    oglSetWorldMatrix(&IdentityMatrix);

    TextureState::SetTextureUnit(0);
    oglSetTextureMatrix(&IdentityMatrix);

    if(ShadowMap::GetInstance().Enabled() && ShadowMap::GetInstance().Valid())
    {
        ShadowMap::GetInstance().UnApplyShadowMaps(6);
    }
    else
        ProgramState::Bind(0);
}
#endif

#if USE_STATESET
/*--------------------------------------------------------------------------------------
Function: DrawScenarioOceanBin
Description: HACK
---------------------------------------------------------------------------------------*/
static void DrawScenarioOceanBin()
{
   if(scenariooceanbin.empty())
       return;

    waterdrawn = true;

    static int firsttime = frametime;

    float dtime = (float)(frametime - firsttime) * 0.001f;

    glslProgram* pgm = dynamic_cast<Program*>(scenarioocean->GetRenderState(RS_GLSLPROGRAM))->GetProgram();
    pgm->GetUniform("time")->Set(dtime);

    dynamic_cast<TextureUnit*>(scenarioocean->GetRenderState(RS_TEXTURE_UNIT2))->SetTexture(obdrawGetSurfaceTexture());

    if(g_wantWaterReflections)
    {
        dynamic_cast<TextureUnit*>(scenarioocean->GetRenderState(RS_TEXTURE_UNIT1))->SetTexture(obdrawGetWaterReflectionTexture());
    }
    else
    {
        dynamic_cast<TextureUnit*>(scenarioocean->GetRenderState(RS_TEXTURE_UNIT1))->SetTexture(_waterreflection);
    }

    int matid;

    if(fxWeatherIsCloudy())
        matid = _watermaterialcloudy;
    else
        matid = _watermaterial;

    dynamic_cast<Material*>(scenarioocean->GetRenderState(RS_MATERIAL))->SetMaterialID(matid);

    scenarioocean->Apply();

    OBJECT* o;

    for(unsigned int i = 0; i < scenariooceanbin.size(); i++)
    {
        o = scenariooceanbin[i];
        watermindistance = std::min(watermindistance, o->distsqr);

        if(g_wantWaterReflections)
        {
            // Need the height of the water plane closest to the user for mirroring
            // the view matrix.
            if(reset)
            {
                reset = false;
                distance2 = o->distsqr > 0.0f ? o->distsqr : 0.0f;
                waterheight = o->pos.z;
            }
            else
            {
                float dist = Vec3f(o->bbox[0]).length2();

                if(dist < distance2)
                {
                    waterheight = o->pos.z;
                    distance2 = dist;
                }

                dist = Vec3f(o->bbox[1]).length2();

                if(dist < distance2)
                {
                    waterheight = o->pos.z;
                    distance2 = dist;
                }

                if(o->distsqr < distance2)
                {
                    waterheight = o->pos.z;
                    distance2 = o->distsqr;
                }
            }
        }

        oglSetWorldMatrix(&o->mat);

        if(o->ptverts)
        {
            piRenderScene(piPT_TRIANGLELIST, piVT_CVERTEX, o->ptverts, o->numtverts);
        }
        else
        {
            obdrawDrawMesh(o);
        }
    }
}
#else
/*--------------------------------------------------------------------------------------
Function: DrawScenarioOceanBin
Description: HACK
---------------------------------------------------------------------------------------*/
static void DrawScenarioOceanBin()
{
   if(scenariooceanbin.empty())
       return;

   oglSetRenderState(oglRENDERSTATE_LIGHTING, 0);

    waterdrawn = true;

    static int firsttime = frametime;

    float dtime = (float)(frametime - firsttime) * 0.001f;

    scenarioocean->GetProgram()->GetUniform("time")->Set(dtime);

    Texture2D* t = obdrawGetSurfaceTexture();

    if(g_wantWaterReflections)
    {
        Texture2D* t = obdrawGetWaterReflectionTexture();
        scenarioocean->AddTexture(1, t);
    }
    else
    {
        scenarioocean->AddTexture(1, _waterreflection);
    }

    scenarioocean->AddTexture(2, t);

    int matid;

    if(fxWeatherIsCloudy())
        matid = _watermaterialcloudy;
    else
        matid = _watermaterial;

    scenarioocean->SetMaterial(matid);
    scenarioocean->Apply();

    OBJECT* o;

    for(unsigned int i = 0; i < scenariooceanbin.size(); i++)
    {
        o = scenariooceanbin[i];

        if(g_wantWaterReflections)
        {
            // Need the height of the water plane closest to the user for mirroring
            // the view matrix.
            if(reset)
            {
                reset = false;
                distance2 = o->distsqr > 0.0f ? o->distsqr : 0.0f;
                waterheight = o->pos.z;
            }
            else
            {
                float dist = Vec3f(o->bbox[0]).length2();

                if(dist < distance2)
                {
                    waterheight = o->pos.z;
                    distance2 = dist;
                }

                dist = Vec3f(o->bbox[1]).length2();

                if(dist < distance2)
                {
                    waterheight = o->pos.z;
                    distance2 = dist;
                }

                if(o->distsqr < distance2)
                {
                    waterheight = o->pos.z;
                    distance2 = o->distsqr;
                }
            }
        }

        oglSetWorldMatrix(&o->mat);

        if(o->ptverts)
        {
            piRenderScene(piPT_TRIANGLELIST, piVT_CVERTEX, o->ptverts, o->numtverts);
        }
        else
        {
            obdrawDrawMesh(o);
        }
    }

    piSetRenderState(piRENDERSTATE_LIGHTING, piLIGHTING_DEFAULT);
    oglSetWorldMatrix(&IdentityMatrix);

    TextureState::SetTextureUnit(0);
    oglSetTextureMatrix(&IdentityMatrix);

    ProgramState::Bind(0);
}
#endif

/*--------------------------------------------------------------------------------------
Function: CoastDrawCallback
Description:
---------------------------------------------------------------------------------------*/
static bool CoastDrawCallback(OBJECT* o)
{
    if(ogl::UseDeprecatedPath() && !gWaterShadersEnabled)
    {
        waterbin.push_back(o);
        return true;
    }

    if(g_wantWaterReflections)
    {
        // Need the height of the water plane closest to the user for mirroring
        // the view matrix.
        if(reset)
        {
            reset = false;
            distance2 = o->distsqr > 0.0f ? o->distsqr : 0.0f;
            waterheight = o->pos.z;
        }
        else
        {
            float dist = Vec3f(o->bbox[0]).length2();

            if(dist < distance2)
            {
                waterheight = o->pos.z;
                distance2 = dist;
            }

            dist = Vec3f(o->bbox[1]).length2();

            if(dist < distance2)
            {
                waterheight = o->pos.z;
                distance2 = dist;
            }

            if(o->distsqr < distance2)
            {
                waterheight = o->pos.z;
                distance2 = o->distsqr;
            }
        }
    }

    waterbin.push_back(o);

    // Have to draw now to avoid sorting errors
    if(o->flag2 & OBJECT_HASALPHA)
    {
        if(ogl::UseDeprecatedPath())
            glslWaterDrawBinDeprecated();
        else
            glslWaterDrawBin();
    }

    return true;
};

/*--------------------------------------------------------------------------------------
Function: RiverDrawCallback
Description:
---------------------------------------------------------------------------------------*/
static bool RiverDrawCallback(OBJECT* o)
{
    if(!gWaterShadersEnabled)
    {
        waterbin.push_back(o);
        return true;
    }

    if(g_wantWaterReflections)
    {
        // Need the height of the water plane closest to the user for mirroring
        // the view matrix.
        if(reset)
        {
            reset = false;
            distance2 = o->distsqr > 0.0f ? o->distsqr : 0.0f;
            waterheight = o->pos.z;
        }
        else
        {
            float dist = Vec3f(o->bbox[0]).length2();

            if(dist < distance2)
            {
                waterheight = o->pos.z;
                distance2 = dist;
            }

            dist = Vec3f(o->bbox[1]).length2();

            if(dist < distance2)
            {
                waterheight = o->pos.z;
                distance2 = dist;
            }

            if(o->distsqr < distance2)
            {
                waterheight = o->pos.z;
                distance2 = o->distsqr;
            }
        }
    }

    waterbin.push_back(o);

    return true;
};

#if USE_STATESET
/*--------------------------------------------------------------------------------------
Function: InitMaterial
Description:
---------------------------------------------------------------------------------------*/
static void InitMaterial()
{
    h2omaterial = new RenderStateSet;
    h2omaterial->SetRenderState(new TextureUnit(0, _watertex));
    h2omaterial->SetRenderState(new TextureUnit(1, _waternormal));
    h2omaterial->SetRenderState(new TextureUnit(2));
    h2omaterial->SetRenderState(new TextureUnit(3));

    oglTextureObject* tbo = oglTextureObject::GetTextureObject("dynamicglobal");
    h2omaterial->SetRenderState(new TextureUnit(4, tbo->GetTextureObjectName()));

    h2omaterial->SetRenderState(new Program("water"));
}
#else
/*--------------------------------------------------------------------------------------
Function: InitMaterial
Description:
---------------------------------------------------------------------------------------*/
static void InitMaterial()
{
    h2omaterial = new oglMaterial;

    h2omaterial->AddTexture(0, _watertex);
    h2omaterial->AddTexture(1, _waternormal);

    oglTextureObject* tbo = oglTextureObject::GetTextureObject("dynamicglobal");
    h2omaterial->AddTexture(4, GL_TEXTURE_BUFFER, tbo->GetTextureObjectName());

    h2omaterial->SetShader("water");
}
#endif

/*--------------------------------------------------------------------------------------
Function: glslInitWaterTexture
Description:
---------------------------------------------------------------------------------------*/
void glslInitWaterTexture()
{
    if(gCustomTerrain)
    {
        _watertex = ldTextureLoad(SCEN_MGR->MapPath("water1.dds"), 0);
        _waternormal = ldTextureLoad(SCEN_MGR->MapPath("water1_normal.dds"), LDPIX_FLIP);
    }
    else
    {
	    _watertex = ldTextureLoad("terrain/to/water1.dds", 0);
        _waternormal = ldTextureLoad("terrain/to/water/water1_normal.dds", LDPIX_FLIP);
    }
}

#if USE_STATESET
/*--------------------------------------------------------------------------------------
Function: glslWaterInit
Description:
---------------------------------------------------------------------------------------*/
bool glslWaterInit()
{
    glslInitWaterTexture();

    // Temporary until redo of FBO's
    _waterreflection = oglCreateRawTexture(-1, 0, 256, 256, 24, 4, CRT_FLAG_NONE);

    if(!oglIsTexture(_waterreflection, 0))
    {
        return false;
    }

    TEXTURE* t = oglGetTextureData(_waterreflection);

    if(t)
    {
        for(int i = 0; i < t->miplevels; i++)
        {
            ldtexFillTexture(_waterreflection, i, piRGBA_MAKE(74, 89, 82, 255));
        }
    }

    piMATERIAL material;
    memset(&material, 0, sizeof(piMATERIAL));

    material.diffuse.set(1, 1, 1, 1);
    material.ambient.set(0.05f, 0.05f, 0.05f, 1.0f);
    material.specular.set(1, 1, 1, 1);
    material.emissive.set(0, 0, 0, 1);
    material.power = 64.0f;

    _watermaterial = oglCreateMaterial((void*)&material);

    memset(&material, 0, sizeof(piMATERIAL));

    material.diffuse.set(1, 1, 1, 1);
    material.ambient.set(0.05f, 0.05f, 0.05f, 1.0f);
    material.specular.set(0.0f, 0.0f, 0.0f, 1);
    material.emissive.set(0, 0, 0, 1);
    material.power = 64.0f;

    _watermaterialcloudy = oglCreateMaterial((void*)&material);

    if(ogl::UseDeprecatedPath())
    {
        h2omaterial = new RenderStateSet;

        h2omaterial->SetRenderState(new TextureUnit(0, _watertex));
        h2omaterial->SetRenderState(new TextureUnit(1, _waternormal));
        h2omaterial->SetRenderState(new TextureUnit(2));
        h2omaterial->SetRenderState(new TextureUnit(3));
        h2omaterial->SetRenderState(new Material(_watermaterial));

        h2omaterial->SetRenderState(new Program("ocean"));
        h2omaterial->SetMode(GL_LIGHTING, 0);
        h2omaterial->SetMode(GL_BLEND, 0);
        h2omaterial->SetMode(GL_ALPHA_TEST, 0);

        h2oshadowmaterial = new RenderStateSet(*h2omaterial);
        h2oshadowmaterial->SetRenderState(new Program("oceanshadow"));
        h2oshadowmaterial->SetRenderState(new TextureUnit(4));

        glslProgram* pgm = dynamic_cast<Program*>(h2oshadowmaterial->GetRenderState(RS_GLSLPROGRAM))->GetProgram();

        if(pgm)
            pgm->GetUniform("scale")->Set(ShadowMap::GetInstance().GetScale());
    }
    else
    {
        InitMaterial();
    }

    glslProgram* pgm = dynamic_cast<Program*>(h2omaterial->GetRenderState(RS_GLSLPROGRAM))->GetProgram();

    if(pgm)
        pgm->GetUniform("alpha")->Set(1.0f);

    _initfailed = false;

    return true;
}
#else
/*--------------------------------------------------------------------------------------
Function: glslWaterInit
Description:
---------------------------------------------------------------------------------------*/
bool glslWaterInit()
{
    glslInitWaterTexture();

    // Temporary until redo of FBO's
    _waterreflection = oglCreateRawTexture(-1, 0, 256, 256, 24, 4, CRT_FLAG_NONE);

    if(!oglIsTexture(_waterreflection, 0))
    {
        return false;
    }

    TEXTURE* t = oglGetTextureData(_waterreflection);

    if(t)
    {
        for(int i = 0; i < t->miplevels; i++)
        {
            ldtexFillTexture(_waterreflection, i, piRGBA_MAKE(74, 89, 82, 255));
        }
    }

    piMATERIAL material;
    memset(&material, 0, sizeof(piMATERIAL));

    material.diffuse.set(1, 1, 1, 1);
    material.ambient.set(0.05f, 0.05f, 0.05f, 1.0f);
    material.specular.set(1, 1, 1, 1);
    material.emissive.set(0, 0, 0, 1);
    material.power = 64.0f;

    _watermaterial = oglCreateMaterial((void*)&material);

    memset(&material, 0, sizeof(piMATERIAL));

    material.diffuse.set(1, 1, 1, 1);
    material.ambient.set(0.05f, 0.05f, 0.05f, 1.0f);
    material.specular.set(0.0f, 0.0f, 0.0f, 1);
    material.emissive.set(0, 0, 0, 1);
    material.power = 64.0f;

    _watermaterialcloudy = oglCreateMaterial((void*)&material);

    if(ogl::UseDeprecatedPath())
    {
        h2omaterial = new oglMaterial;

        h2omaterial->AddTexture(0, _watertex);
        h2omaterial->AddTexture(1, _waternormal);

        h2omaterial->SetShader("ocean");

        h2oshadowmaterial = new oglMaterial(*h2omaterial);
        h2oshadowmaterial->SetShader("oceanshadow");
        h2oshadowmaterial->GetProgram()->GetUniform("scale")->Set(ShadowMap::GetInstance().GetScale());
    }
    else
    {
        InitMaterial();
    }

    h2omaterial->GetProgram()->GetUniform("alpha")->Set(1.0f);

    _initfailed = false;

    return true;
}
#endif

/*--------------------------------------------------------------------------------------
Function: glslOceanSetup
Description:
---------------------------------------------------------------------------------------*/
void glslOceanSetup(OBJECT* o)
{
    o->drawcallback = OceanDrawCallback;
}

/*--------------------------------------------------------------------------------------
Function: glslCoastSetup
Description:
---------------------------------------------------------------------------------------*/
void glslCoastSetup(OBJECT* o)
{
    o->drawcallback = CoastDrawCallback;
}

#if USE_STATESET
/*--------------------------------------------------------------------------------------
Function: glslScenarioCoastSetup
Description:
---------------------------------------------------------------------------------------*/
void glslScenarioCoastSetup(OBJECT* o, const char* shader)
{
    int w = ldTextureLoad(SCEN_MGR->MapPath("water1.dds"), 0);

    if(!oglIsTexture(w, 0))
        w = ldTextureLoad("terrain/to/water1.dds", 0);

    int wn = ldTextureLoad(SCEN_MGR->MapPath("water1_normal.dds"), 0);

    if(!oglIsTexture(wn, 0))
        wn = ldTextureLoad("terrain/to/water/water1_normal.dds", 0);

    int wh = ldTextureLoad(SCEN_MGR->MapPath("waterheight.dds"), 0);

    if(!oglIsTexture(wh, 0))
    {
        Write_Debug("Coast scenarios need waterheight.dds");
    }

    scenariocoast = new RenderStateSet;

    scenariocoast->SetRenderState(new Program(shader));
    scenariocoast->SetRenderState(new TextureUnit(0, PostRenderFilter::GetInstance().getscenetexture()));
    scenariocoast->SetRenderState(new TextureUnit(1, wn));
    scenariocoast->SetRenderState(new TextureUnit(2));
    scenariocoast->SetRenderState(new TextureUnit(3));
    scenariocoast->SetRenderState(new TextureUnit(4, wh));
    scenariocoast->SetRenderState(new TextureUnit(5, w));
    scenariocoast->SetRenderState(new Material);
    scenariocoast->SetMode(GL_LIGHTING, 0);

    o->drawcallback = ScenarioCoastDrawCallback;
}
#else
/*--------------------------------------------------------------------------------------
Function: glslScenarioCoastSetup
Description:
---------------------------------------------------------------------------------------*/
void glslScenarioCoastSetup(OBJECT* o, const char* shader)
{
    int wn = ldTextureLoad(SCEN_MGR->MapPath("water1_normal.dds"), 0);

    if(!oglIsTexture(wn, 0))
        wn = ldTextureLoad("terrain/to/water/water1_normal.dds", 0);

    int wh = ldTextureLoad(SCEN_MGR->MapPath("waterheight.dds"), 0);

    if(!oglIsTexture(wh, 0))
    {
        Write_Debug("Coast scenarios need waterheight.dds");
    }

    scenariocoast = new oglMaterial;
    scenariocoast->SetShader(shader);

    scenariocoast->GetProgram()->GetUniform("scale")->Set(ShadowMap::GetInstance().GetScale());

    scenariocoast->AddTexture(0, PostRenderFilter::GetInstance().getscenetexture()); // refr
    scenariocoast->AddTexture(1, wn); //src2
    scenariocoast->AddTexture(2, -1); // refl
    scenariocoast->AddTexture(3, -1); // surf
    scenariocoast->AddTexture(4, wh); // ref
    scenariocoast->AddTexture(5, -1); // shadow

    o->drawcallback = ScenarioCoastDrawCallback;
}
#endif

#if USE_STATESET
/*--------------------------------------------------------------------------------------
Function: glslScenarioRiverSetup
Description:
---------------------------------------------------------------------------------------*/
void glslScenarioRiverSetup(OBJECT* o, const char* shader)
{
    int w = ldTextureLoad(SCEN_MGR->MapPath("water1.dds"), 0);

    if(!oglIsTexture(w, 0))
        w = ldTextureLoad("terrain/to/water1.dds", 0);

    int wn = ldTextureLoad(SCEN_MGR->MapPath("water1_normal.dds"), 0);

    if(!oglIsTexture(wn, 0))
        wn = ldTextureLoad("terrain/to/water/water1_normal.dds", 0);

    int wh = ldTextureLoad(SCEN_MGR->MapPath("waterheight.dds"), 0);

    if(!oglIsTexture(wh, 0))
    {
        Write_Debug("River scenarios need waterheight.dds");
    }

    scenarioriver = new RenderStateSet;
    scenarioriver->SetRenderState(new Program(shader));

    dynamic_cast<Program*>(scenarioriver->GetRenderState(RS_GLSLPROGRAM))->GetProgram()->GetUniform("scale")->Set(ShadowMap::GetInstance().GetScale());

    scenarioriver->SetRenderState(new TextureUnit(0, w));
    scenarioriver->SetRenderState(new TextureUnit(1, wn));
    scenarioriver->SetRenderState(new TextureUnit(2));      // reflection
    scenarioriver->SetRenderState(new TextureUnit(3));      // surface effects
    scenarioriver->SetRenderState(new TextureUnit(4, wh));  // height map
    scenarioriver->SetRenderState(new TextureUnit(5, PostRenderFilter::GetInstance().getscenetexture()));
    scenarioriver->SetRenderState(new TextureUnit(6));      // shadow
    scenarioriver->SetRenderState(new Material);
    scenarioriver->SetRenderState(new TextureMatrix(6));

    scenarioriver->SetMode(GL_LIGHTING, 0);

    o->drawcallback = ScenarioRiverDrawCallback;
}
#else
/*--------------------------------------------------------------------------------------
Function: glslScenarioRiverSetup
Description:
---------------------------------------------------------------------------------------*/
void glslScenarioRiverSetup(OBJECT* o, const char* shader)
{
    int w = ldTextureLoad(SCEN_MGR->MapPath("water1.dds"), 0);

    if(!oglIsTexture(w, 0))
        w = ldTextureLoad("terrain/to/water1.dds", 0);

    int wn = ldTextureLoad(SCEN_MGR->MapPath("water1_normal.dds"), 0);

    if(!oglIsTexture(wn, 0))
        wn = ldTextureLoad("terrain/to/water/water1_normal.dds", 0);

    int wh = ldTextureLoad(SCEN_MGR->MapPath("waterheight.dds"), 0);

    if(!oglIsTexture(wh, 0))
    {
        Write_Debug("River scenarios need waterheight.dds");
    }

    scenarioriver = new oglMaterial;
    scenarioriver->SetShader(shader);

    scenarioriver->GetProgram()->GetUniform("scale")->Set(ShadowMap::GetInstance().GetScale());

    scenarioriver->AddTexture(0, w);
    scenarioriver->AddTexture(1, wn);
    scenarioriver->AddTexture(2, -1); // reflection
    scenarioriver->AddTexture(3, -1); // surface effects
    scenarioriver->AddTexture(4, wh); // height map
    scenarioriver->AddTexture(5, PostRenderFilter::GetInstance().getscenetexture());
    scenarioriver->AddTexture(6, -1); // shadow

    o->drawcallback = ScenarioRiverDrawCallback;
}
#endif

#if USE_STATESET
/*--------------------------------------------------------------------------------------
Function: glslScenarioOceanSetup
Description:
---------------------------------------------------------------------------------------*/
void glslScenarioOceanSetup(OBJECT* o, const char* shader)
{
    int wn = ldTextureLoad(SCEN_MGR->MapPath("water1_normal.dds"), 0);

    if(!oglIsTexture(wn, 0))
        wn = ldTextureLoad("terrain/to/water/water1_normal.dds", 0);

    scenarioocean = new RenderStateSet;
    scenarioocean->SetRenderState(new Program(shader));

    scenarioocean->SetRenderState(new TextureUnit(0, wn));
    scenarioocean->SetRenderState(new TextureUnit(1)); // reflection
    scenarioocean->SetRenderState(new TextureUnit(2)); // surface effects
    scenarioocean->SetRenderState(new Material);

    scenarioocean->SetMode(GL_LIGHTING, 0);

    o->drawcallback = ScenarioOceanDrawCallback;
}
#else
/*--------------------------------------------------------------------------------------
Function: glslScenarioOceanSetup
Description:
---------------------------------------------------------------------------------------*/
void glslScenarioOceanSetup(OBJECT* o, const char* shader)
{
    int wn = ldTextureLoad(SCEN_MGR->MapPath("water1_normal.dds"), 0);

    if(!oglIsTexture(wn, 0))
        wn = ldTextureLoad("terrain/to/water/water1_normal.dds", 0);

    scenarioocean = new oglMaterial;
    scenarioocean->SetShader(shader);

    scenarioocean->AddTexture(0, wn);
    scenarioocean->AddTexture(1, -1); // reflection
    scenarioocean->AddTexture(2, -1); // surface effects

    o->drawcallback = ScenarioOceanDrawCallback;
}
#endif

/*--------------------------------------------------------------------------------------
Function: glslRiverSetup
Description:
---------------------------------------------------------------------------------------*/
void glslRiverSetup(OBJECT* o)
{
    o->drawcallback = RiverDrawCallback;
}
