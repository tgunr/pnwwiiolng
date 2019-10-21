#pragma once

#include "granny.h"
#include <stack>
#include "include/systypes.h"
//#include "gr2/gr2layer.h"
#include "math/Vec3f.h"
#include "mem/mem.h"

#if USE_STATESET
#include "ogl/States.h"
#endif

struct gr2AnimTrooper;

template<class T>
class GR2_POOL
{
public:

	// ifdef this by OS or something?
	static const UINT32 OS_PAGE_SIZE = 4096;

	enum STRATEGY
	{
		NEW,
		ALLOC
	};

	enum HOOK
	{
		CREATE,
		POP,
		PUSH,
		FREE
	};

	// you can assign a callback hook to process a poolObject
	// at certain pool events as defined in the above enum HOOK
	// this can be used to create or delete additional resources
	using INSTANCE_HOOK = void(*)(HOOK, T*);

	GR2_POOL(UINT32 is = 32, UINT32 bs = 32, STRATEGY s = ALLOC, INSTANCE_HOOK h = nullptr) :
        _maxout(0),
		_inout(0),
        _objectCount(0),
        _blockSize(bs),
        _strategy(s),
        _hook(h)
	{
		if(is)
			Allocate(is); // is = initialSize
	}

	virtual ~GR2_POOL()
	{
		Free();
	}

    GR2_POOL<T>( const GR2_POOL<T> &obj ) {
        // dummy, we don't need a copy instructor
    }

	T* Pop(bool reset = true)
	{
		if(_pool.empty())
			Allocate(_blockSize);

		T* poolObject = _pool.top();

		if(poolObject)
		{
			_pool.pop();

			if(reset && _strategy == ALLOC)
				memset((void *)poolObject, 0, sizeof(T));

			if(_hook)
				_hook(POP, poolObject);

			++_inout;

			if(_inout > _maxout)
				_maxout = _inout;
		}

		return poolObject;
	}

	bool Push(T* poolObject)
	{
		bool pushed = false;

		if(poolObject && !_blocks.empty())
		{
			if(_hook)
				_hook(PUSH, poolObject);

			_pool.push(poolObject);

			--_inout;
			pushed = true;
		}

		return pushed;
	}

	void Allocate(UINT32 num)
	{
		// block pointer
		T* block = nullptr;

		// size in bytes of our object
		size_t size = sizeof(T);

		// total bytes we need
		size_t total = num * size;

		// room we have left
		size_t waste = (size_t)(total % OS_PAGE_SIZE);

		// can we use the extra room?
		if(waste >= size)
			num += (UINT32)(waste / size);

		if(_strategy == NEW)
		{
			block	= new T[num];
		}
		else
		{
			block	= (T*)memCalloc(num, size);
		}

		if(block)
		{
			for(unsigned int i = 0; i < num; ++i)
			{
				T* poolObject = &block[i];

				if(_hook)
					_hook(CREATE, poolObject);

				_pool.push(poolObject);

				// lets keep a count of the number of objects for debugging
				++_objectCount;
			}

			_blocks.push_back(block);
		}
	}

	void Free()
	{
	    while(!_pool.empty())
		{
			if(_hook)
			{
				T* poolObject = _pool.top();
				_hook(FREE, poolObject);
			}

			_pool.pop();
		}

		if(_strategy == NEW)
		{
			for(unsigned int i = 0; i < _blocks.size(); ++i)
			{
				delete [] _blocks[i];
				_blocks[i] = nullptr;
			}
		}
		else
		{
			for(unsigned int i = 0; i < _blocks.size(); ++i)
			{
				memFree(_blocks[i]);
				_blocks[i] = nullptr;
			}
		}

		_blocks.clear();
	}

	UINT32 Count()
	{
		return _objectCount;
	}

	UINT32 InOut()
	{
		return _inout;
	}

	UINT32 MemSize()
	{
		return _objectCount * sizeof(T);
	}

protected:

	UINT32 _maxout;
	UINT32 _inout; // outstanding object count (for debug); should be 0 when this pool is freed
	UINT32 _objectCount;
	UINT32 _blockSize;
	STRATEGY _strategy;
	INSTANCE_HOOK _hook;
	std::stack<T*> _pool;
	std::vector<T*> _blocks;
};

struct gr2Event
{
	SINT32		eventType;
	FLOAT32		eventTime;
	char		eventData[MAX_EVENT_DATA+1];

#ifdef GR2_MEMORY_PAD
	char pad[4];
#endif
};

