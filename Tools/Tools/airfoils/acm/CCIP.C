#include <pm.h>
#include "dis.h"

int       select_mk82(craft *);
int       display_mk82(craft *, craftType *, viewer *, int *, int *);
int       update_mk82(craft *);
int       press_mk82(craft *);
int       release_mk82(craft *);

extern int cdebug;

weaponDesc mk82Desc =
{
	WK_MK82,
	select_mk82,				/* select */
	update_mk82,				/* update */
	display_mk82,				/* display procedure */
	press_mk82,				    /* fire */
	release_mk82,				/* fire button release */
};


void
computeImpactPoint ( craft *c, craftType *bomb, WorldCoordinates *ip )
{
	double t_sec, ground_speed_fps, loft_distance_feet;
	double T1_sec, T2_sec;
	double A, B, C, S4AC;
	double sin_course, cos_course;

	/*
	 * Local terrain skin location
	 */

	*ip = c->w;
	ip->z = localAltitude( &c->Sg, &c->w );

	A = - 0.5 * earth_g;
	B = - c->Cg.z;
	C = METERStoFEET ( c->w.z - ip->z );

	S4AC = sqrt ( B * B - 4.0 * A * C );

	/*
	 *  Compute time to impact using the solution to the quadratic formula:
	 *
	 *     x = 0.5 * ( - earth_g ) * t^2  +  v0 * t  +  x0
	 *
	 *   v0 = vertical velocity (up positive, feet-per-second)
	 *   x0 = height above surface (feet)
	 */

	if (S4AC >= 0.0) {

		T1_sec = ( - B + S4AC ) / ( 2.0 * A );
		T2_sec = ( - B - S4AC ) / ( 2.0 * A );

	}
	else {
		/* complex root(s) */
	}

	t_sec = (T1_sec > T2_sec) ? T1_sec : T2_sec;

	ground_speed_fps = sqrt ( c->Cg.x * c->Cg.x + c->Cg.y * c->Cg.y );

	loft_distance_feet = ground_speed_fps * t_sec;

	/*
	 *  Based on instantaneous velocity, not aircraft orientation
	 */

	cos_course = c->Cg.x / ground_speed_fps;
	sin_course = c->Cg.y / ground_speed_fps;

	DISUpdateWorldCoordinates(ip, cos_course, sin_course, FEETtoMETERS ( loft_distance_feet ) );

}

static int count[MAXPLAYERS];
static int hasFired[MAXPLAYERS];

int
select_mk82( craft * c )
{
	hasFired[c->pIndex] = 0;
	count[c->pIndex] = countOrdinance(c, "mk82");
	return 1;
}

void
initmk82(void)
{

	craftType *c;
	FILE     *f;
	dis_entity_type em1 =
	{2, 9, 225, 2, 73, 0, 0};
	dis_entity_type em2 =
	{0, 0, 0, 0, 0, 0, 0};

	c = newCraft();
	c->name = strdup("Mk 82");

	wtbl[2] = mk82Desc;
	wtbl[2].w = c;

	c->entityType = em1;
	c->altEntityType = em2;

	/*
	 *  Unverified parameters calculated using DATCOM recommendations
	 */

	c->CDOrigin = 0.081;

	c->CDFactor = 0.0;

	c->CDBOrigin = 0.0;
	c->CDBFactor = 0.0;

	/*
	 * dCL/da = 4.09
	 * dCm,cg/da = -21.4
	 */

	f = acm_fopen("tracer.obv", "r");
	c->object = VReadObject(f);
	fclose(f);

}

/*ARGSUSED */
int
update_mk82(craft * c)
{
	int i;

	/*
	 *  Bombs won't drop if we have "Weight on wheels"
	 */

	if ((c->flags & FL_GND_CONTACT) == 0) {

		if (hasFired[c->pIndex] && count[c->pIndex] > 0) {

			i = readyStation(c, "mk82");
			if (i < 0) {
				fprintf(stderr, "Oops. Can't find Mk-82\n");
			}
			else {

				/*
				 *  In arcade mode, we never run out of ammunition
				 */

				if (arcadeMode == 0) {
					c->station[i].type = "";
					count[c->pIndex]--;
				}

				dropOrdinance (c, i);
				/* playSound(c, SoundBombDrop); */
			}
			hasFired[c->pIndex] = 0;
		}
	}
	return 1;
}

int
display_mk82(craft *c, craftType *ct, viewer *u, int *x, int *y)
{
	return 0;
}

int
press_mk82(craft *c)
{
	return 0;
}

