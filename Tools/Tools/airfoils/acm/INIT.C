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

#ifndef WIN32
#include <X11/Xos.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>

#include "pm.h"
#include "alarm.h"

extern craftType *newCraft (void);
extern int compileAircraftInventory (void);
extern void initaim9 (void), initm61a1 (void), initmk82(void), initaim120(void);
extern void buildExplosion (void);
extern void placeObject ();
extern void InitNavaidSystem ();
extern void DMECheckAlarm (char *arg1, char *arg2);
extern void resupplyCheck (char *arg1, char *arg2);
extern void acm_srand PARAMS ((int seed));
extern void addAltitudeEntry (WorldCoordinates * w);
extern int AddNavaid (char *ident, char *type, WorldCoordinates * w,
		      char *magvar, double freq);
extern int AddILS (char *ident, char *type, WorldCoordinates * w,
	WorldCoordinates * gsw, char *magvar, double freq, double loc_width,
		   double loc_bearing, double gs_angle);

/*
 *  Defined in doViews.c :
 */

extern VPolygon **poly;
extern long polyCount;

void undersampleObject (VObject * obj, int rate);

static char *errmsg = "Sorry, I cannot find the file %s in ../objects/, %s or %s\n";

extern void
  AddRunway (VMatrix * RWYtoXYZ, double length, double width, int flags,
	     VPolygon *** poly, int *poly_count);

/*
 *  acm_fopen :  Open an important acm data file (or die trying).
 */

char _acm_fopen_library_dir[2048];

FILE     *
acm_fopen(char *name, char *access)
{

	FILE     *f;
	char      libname[256];

	if ((f = fopen(name, access)) == (FILE *) NULL) {
		strcpy(libname, "../objects/");
		strcat(libname, name);
		if ((f = fopen(libname, access)) == (FILE *) NULL) {
			strcpy(libname, _acm_fopen_library_dir);
			strcat(libname, name);
			if ((f = fopen(libname, access)) == (FILE *) NULL) {
				strcpy (libname, SRCDIR);
				strcat (libname, "/../objects/");
				strcat (libname, name);
				if ((f = fopen(libname, access)) == (FILE *) NULL) {
					fprintf(stderr, errmsg, name, _acm_fopen_library_dir, libname);
					exit(1);
				}
			}
		}
	}

	return f;
}

/*
 *  acm_find_file :  Find an important acm data file (or die trying).
 */

char     *
acm_find_file(char *name)
{
	static char pname[1024];
	struct stat statbuf;

	strcpy(pname, name);
	if (stat(name, &statbuf) != 0) {
		strcpy(pname, "../objects/");
		strcat(pname, name);
		if (stat(pname, &statbuf) != 0) {
			strcpy(pname, _acm_fopen_library_dir);
			strcat(pname, name);
			if (stat(pname, &statbuf) != 0) {
				fprintf(stderr, errmsg, name, _acm_fopen_library_dir);
				exit(1);
			}
		}
	}

	return pname;
}

enum state_t {
    SKIPPING_SPACES,
    ARGUMENT
};

int
split (char *s, int *argc, char *argv[])
{
    int done = 0;
    enum state_t state = SKIPPING_SPACES;

    *argc = 0;

    for (; done == 0; ++s) {

		switch (state) {

		case SKIPPING_SPACES:
			if (isspace (*s)) {
			}
			else if (isascii (*s)) {
				argv[*argc] = s;
				(*argc)++;
				state = ARGUMENT;
			}
			else if (*s == '\0') {
				done = 1;
			}
			break;

		case ARGUMENT:
			if (isspace (*s)) {
				*s = '\0';
				state = SKIPPING_SPACES;
			}
			else if (*s == '\0') {
				*s = '\0';
				done = 1;
			}
		}
    }
    return 0;
}

/*
 *  readScene :  Process the scene description file, returns zero if successful
 *               or -1 otherwise.
 */

