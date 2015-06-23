#include <syscfg.h> // Path is project specific.
/* -*-c++-*- OpenSceneGraph - Copyright (C) 1998-2006 Robert Osfield
 *
 * This library is open source and may be redistributed and/or modified under
 * the terms of the OpenSceneGraph Public License (OSGPL) version 0.0 or
 * (at your option) any later version.  The full license is in LICENSE file
 * included with this distribution, and on the openscenegraph.org website.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * OpenSceneGraph Public License for more details.
*/

// initial FBO support written by Marco Jez, June 2005.

#include "osgImport/FrameBufferObject.h"
#include "osgImport/Texture2D.h"
#include "osgImport/TextureRectangle.h"

static buffered_object<ref_ptr<FBOExtensions> > s_extensions;

FBOExtensions* FBOExtensions::instance(unsigned contextID, bool createIfNotInitalized)
{
    if(!s_extensions[contextID].valid() && createIfNotInitalized) s_extensions[contextID] = new FBOExtensions(contextID);
    return s_extensions[contextID].get();
}

/**************************************************************************
 * FBOExtensions
 **************************************************************************/
#if(0)
#define LOAD_FBO_EXT(name)    setGLExtensionFuncPtr(name, (#name))
#endif

FBOExtensions::FBOExtensions(unsigned int contextID)
:   _supported(false)
#if(0)
    glBindRenderbufferEXT(0),
    glGenRenderbuffersEXT(0),
    glDeleteRenderbuffersEXT(0),
    glRenderbufferStorageEXT(0),
    glRenderbufferStorageMultisampleEXT(0),
    glRenderbufferStorageMultisampleCoverageNV(0),
    glBindFramebufferEXT(0),
    glDeleteFramebuffersEXT(0),
    glGenFramebuffersEXT(0),
    glCheckFramebufferStatusEXT(0),
    glFramebufferTexture1DEXT(0),
    glFramebufferTexture2DEXT(0),
    glFramebufferTexture3DEXT(0),
    glFramebufferTextureLayerEXT(0),
    glFramebufferRenderbufferEXT(0),
    glGenerateMipmapEXT(0),
    glBlitFramebufferEXT(0)
