#include <syscfg.h> // Path is project specific.
#include <string.h>

#include "js/jsVehicle.h"
#include "js/jsClient.h"
#include "fio/fio.h"
#include "ui/uiMain.h"

/************************************************************************************************
	jsVehtype
*************************************************************************************************/

static const char* vehtypenames[VEH_MAXTYPE] =
{
	"none",
	"floater",
	"person",
	"air",
	"ground",
	"sea"
#ifdef DOROTHY
	, "mech"
#endif
};

JSClass jsVehtype::jsclass = JS_CLASS_MACRO("vehtype");
JSFunctionSpec jsVehtype::jsfunctions[] =
{
	{0}
};

JSPropertySpec jsVehtype::jsproperties[] =
{
	{"vcategory",	jsVehtype::CATEGORY_PROP,		JSPROP_PERMANENT},
	{"vclass",		jsVehtype::CLASS_PROP,			JSPROP_PERMANENT},
	{"description",	jsVehtype::DESCRIPTION_PROP,	JSPROP_PERMANENT},
	{0}
};

/************************************************************************************************
	jsVehtype
*************************************************************************************************/

jsVehtype::jsVehtype(vehType const* v)
			: phui::js::Object(&jsclass,jsfunctions,jsproperties)
			, vehtype(v)
{
	JS_CONSTRUCTOR_LOCK;
	Init();
};

jsVehtype::jsVehtype(vehType const* v, WeaponCounter const& c)
			: phui::js::Object(&jsclass, jsfunctions,jsproperties)
			, vehtype(v)
{
	JS_CONSTRUCTOR_LOCK;
	Init();
	DefineProperty("counter", new jsWeaponCounter(c));
}

void jsVehtype::Init()
{
	DefineProperty("country", new jsCountry((WWII_COUNTRY)vehtype->WeaponSpec.wCountry));
	DefineProperty("type", phui::js::ConstString((vehtype->Type < VEH_MAXTYPE) ? vehtypenames[vehtype->Type] : ""));
	DefineProperty("infotext",  phui::js::NewString( vehtype->InfoText.c_str()));
	DefineProperty("id", phui::js::NewString( (char*)vehtype->Id));
	DefineProperty("index",INT_TO_JSVAL(vehtype->VehicleTypeNum));
	DefineProperty("onlineonly",INT_TO_JSVAL(vehtype->OnlineOnly));
	DefineProperty("ismulticrew",BOOLEAN_TO_JSVAL(vehtype->VehicleIsMultiCrew));
	DefineProperty("vcategoryid",INT_TO_JSVAL(vehtype->WeaponSpec.wCategory));
	DefineProperty("vclassid",INT_TO_JSVAL(vehtype->WeaponSpec.wClass));

	UINT16 shortId = vehtype->WeaponSpec.asUINT16();
	char shortBuffer[20+1];
	snprintf(shortBuffer, 20, "%u", (UINT32)shortId);
	DefineProperty("weapkey", phui::js::NewString(shortBuffer));
}

jsVehtype::~jsVehtype()
{
}

JSBool jsVehtype::GetProperty(JSContext* ctx, int propid, jsval* val)
{
	// these are done on the fly in case the UI language changes
	switch(propid)
	{
		case CATEGORY_PROP:
		{
			*val = phui::js::NewString(vehTypeLoc(1, vehtype));
		}
		break;
		case CLASS_PROP:
		{
			*val = phui::js::NewString(vehTypeLoc(2, vehtype));
		}
		break;
		case DESCRIPTION_PROP:
		{
			*val = phui::js::NewString(vehTypeLoc(3, vehtype));
		}
		break;
		default:
			// do nothing
		break;
	}

	return JS_TRUE;
}
