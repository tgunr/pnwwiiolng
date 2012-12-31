#include <syscfg.h> // Path is project specific.
#include "gr2/gr2Weapon.h"
#include "gr2/gr2Vehicle.h"
#include "gr2/gr2Data.h"
#include "gr2/gr2DamageModel.h"
#include "veh/vehmain.h"
#include "include/vars.h"
#include "math/math3d.h"
#include "math/mathvect.h"
#include "math/mathUnitConversion.h"

static gr2Weapon::CreatorMap _creatorMap;

gr2Weapon::gr2Weapon(gr2ModuleSpec* spec) :
	gr2GameObject(),
	_spec(spec),
	_gameWeapon(NULL),
	_fireBone(-1),
	_ejectBone(-1),
	_weaponMount(-1),
	_weaponIndex(-1),
	_weaponAmmo(-1),
	_attachBone(-1),
	_connectBone(-1),
	_startFireTime(0),
	_lastFireCheckTime(0),
	_coolDown(0),
	_active(true)
{
	SetObjectType(GOT_WEAPON);
}

gr2Weapon::gr2Weapon(const char* skeletonFile, bool skeletonIsLOD) :
	gr2GameObject(skeletonFile, skeletonIsLOD),
	_spec(NULL),
	_gameWeapon(NULL),
	_fireBone(-1),
	_ejectBone(-1),
	_weaponMount(-1),
	_weaponIndex(-1),
	_weaponAmmo(-1),
	_attachBone(-1),
	_connectBone(-1),
	_startFireTime(0),
	_lastFireCheckTime(0),
	_coolDown(0),
	_active(true)
{
	SetObjectType(GOT_WEAPON);
}

gr2Weapon::gr2Weapon(gr2ModelRef skeleton, bool skeletonIsLOD) :
	gr2GameObject(skeleton, skeletonIsLOD),
	_spec(NULL),
	_gameWeapon(NULL),
	_fireBone(-1),
	_ejectBone(-1),
	_weaponMount(-1),
	_weaponIndex(-1),
	_weaponAmmo(-1),
	_attachBone(-1),
	_connectBone(-1),
	_startFireTime(0),
	_lastFireCheckTime(0),
	_coolDown(0),
	_active(true)
{
	SetObjectType(GOT_WEAPON);
}

gr2Weapon::~gr2Weapon()
{
}

void gr2Weapon::SetMount(int mount, int index)
{
	_weaponMount = (mount >= 0 && mount < 16) ? mount: 0;
	_weaponIndex = (index >= 0 && index < 4) ? index: 0;
}

int gr2Weapon::GetWeaponMount() const
{
	return _weaponMount;
}

void gr2Weapon::SetAmmo(int ammo)
{
	_weaponAmmo = ammo;
}

void gr2Weapon::Toggle()
{
	_active = !_active;
}

// is this weapon active?
bool gr2Weapon::IsActive() const
{
	return _active;
}

void gr2Weapon::SetAttachBone(const char* bone)
{
	if(_parent)
		SetAttachBone(_parent->FindBone(bone));
}

void gr2Weapon::SetAttachBone(int bone)
{
	_attachBone = bone;
}

bool gr2Weapon::Init()
{
	if(_spec)
	{
		/* we might use these later
		// set the skeleton if we have one
		if(_spec->skeleton.size())
			SetSkeleton(_spec->skeleton.c_str(), true);
		*/

		// add visual lods
		if(_spec->data->shapes.size())
		{
			gr2Vehicle* vehicle = GetParentVehicle();
			const char* shape = (vehicle) ? vehicle->GetGameVehicleShape(): "";

			if(_spec->data->shapes.find(shape) != _spec->data->shapes.end())
			{
				gr2ModelSpecs& model = _spec->data->shapes[shape];

				for(int i = 0; i < (int)model.size(); i++)
				{
					gr2LodSpec& lod = model[i];
					AddLOD(_spec->Path(lod.model), lod.distance);
				}
			}
		}

		for(int i = 0; i < (int)_spec->data->states.size(); i++)
		{
			gr2StateSpec* spec = _spec->data->states[i];
			AddState(gr2State::Create(spec));
		}

		// set the damage model if we have one
		if(_spec->data->damageModel.components.size())
			_damageModel = new gr2DamageModel(this, &_spec->data->damageModel);

		// need to look up the underlying weapon
		//SetAttachBone(_spec->attach.c_str());
		//SetMount(_spec->mount, _spec->index);
		//SetAmmo(_spec->ammo);
	}

	if(GetSkeleton())
	{
		// lookup our connect bone
		if(_connectBone == -1) _connectBone = FindBone("connect", false);

		// lookup our firepoint
		if(_fireBone == -1) _fireBone = FindBone("firepoint", false);

		// lookup our ejectpoint
		if(_ejectBone == -1) _ejectBone = FindBone("ejectpoint", false);

		// update our attach point
		if(_parent && _parent->GetSkeleton())
			_parent->AttachObject(this, _connectBone, _attachBone);
	}

	gr2Vehicle* parentVehicle = GetParentVehicle();

	if(parentVehicle)
	{
		vehVehicle* vehicle = parentVehicle->GetGameVehicle();

		if(vehicle && vehicle->VehicleType && _spec)
		{
			// get a ref to the game weapon
			_gameWeapon = &vehicle->Weapons[_weaponMount][_weaponIndex];

			const std::string& weapTypeName = _spec->data->config.find("gameweapon");

			// init the weapon type
			weapFindWeaponTypeByName(&(vehicle->VehicleType->Loadout[0].WeaponTypes[_weaponMount][_weaponIndex]), (UCHAR*)weapTypeName.c_str());

			// init the game weapon
			weapInitializeWeapon(_gameWeapon, vehicle->VehicleType->Loadout[0].WeaponTypes[_weaponMount][_weaponIndex]);

			// set up ammo
			_gameWeapon->AmmoTypeInUse									= _weaponAmmoType;
			_gameWeapon->NumRoundsAmmoLeft[_gameWeapon->AmmoTypeInUse]	= _weaponAmmo;
			_gameWeapon->NumTotalRounds[_gameWeapon->AmmoTypeInUse]		= _weaponAmmo;

			//if(_gameWeapon->WeaponType->FireType > 1)
			//	_gameWeapon->WeaponType->ReloadTime = 0;
		}
	}

	return gr2GameObject::Init();
}

