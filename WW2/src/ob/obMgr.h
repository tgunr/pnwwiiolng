#pragma once

#include "osgImport/BoundingBox.h"
#include "math/vec4f.h"
#include "osgImport/Referenced.h"
#include "osgImport/ref_ptr.h"
#include "ogl/oglMesh.h"

// these flags are based on the blitzwor comment but I don't think they are actually used anymore
// nonetheless, I've preserved them just in case
#define LOAD_FLAG_REDUCEMODEL			(1<<0)  // unused?
#define LOAD_FLAG_REALTIMEBSP			(1<<1)  // unused?
#define LOAD_FLAG_REPLACEMENTTILE	(1<<2) // replacement tile
#define LOAD_FLAG_ENTERABLE				(1<<4) // enterable building

#if USE_STATESET
class RenderStateSet;
#else
class oglMaterial;
#endif
class ModelInstance;
struct PoolModel;

namespace ModelTypes
{// Terminology might be wrong
  enum
  {
    GENERAL,         // 0
    AUTOGUNNER,      // 1
    BUILDING,        // 2
    BRIDGE,          // 3
    CAPTURE_PLACE,   // 4
    CONTROL_POINT,   // 5
    FACTORY,         // 6
    GRASS,           // 7
    SPAWN,           // 8
    TERRAIN_OBJECT,  // 9
    TERRAIN_TILE,    // 10
    VEHICLE,         // 11
    CLOUD            // 12
  };
}

namespace	ogl{class StaticBuffer;}

typedef std::vector<MATERIAL> MODELMATERIALS;
inline bool operator == (const MATERIAL &a, const MATERIAL &b) { return !memcmp(&a, &b, sizeof(MATERIAL)); }

typedef std::map<std::string, PoolModel*> PoolMap;
typedef struct
{
	PoolModel*	poolModel;
	piMATRIX	transform;
	bool		placeOntoMasterObject;
}SubModelReference;

struct PoolNode;
struct POOLOBJHDR;
struct POOLOBJDATA;
struct DOF;
class physicsNodeCollider;

// the "pool object"
struct PoolNode
{
	PoolNode* parent;
	PoolNode* sibling;
	PoolNode* child;
	PoolNode* attached;

	char name[32];
	UINT32 flags;
	UINT32 flag2;
    UINT32 flag3;

	POOLOBJDATA* objdata;
	piUNIFIED_VERTEX* vertices;
	piINDICES* indices;
	piVECTOR* faceNormals;
	piVECTOR* collisionNormals;
	DOF* dof;
	char* comment;
//	void* spare;

    int vboid;

	int texid, matid;

#if USE_STATESET
    ref_ptr<RenderStateSet> material;
#else
    ref_ptr<oglMaterial> material;
#endif

    ogl::MeshContext*	meshContext;

    physicsNodeCollider* collider;

	PoolNode();
	~PoolNode();

	void load(PoolModel*, int fd); // load file from disk
	OBJECT* createPoolInstance(ModelInstance* modelInstance, const piMATRIX& matrix); // create an instance of the model
    void InterleaveTangentData(const std::vector<Vec4f>& t,
                               const std::vector<Vec4f>& b,
                               const std::vector<Vec4f>& n,
                               PoolModel& poolModel);
#ifdef ANDREWTEST
    size_t calcMemoryUsage();
#endif
};

class oglVertexBuffer;

typedef std::set<std::string> TextureRefSet;

// the "pool model object"
struct PoolModel
{
	struct  PropertyFlags
	{
		enum
		{
			HOLLOW_REFERENCE	= 1,
			STRAT_DATA			= 1<<1,
			MARK_FOR_ICON		= 1<<2
		};
	};

	PoolModel(const char* path, unsigned int flags = 0);
	~PoolModel();

	std::string			directory;
	std::string			modelname;
	PoolNode*			node; // top level pool object for model
    oglVertexBuffer* vbo;

	UINT32				modelType;
	int						placeMode;
	int						propertyFlags;
	unsigned int		loadFlags; // flags specified in the blitzwor.csv flags column

  bool hastrees;
  bool hasRoads;

	std::vector<SubModelReference>		subModelReferences;
	std::vector<ogl::StaticBuffer*>		staticBuffers;
	std::vector<OBJECT*>				collideBranches;
	BoundingBox							bounds;					// Overall bounding box

