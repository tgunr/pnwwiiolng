#pragma once

#include "pi/pi.h"
struct LISTHEADER;

#define MINOBJECTS				96000

// flags
#define OBJECT_VISIBLE          0x00000001
#define OBJECT_LITE             0x00000002
#define OBJECT_DYNALITE         0x00000004
#define OBJECT_COLLISION        0x00000008
#define OBJECT_CLIPPED          0x00000010
#define OBJECT_INDEXED          0x00000020
#define OBJECT_LINESTRIP        0x00000040
#define OBJECT_LINECLOSED       0x00000080
#define OBJECT_TRIANGLEFAN      0x00000100
#define OBJECT_TRIANGLELIST     0x00000200
#define OBJECT_NOBACKFACE       0x00000400
#define OBJECT_USELOD           0x00000800
#define OBJECT_ATTACHED         0x00001000
#define OBJECT_SKIPDRAWING      0x00002000
#define OBJECT_USEVERTEXBSP     0x00004000
#define OBJECT_RADIUSCOLLISION  0x00008000
#define OBJECT_USEMATRIX        0x00010000
#define OBJECT_REFLECTIVE       0x00020000
#define OBJECT_NOFILTER         0x00040000
#define OBJECT_REVERSENORMALS   0x00080000
#define OBJECT_TESSELATION		0x00100000
#define OBJECT_DYNASHADOW		0x00200000
#define OBJECT_DYNAENVMAP		0x00400000
#define OBJECT_DYNAFOG		    0x00800000
#define OBJECT_COPY		        0x01000000
#define OBJECT_ZWRITEOFF        0x02000000
#define OBJECT_NOSPECULAR		0x04000000
#define OBJECT_TERRAIN			0x08000000
#define OBJECT_USERDRAW			0x10000000
#define OBJECT_DOF				0x20000000
#define OBJECT_LOD				0x40000000
#define OBJECT_GROUP			0x80000000
#define OBJECT_NOTOBJECT		(OBJECT_DOF|OBJECT_LOD|OBJECT_GROUP)

// flag2
#define OBJECT_USEBUFFERS				0x00000001	// use object buffers (vertex and/or index)
#define OBJECT_POOL						0x00000002	// origin is from object pool
#define OBJECT_TOPMOST					0x00000004	// topmost object in object tree
#define OBJECT_NORMALMAP                0x00000008	// Has normal map applied
#define OBJECT_ZDISABLE					0x00000010	// z buffer disable
#define OBJECT_ISCOCKPIT				0x00000020  // uses cockpit z-buffer region
#define OBJECT_USESDECAL				0x00000040  // uses texture in obj->shadow object
													//  as a decal* NOTE*! if this object
													//  is deleted in the meantime things
													//  *WILL* blow up! use with care!
#define OBJECT_USERVEHICLE				0x00000080  // indicates an object is part of a user
													// controlled vehicle
#define OBJECT_NOFOG					0x00000100	// no fog for this object
#define OBJECT_HASALPHA					0x00000200
#define OBJECT_ANIMATION				0x00000400	// declares an object to have secondary
													// object management in the animation
													// modules
#define OBJECT_ALTEREDTEXTURE			0x00000800	// indicates that the texture has been scribbled
#define OBJECT_BLENDADDITIVE			0x00001000  // object uses additive blending
#define OBJECT_DOF_USECHILDMATRIX		0x00002000	// when doing dof matrices, set dof matrix to child's matrix
#define OBJECT_HIDDEN					0x00004000	// object is hidden and will NEVER be displayed!
#define OBJECT_TERRAIN_OBJECT			0x00008000	// object is placed on the terrain
#define OBJECT_SPECIAL_LOD				0x00010000	// lod is a special lod
#define OBJECT_STRAT_OBJECT				0x00020000	// object is a strategic object
#define OBJECT_DONT_UPDATE				0x00040000	// don't update the matrices for this object unless specified
#define OBJECT_HAS_FX                   0x00080000	// object has FX
#define OBJECT_IS_PARTOF_VEHICLE		0x00100000	// object is part of a vehicle
#define OBJECT_ISFPS					0x00200000	// FPS
#define OBJECT_SURFACENORMALCOPY		0x00400000	// object uses its own copy of surface normals
#define OBJECT_COLLISIONENORMALCOPY		0x00800000	// object uses its own copy of collision normals
#define OBJECT_ISTPS					0x01000000	// third person trooper object
#define OBJECT_GRANNY					0x02000000	// granny object (generic, includes FPS and TPS, here for future expansion)
#define OBJECT_MISSING					0x04000000	// object is not longer part of vehicle
													// (add mainly to stop DOF updating and remove hitmaps in visdamage objects)
