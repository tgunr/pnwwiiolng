#include <syscfg.h> // Path is project specific.
/*--------------------------------------------------------------------------------------
Module: glslSpecular.cpp
Description:
---------------------------------------------------------------------------------------*/
#include "gl/glew.h"
#include "shader/glslSpecular.h"
#include "shader/glsl.h"
#include "shader/glslProgram.h"
#include "include/vars.h"
#include "ld/ldtexture.h"
#include "ogl/oglMesh.h"
#include "ob/obdraw.h"
#include "ogl/oglGlobalStates.h"
#include "ogl/States.h"
#include "ogl/oglMaterial.h"
#include "math/math3d.h"
#include "math/mathnum.h"
#include "ogl/oglTextureObject.h"

static int defaultSpecMap = -1;
static std::vector<OBJECT*> specularbin;

#include "osgImport/ShadowMap.h"
#include "scen/ScenarioManager.h"

static ref_ptr<oglMaterial> shadowmaterial;

#define MAX_SPEC_PRIMITIVES   1000

/*--------------------------------------------------------------------------------------
Function: glslSpecularClearBin
Description:
---------------------------------------------------------------------------------------*/
void glslSpecularClearBin()
{
    specularbin.clear();
}

/*------------------------------------------------------------------------
Function: AddSpecularDrawInstance
Description:
-------------------------------------------------------------------------*/
static void AddSpecularDrawInstance(const OBJECT* o, bool push = false)
{
    logTimerBeginMacro();

	static std::vector<Vec4f> data;
    static ref_ptr<oglTextureObject> tbo;
    static int primcount = 0;

    if(!tbo.valid())
    {
        tbo = oglTextureObject::GetTextureObject("dynamicglobal");
    }

    if(o)
    {
        if(push)
        {
            // Time to push objects to card.
            if(primcount)
            {
                tbo->UpdateBuffer(data.size() * sizeof(Vec4f), &data[0], 0);

                o->material->Apply();

                if(primcount > 1)
                    obdrawDrawObjectInstanced(o, 0, primcount);
                else
                    obdrawDrawObject(o);
            }

            primcount = 0;

			data.clear();
        }
        else
        {
            Quat q = Matrixf(o->mat).getRotate();

            Vec4f p(o->mat._41, o->mat._42, o->mat._43, o->level * colorONEOVER255);

            data.push_back(q.asVec4f());
            data.push_back(p);

            primcount++;

            if(primcount >= MAX_SPEC_PRIMITIVES)
            {
                tbo->UpdateBuffer(data.size() * sizeof(Vec4f), &data[0], 0);

                o->material->Apply();

				obdrawDrawObjectInstanced(o, 0, primcount);

                primcount = 0;

				data.clear();
            }
        }
    }

    logTimerEndMacro();
}

#if USE_STATESET
/*--------------------------------------------------------------------------------------
Function: glslSpecularDrawBin
Description:
---------------------------------------------------------------------------------------*/
void glslSpecularDrawBin()
{
    OBJECT* o = nullptr;
    OBJECT* last = nullptr;

    if(specularbin.empty())
        return;

    const unsigned int size = specularbin.size();

    const RenderStateSet* old = nullptr;

    oglSetWorldMatrix(&IdentityMatrix);

    for(unsigned int i = 0; i < size; ++i)
    {
        o = specularbin[i];

        if(old != o->material)
        {
            if(last)
            {
                AddSpecularDrawInstance(last, true);
            }
        }

        old = o->material;
        last = o;

        AddSpecularDrawInstance(o);
    }

    AddSpecularDrawInstance(last, true);

    specularbin.clear();
}
#else
/*--------------------------------------------------------------------------------------
Function: glslSpecularDrawBin
Description:
---------------------------------------------------------------------------------------*/
void glslSpecularDrawBin()
{
    OBJECT* o = nullptr;
    OBJECT* last = nullptr;

    if(specularbin.empty())
        return;

    const unsigned int size = specularbin.size();

    const oglMaterial* old = nullptr;

    oglSetWorldMatrix(&IdentityMatrix);

    for(unsigned int i = 0; i < size; ++i)
    {
        o = specularbin[i];

        if(old != o->material)
        {
            if(last)
            {
                AddSpecularDrawInstance(last, true);
            }
        }

        old = o->material;
        last = o;

        AddSpecularDrawInstance(o);
    }

    AddSpecularDrawInstance(last, true);

    specularbin.clear();
}
#endif

