#include "syscfg.h" // Path is project specific.
//////////////////////////////////////////////////////////////////////
// Module: fbo.cpp
// Description:
//////////////////////////////////////////////////////////////////////
#if USE_OSG
#else
#include "ogl/oglfbo.h"
#include "ogl/oglvars.h"
#include "include/vars.h"
#include "fio/fio.h"
#include "trn/trnsky.h"
#include "draw/sneaky.h"
#include "ogl/GLCounter.h"

#include <string>

static CFbo* g_fbo = NULL;

//////////////////////////////////////////////////////////////////////
// Function: getFBO
// Description:
//////////////////////////////////////////////////////////////////////
CFbo* getFBO()
{
    return g_fbo;
}

//////////////////////////////////////////////////////////////////////
// Function: fboSupported
// Description:
//////////////////////////////////////////////////////////////////////
bool fboSupported()
{
    if(!g_fbo)
    {
        return false;
    }

    return g_fbo->fboOn();
}

//////////////////////////////////////////////////////////////////////
// Function: fboInitialize
// Description:
//////////////////////////////////////////////////////////////////////
void fboInitialize()
{
    g_fbo = new CFbo;
    g_fbo->initialize();
}

//////////////////////////////////////////////////////////////////////
// Function: CFbo
// Description:
//////////////////////////////////////////////////////////////////////
CFbo::CFbo()
{
    m_supported = FALSE;
    m_isbound = FALSE;
    m_lastTexture = 0;

    m_fbo = 0;
}

//////////////////////////////////////////////////////////////////////
// Function: ~CFbo
// Description:
//////////////////////////////////////////////////////////////////////
CFbo::~CFbo()
{
}

//////////////////////////////////////////////////////////////////////
// Function: deletefbo
// Description:
//////////////////////////////////////////////////////////////////////
void CFbo::deletefbo(GLuint fbo)
{
    glDeleteFramebuffersEXT(1, &fbo);
}

//////////////////////////////////////////////////////////////////////
// Function: kill
// Description:
//////////////////////////////////////////////////////////////////////
void CFbo::kill()
{
    if(m_fbo)
    {
        glDeleteFramebuffersEXT(1, &m_fbo);
    }

    for(UINT32 j = 0; j < m_depth.size(); j++)
    {
        glDeleteRenderbuffersEXT(1, &m_depth[j]);
    }

    for(UINT32 i = 0; i < m_textures.size(); i++)
    {
        glDeleteTextures(1, &m_textures[i].texture);
    }

    m_textures.clear();
    m_depth.clear();
}

//////////////////////////////////////////////////////////////////////
// Function: createDepthBuffer
// Description:
//////////////////////////////////////////////////////////////////////
SINT32 CFbo::createDepthBuffer(SINT32 depthtype, SINT32 w, SINT32 h, SINT32 bits)
{
    if(bits <= 0)
    {
        return FALSE;
    }

    GLuint depth = 0;

    glGenRenderbuffersEXT(1, &depth);

    if(!depth)
    {
        return FALSE;
    }

    glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, depth);
    glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT,
                             bits > 16 ? GL_DEPTH_COMPONENT24 : GL_DEPTH_COMPONENT16,
                             w,
                             h);

    glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);

    m_depth.push_back(depth);

    return m_depth.size();
}

//////////////////////////////////////////////////////////////////////
// Function: createRenderCubemap
// Description:
//////////////////////////////////////////////////////////////////////
SINT32 CFbo::createRenderCubemap(GLint filtermin,
                                 GLint filtermag,
                                 GLint internalformat,
                                 GLenum format,
                                 GLenum type,
                                 SINT32 w,
                                 SINT32 h)
{
    glEnable(GL_TEXTURE_CUBE_MAP_ARB);

    fbotexture t;
    t.w = w;
    t.h = h;
    t.target = GL_TEXTURE_CUBE_MAP_ARB;
    t.attachment = GL_COLOR_ATTACHMENT0_EXT;

    gl_GenTextures(1, &t.texture);

    // Initialize color texture
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, t.texture);

    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);

    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MAG_FILTER, filtermag);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MIN_FILTER, filtermin);

	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    for(SINT32 i = 0; i < 6; i++)
    {
        GLenum target = GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB + i;

        glTexImage2D(target,
                     0,
                     internalformat,
                     t.w,
                     t.h,
                     0,
                     format,
                     type,
                     NULL);
    }

    GLenum e = glGetError();

    if(e != GL_NO_ERROR)
    {
        const GLubyte* s = gluErrorString(e);

        if(s)
        {
            Write_Log(LOG_OLD, LOG_INFO, (const SCHAR*)s);
        }

        return -1;
    }

    m_textures.push_back(t);

    glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, 0);
    glDisable(GL_TEXTURE_CUBE_MAP_ARB);

    return m_textures.size();
}