struct gr2QueuedEvent
{
	UINT32				type;
	FLOAT32				timeToOccur;
	FLOAT32				prevAnimLoopTime;
	granny_control*		control;
	char				data[MAX_EVENT_DATA+1];

#ifdef GR2_MEMORY_PAD
	char pad[12];
#endif
};

struct gr2BoneTwist
{
	gr2BoneTwist(UINT32 b, UINT32 a, FLOAT64 a2) : bone(b), axis(a), angle(a2)
	{
	}

	UINT32				bone; // which bone to rotate
	UINT32				axis; // which axis to rotate on
	FLOAT64				angle; // how much to rotate in degrees
};

struct gr2BoneAdjust
{
	gr2BoneAdjust(UINT32 b, Vec3f& a, SINT32 d) : bone(b), adjustment(a)
	{
		stoptime = frametime + d;
	}

	SINT32 stoptime;
	UINT32 bone; // which bone to adjust
	Vec3f adjustment;
};

using GR2_EVENT_LIST = std::vector<gr2Event*>;

struct gr2Animation
{
	granny_animation*	animation;
	SCHAR				name[MAX_GR2_PATH_SIZE+1];
	granny_track_mask*	mask;
	gr2Model*			target;
	SINT32				animType;
	GR2_EVENT_LIST*		events;
	//gr2Event*			events[MAX_NUM_ANIM_EVENTS];

	static gr2Animation* Load(const char* path, SINT32 useCache);

#ifdef GR2_MEMORY_PAD
	char pad[4];
#endif
};

using GR2_ANIM_EVENTS = std::map<granny_animation*,GR2_EVENT_LIST>;

struct gr2Control
{
	gr2Model*			model;
	granny_control*		control;
	FLOAT32				easeOutDuration;
	FLOAT32				easeInDuration;
	gr2Animation*		animation;

	void SetLastFrame();
	void SetLoopCount(SINT32 numLoops);
	void SetEaseIn(FLOAT32 time);
	void SetEaseOut(FLOAT32 time);
	void SetSpeed(FLOAT32 speed);
	void SetWeight(FLOAT32 weight);
	UINT32 IsActive();
	UINT32 IsComplete();
	void Stop(LOGICAL immediate);
	void Update();

#ifdef GR2_MEMORY_PAD
	char pad[12];
#endif
};

// blend types
// animation blend (list of animations .. note that each animation contains the mask to play it)
struct gr2AnimationBlend
{
	enum
	{
		BASE = 0,		// first animation is the base and is weighted against the other animations (base + up to 4)
		ARBITRARY,		// animations in the blend are weighted arbitrarily (up to 4)
		FULL,			// all animations are blended at full weight (bone weigthing still applies)
		TRANSITION,		// all animations start full bu tfade out over the life of the blend (plays for 1 loop)
	};

	// blend type specific data (currently only used by TRANSITION blends
	FLOAT32				mod1;
	FLOAT32				mod2;
	FLOAT32				length;

	SINT32				type;
	SINT32				numBlends;
	gr2Animation*		animations[GR2_MAX_ANIMS_PER_BLEND];
	SCHAR				blendFile[255];

	static gr2AnimationBlend* Load(const char* path);
	static gr2AnimationBlend* Create(const char* path);

	//void SetWeights(FLOAT32
	FLOAT32 GetBaseLength();
	FLOAT32 GetLength(UINT32 index = 0);
	FLOAT32 GetTime(gr2Model* model, UINT32 index = 0);
	const char*	GetPath();

#ifdef GR2_MEMORY_PAD
	char pad[8];
#endif
};

// control blend .. list of controls, one per animation in the blend
// note .. the control here is NOT a *, it's the real thing .. this is so I'm not continuously allocating

using CONTROL_CALLBACK = void(*)(void*);

struct gr2ControlBlend
{
//	gr2AnimationBlend*	blend;
	gr2Control				controls[GR2_MAX_ANIMS_PER_BLEND];
	FLOAT32					easeOutDuration;
	FLOAT32					easeInDuration;
	FLOAT32					modifier;
	FLOAT32					startWeight;
	SINT32					startTime;	// time (use frametime for comparison) at which to kill this control
	SINT32					killTime;	// time (use frametime for comparison) at which to kill this control
	CONTROL_CALLBACK		killCallback;
	void*					killData;
	LOGICAL					active;		// needed?
	LOGICAL					mixin;		// declares a mixin, i.e. something that isn't to be removed as part of the normal animation changing
	SINT32					loopCount;
//	SINT32					easeOutTime;	// time (use frametime for comparison) at which to start blending this control out

#ifdef GR2_MEMORY_PAD
	char pad[4];
#endif
};

