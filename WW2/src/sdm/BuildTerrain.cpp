#include <syscfg.h> // Path is project specific.
#if platOS == OS_WIN32
#include <windows.h>
#include <commctrl.h>
#include <CommDlg.h>
#include <mmsystem.h>
#include <ShellAPI.h>
#include <objbase.h>
#include <shlobj.h>
#endif

#include <iostream>
#include <fstream>
#include <sstream>
#if platOS == OS_WIN32
#include <tuple>
#include <hash_map>
#else
#include <tr1/tuple>
#include <map>
#endif
#include <iomanip>

#include "sdm/BuildTerrain.h"
#include "include/systypes.h"
#include "dbug/dbug.h"
#include "strt/strtDTL.h"
#include "pi/piogl.h"
#include "sdm/uistrat.h"
#include "trn/trntypes.h"
#include "trn/trnldr.h"
#include "sdm/sdm.h"
#include "ob/obMgr.h"
#include "ob/obpool.h"
#include "math/math3d.h"
#include "math/mathnum.h"
#include "math/mathUnitConversion.h"
#include "math/Matrixd.h"
#include "math/Matrixf.h"
#include "ml/mltypes.h"
#include "physics/physicsmain.h"
#include "ld/ldmdl.h"
#include "ob/obplyr.h"
#include "trn/trnvars.h"
#include "rn/rnlogo.h"
#include "spt/sptmain.h"
#include "trn/trncd.h"

using namespace std;
using namespace tr1;

#if platOS == OS_MACOS

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

typedef struct WIN32_FIND_DATAA
{
	struct dirent* ent;
	char cFileName[256];
} WIN32_FIND_DATAA;

typedef struct HANDLE
{
  DIR* dir;
  const char* pattern;
} HANDLE;

static bool endsmatch( const char* s, const char* m )
{
  int sl = strlen(s);
  int ml = strlen(m);

  if ( ml < sl )
  {
    const char* sp = s+sl;
    const char* mp = m+ml;
    while ( mp != m )
    {
      if ( *sp-- != *mp-- )
      {
        return false;
      }
    }
    return true;
  }
  return false;
}

static bool findNextMatchingEntry( HANDLE&h, WIN32_FIND_DATAA* finddata )
{
  do
  {
    finddata->ent = readdir(h.dir);
    if ( finddata->ent == NULL )
    {
      return false;
    }

  } while ( ! endsmatch( finddata->ent->d_name, h.pattern ) );

  strncpy(finddata->cFileName, finddata->ent->d_name, 255);
  return true;
}

static HANDLE FindFirstFileA(const char*pattern, WIN32_FIND_DATAA* finddata)
{
  HANDLE h;

  char* slash = strrchr( pattern, '/' );
  *slash = NULL;
  strcpy( slash+2, h.pattern ); // skip *

  h.dir = opendir(pattern);
  if ( h.dir == 0 )
  {
    return h;
  }

  if ( ! findNextMatchingEntry(h, finddata ) )
  {
    closedir(h.dir);
    h.dir = 0;
    return h;
  }

  return h;
}

static bool FindNextFileA(HANDLE& h, WIN32_FIND_DATAA* finddata)
{
  return findNextMatchingEntry(h, finddata );
}

static void FindClose(HANDLE& h)
{
  if ( h.dir )
  {
    closedir(h.dir);
  }
}

#endif

typedef tuple<int, int, int>			StratObjId;
typedef pair<_DEPOTLINK*, _BUILDING*>	LinkCPFdata;
typedef pair<_WAYPOINT*, _BUILDING*>	WaypointCPFdata;

typedef map<StratObjId, _CP*>				ChokepointCPFmap;
typedef map<StratObjId, _FACILITY*>			FacilityCPFmap;
typedef map<StratObjId, _BUILDING*>			BuildingCPFmap;
typedef map<StratObjId, LinkCPFdata>		LinkCPFmap;
typedef map<StratObjId, WaypointCPFdata>	WaypointCPFmap;

static ChokepointCPFmap	chokepointCPFmap;
static FacilityCPFmap	facilityCPFmap;
static BuildingCPFmap	buildingCPFmap;
static LinkCPFmap		linkCPFmap;
static WaypointCPFmap	waypointCPFmap;

#define COLLIDE_MASK    COLLIDER_TRN_BUILDING | COLLIDER_TRN_BRIDGE | COLLIDER_TILE_MASK | COLLIDER_TRN_CULTURAL

struct ObjectId
{
	int			posX, posY;
	const char*	fileName;
	ObjectId(int _posX, int _posY, const char* _fileName) : posX(_posX), posY(_posY), fileName(_fileName) {}

	bool operator < (const ObjectId& rhv) const
	{
		if(posX != rhv.posX)	return posX < rhv.posX;
		if(posY != rhv.posY)	return posY < rhv.posY;
		return strcmp(fileName, rhv.fileName) < 0;
	}
};

typedef map<ObjectId, BUILDING*> BuildingMap;
typedef map<ObjectId, CP*> CPMap;
typedef map<ObjectId, FACILITY*> FacilityMap;

struct CellStratData
{
	BuildingMap buildings;
    CPMap chokepoints;
    FacilityMap facilities;

	vector<DEPOT_LINK*>			links;
	vector<WAYPOINT*>			waypoints;

	// We need that for inserting sub-model references
	int	maxStratId;

    CellStratData()
    {
        maxStratId = 0;
    }
};

typedef map<pair<int, int>, CellStratData>	CellStratMap;
static CellStratMap		cellStratMap;

struct TerrainPosition
{
	int		supercellX, supercellY;
	int		octetX, octetY;
	Vec3f	worldPosition;			// Absolute position
	Vec3f	cellPosition;			// Relative to the supercell origin

	void Set(const Vec3f& absolutePosition);
	TerrainPosition(const Vec3f& absolutePosition)	{Set(absolutePosition);}
	TerrainPosition() : supercellX(), supercellY(), octetX(), octetY()	{}
};

struct ModelStats
{
	int				totalInstanceCount;
	int				steepPlaces;
	int				nonPlanarCount;
	vector<float>	slopeDeltaList;

	ModelStats() : totalInstanceCount(0), steepPlaces(0), nonPlanarCount(0)	{}
	ModelStats(const ModelStats& draft) : totalInstanceCount(draft.totalInstanceCount), steepPlaces(draft.totalInstanceCount), nonPlanarCount(draft.totalInstanceCount)	{}
};
typedef map<const PoolModel*, ModelStats>	ModelStatsMap;
static ModelStatsMap modelStatsMap;

#if platOS == OS_MACOS
static const string		dataPath	= "/Users/ahwulf/ww2data/baseline/DATA/";
static const string		dtlCpfDir	= "rawTerrain/";
static const string		sdmDir		= "Terrain/";
#else
static const string		dataPath	= "Data\\";
static const string		dtlCpfDir	= "rawTerrain\\";
static const string		sdmDir		= "Terrain\\";
#endif

static dSpaceID			worldPhysicsSpace;
static fstream			logFile;
static stringstream		allModelProblems;
static stringstream		stratErrors;
static stringstream		dtlErrors;
static set<PoolModel*>	missingTopBases;
static set<PoolModel*>	missingBottomBases;
static set<string>		missingModels;
static streampos		streamUpdatePos;
static int				sdmFilesMerged;

static void MergeCPFdata()
{
	logFile << "Merging Strat Data...";

	for(ChokepointCPFmap::iterator it = chokepointCPFmap.begin(); it != chokepointCPFmap.end(); ++it)
	{
		_CP*	cpCPF	= it->second;
		CP*		cpSDM	= (CP*)memAlloc(sizeof(CP));


		strcpy(cpSDM->fileName,	cpCPF->objname);
		strcpy(cpSDM->name, cpCPF->name);
		cpSDM->oid.id		= cpCPF->cpid.id;
		cpSDM->oid.objClass	= CLASS_CP;
		cpSDM->oid.objType	= cpCPF->ctype;
		cpSDM->serial		= cpCPF->serial;
		cpSDM->posx			= cpCPF->posx;
		cpSDM->posy			= cpCPF->posy;
		cpSDM->subType		= 0;
		cpSDM->side			= cpCPF->side;
		cpSDM->country		= cpCPF->country;
		cpSDM->stratFlags	= (UINT8)cpCPF->flags;

		CellStratData*	cellStratData = &cellStratMap[pair<int, int>(cpCPF->cpid.scx, cpCPF->cpid.scy)];

        if(cellStratData->chokepoints.insert(pair<ObjectId, CP*>(ObjectId(cpSDM->posx, cpSDM->posy, cpSDM->fileName), cpSDM)).second == false)
		{
			stratErrors << "Duplicate strat chokepoint: modelName(" << cpSDM->fileName << "); superCell(" << cpCPF->cpid.scx << "," << cpCPF->cpid.scy << "); position(" << cpSDM->posx << ","  << cpSDM->posy << ")" << endl;
		}
		else if(cellStratData->maxStratId < cpSDM->oid.id)
            cellStratData->maxStratId = cpSDM->oid.id;
	}

	for(FacilityCPFmap::iterator it = facilityCPFmap.begin(); it != facilityCPFmap.end(); ++it)
	{
		_FACILITY*	facCPF	= it->second;
		FACILITY*	facSDM	= (FACILITY*)memAlloc(sizeof(FACILITY));


		ChokepointCPFmap::iterator	cpCPF	= chokepointCPFmap.find(StratObjId(facCPF->cpid.scx, facCPF->cpid.scy, facCPF->cpid.id));
		if(cpCPF == chokepointCPFmap.end())
		{
			logFile << endl << "fatal error: facility(" << facCPF->fid.scx << "," << facCPF->fid.scy << "," << facCPF->fid.id << ") refers to inexistent CP!" << endl;
			piExitProgram (__FILE__, __LINE__, "As described in terrainBuildLog.txt.");
		}

		strcpy(facSDM->fileName, facCPF->objname);
		strcpy(facSDM->name, facCPF->name);
		facSDM->oid.id			= facCPF->fid.id;
		facSDM->oid.objClass	= CLASS_FACILITY;
		facSDM->oid.objType		= facCPF->ftype;
		facSDM->serial			= facCPF->serial;
		facSDM->posx			= facCPF->posx;
		facSDM->posy			= facCPF->posy;
		facSDM->subType			= facCPF->fsubtype;
		facSDM->originalSide	= facCPF->side;
		facSDM->originalCountry	= facCPF->country;
		facSDM->stratFlags		= (UINT8)facCPF->flags;
		facSDM->cpid.Populate(cpCPF->second, cpCPF->second->cpid.id, CLASS_CP, cpCPF->second->ctype) ;

		CellStratData*	cellStratData = &cellStratMap[pair<int, int>(facCPF->fid.scx, facCPF->fid.scy)];
		if(cellStratData->facilities.insert(pair<ObjectId, FACILITY*>(ObjectId(facSDM->posx, facSDM->posy, facSDM->fileName), facSDM)).second == false)
		{
			stratErrors << "Duplicate strat facility: modelName(" << facSDM->fileName << "); superCell(" << facCPF->fid.scx << "," << facCPF->fid.scy << "); position(" << facSDM->posx << ","  << facSDM->posy << ")" << endl;
		}
		else if(cellStratData->maxStratId < facSDM->oid.id)
            cellStratData->maxStratId = facSDM->oid.id;
    }


	for(BuildingCPFmap::iterator it = buildingCPFmap.begin(); it != buildingCPFmap.end(); ++it)
	{
		_BUILDING*	bldCPF	= it->second;
		BUILDING*	bldSDM	= (BUILDING*)memAlloc(sizeof(BUILDING));


		FacilityCPFmap::iterator	facCPF	= facilityCPFmap.find(StratObjId(bldCPF->fid.scx, bldCPF->fid.scy, bldCPF->fid.id));
		if(facCPF == facilityCPFmap.end())
		{
			logFile << endl << "fatal error: building(" << bldCPF->bid.scx << "," << bldCPF->bid.scy << "," << bldCPF->bid.id << ") refers to inexistent facility!" << endl;
			piExitProgram (__FILE__, __LINE__, "As described in terrainBuildLog.txt.");
		}

		strcpy(bldSDM->fileName, bldCPF->objname);
		strcpy(bldSDM->name, bldCPF->name);
		bldSDM->oid.id			= bldCPF->bid.id;
		bldSDM->oid.objClass	= CLASS_BUILDING;
		bldSDM->oid.objType		= bldCPF->btype;
		bldSDM->serial			= bldCPF->serial;
		bldSDM->posx			= bldCPF->posx;
		bldSDM->posy			= bldCPF->posy;
		bldSDM->subType			= bldCPF->bsubtype;
//		bldSDM->control			= bldCPF->control;
		bldSDM->stratFlags		= (UINT8)bldCPF->flags;

		bldSDM->pid.id			= facCPF->second->fid.id;
		bldSDM->pid.objClass	= CLASS_BUILDING;
		bldSDM->pid.objType		= facCPF->second->ftype;

		CellStratData*	cellStratData = &cellStratMap[pair<int, int>(bldCPF->bid.scx, bldCPF->bid.scy)];
		if(cellStratData->buildings.insert(pair<ObjectId, BUILDING*>(ObjectId(bldSDM->posx, bldSDM->posy, bldSDM->fileName), bldSDM)).second == false)
		{
			stratErrors << "Duplicate strat building: modelName(" << bldSDM->fileName << "); superCell(" << bldCPF->bid.scx << "," << bldCPF->bid.scy << "); position(" << bldSDM->posx << ","  << bldSDM->posy << ")" << endl;
		}
		else if(cellStratData->maxStratId < bldSDM->oid.id)
            cellStratData->maxStratId = bldSDM->oid.id;
	}

	for(LinkCPFmap::iterator it = linkCPFmap.begin(); it != linkCPFmap.end(); ++it)
	{
		_DEPOTLINK*	linkCPF	= it->second.first;
		DEPOT_LINK*	linkSDM	= (DEPOT_LINK*)memAlloc(sizeof(DEPOT_LINK));


		strcpy(linkSDM->name, linkCPF->name);
		linkSDM->oid.id					= linkCPF->lid.id;
		linkSDM->oid.objClass			= CLASS_LINK;
		linkSDM->oid.objType			= 0;
		linkSDM->serial					= linkCPF->serial;
		linkSDM->stratFlags				= (UINT8)linkCPF->flags;

		FacilityCPFmap::iterator	facMapIt = facilityCPFmap.find(StratObjId(linkCPF->fbid.scx, linkCPF->fbid.scy, linkCPF->fbid.id));
		if(facMapIt != facilityCPFmap.end())
		{
			linkSDM->firebaseId.Populate(facMapIt->second, facMapIt->second->fid.id, CLASS_FACILITY, facMapIt->second->ftype) ;
		}
		else
		{
			//logFile << endl << "fatal error: link(" << linkCPF->lid.scx << "," << linkCPF->lid.scy << "," << linkCPF->lid.id << ") refers to inexistent firebase!" << endl;
			//piExitProgram (__FILE__, __LINE__, "As described in terrainBuildLog.txt.");
		}

		BuildingCPFmap::iterator	startBldCPF	= buildingCPFmap.find(StratObjId(linkCPF->lbid.scx, linkCPF->lbid.scy, linkCPF->lbid.id));
		if(startBldCPF == buildingCPFmap.end())
		{
			logFile << endl << "fatal error: link(" << linkCPF->lid.scx << "," << linkCPF->lid.scy << "," << linkCPF->lid.id << ") refers to inexistent start depot!" << endl;
			piExitProgram (__FILE__, __LINE__, "As described in terrainBuildLog.txt.");
		}

		linkSDM->startDepotId.Populate(startBldCPF->second, linkCPF->lbid.id, CLASS_BUILDING, startBldCPF->second->btype) ;

		BuildingCPFmap::iterator	endBldCPF	= buildingCPFmap.find(StratObjId(linkCPF->bid.scx, linkCPF->bid.scy, linkCPF->bid.id));
		if(endBldCPF == buildingCPFmap.end())
		{
			logFile << endl << "fatal error: link(" << linkCPF->lid.scx << "," << linkCPF->lid.scy << "," << linkCPF->lid.id << ") refers to inexistent end depot!" << endl;
			piExitProgram (__FILE__, __LINE__, "As described in terrainBuildLog.txt.");
		}
		linkSDM->endDepotId.Populate(endBldCPF->second, endBldCPF->second->bid.id, CLASS_BUILDING, endBldCPF->second->btype) ;

		if(linkCPF->waypoint)
		{
			WaypointCPFmap::iterator	wptCPF	= waypointCPFmap.find(StratObjId(linkCPF->waypoint->wpid.scx, linkCPF->waypoint->wpid.scy, linkCPF->waypoint->wpid.id));
			if(wptCPF == waypointCPFmap.end())
			{
				logFile << endl << "fatal error: link(" << linkCPF->lid.scx << "," << linkCPF->lid.scy << "," << linkCPF->lid.id << ") refers to inexistent waypoint!" << endl;
				piExitProgram (__FILE__, __LINE__, "As described in terrainBuildLog.txt.");
			}
			float dx			= (float)(wptCPF->second.first->posx - endBldCPF->second->posx);
			float dy			= (float)(wptCPF->second.first->posy - endBldCPF->second->posy);
			linkSDM->distance	= (int)sqrt(dx * dx + dy * dy);
		}
		else
		{
			float dx			= (float)(endBldCPF->second->posx - it->second.second->posx);
			float dy			= (float)(endBldCPF->second->posy - it->second.second->posy);
			linkSDM->distance	= (int)sqrt(dx * dx + dy * dy);
		}

		CellStratData*	cellStratData = &cellStratMap[pair<int, int>(linkCPF->lid.scx, linkCPF->lid.scy)];
		cellStratData->links.push_back(linkSDM);
		if(cellStratData->maxStratId < linkSDM->oid.id)
            cellStratData->maxStratId = linkSDM->oid.id;
	}


	for(WaypointCPFmap::iterator it = waypointCPFmap.begin(); it != waypointCPFmap.end(); ++it)
	{
		_WAYPOINT*	wptCPF	= it->second.first;
		WAYPOINT*	wptSDM	= (WAYPOINT*)memAlloc(sizeof(WAYPOINT));


		wptSDM->oid.id			= wptCPF->wpid.id;
		wptSDM->oid.objClass	= CLASS_WAYPOINT;
		wptSDM->oid.objType		= wptCPF->type;
		if(wptCPF->flags & WAYPOINT_FLAG_BRIDGE)
		{
			wptSDM->pid.id			= wptCPF->cpid.id;
			wptSDM->pid.objClass	= CLASS_CP;
			wptSDM->pid.objType		= 0;
			wptSDM->subType			= WAYPOINT_BRIDGE;
		}
		else
		{
			wptSDM->pid.id			= 0;
			wptSDM->pid.objClass	= 0;
			wptSDM->pid.objType		= 0;
			wptSDM->subType			= WAYPOINT_NORMAL;
		}

		wptSDM->posx				= wptCPF->posx;
		wptSDM->posy				= wptCPF->posy;
		if(wptCPF->next != NULL)
		{
			wptSDM->nextpt.Populate(wptCPF->next, wptCPF->next->wpid.id, CLASS_WAYPOINT, 0) ;
			FLOAT64	dx = (FLOAT64)wptCPF->next->posx - (FLOAT64)wptSDM->posx;
			FLOAT64	dy = (FLOAT64)wptCPF->next->posy - (FLOAT64)wptSDM->posy;
			wptSDM->distance = (SINT32)sqrt(dx * dx + dy * dy);
		}
		else
		{
			LinkCPFmap::iterator	linkIt = linkCPFmap.find(StratObjId(wptCPF->lid.scx, wptCPF->lid.scy, wptCPF->lid.id));
			if(linkIt == linkCPFmap.end())
			{
				logFile << endl << "fatal error: waypoint(" << wptCPF->wpid.scx << "," << wptCPF->wpid.scy << "," << wptCPF->wpid.id << ") refers to inexistent link!" << endl;
				piExitProgram (__FILE__, __LINE__, "As described in terrainBuildLog.txt.");
			}

			BuildingCPFmap::iterator	bldIt = buildingCPFmap.find(StratObjId(linkIt->second.first->lbid.scx, linkIt->second.first->lbid.scy, linkIt->second.first->lbid.id));
			if(bldIt == buildingCPFmap.end())
			{
				logFile << endl << "fatal error: link(" << linkIt->second.first->lbid.scx << "," << linkIt->second.first->lbid.scy << "," << linkIt->second.first->lbid.id << ") refers to inexistent building!" << endl;
				piExitProgram (__FILE__, __LINE__, "As described in terrainBuildLog.txt.");
			}

			wptSDM->nextpt.Populate(bldIt->second, linkIt->second.first->lid.id, CLASS_LINK, 0) ;
			FLOAT64 dx = (FLOAT64)bldIt->second->posx - (FLOAT64)wptCPF->posx;
			FLOAT64 dy = (FLOAT64)bldIt->second->posy - (FLOAT64)wptCPF->posy;
			wptSDM->distance = (SINT32)sqrt(dx * dx + dy * dy);
		}

		wptSDM->lid.Populate(it->second.second, wptCPF->lid.id, CLASS_LINK, 0) ;
		wptSDM->serial			= wptCPF->serial;
		wptSDM->stratFlags		= (UINT8)wptCPF->flags;

		CellStratData*	cellStratData = &cellStratMap[pair<int, int>(wptCPF->wpid.scx, wptCPF->wpid.scy)];
		cellStratData->waypoints.push_back(wptSDM);
		if(cellStratData->maxStratId < wptSDM->oid.id)
            cellStratData->maxStratId = wptSDM->oid.id;
	}
	logFile << " done." << endl << endl;
}

