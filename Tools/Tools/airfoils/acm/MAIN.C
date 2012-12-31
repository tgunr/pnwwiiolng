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

#if defined(SVR4)
#include <sys/filio.h>
#endif

#include "manifest.h"
#include "patchlevel.h"
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <pwd.h>
#include <signal.h>
#include <sys/socket.h>
#include "pm.h"

#undef a

#ifdef SVR4
/*
 * This forces the inclusion of filio.h (and other things) in ioctl.h 
 * on NCR SVR4.
 */
#define BSD_COMP 1
#endif							/* SVR4 */

#include <sys/ioctl.h>
#include <sys/time.h>
#ifdef _AIX
#include <sys/select.h>
#endif
#include <netinet/in.h>
#include <netdb.h>
#include <setjmp.h>

#ifdef SVR4
static sigset_t sigset_mask, sigset_omask;
#endif							/* SVR4 */

#if !defined(lint) && !defined(SABER)
static char sccsid[] = "@(#) acm by Riley Rainey; Revision 5.0";
#endif

extern struct servent *getservent( /* ??? */ );
int       sdebug = 1;
int       listen_socket;
char     *sceneFile = (char *) NULL;
extern double atof(const char *);

extern void SetJoystickPort PARAMS((char *));

int processCommandFile(const char *fname, 
				   int *argc, 
				   char *argv[],
				   char *display, 
				   char *name,
				   char *switches);

int processCommandSwitches (int argc, 
							char *argv[], 
							char *display, 
							char *name,
							char *switches);

#ifdef HAVE_DIS

#include "dis.h"

int       dis_site = DIS_SITE;
int       dis_application = DIS_APPLICATION;
int       dis_exercise = DIS_EXERCISE;

void
disEntityEnterCb(int eid, dis_entity_type * etype, int force, craft ** cptr)
{
	int       i, j, top, mtype;
	craft    *c;
	char     *type;
	int       team;
	craft    *tbl;

	*cptr = NULL;

#ifdef DIS_DEBUG
	printf("%8.2f: Network Entity enter: %d\n", curTime, eid);
#endif

	if (etype->kind == DISKindPlatform) {
		tbl = ptbl;
		top = MAXPLAYERS;
		mtype = CT_DIS_PLANE;
	}
	else if (etype->kind == DISKindMunition) {
		tbl = mtbl;
		top = MAXPROJECTILES;
		mtype = CT_DIS_MUNITION;
	}
	else {
		return;
	}

/*
 *  most of this is a copy of the newDrone code in newPlane.c
 */

	for (i = 0; i < top; ++i) {

		if (tbl[i].type == CT_FREE) {

			if (force == DISForceFriendly) {
				team = 1;
			}
			else {
				team = 2;
			}

			c = &tbl[i];
			*cptr = c;

			c->createTime = curTime;
			c->vl = NULL;
			c->disId = eid;
			c->team = team;
			c->curOpponent = -1;
			c->holdCount = 0;

			c->vl = NULL;
			c->type = mtype;
			c->cinfo = lookupCraftByEntityType(etype);

/*
 *  If we don't know about a given entity type, then we will not track it.
 */

			if (!c->cinfo) {
				*cptr = NULL;
				c->type = CT_FREE;
				return;
			}
			strncpy(c->name, "DIS", sizeof(c->name));

			c->curNWDef = 0.0;
			c->flags = 0;
			c->radarMode = RM_OFF;
			c->curRadarTarget = -1;

			for (j = 0; j < 6; ++j) {
				c->leftHUD[j] = Vmalloc(32);
				c->rightHUD[j] = Vmalloc(32);
				strcpy(c->leftHUD[j], "");
				strcpy(c->rightHUD[j], "");
			}

			break;
		}
	}

	if (i == top)
		fprintf(stderr, "Out of players (increase MAXPLAYERS)\n");
}

