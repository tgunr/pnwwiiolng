#pragma once

#include "ob/obmain.h"
#include "trn/trntypes.h"
#include "strt/strtDTL.h"
#include "sdm/sdmHash.h"
#include "sdm/sdmObjInfo.h"
#include "rn/rnvis.h"
#include "wwii/wwiiHash.h"
#include <set>
#include <map>
#include <vector>

#define WANT_NON_REPLACEMENT_TILES 0
#define USE_HEIGHT_AT 1

	struct trncdDTLCollider;

	using TDTL_TREES = std::unordered_set<UINT32>;

	struct TERRAIN_DETAIL
	{
		SINT16			scx;							// super cell x
		SINT16			scy;							// super cell y
		UINT16			flags;							// flags for actions taken
		UINT32			dataversion;					// data version using simple incremental technique
		TRNSTATEBLOCK	trnstate;						// state block used for sequenced loading/unloading of terrain
		struct OBJECT*	terrain;						// terrain object
		struct OBJECT*	terrainorig;					// terrain object before textures
		struct OBJECT*	nonreplacementtiles[MAX_TERRAIN_TEXTURES]; // texture objects
		SINT16			numoctetsacross;				// number of octets across super cell
		SINT16			numoctetsdown;					// number of octets down
		SINT16			numheightmapelements;			// count of points for terrain
		SINT16			heightmap[SUPERCELL_SIZE_XY];	// points
		SINT16			nmcnt;							// count of normals for terrain
		SINT16			nm[SUPERCELL_SIZE_XY];			// normals
		SINT16			tricnt;							// triangle texture name counter
		SINT8*			triname[MAX_TERRAIN_TEXTURES];	// triangle texture names
		SINT16			trindx;							// used triangle slot counter
		TERRAIN_INFO	trinfo[MAX_TERRAIN_EDIT_LIST];	// used triangle information
		SINT16			tobndx;							// object slot counter
		SDM_ObjInfo		trnobj[MAX_TERRAIN_OCTETS * MAX_TRNOBJ_OCTET];	// objects in this super cell
		const char* trnobjRefs[MAX_TERRAIN_OCTETS * MAX_TRNOBJ_OCTET]; // references for quicker unload
		UINT32*			toldflags;						// object loaded flags

		// strat pointers
		SINT32			numCPs;
		SINT32			numFacilities;
		SINT32			numBuildings;
		SINT32			numLinks;
		SINT32			numWaypoints;
		CP*				pcp;		// CPs
		FACILITY*		pfac;		// Facilities
		BUILDING*		pbld;		// Building
		DEPOT_LINK*		plink;		// Link
		WAYPOINT*		pwp;		// Waypoint
		STRAT_BUILDING_LIST*			buildingTable;
		STRAT_FACILITY_TABLE*		facTable;
		STRAT_BUILDING_HASH			bldHash;
		LOGICAL			stratLoaded;

#if USE_HEIGHT_AT
		PLANE           plane[SUPERCELL_OCTETS_XY][8];
#endif

		struct trncdDTLCollider* collider;

        TDTL_TREES*		m_trees;

#ifdef SPATIAL_DB
//        void*           sdbInfo;
//        void*           sdbDatabase;
#endif

	};

	typedef LOGICAL (*TRN_OBJECT_NOTIFY)(struct OBJECT* lpobj, SINT32 scx, SINT32 scy, UINT32 id, REL_OBJ_ID * ObjId);

	extern LOGICAL trnmainInit(SINT8* name, FLOAT32 xorigin, FLOAT32 yorigin);
	extern void trnmainKill(void);
	extern LOGICAL trnmainSetTerrainMatrix(OBJECT* lpobj);
	extern LOGICAL trnmainGetDimensions(FLOAT64* left, FLOAT64* top, FLOAT64* width, FLOAT64* height);
	extern LOGICAL trnmainGetHorizon(FLOAT32* reshorizon);
	extern LOGICAL trnmainUpdate(void);
	extern LOGICAL trnmainDraw();
	extern LOGICAL trnmainLODChange(void);
	extern LOGICAL trnmainDrawLastFrame(OBJECT* lpobj);
	extern LOGICAL trnmainAddAltTexture(OBJECT** retobj, OBJECT* lpobj, SINT32 octet, const SINT8* texname);
	extern LOGICAL trnmainAddToObject(OBJECT* newobj, OBJECT* lpobj, SINT32 poly, SINT32 tonum, SINT32 drag, SINT32 roughness, SINT32 style, SINT32 orientation);
	extern LOGICAL trnmainDrawObject(OBJECT* lpobj);
	extern LOGICAL trnmainGetMapName(SINT8* mapname);
	extern LOGICAL trnmainAddObject(const char* objname, piVECTOR* pos, OBJECT* lpmain,
	                                SINT32 StratSuperCellX, SINT32 StratSuperCellY, UINT32 StratId,
	                                REL_OBJ_ID * ObjId,
	                                SINT32 drag, SINT32 roughness,
	                                SINT32 style, SINT32 orientation, SINT32 sc, bool nonPlanarBase,
	                                const SDM_ObjInfo& objInfo, TERRAIN_DETAIL* td,
	                                const char** objRefName);

	extern LOGICAL trnmainSetObjectCreationCallback(TRN_OBJECT_NOTIFY callback);
	extern LOGICAL trnmainSetObjectDeletionCallback(TRN_OBJECT_NOTIFY callback);
	extern LOGICAL trnmainPositionToLatLong(FLOAT64 x, FLOAT64 y, SINT32* latdeg, SINT32* latmin, SINT32* latsec, SINT32* north, SINT32* longdeg, SINT32* longmin, SINT32* longsec, SINT32* east);
	extern LOGICAL trnmainLatLongToPosition(SINT32 latdeg, SINT32 latmin, SINT32 latsec, SINT32 north, SINT32 longdeg, SINT32 longmin, SINT32 longsec, SINT32 east, FLOAT64* x, FLOAT64* y);
	extern LOGICAL trnGetWorldOriginOffset(FLOAT64VECTOR * Offset);
	extern LOGICAL trnCheckAndAdjustCameraPositionWithOriginOffset(piVECTOR * CameraPos);
	extern UINT32 trnUpdateTerrainOriginOffset(LOGICAL force);
	extern LOGICAL trnmainWorldOriginMoved(void);
	extern LOGICAL trnmainDoRawUpdate(LOGICAL ForceCompleteLoad);
	extern TERRAIN_DETAIL* trnGetTerainDetail(SINT16 scx, SINT16 scy);
	extern LOGICAL trnFindStratBuildingByTO(struct BUILDING** const bld, struct trnCDTerrainObject* const TO);

	extern LOGICAL trnmainSetLightingUpdate(LOGICAL newLightingUpdate);

	extern SINT32 trnmainGetNumDrawnTriangles(void);

	// called whenever a terain object is loaded or unloaded to
	// copy strat data and update bldHash
	extern SINT32 trnUpdateStratData(TERRAIN_DETAIL* td, OBJECT* lpobj, SINT32 bid, LOGICAL loaded);
	extern SINT32 trnSetStratData(TERRAIN_DETAIL* td, SINT32 bid, SINT32 country = -1, SINT32 level = -1);
	extern LOGICAL trnGetStratData(SINT16 scx, SINT16 scy, SINT32 id, SINT32* country, SINT32* level);

    extern void trnmainSetDraw(LOGICAL bOn);
    extern void trnmainDrawOctetTriangles(LOGICAL draw);
    extern bool trnmainIsReplacement(const FLOAT64VECTOR& pos);

    extern float trnmainHeightAt(TERRAIN_DETAIL* dtl, double x, double y);
    extern PLANE* trnmainGetPlaneAt(TERRAIN_DETAIL* dtl, double x, double y);

	extern  void trnmainGetShadowReceivers(std::map<int, std::vector<piVECTOR> >& octets,
                               std::map<int, OBJECT*>& objects);

    extern void trnmainGetSupercellAndOctet(const Vec3d& pos, int& sx, int& sy, int& r, int& c);
