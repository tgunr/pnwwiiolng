#include "syscfg.h" // Path is project specific.
/*--------------------------------------------------------------------------------
Module: ldtexture.cpp
Description:
--------------------------------------------------------------------------------*/
#include "ogl/ogl.h"
#include "ld/ldpix.h"
#include "ld/ldtexture.h"
#include "osgImport/Referenced.h"
#include "osgImport/Image.h"
#include "osgImport/TextureCubeMap.h"
#include "fio/fio.h"
#include "mem/mem.h"

#include <map>

using ldTextureMap = std::map<int, ref_ptr<Texture>>;
static ldTextureMap* Textures = new ldTextureMap(); // crash on exit on Mac avoided

extern Image* ReadDDSFile(const std::string& name);
extern Image* ReadTGAFile(const std::string& name);
extern Image* ReadBMPFile(const std::string& name, unsigned int transparentColor);
extern std::string optionalFileSuffix;

/*--------------------------------------------------------------------------------
Module: loadImage
Description:
--------------------------------------------------------------------------------*/
static Image* loadImage(const std::string& name, unsigned int flags, unsigned int transparentColor)
{
    ref_ptr<Image> img;

    if(name.find(".dds") != std::string::npos)
    {
        img = ReadDDSFile(name);
    }
    else if(name.find(".tga") != std::string::npos)
    {
        img = ReadTGAFile(name);
    }
    else if(name.find(".bmp") != std::string::npos)
    {
        img = ReadBMPFile(name, transparentColor);
    }

    if(!img.valid() || !img->data())
    {
        return nullptr;
    }

    if(flags & LDPIX_FLIP)
    {
        img->flipVertical();
    }

    if(flags & LDPIX_FLIPHORIZONTAL)
    {
        img->flipHorizontal();
    }

    if(img.valid() && img->data())
    {
        // Don't want this to delete until texture has been created.
        return img.release();
    }

    return nullptr;
}

/*--------------------------------------------------------------------------------
Module: ldtextureLoadImage
Description:
--------------------------------------------------------------------------------*/
Image* ldtextureLoadImage(const std::string& name, unsigned int flags, unsigned int transparentColor)
{
    return loadImage(name, flags, transparentColor);
}

