#include <syscfg.h> // Path is project specific.
/**********************************************************************
*
*    FILE:            ReaderWriterdds.cpp
*
*    DESCRIPTION:    Class for reading a DDS file into an osg::Image.
*
*                    Example on reading a DDS file code can be found at:
*                    http://developer.nvidia.com/docs/IO/1327/ATT/
*                    ARB_texture_compression.pdf
*                    Author: Sebastien Domine, NVIDIA Corporation
*
*    CREATED BY:        Rune Schmidt Jensen, rsj@uni-dk
*
*    HISTORY:        Created   31.03.2003
*             Modified  13.05.2004
*                by George Tarantilis, gtaranti@nps.navy.mil
*
**********************************************************************/

#include "osgImport/Texture.h"
#include "osgImport/ref_ptr.h"
#include "osgImport/Image.h"

#include <iomanip>

#include "fio/fio.h"

// NOTICE ON WIN32:
// typedef DWORD unsigned long;
// sizeof(DWORD) = 4

typedef unsigned int UI32;
typedef int I32;

struct  DDCOLORKEY
{
    DDCOLORKEY():
        dwColorSpaceLowValue(0),
        dwColorSpaceHighValue(0) {}

    UI32    dwColorSpaceLowValue;
    UI32    dwColorSpaceHighValue;
};

struct DDPIXELFORMAT
{

    DDPIXELFORMAT():
        dwSize(0),
        dwFlags(0),
        dwFourCC(0),
        dwRGBBitCount(0),
        dwRBitMask(0),
        dwGBitMask(0),
        dwBBitMask(0),
        dwRGBAlphaBitMask(0) {}


    UI32    dwSize;
    UI32    dwFlags;
    UI32    dwFourCC;
    union
    {
        UI32    dwRGBBitCount;
        UI32    dwYUVBitCount;
        UI32    dwZBufferBitDepth;
        UI32    dwAlphaBitDepth;
        UI32    dwLuminanceBitDepth;
    };
    union
    {
        UI32    dwRBitMask;
        UI32    dwYBitMask;
    };
    union
    {
        UI32    dwGBitMask;
        UI32    dwUBitMask;
    };
    union
    {
        UI32    dwBBitMask;
        UI32    dwVBitMask;
    };
    union
    {
        UI32    dwRGBAlphaBitMask;
        UI32    dwYUVAlphaBitMask;
        UI32    dwRGBZBitMask;
        UI32    dwYUVZBitMask;
    };
};

struct  DDSCAPS2
{
     DDSCAPS2():
        dwCaps(0),
        dwCaps2(0),
        dwCaps3(0),
        dwCaps4(0) {}

    UI32       dwCaps;
    UI32       dwCaps2;
    UI32       dwCaps3;
    union
    {
        UI32       dwCaps4;
        UI32       dwVolumeDepth;
    };
};

struct DDSURFACEDESC2
{
    DDSURFACEDESC2():
        dwSize(0),
        dwFlags(0),
        dwHeight(0),
        dwWidth(0),
        lPitch(0),
        dwBackBufferCount(0),
        dwMipMapCount(0),
        dwAlphaBitDepth(0),
        dwReserved(0),
        lpSurface(0),
        dwTextureStage(0) {}


    UI32         dwSize;
    UI32         dwFlags;
    UI32         dwHeight;
    UI32         dwWidth;
    union
    {
        I32              lPitch;
        UI32     dwLinearSize;
    };
    union
    {
        UI32      dwBackBufferCount;
        UI32      dwDepth;
    };
    union
    {
        UI32     dwMipMapCount;
        UI32     dwRefreshRate;
    };
    UI32         dwAlphaBitDepth;
    UI32         dwReserved;
    UI32        lpSurface;         //Fred Marmond: removed from pointer type to UI32 for 64bits compatibility. it is unused data
    DDCOLORKEY    ddckCKDestOverlay;
    DDCOLORKEY    ddckCKDestBlt;
    DDCOLORKEY    ddckCKSrcOverlay;
    DDCOLORKEY    ddckCKSrcBlt;
    DDPIXELFORMAT ddpfPixelFormat;
    DDSCAPS2      ddsCaps;
    UI32 dwTextureStage;
};

//
// Structure of a DXT-1 compressed texture block
// see http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directx9_c/Opaque_and_1_Bit_Alpha_Textures.asp
//
struct DXT1TexelsBlock
{
    unsigned short color_0;     // colors at their
    unsigned short color_1;     // extreme
    unsigned int   texels4x4;   // interpolated colors (2 bits per texel)
};