#define OBJECT_SPEEDTREE                0x08000000  // Object contains a speedtree
#define OBJECT_FLAG                     0x10000000  // Object needs a shader
#define OBJECT_AIRRAIDSIREN             0x20000000  // Object is a siren object
#define OBJECT_IS_ROTATED               0x40000000  // Terrain object rotated
#define OBJECT_IS_GRASS                 0x80000000  // Object is grass object

// flag3 HERE
#define OBJECT_IS_INTERIOR              0x00000001 // Object is on interior material
#define OBJECT_CAST_SHADOW              0x00000002 // Object is shadow caster
#define OBJECT_TREE_CLUSTER             0x00000004 // Parent to SpeedTree cluster
#define OBJECT_HAS_POPUP                0x00000008 // Object has popup message
#define OBJECT_PLAYER_PLACED            0x00000010 // Object is PPO
#define OBJECT_SPAWN                    0x00000020 // Object is a spawn object
#define OBJECT_SPECULARMAP              0x00000040 // Object is a specular map
#define OBJECT_COAST_WATER              0x00000080 // Object is coast
#define OBJECT_RIVER_WATER              0x00000100 // Object is river
#define OBJECT_CLOUDS                   0x00000200 // Object is cloud
#define OBJECT_RAINCLOUDS               0x00000400 // Object is rain cloud
#define OBJECT_IS_GR2VEHICLE            0x00000800 // this object is an animated granny vehicle
#define OBJECT_CAPTURABLE            0x00001000 // capture areas are marked with this
#define OBJECT_NOFIRE					0x00002000 // this object represents an area where you can't fire
#define OBJECT_PHYSICAL					0x00004000 // this object is under physics control, don't update
#define OBJECT_STO						0x00008000 // this object represents an STO (server tracked object)
#define OBJECT_WALK						0x00010000 // this object is part of the collide walk object tree
#define OBJECT_HANDLES_COUNTRY			0x00020000 // this object handles its country ownership externally from the strat system
#define OBJECT_IS_ROAD                  0x00040000 // this object handles its country ownership externally from the strat system


// gfxmodes
#define GRAPHICS_TAM_MASK				0x0000000f	// texture address modes mask

// Implement the mode in both u and v directions
#define GRAPHICS_TAM_TUV_WRAP			0x00000000	// default
#define GRAPHICS_TAM_TUV_CLAMP			0x00000001
#define GRAPHICS_TAM_TUV_MIRROR			0x00000002

// Implement the mode in u direction (left to right)
#define GRAPHICS_TAM_TU_WRAP			0x00000003
#define GRAPHICS_TAM_TU_CLAMP			0x00000004
#define GRAPHICS_TAM_TU_MIRROR			0x00000005

// Implement the mode in v direction (up and down)
#define GRAPHICS_TAM_TV_WRAP			0x00000006
#define GRAPHICS_TAM_TV_CLAMP			0x00000007
#define GRAPHICS_TAM_TV_MIRROR			0x00000008

#define GRAPHICS_NO_MIPMAP				0x00000010

#define DOF_IGNORECENTER				0x00000001	// ignore center position
#define	DOF_ROTATES_X					0x00000002	// dof rotates on x axis
#define DOF_ROTATES_Y					0x00000004	// dof rotates on y axis
#define DOF_ROTATES_Z					0x00000008	// dof rotates on z axis
#define	DOF_TRANSLATES_X				0x00000010	// dof translates on x axis
#define DOF_TRANSLATES_Y				0x00000020	// dof translates on y axis
#define DOF_TRANSLATES_Z				0x00000040	// dof translates on z axis
#define DOF_USE_DOFPOSMATRIX			0x00000080	// use DOFPOS matrix
#define DOF_ONE_ROTATE_AXIS				0x00000100	// at least one rotate axis
#define DOF_ONE_TRANS_AXIS				0x00000200	// at least one tranlation axis
#define DOF_FREE_MEMORY					0x00000400  // new HACK to stop leak

