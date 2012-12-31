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
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#define MAX_MAPPED_STRING_LEN	20
#define MAX_POPUP_STRING_LEN	40
#include <stdio.h>
#include "dis.h"

#define REF_X		-1.3
#define REF_Y		-1.1

extern int debug;

/*  There's a bug lurking here, but for now ... */

#ifdef WIN32
#define SCALE_1		5.0
#else
#define SCALE_1		7.0
#endif

struct _dis_browse {
	char  info[32];
	Entity_t  *p;
	craftType *cinfo;   /* craftType iff this an aircraft we can grab */
};

#define BROWSE_MAX 256
#define ITEM_LIMIT 5
#define LINE_SPACING 0.2

static struct _dis_browse browse_info[BROWSE_MAX];
static int bcount;

int controlRequestCallback( dis_pdu *p, void *pu );

/* from drone.c */
extern endGameDistanceCheck ( char *, char * );

/*
 *  Generate the stealth browsing table from the current DIS
 *  entity database
 */

void
buildBrowseInfoTable ()
{
	Entity_t * e = dis_getEntityTable(), *ep;
	long etop = dis_getEntityTop();
	int i=0;
	craftType * cinfo;
	char *marker;

	ep = e;
	bcount = 0;

	/*
	 *  Update the list of entities we might be interested in following
	 */

	for (i = 0; i <= etop && bcount < BROWSE_MAX; ++i, ++ep) {
		if (ep->local != -1 && 
			ep->entityType.kind == DISKindPlatform &&
			ep->entityType.domain == DISDomainAir) {
			browse_info[bcount].p = ep;

			/*
			 *  Was this an aircraft type defined in the inventory file?
			 *  If so, mark it as "flyable".
			 */

			cinfo = lookupCraftByEntityType( &ep->entityType );
			if (cinfo && cinfo->CLift) {
				browse_info[bcount].cinfo = cinfo;
				marker = "* ";
			}
			else {
				browse_info[bcount].cinfo = NULL;
				marker = "  ";
			}

			sprintf (browse_info[bcount].info, "%s%d,%d,%d",
					 marker,
					 ep->entityId.sim_id.site_id, 
					 ep->entityId.sim_id.application_id,
					 ep->entityId.entity_id
					 );
			++ bcount;
		}
	}
}

/*
 *  Display the stealth browser page in the MFD
 */

void
doBrowsePage(craft * c, viewer * u)
{
	XSegment  seg[2048], m_seg[256];
	char buf[256], buf1[256];
	XRectangle rect[1];
	int       m_i = 0, i = 0, xc, yc, h, x, y;
	int       xscale, yscale, xoffset;
	int item_count;
	static ZInfo z, zm;
	double yy;


	if (c->radarMode != RM_DIS_BROWSE)
		return;

	rect[0].x = u->rx;
	rect[0].y = u->ry;
	rect[0].width = u->radarWidth;
	rect[0].height = u->radarWidth;

	z.depth = --u->v->depth;
	z.color = (Color) (u->v->pixel[radarBackgroundColor->cIndex]);
	zm.depth = z.depth;
	zm.color = (Color) (u->v->pixel[HSIMagentaColor->cIndex]);
	FillRectangle(u->v->w, u->rx, u->ry,
				  u->radarWidth, u->radarWidth, &z);

	xoffset = (int) ((-15.0 * u->scaleFactor) + 0.5);
	xc = u->rx + (u->radarWidth + 1) / 2;
	yc = u->ry + (u->radarWidth + 1) / 2;

	xscale = (int) (u->v->Scale.x / (SCALE_1 * 4));
	yscale = (int) (u->v->Scale.y / (SCALE_1 * 4));

	h = (int) (11.0 * u->scaleFactor + 0.5);
	y = (int) ((REF_Y + yy) * u->v->Scale.y / (SCALE_1 * 4));

	yy = 0.0;

	/*
	 *  Update the list of entities we might be interested in following
	 */

	buildBrowseInfoTable ();

	/*
	 *  display current DIS entity browsing page
	 */

	item_count = 0;
	for (i=u->browseBase; 
		 i<bcount && item_count<ITEM_LIMIT; 
		 ++i, ++item_count) {
		strcpy(buf, browse_info[i].info);
		x = (int) (REF_X * u->v->Scale.x / (SCALE_1 * 4));
		y = (int) ((REF_Y + yy) * u->v->Scale.y / (SCALE_1 * 4));
		if (u->browseSelectedItem == i) {
			VDrawStrokeString(u->v, x + xc, y + yc,
							  buf,
							  strlen(buf), h, &zm);
		}
		else {
			VDrawStrokeString(u->v, x + xc, y + yc,
							  buf,
							  strlen(buf), h, &u->z);
		}
		yy += LINE_SPACING;
	}

	u->v->w->clip.x1 = rect[0].x;
	u->v->w->clip.y1 = rect[0].y;
	u->v->w->clip.x2 = rect[0].x + rect[0].width - 1;
	u->v->w->clip.y2 = rect[0].y + rect[0].height - 1;

	VDrawSegments(u->v, m_seg, m_i, 
				  (Color) (u->v->pixel[HSIMagentaColor->cIndex]));

	VDrawSegments(u->v, seg, i, 
				  (Color) (u->v->pixel[HUDPixel]));

	u->v->w->clip.x1 = 0;
	u->v->w->clip.y1 = 0;
	u->v->w->clip.x2 = u->v->w->width - 1;
	u->v->w->clip.y2 = u->v->w->height - 1;

	return;
}

