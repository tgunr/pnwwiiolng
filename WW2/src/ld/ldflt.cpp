#include <syscfg.h> // Path is project specific.
// Copyright 2008 Playnet Inc, All rights reserved
// Not to be distributed in API form except as part of a compiled library
// under License or NDA

#include <math.h>
#include <string.h>
#include <ctype.h>
#include <string>
#include <vector>
#include <array>
#include "pi/pi.h"
#include "include/vars.h"
#include "mem/mem.h"
#include "ld/ldmdl.h"
#include "ld/ldflt.h"
#include "ld/ldbmp.h"
#include "ld/ldpix.h"
#include "fio/fio.h"
#include "math/math3d.h"
#include "ob/obc.h"
#include "ob/obmain.h"
#include "str/strpool.h"
#include "sys/sysWinISO.h"
#include "ld/lddds.h"
#include <assert.h>
#include "sysmain.h"
#include "wwii/wwiiHash.h"
#include "ldtexture.h"

#define MERGE_TREE_OBJECTS 1

#include "spt/sptmain.h"

struct fltColor
{
	UCHAR	alpha;
	UCHAR	blue;
	UCHAR	green;
	UCHAR	red;
};

union colorstruct
{
	fltColor rgb;
	UINT32	full;
};

static std::string s_loadError;

static void FLTLoadFailure(const char* problem, const char* doing)
{
	s_loadError += problem;
	s_loadError += " on ";
	s_loadError += doing;
	s_loadError += '\n';
}

static void FLTChunkFailure(const char* chunkType)
{
	s_loadError += "Unable to load FLT chunk ";
	s_loadError += chunkType;
	s_loadError += '\n';
}

static void FLTUnknownTexture(const char* textureName)
{
	s_loadError += "Reference to missing texture ";
	s_loadError += textureName;
	s_loadError += '\n';
}

static void FLTGeneralError(const char* error)
{
	s_loadError += error;
	s_loadError += '\n';
}

static void FLTItemFileError(const char* error, const char* item, const char* file)
{
	s_loadError += error;
	s_loadError += ": id=";
	s_loadError += item;
	s_loadError += ": file=";
	s_loadError += file;
	s_loadError += '\n';
}

// persistent variables
static SINT8				pathname[MAX_PATH];
static char					filename[MAX_PATH];
static char					grpname[MAX_GROUP_SIZE][32];
static struct OBJECT*		groups[MAX_GROUP_SIZE];
static Meshpool				meshpools[MAX_GROUP_SIZE];
static SINT32				meshpool_idx;							// index for the mesh pools
static SINT32				grouplevels[MAX_GROUP_SIZE];
static SINT32				groupindex;
static char					objname[MAX_LEVELS][32];
static long					offset;
static float				localscale;
static fltCHUNK*			s_chunk;
static OBJECT				tv;
static OBJECT*				attached;
static OBJECT*				lastobject;
static OBJECT*				headObject;
static OBJECT*				topmostObject;
static MATERIAL				mat;
//static LIGHT				light;

static std::string			s_conversionFile;

// To convert to DirectX Handedness
static piMATRIX				translate;

static const size_t MAX_TEXTURE_REFS = 64;
static const size_t MAX_FLT_MATERIALS = 64;

static long					fltrevision;
static void*				rawverts;
static fltFACE*				s_face;
static int					facelevel;
static int					facelayer;
static UINT32				facecolor;
static fltVERTEX*			vertlist;
static fltMATERIAL*			material;
static fltTEXTUREREFERENCE*	texref;
static fltOBJECT			object;
static int					objreduce;
static SINT8*				textcomment;
static SINT32				textlevel;
static std::vector<colorstruct>	s_colortable;
static int					matlevel;
static int					texlevel;
static size_t				s_nummaterials;
static int					numtexrefs;
static int					colorlevel;
static int					totsizeverts;
static int					sizeverts;
static int					numvertices;
static int					presvert;
static int					extlevel;
static int					level;
static int					objectlevel;
static int					doflevel;
static piMATRIX				s_matrixRecord;
static int					vlistlevel;
static int					texid[64];
//static int				numuses[64];
static bool					texalpha[64];
static bool					hasalpha;
static bool					topmost;

static unsigned int _textureflags2[MAX_TEXTURE_REFS];
static unsigned int _textureflags3[MAX_TEXTURE_REFS];

static bool bTopmostObject;
static bool g_bSpeedTrees;
static bool g_isGrass;
static bool _hasspeedtree;
static SINT32 g_grassType;

static struct CurrentLod
{
	int				level;
	const char*		name;
	double			switchin;
	double			switchout;
} s_currentLod;

// ----------------- faster loading

static std::vector<char>& streamBuffer = * new std::vector<char>(); // reference to vector in ldfltLoad
static size_t streamOffset;

void ldfltKill()
{
	delete &streamBuffer;
}

template<typename PtrType>
size_t localBufferUseBytes(PtrType*& ptr, const size_t requestBytes)
{
	const size_t bytesInBuffer = streamBuffer.size();
	if (streamOffset >= bytesInBuffer)
		return 0;
	const size_t remainingBytes = bytesInBuffer - streamOffset;
	if (requestBytes > remainingBytes)
		return 0;
	ptr = reinterpret_cast<PtrType*>(&streamBuffer[streamOffset]);
	streamOffset += requestBytes;

	return requestBytes;
}

template<typename ObjectType, size_t Count=1>
size_t localBufferUseObject(ObjectType*& ptr)
{
	return localBufferUseBytes(ptr, sizeof(ObjectType) * Count);
}

size_t localBufferCopyBytes(void* const dest, const int requestBytes)
{
	void* src;
	const size_t result = localBufferUseBytes(src, requestBytes);
	if (result <= 0)
		return result;
	memcpy(dest, src, result);
	return result;
}

template<typename ObjectType>
size_t localBufferCopyObject(ObjectType* const dest)
{
	ObjectType* src;
	size_t result = localBufferUseObject(src);
	if (result <= 0)
		return result;
	memcpy(dest, src, result);
	return result;
}

static int localSeek(int offset)
{
	if ( offset <0 )
	{
		offset = 0;
		return -1;
	}
	if ( (size_t)offset >= streamBuffer.size())
	{
		offset = streamBuffer.size();
		return -1;
	}

	streamOffset = offset;

	return streamOffset;
}

inline bool isAlphaBlend()
{
	return (s_face->billboard == FIXED_ALPHA_BLENDING) || (s_face->billboard == AXIAL_ROTATE_WITH_ALPHA_BLENDING) || (s_face->billboard == POINT_ROTATE_WITH_ALPHA_BLENDING);
}

// these four functions apply to little-endian systems only. Big-endian
// systems have their own definition. By performing the #if here, we
// allow the file loaders to have minimal intrusions of endian swaps.

static UINT16 ShortEndian(UINT16 word)
{
	return((word >> 8) | ((word & 0xff) << 8));
}


static UINT32 LongEndian(UINT32 dword)
{
	return((UINT32)ShortEndian((UINT16)(dword >> 16)) | ((UINT32)ShortEndian((UINT16)(dword & 0xffff)) << 16));
}


static float FloatEndian(float f)
{
	struct
	{
	    union
		{
			float f;
			UINT32 dw;
		};
	} fdw;

	fdw.f = f;
	fdw.dw = LongEndian(fdw.dw);

	return fdw.f;
}


static double DoubleEndian(double dbl)
{
	struct
	{
	    union
	    {
		    double	dbl;
		    struct
		    {
		    UINT32	dw1;
		    UINT32	dw2;
		    } dw;
	    };
	} ddw;

	ddw.dbl = dbl;

	ddw.dw.dw1 = LongEndian(ddw.dw.dw1);
	ddw.dw.dw2 = LongEndian(ddw.dw.dw2);

	std::swap(ddw.dw.dw1, ddw.dw.dw2);

	return ddw.dbl;
}

// read a vertex
static void ConvertVector(piVECTOR* vector)
{
	// To convert to DirectX Handedness
	math3dVectorMultiply(vector, vector, &translate);
}

// for unknown or irrelevant chunks of data
static void ReadUnknown()
{
	offset += s_chunk->next;
}

static UINT16 FindTexPatIndex(UINT16 patindex)
{
	for (auto i=0; i < numtexrefs; i++)
	{
		if (patindex == texref[i].patindex)
		{
			return i;
		}
	}

	return (UINT16)-1;
}

static UINT32 CreateColor(int colorindex)
{
	if (PN_UNLIKELY(s_colortable.empty()))
	{
		Write_Debug("Tried to get a color but the color table has not been created");
		return(0xffffffff);
	}

	UINT32		rgba, alpha;
	colorstruct	color;
	color.full = s_colortable[colorindex].full;
	if (s_face->matindex >= 0)
	{
		alpha = (UCHAR)(material[s_face->matindex].alpha * 255.0f);
	}
	else
	{
		alpha = (UINT32)((float)(65535 - s_face->transparency) / 65535.0f * 255.0f);
	}

	rgba = piRGBA_MAKE(color.rgb.red, color.rgb.green, color.rgb.blue, alpha);

	return(rgba);
}

static bool ReadHeader(void)
{
	fltHEADER* header;
	if ( !localBufferUseObject(header) )
		return false;

	//header->format = LongEndian(header->format);
	//header->edit   = LongEndian(header->edit);

	fltrevision = LongEndian(header->format);

	//Write_Debug("Header for id: %s", header.id);
	//Write_Debug("Openflight version: %d", header.format);

	offset += s_chunk->next;

	return true;
}

static bool CreateLevel(OBJECT** lpnewobj)
{
	SINT32		i;
	OBJECT*	lpobj;

	lpobj = obmainGetNewObject();

	g_bSpeedTrees = false;

	if (lpobj == nullptr)
	{
		return false;
	}

	if (groupindex >= 0)
	{
		if (groups[groupindex] != nullptr)
		{
			if (groups[groupindex]->child == nullptr)
			{
		if(groups[groupindex]->flag2 & OBJECT_SPEEDTREE)
		{
			lpobj->flag2 |= OBJECT_SPEEDTREE;
			g_bSpeedTrees = true;
		}

				groups[groupindex]->child = lpobj;
			}
		}
	}

	groupindex += 1;

	if (groupindex >= MAX_GROUP_SIZE)
		return FLTGeneralError("Group index overflow"), false;

	grouplevels[groupindex] = level;
	if (groups[groupindex] != nullptr)
	{
		if(groups[groupindex - 1]->flag2 & OBJECT_SPEEDTREE)
		{
			lpobj->flag2 |= OBJECT_SPEEDTREE;
			g_bSpeedTrees = true;
		}

		groups[groupindex]->sibling = lpobj;
	}

	groups[groupindex] = lpobj;

	for (i=groupindex+1; i < MAX_GROUP_SIZE; i++)
	{
		grouplevels[i] = -1;
		groups[i] = nullptr;
	}

	if (topmost)
	{
		lpobj->flag2 = OBJECT_TOPMOST;
		obmainMoveToNotObjectTree(lpobj);
		topmostObject = lpobj;
		topmost = false;
		Write_Debug("Topmost object was tagged");
	}

	*lpnewobj = lpobj;

	return true;
}