typedef LOGICAL (*DeprecatedDrawCallback)(struct OBJECT* lpobj);
typedef LOGICAL (*PROCESSCALLBACK)(struct OBJECT* lplod, UINT32 user);

typedef bool (*CullCallback)(struct OBJECT* o);
typedef bool (*UpdateCallback)(struct OBJECT* o);
typedef bool (*DrawCallback)(struct OBJECT* o);
typedef bool (*KillCallback)(struct OBJECT* o);

#ifdef __cplusplus

#include "ob/stride_ptr.h"

// "smart" strided pointers
typedef stride_ptr<piVECTOR> 				piVectorPtr;
typedef stride_ptr<piVECTOR const> 			piVectorConstPtr;
typedef stride_ptr<piVERTEX_COLOR> 			piVertexColorPtr;
typedef stride_ptr<piVERTEX_COLOR const> 	piVertexColorConstPtr;
typedef stride_ptr<piUV> 					piTexCoordPtr;
typedef stride_ptr<piUV const> 				piTexCoordConstPtr;

struct piStridedVertexConstPtr
{
	piVectorConstPtr 		pos;
	piVectorConstPtr 		norm;
	piVertexColorConstPtr 	color;
	piTexCoordConstPtr 		tex;

	inline piStridedVertexConstPtr& operator++() 		{ ++pos; ++norm; ++color; ++tex; return *this; }
	inline piStridedVertexConstPtr& operator--() 		{ pos++; norm++; color++; tex++; return *this; }

	inline piStridedVertexConstPtr& operator+=(int n)	{ pos+=n; norm+=n; color+=n; tex+=n; return *this; }
	inline piStridedVertexConstPtr& operator-=(int n)	{ pos-=n; norm-=n; color-=n; tex-=n; return *this; }

	inline piStridedVertexConstPtr  operator++(int) 	{ piStridedVertexConstPtr tmp = *this; ++(*this); return tmp; }
	inline piStridedVertexConstPtr  operator--(int) 	{ piStridedVertexConstPtr tmp = *this; ++(*this); return tmp; }
};

#endif // __cplusplus

// forward declare to avoid header clutter
class physicsModel;
class physicsObject;
struct gr2AnimTrooper;
struct PoolNode;

struct OBJECTSTATICDOF
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

};

struct OBJECTDOF
{
	OBJECTSTATICDOF*	dofstatic;		// DOF Static Elements
	float				dofvarvalue;	// DOF Variable -1.0 = minpos, 0.0 = neutral, 1.0 = max
	UINT16				dofvarnum;		// DOF Variable number
	UINT16				dofvarsubnum;	// DOF Variable subnumber
	float				dofmaxval;		// DOF input variable max value
	float				dofneutralval;  // DOF input variable neutral value
	float				dofminval;		// DOF input variable min value
	piMATRIX			dofmatrix1;		// DOF processing matrix 1
	piMATRIX			dofmatrix2;		// DOF processing matrix 2
	piMATRIX			dofposmatrix;	// DOF position matrix
	UINT32				dofflags;		// DOF Flags

};

class physicsObjectCollider;
struct PoolModel;
class ModelInstance;

#if USE_STATESET
class RenderStateSet;
#else
class oglMaterial;
#endif

namespace ogl	{class MeshContext;}

struct OBJECT
{
	typedef	char	Name[64];

	struct OBJECT*	prev;			// Previous object in list
	struct OBJECT*	next;			// Next object in list
	struct OBJECT*	attached;		// Attached object
	struct OBJECT*	sibling;		// Sibling object
	struct OBJECT*	child;			// Child object

	Name				name;		// Object name

	char*				comment;		// Text comment
	UINT32				flags;			// Object flags 1
	UINT32				flag2;			// Object flags 2
    UINT32              flag3;          // Object flags 3
    UINT32              treeID = 0;     // ID of speedtree object
	float               createtime;     // When an FX was created.
	float               updatetime;     // When an FX was last updated.
	void*				lpspare;		// Object spare pointer

