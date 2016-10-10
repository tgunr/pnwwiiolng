#include <syscfg.h> // Path is project specific.
#include <set>
#include <map>

#include "pi/pi.h"
#include "math/mathEul.h"
#include "ob/obmain.h"
#include "math/mathvect.h"
#include "math/math3d.h"
#include "trn/trnmain.h"
#include "trn/trnvars.h"
#include "physics/physicsMain.h"
#include "gr2/gr2Physics.h"
#include "fp/fpTPSPhysics.h"
#include "sdm/sdm.h"
#include "physics/odeConvert.h"
#include "ui/uiChat.h"
#include "spt/Random.h"
#include "ob/obMgr.h"
#include "trn/trnwtr.h"
#include "ml/mlutility.h"
#include "draw/draw3d.h"
#include "ob/obpool.h"
#include "scen/ScenarioManager.h"

struct WorldForce
{
	Vec3d position;
	FLOAT64 radius; // radius at which you are affected
	FLOAT64 minRadius; // min radius threshold
	FLOAT64 minForce;
	FLOAT64 maxForce;
	SINT32 expires;
};

using WorldForces = std::vector<WorldForce>;

static dWorld* _odeWorld;
static double _odeStepSize;
static int _odeLastUpdateTime;
static double _timeMultiplier;
static physicsObjectSet _objects;
static physicsObjectSet _garbage;
static double _erp, _cfm;
static FLOAT64VECTOR _worldoffset;
static WorldForces _worldForces;
static bool physicsInited = false;

#define DEBUG_ODE_VERTEX_SHEARING
#undef DEBUG_ODE_VERTEX_SHEARING

#ifdef DEBUG_ODE_VERTEX_SHEARING
static unsigned int _totalVsize = 0;
#endif //DEBUG_ODE_VERTEX_SHEARING

void physicsSetStepSize(double stepsize) { _odeStepSize = stepsize; }
void physicsSetTimeMultiplier(double mult) { _timeMultiplier = mult; }
void physicsSetWorldOffset(FLOAT64VECTOR const& offset)
{
	_worldoffset.x = offset.x;
	_worldoffset.y = offset.y;
	_worldoffset.z = 0;
}

const FLOAT64VECTOR& physicsGetWorldOffset()
{
	return _worldoffset;
}

Vec3d physicsPhysicalToGame(const Vec3d& pos)
{
	Vec3d to(pos);

	to.x() += _worldoffset.x;
	to.y() += _worldoffset.y;
	to.z() += _worldoffset.z;

	return to;
}

Vec3d physicsGameToPhysical(const Vec3d& pos)
{
	Vec3d to(pos);

	to.x() -= _worldoffset.x;
	to.y() -= _worldoffset.y;
	to.z() -= _worldoffset.z;

	return to;
}

double physicsGetCFM()
{
	return _cfm;
}

double physicsGetERP()
{
	return _erp;
}


static void odeCalcConstraints(double kp, double kd, double h, double& erp, double& cfm)
{
	erp = .9f;//1 / (1 + kd/(h*kp));
	cfm = .0001f; //1 / (h*kp + kd);
}

void physicsInit()
{
	dInitODE();

	_odeWorld = new dWorld;
	_odeWorld->setGravity(0,0,-9.8);

	_odeLastUpdateTime = 0;

	_timeMultiplier = 1.0f;
	_odeStepSize = .0125f;
	odeCalcConstraints(50000,1000,_odeStepSize, _erp, _cfm);

	dWorldSetContactMaxCorrectingVel (*_odeWorld, 100);
	dWorldSetContactSurfaceLayer (*_odeWorld, 0.01);

	//dWorldSetQuickStepNumIterations (*_odeWorld, 10);
	//dWorldSetAutoEnableDepthSF1(*_odeWorld, 2);
	dWorldSetAutoDisableFlag (*_odeWorld, 1);
	dWorldSetAutoDisableLinearThreshold (*_odeWorld, .1);
	dWorldSetAutoDisableAngularThreshold (*_odeWorld, .1);
	dWorldSetAutoDisableSteps (*_odeWorld, 5);

	colliderInit();
	//gr2PhysicsInit();
	fpTPSRagdollInit();

    physicsInited = true;
}

