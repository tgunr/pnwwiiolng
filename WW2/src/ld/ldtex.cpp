#include "syscfg.h" // Path is project specific.
//	 Copyright 2008 Playnet Inc, All rights reserved
//	 Not to be distributed in API form except as part of a compiled library
//	 under License or NDA

#include "pi/pi.h"
#include "include/vars.h"
#include "ld/ldpix.h"

#include <algorithm>

LOGICAL ldtexOpenWork(int texid, int miplevel, piSURFACEDESC* lppitext)
{
  if (!piGetTextureMemoryDescriptor(texid, miplevel, lppitext))
  {
    return(FALSE);
  }

  if (!piLockTextureMemorySurface(texid, miplevel, lppitext))
  {
    return(FALSE);
  }

  return(TRUE);
}

LOGICAL ldtexCloseWork(int texid, int miplevel)
{
  if (!piUnlockTextureMemorySurface(texid, miplevel))
  {
    return(FALSE);
  }

  if (piGetTextureHandle(texid, miplevel) == -1)
  {
    return(FALSE);
  }

  return(TRUE);
}


LOGICAL ldtexPaintSurfacePixels(piSURFACEDESC* lpdest, int count, UINT32 FillColor[], int x[], int y[])
{
	int     i;
	int     Color;
	char*    cdptr = nullptr;
	short*   sdptr = nullptr;
	long*    ldptr = nullptr;
	int		offset;

	if (lpdest->PixelFormat.dwRGBBitCount <= 8)
		cdptr = (char*)lpdest->Surface;
	else if (lpdest->PixelFormat.dwRGBBitCount <= 16)
		sdptr = (short*)lpdest->Surface;
	else if (lpdest->PixelFormat.dwRGBBitCount <= 32)
		ldptr = (long*)lpdest->Surface;

	for (i=0; i < count; i++)
	{
		Color = piColorToDescColor(lpdest, FillColor[i]);

		offset = (lpdest->Height - y[i]) * lpdest->Width + x[i];

		if (cdptr)
		{
			cdptr[offset] = (char)Color;
		}
		else if (sdptr)
		{
			sdptr[offset] = (short)Color;
		}
		else if (ldptr)
		{
			ldptr[offset] = (long)Color;
		}
	}

	return(TRUE);

}


// note .. this assumes that (x1,y1) and (x2,y2) are in bounds
LOGICAL ldtexFillSurfaceRegion(piSURFACEDESC* lpdest, UINT32 FillColor, int x1, int y1, int x2, int y2)
{
char*    cdptr;
short*   sdptr;
long*    ldptr;
int     i, j;
int     Color;
int		lowX, lowY, highX, highY;


	Color = piColorToDescColor(lpdest, FillColor);

	if (x1<x2)
	{
		lowX = x1;
		highX = x2;
	}
	else
	{
		lowX = x2;
		highX = x1;
	}

	if (y1<y2)
	{
		lowY = y1;
		highY = y2;
	}
	else
	{
		lowY = y2;
		highY = y1;
	}


	if (lpdest->PixelFormat.dwRGBBitCount <= 8)
	{
		cdptr = (char*)lpdest->Surface + lowY * lpdest->Width;
		for (i=lowY; i < highY; i++)
		{
			for (j=lowX; j < highX; j++)
			{
				cdptr[j] = (char)Color;
			}

			cdptr += lpdest->Width;
		}
	}
	else
	if (lpdest->PixelFormat.dwRGBBitCount <= 16)
	{
		sdptr = (short*)lpdest->Surface + lowY * lpdest->Width;
		for (i=lowY; i < highY; i++)
		{
			for (j=lowX; j < highX; j++)
			{
				sdptr[j] = (short)Color;
			}

			sdptr += lpdest->Width;
		}
	}
	else
	if (lpdest->PixelFormat.dwRGBBitCount <= 32)
	{
		ldptr = (long*)lpdest->Surface + lowY * lpdest->Width;
		for (i=lowY; i < highY; i++)
		{
			for (j=lowX; j < highX; j++)
			{
				ldptr[j] = (long)Color;
			}

			ldptr += lpdest->Width;
		}
	}

	return(TRUE);

}


