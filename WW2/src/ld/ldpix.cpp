#include <syscfg.h> // Path is project specific.
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

//#define TEX_FILE_SUPPORT	1
//#define TIMING_TEST		1

#define	DEBUG_REDUCTION
#undef	DEBUG_REDUCTION

#ifdef TEX_FILE_SUPPORT
	int ldtexLoadTexFileToSurface(char* Filename, UINT32 TransparentColor, UINT32 WantFlags);
#endif

/* for looking for duplicat textures
typedef std::pair<std::string,UINT32> TextureSig;
typedef std::map<TextureSig,UINT32> TextureCountMap;

class TextureCountReporter : public TextureCountMap
{
public:

	TextureCountReporter() : TextureCountMap()
	{
	}

	virtual ~TextureCountReporter()
	{
		TextureCountReporter::iterator it = begin();
		TextureCountMap possibles;

		for(; it != end(); it++)
		{
			if(it->second > 1)
				possibles[it->first] = it->second;
		}
	}
};

static TextureCountReporter tcounts;
*/

typedef int (*LoadFileToSurface)(const char* Filename, UINT32 TransparentColor, UINT32 WantFlags);

static int _loadImageFileToSurface(const char* name, UINT32 transcolor, UINT32 flags);

struct GFX_FORMATS
{
	char  Extension[5];
	LoadFileToSurface Routine;

} GfxFormats[]={

#ifdef TEX_FILE_SUPPORT
//				{"tex",ldtexLoadTexFileToSurface},
#endif
                {"dds", ldLoadTexture},
 //               {"3df",ld3dfLoad3DFFileToSurface},
                {"bmp",ldbmpLoadBMPFileToSurface},
 //               {"iff",ldlbmLoadLBMFileToSurface},
//                {"jpg",ldjpgLoadJPGFileToSurface},
 //               {"lbm",ldlbmLoadLBMFileToSurface},
                {"pcx",ldpcxLoadPCXFileToSurface},
 //               {"ppm",ldppmLoadPPMFileToSurface},
                {"png", ldLoadTexture},
 //               {"rgb",ldrgbLoadRGBFileToSurface},
 //               {"tga",ldtgaLoadTGAFileToSurface},
 //               {"tif",ldtifLoadTIFFFileToSurface},
 //               {"tiff",ldtifLoadTIFFFileToSurface},
                {"",nullptr}
               };

static LOGICAL reduceTextures = FALSE;

int ldpixLoadImageFileToSurface(const char* Filename, UINT32 TransparentColor, UINT32 Flags)
{
  SINT32 texid = _loadImageFileToSurface(Filename, TransparentColor, Flags);

    if(!(Flags & LDPIX_NO_TEXTURE_GENERATE))
    {
        piGetTextureHandle(texid, 0);
    }

	return texid;
}

/*
static void _checkTexture(int texid)
{
	if(texid >= 0)
	{
		TEXTURE* t = piGetTextureData(texid);

		const char* name = t->name;
		UINT32 ts = t->imageheight[0] * t->imagewidth[0];
		const char* ptr = nullptr;

		while((ptr = strstr(name, "\\")) != nullptr)
		{
			name = ptr + 1;
		}

		TextureSig p = std::make_pair(name,ts);

		TextureCountMap::iterator found = tcounts.find(p);

		if(found != tcounts.end())
		{
			tcounts[p]++;
		}
		else
		{
			tcounts[p] = 1;
		}
	}
}
*/

