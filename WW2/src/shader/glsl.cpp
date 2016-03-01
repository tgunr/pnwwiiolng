/*------------------------------------------------------------------
Module: osgShader.cpp
Description: Wrapper for creating vertex/frag shaders
-------------------------------------------------------------------*/
#include "gl/glew.h"
#include "ogl/ogl.h"
#include "mem/mem.h"

#include "shader/glslProgram.h"
#include "shader/glslShader.h"
#include "shader/glslNormal.h"
#include "shader/glslSpecular.h"
#include "shader/glslWater.h"
#include "shader/glslCloth.h"
#include "shader/glsl.h"
#include "fio/fio.h"
#include "ob/obdlite.h"
#include "math/math3d.h"
#include "math/mathnum.h"
#include "ogl/oglGlobalStates.h"
#include "xml/XMLLoader2.h"
#include "ogl/States.h"

/*--- For debug reloading of shaders, if necessary ---*/
#ifdef BETA_RELEASE

#include "ui/uiChat.h"

#include <map>
#include <string>

static bool ShaderReload = false;

typedef struct tagShaderData
{
    tagShaderData(const char* path, glslShader* sh)
    {
        name = path;
        shader = sh;

        fioModified(&then, path);
        now = then;
    }

    std::string name;
    int then;
    int now;
    glslShader* shader;

} ShaderData;

typedef std::set<ShaderData *> ShaderMap;
static ShaderMap _shaders;

#endif // BETA_RELEASE

class ShaderLoader : public XMLLoader2<ShaderLoader>
{
public:

	ShaderLoader(const char* path) : XMLLoader2<ShaderLoader>(path)
	{
        AddHandler("program", &ShaderLoader::ReadProgram, this);
		AddHandler("vertexshader", &ShaderLoader::ReadVertexShader, this);
		AddHandler("fragmentshader", &ShaderLoader::ReadFragmentShader, this);
		AddHandler("geometryshader", &ShaderLoader::ReadGeometryShader, this);
		AddHandler("uniform", &ShaderLoader::ReadUniform, this);
		AddHandler("attribute", &ShaderLoader::ReadAttribute, this);
		AddHandler("materialconstants", &ShaderLoader::ReadMaterialsUsed, this);
        AddHandler("transformfeedbackvaryings", &ShaderLoader::ReadTranformFeedbackVaryings, this);
	}

	virtual ~ShaderLoader()
	{
	}

	XML_TAG_HANDLER(ReadProgram)
	{
        if(first)
        {
            const char* s = xmlGetStringFromNode(node, "statesetonly");
            
#if USE_STATESET
            if(s && !strncmp(s, "false", 4))
            {
                return false;
            }
#else
            if(s && !strncmp(s, "true", 4))
            {
                return false;
            }
#endif

            s = xmlGetStringFromNode(node, "name");

            pgm = GLSL.getProgram(s);

            if(pgm)
                return false;

            pgm = GLSL.loadProgram(s, nullptr, nullptr);
        }
        else
        {
            if(pgm)
            {
                if(ogl::UseDeprecatedPath())
                    glslLoadGlobalUniforms(pgm);

                pgm->Apply();
            }
        }

        return true;
	}

    XML_TAG_HANDLER(ReadMaterialsUsed)
    {
        if(first)
        {
            const char* s = xmlGetStringFromNode(node, "name");

            if(s)
            {
                if(strstr(s, "ka"))
                    pgm->SetUseKa(true);
                if(strstr(s, "kd"))
                    pgm->SetUseKd(true);
                if(strstr(s, "ke"))
                    pgm->SetUseKe(true);
                if(strstr(s, "ks"))
                    pgm->SetUseKs(true);
                if(strstr(s, "shininess"))
                    pgm->SetUseShininess(true);
            }
        }

        return true;
    }

	XML_TAG_HANDLER(ReadVertexShader)
	{
        if(first)
        {
            const char* s = xmlGetStringFromNode(node);
           
            ref_ptr<glslShader> shader = GLSL.loadVertexShader(s);
            pgm->AddShader(shader.get());
        }

        return true;
	}

	XML_TAG_HANDLER(ReadFragmentShader)
    {
        if(first)
        {
            const char* s = xmlGetStringFromNode(node);
           
            ref_ptr<glslShader> shader = GLSL.loadFragmentShader(s);
            pgm->AddShader(shader.get());
        }

        return true;
    }

