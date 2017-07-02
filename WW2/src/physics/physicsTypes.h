#pragma once

#include <vector>
#include <set>
#include "ode/ode.h"
#include "ode/odecpp.h"
#include "physics/odeConvert.h"
#include "physics/physicsmain.h"
#include "physics/colliderMain.h"

class physicsModel;
class physicsBodyBase;
class physicsNodeCollider;
class physicsObjectCollider;

using physicsBodySet = std::set<physicsBodyBase*>;

class physicsObject
{
public:

	virtual void BeginStep() = 0;
	virtual void EndStep() = 0;
	virtual bool ProcessFlag(unsigned int flag, physicsObject* o = nullptr){ return false; }
	virtual void OnRemove() {}
};

class physicsModel : public physicsObject
{
public:

	physicsModel() : active(false), killed(false)
	{
	}

	virtual ~physicsModel()
	{
	}

	virtual void BeginStep();
	virtual void EndStep();
	virtual bool IsActive();
	virtual void Activate();
	virtual void Deactivate();
	virtual bool IsOwned(){ return false; }
	virtual void Kill(){ killed = true; }

protected:

	bool active, killed;
	physicsBodySet bodies;

};

class physicsBodyBase : public physicsObject
{
public:

	static dWorldID DefaultWorld();

	physicsBodyBase();
	virtual ~physicsBodyBase();

	// class based accessors
	void SetPosition(Vec3d const &, bool game = true);
	void SetRotation(Matrixd const&);
	void SetRotation(Quat const&);
	void SetLinearVel(Vec3d const&);
	void SetAngularVel(Vec3d const&);
	void SetTransform(Matrixd const&);
	Vec3d& GetPosition(bool game = true) const;
	Matrixf GetRotationMatrix() const;
	Quat GetRotation() const;
	Vec3d GetLinearVel() const;
	Vec3d GetAngularVel() const;
	Matrixd GetTransform(bool game = true) const;

	// struct based accessors
	void setPosition(FLOAT64VECTOR const&);
	void setLinearVel(FLOAT64VECTOR const&);
	void setAngularVel(FLOAT64VECTOR const&);
	void setRotation(FLOAT64QUAT const&);
	void setAttitude(FLOAT64ATT const&);
	FLOAT64VECTOR getPosition() const;
	FLOAT64VECTOR getLinearVel() const;
	FLOAT64ATT getAttitude() const;
	FLOAT64VECTOR getAngularVel() const;
	void setModelPosition(FLOAT64VECTOR const&);
	FLOAT64VECTOR getModelPosition() const;

	void addForce(const FLOAT64VECTOR& F);
	void addForce(double x, double y, double z);
	void addTorque(const FLOAT64VECTOR& T);
	void addTorque(double x, double y, double z);

	// overrides for subclasses
	virtual void Collide(dGeomID bodyGeom, dGeomID geom){}
	virtual int InitCollisions(){ return 0; }
	virtual void BeginStep(){ applyForces(); }
	virtual void EndStep(){}

	bool IsEnabled() const
	{
		return (_body) ? _body->isEnabled(): false;
	}

	void Expire()
	{
		SetExpired(true);
	}

	bool Expired() const
	{
		return _expired;
	}

	virtual void SetExpired(bool e)
	{
		_expired = e;

		if(_body)
		{
			if(Expired())
			{
				dBodyDisable(_body->id());
			}
			else
			{
				dBodyEnable(_body->id());
			}
		}
	}

	void SetOwner(void* o)
	{
		_owner = o;
	}

	void* GetOwner() const
	{
		return _owner;
	}

	bool IsOwned() const
	{
		return (_owner) ? true: false;
	}

	bool SetEnabled(bool e)
	{
		if(_body && e != (bool)_body->isEnabled())
		{
			if(e) dBodyEnable(_body->id());
			else dBodyDisable(_body->id());
			return true;
		}

		return false;
	}

	dBody* Body(){ return _body; }

protected:

	void applyForces();

	dBody* _body;
	dVector3 _lastFreePos;
	dVector3 _prevPos;
	dVector3 _cm;
	FLOAT64VECTOR _force, _torque;
	bool _expired;
	void* _owner;
};

class physicsBody : public physicsBodyBase
{
public:

	physicsBody();
	physicsBody(UINT32 cat, UINT32 col);
	physicsBody(dSpaceID s, UINT32 cat, UINT32 col);
	virtual ~physicsBody();

	virtual bool handleContact(dContact*);

	void setRestitution(float);
	float getRestitution() const;

	void setMass (double m, FLOAT64VECTOR const& cm
				, double Ixx, double Iyy, double Izz
				, double Ixy, double Ixz, double Iyz
				, struct OBJECT* object );

	void setMassFromMesh(double density, FLOAT64VECTOR const& cm);

	void makeCapsuleColliderFromObject(struct OBJECT*, FLOAT32 scaler = 1.0f);	// aligned with z
	void makeBoxColliderFromObject(struct OBJECT*, FLOAT32 scaler = 1.0f);
	void makeMeshColliderFromObject(struct OBJECT*, FLOAT64VECTOR* offset = nullptr);

	dMass const* getMass() const;

	virtual void Collide(dGeomID bodyGeom, dGeomID geom);
	virtual void BeginStep();
	virtual void EndStep();

	virtual int InitCollisions();
	virtual void AdjustNormalFilter(){}

	std::vector<dContact> const& collisionContacts() { return _contacts; }
	void setEnhancedCollisionCheck(bool b) { _enhancedCollisionCheck = b;}
	void setCollisionContacts(int c);

