#pragma once

// Why are these defined here instead of #including sdm.h or strtDTL.h? Why did you need another header?

#pragma pack(push, 4)

// Defines
#define MAX_NAME_LENGTH			64
#define MAX_OBJNAME_LENGTH		32

// Structures
struct FULLID
{
	SINT32	scx;
	SINT32	scy;
	SINT32	id;
};

#define GLOBAL_FLAG_NEW			0x8000
#define GLOBAL_FLAG_CHANGED		0x4000

struct _CP
{
	struct _CP*			next;
	struct _FACILITY*	facility;
	char	name[MAX_NAME_LENGTH];
	char	objname[MAX_OBJNAME_LENGTH];
	SINT32	trnum;			// Visible terrain super number
	SINT32	posx;			// X Position of object in world
	SINT32	posy;			// Y Position of object in world
	FULLID	cpid;			// CP ID
	SINT32	side;			// Side of the CP
	SINT32	country;		// Country of the CP
	SINT32	ctype;			// Type of the CP
	UINT32	flags;			// Link flags
	UINT32	serial;			// Global unique serial number

};

struct _FACILITY
{
	struct _FACILITY*		next;
	struct _BUILDING*		building;
	char	name[MAX_NAME_LENGTH];
	char	objname[MAX_OBJNAME_LENGTH];
	SINT32	trnum;			// Visible terrain super number
	SINT32	posx;			// X Position of object in world
	SINT32	posy;			// Y Position of object in world
	FULLID	fid;			// Facility ID
	FULLID	cpid;			// Parent CP ID
	SINT32	side;			// Side of the Facility
	SINT32	country;		// Country of the Facility
	SINT32	ftype;			// Facility type
	SINT32	fsubtype;		// Facility subtype
	UINT32	flags;			// Link flags
	UINT32	serial;			// Global unique serial number

};

struct _BUILDING
{
	struct _BUILDING*		next;
	struct _DEPOTLINK*	link;
	char	name[MAX_NAME_LENGTH];
	char	objname[MAX_OBJNAME_LENGTH];
	SINT32	trnum;			// Visible terrain super number
	SINT32	posx;			// X Position of object in world
	SINT32	posy;			// Y Position of object in world
	FULLID	bid;			// Building ID
	FULLID	fid;			// Parent Facility ID
	SINT32	btype;			// Building type
	SINT32	bsubtype;		// Building subtype
	SINT32	control;		// Control building or not - only 1 building
	UINT32	flags;			// Link flags
	UINT32	serial;			// Global unique serial number

};

#define LINK_FLAG_SETOPEN		0x0001
#define LINK_FLAG_DISABLED		0x0002
#define LINK_FLAG_CONVOY		0x0004

struct _DEPOTLINK
{
	struct _DEPOTLINK*		next;
	struct _WAYPOINT*		waypoint;
	char	name[MAX_NAME_LENGTH];
	FULLID	lid;			// Link ID
	FULLID	lcpid;			// Local CP Depot ID
	FULLID	lfid;			// Local Facility Depot ID
	FULLID	lbid;			// Local Building ID
	FULLID	cpid;			// Finish CP Depot ID
	FULLID	fid;			// Finish Facility Depot ID
	FULLID	bid;			// Finish Building ID
	FULLID	fbid;			// Local Firebase ID
	UINT32	distance;		// Length of Link
	UINT32	flags;			// Link flags
	UINT32	serial;			// Global unique serial number

};

#define WAYPOINT_FLAG_BRIDGE	0x0001

struct _WAYPOINT
{
	struct _WAYPOINT*		next;
	SINT32	posx;			// X Position of object in world
	SINT32	posy;			// Y Position of object in world
	FULLID	wpid;			// Waypoint ID
	FULLID	lid;			// Parent Link ID
	FULLID	cpid;			// Bridge CP ID (WAYPOINT_FLAG_BRIDGE will be set in flags)
	SINT32	type;			// Type
	SINT32	subtype;		// Subtype
	UINT32	flags;			// Waypoint flags
	UINT32	serial;			// Global unique serial number

};

#pragma pack(pop)