void physicsAddObject(physicsObject* o)
{
	_objects.insert(o);
}

void physicsRemoveObject(physicsObject* o, bool now)
{
	if(now)
	{
		_objects.erase(o);
		o->OnRemove();
	}
	else
	{
		_garbage.insert(o);
	}
}

void physicsRemoveAllObjects(bool includeOwned)
{
    _objects.clear();
}

physicsObject* physicsFindObject(PHYSICS_MODEL_FILTER filter, void* const data)
{
	if(filter)
	{
		physicsObjectSet::iterator it;
		physicsObject* found = nullptr;

		for( it = _objects.begin() ; it != _objects.end(); ++it)
		{
			found = filter(*it, data);
			if(found) return found;
		}
	}

	return nullptr;
}

bool physicsFindObject(physicsObjectSet& list, PHYSICS_MODEL_FILTER filter, void* const data)
{
	if(filter)
	{
		physicsObjectSet::iterator it;
		physicsObject* found = nullptr;

		for(it = _objects.begin(); it != _objects.end(); ++it)
		{
			found = filter(*it, data);
			if(found) list.insert(*it);
		}
	}

	return !list.empty();
}

#define WFORCE_DEFAULT 7.0f // meters per second
#define WFORCE_DURATION 3000.0f // milliseconds

int physicsAddWorldForce(const FLOAT64VECTOR& pos, FLOAT64 energy)
{
	WorldForce force;

	force.position.set(pos);

	if(energy <= 1000000.0f) // 2 million (grenades, small explosives)
	{
		force.radius = 20.0f;		// meters
		force.minRadius = 15.0f;	// meters
		force.minForce = 8.0f;	// meters per second
		force.maxForce = 18.0f;	// meters per second
	}
	else if(energy <= 200000000.0f) // 200 million (small bombs)
	{
		force.radius = 30.0f;			// meters
		force.minRadius = 20.0f;		// meters
		force.minForce = 15.0f;		// meters per second
		force.maxForce = 40.0f;	// meters per second
	}
	else // every thing else (big bombs)
	{
		force.radius = 50.0f;			// meters
		force.minRadius = 35.0f;	// meters
		force.minForce = 20.0f;		// meters per second
		force.maxForce = 50.0f;	// meters per second
	}

	force.expires = frametime + (int)WFORCE_DURATION;

	_worldForces.push_back(force);

	return (int)_worldForces.size();
}

void physicsSetWorldForce(int index, FLOAT64 energy)
{
	//if(index >= 0 && index < (int)_worldForces.size())
	//	_worldForces.at(index).energy = energy;
}

Vec3d& physicsGetWorldForce(const FLOAT64VECTOR& pos, bool randFallback)
{
	static Vec3d push;
	push.set(0, 0, 0);

	Vec3d p;
	p.set(pos);

	if(!_worldForces.empty())
	{
		Vec3d diff;
		WorldForces::iterator it = _worldForces.begin();

		for(; it != _worldForces.end(); ++it)
		{
			if(frametime < it->expires)
			{
				WorldForce& force = *it;

				diff = p - force.position;

				if(diff.length2() < force.radius * force.radius)
				{
					// move it to around the chest area
					diff.z() += 1.5f;

					// tmod gets less the further away in time we are from the force event
					float tmod = 1.0f; //(float)(force.expires - frametime) / (float)WFORCE_DURATION;

					// dmod gets less the further away in distance we are from the force event
					float dmod = 1.0f - ((float)diff.length() / (float)force.minRadius);

					// clamp it
					if(dmod < 0)
						dmod = 0;

					// calculate mod
					double mod = (double)((tmod + (dmod*2.0f)) / 3.0f); // dmod weighted more

					// calculate force scaling
					double scale = (double)(force.minForce + ((force.maxForce - force.minForce) * mod));

					// clamp scale
					if(scale > force.maxForce)
						scale = force.maxForce;

					diff.normalize();
					diff *= scale;

					// if its greater, use it
					if(diff.length2() > push.length2())
						push = diff;
				}
			}
		}
	}

	if(push.length2())
	{
		// push us up for dramatic explosions
		float len = (float)push.length();
		push.z() += GetRandom(len/2.0f, len);

		if(SCEN_MGR->IsActive())
		{
			static float dmod = 5.0f;
			push *= dmod;
		}
	}
	else if(randFallback)
	{
		// return a random value
		float force = WFORCE_DEFAULT;
		push.set(GetRandom(-force, force), GetRandom(-force, force), GetRandom(-2.0f, 1.0f));
	}

	//push *= 20.0f;

	return push;
}