//
// DDSURFACEDESC2 flags that mark the validity of the struct data
//
#define DDSD_CAPS               0x00000001l     // default
#define DDSD_HEIGHT             0x00000002l        // default
#define DDSD_WIDTH              0x00000004l        // default
#define DDSD_PIXELFORMAT        0x00001000l        // default
#define DDSD_PITCH              0x00000008l     // For uncompressed formats
#define DDSD_MIPMAPCOUNT        0x00020000l
#define DDSD_LINEARSIZE         0x00080000l     // For compressed formats
#define DDSD_DEPTH              0x00800000l        // Volume Textures

//
// DDPIXELFORMAT flags
//
#define DDPF_ALPHAPIXELS        0x00000001l
#define DDPF_FOURCC             0x00000004l        // Compressed formats
#define DDPF_RGB                0x00000040l        // Uncompressed formats
#define DDPF_ALPHA              0x00000002l
#define DDPF_COMPRESSED         0x00000080l
#define DDPF_LUMINANCE          0x00020000l
#define DDPF_BUMPLUMINANCE      0x00040000l        // L,U,V
#define DDPF_BUMPDUDV           0x00080000l        // U,V

//
// DDSCAPS flags
//
#define DDSCAPS_TEXTURE         0x00001000l     // default
#define DDSCAPS_COMPLEX         0x00000008l
#define DDSCAPS_MIPMAP          0x00400000l
#define DDSCAPS2_VOLUME         0x00200000l


#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
    ((UI32)(char)(ch0) | ((UI32)(char)(ch1) << 8) |   \
    ((UI32)(char)(ch2) << 16) | ((UI32)(char)(ch3) << 24))
#endif //defined(MAKEFOURCC)

/*
* FOURCC codes for DX compressed-texture pixel formats
*/
#define FOURCC_DXT1  (MAKEFOURCC('D','X','T','1'))
#define FOURCC_DXT2  (MAKEFOURCC('D','X','T','2'))
#define FOURCC_DXT3  (MAKEFOURCC('D','X','T','3'))
#define FOURCC_DXT4  (MAKEFOURCC('D','X','T','4'))
#define FOURCC_DXT5  (MAKEFOURCC('D','X','T','5'))

static void endianSwap32(unsigned char* c)
{
	(c)[0] ^= (c)[3]; (c)[3] ^= (c)[0]; (c)[0] ^= (c)[3];
	(c)[1] ^= (c)[2]; (c)[2] ^= (c)[1]; (c)[1] ^= (c)[2];
}

static int isLittleEndianMachine()
{
    int a = 1;
    return (int)(*(char*)&a);
}

static void endianSwapBlock32(long* l, int  n)
{
	for(int i = 0; i < n; i++, l++)
		endianSwap32((unsigned char*)l);
}

