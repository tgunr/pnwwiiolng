#ifndef SDM_H
#define SDM_H

#include <inttypes.h>

/************************************************************************************************
	sdm
*************************************************************************************************/

#define SDM_SUPERCELL_SIZE_X		33
#define SDM_SUPERCELL_SIZE_Y		33
#define SDM_SUPERCELL_SIZE_X1		32
#define SDM_SUPERCELL_SIZE_Y1		32
#define SDM_SUPERCELL_SIZE_XY		(SDM_SUPERCELL_SIZE_X * SDM_SUPERCELL_SIZE_Y)
#define SDM_SUPERCELL_SIZE_XY1		(SDM_SUPERCELL_SIZE_X * SDM_SUPERCELL_SIZE_Y)
#define SDM_SUPERCELL_OCTETS_X		16
#define SDM_SUPERCELL_OCTETS_Y		16
#define SDM_SUPERCELL_OCTETS_X1		17
#define SDM_SUPERCELL_OCTETS_Y1		17
#define SDM_SUPERCELL_OCTETS_XY		(SDM_SUPERCELL_OCTETS_X  * SDM_SUPERCELL_OCTETS_Y)
#define SDM_SUPERCELL_OCTETS_XY1	(SDM_SUPERCELL_OCTETS_X1 * SDM_SUPERCELL_OCTETS_Y1)
#define SDM_VERTICES_PER_OCTET		9
#define SDM_TRIANGLES_PER_OCTET		8
#define SDM_TERRAIN_TRIANGLES		2048

#define SDM_MAX_TERRAIN_TEXTURES		32
#define SDM_MAX_TERRAIN_EDIT_LIST		(SDM_MAX_TERRAIN_OCTETS * 3)
#define SDM_MAX_TRNOBJ_OCTET			8
#define SDM_NUM_TRIANGLES_PER_OCTET		8


#define SDM_FLAG_DATA_RLE			0x0001
#define SDM_FLAG_NORMALS_RLE		0x0002

#define SDM_MAX_STRAT_LIST			1024 // just some arbitrarily large value
#define SDM_MAX_STRT_NAME_LENGTH	64
#define SDM_MAX_OBJNAME_LENGTH		32

#define SDM_SUPERCELL_SIZE				12800.0f
#define SDM_OCTET_SIZE					800.0f
#define SDM_OCTET_SIZE_2				400.0f

#define SUPERCELL_NORMAL_MASK_X	0xf800
#define SUPERCELL_NORMAL_MASK_Y	0x07c0
#define SUPERCELL_NORMAL_MASK_Z	0x003f
#define SUPERCELL_NORMAL_MAG_X	16.0f
#define SUPERCELL_NORMAL_MAG_Y	16.0f
#define SUPERCELL_NORMAL_MAG_Z	63.0f

#define SDM_WORLD_ARC_WIDTH			180.0f
#define SDM_WORLD_ARC_HEIGHT		70.1875f
#define SDM_WORLD_WIDTH				8640000.0f
#define SDM_WORLD_HEIGHT			4211200.0f

#define SDM_LON_SIZE (SDM_SUPERCELL_SIZE * SDM_WORLD_ARC_WIDTH / SDM_WORLD_WIDTH)
#define SDM_LAT_SIZE (SDM_SUPERCELL_SIZE * SDM_WORLD_ARC_HEIGHT / SDM_WORLD_HEIGHT)

#define SDM_LON_SPACING (SDM_LON_SIZE/(double)SDM_SUPERCELL_SIZE_X1)
#define SDM_LAT_SPACING (SDM_LAT_SIZE/(double)SDM_SUPERCELL_SIZE_Y1)

// objClass
enum
{
	SDM_CLASS_CP=1,
	SDM_CLASS_FACILITY,
	SDM_CLASS_BUILDING,
	SDM_CLASS_LINK,
	SDM_CLASS_WAYPOINT,
	SDM_MAX_STRAT_CLASSES
};

