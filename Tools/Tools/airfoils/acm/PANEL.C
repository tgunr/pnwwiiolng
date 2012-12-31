
/*
 *  acm : an aerial combat simulator for X
 *  Copyright (C) 1991-1997  Riley Rainey
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 dated June, 1991.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program;  if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave., Cambridge, MA 02139, USA.
 */

#include "pm.h"

int       doGearLights(craft * c, viewer * u);

#ifdef WIN32

#include <ddraw.h>
#include "..\pc\resource.h"
#define handleDn_width 30

extern    LPDIRECTDRAWSURFACE
          LoadACMBitmap(char *szBitmap, int *width, int *height);
extern void
          PutACMBitmap(LPDIRECTDRAWSURFACE bm, char *szBitmap, int w, int h, int x, int y, int len);

static LPDIRECTDRAWSURFACE gear_h[2];
int       gh_width, gh_height;
static LPDIRECTDRAWSURFACE gear_light[3];
int       gl_width, gl_height;
static LPDIRECTDRAWSURFACE flaps[4];
int       flaps_width, flaps_height;

extern LPDIRECTDRAW GetDirectDrawInterface(int iWndIndex);
extern LPDIRECTDRAWSURFACE GetBackBuffer(int iWndIndex);

#else
#include "bitmap.h"
#endif

static struct {
	long      mask;
	char     *name;
}        *pptr, panelVec[] = {

	{
		SYS_ENGINE1, "OIL PRES"
	},
	{
		SYS_HYD1, "HYD1 PRES"
	},
	{
		SYS_HYD2, "HYD2 PRES"
	},
	{
		SYS_GEN1, "GEN1 FAIL"
	},
	{
		SYS_GEN2, "GEN2 FAIL"
	},
	{
		SYS_FLAPS, "FLAP FAIL"
	},
	{
		SYS_SPEEDBRAKE, "SPBRK FAIL"
	},
	{
		SYS_RADAR, "RADAR FAIL"
	},
	{
		SYS_TEWS, "TEWS FAIL"
	},
	{
		SYS_HUD, " HUD FAIL"
	},
	{
		FLAG_LOWFUEL, " LOW FUEL"
	},
	{
		FLAG_SPEEDBRAKE, "SPD BRAKE"
	},
	{
		FLAG_WHEELBRAKE, "  BRAKES"
	},
	{
		0, (char *) 0
	}
};

#define panelRows	7
#define panelColumns	2
#define panelChars	10
#define lightMargin	((rftw) / 3)
#define panelWMargin	(rftw * 2)
#define panelHMargin	(rfth / 3)

long      lastBits[MAXPLAYERS];
int       lastPos[MAXPLAYERS];
int       lastGPos[MAXPLAYERS][3];

#ifdef WIN32
void
printDDError(char *p, HRESULT code)
{
	printf("Direct Draw error: %s ", p);

	switch (code) {
	case DDERR_DCALREADYCREATED:
		printf("DC already created\n");
		break;
	case DDERR_GENERIC:
		printf("generic error\n");
		break;
	case DDERR_INVALIDOBJECT:
		printf("invalid object\n");
		break;
	case DDERR_INVALIDPARAMS:
		printf("invalid params\n");
		break;
	case DDERR_INVALIDSURFACETYPE:
		printf("invalid surface type\n");
		break;
	case DDERR_SURFACELOST:
		printf("surface lost\n");
		break;
	case DDERR_UNSUPPORTED:
		printf("unsupported\n");
		break;
	case DDERR_WASSTILLDRAWING:
		printf("was still drawing\n");
		break;
	case DDERR_INCOMPATIBLEPRIMARY:
		printf("incompatible primary\n");
		break;
	case DDERR_INVALIDCAPS:
		printf("invalid caps\n");
		break;
	case DDERR_INVALIDPIXELFORMAT:
		printf("invalid pixel format\n");
		break;
	case DDERR_NOALPHAHW:
		printf("no alpha hardware\n");
		break;
	case DDERR_NOCOOPERATIVELEVELSET:
		printf("no flip hardware\n");
		break;
	case DDERR_NODIRECTDRAWHW:
		printf("no direct draw hardware\n");
		break;
	case DDERR_NOEMULATION:
		printf("no emulation\n");
		break;
	case DDERR_NOEXCLUSIVEMODE:
		printf("no exclusive mode\n");
		break;
	case DDERR_NOFLIPHW:
		printf("no flip hardware\n");
		break;
	case DDERR_NOMIPMAPHW:
		printf("no mip map hardware\n");
		break;
	case DDERR_NOZBUFFERHW:
		printf("no z-buffer hardware\n");
		break;
	case DDERR_OUTOFMEMORY:
		printf("out of memory\n");
		break;
	case DDERR_OUTOFVIDEOMEMORY:
		printf("out of video memory\n");
		break;
	case DDERR_PRIMARYSURFACEALREADYEXISTS:
		printf("primary surface already exists\n");
		break;
	case DDERR_UNSUPPORTEDMODE:
		printf("unsupported mode\n");
		break;
	default:
		printf("error code 0x%x, %d\n", code, code);
		break;
	}
}
#endif

