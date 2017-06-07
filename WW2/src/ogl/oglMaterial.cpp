#include "syscfg.h" // Path is project specific.
/*--------------------------------------------------------------------------
Module: oglMaterial
Description:
---------------------------------------------------------------------------*/
#include "ogl/oglMaterial.h"
#include "ogl/States.h"
#include "shader/glsl.h"

int oglMaterial::oldmaterial = -1;

/*--------------------------------------------------------------------------
Function: oglMaterial::oglMaterial
Description:
---------------------------------------------------------------------------*/
oglMaterial::oglMaterial() :
    matid(-1),
    program(nullptr),
    firsttime(true),
    poolmaterial(false),
    wantshader(false),
    material(nullptr)
{
}

/*--------------------------------------------------------------------------
Function: oglMaterial::ApplyMaterial
Description:
---------------------------------------------------------------------------*/
void oglMaterial::ApplyMaterial()
{
    bool materialset = false;

    if(!program)
    {
        if(!shadername.empty())
        {
            program = GLSL.getProgram(shadername.c_str());
        }
    }

    if(program)
    {
        if(ogl::UseDeprecatedPath())
        {
            oglSetActiveMaterial(matid);
            materialset = true;
        }
        else if(oglIsMaterial(matid))
        {
            if(oldmaterial != matid)
            {
                const piMATERIAL& mat = oglGetMaterial(matid);

                if(firsttime)
                {
                    firsttime = false;
                    material = glslGetUniformBlock("MaterialBlock");
                }

                // Will be nullptr until shader is compiled/linked
                if(material)
                {
                    material->SetBlock(&mat, sizeof(piMATERIAL));
                    oldmaterial = matid;
                }
            }

            materialset = true;
        }
    }

    if(!materialset)
        oglSetActiveMaterial(matid);

#if USE_STATESET
        States::HaveAppliedAttribute(RS_MATERIAL);
#endif
}

/*--------------------------------------------------------------------------
Function: oglMaterial::ApplyTexture
Description:
---------------------------------------------------------------------------*/
void oglMaterial::ApplyTexture(int unit, GLenum tgt)
{
    if(!texid.empty())
    {
        TextureState::Start();
        TextureState::Bind(unit, texid[unit], tgt);
        TextureState::End();
    }
}

/*--------------------------------------------------------------------------
Function: oglMaterial::ApplyShader
Description:
---------------------------------------------------------------------------*/
void oglMaterial::ApplyShader()
{
    if(!program)
    {
        if(!shadername.empty())
        {
            program = GLSL.getProgram(shadername.c_str());
        }
    }

    if(wantshader && program)
    {
        program->Apply();
        program->ApplyUniformMap();

#if USE_STATESET
        States::HaveAppliedAttribute(RS_GLSLPROGRAM);
#endif
    }
    else
    {
        ProgramState::Bind(0);
    }
}

/*--------------------------------------------------------------------------
Function: oglMaterial::ApplyTextures
Description:
---------------------------------------------------------------------------*/
void oglMaterial::ApplyTextures()
{
    for(unsigned int i = 0; i < texid.size(); ++i)
    {
        TextureState::Bind(i, texid[i], target[i]);
    }
}

/*--------------------------------------------------------------------------
Function: oglMaterial::Apply
Description:
---------------------------------------------------------------------------*/
void oglMaterial::Apply()
{
    logTimerBeginMacro();

    logTimerCallVoid(ApplyMaterial);

    TextureState::Start();
    logTimerCallVoid(ApplyTextures);
    TextureState::End();

    logTimerCallVoid(ApplyShader);

    logTimerEndMacro();
}
