#include <syscfg.h> // Path is project specific.
#include "sdm/sdmDefines.h"
#include "physics/physicsMain.h"
#include "physics/colliderMain.h"
#include "physics/colliderQuery.h"
#include "draw/draw3d.h"

static dSpaceID _odeWorldSpace, _odeVehicleSpace, _odeDynamicSpace, _odeTerrainSpace, _odeTreeSpace;
static dSpaceID _odeTerrainTileSpace, _odeTerrainDetailSpace, _odeTerrainSimpleSpace;

int colliderInit()
{
	_odeWorldSpace = dSimpleSpaceCreate(0);
	//_odeTerrainSpace = dHashSpaceCreate(_odeWorldSpace);
	_odeVehicleSpace = dSimpleSpaceCreate(_odeWorldSpace);
	_odeDynamicSpace = dSimpleSpaceCreate(0); // exists outside of the rest of the world

	_odeTerrainSpace = dSimpleSpaceCreate(_odeWorldSpace);
	_odeTerrainDetailSpace = dHashSpaceCreate(_odeTerrainSpace);
	_odeTerrainSimpleSpace = dSimpleSpaceCreate(_odeTerrainSpace);
	_odeTerrainTileSpace = dSimpleSpaceCreate(_odeTerrainSpace);
	_odeTreeSpace = dHashSpaceCreate(_odeTerrainSpace);

	// collide object owners wil be responsible for object deletion
	dSpaceSetCleanup(_odeWorldSpace, 0);
	dSpaceSetCleanup(_odeTerrainSpace, 0);
	dSpaceSetCleanup(_odeTerrainDetailSpace, 0);
	dSpaceSetCleanup(_odeTerrainSimpleSpace, 0);
	dSpaceSetCleanup(_odeTerrainTileSpace, 0);
	dSpaceSetCleanup(_odeVehicleSpace, 0);
	dSpaceSetCleanup(_odeDynamicSpace, 0);

	// except for the tree space, at least for now
	dSpaceSetCleanup(_odeTreeSpace, 1);

	colliderSetBits((dGeomID)_odeTerrainSpace, COLLIDER_TERRAIN_MASK, 0);
	colliderSetBits((dGeomID)_odeTerrainDetailSpace, COLLIDER_DETAILED_MASK, 0);
	colliderSetBits((dGeomID)_odeTerrainTileSpace, COLLIDER_TILE_MASK, 0);
	colliderSetBits((dGeomID)_odeTerrainSimpleSpace, COLLIDER_SIMPLE_MASK, 0);
	colliderSetBits((dGeomID)_odeTreeSpace, COLLIDER_TRN_TREE, 0);
	colliderSetBits((dGeomID)_odeVehicleSpace, COLLIDER_VEHICLE_MASK, 0);
	colliderSetBits((dGeomID)_odeDynamicSpace, COLLIDER_DYNA_MASK, 0);

	colliderQuery::setDefaultCollideWithSpace(_odeWorldSpace);

	return 0;
}

void colliderKill()
{
	if(_odeVehicleSpace) dSpaceDestroy(_odeVehicleSpace);
	if(_odeTerrainSpace) dSpaceDestroy(_odeTerrainSpace);
	if(_odeTerrainDetailSpace) dSpaceDestroy(_odeTerrainDetailSpace);
	if(_odeTerrainSimpleSpace) dSpaceDestroy(_odeTerrainSimpleSpace);
	if(_odeTerrainTileSpace) dSpaceDestroy(_odeTerrainTileSpace);
	if(_odeDynamicSpace) dSpaceDestroy(_odeDynamicSpace);
	if(_odeWorldSpace)	 dSpaceDestroy(_odeWorldSpace);
	if(_odeTreeSpace) dSpaceDestroy(_odeTreeSpace);
}

dSpaceID colliderTerrainSpace() { return _odeTerrainSpace; }
dSpaceID colliderTerrainDetailSpace() { return _odeTerrainDetailSpace; }
dSpaceID colliderTerrainSimpleSpace() { return _odeTerrainSimpleSpace; }
dSpaceID colliderTerrainTileSpace() { return _odeTerrainTileSpace; }
dSpaceID colliderTreeSpace(){ return _odeTreeSpace; }
dSpaceID colliderVehicleSpace() { return _odeVehicleSpace; }
dSpaceID colliderWorldSpace() { return _odeWorldSpace; }
dSpaceID colliderDynamicSpace(){ return _odeDynamicSpace; }

