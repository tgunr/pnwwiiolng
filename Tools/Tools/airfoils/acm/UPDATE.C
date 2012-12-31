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
#include "alarm.h"
#include <X11/Xutil.h>
#include <sys/time.h>

double    lastTime = 0.0, sum = 0.0;
long      frameCount = 0;

#ifdef HAVE_DIS
long      sendCount = 0;

#endif

double    gameEmptyTime = 0.0;

extern int flightCalculations(craft * c);
extern int missileCalculations(craft * c);
extern void doEvents(void), doViews(void), flapControl(craft * c),
          resupplyCheck(char *arg1, char *arg2);
extern void alarmCheck(double delta);
extern void blackBoxInput(void), blackBoxOutput(void);

int       cur = 0;
static int warned = 0;

#ifdef HAVE_DIS

#include "dis.h"

#define FT_TO_M(x)	(0.3048*(x))
#define M_TO_FT(x)	((x)/0.3048)

/*
 *  Update DIS entity state corresponding to te given craft
 */

void
disBroadcast(craft * p)
{
	double    location[3], velocity[3], linearAcceleration[3];
	double    orientation[3], angularVelocity[3];
	VPoint    tmp, tmp1;
	VMatrix   ABCtoXYZ, NEDtoXYZ;
	double    dt;
	int       res;
	static double base = -1;

	if (base < 0)
		base = curTime;

/*
 *  Well, this is a bit strange, but ACM's coordinate system for positions
 *  is meters in the Geocentric; for velocities are expressed as feet per
 *  second in the local NED [north-east-down] system.
 */

	location[0] = p->Sg.x;
	location[1] = p->Sg.y;
	location[2] = p->Sg.z;

	tmp.x = FEETtoMETERS(p->Cg.x);
	tmp.y = FEETtoMETERS(p->Cg.y);
	tmp.z = FEETtoMETERS(p->Cg.z);
	VReverseTransform_(&tmp, &p->XYZtoNED, (VPoint *) & velocity[0]);

	dt = curTime - p->disLastTime;
	p->disLastCg = p->Cg;

/*
 *  Derive ECI [Geocentric] heading, pitch, roll
 */
	transpose(&p->XYZtoNED, &NEDtoXYZ);
	/* the trihedral is an "ABCtoNED" transformation */
	VMatrixMultByRank(&p->trihedral, &NEDtoXYZ, &ABCtoXYZ, 3);
	matrixToEuler(&ABCtoXYZ, &orientation[0], &orientation[1], &orientation[2]);
	/*
	 *  Body frame angular velocities.
	 */

	angularVelocity[0] = p->p;	/* x-axis */
	angularVelocity[1] = p->q;	/* y-axis */
	angularVelocity[2] = -p->r;	/* z-axis */

	/*
	 *  Transform linear acceleration vector
	 *  from body coordinates to ECI system
	 */

	tmp = p->linAcc;
	tmp.x = FEETtoMETERS(tmp.x);
	tmp.y = FEETtoMETERS(tmp.y);
	tmp.z = FEETtoMETERS(tmp.z);
	VTransform_(&tmp, &ABCtoXYZ, (VPoint *) & linearAcceleration[0]);

#ifdef notdef
	printf("X = %7.4f %7.4f %7.4f\n",
		   XYZtoABC.m[0][0], XYZtoABC.m[0][1], XYZtoABC.m[0][2]);
	printf("Y = %7.4f %7.4f %7.4f\n",
		   XYZtoABC.m[1][0], XYZtoABC.m[1][1], XYZtoABC.m[1][2]);
	printf("Z = %7.4f %7.4f %7.4f\n",
		   XYZtoABC.m[2][0], XYZtoABC.m[2][1], XYZtoABC.m[2][2]);

	buildEulerMatrix(orientation[2], orientation[1], orientation[0],
					 &XYZtoABC);
	printf("---\nX = %7.4f %7.4f %7.4f\n",
		   XYZtoABC.m[0][0], XYZtoABC.m[0][1], XYZtoABC.m[0][2]);
	printf("Y = %7.4f %7.4f %7.4f\n",
		   XYZtoABC.m[1][0], XYZtoABC.m[1][1], XYZtoABC.m[1][2]);
	printf("Z = %7.4f %7.4f %7.4f\n",
		   XYZtoABC.m[2][0], XYZtoABC.m[2][1], XYZtoABC.m[2][2]);

	printf("ECI orientation [deg] (h,p,r) = %7.2f, %7.2f, %7.2f\n",
		   RADtoDEG(orientation[0]), RADtoDEG(orientation[1]),
		   RADtoDEG(orientation[2]));

	printf("ECI angular velocities [deg/sec] (h,p,r) = %7.2f, %7.2f, %7.2f\n",
		   RADtoDEG(angularVelocity[0]), RADtoDEG(angularVelocity[1]),
		   RADtoDEG(angularVelocity[2]));
#endif

	res = dis_entityState(p->disId, location, velocity, linearAcceleration,
						  orientation, angularVelocity);
	if (res) {
		sendCount++;
#ifdef notdef
		printf("%6.2f: h=%8.2f, p=%8.2f, r=%8.2f, p=%7.2f, q=%7.2f, r=%7.2f\n",
			   curTime - base,
			   RADtoDEG(orientation[0]),
			   RADtoDEG(orientation[1]),
			   RADtoDEG(orientation[2]),
			   RADtoDEG(angularVelocity[0]),
			   RADtoDEG(angularVelocity[1]),
			   RADtoDEG(angularVelocity[2]));
		printf("%.2f %.1f %.1f %.1f %.1f %.1f %.1f\n",
			   curTime - base,
			   RADtoDEG(orientation[0]),
			   RADtoDEG(orientation[1]),
			   RADtoDEG(orientation[2]),
			   RADtoDEG(angularVelocity[0]),
			   RADtoDEG(angularVelocity[1]),
			   RADtoDEG(angularVelocity[2]));
#endif
	}
}

