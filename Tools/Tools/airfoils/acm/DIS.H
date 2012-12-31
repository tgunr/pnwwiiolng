
/*
 * $Id: dis.h,v 1.2 1998/05/12 03:59:37 riley Exp $
 *
 *  Copyright (C) 1995  Mats Lofkvist  CelsiusTech Electronics AB
 *
 *  Additions Copyright (c) 1998 Web Simulations, Inc.
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

#ifndef DIS_H
#define DIS_H

#define DIS_ENTITY_STATE_WAIT		0	/* Created, but non participating */
#define DIS_ENTITY_STATE_SIMULATING	1	/* Actively simulating */
#define DIS_ENTITY_STATE_STOPPED	2	/* Stopped/Frozen */

#define DIS_ENTITY_STATE_NONE		3   /* used in pending state */

/* entity types (obsolete) */
#define DIS_ENTITY_OTHER	0
#define DIS_ENTITY_F16		1
#define DIS_ENTITY_MIG29	2
#define DIS_ENTITY_AIM9M	3

/* detonation types (obsolete) */
#define DIS_DETONATION_OTHER	0
#define DIS_DETONATION_M61A1	1
#define DIS_DETONATION_AIM9M	2

/* fire types */
#define DIS_FIRE_OTHER		0
#define DIS_FIRE_M61A1		1
#define DIS_FIRE_AIM9M		2

#define DIS_ID_NONE		-1

