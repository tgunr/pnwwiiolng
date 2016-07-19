#include <syscfg.h>
/*------------------------------------------------------------------------------------
Module: glslUniform.cpp
Description:
-------------------------------------------------------------------------------------*/
#include "gl/glew.h"
#include "shader/glslUniform.h"
#include "log/log.h"
#include "math/Vec2f.h"
#include "math/Vec3d.h"
#include "math/Vec4d.h"
#include "shader/glslProgram.h"
#include "ogl/oglGlobalStates.h"
#include "ogl/oglVertexBuffer.h"

/*------------------------------------------------------------------------------------
Function: glslUniform::Uniform()
Description:
-------------------------------------------------------------------------------------*/
glslUniform::glslUniform() :
    _numElements(0),
    _modifiedCount(0),
    _oldModifiedCount(0),
    _oneTimeApply(false),
    global(false),
    _type(UNDEFINED)
{
}

/*------------------------------------------------------------------------------------
Function: glslUniform::Uniform()
Description:
-------------------------------------------------------------------------------------*/
glslUniform::glslUniform(Type type, const std::string& name, int numElements, bool isglobal) :
    _numElements(0),
    _modifiedCount(0),
    _oneTimeApply(false),
    global(isglobal),
    _type(type)
{
    SetName(name);
    SetNumElements(numElements);
    AllocateDataArray();
}

/*------------------------------------------------------------------------------------
Function: glslUniform::SetName()
Description:
-------------------------------------------------------------------------------------*/
void glslUniform::SetName(const std::string& name)
{
    if(_name != "")
    {
        Write_Log(LOG_OLD, LOG_INFO, "ERROR: cannot change Uniform name");
        return;
    }

    _name = name;
}

/*------------------------------------------------------------------------------------
Function: glslUniform::SetArray()
Description:
-------------------------------------------------------------------------------------*/
bool glslUniform::SetArray(std::vector<float>& array)
{
    // Incoming array must match configuration of the Uniform
    if(GetInternalArrayType(GetType()) != GL_FLOAT)
    {
        Write_Log(LOG_OLD, LOG_WARN, "ERROR: Uniform::setArray : incompatible array");
        return false;
    }

    _floatArray = array;
    _intArray.clear();

    dirty();

    return true;
}

/*------------------------------------------------------------------------------------
Function: glslUniform::SetArray()
Description:
-------------------------------------------------------------------------------------*/
bool glslUniform::SetArray(std::vector<int>& array)
{
    // Incoming array must match configuration of the Uniform
    if(GetInternalArrayType(GetType()) != GL_INT)
    {
        Write_Log(LOG_OLD, LOG_WARN, "ERROR: Uniform::setArray : incompatible array");
        return false;
    }

    _intArray = array;
    _floatArray.clear();

    dirty();

    return true;
}

/*------------------------------------------------------------------------------------
Function: glslUniform::SetNumElements()
Description:
-------------------------------------------------------------------------------------*/
void glslUniform::SetNumElements(unsigned int numElements)
{
    if(numElements < 1)
    {
        Write_Log(LOG_OLD, LOG_WARN, "ERROR: Uniform numElements < 1 is invalid");
        return;
    }

    if(numElements == _numElements) return;

    if(_numElements > 0)
    {
        Write_Log(LOG_OLD, LOG_WARN, "ERROR: cannot change Uniform numElements, size already fixed.");
        return;
    }

    _numElements = numElements;

    AllocateDataArray();
}

/*------------------------------------------------------------------------------------
Function: glslUniform::GetInternalArrayType()
Description:
-------------------------------------------------------------------------------------*/
GLenum glslUniform::GetInternalArrayType(Type t)
{
    switch(t)
    {
    case FLOAT:
    case FLOAT_VEC2:
    case FLOAT_VEC3:
    case FLOAT_VEC4:
    case FLOAT_MAT2:
    case FLOAT_MAT3:
    case FLOAT_MAT4:
    case FLOAT_MAT2x3:
    case FLOAT_MAT2x4:
    case FLOAT_MAT3x2:
    case FLOAT_MAT3x4:
    case FLOAT_MAT4x2:
    case FLOAT_MAT4x3:
        {
            return GL_FLOAT;
        }
    case INT:
    case INT_VEC2:
    case INT_VEC3:
    case INT_VEC4:
    case BOOL:
    case BOOL_VEC2:
    case BOOL_VEC3:
    case BOOL_VEC4:
    case SAMPLER_1D:
    case SAMPLER_2D:
    case SAMPLER_1D_ARRAY:
    case SAMPLER_2D_ARRAY:
    case SAMPLER_3D:
    case SAMPLER_CUBE:
    case SAMPLER_1D_SHADOW:
    case SAMPLER_2D_SHADOW:
    case SAMPLER_1D_ARRAY_SHADOW:
    case SAMPLER_2D_ARRAY_SHADOW:
    case SAMPLER_BUFFER:
    case SAMPLER_CUBE_SHADOW:
    case INT_SAMPLER_1D:
    case INT_SAMPLER_2D:
    case INT_SAMPLER_3D:
    case INT_SAMPLER_CUBE:
    case INT_SAMPLER_2D_RECT:
    case INT_SAMPLER_1D_ARRAY:
    case INT_SAMPLER_2D_ARRAY:
    case INT_SAMPLER_BUFFER:
    case UNSIGNED_INT_SAMPLER_1D:
    case UNSIGNED_INT_SAMPLER_2D:
    case UNSIGNED_INT_SAMPLER_3D:
    case UNSIGNED_INT_SAMPLER_CUBE:
    case UNSIGNED_INT_SAMPLER_2D_RECT:
    case UNSIGNED_INT_SAMPLER_1D_ARRAY:
    case UNSIGNED_INT_SAMPLER_2D_ARRAY:
    case UNSIGNED_INT_SAMPLER_BUFFER:
        {
            return GL_INT;
        }
    // TODO integrate new types
    case UNSIGNED_INT_VEC2:
    case UNSIGNED_INT_VEC3:
    case UNSIGNED_INT_VEC4:
    default:
        {
            return 0;
        }
    }
}

