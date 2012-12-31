/*
 *  acm : an aerial combat simulator for X
 *  Copyright (C) 1991-1998  Riley Rainey
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

#include <math.h>
#include "pm.h"
#ifdef WIN32
#define handleDn_width	30
#define flaps0_width	64
#define eng_x_hot		32
#define eng_y_hot		32
#include <windows.h>
#include <ddraw.h>
#include "..\pc\resource.h"

extern void printDDError(char *, HRESULT code);

extern    LPDIRECTDRAWSURFACE
          LoadACMBitmap(char *szBitmap, int *width, int *height);
extern void
          PutACMBitmap(LPDIRECTDRAWSURFACE bm, char *,
					   int w, int h, int x, int y, int len);
extern LPDIRECTDRAW GetDirectDrawInterface(int iWndIndex);
extern LPDIRECTDRAWSURFACE GetBackBuffer(int iWndIndex);

static LPDIRECTDRAWSURFACE rpm_g = 0;
int       rpm_width = 0, rpm_height = 0;
static LPDIRECTDRAWSURFACE flaps_bm[4] =
{0, 0, 0, 0};
int       flaps_width = 0, flaps_height = 0;

#else
#include "bitmap.h"
#endif
#include <stdio.h>

extern char *ItoA(int n, char *s);
extern void doTEWS(craft *, viewer *);
extern void doFlightStatusPage(craft *, viewer *);
extern void doScale(Viewport * vp, scaleControl * s, double value);
extern void doCompassScale(Viewport * vp, scaleControl * s, double value);
extern void doHUD(craft * c, viewer * u), doHSI(craft * c, viewer * u);
extern void doPanel(craft *, viewer *);
extern int doRadar(craft * c, viewer * u);
int       doRPM(craft * c, viewer * u);

void renderCockpitView ( craft *c , viewer *u );

#define lim	((double) 200 * NM)

VPoint    hex[6] =
{
	{1.00000, 0.00000, 0.0},
	{0.50000, 0.86603, 0.0},
	{-0.50000, 0.86603, 0.0},
	{-1.00000, 0.00000, 0.0},
	{-0.50000, -0.86603, 0.0},
	{0.50000, -0.86603, 0.0}
};

VPolygon **poly;
long      polyCount;

static craft *sortList;

void      insertCraft(craft * c, craft * p);

void
freeRendering (void)
{
	int i;

	for (i=0; i<polyCount; ++i) {
		if (poly[i]) {
			/*VDestroyPolygon (poly[i]);*/
		}
	}
	if (poly) {
		free (poly);
	}
}

void
addGround(craft * c, viewer *u,
		  VPolygon ** poly, long *count, VColor * color, double z)
{
	VPoint    hex1[6], tmp;
	double    d;
	int       i, j, k;
	long      cnt = *count;

	/*
	 *  We should really be drawing all these hexagons if any of the viewers
	 *  is in depth-cueued mode, not just the real player.
	 */

	if (u->v->flags & VPDepthCueing) {
		k = _VDefaultWorkContext->depthCueSteps - 1;
		for (i = 0; i < _VDefaultWorkContext->depthCueSteps; ++i, --k) {
			d = (i == 0) ? lim : (_VDefaultWorkContext->visTable[k]);
			for (j = 0; j < 6; ++j) {

				/* coordinates in NED (meters) frame */

				hex1[j].x = hex[j].x * d;
				hex1[j].y = hex[j].y * d;
				hex1[j].z = z;

				/* convert NED (meters) to XYZ (meters) */

				VReverseTransform(&hex1[j], &c->XYZtoNED, &tmp);
				hex1[j] = tmp;
			}
			poly[cnt] = VCreatePolygon(6, hex1, color);
			poly[cnt++]->assignedDepth = k;
		}
	}
	/*
	 *  Otherwise draw just a single "ground" hexagon.
	 */

	else {
		d = FEETtoMETERS(lim);
		for (j = 0; j < 6; ++j) {
			hex1[j].x = hex[j].x * d;
			hex1[j].y = hex[j].y * d;
			hex1[j].z = z;
			VReverseTransform(&hex1[j], &c->XYZtoNED, &tmp);
			hex1[j] = tmp;
		}
		poly[cnt++] = VCreatePolygon(6, hex1, color);
	}
	*count = cnt;
}

