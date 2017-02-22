#include <syscfg.h> // Path is project specific.
//

#include <math.h>
#include "pi/pi.h"
#include "include/vars.h"
#include "mem/mem.h"
#include "fio/fio.h"
#include "trn/trnmain.h"
#include "trn/trnwtr.h"
#include "trn/trncd.h"
#include "trn/trntod.h"
#include "ob/obc.h"
#include "ob/obmain.h"
#include "ob/obdlite.h"
#include "ob/obdraw.h"
#include "ob/obcs.h"
#include "ob/obpool.h"
#include "trn/trnsky.h"
#include "trn/trnop.h"
#include "trn/trnldr.h"
#include "trn/trntypes.h"
#include "ob/obslite.h"
#include "ld/ldmdl.h"
#include "ld/ldpix.h"
#include "math/math3d.h"
#include "math/mathvect.h"
#include "draw/draw3d.h"
#include "rn/rnmain.h"
#include "ogrp/ogrptype.h"
#include "locl/loclMain.h"
#include "comm/commClientMessages.h"
#include "rn/rnvis.h"
#include "trn/trnautogun.h"
#include "str/strpool.h"
#include "fx/fxBlt.h"
#include "fx/fxHit.h"
#include "trn/trngrass.h"
#include "trn/trncloud.h"
#include "trn/trndata.h"
#include "ld/lddds.h"
#include "ob/obMgr.h"
#include "errno.h"
#include "trn/speedgrass.h"
#include "trn/trnRadClutter.h"
#include "sdm/StratDB.h"

#define GLOBALS
#include "trn/trnvars.h"
#undef GLOBALS

#ifdef SPATIAL_DB
#include "SpatialDB.h"
#endif

#include "wwii/wwiiCovert.h"

static Covert<LOGICAL> g_trnmainDraw = TRUE;
static Covert<LOGICAL> g_drawOctetTriangles = FALSE;

static std::map<int, std::vector<piVECTOR> > ShadowOctet;
static std::map<int, OBJECT*> ShadowObject;

static int mapindex = 0;

// typedefs
struct _CHAOTICSEEDTYPE
{
	FLOAT64 X;
	FLOAT64 Y;
	FLOAT64 Z;

	FLOAT64 dt;
	FLOAT64 A;
	FLOAT64 B;
	FLOAT64 C;
};


// defines
#define MAX_IMPORT_OBJECTS	32

// Time for sun to travel in the sky 1 degree
#define UPDATE_TOD_DELTA	(4 * 60)

// Movement hysteresis based on portion of Supercell size
#define TERRAIN_HYSTERESIS		0.1f


// static variables
static SINT16				hystscx, hystscy;

static LOGICAL				updated, LightingUpdate;

static FLOAT64				InitXOrigin, InitYOrigin;
static LOGICAL				_TerrainMoved;		// flag to state that the origin offset needs to move
static LOGICAL				_WorldMoved;
static piVECTOR				_CameraAdjustment;	// used to save an origin offset jump to next frame
static _CHAOTICSEEDTYPE		_Seed;
static _CHAOTICSEEDTYPE		_CurrentSeed;
static OBJECT*				_ImportObjectList[MAX_IMPORT_OBJECTS];
static SINT32				_CurrentImportObjectCount;
static SINT32				NumDrawnTriangles;

// static prototypes
static LOGICAL RawUpdate(LOGICAL ForceCompleteReLoad);
LOGICAL trnmainFindSupercell(SINT16* scx, SINT16* scy, piVECTOR* p);
static LOGICAL trnmainAdjustTerrainSections(piVECTOR* CameraAdjustment);

// Removing this causes undeclared error in BETA_RELEASE
#ifdef BETA_RELEASE
extern UINT32 OverrideSpawner;
#endif

// Static functions
static SINT32 _GetChaoticNumber()
{
FLOAT64		X,Y,Z;
SINT16		ReturnNum;

	X = _CurrentSeed.X;
	Y = _CurrentSeed.Y;
	Z = _CurrentSeed.Z;

	_CurrentSeed.X += (-_CurrentSeed.A * X * _CurrentSeed.dt) +
					  ( _CurrentSeed.A * Y * _CurrentSeed.dt);
	_CurrentSeed.Y += ( _CurrentSeed.B * X * _CurrentSeed.dt) -
					  ( _CurrentSeed.dt * Y) -
					  ( _CurrentSeed.dt * Z * X);
	_CurrentSeed.Z += (-_CurrentSeed.C * Z * _CurrentSeed.dt) +
					  ( _CurrentSeed.dt * X * Y);

	ReturnNum = (SINT16)*(SINT16*)((UCHAR*)&_CurrentSeed.Y + 3);

	return ((SINT32)ReturnNum);
}

static LOGICAL _SeedChaoticNumberGenerator()
{
_CHAOTICSEEDTYPE Seed;
SINT32	i;
SINT32	Temp;
UINT16	TempNum;

	Seed.X = Seed.Y = Seed.Z = 1.0;
	Seed.A = 5;
	Seed.B = 15;
	Seed.C = 1;
	Seed.dt = 0.02;

	TempNum = (UINT16)frametime;
	*(UINT16*)((UCHAR*)&Seed.Y + 3) = TempNum;

	_CurrentSeed = Seed;

	for (i=0;i<(frametime & 0xFF) + ((frametime & 0xFF00) >> 8);i++)
	{
		Temp = _GetChaoticNumber();

	}

	return TRUE;
}

static UINT32 _GetChaoticNumberGeneratorSeed(_CHAOTICSEEDTYPE * Seed)
{
	if (Seed == nullptr)
	{
		return FALSE;
	}

	*Seed = _CurrentSeed;

	return TRUE;
}

LOGICAL trnmainSetObjectCreationCallback(TRN_OBJECT_NOTIFY callback)
{
	creation = callback;

	return(TRUE);
}

LOGICAL trnmainSetObjectDeletionCallback(TRN_OBJECT_NOTIFY callback)
{
	deletion = callback;

	return(TRUE);
}

LOGICAL trnmainGetMapName(SINT8* map)
{
	strcpy(map, mapname);

	return(TRUE);
}

LOGICAL trnmainSetTerrainMatrix(OBJECT* lpobj)
{
	if (lpobj == nullptr)
	{
		return(FALSE);
	}

	lpobj->mat = IdentityMatrix;
	lpobj->mat._41 = -((FLOAT32)WorldOriginOffset.x - lpobj->rot.x);
	lpobj->mat._42 = -((FLOAT32)WorldOriginOffset.y - lpobj->rot.y);
	lpobj->mat._43 = -((FLOAT32)WorldOriginOffset.z - lpobj->rot.z);

	return(TRUE);
}

TERRAIN_DETAIL* trnGetTerainDetail(SINT16 scx, SINT16 scy)
{
SINT32 i;

	for (i=0; i < NUM_TERRAIN_RECTANGLES; i++)
	{
		if (activeTerrainDetail[i] &&
			trndtl[i].scx == scx &&
			trndtl[i].scy == scy)
		{
 //           if(trndtl[i].trnstate.state != TRN_STATE_WAIT)
			if (trndtl[i].stratLoaded == FALSE)
			{
                return nullptr;
            }

			return &trndtl[i];
		}
	}

	return nullptr;
}

LOGICAL trnGetStratData(SINT16 scx, SINT16 scy, SINT32 id, SINT32* country, SINT32* level)
{
	FULL_OBJ_ID fid;

	fid.cell.ox		= scx << 4;
	fid.cell.oy		= scy << 4;
	fid.id			= id;
	fid.objClass	= 0;
	fid.objType		= 0;

	LOGICAL result = FALSE;

	StratDB::StratObject* stratObj = StratDB::FindStratObject(fid);
	if(!stratObj) return result;

	if(country && *country == -1)
	{
		*country = stratObj->Country();

		if(stratObj->StratClass() == SDM_CLASS_BUILDING)
		{
			StratDB::StratObject* fac = stratObj->ParentStratObject();
			if(fac) *country = fac->Country();
		}

		result = TRUE;
	}

	if(level && *level == -1)
	{
		*level = stratObj->VisLevel();
		result = TRUE;
	}

	return result;
}

SINT32 trnSetStratData(TERRAIN_DETAIL* td, SINT32 bid, SINT32 country, SINT32 level) {
	if ( country == -1 || level == -1 )
		trnGetStratData(td->scx, td->scy, bid, &country, &level);

	if ( 0 > bid || bid >= td->bldHash.maxBldID ) {
		Write_Log(LOG_COMM, LOG_INFO, "trnSetStratData: Bad bid %d (>= %d)", bid, td->bldHash.maxBldID);
		return -EINVAL;
	}

	if ( ! td->bldHash.building ) return -1;

	if ( country >= 0)
	{
		td->bldHash.building[bid].owner = country;

		if (td->bldHash.building[bid].isFacility)
		{
			SINT32 i;
			STRAT_BUILDING_LIST* blist;
			SINT32 numBuildings;

			blist = sdmGetFaciltyBidList(td->facTable, (SINT16)bid);

			if (blist == nullptr)
			{
#ifdef BETA_RELEASE
				rnmainListText("Invalid building message");
#endif
				Write_Debug("Invalid building message in recvWorldUpdate, "
				            "supercell [%d,%d] bid = %d", td->scx,td->scy, bid);
				return -EINVAL;
			}

			numBuildings = blist->numBuildings;

			// update buildings
			for(i=0;i<numBuildings;i++)
				trnSetStratData(td, blist->bid[i], country, -1);
		}
	}

	if(level > -1)
		td->bldHash.building[bid].level = level;

	if (td->bldHash.building[bid].loaded)
		trnUpdateObjectStatus(td->scx, td->scy, bid, country, level);

	return 0;
}

SINT32 trnUpdateStratData(TERRAIN_DETAIL* td, OBJECT* lpobj, SINT32 bid, LOGICAL loaded)
{
	if ( bid == 0 ) // TODO: Why can't it be zero?
		return -EINVAL;
	if ( td == nullptr || td->bldHash.building == nullptr )
		return -EINVAL;

	if ( bid < 0 ) {
		if ( lpobj == nullptr || loaded == TRUE )
			return -EINVAL;

		for ( int i = 0 ; i < td->bldHash.maxBldID ; ++i ) {
			if ( td->bldHash.building[i].pobj != lpobj )
				continue;
			bid = i;
			break;
		}
	}

	if ( bid < 0 ) {
		Write_Log(LOG_COMM, LOG_INFO, "trnUpdateStratData: Bad bid %d (<0) - model type %d - model name %s",
		          bid, lpobj->modelInstance == nullptr ? -1 : lpobj->modelInstance->GetModelType(),
		          lpobj->name);
		return -EINVAL;
	}

	if ( bid >= td->bldHash.maxBldID ) {
		Write_Log(LOG_COMM, LOG_INFO, "trnUpdateStratData: Bad bid %d (>= %d)", bid, td->bldHash.maxBldID);
		return -EINVAL;
	}


	td->bldHash.building[bid].loaded = loaded;

	if ( loaded && td->bldHash.building[bid].isBuilder )
		trnSetBuilderFlag(td->scx, td->scy, bid);

	if ( loaded ) {
		trnUpdateObjectStatus(td->scx, td->scy, bid);
//		trnUpdateObjectStatus(td->scx,td->scy,bid,-1,td->bldHash.building[bid].level);
		td->bldHash.building[bid].pobj = lpobj;
	} else
		td->bldHash.building[bid].pobj = nullptr;

	return 0;
}

LOGICAL trnmainInit(SINT8* name, FLOAT32 xorigin, FLOAT32 yorigin)
{
//char	line[256];
int		i, j;

    trndataInit();

	creation = nullptr;
	deletion = nullptr;

	hystscx = SUPERCELL_INVALID;
	hystscy = SUPERCELL_INVALID;

	strcpy(mapname, name);

	_SeedChaoticNumberGenerator();
	_GetChaoticNumberGeneratorSeed(&_Seed);

	if (!trnldrInit())
	{
		return(FALSE);
	}

	trncdInit();

	InitXOrigin = xorigin / WORLD_ARC_WIDTH * WORLD_WIDTH;
	InitYOrigin = yorigin / WORLD_ARC_HEIGHT * WORLD_HEIGHT;

	WorldOriginOffset.x = InitXOrigin;
	WorldOriginOffset.y = InitYOrigin;
	WorldOriginOffset.z = 0;

	TerrainStable = TRUE;

	// New square terrain
	xstep = 800.0f * ScaleFactor;
	ystep = 800.0f * ScaleFactor;

	// 2:1 scale
	xstep *= 0.5f;
	ystep *= 0.5f;

	Write_Debug("Terrain square dimensions: width = %f, height = %f", xstep / ScaleFactor, ystep / ScaleFactor);

	trnlod = 0;

	horizon = 8000.0f * ScaleFactor;
	Write_Debug("Terrain minimal horizons: %f", horizon / ScaleFactor);

	for (i=0; i < NUM_TERRAIN_RECTANGLES; i++)
	{
#ifdef SPATIAL_DB
//        trndtl[i].sdbInfo = nullptr;
//        trndtl[i].sdbDatabase = nullptr;
#endif
		trndtl[i].scx = SUPERCELL_INVALID;
		trndtl[i].scy = SUPERCELL_INVALID;
		if (!obmainCreateObject(&trndtl[i].terrain, "", MAX_TERRAIN_VERTICES, MAX_TERRAIN_TRIANGLES))
		{
			return(FALSE);
		}

		trndtl[i].terrain->flags = OBJECT_COLLISION | OBJECT_USEMATRIX;
		strprintf(trndtl[i].terrain->name, "%s_r%d", name, i);
//		lstrcpy(trndtl[i].terrain->name, name);
//		strprintf(line, "_r%d", i);
//		lstrcat(trndtl[i].terrain->name, line);
		trndtl[i].terrain->texid	= -1;
		trndtl[i].terrain->trnum = i;
		trndtl[i].terrain->layer = 2;
		trnmainSetTerrainMatrix(trndtl[i].terrain);

		trndtl[i].numheightmapelements = 0;
		trndtl[i].nmcnt = 0;
		trndtl[i].tricnt = 0;
		trndtl[i].triname[0] = (SCHAR*)memAlloc(32 * MAX_TERRAIN_TEXTURES);
		for (j=1; j < MAX_TERRAIN_TEXTURES; j++)
		{
			trndtl[i].triname[j] = trndtl[i].triname[j-1] + 32;
		}

		trndtl[i].trindx = 0;
		trndtl[i].tobndx = 0;
	}

	if (!trnskyInit("sky", NUM_SKY_MESHES, horizon / ScaleFactor))
	{
		return(FALSE);
	}

	if (!trnwtrInit("water"))
	{
		return(FALSE);
	}

	if (!RawUpdate(FALSE))
	{
		return(FALSE);
	}

	trnTODInit();

    // If this fails, should display old grass
    radclutterInit();

	return(TRUE);
}