/*
static void odeCollisionCallback (void* cookie, dGeomID o1, dGeomID o2)
{
	if(dGeomIsSpace(o1) || dGeomIsSpace(o2))
	{
		dSpaceCollide2 ((dGeomID)o1, (dGeomID)o2, cookie, odeCollisionCallback);
		return;
	}
	dBodyID b1 = dGeomGetBody(o1);
	dBodyID b2 = dGeomGetBody(o2);

	if ((b1 && b2) || (!b1 && !b2)) return;

	physicsBody* pbody = (physicsBody*)dBodyGetData(b1 ? b1 : b2);
	if(pbody)
		pbody->Collide(b1 ? o1 : o2, b1 ? o2 : o1);
}
*/

void physicsUpdate(int frametime)
{
	static int _lastTime = 0;

	if(frametime - _lastTime > 5000)
	{
		/*
		SpaceMap::iterator it =  _superCellSpaceMap.begin();
		for(; it != _superCellSpaceMap.end(); ++it)
		{
			if(dSpaceGetNumGeoms(it->second) < 1)
			{
				dSpaceDestroy(it->second);
				_superCellSpaceMap.erase(it);
				break;
			}
		}
		*/

		// clear out world forces every 5 seconds
		if(!_worldForces.empty())
		{
			WorldForces::iterator it = _worldForces.begin();

			while(it != _worldForces.end())
			{
				if(it->expires >= frametime)
					it = _worldForces.erase(it);
				else
					++it;
			}
		}

#ifdef DEBUG_ODE_VERTEX_SHEARING
		if(_totalVsize)
			uiChatPrintLine("Total memory allocated for sheered vertices is %u bytes", _totalVsize);
#endif // DEBUG_ODE_VERTEX_SHEARING

		_lastTime = frametime;
	}

	double dt = (frametime - _odeLastUpdateTime) / 1000.0 * _timeMultiplier;

	// inits after half a second from the first frametime update
	if(dt > .5)
	{
		_odeLastUpdateTime = frametime;
		dt = 0;
	}

	int n = (int)(dt / _odeStepSize);

	if(n && (!_objects.empty() || !_garbage.empty()))
    {
		dt /= n;		// reduce dt from the frame delta to the step delta

		/*
		static int _lastUpdateCheck = 0;

		if(!_objects.empty() && frametime > _lastUpdateCheck)
		{
			uiChatPrintLine("Processing %d physics objects %d times!", (int)_objects.size(), n);
			_lastUpdateCheck = frametime + 1000;
		}
		*/

		for(int i = 0; i < n; i++)
		{
			physicsObjectSet::iterator it;

			for( it = _objects.begin() ; it != _objects.end(); ++it)
					(*it)->BeginStep();

			dWorldQuickStep(_odeWorld->id(), _odeStepSize);

			for( it = _objects.begin() ; it != _objects.end(); ++it)
					(*it)->EndStep();

			// clean up garbage, if needed
			for( it = _garbage.begin() ; it != _garbage.end(); ++it)
					physicsRemoveObject(*it, true);

			_garbage.clear();
		}
	}

	_odeLastUpdateTime += (int)(n * _odeStepSize * 1000.0 / _timeMultiplier);

	// update our gr2Physics objects
	//gr2PhysicsUpdate();
}

void physicsKill()
{
    if(!physicsInited)
        return;

    if(!_objects.empty())
	    _objects.clear();

    if(_odeWorld)
	    delete _odeWorld;

	//fpTPSRagdollKill();
	colliderKill();
	dCloseODE();
}