int
release_mk82(craft *c)
{
	return 0;
}


int
dropOrdinance ( craft *c, int ind )
{
	craft *m;
	int i;
	VPoint    s, s1;
	VPoint    cY, mX, mY, mZ;
	double    v;

#ifdef HAVE_DIS
	double    disLocation[3];
	double    disVelocity[3];
	double    disZeroVec[3];
	double    disOrientation[3];

#endif

	/*
	 *  Find an empty projectile entry
	 */

	for ((i = 0, m = &mtbl[0]); i < MAXPROJECTILES; (++i, ++m)) {
		if (m->type == CT_FREE) {
			m->type = CT_BOMB;
			break;
		}
	}

	if (i == MAXPROJECTILES)
		return -1;

	m->cinfo = lookupCraft("aim-9m");
	m->fuel = 0.0;
	m->curThrust = 0.0;
	m->owner = c->pIndex;

	m->gvs_instance = (GVS_OBI) NULL;

/*
 *  Build trihedral based on the launching aircraft's current velocity vector
 *  rather than simply it's current direction vector.
 *
 *      (1)  build a unit velocity vector.
 *      (2)  calculate missiles local Z axis from
 *              plane's-y-axis CROSS missile's-unit-velocity-vector
 *      (3)  calculate missile's Y axis.
 */

	if ((v = mag(c->Cg)) < 1.0) {
		m->trihedral = c->trihedral;
		m->curRoll = c->curRoll;
		m->curPitch = c->curPitch;
		m->curHeading = c->curHeading;
	}
	else {
		mX = c->Cg;
		mX.x /= v;
		mX.y /= v;
		mX.z /= v;
		cY.x = c->trihedral.m[0][1];
		cY.y = c->trihedral.m[1][1];
		cY.z = c->trihedral.m[2][1];

		VCrossProd(&mX, &cY, &mZ);
		VCrossProd(&mZ, &mX, &mY);

		m->trihedral.m[0][0] = mX.x;
		m->trihedral.m[1][0] = mX.y;
		m->trihedral.m[2][0] = mX.z;
		m->trihedral.m[0][1] = mY.x;
		m->trihedral.m[1][1] = mY.y;
		m->trihedral.m[2][1] = mY.z;
		m->trihedral.m[0][2] = mZ.x;
		m->trihedral.m[1][2] = mZ.y;
		m->trihedral.m[2][2] = mZ.z;

		euler(m);
	}

	m->Cg = c->Cg;
	VTransform(&(c->cinfo->wStation[ind]), &(c->trihedral), &s1);
	VReverseTransform_(&s1, &c->XYZtoNED, &s);
	m->Sg.x = c->prevSg.x + FEETtoMETERS(s.x);
	m->Sg.y = c->prevSg.y + FEETtoMETERS(s.y);
	m->Sg.z = c->prevSg.z + FEETtoMETERS(s.z);
	DISGeocentricToWorldCoordinates
		((dis_world_coordinates *) & m->Sg, &m->w);
	m->prevw = m->w;
	GenerateWorldToLocalMatrix(&m->w, &m->XYZtoNED);
	m->armTimer = m->cinfo->armDelay;
	m->flags = 0;
	m->createTime = curTime;

	m->curRadarTarget = -1;

#ifdef HAVE_DIS

/*
 *  ACM bombs are DIS "tracked munitions", so we are
 *  responsible for sending entity state PDU's for them
 */

	if (disInUse) {
		VPoint    tmp;

		disLocation[0] = m->Sg.x;
		disLocation[1] = m->Sg.y;
		disLocation[2] = m->Sg.z;
		tmp.x = FEETtoMETERS(m->Cg.x);
		tmp.y = FEETtoMETERS(m->Cg.y);
		tmp.z = FEETtoMETERS(m->Cg.z);
		VReverseTransform_(&tmp, &m->XYZtoNED, (VPoint *) & disVelocity[0]);
		disZeroVec[0] = 0.0;
		disZeroVec[1] = 0.0;
		disZeroVec[2] = 0.0;
		disOrientation[0] = m->curHeading;
		disOrientation[1] = m->curPitch;
		disOrientation[2] = m->curRoll;
		dis_entityEnter(c->team, m,
						&m->cinfo->entityType,
						&m->cinfo->altEntityType,
						disLocation, disVelocity,
						disZeroVec, disOrientation,
						disZeroVec, &m->disId);
#ifdef DIS_DEBUG
		printf("Bomb Entering m%d %d\n", i, m->disId);
#endif
	}
#endif
	return 0;
}
