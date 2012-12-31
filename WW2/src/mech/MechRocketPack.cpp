#include <syscfg.h> // Path is project specific.
#include "mech/MechRocketPack.h"
#include "mech/MechVehicle.h"
#include "gr2/gr2Data.h"
#include "veh/vehmain.h"
#include "include/vars.h"
#include "math/math3d.h"
#include "math/mathvect.h"
#include "pers/persGunsight.h"
#include "math/mathUnitConversion.h"
#include "js/jsGR2Object.h"

MechRocketPack::MechRocketPack(gr2ModuleSpec* spec) :
	MechWeapon(spec),
	_fireStage(FIRE_STAGE_READY),
	_salvo(1),
	_setupDelay(0),
	_stageTime(0),
	_currentTube(-1),
	_shortRange(0),
	_fireAngle(0),
	_wantedAngle(0),
	_turretIndex(-1),
	_srMountX(-1),
	_srFire(false),
	_tubeReloadTime(0),
	_tubeReloadPeriod(0)
{
}

MechRocketPack::~MechRocketPack()
{
}

#define IS_PARAM(x) (!stricmp(name.c_str(), x))

void MechRocketPack::AddConfigParam(const std::string& name, const std::string& value)
{
	// 1st crack goes to MechWeapon
	MechWeapon::AddConfigParam(name, value);

	// now lets see if we're interested
	if(IS_PARAM("salvo"))			_salvo				= atoi(value.c_str());
	else if(IS_PARAM("setupdelay"))	_setupDelay			= atoi(value.c_str());
	else if(IS_PARAM("turret"))		_turretIndex		= atoi(value.c_str());
	else if(IS_PARAM("srmountx"))	_srMountX			= atoi(value.c_str());
	else if(IS_PARAM("shortrange"))	_shortRange			= (float)atof(value.c_str());
}

bool MechRocketPack::Init()
{
	if(MechWeapon::Init())
	{
		char name[25+1];
		std::string firepoint	= (_firePoint) ? *_firePoint: "firepoint";
		int lastFound			= -1;
		int bone				= -1;

		for(int i = 0; i < _salvo; i++)
		{
			// firepoint names are like Firepoint_Left_01 ...
			snprintf(name, 25, (i + 1 < 10) ? "%s0%d": "%s%d", firepoint.c_str(), i + 1);

			bone = gr2ModelFindBone(GetSkeleton(), name, true);

			if(bone == -1 && lastFound >= 0)
				bone = lastFound;

			if(bone >= 0)
			{
				lastFound = bone;

				RocketTube tube;

				// create a name for it
				snprintf(name, 25, "tube%d", i);

				tube.name = name;
				tube.firebone = bone;
				tube.loaded = true;

				_tubes.push_back(tube);
			}
		}

		_useType = UT_TOGGLE;

		// short range ammo
		_gameWeapon->NumRoundsAmmoLeft[1]	= _weaponAmmo;
		_gameWeapon->NumTotalRounds[1]		= _weaponAmmo;

		return true;
	}

	return false;
}

void MechRocketPack::Update(FLOAT64 delta)
{
	UpdatePower(delta);

	if(IsActive())
	{
		MechVehicle* mech = GetParentVehicle();

		// set the mount flag if we're firing short range
		if(_srFire && mech->IsFirstPerson())
			mech->UpdateMount(_srMountX, true);

		switch(_fireStage)
		{
			case FIRE_STAGE_READY:
			{
				SetFireStage(FIRE_STAGE_INIT);
			}
			break;
			case FIRE_STAGE_INIT:
			{
				DoSetupStage(delta);
			}
			break;
			case FIRE_STAGE_SETUP:
			{
				if(IsStageComplete())
					SetFireStage(FIRE_STAGE_FIRE);
				else
					DoSetupStage(delta);
			}
			break;
			case FIRE_STAGE_FIRE:
			{
				// this call sets the next stage when its done
				DoFireStage(delta);
			}
			break;
			case FIRE_STAGE_RELOAD:
			{
				DoReloadStage(delta);
			}
			break;
		}

		_lastFireCheckTime = frametime;
	}
	else if(_fireStage == FIRE_STAGE_RELOAD)
	{
		if(IsStageComplete())
			ResetFireStage();
		else
			DoReloadStage(delta);
	}

	gr2GameObject::Update(delta);
}

