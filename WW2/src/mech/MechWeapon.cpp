#include <syscfg.h> // Path is project specific.
#include "mech/MechWeapon.h"
#include "mech/MechVehicle.h"
#include "gr2/gr2Data.h"
#include "gr2/gr2DamageModel.h"
#include "veh/vehmain.h"
#include "include/vars.h"
#include "math/math3d.h"
#include "math/mathvect.h"
#include "math/mathUnitConversion.h"
#include "js/jsGR2Object.h"
#include "imap/ww2func.h"

//static gr2Weapon::CreatorMap _creatorMap;

MechWeapon::MechWeapon(gr2ModuleSpec* spec) :
	MechModule(spec),
	_gameWeapon(NULL),
	_firePoint(NULL),
	_fireBone(-1),
	_ejectBone(-1),
	_weaponAmmo(-1),
	_weaponAmmoType(0),
	_fireDelay(0),
	_startFireTime(0),
	_lastFireCheckTime(0)
{
	SetObjectType(GOT_WEAPON);
}

MechWeapon::~MechWeapon()
{
}

void MechWeapon::SetAmmo(int ammo)
{
	_weaponAmmo = ammo;
}

bool MechWeapon::Init()
{
	if(MechModule::Init())
	{
		MechVehicle* mech = GetParentVehicle();

		if(GetSkeleton())
		{
			std::string firepoint = (_firePoint) ? *_firePoint: "firepoint";

			// lookup our firepoint
			if(_fireBone == -1) _fireBone = FindBone(firepoint.c_str(), false);

			// lookup our ejectpoint
			if(_ejectBone == -1) _ejectBone = FindBone("ejectpoint", false);
		}

		if(mech)
		{
			vehVehicle* vehicle = mech->GetGameVehicle();

			if(vehicle && vehicle->VehicleType && _spec)
			{
				const std::string& weaponTypeName = _spec->data->config.find("gameweapon");

				if(weaponTypeName.size())
				{
					//std::string weaponTypeName = _spec->data->config["gameweapon"];

					/*
					if(_mountX < 0 || _mountY < 0 || vehicle->VehicleType->Loadout[0].WeaponTypes[_mountX][_mountY] != NULL)
					{
						// lets find the next available mount position
						for(int x = 0; x < 16; x++)
						{
							for(int y = 0; y < 4; y++)
							{
								if(vehicle->VehicleType->Loadout[0].WeaponTypes[x][y] == NULL)
								{
									_mountX = x;
									_mountY = y;
									x = 16; // we're done
									break;
								}
							}
						}
					}
					*/

					// doesn't handle the case were a weapon x and y wasn't found properly

					// get a ref to the game weapon
					_gameWeapon = &vehicle->Weapons[_mountX][_mountY];

					// init the weapon type
					weapFindWeaponTypeByName(&(vehicle->VehicleType->Loadout[0].WeaponTypes[_mountX][_mountY]), (UCHAR*)weaponTypeName.c_str());

					// init the game weapon
					weapInitializeWeapon(_gameWeapon, vehicle->VehicleType->Loadout[0].WeaponTypes[_mountX][_mountY]);

					// set up ammo
					_uses														= _weaponAmmo;
					_gameWeapon->AmmoTypeInUse									= _weaponAmmoType;
					_gameWeapon->NumRoundsAmmoLeft[_gameWeapon->AmmoTypeInUse]	= _weaponAmmo;
					_gameWeapon->NumTotalRounds[_gameWeapon->AmmoTypeInUse]		= _weaponAmmo;

					// ms between cycles
					if(!_cyclePeriod)
						_cyclePeriod = (int)(_gameWeapon->WeaponType->RateOfFire * 1000);

					if(_gameWeapon->WeaponType->FireType > 1)
					{
						_useType = UT_CONTINUOUS;
					}
					else
					{
						_useType = UT_INSTANT;

						if(_recoveryPeriod < _cyclePeriod)
							_recoveryPeriod = _cyclePeriod;
					}
				}
			}
		}

		return true;
	}

	return false;
}

