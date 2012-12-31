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
#include <memory.h>

#include <pm.h>
#ifdef AFDS
#include <afds.h>
#endif
#ifdef HAVE_DIS
#include "dis.h"
#endif

static craftType ctype[MAXCRAFTTYPES];

extern FILE *acm_fopen(char *, char *);
craftType *newCraft(void);
craftType *readPlane();
double genericThrust PARAMS((craft *));
void genericResupply PARAMS((craft *));

extern int radioReceptionCheck(craft * c, radio_t * r);
extern void initRadio(craft * c, radio_t * r);
extern void initDamage(craft *);
extern double efrandom(void);

void
freeCraftTypes (void)
{
	int i, j;

	for (i = 0; i < MAXCRAFTTYPES; ++i) {

		if (ctype[i].name != (char *) NULL) {

			free(ctype[i].name);

			if (ctype[i].object) {
				VDestroyObject (ctype[i].object);
			}

			if (ctype[i].objname) {
				free (ctype[i].objname);
			}

			if (ctype[i].description) {
				free(ctype[i].description);
			}

			if (ctype[i].modelname) {
				free(ctype[i].modelname);
			}

			if (ctype[i].CLift) {
				free (ctype[i].CLift->entry);
				free (ctype[i].CLift);
			}
			if (ctype[i].CDb) {
				free (ctype[i].CDb->entry);
				free (ctype[i].CDb);
			}
			if (ctype[i].CnBeta) {
				free (ctype[i].CnBeta->entry);
				free (ctype[i].CnBeta);
			}
			if (ctype[i].ClBeta) {
				free (ctype[i].ClBeta->entry);
				free (ctype[i].ClBeta);
			}
			if (ctype[i].Thrust) {
				free (ctype[i].Thrust->entry);
				free (ctype[i].Thrust);
			}
			if (ctype[i].ABThrust) {
				free (ctype[i].ABThrust->entry);
				free (ctype[i].ABThrust);
			}
			for (j=0; j<STATIONS; ++j) {
				if (ctype[i].station[j].type) {
					free (ctype[i].station[j].type);
				}
			}
			ctype[i].name = NULL;
		}
	}
}

int
getCraftCount(void)
{
	int       i;

	for (i = 0; i < MAXCRAFTTYPES; ++i) {
		if (ctype[i].name == (char *) NULL) {
			break;
		}
	}

	return i;
}

craftType *
lookupCraftByIndex(int craft_index)
{
	return &ctype[craft_index];
}

craftType *
lookupCraft(const char *name)
{
	int       i;
	craftType *c = NULL;

	for (i = 0; i < MAXCRAFTTYPES; ++i) {
		if (ctype[i].name != (char *) NULL) {
			if (strcmp(name, ctype[i].name) == 0)
				return &ctype[i];
		}
	}

	return c;
}

#ifdef HAVE_DIS

/*
 *  Find (or possible generate) the craftType entry associated with a
 *  given DIS entity type.
 */

craftType *
lookupCraftByEntityType( const dis_entity_type * id )
{
	int       i, depthcue = 1;
	craftType *c = NULL;
	entity_object_map *ep;

	for (i = 0; i < MAXCRAFTTYPES; ++i) {
		if (ctype[i].name != (char *) NULL) {

/*
 * This search could be done better; we could stop when the entityType
 * field being tested is zero.
 */

			if (ctype[i].entityType.kind == id->kind &&
				ctype[i].entityType.domain == id->domain &&
				ctype[i].entityType.country == id->country &&
				ctype[i].entityType.category == id->category &&
				ctype[i].entityType.subcategory == id->subcategory &&
				ctype[i].entityType.specific == id->specific &&
				ctype[i].entityType.extra == id->extra) {
				return &ctype[i];
			}
		}
	}

	/*
	 * Well, there wasn't a craft type defined that matched the desired
	 * entity type.  So, we'll look for an entry in the patterns contained
	 * in the eo_map.  If we find a match, create a new craftType entry
	 * and return it.
	 */

	ep = eo_map;

	for (i = 0; i < eo_map_count; ++ i) {

		FILE *f1;

		if (entityWildcardMatch (id, &ep->entity_type, &ep->entity_mask)) {

			c = newCraft();
			if (c) {
				char *p = strrchr (ep->object_name, '.');

				f1 = acm_fopen (ep->object_name, "r");
				c->name = strdup (ep->object_name);
				c->objname = strdup (ep->object_name);
				c->entityType = *id;
				c->altEntityType = *id;
				c->object = NULL;
				if (p != NULL && (strcmp (p, ".dxf") == 0 || 
								  strcmp (p, ".DXF") == 0)) {
					c->object = VReadDepthCueuedDXFObject (f1, depthcue);
				}
				else {
					c->object = VReadDepthCueuedObject (f1, depthcue);
				}

				if (c->object) {
					ep->obj = c->object;
				}
				else {
					printf ("error reading object file: %s\n", ep->object_name);
				}
			 
				fclose (f1);
				return c;
			}
			else {
				printf ("craft table overflow in lookupCraftByEntityType()\n");
			}
		}
		++ ep;
	}

	return c;
}

