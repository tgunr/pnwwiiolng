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
#include "dis.h"
#include <stdio.h>
#ifdef WIN32
#include <ddraw.h>
extern LPDIRECTDRAW GetDirectDrawInterface(int iWndIndex);
extern LPDIRECTDRAWSURFACE GetBackBuffer(int iWndIndex);

extern void printDDError(char *, HRESULT code);

#endif

#define scanSlope 	2.1445

void      GenerateWorldToRadarMatrix(craft * c, VMatrix * m, double, double);
int       computeRadarEnergy(craft * c, int i, VMatrix * m, double els, double azs,
							 double energy);
int       addRadarInfo(craft * c, int i, int beam, double d);
int       clearRadarInfo(craft * c);

void      plotPrimaryTarget(craft * c, viewer * u, int i, int x, int y),
          plotNormalTarget(viewer * u, int x, int y);
double    radarFrameInterval(craft * c);

#define SetSegment(s, xa, ya, xb, yb) {s.x1=u->rx+(xa); \
s.x2=u->rx+(xb); s.y1=u->ry+(ya); s.y2=u->ry+(yb); }

static char *thirty = "30";
static Point tgt[4];
static _BOOL tgt_valid = FALSE;

int       computeRadar(craft * c);

/*
 *  We'll build an array that contains the amount of radar radiation that each
 *  craft can "see" coming from another player.  This is used to build each players
 *  radar threat display.
 *
 *  rval[i][j] will represent the amount of radar energy that player j sees coming from
 *  player i.
 *
 *  For neatness, the rval array has been moved to the ptbl vector.
 */

char     *
ItoA(int n, char *s)
{

	if (abs(n) > 999) {
		sprintf(s, "%d,%3.3d", n / 1000, abs(n) % 1000);
	}
	else
		sprintf(s, "%d", n);

	return s;
}