LOGICAL ldtexFillSurface(piSURFACEDESC* lpdest, UINT32 FillColor)
{
int		width, height;

  width = (int)lpdest->Width;
  height = (int)lpdest->Height;

  return (ldtexFillSurfaceRegion (lpdest, FillColor, 0, 0, width, height));
}

// texture ops for the above surface functions
LOGICAL ldtexFillTextureRegion(int texid, int miplevel, UINT32 FillColor, int x1, int y1, int x2, int y2)
{
piSURFACEDESC	pisd;

  if (!ldtexOpenWork(texid, miplevel, &pisd) ||
      !ldtexFillSurfaceRegion(&pisd, FillColor, x1, y1, x2, y2) ||
      !ldtexCloseWork(texid, miplevel))
  {
    return(FALSE);
  }

  return(TRUE);
}

LOGICAL ldtexPaintTexturePixels(int texid, int miplevel, int count, UINT32 FillColor[], int x[], int y[])
{
piSURFACEDESC	pisd;

  if (!ldtexOpenWork(texid, miplevel, &pisd) ||
      !ldtexPaintSurfacePixels(&pisd, count, FillColor, x, y) ||
      !ldtexCloseWork(texid, miplevel))
  {
    return(FALSE);
  }

  return(TRUE);
}

LOGICAL ldtexFillTexture(int texid, int miplevel, UINT32 FillColor)
{
piSURFACEDESC	pisd;

  if (!ldtexOpenWork(texid, miplevel, &pisd) ||
      !ldtexFillSurface(&pisd, FillColor) ||
      !ldtexCloseWork(texid, miplevel))
  {
    return(FALSE);
  }

  return(TRUE);
}

LOGICAL ldtexCopySubsurfaceToSubsurface(int cwidth, int cheight, int srcoff, piSURFACEDESC* lpsrc, int destoff, piSURFACEDESC* lpdest, UINT32 TransparentColor)
{
char*    csptr, *cdptr;
short*   ssptr, *sdptr;
long*    lsptr, *ldptr;
int     i, j, width, height/*, Count=0*/;
int     AlphaMask;
int     SearchColor, Color;

	width = std::min(cwidth, (int)lpdest->Width);
	height = std::min(cheight, (int)lpdest->Height);

	AlphaMask   = lpdest->PixelFormat.dwRGBAlphaBitMask;

	// If the transparent color is all ones, then all bits need to have alpha enabled
	if (TransparentColor == 0xffffffff)
	{
		SearchColor = -1;
	}
	else
	{
		SearchColor = piColorToDescColor(lpdest, TransparentColor);
	}

	if (lpdest->PixelFormat.dwRGBBitCount <= 8)
	{
		csptr = (char*)lpsrc->Surface + srcoff;
		cdptr = (char*)lpdest->Surface + destoff;
		for (i=0; i < height; i++)
		{
			for (j=0; j < width; j++)
			{
				Color = csptr[j];
				if (Color == SearchColor)
				{
					////cdptr[j] = 0;
				}
				else
				{
					cdptr[j] = Color | AlphaMask;
//					Count += 1;
				}
			}

			csptr += lpsrc->Width;
			cdptr += lpdest->Width;
		}
	}
	else
	if (lpdest->PixelFormat.dwRGBBitCount <= 16)
	{
		ssptr = (short*)lpsrc->Surface + srcoff;
		sdptr = (short*)lpdest->Surface + destoff;
		for (i=0; i < height; i++)
		{
			for (j=0; j < width; j++)
			{
				Color = ssptr[j];
				if (Color == SearchColor)
				{
					////sdptr[j] = 0;
				}
				else
				{
					sdptr[j] = Color | AlphaMask;
//					Count += 1;
				}
			}

			ssptr += lpsrc->Width;
			sdptr += lpdest->Width;
		}
	}
	else
	if (lpdest->PixelFormat.dwRGBBitCount <= 32)
	{
		lsptr = (long*)lpsrc->Surface + srcoff;
		ldptr = (long*)lpdest->Surface + destoff;
		for (i=0; i < height; i++)
		{
			for (j=0; j < width; j++)
			{
			Color = lsptr[j];
				if (Color == SearchColor)
				{
					////sdptr[j] = 0;
				}
				else
				{
					ldptr[j] = Color | AlphaMask;
//					Count += 1;
				}
			}

			lsptr += lpsrc->Width;
			ldptr += lpdest->Width;
		}
	}

	/*
	WriteDebug("Updated %d alpha values in the output texture", Count);
	*/
	return(TRUE);
}

