#pragma once

#include <map>
#include <string>
#include "physics/colliderMain.h"
#include "physics/physicsMain.h"
#include "math/Matrixf.h"
#include "math/Matrixd.h"

#define GR2_PHYSICS_MAXNAME 255
#define GR2_COLLIDE_THRESHOLD 5000

struct gr2Model;
struct dContact;
struct OBJECT;
class gr2PhysicsModel;
class gr2PhysicsBone;

typedef gr2PhysicsBone* GR2PhysicsBoneRef;
typedef std::vector<dGeomID> GR2GeomList;

// these are not used yet, but will be

//void gr2PhysicsInit();
//void gr2PhysicsUpdate();
void gr2PhysicsReset();
bool gr2PhysicsDrawCallback(OBJECT* ob);
void gr2PhysicsKill();
//void gr2PhysicsAddModel(gr2PhysicsModel* model);
//void gr2PhysicsRemoveModel(gr2PhysicsModel* model);

struct gr2ValueModel
{
	float* _value;
	float _start, _stop;
	int _duration;

	gr2ValueModel() : _value(NULL), _start(0), _stop(0), _duration(0)
	{
	}

	void init()
	{
		if(_duration && _value)
			*_value = _start;
	}

	bool run(int started, bool loop = false)
	{
		if(!_value || !_duration)
			return false;

		float diff = _start - _stop;

		if(!diff)
			return false;

		bool increasing = (_start < _stop);

		// if loop is true, then value will be modified up or down until duration
		// if hit, then it will reverse and recalculate back to the original value
		// in the second block

		if(frametime < started + _duration)
		{
			if(increasing) // increasing value
			{
				*_value = _start + (diff * ((float)(frametime - started) / (float)_duration));

				if(*_value > _stop)
					*_value = _stop;

				return true;
			}
			else // decreasing value
			{
				*_value = _start - (diff * ((float)(frametime - started) / (float)_duration));

				if(*_value < _stop)
					*_value = _stop;

				return true;
			}
		}
		else if(loop && (frametime < started + _duration + _duration))
		{
			if(increasing) // started off increasing value
			{
				*_value = _stop - (diff * ((float)(frametime - started + _duration) / (float)_duration));

				if(*_value < _start)
					*_value = _start;

				return true;
			}
			else // started off decreasing value
			{
				*_value = _stop + (diff * ((float)(frametime - started + _duration) / (float)_duration));

				if(*_value > _start)
					*_value = _start;

				return true;
			}
		}

		return false;
	}
};

typedef std::map<float*,gr2ValueModel> GR2ValueModelMap;

class gr2PhysicsBody : public physicsBodyBase
{
public:

	enum STATES
	{
		NO_STATE		= 0,
		SPLASHED		= 1<<0,
		CONTACT			= 1<<1, // first contact
		CONTACTED		= 1<<2, // other contacts
		VEHICLE_HIT		= 1<<3, // if a vehicle hit us
		KILLED			= 1<<4  // killed by reset
	};

	gr2PhysicsBody();
	virtual ~gr2PhysicsBody();

	virtual bool HandleContact(colliderQuery::Collision&);
	virtual void AdjustCollideJoint(colliderQuery::Collision& hit, dContact* contact){}
	void AttachCollideJoint(colliderQuery::Collision&);
	void ClearCollideJoints();
	UINT32 LastCollisions() const { return _lastCollision; }
	void SetCollideWithSiblings();

	virtual void SetExpired(bool e);

protected:

	UINT32 _allCollisions;
	UINT32 _lastCollision;
	GR2GeomList _geoms;
	dJointGroup* _collideJoints;
};

class gr2PhysicsBone : public gr2PhysicsBody
{
public:

	enum AXIS
	{
		X = 1,
		Y,
		Z
	};

	// how is the bone oriented, from the body center or the connected joint
	enum ORIENTATION
	{
		CENTER = 0,
		AXIAL,
		STATIC,
		MESH
	};

