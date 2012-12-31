/*
 *  Copyright (C) 1995  Mats Lofkvist  CelsiusTech Electronics AB
 *
 *  With additions by Riley Rainey, 1995-1998
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

static char rcsid[] = "$Id: dis_if.c,v 1.2 1998/05/12 03:59:37 riley Exp $";

#include <stdio.h>
#include <math.h>
#ifndef WIN32
#include <sys/time.h>
#endif
#include <assert.h>

#include <pm.h>
#include <dis/dis.h>
#include <dis.h>

#ifdef WIN32
extern DISxApplicationInfo *WinACMDISxInitializeApplication(int, int, int);
extern int WinACMDISReadPDU (DISxApplicationInfo *, dis_pdu*);
extern int WinACMDISWritePDU (DISxApplicationInfo *, dis_pdu*);
#endif

int acknowledgePDU (dis_acknowledge_pdu *pdu);
int transferControlPDU (dis_transfer_control_pdu *pdu);
int setDataPDU (dis_set_data_pdu *pdu);
int startPDU (dis_start_pdu *);
int stopPDU (dis_stop_pdu *);
static int dis_initializeEMInfo ( Entity_t *e );

#define DEFAULT_LOCATION_THRESHOLD      1.0
#define DEFAULT_ORIENTATION_THRESHOLD   (3.0 * M_PI / 180.0)

#define SEND_TIMEOUT_SECONDS	4.8
#define RECV_TIMEOUT_SECONDS	12.0

static dis_EntityEnterCb entityEnterCb = NULL;
static dis_DetonationCb detonationCb = NULL;

static double locationThreshold = DEFAULT_LOCATION_THRESHOLD;
static double orientationThreshold = DEFAULT_ORIENTATION_THRESHOLD;

static int exercise;
static int site;
static int application;

static  OutstandingRequestInfo_t *request_chain_head = 0;
static  OutstandingRequestInfo_t *request_chain_tail = 0;

static int network_enabled = 1;

TransferControlRequestCallback transferControlRequestCallback = 0;

void
dis_setTransferControlRequestCallback ( TransferControlRequestCallback p )
{
	transferControlRequestCallback = p;
}

static OutstandingRequestInfo_t *
addRequest( dis_request_id request_id )
{
	OutstandingRequestInfo_t *p = malloc (sizeof(OutstandingRequestInfo_t));

	if (p) {
		memset (p, 0, sizeof(OutstandingRequestInfo_t));
		p->request_id = request_id;

		p->next = NULL;
		p->prev = request_chain_tail;
		if ( request_chain_tail ) {
			p->next = p;
		}
		if (request_chain_head == NULL) {
			request_chain_head = p;
		}
		request_chain_tail = p;
	}

	return p;
}

/*
 *  Remove the specified request tracking structure from the
 *  request tracking list.
 */

static void
removeRequest(OutstandingRequestInfo_t *pItem)
{
	OutstandingRequestInfo_t *p = request_chain_head;

	/*
	 * The request list is a mundane doubly linked list.
	 */

	if (pItem) {
		if (pItem->prev == NULL) {
			request_chain_head = p->next;
		}
		else {
			pItem->prev->next = p->next;
		}

		if (pItem->next == NULL) {
			request_chain_tail = p->prev;
		}
		else {
			pItem->next->prev = p->prev;
		}

		free ( pItem );
	}
}

static OutstandingRequestInfo_t *
findRequestByRequestID ( dis_request_id request_id )
{
	OutstandingRequestInfo_t *p = request_chain_head;

	while ( p ) {
		if (p->request_id == request_id) {
			break;
		}
		p = p->next;
	}

	return p;
}

/*
 *  We must limit our PDU transmission rate on lower bandwidth
 *  connections.
 */

static double bandwidth_bps = 0.0;	/* 0.0 turns off limiting */

static double theTime;
static int absoluteTime = 0;

static DISxApplicationInfo *app;

DISxApplicationInfo *
dis_getApplicationInfo(void)
{
	return app;
}

extern int killPlayer(craft * c);

int dis_shouldTransmitPDUs ( Entity_t *e );

static Entity_t *entities;
static int entity_top = -1;

int dis_isLocalEntity (const dis_entity_id *id);
Entity_t *
dis_getEntityTable(void)
{
	return entities;
}

long
dis_getEntityTop(void)
{
	return entity_top;
}

void
dis_enableNetwork(int state)
{
	network_enabled = state;
}

/*
 *  These coordinate system conversion routines don't do very much today,
 *  but, eventually, we must support a spheroid world (WGS84 standard).
 */

void
ACMtoDISWorld(VPoint * in, dis_world_coordinates * out)
{
	out->x = (in->x);
	out->y = (in->y);
	out->z = (in->z);
}

void
DIStoACMWorld(dis_world_coordinates * in, VPoint * out)
{
	out->x = (in->x);
	out->y = (in->y);
	out->z = (in->z);
}

void
ACMtoDISVelocity(VPoint * in, dis_float_vector * out)
{
	out->x = (float) FEETtoMETERS(in->x);
	out->y = (float) FEETtoMETERS(in->y);
	out->z = (float) FEETtoMETERS(in->z);
}

void
DIStoACMVelocity(dis_float_vector * in, VPoint * out)
{
	out->x = METERStoFEET(in->x);
	out->y = METERStoFEET(in->y);
	out->z = METERStoFEET(in->z);
}

/*
 *  Generate a transform matrix to get from geocentric to local NED coordinates
 */

void
GenerateWorldToLocalMatrix(WorldCoordinates * w, VMatrix * m)
{
	dis_world_coordinates gc;
	VPoint    p;

	VIdentMatrix(m);
	VRotate(m, ZRotation, -w->longitude);
	VRotate(m, YRotation, -w->latitude);
	VRotate(m, YRotation, -DEGtoRAD(90.0));
	DISWorldCoordinatesToGeocentric(w, &gc);
	VTransform((VPoint *) & gc, m, &p);
	m->m[0][3] = -p.x;
	m->m[1][3] = -p.y;
	m->m[2][3] = -p.z;
}

/*
 *  f i n d E n t i t y
 *
 *  Find the entity with Dis id id in the local entities table
 *  and return its index in the table.
 *
 *  The id (index) is returned on success, -1 is returned on failure.
 */

static int
findEntity(const dis_entity_id * id)
{
	int       i;
	Entity_t   *p = entities;

	for (i = 0; i <= entity_top; i++) {
		if (p->local != -1 &&
			p->entityId.entity_id == id->entity_id &&
			p->entityId.sim_id.application_id == id->sim_id.application_id &&
			p->entityId.sim_id.site_id == id->sim_id.site_id) {
			return i;
		}
		++p;
	}

	return -1;
}

/*
 *  f i n d L o c a l E n t i t y
 *
 *  Find the local entity with Dis id id in the local entities table
 *  and return its index in the table.
 *
 *  The id (index) is returned on success, -1 is returned on failure.
 */

static int
findLocalEntity( const dis_entity_id * id )
{
	int       i;
	Entity_t   *p = entities;

	for (i = 0; i <= entity_top; i++) {
		if (p->local == 1 &&
			p->entityId.entity_id == id->entity_id &&
			p->entityId.sim_id.application_id == id->sim_id.application_id &&
			p->entityId.sim_id.site_id == id->sim_id.site_id) {
			return i;
		}
		++p;
	}

	return -1;
}

/*
 *  d i s _ s e t B a n d w i d t h
 *
 *  Set and estimate of the available network bandwidth
 *  in bits per second.  This value is used to limit entity
 *  state transmissions in an effort to keep UDP traffic
 *  as close to real-time as possible.
 */

void
dis_setBandwidth(double bps)
{
	bandwidth_bps = bps;
}

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