//////////////////////////////////////////////////////////////////////
// Function: ldtexCopyTex1toTex2Rotated
// Description:: Copy texture 1 onto texture 2 rotated 90 degrees
//////////////////////////////////////////////////////////////////////
LOGICAL ldtexCopyTex1toTex2Rotated(SINT32 w,
                                   SINT32 h,
                                   SINT32 srcOffset,
                                   piSURFACEDESC* src,
                                   SINT32 destOffset,
                                   piSURFACEDESC* dest,
                                   UINT32 transColor,
								   LOGICAL	flipHoriz)
{
    SCHAR* cs, *cd;
    SINT16* ss, *sd;
    SINT32* ls, *ld;
    SINT32 i, j, width, height/*, Count = 0*/;
    SINT32 AlphaMask;
    SINT32 SearchColor, Color;
	SINT32	horizInc;

    width = std::min(w, (SINT32)dest->Width);
    height = std::min(h, (SINT32)dest->Height);

    AlphaMask   = dest->PixelFormat.dwRGBAlphaBitMask;

    // If the transparent color is all ones, then all bits need to have alpha enabled
    if(transColor == 0xffffffff)
    {
        SearchColor = -1;
    }
    else
    {
        SearchColor = piColorToDescColor(dest, transColor);
    }

    if(dest->PixelFormat.dwRGBBitCount <= 8)
    {
		cs = (SCHAR*)src->Surface + srcOffset;
		cd = (SCHAR*)dest->Surface + destOffset;

		for(i = 0; i < width; i++)
		{
			cs = (SCHAR*)src->Surface + srcOffset;

			if (flipHoriz)
			{
				horizInc = (width - 1) - i;
			}
			else
				horizInc = i;

			for(j = height - 1; j >= 0; j--)
			{
				Color = cs[horizInc];
				cs += src->Width;

				if(Color == SearchColor)
				{
					// Do nothing
				}
				else
				{
					cd[j] = Color | AlphaMask;
//					Count += 1;
				}
			}

			cd += dest->Width;
		}
	}
	else if(dest->PixelFormat.dwRGBBitCount <= 16)
	{
		ss = (SINT16*)src->Surface + srcOffset;
		sd = (SINT16*)dest->Surface + destOffset;

		for(i = 0; i < width; i++)
		{
			ss = (SINT16*)src->Surface + srcOffset;

			if (flipHoriz)
			{
				horizInc = (width - 1) - i;
			}
			else
				horizInc = i;

			for(j = height - 1; j >= 0; j--)
			{
				Color = ss[horizInc];
				ss += src->Width;

				if(Color == SearchColor)
				{
					// Do nothing
				}
				else
				{
					sd[j] = Color | AlphaMask;
//					Count += 1;
				}
			}

			sd += dest->Width;
		}
	}
	else if(dest->PixelFormat.dwRGBBitCount <= 32)
	{
		ls = (SINT32*)src->Surface + srcOffset;
		ld = (SINT32*)dest->Surface + destOffset;

		for(i = 0; i < width; i++)
		{
			ls = (SINT32*)src->Surface + srcOffset;

			if (flipHoriz)
			{
				horizInc = (width - 1) - i;
			}
			else
				horizInc = i;

			for(j = height - 1; j >= 0; j--)
			{
				Color = ls[horizInc];
				ls += src->Width;

				if(Color == SearchColor)
				{
					// Do nothing
				}
				else
				{
					ld[j] = Color | AlphaMask;
//					Count += 1;
				}
			}

			ld += dest->Width;
		}
	}

	return TRUE;
}