/*------------------------------------------------------------------------------------
Function: glslUniform::AllocateDataArray()
Description:
-------------------------------------------------------------------------------------*/
void glslUniform::AllocateDataArray()
{
    // If one array is already allocated, the job is done.
    if(!_floatArray.empty() || !_intArray.empty()) return;

    // Array cannot be created until _type and _numElements are specified
    int arrayNumElements = GetInternalArrayNumElements();

    if(arrayNumElements)
    {
        switch(GetInternalArrayType(GetType()))
        {
        case GL_FLOAT:
            {
                _floatArray.resize(arrayNumElements);
                _intArray.clear();
                return;
            }
        case GL_INT:
            {
                _intArray.resize(arrayNumElements);
                _floatArray.clear();
                return;
            }
        default:
            {
                break;
            }
        }
    }

    _floatArray.clear();
    _intArray.clear();
}

/*------------------------------------------------------------------------------------
Function: glslUniform::GetTypeNumComponents()
Description:
-------------------------------------------------------------------------------------*/
int glslUniform::GetTypeNumComponents(Type t) const
{
    switch(t)
    {
    case FLOAT:
    case INT:
    case BOOL:
    case SAMPLER_1D:
    case SAMPLER_2D:
    case SAMPLER_1D_ARRAY:
    case SAMPLER_2D_ARRAY:
    case SAMPLER_3D:
    case SAMPLER_CUBE:
    case SAMPLER_1D_SHADOW:
    case SAMPLER_2D_SHADOW:
    case SAMPLER_1D_ARRAY_SHADOW:
    case SAMPLER_2D_ARRAY_SHADOW:
    case SAMPLER_BUFFER:
    case SAMPLER_CUBE_SHADOW:
    case INT_SAMPLER_1D:
    case INT_SAMPLER_2D:
    case INT_SAMPLER_3D:
    case INT_SAMPLER_CUBE:
    case INT_SAMPLER_2D_RECT:
    case INT_SAMPLER_1D_ARRAY:
    case INT_SAMPLER_2D_ARRAY:
    case INT_SAMPLER_BUFFER:
    case UNSIGNED_INT_SAMPLER_1D:
    case UNSIGNED_INT_SAMPLER_2D:
    case UNSIGNED_INT_SAMPLER_3D:
    case UNSIGNED_INT_SAMPLER_CUBE:
    case UNSIGNED_INT_SAMPLER_2D_RECT:
    case UNSIGNED_INT_SAMPLER_1D_ARRAY:
    case UNSIGNED_INT_SAMPLER_2D_ARRAY:
    case UNSIGNED_INT_SAMPLER_BUFFER:
        {
            return 1;
        }
    case FLOAT_VEC2:
    case INT_VEC2:
    case BOOL_VEC2:
    case UNSIGNED_INT_VEC2:
        {
            return 2;
        }
    case FLOAT_VEC3:
    case INT_VEC3:
    case BOOL_VEC3:
    case UNSIGNED_INT_VEC3:
        {
            return 3;
        }
    case FLOAT_VEC4:
    case FLOAT_MAT2:
    case INT_VEC4:
    case BOOL_VEC4:
    case UNSIGNED_INT_VEC4:
        {
            return 4;
        }
    case FLOAT_MAT2x3:
    case FLOAT_MAT3x2:
        {
            return 6;
        }
    case FLOAT_MAT2x4:
    case FLOAT_MAT4x2:
        {
            return 8;
        }
    case FLOAT_MAT3:
        {
            return 9;
        }
    case FLOAT_MAT3x4:
    case FLOAT_MAT4x3:
        {
            return 12;
        }
    case FLOAT_MAT4:
        {
            return 16;
        }
    default:
        {
            return 0;
        }
    }
}

/*------------------------------------------------------------------------------------
Function: glslUniform::GetInternalArrayNumElements()
Description:
-------------------------------------------------------------------------------------*/
unsigned int glslUniform::GetInternalArrayNumElements()
{
    if(GetNumElements() < 1 || GetType() == UNDEFINED) return 0;

    return GetNumElements() * GetTypeNumComponents(GetType());
}

/*------------------------------------------------------------------------------------
Function: glslUniform::SetType()
Description:
-------------------------------------------------------------------------------------*/
bool glslUniform::SetType(Type t)
{
    if(_type == t)
    {
        return true;
    }

    if(_type != UNDEFINED)
    {
        Write_Log(LOG_OLD, LOG_WARN, "ERROR: cannot change Uniform type");
        return false;
    }

    _type = t;

    AllocateDataArray();

    return true;
}

