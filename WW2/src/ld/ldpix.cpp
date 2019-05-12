#include "syscfg.h" // Path is project specific.
//

#include <math.h>
#include <string.h>
#include <math.h>
#include "pi/pi.h"
#include "include/vars.h"
#include "mem/mem.h"
#include "fio/fio.h"
#include "ld/lddds.h"
#include "ld/ldpix.h"
#include "ld/ldbmp.h"
#include "ld/ldpcx.h"
#include "ld/ldmdl.h"
#include "rn/rnlogo.h"
#include "str/stringLib.h"
#include "ld/ldtexture.h"
#include <algorithm>

extern SINT32	LoadingScreen;

using LoadFileToSurface = int (*)(const char* Filename, UINT32 TransparentColor, UINT32 WantFlags);

static int _loadImageFileToSurface(const char* name, UINT32 transcolor, UINT32 flags);

struct GFX_FORMATS
{
	char  Extension[5];
	LoadFileToSurface Routine;
} GfxFormats[] = {
    {"dds", ldLoadTexture},
    //{"3df",ld3dfLoad3DFFileToSurface},
    {"bmp",ldLoadTexture},//ldbmpLoadBMPFileToSurface},
    //{"iff",ldlbmLoadLBMFileToSurface},
    //{"jpg",ldjpgLoadJPGFileToSurface},
    //{"lbm",ldlbmLoadLBMFileToSurface},
    //{"pcx",ldpcxLoadPCXFileToSurface},
    //{"ppm",ldppmLoadPPMFileToSurface},
    {"png", ldLoadTexture},
    //{"rgb",ldrgbLoadRGBFileToSurface},
    //{"tga",ldtgaLoadTGAFileToSurface},
    //{"tif",ldtifLoadTIFFFileToSurface},
    //{"tiff",ldtifLoadTIFFFileToSurface},
    {"",nullptr}
};

int ldpixLoadImageFileToSurface(const char* Filename, UINT32 TransparentColor, UINT32 Flags)
{
    SINT32 texid = _loadImageFileToSurface(Filename, TransparentColor, Flags);

    if(!(Flags & LDPIX_NO_TEXTURE_GENERATE))
    {
        piGetTextureHandle(texid, 0);
    }

	return texid;
}

int _loadImageFileToSurface(const char* Filename, UINT32 TransparentColor, UINT32 Flags)
{
	char		basename[256], localname[256], tmpname[256];

	bool WantMipmap = (Flags & LDPIX_MIPMAP) != 0;
	if (WantMipmap
		&& (strncasecmp("ui\\", Filename, 3) == 0
			|| strncasecmp("\\fonts\\", Filename, 7) == 0
			|| strncasecmp("splashscreen", Filename, 12) == 0))
	{
		WantMipmap = false;
	}

	strsafecpy(basename, Filename);
	char* ptr = (char*)strrchr(basename, '.');
	if (ptr)
	{
		*ptr = 0;
	}
	_strlwr(basename);

	int texid = piGetNamedTexture(basename);
	if (piIsTexture(texid, 0))
	{
		piAddTexRef(texid);
		return(texid);
	}

	strsafecpy(localname, basename);
#ifdef GR2_TEXTURE_DEBUG
    WriteLog(LOG_TEXTURE, LOG_INFO, "[TEXTURE] loading %s", localname);
#endif
	size_t suffixOffset = strprintf(tmpname, "%s.", localname);
	for (int i=0; GfxFormats[i].Routine; i++)
	{
		strncpy(tmpname + suffixOffset, GfxFormats[i].Extension, sizeof(tmpname) - suffixOffset);
		// if file exists, load it
		if (fioFileExists(tmpname)) {
			if(GfxFormats[i].Routine == ldLoadTexture) {
				texid = ldLoadTexture(tmpname, TransparentColor, Flags);
				if (piIsTexture(texid, 0)) {
					return texid;
				}
			} else {
				assert(false);
			}
		}
    }
    return -1;
}