	piMATRIX			mat;			// Rotation/transformation matrix
	piMATRIX			matint;			// Internal usage matrix

	OBJECTDOF*			dof;			// DOF

	short				feature;		// Object feature number
	short				layer;			// Sorting layer
	UINT16				level;			// Tesselation level
	piVECTOR			loc;			// Agrigate object center position
	piVECTOR			pos;			// Object center position
	piVECTOR			att;			// Object attitude
	piVECTOR			rot;			// Angular rotation

	float				minrad;			// Minimum object radius
	float				maxrad;			// Maximum object radius
	float				minlod;			// Minimum LOD distance
	float				maxlod;			// Maximum LOD distance

	struct OBJECT*	shadow;		// Shadow object, also a generic object pointer

    CullCallback cullcallback;
    UpdateCallback updatecallback;
    DrawCallback drawcallback;
    KillCallback killcallback;

	piVECTOR			obox[2];		// Object bounding box
	piVECTOR			bbox[2];		// Bounding box`
	float				distsqr;		// Distance to object (squared)
	char				trnum;			// Terrain rectangle number

	UINT16				numverts;		// Number of object vertices

	UINT16				numtris;		// Number of object indices
	piINDICES*			ptris;			// Object indices

	UINT16				numtverts;		// Number of lite tesselated vertices   REMOVE
	piCVERTEX*			ptverts;		// Lite tesselated vertices             REMOVE

	piVECTOR*			psnorig;		// Original surface normals
	piVECTOR*			psnorms;		// Surface normals (may point to psnorig)
	piVECTOR*			pscdon;			// Original surface collision detection normals
	piVECTOR*			pscdn;			// Surface collision detection normals (may point to pscdon)

	piVECTOR			vel;			// Movement speed

	const char* matname;		// Material name
	const char* texname;		// Texture name
	short matid;			// Material id
	short texid;			// Texture id

    bool freepsnorms;
    bool freepscdn;

    float reflectiondist;
    float shadowdist;

	// TODO: Currently doubles up as a TerrainTile pointer...
    SINT32              soundID;        // Sound associated with this object.

/*
	These are the new functions for accessing object vertex data. These functions should
	used instead of directly accessing the vertex fields of the OBJECT structure. Only a
	C++ interface is currently provided.

	These functions return a separate stride_ptr "smart" pointer for the vertex position (x,y,z),
	vertex normal (nx,ny,nz), vertex color (color, specular), and texture co-ordinates (u,v) respectively.

	The categories of accessor functions are as follows:

	Immutable - This is the original vertex data from the pool object. Since it should never be modified
	only const references to its data is returned.

	Mutable - This is a copy of the pool data which the application can modify. This data is created
	"lazily" as needed.

	Render - This will be the primary function used in most cases (i.e. ones in which you don't need
	to modify the data and only need the "current" data). Returns the mutable data if it exists,
	otherwise the original immutable data.

	Static - This is a special case for objects that were not created from the pool, which therefore
	can have modifiable "original" vertex data. This function will return nullptr if the object
	was created from the pool.
*/
#ifdef __cplusplus

    piVectorConstPtr		RenderPositions() const;
	piVectorConstPtr		RenderNormals() const;
	piVertexColorConstPtr	RenderColors() const;
	piTexCoordConstPtr		RenderTexCoords() const;

	piVectorConstPtr		ImmutablePositions() const;
	piVectorConstPtr		ImmutableNormals() const;
	piVertexColorConstPtr	ImmutableColors() const;
	piTexCoordConstPtr		ImmutableTexCoords() const;

	piVectorPtr				MutablePositions();
	piVectorPtr				MutableNormals();
	piVertexColorPtr		MutableColors();
	piTexCoordPtr			MutableTexCoords();

	piVectorPtr				StaticPositions();
	piVectorPtr				StaticNormals();
	piVertexColorPtr		StaticColors();
	piTexCoordPtr			StaticTexCoords();

    const piUNIFIED_VERTEX* RenderData();

	// Strided Data for d3dpRenderIndexStridedScene.
	void StridedRenderData(piSTRIDEDDATA*) const;

	// Strided Ptr for all vertex data.
	// Mainly to make vertex reduction code in trnmain a little cleaner.
	void StridedRenderVertices(piStridedVertexConstPtr*) const;