Image* ReadDDSFile(const std::string& name)
{
    DDSURFACEDESC2 ddsd;

    Write_Log(LOG_TEXTURE, LOG_INFO, "[ReadDDSFile] Reading image %s", name.c_str());

    int fd = -1;
    fd = fioOpen(name.c_str());

    if(fd < 0)
    {
        Write_Debug("[ReadDDSFile] cannot open %s", name.c_str());
        return nullptr;
    }

    char filecode[4];

    fioRead(fd, filecode, 4);

    if(strncmp(filecode, "DDS ", 4) != 0)
    {
        Write_Debug("[ReadDDSFile] File %s not DDS", name.c_str());
        fioClose(fd);
        return nullptr;
    }

    // Get the surface desc.
    fioRead(fd, &ddsd, sizeof(ddsd));

	if(!isLittleEndianMachine())
    {
		endianSwapBlock32((long*)&ddsd, sizeof(ddsd)/4);
    }

    ref_ptr<Image> image = new Image;

    //Check valid structure sizes
    if(ddsd.dwSize != 124 && ddsd.ddpfPixelFormat.dwSize != 32)
    {
        Write_Debug("[ReadDDSFile] File %s not valid structure size", name.c_str());
        fioClose(fd);
        return nullptr;
    }

    bool is3dImage = false;
    int depth = 1;

    // Check for volume image
    if(ddsd.dwDepth > 0 && (ddsd.dwFlags & DDSD_DEPTH))
    {
        is3dImage = true;
        depth = ddsd.dwDepth;
    }

    // Retreive image properties.
    int s = ddsd.dwWidth;
    int t = ddsd.dwHeight;
    int r = depth;

    unsigned int dataType = GL_UNSIGNED_BYTE;
    unsigned int pixelFormat = 0;
    unsigned int internalFormat = 0;

    // Handle some esoteric formats
    if(ddsd.ddpfPixelFormat.dwFlags & DDPF_BUMPDUDV)
    {
        fioClose(fd);
        Write_Log(LOG_OLD, LOG_WARN, "ERROR: DDPF_BUMPDUDV format is not supported");
        return nullptr;
    }

    if(ddsd.ddpfPixelFormat.dwFlags & DDPF_BUMPLUMINANCE)
    {
        fioClose(fd);
        Write_Log(LOG_OLD, LOG_WARN, "ERROR: DDPF_BUMPLUMINANCE format is not supported");
        return nullptr;
    }

    // Uncompressed formats will usually use DDPF_RGB to indicate an RGB format,
    // while compressed formats will use DDPF_FOURCC with a four-character code.

    bool usingAlpha = ddsd.ddpfPixelFormat.dwFlags & DDPF_ALPHAPIXELS;
    bool checkIfUsingOneBitAlpha = false;

    // Uncompressed formats.
    if(ddsd.ddpfPixelFormat.dwFlags & DDPF_RGB)
    {
        struct RGBFormat
        {
            const char*  name;
            UI32 bitCount;
            UI32 rBitMask;
            UI32 gBitMask;
            UI32 bBitMask;
            UI32 aBitMask;
            unsigned int internalFormat;
            unsigned int pixelFormat;
            unsigned int dataType;
        };

        const unsigned int UNSUPPORTED = 0;

        static const RGBFormat rgbFormats[] =
        {
            {"R3G3B2", 8, 0xe0, 0x1c, 0x03, 0x00, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE_3_3_2},
            {"R5G6B5", 16, 0xf800, 0x07e0, 0x001f, 0x0000, GL_RGB , GL_RGB , GL_UNSIGNED_SHORT_5_6_5},
            {"A1R5G5B5", 16, 0x7c00, 0x03e0, 0x001f, 0x8000, GL_RGBA, GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV},
            {"X1R5G5B5", 16, 0x7c00, 0x03e0, 0x001f, 0x0000, GL_RGB , GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV},
            {"A4R4G4B4", 16, 0x0f00, 0x00f0, 0x000f, 0xf000, GL_RGBA, GL_BGRA, GL_UNSIGNED_SHORT_4_4_4_4_REV},
            {"X4R4G4B4", 16, 0x0f00, 0x00f0, 0x000f, 0x0000, GL_RGB , GL_BGRA, GL_UNSIGNED_SHORT_4_4_4_4_REV},
            {"A8R3G3B2", 16, 0x00e0, 0x001c, 0x0003, 0xff00, GL_RGBA, GL_BGRA, UNSUPPORTED},
            {"R8G8B8", 24, 0xff0000, 0x00ff00, 0x0000ff, 0x000000, GL_RGB, GL_BGR, GL_UNSIGNED_BYTE},
            {"B8G8R8", 24, 0x0000ff, 0x00ff00, 0xff0000, 0x000000, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE},
            {"A8R8G8B8", 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000, GL_RGBA, GL_BGRA, GL_UNSIGNED_BYTE},
            {"X8R8G8B8", 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000, GL_RGB , GL_BGRA, GL_UNSIGNED_BYTE},
            {"A8B8G8R8", 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE},
            {"X8B8G8R8", 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0x00000000, GL_RGB , GL_RGBA, GL_UNSIGNED_BYTE},
            {"A2R10G10B10", 32, 0x000003ff, 0x000ffc00, 0x3ff00000, 0xc0000000, GL_RGBA, GL_BGRA, GL_UNSIGNED_INT_2_10_10_10_REV},
            {"A2B10G10R10", 32, 0x3ff00000, 0x000ffc00, 0x000003ff, 0xc0000000, GL_RGBA, GL_RGBA, GL_UNSIGNED_INT_2_10_10_10_REV},
            {"G16R16", 32, 0x0000ffff, 0xffff0000, 0x00000000, 0x00000000, GL_RGB, UNSUPPORTED, GL_UNSIGNED_SHORT},
        };

        bool found = false;

        for(unsigned int i = 0; i < sizeof(rgbFormats) / sizeof ( RGBFormat); i++)
        {
            const RGBFormat& f = rgbFormats[i];

            if(ddsd.ddpfPixelFormat.dwRGBBitCount == f.bitCount &&
               ddsd.ddpfPixelFormat.dwRBitMask == f.rBitMask &&
               ddsd.ddpfPixelFormat.dwGBitMask == f.gBitMask &&
               ddsd.ddpfPixelFormat.dwBBitMask == f.bBitMask &&
               ddsd.ddpfPixelFormat.dwRGBAlphaBitMask == f.aBitMask)
            {
                if (f.internalFormat != UNSUPPORTED &&
                    f.pixelFormat    != UNSUPPORTED &&
                    f.dataType       != UNSUPPORTED)
                {
                    Write_Log(LOG_OLD, LOG_INFO, " format = %s", f.name);
                    internalFormat = f.internalFormat;
                    pixelFormat    = f.pixelFormat;
                    dataType       = f.dataType;
                    found = true;
                    break;
                }
                else
                {
                    fioClose(fd);
                    Write_Log(LOG_OLD, LOG_WARN, "ERROR: format is not supported");
                    return nullptr;
                }
            }
        }

        if(!found)
        {
            fioClose(fd);
            Write_Log(LOG_OLD, LOG_WARN, "ERROR: unhandled RGB pixel format in dds file");
            return nullptr;
        }
    }
    else if(ddsd.ddpfPixelFormat.dwFlags & DDPF_LUMINANCE)
    {
        internalFormat = usingAlpha ? GL_LUMINANCE_ALPHA : GL_LUMINANCE;
        pixelFormat = usingAlpha ? GL_LUMINANCE_ALPHA : GL_LUMINANCE;

        if(usingAlpha && ddsd.ddpfPixelFormat.dwLuminanceBitDepth == 8)
        {
            Write_Log(LOG_OLD, LOG_INFO, "format = L4A4");
            pixelFormat = GL_LUMINANCE4_ALPHA4; // invalid enumerant?
        }
        else if ( usingAlpha && ddsd.ddpfPixelFormat.dwLuminanceBitDepth == 32)
        {
            Write_Log(LOG_OLD, LOG_INFO, "format = L16A16");
            dataType = GL_UNSIGNED_SHORT;
        }
        else if ( !usingAlpha && ddsd.ddpfPixelFormat.dwLuminanceBitDepth == 16)
        {
            Write_Log(LOG_OLD, LOG_INFO, "format = L16");
            dataType = GL_UNSIGNED_SHORT;
        }
        else if(usingAlpha)
        {
            Write_Log(LOG_OLD, LOG_WARN, "format = L8A8");
        }
        else
        {
            Write_Log(LOG_OLD, LOG_WARN, "format = L8");
        }
    }
    else if(ddsd.ddpfPixelFormat.dwFlags & DDPF_ALPHA)
    {
        Write_Log(LOG_OLD, LOG_WARN, "format = ALPHA");
        internalFormat = GL_ALPHA;
        pixelFormat    = GL_ALPHA;
    }
    // Compressed formats
    else if(ddsd.ddpfPixelFormat.dwFlags & DDPF_FOURCC)
    {
        // TODO: Image::isImageTranslucent() doesn't work with S3TC compressed files
        switch(ddsd.ddpfPixelFormat.dwFourCC)
        {
        case FOURCC_DXT1:
            Write_Log(LOG_OLD, LOG_INFO, "format = DXT1");
            if (usingAlpha)
            {
                internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
                pixelFormat    = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
            }
            else
            {
                internalFormat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
                pixelFormat    = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
                checkIfUsingOneBitAlpha = true;
            }
            break;
        case FOURCC_DXT3:
            Write_Log(LOG_OLD, LOG_INFO, "format = DXT3");
            internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
            pixelFormat    = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
            break;
        case FOURCC_DXT5:
            Write_Log(LOG_OLD, LOG_INFO, "format = DXT5");
            internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
            pixelFormat    = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
            break;
        case 0x00000024: // A16B16G16R16
            Write_Log(LOG_OLD, LOG_INFO, "format = A16B16G16R16");
            internalFormat = GL_RGBA;
            pixelFormat    = GL_RGBA;
            dataType       = GL_UNSIGNED_SHORT;
            break;
        case 0x00000071: // A16B16G16R16F
            Write_Log(LOG_OLD, LOG_INFO, "format = A16B16G16R16F");
            internalFormat = GL_RGBA; // why no transparency?
            pixelFormat    = GL_RGBA;
            dataType       = GL_HALF_FLOAT_NV;
            break;
        case 0x0000006E: // Q16W16V16U16
            Write_Log(LOG_OLD, LOG_INFO, "format = Q16W16V16U16");
            internalFormat = GL_RGBA;
            pixelFormat    = GL_RGBA;
            dataType       = GL_UNSIGNED_SHORT;
            break;
        case 0x00000070: // G16R16F
            fioClose(fd);
            Write_Log(LOG_OLD, LOG_INFO, "G16R16F format is not supported");
            return nullptr;
            break;
        case 0x00000073: // G32R32F
            Write_Log(LOG_OLD, LOG_INFO, "G32R32F format is not supported");
            fioClose(fd);
            return nullptr;
            break;
        case 0x00000072: // R32F
            Write_Log(LOG_OLD, LOG_INFO, "format = R32F");
            internalFormat = GL_RGB;
            pixelFormat    = GL_RED;
            dataType       = GL_FLOAT;
            break;
        case 0x0000006F: // R16F
            Write_Log(LOG_OLD, LOG_INFO, "format = R16F");
            internalFormat = GL_RGB;
            pixelFormat    = GL_RED;
            dataType       = GL_HALF_FLOAT_NV;
            break;
        case 0x00000074: // A32B32G32R32F
            Write_Log(LOG_OLD, LOG_INFO, "format = A32B32G32R32F");
            internalFormat = GL_RGBA;
            pixelFormat    = GL_RGBA;
            dataType       = GL_FLOAT;
            break;
        case 0x00000075: // CxV8U8
            fioClose(fd);
            Write_Log(LOG_OLD, LOG_INFO, "CxV8U8 format is not supported");
            return nullptr;

        case MAKEFOURCC( 'U', 'Y', 'V', 'Y'): // not supported in OSG
        case MAKEFOURCC( 'U', 'Y', 'V', '2'): // not supported in OSG
        case MAKEFOURCC( 'R', 'G', 'B', 'G'): // R8G8_B8G8 -- what is it?
        case MAKEFOURCC( 'G', 'R', 'G', 'B'): // G8R8_G8B8 -- what is it?
            //break;

        default:
            fioClose(fd);
            Write_Log(LOG_OLD, LOG_WARN, "ERROR: unhandled FOURCC pixel format");
            return nullptr;
        }
    }
    else
    {
        fioClose(fd);
        Write_Log(LOG_OLD, LOG_WARN, "ERROR: unhandled FOURCC pixel format");
        return nullptr;
    }

    // Now set mipmap data (offsets into image raw data)
    //###[afarre_051904]
    Image::MipmapDataType mipmaps;

    // Take care of mipmaps if any.
    if(ddsd.dwMipMapCount>1)
    {
        // Now set mipmap data (offsets into image raw data).
        //###[afarre_051904]osg::Image::MipmapDataType mipmaps;

        //This is to complete mipmap sequence until level Nx1

        //debugging messages
        float power2_s = logf((float)s)/logf((float)2);
        float power2_t = logf((float)t)/logf((float)2);

		mipmaps.resize((unsigned int)std::max(power2_s, power2_t), 0);

        // Handle S3TC compressed mipmaps.
        if( (ddsd.ddpfPixelFormat.dwFlags & DDPF_FOURCC)
            &&
            (ddsd.ddpfPixelFormat.dwFourCC == FOURCC_DXT1 ||
             ddsd.ddpfPixelFormat.dwFourCC == FOURCC_DXT3 ||
             ddsd.ddpfPixelFormat.dwFourCC == FOURCC_DXT5))
        {
            int width = ddsd.dwWidth;
            int height = ddsd.dwHeight;
            int blockSize = (ddsd.ddpfPixelFormat.dwFourCC == FOURCC_DXT1) ? 8 : 16;
            int offset = 0;
            for (unsigned int k = 1; k < ddsd.dwMipMapCount && (width || height); ++k)
            {
                if (width == 0)
                    width = 1;
                if (height == 0)
                    height = 1;
                offset += (((width+3)/4) * ((height+3)/4) * blockSize);
                mipmaps[k-1] = offset;
                width >>= 1;
                height >>= 1;
            }
            //###[afarre_051904] osgImage->setMipmapData(mipmaps);
        }
        // Handle uncompressed mipmaps
        else
        {
            int offset = 0;
            int width = ddsd.dwWidth;
            int height = ddsd.dwHeight;
            for (unsigned int k = 1; k < ddsd.dwMipMapCount && (width || height); ++k)
            {
                if (width == 0)
                    width = 1;
                if (height == 0)
                    height = 1;
                offset += height *
                    Image::computeRowWidthInBytes( width, pixelFormat, dataType, 1);
                mipmaps[k-1] = offset;
                width >>= 1;
                height >>= 1;
            }
            //###[afarre_051904] osgImage->setMipmapData(mipmaps);
        }
    }

    // Set before any modifications (DXT1's with no alpha will become RGBA below)
    image->SetOriginalInternalTextureFormat(internalFormat);
    image->SetOriginalPixelFormat(pixelFormat);

    image->setImage(s,t,r, internalFormat, pixelFormat, dataType, 0, Image::USE_NEW_DELETE);
    if (mipmaps.size()>0)  image->setMipmapLevels(mipmaps);
    unsigned int size = image->getTotalSizeInBytesIncludingMipmaps();

    if(size <= 0)
    {
        Write_Log(LOG_OLD, LOG_WARN, "ERROR: size <= 0");
        fioClose(fd);
        return nullptr;
    }

    unsigned char* imageData = new unsigned char [size];

#ifdef TESTMAC
//  printf("%60s: %'12d [%4dx%4d=%2d]\n",name.c_str(), size, s, t, r);
#endif

    if(!imageData)
    {
        fioClose(fd);
        Write_Log(LOG_OLD, LOG_WARN, "ERROR: imageData == nullptr");
        return nullptr;
    }

    // Read image data
    fioRead(fd, imageData, size);
    fioClose(fd);

    // Check if alpha information embedded in the 8-byte encoding blocks
    if (checkIfUsingOneBitAlpha)
    {
        const DXT1TexelsBlock* texelsBlock = reinterpret_cast<const DXT1TexelsBlock*>(imageData);

        // Only do the check on the first mipmap level
        unsigned int numBlocks = mipmaps.size()>0 ? mipmaps[0] / 8 : size / 8;

        for (int i=numBlocks; i>0; --i, ++texelsBlock)
        {
            if (texelsBlock->color_0<=texelsBlock->color_1)
            {
                // Texture is using the 1-bit alpha encoding, so we need to update the assumed pixel format
                internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
                pixelFormat    = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
                break;
            }
        }
    }

    image->setImage(s,t,r, internalFormat, pixelFormat, dataType, imageData, Image::USE_NEW_DELETE);
    if (mipmaps.size()>0)  image->setMipmapLevels(mipmaps);

    Write_Log(LOG_TEXTURE, LOG_INFO, "[ReadDDSFile] Image %s loaded", name.c_str());

    // Return Image.
    return image.release();
}


