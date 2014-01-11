#include <syscfg.h> // Path is project specific.
#include <algorithm>

#include "colliderQuery.h"
#include "colliderMain.h"
#include "physics/odeConvert.h"
#include "physics/physicsmain.h"

dSpaceID colliderQuery::_defaultCollideWithSpace;

colliderQuery::colliderQuery(dSpaceID space, bool useDefaultCollideWithSpace) :
	_space(space),
	_dirty(true),
	_usebbox(true),
	_maxcontacts(32)
{
	// init us
	init(useDefaultCollideWithSpace);

	// no filter by default
	_filter[0] = _filter[1] = _filter[2] = 0.0f;
	_filtered = false;
}

colliderQuery::colliderQuery(dGeomID geom, unsigned int cat, unsigned int col, dSpaceID space, bool useDefaultCollideWithSpace) :
	_space(space),
	_dirty(true),
	_usebbox(true),
	_maxcontacts(32)
{
	// init us
	init(useDefaultCollideWithSpace);

	// add our geom, we own this geom
	addGeom(geom, cat, col);

	// no filter by default
	_filter[0] = _filter[1] = _filter[2] = 0.0f;
	_filtered = false;
}

colliderQuery::~colliderQuery()
{
	dGeomDestroy(_bbox);
	dSpaceDestroy(_space);
}

void colliderQuery::init(bool useDefaultCollideSpace)
{
	if(_space == 0)
	{
		_space = dSimpleSpaceCreate(0);
		dSpaceSetCleanup(_space, 1);
		colliderSetBits((dGeomID)_space, 0, 0);
	}

	_bbox = dCreateGeomTransform(0);
	dGeomTransformSetInfo(_bbox, 1);
	dGeomTransformSetGeom(_bbox, dCreateBox(0,0,0,0));

	if(useDefaultCollideSpace)
		addGeomToCollideWith((dGeomID)_defaultCollideWithSpace);
}

void colliderQuery::addGeomToCollideWith(dGeomID geom)
{
	if(geom)
	{
		_geomsToCollide.push_back(geom);
		_dirty = true;
	}
}

void colliderQuery::setGeomToCollideWith(dGeomID geom)
{
	_geomsToCollide.clear();
	addGeomToCollideWith(geom);
}

dGeomID colliderQuery::addGeom(dGeomID g, unsigned int cat, unsigned int col)
{
	dSpaceID current = dGeomGetSpace(g);

	// move the geom to my space
	if(current != _space)
	{
		if(current)
			dSpaceRemove(current, g);

		dSpaceAdd(_space, g);
	}

	// save the geom
	_myGeoms.push_back(g);

	// set the geom's bits
	colliderSetBits(g, cat, col);

	// return our added geom
	return g;
}

void colliderQuery::resetBits()
{
	// clear our geom's bits
	setBits(0, 0);

	// clear our space's bits
	colliderSetBits((dGeomID)_space, 0, 0);
}

void colliderQuery::setBits(unsigned int cat, unsigned int col)
{
	if(!_myGeoms.empty())
	{
		for(int i = 0; i < (int)_myGeoms.size(); i++)
			colliderSetBits(_myGeoms[i], cat, col);
	}
	else
	{
		colliderSetBits((dGeomID)_space, cat, col);
	}
}

void colliderQuery::addBits(unsigned int cat, unsigned int col)
{
	if(!_myGeoms.empty())
	{
		for(int i = 0; i < (int)_myGeoms.size(); i++)
			colliderAddBits(_myGeoms[i], cat, col);
	}
	else
	{
		colliderAddBits((dGeomID)_space, cat, col);
	}
}

void colliderQuery::sort(bool reverse)
{
	_collisions.sortByDepthAndGeom();

	if(reverse)
		std::reverse(_collisions.begin(), _collisions.end());
}

void colliderQuery::updateBBox()
{
	dReal aabb[6];
	dGeomGetAABB((dGeomID)_space, aabb);

	dReal lx = aabb[1] - aabb[0];
	dReal ly = aabb[3] - aabb[2];
	dReal lz = aabb[5] - aabb[4];
	dReal x = (aabb[1] + aabb[0])/2.0;
	dReal y = (aabb[3] + aabb[2])/2.0;
	dReal z = (aabb[5] + aabb[4])/2.0;

	// make sure we have a valid bbox
	if(lx <= 0 || _isnan(lx) || !_finite(lx))
		lx = .01f;

	if(ly <= 0 || _isnan(ly) || !_finite(ly))
		ly = .01f;

	if(lz <= 0 || _isnan(lz) || !_finite(lz))
		lz = .01f;

	if(_isnan(x) || !_finite(x))
		x = 0;

	if(_isnan(y) || !_finite(y))
		y = 0;

	if(_isnan(z) || !_finite(z))
		z = 0;

	dGeomSetPosition(_bbox, x,y,z);
	dGeomBoxSetLengths(dGeomTransformGetGeom(_bbox), lx,ly,lz);
}

