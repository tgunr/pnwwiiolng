#include <syscfg.h> // Path is project specific.
/*------------------------------------------------------------------------------------
Module: glslShader.cpp
Description:
-------------------------------------------------------------------------------------*/
#include "gl/glew.h"
#include "shader/glslShader.h"
#include "shader/glslProgram.h"
#include "log/log.h"
#include "spt/Random.h"

/*------------------------------------------------------------------------------------
Function: glslShader::glslShader()
Description:
-------------------------------------------------------------------------------------*/
glslShader::glslShader(Type type) :
    _type(type),
    _shaderhandle(0),
    _needsCompile(false),
    _isCompiled(false)
{
}

/*------------------------------------------------------------------------------------
Function: glslShader::glslShader()
Description:
-------------------------------------------------------------------------------------*/
glslShader::glslShader(Type type, const std::string& source) :
    _type(type),
    _shaderhandle(0),
    _needsCompile(false),
    _isCompiled(false)
{
    SetShaderSource(source);
}

/*------------------------------------------------------------------------------------
Function: glslShader::SetShaderSource()
Description:
-------------------------------------------------------------------------------------*/
void glslShader::SetShaderSource(const std::string& sourceText)
{
    _shaderSource = sourceText;

    DirtyShader();
}

/*------------------------------------------------------------------------------------
Function: glslShader::AddProgramRef()
Description:
-------------------------------------------------------------------------------------*/
bool glslShader::AddProgramRef(glslProgram* program)
{
    ProgramSet::iterator itr = _programSet.find(program);
    if(itr != _programSet.end()) return false;

    _programSet.insert(program);
    return true;
}

/*------------------------------------------------------------------------------------
Function: glslShader::RemoveProgramRef()
Description:
-------------------------------------------------------------------------------------*/
bool glslShader::RemoveProgramRef(glslProgram* program)
{
    ProgramSet::iterator itr = _programSet.find(program);
    if(itr == _programSet.end()) return false;

    _programSet.erase(itr);
    return true;
}

/*------------------------------------------------------------------------------------
Function: glslShader::DirtyShader()
Description:
-------------------------------------------------------------------------------------*/
void glslShader::DirtyShader()
{
    _needsCompile = true;

    ProgramSet::iterator itr = _programSet.begin();

    // Also mark Programs that depend on us as needing relink.
    for(; itr != _programSet.end(); ++itr)
    {
        (*itr)->DirtyProgram();
    }
}

/*------------------------------------------------------------------------------------
Function: glslShader::GetTypename()
Description:
-------------------------------------------------------------------------------------*/
const char* glslShader::GetTypename()
{
    switch(GetType())
    {
        case VERTEX:    return "VERTEX";
        case FRAGMENT:  return "FRAGMENT";
        case GEOMETRY:  return "GEOMETRY";
        default:        return "UNDEFINED";
    }
}

/*------------------------------------------------------------------------------------
Function: glslShader::GetShaderInfoLog()
Description:
-------------------------------------------------------------------------------------*/
bool glslShader::GetShaderInfoLog(std::string& result) const
{
    GLsizei bufLen = 0;        // length of buffer to allocate
    GLsizei strLen = 0;        // strlen GL actually wrote to buffer

    glGetShaderiv(_shaderhandle, GL_INFO_LOG_LENGTH, &bufLen);

    if(bufLen > 1)
    {
        GLchar* infoLog = new GLchar[bufLen];
        glGetShaderInfoLog(_shaderhandle, bufLen, &strLen, infoLog);

        if(strLen > 0) result = infoLog;

        delete [] infoLog;
    }

    return strLen > 0;
}

/*------------------------------------------------------------------------------------
Function: glslShader::CompileShader()
Description:
-------------------------------------------------------------------------------------*/

#ifndef ANTIHACKER
#define ANTIHACKER
#endif

static char getrandomdigit() { return GetRandom( '0', '9' ); }

void glslShader::CompileShader()
{
    if(!_needsCompile) return;
    _needsCompile = false;

    GLint compiled = GL_FALSE;
    const char* sourceText = GetShaderSource().c_str();

    if(!_shaderhandle)
    {
        _shaderhandle = glCreateShader(GetType());
    }

#ifdef ANTIHACKER
    // anti hacker replacement hack - add random text to shader
    std::vector<char> temp;
    temp.insert( temp.end(),&sourceText[0], &sourceText[strlen(sourceText)]);
    temp.push_back('/');
    temp.push_back('/');
    for ( int i=0; i<8; i++ )
    {
        temp.push_back( getrandomdigit() );
    }
    temp.push_back( 0 );
    const char* s = &temp[0];
    glShaderSource(_shaderhandle, 1, &s, nullptr);
#else
    glShaderSource(_shaderhandle, 1, &sourceText, nullptr);
#endif

    glCompileShader(_shaderhandle);
    glGetShaderiv(_shaderhandle, GL_COMPILE_STATUS, &compiled);

    _isCompiled = (compiled == GL_TRUE);

    if(!_isCompiled)
    {
        Write_Log(LOG_OLD, LOG_WARN, "ERROR: %s %s FALED", GetTypename(), GetName().c_str());

        std::string infoLog;

        if(GetShaderInfoLog(infoLog))
        {
            Write_Log(LOG_OLD, LOG_WARN, "%s", infoLog.c_str());
        }
    }
    else
    {
        std::string infoLog;

        if(GetShaderInfoLog(infoLog))
        {
            Write_Log(LOG_OLD, LOG_WARN, "%s", infoLog.c_str());
        }
    }
}