#endif
{
    if(!GLEW_EXT_framebuffer_object)
    {
        Write_Log(LOG_OLD, LOG_INFO, "!GLEW_EXT_framebuffer_object");
        return;
    }

#if(0)
    LOAD_FBO_EXT(glBindRenderbufferEXT);
    LOAD_FBO_EXT(glGenRenderbuffersEXT);
    LOAD_FBO_EXT(glDeleteRenderbuffersEXT);
    LOAD_FBO_EXT(glRenderbufferStorageEXT);
    LOAD_FBO_EXT(glBindFramebufferEXT);
    LOAD_FBO_EXT(glDeleteFramebuffersEXT);
    LOAD_FBO_EXT(glGenFramebuffersEXT);
    LOAD_FBO_EXT(glCheckFramebufferStatusEXT);
    LOAD_FBO_EXT(glFramebufferTexture1DEXT);
    LOAD_FBO_EXT(glFramebufferTexture2DEXT);
    LOAD_FBO_EXT(glFramebufferTexture3DEXT);
    LOAD_FBO_EXT(glFramebufferTextureLayerEXT);
    LOAD_FBO_EXT(glFramebufferRenderbufferEXT);
    LOAD_FBO_EXT(glGenerateMipmapEXT);
#endif

    _supported =
        glBindRenderbufferEXT != 0 &&
        glDeleteRenderbuffersEXT != 0 &&
        glGenRenderbuffersEXT != 0 &&
        glRenderbufferStorageEXT != 0 &&
        glBindFramebufferEXT != 0 &&
        glDeleteFramebuffersEXT != 0 &&
        glGenFramebuffersEXT != 0 &&
        glCheckFramebufferStatusEXT != 0 &&
        glFramebufferTexture1DEXT != 0 &&
        glFramebufferTexture2DEXT != 0 &&
        glFramebufferTexture3DEXT != 0 &&
        glFramebufferRenderbufferEXT != 0 &&
        glGenerateMipmapEXT != 0;

    Write_Log(LOG_OLD, LOG_INFO, "[FBOExtensions::FBOExtensions] glBindRenderbufferEXT = %x", glBindRenderbufferEXT);
    Write_Log(LOG_OLD, LOG_INFO, "[FBOExtensions::FBOExtensions] glDeleteRenderbuffersEXT = %x", glDeleteRenderbuffersEXT);
    Write_Log(LOG_OLD, LOG_INFO, "[FBOExtensions::FBOExtensions] glGenRenderbuffersEXT = %x", glGenRenderbuffersEXT);
    Write_Log(LOG_OLD, LOG_INFO, "[FBOExtensions::FBOExtensions] glRenderbufferStorageEXT = %x", glRenderbufferStorageEXT);
    Write_Log(LOG_OLD, LOG_INFO, "[FBOExtensions::FBOExtensions] glBindFramebufferEXT = %x", glBindFramebufferEXT);
    Write_Log(LOG_OLD, LOG_INFO, "[FBOExtensions::FBOExtensions] glDeleteFramebuffersEXT = %x", glDeleteFramebuffersEXT);
    Write_Log(LOG_OLD, LOG_INFO, "[FBOExtensions::FBOExtensions] glGenFramebuffersEXT = %x", glGenFramebuffersEXT);
    Write_Log(LOG_OLD, LOG_INFO, "[FBOExtensions::FBOExtensions] glCheckFramebufferStatusEXT = %x", glCheckFramebufferStatusEXT);
    Write_Log(LOG_OLD, LOG_INFO, "[FBOExtensions::FBOExtensions] glFramebufferTexture1DEXT = %x", glFramebufferTexture1DEXT);
    Write_Log(LOG_OLD, LOG_INFO, "[FBOExtensions::FBOExtensions] glFramebufferTexture2DEXT = %x", glFramebufferTexture2DEXT);
    Write_Log(LOG_OLD, LOG_INFO, "[FBOExtensions::FBOExtensions] glFramebufferTexture3DEXT = %x", glFramebufferTexture3DEXT);
    Write_Log(LOG_OLD, LOG_INFO, "[FBOExtensions::FBOExtensions] glFramebufferRenderbufferEXT = %x", glFramebufferRenderbufferEXT);
    Write_Log(LOG_OLD, LOG_INFO, "[FBOExtensions::FBOExtensions] glGenerateMipmapEXT = %x", glGenerateMipmapEXT);

    if(!GLEW_EXT_framebuffer_blit)
    {
        Write_Log(LOG_OLD, LOG_INFO, "[FBOExtensions::FBOExtensions] !GLEW_EXT_framebuffer_blit");
        return;
    }

    if(GLEW_EXT_framebuffer_blit)
    {
#if(0)
        LOAD_FBO_EXT(glBlitFramebufferEXT);
#endif
    }

    if(GLEW_EXT_framebuffer_multisample)
    {
#if(0)
        LOAD_FBO_EXT(glRenderbufferStorageMultisampleEXT);
#endif
    }

    if(GLEW_NV_framebuffer_multisample_coverage)
    {
#if(0)
        LOAD_FBO_EXT(glRenderbufferStorageMultisampleCoverageNV);
#endif
    }
}


/**************************************************************************
 * RenderBuffer
 **************************************************************************/

//////////////////////////////////////////////////////////////////////
// static cache of glRenderbuffers flagged for deletion, which will actually
// be deleted in the correct GL context.

typedef std::list<GLuint> RenderBufferHandleList;
typedef buffered_object<RenderBufferHandleList> DeletedRenderBufferCache;

static DeletedRenderBufferCache s_deletedRenderBufferCache;

void RenderBuffer::deleteRenderBuffer(unsigned int contextID, GLuint rb)
{
    if( rb)
    {
        // add glProgram to the cache for the appropriate context.
        s_deletedRenderBufferCache[contextID].push_back(rb);
    }
}

void RenderBuffer::flushDeletedRenderBuffers(unsigned int contextID,double /*currentTime*/, double& availableTime)
{
#if(0)
    // if no time available don't try to flush objects.
    if(availableTime<=0.0) return;

    const FBOExtensions* extensions = FBOExtensions::instance(contextID,true);
    if(!extensions || !extensions->isSupported()) return;

    const Timer& timer = *Timer::instance();
    Timer_t start_tick = timer.tick();
    double elapsedTime = 0.0;

    {
        RenderBufferHandleList& pList = s_deletedRenderBufferCache[contextID];
        for(RenderBufferHandleList::iterator titr=pList.begin();
            titr!=pList.end() && elapsedTime<availableTime;
           )
        {
            extensions->glDeleteRenderbuffersEXT(1, &(*titr));
            titr = pList.erase( titr);
            elapsedTime = timer.delta_s(start_tick,timer.tick());
        }
    }

    availableTime -= elapsedTime;
#endif
}

