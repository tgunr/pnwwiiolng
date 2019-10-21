#include "syscfg.h" // Path is project specific.
/*------------------------------------------------------------------------------------
Module: glslProgram.cpp
Description:
-------------------------------------------------------------------------------------*/
#include "gl/glew.h"
#include "shader/glslProgram.h"
#include "log/log.h"
#include "ogl/oglGlobalStates.h"
#include "shader/glsl.h"
#include "ogl/States.h"

// Use for shared uniforms blocks (Add if necessary)
enum
{
    base_matrixblock,
    base_lightblock0,
    base_lightblock1,
    base_lightblock2,
    base_lightblock3,
    base_ambientblock,
    base_fogblock,
    base_texturematrix,
    base_instanceblock,
    base_materialblock,
    base_testblock // for debugging
};

static std::set<std::string> BlockMap;
static std::map<std::string, int> BaseID;

using UniformBlockList = std::unordered_map<std::string, glslUniformBlock*>;
static UniformBlockList _uniformBlockList;

/*------------------------------------------------------------------------------------
Function: glslProgramInit
Description:
-------------------------------------------------------------------------------------*/
void glslProgramInit()
{
    if(!ogl::UseDeprecatedPath())
    {
        BaseID["MatrixBlock"] = base_matrixblock;
        BaseID["LightBlock0"] = base_lightblock0;
        BaseID["LightBlock1"] = base_lightblock1;
        BaseID["LightBlock2"] = base_lightblock2;
        BaseID["LightBlock3"] = base_lightblock3;
        BaseID["AmbientBlock"] = base_ambientblock;
        BaseID["FogBlock"] = base_fogblock;
        BaseID["TextureMatrixBlock"] = base_texturematrix;
        BaseID["InstanceBlock"] = base_instanceblock;
        BaseID["MaterialBlock"] = base_materialblock;
        BaseID["TestBlock"] = base_testblock;
    }
}

/*------------------------------------------------------------------------------------
Function: glslProgramKill
Description:
-------------------------------------------------------------------------------------*/
void glslProgramKill()
{
}

/*------------------------------------------------------------------------------------
Function: glslProgram::glslProgram()
Description:
-------------------------------------------------------------------------------------*/
glslProgram::glslProgram() :
    _needsLink(true),
    _isLinked(false),
    _firsttime(true),
    useke(false),
    useka(false),
    useks(false),
    usekd(false),
    useshininess(false),
    _programhandle(0),
    _uniformList()
{
}

/*------------------------------------------------------------------------------------
Function: glslProgram::~glslProgram()
Description:
-------------------------------------------------------------------------------------*/
glslProgram::~glslProgram()
{
}

/*------------------------------------------------------------------------------------
Function: glslProgram::DirtyProgram()
Description:
-------------------------------------------------------------------------------------*/
void glslProgram::DirtyProgram()
{
    _needsLink = true;
    _isLinked = false;
    _firsttime = true;
}

/*------------------------------------------------------------------------------------
Function: glslProgram::AddShader()
Description:
-------------------------------------------------------------------------------------*/
bool glslProgram::AddShader(glslShader* shader)
{
    if(!shader)
    {
        return false;
    }

    for(unsigned int i = 0; i < _shaderList.size(); i++)
    {
        if(shader == _shaderList[i])
        {
            return false;
        }
    }

    _shadersToAttach.push_back(shader);
    shader->AddProgramRef(this);

    _shaderList.push_back(shader);

    DirtyProgram();

    return true;
}

/*------------------------------------------------------------------------------------
Function: glslProgram::AddBindAttribLocation()
Description:
-------------------------------------------------------------------------------------*/
void glslProgram::AddBindAttribLocation(const std::string& name, GLuint index)
{
    _attribBindingList[name] = index;
    DirtyProgram();
}

