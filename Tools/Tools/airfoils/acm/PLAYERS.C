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

#include "pm.h"
#include "eng.xbm"
#include "flaps0.xbm"
#include "flaps1.xbm"
#include "flaps2.xbm"
#include "flaps3.xbm"
#include "handleUp.xbm"
#include "handleDn.xbm"
#include "gearUp.xbm"
#include "gearTran.xbm"
#include "gearDown.xbm"
#include <stdio.h>
#include <X11/Xutil.h>
#ifndef __hpux
#include <X11/Xos.h>
#endif
#include <X11/cursorfont.h>
#ifdef HAVE_DIS
#include "dis.h"
#endif

#if defined(HAVE_STDLIB_H)
#ifndef bzero
#define bzero(s,n)      memset ((s), 0, (n))
#endif
#endif							/* HAVE_STDLIB_H */

extern void resizePlayerWindow(craft * c, 
							   viewer * u, 
							   int width, 
							   int height, 
							   int initial_flag);
extern void blackBoxKillPlayer(int id);
extern void printValidAircraft(int s);
extern void buildEulerMatrix(double roll, double pitch, double heading, 
							 VMatrix * m);
extern void transpose(VMatrix * m, VMatrix * r);

#define BORDER	1

#define	ARG_FONT		"font"
#define ARG_RADAR_FONT		"radarfont"
#define	ARG_BORDER_COLOR	"bordercolor"
#define ARG_BORDER		"borderwidth"
#define	ARG_GEOMETRY		"geometry"
/* #define DEFAULT_BACKGROUND   "#93bde4"   /* my version of sky blue */
/* #define DEFAULT_BACKGROUND   "#7491ae"   /* my version of sky blue */
#define DEFAULT_BACKGROUND	"#7c99b6"	/* my version of sky blue */
#define DEFAULT_BORDER		"black"
#define DEFAULT_RADAR_FONT	"-misc-fixed-medium-*-*-*-*-120-*-*-*-*-*-*"

#define SW_BORDER	      1
#define SW_BACKGROUND	  2
#define SW_HUDFONT	      3
#define SW_GEOM		      4
#define SW_RADARFONT	  5
#define SW_TEAM		      6
#define SW_DEFAULT_VISUAL 8
#define SW_PLANE	      9
#define SW_LIST_PLAYER	 10
#define SW_MONOCHROME	 11
/* 12 unused */
#define SW_PASSIVE       13
#define SW_LATITUDE      14
#define SW_LONGITUDE     15
#define SW_ALTITUDE      16
#define SW_AIRSPEED_KTS  17
#define SW_HEADING       18
#define SW_END_GAME      19
#define SW_LAST 19             /* last entry in list */
               
struct {
	char     *sw;
	int       value;
} swt[] = {
	"-bw",        SW_BORDER,
	"-skycolor",  SW_BACKGROUND,
	"-hudfont",   SW_HUDFONT,
	"-radarfont", SW_RADARFONT,
	"-geometry",  SW_GEOM,
	"-team",      SW_TEAM,
	"-stealth",   SW_PASSIVE,
	"-cmap",      SW_DEFAULT_VISUAL,
	"-mono",      SW_MONOCHROME,
	"-plane",     SW_PLANE,
	"-list",      SW_LIST_PLAYER,
	"-latitude",  SW_LATITUDE,
	"-longitude", SW_LONGITUDE,
	"-altitude",  SW_ALTITUDE,
	"-airspeed-kts", SW_AIRSPEED_KTS,
	"-heading",   SW_HEADING,
	"-end-game",  SW_END_GAME,
	NULL, 0
}, *swp;

void
recoverAcmArgv(char *args, int *argc, char **argv)
{

	char     *s;

	argv[0] = ACM;
	argv[1] = args;

	if (*args == '\0') {
		*argc = 1;
		argv[1] = (char *) NULL;
		return;
	}
	else
		*argc = 2;

	for (s = args; *s;) {
		if (*s == '|') {
			*s = '\0';
			argv[(*argc)++] = ++s;
		}
		else
			++s;
	}

	argv[*argc] = (char *) NULL;
}