using GR2_ID_LIST = std::vector<int>;

struct gr2MeshData
{
	gr2MeshData() :
		GrannyMesh(nullptr),
		GrannyBinding(nullptr),
		GrannyDeformer(nullptr),
		rigidMesh(TRUE),
		indexArray(nullptr),
		vertexArray(nullptr),
		pngt(nullptr),
		drawcallback(nullptr),
        is_normal(false),
        NormalEnabled(9999),
        vboid(0)
	{
	}

	~gr2MeshData()
	{
		reset();
    }

	void reset()
	{
		drawcallback = nullptr;
		textureRefs.clear();
		materialRefs.clear();

        if(indexArray)
            delete[] indexArray;

		indexArray = nullptr;

        if(vertexArray)
            delete[] vertexArray;

		vertexArray = nullptr;

        if(GrannyBinding)
        {
            GrannyFreeMeshBinding(GrannyBinding);
            GrannyBinding = nullptr;
        }

        if(GrannyDeformer)
        {
            GrannyFreeMeshDeformer(GrannyDeformer);
            GrannyDeformer = nullptr;
        }

        if(pngt)
            delete[] pngt;

        is_normal = false;
        NormalEnabled = 9999;

		pngt = nullptr;
	}

	// Granny's mesh information for this mesh
	granny_mesh* GrannyMesh;

	// The binding between this mesh and the model its deformed with
	granny_mesh_binding* GrannyBinding;

	// The deformer that I'm using to deform the mesh
	granny_mesh_deformer* GrannyDeformer;

	LOGICAL	rigidMesh;

	// mesh data
	SINT16* indexArray;
	piGVERTEX* vertexArray;

	// normals
	piTVERTEX* pngt;

    granny_pwngt34332_vertex* pwngt;
    granny_pnt332_vertex* pnt;

	// texture references
	GR2_ID_LIST textureRefs;

	// material references
	GR2_ID_LIST materialRefs;

	bool (*drawcallback)(gr2Mesh* mesh, short* indices, int numverts, granny_tri_material_group* g);
    bool is_normal;
    LOGICAL NormalEnabled;
    int vboid;

#if USE_STATESET
    RenderStateSet* stateset;
#endif
	// draw block index
	//SINT32 block;
};

struct gr2Mesh
{
	// our shared mesh data
	gr2MeshData* data;

	// do we draw this mesh
	LOGICAL active;

	// should we make this mesh mutable
	LOGICAL mutate;

	// local copies if needed
	piGVERTEX* vertexArray;
	piTVERTEX* pngt;

	// when we last deformed this mesh
	SINT32 lastDeform;

    gr2Mesh()
    {
		data = nullptr;
		vertexArray = nullptr;
		pngt = nullptr;
        active = FALSE;
		mutate = FALSE;
		lastDeform = 0;
    }

    ~gr2Mesh()
    {
		reset();
    }

	piGVERTEX* Vertices()
	{
		if(mutate && !vertexArray && data->vertexArray)
		{
			SINT32 vertexCount = GrannyGetMeshVertexCount(data->GrannyMesh);

			vertexArray = new piGVERTEX[vertexCount];
			memcpy(vertexArray, data->vertexArray, vertexCount * sizeof(piGVERTEX));
		}

		return (vertexArray) ? vertexArray : data->vertexArray;
	}

	piTVERTEX* PNGT()
	{
		if(mutate && !pngt && data->pngt)
		{
			SINT32 vertexCount = GrannyGetMeshVertexCount(data->GrannyMesh);

			pngt = new piTVERTEX[vertexCount];
			memcpy(pngt, data->pngt, vertexCount * sizeof(piTVERTEX));
		}

		return (pngt) ? pngt : data->pngt;
	}