int
readScene (char *ground, int len)
{
    FILE *f, *f1;
    char file[256], *name, *filep;
    char *argv[32], line[1024];
    char lat[64], lon[64], lat_lon[64];
    double z, heading, freq, mag, length, width;
    int s, i = 0, j, undersample, depthcue, argc;
    craftType *c;
    extern char *sceneFile;	/* from server.c */
    VPoint scale, down, local_fwd, local_right, local_down;
    int done = 0;
    WorldCoordinates w, w1, w2, gsw;
    dis_world_coordinates xyz, xyz1;
    double loc_width, loc_bearing, gs_angle;
    VObject *object = NULL;
    VPolygon **rpoly;
    int rpoly_count;
    VMatrix RWYtoXYZ, XYZtoNED;

	/* initialize list of active viewers -- null list */

	vl_head = vl_tail = NULL;
	vl_count = 0;

	rpoly = NULL;
	rpoly_count = 0;

    name = (sceneFile) ? sceneFile : "default-scene";

    /*
     *  The units used in the objects we've created ate expressed in FEET.
     *  Internally, graphics objects should be METERS.
     */

    scale.x = scale.y = scale.z = FEETtoMETERS (1.0);
    VSetReadObjectScale (&scale);

    f = acm_fopen (name, "r");

    fscanf (f, "%s %s %lf %lf", lat, lon, &z, &heading);
#ifdef FLAT_WORLD
    teamLoc[1].x = x * NM;
    teamLoc[1].y = y * NM;
    teamLoc[1].z = z;
#else
    strcat (lat, lon);
    DISStringToLatLong (lat, &teamLatLon[1]);
    teamLatLon[1].z = FEETtoMETERS (z);
#endif
    teamHeading[1] = DEGtoRAD (heading);

    fscanf (f, "%s %s %lf %lf", lat, lon, &z, &heading);
#ifdef FLAT_WORLD
    teamLoc[2].x = x * NM;
    teamLoc[2].y = y * NM;
    teamLoc[2].z = z;
#else
    strcat (lat, lon);
    DISStringToLatLong (lat, &teamLatLon[2]);
    teamLatLon[2].z = FEETtoMETERS (z);
#endif
    teamHeading[2] = DEGtoRAD (heading);

    /*
     *  Get the ground color (the first call to fgets() gets us to a new line).
     */

    ground[0] = '\0';
    if (fgets (ground, len, f) == (char *) NULL) {
		return -1;
    }
    if (fgets (ground, len, f) == (char *) NULL) {
		return -1;
    }
    len = strlen (ground);
    if (len > 0 && ground[len - 1] == '\n') {
		ground[len - 1] = '\0';
		if (len > 0 && ground[len - 2] == '\r') {
			ground[len - 2] = '\0';
		}
    }

    /*
     *  Now get the NAVAID and runway records
     */

    fgets (line, sizeof (line), f);
    done = 0;
    while (done == 0) {
	len = strlen (line);
	if (len > 0 && line[len - 1] == '\n') {
	    line[len - 1] = '\0';
	}
	split (line, &argc, argv);

    /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
     *
     *  RWY : Runway record
     *
     *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

	if (strcmp (argv[0], "RWY") == 0) {

	    /*
	     *  Parse Latitude/Longitude of both runway ends
	     */
	    length = FEETtoMETERS (atoi (argv[4]));
	    width = FEETtoMETERS (atoi (argv[5]));

	    strcpy (lat_lon, argv[6]);
	    strcat (lat_lon, argv[7]);
	    DISStringToLatLong (lat_lon, &w);
	    w.z = FEETtoMETERS (atoi (argv[3]));

	    addAltitudeEntry (&w);

	    strcpy (lat_lon, argv[8]);
	    strcat (lat_lon, argv[9]);
	    DISStringToLatLong (lat_lon, &w1);
	    w1.z = w.z;

	    DISWorldCoordinatesToGeocentric (&w,
					   (dis_world_coordinates *) & xyz);
	    mag = sqrt (xyz.x * xyz.x + xyz.y * xyz.y + xyz.z * xyz.z);

	    DISWorldCoordinatesToGeocentric (&w1,
					  (dis_world_coordinates *) & xyz1);

	    /*
	     *  Average those two points to generate a midpoint that will be the
	     *  origin of a runway coordinate system.
	     */

	    local_fwd.x = xyz1.x - xyz.x;
	    local_fwd.y = xyz1.y - xyz.y;
	    local_fwd.z = xyz1.z - xyz.z;

	    xyz.x = (xyz.x + xyz1.x) / 2.0;
	    xyz.y = (xyz.y + xyz1.y) / 2.0;
	    xyz.z = (xyz.z + xyz1.z) / 2.0;

	    DISGeocentricToWorldCoordinates (&xyz, &w2);
	    GenerateWorldToLocalMatrix (&w2, &XYZtoNED);
	    down.z = 1.0;
	    down.x = down.y = 0.0;
	    VReverseTransform_ (&down, &XYZtoNED, &local_down);

	    mag = sqrt (
			   local_fwd.x * local_fwd.x +
			   local_fwd.y * local_fwd.y +
			   local_fwd.z * local_fwd.z
		);
	    local_fwd.x /= mag;
	    local_fwd.y /= mag;
	    local_fwd.z /= mag;

	    /*
	     *  A basic property of Cross Products: k x i = j
	     */

	    VCrossProd (&local_down, &local_fwd, &local_right);

	    /*
	     *  Generate a transformation matrix to get from "runway" coordinates to
	     *  Geocentric.
	     */
	    VIdentMatrix (&RWYtoXYZ);

	    RWYtoXYZ.m[0][0] = local_fwd.x;
	    RWYtoXYZ.m[1][0] = local_fwd.y;
	    RWYtoXYZ.m[2][0] = local_fwd.z;

	    RWYtoXYZ.m[0][1] = local_right.x;
	    RWYtoXYZ.m[1][1] = local_right.y;
	    RWYtoXYZ.m[2][1] = local_right.z;

	    RWYtoXYZ.m[0][2] = local_down.x;
	    RWYtoXYZ.m[1][2] = local_down.y;
	    RWYtoXYZ.m[2][2] = local_down.z;

	    RWYtoXYZ.m[0][3] = xyz.x;
	    RWYtoXYZ.m[1][3] = xyz.y;
	    RWYtoXYZ.m[2][3] = xyz.z;

	    /*
	     *  AddRunway will return rpoly_count indicating the number of polygons
	     *  that were generated. rpoly is a malloc'ed vector of pointers to 
		 *  the polygons.
	     */

	    AddRunway (&RWYtoXYZ, length, width, 0,
		       &rpoly, &rpoly_count);

	    /*
	     *  optimize a bit here and put all the
	     *  runways for a given airport in the same object ...
	     *  object->name is the identifier of the current airport.
	     */

	    if ((object != NULL) && (strcmp (object->name, argv[1]) == 0)) {

		/*
		 *  New runway at same airport as last .. add a polygon to the last object.
		 */

		object->polygon = (VPolygon **) realloc (
							    object->polygon,
		    sizeof (VPolygon *) * (object->numPolys + rpoly_count));
		if (!object->polygon) {
		    printf ("memory allocation error\n");
		    exit (1);
		}
		for (j = 0; j < rpoly_count; ++j) {
		    object->polygon[object->numPolys + j] = rpoly[j];
		}
		object->numPolys += rpoly_count;
		free (rpoly);
		rpoly_count = 0;
		VComputeObjectExtent (object);
	    }
	    else {

		/*
		 *    New airport.  New object and craftInfo.
		 */
		object = (VObject *) Vmalloc (sizeof (VObject));
		memset (object, 0, sizeof (VObject));
		object->name = strdup (argv[1]);
		object->numPolys = rpoly_count;
		object->polygon = rpoly;
		object->order = (unsigned short *) NULL;

		VComputeObjectExtent (object);

		stbl[i].type = CT_SURFACE;
		stbl[i].flags = FL_FIXED_OBJECT;
		stbl[i].cinfo = newCraft ();
		if (! stbl[i].cinfo) {
			fprintf (stderr, "Out of craft types while loading scene.\n");
			exit(1);
		}
		stbl[i].cinfo->name = strdup (argv[1]);
		stbl[i].cinfo->object = object;
		stbl[i].cinfo->placeProc = NULL;
		stbl[i].curHeading = stbl[i].curPitch = stbl[i].curRoll = 0.0;
		stbl[i].Sg.x = stbl[i].Sg.y = stbl[i].Sg.z = 0.0;
		if (++i == MAXSURFACE) {
		    fprintf (stderr,
		    "Only the first %d surface objects will be displayed.\n",
			     MAXSURFACE);
		    i = MAXSURFACE - 1;
		}
	    }
	}

    /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
     *
     *  NAV: NAVAID record
     *
     *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

	else if (strcmp (argv[0], "NAV") == 0) {

	    /*
	     *  Parse Latitude/Longitude
	     */

	    strcpy (lat_lon, argv[3]);
	    strcat (lat_lon, argv[4]);
	    DISStringToLatLong (lat_lon, &w);
	    w.z = FEETtoMETERS (atoi (argv[5]));
	    freq = strtod (argv[7], (char **) NULL);

	    AddNavaid (argv[1], argv[2], &w, argv[6], freq);
	}

    /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
     *
     *  ILS :  Instrument Landing System record
     *
     *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

	else if (strcmp (argv[0], "ILS") == 0) {

	    /*
	     *  Parse Latitude/Longitude of localizer transmitter
	     */

	    strcpy (lat_lon, argv[5]);
	    strcat (lat_lon, argv[6]);
	    DISStringToLatLong (lat_lon, &w);
	    w.z = FEETtoMETERS (atoi (argv[9]));

	    /*
	     *  Is Glide Slope transmitter information present?
	     */

	    if (strcmp (argv[7], "-") != 0) {
		strcpy (lat_lon, argv[7]);
		strcat (lat_lon, argv[8]);
		DISStringToLatLong (lat_lon, &gsw);
		gsw.z = w.z;
	    }
	    freq = strtod (argv[4], (char **) NULL);
	    loc_width = strtod (argv[11], (char **) NULL);
	    loc_bearing = strtod (argv[12], (char **) NULL);
	    gs_angle = strtod (argv[13], (char **) NULL);

	    AddILS (argv[3], argv[2], &w, &gsw, argv[10], freq,
		    loc_width, loc_bearing, gs_angle);
	}
	else if (strcmp (argv[0], "FEATURES") == 0) {
	    done = 1;
	}
	if (done == 0) {
	    fgets (line, sizeof (line), f);
	}
    }

  /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
   *
   *  Now the simple terrain/man-made features
   *
   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

    while (s = fscanf (f, "%s %s %s %lf %lf",
		       file, lat, lon, &z, &heading) == 5) {

	depthcue = 1;

	if (file[0] == '@') {
	    filep = &file[1];
	    undersample = 1;
	}
	else if (file[0] == '+') {
	    filep = &file[1];
	    depthcue = 0;
	}
	else {
	    filep = file;
	    undersample = 0;
	}
	stbl[i].type = CT_SURFACE;
	if ((c = lookupCraft (filep))) {
	    stbl[i].cinfo = c;
	}
	else {
		char *p = strrchr (filep, '.');

	    f1 = acm_fopen (filep, "r");
	    stbl[i].cinfo = newCraft ();
	    stbl[i].cinfo->name = strdup (filep);
		stbl[i].cinfo->object = NULL;
		if (p != NULL && (strcmp (p, ".dxf") == 0 ||
						  strcmp (p, ".DXF") == 0)) {
			stbl[i].cinfo->object = VReadDepthCueuedDXFObject (f1,
															   depthcue);
		}
		else {
			stbl[i].cinfo->object = VReadDepthCueuedObject (f1,
															depthcue);
		}

	    if (!stbl[i].cinfo->object) {
			fprintf (stderr,
					 "Error reading object %s\n", filep);
			exit (1);
	    }
	    if (undersample) {
			undersampleObject (stbl[i].cinfo->object, 3);
	    }
	    fclose (f1);
	}
#ifdef FLAT_WORLD
	stbl[i].Sg.x = x * NM;
	stbl[i].Sg.y = y * NM;
	stbl[i].Sg.z = z;
#else
	strcat (lat, lon);
	DISStringToLatLong (lat, &stbl[i].w);
	stbl[i].w.z = FEETtoMETERS (z);
	DISWorldCoordinatesToGeocentric (&stbl[i].w,
				    (dis_world_coordinates *) & stbl[i].Sg);
#endif
	stbl[i].curHeading = DEGtoRAD (heading);
	stbl[i].curPitch = stbl[i].curRoll = 0.0;
	if (++i == MAXSURFACE) {
	    fprintf (stderr,
		   "Only the first %d surface objects will be displayed.\n",
		     MAXSURFACE);
	    fclose (f);
	    return 0;
	}
    }
    fclose (f);
    return (s == EOF || s == 0) ? 0 : -1;
}