int
dis_init(int xexercise, int xsite, int xapplication,
		 dis_EntityEnterCb xentityEnterCb,
		 dis_DetonationCb xdetonationCb)
{
	int       i;
	dis_simulation_addr addr;

	entityEnterCb = xentityEnterCb;
	detonationCb = xdetonationCb;

	exercise = xexercise;

#ifdef WIN32
#define DISxReadPDU		WinACMDISReadPDU
#define DISxWritePDU	WinACMDISWritePDU
	app = WinACMDISxInitializeApplication(xexercise, 
										  xsite, xapplication);
	if (app == NULL) {
		return -1;
	}
#else
	app = DISxInitializeApplication(xexercise, xsite, xapplication);
	if (app == NULL) {
		return -1;
	}
#endif

#ifdef USE_CONTROL_REQUEST
	DISxSetPDUProtocolFamily ( PDUTypeExperimentalRequestControl,
							   PDUFamilySimulationManagement );
	DISxSetPDUProtocolFamily ( PDUTypeExperimentalGrantControl,
							   PDUFamilySimulationManagement );
#endif

/*
 *  Get the actual simulation address assigned to us.
 */

	DISxGetSimulationAddress(app, &addr);
	site = addr.site_id;
	application = addr.application_id;

/*
 *  Allocate storage for the entity table and initialize it.
 */

	entities = (Entity_t *) malloc(sizeof(Entity_t) * MAX_ENTITIES);
	for (i = 0; i < MAX_ENTITIES; i++) {
		entities[i].local = -1;
	}

	return 0;
}

/*
 *  d i s _ c l o s e
 *
 *  Close down the DIS library.
 *
 *  Zero is returned on success.
 */

int
dis_close(void)
{
	if (entities) {
		free (entities);
	}
	return 0;
}

/*
 *  d i s _ s e t D R T h r e s h o l d s
 *
 *  Set the dead reckoning thresholds for location and orientation.
 *  The values shall be given in meters and radians (psi, theta, phi).
 *
 *  Zero is returned on success.
 */

int
dis_setDRThresholds(double location, double orientation)
{
	locationThreshold = location;
	orientationThreshold = orientation;

	return 0;
}

static const double timeFactor = 596523.235556;		/* 2^31/3600 bits per second */

/*
 *  t i m e D I S T o D o u b l e
 *
 *  Convert a DIS timestamp to a double in UNIX format (seconds since 1970).
 *
 *  If the timestamp _and_ the local time both are absolute times,
 *  the timestamp is used for the part of hour. The local time 'theTime'
 *  is used to get the hour part. The returned value will be
 *  the closest possible to 'theTime'.
 *
 *  If either the timestamp or the local time are _not_ absolute,
 *  the local time is returned. This could be improved...
 */

#ifdef WIN32
#define rint(x)	(double)( (int)(x) )
#endif

static double
timeDISToDouble(dis_timestamp disTime)
{
	double    seconds;			/* seconds into the current hour */
	double    myseconds;		/* ditto for 'theTime' */
	double    diffseconds;
	double    myhour;			/* hour part of 'theTime' */

	/* if either time is not absolute, return the local time */
	if (disTime.type == 0 || absoluteTime == 0)
		return theTime;

	seconds = disTime.time / timeFactor;
	myseconds = fmod(theTime, 3600.0);
	myhour = rint((theTime - myseconds) / 3600.0);

	diffseconds = myseconds - seconds;

#ifdef DIS_DEBUG_TIME
	printf("time diff %f s\n", diffseconds);
#endif
	if (diffseconds > 1800.0)
		return 3600.0 * (myhour + 1) + seconds;
	else if (diffseconds < -1800.0)
		return 3600.0 * (myhour - 1) + seconds;
	else
		return 3600.0 * myhour + seconds;
}

/*
 *  t i m e D o u b l e T o D I S
 *
 *  Convert a double in UNIX format (seconds since 1970) to a DIS timestamp.
 *  If reference is 0, the time will be marked relative.
 *  If reference is 1, the time will be marked absolute, i.e. true UTC time.
 */

static    dis_timestamp
timeDoubleToDIS(double time, int reference)
{
	unsigned long tmp;
	dis_timestamp res;

	tmp = (unsigned long) (fmod(time, 3600.0) * timeFactor);
	if (tmp > 2147483647L)		/* 2^31 - 1 */
		res.time = 2147483647L;
	else
		res.time = tmp;
	res.type = reference;

	return res;
}

/*
 *  d i s _ s e t T i m e
 *
 *  Set the current time in the DIS library as a relative time.
 *
 *  Zero is returned on success.
 */

int
dis_setTime(double time)
{
	theTime = time;
	absoluteTime = 0;

	return 0;
}

/*
 *  d i s _ s e t T i m e A b s o l u t e
 *
 *  Set the current time in the DIS library using the system time,
 *  and mark the time as absolute, i.e. true UTC time.
 *  This will improve the dead reckoning performance on networks
 *  with significant delays between players _iff_ this host really
 *  have true UTC time (and others declaring true time also really
 *  have it).
 *
 *  Zero is returned on success.
 */

int
dis_setTimeAbsolute(void)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);
	theTime = tv.tv_sec + tv.tv_usec / 1000000.0;
	absoluteTime = 1;

	return 0;
}

/*
 *  g e t E n t i t y S t a t e D a t a
 *
 *  Read in the entity state data from the entity state PDU es
 *  and write it to the local entity with id (index) eid.
 */

static void
getEntityStateData(int eid, dis_entity_state_pdu * es)
{

	entities[eid].lastTime = timeDISToDouble(es->hdr.time_stamp);
	entities[eid].lastLocation[0] = es->pos.x;
	entities[eid].lastLocation[1] = es->pos.y;
	entities[eid].lastLocation[2] = es->pos.z;
	entities[eid].lastVelocity[0] = es->vel.x;
	entities[eid].lastVelocity[1] = es->vel.y;
	entities[eid].lastVelocity[2] = es->vel.z;
	entities[eid].lastOrientation[0] = es->orientation.psi;
	entities[eid].lastOrientation[1] = es->orientation.theta;
	entities[eid].lastOrientation[2] = es->orientation.phi;
	entities[eid].lastLinearAcc[0] = es->dr_parm.linear_acc.x;
	entities[eid].lastLinearAcc[1] = es->dr_parm.linear_acc.y;
	entities[eid].lastLinearAcc[2] = es->dr_parm.linear_acc.z;
	entities[eid].lastAngularVel[0] = es->dr_parm.angular_vel.x;
	entities[eid].lastAngularVel[1] = es->dr_parm.angular_vel.y;
	entities[eid].lastAngularVel[2] = es->dr_parm.angular_vel.z;

	if (es->marking.charset == DISCharSetASCII)
		strcpy(entities[eid].markings, es->marking.marking);
	else
		entities[eid].markings[0] = '\0';

	DISProcessNewDRParameters(es, &entities[eid].dr);
}

/*
 *  e n t i t y E n t e r
 *
 *  Process the entity state PDU esPDU for a new (currently unknown)
 *  entity.
 *
 *  Zero is returned on success.
 */

static int
entityEnter(dis_entity_state_pdu * esPDU)
{
	int       eid;
	craft    *c = NULL;

	for (eid = 0; eid < MAX_ENTITIES; eid++) {
		if (entities[eid].local == -1) {
			break;
		}
	}
	if (eid >= MAX_ENTITIES) {
		return -1;
	}
	if (eid > entity_top) {
		entity_top = eid;
	}
	entities[eid].local = 0;
	entities[eid].state = DIS_ENTITY_STATE_SIMULATING;
	entities[eid].pending_state = DIS_ENTITY_STATE_NONE;
	entities[eid].emit_while_frozen = 0;
	entities[eid].forceId = esPDU->force_id;
	entities[eid].entityId = esPDU->id;
	entities[eid].entityType = esPDU->type;
	entities[eid].altEntityType = esPDU->alt_type;
	entities[eid].em = (EntityEM_t *) NULL;

	/*
     *  We only care about setting the dead reckoning thresholds
	 *  so that we can assume comtrol of an entity.
	 */

	DISSetDRThresholds(&entities[eid].dr, SEND_TIMEOUT_SECONDS,
					   locationThreshold, orientationThreshold);

	getEntityStateData(eid, esPDU);

	/*
	 * Pass entity information to the main ACM code.  Based on the DIS
	 * entity type, it will determine if this is worth tracking.
	 */

	(entityEnterCb) ( eid, &esPDU->type, esPDU->force_id, &c );

	if (c) {
		/* ACM says it's worth tracking */
		entities[eid].c = c;
#ifdef DIS_DEBUG
		printf ("adding entity %d; entity top %d\n", eid, entity_top);
#endif
	}
	else {
		/* must not be an entity we care about ... */
		entities[eid].local = -1;
		if (eid == entity_top) {
			entity_top--;
		}
		return -1;
	}

	return eid;
}

/*
 *  e n t i t y E x i t
 *
 *  Remove the entity with id (index) eid from the local table.
 */