bool MechRocketPack::Fire()
{
	_fireBone = -1;

	if(IsValidTube() && _tubes.at(_currentTube).loaded)
		_fireBone = _tubes.at(_currentTube).firebone;

	if(_fireBone >= 0 && MechWeapon::Fire())
	{
		// hide the rocket mesh
		_tubes.at(_currentTube).loaded = false;
		return true;
	}

	return false;
}

void MechRocketPack::HandleStateEvent(GR2_STATE_EVENT e, gr2State* s)
{
	switch(s->flagType)
	{
		case GFT_ACTION:
		{
			switch(e)
			{
				case GSE_START:
				{
					switch(s->value)
					{
						case MW_ACTION_FIRE:
						{
							if(IsValidTube())
							{
								for(int i = 0; i < (int)s->states.size(); i++)
								{
									if(s->states[i]->type == GST_EFFECT)
									{
										gr2EffectState* state = (gr2EffectState*)(s->states[i]);
										state->bone = _tubes.at(_currentTube).firebone;
									}
								}
							}
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

	MechWeapon::HandleStateEvent(e, s);
}

bool MechRocketPack::IsValidTube() const
{
	return (_currentTube >= 0 && _currentTube < (int)_tubes.size());
}

void MechRocketPack::ResetFireStage()
{
	// reset members
	_srFire = false;
	_currentTube = -1;
	_tubeReloadTime = 0;
	_tubeReloadPeriod = 0;

	// set the default fire stage
	SetFireStage(FIRE_STAGE_READY);

	// make sure we're as loaded as we can be
	ReloadAllTubes();

	if(GetParentVehicle()->IsFirstPerson() && IsActive())
		SetActive(false);
}

void MechRocketPack::SetFireStage(FIRE_STAGE s, int duration)
{
	_fireStage = s;
	_stageTime = frametime + duration;
}

bool MechRocketPack::IsStageComplete() const
{
	return (frametime >= _stageTime);
}

// rocket the pack
void MechRocketPack::DoSetupStage(FLOAT64 delta)
{
	MechVehicle* mech = GetParentVehicle();

	if(_fireStage == FIRE_STAGE_INIT)
	{
		SetFireStage(FIRE_STAGE_SETUP, _setupDelay);

		if(mech->IsFirstPerson())
		{
			FLOAT32 range = mech->GetAimRange();

			_srFire = (_shortRange && range <= _shortRange);

			// select the proper ammo type to use
			_gameWeapon->AmmoTypeInUse = (_srFire) ? 1: 0;

			SINT8* ammo = (SINT8*)_gameWeapon->WeaponType->AmmoType[_gameWeapon->AmmoTypeInUse]->Name;

			// we now have our fire angle
			persGunsightCalcSightAngle(&_wantedAngle, ammo, range);

			if(_srFire)
				_wantedAngle = -_wantedAngle;

			gr2Vehicle::gr2Turret* turret = mech->GetTurret(_turretIndex);
			if(turret) turret->manualElevate = RAD_TO_DEG(_wantedAngle);

			if(_srFire)
				mech->UpdateMount(_srMountX, true);
		}
		else
		{
			_gameWeapon->AmmoTypeInUse = (mech->GetMountState(_srMountX)) ? 1: 0;
		}
	}
}

// fire the salvo
void MechRocketPack::DoFireStage(FLOAT64 delta)
{
	// init the first tube
	if(!_startFireTime)
	{
		_startFireTime = frametime + _fireDelay;
		++_currentTube;
	}

	if(frametime >= _startFireTime)
	{
		// fire the next rocket if its ready
		if(Fire())
		{
			// set the next fire time
			_startFireTime = frametime + _fireDelay;
			++_currentTube;
		}
		else
		{
			SetFireStage(FIRE_STAGE_RELOAD, _recoveryPeriod);
		}
	}
}

// rotate and reload the pack
void MechRocketPack::DoReloadStage(FLOAT64 delta)
{
	if(GetParentVehicle()->IsFirstPerson() && IsActive())
		SetActive(false);

	_srFire			= false;
	_wantedAngle	= 0;

	gr2Vehicle::gr2Turret* turret = GetParentVehicle()->GetTurret(_turretIndex);
	if(turret) turret->manualElevate = _wantedAngle;

	if(!_tubeReloadTime)
	{
		_tubeReloadPeriod = (_recoveryPeriod - (int)(_recoveryPeriod * .2))/_tubes.size();
		_tubeReloadTime = frametime + _tubeReloadPeriod;
	}

	if(frametime >= _tubeReloadTime)
	{
		// reload the next tube
		for(int i = 0; i < (int)_tubes.size(); i++)
		{
			if(i < _gameWeapon->NumRoundsAmmoLeft[0] && !_tubes.at(i).loaded)
			{
				_tubes.at(i).loaded = true;
				break;
			}
		}

		_tubeReloadTime = frametime + _tubeReloadPeriod;
	}
}

void MechRocketPack::RotatePack(FLOAT64 delta)
{
	if(_fireAngle < _wantedAngle)
	{
		//_fireAngle += (FLOAT32)(_wantedAngle * (delta/_setupDelay));
		_fireAngle = _wantedAngle;
		if(_fireAngle > _wantedAngle) _fireAngle = _wantedAngle;
	}
	else if(_fireAngle > _wantedAngle)
	{
		//_fireAngle -= (FLOAT32)(_wantedAngle * (delta/_setupDelay));
		_fireAngle = _wantedAngle;
		if(_fireAngle < _wantedAngle) _fireAngle = _wantedAngle;
	}

	if(_fireAngle)
		GetParentVehicle()->RotateBoneLocally(_attachBone, 0, _fireAngle);
}

void MechRocketPack::ReloadAllTubes()
{
	for(int i = 0; i < (int)_tubes.size(); i++)
	{
		if(i < _gameWeapon->NumRoundsAmmoLeft[0] && !_tubes.at(i).loaded)
			_tubes.at(i).loaded = true;
	}
}

void MechRocketPack::InitJSProps(jsGR2Object* ui, int index)
{
	// our properties
	char* propNames[] =
	{
		"salvo", "currenttube", "shortrange", "firestage", "numloaded",
	};

	// loop over and add our properties to the JS object
	JSPropertySpec spec;

	// add standard props
	for(int i = 0; i < MechRocketPack::TOTAL_PROPS; i++)
	{
		spec.name		= propNames[i];
		spec.tinyid		= i+index;
		spec.flags		= JSPROP_PERMANENT|JSPROP_READONLY;
		spec.setter		= NULL;
		spec.getter		= NULL;

		// add it
		ui->AddGR2Property(spec);
	}

	// now add in the dynamic tube props
	for(int i = 0; i < (int)_tubes.size(); i++)
	{
		// create the prop name
		spec.name		= _tubes.at(i).name.c_str();
		spec.tinyid		= i+index+MechRocketPack::TOTAL_PROPS;
		spec.flags		= JSPROP_PERMANENT|JSPROP_READONLY;
		spec.setter		= NULL;
		spec.getter		= NULL;

		// add it
		ui->AddGR2Property(spec);
	}

	// let the base class set up properties
	MechWeapon::InitJSProps(ui, MechRocketPack::TOTAL_PROPS+index+_tubes.size());
}

JSBool MechRocketPack::GetJSProperty(JSContext* ctx, int propid, jsval* val)
{
	switch(propid)
	{
		case SALVO_PROP:
		{
			*val = INT_TO_JSVAL(_salvo);
		}
		break;
		case CURRENTTUBE_PROP:
		{
			*val = INT_TO_JSVAL(_currentTube);
		}
		break;
		case SHORTRANGE_PROP:
		{
			JS_NewDoubleValue(ctx, _shortRange, val);
		}
		break;
		case FIRESTAGE_PROP:
		{
			*val = INT_TO_JSVAL(_fireStage);
		}
		break;
		case NUMLOADED_PROP:
		{
			int loaded = 0;

			for(int i = 0; i < (int)_tubes.size(); i++)
			{
				if(_tubes.at(i).loaded)
					++loaded;
			}

			*val = INT_TO_JSVAL(loaded);
		}
		break;
		default:
		{
			int tubeProp = propid - MechRocketPack::TOTAL_PROPS;

			// is this a tube prop?
			if(tubeProp >= 0 && tubeProp < (int)_tubes.size())
			{
				if(_currentTube == tubeProp)
				{
					*val = phui::js::ConstString("fire");
				}
				else if(_tubes.at(tubeProp).loaded)
					*val = phui::js::ConstString("loaded");
				else
					*val = phui::js::ConstString("empty");

				return JS_TRUE;
			}

			// its not, let the underlying weapon class handle it
			return MechWeapon::GetJSProperty(ctx, tubeProp-_tubes.size(), val);
		}
		break;
	}

	return JS_TRUE;
}

gr2GameObject* MechRocketPack::Create(gr2ModuleSpec* spec)
{
	return new MechRocketPack(spec);
}