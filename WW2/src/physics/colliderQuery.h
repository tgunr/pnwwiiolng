#pragma once

#include <vector>
#include <ode/ode.h>
#include "math/Vec3d.h"

class colliderQuery
{
public:
	// takes ownership of space passed in, if 0 it creates its own space
	// and owns geoms added unless you change by dSpaceSetCleanup
	// normal usage is to call with default arguments and pass space()
	// when creating geometry you want to collide
	colliderQuery(dSpaceID space=0, bool useDefaultCollideWithSpace=true);
	colliderQuery(dGeomID geom, unsigned int cat, unsigned int col, dSpaceID space=0, bool useDefaultCollideWithSpace=true);
	virtual ~colliderQuery();

    colliderQuery( const colliderQuery & rhs ) { }
    
	dGeomID addGeom(dGeomID g, unsigned int cat, unsigned int col);
	void addGeomToCollideWith(dGeomID);
	void setGeomToCollideWith(dGeomID);
	bool findCollisions(bool force = false);
	void setDirty(bool d) { _dirty = d;}
	void setUseBoundingBox(bool b) { _usebbox = b; }
	void setMaxContactsPerHit(int m) { _maxcontacts = (m > 0 && m <= 32) ? m: _maxcontacts; }

	//virtual methods
	virtual void resetBits();
	virtual void setBits(unsigned int cat, unsigned int col);
	virtual void addBits(unsigned int cat, unsigned int col);

	virtual void sort(bool reverse = false);

	struct Collision
	{
		Collision(dContactGeom const&);
		Vec3d pos;
		Vec3d normal;
		double depth;
        int triindex;

		// g1 is colliderQuery geometry, g2 is geom collided with
		// use (colliderGeom*)dGeomGetData(collision.g2) to get colliderGeom object
		dGeomID g1, g2;
	};

	class CollisionList : public std::vector<Collision>
	{
	public:
		void sortByDepthAndGeom();
	};

	using dGeomList = std::vector<dGeomID>;

	colliderQuery::CollisionList& getCollisions(bool sorted = true); // all contacts
	bool getCollisions(CollisionList&) const;
	bool getCollisions(dGeomID geom, CollisionList& collisions, bool sorted = false) const;

	dSpaceID space() const { return _space; }
	dGeomID geom(int i) { return (!_myGeoms.empty() && i >= 0 && i < (int)_myGeoms.size()) ? _myGeoms.at(i): 0; }
	colliderQuery::dGeomList& geoms() { return _myGeoms; }
	void disable();
    void clearGeomsToCollideWith() {_geomsToCollide.clear();}

	static void collisionCallback(void* cookie, dGeomID o1, dGeomID o2);
	static void bboxCollisionCallback(void* cookie, dGeomID o1, dGeomID o2);
    static int TriRayCallback(dGeomID TriMesh, dGeomID Ray, int TriangleIndex, dReal u, dReal v);
    static int TriCallback (dGeomID TriMesh, dGeomID RefObject, int TriangleIndex);
    static void TriArrayCallback(dGeomID TriMesh, dGeomID RefObject, const int* TriIndices, int TriCount);

	void collisionCallback(dGeomID o1, dGeomID o2);

	void setNormalFilter(double x = 0.0f, double y = 0.0f, double z = 0.0f);
	bool isNormalFiltering(){ return _filtered; }
	bool isNormalFiltered(dVector3 normal);

	static void setDefaultCollideWithSpace(dSpaceID s) { _defaultCollideWithSpace = s; }
    static bool normalFacingStartPoint(dGeomID g1, const Vec3d& normal);

protected:

	void init(bool useDefaultCollideSpace);

	bool findCollisions(dGeomID);
	bool findCollisionsWithBBox(dGeomID);
	void updateBBox();

	static dSpaceID _defaultCollideWithSpace;
	dSpaceID _space;
	CollisionList _collisions;
	dGeomList _geomsToCollide;
	dGeomList _myGeoms; // geoms this query object owns; use of this list is optional
	bool _dirty, _usebbox;
	dGeomID _bbox;
	int _maxcontacts;
	dVector3 _filter;
	bool _filtered;
};

class colliderRay : public colliderQuery
{
public:

	colliderRay(double l = 1, double px = 0, double py = 0, double pz = 0, double dx = 0, double dy = 1, double dz = 0);
	colliderRay(double l, Vec3d& p, Vec3d& d);
	virtual ~colliderRay();

	void set(double l, Vec3d& p, Vec3d& d);
	void set(double l, double px, double py, double pz, double dx, double dy, double dz);

	const colliderQuery::Collision* getCollision(); // closest contact
};

extern const colliderQuery::CollisionList& collideRayList(unsigned int bits, double length, Vec3d& start, Vec3d& dir, bool filter = false);
extern float collideRay(unsigned int bits, double length, Vec3d& start, Vec3d& dir, bool filter = false);
extern float collideRay(unsigned int bits, double length, double sx, double sy, double sz, double dx = 0.0f, double dy = 0.0f, double dz = -1.0f, bool gamePos = true);