bool colliderQuery::findCollisionsWithBBox(dGeomID geom)
{
	size_t n = _collisions.size();
	dSpaceCollide2 (_bbox, geom, this, bboxCollisionCallback);
	return _collisions.size() != n;
}

bool colliderQuery::findCollisions(bool force)
{
	if(!_dirty)
	{
		if(!force)
			return _collisions.size() != 0;
	}

	_collisions.clear();
	bool collided = false;
	dGeomList::iterator it = _geomsToCollide.begin();

	if(_usebbox)
	{
		updateBBox();
		for(; it != _geomsToCollide.end(); ++it)
			collided |= findCollisionsWithBBox(*it);
	}
	else
	{
		for(; it != _geomsToCollide.end(); ++it)
			collided |= findCollisions(*it);
	}

	_dirty = false;

	return collided;
}

colliderQuery::Collision::Collision(dContactGeom const& contact)
{
	pos = Vec3Ode(contact.pos);
	normal = Vec3Ode(contact.normal);
	depth  = contact.depth;
    triindex = contact.side2;
	g1 = contact.g1;
	g2 = contact.g2;
}

// Alpha collider
int colliderQuery::TriRayCallback(dGeomID TriMesh, dGeomID Ray, int TriangleIndex, dReal u, dReal v)
{
    return 0;
}

int colliderQuery::TriCallback(dGeomID TriMesh, dGeomID RefObject, int TriangleIndex)
{
    return 1;
}

void colliderQuery::TriArrayCallback(dGeomID TriMesh, dGeomID RefObject, const int* TriIndices, int TriCount)
{

}

// Is the normal facing the starting point of the ray?
bool colliderQuery::normalFacingStartPoint(dGeomID g1, const Vec3d& normal)
{
    Vec3d start, end;

    // Get start point of original ray used for the collision
    if(osgGeomRayGet(g1, start, end))
    {
        Vec3d dir = start - end;

        // Get a normalize vector
		dir.normalize();
        //osgMath::fastNormalize(dir);

        // Check with collision normal to see if the hit is facing starting point.
        // Some collisions may have the same time, so they do not sort correctly. (i.e., on signs)
        if((dir * normal) < 0)
        {
            return false;
        }

        return true;
    }

    return false;
}

void colliderQuery::bboxCollisionCallback (void* cookie, dGeomID o1, dGeomID o2)
{
	if(dGeomIsSpace(o1) || dGeomIsSpace(o2))
	{
		dSpaceCollide2 (o1, o2, cookie, bboxCollisionCallback);
		return;
	}

	colliderQuery* query = (colliderQuery*)cookie;
	dGeomID g = (o1 == query->_bbox) ? o2 : o1;

	dSpaceCollide2((dGeomID)query->space(), g, cookie, collisionCallback);
}

void colliderQuery::collisionCallback(dGeomID o1, dGeomID o2)
{
	dContactGeom contacts[32];
	int numc = dCollide (o1, o2, _maxcontacts, &contacts[0], sizeof(dContactGeom));

	if(numc > 0)
	{
		dSpaceID geomSpace = dGeomGetSpace(o2);

		for (int i = 0; i < numc; i++)
		{
			// if we're filtered make sure this contact's normal is within 90 degrees of the new contact's normal
			if(!_filtered || dDOT(_filter, contacts[i].normal) >= 0)
			{
				Collision collision(contacts[i]);

				// won't work if in subspace?
				if(geomSpace == _space)
				{
					// swap so query space objects are always first
					std::swap(collision.g1, collision.g2);
					collision.normal = -collision.normal;

					// depth for rays is always the distance along the ray to the hit
					// so we need to adjust the depth as well if we're going to swap it here
					//if(dGeomGetClass(collision.g1) == dRayClass)
					//	collision.depth = dGeomRayGetLength(collision.g1) - collision.depth;
				}

				_collisions.push_back(collision);
			}
		}
	}
}

void colliderQuery::setNormalFilter(double x, double y, double z)
{
	_filtered = false;

	if(x || y || z)
	{
		_filtered = true;

		_filter[0] = x;
		_filter[1] = y;
		_filter[2] = z;

		// make sure we're normalized
		dNormalize3(_filter);
	}
}

bool colliderQuery::isNormalFiltered(dVector3 normal)
{
	if(dDOT(_filter, normal) >= 0.0f)
		return true;

	return false;
}

void colliderQuery::disable()
{
	if(!_myGeoms.empty())
	{
		for(int i = 0; i < (int)_myGeoms.size(); i++)
			dGeomDisable(_myGeoms[i]);
	}
}