LOGICAL ldpixCopyBitsToSurface(UCHAR* bits, int bitsPitch, int width, int height, piSURFACEDESC* lppisd, UINT32 TransparentColor)
{
piRGBTRIPLE*  rgbptr;
char*    cptr;
short*   sptr;
long*    lptr;
int     i, j;//, Count=0;
int     RedBits, RedShift, RedMask;
int     GreenBits, GreenShift, GreenMask;
int     BlueBits, BlueShift, BlueMask;
int     AlphaBits, AlphaShift, AlphaMask;
int     SearchColor, Color;

   if(bitsPitch == 0)
	    bitsPitch = width*3;

	rgbptr = (piRGBTRIPLE*)bits;

	RedMask     = lppisd->PixelFormat.dwRBitMask;
	GreenMask   = lppisd->PixelFormat.dwGBitMask;
	BlueMask    = lppisd->PixelFormat.dwBBitMask;
	AlphaMask   = lppisd->PixelFormat.dwRGBAlphaBitMask;

	RedBits     = 8 - piFindBitCount(RedMask);
	RedShift    = piFindBitShift(RedMask);
	GreenBits   = 8 - piFindBitCount(GreenMask);
	GreenShift  = piFindBitShift(GreenMask);
	BlueBits    = 8 - piFindBitCount(BlueMask);
	BlueShift   = piFindBitShift(BlueMask);
	AlphaBits   = piFindBitCount(AlphaMask);
	AlphaShift  = piFindBitShift(AlphaMask);

#if defined(OPENGL3D) && platOS == OS_WIN32
	if(lppisd->PixelFormat.dwRGBBitCount == 32)
	{
		UINT32  uTemp = BlueShift;
		BlueShift = RedShift;
		RedShift = uTemp;
	}
#endif

	// If the transparent color is all ones, then all bits need to have alpha enabled
	if (piRGBA_GETALPHA(TransparentColor))
	{
		SearchColor = -1;
	}
	else
	{
		SearchColor = ((piRGBA_GETRED(TransparentColor) >> RedBits) << RedShift) |
						((piRGBA_GETGREEN(TransparentColor) >> GreenBits) << GreenShift) |
						((piRGBA_GETBLUE(TransparentColor) >> BlueBits) << BlueShift);
	}

  if (lppisd->PixelFormat.dwRGBBitCount <= 8)
  {

    for (i=0; i < height; i++)
    {
	    cptr = ((char*)lppisd->Surface + i*lppisd->Pitch);
		rgbptr = (piRGBTRIPLE*)(bits + i*bitsPitch);

      for (j=0; j < width; j++, rgbptr++)
      {
        Color = ((rgbptr->rgbtRed >> RedBits) << RedShift) |
                ((rgbptr->rgbtGreen >> GreenBits) << GreenShift) |
                ((rgbptr->rgbtBlue >> BlueBits) << BlueShift);
        if (Color == SearchColor)
        {
          *cptr++ = 0;
        }
        else
        {
          *cptr++ = Color | AlphaMask;
        }
      }
    }
  }
  else
  if (lppisd->PixelFormat.dwRGBBitCount <= 16)
  {
    for (i=0; i < height; i++)
    {
	    sptr = (short*)((char*)lppisd->Surface + i*lppisd->Pitch);
		rgbptr = (piRGBTRIPLE*)(bits + i*bitsPitch);

      for (j=0; j < width; j++, rgbptr++)
      {
        Color = ((rgbptr->rgbtRed >> RedBits) << RedShift) |
                ((rgbptr->rgbtGreen >> GreenBits) << GreenShift) |
                ((rgbptr->rgbtBlue >> BlueBits) << BlueShift);
        if (Color == SearchColor)
        {
          *sptr++ = 0;
        }
        else
        {
          *sptr++ = Color | AlphaMask;
        }
      }
    }
  }
  else
  if (lppisd->PixelFormat.dwRGBBitCount <= 24)
  {
    for (i=0; i < height; i++)
    {
    	cptr = ((char*)lppisd->Surface + i*lppisd->Pitch);
		rgbptr = (piRGBTRIPLE*)(bits + i*bitsPitch);
      for (j=0; j < width; j++, rgbptr++)
      {
        Color = ((rgbptr->rgbtRed >> RedBits) << RedShift) |
                ((rgbptr->rgbtGreen >> GreenBits) << GreenShift) |
                ((rgbptr->rgbtBlue >> BlueBits) << BlueShift);
        if (Color == SearchColor)
        {
          *cptr++ = 0;
          *cptr++ = 0;
          *cptr++ = 0;
        }
        else
        {
			Color |= AlphaMask;
          *cptr++ = (Color & 0xff0000) >> 16;
          *cptr++ = (Color & 0x00ff00) >> 8;
          *cptr++ = (Color & 0x0000ff);
        }
      }
    }
  }
  else
  if (lppisd->PixelFormat.dwRGBBitCount <= 32)
  {
    for (i=0; i < height; i++)
    {
   		lptr = (long*)((char*)lppisd->Surface + i*lppisd->Pitch);
		rgbptr = (piRGBTRIPLE*)(bits + i*bitsPitch);
      for (j=0; j < width; j++, rgbptr++)
      {
        Color = ((rgbptr->rgbtRed >> RedBits) << RedShift) |
                ((rgbptr->rgbtGreen >> GreenBits) << GreenShift) |
                ((rgbptr->rgbtBlue >> BlueBits) << BlueShift);

        if (Color == SearchColor)
        {
          *lptr++ = 0;
        }
        else
        {
          *lptr++ = Color | AlphaMask;
        }
      }
    }
  }

  return(TRUE);
}

