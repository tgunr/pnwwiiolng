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

#ifndef _pm_h
#define _pm_h

#include <manifest.h>

#include <scale.h>
#include <damage.h>
#include <interpolate.h>
#include <imath.h>
#include <Vlib.h>
#if defined(NETAUDIO)
#include <audio/audiolib.h>
#else
#if defined(HPAUDIO)
#include <audio/Alib.h>
#else
typedef char *AuServer;
typedef char AuBucketID;
typedef char AuFlowID;
typedef char AuDeviceID;

#endif
#endif

/*
 * eventually we will include gv_sys.h
 */
#define HAVE_GVS 1
#define GVS_OBI void *
#define GVS_OBD void *

struct _craft;

#ifdef HAVE_DIS
#include <dis/dis.h>
#else

/*
 *  We need this typedef to store information from the inventory file,
 *  even if we are not using DIS.
 */

struct dis_entity_type {
	unsigned char kind;
	unsigned char domain;
	unsigned short country;
	unsigned char category;
	unsigned char subcategory;
	unsigned char specific;
	unsigned char extra;
};
typedef struct dis_entity_type dis_entity_type;

#endif							/* HAVE_DIS */


#ifdef FLAT_WORLD
typedef struct _worldcoordinates {
	double    latitude;			/* [radians, north positive] */
	double    longitude;		/* [radians, east positive] */
	double    z;				/* above reference ellipsoid [meters] */
} WorldCoordinates;

#endif

#ifdef AFDS
#include <fplan.h>
#endif

/*
 *  redraw.c contains some simple procedures to help minimize
 *  redrawing panel items.
 */

typedef struct _drawnItem {
	struct _drawnItem *next;
	int       redraw;
} drawnItem;

typedef struct _drawnItemController {
	drawnItem *head;
} drawnItemController;

#ifdef WIN32
typedef BOOL _BOOL;
#else
typedef int _BOOL;
#endif

#ifndef TRUE
#define TRUE	1
#define FALSE	0
#endif

#include <navaid.h>

/*
 *  Pre-defined sounds that can be played for the user
 */
#define SoundEngine			0
#define SoundCrash		    1
#define SoundGearUp		    2
#define SoundGearDown		3
#define SoundMissileLaunch	4
#define SoundCannonFiring	5
#define SoundExplosion		6
#define SoundTouchdown		7
#define SoundStallWarning	8
#define SoundLockWarning	9
#define SoundAPGLockAcquired 10

#define NUM_SOUNDS		11

/*
 *  Each rendering window has a _viewer structure which may be in one
 *  of several states.
 */

#define ViewerStateNormal         0  /* just a plane that we control */
#define ViewerStateBrowsing       1  /* not yet associated with an entity */
#define ViewerStatePiggyback      2  /* a stealth glued to an entity */
#define ViewerStateControlPending 3  /* requested control of an entity */
#define ViewerStateControlling    4  /* have control of another entity */

typedef struct _viewer {
	struct _viewer *next;

	/*
	 *  viewers used to be tied completely to a given aircraft; this is
	 *  no longer the case.  The state value is used to track relocation
	 *  from one entity to the next.
	 */

	int viewer_state;

	/*
	 *   we'll keep a doubly linked list of all active rendering
	 *   windows (viewers) 
	 */

	struct _viewer *vl_next;
	struct _viewer *vl_prev;

	struct _craft *c;            /* back pointer to associated craft */
	struct _craft *watchedCraft;/* back pointer to associated craft, 
									when in stealth mode */


	Display  *dpy;				/* display for this user */
#if defined(HPAUDIO)
	Audio    *aserver;			/* audio server for this user */
	SBucket  *sound[NUM_SOUNDS];
	ATransID  flow[NUM_SOUNDS];
#else
/*
 *  These fields should be defined even if no audio driver is present
 */

	AuServer *aserver;			/* audio server for this user */
	AuBucketID sound[NUM_SOUNDS];
	AuFlowID  flow[NUM_SOUNDS];
	AuDeviceID audioOutput[1];	/* audio output device ID */
#endif
#ifdef WIN32
	Window    win;				/* window for this user */
	GC        gc;				/* GC for drawing */
	GC        gauge_gc;			/* GC for RPM, TEWS, etc */
#else
	Window    win;				/* window for this user */
	GC        gc;				/* GC for drawing */
	GC        gauge_gc;			/* GC for RPM, TEWS, etc */

	XFontStruct *font;			/* HUD font */
	XFontStruct *rfont;			/* radar font */

	Pixmap    eng;				/* Engine "RPM" gauge */
	Pixmap    flap[5];			/* flap setting indicators */
	Pixmap    handle[2];		/* ldg gear handle indicators */
	Pixmap    gearLight[3];		/* ldg gear status lights */
	Atom      protocolsAtom;
	Atom      deleteWindowAtom;
	Atom      closedownAtom;
	int       cn;				/* X connection fd */
#endif
	Viewport *v;				/* Viewport for out-of-cockpit views */
	int       width, height;	/* width & height of viewing window */
	int       xCenter, yCenter;	/* center of viewing window */
	int       rftw, rfth;		/* radar font width & height */
	short     rx, ry;			/* radar x and y location */
	int       radarWidth;		/* width of radar screen (pixels) */
	int       radarHeight;		/* height of radar screen (pixels) */
	int       lastRPM;			/* last RPM fraction x 1000(0 .. 1000) */
	drawnItemController drawControl;
	drawnItem rpmState;
	drawnItem flapState;
	drawnItem annunciatorState;
	drawnItem fuelState;
	drawnItem consumpState;
	drawnItem gearState;
	drawnItem radarState;
	scaleControl altScale;		/* altitude scale control */
	scaleControl velScale;		/* airspeed scale control */
	scaleControl hdgScale;		/* heading scale control */
	int       TEWSx, TEWSy;		/* location of center of TEWS display */
	int       TEWSSize;			/* size of TEWS display (pixels) */
	int       panelx, panely;	/* location of top/right corner */
	double    scaleFactor;		/* overall scaling factor for display */
	ZInfo     z;				/* depth/color information for HUD */
	ZInfo     rz;				/* depth/color information for radar */
	Segment   radarImage[1024];	/* the last radar frame */
	int       radarImageCount;
	int       browseBase;       /* first drawn DIS entity in RM_DIS_BROWSE */
	int       browseSelectedItem;
	unsigned long browseClickTime;
	VPoint    viewDirection;
	VPoint    viewUp;
} viewer;

