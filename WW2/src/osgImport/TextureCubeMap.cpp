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
#include "ogl/ogl.h"

#include "osgImport/ref_ptr.h"
#include "osgImport/Image.h"
#include "osgImport/TextureCubeMap.h"
#include "ogl/States.h"

static GLenum faceTarget[6] =
{
    GL_TEXTURE_CUBE_MAP_POSITIVE_X,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
    GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
    GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
};

TextureCubeMap::TextureCubeMap():
            _textureWidth(0),
            _textureHeight(0),
            _numMipmapLevels(0)
{
    setUseHardwareMipMapGeneration(false);
}

TextureCubeMap::~TextureCubeMap()
{
}

void TextureCubeMap::setImage( unsigned int face, Image* image)
{
    if (_images[face] == image) return;

#if(0)
    unsigned numImageSequencesBefore = 0;
    for (unsigned int i=0; i<getNumImages(); ++i)
    {
        osg::ImageSequence* is = dynamic_cast<osg::ImageSequence*>(_images[i].get());
        if (is) ++numImageSequencesBefore;
    }
#endif

    _images[face] = image;
    _modifiedCount[face].setAllElementsTo(0);

#if(0)
    // find out if we need to reset the update callback to handle the animation of ImageSequence
    unsigned numImageSequencesAfter = 0;
    for (unsigned int i=0; i<getNumImages(); ++i)
    {
        osg::ImageSequence* is = dynamic_cast<osg::ImageSequence*>(_images[i].get());
        if (is) ++numImageSequencesAfter;
    }

    if(numImageSequencesBefore>0)
    {
        if (numImageSequencesAfter==0)
        {
            setUpdateCallback(0);
            setDataVariance(osg::Object::STATIC);
        }
    }
    else if (numImageSequencesAfter>0)
    {
        setUpdateCallback(new ImageSequence::UpdateCallback());
        setDataVariance(osg::Object::DYNAMIC);
    }
#endif
}

Image* TextureCubeMap::getImage(unsigned int face)
{
    return _images[face].get();
}

const Image* TextureCubeMap::getImage(unsigned int face) const
{
    return _images[face].get();
}

bool TextureCubeMap::imagesValid() const
{
    for (int n=0; n<6; n++)
    {
        if (!_images[n].valid() || !_images[n]->data())
            return false;
    }
    return true;
}

void TextureCubeMap::computeInternalFormat() const
{
    if (imagesValid()) computeInternalFormatWithImage(*_images[0]);
    else computeInternalFormatType();
}

