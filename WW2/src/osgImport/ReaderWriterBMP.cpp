#include <syscfg.h> // Path is project specific.
// -*-c++-*-

/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "osgImport/Texture.h"
#include "osgImport/ref_ptr.h"
#include "osgImport/Image.h"

#include "fio/fio.h"

/****************************************************************************
 *
 * Follows is code written by GWM and translated to fit with the OSG Ethos.
 *
 * Ported into the OSG as a plugin, Geoff Michel October 2001.
 * For patches, bugs and new features
 * please send them direct to the OSG dev team.
 *
 **********************************************************************/

// find least-significant (lowest) bit position in 16-bit mask
static unsigned int findLeastSignificantBit(unsigned short mask)
{
    unsigned int shift = 1;
    while ((mask & 0x01) == 0)
    {
        mask >>= 1;
        ++shift;
    }
    return shift;
}

// find most-significant (highest) bit position in 16-bit mask
static unsigned int findMostSignificantBit(unsigned short mask)
{
    unsigned int shift = 16;
    while ((mask & 0x8000) == 0)
    {
        mask <<= 1;
        --shift;
    }
    return shift;
}


/*
 * BMP header
 */
const unsigned short BMP_MAGIC_BM = 0x424D; // 'BM'
const unsigned short BMP_MAGIC_MB = 0x4D42; // 'MB'

struct BMPHeader
{
    unsigned short magic; // stored 'BM', but read as ushort
    unsigned int fileSize;
    unsigned short reserved1;
    unsigned short reserved2;
    unsigned int imageOffset;
};

/*
 * Windows v3 header
 */

enum BMPCOMPRESSION
{
    _BI_RGB,
    _BI_RLE8,
    _BI_RLE4,
    _BI_BITFIELDS,
    _BI_JPEG,
    _BI_PNG
};

struct _BITMAPINFOHEADER
{
    //unsigned int hdrSize;
    int width, height;
    unsigned short colorPlanes;
    unsigned short bitsPerPixel;
    unsigned int compression;
    unsigned int imageSize;
    int horizontalPixelPerMeter;
    int verticalPixelPerMeter;
    unsigned int numColorsInPalette;
    unsigned int numImportantColors;
};
/*
 * OS/2 v1 header
 */

struct _BITMAPCOREHEADER
{
    //unsigned int hdrSize;
    unsigned short width, height;
    unsigned short colorPlanes;
    unsigned short bitsPerPixel;
};

inline void swapBytes2(char* in)
{
    std::swap(in[0],in[1]);
}

inline void swapBytes4(char* in)
{
    std::swap(in[0], in[3]);
    std::swap(in[1], in[2]);
}

