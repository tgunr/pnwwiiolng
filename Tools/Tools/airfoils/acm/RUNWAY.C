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

/*
 *  Runway markings are designed to conform to FAA AC 150/5340-1G
 *  dated 9/27/93.
 */

#include "pm.h"

void
          AddSymmetricPolygons(VMatrix * RWYtoXYZ, double x, double y, int count,
							   double length, double width, double margin,
							   VPolygon ** p, int p_count);

#define CENTERLINE_WIDTH		FEETtoMETERS(3.0)
#define CENTERLINE_LENGTH		FEETtoMETERS(120.0)
#define CENTERLINE_X_OFFSET		FEETtoMETERS(400.0)
#define CENTERLINE_GAP			FEETtoMETERS(80.0)
#define TOUCHDOWN_ZONE_WIDTH		FEETtoMETERS(6.0)
#define TOUCHDOWN_ZONE_LENGTH		FEETtoMETERS(75.0)
#define TOUCHDOWN_ZONE_OFFSET		FEETtoMETERS(500.0)
#define TOUCHDOWN_ZONE_MARGIN		FEETtoMETERS(5.0)
#define THRESHOLD_STRIPE_WIDTH		FEETtoMETERS(5.75)
#define THRESHOLD_STRIPE_LENGTH		FEETtoMETERS(150.0)
#define THRESHOLD_STRIPE_X_OFFSET	FEETtoMETERS(20.0)
#define THRESHOLD_STRIPE_Y_OFFSET	FEETtoMETERS(3.0)
#define THRESHOLD_STRIPE_MARGIN		FEETtoMETERS(5.75)
#define FIXED_MARKER_LENGTH		FEETtoMETERS(150.0)
#define FIXED_MARKER_WIDTH		FEETtoMETERS(30.0)
#define FIXED_MARKER_Y_OFFSET		FEETtoMETERS(3.0)
#define FIXED_MARKER_X_OFFSET		FEETtoMETERS(1000.0)
#define WHITE_PAINT			"#ccc"

#define RUNWAY_CULL_DISTANCE		50000.0		/* fifty kilometers */
#define MARKING_CULL_DISTANCE		5000.0	/* five kilometers */

