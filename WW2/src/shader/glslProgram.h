#pragma once

#include "shader/glslShader.h"
#include "shader/glslUniform.h"
#include "shader/glslUniformBlock.h"
#include "osgImport/Referenced.h"
#include "osgImport/ref_ptr.h"
#include "ogl/oglGlobalStates.h"
#include "ogl/oglVertexBuffer.h"

#include <vector>
#include "wwii/wwiiHash.h"

struct ActiveVarInfo
{
    ActiveVarInfo() :
        location(-1),
        type(glslUniform::UNDEFINED),
        size(-1),
        count(0)
    {
    }

    ActiveVarInfo(GLint l, GLenum t, GLint s) :
        uniform(nullptr),
        location(l),
        type(t),
        size(s),
        count(0)
    {
    }

    glslUniform* uniform;

    GLuint location;
    GLenum type;
    GLint size;
    GLuint count;
};

class glslProgram : public Referenced
{
public:
    glslProgram();
    virtual ~glslProgram();

    struct eqstr
    {
        bool operator()(const char* s1, const char* s2) const
        {
            return strcmp(s1, s2) < 0;
        }
    };

    void DirtyProgram();
    void LinkProgram();
    void AddBindAttribLocation(const std::string& name, GLuint index);
    void SetName(const char* name) {_name = name;}
    void Apply();
    void Apply(glslUniform& uniform);
    void Apply(const ActiveVarInfo& info);

    void ApplyUniformMap();
    void ApplyFirstTimeUpdate();
    void CompileGLObjects();
    void AddUniform(glslUniform* uniform);
    void SetUseKa(bool ka) {useka = ka;}
    void SetUseKs(bool ks) {useks = ks;}
    void SetUseKd(bool kd) {usekd = kd;}
    void SetUseKe(bool ke) {useke = ke;}
    void SetUseShininess(bool shininess) {useshininess = shininess;}
    void AddTransformFeedbackVarying(const char* p) {TransformFeedbackVaryings.push_back(p);}

    void RemoveUniform(const char* name);

    bool AddShader(glslShader* shader);
    bool GetProgramInfoLog(std::string& result) const;
    bool IsFixedFunction() const;
    bool IsLinked() {return _isLinked;}

    GLhandleARB GetHandle() {return _programhandle;}

    typedef std::vector<ref_ptr<glslShader> > ShaderList;

	using ActiveVarInfoMap = std::unordered_map<std::string, ActiveVarInfo>;

    using AttribBindingList = std::unordered_map<std::string, GLuint>;

#if platOS == OS_MACOS
    struct eqs
    {
        bool operator()(const char* s1, const char* s2) const
        {
            return strcmp(s1, s2) == 0;
        }
    };

    using UniformList = std::unordered_map<const char*, glslUniform*, std::hash<const char*>, eqs >;
#else
    using UniformList = std::unordered_map<const char*, glslUniform*, std::hash_compare<const char*, eqstr> >;
#endif

	using UniformModifiedCountPair = std::pair<const glslUniform*, unsigned int>;
	using LastAppliedUniformList = std::vector<UniformModifiedCountPair>;

    const AttribBindingList& GetAttribBindingList() const {return _attribBindingList;}

    inline bool UseKe() {return useke;}
    inline bool UseKd() {return usekd;}
    inline bool UseKs() {return useks;}
    inline bool UseKa() {return useka;}
    inline bool UseShininess() {return useshininess;}

     inline GLint GetUniformLocation(const std::string& name) const
    {
        ActiveVarInfoMap::const_iterator itr = _uniformInfoMap.find(name);
        return itr != _uniformInfoMap.end() ? itr->second.location : -1;
    }

    inline GLint GetAttribLocation(const std::string& name) const
    {
        ActiveVarInfoMap::const_iterator itr = _attribInfoMap.find(name);
        return itr != _attribInfoMap.end() ? itr->second.location : -1;
    }

    inline const std::string& GetName() const {return _name;}

    glslUniform* GetOrCreateUniform(const char* name, glslUniform::Type type, unsigned int numElements = 1);

    glslUniform* GetUniform(const char* name);

    unsigned int GetBindAttribLocation(const std::string& name);

protected:

    void RemoveUniformsIfNecessary();

    bool _needsLink;
    bool _isLinked;
    bool _firsttime;
    bool useke;
    bool useka;
    bool useks;
    bool usekd;
    bool useshininess;

    LastAppliedUniformList _lastAppliedUniformList;

    std::string _name;
    std::vector<const char*> TransformFeedbackVaryings;

    GLhandleARB _programhandle;

    ShaderList _shaderList;
    ShaderList _shadersToAttach;

    UniformList _uniformList;

    AttribBindingList _attribBindingList;

    ActiveVarInfoMap _uniformInfoMap;
    ActiveVarInfoMap _attribInfoMap;
};

