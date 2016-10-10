#include <syscfg.h> // Path is project specific.
#include "pi/pi.h"

#include "draw/drawIcon.h"

#include "draw/draw2d.h"
#include "draw/draw3d.h"
#include "fio/fio.h"
#include "ld/ldpix.h"
#include "mem/mem.h"

/************************************************************************************************
	DrawIcons
*************************************************************************************************/

static void read_uv_file(const char* filename, piUV**, int* numIcons);

/************************************************************************************************
	drawIconLoad
*************************************************************************************************/

drawIconRef drawIconLoad(const char* iconFile)
{
	DrawIcons* ref = (DrawIcons*)memAlloc(sizeof(DrawIcons));
	static char filename[256];

	if(ref)
	{
		strprintf(filename, "%s.bmp", iconFile);
		ref->texid = ldpixLoadImageFileToSurface(filename, colorMAGENTA | colorALPHA100, LDPIX_NONE);

        // ldpixLoadImageFileToSurface increments texture ref count if already loaded.
        //piAddTexRef(ref->texid);

		strprintf(filename, "%s.uv", iconFile);
		read_uv_file(filename, &ref->uv, &ref->numIcons);
	}

	return ref;
}

/************************************************************************************************
	drawIconKill
*************************************************************************************************/

void drawIconKill(drawIconRef ref)
{
	if(ref)
	{
		if (piIsTexture(ref->texid, 0))
			piFreeMipTextures(ref->texid);

		if(ref->uv)
			memFree(ref->uv);

		memFree((void*)ref);
	}
}

/************************************************************************************************
	drawIcon2d
*************************************************************************************************/

void drawIcon2d(drawIconRef ref, int iconID, float x, float y)
{
	if(!ref || !ref->uv || ref->texid < 0) return;
	if(iconID < 0 || iconID >= ref->numIcons) return;

	piSetActiveTexture(ref->texid, 0);
	draw2dByUV(x, y, zdepthLAYER0, &ref->uv[2 * iconID], 16, 16, colorWHITEALPHA);
}

/************************************************************************************************
	drawIcon3d
*************************************************************************************************/

void drawIcon3d(drawIconRef ref, int iconID, piVECTOR verts[4], unsigned int color)
{
	if(!ref || !ref->uv || ref->texid < 0) return;
	if(iconID < 0 || iconID >= ref->numIcons) return;

	draw3dTexture(ref->texid,  &ref->uv[2 * iconID], verts, color);
}

/************************************************************************************************
	drawIconGetUV
*************************************************************************************************/

void drawIconGetUV(drawIconRef ref, int iconID, float* u1, float* v1, float* u2, float* v2)
{
	*u1 = *v1 = *u2 = *v2 = 0;
	if(!ref || !ref->uv || ref->texid < 0) return;
	if(iconID < 0 || iconID >= ref->numIcons) return;

	*u1 = ref->uv[2 * iconID].u;
	*v1 = ref->uv[2 * iconID].v;
	*u2 = ref->uv[2 * iconID+1].u;
	*v2 = ref->uv[2 * iconID+1].v;
}

int drawIconGetTexId(drawIconRef ref)
{
	return ref->texid;
}

/************************************************************************************************
	read_uv_file
*************************************************************************************************/

static void read_uv_file(const char* filename, piUV** uvout, int* numIcons)
{
	int i, capacity;
	piUV* uv;
	int fd;
	static char line[256];

	capacity = 64;
	*numIcons = 0;
	*uvout = uv = (piUV*)memAlloc(2 * capacity * sizeof(piUV));

	fd = fioOpen(filename);
	if(fd < 0) return;

	for(i = 0;; i++)
	{
		if(fioGetLine(fd, line, 255) < 1) break;

		if(i >= capacity)
		{
			piUV* newuv = (piUV*)memAlloc(2 * (capacity + 64) * sizeof(piUV));
			memcpy(newuv, uv, 2 * capacity * sizeof(piUV));
			capacity += 64;
			memFree(uv);
			uv = newuv;
		}

		if(sscanf(line, "%f %f %f %f", &uv[2*i].u, &uv[2*i].v, &uv[2*i+1].u, &uv[2*i+1].v) != 4) break;
	}

	fioClose(fd);

	*uvout = uv;
	*numIcons = i;
}
