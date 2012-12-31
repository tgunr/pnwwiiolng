#include <syscfg.h> // Path is project specific.

#include "mech/MechModule.h"
#include "mech/MechVehicle.h"
#include "gr2/gr2Data.h"
#include "gr2/gr2DamageModel.h"
#include "imap/ww2func.h"
#include "include/vars.h"
#include "js/jsGR2Object.h"

MechModule::MechModule(gr2ModuleSpec* spec) :
	gr2GameObject(),
	_spec(spec),
	_parentMech(NULL),
	_enabled(true),
	_active(false),
	_powered(false),
	_idlePower(0),
	_uses(-1),
	_usePower(0),
	_usedPower(0),
	_usePeriod(0),
	_rechargePeriod(0),
	_rechargedTime(0),
	_minPower(0),
	_slot(-1),
	_mountX(-1),
	_mountY(0),
	_attachBone(-1),
	_connectBone(-1),
	_useType(UT_TOGGLE),
	_lastActivated(0)
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
			if(_idlePower && IsEnabled())
				_powered = mech->ProcessPower((SINT32)(_idlePower * delta), _minPower);

			if(IsActive() && _useType == UT_INSTANT && mech->GetGameVehicleData()->LastVehicleUpdateTime > _lastActivated)
				SetActive(false);

			mech->UpdateMount(_mountX, IsActive());
		}
		else
		{
			_active = mech->GetMountState(_mountX);
		}
	}
}

UINT32 MechModule::ProcessControlInputs(SINT32 param, UINT32 id, UINT32 reserve)
{
	// modules with a slot specified can simply be activated via keys 1 through 0
	UINT32 handled = 0;

	if(GetParentVehicle()->IsFirstPerson())
	{
		switch (id)
		{
			case IMAP_HOLD_MAINTRIGGER:
			case IMAP_HOLD_SECONDTRIGGER:
			case IMAP_MECH_SLOT0:
			case IMAP_MECH_SLOT1:
			case IMAP_MECH_SLOT2:
			case IMAP_MECH_SLOT3:
			case IMAP_MECH_SLOT4:
			case IMAP_MECH_SLOT5:
			case IMAP_MECH_SLOT6:
			case IMAP_MECH_SLOT7:
			case IMAP_MECH_SLOT8:
			case IMAP_MECH_SLOT9:
			{
				UINT32 input = id;

				if(input == IMAP_HOLD_MAINTRIGGER) input = IMAP_MECH_SLOT0;
				if(input == IMAP_HOLD_SECONDTRIGGER) input = IMAP_MECH_SLOT1;

				// handle my input only
				if(input == IMAP_MECH_SLOT0 + _slot)
				{
					Use(param, reserve);
					handled = 1;
				}
			}
			break;
			default:
				// input not used
			break;
		}
	}

	return handled;
}

void MechModule::Use(SINT32 on, UINT32 data)
{
	MechVehicle* mech	= GetParentVehicle();
	bool lastActive		= _active;

	switch(_useType)
	{
		case UT_TOGGLE:
		{
			if(on)
				SetActive(Toggle());
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
				SetActive(true);
		}
		break;
	}

	// process the use power if we have any
	if(!lastActive && IsActive() && !mech->ProcessPower(_usePower, _minPower))
		SetActive(false); // SetActive will update the mount if needed
	else
		mech->UpdateMount(_mountX, IsActive());
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
	else if(IS_PARAM("rechargeperiod"))		_rechargePeriod		= atoi(value.c_str());
	else if(IS_PARAM("minpower"))			_minPower			= atoi(value.c_str());
	else if(IS_PARAM("slot"))				_slot				= atoi(value.c_str());
	else if(IS_PARAM("mountx"))				_mountX				= atoi(value.c_str());
	else if(IS_PARAM("mounty"))				_mountY				= atoi(value.c_str());
	else if(IS_PARAM("attach") && _parent)	_attachBone			= _parent->FindBone(value.c_str(), true);
	else if(IS_PARAM("connect"))			_connectBone		= FindBone(value.c_str(), true);
	else if(IS_PARAM("usetype"))
	{
		if(IS_VALUE("toggle"))				_useType			= UT_TOGGLE;
		else if(IS_VALUE("continuous"))		_useType			= UT_CONTINUOUS;
		else if(IS_VALUE("instant"))		_useType			= UT_INSTANT;
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

bool MechModule::Toggle()
{
	_enabled = !_enabled;
	return _enabled;
}

// is this module enabled?
bool MechModule::IsEnabled() const
{
	return _enabled;
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

bool MechModule::SetActive(bool a)
{
	bool changed = (_active != a);

	_active = a;

	MechVehicle* mech = GetParentVehicle();

	if(mech && mech->IsFirstPerson())
		mech->UpdateMount(_mountX, _active);

	if(changed)
		_lastActivated = (_active) ? frametime: 0;

	return changed;
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
		"idlepower", "minpower", "slot", "mountx", "mounty", "usetype"
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
			gr2ObjectConfig::iterator found = _spec->config.find("ui");

			if(found != _spec->config.end())
				*val = phui::js::ConstString(found->second.c_str());
			else
				*val = phui::js::ConstString("");
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
		case SLOT_PROP:
		{
			*val = INT_TO_JSVAL(_slot);
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
		default:
			return gr2GameObject::GetJSProperty(ctx, propid-MechModule::TOTAL_PROPS, val);
		break;
	}

	return JS_TRUE;
}

gr2GameObject* MechModule::Create(gr2ModuleSpec* spec)
{
	return new MechModule(spec);
}