int
doRadar(craft * c, viewer * u)
{

	register radarInfo *p;
	int       i, x, y, mono, j = -1, state;
	int       xc, yc;
	double    xs, ys, dmax;
	char     *buf;
	XSegment *seg, seg1[4];
	static ZInfo z;
	int       primary = 0;
	_BOOL      time_up = FALSE;

	mono = u->v->flags & VPMono;

	seg = u->radarImage;

/*
 *  Compute radar emissions
 */

	computeRadar(c);

/*
 *  Not a real radar mode? -- let someone else do it.
 */

	if (c->radarMode == RM_ILS)
		return 0;

/*
 *  Are we ready to give the pilot the next frame ? If not, mark the
 *  radar display as an "Unchanged" area, and return.
 */

	if (curTime >= c->nextRadarTime) {
		redrawItem(&u->radarState);
		time_up = TRUE;
	}

/*
 *  If we fall through to here, then a redraw is required.
 *  There are four possible scenarios:
 *
 *  1) The frame redraw timer has expired.
 *     An entire radar frame, including target information,
 *     must be generated and displayed.  A logical copy
 *     of the image is saved for possible future use.
 *
 *  2) The frame redraw timer has not expired, but some
 *     external event has triggered a frame redraw (e.g.
 *     the window was uncovered after being hidden).
 *     In this case, the frame is redrawn from the
 *     information used to generate the last frame.
 *
 *  3) The frame redraw timer has not expired, but
 *     the user changed radar modes.  In this case, we should
 *     redraw just the legend information (i.e. no targets),
 *     save a copy of what was drawn, and reset the frame
 *     timer.
 *
 *  4) If none of the first three cases are true, then simply
 *     mark the screen region associated with the radar display
 *     as unchanged, and return.
 */

	state = 4;

	if (time_up) {
		state = 1;
	}
	else {
		if (isRedrawRequired(&u->radarState) == TRUE) {
			state = 2;
		}
		if (c->flags & FL_RADAR_MODE_CHANGE) {
			state = 3;
		}
	}

	/*
	 *  Case 4; nothing interesting happens.  Mark the display as unchanged
	 */

	if (state == 4) {
		z.depth = --u->v->depth;
		z.color = UnchangedColor;
		VFillRectangle(u->v, u->rx, u->ry,
		  u->radarWidth, u->radarWidth, z.color);
		return 0;
	}

	c->flags &= ~FL_RADAR_MODE_CHANGE;

	if (state == 2) {

		z.depth = --u->v->depth;
		z.color = u->v->pixel[radarBackgroundColor->cIndex];
		VFillRectangle(u->v, u->rx, u->ry,
		  u->radarWidth, u->radarWidth, z.color);


		VSetClipRect (u->v, 
			      u->rx + 1,
			      u->ry + 1,
			      u->rx + u->radarWidth - 2,
			      u->ry + u->radarHeight - 2);
		
		VDrawSegments(u->v,
					  u->radarImage, u->radarImageCount,
					  u->v->pixel[radarColor->cIndex]);

		VSetClipRect (u->v, 0, 0, -1, -1);

		if (tgt_valid) {
			FillPolygon(u->v->w, tgt, 4, &u->rz);
		}
		return 0;
	}

	tgt_valid = FALSE;

/*
 *  Verify thet the current target is still visible.
 */

	if (c->curRadarTarget != -1) {
		for (i = 0, p = c->rinfo; i < c->rtop; ++i, ++p) {
			if (c->curRadarTarget == p->targetID) {
				p->locked = 1;
				j = i;
				break;
			}
		}
		if (j == -1) {
			c->curRadarTarget = -1;
			dis_radarTargetChanged(c);
		}
	}

/*
 *  Go find the closest radar return ... that becomes our victim.
 */

	if (state == 1) {
		dmax = 1000.0 * NM;

		if (c->curRadarTarget == -1) {
			for (i = 0, p = c->rinfo, j = -1; i < c->rtop; ++i, ++p) {
				if (p->d < dmax) {
					dmax = p->d;
					j = i;
				}
			}
			if (j != -1) {
				c->curRadarTarget = c->rinfo[j].targetID;
				c->rinfo[j].locked = 1;
				dis_radarTargetChanged(c);
				playSound(c, SoundAPGLockAcquired);
			}
		}
	}

	xc = (u->radarWidth + 1) / 2;
	yc = (u->radarWidth + 1) / 2;

/*
 *  Fill background
 */

	z.depth = --u->v->depth;
	z.color = u->v->pixel[radarBackgroundColor->cIndex];
	VFillRectangle(u->v, u->rx, u->ry,
	  u->radarWidth, u->radarWidth, z.color);

/*
 *  Draw reference "thing" and the border
 */

	u->radarImageCount = 0;
	SetSegment(seg[u->radarImageCount], xc, yc, xc - 5, yc + 5);
	u->radarImageCount++;
	SetSegment(seg[u->radarImageCount], xc - 10, yc, xc - 5, yc + 5);
	u->radarImageCount++;
	SetSegment(seg[u->radarImageCount], xc - 10, yc, xc - 18, yc);
	u->radarImageCount++;
	SetSegment(seg[u->radarImageCount], xc, yc, xc + 5, yc + 5);
	u->radarImageCount++;
	SetSegment(seg[u->radarImageCount], xc + 10, yc, xc + 5, yc + 5);
	u->radarImageCount++;
	SetSegment(seg[u->radarImageCount], xc + 10, yc, xc + 18, yc);
	u->radarImageCount++;

/*
 *  Radar dead?
 */

	if (isFunctioning(c, SYS_RADAR) == 0) {
		VDrawSegments(u->v, seg, u->radarImageCount, u->v->pixel[radarColor->cIndex]);
		return 0;
	}

//  20, i
	/*
	 *  Radar off?
	 */

	if (c->radarMode <= RM_STANDBY) {
		if (c->radarMode == RM_OFF)
			buf = "RADAR OFF";
		else
			buf = "RADAR STBY";
		c->curRadarTarget = -1;
		VGetStrokeString(u->v, u->rx + u->rftw * 4,
						 u->ry + u->rfth * 3,
					seg, &u->radarImageCount, buf, strlen(buf), u->rfth);
		return 0;
	}

	c->nextRadarTime = curTime + radarFrameInterval(c);

/*
 *  Show acm and stt HACK
 */

	if (c->radarMode == RM_ACM) {
		buf = "ACM 20x30";
		VGetStrokeString(u->v, u->rx + u->rftw * 4,
						 u->ry + u->rfth * 3,
					seg, &u->radarImageCount, buf, strlen(buf), u->rfth);
	}
	else if (c->radarMode == RM_STT) {
		buf = "STT";
		VGetStrokeString(u->v, u->rx + u->rftw * 4,
						 u->ry + u->rfth * 3,
					seg, &u->radarImageCount, buf, strlen(buf), u->rfth);
	}

/*
 *  Plot reference lines
 */

	i = (u->radarWidth + 3) / 4;
	y = u->rfth + 3;
	SetSegment(seg[u->radarImageCount], xc, y, xc, y + 4);
	u->radarImageCount++;
	y = u->radarWidth - 10 - u->rfth;
	SetSegment(seg[u->radarImageCount], xc, y, xc, y + 4);
	u->radarImageCount++;

	x = xc - i;
	y = u->rfth + 6;
	SetSegment(seg[u->radarImageCount], x, y, x, y + 4);
	u->radarImageCount++;
	y = u->radarWidth - 10 - u->rfth;
	SetSegment(seg[u->radarImageCount], x, y, x, y + 4);
	u->radarImageCount++;
	VGetStrokeString(u->v, x - u->rftw + u->rx, y + 6 + u->rfth + u->ry,
					 seg, &u->radarImageCount, thirty, 2, u->rfth);

	x = xc + i;
	y = u->rfth + 6;
	SetSegment(seg[u->radarImageCount], x, y, x, y + 4);
	u->radarImageCount++;
	y = u->radarWidth - 10 - u->rfth;
	SetSegment(seg[u->radarImageCount], x, y, x, y + 4);
	u->radarImageCount++;
	VGetStrokeString(u->v, x - u->rftw + u->rx, y + 6 + u->rfth + u->ry,
					 seg, &u->radarImageCount, thirty, 2, u->rfth);

	i = (u->radarWidth + 3) / 4;
	x = 2;
	SetSegment(seg[u->radarImageCount], x, yc, x + 4, yc);
	u->radarImageCount++;
	x = u->radarWidth - 6;
	SetSegment(seg[u->radarImageCount], x, yc, x + 4, yc);
	u->radarImageCount++;

	x = 5 + 2 * u->rftw;
	y = yc - i;
	SetSegment(seg[u->radarImageCount], x, y, x + 4, y);
	u->radarImageCount++;
	x = 3;
	VGetStrokeString(u->v, x + u->rx, y + (u->rfth + 1) / 2 + u->ry,
					 seg, &u->radarImageCount, thirty, 2, u->rfth);
	x = u->radarWidth - 6;
	SetSegment(seg[u->radarImageCount], x, y, x + 4, y);
	u->radarImageCount++;

	x = 5 + 2 * u->rftw;
	y = yc + i;
	SetSegment(seg[u->radarImageCount], x, y, x + 4, y);
	u->radarImageCount++;
	x = 3;
	VGetStrokeString(u->v, x + u->rx, y + (u->rfth + 1) / 2 + u->ry,
					 seg, &u->radarImageCount, thirty, 2, u->rfth);
	x = u->radarWidth - 6;
	SetSegment(seg[u->radarImageCount], x, y, x + 4, y);
	u->radarImageCount++;

/*
 *  If the user was simply changing radar modes skip looking
 *  for new targets ..
 */

	if (state != 3) {

/*
 *  Now plot all targets visible to the radar system.
 */


		VSetClipRect (u->v, 
			      u->rx + 1,
			      u->ry + 1,
			      u->rx + u->radarWidth - 2,
			      u->ry + u->radarHeight - 2);
		

/*
 *  Scan the radar information table and plot targets
 */

		for (i = 0, p = c->rinfo; i < c->rtop; ++i, ++p) {
			if (p->rel.x >= 0.0) {
				xs = p->rel.y / (p->rel.x * scanSlope);
				ys = p->rel.z / (p->rel.x * scanSlope);
				if (fabs(xs) <= 1.0 && fabs(ys) <= 1.0) {
					x = (int) (xs * (double) xc + xc);
					y = (int) (ys * (double) yc + yc);
					if (p->locked) {
						plotPrimaryTarget(c, u, p->targetID, x, y);
						primary = 1;
					}
					else {
						plotNormalTarget(u, x, y);
					}
				}
			}
		}
	}

	if (primary == 0) {
		sprintf(c->rightHUD[0], "R_+__");
		sprintf(c->rightHUD[1], "");
		sprintf(c->rightHUD[2], "");
	}

	VDrawSegments(u->v, u->radarImage, u->radarImageCount,
				  u->v->pixel[radarColor->cIndex]);

	VSetClipRect (u->v, 0, 0, -1, -1);
	return 0;
}