static unsigned char* bmp_load(int fd,
                               int& width_ret,
                               int& height_ret,
                               int& numComponents_ret,
                               unsigned int transparentColor)
{
    // actual file size
    size_t actFileSize = fioSeek(fd, 0, SEEK_END);
    fioSeek(fd, 0, SEEK_SET);

    bool swap;

    BMPHeader bmp;
    {
        /*
         * read each part individually to avoid struct packing issues (and #pragma)
         */
        fioRead(fd, (char*)&bmp.magic, sizeof(bmp.magic));
        fioRead(fd, (char*)&bmp.fileSize, sizeof(bmp.fileSize));
        fioRead(fd, (char*)&bmp.reserved1, sizeof(bmp.reserved1));
        fioRead(fd, (char*)&bmp.reserved2, sizeof(bmp.reserved2));
        fioRead(fd, (char*)&bmp.imageOffset, sizeof(bmp.imageOffset));

        if (bmp.magic != BMP_MAGIC_BM && bmp.magic != BMP_MAGIC_MB)
        {
            WriteDebug("Invalid BMP magic");
            return 0;
        }

        swap = (bmp.magic == BMP_MAGIC_BM); // means machine is big-endian and must swap
        if (swap)
        {
            swapBytes4((char*) &bmp.fileSize);
            swapBytes4((char*) &bmp.imageOffset);
        }

        if (bmp.fileSize != actFileSize)
        {
            bmp.fileSize = actFileSize;
        }
    }

    _BITMAPINFOHEADER dib;
    unsigned int dibHdrSize;

    /*
     * read DIB header
     */
    fioRead(fd, (char*)&dibHdrSize, sizeof(dibHdrSize));
    if (swap)
        swapBytes4((char*) &dibHdrSize);

    if (dibHdrSize == 12)
    {
        /*
         * OS/2 v1
         */
        _BITMAPCOREHEADER hdr;

        unsigned int expectHdrSize = sizeof(hdr) + sizeof(dibHdrSize);
        if (expectHdrSize != dibHdrSize)
        {
            WriteDebug("Invalid BMP OS/2 v1 header size %d != %d", expectHdrSize, dibHdrSize);
            return 0;
        }

        fioRead(fd, (char*)&hdr, sizeof(hdr));
        if (swap)
        {
            swapBytes2((char*) &hdr.width);
            swapBytes2((char*) &hdr.height);
            swapBytes2((char*) &hdr.colorPlanes);
            swapBytes2((char*) &hdr.bitsPerPixel);
        }

        // store to BITMAPINFOHEADER
        memset(&dib, 0, sizeof(dib));
        dib.width = hdr.width;
        dib.height = hdr.height;
        dib.colorPlanes = hdr.colorPlanes;
        dib.bitsPerPixel = hdr.bitsPerPixel;
    }
    else if (dibHdrSize == 40 || dibHdrSize == 108 || dibHdrSize == 124)
    {
        /*
         * Windows v3/v4/v5 header
         * reads only the common part (i.e. v3) and skips over the rest
         */
        fioRead(fd, (char*)&dib, sizeof(dib));
        if (swap)
        {
            swapBytes4((char*) &dib.width);
            swapBytes4((char*) &dib.height);
            swapBytes2((char*) &dib.colorPlanes);
            swapBytes2((char*) &dib.bitsPerPixel);
            swapBytes4((char*) &dib.compression);
            swapBytes4((char*) &dib.imageSize);
            swapBytes4((char*) &dib.numColorsInPalette);
            swapBytes4((char*) &dib.numImportantColors);
        }
    }
    else
    {
        WriteDebug("Unsupported BMP/DIB header size = %d", dibHdrSize);
        return 0;
    }

    // sanity checks
    if (dib.height < 0)
    {
        WriteDebug("BMP Image is upside-down");
        dib.height *= -1;
    }
    if (dib.colorPlanes != 1)
    {
        WriteDebug("Invalid BMP number of color planes = %d", dib.colorPlanes);
        return 0;
    }
    if (dib.bitsPerPixel == 0)
    {
        WriteDebug("Invalid BMP bits/pixel = %d", dib.bitsPerPixel);
        return 0;
    }
    if (dib.compression != _BI_RGB && dib.compression != _BI_BITFIELDS)
    {
        WriteDebug("Unsupported BMP compression = %d", dib.compression);
        return 0;
    }

    /*
     * color masks
     */
    unsigned int redMask, greenMask, blueMask;
    if (dib.bitsPerPixel == 16 && dib.compression == _BI_BITFIELDS)
    {
        fioRead(fd, (char*)&redMask, sizeof(redMask));
        fioRead(fd, (char*)&greenMask, sizeof(greenMask));
        fioRead(fd, (char*)&blueMask, sizeof(blueMask));

        if(swap)
        {
            swapBytes4((char*)&redMask);
            swapBytes4((char*)&greenMask);
            swapBytes4((char*)&blueMask);
        }
    }
    else
    {
        redMask = 0x7c00;
        greenMask = 0x03e0;
        blueMask = 0x001f;
    }

    // determine shift width...
    unsigned int redShift = findLeastSignificantBit(redMask) - 1;
    unsigned int greenShift = findLeastSignificantBit(greenMask) - 1;
    unsigned int blueShift = findLeastSignificantBit(blueMask) - 1;

    // determine mask width
    unsigned int redMaskWidth = findMostSignificantBit(redMask) - redShift;
    unsigned int greenMaskWidth = findMostSignificantBit(greenMask) - greenShift;
    unsigned int blueMaskWidth = findMostSignificantBit(blueMask) - blueShift;

    unsigned int imageBytesPerPixel = 0;

    /*
     * color palette
     */
    std::vector<unsigned char> colorPalette;
    if (dib.bitsPerPixel < 16)
    {
        // defaults to 2^n
        if (dib.numColorsInPalette == 0)
            dib.numColorsInPalette = 1 << dib.bitsPerPixel;

        // allocate/read color palette
        imageBytesPerPixel = (dibHdrSize == 12 ? 3 : 4); // OS/2 v1 stores RGB, else RGBA
        colorPalette.resize(dib.numColorsInPalette * imageBytesPerPixel);
        fioRead(fd, (char*)&*colorPalette.begin(), colorPalette.size());
    }
    else
    {
        if (dib.bitsPerPixel == 16)
            imageBytesPerPixel = 3;
        else if (dib.bitsPerPixel == 24 || dib.bitsPerPixel == 32)
            imageBytesPerPixel = dib.bitsPerPixel / 8;
        else
        {
            WriteDebug("Unsupported BMP bit depth %d", dib.bitsPerPixel);
            return 0;
        }
    }

    size_t Where = fioSeek(fd, 0, SEEK_CUR);
    if (Where != bmp.imageOffset)
    {
        // this can happen because we don't fully parse v4/v5 headers
        WriteDebug("BMP streampos out-of-sync where = %d imageOffset = %d", Where, bmp.imageOffset);
        fioSeek(fd, bmp.imageOffset, SEEK_SET);
    }

    /*
     * image data
     */
    const unsigned int imageBytesPerRow = dib.width * imageBytesPerPixel;
    const unsigned int imageBufferSize = imageBytesPerRow * dib.height;
    unsigned char* imageBuffer = new unsigned char[imageBufferSize];
    //printf("imageBytesPerPixel=%u imageBytesPerRow=%u\n", imageBytesPerPixel, imageBytesPerRow);

    // byte/row in BMP image data
    unsigned int bytesPerPixel;
    unsigned int bytesPerRow;
    if (dib.bitsPerPixel >= 8)
    {
        bytesPerPixel = dib.bitsPerPixel / 8;
        bytesPerRow = dib.width * bytesPerPixel;
    }
    else
    {
        bytesPerPixel = 1;
        bytesPerRow = (unsigned int) (dib.width * (dib.bitsPerPixel / 8.0f));
    }
    const unsigned int bytesPerRowAlign = (unsigned int) ceilf(bytesPerRow / 4.0f) * 4;
    //printf("bytesPerPixel=%u bytesPerRow=%u bytesPerRowAlign=%u\n", bytesPerPixel, bytesPerRow, bytesPerRowAlign);

    std::vector<unsigned char> rowBuffer;
    rowBuffer.resize(bytesPerRowAlign);

    if (dib.bitsPerPixel >= 16)
    {
        unsigned char* imgp = imageBuffer;
        for (int i = 0; i < dib.height; ++i)
        {
            // read row
            unsigned char* rowp = &*rowBuffer.begin();
            fioRead(fd, (char*)rowp, rowBuffer.size());

            // copy to image buffer, swap/unpack BGR to RGB(A)
            for (unsigned int j = 0; j < bytesPerRow; j += bytesPerPixel)
            {
                if (dib.bitsPerPixel == 16)
                {
                    // 16-bit RGB -> 24-bit RGB
                    unsigned short rgb16 = (rowp[1] << 8) | rowp[0];
                    if (swap)
                        swapBytes2((char*) &rgb16);

                    imgp[0] = (rgb16 & redMask) >> redShift;
                    imgp[1] = (rgb16 & greenMask) >> greenShift;
                    imgp[2] = (rgb16 & blueMask) >> blueShift;

                    // expand range
                    imgp[0] <<= (8-redMaskWidth);
                    imgp[1] <<= (8-greenMaskWidth);
                    imgp[2] <<= (8-blueMaskWidth);
                }
                else
                {
                    // BGR -> RGB(A)
                    imgp[0] = rowp[2];
                    imgp[1] = rowp[1];
                    imgp[2] = rowp[0];
                    if (imageBytesPerPixel == 4)
                    {
                        imgp[3] = 0xff;
                    }
                }
                imgp += imageBytesPerPixel;
                rowp += bytesPerPixel;
            }
        }
    }
    else
    {
        const int idxPerByte = 8 / dib.bitsPerPixel; // color indices per byte
        const int idxMask = (1 << dib.bitsPerPixel) - 1; // index mask
        //printf("idxPerByte=%d idxMask=%02x\n", idxPerByte, idxMask);

        unsigned char* imgp = imageBuffer;
        for (int i = 0; i < dib.height; ++i)
        {
            // read row
            unsigned char* rowp = &*rowBuffer.begin();
            fioRead(fd, (char*)rowp, rowBuffer.size());

            int j = 0;
            while (j < dib.width)
            {
                // unpack bytes/indices to image buffer
                unsigned char val = rowp[0];
                for (int k = 0; k < idxPerByte && j < dib.width; ++k, ++j)
                {
                    unsigned int idx = (val >> ((idxPerByte-1-k) * dib.bitsPerPixel)) & idxMask;
                    idx *= imageBytesPerPixel;

                    unsigned char r = colorPalette[idx+2];
                    unsigned char g = colorPalette[idx+1];
                    unsigned char b = colorPalette[idx+0];
                    unsigned char a = 0;

                    if(imageBytesPerPixel == 4)
                    {
                        a = 0xff;
                    }

                    unsigned int color = piRGBA_MAKE(r, g, b, a);

                    if(color == transparentColor)
                    {
                        r = 0;
                        g = 0;
                        b = 0;
                        a = 0;
                    }

                    imgp[0] = r;
                    imgp[1] = g;
                    imgp[2] = b;

                    if(imageBytesPerPixel == 4)
                    {
                        imgp[3] = a;
                    }

                    imgp += imageBytesPerPixel;
                }
                ++rowp;
            }
        }
    }

    // return result
    width_ret = dib.width;
    height_ret = dib.height;
    numComponents_ret = imageBytesPerPixel;

    return imageBuffer;
}