void
doViews(void)
{
	viewer *u;

	for (u = vl_head; u != NULL; u=u->vl_next ) {

		renderCockpitView( u->c, u );

	}

}

void
renderCockpitView ( craft *c, viewer *u )
{

	/*
	 *  Build a vector of polygons for all objects in the scene.
	 *
	 *  This vector should be ordered from "most distant" to "closest" so that
	 *  the final display will end up correct.  Rather than generalizing this
	 *  to death, we'll use a few heuristics to get very close to what we need:
	 *
	 *  (0) Build a single polygon to represent the ground.
	 *  (1) Objects on the surface (stbl) are collected first.
	 *  (2) Planes and projectiles (ptbl and mtbl) are first sorted in 
	 *      descending order by their distance from the observer and then 
	 *      polygons are collected.
	 */

	long      polyCount;
	unsigned long curPixel;
	craft *p;
	int j;
	VPoint    tmp, vp, fwd, up;
	static ZInfo z, panelz;
	double    d, local_z;
	short     bgIndex;

	craft     *vc;

	/*
	 *  pay attention: vc will be the viewer craft informataion, c will be
	 *  the watched entity craft information
	 */

	vc = c;
	if ( c->type == CT_DIS_STEALTH ) {

		if ( c->vl->viewer_state == ViewerStatePiggyback ) {
			
			c = c->vl->watchedCraft;
		}

	}

	/*
	 *  Set up the eyespace transformation for this viewpoint
	 */

	if (vc->flags & FL_CHASE_VIEW) {

		vp.x = c->Sg.x - c->prevSg.x;
		vp.y = c->Sg.y - c->prevSg.y;
		vp.z = c->Sg.z - c->prevSg.z;
		d = mag(vp);

		/*
		 *  If our velocity is greater than 2 fps, point the view along 
		 *  the velocity vector (approximately); otherwise the view will 
		 *  be along the direction of aircraft orientation.
		 */

		if (d / deltaT > 2.0) {
			vp.x *= FEETtoMETERS(-75.0) / d;
			vp.y *= FEETtoMETERS(-75.0) / d;
			vp.z *= FEETtoMETERS(-75.0) / d;
			vp.x += c->Sg.x;
			vp.y += c->Sg.y;
			vp.z += c->Sg.z;
			fwd = c->Sg;
		}
		else {
			fwd.x = -75.0;
			fwd.y = fwd.z = 0.0;
			VTransform_(&fwd, &c->trihedral, &tmp);
			VReverseTransform_(&tmp, &c->XYZtoNED, &vp);
			vp.x = c->Sg.x + FEETtoMETERS(vp.x);
			vp.y = c->Sg.y + FEETtoMETERS(vp.y);
			vp.z = c->Sg.z + FEETtoMETERS(vp.z);
			
			if (c->cinfo) {
				VTransform_(&c->cinfo->viewPoint, &c->trihedral, &tmp);
			}
			else {
				VPoint zero = {0, 0, 0};
				tmp = zero;
			}
			VReverseTransform_(&tmp, &c->XYZtoNED, &fwd);
			fwd.x = c->Sg.x + FEETtoMETERS(fwd.x);
			fwd.y = c->Sg.y + FEETtoMETERS(fwd.y);
			fwd.z = c->Sg.z + FEETtoMETERS(fwd.z);
		}
		
		tmp.x = tmp.y = 0.0;
		tmp.z = -1.0;
		VReverseTransform_(&tmp, &c->XYZtoNED, &up);
		up.x += vp.x;
		up.y += vp.y;
		up.z += vp.z;
		
	}
	else {
		if (c->cinfo) {
			VTransform_(&c->cinfo->viewPoint, &(c->trihedral), &tmp);
		}
		else {
			tmp.x = tmp.y = tmp.z = 0.0;
		}
		tmp.x = FEETtoMETERS(tmp.x);
		tmp.y = FEETtoMETERS(tmp.y);
		tmp.z = FEETtoMETERS(tmp.z);
		VReverseTransform(&tmp, &c->XYZtoNED, &vp);
		
		VTransform_(&u->viewDirection, &(c->trihedral), &tmp);
		VReverseTransform_(&tmp, &c->XYZtoNED, &fwd);
		fwd.x += vp.x;
		fwd.y += vp.y;
		fwd.z += vp.z;
		
		VTransform_(&u->viewUp, &(c->trihedral), &tmp);
		VReverseTransform_(&tmp, &c->XYZtoNED, &up);
		up.x += vp.x;
		up.y += vp.y;
		up.z += vp.z;
	}
	
	local_z = localAltitude(&c->Sg, &c->w);
	
	VGetEyeSpace(u->v, vp, fwd, up);
	
	polyCount = 0;
	sortList = (craft *) NULL;

	/*
	 *  With our simple cloud model, we are either above, in, or below the clouds
	 *
	 *  What we display in the outside-cockpit view will depend of where we are.
	 *
	 *  cbase == ctop means no clouds
	 */

	bgIndex = 0;
	if (cbase == ctop) {
		
		addGround(c, u, poly, &polyCount, groundColor, -(local_z - c->w.z));
		for (j = 0; j < polyCount; ++j) {
			VTransformPolygon(poly[j], &u->v->eyeSpace);
		}
		
		for ((j = 0, p = stbl); j < MAXSURFACE; (++j, ++p)) {
			if (p->type != CT_FREE)
				insertCraft(c, p);
		}
		
		for ((j = 0, p = ptbl); j < MAXPLAYERS; (++j, ++p)) {
			if (p->type != CT_FREE &&
				p->type != CT_DIS_STEALTH &&
				(p != c || c->flags & FL_CHASE_VIEW)) {
				insertCraft(c, p);
			}
		}
		
		for ((j = 0, p = mtbl); j < MAXPROJECTILES; (++j, ++p)) {
			if (p->type != CT_FREE) {
				insertCraft(c, p);
			}
		}
	}
	else if (c->w.z > ctop) {
		
		addGround(c, u, poly, &polyCount, cloudColor, -(ctop - c->w.z));
		for (j = 0; j < polyCount; ++j) {
			VTransformPolygon(poly[j], &u->v->eyeSpace);
		}
		
		for ((j = 0, p = ptbl); j < MAXPLAYERS; (++j, ++p)) {
			if (p->type != CT_FREE &&
				p->type != CT_DIS_STEALTH &&
				(p != c || c->flags & FL_CHASE_VIEW) &&
				p->w.z > ctop) {
				insertCraft(c, p);
			}
		}
		
		for ((j = 0, p = mtbl); j < MAXPROJECTILES; (++j, ++p)) {
			if (p->type != CT_FREE && p->w.z > ctop) {
				insertCraft(c, p);
			}
		}
	}
	else if (c->w.z > cbase) {
		bgIndex = cloudColor->cIndex;
	}
	else {
		
		addGround(c, u, poly, &polyCount, groundColor, -(local_z - c->w.z));
		addGround(c, u, poly, &polyCount, cloudColor, -(cbase - c->w.z));
		for (j = 0; j < polyCount; ++j) {
			VTransformPolygon(poly[j], &u->v->eyeSpace);
		}
		
		for ((j = 0, p = stbl); j < MAXSURFACE; (++j, ++p)) {
			if (p->type != CT_FREE)
				insertCraft(c, p);
		}
		
		for ((j = 0, p = ptbl); j < MAXPLAYERS; (++j, ++p)) {
			if (p->type != CT_FREE &&
				p->type != CT_DIS_STEALTH &&
				(p != c || c->flags & FL_CHASE_VIEW) &&
				p->w.z < cbase) {
				insertCraft(c, p);
			}
		}
		
		for ((j = 0, p = mtbl); j < MAXPROJECTILES; (++j, ++p)) {
			if (p->type != CT_FREE && p->w.z < cbase) {
				insertCraft(c, p);
			}
		}
	}
	
	for (p = sortList; p != (craft *) NULL; p = (craft *) p->next)
		placeCraft(u->v, c, u, p, poly, &polyCount);

	/*
     *  Clip all polygons against the view frustrum
	 */

	for (j = 0; j < polyCount; ++j) {
		poly[j] = VClipSidedPolygon(u->v, poly[j],
									u->v->clipPoly);
	}

	/*
	 *  Display this image for each viewer associated with this craft
	 */

	/*
	 *  Fill in the sky color
	 */

	z.depth = MaxDepth;
	z.color = (unsigned short) (u->v->pixel[bgIndex]);
	VFillRectangle (u->v, 0, 0, u->width, u->height, z.color);
	panelz.depth = MaxDepth;
	panelz.color = (unsigned short) (u->v->pixel[blackPixel]);

	/*
	 *  Now the polygons ..
	 */

	curPixel = -1;
	for (j = 0; j < polyCount; ++j) {
		if (poly[j]) {
			VFillPolygon(u->v, u->win, u->gc, poly[j]);
		}
	}

	/*  Draw Head-Up Display and instrument panel */

	doHUD( c, u );
	doBrowsePage( vc, u );

	if ( u->viewer_state == ViewerStateNormal ) {
		doRadar( c, u );
		doHSI( c, u );		/* doRadar must be called first */
		doTEWS( c, u );		/* doRadar must be called first */
		doFlightStatusPage( c, u );
		doRPM( c, u );
	}

	/*  Expose the completed drawing  */

	VExposeBuffer( u->v, u->gc );

	/*
	 *  I moved this after VExpose... to avoid a conflict with the V library.
	 */

	if ( u->viewer_state == ViewerStateNormal ) {
		doPanel( c, u );
	}

	/*
	 *  Release polygons
	 */

	for (j = 0; j < polyCount; ++j) {
		if (poly[j]) {
			VDestroyPolygon(poly[j]);
		}
	}

}