#endif

#ifndef WIN32

void
printValidAircraft(int s)
{
	char      buf[256];
	int       i;

	sprintf(buf, "Valid aircraft types are:\n\n");
	write(s, buf, strlen(buf));

	for (i = 0; i < MAXCRAFTTYPES; ++i) {
		if (ctype[i].name != (char *) NULL) {
			if (ctype[i].CLift) {
				sprintf(buf, "        %s\n", ctype[i].name);
				write(s, buf, strlen(buf));
			}
		}
	}
}
#endif

craftType *
newCraft(void)
{
	int       i;

	for (i = 0; i < MAXCRAFTTYPES; ++i)
		if (ctype[i].name == (char *) NULL) {
			memset (&ctype[i], 0, sizeof (craft));
			ctype[i].name = "*allocated*";
			return &ctype[i];
		}

	return (craftType *) NULL;
}

double
genericThrust( craft * c )
{
	register double t, ts;

	if (c->flags & FL_AFTERBURNER) {
		t = interpolate(c->cinfo->ABThrust, c->mach) * c->cinfo->maxABThrust;
	}
	else {
		t = interpolate(c->cinfo->Thrust, c->mach) * c->cinfo->maxThrust;
	}

	ts = c->throttle / 32768.0;
	c->rpm = (c->rpm - ts) * exp(deltaT / c->cinfo->engineLag) + ts;

	return t * c->rpm * c->rpm * c->rho / 0.002377;
}

double
fuelUsed( craft * c )
{
	double    spFuelConsump;

	if (c->flags & FL_AFTERBURNER) {
		spFuelConsump = c->cinfo->spABFuelConsump;
	}
	else {
		spFuelConsump = c->cinfo->spFuelConsump;
	}
	return spFuelConsump * c->curThrust * deltaT / 3600.0;
}

void
genericResupply(craft * c)
{

	int       i;

	c->fuel = c->cinfo->maxFuel;

	for (i = 0; i < c->cinfo->sCount; i++) {
		c->station[i] = c->cinfo->station[i];
	}
	initDamage(c);
}

extern void initPanel(craft * c);

int
newPlaneReserve(void)
{
	int       i;

	for (i = 0; i < MAXPLAYERS; ++i) {
		if (ptbl[i].type == CT_FREE) {
			ptbl[i].type = CT_RESERVED;
			return i;
		}
	}

	return -1;
}

int
newPlane(const char *planeType, int reservation_index)
{

	int       i, j;
	craft    *c;

	/*
	 *  Begin our search for an entry at the specified reservation location
	 */

	if (reservation_index == -1) {
		i = 0;
	}
	else {
		i = reservation_index;
	}

	for (; i < MAXPLAYERS; ++i) {
		if (ptbl[i].type == CT_FREE || ptbl[i].type == CT_RESERVED) {

			c = &ptbl[i];
			if ((c->cinfo = lookupCraft(planeType)) == NULL) {
				return -2;
			}

			c->type = CT_PLANE;

#ifdef AFDS
			AFDSInit( c );
#endif

			c->Cg.x = 0.0;
			c->Cg.y = 0.0;
			c->Cg.z = 0.0;
#ifdef FLAT_WORLD
			c->Sg.x = 0.0;
			c->Sg.y = 0.0;
			c->Sg.z = -c->cinfo->groundingPoint.z;
			c->prevSg = c->Sg;
#else
			c->w.z = FEETtoMETERS(c->cinfo->groundingPoint.z);
			c->prevw = c->w;
#endif

			c->p = c->q = c->r = 0.0;
			c->Se = c->Sr = c->Sa = 0.0;
			c->SeTrim = c->SaTrim = 0.0;

			c->throttle = 8192;
			c->curThrust = (c->cinfo->thrust) (c);
			c->rpm = (double) c->throttle / 32768.0;
			c->curFlap = 0.0;
			c->flapSetting = 0.0;
			c->curGear[0] = c->curGear[1] =
				c->curGear[2] = pi / 2.0;
			c->curSpeedBrake = 0.0;
			c->speedBrakeSetting = 0.0;
			c->curHeading = c->curPitch = c->curRoll = 0.0;
			VIdentMatrix(&(c->trihedral));
			c->curNWDef = 0.0;
			c->flags = FL_NWS | FL_GHANDLE_DN | FL_GND_CONTACT;
			c->radarMode = RM_STANDBY;
			c->curRadarTarget = -1;
			c->groundCgx = 0.0;
			for (j = 0; j < 6; ++j) {
				c->leftHUD[j] = Vmalloc(32);
				c->rightHUD[j] = Vmalloc(32);
				strcpy(c->leftHUD[j], "");
				strcpy(c->rightHUD[j], "");
			}

/*
 *  rearm and fuel the aircraft.
 */

			(*c->cinfo->resupply) (c);

			initPanel(c);

/* a trick to select a valid weapon */

			c->curWeapon = WEAPONTYPES - 1;
			selectWeapon(c);

/*
 *  Set up the radios
 */
			initRadio(c, &c->navReceiver[0]);
			initRadio(c, &c->navReceiver[1]);
			c->hsiSelect = &c->navReceiver[0];

			break;

		}
	}

	if (i == MAXPLAYERS)
		return -1;

	return i;
}