// Brrrr... whatever!
static char* ParseNextArgument(char* buffer, char* argument)
{
char*			ptr;
int				i;
bool			quoted;

	if (buffer == NULL)
	{
		return(NULL);
	}

	ptr = buffer;

	// We skip leading spaces
	while (*ptr != 0 && isspace(*ptr))
	{
		ptr += 1;
	}

	quoted = *ptr == '\"';

	if (quoted)
	{
		ptr += 1;
		i = 0;
		do
		{
			argument[i++] = *ptr++;
		}
		while (*ptr != 0 && *ptr != '\"');
		argument[i] = 0;
		if (*ptr == '\"')
		{
			ptr += 1;
		}
	}
	else
	{
		i = 0;
		do
		{
			argument[i++] = *ptr++;
		}
		while (*ptr != 0 && !isspace(*ptr));
		argument[i] = 0;
	}

	// We skip trailing spaces
	while (*ptr != 0 && isspace(*ptr))
	{
		ptr += 1;
	}

	return(ptr);
}


static void LoadStratFile(const char* fileName)
{
	// This is basically the old cpf file loading code to the most part
	static _CP*		pcplist;
	FILE*			hfile = 0;
	char*			ptr, buffer[256], dummy[32];
	_CP*			pcp;
	_FACILITY*		pfac;
	_BUILDING*		pbld;
	_DEPOTLINK*		plink;
	_WAYPOINT*		pwaypoint;
	int				length;


	hfile = fopen(fileName, "rb");
	if (hfile == NULL)	return;

	Write_Debug("Loading CPF file %s", fileName);
	do
	{
		if (feof(hfile))	break;
		ptr = fgets(buffer, 255, hfile);
		if (ptr == NULL)	continue;
		if (buffer[0] == '#' ||	strncmp(buffer, "//", 2) == 0)	continue;

		if (strncmp(buffer, "CP", 2) == 0)
		{
			pcp = (_CP*)memAlloc(sizeof(_CP));

			ptr = ParseNextArgument(buffer, dummy);
			ptr = ParseNextArgument(ptr, pcp->name);
			ptr = ParseNextArgument(ptr, pcp->objname);
			length = sscanf(ptr, "%d %d %d %d %d %d %d %d %d %d %d\r",
							&pcp->trnum,
							&pcp->posx,
							&pcp->posy,
							&pcp->cpid.scx,
							&pcp->cpid.scy,
							&pcp->cpid.id,
							&pcp->side,
							&pcp->country,
							&pcp->ctype,
							&pcp->flags,
							&pcp->serial);
			if(length != 11)	{ Write_Debug("Unable to scan the input CP line"); }

			chokepointCPFmap[StratObjId(pcp->cpid.scx, pcp->cpid.scy, pcp->cpid.id)] = pcp;
		}
		else
		if (strncmp(buffer, "FACILITY", 8) == 0)
		{
			pfac = (_FACILITY*)memAlloc(sizeof(_FACILITY));

			ptr = ParseNextArgument(buffer, dummy);
			ptr = ParseNextArgument(ptr, pfac->name);
			ptr = ParseNextArgument(ptr, pfac->objname);
			length = sscanf(ptr, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\r",
							&pfac->trnum,
							&pfac->posx,
							&pfac->posy,
							&pfac->fid.scx,
							&pfac->fid.scy,
							&pfac->fid.id,
							&pfac->cpid.scx,
							&pfac->cpid.scy,
							&pfac->cpid.id,
							&pfac->side,
							&pfac->country,
							&pfac->ftype,
							&pfac->fsubtype,
							&pfac->flags,
							&pfac->serial);
			if(length != 15)	{ Write_Debug("Unable to scan the input Facility line"); }

			pfac->next = pcp->facility;
			pcp->facility = pfac;

			facilityCPFmap[StratObjId(pfac->fid.scx, pfac->fid.scy, pfac->fid.id)] = pfac;
		}
		else
		if (strncmp(buffer, "BUILDING", 8) == 0)
		{
			pbld = (_BUILDING*)memAlloc(sizeof(_BUILDING));

			ptr = ParseNextArgument(buffer, dummy);
			ptr = ParseNextArgument(ptr, pbld->name);
			ptr = ParseNextArgument(ptr, pbld->objname);
			length = sscanf(ptr, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d\r",
							&pbld->trnum,
							&pbld->posx,
							&pbld->posy,
							&pbld->bid.scx,
							&pbld->bid.scy,
							&pbld->bid.id,
							&pbld->fid.scx,
							&pbld->fid.scy,
							&pbld->fid.id,
							&pbld->control,
							&pbld->btype,
							&pbld->bsubtype,
							&pbld->flags,
							&pbld->serial);
			if (length != 14)	{ Write_Debug("Unable to scan the input Building line"); }

			pbld->next = pfac->building;
			pfac->building = pbld;

			buildingCPFmap[StratObjId(pbld->bid.scx, pbld->bid.scy, pbld->bid.id)] = pbld;
		}
		else
		if (strncmp(buffer, "LINK", 4) == 0)
		{
			plink = (_DEPOTLINK*)memAlloc(sizeof(_DEPOTLINK));

			ptr = ParseNextArgument(buffer, dummy);
			ptr = ParseNextArgument(ptr, plink->name);
			length = sscanf(ptr, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\r",
							&plink->lid.scx,
							&plink->lid.scy,
							&plink->lid.id,
							&plink->lcpid.scx,
							&plink->lcpid.scy,
							&plink->lcpid.id,
							&plink->lfid.scx,
							&plink->lfid.scy,
							&plink->lfid.id,
							&plink->lbid.scx,
							&plink->lbid.scy,
							&plink->lbid.id,
							&plink->cpid.scx,
							&plink->cpid.scy,
							&plink->cpid.id,
							&plink->fid.scx,
							&plink->fid.scy,
							&plink->fid.id,
							&plink->bid.scx,
							&plink->bid.scy,
							&plink->bid.id,
							&plink->fbid.scx,
							&plink->fbid.scy,
							&plink->fbid.id,
							&plink->distance,
							&plink->flags,
							&plink->serial);
			if (length != 27)	{ Write_Debug("Unable to scan the input Link line"); }

			plink->next = pbld->link;
			pbld->link = plink;

			linkCPFmap[StratObjId(plink->lid.scx, plink->lid.scy, plink->lid.id)] = LinkCPFdata(plink, pbld);
		}
		else
		if (strncmp(buffer, "WAYPOINT", 8) == 0)
		{
			pwaypoint =(_WAYPOINT*) memAlloc(sizeof(_WAYPOINT));

			ptr = ParseNextArgument(buffer, dummy);
			length = sscanf(ptr, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\r",
							&pwaypoint->posx,
							&pwaypoint->posy,
							&pwaypoint->wpid.scx,
							&pwaypoint->wpid.scy,
							&pwaypoint->wpid.id,
							&pwaypoint->lid.scx,
							&pwaypoint->lid.scy,
							&pwaypoint->lid.id,
							&pwaypoint->cpid.scx,
							&pwaypoint->cpid.scy,
							&pwaypoint->cpid.id,
							&pwaypoint->type,
							&pwaypoint->subtype,
							&pwaypoint->flags,
							&pwaypoint->serial);
			if (length != 15)	{ Write_Debug("Unable to scan the input Waypoint line"); }

			pwaypoint->next = plink->waypoint;
			plink->waypoint = pwaypoint;

			waypointCPFmap[StratObjId(pwaypoint->wpid.scx, pwaypoint->wpid.scy, pwaypoint->wpid.id)] = WaypointCPFdata(pwaypoint, pbld);
		}
	}
	while (TRUE);

	if (pcp != NULL)
	{
		pcp->next = pcplist;
		pcplist = pcp;
	}
	fclose(hfile);
}