typedef struct _wctl {
	char     *type;				/* type of weapon at this station */
	int       info;				/* extra info (weapon specific) */
	int       info2;			/* "      " */
	int       info3;			/* "      " */
	double    info4;			/* time offset to next round firing (sec) */
} weaponStation;

/*
 *  This structure describes a class of aircraft (e.g. an F-16C).
 */

typedef struct _craftType {
	char     *name;				/* short name of aircraft class */
	char     *description;		/* long name */

	char	 *modelname;        /* compiled object model to drive this aircraft type */

	double    aspectRatio;		/* wing aspect ratio */

	double    CLOrigin, CLSlope;	/* Defines the CL characteristic eqn */
	double    CLNegStall, CLPosStall;
	double    betaStall;		/* Stall angle for rudder */
	double    CDOrigin, CDFactor;	/* Defines the CD Characteristic eqn */
	ITable   *CLift;			/* compute lift coefficent */
	ITable   *CDb;				/* compute body + wave drag coeff */
	ITable   *CnBeta;			/* compute yaw moment due to sideslip */
	ITable   *ClBeta;			/* compute roll moment due to sideslip */
	double    CDBOrigin, CDBFactor;		/* Defines the CD Characteristic eqn */
	double    CDBPhase;
	double    CYbeta;			/* Side-force from side-slip (dCY/dBeta) */
	double    Clda;				/* roll moment from aileron offset */
	double    Cldr;				/* roll moment from rudder offset */
	double    Clp;				/* roll damping */

	double    Cmq;				/* damping in pitch */
	double    Cnr;				/* damping in yaw */

	double    maxAileron;		/* maximum aileron offset */
	double    maxRudder;		/* maximum rudder offset */

	double    effElevator;		/* Elevator effectiveness */
	double    effRudder;		/* Rudder effectiveness */
	double    SeTrimTakeoff;	/* Takeoff elevator trim setting */

	VMatrix   I;				/* Moments of Inertia about CG in [xyz] */
	double    cmSlope;			/* CmAlpha curve slope */
	double    cmFactor;			/* CmAlpha factor when stalled */

	double    maxFlap;			/* maximum flap setting (radians) */
	double    cFlap;			/* lift coefficient of flaps */
	double    cFlapDrag;		/* drag coefficient of lowered flaps */
	double    flapRate;			/* flap movement rate (radians/sec) */

	double    cGearDrag;		/* drag coefficient of lowered gear */
	double    gearRate;			/* landging gear movement rate (rad/sec) */

	double    maxSpeedBrake;	/* maximum speed brake setting (radians) */
	double    cSpeedBrake;		/* drag coefficient of 90 degree speed brake */
	double    speedBrakeRate;	/* rate of speed brake movement (radians/sec) */
	double    speedBrakeIncr;	/* number of radians than one keystroke moves brake */

	double    wingS;			/* wing area (ft^2) */
	double    wings;			/* wing half-span (ft) */
	double    c;				/* mean aerodynamic chord (MAC) (ft) */
	double    emptyWeight;		/* empty weight (lbs.) */
	double    maxFuel;			/* maximum internal fuel (lbs.) */

	double    maxThrust;		/* max static thrust, military power (lb) */
	double    maxABThrust;		/* max static thrust, afterburner on  (lb) */
	double    (*thrust) (struct _craft *);	/* computes current thrust */
	ITable   *Thrust;			/* Change in thrust due to mach number */
	ITable   *ABThrust;			/* Change in thrust due to mach number */
	double    engineLag;		/* controls lag between throttle and RPM */
	double    spFuelConsump;	/* specific fuel consump(lb fuel/lb T x hr) */
	double    spABFuelConsump;
	VPoint    groundingPoint;	/* hypothetical single pt of contact w/ground */
	VPoint    viewPoint;		/* pilot's viewing location wrt CG */

	double    muStatic;			/* static coefficient of friction no-brakes */
	double    muKinetic;		/* moving coefficient of friction no-brakes */
	double    muBStatic;		/* static brakes-on */
	double    muBKinetic;		/* moving brakes-on */

	double    maxNWDef;			/* maximum nosewheel deflection (radians) */
	double    NWIncr;			/* deflection for each unit */
	double    maxNWS;			/* maximum NWS velocity */
	double    gearD1;			/* x station wrt nose gear of main gear */
	double    gearD2;			/* x station wrt CG of main gear */
	VPoint    rm, rn;			/* location if main/nose gear attachments */
	double    Dm, Dn;			/* main/nose oleo damping factor */
	double    Km, Kn;			/* main/nose oleo spring factor */
	double    Gm, Gn;			/* main/nose strut length with tire */
	double    cmMax, cnMax;		/* main/nose maximum oleo extension distance */
	VPoint    tailExtent;		/* as we rotate, this part may drag */
	double    armDelay;			/* arming delay for missiles */

	long      damageBits;		/* initial bit mask of damaged systems */
	long      structurePts;		/* maximum structural damage */

	double    radarOutput;		/* radar output (watts) */
	double    radarTRange;		/* tracking (lock) radar range (nm) */
	double    radarDRange;		/* detection radar range (nm) */
	double    TEWSThreshold;

	long      sCount;			/* number of weapon stations */
	VPoint    wStation[STATIONS];	/* weapon's stations (launch points) */
	weaponStation station[STATIONS];	/* whatcan be at each weapon station */

	int       (*placeProc) (Viewport *, struct _craft *, VMatrix *, VPolygon **, long *);	/* object placement procedure (for special craft) */
	char     *objname;			/* name of file containing the object */
	VObject  *object;			/* what it looks like */
	void      (*resupply) (struct _craft *);	/* the plane's rearm & refuel procedure */
	dis_entity_type entityType;	/* craft type used in DIS */
	dis_entity_type altEntityType;	/* alternate craft type used in DIS */

#ifdef HAVE_GVS
	GVS_OBD gvs_object;
#endif

} craftType;