void
selectCockpitItem( craft *c, 
				   viewer *u, 
				   int x, 
				   int y, 
				   unsigned long time
				   )
{
	double dx, dy, yscale;
	int item;
	int xc, yc;

	xc = u->rx + (u->radarWidth + 1) / 2;
	yc = u->ry + (u->radarWidth + 1) / 2;

	yscale = (int) (u->v->Scale.y / (SCALE_1 * 4));

	if (c->radarMode != RM_DIS_BROWSE) {
		return;
	}

	/*
	 * Click on radar set?
	 */

	if (x > u->rx && x < u->rx+u->radarWidth &&
		y > u->ry && y < u->ry + u->radarWidth) {

		/* get index of selected item */

		dx = (x - yc) / yscale;
		dy = (y - yc) / yscale;
		dy -= REF_Y;
		item = (int) ( dy / LINE_SPACING ) + u->browseBase;

		if (item < bcount && item >= 0) {

			/*
			 *  Double-Click?  Activate control request
			 */

			if (u->browseSelectedItem == item &&
				time - u->browseClickTime < 500) {

#ifdef USE_REQUEST_CONTROL

				/*
				 * We can only take control of aircraft that we have a
				 * definition for.  When that's the case for a given
				 * entity, cinfo will be non-NULL.
				 */

				if (browse_info[item].cinfo) {

					/*
					 * If we're in end-game mode, check for hostile
					 * aircraft in our proximity.  Calling 
					 * endGameDistanceCheck once will cause it to be called
					 * once per second.
					 */

					u->viewer_state = ViewerStateControlPending;

					if ( end_game_mode ) {
						 browse_info[item].p->c->flags |= FL_END_GAME_DRONE;
						 endGameDistanceCheck ((char *) browse_info[item].p->c,
											   (char *) u );
					}
					else {
						dis_requestControl ( browse_info[item].p,
											 controlRequestCallback, 
											 u );
					}
				}
				else {
					XBell( u->dpy, 50 );
				}
#else
				XBell( u->dpy, 50 );
#endif

			}
			else {

				stealthCraft ( browse_info[item].p->c, u, item, 0 );

			}
		}

	}

	u->browseClickTime = time;
}

/*
 * Switch attention to the designated aircraft
 */