/*------------------------------------------------------------------------------------
Function: glslUniform::GetGlApiType()
Description:
-------------------------------------------------------------------------------------*/
glslUniform::Type glslUniform::GetGlApiType(Type t) const
{
    switch(t)
    {
    case BOOL:
    case SAMPLER_1D:
    case SAMPLER_2D:
    case SAMPLER_1D_ARRAY:
    case SAMPLER_2D_ARRAY:
    case SAMPLER_3D:
    case SAMPLER_CUBE:
    case SAMPLER_1D_SHADOW:
    case SAMPLER_2D_SHADOW:
    case SAMPLER_1D_ARRAY_SHADOW:
    case SAMPLER_2D_ARRAY_SHADOW:
    case SAMPLER_BUFFER:
    case SAMPLER_CUBE_SHADOW:
    case INT_SAMPLER_1D:
    case INT_SAMPLER_2D:
    case INT_SAMPLER_3D:
    case INT_SAMPLER_CUBE:
    case INT_SAMPLER_2D_RECT:
    case INT_SAMPLER_1D_ARRAY:
    case INT_SAMPLER_2D_ARRAY:
    case INT_SAMPLER_BUFFER:
    case UNSIGNED_INT_SAMPLER_1D:
    case UNSIGNED_INT_SAMPLER_2D:
    case UNSIGNED_INT_SAMPLER_3D:
    case UNSIGNED_INT_SAMPLER_CUBE:
    case UNSIGNED_INT_SAMPLER_2D_RECT:
    case UNSIGNED_INT_SAMPLER_1D_ARRAY:
    case UNSIGNED_INT_SAMPLER_2D_ARRAY:
    case UNSIGNED_INT_SAMPLER_BUFFER:
        {
            return INT;
        }
    case BOOL_VEC2:
        {
            return INT_VEC2;
        }
    case BOOL_VEC3:
        {
            return INT_VEC3;
        }
    case BOOL_VEC4:
        {
            return INT_VEC4;
        }
    default:
        {
            return t;
        }
    }
}

/*------------------------------------------------------------------------------------
Function: glslUniform::GetTypename()
Description:
-------------------------------------------------------------------------------------*/
const char* glslUniform::GetTypename(Type t)
{
    switch(t)
    {
    case FLOAT: return "float";
    case FLOAT_VEC2: return "vec2";
    case FLOAT_VEC3: return "vec3";
    case FLOAT_VEC4: return "vec4";
    case INT: return "int";
    case INT_VEC2: return "ivec2";
    case INT_VEC3: return "ivec3";
    case INT_VEC4: return "ivec4";
    case BOOL: return "bool";
    case BOOL_VEC2: return "bvec2";
    case BOOL_VEC3: return "bvec3";
    case BOOL_VEC4: return "bvec4";
    case FLOAT_MAT2: return "mat2";
    case FLOAT_MAT3: return "mat3";
    case FLOAT_MAT4: return "mat4";
    case SAMPLER_1D: return "sampler1D";
    case SAMPLER_2D: return "sampler2D";
    case SAMPLER_1D_ARRAY: return "sampler1DArray";
    case SAMPLER_2D_ARRAY: return "sampler2DArray";
    case SAMPLER_3D: return "sampler3D";
    case SAMPLER_CUBE: return "samplerCube";
    case SAMPLER_1D_SHADOW: return "sampler1DShadow";
    case SAMPLER_2D_SHADOW: return "sampler2DShadow";
    case SAMPLER_1D_ARRAY_SHADOW: return "sampler1DArrayShadow";
    case SAMPLER_2D_ARRAY_SHADOW: return "sampler2DArrayShadow";
    case FLOAT_MAT2x3: return "mat2x3";
    case FLOAT_MAT2x4: return "mat2x4";
    case FLOAT_MAT3x2: return "mat3x2";
    case FLOAT_MAT3x4: return "mat3x4";
    case FLOAT_MAT4x2: return "mat4x2";
    case FLOAT_MAT4x3: return "mat4x3";
    case SAMPLER_BUFFER: return "samplerBuffer";
    case SAMPLER_CUBE_SHADOW: return "samplerCubeShadow";
    case UNSIGNED_INT_VEC2: return "uvec2";
    case UNSIGNED_INT_VEC3: return "uvec3";
    case UNSIGNED_INT_VEC4: return "uvec4";
    case INT_SAMPLER_1D: return "isampler1D";
    case INT_SAMPLER_2D: return "isampler2D";
    case INT_SAMPLER_3D: return "isampler3D";
    case INT_SAMPLER_CUBE: return "isamplerCube";
    case INT_SAMPLER_2D_RECT: return "isampler2DRect";
    case INT_SAMPLER_1D_ARRAY: return "isampler1DArray";
    case INT_SAMPLER_2D_ARRAY: return "isampler2DArray";
    case INT_SAMPLER_BUFFER: return "isamplerBuffer";
    case UNSIGNED_INT_SAMPLER_1D: return "usampler1D";
    case UNSIGNED_INT_SAMPLER_2D: return "usampler2D";
    case UNSIGNED_INT_SAMPLER_3D: return "usampler3D";
    case UNSIGNED_INT_SAMPLER_CUBE: return "usamplerCube";
    case UNSIGNED_INT_SAMPLER_2D_RECT: return "usampler2DRect";
    case UNSIGNED_INT_SAMPLER_1D_ARRAY: return "usampler1DArray";
    case UNSIGNED_INT_SAMPLER_2D_ARRAY: return "usampler2DArray";
    case UNSIGNED_INT_SAMPLER_BUFFER: return "usamplerBuffer";
    default: return "UNDEFINED";
    }
}

