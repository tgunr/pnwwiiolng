#pragma once

#pragma pack(push)
#pragma pack(1)

	// Defines
	#define PLAYNET_COMPANY			('P' + ('N' << 8))		// short for Playnet

	#define PLAYNET_TERRAINFILE		('T' + ('R' << 8))		// short for Terrain (File Type)
	#define PLAYNET_MERGEFILE		('S' + ('D' << 8))		// short for Strat/Detail Merge (File Type)

	// NOTE: Be sure and change a version element every time you change the object file data or structure!!
	#define TERRAINFILE_MAJOR		2
	#define TERRAINFILE_MINOR		2
	#define TERRAINFILE_VERSION		((TERRAINFILE_MAJOR << 8) + TERRAINFILE_MINOR)	// version; MSB is major, LSB is minor
	#define TERRAINFILE_EXTENSION	".dtl"

	#define MERGEFILE_MAJOR			1
	#define MERGEFILE_MINOR			1
	#define MERGEFILE_VERSION		((MERGEFILE_MAJOR << 8) + MERGEFILE_MINOR)		// version; MSB is major, LSB is minor
	#define MERGEFILE_EXTENSION		".sdm"

	#define TERRAIN_NODATA			-9999
	#define TERRAIN_NONORMALS		0x003f

	#define SUPERCELL_SIZE_X		33
	#define SUPERCELL_SIZE_X1		(SUPERCELL_SIZE_X - 1)
	#define SUPERCELL_SIZE_Y		33
	#define SUPERCELL_SIZE_Y1		(SUPERCELL_SIZE_Y - 1)
	#define SUPERCELL_SIZE_XY		(SUPERCELL_SIZE_X * SUPERCELL_SIZE_Y)
	#define SUPERCELL_OCTETS_X		16
	#define SUPERCELL_OCTETS_Y		16
	#define SUPERCELL_OCTETS_XY		(SUPERCELL_OCTETS_X * SUPERCELL_OCTETS_Y)
	#define SUPERCELL_WIDTH			(SUPERCELL_SIZE_X1 * xstep)
	#define SUPERCELL_HEIGHT		(SUPERCELL_SIZE_Y1 * ystep)

    #define TRNINDEX(r, c) r * SUPERCELL_OCTETS_Y + c

	#define SUPERCELL_NORMAL_MASK_X	0xf800
	#define SUPERCELL_NORMAL_MASK_Y	0x07c0
	#define SUPERCELL_NORMAL_MASK_Z	0x003f

	#define SUPERCELL_NORMAL_SHIFT	11

	#define SUPERCELL_NORMAL_MAG_X	16.0f
	#define SUPERCELL_NORMAL_MAG_Y	16.0f
	#define SUPERCELL_NORMAL_MAG_Z	63.0f

	#define SUPERCELL_INVALID		-32768

	#define SUPERCELL_ARC_X			0.0046542113386515455384631753826363
	#define SUPERCELL_ARC_Y			0.0037233690709212364307705403056
	#define SUPERCELL_ARC_DEG_X		0.266667
	#define SUPERCELL_ARC_DEG_Y		0.2133336

	#define WORLD_ARC_WIDTH			180.0
	#define WORLD_ARC_HEIGHT		70.1875

	#define WORLD_WIDTH				8640000
	#define WORLD_HEIGHT			4211200

	// Terrain file flags
	#define TF_FLAG_DATA_RLE		0x0001
	#define TF_FLAG_NORMALS_RLE		0x0002

	// Altitude terrain breaks for auto-placement of terrain tiles
	#define ALT_TRN_NUMBER				8
	#define ALT_TRN_BELOW				-10000
	#define ALT_TRN_WATER				0
	#define ALT_TRN_DUNES				20
	#define ALT_TRN_FARMS				200
	#define ALT_TRN_CLEAR				500
	#define ALT_TRN_FOREST				1000
	#define ALT_TRN_ROCKY				3000
	#define ALT_TRN_ICY					10000

	// #defines
	#define RANDOM_MAX_INTEGER			(RAND_MAX / 2)
	#define RANDOM_MAX_INVERT			(1.0f / RANDOM_MAX_INTEGER)
	#define RANDOM_FLOAT				((FLOAT32)(rand() - RANDOM_MAX_INTEGER) * RANDOM_MAX_INVERT)
	#define WATER_COLOR_RED				0
	#define WATER_COLOR_GREEN			31
	#define WATER_COLOR_BLUE			127
	#define WATER_COLOR_ALPHA			255
	#define TERRAIN_RECTANGLES_X		5
	#define TERRAIN_RECTANGLES_Y		5
	#define NUM_TERRAIN_RECTANGLES		(TERRAIN_RECTANGLES_X * TERRAIN_RECTANGLES_Y)
	#define NUM_SKY_MESHES				6
	#define NUM_VERTICES_PER_OCTET		9
	#define NUM_TRIANGLES_PER_OCTET		8
	#define MAX_TERRAIN_TEXTURES		32
	#define MAX_TERRAIN_TRIANGLES		2048
	#define MAX_TERRAIN_OCTETS			(MAX_TERRAIN_TRIANGLES / NUM_TRIANGLES_PER_OCTET)
	#define MAX_TERRAIN_EDIT_LIST		256
	#define MAX_TERRAIN_VERTICES		(MAX_TERRAIN_OCTETS * NUM_VERTICES_PER_OCTET)
	#define MAX_TRNOBJ_OCTET			24

	#define OBJFLAG_ON_GROUND			0x00000001
	#define OBJFLAG_RELATIVE_GROUND		0x00000002
	#define OBJFLAG_ABSOLUTE			0x00000004
	#define OBJFLAG_HEIGHT_CONFORM		0x00000008
	#define OBJFLAG_BOTTOM_CONFORM		0x00000010
	#define OBJFLAG_PLACE_DOWN			0x00000020
	#define OBJFLAG_PLACE_ROTATE		0x00000040

	#define TRNOBJ_NON_PLANAR_BASE 0x1

	#define TRNOBJ_LOADED_FLAG_SIZE		32			// Size (in bits) of each flags element

	#define TDFLAG_MODIFIED				0x0001
	#define TDFLAG_PERIPHERY			0x0002
	#define TDFLAG_CHECKOUT				0x0004		// Terrain Editor
	#define TDFLAG_ADD					0x0008		// Terrain Editor
	#define TDFLAG_LOADUNLOAD			0x0010
	#define TDFLAG_LOADFAR				0x0020		// Used for clouds because of their size
    #define TDFLAG_PLANES_CREATED       0x0040      // Plane equations created for height information
	#define MAX_CAMERA_OFFSET			(2000.0 / ScaleFactor)  // the camera should never be more than this from offset origin