/*
osg::Image::MipmapDataType mipmaps;
osgImage->setMipmapData(mipmaps);
osgImage->setImage(s,t,r, internalFormat, pixelFormat, dataType, 0, osg::Image::USE_NEW_DELETE);
printf("INVENTO===> gtsibim:%d  grsib:%d   mi_size:%d   lPitch%d\n",
    osgImage->getTotalSizeInBytesIncludingMipmaps(),
    osgImage->getRowSizeInBytes(), size, ddsd.lPitch);
printf("CORRECTO**> gtsibim:%d  grsib:%d   mi_size:%d   lPitch%d\n",
    osgImage->getTotalSizeInBytesIncludingMipmaps(),
    osgImage->getRowSizeInBytes(), size, ddsd.lPitch);

 */





bool WriteDDSFile(const Image* img, std::ostream& fout)
{

    // Initialize ddsd structure and its members
    DDSURFACEDESC2 ddsd;
    DDPIXELFORMAT  ddpf;
    //DDCOLORKEY     ddckCKDestOverlay;
    //DDCOLORKEY     ddckCKDestBlt;
    //DDCOLORKEY     ddckCKSrcOverlay;
    //DDCOLORKEY     ddckCKSrcBlt;
    DDSCAPS2       ddsCaps;

    ddsd.dwSize = sizeof(ddsd);
    ddpf.dwSize = sizeof(ddpf);

    // Default values and initialization of structures' flags
    unsigned int SD_flags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    unsigned int CAPS_flags  = DDSCAPS_TEXTURE;
    unsigned int PF_flags = 0;
    unsigned int CAPS2_flags = 0;

    // Get image properties
    unsigned int dataType       = img->getDataType();
    unsigned int pixelFormat    = img->getPixelFormat();
    //unsigned int internalFormat = img->getInternalTextureFormat();
    //unsigned int components     = osg::Image::computeNumComponents(pixelFormat);
    unsigned int pixelSize      = Image::computePixelSizeInBits(pixelFormat, dataType);
    unsigned int imageSize      = img->getImageSizeInBytes();
    bool is3dImage = false;

    ddsd.dwWidth  = img->s();
    ddsd.dwHeight = img->t();
    int r = img->r();

    if(r > 1)  /* check for 3d image */
    {
        is3dImage = true;
        ddsd.dwDepth = r;
        SD_flags    |= DDSD_DEPTH;
        CAPS_flags  |= DDSCAPS_COMPLEX;
        CAPS2_flags |= DDSCAPS2_VOLUME;
    }

    // Determine format - set flags and ddsd, ddpf properties
    switch (pixelFormat)
    {
        //Uncompressed
    case GL_RGBA:
        {
            ddpf.dwRBitMask        = 0x000000ff;
            ddpf.dwGBitMask        = 0x0000ff00;
            ddpf.dwBBitMask        = 0x00ff0000;
            ddpf.dwRGBAlphaBitMask = 0xff000000;
            PF_flags |= (DDPF_ALPHAPIXELS | DDPF_RGB);
            ddpf.dwRGBBitCount = pixelSize;
            ddsd.lPitch = img->getRowSizeInBytes();
            SD_flags |= DDSD_PITCH;
        }
        break;
    case GL_BGRA:
        {
            ddpf.dwBBitMask        = 0x000000ff;
            ddpf.dwGBitMask        = 0x0000ff00;
            ddpf.dwRBitMask        = 0x00ff0000;
            ddpf.dwRGBAlphaBitMask = 0xff000000;
            PF_flags |= (DDPF_ALPHAPIXELS | DDPF_RGB);
            ddpf.dwRGBBitCount = pixelSize;
            ddsd.lPitch = img->getRowSizeInBytes();
            SD_flags |= DDSD_PITCH;
        }
        break;
    case GL_LUMINANCE_ALPHA:
        {
            ddpf.dwRBitMask         = 0x00ff0000;
            ddpf.dwRGBAlphaBitMask  = 0x000000ff;
            PF_flags |= (DDPF_ALPHAPIXELS | DDPF_LUMINANCE);
            ddpf.dwRGBBitCount = pixelSize;
            ddsd.lPitch = img->getRowSizeInBytes();
            SD_flags |= DDSD_PITCH;
        }
        break;
    case GL_RGB:
        {
            ddpf.dwRBitMask        = 0x000000ff;
            ddpf.dwGBitMask        = 0x0000ff00;
            ddpf.dwBBitMask        = 0x00ff0000;
            PF_flags |= DDPF_RGB;
            ddpf.dwRGBBitCount = pixelSize;
            ddsd.lPitch = img->getRowSizeInBytes();
            SD_flags |= DDSD_PITCH;
        }
        break;
    case GL_LUMINANCE:
        {
            ddpf.dwRBitMask         = 0x00ff0000;
            PF_flags |= DDPF_LUMINANCE;
            ddpf.dwRGBBitCount = pixelSize;
            ddsd.lPitch = img->getRowSizeInBytes();
            SD_flags |= DDSD_PITCH;
        }
        break;
    case GL_ALPHA:
        {
            ddpf.dwRGBAlphaBitMask  = 0x000000ff;
            PF_flags |= DDPF_ALPHA;
            ddpf.dwRGBBitCount = pixelSize;
            ddsd.lPitch = img->getRowSizeInBytes();
            SD_flags |= DDSD_PITCH;
        }
        break;

        //Compressed
    case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        {
            ddpf.dwFourCC = FOURCC_DXT1;
            PF_flags |= (DDPF_ALPHAPIXELS | DDPF_FOURCC);
            ddsd.dwLinearSize = imageSize;
            SD_flags |= DDSD_LINEARSIZE;
        }
        break;
    case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
        {
            ddpf.dwFourCC = FOURCC_DXT3;
            PF_flags |= (DDPF_ALPHAPIXELS | DDPF_FOURCC);
            ddsd.dwLinearSize = imageSize;
            SD_flags |= DDSD_LINEARSIZE;
        }
        break;
    case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
        {
            ddpf.dwFourCC = FOURCC_DXT5;
            PF_flags |= (DDPF_ALPHAPIXELS | DDPF_FOURCC);
            ddsd.dwLinearSize = imageSize;
            SD_flags |= DDSD_LINEARSIZE;
        }
        break;
    case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
        {
            ddpf.dwFourCC = FOURCC_DXT1;
            PF_flags |= DDPF_FOURCC;  /* No alpha here */
            ddsd.dwLinearSize = imageSize;
            SD_flags |= DDSD_LINEARSIZE;
        }
        break;
    default:
        Write_Debug("Warning:: unhandled pixel format in image, file cannot be written.");
        return false;
    }


    // set even more flags
    if(img->isMipmap() && !is3dImage)
    {
        SD_flags   |= DDSD_MIPMAPCOUNT;
        CAPS_flags |= DDSCAPS_COMPLEX | DDSCAPS_MIPMAP;
        ddsd.dwMipMapCount = img->getNumMipmapLevels();
    }


    // Assign flags and structure members of ddsd
    ddsd.dwFlags    = SD_flags;
    ddpf.dwFlags    = PF_flags;
    ddsCaps.dwCaps  = CAPS_flags;
    ddsCaps.dwCaps2 = CAPS2_flags;

    ddsd.ddpfPixelFormat = ddpf;
    ddsd.ddsCaps = ddsCaps;


    // Write DDS file
    fout.write("DDS ", 4); /* write FOURCC */
    fout.write(reinterpret_cast<char*>(&ddsd), sizeof(ddsd)); /* write file header */

    //    int isize = img->getTotalSizeInBytesIncludingMipmaps();
    if(!is3dImage)
    {
        fout.write(reinterpret_cast<const char*>(img->data()), img->getTotalSizeInBytesIncludingMipmaps());
    }
    else  /* 3d image */
    {
        for(int i = 0; i < r; ++i)
        {
            fout.write(reinterpret_cast<const char*>(img->data(0, 0, i)), imageSize);
        }
    }

    // Check for correct saving
    if (fout.fail())
    {
        return false;
    }

    // If we get that far the file was saved properly //
    return true;
}
#if(0)