/*------------------------------------------------------------------------------------
Function: glslUniform::IsCompatibleType()
Description:
-------------------------------------------------------------------------------------*/
bool glslUniform::IsCompatibleType(Type t)
{
    if((t == UNDEFINED) || (GetType() == UNDEFINED))
    {
        return false;
    }

    if(t == GetType())
    {
        return true;
    }

    if(GetGlApiType(t) == GetGlApiType(GetType()))
    {
        return true;
    }

    Write_Log(LOG_OLD, LOG_WARN, "ERROR: Cannot assign between types %s and %s", GetTypename(t), GetTypename(GetType()));

    return false;
}

/*------------------------------------------------------------------------------------
Function: glslUniform::SetElement()
Description:
-------------------------------------------------------------------------------------*/
bool glslUniform::SetElement(unsigned int index, const piUV& v2)
{
    if(index >= GetNumElements() || !IsCompatibleType(FLOAT_VEC2))
    {
        return false;
    }

    unsigned int j = index * GetTypeNumComponents(GetType());

    if(_floatArray[j + 0] == v2.u &&
       _floatArray[j + 1] == v2.v)
    {
        return true;
    }

    _floatArray[j + 0] = v2.u;
    _floatArray[j + 1] = v2.v;

    dirty();

    return true;
}

/*------------------------------------------------------------------------------------
Function: glslUniform::SetElement()
Description:
-------------------------------------------------------------------------------------*/
bool glslUniform::SetElement(unsigned int index, const Vec2f& v2)
{
    if(index >= GetNumElements() || !IsCompatibleType(FLOAT_VEC2))
    {
        return false;
    }

    unsigned int j = index * GetTypeNumComponents(GetType());

    if(_floatArray[j + 0] == v2.x() &&
       _floatArray[j + 1] == v2.y())
    {
        return true;
    }

    _floatArray[j + 0] = v2.x();
    _floatArray[j + 1] = v2.y();

    dirty();

    return true;
}

/*------------------------------------------------------------------------------------
Function: glslUniform::SetElement()
Description:
-------------------------------------------------------------------------------------*/
bool glslUniform::SetElement(unsigned int index, const piVECTOR& v3)
{
    if(index >= GetNumElements() || !IsCompatibleType(FLOAT_VEC3))
    {
        return false;
    }

    unsigned int j = index * GetTypeNumComponents(GetType());

    if(_floatArray[j + 0] == v3.x &&
       _floatArray[j + 1] == v3.y &&
       _floatArray[j + 2] == v3.z)
    {
        return true;
    }

    _floatArray[j + 0] = v3.x;
    _floatArray[j + 1] = v3.y;
    _floatArray[j + 2] = v3.z;

    dirty();

    return true;
}

/*------------------------------------------------------------------------------------
Function: glslUniform::SetElement()
Description:
-------------------------------------------------------------------------------------*/
bool glslUniform::SetElement(unsigned int index, const Vec3d& v3)
{
    if(index >= GetNumElements() || !IsCompatibleType(FLOAT_VEC3))
    {
        return false;
    }

    unsigned int j = index * GetTypeNumComponents(GetType());

    if(_floatArray[j + 0] == (float)v3.x() &&
       _floatArray[j + 1] == (float)v3.y() &&
       _floatArray[j + 2] == (float)v3.z())
    {
        return true;
    }

    _floatArray[j + 0] = (float)v3.x();
    _floatArray[j + 1] = (float)v3.y();
    _floatArray[j + 2] = (float)v3.z();

    dirty();

    return true;
}

/*------------------------------------------------------------------------------------
Function: glslUniform::SetElement()
Description:
-------------------------------------------------------------------------------------*/
bool glslUniform::SetElement(unsigned int index, const Vec3f& v3)
{
    if(index >= GetNumElements() || !IsCompatibleType(FLOAT_VEC3))
    {
        return false;
    }

    unsigned int j = index * GetTypeNumComponents(GetType());

    if(_floatArray[j + 0] == v3.x() &&
       _floatArray[j + 1] == v3.y() &&
       _floatArray[j + 2] == v3.z())
    {
        return true;
    }

    _floatArray[j + 0] = v3.x();
    _floatArray[j + 1] = v3.y();
    _floatArray[j + 2] = v3.z();

    dirty();

    return true;
}

/*------------------------------------------------------------------------------------
Function: glslUniform::SetElement()
Description:
-------------------------------------------------------------------------------------*/
bool glslUniform::SetElement(unsigned int index, const Vec4d& v4)
{
    if(index >= GetNumElements() || !IsCompatibleType(FLOAT_VEC4))
    {
        return false;
    }

    unsigned int j = index * GetTypeNumComponents(GetType());

    if(_floatArray[j + 0] == (float)v4.x() &&
       _floatArray[j + 1] == (float)v4.y() &&
       _floatArray[j + 2] == (float)v4.z() &&
       _floatArray[j + 3] == (float)v4.w())
    {
        return true;
    }

    _floatArray[j + 0] = (float)v4.x();
    _floatArray[j + 1] = (float)v4.y();
    _floatArray[j + 2] = (float)v4.z();
    _floatArray[j + 3] = (float)v4.w();

    dirty();

    return true;
}

/*------------------------------------------------------------------------------------
Function: glslUniform::SetElement()
Description:
-------------------------------------------------------------------------------------*/
bool glslUniform::SetElement(unsigned int index, const Vec4f& v4)
{
    if(index >= GetNumElements() || !IsCompatibleType(FLOAT_VEC4))
    {
        return false;
    }

    unsigned int j = index * GetTypeNumComponents(GetType());

    if(_floatArray[j + 0] == v4.x() &&
       _floatArray[j + 1] == v4.y() &&
       _floatArray[j + 2] == v4.z() &&
       _floatArray[j + 3] == v4.w())
    {
        return true;
    }

    _floatArray[j + 0] = v4.x();
    _floatArray[j + 1] = v4.y();
    _floatArray[j + 2] = v4.z();
    _floatArray[j + 3] = v4.w();

    dirty();

    return true;
}