	gr2PhysicsBone(gr2PhysicsModel* model,	// parent physics model
					const char* name,		// bone name
					int orient,				// bone orientation
					double mass,			// bone mass
					double len,				// capsule length
					double rad,				// capsule radius
					double ang,				// rotation angle
					int axis);				// rotation axis 1 = x, 2 = y, 3 = z

	virtual ~gr2PhysicsBone();

	virtual void BodyToBone(gr2Model* from = NULL); // bone to body
	virtual void BoneToBody(gr2Model* to = NULL); // body to bone

	// creation override for custom setups
	virtual void Init();
	virtual void Setup();

	//virtual void initCollisions(){}
	virtual void Start();
	virtual void BeginStep();
	virtual void EndStep();
	virtual void AdjustCollideJoint(colliderQuery::Collision& hit, dContact* contact);
	virtual void SetExpired(bool e);

	// math models
	virtual void SetFrictionModel(float start, float stop, int duration);
	virtual void SetStiffnessModel(float start, float stop, int duration);
	void SetMods(float smod, float fmod);

	virtual void SetDisableModel(int duration);

	void AddValueModel(float& value, float start, float stop, int duration);
	bool RunValueModel(float& value, bool loop = false);

	void SetJointID(dJointID joint);

	bool MyShape(dGeomID geom);
	dGeomID SetShape(dGeomID geom, int cat = 0, int col = 0);
	dGeomID AddShape(dGeomID geom, int cat = 0, int col = 0);
	dGeomID SetBoxShape(dReal mass, double lx, double ly, double lz);
	dGeomID SetCapsuleShape(dReal mass, double angle = 0, int axis = 2);

	const char* GetName() const { return _name; }
	gr2PhysicsModel* GetModel() const { return _model; }
	int GetBone() const { return _bone; }
	dBodyID GetBody() const { return _bodyID; }
	dGeomID GetGeom(int index = 0) const;
	int GetShape(int index = 0) const;
	dJointID GetJoint() const { return _joint; }
	const GR2GeomList& GetGeoms() const{ return _geoms; }

	bool IsCollidable(){ return !_geoms.empty(); }

	// debug
	void SetColliderColor(UINT32 color){ _color = color; }
	void DrawCollider(GLUquadric* obj);
	void DrawBodyPosition(GLUquadric* obj, bool adjusted = false);
	void DrawBonePosition(GLUquadric* obj);

	// dimensions
	void SetLength(float l) { _length = l; }
	void SetRadius(float r) { _radius = r; }
	void SetAxis(AXIS a) { _axis = a; }
	void SetOrientation(ORIENTATION o) { _orientation = o; }
	void SetParent(gr2PhysicsBone* p) { _parent = p; }
	void SetLocalTransform(Matrixf& t) { _localTransform.set(t); }
	void SetOffset(double offset){ _offset = offset; }

	// higher stiffness means more a rigid joint:  0 to 50000 ?
	// fugde reduces push back force when a constraint is met in a step: 0 to 1, lower scales force down
	// bounce is how bouncing a stop is, 0 to 1, o means hard, 1 means fully bouncy
	// cfm is how hard the constraint it, lower is harder, higher is softer...0 to ?
	// serp is the error reduction param at a stop how much error in position a joint
	// is allowed to have, 0 to 1, 0 means no correction, 1 means rigid positions, .2 is reccomended
	// scfm cfm used at a stop
	dJointID SetJointParams(int axis = 0,
							double fudge = .2f,
							double stiffness = -1.0,
							double cfm = -1.0f,
							double serp = -1.0f,
							double scfm = -1.0f,
							double bounce = -1.0f);

	double GetMass() const { return _mass.mass; }
	float GetLength() const { return _length; }
	float GetRadius() const { return _radius; }
	AXIS GetAxis() const { return _axis; }
	ORIENTATION GetOrientation() const { return _orientation; }
	gr2PhysicsBone* GetParent() const { return _parent; }
	const Matrixf& GetLocalTransform() const { return _localTransform; }