static void
entityExit(int eid)
{
	craft    *c;

#ifdef DIS_DEBUG
	printf("entityExit (%d)\n", eid);
#endif

	c = entities[eid].c;
	if (c != NULL && c->type == CT_DIS_PLANE) {
#ifdef DIS_DEBUG
		printf("killing ptbl player index %d in entityExit\n", c->pIndex);
#endif
		killPlayer(c);
	}
	entities[eid].local = -1;
	if (eid == entity_top) {
		entity_top--;
	}
}

/*
 *  e n t i t y S t a t e P D U
 *
 *  Process an incoming entity state PDU.
 *
 *  Zero is returned on success.
 */

static int
entityStatePDU(dis_entity_state_pdu * esPDU)
{
	int       eid;

	eid = findEntity(&esPDU->id);

	if ((esPDU->appearance & DISAppearanceDamageDestroyed)) {
		/* deactivated or destroyed entity. if we know about it, exit it */
#ifdef DIS_DEBUG
		printf ("entity %d marked as destroyed.\n");
#endif
		if (eid >= 0) {
			entityExit(eid);
		}
		return 0;
	}
	else {
		/* normal entity state PDU. if we know about it, update data,
		   otherwise enter it */
		if (eid >= 0) {
			getEntityStateData(eid, esPDU);
			return 0;
		}
		else {
			eid = entityEnter(esPDU);
			if (eid >= 0) {
				return 0;
			}
			else
				return -1;
		}
	}
}

/*
 *  f i r e P D U
 *
 *  Process an incoming fire PDU.
 *
 *  Zero is returned on success.
 */

 /*
  *  These munition types are renderable with ACM's cannon simulation
  *  code.
  */

static dis_entity_type cannon_types[] =
{
	{2, 2, 225, 2, 1, 0, 0},
	{2, 2, 225, 2, 2, 0, 0},
	{2, 2, 225, 2, 3, 0, 0},
	{2, 2, 225, 2, 4, 0, 0},
	{2, 2, 222, 2, 1, 0, 0},
	{0, 0, 0, 0, 0, 0, 0}
};

static int
firePDU(dis_fire_pdu * fPDU)
{
	int       i, eid;
	craft    *m;
	dis_entity_type *dp;

	for (dp = cannon_types; dp->kind != 0; ++dp) {
		if (fPDU->burst.munition.kind == dp->kind &&
			fPDU->burst.munition.domain == dp->domain &&
			fPDU->burst.munition.country == dp->country &&
			fPDU->burst.munition.category == dp->category &&
			fPDU->burst.munition.subcategory == dp->subcategory) {
			break;
		}
	}

/*
 *  Not one of the ones that we model?  Then do nothing.
 */

	if (dp->kind == 0) {
		return 0;
	}
/*
 *  Allocate a projectile record
 */

	for ((i = 0, m = mtbl); i < MAXPROJECTILES; (++i, ++m)) {
		if (m->type == CT_FREE) {
			m->type = CT_DIS_CANNON;
			break;
		}
	}
	if (i == MAXPROJECTILES) {
		return -1;
	}
/*
 *  Fill out the projectile record
 */

	eid = findEntity(&fPDU->firing_id);
	if (eid >= 0) {
		m->owner = entities[eid].c->pIndex;
	}
	else {
		m->owner = -1;
	}
	m->createTime = curTime;
	m->curRoll = 0.0;
	m->curPitch = 0.0;
	m->curHeading = 0.0;

/*
 *  Determine the initial position.
 */

	DIStoACMVelocity((dis_float_vector *) &fPDU->vel, &m->Cg);
	DIStoACMWorld(&fPDU->pos, &m->Sg);

	m->prevSg = m->Sg;
	m->rounds = fPDU->burst.quantity;
	m->tracerMod = 10 /*TRACER_MOD */ ;
	m->tracerVal = 0;
	m->offset = 0.0;
	m->interval = deltaT;
	m->cinfo = lookupCraft("m61a1 cannon");

	return 0;
}

/*
 *  d e t o n a t i o n P D U
 *
 *  Process an incoming detonation PDU.
 *
 *  Zero is returned on success.
 */

static int
detonationPDU(dis_detonation_pdu * dPDU)
{
	int       firingEid, targetEid, eid, ftype;
	double    time, worldLocation[3], entityLocation[3];
	craft    *c;

	if (detonationCb == NULL) {
		eid = findEntity(&dPDU->munition_id);
		if (eid >= 0) {
			entities[eid].local = -1;
			if (eid == entity_top) {
				entity_top--;
			}
		}
		return 0;
	}
	firingEid = findEntity(&dPDU->firing_id);
	if (firingEid < 0)
		return -1;
	targetEid = findEntity(&dPDU->target_id);
	if (targetEid < 0)
		return -2;

	/* the ftype field isn't relly used anymore */

	if (dPDU->burst.munition.category == 2)		/* Ballistic */
		ftype = DIS_FIRE_M61A1;
	else
		ftype = DIS_FIRE_AIM9M;

	time = timeDISToDouble(dPDU->hdr.time_stamp);

	worldLocation[0] = dPDU->pos.x;
	worldLocation[1] = dPDU->pos.y;
	worldLocation[2] = dPDU->pos.z;

	entityLocation[0] = dPDU->loc.x;
	entityLocation[1] = dPDU->loc.y;
	entityLocation[2] = dPDU->loc.z;

	eid = findEntity(&dPDU->munition_id);
	c = (eid < 0) ? NULL : entities[eid].c;

	(detonationCb) (ftype, firingEid, targetEid, time, worldLocation,
					entityLocation, c, dPDU);

	if (eid >= 0) {
		entities[eid].local = -1;
		if (eid == entity_top) {
			entity_top--;
		}
	}
	return 0;
}

/*
 *  e m i s s i o n P D U
 *
 *  Process an incoming EM emission PDU.
 *
 *  Zero is returned on success.
 */

static int
emissionPDU(dis_em_emission_pdu * pdu)
{
	Entity_t   *e;
	int       emitterEid;

	emitterEid = findEntity(&pdu->emitter_id);
	if (emitterEid < 0) {
		return -1;
	}
	else {
		e = &entities[emitterEid];
	}

/*
 *  First emission received?
 */

	if (e->em == (EntityEM_t *) NULL) {

		dis_initializeEMInfo ( e );

	}

/*
 *  Not the first emission.  Free the old PDU variable fields and insert
 *  the new one.
 */

	else {
		DISFreePDUComponents((dis_pdu *) & e->em->em);
	}
	e->em->em = *pdu;
	e->em->lastTime = theTime;
	return 0;
}

/*
 *  d i s _ r e c e i v e
 *
 *  Process all available incoming PDU's from the network.
 *  User callbacks will be called for entering entities,
 *  exiting entities, firing entities and detonations.
 *
 *  Zero is returned on success.
 */

