#pragma once

#include "colliderQuery.h"

#define SPACE_DATA(s,t) ((s) ? ((t)*)dGeomGetData((dGeomID)(s)): NULL)

extern int  colliderInit();
void colliderKill();

extern dSpaceID colliderSpace(dGeomID g);
extern dSpaceID colliderWorldSpace();

// terrain spaces

// parent terrain space
extern dSpaceID colliderTerrainSpace();
extern dSpaceID colliderTreeSpace();

// child terrain spaces
extern dSpaceID colliderTerrainDetailSpace(); // detailed manmade terrain parent space
extern dSpaceID colliderTerrainSimpleSpace(); // for simple collision primitives (fast colliding)
extern dSpaceID colliderTerrainTileSpace(); // replacement tiles

extern dSpaceID colliderVehicleSpace();
extern dSpaceID colliderDynamicSpace();
extern dSpaceID colliderForPosition(Vec3d const&);
extern dSpaceID colliderForPosition(FLOAT64VECTOR const&);

// collider bits; can set optional child to the same bits
extern void colliderSetBits(dGeomID geom, unsigned int cat, unsigned int col);
extern void colliderAddBits(dGeomID geom, unsigned int cat, unsigned int col);
extern bool colliderIsType(dGeomID geom, unsigned int checkFlags); // cat is the category below
extern bool colliderIsType(unsigned int catFlags, unsigned int checkFlags); // cat a the category below

// debug
extern std::vector<std::string> colliderGetDescription(unsigned int flags);
void colliderDebugContact(Vec3d pos, Vec3d normal, UINT32 color = 0, const char* altName = NULL, double scaler = .1f);
void colliderDebugContact(const colliderQuery::Collision& contact, UINT32 color = 0, const char* altName = NULL, double scaler = .1f);
void colliderDebugContact(dContact& contact, UINT32 color = 0, const char* altName = NULL, double scaler = .1f);

template<class T>
T* colliderData(dGeomID g)
{
	if(!g)
		return NULL;

	return (T*)dGeomGetData(g);
}

// you can pass in a dSpaceID..simply cast it as a dGeomID
template<class T>
T* colliderSpaceData(dGeomID g)
{
	if(!g)
		return NULL;

	dSpaceID space = (dGeomIsSpace(g)) ?  (dSpaceID)g: colliderSpace(g);

	if(space)
		return (T*)dGeomGetData((dGeomID)space);

	return NULL;
}

// if you add to this list, make sure and update the final enumeration COLLIDER_COUNT and
// add a friendly descriptor to the bottom of colliderMain.cpp > _descriptors for debug purposes
enum
{
	COLLIDER_VOID			= 0,			// nothing
	COLLIDER_NORAGDOLL		= 1<<0,		// don't ragdoll here
	COLLIDER_TRN_GROUND		= 1<<1,		// earth
	COLLIDER_TRN_BUSH		= 1<<2,		// non-tree foliage
	COLLIDER_TRN_TREE		= 1<<3,
	COLLIDER_TRN_WATER		= 1<<4,
	COLLIDER_TRN_BUILDING	= 1<<5,		// buildings
	COLLIDER_TRN_BRIDGE		= 1<<6,		// bridges
	COLLIDER_TRN_CULTURAL	= 1<<7,		// man made objects
	COLLIDER_VEH_TROOP		= 1<<8,
	COLLIDER_VEH_GROUND		= 1<<9,
	COLLIDER_VEH_AIR		= 1<<10,
	COLLIDER_VEH_SEA		= 1<<11,
	COLLIDER_AMMO			= 1<<12,	// active in flight ammo
	COLLIDER_FPS			= 1<<13,	// fps infantry collider for movement
	COLLIDER_CORPSE			= 1<<14,    // duh
    COLLIDER_VEH_HULK		= 1<<15,
	COLLIDER_DEBRIS			= 1<<16,
	COLLIDER_WEAPON			= 1<<17,
	COLLIDER_CAPTURE		= 1<<18,
	COLLIDER_NOFIRE			= 1<<19,
	COLLIDER_HEIGHTMAP		= 1<<20,
	COLLIDER_WALK			= 1<<21,
	COLLIDER_ENTERABLE		= 1<<22,	// anything you can get in or under (disallow grass or rain in these areas)
	COLLIDER_TILE			= 1<<23,	// a terrain replacement tile
	COLLIDER_BOX			= 1<<24,	// any enterable manmande object (bbox where stuff can't be placed)
	COLLIDER_STO			= 1<<25,	// sto objects
	COLLIDER_AREA			= 1<<26,	// collider type used to differentiate this collider as one that detects other collider types
	COLLIDER_BLOCK			= 1<<27,	// a collidable block (currently only used with scenarios)
	COLLIDER_VOLUME			= 1<<28,	// volume collider for scenario play (I know similar to block...a block simply blocks players)
	COLLIDER_COUNT			= 29		// total number of collider categories (not including the complex groups below)
};

