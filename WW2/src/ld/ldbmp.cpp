#include <syscfg.h> // Path is project specific.
//	 Copyright 2008 Playnet Inc, All rights reserved
//	 Not to be distributed in API form except as part of a compiled library
//	 under License or NDA

#include <math.h>
#include "pi/pi.h"
#include "include/vars.h"
#include "mem/mem.h"
#include "fio/fio.h"
#include "ld/ldpix.h"
#include "ld/ldbmp.h"

static LOGICAL ispow2(int l)
{
    if(l <= 0) return FALSE;

    return (!(l & (l - 1)));
}
int ldbmpLoadDIBToSurface(piBITMAPINFO* bi, UINT32 TransparentColor, LOGICAL WantFlags)
{
LOGICAL  retval=TRUE;
int   texid, bitcount;
int   width, height;
piSURFACEDESC	pisd;
UCHAR*  bits;
UINT32 bitspitch = 0;

  width = bi->bmiHeader.biWidth;
  height = bi->bmiHeader.biHeight;



	if ((bi->bmiHeader.biBitCount * bi->bmiHeader.biPlanes) != 8 &&
	    (bi->bmiHeader.biBitCount * bi->bmiHeader.biPlanes) != 24)
	{
    Write_Debug("Invalid BMP image format");
    return(-1);
	}

  bits = (UCHAR*)bi + bi->bmiHeader.biSize;
  bitcount = bi->bmiHeader.biBitCount;

  bitspitch = bi->bmiHeader.biSizeImage / height;

  if (bitcount == 8)
  {
  piRGBQUAD* pal;
  void*    newbits;

    pal = (piRGBQUAD*)bits;
    if (bi->bmiHeader.biClrUsed == 0)
    {
      bits += 256 * sizeof(piRGBQUAD);
    }
    else
    {
      bits += bi->bmiHeader.biClrUsed * sizeof(piRGBQUAD);
    }

    newbits = ldpixConvert8To24(bitspitch, height, pal, bits);
    if (newbits == nullptr)
    {
      return(-1);
    }

    bits = (UCHAR*)newbits;
    bitspitch *= 3;
  }

  WantFlags |= CRT_FLAG_WRITABLE;		// We have to set this flag so that no compressed format is used (who would guess this by the name?!!)

  texid = piCreateRawTexture(-1, 0, width, height, TexBpp, piRGBA_GETALPHA(TransparentColor), (SINT32)WantFlags);

  if (!piIsTexture(texid, 0))
  {
    if (bitcount == 8)
    {
      memFree(bits);
    }
    return(-1);
  }

	if (!piLockTextureMemorySurface(texid, 0, &pisd))
  {
    Write_Debug("Could not get a lock on the text surface");
    piFreeTexture(texid, 0);
    if (bitcount == 8)
    {
      memFree(bits);
    }
    return(-1);
  }

	TransparentColor = piRGBA_TORGB(TransparentColor);
 if (!ldpixCopyBitsToSurface(bits, bitspitch, width, height, &pisd, TransparentColor))
  {
    retval = FALSE;
  }

	piUnlockTextureMemorySurface(texid, 0);

  if (bitcount == 8)
  {
    memFree(bits);
  }

  if (retval == FALSE)
  {
    piFreeTexture(texid, 0);
    return(-1);
  }

  return(texid);
}

LOGICAL ldbmpLoadAlphaToSurface(int texid, int miplevel, UCHAR* bits, int width, int height, UINT32 bitspitch)
{
LOGICAL  retval=TRUE;
piSURFACEDESC	pisd;

  if (!piIsTexture(texid, miplevel))
  {
    return(FALSE);
  }

	if (!piLockTextureMemorySurface(texid, miplevel, &pisd))
  {
    Write_Debug("Could not get a lock on the text surface");
    return(FALSE);
  }

  /*if (width != (int)pisd.Width ||
      height != (int)pisd.Height)
  {
    Write_Debug("The alpha file width/height do not match the surface");
    retval = FALSE;
  }
	else*/
  if (!ldpixCopyAlphaToSurface(bits, bitspitch, width, height, &pisd))
  {
    retval = FALSE;
  }

	piUnlockTextureMemorySurface(texid, miplevel);

  return(retval);
}