	void Stamp()
	{
		SINT32 vertexCount = GrannyGetMeshVertexCount(data->GrannyMesh);

		if(data->vertexArray)
		{
			vertexArray = new piGVERTEX[vertexCount];
			memcpy(vertexArray, data->vertexArray, vertexCount * sizeof(piGVERTEX));
		}

		if(data->pngt)
		{
			pngt = new piTVERTEX[vertexCount];
			memcpy(pngt, data->pngt, vertexCount * sizeof(piTVERTEX));
		}
	}

	void reset()
	{
		data = nullptr;
		active = FALSE;
		mutate = FALSE;
		lastDeform = 0;

        if(vertexArray)
            delete[] vertexArray;

		vertexArray = nullptr;

        if(pngt)
            delete[] pngt;

		pngt = nullptr;
	}

#ifdef GR2_MEMORY_PAD
	char pad[4];
#endif
};

// some types used by gr2Model
using GR2_BLEND_MAP = std::map<gr2AnimationBlend*,gr2ControlBlend*>;
using GR2_MESH_LIST = std::vector<gr2Mesh*>;
using GR2_MODEL_LIST = std::vector<gr2Model*>;
using GR2_QEVENT_LIST = std::vector<gr2QueuedEvent*>;
using GR2_TWIST_LIST = std::vector<gr2BoneTwist>;
using GR2_ADJUST_LIST = std::vector<gr2BoneAdjust>;
using GR2_LOD = gr2Model*;

// finally, define the model, which wraps it all up
struct gr2Model
{
	gr2Model();
	virtual ~gr2Model();

	// load a model
	static gr2Model* Load(const char* path, gr2Model* wornOnModel, SINT32* boneCount, SINT32 instantiate, int modelIndex = -1, bool wantNormals = true);

	// reset this model back to a base state
	virtual void reset();

	// attachments
	LOGICAL Attach(gr2Model* toModel, SCHAR* toBone, SCHAR* fromBone = nullptr);
	LOGICAL	Attach(gr2Model* toModel, SINT32 toBone, SINT32 fromBone = 0);
	void Detach();
	void UpdateAttachment();
	void FreeAttachments();
	void TransferAttachments(gr2Model* to);

	// rendering
	bool Render(bool update = true, bool attached = false);
	void RenderAttachments(bool update = true);
	void RenderShadow(bool update = true);
	void RenderDamage(bool update = true);
	void RenderSkeleton();

	// make the meshes associated with this model mutable
	void MakeMutable();

	// updates
	virtual void Update(bool force = false, bool granular = true);

	// orientation
	void SetRotation(union piMATRIX* matrix);
	void SetPosition(piVECTOR* position);
	void GetPosition(piVECTOR* position);
	void GetFinalTransform(piMATRIX* matrix);

	// misc accessors
	SINT32 GetBoneCount();
	gr2Model* GetWornOnModel();
	granny_skeleton* GetSkeleton();
	granny_model_instance* GetInstance();
	granny_world_pose* GetWorldPose();
	void CopyWorldPose(gr2Model* to);
	const char*	GetPath();

	// status
	void SetActive(bool active);
	LOGICAL IsActive();
	void SetMeshActive(int mesh, bool active);
	void SetMeshActive(const char* mesh, bool active);
	LOGICAL IsMeshActive(int mesh);
	LOGICAL IsMeshActive(const char* mesh);

	// bone accessors
	granny_real32* GetBoneTransform(SCHAR* boneName);
	granny_real32* GetBoneTransform(int boneIndex);
	granny_real32* GetCompositeBoneTransform(SCHAR* boneName);
	granny_real32* GetCompositeBoneTransform(int boneIndex);
	LOGICAL GetBonePosition(SCHAR* boneName, piMATRIX* matrix);
	LOGICAL GetBonePosition(int boneIndex, piMATRIX* matrix);
	LOGICAL GetRawBonePosition(SCHAR* boneName, piMATRIX* matrix);
	LOGICAL	HasBone(SCHAR* boneName);
	LOGICAL HasMesh(const char* mesh, int* meshIndex, int* boneIndex);
	SINT32 FindBone(SCHAR* boneName, LOGICAL exact = 1);
	SINT32 FindBones(std::vector<int>& found, SCHAR* boneName, LOGICAL exact = 1);
	void SetPositioningBone(SCHAR* boneName);

	// collision detections
	LOGICAL CollideRay(piVECTOR* rayStart, piVECTOR* rayNormal, SINT32* location, piVECTOR* collisionPnt);

