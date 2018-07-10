#include <syscfg.h> // Path is project specific.
#include "gr2/gr2Vehicle.h"
#include "gr2/gr2Main.h"
#include "gr2/gr2DamageModel.h"
#include "pers/persMain.h"
#include "include/vars.h"
#include "math/math3d.h"
#include "math/matheul.h"
#include "math/mathvect.h"
#include "trn/trnmain.h"
#include "ml/mlutility.h"
#include "ui/uiChat.h"
#include "gr2/gr2Data.h"
#include "gr2/gr2Module.h"
#include "gr2/gr2VehicleStates.h"

// static members
static gr2Vehicle::CreatorMap _creatorMap;

// static functions
static void _objectCollisions(void* data, dGeomID o1, dGeomID o2);
static void _terrainCollisions(void* data, dGeomID o1, dGeomID o2);

gr2Vehicle::gr2Vehicle(vehVehicle* vehicle, OBJECT* pObject) :
	gr2GameObject(),
	_vehicle(vehicle),
	_eyeMatrix(identity),
	_rotation(identity),
	_lastFlags(0),
	_currentSpeed(0),
	_currentLift(0),
	_throttle(0),
	_cruise(0),
	_maxSpeed(0),
	_maxAccel(0),
	_maxTurn(0),
	_spec(NULL),
	_collided(false),
	_aglAltitude(0),
	_lastNetUpdate(0)
{
	_lookAt.x = 0.0f;
	_lookAt.y = 1.0f;
	_lookAt.z = 0.0f;

	_lastFreePos[0] = _lastFreePos[1] = _lastFreePos[2] = 0;
	_lastCollision.x = _lastCollision.y = _lastCollision.z = 0;
	_aglPosition.x = _aglPosition.y = _aglPosition.z = 0;
	_position.x = _position.y = _position.z = 0;

	SetGameObject(pObject);
	SetObjectType(GOT_VEHICLE);

	// get the spec
	_spec = gr2GetVehicleSpec((const char*)vehicle->VehicleType->Id);
}

gr2Vehicle::gr2Vehicle(vehVehicle* vehicle, OBJECT* pObject, const char* skeletonFile, bool skeletonIsLOD) :
	gr2GameObject(skeletonFile, skeletonIsLOD),
	_vehicle(vehicle),
	_eyeMatrix(identity),
	_rotation(identity),
	_lastFlags(0),
	_currentSpeed(0),
	_currentLift(0),
	_throttle(0),
	_cruise(0),
	_maxSpeed(0),
	_maxAccel(0),
	_maxTurn(0),
	_spec(NULL),
	_collided(false),
	_aglAltitude(0)
{
	_lookAt.x = 0.0f;
	_lookAt.y = 1.0f;
	_lookAt.z = 0.0f;

	_lastFreePos[0] = _lastFreePos[1] = _lastFreePos[2] = 0;
	_lastCollision.x = _lastCollision.y = _lastCollision.z = 0;
	_position.x = _position.y = _position.z = 0;

	SetGameObject(pObject);
	SetObjectType(GOT_VEHICLE);

	// get the spec
	_spec = gr2GetVehicleSpec((const char*)vehicle->VehicleType->Id);
}

gr2Vehicle::gr2Vehicle(vehVehicle* vehicle, OBJECT* pObject, gr2ModelRef skeleton, bool skeletonIsLOD) :
	gr2GameObject(skeleton, skeletonIsLOD),
	_vehicle(vehicle),
	_eyeMatrix(identity),
	_rotation(identity),
	_lastFlags(0),
	_currentSpeed(0),
	_currentLift(0),
	_throttle(0),
	_cruise(0),
	_maxSpeed(0),
	_maxAccel(0),
	_maxTurn(0),
	_spec(NULL),
	_collided(false),
	_aglAltitude(0)
{
	_lookAt.x = 0.0f;
	_lookAt.y = 1.0f;
	_lookAt.z = 0.0f;

	_lastFreePos[0] = _lastFreePos[1] = _lastFreePos[2] = 0;
	_lastCollision.x = _lastCollision.y = _lastCollision.z = 0;
	_position.x = _position.y = _position.z = 0;

	SetGameObject(pObject);
	SetObjectType(GOT_VEHICLE);

	// get the spec
	_spec = gr2GetVehicleSpec((const char*)vehicle->VehicleType->Id);
}

gr2Vehicle::~gr2Vehicle()
{
	if(_collider)
		dGeomDestroy(_collider);

	if(_collisionSpace)
		dSpaceDestroy(_collisionSpace);

	_collider = 0;
	_collisionSpace = 0;
}

void gr2Vehicle::PackFlags(VEHICLE_UPDATE_HEADER* header, SHARED_VEHICLE_INFO* info)
{
	UINT32 sharedFlags = _flags[GFT_CONDITION] & GetSharedMask();

	info->stateFlags0	= (UINT16)(sharedFlags & 0xffff);
	info->stateFlags1	= (UINT16)(sharedFlags >> 16);
	info->visBits		= (_damageModel) ? _damageModel->GetDamageFlags(): 0;

	if(info->stateFlags0) header->present |= PR_STATE0;
	if(info->stateFlags1) header->present |= PR_STATE1;
	if(info->visBits) header->present |= PR_VEH_VIS;

	_lastNetUpdate = frametime;
}

void gr2Vehicle::UnpackFlags(const VEHICLE_UPDATE_HEADER* header)
{
	UINT32 sharedMask	= GetSharedMask();
	UINT32 flag			= 0;

	if(header->present & PR_STATE0)
	{
		for(int i = 0; i < 16; i++)
		{
			flag = (1<<i);

			if(sharedMask & flag)
			{
				if(header->vi.stateFlags0 & flag)
					SetState(GFT_CONDITION, flag);
				else
					ClearState(GFT_CONDITION, flag);
			}
		}
	}

	if(header->present & PR_STATE1)
	{
		for(int i = 0; i < 16; i++)
		{
			flag = (1<<i);

			if(header->vi.stateFlags1 & flag)
				SetState(GFT_CONDITION, 1<<(i+16));
			else
				ClearState(GFT_CONDITION, 1<<(i+16));
		}
	}

	if(header->present & PR_VEH_VIS && _damageModel)
		_damageModel->SetDamageFlags(header->vi.visBits);
}