/*
 *  Get a new radar target.
 */

int
newRadarTarget(craft * c)
{
	int       i, j = -1;
	radarInfo *p;

	if (c->curRadarTarget == -1) {
		return -1;
	}

/*
 *  Locate the locked target in the list of visible targets.
 */

	for (i = 0, p = c->rinfo; i < c->rtop; ++i, ++p) {
		if (c->curRadarTarget == p->targetID) {
			j = i;
			p->locked = 0;
			break;
		}
	}
	if (j == -1) {
		return -1;
	}
	++j;
	i = (j == c->rtop) ? 0 : j;
	c->rinfo[i].locked = 1;
	return c->rinfo[i].targetID;
}

void
plotNormalTarget(viewer * u, int x, int y)
{

	SetSegment(u->radarImage[u->radarImageCount], x - 2, y - 2, x - 2, y + 2);
	u->radarImageCount++;
	SetSegment(u->radarImage[u->radarImageCount], x - 2, y + 2, x + 2, y + 2);
	u->radarImageCount++;
	SetSegment(u->radarImage[u->radarImageCount], x + 2, y + 2, x + 2, y - 2);
	u->radarImageCount++;
	SetSegment(u->radarImage[u->radarImageCount], x + 2, y - 2, x - 2, y - 2);
	u->radarImageCount++;
}