//////////////////////////////////////////////////////////////////////
// Function: createDepthTexture
// Description:
//////////////////////////////////////////////////////////////////////
SINT32 CFbo::createDepthTextureFBO(GLuint texid)
{
    GLuint fbo;

    glGenFramebuffersEXT(1, &fbo);

    //Activate the frame buffer object
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);

    //Attach our depth texture to the frame buffer object
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,
                              GL_DEPTH_ATTACHMENT_EXT,
                              GL_TEXTURE_2D,
                              texid,
                              0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, ddpDefaultFramebuffer());

    m_depthtexture[fbo] = texid;

    return fbo;
}

//////////////////////////////////////////////////////////////////////
// Function: createRenderTexture
// Description:
//////////////////////////////////////////////////////////////////////
SINT32 CFbo::createRenderTexture(GLint filtermin,
                                 GLint filtermag,
                                 GLint internalformat,
                                 GLenum format,
                                 GLenum type,
                                 GLenum target,
                                 SINT32 w,
                                 SINT32 h)
{
    glEnable(target);

    fbotexture t;
    t.w = w;
    t.h = h;
    t.target = target;
    t.attachment = GL_COLOR_ATTACHMENT0_EXT;

    gl_GenTextures(1, &t.texture);

    // Initialize color texture
    glBindTexture(target, t.texture);

    glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, filtermag);
    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, filtermin);

	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(target,
                 0,
                 internalformat,
                 t.w,
                 t.h,
                 0,
                 format,
                 type,
                 NULL);

    GLenum e = glGetError();

    if(e != GL_NO_ERROR)
    {
        const GLubyte* s = gluErrorString(e);

        if(s)
        {
            Write_Log(LOG_OLD, LOG_INFO, (const SCHAR*)s);
        }

        return -1;
    }

    m_textures.push_back(t);

    glBindTexture(target, 0);
    glDisable(target);

    return m_textures.size();
}

//////////////////////////////////////////////////////////////////////
// Function: activateCubemapTarget
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL CFbo::activateCubemapTarget(SINT32 texture, SINT32 depth, SINT32 face)
{
    if(!m_supported)
    {
        return FALSE;
    }

    if(texture - 1 < 0 && depth - 1 < 0)
    {
        deactivate();

        return TRUE;
    }

    if(!m_isbound)
    {
        glBindTexture(m_textures[texture - 1].target, 0);
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_fbo);

        m_isbound = TRUE;
    }

    if(texture > 0)
    {
        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,
                                  m_textures[texture - 1].attachment,
                                  GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB + face,
                                  m_textures[texture - 1].texture,
                                  0);
    }
    else
    {
        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,
                                  m_textures[m_lastTexture - 1].attachment,
                                  GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB + face,
                                  0,
                                  0);
    }

    if(depth >= 0)
    {
        glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,
                                     GL_DEPTH_ATTACHMENT_EXT,
                                     GL_RENDERBUFFER_EXT,
                                     m_depth[depth - 1]);

        piClearTargets(piCLEAR_ZBUFFER);
    }
    else
    {
        glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,
                                     GL_DEPTH_ATTACHMENT_EXT,
                                     GL_RENDERBUFFER_EXT,
                                     0);
    }

    if(!checkStatus())
    {
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, ddpDefaultFramebuffer());
        return FALSE;
    }

    m_lastTexture = texture;
    m_textures[texture - 1].lasttarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB + face;

    glViewport(0, 0, m_textures[texture - 1].w, m_textures[texture - 1].h);

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: activateTarget
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL CFbo::activateTarget(SINT32 texture, SINT32 depth)
{
    if(!m_supported)
    {
        return FALSE;
    }

    if(texture - 1 < 0 && depth - 1 < 0)
    {
        deactivate();

        return TRUE;
    }

    if(!m_isbound)
    {
        glBindTexture(m_textures[texture - 1].target, 0);
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_fbo);

        m_isbound = TRUE;
    }

    if(texture > 0)
    {
        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,
                                  m_textures[texture - 1].attachment,
                                  m_textures[texture - 1].target,
                                  m_textures[texture - 1].texture,
                                  0);
    }
    else
    {
        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,
                                  m_textures[m_lastTexture - 1].attachment,
                                  m_textures[m_lastTexture - 1].target,
                                  0,
                                  0);
    }

    if(depth >= 0)
    {
        glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,
                                     GL_DEPTH_ATTACHMENT_EXT,
                                     GL_RENDERBUFFER_EXT,
                                     m_depth[depth - 1]);

        piClearTargets(piCLEAR_ZBUFFER);
    }
    else
    {
        glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,
                                     GL_DEPTH_ATTACHMENT_EXT,
                                     GL_RENDERBUFFER_EXT,
                                     0);
    }

    if(!checkStatus())
    {
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, ddpDefaultFramebuffer());
        return FALSE;
    }

    m_lastTexture = texture;
    m_textures[texture - 1].lasttarget = m_textures[texture - 1].target;

    glViewport(0, 0, m_textures[texture - 1].w, m_textures[texture - 1].h);

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: activateDepthTexture
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL CFbo::activateDepthTextureFBO(GLuint fbo)
{
    if(!m_supported)
    {
        return FALSE;
    }

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);

    if(!checkStatus())
    {
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, ddpDefaultFramebuffer());
        return FALSE;
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: deactivate
// Description:
//////////////////////////////////////////////////////////////////////
void CFbo::deactivate()
{
    if(!m_supported || !m_isbound)
    {
        return;
    }

    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,
                              GL_COLOR_ATTACHMENT0_EXT,
                              m_textures[m_lastTexture - 1].lasttarget,
                              0,
                              0);

    glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,
                                 GL_DEPTH_ATTACHMENT_EXT,
                                 GL_RENDERBUFFER_EXT,
                                 0);

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, ddpDefaultFramebuffer());
    m_isbound = FALSE;

    glViewport(0, 0, piGetWidth(), piGetHeight());
}