void
AddRunway(VMatrix * RWYtoXYZ, double length, double width, int flags,
		  VPolygon *** poly, int *poly_count)
{
	VPoint    tmp, vertex[8];
	double    start, stop, x, y;
	int       i, stripes;
	VPolygon **p;

	*poly_count = i = 0;
	p = (VPolygon **) Vmalloc(sizeof(VPolygon *));

/*
 *  First, add the runway surface polygon ...
 */

	tmp.x = length / 2.0;
	tmp.y = width / 2.0;
	tmp.z = 0.0;
	VTransform(&tmp, RWYtoXYZ, &vertex[0]);
	tmp.x = -length / 2.0;
	tmp.y = width / 2.0;
	tmp.z = 0.0;
	VTransform(&tmp, RWYtoXYZ, &vertex[1]);
	tmp.x = -length / 2.0;
	tmp.y = -width / 2.0;
	tmp.z = 0.0;
	VTransform(&tmp, RWYtoXYZ, &vertex[2]);
	tmp.x = length / 2.0;
	tmp.y = -width / 2.0;
	tmp.z = 0.0;
	VTransform(&tmp, RWYtoXYZ, &vertex[3]);

	p[i] = VCreatePolygon(4, vertex,
						  VAllocDepthCueuedColor("#b7b19f", 1));
	p[i]->flags |= PolyUseCullDistance;
	p[i]->cullDistance = RUNWAY_CULL_DISTANCE;
	i++;

/*
 *  Now the runway centerline markings.
 */

	start = (0.5 * -length) + CENTERLINE_X_OFFSET - 0.5 * CENTERLINE_LENGTH;
	stop = (0.5 * length) - CENTERLINE_X_OFFSET;

	for (x = start; x < stop; x += CENTERLINE_LENGTH + CENTERLINE_GAP) {
		tmp.x = x - CENTERLINE_LENGTH / 2.0;
		tmp.y = CENTERLINE_WIDTH / 2.0;
		tmp.z = 0.0;
		VTransform(&tmp, RWYtoXYZ, &vertex[0]);
		tmp.x = x + CENTERLINE_LENGTH / 2.0;
		tmp.y = CENTERLINE_WIDTH / 2.0;
		tmp.z = 0.0;
		VTransform(&tmp, RWYtoXYZ, &vertex[1]);
		tmp.x = x + CENTERLINE_LENGTH / 2.0;
		tmp.y = -CENTERLINE_WIDTH / 2.0;
		tmp.z = 0.0;
		VTransform(&tmp, RWYtoXYZ, &vertex[2]);
		tmp.x = x - CENTERLINE_LENGTH / 2.0;
		tmp.y = -CENTERLINE_WIDTH / 2.0;
		tmp.z = 0.0;
		VTransform(&tmp, RWYtoXYZ, &vertex[3]);
		p = realloc(p, sizeof(VPolygon *) * (i + 1));
		p[i] = VCreatePolygon(4, vertex,
							  VAllocDepthCueuedColor(WHITE_PAINT, 1));
		p[i]->flags |= PolyUseCullDistance;
		p[i]->cullDistance = MARKING_CULL_DISTANCE;
		i++;
	}

	/*
	 * Runway threshold stripes
	 */

	/* From the Aeronautical Information Manual */

	if (width >= 18.0) {

		stripes = 0;

		if (width >= 60.0) {
			stripes = 16;
		}
		else if (width >= 45.0) {
			stripes = 12;
		}
		else if (width >= 30.0) {
			stripes = 8;
		}
		else if (width >= 23.0) {
			stripes = 6;
		}
		else if (width >= 18.0) {
			stripes = 4;
		}

		stripes >>= 1;

		x = 0.5 * length - THRESHOLD_STRIPE_X_OFFSET;
		y = width / 2.0 - THRESHOLD_STRIPE_Y_OFFSET;

		for (; stripes > 0; --stripes,
			 y -= THRESHOLD_STRIPE_WIDTH + THRESHOLD_STRIPE_MARGIN) {

			p = realloc(p, sizeof(VPolygon *) * (i + 4));

			tmp.x = x;
			tmp.y = y;
			tmp.z = 0.0;
			VTransform(&tmp, RWYtoXYZ, &vertex[0]);
			tmp.x = x - THRESHOLD_STRIPE_LENGTH;
			tmp.y = y;
			tmp.z = 0.0;
			VTransform(&tmp, RWYtoXYZ, &vertex[1]);
			tmp.x = x - THRESHOLD_STRIPE_LENGTH;
			tmp.y = y - THRESHOLD_STRIPE_WIDTH;
			tmp.z = 0.0;
			VTransform(&tmp, RWYtoXYZ, &vertex[2]);
			tmp.x = x;
			tmp.y = y - THRESHOLD_STRIPE_WIDTH;
			tmp.z = 0.0;
			VTransform(&tmp, RWYtoXYZ, &vertex[3]);
			p[i] = VCreatePolygon(4, vertex,
								  VAllocDepthCueuedColor(WHITE_PAINT, 1));
			p[i]->flags |= PolyUseCullDistance;
			p[i]->cullDistance = MARKING_CULL_DISTANCE;
			i++;

			/*
			 *  Opposite stripe on same runway end
			 */

			tmp.x = x;
			tmp.y = -y;
			tmp.z = 0.0;
			VTransform(&tmp, RWYtoXYZ, &vertex[0]);
			tmp.x = x;
			tmp.y = -y + THRESHOLD_STRIPE_WIDTH;
			tmp.z = 0.0;
			VTransform(&tmp, RWYtoXYZ, &vertex[1]);
			tmp.x = x - THRESHOLD_STRIPE_LENGTH;
			tmp.y = -y + THRESHOLD_STRIPE_WIDTH;
			tmp.z = 0.0;
			VTransform(&tmp, RWYtoXYZ, &vertex[2]);
			tmp.x = x - THRESHOLD_STRIPE_LENGTH;
			tmp.y = -y;
			tmp.z = 0.0;
			VTransform(&tmp, RWYtoXYZ, &vertex[3]);
			p[i] = VCreatePolygon(4, vertex,
								  VAllocDepthCueuedColor(WHITE_PAINT, 1));
			p[i]->flags |= PolyUseCullDistance;
			p[i]->cullDistance = MARKING_CULL_DISTANCE;
			i++;

			/*
			 *  Stripe on opposite runway end
			 */

			tmp.x = -x;
			tmp.y = y;
			tmp.z = 0.0;
			VTransform(&tmp, RWYtoXYZ, &vertex[0]);
			tmp.x = -x;
			tmp.y = y - THRESHOLD_STRIPE_WIDTH;
			tmp.z = 0.0;
			VTransform(&tmp, RWYtoXYZ, &vertex[1]);
			tmp.x = -x + THRESHOLD_STRIPE_LENGTH;
			tmp.y = y - THRESHOLD_STRIPE_WIDTH;
			tmp.z = 0.0;
			VTransform(&tmp, RWYtoXYZ, &vertex[2]);
			tmp.x = -x + THRESHOLD_STRIPE_LENGTH;
			tmp.y = y;
			tmp.z = 0.0;
			VTransform(&tmp, RWYtoXYZ, &vertex[3]);
			p[i] = VCreatePolygon(4, vertex,
								  VAllocDepthCueuedColor(WHITE_PAINT, 1));
			p[i]->flags |= PolyUseCullDistance;
			p[i]->cullDistance = MARKING_CULL_DISTANCE;
			i++;

			/*
			 *  Opposite stripe on opposite runway end
			 */

			tmp.x = -x;
			tmp.y = -y;
			tmp.z = 0.0;
			VTransform(&tmp, RWYtoXYZ, &vertex[0]);
			tmp.x = -x + THRESHOLD_STRIPE_LENGTH;
			tmp.y = -y;
			tmp.z = 0.0;
			VTransform(&tmp, RWYtoXYZ, &vertex[1]);
			tmp.x = -x + THRESHOLD_STRIPE_LENGTH;
			tmp.y = -y + THRESHOLD_STRIPE_WIDTH;
			tmp.z = 0.0;
			VTransform(&tmp, RWYtoXYZ, &vertex[2]);
			tmp.x = -x;
			tmp.y = -y + THRESHOLD_STRIPE_WIDTH;
			tmp.z = 0.0;
			VTransform(&tmp, RWYtoXYZ, &vertex[3]);
			p[i] = VCreatePolygon(4, vertex,
								  VAllocDepthCueuedColor(WHITE_PAINT, 1));
			p[i]->flags |= PolyUseCullDistance;
			p[i]->cullDistance = MARKING_CULL_DISTANCE;
			i++;
		}

	}

	/*
	 *  Fixed distance marker
	 */

	if (length > FEETtoMETERS(3000.0)) {

		p = realloc(p, sizeof(VPolygon *) * (i + 4));

		AddSymmetricPolygons(RWYtoXYZ,
							 0.5 * length - FIXED_MARKER_X_OFFSET,
							 0.5 * width - FIXED_MARKER_Y_OFFSET,
							 1,
							 FIXED_MARKER_LENGTH, FIXED_MARKER_WIDTH, 0.0,
							 p, i);

		i += 4;
	}

	*poly_count = i;
	*poly = p;
}

