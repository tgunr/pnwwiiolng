#pragma once

#define HEADER_RECORD         1
#define GROUP_RECORD          2
#define OBJECT_RECORD         4
#define FACE_RECORD           5
#define PUSH_LEVEL            10
#define POP_LEVEL             11
#define DOF_RECORD            14
#define PUSH_SUBFACE          19
#define POP_SUBFACE           20
#define PUSH_EXTENSION        21
#define POP_EXTENSION         22
#define TEXT_COMMENT          31
#define COLOR_PALETTE         32
#define LONG_ID               33
#define MATRIX                49
#define VECTOR                50
#define BSP                   55
#define REPLICATE             60
#define INSTANCE_REFERENCE    61
#define INSTANCE_DEFINITION   62
#define EXTERNAL              63
#define TEXTURE_REFERENCE     64
#define MATERIAL_TABLE        66
#define VERTEX_PALETTE        67
#define VERTEX_COORDINATE     68
#define VERTEX_NORMAL         69
#define VERTEX_NORMAL_UV      70
#define VERTEX_UV             71
#define VERTEX_LIST           72
#define FLT_LOD               73
#define BOUNDING_BOX          74
#define ROTATE_EDGE           76
#define TRANSLATE             78
#define SCALE_INDEPENDENT_XYZ 79
#define ROTATE_POINT          80
#define ROTATE_SCALE          81
#define PUT                   82
#define EYEPOINT_TRACKPLANE   83
#define ROAD_SEGMENT          87
#define ROAD_ZONE             88
#define MORPH_VERTEX_LIST     89
#define SOUND                 91
#define ROAD_PATH             92
#define SOUND_PALETTE         93
#define TEXT_RECORD           95
#define SWITCH                96
#define CLIP_REGION           98
#define EXTENSION               100
#define LIGHT_SOURCE            101
#define LIGHT_SOURCE_PALETTE    102
#define BOUNDING_SPHERE         105
#define BOUNDING_CYLINDER       106
#define BOUNDING_VOLUME_CENTER  108
#define BOUNDING_VOLUME_ORIENT  109
#define LIGHT_POINT             111
#define TEXTURE_MAPPING_PALETTE 112
#define MATERIAL_PALETTE        113
#define COLOR_NAME_PALETTE      114
#define CAT                     115
#define CAT_DATA                116
#define CURVE                   126

#define MAX_GROUP_SIZE		64
#define MAX_LEVELS			32


#ifdef _MWERKS
	#pragma options align=packed
#endif

#pragma pack(1)

struct Meshpool
{
	struct OBJECT* attached;
	UINT32	 level;
};												// pointer to mesh objects for merging of objects and the lowest level the meshpool is valid for

struct fltCHUNK
{
	short	cid;
	UINT16	next;
};

struct fltHEADER
{
	char      id[8];
	int       format;
	int       edit;
	char      dt[32];
	short     nextgroup;
	short     nextlod;
	short     nextobject;
	short     nextface;
	short     unit;
	char      vertexunits;
	char      texwhite;
	int       flags;
	int       reserved1[6];
	int       projection;
	int       reserved2[4];
	short     nextdof;
	short     vertexstorage;
	int       dborigin;
	double    swdbcoordx;
	double    swdbcoordy;
	double    deltax;
	double    deltay;
	short     nextsound;
	short     nextpath;
	int       reserved3[2];
	short     nextclip;
	short     nexttext;
	short     nextbsp;
	short     nextswitch;
	int       reserved4;
	double    swcornlat;
	double    swcornlong;
	double    necornlat;
	double    necornlong;
	double    originlat;
	double    originlong;
	double    lamuplat;
	double    lamuplong;
	short     nextlightsrc;
	short     nextlightpt;
	short     nextroad;
	short     nextcat;
	short     reserved5[4];
	int       earthellip;
	short     nextcurve;
	short     nextadapt;
	short     reserved6;
};

struct fltGROUP
{
	char      id[8];
	short     priority;
	short     reserved1;
	int       flags;
	short     spfxid1;
	short     spfxid2;
	short     significance;
	char      layercode;
	char      reserved2;
	int       reserved3;
};