static bool ReadGroup(void)
{
	fltGROUP* group;
	if ( !localBufferUseObject(group) )
		return false;

	offset += s_chunk->next;

	// If group.id is <= 7, then we can compare the conversion filename
	// with the group id here. Else, we will need to compare in ReadLongID.
	if(topmost)
	{
		if(s_conversionFile.length() <= 7)
		{
			bTopmostObject = false;
			if (stricmp(group->id, s_conversionFile.c_str()))
			{
				Write_Debug("[ReadGroup] FAILURE: Topmost group.id = %s Filename = %s\n",
							group->id, s_conversionFile.c_str());

				FLTItemFileError("Topmost group", group->id, s_conversionFile.c_str());

				return false;
			}
		}
		else
		{
			// Save topmost flag for ReadLongID. Necessary to compare
			// filenames against topmost object names if filename is greater
			// than 7.
			bTopmostObject = true;
		}
	}

	if (strcmp(group->id, "Default") == 0)
	{
		FLTGeneralError("[ReadGroup] FAILURE: Group.id is 'Default'");
		return false;
	}

	OBJECT* lpobj;
	if (!CreateLevel(&lpobj))
	{
		FLTGeneralError("[ReadGroup] CreateLevel failed");
		return false;
	}

	lpobj->flags = OBJECT_GROUP;

	if(!strncmp(group->id, "speedtr", 7))
	{
		lpobj->flag2 |= OBJECT_SPEEDTREE;
		g_bSpeedTrees = true;
		_hasspeedtree = true;
	}

	g_isGrass = false;
	g_grassType = 0;

	strcpy(grpname[groupindex], group->id);
	strcpy(lpobj->name, group->id);

	lastobject = lpobj;

	if (objectlevel != -1)
	{
		attached = nullptr;
		lastobject	= nullptr;
		//objectlevel = level;		// WTF!?!
		numvertices = 0;
		strcpy(objname[level], grpname[groupindex]);
	}

	//	Write_Debug("Processing group: %s (%d)", grpname[groupindex], level);

	return true;
}


static bool ReadVertexPalette(void)
{
	fltVERTEXPALETTE* vp;

	if ( !localBufferUseObject(vp) )
		return false;

	totsizeverts = LongEndian(vp->length) - 8;
	sizeverts = 0;
	numvertices = 0;

	rawverts = memAlloc(totsizeverts);
	localBufferCopyBytes(rawverts, totsizeverts);

	offset += LongEndian(vp->length);

	return true;
}


static bool ReadVertexCoordinate(void* pvc, piUNIFIED_VERTEX* vertices)
{
	fltVERTEXCOORDINATE* vertcoord=(fltVERTEXCOORDINATE*)pvc;
	fltVERTEXCOORDINATE vc;
	piVECTOR	pos;

#ifdef DEBUG_FPU
	SetupFPU(false);
#endif
	vc.colorname	= ShortEndian(vertcoord->colorname);
	vc.flags		= ShortEndian(vertcoord->flags);
	vc.x			= DoubleEndian(vertcoord->x);
	vc.y			= DoubleEndian(vertcoord->y);
	vc.z			= DoubleEndian(vertcoord->z);
	vc.color		= LongEndian(vertcoord->color);
#ifdef DEBUG_FPU
	SetupFPU(true);
#endif

	pos.x = (float)vc.x;
	pos.y = (float)vc.y;
	pos.z = (float)vc.z;

	ConvertVector(&pos);

	vertices[presvert].x = pos.x;
	vertices[presvert].y = pos.y;
	vertices[presvert].z = pos.z;

	vertices[presvert].color = facecolor;

	presvert += 1;

	numvertices += 1;

	return true;
}


static bool ReadVertexNormal(void* pvn, piUNIFIED_VERTEX* vertices)
{
	fltVERTEXNORMAL* vertnorm=(fltVERTEXNORMAL*)pvn;
	fltVERTEXNORMAL vn;
	piVECTOR	pos;

#ifdef DEBUG_FPU
	SetupFPU(false);
#endif
	vn.colorname	= ShortEndian(vertnorm->colorname);
	vn.flags		= ShortEndian(vertnorm->flags);
	vn.x			= DoubleEndian(vertnorm->x);
	vn.y			= DoubleEndian(vertnorm->y);
	vn.z			= DoubleEndian(vertnorm->z);
	vn.i			= FloatEndian(vertnorm->i);
	vn.j			= FloatEndian(vertnorm->j);
	vn.k			= FloatEndian(vertnorm->k);
	vn.color		= LongEndian(vertnorm->color);
#ifdef DEBUG_FPU
	SetupFPU(true);
#endif

	pos.x = (float)vn.x;
	pos.y = (float)vn.y;
	pos.z = (float)vn.z;

	ConvertVector(&pos);

	vertices[presvert].x = pos.x;
	vertices[presvert].y = pos.y;
	vertices[presvert].z = pos.z;

	vertices[presvert].nx = vn.i;
	vertices[presvert].ny = vn.j;
	vertices[presvert].nz = -vn.k;

	vertices[presvert].color = facecolor;

	presvert += 1;

	numvertices += 1;

	return true;
}


static bool ReadVertexUV(void* pvuv, piUNIFIED_VERTEX* vertices)
{
	UINT16	index;
	fltVERTEXUV* vertuv=(fltVERTEXUV*)pvuv;
	fltVERTEXUV vuv;
	piVECTOR	pos;


#ifdef DEBUG_FPU
	SetupFPU(false);
#endif
	vuv.colorname	= ShortEndian(vertuv->colorname);
	vuv.flags		= ShortEndian(vertuv->flags);
	vuv.x			= DoubleEndian(vertuv->x);
	vuv.y			= DoubleEndian(vertuv->y);
	vuv.z			= DoubleEndian(vertuv->z);
	vuv.u			= FloatEndian(vertuv->u);
	vuv.v			= FloatEndian(vertuv->v);
	vuv.color		= LongEndian(vertuv->color);
#ifdef DEBUG_FPU
	SetupFPU(true);
#endif

	pos.x = (float)vuv.x;
	pos.y = (float)vuv.y;
	pos.z = (float)vuv.z;

	ConvertVector(&pos);

	vertices[presvert].x = pos.x;
	vertices[presvert].y = pos.y;
	vertices[presvert].z = pos.z;

	vertices[presvert].color = facecolor;

	vertices[presvert].tu = vuv.u;
	vertices[presvert].tv = vuv.v;

	presvert += 1;

	numvertices += 1;

	index = FindTexPatIndex(s_face->texpatindex);

	if (index != 0xffff)
	{
		tv.texname = strpool(texref[index].name);
		tv.texid = texid[index];

		if (texalpha[index])
		{
			hasalpha = true;
		}
	}

	return true;
}


static bool ReadVertexNormalUV(void* pvnuv, piUNIFIED_VERTEX* vertices)
{
	UINT16	index;
	fltVERTEXNORMALUV* vertnormuv=(fltVERTEXNORMALUV*)pvnuv;
	fltVERTEXNORMALUV vnuv;
	piVECTOR	pos;

#ifdef DEBUG_FPU
	SetupFPU(false);
#endif

	vnuv.colorname	= ShortEndian(vertnormuv->colorname);
	vnuv.flags		= ShortEndian(vertnormuv->flags);
	vnuv.x			= DoubleEndian(vertnormuv->x);
	vnuv.y			= DoubleEndian(vertnormuv->y);
	vnuv.z			= DoubleEndian(vertnormuv->z);
	vnuv.i			= FloatEndian(vertnormuv->i);
	vnuv.j			= FloatEndian(vertnormuv->j);
	vnuv.k			= FloatEndian(vertnormuv->k);
	vnuv.u			= FloatEndian(vertnormuv->u);
	vnuv.v			= FloatEndian(vertnormuv->v);
	vnuv.color		= LongEndian(vertnormuv->color);

#ifdef DEBUG_FPU
	SetupFPU(true);
#endif

	pos.x = (float)vnuv.x;
	pos.y = (float)vnuv.y;
	pos.z = (float)vnuv.z;

	ConvertVector(&pos);

	vertices[presvert].x = pos.x;
	vertices[presvert].y = pos.y;
	vertices[presvert].z = pos.z;


	vertices[presvert].nx = vnuv.i;
	vertices[presvert].ny = vnuv.j;
	vertices[presvert].nz = -vnuv.k;

	vertices[presvert].color = facecolor;

	vertices[presvert].tu = vnuv.u;
	vertices[presvert].tv = vnuv.v;

	presvert += 1;

	numvertices += 1;

	index = FindTexPatIndex(s_face->texpatindex);
	if (index != 0xffff)
	{
		tv.texname = strpool(texref[index].name);
		tv.texid = texid[index];
		if (texalpha[index])
		{
			hasalpha = true;
		}
	}

	return true;
}

struct eqstr
{
	bool operator()(const char* s1, const char* s2) const
	{
		return strcmp(s1, s2) == 0;
	}
};

#if platOS == OS_MACOS
typedef stdext::hash_set<const char*, stdext::hash<const char*>, eqstr> CharPSet;
static CharPSet* RoadTextureNames = nullptr;

static void initRoadTextureNames()
{
	RoadTextureNames = new CharPSet;
	RoadTextureNames->insert("terrain\\to\\asphault1");
	RoadTextureNames->insert("terrain\\to\\roadt1");
	RoadTextureNames->insert("terrain\\to\\roadway002");
	RoadTextureNames->insert("terrain\\to\\tan_trans");
	RoadTextureNames->insert("terrain\\to\\tangray1");
	RoadTextureNames->insert("terrain\\to\\urbaninter1");
	RoadTextureNames->insert("terrain\\to\\urbaninter2");
	RoadTextureNames->insert("terrain\\to\\urbaninter3");
	RoadTextureNames->insert("terrain\\to\\urbaninter4");
	RoadTextureNames->insert("terrain\\to\\urbaninter5");
	RoadTextureNames->insert("terrain\\to\\urbanroad1");
	RoadTextureNames->insert("terrain\\to\\urbanroad2");
	RoadTextureNames->insert("terrain\\to\\urbanroad3");
	RoadTextureNames->insert("terrain\\to\\urbanroad4");
	RoadTextureNames->insert("terrain\\to\\urbanroad5");
	RoadTextureNames->insert("terrain\\brdg\\asphault1");
	RoadTextureNames->insert("terrain\\brdg\\bridgeroad");
	RoadTextureNames->insert("terrain\\brdg\\pavedroad");
	RoadTextureNames->insert("terrain\\brdg\\tan_trans");
	RoadTextureNames->insert("terrain\\brdg\\tangray1");
	RoadTextureNames->insert("terrain\\brdg\\urbanroad1");
}
#endif

