// main gr2 module

/*
	 GR2 System Core
	 Copyright 2001 Playnet Inc, All rights reserved
	 Not to be distributed in API form except as part of a compiled library
	 under License or NDA

	Originating Programmer : Troy Lowe
	Maintaining Programmer :
	Supporting Programmer  :

	Rev 1  c2001
*/

#include "syscfg.h"
#include "include/systypes.h"
#include "mem/mem.h"

#include "gr2/gr2Main.h"
#include "gr2/gr2Data.h"
#include "gr2/gr2Vehicle.h"
#include "mech/MechVehicle.h"
#include "mech/MechRocketPack.h"

#include "pers/persmain.h"
#include "ml/mltypes.h"
#include <math.h>
#include "math/mathvect.h"
#include "math/math3d.h"
#include "math/matheul.h"
#include "include/vars.h"
#include "trn/trnmain.h"
#include "ml/mlutility.h"
#include "draw/draw3d.h"
#include "weap/weapons.h"
#include "fp/fpFPS.h"
#include "fp/fpTypes.h"
#include "fp/fpMain.h"
#include "veh/vehmain.h"
#include "fx/FXInterface.h"
#include "fio/fio.h"
#include "pers/persgunsight.h"
#include "fx/fxAmb.h"
#include "gr2/gr2Anim.h"

#define GR2VEHICLE_STAND_EYE_HEIGHT 7.0f // meters

using VehicleList = std::vector<gr2Vehicle*>;
using VehicleMap = std::map<vehVehicle*,gr2Vehicle*>;
using ObjectMap = std::map<OBJECT*,gr2Vehicle*>;

// list of active vehicles
static VehicleList _vehicles;

// lookup maps
static VehicleMap _vehicleMap;
static ObjectMap _objectMap;

// calls the vehicle's draw function
static LOGICAL _gr2DrawCallback(OBJECT* pObject);

LOGICAL gr2MainInit()
{
	gr2AnimInit(); // this makes sure some gr2 stuff is setup before the vehicles get inited
	gr2InitData();

	// initialize vehicle creators
	gr2Vehicle::RegisterCreator("mech", &MechVehicle::CreateMech);

	// initialize weapon creators
	//gr2Weapon::RegisterCreator("rocketpack", &MechRocketPack::CreateWeapon);

	return 1;
}

LOGICAL gr2MainUpdate(FLOAT64 DeltaTime)
{
	//for(int i = 0; i < (int)_vehicles.size(); i++)
	//	_vehicles[i]->Update();

	return 1;
}

// kill all vehicle resources
LOGICAL gr2MainKill()
{
	gr2KillData();
	return 1;
}

LOGICAL gr2InitVehicle(vehVehicle* v)
{
	gr2MainInit();

	gr2Vehicle* vehicle = gr2GetVehicle(v);
	if(vehicle) return vehicle->Init();

	return 0;
}


LOGICAL gr2UpdateVehicle(vehVehicle* v, SINT32 TicTime)
{
	gr2Vehicle* vehicle = gr2GetVehicle(v);
	if(!vehicle) return FALSE;

	if(vehicle->IsFirstPerson())
	{
		mlVehicleData* VehicleData = vehicle->GetGameVehicleData();
		SINT32 j = 0;

		for (j=1; j < VEH_MAX_DAMAGE_OBJECTS; j++)
		{
			if (VehicleData->NumRoundsHit[j] > 0)
				vehicle->ApplyDamage(j - 1, VehicleData->DamageAccrued[j]); // -1 to hit location because it's STUPID and is 1 based
		}

		// update the gr2 stuff
		vehicle->Update(TicTime/1000.0);

		// clear out damage que list
		if (VehicleData->NewDamage == 1)
		{
			for (j=0;j<VEH_MAX_DAMAGE_OBJECTS;j++)
			{
				VehicleData->DamageAccrued[j] = 0.0;
				VehicleData->NumRoundsHit[j] = 0;
			}

			VehicleData->NewDamage = 0;
		}

		if(VehicleData->KillCrew == TRUE || VehicleData->LastKillCrew == TRUE)
			vehApplyDamageToVehicleComponent(VehicleData->VehicleMasterPtr, 1, 10000, frametime);

		// check for dead guy
		if(persGetCrewDeadForKillScore(0) == 1)
			VehicleData->VehicleIsDead = 1;
	}
	else
	{
		// update the gr2 stuff
		vehicle->Update(TicTime/1000.0);
	}

	return TRUE;
}