static void LoadStratData()
{
#if platOS == OS_MACOS
	string				directory		= "/Users/ahwulf/ww2data/baseline/DATA/rawTerrain/";
	string				searchPattern	= "/Users/ahwulf/ww2data/baseline/DATA/rawTerrain/*.cpf";
#else
	string				directory		= "Data\\rawTerrain\\";
	string				searchPattern	= "Data\\rawTerrain\\*.cpf";
#endif
	WIN32_FIND_DATAA	finddata;
	HANDLE				search;


	logFile << "Loading Strat Data...";

	search = FindFirstFileA(searchPattern.c_str(), &finddata);
#if platOS == OS_MACOS
	if(search.dir == 0)	return;
#else
	if(search == INVALID_HANDLE_VALUE)	return;
#endif

	do
	{
		string	fileName = directory + finddata.cFileName;
		LoadStratFile(fileName.c_str());
	}
	while(FindNextFileA(search, &finddata));
	logFile << " done." << endl;

	MergeCPFdata();

	FindClose(search);

	for(ChokepointCPFmap::iterator it = chokepointCPFmap.begin(); it != chokepointCPFmap.end(); ++it)
    {
        memFree(it->second);
    }

    for(FacilityCPFmap::iterator it = facilityCPFmap.begin(); it != facilityCPFmap.end(); ++it)
    {
        memFree(it->second);
    }

    for(BuildingCPFmap::iterator it = buildingCPFmap.begin(); it != buildingCPFmap.end(); ++it)
    {
        memFree(it->second);
    }

    for(LinkCPFmap::iterator it = linkCPFmap.begin(); it != linkCPFmap.end(); ++it)
    {
        memFree(it->second.first);
    }

    for(WaypointCPFmap::iterator it = waypointCPFmap.begin(); it != waypointCPFmap.end(); ++it)
    {
        memFree(it->second.first);
    }

	chokepointCPFmap.clear();
	facilityCPFmap.clear();
	buildingCPFmap.clear();
	linkCPFmap.clear();
	waypointCPFmap.clear();
}

struct BushClutterInfo
{
    int octet;
    int base;
    float x;
    float y;
    float z;
};

class SuperCellTerrainData
{
public:
		struct TerrainObject
		{
			SDM_ObjInfo			objInfo;

			PoolModel*			poolModel;
			ModelInstance*		instance;
			const dSpaceID&		parentSpace;
			dSpaceID			objectPhysicsSpace;
			TerrainObject*		masterObject;
			bool				placeOntoMasterObject;

            int scx;
            int scy;

			struct StratData
			{
				CP*			cp;
				FACILITY*	facility;
				BUILDING*	building;
			}stratData;

			TerrainPosition							position;
			Matrixf									transform;

			void PlaceDown(OBJECT* o, PLANE* planes);
			void PlaceOnGround();
			void PlaceStaticBase();
			void PlaceDownRotate();

			bool PlaceSheared();
            bool PlaceConcave();

#if(0)
			const OBJECT*	GetObjectByComment(const char* comment, const OBJECT* o);
#endif
			void			LogId();
			void			AddPlacementStats();

			TerrainObject(const Matrixf& worldTransform, const Matrixf& cellTransform, const dSpaceID& octetPhysicsSpace);
			~TerrainObject();

		};

	private:

		typedef multimap<const PoolModel*, TerrainObject*>	ObjectList;

		struct Octet
		{
			TerrainPosition				position;
			dSpaceID					octetPhysicsSpace;
			TerrainObject*				replacementTile;
			vector<TerrainObject*>		objects;
			TERRAIN_INFO				editInfo;
		}octets[SDM_SUPERCELL_OCTETS_X][SDM_SUPERCELL_OCTETS_Y];


		TERRAINFILE		terrainHdr;

		short			heightMapElementCnt;
		short			heightMapShort[SUPERCELL_SIZE_XY];
		float			heightMap[SUPERCELL_SIZE_XY];		// heightMap[16][16] is at origin
		short			normalsCnt;
		short			normalShort[SUPERCELL_SIZE_XY];
		short			normals[SUPERCELL_SIZE_XY];

		int				nullTextureIdx;
		short			texNameCnt;
		char			texNames[32][MAX_TERRAIN_TEXTURES];

		short			editListCnt;
		TERRAIN_INFO	editList[MAX_TERRAIN_EDIT_LIST];

		short			objectCnt;
		DTL_TRNOBJ_INFO	objects[SDM_SUPERCELL_OCTETS_XY * MAX_TRNOBJ_OCTET];

		string			cellName;
		int				supercellX, supercellY;
		Matrixf			cellTransform;
		Vec3f			worldOrigin;

        TerrainObject*	cellHeightMap;
        PLANE* planes;
        OBJECT* newHeightMap;

        std::vector<struct BushClutterInfo> bushclutterdata;
        short basecluttercount[MaxBushes];
        float minheight;

        //struct ClutterBushHeader bushheader;

		dSpaceID		cellPhysicsSpace;

		TerrainObject*	AddTerrainObject(const char* name, const Matrixf& worldTransform, int ortdrg);
		TerrainObject*	AddSubObjects(TerrainObject* masterObject, const Matrixf& worldTransform);
#if(0)
		void			PlaceHeightMap(OBJECT* o);
#endif
		void			PlaceTerrain();

	public:

		SuperCellTerrainData(int x, int y, const Vec3f& offset);
		~SuperCellTerrainData();

		void Preprocess();
		void PlaceTerrainObjects(int pass);
        void PlaceClutter();
		void WriteToSDM();
};

typedef std::set<SuperCellTerrainData::TerrainObject*> ToDeleteTerrainObjects;
static ToDeleteTerrainObjects TerrainObjectsToDelete;

void TerrainPosition::Set(const Vec3f& absolutePos)
{
	supercellX			= (int)floor(absolutePos.x() / SDM_SUPERCELL_SIZE);
	supercellY			= (int)floor(absolutePos.y() / SDM_SUPERCELL_SIZE);
	octetX				= (int)((absolutePos.x() - (supercellX * SDM_SUPERCELL_SIZE)) / SDM_OCTET_SIZE);
	octetY				= (int)((absolutePos.y() - (supercellY * SDM_SUPERCELL_SIZE)) / SDM_OCTET_SIZE);
	worldPosition		= absolutePos;
	cellPosition.set(	absolutePos.x() - supercellX * SDM_SUPERCELL_SIZE,
						absolutePos.y() - supercellY * SDM_SUPERCELL_SIZE,
						absolutePos.z()	);
}

SuperCellTerrainData::TerrainObject::TerrainObject(const Matrixf& worldTransform, const Matrixf& cellTransform, const dSpaceID& octetPhysicsSpace)
				:	objInfo(),
					poolModel(0),
					instance(0),
					stratData(),
					position(worldTransform.getTrans()),
					transform(worldTransform),
					parentSpace(octetPhysicsSpace),
					objectPhysicsSpace(dHashSpaceCreate(octetPhysicsSpace)),
					masterObject(0),
					placeOntoMasterObject(false)
{
	transform.setTrans(position.cellPosition);	// We work in local space when merging
	transform *= cellTransform;
	transform.get(objInfo.transform);

    TerrainObjectsToDelete.insert(this);
}

SuperCellTerrainData::TerrainObject::~TerrainObject()
{
	dSpaceDestroy(objectPhysicsSpace);
}

#if(0)
const OBJECT* SuperCellTerrainData::TerrainObject::GetObjectByComment(const char* comment, const OBJECT* o)
{
	static const OBJECT*	objectFound		= 0;
	const OBJECT*			returnObject	= 0;


	if(o->comment && strstr(o->comment, comment))	objectFound = o;

	if(o->child && !objectFound)	GetObjectByComment(comment, o->child);
	if(o->attached && !objectFound)	GetObjectByComment(comment, o->attached);
	if(o->sibling && !objectFound)	GetObjectByComment(comment, o->sibling);

	if(o == o->modelInstance->rootObject)
	{
		returnObject = objectFound;
		objectFound = 0;
	}
	return returnObject;
}
#endif

void GetSurfaceContactCallback(void* data, dGeomID o1, dGeomID o2)
{
	dContactGeom	contact;
	dContactGeom*	returnContact	= (dContactGeom*)data;
	int				contanctCount	= 0;


	contanctCount = dCollide(o1, o2, 1, &contact, sizeof(dContactGeom));
	if(contanctCount != 0)
        *returnContact = contact;

	assert(contanctCount < 2);
}


bool GetSurfaceContact(dContactGeom& contact, unsigned long collideBits, const dSpaceID& space)
{
	const	float	undefined	= 9999;
			dGeomID	ray			= dCreateRay(0, 10000);

	contact.pos[2] = undefined;
	dGeomSetCategoryBits(ray, COLLIDER_VOID);
	dGeomSetCollideBits(ray, collideBits);
	dGeomRaySet(ray, contact.pos[0], contact.pos[1], 4000.0, 0, 0, -1);

	dSpaceCollide2(ray, (dxGeom*)space, &contact, &GetSurfaceContactCallback);

	dGeomDestroy(ray);

	if(contact.pos[2] == undefined)
	{
		contact.pos[2] = 0;
		contact.normal[0] = 0;
		contact.normal[1] = 0;
		contact.normal[2] = 1;

        return false;
	}

    return true;
}

void SuperCellTerrainData::TerrainObject::PlaceDown(OBJECT* o, PLANE* planes)
{
	if(!(o->flags & OBJECT_NOTOBJECT) && o->numverts != 0)
	{
		piVectorPtr		vertices		= o->MutablePositions();

        // This is the octet we want
        int currentoctet = position.octetY * SUPERCELL_OCTETS_Y + position.octetX;
        // This is where the triagles begin for that octet
        currentoctet *= SDM_TRIANGLES_PER_OCTET;

        // Center of this octet
        Vec3f center(position.octetX * SDM_OCTET_SIZE + SDM_OCTET_SIZE_2,
                     position.octetY * SDM_OCTET_SIZE + SDM_OCTET_SIZE_2,
                     0.0f);

        for(int i=0; i < o->numverts; i++)
		{
			Vec3f v1 = vertices[i];

			v1 = v1 * transform;

            Vec3f offset = v1 - center;

            int tri = 0;

	        if(offset.x() < 0)
	        {
		        if(offset.y() > 0)
			        tri = (-offset.x() > offset.y()) ? 1 : 2;
		        else
			        tri = (-offset.x() > -offset.y()) ? 0 : 7;
	        }
            else
	        {
		        if(offset.y() > 0)
			        tri = (offset.x() > offset.y()) ? 4 : 3;
		        else
			        tri = (offset.x() > -offset.y()) ? 5 : 6;
	        }

            float z = planes[currentoctet + tri].Z(v1.x(), v1.y());

            vertices[i].z += z;
		}
	}

	if(o->child)
        PlaceDown(o->child, planes);
	if(o->attached)
        PlaceDown(o->attached, planes);
	if(o->sibling)
        PlaceDown(o->sibling, planes);
}

#if(0)
void SuperCellTerrainData::PlaceHeightMap(OBJECT* o)
{
	if(!(o->flags & OBJECT_NOTOBJECT) && o->numverts != 0)
	{
		piVectorPtr		vertices		= o->MutablePositions();

		for(int i=0; i < o->numverts; i++)
		{
			Vec3f	vertex = vertices[i];

			int x = (int)(vertex.x() / 400);
			int y = (int)(vertex.y() / 400);

			vertices[i].z = heightMapShort[y * 32 + x];//heightMap[x][y];
			vertex = vertex * o->modelInstance->transform;
		}
	}

	if(o->child)	PlaceHeightMap(o->child);
	if(o->attached)	PlaceHeightMap(o->attached);
	if(o->sibling)	PlaceHeightMap(o->sibling);
}
#endif

void SuperCellTerrainData::TerrainObject::PlaceOnGround()
{
	dContactGeom	contact;
	Vec3f			centerPoint			= poolModel->bounds.center();
	Vec3f			centerPointWorld	= centerPoint * transform;

	contact.pos[0] = centerPointWorld.x();
	contact.pos[1] = centerPointWorld.y();

	GetSurfaceContact(contact, COLLIDE_MASK, worldPhysicsSpace);

	transform.setTrans(transform(3, 0), transform(3, 1), (float)-contact.pos[2] - poolModel->bounds.zMax());
}

void SuperCellTerrainData::TerrainObject::PlaceDownRotate()
{
    dContactGeom contact;
    Vec3f center = poolModel->bounds.center();

    Matrixf realtransform(transform);

    if(position.supercellX != scx || position.supercellY != scy)
    {
        // Make sure we are colliding with the correct supercell. Some objects overlap.
        realtransform(3, 0) = position.worldPosition.x() - scx * SDM_SUPERCELL_SIZE;
        realtransform(3, 1) = position.worldPosition.y() - scy * SDM_SUPERCELL_SIZE;
    }

    BoundingBox bounds;

    bool missingtopbase = false;

	if(!poolModel->GetSubObjectBoundsByName(&bounds, "topbase", 0))
	{
		if(!poolModel->GetSubObjectBoundsByComment(&bounds, "topbase", 0))
		{
            missingtopbase = true;
		}
	}

    if(!missingtopbase)
    {
        center = bounds.center();
    }

    Vec3f modelcenter = center * realtransform;

	contact.pos[0] = modelcenter.x();
	contact.pos[1] = modelcenter.y();

	GetSurfaceContact(contact, COLLIDE_MASK, worldPhysicsSpace);

    // Normal to rotate 'to'
    Vec3f to(0, 0, 0);

    // Won't work if surface is not 'planer'
    to += Vec3f((float)contact.normal[0], (float)contact.normal[1], (float)contact.normal[2]);

    Vec3f from(0, 0, 1);

    Matrixf rot;
    // Reversed because of our f-up'd coordinate system.
    rot.makeRotate(to, from);

    transform = Matrixf(realtransform.getRotate()) * rot;

    center = Vec3f(center.x(), center.y(), 0) * transform;

    contact.pos[0] = modelcenter.x();
	contact.pos[1] = modelcenter.y();

	GetSurfaceContact(contact, COLLIDE_MASK, worldPhysicsSpace);

	transform.setTrans(position.cellPosition.x(),
			           position.cellPosition.y(),
					  -float(contact.pos[2]) - center.z());
}