#define fltOBJECT_NOTINDAYLIGHT   0x80000000
#define fltOBJECT_NOTATDUSK       0x40000000
#define fltOBJECT_NOTATNIGHT      0x20000000
#define fltOBJECT_NOTILLUMINATED  0x10000000
#define fltOBJECT_FLATSHADED      0x08000000
#define fltOBJECT_SHADOW          0x04000000

struct fltOBJECT
{
	char        id[8];
	int         flags;
	short       priority;
	UINT16        transparency;
	short       spfxid1;
	short       spfxid2;
	short       significance;
	short       spare;
};

#define fltFACE_TERRAIN           0x80000000
#define fltFACE_NOCOLOR           0x40000000
#define fltFACE_NOALTCOLOR        0x20000000
#define fltFACE_PACKEDCOLOR       0x10000000
#define fltFACE_FOOTPRINT         0x08000000
#define fltFACE_HIDDEN            0x04000000

// billboard
enum BillboardType
{
	FIXED_NO_ALPHA_BLENDING = 0,
	FIXED_ALPHA_BLENDING = 1,
	AXIAL_ROTATE_WITH_ALPHA_BLENDING = 2,
	POINT_ROTATE_WITH_ALPHA_BLENDING = 4
};

struct fltFACE
{
	char        id[8];
	int         ircolor;
	short       priority;
	char        drawtype;
	char        texwhite;
	UINT16        colorname;
	UINT16        altcolorname;
	char        reserved1;
	char        billboard;
	short       dettexpat;
	short       texpatindex;
	short       matindex;
	short       surfmatcode;
	short       fid;
	int         irmat;
	UINT16        transparency;
	char        lodgen;
	char        linestyle;
	int         flags;
	char        lightmode;
	char        reserved2;
	short       reserved3;
	int         reserved4;
	UINT32       color;
	UINT32       altcolor;
	short       texmapindex;
	short       reserved5;
	UINT32       colorindex;
	UINT32       altcolorindex;
	////short       reserved6;
	////short       reserved7;
};

struct fltLIGHT
{
	short       surfmatcode;
	short       fid;
	UINT32       backcolor;
	int         dispmode;
	float       intensity;
	float       backintensity;
	float       mindefocus;
	float       maxdefocus;
	int         fademode;
	int         fogpunch;
	int         dirmode;
	int         rangemode;
	float       minpixel;
	float       maxpixel;
	float       actsize;
	float       tfopixelsize;
	float       tfoexp;
	float       tfoscalar;
	float       tfoclamp;
	float       fogscalar;
	float       reserved;
	float       diffsize;
	int         dirtype;
	float       hlobe;
	float       vlobe;
	float       loberoll;
	float       dfoexp;
	float       dirambint;
	float       animper;
	float       animphase;
	float       animenaper;
	float       significance;
	int         calidraw;
	int         flags;
};

#define fltDOF_XTRANS_LIMITED		0x80000000
#define fltDOF_YTRANS_LIMITED		0x40000000
#define fltDOF_ZTRANS_LIMITED		0x20000000
#define fltDOF_PITCH_ROT_LIMITED	0x10000000
#define fltDOF_ROLL_ROT_LIMITED		0x08000000
#define fltDOF_YAW_ROT_LIMITED		0x04000000
#define fltDOF_XSCALE_LIMITED		0x01000000
#define fltDOF_YSCALE_LIMITED		0x00800000
#define fltDOF_ZSCALE_LIMITED		0x00400000

struct fltDOF
{
	char        name[8];
	int         reserved;
	double      originx;
	double      originy;
	double      originz;
	double      pointxaxisx;
	double      pointxaxisy;
	double      pointxaxisz;
	double      pointxyplanex;
	double      pointxyplaney;
	double      pointxyplanez;
	double      minz;
	double      maxz;
	double      curz;
	double      incz;
	double      miny;
	double      maxy;
	double      cury;
	double      incy;
	double      minx;
	double      maxx;
	double      curx;
	double      incx;
	double      minpitchx;
	double      maxpitchx;
	double      curpitchx;
	double      incpitchx;
	double      minpitchy;
	double      maxpitchy;
	double      curpitchy;
	double      incpitchy;
	double      minpitchz;
	double      maxpitchz;
	double      curpitchz;
	double      incpitchz;
	double      minzscale;
	double      maxzscale;
	double      curzscale;
	double      inczscale;
	double      minyscale;
	double      maxyscale;
	double      curyscale;
	double      incyscale;
	double      minxscale;
	double      maxxscale;
	double      curxscale;
	double      incxscale;
	int         flags;
};