/*--------------------------------------------------------------------------------
Module: loadTexture
Description:
--------------------------------------------------------------------------------*/
static int loadTexture(const std::string& name, unsigned int flags = 0, unsigned int transparentColor = 0, int texid = -1)
{
    std::string str = name;

    // Remove extension
    std::string::size_type pos = str.rfind('.');

    TEXTURETYPE ttype = TEXTURE_NONE;
    std::string ext = ".";
    if(pos != std::string::npos)
    {
        ext = ext + name.substr(pos+1);
        if(name.find(".dds") != std::string::npos)
            ttype = TEXTURE_DDS;
        else if(name.find(".tga") != std::string::npos)
            ttype = TEXTURE_TGA;
        else if(name.find(".bmp") != std::string::npos)
            ttype = TEXTURE_BMP;
        else
            ttype = TEXTURE_NONE;

        str.erase(pos, str.length()-pos);
    }

    if (texid == -1)
    {
        // Already loaded?
        texid = oglGetNamedTexture(str.c_str());

        if(oglIsTexture(texid, 0))
        {
            oglAddTexRef(texid);
            return texid;
        }
    }
    
    // If suffix is defined (theme) and file exists, load that instead
    std::string fileName = name;
    if (!optionalFileSuffix.empty() && fioFileExists(str, optionalFileSuffix, ext))
        fileName = str + optionalFileSuffix + ext;

    // we get lots of attempts to load files that don't exist so lets not output a warning
    if (!fioFileExists(fileName.c_str()))
        return -1;
    
    ref_ptr<Image> img = loadImage(fileName, flags, transparentColor);

    if(!img.valid() || !img->data())
    {
        dbugLog(_FLW, "Error while loading image file: %s", fileName.c_str());
        return -1;
    }

    img->setFileName(fileName);

    oglSetActiveTextureStage(0);

    ref_ptr<Texture> tex = new Texture2D;

    dynamic_cast<Texture2D*>(tex.get())->setImage(img.get());

    if(flags & LDPIX_CLAMP)
    {
        tex->setWrap(Texture::WRAP_S, Texture::CLAMP);
        tex->setWrap(Texture::WRAP_T, Texture::CLAMP);
    }
    else if(flags & LDPIX_CLAMP_TO_EDGE)
    {
        tex->setWrap(Texture::WRAP_S, Texture::CLAMP_TO_EDGE);
        tex->setWrap(Texture::WRAP_T, Texture::CLAMP_TO_EDGE);
    }
    else if(flags & LDPIX_CLAMP_TO_BORDER)
    {
        tex->setWrap(Texture::WRAP_S, Texture::CLAMP_TO_BORDER);
        tex->setWrap(Texture::WRAP_T, Texture::CLAMP_TO_BORDER);
    }
    else
    {
        tex->setWrap(Texture::WRAP_S, Texture::REPEAT);
        tex->setWrap(Texture::WRAP_T, Texture::REPEAT);
    }

    tex->apply();

    if(!tex->getTextureObject(0))
    {
        tex->unref();
        return -1;
    }

    // Put it into our list
    int id = oglGetNewTextureID();

    if(id < 0)
    {
        tex->unref();
        return -1;
    }

    TEXTURE* t = oglGetTextureData2(id);
    t->type = ttype;

    t->newrelease = true;

    t->miplevels = 1;
    t->genlist = tex->getTextureObject(0)->_id;
    t->target = tex->getTextureTarget();
    t->imageheight[0] = tex->getTextureHeight();
    t->imagewidth[0] = tex->getTextureWidth();
    t->po2height[0] = t->imageheight[0];
    t->po2width[0] = t->imagewidth[0];
    t->comp[0] = img->computeNumComponents(img->GetOriginalPixelFormat());
    t->format[0] = img->getPixelFormat();
    t->imagesize[0] = img->getTotalSizeInBytes();
    t->widthfactor[0] = 1.0f;
    t->heightfactor[0] = 1.0f;
    t->hasalpha = t->comp[0] == 4;
    t->hdevtex[0] = 1;
    t->suffix = optionalFileSuffix;
    t->ext = ext;
    t->flags = flags;
    t->transparentColor = transparentColor;
    
    oglSetTextureName(id, str.c_str() ); // make sure index is updated

    (*Textures)[id] = tex;

    if(flags & LDPIX_CREATE_SURFACE_DATA)
    {
        size_t size = 0;

        if(!img->isMipmap())
            size = img->getTotalSizeInBytes();
        else
            size = img->getMipmapOffset(1);

        t->iswritable = true;
        t->lpsurf[0] = memAlloc(size);
        memcpy(t->lpsurf[0], img->data(), size);
    }

  // Image is alreay copied to the card. Remove data to save memory
    dynamic_cast<Texture2D*>(tex.get())->setImage(nullptr);
    assert(img->referenceCount() == 1);

    Write_Debug("[loadTexture] %s texture created", name.c_str());

    return id;
}

/*--------------------------------------------------------------------------------
 Module: loadTexture
 Description:
 --------------------------------------------------------------------------------*/
static int reloadTexture(int texid, TEXTURE *t )
{

    // If suffix is defined (theme) and file exists, load that instead
    std::string fileName = t->name + t->ext;
    if (!optionalFileSuffix.empty() && fioFileExists(t->name, optionalFileSuffix, t->ext))
    {
        fileName = t->name + optionalFileSuffix + t->ext;
        
        // if the suffix hasn't changed, just return - no changes
        if (t->suffix.compare(optionalFileSuffix)==0)
            return 0;
    }
    else
    {
        // if the current image has no suffix, just return - no changes
        return 0;
    }
    
    ref_ptr<Image> img = loadImage(fileName, t->flags, t->transparentColor);
    
    if(!img.valid() || !img->data())
    {
        return -1;
    }
    
    img->setFileName(fileName);
    
    oglSetActiveTextureStage(0);
    
    ref_ptr<Texture> tex = (*Textures)[texid];
    
    dynamic_cast<Texture2D*>(tex.get())->setImage(img.get());
    
    tex->apply();
    
    if(!tex->getTextureObject(0))
    {
        tex->unref();
        return -1;
    }
    
    // Put it into our list
    t->genlist = tex->getTextureObject(0)->_id;
    t->target = tex->getTextureTarget();
    t->imageheight[0] = tex->getTextureHeight();
    t->imagewidth[0] = tex->getTextureWidth();
    t->po2height[0] = t->imageheight[0];
    t->po2width[0] = t->imagewidth[0];
    t->comp[0] = img->computeNumComponents(img->GetOriginalPixelFormat());
    t->format[0] = img->getPixelFormat();
    t->imagesize[0] = img->getTotalSizeInBytes();
    t->widthfactor[0] = 1.0f;
    t->heightfactor[0] = 1.0f;
    t->hasalpha = t->comp[0] == 4;
    t->hdevtex[0] = 1;
    t->suffix = optionalFileSuffix;
    
    if(t->flags & LDPIX_CREATE_SURFACE_DATA)
    {
        size_t size = 0;
        
        if(!img->isMipmap())
            size = img->getTotalSizeInBytes();
        else
            size = img->getMipmapOffset(1);
        
        t->iswritable = true;
        t->lpsurf[0] = memAlloc(size);
        memcpy(t->lpsurf[0], img->data(), size);
    }
    
    // Image is alreay copied to the card. Remove data to save memory
    dynamic_cast<Texture2D*>(tex.get())->setImage(nullptr);
    assert(img->referenceCount() == 1);
    
    Write_Debug("[loadTexture] %s texture reloaded", t->name);
    
    return texid;
}