Visual   *
get_pseudo_visual(Display * display, int screen, unsigned int *depth)
{

	XVisualInfo vTemplate;
	XVisualInfo *visualList;
	int       i, visualsMatched;

/*
 *  Get all Visuals on this screen
 */

	vTemplate.screen = screen;
	visualList = XGetVisualInfo(display, VisualScreenMask,
								&vTemplate, &visualsMatched);

	if (visualsMatched == 0)
		return NULL;

/*
 *  look first for a suitable PseudoColor visual, then a GreyScale visual
 *  if no PseudoColor is present
 */

	for (i = 0; i < visualsMatched; ++i) {
		if (visualList[i].visual->class == PseudoColor &&
			visualList[i].depth >= 4) {
			*depth = visualList[i].depth;
			XFree((char *) visualList);
			return visualList[i].visual;
		}
	}

	for (i = 0; i < visualsMatched; ++i) {
		if (visualList[i].visual->class == GrayScale &&
			visualList[i].depth >= 4) {
			*depth = visualList[i].depth;
			XFree((char *) visualList);
			return visualList[i].visual;
		}
	}

	XFree((char *) visualList);

	return NULL;
}

int
newPlayer(int s, char *display, char *logname, char *switches)
{

	char     *fontName;			/* Name of font for string */
	XSizeHints xsh;				/* Size hints for window manager */
	Colormap  cmap;
	XGCValues gcv;
	unsigned long pad;			/* Font size parameters */
	unsigned long bd;			/* Pixel values */
	unsigned long bw;			/* Border width */
	char     *tempstr;			/* Temporary string */
	XColor    color;			/* Temporary color */
	char     *geomSpec;			/* Window geometry string */
	XWMHints  xwmh;				/* Window manager hints */
	Cursor    cursor;
	char    **c;
	char      err[256];
	static char *background = NULL;
	int       borderWidth = -1;
	int       player;
	viewer   *u;
	craft    *cf;
	int       argc;
	char     *argv[32];
	int       screen;
	char     *hudfont = NULL, *radarfont = NULL;
	int       team = 1;
	char     *plane = NULL;		/* name of plane type */
	int       obsrver = -1;
	int       width, height;	/* dimensions of main window */
	double    scale;
	unsigned  depth, mono = 0;
	long      win_attr_mask;
	Visual   *theVisual;
	XSetWindowAttributes window_attributes;
	int       useDefaultVisual = 1;
	int       passive = 0;
	Atom      atom[2];
	int       overrides[SW_LAST+1];
	double    overrideLatitude, overrideLongitude, overrideAltitude;
	double    overrideHeading_rad, overrideAirspeed_fps;
	int       end_game = 0;

#ifdef HAVE_DIS
	double    disLocation[3];
	double    disZeroVec[3];
	double    disOrientation[3];
	int       disType;

#endif

	memset(overrides, 0, sizeof(overrides));

	recoverAcmArgv(switches, &argc, argv);

	geomSpec = NULL;
	u = (viewer *) malloc(sizeof(viewer));
	if (!u) {
		printf ("unable to allocate viewer\n");
		exit (1);
	}
	u->watchedCraft = NULL;
	u->viewer_state = ViewerStateNormal;
	u->browseBase = 0;
	u->browseSelectedItem = -1;
	u->browseClickTime = 0;
	u->viewDirection.x = 1.0;
	u->viewDirection.y = 0.0;
	u->viewDirection.z = 0.0;
	u->viewUp.x = 0.0;
	u->viewUp.y = 0.0;
	u->viewUp.z = -1.0;

/*
 *  Parse command line
 */

	for (c = &argv[1]; *c != (char *) NULL; ++c)
		if (**c == '-') {
			for (swp = &swt[0]; swp->value != 0; ++swp)
				if (strcmp(swp->sw, *c) == 0) {

					switch (swp->value) {

					case SW_GEOM:
						geomSpec = *(++c);
						break;

					case SW_END_GAME:
						end_game = 1;
						break;

					case SW_BORDER:
						borderWidth = atoi(*(++c));
						break;

					case SW_BACKGROUND:
						background = *(++c);
						break;

					case SW_HUDFONT:
						hudfont = *(++c);
						break;

					case SW_RADARFONT:
						radarfont = *(++c);
						break;

					case SW_TEAM:
						team = atoi(*(++c));
						break;

					case SW_PASSIVE:
					    passive = 1;
						break;

					case SW_PLANE:
						plane = *(++c);
						break;

					case SW_MONOCHROME:
						mono = 1;
						break;

					case SW_LATITUDE:
						overrideLatitude = DEGtoRAD(atof(*(++c)));
						overrides[SW_LATITUDE] = 1;
						break;

					case SW_LONGITUDE:
						overrideLongitude = DEGtoRAD(atof(*(++c)));
						overrides[SW_LONGITUDE] = 1;
						break;

					case SW_ALTITUDE:
						overrideAltitude = FEETtoMETERS(atof(*(++c)));
						overrides[SW_ALTITUDE] = 1;
						break;

					case SW_AIRSPEED_KTS:
						overrideAirspeed_fps = KTStoFPS(atof(*(++c)));
						if (overrideAirspeed_fps > KTStoFPS(2500.0)) {
							printf ("You really should slow down.\n");
							printf ("At least to less than 2500 knots.\n");
							exit (1);
						}
						overrides[SW_AIRSPEED_KTS] = 1;
						break;

					case SW_HEADING:
						overrideHeading_rad = DEGtoRAD(atof(*(++c)));
						overrides[SW_HEADING] = 1;
						break;

					case SW_LIST_PLAYER:
						sprintf(err, "\nname\t\tnumber\n");
						write(s, err, strlen(err));
						sprintf(err, "-------------------------\n");
						write(s, err, strlen(err));
						for (team = 0; team < MAXPLAYERS; team++) {
							if (ptbl[team].type == CT_PLANE) {
								sprintf(err, "%-16s  %d\n",
										ptbl[team].name, team);
								write(s, err, strlen(err));
							}
						}
						return -1;

					case SW_DEFAULT_VISUAL:
						useDefaultVisual = 0;
						break;
					}
					break;
				}
			if (swp->value == 0) {
				free((char *) u);
				sprintf(err, "%s: invalid switch %s", ACM, *c);
				write(s, err, strlen(err));
				return -1;
			}
		}

	if ( 1 ) {

		if (!plane) {
			plane = (team == 1) ? "F-16" : "MiG-29";
		}

		if ((player = newPlane(plane, -1)) < 0) {
			if (player == -1) {
				sprintf(err,
				"Sorry, no room for any more players at this moment.\n");
				write(s, err, strlen(err));
			}
			else {
				sprintf(err,
						"You have selected an unknown plane type.\n");
				write(s, err, strlen(err));
				printValidAircraft(s);
			}
			return -1;
		}

		cf = &ptbl[player];

/*
 *  assign an initial location basewd on the player's team.
 */

		if (team != 2) {
			team = 1;
		}

		cf->w.latitude  = teamLatLon[team].latitude;
		cf->w.longitude = teamLatLon[team].longitude;
		cf->w.z = cf->prevw.z = cf->w.z + teamLatLon[team].z;
		DISWorldCoordinatesToGeocentric(&cf->w,
									 (dis_world_coordinates *) & cf->Sg);
		GenerateWorldToLocalMatrix(&cf->w, &cf->XYZtoNED);

		cf->curHeading = teamHeading[team];

		buildEulerMatrix(cf->curRoll, cf->curPitch, cf->curHeading,
						 &(cf->trihedral));
	}

	/*
	 *  If we're passive (stealth mode), wipe out aircraft information;
	 *  this entry will become a placeholder for our browsing view.
	 */

	if ( passive ) {

		u->viewer_state = ViewerStateBrowsing;
		cf->type = CT_DIS_STEALTH;
		cf->cinfo = NULL;
		cf->radarMode = RM_DIS_BROWSE;

		if ( end_game ) {
			end_game_mode = 1;
		}

		/*
		 *  Stealth a specific entity?  Go ahead and set it as the subject.
		 *
		 *  This requires snooping for entities in the PDU stream prior
		 *  to looking for the entity in the table.
		 */

		if ( subjectEntitySpecified ) {
			craft *c;

			printf ("Building entity database ... ");
			fflush ( stdout );

			updateSimTimeFromSystemClock ();
			dis_snoop ( 5500 );

			printf ("done.\n");

			c = locateCraftByDISEntityID ( & subjectEntityID );
			if ( c ) {

				stealthCraft ( c, u, -1, 1);

			}
		}

	}
	else {

		if ( end_game ) {
			fprintf ( stderr, "The -end-game switch is only valid when used " );
			fprintf ( stderr, "with stealth mode.\n" );
			fprintf ( stderr, "The switch will be ignored.\n\n" );
		}

		if ( subjectEntitySpecified ) {
			fprintf ( stderr, "The -subject-entity-id switch is only valid when used " );
			fprintf ( stderr, "with stealth mode.\n" );
			fprintf ( stderr, "The switch will be ignored.\n\n" );
		}

	}

	if ((u->dpy = XOpenDisplay(display)) == (Display *) NULL) {
		free((char *) u);
		cf->type = CT_FREE;
		sprintf(err, "%s: can't open %s\n", ACM, display);
		write(s, err, strlen(err));
		return -1;
	}
	screen = DefaultScreen(u->dpy);

	/*
	 *  Add viewer to list
	 */

	addViewer ( u );
	u->c = cf;

	if (radarfont)
		fontName = radarfont;
	else if ((fontName = XGetDefault(u->dpy, ACM, ARG_RADAR_FONT)) == NULL) {
		fontName = DEFAULT_RADAR_FONT;
	}
	if ((u->rfont = XLoadQueryFont(u->dpy, fontName)) == NULL) {
		XCloseDisplay(u->dpy);
		free((char *) u);
		cf->type = CT_FREE;
		sprintf(err, "%s: display %s doesn't know font %s\n",
				ACM, display, fontName);
		write(s, err, strlen(err));
		return -1;
	}

/*
 *  If the player has specified that the want the default Visual, simply
 *  give 'em that along with the default Colormap.
 */

	if (useDefaultVisual) {

		theVisual = DefaultVisual(u->dpy, screen);
		cmap = DefaultColormap(u->dpy, screen);
		depth = DisplayPlanes(u->dpy, screen);

	}

/*
 *  Look for a visual; if we can't find one,
 *  fall back to monochrome mode.
 */

	else {
		if ((theVisual = get_pseudo_visual(u->dpy, screen, &depth)) == NULL) {
			theVisual = DefaultVisual(u->dpy, screen);
			cmap = DefaultColormap(u->dpy, screen);
			depth = DefaultDepth(u->dpy, screen);
		}
		else {
			cmap = XCreateColormap(u->dpy, RootWindow(u->dpy, screen),
								   theVisual, AllocNone);
		}
	}

/*
 * Select colors for the border, the window background, and the
 * foreground.  We use the default colormap to allocate the colors in.
 */

	if (background == NULL)
		background = DEFAULT_BACKGROUND;

	if ((tempstr = XGetDefault(u->dpy, ACM, ARG_BORDER_COLOR)) == NULL)
		tempstr = DEFAULT_BORDER;
	if (XParseColor(u->dpy, cmap, tempstr, &color) == 0) {
		XCloseDisplay(u->dpy);
		free((char *) u);
		cf->type = CT_FREE;
		sprintf(err, "Can't get border color %s\n", tempstr);
		write(s, err, strlen(err));
		return -1;
	}

	if (depth == 1)
		bd = BlackPixel(u->dpy, screen);
	else {
		if (XAllocColor(u->dpy, cmap, &color) == 0) {
			XCloseDisplay(u->dpy);
			free((char *) u);
			cf->type = CT_FREE;
			sprintf(err, "Cannot allocate color cells\n");
			write(s, err, strlen(err));
			return -1;
		}
		bd = color.pixel;
	}

/*
 * Set the border width of the window, and the gap between the text
 * and the edge of the window, "pad".
 */

	pad = BORDER;
	if (borderWidth >= 0)
		bw = borderWidth;
	else if ((tempstr = XGetDefault(u->dpy, ACM, ARG_BORDER)) == NULL)
		bw = 1;
	else
		bw = atoi(tempstr);

/*
 * Deal with providing the window with an initial position & size.
 * Fill out the XSizeHints struct to inform the window manager.
 */

	if (geomSpec == NULL)
		geomSpec = XGetDefault(u->dpy, ACM, ARG_GEOMETRY);

/*
 * If the defaults database doesn't contain a specification of the
 * initial size & position, locate it in the center of the screen.
 */

	if (geomSpec == NULL) {
		xsh.flags = PPosition | PSize;
		xsh.height = FS_WINDOW_HEIGHT * DisplayWidth(u->dpy, screen) / 1280;
		xsh.width = FS_WINDOW_WIDTH * DisplayWidth(u->dpy, screen) / 1280;
		xsh.x = (DisplayWidth(u->dpy, screen) - xsh.width) / 2;
		xsh.y = (DisplayHeight(u->dpy, screen) - xsh.height) / 2;
	}
	else {
		int       bitmask;

		bzero((char *) &xsh, sizeof(xsh));
		bitmask = XParseGeometry(geomSpec, &xsh.x, &xsh.y,
								 &xsh.width, &xsh.height);
		if (bitmask & (XValue | YValue)) {
			xsh.flags |= USPosition;
		}
		if (bitmask & (WidthValue | HeightValue)) {
			xsh.flags |= USSize;
		}
	}

	width = xsh.width;
	height = xsh.height;

/*
 * Create the Window with the information in the XSizeHints, the
 * border width,  and the border & background pixels.
 */

	win_attr_mask = CWColormap | CWBitGravity | CWBackPixel;
	window_attributes.colormap = cmap;
	window_attributes.bit_gravity = NorthWestGravity;
	window_attributes.background_pixel = BlackPixel(u->dpy, screen);

	u->win = XCreateWindow(u->dpy, RootWindow(u->dpy, screen),
						   xsh.x, xsh.y, xsh.width, xsh.height, bw, depth,
			  InputOutput, theVisual, win_attr_mask, &window_attributes);

	scale = (double) xsh.width / (double) FS_WINDOW_WIDTH;

/*
 * Create a pixmap of the engine RPM gauge and flap indicators.
 */

	u->eng = XCreateBitmapFromData(u->dpy, u->win, eng_bits, eng_width,
								   eng_height);
	u->flap[0] = XCreateBitmapFromData(u->dpy, u->win, flaps0_bits,
									   flaps0_width, flaps0_height);
	u->flap[1] = XCreateBitmapFromData(u->dpy, u->win, flaps1_bits,
									   flaps1_width, flaps1_height);
	u->flap[2] = XCreateBitmapFromData(u->dpy, u->win, flaps2_bits,
									   flaps2_width, flaps2_height);
	u->flap[3] = XCreateBitmapFromData(u->dpy, u->win, flaps3_bits,
									   flaps3_width, flaps3_height);
	u->handle[0] = XCreateBitmapFromData(u->dpy, u->win, handleUp_bits,
										 handleUp_width, handleUp_height);
	u->handle[1] = XCreateBitmapFromData(u->dpy, u->win, handleDn_bits,
										 handleDn_width, handleDn_height);
	u->gearLight[0] = XCreateBitmapFromData(u->dpy, u->win, gearUp_bits,
											gearUp_width, gearUp_height);
	u->gearLight[1] = XCreateBitmapFromData(u->dpy, u->win, gearTran_bits,
										gearTran_width, gearTran_height);
	u->gearLight[2] = XCreateBitmapFromData(u->dpy, u->win, gearDown_bits,
										gearDown_width, gearDown_height);

/*
 * Set the standard properties and hints for the window managers.
 */

	XSetStandardProperties(u->dpy, u->win, ACM, ACM, None, argv, argc, &xsh);
	xwmh.flags = InputHint | StateHint;
	xwmh.input = True;
	xwmh.initial_state = NormalState;
	XSetWMHints(u->dpy, u->win, &xwmh);
	cursor = XCreateFontCursor(u->dpy, XC_tcross);
	XDefineCursor(u->dpy, u->win, cursor);

/*
 *  Tell the window manager that we'd like to participate in the
 *  WM_CLOSEDOWN and WM_DELETE_WINDOW protocols.
 */

	u->protocolsAtom = XInternAtom(u->dpy, "WM_PROTOCOLS", False);
	atom[0] =
		u->closedownAtom = XInternAtom(u->dpy, "WM_CLOSEDOWN", False);
	atom[1] =
		u->deleteWindowAtom = XInternAtom(u->dpy, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(u->dpy, u->win, atom, 2);

/*
 *  Fill-in the viewer structure
 */
	XSelectInput(u->dpy, u->win, KeyPressMask | ButtonPressMask |
				 StructureNotifyMask | ButtonReleaseMask | ExposureMask);
	cf->team = team;

	cf->vl = u;
	strncpy(cf->name, logname, sizeof(cf->name));
	strncpy(cf->display, display, sizeof(cf->display));
	u->next = (viewer *) NULL;

/*
 *  We use the drawController to optimize areas that we'll
 *  be drawing using X, as well.
 */
	initializeDrawnItemController(&u->drawControl);
	addDrawnItem(&u->drawControl, &u->rpmState);
	addDrawnItem(&u->drawControl, &u->flapState);
	addDrawnItem(&u->drawControl, &u->fuelState);
	addDrawnItem(&u->drawControl, &u->consumpState);
	addDrawnItem(&u->drawControl, &u->gearState);
	addDrawnItem(&u->drawControl, &u->annunciatorState);
	addDrawnItem(&u->drawControl, &u->radarState);
	redrawAllItems(&u->drawControl);

/*
 *  Connect to the audio server
 */

	if (initializeAudio(cf, u, display) != 0) {
		sprintf(err, "Sound is not available on workstation %s\n", display);
		write(s, err, strlen(err));
	}

#ifdef USE_PIXMAP_ANIMATION
	if (depth != 1)
		_VDefaultWorkContext->usePixmaps = 1;
#endif

	_VDefaultWorkContext->usePixmaps = mono ? 1 : 0;

	u->v = VOpenViewport(u->dpy, screen, u->win, cmap, theVisual,
						 UNITS_METERS, scale, FEETtoMETERS(0.50),
						 width * VIEW_WINDOW_WIDTH / FS_WINDOW_WIDTH,
						 height * VIEW_WINDOW_HEIGHT / FS_WINDOW_HEIGHT);

/*
 *  If this is a color-rich visual, then enable depth-cueing mode
 */

	if (depth >= 6) {
		ViewportSetDepthCueing(u->v, 1);
	}

	if (VBindColors(u->v, background) < 0) {
		XCloseDisplay(u->dpy);
		free((char *) u);
		ptbl[player].type = CT_FREE;
		sprintf(err, "Error in binding colors.\n");
		write(s, err, strlen(err));
		return -1;
	}

	whitePixel = whiteColor->cIndex;
	blackPixel = blackColor->cIndex;
	HUDPixel = HUDColor->cIndex;

/*
 * Create the GC for drawing the picture.
 */

	gcv.graphics_exposures = False;
	gcv.font = u->rfont->fid;
	u->gc = XCreateGC(u->dpy, u->win, GCGraphicsExposures | GCFont, &gcv);

	gcv.graphics_exposures = False;
	gcv.foreground = u->v->flags & VPMono ?
		WhitePixel(u->v->dpy, u->v->screen) :
		VConstantColor(u->v, whitePixel);
	gcv.background = u->v->flags & VPMono ?
		BlackPixel(u->v->dpy, u->v->screen) :
		VConstantColor(u->v, blackPixel);
	gcv.line_width = 2;
	gcv.font = u->rfont->fid;
	u->gauge_gc = XCreateGC(u->dpy, u->win,
					  GCGraphicsExposures | GCForeground | GCBackground |
							GCLineWidth | GCFont, &gcv);


	resizePlayerWindow(cf, u, width, height, 1);

/*
 * Map the window to make it visible.
 */

	XMapWindow(u->dpy, u->win);
	if (depth != 1) {
#ifdef notdef
		register int curPixel;

		VExposeBuffer(u->v, u->gc);
		curPixel = *(u->v->pixel);
		XSetForeground(u->dpy, u->gc, curPixel);
		XFillRectangle(u->dpy, u->win, u->gc, 0, 0, u->width, u->height);
#endif
	}

	if (depth == 1)
		XSetWindowBackground(u->dpy, u->win, BlackPixel(u->dpy, screen));
	else
		XSetWindowBackground(u->dpy, u->win, VConstantColor(u->v, blackPixel));

	u->z.color = u->v->pixel[HUDPixel];
	u->z.depth = 1;

	u->rz.color = u->v->pixel[radarColor->cIndex];
	u->rz.depth = 2;

	++ptblCount;

	/*
	 * Take appropriate actions based on any overrides specified by the user
	 */

	if (overrides[SW_LATITUDE] || 
		overrides[SW_LONGITUDE] ||
		overrides[SW_ALTITUDE]) {

		if (overrides[SW_LATITUDE]) {
			cf->w.latitude = overrideLatitude;
		}

		if (overrides[SW_LONGITUDE]) {
			cf->w.longitude = overrideLongitude;
		}

		if (overrides[SW_ALTITUDE]) {
			cf->w.z = overrideAltitude;
		}
		else {
			cf->w.z = 0.0;
		}
		
		DISWorldCoordinatesToGeocentric(&cf->w,
										(dis_world_coordinates *) & cf->Sg);

		if (overrides[SW_ALTITUDE] == 0) {
			cf->w.z = localAltitude( &cf->Sg, &cf->w ) +
				FEETtoMETERS( cf->cinfo->groundingPoint.z );
			DISWorldCoordinatesToGeocentric(&cf->w,
											(dis_world_coordinates *) &cf->Sg);
		}

		cf->prevw = cf->w;
		cf->prevSg = cf->Sg;

		GenerateWorldToLocalMatrix(&cf->w, &cf->XYZtoNED);
	}

	if (overrides[SW_HEADING]) {

		cf->curHeading = overrideHeading_rad;

		buildEulerMatrix(cf->curRoll, cf->curPitch, cf->curHeading,
						 &(cf->trihedral));
	}

	if (overrides[SW_AIRSPEED_KTS]) {
		VPoint v = { overrideAirspeed_fps, 0.0, 0.0 };
		VTransform_ ( &v, &cf->trihedral, &cf->Cg );
	}

	/*
	 *  Transmit initial DIS entity state PDU
	 */
	
#ifdef HAVE_DIS
	if ( disInUse && cf->type != CT_DIS_STEALTH ) {

		DISWorldCoordinatesToGeocentric(&cf->w,
								  (dis_world_coordinates *) disLocation);
		disZeroVec[0] = 0.0;
		disZeroVec[1] = 0.0;
		disZeroVec[2] = 0.0;
		disOrientation[0] = cf->curHeading;
		disOrientation[1] = 0.0;
		disOrientation[2] = 0.0;

		dis_entityEnter(team, cf,
						&cf->cinfo->entityType,
						&cf->cinfo->altEntityType,
						disLocation, disZeroVec, disZeroVec,
						disOrientation, disZeroVec, &cf->disId);
	}
#endif

	return 0;

}

#ifndef WIN32

/*
 *  Until we have a "you're dead" dialog box in the UNIX version of ACM, 
 *  this will have to do ...
 */

int
killPlayerEx(craft * c,...)
{
	return killPlayer(c);
}
#endif

int
killPlayer(craft * c)
{

	viewer   *v, *vn;
	int       i;
	VPoint	 vel = { 0, 0, 0 };

	playSound(c, SoundCrash);

/*
 *  Decrement the player count, iff this is a real person that just got
 *  killed.
 */

	if (c->type == CT_PLANE && (c->flags & FL_BLACK_BOX) == 0) {
		--ptblCount;
	}

/*
 *  Erase our radar emissions
 */

	for (i = 0; i < MAXPLAYERS; ++i)
		ptbl[i].rval[c->pIndex] = 0.0;

/*
 *  Replace the plane with an explosion.
 */

	newExplosion(&(c->Sg), &vel, 30.0, 15.0, 4.0);

/*
 *  Free flight director storage
 */

#ifdef AFDS
	AFDSFree( c );

	if (c->fp) {
		freeFlightPlan ( c->fp );
	}
#endif

/*
 *  Free HUD string storage
 */

	if (c->leftHUD[0] != (char *) NULL)
		for (i = 0; i < 6; ++i) {
			free(c->leftHUD[i]);
			free(c->rightHUD[i]);
		}

/*
 *  Close viewers' display
 */

	for (v = c->vl; v != (viewer *) NULL;) {

		/*
		 *  If this was a situation where we had grabbed control in steath
		 *  mode and then died, return to the browsing state.
		 */

		if ( v->viewer_state == ViewerStateNormal && 
			 v->watchedCraft != NULL &&
			 v->watchedCraft->type == CT_DIS_STEALTH ) {
			printf ("player killed: returning to stealth browsing mode\n");
			v->viewer_state = ViewerStateBrowsing;
			v->c = v->watchedCraft;
			v->watchedCraft = NULL;
			v = NULL;
		}
		else {

			/*
			 *  remove viewer from viewer list
			 */

			if (removeViewer ( v )) {
				printf ("unable to locate viewer in viewer list\n");
			}

			/*
			 *  shutdown the viewer
			 */

			VCloseViewport(v->v);
			XCloseDisplay(v->dpy);
			shutdownAudio(c, v);
			vn = (viewer *) v->next;
			free((char *) v);
			v = NULL;
		}
	}

	if (c->flags & FL_RECORD) {
		--recordCount;
	}

	if (c->flags & FL_BLACK_BOX)
		blackBoxKillPlayer(c->pIndex);

#ifdef HAVE_DIS
	if (c->type == CT_PLANE || c->type == CT_DRONE) {
		/* active (broadcasting) player, announce its death */
#ifdef DIS_DEBUG
		printf("Killed local player/drone: %d %d\n", c->pIndex, c->disId);
#endif
		dis_entityExit(c->disId);
	}
#endif

	c->type = CT_FREE;
	return 0;
}