/*------------------------------------------------------------------------------------
Function: glslProgram::GetBindAttribLocation()
Description:
-------------------------------------------------------------------------------------*/
unsigned int glslProgram::GetBindAttribLocation(const std::string& name)
{
    if(_attribBindingList.find(name) != _attribBindingList.end())
    {
        return _attribBindingList.find(name)->second;
    }

    return 0;
}

/*------------------------------------------------------------------------------------
Function: glslProgram::GetProgramInfoLog()
Description:
-------------------------------------------------------------------------------------*/
bool glslProgram::GetProgramInfoLog(std::string& result) const
{
    GLsizei bufLen = 0;        // length of buffer to allocate
    GLsizei strLen = 0;        // strlen GL actually wrote to buffer

    glGetProgramiv(_programhandle, GL_INFO_LOG_LENGTH, &bufLen);

    if(bufLen > 1)
    {
        GLchar* infoLog = new GLchar[bufLen] {};
        glGetProgramInfoLog(_programhandle, bufLen, &strLen, infoLog);

        if(strLen > 0) result = infoLog;

        delete [] infoLog;
    }

    return strLen > 0;
}

/*------------------------------------------------------------------------------------
Function: glslProgram::LinkProgram()
Description:
-------------------------------------------------------------------------------------*/
void glslProgram::LinkProgram()
{
    if(!_needsLink)
    {
        return;
    }

    if(!_programhandle)
    {
        _programhandle = glCreateProgram();
    }

    _needsLink = false;

    if(!TransformFeedbackVaryings.empty())
    {
	    glTransformFeedbackVaryings(_programhandle,
                                    TransformFeedbackVaryings.size(),
                                    &TransformFeedbackVaryings[0],
                                    GL_INTERLEAVED_ATTRIBS);
    }

    // Attach shaders to the program
    for(unsigned int i = 0; i < _shadersToAttach.size(); i++)
    {
        glAttachShader(_programhandle, _shadersToAttach[i]->GetHandle());
    }

    _shadersToAttach.clear();
    _uniformInfoMap.clear();
    _attribInfoMap.clear();
    _lastAppliedUniformList.clear();

    // Set any explicit vertex attribute bindings
    const AttribBindingList& bindlist = GetAttribBindingList();
    AttribBindingList::const_iterator itr = bindlist.begin();

    for(; itr != bindlist.end(); ++itr)
    {
        glBindAttribLocation(_programhandle, itr->second, itr->first.c_str());
    }

    // Link the glProgram
    GLint linked = GL_FALSE;
    glLinkProgram(_programhandle);

    glGetProgramiv(_programhandle, GL_LINK_STATUS, &linked);
    _isLinked = (linked == GL_TRUE);

    if(!_isLinked)
    {
        Write_Debug("glLinkProgram: FAILED");

        std::string infoLog;

        if(GetProgramInfoLog(infoLog))
        {
            Write_Log(LOG_OLD, LOG_WARN, "glLinkProgram: %s %s", GetName().c_str(), infoLog.c_str());
        }

        return;
    }
    else
    {
        std::string infoLog;
        if(GetProgramInfoLog(infoLog))
        {
            Write_Log(LOG_OLD, LOG_WARN, "glLinkProgram: %s %s", GetName().c_str(), infoLog.c_str());
        }
    }

    if(!ogl::UseDeprecatedPath())
    {
        int blocks;
        glGetProgramiv(_programhandle, GL_ACTIVE_UNIFORM_BLOCKS, &blocks);

        char blockname[256];
        char name[256];
        int base;
        int numUniforms;
        GLuint* indices;
        GLuint index;
        GLint blocksize;

        for(int i = 0; i < blocks; i++)
        {
            glGetActiveUniformBlockName(_programhandle, i, 256, nullptr, blockname);

            Write_Debug("BLOCKNAME: %s", blockname);

            if(BaseID.find(blockname) == BaseID.end())
            {
                Write_Debug("BLOCKNAME %s not found. Add to BaseID map in glslProgram.cpp", blockname);
                continue;
            }

            index = glGetUniformBlockIndex(_programhandle, blockname);
            base = BaseID.find(blockname)->second;

            // Associate base binding with this block
            glUniformBlockBinding(_programhandle, index, base);

            // Block already initialized (these are shared between shaders)
            if(BlockMap.find(blockname) != BlockMap.end())
            {
                continue;
            }

            glslUniformBlock* u = glslGetOrCreateUniformBlock(blockname);

            if(!u)
                continue;

            BlockMap.insert(blockname);

            glGetActiveUniformBlockiv(_programhandle, index, GL_UNIFORM_BLOCK_DATA_SIZE, &blocksize);

            u->SetBase(base);
            u->SetBlockSize(blocksize);

            glGetActiveUniformBlockiv(_programhandle, index, GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS, &numUniforms);

            indices = new GLuint[numUniforms];
            glGetActiveUniformBlockiv(_programhandle, index, GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES, (GLint*)indices);

            GLint byteoffset;
            GLint size;
            GLint type;

            for(int j = 0; j < numUniforms; j++)
            {
                glGetActiveUniformName(_programhandle, indices[j], 256, nullptr, name);

                glGetActiveUniformsiv(_programhandle, 1, (GLuint*)&indices[j], GL_UNIFORM_OFFSET, &byteoffset);
                glGetActiveUniformsiv(_programhandle, 1, (GLuint*)&indices[j], GL_UNIFORM_SIZE, &size);
                glGetActiveUniformsiv(_programhandle, 1, (GLuint*)&indices[j], GL_UNIFORM_TYPE, &type);

                Write_Debug("BLOCK UNIFORM %s: size = %d byteoffset = %d type = %x", name, size, byteoffset, type);

                u->AddUniform(name, size, byteoffset, type);
            }

            u->Buffer();

            delete[] indices;
        }
    }

    // Build _uniformInfoMap
    GLint numUniforms = 0;
    GLsizei maxLen = 0;
    glGetProgramiv(_programhandle, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxLen);
    glGetProgramiv(_programhandle, GL_ACTIVE_UNIFORMS, &numUniforms);

    if(numUniforms > 0 && maxLen > 1)
    {
        GLint size = 0;
        GLenum type = 0;
        GLchar* name = new GLchar[maxLen];

        for(GLint i = 0; i < numUniforms; i++)
        {
            glGetActiveUniform(_programhandle, i, maxLen, 0, &size, &type, name);
            GLint loc = glGetUniformLocation(_programhandle, name);

            if(loc != -1)
            {
                // F-nvidia. Drivers do not follow GL spec.
                // Uniform names associated with uniform blocks should have loc -1.
                // Shaders will need to have block members start with "b_"
                if(name[0] == 'b' && name[1] == '_')
                {
                    continue;
                }

                // Some ati drivers find array locations with '[0]'
                char* f_ati = strstr(name, "[0]");

                if(f_ati && strlen(f_ati) == 3) // avoid any "[0].name" possibilities
                {
                    *f_ati = 0;
                }

                _uniformInfoMap[name] = ActiveVarInfo(loc, type, size);
                Write_Log(LOG_OLD, LOG_INFO, "UNIFORM: name: %s loc: %d size: %d type: %d", name, loc, size, type);
            }
        }

        delete [] name;
    }

    // Build _attribInfoMap
    GLint numAttrib = 0;

    glGetProgramiv(_programhandle, GL_ACTIVE_ATTRIBUTES, &numAttrib );
    glGetProgramiv(_programhandle, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &maxLen );

    if(numAttrib > 0 && maxLen > 1)
    {
        GLint size = 0;
        GLenum type = 0;
        GLchar* name = new GLchar[maxLen];

        for(GLint i = 0; i < numAttrib; i++)
        {
            glGetActiveAttrib(_programhandle, i, maxLen, 0, &size, &type, name);
            GLint loc = glGetAttribLocation(_programhandle, name);

            if(loc != -1)
            {
                _attribInfoMap[name] = ActiveVarInfo(loc, type, size);
                Write_Log(LOG_OLD, LOG_INFO, "ATTRIBUTE: name: %s loc: %d size: %d", name, loc, size);
            }
        }

        delete [] name;
    }
}