LOGICAL ldpixCopySurfaceToBits(piSURFACEDESC* lppisd, UCHAR* bits)
{
piRGBTRIPLE*  rgbptr;
char*    cptr;
short*   sptr, s;
int     i, j, width, height;
int     RedBits, RedShift, RedMask;
int     GreenBits, GreenShift, GreenMask;
int     BlueBits, BlueShift, BlueMask;
int     AlphaBits, AlphaShift, AlphaMask;

	rgbptr = (piRGBTRIPLE*)bits;

	RedMask     = lppisd->PixelFormat.dwRBitMask;
	GreenMask   = lppisd->PixelFormat.dwGBitMask;
	BlueMask    = lppisd->PixelFormat.dwBBitMask;
	AlphaMask   = lppisd->PixelFormat.dwRGBAlphaBitMask;

	RedBits     = 8 - piFindBitCount(RedMask);
	RedShift    = piFindBitShift(RedMask);
	GreenBits   = 8 - piFindBitCount(GreenMask);
	GreenShift  = piFindBitShift(GreenMask);
	BlueBits    = 8 - piFindBitCount(BlueMask);
	BlueShift   = piFindBitShift(BlueMask);
	AlphaBits   = piFindBitCount(AlphaMask);
	AlphaShift  = piFindBitShift(AlphaMask);

	width = lppisd->Width;
	height = lppisd->Height;
/*
	if (lppisd->PixelFormat.dwRGBBitCount <= 8)
	{
		cptr = (char*)lppisd->Surface;
		for (i=0; i < height; i++)
		{
			for (j=0; j < width; j++, rgbptr++)
			{
				Color = ((rgbptr->rgbtRed >> RedBits) << RedShift) |
						((rgbptr->rgbtGreen >> GreenBits) << GreenShift) |
						((rgbptr->rgbtBlue >> BlueBits) << BlueShift);
				if (Color == SearchColor)
				{
					*cptr++ = 0;
				}
				else
				{
					*cptr++ = Color | AlphaMask;
					Count += 1;
				}
			}

			cptr += lppisd->Pitch - width;
		}
	}
	else
*/
	if (lppisd->PixelFormat.dwRGBBitCount <= 16)
	{
		cptr = (char*)bits;
		sptr = (short*)lppisd->Surface;
		GreenShift -= GreenBits;
		RedShift -= RedBits;
		for (i=0; i < height; i++)
		{
			for (j=0; j < width; j++)
			{
				s = *sptr++;
				*cptr++ = ((s & BlueMask)) << BlueBits;
				*cptr++ = ((s & GreenMask) >> GreenShift);
				*cptr++ = ((s & RedMask) >> RedShift);
			}

			sptr += lppisd->Pitch / 2 - width;
		}
	}
	else
	if (lppisd->PixelFormat.dwRGBBitCount <= 24)
	{
		cptr = (char*)lppisd->Surface;
		for (i=0; i < height; i++)
		{
			for (j=0; j < width; j++, rgbptr++)
			{
				rgbptr->rgbtBlue = *cptr++;
				rgbptr->rgbtGreen = *cptr++;
				rgbptr->rgbtRed = *cptr++;
			}

			cptr += lppisd->Pitch - width * 3;
		}
	}
	else
	if (lppisd->PixelFormat.dwRGBBitCount <= 32)
	{
		cptr = (char*)lppisd->Surface;
		for (i=0; i < height; i++)
		{
			for (j=0; j < width; j++, rgbptr++)
			{
				rgbptr->rgbtBlue = *cptr++;
				rgbptr->rgbtGreen = *cptr++;
				rgbptr->rgbtRed = *cptr++;
				cptr++;
			}

			cptr += lppisd->Pitch - width * 4;
		}
	}

	return(TRUE);
}