Image* ReadBMPFile(const std::string& name, unsigned int transparentColor)
{
    int s, t;
    int internalFormat;

    int fd = -1;
    fd = fioOpen(name.c_str());

    if(fd < 0)
    {
        if (errno != ENOENT) {
            WriteDebug("[ReadBMPFile] cannot open %s", name.c_str());
        }
        return nullptr;
    }

    WriteLog(LOG_TEXTURE, LOG_INFO, "[ReadBMPFile] Reading image %s", name.c_str());

    unsigned char* imageData = bmp_load(fd, s, t, internalFormat, transparentColor);

    fioClose(fd);

    if(imageData == 0)
        return nullptr;

    unsigned int pixelFormat;

    switch(internalFormat)
    {
    case 1:
        pixelFormat = GL_LUMINANCE;
        break;
    case 2:
        pixelFormat = GL_LUMINANCE_ALPHA;
        break;
    case 3:
        pixelFormat = GL_RGB;
        break;
    default:
        pixelFormat = GL_RGBA;
        break;
    }

    ref_ptr<Image> img = new Image;

    img->setImage(s,
                  t,
                  1,
                  internalFormat,
                  pixelFormat,
                  GL_UNSIGNED_BYTE,
                  imageData,
                  Image::USE_NEW_DELETE);

    // Add alpha if necessary
    std::string alphapath = name.substr(0, name.length() - 4); //-4 = ".ext"
	alphapath += "_alpha.bmp";

    fd = fioOpen(alphapath.c_str());
    if(fd < 0) {
        return img.release();
    }

    unsigned char* alphaData = bmp_load(fd, s, t, internalFormat, 0);

    if(!alphaData)
        return img.release();

    switch(internalFormat)
    {
    case 1:
        pixelFormat = GL_LUMINANCE;
        break;
    case 2:
        pixelFormat = GL_LUMINANCE_ALPHA;
        break;
    case 3:
        pixelFormat = GL_RGB;
        break;
    default:
        pixelFormat = GL_RGBA;
        break;
    }

    ref_ptr<Image> aimg = new Image;

    aimg->setImage(s,
                   t,
                   1,
                   internalFormat,
                   pixelFormat,
                   GL_UNSIGNED_BYTE,
                   alphaData,
                   Image::USE_NEW_DELETE);

    if(aimg->s() != img->s() || aimg->t() != img->t())
        return img.release();

    ref_ptr<Image> outimage = new Image;

    outimage->setFileName(img->getFileName());
    outimage->allocateImage(img->s(), img->t(), 1, GL_RGBA, GL_UNSIGNED_BYTE);

    // Add alpha
    for(int row = 0; row < img->t(); row++)
    {
        unsigned char* in = img->data(0,row);
        unsigned char* alpha = aimg->data(0,row);
        unsigned char* out = outimage->data(0,row);

        for(int col = 0; col < img->s(); col++)
        {
	        *out++ = *in++;
	        *out++ = *in++;
	        *out++ = *in++;
	        *out++ = *alpha;
            alpha += pixelFormat == GL_RGB ? 3 : 4;
        }
    }

    return outimage.release();
}

