#pragma once

#include <vector>
#include <set>
#include "ode/ode.h"
#include "ode/odecpp.h"
#include "physics/odeConvert.h"
#include "physics/physicsTypes.h"
#include "physics/colliderMain.h"

class physicsObject;
class physicsModel;
class physicsBodyBase;
class physicsCollider;

typedef std::set<physicsObject*> physicsObjectSet;
typedef std::set<physicsModel*> physicsModelSet;

typedef physicsObject* (PHYSICS_MODEL_FILTER) (physicsObject*, void* const);

extern void physicsInit();
extern void physicsUpdate(int ms);
extern void physicsKill();

extern dWorld* physicsGetWorld();
extern int physicsAddWorldForce(const FLOAT64VECTOR& pos, FLOAT64 energy);
extern void physicsSetWorldForce(int index, FLOAT64 energy);
extern Vec3d& physicsGetWorldForce(const FLOAT64VECTOR& pos, bool randFallback = true);
extern void physicsAddObject(physicsObject*);
extern void physicsRemoveObject(physicsObject*, bool now = false);
extern void physicsRemoveAllObjects(bool includeOwned = false);
extern physicsObject* physicsFindObject(PHYSICS_MODEL_FILTER* filter, void* const data = nullptr);
extern bool physicsFindObject(physicsObjectSet& list, PHYSICS_MODEL_FILTER* filter, void* const data = nullptr);

extern void physicsSetStepSize(float);
extern void physicsSetTimeMultiplier(float);
extern void physicsSetWorldOffset(FLOAT64VECTOR const&);
extern const FLOAT64VECTOR& physicsGetWorldOffset();
extern double physicsGetCFM();
extern double physicsGetERP();

extern Vec3d physicsPhysicalToGame(const Vec3d& pos);
extern Vec3d physicsGameToPhysical(const Vec3d& pos);

// node colliders
struct PoolModel;
struct PoolNode;
extern int physicsRecurseAndCreateNodeCollider(PoolModel* model, PoolNode* node, bool siblings = false, bool onlyVisible = true);
extern int physicsCreateNodeCollider(PoolModel* model, PoolNode* node);
extern void physicsRecurseAndToggleNodeCollider(PoolNode* node, bool enabled, bool siblings = false);
extern void physicsRecurseAndFreeNodeCollider(PoolNode* node, bool siblings = true);
extern void physicsFreeNodeCollider(PoolNode* node);

// object colliders
extern dSpaceID physicsRecurseAndCreateObjectCollider(struct OBJECT*, dSpaceID space, unsigned int cat, unsigned int col
											, bool siblings, bool onlyVisible, bool createSpace, void* spaceData = nullptr);
extern void physicsRecurseAndFreeObjectCollider(OBJECT*, bool siblings=false);
extern void physicsRecurseAndToggleObjectCollider(OBJECT*, bool enabled, bool siblings=false);
extern void physicsCreateObjectCollider(OBJECT*, dSpaceID, unsigned int cat, unsigned int col);
extern void physicsFreeObjectCollider(OBJECT*);

extern FLOAT64QUAT QuatFromAxisAndAngle(FLOAT64VECTOR const& axis, double angle);