void gr2Weapon::Update(FLOAT64 delta)
{
	if(HasFlag(GFT_CONTROL, GW_CONTROL_FIRE))
	{
		if(_fireDelay && !_startFireTime)
			_startFireTime = frametime + (int)(1000 * _fireDelay);

		if(!_startFireTime || frametime >= _startFireTime)
		{
			if(!Fire())
				StopFiring();
		}
	}

	_lastFireCheckTime = frametime;

	gr2GameObject::Update(delta);
}

// gets the absolute position
bool gr2Weapon::GetAbsoluteWorldPosition(FLOAT64VECTOR* position)
{
	return GetParentVehicle()->GetAbsoluteWorldPosition(position);
}

gr2Vehicle* gr2Weapon::GetParentVehicle() const
{
	gr2Object* parent = _parent;

	while(parent && !parent->IsObjectType(GOT_VEHICLE))
		parent = parent->GetParentObject();

	return dynamic_cast<gr2Vehicle*>(parent);
}

bool gr2Weapon::IsReady()
{
	if(!_gameWeapon || !IsActive() || !HasAmmo() || IsReloading())
		return false;

	return true;
}

bool gr2Weapon::IsReloading()
{
	if(_gameWeapon)
		return weapIsWeaponReloading(_gameWeapon, frametime/1000.0f);

	return false;
}

bool gr2Weapon::IsRecycling()
{
	if(_gameWeapon)
	{
		FLOAT64 fireDelta = (frametime - _lastFireCheckTime)/1000.0f;
		return (fireDelta < _gameWeapon->WeaponType->RateOfFire/1000.0f);
	}

	return false;
}

bool gr2Weapon::HasAmmo()
{
	if(_gameWeapon)
		return (_gameWeapon->NumRoundsAmmoLeft[_gameWeapon->AmmoTypeInUse]);

	return false;
}


bool gr2Weapon::StartFiring()
{
	if(IsReady())
	{
		if(GetParentVehicle()->IsFirstPerson())
			GetParentVehicle()->GetGameVehicleData()->MountIsFiring[_weaponMount] = 1;

		SetFlag(GFT_CONTROL, GW_CONTROL_FIRE);
		return true;
	}

	return false;
}

bool gr2Weapon::StopFiring()
{
	if(HasFlag(GFT_CONTROL, GW_CONTROL_FIRE))
		SetState("cooldown");

	ClearFlag(GFT_CONTROL, GW_CONTROL_FIRE);
	ClearState("fire");

	if(GetParentVehicle()->IsFirstPerson())
		GetParentVehicle()->GetGameVehicleData()->MountIsFiring[_weaponMount] = 0;

	_startFireTime = 0;

	return true;
}

void gr2Weapon::SetConvergence(piMATRIX* fireMat)
{
	gr2Vehicle* vehicle = GetParentVehicle();
	float accuracy		= vehicle->GetAccuracy();

	if(!accuracy) return;

	FLOAT64VECTOR fireVec, comVec;

	if(GetAbsoluteBonePosition(_fireBone, &fireVec) && vehicle->GetAbsoluteBonePosition("COM", &comVec))
	{
		const FLOAT64VECTOR* aimPosPtr = vehicle->GetAimPosition();
		FLOAT64VECTOR aimPos;

		aimPos.x	= aimPosPtr->x;
		aimPos.y	= aimPosPtr->y;
		aimPos.z	= 0;
		fireVec.z	= 0;
		comVec.z	= 0;

		mathSubVectors(&fireVec, &aimPos, &fireVec);
		mathSubVectors(&comVec, &aimPos, &comVec);

		mathNormalize(&fireVec);
		mathNormalize(&comVec);

		float angle = (float)RAD_TO_DEG(acos(mathVectDotProduct(&fireVec, &comVec)));

		if(_weaponIndex == 1 || _weaponMount == 1)
			angle *= -1;

		piMATRIX zRot = identity;
		GenerateRotation(2, accuracy * angle, &zRot);
		math3dMatrixMultiply(fireMat, fireMat, &zRot);
	}
}