LOGICAL ldpixCopyAlphaToSurface(UCHAR* bits, int bitsPitch, int width, int height, piSURFACEDESC* lppisd)
{
char*    cptr;
short*   sptr;
long*    lptr;
int     i, j;
int     RedBits, RedShift, RedMask;
int     GreenBits, GreenShift, GreenMask;
int     BlueBits, BlueShift, BlueMask;
int     AlphaBits, AlphaShift, AlphaMask;
int     Color, ColorMask;
UCHAR* bitp;

  if(bitsPitch == 0)
	    bitsPitch = width;


  RedMask     = lppisd->PixelFormat.dwRBitMask;
  GreenMask   = lppisd->PixelFormat.dwGBitMask;
  BlueMask    = lppisd->PixelFormat.dwBBitMask;
  AlphaMask   = lppisd->PixelFormat.dwRGBAlphaBitMask;
  ColorMask   = RedMask | GreenMask | BlueMask;

  RedBits     = 8 - piFindBitCount(RedMask);
  RedShift    = piFindBitShift(RedMask);
  GreenBits   = 8 - piFindBitCount(GreenMask);
  GreenShift  = piFindBitShift(GreenMask);
  BlueBits    = 8 - piFindBitCount(BlueMask);
  BlueShift   = piFindBitShift(BlueMask);
  AlphaBits   = piFindBitCount(AlphaMask);
  AlphaShift  = piFindBitShift(AlphaMask);

  if (lppisd->PixelFormat.dwRGBBitCount <= 8)
  {
    for (i=0; i < height; i++)
    {
	    cptr = ((char*)lppisd->Surface + i*lppisd->Pitch);
		bitp = bits + i*bitsPitch;

      for (j=0; j < width; j++, bitp++)
      {
        Color = *cptr & ColorMask;
        *cptr++ = Color | (AlphaMask & ((*bitp >> (8 - AlphaBits)) << AlphaShift));
      }
    }
  }
  else
  if (lppisd->PixelFormat.dwRGBBitCount <= 16)
  {
   for (i=0; i < height; i++)
    {
	    sptr = (short*)((char*)lppisd->Surface + i*lppisd->Pitch);
 		bitp = bits + i*bitsPitch;
      for (j=0; j < width; j++, sptr++,bitp++)
      {
        Color = *sptr & ColorMask;
        *sptr = Color | (AlphaMask & ((*bitp >> (8 - AlphaBits)) << AlphaShift));
      }
    }
  }
  else
  if (lppisd->PixelFormat.dwRGBBitCount <= 32)
  {
   for (i=0; i < height; i++)
    {
  		lptr = (long*)((char*)lppisd->Surface + i*lppisd->Pitch);
  		bitp = bits + i*bitsPitch;
      for (j=0; j < width; j++, bitp++)
      {
        Color = *lptr & ColorMask;
        *lptr++ = Color | (AlphaMask & ((*bitp >> (8 - AlphaBits)) << AlphaShift));
      }
    }
  }

  return(TRUE);
}