struct fltVERTEX
{
	int         vertoff;
};

struct fltVERTEXPALETTE
{
	int         length;
};

struct fltMORPVERTEX
{
	struct
	{
		int         vertoff0;
		int         vertoff100;
	} *vertex;
};

struct fltBSPSEPERATIONPLANE
{
	char          name[8];
	int           reserved;
	double        firstplane;
	double        secondplane;
	double        thirdplane;
	double        forthplane;
};

struct fltLOD
{
	char          name[8];
	int           reserved;
	double        switchin;
	double        switchout;
	short         spfxid1;
	short         spfxid2;
	int           flags;
	double        centerx;
	double        centery;
	double        centerz;
	double        transition;
};

struct fltLIGHTSOURCE
{
	char          name[8];
	int           reserved1;
	int           lightindex;
	int           reserved2;
	int           flags;
	int           reserved3;
	double        posx;
	double        posy;
	double        posz;
	float         yaw;
	float         pitch;
};

struct fltVECTOR
{
	float       i;
	float       j;
	float       k;
};

#define fltVERTEX_FROZEN		0x4000

struct fltVERTEXCOORDINATE
{
	UINT16        colorname;
	short       flags;
	double      x;
	double      y;
	double      z;
	int         color;
	UINT32       vertexcolor;  // may be reserved
};

struct fltVERTEXNORMAL
{
	UINT16        colorname;
	short       flags;
	double      x;
	double      y;
	double      z;
	float       i;
	float       j;
	float       k;
	int         color;
	//DWORD       vertexcolor;
};

struct fltVERTEXUV
{
	UINT16        colorname;
	short       flags;
	double      x;
	double      y;
	double      z;
	float       u;
	float       v;
	int         color;
	UINT32       vertexcolor;  // may be reserved
};

struct fltVERTEXNORMALUV
{
	UINT16        colorname;
	short       flags;
	double      x;
	double      y;
	double      z;
	float       i;
	float       j;
	float       k;
	float       u;
	float       v;
	int         color;
	//DWORD       vertexcolor;
};

struct fltCOLORENTRY
{
	UINT16        namelen;
	short       reserved2;
	short       index;
	short       reserved3;
	char        name[1];
};

struct fltCOLORPALETTE
{
	char        reserved[128];
	int         briteRGB[1024];
	//int         numnames;
	//fltCOLORENTRY entry[1];
};

struct fltMATERIAL
{
	float       ambRed;
	float       ambGreen;
	float       ambBlue;
	float       diffRed;
	float       diffGreen;
	float       diffBlue;
	float       specRed;
	float       specGreen;
	float       specBlue;
	float       emissRed;
	float       emissGreen;
	float       emissBlue;
	float       shininess;
	float       alpha;
	int         flags;
	char        name[12];
	int         spares[28];
};

struct fltMATERIAL_PALETTE
{
	int         index;
	char        name[12];
	int         flags;
	float       ambRed;
	float       ambGreen;
	float       ambBlue;
	float       diffRed;
	float       diffGreen;
	float       diffBlue;
	float       specRed;
	float       specGreen;
	float       specBlue;
	float       emissRed;
	float       emissGreen;
	float       emissBlue;
	float       shininess;
	float       alpha;
	int         spares;
};

struct fltTEXTUREREFERENCE
{
	char        name[200];
	int         patindex;
	int         x;
	int         y;
};

struct fltEXTENSION
{
	 char      id[8];
	 char      siteid[8];
	 char      reserved1;
	 char      revision;
	 short     code;
};

#pragma pack()

#ifdef _MWERKS
	#pragma options align=reset
#endif

#define fltSOLID_BACKFACED    0
#define fltSOLID_NOBACKFACE   1
#define fltWIRE_OPEN          2
#define fltWIRE_CLOSED        3
#define fltALTERNATE_COLOR    4

struct OBJECT* ldfltLoad(const std::string& name, float scale, int reduce);