class ReaderWriterDDS : public osgDB::ReaderWriter
{
public:

    ReaderWriterDDS()
    {
        supportsExtension("dds","DDS image format");
    }

    virtual const char* className() const
    {
        return "DDS Image Reader/Writer";
    }

    virtual ReadResult readObject(const std::string& file, const osgDB::ReaderWriter::Options* options) const
    {
        return readImage(file,options);
    }

    virtual ReadResult readObject(std::istream& fin, const Options* options) const
    {
        return readImage(fin,options);
    }

    virtual ReadResult readImage(const std::string& file, const osgDB::ReaderWriter::Options* options) const
    {
        std::string ext = osgDB::getLowerCaseFileExtension(file);
        if (!acceptsExtension(ext)) return ReadResult::FILE_NOT_HANDLED;

  		if (options && options->getOptionString().find("fio_archive")!=std::string::npos)
		{
 			pa_istream stream(file.c_str(), std::ios::in | std::ios::binary);
			if(!stream) return ReadResult::FILE_NOT_HANDLED;
			ReadResult rr = readImage(stream, options);
			if(rr.validImage()) rr.getImage()->setFileName(file);

			return rr;
		}

        std::string fileName = osgDB::findDataFile( file, options);

        if (fileName.empty()) return ReadResult::FILE_NOT_FOUND;

        std::ifstream stream(fileName.c_str(), std::ios::in | std::ios::binary);
        if(!stream) return ReadResult::FILE_NOT_HANDLED;
        ReadResult rr = readImage(stream, options);
        if(rr.validImage()) rr.getImage()->setFileName(file);
        return rr;
    }

