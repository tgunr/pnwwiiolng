/*------------------------------------------------------------------------------------
Module: glslUniformBlock.cpp
Description:
-------------------------------------------------------------------------------------*/
#include "gl/glew.h"
#include "shader/glslUniformBlock.h"
#include "shader/glslUniform.h"
#include "log/log.h"
#include "shader/glslProgram.h"
#include "ogl/oglGlobalStates.h"
#include "ogl/States.h"
#include "math/Matrixf.h"

using UniformBlockList = std::unordered_map<std::string, glslUniformBlock*>;
static UniformBlockList _uniformBlockList;

/*------------------------------------------------------------------------------------
Function: glslUniformBlockInit()
Description:
-------------------------------------------------------------------------------------*/
void glslUniformBlockInit()
{
}

/*------------------------------------------------------------------------------------
Function: glslProgram::glslGetUniformBlock()
Description:
-------------------------------------------------------------------------------------*/
glslUniformBlock* glslGetUniformBlock(const std::string& name)
{
    UniformBlockList::iterator itr = _uniformBlockList.find(name);

    if(itr !=_uniformBlockList.end())
    {
        return itr->second;
    }

    return nullptr;
}

/*------------------------------------------------------------------------------------
Function: glslProgram::GetOrCreateUniformBlock()
Description:
-------------------------------------------------------------------------------------*/
glslUniformBlock* glslGetOrCreateUniformBlock(const std::string& name)
{
    //Look for an appropriate uniform.
    UniformBlockList::iterator itr = _uniformBlockList.find(name);

    if(itr !=_uniformBlockList.end())
    {
        return itr->second;
    }

    //No uniform found matching name so create it..
    glslUniformBlock* uniform = new glslUniformBlock(name);
    _uniformBlockList[uniform->GetName()] = uniform;

    return uniform;
}

/*------------------------------------------------------------------------------------
Function: glslUniformBlock::glslUniformBlock()
Description:
-------------------------------------------------------------------------------------*/
glslUniformBlock::glslUniformBlock() :
    _numElements(0), 
    _ubo(0),
    _dirty(false),
    _base(-1)
{
}

/*------------------------------------------------------------------------------------
Function: glslUniformBlock::glslUniformBlock()
Description:
-------------------------------------------------------------------------------------*/
glslUniformBlock::glslUniformBlock(const std::string& name) :
    _numElements(0), 
    _ubo(0),
    _dirty(false),
    _base(-1)
{
    _name = name;
}

/*------------------------------------------------------------------------------------
Function: glslUniformBlock::AddUniform()
Description:
-------------------------------------------------------------------------------------*/
void glslUniformBlock::AddUniform(const std::string& name, int size, int byteoffset, int type)
{
    //Look for an appropriate uniform.
    UniformList::iterator itr = _uniformList.find(name);

    if(itr !=_uniformList.end())
    {
        return;
    }

    struct UniformParams* params = new struct UniformParams;

    params->type = type;
    params->size = size;
    params->offset = byteoffset;

    GLenum e = GetInternalArrayType((Type)type);

    if(e == GL_FLOAT)
    {
        if(_floatArray.empty())
            _floatArray.resize(_blocksize / sizeof(float));

        memset(&_floatArray[0], -1, _blocksize);
        params->index = byteoffset / sizeof(float);
    }
    else
    {
        if(_intArray.empty())
            _intArray.resize(_blocksize / sizeof(int));

        params->index = byteoffset / sizeof(int);
    }

    _uniformList[name] = params;
}

