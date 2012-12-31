#pragma once

#include "phui/Javascript.h"
#include "gr2/gr2GameObject.h"

/************************************************************************************************
	jsGR2Object
*************************************************************************************************/

#define MAX_JS_PROPERTIES 128

class jsGR2Object : public phui::js::Object
{
public:

	enum
	{
		MODULES_PROP,
		TOTAL_PROPS
	};

	jsGR2Object(gr2GameObject* o);
	jsGR2Object(JSClass* c, gr2GameObject* o);

	virtual void InitJSProps(int index = 0);
	virtual JSBool GetProperty(JSContext* ctx, int propid, jsval* val);
	virtual JSBool SetProperty(JSContext* ctx, int propid, jsval* val);
	void AddGR2Property(JSPropertySpec& spec);

	gr2GameObject* object;

	static JSClass jsclass;

protected:
	~jsGR2Object();

	// properties
	JSPropertySpec _jsProperties[MAX_JS_PROPERTIES];

	phui::js::array_root_ptr _attached;
};