void SuperCellTerrainData::TerrainObject::PlaceStaticBase()
{
    BoundingBox topBaseBounds, bottomBaseBounds;
    dContactGeom contact;
    FLOAT32 minCollisionHeight = FLT_MAX;
    FLOAT32 maxCollisionHeight = -FLT_MAX;
    Vec3f corners[4];
    float marginDelta;
    bool bottomBase	= true;

    Matrixf realtransform(transform);

    if(position.supercellX != scx || position.supercellY != scy)
    {
        // Make sure we are colliding with the correct supercell. Some objects overlap.
        realtransform(3, 0) = position.worldPosition.x() - scx * SDM_SUPERCELL_SIZE;
        realtransform(3, 1) = position.worldPosition.y() - scy * SDM_SUPERCELL_SIZE;
    }

	if(!poolModel->GetSubObjectBoundsByName(&topBaseBounds, "topbase", 0))
	{
		if(!poolModel->GetSubObjectBoundsByComment(&topBaseBounds, "topbase", 0))
		{
			missingTopBases.insert(poolModel);
			return;
		}
	}

    // Going to place object from bottom of topbase
    float maxz = topBaseBounds.zMax();

	corners[0].set(topBaseBounds.xMin(), topBaseBounds.yMin(), -FLT_MAX);
	corners[1].set(topBaseBounds.xMax(), topBaseBounds.yMin(), -FLT_MAX);
	corners[2].set(topBaseBounds.xMax(), topBaseBounds.yMax(), -FLT_MAX);
    corners[3].set(topBaseBounds.xMin(), topBaseBounds.yMax(), -FLT_MAX);

	for(int i = 0; i < 4; i++)
	{
		corners[i] = corners[i] * realtransform;

		contact.pos[0] = corners[i].x();
		contact.pos[1] = corners[i].y();

		GetSurfaceContact(contact, COLLIDE_MASK, worldPhysicsSpace);

		if(contact.pos[2] > maxCollisionHeight)
            maxCollisionHeight = (float)contact.pos[2];

		if(contact.pos[2] < minCollisionHeight)
            minCollisionHeight = (float)contact.pos[2];
	}

    float z = maxCollisionHeight + maxz;

    transform.setTrans(transform.getTrans() + Vec3f(0, 0, -z));

	if(!poolModel->GetSubObjectBoundsByName(&bottomBaseBounds, "bottombase", 0))
	{
		if(!poolModel->GetSubObjectBoundsByComment(&bottomBaseBounds, "bottombase", 0))
		{
			missingBottomBases.insert(poolModel);
			bottomBase = false;
		}
	}

	if(bottomBase)
	{
		minCollisionHeight	= FLT_MAX;

	    corners[0].set(bottomBaseBounds.xMin(), bottomBaseBounds.yMin(), -FLT_MAX);
	    corners[1].set(bottomBaseBounds.xMax(), bottomBaseBounds.yMin(), -FLT_MAX);
	    corners[2].set(bottomBaseBounds.xMax(), bottomBaseBounds.yMax(), -FLT_MAX);
        corners[3].set(bottomBaseBounds.xMin(), bottomBaseBounds.yMax(), -FLT_MAX);

		for(int i = 0; i < 4; i++)
		{
			corners[i] = corners[i] * realtransform;

			contact.pos[0] = corners[i].x();
			contact.pos[1] = corners[i].y();

			GetSurfaceContact(contact, COLLIDE_MASK, worldPhysicsSpace);

			if(contact.pos[2] < minCollisionHeight)
                minCollisionHeight = (float)contact.pos[2];
		}

		marginDelta = maxCollisionHeight - minCollisionHeight - bottomBaseBounds.zMax();
	}
	else
        marginDelta = maxCollisionHeight - minCollisionHeight - topBaseBounds.zMax();

	if(marginDelta > 0.1)
	{
		dtlErrors << "Base is " << marginDelta << " units too short: ";
		LogId();
	}
}


bool SuperCellTerrainData::TerrainObject::PlaceSheared()
{
	dContactGeom	contact;
	Vec3f			centerPoint			= poolModel->bounds.center();

    Matrixf realtransform(transform);

    if(position.supercellX != scx || position.supercellY != scy)
    {
        // Make sure we are colliding with the correct supercell. Some objects overlap.
        realtransform(3, 0) = position.worldPosition.x() - scx * SDM_SUPERCELL_SIZE;
        realtransform(3, 1) = position.worldPosition.y() - scy * SDM_SUPERCELL_SIZE;
    }

	Vec3f			centerPointWorld	= centerPoint * realtransform;
	Matrixf			shearTransform;
	float			incline;
	Vec3f			corners[4];

	contact.pos[0] = centerPointWorld.x();
	contact.pos[1] = centerPointWorld.y();

	if(!GetSurfaceContact(contact, COLLIDE_MASK, worldPhysicsSpace))
        return false;

    OBJECT* o = (OBJECT*)dGeomGetData(contact.g2);

	if(	poolModel->modelType != ModelTypes::TERRAIN_OBJECT	&&
		poolModel->modelType != ModelTypes::TERRAIN_TILE)
	{
		incline = (float)RAD_TO_DEG(acos(fabs(contact.normal[2])));
		if(incline > 5)
		{
			dtlErrors << "Too steep incline of " << incline << ": ";
			LogId();
		}
	}

	shearTransform(0,2) = (float)(contact.normal[0] / contact.normal[2]);
	shearTransform(1,2) = (float)(contact.normal[1] / contact.normal[2]);

	realtransform = Matrixf(realtransform.getRotate()) * shearTransform;

	// Compensate shearing for the z-axis translation.
	Vec3f	centerPointSheared = Vec3f(centerPoint.x(), centerPoint.y(), 0) * realtransform;
	contact.pos[0] = centerPointWorld.x();
	contact.pos[1] = centerPointWorld.y();
	GetSurfaceContact(contact, COLLIDE_MASK, worldPhysicsSpace);

	realtransform.setTrans(	position.cellPosition.x(),
						position.cellPosition.y(),
						(float)-contact.pos[2] - centerPointSheared.z() );

    transform.set(realtransform);

	// Check if this place is planar
	corners[0].set(poolModel->bounds.xMin(), poolModel->bounds.yMin(), 0);
	corners[1].set(poolModel->bounds.xMax(), poolModel->bounds.yMax(), 0);
	corners[2].set(corners[0].x(), corners[1].y(), 0);
	corners[3].set(corners[1].x(), corners[0].y(), 0);

	for(int i=0; i < 4; i++)
	{
		corners[i] = corners[i] * realtransform;
		contact.pos[0] = corners[i].x();
		contact.pos[1] = corners[i].y();

		GetSurfaceContact(contact, COLLIDE_MASK, worldPhysicsSpace);

        double z1 = fabs(contact.pos[2]);
        double z2 = fabs(corners[i].z());

        double radius = poolModel->bounds.radius();
        radius /= 1000;
        radius += 0.2f;

		if(z1 - z2 > radius)
		{
			if(poolModel->modelType != ModelTypes::TERRAIN_OBJECT)
			{
				dtlErrors << "Non-planar placement: ";
				LogId();
			}
		}
	}

    return true;
}


bool SuperCellTerrainData::TerrainObject::PlaceConcave()
{
	if(!PlaceSheared())
        return false;

	dContactGeom	contact;
	Vec3f			corners[4];

    Matrixf realtransform(transform);

    if(position.supercellX != scx || position.supercellY != scy)
    {
        // Make sure we are colliding with the correct supercell. Some objects overlap.
        realtransform(3, 0) = position.worldPosition.x() - scx * SDM_SUPERCELL_SIZE;
        realtransform(3, 1) = position.worldPosition.y() - scy * SDM_SUPERCELL_SIZE;
    }

	// Check if this place is concave
	corners[0].set(poolModel->bounds.xMin(), poolModel->bounds.yMin(), 0);
	corners[1].set(poolModel->bounds.xMax(), poolModel->bounds.yMax(), 0);
	corners[2].set(corners[0].x(), corners[1].y(), 0);
	corners[3].set(corners[1].x(), corners[0].y(), 0);

	for(int i=0; i < 4; i++)
	{
		corners[i] = corners[i] * realtransform;
		contact.pos[0] = corners[i].x();
		contact.pos[1] = corners[i].y();

		GetSurfaceContact(contact, COLLIDE_MASK, worldPhysicsSpace);

		if((contact.pos[2] + 0.1) < -corners[i].z())
		{
			// There are too many terrain objects where this happens, so don't clutter the log when removing them here.
			if(poolModel->modelType != ModelTypes::TERRAIN_OBJECT)
			{
				dtlErrors << "Convex placement: ";
				LogId();
			}
			return false;
		}
	}
	return true;
}


void SuperCellTerrainData::TerrainObject::LogId()
{
	// this->transform is under construction so get something temporary
	Matrixf	worldPosTransform(objInfo.transform);
	Vec3f	center = poolModel->bounds.center();


	worldPosTransform.setTrans(position.worldPosition);
	center = center * worldPosTransform;

	dtlErrors << "superCell(" << position.supercellX << ", " << position.supercellY << ");";
	dtlErrors << " position(" << center.x() << ", " << center.y() << ");";
	dtlErrors << " modelName(" << poolModel->modelname << ")" << endl;
}


void SuperCellTerrainData::TerrainObject::AddPlacementStats()
{
	ModelStatsMap::iterator modelStatsIt		= modelStatsMap.find(poolModel);
	ModelStats&				modelStats			= modelStatsIt->second;
	dContactGeom			contact;
	Vec3f					centerPoint			= poolModel->bounds.center();
	Vec3f					centerPointWorld	= centerPoint * transform;
	Matrixf					transform			= this->transform;
	Matrixf					shearTransform;
	float					incline;
	Vec3f					corners[4];
	BoundingBox				topBaseBounds, bottomBaseBounds;
	FLOAT32					minCollisionHeight	= FLT_MAX;
	FLOAT32					maxCollisionHeight	= -FLT_MAX;
	float					marginDelta;


	assert(modelStatsIt != modelStatsMap.end());

	if(poolModel->modelType != ModelTypes::TERRAIN_TILE)
	{
		contact.pos[0] = centerPointWorld.x();
		contact.pos[1] = centerPointWorld.y();

		GetSurfaceContact(contact, COLLIDER_ALL, worldPhysicsSpace);

		incline = (float)RAD_TO_DEG(acos(fabs(contact.normal[2])));
		if(incline > 5)	modelStats.steepPlaces++;
	}

	if(!poolModel->GetSubObjectBoundsByName(&topBaseBounds, "topbase", 0))
	{
		topBaseBounds = poolModel->bounds;
	}

	corners[0].set(topBaseBounds.xMin(), topBaseBounds.yMin(), 0);
	corners[1].set(topBaseBounds.xMax(), topBaseBounds.yMax(), 0);
	corners[2].set(corners[0].x(), corners[1].y(), 0);
	corners[3].set(corners[1].x(), corners[0].y(), 0);

	for(int i=0; i < 4; i++)
	{
		corners[i] = corners[i] * transform;
		contact.pos[0] = corners[i].x();
		contact.pos[1] = corners[i].y();

		GetSurfaceContact(contact, COLLIDER_ALL, worldPhysicsSpace);
		if(contact.pos[2] > maxCollisionHeight)	maxCollisionHeight = (float)contact.pos[2];
		if(contact.pos[2] < minCollisionHeight)	minCollisionHeight = (float)contact.pos[2];
	}

	transform.setTrans(transform.getTrans() + Vec3f(0, 0, -maxCollisionHeight));

	if(poolModel->GetSubObjectBoundsByName(&bottomBaseBounds, "bottombase", 0))
	{
		minCollisionHeight	= FLT_MAX;

		corners[0].set(bottomBaseBounds.xMin(), bottomBaseBounds.yMin(), -FLT_MAX);
		corners[1].set(bottomBaseBounds.xMax(), bottomBaseBounds.yMax(), -FLT_MAX);
		corners[2].set(corners[0].x(), corners[1].y(), -FLT_MAX);
		corners[3].set(corners[1].x(), corners[0].y(), -FLT_MAX);

		for(int i=0; i < 4; i++)
		{
			corners[i] = corners[i] * transform;
			contact.pos[0] = corners[i].x();
			contact.pos[1] = corners[i].y();

			GetSurfaceContact(contact, COLLIDER_ALL, worldPhysicsSpace);
			if(contact.pos[2] < minCollisionHeight)	minCollisionHeight = (float)contact.pos[2];
		}

		marginDelta = maxCollisionHeight - minCollisionHeight - bottomBaseBounds.zMax();
	}
	else	marginDelta = maxCollisionHeight - minCollisionHeight;
	marginDelta = fabs(marginDelta);

	if(marginDelta > (0.2 + (poolModel->bounds.radius() / 1000)) )	modelStats.nonPlanarCount++;

	float previousValue = -FLT_MAX;
	if(modelStats.slopeDeltaList.empty())	modelStats.slopeDeltaList.insert(modelStats.slopeDeltaList.begin(), marginDelta);
	else
	{
		for(vector<float>::iterator it = modelStats.slopeDeltaList.begin(); it != modelStats.slopeDeltaList.end(); ++it)
		{
			if(marginDelta >= previousValue && marginDelta >= *it)
			{
				modelStats.slopeDeltaList.insert(it, marginDelta);
				break;
			}
			previousValue = *it;
		}
	}
}