static bool bmp_save(const Image& img, int fd)
{
    BMPHeader bmp;
    const unsigned int bmpHdrSize = 14;

    struct _BITMAPINFOHEADER dib;
    assert(sizeof(dib) == 36);
    const unsigned int dibHdrSize = sizeof(dib) + 4;

    const unsigned int bytesPerRowAlign = ((img.s() * 3 + 3) / 4) * 4;

    const bool swap = false;

    // BMP header
    {
        bmp.magic = BMP_MAGIC_BM;
        bmp.reserved1 = bmp.reserved2 = 0;
        bmp.imageOffset = bmpHdrSize + dibHdrSize;
        bmp.fileSize = bmp.imageOffset + bytesPerRowAlign * img.t();

        if(swap)
        {
            // big-endian must swap everything except magic
            swapBytes4((char*) &bmp.fileSize);
            swapBytes4((char*) &bmp.imageOffset);
        }
        else
        {
            // little-endian must swap the magic
            swapBytes2((char*) &bmp.magic);
        }

        fioWrite(fd, &bmp.magic, sizeof(bmp.magic));
        fioWrite(fd, &bmp.fileSize, sizeof(bmp.fileSize));
        fioWrite(fd, &bmp.reserved1, sizeof(bmp.reserved1));
        fioWrite(fd, &bmp.reserved2, sizeof(bmp.reserved2));
        fioWrite(fd, &bmp.imageOffset, sizeof(bmp.imageOffset));
    }

    // DIB header
    {
        dib.width = img.s();
        dib.height = img.t();
        dib.colorPlanes = 1;
        dib.bitsPerPixel = 24;
        dib.compression = BI_RGB;
        dib.imageSize = bytesPerRowAlign * img.t();
        dib.horizontalPixelPerMeter = 1000;
        dib.verticalPixelPerMeter = 1000;
        dib.numColorsInPalette = 0;
        dib.numImportantColors = 0;

        if(swap)
        {
            swapBytes4((char*) &dibHdrSize);
            swapBytes4((char*) &dib.width);
            swapBytes4((char*) &dib.height);
            swapBytes2((char*) &dib.colorPlanes);
            swapBytes2((char*) &dib.bitsPerPixel);
            swapBytes4((char*) &dib.imageSize);
            swapBytes4((char*) &dib.horizontalPixelPerMeter);
            swapBytes4((char*) &dib.verticalPixelPerMeter);
        }

        fioWrite(fd, &dibHdrSize, sizeof(dibHdrSize));
        fioWrite(fd, &dib, sizeof(dib));
    }

    const unsigned int channelsPerPixel = img.computeNumComponents(img.getPixelFormat());

    std::vector<unsigned char> rowBuffer(bytesPerRowAlign);

    for(int y = 0; y < img.t(); ++y)
    {
        const unsigned char* imgp = img.data() + img.s() * y * channelsPerPixel;

        for(int x = 0; x < img.s(); ++x)
        {
            // RGB -> BGR
            unsigned int rowOffs = x * 3, imgOffs = x * channelsPerPixel;
            rowBuffer[rowOffs + 2] = imgp[imgOffs + 0];
            rowBuffer[rowOffs + 1] = imgp[imgOffs + 1];
            rowBuffer[rowOffs + 0] = imgp[imgOffs + 2];
        }

        fioWrite(fd, &*rowBuffer.begin(), rowBuffer.size());
    }

    return true;
}

