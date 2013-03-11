#include <syscfg.h> // Path is project specific.
// Created: Tue Apr 10 15:54:01 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Eric "Meat" Hall

#include "dbug/dbug.h"

#include "str/stringLib.h"
#include "sdm/sdmLoad.h"
#include "dbse/dbseWrapper.h"
#include "dbse/dbseConnection.h"
#include "sdm/sdmParse.h"
#include "include/mydbQuery.h"
#include "wwii/wwiiString.h"

DBHANDLE dbh;
extern DBCONNECTION dbConnection ;

static char _statement[4096] ;
static char g_name[MAX_STRT_NAME_LENGTH], g_fileName[MAX_OBJNAME_LENGTH];

typedef struct CPWITHID
{
	struct CPWITHID* next ;
	struct CPWITHID* prev ;
	SINT32		cp_oid ;
	SINT32		ox, oy, id ;
	SCHAR		name[81] ;
	CP			cp ;
	LOGICAL		bIsInDB ;
	LOGICAL		bIsCurrent ;
	WWII_COUNTRY	owner ;
	WWII_COUNTRY	controller ;
	UINT32		numLinks ;
	SINT32		absx, absy ;
} CPWITHID ;
static CPWITHID* cps;
static CPWITHID* lastCW ;
static UINT32 uMaxCPOID ;

typedef struct FACWITHID
{
	struct FACWITHID* next ;
	struct FACWITHID* prev ;
	SINT32		fac_oid ;
	SINT32		ox, oy, id, sx, sy ;
	SCHAR		name[81] ;
	FACILITY	fac ;
	LOGICAL		bIsInDB ;
	LOGICAL		bIsCurrent ;
	WWII_COUNTRY owner ;
	SINT32		absx, absy ;
} FACWITHID ;
static FACWITHID* facs ;
static FACWITHID* lastFW ;
static UINT32 uMaxFacOID ;

typedef struct BUILDWITHID
{
	struct BUILDWITHID* next ;
	struct BUILDWITHID* prev ;
	SINT32		oid ;
	BUILDING	object ;
	SINT32	ox, oy ;
} BUILDWITHID ;
static BUILDWITHID* builds ;
static BUILDWITHID* lastBW ;
static UINT32 uNumBuilds ;

typedef struct DLINKWITHID
{
	struct DLINKWITHID* next ;
	struct DLINKWITHID* prev ;
	SINT32		oid ;
	DEPOT_LINK	object ;
} DLINKWITHID ;
static DLINKWITHID* dlinks ;
static DLINKWITHID* lastDLW ;
static UINT32 uNumDlinks ;

void dbseAddSupercell(int x, int y);

#ifdef TESTING
# define dbseDoStatement(x, y, z) printf("%s\n", y)
# define SimpleQuery(dbh, ...) printf(__VA_ARGS__)
#else
# define SimpleQuery(dbh, ...) DBSimpleQuery(dbh, __VA_ARGS__)
#endif

//////////////////////////////////////////////////////////////////////
// Execute a statement that requires us to abort if it fails.

static void
_executeStatement(const char* what_, const char* const statement_)
{
	try
	{
		DBQuery q(dbh, statement_);
	}
	catch (std::exception& e)
	{
		fprintf(stderr, "Query: %s\n", statement_);
		LogAbort("Failed to %s: %s", what_, e.what());
	}
}

//////////////////////////////////////////////////////////////////////
// Execute a statement that requires us to abort if it fails.

static void
_executeStatementf(const char* what_, const char* const fmt_, ...)
{
	try
	{
		va_list args;
		va_start(args, fmt_);
		vsnprintf(_statement, sizeof(_statement), fmt_, args);
		va_end(args);
		DBQuery q(dbh, _statement);
	}
	catch (std::exception& e)
	{
		LogAbort("Failed to %s: %s", what_, e.what());
	}
}

static bool
_getFilenameXY(const std::string& filename, SINT16* scx, SINT16* scy)
{
	UINT32 n = filename.rfind('/') + 1 ;
	if ( n >= filename.length() )
		n = 0 ;
	const UINT32 assigned = sscanf(filename.c_str() + n, "%hx_%hx", scx, scy) ;
	return ( assigned == 2 ) ;
}

void
dbseAddSupercell(int x ,int y)
{
	_executeStatementf("add supercell", "INSERT IGNORE INTO strat_supercell(scx,scy,added)VALUES(%d,%d,NOW())", x, y) ;
}