void colliderQuery::collisionCallback (void* cookie, dGeomID o1, dGeomID o2)
{
	if(dGeomIsSpace(o1) || dGeomIsSpace(o2))
	{
		dSpaceCollide2 (o1, o2, cookie, collisionCallback);
		return;
	}

	((colliderQuery*)cookie)->collisionCallback(o1,o2);
}

bool colliderQuery::findCollisions(dGeomID geom)
{
	size_t n = _collisions.size();
	dSpaceCollide2 ((dGeomID)_space, geom, this, collisionCallback);
	return _collisions.size() != n;
}

colliderQuery::CollisionList& colliderQuery::getCollisions(bool sorted)
{
	if(sorted)
		_collisions.sortByDepthAndGeom();

	return _collisions;
}

bool colliderQuery::getCollisions(CollisionList& collisions) const
{
	collisions = _collisions;
	return !collisions.empty();
}

bool colliderQuery::getCollisions(dGeomID geom, CollisionList& collisions, bool sorted) const
{
	CollisionList::const_iterator it = _collisions.begin();

	for(; it != _collisions.end(); ++it)
	{
		if(it->g1 == geom || it->g2 == geom)
			collisions.push_back(*it);
	}

	if(sorted && !collisions.empty())
		collisions.sortByDepthAndGeom();

	return !collisions.empty();
}

static bool sortByDepthAndGeom(colliderQuery::Collision const& a, colliderQuery::Collision const& b)
{
	if(a.g1 != b.g1) return (a.g1 < b.g1);

	return  (a.depth < b.depth);
}

void colliderQuery::CollisionList::sortByDepthAndGeom()
{
	std::sort(begin(), end(), ::sortByDepthAndGeom);
}

colliderRay::colliderRay(double l, double px, double py, double pz, double dx, double dy, double dz) :
	colliderQuery(0, true)
{
	Vec3d pos(px, py, pz);
	Vec3d dir(dx, dy, dz);

	addGeom(dCreateRay(_space, l), 0, 0);
	dGeomRaySet(_myGeoms.at(0), pos.x(), pos.y(), pos.z(), dir.x(), dir.y(), dir.z());
}

colliderRay::colliderRay(double l, Vec3d& p, Vec3d& d) :
	colliderQuery(0, true)
{
	addGeom(dCreateRay(_space, l), 0, 0);
	dGeomRaySet(_myGeoms.at(0), p.x(), p.y(), p.z(), d.x(), d.y(), d.z());
}

colliderRay::~colliderRay()
{
	// _geom is cleaned up by colliderQuery killing the space
}

void colliderRay::set(double l, Vec3d& p, Vec3d& d)
{
	setMaxContactsPerHit(2);
	dGeomRaySetLength(_myGeoms.at(0), l);
	dGeomRaySet(_myGeoms.at(0), p.x(), p.y(), p.z(), d.x(), d.y(), d.z());
	setDirty(true);
}

void colliderRay::set(double l, double px, double py, double pz, double dx, double dy, double dz)
{
	setMaxContactsPerHit(2);
	dGeomRaySetLength(_myGeoms.at(0), l);
	dGeomRaySet(_myGeoms.at(0), px, py, pz, dx, dy, dz);
	setDirty(true);
}

const colliderQuery::Collision* colliderRay::getCollision()
{
	if(!_collisions.empty())
	{
		_collisions.sortByDepthAndGeom();
		return &_collisions.at(0);
	}

	return nullptr;
}

const colliderQuery::CollisionList& collideRayList(unsigned int bits,
                                                   double length,
                                                   Vec3d& start,
                                                   Vec3d& dir,
												   bool filter)
{
	static colliderRay ray;

	// set the ray info
	ray.resetBits();
	ray.setBits(0, bits);
	ray.set(length, start, dir);

	if(filter)
	{
		Vec3d nf = dir * -1.0f;
		ray.setNormalFilter(nf.x(), nf.y(), nf.z());
	}
	else
	{
		ray.setNormalFilter(0, 0, 0);
	}

	// shoot the ray
	ray.findCollisions(true);

	// return the results
	return ray.getCollisions(true);
}

float collideRay(unsigned int bits,
                 double length,
                 Vec3d& start,
                 Vec3d& dir,
				 bool filter)
{
	const colliderQuery::CollisionList& collisions = collideRayList(bits, length, start, dir, filter);
	return (!collisions.empty()) ? (float)collisions.at(0).depth: 0.0f;
}

float collideRay(unsigned int bits, double length,
					double sx, double sy, double sz,
					double dx, double dy, double dz,
					bool gamePos)
{
	Vec3d start(sx, sy, sz);
	Vec3d dir(dx, dy, dz);

	if(gamePos)
		start = physicsGameToPhysical(start);

	return collideRay(bits, length, start, dir);
}