int _loadImageFileToSurface(const char* Filename, UINT32 TransparentColor, UINT32 Flags)
{
	UINT32		WantFlags;
	char*		ptr;
	char		basename[256], localname[256], tmpname[256], alphaname[256];
	int			i, j, texid, width, height;
	int			newwidth, newheight;
	TEXTURE*	lptex;
	int			mip_texid;
	//static int	count;

#ifdef LOAD_TIMER
SINT32		t1, t2, t3, t4;
#endif

#ifdef LOAD_TIMER
	t1 = piReadTimeStamp32();
#endif

	bool WantMipmap = (Flags & LDPIX_MIPMAP) != 0;
	bool WantDXT = (Flags & LDPIX_NO_DXT) == 0;

	if (WantMipmap
		&& (strncasecmp("ui\\", Filename, 3) == 0
			|| strncasecmp("\\fonts\\", Filename, 7) == 0
			|| strncasecmp("splashscreen", Filename, 12) == 0))
	{
		WantMipmap = FALSE;
	}

    WantFlags = (WantMipmap ? CRT_FLAG_MIPMAP : CRT_FLAG_NONE) | (WantDXT ? CRT_FLAG_NONE : CRT_FLAG_WRITABLE);

	strcpy(basename, Filename);
	_strlwr(basename);

	ptr = (char*)strrchr(basename, '.');
	if (ptr)
	{
		*ptr = 0;
	}

	texid = piGetNamedTexture(basename);
	if (piIsTexture(texid, 0))
	{
		piAddTexRef(texid);
		return(texid);
	}

	strcpy(localname, basename);

    Write_Log(LOG_TEXTURE, LOG_INFO, "[TEXTURE] loading %s", localname);

	size_t suffixOffset = strprintf(tmpname, "%s.", localname);
	for (i=0; GfxFormats[i].Routine; i++)
	{
		strncpy(tmpname + suffixOffset, GfxFormats[i].Extension, sizeof(tmpname) - suffixOffset);

		if (fioAccess(tmpname))
		{
			break;
		}
	}

	if (GfxFormats[i].Routine == nullptr)
	{
		Write_Debug("Texture file not found: %s", localname);
		return(-1);
	}

    const char* regionPath = fioGetRegionalPath(tmpname);

    if(GfxFormats[i].Routine == ldLoadTexture)
    {
        return ldLoadTexture(regionPath, TransparentColor, Flags);
    }

#ifdef TEX_FILE_SUPPORT
	if (GfxFormats[i].Routine == ldtexLoadTexFileToSurface)
	{
		SINT32 t1, t2;
		static	UINT32	total_pixels;

		t1 = piReadTimeStamp32();
		texid = ldtexLoadTexFileToSurface(tmpname, TransparentColor, WantFlags);
		if (!piIsTexture(texid, 0))
		{
			return(-1);
		}

		lptex = piGetTextureData(texid);
		piSetTextureName(texid, localname);

		t2 = piReadTimeStamp32();
		Write_Debug ("Cycles spent on tex file %s (%d, %d) = %d .. average = %d",
					tmpname,
					lptex->imagewidth[0], lptex->imageheight[0], t2 - t1,
					((t2-t1)/(lptex->imagewidth[0]*lptex->imageheight[0])));

		total_pixels += (lptex->imagewidth[0]*lptex->imageheight[0]);
		Write_Debug ("Total pixels = %d\n", total_pixels);
		return(texid);
	}
#endif

	bool loadalpha = false;

#if defined (EVENT_LOG_EXTREME) || defined (DEBUG_REDUCTION)
	Write_Debug("Texture loading: %s", localname);
#endif

	lstrcpy(alphaname, localname);
	lstrcat(alphaname, "_alpha.bmp");
	if (fioAccess(alphaname))
	{
		TransparentColor = piRGBA_SETALPHA(TransparentColor,8);
		loadalpha = true;

#ifdef CONVERSION_PROGRAM
		nErrorCode = ALPHA_FILE_FOUND;
		return(-1);
#endif

	}

#ifdef LOAD_TIMER
	t2 = piReadTimeStamp32();
	Write_Debug ("Times:\n\t\tInit: %10d", t2-t1);
#endif

	texid = GfxFormats[i].Routine(regionPath, TransparentColor, WantFlags);

#ifdef LOAD_TIMER
	t3 = piReadTimeStamp32();
	Write_Debug ("\t\tRead: %10d", t3-t2);
#endif

	if (!piIsTexture(texid, 0))
	{
        Write_Debug("Failed loading %s", regionPath);
		return(-1);
	}

	lptex = piGetTextureData(texid);
	piSetTextureName(texid, localname);

	//_checkTexture(texid);

	width = lptex->imagewidth[0];
	height = lptex->imageheight[0];

	if (loadalpha)
	{
		if (!ldbmpLoadBMPAlphaFileToSurface(alphaname, texid, 0))
		{
			piFreeMipTextures(texid);
			return(-1);
		}

		lptex->hasalpha = TRUE;
	}

	// Create a mipmap for this texture
	if (width <= MinTextureDim ||
		height <= MinTextureDim)
	{
		reduceTextures = FALSE;
	}

	bool actuallyReduce = false;
	// let's determine if we want to reduce
	if (reduceTextures)
	{
		if (lptex->hasalpha)
		{
			if ((width > 256) || (height > 256))
			{
				actuallyReduce = true;
#ifdef DEBUG_REDUCTION
				Write_Debug ("DEBUG_REDUCTION: Reducing large texture %s .. (%d, %d)", localname, width, height);
#endif
			}
			else
			{
#ifdef DEBUG_REDUCTION
				Write_Debug ("DEBUG_REDUCTION:  Not reducing %s due to alpha", localname);
#endif
			}
		}
		else
		{
			if ((width <= 32) || (height <= 32))
			{
#ifdef DEBUG_REDUCTION
				Write_Debug ("DEBUG_REDUCTION: Not reducing small texture %s .. (%d, %d)", localname, width, height);
#endif
			}
			else
			{
				actuallyReduce = true;
			}
		}
	}

	// reduce texture if we need to
	while (actuallyReduce)
	{
		int new_texid;
		piSURFACEDESC	ddsd1, ddsd2;
		TEXTURE*	new_lptex;

		newwidth = width / 2;
		newheight = height / 2;

		if (newwidth <= MinTextureDim || newheight <= MinTextureDim)
		{
#ifdef DEBUG_REDUCTION
			Write_Debug ("DEBUG_REDUCTION:  Not reducing %s due to size .. (%d, %d) < %d", localname, newwidth, newheight, MinTextureDim);
#endif
			break;
		}

		// check for custom level first
		sprintf(localname, "%s_1", basename);

		lstrcpy(tmpname, localname);
		lstrcat(tmpname, ".");
		lstrcat(tmpname, GfxFormats[i].Extension);

		if (fioAccess(tmpname))
		{
			lstrcpy(alphaname, localname);
			lstrcat(alphaname, "_alpha.bmp");
			loadalpha = false;
			if (fioAccess(alphaname))
			{
				TransparentColor = piRGBA_SETALPHA(TransparentColor, 8);
				loadalpha = true;
			}

			new_texid = GfxFormats[i].Routine(tmpname, TransparentColor, WantFlags);
			if (piIsTexture(new_texid, 0))
			{
				if (loadalpha)
				{
					if (!ldbmpLoadBMPAlphaFileToSurface(alphaname, new_texid, 0))
					{
						piFreeMipTextures(texid);
#ifdef DEBUG_REDUCTION
						Write_Debug ("DEBUG_REDUCTION:  Not reducing %s due to alpha load error while loading custom mipmap", localname);
#endif
						return(-1);
					}
				}

			}
		}
		else
		{
			new_texid = piCreateRawTexture(-1, 0, newwidth, newheight, TexBpp, piRGBA_GETALPHA(TransparentColor), WantFlags);
			if (!piIsTexture(new_texid, 0))
			{
				Write_Debug ("Failed to create new texture for reduction");
				piFreeMipTextures(texid);
				return(-1);
			}

			if (piLockTextureMemorySurface(texid, 0, &ddsd1))
			{
				if (piLockTextureMemorySurface(new_texid, 0, &ddsd2))
				{
					ldpixScaleImage(&ddsd1.PixelFormat,
						width, height, ddsd1.Pitch, (UCHAR*)ddsd1.Surface,
						newwidth, newheight, ddsd2.Pitch, (UCHAR*)ddsd2.Surface);

					piUnlockTextureMemorySurface(new_texid, 0);
				}

				piUnlockTextureMemorySurface(texid, 0);
			}
		}

		// texture pointers can be invalidated by adding a texture, so reacquire the lptex data
		lptex = piGetTextureData(texid);

		// copy data across from old texture to new texture
		new_lptex = piGetTextureData(new_texid);
		piSetTextureName(new_texid, lptex->name);

		width = new_lptex->imagewidth[0];
		height = new_lptex->imageheight[0];
		new_lptex->hasalpha = lptex->hasalpha;

		piFreeMipTextures(texid);

		texid = new_texid;
		lptex = new_lptex;

#ifdef DEBUG_REDUCTION
		Write_Debug ("DEBUG_REDUCTION:  Texture %s reduced", localname);
#endif

		break;
	}

	if (bMipmapEnable && WantMipmap)
	{
		// We create the mip levels for all OSes, now
		newwidth = width;
		newheight = height;

		for (j=1;; j++)
		{
			// Create a mipmap for this texture
			if (newwidth == MinTextureDim &&
				newheight == MinTextureDim)
			{
				break;
			}

			if (newwidth >= 2) newwidth = newwidth / 2;
			if (newheight >= 2) newheight = newheight / 2;

			// if we've reduced textures, grab the next lower mipmap
			//if (!lptex->hasalpha && reduceTextures)
			if (actuallyReduce)
				sprintf(localname, "%s_%d", basename, j+1);
			else
				sprintf(localname, "%s_%d", basename, j);

			lstrcpy(tmpname, localname);
			lstrcat(tmpname, ".");
			lstrcat(tmpname, GfxFormats[i].Extension);

			if (fioAccess(tmpname))
			{
				lstrcpy(alphaname, localname);
				lstrcat(alphaname, "_alpha.bmp");
				loadalpha = false;
				if (fioAccess(alphaname))
				{
					TransparentColor = piRGBA_SETALPHA(TransparentColor, 8);
					loadalpha = true;
				}

				mip_texid = GfxFormats[i].Routine(tmpname, TransparentColor, WantFlags);
				if (piIsTexture(mip_texid, 0))
				{
					piSURFACEDESC	ddsd1, ddsd2;

					if (loadalpha)
					{
						if (!ldbmpLoadBMPAlphaFileToSurface(alphaname, mip_texid, 0))
						{
							piFreeMipTextures(mip_texid);
							piFreeMipTextures(texid);
							return(-1);
						}

						lptex = piGetTextureData(texid);
						lptex->hasalpha = TRUE;
					}

					// OpenGL creates the mip levels along with the base level, no need to create them here
#if defined(DIRECT3D)
					texid = piCreateRawTexture(	texid, j, newwidth, newheight, TexBpp,
												piRGBA_GETALPHA(TransparentColor), WantFlags);
#endif

					// ok, we now need to move the data from mip_texid,0 to texid, j
					if (piLockTextureMemorySurface(texid, j, &ddsd1))
					{
						if (piLockTextureMemorySurface(mip_texid, 0, &ddsd2))
						{
							// copy ddsd2 to ddsd1
							if (!(ldpixCopySurfaceToSurface (&ddsd2, &ddsd1)))
							{
								piUnlockTextureMemorySurface(mip_texid, 0);
								piUnlockTextureMemorySurface(texid, j);
								Write_Debug ("Failed to copy mip surfaces!");
								piFreeMipTextures(mip_texid);
								piFreeMipTextures(texid);
								return(-1);
							}

							piUnlockTextureMemorySurface(mip_texid, 0);
						}
						else
						{
							Write_Debug ("failed to lock texture memory for mip texture");
							piFreeMipTextures(mip_texid);
							piFreeMipTextures(texid);
							return(-1);
						}

						piUnlockTextureMemorySurface(texid, j);
					}
					else
					{
						Write_Debug ("Failed to lock texture memory for new mip level");
						piFreeMipTextures(mip_texid);
						piFreeMipTextures(texid);
						return(-1);
					}

#ifdef EVENT_LOG_EXTREME
					Write_Debug ("Loaded custom mip level %d for %s", j, basename);
#endif
					piFreeMipTextures(mip_texid);
				}
				else
				{
					piFreeMipTextures(texid);
					return(-1);
				}
			}
			else
			{
				piSURFACEDESC	ddsd1, ddsd2;

				if (piLockTextureMemorySurface(texid, j-1, &ddsd1))
				{
					if (piLockTextureMemorySurface(texid, j, &ddsd2))
					{
						ldpixScaleImage(&(ddsd1.PixelFormat),
							width, height, ddsd1.Pitch, (UCHAR*)ddsd1.Surface,
							newwidth, newheight, ddsd2.Pitch, (UCHAR*)ddsd2.Surface);

						piUnlockTextureMemorySurface(texid, j);
					}

					piUnlockTextureMemorySurface(texid, j-1);
				}
			}

			if (width >= 2) width /= 2;
			if (height >= 2) height /= 2;
		}
	}

	// If this section of code is not executed the texture (as originally created) is used.
	// This code only converts the texture object from its original format to a compressed one.
#if defined(DIRECT3D)
	if (WantDXT)
	{
		if (lptex->hasalpha)
		{
			// Explicit alpha (DXT3)
			if (SupportDXT3)
			{
				piConvertTextureFormat(texid, piDXT3);
			}
		}
		else
		{
			// Opaque / one-bit alpha (DXT1)
			if (SupportDXT1)
			{
				piConvertTextureFormat(texid, piDXT1);
			}
		}
	}
#endif

#ifdef LOAD_TIMER
	t4 = piReadTimeStamp32();

	Write_Debug ("\t\tProc: %10d", t4-t3);
#endif

	return(texid);
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
  Write_Debug("Updated %d alpha values in the output texture", Count);
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

//				Write_Debug ("Picking %d, %d", x_index, y_index);

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
		Write_Debug("Surface's descriptor is nullptr");
		return(FALSE);
	}

	if (lppisd1->PixelFormat.dwRGBBitCount != lppisd2->PixelFormat.dwRGBBitCount)
	{
		Write_Debug("Surface's bitcounts do not match");
		return(FALSE);
	}

	if (lppisd1->Width != lppisd2->Width ||
		lppisd1->Height != lppisd2->Height)
	{
		Write_Debug("Surface's dimensions do not match");
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

void ldpixSetTextureReduction(LOGICAL reduce)
{
	reduceTextures = reduce;
}

LOGICAL ldpixGetTextureReduction()
{
	return reduceTextures;
}


#ifdef TEX_FILE_SUPPORT

struct tex_def
{
	UINT32	num_mips;
	UINT32	alpha_file;
	UINT32	holes_poked;
	UINT32	width[MAXMIPLEVELS];
	UINT32	height[MAXMIPLEVELS];
	// this isn't used in ldpix
	UCHAR*  bits[MAXMIPLEVELS];
};

struct texcRGBA
{
	UCHAR	rgbBlue;
	UCHAR	rgbGreen;
	UCHAR	rgbRed;
	UCHAR	rgbAlpha;

};

LOGICAL ldpixCopyAllBitsToSurface(UCHAR* bits, int width, int height, piSURFACEDESC* lppisd)
{
texcRGBA*  rgbptr;
char*    cptr;
short*   sptr;
long*    lptr;
int     i, j, Count=0;
int     RedBits, RedShift, RedMask;
int     GreenBits, GreenShift, GreenMask;
int     BlueBits, BlueShift, BlueMask;
int     AlphaBits, AlphaShift, AlphaMask;
int     Color;
UINT32  uTemp = 0;

  rgbptr = (texcRGBA*)bits;

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
		uTemp = BlueShift;
		BlueShift = RedShift;
		RedShift = uTemp;
	}
#endif

  if (lppisd->PixelFormat.dwRGBBitCount <= 8)
  {
    cptr = (char*)lppisd->Surface;
    for (i=0; i < height; i++)
    {
      for (j=0; j < width; j++, rgbptr++)
      {
        Color = ((rgbptr->rgbRed >> RedBits) << RedShift) |
                ((rgbptr->rgbGreen >> GreenBits) << GreenShift) |
                ((rgbptr->rgbBlue >> BlueBits) << BlueShift) |
				(((rgbptr->rgbAlpha >> AlphaBits) << AlphaShift ) & AlphaMask);
          *cptr++ = Color;
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
      for (j=0; j < width; j++, rgbptr++)
      {

//		  unsigned short	subr, subg, subb, suba;
//		  subr = (rgbptr->rgbRed >> RedBits) << RedShift;
//		  subg = (rgbptr->rgbGreen >> GreenBits) << GreenShift;
//		  subb = (rgbptr->rgbBlue >> BlueBits) << BlueShift;
//		  suba = (rgbptr->rgbAlpha >> AlphaBits) << AlphaShift;

//		  Color = subr | subg | subb | suba;

			*sptr++ =	((rgbptr->rgbRed >> RedBits) << RedShift) |
						((rgbptr->rgbGreen >> GreenBits) << GreenShift) |
						((rgbptr->rgbBlue >> BlueBits) << BlueShift) |
						(((rgbptr->rgbAlpha >> AlphaBits) << AlphaShift) & AlphaMask);

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
        Color = ((rgbptr->rgbRed >> RedBits) << RedShift) |
                ((rgbptr->rgbGreen >> GreenBits) << GreenShift) |
                ((rgbptr->rgbBlue >> BlueBits) << BlueShift) |
				(((rgbptr->rgbAlpha >> AlphaBits) << AlphaShift) & AlphaMask);

		  *cptr++ = (Color & 0xff0000) >> 16;
          *cptr++ = (Color & 0x00ff00) >> 8;
          *cptr++ = (Color & 0x0000ff);
      }

      cptr += lppisd->Pitch - width * 3;
    }
  }
  else
  if (lppisd->PixelFormat.dwRGBBitCount <= 32)
  {
    lptr = (long*)lppisd->Surface;
    for (i=0; i < height; i++)
    {
      for (j=0; j < width; j++, rgbptr++)
      {
        Color = ((rgbptr->rgbRed >> RedBits) << RedShift) |
                ((rgbptr->rgbGreen >> GreenBits) << GreenShift) |
                ((rgbptr->rgbBlue >> BlueBits) << BlueShift);
                (((rgbptr->rgbAlpha >> AlphaBits) << AlphaShift) & AlphaMask);

          *lptr++ = Color;
      }

      lptr += lppisd->Pitch / 4 - width;
    }
  }

  return(TRUE);
}


int ldtexLoadTexFileToSurface(char* Filename, UINT32 TransparentColor, UINT32 WantFlags)
{
	int				stream;
	tex_def			tex_data;
	int				texid;
	UINT32			i;
	piSURFACEDESC	pisd;
	size_t				cur_dat_size;
	UCHAR*			bits, *bufptr;
	TEXTURE*		lptex;
#ifdef TIMING_TEST
	SINT32			t1, t2, t3, t4;
#endif


#ifdef TIMING_TEST
	t1 = piReadTimeStamp32();
#endif

	stream = fioOpen(Filename);
	if (stream == -1)
	{
		return -1;
	}

	if (sizeof(tex_def) != fioRead(stream, &tex_data, sizeof(tex_def)))
	{
		fioClose(stream);
		return -1;
	}

	if (tex_data.alpha_file)
	{
		TransparentColor = piRGBA_SETALPHA(TransparentColor,8);
#ifdef CONVERSION_PROGRAM
		nErrorCode = ALPHA_FILE_FOUND;
		return -1;
#endif
	}
	else if (tex_data.holes_poked)
	{
		TransparentColor = piRGBA_SETALPHA(TransparentColor,8);
	}

#ifdef TIMING_TEST
	t2 = piReadTimeStamp32();
	Write_Debug ("Cycles spent on header for tex file %s = %d",
		Filename,
		t2 - t1);
#endif

	cur_dat_size = 0;
	for (i=0; i < tex_data.num_mips; i++)
	{
		cur_dat_size += (tex_data.width[i] * tex_data.height[i] * 4);
	}

#ifdef TIMING_TEST
	t1 = piReadTimeStamp32();
	t3 = piReadTimeStamp32();
#endif

	bits = memAlloc (cur_dat_size);

	if (cur_dat_size != fioRead(stream, bits, cur_dat_size))
	{
		memFree(bits);
		Write_Debug("Failed to read mip data of texture %s", Filename);
		return -1;
	}
#ifdef TIMING_TEST
	t4 = piReadTimeStamp32();
	Write_Debug ("Cycles spent allocating and reading mip data for tex file %s = %d",
		Filename,
		t4 - t3);
#endif

	bufptr = bits;

	for (i=0; i < tex_data.num_mips; i++)
	{
		cur_dat_size = tex_data.width[i] * tex_data.height[i] * 4;

		if (i==0)
		{

#ifdef TIMING_TEST
			t3 = piReadTimeStamp32();
#endif

			texid = piCreateRawTexture(-1, 0, tex_data.width[0], tex_data.height[0], TexBpp,
				piRGBA_GETALPHA(TransparentColor), WantFlags);

			if (!piIsTexture(texid, 0))
			{
				return -1;
			}

#ifdef TIMING_TEST
			t4 = piReadTimeStamp32();
			Write_Debug ("Cycles spent on first piCreateRaw+malloc for tex file %s = %d",
				Filename,
				t4 - t3);
#endif

		}
		else
		{

#ifdef TIMING_TEST
			t3 = piReadTimeStamp32();
#endif

			texid = piCreateRawTexture(texid, i, tex_data.width[i], tex_data.height[i],
				TexBpp, piRGBA_GETALPHA(TransparentColor), WantFlags);

			if (!piIsTexture(texid, i))
			{
				memFree(bits);
				piFreeMipTextures(texid);
				return(-1);
			}

#ifdef TIMING_TEST
			t4 = piReadTimeStamp32();
			Write_Debug ("Cycles spent on successive piCreateRaw for tex file %s = %d",
				Filename,
				t4 - t3);
#endif

		}

#ifdef TIMING_TEST
		t3 = piReadTimeStamp32();
#endif

		if (!piLockTextureMemorySurface(texid, i, &pisd))
		{
			Write_Debug("Could not get a lock on the text surface");
			memFree(bits);
			piFreeMipTextures(texid);
			return(-1);
		}

#ifdef TIMING_TEST
		t4 = piReadTimeStamp32();

		Write_Debug ("Cycles spent locking mip #%d for tex file %s = %d",
			i,
			Filename,
			t4 - t3);
#endif

		/*
		t3 = piReadTimeStamp32();
		if ((cur_dat_size) != (long)fioRead(stream, bits, cur_dat_size))
		{
			memFree(bits);
			Write_Debug("Failed to read mip level %d of texture %s", i, Filename);
			piFreeMipTextures(texid);
			return -1;
		}
		t4 = piReadTimeStamp32();
		Write_Debug ("Cycles spent reading mip #%d for tex file %s = %d", i, Filename, t4 - t3);
		*/

#ifdef TIMING_TEST
		t3 = piReadTimeStamp32();
#endif

		if (!ldpixCopyAllBitsToSurface(	bufptr,
										tex_data.width[i],
										tex_data.height[i],
										&pisd))
		{
			memFree(bits);
			Write_Debug("Failed to copy mip level %d of texture %s to surface", i, Filename);
			piFreeMipTextures(texid);
			return -1;
		}

#ifdef TIMING_TEST
		t4 = piReadTimeStamp32();
		Write_Debug ("Cycles spent converting mip #%d for tex file %s = %d", i, Filename, t4 - t3);
		t3 = piReadTimeStamp32();
#endif

		piUnlockTextureMemorySurface(texid, i);

#ifdef TIMING_TEST
		t4 = piReadTimeStamp32();
		Write_Debug ("Cycles spent unlocking mip #%d for tex file %s = %d", i, Filename, t4 - t3);
#endif

		bufptr += (tex_data.width[i] * tex_data.height[i] * 4);
	}

	memFree(bits);

	lptex = piGetTextureData(texid);

	if (tex_data.alpha_file)
	{
		lptex->hasalpha = TRUE;
	}

#ifdef TIMING_TEST
	t2 = piReadTimeStamp32();

	Write_Debug ("Total time processing tex file %s = %d", Filename, t4 - t3);
#endif

	return (texid);
}

#endif // TEX_FILE_SUPPORT