void
AddSymmetricPolygons(VMatrix * RWYtoXYZ, double x, double y, int count,
					 double length, double width, double margin,
					 VPolygon ** p, int p_count)
{
	VPoint    tmp, vertex[4];

	for (; count > 0; --count, y -= width + margin) {
		tmp.x = x;
		tmp.y = y;
		tmp.z = 0.0;
		VTransform(&tmp, RWYtoXYZ, &vertex[0]);
		tmp.x = x - length;
		tmp.y = y;
		tmp.z = 0.0;
		VTransform(&tmp, RWYtoXYZ, &vertex[1]);
		tmp.x = x - length;
		tmp.y = y - width;
		tmp.z = 0.0;
		VTransform(&tmp, RWYtoXYZ, &vertex[2]);
		tmp.x = x;
		tmp.y = y - width;
		tmp.z = 0.0;
		VTransform(&tmp, RWYtoXYZ, &vertex[3]);
		p[p_count] = VCreatePolygon(4, vertex,
								 VAllocDepthCueuedColor(WHITE_PAINT, 1));
		p[p_count]->flags |= PolyUseCullDistance;
		p[p_count]->cullDistance = MARKING_CULL_DISTANCE;
		p_count++;

		/*
		 *  Opposite stripe on same runway end
		 */

		tmp.x = x;
		tmp.y = -y;
		tmp.z = 0.0;
		VTransform(&tmp, RWYtoXYZ, &vertex[0]);
		tmp.x = x;
		tmp.y = -y + width;
		tmp.z = 0.0;
		VTransform(&tmp, RWYtoXYZ, &vertex[1]);
		tmp.x = x - length;
		tmp.y = -y + width;
		tmp.z = 0.0;
		VTransform(&tmp, RWYtoXYZ, &vertex[2]);
		tmp.x = x - length;
		tmp.y = -y;
		tmp.z = 0.0;
		VTransform(&tmp, RWYtoXYZ, &vertex[3]);
		p[p_count] = VCreatePolygon(4, vertex,
								 VAllocDepthCueuedColor(WHITE_PAINT, 1));
		p[p_count]->flags |= PolyUseCullDistance;
		p[p_count]->cullDistance = MARKING_CULL_DISTANCE;
		p_count++;

		/*
		 *  Stripe on opposite runway end
		 */

		tmp.x = -x;
		tmp.y = y;
		tmp.z = 0.0;
		VTransform(&tmp, RWYtoXYZ, &vertex[0]);
		tmp.x = -x;
		tmp.y = y - width;
		tmp.z = 0.0;
		VTransform(&tmp, RWYtoXYZ, &vertex[1]);
		tmp.x = -x + length;
		tmp.y = y - width;
		tmp.z = 0.0;
		VTransform(&tmp, RWYtoXYZ, &vertex[2]);
		tmp.x = -x + length;
		tmp.y = y;
		tmp.z = 0.0;
		VTransform(&tmp, RWYtoXYZ, &vertex[3]);
		p[p_count] = VCreatePolygon(4, vertex,
								 VAllocDepthCueuedColor(WHITE_PAINT, 1));
		p[p_count]->flags |= PolyUseCullDistance;
		p[p_count]->cullDistance = MARKING_CULL_DISTANCE;
		p_count++;

		/*
		 *  Opposite stripe on opposite runway end
		 */

		tmp.x = -x;
		tmp.y = -y;
		tmp.z = 0.0;
		VTransform(&tmp, RWYtoXYZ, &vertex[0]);
		tmp.x = -x + length;
		tmp.y = -y;
		tmp.z = 0.0;
		VTransform(&tmp, RWYtoXYZ, &vertex[1]);
		tmp.x = -x + length;
		tmp.y = -y + width;
		tmp.z = 0.0;
		VTransform(&tmp, RWYtoXYZ, &vertex[2]);
		tmp.x = -x;
		tmp.y = -y + width;
		tmp.z = 0.0;
		VTransform(&tmp, RWYtoXYZ, &vertex[3]);
		p[p_count] = VCreatePolygon(4, vertex,
								 VAllocDepthCueuedColor(WHITE_PAINT, 1));
		p[p_count]->flags |= PolyUseCullDistance;
		p[p_count]->cullDistance = MARKING_CULL_DISTANCE;
		p_count++;
	}
}