void SuperCellTerrainData::WriteToSDM()
{
	FILE*					sdmFile;
	string					fileName;

	ObjectList				cellReplacementObjects;
	ObjectList				cellObjects;
	ObjectList::iterator	objectMapIt;
	unsigned short			totalObjectCount	= 0;
	set<unsigned short>		stratIds;


#if !WANT_NON_REPLACEMENT_TILES
	// These data are no longer needed as well.
	editListCnt = 256;
	memset(editList, 0, sizeof(objects));
	for(int i=0; i < editListCnt; i++)
	{
		editList[i].octet = i;
		editList[i].txndx = nullTextureIdx;
	}
#endif

	if(cellName.find("Default") == string::npos)
	{
		for(int octetX=0; octetX < SUPERCELL_OCTETS_X; octetX++)
		{
			for(int octetY=0; octetY < SUPERCELL_OCTETS_Y; octetY++)
			{
				Octet*			octet			= &octets[octetX][octetY];

                if(!octet->replacementTile)
                    continue;

				cellReplacementObjects.insert(ObjectList::value_type(octet->replacementTile->poolModel, octet->replacementTile));
				for(unsigned int i=0; i < octet->objects.size(); i++)
				{
					cellObjects.insert(ObjectList::value_type(octet->objects[i]->poolModel, octet->objects[i]));
				}
			}
		}
	}

	totalObjectCount = cellReplacementObjects.size() + cellObjects.size();
	if(totalObjectCount > SDM_SUPERCELL_OCTETS_XY * MAX_TRNOBJ_OCTET)
	{
		logFile << endl << "Fatal error: superCell(" << supercellX << "," << supercellY << "): too many terrain objects: " << totalObjectCount << endl;
		logFile << "MAX_TRNOBJ_OCTET has to be increased.";
		piExitProgram (__FILE__, __LINE__, "As described in terrainBuildLog.txt.");
	}


	fileName.append(dataPath + sdmDir + cellName + MERGEFILE_EXTENSION);

#if DEBUG_WW2
    char str[512];
    sprintf(str, "[%d] %s\n", sdmFilesMerged, cellName.c_str());
    OutputDebugStringA(str);
#endif

    sdmFile = fopen(fileName.c_str(), "w+b");
	assert(sdmFile);

	terrainHdr.company		= PLAYNET_COMPANY;
	terrainHdr.flags		= 0;
	terrainHdr.filetype		= PLAYNET_MERGEFILE;
	terrainHdr.version		= MERGEFILE_VERSION;
	fwrite(&terrainHdr, sizeof(TERRAINFILE), 1, sdmFile);

	short heigthMapBytes = heightMapElementCnt * sizeof(short);
	fwrite(&heigthMapBytes, sizeof(short), 1, sdmFile);
	fwrite(heightMapShort, sizeof(short), heightMapElementCnt, sdmFile);

	short normalsBytes = normalsCnt * sizeof(short);
	fwrite(&normalsBytes, sizeof(short), 1, sdmFile);
	fwrite(normalShort, sizeof(short), normalsCnt, sdmFile);

	fwrite(&texNameCnt, sizeof(short), 1, sdmFile);
	if(texNameCnt > 0)	fwrite(texNames, 32, texNameCnt, sdmFile);

	fwrite(&editListCnt, sizeof(short), 1, sdmFile);
	if(editListCnt > 0) 	fwrite(editList, sizeof(TERRAIN_INFO), editListCnt, sdmFile);

	fwrite(&totalObjectCount, sizeof(short), 1, sdmFile);

	for(objectMapIt = cellReplacementObjects.begin(); objectMapIt != cellReplacementObjects.end(); ++objectMapIt)
	{
		TerrainObject*	object = objectMapIt->second;

		// Set to world position for the runtime data
		object->transform.setTrans(object->transform.getTrans() + Vec3f(	object->position.supercellX * SDM_SUPERCELL_SIZE,
																			object->position.supercellY * SDM_SUPERCELL_SIZE,
																			0	));
		object->transform.get(object->objInfo.transform);

		fwrite(&object->objInfo, sizeof(SDM_ObjInfo), 1, sdmFile);
	}
	for(objectMapIt = cellObjects.begin(); objectMapIt != cellObjects.end(); ++objectMapIt)
	{
		TerrainObject*	object = objectMapIt->second;
		// Set to world position for the runtime data
		object->transform.setTrans(object->transform.getTrans() + Vec3f(	object->position.supercellX * SDM_SUPERCELL_SIZE,
																			object->position.supercellY * SDM_SUPERCELL_SIZE,
																			0	));
		object->transform.get(object->objInfo.transform);

		fwrite(&object->objInfo, sizeof(SDM_ObjInfo), 1, sdmFile);
	}

	terrainHdr.length = ftell(sdmFile) - sizeof(TERRAINFILE);
	fseek(sdmFile, 0, SEEK_SET);
	fwrite(&terrainHdr, sizeof(TERRAINFILE), 1, sdmFile);
	fseek(sdmFile, terrainHdr.length + sizeof(TERRAINFILE), SEEK_SET);

	STRT_DTL_HEADER stratHeader;
	stratHeader.numCPs			= cellStratMap[pair<int, int>(supercellX, supercellY)].chokepoints.size();
	stratHeader.numFacilities	= cellStratMap[pair<int, int>(supercellX, supercellY)].facilities.size();
	stratHeader.numBuildings	= cellStratMap[pair<int, int>(supercellX, supercellY)].buildings.size();
	stratHeader.numLinks		= cellStratMap[pair<int, int>(supercellX, supercellY)].links.size();
	stratHeader.numWaypoints	= cellStratMap[pair<int, int>(supercellX, supercellY)].waypoints.size();
	stratHeader.reserved1		= 0;
	stratHeader.reserved2		= 0;
	stratHeader.reserved3		= 0;

	fwrite(&stratHeader, sizeof(STRT_DTL_HEADER), 1, sdmFile);

	CellStratData*	cellStratData	= &cellStratMap[pair<int, int>(supercellX, supercellY)];
	assert(cellStratData);

	for(CPMap::iterator it = cellStratData->chokepoints.begin(); it != cellStratData->chokepoints.end(); ++it)
	{
		CP*	cp = it->second;

		if(stratIds.insert(cp->oid.id).second == false)
		{
			stratErrors << "Duplicate strat id for choke point: " << cp->oid.id << "; name(" << cp->name << "); superCell(" << supercellX << "," << supercellY << ")" << endl;
		}

		fwrite(cp, sizeof(CP), 1, sdmFile);
	}

	for(FacilityMap::iterator it = cellStratData->facilities.begin();  it != cellStratData->facilities.end(); ++it)
	{
		FACILITY* facility = it->second;

		if(stratIds.insert(facility->oid.id).second == false)
		{
			stratErrors << "Duplicate strat id for facility: " << facility->oid.id << "; name(" << facility->name << "); superCell(" << supercellX << "," << supercellY << ")" << endl;
		}

		fwrite(facility, sizeof(FACILITY), 1, sdmFile);
	}

    for(BuildingMap::iterator it = cellStratData->buildings.begin();  it != cellStratData->buildings.end(); ++it)
	{
		BUILDING* building = it->second;

		if(stratIds.insert(building->oid.id).second == false)
		{
			stratErrors << "Duplicate strat id for building: " << building->oid.id << "; name(" << building->name << "); superCell(" << supercellX << "," << supercellY << ")" << endl;
		}

		fwrite(building, sizeof(BUILDING), 1, sdmFile);
	}

    unsigned int size = cellStratData->links.size();

	for(unsigned int i = 0;  i < size; ++i)
	{
		DEPOT_LINK* link = cellStratData->links[i];

		if(stratIds.insert(link->oid.id).second == false)
		{
			stratErrors << "Duplicate strat id for link: " << link->oid.id << "; name(" << link->name << "); superCell(" << supercellX << "," << supercellY << ")" << endl;
		}

		fwrite(link, sizeof(DEPOT_LINK), 1, sdmFile);
	}

    size = cellStratData->waypoints.size();

	for(unsigned int i = 0;  i < size; ++i)
	{
		WAYPOINT* waypoint = cellStratData->waypoints[i];

		if(stratIds.insert(waypoint->oid.id).second == false)
		{
			stratErrors << "Duplicate strat id for waypoint: " << waypoint->oid.id << "; superCell(" << supercellX << "," << supercellY << ")" << endl;
		}

		fwrite(waypoint, sizeof(WAYPOINT), 1, sdmFile);
	}

    // Clutter

    fwrite(basecluttercount, sizeof(basecluttercount), 1, sdmFile);
    fwrite(&minheight, sizeof(float), 1, sdmFile);

    unsigned int pos = 0;
    int idx = 0;

    for(int i = 0; i < MaxBushes; i++)
    {
        for(int j = 0; j < basecluttercount[i]; j++)
        {
            fwrite((unsigned char*)&bushclutterdata[idx].octet, sizeof(unsigned char), 1, sdmFile);

            pos = ((unsigned int)(bushclutterdata[idx].x + 0.5f)) << 22;
            pos |= ((unsigned int)(bushclutterdata[idx].y + 0.5f)) << 12;
            pos |= (unsigned int)(bushclutterdata[idx].z * 10.0f);

            idx++;

            fwrite(&pos, sizeof(unsigned int), 1, sdmFile);
        }
    }

	fclose(sdmFile);
}

void TerrainObjectSafeDelete(SuperCellTerrainData::TerrainObject* to)
{
    if(!to)
        return;

    if(to->instance && to->instance->rootObject)
    {
        obpoolFreeObjectCallback(to->instance->rootObject);
        to->instance->rootObject = NULL;
    }

    ToDeleteTerrainObjects::iterator itr = TerrainObjectsToDelete.find(to);

    if(itr != TerrainObjectsToDelete.end())
    {
        TerrainObjectsToDelete.erase(itr);
        delete to;
    }
}


SuperCellTerrainData::TerrainObject* SuperCellTerrainData::AddTerrainObject(const char* name, const Matrixf& worldTransform, int ortdrg)
{
	TerrainPosition			objectPosition(worldTransform.getTrans());
	Octet*					octet			= &octets[objectPosition.octetX][objectPosition.octetY];
	TerrainObject*			terrainObject	= new TerrainObject(worldTransform, cellTransform, octet->octetPhysicsSpace);

	ModelStatsMap::iterator	modelStatsMapIt;
	int						objectIndex		= octet->objects.size();


	if(strstr(name, "null1"))
	{
		int averageHeight = 0;

        // This is the octet we want
        int ox = terrainObject->position.octetX;
        int oy = terrainObject->position.octetY;

        int currentoctet = oy * SUPERCELL_OCTETS_Y + ox;

        // This is where the triagles begin for that octet
        currentoctet *= SDM_TRIANGLES_PER_OCTET;

        piVectorConstPtr pos = newHeightMap->RenderPositions();

		for(int v = 0; v < NUM_VERTICES_PER_OCTET; v++)
		{
            // -= for back-ass-wards coordinate system
            averageHeight -= (int)pos[currentoctet + v].z;
		}

		averageHeight /= NUM_VERTICES_PER_OCTET;

		if		(averageHeight > ALT_TRN_ROCKY)
            terrainObject->poolModel	= obmgrGetPoolModel("icy1");
		else if	(averageHeight > ALT_TRN_FOREST)
            terrainObject->poolModel	= obmgrGetPoolModel("rocky1");
		else if	(averageHeight > ALT_TRN_CLEAR)
            terrainObject->poolModel	= obmgrGetPoolModel("forest1");
		else if	(averageHeight > ALT_TRN_FARMS)
            terrainObject->poolModel	= obmgrGetPoolModel("clear1");
		else if	(averageHeight > ALT_TRN_DUNES)
            terrainObject->poolModel	= obmgrGetPoolModel("farms1");
		else if	(averageHeight > ALT_TRN_WATER)
            terrainObject->poolModel	= obmgrGetPoolModel("dunes1");
		else if	(averageHeight == ALT_TRN_WATER)
            terrainObject->poolModel	= obmgrGetPoolModel("water1");
		else
            terrainObject->poolModel	= obmgrGetPoolModel("dunes1");

		strcpy(terrainObject->objInfo.objname, terrainObject->poolModel->modelname.c_str());

		terrainObject->objInfo.ortdrg	= (rand() % 4) * 2;
	}
	else
	{
		terrainObject->poolModel	= obmgrGetPoolModel(name);
		strcpy(terrainObject->objInfo.objname, name);

		terrainObject->objInfo.ortdrg		= ortdrg;
	}

	if(!terrainObject->poolModel)
	{
		missingModels.insert(string(name));

        TerrainObjectSafeDelete(terrainObject);
        return 0;
	}


	if(terrainObject->poolModel->modelType == ModelTypes::TERRAIN_TILE)
	{
		if(octet->replacementTile)
		{
            TerrainObjectSafeDelete(octet->replacementTile);
		}
		octet->replacementTile = terrainObject;
		octet->replacementTile->objInfo.flags = TDFLAG_LOADFAR;
	}
	else
        octet->objects.push_back(terrainObject);	// The master object reference has to be in the list before it's sub-objects

	modelStatsMapIt = modelStatsMap.insert(ModelStatsMap::value_type(terrainObject->poolModel, ModelStats())).first;
	modelStatsMapIt->second.totalInstanceCount++;

	TerrainObject*	actualTerrainObject = AddSubObjects(terrainObject, worldTransform);
	if(actualTerrainObject != terrainObject)
	{
        TerrainObjectSafeDelete(terrainObject);
		terrainObject = actualTerrainObject;
		octet->objects.erase(octet->objects.begin() + objectIndex);
	}

	terrainObject->objInfo.flags		= 0;
	terrainObject->objInfo.pos.x		= terrainObject->position.worldPosition.x();
	terrainObject->objInfo.pos.y		= terrainObject->position.worldPosition.y();
	terrainObject->objInfo.pos.z		= terrainObject->position.worldPosition.z();
	terrainObject->objInfo.ruftrn		= 0;

#if !WANT_NON_REPLACEMENT_TILES
	terrainObject->objInfo.txndx		= nullTextureIdx;
#endif

    SDM_ObjInfo& oi = terrainObject->objInfo;

    // Orient all ocean tiles the same direction
    if(!strncmp(oi.objname, "water1", 6))
    {
        oi.ortdrg = 0;
        Vec3f trans = terrainObject->transform.getTrans();
        terrainObject->transform.makeIdentity();
        terrainObject->transform.setTrans(trans);
    }

	return terrainObject;
}


