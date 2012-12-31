
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
#include "horizon.h"
#include <stdio.h>

extern void doScale(Viewport * vp, scaleControl * s, double value), doCompassScale(Viewport * vp, scaleControl * s, double value);
extern void ShortTransform(short_point_t *, short_matrix_t * m, short_point_t * b);
void      doLadder(craft * c, viewer * u, double vel);
extern void VMatrixToShortMatrix(VMatrix * v, short_matrix_t * s);
extern double magHeading(craft *);
extern int doWeaponDisplay(craft * c, viewer * u, int *x, int *y);

void
doHUD(craft * c, viewer * u)
{

	char      buffer[80];
	int       i, x, x1, y, scale, fth, scale1;
	double    vel;

	scale1 = (int) (u->scaleFactor * 2048.0);

	scale = (int) (12.0 * u->scaleFactor + 0.5);
	fth = (int) (18.0 * u->scaleFactor + 0.5);

	if (u->viewDirection.x < 0.90)
		return;

#ifdef notdef
	vel = mag(c->Cg);
	sprintf(buffer, "a=%.1f  b=%.1f", RADtoDEG(c->alpha),
			RADtoDEG(c->beta));
#else
	if ((vel = mag(c->Cg)) < 50.0)
		sprintf(buffer, "a=0.0");
	else
		sprintf(buffer, "a=%.1f", RADtoDEG(c->alpha));
#endif

	/* Angle of Attack */
	x = 750 * scale1 / 2048;
	y = u->yCenter - u->velScale.length / 2 - (int) (u->scaleFactor * 50);
	VDrawStrokeString(u->v, x, y, buffer, strlen(buffer),
					  scale, &u->z);

	/* Accelerometer */
	x1 = 375 * scale1 / 2048;
	sprintf(buffer, "%4.1f", - c->G.z);
	VDrawStrokeString(u->v, x1, y, buffer, strlen(buffer),
					  scale, &u->z);

	/*  vertical velocity */
	x = 725 * scale1 / 2048;
	y = u->yCenter + u->velScale.length / 2 + (int) (u->scaleFactor * 14.0);
	sprintf(buffer, "%6d0", (int) (-c->Cg.z * 6.0));
	VDrawStrokeString(u->v, x, y, buffer, strlen(buffer),
					  scale, &u->z);

	/*  Compass, altititude and airspeed cards */
	doCompassScale(u->v, &(u->hdgScale), RADtoDEG(magHeading(c)) * 100.0);
	doScale(u->v, &(u->altScale), METERStoFEET(c->w.z));
	doScale(u->v, &(u->velScale), FPStoKTS(mag(c->Cg)));

	/* Mach number */
	if (c->mach >= 0.20) {
		sprintf(c->leftHUD[1], "   %4.2f", c->mach);
	}
	else {
		strcpy(c->leftHUD[1], "");
	}

	x = 395 * scale1 / 2048;
	y = u->yCenter - 3;
	VDrawStrokeString(u->v, x, y, "C", 1,
					  scale * 9 / 10, &u->z);

	x = 340 * scale1 / 2048;
	y = u->yCenter + u->velScale.length / 2 + (int) (u->scaleFactor * 50);

	doLadder(c, u, vel);

	for (i = 0; i < 5; ++i) {
		VDrawStrokeString(u->v, x, y, c->leftHUD[i],
						  strlen(c->leftHUD[i]), scale, &u->z);
		y += fth;
	}

	x = 765 * scale1 / 2048;
	y = u->yCenter + u->velScale.length / 2 + (int) (u->scaleFactor * 50);

	for (i = 0; i < 5; ++i) {
		VDrawStrokeString(u->v, x, y, c->rightHUD[i],
						  strlen(c->rightHUD[i]), scale, &u->z);
		y += fth;
	}

	return;

}

