#include <syscfg.h> // Path is project specific.
/*-------------------------------------------------------------------------
Module: glslCloth.cpp
Description:
--------------------------------------------------------------------------*/
#include "shader/glsl.h"
#include "shader/glslCloth.h"
#include "math/Vec3f.h"
#include "ogrp/ogrptype.h"
#include "ogl/oglMesh.h"
#include "ob/obdraw.h"
#include "ogl/States.h"
#include "ogl/oglMaterial.h"

struct Flags
{
    Vec3f pos1;
    Vec3f pos2;
    Vec3f dir;
    Vec3f right;

    bool init;
    bool valid;

    int lastupdate;
    float flagangle;
};

static std::map<OBJECT*, Flags> _data;

static float _speed = 8.0f;
static int _material = -1;

/*-------------------------------------------------------------------------
Function: glslClothInit
Description:
--------------------------------------------------------------------------*/
bool glslClothInit()
{
    return true;
}

/*-------------------------------------------------------------------------
Function: InitFlags
Description: Can't do this until first update since the dofs are not loaded
when the setup is called.
--------------------------------------------------------------------------*/
void InitFlags(OBJECT* o, Flags& flags)
{
    flags.init = true;

    // If terrain object, get DOF parameters.
    if(o->flag2 & OBJECT_TERRAIN_OBJECT)
    {
        ogrpParseDOFParameters(o, nullptr, 0, 0);
    }

    OBJECT* dof1 = nullptr;
    OBJECT* dof2 = nullptr;

    obmainFindComment(&dof1, o, "origin1");
    obmainFindComment(&dof2, o, "origin2");

    if(!dof1 || !dof2)
    {
        return;
    }

    if(dof1)
    {
        const piMATRIX& m = dof1->dof->dofmatrix2;
        flags.pos1.set(m._41, m._42, m._43);
        flags.dir.set(m._21, m._22, m._23);
        flags.right.set(m._31, m._32, m._33);
    }

    if(dof2)
    {
        const piMATRIX& m = dof2->dof->dofmatrix2;
        flags.pos2.set(m._41, m._42, m._43);
    }

    flags.lastupdate = 0;
    flags.flagangle = 0;

    flags.valid = true;
}

/*-------------------------------------------------------------------------
Function: FlagUpdate
Description:
--------------------------------------------------------------------------*/
static void FlagUpdate(OBJECT* o, glslProgram* pgm, Flags& flag)
{
    float dTime = (frametime - flag.lastupdate) * 0.001f;

    flag.flagangle += _speed * dTime;
    flag.flagangle = fmod(flag.flagangle, 360.0f);

    Vec3f angle = Vec3f(flag.flagangle, flag.flagangle, flag.flagangle);

    bool reverse = false;

    if(angle * flag.dir > 0)
        reverse = true;

    pgm->GetUniform("currentAngle")->Set(reverse ? -flag.flagangle : flag.flagangle);

    flag.lastupdate = frametime;
}

/*--------------------------------------------------------------------------------------
Function: draw
Description:
---------------------------------------------------------------------------------------*/
static inline void draw(OBJECT* o)
{
    if(o->flag2 & OBJECT_USEBUFFERS)
    {
        if(o->meshContext && o->meshContext->IsBuffered())
        {
            o->meshContext->Draw();
		}
    }
    else
    {
        piDrawScene(piPT_TRIANGLELIST,
                    (float*)o->RenderPositions().ptr,
                    (float*)o->RenderNormals().ptr,
                    (float*)o->RenderTexCoords().ptr,
                    nullptr,
                    (unsigned short*)o->ptris,
                    sizeof(piUNIFIED_VERTEX),
                    o->numverts,
                    o->numtris * 3);
    }
}

/*-------------------------------------------------------------------------
Function: FlagKillCallback
Description:
--------------------------------------------------------------------------*/
static bool FlagKillCallback(OBJECT* o)
{
    std::map<OBJECT*, Flags>::iterator itr = _data.find(o);

    if(itr != _data.end())
    {
        _data.erase(itr);
    }

    return true;
}