static bool isRoadTexture( int texid )
{
#if platOS == OS_MACOS
	TEXTURE* tp = oglGetTextureData(texid);
	bool res = tp!=nullptr && RoadTextureNames->find( tp->name ) != RoadTextureNames->end();
	if ( res )
	{
		//printf("%s\n",tp->name);
	}
	return res;
#else
	return false;
#endif
}

#define LOOK_FOR_DEGENERATE_TRIANGLES 0

#if LOOK_FOR_DEGENERATE_TRIANGLES
#include "SuperFastHash.h"
#endif

static bool ReadVertexList(void)
{
	void*				pdata					= nullptr;
	fltCHUNK*			pchunk					= nullptr;
	int					i, num, length, index;
	OBJECT*				lpobj					= nullptr;
	piUNIFIED_VERTEX*	vertices				= nullptr;

#if platOS == OS_MACOS
	if ( RoadTextureNames == nullptr )
	{
		initRoadTextureNames();
	}
#endif

	length = (int)s_chunk->next - 4;

	if (vertlist != nullptr)
	{
		memFree(vertlist);
	}

	vlistlevel = level;

	vertlist = (fltVERTEX*)memAlloc(length);
	if (!localBufferCopyBytes(vertlist, length))
		return false;
	num = length /= 4;

	if( s_face->drawtype != fltWIRE_OPEN && s_face->drawtype != fltWIRE_CLOSED && num < 3 )
	{
#ifdef EVENT_LOG_EXTREME
		Write_Debug("Degenerated face found in .flt! Skipped in .mdl.");
#endif
		offset += s_chunk->next;

		Write_Debug("Degenerate face found in .flt! This face %s is not a polygon", s_face->id);
		return false;
	}

	//memset(&tv, 0, sizeof(tv));
	if (s_face->texpatindex >= 0 ||
		s_face->matindex >= 0 ||
		s_face->transparency != 0 ||
		object.transparency != 0)
	{
		tv.flags |= OBJECT_VISIBLE;
	}
	else
	{
		tv.flags &= ~OBJECT_VISIBLE;
	}

	////tv.flags |= OBJECT_VISIBLE;

	//if (!(object.flags & fltOBJECT_NOTINDAYLIGHT))
	if (object.flags & fltOBJECT_NOTINDAYLIGHT)
	{
		tv.flags &= ~OBJECT_VISIBLE;
	}
	else
	{
		tv.flags |= OBJECT_VISIBLE;
	}

	////tv.flags &= ~OBJECT_VISIBLE;////

	if (!(object.flags & fltOBJECT_NOTATDUSK))
	{
		//tv.flags |= OBJECT_COLLISION;
	}

	if (object.flags & fltOBJECT_NOTILLUMINATED)
	{
		tv.flags |= OBJECT_RADIUSCOLLISION;
	}

	tv.flags |= OBJECT_CLIPPED;
	if (s_currentLod.level != -1)
	{
		tv.flags |= OBJECT_USELOD;
		tv.minlod = (float)(s_currentLod.switchout * s_currentLod.switchout) * localscale * localscale;
		tv.maxlod = (float)(s_currentLod.switchin * s_currentLod.switchin) * localscale * localscale;
	}

	if (s_face->drawtype == fltWIRE_OPEN)
	{
		tv.flags |= OBJECT_LINESTRIP
				|	OBJECT_LITE
				|	OBJECT_DYNALITE
				|	OBJECT_NOSPECULAR; // just to be sure
	}
	else
	if (s_face->drawtype == fltWIRE_CLOSED)
	{
		tv.flags |= OBJECT_LINECLOSED
				|	OBJECT_LITE
				|	OBJECT_DYNALITE
				|	OBJECT_NOSPECULAR; // just to be sure
		length += 1;
	}
	else
	{
		if (s_face->drawtype == fltSOLID_NOBACKFACE)
		{
		//tv.flags |= OBJECT_NOBACKFACE;
		//length *= 2;
		}

		//tv.flags |= OBJECT_TRIANGLELIST;
		tv.flags |= OBJECT_TRIANGLEFAN
				|	OBJECT_INDEXED
				|	OBJECT_LITE
				|	OBJECT_DYNALITE;
	}

	if (s_face->drawtype == fltWIRE_OPEN || s_face->drawtype == fltWIRE_CLOSED)
	{
		// For lines, no indices are allowed
		tv.flags &= ~OBJECT_INDEXED;
	}

	if (objreduce & 2)
	{
		tv.flags |= OBJECT_USEVERTEXBSP;
	}

	if (matlevel != -1)
	{
		if (s_face->matindex >= 0)
		{
			tv.matname = strpool(material[s_face->matindex].name);
			if (tv.matname[0] == 0)
			{
				Write_Debug("Material has no name!");
			}
		}
		else
		{
			tv.matname = "";
		}
	}
	else
	{
		tv.matname = "";
	}

	tv.flags |= OBJECT_USEMATRIX;

	tv.matid = -1;
	tv.texname = "";
	tv.texid = -1;
	tv.feature = s_face->fid;
	tv.level = s_face->surfmatcode;
	tv.layer = object.priority + facelayer;

	////Write_Debug(" vertices: %d", length);////

	if (length < 2)
	{
		Write_Debug("Points are not allowable object types!");
		return false;
	}

	strcpy(tv.name, objname[level]);

	// Early warning.
	if(!strncmp(tv.name, "airwarn", 7))
	{
		tv.flag2 |= OBJECT_AIRRAIDSIREN;
	}

	if (s_face->drawtype == fltWIRE_OPEN || s_face->drawtype == fltWIRE_CLOSED)
	{
		tv.numtris	= 0;
		tv.numverts	= length;
	}
	else
	{
		tv.numtris	= length - 2;
		tv.numverts = tv.numtris * 3;
	}

	tv.CreateStaticData();

	if (tv.numtris > 0)
	{
		tv.ptris		= (piINDICES*)memAlloc(tv.numtris * sizeof(piINDICES));
	}
	vertices		= (piUNIFIED_VERTEX* )memAlloc(tv.numverts * sizeof(piUNIFIED_VERTEX));

	tv.mat			= IdentityMatrix;
	presvert		= 0;

#if LOOK_FOR_DEGENERATE_TRIANGLES
	std::set<size_t> duplicate;
#endif

	for (i=0; i < num; i++)
	{
		vertlist[i].vertoff = LongEndian(vertlist[i].vertoff) - 8;
		if (vertlist[i].vertoff >= totsizeverts)
		{
			break;
		}

		pchunk = (fltCHUNK*)((char*)rawverts + vertlist[i].vertoff);
		pdata = (void*)((char*)pchunk + 4);
		switch (ShortEndian(pchunk->cid))
		{
		case VERTEX_COORDINATE:
			////Write_Debug(" VERTEX_COORDINATE");
			ReadVertexCoordinate(pdata, vertices);
		break;
		case VERTEX_NORMAL:
			////Write_Debug(" VERTEX_NORMAL");
			ReadVertexNormal(pdata, vertices);
		break;
		case VERTEX_NORMAL_UV:
			////Write_Debug(" VERTEX_NORMAL_UV");
			ReadVertexNormalUV(pdata, vertices);
		break;
		case VERTEX_UV:
			////Write_Debug(" VERTEX_UV");
			ReadVertexUV(pdata, vertices);
		break;
		}

#if LOOK_FOR_DEGENERATE_TRIANGLES
		size_t code = SuperFastHash((const char*)&vertices[i], sizeof(piUNIFIED_VERTEX));

		if(duplicate.find(code) != duplicate.end())
		{
			int stop = 0;
		}

		duplicate.insert(code);
#endif
	}

	index = FindTexPatIndex(s_face->texpatindex);
	if (index != 0xffff)
	{
		if (piIsTexture(tv.texid, 0))
		{
			//if (numuses[index] > 0)
			{
				piAddTexRef(tv.texid);
			}

			//numuses[index] += 1;
		}

		if ( isRoadTexture(tv.texid) )
		{
			tv.flag3 |= OBJECT_IS_ROAD;
//			tv.texid = ldTextureLoad("white.dds", 0);
		}

		if(_textureflags2[index] & OBJECT_NORMALMAP)
		{
			tv.flag2 |= OBJECT_NORMALMAP;
		}

		if(_textureflags3[index] & OBJECT_SPECULARMAP)
		{
			tv.flag3 |= OBJECT_SPECULARMAP;
		}

		if(_textureflags3[index] & OBJECT_COAST_WATER)
		{
			tv.flag3 |= OBJECT_COAST_WATER;
		}

		if(_textureflags3[index] & OBJECT_RIVER_WATER)
		{
			tv.flag3 |= OBJECT_RIVER_WATER;
		}
	}

	if ((hasalpha && isAlphaBlend()) || piRGBA_GETALPHA(facecolor) != 255)
	{
		tv.layer += 1;
		tv.flag2 |= OBJECT_HASALPHA;
	}

	if (tv.level != 0)
	{
		tv.flag2 &= ~OBJECT_HASALPHA;
	}

	if (tv.layer > 16)
	{
		tv.layer = 16;
	}

	if (s_face->drawtype != fltWIRE_OPEN && s_face->drawtype != fltWIRE_CLOSED)
	{
		// Triangulate the face and reverse vertex order to CCW
		for (int tri=0; tri < tv.numtris; tri++)
		{
			memcpy(tv.puverts + (tri*3), vertices, sizeof(piUNIFIED_VERTEX));
			memcpy(tv.puverts + (tri*3)+1, vertices + tri+2, sizeof(piUNIFIED_VERTEX));
			memcpy(tv.puverts + (tri*3)+2, vertices + tri+1, sizeof(piUNIFIED_VERTEX));
		}
	}
	else
	{
		memcpy(tv.puverts, vertices, tv.numverts * sizeof(piUNIFIED_VERTEX));
	}


#ifdef FORCE_DDS_NO_FLIP
	// Flip for dds textures
	TEXTURE* t = piGetTextureData(tv.texid);

	if(t)
	{
		if(t->isdds())
		{
			for(int vert=0; vert < tv.numverts; vert++)
			{	// Flip the v texture coordinate
				(tv.puverts + vert)->tv = 1 - (tv.puverts + vert)->tv;
			}
		}
	}
#endif

	// convert to DirectX handedness
	piVectorPtr spos = tv.StaticPositions();
	piVectorPtr snorm = tv.StaticPositions();
	piVertexColorPtr sc = tv.StaticColors();
	piTexCoordPtr stex = tv.StaticTexCoords();

	// close the line
	if (s_face->drawtype == fltWIRE_CLOSED)
	{
		spos[presvert]	= spos[0];
		snorm[presvert]	= snorm[0];
		sc[presvert] 	= sc[0];
		stex[presvert]	= stex[0];
	}

	if (s_face->drawtype != fltWIRE_OPEN && s_face->drawtype != fltWIRE_CLOSED)
	{
		for (i=0; i < tv.numtris; i++)
		{
			tv.ptris[i].v1 = (i*3);
			tv.ptris[i].v2 = (i*3) + 1;
			tv.ptris[i].v3 = (i*3) + 2;
		}
	}

	if (s_face->flags & fltFACE_HIDDEN)
	{
		tv.flag2 |= OBJECT_HIDDEN;
	}

	ldmdlGetBounds(&tv);
	lpobj = obcObject(attached, &tv);

	if (lpobj == nullptr)
	{
		return false;
	}
	else
	{
		if(g_bSpeedTrees)
		{
			lpobj->flag2 |= OBJECT_SPEEDTREE;
		}

		if(g_isGrass)
		{
			lpobj->flag2 |= OBJECT_IS_GRASS;
			lpobj->layer = g_grassType;
		}

	//		Write_Debug("Object face '%s' (%s) was created (%d)", tv.name, s_face->id, level);
		if (attached == nullptr)
		{
			if (groups[groupindex] != nullptr)
			{
				if (groups[groupindex]->child == nullptr)
				{
					groups[groupindex]->child = lpobj;
				}
			}

			groupindex += 1;
			if (groups[groupindex] != nullptr)
			{
				groups[groupindex]->sibling = lpobj;
			}

			grouplevels[groupindex] = level;
			groups[groupindex] = lpobj;

			headObject = lpobj;
			attached = lpobj;

			if(textcomment != nullptr)
			{
				SCHAR* p = textcomment;

				if(strstr(p, "popup="))
				{
					lpobj->flag3 |= OBJECT_HAS_POPUP;
				}

				if(strstr(p, "flag"))
				{
					lpobj->flag2 |= OBJECT_FLAG;
				}

				if(strstr(p, "grass"))
				{
					lpobj->flag2 |= OBJECT_IS_GRASS;

					SCHAR* typestr = strchr(p, '=');
					if(typestr) typestr++;

					if(typestr)
					{
						g_grassType = atol(typestr);
					}

					lpobj->layer = g_grassType;
					g_isGrass = true;
				}

				if(strstr(p, "capture"))
				{
					lpobj->flag3 |= OBJECT_CAPTURABLE;
				}

				if(strstr(p, "nofire"))
				{
					lpobj->flag3 |= OBJECT_NOFIRE;
				}

				lpobj->comment = textcomment;
				textcomment = nullptr;
			}
		}
		else
		{
			strcpy(lpobj->name, attached->name);
			attached = lpobj;
		}

		lastobject = lpobj;

		if (s_face->drawtype == fltSOLID_NOBACKFACE)
		{
			// Create a inverted face copy if the face is two sided
			// This increases the triangle count, but reduces the amount of batches

			OBJECT*	backfaceObj = obmainCopyObjectFull(lpobj);

			obmainAttachObject(attached, backfaceObj);
			attached = backfaceObj;

			if (s_face->drawtype != fltWIRE_OPEN && s_face->drawtype != fltWIRE_CLOSED)
			{
				for(i=0; i < num; i++)
				{
					// Invert the normals
					vertices[i].nx = -vertices[i].nx;
					vertices[i].ny = -vertices[i].ny;
					vertices[i].nz = -vertices[i].nz;
				}

				for (i=0; i < tv.numtris; i++)
				{
					// Reverse vertex order to CW
					memcpy(backfaceObj->puverts + (i*3), vertices, sizeof(piUNIFIED_VERTEX));
					memcpy(backfaceObj->puverts + (i*3)+1, vertices + i+1, sizeof(piUNIFIED_VERTEX));
					memcpy(backfaceObj->puverts + (i*3)+2, vertices + i+2, sizeof(piUNIFIED_VERTEX));
				}

#ifdef FORCE_DDS_NO_FLIP
				// Flip for dds textures
				TEXTURE* t = piGetTextureData(backfaceObj->texid);

				if(t)
				{
					if(t->isdds())
					{
						for(int vert=0; vert < backfaceObj->numverts; vert++)
						{	// Flip the v texture coordinate
							(backfaceObj->puverts + vert)->tv = 1 - (backfaceObj->puverts + vert)->tv;
						}
					}
				}
#endif
			}
			else
			{
				memcpy(backfaceObj->puverts, vertices, tv.numverts * sizeof(piUNIFIED_VERTEX));
			}
		}
	}

	offset += s_chunk->next;

	memFree(vertices);
	vertices = nullptr;

	return true;
}