void
disDetonationCb(int ftype, 
				int firingEid, 
				int targetEid, 
				double time, 
				double *worldLocation, 
				double *entityLocation, 
				craft * m, 
				dis_detonation_pdu *dpdu)
{
	craft    *c;
	int       j;
	VPoint    Sg, rloc, rvel, tmp;
	double   exp_diameter, dist_meters, vel_meters_per_sec;

	if (m) {
		killMissile(m, (craft *) NULL);
	}

#ifdef DIS_DEBUG
	printf("DIS detonation seen; target %d\n", targetEid);
#endif

	Sg.x = worldLocation[0];
	Sg.y = worldLocation[1];
	Sg.z = worldLocation[2];

	for (c = ptbl, j = 0; j < MAXPLAYERS; ++j, ++c)

		if (c->type != CT_FREE && c->disId == targetEid) {

			/* found the target */

			if (c->type != CT_DIS_PLANE) {

				/* impact distance from C.G. */

				rloc.x = c->Sg.x - Sg.x;
				rloc.y = c->Sg.y - Sg.y;
				rloc.z = c->Sg.z - Sg.z;
				dist_meters = mag(rloc);

				/* impact velocity */

				tmp.x = FEETtoMETERS(c->Cg.x);
				tmp.y = FEETtoMETERS(c->Cg.y);
				tmp.z = FEETtoMETERS(c->Cg.z);
				VReverseTransform_(&tmp, &c->XYZtoNED, &rvel);
				rvel.x = dpdu->vel.x - rvel.x;
				rvel.y = dpdu->vel.y - rvel.y;
				rvel.z = dpdu->vel.z - rvel.z;
				vel_meters_per_sec = mag(rvel);

				/* the target is a local player, damage him */
				if (absorbDISDamage(c, 
									&dpdu->burst.munition,
									dpdu->burst.warhead, 
									dpdu->burst.fuze,
									dist_meters,
									vel_meters_per_sec,
									&exp_diameter) == 0) {
					killPlayerEx(c,
								 "Your aircraft has been destroyed.",
								 (ftype == DIS_FIRE_M61A1 ?
								  "You were struck by cannon fire." :
								  "Something (presumably a missile) detotated close enough to your aircraft to destroy it."));
				}
				if (exp_diameter > 0.0) {
					VPoint vel = { 0, 0, 0 };
					newExplosion (&Sg, &vel, exp_diameter * 0.3, exp_diameter, 3.0);
				}

			}
			break;
		}
}

int
disInit(void)
{
	int       err, count;
	char      name[32];
	struct sockaddr_in sin;

	err = dis_init(dis_exercise,
				   dis_site, dis_application,
				   disEntityEnterCb, disDetonationCb);
	dis_setDRThresholds(DIS_LOCATION_THRESHOLD, DIS_ORIENTATION_THRESHOLD);

	return err;
}

#endif

void
parseinfo(char *s, char *a, char *b, char *c)
{

	char     *p;

	for (p = a; *s; ++s, ++p)
		if ((*p = *s) == ' ') {
			*p = '\0';
			break;
		}
	++s;

	for (p = b; *s; ++s, ++p)
		if ((*p = *s) == ' ') {
			*p = '\0';
			break;
		}
	++s;

	strcpy(c, s);

	return;
}

/* HACK; fix the switch handling in newPlayer */
static char switches[1024];