	virtual Vec3d PhysicalPosition() const;
	virtual Matrixf PhysicalRotation() const;
	virtual Matrixf PhysicalTransform() const;

	virtual Vec3d VisualPosition() const;
	virtual Matrixf VisualRotation() const;
	virtual Matrixf VisualTransform() const;

	virtual void VisualToPhysical(Matrixd& transform);
	virtual void PhysicalToVisual(Matrixd& transform);

	virtual void Adjust(Matrixd& transform, Vec3d& offset, double angle, int axis);

	virtual Vec3d CollidePoint(Vec3d& pos) const; // this is a world position
	virtual Matrixd CollideTransform() const;

protected:

	GR2ValueModelMap _valueModels;

	char _name[GR2_PHYSICS_MAXNAME+1];

	gr2PhysicsModel* _model;
	gr2PhysicsBone* _parent;

	int _bone;
	dJointID _joint;
	dBodyID _bodyID;
	dMass _mass;
	float _length, _radius; // bone dimensions
	AXIS _axis;
	double _angle;
	ORIENTATION _orientation;
	double _offset; // bone offset

	Matrixf _localTransform;
	Matrixf _restTransform;

	 // friction model
	float _friction, _stiffness;
	float _frictMod, _stiffMod;

	// time over which to bleed friction and in what direction
	//int _fTime, _fDir;

	// period over which to increase disable threshold (guarantees the body will disable at some point)
	int _autoDisableTime;

	// when we're were last inited
	int _initTime;

	// debug collider color
	UINT32 _color;
};

// for bones centered around the visual bone position
class gr2CenterBone : public gr2PhysicsBone
{
public:
	gr2CenterBone(gr2PhysicsModel* model,	// parent physics model
					const char* name,		// bone name
					double mass,			// bone mass
					double len,				// capsule length
					double rad);			// capsule radius

	virtual ~gr2CenterBone(){}
};

// for bones that are relative to the connected joint
class gr2AxialBone : public gr2PhysicsBone
{
public:
	gr2AxialBone(gr2PhysicsModel* model,	// parent physics model
					const char* name,		// bone name
					double mass,			// bone mass
					double len,				// capsule length
					double rad);				// capsule radius

	virtual ~gr2AxialBone(){}

	// override so we can set the rest offset
	virtual void Init();
	virtual void PhysicalToVisual(Matrixd& transform);
};

// for bones that are relative to the connected joint
class gr2AxialFootBone : public gr2PhysicsBone
{
public:
	gr2AxialFootBone(gr2PhysicsModel* model,	// parent physics model
					const char* name,		// bone name
					double mass,			// bone mass
					double len,				// capsule length
					double rad);				// capsule radius

	virtual ~gr2AxialFootBone(){}

	// override so we can set the rest offset
	virtual void Init();
	virtual void PhysicalToVisual(Matrixd& transform);
	virtual void VisualToPhysical(Matrixd& transform);
};

// for bones that are relative to the parent bone (not dynamic)
class gr2StaticBone
{
public:
	gr2StaticBone(gr2PhysicsModel* model,	// parent physics model
					const char* name);		// bone name

	virtual ~gr2StaticBone(){}

	// so we can calculate our relative position to our parent
	void Init();
	void BodyToBone(gr2Model* to = NULL);

	void SetParent(int p) { _parent = p; }

	int GetBone() const { return _bone; }
	const char* GetName() const { return _name; }

protected:

	char _name[GR2_PHYSICS_MAXNAME+1];

	gr2PhysicsModel* _model;

	int _parent;
	int _bone;

	Matrixf _localTransform;
	Matrixf _restTransform;

	// when we're were last inited
	int _initTime;
};

// for bones that are relative to the parent bone (not dynamic)
class gr2MeshBone : public gr2PhysicsBone
{
public:
	gr2MeshBone(gr2PhysicsModel* model,	// parent physics model
					const char* name);		// bone name

