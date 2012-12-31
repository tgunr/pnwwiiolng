#include <syscfg.h> // Path is project specific.

#include "mech/MechModule.h"
#include "mech/MechVehicle.h"
#include "gr2/gr2Data.h"
#include "gr2/gr2DamageModel.h"
#include "imap/ww2func.h"
#include "include/vars.h"
#include "js/jsGR2Object.h"

#define MIN_ONLINE_CYCLE_TIME 300

MechModule::MechModule(gr2ModuleSpec* spec) :
	gr2GameObject(),
	_spec(spec),
	_parentMech(NULL),
	_enabled(true),
	_blocked(false),
	_active(false),
	_powered(true),
	_idlePower(0),
	_uses(-1),
	_usePower(0),
	_usedPower(0),
	_usePeriod(0),
	_useTime(0),
	_recoveryPeriod(0),
	_recoveryTime(0),
	_minPower(0),
	_maxPower(0),
	_useInput(-1),
	_enableInput(-1),
	_mountX(-1),
	_mountY(0),
	_attachBone(-1),
	_connectBone(-1),
	_useType(UT_TOGGLE),
	_modified(0),
	_cyclePeriod(0),
	_cycleTime(0),
	_efficiency(1.0f),
	_effMinDamageLevel(1),
	_effDropPerLevel(0),
	_effMinimum(0),
	_effFlags(0)
{
	SetObjectType(GOT_MODULE);
}

MechModule::~MechModule()
{
}