//////////////////////////////////////////////////////////////////////
// FUNCTION: ldtexCopyColorTextToSubsurface
// DESCRIPTION: Use this to write color text to a subsurface.
//////////////////////////////////////////////////////////////////////
LOGICAL ldtexCopyColorTextToSubsurface(SINT32 nCharWidth,
									   SINT32 nCharHeight,
									   SINT32 nSourceOffset,
									   piSURFACEDESC* lpSrcDesc,
									   SINT32 nDestinationOffset,
									   piSURFACEDESC* lpDestDesc,
									   UINT32 uTransparentColor,
									   UINT32 uTextColor)
{
	SINT8* pszSource;
	SINT8* pszDestination;
	SINT16* pnShortSrc;
	SINT16* pnShortDest;
	SINT32 i, j, nWidth, nHeight;
	SINT32 nCount = 0;
	SINT32 nAlphaMask = 0x0;
	UINT32 uSearchColor = 0x0;
	UINT32 uColor = 0x0;
	LOGICAL bNoSearchColor = FALSE;
	UINT32 uConvertedTextColor = piColorToDescColor(lpDestDesc, uTextColor);

	nWidth = std::min(nCharWidth, (SINT32)lpDestDesc->Width);
	nHeight = std::min(nCharHeight, (SINT32)lpDestDesc->Height);

	nAlphaMask = lpDestDesc->PixelFormat.dwRGBAlphaBitMask;

	// If the transparent color is all ones, then all bits need to have alpha enabled
	if(uTransparentColor == 0xffffffff)
	{
		bNoSearchColor = TRUE;
	}
	else
	{
		uSearchColor = piColorToDescColor(lpDestDesc, uTransparentColor);
	}

	/////////////////////////////////////////////
	// 8 BIT COLOR
	/////////////////////////////////////////////
	if(lpDestDesc->PixelFormat.dwRGBBitCount <= 8)
	{
		pszSource = (SINT8*)lpSrcDesc->Surface + nSourceOffset;
		pszDestination = (SINT8*)lpDestDesc->Surface + nDestinationOffset;

		for(i = 0; i < nHeight; i++)
		{
			for(j = 0; j < nWidth; j++)
			{
				uColor = pszSource[j];

				if(uColor == uSearchColor && !bNoSearchColor)
				{
					// Do Nothing. Not drawing transparent colors
				}
				else
				{
					pszDestination[j] = uConvertedTextColor | nAlphaMask;
					nCount++;
				}
			}

			pszSource += lpSrcDesc->Width;
			pszDestination += lpDestDesc->Width;
		}
	}
	/////////////////////////////////////////////
	// 8 - 16 BIT COLOR
	/////////////////////////////////////////////
	else if(lpDestDesc->PixelFormat.dwRGBBitCount <= 16)
	{
		pnShortSrc = (SINT16*)lpSrcDesc->Surface + nSourceOffset;
		pnShortDest = (SINT16*)lpDestDesc->Surface + nDestinationOffset;

		for(i = 0; i < nHeight; i++)
		{
			for(j = 0; j < nWidth; j++)
			{
				uColor = pnShortSrc[j];

				if(uColor == uSearchColor && !bNoSearchColor)
				{
					// Do Nothing. Not drawing transparent colors
				}
				else
				{
					pnShortDest[j] = uConvertedTextColor | nAlphaMask;
					nCount++;
				}
			}

			pnShortSrc += lpSrcDesc->Width;
			pnShortDest += lpDestDesc->Width;
		}
	}
	/////////////////////////////////////////////
	// 16 - 32 BIT COLOR
	/////////////////////////////////////////////
/////////////////////////////////////////////////////////
// FIXME: [DCF]
// If the user selects 32 bit depth, the font textures
// are still created as 16 bits. This requires a HACK! to
// provide the correct offsets from source to destination.
// Need to figure out why the fonts are still being created
// as 16 bits.
//////////////////////////////////////////////////////////
#if platOS == OS_MACOS // works ok on the mac
	else if(lpDestDesc->PixelFormat.dwRGBBitCount <= 32)
	{
		int row, col, bpp;
		UCHAR* srcP, *dstP;
		UCHAR* srcAddr, *dstAddr;

		bpp = lpDestDesc->PixelFormat.BitCount / 8;
		srcAddr = (UCHAR*)lpSrcDesc->Surface  + nSourceOffset * 4;
        dstAddr = (UCHAR*)lpDestDesc->Surface + nDestinationOffset * bpp;

		for(row = 0; row < nHeight; row++)
		{
			srcP = srcAddr + row * lpSrcDesc->Pitch;
			dstP = dstAddr + row * lpDestDesc->Pitch;

			for(col = 0; col < nWidth; col++, srcP += 4, dstP += bpp)
			{
				uColor = *(UINT32*)srcP;

				if(bNoSearchColor || uColor != uSearchColor)
				{
					uColor = uTextColor | nAlphaMask;
					*(dstP) = piRGBA_GETRED  (uColor);
					*(dstP+1) = piRGBA_GETGREEN(uColor);
					*(dstP+2) = piRGBA_GETBLUE (uColor);
					 if(bpp == 4) *(dstP+3) = piRGBA_GETALPHA(uColor); // sometimes destination is only 24 bits
				}
			}
		}
	}
#else
	else if(lpDestDesc->PixelFormat.dwRGBBitCount <= 32)
	{
        SINT32* lpDestination;
		//////////////////////////////////////////////////////////
		//lpSource = (SINT32*)lpSrcDesc->Surface + nSourceOffset;
		//////////////////////////////////////////////////////////
		pnShortSrc = (SINT16*)lpSrcDesc->Surface + nSourceOffset;
      	lpDestination = (SINT32*)lpDestDesc->Surface + nDestinationOffset;

		for(i = 0; i < nHeight; i++)
		{
			for(j = 0; j < nWidth; j++)
			{
				uColor = pnShortSrc[j];
                ///////////////////////
				//uColor = lpSource[j];
                ///////////////////////
				if(uColor == uSearchColor && !bNoSearchColor)
				{
					// Do Nothing. Not drawing transparent colors
				}
				else
				{
					lpDestination[j] = uConvertedTextColor | nAlphaMask;
					nCount++;
				}
			}

            //////////////////////////////
		    //lpSource += lpSrcDesc->Width;
            //////////////////////////////
			pnShortSrc += lpSrcDesc->Width;
			lpDestination += lpDestDesc->Width;
		}
	}
#endif // platOS == OS_MACOS

	return TRUE;
}