/*------------------------------------------------------------------------------------
Function: glslUniformBlock::Buffer()
Description:
-------------------------------------------------------------------------------------*/
void glslUniformBlock::Buffer()
{
    // Create a buffer object
    glGenBuffers(1, &_ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, _ubo);

    // Allocate room for this size
    glBufferData(GL_UNIFORM_BUFFER, _blocksize, nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

/*------------------------------------------------------------------------------------
Function: glslUniformBlock::GetInternalArrayType()
Description:
-------------------------------------------------------------------------------------*/
GLenum glslUniformBlock::GetInternalArrayType(Type t)
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
Function: glslUniformBlock::GetTypeNumComponents()
Description:
-------------------------------------------------------------------------------------*/
int glslUniformBlock::GetTypeNumComponents(Type t) const
{
    switch(t)
    {
    case FLOAT:
    case INT:
    case BOOL:
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
Function: glslUniformBlock::GetInternalArrayNumElements()
Description:
-------------------------------------------------------------------------------------*/
unsigned int glslUniformBlock::GetInternalArrayNumElements()
{
    if(GetNumElements() < 1 || GetType() == UNDEFINED) return 0;

    return GetNumElements() * GetTypeNumComponents(GetType());
}

/*------------------------------------------------------------------------------------
Function: glslUniformBlock::GetGlApiType()
Description:
-------------------------------------------------------------------------------------*/
glslUniformBlock::Type glslUniformBlock::GetGlApiType(Type t) const
{
    switch(t)
    {
    case BOOL:
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
Function: glslUniformBlock::SetElement()
Description:
-------------------------------------------------------------------------------------*/
void glslUniformBlock::SetElement(UniformParams* u, float f)
{
    int idx = u->index;

    if(_floatArray[idx] != f)
    {
        UniformBufferBind::BindBufferBase(_ubo, _base);
        glBufferSubData(GL_UNIFORM_BUFFER, u->offset, 4, &f);

        _floatArray[idx] = f;
   }
}

/*------------------------------------------------------------------------------------
Function: glslUniformBlock::SetElement()
Description:
-------------------------------------------------------------------------------------*/
void glslUniformBlock::SetElement(UniformParams* u, const Vec2f& v2)
{
    int idx = u->index;

    if(_floatArray[idx + 0] != v2.x() ||
       _floatArray[idx + 1] != v2.y())
    {
        UniformBufferBind::BindBufferBase(_ubo, _base);
        glBufferSubData(GL_UNIFORM_BUFFER, u->offset, 8, v2.ptr());
 
        _floatArray[idx + 0] = v2.x();
        _floatArray[idx + 1] = v2.y();
    }
}

/*------------------------------------------------------------------------------------
Function: glslUniformBlock::SetElement()
Description:
-------------------------------------------------------------------------------------*/
void glslUniformBlock::SetElement(UniformParams* u, const piVECTOR& v3)
{
    int idx = u->index;

    if(_floatArray[idx + 0] != v3.x ||
       _floatArray[idx + 1] != v3.y ||
       _floatArray[idx + 2] != v3.z)
    {
        UniformBufferBind::BindBufferBase(_ubo, _base);
        glBufferSubData(GL_UNIFORM_BUFFER, u->offset, 12, &v3);

        _floatArray[idx + 0] = v3.x;
        _floatArray[idx + 1] = v3.y;
        _floatArray[idx + 2] = v3.z;
   }
}

/*------------------------------------------------------------------------------------
Function: glslUniformBlock::SetElement()
Description:
-------------------------------------------------------------------------------------*/
void glslUniformBlock::SetElement(UniformParams* u, const Vec3d& v3)
{
    Vec3f pt(v3);

    int idx = u->index;

    if(_floatArray[idx + 0] != pt.x() ||
       _floatArray[idx + 1] != pt.y() ||
       _floatArray[idx + 2] != pt.z())
    {
        UniformBufferBind::BindBufferBase(_ubo, _base);
        glBufferSubData(GL_UNIFORM_BUFFER, u->offset, 12, pt.ptr());

        _floatArray[idx + 0] = pt.x();
        _floatArray[idx + 1] = pt.y();
        _floatArray[idx + 2] = pt.z();
    }
}

/*------------------------------------------------------------------------------------
Function: glslUniformBlock::SetElement()
Description:
-------------------------------------------------------------------------------------*/
void glslUniformBlock::SetElement(UniformParams* u, const Vec3f& v3)
{
    int idx = u->index;

    if(_floatArray[idx + 0] != v3.x() ||
       _floatArray[idx + 1] != v3.y() ||
       _floatArray[idx + 2] != v3.z())
    {
        UniformBufferBind::BindBufferBase(_ubo, _base);
        glBufferSubData(GL_UNIFORM_BUFFER, u->offset, 12, v3.ptr());

        _floatArray[idx + 0] = v3.x();
        _floatArray[idx + 1] = v3.y();
        _floatArray[idx + 2] = v3.z();
    }
}

/*------------------------------------------------------------------------------------
Function: glslUniformBlock::SetElement()
Description:
-------------------------------------------------------------------------------------*/
void glslUniformBlock::SetElement(UniformParams* u, const Vec4d& v4)
{
    Vec4f pt(v4);

    int idx = u->index;

    if(_floatArray[idx + 0] != pt.x() ||
       _floatArray[idx + 1] != pt.y() ||
       _floatArray[idx + 2] != pt.z() ||
       _floatArray[idx + 3] != pt.w())
    {
        UniformBufferBind::BindBufferBase(_ubo, _base);
        glBufferSubData(GL_UNIFORM_BUFFER, u->offset, 16, pt.ptr());

        _floatArray[idx + 0] = pt.x();
        _floatArray[idx + 1] = pt.y();
        _floatArray[idx + 2] = pt.z();
        _floatArray[idx + 3] = pt.w();
    }
}

/*------------------------------------------------------------------------------------
Function: glslUniformBlock::SetElement()
Description:
-------------------------------------------------------------------------------------*/
void glslUniformBlock::SetElement(UniformParams* u, const Vec4f& v4)
{
    int idx = u->index;

    if(_floatArray[idx + 0] != v4.x() ||
       _floatArray[idx + 1] != v4.y() ||
       _floatArray[idx + 2] != v4.z() ||
       _floatArray[idx + 3] != v4.w())
    {
        UniformBufferBind::BindBufferBase(_ubo, _base);
        glBufferSubData(GL_UNIFORM_BUFFER, u->offset, 16, v4.ptr());

        _floatArray[idx + 0] = v4.x();
        _floatArray[idx + 1] = v4.y();
        _floatArray[idx + 2] = v4.z();
        _floatArray[idx + 3] = v4.w();
    }
}

/*------------------------------------------------------------------------------------
Function: glslUniformBlock::SetElementData()
Description:
-------------------------------------------------------------------------------------*/
void glslUniformBlock::SetElementData(UniformParams* u, const void* data, int bytesize)
{
    logTimerBeginMacro();
    UniformBufferBind::BindBufferBase(_ubo, _base);
    glBufferSubData(GL_UNIFORM_BUFFER, u->offset, bytesize, data);
    logTimerEndMacro();
}

/*------------------------------------------------------------------------------------
Function: glslUniformBlock::SetElement()
Description:
-------------------------------------------------------------------------------------*/
void glslUniformBlock::SetElement(UniformParams* u, const piCOLORVALUE& v4)
{
    int idx = u->index;

    if(_floatArray[idx + 0] != v4.r ||
       _floatArray[idx + 1] != v4.g ||
       _floatArray[idx + 2] != v4.b ||
       _floatArray[idx + 3] != v4.a)
    {
        UniformBufferBind::BindBufferBase(_ubo, _base);
        glBufferSubData(GL_UNIFORM_BUFFER, u->offset, 16, &v4);

        _floatArray[idx + 0] = v4.r;
        _floatArray[idx + 1] = v4.g;
        _floatArray[idx + 2] = v4.b;
        _floatArray[idx + 3] = v4.a;
    }
}

/*------------------------------------------------------------------------------------
Function: glslUniformBlock::SetElement()
Description:
-------------------------------------------------------------------------------------*/
void glslUniformBlock::SetElement(UniformParams* u, const piMATRIX& m4)
{
    int idx = u->index;

    if(_floatArray[idx + 0] != m4._11 ||
       _floatArray[idx + 1] != m4._12 ||
       _floatArray[idx + 2] != m4._13 ||
       _floatArray[idx + 3] != m4._14 ||
       _floatArray[idx + 4] != m4._21 ||
       _floatArray[idx + 5] != m4._22 ||
       _floatArray[idx + 6] != m4._23 ||
       _floatArray[idx + 7] != m4._24 ||
       _floatArray[idx + 8] != m4._31 ||
       _floatArray[idx + 9] != m4._32 ||
       _floatArray[idx + 10] != m4._33 ||
       _floatArray[idx + 11] != m4._34 ||
       _floatArray[idx + 12] != m4._41 ||
       _floatArray[idx + 13] != m4._42 ||
       _floatArray[idx + 14] != m4._43 ||
       _floatArray[idx + 15] != m4._44)
    {
        UniformBufferBind::BindBufferBase(_ubo, _base);
        glBufferSubData(GL_UNIFORM_BUFFER, u->offset, 64, &m4);

        _floatArray[idx + 0] = m4._11;
        _floatArray[idx + 1] = m4._12;
        _floatArray[idx + 2] = m4._13;
        _floatArray[idx + 3] = m4._14;
        _floatArray[idx + 4] = m4._21;
        _floatArray[idx + 5] = m4._22;
        _floatArray[idx + 6] = m4._23;
        _floatArray[idx + 7] = m4._24;
        _floatArray[idx + 8] = m4._31;
        _floatArray[idx + 9] = m4._32;
        _floatArray[idx + 10] = m4._33;
        _floatArray[idx + 11] = m4._34;
        _floatArray[idx + 12] = m4._41;
        _floatArray[idx + 13] = m4._42;
        _floatArray[idx + 14] = m4._43;
        _floatArray[idx + 15] = m4._44;
    }
}

/*------------------------------------------------------------------------------------
Function: glslUniformBlock::SetElement()
Description:
-------------------------------------------------------------------------------------*/
void glslUniformBlock::SetElement(UniformParams* u, const piMATRIX3x3& m3)
{
    int idx = u->index;

    if(_floatArray[idx + 0] != m3._11 ||
       _floatArray[idx + 1] != m3._12 ||
       _floatArray[idx + 2] != m3._13 ||
       _floatArray[idx + 3] != m3._21 ||
       _floatArray[idx + 4] != m3._22 ||
       _floatArray[idx + 5] != m3._23 ||
       _floatArray[idx + 6] != m3._31 ||
       _floatArray[idx + 7] != m3._32 ||
       _floatArray[idx + 8] != m3._33)
    {
        UniformBufferBind::BindBufferBase(_ubo, _base);
        glBufferSubData(GL_UNIFORM_BUFFER, u->offset, 36, &m3);

        _floatArray[idx + 0] = m3._11;
        _floatArray[idx + 1] = m3._12;
        _floatArray[idx + 2] = m3._13;
        _floatArray[idx + 3] = m3._21;
        _floatArray[idx + 4] = m3._22;
        _floatArray[idx + 5] = m3._23;
        _floatArray[idx + 6] = m3._31;
        _floatArray[idx + 7] = m3._32;
        _floatArray[idx + 8] = m3._33;
    }
}

/*------------------------------------------------------------------------------------
Function: glslUniformBlock::SetElement()
Description:
-------------------------------------------------------------------------------------*/
void glslUniformBlock::SetElement(UniformParams* u, const Matrixf& m4)
{
    int idx = u->index;

    if(_floatArray[idx + 0] != m4(0, 0) ||
       _floatArray[idx + 1] != m4(0, 1) ||
       _floatArray[idx + 2] != m4(0, 2) ||
       _floatArray[idx + 3] != m4(0, 3) ||
       _floatArray[idx + 4] != m4(1, 0) ||
       _floatArray[idx + 5] != m4(1, 1) ||
       _floatArray[idx + 6] != m4(1, 2) ||
       _floatArray[idx + 7] != m4(1, 3) ||
       _floatArray[idx + 8] != m4(2, 0) ||
       _floatArray[idx + 9] != m4(2, 1) ||
       _floatArray[idx + 10] != m4(2, 2) ||
       _floatArray[idx + 11] != m4(2, 3) ||
       _floatArray[idx + 12] != m4(3, 0) ||
       _floatArray[idx + 13] != m4(3, 1) ||
       _floatArray[idx + 14] != m4(3, 2) ||
       _floatArray[idx + 15] != m4(3, 3))
    {
        UniformBufferBind::BindBufferBase(_ubo, _base);
        glBufferSubData(GL_UNIFORM_BUFFER, u->offset, 64, m4.ptr());

        _floatArray[idx + 0] = m4(0, 0);
        _floatArray[idx + 1] = m4(0, 1);
        _floatArray[idx + 2] = m4(0, 2);
        _floatArray[idx + 3] = m4(0, 3);
        _floatArray[idx + 4] = m4(1, 0);
        _floatArray[idx + 5] = m4(1, 1);
        _floatArray[idx + 6] = m4(1, 2);
        _floatArray[idx + 7] = m4(1, 3);
        _floatArray[idx + 8] = m4(2, 0);
        _floatArray[idx + 9] = m4(2, 1);
        _floatArray[idx + 10] = m4(2, 2);
        _floatArray[idx + 11] = m4(2, 3);
        _floatArray[idx + 12] = m4(3, 0);
        _floatArray[idx + 13] = m4(3, 1);
        _floatArray[idx + 14] = m4(3, 2);
        _floatArray[idx + 15] = m4(3, 3);
    }
}

/*------------------------------------------------------------------------------------
Function: glslUniformBlock::Set()
Description:
-------------------------------------------------------------------------------------*/
void glslUniformBlock::Set(const std::string& uniform, float f)
{
    UniformParams* u = GetUniform(uniform);
    
    if(u)
        SetElement(u, f);
}

/*------------------------------------------------------------------------------------
Function: glslUniformBlock::Set()
Description:
-------------------------------------------------------------------------------------*/
void glslUniformBlock::Set(const std::string& uniform, const Vec2f& v2)
{
    UniformParams* u = GetUniform(uniform);
    
    if(u)
        SetElement(u, v2);
}

/*------------------------------------------------------------------------------------
Function: glslUniformBlock::Set()
Description:
-------------------------------------------------------------------------------------*/
void glslUniformBlock::Set(const std::string& uniform, const piVECTOR& v3)
{
    UniformParams* u = GetUniform(uniform);
    
    if(u)
        SetElement(u, v3);
}

/*------------------------------------------------------------------------------------
Function: glslUniformBlock::Set()
Description:
-------------------------------------------------------------------------------------*/
void glslUniformBlock::Set(const std::string& uniform, const Vec3d& v3)
{
    UniformParams* u = GetUniform(uniform);
    
    if(u)
        SetElement(u, v3);
}

/*------------------------------------------------------------------------------------
Function: glslUniformBlock::Set()
Description:
-------------------------------------------------------------------------------------*/
void glslUniformBlock::Set(const std::string& uniform, const Vec3f& v3)
{
    UniformParams* u = GetUniform(uniform);
    
    if(u)
        SetElement(u, v3);
}

/*------------------------------------------------------------------------------------
Function: glslUniformBlock::Set()
Description:
-------------------------------------------------------------------------------------*/
void glslUniformBlock::Set(const std::string& uniform, const Vec4d& v4)
{
    UniformParams* u = GetUniform(uniform);
    
    if(u)
        SetElement(u, v4);
}

/*------------------------------------------------------------------------------------
Function: glslUniformBlock::Set()
Description:
-------------------------------------------------------------------------------------*/
void glslUniformBlock::Set(const std::string& uniform, const Vec4f& v4)
{
    UniformParams* u = GetUniform(uniform);
    
    if(u)
        SetElement(u, v4);
}

/*------------------------------------------------------------------------------------
Function: glslUniformBlock::SetData()
Description:
-------------------------------------------------------------------------------------*/
void glslUniformBlock::SetData(const std::string& uniform, const void* data, int bytesize)
{
    logTimerBeginMacro();

    UniformParams* u = GetUniform(uniform);
    
    if(u)
        SetElementData(u, data, bytesize);

    logTimerEndMacro();
}

/*------------------------------------------------------------------------------------
Function: glslUniformBlock::Set()
Description:
-------------------------------------------------------------------------------------*/
void glslUniformBlock::Set(const std::string& uniform, const piCOLORVALUE& v4)
{
    UniformParams* u = GetUniform(uniform);
    
    if(u)
        SetElement(u, v4);
}

/*------------------------------------------------------------------------------------
Function: glslUniformBlock::Set()
Description:
-------------------------------------------------------------------------------------*/
void glslUniformBlock::Set(const std::string& uniform, const piMATRIX& m4)
{
    UniformParams* u = GetUniform(uniform);
    
    if(u)
        SetElement(u, m4);
}

/*------------------------------------------------------------------------------------
Function: glslUniformBlock::Set()
Description:
-------------------------------------------------------------------------------------*/
void glslUniformBlock::Set(const std::string& uniform, const piMATRIX3x3& m3)
{
    UniformParams* u = GetUniform(uniform);
    
    if(u)
        SetElement(u, m3);
}

/*------------------------------------------------------------------------------------
Function: glslUniformBlock::Set()
Description:
-------------------------------------------------------------------------------------*/
void glslUniformBlock::Set(const std::string& uniform, const Matrixf& m4)
{
    UniformParams* u = GetUniform(uniform);
    
    if(u)
        SetElement(u, m4);
}

/*------------------------------------------------------------------------------------
Function: glslUniformBlock::ApplyUBO()
Description:
-------------------------------------------------------------------------------------*/
void glslUniformBlock::SetBlock(const void* data, int bytesize)
{
    logTimerBeginMacro();

    if(bytesize <= _blocksize)
    {
        UniformBufferBind::BindBufferBase(_ubo, _base);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, bytesize, data);
    }

    logTimerEndMacro();
}
