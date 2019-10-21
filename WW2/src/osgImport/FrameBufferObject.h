#pragma once
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

#include "ogl/ogl.h"
#include "osgImport/Texture.h"
#include "osgImport/buffered_value.h"
#include "osgImport/Referenced.h"
#include "ogl/States.h"

#ifndef GL_EXT_framebuffer_object
#define GL_EXT_framebuffer_object 1
#define GL_FRAMEBUFFER_EXT                     0x8D40
#define GL_RENDERBUFFER_EXT                    0x8D41
#define GL_STENCIL_INDEX_EXT                   0x8D45
#define GL_STENCIL_INDEX1_EXT                  0x8D46
#define GL_STENCIL_INDEX4_EXT                  0x8D47
#define GL_STENCIL_INDEX8_EXT                  0x8D48
#define GL_STENCIL_INDEX16_EXT                 0x8D49
#define GL_RENDERBUFFER_WIDTH_EXT              0x8D42
#define GL_RENDERBUFFER_HEIGHT_EXT             0x8D43
#define GL_RENDERBUFFER_INTERNAL_FORMAT_EXT    0x8D44
#define GL_RENDERBUFFER_RED_SIZE_EXT           0x8D50
#define GL_RENDERBUFFER_GREEN_SIZE_EXT         0x8D51
#define GL_RENDERBUFFER_BLUE_SIZE_EXT          0x8D52
#define GL_RENDERBUFFER_ALPHA_SIZE_EXT         0x8D53
#define GL_RENDERBUFFER_DEPTH_SIZE_EXT         0x8D54
#define GL_RENDERBUFFER_STENCIL_SIZE_EXT       0x8D55
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_EXT            0x8CD0
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_EXT            0x8CD1
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL_EXT          0x8CD2
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE_EXT  0x8CD3
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_3D_ZOFFSET_EXT     0x8CD4
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LAYER_EXT          0x8CD4
#define GL_COLOR_ATTACHMENT0_EXT                0x8CE0
#define GL_COLOR_ATTACHMENT1_EXT                0x8CE1
#define GL_COLOR_ATTACHMENT2_EXT                0x8CE2
#define GL_COLOR_ATTACHMENT3_EXT                0x8CE3
#define GL_COLOR_ATTACHMENT4_EXT                0x8CE4
#define GL_COLOR_ATTACHMENT5_EXT                0x8CE5
#define GL_COLOR_ATTACHMENT6_EXT                0x8CE6
#define GL_COLOR_ATTACHMENT7_EXT                0x8CE7
#define GL_COLOR_ATTACHMENT8_EXT                0x8CE8
#define GL_COLOR_ATTACHMENT9_EXT                0x8CE9
#define GL_COLOR_ATTACHMENT10_EXT               0x8CEA
#define GL_COLOR_ATTACHMENT11_EXT               0x8CEB
#define GL_COLOR_ATTACHMENT12_EXT               0x8CEC
#define GL_COLOR_ATTACHMENT13_EXT               0x8CED
#define GL_COLOR_ATTACHMENT14_EXT               0x8CEE
#define GL_COLOR_ATTACHMENT15_EXT               0x8CEF
#define GL_DEPTH_ATTACHMENT_EXT                 0x8D00
#define GL_STENCIL_ATTACHMENT_EXT               0x8D20
#define GL_FRAMEBUFFER_COMPLETE_EXT                          0x8CD5
#define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT             0x8CD6
#define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT     0x8CD7
#define GL_FRAMEBUFFER_INCOMPLETE_DUPLICATE_ATTACHMENT_EXT   0x8CD8
#define GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT             0x8CD9
#define GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT                0x8CDA
#define GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT            0x8CDB
#define GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT            0x8CDC
#define GL_FRAMEBUFFER_UNSUPPORTED_EXT                       0x8CDD
#define GL_FRAMEBUFFER_BINDING_EXT             0x8CA6
#define GL_RENDERBUFFER_BINDING_EXT            0x8CA7
#define GL_MAX_COLOR_ATTACHMENTS_EXT           0x8CDF
#define GL_MAX_RENDERBUFFER_SIZE_EXT           0x84E8
#define GL_INVALID_FRAMEBUFFER_OPERATION_EXT   0x0506
#endif

