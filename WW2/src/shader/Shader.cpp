#include <syscfg.h> // Path is project specific.
//////////////////////////////////////////////////////////////////////
// Module: Shader.cpp
// Description:
//////////////////////////////////////////////////////////////////////
#include "shader/Shader.h"

#include "fio/fio.h"
#include "mem/mem.h"
#include "ogl/oglvars.h"

#ifdef _DEBUG
#include "rn/rnmain.h"
#endif

#include <string>

#include "sys/sysWinISO.h"			// Must be the last thing you include

#if(1)
#define CG_VERSION      _T("1, 2, 0, 0")
#define CGGL_VERSION   _T("1, 2, 0, 0")
#endif

static void _cgErrorCallback();

static LOGICAL g_bError = FALSE;

extern void GetNamedFileVersion(const TCHAR **, const TCHAR*);

// static variables
LOGICAL CWW2Shader::m_lock;

//////////////////////////////////////////////////////////////////////
// Function: CWW2Shader
// Description:
//////////////////////////////////////////////////////////////////////
CWW2Shader::CWW2Shader()
{
    cgSetErrorCallback(_cgErrorCallback);

    m_cgContext = (CGcontext)0;
    m_cgVprofile = (CGprofile)0;
    m_cgFprofile = (CGprofile)0;

    m_shadername[0] = 0;
}

//////////////////////////////////////////////////////////////////////
// Function: CWW2Shader
// Description:
//////////////////////////////////////////////////////////////////////
CWW2Shader::CWW2Shader(const SCHAR* shader, SINT32 shaderType)
{
    cgSetErrorCallback(_cgErrorCallback);

    m_cgContext = (CGcontext)0;
    m_cgVprofile = (CGprofile)0;
    m_cgFprofile = (CGprofile)0;

    try
    {
        loadProgram(shader, shaderType);

        if(m_cgPgm <= 0)
        {
            throw "[CWW2Shader] FAILED to load program";
        }

    }
    catch(const SCHAR* msg)
    {
        Write_Log(LOG_OLD, LOG_WARN, msg);
    }
}

//////////////////////////////////////////////////////////////////////
// Function: ~CWW2Shader
// Description:
//////////////////////////////////////////////////////////////////////
CWW2Shader::~CWW2Shader()
{
 #if platOS != OS_MACOS // this gets call from static destructor long after everything has been uninitialized
	if(isProgram())
    {
        killProgram();
    }

    if(cgIsContext(m_cgContext))
    {
        cgDestroyContext(m_cgContext);
        m_cgContext = NULL;
    }
#endif
}