dWorld* physicsGetWorld()
{
	return _odeWorld;
}

static bool _validMaterial(const char* name)
{
	if(!strcasecmp(name, "clouds"))
		return false;
	else if(!strcasecmp(name, "metalrail"))
		return false;

	return true;
}

// node colliders
int physicsRecurseAndCreateNodeCollider(PoolModel* model, PoolNode* node, bool siblings, bool onlyVisible)
{
	int created = 0;

	if(!onlyVisible || (!(node->flags & OBJECT_NOTOBJECT) && (node->flags & OBJECT_VISIBLE)))
	{
		created += physicsCreateNodeCollider(model, node);
	}
	else if(node->collider)
	{
		physicsFreeNodeCollider(node);
	}

	if(node->attached)
		created += physicsRecurseAndCreateNodeCollider(model, node->attached, false, onlyVisible);

	if(node->child)
		created += physicsRecurseAndCreateNodeCollider(model, node->child, true, onlyVisible);

	if(siblings && node->sibling)
		created += physicsRecurseAndCreateNodeCollider(model, node->sibling, true, onlyVisible);

	return created;
}

int physicsCreateNodeCollider(PoolModel* model, PoolNode* node)
{
    if(node->collider || !node->objdata || node->objdata->numtris < 1)
		return 0;

	node->collider = new physicsNodeCollider(model, node, 0);
	return 1;
}

void physicsRecurseAndToggleNodeCollider(PoolNode* node, bool enabled, bool siblings)
{
	if(node->collider)
		node->collider->setEnabled(enabled);

	if(node->attached)
		physicsRecurseAndToggleNodeCollider(node->attached, enabled, false);

	if(node->child)
		physicsRecurseAndToggleNodeCollider(node->child, enabled, true);

	if(siblings && node->sibling)
		physicsRecurseAndToggleNodeCollider(node->sibling, enabled, true);
}

void physicsRecurseAndFreeNodeCollider(PoolNode* node, bool siblings)
{
	if(node->collider)
		physicsFreeNodeCollider(node);

	if(node->attached)
		physicsRecurseAndFreeNodeCollider(node->attached, false);

	if(node->child)
		physicsRecurseAndFreeNodeCollider(node->child, true);

	if(siblings && node->sibling)
		physicsRecurseAndFreeNodeCollider(node->sibling, true);
}

void physicsFreeNodeCollider(PoolNode* node)
{
	if(node->collider)
	{
		delete node->collider;
		node->collider = 0;
	}
}

dSpaceID physicsRecurseAndCreateObjectCollider(OBJECT* Object, dSpaceID parentSpace, unsigned int cat, unsigned int col,
							bool siblings, bool onlyVisible, bool createSpace, void* spaceData)
{
	dSpaceID space = createSpace ? dSimpleSpaceCreate(parentSpace) : parentSpace;

	if(createSpace)
	{
		dSpaceSetCleanup(space, 0);
		colliderSetBits((dGeomID)space, cat, col);

		if(spaceData)
			dGeomSetData((dGeomID)space, spaceData);
	}

	if(!onlyVisible || (!(Object->flags & OBJECT_NOTOBJECT) && (Object->flags & OBJECT_VISIBLE) && !(Object->flag2 & OBJECT_MISSING)))
	{
		physicsCreateObjectCollider(Object, space, cat, col);
	}
	else if(Object->collider)
	{
		delete Object->collider;
		Object->collider = 0;
	}

	if(Object->attached)
		physicsRecurseAndCreateObjectCollider(Object->attached, space, cat, col, false, onlyVisible, false);

	if(Object->child)
		physicsRecurseAndCreateObjectCollider(Object->child, space, cat, col, true, onlyVisible, false);

	if(siblings && Object->sibling)
		physicsRecurseAndCreateObjectCollider(Object->sibling, space, cat, col, true, onlyVisible, false);

	return space;
}

void physicsRecurseAndFreeObjectCollider(OBJECT* Object, bool siblings)
{
	if(Object->collider)
		physicsFreeObjectCollider(Object);

	if(Object->attached)
		physicsRecurseAndFreeObjectCollider(Object->attached, false);

	if(Object->child)
		physicsRecurseAndFreeObjectCollider(Object->child, true);

	if(siblings && Object->sibling)
		physicsRecurseAndFreeObjectCollider(Object->sibling, true);
}