/*------------------------------------------------------------------------------------
Function: glslUniform::SetElement()
Description:
-------------------------------------------------------------------------------------*/
bool glslUniform::SetElement(unsigned int index, const piQUAT& v4)
{
    if(index >= GetNumElements() || !IsCompatibleType(FLOAT_VEC4))
    {
        return false;
    }

    unsigned int j = index * GetTypeNumComponents(GetType());

    if(_floatArray[j + 0] == v4.x &&
       _floatArray[j + 1] == v4.y &&
       _floatArray[j + 2] == v4.z &&
       _floatArray[j + 3] == v4.w)
    {
        return true;
    }

    _floatArray[j + 0] = v4.x;
    _floatArray[j + 1] = v4.y;
    _floatArray[j + 2] = v4.z;
    _floatArray[j + 3] = v4.w;

    dirty();

    return true;
}

/*------------------------------------------------------------------------------------
Function: glslUniform::SetElement()
Description:
-------------------------------------------------------------------------------------*/
bool glslUniform::SetElement(unsigned int index, const piCOLORVALUE& v4)
{
    if(index >= GetNumElements() || !IsCompatibleType(FLOAT_VEC4))
    {
        return false;
    }

    unsigned int j = index * GetTypeNumComponents(GetType());

    if(_floatArray[j + 0] == v4.r &&
       _floatArray[j + 1] == v4.g &&
       _floatArray[j + 2] == v4.b &&
       _floatArray[j + 3] == v4.a)
    {
        return true;
    }

    _floatArray[j + 0] = v4.r;
    _floatArray[j + 1] = v4.g;
    _floatArray[j + 2] = v4.b;
    _floatArray[j + 3] = v4.a;

    dirty();

    return true;
}

/*------------------------------------------------------------------------------------
Function: glslUniform::SetElement()
Description:
-------------------------------------------------------------------------------------*/
bool glslUniform::SetElement(unsigned int index, const piMATRIX& m4)
{
    if(index >= GetNumElements() || !IsCompatibleType(FLOAT_MAT4))
    {
        return false;
    }

    unsigned int j = index * GetTypeNumComponents(GetType());

    const float* p = (float*)&m4;

    bool changed = false;

    for(int i = 0; i < 16; i++)
    {
        if(_floatArray[j + i] == p[i])
            continue;

        changed = true;
        _floatArray[j + i] = p[i];
    }

    if(changed)
    {
        dirty();
    }

    return true;
}

/*------------------------------------------------------------------------------------
Function: glslUniform::SetElement()
Description:
-------------------------------------------------------------------------------------*/
bool glslUniform::SetElement(unsigned int index, const piMATRIX3x3& m3)
{
    if(index >= GetNumElements() || !IsCompatibleType(FLOAT_MAT3))
    {
        return false;
    }

    unsigned int j = index * GetTypeNumComponents(GetType());

    const float* p = (float*)&m3;

    bool changed = false;

    for(int i = 0; i < 9; i++)
    {
        if(_floatArray[j + i] == p[i])
            continue;

        changed = true;
        _floatArray[j + i] = p[i];
    }

    if(changed)
    {
        dirty();
    }

    return true;
}

/*------------------------------------------------------------------------------------
Function: glslUniform::SetElement()
Description:
-------------------------------------------------------------------------------------*/
bool glslUniform::SetElement(unsigned int index, const Matrixf& m4)
{
    if(index >= GetNumElements() || !IsCompatibleType(FLOAT_MAT4))
    {
        return false;
    }

    unsigned int j = index * GetTypeNumComponents(GetType());

    const float* p = (float*)&m4;

    bool changed = false;

    for(int i = 0; i < 16; i++)
    {
        if(_floatArray[j + i] == p[i])
            continue;

        changed = true;
        _floatArray[j + i] = p[i];
    }

    if(changed)
    {
        dirty();
    }

    return true;
}

/*------------------------------------------------------------------------------------
Function: glslUniform::SetElement()
Description:
-------------------------------------------------------------------------------------*/
bool glslUniform::SetElement(unsigned int index, int i)
{
    if(index >= GetNumElements() || !IsCompatibleType(INT))
    {
        return false;
    }

    unsigned int j = index * GetTypeNumComponents(GetType());

    if(i == _intArray[j])
        return true;

    _intArray[j] = i;

    dirty();

    return true;
}

/*------------------------------------------------------------------------------------
Function: glslUniform::SetElement()
Description:
-------------------------------------------------------------------------------------*/
bool glslUniform::SetElement(unsigned int index, int i0, int i1)
{
    if(index >= GetNumElements() || !IsCompatibleType(INT_VEC2))
    {
        return false;
    }

    unsigned int j = index * GetTypeNumComponents(GetType());

    if(_intArray[j + 0] == i0 &&
       _intArray[j + 1] == i1)
    {
        return true;
    }

    _intArray[j + 0] = i0;
    _intArray[j + 1] = i1;

    dirty();

    return true;
}

/*------------------------------------------------------------------------------------
Function: glslUniform::SetElement()
Description:
-------------------------------------------------------------------------------------*/
bool glslUniform::SetElement(unsigned int index, int i0, int i1, int i2)
{
    if(index >= GetNumElements() || !IsCompatibleType(INT_VEC3))
    {
        return false;
    }

    unsigned int j = index * GetTypeNumComponents(GetType());

    if(_intArray[j + 0] == i0 &&
       _intArray[j + 1] == i1 &&
       _intArray[j + 2] == i2)
    {
        return true;
    }

    _intArray[j + 0] = i0;
    _intArray[j + 1] = i1;
    _intArray[j + 2] = i2;

    dirty();

    return true;
}