	XML_TAG_HANDLER(ReadGeometryShader)
    {
        if(first)
        {
            const char* s = xmlGetStringFromNode(node);
           
            ref_ptr<glslShader> shader = GLSL.loadGeometryShader(s);
            pgm->AddShader(shader.get());
        }

        return true;
    }

    XML_TAG_HANDLER(ReadTranformFeedbackVaryings)
    {
        if(first)
        {
            struct _xmlAttr* prop = node->properties;

            for(; prop != nullptr; prop = prop->next)
            {
                const char* p = (const char*)prop->children->content;
                pgm->AddTransformFeedbackVarying(p);
            }
        }

        return true;
    }

	XML_TAG_HANDLER(ReadUniform)
	{
        if(first)
        {
            const char* p = nullptr;
            std::string name;
            std::string type;
            bool onetimeapply = false;
            unsigned int numelements = 0;

            xmlGetNodeValue(node, XML_DATA_STRING, &p, "name");
            name = p;

            if(!xmlGetNodeValue(node, XML_DATA_BOOL, &onetimeapply, "onetimeapply"))
                onetimeapply = false;
            
            if(!xmlGetNodeValue(node, XML_DATA_UINT, &numelements, "numelements"))
                numelements = 1;

            if(xmlGetNodeValue(node, XML_DATA_STRING, &p, "type"))
            {
                type = p;

                if(type.find("samplerbuffer") != std::string::npos)
                {
                    unsigned int unit;

                    if(!xmlGetNodeValue(node, XML_DATA_UINT, &unit, "unit"))
                        unit = 0;
                
                    pgm->GetOrCreateUniform(name.c_str(), glslUniform::SAMPLER_BUFFER, numelements);
                    pgm->GetUniform(name.c_str())->Set((int)unit);
                }
                else if(type.find("sampler2darray") != std::string::npos)
                {
                    unsigned int unit;

                    if(!xmlGetNodeValue(node, XML_DATA_UINT, &unit, "unit"))
                        unit = 0;
                
                    pgm->GetOrCreateUniform(name.c_str(), glslUniform::SAMPLER_2D_ARRAY, numelements);
                    pgm->GetUniform(name.c_str())->Set((int)unit);
                }
                else if(type.find("samplerCube") != std::string::npos)
                {
                    unsigned int unit;

                    if(!xmlGetNodeValue(node, XML_DATA_UINT, &unit, "unit"))
                        unit = 0;
                
                    pgm->GetOrCreateUniform(name.c_str(), glslUniform::SAMPLER_CUBE, numelements);
                    pgm->GetUniform(name.c_str())->Set((int)unit);
                }
                else if(type.find("sampler2dshadow") != std::string::npos)
                {
                    unsigned int unit;

                    if(!xmlGetNodeValue(node, XML_DATA_UINT, &unit, "unit"))
                        unit = 0;
                
                    pgm->GetOrCreateUniform(name.c_str(), glslUniform::SAMPLER_2D_SHADOW, numelements);
                    pgm->GetUniform(name.c_str())->Set((int)unit);
                }
                else if(type.find("unsignedintsampler2d") != std::string::npos)
                {
                    unsigned int unit;

                    if(!xmlGetNodeValue(node, XML_DATA_UINT, &unit, "unit"))
                        unit = 0;
                
                    pgm->GetOrCreateUniform(name.c_str(), glslUniform::UNSIGNED_INT_SAMPLER_2D, numelements);
                    pgm->GetUniform(name.c_str())->Set((int)unit);
                }
                else if(type.find("intsampler2d") != std::string::npos)
                {
                    unsigned int unit;

                    if(!xmlGetNodeValue(node, XML_DATA_UINT, &unit, "unit"))
                        unit = 0;
                
                    pgm->GetOrCreateUniform(name.c_str(), glslUniform::INT_SAMPLER_2D, numelements);
                    pgm->GetUniform(name.c_str())->Set((int)unit);
                }
                else if(type.find("sampler2d") != std::string::npos)
                {
                    unsigned int unit;

                    if(!xmlGetNodeValue(node, XML_DATA_UINT, &unit, "unit"))
                        unit = 0;
                
                    pgm->GetOrCreateUniform(name.c_str(), glslUniform::SAMPLER_2D, numelements);
                    pgm->GetUniform(name.c_str())->Set((int)unit);
                }
                else if(type.find("float_vec2") != std::string::npos)
                    pgm->GetOrCreateUniform(name.c_str(), glslUniform::FLOAT_VEC2, numelements);
                else if(type.find("float_vec3") != std::string::npos)
                    pgm->GetOrCreateUniform(name.c_str(), glslUniform::FLOAT_VEC3, numelements);
                else if(type.find("float_vec4") != std::string::npos)
                    pgm->GetOrCreateUniform(name.c_str(), glslUniform::FLOAT_VEC4, numelements);
                else if(type.find("float_mat2") != std::string::npos)
                    pgm->GetOrCreateUniform(name.c_str(), glslUniform::FLOAT_MAT2, numelements);
                else if(type.find("float_mat3") != std::string::npos)
                    pgm->GetOrCreateUniform(name.c_str(), glslUniform::FLOAT_MAT3, numelements);
                else if(type.find("float_mat4") != std::string::npos)
                    pgm->GetOrCreateUniform(name.c_str(), glslUniform::FLOAT_MAT4, numelements);
                else if(type.find("float") != std::string::npos)
                    pgm->GetOrCreateUniform(name.c_str(), glslUniform::FLOAT, numelements);
                else if(type.find("int") != std::string::npos)
                    pgm->GetOrCreateUniform(name.c_str(), glslUniform::INT, numelements);
            }

            pgm->GetUniform(name.c_str())->SetOneTimeApply(onetimeapply);
        }

        return true;
	}