void RenderBuffer::discardDeletedRenderBuffers(unsigned int contextID)
{
    RenderBufferHandleList& pList = s_deletedRenderBufferCache[contextID];
    pList.clear();
}


RenderBuffer::RenderBuffer()
:   _internalFormat(GL_DEPTH_COMPONENT24),
    _width(512),
    _height(512),
    _samples(0),
    _colorSamples(0)
{
}

RenderBuffer::RenderBuffer(int width, int height, GLenum internalFormat, int samples, int colorSamples)
:   _internalFormat(internalFormat),
    _width(width),
    _height(height),
    _samples(samples),
    _colorSamples(colorSamples)
{
}

RenderBuffer::~RenderBuffer()
{
    for(unsigned i=0; i<_objectID.size(); ++i)
    {
        if(_objectID[i]) deleteRenderBuffer(i, _objectID[i]);
    }
}

int RenderBuffer::getMaxSamples(unsigned int contextID)
{
    static buffered_value<GLint> maxSamplesList;

    GLint& maxSamples = maxSamplesList[contextID];

    if(!maxSamples && glRenderbufferStorageMultisampleEXT != 0)
    {
        glGetIntegerv(GL_MAX_SAMPLES_EXT, &maxSamples);
    }

    return maxSamples;
}

GLuint RenderBuffer::getObjectID(unsigned int contextID) const
{
    GLuint &objectID = _objectID[contextID];

    int &dirty = _dirty[contextID];

    if(objectID == 0)
    {
        glGenRenderbuffersEXT(1, &objectID);
        if(objectID == 0)
            return 0;
        dirty = 1;
    }

    if(dirty)
    {
        // bind and configure
        glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, objectID);

        // framebuffer_multisample_coverage specification requires that coverage
        // samples must be >= color samples.
        if(_samples < _colorSamples)
        {
            //notify(WARN) << "Coverage samples must be greater than or equal to color samples."
             //   " Setting coverage samples equal to color samples." << std::endl;
            const_cast<RenderBuffer*>(this)->setSamples(_colorSamples);
        }

        if(_samples > 0 && glRenderbufferStorageMultisampleCoverageNV != 0)
        {
            int samples = std::min(_samples, getMaxSamples(contextID));
            int colorSamples = std::min(_colorSamples, samples);

            glRenderbufferStorageMultisampleCoverageNV(GL_RENDERBUFFER_EXT,
                samples, colorSamples, _internalFormat, _width, _height);
        }
        else if(_samples > 0 && glRenderbufferStorageMultisampleEXT != 0)
        {
            int samples = std::min(_samples, getMaxSamples(contextID));

            glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER_EXT,
                samples, _internalFormat, _width, _height);
        }
        else
        {
            glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, _internalFormat, _width, _height);
        }
        dirty = 0;
    }

    return objectID;
}

/**************************************************************************
 * FrameBufferAttachment
 **************************************************************************/

#ifndef GL_TEXTURE_CUBE_MAP_POSITIVE_X
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X  0x8515
#endif

struct FrameBufferAttachment::Pimpl
{
    enum TargetType
    {
        RENDERBUFFER,
        TEXTURE1D,
        TEXTURE2D,
        TEXTURE3D,
        TEXTURECUBE,
        TEXTURERECT,
        TEXTURE2DARRAY
    };

    TargetType targetType;
    ref_ptr<RenderBuffer> renderbufferTarget;
    ref_ptr<Texture> textureTarget;
    int cubeMapFace;
    int level;
    int zoffset;

    explicit Pimpl(TargetType ttype = RENDERBUFFER, int lev = 0)
    :    targetType(ttype),
        cubeMapFace(0),
        level(lev),
        zoffset(0)
    {
    }

    Pimpl(const Pimpl &copy)
    :    targetType(copy.targetType),
        renderbufferTarget(copy.renderbufferTarget),
        textureTarget(copy.textureTarget),
        cubeMapFace(copy.cubeMapFace),
        level(copy.level),
        zoffset(copy.zoffset)
    {
    }

    ~Pimpl()
    {
    }
};