bool writeBMPImage(const Image& image)
{
    int old = fioQuery();
    fioSet(ISUSERDIRECTORY);

    int fd = fioOpenWrite(image.getFileName().c_str());

    bmp_save(image, fd);

    fioClose(fd);
    fioSet(old);

    return true;
}

#if(0)
class ReaderWriterBMP : public osgDB::ReaderWriter
{
    public:

        ReaderWriterBMP()
        {
            supportsExtension("bmp","BMP Image format");
        }

        const char* className() const { return "BMP Image Reader"; }


        ReadResult readObject(std::istream& fin, const Options* options = 0) const
        {
            return readImage(fin, options);
        }

        ReadResult readObject(const std::string& file, const Options* options = 0) const
        {
            return readImage(file, options);
        }


        ReadResult readImage(std::istream& fin, const Options* = 0) const
        {
            return readBMPStream(fin);
        }

        ReadResult readImage(const std::string& file, const Options* options = 0) const
        {
            std::string ext = osgDB::getLowerCaseFileExtension(file);
            if (!acceptsExtension(ext)) return ReadResult::FILE_NOT_HANDLED;

            std::string fileName = osgDB::findDataFile(file, options);
            if (fileName.empty()) return ReadResult::FILE_NOT_FOUND;

            osgDB::ifstream istream(fileName.c_str(), std::ios::in | std::ios::binary);
            if(!istream) return ReadResult::FILE_NOT_HANDLED;

            ReadResult rr = readBMPStream(istream);
            if(rr.validImage()) rr.getImage()->setFileName(file);

            return rr;
        }