/*------------------------------------------------------------------------------------
Function: glslUniform::SetElement()
Description:
-------------------------------------------------------------------------------------*/
bool glslUniform::SetElement(unsigned int index, int i0, int i1, int i2, int i3)
{
    if(index >= GetNumElements() || !IsCompatibleType(INT_VEC4))
    {
        return false;
    }

    unsigned int j = index * GetTypeNumComponents(GetType());

    if(_intArray[j + 0] == i0 &&
       _intArray[j + 1] == i1 &&
       _intArray[j + 2] == i2 &&
       _intArray[j + 3] == i3)
    {
        return true;
    }

    _intArray[j + 0] = i0;
    _intArray[j + 1] = i1;
    _intArray[j + 2] = i2;
    _intArray[j + 3] = i3;

    dirty();

    return true;
}

/*------------------------------------------------------------------------------------
Function: glslUniform::SetElement()
Description:
-------------------------------------------------------------------------------------*/
bool glslUniform::SetElement(unsigned int index, bool i)
{
    if(index >= GetNumElements() || !IsCompatibleType(BOOL))
    {
        return false;
    }

    unsigned int j = index * GetTypeNumComponents(GetType());

    if(_intArray[j] == (int)i)
        return true;

    _intArray[j] = i;

    dirty();

    return true;
}

/*------------------------------------------------------------------------------------
Function: glslUniform::SetElement()
Description:
-------------------------------------------------------------------------------------*/
bool glslUniform::SetElement(unsigned int index, bool i0, bool i1)
{
    if(index >= GetNumElements() || !IsCompatibleType(BOOL_VEC2))
    {
        return false;
    }

    unsigned int j = index * GetTypeNumComponents(GetType());

    if(_intArray[j + 0] == (int)i0 &&
       _intArray[j + 1] == (int)i1)
    {
        return true;
    }

    _intArray[j + 0] = i0;
    _intArray[j + 1] = i1;

    dirty();

    return true;
}

/*------------------------------------------------------------------------------------
Function: glslUniform::SetElement()
Description:
-------------------------------------------------------------------------------------*/
bool glslUniform::SetElement(unsigned int index, bool i0, bool i1, bool i2)
{
    if(index >= GetNumElements() || !IsCompatibleType(BOOL_VEC3))
    {
        return false;
    }

    unsigned int j = index * GetTypeNumComponents(GetType());

    if(_intArray[j + 0] == (int)i0 &&
       _intArray[j + 1] == (int)i1 &&
       _intArray[j + 2] == (int)i2)
    {
        return true;
    }

    _intArray[j + 0] = i0;
    _intArray[j + 1] = i1;
    _intArray[j + 2] = i2;

    dirty();

    return true;
}

/*------------------------------------------------------------------------------------
Function: glslUniform::SetElement()
Description:
-------------------------------------------------------------------------------------*/
bool glslUniform::SetElement(unsigned int index, bool i0, bool i1, bool i2, bool i3)
{
    if(index >= GetNumElements() || !IsCompatibleType(BOOL_VEC4))
    {
        return false;
    }

    unsigned int j = index * GetTypeNumComponents(GetType());

    if(_intArray[j + 0] == (int)i0 &&
       _intArray[j + 1] == (int)i1 &&
       _intArray[j + 2] == (int)i2 &&
       _intArray[j + 3] == (int)i3)
    {
        return true;
    }

    _intArray[j + 0] = i0;
    _intArray[j + 1] = i1;
    _intArray[j + 2] = i2;
    _intArray[j + 3] = i3;

    dirty();

    return true;
}

/*------------------------------------------------------------------------------------
Function: glslUniform::SetElement()
Description:
-------------------------------------------------------------------------------------*/
bool glslUniform::SetElement(unsigned int index, float f0, float f1, float f2, float f3)
{
    if(index >= GetNumElements() || !IsCompatibleType(FLOAT_VEC4))
    {
        return false;
    }

    unsigned int j = index * GetTypeNumComponents(GetType());

    if(_floatArray[j + 0] == f0 &&
       _floatArray[j + 1] == f1 &&
       _floatArray[j + 2] == f2 &&
       _floatArray[j + 3] == f3)
    {
        return true;
    }

    _floatArray[j + 0] = f0;
    _floatArray[j + 1] = f1;
    _floatArray[j + 2] = f2;
    _floatArray[j + 3] = f3;

    dirty();

    return true;
}

/*------------------------------------------------------------------------------------
Function: glslUniform::SetElement()
Description:
-------------------------------------------------------------------------------------*/
bool glslUniform::SetElement(unsigned int index, float f0, float f1, float f2)
{
    if(index >= GetNumElements() || !IsCompatibleType(FLOAT_VEC3))
    {
        return false;
    }

    unsigned int j = index * GetTypeNumComponents(GetType());

    if(_floatArray[j + 0] == f0 &&
       _floatArray[j + 1] == f1 &&
       _floatArray[j + 2] == f2)
    {
        return true;
    }

    _floatArray[j + 0] = f0;
    _floatArray[j + 1] = f1;
    _floatArray[j + 2] = f2;

    dirty();

    return true;
}