int
dis_receive(void)
{
	int       status, err, free_needed;
	dis_pdu   pdu;
	int       i;

	if (network_enabled == 0) {
		return 0;
	}

	err = 0;
	while ((status = DISxReadPDU(app, &pdu)) == 0) {

		free_needed = 1;

		/* ignore other exercises */
		if (pdu.hdr.exercise_id != exercise)
			goto free_pdu;

		switch (pdu.hdr.pdu_type) {
		case PDUTypeEntityState:
			/* don't read our own broadcasts */
			if (dis_isLocalEntity(&pdu.entity_state.id))
				goto free_pdu;

			err = entityStatePDU(&pdu.entity_state);
			break;

		case PDUTypeFire:
			/* don't read our own broadcasts */
			if (dis_isLocalEntity(&pdu.detonation.firing_id))
				goto free_pdu;

			err = firePDU(&pdu.fire);
			break;

		case PDUTypeDetonation:
			/* don't read our own broadcasts */
			if (dis_isLocalEntity(&pdu.detonation.firing_id))
				goto free_pdu;

			err = detonationPDU(&pdu.detonation);
			break;

		case PDUTypeEmission:
			/* don't read our own broadcasts */
			if (dis_isLocalEntity(&pdu.em_emission.emitter_id))
				goto free_pdu;
			err = emissionPDU(&pdu.em_emission);
			free_needed = 0;
			break;

		case PDUTypeSetData:
			/* don't read our own broadcasts */
			if (dis_isLocalEntity(&pdu.set_data.orig_id))
				goto free_pdu;
			err = setDataPDU( &pdu.set_data );
			free_needed = 0;
			break;

		case PDUTypeStopFreeze:
			/* don't read our own broadcasts */
			if (dis_isLocalEntity(&pdu.stop.orig_id))
				goto free_pdu;
			err = stopPDU( &pdu.stop );
			free_needed = 0;
			break;

		case PDUTypeStartResume:
			/* don't read our own broadcasts */
			if (dis_isLocalEntity(&pdu.start.orig_id))
				goto free_pdu;
			err = startPDU( &pdu.start );
			free_needed = 0;
			break;

		case PDUTypeTransferControl:
			/* don't read our own broadcasts */
			if (dis_isLocalEntity(&pdu.transfer_control.orig_id))
				goto free_pdu;
			err = transferControlPDU( &pdu.transfer_control );
			free_needed = 0;
			break;

		case PDUTypeAcknowledge:
			/* don't read our own broadcasts */
			if (dis_isLocalEntity(&pdu.acknowledge.orig_id))
				goto free_pdu;
			err = acknowledgePDU( &pdu.acknowledge );
			free_needed = 0;
			break;

		default:
			fprintf(stderr, 
					"dis_receive: Ignoring PDU type %d\n", pdu.hdr.pdu_type);
			err = 0;
			break;
		}

/*
 *  Free any dynamically allocated variable components that are part of this
 *  PDU.
 */

	  free_pdu:
		if (free_needed) {
			DISFreePDUComponents(&pdu);
		}

	}							/* while (... DISxReadPDU ...) */

/*
 *  check for timeouts on remote entities and look for pending state changes
 */

	for (i = 0; i <= entity_top; i++) {
		Entity_t *e = &entities[i];
		if (e->local == 0
			&& theTime - e->lastTime > RECV_TIMEOUT_SECONDS) {
#ifdef DIS_DEBUG
			printf ("entity state PDU timeout: index %d\n", i);
#endif
			entityExit(i);
		}

		if ( e->pending_state != DIS_ENTITY_STATE_NONE ) {
			if (theTime >= e->pending_time) {
				e->state = e->pending_state;
				e->pending_state = DIS_ENTITY_STATE_NONE;
			}
		}
	}

	if (err != 0) {
		return -2;
	}
	else {
		return 0;
	}
}

/*
 *  s e n d E n t i t y S t a t e
 *
 *  Send an entity state PDU for the local entity with id (index) eid.
 *
 *  Zero is returned on success.
 */

static int
sendEntityState(int eid)
{
	dis_entity_state_pdu pdu, *esPDU = &pdu;
	int       i;
	Entity_t  *e = &entities[eid];

	if (network_enabled == 0) {
		return 0;
	}

	if  (dis_shouldTransmitPDUs ( e ) == 0) {
		return 0;
	}

	esPDU->hdr.pdu_type = PDUTypeEntityState;
	esPDU->hdr.time_stamp = timeDoubleToDIS(theTime, absoluteTime);
	esPDU->id = e->entityId;
	esPDU->force_id = e->forceId;
	esPDU->art_parm_count = 0;
	esPDU->type = e->entityType;
	esPDU->alt_type = e->altEntityType;
	esPDU->pos.x = e->location[0];
	esPDU->pos.y = e->location[1];
	esPDU->pos.z = e->location[2];
	esPDU->vel.x = (float) e->velocity[0];
	esPDU->vel.y = (float) e->velocity[1];
	esPDU->vel.z = (float) e->velocity[2];
	esPDU->orientation.psi = (float) e->orientation[0];
	esPDU->orientation.theta = (float) e->orientation[1];
	esPDU->orientation.phi = (float) e->orientation[2];
	esPDU->appearance = e->appearance;
	esPDU->dr_parm.algorithm = DISDRMethodRVW;
	esPDU->dr_parm.linear_acc.x = (float) e->linearAcceleration[0];
	esPDU->dr_parm.linear_acc.y = (float) e->linearAcceleration[1];
	esPDU->dr_parm.linear_acc.z = (float) e->linearAcceleration[2];
	esPDU->dr_parm.angular_vel.x = (float) e->angularVelocity[0];
	esPDU->dr_parm.angular_vel.y = (float) e->angularVelocity[1];
	esPDU->dr_parm.angular_vel.z = (float) e->angularVelocity[2];
	esPDU->marking.charset = DISCharSetASCII;
	memset(esPDU->marking.marking, 0, MARKINGS_LEN);
	strcpy(esPDU->marking.marking, e->markings);
	esPDU->capabilities = 0;
	esPDU->art_parm = NULL;

	if (DISxWritePDU(app, (dis_pdu *) esPDU) != 0) {
		printf ("error writing PDU\n");
		return -2;
	}
	else {
		DISProcessNewDRParameters(esPDU, &entities[eid].dr);
		entities[eid].lastTime = theTime;
		for (i = 0; i < 3; i++) {
			entities[eid].lastLocation[i] = entities[eid].location[i];
			entities[eid].lastVelocity[i] = entities[eid].velocity[i];
			entities[eid].lastLinearAcc[i] = entities[eid].linearAcceleration[i];
			entities[eid].lastOrientation[i] = entities[eid].orientation[i];
			entities[eid].lastAngularVel[i] = entities[eid].angularVelocity[i];
		}

		return 0;
	}
}

/*
 *  s e t P o s D a t a
 *
 *  Set the position data of the entity with id (index) eid to the given
 *  values.
 */

static void
setPosData(int eid, double loc[3], double vel[3], double linAcc[3],
		   double ori[3], double angVel[3])
{
	int       i;

	for (i = 0; i < 3; i++) {
		entities[eid].location[i] = loc[i];
		entities[eid].velocity[i] = vel[i];
		entities[eid].linearAcceleration[i] = linAcc[i];
		entities[eid].orientation[i] = ori[i];
		entities[eid].angularVelocity[i] = angVel[i];
	}
}

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

int
dis_entityEnter(int team, 
				craft * c, 
				dis_entity_type * e1, 
				dis_entity_type * e2,
				double loc[3], 
				double vel[3],
				double linAcc[3], 
				double ori[3], 
				double angVel[3],
				int *neid)
{
	static int eid;

	for (eid = 0; eid < MAX_ENTITIES; eid++) {
		if (entities[eid].local < 0) {
			break;
		}
	}
	if (eid >= MAX_ENTITIES) {
		return -1;
	}
	if (eid > entity_top) {
		entity_top = eid;
	}
#ifdef DIS_DEBUG
	printf("Allocated eid %d; entity_top %d\n", eid, entity_top);
#endif

	entities[eid].local = 1;
	entities[eid].c = c;
	entities[eid].state = DIS_ENTITY_STATE_SIMULATING;
	entities[eid].pending_state = DIS_ENTITY_STATE_NONE;
	entities[eid].emit_while_frozen = 0;

	DISxIssueEntityID(app, &entities[eid].entityId);

	entities[eid].forceId = (team == 1) ?
		DISForceFriendly :
		DISForceOpposing;
	entities[eid].entityType = *e1;
	entities[eid].altEntityType = *e2;

	memset(entities[eid].markings, 0, MARKINGS_LEN);
	entities[eid].markings[0] = '\0';
	entities[eid].appearance = 0;
	setPosData(eid, loc, vel, linAcc, ori, angVel);

	*neid = eid;

/*
 *  Allocate  EM emission fields (optimized for the fact that we have
 *  only one system, with one beam, and one potentially tracked target.
 */

	entities[eid].em =
		(EntityEM_t *) Vmalloc(sizeof(EntityEM_t));
	entities[eid].em->mode = 0;
	entities[eid].em->cur_target = -1;
	entities[eid].em->em.hdr.pdu_type = PDUTypeEmission;
	entities[eid].em->em.emitter_id = entities[eid].entityId;

	entities[eid].em->s =
		(dis_em_system_info *) Vmalloc(sizeof(dis_em_system_info));
	entities[eid].em->b =
		(dis_beam_info *) Vmalloc(sizeof(dis_beam_info));
	entities[eid].em->target =
		(dis_track_info *) Vmalloc(sizeof(dis_track_info));
	entities[eid].em->em.system = entities[eid].em->s;
	entities[eid].em->em.num_systems = 1;
	entities[eid].em->em.system[0].num_beams = 0;

	DISSetDRThresholds(&entities[eid].dr, SEND_TIMEOUT_SECONDS,
					   locationThreshold, orientationThreshold);

	/* let the normal dis code do it */
#ifdef notdef
	if (sendEntityState(eid) < 0)
		return -1;
	else
#endif
		return 0;
}