extern double heading(VPoint * x);

void
plotPrimaryTarget(craft * c, viewer * u, int i, int x, int y)
{

	int       xp, yp;
	char      s[16], lr;
	VPoint    tmp, rel, deltaV;

	double    d, cl, targetAspectAngle;

	xp = u->radarWidth - 9 * u->rftw;
	yp = u->radarWidth - 9 * u->rfth;

	tgt_valid = TRUE;
	tgt[0].x = u->rx + x;
	tgt[0].y = u->ry + y - 4;
	tgt[1].x = u->rx + x + 4;
	tgt[1].y = u->ry + y;
	tgt[2].x = u->rx + x;
	tgt[2].y = u->ry + y + 4;
	tgt[3].x = u->rx + x - 4;
	tgt[3].y = u->ry + y;
	FillPolygon(u->v->w, tgt, 4, &u->rz);

/*
 *  Heading of target
 */

	sprintf(s, "  %3.3d", (int) (RADtoDEG(ptbl[i].curHeading)));
	VGetStrokeString(u->v, xp + u->rx, yp + u->ry,
					 u->radarImage, &u->radarImageCount,
					 s, strlen(s), u->rfth);

/*
 *  Groundspeed of target
 */

	rel.x = ptbl[i].Cg.x;
	rel.y = ptbl[i].Cg.y;
	rel.z = 0.0;
	sprintf(s, "  %3.3d K", (int) (Vmagnitude(&rel) / NM * 3600.0));
	VGetStrokeString(u->v, xp + u->rx, yp + (15 * u->rfth / 10) + u->ry,
					 u->radarImage, &u->radarImageCount,
					 s, strlen(s), u->rfth);

/*
 *  Relative heading to target.
 */

	tmp.x = METERStoFEET(ptbl[i].Sg.x - c->Sg.x);
	tmp.y = METERStoFEET(ptbl[i].Sg.y - c->Sg.y);
	tmp.z = METERStoFEET(ptbl[i].Sg.z - c->Sg.z);
	VTransform_(&tmp, &c->XYZtoNED, &rel);
	targetAspectAngle = ptbl[i].curHeading - heading(&rel);
	if (targetAspectAngle > 0.0) {
		lr = 'R';
	}
	else {
		lr = 'L';
		targetAspectAngle = -targetAspectAngle;
	}

	sprintf(s, "  %3.3d %c", (int) (RADtoDEG(targetAspectAngle) + 0.5), lr);
	VGetStrokeString(u->v, xp + u->rx, yp + (30 * u->rfth / 10) + u->ry,
					 u->radarImage, &u->radarImageCount,
					 s, strlen(s), u->rfth);

/*
 *  Closure rate
 */

	deltaV.x = ptbl[i].Cg.x - c->Cg.x;
	deltaV.y = ptbl[i].Cg.y - c->Cg.y;
	deltaV.z = ptbl[i].Cg.z - c->Cg.z;
	d = mag(rel);
	cl = -(deltaV.x * rel.x + deltaV.y * rel.y + deltaV.z * rel.z) / d;
	c->targetDistance = d;
	c->targetClosure = cl;
	sprintf(s, "%3.3d", (int) (cl / NM * 3600.0));
	VGetStrokeString(u->v, xp + u->rx, yp + (60 * u->rfth / 10) + u->ry,
					 u->radarImage, &u->radarImageCount,
					 s, strlen(s), u->rfth);

/*
 *  Range to target
 */

#ifdef WIN32
#define XA	60
#define XB	225
#else
#define XA	40
#define XB	150
#endif

	xp = XA * u->radarWidth / RADAR_WINDOW_WIDTH;
	yp = u->rfth + 4;
	sprintf(s, "%d", (int) (d / NM));
	VGetStrokeString(u->v, xp + u->rx, yp + u->ry,
					 u->radarImage, &u->radarImageCount,
					 s, strlen(s), u->rfth);

/*
 *  Altitude of target
 */

	xp = XB * u->radarWidth / RADAR_WINDOW_WIDTH;
	yp = u->rfth + 4;
	sprintf(s, "%d", (int) (METERStoFEET(ptbl[i].w.z) / 1000.0));
	VGetStrokeString(u->v, xp + u->rx, yp + u->ry,
					 u->radarImage, &u->radarImageCount,
					 s, strlen(s), u->rfth);

	sprintf(c->rightHUD[0], "R_+__");
	sprintf(c->rightHUD[1], "%05.1f", d / NM);
	sprintf(c->rightHUD[2], " %04.4d", (int) (cl / NM * 3600.0));
}