/*------------------------------------------------------------------------------------
Function: glslUniform::SetElement()
Description:
-------------------------------------------------------------------------------------*/
bool glslUniform::SetElement(unsigned int index, float f0, float f1)
{
    if(index >= GetNumElements() || !IsCompatibleType(FLOAT_VEC2))
    {
        return false;
    }

    unsigned int j = index * GetTypeNumComponents(GetType());

    if(_floatArray[j + 0] == f0 &&
       _floatArray[j + 1] == f1)
    {
        return true;
    }

    _floatArray[j + 0] = f0;
    _floatArray[j + 1] = f1;

    dirty();

    return true;
}

/*------------------------------------------------------------------------------------
Function: glslUniform::SetElement()
Description:
-------------------------------------------------------------------------------------*/
bool glslUniform::SetElement(unsigned int index, float f0)
{
    if(index >= GetNumElements() || !IsCompatibleType(FLOAT))
    {
        return false;
    }

    unsigned int j = index * GetTypeNumComponents(GetType());

    if(_floatArray[j + 0] == f0)
    {
        return true;
    }

    _floatArray[j + 0] = f0;

    dirty();

    return true;
}

/*------------------------------------------------------------------------------------
Function: glslUniform::Set()
Description:
-------------------------------------------------------------------------------------*/
bool glslUniform::Set(const piUV& v2)
{
    if(GetNumElements() == 0) SetNumElements(1);
    return IsScalar() ? SetElement(0, v2) : false;
}

/*------------------------------------------------------------------------------------
Function: glslUniform::Set()
Description:
-------------------------------------------------------------------------------------*/
bool glslUniform::Set(const Vec2f& v2)
{
    if(GetNumElements() == 0) SetNumElements(1);
    return IsScalar() ? SetElement(0, v2) : false;
}

/*------------------------------------------------------------------------------------
Function: glslUniform::Set()
Description:
-------------------------------------------------------------------------------------*/
bool glslUniform::Set(const piVECTOR& v3 )
{
    if(GetNumElements() == 0) SetNumElements(1);
    return IsScalar() ? SetElement(0, v3) : false;
}

/*------------------------------------------------------------------------------------
Function: glslUniform::Set()
Description:
-------------------------------------------------------------------------------------*/
bool glslUniform::Set(const Vec3d& v3 )
{
    if(GetNumElements() == 0) SetNumElements(1);
    return IsScalar() ? SetElement(0, v3) : false;
}

/*------------------------------------------------------------------------------------
Function: glslUniform::Set()
Description:
-------------------------------------------------------------------------------------*/
bool glslUniform::Set(const Vec3f& v3 )
{
    if(GetNumElements() == 0) SetNumElements(1);
    return IsScalar() ? SetElement(0, v3) : false;
}

/*------------------------------------------------------------------------------------
Function: glslUniform::Set()
Description:
-------------------------------------------------------------------------------------*/
bool glslUniform::Set(const Vec4d& v4 )
{
    if(GetNumElements() == 0) SetNumElements(1);
    return IsScalar() ? SetElement(0, v4) : false;
}

/*------------------------------------------------------------------------------------
Function: glslUniform::Set()
Description:
-------------------------------------------------------------------------------------*/
bool glslUniform::Set(const Vec4f& v4 )
{
    if(GetNumElements() == 0) SetNumElements(1);
    return IsScalar() ? SetElement(0, v4) : false;
}

/*------------------------------------------------------------------------------------
Function: glslUniform::Set()
Description:
-------------------------------------------------------------------------------------*/
bool glslUniform::Set(const piQUAT& v4)
{
    if(GetNumElements() == 0) SetNumElements(1);
    return IsScalar() ? SetElement(0, v4) : false;
}

/*------------------------------------------------------------------------------------
Function: glslUniform::Set()
Description:
-------------------------------------------------------------------------------------*/
bool glslUniform::Set(const piCOLORVALUE& v4)
{
    if(GetNumElements() == 0) SetNumElements(1);
    return IsScalar() ? SetElement(0, v4) : false;
}

/*------------------------------------------------------------------------------------
Function: glslUniform::Set()
Description:
-------------------------------------------------------------------------------------*/
bool glslUniform::Set(const piMATRIX& m4)
{
    if(GetNumElements() == 0) SetNumElements(1);
    return IsScalar() ? SetElement(0, m4) : false;
}

/*------------------------------------------------------------------------------------
Function: glslUniform::Set()
Description:
-------------------------------------------------------------------------------------*/
bool glslUniform::Set(const piMATRIX3x3& m3)
{
    if(GetNumElements() == 0) SetNumElements(1);
    return IsScalar() ? SetElement(0, m3) : false;
}

/*------------------------------------------------------------------------------------
Function: glslUniform::Set()
Description:
-------------------------------------------------------------------------------------*/
bool glslUniform::Set(const Matrixf& m4)
{
    if(GetNumElements() == 0) SetNumElements(1);
    return IsScalar() ? SetElement(0, m4) : false;
}

/*------------------------------------------------------------------------------------
Function: glslUniform::Set()
Description:
-------------------------------------------------------------------------------------*/
bool glslUniform::Set(int i)
{
    if(GetNumElements() == 0) SetNumElements(1);
    return IsScalar() ? SetElement(0, i) : false;
}

/*------------------------------------------------------------------------------------
Function: glslUniform::Set()
Description:
-------------------------------------------------------------------------------------*/
bool glslUniform::Set(int i0, int i1)
{
    if(GetNumElements() == 0) SetNumElements(1);
    return IsScalar() ? SetElement(0, i0, i1) : false;
}