	XML_TAG_HANDLER(ReadAttribute)
	{
        if(first)
        {
            const char* p;
            std::string name;
            std::string type;

            xmlGetNodeValue(node, XML_DATA_STRING, &p, "name");
            name = p;

            xmlGetNodeValue(node, XML_DATA_STRING, &p, "type");
            type = p;

            if(type.find("attribute_normal") != std::string::npos)
                pgm->AddBindAttribLocation(name, oglVertexBuffer::ATTRIBUTE_NORMAL);
            else if(type.find("attribute_position") != std::string::npos)
                pgm->AddBindAttribLocation(name, oglVertexBuffer::ATTRIBUTE_POSITION);
            else if(type.find("attribute_texcoord0") != std::string::npos)
                pgm->AddBindAttribLocation(name, oglVertexBuffer::ATTRIBUTE_TEXCOORD0);
            else if(type.find("attribute_texcoord1") != std::string::npos)
                pgm->AddBindAttribLocation(name, oglVertexBuffer::ATTRIBUTE_TEXCOORD1);
            else if(type.find("attribute_texcoord2") != std::string::npos)
                pgm->AddBindAttribLocation(name, oglVertexBuffer::ATTRIBUTE_TEXCOORD2);
            else if(type.find("attribute_texcoord3") != std::string::npos)
                pgm->AddBindAttribLocation(name, oglVertexBuffer::ATTRIBUTE_TEXCOORD3);
            else if(type.find("attribute_tangentnormal") != std::string::npos)
                pgm->AddBindAttribLocation(name, oglVertexBuffer::ATTRIBUTE_TANGENTNORMAL);
            else if(type.find("attribute_tangent") != std::string::npos)
                pgm->AddBindAttribLocation(name, oglVertexBuffer::ATTRIBUTE_TANGENT);
            else if(type.find("attribute_binormal") != std::string::npos)
                pgm->AddBindAttribLocation(name, oglVertexBuffer::ATTRIBUTE_BINORMAL);
            else if(type.find("attribute_color") != std::string::npos)
                pgm->AddBindAttribLocation(name, oglVertexBuffer::ATTRIBUTE_COLOR);
        }

        return true;
	}

private:
    glslProgram* pgm;
};

struct LightUniforms
{
    glslUniform* enabled;
    glslUniform* kc;
    glslUniform* kl;
    glslUniform* kq;
    glslUniform* dir;
    glslUniform* pos;
    glslUniform* diffuse;
    glslUniform* specular;
    glslUniform* ambient;
};

#define max_lights  4
static struct LightUniforms lightuniforms[max_lights];

OpenGLShader GLSL;