typedef struct _wdsc {
	int       mask;				/* key mask */
	int       (*select) (struct _craft *);	/* weapon select procedure */
	int       (*update) (struct _craft *);	/* per tick update procedure */
	int       (*display) (struct _craft *, craftType *, viewer *, int *, int *);	/* display update procedure */
	int       (*firePress) (struct _craft *);	/* fire button pressed procedure */
	int       (*fireRelease) (struct _craft *);		/* fire button released */
	craftType *w;				/* description of this weapon */
} weaponDesc;

#define WK_M61A1	0x01		/* M-61A1 Vulcan 20mm cannon */
#define WK_AIM9M	0x02		/* Sidewinder missile */
#define WK_MK82     0x04		/* Mk 82 (500 lb) gravity bomb */
#define WK_AGM65    0x08
#define WK_AIM120   0x10

/*
 *  Radar tracking information
 */

typedef struct _radarInfo {
	int       beamID;			/* index number of beam */
	VPoint    rel;				/* location relative to radar set */
	double    d;				/* distance to target (feet) */
	int       targetID;			/* craft id of target */
	int       locked;			/* 1=we have a "lock" */
	int       altDisplay_kft;   /* altitude readout (feet x 1000) */
	int		  headingDisplay_deg; /* ground track of target */
	int       x, y;
} radarInfo;

/*
 *  This structure describes a particular instance of aircraft.
 */