//////////////////////////////////////////////////////////////////////
// Function: initialize
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL CFbo::initialize()
{
#ifndef ENABLE_FBO
    m_supported = FALSE;
    return TRUE;
#endif

    if(!GLEW_EXT_framebuffer_object)
    {
        const SCHAR* s = "[CFbo::initialize] No frame buffer support for FBO";
        Write_Log(LOG_OLD, LOG_INFO, s);
        m_supported = FALSE;
        return FALSE;
    }

    m_supported = GLEW_ARB_fragment_program;

    if(!m_supported)
    {
        const SCHAR* s = "[CFbo::initialize] No shader support for FBO";
        Write_Log(LOG_OLD, LOG_INFO, s);
        return FALSE;
    }

    glGenFramebuffersEXT(1, &m_fbo);

    if(!m_fbo)
    {
        const SCHAR* s = "[CFbo::initialize] Failed to create Framebuffer Object";
        Write_Log(LOG_OLD, LOG_INFO, s);
        m_supported = FALSE;
        return FALSE;
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: checkStatus
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL CFbo::checkStatus()
{
    if(!m_supported)
    {
        return FALSE;
    }

    GLenum e = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);

    switch(e)
    {
    case GL_FRAMEBUFFER_COMPLETE_EXT:
        {
            //Write_Log(LOG_OLD, LOG_INFO, "GL_FRAMEBUFFER_COMPLETE_EXT");
            return TRUE;
        }
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
        {
            Write_Log(LOG_OLD, LOG_INFO, "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT");
            return FALSE;
        }
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
        {
            Write_Log(LOG_OLD, LOG_INFO, "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT");
            return FALSE;
        }
    case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
        {
            Write_Log(LOG_OLD, LOG_INFO, "GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT");
            return FALSE;
        }
    case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
        {
            Write_Log(LOG_OLD, LOG_INFO, "GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT");
            return FALSE;
        }
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
        {
            Write_Log(LOG_OLD, LOG_INFO, "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT");
            return FALSE;
        }
    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
        {
            Write_Log(LOG_OLD, LOG_INFO, "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT");
            return FALSE;
        }
    case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
        {
            Write_Log(LOG_OLD, LOG_INFO, "GL_FRAMEBUFFER_UNSUPPORTED_EXT");
            return FALSE;
        }
    }

    // Should never get here
    return FALSE;
}
#endif