void
disGetState(craft * p)
{
	static double base = -1;
	double    location[3];
	double    velocity[3];
	double    orientation[3];
	VPoint    tmp;
	VMatrix   ABCtoXYZ;
	int       res;

	if (base < 0)
		base = curTime;

	res = dis_getEntityState(p->disId, location, velocity, orientation);
	/* if (res != 0) should not happen? */

	p->prevSg = p->Sg;
	p->Sg.x = location[0];
	p->Sg.y = location[1];
	p->Sg.z = location[2];

	DISGeocentricToWorldCoordinates
		((dis_world_coordinates *) & p->Sg, &p->w);
	GenerateWorldToLocalMatrix(&p->w, &p->XYZtoNED);

	tmp.x = M_TO_FT(velocity[0]);
	tmp.y = M_TO_FT(velocity[1]);
	tmp.z = M_TO_FT(velocity[2]);
	VTransform_(&tmp, &p->XYZtoNED, &p->Cg);

/*
 *  Compute the "ABCtoNED" trihedral from the DIS euler angles
 */

	p->curHeading = orientation[0];
	p->curPitch = orientation[1];
	p->curRoll = orientation[2];

	buildEulerMatrix(p->curRoll, p->curPitch, p->curHeading,
					 &ABCtoXYZ);
	VMatrixMultByRank(&ABCtoXYZ, &p->XYZtoNED, &p->trihedral, 3);

/*
 *  Now derive NED heading, pitch and roll from adjusted trihedral
 */

	euler(p);

#ifdef notdef
	printf("%d: h=%.0f, p=%.0f, r=%.0f  ",
		   p->disId,
		   RADtoDEG(p->curHeading),
		   RADtoDEG(p->curPitch),
		   RADtoDEG(p->curRoll));
	printf("%.2f %.1f %.1f %.1f\n",
		   curTime - base,
		   RADtoDEG(orientation[0]),
		   RADtoDEG(orientation[1]),
		   RADtoDEG(orientation[2]));
#endif
}

#endif

void
updateSimTimeFromSystemClock ()
{
	struct timeval tp;
	struct timezone tzp;

	gettimeofday(&tp, &tzp);
	curTime = (double) tp.tv_sec + (double) tp.tv_usec / 1000000.0;
}


