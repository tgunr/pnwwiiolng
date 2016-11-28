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
#include "ld/ldpcx.h"

static void ReadPCX(int width, int height, UCHAR* inbits, UCHAR* outbits)
{
int		i, j, k, l, cb;

	l = 0;
	// reverse the rows; like a bitmap
	for (i=height-1; i >= 0; i--)
	{
		// reverse colors; like a DIB
		for (j=2; j >= 0; j--)
		{
			for (k=0; k < width; k++)
			{
				if (l == 0)
				{
					l = *inbits++;
					if ((l & 0xc0) == 0xc0)
					{
						l &= 0x3f;
						cb = *inbits++;
					}
					else
					{
						cb = l;
						l = 1;
					}
				}

				outbits[i * width * 3 + k * 3 + j] = cb;
				l -= 1;
			}
		}

		if (k & 1)
		{
			inbits++;
		}
	}
}

int ldpcxLoadPCXFileToSurface(const char* Filename, UINT32 TransparentColor, UINT32 WantFlags)
{
LOGICAL  retval=TRUE;
int   texid;
int   width, height;
size_t  length;
piSURFACEDESC	pisd;
PCXHEADER*		pfh;
void*  bits, *inbits;
int   stream;
    TEXTURE* t = nullptr;

	stream = fioOpen(Filename);
	if (stream == -1)
	{
		return(-1);
	}

	length = fioSeek(stream, 0, SEEK_END);
	fioSeek(stream, 0, SEEK_SET);
	pfh = (PCXHEADER*)memAlloc(length);
	if (pfh == nullptr)
	{
		fioClose(stream);
		return(-1);
	}

	if (length != fioRead(stream, pfh, length))
	{
		fioClose(stream);
		memFree(pfh);
		return(-1);
	}

	fioClose(stream);

	width = pfh->xmax - pfh->xmin + 1;
	height = pfh->ymax - pfh->ymin + 1;

	if ((pfh->bpp * pfh->nplanes) != 24 || pfh->version != 5)
	{
		memFree(pfh);
		return(-1);
	}

	texid = piCreateRawTexture(-1, 0, width, height, TexBpp, piRGBA_GETALPHA(TransparentColor), WantFlags);
	if (!piIsTexture(texid, 0))
	{
		memFree(pfh);
		return(-1);
	}

    t = piGetTextureData(texid);

    if(t)
    {
        t->type = TEXTURE_PCX;
    }

	if (!piLockTextureMemorySurface(texid, 0, &pisd))
	{
		Write_Debug("Could not get a lock on the text surface");
		piFreeTexture(texid, 0);
		return(-1);
	}

	bits = (UCHAR*)pfh + sizeof(PCXHEADER);
	inbits = (void*)memAlloc(width * 3 * height);

	ReadPCX(width, height, (UCHAR*)bits, (UCHAR*)inbits);

	memFree(pfh);

	TransparentColor = piRGBA_TORGB(TransparentColor);
	if (!ldpixCopyBitsToSurface((UCHAR*)inbits, 0, width, height, &pisd, TransparentColor))
	{
		retval = FALSE;
	}

	piUnlockTextureMemorySurface(texid, 0);

	memFree(inbits);

	if (retval == FALSE)
	{
		piFreeTexture(texid, 0);
		return(-1);
	}

	return(texid);
}