	virtual ~gr2MeshBone()
	{
		_geoms.clear();
	}

	virtual void BeginStep();
	virtual bool HandleContact(colliderQuery::Collision&);

	// so we can calculate our relative position to our parent
	//virtual void Init();
	//virtual void BodyToBone(gr2Model* to);

	//virtual bool IsCollidable(){ return (meshCollider.geoms() > 0); }

protected:
	//void makeCollider();

	Vec3d lastFreePos;
	Vec3d colliderOffset;
	physicsCollider meshCollider;
};

class gr2PhysicsModel : public physicsModel
{
public:

	enum DRAW_STATE
	{
		DS_DRAW,
		DS_NODRAW,
		DS_DELETE
	};

	//typedef std::map<std::string,GR2PhysicsBoneRef> BoneMap;
	typedef std::vector<gr2PhysicsBone*> BoneList;
	typedef std::vector<gr2StaticBone*> StaticBoneList;
	typedef std::vector<dJointID> JointList;

	gr2PhysicsModel(int timeout = -1);
	gr2PhysicsModel(gr2Model* model, OBJECT* ob = NULL, int timeout = -1);
	virtual ~gr2PhysicsModel();

	// inits should be called roughly in this order the first time
	virtual void Init(int timeout = -1);
	virtual bool InitSkeleton(const char* path); // set our base skeleton
	virtual void InitBones(); // tell our bones/bodies to init themselves
	virtual void InitModel(gr2Model* m); // setup up our visual model
	virtual void InitNode(OBJECT* ob); //Vec3d* pos, Quat* rot); // init our scenegraph node for drawing
	virtual void InitPose(gr2Model* poseModel); // initializes bodies to current pose

	// update functions
	virtual void BeginStep();
	virtual void Update();
	virtual void EndStep();
	virtual void Deactivate();
	//virtual bool IsActive();

	virtual void Attach(gr2PhysicsModel* m);
	virtual void OnAttach(gr2PhysicsModel* m);
	virtual void Unattach();
	virtual void OnUnattach(gr2PhysicsModel* m);

	virtual void OnExpire(DRAW_STATE& state){}
	virtual DRAW_STATE OnDraw();
	virtual void Draw();

	void BonesToBodies(gr2Model* to = NULL); // bodies to bones
	void BodiesToBones(gr2Model* from = NULL); // bones to bodies

	void UpdateBones();		// updates bone positions and rotations from bodies
	void UpdateVelocity();  // bleed model velocity over time
	void UpdateBoneTransform(float* boneMat, Vec3d& pos, Matrixd& rot);
	void UpdateBoneTransform(float* boneMat, Vec3d& pos, Matrixf& rot);

	bool IsExpired();
	void Expire();
	void SetExpires(int timeout);
	void SetActive(bool a, int timeout = -1);

	virtual bool ProcessFlag(unsigned int flag, physicsObject* o = NULL);
	//virtual void SetFlag(unsigned int f, gr2PhysicsBody* b = NULL);

	// gr2 bone accessors
	virtual bool GetBoneTransform(const char* name, Matrixf* transform);
	virtual bool GetBoneTransform(int bone, Matrixf* transform);
	virtual bool GetWorldBoneTransform(const char* bone, Matrixd* transform);
	virtual bool GetWorldBonePosition(const char* bone, Vec3d& position, Vec3d* offset = NULL);
	virtual Vec3d& GetWorldBonePosition(const char* bone);

	const Matrixd& GetWorldTransform() const { return _worldTransform; }
	const Vec3d& GetVelocity(){ return _velocity; }
	gr2PhysicsBone* GetBone(int bone);
	gr2PhysicsBone* GetBone(dBodyID body);
	gr2PhysicsBone* GetBone(const char* name);
	gr2StaticBone* GetStaticBone(int bone);
	gr2StaticBone* GetStaticBone(const char* name);
	BoneList& GetBones(){ return _bones; }
	int GetBoneIndex(dBodyID body);
	double GetBoneDist(const char* bone1, const char* bone2);
	gr2Model* GetSkeleton();
	dSpaceID GetSpaceID(){ return _collider->space(); }
	gr2Model* GetModel(){ return _model; }
	OBJECT* GetModelNode(){ return _modelNode; }