	// texture mods
	void Personalize(SINT32 countryID, SINT32 branchID, SCHAR* handle, SINT32 decalID);
	void FindAndReplaceTexture(SINT32 origTexID, UINT32 replaceTexID);
	SINT32* GetLoadedTextures(SINT32* numTextures);

	// animation and blend playing
	gr2Control*	PlayAnimation(gr2Animation* animation);
	void StopAnimation(gr2Control* control);

	LOGICAL	PlayBlend(gr2AnimationBlend* animationBlend, bool allowResurrect = true);
	LOGICAL	PlayBlend(gr2AnimationBlend* blend, FLOAT32 easeInTime, FLOAT32 easeOutTime, bool allowResurrect = true);

	// animation blend control
	void SyncBaseLoop(gr2AnimationBlend* blend, gr2Control* control);
	void SetMixin(gr2AnimationBlend* animationBlend);
	void SetWeight(gr2AnimationBlend* animationBlend, FLOAT32 weight);
	void SetSpeed(gr2AnimationBlend* animationBlend, FLOAT32 speed);
	void SetLastFrame(gr2AnimationBlend* animationBlend);
	void SetLoopCount(gr2AnimationBlend* animationBlend, SINT32 count);
	void SetLoopCountForGrenade(gr2AnimationBlend* animationBlend, SINT32 count);
	void SetEaseOut(gr2AnimationBlend* animationBlend, FLOAT32 time);
	void SetEaseIn(gr2AnimationBlend* animationBlend, FLOAT32 time);
	void ResurrectBlend(gr2AnimationBlend* animationBlend, FLOAT32 maxEaseInTime);
	void RemoveWhenDone(gr2AnimationBlend* animationBlend);
	// note .. set easeOutOverrideTime to -1 if you want to use the default ease out time
	LOGICAL RemoveAllBlends(FLOAT32 easeOutOverrideTime, gr2AnimationBlend* blend = nullptr);
	LOGICAL ClearBlends();
	LOGICAL IsBlendComplete(gr2AnimationBlend* animationBlend);
	FLOAT32	GetBlendCompletion(gr2AnimationBlend* animationBlend);
	void SetStartOffset(gr2AnimationBlend* animationBlend, FLOAT32 percent);
	void EaseIn(gr2AnimationBlend* animationBlend, FLOAT32 time);
	void EaseOut(gr2AnimationBlend* animationBlend, FLOAT32 time);

	//virtual void Update(bool force = false, bool granular = true);

	// event handling
	const char*	GetEventData(SINT32 eventID);
	SINT32 GetEventType(SINT32 eventID);
	void HandleEvent(SINT32 eventID);
	void SetEventHandler(OBJECT* pObject, GR2_EVENTHANDLER eventHandler);
	void UpdateBlendEvents();
	void UpdateEvents();

	// twist handling
	void CopyTwists(gr2Model* to);
	void SetTorsoRotation(FLOAT64 angle, LOGICAL useAltAxis);
	void SetHeadRotation(FLOAT64 angle);
	void SetHeadPitch(FLOAT64 angle);
	void SetModelPitch(FLOAT64 angle);

	FLOAT64 SetModelRotation(FLOAT64 angle);
	FLOAT64 GetModelRotationDiff(FLOAT64 angle);
	FLOAT64 GetTorsoRotationDiff(FLOAT64 angle);

	void SetBoneTwist(UINT32 bone, UINT32 axis, FLOAT64 angle = 0.0f);
	FLOAT64& AddBoneTwist(UINT32 bone, UINT32 axis, FLOAT64 angle = 0.0f);
	void AddBoneAdjust(UINT32 bone, Vec3f& adjustment, SINT32 duration);

	bool AddAttachment(gr2Model* attachment, int attachBone, int parentBone = 0, bool store = false);
	bool RemoveAttachment(gr2Model* attachment);

	// granny data
	granny_model*			grannyModel;
	granny_model_instance*	GrannyInstance;
	granny_world_pose*		WorldPose;
	granny_real32			lastUpdateTime;
	granny_real32			lastDeformTime; // last frametime we were deformed
#ifdef LOD_ANIMATIONS
	granny_real32			lodError;
#endif

	// pointer to my own skeleton
	granny_skeleton*		skeleton;
	int						boneCount;

	// wearing stuff
	int						usesAnotherSkeleton;
	gr2Model*				wornOnModel;

	// attach stuff
	int						attachBone, attachToBone;
	gr2Model*				attachToModel;
	granny_matrix_4x4		attachOffset;
	LOGICAL					updateAttachment;