void physicsRecurseAndToggleObjectCollider(OBJECT* Object, bool enabled, bool siblings)
{
	if(Object->collider)
		Object->collider->setEnabled(enabled);

	if(Object->attached)
		physicsRecurseAndToggleObjectCollider(Object->attached, false);

	if(Object->child)
		physicsRecurseAndToggleObjectCollider(Object->child, true);

	if(siblings && Object->sibling)
		physicsRecurseAndToggleObjectCollider(Object->sibling, true);
}

#define OBJECT_COLLIDER_DEBUG 1
#undef OBJECT_COLLIDER_DEBUG

static inline bool _createPoolCollider(OBJECT* o)
{
	if(g_buildTerrain || !o->modelNode || o->modelNode->collider)
		return false;

	if(o->modelInstance->IsShearPlacement() || !o->modelInstance->IsStaticTerrainModel())
		return false;

	return true;
}

void physicsCreateObjectCollider(OBJECT* Object, dSpaceID space, unsigned int cat, unsigned int col)
{
	if(Object->collider || Object->numtris < 1 || !_validMaterial(Object->texname) || (Object->flag2 & OBJECT_SPEEDTREE))
		return;

	ModelInstance* instance = Object->modelInstance;

	if(instance && _createPoolCollider(Object))
		physicsCreateNodeCollider(instance->GetModel(), Object->modelNode); // slow path 1/3

	cat = physicsCollider::DetermineCategory(cat, Object);

	if(cat == COLLIDER_VOID)
		return;

	bool isSheared = (instance && instance->IsShearPlacement());

	// are we really sheared?
	//if((Object->mat._13 + Object->mat._23) == 0) // mat(0,2) + mat(1,2)
	//	isSheared = false;

#ifdef OBJECT_COLLIDER_DEBUG
	static OBJECT* lo = nullptr;

	OBJECT* check = (Object->modelInstance) ? Object->modelInstance->rootObject: Object;

	bool newOb = (lo != check);

	lo = check;
#endif

	if(isSheared)
	{
		//if(colliderIsType(cat, COLLIDER_MANMADE_MASK))
		//	isSheared = false;
	}

	// sheared game colliders now have mutated (sheared) vertices in them so we no longer need the sheared collider
	// it would mutate the vertices itself
	/*
	if(isSheared)
	{
#ifdef OBJECT_COLLIDER_DEBUG
		if(newOb && lo && lo->name)
			uiChatPrintLine("New Shear Collider for %s", lo->name);
#endif

		Object->collider = new physicsShearedCollider(space, cat, col);
	}
	else
	*/

	// we can't use sheared objects with transform colliders because the shear is per instance
	if(!isSheared && Object->modelNode && Object->modelNode->collider)
	{
#ifdef OBJECT_COLLIDER_DEBUG
		if(newOb && lo && lo->name)
			uiChatPrintLine("New Transform Collider for %s", lo->name);
#endif

		Object->collider = new physicsTransformCollider(space, cat, col);
	}
	else
	{
#ifdef OBJECT_COLLIDER_DEBUG
		if(newOb && lo && lo->name)
			uiChatPrintLine("New OBJECT Collider for %s", lo->name);
#endif

		Object->collider = new physicsObjectCollider(space, cat, col);
	}

	Object->collider->create(Object, &Object->mat); // other slow path 2/3

#ifdef OBJECT_COLLIDER_DEBUG
	unsigned int scat = dGeomGetCategoryBits((dGeomID)space);
	unsigned int scol = dGeomGetCollideBits((dGeomID)space);

	std::vector<std::string> scatFlags = colliderGetDescription(scat);
	std::vector<std::string> scolFlags = colliderGetDescription(scol);

	int gcount = Object->collider->geoms();

	for(int g = 0; g < gcount; g++)
	{
		dGeomID id = Object->collider->id(g);

		std::vector<std::string> catFlags = colliderGetDescription(dGeomGetCategoryBits(id));
		std::vector<std::string> colFlags = colliderGetDescription(dGeomGetCollideBits(id));
	}
#endif

	/*
	if(Object->physicsGeom->geoms())
	{
		Object->physicsGeom->setCategoryMask(categoryMask);
		Object->physicsGeom->setCollideMask(collideMask);
	}
	*/
}