/*-------------------------------------------------------------------------
Function: FlagDrawCallbackDeprecated
Description:
--------------------------------------------------------------------------*/
static bool FlagDrawCallbackDeprecated(OBJECT* o)
{
    if(obdrawIsRenderPass(RENDER_FINAL_SHADOW))
    {
        return true;
    }

    if(!oglIsMaterial(_material))
    {
        _material = oglFindMaterialName("cloth");

#if USE_STATESET
        Material* m = dynamic_cast<Material*>(o->material->GetRenderState(RS_MATERIAL));
        m->SetMaterialID(_material);
#else
        o->material->SetMaterial(_material);
#endif
    }

    std::map<OBJECT*, Flags>::iterator itr = _data.find(o);

    if(itr == _data.end())
    {
#if USE_STATESET
        Program* p = dynamic_cast<Program*>(o->material->GetRenderState(RS_GLSLPROGRAM));
        p->SetEnabled(false);
#else
        o->material->EnableShader(false);
#endif
        return false;
    }

    Flags& flag = itr->second;

    if(flag.init == false)
        InitFlags(o, flag);

    if(!flag.valid)
    {
#if USE_STATESET
        Program* p = dynamic_cast<Program*>(o->material->GetRenderState(RS_GLSLPROGRAM));
        p->SetEnabled(false);
#else
        o->material->EnableShader(false);
#endif
        return false;
    }

#if USE_STATESET
    Program* p = dynamic_cast<Program*>(o->material->GetRenderState(RS_GLSLPROGRAM));
    p->SetEnabled(true);
#else
    o->material->EnableShader(true);
#endif

#if USE_STATESET
    glslProgram* pgm = dynamic_cast<Program*>(o->material->GetRenderState(RS_GLSLPROGRAM))->GetProgram();
#else
    glslProgram* pgm = o->material->GetProgram();
#endif

    pgm->GetUniform("pos1")->Set(flag.pos1);
    pgm->GetUniform("pos2")->Set(flag.pos2);
    pgm->GetUniform("right")->Set(flag.right);

    FlagUpdate(o, pgm, flag);

#if USE_STATESET
#else
    oglPushRenderState(oglRENDERSTATE_LIGHTING);
    oglSetRenderState(oglRENDERSTATE_LIGHTING, 0);
#endif

    o->material->Apply();

    oglSetWorldMatrix(&o->mat);

    draw(o);

#if USE_STATESET
#else
    ProgramState::Bind(0);
    oglPopRenderState(oglRENDERSTATE_LIGHTING);
#endif

    return true;
}

/*-------------------------------------------------------------------------
Function: FlagDrawCallback
Description:
--------------------------------------------------------------------------*/
static bool FlagDrawCallback(OBJECT* o)
{
    std::map<OBJECT*, Flags>::iterator itr = _data.find(o);

    if(itr == _data.end())
    {
#if USE_STATESET
        dynamic_cast<Program*>(o->material->GetRenderState(RS_GLSLPROGRAM))->SetShader("drawtexture");
#else
        o->material->SetShader("drawtexture");
#endif
        return false;
    }

    Flags& flag = itr->second;

    if(flag.init == false)
        InitFlags(o, flag);

    if(!flag.valid)
    {
#if USE_STATESET
        dynamic_cast<Program*>(o->material->GetRenderState(RS_GLSLPROGRAM))->SetShader("drawtexture");
#else
        o->material->SetShader("drawtexture");
#endif
        return false;
    }

#if USE_STATESET
    dynamic_cast<Program*>(o->material->GetRenderState(RS_GLSLPROGRAM))->SetShader("flagprogram");
#else
    o->material->SetShader("flagprogram");
#endif

#if USE_STATESET
    glslProgram* pgm = dynamic_cast<Program*>(o->material->GetRenderState(RS_GLSLPROGRAM))->GetProgram();
#else
    glslProgram* pgm = o->material->GetProgram();
#endif

    pgm->GetUniform("pos1")->Set(flag.pos1);
    pgm->GetUniform("pos2")->Set(flag.pos2);
    pgm->GetUniform("right")->Set(flag.right);

    FlagUpdate(o, pgm, flag);

    oglSetWorldMatrix(&o->mat);
    o->material->Apply();

    obdrawDrawObject(o);

    return true;
}

/*-------------------------------------------------------------------------
Function: glslClothFlagSetup
Description:
--------------------------------------------------------------------------*/
void glslClothFlagSetup(OBJECT* o)
{
    Flags flags;

    flags.init = false;
    flags.valid = false;

    _data[o] = flags;

    if(ogl::UseDeprecatedPath())
        o->drawcallback = FlagDrawCallbackDeprecated;
    else
    {
        if(!oglIsMaterial(_material))
            _material = oglFindMaterialName("cloth");

#if USE_STATESET
        o->material->SetMode(GL_LIGHTING, 0);
        o->material->SetRenderState(new Program("flagprogram"));
#else
        o->material->SetMaterial(_material);
#endif
        o->drawcallback = FlagDrawCallback;
    }

    o->killcallback = FlagKillCallback;
}