	// Copies original data to mutable data if it exists.
	void SyncMutableData();

	// Creates static vertex data.
	void CreateStaticData();

	// NOP now.
	void SyncStaticData();

	void PopulateIndexes(UINT16 index)
	{
		for (auto it = ptris + index, end = ptris + numtris; it != end; ++it)
		{
			// Funky useless indices
			it->v1 = index++;
			it->v2 = index++;
			it->v3 = index++;
		}
	}

#endif // __cplusplus

	// treat these are private to obmain/obpool (although there is some outside code that does
	// access these fields, this practice should be kept to a minimum)
	piUNIFIED_VERTEX*	puverts;
	piUNIFIED_VERTEX*	pverts;

	// trooper mesh and model data
	gr2AnimTrooper* animModel;

	physicsObjectCollider*	collider;
	ModelInstance*			modelInstance;
	PoolNode*					modelNode; // pointer to the originating pool node (for collider purposes)
	ogl::MeshContext*		meshContext;

#if USE_STATESET
    RenderStateSet* material;
#else
    oglMaterial* material;
#endif

	physicsObject*			physicalModel;
#ifdef SPATIAL_DB
    void*           sdbInfo;
#endif
};

typedef OBJECT DOFOBJECT;
typedef OBJECT LODOBJECT;
typedef OBJECT GROUPOBJECT;

extern struct OBJECT* obmainGetNewObject();
extern struct OBJECT* obmainGetNewNotObject(UINT32 obtype);
extern struct OBJECT* obmainGetFreeObject();
extern LOGICAL	 obmainFreeObject(struct OBJECT* lpobj);

extern LOGICAL		obmainMoveToNotObjectTree(struct OBJECT* lpobj);
extern LOGICAL		obmainMoveToObjectTree(struct OBJECT* lpobj);
extern LOGICAL		obmainAttachObject(struct OBJECT* lpmain, struct OBJECT* lpobj);
extern LOGICAL		obmainAddChildObject(struct OBJECT* lpmain, struct OBJECT* lpobj);
extern LOGICAL		obmainSetObject(struct OBJECT* lpnew, struct OBJECT* lpold);

// Flags
extern LOGICAL		obmainSetFlagsInObjectTree(struct OBJECT* lpmain, UINT32 flags);
extern LOGICAL		obmainClearFlagsInObjectTree(struct OBJECT* lpmain, UINT32 flags);
extern LOGICAL		obmainSetFlagsInThisTree(struct OBJECT* lpmain, UINT32 flags);
extern LOGICAL		obmainClearFlagsInThisTree(struct OBJECT* lpmain, UINT32 flags);

// Flag 2
extern LOGICAL		obmainSetFlag2InObjectTree(struct OBJECT* lpmain, UINT32 flag2);
extern LOGICAL		obmainClearFlag2InObjectTree(struct OBJECT* lpmain, UINT32 flag2);
extern LOGICAL		obmainSetFlag2InThisTree(struct OBJECT* lpmain, UINT32 flag2);
extern LOGICAL		obmainClearFlag2InThisTree(struct OBJECT* lpmain, UINT32 flag2);

// Flag 3
extern LOGICAL		obmainSetFlag3InObjectTree(struct OBJECT* o, UINT32 flag3);
extern LOGICAL		obmainClearFlag3InObjectTree(struct OBJECT* o, UINT32 flag3);
extern LOGICAL		obmainSetFlag3InThisTree(struct OBJECT* o, UINT32 flag3);
extern LOGICAL		obmainClearFlag3InThisTree(struct OBJECT* o, UINT32 flag3);