LOGICAL trnmainGetDimensions(FLOAT64* left, FLOAT64* top, FLOAT64* width, FLOAT64* height)
{
	*left	= (hdr.ulxmap / WORLD_ARC_WIDTH * WORLD_WIDTH);
	*top	= (hdr.ulymap / WORLD_ARC_HEIGHT * WORLD_HEIGHT);
	*width	= (FLOAT64)((hdr.ncols - 1) / SUPERCELL_SIZE_X1) * SUPERCELL_WIDTH;
	*height	= (FLOAT64)((hdr.nrows - 1) / SUPERCELL_SIZE_Y1) * SUPERCELL_HEIGHT;

	return(TRUE);
}

LOGICAL trnmainGetHorizon(FLOAT32* rethorizon)
{
	*rethorizon = horizon;

	return(TRUE);
}

LOGICAL trnmainFindSupercell(SINT16* scx, SINT16* scy, piVECTOR* p)
{
SINT16		lscx, lscy;
FLOAT32		x, y, left, top, right, bottom;

	if (p == nullptr)
	{
		return(FALSE);
	}

	y = p->y;
	if (y < 0.0f)
	{
		y -= SUPERCELL_HEIGHT;
	}

	lscy = (SINT16)(y / SUPERCELL_HEIGHT);

	x = p->x;
	if (x < 0.0f)
	{
		x -= SUPERCELL_WIDTH;
	}

	lscx = (SINT16)(x / SUPERCELL_WIDTH);

	if (lscx != hystscx || lscy != hystscy)
	{
		if (hystscx != SUPERCELL_INVALID &&
			hystscy != SUPERCELL_INVALID)
		{
			left = hystscx * SUPERCELL_WIDTH;
			right = left + SUPERCELL_WIDTH;
			bottom = hystscy * SUPERCELL_HEIGHT;
			top = bottom + SUPERCELL_HEIGHT;
			left -= SUPERCELL_WIDTH * TERRAIN_HYSTERESIS;
			right += SUPERCELL_WIDTH * TERRAIN_HYSTERESIS;
			bottom -= SUPERCELL_HEIGHT * TERRAIN_HYSTERESIS;
			top += SUPERCELL_HEIGHT * TERRAIN_HYSTERESIS;
			if (p->x >= left &&
				p->x <= right &&
				p->y >= bottom &&
				p->y <= top)
			{
				lscx = hystscx;
				lscy = hystscy;
			}
		}

		hystscx = lscx;
		hystscy = lscy;
	}

	*scx = lscx;
	*scy = lscy;

	return(TRUE);
}

LOGICAL trnmainAddAltTexture(OBJECT** retobj, OBJECT* lpobj, SINT32 octet, const SINT8* texname)
{
	LOGICAL		load=FALSE;
	static SINT8		filename[_MAX_PATH];
	static SINT8		objname[64];
	SINT32		i, texid, tonum;
	OBJECT*	newobj=nullptr;
	TERRAIN_DETAIL*	td;

	td = &trndtl[static_cast<int>(lpobj->trnum)];
	if (strcmp(texname, "null1.bmp") == 0)
	{
		Write_Debug("Null texture object was used");
	}

	for (i=0; i < MAX_TERRAIN_TEXTURES; i++)
	{
		if (td->nonreplacementtiles[i] != nullptr)
		{
			if (strcmp(td->nonreplacementtiles[i]->texname, texname) == 0)
			{
				newobj = td->nonreplacementtiles[i];
				break;
			}
		}
	}

	if (i == MAX_TERRAIN_TEXTURES)
	{
		for (i=0; i < MAX_TERRAIN_TEXTURES; i++)
		{
			if (td->nonreplacementtiles[i] != nullptr)
			{
				if (td->nonreplacementtiles[i]->numtris == 0)
				{
					load = TRUE;
					newobj = td->nonreplacementtiles[i];
					break;
				}
			}
		}
	}

	if (newobj == nullptr)
	{
		for (i=0; i < MAX_TERRAIN_TEXTURES; i++)
		{
			if (td->nonreplacementtiles[i] == nullptr)
			{
				strprintf(objname, "%s_t%d", td->terrain->name, i);
				if (!obmainCreateObject(&newobj, objname, MAX_TERRAIN_VERTICES, MAX_TERRAIN_TRIANGLES))
				{
					return(FALSE);
				}

				load = TRUE;
				td->nonreplacementtiles[i] = newobj;
				break;
			}

			if (td->nonreplacementtiles[i]->numtris == 0)
			{
				newobj = td->nonreplacementtiles[i];
				strprintf(newobj->name, "%s_t%d", td->terrain->name, i);
				break;
			}
		}

		if (i == MAX_TERRAIN_TEXTURES)
		{
			Write_Debug("No room for terrain texture!");
			return(FALSE);
		}

		newobj->layer = 2;

		newobj->rot = td->terrain->rot;
		trnmainSetTerrainMatrix(newobj);

		newobj->numverts = 0;
		newobj->numtris = 0;

		newobj->flags |= OBJECT_USEMATRIX
						|OBJECT_LITE
						|OBJECT_DYNALITE
						|OBJECT_VISIBLE
						|OBJECT_INDEXED
						|OBJECT_TRIANGLELIST
						|OBJECT_COLLISION
						|OBJECT_TERRAIN
//						|OBJECT_NOSPECULAR
						;

        // Remove object from standard rendering system
        obmainRemoveObjectTree(newobj);

		newobj->trnum = lpobj->trnum;
	}

	if (load)
	{
		newobj->texname = strpool(texname);

		strcpy(filename, TERRAIN_PREPATH);
		strcpy(filename + sizeof(TERRAIN_PREPATH)-1, newobj->texname);
		texid = ldpixLoadImageFileToSurface(filename, colorBLACK, LDPIX_MIPMAP);
		if (!piIsTexture(texid, 0))
		{
			Write_Debug("Could not load file as a texture");
			obmainFreeObject(newobj);
			return(FALSE);
		}

		newobj->texid = texid;

		// Added materials to objects based on texture names

		// Copy texture name sans extension
		SINT32 i, lastDot=-1;
		for(i = 0; texname[i] != 0; ++i)
		{
			filename[i] = texname[i];
			if(texname[i] == '.')
				lastDot = i;
		}
		if(lastDot != -1)
			filename[lastDot] = 0;
		else
			filename[i] = 0;

		Write_Debug("Material = %s", filename);
		newobj->matid = piFindMaterialName(filename);
		if (piIsMaterial(newobj->matid))
		{
			newobj->matname = strpool(filename);
		}
		else
		{
			newobj->matid = piFindMaterialName(TERRAIN_DEFAULT_MATERIAL);
			newobj->matname = TERRAIN_DEFAULT_MATERIAL;
		}

		newobj->flags |= OBJECT_VISIBLE;
	}

	tonum = 0;//i + 1;

	trnmainAddToObject(newobj, lpobj, octet, tonum, 0, 0, 0, rand() & 0x03);

#ifndef USE_3DAPI_LIGHTING
	if (!(newobj->flags & OBJECT_DYNALITE))
	{
	int         numlights;
	LIGHT*     lplite;

		numlights = piGetLights(&lplite);
		if (!obsliteCalculate(1, newobj, numlights, lplite, piGetAmbLight()))
		{
			return(FALSE);
		}
	}
#endif

	*retobj = newobj;

	return(TRUE);
}

LOGICAL trnmainAddToObject(OBJECT* newobj, OBJECT* lpobj, SINT32 octet, SINT32 tonum, SINT32 drag, SINT32 roughness, SINT32 style, SINT32 orientation)
{
SINT32				i, ofs, tri;
SINT32				nv, nt;
piINDICES*			pt;
TERRAIN_DETAIL*	td;

	if (newobj == nullptr ||
		lpobj == nullptr)
	{
		return(FALSE);
	}

#ifdef DEBUG_WW2
	if (strcmp(newobj->texname, lpobj->texname) == 0)
	{
		Write_Debug("Source and destination texture objects are the same: '%s'", newobj->texname);
#ifdef DONT_USE
		return(TRUE);
#endif
	}
	else if (strcmp(newobj->texname, "null1.bmp") == 0 && strlen(lpobj->texname) > 0)
	{
		Write_Debug("Null texture object was destination of source %s", lpobj->texname);
	}
	else if (strcmp(lpobj->texname, "null1.bmp") == 0)
	{
		Write_Debug("Null texture object was source");
	}
#endif

	if (!loading)
	{
		td = &trndtl[static_cast<int>(lpobj->trnum)];
		if (td->trindx < 0 || td->trindx >= MAX_TERRAIN_EDIT_LIST)
		{
			Write_Debug("Out of edit buffer space.");
			return(FALSE);
		}
	}

	// Just incase a rogue object gets through; should never happen
	if (lpobj->numtris == 0)
	{
		Write_Debug("Terrain error: Old object (%s) should be disabled (%s)", lpobj->name, lpobj->texname);
		return(TRUE);
	}

	tri = octet * NUM_TRIANGLES_PER_OCTET;
	if (tri >= lpobj->numtris)
	{
		Write_Debug("Number of source triangle (%d) was higher than is available (%d)", tri, lpobj->numtris);
		return(FALSE);
	}

	ofs = lpobj->ptris[tri].v3;
	if (ofs >= lpobj->numverts)
	{
		Write_Debug("Number of source vertex (%d) was higher than is available (%d)", ofs, lpobj->numverts);
		return(FALSE);
	}

	nv = newobj->numverts;
	if (nv >= (NUM_VERTICES_PER_OCTET * SUPERCELL_OCTETS_XY))
	{
		Write_Debug("Error in number of vertices in destination texture object");
		return(FALSE);
	}

	memcpy(&newobj->puverts[nv], &lpobj->puverts[ofs],	NUM_VERTICES_PER_OCTET * sizeof(piUNIFIED_VERTEX));

	piVertexColorPtr sc = newobj->StaticColors();

	for (i=0; i < NUM_VERTICES_PER_OCTET; i++)
	{
		sc[nv+i].color	= colorWHITE|colorALPHA;
		sc[nv+i].specular	= colorALPHA;
	}

	nt = newobj->numtris;
	if (nt >= (NUM_TRIANGLES_PER_OCTET * SUPERCELL_OCTETS_XY))
	{
		Write_Debug("Error in number of triangles in destination texture object");
		return(FALSE);
	}

	memcpy(&newobj->psnorig[nt],	&lpobj->psnorig[tri],		NUM_TRIANGLES_PER_OCTET * sizeof(piVECTOR));
	memcpy(&newobj->psnorms[nt],	&lpobj->psnorms[tri],		NUM_TRIANGLES_PER_OCTET * sizeof(piVECTOR));
	memcpy(&newobj->pscdon[nt * 3],	&lpobj->pscdon[tri * 3],	NUM_TRIANGLES_PER_OCTET * 3 * sizeof(piVECTOR));
	memcpy(&newobj->pscdn[nt * 3],	&lpobj->pscdn[tri * 3],		NUM_TRIANGLES_PER_OCTET * 3 * sizeof(piVECTOR));

	pt = newobj->ptris;

	pt[nt + 0].v1 = nv + 1;
	pt[nt + 0].v2 = nv + 2;
	pt[nt + 0].v3 = nv + 0;

	pt[nt + 1].v1 = nv + 2;
	pt[nt + 1].v2 = nv + 3;
	pt[nt + 1].v3 = nv + 0;

	pt[nt + 2].v1 = nv + 3;
	pt[nt + 2].v2 = nv + 4;
	pt[nt + 2].v3 = nv + 0;

	pt[nt + 3].v1 = nv + 4;
	pt[nt + 3].v2 = nv + 5;
	pt[nt + 3].v3 = nv + 0;

	pt[nt + 4].v1 = nv + 5;
	pt[nt + 4].v2 = nv + 6;
	pt[nt + 4].v3 = nv + 0;

	pt[nt + 5].v1 = nv + 6;
	pt[nt + 5].v2 = nv + 7;
	pt[nt + 5].v3 = nv + 0;

	pt[nt + 6].v1 = nv + 7;
	pt[nt + 6].v2 = nv + 8;
	pt[nt + 6].v3 = nv + 0;

	pt[nt + 7].v1 = nv + 8;
	pt[nt + 7].v2 = nv + 1;
	pt[nt + 7].v3 = nv + 0;

	trnldrSetTextureRotation(newobj, orientation, nv / NUM_VERTICES_PER_OCTET);

	nt += NUM_TRIANGLES_PER_OCTET;
	newobj->numtris = nt;

	nv += NUM_VERTICES_PER_OCTET;
	newobj->numverts = nv;

	newobj->SyncMutableData();

	if (!loading)
	{
		if (!ldmdlGetBounds(newobj))
		{
			return(FALSE);
		}
	}

	if ((lpobj->numtris - NUM_TRIANGLES_PER_OCTET) > tri)
	{
		nt = lpobj->numtris - NUM_TRIANGLES_PER_OCTET;
		for (i=tri; i < nt; i++)
		{
			lpobj->ptris[i].v1 = lpobj->ptris[i + NUM_TRIANGLES_PER_OCTET].v1 - NUM_VERTICES_PER_OCTET;
			lpobj->ptris[i].v2 = lpobj->ptris[i + NUM_TRIANGLES_PER_OCTET].v2 - NUM_VERTICES_PER_OCTET;
			lpobj->ptris[i].v3 = lpobj->ptris[i + NUM_TRIANGLES_PER_OCTET].v3 - NUM_VERTICES_PER_OCTET;
		}

		nt = lpobj->numtris - tri - NUM_TRIANGLES_PER_OCTET;
		memmove(&lpobj->psnorig[tri],	&lpobj->psnorig[tri + NUM_TRIANGLES_PER_OCTET],			nt * sizeof(piVECTOR));
		memmove(&lpobj->psnorms[tri],	&lpobj->psnorms[tri + NUM_TRIANGLES_PER_OCTET],			nt * sizeof(piVECTOR));
		memmove(&lpobj->pscdon[tri * 3],	&lpobj->pscdon[tri * 3 + 3 * NUM_TRIANGLES_PER_OCTET],	nt * 3 * sizeof(piVECTOR));
		memmove(&lpobj->pscdn[tri * 3],	&lpobj->pscdn[tri * 3 + 3 * NUM_TRIANGLES_PER_OCTET],	nt * 3 * sizeof(piVECTOR));

		nv = lpobj->numverts - ofs - NUM_VERTICES_PER_OCTET;

		memmove(&lpobj->puverts[ofs],	&lpobj->puverts[ofs + NUM_VERTICES_PER_OCTET],	nv * sizeof(piUNIFIED_VERTEX));
		lpobj->SyncMutableData();
	}
	else
	{
		Write_Debug("No triangles were relocated in the source object: %s (%s)", lpobj->name, lpobj->texname);
	}

	newobj->trnum = lpobj->trnum;

	lpobj->numverts -= NUM_VERTICES_PER_OCTET;
	lpobj->numtris -= NUM_TRIANGLES_PER_OCTET;

	if (lpobj->numtris == 0)
	{
		if (!loading)
		{
			Write_Debug("Old texture object (%s) was disabled (%s)", lpobj->name, lpobj->texname);
			lpobj->numverts		= 0;
			lpobj->flags		&= ~OBJECT_VISIBLE;
			lpobj->texname = "";
			piFreeMipTextures(lpobj->texid);
			lpobj->texid		= -1;
		}
		else
		{
			Write_Debug("Old object (%s) should be disabled (%s); no triangles", lpobj->name, lpobj->texname);
		}
	}
	else
	if (lpobj->numtris == 0xffff)
	{
		// Should never happen
		Write_Debug("Terrain error: Negative value for triangles on old object");
	}
	else
	{
		if (!loading)
		{
			if (!ldmdlGetBounds(lpobj))
			{
				return(FALSE);
			}
		}
	}

	if (!loading)
	{
		td = &trndtl[static_cast<int>(newobj->trnum)];
		for (i=0; i < td->tricnt; i++)
		{
			if (strcmp(td->triname[i], newobj->texname) == 0)
			{
				break;
			}
		}

		if (i == td->tricnt)
		{
			strcpy(td->triname[i], newobj->texname);
			td->tricnt += 1;
		}

		if (td->trindx < 0 || td->trindx >= MAX_TERRAIN_EDIT_LIST)
		{
			// Should never happen
			Write_Debug("Terrain error: edit list has an illegal number of entries");
		}
		else
		{
			ofs = (newobj->numtris - NUM_TRIANGLES_PER_OCTET) / NUM_TRIANGLES_PER_OCTET;
			td->trinfo[td->trindx].octet	= ofs;						// octet number
			td->trinfo[td->trindx].octnum	= octet;					// original octet number
			td->trinfo[td->trindx].tonum	= tonum;					// original triangle number
			td->trinfo[td->trindx].txndx	= i;						// texture name index
			td->trinfo[td->trindx].ortdrg	= orientation | drag << 4;	// orientation/drag LSN/MSN
			td->trinfo[td->trindx].ruftrn	= roughness | style << 4;	// roughness/terrain LSN/MSN
			td->trindx += 1;
		}
	}

	return(TRUE);
}