#if USE_STATESET
/*--------------------------------------------------------------------------------------
Function: glslSpecularDrawBinDeprecated
Description:
---------------------------------------------------------------------------------------*/
void glslSpecularDrawBinDeprecated()
{
    OBJECT* o = nullptr;

    if(specularbin.empty())
        return;

    const unsigned int size = specularbin.size();
    const RenderStateSet* old = nullptr;

    if(gCustomTerrain)
    {
        for(size_t i = 0; i < size; i++)
        {
            o = specularbin[i];

            if(ShadowMap::GetInstance().Enabled())
            {
                TextureUnit* tu = dynamic_cast<TextureUnit*>(o->material->GetRenderState(RS_TEXTURE_UNIT2));
                TextureMatrix* tm = dynamic_cast<TextureMatrix*>(o->material->GetRenderState(RS_TEXTURE_MATRIX2));

                if(!tu)
                {
                    tu = new TextureUnit(2, ShadowMap::GetInstance().GetShadowMapTexture());
                    o->material->SetRenderState(tu);

                    tm = new TextureMatrix(2);
                    o->material->SetRenderState(tm);
                }
                else
                {
                    tu->SetEnabled(true);
                    tm->SetEnabled(true);
                }

                Program* pgm = dynamic_cast<Program*>(o->material->GetRenderState(RS_GLSLPROGRAM));
                pgm->SetShader("specularmapshadow");
            }
            else
            {
                TextureUnit* tu = dynamic_cast<TextureUnit*>(o->material->GetRenderState(RS_TEXTURE_UNIT2));
                TextureMatrix* tm = dynamic_cast<TextureMatrix*>(o->material->GetRenderState(RS_TEXTURE_MATRIX2));

                if(tu)
                {
                    tu->SetEnabled(false);
                    tm->SetEnabled(false);
                }

                Program* pgm = dynamic_cast<Program*>(o->material->GetRenderState(RS_GLSLPROGRAM));
                pgm->SetShader("specularmap");
            }

            if(old != o->material)
            {
                o->material->Apply();
            }

            old = o->material;

            oglSetWorldMatrix(&o->mat);
            obdrawDrawMesh(o);
        }
    }
    else
    {
        for(unsigned int i = 0; i < size; ++i)
        {
            o = specularbin[i];

            if(old != o->material)
            {
                o->material->Apply();
            }

            old = o->material;

            oglSetWorldMatrix(&o->mat);
            obdrawDrawMesh(o);
        }
    }

    oglSetWorldMatrix(&IdentityMatrix);

    specularbin.clear();
}
#else
/*--------------------------------------------------------------------------------------
Function: glslSpecularDrawBinDeprecated
Description:
---------------------------------------------------------------------------------------*/
void glslSpecularDrawBinDeprecated()
{
    OBJECT* o = nullptr;

    if(specularbin.empty())
        return;

    oglSetRenderState(oglRENDERSTATE_LIGHTING, 0);

    const unsigned int size = specularbin.size();
    const oglMaterial* old = nullptr;

    if(gCustomTerrain)
    {
        if(ShadowMap::GetInstance().Enabled())
        {
            ShadowMap::GetInstance().SetShadowStuff(*shadowmaterial, 2);
            shadowmaterial->Apply();
        }
        else
        {
            glslProgram* pgm = GLSL.getProgram("specularmap");

            pgm->Apply();
            pgm->ApplyUniformMap();
        }

        TextureState::Start();

        for(unsigned int i = 0; i < size; ++i)
        {
            o = specularbin[i];

            if(old != o->material)
            {
                o->material->ApplyMaterial();
                o->material->ApplyTextures();
            }

            old = o->material;

            oglSetWorldMatrix(&o->mat);
            obdrawDrawMesh(o);
        }
    }
    else
    {
        glslProgram* pgm = GLSL.getProgram("specularmap");

        pgm->Apply();
        pgm->ApplyUniformMap();

        TextureState::Start();

        for(unsigned int i = 0; i < size; ++i)
        {
            o = specularbin[i];

            if(old != o->material)
            {
                o->material->ApplyMaterial();
                o->material->ApplyTextures();
            }

            old = o->material;

            oglSetWorldMatrix(&o->mat);
            obdrawDrawMesh(o);
        }
    }

    oglSetRenderState(oglRENDERSTATE_LIGHTING, piLIGHTING_DEFAULT);
    oglSetWorldMatrix(&IdentityMatrix);

    ProgramState::Bind(0);
    TextureState::End();

    if(gCustomTerrain)
    {
        if(ShadowMap::GetInstance().Enabled())
        {
            ShadowMap::GetInstance().UnApplyShadowMaps(2);
        }
    }

    specularbin.clear();
}
#endif

/*--------------------------------------------------------------------------------------
Function: SpecularDrawCallback
Description:
---------------------------------------------------------------------------------------*/
static bool SpecularDrawCallback(OBJECT* o)
{
    if(o->flag2 & OBJECT_HASALPHA && !(o->flags & OBJECT_NOFILTER))
    {
        // Transparent bin
        oglSetWorldMatrix(&o->mat);
        o->material->Apply();

        obdrawDrawMesh(o);
    }
    else
    {
        // Opaque bin, group objects to save draw calls
        specularbin.push_back(o);
    }

    return true;
};

