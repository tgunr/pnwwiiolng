#pragma once

#include <string>
#include "osgImport/Texture2D.h"
#include "osgImport/Texture2DArray.h"
#include "osgImport/TextureRectangle.h"

extern int ldTextureLoad(const std::string& name, unsigned int flags = 0, unsigned int transparentColor = 0);
extern int ldLoadTexture(const char* name, unsigned int transparentcolor = 0, unsigned int flags = 0);
extern int ldLoadTexture2DArray(const std::string& texturename,
                         const std::vector<std::string>& names,
                         Texture::WrapMode wrap_s = Texture::CLAMP,
                         Texture::WrapMode wrap_t = Texture::CLAMP,
                         Texture::FilterMode min_filter = Texture::LINEAR_MIPMAP_LINEAR,
                         Texture::FilterMode mag_filter = Texture::LINEAR);
extern int ldLoadTextureCubeMap(const std::string& texturename,
                         const std::vector<std::string>& names,
                         Texture::WrapMode wrap_s = Texture::CLAMP,
                         Texture::WrapMode wrap_t = Texture::CLAMP,
                         Texture::FilterMode min_filter = Texture::LINEAR_MIPMAP_LINEAR,
                         Texture::FilterMode mag_filter = Texture::LINEAR);
extern int ldLoadTexture2D(const std::string& texturename,
                    Texture::WrapMode wrap_s = Texture::CLAMP,
                    Texture::WrapMode wrap_t = Texture::CLAMP,
                    Texture::FilterMode min_filter = Texture::LINEAR_MIPMAP_LINEAR,
                    Texture::FilterMode mag_filter = Texture::LINEAR,
                    unsigned int flags = 0);
extern int ldLoadTextureRectangle(const std::string& texturename,
                           Texture::WrapMode wrap_s = Texture::CLAMP,
                           Texture::WrapMode wrap_t = Texture::CLAMP,
                           Texture::FilterMode min_filter = Texture::LINEAR_MIPMAP_LINEAR,
                           Texture::FilterMode mag_filter = Texture::LINEAR,
                           unsigned int flags = 0);
extern void ldTextureRelease(TEXTURE* t);
extern void ldTextureRelease(int id);
extern void ldTextureKill();
extern void ldReloadTextures();

extern Texture2D* ldTextureGet2D(int id);
extern TextureRectangle* ldTextureGetRectangle(int id);
extern Texture2DArray* ldTextureGet2DTextureArray(int id);

extern unsigned int ldTextureGetTextureObject(int id);
extern Image* ldtextureLoadImage(const std::string& name, unsigned int flags, unsigned int transparentColor);