//////////////////////////////////////////////////////////////////////
// choke point

static SINT32
_findCP(SINT32 ox, SINT32 oy, SINT32 id)
{
	CPWITHID* cpw ;

	for ( cpw = cps ; cpw != NULL ; cpw = cpw->next )
	{
		if ( cpw->ox == ox && cpw->oy == oy && cpw->id == id && cpw->bIsCurrent )
			return cpw->cp_oid ;
	}

	return 0 ;
}

void*
_newCPW(UINT32 uOID, SINT32 ox, SINT32 oy, SINT32 id, const char* name, WWII_COUNTRY owner, WWII_COUNTRY controller, SINT32 absx, SINT32 absy)
{
	CPWITHID* newCP ;

	newCP = new CPWITHID ;
	memset(newCP, 0, sizeof(CPWITHID)) ;

	newCP->cp_oid = uOID ;
	newCP->ox = ox ;
	newCP->oy = oy ;
	newCP->id = id ;
	strlcpy(newCP->name, name, sizeof(newCP->name)) ;
	newCP->owner = owner ;
	newCP->controller = controller ;
	newCP->absx = absx ;
	newCP->absy = absy ;

	if ( uOID > uMaxCPOID )
		uMaxCPOID = uOID ;

	// Link it to the list

	if ( cps == NULL )
		cps = newCP ;
	newCP->prev = lastCW ;
	if ( lastCW != NULL )
		lastCW->next = newCP ;
	newCP->next = NULL ;
	lastCW = newCP ;

	return (void*)newCP ;
}

LOGICAL
dbseLoadCP(CP* cp)
{
	SINT32 ox, oy ;
	CPWITHID* newCP ;

	if ( cp == NULL )
		return FALSE ;

	ox = cp->posx / 800 ;
	oy = cp->posy / 800 ;

	for ( newCP = cps ; newCP != NULL ; newCP = newCP->next )
	{
		if ( newCP->bIsCurrent )
			continue ;
		if ( newCP->ox == ox && newCP->oy == oy && newCP->id == cp->oid.id )
			break ;
		if ( strcasecmp(cp->name, newCP->name) == 0 )
			break ;
	}

	if ( newCP == NULL )
	{	// Reached end of list, its not in the table
		uMaxCPOID++ ;
		newCP = (CPWITHID*)_newCPW(uMaxCPOID, ox, oy, cp->oid.id, cp->name, (WWII_COUNTRY)cp->country, (WWII_COUNTRY)cp->country, cp->posx, cp->posy) ;
		newCP->bIsInDB = FALSE ;
	}
	else
	{
		newCP->bIsInDB = TRUE ;	// Innocent until proven guilty
		if ( newCP->owner != cp->country )
		{
			newCP->owner = (WWII_COUNTRY)cp->country ;
			newCP->bIsInDB = FALSE ;
		}
		if ( newCP->controller != cp->country )
		{
			newCP->controller = (WWII_COUNTRY)cp->country ;
			newCP->bIsInDB = FALSE ;
		}
		if ( newCP->ox != ox || newCP->oy != oy || newCP->id != cp->oid.id || newCP->absx != cp->posx || newCP->absy != cp->posy )
		{
			// Update the entries
			newCP->ox = ox ;
			newCP->oy = oy ;
			newCP->id = cp->oid.id ;
			newCP->absx = cp->posx ;
			newCP->absy = cp->posy ;
			newCP->bIsInDB = FALSE ;
		}
		if ( strcmp(newCP->name, cp->name) != 0 )
		{
			strlcpy(newCP->name, cp->name, sizeof(newCP->name)) ;
			newCP->bIsInDB = FALSE ;
		}
	}

	memcpy(&newCP->cp, cp, sizeof(CP)) ;
	newCP->bIsCurrent = TRUE ;

	return TRUE ;
}