/*--------------------------------------------------------------------------------
Function: ldTextureLoad
Description:
--------------------------------------------------------------------------------*/
int ldTextureLoad(const std::string& name, unsigned int flags, unsigned int transparentColor)
{
    return loadTexture(name, flags, transparentColor);
}

/*--------------------------------------------------------------------------------
Function: ldLoadTexture
Description:
--------------------------------------------------------------------------------*/
int ldLoadTexture(const char* name, unsigned int transparentcolor, unsigned int flags)
{
    return loadTexture(name, flags, transparentcolor);
}

/*--------------------------------------------------------------------------------
 Function: ldReloadTextures
 Description:
 --------------------------------------------------------------------------------*/
void ldReloadTextures()
{
    ldTextureMap::iterator it;
    for ( it = Textures->begin(); it != Textures->end(); it++) {
        int textureId = it->first;
        TEXTURE* t = oglGetTextureData(textureId);
        if (t != nullptr) {
            std::string fileName = reinterpret_cast<char *>(t->name) + optionalFileSuffix + (t->isdds()?".dds":(t->istga()?".tga":".bmp"));
            
            // if the texture suffix is different than current suffix
            if (t->suffix.compare(optionalFileSuffix) != 0) {
                std::string fName = t->name + t->ext;
                reloadTexture(textureId, t);
//                reloadTexture(fName, flags, tc, textureId);
            }
        }
    }
}

/*--------------------------------------------------------------------------------
Function: ldLoadTexture2D
Description:
--------------------------------------------------------------------------------*/
int ldLoadTexture2D(const std::string& texturename,
                    Texture::WrapMode wrap_s,
                    Texture::WrapMode wrap_t,
                    Texture::FilterMode min_filter,
                    Texture::FilterMode mag_filter,
                    unsigned int flags)
{
    int texid = -1;

    oglSetActiveTextureStage(0);

    std::string str = texturename;

    // Remove extension
    int pos = str.rfind('.');

    if(pos > -1)
    {
        str.erase(pos, str.length()-pos);
    }

    // Already loaded?
    texid = oglGetNamedTexture(str.c_str());

    if(oglIsTexture(texid, 0))
    {
        oglAddTexRef(texid);
        return texid;
    }

    ref_ptr<Image> img = loadImage(texturename, flags, 0);

    if(!img.valid() || !img->data())
    {
        return -1;
    }

    img->setFileName(texturename);

    ref_ptr<Texture> tex = new Texture2D;

    dynamic_cast<Texture2D*>(tex.get())->setImage(img.get());

    tex->setWrap(Texture::WRAP_S, wrap_s);
    tex->setWrap(Texture::WRAP_T, wrap_t);
    tex->setFilter(Texture::MAG_FILTER, mag_filter);
    tex->setFilter(Texture::MIN_FILTER, min_filter);

    tex->apply();

    if(!tex->getTextureObject(0))
    {
        tex->unref();
        return -1;
    }

    // Put it into our list
    int id = oglGetNewTextureID();

    if(id < 0)
    {
        tex->unref();
        return -1;
    }

    TEXTURE* t = oglGetTextureData2(id);

    // TODO: Add more formats
    if(texturename.find(".dds") != std::string::npos)
        t->type = TEXTURE_DDS;
    else if(texturename.find(".tga") != std::string::npos)
        t->type = TEXTURE_TGA;
    else if(texturename.find(".bmp") != std::string::npos)
        t->type = TEXTURE_BMP;
    else
        t->type = TEXTURE_NONE;

    t->newrelease = true;

    t->miplevels = 1;
    t->genlist = tex->getTextureObject(0)->_id;
    t->target = tex->getTextureTarget();
    t->imageheight[0] = tex->getTextureHeight();
    t->imagewidth[0] = tex->getTextureWidth();
    t->po2height[0] = t->imageheight[0];
    t->po2width[0] = t->imagewidth[0];
    t->comp[0] = img->computeNumComponents(img->getPixelFormat());
    t->format[0] = img->getPixelFormat();
    t->imagesize[0] = img->getTotalSizeInBytes();
    t->widthfactor[0] = 1.0f;
    t->heightfactor[0] = 1.0f;
    t->hasalpha = t->comp[0] == 4;
    t->hdevtex[0] = 1;

    // Don't copy extension
    std::string s(texturename);
    pos = s.rfind('.');

    if(pos > -1)
    {
        s.erase(pos, s.length() - pos);
    }

    oglSetTextureName(id, s.c_str() ); // make sure index is updated

    (*Textures)[id] = tex;

    if(flags & LDPIX_CREATE_SURFACE_DATA)
    {
        int size = 0;

        if(!img->isMipmap())
            size = img->getTotalSizeInBytes();
        else
            size = img->getMipmapOffset(1);

        t->iswritable = true;
        t->lpsurf[0] = memAlloc(size);
        memcpy(t->lpsurf[0], img->data(), size);
    }

   // Image is alreay copied to the card. Remove data to save memory
    dynamic_cast<Texture2D*>(tex.get())->setImage(nullptr);
    assert(img->referenceCount() == 1);

    Write_Debug("[loadTexture] %s texture created", texturename.c_str());

    return id;
}