#if(0)
static LOGICAL GetHeight(SINT32* retd, SINT16 d)
{
	if (d == TERRAIN_NODATA)
	{
		*retd = 0;
	}
	else
	{
		*retd = (SINT32)(-(FLOAT32)d * 2.0f * ScaleFactor);
	}

	return(TRUE);
}
#endif
#if(0)
static LOGICAL SetHeight(SINT16* retd, SINT32 d)
{
	if (d == 0)
	{
		*retd = TERRAIN_NODATA;
	}
	else
	{
		*retd = (SINT16)(-(FLOAT32)d / 2.0f / ScaleFactor);
	}

	return(TRUE);
}
#endif

LOGICAL trnmainAddObject(const char* objname, piVECTOR* pos, OBJECT* lpmain,
                         SINT32 StratSuperCellX, SINT32 StratSuperCellY, UINT32 StratId,
                         REL_OBJ_ID * ObjId,
                         SINT32 drag, SINT32 roughness,
                         SINT32 style, SINT32 orientation, SINT32 sc, bool nonPlanarBase,
                         const SDM_ObjInfo& objInfo, TERRAIN_DETAIL* terrainDetail,
                         const char** objRefName) {
	assert ( loading && ! roughness && ! style );

	char newname[_MAX_PATH];
	lstrcpy(newname, TERRAIN_PREPATH);
	lstrcpy(newname + sizeof(TERRAIN_PREPATH) - 1, objname);

	PoolModel* poolModel = obmgrGetPoolModel(objInfo.objname);
	assert ( poolModel );

	// Ticket# 4583. Set for correct DTL selection for tree placements
	extern void trnopSetSC(int sc);
	trnopSetSC(sc);

	if ( poolModel->IsStaticTerrainModel() ) {
		FLOAT64VECTOR WorldOriginOffset;
		trnGetWorldOriginOffset(&WorldOriginOffset);

		piMATRIX worldTransform = objInfo.transform;
		worldTransform._41 -= (float)WorldOriginOffset.x;
		worldTransform._42 -= (float)WorldOriginOffset.y;
		worldTransform._43 -= (float)WorldOriginOffset.z;

		obmgrCreateModelInstance(newname, worldTransform, true);
	} else if ( ! obmgrCreateModelInstance(newname) ) {
		return FALSE;
	}

	const char* ptr = strrchr(objname, '\\');
	if (ptr != nullptr)
		strsafecpy(newname, ++ptr);
	else
		strsafecpy(newname, objname);

	OBJECT* masterObj, *newobj;
	masterObj = newobj = obmainFindTopmostObject(newname);
	assert(masterObj);

	UINT32 flags = OBJECT_TERRAIN_OBJECT;

	if ( orientation != 0 )
		flags |= OBJECT_IS_ROTATED;
	if ( StratId != 0 )
		flags |= OBJECT_STRAT_OBJECT;

	if ( ! obmainSetFlag2InObjectTree(newobj, flags) )
		return FALSE;

	piVECTOR newatt;
	newatt.x = newatt.y = 0.0f;
	newatt.z = (FLOAT32)orientation * (PI * 0.25f);
	newobj->att = newatt;

	newobj->trnum = sc;

	if ( poolModel->IsStaticTerrainModel() ) {
		obmgrHandleSpecialObjects(newobj);

		if ( poolModel->hastrees )
			trnopHandleSpeedTree(newobj->child);
	}

	if ( ! poolModel->IsStaticTerrainModel() ) {
		// translate the object to its new position
		if ( ! trnopOrientSubobjects(newobj, pos, nonPlanarBase) ) {
			obpoolFreeObjectCallback(newobj);
			return FALSE;
		}
	}

	strprintf(newname, "%s_%d_%d", newname, (int)pos->x, (int)pos->y);
	assert ( strlen(newname) < sizeof(newobj->name) );
	strcpy(newobj->name, newname);
	*objRefName = newobj->name;

	//Write_Debug("Object renamed: %s", newname);

	// temporary hack for the terrain collision detection routines

	// In trnmainAddObject
	//if (1)
	{
		trncdAddTerrainObject(newobj, StratSuperCellX, StratSuperCellY, StratId, ObjId);
		TERRAIN_DETAIL* td = trnGetTerainDetail((SINT16)StratSuperCellX, (SINT16)StratSuperCellY);
		trnUpdateStratData(td, newobj, StratId, TRUE);

#ifdef SPATIAL_DB
        if ( (newobj->flag3 & (OBJECT_CLOUDS|OBJECT_RAINCLOUDS)) == 0 && (strstr( objname, "SPWN" ) == nullptr) ) // need way to determine SPWN objects
        {
            SDB_Box2D b;
            FLOAT32 avgz = 0.0;
            assert(newobj->child);
            size_t counter = 0;

            OBJECT* topobj = newobj->child;
            while ( topobj != nullptr )
            {
                OBJECT* obj = topobj->child;
                while ( obj != nullptr )
                {
                    trnCDTerrainObject* tto = (trnCDTerrainObject*)(obj->lpspare);
                    if ( tto )
                    {
                        b.add( tto->BBox1.x+WorldOriginOffset.x, tto->BBox1.y+WorldOriginOffset.y, tto->BBox2.x+WorldOriginOffset.x, tto->BBox2.y+WorldOriginOffset.y );
                        if ( counter==0 )
                        {
                            avgz = (tto->BBox1.z + tto->BBox2.z) / 2;
                        }
                        counter++;
                    }

                    obj = obj->sibling;
                }
                topobj = topobj->sibling;
            }

            assert( avgz != -FLT_MAX );

            bool isTile = poolModel->modelType==ModelTypes::TERRAIN_TILE;

            if ( b.min_x < 100000000.0 ) // has an actual bounding box
            {
                if ( newobj->sdbInfo != 0 )
                {
                    printf("trying to add spatial db object twice\n");
                }

                SDB_AddObject( newobj, b.min_x, b.min_y, b.max_x, b.max_y, avgz,terrainDetail, isTile );
            }
        }
#endif
	}

	return TRUE;
}

/*
	New terrain cell layout

	00		01		02		03		04
		-------------------------
	05	|	06		07		08	|	09
		|						|
	10	|	11		12		13	|	14
		|						|
	15	|	16		17		18	|	19
		-------------------------
	20		21		22		23		24


	Old terrain cell layout

	-------------------------
	|	00		01		02	|
	|						|
	|	03		04		05	|
	|						|
	|	06		07		08	|
	-------------------------


	Conversion of old to new terrain cells

	00	->	06
	01	->	07
	02	->	08
	03	->	11
	04	->	12
	05	->	13
	06	->	16
	07	->	17
	08	->	18
*/