void
doLadder(craft * c, viewer * u, double vel)
{

	int       a1, a2, a3, a4, a5, plotFPM, npath;
	XSegment  seg[400];
	XRectangle rect[1];
	int       i, j, k, kmax, windX, windY, w, tx, ty, scale, x, y;
	VPoint    tmp, tmp1, t1;
	VMatrix   m;
	double    v, d;
	short_matrix_t im;
	short_point_t it1, it2;
	short_fixed_t limit;
	register short xscale, yscale;
	extern void buildEulerMatrix(double roll, double pitch, double heading, VMatrix * m);

	limit = (short) (0.92 * UNITY);

	scale = (int) (12.0 * u->scaleFactor + 0.5);

	rect[0].x = u->xCenter - (int) (137.5 * u->scaleFactor + 0.5);
	rect[0].y = u->yCenter - (int) (116.0 * u->scaleFactor + 0.5);
	rect[0].width = (unsigned short) (275.0 * u->scaleFactor + 0.5);
	rect[0].height = (unsigned short) (252.0 * u->scaleFactor + 0.5);


	npath = sizeof(horizon_path) / sizeof(horizon_path[0]);

/*
 *  Build a transformation matrix to be used to display the flight
 *  path ladder (artificial horizon).
 *
 *  One thing we do is to keep the ladder centered on the flight path
 *  marker.  The correction angle "v" is calculated to make that happen.
 */

	if (fabs(c->Cg.x) < 5.0 && fabs(c->Cg.y) < 5.0) {
		v = 0.0;
	}
	else {
		t1.x = cos(c->curHeading) * c->Cg.x +
			sin(c->curHeading) * c->Cg.y;
		t1.y = -sin(c->curHeading) * c->Cg.x +
			cos(c->curHeading) * c->Cg.y;
		v = atan2(t1.y, t1.x);
	}

	VIdentMatrix(&m);
	VRotate(&m, ZRotation, v);
	VRotate(&m, YRotation, -c->curPitch);
	VRotate(&m, XRotation, c->curRoll);
	VMatrixToShortMatrix(&m, &im);

	xscale = (short) u->v->Scale.x >> 2;
	yscale = (short) u->v->Scale.y >> 2;

/*
 *  Our horizon structure is a list of point paths on a unit circle.  Traverse
 *  that structure, transforming each set of points into screen coordinates,
 *  and add a segment entry for each one that'll be displayed.
 */

	for (i = 0, j = 0; j < npath; ++j) {
		k = horizon_path[j].vertex_start;
		ShortTransform((short_point_t *) &horizon_vertex[k],
			&im, &it1);
		if (it1.x > limit) {
			kmax = k + horizon_path[j].vertex_count;
			for (++k; k < kmax; ++k) {
				ShortTransform((short_point_t *) &horizon_vertex[k],
					&im, &it2);
				if (it2.x > limit) {
					seg[i].x1 = u->xCenter + (xscale * it1.y / it1.x);
					seg[i].y1 = u->yCenter - (yscale * it1.z / it1.x);
					seg[i].x2 = u->xCenter + (xscale * it2.y / it2.x);
					seg[i++].y2 = u->yCenter - (yscale * it2.z / it2.x);
				}
				it1 = it2;
			}
		}
	}

	/*u->v->w->clip.x1 = rect[0].x;
	u->v->w->clip.y1 = rect[0].y;
	u->v->w->clip.x2 = rect[0].x + rect[0].width - 1;
	u->v->w->clip.y2 = rect[0].y + rect[0].height - 1;*/

	VSetClipRect (u->v, rect[0].x, rect[0].y, rect[0].x + rect[0].width - 1,
		       rect[0].y + rect[0].height - 1);
		      

/*
 *  Determine the location of the flight path marker
 */

	VReverseTransform_(&c->Cg, &c->trihedral, &tmp);

	plotFPM = 1;

	if (vel < 50.0 || tmp.x == 0.0) {
		windX = u->xCenter;
		windY = u->yCenter;
	}
	else if (tmp.x > 0.0) {
		windX = u->xCenter + ((int) (tmp.y * u->v->Scale.x / tmp.x) / 4);
		windY = u->yCenter + ((int) (tmp.z * u->v->Scale.y / tmp.x) / 4);
	}
	else
		plotFPM = 0;

	if (plotFPM) {

		a1 = (int) (18.0 * u->scaleFactor + 0.5);
		a2 = (int) (9.0 * u->scaleFactor + 0.5);
		a3 = (int) (15.0 * u->scaleFactor + 0.5);

		seg[i].x1 = windX - a1;
		seg[i].y1 = windY;
		seg[i].x2 = windX - a2;
		seg[i++].y2 = windY;

		seg[i].x1 = windX + a1;
		seg[i].y1 = windY;
		seg[i].x2 = windX + a2;
		seg[i++].y2 = windY;

		seg[i].x1 = windX;
		seg[i].y1 = windY - a2;
		seg[i].x2 = windX;
		seg[i++].y2 = windY - a1;
	}

/*
 *  Gather weapon display info (and maybe draw a reticle).
 */

	x = y = -1;
	doWeaponDisplay(c, u, &x, &y);

/*
 *  Draw a target designator around the current primary radar target.
 */

	if (c->curRadarTarget >= 0) {

		w = (int) (DESIGNATOR_SIZE * u->scaleFactor);

		VTransform(&ptbl[c->curRadarTarget].Sg, &c->XYZtoNED, &tmp1);
		VReverseTransform_(&tmp1, &c->trihedral, &tmp);

/* radar target is assumed to be ahead of us (tmp.z > 0.0) */

		tx = (u->v->Middl.x + ((int) (tmp.y * u->v->Scale.x / tmp.x)) >> 2);
		ty = (u->v->Middl.y + ((int) (tmp.z * u->v->Scale.y / tmp.x)) >> 2);

		d = sqrt((double) ((x - tx) * (x - tx) + (y - ty) * (y - ty)));

/*
 *  If no LCOS reticle is was plotted or if the distance to the LCOS reticle is
 *  sufficient, then plot a radar target designator box.
 */

		if (x == -1 || d > w * 2.5) {

			seg[i].x1 = tx - w;
			seg[i].y1 = ty - w;
			seg[i].x2 = tx + w;
			seg[i++].y2 = ty - w;

			seg[i].x1 = tx + w;
			seg[i].y1 = ty - w;
			seg[i].x2 = tx + w;
			seg[i++].y2 = ty + w;

			seg[i].x1 = tx + w;
			seg[i].y1 = ty + w;
			seg[i].x2 = tx - w;
			seg[i++].y2 = ty + w;

			seg[i].x1 = tx - w;
			seg[i].y1 = ty + w;
			seg[i].x2 = tx - w;
			seg[i++].y2 = ty - w;
		}
	}

	VDrawSegments(u->v, seg, i, (unsigned short) (u->v->pixel[HUDPixel]));

	VDrawArc(u->v, u->xCenter - 2, u->yCenter - 2, 4, 4, 0, 360 * 64,
			 (unsigned short) (u->v->pixel[HUDPixel]));

	if (plotFPM) {
		a4 = (int) (16.0 * u->scaleFactor + 0.5);
		a5 = (a4 + 1) / 2;
		VDrawArc(u->v, windX - a5, windY - a5, a4, a4, 0, 360 * 64,
				 (unsigned short) (u->v->pixel[HUDPixel]));
	}

	/* u->v->w->clip.x1 = 0;
	u->v->w->clip.y1 = 0;
	u->v->w->clip.x2 = u->v->w->width - 1;
	u->v->w->clip.y2 = u->v->w->height - 1; */

	VSetClipRect (u->v, 0, 0, -1, -1);

	return;
}