static void
dbseSaveCP(CPWITHID* cpw)
{
	if ( cpw == NULL )
	{
		if ( _statement[0] != 0 )
			_executeStatement("save cp", _statement) ;
		return ;
	}

	// Don't save place holders
	if ( !cpw->bIsCurrent )
		return ;

	static char* start = NULL ;
	static char* p = NULL ;

	if ( _statement[0] == 0 )
	{
		start = _statement + sprintf(_statement
			, "INSERT INTO strat_cp"
				"(cp_oid,cp_ox,cp_oy,cp_type,cp_id"
				",name,objectname"
				",side,country,conside,concountry"
				",originalside,originalcountry"
				",scx,scy"
				",num_links"
				",absx,absy"
				",added"
			    ")VALUES") ;
		p = start ;
	}


	CP* cp = &cpw->cp ;
	SINT32 sx, sy ;

	strescape(g_name,cp->name,'\'');
	strescape(g_fileName,cp->fileName,'\'');

	sx = cpw->ox >> 4 ;
	sy = cpw->oy >> 4 ;

	p += sprintf(p, "%c"
						"(%u,%d,%d,%u,%u"
						",'%s','%s'"
			    		",%u,%u,%u,%u"
			    		",%u,%u"
			    		",%d,%d"
						",%u"
						",%d,%d"
						",now())"
					, ( p > start ) ? ',' : ' '
			    	, cpw->cp_oid, cpw->ox, cpw->oy, cp->oid.objType, cp->oid.id
			    	, g_name, g_fileName
			    	, wwiiSide(cpw->owner), cpw->owner, wwiiSide(cpw->controller), cpw->controller
			    	, wwiiSide(cp->country), cp->country
			    	, sx, sy
					, cpw->numLinks
					, cpw->absx, cpw->absy
			    	) ;

	assert( p < _statement + 4090 ) ;
	if ( p > _statement + 3800 )
	{
		_executeStatement("save cp", _statement) ;
		p = start ;
	}

	return ;
}

//////////////////////////////////////////////////////////////////////
// facility

static SINT32
_findFacility(SINT32 ox, SINT32 oy, SINT32 id)
{
FACWITHID* facw ;

	for ( facw = facs ; facw != NULL ; facw = facw->next )
	{
		if ( facw->ox == ox && facw->oy == oy && facw->id == id )
			return facw->fac_oid ;
	}

	return 0 ;
}

SINT32
_findFacilityBySC(SINT32 sx, SINT32 sy, SINT32 id)
{
FACWITHID* facw ;

	for ( facw = facs ; facw != NULL ; facw = facw->next )
	{
		if ( facw->sx == sx && facw->sy == sy && facw->id == id )
			return facw->fac_oid ;
	}

	return 0 ;
}

void*
_newFacW(UINT32 uOID, SINT32 ox, SINT32 oy, SINT32 id, const char* name, WWII_COUNTRY owner, WWII_COUNTRY /*dummy*/, SINT32 absx, SINT32 absy)
{
	FACWITHID* newFac ;

	newFac = new FACWITHID ;
	memset(newFac, 0, sizeof(FACWITHID)) ;

	newFac->fac_oid = uOID ;
	newFac->ox = ox ;
	newFac->oy = oy ;
	newFac->id = id ;
	newFac->sx = ox >> 4 ;
	newFac->sy = oy >> 4 ;
	strlcpy(newFac->name, name, sizeof(newFac->name)) ;
	newFac->owner = owner ;
	newFac->absx = absx ;
	newFac->absy = absy ;

	if ( uOID > uMaxFacOID )
		uMaxFacOID = uOID ;

	// Link it to the list

	if ( facs == NULL )
		facs = newFac ;
	newFac->prev = lastFW ;
	if ( lastFW != NULL )
		lastFW->next = newFac ;
	newFac->next = NULL ;
	lastFW = newFac ;

	newFac->bIsCurrent = FALSE ;

	return (void*)newFac ;
}