bool gr2Vehicle::Init()
{
	if(_inited)
		return true;

	// initialize gr2 vehicle modules
	gr2ModuleInit();

	if(_spec)
	{
		// set the skeleton if we have one
		if(_spec->skeleton.size())
			SetSkeleton(_spec->Path(_spec->skeleton), false);

		for(int i = 0; i < (int)_spec->model.size(); i++)
		{
			gr2LodSpec& lod = _spec->model[i];
			AddLOD(_spec->Path(lod.model), lod.distance);
		}

		// add configuration parameters so this mech knows
		// how to behave
		gr2ObjectConfig::iterator cit = _spec->config.begin();
		for(; cit != _spec->config.end(); cit++)
			AddConfigParam(cit->first, cit->second);

		// module Init() actually attaches the visual object
		for(int i = 0; i < (int)_spec->loadout.size(); i++)
			AttachObject(gr2Module::Create(&_spec->loadout[i]));

		for(int i = 0; i < (int)_spec->states.size(); i++)
		{
			gr2StateSpec* spec	= _spec->states[i];
			gr2State* state		= gr2State::Create(spec);

			if(state && !AddState(state))
				delete state;
		}

		// set the damage model if we have one
		if(_spec->damageModel.components.size())
			_damageModel = new gr2DamageModel(this, &_spec->damageModel);

		// setup turrets
		if(GetSkeleton())
		{
			for(int i = 0; i < (int)_spec->turrets.size(); i++)
			{
				gr2TurretSpec& spec = _spec->turrets.at(i);
				gr2Turret turret;

				turret.bone				= FindBone(spec.bone.c_str());
				turret.disabled			= false;
				turret.efficiency		= 1.0f;
				turret.elevate			= 0.0f;
				turret.traverse			= 0.0f;
				turret.wantedTraverse	= 0.0f;
				turret.wantedElevate	= 0.0f;
				turret.manualTraverse	= 0.0f;
				turret.manualElevate	= 0.0f;
				turret.spec				= &spec;

				_turrets.push_back(turret);
			}
		}

		if(!_cruise)
			_cruise = _maxSpeed;

		// setup collision detection
		_collisionSpace = dSimpleSpaceCreate(colliderVehicleSpace());
		_collider		= dCreateCapsule(_collisionSpace, 2.0f, 1.0f);

		// position in the physics world
		dGeomSetData(_collider, this);
		dGeomSetCategoryBits((dGeomID)_collisionSpace, COLLIDER_VEHICLE_MASK);
	}

	// for some reason this always starts out with data in it
	_vehicle->PosData.Att.Yaw = 0;

	_spawnTime = frametime;

	return gr2GameObject::Init();
}

// send granny model and attachments to renderer
void gr2Vehicle::Draw()
{
	piSetActiveMaterial(piFindMaterialName("forest1"));

	if(_object->flags & OBJECT_USEMATRIX)
	{
        piPushMatrix(WORLD_MATRIX);

		// FIXME
		// Since we are using left handed matrices and granny doesn't support them
		// we must kludge our way around that fact
		piMATRIX world = _object->mat;
        world._31 = -world._31;
        world._32 = -world._32;
        world._33 = -world._33;

		piSetWorldMatrix(&world);
		piSetRenderState(piRENDERSTATE_CULLMODE, piCULL_CW);
	}

	gr2GameObject::Draw();

	if (_object->flags & OBJECT_USEMATRIX)
	{
		piSetRenderState(piRENDERSTATE_CULLMODE, piCULLMODE_DEFAULT);
        piPopMatrix(WORLD_MATRIX);
	}
}