/*--------------------------------------------------------------------------------
Function: ldLoadTextureRectangle
Description:
--------------------------------------------------------------------------------*/
int ldLoadTextureRectangle(const std::string& texturename,
                           Texture::WrapMode wrap_s,
                           Texture::WrapMode wrap_t,
                           Texture::FilterMode min_filter,
                           Texture::FilterMode mag_filter,
                           unsigned int flags)
{
    int texid = -1;

    oglSetActiveTextureStage(0);

    std::string str = texturename;

    // Remove extension
    int pos = str.rfind('.');

    if(pos > -1)
    {
        str.erase(pos, str.length()-pos);
    }

    // Already loaded?
    texid = oglGetNamedTexture(str.c_str());

    if(oglIsTexture(texid, 0))
    {
        oglAddTexRef(texid);
        return texid;
    }

    ref_ptr<Image> img = loadImage(texturename, flags, 0);

    if(!img.valid() || !img->data())
    {
        return -1;
    }

    img->setFileName(texturename);

    ref_ptr<Texture> tex = new TextureRectangle;

    dynamic_cast<TextureRectangle*>(tex.get())->setImage(img.get());

    tex->setWrap(Texture::WRAP_S, wrap_s);
    tex->setWrap(Texture::WRAP_T, wrap_t);
    tex->setFilter(Texture::MAG_FILTER, mag_filter);
    tex->setFilter(Texture::MIN_FILTER, min_filter);

    tex->apply();

    if(!tex->getTextureObject(0))
    {
        tex->unref();
        return -1;
    }

    // Put it into our list
    int id = oglGetNewTextureID();

    if(id < 0)
    {
        tex->unref();
        return -1;
    }

    TEXTURE* t = oglGetTextureData2(id);

    // TODO: Add more formats
    if(texturename.find(".dds") != std::string::npos)
        t->type = TEXTURE_DDS;
    else if(texturename.find(".tga") != std::string::npos)
        t->type = TEXTURE_TGA;
    else if(texturename.find(".bmp") != std::string::npos)
        t->type = TEXTURE_BMP;
    else
        t->type = TEXTURE_NONE;

    t->newrelease = true;

    t->miplevels = 1;
    t->genlist = tex->getTextureObject(0)->_id;
    t->target = tex->getTextureTarget();
    t->imageheight[0] = tex->getTextureHeight();
    t->imagewidth[0] = tex->getTextureWidth();
    t->po2height[0] = t->imageheight[0];
    t->po2width[0] = t->imagewidth[0];
    t->comp[0] = img->computeNumComponents(img->getPixelFormat());
    t->format[0] = img->getPixelFormat();
    t->imagesize[0] = img->getTotalSizeInBytes();
    t->widthfactor[0] = 1.0f;
    t->heightfactor[0] = 1.0f;
    t->hasalpha = t->comp[0] == 4;
    t->hdevtex[0] = 1;

    // Don't copy extension
    std::string s(texturename);
    pos = s.rfind('.');

    if(pos > -1)
    {
        s.erase(pos, s.length() - pos);
    }

    oglSetTextureName(id, s.c_str() ); // make sure index is updated

    (*Textures)[id] = tex;

    if(flags & LDPIX_CREATE_SURFACE_DATA)
    {
        int size = 0;

        if(!img->isMipmap())
            size = img->getTotalSizeInBytes();
        else
            size = img->getMipmapOffset(1);

        t->iswritable = true;
        t->lpsurf[0] = memAlloc(size);
        memcpy(t->lpsurf[0], img->data(), size);
    }

   // Image is alreay copied to the card. Remove data to save memory
    dynamic_cast<TextureRectangle*>(tex.get())->setImage(nullptr);
    assert(img->referenceCount() == 1);

    Write_Debug("[loadTexture] %s texture created", texturename.c_str());

    return id;
}