LOGICAL
dbseLoadFacility(FACILITY* fac)
{
	FACWITHID* newFac ;
	SINT32 ox, oy ;

	if ( fac == NULL )
		return FALSE ;

	ox = fac->posx / 800 ;
	oy = fac->posy / 800 ;

	for ( newFac = facs ; newFac != NULL ; newFac = newFac->next )
	{
		if ( newFac->bIsCurrent )
			continue ;
		if ( newFac->ox == ox && newFac->oy == oy && newFac->id == fac->oid.id )
			break ;
		if ( strcasecmp(fac->name, newFac->name) == 0 )
			break ;
	}

	if ( newFac == NULL )
	{
		uMaxFacOID++ ;
		newFac = (FACWITHID*)_newFacW(uMaxFacOID, ox, oy, fac->oid.id, fac->name, (WWII_COUNTRY)fac->originalCountry, WWII_NO_COUNTRY, fac->posx, fac->posy) ;
		newFac->bIsInDB = FALSE ;
	}
	else
	{
		newFac->bIsInDB = TRUE ;
		if ( newFac->owner != fac->originalCountry )
		{
			newFac->owner = (WWII_COUNTRY)fac->originalCountry ;
			newFac->bIsInDB = FALSE ;
		}
		if ( newFac->ox != ox || newFac->oy != oy || newFac->id != fac->oid.id || newFac->absx != fac->posx || newFac->absy != fac->posy )
		{
			// Update the entries
			newFac->ox = ox ;
			newFac->oy = oy ;
			newFac->id = fac->oid.id ;
			newFac->sx = ox >> 4 ;
			newFac->sy = oy >> 4 ;
			newFac->absx = fac->posx ;
			newFac->absy = fac->posy ;
			newFac->bIsInDB = FALSE ;
		}
		if ( strcmp(newFac->name, fac->name) != 0 )
		{
			strlcpy(newFac->name, fac->name, sizeof(newFac->name)) ;
			newFac->bIsInDB = FALSE ;
		}
	}

	memcpy(&newFac->fac, fac, sizeof(FACILITY)) ;
	newFac->bIsCurrent = TRUE ;

	return TRUE ;
}

static void
dbseSaveFACILITY(FACWITHID* facw)
{
	if ( facw == NULL )
	{
		if ( _statement[0] != 0 )
			_executeStatement("save facility", _statement);
		return ;
	}

	// Don't save place holders
	if ( !facw->bIsCurrent )
		return ;

	static char* start = NULL ;
	static char* p = NULL ;

	if ( _statement[0] == 0 )
	{
		start = _statement + sprintf(_statement
			, "INSERT INTO strat_facility"
				"(facility_oid,facility_ox,facility_oy"
				",facility_type,facility_id,facility_subtype"
				",cp_ox,cp_oy,cp_id,cp_oid"
				",name,objectname"
				",side,country"
				",originalside,originalcountry"
				",scx,scy"
				",absx,absy"
				",added"
				")"
				" VALUES") ;
		p = start ;
	}


	FACILITY* facility = &facw->fac ;

	// Find the oid of the parent cp
	SINT32 cp_oid ;
	cp_oid = _findCP(facility->cpid.cell.ox, facility->cpid.cell.oy, facility->cpid.id) ;
	if ( cp_oid == 0 )
	{
		fprintf(stderr, "facility %s (%d.%d.%d) can't find parent cp\n"
						, facility->name, facw->ox, facw->oy, facility->oid.id) ;
		return ;
	}

	strescape(g_name, facility->name, '\'') ;
	strescape(g_fileName, facility->fileName, '\'') ;

	p += sprintf(p
				, "%c(%u,%d,%d"
					",%u,%u,%u"
					",%d,%d,%d,%u"
					",'%s','%s'"
					",%u,%u"
					",%u,%u"
					",%d,%d"
					",%d,%d"
					",now()"
				")"
				, (p > start) ? ',' : ' '
				, facw->fac_oid, facw->ox, facw->oy
				, facility->oid.objType, facility->oid.id, facility->subType
				, facility->cpid.cell.ox, facility->cpid.cell.oy, facility->cpid.id
				, cp_oid, g_name, g_fileName
				, wwiiSide(facw->owner), facw->owner
				, facility->originalSide, facility->originalCountry
				, facw->sx, facw->sy
				, facw->absx, facw->absy
				);

	assert( p < _statement + 4090 ) ;
	if ( p > _statement + 3800 )
	{
		_executeStatement("save facility", _statement);
		p = start ;
	}

	return ;
}

//////////////////////////////////////////////////////////////////////
// building

static BUILDWITHID*
_findBuilding(SINT32 ox, SINT32 oy, SINT32 id)
{
	BUILDWITHID* buildw ;

	for ( buildw = builds ; buildw != NULL ; buildw = buildw->next )
	{
		if ( buildw->ox == ox && buildw->oy == oy && buildw->object.oid.id == id )
			return buildw ;
	}

	return NULL ;
}

LOGICAL
dbseLoadBuilding(BUILDING* object)
{
	if ( object == NULL )
		return FALSE ;

	BUILDWITHID* newObj = new BUILDWITHID ;
	newObj->object = *object ;
	newObj->oid = ++uNumBuilds ;

	if ( builds == NULL )
		builds = newObj ;
	newObj->prev = lastBW ;
	if ( lastBW != NULL )
		lastBW->next = newObj ;
	newObj->next = NULL ;
	lastBW = newObj ;

	return TRUE ;
}