/*
 *  doTEWS :  update the threat display for player i.
 */

void
doTEWS(craft * c, viewer * u)
{

	register int i, x, y, hostile_found = 0;
	VPoint    rel, tmp;
	double    m, unit;

#ifdef WIN32
	HDC       hdc;
	HRESULT   result;
	HBRUSH    hbrush, hOldBrush;
	HPEN      hOldPen;
	RECT      rect;
	LPDIRECTDRAWSURFACE lpBB = GetBackBuffer(0);

	result = lpBB->lpVtbl->GetDC(lpBB, &hdc);
	if (result != DD_OK) {
		printDDError("doTEWS()", result);
		exit(1);
	}

	SetRect(&rect, 0, 0, u->TEWSSize, u->TEWSSize);
	OffsetRect(&rect, u->TEWSx - u->TEWSSize / 2,
			   u->TEWSy - u->TEWSSize / 2);

	hbrush = GetStockObject(BLACK_BRUSH);
	FillRect(hdc, &rect, hbrush);

	hOldBrush = SelectObject(hdc, hbrush);
	hOldPen = SelectObject(hdc, GetStockObject(WHITE_PEN));
	Ellipse(hdc, rect.left, rect.top, rect.right, rect.bottom);

#else
	XSetForeground(u->dpy, u->gc, u->v->flags & VPMono ?
				   BlackPixel(u->v->dpy, u->v->screen) :
				   VConstantColor(u->v, blackPixel));
	XFillRectangle(u->dpy, u->win, u->gc, u->TEWSx - u->TEWSSize / 2,
				   u->TEWSy - u->TEWSSize / 2, u->TEWSSize, u->TEWSSize);
	XSetForeground(u->dpy, u->gc, u->v->flags & VPMono ?
				   WhitePixel(u->v->dpy, u->v->screen) :
				   VConstantColor(u->v, whitePixel));

	XDrawArc(u->dpy, u->win, u->gc, u->TEWSx - (u->TEWSSize + 1) / 2,
			 u->TEWSy - (u->TEWSSize + 1) / 2, u->TEWSSize, u->TEWSSize,
			 0, 360 * 64);
#endif

	for (i = 0; i < MAXPLAYERS; ++i) {

		if (c->pIndex == i)
			continue;

		if (c->rval[i] > c->cinfo->TEWSThreshold) {
			VTransform(&ptbl[i].Sg, &c->XYZtoNED, &tmp);
			VReverseTransform_(&tmp, &c->trihedral, &rel);
			m = mag(rel);
			rel.x /= m;
			rel.y /= m;
			rel.z /= m;
			unit = sqrt(rel.x * rel.x + rel.y * rel.y);
			if (unit == 0.0) {
				rel.x = 1.0;
				rel.y = 0.0;
			}
			x = u->TEWSx + (int) (rel.y * u->TEWSSize * 0.4 / unit);
			y = u->TEWSy - (int) (rel.x * u->TEWSSize * 0.4 / unit);
			if (c->team == ptbl[i].team) {

#ifdef WIN32
				RECT      rect;

				SetRect(&rect, -2, -2, 3, 3);
				OffsetRect(&rect, x, y);
				FrameRect(hdc, &rect, GetStockObject(WHITE_BRUSH));
#else
				XDrawLine(u->dpy, u->win, u->gc,
						  x - 2, y - 2, x - 2, y + 2);
				XDrawLine(u->dpy, u->win, u->gc,
						  x - 2, y + 2, x + 2, y + 2);
				XDrawLine(u->dpy, u->win, u->gc,
						  x + 2, y + 2, x + 2, y - 2);
				XDrawLine(u->dpy, u->win, u->gc,
						  x + 2, y - 2, x - 2, y - 2);
#endif

			}
			else {

#ifdef WIN32
				RECT      rect;

				SetRect(&rect, -2, -2, 3, 3);
				OffsetRect(&rect, x, y);
				FillRect(hdc, &rect, GetStockObject(WHITE_BRUSH));
#else
				XFillRectangle(u->dpy, u->win, u->gc,
							   x - 3, y - 3, 7, 7);
#endif
				hostile_found = 1;
			}
		}
	}

#ifdef WIN32
	SelectObject(hdc, hOldPen);
	SelectObject(hdc, hOldBrush);
	lpBB->lpVtbl->ReleaseDC(lpBB, hdc);
#endif

	if (c->vl == u) {
		if (hostile_found == 1 && u->flow[SoundLockWarning] == 0) {
			playContinuousSound(c, SoundLockWarning);
		}
		else if (hostile_found == 0 && u->flow[SoundLockWarning] != 0) {
			stopSound(c, SoundLockWarning);
		}
	}
}