void TextureCubeMap::apply() const
{
    // get the contextID (user defined ID of 0 upwards) for the
    // current OpenGL context.
    const unsigned int contextID = 0;//state.getContextID();
    const Extensions* extensions = getExtensions(contextID,true);

    if (!extensions->isCubeMapSupported())
        return;

    // get the texture object for the current contextID.
    TextureObject* textureObject = getTextureObject(contextID);

    if (textureObject != 0)
    {
        textureObject->bind();

        if (getTextureParameterDirty(0))
            applyTexParameters(GL_TEXTURE_CUBE_MAP);

        if (_subloadCallback.valid())
        {
            _subloadCallback->subload(*this);
        }
        else
        {
            for (int n=0; n<6; n++)
            {
                const Image* image = _images[n].get();
                if (image && getModifiedCount((Face)n,contextID) != image->getModifiedCount())
                {
                    applyTexImage2D_subload(faceTarget[n], _images[n].get(), _textureWidth, _textureHeight, _internalFormat, _numMipmapLevels);
                    getModifiedCount((Face)n,contextID) = image->getModifiedCount();
                }
            }
        }

    }
    else if (_subloadCallback.valid())
    {
        _textureObjectBuffer[contextID] = textureObject = generateTextureObject(contextID,GL_TEXTURE_CUBE_MAP);

        textureObject->bind();

        applyTexParameters(GL_TEXTURE_CUBE_MAP);

        _subloadCallback->load(*this);

        // in theory the following line is redundent, but in practice
        // have found that the first frame drawn doesn't apply the textures
        // unless a second bind is called?!!
        // perhaps it is the first glBind which is not required...
        //glBindTexture( GL_TEXTURE_CUBE_MAP, handle );

    }
    else if (imagesValid())
    {

        // compute the internal texture format, this set the _internalFormat to an appropriate value.
        computeInternalFormat();

        // compute the dimensions of the texture.
        computeRequiredTextureDimensions(*_images[0],_textureWidth, _textureHeight, _numMipmapLevels);

        // cubemap textures must have square dimensions
        if( _textureWidth != _textureHeight )
        {
            _textureWidth = _textureHeight = std::min( _textureWidth , _textureHeight );
        }

        textureObject = generateTextureObject(
                contextID,GL_TEXTURE_CUBE_MAP,_numMipmapLevels,_internalFormat,_textureWidth,_textureHeight,1,0);

        textureObject->bind();

        applyTexParameters(GL_TEXTURE_CUBE_MAP);

        for (int n=0; n<6; n++)
        {
            const Image* image = _images[n].get();
            if (image)
            {
                if (textureObject->isAllocated())
                {
                    applyTexImage2D_subload(faceTarget[n], image, _textureWidth, _textureHeight, _internalFormat, _numMipmapLevels);
                }
                else
                {
                    applyTexImage2D_load(faceTarget[n], image, _textureWidth, _textureHeight, _numMipmapLevels);
                }
                getModifiedCount((Face)n,contextID) = image->getModifiedCount();
            }


        }

        _textureObjectBuffer[contextID] = textureObject;

        if (_unrefImageDataAfterApply && areAllTextureObjectsLoaded())
        {
            TextureCubeMap* non_const_this = const_cast<TextureCubeMap*>(this);
            for (int n=0; n<6; n++)
            {
                if (_images[n].valid())
                {
                    non_const_this->_images[n] = 0;
                }
            }
        }

    }
    else if ( (_textureWidth!=0) && (_textureHeight!=0) && (_internalFormat!=0) )
    {
        _textureObjectBuffer[contextID] = textureObject = generateTextureObject(
                contextID,GL_TEXTURE_CUBE_MAP,_numMipmapLevels,_internalFormat,_textureWidth,_textureHeight,1,0);

        textureObject->bind();

        applyTexParameters(GL_TEXTURE_CUBE_MAP);

        for (int n=0; n<6; n++)
        {
            // no image present, but dimensions at set so less create the texture
            glTexImage2D( faceTarget[n], 0, _internalFormat,
                         _textureWidth, _textureHeight, _borderWidth,
                         _sourceFormat ? _sourceFormat : _internalFormat,
                         _sourceType ? _sourceType : GL_UNSIGNED_BYTE,
                         0);
        }

    }
    else
    {
        TextureState::Disable();
    }

    // if texture object is now valid and we have to allocate mipmap levels, then
    if (textureObject != 0 && _texMipmapGenerationDirtyList[contextID])
    {
        generateMipmap();
    }
}

