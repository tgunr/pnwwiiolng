
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
#include <stdio.h>
#include <string.h>

#include "pm.h"

int       select_aim120(craft *);
int       display_aim120(craft *, craftType *, viewer *, int *, int *);
int       update_aim120(craft *);
int       fire_aim120(craft *);
int       getIRTarget(craft * c, VPoint * t, double scanSlope);
extern int fireMissile(craft * c, int ind);
extern FILE *acm_fopen(char *, char *);

weaponDesc aim120Desc =
{
	WK_AIM120,
	select_aim120,				/* select */
	update_aim120,				/* update */
	display_aim120,				/* display procedure */
	fire_aim120,					/* fire */
	(int (*)(craft *)) NULL,	/* fire button release */
};

int       hasFired[MAXPLAYERS];
static int count[MAXPLAYERS];

/*
 *  aim120 selection function
 *
 *  A selection function normally determines whether there are any weapons
 *  of this type on-board.  If so, and the weapon system is functional
 *  (in other words, undamaged) then return 1; otherwise return 0.
 */

/*ARGSUSED */
int
select_aim120(craft * c)
{

	hasFired[c->pIndex] = 0;
	count[c->pIndex] = countOrdinance(c, "aim120");
	return 1;

}

static double
Rmax(craft *c)
{
	return 40.0 * NM;
}

void
computeASECircleParameters(craft *c,
						   double * ASE_diameter_millirad,
						   double * ASE_dot_az_millirad,
						   double * ASE_dot_el_millirad)
{
	double range_feet, rmax_feet = Rmax(c), hs, omegay, omegap, h;
	VPoint v, t, vrel;
	craft *target;
	radarInfo *pr = NULL, *p;
	int i;

	/*
	 * find target information entry in the radar info table
	 */

	for (i = 0, p = c->rinfo; i < c->rtop; ++i, ++p) {
		if (c->curRadarTarget  == p->targetID ) {
			pr = p;
			break;
		}
	}

	/*
	 *  Without a radar lock, display only the ASE circle.
	 *
	 *  We represent this case by passing the ASE diameter as a negative value.
	 */

	if ( c->curRadarTarget == -1 || pr == NULL) {
		*ASE_diameter_millirad = -130.0;
		*ASE_dot_az_millirad = 0.0;
		*ASE_dot_az_millirad = 0.0;
		return;
	}

	target = &ptbl[c->curRadarTarget];

	/*
	 *  Range greater than Rmax? Place aircraft and target on
	 *  a lead collision course.
	 */

	v.x = target->Cg.x - c->Cg.x;
	v.y = target->Cg.y - c->Cg.y;
	v.z = target->Cg.z - c->Cg.z;

	/*
	 * t becomes relative position of target wrt to aircraft body axes (feet)
	 */

	t = pr->rel;

	VReverseTransform_( &v, &c->trihedral, &vrel );

	/*
	 *  If range is less than Rmax, compute lead collision based on aircraft
	 *  plus missile velocity, not just our aircraft's velocity.
	 */

	if (range_feet < rmax_feet) {

		vrel.x -= 1000.0;  /* 1,000 fps is just a SWAG */
		
	}

	hs = t.x * t.x + t.y * t.y;

	/*
	 *  Omega values are rates of azimuth and elevation changes (rad/sec)
	 */

	omegay = (vrel.y * t.x - vrel.x * t.y) / hs;

	omegap = (vrel.z * hs - t.z * (vrel.x * t.x + vrel.y * t.y)) /
		(sqrt(hs) * (hs + t.z * t.z));

	/*
	 *  Just SWAGs here ...
	 */

	*ASE_diameter_millirad = 130.0;
	*ASE_dot_az_millirad = RADtoDEG(omegay) * 150.0;
	*ASE_dot_el_millirad = RADtoDEG(omegap) * 150.0;

	h = sqrt (
		*ASE_dot_az_millirad * *ASE_dot_az_millirad +
		*ASE_dot_el_millirad * *ASE_dot_el_millirad
		);

	/*
	 *  Limit ASE "dot" to position just outside the circle
	 */

	if (h > 70.0) {
		*ASE_dot_az_millirad = *ASE_dot_az_millirad * 70.0 / h;
		*ASE_dot_el_millirad = *ASE_dot_el_millirad * 70.0 / h;
	}

}

/*ARGSUSED */
int
update_aim120(craft * c)
{

	register int i;

	/*
	 *  Missile won't fire if we have "Weight on wheels"
	 */

	if ((c->flags & FL_GND_CONTACT) == 0) {
		if (hasFired[c->pIndex] && count[c->pIndex] > 0) {
			i = readyStation(c, "aim120");
			if (i < 0)
				fprintf(stderr, "Oops. Can't find an AIM-120\n");
			else {

				/*
				 *  In arcade mode, we never run out of ammunition
				 */

				if (arcadeMode == 0) {
					c->station[i].type = "";
					count[c->pIndex]--;
				}
				fireMissile(c, i);
				playSound(c, SoundMissileLaunch);
			}
			hasFired[c->pIndex] = 0;
		}
	}
	return 1;
}

/*ARGSUSED */
int
fire_aim120(craft * c)
{

	hasFired[c->pIndex] = 1;
	return 1;
}