void
doDroneRadar(craft * c)
{
	int       i, j = -1;
	radarInfo *p;
	double    dmax = 500.0 * NM;
	VPoint    tmp, rel, deltaV;

/*
 *  Compute radar emissions
 */

	computeRadar(c);

/*
 *  If this is not one of our drones, we're done ...
 */

	if (c->type != CT_DRONE) {
		return;
	}

/*
 *  Go find the closest radar return ... that becomes our victim.
 */

	if (c->curRadarTarget == -1) {
		for (i = 0, p = c->rinfo; i < c->rtop; ++i, ++p) {
			if (p->d < dmax) {
				dmax = p->d;
				j = i;
			}
		}
		c->curRadarTarget = j;
		dis_radarTargetChanged(c);
	}

/*
 *  Is the current radar target still visible to the radar set?
 */

	else {
		for (i = 0, p = c->rinfo; i < c->rtop; ++i, ++p) {
			if (c->curRadarTarget == p->targetID) {
				j = i;
			}
		}
		if (j == -1) {
			c->curRadarTarget = -1;
			dis_radarTargetChanged(c);
		}
	}

/*
 *  Compute tracking parameters
 */

	if ((i = c->curRadarTarget) != -1) {
		tmp.x = METERStoFEET(ptbl[i].Sg.x - c->Sg.x);
		tmp.y = METERStoFEET(ptbl[i].Sg.y - c->Sg.y);
		tmp.z = METERStoFEET(ptbl[i].Sg.z - c->Sg.z);
		VTransform_(&tmp, &c->XYZtoNED, &rel);
		deltaV.x = ptbl[i].Cg.x - c->Cg.x;
		deltaV.y = ptbl[i].Cg.y - c->Cg.y;
		deltaV.z = ptbl[i].Cg.z - c->Cg.z;
		c->targetDistance = mag(rel);
		c->targetClosure = -(deltaV.x * rel.x +
				deltaV.y * rel.y + deltaV.z + rel.z) / c->targetDistance;
	}
}