	// position stuff
	piMATRIX*				finalModelTransform;
	SINT32					rootPositioningBone;

	// texture data
	SINT32					numTextures;
	SINT32*					loadedTextures;
	SINT32					currentFaceID;
	SINT32					patchTexID;

	// game data
	OBJECT*					pObject;
	gr2Object*				object;

	// do we draw and update this model?
	LOGICAL					active;

	// used by trooper skeletons
	SINT32					surfaceID;

    const char*				name;

	// minimum visible range in meters (o means no min limit)
	UINT32					minVis;

	// maximum visible range in meters (o means infinite)
	UINT32					maxVis;

	GR2_MODEL_LIST			attachments;

	// The meshes that comprise this model
	GR2_MESH_LIST			meshes;

	// event handler
	GR2_EVENTHANDLER		eventHandler;

	// arbitrary bone twisting (head turning, rotational smoothing, etc...)
	GR2_TWIST_LIST			twists;

	// arbitrary bone position adjustments (gun recoil)
	GR2_ADJUST_LIST			adjustments;

	// twisting state
	//FLOAT64					torsoTwist, headTwist, modelTwist, headPitch, modelPitch;
	//LOGICAL					useAltTorsoAxis;

	// controls running on this model
	GR2_BLEND_MAP*			blends;

	// events this model needs to handle
	GR2_QEVENT_LIST			events;

#ifdef GR2_MEMORY_PAD
	//char pad[0];
#endif

};

// supports up to 4 models arbitrarily on and off
struct gr2ModelList : public std::vector<GR2_LOD>
{
	gr2ModelList();
	virtual ~gr2ModelList();

	// add a model and the max distance it can be viewed at
	// returns its index
	virtual gr2Model* AddModel(gr2Model* model);

	// returns the first active model
	virtual gr2Model* GetActive();

	// get a model by index
	virtual gr2Model* Get(SINT32 index);

	// get a model by lod distance
	virtual gr2Model* Get(FLOAT32 distance);

	// set a model active or not by index
	virtual void Set(SINT32 index, bool active = true);

	// update any lods in this multimodel; behavior is subclass dependendant
	virtual void UpdateLODS(UINT32 distance, UINT32 realDistance = 0);

	// do a gr2Update on any active models
	virtual void Update(bool force, bool granular);

	// render any active models
	virtual bool Render(bool update = true);

	// free all models in this multimodel
	void Free();
};

// supports up to 4 model lods
struct gr2ModelLOD : public gr2ModelList
{
	gr2ModelLOD();
	virtual ~gr2ModelLOD(){}

	// returns the first active model if -1
	// otherwise the specified model via index if its active
	virtual gr2Model* GetActive();

	// return our active lod index
	virtual SINT32 GetLOD(){ return _active; }

	// set a model active or not by index
	virtual void Set(SINT32 index, bool active = true);

	// update any lods in this multimodel; behavior is subclass depdendent
	virtual void UpdateLODS(UINT32 distance, UINT32 realDistance = 0);

	// do a gr2Update on any active models
	virtual void Update(bool force, bool granular);

	// render any active models
	virtual bool Render(bool update = true);

private:

	SINT32 _active;
};

struct gr2ModelAlt
{
	gr2ModelAlt();
	virtual ~gr2ModelAlt();

	gr2Model* AddModel(UINT32 index, gr2Model* model);
	gr2Model* GetActive();
	gr2Model* Get(SINT32 index);
	gr2Model* Get(FLOAT32 distance);
	GR2_MODEL_LIST GetAllModels();
	SINT32 GetLOD();
	UINT32 Size(SINT32 index = -1); // -1 means all models; >= 0 index means size of that model array
	void Set(bool alt = false);
	void Set(SINT32 index);
	void Toggle();
	void UpdateLODS(UINT32 distance, UINT32 realDistance = 0);
	void Update(bool force, bool granular);
	bool Render(bool update = true);
	void Free();

private:

	SINT32 _active;
	gr2ModelLOD models[2];
};