static bool ReadMatrix(void)
{
	if (!localBufferCopyObject(&s_matrixRecord))
		return false;

	for(size_t i=0; i<16; i++)
	{
		*((float*)&s_matrixRecord + i) = FloatEndian(*((float*)&s_matrixRecord + i));
	}

	offset += s_chunk->next;

	return true;
}

static bool ReadMaterialTable(const char* name)
{
	const size_t materialsLen = s_chunk->next - 4;
	s_nummaterials = materialsLen / sizeof(fltMATERIAL);
	if (s_nummaterials > MAX_FLT_MATERIALS)
	{
		Write_Debug("fltLoad failed at %s:%u", __FILE__, __LINE__);
		return false;
	}
	if (s_nummaterials * sizeof(fltMATERIAL) != materialsLen)
	{
		Write_Debug("Bad materials data");
		return false;
	}

	material = (fltMATERIAL*)memAlloc(materialsLen);
	if (!localBufferCopyBytes(material, materialsLen))
		return false;

	matlevel = level;

	for (size_t i = 0; i < s_nummaterials; ++i)
	{
		Write_Debug("Material %u loaded: %s", i, material[i].name);

		material[i].ambRed	= FloatEndian(material[i].ambRed);
		material[i].ambGreen	= FloatEndian(material[i].ambGreen);
		material[i].ambBlue	= FloatEndian(material[i].ambBlue);
		material[i].diffRed	= FloatEndian(material[i].diffRed);
		material[i].diffGreen	= FloatEndian(material[i].diffGreen);
		material[i].diffBlue	= FloatEndian(material[i].diffBlue);
		material[i].specRed	= FloatEndian(material[i].specRed);
		material[i].specGreen	= FloatEndian(material[i].specGreen);
		material[i].specBlue	= FloatEndian(material[i].specBlue);
		material[i].emissRed	= FloatEndian(material[i].emissRed);
		material[i].emissGreen = FloatEndian(material[i].emissGreen);
		material[i].emissBlue	= FloatEndian(material[i].emissBlue);
		material[i].shininess	= FloatEndian(material[i].shininess);
		material[i].alpha		= FloatEndian(material[i].alpha);
		material[i].flags		= LongEndian(material[i].flags);

		// No name, no material (we key off name!)
		if (material[i].name[0] != 0)
		{
			memset(&mat, 0, sizeof(mat));
			strcpy(mat.name, material[i].name);

			mat.material.ambient.r = material[i].ambRed;
			mat.material.ambient.g = material[i].ambGreen;
			mat.material.ambient.b = material[i].ambBlue;
			mat.material.ambient.a = material[i].alpha;

			mat.material.diffuse.r = material[i].diffRed;
			mat.material.diffuse.g = material[i].diffGreen;
			mat.material.diffuse.b = material[i].diffBlue;
			mat.material.diffuse.a = material[i].alpha;

			mat.material.specular.r = material[i].specRed;
			mat.material.specular.g = material[i].specGreen;
			mat.material.specular.b = material[i].specBlue;
			mat.material.specular.a = material[i].alpha;

			mat.material.emissive.r = material[i].emissRed;
			mat.material.emissive.g = material[i].emissGreen;
			mat.material.emissive.b = material[i].emissBlue;
			mat.material.emissive.a = material[i].alpha;

			mat.material.power = material[i].shininess;

//			Write_Debug("Material name: %s", mat.matname);
			if (!obcMaterial(&mat))
			{
				return false;
			}
		}
	}

	offset += s_chunk->next;

	return true;
}

static bool ReadMaterialPalette(const char* name)
{
	if (material == nullptr)
	{
		material = (fltMATERIAL*)memAlloc(64 * sizeof(fltMATERIAL));
		matlevel = level;
	}

	fltMATERIAL_PALETTE matpal;
	if (!localBufferCopyObject(&matpal))
		return false;

	size_t i = LongEndian(matpal.index);
	if(i >= 64)
	{
		Write_Log(LOG_OLD, LOG_WARN, "[ReadMaterialPalette] material index is %d. It should be 0<=x<=63.", i);
		return false;
	}

	material[i].ambRed		= FloatEndian(matpal.ambRed);
	material[i].ambGreen	= FloatEndian(matpal.ambGreen);
	material[i].ambBlue		= FloatEndian(matpal.ambBlue);
	material[i].diffRed		= FloatEndian(matpal.diffRed);
	material[i].diffGreen	= FloatEndian(matpal.diffGreen);
	material[i].diffBlue	= FloatEndian(matpal.diffBlue);
	material[i].specRed		= FloatEndian(matpal.specRed);
	material[i].specGreen	= FloatEndian(matpal.specGreen);
	material[i].specBlue	= FloatEndian(matpal.specBlue);
	material[i].emissRed	= FloatEndian(matpal.emissRed);
	material[i].emissGreen	= FloatEndian(matpal.emissGreen);
	material[i].emissBlue	= FloatEndian(matpal.emissBlue);
	material[i].shininess	= FloatEndian(matpal.shininess);
	material[i].alpha		= FloatEndian(matpal.alpha);
	material[i].flags		= LongEndian(matpal.flags);
	strcpy(material[i].name, matpal.name);

	// HACK! for max to flt plugging anomalies
	// If there is a '*' in the name, then use the name previous to the '*'
	// MAX requires unique names, and FLT limit of 12 chars for material names makes this work.
	char* star = strchr(matpal.name, '*');
	if (star)
		*star = '\0';
	strcpy(material[i].name, matpal.name);

	// No name, no material (we key off name!)
	if (material[i].name[0] != 0)
	{
		memset(&mat, 0, sizeof(mat));
		mat.material.power = 1.0f;
		strcpy(mat.name, material[i].name);

		mat.material.ambient.r = material[i].ambRed;
		mat.material.ambient.g = material[i].ambGreen;
		mat.material.ambient.b = material[i].ambBlue;
		mat.material.ambient.a = material[i].alpha;

		mat.material.diffuse.r = material[i].diffRed;
		mat.material.diffuse.g = material[i].diffGreen;
		mat.material.diffuse.b = material[i].diffBlue;
		mat.material.diffuse.a = material[i].alpha;

		mat.material.specular.r = material[i].specRed;
		mat.material.specular.g = material[i].specGreen;
		mat.material.specular.b = material[i].specBlue;
		mat.material.specular.a = material[i].alpha;

		mat.material.emissive.r = material[i].emissRed;
		mat.material.emissive.g = material[i].emissGreen;
		mat.material.emissive.b = material[i].emissBlue;
		mat.material.emissive.a = material[i].alpha;

		mat.material.power = material[i].shininess;

//		Write_Debug("Material name: %s", mat.matname);
		if (!obcMaterial(&mat))
		{
			return false;
		}

		++s_nummaterials;
		if (s_nummaterials > 64)
		{
			Write_Debug("Too many materials!");
			return false;
		}
	}

	offset += s_chunk->next;

	return true;
}