LOGICAL ldpixSetAlphaToSurface(int AlphaPercent, piSURFACEDESC* lppisd)
{
char*    cptr;
short*   sptr;
long*    lptr;
int     i, j, width, height;
int     RedMask;
int     GreenMask;
int     BlueMask;
int     AlphaBits, AlphaShift, AlphaMask, AlphaValue;
int     Color, ColorMask;

  width = lppisd->Width;
  height = lppisd->Height;

  RedMask     = lppisd->PixelFormat.dwRBitMask;
  GreenMask   = lppisd->PixelFormat.dwGBitMask;
  BlueMask    = lppisd->PixelFormat.dwBBitMask;
  AlphaMask   = lppisd->PixelFormat.dwRGBAlphaBitMask;
  ColorMask   = RedMask | GreenMask | BlueMask;

  AlphaBits   = piFindBitCount(AlphaMask);
  AlphaShift  = piFindBitShift(AlphaMask);
  AlphaValue  = AlphaBits * AlphaPercent / 100;
  AlphaValue  = (AlphaValue << AlphaShift) & AlphaMask;

  if (lppisd->PixelFormat.dwRGBBitCount <= 8)
  {
    cptr = (char*)lppisd->Surface;
    for (i=0; i < height; i++)
    {
      for (j=0; j < width; j++)
      {
        Color = *cptr & ColorMask;
        *cptr++ = Color | AlphaValue;
      }

      cptr += lppisd->Pitch - width;
    }
  }
  else
  if (lppisd->PixelFormat.dwRGBBitCount <= 16)
  {
    sptr = (short*)lppisd->Surface;
    for (i=0; i < height; i++)
    {
      for (j=0; j < width; j++, sptr++)
      {
        Color = *sptr & ColorMask;
        *sptr = Color | AlphaValue;
      }

      sptr += lppisd->Pitch / 2 - width;
    }
  }
  else
  if (lppisd->PixelFormat.dwRGBBitCount <= 32)
  {
    lptr = (long*)lppisd->Surface;
    for (i=0; i < height; i++)
    {
      for (j=0; j < width; j++)
      {
        Color = *lptr & ColorMask;
        *lptr++ = Color | AlphaValue;
      }

      lptr += lppisd->Pitch / 4 - width;
    }
  }

  return(TRUE);
}