    virtual ReadResult readImage(std::istream& fin, const Options* options) const
    {
        osg::Image* osgImage = ReadDDSFile(fin);
        if (osgImage==nullptr) return ReadResult::FILE_NOT_HANDLED;

        if (options && options->getOptionString().find("dds_flip")!=std::string::npos)
        {
            osgImage->flipVertical();
        }

        return osgImage;
    }

    virtual WriteResult writeObject(const osg::Object& object,const std::string& file, const osgDB::ReaderWriter::Options* options) const
    {
        const osg::Image* image = dynamic_cast<const osg::Image*>(&object);
        if (!image) return WriteResult::FILE_NOT_HANDLED;

        return writeImage(*image,file,options);
    }

    virtual WriteResult writeObject(const osg::Object& object,std::ostream& fout,const Options* options) const
    {
        const osg::Image* image = dynamic_cast<const osg::Image*>(&object);
        if (!image) return WriteResult::FILE_NOT_HANDLED;

        return writeImage(*image,fout,options);
    }


    virtual WriteResult writeImage(const osg::Image &image,const std::string& file, const osgDB::ReaderWriter::Options* options) const
    {
        std::string ext = osgDB::getFileExtension(file);
        if (!acceptsExtension(ext)) return WriteResult::FILE_NOT_HANDLED;

        std::ofstream fout(file.c_str(), std::ios::out | std::ios::binary);
        if(!fout) return WriteResult::ERROR_IN_WRITING_FILE;

        return writeImage(image,fout,options);
    }

    virtual WriteResult writeImage(const osg::Image& image,std::ostream& fout,const Options*) const
    {
        bool success = WriteDDSFile(&image, fout);

        if(success)
            return WriteResult::FILE_SAVED;
        else
            return WriteResult::ERROR_IN_WRITING_FILE;
    }
};

// now register with Registry to instantiate the above
// reader/writer.
REGISTER_OSGPLUGIN(dds, ReaderWriterDDS)
#endif