typedef struct _craft {
	int       pIndex;			/* index number of this element in the vector */
	int       type;				/* craft type */
	int       team;				/* team number, also used for chased plane if this is a chaser */
	double    createTime;		/* creation time of this craft */
	viewer   *vl;				/* list of viewers */
	VMatrix   trihedral;		/* transforms acftXYZ to NED [north-east-down] */
	/* x = forward, y=right wing, z=down */
	VPoint    Cg;				/* Velocity vector in NED system [ft/sec] */
	double	  VT;				/* airspeed [ft/sec] */
	VPoint    Sg;				/* Position in Geocentric system [meters] */
	WorldCoordinates w;			/* The "real" (geodetic) position */
	WorldCoordinates prevw;		/* The last "real" position */
	double    localTerrain_feet;
	VMatrix   XYZtoNED;			/* converts geocentric to north-east-down */
	double    rho;				/* current rho value */
	double    mach1;			/* current value of mach 1.0 (fps) */
	double    mach;				/* current mach number */
	VPoint    G;				/* g-force vector in acft system */
	VPoint    linAcc;			/* acceleration in acft system (ft/sec^2) */
	VPoint    prevSg;			/* last interval's Sg value (ft) */
	double    p, q, r;			/* roll, pitch, and yaw rates (rad/sec) */

	double	  pitchComm;		/* pilot's pitch command  (-1.0 .. 1.0) */
	double	  rollComm;			/* pilot's roll command   (-1.0 .. 1.0) */
	double    rudderComm;		/* pilot's rudder command (-1.0 .. 1.0) */
	int	      throttleComm;		/* pilot's throttle command (0 .. 32768) */

	double    Se, Sa, Sr;		/* control surface positions (-1 <= x <= 1) */
	double    SeTrim;			/* pitch trim setting */
	double    SaTrim;			/* roll trim setting */
	double    curHeading, curPitch, curRoll;	/* Euler angles for acft */
	double    curThrust;		/* Current thrust value (lbs) */
	double    curFlap;			/* current flap setting (radians) */
	double    flapSetting;		/* current target flap setting (radians) */
	double    curSpeedBrake;	/* current speed brake position (radians) */
	double    speedBrakeSetting;	/* current target speed brake setting(rad) */
	double    curGear[3];		/* current ldg gear location (0.0 = up) */
	/* [0] - nose, [1] - right, [2] - left */
	int       throttle;			/* thrust setting 0 - 32768 */
	double    rpm;				/* actual engine RPM (0.0 .. 1.0) */
	double    alpha, beta;		/* angles of attack and sideslip (rad) */
	double    fuel;				/* current fuel on board (lbs) */
	unsigned long flags;		/* flag word */

	long      damageBits;		/* bit flags of damaged subsystems */
	long      structurePts;		/* damage pts that can be absorbed */
	double    leakRate;			/* fuel leakage rate (lbs/second) */
	double    damageCL;			/* damage induced roll */
	double    damageCM;			/* damage induced pitch */

	double    groundCgx;		/* groundspeed */

	int       radarMode;		/* radar mode */
	double    nextRadarTime;	/* time of next Radar frame */
	double    curNWDef;			/* Current nosewheel deflection (radians) */
	craftType *cinfo;			/* General craft information */
	double    order;			/* temporary value used to sort craft */
	struct _craft *next;		/* next craft in sorted list */
	VPoint    viewDirection;	/* where the pilot is currently looking */
	VPoint    viewUp;			/* the "up" direction of the pilot's view */

	char      name[32];			/* logname of player */
	char      display[32];		/* display name of player */

	short     curRadarTarget;	/* our primary "threat" */
	double    targetDistance;	/* distance to primary target [feet] */
	double    targetClosure;	/* closure rate on primary target [fps] */
	short     relValid[MAXPLAYERS];
	short     curOpponent;		/* who this drone is trying to kill */
	short     curDroneMode;     /* drone operating mode (DM_ constants ) */
	short     holdCount;		/* non-zero when drones holding fire */
	VPoint    relPos[MAXPLAYERS];
	double    rval[MAXPLAYERS];	/* radar strength seen by us from other craft */
	/* relative positions of possible targets */
	char     *leftHUD[6];		/* strings in lower left corner of HUD */
	/* (reserved for weapons' status */
	char     *rightHUD[6];		/* strings in lower right corner of HUD */
	/* (reserved for future use) */
	int       curWeapon;		/* index of currently selected weapon */
	weaponStation station[STATIONS];	/* what's at each weapon station */
	radio_t   navReceiver[2];
	radio_t  *hsiSelect;		/* pointer to radio being used for hsi */

	char      lastConsump[32];
	char      lastTotal[32];
	char      lastFlap[16];
	char      lastRPM[16];		/* last real rpm percentage */
	char      lastRPMO[16];		/* last ordered rpm percentage */
	radarInfo rinfo[32];		/* radar target information */
	int       rtop;				/* number of entries used in rinfo */
	int		  rrangeScale;      /* normally 5, 10, 20, 40, 80 nm */
	int		  rDesignatorX;		/* x location (pixels) of radar target designator */
	int       rDesignatorY;
	int       rDesignatorState;
#ifdef HAVE_DIS
	int       disId;			/* DIS identity */
	VPoint    disLastCg;		/* last velocity vector (used for local) */
	double    disLastTime;		/* last time (used for local) */
#endif
#ifdef AFDS
	XFlightPlan *fp;				/* current flight plan */
	int		  fpIndex;			/* index of current waypoint in flight plan */

	void	  *flightDirector;	/* flight director state information */
#endif

	void	  *pAutoAcftEntity; /* ptr to AutoAircraftEntity */
	void      *pAcftEntity;		/* ptr to special flight model */

#ifdef HAVE_GVS
	GVS_OBI   gvs_instance;		/* ptr to GVS object instance */
#endif

	VPoint    interceptStartPoint; /* geocentric coordinates of location
									  where an end-game intercept was
									  initiated */

} craft;