LOGICAL ldpixCopyBitsToSubsurface(UCHAR* bits, piRGBQUAD* pal, int bwidth, int bheight, int row, int col, piSURFACEDESC* lppisd, UINT32 TransparentColor)
{
piRGBTRIPLE*  rgbptr, RgbColor;
char*    cptr;
short*   sptr;
long*    lptr;
int     i, j, width, height, offset;//, Count=0;
int     RedBits, RedShift, RedMask;
int     GreenBits, GreenShift, GreenMask;
int     BlueBits, BlueShift, BlueMask;
int     AlphaBits, AlphaShift, AlphaMask;
int     SearchColor, Color;
piRGBQUAD PalColor;

  rgbptr = (piRGBTRIPLE*)bits;
  width = std::min(bwidth, (int)lppisd->Width);
  if (bwidth + col > (int)lppisd->Width)
  {
    width = lppisd->Width - col;
  }
  height = std::min(bheight, (int)lppisd->Height);
  if (bheight + row > (int)lppisd->Height)
  {
    height = lppisd->Height - row;
  }
  offset = row * lppisd->Width + col;

  RedMask     = lppisd->PixelFormat.dwRBitMask;
  GreenMask   = lppisd->PixelFormat.dwGBitMask;
  BlueMask    = lppisd->PixelFormat.dwBBitMask;
  AlphaMask   = lppisd->PixelFormat.dwRGBAlphaBitMask;

  RedBits     = 8 - piFindBitCount(RedMask);
  RedShift    = piFindBitShift(RedMask);
  GreenBits   = 8 - piFindBitCount(GreenMask);
  GreenShift  = piFindBitShift(GreenMask);
  BlueBits    = 8 - piFindBitCount(BlueMask);
  BlueShift   = piFindBitShift(BlueMask);
  AlphaBits   = piFindBitCount(AlphaMask);
  AlphaShift  = piFindBitShift(AlphaMask);

  // If the transparent color is all ones, then all bits need to have alpha enabled
  if (piRGBA_GETALPHA(TransparentColor))
  {
    SearchColor = -1;
  }
  else
  {
    SearchColor = ((piRGBA_GETRED(TransparentColor) >> RedBits) << RedShift) |
                  ((piRGBA_GETGREEN(TransparentColor) >> GreenBits) << GreenShift) |
                  ((piRGBA_GETBLUE(TransparentColor) >> BlueBits) << BlueShift);
  }

  if (lppisd->PixelFormat.dwRGBBitCount <= 8)
  {
    cptr = (char*)lppisd->Surface + offset;
    if (pal == nullptr)
    {
      for (i=0; i < height; i++)
      {
        for (j=0; j < width; j++, rgbptr++)
        {
          Color = ((rgbptr->rgbtRed >> RedBits) << RedShift) |
                  ((rgbptr->rgbtGreen >> GreenBits) << GreenShift) |
                  ((rgbptr->rgbtBlue >> BlueBits) << BlueShift);
          if (Color == SearchColor)
          {
            cptr[j] = 0;
          }
          else
          {
            cptr[j] = Color | AlphaMask;
//            Count += 1;
          }
        }

        cptr += lppisd->Width;
      }
    }
    else
    {
      for (i=0; i < height; i++)
      {
        for (j=0; j < width; j++, bits++)
        {
          PalColor = pal[*bits];
          Color = ((PalColor.rgbRed >> RedBits) << RedShift) |
                  ((PalColor.rgbGreen >> GreenBits) << GreenShift) |
                  ((PalColor.rgbBlue >> BlueBits) << BlueShift);
          if (Color == SearchColor)
          {
            cptr[j] = 0;
          }
          else
          {
            cptr[j] = Color | AlphaMask;
//            Count += 1;
          }
        }

        cptr += lppisd->Width;
      }
    }
  }
  else
  if (lppisd->PixelFormat.dwRGBBitCount <= 16)
  {
    sptr = (short*)lppisd->Surface + offset;
    if (pal == nullptr)
    {
      for (i=0; i < height; i++)
      {
        for (j=0; j < width; j++)
        {
          RgbColor = rgbptr[j];
          Color = ((RgbColor.rgbtRed >> RedBits) << RedShift) |
                  ((RgbColor.rgbtGreen >> GreenBits) << GreenShift) |
                  ((RgbColor.rgbtBlue >> BlueBits) << BlueShift);
          if (Color == SearchColor)
          {
            sptr[j] = 0;
          }
          else
          {
            sptr[j] = Color | AlphaMask;
//            Count += 1;
          }
        }

        sptr += lppisd->Width;
        rgbptr += bwidth;
      }
    }
    else
    {
      for (i=0; i < height; i++)
      {
        for (j=0; j < width; j++, bits++)
        {
          PalColor = pal[*bits];
          Color = ((PalColor.rgbRed >> RedBits) << RedShift) |
                  ((PalColor.rgbGreen >> GreenBits) << GreenShift) |
                  ((PalColor.rgbBlue >> BlueBits) << BlueShift);
          if (Color == SearchColor)
          {
            sptr[j] = 0;
          }
          else
          {
            sptr[j] = Color | AlphaMask;
//            Count += 1;
          }
        }

        sptr += lppisd->Width;
      }
    }
  }
  else
  if (lppisd->PixelFormat.dwRGBBitCount <= 32)
  {
    lptr = (long*)lppisd->Surface + offset;
    if (pal == nullptr)
    {
      for (i=0; i < height; i++)
      {
        for (j=0; j < width; j++, rgbptr++)
        {
          Color = ((rgbptr->rgbtRed >> RedBits) << RedShift) |
                  ((rgbptr->rgbtGreen >> GreenBits) << GreenShift) |
                  ((rgbptr->rgbtBlue >> BlueBits) << BlueShift);
          if (Color == SearchColor)
          {
            lptr[j] = 0;
          }
          else
          {
            lptr[j] = Color | AlphaMask;
//            Count += 1;
          }
        }

        lptr += lppisd->Width;
      }
    }
    else
    {
      for (i=0; i < height; i++)
      {
        for (j=0; j < width; j++, bits++)
        {
          PalColor = pal[*bits];
          Color = ((PalColor.rgbRed >> RedBits) << RedShift) |
                  ((PalColor.rgbGreen >> GreenBits) << GreenShift) |
                  ((PalColor.rgbBlue >> BlueBits) << BlueShift);
          if (Color == SearchColor)
          {
            lptr[j] = 0;
          }
          else
          {
            lptr[j] = Color | AlphaMask;
//            Count += 1;
          }
        }

        lptr += lppisd->Width;
      }
    }
  }
/*
  WriteDebug("Updated %d alpha values in the output texture", Count);
*/
  return(TRUE);
}

