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

#include <stdio.h>
#include <math.h>
#define ALLOCATE_SPACE
#include "pm.h"
#ifdef AFDS
#include <afds.h>
#endif
#include "dis.h"

#ifdef WIN32
#include <float.h>
#endif

//extern void transpose();

int       debug = 0;

/*
 *  We keep a table of atmospheric constants for different altitudes.
 */

struct {
	double    alt;				/* altitude in feet */
	double    rho;				/* rho value (air density) */
	double    mach1;			/* speed of sound in feet per second */
}        *rhop, rhoTable[] = {

#ifdef notdef
	{
		0.0, 23.77, 1116.9
	},
	{
		2.0, 22.41, 1109.2
	},
	{
		4.0, 21.11, 1101.4
	},
	{
		6.0, 19.87, 1093.6
	},
	{
		8.0, 18.68, 1085.7
	},
	{
		10.0, 17.55, 1077.8
	},
	{
		15.0, 14.96, 1057.7
	},
	{
		20.0, 12.66, 1037.3
	},
	{
		25.0, 10.65, 1016.4
	},
	{
		30.0, 8.89, 995.1
	},
	{
		35.0, 7.365, 973.3
	},
	{
		40.0, 5.851, 968.5
	},
	{
		50.0, 3.618, 968.5
	},
#endif
	{
		60.0, 2.238, 968.5
	},
	{
		80.0, 0.9065, 980.0
	},
	{
		100.0, 0.3371, 1015.0
	},
	{
		120.0, 0.1340, 1053.0
	},
	{
		160.0, 0.02622, 1083.0
	},
	{
		100000.0, 0.02622, 1083.0
	}
};								/* a large value for alt at the end */

double    deltaT;				/* Update interval in seconds */
double    halfDeltaTSquared;	/* 0.5 * deltaT * deltaT */
double    CM, CN, C_Y;

extern double groundContactTime(craft * c, VPoint * contactSg);
extern int groundDynamics(craft * c, double startT, double CL, double CD, double CM, double w, double qS);

/*
 *  calcRho : Calculate air density and the speed of sound by interpolation.
 */

double
calcRho(double alt, double *mach)
{

	double    deltaAlt, b;
	extern void airProperties(double h, double *rho, double *mach1);
	double    rho;

	if (alt <= 60000.0) {
		airProperties(alt, &rho, mach);
		return rho;
	}

	alt = alt / 1000.0;

	for (rhop = rhoTable; alt > (rhop + 1)->alt; ++rhop);
	deltaAlt = (rhop + 1)->alt - rhop->alt;
	b = ((rhop + 1)->mach1 - rhop->mach1) / deltaAlt;
	*mach = rhop->mach1 + b * (alt - rhop->alt);
	b = ((rhop + 1)->rho - rhop->rho) / deltaAlt;
	return (rhop->rho + b * (alt - rhop->alt)) / 10000.0;

}

/*
 *  twoorder: solve linear second-order differential equation with initial
 *      conditions known.
 *
 *  y_prime_prime + d * y_prime + k * y = 0
 *
 *  given initial conditions:
 *        y == y.
 *        y_prime == v.
 *
 *  results are *newy and *newv for x+deltaT seconds into the future.
 *  
 */