	physicsObjectCollider* getCollider() { return _geom; }

	// convenience function for single geom bodies
	dGeomID geom(int index = 0);

	// these functions aren't finalized yet and will probably result in anomalies
	void TransformToOBJECT(OBJECT* o);
	void TransformFromOBJECT(OBJECT* o);

protected:

	std::vector<dContact> _contacts;

	void damp();
	void recurseAndMakeMeshCollider(struct OBJECT*, bool siblings=false);
	dGeomID rayCheck(); // returns the thing we hit
	dGeomID preCheck(); // returns the thing we hit

	Vec3d _velocity;
	FLOAT64 _speed; // only updated every _nextUpdate
	Vec3d _contactsNormal;
	colliderQuery* collider;
	dJointGroup* collideJoints;
	physicsObjectCollider* _geom;
	dMass _mass;
	float _restitution;
	bool _enhancedCollisionCheck;
	UINT32 _collided;
	dGeomID _thisContact; // what we're currently in contact with
	dGeomID _lastContact; // last thing we hit
	SINT32 _nextUpdate; // next time we update enhanced collision data
	UINT32 _allHits; // what we have hit in total (collider flags)
};

// base class for actual colliders
class physicsCollider
{
protected:

	struct geomMesh
	{
		geomMesh() : geom(0), mesh(0) {}
		geomMesh(dGeomID g, dTriMeshDataID m) : geom(g), mesh(m) {}

		void destroy()
		{
			if(geom) dGeomDestroy(geom);
			if(mesh) dGeomTriMeshDataDestroy(mesh);
			geom = 0;
			mesh = 0;
		}

		dGeomID geom;
		dTriMeshDataID mesh;
	};

public:
	physicsCollider(UINT32 cat = COLLIDER_ALL) :
		_space(0),
		_categoryMask(cat),
		_collideMask(COLLIDER_VOID)
	{
	}

	virtual ~physicsCollider()
	{
		for(std::vector<geomMesh>::iterator it = _geom.begin(); it != _geom.end(); ++it)
			it->destroy();

		_geom.clear();
	}

	// sub classes override for some specific source object type
	//virtual int create(void* o, piMATRIX* transform = nullptr){ return -1;}

	int createMesh(const void* Vertices, int VStride, int VCount, const void* Indices, int IStride, int ICount, void* userdata, piMATRIX* transform = nullptr);
	int createCapsule(float radius, float len);
	int createSphere(float radius);
	int createBox(float lx, float ly, float lz);
	int createCylinder(float radius, float length);
	int addGeom(dGeomID geom);

	void attachToBody(dBody*);
	void offsetPosition(double,double,double);
	void offsetRotation(const dMatrix3);

	virtual void updateTransform(piMATRIX const&);
	virtual void updatePosition(float x, float y, float z); // from object->mat if need be

	void setSpace(dSpaceID space){ _space = space; }
	void setCollideMask(unsigned int mask);
	void setCategoryMask(unsigned int mask);

	UINT32 getCollideMask(){ return _collideMask; }
	UINT32 getCategoryMask(){ return _categoryMask; }
	dSpaceID getSpace(){ return _space; }

	bool isCollidable(UINT32 cflags);

	int geoms() const { return _geom.size(); }
	void setEnabled(bool e);

	dGeomID id(int index = 0)
	{
		if(index >= 0 && index < (int)_geom.size())
			return _geom.at(index).geom;

		return nullptr;
	}

	static UINT32 DetermineCategory(UINT32 cat, const char* path, const char* comments = nullptr, int texid = -1);
	static UINT32 DetermineCategory(UINT32 cat, OBJECT* o);

protected:

	dSpaceID _space;
	std::vector<geomMesh> _geom;
	unsigned int _categoryMask, _collideMask;
};

class physicsNodeCollider : public physicsCollider
{
public:
	physicsNodeCollider(struct PoolModel* m, struct PoolNode* n, UINT32 cat) : physicsCollider(cat),
		node(n)
	{
		create(n);
	}

	virtual ~physicsNodeCollider()
	{
	}

	virtual int create(struct PoolNode* n, piMATRIX* transform = nullptr);

	// the poolnode we represent
	struct PoolNode* node;
};

class physicsObjectCollider : public physicsCollider
{
public:
	physicsObjectCollider(dSpaceID space, UINT32 cat = COLLIDER_ALL, UINT32 col = COLLIDER_ALL);
	virtual ~physicsObjectCollider();

	virtual int create(OBJECT* o, piMATRIX* transform = nullptr);
};

// this type of collider wraps a pool node collider
class physicsTransformCollider : public physicsObjectCollider
{
public:
	physicsTransformCollider(dSpaceID space, UINT32 cat = COLLIDER_ALL, UINT32 col = COLLIDER_ALL);
	virtual ~physicsTransformCollider();

	virtual int create(OBJECT* o, piMATRIX* transform = nullptr);

protected:

	// the node collider we wrap
	physicsNodeCollider* _nodeCollider;
};

// this type of collider wraps a pool node collider
class physicsShearedCollider : public physicsObjectCollider
{
public:
	physicsShearedCollider(dSpaceID space, UINT32 cat = COLLIDER_ALL, UINT32 col = COLLIDER_ALL);
	virtual ~physicsShearedCollider();

	virtual void updateTransform(piMATRIX const&);
	virtual int create(OBJECT* o, piMATRIX* transform = nullptr);

protected:

	// the node collider we wrap
	float* _vertices;
};