SuperCellTerrainData::TerrainObject* SuperCellTerrainData::AddSubObjects(TerrainObject* masterObject, const Matrixf& worldTransform)
{
	ObjectId							objectId((int)masterObject->position.worldPosition.x(), (int)masterObject->position.worldPosition.y(), masterObject->poolModel->modelname.c_str());
	CellStratData*						cellStratData			= &cellStratMap[pair<int, int>(supercellX, supercellY)];
	int									stratObjectCount		= 0;
	bool								isHollowReference		= masterObject->poolModel->propertyFlags & PoolModel::PropertyFlags::HOLLOW_REFERENCE;
	PoolModel*							originalPoolModel		= masterObject->poolModel;

	CPMap::iterator	masterObjectChokepoint = cellStratData->chokepoints.find(objectId);
	FacilityMap::iterator masterObjectFacility = cellStratData->facilities.find(objectId);
	BuildingMap::iterator masterObjectBuilding = cellStratData->buildings.find(objectId);


	if(masterObjectChokepoint != cellStratData->chokepoints.end())	{masterObject->stratData.cp			= masterObjectChokepoint->second; stratObjectCount++;}
	if(masterObjectFacility != cellStratData->facilities.end())		{masterObject->stratData.facility	= masterObjectFacility->second;	stratObjectCount++;}
	if(masterObjectBuilding != cellStratData->buildings.end())		{masterObject->stratData.building	= masterObjectBuilding->second;	stratObjectCount++;}
	assert(stratObjectCount <= 1);

	for(unsigned int i=0; i < originalPoolModel->subModelReferences.size(); i++)
	{
		SubModelReference	subModelReference		= originalPoolModel->subModelReferences[i];
		Matrixf				subModelWorldTransform	= Matrixf(subModelReference.transform) * worldTransform;
		ObjectId			subModelObjectId((int)subModelWorldTransform.getTrans().x(), (int)subModelWorldTransform.getTrans().y(), subModelReference.poolModel->modelname.c_str());
		TerrainObject*		subModelTerrainObject	= AddTerrainObject(subModelReference.poolModel->modelname.c_str(), subModelWorldTransform, 0);


		assert(subModelTerrainObject);
		subModelTerrainObject->placeOntoMasterObject = subModelReference.placeOntoMasterObject;

		if(subModelReference.poolModel->propertyFlags & PoolModel::PropertyFlags::STRAT_DATA)
		{
			if(masterObject->stratData.cp)
			{
				stringstream	stratNameSuffix;
				CP*				chokepoint;

                chokepoint = (CP*)memAlloc(sizeof(*masterObject->stratData.cp));
                memcpy(chokepoint, masterObject->stratData.cp, sizeof(CP));

                strcpy(chokepoint->fileName, subModelObjectId.fileName);
				chokepoint->posx = subModelObjectId.posX;
				chokepoint->posy = subModelObjectId.posY;

				if(!isHollowReference)
				{
					cellStratData->maxStratId++;
					chokepoint->oid.id = cellStratData->maxStratId;
					stratNameSuffix << "i" << i;	// Should be sufficient
					strcat(chokepoint->name, stratNameSuffix.str().c_str());
				}

				cellStratData->chokepoints.insert(pair<ObjectId, CP*>(subModelObjectId, chokepoint));
				subModelTerrainObject->stratData.cp = chokepoint;
				if(isHollowReference)
				{
					cellStratData->chokepoints.erase(masterObjectChokepoint);
					masterObjectChokepoint	= cellStratData->chokepoints.find(subModelObjectId);
				}
			}
			if(masterObject->stratData.facility)
			{
				stringstream	stratNameSuffix;
				FACILITY*		facility;

				facility = (FACILITY*)memAlloc(sizeof(*masterObject->stratData.facility));
                memcpy(facility, masterObject->stratData.facility, sizeof(FACILITY));

				strcpy(facility->fileName, subModelObjectId.fileName);
				facility->posx = subModelObjectId.posX;
				facility->posy = subModelObjectId.posY;

				if(!isHollowReference)
				{
					cellStratData->maxStratId++;
					facility->oid.id = cellStratData->maxStratId;
					stratNameSuffix << "i" << i;	// Should be sufficient
					strcat(facility->name, stratNameSuffix.str().c_str());
				}

				cellStratData->facilities.insert(pair<ObjectId, FACILITY*>(subModelObjectId, facility));
				subModelTerrainObject->stratData.facility = facility;
				if(isHollowReference)
				{
					cellStratData->chokepoints.erase(masterObjectChokepoint);
					masterObjectFacility	= cellStratData->facilities.find(subModelObjectId);
				}
			}
			if(masterObject->stratData.building)
			{
				stringstream	stratNameSuffix;
				BUILDING*		building;

				building = (BUILDING*)memAlloc(sizeof(*masterObject->stratData.building));
                memcpy(building, masterObject->stratData.building, sizeof(BUILDING));

				strcpy(building->fileName, subModelObjectId.fileName);

                building->oid = masterObject->stratData.building->oid;
                building->pid = masterObject->stratData.building->pid;

				building->posx = subModelObjectId.posX;
				building->posy = subModelObjectId.posY;

				if(!isHollowReference)
				{
					cellStratData->maxStratId++;
					building->oid.id = cellStratData->maxStratId;
					stratNameSuffix << "i" << i;	// Should be sufficient
					strcat(building->name, stratNameSuffix.str().c_str());
				}

				cellStratData->buildings.insert(pair<ObjectId, BUILDING*>(subModelObjectId, building));
				subModelTerrainObject->stratData.building = building;
				if(isHollowReference)
				{
					cellStratData->buildings.erase(masterObjectBuilding);
					masterObjectBuilding	= cellStratData->buildings.find(subModelObjectId);
				}
			}
		}

		if(isHollowReference)
		{
			// The first sub-model referenced is always the new master model
			masterObject			= subModelTerrainObject;
			isHollowReference		= masterObject->poolModel->propertyFlags & PoolModel::PropertyFlags::HOLLOW_REFERENCE;
			objectId				= subModelObjectId;
		}
		else	subModelTerrainObject->masterObject = masterObject;
	}

	return masterObject;
}

void SuperCellTerrainData::PlaceTerrain()
{
	for(int octetX=0; octetX < SUPERCELL_OCTETS_X; octetX++)
	{
		for(int octetY=0; octetY < SUPERCELL_OCTETS_Y; octetY++)
		{
			Octet*			octet			= &octets[octetX][octetY];
			TerrainObject*	replacementTile = octet->replacementTile;

#if WANT_NON_REPLACEMENT_TILES
            if(!replacementTile)
                continue;
#else
			assert(replacementTile);
#endif
			replacementTile->instance = replacementTile->poolModel->CreateInstance(replacementTile->objInfo.transform);
			assert(replacementTile->instance);

            replacementTile->PlaceDown(replacementTile->instance->rootObject, planes);

			physicsRecurseAndCreateObjectCollider(	replacementTile->instance->rootObject,
													octet->octetPhysicsSpace,
													COLLIDER_TRN_GROUND,
													COLLIDER_VOID,
													true,
													false,
													true	);
			dSpaceClean(octet->octetPhysicsSpace);
			dSpaceClean(cellPhysicsSpace);
			dSpaceClean(worldPhysicsSpace);
		}
	}

    if(planes)
    {
        delete [] planes;
        planes = NULL;
    }
}

struct AttachMatrix
{
    piMATRIX m;

    void apply(OBJECT* o)
    {
        if(o)
        {
            o->mat = m;

            apply(o->child);
            apply(o->sibling);
            apply(o->attached);
        }
    }
};

static bool _sort(const BushClutterInfo& l, const BushClutterInfo& r)
{
    return l.base < r.base;
}

void SuperCellTerrainData::PlaceClutter()
{
#define MAX_CLUTTER_PER_OCTET   32

    dContactGeom contact;

    memset(basecluttercount, 0, sizeof(short) * 6);
    minheight = FLT_MAX;

    std::vector<float> z;

    unsigned int bits = 0;

    for(int c = 0; c < SDM_SUPERCELL_OCTETS_X; c++)
    {
        for(int r = 0; r < SDM_SUPERCELL_OCTETS_Y; r++)
        {
            unsigned int seed = supercellX * 4919 + supercellY * 7433 + r * 2371 + c * 6043;
            PNRandom::seedrandom(seed);

            for(int i = 0; i < MAX_CLUTTER_PER_OCTET; i++)
            {
                // World position
                double x = PNRandom::random(c * SDM_OCTET_SIZE, c * SDM_OCTET_SIZE + SDM_OCTET_SIZE);
                double y = PNRandom::random(r * SDM_OCTET_SIZE, r * SDM_OCTET_SIZE + SDM_OCTET_SIZE);

                contact.pos[0] = x;
                contact.pos[1] = y;

                if(GetSurfaceContact(contact, COLLIDER_ALL, worldPhysicsSpace))
                {
                    int num = PNRandom::random(0, 2);
                    int type[3];
                    int density;
                    float width, height;

                    OBJECT* o = (OBJECT*)dGeomGetData(contact.g2);

                    mlGetGroundClutterStuff(&type[0],
                                            &type[1],
                                            &type[2],
                                            &width,
                                            &height,
                                            &density,
                                            o->texname);

		            int cluttertype = type[num];

                    if(cluttertype == 9)
                    {
                        // Grass
                    }
                    else if(cluttertype > 0)
                    {
                        BushClutterInfo bush;

                        bush.octet = TRNINDEX(r, c);
                        bush.base = PNRandom::random(0, 5);
                        bush.x = (float)(x - c * SDM_OCTET_SIZE);
                        bush.y = (float)(y - r * SDM_OCTET_SIZE);
                        bush.z = (float)contact.pos[2];

                        minheight = std::min(minheight, bush.z);

                        basecluttercount[bush.base]++;

                        bushclutterdata.push_back(bush);
                    }
                }
            }
        }
    }

    std::sort(bushclutterdata.begin(), bushclutterdata.end(), _sort);

    for(unsigned int i = 0; i < bushclutterdata.size(); i++)
    {
        bushclutterdata[i].z -= minheight;
    }
}

void SuperCellTerrainData::PlaceTerrainObjects(int pass)
{
	for(int octetX=0; octetX < SUPERCELL_OCTETS_X; octetX++)
	{
		for(int octetY=0; octetY < SUPERCELL_OCTETS_Y; octetY++)
		{
			Octet*	octet = &octets[octetX][octetY];

			for(vector<TerrainObject*>::iterator it = octet->objects.begin(); it != octet->objects.end();)
			{
				TerrainObject*			object			= *it;
				UINT32					modelType		= object->poolModel->modelType;
				bool					keepObject		= true;

                // F'ng HACK! for object on object collisions, which was never implemented.
                bool needcollider = false;
                bool specialobject = false;

                if(object->poolModel->modelname.find("scdkrd14-36i") != std::string::npos ||
                   object->poolModel->modelname.find("scdkrd14-36o") != std::string::npos ||
                   object->poolModel->modelname.find("bldgard1") != std::string::npos)
                {
                    specialobject = true;
                }

                // These models have objects on top, so handle during first pass
                if(object->poolModel->modelType == ModelTypes::BRIDGE ||
                   object->poolModel->modelType == ModelTypes::TERRAIN_OBJECT)
                {
                    specialobject = true;
                }

                if(pass == 0)
                {
                    if(specialobject)
                    {
                        if(!object->instance)
                        {
                            object->instance = object->poolModel->CreateInstance(object->objInfo.transform);
                        }

                        needcollider = true;
                    }
                    else
                    {
                        ++it;
                        continue;
                    }
                }
                // Second pass
                else
                {
                    // Handled during first pass
                    if(specialobject)
                    {
                        ++it;
                        continue;
                    }
                }

                // This is the supercell we are interested in. Need this since some objects
                // overlap.
                object->scx = supercellX;
                object->scy = supercellY;

 				assert(!(object->poolModel->propertyFlags & PoolModel::PropertyFlags::HOLLOW_REFERENCE));

				if(object->poolModel->placeMode != OBJVAL_NO_PLACEMODE)
                    object->AddPlacementStats();

                if(object->placeOntoMasterObject)
				{
					TerrainObject*	masterObject			= object->masterObject;
					Vec3f			masterObjectTranslation	= masterObject->transform.getTrans();
					Vec3f			subObjectTranslation	= object->transform.getTrans();

					object->transform.setTrans(Vec3f(	subObjectTranslation.x(),
														subObjectTranslation.y(),
														masterObjectTranslation.z() - subObjectTranslation.z()	));

					++it;
					continue;
				}

				switch(object->poolModel->placeMode)
				{
					case OBJVAL_PLACE_DOWN:
					case OBJVAL_OCTET_CONFORMRADIAL:
						if(	modelType != ModelTypes::TERRAIN_TILE	&&
							modelType != ModelTypes::TERRAIN_OBJECT	)
						{
							object->PlaceSheared();
						}
						break;

					case OBJVAL_PLACE_BASE:
						object->PlaceStaticBase();
						break;

					case OBJVAL_PLACE_SHEARED:
						object->PlaceSheared();
						break;

					case OBJVAL_PLACE_CONCAVE:
						keepObject = object->PlaceConcave();
						break;

					case OBJVAL_PLACE_STRAIGHT_DOWN:
						object->PlaceOnGround();
						break;
                    case OBJVAL_PLACE_DOWN_ROTATE:
                        {
                            object->PlaceDownRotate();
                            break;
                        }
				}

				if(!keepObject)
				{
					// We have to make sure that we don't remove essantial objects.
					if(!object->stratData.cp && !object->stratData.facility && !object->stratData.building)
					{
						// Make sure that we already sent out an warning if this is something meaningful.
						TerrainObjectSafeDelete(*it);
						it = octet->objects.erase(it);
						continue;
					}
					else keepObject = true;
				}

                if(needcollider && object->instance)
                {
                    OBJECT* o = NULL;

                    trnFindCollisionObject(&o, object->instance->rootObject);

                    if(!o)
                    {
                        FindByName<OBJECT> fbn;
                        fbn.set("lod1");
                        fbn.find(object->instance->rootObject);

                        o = (OBJECT*)fbn.node;
                    }

                    if(!o)
                        o = object->instance->rootObject;

                    AttachMatrix am;
                    object->transform.get(am.m);

                    am.apply(o);

                    unsigned int flag = 0;

                    if(object->poolModel->modelType == ModelTypes::BUILDING)
                        flag = COLLIDER_TRN_BUILDING;
                    else if(object->poolModel->modelType == ModelTypes::BRIDGE)
                        flag = COLLIDER_TRN_BRIDGE;
                    else if(object->poolModel->modelType == ModelTypes::TERRAIN_OBJECT)
                        flag = COLLIDER_TRN_CULTURAL;

			        physicsRecurseAndCreateObjectCollider(o,
                                                          octet->octetPhysicsSpace,
                                                          flag,
                                                          COLLIDER_VOID,
                                                          true,
                                                          false,
                                                          true);
                }

				if(keepObject)
                    ++it;
			}
		}
	}
}