static void
dbseSaveBUILDING(BUILDWITHID* buildw)
{
	if ( buildw == NULL )
	{
		if ( _statement[0] != 0 )
			_executeStatement("save building", _statement);
		return ;
	}

	static char* start = NULL ;
	static char* p = NULL ;

	if ( _statement[0] == 0 )
	{
		start = _statement + sprintf(_statement
			, "INSERT INTO strat_object"
				"(object_oid, object_ox,object_oy,object_type,object_id,object_subtype,"
				  "facility_oid,name,objectname,control,scx,scy,absx,absy,added)"
				"VALUES") ;
		p = start ;
	}

	SINT32 sx, sy ;
	SINT32 fac_oid ;
	BUILDING* building ;

	building = &buildw->object ;

	strescape(g_name, building->name, '\'');
	strescape(g_fileName, building->fileName, '\'');

	buildw->ox = building->posx / 800 ;
	buildw->oy = building->posy / 800 ;
	sx = buildw->ox >> 4 ;
	sy = buildw->oy >> 4 ;

	fac_oid = _findFacilityBySC(sx, sy, building->pid.id) ;
	if ( fac_oid == 0 )
	{
		fprintf(stderr, "building %s (%d.%d.%u %s) can't find parent facility (%d.%d.%u)\n"
					, g_name
					, buildw->ox, buildw->oy, building->oid.id
					, building->fileName
					, sx, sy, building->pid.id
					) ;
		return ;
	}

	p += sprintf(p, "%c(%u,%d,%d"
					",%u,%u,%u"
					",%u"
					",'%s','%s'"
					",'%s'"
					",%d,%d"
					",%d,%d"
					",now())"
			, (p > start) ? ',' : ' '
			, buildw->oid, buildw->ox, buildw->oy
			, building->oid.objType, building->oid.id, building->subType
			, fac_oid
			, g_name, g_fileName
			, building->Xcontrol ? "true" : "false"
			, sx, sy
			, building->posx, building->posy
			);

	if ( p >= start + 3500 )
	{
		_executeStatement("save building", _statement);
		p = start ;
	}

	return ;
}

static bool
_addLink(SINT32 ox, SINT32 oy, SINT32 id)
{
	for ( BUILDWITHID* b = builds ; b != NULL ; b = b->next )
	{
		SINT32 box = b->object.posx / 800 ;
		SINT32 boy = b->object.posy / 800 ;
		if ( box == ox && boy == oy && b->object.oid.id == id )
		{
			SINT32 sx = box >> 4 ;
			SINT32 sy = boy >> 4 ;
			for ( FACWITHID* f = facs ; f != NULL ; f = f->next )
			{
				if ( f->sx == sx && f->sy == sy && f->id == b->object.pid.id )
				{
					for ( CPWITHID* c = cps ; c != NULL ; c = c->next )
					{
						if ( c->ox == f->fac.cpid.cell.ox && c->oy == f->fac.cpid.cell.oy && c->id == f->fac.cpid.id )
						{
							c->numLinks++ ;
							return true ;
						}
					}
					printf("No cp\n") ; return false ;
				}
			}
			printf("No fac\n") ; return false ;
		}
	}
	printf("No building\n") ;
	return false ;
}

//////////////////////////////////////////////////////////////////////
// depot link

LOGICAL
dbseLoadDepotLink(DEPOT_LINK* object)
{
	DLINKWITHID* newObj ;

	if ( object == NULL )
		return FALSE ;

	newObj = new DLINKWITHID ;
	newObj->object = *object ;
	newObj->oid = ++uNumDlinks ;

	if ( dlinks == NULL )
		dlinks = newObj ;
	newObj->prev = lastDLW ;
	if ( lastDLW != NULL )
		lastDLW->next = newObj ;
	newObj->next = NULL ;
	lastDLW = newObj ;

	return TRUE ;
}

