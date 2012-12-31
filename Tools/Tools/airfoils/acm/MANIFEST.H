
/*
 *  acm : an aerial combat simulator for X
 *  Copyright (C) 1991-1996  Riley Rainey
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

/*
 *  PLATFORM-SPECIFIC VALUES
 */

#ifdef WIN32
#define printf acm_printf
#define fprintf acm_fprintf
#endif

#if !defined(HAVE_COPYSIGN)
#define NEEDS_COPYSIGN
#endif

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#ifdef __hpux
#define sigvec sigvector
#endif							/* hpux */

#if defined(HAVE_STDLIB_H)
#include <stdlib.h>
#else
#if defined(HAVE_MALLOC_H)
#include <malloc.h>
#endif
#endif

/*
 *   GENERAL VALUES
 */

/*
 *  Maximum number of surface objects
 */

#ifndef MAXSURFACE
#define MAXSURFACE	128
#endif

/*
 *  Maximum number of players and drones
 */

#ifndef MAXPLAYERS
#define MAXPLAYERS	32
#endif

/*
 *  Maximum number of observers
 */

#ifndef MAXOBSERVERS
#define MAXOBSERVERS    1
#endif

/*
 *  Maximum number of chasers
 */

#ifndef MAXCHASERS
#define MAXCHASERS      1
#endif

/*
 *  Maximum number of missiles and cannon streams
 */

#ifndef MAXPROJECTILES
#define MAXPROJECTILES	(MAXPLAYERS * 8)
#endif

#ifndef MAXEXPLOSIONS
#define MAXEXPLOSIONS	MAXPROJECTILES
#endif

#ifndef MAXCRAFTTYPES
#define MAXCRAFTTYPES	128
#endif

#ifndef STATIONS
#define STATIONS	9
#endif

#ifndef WEAPONTYPES
#define WEAPONTYPES	4
#endif

/*
 *  We'll check every now and again to see if any aircraft are waiting to
 *  be resupplied.  RESUPPLY_EVERY defines the number of seconds
 *  between each check (30 seconds, here).
 */

#ifndef RESUPPLY_INTERVAL
#define RESUPPLY_INTERVAL	((double) 30)
#endif

/*
 *  The default exercise id and site id to use for DIS.
 *  These are overrriden on the acm command line with the "-dis-xxx"
 *  options.
 *  (Note that the default DIS application id of zero means that acm
 *   will choose an application id from the host address that have
 *   a high probability of beeing unique.)
 */

#define DIS_EXERCISE		1
#define DIS_SITE		17
#define DIS_APPLICATION		0

/*
 *  The maximum allowed dead reckoning errors for DIS.
 *  (Note: the units are meters and radians.)
 */

#define DIS_LOCATION_THRESHOLD		3.0
#define DIS_ORIENTATION_THRESHOLD	(2*3.141593/180.0)

/*
 *  ACM becomes the name used to lookup X resources on a particular
 *  X server.
 */

#ifndef ACM
#define ACM			"acm"
#endif

/*
 *  Linear response to control stick inputs are closer to reality, but can 
 *  make the plane harder to fly.
 */

/* #define LINEAR_CONTROL_RESPONSE */

/*
 *  NO_FUZZ removes the "no response" area around center stick position.
 */
 
/* #define NO_FUZZ */
     
/*
 *  We have code in-place to perform flight debugging.  Define this to
 *  compile in that code.
 */

/* #define FLIGHTDEBUG */

/*
 *  When on the ground, we won't allow planes to stray too far from their
 *  team's airport.  MAX_GROUND_DISTANCE defines that distance.
 */

#ifndef MAX_GROUND_DISTANCE
#define MAX_GROUND_DISTANCE	(3.0 * NM)
#endif

/*
 *  Define a library directory to contain the acm files
 */

#ifndef ACM_LIBRARY
#define ACM_LIBRARY		"/usr/local/games/acm/"
#endif

/*
 *  SRCDIR is the toplevel directory of the source tree
 */

#ifndef SRCDIR
#define SRCDIR	".."
#endif

/*
 *  a signal handler function type
 */

#ifdef RETSIGTYPE
#define acm_sig_t       RETSIGTYPE
#else
#ifdef SIGVOID
#define acm_sig_t	void		/* System V returns void */
#else
#define acm_sig_t	int			/* BSD does int's */
#endif
#endif

/*
 *  I have defined some special purpose, debugging keys;  define
 *  SPECIAL_KEYS to enable their use.
 */

#define SPECIAL_KEYS

/*
 *  If you want the simulation clock to run closer to real-time,
 *  define REAL_DELTA_T.  In this mode the master update loop will tweak
 *  the time interval (deltaT) based on the amount of real-time that the
 *  last update took.  A drawback to using this mode is that back-box
 *  input/output works, but creates bogus output -- the current black-box
 *  code assumes that the time interval between samples is fixed.
 *
 *  Defining WATCH_FRAME_RATE causes statistics to be printed periodically
 *  at the acms server's tty.
 */

/* #define WATCH_FRAME_RATE */

/*
 *  MAX_GAME_IDLE_SECONDS defines the amount of time that a server is allowed
 *  to remain idle before it should exit.
 */

#define MAX_GAME_IDLE_SECONDS	0.0

/*
 *  DEFAULT_DRONE_FACTOR controls how hard drones are allowed to maneuver.
 *  Values vary from 0.0 to 1.0.
 */

#define DEFAULT_DRONE_FACTOR	0.5

/*
 *  MIN_DRONE_FACTOR sets the minimum reasonable drone maneuvering factor.
 */

#define MIN_DRONE_FACTOR 	0.05

/*
 *  MAG_VAR sets the magnetic heading offset from true heading.
 */

#define MAG_VAR			0.0

/*
 *  CLOUD_BASE and CLOUD_TOP sets the altitude of base and top of
 *  the cloud layer in feet. If base == top, no clouds are used.
 */

#define CLOUD_BASE		0
#define CLOUD_TOP		0