main(int argc, char **argv)
{

	struct sockaddr_in sin;
	int       on = 1;
	int       i, news;
	char      name[64];
	char      display[64];
	char      *sargv[2];
	Display   *dpy;
	FILE      *splash_file;

	strcpy( display, "" );
	strcpy( name, "" );

#ifdef MALLOC_DEBUG
	mcheck((char *) NULL);
#endif

	curTime = 0.0;

	real_delta_t = 1;

	depth_cue_steps = 8;

	/*
	 * An endGameThreshold of -1.0 means "use the radar lock range for
	 * the current aircraft.
	 */

	end_game_threshold_meters = -1.0;
	end_game_mode = 0;

	/*
	 *  When accepting control of an entity, the default is to use our
	 *  site ID and keep everything else the same.
	 */

	transferEntityIdBits = 0x4;

	/*
	 * these only apply to non-real time updating
	 */

	update_interval_millisec = 1000.0 / 50.0;  /* 50.0 hertz */
	frame_interval_millisec = 1000.0 / 15.0;   /* 15.0 hertz */

	watch_frame_rate = 0;

	droneAggressiveness = DEFAULT_DRONE_FACTOR;

#ifdef HAVE_DIS
	disAbsoluteTime = 0;
#endif

	ptblCount = ctblCount = 0;
	visibility = FEETtoMETERS(50.0 * NM);

	/*
	 *  Define handler for DIS transfer control requests
	 */

	dis_setTransferControlRequestCallback ( transferControlRequestHandler );

/*
 *  parse arguments
 */

	switches[0] = '\0';

	processCommandSwitches (argc, argv, display, name, switches);

#ifdef HAVE_DIS
	{

/*
 *  If the user has not specified a DIS application ID and has not referred
 *  us to a SIM/x server, automatically generate an application id based
 *  on the low-order 16-bits of this host's IP address.  This is a bit of a
 *  hack, but it will assure that we get a unique application id so long as we
 *  are not using any sort of DIS bridge software.
 */

		char      name[256], *p;
		struct hostent *h;

		gethostname(name, sizeof(name));

		if (dis_application == DIS_APPLICATION) {
			if ((h = gethostbyname(name)) != (struct hostent *) NULL) {
				p = h->h_addr;
				dis_application = (p[2] << 8) | p[3];
			}
		}
	}

	if (real_delta_t == 0) {

		redraw_interval = (int) 
			(frame_interval_millisec / update_interval_millisec + 0.5);
		/*
		 *  Set time intervals.
		 */

		deltaT = update_interval_millisec / 1000.0;

		halfDeltaTSquared = 0.5 * deltaT * deltaT;

	}

	if (disInit() == 0)
		disInUse = 1;
	else
		disInUse = 0;
#endif

	if (strlen(display) == 0) {
		if (getenv("DISPLAY")) {
			strncpy(display, getenv("DISPLAY"), sizeof(display));
		}
		else {
			fprintf(stderr, "Excuse me, but you have no DISPLAY.\n"
					   "How do you use X, anyway?\n");
			exit(1);
		}
	}

	printf("\
ACM version %s,  Copyright (C) 1991-1998   Riley Rainey (rrainey@ix.netcom.com)\n\n\
ACM comes with ABSOLUTELY NO WARRANTY.\n\
This is free software, and you are welcome to distribute it under the\n\
conditions described in the COPYING file.\n\n", REVISION_STRING);

#ifdef HAVE_DIS
	if (disInUse)
		printf("DIS protocol active.\n\n");
#endif

#if defined(NETAUDIO)
	printf("This ACM program was built with sound support (netaudio).\n\n");
#else
#if defined(_HPUX_SOURCE)
	printf("This ACM program was built with sound support (HP AAPI).\n\n");
#endif
#endif

	init(".");

	dpy = XOpenDisplay(display);
	splash_file = acm_fopen ("acmlogo-unix.xwd", "r");

	sargv[0] = "acm";
	sargv[1] = NULL;

	splashScreen(dpy, splash_file, 1, sargv);

	news = 2;

	if (newPlayer(news, display, name, switches) == 0) {
	}

	input();
#ifdef LINT
	return 0;
#endif

}

#ifdef HAVE_STRUCT_SIGACTION
static struct sigaction alrm;
#else
static struct sigvec    alrm;
#endif

int       doUpdate = 0;

acm_sig_t
myalarm(int s)
{
	doUpdate++;
#ifdef HAVE_STRUCT_SIGACTION
	sigaction(SIGALRM, &alrm, (struct sigaction *) 0);
#else
	sigvec   (SIGALRM, &alrm, (struct sigvec *)    0);
#endif
}

acm_sig_t
killed(int i)
{
	printf("\ninterrupt\n");
	shutdown(listen_socket, 2);
	close(listen_socket);
#ifdef HAVE_DIS
	dis_close();
#endif
	exit(0);
}

input(void)
{

	fd_set    fdset, ofdset;
	int       news = -1, playerchange = 0, n, pno, addrlen;
	int       on = 1;
	struct sockaddr addr;
	struct itimerval update;
	char     *bp, buf[128], name[64], display[64], args[256];
	struct timeval zero_timeout, update_timeout;

	signal(SIGINT, killed);
	signal(SIGQUIT, killed);

	zero_timeout.tv_sec = 0;
	zero_timeout.tv_usec = 0;
	update_timeout.tv_sec = 0;
	update_timeout.tv_usec = (long) (update_interval_millisec * 1000);

	/*
	 *  If we aren't supposed to be running full-bore, then setup the
	 *  real time interval clock.
	 */

	if (real_delta_t == 0) {

#ifdef HAVE_STRUCT_SIGACTION
		alrm.sa_handler = myalarm;
		sigemptyset( &alrm.sa_mask );
		alrm.sa_flags = 0;
		sigaction( SIGALRM, &alrm, (struct sigaction *) 0 );
#else
		alrm.sv_handler = myalarm;
		sigemptyset( &alrm.sv_mask );
#ifdef __hpux
		alrm.sv_flags = SV_BSDSIG;
#else
		alrm.sv_flags = SV_INTERRUPT;
#endif
		sigvec(SIGALRM, &alrm, (struct sigvec *) 0);
#endif

		/*
		 *  Set real time clock to interrupt us at the appropriate interval
		 */

		update.it_interval.tv_sec = 0;
		update.it_interval.tv_usec = (long) (update_interval_millisec * 1000);
		update.it_value.tv_sec = 0;
		update.it_value.tv_usec = (long) (update_interval_millisec * 1000);

		setitimer(ITIMER_REAL, &update, 0);

	}

	FD_ZERO(&ofdset);
	FD_ZERO(&fdset);
	FD_SET(listen_socket, &ofdset);

	for (;;) {

#ifdef SVR4
		(void) sigprocmask(SIG_SETMASK, &sigset_omask, NULL);
#else
		sigsetmask(0);
#endif							/* SVR4 */

		fdset = ofdset;

		if (real_delta_t == 0) {
			pno = select(32, &fdset, (fd_set *) NULL, (fd_set *) NULL,
						 (struct timeval *) NULL);
		}
		else {
			if (ptblCount == 0) {
				pno = select(32, &fdset, (fd_set *) NULL, (fd_set *) NULL,
							 &update_timeout);
			}
			else {
			pno = select(32, &fdset, (fd_set *) NULL, (fd_set *) NULL,
						 &zero_timeout);
			}
			doUpdate++;
		}

#ifdef SVR4
		(void) sigemptyset(&sigset_mask);
		(void) sigaddset(&sigset_mask, SIGALRM);
		(void) sigprocmask(SIG_BLOCK, &sigset_mask, &sigset_omask);
#else
		sigblock(sigmask(SIGALRM));
#endif							/* SVR4 */

		if (pno < 0) {
			FD_CLR(listen_socket, &fdset);
			if (news > 0)
				FD_CLR(news, &fdset);
		}
		if (doUpdate) {
			doUpdate = 0;
			redraw();
		}
	}
}