/*------------------------------------------------------------------------------------
Function: glslUniform::Set()
Description:
-------------------------------------------------------------------------------------*/
bool glslUniform::Set(int i0, int i1, int i2)
{
    if(GetNumElements() == 0) SetNumElements(1);
    return IsScalar() ? SetElement(0, i0, i1, i2) : false;
}

/*------------------------------------------------------------------------------------
Function: glslUniform::Set()
Description:
-------------------------------------------------------------------------------------*/
bool glslUniform::Set(int i0, int i1, int i2, int i3)
{
    if(GetNumElements() == 0) SetNumElements(1);
    return IsScalar() ? SetElement(0, i0, i1, i2, i3) : false;
}

/*------------------------------------------------------------------------------------
Function: glslUniform::Set()
Description:
-------------------------------------------------------------------------------------*/
bool glslUniform::Set(bool b)
{
    if(GetNumElements() == 0) SetNumElements(1);
    return IsScalar() ? SetElement(0, b) : false;
}

/*------------------------------------------------------------------------------------
Function: glslUniform::Set()
Description:
-------------------------------------------------------------------------------------*/
bool glslUniform::Set(bool b0, bool b1)
{
    if(GetNumElements() == 0) SetNumElements(1);
    return IsScalar() ? SetElement(0, b0, b1) : false;
}

/*------------------------------------------------------------------------------------
Function: glslUniform::Set()
Description:
-------------------------------------------------------------------------------------*/
bool glslUniform::Set(bool b0, bool b1, bool b2)
{
    if(GetNumElements() == 0) SetNumElements(1);
    return IsScalar() ? SetElement(0, b0, b1, b2) : false;
}

/*------------------------------------------------------------------------------------
Function: glslUniform::Set()
Description:
-------------------------------------------------------------------------------------*/
bool glslUniform::Set(bool b0, bool b1, bool b2, bool b3)
{
    if(GetNumElements() == 0) SetNumElements(1);
    return IsScalar() ? SetElement(0, b0, b1, b2, b3) : false;
}

/*------------------------------------------------------------------------------------
Function: glslUniform::Set()
Description:
-------------------------------------------------------------------------------------*/
bool glslUniform::Set(float f0, float f1, float f2, float f3)
{
    if(GetNumElements() == 0) SetNumElements(1);
    return IsScalar() ? SetElement(0, f0, f1, f2, f3) : false;
}

/*------------------------------------------------------------------------------------
Function: glslUniform::Set()
Description:
-------------------------------------------------------------------------------------*/
bool glslUniform::Set(float f0, float f1, float f2)
{
    if(GetNumElements() == 0) SetNumElements(1);
    return IsScalar() ? SetElement(0, f0, f1, f2) : false;
}

/*------------------------------------------------------------------------------------
Function: glslUniform::Set()
Description:
-------------------------------------------------------------------------------------*/
bool glslUniform::Set(float f0, float f1)
{
    if(GetNumElements() == 0) SetNumElements(1);
    return IsScalar() ? SetElement(0, f0, f1) : false;
}

/*------------------------------------------------------------------------------------
Function: glslUniform::Set()
Description:
-------------------------------------------------------------------------------------*/
bool glslUniform::Set(float f0)
{
    if(GetNumElements() == 0) SetNumElements(1);
    return IsScalar() ? SetElement(0, f0) : false;
}

/*------------------------------------------------------------------------------------
Function: glslUniform::Apply()
Description:
-------------------------------------------------------------------------------------*/
void glslUniform::Apply(GLint location) const
{
    GLsizei num = GetNumElements();
    if(num < 1) return;

    logTimerBeginMacro();

    switch(GetGlApiType(GetType()))
    {
    case FLOAT:
        {
            if(!_floatArray.empty())
            {
                glUniform1fv(location, num, &_floatArray.front());
            }

            break;
        }
    case FLOAT_VEC2:
        {
            if(!_floatArray.empty())
            {
                glUniform2fv(location, num, &_floatArray.front());
            }

            break;
        }
    case FLOAT_VEC3:
        {
            if(!_floatArray.empty())
            {
                glUniform3fv(location, num, &_floatArray.front());
            }

            break;
        }
    case FLOAT_VEC4:
        {
            if(!_floatArray.empty())
            {
                glUniform4fv(location, num, &_floatArray.front());
            }

            break;
        }
    case FLOAT_MAT4:
        {
            if(!_floatArray.empty())
            {
                glUniformMatrix4fv(location, num, GL_FALSE, &_floatArray.front());
            }

            break;
        }
    case FLOAT_MAT3:
        {
            if(!_floatArray.empty())
            {
                glUniformMatrix3fv(location, num, GL_FALSE, &_floatArray.front());
            }

            break;
        }
    case INT:
        {
            if(!_intArray.empty())
            {
                glUniform1iv(location, num, (const GLint*)&_intArray.front());
            }

            break;
        }
    case INT_VEC2:
        {
            if(!_intArray.empty())
            {
                glUniform2iv(location, num, (const GLint*)&_intArray[0]);
            }

            break;
        }
    case INT_VEC3:
        {
            if(!_intArray.empty())
            {
                glUniform3iv(location, num, (const GLint*)&_intArray[0]);
            }

            break;
        }
    case INT_VEC4:
        {
            if(!_intArray.empty())
            {
                glUniform4iv(location, num, (const GLint*)&_intArray[0]);
            }

            break;
        }
    default:
        {
            Write_Log(LOG_OLD, LOG_WARN, "ERROR: %s %s", _FL);
            break;
        }
    }

    logTimerEndMacro();
}