/*
 *  undersampleObject
 *
 *  Remove some of the detail in an object to reduce the CPU overhead
 *  of transforming and clipping it.
 */

void
undersampleObject (VObject * obj, int rate)
{
    int i, j, k;

    k = 0;

    for (i = 1; i < obj->numPolys; ++i) {
	for (j = 0; j < obj->polygon[i]->numVtces; j += rate) {
	    obj->polygon[i]->vertex[k++] =
		obj->polygon[i]->vertex[j];
	}
	obj->polygon[i]->numVtces = k;
    }
}

/*
 *  init :  Set up the acm server
 */

int
init (const char *runtime_directory)
{
    int i;
    craft *p;
    alarm_id_t id;
    char ground[256];

	if (runtime_directory != NULL) {
		strcpy (_acm_fopen_library_dir, runtime_directory);
	}

    polyCount = 32768;
    poly = (VPolygon **) Vmalloc (sizeof (VPolygon *) * polyCount);

    cbase = -CLOUD_BASE;
    ctop = -CLOUD_TOP;

    /*
     *  VSetDepthCue() must be called before calling VSetVisibility().
     */

    VSetDepthCue (VAllocColor ("#A6BBCD"), depth_cue_steps);
    VSetVisibility (visibility);

    for ((i = 0, p = stbl); i < MAXSURFACE; (++i, ++p))
		p->type = CT_FREE;

    for ((i = 0, p = ptbl); i < MAXPLAYERS; (++i, ++p)) {
		p->pIndex = i;
		p->type = CT_FREE;
    }

    for ((i = 0, p = mtbl); i < MAXPROJECTILES; (++i, ++p)) {
		p->pIndex = i;
		p->type = CT_FREE;
    }

    acm_srand ((int) time (0));

    HUDColor = VAllocColor ("white");
    whiteColor = VAllocColor ("white");
    blackColor = VAllocColor ("black");
    radarColor = VAllocColor ("#0c0");
    HSIMagentaColor = VAllocColor ("magenta");
    radarBackgroundColor = VAllocColor ("#094200");
    cloudColor = VAllocDepthCueuedColor ("#ccc", 1);

    buildExplosion ();

    /*
     *  Set up the scene
     */

    if (readScene (ground, sizeof (ground)) != 0) {
		fprintf (stderr, "Fatal error\n");
		exit (1);
    }

    groundColor = VAllocDepthCueuedColor (ground, 1);

    /*
     *  Compile the aircraft inventory, DIS entity/munition maps
     */

    if (compileAircraftInventory () != 0) {
		fprintf (stderr, "Fatal error\n");
		exit (1);
    }

    if (compileEntityMap("object-map.txt", &eo_map_count, &eo_map) != 0) {
		fprintf (stderr, "Fatal error\n");
		exit(1);
    }

    if (compileMunitionMap("munition-map.txt",
						   &mun_map_count, &mun_map) != 0) {
		fprintf (stderr, "Fatal error\n");
		exit(1);
    }

    /*
     *  Initialize weapons
     */

    initaim9 ();
    initm61a1 ();
	initmk82 ();
	initaim120 ();

    /*
     *  Set up the radio/navaid check.
     */

    id = addAlarm (5.0, DMECheckAlarm, NULL, NULL);

    /*
     *  Add the periodic resupply check procedure
     */

    id = addAlarm (RESUPPLY_INTERVAL, resupplyCheck, NULL, NULL);

    return 0;
}

void
closeScene()
{
	int i;

	for (i=0; i<MAXSURFACE; ++i) {
		if (stbl[i].type != CT_FREE) {
		}
	}
}

void
freeAll(void)
{
	freeRendering();
	freeCraftTypes();
	freeAllNavaids();
	freeEffects();
	dis_close();
	releaseVResources ();
}