double
missileTimeToImpact (craft * c, craftType * w)
{
	double v, t, root1, root2, r, a1, d, n;

	v = c->VT;
	a1 = (w->maxThrust - 0.5 * c->rho * w->CDOrigin * v * v)
		/ (w->emptyWeight + w->maxFuel) * earth_g;

	if (c->curRadarTarget >= 0 && a1 >= 0.0) {

		d = c->targetDistance;
		r = c->targetClosure;

		n = r * r + 2.0 * a1 * d;
		if (n > 0) {
			n = sqrt(n);
			root1 = (-r + n) / a1;
			root2 = (-r - n) / a1;
			if (root1 >= 0.0)
				if (root2 >= 0.0)
					if (root1 < root2)
						t = root1;
					else
						t = root2;
				else
					t = root1;
			else if (root2 >= 0.0)
				t = root2;
			else
				t = -1.0;
		}
		else
			t = -1.0;
	}

	else {
		t = -1.0;
	}

	return t;

}

/*
 *  aim120 display function
 *
 *  Update the HUD display strings associated with this weapon system.
 *
 *  This code may be called by drones.  Return a nonzero value if
 *  have a reasonable chance of scoring a kill.
 */

/*ARGSUSED */
int
display_aim120(craft * c, craftType * w, viewer * u, int *i1, int *i2)
{

	char      s[16];
	double    d, a1, v, r, root1, root2, n, t;
	VPoint    tmp;
	int       target, result = 0;

	sprintf(s, "%d AIM-120", count[c->pIndex]);
	strcpy(c->leftHUD[3], s);

	v = mag(c->Cg);
	a1 = (w->maxThrust - 0.5 * c->rho * w->CDOrigin * v * v)
		/ (w->emptyWeight + w->maxFuel) * earth_g;

	if (c->curRadarTarget >= 0 && a1 >= 0.0) {

		d = c->targetDistance;
		r = c->targetClosure;

		n = r * r + 2.0 * a1 * d;
		if (n > 0) {
			n = sqrt(n);
			root1 = (-r + n) / a1;
			root2 = (-r - n) / a1;
			if (root1 >= 0.0)
				if (root2 >= 0.0)
					if (root1 < root2)
						t = root1;
					else
						t = root2;
				else
					t = root1;
			else if (root2 >= 0.0)
				t = root2;
			else
				t = -1.0;
		}
		else
			t = -1.0;
	}

	else
		t = -1.0;

/*
 *  See if the missiles can lock onto any target.  We'll constrain getIRTarget()
 *  so that it will only select target's in a twenty degree boresight cone.
 */

	if (count[c->pIndex] > 0) {
		target = getIRTarget(c, &tmp, 0.17633);
	}
	else {
		target = -1;
	}

	if (target >= 0 && t <= 15.0)
		sprintf(s, "LOCKED   %d", (int) (t + 0.5));
	else if (t < 0.0)
		sprintf(s, "ARM      --");
	else if (t <= 15.0)
		sprintf(s, "IN RANGE %d", (int) (t + 0.5));
	else
		sprintf(s, "ARM      %d", (int) (t + 0.5));

/*
 *  Set result equal to one if we are recommending a missile shot
 */

	if (target >= 0 && t <= 10.0 && t > (w->armDelay + 0.5)) {
		result = 1;
	}

	strcpy(c->leftHUD[2], s);

	strcpy(c->leftHUD[4], "");

	return result;
}

extern craftType *newCraft(void);

void
initaim120(void)
{

	craftType *c;
	FILE     *f;
	dis_entity_type em1 =
	{2, 1, 225, 1, 2, 1, 0};
	dis_entity_type em2 =
	{0, 0, 0, 0, 0, 0, 0};

	c = newCraft();
	c->name = strdup("aim120");

	c->entityType = em1;
	c->altEntityType = em2;

	wtbl[3] = aim120Desc;
	wtbl[3].w = c;

	c->CDOrigin = 0.2;			/* 5" radius of body */
	c->CDFactor = -2.56694;

	c->CDBOrigin = 0.0;
	c->CDBFactor = 0.0;

	VIdentMatrix(&(c->I));
	c->I.m[0][0] = 0.0;
	c->I.m[1][1] = 0.0;
	c->I.m[2][2] = 0.0;
	c->cmSlope = -1.88;
	c->cmFactor = -1.00;

	c->wingS = 1.0;

/*
 *  Assume 150.0 lbs of weight is fuel and that it burns for about 4 seconds.
 *  That yields a fuel burn rate of 40 lbs/second.
 */

	c->emptyWeight = 100.0;
	c->maxFuel = 234.0;
	c->maxThrust = 2500.0;
	c->spFuelConsump = 16.0;  /* Isp = 220,  SFC = 3600.0 / Isp */

/*
 *  half-second arming delay
 */

	c->armDelay = 0.5;

	c->groundingPoint.x = 0.0;
	c->groundingPoint.y = 0.0;
	c->groundingPoint.z = 0.0;

	c->viewPoint.x = 0.0;
	c->viewPoint.y = 0.0;
	c->viewPoint.z = 0.0;

	c->muStatic = 0.0;
	c->muKinetic = 0.0;
	c->muBStatic = 0.0;
	c->muBKinetic = 0.0;

	c->maxNWDef = 0.0;
	c->NWIncr = 0.0;
	c->maxNWS = 0.0;
	c->gearD1 = 0.0;
	c->gearD2 = 0.0;

	f = acm_fopen("aim9.obv", "r");
	c->object = VReadObject(f);
	fclose(f);

}