static bool ReadColorPalette(void)
{
	size_t length = s_chunk->next - 4;
	fltCOLORPALETTE* colorpal;
	localBufferUseBytes(colorpal, length);

	length = (length - 128) / sizeof(UINT32);
	s_colortable.resize(length * 128);

	colorstruct* color = &s_colortable.front();
	for (size_t i=0; i < length; ++i)
	{
		for (size_t j=0; j < 128; ++color, ++j)
		{
			color->full			=	colorpal->briteRGB[i];
			color->rgb.red		=	color->rgb.red * j / 127;
			color->rgb.green	=	color->rgb.green * j / 127;
			color->rgb.blue		=	color->rgb.blue * j / 127;
		}
	}

	colorlevel = level;

	offset += s_chunk->next;

	return true;
}


static bool ReadTextureReference(void)
{
	char filename[256];
	filename[0] = '\0';

	if(texref == nullptr)
	{
		texref = (fltTEXTUREREFERENCE*)memAlloc(MAX_TEXTURE_REFS * sizeof(fltTEXTUREREFERENCE));
		texlevel = level;
	}

	if (!localBufferCopyObject(&texref[numtexrefs]))
		return false;

	texref[numtexrefs].patindex = LongEndian(texref[numtexrefs].patindex);
	texref[numtexrefs].x		= LongEndian(texref[numtexrefs].x);
	texref[numtexrefs].y		= LongEndian(texref[numtexrefs].y);

	// HACK to work around MAX flt plugin texture paths
	std::string str = texref[numtexrefs].name;
	UINT32 pos = str.rfind('\\');

	if(pos == std::string::npos)
		pos = str.rfind('/');

	UINT32 pos2 = str.rfind('.');
	pos2 = pos2 == std::string::npos ? str.length() : pos2;

	// Copy texture name
	str = str.substr(pos + 1, pos2 - pos - 1);

	strcpy(texref[numtexrefs].name, str.c_str());

	lstrcpy(filename, pathname);
	lstrcat(filename, texref[numtexrefs].name);

	offset += s_chunk->next;

	// If the name is not a filename (for a textured surface),
	// then the surface will be untextured...
	texid[numtexrefs] = ldpixLoadImageFileToSurface(filename, colorBLACK, LDPIX_MIPMAP);

	if(!piIsTexture(texid[numtexrefs], 0))
		return FLTUnknownTexture(filename), false;

	//numuses[numtexrefs] = 0;
	texalpha[numtexrefs] = piTextureHasAlpha(texid[numtexrefs]);

	// Check for normal map
	str = filename;
	str += "_normal.dds";

	if(fioAccess(str.c_str()))
	{
		_textureflags2[numtexrefs] |= OBJECT_NORMALMAP;
	}

	// New normal mapping tool outputs _nrm.dds
	str = filename;
	str += "_nrm.dds";

	if(fioAccess(str.c_str()))
	{
		_textureflags2[numtexrefs] |= OBJECT_NORMALMAP;
	}

	// Check for specular map
	str = filename;
	str += "_spec.dds";

	if(fioAccess(str.c_str()))
	{
		_textureflags3[numtexrefs] |= OBJECT_SPECULARMAP;
	}

	str = filename;

	if(str.find("water") != std::string::npos)
	{
		_textureflags3[numtexrefs] |= OBJECT_COAST_WATER;
	}

	if(str.find("rivers") != std::string::npos)
	{
		_textureflags3[numtexrefs] |= OBJECT_RIVER_WATER;
	}

	numtexrefs++;

	return true;
}

static bool ReadFace(void)
{
	if (!localBufferUseObject(s_face))
		return false;

	facelevel = level;

	s_face->ircolor        = LongEndian(s_face->ircolor);
	s_face->priority       = ShortEndian(s_face->priority);
	s_face->colorname      = ShortEndian(s_face->colorname);
	s_face->altcolorname   = ShortEndian(s_face->altcolorname);
	s_face->dettexpat      = ShortEndian(s_face->dettexpat);
	s_face->texpatindex    = ShortEndian(s_face->texpatindex);
	s_face->matindex       = ShortEndian(s_face->matindex);
	s_face->surfmatcode    = ShortEndian(s_face->surfmatcode);
	s_face->fid            = ShortEndian(s_face->fid);
	s_face->irmat          = LongEndian(s_face->irmat);
	s_face->transparency   = ShortEndian(s_face->transparency);
	s_face->flags          = LongEndian(s_face->flags);
	s_face->color          = LongEndian(s_face->color);
	s_face->altcolor       = LongEndian(s_face->altcolor);
	s_face->texmapindex    = ShortEndian(s_face->texmapindex);
	s_face->colorindex     = LongEndian(s_face->colorindex);
	s_face->altcolorindex  = LongEndian(s_face->altcolorindex);


	//	Write_Debug("Processing face: %s (%d)", s_face->id, level);

	if (objectlevel == -1)
	{
		Write_Debug("Error! Trying to add a face when no object has been defined!");
		return false;
	}

	memset(&tv, 0, sizeof(tv));

	if (topmost)
	{
		OBJECT* o = obcObject(attached, &tv);
		o->flag2 = OBJECT_TOPMOST;
		obmainMoveToNotObjectTree(o);
		topmostObject = o;
		topmost = false;
	//		Write_Debug("Topmost object was tagged");
	}

	if (fltrevision < 1510)
	{
		facecolor = CreateColor(s_face->colorname);
	}
	else
	{
		facecolor = CreateColor(s_face->colorindex);
	}

	hasalpha = false;

	offset += s_chunk->next;

	return true;
}


static bool ReadLOD(void)
{
	fltLOD* lod;
	if (!localBufferUseObject(lod))
		return false;

	const bool hasFx = (strncmp(lod->name, "fx", 2) == 0);

	s_currentLod.level = level;
	s_currentLod.name = lod->name;
	s_currentLod.switchin = DoubleEndian(lod->switchin);
	s_currentLod.switchout = DoubleEndian(lod->switchout);
	// these all appear to be unused, so no point translating them.
	//s_currentLod.spfxid1     = ShortEndian(lod->spfxid1);
	//s_currentLod.spfxid2     = ShortEndian(lod->spfxid2);
	//s_currentLod.flags       = LongEndian(lod->flags);
	//s_currentLod.centerx     = DoubleEndian(lod->centerx);
	//s_currentLod.centery     = DoubleEndian(lod->centery);
	//s_currentLod.centerz     = DoubleEndian(lod->centerz);
	//s_currentLod.transition  = DoubleEndian(lod->transition);

	OBJECT*	lpobj;
	if (!CreateLevel(&lpobj))
	{
		return false;
	}

	lpobj->flags = OBJECT_LOD;

	if(hasFx)
	{
		lpobj->flag2 |= OBJECT_HAS_FX;
	}

	strcpy(lpobj->name, s_currentLod.name);
	lpobj->minlod = (float)(s_currentLod.switchout * s_currentLod.switchout) * localscale * localscale;
	lpobj->maxlod = (float)(s_currentLod.switchin * s_currentLod.switchin) * localscale * localscale;

	attached = nullptr;
	lastobject = lpobj;

	//	Write_Debug("Processing LOD: %s; in=%g, out=%g (%d)", lod->name, (float)s_currentLod.switchin, (float)s_currentLod.switchout, s_currentLod.level);

	offset += s_chunk->next;

	return true;
}


static bool ReadObject(void)
{
	if (!localBufferCopyObject(&object))
		return false;

	strcpy(objname[level], object.id);
	strlwr(objname[level]);
	for (size_t i=level+1; i < MAX_LEVELS; i++)
	{
		strcpy(objname[i], objname[level]);
	}

	object.flags		= LongEndian(object.flags);
	object.priority		= ShortEndian(object.priority);
	object.transparency	= ShortEndian(object.transparency);
	object.spfxid1		= ShortEndian(object.spfxid1);
	object.spfxid2		= ShortEndian(object.spfxid2);
	object.significance	= ShortEndian(object.significance);

	//	Write_Debug("Processing object: %s (%d)", objname[level], level);

	attached = nullptr;
	headObject	= nullptr;
	lastobject	= nullptr;
	objectlevel = level;
	numvertices = 0;

	offset += s_chunk->next;

	return true;
}


static bool ReadExtension(void)
{
	fltEXTENSION*  ext;
	if (!localBufferUseObject(ext))
		return false;

#if (0)
	ext->code = ShortEndian(ext->code);
	ext->reserved1 = 0;  // terminate the site id
	//Write_Debug(" extension: '%s', '%s', code=0x%x, level=%d", ext->id, ext->siteid, ext->code, extlevel);
#endif

	offset += s_chunk->next;

	return true;
}