void
dis_setEntityMarkings(int eid, char *markings)
{
	strncpy(entities[eid].markings, markings, MARKINGS_LEN);
}

void
dis_getEntityMarkings(int eid, char *markings, int max)
{
	strncpy(markings, entities[eid].markings, max);
}

void
dis_setEntityAppearance(int eid, dis_entity_appearance x)
{
	entities[eid].appearance = x;
}

dis_entity_appearance
dis_getEntityAppearance(int eid)
{
	return entities[eid].appearance;
}

/*
 *  d i s _ e n t i t y E x i t
 *
 *  Remove the local entity with id eid from the simulation.
 *
 *  Zero is returned on success.
 */

int
dis_entityExit(int eid)
{

#ifdef DIS_DEBUG
	printf("Entity_t exit called: eid %d\n", eid);
#endif

	if (entities[eid].local != 1)
		return -1;
	else {
		entities[eid].appearance = DISAppearanceDamageDestroyed;
		sendEntityState(eid);

		if (entities[eid].em) {
			free(entities[eid].em->s);
			free(entities[eid].em->b);
			free(entities[eid].em->target);
			free(entities[eid].em);
		}
		entities[eid].local = -1;
		if (eid == entity_top) {
			entity_top--;
		}
		return 0;
	}
}

/*
 *  d i s _ e n t i t y S t a t e
 *
 *  Update the state information for a local entity.
 *  The information will be broadcasted on the network
 *  only if it is necessary to keep the other hosts dead
 *  reckoning from exceeding the thresholds.
 *  See dis_entityEnter for information about the arguments.
 *
 *  This procedure also handles the transmission of emission PDU's.
 *
 *  Zero is returned on success.
 */

int
dis_entityState(int eid, double loc[3], double vel[3],
				double linAcc[3],
				double ori[3], double angVel[3])
{
	double    delta, min_delta;
	int       needToSend = 0, needEMPDU = 0, status, i, j;
	dis_euler_angles ori_e;

	if (entities[eid].local != 1)
		return -1;

	setPosData(eid, loc, vel, linAcc, ori, angVel);

/*
 *  EM emission PDU possibly needed ?
 */

	delta = theTime - entities[eid].lastTime;

	if (entities[eid].em && entities[eid].em->mode > 0) {
		if (delta > SEND_TIMEOUT_SECONDS) {
			needEMPDU = 1;
			for (i = 0; i < entities[eid].em->em.num_systems; ++i) {
				for (j = 0; j < entities[eid].em->em.system[0].num_beams; ++j) {
					entities[eid].em->em.system[i].beam[j].fundamental.beam_sweep_sync += (float) delta;
				}
			}
			entities[eid].em->lastTime = theTime;
			entities[eid].em->em.state_update = 0;
		}
	}

	ori_e.psi   = (float) ori[0];
	ori_e.theta = (float) ori[1];
	ori_e.phi   = (float) ori[2];

	needToSend = DISTestDRThresholds(&entities[eid].dr, delta,
				(dis_world_coordinates *) loc, &ori_e);

	/*
	 *  Are we limiting PDU tansmissions? 
	 *
	 *  If so, ensure enough time has passed since
	 *  our last entity state transmission.
	 */

	if (bandwidth_bps > 0.0) {
		min_delta = 1440.0 * entity_top / bandwidth_bps;
		if (delta < min_delta) {
			needToSend = 0;
		}
	}

	if (needToSend != 0) {
		sendEntityState(eid);
	}
	else {
		/* no need to send ES PDU */
	}

	if ( needEMPDU && entities[eid].state == DIS_ENTITY_STATE_SIMULATING ) {
		entities[eid].em->em.hdr.time_stamp =
			timeDoubleToDIS(theTime, absoluteTime);
		status = DISxWritePDU(app, (dis_pdu *) & entities[eid].em->em);
	}
	return (needToSend > 0) ? 1 : 0;
}

/*
 *  d i s _ g e t E n t i t y S t a t e
 *
 *  Return state information for a remote entity.
 *  The state information is dead reckoned from the last
 *  received data on the entity.
 *
 *  Zero is returned on success.
 */

int
dis_getEntityState(int eid, double loc[3], double vel[3], double ori[3])
{
	int       i;

	if (entities[eid].local < 0) {
		return -1;
	}
	else if (entities[eid].local == 1) {
		for (i = 0; i < 3; i++) {
			loc[i] = entities[eid].location[i];
			ori[i] = entities[eid].orientation[i];
		}

		return 0;
	}
	else {
		VMatrix   orientation;
		dis_linear_vel_vector drvel;

		DISComputeDRPosition(&entities[eid].dr,
							 theTime - entities[eid].lastTime,
					  (dis_world_coordinates *) & entities[eid].location,
							 &drvel,
							 &orientation);

		entities[eid].velocity[0] = drvel.x;
		entities[eid].velocity[1] = drvel.y;
		entities[eid].velocity[2] = drvel.z;

		matrixToEuler(&orientation,
					  &entities[eid].orientation[0],
					  &entities[eid].orientation[1],
					  &entities[eid].orientation[2]);

		for (i = 0; i < 3; i++) {
			loc[i] = entities[eid].location[i];
			vel[i] = entities[eid].velocity[i];
			ori[i] = entities[eid].orientation[i];
		}

		return 0;
	}
}

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

int
dis_fire(int ftype, int firingEid, int targetEid, int rounds,
		 double location[3], double velocity[3], double range,
		 int *eventId, int *missileEid)
{
	/* TODO */
	*eventId = 0;
	*missileEid = 0;

	return 0;
}

static dis_entity_id null_id =
{0, 0, 0};

int
dis_fire_cannon(craft * c, VPoint * pos, VPoint * vel, int quantity, int rate)
{
	dis_fire_pdu fire;
	int       status;

	if (network_enabled == 0) {
		return 0;
	}

	fire.hdr.pdu_type = PDUTypeFire;

	fire.firing_id = entities[c->disId].entityId;
	fire.target_id = null_id;
	fire.munition_id = null_id;

	DISxIssueEventID(app, &fire.event);

	fire.fire_mission_index = 0;	/* NO_FIRE_MISSION */

	ACMtoDISWorld(pos, &fire.pos);
	fire.burst.munition = cannon_types[1];
	fire.burst.warhead = 0;
	fire.burst.fuze = 0;
	fire.burst.quantity = quantity;
	fire.burst.rate = rate;

	ACMtoDISVelocity(vel, (dis_float_vector *) &fire.vel);

	fire.range = 0.0f;

	fire.hdr.time_stamp = timeDoubleToDIS(theTime, absoluteTime);

	status = DISxWritePDU(app, (dis_pdu *) & fire);
	return (status == 0) ? 0 : -1;
}

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

int
dis_detonation(dis_entity_type * etype,
			   int firingEid, int targetEid, int missileEid,
			   double worldLocation[3], double entityLocation[3],
			   double vel[3])
{
	dis_detonation_pdu pdu;
	int       status;

	if (network_enabled == 0) {
		return 0;
	}

	pdu.hdr.pdu_type = PDUTypeDetonation;

	if (entities[firingEid].local != 1) {
		return -1;
	}
	if (targetEid != DIS_ID_NONE && entities[targetEid].local == -1) {
		return -2;
	}
	pdu.firing_id = entities[firingEid].entityId;
	if (targetEid != DIS_ID_NONE) {
		pdu.target_id = entities[targetEid].entityId;
	}
	else {
		pdu.target_id = null_id;
	}

#ifdef DIS_DEBUG
	printf("detonation: %d %d %d\n", firingEid, targetEid, missileEid);
#endif

	if (missileEid != DIS_ID_NONE) {
		pdu.munition_id = entities[missileEid].entityId;
	}
	else {
		pdu.munition_id = null_id;
	}

	DISxIssueEventID(app, &pdu.event);

	pdu.vel.x = (float) vel[0];
	pdu.vel.y = (float) vel[1];
	pdu.vel.z = (float) vel[2];

	pdu.pos.x = worldLocation[0];
	pdu.pos.y = worldLocation[1];
	pdu.pos.z = worldLocation[2];

	pdu.burst.munition = *etype;

/*
 *  This code will need some extra work ...
 */

	if (pdu.burst.munition.category == 2) {
		pdu.burst.warhead = DISWarheadKinetic;
		pdu.burst.fuze = DISFuzeContact;
		pdu.burst.quantity = 1;
		pdu.burst.rate = 0;
		pdu.result = DISDetonationResultEntityImpact;
	}
	else {
		pdu.burst.warhead = DISWarheadHEFragment;
		pdu.burst.fuze = DISFuzeProximity;
		pdu.burst.quantity = 1;
		pdu.burst.rate = 0;
		pdu.result = DISDetonationResultDetonation;
	}

	pdu.loc.x = (float) entityLocation[0];
	pdu.loc.y = (float) entityLocation[1];
	pdu.loc.z = (float) entityLocation[2];

	pdu.result = 0;
	pdu.num_art_parms = 0;
	pdu.art_parm = NULL;

	pdu.hdr.time_stamp = timeDoubleToDIS(theTime, absoluteTime);

	status = DISxWritePDU(app, (dis_pdu *) & pdu);
	return (status == 0) ? 0 : -1;
}

