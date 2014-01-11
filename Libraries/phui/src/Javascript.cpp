#include "phui-precompiled-header.h"

#include <map>
#include <set>
#include <typeinfo>
#include <errno.h>

#include "phui/Javascript.h"
#include "phui/Util.h"

using namespace phui;
using namespace phui::js;

/************************************************************************************************
	Javascript
*************************************************************************************************/

typedef std::map<std::string, jsval_root> StringMap;
static StringMap* _jsconststr;

static JSRuntime* _jsrt;
static JSContext* _jsglobctx;
static Object* _jsglob;
static void _defaulterrorreporter(JSContext *cx, const char *message, JSErrorReport *report);
static JSErrorReporter _errorreporter = _defaulterrorreporter;
static AbortHandler _abort;
extern "C" JSClass js_ArrayClass;

//#define JAVASCRIPT_DEBUG
#undef JAVASCRIPT_DEBUG

#ifdef JAVASCRIPT_DEBUG
static std::set<Object*> _objects;
#endif

/************************************************************************************************
	Init
*************************************************************************************************/

int phui::js::Init()
{
 	// dodgy as hell but what the heck
	js_ArrayClass.flags |= JSCLASS_HAS_PRIVATE;
	js_ArrayClass.finalize = phui::js::Object::JSClassFinalize;
	
   _jsrt = JS_NewRuntime(3L * 1024L * 1024L);
	if(!_jsrt) return -1;
	
	_jsglobctx = JS_NewContext(_jsrt, 8192);
	
	if(_errorreporter)
		JS_SetErrorReporter(_jsglobctx, _errorreporter);

	_jsconststr = new StringMap;
	
	return 0;
}

/************************************************************************************************
	Kill
*************************************************************************************************/

void phui::js::Kill()
{
	if(_jsglobctx)
		JS_DestroyContext(_jsglobctx);
	
	if(_jsrt)
		JS_DestroyRuntime(_jsrt);

	JS_ShutDown();

	_jsrt = nullptr;
	_jsglob = nullptr;
	_jsglobctx = nullptr;
}

/************************************************************************************************
	SetGlobalObject
*************************************************************************************************/

int phui::js::SetGlobalObject(Object* obj)
{
	if(_jsglob) return -1;
	
	if (!JS_InitStandardClasses(obj->jsContext(), obj->jsObject())) return -1;

	obj->Acquire();
	
	_jsglob = obj;

	return 0;
}

/************************************************************************************************
	GlobalObject
*************************************************************************************************/

Object*  phui::js::GlobalObject()
{
	return _jsglob;
}

/************************************************************************************************
	GlobalObject
*************************************************************************************************/

phui::js::Object::Object(JSClass* jsclass, JSContext* ctx, JSObject* parent, JSFunctionSpec* func, JSPropertySpec* prop)
				: _jsctx(_jsglobctx),_jsobj(0), _jsval(JSVAL_NULL)	

{
	CreateJS(jsclass, ctx, parent,func ? func->name : "?",prop ? prop->name : "?");
	
	if(func)
		DefineFunctions(func);
	if(prop)
		DefineProperties(prop);
#ifdef JAVASCRIPT_DEBUG
	_objects.insert(this);
#endif
}

phui::js::Object::Object(JSClass* jsclass, JSFunctionSpec* func, JSPropertySpec* prop)
			: _jsctx(_jsglobctx),_jsobj(0), _jsval(JSVAL_NULL)
{
	CreateJS(jsclass, _jsctx, nullptr,func ? func->name : "?",prop ? prop->name : "?");
	
	if(func)
		DefineFunctions(func);
	if(prop)
		DefineProperties(prop);
#ifdef JAVASCRIPT_DEBUG
	_objects.insert(this);
#endif
}


phui::js::Object::~Object()
{
	if(_jsctx && _jsobj)
		JS_SetPrivate(_jsctx, _jsobj, 0);

#ifdef JAVASCRIPT_DEBUG
	_objects.erase(this);
#endif
}

/************************************************************************************************
	CreateJS
*************************************************************************************************/

static char lasterror[512];

void phui::js::Object::CreateJS(JSClass* jsclass, JSContext* ctx, JSObject* parent, const char* lastfunc, const char* lastprop)
{
	if(parent == nullptr && _jsglob)
		parent = _jsglob->jsObject();
	
	if(jsclass)
	{
		_jsobj = JS_NewObject(_jsctx, jsclass, nullptr, parent);
#ifdef SHOW_JS_ALLOCATIONS
//        printf( "JS_NewObject created: %s.%s.%s\n", jsclass ? jsclass->name : "?", lastfunc ? lastfunc : "nil", lastprop ? lastprop : "nil" );
#endif
		
		if(_jsobj)
		{
			_jsval = OBJECT_TO_JSVAL(_jsobj);
		}else
		{
			if(_abort)
            {
                sprintf( lasterror, "JS_NewObject failure: %s.%s.%s\n", jsclass ? jsclass->name : "?", lastfunc, lastprop );
				_abort(lasterror);
            }
		}
	}
	
	if(_jsctx && _jsobj)
		JS_SetPrivate(_jsctx, _jsobj, this);
		
	_jsclass = jsclass;
}