LOGICAL ldbmpLoadBMPFile(const char* Filename, piBITMAPINFO** bmi)
{
size_t  length;
piBITMAPFILEHEADER  bfh;
piBITMAPINFO*  bi;
int   stream;

	stream = fioOpen(Filename);
	if (stream == -1)
	{
		return(FALSE);
	}

	length = fioSeek(stream, 0, SEEK_END) - sizeof(piBITMAPFILEHEADER);
	fioSeek(stream, 0, SEEK_SET);
	if (sizeof(piBITMAPFILEHEADER) != fioRead(stream, &bfh, sizeof(piBITMAPFILEHEADER)))
	{
		fioClose(stream);
		return(FALSE);
	}

	if (bfh.bfType != (UINT16)('B' | ('M' << 8)))
	{
		fioClose(stream);
		return(FALSE);
	}

	bi = (piBITMAPINFO*)memAlloc(length);
	if (bi == nullptr)
	{
		fioClose(stream);
		return(FALSE);
	}

	if (length != fioRead(stream, bi, length))
	{
		fioClose(stream);
		memFree(bi);
		return(FALSE);
	}

	fioClose(stream);

	// set the biSizeImage ourselves
	if(bi->bmiHeader.biSizeImage == 0)
	{
		int pitch = 0;
		if (bi->bmiHeader.biBitCount == 8)
			pitch = bi->bmiHeader.biWidth;
		else
			pitch = bi->bmiHeader.biWidth*3;

		if(!ispow2(bi->bmiHeader.biWidth)) // eh whatever shouldn't have to do this check, probably some stupid bug that works around another stupid bug
		{
			int pitchMod4 = pitch & 3;
			if(pitchMod4)
				pitch += (4 - pitchMod4);
		}

		bi->bmiHeader.biSizeImage = pitch * bi->bmiHeader.biHeight;
	}

	*bmi = bi;

	return(TRUE);
}

int ldbmpLoadBMPFileToSurface(const char* Filename, UINT32 TransparentColor, UINT32 WantFlags)
{
int   texid;
piBITMAPINFO*  bi;
    TEXTURE* t = nullptr;

	if (!ldbmpLoadBMPFile(Filename, &bi))
	{
		return(-1);
	}

	texid = ldbmpLoadDIBToSurface(bi, TransparentColor, WantFlags);

    t = piGetTextureData(texid);

    if(t)
    {
        t->type = TEXTURE_BMP;
    }

	memFree(bi);

	return(texid);
}

LOGICAL ldbmpLoadBMPAlphaFileToSurface(const char* Filename, int texid, int miplevel)
{
LOGICAL		retval;
UCHAR*		bits;
int			i;
int			width, height;
piBITMAPINFO*	bmi;
piRGBQUAD*		pal;
UCHAR* bitsp;
int size;
UINT32 bitspitch = 0;

	if (!ldbmpLoadBMPFile(Filename, &bmi))
	{
		return(FALSE);
	}

	if (bmi->bmiHeader.biBitCount != 8)
	{
        Write_Debug("[ldbmpLoadBMPAlphaFileToSurface] **ERROR** %s needs 8 bit count.", Filename);
		memFree(bmi);
		return(FALSE);
	}

	bits = (UCHAR*)bmi + bmi->bmiHeader.biSize;
	pal = (piRGBQUAD*)bits;
	if (bmi->bmiHeader.biClrUsed == 0)
	{
		bits += 256 * sizeof(piRGBQUAD);
	}
	else
	{
		bits += bmi->bmiHeader.biClrUsed * sizeof(piRGBQUAD);
	}

	width = bmi->bmiHeader.biWidth;
	height = bmi->bmiHeader.biHeight;

	// make the palette index equal to the palette value
	// (for palettes that are not black to white gray scale)
	bitsp = bits;
	size = bmi->bmiHeader.biSizeImage;
	for(i = 0; i < size; i++, bitsp++)
	{
		*bitsp = pal[*bitsp].rgbBlue;
	}

    bitspitch = bmi->bmiHeader.biSizeImage / height;

	retval = ldbmpLoadAlphaToSurface(texid, miplevel, bits, width, height,bitspitch);

	memFree(bmi);

	if (retval)
	{
#ifdef EVENT_LOG_EXTREME
		Write_Debug("Alpha channel added to texture %d/%d", texid, miplevel);
#endif
	}

	return(retval);
}