static LOGICAL RawUpdate(LOGICAL ForceCompleteReload)
{
	// This keeps us from bouncing all over the place while the world is moving under us
	if (_WorldMoved || !TerrainStable)
	{
		return(TRUE);
	}

	static TERRAIN_DETAIL	td;	// Because its HUGE
	SINT32			i, j, bits;
	piVECTOR		p;
	SINT16			scx, scy;

	p.x = -playpos.x + (FLOAT32)WorldOriginOffset.x;
	p.y = -playpos.y + (FLOAT32)WorldOriginOffset.y;
	p.z = -playpos.z + (FLOAT32)WorldOriginOffset.z;

	// 11111 11111 11111 11111 11111
	// 1 1111 1111 1111 1111 1111 1111
	bits = 0x1ffffff;
	scx = SUPERCELL_INVALID;
	scy = SUPERCELL_INVALID;
	if (!trnmainFindSupercell(&scx, &scy, &p))
	{
		Write_Debug("Could not find the center terrain square");

#ifdef DONT_USE
		for (i=0; i < NUM_TERRAIN_RECTANGLES; i++)
		{
			trndtl[i].terrain->flags &= OBJECT_SKIPDRAWING;
			trndtl[i].scx = SUPERCELL_INVALID;
			trndtl[i].scy = SUPERCELL_INVALID;
		}
#endif

		return(TRUE);
	}
	else
	{
		// On our initial launch into the world, our position is
		// at supercell x = 0 and supercell y = 0; this is a
		// temporary situation until we launch to our true position
		// in the world; so, to prevent loading terrain data that
		// will just be dumped, we ignore our first launch; later,
		// if we end up at this location, then it's because we are
		// supposed to be here; so process everything normally
		//
		// At the very least, use this for the Terrain Editor!
//#ifdef BETA_RELEASE
	if(!OverrideSpawner)
	{
//#endif
		{
		static LOGICAL	InitialLaunch=TRUE;

			if (scx == 0 && scy == 0)
			{
				if (InitialLaunch)
				{
					return(TRUE);
				}
			}
			else
			{
				InitialLaunch = FALSE;
			}
		}
//#ifdef BETA_RELEASE
}
//#endif
		if (ForceCompleteReload != TRUE)
		{
			for (i=0; i < NUM_TERRAIN_RECTANGLES; i++)
			{
				if (trndtl[i].scx == scx && trndtl[i].scy == scy)
				{
					switch (i)
					{
					case 6:
						td = trndtl[24];
						trndtl[24] = trndtl[18];
						trndtl[18] = trndtl[12];
						trndtl[12] = trndtl[6];
						trndtl[6] = trndtl[0];
						trndtl[0] = td;
						trnldrStartUnloadingSecondHalf(0);
						trnldrStartUnloadingFirstHalf(24);

						td = trndtl[23];
						trndtl[23] = trndtl[17];
						trndtl[17] = trndtl[11];
						trndtl[11] = trndtl[5];
						trndtl[5] = td;
						trnldrStartUnloadingSecondHalf(5);
						trnldrStartUnloadingFirstHalf(23);

						td = trndtl[22];
						trndtl[22] = trndtl[16];
						trndtl[16] = trndtl[10];
						trndtl[10] = td;
						trnldrStartUnloadingSecondHalf(10);
						trnldrStartUnloadingFirstHalf(22);

						td = trndtl[21];
						trndtl[21] = trndtl[15];
						trndtl[15] = td;
						trnldrStartUnloadingSecondHalf(15);

						trnldrStartUnloadingSecondHalf(20);

						td = trndtl[19];
						trndtl[19] = trndtl[13];
						trndtl[13] = trndtl[7];
						trndtl[7] = trndtl[1];
						trndtl[1] = td;
						trnldrStartUnloadingSecondHalf(1);
						trnldrStartUnloadingFirstHalf(19);

						td = trndtl[14];
						trndtl[14] = trndtl[8];
						trndtl[8] = trndtl[2];
						trndtl[2] = td;
						trnldrStartUnloadingSecondHalf(2);
						trnldrStartUnloadingFirstHalf(14);

						td = trndtl[9];
						trndtl[9] = trndtl[3];
						trndtl[3] = td;
						trnldrStartUnloadingSecondHalf(3);

						trnldrStartUnloadingSecondHalf(4);

						// 00001 00011 00011 01111 11111
						// 0 0001 0001 1000 1101 1111 1111
						bits = 0x0118dff;
						break;
					case 7:
						td = trndtl[24];
						trndtl[24] = trndtl[19];
						trndtl[19] = trndtl[14];
						trndtl[14] = trndtl[9];
						trndtl[9] = trndtl[4];
						trndtl[4] = td;
						trnldrStartUnloadingSecondHalf(4);

						td = trndtl[23];
						trndtl[23] = trndtl[18];
						trndtl[18] = trndtl[13];
						trndtl[13] = trndtl[8];
						trndtl[8] = trndtl[3];
						trndtl[3] = td;
						trnldrStartUnloadingSecondHalf(3);
						trnldrStartUnloadingFirstHalf(23);

						td = trndtl[22];
						trndtl[22] = trndtl[17];
						trndtl[17] = trndtl[12];
						trndtl[12] = trndtl[7];
						trndtl[7] = trndtl[2];
						trndtl[2] = td;
						trnldrStartUnloadingSecondHalf(2);
						trnldrStartUnloadingFirstHalf(22);

						td = trndtl[21];
						trndtl[21] = trndtl[16];
						trndtl[16] = trndtl[11];
						trndtl[11] = trndtl[6];
						trndtl[6] = trndtl[1];
						trndtl[1] = td;
						trnldrStartUnloadingSecondHalf(1);
						trnldrStartUnloadingFirstHalf(21);

						td = trndtl[20];
						trndtl[20] = trndtl[15];
						trndtl[15] = trndtl[10];
						trndtl[10] = trndtl[5];
						trndtl[5] = trndtl[0];
						trndtl[0] = td;
						trnldrStartUnloadingSecondHalf(0);

						// 00000 00000 00000 01110 11111
						// 0 0000 0000 0000 0001 1101 1111
						bits = 0x00001df;
						break;
					case 8:
						td = trndtl[20];
						trndtl[20] = trndtl[16];
						trndtl[16] = trndtl[12];
						trndtl[12] = trndtl[8];
						trndtl[8] = trndtl[4];
						trndtl[4] = td;
						trnldrStartUnloadingSecondHalf(4);
						trnldrStartUnloadingFirstHalf(20);

						td = trndtl[15];
						trndtl[15] = trndtl[11];
						trndtl[11] = trndtl[7];
						trndtl[7] = trndtl[3];
						trndtl[3] = td;
						trnldrStartUnloadingSecondHalf(3);
						trnldrStartUnloadingFirstHalf(15);

						td = trndtl[10];
						trndtl[10] = trndtl[6];
						trndtl[6] = trndtl[2];
						trndtl[2] = td;
						trnldrStartUnloadingSecondHalf(2);
						trnldrStartUnloadingFirstHalf(10);

						td = trndtl[5];
						trndtl[5] = trndtl[1];
						trndtl[1] = td;
						trnldrStartUnloadingSecondHalf(1);

						trnldrStartUnloadingSecondHalf(0);

						td = trndtl[21];
						trndtl[21] = trndtl[17];
						trndtl[17] = trndtl[13];
						trndtl[13] = trndtl[9];
						trndtl[9] = td;
						trnldrStartUnloadingSecondHalf(9);
						trnldrStartUnloadingFirstHalf(21);

						td = trndtl[22];
						trndtl[22] = trndtl[18];
						trndtl[18] = trndtl[14];
						trndtl[14] = td;
						trnldrStartUnloadingSecondHalf(14);
						trnldrStartUnloadingFirstHalf(22);

						td = trndtl[23];
						trndtl[23] = trndtl[19];
						trndtl[19] = td;
						trnldrStartUnloadingSecondHalf(19);

						trnldrStartUnloadingSecondHalf(24);

						// 10000 11000 11000 11110 11111
						// 1 0000 1100 0110 0011 1101 1111
						bits = 0x10c63df;
						break;
					case 11:
						td = trndtl[4];
						trndtl[4] = trndtl[3];
						trndtl[3] = trndtl[2];
						trndtl[2] = trndtl[1];
						trndtl[1] = trndtl[0];
						trndtl[0] = td;
						trnldrStartUnloadingSecondHalf(0);

						td = trndtl[9];
						trndtl[9] = trndtl[8];
						trndtl[8] = trndtl[7];
						trndtl[7] = trndtl[6];
						trndtl[6] = trndtl[5];
						trndtl[5] = td;
						trnldrStartUnloadingSecondHalf(5);
						trnldrStartUnloadingFirstHalf(9);

						td = trndtl[14];
						trndtl[14] = trndtl[13];
						trndtl[13] = trndtl[12];
						trndtl[12] = trndtl[11];
						trndtl[11] = trndtl[10];
						trndtl[10] = td;
						trnldrStartUnloadingSecondHalf(10);
						trnldrStartUnloadingFirstHalf(14);

						td = trndtl[19];
						trndtl[19] = trndtl[18];
						trndtl[18] = trndtl[17];
						trndtl[17] = trndtl[16];
						trndtl[16] = trndtl[15];
						trndtl[15] = td;
						trnldrStartUnloadingSecondHalf(15);
						trnldrStartUnloadingFirstHalf(19);

						td = trndtl[24];
						trndtl[24] = trndtl[23];
						trndtl[23] = trndtl[22];
						trndtl[22] = trndtl[21];
						trndtl[21] = trndtl[20];
						trndtl[20] = td;
						trnldrStartUnloadingSecondHalf(20);

						// 00001 00011 00011 00011 00001
						// 0 0001 0001 1000 1100 0110 0001
						bits = 0x0118c61;
						break;
					case 12:
						return(TRUE);
						break;
					case 13:
						td = trndtl[0];
						trndtl[0] = trndtl[1];
						trndtl[1] = trndtl[2];
						trndtl[2] = trndtl[3];
						trndtl[3] = trndtl[4];
						trndtl[4] = td;
						trnldrStartUnloadingSecondHalf(4);

						td = trndtl[5];
						trndtl[5] = trndtl[6];
						trndtl[6] = trndtl[7];
						trndtl[7] = trndtl[8];
						trndtl[8] = trndtl[9];
						trndtl[9] = td;
						trnldrStartUnloadingSecondHalf(9);
						trnldrStartUnloadingFirstHalf(5);

						td = trndtl[10];
						trndtl[10] = trndtl[11];
						trndtl[11] = trndtl[12];
						trndtl[12] = trndtl[13];
						trndtl[13] = trndtl[14];
						trndtl[14] = td;
						trnldrStartUnloadingSecondHalf(14);
						trnldrStartUnloadingFirstHalf(10);

						td = trndtl[15];
						trndtl[15] = trndtl[16];
						trndtl[16] = trndtl[17];
						trndtl[17] = trndtl[18];
						trndtl[18] = trndtl[19];
						trndtl[19] = td;
						trnldrStartUnloadingSecondHalf(19);
						trnldrStartUnloadingFirstHalf(15);

						td = trndtl[20];
						trndtl[20] = trndtl[21];
						trndtl[21] = trndtl[22];
						trndtl[22] = trndtl[23];
						trndtl[23] = trndtl[24];
						trndtl[24] = td;
						trnldrStartUnloadingSecondHalf(24);

						// 10000 11000 11000 11000 10000
						// 1 0000 1100 0110 0011 0001 0000
						bits = 0x10c6310;
						break;
					case 16:
						td = trndtl[4];
						trndtl[4] = trndtl[8];
						trndtl[8] = trndtl[12];
						trndtl[12] = trndtl[16];
						trndtl[16] = trndtl[20];
						trndtl[20] = td;
						trnldrStartUnloadingSecondHalf(20);
						trnldrStartUnloadingFirstHalf(4);

						td = trndtl[3];
						trndtl[3] = trndtl[7];
						trndtl[7] = trndtl[11];
						trndtl[11] = trndtl[15];
						trndtl[15] = td;
						trnldrStartUnloadingSecondHalf(15);
						trnldrStartUnloadingFirstHalf(3);

						td = trndtl[2];
						trndtl[2] = trndtl[6];
						trndtl[6] = trndtl[10];
						trndtl[10] = td;
						trnldrStartUnloadingSecondHalf(10);
						trnldrStartUnloadingFirstHalf(2);

						td = trndtl[1];
						trndtl[1] = trndtl[5];
						trndtl[5] = td;
						trnldrStartUnloadingSecondHalf(5);

						trnldrStartUnloadingSecondHalf(0);

						td = trndtl[9];
						trndtl[9] = trndtl[13];
						trndtl[13] = trndtl[17];
						trndtl[17] = trndtl[21];
						trndtl[21] = td;
						trnldrStartUnloadingSecondHalf(21);
						trnldrStartUnloadingFirstHalf(9);

						td = trndtl[14];
						trndtl[14] = trndtl[18];
						trndtl[18] = trndtl[22];
						trndtl[22] = td;
						trnldrStartUnloadingSecondHalf(22);
						trnldrStartUnloadingFirstHalf(14);

						td = trndtl[19];
						trndtl[19] = trndtl[23];
						trndtl[23] = td;
						trnldrStartUnloadingSecondHalf(23);

						trnldrStartUnloadingSecondHalf(24);

						// 11111 01111 00011 00011 00001
						// 1 1111 0111 1000 1100 0110 0001
						bits = 0x1f78c61;
						break;
					case 17:
						td = trndtl[0];
						trndtl[0] = trndtl[5];
						trndtl[5] = trndtl[10];
						trndtl[10] = trndtl[15];
						trndtl[15] = trndtl[20];
						trndtl[20] = td;
						trnldrStartUnloadingSecondHalf(20);

						td = trndtl[1];
						trndtl[1] = trndtl[6];
						trndtl[6] = trndtl[11];
						trndtl[11] = trndtl[16];
						trndtl[16] = trndtl[21];
						trndtl[21] = td;
						trnldrStartUnloadingSecondHalf(21);
						trnldrStartUnloadingFirstHalf(1);

						td = trndtl[2];
						trndtl[2] = trndtl[7];
						trndtl[7] = trndtl[12];
						trndtl[12] = trndtl[17];
						trndtl[17] = trndtl[22];
						trndtl[22] = td;
						trnldrStartUnloadingSecondHalf(22);
						trnldrStartUnloadingFirstHalf(2);

						td = trndtl[3];
						trndtl[3] = trndtl[8];
						trndtl[8] = trndtl[13];
						trndtl[13] = trndtl[18];
						trndtl[18] = trndtl[23];
						trndtl[23] = td;
						trnldrStartUnloadingSecondHalf(23);
						trnldrStartUnloadingFirstHalf(3);

						td = trndtl[4];
						trndtl[4] = trndtl[9];
						trndtl[9] = trndtl[14];
						trndtl[14] = trndtl[19];
						trndtl[19] = trndtl[24];
						trndtl[24] = td;
						trnldrStartUnloadingSecondHalf(24);

						// 11111 01110 00000 00000 00000
						// 1 1111 0111 0000 0000 0000 0000
						bits = 0x1f70000;
						break;
					case 18:
						td = trndtl[0];
						trndtl[0] = trndtl[6];
						trndtl[6] = trndtl[12];
						trndtl[12] = trndtl[18];
						trndtl[18] = trndtl[24];
						trndtl[24] = td;
						trnldrStartUnloadingSecondHalf(24);
						trnldrStartUnloadingFirstHalf(0);

						td = trndtl[1];
						trndtl[1] = trndtl[7];
						trndtl[7] = trndtl[13];
						trndtl[13] = trndtl[19];
						trndtl[19] = td;
						trnldrStartUnloadingSecondHalf(19);
						trnldrStartUnloadingFirstHalf(1);

						td = trndtl[2];
						trndtl[2] = trndtl[8];
						trndtl[8] = trndtl[14];
						trndtl[14] = td;
						trnldrStartUnloadingSecondHalf(14);
						trnldrStartUnloadingFirstHalf(2);

						td = trndtl[3];
						trndtl[3] = trndtl[9];
						trndtl[9] = td;
						trnldrStartUnloadingSecondHalf(9);

						trnldrStartUnloadingSecondHalf(4);

						td = trndtl[5];
						trndtl[5] = trndtl[11];
						trndtl[11] = trndtl[17];
						trndtl[17] = trndtl[23];
						trndtl[23] = td;
						trnldrStartUnloadingSecondHalf(23);
						trnldrStartUnloadingFirstHalf(5);

						td = trndtl[10];
						trndtl[10] = trndtl[16];
						trndtl[16] = trndtl[22];
						trndtl[22] = td;
						trnldrStartUnloadingSecondHalf(22);
						trnldrStartUnloadingFirstHalf(10);

						td = trndtl[15];
						trndtl[15] = trndtl[21];
						trndtl[21] = td;
						trnldrStartUnloadingSecondHalf(21);

						trnldrStartUnloadingSecondHalf(20);

						// 11111 11110 11000 11000 10000
						// 1 1111 1111 0110 0011 0001 0000
						bits = 0x1ff6310;
						break;
					default:
						// A periphery supercell causes a full load like a warp
						Write_Debug("Supercell %d became center square (%04x_%04x)", i, (UINT16)scx, (UINT16)scy);
						i = NUM_TERRAIN_RECTANGLES;
						bits = 0x1ffffff;
						break;
					}

					if (i != NUM_TERRAIN_RECTANGLES)
					{
						Write_Debug("Supercell %d became center square (%04x_%04x)", i, (UINT16)scx, (UINT16)scy);
					}

					break;
				}
			}
		}
		else
		{
			i = NUM_TERRAIN_RECTANGLES;
			bits = 0x1ffffff;
		}

		if (i == NUM_TERRAIN_RECTANGLES)
		{
			for (i=0; i < NUM_TERRAIN_RECTANGLES; i++)
			{
				trnldrReleaseSupercell(i);
				// This process can take a long time, so attend to the network so the host
				// doesn't think we've lost connection.
				teulUpdateTransport();
			}
		}

		for (i=0; i < NUM_TERRAIN_RECTANGLES; i++)
		{
			for (j=0; j < MAX_TERRAIN_TEXTURES; j++)
			{
				if (trndtl[i].nonreplacementtiles[j] != nullptr)
				{
					trndtl[i].nonreplacementtiles[j]->trnum = i;
				}
			}

			trndtl[i].terrain->trnum = i;
		}
	}

	// Periphery supercells only get limited loading until they become  potentially viewable

	// 00000 00000 00000 00000 00001
	// 0 0000 0000 0000 0000 0000 0001
	if (bits & 0x0000001)
	{
		if (!trnldrStartLoadingFirstHalf(0, (SINT16)(scx - 2), (SINT16)(scy + 2)))
		{
			Write_Debug("Could not build terrain map at 0");
			return(FALSE);
		}
	}

	// 00000 00000 00000 00000 00010
	// 0 0000 0000 0000 0000 0000 0010
	if (bits & 0x0000002)
	{
		if (!trnldrStartLoadingFirstHalf(1, (SINT16)(scx - 1), (SINT16)(scy + 2)))
		{
			Write_Debug("Could not build terrain map at 1");
			return(FALSE);
		}
	}

	// 00000 00000 00000 00000 00100
	// 0 0000 0000 0000 0000 0000 0100
	if (bits & 0x0000004)
	{
		if (!trnldrStartLoadingFirstHalf(2, scx, (SINT16)(scy + 2)))
		{
			Write_Debug("Could not build terrain map at 2");
			return(FALSE);
		}
	}

	// 00000 00000 00000 00000 01000
	// 0 0000 0000 0000 0000 0000 1000
	if (bits & 0x0000008)
	{
		if (!trnldrStartLoadingFirstHalf(3, (SINT16)(scx + 1), (SINT16)(scy + 2)))
		{
			Write_Debug("Could not build terrain map at 3");
			return(FALSE);
		}
	}

	// 00000 00000 00000 00000 10000
	// 0 0000 0000 0000 0000 0001 0000
	if (bits & 0x0000010)
	{
		if (!trnldrStartLoadingFirstHalf(4, (SINT16)(scx + 2), (SINT16)(scy + 2)))
		{
			Write_Debug("Could not build terrain map at 4");
			return(FALSE);
		}
	}

	// 00000 00000 00000 00001 00000
	// 0 0000 0000 0000 0000 0010 0000
	if (bits & 0x0000020)
	{
		if (!trnldrStartLoadingFirstHalf(5, (SINT16)(scx - 2), (SINT16)(scy + 1)))
		{
			Write_Debug("Could not build terrain map at 5");
			return(FALSE);
		}
	}

	// 00000 00000 00000 10000 00000
	// 0 0000 0000 0000 0010 0000 0000
	if (bits & 0x0000200)
	{
		if (!trnldrStartLoadingFirstHalf(9, (SINT16)(scx + 2), (SINT16)(scy + 1)))
		{
			Write_Debug("Could not build terrain map at 9");
			return(FALSE);
		}
	}

	// 00000 00000 00001 00000 00000
	// 0 0000 0000 0000 0100 0000 0000
	if (bits & 0x0000400)
	{
		if (!trnldrStartLoadingFirstHalf(10, (SINT16)(scx - 2), scy))
		{
			Write_Debug("Could not build terrain map at 10");
			return(FALSE);
		}
	}

	// 00000 00000 10000 00000 00000
	// 0 0000 0000 0100 0000 0000 0000
	if (bits & 0x0004000)
	{
		if (!trnldrStartLoadingFirstHalf(14, (SINT16)(scx + 2), scy))
		{
			Write_Debug("Could not build terrain map at 14");
			return(FALSE);
		}
	}

	// 00000 00001 00000 00000 00000
	// 0 0000 0000 1000 0000 0000 0000
	if (bits & 0x0008000)
	{
		if (!trnldrStartLoadingFirstHalf(15, (SINT16)(scx - 2), (SINT16)(scy - 1)))
		{
			Write_Debug("Could not build terrain map at 15");
			return(FALSE);
		}
	}

	// 00000 10000 00000 00000 00000
	// 0 0000 1000 0000 0000 0000 0000
	if (bits & 0x0080000)
	{
		if (!trnldrStartLoadingFirstHalf(19, (SINT16)(scx + 2), (SINT16)(scy - 1)))
		{
			Write_Debug("Could not build terrain map at 19");
			return(FALSE);
		}
	}

	// 00001 00000 00000 00000 00000
	// 0 0001 0000 0000 0000 0000 0000
	if (bits & 0x0100000)
	{
		if (!trnldrStartLoadingFirstHalf(20, (SINT16)(scx - 2), (SINT16)(scy - 2)))
		{
			Write_Debug("Could not build terrain map at 20");
			return(FALSE);
		}
	}

	// 00010 00000 00000 00000 00000
	// 0 0010 0000 0000 0000 0000 0000
	if (bits & 0x0200000)
	{
		if (!trnldrStartLoadingFirstHalf(21, (SINT16)(scx - 1), (SINT16)(scy - 2)))
		{
			Write_Debug("Could not build terrain map at 21");
			return(FALSE);
		}
	}

	// 00100 00000 00000 00000 00000
	// 0 0100 0000 0000 0000 0000 0000
	if (bits & 0x0400000)
	{
		if (!trnldrStartLoadingFirstHalf(22, scx, (SINT16)(scy - 2)))
		{
			Write_Debug("Could not build terrain map at 22");
			return(FALSE);
		}
	}

	// 01000 00000 00000 00000 00000
	// 0 1000 0000 0000 0000 0000 0000
	if (bits & 0x0800000)
	{
		if (!trnldrStartLoadingFirstHalf(23, (SINT16)(scx + 1), (SINT16)(scy - 2)))
		{
			Write_Debug("Could not build terrain map at 23");
			return(FALSE);
		}
	}

	// 10000 00000 00000 00000 00000
	// 1 0000 0000 0000 0000 0000 0000
	if (bits & 0x1000000)
	{
		if (!trnldrStartLoadingFirstHalf(24, (SINT16)(scx + 2), (SINT16)(scy - 2)))
		{
			Write_Debug("Could not build terrain map at 24");
			return(FALSE);
		}
	}


	// We have warped to new location so do immediate loading of supercells

	if (bits == 0x1ffffff)
	{
		Write_Debug("Warping to new terrain location");

		// Load terrain cells (initial)

		// Center square always inherits; except on warps

		// 00000 00000 00100 00000 00000
		// 0 0000 0000 0001 0000 0000 0000
		if (bits & 0x0001000)
		{
			trndtl[12].flags &= ~TDFLAG_PERIPHERY;
			trndtl[12].scx = scx;
			trndtl[12].scy = scy;
			if (!trnldrLoadSupercell(12))
			{
				Write_Debug("Could not build center terrain map");
				return(FALSE);
			}
		}

		// 00000 00000 00000 00010 00000
		// 0 0000 0000 0000 0000 0100 0000
		if (bits & 0x0000040)
		{
			trndtl[6].flags &= ~TDFLAG_PERIPHERY;
			trndtl[6].scx = scx - 1;
			trndtl[6].scy = scy + 1;
			if (!trnldrLoadSupercell(6))
			{
				Write_Debug("Could not build top left terrain map");
				return(FALSE);
			}
		}

		// 00000 00000 00000 00100 00000
		// 0 0000 0000 0000 0000 1000 0000
		if (bits & 0x0000080)
		{
			trndtl[7].flags &= ~TDFLAG_PERIPHERY;
			trndtl[7].scx = scx;
			trndtl[7].scy = scy + 1;
			if (!trnldrLoadSupercell(7))
			{
				Write_Debug("Could not build top center terrain map");
				return(FALSE);
			}
		}

		// 00000 00000 00000 01000 00000
		// 0 0000 0000 0000 0001 0000 0000
		if (bits & 0x0000100)
		{
			trndtl[8].flags &= ~TDFLAG_PERIPHERY;
			trndtl[8].scx = scx + 1;
			trndtl[8].scy = scy + 1;
			if (!trnldrLoadSupercell(8))
			{
				Write_Debug("Could not build right top terrain map");
				return(FALSE);
			}
		}

		// 00000 00000 00010 00000 00000
		// 0 0000 0000 0000 1000 0000 0000
		if (bits & 0x0000800)
		{
			trndtl[11].flags &= ~TDFLAG_PERIPHERY;
			trndtl[11].scx = scx - 1;
			trndtl[11].scy = scy;
			if (!trnldrLoadSupercell(11))
			{
				Write_Debug("Could not build left center terrain map");
				return(FALSE);
			}
		}

		// 00000 00000 01000 00000 00000
		// 0 0000 0000 0010 0000 0000 0000
		if (bits & 0x0002000)
		{
			trndtl[13].flags &= ~TDFLAG_PERIPHERY;
			trndtl[13].scx = scx + 1;
			trndtl[13].scy = scy;
			if (!trnldrLoadSupercell(13))
			{
				Write_Debug("Could not build right center terrain map");
				return(FALSE);
			}
		}

		// 00000 00010 00000 00000 00000
		// 0 0000 0001 0000 0000 0000 0000
		if (bits & 0x0010000)
		{
			trndtl[16].flags &= ~TDFLAG_PERIPHERY;
			trndtl[16].scx = scx - 1;
			trndtl[16].scy = scy - 1;
			if (!trnldrLoadSupercell(16))
			{
				Write_Debug("Could not build left bottom terrain map");
				return(FALSE);
			}
		}

		// 00000 00100 00000 00000 00000
		// 0 0000 0010 0000 0000 0000 0000
		if (bits & 0x0020000)
		{
			trndtl[17].flags &= ~TDFLAG_PERIPHERY;
			trndtl[17].scx = scx;
			trndtl[17].scy = scy - 1;
			if (!trnldrLoadSupercell(17))
			{
				Write_Debug("Could not build bottom center terrain map");
				return(FALSE);
			}
		}

		// 00000 01000 00000 00000 00000
		// 0 0000 0100 0000 0000 0000 0000
		if (bits & 0x0040000)
		{
			trndtl[18].flags &= ~TDFLAG_PERIPHERY;
			trndtl[18].scx = scx + 1;
			trndtl[18].scy = scy - 1;
			if (!trnldrLoadSupercell(18))
			{
				Write_Debug("Could not build right bottom terrain map");
				return(FALSE);
			}
		}


		// Modify terrain cells

		// 00000 00000 00100 00000 00000
		// 0 0000 0000 0001 0000 0000 0000
		if (bits & 0x0001000)
		{
			if (!trnldrModifySupercell(12))
			{
				Write_Debug("Could not modify center terrain map");
				return(FALSE);
			}
		}

		// 00000 00000 00000 00010 00000
		// 0 0000 0000 0000 0000 0100 0000
		if (bits & 0x0000040)
		{
			if (!trnldrModifySupercell(6))
			{
				Write_Debug("Could not modify top left terrain map");
				return(FALSE);
			}
		}

		// 00000 00000 00000 00100 00000
		// 0 0000 0000 0000 0000 1000 0000
		if (bits & 0x0000080)
		{
			if (!trnldrModifySupercell(7))
			{
				Write_Debug("Could not modify top center terrain map");
				return(FALSE);
			}
		}

		// 00000 00000 00000 01000 00000
		// 0 0000 0000 0000 0001 0000 0000
		if (bits & 0x0000100)
		{
			if (!trnldrModifySupercell(8))
			{
				Write_Debug("Could not modify right top terrain map");
				return(FALSE);
			}
		}

		// 00000 00000 00010 00000 00000
		// 0 0000 0000 0000 1000 0000 0000
		if (bits & 0x0000800)
		{
			if (!trnldrModifySupercell(11))
			{
				Write_Debug("Could not modify left center terrain map");
				return(FALSE);
			}
		}

		// 00000 00000 01000 00000 00000
		// 0 0000 0000 0010 0000 0000 0000
		if (bits & 0x0002000)
		{
			if (!trnldrModifySupercell(13))
			{
				Write_Debug("Could not modify right center terrain map");
				return(FALSE);
			}
		}

		// 00000 00010 00000 00000 00000
		// 0 0000 0001 0000 0000 0000 0000
		if (bits & 0x0010000)
		{
			if (!trnldrModifySupercell(16))
			{
				Write_Debug("Could not modify left bottom terrain map");
				return(FALSE);
			}
		}

		// 00000 00100 00000 00000 00000
		// 0 0000 0010 0000 0000 0000 0000
		if (bits & 0x0020000)
		{
			if (!trnldrModifySupercell(17))
			{
				Write_Debug("Could not modify bottom center terrain map");
				return(FALSE);
			}
		}

		// 00000 01000 00000 00000 00000
		// 0 0000 0100 0000 0000 0000 0000
		if (bits & 0x0040000)
		{
			if (!trnldrModifySupercell(18))
			{
				Write_Debug("Could not modify right bottom terrain map");
				return(FALSE);
			}
		}

		TerrainStable = TRUE;
		TerrainLightingUpdate = TRUE;
	}
	else
	{
		// Load potentially viewable supercells

		// 00000 00000 00000 00010 00000
		// 0 0000 0000 0000 0000 0100 0000
		if (bits & 0x0000040)
		{
			if (!trnldrStartLoadingSecondHalf(6, (SINT16)(scx - 1), (SINT16)(scy + 1)))
			{
				Write_Debug("Could not build top left terrain map");
				return(FALSE);
			}
		}

		// 00000 00000 00000 00100 00000
		// 0 0000 0000 0000 0000 1000 0000
		if (bits & 0x0000080)
		{
			if (!trnldrStartLoadingSecondHalf(7, scx, (SINT16)(scy + 1)))
			{
				Write_Debug("Could not build top center terrain map");
				return(FALSE);
			}
		}

		// 00000 00000 00000 01000 00000
		// 0 0000 0000 0000 0001 0000 0000
		if (bits & 0x0000100)
		{
			if (!trnldrStartLoadingSecondHalf(8, (SINT16)(scx + 1), (SINT16)(scy + 1)))
			{
				Write_Debug("Could not build right top terrain map");
				return(FALSE);
			}
		}

		// 00000 00000 00010 00000 00000
		// 0 0000 0000 0000 1000 0000 0000
		if (bits & 0x0000800)
		{
			if (!trnldrStartLoadingSecondHalf(11, (SINT16)(scx - 1), scy))
			{
				Write_Debug("Could not build left center terrain map");
				return(FALSE);
			}
		}


		/*
			Center square (12) always inherits; except on warps
		*/


		// 00000 00000 01000 00000 00000
		// 0 0000 0000 0010 0000 0000 0000
		if (bits & 0x0002000)
		{
			if (!trnldrStartLoadingSecondHalf(13, (SINT16)(scx + 1), scy))
			{
				Write_Debug("Could not build right center terrain map");
				return(FALSE);
			}
		}

		// 00000 00010 00000 00000 00000
		// 0 0000 0001 0000 0000 0000 0000
		if (bits & 0x0010000)
		{
			if (!trnldrStartLoadingSecondHalf(16, (SINT16)(scx - 1), (SINT16)(scy - 1)))
			{
				Write_Debug("Could not build left bottom terrain map");
				return(FALSE);
			}
		}

		// 00000 00100 00000 00000 00000
		// 0 0000 0010 0000 0000 0000 0000
		if (bits & 0x0020000)
		{
			if (!trnldrStartLoadingSecondHalf(17, scx, (SINT16)(scy - 1)))
			{
				Write_Debug("Could not build bottom center terrain map");
				return(FALSE);
			}
		}

		// 00000 01000 00000 00000 00000
		// 0 0000 0100 0000 0000 0000 0000
		if (bits & 0x0040000)
		{
			if (!trnldrStartLoadingSecondHalf(18, (SINT16)(scx + 1), (SINT16)(scy - 1)))
			{
				Write_Debug("Could not build right bottom terrain map");
				return(FALSE);
			}
		}

		TerrainStable = FALSE;
	}

#ifdef DONT_USE
	// ensure all terrain cells have correct matricies
	Mat = IdentityMatrix;
	Mat._41 = -(FLOAT32)WorldOriginOffset.x;
	Mat._42 = -(FLOAT32)WorldOriginOffset.y;
	Mat._43 = -(FLOAT32)WorldOriginOffset.z;
	_SetTerrainMatricies(Mat);
#endif

	updated = TRUE;

	return(TRUE);
}

