/*
 *  acm : an aerial combat simulator for X
 *  Copyright (C) 1991,1992,1997  Riley Rainey
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
#include <math.h>

extern double strtod(const char *, char **);

struct _navaid_t *vhf_navaid[VOR_CHANNEL_COUNT];

static char *bad_freq = "*****";

static navaid_t *test_navaid = NULL;

#ifdef MAG_VAR
double    mag_var = DEGtoRAD(MAG_VAR);

#else
double    mag_var = DEGtoRAD(8.0);	/* eight degrees east */

#endif

void      InitNavaid(navaid_t * n);
char     *PrintableVHFFreq(freq_t f, char *s);

extern alarm_id_t addAlarm(double delta, void (*proc) (char *, char *), char *arg1, char *arg2);
extern double heading(VPoint * x);

double
magHeading(craft * c)
{
	register double h = c->curHeading + mag_var;

	return (h < 0.0) ? h + 2.0 * pi : h;
}

/*
 *  Search for a NAVAID receivable at the frequency specified
 *  by the supplied radio receiver.
 *
 *  Return 1 for success (r->station is set to the NAVAID).
 *  Return 0 if no station was found.
 */

int
radioReceptionCheck(craft * c, radio_t * r)
{
	navaid_t *n;
	VPoint    p;

	for (n = test_navaid; n; n = n->next) {
		if (r->frequency == n->frequency) {
			if (n->flags & NAVAID_LOC) {
				VTransform(&c->Sg, &n->lt, &p);
#ifdef DEBUG
				printf("tested: %s %f %f %f\n", n->id, p.x, p.y, p.z);
#endif
				if (p.x > 0.0 && fabs(p.y / p.x) < 1.192) {
#ifdef DEBUG
					printf("locked: %s %f %f %f\n", n->id, p.x, p.y, p.z);
#endif
					r->station = n;
					return 1;
				}
			}
			else {
				r->station = n;
				return 1;
			}
		}
	}

	return 0;
}

void
radioFrequencyChanged(craft * c, radio_t * r)
{
	r->station = NULL;
	radioReceptionCheck(c, r);
	PrintableVHFFreq(r->frequency, r->freq_readout);
	strcpy(r->dme_readout, "DME ----");
}

void
initRadio(craft * c, radio_t * r)
{
	r->frequency = r->standby_frequency = 0;
	r->obs_setting = 0;
	radioFrequencyChanged(c, r);
}

void
freeAllNavaids (void)
{
	navaid_t *n = test_navaid, *p;
	while (n) {
		p = n;
		n = n->next;
		free (p);
	}
}

int
AddNavaid(char *ident, char *type, WorldCoordinates * w,
		  char *magvar, double freq)
{
	navaid_t *n;
	char      var[3];
	double    mvar;

	n = (navaid_t *) malloc(sizeof(navaid_t));
	strncpy(var, magvar, 2);
	var[2] = '\0';
	mvar = strtod(var, (char **) NULL);
	mvar = (magvar[2] == 'E') ? -mvar : mvar;
	n->bearing = n->magvar = DEGtoRAD(mvar);

	strncpy(n->id, ident, 4);
	n->loc = *w;
	GenerateWorldToLocalMatrix(w, &n->lt);
	VRotate(&n->lt, ZRotation, n->magvar);
	DISWorldCoordinatesToGeocentric(w, (dis_world_coordinates *) & n->Sg);

	if (strcmp(type, "VORTAC") == 0) {
		n->flags = NAVAID_VOR | NAVAID_DME;
	}
	else if (strcmp(type, "TACAN") == 0) {
		n->flags = NAVAID_VOR | NAVAID_DME;
	}
	else if (strcmp(type, "VOR/DME") == 0) {
		n->flags = NAVAID_VOR | NAVAID_DME;
	}
	else if (strcmp(type, "VOR") == 0) {
		n->flags = NAVAID_VOR;
	}
	else if (strcmp(type, "NDB") == 0) {
		n->flags = NAVAID_NDB;
	}
	else {
		free(n);
		return -1;
	}

	if (n->flags & NAVAID_VOR) {
		n->frequency = (int) ((freq - 108.00) * 20.0 + 0.5);
	}
	else {
		n->frequency = -(int) (freq + 0.5);
	}
	n->next = test_navaid;
	test_navaid = n;
	return 0;
}