/*
// a model that has extra functionality for handling animations
struct gr2AnimModel : public gr2Model
{
	gr2AnimModel();
	virtual ~gr2AnimModel();

	static gr2AnimModel* Load(const char* path, gr2Model* wornOnModel, SINT32* boneCount, SINT32 instantiate, int modelIndex = -1);

	// reset this model back to a base state
	virtual void reset();

	// animation and blend playing
	gr2Control*	PlayAnimation(gr2Animation* animation);
	LOGICAL	PlayBlend(gr2AnimationBlend* animationBlend, bool allowResurrect = true);
	LOGICAL	PlayBlend(gr2AnimationBlend* blend, FLOAT32 easeInTime, FLOAT32 easeOutTime, bool allowResurrect = true);

	// animation blend control
	void SyncBaseLoop(gr2AnimationBlend* blend, gr2Control* control);
	void SetMixin(gr2AnimationBlend* animationBlend);
	void SetWeight(gr2AnimationBlend* animationBlend, FLOAT32 weight);
	void SetSpeed(gr2AnimationBlend* animationBlend, FLOAT32 speed);
	void SetLastFrame(gr2AnimationBlend* animationBlend);
	void SetLoopCount(gr2AnimationBlend* animationBlend, SINT32 count);
	void SetLoopCountForGrenade(gr2AnimationBlend* animationBlend, SINT32 count);
	void SetEaseOut(gr2AnimationBlend* animationBlend, FLOAT32 time);
	void SetEaseIn(gr2AnimationBlend* animationBlend, FLOAT32 time);
	void ResurrectBlend(gr2AnimationBlend* animationBlend, FLOAT32 maxEaseInTime);
	void RemoveWhenDone(gr2AnimationBlend* animationBlend);
	// note .. set easeOutOverrideTime to -1 if you want to use the default ease out time
	LOGICAL RemoveAllBlends(FLOAT32 easeOutOverrideTime, gr2AnimationBlend* blend = nullptr);
	LOGICAL ClearBlends();
	LOGICAL IsBlendComplete(gr2AnimationBlend* animationBlend);
	FLOAT32	GetBlendCompletion(gr2AnimationBlend* animationBlend);
	void SetStartOffset(gr2AnimationBlend* animationBlend, FLOAT32 percent);
	void EaseIn(gr2AnimationBlend* animationBlend, FLOAT32 time);
	void EaseOut(gr2AnimationBlend* animationBlend, FLOAT32 time);

	virtual void Update(bool force = false, bool granular = true);

	// event handling
	const char*	GetEventData(SINT32 eventID);
	SINT32 GetEventType(SINT32 eventID);
	void HandleEvent(SINT32 eventID);
	void SetEventHandler(OBJECT* pObject, GR2_EVENTHANDLER eventHandler);
	void UpdateBlendEvents();
	void UpdateEvents();

	// twist handling
	void CopyTwists(gr2Model* to);
	void SetTorsoRotation(FLOAT64 angle, LOGICAL useAltAxis);
	void SetHeadRotation(FLOAT64 angle);
	void SetHeadPitch(FLOAT64 angle);
	void SetModelPitch(FLOAT64 angle);

	FLOAT64 SetModelRotation(FLOAT64 angle);
	FLOAT64 GetModelRotationDiff(FLOAT64 angle);
	FLOAT64 GetTorsoRotationDiff(FLOAT64 angle);

	// event handler
	GR2_EVENTHANDLER		eventHandler;

	// twisting state
	FLOAT64					torsoTwist, headTwist, modelTwist, headPitch, modelPitch;
	LOGICAL					useAltTorsoAxis;

	// controls running on this model
	GR2_BLEND_MAP*			blends;

	// events this model needs to handle
	GR2_QEVENT_LIST			events;

#ifdef GR2_MEMORY_PAD
	//char pad[0];
#endif

};

*/

#ifdef GR2_BLEND_EDITOR
class gr2BlendEditor
{
public:

	enum BLEND_TYPE
	{
		NONE = 0,
		ANIMATION,
		EMOTE,
		MIXIN
	};

	gr2BlendEditor();
	~gr2BlendEditor();

	// load blend
	// create blend
	// save blend

	static gr2BlendEditor* GetInstance();

private:

	BLEND_TYPE _type;
	SINT32 _states[4]; // we can have up to 4 states

};
#endif

using gr2ModelRef = gr2Model*;
//using gr2AnimModelRef = gr2AnimModel*;
using gr2ControlRef = gr2Control*;
using gr2AnimationRef = gr2Animation*;
using gr2AnimationBlendRef = gr2AnimationBlend*;
using gr2ControlBlendRef = gr2ControlBlend*;