void* ldpixConvert8To24(int width, int height, piRGBQUAD* pal, UCHAR* inbits)
{
UCHAR*	outbits, *tmpbits;
int		i, j;

	outbits = (UCHAR*)memAlloc(width * 3 * height);
	if (outbits == nullptr)
	{
		return(nullptr);
	}

	tmpbits = outbits;
	for (i=0; i < height; i++)
	{
		for (j=0; j < width; j++)
		{
			*tmpbits++ = pal[*inbits].rgbRed;
			*tmpbits++ = pal[*inbits].rgbGreen;
			*tmpbits++ = pal[*inbits++].rgbBlue;
		}
	}

	return(outbits);
}

void ldpixInvertImage(int bits, int width, int height, UCHAR* inbits)
{
UCHAR*	ptr;
int		i, up, dn;

	width *= bits / 8;
	ptr = (UCHAR*)memAlloc(width);
	up = 0;
	dn = (height - 1) * width;
	height /= 2;
	for (i=0; i < height; i++, up += width, dn -= width)
	{
		memcpy(ptr, &inbits[up], width);
		memcpy(&inbits[up], &inbits[dn], width);
		memcpy(&inbits[dn], ptr, width);
	}

	memFree(ptr);
}

void ldpixScaleImage(piPIXELFORMAT* pixformat, int inwidth, int inheight, int inpitch, UCHAR* inbits,
					 int outwidth, int outheight, int outpitch, UCHAR* outbits)
{
UCHAR*	inptr, *outptr;
int		i, j, k, size;
int		widthinoffset, heightinoffset, widthoutoffset, heightoutoffset;
int		widthinscale, heightinscale, widthoutscale, heightoutscale;
int		widthinhalf, heightinhalf;

int		x_index, y_index, base_offset, widthblendoffset, heightblendoffset;
unsigned long	points[4];
UINT16*	inptr_16, *outptr_16;

int		red, blue, green, alpha;

	int bits = pixformat->BitCount;

	size = bits / 8;

	widthinscale = inwidth * 65536 / outwidth;
	widthoutscale = 65536;

	heightinscale = inheight * 65536 / outheight;
	heightoutscale = 65536;

	widthinhalf = widthinscale / 2;
	heightinhalf = heightinscale / 2;

	inptr = inbits;
	outptr = outbits;
	i = 0;
	do
	{
		heightinoffset = (i * heightinscale + heightinhalf) / 65536;
		if (heightinoffset >= inheight)
		{
			break;
		}

		heightoutoffset = (i * heightoutscale) / 65536;
		if (heightoutoffset >= outheight)
		{
			break;
		}

		y_index = heightinoffset;

		heightinoffset *= inpitch;
		heightoutoffset *= outpitch;
		j = 0;
		do
		{
			widthinoffset = (j * widthinscale + widthinhalf) / 65536;
			if (widthinoffset >= inwidth)
			{
				break;
			}

			widthoutoffset = (j * widthoutscale) / 65536;
			if (widthoutoffset >= outwidth)
			{
				break;
			}

			x_index = widthinoffset;

			widthinoffset *= size;
			widthoutoffset *= size;

			// seems everything is coming in as 16 bits, but just in case ..
			// use a low quality one for non 16 bit images
			// FIXME - if needed
			if (bits != 16)
			{
				for (k=0; k < size; k++)
				{
					outptr[heightoutoffset + widthoutoffset + k] =
						inptr[heightinoffset + widthinoffset + k];
				}
			}
			else
			{
				base_offset = (heightinoffset + widthinoffset)/2;
				inptr_16 = (UINT16*)inptr;
				outptr_16 = (UINT16*)outptr;

//				WriteDebug ("Picking %d, %d", x_index, y_index);

				// basic algorithm .. if we have an odd index, subtract, otherwise add
				if (x_index & 1)	// fast modulo 2
				{
					widthblendoffset = -size;
				}
				else
				{
					widthblendoffset = size;
				}

				if (y_index & 1)	// fast modulo 2
				{
					heightblendoffset = -inpitch;
				}
				else
				{
					heightblendoffset = inpitch;
				}

				heightblendoffset /= 2;
				widthblendoffset /= 2;


				points[0] = inptr_16[base_offset];
				points[1] = inptr_16[base_offset + heightblendoffset];
				points[2] = inptr_16[base_offset + widthblendoffset];
				points[3] = inptr_16[base_offset + heightblendoffset + widthblendoffset];

				red = (((points[0]&pixformat->RedBitMask) + (points[1]&pixformat->RedBitMask) +
					(points[2]&pixformat->RedBitMask) + (points[3]&pixformat->RedBitMask)) / 4) & pixformat->RedBitMask;
				blue = (((points[0]&pixformat->BlueBitMask) + (points[1]&pixformat->BlueBitMask) +
					(points[2]&pixformat->BlueBitMask) + (points[3]&pixformat->BlueBitMask)) / 4) & pixformat->BlueBitMask;
				green = (((points[0]&pixformat->GreenBitMask) + (points[1]&pixformat->GreenBitMask) +
					(points[2]&pixformat->GreenBitMask) + (points[3]&pixformat->GreenBitMask)) / 4) & pixformat->GreenBitMask;
				alpha = (((points[0]&pixformat->AlphaBitMask) + (points[1]&pixformat->AlphaBitMask) +
					(points[2]&pixformat->AlphaBitMask) + (points[3]&pixformat->AlphaBitMask)) / 4) & pixformat->AlphaBitMask;
				// Averaging filter
				outptr_16[(heightoutoffset + widthoutoffset)/2] = red + blue + green + alpha;

				// Point filter
				//	outptr_16[(heightoutoffset + widthoutoffset)/2] = inptr_16[base_offset];

			}
			j += 1;
		}
		while(TRUE);

		i += 1;
	}
	while(TRUE);
}