/*------------------------------------------------------------------------------------
Function: glslProgram::IsFixedFunction()
Description:
-------------------------------------------------------------------------------------*/
bool glslProgram::IsFixedFunction() const
{
    return _shaderList.empty();
}

/*------------------------------------------------------------------------------------
Function: glslProgram::Apply()
Description:
-------------------------------------------------------------------------------------*/
void glslProgram::Apply(const ActiveVarInfo& info)
{
    if(info.uniform == nullptr)
        return;

    logTimerBeginMacro();

    GLint location = info.location;
    const glslUniform& uniform = *info.uniform;

    if(location >= 0)
    {
        if((unsigned int)location >= _lastAppliedUniformList.size())
        {
            _lastAppliedUniformList.resize(location + 1);
        }

        const glslUniform* lastAppliedUniform = _lastAppliedUniformList[location].first;

        // Apply if the uniforms are different
        if(lastAppliedUniform != &uniform)
        {
            // new attribute
            uniform.Apply(location);
            _lastAppliedUniformList[location].first = &uniform;
            _lastAppliedUniformList[location].second = uniform.GetModifiedCount();
        }
        // Apply if the uniform has been updated
        else if(_lastAppliedUniformList[location].second != uniform.GetModifiedCount())
        {
            // existing attribute has been modified
            uniform.Apply(location);
            _lastAppliedUniformList[location].first = &uniform;
            _lastAppliedUniformList[location].second = uniform.GetModifiedCount();
        }
    }

    logTimerEndMacro();
}