/*
 *  This structure describes a particular instance of observers.
 */

typedef struct _observer {
	VPoint    Sg;				/* Position in Geoparallel system */
	VPoint    viewPoint;		/* Actual viewpoint */
	viewer   *vl;				/* list of viewers (actually 1 struct) */

} observer;

/*
 * The entity_object_map is used to map DIS entity types to a
 * displayable object.
 */

typedef struct _entity_object_map {
	dis_entity_type entity_type;
	dis_entity_type entity_mask;
	char *          object_name;
	VObject         *obj;
} entity_object_map;

/*
 * The munition_map is used to map munition entity/warhead combinations
 * to explosions sizes and damage.
 */

typedef struct _munition_map {
	dis_entity_type entity_type;
	dis_entity_type entity_mask;
	u_short         warhead_type;
	u_short         warhead_mask;
	double          explosion_diameter_meters;
	double          damage_factor;
	int             kinetic_flag;
} munition_map;

/*
 *  We'll use some defines to reduce the storage required for the craft
 *  (unions would be cleaner, perhaps).
 */

#define	offset		fuel
#define interval	rpm			/* time interval in this burst */
#define	rounds		curRadarTarget
#define	tracerMod	structurePts
#define tracerVal	radarMode
#define owner		curWeapon
#define duration	radarMode
#define flameDuration	throttle
#define	escale		SeTrim
#define armTimer	curNWDef

/*
 *  Craft type definitions
 */

#define CT_FREE		0			/* an unused craft entry */
#define CT_PLANE	1			/* a player */
#define CT_MISSILE	2			/* an air to air missile */
#define CT_CANNON	3			/* a stream of cannon fire */
#define CT_SURFACE	4			/* surface object (e.g. a runway) */
#define CT_CHASER	5			/* a non-player "watcher" chasing a plane */
#define CT_DRONE	6			/* a target drone */
#define CT_EXPLOSION	7		/* an explosion */
#ifdef HAVE_DIS
#define CT_DIS_PLANE	8		/* external player (on different server) */
#define CT_DIS_MUNITION 9		/* external tracked munition */
#define CT_DIS_CANNON	10		/* external untracked munition (cannon) */
#define CT_RESERVED     11		/* a reserved, uninitialized entry */
#define CT_DIS_STEALTH  12      /* a place-holder for stealths */
#define CT_BOMB         13      /* gravity bomb */
#endif

/*  Flag word definitions */

#define FL_RECORD	    (1<<0)	  /* activate recording function */
#define FL_AFTERBURNER	(1<<1)	  /* afterburner state */
#define FL_BRAKES	    (1<<2)	  /* wheel brake state */
#define	FL_NWS		    (1<<3)	  /* nose-wheel steering mode */
#define FL_HAS_GYRO	    (1<<4)	  /* missile is gyroscope equiped */
#define FL_GHANDLE_DN	(1<<5)	  /* landing gear handle state (1 = DOWN) */
#define FL_GEAR0_UP	    (1<<6)	  /* set when nose gear is completely up */
#define FL_GEAR1_UP	    (1<<7)	  /* set when right gear is completely up */
#define FL_GEAR2_UP	    (1<<8)	  /* set when left gear is completely up */
#define FL_GND_CONTACT	(1<<9)	  /* set when we're touching the ground */
#define FL_FIXED_OBJECT (1<<10)	  /* fixed (surface) object */
#define FL_CHASE_VIEW	(1<<11)	  /* chase plane view */
#define FL_BLACK_BOX	(1<<12)	  /* object is from a black box recording */
#define FL_BALLISTIC	(1<<13)	  /* munition has no target */
#define FL_RADAR_MODE_CHANGE (1<<14) /* set when user changes radar mode */
#define FL_END_GAME_DRONE (1<<15) /* drone in CALSPAN end-game mode */