void
twoOrder(double k, double d, double y, double v, double *newy, double *newv)
{

	double    s, s1, s2, t, ac, x, c1, c2, exp_s1_x, exp_s2_x;
	int       return_zero = 0;

	k = -k;
	d = -d;

	/*
	 *  Get root(s) to the charateristic equation
	 */

	ac = d * d - 4.0 * k;
	if (ac < 0.0) {
		/*
		 * Imaginary roots
		 */
		s = -d / 2.0;
		t = sqrt(-ac) / 2.0;
	}
	else if (ac == 0.0) {
		/*
		 *  one real root
		 */
		s = (-d + sqrt(ac)) / 2.0;
		t = 0.0;
	}
	else {

		/*
		 *  Two, unequal real roots
		 */

		s1 = (-d + sqrt(ac)) / 2.0;
		s2 = (-d - sqrt(ac)) / 2.0;
		c1 = (s2 * y - v) / (s2 - s1);
		c2 = y - c1;
		x = deltaT;
		exp_s1_x = exp(s1 * x);
		exp_s2_x = exp(s2 * x);
		*newy = c1 * exp_s1_x + c2 * exp_s2_x;
		*newv = c1 * s1 * exp_s1_x + c2 * s2 * exp_s2_x;
		return;
	}

	if (t == 0.0 || y == 0.0) {
		x = 0.0;
	}
	else {
		x = atan2(y * s - v, t * y) / t;
	}

	if (x == 0.0) {
		c1 = y;
	}
	else if (cos(t * x) != 0.0) {
		exp_s1_x = exp(s * x);
		if (exp_s1_x == 0.0) {
			return_zero = 1;
		}
		else {
			c1 = y / (exp_s1_x * cos(t * x));
		}
	}
	else {
		return_zero = 1;
	}

	if (return_zero) {
		*newy = 0.0;
		*newv = v;
		return;
	}

/*
 *  Now we can compute the values of y and v at the end of this
 *  time interval;
 */

#ifdef notdef
	printf("s = %g, t = %g, x = %g, y = %g, c1 = %g\n", s, t, x,
		   y, c1);

	if (fabs(y - (exp(s * x) * c1 * cos(t * x))) > 0.001)
		printf("*** possible error ***\n");

	*newv = exp(s * x) * c1 * (s * cos(t * x) - t * sin(t * x));
	if (fabs(v - *newv) > 0.001)
		printf("*** possible v error *** %g %g\n", v, *newv);
#endif

	x += deltaT;

	*newy = exp(s * x) * c1 * cos(t * x);
	*newv = exp(s * x) * c1 * (s * cos(t * x) - t * sin(t * x));

	if (isnan(*newy) || isnan(*newv)) {
		printf("Gotcha\n");
	}

#ifdef notdef
	printf("ny = %g, nv = %g\n", *newy, *newv);
#endif

}

/*
 *  calcCoefficients :  Calculate CLift and friends
 */

void
calcCoefficients(craft * c, double *CLift, double *CDrag)
{

	double    CDAlpha, CDBeta;
	register craftType *p = c->cinfo;

/*
 *  We used to interpolate these values, but now use several characteristic
 *  equations to compute these values for a given alpha value. The basic
 *  formulas are:
 *
 *
 *   C  = C        + (alpha * (C       + sin(curFlap) * cFlap ))
 *    L    LOrigin          LSlope
 *
 *
 *   C  = zero-lift-wave-and-body-drag + induced-drag +
 *    D     speed-brake-drag + flap drag + landing-gear-drag +
 *      drag-based-on-sideslip
 *
 *  There are independent equations defining drag resulting from alpha
 *  and beta values.  The hypoteneuse of those two values becomes the
 *  resultant CDrag value.
 */

	*CLift = interpolate(p->CLift, c->alpha) +
		sin(c->curFlap) * p->cFlap;

	CM = p->cmSlope + c->damageCM;

	CDAlpha = interpolate(p->CDb, c->mach) +
		*CLift * *CLift / (pi * p->aspectRatio);
	CDAlpha += sin(c->curSpeedBrake) * p->cSpeedBrake;
	CDAlpha += sin(c->curFlap) * p->cFlapDrag;
	CDAlpha += (sin(c->curGear[0])
				+ sin(c->curGear[1])
				+ sin(c->curGear[2])) / 3.0 * p->cGearDrag;

	if (fabs(c->beta) > p->betaStall)
		CN = interpolate(p->CnBeta, fabs(c->alpha)) * fabs(sin(c->beta));
	else
		CN = interpolate(p->CnBeta, fabs(c->alpha));

	CDBeta = p->CDBOrigin + p->CDBFactor *
		sin(c->beta + p->CDBPhase);

	*CDrag = sqrt(CDAlpha * CDAlpha + CDBeta * CDBeta);

	C_Y = p->CYbeta * c->beta /* * fabs(cos(c->beta))*/;

}

double
heading(VPoint * x)
{

	double    m;

	if (x->x == 0.0 && x->y == 0.0)
		return 0.0;

	if ((m = atan2(x->y, x->x)) < 0.0)
		return (pi * 2.0 + m);
	else
		return m;
}