        WriteResult writeImage(const osg::Image& image, std::ostream& fout, const Options* = 0) const
        {
            if (bmp_save(image, fout))
                return WriteResult::FILE_SAVED;
            else
                return WriteResult::ERROR_IN_WRITING_FILE;
        }

        WriteResult writeImage(const osg::Image& img, const std::string& fileName, const Options* options = 0) const
        {
            std::string ext = osgDB::getFileExtension(fileName);
            if (!acceptsExtension(ext)) return WriteResult::FILE_NOT_HANDLED;

            osgDB::ofstream fout(fileName.c_str(), std::ios::out | std::ios::binary);
            if (!fout) return WriteResult::ERROR_IN_WRITING_FILE;

            return writeImage(img, fout, options);
        }

    private:
        static ReadResult readBMPStream(std::istream& fin)
        {
            int s, t;
            int internalFormat;

            unsigned char* imageData = bmp_load(fin, s, t, internalFormat);
            if (imageData == 0) return ReadResult::ERROR_IN_READING_FILE;

            unsigned int pixelFormat;
            switch (internalFormat)
            {
            case 1:
                pixelFormat = GL_LUMINANCE;
                break;
            case 2:
                pixelFormat = GL_LUMINANCE_ALPHA;
                break;
            case 3:
                pixelFormat = GL_RGB;
                break;
            default:
                pixelFormat = GL_RGBA;
                break;
            }

            osg::Image* img = new osg::Image;
            img->setImage(s, t, 1,
                internalFormat, pixelFormat, GL_UNSIGNED_BYTE, imageData,
                osg::Image::USE_NEW_DELETE);

            return img;
        }
};

// now register with Registry to instantiate the above
// reader/writer.
REGISTER_OSGPLUGIN(bmp, ReaderWriterBMP)
#endif