/*------------------------------------------------------------------------------------
Function: glslProgram::Apply()
Description:
-------------------------------------------------------------------------------------*/
void glslProgram::Apply(glslUniform& uniform)
{
    logTimerBeginMacro();

    GLint location = GetUniformLocation(uniform.GetName());

    if(location >= 0)
    {
        if((unsigned int)location >= _lastAppliedUniformList.size())
        {
            _lastAppliedUniformList.resize(location + 1);
        }

        const glslUniform* lastAppliedUniform = _lastAppliedUniformList[location].first;

        // Apply if the uniforms are different
        if(lastAppliedUniform != &uniform)
        {
            // new attribute
            uniform.Apply(location);
            _lastAppliedUniformList[location].first = &uniform;
            _lastAppliedUniformList[location].second = uniform.GetModifiedCount();
        }
        // Apply if the uniform has been updated
        else if(_lastAppliedUniformList[location].second != uniform.GetModifiedCount())
        {
            // existing attribute has been modified
            uniform.Apply(location);
            _lastAppliedUniformList[location].first = &uniform;
            _lastAppliedUniformList[location].second = uniform.GetModifiedCount();
        }
    }

    logTimerEndMacro();
}

/*------------------------------------------------------------------------------------
Function: glslProgram::CompileGLObjects()
Description:
-------------------------------------------------------------------------------------*/
void glslProgram::CompileGLObjects()
{
    if(IsFixedFunction()) return;

    for(unsigned int i = 0; i < _shaderList.size(); i++)
    {
        _shaderList[i]->CompileShader();
    }

    LinkProgram();
}

/*------------------------------------------------------------------------------------
Function: glslProgram::Apply()
Description:
-------------------------------------------------------------------------------------*/
void glslProgram::Apply()
{
    if(IsFixedFunction())
    {
        ProgramState::Bind(0);
        return;
    }

    if(_needsLink)
    {
        CompileGLObjects();
    }

    if(_isLinked)
    {
        if(_programhandle)
        {
            ProgramState::Bind(_programhandle);
        }
    }
    else
    {
        _firsttime = true;
        ProgramState::Bind(0);
    }
}