/*
 * d i s _ i n i t i a l i z e E M I n f o
 *
 * Initialize EM info data structures.  Returns zero on success, -1 on error.
 */

static int
dis_initializeEMInfo ( Entity_t *e )
{
	int result = 0;

	if (e->em == (EntityEM_t *) NULL) {

/*
 *  Allocate  EM emission information structure and initialize it to reflect
 *  that this is an external entity (mode == -1).
 */

		e->em = (EntityEM_t *) Vmalloc(sizeof(EntityEM_t));
		if (e->em) {
			e->em->mode = -1;
			e->em->cur_target = -1;
		}
		else {
			result = -1;
		}

	}
	return result;
}

/*
 *  Construct an EM emission PDU that reflects the current state of our
 *  radar set.
 */

int
constructEmissionPDU(craft * c, int mode, int update)
{
	dis_em_emission_pdu *em;
	dis_em_system_info *s;
	dis_beam_info *b;
	dis_track_info *target;

	entities[c->disId].em->mode = mode;
	em = &entities[c->disId].em->em;
	s = entities[c->disId].em->s;
	b = entities[c->disId].em->b;
	target = entities[c->disId].em->target;

	DISxIssueEventID(app, &em->event);

	em->system = s;
	em->state_update = update;
	em->num_systems = 1;

	s->location.x = 0.0f;
	s->location.y = 0.0f;
	s->location.z = 0.0f;
	s->emitter_system.name = 0;
	s->emitter_system.function = DISEmitterFuncAirborneFireControl;
	s->emitter_system.id = 1;

	switch (mode) {
	case 0:
		s->num_beams = 0;
		s->beam = NULL;
		break;

/*
 *  Three-bar track while scan mode
 */

	case 1:
		s->num_beams = 1;
		s->beam = b;
		b->beam_id = 1;
		b->beam_parm_index = 0;
		b->beam_function = DISBeamFuncAcquisitionAndTracking;
		b->fundamental.freq = 9000.0f;
		b->fundamental.erp = 100.0f;
		b->fundamental.prf = 18000.0f;
		b->fundamental.pulse_width = 1.0f;
		b->fundamental.beam_azimuth_center = 0.0f;
		b->fundamental.beam_azimuth_sweep = 120.0f;
		b->fundamental.beam_elev_center = 0.0f;
		b->fundamental.beam_elev_sweep = 120.0f;
		b->fundamental.beam_sweep_sync = 0.0f;
		b->pad = 0;
		b->jamming_mode = 0;
		break;

/*
 *  Four bar 20 x 30 ACM mode
 */

	case 2:
		s->num_beams = 1;
		s->beam = b;
		b->beam_id = 1;
		b->beam_parm_index = 1;
		b->beam_function = DISBeamFuncAcquisitionAndTracking;
		b->fundamental.freq = 9000.0f;
		b->fundamental.erp = 100.0f;
		b->fundamental.prf = 18000.0f;
		b->fundamental.pulse_width = 1.0f;
		b->fundamental.beam_azimuth_center = 0.0f;
		b->fundamental.beam_azimuth_sweep = 30.0f;
		b->fundamental.beam_elev_center = 0.0f;
		b->fundamental.beam_elev_sweep = 20.0f;
		b->fundamental.beam_sweep_sync = 0.0f;
		b->pad = 0;
		b->jamming_mode = 0;
		break;

/*
 *  Single target track
 */

	case 3:
		s->num_beams = 1;
		s->beam = b;
		b->beam_id = 1;
		b->beam_parm_index = 2;
		b->beam_function = DISBeamFuncAcquisitionAndTracking;
		b->fundamental.freq = 9000.0f;
		b->fundamental.erp = 100.0f;
		b->fundamental.prf = 18000.0f;
		b->fundamental.pulse_width = 1.0f;
		b->fundamental.beam_azimuth_center = 0.0f;	/* wrong, don't care */
		b->fundamental.beam_azimuth_sweep = 0.0f;
		b->fundamental.beam_elev_center = 0.0f;		/* wrong, don't care */
		b->fundamental.beam_elev_sweep = 0.0f;
		b->fundamental.beam_sweep_sync = 0.0f;
		b->pad = 0;
		b->jamming_mode = 0;
		break;
	}

	entities[c->disId].em->cur_target = c->curRadarTarget;

	if (c->curRadarTarget == -1) {
		b->tracked_target = NULL;
		b->num_targets = 0;
	}
	else {
		b->tracked_target = target;
		b->num_targets = 1;

		target->target =
			entities[ptbl[c->curRadarTarget].disId].entityId;
		target->emitter_id = 1;
		target->beam_id = 1;
	}
	return 0;
}

/*
 *  Set local entities current radar mode (modes are 0=off, 1=wide scan)
 */

int
dis_setRadarMode(craft * c, int mode, int update)
{
	int       status;
	Entity_t  *e = &entities[c->disId];

	if ( e->em == NULL) {
		dis_initializeEMInfo ( e );
	}

	if ( e->em->mode == mode) {
		return 0;
	}

	if ( e->em->mode == -1 ) {
		e->em->s = (dis_em_system_info *) Vmalloc(sizeof(dis_em_system_info));
		e->em->b = (dis_beam_info *) Vmalloc(sizeof(dis_beam_info));
		e->em->target = (dis_track_info *) Vmalloc(sizeof(dis_track_info));
		e->em->em.system = e->em->s;
		e->em->em.num_systems = 1;
		e->em->em.system[0].num_beams = 0;
	}
	constructEmissionPDU(c, mode, 1);
	entities[c->disId].em->em.hdr.time_stamp =
		timeDoubleToDIS(theTime, absoluteTime);
	entities[c->disId].em->lastTime = theTime;
	if (network_enabled == 0) {
		return 0;
	}
	status = DISxWritePDU(app, (dis_pdu *) & entities[c->disId].em->em);
	return (status == 0) ? 0 : -1;
}

/*
 *  Notify the world that our current radar target changed
 */

int
dis_radarTargetChanged(craft * c)
{
	int       status;

	if ( entities[c->disId].em == NULL) {
		dis_initializeEMInfo ( &entities[c->disId] );
	}

	if (entities[c->disId].em->cur_target == c->curRadarTarget) {
		return 0;
	}
	constructEmissionPDU(c, entities[c->disId].em->mode, 1);
	entities[c->disId].em->em.hdr.time_stamp =
		timeDoubleToDIS(theTime, absoluteTime);
	entities[c->disId].em->lastTime = theTime;
	if (network_enabled == 0) {
		return 0;
	}
	status = DISxWritePDU(app, (dis_pdu *) & entities[c->disId].em->em);
	return (status == 0) ? 0 : -1;
}

/*
 *  Get the number of beams emitted from this aircraft
 */

int
dis_getBeamCount(craft * c)
{
	if (entities[c->disId].em && entities[c->disId].em->em.num_systems > 0) {
		return entities[c->disId].em->em.system[0].num_beams;
	}
	return 0;
}

/*
 *  Get parameters describing the specified beam
 */

void
dis_getRadarParameters(craft * c, int j, double *az_center, double *az_width,
					   double *el_center, double *el_width, double *e)
{
	dis_beam_info *b;

	if ( entities[c->disId].em == NULL) {
		dis_initializeEMInfo ( &entities[c->disId] );
	}

	b = &entities[c->disId].em->em.system[0].beam[j];

	*az_center = b->fundamental.beam_azimuth_center;
	*az_width = b->fundamental.beam_azimuth_sweep;
	*el_center = b->fundamental.beam_elev_center;
	*el_width = b->fundamental.beam_elev_sweep;
	*e = b->fundamental.erp;
}