int
computeRadar(craft * c)
{
	VMatrix   m1;
	register int i, j, k;
	double    el_center, az_center, el_width, az_width, e, el_slope, az_slope;
	craft    *c1;

	clearRadarInfo(c);
	for (i = 0; i < MAXPLAYERS; ++i) {
		ptbl[i].rval[c->pIndex] = 0.0;
	}

	if (c->type != CT_DIS_PLANE && 
		c->type != CT_DIS_STEALTH &&
		(c->radarMode == RM_OFF ||
				 c->radarMode == RM_STANDBY || c->radarMode == RM_ILS)) {
		return 0;
	}

	i = dis_getBeamCount(c);

	for (j = 0; j < i; ++j) {
		dis_getRadarParameters(c, j, &az_center, &az_width,
							   &el_center, &el_width, &e);

/*
 *  Note: the energy value returned by DIS for EM emissions is in units of
 *        dBm.  dBm = 10.0 * log10 (e_watts / 0.001).
 *
 *  Sooo, to convert to watts, we'd do this:
 *
 *  e = 1000.0 * exp10(e / 10.0);
 */

/*
 * "Transmission Test Procedures for DIS Compliance: Distributed  Emission
 *  Regeneration Protocol Family" says that the elevation value is
 *  up-postive, and azimith is left-postive; units in radians (!)
 */

		GenerateWorldToRadarMatrix(c, &m1, el_center, az_center);
		el_slope = atan(el_width / 2.0);
		az_slope = atan(az_width / 2.0);

		for (c1 = ptbl, k = 0; k < MAXPLAYERS; ++k, ++c1) {
			if (c1->type == CT_FREE ||
				c1->type == CT_DIS_STEALTH || 
				c->pIndex == k) {
				continue;
			}
			computeRadarEnergy(c, k, &m1, el_slope, az_slope, e);
		}
	}

	return 0;
}

/*
 *  Compute the radar energy arriving at the target.
 */