/************************************************************************************************
	CompileScript
*************************************************************************************************/

JSScript* phui::js::Object::CompileScript(const char* script, const char* filename)
{
	if(_jsctx && _jsobj)
		return JS_CompileScript(_jsctx, _jsobj,script, strlen(script), filename, 0);
	
	return 0;
}

/************************************************************************************************
	DestroyScript
*************************************************************************************************/

void phui::js::Object::DestroyScript(JSScript* jscript)
{
	if(_jsctx)
		JS_DestroyScript(_jsctx, jscript);
}
	
/************************************************************************************************
	RunScript
*************************************************************************************************/

jsval phui::js::Object::RunScript(JSScript* jscript)
{
	jsval result = JSVAL_NULL;
	
	if(_jsctx && _jsobj)
	{
		JS_ClearPendingException(_jsctx);
		JS_ExecuteScript(_jsctx, _jsobj, jscript, &result);
//		JS_MaybeGC(_jsctx);
	}
	
	return result;
}

/************************************************************************************************
	RunScript
*************************************************************************************************/

jsval phui::js::Object::RunScript(const char* script, const char* filename)
{
	jsval result = JSVAL_NULL;

	if(_jsctx && _jsobj)
	{
		JS_ClearPendingException(_jsctx);
		JS_EvaluateScript(_jsctx, _jsobj, script, strlen(script), filename, 0,  &result);
//		JS_MaybeGC(_jsctx);
	}
		
	return result;
}

JSBool phui::js::Object::GetProperty(JSContext*, int propid, jsval* val)
{
	return JS_TRUE;
}

JSBool phui::js::Object::SetProperty(JSContext*, int propid, jsval* val)
{
	return JS_TRUE;
}

void phui::js::Object::Finalize()
{
#ifdef JAVASCRIPT_DEBUG
//	printf("Finalize: %s 0x%X\n", _jsclass && _jsclass->name ? _jsclass->name : "???", (int)_jsobj);
#endif
	_jsobj = 0;
	_jsval = 0;
}

void phui::js::Object::DefineFunctions(JSFunctionSpec* func)
{
	if(_jsctx && _jsobj)
		JS_DefineFunctions(_jsctx, _jsobj, func);
}

void phui::js::Object::DefineProperties(JSPropertySpec* prop)
{
	if(_jsctx && _jsobj)
		JS_DefineProperties(_jsctx, _jsobj, prop);
}

void phui::js::Object::DefineProperty(const char* name, phui::js::Object* obj, uintN flags)
{
	DefineProperty(name, obj->jsVal(), flags);
}

void phui::js::Object::DefineProperty(const char* name, jsval val, uintN flags)
{
	if(_jsctx && _jsobj)
	{
		jsval_root jsvalroot;
		jsvalroot.set(_jsctx, val);
		JS_DefineProperty(_jsctx, _jsobj, name, val, nullptr, nullptr, flags);
	}
}

void phui::js::Object::Root(void* ptr)
{
	if(_jsctx)
		JS_AddRoot(_jsctx, ptr);
}

void phui::js::Object::UnRoot(void* ptr)
{
	if(_jsctx)
		JS_RemoveRoot(_jsctx, ptr);
}

/************************************************************************************************
	JSClassGetProperty
*************************************************************************************************/

JSBool phui::js::Object::JSClassGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	Object* dom = (Object*)JS_GetPrivate(cx, obj);
	if(!dom) return JS_FALSE;
	
	int prop = JSVAL_TO_INT(id);
	
	return dom->GetProperty(cx,prop, vp);
}

/************************************************************************************************
	JSClassSetProperty
*************************************************************************************************/

JSBool phui::js::Object::JSClassSetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	Object* dom = (Object*)JS_GetPrivate(cx, obj);
	if(!dom) return JS_FALSE;
	
	int prop = JSVAL_TO_INT(id);
	
	return dom->SetProperty(cx,prop, vp);
}

/************************************************************************************************
	JSClassFinalize
*************************************************************************************************/

void phui::js::Object::JSClassFinalize(JSContext *cx, JSObject *obj)
{
	Object* dom = (Object*)JS_GetPrivate(cx, obj);
	if(dom)
	{
		if(cx == dom->jsContext())
		{
			dom->Finalize();
			dom->Release();
		}
	}
#ifdef JAVASCRIPT_DEBUG
//	else printf("Finalize on deleted object 0x%x\n", (int)obj);
#endif
}

/************************************************************************************************
	Array
*************************************************************************************************/

phui::js::Array::Array() : js::Object(nullptr)
{
	_jsobj = JS_NewArrayObject(_jsctx, 0, nullptr);
	_jsval = OBJECT_TO_JSVAL(_jsobj);	
	_jsclass = &js_ArrayClass;
	
	// we hacked up js_ArrayClass to make this work
	if(_jsctx && _jsobj)
		JS_SetPrivate(_jsctx, _jsobj, this);
}

