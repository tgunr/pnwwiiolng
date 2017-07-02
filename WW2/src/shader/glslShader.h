#pragma once

class glslProgram;

#include <string>
#include <set>
#include "osgImport/Referenced.h"

class glslShader : public Referenced
{
public:
    enum Type
    {
        VERTEX = GL_VERTEX_SHADER,
        FRAGMENT = GL_FRAGMENT_SHADER,
        GEOMETRY = GL_GEOMETRY_SHADER_EXT,
        UNDEFINED = -1
    };

    glslShader(Type type = UNDEFINED);
    glslShader(Type type, const std::string& source);

    bool AddProgramRef(glslProgram* program);
    bool RemoveProgramRef(glslProgram* program);
    bool GetShaderInfoLog(std::string& result) const;

    void DirtyShader();
    void SetShaderSource(const std::string& sourceText);
    void CompileShader();

    Type GetType() {return _type;}

    const char* GetTypename();

    const std::string& GetShaderSource() {return _shaderSource;}
    const std::string& GetName() {return _name;}
    void SetName(const std::string& name) {_name = name;}

    GLhandleARB GetHandle() {return _shaderhandle;}

    using ProgramSet = std::set<glslProgram*>;

protected:
    GLhandleARB _shaderhandle;

    std::string _shaderSource;
    std::string _name;

    ProgramSet _programSet;

    Type _type;

    bool _needsCompile;
    bool _isCompiled;
};