int
AddILS(char *ident, char *type, WorldCoordinates * w,
	 WorldCoordinates * gsw, char *magvar, double freq, double loc_width,
	   double loc_bearing, double gs_angle)
{
	navaid_t *n;
	char      var[3];
	double    mvar;

	n = (navaid_t *) malloc(sizeof(navaid_t));
	strncpy(var, magvar, 2);
	var[2] = '\0';
	mvar = strtod(var, (char **) NULL);
	mvar = (magvar[2] == 'E') ? -mvar : mvar;
	n->magvar = DEGtoRAD(mvar);

	strncpy(n->id, ident, 4);
	n->bearing = DEGtoRAD(loc_bearing);

	n->loc = *w;
	GenerateWorldToLocalMatrix(w, &n->lt);
	VRotate(&n->lt, ZRotation, -n->bearing + n->magvar - M_PI);
	DISWorldCoordinatesToGeocentric(w, (dis_world_coordinates *) & n->Sg);

	n->gs_loc = *gsw;
	GenerateWorldToLocalMatrix(gsw, &n->gst);
	VRotate(&n->gst, ZRotation, -n->bearing + n->magvar - M_PI);

	n->slope = DEGtoRAD(gs_angle);
	n->beam_width = DEGtoRAD(loc_width);

	if (strcmp(type, "ILS") == 0) {
		n->flags = NAVAID_LOC | NAVAID_GS;
	}
	else if (strcmp(type, "ILS/DME") == 0) {
		n->flags = NAVAID_LOC | NAVAID_GS | NAVAID_DME;
	}
	else if (strcmp(type, "LOCALIZER") == 0) {
		n->flags = NAVAID_LOC;
	}
	else if (strcmp(type, "LOC/GS") == 0) {
		n->flags = NAVAID_LOC | NAVAID_GS;
	}
	else if (strcmp(type, "LOC/DME") == 0) {
		n->flags = NAVAID_LOC | NAVAID_DME;
	}
	else {
		free(n);
		return -1;
	}

	n->frequency = (int) ((freq - 108.00) * 20.0 + 0.5);
	n->next = test_navaid;
	test_navaid = n;
	return 0;
}

void
InitNavaid(navaid_t * n)
{
	VMatrix   m;
	VPoint    p;

	VIdentMatrix(&m);
	VSetPoint(p, 1.0, 0.0, 0.0);

	switch (n->flags & (NAVAID_VOR | NAVAID_LOC | NAVAID_GS)) {

	case NAVAID_VOR:
	case NAVAID_LOC:
		break;

	case (NAVAID_LOC | NAVAID_GS):
		VRotate(&m, ZRotation, n->bearing);
		VRotate(&m, YRotation, n->slope);
		n->gst = m;
		break;
	}
}

double
glideSlopeOffset(navaid_t * n, craft * c)
{
	VPoint    p;

	VTransform(&c->Sg, &n->gst, &p);
	return -(atan(-p.z / p.x) - n->slope);

}

double
radial(navaid_t * n, craft * c, VPoint * vec)
{
	VTransform(&c->Sg, &n->lt, vec);
	return heading(vec);
}

char     *
PrintableVHFFreq(freq_t f, char *s)
{
	if (f > VOR_CHANNEL_COUNT) {
		fprintf(stderr,
				"invalid frequency passed to PrintableVHFFreq\n");
		return bad_freq;
	}

	sprintf(s, "%5d", 10800 + 5 * f);
	s[6] = '\0';
	s[5] = s[4];
	s[4] = s[3];
	s[3] = '.';

	return s;
}

char     *
PrintableTACANChannel(freq_t f, char *s)
{
	if (f > VOR_CHANNEL_COUNT) {
		fprintf(stderr,
				"invalid frequency passed to PrintableTACANChannel\n");
		return bad_freq;
	}

	sprintf(s, "%d%c", f / 2 + 17, (f % 2) ? 'Y' : 'X');
	return s;
}

/* ARGSUSED */
void
DMECheckAlarm(char *arg1, char *arg2)
{

	int       i;
	craft    *c;
	VPoint    p;
	navaid_t *n;
	alarm_id_t id;

	for (i = 0, c = ptbl; i < MAXPLAYERS; ++i, ++c) {

/*  Ensure that we're still receiving the same station */

		if (c->hsiSelect) {
			radioReceptionCheck(c, c->hsiSelect);
		}

		if ((c->type == CT_PLANE) && c->hsiSelect->station) {

			n = c->hsiSelect->station;
			p.x = c->Sg.x - n->Sg.x;
			p.y = c->Sg.y - n->Sg.y;
			p.z = c->Sg.z - n->Sg.z;

			sprintf(c->hsiSelect->dme_readout, "DME %.1f",
					METERStoFEET(mag(p)) / NM);
		}
	}

	id = addAlarm(5.0, DMECheckAlarm, (char *) NULL, (char *) NULL);
}