// all colliders
#define COLLIDER_ALL 0xffffffff

// any manmade terrain object
#define COLLIDER_MANMADE_MASK			COLLIDER_TRN_BUILDING|COLLIDER_TRN_BRIDGE|COLLIDER_TRN_CULTURAL

// anything you can walk on
#define COLLIDER_HARDTERRAIN_MASK		COLLIDER_TRN_GROUND|COLLIDER_TRN_TREE|COLLIDER_MANMADE_MASK|COLLIDER_VEH_HULK

// all non-replacement non-manmade terrain objects
#define COLLIDER_GROUND_MASK			COLLIDER_TRN_GROUND|COLLIDER_TRN_BUSH|COLLIDER_TRN_TREE|COLLIDER_TRN_WATER

// reaplcement tiles
#define COLLIDER_TILE_MASK				COLLIDER_TRN_GROUND|COLLIDER_TRN_WATER|COLLIDER_TILE

// what goes in the blank space
#define COLLIDER_SIMPLE_MASK			COLLIDER_BOX|COLLIDER_BLOCK

// all terrain
#define COLLIDER_TERRAIN_MASK			COLLIDER_HARDTERRAIN_MASK|COLLIDER_GROUND_MASK|COLLIDER_TILE_MASK|COLLIDER_SIMPLE_MASK

// detailed terrain objects
#define COLLIDER_DETAILED_MASK			COLLIDER_HARDTERRAIN_MASK|COLLIDER_TRN_BUSH|COLLIDER_TRN_WATER|COLLIDER_ENTERABLE

// drivable vehicle mask
#define COLLIDER_DRIVABLE_MASK			COLLIDER_VEH_GROUND|COLLIDER_VEH_AIR|COLLIDER_VEH_SEA

// game vehicles
#define COLLIDER_VEHICLE_MASK			COLLIDER_DRIVABLE_MASK|COLLIDER_VEH_TROOP

// grass, bushes, trees
#define COLLIDER_FOLIAGE_MASK			COLLIDER_TRN_BUSH|COLLIDER_TRN_TREE

// things ammo can hit
#define COLLIDER_AMMO_MASK				COLLIDER_VEHICLE_MASK|COLLIDER_HARDTERRAIN_MASK|COLLIDER_TRN_WATER|COLLIDER_CORPSE

// things we want the FPS system to know about
#define COLLIDER_FPS_MASK				COLLIDER_VEHICLE_MASK|COLLIDER_HARDTERRAIN_MASK|COLLIDER_TRN_WATER
#define COLLIDER_CAGE_MASK				COLLIDER_VEHICLE_MASK|COLLIDER_HARDTERRAIN_MASK|COLLIDER_TILE

// things a ragdoll interacts with
#define COLLIDER_CORPSE_MASK			COLLIDER_DRIVABLE_MASK|COLLIDER_HARDTERRAIN_MASK|COLLIDER_TRN_WATER|COLLIDER_WEAPON

#define COLLIDER_MOVE_MASK				COLLIDER_VEHICLE_MASK|COLLIDER_HARDTERRAIN_MASK|COLLIDER_TRN_WATER
#define COLLIDER_DEPLOY_MASK			COLLIDER_TRN_GROUND|COLLIDER_MANMADE_MASK
#define COLLIDER_DYNA_MASK				COLLIDER_AMMO|COLLIDER_DEBRIS
#define COLLIDER_WEAPON_MASK			COLLIDER_DRIVABLE_MASK|COLLIDER_HARDTERRAIN_MASK|COLLIDER_TRN_WATER|COLLIDER_CORPSE
#define COLLIDER_CONTACT_MASK			COLLIDER_HARDTERRAIN_MASK|COLLIDER_DRIVABLE_MASK