void gr2Vehicle::Update(FLOAT64 delta)
{
	if(IsFirstPerson())
	{
		FLOAT64VECTOR	LastPos;
		OBJECT*			pObject;
		FLOAT64VECTOR	WorldOriginOffset;
		piVECTOR		TempPiVec;

		vehVehicle* Vehicle				= _vehicle;
		mlVehicleData*	pVehicleData	= GetGameVehicleData();

		// Sanity Checks
		if (pVehicleData == NULL) return;
		if (pVehicleData->VehicleObjectData == NULL) return;
		if (pVehicleData->VehicleObjectData->PlaneObject == NULL) return;

		// Initialize
		pObject = pVehicleData->VehicleObjectData->PlaneObject;

		// determines turns and speed
		CalculateMoves(delta);

		FLOAT64VECTOR CurrentVelocity;

		// no moving for now
		CurrentVelocity.x = 0;
		CurrentVelocity.y = 0;
		CurrentVelocity.z = 0;

		GetVelocity(&CurrentVelocity);

		// Update Positional Info
		LastPos.x = pVehicleData->VehiclePos.x;
		LastPos.y = pVehicleData->VehiclePos.y;
		LastPos.z = pVehicleData->VehiclePos.z;

		// actually moves and rotates the vehicle
		UpdateMoves(delta, &CurrentVelocity);

		// get the world offset; used in various calculations
		trnGetWorldOriginOffset(&WorldOriginOffset);

		// /////////////////
		// Collision Code //
		////////////////////

		static int LastFallTime10ms = 0;
		static int StartFallTime10ms = 0;
		static FLOAT64 StartFallHeight = 0;

		// update the last yaw
		pVehicleData->LastYaw = pVehicleData->VehicleAtt.Yaw;

		dGeomID ray = dCreateRay(0, 5000);

		if(ray)
		{
			// temp vars
			piVECTOR down = {0, 0, -1};
			FLOAT64VECTOR rayPos;
			FLOAT64VECTOR& physicsOffset = physicsGetWorldOffset();

			// reset the above ground alt position
			_aglPosition.z = -5000;

			// init the ray position
			rayPos.x = rayPos.y = rayPos.z = 0;

			// get our pos
			GetAbsoluteWorldPosition(&rayPos);

			// subtract the physics offset so we're in the physics realm
			mathSubVectors(&rayPos, &rayPos, &physicsOffset);

			// now we need move the ray's position up and shoot it straight down
			rayPos.z += 1000;

			// setup the ODE stuff
			dGeomRaySet(ray, rayPos.x, rayPos.y, rayPos.z, down.x, down.y, down.z);
			dGeomSetCollideBits(ray, COLLIDER_TERRAIN_MASK);
			dGeomSetData(ray, this);

			// now we try to collide with the ground; callback handles the actual functionality
			dSpaceCollide2(ray, (dGeomID)colliderTerrainSpace(), this, _terrainCollisions);

			// we're done, destroy the ray
			dGeomDestroy(ray);
		}

		//
		// Falling Code
		//
		if(CurrentVelocity.z <= 0) // falling down or on the ground
		{
			FLOAT64 Velocity = 0.0;

			LastPos.z					= pVehicleData->VehiclePos.z;
			pVehicleData->VehiclePos.z	= _aglPosition.z;

			// final test for falling based on the resulting position
			if ((LastPos.z - pVehicleData->VehiclePos.z > 0.15))// && (!fpsTrooper.teleportToGround))
			{
				if (StartFallTime10ms == 0)
				{
					StartFallTime10ms = ft10mscount;
					StartFallHeight = LastPos.z;
					Velocity = 0.0f;
				}

				if (ft10mscount - StartFallTime10ms > 10) // i.e. 10 * 10ms == 0.1 second
				{
					Velocity = 9.8 * (ft10mscount - StartFallTime10ms - 10) / 100.0;
				}
				else
				{
					pVehicleData->VehiclePos.z = LastPos.z;
				}

				LastPos.z = LastPos.z - Velocity * (ft10mscount - LastFallTime10ms) / 100.0f;

				if (LastPos.z > pVehicleData->VehiclePos.z)
				{
					pVehicleData->VehiclePos.z = LastPos.z;
				}

				LastFallTime10ms = ft10mscount;

				// let subclasses handle falling
				SetFalling(true);
			}
			else
			{
				if (StartFallTime10ms)
					StartFallTime10ms = 0;

				// let subclasses handle falling
				if(IsFalling())
				{
					SetState("landed");
					SetFalling(false);
				}
			}

			CurrentVelocity.z = Velocity;
			//Write_Debug("Vehicle Fall = %f", Velocity);

			//
			// End Falling
			//
		}
		else // lifting up
		{
			static int StartLiftTime10ms = 0;
			static int LastLiftTime10ms = 0;

			FLOAT64 Velocity = 0.0;
			FLOAT64	targetHeight = pVehicleData->VehiclePos.z;

			SetFalling(true);
			//Write_Debug("Vehicle Lift = %f", _currentLift);
		}

		pVehicleData->VehicleVel = CurrentVelocity;

		if(CurrentVelocity.z > 0)
			pVehicleData->VehicleVel.z = 0; // we adjust z directly, so don't add velocity to it

		// Set VechicleAttMat
		pVehicleData->VehicleAtt.Pitch = pVehicleData->VehicleAtt.Roll = 0.0;

		TempPiVec.x =  (FLOAT32)pVehicleData->VehicleAtt.Pitch;
		TempPiVec.y =  (FLOAT32)pVehicleData->VehicleAtt.Roll;
		TempPiVec.z = -(FLOAT32)pVehicleData->VehicleAtt.Yaw;

		mathEul_ToMatrix(&pVehicleData->VehicleAttMat,EulOrdXYZs,&TempPiVec);

		// only do ODE collisions after 30 seconds have passed; we use ODE to
		// see if we collided with buildings and/or vehicles with an ODE collider
		if(_collider && _collisionSpace && (frametime > _spawnTime + 60000))
		{
			_collided = false;

			// do ode collisions
			dSpaceCollide2(_collider, (dGeomID)colliderTerrainSpace(), this, _objectCollisions);
			dSpaceCollide2(_collider, (dGeomID)colliderVehicleSpace(), this, _objectCollisions);
		}

		if(!_collided)
			dOPE(_lastFreePos,=,dGeomGetPosition(_collider));

		////////////////////////
		// End Collision Code //
		////////////////////////

		pVehicleData->VehicleAttVel.Pitch = 0;
		pVehicleData->VehicleAttVel.Roll = 0;
		pVehicleData->VehicleAttVel.Yaw = 0;

		// update base class
		gr2GameObject::Update(delta);

		// update the camera as needed
		UpdateEyeMatrix(delta);
	}
	else
	{
		// determine moves and speed
		CalculateMoves(delta);

		if(_collider)
		{
			FLOAT64VECTOR& offset = physicsGetWorldOffset();
			dGeomSetPosition(_collider, _vehicle->PosData.Pos.x - offset.x, _vehicle->PosData.Pos.y - offset.y, (_vehicle->PosData.Pos.z - offset.z) + 4.5f);
		}

		// update the base class
		gr2GameObject::Update(delta);
	}

	_vehicle->StickVehicleToGround = !IsFalling();
}

bool gr2Vehicle::CollideWithRay(piVECTOR* start, piVECTOR* end, SINT32* location, piVECTOR* impact, piMATRIX* transform, bool recursive, bool update)
{
	return gr2GameObject::CollideWithRay(start, end, location, impact, &_object->mat, true, update);
}

void gr2Vehicle::CollideWithGeom(dGeomID hit)
{
	if(!_collider) return;

	const int MAX_CONTACTS = 8;
	dContact contacts[MAX_CONTACTS];

	FLOAT64VECTOR& offset = physicsGetWorldOffset();
	mlVehicleData* data = GetGameVehicleData();

	int numc = dCollide(_collider, hit, MAX_CONTACTS, &contacts[0].geom, sizeof(dContact));

	for (int i = 0; i < numc; i++)
	{
		dContact* contact = &contacts[i];

		if(contact->geom.depth < .000001) continue; // depth 0 from various trimesh colliders causes "sticks"

		// throw out any normals that are "on the wrong side" to prevent objects from "sticking" or penetrating
		// NOTE: this code is not generalized for all cases, it only works if center of object is inside geometry
		// so we don't do this for trimesh and assume the center is inside for simple capsule used by bouncing grenades

		dVector3 vec;

		if(true)
		{
			dOPC(vec,*,contact->geom.normal,contact->geom.depth);
			dOP(vec,+,contact->geom.pos,vec);
			dOP(vec,-,vec, _lastFreePos);
			if(dDOT(vec, contact->geom.normal) > 0) continue;
		}

		dOPC(vec,*,contact->geom.normal,contact->geom.depth);
		//dOP(vec,+,contact->geom.pos,vec);

		//if(handleContact(contact))
		//	_contacts.push_back(*contact);

		data->VehiclePos.x += vec[0]; //data->LastVehiclePos.x;
		data->VehiclePos.y += vec[1]; //data->LastVehiclePos.y;
		//data->VehiclePos.z += vec[2]; //data->LastVehiclePos.z;

		_lastCollision.x = contact->geom.pos[0] + offset.x;
		_lastCollision.y = contact->geom.pos[1] + offset.y;
		_lastCollision.z = contact->geom.pos[2] + offset.z;

		SetState("collision");

		_collided = true;

		break;
	}

	//if(numc > 0)
	//	_collided = true;
}