/*
 *  Convert a transformation matrix into the equivalent
 *  heading, pitch and roll angles.
 */

#define EPSILON 1.0e-6

void
matrixToEuler(VMatrix * mt, double *heading, double *pitch, double *roll)
{
	double    sin_theta;

	sin_theta = -mt->m[2][0];

	if (fabs(sin_theta) > 1.0 - EPSILON) {
		/* we have the nose pointing very close to straight up or straight down,
		   set roll to zero and compute the resulting heading */

		*heading = atan2(-mt->m[0][1], mt->m[1][1]);
		if (*heading < 0.0)
			*heading += 2.0 * M_PI;

		if (sin_theta > 0.0)
			*pitch = M_PI / 2.0;
		else
			*pitch = -M_PI / 2.0;

		*roll = 0.0;
	}
	else {
		*heading = atan2(mt->m[1][0], mt->m[0][0]);
		if (*heading < 0.0)
			*heading += 2.0 * M_PI;

		*pitch = asin(sin_theta);

		*roll = atan2(mt->m[2][1], mt->m[2][2]);
	}
}

void
euler(craft * c)
{

	register double i, j, k, m;

/*
 *  Compute the heading ...
 */

	i = c->trihedral.m[0][0];
	j = c->trihedral.m[1][0];
	k = c->trihedral.m[2][0];

	if (i == 0.0 && j == 0.0)
		c->curHeading = 0.0;
	else if ((m = atan2(j, i)) < 0.0)
		c->curHeading = pi * 2.0 + m;
	else
		c->curHeading = m;

/*
 *  and Pitch ...
 */

	c->curPitch = -asin(k);

/*
 *  and Roll ...
 */

	c->curRoll = atan2(c->trihedral.m[2][1], c->trihedral.m[2][2]);

}

void
craftToGround(craft * c, VPoint * p, VPoint * g)
{

	VTransform_(p, &(c->trihedral), g);

}

void
calcGForces(craft * c, VPoint * f, double w)
{

	VPoint  t, t1;
	double	m_slugs;

	m_slugs = w / earth_g;

	t = *f;
	t.x = t.x / m_slugs;
	t.y = t.y / m_slugs;
	t.z = t.z / m_slugs;

	VReverseTransform_(&t, &c->trihedral, &c->linAcc);

	t.z -= earth_g;

	VReverseTransform_ (&t, &c->trihedral, &t1);

	c->G.x = t1.x / earth_g;
	c->G.y = t1.y / earth_g;
	c->G.z = t1.z / earth_g;

}

void
calcAlphaBeta(craft * c, double *alpha, double *beta)
{

	VPoint    C;
	double    h;

	if (mag(c->Cg) > 0.0) {
		VReverseTransform_(&c->Cg, &c->trihedral, &C);
		*beta = atan2(C.y, C.x);
		h = sqrt(C.y * C.y + C.x * C.x);
		*alpha = atan(C.z / h);
	}
	else {
		*alpha = 0.0;
		*beta = 0.0;
	}

}

/*
 *  buildEulerMatrix :  Build a transformation matrix based on the supplied
 *          euler angles.
 */

void
buildEulerMatrix(double roll, double pitch, double heading, VMatrix * m)
{

	register double sinPhi, cosPhi, sinTheta, cosTheta, sinPsi, cosPsi;

	sinPhi = sin(roll);
	cosPhi = cos(roll);
	sinTheta = sin(pitch);
	cosTheta = cos(pitch);
	sinPsi = sin(heading);
	cosPsi = cos(heading);

	m->m[0][0] = cosTheta * cosPsi;
	m->m[0][1] = sinPhi * sinTheta * cosPsi - cosPhi * sinPsi;
	m->m[0][2] = cosPhi * sinTheta * cosPsi + sinPhi * sinPsi;
	m->m[1][0] = cosTheta * sinPsi;
	m->m[1][1] = sinPhi * sinTheta * sinPsi + cosPhi * cosPsi;
	m->m[1][2] = cosPhi * sinTheta * sinPsi - sinPhi * cosPsi;
	m->m[2][0] = -sinTheta;
	m->m[2][1] = sinPhi * cosTheta;
	m->m[2][2] = cosPhi * cosTheta;
	m->m[0][3] = m->m[1][3] = m->m[2][3] = 0.0;
	m->m[3][0] = m->m[3][1] = m->m[3][2] = 0.0;
	m->m[3][3] = 1.0;

}