/*
 *  insertCraft :  Insert a craft into sortList in descending order.
 */

void
insertCraft(craft * c, craft * p)
{

	double    dist, offset;
	VPoint    ds;
	craft    *q, *prev;

	/*
	 *  Here's a kludge for you:  to avoid polygon clipping, I'm going to 
	 *  cheat and hack a way to get ground objects to display properly.
	 *  if the Z coordinate of an object is zero (i.e. on ground objects),
	 *  I'll add a huge offset to their distance values to force them to be
	 *  plotted first -- and in roughly their correct drawing order.
	 *
	 *  To support automated world outline maps, stbl[0] is considered to be
	 *  the world map, and will always be plotted in the background.
	 */

	offset = (p->Sg.z == 0.0) ? 1000.0 * NM : 0.0;
	if (p == &stbl[0]) {
		offset += 500.0 * NM;
	}
	ds.x = p->Sg.x - c->Sg.x;
	ds.y = p->Sg.y - c->Sg.y;
	ds.z = p->Sg.z - c->Sg.z;
	p->order = dist = mag(ds) + offset;

	if (sortList != (craft *) NULL) {
		for ((q = sortList, prev = (craft *) NULL); q != (craft *) NULL;) {
			if (q->order < dist) {
				p->next = (struct _craft *) q;
				if (prev == (craft *) NULL)
					sortList = p;
				else
					prev->next = (struct _craft *) p;
				break;
			}
			prev = q;
			q = (craft *) q->next;
		}
		if (q == (craft *) NULL) {
			prev->next = (struct _craft *) p;
			p->next = (struct _craft *) NULL;
		}
	}
	else {
		sortList = p;
		p->next = (struct _craft *) NULL;
	}
}