void gr2Vehicle::CollideWithGround(dGeomID ray, dGeomID hit)
{
	if(hit == _collider) return;

	const int MAX_CONTACTS = 3;
	dContact contacts[MAX_CONTACTS];
	FLOAT64VECTOR& offset = physicsGetWorldOffset();
	FLOAT64VECTOR position;

	int numc = dCollide(ray, hit, MAX_CONTACTS, &contacts[0].geom, sizeof(dContact));

	for (int i = 0; i < numc; i++)
	{
		dContact* contact = &contacts[i];

		position.x	= contact->geom.pos[0] + offset.x;
		position.y	= contact->geom.pos[1] + offset.y;
		position.z	= contact->geom.pos[2] + offset.z;

		if(position.z > _aglPosition.z)
		{
			_aglAltitude = GetRangeToPosition(position);
			_aglPosition = position;
		}
	}
}

// gets the absolute position
bool gr2Vehicle::GetAbsoluteWorldPosition(FLOAT64VECTOR* position)
{
	if(IsFirstPerson())
	{
		mlVehicleData* data = GetGameVehicleData();
		FLOAT64VECTOR temp	= *position;

		mathVectorMultiply64(position, &temp, &data->VehicleAttMat);
		mathAddVectors(position, position, &(data->VehiclePos));

		return true;
	}
	else
	{
		FLOAT64VECTOR temp = *position;

		mathVectorMultiply64(position, &temp, &_vehicle->PosData.AttMat);
		mathAddVectors(position, position, &(_vehicle->PosData.Pos));

		return true;
	}

	return false;
}

bool gr2Vehicle::GetAttitudeMatrix(piMATRIX* mat)
{
	if(_vehicle)
	{
		if(_vehicle->ModelData)
			*mat = _vehicle->ModelData->VehicleAttMat;
		else
			*mat = _vehicle->PosData.AttMat;

		return true;
	}

	return false;
}

bool gr2Vehicle::GetEyeMatrix(piMATRIX* pMatrix)
{
	*pMatrix = _eyeMatrix;
	return true;
}

void gr2Vehicle::SetEyePosition(piVECTOR* pVector)
{
	_eyeMatrix._41 = pVector->x;
	_eyeMatrix._42 = pVector->y;
	_eyeMatrix._43 = -pVector->z;
}

void gr2Vehicle::UpdateEyeMatrix(FLOAT64 delta)
{
	static piVECTOR up = {0.0f, 0.0f, 1.0f};

	piVECTOR dir, eye;

	/*
	up.x = 0;
	up.y = 0;
	up.z = 1;
	*/

	/*
	lookat.x = 0.0f;
	lookat.y = 0.0f;
	lookat.z = -10.0f;

	cam.x = -10.0f;
	cam.y = -20.0f;
	cam.z = -20.0f;
	*/

	eye.x = _eyeMatrix._41;
	eye.y = _eyeMatrix._42;
	eye.z = -_eyeMatrix._43;

	math3dSubtractVectors(&dir, &eye, &_lookAt);

	/*
	LookAt(&dir, &up, &_eyePoint);

	_eyePoint._41 = cam.x;
	_eyePoint._42 = cam.y;
	_eyePoint._43 = -cam.z;
	*/

	_eyeMatrix = identity;

	piVECTOR& y = dir; // for clarity
	math3dFastNormalize(&y);

	piVECTOR x;
	math3dCrossProduct(&x, &up, &y);
	math3dFastNormalize(&x);

	piVECTOR z;
	math3dCrossProduct(&z, &y, &x);
	math3dFastNormalize(&z);

	_eyeMatrix._11 = x.x;
	_eyeMatrix._12 = x.y;
	_eyeMatrix._13 = x.z;

	_eyeMatrix._21 = y.x;
	_eyeMatrix._22 = y.y;
	_eyeMatrix._23 = y.z;

	// might need to negate these
	_eyeMatrix._31 = z.x;
	_eyeMatrix._32 = z.y;
	_eyeMatrix._33 = z.z;

	_eyeMatrix._41 = eye.x;
	_eyeMatrix._42 = eye.y;
	_eyeMatrix._43 = -eye.z;
}

void gr2Vehicle::LookAt(piVECTOR* pVector)
{
	_lookAt.x = pVector->x;
	_lookAt.y = pVector->y;
	_lookAt.z = pVector->z;
}

bool gr2Vehicle::GetWorldToScreen(const FLOAT64VECTOR* position, float* sx, float* sy, float* sz)
{
	// gluProject code below

	GLdouble x, y, z;
	x = y = z = 0;

	GLdouble modelMatrix[16];
	GLdouble projMatrix[16];
	GLint viewport[4];

	glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
	//glMatrixMode(GL_PROJECTION);
	//glPushMatrix();
	//glOrtho(-1, 1, -1, 1, -1.0, 1.0);
	glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
	//glPopMatrix();
	//glMatrixMode(GL_MODELVIEW);
	glGetIntegerv(GL_VIEWPORT, viewport);

	if(gluProject(position->x, position->y, position->z, modelMatrix, projMatrix, viewport, &x, &y, &z))
	{
		*sx = (FLOAT32)x;
		*sy = (FLOAT32)y;
		if(sz) *sz = (FLOAT32)z;

		return true;
	}

	return false;
}

bool gr2Vehicle::GetWorldToScreen(const piVECTOR* position, float* sx, float* sy, float* sz)
{
	// gluProject code below

	GLdouble x, y, z;
	x = y = z = 0;

	GLdouble modelMatrix[16];
	GLdouble projMatrix[16];
	GLint viewport[4];

	glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
	glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
	glGetIntegerv(GL_VIEWPORT, viewport);

	if(gluProject((GLdouble)position->x, (GLdouble)position->y, (GLdouble)position->z, modelMatrix, projMatrix, viewport, &x, &y, &z))
	{
		*sx = (FLOAT32)x;
		*sy = (FLOAT32)y;
		if(sz) *sz = (FLOAT32)z;

		return true;
	}

	return false;
}

