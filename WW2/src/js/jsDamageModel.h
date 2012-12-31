#pragma once

#include "phui/Javascript.h"
#include "gr2/gr2DamageModel.h"

/************************************************************************************************
	jsDamageModel
*************************************************************************************************/

class jsDamageModel : public phui::js::Object
{
public:

	enum
	{
		POINTS_PROP,
		LEVELS_PROP,
		COMPONENTS_PROP
	};

	jsDamageModel(gr2DamageModel* m);
	JS_CLASS_DATA;

	virtual JSBool GetProperty(JSContext* ctx, int propid, jsval* val);

protected:
	virtual ~jsDamageModel(){}

	gr2DamageModel* _model;

	phui::js::array_root_ptr _components;
};

/************************************************************************************************
	jsDamageComponent
*************************************************************************************************/

class jsDamageComponent : public phui::js::Object
{
public:

	enum
	{
		POINTS_PROP,
		LEVEL_PROP,
		NAME_PROP,
		DESTROYED_PROP,
		LOGICAL_PROP,
		COMPONENTS_PROP
	};

	jsDamageComponent(gr2DamageComponent* c);
	JS_CLASS_DATA;

	virtual JSBool GetProperty(JSContext* ctx, int propid, jsval* val);

protected:
	virtual ~jsDamageComponent(){}

	gr2DamageComponent* _component;

	phui::js::array_root_ptr _components;
};