#if USE_STATESET
/*--------------------------------------------------------------------------------------
Function: SpecularDrawCallbackDeprecated
Description:
---------------------------------------------------------------------------------------*/
static bool SpecularDrawCallbackDeprecated(OBJECT* o)
{
    bool shadowpass = obdrawIsRenderPass(RENDER_FINAL_SHADOW);
    bool nospecular = o->flag3 & OBJECT_IS_INTERIOR ||
                      !WantSpecularMaps ||
                      !oglIsTexture(o->texid, 0);

    if(nospecular || shadowpass)
    {
        oglSetWorldMatrix(&o->mat);

        o->material->SetMode(GL_LIGHTING, 1);
        o->material->GetRenderState(RS_GLSLPROGRAM)->SetEnabled(false);
        o->material->GetRenderState(RS_TEXTURE_UNIT1)->SetEnabled(false);

        return false;
    }

    // Opaque bin, group objects to save draw calls
    o->material->SetMode(GL_LIGHTING, 0);
    o->material->GetRenderState(RS_GLSLPROGRAM)->SetEnabled(true);
    o->material->GetRenderState(RS_TEXTURE_UNIT1)->SetEnabled(true);

    if(o->flag2 & OBJECT_HASALPHA && !(o->flags & OBJECT_NOFILTER))
    {
        // Transparent bin, need to draw as is
    }
    else
    {
        specularbin.push_back(o);
        return true;
    }

    return false;
};
#else
/*--------------------------------------------------------------------------------------
Function: SpecularDrawCallbackDeprecated
Description:
---------------------------------------------------------------------------------------*/
static bool SpecularDrawCallbackDeprecated(OBJECT* o)
{
    bool shadowpass = obdrawIsRenderPass(RENDER_FINAL_SHADOW);
    bool nospecular = o->flag3 & OBJECT_IS_INTERIOR ||
                      !WantSpecularMaps ||
                      !oglIsTexture(o->texid, 0);

    if(nospecular || shadowpass)
    {
        oglSetRenderState(oglRENDERSTATE_LIGHTING, piLIGHTING_DEFAULT);

        if(!shadowpass)
        {
            o->material->ApplyMaterial();

            TextureState::Start();
            o->material->ApplyTexture(0);
            TextureState::End();
        }

        oglSetWorldMatrix(&o->mat);

        obdrawDrawMesh(o);

        return true;
    }

    if(o->flag2 & OBJECT_HASALPHA && !(o->flags & OBJECT_NOFILTER))
    {
        // Transparent bin, need to draw as is
    }
    else
    {
        // Opaque bin, group objects to save draw calls
        specularbin.push_back(o);
        return true;
    }

    oglSetRenderState(oglRENDERSTATE_LIGHTING, 0);

    o->material->Apply();
    oglSetWorldMatrix(&o->mat);

    obdrawDrawMesh(o);

    ProgramState::Bind(0);

    oglSetRenderState(oglRENDERSTATE_LIGHTING, piLIGHTING_DEFAULT);

    return true;
};
#endif

/*--------------------------------------------------------------------------------------
Function: glslSpecularInit
Description:
---------------------------------------------------------------------------------------*/
bool glslSpecularInit()
{
    if(ogl::UseDeprecatedPath())
    {
        shadowmaterial = new oglMaterial;
        shadowmaterial->SetShader("specularmapshadow");
        shadowmaterial->GetProgram()->GetUniform("scale")->Set(ShadowMap::GetInstance().GetScale());
    }

    return true;
}

#if USE_STATESET
/*--------------------------------------------------------------------------------------
Function: glslSpecularSetup
Description:
---------------------------------------------------------------------------------------*/
void glslSpecularSetup(OBJECT* o)
{
    if(piIsMaterialInterior(o->matid))
    {
        o->flag3 &= ~OBJECT_SPECULARMAP;

        o->material->RemoveRenderState(RS_GLSLPROGRAM);
        o->material->RemoveRenderState(RS_TEXTURE_UNIT1);

        return;
    }

    if(ogl::UseDeprecatedPath())
        o->drawcallback = SpecularDrawCallbackDeprecated;
    else
        o->drawcallback = SpecularDrawCallback;
}
#else
/*--------------------------------------------------------------------------------------
Function: glslSpecularSetup
Description:
---------------------------------------------------------------------------------------*/
void glslSpecularSetup(OBJECT* o)
{
    if(!(o->flag3 & OBJECT_SPECULARMAP))
    {
        return;
    }

    if(!oglIsTexture(o->texid, 0))
    {
        return;
    }

    if(piIsMaterialInterior(o->matid))
    {
        return;
    }

    glslProgram* pgm = GLSL.getProgram("specularmap");

    if(pgm)
    {
        if(ogl::UseDeprecatedPath())
            o->drawcallback = SpecularDrawCallbackDeprecated;
        else
            o->drawcallback = SpecularDrawCallback;
    }
    else
    {
        o->flag3 &= ~OBJECT_SPECULARMAP;
    }
}
#endif