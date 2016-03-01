/*--------------------------------------------------------------------------------------
Module: glslNormal.cpp
Description:
---------------------------------------------------------------------------------------*/
#include "gl/glew.h"
#include "shader/glslNormal.h"
#include "shader/glsl.h"
#include "shader/glslProgram.h"
#include "ogl/oglGlobalStates.h"
#include "ogl/oglMesh.h"
#include "include/vars.h"
#include "ld/ldtexture.h"
#include "ob/obdraw.h"
#include "ob/obMgr.h"
#include "ogl/States.h"
#include "ogl/oglMaterial.h"
#include "ui/uiDotCommands.h"
#include "math/math3d.h"
#include "math/mathnum.h"
#include "gr2/gr2layer.h"
#include "ogl/oglTextureObject.h"
#include "gr2/gr2LayerTypes.h"

static std::vector<OBJECT*> normalbin;

#include "osgImport/ShadowMap.h"
#include "scen/ScenarioManager.h"
static ref_ptr<oglMaterial> shadowmaterial;

#define MAX_NORMAL_PRIMITIVES   1000

/*--------------------------------------------------------------------------------------
Function: drawFPS
Description:
---------------------------------------------------------------------------------------*/
static inline void drawFPS(gr2Mesh* mesh, int numverts, short* indices, int count)
{ 
    oglRenderIndexScene(oglPT_TRIANGLELIST,
                        oglVT_TVERTEX,
                        (void*)mesh->PNGT(),
                        numverts,
                        indices,
                        count);
}

/*--------------------------------------------------------------------------------------
Function: glslNormalClearBin
Description:
---------------------------------------------------------------------------------------*/
void glslNormalClearBin()
{
    normalbin.clear();
}

/*------------------------------------------------------------------------
Function: AddNormalDrawInstance
Description:
-------------------------------------------------------------------------*/
static void AddNormalDrawInstance(const OBJECT* o, bool push = false)
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

            if(primcount >= MAX_NORMAL_PRIMITIVES)
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
Function: glslNormalDrawBin
Description:
---------------------------------------------------------------------------------------*/
void glslNormalDrawBin()
{
}
#else
/*--------------------------------------------------------------------------------------
Function: glslNormalDrawBin
Description:
---------------------------------------------------------------------------------------*/
void glslNormalDrawBin()
{
    OBJECT* o = nullptr;
    const OBJECT* last = nullptr;

    if(normalbin.empty())
        return;

    const unsigned int size = normalbin.size();

    const oglMaterial* old = nullptr;

#if DEBUG_WW2
    int same = 0;
#endif

    oglSetWorldMatrix(&IdentityMatrix);

    for(unsigned int i = 0; i < size; ++i)
    {
        o = normalbin[i];

        if(old != o->material)
        {
            if(last)
            {
                AddNormalDrawInstance(last, true);
            }
        }
#if DEBUG_WW2
        else
        {
            same++;
        }
#endif

        old = o->material;
        last = o;

        AddNormalDrawInstance(o);
    }

    AddNormalDrawInstance(last, true);
    normalbin.clear();
}
#endif