#if defined(__cplusplus)
extern    "C" {
#endif

/*
 *  d i s _ E n t i t y E n t e r C b
 *
 *  Callback used when a new remote entity enter the simulation.
 *  The arguments are the id of the new entity and its type
 *  (one of DIS_ENTITY_XXX).
 */

	typedef void (*dis_EntityEnterCb) (int eid,
					  dis_entity_type * etype, int force, craft ** cptr);

/*
 *  d i s _ D e t o n a t i o n C b
 *
 *  Callback used when a remote detonation occurs.
 *  The arguments are the type of fire detonating (one of DIS_FIRE_XXX),
 *  the id of the firing entity, the id of the target entity,
 *  the time of the detonation and the location of the detonation in
 *  world coordinates and in target body coordinates.
 */

	typedef void (*dis_DetonationCb) (int ftype, 
									  int firingEid, 
									  int targetEid,
									  double time, 
									  double worldLocation[3],
									  double entityLocation[3], 
									  craft * munition,
									  dis_detonation_pdu *dpdu);

/*
 *  d i s _ i n i t
 *
 *  Initialize the DIS library.
 *  The broadcast device used is specified with device (e.g. "le0" on a Sun).
 *  The port for receiving DIS packets is specified with port.
 *     (The port number is currently hardwired to 3000.)
 *  The DIS exercise number, site number and application number are specified
 *  by the corresponding arguments.
 *     (The application number is currently hardwired to a part of the local
 *      hostid. This will ensure uniqe application numbers on a local network.)
 *  User callbacks for entity enter, entity exit, fire and detonation are
 *  specified by the last arguments. NULL callbacks may be used for fire
 *  and detonation.
 *
 *  Zero is returned on success.
 */

	int       dis_init(int exercise, int site, int application,
					   dis_EntityEnterCb entityEnterCb,
					   dis_DetonationCb detonationCb);

/*
 *  d i s _ c l o s e
 *
 *  Close down the DIS library.
 *
 *  Zero is returned on success.
 */

	int       dis_close(void);

/*
 *  d i s _ s e t D R T h r e s h o l d s
 *
 *  Set the dead reckoning thresholds for location and orientation.
 *  The values shall be given in meters and radians (psi, theta, phi).
 *
 *  Zero is returned on success.
 */

	int       dis_setDRThresholds(double location, double orientation);

/*
 *  d i s _ s e t T i m e
 *
 *  Set the current time in the DIS library.
 *
 *  Zero is returned on success.
 */

	int       dis_setTime(double time);

/*
 *  d i s _ s e t T i m e A b s o l u t e
 *
 *  Set the current time in the DIS library using the system time,
 *  and mark the time as absolute, i.e. true UTC time.
 *  This will improve the dead reckoning performance on networks
 *  with significant delays between players _iff_ all hosts have
 *  true UTC time (with millisecond precision!).
 *
 *  If this is used when players do _not_ have synchronized clocks,
 *  the result will be very strange positions for external players!!
 *
 *  Zero is returned on success.
 */

	int       dis_setTimeAbsolute(void);

/*
 *  d i s _ r e c e i v e
 *
 *  Process all available incoming PDU's from the network.
 *  User callbacks will be called for entering entities,
 *  exiting entities, firing entities and detonations.
 *
 *  Zero is returned on success.
 */

	int       dis_receive(void);

/*
 *  d i s _ e n t i t y E n t e r
 *
 *  Enter an entity of type etype (DIS_ENTITY_XXX).
 *  The initial location, velocity, linear acceleration, orientation
 *  and angular velocity will be set from the corresponding arguments.
 *  Velocity and acceleration shall be given in world coordinates.
 *  All parameter units are based on meters, radians and seconds.
 *
 *  The world coordinate system used in DIS is GCC (geocentric cartesian
 *  coordinates), an earth-centered right-handed Cartesian system with
 *  the positive X-axis passing through the Prime Meridian at the Equator,
 *  with the positive Y-axis passing through 90 degrees East longitude
 *  at the Equator and with the positive Z-axis passing through the
 *  North Pole.
 *
 *  The body coordinate system used in DIS is centered at the center of
 *  the entity's bounding volume (excluding articulated parts) and have
 *  the positive x-axis pointing to the front of the entity, the positive
 *  y-axis pointing to the right side of the entity and the positive z-axis
 *  pointing out of the bottom of the entity.
 *
 *  Orientation is given as [psi, theta, phi]. Angular velocity is given
 *  as [angular velocity around body x-axis, ditto y, ditto z].
 *
 *  The id to be used for further reference is returned in eid.
 *
 *  Zero is returned on success.
 */

	int       dis_entityEnter(int team, 
				  craft * c, 
				  dis_entity_type * e1, 
				  dis_entity_type * e2,	 
				  double location[3],
				  double velocity[3],
				  double linearAcceleration[3],
				  double orientation[3], 
				  double angularVelocity[3],
				  int *eid);

/*
 *  d i s _ e n t i t y E x i t
 *
 *  Remove the local entity with id eid from the simulation.
 *
 *  Zero is returned on success.
 */

	int       dis_entityExit(int eid);

/*
 *  d i s _ e n t i t y S t a t e
 *
 *  Update the state information for a local entity.
 *  The information will be broadcasted on the network
 *  only if it is necessary to keep the other hosts dead
 *  reckoning from exceeding the thresholds.
 *  See dis_entityEnter for information about the arguments.
 *
 *  Zero is returned on success.
 */

	int       dis_entityState(int eid, double location[3], double velocity[3],
							  double linearAcceleration[3],
					   double orientation[3], double angularVelocity[3]);

/*
 *  d i s _ g e t E n t i t y S t a t e
 *
 *  Return state information for a remote entity.
 *  The state information is dead reckoned from the last
 *  received data on the entity.
 *
 *  Zero is returned on success.
 */

	int       dis_getEntityState(int eid, double location[3], double velocity[3],
								 double orientation[3]);

/*
 *  d i s _ f i r e
 *
 *  Broadcast information about an entity firing a weapon.
 *  The type of fire is given by ftype as one of the DIS_FIRE_XXX types.
 *  The id's of the firing entity and the target entity are given with
 *  firingEid and targetEid or as DIS_ID_NONE if not known.
 *  The number of rounds, location of the source of fire, the velocity
 *  vector of the rounds and the range of the rounds are given with
 *  the corresponding arguments.
 *  The id of the event generated is returned in eventId.
 *  If the fire type is a missile, a missile entity is created and its
 *  id is returned in missileEid. The user program should generate
 *  position data for the missile during its lifetime by calling
 *  dis_entityState().
 *
 *  Zero is returned on success.
 *
 *  Not yet implemented.
 */

	int       dis_fire(int ftype, int firingEid, int targetEid, int rounds,
			   double location[3], 
			   double velocity[3], 
			   double range,
			   int *eventId, int *missileEid);

/*
 *  d i s _ d e t o n a t i o n
 *
 *  Broadcast information about a detonation.
 *  The type of fire is given by ftype as one of the DIS_FIRE_XXX types.
 *  The id's of the firing entity and the target entity are given with
 *  firingEid and targetEid or as DIS_ID_NONE if not known.
 *  The id of the corresponding fire event is given as eventId or as
 *  DIS_ID_NONE if not known.
 *  If the detonation is from a missile, the id of the missile is given
 *  as missileEid  or as DIS_ID_NONE. The library will exit the
 *  missile entity.
 *  The location of the detonation in world coordinates and in target
 *  body coordinates are given as worldLocation and entityLocation.
 *
 *  Zero is returned on success.
 */

	int       dis_detonation(dis_entity_type * etype,
				 int firingEid, 
				 int targetEid,
				 int missileEid, 
				 double worldLocation[3],
				 double entityLocation[3],
				 double vel_meters[3]);

/*
 *  Under some conditions, we don't want to really participate in a
 *  network. By calling dis_enableNetwork(0), we turn off network
 *  transmissions.
 */

	extern void dis_enableNetwork(int state);

	extern void dis_setEntityMarkings(int eid, char *markings);

	extern void dis_getEntityMarkings(int eid, char *markings, int max);

	extern void dis_setEntityAppearance(int eid, dis_entity_appearance x);

	extern dis_entity_appearance dis_getEntityAppearance(int eid);

	extern int dis_radarTargetChanged(craft * c);

	extern int dis_setRadarMode(craft * c, int mode, int update);

	extern int dis_getBeamCount(craft * c);

	extern void dis_getRadarParameters(craft * c, 
									   int j, 
									   double *az_center, 
									   double *az_width,
									   double *el_center, 
									   double *el_width, 
									   double *e);

	extern void dis_setBandwidth(double bps);

	extern int dis_canSimulate ( int eid );

/*
 *  d i s _ s n o o p
 *
 *  Process incoming PDUs for the specified number of milliseconds, thus
 *  priming the enity table.  This is called only during program
 *  initialization, when a complete entity table is required.
 */

	extern int dis_snoop ( int millisec );

#ifdef USE_REQUEST_CONTROL

	typedef int (*RequestControlCallback)(dis_pdu *, void *);

#endif

/*
 *  These definitions and data structures have been moved from dis_if.c
 */

#define MARKINGS_LEN 11

typedef struct _Entity_t {
	int       mode;				/* current mode */
	int       cur_target;		/* current radar target (ptbl index) */
	double    lastTime;			/* time of last update */
	dis_em_emission_pdu em;
	dis_em_system_info *s;		/* systems passed to us */
	dis_beam_info *b;			/* total active beams */
	dis_track_info *target;		/* track/jam target info */
} EntityEM_t;

typedef struct {
	int       local;			/* -1: free, 0: remote, 1: local */
	int		  state;			/* entity state; per [1] 4.5.5.5.3 */
	int       pending_state;
	double    pending_time;     /* time of state change */
	int		  emit_while_frozen; /* 1 to send entity state updates */
	craft    *c;				/* pointer to craft structure  in ptbl */
	double    lastTime;			/* last send for local, receive for remote */
	u_char    forceId;
	dis_entity_id entityId;
	dis_entity_type entityType;
	dis_entity_type altEntityType;
	dis_dr_parameters dr;		/* dead reckoning parameters */
	EntityEM_t *em;				/* EM emission PDU information, or NULL */
	unsigned char markings[MARKINGS_LEN+1];
	unsigned int appearance;
	double    location[3];
	double    velocity[3];
	double    linearAcceleration[3];
	double    orientation[3];
	double    angularVelocity[3];
	double    lastLocation[3];
	double    lastVelocity[3];
	double    lastLinearAcc[3];
	double    lastOrientation[3];
	double    lastAngularVel[3];
#ifdef USE_REQUEST_CONTROL
	RequestControlCallback controlRequestCallback;
	void      *callbackData;
#endif

} Entity_t;

#define MAX_ENTITIES 128

	extern Entity_t * dis_getEntityTable(void);
	extern long dis_getEntityTop(void);

	typedef enum _outstanting_request_t {
		OUTSTANDING_REQUEST_TRANSFER_CONTROL = 0,
	} OutstandingRequest_t;

	typedef struct _request_info_t {
		OutstandingRequest_t    request_type;/* type of pending request     */
		dis_request_id          request_id;  /* DIS request identifier      */
		Entity_t               *e;           /* entity involved in request  */
		double                  timeout_time;/* when will the request expire*/
		struct _request_info_t *next;
		struct _request_info_t *prev;
	} OutstandingRequestInfo_t;

#ifdef USE_REQUEST_CONTROL

	extern int dis_requestControl (Entity_t *e, 
								   RequestControlCallback callbackFn, 
								   void *arg);

	typedef int (*TransferControlRequestCallback)(Entity_t *, dis_transfer_control_pdu *);

	extern void dis_setTransferControlRequestCallback ( TransferControlRequestCallback  );

#endif

#if defined(__cplusplus)
};

#endif

#endif