/*--------------------------------------------------------------------------------
Function: ldLoadTexture2DArray
Description:
--------------------------------------------------------------------------------*/
int ldLoadTexture2DArray(const std::string& texturename,
                         const std::vector<std::string>& names,
                         Texture::WrapMode wrap_s,
                         Texture::WrapMode wrap_t,
                         Texture::FilterMode min_filter,
                         Texture::FilterMode mag_filter)
{
    oglSetActiveTextureStage(0);

    ref_ptr<Texture> tex = new Texture2DArray;

    dynamic_cast<Texture2DArray*>(tex.get())->setTextureDepth(names.size());

    for(unsigned int i = 0; i < names.size(); ++i)
    {
        ref_ptr<Image> img = loadImage(names[i], 0, 0);

        if(!img.valid())
        {
            tex->unref();
            return -1;
        }

        dynamic_cast<Texture2DArray*>(tex.get())->setImage(i, img.get());
    }

    tex->setWrap(Texture::WRAP_S, wrap_s);
    tex->setWrap(Texture::WRAP_T, wrap_t);
    tex->setFilter(Texture::MAG_FILTER, mag_filter);
    tex->setFilter(Texture::MIN_FILTER, min_filter);

    tex->apply();

    if(!tex->getTextureObject(0))
    {
        tex->unref();
        return 0;
    }

    // Put it into our list
    int id = oglGetNewTextureID();

    if(id < 0)
    {
        tex->unref();
        return -1;
    }

    TEXTURE* t = oglGetTextureData2(id);
    t->type = TEXTURE_NONE;

    t->newrelease = true;

    t->miplevels = 1;
    t->genlist = tex->getTextureObject(0)->_id;
    t->target = tex->getTextureTarget();
    t->imageheight[0] = tex->getTextureHeight();
    t->imagewidth[0] = tex->getTextureWidth();
    t->po2height[0] = t->imageheight[0];
    t->po2width[0] = t->imagewidth[0];
    t->widthfactor[0] = 1.0f;
    t->heightfactor[0] = 1.0f;
    t->hasalpha = false;
    t->hdevtex[0] = 1;

    oglSetTextureName(id, texturename.c_str() ); // make sure index is updated

    (*Textures)[id] = tex;

    // Remove data since it has been unloaded to the card.
    for(unsigned int i = 0; i < names.size(); ++i)
    {
        tex->setImage(i, nullptr);
    }

    return id;
}