#include <map>
typedef std::map<std::pair<int,int>, dSpaceID> SpaceMap;

SpaceMap _superCellSpaceMap;
SpaceMap _scenarioSpaceMap;

dSpaceID colliderSpace(dGeomID g)
{
	dSpaceID space = dGeomGetSpace(g);

	if(!space)
	{
		int type = dGeomGetClass(g);

		if(type == dGeomTransformClass)
		{
			dGeomID transform = dGeomTransformGetGeom(g);

			if(transform)
				space = dGeomGetSpace(transform);
		}
	}

	return space;
}

dSpaceID colliderForPosition(FLOAT64VECTOR const& pos)
{
	Vec3d p;
	p.set(pos);
	return colliderForPosition(p);
}

dSpaceID colliderForPosition(Vec3d const& pos)
{
	if(_isnan(pos.x()) || _isnan(pos.y()))
		return nullptr;

	int sx = (int)(pos.x()/SDM_SUPERCELL_SIZE);
	int sy = (int)(pos.y()/SDM_SUPERCELL_SIZE);

	dSpaceID space = nullptr;

	// supercell 0,0 is considered scenario space now
	if(false && !sx && !sy)
	{
		const double SCENARIO_SPACE = 200.0f;

		// break it down to SCENARIO_SPACExSCENARIO_SPACE chunks
		int sx = (int)(pos.x()/SCENARIO_SPACE);
		int sy = (int)(pos.y()/SCENARIO_SPACE);

		space = _scenarioSpaceMap[std::pair<int,int>(sx,sy)];

		if(space == 0)
		{
			dVector3 center, extents;
			extents[0] = SCENARIO_SPACE;
			extents[1] = SCENARIO_SPACE;
			extents[2] = SCENARIO_SPACE;
			center[0] = SCENARIO_SPACE * sx + SCENARIO_SPACE/2.0;
			center[1] = SCENARIO_SPACE * sy + SCENARIO_SPACE/2.0;
			center[2] = 0;

			// create the space;
			space = dQuadTreeSpaceCreate(_odeTerrainDetailSpace, center, extents, 8);

			// objects manage their geoms
			dSpaceSetCleanup(space, 0);

			// initially none, but as geoms are added, bits of sub colliders will be added to the space// COLLIDER_TERRAIN_MASK
			colliderSetBits((dGeomID)space, 0, 0);

			// save it for later use
			_scenarioSpaceMap[std::pair<int,int>(sx,sy)] = space;
		}
	}
	else
	{
		space = _superCellSpaceMap[std::pair<int,int>(sx,sy)];

		if(space == 0)
		{
			dVector3 center, extents;
			extents[0] = SDM_SUPERCELL_SIZE;
			extents[1] = SDM_SUPERCELL_SIZE;
			extents[2] = 3200;
			center[0] = SDM_SUPERCELL_SIZE * sx + SDM_SUPERCELL_SIZE/2.0;
			center[1] = SDM_SUPERCELL_SIZE * sy + SDM_SUPERCELL_SIZE/2.0;
			center[2] = 0;
			space = dQuadTreeSpaceCreate(_odeTerrainDetailSpace,center,extents, 8);
			dSpaceSetCleanup(space, 0);
			colliderSetBits((dGeomID)space, 0, 0); // initially none, but as geoms are added, bits of sub colliders will be added to the space// COLLIDER_TERRAIN_MASK
			_superCellSpaceMap[std::pair<int,int>(sx,sy)] = space;
		}
	}

    // Read in a forum to do this to help reduce initial stutter.
	if(space)
		dSpaceClean(space);

	return space;
}

void colliderSetBits(dGeomID geom, unsigned int cat, unsigned int col)
{
	if(!geom) return;

	dGeomSetCategoryBits(geom, cat);
	dGeomSetCollideBits(geom, col);

	if(dGeomGetClass(geom) == dGeomTransformClass)
		colliderSetBits(dGeomTransformGetGeom(geom), cat, col);

	dSpaceID space = dGeomGetSpace(geom);

	if(space && (dGeomID)space != geom)
		colliderAddBits((dGeomID)space, cat, col);
}