static bool ReadDOF(void)
{
	OBJECT*	lpobj;

	fltDOF* dofPtr;
	if (!localBufferUseObject(dofPtr))
		return false;
	fltDOF& dof = *dofPtr;

#ifdef DEBUG_FPU
	SetupFPU(false);
#endif

	dof.reserved		= LongEndian(dof.reserved);
	dof.originx			= DoubleEndian(dof.originx);
	dof.originy			= DoubleEndian(dof.originy);
	dof.originz			= DoubleEndian(dof.originz);
	dof.pointxaxisx		= DoubleEndian(dof.pointxaxisx);
	dof.pointxaxisy		= DoubleEndian(dof.pointxaxisy);
	dof.pointxaxisz		= DoubleEndian(dof.pointxaxisz);
	dof.pointxyplanex	= DoubleEndian(dof.pointxyplanex);
	dof.pointxyplaney	= DoubleEndian(dof.pointxyplaney);
	dof.pointxyplanez	= DoubleEndian(dof.pointxyplanez);
	dof.minz			= DoubleEndian(dof.minz);
	dof.maxz			= DoubleEndian(dof.maxz);
	dof.curz			= DoubleEndian(dof.curz);
	dof.incz			= DoubleEndian(dof.incz);
	dof.miny			= DoubleEndian(dof.miny);
	dof.maxy			= DoubleEndian(dof.maxy);
	dof.cury			= DoubleEndian(dof.cury);
	dof.incy			= DoubleEndian(dof.incy);
	dof.minx			= DoubleEndian(dof.minx);
	dof.maxx			= DoubleEndian(dof.maxx);
	dof.curx			= DoubleEndian(dof.curx);
	dof.incx			= DoubleEndian(dof.incx);
	dof.minpitchx		= DoubleEndian(dof.minpitchx);
	dof.maxpitchx		= DoubleEndian(dof.maxpitchx);
	dof.curpitchx		= DoubleEndian(dof.curpitchx);
	dof.incpitchx		= DoubleEndian(dof.incpitchx);
	dof.minpitchy		= DoubleEndian(dof.minpitchy);
	dof.maxpitchy		= DoubleEndian(dof.maxpitchy);
	dof.curpitchy		= DoubleEndian(dof.curpitchy);
	dof.incpitchy		= DoubleEndian(dof.incpitchy);
	dof.minpitchz		= DoubleEndian(dof.minpitchz);
	dof.maxpitchz		= DoubleEndian(dof.maxpitchz);
	dof.curpitchz		= DoubleEndian(dof.curpitchz);
	dof.incpitchz		= DoubleEndian(dof.incpitchz);
	dof.minzscale		= DoubleEndian(dof.minzscale);
	dof.maxzscale		= DoubleEndian(dof.maxzscale);
	dof.curzscale		= DoubleEndian(dof.curzscale);
	dof.inczscale		= DoubleEndian(dof.inczscale);
	dof.minyscale		= DoubleEndian(dof.minyscale);
	dof.maxyscale		= DoubleEndian(dof.maxyscale);
	dof.curyscale		= DoubleEndian(dof.curyscale);
	dof.incyscale		= DoubleEndian(dof.incyscale);
	dof.minxscale		= DoubleEndian(dof.minxscale);
	dof.maxxscale		= DoubleEndian(dof.maxxscale);
	dof.curxscale		= DoubleEndian(dof.curxscale);
	dof.incxscale		= DoubleEndian(dof.incxscale);
	dof.flags			= LongEndian(dof.flags);

#ifdef DEBUG_FPU
	SetupFPU(true);
#endif

	if (!CreateLevel(&lpobj))
	{
		return false;
	}


	lpobj->flags = OBJECT_DOF;
	if (topmost)
	{
		lpobj->flag2 = OBJECT_TOPMOST;
		obmainMoveToNotObjectTree(lpobj);
		topmostObject = lpobj;
		topmost = false;
		Write_Debug("Topmost object was tagged");
	}

	strcpy(lpobj->name, dof.name);

	lastobject = lpobj;

	lpobj->dof = (OBJECTDOF*)memAlloc(sizeof(OBJECTDOF));
	lpobj->dof->dofstatic = (OBJECTSTATICDOF*)memAlloc(sizeof(OBJECTSTATICDOF));

	lpobj->dof->dofstatic->doforigin.x = (float)dof.originx;
	lpobj->dof->dofstatic->doforigin.y = (float)dof.originy;
	lpobj->dof->dofstatic->doforigin.z = (float)dof.originz;

	lpobj->dof->dofstatic->dofaxis.x = (float)dof.pointxaxisx;
	lpobj->dof->dofstatic->dofaxis.y = (float)dof.pointxaxisy;
	lpobj->dof->dofstatic->dofaxis.z = (float)dof.pointxaxisz;

	lpobj->dof->dofstatic->dofplane.x = (float)dof.pointxyplanex;
	lpobj->dof->dofstatic->dofplane.y = (float)dof.pointxyplaney;
	lpobj->dof->dofstatic->dofplane.z = (float)dof.pointxyplanez;

	lpobj->dof->dofstatic->dofmintrans.x = (float)dof.minx;
	lpobj->dof->dofstatic->dofmaxtrans.x = (float)dof.maxx;
	lpobj->dof->dofstatic->dofcurtrans.x = (float)dof.curx;
	lpobj->dof->dofstatic->dofinctrans.x = (float)dof.incx;
	lpobj->dof->dofstatic->dofmintrans.y = (float)dof.miny;
	lpobj->dof->dofstatic->dofmaxtrans.y = (float)dof.maxy;
	lpobj->dof->dofstatic->dofcurtrans.y = (float)dof.cury;
	lpobj->dof->dofstatic->dofinctrans.y = (float)dof.incy;
	lpobj->dof->dofstatic->dofmintrans.z = (float)dof.minz;
	lpobj->dof->dofstatic->dofmaxtrans.z = (float)dof.maxz;
	lpobj->dof->dofstatic->dofcurtrans.z = (float)dof.curz;
	lpobj->dof->dofstatic->dofinctrans.z = (float)dof.incz;

	lpobj->dof->dofstatic->dofminrotate.x = (float)dof.minpitchx;
	lpobj->dof->dofstatic->dofmaxrotate.x = (float)dof.maxpitchx;
	lpobj->dof->dofstatic->dofcurrotate.x = (float)dof.curpitchx;
	lpobj->dof->dofstatic->dofincrotate.x = (float)dof.incpitchx;
	lpobj->dof->dofstatic->dofminrotate.y = (float)dof.minpitchy;
	lpobj->dof->dofstatic->dofmaxrotate.y = (float)dof.maxpitchy;
	lpobj->dof->dofstatic->dofcurrotate.y = (float)dof.curpitchy;
	lpobj->dof->dofstatic->dofincrotate.y = (float)dof.incpitchy;
	lpobj->dof->dofstatic->dofminrotate.z = (float)dof.minpitchz;
	lpobj->dof->dofstatic->dofmaxrotate.z = (float)dof.maxpitchz;
	lpobj->dof->dofstatic->dofcurrotate.z = (float)dof.curpitchz;
	lpobj->dof->dofstatic->dofincrotate.z = (float)dof.incpitchz;

	lpobj->dof->dofstatic->dofminscale.x = (float)dof.minxscale;
	lpobj->dof->dofstatic->dofmaxscale.x = (float)dof.maxxscale;
	lpobj->dof->dofstatic->dofcurscale.x = (float)dof.curxscale;
	lpobj->dof->dofstatic->dofincscale.x = (float)dof.incxscale;
	lpobj->dof->dofstatic->dofminscale.y = (float)dof.minyscale;
	lpobj->dof->dofstatic->dofmaxscale.y = (float)dof.maxyscale;
	lpobj->dof->dofstatic->dofcurscale.y = (float)dof.curyscale;
	lpobj->dof->dofstatic->dofincscale.y = (float)dof.incyscale;
	lpobj->dof->dofstatic->dofminscale.z = (float)dof.minzscale;
	lpobj->dof->dofstatic->dofmaxscale.z = (float)dof.maxzscale;
	lpobj->dof->dofstatic->dofcurscale.z = (float)dof.curzscale;
	lpobj->dof->dofstatic->dofincscale.z = (float)dof.inczscale;

	//	Write_Debug("Processing DOF: %s (%d)", dof.name, level);

	doflevel = level;

	offset += s_chunk->next;

	return true;
}

static bool ReadTextComment(void)
{
	char*	thisTextComment = 0;
	SINT32	len;


	len = s_chunk->next - 4;

	thisTextComment = (char*)memAlloc(len+1);
	if (!localBufferCopyBytes(thisTextComment, len))
		return false;
	len -= 1;

	textlevel = level;
	if(textlevel != facelevel || facelevel < 0)
	{
		if(textcomment != nullptr)
		{
			Write_Debug("Old text comment was unused");
			memFree(textcomment);
		}
		textcomment = thisTextComment;
	}
	else
	{	// Don't take comments from face attributes!
		memFree(thisTextComment);
		thisTextComment = nullptr;
		offset += s_chunk->next;
		return true;
	}

	// trim white space and junk
	while (len >= 0 && iscntrl(textcomment[len]))
	{
		textcomment[len] = 0;
		--len;
	}

	if (len < 0)
	{
		// comment has no data
		memFree(textcomment);
		textcomment = nullptr;
	}
	else
	{
		SCHAR* p = textcomment;

		if(lastobject != nullptr)
		{
			if(strstr(p, "flag"))
			{
				lastobject->flag2 |= OBJECT_FLAG;
			}

			if(strstr(p, "popup="))
			{
				lastobject->flag3 |= OBJECT_HAS_POPUP;
			}

			if(strstr(p, "grass"))
			{
				lastobject->flag2 |= OBJECT_IS_GRASS;

				SCHAR* typestr = strchr(p, '=');
				if(typestr) typestr++;

				if(typestr)
				{
					g_grassType = atol(typestr);
				}

				lastobject->layer = g_grassType;
				g_isGrass = true;
			}

			if(strstr(p, "spawnobject"))
			{
				lastobject->flag3 |= OBJECT_SPAWN;
			}

			if(strstr(p, "capture"))
			{
				lastobject->flag3 |= OBJECT_CAPTURABLE;
			}

			if(strstr(p, "nofire"))
			{
				lastobject->flag3 |= OBJECT_NOFIRE;
			}

			lastobject->comment = textcomment;
			textcomment = nullptr;
			textlevel = -1;
		}
	}

	offset += s_chunk->next;

	return true;
}