static void
dbseSaveDEPOT_LINK(DLINKWITHID* linkw)
{
	if ( linkw == NULL )
	{
		if ( _statement[0] != 0 )
			_executeStatement("save depot link", _statement);
		return ;
	}

	static char* start = NULL ;
	static char* p = NULL ;

	if ( _statement[0] == 0 )
	{
		start = _statement + sprintf(_statement,
								"INSERT INTO strat_link"
									"(link_oid,link_ox,link_oy,link_type,link_id"
									",scx,scy"
									",startdepot_ox,startdepot_oy,startdepot_id,startdepot_oid"
									",enddepot_ox,enddepot_oy,enddepot_id,enddepot_oid"
									",firebase_ox,firebase_oy,firebase_id,firebase_oid"
									",distance,name,added)"
									" VALUES") ;
		p = start ;
	}

	SINT32 ox,oy,sx,sy;

	DEPOT_LINK* link = &linkw->object ;

	ox = link->startDepotId.cell.ox ;
	oy = link->startDepotId.cell.oy ;
	sx = ox >> 4 ;
	sy = oy >> 4 ;

	BUILDWITHID* bldg ;
	bldg = _findBuilding(link->startDepotId.cell.ox, link->startDepotId.cell.oy, link->startDepotId.id) ;
	if ( bldg == NULL )
	{
		fprintf(stderr, "link %s (%d.%d.%d) unable to find startDepot building\n"
						, g_name, ox, oy, link->oid.id) ;
		return ;
	}
	UINT32 start_fac_oid = _findFacilityBySC(bldg->ox >> 4, bldg->oy >> 4, bldg->object.pid.id) ;
	if ( start_fac_oid == 0 )
	{
		fprintf(stderr, "link %s (%d.%d.%d) unable to find startDepot facility\n"
						, g_name, ox, oy, link->oid.id) ;
		return ;
	}

	bldg = _findBuilding(link->endDepotId.cell.ox, link->endDepotId.cell.oy, link->endDepotId.id) ;
	if ( bldg == NULL )
	{
		fprintf(stderr, "link %s (%d.%d.%d) unable to find endDepot building\n"
						, g_name, ox, oy, link->oid.id) ;
		return ;
	}
	UINT32 end_fac_oid = _findFacilityBySC(bldg->ox >> 4, bldg->oy >> 4, bldg->object.pid.id) ;
	if ( end_fac_oid == 0 )
	{
		fprintf(stderr, "link %s (%d.%d.%d) unable to find endDepot facility\n"
						, g_name, ox, oy, link->oid.id) ;
		return ;
	}

	UINT32 fb_fac_oid = 0 ;
	if ( link->firebaseId.id != 0 )
	{
		fb_fac_oid = _findFacility(link->firebaseId.cell.ox, link->firebaseId.cell.oy, link->firebaseId.id) ;
		if ( fb_fac_oid == 0 )
		{
			fprintf(stderr, "link %s (%d.%d.%d) unable to find firebase Facility (%d.%d.%d)\n"
							, g_name, ox, oy, link->oid.id
							, link->firebaseId.cell.ox, link->firebaseId.cell.oy, link->firebaseId.id) ;
			return ;
		}
	}

	strescape(g_name, link->name,'\'') ;

	p += sprintf(p, "%c(%u,%d,%d,%d,%d"
						",%d,%d"
						",%d,%d,%d,%u"
						",%d,%d,%d,%u"
						",%d,%d,%d,%u"
						",%d,'%s',now()"
					")"
			, (p > start) ? ',' : ' '
			, linkw->oid, ox, oy, link->oid.objType, link->oid.id, sx, sy
			, link->startDepotId.cell.ox,link->startDepotId.cell.oy,link->startDepotId.id,start_fac_oid
			, link->endDepotId.cell.ox,link->endDepotId.cell.oy,link->endDepotId.id,end_fac_oid
			, link->firebaseId.cell.ox,link->firebaseId.cell.oy,link->firebaseId.id,fb_fac_oid
			, link->distance,link->name
			) ;

	assert( p < _statement + 4090 ) ;
	if ( p > _statement + 3800 )
	{
		_executeStatement("save depot link", _statement);
		p = start ;
	}

	return ;
}

//////////////////////////////////////////////////////////////////////
// update the CP index for an entry

static char*
_indexSave(char* start, char* p, UINT32 oid, SINT32 ox, SINT32 oy, UINT32 id, char* name)
{
	static char escapedName[128] ;
	strescape(escapedName, name, '\'') ;

	p = p + sprintf(p, "%c(%u,%d,%d,%u,'%s')", (p > start) ? ',' : ' ', oid, ox, oy, id, escapedName) ;

	static const char* CUTOFF = _statement + 3940 ;
	if ( p < CUTOFF )
		return p ;

	_executeStatement("save index", _statement);

	return start ;
}