/*------------------------------------------------------------------
Function: InitGlobalUniforms
Description:
-------------------------------------------------------------------*/
static void InitGlobalUniforms()
{
    char str[64];

    for(int i = 0; i < max_lights; i++)
    {
        sprintf(str, "lightsource[%d].enabled", i);
        lightuniforms[i].enabled = new glslUniform(glslUniform::FLOAT, str, 1, true);

        sprintf(str, "lightsource[%d].kc", i);
        lightuniforms[i].kc = new glslUniform(glslUniform::FLOAT, str, 1, true);

        sprintf(str, "lightsource[%d].kl", i);
        lightuniforms[i].kl = new glslUniform(glslUniform::FLOAT, str, 1, true);

        sprintf(str, "lightsource[%d].kq", i);
        lightuniforms[i].kq = new glslUniform(glslUniform::FLOAT, str, 1, true);

        sprintf(str, "lightsource[%d].dir", i);
        lightuniforms[i].dir = new glslUniform(glslUniform::FLOAT_VEC3, str, 1, true);

        sprintf(str, "lightsource[%d].pos", i);
        lightuniforms[i].pos = new glslUniform(glslUniform::FLOAT_VEC3, str, 1, true);

        sprintf(str, "lightsource[%d].diffuse", i);
        lightuniforms[i].diffuse = new glslUniform(glslUniform::FLOAT_VEC4, str, 1, true);

        sprintf(str, "lightsource[%d].specular", i);
        lightuniforms[i].specular = new glslUniform(glslUniform::FLOAT_VEC4, str, 1, true);

        sprintf(str, "lightsource[%d].ambient", i);
        lightuniforms[i].ambient = new glslUniform(glslUniform::FLOAT_VEC4, str, 1, true);
    }
}

/*------------------------------------------------------------------
Function: LoadShaders
Description:
-------------------------------------------------------------------*/
static bool LoadShaders()
{
    std::string path;
    
    if(ogl::UseDeprecatedPath())
        path = "fx/xml/shadersdeprecated.xml";
    else
        path = "fx/xml/shaders.xml";

    ShaderLoader load(path.c_str());
    load.Load();

    ProgramState::Bind(0);

    return true;
}

/*------------------------------------------------------------------
Function: glslInit
Description:
-------------------------------------------------------------------*/
bool glslLoadShaderXML(const char* file)
{
    ShaderLoader load(file);
    load.Load();

    ProgramState::Bind(0);

    return true;
}

/*------------------------------------------------------------------
Function: glslKill
Description:
-------------------------------------------------------------------*/
void glslKill()
{
    GLSL.Kill();

    for(int i = 0; i < max_lights; i++)
    {
        delete lightuniforms[i].enabled;
        delete lightuniforms[i].kc;
        delete lightuniforms[i].kl;
        delete lightuniforms[i].kq;
        delete lightuniforms[i].dir;
        delete lightuniforms[i].pos;
        delete lightuniforms[i].diffuse;
        delete lightuniforms[i].specular;
        delete lightuniforms[i].ambient;
    }
}

/*------------------------------------------------------------------
Function: glslInit
Description:
-------------------------------------------------------------------*/
bool glslInit()
{
    extern void glslProgramInit();
    extern void oglTextureObjectInit();

    glslProgramInit();

    if(!ogl::UseDeprecatedPath())
    {
        oglTextureObjectInit();
    }

    if(ogl::UseDeprecatedPath())
        InitGlobalUniforms();
    
    if(!LoadShaders())
        return false;

    if(!glslNormalInit())
        return false;

    if(!glslSpecularInit())
        return false;

    if(!glslWaterInit())
        return false;

    if(!glslClothInit())
        return false;

    return true;
}

/*------------------------------------------------------------------
Function: glslPostRenderUpdate
Description:
-------------------------------------------------------------------*/
bool glslPostRenderUpdate()
{
    GLSL.update();

    glslWaterUpdate();

    return true;
}