void
initPanel(craft * c)
{
	lastPos[c->pIndex] = -1;
	lastGPos[c->pIndex][0] = lastGPos[c->pIndex][1] =
		lastGPos[c->pIndex][2] = -1;
	lastBits[c->pIndex] = SYS_NODAMAGE;
}

void
redrawPanel(craft * c, viewer * u)
{
	lastPos[c->pIndex] = -1;
	lastGPos[c->pIndex][0] = lastGPos[c->pIndex][1] =
		lastGPos[c->pIndex][2] = -1;
	lastBits[c->pIndex] = ~lastBits[c->pIndex];
	u->lastRPM = -1;
	redrawAllItems(&u->drawControl);
}

static int rfth = 0, rftw = 0;

void
resizePanel(craft * c, viewer * u)
{
#ifdef WIN32
	if (rfth == 0) {
		LPDIRECTDRAWSURFACE lpBB = GetBackBuffer(0);
		TEXTMETRIC tm;
		HFONT     hfont, hOldFont;
		HDC       hdc;
		HRESULT   result;

		result = lpBB->lpVtbl->GetDC(lpBB, &hdc);
		if (result != DD_OK) {
			printDDError("resizePanel", result);
			exit(1);
		}

		hfont = GetStockObject(ANSI_FIXED_FONT);
		hOldFont = SelectObject(hdc, hfont);
		GetTextMetrics(hdc, &tm);
		rfth = tm.tmHeight;
		rftw = tm.tmAveCharWidth;
		result = lpBB->lpVtbl->ReleaseDC(lpBB, hdc);
	}
#else

	rftw = u->rfont->max_bounds.width;

#endif

	u->panelx = u->TEWSx - (int) (20 * u->scaleFactor + 0.5) -
		(u->TEWSSize + 1) / 2 -
		panelColumns * (panelChars * rftw + 2 * lightMargin +
						panelWMargin);
	u->panely = u->ry + (int) (5.0 * u->scaleFactor + 0.5);
	redrawPanel(c, u);

}