void TextureCubeMap::copyTexSubImageCubeMap(int face, int xoffset, int yoffset, int x, int y, int width, int height )
{
    const unsigned int contextID = 0;//state.getContextID();
    const Extensions* extensions = getExtensions(contextID,true);

    if (!extensions->isCubeMapSupported())
        return;

    if (_internalFormat==0) _internalFormat=GL_RGBA;

    // get the texture object for the current contextID.
    TextureObject* textureObject = getTextureObject(contextID);

    if (!textureObject)
    {

        if (_textureWidth==0) _textureWidth = width;
        if (_textureHeight==0) _textureHeight = height;

        // create texture object.
        apply();

        textureObject = getTextureObject(contextID);

        if (!textureObject)
        {
            // failed to create texture object
            Write_Log(LOG_OLD, LOG_INFO, "Warning : failed to create TextureCubeMap texture obeject");
            return;
        }

    }

    GLenum target = faceTarget[face];

    if (textureObject)
    {
        // we have a valid image
        textureObject->bind();

        applyTexParameters(GL_TEXTURE_CUBE_MAP);

        bool needHardwareMipMap = (_min_filter != LINEAR && _min_filter != NEAREST);
        bool hardwareMipMapOn = false;
        if (needHardwareMipMap)
        {
            hardwareMipMapOn = isHardwareMipmapGenerationEnabled();

            if (!hardwareMipMapOn)
            {
                // have to switch off mip mapping
                Write_Log(LOG_OLD, LOG_INFO, "Warning: TextureCubeMap::copyTexImage2D(,,,,) switch off mip mapping as hardware support not available.");
                _min_filter = LINEAR;
            }
        }

        GenerateMipmapMode mipmapResult = mipmapBeforeTexImage(hardwareMipMapOn);

        glCopyTexSubImage2D( target , 0, xoffset, yoffset, x, y, width, height);

        mipmapAfterTexImage(mipmapResult);

        // inform state that this texture is the current one bound.
        //state.haveAppliedTextureAttribute(state.getActiveTextureUnit(), this);

    }
}

void TextureCubeMap::allocateMipmap() const
{
    const unsigned int contextID = 0;//state.getContextID();

    // get the texture object for the current contextID.
    TextureObject* textureObject = getTextureObject(contextID);

    if (textureObject && _textureWidth != 0 && _textureHeight != 0)
    {
        // bind texture
        textureObject->bind();

        // compute number of mipmap levels
        int width = _textureWidth;
        int height = _textureHeight;
        int numMipmapLevels = Image::computeNumberOfMipmapLevels(width, height);

        // we do not reallocate the level 0, since it was already allocated
        width >>= 1;
        height >>= 1;

        for( GLsizei k = 1; k < numMipmapLevels  && (width || height); k++)
        {
            if (width == 0)
                width = 1;
            if (height == 0)
                height = 1;

            for (int n=0; n<6; n++)
            {
                glTexImage2D( faceTarget[n], k, _internalFormat,
                            width, height, _borderWidth,
                            _sourceFormat ? _sourceFormat : _internalFormat,
                            _sourceType ? _sourceType : GL_UNSIGNED_BYTE,
                            0);
            }

            width >>= 1;
            height >>= 1;
        }

        // inform state that this texture is the current one bound.
        //state.haveAppliedTextureAttribute(state.getActiveTextureUnit(), this);
    }
}

using BufferedExtensions = buffered_value<ref_ptr<TextureCubeMap::Extensions>>;
static BufferedExtensions s_extensions;

TextureCubeMap::Extensions* TextureCubeMap::getExtensions(unsigned int contextID,bool createIfNotInitalized)
{
    if (!s_extensions[contextID] && createIfNotInitalized) s_extensions[contextID] = new Extensions(contextID);
    return s_extensions[contextID].get();
}

void TextureCubeMap::setExtensions(unsigned int contextID,Extensions* extensions)
{
    s_extensions[contextID] = extensions;
}

TextureCubeMap::Extensions::Extensions(unsigned int contextID)
{
    setupGLExtensions(contextID);
}

TextureCubeMap::Extensions::Extensions(const Extensions& rhs):
    Referenced()
{
    _isCubeMapSupported = rhs._isCubeMapSupported;
}

void TextureCubeMap::Extensions::lowestCommonDenominator(const Extensions& rhs)
{
    if (!rhs._isCubeMapSupported) _isCubeMapSupported = false;
}

void TextureCubeMap::Extensions::setupGLExtensions(unsigned int contextID)
{
    _isCubeMapSupported = GLEW_ARB_texture_cube_map ||
                          GLEW_EXT_texture_cube_map ||
                          strncmp((const char*)glGetString(GL_VERSION),"1.3",3)>=0;;
}