LOGICAL gr2KillVehicle(vehVehicle* v)
{
	if(gr2IsAnimVehicle(v))
	{
		VehicleList::iterator it = _vehicles.begin();

		for(; it != _vehicles.end(); it++)
		{
			gr2Vehicle* vehicle = *it;

			if(vehicle->GetGameVehicle() == v)
			{
				// erase the vehicle from the vehicle map
				VehicleMap::iterator vit = _vehicleMap.find(v);
				if(vit != _vehicleMap.end()) _vehicleMap.erase(vit);

				// erase the vehicle from the object map
				ObjectMap::iterator oit = _objectMap.find(vehicle->GetGameObject());
				if(oit != _objectMap.end()) _objectMap.erase(oit);

				// delete the vehicle
				delete vehicle;

				// erase the vehicle from the main vehicle list
				_vehicles.erase(it);

				// we're done
				break;
			}
		}
	}

	return 1;
}

// create a new game object; links up the game vehicle and the gr2 class
OBJECT* gr2CreateVehicleObject(vehVehicle* v, UINT32 type)
{
	OBJECT* vobject, *childObject;

	obmainCreateObjectFull (&vobject, "gr2Vehicle", 0, 0);

	// flags
	vobject->flags = 0;
	vobject->flags |= OBJECT_VISIBLE;
	vobject->flags |= OBJECT_LITE;
	vobject->flags |= OBJECT_DYNALITE;
	vobject->flags |= OBJECT_INDEXED;
	vobject->flags |= OBJECT_TRIANGLELIST;
	vobject->flags |= OBJECT_USERDRAW;
	// copied from anim.c?
	vobject->flags |= OBJECT_COLLISION;
	vobject->flags |= OBJECT_USEMATRIX;

	// flag2
	vobject->flag2 |= OBJECT_ANIMATION;
	vobject->flag2 |= OBJECT_TOPMOST;
	vobject->flag2 |= OBJECT_ISFPS; // probbaly don't need this flag?

	// flag3
	vobject->flag3 |= OBJECT_IS_GR2VEHICLE; // <-- identifies this as a gr2 object to other game systems

	// how do we draw?
	vobject->userdraw = _gr2DrawCallback;

	// could use granny's bounding box functions to
	// set the bounding box; would have to move
	// this to below the gr2Vehicle creation below
	vobject->bbox[0].x = -3.5f;
	vobject->bbox[0].y = -3.0f;
	vobject->bbox[0].z = 0;

	vobject->bbox[1].x = 3.5f;
	vobject->bbox[1].y = 4.0f;
	vobject->bbox[1].z = 10.0f;

	vobject->maxrad = 4.0f;

	// make lod1
	obmainCreateObjectFull (&childObject, "lod1", 0, 0);

	// flags
	childObject->flags |= OBJECT_VISIBLE;
	childObject->flags |= OBJECT_LITE;
	childObject->flags |= OBJECT_DYNALITE;
	childObject->flags |= OBJECT_INDEXED;
	childObject->flags |= OBJECT_TRIANGLELIST;
	childObject->flags |= OBJECT_USERDRAW;
	childObject->flags |= OBJECT_LOD;

	// flag2 flags
	childObject->flag2 |= OBJECT_ANIMATION;
	childObject->flag2 |= OBJECT_ISFPS;

	// flag3 flags
	childObject->flag3 |= OBJECT_IS_GR2VEHICLE; // <-- identifies this as a gr2 object to other game systems

	vobject->child = childObject;

	// add it to the current vehicle list
	gr2Vehicle* newVehicle = NULL;

	gr2VehicleSpec* vspec = gr2GetVehicleSpec((const char*)v->VehicleType->Id);
	if(vspec) newVehicle = gr2Vehicle::Create(vspec->klass, v, vobject);

	if(newVehicle)
	{
		_vehicles.push_back(newVehicle);

		_vehicleMap[v]		= newVehicle;
		_objectMap[vobject]	= newVehicle;
	}
	// else clean up

	return vobject;
}