/*------------------------------------------------------------------
Function: glslLoadGlobalUniforms
Description:
-------------------------------------------------------------------*/
void glslLoadGlobalUniforms(glslProgram* pgm)
{
    if(ogl::UseDeprecatedPath())
    {
        for(int i = 0; i < max_lights; i++)
        {
            pgm->AddUniform(lightuniforms[i].enabled);
            pgm->AddUniform(lightuniforms[i].kc);
            pgm->AddUniform(lightuniforms[i].kl);
            pgm->AddUniform(lightuniforms[i].kq);
            pgm->AddUniform(lightuniforms[i].dir);
            pgm->AddUniform(lightuniforms[i].pos);
            pgm->AddUniform(lightuniforms[i].specular);
            pgm->AddUniform(lightuniforms[i].ambient);
            pgm->AddUniform(lightuniforms[i].diffuse);
        }
    }
}

/*------------------------------------------------------------------
Function: UpdateLightsDeprecated
Description:
-------------------------------------------------------------------*/
static inline void UpdateLightsDeprecated()
{
    DYNALITE* lights = nullptr;
    int num = 0;

    num = obdliteGetLights(&lights);

    if(num > 0)
    {
        for(int i = 0; i < max_lights; i++)
        {
            lightuniforms[i].enabled->Set((lights[i].light.flags & LITE_ON) ? 1.0f : 0.0f);

            if(lights[i].light.flags & LITE_ON)
            {
                lightuniforms[i].diffuse->Set(lights[i].light.light.dcvColor);
                lightuniforms[i].ambient->Set(lights[i].light.light.dcvAmbient);
                lightuniforms[i].specular->Set(lights[i].light.light.dcvSpecular);
                lightuniforms[i].kc->Set(lights[i].light.light.dvAttenuation0);
                lightuniforms[i].kl->Set(lights[i].light.light.dvAttenuation1);
                lightuniforms[i].kq->Set(lights[i].light.light.dvAttenuation2);

                piVECTOR dir = lights[i].light.light.dvDirection;
                dir.x = -dir.x;
                dir.y = -dir.y;
                dir.z = -dir.z;

                // Put light in eye space
                math3dMultiplyRotation(&dir, &dir, oglGetViewMatrix());
                math3dNormalize(&dir);
                lightuniforms[i].dir->Set(dir);

                if(i == 2 || i == 3)
                {
                    piVECTOR pos = lights[i].light.light.dvPosition;

                    math3dVectorMultiply(&pos, &pos, oglGetViewMatrix());
                    lightuniforms[i].pos->Set(pos);
                }
            }
        }
    }
}