#ifndef GL_EXT_framebuffer_blit
#define GL_EXT_framebuffer_blit 1
#define GL_DRAW_FRAMEBUFFER_BINDING_EXT 0x8CA6
#define GL_READ_FRAMEBUFFER_EXT 0x8CA8
#define GL_DRAW_FRAMEBUFFER_EXT 0x8CA9
#define GL_READ_FRAMEBUFFER_BINDING_EXT 0x8CAA
#endif

#ifndef GL_EXT_framebuffer_multisample
#define GL_EXT_framebuffer_multisample 1
#define GL_RENDERBUFFER_SAMPLES_EXT                 0x8CAB
#define GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_EXT   0x8D56
#define GL_MAX_SAMPLES_EXT                          0x8D57
#endif

#ifndef GL_MAX_SAMPLES_EXT
// Workaround for Centos 5 and other distros that define
// GL_EXT_framebuffer_multisample but not GL_MAX_SAMPLES_EXT
#define GL_MAX_SAMPLES_EXT 0x8D57
#endif

#ifndef GL_NV_framebuffer_multisample_coverage
#define GL_NV_framebuffer_multisample_coverage 1
#define GL_RENDERBUFFER_COVERAGE_SAMPLES_NV     0x8CAB
#define GL_RENDERBUFFER_COLOR_SAMPLES_NV        0x8E10
#define GL_MAX_MULTISAMPLE_COVERAGE_MODES_NV    0x8E11
#define GL_MULTISAMPLE_COVERAGE_MODES_NV        0x8E12
#endif

#ifndef GL_VERSION_1_4
#define GL_DEPTH_COMPONENT16              0x81A5
#define GL_DEPTH_COMPONENT24              0x81A6
#define GL_DEPTH_COMPONENT32              0x81A7
#endif

/**************************************************************************
 * FBOExtensions
 **************************************************************************/

    class FBOExtensions : public Referenced
    {
    public:
#if(0)
        typedef void TglBindRenderbufferEXT(GLenum, GLuint);
        typedef void TglDeleteRenderbuffersEXT(GLsizei n, const GLuint* renderbuffers);
        typedef void TglGenRenderbuffersEXT(GLsizei, GLuint*);
        typedef void TglRenderbufferStorageEXT(GLenum, GLenum, GLsizei, GLsizei);
        typedef void TglRenderbufferStorageMultisampleEXT(GLenum, GLsizei, GLenum, GLsizei, GLsizei);
        typedef void TglRenderbufferStorageMultisampleCoverageNV(GLenum, GLsizei, GLsizei, GLenum, GLsizei, GLsizei);
        typedef void TglBindFramebufferEXT(GLenum, GLuint);
        typedef void TglDeleteFramebuffersEXT(GLsizei n, const GLuint* framebuffers);
        typedef void TglGenFramebuffersEXT(GLsizei, GLuint*);
        typedef GLenum TglCheckFramebufferStatusEXT(GLenum);
        typedef void TglFramebufferTexture1DEXT(GLenum, GLenum, GLenum, GLuint, GLint);
        typedef void TglFramebufferTexture2DEXT(GLenum, GLenum, GLenum, GLuint, GLint);
        typedef void TglFramebufferTexture3DEXT(GLenum, GLenum, GLenum, GLuint, GLint, GLint);
        typedef void TglFramebufferTextureLayerEXT(GLenum, GLenum, GLuint, GLint, GLint);
        typedef void TglFramebufferRenderbufferEXT(GLenum, GLenum, GLenum, GLuint);
        typedef void TglGenerateMipmapEXT(GLenum);
        typedef void TglBlitFramebufferEXT(GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLbitfield, GLenum);

        TglBindRenderbufferEXT* glBindRenderbufferEXT;
        TglGenRenderbuffersEXT* glGenRenderbuffersEXT;
        TglDeleteRenderbuffersEXT* glDeleteRenderbuffersEXT;
        TglRenderbufferStorageEXT* glRenderbufferStorageEXT;
        TglRenderbufferStorageMultisampleEXT* glRenderbufferStorageMultisampleEXT;
        TglRenderbufferStorageMultisampleCoverageNV* glRenderbufferStorageMultisampleCoverageNV;
        TglBindFramebufferEXT* glBindFramebufferEXT;
        TglDeleteFramebuffersEXT* glDeleteFramebuffersEXT;
        TglGenFramebuffersEXT* glGenFramebuffersEXT;
        TglCheckFramebufferStatusEXT* glCheckFramebufferStatusEXT;
        TglFramebufferTexture1DEXT* glFramebufferTexture1DEXT;
        TglFramebufferTexture2DEXT* glFramebufferTexture2DEXT;
        TglFramebufferTexture3DEXT* glFramebufferTexture3DEXT;
        TglFramebufferTextureLayerEXT* glFramebufferTextureLayerEXT;
        TglFramebufferRenderbufferEXT* glFramebufferRenderbufferEXT;
        TglGenerateMipmapEXT* glGenerateMipmapEXT;
        TglBlitFramebufferEXT* glBlitFramebufferEXT;
#endif

        static FBOExtensions* instance(unsigned contextID, bool createIfNotInitalized);

        bool isSupported() const { return _supported; }
        bool isMultisampleSupported() const { return glRenderbufferStorageMultisampleEXT != 0; }
        bool isMultisampleCoverageSupported() const { return glRenderbufferStorageMultisampleCoverageNV != 0; }

    protected:
        FBOExtensions(unsigned int contextID);

    private:
        bool _supported;
    };