// cp objTypes
enum
{
	SDM_CP_GENERIC=1,
	SDM_CP_SMALL,
	SDM_CP_MEDIUM,
	SDM_CP_LARGE,
	SDM_CP_BRIDGE,
	SDM_MAX_CP_TYPES
};

// Facility objTypes
enum
{
	SDM_FACILITY_CITY=1,
	SDM_FACILITY_FACTORY,
	SDM_FACILITY_BASE,
	SDM_FACILITY_DEPOT,
	SDM_FACILITY_OMT,
	SDM_FACILITY_SHIPYARD,
	SDM_FACILITY_FIREBASE,
	SDM_FACILITY_AIRBASE,
	SDM_FACILITY_ARMYBASE,
	SDM_FACILITY_NAVALBASE,
	SDM_MAX_FACILITY_TYPES
};

// factory objTypes
enum
{
	SDM_FACTORY_GENERIC = 1,
	SDM_FACTORY_MUNITIONS,
	SDM_FACTORY_AIRCRAFT,
	SDM_FACTORY_OIL_REFINERY,
	SDM_FACTORY_CHEMICAL_PLANT,
	SDM_FACTORY_MACHINERY,
	SDM_FACTORY_VEHICLE,
	SDM_MAX_FACTORY_SUBTYPES
};

// Depot
enum
{
	SDM_DEPOT_GENERIC=1,
	SDM_DEPOT_TRAIN,
	SDM_DEPOT_TRUCK,
	SDM_DEPOT_BARGE,
	SDM_DEPOT_CONVOY,
	SDM_DEPOT_PACK_ANIMAL,
	SDM_DEPOT_WAGON,
	SDM_DEPOT_PORTER,
	SDM_MAX_DEPOT_SUBTYPE
};

// OMT
enum
{
	SDM_OMT_GENERIC=1,
	SDM_OMT_RAIL,
	SDM_OMT_ROAD,
	SDM_OMT_SEA,
	SDM_MAX_OMT_SUBTYPES
};

// building
enum
{
	SDM_BUILDING_GENERIC=1,
	SDM_BUILDING_RESEARCH,
	SDM_BUILDING_SPAWN,
	SDM_BUILDING_AUTOGUNNER,
	SDM_BUILDING_CAPTURE,
	SDM_MAX_BUILDING_TYPES
};

// autogunner
enum
{
	SDM_AUTOGUNNER_MGNEST=1,
	SDM_AUTOGUNNER_RIFLEMAN,
	SDM_AUTOGUNNER_RIFLEPIT,
	SDM_AUTOGUNNER_MORTARPIT,
	SDM_AUTOGUNNER_ARTILLERYLIGHT,
	SDM_AUTOGUNNER_ARTILLERYMEDIUM,
	SDM_AUTOGUNNER_ARTILLERYHEAVY,
	SDM_AUTOGUNNER_COASTDEFENSE,
	SDM_AUTOGUNNER_FORTCUPOUT,
	SDM_AUTOGUNNER_AAALIGHT,
	SDM_AUTOGUNNER_AAAMEDIUM,
	SDM_AUTOGUNNER_AAAHEAVY,
	SDM_AUTOGUNNER_TORPEDO,
	SDM_AUTOGUNNER_ATLIGHT,
	SDM_AUTOGUNNER_ATMEDIUM,
	SDM_AUTOGUNNER_ATHEAVY,
	SDM_MAX_AUTOGUNNER_SUBTYPES
};

// waypoint
enum
{
	SDM_WAYPOINT_NORMAL=1,
	SDM_WAYPOINT_BRIDGE=2,
	SDM_MAX_WAYPOINT_TYPES
};

enum
{
	SDM_STRAT_MODIFIED=1,
	SDM_STRAT_CONTROL=2,
	SDM_MAX_STRAT_FLAGS
};

enum
{
	SDM_TRNOBJ_NON_PLANAR_BASE = 0x1
};