/*------------------------------------------------------------------
Function: UpdateLights
Description:
-------------------------------------------------------------------*/
static inline void UpdateLights()
{
    DYNALITE* lights = nullptr;
    int num = 0;

    num = obdliteGetLights(&lights);

    static glslUniformBlock* lightblock0;
    static glslUniformBlock* lightblock1;
    static glslUniformBlock* lightblock2;
    static glslUniformBlock* lightblock3;

    static bool firsttime = true;

    if(firsttime)
    {
        lightblock0 = glslGetUniformBlock("LightBlock0");
        lightblock1 = glslGetUniformBlock("LightBlock1");
        lightblock2 = glslGetUniformBlock("LightBlock2");
        lightblock3 = glslGetUniformBlock("LightBlock3");

        if(lightblock0)
            firsttime = false;
        else 
            return;
    }

    piVECTOR dir;
    piVECTOR pos;

    if(num > 0)
    {
        if(lights[0].light.flags & LITE_ON)
        {
            dir = lights[0].light.light.dvDirection;
            
            dir.x = -dir.x;
            dir.y = -dir.y;
            dir.z = -dir.z;

            // Put light in eye space
            math3dMultiplyRotation(&dir, &dir, oglGetViewMatrix());
            math3dNormalize(&dir);

            lightblock0->Set("b_enabled0", 1.0f);
            lightblock0->Set("b_kc0", lights[0].light.light.dvAttenuation0);
            lightblock0->Set("b_kl0", lights[0].light.light.dvAttenuation1);
            lightblock0->Set("b_kq0", lights[0].light.light.dvAttenuation2);
            lightblock0->Set("b_dir0", dir);
            lightblock0->Set("b_diffuse0", lights[0].light.light.dcvColor);
            lightblock0->Set("b_specular0", lights[0].light.light.dcvSpecular);
            lightblock0->Set("b_ambient0", lights[0].light.light.dcvAmbient);
        }
        else
        {
            lightblock0->Set("b_enabled0", 0.0f);
        }
       
        if(lights[1].light.flags & LITE_ON)
        {
            dir = lights[1].light.light.dvDirection;
            
            dir.x = -dir.x;
            dir.y = -dir.y;
            dir.z = -dir.z;

            // Put light in eye space
            math3dMultiplyRotation(&dir, &dir, oglGetViewMatrix());
            math3dNormalize(&dir);

            lightblock1->Set("b_enabled1", 1.0f);
            lightblock1->Set("b_kc1", lights[1].light.light.dvAttenuation0);
            lightblock1->Set("b_kl1", lights[1].light.light.dvAttenuation1);
            lightblock1->Set("b_kq1", lights[1].light.light.dvAttenuation2);
            lightblock1->Set("b_dir1", dir);
            lightblock1->Set("b_diffuse1", lights[1].light.light.dcvColor);
            lightblock1->Set("b_specular1", lights[1].light.light.dcvSpecular);
            lightblock1->Set("b_ambient1", lights[1].light.light.dcvAmbient);
        }
        else
        {
            lightblock1->Set("b_enabled1", 0.0f);
        }

        if(lights[2].light.flags & LITE_ON)
        {
            dir = lights[2].light.light.dvDirection;
            
            dir.x = -dir.x;
            dir.y = -dir.y;
            dir.z = -dir.z;

            // Put light in eye space
            math3dMultiplyRotation(&dir, &dir, oglGetViewMatrix());
            math3dNormalize(&dir);

            Vec4f p(lights[2].light.light.dvPosition);
            p = p * Matrixf(*oglGetViewMatrix());

            p.get(pos);

            lightblock2->Set("b_enabled2", 1.0f);
            lightblock2->Set("b_kc2", lights[2].light.light.dvAttenuation0);
            lightblock2->Set("b_kl2", lights[2].light.light.dvAttenuation1);
            lightblock2->Set("b_kq2", lights[2].light.light.dvAttenuation2);
            lightblock2->Set("b_dir2", dir);
            lightblock2->Set("b_pos2", pos);
            lightblock2->Set("b_diffuse2", lights[2].light.light.dcvColor);
            lightblock2->Set("b_specular2", lights[2].light.light.dcvSpecular);
            lightblock2->Set("b_ambient2", lights[2].light.light.dcvAmbient);
        }
        else
        {
            lightblock2->Set("b_enabled2", 0.0f);
        }

        if(lights[3].light.flags & LITE_ON)
        {
            dir = lights[3].light.light.dvDirection;
            
            dir.x = -dir.x;
            dir.y = -dir.y;
            dir.z = -dir.z;

            // Put light in eye space
            math3dMultiplyRotation(&dir, &dir, oglGetViewMatrix());
            math3dNormalize(&dir);

            Vec4f p(lights[3].light.light.dvPosition);
            p = p * Matrixf(*oglGetViewMatrix());

            p.get(pos);

            lightblock3->Set("b_enabled3", 1.0f);
            lightblock3->Set("b_kc3", lights[3].light.light.dvAttenuation0);
            lightblock3->Set("b_kl3", lights[3].light.light.dvAttenuation1);
            lightblock3->Set("b_kq3", lights[3].light.light.dvAttenuation2);
            lightblock3->Set("b_dir3", dir);
            lightblock3->Set("b_pos3", pos);
            lightblock3->Set("b_diffuse3", lights[3].light.light.dcvColor);
            lightblock3->Set("b_specular3", lights[3].light.light.dcvSpecular);
            lightblock3->Set("b_ambient3", lights[3].light.light.dcvAmbient);
        }
        else
        {
            lightblock3->Set("b_enabled3", 0.0f);
        }
    }

    glslUniformBlock* ambient = glslGetUniformBlock("AmbientBlock");

    if(ambient)
    {
        oglCOLOR amb = oglGetAmbLight();
        piCOLORVALUE c;

        c.r = piRGBA_GETRED(amb) * colorONEOVER255;
        c.g = piRGBA_GETGREEN(amb) * colorONEOVER255;
        c.b = piRGBA_GETBLUE(amb) * colorONEOVER255;
        c.a = 1.0f;

        ambient->Set("b_globalambient", c);
    }
}