/**************************************************************************
 * RenderBuffer
 **************************************************************************/

    class RenderBuffer: public Referenced
    {
    public:
        RenderBuffer();
        RenderBuffer(int width, int height, GLenum internalFormat, int samples=0, int colorSamples=0);

        inline int getWidth() const;
        inline int getHeight() const;
        inline void setWidth(int w);
        inline void setHeight(int h);
        inline void setSize(int w, int h);
        inline GLenum getInternalFormat() const;
        inline void setInternalFormat(GLenum format);
        inline int getSamples() const;
        inline int getColorSamples() const;
        inline void setSamples(int samples);
        inline void setColorSamples(int colorSamples);

        GLuint getObjectID(unsigned int contextID) const;
        inline int compare(const RenderBuffer &rb) const;

        /** Mark internal RenderBuffer for deletion.
          * Deletion requests are queued until they can be executed
          * in the proper GL context. */
        static void deleteRenderBuffer(unsigned int contextID, GLuint rb);

        /** flush all the cached RenderBuffers which need to be deleted
          * in the OpenGL context related to contextID.*/
        static void flushDeletedRenderBuffers(unsigned int contextID,double currentTime, double& availableTime);

        /** discard all the cached RenderBuffers which need to be deleted in the OpenGL context related to contextID.
          * Note, unlike flush no OpenGL calls are made, instead the handles are all removed.
          * this call is useful for when an OpenGL context has been destroyed. */
        static void discardDeletedRenderBuffers(unsigned int contextID);

        static int getMaxSamples(unsigned int contextID);

    protected:
        virtual ~RenderBuffer();
        RenderBuffer &operator=(const RenderBuffer &) { return *this; }

        inline void dirtyAll() const;

    private:
        mutable buffered_value<GLuint> _objectID;
        mutable buffered_value<int> _dirty;

        GLenum _internalFormat;
        int _width;
        int _height;
        // "samples" in the framebuffer_multisample extension is equivalent to
        // "coverageSamples" in the framebuffer_multisample_coverage extension.
        int _samples;
        int _colorSamples;
    };

    // INLINE METHODS

    inline int RenderBuffer::getWidth() const
    {
        return _width;
    }

    inline int RenderBuffer::getHeight() const
    {
        return _height;
    }

    inline void RenderBuffer::setWidth(int w)
    {
        _width = w;
        dirtyAll();
    }

    inline void RenderBuffer::setHeight(int h)
    {
        _height = h;
        dirtyAll();
    }

    inline void RenderBuffer::setSize(int w, int h)
    {
        _width = w;
        _height = h;
        dirtyAll();
    }

    inline GLenum RenderBuffer::getInternalFormat() const
    {
        return _internalFormat;
    }

    inline void RenderBuffer::setInternalFormat(GLenum format)
    {
        _internalFormat = format;
        dirtyAll();
    }

    inline int RenderBuffer::getSamples() const
    {
        return _samples;
    }

    inline int RenderBuffer::getColorSamples() const
    {
        return _colorSamples;
    }

    inline void RenderBuffer::setSamples(int samples)
    {
        _samples = samples;
        dirtyAll();
    }

    inline void RenderBuffer::setColorSamples(int colorSamples)
    {
        _colorSamples = colorSamples;
        dirtyAll();
    }

    inline void RenderBuffer::dirtyAll() const
    {
        _dirty.setAllElementsTo(1);
    }

    inline int RenderBuffer::compare(const RenderBuffer &rb) const
    {
        if (&rb == this) return 0;
        if (_internalFormat < rb._internalFormat) return -1;
        if (_internalFormat > rb._internalFormat) return 1;
        if (_width < rb._width) return -1;
        if (_width > rb._width) return 1;
        if (_height < rb._height) return -1;
        if (_height > rb._height) return 1;
        return 0;
    }