typedef struct
{
	uint16_t company;
	uint16_t filetype;
	uint16_t version;
	uint16_t flags;
	uint32_t dataversion;
	uint32_t length;
} sdm_header;

typedef struct sdm_dtl_data
{
	int16_t zcnt;
	int16_t z[SDM_SUPERCELL_SIZE_XY];
	int16_t normcnt;
	int16_t norm[SDM_SUPERCELL_SIZE_XY];
} sdm_dtl_data;

typedef struct
{
    uint16_t        octet;			// octet number
    uint16_t        octnum;			// original octet number
    uint8_t			tonum;			// original texture object number
    uint8_t			txndx;			// texture name index
    uint8_t			ortdrg;			// orientation/drag LSN/MSN
    uint8_t			ruftrn;			// roughness/terrain LSN/MSN
} sdm_terrain_info;

typedef struct
{
    float   pos[3];			// object position
    char    objname[32];	// object name in pool
    uint8_t txndx;			// texture object index
    uint8_t ortdrg;			// orientation/drag LSN/MSN
    uint8_t ruftrn;			// roughness/terrain LSN/MSN
    uint8_t flags;			// 
} sdm_trnobj_info;
    
typedef struct sdm_replacement_octet_data
{
    int numtex;
    char texname[SDM_MAX_TERRAIN_TEXTURES][32];

    int numtrninfo;
    sdm_terrain_info    trninfo[SDM_TERRAIN_TRIANGLES];

    int numobjinfo;
    sdm_trnobj_info     objinfo[SDM_SUPERCELL_OCTETS_XY * SDM_MAX_TRNOBJ_OCTET];
} sdm_replacement_octet_data;

typedef struct
{
	uint16_t numCPs;
	uint16_t numFacilities;
	uint16_t numBuildings;    
	uint16_t numLinks;
	uint16_t numWaypoints;
	uint16_t reserved1;
	uint16_t reserved2;
	uint16_t reserved3;
} sdm_strat_header;

typedef struct 
{
	uint16_t	id;
	int8_t		objClass;
	int8_t		objType;
} sdm_rel_obj_id;

typedef struct 
{
	int16_t	ox;
	int16_t	oy;
} sdm_octet;


struct sdm_lookup_id
{
	int16_t	ox;
	int16_t	oy;
	uint16_t id;
};

typedef struct sdm_full_obj_id
{
	sdm_octet	cell;				// octet (get supercell by >> 4)
	uint16_t	id;					// id within the supercell
	int8_t		objClass;			// class is STRAT_CLASS
	int8_t		objType;			// type
#ifdef __cplusplus
	sdm_full_obj_id& init() { cell.ox = cell.oy = 0; id = 0; objClass = objType = 0; return *this; }
	bool valid() const { return (objClass > 0 && objType > 0); }
	int sx() const { return cell.ox >> 4; }
	int sy() const { return cell.oy >> 4; }
	void clearOctetBits() { cell.ox &= 0xfff0; cell.oy &= 0xfff0;} // grr
	sdm_lookup_id const& lookup_id() const { return *(sdm_lookup_id*)this; }
#endif
} sdm_full_obj_id;

typedef struct
{
	sdm_rel_obj_id	oid;			// object ID of this CP
	uint32_t		serial;			// supercell relative serial number

	int32_t		posx;				// X Position in supercell (in meters)
	int32_t		posy;				// Y Position in supercell (in meters)

	int8_t		subType;			// Subtype of REL_OBJ_ID

	int8_t		side;				// Side of the CP
	int8_t		country;			// Country of the CP

	int8_t		stratFlags;			// general strat object flags

	int8_t		name[SDM_MAX_STRT_NAME_LENGTH];
	int8_t		fileName[SDM_MAX_OBJNAME_LENGTH];

} sdm_cp;