static bool ReadLongID(void)
{
	size_t len = size_t(s_chunk->next - 4);
	SCHAR	oldname[32];
	SCHAR	tempname[32];
	SINT32	i;

	char*	completeString = (char*)alloca(len + 1);
	if (PN_UNLIKELY(!localBufferCopyBytes(completeString, len)))
	{
		Write_Debug("[ReadLongID] FAILURE: bad read\n", lastobject->name, s_conversionFile.c_str());
		return false;
	}

	completeString[std::min(len, sizeof(tempname) - 1)] = 0;
	strcpy(tempname, completeString);
	strlwr(tempname);

	// HACK to get around some Max to FLT plugin issues
	if(!strncmp(tempname, "speedtr", 7))
	{
		lastobject->flag2 |= OBJECT_SPEEDTREE;
		g_bSpeedTrees = true;
		_hasspeedtree = true;
	}

	if (facelevel == -1)
	{
		strcpy(objname[level], tempname);
		for (i=level+1; i < MAX_LEVELS; i++)
		{
			strcpy(objname[i], tempname);
		}
	}

	if (lastobject)
	{
		strcpy(oldname, lastobject->name);
		if (facelevel == -1)
		{
			strcpy(lastobject->name, tempname);
			strcpy(objname[level], tempname);
		//			Write_Debug("Existing object name changed from '%s' to '%s' (%d)", oldname, tempname, level);
		}
		else
		{
		//			Write_Debug("Existing object name '%s' was not changed to '%s' (%d)", oldname, tempname, level);
		}

		// Check if topmost name is the file name...required.
		if (bTopmostObject)
		{
			bTopmostObject = false;

			// Make sure we are staring at the same address for topmost
			if (lastobject == groups[0])
			{
				if (stricmp(lastobject->name, s_conversionFile.c_str()))
				{
					FLTItemFileError("ReadLongID", lastobject->name, s_conversionFile.c_str());
					return false;
				}
			}
		}
	}
	else
	{
		if (facelevel == -1)
		{
		//			Write_Debug("Uncreated object name set to '%s' (%d)", objname[level], level);
		}
		else
		{
		//			Write_Debug("Uncreated object '%s', name unchanged: '%s' (%d)", objname[level], tempname, level);
		}
	}

	offset += s_chunk->next;

	return true;
}

static bool ReadPushLevel(void)
{
	level += 1;
	if (level >= MAX_LEVELS)
	{
		Write_Debug("Exceeded maximum number of supported levels: %d; max = %d", level, MAX_LEVELS);
		return false;
	}

	//Write_Debug("Down to level %d", level);

	offset += s_chunk->next;

	return true;
}


static bool ReadPopLevel(void)
{
	if (vlistlevel == level)
	{
		vlistlevel = -1;
		if (vertlist != nullptr)
		{
			memFree(vertlist);
			vertlist = nullptr;
		}
	}

	if (s_currentLod.level == level)
		s_currentLod.level = -1;

	if (facelevel == level)
		facelevel = -1;

	--level;
	if (level == 0)
		topmost = true;

	//Write_Debug("Up to level %d", level);

	if (groupindex >= 0)
	{
		if (grouplevels[groupindex] >= level)
		{
			//Write_Debug("Up a group level: %d (%d)", groupindex, level);

			for (size_t i=groupindex+1; i < MAX_GROUP_SIZE; i++)
			{
				grouplevels[i] = -1;
				groups[i] = nullptr;
			}

			groupindex -= 1;
		}
	}


	if (matlevel == level)
	{
		matlevel = -1;
		s_nummaterials = 0;
		if (material != nullptr)
		{
			memFree(material);
			material = nullptr;
		}
	}

	if (texlevel == level)
	{
		texlevel = -1;
		if (texref != nullptr)
		{
			memFree(texref);
			texref = nullptr;
			for (SINT32 i=0; i < numtexrefs; i++)
			{
				piFreeMipTextures(texid[i]);
			}

			numtexrefs = 0;

			memset(_textureflags2, 0, sizeof(unsigned int) * MAX_TEXTURE_REFS);
			memset(_textureflags3, 0, sizeof(unsigned int) * MAX_TEXTURE_REFS);
		}
	}

	if (textlevel == level)
	{
		textlevel = -1;
		if (textcomment)
		{
			memFree(textcomment);
			textcomment = nullptr;
		}
	}

	if (colorlevel == level)
	{
		colorlevel = -1;
		s_colortable.clear();
	}

	if (doflevel == level)
	{
		doflevel = -1;
	}

	if (objectlevel == level)
	{
		objectlevel = -1;
		ldmdlUpdateLocation(headObject);

		if( objreduce == 1 )
		{
			ldmdlReduceAttachments(headObject);
			lastobject = nullptr;
		}

		if ( objreduce == 2 )
		{
			//ldmdlSortObjectTriangles(attached);
		}

		if(headObject && headObject->comment && strstr(headObject->comment, "instance "))
		{
			memcpy(&headObject->mat, &s_matrixRecord, sizeof(s_matrixRecord));
			s_matrixRecord = IdentityMatrix;
		}
	}

	offset += s_chunk->next;

	return true;
}


static bool ReadPushSubface(void)
{
	facelayer += 1;

	offset += s_chunk->next;

	return true;
}


static bool ReadPopSubface(void)
{
	facelayer -= 1;

	offset += s_chunk->next;

	return true;
}


static bool ReadPushExtension(void)
{
	extlevel += 1;

	offset += s_chunk->next;

	return true;
}


static bool ReadPopExtension(void)
{
	extlevel -= 1;

	offset += s_chunk->next;

	return true;
}

static bool DisplayTree(OBJECT* lpobj, int level)
{
	OBJECT*	lpatt;

	if (lpobj)
	{
	//		Write_Debug("%s (%d)", lpobj->name, level);

		lpatt = lpobj->attached;
		while (lpatt)
		{
			lpatt = lpatt->attached;
		}

		if (lpobj->child)
		{
			if (!DisplayTree(lpobj->child, level+1))
			{
				return false;
			}
		}

		if (lpobj->sibling)
		{
			if (!DisplayTree(lpobj->sibling, level))
			{
				return false;
			}
		}
	}

	return true;
}

#ifdef MAP_MATS
// this doesn't work
typedef std::map<std::string,std::string> MaterialMapper;
extern MaterialMapper materialMap;

static void changeMaterials( OBJECT* obj )
{
	if ( obj )
	{
		std::string altmatname = materialMap[obj->matname];
		if ( altmatname.length()>0 && altmatname != obj->matname )
		{
			int mid = oglFindMaterialName( altmatname.c_str() );
			if ( mid != -1 )
			{
				printf( "change material %s to material %s\n", obj->matname, altmatname.c_str() );
				strcpy( obj->matname, altmatname.c_str() );
				obj->matid = mid;
			}

		}

		changeMaterials( obj->child );
		changeMaterials( obj->attached );
		changeMaterials( obj->sibling );
	}
}
#endif

#define xSHOWLDMSGS 1
#if platOS == OS_MACOS
#define OutputDebugStringA(x) printf(x)
//#undef Write_Debug
//#define Write_Debug DoOutputX

void DoOutputX(const char* str, ...)
{
	printf( "%s\n", str);
}

#endif

#ifdef SHOWLDMSGS
# define LDWrite_Debug(...) WriteToConsole(__VA_ARGS__); Write_Debug(__VA_ARGS__)
#else
# define LDWrite_Debug(...)
#endif