UINT32 gr2ProcessControlInputs(SINT32 param, UINT32 id, UINT32 reserve)
{
	gr2Vehicle* vehicle = gr2GetPersonaVehicle();
	if(vehicle) return vehicle->ProcessControlInputs(param, id, reserve);

	return 0;
}

void gr2UnpackFlags(vehVehicle* v, const VEHICLE_UPDATE_HEADER* header)
{
	gr2Vehicle* vehicle = gr2GetVehicle(v);
	if(vehicle) vehicle->UnpackFlags(header);

	/*
	{
		if(VehicleHeader->present & PR_STATE0)
		{
			v->AnimationBits &= 0xffff0000;
			v->AnimationBits |= VehicleHeader->vi.stateFlags0;
		}

		if(VehicleHeader->present & PR_STATE1)
		{
			v->AnimationBits &= 0x0000ffff;
			v->AnimationBits |= (UINT32)(VehicleHeader->vi.stateFlags1) << 16;
		}

		gr2GetVehicle(v)->UnpackFlags(VehicleHeader);
	}
	*/
}

void gr2PackFlags(VEHICLE_UPDATE_HEADER* header, SHARED_VEHICLE_INFO* info)
{
	gr2Vehicle* vehicle = gr2GetPersonaVehicle();
	if(vehicle) vehicle->PackFlags(header, info);
}

LOGICAL gr2DoWeaponCollision(vehVehicle* v, piVECTOR* start, piVECTOR* end, SINT32* location, piVECTOR* impact)
{
	if(gr2IsAnimVehicle(v))
		return gr2GetVehicle(v)->CollideWithRay(start, end, location, impact, NULL, false);

	return 0;
}

UINT32 gr2ApplyDamage(UINT32 Location, FLOAT32 Damage)
{
	gr2Vehicle* vehicle = gr2GetPersonaVehicle();
	if(vehicle) vehicle->ApplyDamage(Location, Damage);

	return 0;
}

// returns a vehicle by game vehicle; can return null
gr2Vehicle* gr2GetVehicle(vehVehicle* v)
{
	if(v)
	{
		VehicleMap::iterator it = _vehicleMap.find(v);
		if(it != _vehicleMap.end()) return it->second;
	}

	return NULL;
}

gr2Vehicle* gr2GetVehicleByObject(OBJECT* o)
{
	if(o)
	{
		ObjectMap::iterator it = _objectMap.find(o);
		if(it != _objectMap.end()) return it->second;
	}

	return NULL;
}

// returns a vehicle by gr2model ; can return null
gr2Vehicle* gr2GetVehicleByModel(gr2ModelRef m)
{
	for(int i = 0; i < (int)_vehicles.size(); i++)
	{
		if(_vehicles[i]->GetSkeleton() == m)
			return _vehicles[i];
	}

	return NULL;
}


// get my persona's vehicle
gr2Vehicle* gr2GetPersonaVehicle()
{
	return gr2GetVehicle(persGetPersonaVehiclePtr());
}

// return true is this vehicle is an animated vehicle
LOGICAL gr2IsAnimVehicle(vehVehicle* v)
{
	gr2Vehicle* vehicle = gr2GetVehicle(v);
	return (vehicle && vehicle->GetGameObject()->flag3 & OBJECT_IS_GR2VEHICLE);
}

LOGICAL gr2IsValidVehicle(gr2Vehicle* v)
{
	if(v)
	{
		for(int i = 0; i < (int)_vehicles.size(); i++)
			if(_vehicles[i] == v) return true;
	}

	return false;
}

LOGICAL gr2GetEyePoint(piMATRIX* m)
{
	gr2Vehicle* vehicle = gr2GetPersonaVehicle();
	if(vehicle) return vehicle->GetEyeMatrix(m);

	return 0;
}

static LOGICAL _gr2DrawCallback(OBJECT* o)
{
	gr2Vehicle* vehicle = gr2GetVehicleByObject(o);
	if(vehicle) vehicle->Draw();

	return 1;
}