/*--------------------------------------------------------------------------------
Function: ldLoadTextureCubeMap
Description: Follows the order in TextureCubeMap.h
--------------------------------------------------------------------------------*/
int ldLoadTextureCubeMap(const std::string& texturename,
                         const std::vector<std::string>& names,
                         Texture::WrapMode wrap_s,
                         Texture::WrapMode wrap_t,
                         Texture::FilterMode max_filter,
                         Texture::FilterMode min_filter)
{
    oglSetActiveTextureStage(0);

    ref_ptr<Texture> tex = new TextureCubeMap;

    for(unsigned int i = 0; i < names.size(); ++i)
    {
        ref_ptr<Image> img = loadImage(names[i], 0, 0);

        if(!img.valid())
        {
            tex->unref();
            return -1;
        }

        dynamic_cast<TextureCubeMap*>(tex.get())->setImage(i, img.get());
    }

    tex->setWrap(Texture::WRAP_S, wrap_s);
    tex->setWrap(Texture::WRAP_T, wrap_t);
    tex->setFilter(Texture::MAG_FILTER, max_filter);
    tex->setFilter(Texture::MIN_FILTER, min_filter);

    tex->apply();

    if(!tex->getTextureObject(0))
    {
        tex->unref();
        return 0;
    }

    // Put it into our list
    int id = oglGetNewTextureID();

    if(id < 0)
    {
        tex->unref();
        return -1;
    }

    TEXTURE* t = oglGetTextureData2(id);
    t->type = TEXTURE_NONE;

    t->newrelease = true;

    t->miplevels = 1;
    t->genlist = tex->getTextureObject(0)->_id;
    t->target = tex->getTextureTarget();
    t->imageheight[0] = tex->getTextureHeight();
    t->imagewidth[0] = tex->getTextureWidth();
    t->po2height[0] = t->imageheight[0];
    t->po2width[0] = t->imagewidth[0];
    t->widthfactor[0] = 1.0f;
    t->heightfactor[0] = 1.0f;
    t->hasalpha = false;
    t->hdevtex[0] = 1;

    oglSetTextureName(id, texturename.c_str() ); // make sure index is updated

    (*Textures)[id] = tex;

    // Remove data since it has been unloaded to the card.
    for(unsigned int i = 0; i < names.size(); ++i)
    {
        tex->setImage(i, nullptr);
    }

    return id;
}

/*--------------------------------------------------------------------------------
Function: ldTextureKill
Description: Just delete texture objects. Application will call for the release
--------------------------------------------------------------------------------*/
void ldTextureKill()
{
    ldTextureMap::iterator itr = Textures->begin();

    for(; itr != Textures->end(); ++itr)
    {
        TEXTURE* t = oglGetTextureData(itr->first);

        if(t && t->genlist > 0)
        {
            // Only delete texture objects here. These are reused during run-time and cannot
            // be deleted elsewhere.
            glDeleteTextures(1, (const GLuint*)&t->genlist);
            t->genlist = 0;
        }
    }

    Textures->clear();

    delete Textures;
}

/*--------------------------------------------------------------------------------
Module: ldTextureRelease
Description:
--------------------------------------------------------------------------------*/
void ldTextureRelease(int id)
{
    ldTextureRelease(oglGetTextureData(id));
}

/*--------------------------------------------------------------------------------
Module: ldTextureRelease
Description:
--------------------------------------------------------------------------------*/
void ldTextureRelease(TEXTURE* t)
{
    if(t == nullptr)
        return;

    int id = t->id - 1;

    std::map<int, ref_ptr<Texture> >::iterator itr;

    itr = Textures->find(id);

    if(itr != Textures->end())
    {
        Textures->erase(itr);
    }
}

/*--------------------------------------------------------------------------------
Module: ldTextureGet2D
Description:
--------------------------------------------------------------------------------*/
Texture2D* ldTextureGet2D(int id)
{
    std::map<int, ref_ptr<Texture> >::iterator itr;

    itr = Textures->find(id);

    if(itr != Textures->end())
    {
        return dynamic_cast<Texture2D*>(itr->second.get());
    }

    return nullptr;
}

/*--------------------------------------------------------------------------------
Module: ldTextureGetRectangle
Description:
--------------------------------------------------------------------------------*/
TextureRectangle* ldTextureGetRectangle(int id)
{
    std::map<int, ref_ptr<Texture> >::iterator itr;

    itr = Textures->find(id);

    if(itr != Textures->end())
    {
        return dynamic_cast<TextureRectangle*>(itr->second.get());
    }

    return nullptr;
}

/*--------------------------------------------------------------------------------
Module: ldTextureGet2DTextureArray
Description:
--------------------------------------------------------------------------------*/
Texture2DArray* ldTextureGet2DTextureArray(int id)
{
    std::map<int, ref_ptr<Texture> >::iterator itr;

    itr = Textures->find(id);

    if(itr != Textures->end())
    {
        return dynamic_cast<Texture2DArray*>(itr->second.get());
    }

    return nullptr;
}

/*--------------------------------------------------------------------------------
Module: ldTextureGetTextureObject
Description:
--------------------------------------------------------------------------------*/
unsigned int ldTextureGetTextureObject(int id)
{
    std::map<int, ref_ptr<Texture> >::iterator itr;

    itr = Textures->find(id);

    if(itr != Textures->end())
    {
        return itr->second->getTextureObject(0)->_id;
    }

    return 0;
}