typedef struct
{
	sdm_rel_obj_id	oid;			// Facility ID of this facility
	uint32_t		serial;			// supercell relative serial number

	sdm_full_obj_id	cpid;			// Parent CP ID
	int32_t		posx;				// X Position in supercell (in meters)
	int32_t		posy;				// Y Position in supercell (in meters)

	int8_t		subType;			// Subtype of REL_OBJ_ID

	int8_t		originalSide;		// Side of the Facility
	int8_t		originalCountry;	// Country of the Facility

	int8_t		stratFlags;			// general strat object flags

	int8_t		name[SDM_MAX_STRT_NAME_LENGTH];
	int8_t		fileName[SDM_MAX_OBJNAME_LENGTH];

} sdm_facility;

typedef struct
{
	sdm_rel_obj_id	oid;			// object ID of this object
	uint32_t		serial;			// supercell relative serial number

	sdm_rel_obj_id	pid;			// Parent object ID
	int32_t		posx;				// X Position in supercell (in meters)
	int32_t		posy;				// Y Position in supercell (in meters)

	int8_t		subType;			// Subtype of REL_OBJ_ID

	int8_t		control;			// Control building or not - only 1 building

	int8_t		stratFlags;			// general strat object flags

	int8_t		name[SDM_MAX_STRT_NAME_LENGTH];
	int8_t		fileName[SDM_MAX_OBJNAME_LENGTH];

} sdm_building;

typedef struct
{
	sdm_rel_obj_id	oid;			// object ID of this link
	uint32_t		serial;			// supercell relative serial number

	sdm_full_obj_id	startDepotId;   // starting depot Building ID
	sdm_full_obj_id	endDepotId;		// ending depot Building ID
	sdm_full_obj_id firebaseId;		// optional firebase associated with this link

	int32_t		distance;			// Length of Link

	int8_t		stratFlags;			// general strat object flags

	int8_t		name[SDM_MAX_STRT_NAME_LENGTH];
	
} sdm_depot;

typedef struct
{
	sdm_rel_obj_id	oid;			// object ID of this waypoint
	uint32_t		serial;			// supercell relative serial number

	sdm_rel_obj_id	pid;			// Parent object ID (to bridge?) (object attatchment)
	int32_t		posx;				// X Position in supercell (in meters)
	int32_t		posy;				// Y Position in supercell (in meters)

	int8_t		subType;			// Subtype of REL_OBJ_ID

	sdm_full_obj_id	nextpt;			// WAY_POINT ID of next waypoint
	sdm_full_obj_id	lid;			// Link ID

	int32_t		distance;			// distance from last waypoint over terrain.

	int8_t		stratFlags;			// general strat object flags

} sdm_waypoint;

// this is the data structure of doom!
// don't put this on the stack!
typedef struct sdm_strat_data
{
	int numcps;
	int numfacs;
	int numblds;
	int numdepots;
	int numwaypts;
	
	sdm_cp			cp[SDM_MAX_STRAT_LIST];
	sdm_facility	facility[SDM_MAX_STRAT_LIST];
	sdm_building	building[SDM_MAX_STRAT_LIST];
	sdm_depot		depot[SDM_MAX_STRAT_LIST];
	sdm_waypoint	waypoint[SDM_MAX_STRAT_LIST];
} sdm_strat_data;


typedef int (*sdm_read)(int, void*, int);
typedef int (*sdm_seek)(int fd, int offset);

/************************************************************************************************
	sdm
*************************************************************************************************/
// file needs to be positioned at start of data before each call (which it will be if you call
// these 3 functions sequentially in the order below, otherwise you'll need to do it yourself)

int sdm_read_dtl_data			(int fd, sdm_read, sdm_dtl_data*);
int sdm_read_replacement_data   (int fd, sdm_read, sdm_replacement_octet_data*);
int sdm_read_strat_data			(int fd, sdm_read, sdm_strat_data*);

// to seek from start of sdm to replacement data
int sdm_seek_replacement_data(int fd, sdm_read, sdm_seek);
// to seek from start of replacement data to strat data
int sdm_seek_strat_data(int fd, sdm_read, sdm_seek);

#endif // SDM_H