/*
void gr2Vehicle::LookAt(piVECTOR* dir, piVECTOR* up, piMATRIX* result) const
{
	*result = identity;

	piVECTOR y = *dir;
	math3dFastNormalize(&y);

	piVECTOR x;
	math3dCrossProduct(&x, up, &y);
	math3dFastNormalize(&x);

	piVECTOR z;
	math3dCrossProduct(&z, &y, &x);
	math3dFastNormalize(&z);

	result->_11 = x.x;
	result->_12 = x.y;
	result->_13 = x.z;

	result->_21 = y.x;
	result->_22 = y.y;
	result->_23 = y.z;

	// might need to negate these
	result->_31 = z.x;
	result->_32 = z.y;
	result->_33 = z.z;
}
*/

/*
void gr2Vehicle::LookAt(piVECTOR* dir, piVECTOR* up, piMATRIX* result) const
{
	*result = identity;

	piVECTOR z = dir;
	math3dFastNormalize(&z);

	piVECTOR x;
	math3dCrossProduct(&x, up, &z);
	math3dFastNormalize(&x);

	piVECTOR y;
	math3dCrossProduct(&y, &z, &x);
	math3dFastNormalize(&y);

	result->_11 = x.x;
	result->_12 = x.y;
	result->_13 = x.z;

	result->_21 = y.x;
	result->_22 = y.y;
	result->_23 = y.z;

	result->_31 = z.x;
	result->_32 = z.y;
	result->_33 = z.z;
}
*/

bool gr2Vehicle::IsPersonaVehicle() const
{
	return (_vehicle && _vehicle->ModelData);
}

bool gr2Vehicle::IsFirstPerson() const
{
	return IsPersonaVehicle();
}

bool gr2Vehicle::IsThirdPerson() const
{
	return !IsPersonaVehicle();
}

/*
// mouse/keyboard granularity and delta time must be considered
void gr2Vehicle::TurnTurret(GR2_VEHICLE_TURRET_AXIS axis, float amount, int turret)
{
	if(IsFirstPerson())
	{
		mlVehicleData* data = GetGameVehicleData();

		switch(axis)
		{
			case GVTA_VERTICAL_AXIS:
				data->TurretElevate[turret] = amount;
			break;
			case GVTA_HORIZONTAL_AXIS:
				data->TurretTraverse[turret] = amount;
			break;
		}
	}
}
*/

/*
// mouse/keyboard granularity and delta time must be considered
// speed must be considered as well for some vehicles
void gr2Vehicle::TurnVehicle(float amount)
{
	if(IsFirstPerson())
	{
		mlVehicleData* data = GetGameVehicleData();

		// granularity converts to degrees, so multiplier needed for yaw, which is in radians
		//data->VehicleAtt.Yaw -= rotationSpeed*PI/180.0;
		data->VehicleAtt.Yaw -= .5*PI/180.0;

		// Make sure we don't go past the bounds of the Yaw
		while(data->VehicleAtt.Yaw > PI)
			data->VehicleAtt.Yaw -= 2 * PI;

		while(data->VehicleAtt.Yaw < -PI)
			data->VehicleAtt.Yaw += 2 * PI;
	}
}
*/

/*
void gr2Vehicle::MoveVehicle(float amount)
{
	if(IsFirstPerson())
	{
		mlVehicleData* data = GetGameVehicleData();

		// do something
	}

	// determine requestedForward from my current state
	// and mech speed
}
*/

void gr2Vehicle::UpdateMount(UINT32 mount, UINT32 state)
{
	if(IsFirstPerson() && mount < 16)
		GetGameVehicleData()->MountIsFiring[mount] = state;
}

bool gr2Vehicle::GetMountState(UINT32 mount)
{
	if(mount < 16)
	{
		if(IsFirstPerson())
		{
			return GetGameVehicleData()->MountIsFiring[mount];
		}
		else
		{
			if(_vehicle)
				return _vehicle->MountIsFiring[mount];
		}
	}

	return false;
}

void gr2Vehicle::SetMaxSpeed(float ms)
{
	_maxSpeed = ms;
}

void gr2Vehicle::SetMaxTurn(float mt)
{
	_maxTurn = mt;
}

void gr2Vehicle::SetMaxAccel(float ma)
{
	_maxAccel = ma;
}

float gr2Vehicle::GetMaxSpeed(bool fromSpec) const
{
	if(fromSpec)
	{
		const std::string& value = _spec->config.find("maxspeed");
		if(value.size()) return (float)atof(value.c_str());
	}

	return _maxSpeed;
}

float gr2Vehicle::GetMaxTurn(bool fromSpec) const
{
	if(fromSpec)
	{
		const std::string& value = _spec->config.find("maxturn");
		if(value.size()) return (float)atof(value.c_str());
	}

	return _maxSpeed;
}

float gr2Vehicle::GetMaxAccel(bool fromSpec) const
{
	if(fromSpec)
	{
		const std::string& value = _spec->config.find("maxaccel");
		if(value.size()) return (float)atof(value.c_str());
	}

	return _maxSpeed;
}

SINT32 gr2Vehicle::GetLastNetUpdate() const
{
	return _lastNetUpdate;
}

bool gr2Vehicle::GetHUDData(int type, SINT32* data) const
{
	*data = -1;
	return false;
}

FLOAT32 gr2Vehicle::GetRangeToPlayer() const
{
	FLOAT32 range = 0;

	vehVehicle* persona = gr2GetPersonaVehicle()->GetGameVehicle();

	// if I'm not the persona vehicle, calculate
	if(_vehicle != persona)
	{
		FLOAT64VECTOR distance;
		mathSubVectors(&distance, &persona->ModelData->VehiclePos, &_vehicle->PosData.Pos);
		range = (FLOAT32)mathExactVectorLength(&distance);
	}

	return range;
}

FLOAT32 gr2Vehicle::GetRangeToPosition(FLOAT64VECTOR& position) const
{
	FLOAT64VECTOR distance;

	if(IsFirstPerson())
	{
		mathSubVectors(&distance, &GetGameVehicleData()->VehiclePos, &position);
	}
	else
	{
		mathSubVectors(&distance, &GetGameVehicle()->PosData.Pos, &position);
	}

	return (FLOAT32)mathExactVectorLength(&distance);
}


gr2Vehicle::gr2Turret* gr2Vehicle::GetTurret(UINT32 index)
{
	if(index < (UINT32)_turrets.size())
	{
		gr2Turret& turret = _turrets.at(index);
		return &turret;
	}

	return NULL;
}