void physicsFreeObjectCollider(OBJECT* Object)
{
	if(Object->collider)
	{
		delete Object->collider;
		Object->collider = 0;
	}
}

FLOAT64QUAT QuatFromAxisAndAngle(FLOAT64VECTOR const& axis, double angle)
{
	dQuaternion q;
	dQFromAxisAndAngle (q, axis.x, axis.y,axis.z, angle);

	FLOAT64QUAT quat;
	quat.x = q[0]; quat.y = q[1]; quat.z = q[2]; quat.w = q[3];
	return quat;
}

/*
dSpaceID physicsSpaceForPosition(FLOAT64VECTOR const& pos)
{
	int sx = (int)(pos.x/SDM_SUPERCELL_SIZE);
	int sy = (int)(pos.y/SDM_SUPERCELL_SIZE);

	dSpaceID space = _superCellSpaceMap[std::pair<int,int>(sx,sy)];
	if(space == 0)
	{
		dVector3 center, extents;
		extents[0] = SDM_SUPERCELL_SIZE;
		extents[1] = SDM_SUPERCELL_SIZE;
		extents[3] = 3000;
		center[0] = SDM_SUPERCELL_SIZE * sx + SDM_SUPERCELL_SIZE/2.0;
		center[1] = SDM_SUPERCELL_SIZE * sy + SDM_SUPERCELL_SIZE/2.0;
		center[2] = 0;
		space = dQuadTreeSpaceCreate(_odeStaticSpace,center,extents, 4);
		dSpaceSetCleanup(space, 0);
		_superCellSpaceMap[std::pair<int,int>(sx,sy)] = space;
	}

	return space;
}
*/

// pass in a world oriented position
/*
bool physicsRayCollide(double sx, double sy, double sz, double dx, double dy, double dz, float l, FLOAT64VECTOR& p)
{
	if(!_rayCollider) _rayCollider = dCreateRay(_odeDynamicSpace, l);

	// adjust for
	sx -= _worldoffset.x;
	sy -= _worldoffset.y;
	sz -= _worldoffset.z;

	dVector3 pos;
	pos[0] = sx;
	pos[1] = sy;
	pos[2] = sz;

	// set the ray params
	dGeomRaySet(_rayCollider, sx, sy, sz, dx, dy, dz);

	const int MAX_CONTACTS = 16;
	dContact contacts[MAX_CONTACTS];

	dContact* hit	= 0;
	double minlen2	= 100000.0;

	int numc = dCollide (_rayCollider, (dGeomID)_odeStaticSpace, MAX_CONTACTS, &contacts[0].geom, sizeof(dContact));
	numc += dCollide (_rayCollider, (dGeomID)_odeVehicleSpace, MAX_CONTACTS-numc, &contacts[numc].geom, sizeof(dContact));

	// find the closest hit
	for (int i = 0; i < numc; i++)
	{
		dContact* contact = &contacts[i];
		dVector3 delta;

		dOP(delta,-,contact->geom.pos,pos);

		double len2 = dLENGTHSQUARED(delta);

		if(len2 < minlen2)
		{
			// weird edge case flip the normal so it's in the "right" direction
			if(dDOT(delta, contact->geom.normal) > 0)
			{
				contact->geom.normal[0] = -contact->geom.normal[0];
				contact->geom.normal[1] = -contact->geom.normal[1];
				contact->geom.normal[2] = -contact->geom.normal[2];
			}

			minlen2 = len2;
			hit = contact;
		}
	}

	if(hit)
	{
		p.x = hit->geom.pos[0] + _worldoffset.x;
		p.y = hit->geom.pos[1] + _worldoffset.y;
		p.z = hit->geom.pos[2] + _worldoffset.z;
		return true;
	}

	return false;
}
*/