static OBJECT* _fltLoad(const char* const name, const float scale, const int reduce)
{
	SINT32 tstart = timeGetTime();

	s_loadError = "";
	memset(_textureflags2, 0, sizeof(unsigned int) * MAX_TEXTURE_REFS);
	memset(_textureflags3, 0, sizeof(unsigned int) * MAX_TEXTURE_REFS);

	localscale = scale;
	objreduce = reduce;

	// To convert to DirectX Handedness
	translate =	IdentityMatrix;
	translate._11 = localscale;
	translate._22 = localscale;
	translate._33 = -localscale;

	level		= 0;
	s_currentLod.level = -1;
	objectlevel	= -1;
	doflevel	= -1;
	textlevel	= -1;
	facelevel	= -1;
	facelayer	= 0;
	vlistlevel	= -1;
	extlevel	= -1;
	numtexrefs	= 0;
	attached		= nullptr;
	lastobject		= nullptr;
	headObject		= nullptr;
	topmostObject	= nullptr;
	s_matrixRecord	= IdentityMatrix;
	s_face = nullptr;

	meshpool_idx = 0;
	memset( meshpools, 0, sizeof(meshpools) );

	for (size_t i=0; i < MAX_GROUP_SIZE; i++)
	{
		grouplevels[i] = -1;
		groups[i] = nullptr;
	}

	groupindex = -1;

	topmost = true;

	// open flt file
	strcpy(filename, name);
	int stream = fioOpen(filename);
	if (-1 == stream)
	{
		strcpy(filename, TERRAIN_PREPATH);
		strcat(filename, name);
		stream = fioOpen(filename);
		if (-1 == stream)
			return FLTLoadFailure("Unable to read flt file", "open"), nullptr;
	}

	// new code to buffer data locally
	size_t fsize = fioFileSize(filename);
	std::vector<char> buffer(fsize);
	streamBuffer = buffer; // this will be invalid when routine exits
	streamOffset = 0;
	fioRead(stream,&streamBuffer[0],fsize);

	strcpy(pathname, filename);
	char* ptr = strrchr(pathname, '\\');
	if (ptr != nullptr)
	{
		ptr += 1;	// Leave the '\'
		*ptr = 0;
	}
	else
	{
		pathname[0] = 0;
	}

	offset = 0;

	strcpy(filename, name);
	ptr = (char*)strrchr(filename, '.');
	if (ptr != nullptr)
		*ptr = 0;

	ptr = (char*)strrchr(filename, '\\');
	if (ptr != nullptr)
	{
		ptr += 1;
		strcpy(filename, ptr);
	}

	// Need s_conversionFile to compare group.id and filename in ReadGroup
	s_conversionFile = filename;

	for (;;)
	{
		if (localBufferUseObject(s_chunk) != sizeof(*s_chunk))
			break;

		if ( s_chunk->next == 0 )
			return FLTLoadFailure("Bad chunk in flt file", "read"), false;

		//	There is a bug in the later versions of the .FLT file format.
		//	The POP_LEVEL commands at the end of a file are not endian converted.
		if (s_chunk->cid != POP_LEVEL)
		{
			s_chunk->cid = ShortEndian(s_chunk->cid);
			s_chunk->next = ShortEndian(s_chunk->next);
		}

		switch (s_chunk->cid)
		{
		case HEADER_RECORD:
			LDWrite_Debug("HEADER_RECORD");
			if(!ReadHeader())
				return FLTChunkFailure("HEADER_RECORD"), nullptr;
		break;
		case GROUP_RECORD:
			LDWrite_Debug("GROUP_RECORD");
			if(!ReadGroup())
				return FLTChunkFailure("GROUP_RECORD"), nullptr;
		break;
		case OBJECT_RECORD:
			LDWrite_Debug("OBJECT_RECORD");
			if(!ReadObject())
				return FLTChunkFailure("OBJECT_RECORD"), nullptr;
		break;
		case FACE_RECORD:
			LDWrite_Debug("FACE_RECORD");
			if(!ReadFace())
				return FLTChunkFailure("FACE_RECORD"), nullptr;
		break;
		case PUSH_LEVEL:
			if(!ReadPushLevel())
				return FLTChunkFailure("PUSH_LEVEL"), nullptr;
			LDWrite_Debug("PUSH_LEVEL: %d", level);
		break;
		case POP_LEVEL:
			LDWrite_Debug("POP_LEVEL: %d", level);
			if(!ReadPopLevel())
				return FLTChunkFailure("POP_LEVEL"), nullptr;
		break;
		case DOF_RECORD:
			LDWrite_Debug("DOF");
			if (!ReadDOF())
				return FLTChunkFailure("DOF_RECORD"), nullptr;
		break;
		case PUSH_SUBFACE:
			LDWrite_Debug("PUSH_SUBFACE");
			if(!ReadPushSubface())
				return FLTChunkFailure("PUSH_SUBFACE"), nullptr;
		break;
		case POP_SUBFACE:
			LDWrite_Debug("POP_SUBFACE");
			if(!ReadPopSubface())
				return FLTChunkFailure("POP_SUBFACE"), nullptr;
		break;
		case PUSH_EXTENSION:
			LDWrite_Debug("PUSH_EXTENSION");
			if(!ReadPushExtension())
				return FLTChunkFailure("PUSH_EXTENSION"), nullptr;
		break;
		case POP_EXTENSION:
			LDWrite_Debug("POP_EXTENSION");
			if(!ReadPopExtension())
				return FLTChunkFailure("POP_EXTENSION"), nullptr;
		break;
		case TEXT_COMMENT:
			LDWrite_Debug("TEXT_COMMENT");
			if(!ReadTextComment())
				return FLTChunkFailure("TEXT_COMMENT"), nullptr;
		break;
		case COLOR_PALETTE:
			LDWrite_Debug("COLOR_PALETTE");
			if(!ReadColorPalette())
				return FLTChunkFailure("COLOR_PALETTE"), nullptr;
		break;
		case LONG_ID:
			LDWrite_Debug("LONG_ID");
			if(!ReadLongID())
				return FLTChunkFailure("LONG_ID"), nullptr;
		break;
		case MATRIX:
			if (!ReadMatrix())
				return FLTChunkFailure("MATRIX"), nullptr;
		break;
		case VECTOR:
			LDWrite_Debug("VECTOR");
			ReadUnknown();
		break;
		case BSP:
			LDWrite_Debug("BSP");
			ReadUnknown();
		break;
		case REPLICATE:
			LDWrite_Debug("REPLICATE");
			ReadUnknown();
		break;
		case INSTANCE_REFERENCE:
			LDWrite_Debug("INSTANCE_REFERENCE");
			ReadUnknown();
		break;
		case INSTANCE_DEFINITION:
			LDWrite_Debug("INSTANCE_DEFINITION");
			ReadUnknown();
		break;
		case EXTERNAL:
			LDWrite_Debug("EXTERNAL");
			ReadUnknown();
		break;
		case TEXTURE_REFERENCE:
			LDWrite_Debug("TEXTURE_REFERENCE");
			if(!ReadTextureReference())
				return FLTChunkFailure("TEXTURE_REFERENCE"), nullptr;
		break;
		case VERTEX_PALETTE:
			LDWrite_Debug("VERTEX_PALETTE");
			if(!ReadVertexPalette())
				return FLTChunkFailure("VERTEX_PALETTE"), nullptr;
		break;
		case VERTEX_LIST:
			LDWrite_Debug("VERTEX_LIST");
			if(!ReadVertexList())
				return FLTChunkFailure("VERTEX_LIST"), nullptr;
		break;
		case FLT_LOD:
			LDWrite_Debug("LOD");
			if(!ReadLOD())
				return FLTChunkFailure("LOD"), nullptr;
		break;
		case BOUNDING_BOX:
			LDWrite_Debug("BOUNDING_BOX");
			ReadUnknown();
		break;
		case ROTATE_EDGE:
			LDWrite_Debug("ROTATE_EDGE");
			ReadUnknown();
		break;
		case TRANSLATE:
			LDWrite_Debug("TRANSLATE");
			ReadUnknown();
		break;
		case SCALE_INDEPENDENT_XYZ:
			LDWrite_Debug("SCALE_INDEPENDENT_XYZ");
			ReadUnknown();
		break;
		case ROTATE_POINT:
			LDWrite_Debug("ROTATE_POINT");
			ReadUnknown();
		break;
		case ROTATE_SCALE:
			LDWrite_Debug("ROTATE_SCALE");
			ReadUnknown();
		break;
		case PUT:
			LDWrite_Debug("PUT");
			ReadUnknown();
		break;
		case EYEPOINT_TRACKPLANE:
			LDWrite_Debug("EYEPOINT_TRACKPLANE");
			ReadUnknown();
		break;
		case ROAD_SEGMENT:
			LDWrite_Debug("ROAD_SEGMENT");
			ReadUnknown();
		break;
		case ROAD_ZONE:
			LDWrite_Debug("ROAD_ZONE");
			ReadUnknown();
		break;
		case MORPH_VERTEX_LIST:
			LDWrite_Debug("MORPH_VERTEX_LIST");
			ReadUnknown();
		break;
		case SOUND:
			LDWrite_Debug("SOUND");
			ReadUnknown();
		break;
		case ROAD_PATH:
			LDWrite_Debug("ROAD_PATH");
			ReadUnknown();
		break;
		case SOUND_PALETTE:
			LDWrite_Debug("SOUND_PALETTE");
			ReadUnknown();
		case TEXT_RECORD:
			LDWrite_Debug("TEXT_RECORD");
			ReadUnknown();
		break;
		case SWITCH:
			LDWrite_Debug("SWITCH");
			ReadUnknown();
		break;
		case CLIP_REGION:
			LDWrite_Debug("CLIP_REGION");
			ReadUnknown();
		break;
		case EXTENSION:
			LDWrite_Debug("EXTENSION");
			if(!ReadExtension())
				return FLTChunkFailure("EXTENSION"), nullptr;
		break;
		case LIGHT_SOURCE:
			LDWrite_Debug("LIGHT_SOURCE");
			ReadUnknown();
		break;
		case LIGHT_SOURCE_PALETTE:
			LDWrite_Debug("LIGHT_SOURCE_PALETTE");
			ReadUnknown();
		break;
		case BOUNDING_SPHERE:
			LDWrite_Debug("BOUNDING_SPHERE");
			ReadUnknown();
		break;
		case BOUNDING_CYLINDER:
			LDWrite_Debug("BOUNDING_CYLINDER");
			ReadUnknown();
		break;
		case BOUNDING_VOLUME_CENTER:
			LDWrite_Debug("BOUNDING_VOLUME_CENTER");
			ReadUnknown();
		break;
		case BOUNDING_VOLUME_ORIENT:
			LDWrite_Debug("BOUNDING_VOLUME_ORIENT");
			ReadUnknown();
		break;
		case LIGHT_POINT:
			LDWrite_Debug("LIGHT_POINT");
			ReadUnknown();
		break;
		case MATERIAL_TABLE:
			LDWrite_Debug("MATERIAL_TABLE");
			if(!ReadMaterialTable(name))
				return FLTChunkFailure("MATERIAL_TABLE"), nullptr;
		break;
		case TEXTURE_MAPPING_PALETTE:
			LDWrite_Debug("TEXTURE_MAPPING_PALETTE");
			ReadUnknown();
		break;
		case MATERIAL_PALETTE:
			LDWrite_Debug("MATERIAL_PALETTE");
			if(!ReadMaterialPalette(name))
				return FLTChunkFailure("MATERIAL_PALETTE"), nullptr;
		break;
		case COLOR_NAME_PALETTE:
			LDWrite_Debug("COLOR_NAME_PALETTE");
			ReadUnknown();
		break;
		case CAT:
			LDWrite_Debug("CAT");
			ReadUnknown();
		break;
		case CAT_DATA:
			LDWrite_Debug("CAT_DATA");
			ReadUnknown();
		break;
		case CURVE:
			LDWrite_Debug("CURVE");
			ReadUnknown();
		break;
		default:
			Write_Debug("UNKNOWN: %d", s_chunk->cid);
			ReadUnknown();
		break;
		}

		if (offset != localSeek( offset))
		{
			break;
		}
	}

	// close file
	fioClose(stream);

	while (level > 0)
		ReadPopLevel();

	if (rawverts != nullptr)
	{
		memFree(rawverts);
		rawverts = nullptr;
	}

	// If this flag is still true, then something went wrong, because
	// it should have been reset to false. Check for topmost group name
	// not being equal to the file name.
	if(bTopmostObject)
	{
		FLTItemFileError("Topmost", groups[0]->name, s_conversionFile.c_str());
		bTopmostObject = false;
		return false;
	}

	assert(topmostObject);

#ifdef MAP_MATS
	changeMaterials(topmostObject);
#endif

	// ************************** ATLAS CREATION HERE **********************************

	ldmdlReduceModel(topmostObject);
	ldmdlIndexVertices(topmostObject);

#if MERGE_TREE_OBJECTS
	if(groups[0] && _hasspeedtree)
	{
		ldmdlReduceSpeedTreeObjects(groups[0]);
	}

	_hasspeedtree = false;
#endif

	ldmdlProcessBounds(topmostObject);

	if (groups[0] != nullptr)
	{
		ldmdlUpdateGroupFilteringAndBlendStatus(groups[0]);
	}

	//	tstart = GetTickCount() - tstart;
	tstart = timeGetTime() - tstart;
	Write_Debug("Finished ldfltLoad in %d ms", tstart);

	g_bSpeedTrees = false;
	memset(filename, 0, sizeof(filename));

	return topmostObject;
}

OBJECT* ldfltLoad(const std::string& name, float scale, int reduce)
{
	auto obj = _fltLoad(name.c_str(), scale, reduce);
	if (PN_LIKELY(obj != nullptr))
		return obj;

	char error[1024];
	strprintf(error, "Flt: %s\nError: %s", name.c_str(), s_loadError.c_str());
	Write_Debug("%s", error);
	piExitProgram(__FILE__, __LINE__, error);
	return nullptr;
}