LOGICAL ldpixCopySurfaceToSurface(piSURFACEDESC* lppisd1, piSURFACEDESC* lppisd2)
{
char*		cptr1, *cptr2;
short*		sptr1, *sptr2;
long*		lptr1, *lptr2;
int			i, j, width, height;

	if (lppisd1 == nullptr || lppisd2 == nullptr)
	{
		WriteDebug("Surface's descriptor is nullptr");
		return(FALSE);
	}

	if (lppisd1->PixelFormat.dwRGBBitCount != lppisd2->PixelFormat.dwRGBBitCount)
	{
		WriteDebug("Surface's bitcounts do not match");
		return(FALSE);
	}

	if (lppisd1->Width != lppisd2->Width ||
		lppisd1->Height != lppisd2->Height)
	{
		WriteDebug("Surface's dimensions do not match");
		return(FALSE);
	}

  width = (int)lppisd1->Width;
  height = (int)lppisd1->Height;
  if (lppisd1->PixelFormat.dwRGBBitCount <= 8)
  {
    cptr1 = (char*)lppisd1->Surface;
    cptr2 = (char*)lppisd2->Surface;
    for (i=0; i < height; i++)
    {
      for (j=0; j < width; j++)
      {
        *cptr2++ = *cptr1++;
      }

      cptr1 += lppisd1->Pitch - width;
      cptr2 += lppisd2->Pitch - width;
    }
  }
  else
  if (lppisd1->PixelFormat.dwRGBBitCount <= 16)
  {
    sptr1 = (short*)lppisd1->Surface;
    sptr2 = (short*)lppisd2->Surface;
    for (i=0; i < height; i++)
    {
      for (j=0; j < width; j++)
      {
        *sptr2++ = *sptr1++;
      }

      sptr1 += lppisd1->Pitch / 2 - width;
      sptr2 += lppisd2->Pitch / 2 - width;
    }
  }
  else
  if (lppisd1->PixelFormat.dwRGBBitCount <= 32)
  {
    lptr1 = (long*)lppisd1->Surface;
    lptr2 = (long*)lppisd2->Surface;
    for (i=0; i < height; i++)
    {
      for (j=0; j < width; j++)
      {
        *lptr2++ = *lptr1++;
      }

      lptr1 += lppisd1->Pitch / 4 - width;
      lptr2 += lppisd2->Pitch / 4 - width;
    }
  }

  return(TRUE);
}