//	#define MAX_CAMERA_OFFSET			18 * SDM_SUPERCELL_SIZE
	#define TERRAIN_SNAPTO_GRIDSIZE		0.1900123f	// 19cm grid, should be non-divisible by octet size

	#define TERRAIN_DEFAULT_MATERIAL	"forest1"

	// typedefs
	struct DEMHDR
	{
		// data found in header file (*.hdr)
		SINT8		byteorder;		//	= 'M';
		SINT8		layout[8];		//	= "BIL";
		SINT32		nrows;			//	= 6000;
		SINT32		ncols;			//	= 4800;
		SINT32		nbands;			//	= 1;
		SINT32		nbits;			//	= 16;
		SINT32		bandrowbytes;	//	= 9600;
		SINT32		totalrowbytes;	//	= 9600;
		SINT32		bandgapbytes;	//	= 0;
		SINT32		nodata;			//	= -9999;
		FLOAT64		ulxmap;			//	= 20.00416666666667;
		FLOAT64		ulymap;			//	= 39.99583333333333;
		FLOAT64		xdim;			//	= 0.00833333333333;
		FLOAT64		ydim;			//	= 0.00833333333333;
	};

	struct TERRAINFILE
	{
		UINT16		company;		// Company initials
		UINT16		filetype;		// File type (object, terrain, etc.)
		UINT16		version;		// File version in Major/Minor (MSB/LSB) format
		UINT16		flags;			// Flags representing data options
		UINT32		dataversion;	// Data version using simple incremental technique
		UINT32		length;			// Length of data
	};

	enum TRN_STATE
	{
		TRN_STATE_ERROR=1,			// Error occurred during the load/unload process
		TRN_STATE_WAIT,				// Wait for visibility or creation/deletion of supercell
		TRN_STATE_LOAD_LOAD,		// Load all elements of a supercell for immediate visibility
		TRN_STATE_LOAD_MODIFY,		// Modify the supercell elements bases on DTL data
		TRN_STATE_LOAD_DEFAULT,		// Load the default data into a supercell object
		TRN_STATE_LOAD_FILE,		// Load the DTL file
		TRN_STATE_LOAD_NORMALS,		// Translate DTL normals into vertex normals
		TRN_STATE_LOAD_POINTS,		// Translate DTL points into vertex x, y and z
		TRN_STATE_LOAD_CALC,		// Calculate the surface normals and bounding box
		TRN_STATE_LOAD_LIGHT,		// Light surface of terrain
		TRN_STATE_LOAD_DUPBASE,		// Duplicate the base terrain cell
		TRN_STATE_LOAD_TEXTURES,	// Load DTL texture objects
		TRN_STATE_LOAD_OCTETS,		// Do octet substitution into texture objects
		TRN_STATE_LOAD_BOUNDS,		// Calculate bounding information for texture objects
		TRN_STATE_LOAD_OBJECTS,		// Create terrain objects and place on terrain
		TRN_STATE_LOAD_SNAP,		// Snap objects to XY grid
		TRN_STATE_LOAD_DONE,		// Finished all loading
		TRN_STATE_UNLOAD_FULL,		// Unload all elements of a super cell and release
		TRN_STATE_UNLOAD_OBJECTS,	// Delete terrain objects from existing super cell
		TRN_STATE_UNLOAD_OTHERS,	// Unload remainder of super cell and release
		// Add all new states here
		MAX_TRN_STATES				// Maximum number of states in this enum block
	};

	enum OBJVALS
	{
		OBJVAL_ON_GROUND=1,
		OBJVAL_RELATIVE_GROUND,
		OBJVAL_ABSOLUTE,
		OBJVAL_HEIGHT_CONFORM,
		OBJVAL_BOTTOM_CONFORM,
		OBJVAL_PLACE_DOWN,
		OBJVAL_PLACE_ROTATE,
		OBJVAL_OCTET_REPLACE,
		OBJVAL_PLACE_NOTUSED,
		OBJVAL_OCTET_CONFORMRADIAL,
		OBJVAL_PLACE_BASE,
		OBJVAL_PLACE_CONCAVE,
		OBJVAL_PLACE_SHEARED,
		OBJVAL_PLACE_STRAIGHT_DOWN,
        OBJVAL_PLACE_DOWN_ROTATE, // Place down and rotate to terrain
		OBJVAL_NO_PLACEMODE
	};

	struct TERRAIN_INFO
	{
		UINT16			octet;			// octet number
		UINT16			octnum;			// original octet number
		UINT8			tonum;			// original texture object number
		UINT8			txndx;			// texture name index
		UINT8			ortdrg;			// orientation/drag LSN/MSN
		UINT8			ruftrn;			// roughness/terrain LSN/MSN
	};

	struct DTL_TRNOBJ_INFO
	{
		piVECTOR		pos;			// object position
		SINT8			objname[32];	// object name in pool
		UINT8			txndx;			// texture object index
		UINT8			ortdrg;			// orientation/drag LSN/MSN
		UINT8			ruftrn;			// roughness/terrain LSN/MSN
		UINT8			flags;			//

	};

	#define MAX_TSB_NEXTSTATES		16

	struct TRNSTATEBLOCK
	{
		SINT32		state;			// State of the loading/unloading cycle for this terrain cell
		SINT32		varone;			// Variable 1
		SINT32		vartwo;			// Variable 2
		SINT32		varthree;		// Variable 3
		SINT32		nextstate[MAX_TSB_NEXTSTATES];	// Next state (for sequencing)
		SINT16		nextscx[MAX_TSB_NEXTSTATES];	// Next super cell x
		SINT16		nextscy[MAX_TSB_NEXTSTATES];	// Next super cell y

	};

#pragma pack(pop)