void
doPanel(craft * c, viewer * u)
{
	int       cellH, cellW;
	int       xi, yi, x, y, i;
	long      changeBits;
	XPoint    topLeft;
	_BOOL      force_redraw;

#ifdef WIN32
	HFONT     hfont, hOldFont;
	HDC       hdc;
	HRESULT   result;
	HBRUSH    hbrush;
	LPDIRECTDRAWSURFACE lpBB = GetBackBuffer(0);

	result = lpBB->lpVtbl->GetDC(lpBB, &hdc);
	if (result != DD_OK) {
		printDDError("doPanel", result);
		exit(1);
	}

	hfont = GetStockObject(ANSI_FIXED_FONT);
	hOldFont = SelectObject(hdc, hfont);
	if (rfth == 0) {
		TEXTMETRIC tm;

		GetTextMetrics(hdc, &tm);
		rfth = tm.tmHeight;
		rftw = tm.tmAveCharWidth;
	}

#else

	rfth = u->rfont->max_bounds.ascent + u->rfont->max_bounds.descent;
	rftw = u->rfont->max_bounds.width;

#endif

	cellH = rfth + 2 * lightMargin + panelHMargin + 1;
	cellW = rftw * panelChars + 2 * lightMargin + panelWMargin;

	changeBits = lastBits[c->pIndex] ^ c->damageBits;
	if (changeBits != 0) {
		redrawItem(&u->annunciatorState);
	}
	force_redraw = isRedrawRequired(&u->annunciatorState);

	for (pptr = &panelVec[0], i = 0; pptr->mask != 0; ++pptr, ++i) {

		if ((changeBits & pptr->mask) != 0 || force_redraw == TRUE) {
			xi = i / panelRows;
			yi = i % panelRows;
			x = u->panelx;
			x += xi * cellW + lightMargin;
			y = u->panely;
			y += yi * cellH + lightMargin;
			if ((c->damageBits & pptr->mask) == 0) {
				topLeft.x = x - lightMargin;
				topLeft.y = y - lightMargin;

#ifdef WIN32
				{
					RECT      rect;

					hbrush = GetStockObject(GRAY_BRUSH);
					rect.left = topLeft.x;
					rect.top = topLeft.y;
					rect.right = rect.left + cellW - panelWMargin;
					rect.bottom = rect.top + cellH - panelHMargin;
					FrameRect(hdc, &rect, hbrush);
					SetTextColor(hdc, RGB(230, 230, 230));
					SetBkColor(hdc, RGB(0, 0, 0));
					TextOut(hdc, x, y, pptr->name, strlen(pptr->name));
				}
#else
				XDrawRectangle(u->dpy, u->win, u->gc,
							   topLeft.x, topLeft.y,
							   cellW - panelWMargin,
							   cellH - panelHMargin);
				XDrawImageString(u->dpy, u->win, u->gauge_gc,
							x, y + rfth, pptr->name, strlen(pptr->name));
#endif
			}
			else {
				topLeft.x = x - lightMargin;
				topLeft.y = y - lightMargin;
#ifdef WIN32
				{
					RECT      rect;

					hbrush = GetStockObject(GRAY_BRUSH);
					rect.left = topLeft.x;
					rect.top = topLeft.y;
					rect.right = rect.left + cellW - panelWMargin;
					rect.bottom = rect.top + cellH - panelHMargin;
					FrameRect(hdc, &rect, hbrush);
					rect.top += lightMargin;
					rect.left += lightMargin;
					rect.right = rect.left + rftw * panelChars;
					rect.bottom = rect.top + rfth + lightMargin;
					hbrush = GetStockObject(BLACK_BRUSH);
					FillRect(hdc, &rect, hbrush);
				}
#else
				XDrawRectangle(u->dpy, u->win, u->gc,
							   topLeft.x, topLeft.y,
							   cellW - panelWMargin,
							   cellH - panelHMargin);
				/*y -= rfth; */
				XClearArea(u->dpy, u->win, x, y,
						   rftw * panelChars,
						   rfth + lightMargin, False);
#endif
			}
		}
	}

	lastBits[c->pIndex] = c->damageBits;

#ifdef WIN32
	result = lpBB->lpVtbl->ReleaseDC(lpBB, hdc);
#endif

	doGearLights(c, u);

}

static int gear_flag[3] =
{FL_GEAR0_UP, FL_GEAR1_UP, FL_GEAR2_UP};

#ifdef WIN32
/*
 *  These tables hold resource id's of the bitmaps used on the control panel
 */
static char *gear_h_id[] =
{
	MAKEINTRESOURCE(IDB_GH_UP),
	MAKEINTRESOURCE(IDB_GH_DOWN)
};
static char *gear_light_id[] =
{
	MAKEINTRESOURCE(IDB_GEAR_UP),
	MAKEINTRESOURCE(IDB_GEAR_TRANSIT),
	MAKEINTRESOURCE(IDB_GEAR_DOWN)
};
static char *flaps_id[] =
{
	MAKEINTRESOURCE(IDB_FLAPS0),
	MAKEINTRESOURCE(IDB_FLAPS1),
	MAKEINTRESOURCE(IDB_FLAPS2),
	MAKEINTRESOURCE(IDB_FLAPS3)
};

#endif