int
dis_isLocalEntity (const dis_entity_id *id)
{
	int eid = findLocalEntity ( id );

	return (eid >= 0) ? 1 : 0;
}

int
transferControlPDU ( dis_transfer_control_pdu *pdu )
{
	Entity_t     *e;
	int          eid;
	dis_acknowledge_pdu reply_pdu;
	int status = 0;
	int error_return_needed = 0;

	reply_pdu.hdr            = pdu->hdr;
	reply_pdu.hdr.pdu_type   = PDUTypeAcknowledge;
	reply_pdu.hdr.time_stamp = timeDoubleToDIS( theTime, absoluteTime );

	reply_pdu.orig_id      = pdu->recv_id;
	reply_pdu.recv_id      = pdu->orig_id;
	reply_pdu.request_id   = pdu->request_id;

	/*
	 *  Request applies to one of our entities?
	 */

	eid = findLocalEntity( &pdu->target_id );
	if ( eid >= 0 ) {

		e = &entities[eid];

		if (pdu->recv_id.sim_id.site_id == site &&
			( pdu->recv_id.sim_id.application_id == ALL_APPLIC ||
			  pdu->recv_id.sim_id.application_id == application )) {
	
			reply_pdu.acknowledge_flag   = 36;	/* per CALSPAN */
			reply_pdu.resp_flag    = 1;			/* per CALSPAN */

			switch (pdu->transfer_type) {

			/*
			 *  Someone wants us to take control of an entity
			 */

			case DISTransferTypeEntityControllerRequest:

				/*
				 *  the transferControlRequestCallback function is 
				 *  reponsible for determining if we can feasibly take 
				 *  control of the entity.
				 */

				if (transferControlRequestCallback != NULL &
					(*transferControlRequestCallback)(e, pdu) == 0) {

					status = DISxWritePDU( app, (dis_pdu *) & reply_pdu );
					status = (status == 0) ? 0 : -1;

					if (status == 0) {
						e->local = 1;
					}
				}
				else {
					error_return_needed = 1;
				}
				break;

		    /*
			 *  Control of this entity is requested by someone else.
			 */

			case DISTransferTypeEntityRequest:

				if (transferControlRequestCallback != NULL &&
					(*transferControlRequestCallback)(e, pdu) == 0) {

					status = DISxWritePDU( app, (dis_pdu *) & reply_pdu );
					status = (status == 0) ? 0 : -1;

					if (status == 0) {
						e->local = 0;
					}
				}
				else {
					error_return_needed = 1;
				}
				break;
			}

		}
	}
	else {
		/*
		 *  The target entity was not local to us.  Still, the PDU might 
         *  look like it is destined for us.  In this case, return an 
         *  error reply.
		 */

		if (pdu->recv_id.sim_id.site_id == site &&
			( pdu->recv_id.sim_id.application_id == ALL_APPLIC ||
			  pdu->recv_id.sim_id.application_id == application )) {

			error_return_needed = 1;
		}
	}

	if (error_return_needed) {

		reply_pdu.acknowledge_flag   = 36;	/* per CALSPAN */
		reply_pdu.resp_flag  = 5;			/* error state, per CALSPAN */

		status = DISxWritePDU(app, (dis_pdu *) & reply_pdu);
		status = (status == 0) ? 0 : -1;
	}

	return status;
}

int
acknowledgePDU ( dis_acknowledge_pdu *pdu )
{
	Entity_t   *e;
	int       eid;
	OutstandingRequestInfo_t *preq;

	preq = findRequestByRequestID ( pdu->request_id );

	if ( preq != NULL && 
		 preq->request_type == OUTSTANDING_REQUEST_TRANSFER_CONTROL ) {

		e = preq->e;

		/*
		 * no longer need to track the request
		 */

		removeRequest ( preq );

		/*
		 * Were we expecting to be granted control? If not, there is some sort
         * of error.
		 *
		 * This is a non-standard exchange defined by CALSPAN. We are 
         * processing a response to a control request.  The request was 
         * originated by us, the response we just received comes from the 
		 * controlling application.
		 */

		if ( pdu->acknowledge_flag == 36 ) {

			/*
			 * response flag set to "1" to indicate a success
			 *                      "5" indicates a failure
			 */

			switch (pdu->resp_flag) {
			case 1:
				if (e->controlRequestCallback) {
					(e->controlRequestCallback)( (dis_pdu *) pdu, 
												 e->callbackData );
				}
				e->local = 1;
				e->state = DIS_ENTITY_STATE_SIMULATING;
				e->pending_state = DIS_ENTITY_STATE_NONE;
				break;

			case 5:
				if (e->controlRequestCallback) {
					(e->controlRequestCallback)( (dis_pdu *) pdu, 
												 e->callbackData );
				}
				break;

			default:
				printf( "Unrecognized response flag in Acknowledge PDU: %d\n",
						pdu->resp_flag );
				break;
			}

			e->controlRequestCallback = NULL;
			e->callbackData = NULL;
		}
	}

	return 0;
}

int
dis_requestControl (Entity_t *e, 
					int (*callbackFn)(dis_pdu*, void *), 
					void *arg)
{
	dis_pdu pdu;
	int status;
	OutstandingRequestInfo_t *preq;

	memset ( &pdu, 0, sizeof(pdu) );

	if (e->local == -1 ||
		e->local == 1) {
		return -1;
	}

	e->callbackData = arg;
	e->controlRequestCallback = callbackFn;

	pdu.hdr.pdu_type = PDUTypeTransferControl;
	pdu.hdr.time_stamp = timeDoubleToDIS(theTime, absoluteTime);

	pdu.transfer_control.request_id = DISxIssueRequestID ( app );

	pdu.transfer_control.orig_id.sim_id.site_id = site;
	pdu.transfer_control.orig_id.sim_id.application_id = application;
	pdu.transfer_control.orig_id.entity_id = NO_ENTITY;

	pdu.transfer_control.recv_id.sim_id = e->entityId.sim_id;
	pdu.transfer_control.recv_id.entity_id = NO_ENTITY;

	pdu.transfer_control.target_id = e->entityId;

	pdu.transfer_control.reliability_service = 1;
	pdu.transfer_control.num_record_sets = 0;
	pdu.transfer_control.transfer_type = DISTransferTypeEntityRequest;

	/*
	 *  Add tracking information so that we know about this
	 *  outstanding request.
	 */

	preq = addRequest ( pdu.transfer_control.request_id );
	if (preq) {
		preq->request_type = OUTSTANDING_REQUEST_TRANSFER_CONTROL;
		preq->e = e;
		/* requests can timeout, but we don't track that, yet */
		preq->timeout_time = theTime + 5.0;
	}

	status = DISxWritePDU( app, & pdu );
	return (status == 0) ? 0 : -1;
}

int
setDataPDU (dis_set_data_pdu *pdu)
{
	Entity_t     *e;
	int          eid;
	dis_data_pdu reply_pdu;
	unsigned int i;
	int status;

	reply_pdu.hdr     = pdu->hdr;
	reply_pdu.hdr.pdu_type   = PDUTypeData;
	reply_pdu.hdr.time_stamp = timeDoubleToDIS( theTime, absoluteTime );

	reply_pdu.orig_id = pdu->recv_id;
	reply_pdu.recv_id = pdu->orig_id;
	reply_pdu.request_id  = pdu->request_id;

	DISInitializeDatumInfo (&reply_pdu.datum_info);

	eid = findEntity(&pdu->recv_id);
	if (eid < 0) {
		return -1;
	}
	else {
		e = &entities[eid];
	}

	for (i=0; i<pdu->datum_info.num_fixed_data; ++i) {
		switch ( pdu->datum_info.variable_datum[i].datum_id ) {
		case DatumOrientationX:
			break;
		case DatumOrientationY:
			break;
		case DatumOrientationZ:
			break;

		case DatumXVelocity:
			break;
		case DatumYVelocity:
			break;
		case DatumZVelocity:
			break;
		}
	}

	for (i=0; i<pdu->datum_info.num_variable_data; ++i) {
		switch ( pdu->datum_info.variable_datum[i].datum_id ) {
		case DatumGeocentricCoordinatesX:
			break;
		case DatumGeocentricCoordinatesY:
			break;
		case DatumGeocentricCoordinatesZ:
			break;
		}
	}

	/*
	 * Send reply
	 */

	status = DISxWritePDU(app, (dis_pdu *) & reply_pdu);
	return (status == 0) ? 0 : -1;
}