extern struct OBJECT*	obmainFindNamedObject(const char* name);
extern struct OBJECT*	obmainFindGroupObject(const char* name);
extern struct OBJECT*	obmainFindChildObject(const char* group, const char* name);
extern struct OBJECT*	obmainFindChildFromObject(struct OBJECT* lpgrp, const char* name);
extern struct OBJECT*	obmainGetFirstTopmostObject(void);
extern struct OBJECT*	obmainGetNextTopmostObject(struct OBJECT* lpobj);
extern struct OBJECT*	obmainFindTopmostObject(const char* name);
extern struct OBJECT*	obmainGetFirstSubobject(struct OBJECT* lpmain);
extern struct OBJECT*	obmainGetNextSubobject(struct OBJECT* lpobj);
extern LOGICAL		obmainFindCrackedComment(SINT8** retptr, SINT8* comment, const SINT8* cmp);
extern LOGICAL		obmainFindCrackedCommentFirstPart(SINT8** retptr, SINT8* comment, const SINT8* cmp);
extern LOGICAL		obmainFindCommentName(SINT8** retptr, SINT8* comment, const SINT8* cmp);
extern LOGICAL 	obmainFindCommentFirstPartName(SINT8** retptr, SINT8* comment, const SINT8* cmp);
extern LOGICAL		obmainFindComment(struct OBJECT** retobj, OBJECT* lpmain, const char* comment);
extern LOGICAL		obmainFindCommentCallback(struct OBJECT* lpmain, char* comment, UINT32 user, PROCESSCALLBACK callback);
extern LOGICAL		obmainFindCommentFirstPartCallback(struct OBJECT* lpmain, const char* comment, UINT32 user, PROCESSCALLBACK callback);
extern LOGICAL		obmainFindObjectLOD(struct OBJECT* lpmain, int lod, PROCESSCALLBACK callback);
extern LOGICAL		obmainFindObjectDamage(struct OBJECT* lpmain, int dmg, PROCESSCALLBACK callback);
extern void			obmainFindCallback(struct OBJECT* lpmain, void(*callback)(OBJECT&, void*), void* userdata);
extern struct OBJECT*	obmainCopyObjectFull(struct OBJECT* lpcopy);
extern struct OBJECT*	obmainCopyObject(struct OBJECT* lpcopy);
extern struct OBJECT*	obmainCopyNamedObject(char* name);
extern int			obmainGetObjects(LISTHEADER** lplist);
extern LOGICAL		obmainCreateObject(struct OBJECT** newobj, const char* name, int numverts, int numtris);
extern LOGICAL		obmainCreateObjectFull(struct OBJECT** newobj, const char* name, int numverts, int numtris);
extern LOGICAL		obmainSnapObjectToXYGrid(struct OBJECT* Object, FLOAT32 GridSize, UINT32 UseObjectMatrix);
extern LOGICAL		obmainSnapObjectTreeToXYGrid(struct OBJECT* Object,FLOAT32 GridSize,UINT32 UseObjectMatrix);

extern LOGICAL		obmainInit(void);
extern void		obmainKill(void);
extern LOGICAL		obmainUpdate(void);

extern LOGICAL obmainGetBoundingBoxMax(FLOAT32* pfLeft,
								FLOAT32* pfTop,
							    FLOAT32* pfRight,
								FLOAT32* pfBottom,
								FLOAT32* pfMaxZ,
								FLOAT32* pfMinZ,
								OBJECT* pObject);

extern LOGICAL obmainGetBoundingBoxMaxSiblings(FLOAT32* pfLeft,
									    FLOAT32* pfTop,
									    FLOAT32* pfRight,
									    FLOAT32* pfBottom,
								        FLOAT32* pfMaxZ,
										FLOAT32* pfMinZ,
									    OBJECT* pObject);

extern void	obmainDeleteNode(OBJECT* node);
extern void	obmainDeleteNodeTree(OBJECT* node);
extern UINT32 obmainRemoveObjectTree(OBJECT* pObject);
extern UINT32 obmainRemoveSiblings(OBJECT* pObject);
extern LOGICAL obmainSetObjectTreeMatrix(OBJECT * Obj,piMATRIX * Mat);

extern void obmainCreateMutableSurfaceNormals(OBJECT*);
extern void obmainCreateMutableCollisionNormals(OBJECT*);

extern bool obmainValidateObject(const OBJECT*); // last resort to track down garbage objects

// get a screen oriented bounding box for the object
extern LOGICAL obmainGetScreenBBOX(OBJECT* lpob, float& x, float& y, float &z, float& w, float& h);
extern void obmainAddForUpdateCallback(OBJECT* o);

extern LOGICAL listDeleteOBJECT(OBJECT* obj); // special version of listDeleteItem <--- use this one instead of casting
