#include <syscfg.h> // Path is project specific.

#include "veh/vehmain.h"
#include "veh/vehanim.h"
#include "veh/vehupdate.h"
#include "ob/obdshad.h"
#include "veh/vehload.h"
#include "ob/obMgr.h"
#include "ob/obdshad.h"
#include "veh/vehdecal.h"
#include "fx/TransformEffect.h"
#include "pers/persmain.h"

// move this to obmain when we get the chance
void obmainPrintObject(OBJECT* object, int ident=0);

#include <stack>
using OBJECT_DATA_POOL = std::stack<mlObjectData*>;
static OBJECT_DATA_POOL _objectDataPool;

/************************************************************************************************
 mlObjectData::Get() and mlObjectData::Release()
 Manage the pool of previously used objects.
*************************************************************************************************/

mlObjectData*
mlObjectData::Get()
{
	if(_objectDataPool.empty())
		return new mlObjectData;

	mlObjectData* p = _objectDataPool.top();
	_objectDataPool.pop();

    p->Reset();

    return p;
}

void mlObjectData::Release(mlObjectData* object)
{
	assert( object != nullptr );
	_objectDataPool.push(object);
}

void mlObjectData::Reset()
{
    PlaneObject = nullptr;
    TrackSection = nullptr;
    TrackAnimation = nullptr;
    HitMap = nullptr;
    HitBox = nullptr;
    CollideExternObject = nullptr;
    CollideInternObject = nullptr;
    CollideWalkObject = nullptr;
    EffectsObject = nullptr;
    MassObject = nullptr;
	VisDamageGroup = nullptr;
	VisDamageCrapGroup = nullptr;
	VisDamageStubGroup = nullptr;
	VisDamageHulkLOD = nullptr;
	LineCollideObject = nullptr;
	CollidePntList = nullptr;
	LoadoutLODObject = nullptr;
	WheelRadius = nullptr;
	TowObject = nullptr;
	ShadowObject = nullptr;
	HoistObject = nullptr;
	HoistHitBox = nullptr;
	HoistLineCollideObject = nullptr;
	HoistCollidePntList = nullptr;
    spawnObject = nullptr;

	TrackCount = 0;
	TrackAnimationCount = 0;
	NumCollideObjects = 0;
	TowType = 0;
    GearUpCount = 0;
	NumFPSSeats = 0;
	NumVehSeats = 0;
    GearDownCount = 0;
    FlapsUpCount = 0;
    FlapsDownCount = 0;
	NumHoistCollideObjects = 0;
    ProcessContactPointObject = 0;

    SavedMinLODLoadoutRange = 0.0f;
	SavedMaxLODLoadoutRange = 0.0f;
	Radius = 0.0f;
	LowestPoint = 0.0f;
	HighestPoint = 0.0f;

    BBox1.set(0, 0, 0);
    BBox2.set(0, 0, 0);

    memset(LODObjects, 0, sizeof(OBJECT*) * ML_MAX_LODS);
	memset(FPSSeats, 0, sizeof(OBJECT*) * ML_MAX_FPS_SEATS);
	memset(WeaponFirePoints, 0, sizeof(OBJECT*) * VEH_MAX_MOUNTS * 4);
	memset(WeaponConnectPoints, 0, sizeof(OBJECT*) * VEH_MAX_MOUNTS * 4);
	memset(VehSeats, 0, sizeof(OBJECT*) * ML_MAX_VEH_SEATS);
	memset(GearUpGroup, 0, sizeof(OBJECT*) * ML_MAX_GEARUPDOWN_GROUPS);
	memset(GearDownGroup, 0, sizeof(OBJECT*) * ML_MAX_GEARUPDOWN_GROUPS);
	memset(FlapsUpGroup, 0, sizeof(OBJECT*) * ML_MAX_FLAPSUPDOWN_GROUPS);
	memset(FlapsDownGroup, 0, sizeof(OBJECT*) * ML_MAX_FLAPSUPDOWN_GROUPS);
	memset(EngineFastGroup, 0, sizeof(OBJECT*) * ML_MAX_ENGINES * ML_MAX_ENGINEFASTSLOW_GROUPS);
	memset(EngineSlowGroup, 0, sizeof(OBJECT*) * ML_MAX_ENGINES * ML_MAX_ENGINEFASTSLOW_GROUPS);
	memset(FireDirectorObj, 0, sizeof(OBJECT*) * ML_MAX_CREW_POSITIONS);
	memset(TurretConvObj, 0, sizeof(OBJECT*) * ML_MAX_CREW_POSITIONS * VEH_MAX_MOUNTS);
	memset(FirePoint, 0, sizeof(OBJECT*) * ML_MAX_CREW_POSITIONS * VEH_MAX_MOUNTS);

    memset(SavedMinLODRange, 0, sizeof(float) * ML_MAX_LODS);
    memset(SavedMaxLODRange, 0, sizeof(float) * ML_MAX_LODS);

    memset(TraverseReversed, 0, sizeof(LOGICAL) * ML_MAX_CREW_POSITIONS);

    memset(EngineFastCount, 0, sizeof(UINT32) * ML_MAX_ENGINES);
    memset(EngineSlowCount, 0, sizeof(UINT32) * ML_MAX_ENGINES);
}