#if(0)
static LOGICAL FogUpdate()
{
int			i, j, k, num, spec;
FLOAT32		dmin, dmax, idmm, d, x, y, z;
piVECTOR	p;
OBJECT*	tt;

	if (!bFogEnabled)
	{
		return(TRUE);
	}

	p.x = -playpos.x;
	p.y = -playpos.y;
	p.z = -playpos.z;

	dmin = horizon * 0.75f;
	dmin *= dmin;

	dmax = horizon * 1.0f;
	dmax *= dmax;

	idmm = 255.0f / (dmax - dmin);

	for (i=0; i < NUM_TERRAIN_RECTANGLES; i++)
	{
		tt = trndtl[i].terrain;
		if (tt->flags & OBJECT_VISIBLE)
		{
			piVectorPtr spos = tt->StaticPositions();
			piVertexColorPtr sc = tt->StaticColors();

			num = tt->numverts;
			for (j=0; j < num; j++, ++sc, ++spos)
			{
				x = spos->x - p.x;
				y = spos->y - p.y;
				z = spos->z - p.z;
				d = (x * x + y * y + z * z);
				if (d < dmin)
				{
					sc->color = piRGBA_SETALPHA(sc->color, 255);
					continue;
				}

				if (d >= dmax)
				{
					sc->color = piRGBA_SETALPHA(sc->color, 0);
					continue;
				}

				d = (d - dmin) * idmm;
				spec = FloatToInt(d);
				sc->color = piRGBA_SETALPHA(sc->color, 255 - spec);
			}
		}

		for (k=0; k < MAX_TERRAIN_TEXTURES; k++)
		{
			tt = trndtl[i].nonreplacementtiles[k];
			if (tt == nullptr || !(tt->flags & OBJECT_VISIBLE) || tt->numtris == 0)
			{
				continue;
			}

			piVectorPtr spos = tt->StaticPositions();
			piVertexColorPtr sc = tt->StaticColors();

			num = tt->numverts;
			for (j=0; j < num; j++, ++sc, ++spos)
			{
				x = spos->x - p.x;
				y = spos->y - p.y;
				z = spos->z - p.z;
				d = (x * x + y * y + z * z);
				if (d < dmin)
				{
					sc->color = piRGBA_SETALPHA(sc->color, 255);
					continue;
				}

				if (d >= dmax)
				{
					sc->color = piRGBA_SETALPHA(sc->color, 0);
					continue;
				}

				d = (d - dmin) * idmm;
				spec = FloatToInt(d);
				sc->color = piRGBA_SETALPHA(sc->color, 255 - spec);
			}
		}
	}

	return(TRUE);
}
#endif