phui::js::Array::~Array()
{
}
	
void phui::js::Array::SetElement(Object* obj, unsigned int idx)
{
	SetElement(obj->jsVal(), idx);
}

void phui::js::Array::SetElement(jsval val, unsigned int idx)
{
	if(_jsctx && _jsobj)
	{
		if(idx >= _elems.size())
			_elems.resize(idx+1);
		
		_elems[idx] = new Element(_jsctx, _jsobj, idx, val);
	}
}

void phui::js::Array::clear()
{
	if(_jsctx && _jsobj)
	{
		_elems.clear();
		JS_SetArrayLength(_jsctx, _jsobj, 0);
	}
}

phui::js::Array::Element::Element(JSContext* ctx, JSObject* obj, unsigned int i, jsval v) : _ctx(ctx), _obj(obj), _i(i), _val(v), _refcount(0)
{
	JS_SetElement(_ctx, _obj, _i, &_val);
}

phui::js::Array::Element::~Element()
{
	/*if(_ctx && _obj)
	{
		JS_DeleteElement(_ctx, _obj, _i);
	}*/
}
		
void phui::js::Array::Element::Acquire() 
{ 
	_refcount++; 
}

void phui::js::Array::Element::Release() 
{ 
	_refcount--; 
	if (_refcount<1) delete this; 
}

static void _defaulterrorreporter(JSContext *cx, const char *message, JSErrorReport *report)
{
	printf("%s\n", message);
}

void phui::js::SetErrorReporter(JSErrorReporter er)
{
	_errorreporter = er;
	JS_SetErrorReporter(_jsglobctx, _errorreporter);
}

/************************************************************************************************
	NewWideConstString
*************************************************************************************************/

jsval phui::js::ConstString(const char *s)
{
	jsval jval = JSVAL_NULL;
	
	StringMap::iterator it = _jsconststr->find(s);
	if(it != _jsconststr->end())
	{
		jval = it->second.jsVal();
	}else
	{
		jsval_root valroot;
		jval =  NewString(s);
		valroot.set(_jsglobctx,jval);
		(*_jsconststr)[s] = valroot;
	}
	
	return jval;
}

/************************************************************************************************
	NewString
*************************************************************************************************/

jsval phui::js::NewString(const wchar_t *s)
{
	static jschar buffer[PHUI_MAX_BUFFER];
	int len = wcslen(s);
	if(len > PHUI_MAX_BUFFER-1) len = PHUI_MAX_BUFFER-1;
	buffer[len] = '\0';
	
	jschar* js = buffer;
	for(int i = 0; i < len; i++, s++, js++)
		*js = *s;

	return STRING_TO_JSVAL(JS_NewUCStringCopyZ(_jsglobctx,buffer));
}

/************************************************************************************************
	NewString
*************************************************************************************************/

jsval phui::js::NewString(const char *s)
{
	static jschar buffer[PHUI_MAX_BUFFER];
	int len = strlen((char*)s);
	if(len > PHUI_MAX_BUFFER-1) len = PHUI_MAX_BUFFER-1;
	buffer[len] = '\0';
	
	jschar* js = buffer;
	for(int i = 0; i < len; i++, s++, js++)
		*js = (unsigned char)*s; // important, needs to be unsigned

	return STRING_TO_JSVAL(JS_NewUCStringCopyZ(_jsglobctx,buffer));
}

/************************************************************************************************
	ValueToWideString
*************************************************************************************************/

const wchar_t* phui::js::ValueToWideString(jsval val)
{
	JSString* jstr = JS_ValueToString(_jsglobctx,val);
	if(!jstr) return L"";
		
	static wchar_t buffer[PHUI_MAX_BUFFER];
	int len = JS_GetStringLength(jstr);
	
	jschar* js = JS_GetStringChars(jstr);
	if(js)
	{
		if(len > PHUI_MAX_BUFFER-1) len = PHUI_MAX_BUFFER-1;
		buffer[len] = 0;
		
		wchar_t* s = buffer;
		for(int i = 0; i < len; i++, s++, js++)
			*s = *js;
	}else
	{
		*buffer = 0;
	}
	return buffer;
}

/************************************************************************************************
	CallFunction
*************************************************************************************************/

JSBool phui::js::Object::CallFunction(const char* name, uintN argc, jsval* argv, jsval* rval)
{
	if(_jsctx && _jsobj)
		return JS_CallFunctionName(_jsctx, _jsobj, name, argc, argv, rval);

	return JS_FALSE;
}

void phui::js::SetAbortHandler(AbortHandler a)
{
	_abort = a;
}

void phui::js::Object::DumpObjects()
{
#ifdef JAVASCRIPT_DEBUG
/*	std::set<Object*>::iterator it = _objects.begin();
	for(; it != _objects.end(); ++it)
	{
		Object* obj = *it;
		printf("%s 0x%x\n", obj->_jsclass ? obj->_jsclass->name : "???", (int)obj->jsObject());
	}*/
	printf("count: %d\n", (int)_objects.size());
#endif
}