//////////////////////////////////////////////////////////////////////
// Update the IDs indexes

void
dbseSaveIndexChanges()
{
	CPWITHID* cpw ;
	FACWITHID* facw ;

	printf("~~~ Updating ID indexes:") ;

	char* start ;
	char* p ;

	printf(" CPs:") ;
	start = _statement + sprintf(_statement, "REPLACE INTO ids_cp(cp_oid,cp_ox,cp_oy,cp_id,name)VALUES") ;
	p = start ;

	for ( cpw = cps ; cpw != NULL ; cpw = cpw->next )
	{
		if ( !cpw->bIsInDB && cpw->bIsCurrent )
		{
			p = _indexSave(start, p, cpw->cp_oid, cpw->ox, cpw->oy, cpw->cp.oid.id, cpw->name) ;
			cpw->bIsInDB = TRUE ;
		}
	}
	if ( p > start )
		_executeStatement("save index", _statement);
	printf("OK") ;

	printf(" Facilities:") ;
	start = _statement + sprintf(_statement, "REPLACE INTO ids_facility(facility_oid,facility_ox,facility_oy,facility_id,name)VALUES") ;
	p = start ;

	for ( facw = facs ; facw != NULL ; facw = facw->next )
	{
		if ( !facw->bIsInDB && facw->bIsCurrent )
		{
			p = _indexSave(start, p, facw->fac_oid, facw->ox, facw->oy, facw->fac.oid.id, facw->name) ;
			facw->bIsInDB = TRUE ;
		}
	}
	if ( p > start )
		_executeStatement("save index", _statement);
	printf("OK") ;

	printf("\r\n") ;
}

static void
_begin(const char* type)
{
	_statement[0] = 0 ;
	SimpleQuery(dbh, "TRUNCATE strat_%s", type) ;
	SimpleQuery(dbh, "ALTER TABLE strat_%s DISABLE KEYS", type) ;
}

static void
_end(const char* type)
{
	SimpleQuery(dbh, "ALTER TABLE strat_%s ENABLE KEYS", type) ;
}

void
title(const char* msg)
{
	printf("\r%-79s\n", msg) ;
}

//////////////////////////////////////////////////////////////////////
// save everything

SINT32
dbseResolveReferences()
{
	CPWITHID* cpw ;
	FACWITHID* facw ;
	BUILDWITHID* buildw ;
	DLINKWITHID* dlinkw ;

	title("~~~ Loaded data ~~~") ;

	title("~~~ resolving links ~~~") ;
	for ( dlinkw = dlinks ; dlinkw != NULL ; dlinkw = dlinkw->next )
	{
		if ( !_addLink(dlinkw->object.startDepotId.cell.ox, dlinkw->object.startDepotId.cell.oy, dlinkw->object.startDepotId.id) )
			printf("no cp for %d.%d.%d %s\n", dlinkw->object.startDepotId.cell.ox, dlinkw->object.startDepotId.cell.oy, dlinkw->object.startDepotId.id, dlinkw->object.name) ;
	}

	title("~~~ saving cps ~~~") ;
	_begin("cp") ;
	for ( cpw = cps ; cpw != NULL ; cpw = cpw->next )
	{
		dbseSaveCP(cpw) ;
	}
	dbseSaveCP(NULL) ;
	_end("cp") ;

	title("~~~ saving facilities ~~~") ;
	_begin("facility") ;
	for ( facw = facs ; facw != NULL ; facw = facw->next )
	{
		dbseSaveFACILITY(facw) ;
	}
	dbseSaveFACILITY(NULL) ;
	_end("facility") ;

	title("~~~ saving buildings ~~~") ;
	_begin("object") ;
	for ( buildw = builds ; buildw != NULL ; buildw = buildw->next )
	{
		dbseSaveBUILDING(buildw) ;
	}
	dbseSaveBUILDING(NULL) ;
	_end("object") ;

	title("~~~ saving depot links ~~~") ;
	_begin("link") ;
	for ( dlinkw = dlinks ; dlinkw != NULL ; dlinkw = dlinkw->next )
	{
		dbseSaveDEPOT_LINK(dlinkw) ;
	}
	dbseSaveDEPOT_LINK(NULL) ;
	_end("link") ;

	return 0 ;
}