void gr2Vehicle::UpdateModel(FLOAT64 delta)
{
	if(!GetSkeleton())
		return;

	piMATRIX tempMatrix;

	// eyeRotation is needed because the origin is offset from the point we want to rotate around ..
	// so .. we translate to origin, do our rotations, and translate back to where we belong, then
	// use the resulting rotations and translations to move the granny model

	// reset
	_rotation = identity;
	memset(&_position, 0, sizeof(_position));

	// translate
	/*
	tempMatrix = identity;
	tempMatrix._41 = 0.0f;
	tempMatrix._42 = 0.0f;
	tempMatrix._43 = TROOPER_STAND_EYE_HEIGHT;

	math3dMatrixMultiply(&_skeletonRotation,&tempMatrix,&_skeletonRotation);
	*/

	// don't do these rotations if we're sitting .. the model doesn't move based on your eyepoint any more
	// instead it moves based on the vehicle you're sitting on
	/*
	if (!(stateFlags & STATE_SIT))
	{
		generateRotationMatrix (AROUND_Z, (horizWobbleAngle + horizWeaponLag), &tempMatrix);
		math3dMatrixMultiply(&eyeRotation,&tempMatrix,&eyeRotation);

		generateRotationMatrix (AROUND_X,
			-(-vertWobbleAngle + (AIM_VERTICAL_ANGLE + vertWeaponLag) - fpsTrooper.currentMuzzleRise),
			&tempMatrix);
		math3dMatrixMultiply(&eyeRotation,&tempMatrix,&eyeRotation);
	}
	*/

	// and translate again
	/*
	tempMatrix = identity;
	tempMatrix._41 = 0.0f;
	tempMatrix._42 = 0.0f;
	tempMatrix._43 = -TROOPER_STAND_EYE_HEIGHT;

	math3dMatrixMultiply(&eyeRotation,&tempMatrix,&eyeRotation);
	*/

	_position.x = _rotation._41;
	_position.y = _rotation._42;
	_position.z = _rotation._43;
	//eyePosition.y += -2; // use this to see the 1st person stuff work in 3rd person

	/*
	piMATRIX tempMatrix2 = eyeRotation;
	float offset = -0.5f; //-1.0f + (2*mathGetFloat32ChaoticNumber());

	generateRotationMatrix (AROUND_Z, offset, &tempMatrix);
	math3dMatrixMultiply(&tempMatrix2,&tempMatrix,&tempMatrix2);

	gr2SetModelRotation (skeletonModel, &tempMatrix2);
	*/

	// orient the model; might need to be pulled from
	// data because each model could be exported
	// with different orientations (ideally not)
	GenerateRotation(AROUND_Z, -90, &tempMatrix);
	math3dMatrixMultiply(&_rotation,&tempMatrix,&_rotation);

	gr2SetModelRotation(_skeleton, &_rotation);
	gr2SetModelPosition(_skeleton, &_position);

	gr2GameObject::UpdateModel(delta);
}

void gr2Vehicle::UpdateCallback(gr2ModelRef model)
{
	// other things may need to be updated here
	if(model == GetSkeleton())
		UpdateTurrets();
}

