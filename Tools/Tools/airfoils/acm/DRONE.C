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
  *  The original "Smart Drone" code was created by
  *  Jason Nyberg (nyberg@ctron.com).  Enhancements added by Riley Rainey.
  */

#include "pm.h"
#include "alarm.h"

extern double efrandom(void);

/*
 *  convert target plane coords from world to drone's
 */

void
myCoordSys(craft * c, craft * p, VPoint * pos, VPoint * vel)
{
	VPoint    tpos;

	VTransform(&p->prevSg, &c->XYZtoNED, &tpos);
	VReverseTransform_(&tpos, &c->trihedral, pos);
	VTransform_(&p->Cg, &c->XYZtoNED, &tpos);
	VReverseTransform_(&tpos, &c->trihedral, vel);
}

void
unholdFireAlarm(char *arg1, char *arg2)
{
	craft    *c = (craft *) arg1;

	if (c->holdCount > 0) {
		c->holdCount--;
	}
}

/* 
 *  Dumbly choose the closest hostile plane to be target
 */

int
pickTarget(craft * c)
{
	int       i, target = -1;
	craft    *p;
	double    d, min;
	VPoint    pos, vel;

	min = 100000000.0;
	for (i = 0, p = ptbl; i < MAXPLAYERS; ++i, ++p) {

		if (p->pIndex == c->pIndex || p->team == c->team ) {
			continue;
		}

		if ( p->type == CT_PLANE || 
			 p->type == CT_DIS_PLANE ||
			 p->type == CT_DRONE) {


			myCoordSys(c, p, &pos, &vel);

			d = mag( pos );

			if (d < min) {
				min = d;
				target = p->pIndex;
			}
		}
	}

	return target;
}

/*
 *  droneFlyTo
 *
 *  Generate sitck/rudder controls to move the plane to the specified
 *  geocentric point.
 */

void
droneFlyTo ( craft *c, VPoint *pos )
{
	double d;
	double phi_rad;

	d = sqrt(pos->x * pos->x + pos->y * pos->y + pos->z * pos->z);

	/*
	 *  DRONE_FACTOR defines just how hard a drone will maneuver into position.
	 *
	 *  Change to a lower or higher value to make it easier or harder.
	 *  Make sure the value is greater than 0.0 and 1.0
	 *  0.2-0.3 makes for good gun practice, 1.0 is virtually impossible to 
	 *  shake.
	 */

#define DRONE_MAX_Se	(droneAggressiveness)
#define DRONE_MAX_Sa 	(droneAggressiveness)
#define DRONE_MAX_Sr	((droneAggressiveness) * 0.1)

	/*
	 *  A lot of conventional 1V1 air combat involves keeping your lift
	 *  vector on the target aircraft.  Phi is the computed angle between our
	 *  target and the lift vector (simplified to be just the negative Z-axis).
	 */

	phi_rad = atan2 ( pos->y, -pos->z );

	/*
	 *  If the target is behind our 3/9-line, we are defensive.  Pull maximum
	 *  G's into the target (after rolling into him).
	 *
	 *  If we are behind of the target, perform pure pursuit (until the code 
	 *  gets a bit smarter).
	 */

	if (pos->x < 0.0) {

		/*
		 *  Wait for lift vector to be close to where we want it before
		 *  pulling G's.
		 */
		if ((fabs(phi_rad) > DEGtoRAD(130.0)) ||
			 fabs(phi_rad) < DEGtoRAD(50.0)) {
			c->Se = DRONE_MAX_Se;
		}
	}
	else {
		c->Se = - pos->z / d * 3.0;
	}

	/*
	 *  Put the lift vector on the target.
	 */

	if (pos->z == 0.0) {
		c->Sa = 0.0;
	}
	else if (fabs (phi_rad) > DEGtoRAD(150.0)) {
		c->Sa = - 0.2 * phi_rad;
	}
	else if (fabs (phi_rad) > DEGtoRAD(20.0)) {
		c->Sa = phi_rad;
	}
	else {
		c->Sa = 0.2 * phi_rad;
	}

	/*
	 *  Don't use the rudder, for now.
	 */

	c->Sr = 0.0;

	/*
	 *  Constrain control surface positions to valid values.
	 */

	if (c->Se > DRONE_MAX_Se)
		c->Se = DRONE_MAX_Se;
	else if (c->Se < -DRONE_MAX_Se)
		c->Se = -DRONE_MAX_Se;

	if (c->Sa > DRONE_MAX_Sa)
		c->Sa = DRONE_MAX_Sa;
	else if (c->Sa < -DRONE_MAX_Sa)
		c->Sa = -DRONE_MAX_Sa;

	if (c->Sr > DRONE_MAX_Sr)
		c->Sr = DRONE_MAX_Sr;
	else if (c->Sr < -DRONE_MAX_Sr)
		c->Sr = -DRONE_MAX_Sr;

	c->Sa = - c->Sa;
	c->Se = - c->Se;

}

/*
 *  Drone flight management in Attack Mode (this is most common)
 */