#if USE_STATESET
/*--------------------------------------------------------------------------------------
Function: glslNormalDrawBinDeprecated
Description:
---------------------------------------------------------------------------------------*/
void glslNormalDrawBinDeprecated()
{
    OBJECT* o = nullptr;

    if(normalbin.empty())
        return;

    const unsigned int size = normalbin.size();

    const RenderStateSet* old = nullptr;

    if(gCustomTerrain)
    {
        if(ShadowMap::GetInstance().Enabled())
        {
            Texture2D* tex = ShadowMap::GetInstance().GetShadowMapTexture();
            const Matrixf& mvp = ShadowMap::GetInstance().GetInverseMVP();
        }
        else
        {
        }

        for(unsigned int i = 0; i < size; ++i)
        {
            o = normalbin[i];

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
            o = normalbin[i];

            if(old != o->material)
            {
                o->material->Apply();
            }

            old = o->material;

            oglSetWorldMatrix(&o->mat);
            obdrawDrawMesh(o);
        }
    }

    normalbin.clear();
}
#else
/*--------------------------------------------------------------------------------------
Function: glslNormalDrawBinDeprecated
Description:
---------------------------------------------------------------------------------------*/
void glslNormalDrawBinDeprecated()
{
    OBJECT* o = nullptr;

    if(normalbin.empty())
        return;

    oglSetRenderState(oglRENDERSTATE_LIGHTING, 0);

    const unsigned int size = normalbin.size();

    const oglMaterial* old = nullptr;

    if(gCustomTerrain)
    {
        if(ShadowMap::GetInstance().Enabled())
        {
            ShadowMap::GetInstance().SetShadowStuff(*shadowmaterial, 3);
            shadowmaterial->Apply();
        }
        else
        {
            glslProgram* pgm = GLSL.getProgram("normalmap");

            pgm->Apply();
            pgm->ApplyUniformMap();
        }

        TextureState::Start();

        for(unsigned int i = 0; i < size; ++i)
        {
            o = normalbin[i];

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
        glslProgram* pgm = GLSL.getProgram("normalmap");

        pgm->Apply();
        pgm->ApplyUniformMap();

        TextureState::Start();

        for(unsigned int i = 0; i < size; ++i)
        {
            o = normalbin[i];

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
            ShadowMap::GetInstance().UnApplyShadowMaps(3);
        }
    }

    normalbin.clear();
}
#endif

#if USE_STATESET
/*--------------------------------------------------------------------------------------
Function: NormalDrawCallbackDeprecated
Description:
---------------------------------------------------------------------------------------*/
static bool NormalDrawCallbackDeprecated(OBJECT* o)
{
    bool shadowpass = obdrawIsRenderPass(RENDER_FINAL_SHADOW);
    bool nonormals = o->flag3 & OBJECT_IS_INTERIOR ||
                     !g_wantNormalMaps ||
                     !oglIsTexture(o->texid, 0) ||
                     !o->meshContext;

    if(nonormals)
    {
        if(o->flag2 & OBJECT_USEBUFFERS)
        {
            if(o->meshContext && o->meshContext->IsBuffered())
            {
                UINT32 type = o->meshContext->GetVertexType();
                UINT32 flags = (ogl::VertexAttributes::ATTRIB6 |
                                ogl::VertexAttributes::ATTRIB7 |
                                ogl::VertexAttributes::ATTRIB15);

                // Not normal mapped...ignore these
                if(type & flags)
                {
                    type &= ~flags;
                    o->meshContext->SetVertexType(type);
                }
            }
        }

        o->material->SetMode(GL_LIGHTING, 1);

        o->material->GetRenderState(RS_GLSLPROGRAM)->SetEnabled(false);
        o->material->GetRenderState(RS_TEXTURE_UNIT2)->SetEnabled(false);
        o->material->GetRenderState(RS_TEXTURE_UNIT1)->SetEnabled(false);

        return false;
    }
    else
    {
        if(o->flag2 & OBJECT_USEBUFFERS)
        {
            if(o->meshContext && o->meshContext->IsBuffered())
            {
                UINT32 type = o->meshContext->GetVertexType();
                UINT32 flags = (ogl::VertexAttributes::ATTRIB6 |
                                ogl::VertexAttributes::ATTRIB7 |
                                ogl::VertexAttributes::ATTRIB15);

                // Normal mapped...add these
                if(!(type & flags))
                {
                    type |= flags;
                    o->meshContext->SetVertexType(type);
                }
            }
        }
    }

    if(!nonormals)
    {
        o->material->GetRenderState(RS_GLSLPROGRAM)->SetEnabled(true);
        o->material->GetRenderState(RS_TEXTURE_UNIT2)->SetEnabled(true);
        o->material->GetRenderState(RS_TEXTURE_UNIT1)->SetEnabled(true);

        o->material->SetMode(GL_LIGHTING, 0);
    }

    if(o->flag2 & OBJECT_HASALPHA && !(o->flags & OBJECT_NOFILTER))
    {
        // Transparent bin, need to draw as is
    }
    else
    {
        // Opaque bin, group objects to save draw calls
        normalbin.push_back(o);
        return true;
    }

    return false;
};
#else
/*--------------------------------------------------------------------------------------
Function: NormalDrawCallbackDeprecated
Description:
---------------------------------------------------------------------------------------*/
static bool NormalDrawCallbackDeprecated(OBJECT* o)
{
    bool shadowpass = obdrawIsRenderPass(RENDER_FINAL_SHADOW);
    bool nonormals = o->flag3 & OBJECT_IS_INTERIOR ||
                     !g_wantNormalMaps ||
                     !oglIsTexture(o->texid, 0) ||
                     !o->meshContext;

    if(nonormals || shadowpass)
    {
        if(o->flag2 & OBJECT_USEBUFFERS)
        {
            if(o->meshContext && o->meshContext->IsBuffered())
            {
                UINT32 type = o->meshContext->GetVertexType();
                UINT32 flags = (ogl::VertexAttributes::ATTRIB6 |
                                ogl::VertexAttributes::ATTRIB7 |
                                ogl::VertexAttributes::ATTRIB15);

                // Not normal mapped...ignore these
                if(type & flags)
                {
                    type &= ~flags;
                    o->meshContext->SetVertexType(type);
                }
            }
        }

        oglSetRenderState(oglRENDERSTATE_LIGHTING, piLIGHTING_DEFAULT);

        oglSetWorldMatrix(&o->mat);

        if(!shadowpass)
        {
            o->material->ApplyMaterial();
            o->material->ApplyTexture(0);

            ProgramState::Bind(0);
        }

        obdrawDrawMesh(o);

        return true;
    }
    else
    {
        if(o->flag2 & OBJECT_USEBUFFERS)
        {
            if(o->meshContext && o->meshContext->IsBuffered())
            {
                UINT32 type = o->meshContext->GetVertexType();
                UINT32 flags = (ogl::VertexAttributes::ATTRIB6 |
                                ogl::VertexAttributes::ATTRIB7 |
                                ogl::VertexAttributes::ATTRIB15);

                // Normal mapped...add these
                if(!(type & flags))
                {
                    type |= flags;
                    o->meshContext->SetVertexType(type);
                }
            }
        }
    }

    if(o->flag2 & OBJECT_HASALPHA && !(o->flags & OBJECT_NOFILTER))
    {
        // Transparent bin, need to draw as is
    }
    else
    {
        // Opaque bin, group objects to save draw calls
        normalbin.push_back(o);
        return true;
    }

    oglSetRenderState(oglRENDERSTATE_LIGHTING, 0);

    oglSetWorldMatrix(&o->mat);

    o->material->Apply();

    obdrawDrawMesh(o);

    ProgramState::Bind(0);

    oglSetRenderState(oglRENDERSTATE_LIGHTING, piLIGHTING_DEFAULT);

    return true;
};
#endif

/*--------------------------------------------------------------------------------------
Function: NormalDrawCallback
Description:
---------------------------------------------------------------------------------------*/
static bool NormalDrawCallback(OBJECT* o)
{
    if(o->flag2 & OBJECT_HASALPHA && !(o->flags & OBJECT_NOFILTER))
    {
        // Transparent bin, need to draw as is
    }
    else
    {
        // Opaque bin, group objects to save draw calls
        normalbin.push_back(o);
        return true;
    }

    oglSetWorldMatrix(&IdentityMatrix);

    AddNormalDrawInstance(o);
    AddNormalDrawInstance(o, true);

    return true;
};

#if USE_STATESET
#else
/*--------------------------------------------------------------------------------------
Function: FpsNormalDrawCallbackDeprecated
Description:
---------------------------------------------------------------------------------------*/
static bool FpsNormalDrawCallbackDeprecated(gr2Mesh* mesh, short* indices, int numverts, granny_tri_material_group* g)
{
    static ref_ptr<oglMaterial> fpsmaterial;

    if(!fpsmaterial.valid())
    {
        fpsmaterial = new oglMaterial;
        fpsmaterial->SetShader("fpsnormal");
    }

    int matid = -1;
    int texid = -1;

    if(piIsMaterialInterior(piGetActiveMaterial()))
    {
        fpsmaterial->SetMaterial(piGetActiveMaterial());
    }

    bool nonormals = piIsMaterialInterior(piGetActiveMaterial()) || !g_wantNormalMaps;

    fpsmaterial->EnableShader(!nonormals);

    if(g->MaterialIndex < (int)mesh->data->textureRefs.size())
    {
        texid = mesh->data->textureRefs.at(g->MaterialIndex);

        if(g->MaterialIndex < (int)mesh->data->materialRefs.size())
            matid = mesh->data->materialRefs.at(g->MaterialIndex);
    }

    // Uses forest material, already set
    if(!piIsMaterialInterior(piGetActiveMaterial()))
        fpsmaterial->SetMaterial(matid);

    if(nonormals)
    {
        fpsmaterial->TextureResize(1);
        fpsmaterial->AddTexture(0, texid);
        fpsmaterial->Apply();

        drawFPS(mesh, numverts, indices, g->TriCount * 3);

        return true;
    }

    oglSetRenderState(oglRENDERSTATE_LIGHTING, 0);

    fpsmaterial->AddTexture(0, texid);
    fpsmaterial->AddTexture(1, oglGetTextureData(texid)->normalMap);

    if(oglIsTexture(oglGetTextureData(texid)->specularMap, 0))
    {
        fpsmaterial->AddTexture(2, oglGetTextureData(texid)->specularMap);
    }
    else
    {
        fpsmaterial->AddTexture(2, oglGetNamedTexture("default_spec"));
    }

    ogl::SetTextureUnitCoordPointer(GL_TEXTURE1, 3, GL_FLOAT, sizeof(oglTVERTEX), &mesh->PNGT()->tx);

    fpsmaterial->Apply();

    drawFPS(mesh, numverts, indices, g->TriCount * 3);

    ProgramState::Bind(0);

    oglSetRenderState(oglRENDERSTATE_LIGHTING, piLIGHTING_DEFAULT);

    return true;
}
#endif

/*--------------------------------------------------------------------------------------
Function: glslNormalInit
Description:
---------------------------------------------------------------------------------------*/
bool glslNormalInit()
{
    if(ogl::UseDeprecatedPath())
    {
        // Only used for custom terrain
        shadowmaterial = new oglMaterial;
        shadowmaterial->SetShader("normalmapshadow");
		// the client will crash on the following line if it hasn't loaded the normalmapshadow
		// shader, which often happens if you haven't set your WorkingDir variable...
        shadowmaterial->GetProgram()->GetUniform("scale")->Set(ShadowMap::GetInstance().GetScale());
    }

    return true;
}

#if USE_STATESET
/*--------------------------------------------------------------------------------------
Function: glslNormalSetup
Description:
---------------------------------------------------------------------------------------*/
void glslNormalSetup(OBJECT* o)
{
    if(piIsMaterialInterior(o->matid))
    {
        o->flag2 &= ~OBJECT_NORMALMAP;
        o->material->RemoveRenderState(RS_GLSLPROGRAM);
        o->material->RemoveRenderState(RS_TEXTURE_UNIT2);
        o->material->RemoveRenderState(RS_TEXTURE_UNIT1);

        return;
    }

    if(ogl::UseDeprecatedPath())
        o->drawcallback = NormalDrawCallbackDeprecated;
    else
        o->drawcallback = NormalDrawCallback;
}
#else
/*--------------------------------------------------------------------------------------
Function: glslNormalSetup
Description:
---------------------------------------------------------------------------------------*/
void glslNormalSetup(OBJECT* o)
{
    if(!(o->flag2 & OBJECT_NORMALMAP))
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

    glslProgram* pgm = GLSL.getProgram("normalmap");

    if(pgm)
    {
        if(ogl::UseDeprecatedPath())
            o->drawcallback = NormalDrawCallbackDeprecated;
        else
            o->drawcallback = NormalDrawCallback;
    }
    else
    {
        o->flag2 &= ~OBJECT_NORMALMAP;
    }
}
#endif

#if USE_STATESET
#else
/*--------------------------------------------------------------------------------------
Function: glslNormalFpsSetup
Description:
---------------------------------------------------------------------------------------*/
void glslNormalFpsSetup(gr2MeshData* data)
{
    glslProgram* pgm = GLSL.getProgram("fpsnormal");

    if(pgm)
    {
        if(ogl::UseDeprecatedPath())
            data->drawcallback = FpsNormalDrawCallbackDeprecated;
     }
    else
    {
        data->drawcallback = nullptr;
    }
}
#endif