	void load();
#ifdef SUPPORT_WWO
	bool		LoadWWO(std::string path);
#endif
	void		LoadMDL(const char* path);
	void		RebuildMDL();
	void		SaveMDL(const POOLOBJHDR* poolobjhdr, const MODELMATERIALS* modelmaterials);
    void        CollectModelTextures(OBJECT* o, TextureRefSet& refset );
	void		CollectModelMaterials(OBJECT* o, MODELMATERIALS* modelmaterials);
	void		CollectModelReferences(OBJECT* o);
	void		CollectCollideBranches(OBJECT* o);
	LOGICAL		ModelNeedsRebuild(const char* path);
	void		ProcessVehicleCollisionData(OBJECT* o);
	void		ProcessTerrainObjectCollisionData(OBJECT* o);
	void		CompileModelBounds(OBJECT* o);
	void		SetModelType();
	void		SetProperties(const OBJECT* o);
	void		CreateContexts(OBJECT* o, LOGICAL isStaticMeshBranch);
	ModelInstance*	CreateInstance(const piMATRIX& transform);
	LOGICAL		IsStaticTerrainModel()	const;
	LOGICAL IsReplacementTile() const;
	LOGICAL IsEnterable() const;
	void		CheckModelSanity()	const;
	bool		GetSubObjectBoundsByName(BoundingBox* bounds_, const char* name, const PoolNode* o) const;
	bool		GetSubObjectBoundsByComment(BoundingBox* bounds_, const char* comment, const PoolNode* o) const;
	void		GetMinLODStats(int& minNumVertices, int& minNumObjects, float& maxlodRange) const;	// Only for debugging in BuildTerrain
};


class ModelInstance
{
	private:

		PoolModel*							poolModel;
		std::vector<ogl::StaticBuffer*>		staticBuffers;
		BoundingBox							bounds;

	public:

		OBJECT*								rootObject;
		piMATRIX							transform;

		ModelInstance(PoolModel* pM, const piMATRIX& m);
		~ModelInstance();

		PoolModel*				GetModel() const { return poolModel; }
		const char*			GetModelName() const;
		const char*			GetModelDirectory() const;
		UINT32				GetModelType() const	{return poolModel->modelType;}
		int					GetPlaceMode() const	{return poolModel->placeMode;}
  int getPropertyFlags() const	{return poolModel->propertyFlags;}

        bool                IsShearPlacement() const;

		LOGICAL				IsStaticTerrainModel()		const;
		const BoundingBox&	GetBounds() const {return bounds;}
};

// For PoolNode or OBJECT
template<class T>
struct FindByName
{
    FindByName()
    {
        name = nullptr;
        node = nullptr;
    }

    void set(const char* n)
    {
        name = n;
        node = nullptr;
    }

    void find(const T* o)
    {
        if(o && !node)
        {
            if(!strcasecmp(o->name, name))
            {
                node = o;
            }

            find(o->child);
            find(o->sibling);
        }

        // Skip attached since named the same
    }

    const char* name;
    const T* node;
};

// For PoolNode or OBJECT
template<class T>
struct FindByComment
{
    FindByComment()
    {
        comment = nullptr;
        node = nullptr;
    }

    void set(const char* c)
    {
        node = nullptr;
        comment = c;
    }

    void find(const T* o)
    {
        if(o && !node)
        {
            if(o->comment)
            {
                // FIXME: to find exact word...
                if(strstr(o->comment, comment))
                {
                    node = o;
                }
            }

            find(o->child);
            find(o->sibling);
        }
        // Skip attached since comment the same
    }

    const char* comment;
    const T* node;
};

extern PoolModel* obmgrAddModelToPool(const char* path, LOGICAL forceModelLoading, UINT32 flags = 0);		/// Registers the model with the pool, loads are deferred until an instance of the model is created.
extern PoolModel* obmgrGetPoolModel(const char* path);
extern struct	OBJECT*	obmgrCreateModelInstance(const char* modelname);					/// Creates an instance of the named model.
extern struct	OBJECT*	obmgrCreateModelInstance(	const char* modelName,
											const piMATRIX& transform,
											bool handleSpecialObjects	);			/// Creates an instance of the named model.
extern void			obmgrHandleSpecialObjects(OBJECT* o);

extern const PoolMap* 	obMgrGetPoolMap();
extern void obmgrKill();