/**************************************************************************
 * FrameBufferAttachement
 **************************************************************************/
    class Texture1D;
    class Texture2D;
    class Texture3D;
    class Texture2DArray;
    class TextureCubeMap;
    class TextureRectangle;

    class FrameBufferAttachment : public Referenced
    {
        struct Attachment
        {
            Attachment():
                _internalFormat(GL_NONE),
                _level(0),
                _face(0),
                _mipMapGeneration(false),
                _multisampleSamples(0),
                _multisampleColorSamples(0) {}

            int width() const
            {
                if (_texture.valid()) return _texture->getTextureWidth();
                if (_image.valid()) return _image->s();
                return 0;
            };

            int height() const
            {
                if (_texture.valid()) return _texture->getTextureHeight();
                if (_image.valid()) return _image->t();
                return 0;
            };

            int depth() const
            {
                if (_texture.valid()) return _texture->getTextureDepth();
                if (_image.valid()) return _image->r();
                return 0;
            };

            GLenum              _internalFormat;
            ref_ptr<Image>      _image;
            ref_ptr<Texture>    _texture;
            unsigned int        _level;
            unsigned int        _face;
            bool                _mipMapGeneration;
            unsigned int        _multisampleSamples;
            unsigned int        _multisampleColorSamples;
        };

    public:
        FrameBufferAttachment();
        FrameBufferAttachment(const FrameBufferAttachment& copy);

        explicit FrameBufferAttachment(RenderBuffer* target);
        explicit FrameBufferAttachment(Texture1D* target, int level = 0);
        explicit FrameBufferAttachment(Texture2D* target, int level = 0);
        explicit FrameBufferAttachment(Texture3D* target, int zoffset, int level = 0);
        explicit FrameBufferAttachment(Texture2DArray* target, int layer, int level = 0);
        explicit FrameBufferAttachment(TextureCubeMap* target, int face, int level = 0);
        explicit FrameBufferAttachment(TextureRectangle* target);
        explicit FrameBufferAttachment(Attachment& attachment);

        ~FrameBufferAttachment();

        FrameBufferAttachment&operator = (const FrameBufferAttachment& copy);

        bool isMultisample() const;
        void createRequiredTexturesAndApplyGenerateMipMap() const;
        void attach(GLenum target, GLenum attachment_point) const;
        int compare(const FrameBufferAttachment &fa) const;

    private:
        // use the Pimpl idiom to avoid dependency from
        // all Texture* headers
        struct Pimpl;
        Pimpl* _ximpl;
    };

/**************************************************************************
 * FrameBufferObject
 **************************************************************************/
#if USE_STATESET
    class FrameBufferObject : public RenderState
#else
    class FrameBufferObject : public Referenced