int
doGearLights(craft * c, viewer * u)
{
	int       i, x, y, x1, y1, pos[3], ghpos;
	double    geardown;

	ghpos = (c->flags & FL_GHANDLE_DN) ? 1 : 0;

	x = u->xCenter + (u->radarWidth + 1) / 2 + (int) (20.0 *
												   u->scaleFactor + 0.5);

	y = u->height + (int) ((VISOR_MARGIN + 80.0) * u->scaleFactor + 0.5);

#ifdef WIN32
	/*
	 *  First time through, load bitmaps
	 */
	if (gear_h[0] == 0) {
		gear_h[0] = LoadACMBitmap(gear_h_id[0],
								  &gh_width, &gh_height);
		gear_h[1] = LoadACMBitmap(gear_h_id[1],
								  &gh_width, &gh_height);
		gear_light[0] = LoadACMBitmap(gear_light_id[0],
									  &gl_width, &gl_height);
		gear_light[1] = LoadACMBitmap(gear_light_id[1],
									  &gl_width, &gl_height);
		gear_light[2] = LoadACMBitmap(gear_light_id[2],
									  &gl_width, &gl_height);
		flaps[0] = LoadACMBitmap(flaps_id[0],
								 &flaps_width, &flaps_height);
		flaps[1] = LoadACMBitmap(flaps_id[1],
								 &flaps_width, &flaps_height);
		flaps[0] = LoadACMBitmap(flaps_id[2],
								 &flaps_width, &flaps_height);
		flaps[1] = LoadACMBitmap(flaps_id[3],
								 &flaps_width, &flaps_height);
	}

	PutACMBitmap(gear_h[ghpos], gear_h_id[ghpos], gh_width, gh_height, x, y, 0);
#else

#ifdef notdef
	XSetForeground(u->dpy, u->gc, (u->v->flags & VPMono) ?
				   WhitePixel(u->v->dpy, u->v->screen) :
				   VConstantColor(u->v, whitePixel));

	XSetPlaneMask(u->dpy, u->gc, AllPlanes);
#endif

	if (ghpos != lastPos[c->pIndex]) {
		XCopyPlane(u->dpy, u->handle[ghpos], u->win, u->gauge_gc, 0, 0,
				   handleDn_width, handleDn_height, x, y, 1);
		lastPos[c->pIndex] = ghpos;
	}

#endif

	geardown = pi / 2.0;

	for (i = 0; i < 3; ++i) {
		if (c->flags & FL_GEAR0_UP)
			pos[i] = 0;
		else if (c->curGear[i] != geardown)
			pos[i] = 1;
		else
			pos[i] = 2;

		if (pos[i] != lastGPos[c->pIndex][i]) {
			redrawItem(&u->gearState);
		}
	}

	if (isRedrawRequired(&u->gearState)) {

		x1 = x + handleDn_width + 12 + (int) (8.0 * u->scaleFactor + 0.5);
		y1 = y + 10 + (int) (1.0 * u->scaleFactor + 0.5);
#ifdef WIN32
		PutACMBitmap(gear_light[pos[0]], gear_light_id[pos[0]],
					 gl_width, gl_height, x1, y1, 0);
#else
		XCopyPlane(u->dpy, u->gearLight[pos[0]], u->win, u->gauge_gc,
				   0, 0,
				   gearDown_width, gearDown_height, x1, y1, 1);
#endif
		lastGPos[c->pIndex][0] = pos[0];

		x1 = x + handleDn_width;
		y1 = y + 50 + (int) (5.0 * u->scaleFactor + 0.5);
#ifdef WIN32
		PutACMBitmap(gear_light[pos[1]], gear_light_id[pos[1]],
					 gl_width, gl_height, x1, y1, 0);
#else
		XCopyPlane(u->dpy, u->gearLight[pos[1]], u->win, u->gauge_gc,
				   0, 0,
				   gearDown_width, gearDown_height, x1, y1, 1);
#endif
		lastGPos[c->pIndex][1] = pos[1];

		x1 = x + handleDn_width + 25 + (int) (15.0 * u->scaleFactor + 0.5);
		y1 = y + 50 + (int) (5.0 * u->scaleFactor + 0.5);
#ifdef WIN32
		PutACMBitmap(gear_light[pos[2]], gear_light_id[pos[2]],
					 gl_width, gl_height, x1, y1, 0);
#else
		XCopyPlane(u->dpy, u->gearLight[pos[2]], u->win, u->gauge_gc,
				   0, 0,
				   gearDown_width, gearDown_height, x1, y1, 1);
#endif
		lastGPos[c->pIndex][2] = pos[2];
	}
	return 0;

}