int
stealthCraft ( craft *c, viewer *u, int item, int take_control)
{
	int i;

	/*
	 *  Locate the browse info entry that corresponds to the
	 *  designated craft.
	 */

	if ( item == -1 ) {

		buildBrowseInfoTable ();

		for(i=0; i<bcount; ++i) {
			if (browse_info[i].p->c == c) {
				item = i;
				break;
			}
		}
	}

	/* follow that aircraft */
	if ( item != -1 ) {
		u->browseSelectedItem = item;
	}
	else {
		u->browseSelectedItem = -1;
	}

	if ( end_game_mode && take_control && item != -1 ) {
		browse_info[item].p->c->flags |= FL_END_GAME_DRONE;
		u->viewer_state = ViewerStateControlPending;
		endGameDistanceCheck ((char *) browse_info[item].p->c,
							  (char *) u );
	}
	else {
		u->viewer_state = ViewerStatePiggyback;
	}
	u->watchedCraft = c;

	return 0;
}

craft *
locateCraftByDISEntityID ( dis_entity_id *id )
{
	Entity_t *e = dis_getEntityTable();
	long top = dis_getEntityTop();
	long i;
	craft *result = NULL;

	for (i=0; i<=top; ++i) {
		if ( e->entityId.sim_id.site_id == id->sim_id.site_id &&
			 e->entityId.sim_id.application_id == id->sim_id.application_id &&
			 e->entityId.entity_id == id->entity_id ) {
			result = e->c;
			break;
		}
		++ e;
	}

	return result;

}

#ifdef USE_REQUEST_CONTROL

/*
 *  This callback is invoked by the DIS code when we receive a grant
 *  control PDU that we've been waiting for.
 */

int
controlRequestCallback( dis_pdu *pdu, void *pu )
{
	viewer *u = (viewer *) pu;
	Entity_t * e = dis_getEntityTable(), *ep;
	dis_simulation_addr my_addr;
	dis_entity_id new_entity_id;
	DISxApplicationInfo *app;
	craft *c;

	if ( pdu->hdr.pdu_type == PDUTypeAcknowledge &&
		 pdu->acknowledge.resp_flag == 1) {

	/*
	 * "take over the craft"
	 *
	 * alter the viewer entry to reflect that we've hijacked an aircraft
	 *
	 * alter the ptbl (craft) entry to reflect that this is now an aircraft
	 * that we are responsible for modeling
	 */

		u->viewer_state = ViewerStateNormal;
		c = u->watchedCraft;
		u->watchedCraft = u->c;
		u->c = c;

		c->type = CT_PLANE;
		c->vl = u;

		c->radarMode = RM_STANDBY;

		/*
		 *  Until we can think of a better way to set fuel state,
		 *  damage bits, etc. this will have to suffice.
		 */

		(*c->cinfo->resupply) (c);

		/*
		 *  Set up the radios
		 */

		initRadio(c, &c->navReceiver[0]);
		initRadio(c, &c->navReceiver[1]);
		c->hsiSelect = &c->navReceiver[0];

		ep = &e[c->disId];

		ep->local = 1;

		app = dis_getApplicationInfo();
		DISxGetSimulationAddress ( app, &my_addr );

		/* SITE ID */

		if ((transferEntityIdBits & 0x4)) {
			new_entity_id.sim_id.site_id = my_addr.site_id;
		}
		else {
			new_entity_id.sim_id.site_id = 
				ep->entityId.sim_id.site_id;
		}

		/* APPLICATION ID */

		if ((transferEntityIdBits & 0x2)) {
			new_entity_id.sim_id.application_id = my_addr.application_id;
		}
		else {
			new_entity_id.sim_id.application_id = 
				ep->entityId.sim_id.application_id;
		}

		/* ENTITY ID */

		if ((transferEntityIdBits & 0x1)) {

			/*  Issue a new entity id (good within this application) */

			dis_entity_id temp_id;

			DISxIssueEntityID( app, &temp_id );
			new_entity_id.entity_id = temp_id.entity_id;
		}
		else {

			/* use existing entity ID */
			new_entity_id.entity_id = ep->entityId.entity_id;
		}

		/* TODO: check for collisions in entity table if mode wasn't 0, or 7 */
		ep->entityId = new_entity_id;

	}

	/*
     *  Transfer Control Request was rejected.  Return to stealth state.
     */

	else {
		u->viewer_state = ViewerStatePiggyback;
		XBell( u->dpy, 50 );
	}

	return 0;
}

