/*
 *  acm : an aerial combat simulator for X
 *  Copyright (C) 1991,1992  Riley Rainey
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

#include "interpolate.h"
#include <stdio.h>
#include <stdlib.h>
#ifdef DUMP
#include <signal.h>
#endif

/*
 *  interpolate :  determine the value of a function of one variable
 *                 by interpolation.  Interpolation tables are built by
 *                 the ibuild utility.
 */

static char *ierrmsg = "interpolate: x value is out of bounds\n";

float_t
interpolate(ITable * table, double x)
{

	register int i, count = table->count;
	float_t   result;

	if (x < table->minX) {
		fprintf(stderr, ierrmsg);
		fprintf(stderr, "minimum x = %g; x = %g\n", table->minX, x);
#ifdef DUMP
		kill(getpid(), SIGBUS);
#endif
		return (float_t) I_NaN;
	}

	for (i = 0; i < count; ++i) {
		if (x <= table->entry[i].x) {
			result = (float_t) (table->entry[i].m * x + table->entry[i].b);
			if (isnan(result)) {
				fprintf(stderr, "interpolate: internal error\n");
			}
			return result;
		}
	}

	fprintf(stderr, ierrmsg);
	fprintf(stderr, "maximum x = %g; x = %g\n", table->entry[i - 1].x, x);

#ifdef DUMP
	kill(getpid(), SIGBUS);
#endif

	return (float_t) I_NaN;
}

ITable *
copyITable (ITable *oldp)
{
	ITable * newp;
	int i;

	if (oldp) {
		newp = (ITable *) malloc(sizeof(ITable));
		if (newp) {
			*newp = *oldp;
			newp->entry = (IEntry *) malloc(sizeof(IEntry) * (oldp->count));
			if (newp->entry == NULL) {
				return NULL;
			}
			for (i=0; i<newp->count; ++i) {
				newp->entry[i] = oldp->entry[i];
			}
		}
	}

	return newp;
}

void
freeITable (ITable *oldp)
{
	if ( oldp ) {
		free ( oldp->entry );
		free ( oldp );
	}
}

