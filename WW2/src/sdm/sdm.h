#pragma once

#include "trn/trntypes.h"
#include "sdm/sdmDefines.h"
#include "sdm/sdmEnums.h"

/************************************************************************************************
	sdm
*************************************************************************************************/

struct sdm_header
{
	UINT16 company;
	UINT16 filetype;
	UINT16 version;
	UINT16 flags;
	UINT32 dataversion;
	UINT32 length;
};

struct sdm_dtl_data
{
	SINT16 zcnt;
	SINT16 z[SDM_SUPERCELL_SIZE_XY];
	SINT16 normcnt;
	SINT16 norm[SDM_SUPERCELL_SIZE_XY];
};

struct sdm_strat_header
{
	UINT16 numCPs;
	UINT16 numFacilities;
	UINT16 numBuildings;
	UINT16 numLinks;
	UINT16 numWaypoints;
	UINT16 reserved1;
	UINT16 reserved2;
	UINT16 reserved3;
};

struct sdm_rel_obj_id
{
	UINT16	id;
	SINT8		objClass;
	SINT8		objType;
};

struct sdm_octet
{
	SINT16	ox;
	SINT16	oy;
};

struct sdm_lookup_id
{
	SINT16	ox;
	SINT16	oy;
	UINT16 id;
};

struct sdm_full_obj_id
{
	sdm_octet	cell;				// octet (get supercell by >> 4)
	UINT16	id;					// id within the supercell
	SINT8		objClass;			// class is STRAT_CLASS
	SINT8		objType;			// type

#ifdef __cplusplus
	sdm_full_obj_id& init() { cell.ox = cell.oy = 0; this->id = 0; objClass = objType = 0; return *this; }
	bool valid() const { return (objClass > 0 && objType > 0); }
	int sx() const { return cell.ox >> 4; }
	int sy() const { return cell.oy >> 4; }
	void clearOctetBits() { cell.ox &= 0xfff0; cell.oy &= 0xfff0;} // grr
	sdm_lookup_id const& lookup_id() const { return *(sdm_lookup_id*)this; }
#endif

};

struct sdm_cp
{
	sdm_rel_obj_id	oid;			// object ID of this CP
	UINT32		serial;			// supercell relative serial number

	SINT32		posx;				// X Position in supercell (in meters)
	SINT32		posy;				// Y Position in supercell (in meters)

	SINT8		subType;			// Subtype of REL_OBJ_ID

	SINT8		side;				// Side of the CP
	SINT8		country;			// Country of the CP

	SINT8		stratFlags;			// general strat object flags

	SINT8		name[SDM_MAX_STRT_NAME_LENGTH];
	SINT8		fileName[SDM_MAX_OBJNAME_LENGTH];

};

struct sdm_facility
{
	sdm_rel_obj_id	oid;			// Facility ID of this facility
	UINT32		serial;			// supercell relative serial number

	sdm_full_obj_id	cpid;			// Parent CP ID
	SINT32		posx;				// X Position in supercell (in meters)
	SINT32		posy;				// Y Position in supercell (in meters)

	SINT8		subType;			// Subtype of REL_OBJ_ID

	SINT8		originalSide;		// Side of the Facility
	SINT8		originalCountry;	// Country of the Facility

	SINT8		stratFlags;			// general strat object flags

	SINT8		name[SDM_MAX_STRT_NAME_LENGTH];
	SINT8		fileName[SDM_MAX_OBJNAME_LENGTH];

};

struct sdm_building
{
	sdm_rel_obj_id	oid;			// object ID of this object
	UINT32		serial;			// supercell relative serial number

	sdm_rel_obj_id	pid;			// Parent object ID
	SINT32		posx;				// X Position in supercell (in meters)
	SINT32		posy;				// Y Position in supercell (in meters)

	SINT8		subType;			// Subtype of REL_OBJ_ID

	SINT8		xcontrol;			// Control building or not - only 1 building

	SINT8		stratFlags;			// general strat object flags

	SINT8		name[SDM_MAX_STRT_NAME_LENGTH];
	SINT8		fileName[SDM_MAX_OBJNAME_LENGTH];

};

struct sdm_depot
{
	sdm_rel_obj_id	oid;			// object ID of this link
	UINT32		serial;			// supercell relative serial number

	sdm_full_obj_id	startDepotId;   // starting depot Building ID
	sdm_full_obj_id	endDepotId;		// ending depot Building ID
	sdm_full_obj_id firebaseId;		// optional firebase associated with this link

	SINT32		distance;			// Length of Link

	SINT8		stratFlags;			// general strat object flags

	SINT8		name[SDM_MAX_STRT_NAME_LENGTH];

};

struct sdm_waypoint
{
	sdm_rel_obj_id	oid;			// object ID of this waypoint
	UINT32		serial;			// supercell relative serial number

	sdm_rel_obj_id	pid;			// Parent object ID (to bridge?) (object attatchment)
	SINT32		posx;				// X Position in supercell (in meters)
	SINT32		posy;				// Y Position in supercell (in meters)

	SINT8		subType;			// Subtype of REL_OBJ_ID

	sdm_full_obj_id	nextpt;			// WAY_POINT ID of next waypoint
	sdm_full_obj_id	lid;			// Link ID

	SINT32		distance;			// distance from last waypoint over terrain.

	SINT8		stratFlags;			// general strat object flags

};

// this is the data structure of doom!
// don't put this on the stack!
struct sdm_strat_data
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
};

/************************************************************************************************
	sdm
*************************************************************************************************/
typedef size_t (*sdm_read)(int, void*, size_t);
typedef size_t (*sdm_seek)(int fd, size_t offset);

// file needs to be positioned at start of data before each call (which it will be if you call
// these 3 functions sequentially in the order below, otherwise you'll need to do it yourself)

extern int sdm_read_dtl_data			(int fd, sdm_read, struct sdm_dtl_data*);
extern int sdm_read_replacement_data   (int fd, sdm_read, struct sdm_replacement_octet_data*);
extern int sdm_read_strat_data			(int fd, sdm_read, struct sdm_strat_data*);

// to seek from start of sdm to replacement data
extern int sdm_seek_replacement_data(int fd, sdm_read, sdm_seek);
// to seek from start of replacement data to strat data
extern int sdm_seek_strat_data(int fd, sdm_read, sdm_seek);