FrameBufferAttachment::FrameBufferAttachment()
{
    _ximpl = new Pimpl;
}

FrameBufferAttachment::FrameBufferAttachment(const FrameBufferAttachment &copy)
{
    _ximpl = new Pimpl(*copy._ximpl);
}

FrameBufferAttachment::FrameBufferAttachment(RenderBuffer* target)
{
    _ximpl = new Pimpl(Pimpl::RENDERBUFFER);
    _ximpl->renderbufferTarget = target;
}

#if(0)
FrameBufferAttachment::FrameBufferAttachment(Texture1D* target, int level)
{
    _ximpl = new Pimpl(Pimpl::TEXTURE1D, level);
    _ximpl->textureTarget = target;
}
#endif

FrameBufferAttachment::FrameBufferAttachment(Texture2D* target, int level)
{
    _ximpl = new Pimpl(Pimpl::TEXTURE2D, level);
    _ximpl->textureTarget = target;
}

#if(0)
FrameBufferAttachment::FrameBufferAttachment(Texture3D* target, int zoffset, int level)
{
    _ximpl = new Pimpl(Pimpl::TEXTURE3D, level);
    _ximpl->textureTarget = target;
    _ximpl->zoffset = zoffset;
}

FrameBufferAttachment::FrameBufferAttachment(Texture2DArray* target, int layer, int level)
{
    _ximpl = new Pimpl(Pimpl::TEXTURE2DARRAY, level);
    _ximpl->textureTarget = target;
    _ximpl->zoffset = layer;
}

FrameBufferAttachment::FrameBufferAttachment(TextureCubeMap* target, int face, int level)
{
    _ximpl = new Pimpl(Pimpl::TEXTURECUBE, level);
    _ximpl->textureTarget = target;
    _ximpl->cubeMapFace = face;
}
#endif

FrameBufferAttachment::FrameBufferAttachment(TextureRectangle* target)
{
    _ximpl = new Pimpl(Pimpl::TEXTURERECT);
    _ximpl->textureTarget = target;
}

FrameBufferAttachment::FrameBufferAttachment(Attachment& attachment)
{
    Texture* texture = attachment._texture.get();

    if(texture)
    {
#if(0)
        Texture1D* texture1D = dynamic_cast<Texture1D*>(texture);
        if(texture1D)
        {
            _ximpl = new Pimpl(Pimpl::TEXTURE1D, attachment._level);
            _ximpl->textureTarget = texture1D;
            return;
        }
#endif
        Texture2D* texture2D = dynamic_cast<Texture2D*>(texture);
        if(texture2D)
        {
            _ximpl = new Pimpl(Pimpl::TEXTURE2D, attachment._level);
            _ximpl->textureTarget = texture2D;
            return;
        }
#if(0)
        Texture3D* texture3D = dynamic_cast<Texture3D*>(texture);
        if(texture3D)
        {
            _ximpl = new Pimpl(Pimpl::TEXTURE3D, attachment._level);
            _ximpl->textureTarget = texture3D;
            _ximpl->zoffset = attachment._face;
            return;
        }

        Texture2DArray* texture2DArray = dynamic_cast<Texture2DArray*>(texture);
        if(texture2DArray)
        {
            _ximpl = new Pimpl(Pimpl::TEXTURE2DARRAY, attachment._level);
            _ximpl->textureTarget = texture2DArray;
            _ximpl->zoffset = attachment._face;
            return;
        }

        TextureCubeMap* textureCubeMap = dynamic_cast<TextureCubeMap*>(texture);
        if(textureCubeMap)
        {
            _ximpl = new Pimpl(Pimpl::TEXTURECUBE, attachment._level);
            _ximpl->textureTarget = textureCubeMap;
            _ximpl->cubeMapFace = attachment._face;
            return;
        }
#endif
        TextureRectangle* textureRectangle = dynamic_cast<TextureRectangle*>(texture);
        if(textureRectangle)
        {
            _ximpl = new Pimpl(Pimpl::TEXTURERECT);
            _ximpl->textureTarget = textureRectangle;
            return;
        }
    }

    Image* image = attachment._image.get();
    if(image)
    {
        if(image->s()>0 && image->t()>0)
        {
            GLenum format = attachment._image->getInternalTextureFormat();
            if(format == 0)
                format = attachment._internalFormat;
            _ximpl = new Pimpl(Pimpl::RENDERBUFFER);
            _ximpl->renderbufferTarget = new RenderBuffer(image->s(), image->t(), format);
        }
        else
        {
            //notify(WARN)<<"Error: FrameBufferAttachment::FrameBufferAttachment(Camera::Attachment&) passed an empty Image, image must be allocated first."<<std::endl;
        }
        return;
    }

   // notify(WARN)<<"Error: FrameBufferAttachment::FrameBufferAttachment(Camera::Attachment&) passed an unrecognised Texture type."<<std::endl;
}