//////////////////////////////////////////////////////////////////////
// Function: initialize
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL CWW2Shader::initialize()
{
    // Already initialized
    if(cgIsContext(m_cgContext))
    {
        return TRUE;
    }

    if(!isCorrectVersion())
    {
        return FALSE;
    }

    if(!GLEW_ARB_vertex_program)
    {
        return TRUE;
    }

    m_cgContext = cgCreateContext();

    if(!cgIsContext(m_cgContext))
    {
        return FALSE;
    }

    m_cgVprofile = cgGLGetLatestProfile(CG_GL_VERTEX);

#if 0 //def BETA_RELEASE
    SCHAR szBuff[64];

    sprintf(szBuff, "%s", cgGetProfileString(m_cgVprofile));

    Write_Debug("[CWW2Shader::Init] Latest supported vertex profile is %s", szBuff);
#endif

#if(0)
    m_cgVprofile = CG_PROFILE_ARBVP1;
#endif

    if(cgGLIsProfileSupported(m_cgVprofile))
    {
        cgGLSetOptimalOptions(m_cgVprofile);
    }

    m_cgFprofile = cgGLGetLatestProfile(CG_GL_FRAGMENT);

#if(0)
    m_cgFprofile = CG_PROFILE_ARBFP1;
#endif

#if 0 //def BETA_RELEASE
    sprintf(szBuff, "%s", cgGetProfileString(m_cgFprofile));

    Write_Debug("[CWW2Shader::Init] Latest supported fragment profile is %s", szBuff);
#endif

    if(cgGLIsProfileSupported(m_cgFprofile))
    {
        cgGLSetOptimalOptions(m_cgFprofile);
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: isCorrectVersion
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL CWW2Shader::isCorrectVersion()
{
#ifdef DEBUG_WW2
    return 1;
#endif

#if platOS == OS_WIN32
    const TCHAR* version = NULL;

    GetNamedFileVersion(&version, _T("cg.dll"));

    if(!version)
        return FALSE;

    if(ustrcmp(version, CG_VERSION))
    {
        Write_Debug("[CWW2Shader::isCorrectVersion] ** Incorrect cg.dll Version");
        return FALSE;
    }

    GetNamedFileVersion(&version, _T("cgGL.dll"));

    if(!version)
        return FALSE;

    if(ustrcmp(version, CGGL_VERSION))
    {
        Write_Debug("[CWW2Shader::isCorrectVersion] ** Incorrect cgGL.dll Version");
        return FALSE;
    }
#endif
    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: isProgram
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL CWW2Shader::isProgram()
{
    return (LOGICAL)cgIsProgram(m_cgPgm);
}

//////////////////////////////////////////////////////////////////////
// Function: killProgram
// Description:
//////////////////////////////////////////////////////////////////////
void CWW2Shader::killProgram()
{
    if(cgIsProgram(m_cgPgm))
    {
        cgDestroyProgram(m_cgPgm);
    }
}

//////////////////////////////////////////////////////////////////////
// Function: loadProgram
// Description: Assume all Cg entry points are named "main".
//////////////////////////////////////////////////////////////////////
void CWW2Shader::loadProgram(const SCHAR* program, SINT32 shaderType)
{
    m_cgPgm = 0;

    if(!cgIsContext(m_cgContext))
    {
        if(!initialize())
        {
            return;
        }
    }

    std::string path = "fx\\shaders\\";
    path += program;

    SINT32 handle = fioOpen(path.c_str());

    if(handle == -1)
    {
        return;
    }

    fioSeek(handle, 0L, SEEK_SET);
    SINT32 size = fioSeek(handle, 0, SEEK_END);
    fioSeek(handle, 0L, SEEK_SET);

    SCHAR* code = (SCHAR*)memAlloc(size + 1);

    if(code == NULL)
    {
        fioClose(handle);
        return;
    }

    memset(code, 0, size);

    fioRead(handle, code, size);

    fioClose(handle);

    code[size] = 0;

    m_cgProfile = shaderType == TYPE_VERTEX ? m_cgVprofile : m_cgFprofile;

    m_cgPgm = cgCreateProgram(m_cgContext,
                              CG_SOURCE,
                              code,
                              m_cgProfile,
                              "main",
                              NULL);

    memFree(code);

    if(!cgIsProgram(m_cgPgm))
    {
        Write_Debug("[CWW2Shader::LoadProgram] ** FAILED ** %s", program);
        return;
    }

    cgGLLoadProgram(m_cgPgm);

    if(g_bError)
    {
        g_bError = FALSE;
        killProgram();
        m_cgPgm = 0;
    }

    strncpy(m_shadername, program, 32);
}

//////////////////////////////////////////////////////////////////////
// Function: getParam
// Description:
//////////////////////////////////////////////////////////////////////
inline CGparameter CWW2Shader::getParam(const SCHAR* param)
{
    return cgGetNamedParameter(m_cgPgm, param);
}

//////////////////////////////////////////////////////////////////////
// Function: lockShader
// Description:
//////////////////////////////////////////////////////////////////////
void CWW2Shader::lockShader(LOGICAL lock)
{
    m_lock = lock;
}

//////////////////////////////////////////////////////////////////////
// Function: activate
// Description:
//////////////////////////////////////////////////////////////////////
void CWW2Shader::activate()
{
    if(m_lock)
    {
        return;
    }

    cgGLEnableProfile(m_cgProfile);
    cgGLBindProgram(m_cgPgm);
}

//////////////////////////////////////////////////////////////////////
// Function: deactivate
// Description:
//////////////////////////////////////////////////////////////////////
void CWW2Shader::deactivate()
{
    if(m_lock)
    {
        return;
    }

    cgGLDisableProfile(m_cgProfile);
}

//////////////////////////////////////////////////////////////////////
// Function: setModelViewProjectionMatrix
// Description:
//////////////////////////////////////////////////////////////////////
void CWW2Shader::setModelViewProjectionMatrix(const SCHAR* param)
{
    CGparameter cg = getParam(param);
    cgGLSetStateMatrixParameter(cg,
                                CG_GL_MODELVIEW_PROJECTION_MATRIX,
                                CG_GL_MATRIX_IDENTITY);
}

//////////////////////////////////////////////////////////////////////
// Function: setTextureMatrix
// Description:
//////////////////////////////////////////////////////////////////////
void CWW2Shader::setTextureMatrix(const SCHAR* param)
{
    CGparameter cg = getParam(param);
    cgGLSetStateMatrixParameter(cg,
                                CG_GL_TEXTURE_MATRIX,
                                CG_GL_MATRIX_IDENTITY);
}

//////////////////////////////////////////////////////////////////////
// Function: setModelViewMatrix
// Description:
//////////////////////////////////////////////////////////////////////
void CWW2Shader::setModelViewMatrix(const SCHAR* param)
{
    CGparameter cg = getParam(param);
    cgGLSetStateMatrixParameter(cg,
                                CG_GL_MODELVIEW_MATRIX,
                                CG_GL_MATRIX_IDENTITY);
}

//////////////////////////////////////////////////////////////////////
// Function: setModelViewITMatrix
// Description:
//////////////////////////////////////////////////////////////////////
void CWW2Shader::setModelViewITMatrix(const SCHAR* param)
{
    CGparameter cg = getParam(param);
    cgGLSetStateMatrixParameter(cg,
                                CG_GL_MODELVIEW_MATRIX,
                                CG_GL_MATRIX_INVERSE_TRANSPOSE);
}

//////////////////////////////////////////////////////////////////////
// Function: setMatrix
// Description:
//////////////////////////////////////////////////////////////////////
void CWW2Shader::setMatrix(const SCHAR* param, piMATRIX& m)
{
    CGparameter cg = getParam(param);
    cgGLSetMatrixParameterfr(cg, (const FLOAT32*)&m);
}

//////////////////////////////////////////////////////////////////////
// Function: setMatrix
// Description:
//////////////////////////////////////////////////////////////////////
void CWW2Shader::setMatrix(const SCHAR* param, const FLOAT32 m[16])
{
    CGparameter cg = getParam(param);
    cgGLSetMatrixParameterfr(cg, m);
}

//////////////////////////////////////////////////////////////////////
// Function: setParam4f
// Description:
//////////////////////////////////////////////////////////////////////
void CWW2Shader::setParam4f(const SCHAR* param, FLOAT32 x, FLOAT32 y, FLOAT32 z, FLOAT32 w)
{
    CGparameter cg = getParam(param);
    cgGLSetParameter4f(cg, x, y, z, w);
}

//////////////////////////////////////////////////////////////////////
// Function: setParam4f
// Description:
//////////////////////////////////////////////////////////////////////
void CWW2Shader::setParam4f(const SCHAR* param, piCOLORVALUE& color)
{
    CGparameter cg = getParam(param);
    cgGLSetParameter4f(cg, color.r, color.g, color.b, color.a);
}

//////////////////////////////////////////////////////////////////////
// Function: setParam4f
// Description:
//////////////////////////////////////////////////////////////////////
void CWW2Shader::setParam4f(const SCHAR* param, piVECTOR& v)
{
    CGparameter cg = getParam(param);
    cgGLSetParameter4f(cg, v.x, v.y, v.z, 1.0f);
}

//////////////////////////////////////////////////////////////////////
// Function: setParam4f
// Description:
//////////////////////////////////////////////////////////////////////
void CWW2Shader::setParam4f(const SCHAR* param, FLOAT32 x)
{
    CGparameter cg = getParam(param);
    cgGLSetParameter4f(cg, x, x, x, x);
}

//////////////////////////////////////////////////////////////////////
// Function: setParam4f
// Description:
//////////////////////////////////////////////////////////////////////
void CWW2Shader::setParam4f(const SCHAR* param, UINT32 color)
{
    CGparameter cg = getParam(param);

    FLOAT32 r = (FLOAT32)piRGBA_GETRED(color) * colorONEOVER255;
    FLOAT32 g = (FLOAT32)piRGBA_GETGREEN(color) * colorONEOVER255;
    FLOAT32 b = (FLOAT32)piRGBA_GETBLUE(color) * colorONEOVER255;
    FLOAT32 a = (FLOAT32)piRGBA_GETALPHA(color) * colorONEOVER255;

    cgGLSetParameter4f(cg, r, g, b, a);
}

//////////////////////////////////////////////////////////////////////
// Function: setParam3f
// Description:
//////////////////////////////////////////////////////////////////////
void CWW2Shader::setParam3f(const SCHAR* param, FLOAT32 x, FLOAT32 y, FLOAT32 z)
{
    CGparameter cg = getParam(param);
    cgGLSetParameter3f(cg, x, y, z);
}

//////////////////////////////////////////////////////////////////////
// Function: setParam3f
// Description:
//////////////////////////////////////////////////////////////////////
void CWW2Shader::setParam3f(const SCHAR* param, piVECTOR& v)
{
    CGparameter cg = getParam(param);
    cgGLSetParameter3f(cg, v.x, v.y, v.z);
}

//////////////////////////////////////////////////////////////////////
// Function: setParam3f
// Description:
//////////////////////////////////////////////////////////////////////
void CWW2Shader::setParam3f(const SCHAR* param, piCOLORVALUE& color)
{
    CGparameter cg = getParam(param);
    cgGLSetParameter3f(cg, color.r, color.g, color.b);
}

//////////////////////////////////////////////////////////////////////
// Function: setParam2f
// Description:
//////////////////////////////////////////////////////////////////////
void CWW2Shader::setParam2f(const SCHAR* param, FLOAT32 x, FLOAT32 y)
{
    CGparameter cg = getParam(param);
    cgGLSetParameter2f(cg, x, y);
}

//////////////////////////////////////////////////////////////////////
// Function: setParam1f
// Description:
//////////////////////////////////////////////////////////////////////
void CWW2Shader::setParam1f(const SCHAR* param, FLOAT32 x)
{
    CGparameter cg = getParam(param);
    cgGLSetParameter1f(cg, x);
}

//////////////////////////////////////////////////////////////////////
// Function: disableTexture
// Description:
//////////////////////////////////////////////////////////////////////
void CWW2Shader::disableTexture(const SCHAR* param)
{
    CGparameter cg = getParam(param);
    cgGLDisableTextureParameter(cg);
}

//////////////////////////////////////////////////////////////////////
// Function: setTexture
// Description: Use this when using game engine textures
//////////////////////////////////////////////////////////////////////
void CWW2Shader::setTexture(const SCHAR* param, SINT32 texture)
{
    LPTEXTURE t = piGetTextureData(texture);

    if(!t) return;

    CGparameter cg = getParam(param);
    cgGLSetTextureParameter(cg, t->genlist);
}

//////////////////////////////////////////////////////////////////////
// Function: SetTextureParameter2
// Description: Use this when using the texture name created
// from glGenTextures(#, genlist) (Engine can't handle certain
// texture creations...like cube maps, for example.)
//////////////////////////////////////////////////////////////////////
void CWW2Shader::setTexture2(const SCHAR* param, UINT32 genlist)
{
    CGparameter cg = getParam(param);
    cgGLSetTextureParameter(cg, genlist);
}

//////////////////////////////////////////////////////////////////////
// Function: enableTexture
// Description:
//////////////////////////////////////////////////////////////////////
void CWW2Shader::enableTexture(const SCHAR* param)
{
    CGparameter cg = getParam(param);
    cgGLEnableTextureParameter(cg);
}

//////////////////////////////////////////////////////////////////////
// Function: getMatrix
// Description:
//////////////////////////////////////////////////////////////////////
void CWW2Shader::getMatrix(const SCHAR* param, piMATRIX& m)
{
    CGparameter cg = getParam(param);
    cgGLGetMatrixParameterfr(cg, (GLfloat*)&m);
}

//////////////////////////////////////////////////////////////////////
// Function: _cgErrorCallback
// Description:
//////////////////////////////////////////////////////////////////////
void _cgErrorCallback()
{
    CGerror cgErr = cgGetError();

    if(cgErr != CG_NO_ERROR)
    {
        SCHAR szErr[256] = {0};
        static SINT32 count = 0;

        const SCHAR* p = cgGetErrorString(cgErr);

        strcpy(szErr, "[_cgErrorCallback] ");
        strcat(szErr, p);
        SCHAR s[256];
        sprintf(s, "%d %s", count++, szErr);
        Write_Debug(s);

        g_bError = TRUE;

#ifdef _DEBUG
    rnmainListText(s);
#endif

    }
}