void SuperCellTerrainData::Preprocess()
{
	if(cellName.find("Default") != string::npos)	return;

	if(editListCnt != SUPERCELL_OCTETS_X * SUPERCELL_OCTETS_Y)
	{
		assert(editListCnt == 0);

#if !WANT_NON_REPLACEMENT_TILES
		for(int octetX=0; octetX < SUPERCELL_OCTETS_Y ; octetX++)
		{
			for(int octetY=0; octetY < SUPERCELL_OCTETS_X ; octetY++)
			{
				Octet*	octet			= &octets[octetX][octetY];
				Matrixf	worldTransform	= Matrixf::translate(worldOrigin + Vec3f((octetX+0.5f)*800.0f, (octetY+0.5f)*800.0f, 0.0f));

				TerrainObject*	replacementTile = AddTerrainObject("null1", worldTransform, 0);
				assert(replacementTile);
			}
		}
#endif
		return;
	}

	int octetX, octetY;

#if !WANT_NON_REPLACEMENT_TILES
    // Don't want nonreplacement tiles converted to replacement tiles
	for(octetX=0; octetX < SUPERCELL_OCTETS_X; octetX++)
	{
		for(octetY=0; octetY < SUPERCELL_OCTETS_Y; octetY++)
		{
			Octet*			octet			= &octets[octetX][octetY];
			TerrainObject*	replacementTile	= 0;
			string			tileName		= texNames[octet->editInfo.txndx];
			size_t			bmpPos			= tileName.find(".bmp", 0);
			Matrixf			worldTransform	= Matrixf::rotate(-((octet->editInfo.ortdrg*2) & 0xf) * PI / 4.0f, 0, 0, 1);


			if(bmpPos != tileName.npos)	tileName.erase(bmpPos, tileName.npos);
			if(octet->editInfo.txndx == nullTextureIdx)
			{
				// This tile is propably discribed by one of the terrain objects
				continue;
			}

			worldTransform.setTrans(worldOrigin + Vec3f((octetX+0.5f)*800.0f, (octetY+0.5f)*800.0f, 0.0f));
			AddTerrainObject(tileName.c_str(), worldTransform, octet->editInfo.ortdrg*2);
		}
	}
#endif

	for(int i=0; i < objectCnt; i++)
	{
		Matrixf	worldTransform	= Matrixf::rotate(-(objects[i].ortdrg & 0xf) * PI / 4.0f, 0, 0, 1) * Matrixf::translate(objects[i].pos);

		AddTerrainObject(objects[i].objname, worldTransform, objects[i].ortdrg);
	}

#if !WANT_NON_REPLACEMENT_TILES
	for(octetX=0; octetX < SUPERCELL_OCTETS_X; octetX++)
	{
		for(octetY=0; octetY < SUPERCELL_OCTETS_Y; octetY++)
		{
			Octet*			octet			= &octets[octetX][octetY];

			if(!octet->replacementTile)
			{
				// Some tiles are missing in the edit list for no valid reason
				string			tileName		= texNames[0];
				size_t			bmpPos			= tileName.find(".bmp", 0);
				Matrixf			worldTransform	= Matrixf::translate(worldOrigin + Vec3f((octetX+0.5f)*800.0f, (octetY+0.5f)*800.0f, 0.0f));

				if(bmpPos != tileName.npos)	tileName.erase(bmpPos, tileName.npos);
				AddTerrainObject(tileName.c_str(), worldTransform, 0);
			}
		}
	}
#endif

	PlaceTerrain();
	PlaceTerrainObjects(0);
	PlaceTerrainObjects(1);
    PlaceClutter();
}

SuperCellTerrainData::SuperCellTerrainData(int cellX, int cellY, const Vec3f& offset)
	:	supercellX(cellX),
		supercellY(cellY),
        newHeightMap(NULL),
		cellPhysicsSpace(dHashSpaceCreate(worldPhysicsSpace)),
		octets()
{
	FILE*		dtlFile;
	char		fileName[MAX_PATH];
	string		fullFileName;
    /*
    short			heightMapShort[SUPERCELL_SIZE_XY];
		short			normalsCnt;
		short			normals[SUPERCELL_SIZE_XY];

*/
    memset(heightMap, 0, sizeof(float) * SUPERCELL_SIZE_XY);
    memset(normalShort, 0, sizeof(short) * SUPERCELL_SIZE_XY);
    memset(texNames, 0, sizeof(char) * 32 * MAX_TERRAIN_TEXTURES);
    memset(editList, 0, sizeof(TERRAIN_INFO) * MAX_TERRAIN_EDIT_LIST);
    memset(objects, 0, sizeof(DTL_TRNOBJ_INFO) * SDM_SUPERCELL_OCTETS_XY * MAX_TRNOBJ_OCTET);

	sprintf(fileName, "%04x_%04x", (unsigned short)supercellX, (unsigned short)supercellY);
	//wsprintfA(fileName, "%04x_%04x", (unsigned short)supercellX, (unsigned short)supercellY);
	if(supercellX == SUPERCELL_INVALID && supercellY == SUPERCELL_INVALID)	cellName = "Default";
	else	cellName = fileName;

	fullFileName.append(dataPath + dtlCpfDir + cellName + TERRAINFILE_EXTENSION);

	dtlFile = fopen(fullFileName.c_str(), "rb");
	if(!dtlFile)
	{
		// This can happen when a neighbor to a border cell is requested
		cellName = "Default";
		fullFileName.clear();
		fullFileName.append(dataPath + dtlCpfDir + cellName + TERRAINFILE_EXTENSION);
		dtlFile = fopen(fullFileName.c_str(), "rb");

		if(dtlFile == 0)
			int sdf=1;
		assert(dtlFile);
	}

	worldOrigin.set(supercellX * SDM_SUPERCELL_SIZE, supercellY * SDM_SUPERCELL_SIZE, 0);

	fread(&terrainHdr, sizeof(TERRAINFILE), 1, dtlFile);
	fread(&heightMapElementCnt, sizeof(short), 1, dtlFile);
	heightMapElementCnt = heightMapElementCnt / sizeof(short);

	assert(heightMapElementCnt);

	if(terrainHdr.flags & TF_FLAG_DATA_RLE)
	{
		short	RLEdata[SUPERCELL_SIZE_XY] = {0};
		fread(&RLEdata, sizeof(short), heightMapElementCnt, dtlFile);
		trnldrRLEToData(heightMapElementCnt, RLEdata, &heightMapElementCnt, heightMapShort);
	}
	else	fread(heightMapShort, sizeof(short), heightMapElementCnt, dtlFile);

#if(0)
	for(int x=0; x < SUPERCELL_SIZE_X; x++)
	{
		for(int y=0; y < SUPERCELL_SIZE_Y; y++)
		{
			heightMap[x][SUPERCELL_SIZE_Y-y-1] = heightMapShort[y][x];
			if(heightMap[x][SUPERCELL_SIZE_Y-y-1] == -9999)	heightMap[x][SUPERCELL_SIZE_Y-y-1] = 0;
		}
	}
#endif

	fread(&normalsCnt, sizeof(short), 1, dtlFile);
	normalsCnt = normalsCnt / sizeof(short);
	if(normalsCnt)
	{
		if(terrainHdr.flags & TF_FLAG_NORMALS_RLE)
		{
			short	RLEdata[SUPERCELL_SIZE_XY] = {0};
			fread(RLEdata, sizeof(short),  normalsCnt, dtlFile);
			trnldrRLEToData(normalsCnt, RLEdata, &normalsCnt, normalShort);
		}
		else	fread(&normals, sizeof(short), normalsCnt, dtlFile);
	}

#if(0)
	for(int x=0; x < SUPERCELL_SIZE_Y; x++)
	{
		for(int y=0; y < SUPERCELL_SIZE_X; y++)	normals[SUPERCELL_SIZE_Y-y-1][x] = normalShort[y][x];
	}
#endif

	fread(&texNameCnt, sizeof(short), 1, dtlFile);
	if(texNameCnt > 0)	fread(texNames, 32, texNameCnt, dtlFile);

	fread(&editListCnt, sizeof(short), 1, dtlFile);
	if(editListCnt > 0) 	fread(editList, sizeof(TERRAIN_INFO), editListCnt, dtlFile);

	fread(&objectCnt, sizeof(short), 1, dtlFile);
	if(objectCnt > 0)	fread(objects, sizeof(DTL_TRNOBJ_INFO), objectCnt, dtlFile);

	fclose(dtlFile);


	for(int octetX=0; octetX < SUPERCELL_OCTETS_X; octetX++)
	{
		for(int octetY=0; octetY < SUPERCELL_OCTETS_Y; octetY++)
		{
			memcpy(&octets[octetX][octetY].editInfo, &editList[octetX + octetY*16], sizeof(TERRAIN_INFO));
		}
	}

	nullTextureIdx = 0;
	for(int i=0; i < texNameCnt; i++)
	{
		if(strstr(texNames[i], "null1"))
            nullTextureIdx = i;
	}

	if(cellName.find("Default") != string::npos)
	{
		strcpy(texNames[texNameCnt], "water1.bmp");
		texNameCnt++;
	}
    // Don't force us to have a replacement tile
#if !WANT_NON_REPLACEMENT_TILES
	else if(!strstr(texNames[nullTextureIdx], "null1.bmp"))
	{
		strcpy(texNames[texNameCnt], "null1.bmp");
		nullTextureIdx = texNameCnt;
		texNameCnt++;
	}
#endif

	piMATRIX	cellInstanceTransform;
	cellTransform = Matrixf::translate(offset);

	// Generating special cases procedurally only creates a mess! Use straigtforward data instead.
	cellHeightMap = new TerrainObject(Matrixf::translate(Vec3f(SDM_SUPERCELL_SIZE * supercellX, SDM_SUPERCELL_SIZE * supercellY, 0) + offset), cellTransform, cellPhysicsSpace);

#if(0)
    cellHeightMap->poolModel	= obmgrGetPoolModel("cellheightmap");
    assert(cellHeightMap->poolModel);
#endif

	cellHeightMap->transform.get(cellInstanceTransform);

#if(0)
	cellHeightMap->instance	= cellHeightMap->poolModel->CreateInstance(cellInstanceTransform);
	assert(cellHeightMap->instance);
#endif

#if(1)
    WorldOriginOffset.x = cellHeightMap->position.worldPosition.x() - offset.x();
    WorldOriginOffset.y = cellHeightMap->position.worldPosition.y() - offset.y();
    WorldOriginOffset.z = 0;

    OBJECT* o;

    // Create height map same way application does it...so we work with the same numbers
    obmainCreateObject(&o, "heightmap", MAX_TERRAIN_VERTICES, MAX_TERRAIN_TRIANGLES);
    trnldrInitializeSuperCellVertices(o, cellX, cellY);

    obplyrApplyMatrixToObjectTree(o, &cellInstanceTransform);

    // Load normals from height map
    trnldrLoadNormals(o, normalShort);
    // Load in z values from the height map
    trnldrLoadPoints(o, heightMapShort);
    ldmdlCalcSurfaceNormals(o);
    ldmdlGetBounds(o);

    cellHeightMap->instance = NULL;
    newHeightMap = o;

    // Create plane equations for terrain placement
    planes = new PLANE[o->numtris];

    extern void FindPlaneEquations(OBJECT*, PLANE*);

    FindPlaneEquations(o, planes);

    WorldOriginOffset.x = 0;
    WorldOriginOffset.y = 0;
    WorldOriginOffset.z = 0;

    physicsRecurseAndCreateObjectCollider(newHeightMap,
                                          cellHeightMap->objectPhysicsSpace,
                                          COLLIDER_HEIGHTMAP,
                                          COLLIDER_VOID,
                                          true,
                                          false,
                                          true);
#else
	PlaceHeightMap(cellHeightMap->instance->rootObject);

    physicsRecurseAndCreateObjectCollider(	cellHeightMap->instance->rootObject,
											cellHeightMap->objectPhysicsSpace,
											COLLIDER_HEIGHTMAP,
											COLLIDER_VOID,
											true,
											false,
											true	);
#endif

    // Initialize the octet physics spaces
	for(int octetX=0; octetX < SUPERCELL_OCTETS_Y ; octetX++)
	{
		for(int octetY=0; octetY < SUPERCELL_OCTETS_X ; octetY++)
		{
			octets[octetX][octetY].replacementTile = 0;
			octets[octetX][octetY].octetPhysicsSpace = dHashSpaceCreate(cellPhysicsSpace);
		}
	}
}


SuperCellTerrainData::~SuperCellTerrainData()
{
	for(int octetX=0; octetX < SUPERCELL_OCTETS_X; octetX++)
	{
		for(int octetY=0; octetY < SUPERCELL_OCTETS_Y; octetY++)
		{
			Octet*			octet			= &octets[octetX][octetY];

            if(octet->replacementTile && octet->replacementTile->instance)
            {
                if(octet->replacementTile->instance->rootObject)
                {
                    obpoolFreeObjectCallback(octet->replacementTile->instance->rootObject);
                    octet->replacementTile->instance->rootObject = NULL;
                    octet->replacementTile->instance = NULL;
                }
            }

            TerrainObjectSafeDelete(octet->replacementTile);

			for(unsigned int i=0; i < octet->objects.size(); i++)
			{
                TerrainObjectSafeDelete(octet->objects[i]);
			}

			dSpaceDestroy(octet->octetPhysicsSpace);
		}
	}


	if(cellHeightMap)
    {
        if(cellHeightMap->instance)
        {
            if(cellHeightMap->instance->rootObject)
            {
		        obpoolFreeObjectCallback(cellHeightMap->instance->rootObject);
            }
        }

        TerrainObjectSafeDelete(cellHeightMap);
        cellHeightMap = NULL;
    }

    if(newHeightMap)
    {
	    obpoolFreeObjectCallback(newHeightMap);
    }

    if(planes)
    {
        delete [] planes;
        planes = NULL;
    }

    bushclutterdata.erase(bushclutterdata.begin(), bushclutterdata.end());

    obpoolFreeObjectFinish();

	dSpaceDestroy(cellPhysicsSpace);
}


static void BuildSDM(int cellX, int cellY)
{
	SuperCellTerrainData*	cells[3][3];

    worldPhysicsSpace = dHashSpaceCreate(0);

	// We have to build the heightmap for surrounding cells as well...
    // because some buildings are placed in one supercell but leak onto surrounding cells.
	for(int i=0; i < 9; i++)
	{
		int		x = i%3;
		int		y = i/3;
		Vec3f	offset(SDM_SUPERCELL_SIZE * (x-1), SDM_SUPERCELL_SIZE * (y-1), 0);

		cells[x][y] = new SuperCellTerrainData(cellX + (x-1), cellY + (y-1), offset);
	}

	// That's our goal here.
	cells[1][1]->Preprocess();
	cells[1][1]->WriteToSDM();

	for(int i=0; i < 9; i++)
	{
		int		x = i%3;
		int		y = i/3;

		delete cells[x][y];
	}

    ToDeleteTerrainObjects::iterator itr;
    ToDeleteTerrainObjects::iterator free;

    itr = TerrainObjectsToDelete.begin();

    while(itr != TerrainObjectsToDelete.end())
    {
        free = itr;
        ++itr;

        TerrainObjectSafeDelete(*free);
    }

	dSpaceDestroy(worldPhysicsSpace);

	sdmFilesMerged++;
	logFile.seekp(streamUpdatePos);
	logFile << endl << "SDM Files Built: " << sdmFilesMerged;
}