	colliderQuery* GetCollider(){ return _collider; }
	unsigned int GetColliderFlags();
	colliderQuery::CollisionList& GetCollisions(gr2PhysicsBone* bone);

	const piMATRIX& GetRootMatrix(){ return _rootMatrix; }
	Vec3d& GetRootPos(){ return _lastRootPos; }

	int GetMaterial(){ return _material; }

	const char* GetCorpse(){ return _corpse.c_str(); }

protected:

	gr2PhysicsBone* addBone(gr2PhysicsBone* newBone);

	// known bone types
	gr2PhysicsBone* addCenterBone(const char* name,	double mass, double len, double rad);
	gr2PhysicsBone* addAxialBone(const char* name, double mass, double len, double rad);
	gr2PhysicsBone* addAxialFootBone(const char* name, double mass, double len, double rad);
	gr2StaticBone* addStaticBone(const char* name);
	gr2PhysicsBone* addMeshBone(const char* name);

	// generic bone type
	gr2PhysicsBone* addBone(const char* name,	// bone name
							double mass = 0,	// bone mass
							double len = 0,		// capsule length (minus caps)
							double rad = 0,		// capsule radius
							double ang = 0,		// initial rotation angle
							int axis = 2,		// initial rotation axis 1 = x, 2 = y, 3 = z
							double xoff = 0,	// initial x offset
							int orient = 0);	// bone position orientation

	dJointID addJoint(gr2PhysicsBone* from,
						gr2PhysicsBone* to,
						int type,
						Vec3d* ax1 = 0,
						SINT32 ax1Lo = 0,
						SINT32 ax1Hi = 0,
						Vec3d* ax2 = 0,
						SINT32 ax2Lo = 0,
						SINT32 ax2Hi = 0,
						Vec3d* ax3 = 0,
						SINT32 ax3Lo = 0,
						SINT32 ax3Hi = 0);

	// gr2 model we're simulating and its skeleton
	gr2Model* _skeleton;
	gr2Model* _model;

	BoneList _bones;
	JointList _joints;

	// these bones are simple and don't need all of the stuff real bones need
	StaticBoneList _staticBones;

	int _material;
	int _initTime, _collideTime;
	int _lastUpdate;
	int _expires;
	bool _expired;
	int _enabledBones, _lastEnabledBones, _collideBones, _hiddenBones;
	unsigned int _flags;

	UINT32 _allCollisions;
	UINT32 _lastCollision;

	Matrixd _worldTransform;
	Vec3d _velocity;
	Vec3d _lastRootPos;

	colliderQuery::CollisionList _collisions;
	colliderQuery* _collider;

	// our scenegraph node
	OBJECT* _modelNode;

	gr2PhysicsBone* _rootBone;
	piMATRIX _rootMatrix;

	// model we're attached to; if any
	gr2PhysicsModel* _attached;

	// the path of the gr2 model to leave when corpsing this physics model
	std::string _corpse;
};

class gr2RigidPhysicsModel : public gr2PhysicsModel
{
public:
	gr2RigidPhysicsModel(const char* path, struct vehVehicle* veh, Matrixf transform, int timeout = -1);
	virtual ~gr2RigidPhysicsModel();

	virtual void BeginStep();
	virtual void EndStep();
	virtual void InitPose(gr2Model* poseModel);
	virtual void OnExpire(gr2PhysicsModel::DRAW_STATE& state);
	virtual void OnRemove();
	virtual bool ProcessFlag(unsigned int flag, physicsObject* o = NULL);

protected:
	void damp();
	bool makeCollider();

	bool _damp;
};