int
computeRadarEnergy(craft * c, int i, VMatrix * m, double els, double azs,
				   double energy)
{
	VPoint    r1;
	double    xs, ys, d;

/*
 *  Calculate the coordinates of the target relative to the craft's frame
 *  and the position of the radar search volume.
 */

	r1.x = METERStoFEET(ptbl[i].Sg.x - c->Sg.x);
	r1.y = METERStoFEET(ptbl[i].Sg.y - c->Sg.y);
	r1.z = METERStoFEET(ptbl[i].Sg.z - c->Sg.z);

	VTransform(&r1, m, &c->relPos[i]);

	ptbl[i].rval[c->pIndex] = 0.0;

	if (c->relPos[i].x <= 0.0)
		return 0;

	xs = c->relPos[i].y / c->relPos[i].x;
	ys = c->relPos[i].z / c->relPos[i].x;

/*
 *  if the absolute values of xs and ys are both less than 1.0, then
 *  we are painting this target with radar energy.  Estimate the value of
 *  the energy that the target sees.
 */

	if ((fabs(xs) <= azs) && (fabs(ys) <= els)) {

		d = mag(c->relPos[i]);

		ptbl[i].rval[c->pIndex] = energy / (4.0 * M_PI * d * d);

/*
 *  Can we detect that ?
 */

/*  return_energy = ptbl[i].rval[c->pIndex] / (4.0 * M_PI * d * d); */

/*
 *  Lookdown?
 */

		if (ys > 0.0) {
			d *= 4.0 / 3.0;
		}

		/*
		 *  Reject targets that would not appear on our radar set
		 */

		/*
		 *  Not a DIS platform?
		 */

		if (ptbl[i].cinfo->entityType.kind != DISKindPlatform) {
			return 0;
		}

		/*
		 *  Ground-based entity?  Ignore it.
		 */

		if (ptbl[i].cinfo->entityType.kind == DISKindPlatform &&
			ptbl[i].cinfo->entityType.domain == DISDomainLand) {
			return 0;
		}

/*
 *  For now, compute radar visibility based on fixed ranges.  A better
 *  solution would include factoring estimated target radar cross sections,
 *  antenna size and radar set sensitivity.  Wow. Where would you get that
 *  sort of information?
 */

		if (ptbl[i].flags & FL_GND_CONTACT) {
			return 0;
		}

		if (d >= c->cinfo->radarDRange) {
			return 0;
		}
		else if (d >= c->cinfo->radarTRange) {
			addRadarInfo(c, i, 0, d);
			return 1;
		}
		else {
			addRadarInfo(c, i, 0, d);
			return 2;
		}
	}
	else {
		return 0;
	}
}

/*
 *  Add information about a target that this aircraft's radar can "see".
 */

int
clearRadarInfo(craft * c)
{
	c->rtop = 0;
	return 0;
}

int
addRadarInfo(craft * c, int i, int beam, double d)
{
	if (c->rtop < 16) {
		c->rinfo[c->rtop].rel = c->relPos[i];
		c->rinfo[c->rtop].d = d;
		c->rinfo[c->rtop].targetID = i;
		c->rinfo[c->rtop].beamID = 0;
		c->rinfo[c->rtop].locked = 0;
		++c->rtop;
	}
	return 0;
}

void
GenerateWorldToRadarMatrix(craft * c, VMatrix * m1,
						   double el_center, double az_center)
{
	WorldCoordinates *w = &c->w;
	VMatrix   m, ABCtoNED;

	VIdentMatrix(&m);

	VRotate(&m, ZRotation, -w->longitude);
	VRotate(&m, YRotation, -w->latitude);
	VRotate(&m, YRotation, -DEGtoRAD(90.0));

	VRotate(&m, YRotation, -el_center);
	VRotate(&m, ZRotation, -az_center);
	transpose(&c->trihedral, &ABCtoNED);
	VMatrixMultByRank(&m, &ABCtoNED, m1, 3);

}

double
radarFrameInterval(craft * c)
{
	double    dt = 1.0;

	switch (c->radarMode) {
	case RM_OFF:
	case RM_STANDBY:
		dt = 1.0;
		break;
	case RM_NORMAL:
		dt = 1.0;
		break;
	case RM_STT:
		dt = 1.0;
		break;
	}
	return dt;
}

void
setRadarMode(craft * c, int mode)
{
	int       dis_mode = 0;

	switch (mode) {
	case RM_NORMAL:
		dis_mode = 1;
		break;
	case RM_ACM:
		dis_mode = 2;
		break;
	case RM_STT:
		dis_mode = 3;
		break;
	}
	c->radarMode = mode;
	c->nextRadarTime = curTime + radarFrameInterval(c);
	c->flags |= FL_RADAR_MODE_CHANGE;
	c->curRadarTarget = -1;
	if (disInUse) {
		dis_setRadarMode(c, dis_mode, 1);
	}
}