void gr2Vehicle::UpdateTurrets()
{
	if(IsFirstPerson())
	{
		mlVehicleData* data = GetGameVehicleData();

		if(data)
		{
			for(int i = 0; i < (int)_turrets.size(); i++)
			{
				gr2Turret& turret = _turrets.at(i);

				if(turret.bone < 0) continue;

				// lets handle traverse rotations
				if(turret.spec->traverse.tid >= 0)
				{
					// we deal in degrees
					// turrets deal from -.5 ... 0 ... .5 which equates to -180 ... 0 ... 180 degrees
					// so we have to do some conversions
					int tid					= turret.spec->traverse.tid;
					float wantedTraverse	= (float)data->WantedTurretTraverse[tid];
					float minTraverse		= (turret.spec->traverse.minrot / 180.0f) * .5f;
					float maxTraverse		= (turret.spec->traverse.maxrot / 180.0f) * .5f;

					// current rotations in degrees
					float traverse			= ((float)data->TurretTraverse[tid] * 2) * 180.0f;

					// enforce turret limits
					if(wantedTraverse > maxTraverse)
						data->WantedTurretTraverse[tid] = maxTraverse;
					else if(wantedTraverse < minTraverse)
						data->WantedTurretTraverse[tid] = minTraverse;

					turret.wantedTraverse	= (float)data->WantedTurretTraverse[tid];

					// do rotations
					if(turret.spec->traverse.invert)
						RotateBoneLocally(turret.bone, turret.spec->traverse.axis, -turret.traverse);
					else
						RotateBoneLocally(turret.bone, turret.spec->traverse.axis, turret.traverse);

					if(!turret.disabled && traverse != turret.traverse)
						turret.traverse = (traverse < 0) ? max(traverse, turret.spec->traverse.minrot):
														   min(traverse, turret.spec->traverse.maxrot);
				}

				// lets handle elevation rotations
				if(turret.spec->elevate.tid >= 0)
				{
					// we deal in degrees
					// turrets deal from -.5 ... 0 ... .5 which equates to -180 ... 0 ... 180 degrees
					// so we have to do some conversions
					int tid					= turret.spec->elevate.tid;
					float wantedElevate		= (float)data->WantedTurretElevate[tid];
					float minElevate		= (turret.spec->elevate.minrot / 180.0f) * .5f;
					float maxElevate		= (turret.spec->elevate.maxrot / 180.0f) * .5f;

					// current rotations in degrees
					float elevate			= ((float)data->TurretElevate[tid] * 2) * 180.0f;

					// enforce turret limits
					if(wantedElevate > maxElevate)
						data->WantedTurretElevate[tid] = maxElevate;
					else if(wantedElevate < minElevate)
						data->WantedTurretElevate[tid] = minElevate;

					turret.wantedElevate	= (float)data->WantedTurretElevate[tid];

					// do rotations
					if(turret.spec->elevate.invert)
						RotateBoneLocally(turret.bone, turret.spec->elevate.axis, -turret.elevate);
					else
						RotateBoneLocally(turret.bone, turret.spec->elevate.axis, turret.elevate);

					if(!turret.disabled && elevate != turret.elevate)
						turret.elevate = (elevate < 0) ? max(elevate, turret.spec->elevate.minrot):
														 min(elevate, turret.spec->elevate.maxrot);
				}

				/*
				// we deal in degrees
				// turrets deal from -.5 ... 0 ... .5 which equates to -180 ... 0 ... 180 degrees
				// so we have to do some conversions
				float wantedTraverse	= (float)data->WantedTurretTraverse[0];
				float wantedElevate		= (float)data->WantedTurretElevate[0];
				float maxTraverse		= (turret.spec->traverse / 180.0f) * .5f;
				float maxElevate		= (turret.spec->elevate / 180.0f) * .5f;

				// current rotations in degrees
				float traverse			= ((float)data->TurretTraverse[0] * 2) * 180.0f;
				float elevate			= ((float)data->TurretElevate[0] * 2) * 180.0f;

				// enforce turret limits
				if(wantedTraverse > maxTraverse || wantedTraverse < -maxTraverse)
					data->WantedTurretTraverse[0] = (wantedTraverse < 0) ? -maxTraverse: maxTraverse;

				if(wantedElevate > maxElevate || wantedElevate < -maxElevate)
					data->WantedTurretElevate[0] = (wantedElevate < 0) ? -maxElevate: maxElevate;

				turret.wantedTraverse	= (float)data->WantedTurretTraverse[0];
				turret.wantedElevate	= (float)data->WantedTurretElevate[0];

				// do rotations
				{
					RotateBoneLocally(turret.bone, 1, -turret.traverse);

					if(traverse != turret.traverse)
						turret.traverse = (traverse < 0) ? max(traverse, -turret.spec->traverse): min(traverse, turret.spec->traverse);
				}

				{
					RotateBoneLocally(turret.bone, 0, -turret.elevate);

					if(elevate != turret.elevate)
						turret.elevate = (elevate < 0) ? max(elevate, -turret.spec->elevate): min(elevate, turret.spec->elevate);
				}

				//uiChatPrintLine("Mech Turret: %.2f %.2f", turret.traverse, turret.elevate);
				*/
			}
		}
	}
	else
	{
		for(int i = 0; i < (int)_turrets.size(); i++)
		{
			gr2Turret& turret = _turrets.at(i);

			if(turret.bone < 0) continue;

			// lets handle traverse rotations
			if(turret.spec->traverse.type == gr2TurretRotationSpec::TURRET_TYPE_TRAVERSE)
			{
				// we deal in degrees
				// turrets deal from -.5 ... 0 ... .5 which equates to -180 ... 0 ... 180 degrees
				turret.wantedTraverse	= _vehicle->StateAnimation.TurretTraverse[turret.spec->traverse.tid];

				// current rotations in degrees
				turret.traverse			= (turret.wantedTraverse * 2) * 180.0f;

				// do rotations
				if(turret.spec->traverse.invert)
					RotateBoneLocally(turret.bone, turret.spec->traverse.axis, -turret.traverse);
				else
					RotateBoneLocally(turret.bone, turret.spec->traverse.axis, turret.traverse);
			}

			// lets handle elevation rotations
			if(turret.spec->elevate.type == gr2TurretRotationSpec::TURRET_TYPE_ELEVATE)
			{
				// we deal in degrees
				// turrets deal from -.5 ... 0 ... .5 which equates to -180 ... 0 ... 180 degrees
				turret.wantedElevate	= _vehicle->StateAnimation.TurretElev[turret.spec->elevate.tid];

				// current rotations in degrees
				turret.elevate			= (turret.wantedElevate * 2) * 180.0f;

				// do rotations
				if(turret.spec->elevate.invert)
					RotateBoneLocally(turret.bone, turret.spec->elevate.axis, -turret.elevate);
				else
					RotateBoneLocally(turret.bone, turret.spec->elevate.axis, turret.elevate);
			}

			/*
			// we deal in degrees
			// turrets deal from -.5 ... 0 ... .5 which equates to -180 ... 0 ... 180 degrees
			turret.wantedTraverse	= _vehicle->StateAnimation.TurretTraverse[0];
			turret.wantedElevate	= _vehicle->StateAnimation.TurretElev[0];

			// current rotations in degrees
			turret.traverse			= (turret.wantedTraverse * 2) * 180.0f;
			turret.elevate			= (turret.wantedElevate * 2) * 180.0f;

			// do rotations
			RotateBoneLocally(turret.bone, 1, -turret.traverse);
			RotateBoneLocally(turret.bone, 0, -turret.elevate);
			*/
		}
	}
}

#define MIN_MOVE_DIFFERENCE .01

void gr2Vehicle::CalculateMoves(FLOAT64 delta)
{
	if(IsFirstPerson())
	{
		// handle turns
		mlVehicleData* data = GetGameVehicleData();

		if (data->Steering < 0)
			_currentTurn = (float)((_maxTurn*PI/180.0) * delta);
		else if (data->Steering > 0)
			_currentTurn = -((float)((_maxTurn*PI/180.0) * delta));
		else
			_currentTurn = 0;

		if(false && _currentSpeed < 0)
			_currentTurn *= -1;

		//if (fabs(_currentTurn) < MIN_MOVE_DIFFERENCE)
		//	_currentTurn = 0;

		// handle movement
		FLOAT32	requestedForward = (float)_throttle * _maxSpeed;

		if (((_currentSpeed - requestedForward) * (_currentSpeed - requestedForward))  < (MIN_MOVE_DIFFERENCE * MIN_MOVE_DIFFERENCE))
		{
			_currentSpeed = requestedForward;
		}
		else if(requestedForward > 0) // forward
		{
			if(_currentSpeed < requestedForward)
			{
				_currentSpeed += _maxAccel * (FLOAT32)delta;
				_currentSpeed = min(_currentSpeed, requestedForward);
			}
			else if(_currentSpeed > requestedForward)
			{
				_currentSpeed -= _maxAccel * (FLOAT32)delta;
				_currentSpeed = max(_currentSpeed, requestedForward);
			}
		}
		else if(requestedForward < 0) // reverse
		{
			if (_currentSpeed < requestedForward)
			{
				_currentSpeed += _maxAccel * (FLOAT32)delta;
				_currentSpeed = min(_currentSpeed, requestedForward);
			}
			else if (_currentSpeed > requestedForward)
			{
				_currentSpeed -= _maxAccel * (FLOAT32)delta;
				_currentSpeed = max(_currentSpeed, requestedForward);
			}
		}
		else
		{
			if (_currentSpeed > 0)
			{
				_currentSpeed -= _maxAccel * (FLOAT32)delta;
				if(_currentSpeed < 0) _currentSpeed = 0;
			}
			else if (_currentSpeed < 0)
			{
				_currentSpeed += _maxAccel * (FLOAT32)delta;
				if(_currentSpeed > 0) _currentSpeed = 0;
			}
		}

		_vehicle->ModelData->VehicleAcc.y = (_currentSpeed) ? _currentSpeed: 0;
	}
	else
	{
		_currentSpeed	= (float)mathExactVectorLength(&_vehicle->PosData.Vel);
		_currentTurn	= ((float)_vehicle->PosData.LastUpdateAtt.Yaw - (float)_vehicle->PosData.Att.Yaw);

		if(_vehicle->PosData.Acc.y < 0)
			_currentSpeed *= -1;

	}
}

