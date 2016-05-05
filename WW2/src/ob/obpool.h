#pragma once

#include "pi/pi.h"

/************************************************************************************************
	obpool
*************************************************************************************************/
// I can't believe that this wasn't in here already
#pragma pack(push)
#pragma pack(1)

#define MINPOOLOBJS 110000

#define QUEUE_TEXTURE_LOADING
#undef QUEUE_TEXTURE_LOADING	// Disable to avoid heap fragmentation

struct POOLOBJHDR
{
	POOLOBJHDR*	prev;			// Previous object in list
	POOLOBJHDR*	next;			// Next object in list
	POOLOBJHDR*	attached;		// Attached object
	POOLOBJHDR*	sibling;		// Sibling object
	POOLOBJHDR*	child;			// Child object
	struct POOLOBJ*		pool;			// Pool object pointer

// Runtime pool data
	SINT32	texid;
	SINT32	matid;
	UINT32	gfxmodes;

};


/************************************************************************************************
	MDL format / Pool Structure
*************************************************************************************************/

struct OBJECTFILE
{
	SINT16	company;		// Company initials
	SINT16	filetype;		// File type (object, etc.)
	SINT16	version;		// Major/Minor version information
	UINT32	length;			// Length of data; including this header
};


struct POOLOBJ
{
	SINT32				size;			// Full size of this object
	char				name[32];		// Object name
	UINT32				flags;			// Object flags 1
	UINT32				flag2;			// Object flags 2
    UINT32              flag3;          // Object flags 3
	SINT32				attached;		// Offset to attached object
	SINT32				sibling;		// Offset to sibling object
	SINT32				child;			// Offset to child object
	SINT32				objdata;		// Offset to start of Object data
	SINT32				dof;			// Offset to degree of freedom bead
	SINT32				comment;		// Offset text comment
	SINT32				spare;			// Offset Spare data

};

struct POOLOBJDATA
{
	SINT16				feature;		// Object feature number
	SINT16				layer;			// Sorting layer
	UINT16				level;			// Tesselation level
	UINT16				reserved;		// pad for 4 byte boundary
	piVECTOR			loc;			// Agrigate object center position
	piVECTOR			pos;			// Object center position
	char				matname[32];	// Material name
	char				texname[32];	// Texture name
	FLOAT32				minrad;			// Minimum object radius
	FLOAT32				maxrad;			// Maximum object radius
	FLOAT32				minlod;			// Minimum LOD distance
	FLOAT32				maxlod;			// Maximum LOD distance
	piVECTOR			obox[2];		// Object bounding box
	piVECTOR			bbox[2];		// Bounding box

	UINT16				numverts;		// Number of object vertices
	UINT16				numtris;		// Number of object indices

	SINT32				puverts;		// Offset to object vertices
	SINT32				tangentData;
	SINT32				ptris;			// Offset to object indices
	SINT32				psnorms;		// Offset to surface normals
	SINT32				pscdn;			// Offset to surface collision detection normals
};

struct DOF
{
	piVECTOR			doforigin;		// DOF Origin
	piVECTOR			dofaxis;		// DOF Axis
	piVECTOR			dofplane;		// DOF Plane
	piVECTOR			dofmintrans;	// DOF Minimum Translate
	piVECTOR			dofmaxtrans;	// DOF Maximum Translate
	piVECTOR			dofcurtrans;	// DOF Current Translate
	piVECTOR			dofinctrans;	// DOF Translation Increment
	piVECTOR			dofminrotate;	// DOF Minimum Rotate
	piVECTOR			dofmaxrotate;	// DOF Maximum Rotate
	piVECTOR			dofcurrotate;	// DOF Current Rotate
	piVECTOR			dofincrotate;	// DOF Rotation Increment
	piVECTOR			dofminscale;	// DOF Minimum Scale
	piVECTOR			dofmaxscale;	// DOF Maximum Scale
	piVECTOR			dofcurscale;	// DOF Current Scale
	piVECTOR			dofincscale;	// DOF Scale Increment
	FLOAT32				dofvarvalue;	// DOF Variable -1.0 = minpos, 0.0 = neutral, 1.0 = max
	UINT16				dofvarnum;		// DOF Variable number
	UINT16				dofvarsubnum;	// DOF Variable subnumber
	FLOAT32				dofmaxval;		// DOF input variable max value
	FLOAT32				dofneutralval;  // DOF input variable neutral value
	FLOAT32				dofminval;		// DOF input variable min value
	piMATRIX			dofmatrix1;		// DOF processing matrix 1
	piMATRIX			dofmatrix2;		// DOF processing matrix 2
	piMATRIX			dofposmatrix;	// DOF position matrix
	UINT32				dofflags;		// DOF Flags

};

#pragma pack(pop)

extern LOGICAL	obpoolInit(const SINT8* name);
extern void 	obpoolKill(void);
extern void obpoolUpdate();
extern void obpoolFreeObjectFinish();

extern LOGICAL	obpoolSavePoolCallback(POOLOBJHDR* lpgrp, int hfile);
extern POOLOBJHDR* obpoolObjectToPool(OBJECT* topmost, FLOAT32 localscale, SINT32 reduce);
extern POOLOBJHDR* obpoolTest(OBJECT* topmost, FLOAT32 localscale, SINT32 reduce);
extern LOGICAL	obpoolFreeObjectCallback(OBJECT* lpgrp);
extern LOGICAL	obpoolPoolOffsetsCallback(POOLOBJHDR* lpgrp, POOLOBJHDR* lpparent, UINT32* pooloff);
extern SINT32	obpoolGetObjects(POOLOBJ* *lpobj);
extern LOGICAL	obpoolGetObjectSize(int* objsize, OBJECT* lpobj);

extern POOLOBJHDR* obpoolGetNewObject(void);
extern POOLOBJHDR* obpoolFindNamedObject(SINT8* name);
extern POOLOBJHDR* obpoolCopyObject(POOLOBJ* lpcopy);
extern POOLOBJHDR* obpoolCopyNamedObject(SINT8* name);
extern LOGICAL		 obpoolFreeObject(POOLOBJHDR* lphdr);

#if defined(OPENGL3D) && platOS == OS_WIN32
extern void	obpoolColorSwap(POOLOBJ* lppool);
#endif