void colliderAddBits(dGeomID geom, unsigned int cat, unsigned int col)
{
	if(!geom) return;

	dGeomSetCategoryBits(geom, dGeomGetCategoryBits(geom) | cat);
	dGeomSetCollideBits(geom, dGeomGetCollideBits(geom) | col);

	if(dGeomGetClass(geom) == dGeomTransformClass)
		colliderAddBits(dGeomTransformGetGeom(geom), cat, col);

	dSpaceID space = dGeomGetSpace(geom);

	if(space && (dGeomID)space != geom)
		colliderAddBits((dGeomID)space, cat, col);
}

#if platOS == OS_WIN32
inline
#endif
 bool colliderIsType(dGeomID geom, unsigned int checkFlags)
{
	return (geom && (dGeomGetCategoryBits(geom) & checkFlags) > 0) ? true: false;
}

#if platOS == OS_WIN32
inline
#endif
 bool colliderIsType(unsigned int catFlags, unsigned int checkFlags)
{
	return ((catFlags & checkFlags) > 0) ? true: false;
}

static const char* _descriptors[] =
{
	"noragdoll",
	"ground",
	"bush",
	"tree",
	"water",
	"building",
	"bridge",
	"cultural",
	"troop",
	"groundvehicle",
	"airvehicle",
	"seavehicle",
	"ammo",
	"fps",
	"corpse",
    "hulk",
	"debris",
	"weapon",
	"capture",
	"nofire",
	"heightmap",
	"walk",
	"enterable",
	"tile",
	"box",
	"sto",
	"area",
	"block",
	"volume"
};

std::vector<std::string> colliderGetDescription(unsigned int flags)
{
	std::vector<std::string> desc;

	if(flags != 0)
	{
		if(flags == COLLIDER_ALL)
		{
			desc.push_back("all");
		}
		else
		{
			for(int c = 0; c < COLLIDER_COUNT; c++)
			{
				if(flags & (1<<c))
					desc.push_back(_descriptors[c]);
			}
		}
	}

	return desc;
}

void colliderDebugContact(Vec3d pos, Vec3d normal, UINT32 color, const char* altName, double scaler)
{
	// physics object line
	FLOAT64VECTOR v1, v2, temp;
	Vec3d pv1, pv2;

	normal.normalize(); // make sure
	normal *= scaler;

	pv1 = pos;
	pv2 = pos + normal;

	pv1 = physicsPhysicalToGame(pv1);
	pv2 = physicsPhysicalToGame(pv2);

	pv1.get(v1);
	v1.z = -v1.z;

	pv2.get(v2);
	v2.z = -v2.z;

	static int count = 0;

	char name[32];

	if(altName)
	{
		if(!color)
			color = colorGREENALPHA;

		draw3dSetWorldDebugLine(&v1, &v2, altName, color);
	}
	else
	{
		snprintf(name, 31, "contact%d", count++);
		if(count > 500) count = 0;

		if(!color)
			color = colorGREENALPHA;

		draw3dSetWorldDebugLine(&v1, &v2, name, color);

		// now we draw the base
		temp = v1;

		v2 = v1;

		v1.x -= .01f;
		v2.x += .01f;

		snprintf(name, 31, "contact%d", count++);
		if(count > 500) count = 0;

		draw3dSetWorldDebugLine(&v1, &v2, name, color);

		v1 = temp;
		v2 = v1;

		v1.y -= .01f;
		v2.y += .01f;

		snprintf(name, 31, "contact%d", count++);
		if(count > 500) count = 0;

		draw3dSetWorldDebugLine(&v1, &v2, name, color);
	}
}

void colliderDebugContact(const colliderQuery::Collision& contact, UINT32 color, const char* altName, double scaler)
{
	colliderDebugContact(contact.pos, contact.normal, color, altName, scaler);
}

void colliderDebugContact(dContact& contact, UINT32 color, const char* altName, double scaler)
{
	Vec3d pos		= Vec3Ode(contact.geom.pos);
	Vec3d normal	= Vec3Ode(contact.geom.normal);

	colliderDebugContact(pos, normal, color, altName, scaler);
}
