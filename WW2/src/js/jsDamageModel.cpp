#include <syscfg.h> // Path is project specific.
#include <string.h>

#include "js/jsDamageModel.h"

/************************************************************************************************
	jsDamageModel
*************************************************************************************************/

JSClass jsDamageModel::jsclass = JS_CLASS_MACRO("damagemodel");
JSFunctionSpec jsDamageModel::jsfunctions[] =
{
	{0}
};

JSPropertySpec jsDamageModel::jsproperties[] =
{
	{"points",			jsDamageModel::POINTS_PROP,   	JSPROP_PERMANENT | JSPROP_READONLY},
	{"levels",			jsDamageModel::LEVELS_PROP,   	JSPROP_PERMANENT | JSPROP_READONLY},
	{"components",		jsDamageModel::COMPONENTS_PROP,	JSPROP_PERMANENT | JSPROP_READONLY},
	{0}
};

jsDamageModel::jsDamageModel(gr2DamageModel* m) : phui::js::Object(&jsclass,jsfunctions,jsproperties), _model(m)
{
	JS_CONSTRUCTOR_LOCK;
	_components = new phui::js::Array;
};

JSBool jsDamageModel::GetProperty(JSContext* ctx, int propid, jsval* val)
{
	switch(propid)
	{
		case POINTS_PROP:
		{
			*val = INT_TO_JSVAL(0);
		}
		break;
		case LEVELS_PROP:
		{
			*val = INT_TO_JSVAL(_model->GetLevels());
		}
		break;
		case COMPONENTS_PROP:
		{
			if(!_components->size())
			{
				gr2ComponentsList& components = _model->GetComponents();

				for(UINT32 i = 0; i < (UINT32)components.size(); i++)
					_components->SetElement(new jsDamageComponent(components.at(i)), i);
			}

			*val = _components->jsVal();
		}
		break;
		default:
			return JSVAL_FALSE;
		break;
	}

	return JS_TRUE;
}

/************************************************************************************************
	jsDamageComponent
*************************************************************************************************/

JSClass jsDamageComponent::jsclass = JS_CLASS_MACRO("damagecomponent");
JSFunctionSpec jsDamageComponent::jsfunctions[] =
{
	{0}
};

JSPropertySpec jsDamageComponent::jsproperties[] =
{
	{"points",			jsDamageComponent::POINTS_PROP,   	JSPROP_PERMANENT | JSPROP_READONLY},
	{"level",			jsDamageComponent::LEVEL_PROP,   	JSPROP_PERMANENT | JSPROP_READONLY},
	{"name",			jsDamageComponent::NAME_PROP,   	JSPROP_PERMANENT | JSPROP_READONLY},
	{"destroyed",		jsDamageComponent::DESTROYED_PROP, 	JSPROP_PERMANENT | JSPROP_READONLY},
	{"logical",			jsDamageComponent::LOGICAL_PROP,   	JSPROP_PERMANENT | JSPROP_READONLY},
	{"components",		jsDamageComponent::COMPONENTS_PROP,	JSPROP_PERMANENT | JSPROP_READONLY},
	{0}
};

jsDamageComponent::jsDamageComponent(gr2DamageComponent* c) : phui::js::Object(&jsclass,jsfunctions,jsproperties), _component(c)
{
	JS_CONSTRUCTOR_LOCK;
	_components = new phui::js::Array;
};

JSBool jsDamageComponent::GetProperty(JSContext* ctx, int propid, jsval* val)
{
	switch(propid)
	{
		case POINTS_PROP:
		{
			*val = INT_TO_JSVAL(0);
		}
		break;
		case LEVEL_PROP:
		{
			*val = INT_TO_JSVAL(_component->GetDamageLevel());
		}
		break;
		case NAME_PROP:
		{
			*val = phui::js::ConstString(_component->GetName().c_str());
		}
		break;
		case DESTROYED_PROP:
		{
			*val = BOOLEAN_TO_JSVAL(_component->IsDestroyed());
		}
		break;
		case LOGICAL_PROP:
		{
			*val = BOOLEAN_TO_JSVAL(_component->IsLogicalComponent());
		}
		break;
		case COMPONENTS_PROP:
		{
			if(!_components->size())
			{
				gr2ComponentsList& components = _component->GetComponents();

				for(UINT32 i = 0; i < (UINT32)components.size(); i++)
					_components->SetElement(new jsDamageComponent(components.at(i)), i);
			}

			*val = _components->jsVal();
		}
		break;
		default:
			return JSVAL_FALSE;
		break;
	}

	return JS_TRUE;
}
