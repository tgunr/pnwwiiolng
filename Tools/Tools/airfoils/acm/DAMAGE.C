
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
#include "damage.h"

#if defined (HAVE_RAND)
int
acm_rand(void)
{
	return rand() & 0xffff;
}

void
acm_srand(int seed)
{
	srand(seed);
}

#else
#if defined (HAVE_RANDOM)
int
acm_rand()
{
	return random() & 0xffff;
}

void
acm_srand(int seed)
{
	srandom(seed);
}

#else

int
acm_rand()
{
	return 1;
}

void
acm_srand(int seed)
{
	fprintf(stderr, "Hmm. Your system seems to lack both rand() and random()\n");
	fprintf(stderr, "What kind of system is this, anyway?\n\n");
	exit(1);
}

#endif
#endif

double
frandom(void)
{

	return (double) (acm_rand() & 0x7fff) / 32767.0;
}

/*
 *  Select a subsystem to receive damage.
 */

long
selectSystem(void)
{

	double    r;
	long      i;

	if ((r = frandom()) < 0.25)
		i = SYS_ENGINE1;
	else if (r < 0.35)
		i = SYS_RADAR;
	else if (r < 0.40)
		i = SYS_TEWS;
	else if (r < 0.45)
		i = SYS_HYD1;
	else if (r < 0.50)
		i = SYS_HYD2;
	else if (r < 0.53)
		i = SYS_GEN1;
	else if (r < 0.56)
		i = SYS_GEN2;
	else if (r < 0.61)
		i = SYS_FLAPS;
	else if (r < 0.69)
		i = SYS_SPEEDBRAKE;
	else if (r < 0.78)
		i = SYS_FUEL;
	else if (r < 0.85)
		i = SYS_HUD;
	else if (r < 0.90)
		i = SYS_LEFTMAIN;
	else if (r < 0.95)
		i = SYS_RIGHTMAIN;
	else
		/* per PREfix */
		i = SYS_NOSEGEAR;
	return i;
}

void
damageSystem(craft * c, long sys)
{

	if ((c->damageBits & sys) || (sys == SYS_FUEL)) {

		c->damageBits &= ~sys;

		switch (sys) {

		case SYS_ENGINE1:
			c->throttle = 0;
			break;

		case SYS_RADAR:
			c->curRadarTarget = -1;
			break;

			/*
			 *  Fuel leaks can be up to 40 pounds per second here.
			 */

		case SYS_FUEL:
			c->leakRate += (frandom() + frandom()) * 20.0;
			break;

		case SYS_HYD1:
		case SYS_HYD2:
			if ((c->damageBits & (SYS_HYD1 | SYS_HYD2)) == 0) {
				c->damageBits &= ~SYS_SPEEDBRAKE;
				c->damageBits &= ~SYS_FLAPS;
			}
			break;

		case SYS_GEN1:
		case SYS_GEN2:
			if ((c->damageBits & (SYS_GEN1 | SYS_GEN2)) == 0) {
				c->damageBits &= ~
					(SYS_HUD | SYS_RADAR | SYS_TEWS);
				break;
			}
		}

	}
}

/*
 * absorbDamage :  craft c is hit with d points of damage.
 */

int
absorbDamage(craft * c, int d)
{

	double    n, x;
	register long sys;

	/*
	 *  Actual damage sustained is adjusted by a damage factor that forms a
	 *  bell curve centered around 0.75 * d.
	 */

	x = (frandom() + frandom()) / 2.0 + 0.25;

	d = (int) (d * x + 0.5);

	if (d > 0) {
		playSound(c, SoundExplosion);
	}

	for (; d > 0; --d) {

		/*
		 *  For each damage point absorbed, there is a 15 percent chance that
		 *  it will be absorbed by some subsystem other than the actual
		 *  airframe.
		 */

		if (frandom() <= 0.15) {
			sys = selectSystem();
			damageSystem(c, sys);
		}

		/*
		 *  For each point absorbed by the airframe, there is a 20% chance that
		 *  it'll be absorbed by the wing and induce a rolling moment or a 10 
		 *  percent chance that it will hit a horizontal stabilizer and induce
		 *  a pitching and rolling moment.
		 */

		else {

			if ((n = frandom()) <= 0.20) {
				c->damageCL += (frandom() - 0.5) * 0.20;
			}
			else if (n <= 0.30) {
				c->damageCL += (frandom() - 0.5) * 0.10;
				c->damageCM += (frandom() - 0.5) * 0.20;
			}

			if (--(c->structurePts) <= 0)
				return 0;
		}

	}

	return 1;

}

int
absorbDISDamage(craft * c,
				dis_entity_type *etype, 
				u_short warhead_type,
				u_short fuze_type,
				double distance_meters,
				double velocity_meters_per_sec,
				double *explosion_diameter_meters)
{
	int i;
	int damage_points;
	munition_map *pmm = mun_map;

	*explosion_diameter_meters = 0.0;

	for (i=0; i<mun_map_count; ++i) {
		if (entityWildcardMatch(etype, &pmm->entity_type, &pmm->entity_mask)) {
			if (pmm->warhead_mask == 0 || pmm->warhead_type == warhead_type) {

				/* found a match; assess damage */

				/*
				 * kinetic warhead
				 */

				if (pmm->kinetic_flag) {
					damage_points = (int) ( 0.5 * pmm->damage_factor *
						velocity_meters_per_sec * velocity_meters_per_sec +
										   0.5 );
				}

				/*
				 * blast warhead
				 */

				else {
					if (distance_meters < 1.0) {
						distance_meters = 1.0;
					}
					damage_points = (int) ( pmm->damage_factor / 
						distance_meters * distance_meters + 0.5 );
				}

				*explosion_diameter_meters = pmm->explosion_diameter_meters;

				return absorbDamage ( c, damage_points );
			}
		}
		++ pmm;
	}

	printf ("Warning: missed munition entity lookup failed\n");
	return 1;
}

void
initDamage(craft * c)
{
	c->damageBits = c->cinfo->damageBits;
	c->structurePts = c->cinfo->structurePts;
	c->leakRate = 0.0;
	c->damageCL = 0.0;
	c->damageCM = 0.0;
}

/*
 * compare an incoming DIS entity type against a wildcarded entity type;
 * returns non-zero value if they match.
 */

int
entityWildcardMatch (const dis_entity_type *in,
					 const dis_entity_type *pattern,
					 const dis_entity_type *pattern_mask)
{
	if (pattern_mask->kind == 0 || 
		pattern->kind == in->kind) {

		if (pattern_mask->domain == 0 || 
			pattern->domain == in->domain) {

			if (pattern_mask->country == 0 || 
				pattern->country == in->country) {

				if (pattern_mask->category == 0 || 
					pattern->category == in->category) {

					if (pattern_mask->subcategory == 0 || 
						pattern->subcategory == in->subcategory) {

						if (pattern_mask->specific == 0 || 
							pattern->specific == in->specific) {

							if (pattern_mask->extra == 0 || 
								pattern->extra == in->extra) {
								return 1;
							}

						}

					}

				}

			}

		}

	}

	return 0;
}