#endif

int
doBrowseKeyEvent(craft * c, viewer * u, XEvent * ev, int player)
{

	KeySym    keysym;
	XComposeStatus compose;
	char      buffer[MAX_MAPPED_STRING_LEN];
	int       buflen = MAX_MAPPED_STRING_LEN;

#ifdef SPECIAL_KEYS
	FILE     *fp;
	craft     pentry;

#endif

	(void) XLookupString((XKeyEvent *) ev, buffer, buflen,
						 &keysym, &compose);

	if (player) {

		switch (keysym) {

#ifdef sun
		case XK_R7:
#else
		case XK_Home:
#endif

			break;

		case XK_Prior:
			u->browseBase -= ITEM_LIMIT;
			if (u->browseBase < 0) {
				u->browseBase = 0;
				XBell ( u->dpy, 50 );
			}
			break;

		case XK_Next:
			u->browseBase += ITEM_LIMIT;
			if (u->browseBase >= bcount) {
				u->browseBase = bcount - ITEM_LIMIT + 1;
			}
			if (u->browseBase < 0) {
				u->browseBase = 0;
				XBell ( u->dpy, 50 );
			}
			break;

#ifdef sun
		case XK_Up:
#else
		case XK_KP_8:
#endif
			u->viewDirection.x = 1.0;
			u->viewDirection.y = 0.0;
			u->viewDirection.z = 0.0;
			u->viewUp.x = 0.0;
			u->viewUp.y = 0.0;
			u->viewUp.z = -1.0;
			break;

/* look right */

#ifdef sun
		case XK_Right:
#else
		case XK_KP_6:
#endif
			u->viewDirection.x = 0.0;
			u->viewDirection.y = 1.0;
			u->viewDirection.z = 0.0;
			u->viewUp.x = 0.0;
			u->viewUp.y = 0.0;
			u->viewUp.z = -1.0;
			break;

/* look left */

#ifdef sun
		case XK_Left:
#else
		case XK_KP_4:
#endif
			u->viewDirection.x = 0.0;
			u->viewDirection.y = -1.0;
			u->viewDirection.z = 0.0;
			u->viewUp.x = 0.0;
			u->viewUp.y = 0.0;
			u->viewUp.z = -1.0;
			break;

/* look back */

#ifdef sun
		case XK_Down:
#else
		case XK_KP_2:
#endif
			u->viewDirection.x = -1.0;
			u->viewDirection.y = 0.0;
			u->viewDirection.z = 0.0;
			u->viewUp.x = 0.0;
			u->viewUp.y = 0.0;
			u->viewUp.z = -1.0;
			break;

/* look up */

#ifdef sun
		case XK_R11:
#else
		case XK_KP_5:
#endif
			u->viewDirection.x = 0.0;
			u->viewDirection.y = 0.0;
			u->viewDirection.z = -1.0;
			u->viewUp.x = -1.0;
			u->viewUp.y = 0.0;
			u->viewUp.z = 0.0;
			break;

		case XK_N:
		case XK_n:
			c->flags ^= FL_CHASE_VIEW;
			break;

#ifdef SPECIAL_KEYS

		case XK_o:
			if (absorbDamage(c, 3) == 0) {
				killPlayerEx(c,
							 "You asked to absorb some damage. The aircraft was destroyed.",
							 "No further details are available.");
				return -1;
			}
			break;


		case XK_semicolon:
			debug ^= 1;
			break;

#endif

		case XK_P:
		case XK_p:
			killPlayer(c);
			return -1;
			return -1;
/*NOTREACHED */ break;

		case XK_braceleft:
			startBlackBoxRecording();
			break;

		case XK_braceright:
			endBlackBoxRecording();
			break;

		case XK_bracketleft:
			startBlackBoxPlayback();
			break;

		case XK_k:
		case XK_K:
			CalibrateJoystick();
			break;

		}

	}
	return 0;
}