FrameBufferAttachment::~FrameBufferAttachment()
{
    delete _ximpl;
}

FrameBufferAttachment &FrameBufferAttachment::operator = (const FrameBufferAttachment &copy)
{
    delete _ximpl;
    _ximpl = new Pimpl(*copy._ximpl);
    return *this;
}

bool FrameBufferAttachment::isMultisample() const
{
    if(_ximpl->renderbufferTarget.valid())
    {
        return _ximpl->renderbufferTarget->getSamples() > 0;
    }

    return false;
}

void FrameBufferAttachment::createRequiredTexturesAndApplyGenerateMipMap() const
{
    unsigned int contextID = 0;//state.getContextID();

    // force compile texture if necessary
    Texture::TextureObject* tobj = 0;
    if(_ximpl->textureTarget.valid())
    {
        tobj = _ximpl->textureTarget->getTextureObject(contextID);
        if(!tobj || tobj->_id == 0)
        {
            _ximpl->textureTarget->compileGLObjects();
            tobj = _ximpl->textureTarget->getTextureObject(contextID);
        }
        if(!tobj || tobj->_id == 0)
            return;

        Texture::FilterMode minFilter = _ximpl->textureTarget->getFilter(Texture::MIN_FILTER);
        if(minFilter==Texture::LINEAR_MIPMAP_LINEAR ||
            minFilter==Texture::LINEAR_MIPMAP_NEAREST ||
            minFilter==Texture::NEAREST_MIPMAP_LINEAR ||
            minFilter==Texture::NEAREST_MIPMAP_NEAREST)
        {
            oglSetActiveTextureStage(0);
            _ximpl->textureTarget->apply();
            glGenerateMipmapEXT(_ximpl->textureTarget->getTextureTarget());
        }

    }
}

void FrameBufferAttachment::attach(GLenum target, GLenum attachment_point) const
{
    unsigned int contextID = 0;//state.getContextID();

    Texture::TextureObject* tobj = nullptr;
    if(_ximpl->textureTarget.valid())
    {
        tobj = _ximpl->textureTarget->getTextureObject(contextID);
        if(!tobj || tobj->_id == 0)
        {
            _ximpl->textureTarget->compileGLObjects();
            tobj = _ximpl->textureTarget->getTextureObject(contextID);

        }
        if(!tobj || tobj->_id == 0)
            return;
    }

    switch (_ximpl->targetType)
    {
    default:
    case Pimpl::RENDERBUFFER:
        glFramebufferRenderbufferEXT(target, attachment_point, GL_RENDERBUFFER_EXT, _ximpl->renderbufferTarget->getObjectID(contextID));
        break;
    case Pimpl::TEXTURE1D:
        glFramebufferTexture1DEXT(target, attachment_point, GL_TEXTURE_1D, tobj->_id, _ximpl->level);
        break;
    case Pimpl::TEXTURE2D:
        glFramebufferTexture2DEXT(target, attachment_point, GL_TEXTURE_2D, tobj->_id, _ximpl->level);
        break;
    case Pimpl::TEXTURE3D:
        glFramebufferTexture3DEXT(target, attachment_point, GL_TEXTURE_3D, tobj->_id, _ximpl->level, _ximpl->zoffset);
        break;
    case Pimpl::TEXTURE2DARRAY:
        glFramebufferTextureLayerEXT(target, attachment_point, tobj->_id, _ximpl->level, _ximpl->zoffset);
        break;
    case Pimpl::TEXTURERECT:
        glFramebufferTexture2DEXT(target, attachment_point, GL_TEXTURE_RECTANGLE, tobj->_id, 0);
        break;
    case Pimpl::TEXTURECUBE:
        glFramebufferTexture2DEXT(target, attachment_point, GL_TEXTURE_CUBE_MAP_POSITIVE_X + _ximpl->cubeMapFace, tobj->_id, _ximpl->level);
        break;
    }
}