// Not important unless debugging for memory errors. Free up memory on kill to remove
// superfluous logging
void vehloadKill()
{
    mlObjectData* p = nullptr;

    while(!_objectDataPool.empty())
    {
        p = _objectDataPool.top();
        _objectDataPool.pop();

        delete p;
    }
}

/************************************************************************************************
	vehloadCreateVehicleInstance
*************************************************************************************************/
void vehloadCreateVehicleInstance(vehVehicle* vehicle, LOGICAL hasCockpit)
{
	assert(vehicle && !vehicle->VehicleObjectData);

	vehicle->VehicleObjectData = mlObjectData::Get();
	vehicle->VehicleObjectData->PlaneObject = obmgrCreateModelInstance((char*)vehicle->VehicleType->Shape);

	mlInitVehicleObject(&vehicle->VehicleObjectData, vehicle->VehicleType , hasCockpit);

#ifdef USE_SHADOWS
	vehloadCreateShadow(vehicle);
#endif

	//obAdjustObjectTreeVisListStatus(vehicle->VehicleObjectData->PlaneObject,1);
	vehAnimResetVehicleStateAnimations(vehicle);
    //Initialize weapon mounts
    if(!hasCockpit)
    {
        vehupdateInitGhostFiring(vehicle);
    }
    //vehupdateCreateVehicleLoadObjectPortion(vehicle, 0);

	// players don't need ode colliders on other vehicles if they are an aircraft
	vehVehicle* playerVeh = persGetPersonaVehiclePtr();
	if(playerVeh && playerVeh->VehicleType->Type != VEH_TYPE_AIR)
		vehmainCreateODECollider(vehicle);

    if(vehicle->LoadVehicle)
    {
        vehdecalSetDecals(vehicle);
    }

    // Do this piecemeal until new system complete.
    switch(vehicle->VehicleType->Type)
    {
    case VEH_TYPE_AIR:
    case VEH_TYPE_GROUND:
    case VEH_TYPE_SEA:
        {
            TransformEffect::Apply(vehicle);
            break;
        }
    }

	vehicle->LoadVehicle = 0;
//	obmainPrintObject(vehicle->VehicleObjectData->PlaneObject);
}

#ifdef USE_SHADOWS
/************************************************************************************************
	vehloadCreateShadow
*************************************************************************************************/
void vehloadCreateShadow(struct vehVehicle* veh)
{
	OBJECT* lpobj;
	OBJECT* lpgrp = veh->VehicleObjectData->PlaneObject;

	lpobj = obmainFindChildFromObject(lpgrp, "shadow");
	if (lpobj)
		obdshadCreate(lpgrp, lpobj);
}
#endif

// move this to obmain when we get the chance
#include <iostream>

static void printTab(int ident)
{
	for(int i=0; i < ident;i++)
		std::cout << '\t';
}

void obmainPrintObject(OBJECT* object, int ident)
{
	printTab(ident);
	std::cout <<  object->name;

	std::cout << "\n";
	if(object->child)
	{
		printTab(ident); std::cout << "{\n";
		obmainPrintObject(object->child,ident+1);
		printTab(ident); std::cout << "}\n";
	}

	if(object->sibling)
		obmainPrintObject(object->sibling,ident);
}
