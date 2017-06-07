#include "syscfg.h" // Path is project specific.
#include "js/jsKeymap.h"
#include "imap/ww2func.h"
#include "imap/ww2imap.h"
#include "ui/uiMain.h"
#include "locl/loclLang.h"

/************************************************************************************************
	jsKeymap
*************************************************************************************************/

JSClass jsKeymap::jsclass = JS_CLASS_MACRO("keymap");
JSFunctionSpec jsKeymap::jsfunctions[] =
{
	{0}
};

JSPropertySpec jsKeymap::jsproperties[] =
{
	{0}
};

jsKeymap::jsKeymap(JSContext* ctx, JSObject* parent, int cat, int vehidx, int i, int ki)
	: phui::js::Object(&jsKeymap::jsclass, ctx, parent,jsfunctions,jsproperties)
	, category(cat), vehicleIdx(vehidx), functionidx(i), keyidx(ki)
{
	JS_CONSTRUCTOR_LOCK;

	functionCode = ww2funcGetCode(category, vehicleIdx, functionidx);
	isaxis = ww2funcIsAxis(functionCode);
	ezlevel = ww2funcGetEZLevel(functionCode);

	const std::wstring& fname = uiLocString(IDE_LANG_FUNCTION, ww2GetFuncLoc(functionCode));

	DefineProperty("headerid", INT_TO_JSVAL(ww2funcHeaderLocID(functionCode)));
	DefineProperty("header", phui::js::NewString(ww2funcHeaderLoc(functionCode).c_str()));
	DefineProperty("isaxis", INT_TO_JSVAL(isaxis));
	DefineProperty("keyidx", INT_TO_JSVAL(keyidx));
	DefineProperty("ezlevel", INT_TO_JSVAL(ezlevel));
	DefineProperty("functionname", phui::js::NewString(fname.c_str()));
}


jsKeymap::~jsKeymap()
{
}