void PrintModelPerformanceSeverity(fstream& logFile)
{
	typedef multimap<float, const PoolModel*>	RemodelingPriorities;
	RemodelingPriorities						remodelingPriorities;


	for(ModelStatsMap::iterator it = modelStatsMap.begin(); it != modelStatsMap.end(); ++it)
	{
		const PoolModel*	poolModel		= it->first;
		int					instanceCount	= it->second.totalInstanceCount;
		int					minNumVertices	= 0;
		int					minNumObjects	= 0;
		float				maxlodRange		= 0;
		float				priority		= 0;

		poolModel->GetMinLODStats(minNumVertices, minNumObjects, maxlodRange);
		if(minNumVertices > 600 || minNumObjects > 3)
		{
			priority = 1 / ((minNumVertices / 600.0f) + minNumObjects + (instanceCount / 100.0f) + (maxlodRange / 800.0f));
			remodelingPriorities.insert(RemodelingPriorities::value_type(priority, poolModel));
		}
	}

	logFile << endl;
	logFile << "========================================" << endl;
	logFile << "====== Model Performance Severity ======" << endl;
	logFile << "========================================" << endl;
	for(RemodelingPriorities::iterator it = remodelingPriorities.begin(); it != remodelingPriorities.end(); ++it)
	{
		if(it->second->modelType != ModelTypes::TERRAIN_TILE)
		{
			logFile << 1 / it->first << ": " << it->second->directory << it->second->modelname << endl;
		}
	}
}


void BuildTerrain(bool mergeAll, short cellX, short cellY)
{
	DEMHDR				terrainHeader;
	char				headerName[MAX_PATH];	trnmainGetMapName(headerName);
	WIN32_FIND_DATAA	findData;
	string				headerFileName	= dtlCpfDir + string(headerName) + string(".hdr");
	string				searchPattern	= dataPath + string(dtlCpfDir) + string("*.dtl");
	HANDLE				search			= FindFirstFileA(searchPattern.c_str(), &findData);
	DWORD				startTime		= timeGetTime();
	const PoolMap*		poolMap			= obMgrGetPoolMap();
	stringstream		modelProblems;


	logFile.open("terrainBuildLog.txt", fstream::out | fstream::trunc);
  if ( logFile.fail() )
  {
    printf("fail\n");
  }
	logFile << fixed << setprecision(2);
	allModelProblems << fixed << setprecision(2);
	modelProblems << fixed << setprecision(2);
	stratErrors << fixed << setprecision(2);
	dtlErrors << fixed << setprecision(2);

	logFile << "Loading Terrain Header... ";
	if(!trnldrReadHdrFile(headerFileName.c_str(), &terrainHeader))
	{
		logFile << "failed!";
		piExitProgram (__FILE__, __LINE__, "As described in terrainBuildLog.txt.");
	}
	else
        logFile << "done." << endl;

	double	x1 = terrainHeader.ulxmap / SUPERCELL_ARC_DEG_X;
	double	y1 = terrainHeader.ulymap / SUPERCELL_ARC_DEG_Y;

	if(x1 < 0.0)
        x1 = floor(x1);
	else
        x1 = ceil(x1);
	if(y1 < 0.0)
        y1 = floor(y1);
	else
        y1 = ceil(y1);

	double x2 = (FLOAT64)(terrainHeader.ncols - 1) * terrainHeader.xdim / SUPERCELL_ARC_DEG_X;
	double y2 = (FLOAT64)(terrainHeader.nrows - 1) * terrainHeader.ydim / SUPERCELL_ARC_DEG_Y;

	int scl = (int)x1;
    int sct = (int)y1;
	int scr = scl + (int)x2;
    int scb = sct - (int)y2;

	TerrainPosition tester(Vec3f(-313697, 3607470, 0));

#if platOS == OS_MACOS
	if(search.dir == 0)
  {
    logFile << "Unable to load DTL files at " << dtlCpfDir << "! Merging process skipped.";
  }
#else
	if(search == INVALID_HANDLE_VALUE)
  {
    logFile << "Unable to load DTL files at " << dtlCpfDir << "! Merging process skipped.";
  }
#endif

	else
	{
		string	dtlFileName, cellXString ,cellYString;

		LoadStratData();
		streamUpdatePos = logFile.tellp();

		if(mergeAll)
		{
			do
			{
				dtlFileName = findData.cFileName;

				if(dtlFileName.find("Default") != string::npos)
				{
					BuildSDM(SUPERCELL_INVALID, SUPERCELL_INVALID);
					continue;
				}

				cellXString = dtlFileName.substr(0, dtlFileName.find("_"));
				cellYString = dtlFileName.substr(dtlFileName.find("_", 0)+1, dtlFileName.find("."));
				cellX		= (short)strtol(cellXString.c_str(), 0, 16);
				cellY		= (short)strtol(cellYString.c_str(), 0, 16);

				if(cellX >= scl && cellX <= scr)
                {
                    if(cellY >= scb && cellY <= sct)
                    {
                        BuildSDM(cellX, cellY);
                    }
                }

#if platOS == OS_WIN32
                MSG msg;
                // May want to alt-tab out while building....
                while(PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
                {
                    if(GetMessage(&msg, NULL, 0, 0))
                    {
                        TranslateMessage(&msg);
                        DispatchMessage(&msg);
                    }
                }
#endif
			}
			while(FindNextFileA(search, &findData));
		}
		else
        {
            BuildSDM(cellX, cellY);
        }

		FindClose(search);

		for(CellStratMap::iterator it = cellStratMap.begin(); it != cellStratMap.end(); ++it)
		{
			CellStratData*		cellStratData = &it->second;

			for(CPMap::iterator itr = cellStratData->chokepoints.begin(); itr != cellStratData->chokepoints.end(); ++itr)
			{
				memFree(itr->second);
			}

			for(FacilityMap::iterator itr = cellStratData->facilities.begin(); itr != cellStratData->facilities.end(); ++itr)
			{
				memFree(itr->second);
			}

			for(BuildingMap::iterator itr = cellStratData->buildings.begin(); itr != cellStratData->buildings.end(); ++itr)
			{
				memFree(itr->second);
			}

            for(vector<DEPOT_LINK*>::iterator lStratDataIt = cellStratData->links.begin(); lStratDataIt != cellStratData->links.end(); ++lStratDataIt)
			{
				memFree(*lStratDataIt);
			}
			for(vector<WAYPOINT*>::iterator wStratDataIt = cellStratData->waypoints.begin(); wStratDataIt != cellStratData->waypoints.end(); ++wStratDataIt)
			{
				memFree(*wStratDataIt);
			}
		}
	}


	logFile << endl << endl;
	logFile << "build process completed in: " << (timeGetTime() - startTime) / 1000 << " seconds."  << endl << endl;
	logFile << "========================================" << endl;
	logFile << "============ Missing Models ============" << endl;
	logFile << "========================================" << endl;
	for(set<string>::iterator it = missingModels.begin(); it != missingModels.end(); ++it)
	{
		logFile << *it << endl;
	}
	logFile << endl;
	missingModels.clear();

	logFile << "========================================" << endl;
	logFile << "============ Unused Models =============" << endl;
	logFile << "========================================" << endl;
	for(PoolMap::const_iterator poolMapIt = poolMap->begin(); poolMapIt != poolMap->end(); ++poolMapIt)
	{
		const PoolModel*	poolModel = poolMapIt->second;

		if(	poolModel->modelType != ModelTypes::GENERAL	&&
			poolModel->modelType != ModelTypes::VEHICLE	&&
			poolModel->modelType != ModelTypes::CLOUD )
		{
			if(modelStatsMap.find(poolModel) == modelStatsMap.end())
			{
				logFile << poolModel->directory << poolModel->modelname << endl;
			}
		}
	}
	logFile << endl;

	logFile << "========================================" << endl;
	logFile << "====== Missing Place 11 Top-Bases ======" << endl;
	logFile << "========================================" << endl;
	for(set<PoolModel*>::iterator it = missingTopBases.begin(); it != missingTopBases.end(); ++it)
	{
		logFile << (*it)->directory << (*it)->modelname << endl;
	}
	logFile << endl;
	missingTopBases.clear();

	logFile << "===========================================" << endl;
	logFile << "====== Missing Place 11 Bottom-Bases ======" << endl;
	logFile << "===========================================" << endl;
	for(set<PoolModel*>::iterator it = missingBottomBases.begin(); it != missingBottomBases.end(); ++it)
	{
		logFile << (*it)->directory << (*it)->modelname << endl;
	}
	logFile << endl;
	missingBottomBases.clear();

	logFile << "========================================" << endl;
	logFile << "======== Strat Parsing Warnings ========" << endl;
	logFile << "========================================" << endl;
	logFile << stratErrors.rdbuf();
	logFile << endl;
	stratErrors.clear();
	stratErrors.str("");

	logFile << "========================================" << endl;
	logFile << "========= DTL Parsing Warnings =========" << endl;
	logFile << "========================================" << endl;
	logFile << dtlErrors.rdbuf();
	dtlErrors.clear();
	dtlErrors.str("");

	logFile << endl;
	logFile << "========================================" << endl;
	logFile << "============= Model Stats ==============" << endl;
	logFile << "========================================" << endl;
	for(ModelStatsMap::iterator it = modelStatsMap.begin(); it != modelStatsMap.end(); ++it)
	{
		float				maxSlopeDelta		= 0;
		float				maxCappedDelta		= 0;
		const PoolModel*	poolModel			= it->first;

		if(poolModel->propertyFlags & PoolModel::PropertyFlags::HOLLOW_REFERENCE)	continue;
		if(poolModel->modelType == ModelTypes::TERRAIN_TILE)	continue;
		if(it->second.slopeDeltaList.empty() == false)	maxSlopeDelta = it->second.slopeDeltaList[0];
		if(it->second.slopeDeltaList.size() > 10)	maxCappedDelta = it->second.slopeDeltaList[10];

		logFile << poolModel->directory << poolModel->modelname;
		logFile << ": total instance count(" << it->second.totalInstanceCount << ")";
		if(poolModel->modelType != ModelTypes::TERRAIN_TILE && poolModel->placeMode != OBJVAL_NO_PLACEMODE)
		{
			logFile << "; steep places(" << it->second.steepPlaces;
			logFile << "); non-planar places(" << it->second.nonPlanarCount;
			logFile << "); max slope delta(" << maxSlopeDelta;
			logFile << "); max capped slope delta:(" << maxCappedDelta << ")";
		}

		logFile << endl;
	}

	logFile << endl;
	logFile << "========================================" << endl;
	logFile << "============ Model Problems ============" << endl;
	logFile << "========================================" << endl;
	for(ModelStatsMap::iterator it = modelStatsMap.begin(); it != modelStatsMap.end(); ++it)
	{
		float				maxSlopeDelta	= 0;
		float				maxCappedDelta	= 0;
		const PoolModel*	poolModel		= it->first;
		BoundingBox			baseBounds;
		bool				hasBase = true;
		float				baseUndersize	= 0;
		float				baseOversize	= 0;

		if(poolModel->propertyFlags & PoolModel::PropertyFlags::HOLLOW_REFERENCE)	continue;
		if(poolModel->modelType == ModelTypes::TERRAIN_TILE)	continue;
		if(it->second.slopeDeltaList.empty() == false)	maxSlopeDelta = it->second.slopeDeltaList[0];
		if(it->second.slopeDeltaList.size() > 10)	maxCappedDelta = it->second.slopeDeltaList[10];

		if(	poolModel->placeMode != OBJVAL_NO_PLACEMODE		&&
			poolModel->placeMode != OBJVAL_BOTTOM_CONFORM	&&
			poolModel->placeMode != OBJVAL_PLACE_BASE		&&
			poolModel->modelType != ModelTypes::TERRAIN_TILE )
		{
			if(poolModel->bounds.zMax() > 0.01)	modelProblems << "; underground";
		}

		if(	poolModel->placeMode == OBJVAL_BOTTOM_CONFORM	||
			poolModel->placeMode == OBJVAL_PLACE_BASE	)
		{
			if(it->second.nonPlanarCount < 11)	modelProblems << "; improper place mode";

			if(	!poolModel->GetSubObjectBoundsByName(&baseBounds, "bottombase", 0)	&&
				!poolModel->GetSubObjectBoundsByComment(&baseBounds, "bottombase", 0)	)
			{
				if(	!poolModel->GetSubObjectBoundsByName(&baseBounds, "topbase", 0)	&&
					!poolModel->GetSubObjectBoundsByComment(&baseBounds, "topbase", 0)	)
				{
					modelProblems << "; base object missing";
					hasBase = false;
				}
			}
			if(hasBase)
			{
				baseUndersize = maxCappedDelta - baseBounds.zMax();
				baseOversize = baseBounds.zMax() - maxSlopeDelta;
				if(baseUndersize > 0.01) modelProblems << "; base is " << baseUndersize << " units too short";
				if(baseOversize > 0.01) modelProblems << "; base is " << baseOversize << " units too high";
			}
		}
		else if(poolModel->placeMode != OBJVAL_PLACE_CONCAVE && it->second.nonPlanarCount > 11)	modelProblems << "; improper place mode";


		if(	poolModel->placeMode == OBJVAL_PLACE_DOWN	||
			poolModel->placeMode == OBJVAL_PLACE_SHEARED	)
		{
			if(poolModel->modelType == ModelTypes::BUILDING && it->second.steepPlaces > 11)	modelProblems << "; too many steep places for sheared building";
		}

		if(	poolModel->placeMode < OBJVAL_PLACE_DOWN	||
			(poolModel->placeMode > OBJVAL_PLACE_DOWN && poolModel->placeMode < OBJVAL_OCTET_CONFORMRADIAL) )
		{
			modelProblems << "; deprecated place mode";
		}

		if(modelProblems.str().length() != 0)
		{
			allModelProblems << poolModel->directory << poolModel->modelname << modelProblems.str() << endl;
			modelProblems.clear();
			modelProblems.str("");
		}
	}
	logFile << allModelProblems.str() << endl;
	allModelProblems.clear();
	allModelProblems.str("");

	PrintModelPerformanceSeverity(logFile);

	logFile.close();

	cellStratMap.clear();
	modelStatsMap.clear();
}