void MechWeapon::Update(FLOAT64 delta)
{
	UpdatePower(delta);

	if(IsActive())
	{
		if(_fireDelay && !_startFireTime)
			_startFireTime = frametime + _fireDelay;

		if(!_startFireTime || frametime >= _startFireTime)
		{
			if(!Fire())
				SetActive(false);
		}
	}

	_lastFireCheckTime = frametime;

	gr2GameObject::Update(delta);
}

// gets the absolute position
bool MechWeapon::GetAbsoluteWorldPosition(FLOAT64VECTOR* position)
{
	return GetParentVehicle()->GetAbsoluteWorldPosition(position);
}

bool MechWeapon::IsReady()
{
	if(!_gameWeapon || !HasAmmo() || IsReloading())
		return false;

	return true;
}

bool MechWeapon::IsReloading()
{
	if(_gameWeapon)
		return weapIsWeaponReloading(_gameWeapon, frametime/1000.0f);

	return false;
}

bool MechWeapon::IsRecycling()
{
	if(_gameWeapon)
	{
		FLOAT64 fireDelta = (frametime - _lastFireCheckTime)/1000.0f;
		return (fireDelta < _gameWeapon->WeaponType->RateOfFire/1000.0f);
	}

	return false;
}

bool MechWeapon::HasAmmo()
{
	if(_gameWeapon)
		return (_gameWeapon->NumRoundsAmmoLeft[_gameWeapon->AmmoTypeInUse]);

	return false;
}

bool MechWeapon::SetActive(bool a)
{
	if(a)
	{
		return MechModule::SetActive(IsReady());
	}
	else
	{
		if(MechModule::SetActive(false))
			SetState("cooldown");

		ClearState("fire");
		_startFireTime = 0;
		return true;
	}
}

void MechWeapon::SetConvergence(piMATRIX* fireMat)
{
	MechVehicle* vehicle	= GetParentVehicle();
	float accuracy			= vehicle->GetAccuracy();

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

		if(_mountY == 1 || _mountX == 1)
			angle *= -1;

		piMATRIX zRot = identity;
		GenerateRotation(2, accuracy * angle, &zRot);
		math3dMatrixMultiply(fireMat, fireMat, &zRot);
	}
}

bool MechWeapon::Fire()
{
	if(IsReady())
	{
		SetState("fire");

		gr2Vehicle* vehicle			= GetParentVehicle();

		FLOAT64 currentTime			= frametime / 1000.0;
		FLOAT64 fireDelta			= (frametime - _lastFireCheckTime - 1) / 1000.0f;

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

			//data->MountIsFiring[_mountX] = 1;
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

		// update the _uses member with the ammo left
		_uses = _gameWeapon->NumRoundsAmmoLeft[_gameWeapon->AmmoTypeInUse];

		return true;
	}

	return false;
}

void MechWeapon::HandleStateEvent(GR2_STATE_EVENT e, gr2State* s)
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
						case MW_ACTION_RELOAD:
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

bool MechWeapon::GetFirePoint(piMATRIX* mat)
{
	return GetBoneTransform(_fireBone, mat);
}

bool MechWeapon::GetEjectPoint(piMATRIX* mat)
{
	return GetBoneTransform(_ejectBone, mat);
}

#define IS_PARAM(x) (!stricmp(name.c_str(), x))

void MechWeapon::AddConfigParam(const std::string& name, const std::string& value)
{
	// 1st crack goes to MechModule
	MechModule::AddConfigParam(name, value);

	// now lets see if we're interested
	if(IS_PARAM("ammo"))			_weaponAmmo			= atoi(value.c_str());
	else if(IS_PARAM("ammotype"))	_weaponAmmoType		= atoi(value.c_str());
	else if(IS_PARAM("firedelay"))	_fireDelay			= atoi(value.c_str());
	else if(IS_PARAM("ejectbone"))	_ejectBone			= FindBone(value.c_str(), true);
	else if(IS_PARAM("firebone"))	_fireBone			= FindBone(value.c_str(), true);
	else if(IS_PARAM("firepoint"))	_firePoint			= &value;
}