#if(0)
static LOGICAL WaveUpdate()
{
SINT32		i, j, num;
UINT32		color;
FLOAT32		x, y, z, r0x, r0y, r0freq, r1x, r1y, r1freq;
piVECTOR	p;
OBJECT*	tt;
piINDICES*	pt;

r0x = r0y = r0freq = r1x = r1y = r1freq = 0;

	if (!bAnimatedWater)
	{
		return(TRUE);
	}

	p.x = -playpos.x;
	p.y = -playpos.y;
	p.z = -playpos.z;

	color = piRGBA_MAKE(WATER_COLOR_RED, WATER_COLOR_GREEN, WATER_COLOR_BLUE, WATER_COLOR_ALPHA);
	for (i=0; i < NUM_TERRAIN_RECTANGLES; i++)
	{
		tt = trndtl[i].terrain;
		if (!(tt->flags & OBJECT_VISIBLE))
		{
			continue;
		}

		num = tt->numtris;
		pt = tt->ptris;

		piVectorConstPtr ipos = tt->ImmutablePositions();
		piVectorPtr mpos = tt->MutablePositions();
		piVertexColorConstPtr ic = tt->ImmutableColors();

		num = tt->numverts;
		for (j=0; j < num; j++, ++ipos, ++mpos, ++ic)
		{
			if (ic->color == color)
			{
				x = ipos->x - r0x;
				y = ipos->y - r0y;
				z = (float)cos(PI * r0freq * (ft10mscount + (x * x + y * y) * 0.00001f));
				//z = (float)cos(PI * r0freq * (ft10mscount + (float)sqrt(x * x + y * y)));
				x = ipos->x - r1x;
				y = ipos->y - r1y;
				z += (float)cos(PI * r1freq * (ft10mscount + (x * x + y * y) * 0.00001f));
				//z += (float)cos(PI * r1freq * (ft10mscount + (float)sqrt(x * x + y * y)));
				mpos->z = ipos->z + z * 10.0f * ScaleFactor;
			}
		}

	}

	return(TRUE);
}
#endif

LOGICAL trnmainDoRawUpdate(LOGICAL ForceCompleteLoad)
{
	if (!RawUpdate(ForceCompleteLoad))
	{
		return(FALSE);
	}

	return (TRUE);
}

// Check if position is on a replacement tile.
bool trnmainIsReplacement(const FLOAT64VECTOR& pos)
{
#if !WANT_NON_REPLACEMENT_TILES
    return 1;
#endif

    // Get supercell
    int sx = (int)(pos.x / SDM_SUPERCELL_SIZE);
    int sy = (int)(pos.y / SDM_SUPERCELL_SIZE);

    sx = sx < 0 ? sx - 1 : sx;
    sy = sy < 0 ? sy - 1 : sy;

    piVECTOR v;

    // Get position within a supercell
    v.x = float(pos.x - sx * SDM_SUPERCELL_SIZE);
    v.y = float(pos.y - sy * SDM_SUPERCELL_SIZE);

    // Determine row/column in the supercell to octet level
    int c = (int)(v.x / SDM_OCTET_SIZE);
    int r = (int)(v.y / SDM_OCTET_SIZE);

    int idx = TRNINDEX(r, c);

    TERRAIN_DETAIL* td = nullptr;

    for(int q = 0; q < NUM_TERRAIN_RECTANGLES; q++)
    {
        if(activeTerrainDetail[q] && trndtl[q].scx == sx && trndtl[q].scy == sy)
        {
            td = &trndtl[q];
            break;
        }
    }

    if(td)
    {
        if(!strncmp(td->triname[td->trinfo[idx].txndx], "null1", 5))
        {
            return true;
        }
    }

    return false;
}

LOGICAL trnmainUpdate()
{
	SINT32			paints;
	UINT32			tod;
	static UINT32	lasttod;

	logTimerBeginMacro();

	TerrainLightingUpdate = FALSE;

	if (!trnldrUpdate())
		return FALSE;

	// Set the lighting variable for this frame
	if (DynamicLighting)
	{
		trnTODGetTimeOfDay(&tod);
		if (abs((int)(tod - lasttod)) > UPDATE_TOD_DELTA ||
			TerrainLightingUpdate == TRUE)
		{
			LightingUpdate = TRUE;
			lasttod = tod;
		}
		else
		{
			LightingUpdate = FALSE;
		}
	}

	paints = piGetTotalPaints();

	updated = FALSE;

	// Player won't move through world very fast
	// so check terrain status every so often
	if (ft200msticks)
	{
		if (!RawUpdate(FALSE))
			return FALSE;
	}

	trncdUpdateStratMsgQue();

	trnUpdateTerrainObjectLODs(0);

	trnwtrUpdate();

    radclutterUpdate();
    speedgrassUpdate();

#ifdef USE_GAME_ADVERTISING
    adUpdate();
#endif

    trndataUpdate();

	logTimerEndMacro();

	return TRUE;
}

LOGICAL trnmainLODChange()
{
	trnlod += 1;
	if (trnlod >= 3)
	{
		trnlod = 0;
	}

	return(TRUE);
}

#if(0)
static void OutlineSupercell(TERRAIN_DETAIL* td, DWORD color)
{
SINT32		i;
FLOAT32		horizon, dmax, dist, x, y, z, xx, yy, zz;
piVECTOR	va, vb, vc;
OBJECT*	lpobj;

	lpobj = td->terrainorig;
	if (lpobj == nullptr)
	{
		return;
	}

	piSetRenderState(piRENDERSTATE_ZWRITEENABLE, FALSE);
	piSetRenderState(piRENDERSTATE_ZENABLE, FALSE);

	x = -playpos.x;// + (FLOAT32)WorldOriginOffset.x;// - lpobj->rot.x;
	y = -playpos.y;// + (FLOAT32)WorldOriginOffset.y;// - lpobj->rot.y;
	z = -playpos.z;// + (FLOAT32)WorldOriginOffset.z;// - lpobj->rot.z;

	horizon = piLongLikeFloat(FogEnd);
	dmax = horizon * horizon;

	piVectorConstPtr rpos = lpobj->RenderPositions();
	for (i=0; i < SUPERCELL_OCTETS_X; i++, rpos+=NUM_VERTICES_PER_OCTET)
	{
		va.x = rpos[0].x;
		va.y = rpos[0].y;
		va.z = rpos[0].z;
		if (lpobj->flags & OBJECT_USEMATRIX)
		{
			math3dVectorMultiply(&va, &va, &lpobj->mat);
		}

		xx = va.x - x;
		yy = va.y - y;
		zz = va.z - z;
		dist = xx * xx + yy * yy + zz * zz;
		if (dist >= dmax)
		{
			continue;
		}

		va.x = rpos[7].x;
		va.y = rpos[7].y;
		va.z = rpos[7].z;
		vb.x = rpos[8].x;
		vb.y = rpos[8].y;
		vb.z = rpos[8].z;
		vc.x = rpos[1].x;
		vc.y = rpos[1].y;
		vc.z = rpos[1].z;

		if (lpobj->flags & OBJECT_USEMATRIX)
		{
			math3dVectorMultiply(&va, &va, &lpobj->mat);
			math3dVectorMultiply(&vb, &vb, &lpobj->mat);
			math3dVectorMultiply(&vc, &vc, &lpobj->mat);
		}

		draw3dLine((piVECTOR*)&va, (piVECTOR*)&vb, color);
		draw3dLine((piVECTOR*)&vb, (piVECTOR*)&vc, color);
	}

	rpos = lpobj->RenderPositions();
	for (i=0; i < SUPERCELL_OCTETS_Y; i++, rpos+=SUPERCELL_OCTETS_X*NUM_VERTICES_PER_OCTET)
	{
		va.x = rpos[0].x;
		va.y = rpos[0].y;
		va.z = rpos[0].z;
		if (lpobj->flags & OBJECT_USEMATRIX)
		{
			math3dVectorMultiply(&va, &va, &lpobj->mat);
		}

		xx = va.x - x;
		yy = va.y - y;
		zz = va.z - z;
		dist = xx * xx + yy * yy + zz * zz;
		if (dist >= dmax)
		{
			continue;
		}

		va.x = rpos[1].x;
		va.y = rpos[1].y;
		va.z = rpos[1].z;
		vb.x = rpos[2].x;
		vb.y = rpos[2].y;
		vb.z = rpos[2].z;
		vc.x = rpos[3].x;
		vc.y = rpos[3].y;
		vc.z = rpos[3].z;

		if (lpobj->flags & OBJECT_USEMATRIX)
		{
			math3dVectorMultiply(&va, &va, &lpobj->mat);
			math3dVectorMultiply(&vb, &vb, &lpobj->mat);
			math3dVectorMultiply(&vc, &vc, &lpobj->mat);
		}

		draw3dLine((piVECTOR*)&va, (piVECTOR*)&vb, color);
		draw3dLine((piVECTOR*)&vb, (piVECTOR*)&vc, color);
	}

	piSetRenderState(piRENDERSTATE_ZWRITEENABLE, bZWrite);
	piSetRenderState(piRENDERSTATE_ZENABLE, bZEnable);
}
#endif