/*------------------------------------------------------------------
Function: UpdateFog
Description:
-------------------------------------------------------------------*/
static inline void UpdateFog()
{
    static glslUniformBlock* fog = nullptr;

    if(!fog)
        fog = glslGetUniformBlock("FogBlock");

    if(fog)
    {
        piCOLORVALUE c;

        c.r = piRGBA_GETRED(FogColor) * colorONEOVER255;
        c.g = piRGBA_GETGREEN(FogColor) * colorONEOVER255;
        c.b = piRGBA_GETBLUE(FogColor) * colorONEOVER255;
        c.a = piRGBA_GETALPHA(FogColor) * colorONEOVER255;

        fog->Set("b_fogcolor", c);

        float end = piLongLikeFloat(FogEnd);
        fog->Set("b_end", end);

        float start = piLongLikeFloat(FogStart);
        fog->Set("b_scale", 1.0f / (end - start));
    }
}

/*------------------------------------------------------------------
Function: glslPreRenderUpdate
Description:
-------------------------------------------------------------------*/
bool glslPreRenderUpdate()
{
    if(ogl::UseDeprecatedPath())
    {
        UpdateLightsDeprecated();
    }
    else
    {
        UpdateLights();
        UpdateFog();
    }

    return true;
}

/*------------------------------------------------------------------
Function: glslSetModelViewMatrix
Description:
-------------------------------------------------------------------*/
void glslSetModelViewMatrix()
{
    static glslUniformBlock* u = nullptr;

    if(!u)
        u = glslGetUniformBlock("MatrixBlock");

    const piMATRIX& modelview = oglGetModelViewMatrix();

    Matrixf normal(modelview);
    normal(3, 0) = normal(3, 1) = normal(3, 2) = 0;
    normal = normal.inverse(normal);

    piMATRIX NormalMatrix;
    normal.get(NormalMatrix);
    mathTransposeMatrix(NormalMatrix);

    if(u)
    {
        u->Set("b_MVMatrix", modelview);
        u->Set("b_NormalMatrix", NormalMatrix);
    }
}

/*------------------------------------------------------------------
Function: glslSetProjectionMatrix
Description:
-------------------------------------------------------------------*/
void glslSetProjectionMatrix()
{
    static glslUniformBlock* u = nullptr;

    if(!u)
        u = glslGetUniformBlock("MatrixBlock");

    if(u)
    {
        u->Set("b_ProjectionMatrix", *oglGetProjectionMatrix());
    }
}

/*------------------------------------------------------------------
Function: glslSetTextureMatrix
Description:
-------------------------------------------------------------------*/
void glslSetTextureMatrix(int unit, const piMATRIX& mat)
{
    glslSetTextureMatrix(unit, Matrixf(mat));
}

/*------------------------------------------------------------------
Function: glslSetTextureMatrix
Description:
-------------------------------------------------------------------*/
void glslSetTextureMatrix(int unit, const Matrixf& mat)
{
    static glslUniformBlock* u = nullptr;

    if(!u)
        u = glslGetUniformBlock("TextureMatrixBlock");

    if(u && unit < 3)
    {
        // Having TextureMatrix[3] in shader gives incorrect bytesize (driver???)
        static char buff[3][32] = 
        {
            "b_TextureMatrix0",
            "b_TextureMatrix1",
            "b_TextureMatrix2"
        };

        u->Set(buff[unit], mat);
    }
}

/*------------------------------------------------------------------
Function: glslToggleShaderReload
Description:
-------------------------------------------------------------------*/
void glslToggleShaderReload()
{
#ifdef BETA_RELEASE
    ShaderReload = !ShaderReload;
    uiChatPrintLine("Reloading modifications");
#endif
}

/*------------------------------------------------------------------
Function: ~OpenGLShader()
Description:
-------------------------------------------------------------------*/
OpenGLShader::~OpenGLShader()
{
#ifdef BETA_RELEASE
    ShaderMap::iterator itr = _shaders.begin();
    ShaderMap::iterator free;

    while(itr != _shaders.end())
    {
        free = itr;
        ++itr;
        ShaderData* sd = *free;
        delete sd;
    }

    _shaders.clear();
#endif

    _programs.clear();
    _vshaders.clear();
    _fshaders.clear();
}

