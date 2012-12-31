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
#include <stdio.h>
#ifdef AFDS
#include <afds.h>
#endif

#define REF_X		-1.3
#define REF_Y		-1.1

/*  There's a bug lurking here, but for now ... */

#ifdef WIN32
#define SCALE_1		5.0
#else
#define SCALE_1		7.0
#endif

void   doFSPage(craft * c, viewer * u);

void
doFlightStatusPage(craft * c, viewer * u)
{

	if (c->radarMode != RM_FSPAGE)
		return;

	doFSPage(c, u);
	return;

}

void
doFSPage(craft * c, viewer * u)
{

	XSegment  seg[2048], m_seg[256];
	char buf[256], buf1[256];
	XRectangle rect[1];
	int       m_i = 0, i = 0, xc, yc, h, x, y;
	int       xscale, yscale, xoffset;
	static ZInfo z;
#ifdef AFDS
	AFDS_info *fd = c->flightDirector;
#endif
	double yy;

	rect[0].x = u->rx;
	rect[0].y = u->ry;
	rect[0].width = u->radarWidth;
	rect[0].height = u->radarWidth;

	z.depth = --u->v->depth;
	z.color = (Color) (u->v->pixel[radarBackgroundColor->cIndex]);
	FillRectangle(u->v->w, u->rx, u->ry,
				  u->radarWidth, u->radarWidth, &z);

	xoffset = (int) ((-15.0 * u->scaleFactor) + 0.5);
	xc = u->rx + (u->radarWidth + 1) / 2;
	yc = u->ry + (u->radarWidth + 1) / 2;


	xscale = (int) (u->v->Scale.x / (SCALE_1 * 4));
	yscale = (int) (u->v->Scale.y / (SCALE_1 * 4));

	h = (int) (11.0 * u->scaleFactor + 0.5);

	yy = 0.5;

	sprintf (buf, "G: %+.2f, %+.2f, %+.2f", c->G.x, c->G.y, c->G.z);
	x = (int) (REF_X * u->v->Scale.x / (SCALE_1 * 4));
	y = (int) ((REF_Y + yy) * u->v->Scale.y / (SCALE_1 * 4));
	VDrawStrokeString(u->v, x + xc, y + yc,
						  buf,
						  strlen(buf), h, &u->z);

	yy += 0.2;

	sprintf (buf, "Sa Se Sr: %+.2f %+.2f %+.2f", c->Sa, c->Se, c->Sr);
	x = (int) (REF_X * u->v->Scale.x / (SCALE_1 * 4));
	y = (int) ((REF_Y + yy) * u->v->Scale.y / (SCALE_1 * 4));
	VDrawStrokeString(u->v, x + xc, y + yc,
						  buf,
						  strlen(buf), h, &u->z);
	yy += 0.2;

	sprintf (buf, "Angles: %+.0f, %+.0f, %+.0f",
		RADtoDEG(c->curRoll), RADtoDEG(c->curPitch), RADtoDEG(c->curHeading));

	x = (int) (REF_X * u->v->Scale.x / (SCALE_1 * 4));
	y = (int) ((REF_Y + yy) * u->v->Scale.y / (SCALE_1 * 4));
	VDrawStrokeString(u->v, x + xc, y + yc,
						  buf,
						  strlen(buf), h, &u->z);
	yy += 0.2;

	sprintf (buf, "Rates: %+.1f, %+.1f, %+.1f ]",
		RADtoDEG(c->p), RADtoDEG(c->q), RADtoDEG(- c->r));

	x = (int) (REF_X * u->v->Scale.x / (SCALE_1 * 4));
	y = (int) ((REF_Y + yy) * u->v->Scale.y / (SCALE_1 * 4));
	VDrawStrokeString(u->v, x + xc, y + yc,
						  buf,
						  strlen(buf), h, &u->z);
	yy += 0.2;

#ifdef AFDS
	sprintf (buf, "psiRef/Lambda: %+.1f, %+.2f",
		fd->psiRef_deg, fd->lambda_deg);

	x = (int) (REF_X * u->v->Scale.x / (SCALE_1 * 4));
	y = (int) ((REF_Y + yy) * u->v->Scale.y / (SCALE_1 * 4));
	VDrawStrokeString(u->v, x + xc, y + yc,
						  buf,
						  strlen(buf), h, &u->z);
	yy += 0.2;

	sprintf (buf, "psiCmd/Coupler:",
		fd->psiCommand_deg, fd->x[AFDS_COUPLER_X1]);

	x = (int) (REF_X * u->v->Scale.x / (SCALE_1 * 4));
	y = (int) ((REF_Y + yy) * u->v->Scale.y / (SCALE_1 * 4));
	VDrawStrokeString(u->v, x + xc, y + yc,
						  buf,
						  strlen(buf), h, &u->z);
	yy += 0.2;

	sprintf (buf, "   %+.1f, %+.2f",
		fd->psiCommand_deg, fd->x[AFDS_COUPLER_X1]);

	x = (int) (REF_X * u->v->Scale.x / (SCALE_1 * 4));
	y = (int) ((REF_Y + yy) * u->v->Scale.y / (SCALE_1 * 4));
	VDrawStrokeString(u->v, x + xc, y + yc,
						  buf,
						  strlen(buf), h, &u->z);
	yy += 0.4;

	AFDSGetStatusString ( c, buf, sizeof(buf));

	x = (int) (REF_X * u->v->Scale.x / (SCALE_1 * 4));
	y = (int) ((REF_Y + yy) * u->v->Scale.y / (SCALE_1 * 4));
	VDrawStrokeString(u->v, x + xc, y + yc,
						  buf,
						  strlen(buf), h, &u->z);
#endif /*AFDS*/

	DISLatitudeToString(buf, c->w.latitude, LLM_DMS);

	yy = 0.0;
	x = (int) (REF_X * u->v->Scale.x / (SCALE_1 * 4));
	y = (int) ((REF_Y + yy) * u->v->Scale.y / (SCALE_1 * 4));
	VDrawStrokeString(u->v, x + xc, y + yc,
						  buf,
						  strlen(buf), h, &u->z);
	yy += 0.2;

	DISLongitudeToString(buf, c->w.longitude, LLM_DMS);
	strcat (buf, "   ");
	sprintf (buf1, "%d m", (int)(c->w.z+0.5));
	strcat (buf, buf1);

	x = (int) (REF_X * u->v->Scale.x / (SCALE_1 * 4));
	y = (int) ((REF_Y + yy) * u->v->Scale.y / (SCALE_1 * 4));
	VDrawStrokeString(u->v, x + xc, y + yc,
						  buf,
						  strlen(buf), h, &u->z);
	yy += 0.2;

	u->v->w->clip.x1 = rect[0].x;
	u->v->w->clip.y1 = rect[0].y;
	u->v->w->clip.x2 = rect[0].x + rect[0].width - 1;
	u->v->w->clip.y2 = rect[0].y + rect[0].height - 1;

	VDrawSegments(u->v, m_seg, m_i, (Color) (u->v->pixel[HSIMagentaColor->cIndex]));
	VDrawSegments(u->v, seg, i, (Color) (u->v->pixel[HUDPixel]));

	u->v->w->clip.x1 = 0;
	u->v->w->clip.y1 = 0;
	u->v->w->clip.x2 = u->v->w->width - 1;
	u->v->w->clip.y2 = u->v->w->height - 1;

	return;
}