LOGICAL ldtexCopyBitmapToSubsurface(piBITMAPINFO* pbmi, int texid, int miplevel)
{
LOGICAL					retval=TRUE;
UCHAR*          pbits;
int           offset, w, h, r, c;
piRGBQUAD*       prgb;
piSURFACEDESC	pisd;

  pbits = (UCHAR*)pbmi + pbmi->bmiHeader.biSize;
  if (pbmi->bmiHeader.biBitCount == 8)
  {
    offset = pbmi->bmiHeader.biClrUsed * sizeof(piRGBQUAD);
    if (offset == 0)
    {
      offset = 256 * sizeof(piRGBQUAD);
    }

    prgb = pbmi->bmiColors;
    pbits = (UCHAR*)prgb + offset;
  }
  else
  {
    prgb = nullptr;
  }

  w = pbmi->bmiHeader.biWidth;
  h = pbmi->bmiHeader.biHeight;

	if (!piLockTextureMemorySurface(texid, miplevel, &pisd))
  {
    return(FALSE);
  }

  r = (int)(pisd.Height - h) / 2;
  if (r < 0)
  {
    r = 0;
  }

  c = (int)(pisd.Width - w) / 2;
  if (c < 0)
  {
    c = 0;
  }

	if (!ldpixCopyBitsToSubsurface(pbits, prgb, w, h, r, c, &pisd, colorWHITEALPHA))
  {
    retval = FALSE;
  }

	piUnlockTextureMemorySurface(texid, miplevel);

	return(retval);
}