/**************************************************************************
 * FrameBufferObject
 **************************************************************************/

//////////////////////////////////////////////////////////////////////
// static cache of glRenderbuffers flagged for deletion, which will actually
// be deleted in the correct GL context.

typedef std::list<GLuint> FrameBufferObjectHandleList;
typedef buffered_object<FrameBufferObjectHandleList> DeletedFrameBufferObjectCache;

static DeletedFrameBufferObjectCache* s_deletedFrameBufferObjectCache = new DeletedFrameBufferObjectCache(); // avoids crash on exit due to static deletion ordering problem (mac)

void FrameBufferKill()
{
    if(s_deletedFrameBufferObjectCache)
        delete s_deletedFrameBufferObjectCache;

    s_deletedFrameBufferObjectCache = nullptr;
}

void FrameBufferObject::deleteFrameBufferObject(unsigned int contextID, GLuint rb)
{
    if( rb)
    {
        // add glProgram to the cache for the appropriate context.
        (*s_deletedFrameBufferObjectCache)[contextID].push_back(rb);
    }
}

void FrameBufferObject::flushDeletedFrameBufferObjects(unsigned int contextID,double /*currentTime*/, double& availableTime)
{
#if(0)
    // if no time available don't try to flush objects.
    if(availableTime<=0.0) return;

    const FBOExtensions* extensions = FBOExtensions::instance(contextID,true);
    if(!extensions || !extensions->isSupported()) return;

    const Timer& timer = *Timer::instance();
    Timer_t start_tick = timer.tick();
    double elapsedTime = 0.0;

    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(s_mutex_deletedFrameBufferObjectCache);

        FrameBufferObjectHandleList& pList = s_deletedFrameBufferObjectCache[contextID];
        for(FrameBufferObjectHandleList::iterator titr=pList.begin();
            titr!=pList.end() && elapsedTime<availableTime;
           )
        {
            extensions->glDeleteFramebuffersEXT(1, &(*titr));
            titr = pList.erase( titr);
            elapsedTime = timer.delta_s(start_tick,timer.tick());
        }
    }

    availableTime -= elapsedTime;
#endif
}

void FrameBufferObject::discardDeletedFrameBufferObjects(unsigned int contextID)
{
    FrameBufferObjectHandleList& pList = (*s_deletedFrameBufferObjectCache)[contextID];

    pList.clear();
}

FrameBufferObject::FrameBufferObject()
{
}

FrameBufferObject::~FrameBufferObject()
{
    for(unsigned i=0; i<_fboID.size(); ++i)
    {
        if(_fboID[i]) deleteFrameBufferObject(i, _fboID[i]);
    }

    _attachments.clear();
}

void FrameBufferObject::setAttachment(BufferComponent attachment_point, const FrameBufferAttachment &attachment)
{
    GLenum gl_attachment = convertBufferComponentToGLenum(attachment_point);
    _attachments[attachment_point] = attachment;

    updateDrawBuffers();
    dirtyAll();
}


GLenum FrameBufferObject::convertBufferComponentToGLenum(BufferComponent attachment_point) const
{
    switch(attachment_point)
    {
        case(DEPTH_BUFFER): return GL_DEPTH_ATTACHMENT_EXT;
        case(STENCIL_BUFFER): return GL_STENCIL_ATTACHMENT_EXT;
        case(COLOR_BUFFER): return GL_COLOR_ATTACHMENT0_EXT;
        default: return GLenum(GL_COLOR_ATTACHMENT0_EXT + (attachment_point-COLOR_BUFFER0));
    }
}

void FrameBufferObject::updateDrawBuffers()
{
    _drawBuffers.clear();

    // create textures and mipmaps before we bind the frame buffer object
    for(AttachmentMap::const_iterator i=_attachments.begin(); i!=_attachments.end(); ++i)
    {
        const FrameBufferAttachment &fa = i->second;

        // setup draw buffers based on the attachment definition
        if(i->first >= COLOR_BUFFER0 && i->first <= COLOR_BUFFER15)
            _drawBuffers.push_back(convertBufferComponentToGLenum(i->first));
    }
}

void FrameBufferObject::apply() const
{
    apply(READ_DRAW_FRAMEBUFFER);
}

