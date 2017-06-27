#include "syscfg.h" // Path is project specific.
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
#include "osgImport/Texture2D.h"
#include "ogl/States.h"

Texture2D::Texture2D():
            _textureWidth(0),
            _textureHeight(0),
            _numMipmapLevels(0)
{
    setUseHardwareMipMapGeneration(true);
}

Texture2D::Texture2D(Image* image):
            _textureWidth(0),
            _textureHeight(0),
            _numMipmapLevels(0)
{
    setUseHardwareMipMapGeneration(true);
    setImage(image);
}

Texture2D::~Texture2D()
{
}

void Texture2D::setImage(Image* image)
{
    if (_image == image) return;

#if(0)
    if (dynamic_cast<ImageSequence*>(_image.ptr()))
    {
        setUpdateCallback(0);
        setDataVariance(osg::Object::STATIC);
    }
#endif

    _image = image;
    _modifiedCount.setAllElementsTo(0);

#if(0)
    if (dynamic_cast<osg::ImageSequence*>(_image.get()))
    {
        setUpdateCallback(new ImageSequence::UpdateCallback());
        setDataVariance(osg::Object::DYNAMIC);
    }
#endif
}

void Texture2D::apply() const
{

    //state.setReportGLErrors(true);

    // get the contextID (user defined ID of 0 upwards) for the
    // current OpenGL context.
    const unsigned int contextID = 0;//state.getContextID();

    // get the texture object for the current contextID.
    TextureObject* textureObject = getTextureObject(contextID);

    if (textureObject != 0)
    {
        textureObject->bind();

        if (getTextureParameterDirty(contextID))
            applyTexParameters(GL_TEXTURE_2D);

        if (_subloadCallback.valid())
        {
            _subloadCallback->subload(*this);
        }
        else if (_image.valid() && getModifiedCount(contextID) != _image->getModifiedCount())
        {
            applyTexImage2D_subload(GL_TEXTURE_2D,
                                    _image.get(),
                                    _textureWidth,
                                    _textureHeight,
                                    _internalFormat,
                                    _numMipmapLevels);

            // update the modified tag to show that it is up to date.
            getModifiedCount(contextID) = _image->getModifiedCount();

        }
#if(0)
        else if (_readPBuffer.valid())
        {
            _readPBuffer->bindPBufferToTexture(GL_FRONT);
        }
#endif
    }
    else if (_subloadCallback.valid())
    {

        _textureObjectBuffer[contextID] = textureObject = generateTextureObject(contextID,GL_TEXTURE_2D);

        textureObject->bind();

        applyTexParameters(GL_TEXTURE_2D);

        _subloadCallback->load(*this);

        textureObject->setAllocated(_numMipmapLevels,_internalFormat,_textureWidth,_textureHeight,1,0);

        // in theory the following line is redundent, but in practice
        // have found that the first frame drawn doesn't apply the textures
        // unless a second bind is called?!!
        // perhaps it is the first glBind which is not required...
        //glBindTexture( GL_TEXTURE_2D, handle );

    }
    else if (_image.valid() && _image->data())
    {

        // keep the image around at least till we go out of scope.
        ref_ptr<Image> image = _image;

        // compute the internal texture format, this set the _internalFormat to an appropriate value.
        computeInternalFormat();

        // compute the dimensions of the texture.
        computeRequiredTextureDimensions(*image, _textureWidth, _textureHeight, _numMipmapLevels);


        textureObject = generateTextureObject(
                contextID,GL_TEXTURE_2D,_numMipmapLevels,_internalFormat,_textureWidth,_textureHeight,1,0);

        textureObject->bind();

        applyTexParameters(GL_TEXTURE_2D);

        if (textureObject->isAllocated())
        {
            //notify(NOTICE)<<"Reusing texture object"<<std::endl;
            applyTexImage2D_subload(GL_TEXTURE_2D,
                                    image.get(),
                                   _textureWidth,
                                   _textureHeight,
                                   _internalFormat,
                                   _numMipmapLevels);
        }
        else
        {
            //notify(NOTICE)<<"Creating new texture object"<<std::endl;
            applyTexImage2D_load(GL_TEXTURE_2D,
                                 image.get(),
                                 _textureWidth,
                                 _textureHeight,
                                 _numMipmapLevels);

            textureObject->setAllocated(true);
        }


        // update the modified tag to show that it is upto date.
        getModifiedCount(contextID) = image->getModifiedCount();

        _textureObjectBuffer[contextID] = textureObject;

#if(0)
        if (_unrefImageDataAfterApply && areAllTextureObjectsLoaded() && image.ptr()->getDataVariance()==STATIC)
        {
            Texture2D* non_const_this = const_cast<Texture2D*>(this);
            non_const_this->_image = 0;
        }
#endif

        // in theory the following line is redundent, but in practice
        // have found that the first frame drawn doesn't apply the textures
        // unless a second bind is called?!!
        // perhaps it is the first glBind which is not required...
        //glBindTexture( GL_TEXTURE_2D, handle );

    }
    else if ( (_textureWidth!=0) && (_textureHeight!=0) && (_internalFormat!=0) )
    {
        _textureObjectBuffer[contextID] = textureObject = generateTextureObject(
                contextID,GL_TEXTURE_2D,_numMipmapLevels,_internalFormat,_textureWidth,_textureHeight,1,0);

        textureObject->bind();

        applyTexParameters(GL_TEXTURE_2D);

        // no image present, but dimensions at set so lets create the texture
        glTexImage2D( GL_TEXTURE_2D, 0, _internalFormat,
                     _textureWidth, _textureHeight, _borderWidth,
                     _sourceFormat ? _sourceFormat : _internalFormat,
                     _sourceType ? _sourceType : GL_UNSIGNED_BYTE,
                     0);

#if(0)
        if (_readPBuffer.valid())
        {
            _readPBuffer->bindPBufferToTexture(GL_FRONT);
        }
#endif
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

void Texture2D::computeInternalFormat() const
{
    if(_image.valid())
    {
        computeInternalFormatWithImage(*_image);
    }
    else
    {
        computeInternalFormatType();
    }
}

void Texture2D::copyTexImage2D(int x, int y, int width, int height )
{
    const unsigned int contextID = 0;//state.getContextID();

    if (_internalFormat==0) _internalFormat=GL_RGBA;

    // get the globj for the current contextID.
    TextureObject* textureObject = getTextureObject(contextID);

    if (textureObject)
    {
        if (width==(int)_textureWidth && height==(int)_textureHeight)
        {
            // we have a valid texture object which is the right size
            // so lets play clever and use copyTexSubImage2D instead.
            // this allows use to reuse the texture object and avoid
            // expensive memory allocations.
            copyTexSubImage2D(0 ,0, x, y, width, height);
            return;
        }
        // the relevent texture object is not of the right size so
        // needs to been deleted
        // remove previously bound textures.
        dirtyTextureObject();
        // note, dirtyTextureObject() dirties all the texture objects for
        // this texture, is this right?  Perhaps we should dirty just the
        // one for this context.  Note sure yet will leave till later.
        // RO July 2001.
    }


    // remove any previously assigned images as these are nolonger valid.
    _image = nullptr;

    // switch off mip-mapping.
    //
    _textureObjectBuffer[contextID] = textureObject = generateTextureObject(contextID,GL_TEXTURE_2D);

    textureObject->bind();

    applyTexParameters(GL_TEXTURE_2D);


    bool needHardwareMipMap = (_min_filter != LINEAR && _min_filter != NEAREST);
    bool hardwareMipMapOn = false;
    if (needHardwareMipMap)
    {
        hardwareMipMapOn = isHardwareMipmapGenerationEnabled();

        if (!hardwareMipMapOn)
        {
            // have to switch off mip mapping
            //notify(NOTICE)<<"Warning: Texture2D::copyTexImage2D(,,,,) switch off mip mapping as hardware support not available."<<std::endl;
            _min_filter = LINEAR;
        }
    }

    GenerateMipmapMode mipmapResult = mipmapBeforeTexImage(hardwareMipMapOn);

    glCopyTexImage2D( GL_TEXTURE_2D, 0, _internalFormat, x, y, width, height, 0 );

    mipmapAfterTexImage(mipmapResult);

    _textureWidth = width;
    _textureHeight = height;
    _numMipmapLevels = 1;

    textureObject->setAllocated(_numMipmapLevels,_internalFormat,_textureWidth,_textureHeight,1,0);
}

void Texture2D::copyTexSubImage2D(int xoffset, int yoffset, int x, int y, int width, int height )
{
    const unsigned int contextID = 0;//state.getContextID();

    if (_internalFormat==0) _internalFormat=GL_RGBA;

    // get the texture object for the current contextID.
    TextureObject* textureObject = getTextureObject(contextID);

    if (textureObject)
    {
        // we have a valid image
        textureObject->bind();

        applyTexParameters(GL_TEXTURE_2D);

        bool needHardwareMipMap = (_min_filter != LINEAR && _min_filter != NEAREST);
        bool hardwareMipMapOn = false;
        if (needHardwareMipMap)
        {
            hardwareMipMapOn = isHardwareMipmapGenerationEnabled();

            if (!hardwareMipMapOn)
            {
                // have to switch off mip mapping
               // notify(NOTICE)<<"Warning: Texture2D::copyTexImage2D(,,,,) switch off mip mapping as hardware support not available."<<std::endl;
                _min_filter = LINEAR;
            }
        }

        GenerateMipmapMode mipmapResult = mipmapBeforeTexImage(hardwareMipMapOn);

        glCopyTexSubImage2D( GL_TEXTURE_2D, 0, xoffset, yoffset, x, y, width, height);

        mipmapAfterTexImage(mipmapResult);
    }
    else
    {
        // no texture object already exsits for this context so need to
        // create it upfront - simply call copyTexImage2D.
        copyTexImage2D(x,y,width,height);
    }
}

void Texture2D::allocateMipmap() const
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

            glTexImage2D( GL_TEXTURE_2D, k, _internalFormat,
                     width, height, _borderWidth,
                     _sourceFormat ? _sourceFormat : _internalFormat,
                     _sourceType ? _sourceType : GL_UNSIGNED_BYTE, nullptr);

            width >>= 1;
            height >>= 1;
        }
    }
}

