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
#include <stdio.h>
#include <math.h>
#include <float.h>
#include <map>

#include "osgImport/BufferObject.h"
#include "osgImport/Image.h"
#include "osgImport/ref_ptr.h"

// static cache of deleted buffer object lists which can only
// by completely deleted once the appropriate OpenGL context
// is set.  Used osg::BufferObject::deleteBufferObject(..) and flushDeletedBufferObjects(..) below.
typedef std::multimap<unsigned int,GLuint> BufferObjectMap;
typedef buffered_object<BufferObjectMap> DeletedBufferObjectCache;

static DeletedBufferObjectCache s_deletedBufferObjectCache;

void BufferObject::deleteBufferObject(unsigned int contextID,GLuint globj)
{
    if (globj!=0)
    {
        // insert the globj into the cache for the appropriate context.
        s_deletedBufferObjectCache[contextID].insert(BufferObjectMap::value_type(0,globj));
    }
}

void BufferObject::flushDeletedBufferObjects(unsigned int contextID,double /*currentTime*/, double& availableTime)
{
}

void BufferObject::discardDeletedBufferObjects(unsigned int contextID)
{
}


BufferObject::BufferObject():
    _target(0),
    _usage(0),
    _totalSize(0)
{
}

BufferObject::~BufferObject()
{
    releaseGLObjects();
}

void BufferObject::resizeGLObjectBuffers(unsigned int maxSize)
{
    _bufferObjectList.resize(maxSize);
}

void BufferObject::releaseGLObjects() const
{
    if (1)
    {
        unsigned int contextID = 0;//state->getContextID();
        if (_bufferObjectList[contextID])
        {
             deleteBufferObject(contextID,_bufferObjectList[contextID]);
            _bufferObjectList[contextID] = 0;
        }
    }
    else
    {
        for(unsigned int contextID=0;contextID<_bufferObjectList.size();++contextID)
        {
            if (_bufferObjectList[contextID])
            {
                 deleteBufferObject(contextID,_bufferObjectList[contextID]);
                _bufferObjectList[contextID] = 0;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////
//
//  PixelBufferObject
//
PixelBufferObject::PixelBufferObject(Image* image):
    BufferObject()
{
    _target = GL_PIXEL_UNPACK_BUFFER_ARB;
    _usage = GL_STREAM_DRAW_ARB;
    _bufferEntryImagePair.second = image;
}

PixelBufferObject::~PixelBufferObject()
{
}

void PixelBufferObject::setImage(Image* image)
{
    if (_bufferEntryImagePair.second == image) return;

    _bufferEntryImagePair.second = image;

    dirty();
}

void PixelBufferObject::compileBuffer() const
{
    unsigned int contextID = 0;

    _compiledList[contextID] = 1;

    Image* image = _bufferEntryImagePair.second;

    _bufferEntryImagePair.first.modifiedCount[contextID] = image->getModifiedCount();
    if (!image->valid()) return;

    GLuint& pbo = buffer(contextID);
    if (pbo==0)
    {
        // building for the first time.

        _totalSize = image->getTotalSizeInBytes();

        // don't generate buffer if size is zero.
        if (_totalSize==0) return;

        glGenBuffersARB(1, &pbo);
        glBindBufferARB(_target, pbo);
        glBufferDataARB(_target, _totalSize, NULL, _usage);

    }
    else
    {
        glBindBufferARB(_target, pbo);

        if (_totalSize != image->getTotalSizeInBytes())
        {
            // resize PBO.
            _totalSize = image->getTotalSizeInBytes();
            glBufferDataARB(_target, _totalSize, NULL, _usage);
        }
    }

    void* pboMemory = glMapBufferARB(_target, GL_WRITE_ONLY_ARB);

    // copy data across
    memcpy(pboMemory, image->data(), _totalSize);

    // Unmap the texture image buffer
    glUnmapBufferARB(_target);

    _bufferEntryImagePair.first.modifiedCount[contextID] = image->getModifiedCount();
}

void PixelBufferObject::resizeGLObjectBuffers(unsigned int maxSize)
{
    BufferObject::resizeGLObjectBuffers(maxSize);

    _bufferEntryImagePair.first.modifiedCount.resize(maxSize);
}


//////////////////////////////////////////////////////////////////////
//
//  PixelDataBufferObject
//
//--------------------------------------------------------------------------------
PixelDataBufferObject::PixelDataBufferObject()
{
    _target = GL_ARRAY_BUFFER_ARB;
    _usage = GL_DYNAMIC_DRAW_ARB;
    _bufferData.dataSize = 0;
}

//--------------------------------------------------------------------------------
PixelDataBufferObject::~PixelDataBufferObject()
{
}

//--------------------------------------------------------------------------------
void PixelDataBufferObject::compileBuffer() const
{
    unsigned int contextID = 0;
    if (!isDirty(contextID) || _bufferData.dataSize == 0) return;

    GLuint& pbo = buffer(contextID);
    if (pbo == 0)
    {
        glGenBuffersARB(1, &pbo);
    }

    glBindBufferARB(_target, pbo);
    glBufferDataARB(_target, _bufferData.dataSize, NULL, _usage);
    glBindBufferARB(_target, 0);

    _compiledList[contextID] = 1;
}

//--------------------------------------------------------------------------------
void PixelDataBufferObject::bindBufferInReadMode()
{
    unsigned int contextID = 0;
    if (isDirty(contextID)) compileBuffer();

    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, buffer(contextID));
    _mode[contextID] = READ;
}

//--------------------------------------------------------------------------------
void PixelDataBufferObject::bindBufferInWriteMode()
{
    unsigned int contextID = 0;
    if (isDirty(contextID)) compileBuffer();

    glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, buffer(contextID));
    _mode[contextID] = WRITE;
}

//--------------------------------------------------------------------------------
void PixelDataBufferObject::unbindBuffer(unsigned int contextID) const
{
    switch(_mode[contextID])
    {
        case READ:
            glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB,0);
            break;
        case WRITE:
            glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB,0);
            break;
        default:
            glBindBufferARB(_target,0);
            break;
    }

    _mode[contextID] = NONE;
}

//--------------------------------------------------------------------------------
void PixelDataBufferObject::resizeGLObjectBuffers(unsigned int maxSize)
{
    BufferObject::resizeGLObjectBuffers(maxSize);

    _mode.resize(maxSize);
}