/*------------------------------------------------------------------
Function: _loadShaderSource
Description:
-------------------------------------------------------------------*/
void OpenGLShader::_loadShaderSource(glslShader* shader, const char* file)
{
	size_t size = fioFileSize(file);
    int handle = fioOpen(file);

    if(size && handle != -1)
    {
        char* buffer = new char[size + 1];
        buffer[0] = 0;

        fioRead(handle, buffer, size);
        buffer[size] = 0;

        shader->SetShaderSource(buffer);
        delete [] buffer;
    }

    fioClose(handle);
}

/*------------------------------------------------------------------
Function: loadVertexShader
Description:
-------------------------------------------------------------------*/
ref_ptr<glslShader> OpenGLShader::loadVertexShader(const char* shader)
{
	if(_vshaders.find(shader) != _vshaders.end())
	{
		return _vshaders.find(shader)->second;
	}

    ref_ptr<glslShader> vertex = new glslShader(glslShader::VERTEX);

    _loadShaderSource(vertex.get(), shader);

#ifdef BETA_RELEASE
    ShaderData* sd = new ShaderData(shader, vertex.get());
    _shaders.insert(sd);
#endif

    vertex->SetName(shader);

    _vshaders[shader] = vertex;

    return vertex;
}

/*------------------------------------------------------------------
Function: loadGeometryShader
Description:
-------------------------------------------------------------------*/
ref_ptr<glslShader> OpenGLShader::loadGeometryShader(const char* shader)
{
	if(_gshaders.find(shader) != _gshaders.end())
    {
        return _gshaders.find(shader)->second;
    }

    ref_ptr<glslShader> geometry = new glslShader(glslShader::GEOMETRY);

    _loadShaderSource(geometry.get(), shader);

#ifdef BETA_RELEASE
    ShaderData* sd = new ShaderData(shader, geometry.get());
    _shaders.insert(sd);
#endif

    _gshaders[shader] = geometry;

    return geometry;
}

/*------------------------------------------------------------------
Function: loadFragmentShader
Description:
-------------------------------------------------------------------*/
ref_ptr<glslShader> OpenGLShader::loadFragmentShader(const char* shader)
{
	if(_fshaders.find(shader) != _fshaders.end())
    {
        return _fshaders.find(shader)->second;
    }

    ref_ptr<glslShader> fragment = new glslShader(glslShader::FRAGMENT);

    _loadShaderSource(fragment.get(), shader);

#ifdef BETA_RELEASE
    ShaderData* sd = new ShaderData(shader, fragment.get());
    _shaders.insert(sd);
#endif

    fragment->SetName(shader);

    _fshaders[shader] = fragment;

    return fragment;
}

/*------------------------------------------------------------------
Function: update
Description: Reload shader if necessary...for debugging
-------------------------------------------------------------------*/
void OpenGLShader::update()
{
#ifdef BETA_RELEASE
    if(!ShaderReload)
    {
        return;
    }

    ShaderMap::iterator itr = _shaders.begin();

    for(; itr != _shaders.end(); ++itr)
    {
        ShaderData* sd = *itr;

        fioModified(&sd->now, sd->name.c_str());

        if(sd->now != sd->then)
        {
            sd->then = sd->now;
            _loadShaderSource(sd->shader, sd->name.c_str());
        }
    }

    ShaderReload = false;
#endif
}

/*------------------------------------------------------------------
Function: getProgram
Description:
-------------------------------------------------------------------*/
glslProgram* OpenGLShader::getProgram(const char* name)
{
    if(_programs.find(name) != _programs.end())
    {
        return _programs.find(name)->second.get();
    }

    return nullptr;
}

/*------------------------------------------------------------------
Function: loadProgram
Description:
-------------------------------------------------------------------*/
glslProgram* OpenGLShader::loadProgram(const char* name, const char* vert, const char* frag)
{
    if(_programs.find(name) != _programs.end())
    {
        return _programs.find(name)->second.get();
    }

    ref_ptr<glslShader> vshader;
    ref_ptr<glslShader> fshader;

    if(vert)
    {
        vshader = loadVertexShader(vert);
    }

    if(frag)
    {
        fshader = loadFragmentShader(frag);
    }

    ref_ptr<glslProgram> program = new glslProgram;
    program->SetName(name);

    if(vshader.valid())
    {
        program->AddShader(vshader.get());
    }

    if(fshader.valid())
    {
        program->AddShader(fshader.get());
    }

    _programs[name] = program;

    return program.get();
}
