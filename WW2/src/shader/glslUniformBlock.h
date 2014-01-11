#pragma once

#include <string>
#include <vector>
#include <float.h>
#include <math.h>

#include "pi/pi.h"
#include "math/Vec2f.h"
#include "math/Vec3d.h"
#include "math/Vec4d.h"
#include "wwii/wwiiHash.h"

class Matrixf;
class Matrixd;

struct UniformParams
{
    int size;
    int offset;
    int type;
    int index;

    UniformParams()
    {
        size = 0;
        offset = 0;
        type = -1;
        index = 0;
    }
};

class glslUniformBlock
{
public:
    enum Type
    {
        FLOAT = GL_FLOAT,
        FLOAT_VEC2 = GL_FLOAT_VEC2,
        FLOAT_VEC3 = GL_FLOAT_VEC3,
        FLOAT_VEC4 = GL_FLOAT_VEC4,
        INT = GL_INT,
        INT_VEC2 = GL_INT_VEC2,
        INT_VEC3 = GL_INT_VEC3,
        INT_VEC4 = GL_INT_VEC4,
        BOOL = GL_BOOL,
        BOOL_VEC2 = GL_BOOL_VEC2,
        BOOL_VEC3 = GL_BOOL_VEC3,
        BOOL_VEC4 = GL_BOOL_VEC4,
        FLOAT_MAT2 = GL_FLOAT_MAT2,
        FLOAT_MAT3 = GL_FLOAT_MAT3,
        FLOAT_MAT4 = GL_FLOAT_MAT4,

        // TODO the following must be integrated fully here and Uniform.cpp
        FLOAT_MAT2x3 = GL_FLOAT_MAT2x3,
        FLOAT_MAT2x4 = GL_FLOAT_MAT2x4,
        FLOAT_MAT3x2 = GL_FLOAT_MAT3x2,
        FLOAT_MAT3x4 = GL_FLOAT_MAT3x4,
        FLOAT_MAT4x2 = GL_FLOAT_MAT4x2,
        FLOAT_MAT4x3 = GL_FLOAT_MAT4x3,
        UNSIGNED_INT_VEC2 = GL_UNSIGNED_INT_VEC2_EXT,
        UNSIGNED_INT_VEC3 = GL_UNSIGNED_INT_VEC3_EXT,
        UNSIGNED_INT_VEC4 = GL_UNSIGNED_INT_VEC4_EXT,

        UNDEFINED = 0x0
    };

    typedef stdext::hash_map<std::string, struct UniformParams*> UniformList;

    glslUniformBlock();
    glslUniformBlock(const std::string& name);

    void AddUniform(const std::string& name, int size, int byteoffset, int type);

    inline void dirty()
    {
        _dirty = true;
    }

    inline UniformParams* GetUniform(const std::string& u)
    {
        UniformList::iterator itr = _uniformList.find(u);

        if(itr != _uniformList.end())
        {
            return itr->second;
        }

        return nullptr;
    }

    void Buffer();
    void SetByteOffset(const std::string name, int offset)
    {
        UniformList::iterator i = _uniformList.find(name);

        if(i != _uniformList.end())
        {
            i->second->offset = offset;
        }
    }
    void SetSize(const std::string name, int s)
    {
        UniformList::iterator i = _uniformList.find(name);

        if(i != _uniformList.end())
        {
            i->second->size = s;
        }
    }
    void SetBlockSize(int size) {_blocksize = size;}
    void SetUBO(int ubo) {_ubo = ubo;}
    void SetBase(int base) {_base = base;}
    void SetElement(UniformParams* u, float f);
    void SetElement(UniformParams* u, const Vec3f& v3);
    void SetElement(UniformParams* u, const Vec2f& v2);
    void SetElement(UniformParams* u, const piVECTOR& v3);
    void SetElement(UniformParams* u, const Vec3d& v3);
    void SetElement(UniformParams* u, const Vec4d& v4);
    void SetElement(UniformParams* u, const Vec4f& v4);
    void SetElement(UniformParams* u, const piCOLORVALUE& v4);
    void SetElement(UniformParams* u, const piMATRIX& m4);
    void SetElement(UniformParams* u, const piMATRIX3x3& m3);
    void SetElement(UniformParams* u, const Matrixf& m4);

    void SetElementData(UniformParams* u, const void* data, int bytesize);

    void Set(const std::string& name, float f);
    void Set(const std::string& name, const piVECTOR& v3);
    void Set(const std::string& name, const piCOLORVALUE& v4);
    void Set(const std::string& name, const piMATRIX& m4);
    void Set(const std::string& name, const piMATRIX3x3& m3);
    void Set(const std::string& name, const Matrixf& m4);
    void Set(const std::string& name, const Vec2f& v2);
    void Set(const std::string& name, const Vec3f& v3);
    void Set(const std::string& name, const Vec3d& v3);
    void Set(const std::string& name, const Vec4d& v4);
    void Set(const std::string& name, const Vec4f& v4);

    void SetData(const std::string& name, const void* data, int bytesize);
    void SetBlock(const void* data, int bytesize);

    unsigned int GetInternalArrayNumElements();
    unsigned int GetNumElements() const {return _numElements;}

    const std::string& GetName() const {return _name;}

    int GetTypeNumComponents(Type t) const;

    GLenum GetInternalArrayType(Type t);

    Type GetGlApiType(Type t) const;
    Type GetType() const {return _type;}

protected:
    unsigned int _numElements;

    unsigned int _ubo;

    mutable bool _dirty;

    int _base;
    int _blocksize;

    UniformList _uniformList;

    std::vector<float> _floatArray;
    std::vector<int> _intArray;

    std::string _name;

    Type _type;
};

extern glslUniformBlock* glslGetOrCreateUniformBlock(const std::string& name);
extern glslUniformBlock* glslGetUniformBlock(const std::string& name);

