/*
 *  acm : an aerial combat simulator for X
 *  Copyright (C) 1991-1997  Riley Rainey
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

#include <pm.h>
#include <math.h>

typedef struct _alt_entry {
	VPoint    pos;
	double    z;
} AltitudeEntry;

#define AENTRY_MAX	256
AltitudeEntry aentry[AENTRY_MAX];
int       aentry_count = 0;

/*
 *  Returns the altitude of the ground [units are meters, MSL]
 *
 *  This algorithm does a brute force compuation of the angular offset
 *  from the specified location to each of a set of reference locations.
 *  The altitude returned is the one corresponding to the nearest reference
 *  location -- no interpolation is performed.
 */

double
localAltitude(VPoint * vec, WorldCoordinates * wc)
{
	AltitudeEntry *p;
	double    max = 0.0, q;
	int       i, j = -1;

	for (p = aentry, i = 0; i < aentry_count; ++p, ++i) {
		q = VDotProd(vec, &p->pos);
		if (i == 0 || q > max) {
			max = q;
			j = i;
		}
	}
	return (j == -1) ? 0.0 : aentry[j].z;
}

/*
 *  A call to this routine clears the current ground altitude reference point
 *  cache.
 */

void
clearAltitudeCache(void)
{
	aentry_count = 0;
}

/*
 *  This routine is called to add a new entry to the ground altitude reference
 *  cache.
 */

void
addAltitudeEntry(WorldCoordinates * w)
{
	dis_world_coordinates g;
	double    m;

	if (aentry_count == AENTRY_MAX) {
		return;
	}

	DISWorldCoordinatesToGeocentric(w, &g);

/*
 *  Convert the position vector to a unit vector (this simplifies the search
 *  algorithm by not forcing us to divide the dot product during the search).
 */

	m = sqrt(g.x * g.x + g.y * g.y + g.z * g.z);
	aentry[aentry_count].pos.x = g.x / m;
	aentry[aentry_count].pos.y = g.y / m;
	aentry[aentry_count].pos.z = g.z / m;
	aentry[aentry_count].z = w->z;
	++aentry_count;
}