int
processCommandFile(const char *fname, 
				   int *argc, 
				   char *argv[],
				   char *display, 
				   char *name,
				   char *switches)
{
	FILE *f;
	char commands[4096];
	int  i, c;

	f = fopen (fname, "r");
	if (f == NULL) {
		return -1;
	}

	i = 0;
	while ((c = fgetc(f)) != EOF) {
		if (i >= sizeof(commands)) {
			return -2;
		}
		if (c == '\n') {
			c = ' ';
		}
		commands[i++] = c;
	}
	commands[i] = '\0';

	split (commands, argc, &argv[1]);

	processCommandSwitches ( *argc, argv, display, name, switches);

	return 0;
}

int
processCommandSwitches (int argc, char *argv[], 
						char *display, 
						char *name,
						char *switches)
{
	int i;
	int xargc;
	char *xargv[128];

	for (i = 1; i < argc; ++i) {

#ifdef HAVE_DIS
		if (strcmp(argv[i], "-simx") == 0) {
			dis_site = 0;
			dis_application = 0;
		}
		else if (strcmp(argv[i], "-dis-site") == 0 && ++i < argc) {
			dis_site = strtol(argv[i], (char **) NULL, 0);
		}
		else if (strcmp(argv[i], "-dis-appl") == 0 && ++i < argc) {
			dis_application = strtol(argv[i], (char **) NULL, 0);
		}
		else if (strcmp(argv[i], "-dis-exercise") == 0 && ++i < argc) {
			dis_exercise = strtol(argv[i], (char **) NULL, 0);
		}
		else if (strcmp(argv[i], "-dis-absolute-time") == 0) {
			disAbsoluteTime = 1;
		}
		else if (strncmp(argv[i], "-dis", 4) == 0) {
			fprintf(stderr, "Acm DIS arguments:\n"
					" -dis-exercise <exercise number> (default %d)\n"
					" -dis-site <site number> (default %d)\n"
					" -dis-appl <application number> (default %d)\n"
					" -dis-absolute-time\n",
					DIS_EXERCISE, DIS_SITE, DIS_APPLICATION);
			exit(1);
		}
		else
#endif
		if (strcmp(argv[i], "-init") == 0 && ++i < argc) {
		    processCommandFile (argv[i], &xargc, xargv, 
								display, name, switches);
		}
		else if (strcmp(argv[i], "-arcade") == 0) {
			arcadeMode = 1;
		}
		else if (strcmp(argv[i], "-depth_steps") == 0 && ++i < argc) {
			depth_cue_steps = atoi(argv[i]);
			if (depth_cue_steps < 1) {
				depth_cue_steps = 1;
			}
		}
		else if (strcmp(argv[i], "-da") == 0 && ++i < argc) {
			droneAggressiveness = atof(argv[i]) * NM;
			if (droneAggressiveness <= MIN_DRONE_FACTOR) {
				droneAggressiveness = MIN_DRONE_FACTOR;
			}
			else if (droneAggressiveness > 1.0) {
				droneAggressiveness = 1.0;
			}
		}
		else if (strcmp(argv[i], "-display") == 0 && ++i < argc) {
			strncpy( display, argv[i], 64 );
		}
		else if (strcmp(argv[i], "-geometry") == 0 && ++i < argc) {
			sprintf(&switches[strlen(switches)], "|-geometry|%s", argv[i]);
		}
		else if (strcmp(argv[i], "-js") == 0) {
			if (argv[i + 1] && *(argv[i + 1]) != '-') {
				SetJoystickPort(argv[++i]);
			}
			else {
				SetJoystickPort("/dev/cua0");
			}
		}
		else if (strcmp(argv[i], "-name") == 0 && ++i < argc) {
			strncpy(name, argv[i], 64);
		}
		else if (strcmp(argv[i], "-plane") == 0 && ++i < argc) {
			sprintf(&switches[strlen(switches)], "|-plane|%s", argv[i]);
		}
		else if (strcmp(argv[i], "-scene") == 0 && ++i < argc) {
			sceneFile = argv[i];
		}
		else if (strcmp(argv[i], "-stealth") == 0) {
			sprintf(&switches[strlen(switches)], "|-stealth");
		}
		else if (strcmp(argv[i], "-end-game") == 0) {
			sprintf(&switches[strlen(switches)], "|-end-game");
		}
		else if (strcmp(argv[i], "-threshold-range") == 0 && ++i < argc) {
			double end_game_threshold_nm;

			end_game_threshold_nm = atof( argv[i] );
			if (end_game_threshold_nm < 1.0) {
				end_game_threshold_nm = 1.0;
			}
			end_game_threshold_meters = FEETtoMETERS(end_game_threshold_nm*NM);
		}
		else if (strcmp(argv[i], "-control") == 0) {
			sprintf(&switches[strlen(switches)], "|-stealth");
			/* and, yes, that really should read "|-stealth" */
		}
		else if (strcmp(argv[i], "-team") == 0 && ++i < argc) {
			sprintf(&switches[strlen(switches)], "|-team|%s", argv[i]);
		}
		else if (strcmp(argv[i], "-latitude") == 0 && ++i < argc) {
			sprintf(&switches[strlen(switches)], "|-altitude|%s", argv[i]);
		}
		else if (strcmp(argv[i], "-longitude") == 0 && ++i < argc) {
			sprintf(&switches[strlen(switches)], "|-altitude|%s", argv[i]);
		}
		else if (strcmp(argv[i], "-altitude") == 0 && ++i < argc) {
			sprintf(&switches[strlen(switches)], "|-altitude|%s", argv[i]);
		}
		else if (strcmp(argv[i], "-heading") == 0 && ++i < argc) {
			sprintf(&switches[strlen(switches)], "|-heading|%s", argv[i]);
		}
		else if (strcmp(argv[i], "-airspeed-kts") == 0 && ++i < argc) {
			sprintf(&switches[strlen(switches)], "|-airspeed-kts|%s", argv[i]);
		}
		else if (strcmp(argv[i], "-visibility") == 0 && ++i < argc) {
			visibility = atof(argv[i]);
			if (visibility < 1.0) {
				visibility = 1.0;
			}
			else if (visibility > 500.0) {
				visibility = 500.0;
			}
			visibility *= NM;
			visibility = FEETtoMETERS(visibility);
		}
		else if (strcmp(argv[i], "-frame-rate") == 0 && ++i < argc) {
		    double val = atof(argv[i]);
			frame_interval_millisec = 1000.0 / val;

			real_delta_t = 0;
	    }
		else if (strcmp(argv[i], "-update-rate") == 0 && ++i < argc) {
            double val = atof(argv[i]);
			update_interval_millisec = 1000.0 / val;
			real_delta_t = 0;
	    }
		else if (strcmp(argv[i], "-watch-frame-rate") == 0) {
			watch_frame_rate = 1;
	    }
		else if (strcmp(argv[i], "-transfer-entity-mode") == 0 && ++i < argc) {
			transferEntityIdBits = strtol ( argv[i], NULL, 0 );
		}
		else if (strcmp(argv[i], "-subject-entity-id") == 0 && ++i < argc) {
			dis_entity_id id;
			if (DISParseEntityID ( &id, argv[i], 
								   strlen(argv[i])+1, ":/." ) == 0) {
				subjectEntityID = id;
				subjectEntitySpecified = 1;
			}
			else {
				fprintf(stderr, "Invalid entity ID \"%s\"\n", argv[i]);
				exit (1);
			}
		}
		else {
			fprintf(stderr, "Invalid switch \"%s\"\n", argv[i]);
			exit(1);
			break;
		}
	}
}