static void _cullTriangles(OBJECT* o)
{
    SINT32 i, j, k;
    SINT32 octets;
    FLOAT32 d1;
    FLOAT32 Radius;
    piStridedVertexConstPtr ps1, ps2, ps3, ps4;
    piCVERTEX* pvt;
    piVECTOR const* pn;
    piVECTOR pp, pos, dir;

    octets = o->numtris / NUM_TRIANGLES_PER_OCTET;

    piStridedVertexConstPtr sv;
    o->StridedRenderVertices(&sv);

    pn = o->psnorms;
    pvt = o->ptverts;

    pp.x = -playpos.x + (FLOAT32)WorldOriginOffset.x - o->rot.x;
    pp.y = -playpos.y + (FLOAT32)WorldOriginOffset.y - o->rot.y;
    pp.z = -playpos.z + (FLOAT32)WorldOriginOffset.z - o->rot.z;

    Radius = 800.0f;

    SINT32 index = 0;

    bool flipuv = false;

    bool iswater = trnwtrIsWaterTexture(o->texid);

#ifdef FORCE_DDS_NO_FLIP

    TEXTURE* t = piGetTextureData(o->texid);

    if(t)
    {
        if(t->isdds())
        {
            flipuv = true;
        }
    }

#endif

#ifdef BETA_RELEASE

        piMATRIX mw = IdentityMatrix;

        if(g_drawOctetTriangles)
        {
            mw = *piGetWorldMatrix();
            piSetWorldMatrix(&o->mat);
        }
#endif

    std::vector<piVECTOR>* verts;

    for(i = 0, j = 0; i < octets; i++, pn++)
    {
        ps3 = sv++;
        math3dVectorMultiply(&pos, ps3.pos.ptr, &o->mat);

        if(!rnvisVectorFOVCheck(&pos, Radius))
        {
            sv += NUM_VERTICES_PER_OCTET - 1;
            pn += NUM_TRIANGLES_PER_OCTET - 1;
            continue;
        }

        if(ShadowMapFlags != 0)
        {
            if(!iswater)
            {
                std::vector<piVECTOR> vertices;

                ShadowOctet[mapindex] = vertices;
                ShadowObject[mapindex] = o;

                verts = &ShadowOctet.find(mapindex)->second;
                mapindex++;
            }
        }

        ps4 = sv;

        for(k = 1; k < NUM_TRIANGLES_PER_OCTET; k++, pn++)
        {
            ps1 = sv++;
            ps2 = sv;

            math3dSubtractVectors(&dir, ps1.pos.ptr, &pp);
            d1 = math3dDotProduct(&dir, pn);

            if(d1 >= 0)
            {
                continue;
            }

            math3dVectorMultiply(&pos, ps1.pos.ptr, &o->mat);

            if(!rnvisVectorFOVCheck(&pos, Radius))
            {
                continue;
            }

            // Vertex reduction
            pvt->x = ps1.pos->x;
            pvt->y = ps1.pos->y;
            pvt->z = ps1.pos->z;
            pvt->nx = ps1.norm.ptr->x;
            pvt->ny = ps1.norm.ptr->y;
            pvt->nz = ps1.norm.ptr->z;
            pvt->color    = ps1.color->color;
            pvt->tu = ps1.tex->u;
            pvt->tv = flipuv ? 1.0f - ps1.tex->v : ps1.tex->v;

            index++;
            pvt++;

            pvt->x = ps2.pos->x;
            pvt->y = ps2.pos->y;
            pvt->z = ps2.pos->z;
            pvt->nx = ps2.norm.ptr->x;
            pvt->ny = ps2.norm.ptr->y;
            pvt->nz = ps2.norm.ptr->z;
            pvt->color    = ps2.color->color;
            pvt->tu = ps2.tex->u;
            pvt->tv = flipuv ? 1.0f - ps2.tex->v : ps2.tex->v;

            index++;
            pvt++;

            pvt->x = ps3.pos->x;
            pvt->y = ps3.pos->y;
            pvt->z = ps3.pos->z;
            pvt->nx = ps3.norm.ptr->x;
            pvt->ny = ps3.norm.ptr->y;
            pvt->nz = ps3.norm.ptr->z;
            pvt->color    = ps3.color->color;
            pvt->tu = ps3.tex->u;
            pvt->tv = flipuv ? 1.0f - ps3.tex->v : ps3.tex->v;

            index++;
            pvt++;

            j += 3;

            if(ShadowMapFlags != 0)
            {
                if(!iswater)
                {
                    piVECTOR v1 = *ps1.pos.ptr;
                    piVECTOR v2 = *ps2.pos.ptr;
                    piVECTOR v3 = *ps3.pos.ptr;

                    verts->push_back(v1);
                    verts->push_back(v2);
                    verts->push_back(v3);
                }
            }

#ifdef BETA_RELEASE
            if(g_drawOctetTriangles)
            {
                piVECTOR v1 = *ps1.pos.ptr;
                piVECTOR v2 = *ps2.pos.ptr;
                piVECTOR v3 = *ps3.pos.ptr;

                v1.z -= 1.0f;
                v2.z -= 1.0f;
                v3.z -= 1.0f;

                draw3dLine(&v1, &v2, colorWHITEALPHA);
                draw3dLine(&v2, &v3, colorWHITEALPHA);
                draw3dLine(&v3, &v1, colorWHITEALPHA);
            }
#endif
		}

        ps1 = sv++;

        math3dSubtractVectors(&dir, ps1.pos.ptr, &pp);
        d1 = math3dDotProduct(&dir, pn);

        if(d1 >= 0)
        {
            continue;
        }

        math3dVectorMultiply(&pos, ps1.pos.ptr, &o->mat);

        if(!rnvisVectorFOVCheck(&pos, Radius))
        {
            continue;
        }

        // Vertex reduction
        pvt->x = ps1.pos->x;
        pvt->y = ps1.pos->y;
        pvt->z = ps1.pos->z;
        pvt->nx = ps1.norm.ptr->x;
        pvt->ny = ps1.norm.ptr->y;
        pvt->nz = ps1.norm.ptr->z;
        pvt->color    = ps1.color->color;
        pvt->tu = ps1.tex->u;
        pvt->tv = flipuv ? 1.0f - ps1.tex->v : ps1.tex->v;

        index++;
        pvt++;

        pvt->x = ps4.pos->x;
        pvt->y = ps4.pos->y;
        pvt->z = ps4.pos->z;
        pvt->nx = ps4.norm.ptr->x;
        pvt->ny = ps4.norm.ptr->y;
        pvt->nz = ps4.norm.ptr->z;
        pvt->color    = ps4.color->color;
        pvt->tu = ps4.tex->u;
        pvt->tv = flipuv ? 1.0f - ps4.tex->v : ps4.tex->v;

        index++;
        pvt++;

        pvt->x = ps3.pos->x;
        pvt->y = ps3.pos->y;
        pvt->z = ps3.pos->z;
        pvt->nx = ps3.norm.ptr->x;
        pvt->ny = ps3.norm.ptr->y;
        pvt->nz = ps3.norm.ptr->z;
        pvt->color    = ps3.color->color;
        pvt->tu = ps3.tex->u;
        pvt->tv = flipuv ? 1.0f - ps3.tex->v : ps3.tex->v;

        index++;
        pvt++;

        j += 3;

        if(ShadowMapFlags != 0)
        {
            if(!iswater)
            {
                piVECTOR v1 = *ps1.pos.ptr;
                piVECTOR v2 = *ps4.pos.ptr;
                piVECTOR v3 = *ps3.pos.ptr;

                verts->push_back(v1);
                verts->push_back(v2);
                verts->push_back(v3);
            }
        }

#ifdef BETA_RELEASE
        if(g_drawOctetTriangles)
        {
            piVECTOR v1 = *ps1.pos.ptr;
            piVECTOR v2 = *ps4.pos.ptr;
            piVECTOR v3 = *ps3.pos.ptr;

            v1.z -= 1.0f;
            v2.z -= 1.0f;
            v3.z -= 1.0f;

            draw3dLine(&v1, &v2, colorBLUEALPHA);
            draw3dLine(&v2, &v3, colorBLUEALPHA);
            draw3dLine(&v3, &v1, colorBLUEALPHA);
        }
#endif
	}

#ifdef BETA_RELEASE

    if(g_drawOctetTriangles)
    {
        piSetWorldMatrix(&mw);
    }

#endif

    o->numtverts = j;

    NumDrawnTriangles += j;
    o->distsqr = math3dDotProduct(&o->pos, &playdir) + playtrans.z;
}

static void _drawScene(OBJECT* o)
{
    // Non replacement tiles
    if(o->flags & OBJECT_TERRAIN)
    {
        if(o->numtverts == 0)
        {
            return;
        }
    }
    // Replacement tiles
    else if(o->flag2 & OBJECT_TERRAIN_OBJECT)
    {
        if(o->numverts == 0)
        {
            return;
        }
    }

    SINT32 texType = obdrawGetMultiTextureOpcode(o->texid, nullptr);

    if(texType == MT_SINGLEPASS)
    {
        obdrawApplyMultiTextureStage2(o);
    }

    if(o->flags & OBJECT_TERRAIN)
    {
        piRenderScene(piPT_TRIANGLELIST, piVT_CVERTEX, o->ptverts, o->numtverts);
    }
    else if(o->flag2 & OBJECT_TERRAIN_OBJECT)
    {
        if(o->flags & OBJECT_INDEXED)
        {
            piSTRIDEDDATA sd;
            o->StridedRenderData(&sd);

            piRenderIndexStridedScene(piPT_TRIANGLELIST,
                                      piVT_VERTEX,
                                      &sd,
                                      o->numverts,
                                      (UINT16*)o->ptris,
                                      o->numtris * 3);
        }
        else
        {
            piSTRIDEDDATA sd;
            o->StridedRenderData(&sd);

            piRenderStridedScene(piPT_TRIANGLELIST,
                                 piVT_VERTEX,
                                 &sd,
                                 o->numverts);
        }
    }

    if(texType == MT_SINGLEPASS)
    {
        obdrawReleaseMultiTextureStage(o);
    }
    else if(texType == MT_BUMPMAP)
    {
        obdrawApplyBumpmapTextureStage(o);

        piRenderScene(piPT_TRIANGLELIST, piVT_CVERTEX, o->ptverts, o->numtverts);

        obdrawReleaseBumpmapTextureStage(o);
    }
    else if(texType == MT_MULTIPASS)
    {
        UINT32 flags = o->flags;

        o->flags &= ~OBJECT_INDEXED;

        // Apply any required multitexturing
        obdrawApplyMultiTexture(o);

        o->flags = flags;
    }

    if(trnwtrIsWaterTexture(o->texid))
    {
        if(o->flags & OBJECT_TERRAIN)
        {
            trnwtrPreDraw2(o);
        }
        else
        {
            trnwtrPreDraw(o);
        }

        if(o->flags & OBJECT_TERRAIN)
        {
            piSetRenderState(piRENDERSTATE_ZFUNC, piCMP_LESSEQUAL);
            piRenderScene(piPT_TRIANGLELIST, piVT_CVERTEX, o->ptverts, o->numtverts);
            piSetRenderState(piRENDERSTATE_ZFUNC, ZFunction);
        }
        else if(o->flag2 & OBJECT_TERRAIN_OBJECT)
        {
            if(o->flags & OBJECT_INDEXED)
            {
                piSTRIDEDDATA sd;
                o->StridedRenderData(&sd);

                piRenderIndexStridedScene(piPT_TRIANGLELIST,
                                          piVT_VERTEX,
                                          &sd,
                                          o->numverts,
                                          (UINT16*)o->ptris,
                                          o->numtris * 3);
            }
            else
            {
                piSTRIDEDDATA sd;
                o->StridedRenderData(&sd);

                piRenderStridedScene(piPT_TRIANGLELIST,
                                    piVT_VERTEX,
                                    &sd,
                                    o->numverts);
            }
        }

        trnwtrPostDraw();
	}

#ifdef VEHICLE_OBJECT_ANALYSIS

    for(SINT32i = 1; i < 4; i++)
	{
		for(SINT32 j = 1; j < 4; j++)
        {
            OutlineSupercell(&trndtl[i * 5 + j], piRGBA_MAKE(244,184,54,255));
        }
    }

#endif
}

static void _drawTerrain(OBJECT* o)
{
    piMATRIX wm;

    if(o->flags & OBJECT_SKIPDRAWING)
        return;

    _cullTriangles(o);

    if(o->numtverts == 0)
        return;

    piSetActiveTexture(o->texid, 0);

    if(!piSetActiveMaterial(o->matid))
    {
        piDisableMaterial();
    }

    if(o->flags & OBJECT_USEMATRIX)
    {
        wm = *piGetWorldMatrix();
        piSetWorldMatrix(&o->mat);
    }

    _drawScene(o);

    if(o->flags & OBJECT_USEMATRIX)
    {
        piSetWorldMatrix(&wm);
    }
}

static LOGICAL DrawUpdate(OBJECT* lpobj)
{
    if(trnwtrIsWaterTexture(lpobj->texid))
    {
        _cullTriangles(lpobj);

        if(lpobj->numtverts > 0)
        {
            if(lpobj->drawcallback)
            {
                lpobj->drawcallback(lpobj);
            }
            else
            {
                trnwtrDrawNonReplacementWater(lpobj);
            }
        }

        return TRUE;
    }

    _drawTerrain(lpobj);

    return TRUE;
}

void _drawObjectTriangles(OBJECT* o)
{
    piVectorConstPtr p = o->RenderPositions();

    for(SINT32 i = 0; i < o->numtris; i++)
    {
        draw3dLine(&p[o->ptris[i].v1], &p[o->ptris[i].v2], colorYELLOWALPHA);
        draw3dLine(&p[o->ptris[i].v2], &p[o->ptris[i].v3], colorYELLOWALPHA);
        draw3dLine(&p[o->ptris[i].v3], &p[o->ptris[i].v1], colorYELLOWALPHA);
    }
}

LOGICAL trnmainDrawLastFrame(OBJECT* lpobj)
{
	if (lpobj->numtverts > 0)
	{
		if (!piRenderScene(piPT_TRIANGLELIST, piVT_LVERTEX,
							lpobj->ptverts, lpobj->numtverts))
		{
			return(FALSE);
		}
	}

	return(TRUE);
}

LOGICAL trnmainDrawObject(OBJECT* lpobj)
{
LOGICAL		retval;

return(TRUE);

	if (lpobj->flags & OBJECT_VISIBLE)
	{
		retval = DrawUpdate(lpobj);
		if (!retval)
		{
			return(FALSE);
		}
	}

	return(TRUE);
}

void trnmainSetDraw(LOGICAL bOn)
{
    g_trnmainDraw = bOn;
}

void trnmainDrawOctetTriangles(LOGICAL draw)
{
    g_drawOctetTriangles = draw;
}

LOGICAL trnmainDraw()
{
LOGICAL			retval;
SINT32			i, j;

    extern void trnclutterDrawDebug();

    if(!g_trnmainDraw)
        return TRUE;

    if(ShadowMapFlags != 0)
    {
        mapindex = 0;
        ShadowOctet.clear();
        ShadowObject.clear();
    }

#ifndef USE_3DAPI_LIGHTING
SORTHEADER		sort;

	memset(&sort, 0, sizeof(sort));
	if (DynamicLighting)
	{
		if (LightingUpdate)
		{
			for (i=0; i < NUM_TERRAIN_RECTANGLES; i++)
			{
				if (trndtl[i].terrain != nullptr &&
					trndtl[i].terrain->flags & OBJECT_VISIBLE)
				{
					sort.part = trndtl[i].terrain;
					if (!obdliteCalculate(&sort, FALSE))
					{
						return(FALSE);
					}
				}

				for (j=0; j < MAX_TERRAIN_TEXTURES; j++)
				{
					if (trndtl[i].txtobj[j] != nullptr &&
						trndtl[i].txtobj[j]->flags & OBJECT_VISIBLE)
					{
						sort.part = trndtl[i].txtobj[j];
						if (!obdliteCalculate(&sort, FALSE))
						{
							return(FALSE);
						}
					}
				}
			}
		}
		else
		{
			for (i=0; i < NUM_TERRAIN_RECTANGLES; i++)
			{
				if (activeTerrainDetail[i] == FALSE)
				{
					continue;
				}

				for (j=0; j < MAX_TERRAIN_TEXTURES; j++)
				{
					if (trndtl[i].txtobj[j] != nullptr &&
						trndtl[i].txtobj[j]->flags & OBJECT_VISIBLE &&
						!(trndtl[i].txtobj[j]->flags & OBJECT_NOSPECULAR))
					{
						sort.part = trndtl[i].txtobj[j];
						if (!obdliteCalculate(&sort, FALSE))
						{
							return(FALSE);
						}
					}
				}
			}
		}
	}
#endif

#if USE_STATESET
#else
    piSetRenderState(piRENDERSTATE_ALPHABLENDENABLE, FALSE);
#endif
	// Keep track of number of triangles we have drawn
	NumDrawnTriangles = 0;

	for (i=0; i < NUM_TERRAIN_RECTANGLES; i++)
	{
		if (activeTerrainDetail[i] == FALSE)
		{
			continue;
		}

		if (trndtl[i].terrain != nullptr &&
			trndtl[i].terrain->flags & OBJECT_VISIBLE)
		{
            if(trndtl[i].terrain->numtris > 0)
            {
			    if (rnvisFOVCheck(trndtl[i].terrain))
			    {
				    retval = DrawUpdate(trndtl[i].terrain);
				    if (!retval)
				    {
					    return(FALSE);
				    }
			    }
            }
		}

		for (j=0; j < MAX_TERRAIN_TEXTURES; j++)
		{
			if (trndtl[i].nonreplacementtiles[j] != nullptr)
			{
				if (trndtl[i].nonreplacementtiles[j]->flags & OBJECT_VISIBLE)
				{
                    if(trndtl[i].nonreplacementtiles[j]->numtris > 0)
                    {
					    if (rnvisFOVCheck(trndtl[i].nonreplacementtiles[j]))
					    {
						    //retval = obdrawDrawSingleObject(trndtl[i].txtobj[j]);
						    retval = DrawUpdate(trndtl[i].nonreplacementtiles[j]);
						    if (!retval)
						    {
							    return(FALSE);
						    }
					    }
                    }
				}
			}
		}
	}

#if USE_STATESET
#else
    piSetRenderState(piRENDERSTATE_ALPHABLENDENABLE, piALPHABLENDENABLE_DEFAULT);
	piSetRenderState(piRENDERSTATE_ZFUNC, ZFunction);
#endif
    // Does nothing unless compile switch set.
    trnclutterDrawDebug();

	return(TRUE);
}