void MechWeapon::InitJSProps(jsGR2Object* ui, int index)
{
	// our properties
	char* propNames[] =
	{
		"weapon", "ammotype", "maxammo", "ammo", "reloadtime",
		"weapgroup"
	};

	// loop over and add our properties to the JS object
	JSPropertySpec spec;

	for(int i = 0; i < MechWeapon::TOTAL_PROPS; i++)
	{
		spec.name		= propNames[i];
		spec.tinyid		= i+index;
		spec.flags		= (i == WEAPGROUP_PROP) ? JSPROP_PERMANENT: JSPROP_PERMANENT|JSPROP_READONLY;
		spec.setter		= NULL;
		spec.getter		= NULL;

		// add it
		ui->AddGR2Property(spec);
	}

	// let the base class set up properties
	MechModule::InitJSProps(ui, MechWeapon::TOTAL_PROPS+index);
}

JSBool MechWeapon::GetJSProperty(JSContext* ctx, int propid, jsval* val)
{
	switch(propid)
	{
		case WEAPON_PROP:
		{
			*val = phui::js::ConstString((const char*)_gameWeapon->WeaponType->Name);
		}
		break;
		case AMMOTYPE_PROP:
		{
			*val = phui::js::ConstString((const char*)_gameWeapon->WeaponType->AmmoType[_gameWeapon->AmmoTypeInUse]->Name);
		}
		break;
		case MAXAMMO_PROP:
		{
			*val = INT_TO_JSVAL(_gameWeapon->NumTotalRounds[_gameWeapon->AmmoTypeInUse]);
		}
		break;
		case AMMO_PROP:
		{
			*val = INT_TO_JSVAL(_gameWeapon->NumRoundsAmmoLeft[_gameWeapon->AmmoTypeInUse]);
		}
		break;
		case RELOADTIME_PROP:
		{
			*val = INT_TO_JSVAL(0);
		}
		break;
		case WEAPGROUP_PROP:
		{
			if(_useInput == IMAP_HOLD_MAINTRIGGER)
				*val = INT_TO_JSVAL(1);
			else if (_useInput == IMAP_HOLD_SECONDTRIGGER)
				*val = INT_TO_JSVAL(2);
			else
				*val = INT_TO_JSVAL(0);
		}
		break;
		default:
			return MechModule::GetJSProperty(ctx, propid-MechWeapon::TOTAL_PROPS, val);
		break;
	}

	return JS_TRUE;
}

JSBool MechWeapon::SetJSProperty(JSContext* ctx, int propid, jsval* val)
{
	switch(propid)
	{
		case WEAPGROUP_PROP:
		{
			int32 group = -1;

			if(JS_ValueToInt32(ctx, *val, &group))
				_useInput = (group == 1) ? IMAP_HOLD_MAINTRIGGER: IMAP_HOLD_SECONDTRIGGER;
		}
		break;
		default:
			return MechModule::SetJSProperty(ctx, propid-MechWeapon::TOTAL_PROPS, val);
		break;
	}

	return JS_TRUE;
}

/*
void MechWeapon::RegisterCreator(const char* creatorName, GR2_WEAPON_CREATOR creator)
{
	_creatorMap[creatorName] = creator;
}

MechWeapon* MechWeapon::Create(std::string& creatorName, gr2WeaponSpec* spec)
{
	GR2_WEAPON_CREATOR creator = _creatorMap[creatorName];
	return (creator) ? creator(spec): new gr2Weapon(spec);
}
*/

gr2GameObject* MechWeapon::Create(gr2ModuleSpec* spec)
{
	return new MechWeapon(spec);
}