/*------------------------------------------------------------------------------------
Function: glslProgram::ApplyFirstTimeUpdate()
Description:
-------------------------------------------------------------------------------------*/
void glslProgram::ApplyFirstTimeUpdate()
{
    for(UniformList::iterator itr = _uniformList.begin(); itr != _uniformList.end(); ++itr)
    {
        glslUniform* uniform = itr->second;
        ActiveVarInfoMap::iterator itr2 = _uniformInfoMap.find(uniform->GetName());

        if(itr2 != _uniformInfoMap.end())
        {
            itr2->second.uniform = uniform;
        }
    }
}

/*------------------------------------------------------------------------------------
Function: glslProgram::RemoveUniformsIfNecessary()
Description:
-------------------------------------------------------------------------------------*/
void glslProgram::RemoveUniformsIfNecessary()
{
    glslUniform* u = nullptr;

    std::vector<ActiveVarInfoMap::iterator> remove;

    // Since these are only updated once, remove them to reduce loop count
    for(ActiveVarInfoMap::iterator itr = _uniformInfoMap.begin(); itr != _uniformInfoMap.end(); ++itr)
    {
        u = itr->second.uniform;

        // Only require one time apply. These do not change and can be removed.
        if(!u || u->OneTimeApply())
        {
            remove.push_back(itr);
        }
    }

    for(unsigned int i = 0; i < remove.size(); i++)
    {
        _uniformInfoMap.erase(remove[i]);
    }
}

/*------------------------------------------------------------------------------------
Function: glslProgram::ApplyUniformMap()
Description:
-------------------------------------------------------------------------------------*/
void glslProgram::ApplyUniformMap()
{
    if(_firsttime)
        ApplyFirstTimeUpdate();

    for(ActiveVarInfoMap::iterator itr = _uniformInfoMap.begin(); itr != _uniformInfoMap.end(); ++itr)
    {
        if(itr->second.count != itr->second.uniform->GetModifiedCount())
        {
            Apply(itr->second);
            itr->second.count = itr->second.uniform->GetModifiedCount();
        }
    }

    if(_firsttime)
        RemoveUniformsIfNecessary();

    _firsttime = false;
}

/*------------------------------------------------------------------------------------
Function: glslProgram::GetOrCreateUniform()
Description:
-------------------------------------------------------------------------------------*/
glslUniform* glslProgram::GetOrCreateUniform(const std::string &name,
                                             glslUniform::Type type,
                                             unsigned int numElements)
{
    //Look for an appropriate uniform.
    UniformList::iterator itr = _uniformList.find(name);

    if(itr !=_uniformList.end() && itr->second->GetType() == type)
    {
        return itr->second;
    }

    //No uniform found matching name so create it..
    glslUniform* uniform = new glslUniform(type, name, numElements);
    AddUniform(uniform);

    return uniform;
}

/*------------------------------------------------------------------------------------
Function: glslProgram::AddUniform()
Description:
-------------------------------------------------------------------------------------*/
void glslProgram::AddUniform(glslUniform* uniform)
{
    if(uniform)
    {
        UniformList::iterator itr = _uniformList.find(uniform->GetName());

        if(itr == _uniformList.end())
        {
            _uniformList[uniform->GetName()] = uniform;
        }
    }
}

/*------------------------------------------------------------------------------------
Function: glslProgram::RemoveUniform()
Description:
-------------------------------------------------------------------------------------*/
void glslProgram::RemoveUniform(const std::string &name)
{
	UniformList::iterator itr = _uniformList.find(name);

	if(itr != _uniformList.end())
	{
		_uniformList.erase(itr);
	}
}

/*------------------------------------------------------------------------------------
Function: glslProgram::GetUniform()
Description:
-------------------------------------------------------------------------------------*/
glslUniform* glslProgram::GetUniform(const std::string &name)
{
    UniformList::iterator itr = _uniformList.find(name);

    if(itr!=_uniformList.end())
    {
        return itr->second;
    }

    return nullptr;
}