/*  moved to static global to make WIN32 stuff easier, was local to doRPM */
static int ffth = 0, fftw = 0;

#ifdef WIN32
static char *flaps_id[] =
{
	MAKEINTRESOURCE(IDB_FLAPS0),
	MAKEINTRESOURCE(IDB_FLAPS1),
	MAKEINTRESOURCE(IDB_FLAPS2),
	MAKEINTRESOURCE(IDB_FLAPS3)
};

#endif

int
doRPM(craft * c, viewer * u)
{

	int       x, y, x0, y0, x1, y1, len, i;
	double    percent, percento, p, sp, cp;
	char      s[32], so[32], s1[32];
	int       blt_flaps = 0;

#ifdef WIN32
	int       haveDC = 0;
	HFONT     hfont, hOldFont;
	HDC       hdc;
	HRESULT   result;
	HPEN      hOldPen;
	LPDIRECTDRAWSURFACE lpBB = GetBackBuffer(0);

#endif

	x = u->xCenter + (u->radarWidth + 1) / 2 +
		(int) (20.0 * u->scaleFactor + 0.5);

	y = u->height +
		(int) ((VISOR_MARGIN + 80.0) * u->scaleFactor + 0.5);

	x += handleDn_width + 100 + (int) (10.0 * u->scaleFactor + 0.5);

	len = eng_x_hot - 6;
#ifndef WIN32
	ffth = u->rfont->max_bounds.ascent + u->rfont->max_bounds.descent;
	fftw = u->rfont->max_bounds.width;
#endif

	if (isFunctioning(c, SYS_ENGINE1)) {
		percent = (double) c->rpm * 100.0;
		percento = (double) c->throttle / 32768.0 * 100.0;
	}
	else {
		percent = 0.0;
		percento = 0.0;
	}

	/*
	 *  We only redraw the RPM gauge when we're pretty sure that the gauge moved.
	 *  (forced redraws are triggered in panel.c).
	 */

	sprintf(s, "%3d%s", (int) (percent + 0.5),
			(c->flags & FL_AFTERBURNER) ? "AB" : "  ");
	sprintf(so, "%3d%s", (int) (percento + 0.5),
			(c->flags & FL_AFTERBURNER) ? "AB" : "  ");
	if (strcmp(s, c->lastRPM) != 0 || strcmp(so, c->lastRPMO) != 0) {

#ifdef WIN32
		if (rpm_g == 0) {
			rpm_g = LoadACMBitmap(MAKEINTRESOURCE(IDB_RPM_GAUGE),
								  &rpm_width, &rpm_height);
			flaps_bm[0] = LoadACMBitmap(flaps_id[0],
										&flaps_width, &flaps_height);
			flaps_bm[1] = LoadACMBitmap(flaps_id[1],
										&flaps_width, &flaps_height);
			flaps_bm[2] = LoadACMBitmap(flaps_id[2],
										&flaps_width, &flaps_height);
			flaps_bm[3] = LoadACMBitmap(flaps_id[3],
										&flaps_width, &flaps_height);
		}

		PutACMBitmap(rpm_g, MAKEINTRESOURCE(IDB_RPM_GAUGE), rpm_width, rpm_height,
					 x - eng_x_hot, y - eng_y_hot, 0);
#else
		XCopyPlane(u->dpy, u->eng, u->win, u->gauge_gc,
				   0, 0, eng_width, eng_height,
				   x - eng_x_hot, y - eng_y_hot, 1);
#endif

		if (percent < 60.0) {
			p = (60.0 - percent) / 120.0 * pi;
			sp = sin(p);
			cp = cos(p);
			x0 = (int) (x - (len >> 3) * cp + 0.5);
			y0 = (int) (y + (len >> 3) * sp + 0.5);
			x1 = (int) (x + len * cp + 0.5);
			y1 = (int) (y - len * sp + 0.5);
		}
		else {
			p = (percent - 60.0) / 40.0 * pi;
			sp = sin(p);
			cp = cos(p);
			x0 = (int) (x - (len >> 3) * cp + 0.5);
			y0 = (int) (y - (len >> 3) * sp + 0.5);
			x1 = (int) (x + len * cp + 0.5);
			y1 = (int) (y + len * sp + 0.5);
		}

#ifdef WIN32
		/* Start of DC initialization */

		result = lpBB->lpVtbl->GetDC(lpBB, &hdc);
		if (result != DD_OK) {
			if (result == DDERR_SURFACELOST) {
				lpBB->lpVtbl->Restore(lpBB);
				result = lpBB->lpVtbl->GetDC(lpBB, &hdc);
			}
			if (result != DD_OK) {
				printDDError("doRPM1", result);
				exit(1);
			}
		}

		SetTextColor(hdc, RGB(230, 230, 230));
		SetBkColor(hdc, RGB(0, 0, 0));

		hfont = GetStockObject(ANSI_FIXED_FONT);
		hOldFont = SelectObject(hdc, hfont);
		if (ffth == 0) {
			TEXTMETRIC tm;

			GetTextMetrics(hdc, &tm);
			ffth = tm.tmHeight;
			fftw = tm.tmAveCharWidth;
		}
		haveDC = 1;

		/*  End of DC initialization */

		TextOut(hdc,
				x - eng_x_hot + 3 * fftw,
				y - eng_y_hot + 5,
				s, strlen(s));
		hOldPen = SelectObject(hdc, GetStockObject(WHITE_PEN));
		MoveToEx(hdc, x0, y0, NULL);
		LineTo(hdc, x1, y1);
#else
		XDrawImageString(u->dpy, u->win,
						 u->gauge_gc,
						 x - eng_x_hot + 3 * fftw,
						 y - eng_y_hot + ffth + ffth,
						 so, strlen(so));
		strcpy(c->lastRPM, s);
		strcpy(c->lastRPMO, so);

		XDrawLine(u->dpy, u->win, u->gauge_gc, x0, y0, x1, y1);
#endif

	}

#ifdef WIN32
	if (haveDC == 0) {
		result = lpBB->lpVtbl->GetDC(lpBB, &hdc);
		if (result != DD_OK) {
			printDDError("doRPM2", result);
			exit(1);
		}

		SetTextColor(hdc, RGB(230, 230, 230));
		SetBkColor(hdc, RGB(0, 0, 0));

		hfont = GetStockObject(ANSI_FIXED_FONT);
		hOldFont = SelectObject(hdc, hfont);
		if (ffth == 0) {
			TEXTMETRIC tm;

			GetTextMetrics(hdc, &tm);
			ffth = tm.tmHeight;
			fftw = tm.tmAveCharWidth;
		}
		haveDC = 1;
	}
#endif
	/*
	 *  Print Fuel total and Fuel consumption rate
	 */

	x = x - eng_x_hot;
	y = y + eng_x_hot + ffth + (ffth >> 1);

#ifdef SHOW_LAT_LON
	{
		char      s2[64], s3[64];

		sprintf(s, "%7s lbs Total   %s %s ", ItoA(((int) c->fuel) / 10 * 10, s1),
				DISLatitudeToString(s2, c->w.latitude, LLM_DMS),
				DISLongitudeToString(s3, c->w.longitude, LLM_DMS));
	}
#else
	sprintf(s, "%7s lbs Total", ItoA(((int) c->fuel) / 10 * 10, s1));
#endif

	if (strcmp(s, c->lastTotal) != 0) {
		redrawItem(&u->fuelState);
	}

	if (isRedrawRequired(&u->fuelState)) {
		strcpy(c->lastTotal, s);
#ifdef WIN32
		TextOut(hdc, x, y, s, strlen(s));
#else
		XDrawImageString(u->dpy, u->win, u->gauge_gc,
						 x, y, s, strlen(s));
#endif
	}

	sprintf(s, "%7s lbs/hour",
			ItoA(((int) (fuelUsed(c) * 3600.0 / deltaT)) / 10 * 10, s1));

	if (strcmp(s, c->lastConsump) != 0) {
		redrawItem(&u->consumpState);
	}

	if (isRedrawRequired(&u->consumpState)) {
#ifdef WIN32
		TextOut(hdc, x, y + ffth + 2, s, strlen(s));
#else
		XDrawImageString(u->dpy, u->win, u->gauge_gc,
						 x, y + ffth + 2, s, strlen(s));
#endif
		strcpy(c->lastConsump, s);
	}

	/*
	 *  Display Flap Setting
	 */

	x = u->xCenter - (u->radarWidth + 1) / 2 - flaps0_width -
		(int) (20.0 * u->scaleFactor);
	y = u->TEWSy + u->TEWSSize + (int) (40.0 * u->scaleFactor);

	sprintf(s, "Flaps: %d ", (int) RADtoDEG(c->curFlap));

	if (strcmp(s, c->lastFlap) != 0) {
		redrawItem(&u->flapState);
	}

	if (isRedrawRequired(&u->flapState)) {

		if (c->flapSetting > DEGtoRAD(29.0))
			i = 3;
		else if (c->flapSetting > DEGtoRAD(19.0))
			i = 2;
		else if (c->flapSetting > DEGtoRAD(9.0))
			i = 1;
		else
			i = 0;

		strcpy(c->lastFlap, s);

#ifdef WIN32
		TextOut(hdc, x, y + 15, s, strlen(s));
		blt_flaps = 1;
#else
		XCopyPlane(u->dpy, u->flap[i], u->win, u->gauge_gc,
				   0, 0, flaps0_width, flaps0_height, x, y, 1);
		XDrawImageString(u->dpy, u->win, u->gauge_gc,
					   x + flaps0_x_hot, y + flaps0_y_hot, s, strlen(s));
#endif
	}

#ifdef WIN32
	SelectObject(hdc, hOldFont);
	SelectObject(hdc, hOldPen);
	lpBB->lpVtbl->ReleaseDC(lpBB, hdc);

	if (blt_flaps) {
		PutACMBitmap(flaps_bm[i], flaps_id[i], flaps_width, flaps_height,
					 x, y, 0);
	}

#endif

	return 0;
}