void FrameBufferObject::Blit(FrameBufferObject* dest,
                             int sx,
                             int sy,
                             int swidth,
                             int sheight,
                             int dx,
                             int dy,
                             int dwidth,
                             int dheight,
                             GLenum mask,
                             GLenum filter)
{
    // Bind the resolve framebuffer to blit into.
    apply(FrameBufferObject::READ_FRAMEBUFFER);       // From
    dest->apply(FrameBufferObject::DRAW_FRAMEBUFFER); // To

    glBlitFramebufferEXT(sx,
                         sy,
                         swidth,
                         sheight,
                         dx,
                         dy,
                         dwidth,
                         dheight,
                         mask,
                         filter);

    apply(READ_DRAW_FRAMEBUFFER);
}

void FrameBufferObject::Blit(FrameBufferObject* src,
                             FrameBufferObject* dest,
                             int sx,
                             int sy,
                             int swidth,
                             int sheight,
                             int dx,
                             int dy,
                             int dwidth,
                             int dheight,
                             GLenum mask,
                             GLenum filter)
{
    // Bind the resolve framebuffer to blit into.
    src->apply(FrameBufferObject::READ_FRAMEBUFFER);  // From
    dest->apply(FrameBufferObject::DRAW_FRAMEBUFFER); // To

    glBlitFramebufferEXT(sx,
                         sy,
                         swidth,
                         sheight,
                         dx,
                         dy,
                         dwidth,
                         dheight,
                         mask,
                         filter);

    src->apply(READ_DRAW_FRAMEBUFFER);
}

void FrameBufferObject::apply(BindTarget target) const
{
    unsigned int contextID = 0;//state.getContextID();

    if(_unsupported[contextID])
    {
        Write_Log(LOG_OLD, LOG_INFO, "Unsupported");
        return;
    }

    FBOExtensions* ext = FBOExtensions::instance(contextID,true);
    if(!ext->isSupported())
    {
        _unsupported[contextID] = 1;
        Write_Log(LOG_OLD, LOG_INFO, "EXT_framebuffer_object is not supported");
        return;
    }

    if(_attachments.empty())
    {
        UnApply();
        return;
    }

    int &dirtyAttachmentList = _dirtyAttachmentList[contextID];

    GLuint &fboID = _fboID[contextID];
    if(fboID == 0)
    {
        glGenFramebuffersEXT(1, &fboID);
        if(fboID == 0)
        {
            Write_Log(LOG_OLD, LOG_INFO, "could not create the FBO");
            return;
        }

        dirtyAttachmentList = 1;

    }

    if(dirtyAttachmentList)
    {
        // the set of of attachments appears to be thread sensitive, it shouldn't be because
        // OpenGL FBO handles FrameBufferObject has are multi-buffered...
        // so as a temporary fix will stick in a mutex to ensure that only one thread passes through here
        // at one time.
        // create textures and mipmaps before we bind the frame buffer object
        for(AttachmentMap::const_iterator i=_attachments.begin(); i!=_attachments.end(); ++i)
        {
            const FrameBufferAttachment &fa = i->second;
            fa.createRequiredTexturesAndApplyGenerateMipMap();
        }

    }

    glBindFramebufferEXT(target, fboID);

    // enable drawing buffers to render the result to fbo
    if(_drawBuffers.size() > 0)
    {
        glDrawBuffers(_drawBuffers.size(), &(_drawBuffers[0]));
    }

    if(_attachments.size() == 1)
    {
        if(_attachments.begin()->first == DEPTH_BUFFER)
        {
#if USE_STATESET
            glDrawBuffer(GL_NONE);
            glReadBuffer(GL_NONE);
#else
            oglSetRenderState(oglRENDERSTATE_DRAWBUFFER, GL_NONE);
            oglSetRenderState(oglRENDERSTATE_READBUFFER, GL_NONE);
#endif
        }
    }

    if(dirtyAttachmentList)
    {
        for(AttachmentMap::const_iterator i=_attachments.begin(); i!=_attachments.end(); ++i)
        {
            const FrameBufferAttachment &fa = i->second;
            fa.attach(target, convertBufferComponentToGLenum(i->first));
        }
        dirtyAttachmentList = 0;
    }

}

bool FrameBufferObject::isMultisample() const
{
    if(_attachments.size())
    {
        // If the FBO is correctly set up then all attachments will be either
        // multisampled or single sampled. Therefore we can just return the
        // result of the first attachment.
        return _attachments.begin()->second.isMultisample();
    }

    return false;
}