SINT32
dbseLoadFile(SCHAR* path)
{

	if ( path == NULL || *path == '\0' )
	{
		sdmFinish() ;
		return 1 ;
	}

	SINT32 retval ;
	retval = sdmOpenFile(path, 0) ;
	if ( retval != 0 )
	{
		fprintf(stderr,"Unable to load file %s to database\n",path);
		return retval;
	}

	printf("\r~~~ %s: ", path);

	short int x = 0, y = 0 ;
	_getFilenameXY(path, &x, &y) ;

	dbseAddSupercell(x,y);

	printf(" CPs:") ;
	CP* cp = sdmGetNextCP();
	while (cp)
	{
		dbseLoadCP(cp) ;
		cp = sdmGetNextCP();
	}
	printf("OK") ;

	printf(" Facs:") ;
	FACILITY* facility = sdmGetNextFACILITY();
	while (facility)
	{
		dbseLoadFacility(facility) ;
		facility = sdmGetNextFACILITY();
	}
	printf("OK") ;

	printf(" Bldgs:") ;
	BUILDING* building = sdmGetNextBUILDING();
	while(building)
	{
		dbseLoadBuilding(building) ;
		building = sdmGetNextBUILDING();
	}
	printf("OK") ;

	printf(" Lnks:") ;
	DEPOT_LINK* link = sdmGetNextDEPOT_LINK();
	while(link)
	{
		if ( !dbseLoadDepotLink(link) )
		{
			fprintf(stderr, "unable to find cp for link %s\n", link->name) ;
		}
		link = sdmGetNextDEPOT_LINK();
	}
	printf("OK\r") ;

	sdmCloseFile();

	return retval;
}

void
_loadIndex(const char* type
			, const char* optField1
			, const char* optField2
			, void*(*handler)(UINT32 uOID, SINT32 ox, SINT32 oy, SINT32 id, const char* name, WWII_COUNTRY c1, WWII_COUNTRY c2, SINT32 posx, SINT32 posy)
		  )
{
	try
	{
		DBQuery q(dbh
					, "SELECT i.%s_oid, i.%s_ox, i.%s_oy, i.%s_id, i.name, %s, %s, absx, absy"
						" FROM ids_%s i LEFT JOIN strat_%s USING (%s_oid)"
						" ORDER BY %s_oid ASC"
						, type, type, type, type, optField1, optField2
						, type, type, type
						, type
					) ;

		printf("Pre-load %d %ss: ", q.Rows(), type) ;

		if ( q.Rows() == 0 )
			return ;

		UINT32 oid ;
		SINT32 ox, oy, id, posx, posy ;
		const char* name ;
		WWII_COUNTRY c1, c2 ;

		while (q.FetchRow())
		{
			DBRow& row = q.Row() ;
			oid = row.NextUInt() ;
			ox = row.NextSInt() ;
			oy = row.NextSInt() ;
			id = row.NextUInt() ;
			name = row.NextColumn() ;
			c1 = (WWII_COUNTRY)row.NextUInt() ;
			c2 = (WWII_COUNTRY)row.NextUInt() ;
			posx = row.NextSInt() ;
			posy = row.NextSInt() ;

			handler(oid, ox, oy, id, name, c1, c2, posx, posy) ;
		}

		printf("OK\r\n") ;
	}
	catch (std::exception& e) { fprintf(stderr, "%s: %s\n", __FUNCTION__, e.what()) ; }
}

void
_loadIndexes()
{
	_loadIndex("cp", "country", "concountry", _newCPW) ;
	_loadIndex("facility", "country", "0", _newFacW) ;
}

SINT32
dbseCloseLoader()
{
	dbh.disconnect() ;

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Initialize the strat loader
SINT32
dbseInitStratLoader()
{
	// Please don't ask why... There was a good reason for this, though. -OGS
	dbh.disconnect() ;
	if ( dbh.connect(dbConnection) == false )
		return -1 ;

	// Because we are wielding Latin1 characters for now,
	// we need to tell the database this so that it can
	// go ahead and convert the names to UTF8
//	DBSimpleQuery(dbh.conn[0], "charset latin1") ;
//	DBSimpleQuery(dbh.conn[0], "set names latin1") ;

	_loadIndexes() ;

	return 0;
}