void gr2Vehicle::GetVelocity(FLOAT64VECTOR* velocity)
{
	velocity->x = 0.0f;
	velocity->y = -_currentSpeed;
	velocity->z = _currentLift;
}

void gr2Vehicle::UpdateMoves(FLOAT64 delta, FLOAT64VECTOR* currentVel)
{
	if(IsFirstPerson())
	{
		FLOAT64			CosValue, SinValue;
		FLOAT64VECTOR	Velocity;

		mlVehicleData* data = GetGameVehicleData();

		// update our rotation
		data->VehicleAtt.Yaw += _currentTurn;

		// rotate vector along yaw axis (which is along the Z axis, counterclockwise)
		CosValue = cos((data->VehicleAtt.Yaw + PI));
		SinValue = sin((data->VehicleAtt.Yaw + PI));

		// Update Position
		Velocity.y = CosValue * currentVel->y + SinValue * currentVel->x;
		Velocity.x = -SinValue * currentVel->y + CosValue * currentVel->x;
		Velocity.z = currentVel->z;

		data->LastVehiclePos.x = data->VehiclePos.x;
		data->LastVehiclePos.y = data->VehiclePos.y;
		data->LastVehiclePos.z = data->VehiclePos.z;

		data->VehiclePos.x += Velocity.x * delta;
		data->VehiclePos.y += Velocity.y * delta;
		data->VehiclePos.z += Velocity.z * delta; // jump jets?

		LOGICAL onWater = 0;
		if(false && mlIsFPSOnWater(data, &onWater) && onWater)
			data->VehiclePos.z -= 2;

		currentVel->x = Velocity.x;
		currentVel->y = Velocity.y;
		currentVel->z = Velocity.z;

		if(_collider)
		{
			FLOAT64VECTOR& offset = physicsGetWorldOffset();
			dGeomSetPosition(_collider, data->VehiclePos.x - offset.x, data->VehiclePos.y - offset.y, (data->VehiclePos.z - offset.z) + 5.0f);
		}
	}
}

int gr2Vehicle::PlaySound(const char* sound, FLOAT32 vol, FLOAT32 fMin, FLOAT32 fMax, bool infinite, int priority, int soundType, int playType, OBJECT* emitter)
{
	if(IsThirdPerson())
		return gr2GameObject::PlaySound(sound, 1.0f, 25.0f, 200.0f, infinite, IDE_TP_PRIORITY_LOW, TYPE_FINITE_SOUND, DEF_PLAYTYPE_OBJECT, _object);

	return gr2GameObject::PlaySound(sound, vol, fMin, fMax, infinite, priority, soundType, playType, _object);
}

/*
static char* controlStates[] =
{
	"control",
	"action",
	"condition",
	"damage"
};


bool gr2VehicleStateLookup(GR2_FLAG_TYPE type, const char* name, unsigned int* value)
{
	switch(type)
	{
		case GFT_CONTROL:
		{
			for(int i = 0; i < sizeof(controlStates); i++)
			{
				if(!strcasecmp(name, controlStates[i]))
				{
					*value = 1<<i;
					return true;
				}
			}
		}
		break;
		case GFT_ACTION:
		{
			for(int i = 0; i < sizeof(controlStates); i++)
			{
				if(!strcasecmp(name, controlStates[i]))
				{
					*value = 1<<i;
					return true;
				}
			}
		}
		break;
		case GFT_CONDITION:
		{
			for(int i = 0; i < sizeof(controlStates); i++)
			{
				if(!strcasecmp(name, controlStates[i]))
				{
					*value = 1<<i;
					return true;
				}
			}
		}
		break;
		case GFT_DAMAGE:
		{
			for(int i = 0; i < sizeof(controlStates); i++)
			{
				if(!strcasecmp(name, controlStates[i]))
				{
					*value = 1<<i;
					return true;
				}
			}
		}
		break;
	}

	return false;
}
*/

void gr2Vehicle::RegisterCreator(const char* creatorName, GR2_VEHICLE_CREATOR creator)
{
	_creatorMap[creatorName] = creator;
}

gr2Vehicle* gr2Vehicle::Create(std::string& creatorName, vehVehicle* vehicle, OBJECT* pObject)
{
	GR2_VEHICLE_CREATOR creator = _creatorMap[creatorName];
	return (creator) ? creator(vehicle, pObject): new gr2Vehicle(vehicle, pObject);
}

void _objectCollisions(void* data, dGeomID o1, dGeomID o2)
{
	if(dGeomIsSpace(o1) || dGeomIsSpace(o2))
	{
		dSpaceCollide2((dGeomID)o1, (dGeomID)o2, data, _objectCollisions);
		return;
	}

	if(data)
	{
		gr2Vehicle* vehicle = (gr2Vehicle*)data;

		if(o1 == vehicle->GetCollider())
			vehicle->CollideWithGeom(o2);
		else if(o2 == vehicle->GetCollider())
			vehicle->CollideWithGeom(o1);
	}
}

void _terrainCollisions(void* data, dGeomID o1, dGeomID o2)
{
	if(dGeomIsSpace(o1) || dGeomIsSpace(o2))
	{
		dSpaceCollide2((dGeomID)o1, (dGeomID)o2, data, _terrainCollisions);
		return;
	}

	if(data)
	{
		// geom class 5 is a ray
		if(dGeomGetClass(o1) == 5)
			((gr2Vehicle*)data)->CollideWithGround(o1, o2);
		else if(dGeomGetClass(o2) == 5)
			((gr2Vehicle*)data)->CollideWithGround(o2, o1);

	}
}