/* drone modes */

#define DM_ATTACK       0     /* normal drone attack mode */
#define DM_RETURN       1     /* return to drone activation point 
								   (interceptStartPoint) */
#define DM_RETURN_CAPTURED 2  /* in return mode and moving towards return pt */

/* radar mode definitions */

#define RM_OFF		0			/* radar is off */
#define RM_STANDBY	1			/* standby */
#define RM_NORMAL	2			/* track while scan */
#define RM_ACM		3			/* 20x30 acm */
#define RM_STT		4			/* single target track */
#define RM_ILS		5			/* instrument landing system mode */
#define RM_FSPAGE	6			/* flight status page */
#define RM_DIS_BROWSE 7         /* browse entities */

/* outside view types */

#define VIEW_FORWARD	0
#define VIEW_UP		1
#define VIEW_LEFT	2
#define VIEW_RIGHT	3
#define VIEW_AFT	4
#define VIEW_CHASE	5

#ifndef ALLOCATE_SPACE
#define _var_type extern
#else
#define _var_type
#endif

#if defined(__cplusplus)
extern    "C" {
#endif

	_var_type viewer *vl_head;   /* linked list of all rendering windows */
	_var_type viewer *vl_tail;
	_var_type int    vl_count;

	_var_type int recordCount;	/* number of players recording info */
	_var_type int ptblCount;	/* number of active entries in ptbl */
	_var_type int otblCount;	/* number of entries in otbl        */
	_var_type int ctblCount;	/* number of chasers                */
	_var_type double curTime;	/* current time */

	_var_type craft stbl[MAXSURFACE];	/* table of surface objects */
	_var_type craft ptbl[MAXPLAYERS];	/* table of player aircraft */
	_var_type craft mtbl[MAXPROJECTILES];	/* table of missiles and cannon streams */
	_var_type craft ctbl[MAXPLAYERS];	/* table of chasers */
	_var_type observer otbl[MAXOBSERVERS];	/* table of observers not counted as players */
	_var_type weaponDesc wtbl[WEAPONTYPES];		/* descriptions of different weapons */

#ifdef FLAT_WORLD
	_var_type VPoint teamLoc[3];	/* the centers of each team's airport */
#else
	_var_type WorldCoordinates teamLatLon[3];
#endif
	_var_type double teamHeading[3];	/* heading of plane */

	_var_type double deltaT;	/* Update interval in seconds */
	_var_type double halfDeltaTSquared;		/* 0.5 * deltaT * deltaT */
	_var_type int HUDPixel;		/* index of HUD color in viewport(s) */
	_var_type VColor *HUDColor;
	_var_type int whitePixel;	/* index of white in viewport(s) */
	_var_type VColor *whiteColor;	/* white in viewport(s) */
	_var_type int blackPixel;	/* index of black in viewport(s) */
	_var_type VColor *blackColor;	/* black in viewport(s) */
	_var_type VColor *groundColor;	/* ground color in viewport(s) */
	_var_type VColor *radarColor;	/* radar color */
	_var_type VColor *radarBackgroundColor;		/* radar CRT surface color */
	_var_type VColor *HSIMagentaColor;	/* radar color */
	_var_type VColor *cloudColor;	/* cloud color */
	_var_type int arcadeMode;	/* set by -a switch */
	_var_type int chaseOthers;	/* can one chase other's planes ? */
	_var_type double droneAggressiveness;	/* how hard drones maneuver */
	_var_type double visibility;	/* visibility (feet) */
	_var_type double cbase, ctop;	/* cloud base and tops (feet) */

#ifdef HAVE_DIS
	_var_type int disInUse;		/* true if DIS protocol is used */
	_var_type int disAbsoluteTime;	/* this host have true UTC time */
#endif

    _var_type entity_object_map *eo_map;
    _var_type int eo_map_count;
    _var_type munition_map *mun_map;
    _var_type int mun_map_count;

	_var_type int real_delta_t;    /* 1 when using real-time clock */
	_var_type int redraw_interval; /* render scene every n update frames */
	_var_type double frame_interval_millisec;
	_var_type double update_interval_millisec;
	_var_type int watch_frame_rate;
	_var_type int depth_cue_steps;
	_var_type double end_game_threshold_meters;
	_var_type int end_game_mode;
	_var_type int transferEntityIdBits;
	_var_type dis_entity_id subjectEntityID;
	_var_type int subjectEntitySpecified;     /* =1 if subject entity was
												specfied on the command line */

#undef _var_type

#define earth_g	32.15				/* acceleration due to gravity (fps^2) */
#define pi	3.14159265358979323846
#define mag(v)	(sqrt (v.x * v.x + v.y * v.y + v.z * v.z))

#define NM		6076.115
#define SM		5280.0
#define FPStoMPH(v)	((v) / 5280.0 * 3600.0)
#define FPStoKTS(v)	((v) / 6076.115 * 3600.0)
#define KTStoFPS(v)	((v) * 6076.115 / 3600.0)
#define FEETtoMETERS(v)	((v) * 0.30480060960)
#define METERStoFEET(v)	((v) * 3.2808399)
#define fsign(d)	(d < 0 ? -1.0 : 1.0)

#ifdef WIN32
#define VIEW_WINDOW_HEIGHT	0
#else
#define VIEW_WINDOW_HEIGHT	500
#endif

#define VIEW_WINDOW_WIDTH	1200
#define CHASE_WINDOW_HEIGHT	400
#define CHASE_WINDOW_WIDTH	600

#ifdef WIN32
#define RADAR_WINDOW_WIDTH	450
#define RADAR_WINDOW_HEIGHT	450
#define VISOR_MARGIN		20
#else
#define RADAR_WINDOW_WIDTH	200
#define RADAR_WINDOW_HEIGHT	200
#define VISOR_MARGIN		20
#endif

#define RADAR_X			((VIEW_WINDOW_WIDTH - RADAR_WINDOW_WIDTH)/2)
#define TEWS_X			(RADAR_X - 70)
#define TEWS_SIZE		121
#define FS_WINDOW_WIDTH		(VIEW_WINDOW_WIDTH)
#define FS_WINDOW_HEIGHT (VIEW_WINDOW_HEIGHT+RADAR_WINDOW_HEIGHT+VISOR_MARGIN)
#define	FLAP_X			(RADAR_X - 95)
#define FLAP_Y			(FS_WINDOW_HEIGHT - 53)
#define DESIGNATOR_SIZE		20

/*
 *  Location of the center of the engine RPM gauge.
 */

#define ENG_X			(RADAR_X + RADAR_WINDOW_WIDTH + 20)
#define ENG_Y			(VIEW_WINDOW_HEIGHT+VISOR_MARGIN+RADAR_WINDOW_HEIGHT/2+20)

#define ALT_ORG_X		810
#define ALT_ORG_Y		360
#define ALT_LENGTH		219
#define ALT_ORIENT		orientRight
#define ALT_SCALE		(1530.0 / (double) ALT_LENGTH)
#define ALT_INDEX_SIZE		32
#define ALT_MIN_INTERVAL	100
#define ALT_MIN_SIZE		9
#define ALT_MAJ_INTERVAL	500
#define ALT_MAJ_SIZE		17
#define ALT_DIVISOR		1000.0
#define ALT_FORMAT		"%4.3g"

#define VEL_ORG_X		380
#define VEL_ORG_Y		360
#define VEL_LENGTH		ALT_LENGTH
#define VEL_ORIENT		0
#define VEL_SCALE		(153.0 / (double) VEL_LENGTH)
#define VEL_INDEX_SIZE		32
#define VEL_MIN_INTERVAL	10
#define VEL_MIN_SIZE		9
#define VEL_MAJ_INTERVAL	50
#define VEL_MAJ_SIZE		17
#define VEL_DIVISOR		10.0
#define VEL_FORMAT		"%3.3g"

#define HDG_ORG_X		(VIEW_WINDOW_WIDTH / 2 - (6*33+1) / 2)
#define HDG_LENGTH		(6*33+1)
#define HDG_ORIENT		orientRight		/* really orient TOP */
#define HDG_SCALE		(2727.0 / (double) VEL_LENGTH)
#define HDG_INDEX_SIZE		10
#define HDG_MIN_INTERVAL	500
#define HDG_MIN_SIZE		5
#define HDG_MAJ_INTERVAL	1000
#define HDG_MAJ_SIZE		10
#define HDG_DIVISOR		1000.0
#define HDG_FORMAT		"%2.2g"

#define NC_NOT_PLANE            -1
#define NC_CANNOT_CHASE         -2

	extern double fuelUsed(craft *);

	extern void doViews (void);
	extern void renderCockpitView ( craft *c, viewer *u );

	extern int initializeAudio(craft *, viewer *, char *);
	extern void shutdownAudio(craft *, viewer *);
	extern void playSound(craft *, int);
	extern void playContinuousSound(craft * c, int id);
	extern void stopSound(craft * c, int id);
	extern void setBackgroundSound(craft * c, double dThtlPercent,
								   _BOOL bABFlag, double dDynamicPressure);
	extern char *acm_find_file(char *);

	extern void GenerateWorldToLocalMatrix(WorldCoordinates *, VMatrix *);
	extern void buildEulerMatrix(double roll, double pitch, double heading,
								 VMatrix * m);
	extern void transpose(VMatrix *, VMatrix *);

	extern int redrawItem(drawnItem * p);
	extern _BOOL isRedrawRequired(drawnItem * p);
	extern void initializeDrawnItemController(drawnItemController * c);
	extern void addDrawnItem(drawnItemController * c, drawnItem * p);
	extern void redrawAllItems(drawnItemController * c);

	extern int countOrdinance(craft * c, char *type);
	extern int readyStation(craft * c, char *type);
	extern int selectWeapon(craft * c);
	extern int selectNamedWeapon(craft * c, int id);
	extern int fireWeapon(craft * c);
	extern int ceaseFireWeapon(craft * c);
	extern int doWeaponDisplay(craft * c, viewer * u, int *x, int *y);

	extern void setRadarMode(craft * c, int mode);
	extern int  newRadarTarget(craft *c);
	extern double radarFrameInterval(craft * c);
	extern int bugRadarTarget (craft *c);
	extern void breakRadarLock (craft *c);
	extern void radarScaleUp ( craft *c );
	extern void radarScaleDown ( craft *c );
	extern void shiftRadarCursor ( craft *c, int dx, int dy );
	extern void doDroneRadar ( craft * c );

	extern void radioFrequencyChanged(craft * c, radio_t * r);

	extern double localAltitude(VPoint * vec, WorldCoordinates * wc);
	extern void matrixToEuler(VMatrix * mt, double *heading, 
							  double *pitch, double *roll);

	extern void blackBoxKillPlayer(int);
	extern int killPlayer(craft *);
	extern int killChaser(craft *);
	extern int killPlayerEx(craft *,...);
	extern int newDrone(craft * p, const char *type);

	extern int flapsDown(craft * c);
	extern int flapsUp(craft * c);
	extern int speedBrakeExtend(craft * c);
	extern int speedBrakeRetract(craft * c);

	extern double elevatorSetting(craft * c, double q, double w);
	extern void euler(craft *);

	extern void newExplosion(VPoint * loc, VPoint * vel, double s, 
							 double dur1, double dur2);

	extern int absorbDISDamage(craft * c, 
							   dis_entity_type *etype, 
							   u_short warhead_type,
							   u_short fuze_type,
							   double distance_meters,
							   double vel_meters_per_sec,
							   double *explosion_diameter_meters);

	extern int entityWildcardMatch (const dis_entity_type *in,
									const dis_entity_type *pattern,
									const dis_entity_type *pattern_mask);

	extern void placeCraft(Viewport * v, craft * c, viewer *u,
						   craft * obj, VPolygon ** poly, long *cnt);

	extern craftType *lookupCraft( const char * );
	extern craftType *lookupCraftByEntityType( const dis_entity_type * id );
	extern int newPlane( const char *, int );
	extern craftType * newCraft(void);

	extern void freeCraftTypes (void);
	extern void freeRendering (void);
	extern void freeAll (void);
	extern void freeAllNavaids (void);
	extern void freeEffects(void);

	extern int compileEntityMap( char *name, 
								 int *count, 
								 entity_object_map **pmap );

	extern int compileMunitionMap( char *name, 
								   int *count, 
								   munition_map **pmap );

	extern void addViewer ( viewer *v );
	extern int removeViewer ( viewer *v );
	extern int getViewerCount ( void );

	extern void computeASECircleParameters(craft *c,
						   double * ASE_diameter_millirad,
						   double * ASE_dot_az_millirad,
						   double * ASE_dot_el_millirad);

	extern void doBrowsePage(craft * c, viewer * u);
	extern void selectCockpitItem (craft *c, 
								   viewer * u, 
								   int x, 
								   int y, 
								   unsigned long time );

	extern void computeImpactPoint ( craft *c, craftType *bomb, WorldCoordinates *ip );
	extern int dropOrdinance ( craft *c, int i );

#ifndef WIN32
	extern int doBrowseKeyEvent(craft * c, viewer * u, 
								XEvent * ev, int player);

	extern int splashScreen(Display *dpy,
							FILE *in_file, 
							int argc, 
							char **argv);
#endif

	extern int split (char *s, int *argc, char *argv[]);

	extern FILE     *acm_fopen(char *name, char *access);

	extern ITable * copyITable ( ITable *oldp );
	extern void freeITable ( ITable *oldp );

#include "dis.h"

	extern int transferControlRequestHandler (Entity_t *e, 
										       dis_transfer_control_pdu *pdu );

	extern int stealthCraft ( craft *c, viewer *u, int item, int take_control);
	extern craft * locateCraftByDISEntityID ( dis_entity_id *id );

	extern void updateSimTimeFromSystemClock (void);

#if defined(__cplusplus)
};

#endif

#ifdef WIN32
#include <float.h>
#define isnan _isnan
#endif

#endif