void trnmainGetShadowReceivers(std::map<int, std::vector<piVECTOR> >& octets,
                               std::map<int, OBJECT*>& objects)
{
    octets = ShadowOctet;
    objects = ShadowObject;
}

SINT32 trnmainGetNumDrawnTriangles()
{
	return(NumDrawnTriangles);
}

LOGICAL trnmainSetLightingUpdate(LOGICAL newLightingUpdate)
{
	LightingUpdate = newLightingUpdate;
	TerrainLightingUpdate = newLightingUpdate;

	return(TRUE);
}

void trnmainKill()
{
int		i;
    trndataKill();

	trnskyKill();

	trnwtrKill();

	trncdKill();

	trnldrKill();

	for (i=0; i < _CurrentImportObjectCount; i++)
	{
		obpoolFreeObjectCallback(_ImportObjectList[i]);
	}

	_CurrentImportObjectCount = 0;

	// next line* MUST* occur after trnldrKill();
	trnKillAutogunSystem();

    Write_Log(LOG_OLD, LOG_INFO, "[trnmainKill]");
}

LOGICAL trnmainPositionToLatLong(FLOAT64 x, FLOAT64 y, SINT32* latdeg, SINT32* latmin, SINT32* latsec, SINT32* north, SINT32* longdeg, SINT32* longmin, SINT32* longsec, SINT32* east)
{
LOGICAL		retnorth, reteast;
SINT32		deg, min, sec;

	if (x < 0)
	{
		x = fabs(x);
		reteast = FALSE;
	}
	else
	{
		reteast = TRUE;
	}

	x = x / WORLD_WIDTH * WORLD_ARC_WIDTH;

	// Longitude (X)
	deg = (SINT32)floor(x);
	x = modf(x, &x) * 60.0;
	min = (SINT32)floor(x);
	x = modf(x, &x) * 60.0;
	sec = (SINT32)floor(x);

	if (!east)
	{
		*longdeg = -deg;
	}
	else
	{
		*longdeg = deg;
	}

	*longmin = min;
	*longsec = sec;
	*east = reteast;

	if (y < 0)
	{
		y = fabs(y);
		retnorth = FALSE;
	}
	else
	{
		retnorth = TRUE;
	}

	y = y / WORLD_HEIGHT * WORLD_ARC_HEIGHT;

	// Latitude (Y)
	deg = (SINT32)floor(y);
	y = modf(y, &y) * 60.0;
	min = (SINT32)floor(y);
	y = modf(y, &y) * 60.0;
	sec = (SINT32)floor(y);

	if (!retnorth)
	{
		*latdeg = -deg;
	}
	else
	{
		*latdeg = deg;
	}

	*latmin = min;
	*latsec = sec;
	*north = retnorth;

	return(TRUE);
}

LOGICAL trnmainLatLongToPosition(SINT32 latdeg, SINT32 latmin, SINT32 latsec, SINT32 north, SINT32 longdeg, SINT32 longmin, SINT32 longsec, SINT32 east, FLOAT64* x, FLOAT64* y)
{
FLOAT64		newlat, newlong;

	if (latdeg < 0 || latmin < 0 || latsec < 0)
	{
		latdeg = abs(latdeg);
		latmin = abs(latmin);
		latsec = abs(latsec);
	}

	// Latitude (Y)
	newlat = (FLOAT64)latsec / 3600.0;
	newlat += (FLOAT64)latmin / 60.0;
	newlat += (FLOAT64)latdeg;
	if (!north)
	{
		newlat = -newlat;
	}

	if (longdeg < 0 || longmin < 0 || longsec < 0)
	{
		longdeg = abs(longdeg);
		longmin = abs(longmin);
		longsec = abs(longsec);
	}

	// Longitude (X)
	newlong = (FLOAT64)longsec / 3600.0;
	newlong += (FLOAT64)longmin / 60.0;
	newlong += (FLOAT64)longdeg;

	if (!east)
	{
		newlong = -newlong;
	}

	// Result
	*x = newlong / WORLD_ARC_WIDTH * WORLD_WIDTH;
	*y = newlat / WORLD_ARC_HEIGHT * WORLD_HEIGHT;

	return(TRUE);
}

LOGICAL trnmainAdjustTerrainSections(piVECTOR* CameraAdjustment)
{
SINT32		i, j;
OBJECT*	lpobj;
piMATRIX*	lpmat;

	for (i=0; i < NUM_TERRAIN_RECTANGLES; i++)
	{
		for (j=0; j < MAX_TERRAIN_TEXTURES; j++)
		{
			if (trndtl[i].nonreplacementtiles[j] != nullptr)
			{
				lpobj = trndtl[i].nonreplacementtiles[j];
				lpmat = &lpobj->mat;
				lpmat->_41 += CameraAdjustment->x;
				lpmat->_42 += CameraAdjustment->y;
				lpmat->_43 += CameraAdjustment->z;
			}
		}

		if (trndtl[i].terrain->numtris != 0)
		{
			lpobj = trndtl[i].terrain;
			lpmat = &lpobj->mat;
			lpmat->_41 += CameraAdjustment->x;
			lpmat->_42 += CameraAdjustment->y;
			lpmat->_43 += CameraAdjustment->z;
		}

		if (trndtl[i].terrainorig != nullptr &&
			trndtl[i].terrainorig->numtris != 0)
		{
			lpobj = trndtl[i].terrainorig;
			lpmat = &lpobj->mat;
			lpmat->_41 += CameraAdjustment->x;
			lpmat->_42 += CameraAdjustment->y;
			lpmat->_43 += CameraAdjustment->z;
		}
	}

	return(TRUE);
}

// gets the current 64bit world offset
LOGICAL trnGetWorldOriginOffset(FLOAT64VECTOR* retWorldOriginOffset)
{
	*retWorldOriginOffset = WorldOriginOffset;

	return(TRUE);
}

// checks to see that the camera isn't too far from the relative origin
// and if it is, adjust it (and the world) so that it is no longer too far
//
// returns a TRUE if an adjustment happened, otherwise a FALSE
LOGICAL trnCheckAndAdjustCameraPositionWithOriginOffset(piVECTOR * CameraPos)
{
LOGICAL		RetVal;
LOGICAL		PositionOk;
piVECTOR	CameraAdjustment;

	CameraAdjustment.x = CameraAdjustment.y = CameraAdjustment.z = 0;

	PositionOk = TRUE;

	if (fabs(CameraPos->x) > MAX_CAMERA_OFFSET)
	{
		CameraAdjustment.x = CameraPos->x;
		PositionOk = FALSE;
	}

	if (fabs(CameraPos->y) > MAX_CAMERA_OFFSET)
	{
		CameraAdjustment.y = CameraPos->y;
		PositionOk = FALSE;
	}

	RetVal = FALSE;
	if (PositionOk == FALSE)
	{
		RetVal = TRUE;

		_TerrainMoved = TRUE;

		_CameraAdjustment = CameraAdjustment;
	}

	return RetVal;
}

UINT32 trnUpdateTerrainOriginOffset(LOGICAL force)
{
	if (_TerrainMoved || force)
	{

		mathSubVectors(&WorldOriginOffset,&WorldOriginOffset,&_CameraAdjustment);

#ifdef BETA_RELEASE
		draw3dUpdateDebugPositions(_CameraAdjustment);
#endif

		// now adjust the terrain
		trnmainAdjustTerrainSections(&_CameraAdjustment);
		trncdAdjustTerrainObjects(&_CameraAdjustment);

		_TerrainMoved = FALSE;

		_WorldMoved = TRUE;
	}
	else
	{
		_WorldMoved = FALSE;
	}

	return 0;
}

LOGICAL trnmainWorldOriginMoved()
{
	return _WorldMoved;
}

LOGICAL trnFindStratBuildingByTO(BUILDING** const bld, trnCDTerrainObject* const TO)
{
SINT32 i,j;
TERRAIN_DETAIL*	td;
BUILDING* pbld;

	for (j=0;j<25;j++)
	{
		td = &trndtl[j];

		if (td->scx == TO->StratSupercellX &&
			td->scy == TO->StratSupercellY)
		{

			i = 0;
			pbld = td->pbld;
			while (i < td->numBuildings)
			{
				if (pbld->oid.id == TO->ObjId.id &&
					pbld->oid.objClass == TO->ObjId.objClass &&
					pbld->oid.objType == TO->ObjId.objType)
				{
					*bld = pbld;
					return TRUE;
				}

				pbld += 1;
				i += 1;
			}

		}
	}

	// not found
	return FALSE;
}

/*------------------------------------------------------------------
Function: trnmainGetSupercellAndOctet
Description: Returns your current supercell and octet.
Octet row/column is 0,0 at bottom left and 15,15 at top right. Pos
is world position.
-------------------------------------------------------------------*/
void trnmainGetSupercellAndOctet(const Vec3d& pos, int& sx, int& sy, int& r, int& c)
{
    // Get supercell
    sx = (int)(pos.x() / SDM_SUPERCELL_SIZE);
    sy = (int)(pos.y() / SDM_SUPERCELL_SIZE);

    sx = pos.x() < 0 ? sx - 1 : sx;
    sy = pos.y() < 0 ? sy - 1 : sy;

    piVECTOR v;

    // Get position within a supercell
    v.x = float(pos.x() - sx * SDM_SUPERCELL_SIZE);
    v.y = float(pos.y() - sy * SDM_SUPERCELL_SIZE);

    // Determine row/column in the supercell to octet level
    c = (int)(v.x / SDM_OCTET_SIZE);
    r = (int)(v.y / SDM_OCTET_SIZE);
}

#if USE_HEIGHT_AT
PLANE* trnmainGetPlaneAt(TERRAIN_DETAIL* dtl, double x, double y)
{
    short sx = -1, sy = -1;

    if(!dtl)
    {
        // First find the supercell and then determine the row/colum of our octet
        sx = (int)(x / SDM_SUPERCELL_SIZE);
        sy = (int)(y / SDM_SUPERCELL_SIZE);

        sx = sx < 0 ? sx - 1 : sx;
        sy = sy < 0 ? sy - 1 : sy;

        for(int q = 0; q < NUM_TERRAIN_RECTANGLES; q++)
	    {
            if(activeTerrainDetail[q] && trndtl[q].scx == sx && trndtl[q].scy == sy)
            {
                dtl = &trndtl[q];
                break;
            }
	    }
    }

    if(!dtl || !dtl->terrainorig)
        return nullptr;

    if(!(dtl->flags & TDFLAG_PLANES_CREATED))
    {
        extern void FindPlaneEquations(OBJECT* o, TERRAIN_DETAIL* td);

        FindPlaneEquations(dtl->terrainorig, dtl);
        dtl->flags |= TDFLAG_PLANES_CREATED;
    }

    sx = dtl->scx;
    sy = dtl->scy;

	int i = 0;

    // Get position within a supercell
    float xx, yy;

    xx = float(x) - sx * SDM_SUPERCELL_SIZE;
    yy = float(y) - sy * SDM_SUPERCELL_SIZE;

    // Determine row/column in the supercell to octet level
    int c = (int)(xx / SDM_OCTET_SIZE);
    int r = (int)(yy / SDM_OCTET_SIZE);

    // Get index for this plane
    int plane = r * SUPERCELL_OCTETS_Y + c;

    // Move this octet to the origin.
    float xxx = xx - c * SDM_OCTET_SIZE - SDM_OCTET_SIZE_2;
    float yyy = yy - r * SDM_OCTET_SIZE - SDM_OCTET_SIZE_2;

	if(xxx < 0)
	{
		if(yyy > 0)
			i = (-xxx > yyy) ? 1 : 2;
		else
			i = (-xxx > -yyy) ? 0 : 7;
	}
    else
	{
		if(yyy > 0)
			i = (xxx > yyy) ? 4 : 3;
		else
			i = (xxx > -yyy) ? 5 : 6;
	}

    return &dtl->plane[plane][i];
}

float trnmainHeightAt(TERRAIN_DETAIL* dtl, double x, double y)
{
    short sx = -1, sy = -1;

    if(!dtl)
    {
        // First find the supercell and then determine the row/colum of our octet
        sx = (int)(x / SDM_SUPERCELL_SIZE);
        sy = (int)(y / SDM_SUPERCELL_SIZE);

        sx = sx < 0 ? sx - 1 : sx;
        sy = sy < 0 ? sy - 1 : sy;

        for(int q = 0; q < NUM_TERRAIN_RECTANGLES; q++)
	    {
            if(activeTerrainDetail[q] && trndtl[q].scx == sx && trndtl[q].scy == sy)
            {
                dtl = &trndtl[q];
                break;
            }
	    }
    }

    if(!dtl || !dtl->terrainorig)
        return 0.0f;

    if(!(dtl->flags & TDFLAG_PLANES_CREATED))
    {
        extern void FindPlaneEquations(OBJECT* o, TERRAIN_DETAIL* td);

        FindPlaneEquations(dtl->terrainorig, dtl);
        dtl->flags |= TDFLAG_PLANES_CREATED;
    }

    sx = dtl->scx;
    sy = dtl->scy;

	int i = 0;

    // Get position within a supercell
    float xx, yy;

    xx = float(x) - sx * SDM_SUPERCELL_SIZE;
    yy = float(y) - sy * SDM_SUPERCELL_SIZE;

    // Determine row/column in the supercell to octet level
    int c = (int)(xx / SDM_OCTET_SIZE);
    int r = (int)(yy / SDM_OCTET_SIZE);

    // Get index for this plane
    int plane = r * SUPERCELL_OCTETS_Y + c;

    // Move this octet to the origin.
    float xxx = xx - c * SDM_OCTET_SIZE - SDM_OCTET_SIZE_2;
    float yyy = yy - r * SDM_OCTET_SIZE - SDM_OCTET_SIZE_2;

	if(xxx < 0)
	{
		if(yyy > 0)
			i = (-xxx > yyy) ? 1 : 2;
		else
			i = (-xxx > -yyy) ? 0 : 7;
	}
    else
	{
		if(yyy > 0)
			i = (xxx > yyy) ? 4 : 3;
		else
			i = (xxx > -yyy) ? 5 : 6;
	}

    // Move to original position before the stupid world origin offset(s).
    x -= dtl->terrainorig->mat._41;
    y -= dtl->terrainorig->mat._42;

    x -= (float)WorldOriginOffset.x;
    y -= (float)WorldOriginOffset.y;

    // Get our plane height at this point
    float zz = dtl->plane[plane][i].Z((float)x, (float)y);

    return zz;
}
#endif