#endif
    {
    public:

        enum BufferComponent
        {
            DEPTH_BUFFER,
            STENCIL_BUFFER,
            COLOR_BUFFER,
            COLOR_BUFFER0,
            COLOR_BUFFER1 = COLOR_BUFFER0+1,
            COLOR_BUFFER2 = COLOR_BUFFER0+2,
            COLOR_BUFFER3 = COLOR_BUFFER0+3,
            COLOR_BUFFER4 = COLOR_BUFFER0+4,
            COLOR_BUFFER5 = COLOR_BUFFER0+5,
            COLOR_BUFFER6 = COLOR_BUFFER0+6,
            COLOR_BUFFER7 = COLOR_BUFFER0+7,
            COLOR_BUFFER8 = COLOR_BUFFER0+8,
            COLOR_BUFFER9 = COLOR_BUFFER0+9,
            COLOR_BUFFER10 = COLOR_BUFFER0+10,
            COLOR_BUFFER11 = COLOR_BUFFER0+11,
            COLOR_BUFFER12 = COLOR_BUFFER0+12,
            COLOR_BUFFER13 = COLOR_BUFFER0+13,
            COLOR_BUFFER14 = COLOR_BUFFER0+14,
            COLOR_BUFFER15 = COLOR_BUFFER0+15
        };

        static void Blit(FrameBufferObject* src,
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
                         GLenum filter);

        void Blit(FrameBufferObject* dest,
                  int sx,
                  int sy,
                  int swidth,
                  int sheight,
                  int dx,
                  int dy,
                  int dwidth,
                  int dheight,
                  GLenum mask,
                  GLenum filter);

        using AttachmentMap = std::map<BufferComponent, FrameBufferAttachment>;
        using MultipleRenderingTargets = std::vector<GLenum>;

        FrameBufferObject();

        inline const AttachmentMap& getAttachmentMap() const;


        void setAttachment(BufferComponent attachment_point, const FrameBufferAttachment &attachment);
        inline const FrameBufferAttachment& getAttachment(BufferComponent attachment_point) const;
        inline bool hasAttachment(BufferComponent attachment_point) const;

        inline bool hasMultipleRenderingTargets() const { return !_drawBuffers.empty(); }
        inline const MultipleRenderingTargets& getMultipleRenderingTargets() const { return _drawBuffers; }

        bool isMultisample() const;

#if USE_STATESET
        virtual void apply() const;
        virtual int type() const {return RS_FRAMEBUFFEROBJECT;}
#else
        void apply() const;
#endif
        void UnApply() const
        {
            glBindFramebufferEXT(FrameBufferObject::READ_DRAW_FRAMEBUFFER, ddpDefaultFramebuffer());

            if(_attachments.size() == 1)
            {
                if(_attachments.begin()->first == DEPTH_BUFFER)
                {
#if USE_STATESET
                    glDrawBuffer(GL_BACK);
                    glReadBuffer(GL_BACK);
#else
                    oglSetRenderState(oglRENDERSTATE_DRAWBUFFER, GL_BACK);
                    oglSetRenderState(oglRENDERSTATE_READBUFFER, GL_BACK);
#endif
                }
            }
        }

        enum BindTarget
        {
            READ_FRAMEBUFFER = GL_READ_FRAMEBUFFER_EXT,
            DRAW_FRAMEBUFFER = GL_DRAW_FRAMEBUFFER_EXT,
            READ_DRAW_FRAMEBUFFER = GL_FRAMEBUFFER_EXT
        };

        /** Bind the FBO as either the read or draw target, or both. */
        void apply(BindTarget target) const;

        /** Mark internal FBO for deletion.
          * Deletion requests are queued until they can be executed
          * in the proper GL context. */
        static void deleteFrameBufferObject(unsigned int contextID, GLuint program);

        /** flush all the cached FBOs which need to be deleted
          * in the OpenGL context related to contextID.*/
        static void flushDeletedFrameBufferObjects(unsigned int contextID,double currentTime, double& availableTime);

        /** discard all the cached FBOs which need to be deleted
          * in the OpenGL context related to contextID.*/
        static void discardDeletedFrameBufferObjects(unsigned int contextID);

    protected:
        virtual ~FrameBufferObject();
        FrameBufferObject& operator = (const FrameBufferObject&) { return *this; }

        void updateDrawBuffers();

        inline void dirtyAll();

        GLenum convertBufferComponentToGLenum(BufferComponent attachment_point) const;

    private:
        AttachmentMap               _attachments;

        // Buffers passed to glDrawBuffers when using multiple render targets.
        MultipleRenderingTargets    _drawBuffers;

        mutable buffered_value<int>         _dirtyAttachmentList;
        mutable buffered_value<int>         _unsupported;
        mutable buffered_value<GLuint>      _fboID;

    };

    // INLINE METHODS

    inline const FrameBufferObject::AttachmentMap &FrameBufferObject::getAttachmentMap() const
    {
        return _attachments;
    }

    inline bool FrameBufferObject::hasAttachment(FrameBufferObject::BufferComponent attachment_point) const
    {
        return _attachments.find(attachment_point) != _attachments.end();
    }

    inline const FrameBufferAttachment &FrameBufferObject::getAttachment(FrameBufferObject::BufferComponent attachment_point) const
    {
        return _attachments.find(attachment_point)->second;
    }

    inline void FrameBufferObject::dirtyAll()
    {
        _dirtyAttachmentList.setAllElementsTo(1);
    }

extern void FrameBufferKill();
