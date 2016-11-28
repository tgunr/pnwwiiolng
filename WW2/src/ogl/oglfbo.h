#pragma once

#include "pi/pi.h"

#include <vector>
#include <map>

#define ENABLE_FBO
//#undef ENABLE_FBO

struct fbotexture
{
    GLuint texture;
    GLuint w;
    GLuint h;
    GLuint attachment;
    GLenum target;
    GLenum lasttarget;
};

class CFbo
{
public:
    CFbo();
    virtual ~CFbo();

    LOGICAL initialize();
    LOGICAL checkStatus();
    LOGICAL activateTarget(SINT32 texture = -1, SINT32 depth = -1);
    LOGICAL activateDepthTextureFBO(GLuint fbo = 0);
    LOGICAL activateCubemapTarget(SINT32 texture = -1, SINT32 depth = -1, SINT32 face = 0);

    LOGICAL fboOn() {return m_supported;}

    void kill();
    void deletefbo(GLuint fbo);

    GLuint getTexture(SINT32 texture) {return m_textures[texture - 1].texture;}
    GLuint getDepth(SINT32 depth) {return m_depth[depth - 1];}
    GLuint getDepthTexture(UINT32 fbo) {return m_depthtexture[fbo];}

    GLenum getTarget(SINT32 texture) {return m_textures[texture - 1].target;}

    SINT32 createDepthBuffer(SINT32 depthtype, SINT32 w, SINT32 h, SINT32 bits);

    SINT32 createRenderTexture(GLint filtermin,
                               GLint filtermag,
                               GLint internalformat,
                               GLenum format,
                               GLenum type,
                               GLenum target,
                               SINT32 w,
                               SINT32 h);

    SINT32 createRenderCubemap(GLint filtermin,
                               GLint filtermag,
                               GLint internalformat,
                               GLenum format,
                               GLenum type,
                               SINT32 w,
                               SINT32 h);

    SINT32 createDepthTextureFBO(GLuint texid);

private:

    void deactivate();

    GLuint m_fbo;

    LOGICAL m_supported;
    LOGICAL m_isbound;

    GLenum m_lastTexture;

    std::vector<fbotexture> m_textures;
    std::vector<GLuint> m_depth;
    std::map<GLuint, GLuint> m_depthtexture;
};

void fboInitialize();
bool fboSupported();

CFbo* getFBO();