int
droneCalculationsAttackMode ( craft * c )
{

	double    d, phi, htime;
	VPoint    pos, vel;
	int       x, y;

	/*
	 * Our opponent has exited?  Return to engagement initiation point.
	 */

	if ( (c->flags & FL_END_GAME_DRONE ) ) {
		 if ( c->curOpponent != -1 &&
			  ptbl[c->curOpponent].type == CT_FREE ) {
			 c->curDroneMode = DM_RETURN;
		 }
	}

	/*
	 *  No opponent, or opponent isn't there anymore
	 */

	else if (c->curOpponent == -1 ||
		ptbl[c->curOpponent].type == CT_FREE) {
		c->curOpponent = pickTarget(c);
		c->holdCount = 0;
	}

	if (c->curOpponent != -1) {

		myCoordSys(c, &(ptbl[c->curOpponent]), &pos, &vel);

		droneFlyTo ( c, &pos );

		/*
		 *  Fire at the target, if appropriate.  The newDrone() code has
		 *  already selected AIM-9's as our weapon.
		 *
		 *  We'll have to figure out a way to do lead pursuit in order to 
		 *  fire the cannon; we do pure pursuit now, which is the 
		 *  (somewhat) right thing to fire a missile.
		 */

		if (c->holdCount == 0 &&
			doWeaponDisplay(c, (viewer *) NULL, &x, &y) == 1) {
			fireWeapon(c);
			htime = 10.0 + (efrandom() + efrandom()) * 5.0;
			addAlarm(htime, unholdFireAlarm, (char *) c, NULL);
			c->holdCount++;
		}


	}

	return 0;
}

/*
 *  Drone flight management in Return Modes
 */

int
droneCalculationsReturnMode ( craft * c )
{
	VPoint tpos, pos, vel;
	double dist_meters, closure_meters_per_sec;
	int result = 0;

	/*
	 *  Generate body relative position of return point
	 */

	VTransform(&c->interceptStartPoint, &c->XYZtoNED, &tpos);
	VReverseTransform_(&tpos, &c->trihedral, &pos);

	/*
	 *  Convert NED velocity to body-relative velocity
	 */

	VReverseTransform_(&c->Cg, &c->trihedral, &vel);

	dist_meters = mag( pos );

	closure_meters_per_sec = 
		( vel.x * pos.x + vel.y * pos.y + vel.z * pos.z ) / dist_meters;

	droneFlyTo ( c, &pos );

	/*
	 * If we are in return mode and turned within 30 degrees towards 
	 * the return point, enter the "return-captured" mode.
	 */

	if ( c->curDroneMode == DM_RETURN ) {

		if ( closure_meters_per_sec > 0.866 * FEETtoMETERS(c->VT) ) {
			c->curDroneMode = DM_RETURN_CAPTURED;
		}
	}

	/*
	 * If we are in return-capture mode and start to move away from the
	 * intercept point, then we're done; destroy the aircraft.
	 */

	else {
		if ( closure_meters_per_sec < 0.0 ) {
			killPlayer ( c );
			result = 1;
		}
	}

	return result;
}

int
droneCalculations( craft * c )
{
	int result;

	switch (c->curDroneMode) {

	case DM_ATTACK:
		result = droneCalculationsAttackMode ( c );
		break;

	case DM_RETURN:
		result = droneCalculationsReturnMode ( c );
		break;

	case DM_RETURN_CAPTURED:
		result = droneCalculationsReturnMode ( c );
		break;
	}
	return result;
}

#ifdef USE_REQUEST_CONTROL

extern int controlRequestCallback( dis_pdu *pdu, void *pu );

/*
 *  endGameDistanceCheck
 *
 *  This alarm function is invoked periodically (once per second) to look for
 *  hostile aircraft in the proxmity of this DIS entity -- the entity is
 *  owned by another application at this point, we may ask to take control of
 *  it, if a hostile aircraft comes within range.
 */

void
endGameDistanceCheck (char * p1, char *p2)
{
	double range_meters;
	VPoint del;
	craft *p;
	craft *c =  (craft * ) p1;
	viewer *u = (viewer *) p2;
	int i;
	int done = 0;
	double threshold_meters;

	if ( c->type == CT_DIS_PLANE && (c->flags & FL_END_GAME_DRONE) ) {

		/*
		 * Determine the appropriate range threshold; if one was specified
		 * on the command line, use that. Otherwise use the lock range from
		 * the aicraft definition.
		 */

		if ( end_game_threshold_meters <= 0.0 ) {
			threshold_meters = FEETtoMETERS( c->cinfo->radarTRange * NM );
		}
		else {
			threshold_meters = end_game_threshold_meters;
		}

		for ((i = 0, p = ptbl); (i < MAXPLAYERS) && ( ! done ); (++i, ++p)) {

			/*
			 * Skip this entry if:
			 *
			 *   1) It's the entry for our own aircraft.
			 *   2) The craft isn't a hostile.
			 */

			if (p->pIndex == c->pIndex || c->team == p->team ) {
				continue;
			}

			if ( p->type == CT_PLANE || 
				 p->type == CT_DIS_PLANE ||
				 p->type == CT_DRONE) {

				del.x = p->Sg.x - c->Sg.x;
				del.y = p->Sg.y - c->Sg.y;
				del.z = p->Sg.z - c->Sg.z;

				range_meters = mag( del );

				/*
				 * If the distance is within our threshold, then
				 * initiate a control request.
				 */

				if ( range_meters <= threshold_meters ) {
					Entity_t * e = dis_getEntityTable();

					/*
					 *  Record start point of engagement; we will return
					 *  to this point after a kill.
					 */

					c->interceptStartPoint = c->Sg;

					dis_requestControl ( &e[c->disId],
										 controlRequestCallback, u );

					done = 1;
				}

			}
		}
	}

	/*
	 *  This craft is no longer a DIS plane.  No need to continue proximity
	 *  testing.
	 */

	else {
		done = 1;
	}

	/*
	 *  If nothing was within our threshold range, look again after one
	 *  second has elapsed.
	 */

	if ( ! done ) {
#ifdef DEBUG
		printf ("not done: adding alarm\n");
#endif
		addAlarm ( 1.0, endGameDistanceCheck, p1, p2 );
	}

#ifdef DEBUG
	printf ("distance check exit\n");
#endif
			
}

#endif