bool MechModule::Init()
{
	if(_inited) return false;

	if(_spec)
	{
		/* we might use these later
		// set the skeleton if we have one
		if(_spec->skeleton.size())
			SetSkeleton(_spec->skeleton.c_str(), true);
		*/

		if(_spec->data)
		{
			// add visual lods
			if(_spec->data->shapes.size())
			{
				MechVehicle* mech = GetParentVehicle();
				const char* shape = (mech) ? mech->GetGameVehicleShape(): "";

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

			// set data config
			gr2ObjectConfig::iterator it = _spec->data->config.begin();
			for(; it != _spec->data->config.end(); it++)
				AddConfigParam(it->first, it->second);
		}

		// set instance config; overrides data config
		gr2ObjectConfig::iterator it = _spec->config.begin();
		for(; it != _spec->config.end(); it++)
			AddConfigParam(it->first, it->second);
	}

	// if we have a visible model, attach it to our parent
	if(GetSkeleton() && _parent && _parent->GetSkeleton())
	{
		// lookup bones if they weren't set by the config params
		if(_connectBone == -1) _connectBone = FindBone("connect", false);
		if(_attachBone == -1) _attachBone = _parent->FindBone("attach", false);

		// try to connect us to our parent
		_parent->AttachObject(this, _connectBone, _attachBone);
	}

	return gr2GameObject::Init();
}

void MechModule::Update(FLOAT64 delta)
{
	UpdatePower(delta);
	gr2GameObject::Update(delta);
}

void MechModule::UpdatePower(FLOAT64 delta)
{
	MechVehicle* mech = GetParentVehicle();

	if(mech)
	{
		if(mech->IsFirstPerson())
		{
			if(IsEnabled())
			{
				// make use of efficiency here
				if(_idlePower)
					_powered = mech->ProcessPower((SINT32)(_idlePower * delta), _minPower, _maxPower);

				if(IsActive())
				{
					if(_cyclePeriod && frametime >= _cycleTime)
						_cycleTime = frametime + _cyclePeriod + ((_cycleTime) ? -(frametime - _cycleTime): 0);

					switch(_useType)
					{
						case UT_INSTANT:
						{
							// instant power use is handled in the Use() method
							if(!GetCycleTime() && (!Online || mech->GetLastNetUpdate() >= _useTime))
								SetActive(false);
						}
						break;
						case UT_TOGGLE:
						case UT_CONTINUOUS:
						{
							// make use of efficiency here
							if(_usePower && !mech->ProcessPower((SINT32)(_usePower * delta), _minPower, _maxPower))
								SetActive(false);
							else if(_usePeriod && !GetUseTime())
								SetActive(false);
						}
						break;
						default:
							// do nothing
						break;
					}
				}

				if(!IsActive())
					_cycleTime = 0;
			}

			mech->UpdateMount(_mountX, IsActive());
		}
		else
		{
			SetActive(mech->GetMountState(_mountX));
		}
	}
}

UINT32 MechModule::ProcessControlInputs(SINT32 param, UINT32 id, UINT32 reserve)
{
	if(GetParentVehicle()->IsFirstPerson())
	{
		UINT32 hits = 0;

		if(EnableOnUse())
		{
			if(id == _useInput)
				hits += Use(param, reserve);
		}
		else
		{
			if(id == _useInput)
				hits += Use(param, reserve);

			if(id == _enableInput)
				hits += Enable(param, reserve);
		}

		return (hits) ? 1: 0;
	}

	return 0;
}

int MechModule::Use(SINT32 on, UINT32 data)
{
	if((!EnableOnUse() && !IsEnabled()) || GetRecoveryTime()) return 0;

	MechVehicle* mech = GetParentVehicle();

	switch(_useType)
	{
		case UT_TOGGLE:
		{
			if(on)
				SetActive(!_active);
		}
		break;
		case UT_CONTINUOUS:
		{
			SetActive(on);
		}
		break;
		case UT_INSTANT:
		{
			if(on)
			{
				SetActive(true);

				// make use of efficiency here
				if(IsActive() && mech->ProcessPower(_usePower, _minPower, _maxPower))
					--_uses;
				else
					SetActive(false);

				_modified = frametime;
			}
		}
		break;
		default:
			// do nothing
		break;
	}

	return 1;
}

int MechModule::Enable(SINT32 on, UINT32 data)
{
	if(on) Toggle();
	return 1;
}

bool MechModule::EnableOnUse() const
{
	return (_enableInput >= 0 && _enableInput == _useInput);
}

MechVehicle* MechModule::GetParentVehicle()
{
	if(!_parentMech)
	{
		gr2Object* parent = _parent;

		while(parent && !parent->IsObjectType(GOT_VEHICLE))
			parent = parent->GetParentObject();

		_parentMech = dynamic_cast<MechVehicle*>(parent);
	}

	return _parentMech;
}

#define IS_PARAM(x) (!stricmp(name.c_str(), x))
#define IS_VALUE(x) (!stricmp(value.c_str(), x))

void MechModule::AddConfigParam(const std::string& name, const std::string& value)
{
	// let the parent use the config if applicable
	if(_parent)
		GetParentVehicle()->AddConfigParam(name, value);

	if(IS_PARAM("idlepower"))				_idlePower			= atoi(value.c_str());
	else if(IS_PARAM("uses"))				_uses				= atoi(value.c_str());
	else if(IS_PARAM("usepower"))			_usePower			= atoi(value.c_str());
	else if(IS_PARAM("recoveryperiod"))		_recoveryPeriod		= atoi(value.c_str());
	else if(IS_PARAM("useperiod"))			_usePeriod			= atoi(value.c_str());
	else if(IS_PARAM("cycleperiod"))		_cyclePeriod		= atoi(value.c_str());
	else if(IS_PARAM("minpower"))			_minPower			= atoi(value.c_str());
	else if(IS_PARAM("maxpower"))			_maxPower			= atoi(value.c_str());
	else if(IS_PARAM("enableinput"))		_enableInput		= atoi(value.c_str());
	else if(IS_PARAM("useinput"))			_useInput			= atoi(value.c_str());
	else if(IS_PARAM("mountx"))				_mountX				= atoi(value.c_str());
	else if(IS_PARAM("mounty"))				_mountY				= atoi(value.c_str());
	else if(IS_PARAM("effminlevel"))		_effMinDamageLevel	= atoi(value.c_str());
	else if(IS_PARAM("effdroplevel"))		_effDropPerLevel	= (float)atof(value.c_str());
	else if(IS_PARAM("effminimum"))			_effMinimum			= (float)atof(value.c_str());
	else if(IS_PARAM("attach") && _parent)	_attachBone			= _parent->FindBone(value.c_str(), true);
	else if(IS_PARAM("connect"))			_connectBone		= FindBone(value.c_str(), true);
	else if(IS_PARAM("control"))			SetControl(1<<atoi(value.c_str()));
	else if(IS_PARAM("condition"))			SetCondition(1<<atoi(value.c_str()));
	else if(IS_PARAM("action"))				SetAction(1<<atoi(value.c_str()));
	else if(IS_PARAM("blocks"))				_blockModules.push_back(value);
	else if(IS_PARAM("enabled"))			SetEnabled(!stricmp(value.c_str(), "true"));
	else if(IS_PARAM("usetype"))
	{
		if(IS_VALUE("toggle"))				_useType			= UT_TOGGLE;
		else if(IS_VALUE("continuous"))		_useType			= UT_CONTINUOUS;
		else if(IS_VALUE("instant"))		_useType			= UT_INSTANT;
	}
	else if(IS_PARAM("component"))
	{
		gr2DamageComponent* component = GetParentVehicle()->GetDamageModel()->LookupComponent(value);
		if(component) component->AddListener(this);
	}
}

void MechModule::SetMount(int mx, int my)
{
	_mountX = (mx >= 0 && mx < 16) ? mx: 0;
	_mountY = (my >= 0 && my < 4) ? my: 0;
}

int MechModule::GetMountX() const
{
	return _mountX;
}

int MechModule::GetMountY() const
{
	return _mountY;
}


float MechModule::GetCycleTime() const
{
	return (_cyclePeriod && frametime <= _cycleTime) ? (float)(_cycleTime - frametime)/_cyclePeriod: 0;
}

float MechModule::GetUseTime() const
{
	return (_usePeriod && frametime <= _useTime) ? (float)(_useTime - frametime)/_usePeriod: 0;
}

float MechModule::GetRecoveryTime() const
{
	return (_recoveryPeriod && frametime <= _recoveryTime) ? (float)(_recoveryTime - frametime)/_recoveryPeriod: 0;
}

bool MechModule::Toggle()
{
	SetEnabled(!_enabled);
	return IsEnabled();
}

// is this module enabled?
bool MechModule::IsEnabled() const
{
	return (_uses && _enabled && !_blocked);
}

bool MechModule::IsBlocked() const
{
	return _blocked;
}

bool MechModule::SetBlocked(bool b)
{
	_blocked = b;
	return _blocked;
}

// is this module active?
bool MechModule::IsActive() const
{
	return (_active && IsPowered() && IsEnabled());
}

bool MechModule::IsPowered() const
{
	return _powered;
}

bool MechModule::IsUseType(USE_TYPE type) const
{
	return (_useType == type);
}

bool MechModule::SetActive(bool a)
{
	bool changed = (_active != a);

	_active = a;

	MechVehicle* mech = GetParentVehicle();

	if(mech && mech->IsFirstPerson())
	{
		mech->UpdateMount(_mountX, _active);

		if(changed)
		{
			bool block = false;

			if(EnableOnUse())
				SetEnabled(_active);

			if(_active)
			{
				if(_flags[GFT_CONTROL])		mech->SetControl(_flags[GFT_CONTROL]);
				if(_flags[GFT_CONDITION])	mech->SetCondition(_flags[GFT_CONDITION]);
				if(_flags[GFT_ACTION])		mech->SetAction(_flags[GFT_ACTION]);

				_useTime = frametime + _usePeriod;
				_recoveryTime = 0;

				block = IsEnabled();
			}
			else
			{
				if(_flags[GFT_CONTROL])		mech->ClearControl(_flags[GFT_CONTROL]);
				if(_flags[GFT_CONDITION])	mech->ClearCondition(_flags[GFT_CONDITION]);
				if(_flags[GFT_ACTION])		mech->ClearAction(_flags[GFT_ACTION]);

				_useTime = 0;
				_recoveryTime = frametime + _recoveryPeriod;
			}

			// block or unblock sibling modules
			MechModule* module = NULL;

			for(int i = 0; i < (int)_blockModules.size(); i++)
			{
				const std::string& name = _blockModules.at(i);

				gr2Object::Attachments& attached = mech->GetAttachments();

				for(int j = 0; j < (int)attached.size(); j++)
				{
					gr2Object* object = attached.at(j).object;

					if(object && (object->IsObjectType(GOT_WEAPON) || object->IsObjectType(GOT_MODULE)))
					{
						MechModule* module = dynamic_cast<MechModule*>(object);

						if(module && !stricmp(module->_spec->name.c_str(), name.c_str()))
						{
							module->SetBlocked(block);
							break;
						}
					}
				}
			}

			// update the last mod time
			_modified = frametime;
		}
	}

	return changed;
}

bool MechModule::SetEnabled(bool e)
{
	bool changed = (_enabled != e);

	if(changed)
	{
		_enabled	= e;
		_modified	= frametime;
	}

	return changed;
}

void MechModule::SetEfficiency(float e)
{
	_efficiency = e;
}

float MechModule::GetEfficiency() const
{
	return _efficiency;
}

float MechModule::CalculateEfficiency(float value) const
{
	return (value < 0) ? value + ((1 - _efficiency) * value): _efficiency * value;
}

void MechModule::SetAttachBone(const char* bone)
{
	if(_parent)
		SetAttachBone(_parent->FindBone(bone));
}

void MechModule::SetAttachBone(int bone)
{
	_attachBone = bone;
}

void MechModule::InitJSProps(jsGR2Object* ui, int index)
{
	// define dynamic properties
	char* propNames[] =
	{
		"name", "ui", "enabled", "powered", "active", "usepower",
		"idlepower", "minpower", "mountx", "mounty",
		"usetype", "usetime", "recoverytime", "modified",
		"useperiod", "recoveryperiod", "uses", "cycletime",
		"efficiency", "maxpower"
	};

	JSPropertySpec spec;

	for(int i = 0; i < MechModule::TOTAL_PROPS; i++)
	{
		spec.name		= propNames[i];
		spec.tinyid		= i+index;
		spec.flags		= JSPROP_PERMANENT|JSPROP_READONLY;
		spec.setter		= NULL;
		spec.getter		= NULL;

		// add it
		ui->AddGR2Property(spec);
	}

	gr2GameObject::InitJSProps(ui, MechModule::TOTAL_PROPS+index);
}

JSBool MechModule::GetJSProperty(JSContext* ctx, int propid, jsval* val)
{
	switch(propid)
	{
		case NAME_PROP:
		{
			*val = phui::js::ConstString(_spec->name.c_str());
		}
		break;
		case UI_PROP:
		{
			*val = phui::js::ConstString(_spec->config.find("ui").c_str());
		}
		break;
		case ENABLED_PROP:
		{
			*val = BOOLEAN_TO_JSVAL(IsEnabled());
		}
		break;
		case POWERED_PROP:
		{
			*val = BOOLEAN_TO_JSVAL(IsPowered());
		}
		break;
		case ACTIVE_PROP:
		{
			*val = BOOLEAN_TO_JSVAL(IsActive());
		}
		break;

		case USEPOWER_PROP:
		{
			*val = INT_TO_JSVAL(_usePower);
		}
		break;
		case IDLEPOWER_PROP:
		{
			*val = INT_TO_JSVAL(_idlePower);
		}
		break;
		case MINPOWER_PROP:
		{
			*val = INT_TO_JSVAL(_minPower);
		}
		break;
		case MOUNTX_PROP:
		{
			*val = INT_TO_JSVAL(_mountX);
		}
		break;
		case MOUNTY_PROP:
		{
			*val = INT_TO_JSVAL(_mountY);
		}
		break;
		case USETYPE_PROP:
		{
			*val = INT_TO_JSVAL(_useType);
		}
		break;
		case USETIME_PROP:
		{
			JS_NewDoubleValue(ctx, GetUseTime(), val);
		}
		break;
		case RECOVERYTIME_PROP:
		{
			JS_NewDoubleValue(ctx, GetRecoveryTime(), val);
		}
		break;
		case MODIFIED_PROP:
		{
			*val = INT_TO_JSVAL(_modified);
		}
		break;
		case USEPERIOD_PROP:
		{
			*val = INT_TO_JSVAL(_usePeriod);
		}
		break;
		case RECOVERYPERIOD_PROP:
		{
			*val = INT_TO_JSVAL(_recoveryPeriod);
		}
		break;
		case USES_PROP:
		{
			*val = INT_TO_JSVAL(_uses);
		}
		break;
		case CYCLETIME_PROP:
		{
			JS_NewDoubleValue(ctx, GetCycleTime(), val);
		}
		break;
		case EFFICIENCY_PROP:
		{
			JS_NewDoubleValue(ctx, _efficiency, val);
		}
		break;
		case MAXPOWER_PROP:
		{
			*val = INT_TO_JSVAL(_maxPower);
		}
		break;
		default:
			return gr2GameObject::GetJSProperty(ctx, propid-MechModule::TOTAL_PROPS, val);
		break;
	}

	return JS_TRUE;
}

void MechModule::HandleDamage(gr2DamageComponent* component)
{
	if(component->GetDamageLevel() >= _effMinDamageLevel)
	{
		float decrease = (_effDropPerLevel) ? _effDropPerLevel: 1 / component->GetDamageModel()->GetLevels();
		_efficiency = max(_efficiency - decrease, _effMinimum);
	}
}

gr2GameObject* MechModule::Create(gr2ModuleSpec* spec)
{
	return new MechModule(spec);
}