int
stopPDU (dis_stop_pdu *pdu)
{
	Entity_t     *e;
	int          eid;
	int          status = 0;
	int          need_reply = 1;
	int          all_local_entities = 0;
	dis_acknowledge_pdu reply_pdu;
	struct timeval tv;
	double changeTime;

	reply_pdu.hdr     = pdu->hdr;
	reply_pdu.hdr.pdu_type   = PDUTypeAcknowledge;
	reply_pdu.hdr.time_stamp = timeDoubleToDIS( theTime, absoluteTime );

	reply_pdu.orig_id = pdu->recv_id;
	reply_pdu.recv_id = pdu->orig_id;

	reply_pdu.request_id  = pdu->request_id;
	reply_pdu.acknowledge_flag = DISAcknowledgeFlagStop;
	reply_pdu.resp_flag = DISRequestStatusComplete;

	if (pdu->recv_id.sim_id.site_id == ALL_SITES) {
		all_local_entities = 1;
	}
	else if (pdu->recv_id.sim_id.site_id == site &&
		     pdu->recv_id.sim_id.application_id == ALL_APPLIC) {
		all_local_entities = 1;
	}
	else if (pdu->recv_id.sim_id.site_id == site &&
		     pdu->recv_id.sim_id.application_id == application &&
			 pdu->recv_id.entity_id == ALL_ENTITIES) {
		all_local_entities = 1;
	}

	DISTimeToTimeval( &pdu->real_time, &tv );
	changeTime = tv.tv_sec + tv.tv_usec / 1000000.0;

	if ( all_local_entities ) {
		int i;
		
		e = &entities[0];
		for ( i=0; i<entity_top; ++i, ++e ) {
			
			if (e->local != 1) {
				continue;
			}
			
			if ( changeTime <= theTime ) {
				e->state = DIS_ENTITY_STATE_STOPPED;
			}
			else {
				e->pending_state = DIS_ENTITY_STATE_STOPPED;
				e->pending_time = changeTime;
			}
			
			if ( pdu->behavior & DISFrozenBehaviorRunClock ) {
			}
		
			if ( pdu->behavior & DISFrozenBehaviorTransmit ) {
				e->emit_while_frozen = 1;
			}
		
			if ( pdu->behavior & DISFrozenBehaviorReceive ) {
			}
		}
	}
	else {

		eid = findEntity(&pdu->recv_id);
		if (eid < 0) {
			/* cannot comply, entity not found */
			reply_pdu.orig_id.entity_id = NO_ENTITY;
			reply_pdu.resp_flag = DISRequestStatusOther;
		}
		else {
			e = &entities[eid];

			if (e->local == 1) {

				if ( changeTime <= theTime ) {
					e->state = DIS_ENTITY_STATE_STOPPED;
				}
				else {
					e->pending_state = DIS_ENTITY_STATE_STOPPED;
					e->pending_time = changeTime;
				}
			
				if ( pdu->behavior & DISFrozenBehaviorRunClock ) {
				}
			
				if ( pdu->behavior & DISFrozenBehaviorTransmit ) {
					e->emit_while_frozen = 1;
				}
			
				if ( pdu->behavior & DISFrozenBehaviorReceive ) {
				}
			}
			else {
				need_reply = 0;
			}
		}
	}
	
	/*
	 * Send reply
	 */
	if (need_reply) {
		status = DISxWritePDU(app, (dis_pdu *) & reply_pdu);
	}
	return (status == 0) ? 0 : -1;
}

int
startPDU (dis_start_pdu *pdu)
{
	Entity_t     *e;
	int          eid;
	int          status = 0;
	int          need_reply = 1;
	int          all_local_entities = 0;
	dis_acknowledge_pdu reply_pdu;
	struct timeval tv;
	double changeTime;

	reply_pdu.hdr     = pdu->hdr;
	reply_pdu.hdr.pdu_type   = PDUTypeAcknowledge;
	reply_pdu.hdr.time_stamp = timeDoubleToDIS( theTime, absoluteTime );

	reply_pdu.orig_id = pdu->recv_id;
	reply_pdu.recv_id = pdu->orig_id;

	reply_pdu.request_id  = pdu->request_id;
	reply_pdu.acknowledge_flag = DISAcknowledgeFlagStop;
	reply_pdu.resp_flag = DISRequestStatusComplete;

	if (pdu->recv_id.sim_id.site_id == ALL_SITES) {
		all_local_entities = 1;
	}
	else if (pdu->recv_id.sim_id.site_id == site &&
		     pdu->recv_id.sim_id.application_id == ALL_APPLIC) {
		all_local_entities = 1;
	}
	else if (pdu->recv_id.sim_id.site_id == site &&
		     pdu->recv_id.sim_id.application_id == application &&
			 pdu->recv_id.entity_id == ALL_ENTITIES) {
		all_local_entities = 1;
	}

	DISTimeToTimeval( &pdu->real_time, &tv );
	changeTime = tv.tv_sec + tv.tv_usec / 1000000.0;

	if ( all_local_entities ) {
		int i;

		e = &entities[0];
		for ( i=0; i<entity_top; ++i, ++e ) {

			if (e->local != 1) {
				continue;
			}

			if ( changeTime <= theTime ) {
				e->state = DIS_ENTITY_STATE_SIMULATING;
			}
			else {
				e->pending_state = DIS_ENTITY_STATE_SIMULATING;
				e->pending_time = changeTime;
			}

			e->emit_while_frozen = 0;
		}
	}
	else {
		eid = findEntity( &pdu->recv_id );
		if (eid < 0) {
			/* cannot comply. entity not found */
			reply_pdu.orig_id.entity_id = NO_ENTITY;
			reply_pdu.resp_flag = DISRequestStatusOther;
		}
		else {
			e = &entities[eid];

			/*
			 * Is entity local ?
			 */

			if ( e->local == 1 ) {

				if ( changeTime <= theTime ) {
					e->state = DIS_ENTITY_STATE_SIMULATING;
				}
				else {
					e->pending_state = DIS_ENTITY_STATE_SIMULATING;
					e->pending_time = changeTime;
				}

				e->emit_while_frozen = 0;
			}
			else {
				need_reply = 0;
			}
		}
	}

	/*
	 * Send reply
	 */

	if (need_reply) {
		status = DISxWritePDU(app, (dis_pdu *) & reply_pdu);
	}
	return (status == 0) ? 0 : -1;
}

/*
 *  dis_canSimulate ( int eid )
 *
 *  Returns 1 (TRUE) if the entity is local and it is int the simulating state
 */

int
dis_canSimulate ( int eid )
{
	Entity_t *e;

	if ( eid >= 0 || eid <= entity_top ) {
		e = &entities[eid];
		if ( e->local == 1 && e->state == DIS_ENTITY_STATE_SIMULATING ) {
			return 1;
		}
	}

	return 0;
}

/*
 *  dis_shouldTransmitPDUs ( Entity_t *e )
 *
 *  Determines if an entity should emit entity state PDUs based on 
 *  protocol rules
 */

int
dis_shouldTransmitPDUs ( Entity_t *e )
{
	int result = 0;

	if (e->local == 1) {
		if ( e->state == DIS_ENTITY_STATE_SIMULATING ) {
			result = 1;
		}
		else if ( e->state == DIS_ENTITY_STATE_STOPPED && 
				  e->emit_while_frozen ) {
			result = 1;
		}
	}

	return result;
}

/*
 *  d i s _ s n o o p
 *
 *  Process incoming PDUs for the specified number of milliseconds, thus
 *  priming the enity table.  This is called only during program
 *  initialization, when a complete entity table is required.
 */

int
dis_snoop ( int millisec )
{
	int interval_millisec = 500;

	if (disInUse) {
		if (disAbsoluteTime)
			dis_setTimeAbsolute();
		else
			dis_setTime(curTime);
		dis_receive();
	}

	dis_receive ();

	while ( millisec > 0 ) {
		if ( millisec < interval_millisec ) {
			interval_millisec = millisec;
		}
		usleep ( interval_millisec * 1000 );

		updateSimTimeFromSystemClock ();

		if (disInUse) {
			if (disAbsoluteTime)
				dis_setTimeAbsolute();
			else
				dis_setTime(curTime);
			dis_receive(); 
		}

		dis_receive ();
		millisec -= interval_millisec;
	}

	return 0;
}  