int
newDrone(craft * p, const char *plane_type)
{

	int       i, j, droneTeam;
	craft    *c;
	const char     *type;
	VPoint    s, tmp;

#ifdef HAVE_DIS
	double    disLocation[3];
	double    disZeroVec[3];
	double    disOrientation[3];

#endif

	for (i = 0; i < MAXPLAYERS; ++i) {
		if (ptbl[i].type == CT_FREE) {

			if (p->team == 1) {
				type = "MiG-29";
				droneTeam = 2;
			}
			else {
				type = "F-16";
				droneTeam = 1;
			}

			if (plane_type) {
				type = plane_type;
			}

			c = &ptbl[i];
			*c = *p;
			c->pIndex = i;
#ifdef AFDS
			c->fp = NULL;
			c->flightDirector = NULL;
			AFDSInit( c );
#endif

			/*
			 * Enter attack mode with no oppenent assigned
			 */

			c->curDroneMode = DM_ATTACK;
			c->team = droneTeam;
			c->curOpponent = -1;
			c->holdCount = 0;

			c->vl = NULL;
			c->type = CT_DRONE;
			c->cinfo = lookupCraft(type);

/*
 *  Position the drone about 1500 meters ahead of the player's craft.
 */

			strncpy(c->name, "drone", sizeof(c->name));
			s.x = 500.0 + 100.0 * (efrandom() + efrandom());
			s.y = 100.0 * (efrandom() + efrandom());
			s.z = 0.0;
			VTransform_(&s, &(p->trihedral), &tmp);

/*  convert NED (meters) to Geocentric (meters)  */

			VReverseTransform_(&tmp, &(p->XYZtoNED), &c->Sg);
			c->Sg.x += p->Sg.x;
			c->Sg.y += p->Sg.y;
			c->Sg.z += p->Sg.z;
			c->prevSg = c->Sg;

			DISGeocentricToWorldCoordinates
				((dis_world_coordinates *) & c->Sg, &c->w);
			c->prevw = c->w;
			GenerateWorldToLocalMatrix(&c->w, &c->XYZtoNED);

			c->curThrust = (c->cinfo->thrust) (c);

			c->curNWDef = 0.0;
			c->flags = p->flags & FL_AFTERBURNER;
			c->radarMode = RM_ACM;
			c->curRadarTarget = -1;

			for (j = 0; j < 6; ++j) {
				c->leftHUD[j] = Vmalloc(32);
				c->rightHUD[j] = Vmalloc(32);
				strcpy(c->leftHUD[j], "");
				strcpy(c->rightHUD[j], "");
			}

/*
 *  rearm and fuel the aircraft.
 */

			(*c->cinfo->resupply) (c);

			c->hsiSelect = NULL;

			selectNamedWeapon(c, WK_AIM9M);

			type = "";
			strcpy(c->lastTotal, type);
			strcpy(c->lastConsump, type);
			strcpy(c->lastFlap, type);
			strcpy(c->lastRPM, type);

#ifdef HAVE_DIS
			if (disInUse) {
				disLocation[0] = c->Sg.x;
				disLocation[1] = c->Sg.y;
				disLocation[2] = c->Sg.z;
				disZeroVec[0] = 0.0;
				disZeroVec[1] = 0.0;
				disZeroVec[2] = 0.0;
				disOrientation[0] = c->curHeading;
				disOrientation[1] = c->curPitch;
				disOrientation[2] = c->curRoll;
				dis_entityEnter(droneTeam, c,
								&c->cinfo->entityType,
								&c->cinfo->altEntityType,
								disLocation, disZeroVec,
								disZeroVec, disOrientation,
								disZeroVec, &c->disId);
				dis_setRadarMode(c, 1, 1);
#ifdef DIS_DEBUG
				printf("Entering %d %d\n", c->pIndex, c->disId);
#endif
			}
#endif

			break;
		}
	}

	if (i == MAXPLAYERS)
		return -1;

	return i;
}