bool gr2Weapon::Fire()
{
	if(_gameWeapon && IsReady())
	{
		SetState("fire");

		gr2Vehicle* vehicle			= GetParentVehicle();

		FLOAT64 currentTime			= frametime / 1000.0;
		FLOAT64 fireDelta			= (frametime - _lastFireCheckTime) / 1000.0f;

		FLOAT64VECTOR recoil		= {0,0,0};
		FLOAT64VECTOR position		= {0,0,0}; // firepoint position
		FLOAT64VECTOR vehVelocity	= {0,0,0}; // vehicle's velocity vector
		FLOAT64VECTOR flashPosition	= {0,0,0}; // vehicle's velocity vector
		piMATRIX fireMatrix			= identity; // firepoint rotational matrix
		piMATRIX fireAttitude		= identity;
		UINT32 firedFromGhost		= (vehicle->IsThirdPerson()) ? 1: 0;
		UINT32 canCauseDamage		= !firedFromGhost;
		UINT32 fireFlags			= 0;
		FLOAT64VECTOR relativePos	= {0,0,0};

		piMATRIX* vehAttMat			= NULL;
		FLOAT64VECTOR* vehPos		= NULL;

		if(vehicle->IsFirstPerson())
		{
			mlVehicleData* data = vehicle->GetGameVehicleData();

			vehAttMat	= &data->VehicleAttMat;
			vehPos		= &data->VehiclePos;

			data->MountIsFiring[_weaponMount] = 1;
		}
		else
		{
			vehAttMat	= &vehicle->GetGameVehicle()->PosData.AttMat;
			vehPos		= &vehicle->GetGameVehicle()->PosData.LastUpdatePos;
		}

		//GetAbsoluteBonePosition(_fireBone, &fireMatrix);
		GetBoneTransform(_fireBone, &fireMatrix);

		fireFlags |= WEAP_AMMOFLAG_USE_MEDCOLLIDELIST;
		//fireFlags |= WEAP_AMMOFLAG_CANTHITPLAYER;
		fireFlags |= WEAP_AMMOFLAG_DONTCOLLIDEWITHFIRER;

		if(firedFromGhost)
			fireFlags |= WEAP_AMMOFLAG_DONT_SEND_DAMAGE;

		//fireFlags |= WEAP_AMMOFLAG_FPS_WEAPON;

		/* this negates the flash's direction
		fireMatrix._11 = -1.0f;
		fireMatrix._22 = -1.0f;
		*/

		flashPosition.x = fireMatrix._41;
		flashPosition.y = fireMatrix._42;
		flashPosition.z = fireMatrix._43;

		fireAttitude = fireMatrix;
		fireAttitude._41 = 0;
		fireAttitude._42 = 0;
		fireAttitude._43 = 0;

		if(!firedFromGhost)
			SetConvergence(&fireAttitude);

		math3dMatrixMultiply(&fireAttitude, &fireAttitude, vehAttMat);

		mathVectorMultiply64(&position, &flashPosition, vehAttMat);

		mathAddVectors(&position, &position, vehPos);

		weapFireWeapon(&recoil,
			_gameWeapon,
			&position,
			&vehVelocity,
			&fireAttitude,
			fireDelta,
			currentTime,
			canCauseDamage,
			firedFromGhost,
			fireFlags,
			NULL,
			&relativePos,
			NULL,
			vehicle->GetGameVehicle());

		return true;
	}

	return false;
}

void gr2Weapon::HandleStateEvent(GR2_STATE_EVENT e, gr2State* s)
{
	switch(s->flagType)
	{
		case GFT_ACTION:
		{
			switch(e)
			{
				case GSE_UPDATE:
				{
					switch(s->value)
					{
						case GW_LOGIC_RELOAD:
						{
							if(!IsReloading())
								s->Stop();
						}
						break;
					}
				}
				break;
			}
		}
		break;
		case GFT_CONTROL:
			// do nothing
		break;
		case GFT_CONDITION:
			// do nothing
		break;
	}
}


void gr2Weapon::RegisterCreator(const char* creatorName, GR2_WEAPON_CREATOR creator)
{
	_creatorMap[creatorName] = creator;
}

gr2Weapon* gr2Weapon::Create(std::string& creatorName, gr2ModuleSpec* spec)
{
	GR2_WEAPON_CREATOR creator = _creatorMap[creatorName];
	return (creator) ? creator(spec): new gr2Weapon(spec);
}

bool gr2Weapon::GetFirePoint(piMATRIX* mat)
{
	return GetBoneTransform(_fireBone, mat);
}

bool gr2Weapon::GetEjectPoint(piMATRIX* mat)
{
	return GetBoneTransform(_ejectBone, mat);
}
