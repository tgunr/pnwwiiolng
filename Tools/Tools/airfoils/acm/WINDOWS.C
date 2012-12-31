/*
 *  acm : an aerial combat simulator for X
 *  Copyright (C) 1991,1992,1997  Riley Rainey
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

extern void redrawPanel(craft * c, viewer * u), resizePanel(craft * c, viewer * u);

void
resizePlayerWindow(craft * c, viewer * u, int width, int height, int initial_flag)
{

#ifndef WIN32
	XWindowChanges xwc;

#endif
	double    scale;

	scale = (double) width / (double) FS_WINDOW_WIDTH;

#ifdef notdef
	if (initial_flag == 0) {
		xwc.width = RADAR_WINDOW_WIDTH * scale + 0.5;
		xwc.height = RADAR_WINDOW_HEIGHT * scale + 0.5;
		xwc.x = RADAR_X * scale + 0.5;
		xwc.y = RADAR_Y * scale + 0.5;
		XConfigureWindow(u->dpy, u->rwin,
						 CWX | CWY | CWWidth | CWHeight, &xwc);
	}
#endif

	u->radarWidth = (int) (RADAR_WINDOW_WIDTH * scale + 0.5);
	u->radarHeight = (int) (RADAR_WINDOW_HEIGHT * scale + 0.5);
	u->rx = (int) (RADAR_X * scale + 0.5);
	u->ry = height - u->radarHeight - 1;

	u->width = (int) (VIEW_WINDOW_WIDTH * scale);
	u->height = u->ry;

	u->xCenter = (u->width + 1) / 2;
	u->yCenter = (u->height + 1) / 2;

#ifndef WIN32
	if (initial_flag == 0) {
		XSetPlaneMask(u->dpy, u->gc, AllPlanes);

		if (u->v->flags & VPMono) {
			XSetBackground(u->dpy, u->gc,
						   VConstantColor(u->v, blackPixel));
			XSetForeground(u->dpy, u->gc,
						   VConstantColor(u->v, whitePixel));
		}
		else {
			XSetForeground(u->dpy, u->gc,
						   VConstantColor(u->v, blackPixel));
		}

		XFillRectangle(u->dpy, u->win, u->gc, 0, u->height,
					   u->width, height);
		XFlush(u->dpy);
	}
#endif

#ifdef WIN32
	u->rfth = (int) (11.0 * scale + 0.5);
#else
	u->rfth = (int) (8.0 * scale + 0.5);
#endif
	u->rftw = VFontWidthPixels(u->v, u->rfth);

	u->TEWSSize = (int) (TEWS_SIZE * scale + 0.5);
	u->TEWSx = (int) (TEWS_X * scale + 0.5);
	u->TEWSy = u->ry + (u->TEWSSize >> 1) + (int) (5.0 * scale + 0.5);
	u->scaleFactor = scale;

	ResizeAWindow(u->v->w, height, width);

	if (initial_flag == 0) {
#ifndef WIN32
		XSetPlaneMask(u->dpy, u->gc, u->v->mask);
#endif
		redrawPanel(c, u);
	}

	resizePanel(c, u);

/*
 *  Fill in the scale structures for the airspeed/altitude HUD scales.
 */

	u->altScale.xorg = (int) (ALT_ORG_X * scale);
	u->altScale.length = (int) (ALT_LENGTH * scale);
	u->altScale.yorg = u->yCenter + (u->altScale.length / 2);
	u->altScale.orientation = ALT_ORIENT;
	u->altScale.scale = ALT_SCALE / scale;
	u->altScale.minorInterval = ALT_MIN_INTERVAL;
	u->altScale.minorSize = (int) (ALT_MIN_SIZE * scale);
	u->altScale.majorInterval = ALT_MAJ_INTERVAL;
	u->altScale.majorSize = (int) (ALT_MAJ_SIZE * scale);
	u->altScale.indexSize = (int) (ALT_INDEX_SIZE * scale);
	u->altScale.divisor = ALT_DIVISOR;
	u->altScale.format = ALT_FORMAT;
	u->altScale.pixel = u->v->pixel[HUDPixel];
	u->altScale.fontSize = (int) (14.0 * scale + 0.5);

	u->velScale.xorg = (int) (VEL_ORG_X * scale);
	u->velScale.length = (int) (VEL_LENGTH * scale);
	u->velScale.yorg = u->yCenter + (u->velScale.length / 2);
	u->velScale.orientation = VEL_ORIENT;
	u->velScale.scale = VEL_SCALE / scale;
	u->velScale.minorInterval = VEL_MIN_INTERVAL;
	u->velScale.minorSize = (int) (VEL_MIN_SIZE * scale);
	u->velScale.majorInterval = VEL_MAJ_INTERVAL;
	u->velScale.majorSize = (int) (VEL_MAJ_SIZE * scale);
	u->velScale.indexSize = (int) (VEL_INDEX_SIZE * scale);
	u->velScale.divisor = VEL_DIVISOR;
	u->velScale.format = VEL_FORMAT;
	u->velScale.pixel = u->v->pixel[HUDPixel];
	u->velScale.fontSize = (int) (14.0 * scale + 0.5);

	u->hdgScale.xorg = (int) (HDG_ORG_X * scale);
	u->hdgScale.yorg = u->yCenter + u->velScale.length / 2 /*- 50.0 * scale */ ;
	u->hdgScale.length = (int) (HDG_LENGTH * scale);
	u->hdgScale.orientation = HDG_ORIENT;
	u->hdgScale.scale = HDG_SCALE / scale;
	u->hdgScale.minorInterval = HDG_MIN_INTERVAL;
	u->hdgScale.minorSize = (int) (HDG_MIN_SIZE * scale);
	u->hdgScale.majorInterval = HDG_MAJ_INTERVAL;
	u->hdgScale.majorSize = (int) (HDG_MAJ_SIZE * scale);
	u->hdgScale.indexSize = (int) (HDG_INDEX_SIZE * scale);
	u->hdgScale.divisor = HDG_DIVISOR;
	u->hdgScale.format = HDG_FORMAT;
	u->hdgScale.pixel = u->v->pixel[HUDPixel];
	u->hdgScale.fontSize = (int) (14.0 * scale + 0.5);

}
