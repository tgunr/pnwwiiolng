#pragma once

#include <string>
#include <vector>
#include <float.h>
#include <math.h>

#include "pi/pi.h"
#include "vec4f.h"

class Matrixf;
class Matrixd;
class Vec2f;
class Vec3d;
class Vec4d;
class Vec3f;

class glslUniform
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
        SAMPLER_1D = GL_SAMPLER_1D,
        SAMPLER_2D = GL_SAMPLER_2D,
        SAMPLER_3D = GL_SAMPLER_3D,
        SAMPLER_CUBE = GL_SAMPLER_CUBE,
        SAMPLER_1D_SHADOW = GL_SAMPLER_1D_SHADOW,
        SAMPLER_2D_SHADOW = GL_SAMPLER_2D_SHADOW,

        SAMPLER_1D_ARRAY  = GL_SAMPLER_1D_ARRAY_EXT,
        SAMPLER_2D_ARRAY  = GL_SAMPLER_2D_ARRAY_EXT,
        SAMPLER_1D_ARRAY_SHADOW = GL_SAMPLER_1D_ARRAY_SHADOW_EXT,
        SAMPLER_2D_ARRAY_SHADOW = GL_SAMPLER_2D_ARRAY_SHADOW_EXT,

        // TODO the following must be integrated fully here and Uniform.cpp
        FLOAT_MAT2x3 = GL_FLOAT_MAT2x3,
        FLOAT_MAT2x4 = GL_FLOAT_MAT2x4,
        FLOAT_MAT3x2 = GL_FLOAT_MAT3x2,
        FLOAT_MAT3x4 = GL_FLOAT_MAT3x4,
        FLOAT_MAT4x2 = GL_FLOAT_MAT4x2,
        FLOAT_MAT4x3 = GL_FLOAT_MAT4x3,
        SAMPLER_BUFFER = GL_SAMPLER_BUFFER_EXT,
        SAMPLER_CUBE_SHADOW = GL_SAMPLER_CUBE_SHADOW_EXT,
        UNSIGNED_INT_VEC2 = GL_UNSIGNED_INT_VEC2_EXT,
        UNSIGNED_INT_VEC3 = GL_UNSIGNED_INT_VEC3_EXT,
        UNSIGNED_INT_VEC4 = GL_UNSIGNED_INT_VEC4_EXT,
        INT_SAMPLER_1D = GL_INT_SAMPLER_1D_EXT,
        INT_SAMPLER_2D = GL_INT_SAMPLER_2D_EXT,
        INT_SAMPLER_3D = GL_INT_SAMPLER_3D_EXT,
        INT_SAMPLER_CUBE = GL_INT_SAMPLER_CUBE_EXT,
        INT_SAMPLER_2D_RECT = GL_INT_SAMPLER_2D_RECT_EXT,
        INT_SAMPLER_1D_ARRAY = GL_INT_SAMPLER_1D_ARRAY_EXT,
        INT_SAMPLER_2D_ARRAY = GL_INT_SAMPLER_2D_ARRAY_EXT,
        INT_SAMPLER_BUFFER = GL_INT_SAMPLER_BUFFER_EXT,
        UNSIGNED_INT_SAMPLER_1D = GL_UNSIGNED_INT_SAMPLER_1D_EXT,
        UNSIGNED_INT_SAMPLER_2D = GL_UNSIGNED_INT_SAMPLER_2D_EXT,
        UNSIGNED_INT_SAMPLER_3D = GL_UNSIGNED_INT_SAMPLER_3D_EXT,
        UNSIGNED_INT_SAMPLER_CUBE = GL_UNSIGNED_INT_SAMPLER_CUBE_EXT,
        UNSIGNED_INT_SAMPLER_2D_RECT = GL_UNSIGNED_INT_SAMPLER_2D_RECT_EXT,
        UNSIGNED_INT_SAMPLER_1D_ARRAY = GL_UNSIGNED_INT_SAMPLER_1D_ARRAY_EXT,
        UNSIGNED_INT_SAMPLER_2D_ARRAY = GL_UNSIGNED_INT_SAMPLER_2D_ARRAY_EXT,
        UNSIGNED_INT_SAMPLER_BUFFER = GL_UNSIGNED_INT_SAMPLER_BUFFER_EXT,

        UNDEFINED = 0x0
    };

    glslUniform();
    glslUniform(Type type, const std::string& name, int numElements = 1, bool global = false);

    inline void dirty() {++_modifiedCount;}

    void SetOneTimeApply(bool flag) {_oneTimeApply = flag;}
    void SetGlobal(bool flag) {global = flag;}
    void SetName(const std::string& name);
    void SetNumElements(unsigned int numElements);
    void AllocateDataArray();
    void Apply(GLint location) const;
    inline bool isDirty()
    {
        return _modifiedCount != _oldModifiedCount;
    }
    inline bool IsGlobal()
    {
        return global;
    }
    bool SetType(Type t);
    bool SetArray(std::vector<float>& array);
    bool SetArray(std::vector<int>& array);
    bool IsScalar() const {return _numElements == 1;}
    bool IsCompatibleType(Type t);
    bool SetElement(unsigned int index, const piUV& v2);
    bool SetElement(unsigned int index, const Vec2f& v2);
    bool SetElement(unsigned int index, const piVECTOR& v3);
    bool SetElement(unsigned int index, const Vec3f& v3);
    bool SetElement(unsigned int index, const Vec3d& v3);
    bool SetElement(unsigned int index, const Vec4d& v4);
    bool SetElement(unsigned int index, const Vec4f& v4);
    bool SetElement(unsigned int index, const piQUAT& v4);
    bool SetElement(unsigned int index, const piCOLORVALUE& v4);
    bool SetElement(unsigned int index, const piMATRIX& m4);
    bool SetElement(unsigned int index, const piMATRIX3x3& m3);
    bool SetElement(unsigned int index, const Matrixf& m4);
    bool SetElement(unsigned int index, int i);
    bool SetElement(unsigned int index, int i0, int i1);
    bool SetElement(unsigned int index, int i0, int i1, int i2);
    bool SetElement(unsigned int index, int i0, int i1, int i2, int i3);
    bool SetElement(unsigned int index, bool i);
    bool SetElement(unsigned int index, bool i0, bool i1);
    bool SetElement(unsigned int index, bool i0, bool i1, bool i2);
    bool SetElement(unsigned int index, bool i0, bool i1, bool i2, bool i3);
    bool SetElement(unsigned int index, float f0);
    bool SetElement(unsigned int index, float f0, float f1);
    bool SetElement(unsigned int index, float f0, float f1, float f2);
    bool SetElement(unsigned int index, float f0, float f1, float f2, float f3);

    bool Set(const piUV& v2);
    bool Set(const piVECTOR& v3);
    bool Set(const piQUAT& v4);
    bool Set(const piCOLORVALUE& v4);
    bool Set(const piMATRIX& m4);
    bool Set(const piMATRIX3x3& m3);
    bool Set(const Matrixf& m4);
    bool Set(const Vec2f& v2);
    bool Set(const Vec3f& v3);
    bool Set(const Vec3d& v3);
    bool Set(const Vec4d& v4);
    bool Set(const Vec4f& v4);
    bool Set(int i);
    bool Set(int i0, int i1);
    bool Set(int i0, int i1, int i2);
    bool Set(int i0, int i1, int i2, int i3);
    bool Set(bool b);
    bool Set(bool b0, bool b1);
    bool Set(bool b0, bool b1, bool b2);
    bool Set(bool b0, bool b1, bool b2, bool b3);
    bool Set(float f0);
    bool Set(float f0, float f1);
    bool Set(float f0, float f1, float f2);
    bool Set(float f0, float f1, float f2, float f3);

    bool OneTimeApply() {return _oneTimeApply;}

    unsigned int GetInternalArrayNumElements();
    unsigned int GetNumElements() const {return _numElements;}
    unsigned int GetModifiedCount() const {return _modifiedCount;}

    const char* GetTypename(Type t);
    const std::string& GetName() const {return _name;}

    int GetTypeNumComponents(Type t) const;

    GLenum GetInternalArrayType(Type t);

    Type GetGlApiType(Type t) const;
    Type GetType() const {return _type;}

protected:
    unsigned int _numElements;
    unsigned int _modifiedCount;
    mutable unsigned int _oldModifiedCount;

    bool _oneTimeApply;
    bool global;

    std::string _name;

    std::vector<float> _floatArray;
    std::vector<int> _intArray;

    Type _type;
};

