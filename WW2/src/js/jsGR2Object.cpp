#include <syscfg.h> // Path is project specific.
#include <string.h>

#include "js/jsGR2Object.h"
#include "js/jsClient.h"
#include "fio/fio.h"
#include "ui/uiMain.h"

/************************************************************************************************
	jsGR2Object
*************************************************************************************************/

JSClass jsGR2Object::jsclass = JS_CLASS_MACRO("gr2object");

jsGR2Object::jsGR2Object(gr2GameObject* o) : phui::js::Object(&jsclass), object(o)
{
	JS_CONSTRUCTOR_LOCK;
	memset(_jsProperties, 0, MAX_JS_PROPERTIES*sizeof(JSPropertySpec));
	InitJSProps();
};

jsGR2Object::jsGR2Object(JSClass* c, gr2GameObject* o) : phui::js::Object(c), object(o)
{
	JS_CONSTRUCTOR_LOCK;
	memset(_jsProperties, 0, MAX_JS_PROPERTIES*sizeof(JSPropertySpec));
};

void jsGR2Object::InitJSProps(int index)
{
	JSPropertySpec spec;

	spec.name		= "modules";
    spec.tinyid		= jsGR2Object::MODULES_PROP + index;
    spec.flags		= JSPROP_PERMANENT|JSPROP_READONLY;
	spec.setter		= NULL;
	spec.getter		= NULL;

	// add our property
	AddGR2Property(spec);

	// let the underlying object setup additional props
	object->InitJSProps(this, jsGR2Object::TOTAL_PROPS + index);

	// define the props in the JS object
	DefineProperties(_jsProperties);

	_attached = new phui::js::Array;
}

jsGR2Object::~jsGR2Object()
{
}

JSBool jsGR2Object::GetProperty(JSContext* ctx, int propid, jsval* val)
{
	switch(propid)
	{
		case MODULES_PROP:
		{
			if(!_attached->size())
			{
				gr2Object::Attachments& attachments = object->GetAttachments();
				UINT32 index = 0;

				for(UINT32 i = 0; i < (UINT32)attachments.size(); i++)
				{
					gr2GameObject* child = dynamic_cast<gr2GameObject*>(attachments[i].object);
					if(!child) continue;

					_attached->SetElement(new jsGR2Object(child), index);

					++index;
				}
			}

			*val = _attached->jsVal();
		}
		break;
		default:
			return object->GetJSProperty(_jsctx, propid-jsGR2Object::TOTAL_PROPS, val);
		break;
	}

	return JS_TRUE;
}

JSBool jsGR2Object::SetProperty(JSContext* ctx, int propid, jsval* val)
{
	/*	uncomment this switch if this object ever has any properties
		that can be set
	switch(propid)
	{
		// add props here
		default:
			return object->SetJSProperty(_jsctx, propid-jsGR2Object::TOTAL_PROPS, val);
		break;
	}

	return JS_TRUE;
	*/

	return object->SetJSProperty(_jsctx, propid-jsGR2Object::TOTAL_PROPS, val);
}

void jsGR2Object::AddGR2Property(JSPropertySpec& spec)
{
	_jsProperties[spec.tinyid] = spec;
}