double
elevatorSetting(craft * c, double qS, double w)
{

	register double s, n, L, an;
	register craftType *p = c->cinfo;

	s = c->Se + c->SeTrim;

	if (s > 1.0)
		s = 1.0;
	else if (s < -1.0)
		s = -1.0;

/*
 *  Limit the load factor that will be generated.
 */

	L = p->effElevator * p->CLSlope;

	an = cos(s * p->effElevator);

	n = an * (- s * L + p->CLOrigin) * qS / w;

	if (n > 9.5)
		s = - (9.5 / (an * qS) * w - p->CLOrigin) / L;
	else if (n < -3.0)
		s = - (-3.0 / (an * qS) * w - p->CLOrigin) / L;

	return s;
}

double
aileronSetting(craft * c)
{
	double    Sa = c->Sa + c->SaTrim;

	if (Sa > 1.0) {
		Sa = 1.0;
	}
	else if (Sa < -1.0) {
		Sa = -1.0;
	}
	return Sa;
}

int
flightCalculations(craft * c)
{

	register craftType *p = c->cinfo;
	double    qS, s, CLift, CDrag;
	double    ClBeta;
	double    FLift, FDrag, FWeight, FSideForce;
	double    deltaRoll, deltaPitch, deltaYaw;
	double    y, newy;
	double    xa, xb, xc, xd, r0;
	double    dNorth, dEast, dmag, dHeading_rad;
	double	  mass_slugs;
	VPoint    F, Fg, tmp;
	VMatrix   mtx, new_trihedral;
	dis_entity_appearance appearance;
	int       airborne = 1;

	c->prevSg = c->Sg;
#ifndef FLAT_WORLD
	c->prevw = c->w;
#endif

	c->rho = calcRho(METERStoFEET(c->w.z), &c->mach1);
	calcAlphaBeta(c, &(c->alpha), &(c->beta));

/*
 *  A note about thrust:  Normal thrust diminishes in proportion to the
 *  decrease in air density.
 */

	c->VT = mag(c->Cg);
	c->mach = c->VT / c->mach1;

	if (disInUse) {
		appearance = dis_getEntityAppearance(c->disId);
		appearance &= ~(DISAppearanceAirAfterburnerOn | DISAppearancePlatformPowerplantOn);
	}

	if (c->fuel <= 0.0 || isFunctioning(c, SYS_ENGINE1) == 0) {
		c->curThrust = 0.0;
	}
	else {
		c->curThrust = (*p->thrust) (c);
		if (c->flags & FL_AFTERBURNER) {
			appearance |= DISAppearanceAirAfterburnerOn;
		}
		appearance |= DISAppearancePlatformPowerplantOn;
	}

	if (disInUse) {
		dis_setEntityAppearance(c->disId, appearance);
	}

	calcCoefficients(c, &CLift, &CDrag);
	ClBeta = interpolate(p->ClBeta, fabs(c->alpha));

#ifdef FLIGHTDEBUG
	if (debug) {
		printf("\n------\ntime = %g secs.\n", curTime);
		printf("alpha = %g deg, beta = %g deg\nCL = %g, CD = %g\n",
			   RADtoDEG(c->alpha), RADtoDEG(c->beta), CLift, CDrag);
		printf("CnBeta = %g, ClBeta = %g\n", CN, ClBeta);
	}
#endif

/*
 *  Compute the resultant force vector on the aircraft.  By the way, the
 *  variable "qS" should more properly be named "qS" -- it is the dynamic
 *  pressure times S, the reference wing area.
 */

	qS = c->rho * p->wingS * c->VT * c->VT * 0.5;
	s = p->wings;
	FLift = CLift * qS;
	FDrag = CDrag * qS;
	FSideForce = C_Y * qS;
	FWeight = p->emptyWeight + c->fuel;

	setBackgroundSound(c,
					   c->rpm,
					   (c->flags & FL_AFTERBURNER) ? 1 : 0,
					   qS / p->wingS);

#ifdef FLIGHTDEBUG
	if (debug) {
		printf("rho = %g, FLift = %g lbs, FThrust = %g lbs, ",
			   c->rho, FLift, c->curThrust);
		printf("FDrag = %g lbs\n", FDrag);
	}
#endif

/*
 *  These expressions convert lift and drag forces from wind axes to the
 *  aircraft fixed axes.  The conversion is based on the wind to
 *  aircraft transformation matrix supplied in "Airplane Design" by
 *  Donald Crawford (page 90).
 */

	F.x = c->curThrust
		+ FLift * sin(c->alpha)
		- FDrag * cos(c->alpha) * cos(c->beta);
	F.y = -FDrag * sin(c->beta)
		+ FSideForce;
	F.z = -FLift * cos(c->alpha)
		- FDrag * cos(c->beta) * sin(c->alpha);

/*
 *  Now calculate changes in position (Sg) and velocity (Cg).
 */

	if ((c->VT > p->maxNWS) || ((c->flags & FL_GND_CONTACT) == 0))
		c->flags &= ~FL_NWS;
	else
		c->flags |= FL_NWS;

	if (c->flags & FL_GND_CONTACT) {

		airborne = 0;

/*
 *  groundDynamics handles movement when we're in contact with the earth.
 */

		if (groundDynamics(c, 0.0, CLift, CDrag, CM, FWeight, qS)) {
			return 1;
		}

		craftToGround(c, &F, &Fg);

		if ((c->fuel -= fuelUsed(c) + c->leakRate * deltaT) <= 0.0) {
			c->fuel = 0.0;
			c->curThrust = 0.0;
			c->throttle = 0;
		}

		Fg.z += FWeight;

/* Nose wheel steering is only active when we cannot lift off -- cancel z */

		Fg.z = 0.0;

	}
	else {

/*
 *  Resolve moments
 */

		xa = p->wings * p->wings * p->wingS
			* c->rho * c->VT * p->Clp;
		xb = -p->I.m[0][0];
		xc = qS * p->wings * 2.0 *
			(p->Clda * - aileronSetting(c) * p->maxAileron
			 + ClBeta * c->beta
			 + p->Cldr * c->Sr * p->maxRudder)
			+ c->damageCL * qS;
		xd = c->p + xc / xa;
		r0 = -xd * xb / xa;
		deltaRoll = -xd * xb / xa * exp(-xa / xb * deltaT)
			- deltaT * xc / xa - r0;
		c->p = xd * exp(-xa / xb * deltaT) - xc / xa;

/*
 *  Resolve pitch-axis (Y-axis) changes
 */

		y = c->alpha + elevatorSetting(c, qS, FWeight) *
			p->effElevator;
		twoOrder(CM * qS * p->c / p->I.m[1][1],
				 (0.25 * p->wingS * c->rho * p->c * p->c *
				  c->VT * p->Cmq) / p->I.m[1][1],
				 y, c->q, &newy, &(c->q));
		deltaPitch = newy - y;

/*
 *  Resolve yaw-axis (Z-axis) changes.
 *
 *  We do some trickery here.
 *  If the absolute value of the sideslip angle is greater than 90 degrees,
 *  we trick the code into believing that the sideslip angle is the negative
 *  of its reciprocal value (e.g. -176 becomes -4 degrees).  We do this with
 *  the (somewhat inaccurate) assumption that the CN value for that angle is
 *  roughly equal to the other.
 */

		y = c->beta - c->Sr * p->effRudder;

		if (y > pi / 2.0) {
			y = pi - y;
		}
		else if (y < -pi / 2.0) {
			y = -pi - y;
		}

		twoOrder(CN * qS * s / p->I.m[2][2],
				 (p->wingS * c->rho * s * s *
				  c->VT * p->Cnr) / p->I.m[2][2],
				 y, c->r, &newy, &(c->r));
		deltaYaw = y - newy;

#ifdef FLIGHTDEBUG
		if (debug) {
			printf("p = %g deg/sec, qS = %g\
 deg/sec, r = %g deg/sec\n", RADtoDEG(c->p), RADtoDEG(c->q), RADtoDEG(c->r));
		}
#endif

/*
 *  Compute new aicraft trihedral, but don't set it yet.
 */

		buildEulerMatrix(deltaRoll, deltaPitch, deltaYaw, &mtx);
		VMatrixMultByRank(&mtx, &c->trihedral, &new_trihedral, 3);

		craftToGround(c, &F, &Fg);

/*
 * Compute fuel consumption
 */

		if ((c->fuel -= fuelUsed(c) + c->leakRate * deltaT) <= 0.0) {
			c->fuel = 0.0;
			c->curThrust = 0.0;
		}

		Fg.z += FWeight;

#ifdef FLIGHTDEBUG
		if (debug) {
			printf("v = %g kts (Mach %.3g), Fg = { %g, %g, %g }\n",
				   FPStoKTS(c->VT), c->mach, Fg.x, Fg.y, Fg.z);
			printf("F = { %g, %g, %g }\n",
				   F.x, F.y, F.z);
		}
#endif

/*
 *  on ground G-forces are calculated in gear.c
 */

		if (airborne) {
			calcGForces(c, &Fg, FWeight);
		}

/*
 *  Update our position (in flight mode).
 */
		mass_slugs = FWeight / earth_g;

		dNorth  = FEETtoMETERS(c->Cg.x * deltaT + Fg.x / mass_slugs * halfDeltaTSquared);
		dEast   = FEETtoMETERS(c->Cg.y * deltaT + Fg.y / mass_slugs * halfDeltaTSquared);
		c->w.z -= FEETtoMETERS(c->Cg.z * deltaT + Fg.z / mass_slugs * halfDeltaTSquared);

		dmag = sqrt(dNorth * dNorth + dEast * dEast);

		DISUpdateWorldCoordinatesEx (&c->w, dNorth / dmag, dEast / dmag, dmag,
			&dHeading_rad);

/*
 *  Update velocity vector based on acceleration
 */

		c->Cg.x += Fg.x / mass_slugs * deltaT;
		c->Cg.y += Fg.y / mass_slugs * deltaT;
		c->Cg.z += Fg.z / mass_slugs * deltaT;

/*
 *  Now rotate the trihedral and velocity vector to reflect the change in heading
 *  at the new spheroid location.
 */

		VIdentMatrix( &mtx );
		VRotate( &mtx, ZRotation, dHeading_rad );
		VMatrixMultByRank( &mtx, &new_trihedral, &c->trihedral, 3 );
		c->trihedral = new_trihedral;
		VTransform_ (&c->Cg, &mtx, &tmp);
		c->Cg = tmp;

		euler(c);

#ifdef FLIGHTDEBUG
		if (debug) {
			printf ("heading change %f (deg)\n", RADtoDEG(dHeading_rad));
		}
#endif

/*
 *  Post processing
 *
 *  Check to see if we make ground contact during this interval; if so,
 *  recalculate the aircraft position taking into account the ground
 *  dynamics after touchdown.
 */
		y = groundContactTime(c, &tmp);
		if (y >= 0.0) {
#ifdef FLAT_WORLD
			c->Sg = tmp;
#endif
			if (groundDynamics(c, y, CLift, CDrag, CM, FWeight, qS)) {
				return 1;
			}
			playSound(c, SoundTouchdown);
		}

	}							/* if GND_CONTACT */

	DISWorldCoordinatesToGeocentric(&c->w,
									(dis_world_coordinates *) & c->Sg);
	GenerateWorldToLocalMatrix(&c->w, &c->XYZtoNED);

#ifdef FLIGHTDEBUG
	if (debug) {
		printf("Altitude = %g ft\n", c->w.z);
		printf("Euler angles RPY { %g, %g, %g }\n",
			   RADtoDEG(c->curRoll),
			   RADtoDEG(c->curPitch), RADtoDEG(c->curHeading));
		printf("Cg = { %g, %g, %g }  ", c->Cg.x, c->Cg.y, c->Cg.z);
		printf("Sg = { %g, %g, %g }\n", c->Sg.x, c->Sg.y, c->Sg.z);
	}
#endif

#ifdef AFDS
	executeFlightPlan ( c, deltaT );

	/*
	 *  Flight Director Control Law + integration
	 */

	AFDSIntegrate ( c, deltaT );
#endif

	return 0;
}
