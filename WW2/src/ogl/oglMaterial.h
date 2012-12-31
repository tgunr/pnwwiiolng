#pragma once

#include "osgImport/Referenced.h"
#include "osgImport/ref_ptr.h"
#include "shader/glsl.h"
#include "osgImport/Texture.h"

class oglMaterial : public Referenced
{
public:
    oglMaterial();
    virtual ~oglMaterial();

    inline void SetMaterial(int id)
    {
        extern SINT32 DefaultMaterialId;

        matid = id;

        if(matid < 0)
            matid = DefaultMaterialId;
    }

    inline void AddTexture(unsigned int unit, int id)
    {
        extern TEXTURE* textures;

        if(id > -1 && !textures[id].genlist)
        {
            oglGetTextureHandle(id, 0);
        }

        if(texid.size() < unit + 1)
        {
            target.resize(unit + 1);
            texid.resize(unit + 1);
        }

        target[unit] = id < 0 ? GL_TEXTURE_2D : textures[id].target;
        texid[unit] = id < 0 ? 0 : textures[id].genlist;
    }

    inline void AddTexture(unsigned int unit, Texture* t)
    {
        if(!t->getTextureObject(0))
        {
            t->apply();
        }

        if(texid.size() < unit + 1)
        {
            target.resize(unit + 1);
            texid.resize(unit + 1);
        }

        target[unit] = t->getTextureTarget();
        texid[unit] = t->getTextureObject(0)->_id;
    }

    inline void AddTexture(size_t unit, GLenum tgt, int GLTexId)
    {
        if(texid.size() < unit + 1)
        {
            target.resize(unit + 1);
            texid.resize(unit + 1);
        }

        target[unit] = tgt;
        texid[unit] = GLTexId;
    }

    inline void TextureResize(unsigned int size)
    {
        texid.resize(size);
        target.resize(size);
    }

    inline void SetShader(const std::string name)
    {
        shadername = name;
        program = NULL;
        wantshader = true;
    }

    inline void SetShader(glslProgram* pgm)
    {
        program = pgm;
        wantshader = true;
    }

    inline bool HasShader()
    {
        return !shadername.empty() || program != NULL;
    }

    inline int GetTexture(int unit)
    {
        if(texid.empty())
            return -1;

        if(unit >= (int)texid.size())
            return -1;

        return texid[unit];
    }

    inline GLenum GetTarget(int unit)
    {
         if(target.empty())
            return 0;

        if(unit >= (int)target.size())
            return 0;

        return target[unit];
   }

    inline unsigned int GetTextureSize()
    {
        return texid.size();
    }

    inline int GetMaterial()
    {
        return matid;
    }

    inline glslProgram* GetProgram()
    {
        if(!program)
        {
            if(!shadername.empty())
            {
                program = GLSL.getProgram(shadername.c_str());
            }
        }

        return program;
    }

    inline void SetIsPoolMaterial(bool pool)
    {
        // So we know when to delete on object free's
        poolmaterial = pool;
    }

    inline bool IsPool()
    {
        return poolmaterial;
    }

    inline void EnableShader(bool enable)
    {
        wantshader = enable;
    }

    void Apply();
    void ApplyMaterial();
    void ApplyShader();
    void ApplyTextures();
    void ApplyTexture(int unit, GLenum tgt = GL_TEXTURE_2D);

protected:
    static int oldmaterial;

    int matid;
    std::vector<int> texid;
    std::vector<GLenum> target;
    glslProgram* program;
    std::string shadername;

    bool firsttime;
    bool poolmaterial;
    bool wantshader;

    glslUniformBlock* material;
};