void
setOutsideView(craft * c, viewer *u, int v)
{

	if (v == VIEW_CHASE) {
		c->flags |= FL_CHASE_VIEW;
	}
	else {
		c->flags &= ~FL_CHASE_VIEW;
	}

	switch (v) {
	case VIEW_CHASE:
	case VIEW_FORWARD:
		u->viewDirection.x = 1.0;
		u->viewDirection.y = 0.0;
		u->viewDirection.z = 0.0;
		u->viewUp.x = 0.0;
		u->viewUp.y = 0.0;
		u->viewUp.z = -1.0;
		break;
	case VIEW_UP:
		u->viewDirection.x = 0.0;
		u->viewDirection.y = 0.0;
		u->viewDirection.z = -1.0;
		u->viewUp.x = -1.0;
		u->viewUp.y = 0.0;
		u->viewUp.z = 0.0;
		break;
	case VIEW_LEFT:
		u->viewDirection.x = 0.0;
		u->viewDirection.y = -1.0;
		u->viewDirection.z = 0.0;
		u->viewUp.x = 0.0;
		u->viewUp.y = 0.0;
		u->viewUp.z = -1.0;
		break;
	case VIEW_RIGHT:
		u->viewDirection.x = 0.0;
		u->viewDirection.y = 1.0;
		u->viewDirection.z = 0.0;
		u->viewUp.x = 0.0;
		u->viewUp.y = 0.0;
		u->viewUp.z = -1.0;
		break;
	case VIEW_AFT:
		u->viewDirection.x = -1.0;
		u->viewDirection.y = 0.0;
		u->viewDirection.z = 0.0;
		u->viewUp.x = 0.0;
		u->viewUp.y = 0.0;
		u->viewUp.z = -1.0;
		break;
	}
}