int
redraw(void)
{

	int       i, gameEmpty;
	craft    *p;

	updateSimTimeFromSystemClock ();


	if (lastTime == 0.0) {
		lastTime = curTime - 0.1;
	}

/*
 *  Check if "curTime" ever equals "lastTime", if so, warn that REAL_DELTA_T
 *  probably should not be used on this system.
 */

	if (curTime == lastTime) {
		curTime += 0.0001;
		if (warned == 0) {
			warned = 1;
			fprintf(stderr,
					"Hmm. We seem to have performed a complete inner loop in zero elapsed time.\n\
This seems suspicious.  Perhaps gettimeofday() has a relatively coarse\n\
granularity on this system.\n\
This server should probably be recompiled without REAL_DELTA_T defined.\n");
		}
	}

	if (real_delta_t) {

		deltaT = curTime - lastTime;

/*
 *  If the amount of real elapsed time is unreasonably long, then
 *  make it one update interval.
 */

#ifdef FIXME
		if (deltaT > (double) ((UPDATE_INTERVAL / 1000000.0) * 4)) {
			deltaT = UPDATE_INTERVAL / 1000000.0;
		}
#endif

	}
	else {
		deltaT = update_interval_millisec / 1000.0;
	}

	lastTime = curTime;
	halfDeltaTSquared = 0.5 * deltaT * deltaT;

#ifdef HAVE_DIS
	if (disInUse) {
		if (disAbsoluteTime)
			dis_setTimeAbsolute();
		else
			dis_setTime(curTime);
		dis_receive();
	}
#endif

	gameEmpty = 1;

	doEvents();

	for ((i = 0, p = ptbl); i < MAXPLAYERS; (++i, ++p)) {
		if ((p->type == CT_PLANE) && !(p->flags & FL_BLACK_BOX)) {
			gameEmpty = 0;

			if (flightCalculations(p) == 1) {
				killPlayer(p);
			}
			doWeaponUpdate(p);
			flapControl(p);
#ifdef HAVE_DIS
			if (disInUse)
				disBroadcast(p);
#endif
		}
	}

	for ((i = 0, p = ptbl); i < MAXPLAYERS; (++i, ++p)) {
		if ((p->type == CT_DRONE) && !(p->flags & FL_BLACK_BOX)) {

/*
 *  Determine what actions the drone will take
 */

			if (droneCalculations(p) == 1) {
				killPlayer(p);
			}

/*
 *  Calling flightCalculations makes drones adhere to the flight model.
 */

			if (flightCalculations(p) == 1) {
				killPlayer(p);
			}
			doWeaponUpdate(p);
			flapControl(p);

			doDroneRadar ( p );

#ifdef HAVE_DIS
			if (disInUse)
				disBroadcast(p);
#endif
		}
	}

#ifdef HAVE_DIS
	if (disInUse) {
		for ((i = 0, p = ptbl); i < MAXPLAYERS; (++i, ++p)) {
			if (p->type == CT_DIS_PLANE)
				disGetState(p);
		}
		for ((i = 0, p = mtbl); i < MAXPROJECTILES; (++i, ++p)) {
			if (p->type == CT_DIS_MUNITION)
				disGetState(p);
		}
	}
#endif

	blackBoxInput();

	for ((i = 0, p = mtbl); i < MAXPROJECTILES; (++i, ++p)) {
		if (p->type == CT_MISSILE) {
			if (missileCalculations(p) == 1) {
				killMissile(p, (craft *) NULL);
			}
#ifdef HAVE_DIS
			else if (disInUse) {
				disBroadcast(p);
			}
#endif
		}
		else if (p->type == CT_CANNON || p->type == CT_DIS_CANNON) {
			if (cannonCalculations(p) == 1) {
				killMissile(p, (craft *) NULL);
			}
		}
		else if (p->type == CT_EXPLOSION) {
			--(p->flameDuration);
			if ((--p->duration) <= 0)
				p->type = CT_FREE;
		}
	}

	lookForImpacts();

	if (real_delta_t) {
		doViews();
	}
	else if (cur++ % redraw_interval == 0) {
		doViews();
	}

	blackBoxOutput();

	alarmCheck(deltaT);

	if (watch_frame_rate) {
		sum += deltaT;
		frameCount++;
		if (frameCount % 100 == 0) {
			printf("rate is %lf frames/second\n", (double) frameCount / sum);
#ifdef HAVE_DIS
			if (disInUse)
				printf("DIS send rate is %lf packets/second\n",
				   (double) sendCount / sum);
			sendCount = 0;
#endif
			sum = deltaT;
			frameCount = 1;
		}
	}

	/*
	 *  Nobody on this system active in the simulation? We are done.
	 */

	if (getViewerCount() == 0) {
		exit(0);
	}

	return 0;

}
