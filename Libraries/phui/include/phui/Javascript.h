#pragma once

#include <vector>
#include <wchar.h>

#include "jsapi.h"
#include "jsobj.h"
#include "jsutil.h"

#include "phui/Referenced.h"

namespace phui
{
namespace js
{
class Object;

#define JS_CONSTRUCTOR_LOCK phui::js::obj_root_ptr lock(this)

/************************************************************************************************
	Javascript
*************************************************************************************************/

int Init();
void Kill();

int SetGlobalObject(Object* obj);
Object* GlobalObject();
void SetErrorReporter(JSErrorReporter);

typedef void(*AbortHandler)(const char*);
void SetAbortHandler(AbortHandler);

/************************************************************************************************
	Object
*************************************************************************************************/
/** Wrapper for JS Engine */

class Object : public Referenced
{
public:
	Object(JSClass*, JSContext*, JSObject* parent, JSFunctionSpec*, JSPropertySpec*);
	Object(JSClass*, JSFunctionSpec* func=NULL, JSPropertySpec* prop=NULL);

	JSScript* CompileScript(const char* script, const char* filename = NULL);
	void DestroyScript(JSScript*);
	
	jsval RunScript(JSScript*);
	jsval RunScript(const char* script, const char* filename = NULL);
	
	JSBool CallFunction(const char* name, uintN argc, jsval* argv, jsval* rval);
	
	virtual JSBool GetProperty(JSContext*, int propid, jsval* val);
	virtual JSBool SetProperty(JSContext*, int propid, jsval* val);
	virtual void Finalize();
	
	void DefineFunctions(JSFunctionSpec*);
	void DefineProperties(JSPropertySpec*);
	
	void DefineProperty(const char* name, phui::js::Object*, uintN flags = JSPROP_PERMANENT | JSPROP_READONLY);
	void DefineProperty(const char* name, jsval val, uintN flags = JSPROP_PERMANENT | JSPROP_READONLY);

	JSObject* jsObject() const { return _jsobj; }
	JSContext* jsContext() const { return _jsctx; }
	JSObject* jsParent() const { return (_jsctx && _jsobj) ? JS_GetParent(_jsctx, _jsobj) : 0; }
	jsval jsVal() { return _jsval; }
	
	void Root(void*);
	void UnRoot(void*);
	
// to fill in JSClass structure (used by JSCLASSMACRO below)
static JSBool JSClassGetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp);
static JSBool JSClassSetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp);
static void JSClassFinalize(JSContext *cx, JSObject *obj);
static void DumpObjects();

protected:
	virtual ~Object();

	void CreateJS(JSClass*, JSContext* ctx, JSObject* parent, const char* lastfunc, const char* lastprop);
	
	JSContext* _jsctx;
	JSObject* _jsobj;
	jsval _jsval;
	JSClass* _jsclass;
};

// macro for creating JSClass struct for subclasses just need to specify class name
#define JS_CLASS_MACRO(x) \
{ x, JSCLASS_HAS_PRIVATE, \
JS_PropertyStub, JS_PropertyStub, \
phui::js::Object::JSClassGetProperty, phui::js::Object::JSClassSetProperty, \
JS_EnumerateStandardClasses, JS_ResolveStub, \
JS_ConvertStub, phui::js::Object::JSClassFinalize,JSCLASS_NO_OPTIONAL_MEMBERS\
}

#define JS_STUBCLASS_MACRO(x) \
{ x,  JSCLASS_HAS_PRIVATE, \
    JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub, \
    JS_EnumerateStub, JS_ResolveStub,   JS_ConvertStub,   JS_FinalizeStub, \
    JSCLASS_NO_OPTIONAL_MEMBERS\
}

// more convience macros
#define JS_CLASS_DATA \
static JSClass jsclass; \
static JSFunctionSpec jsfunctions[]; \
static JSPropertySpec jsproperties[];

#define JS_FUNCTION(func) static JSBool func(JSContext*, JSObject*, uintN, jsval*, jsval*)
#define JS_FUNCTION_DEF(class, func) JSBool class::func(JSContext* ctx, JSObject* jsobj, uintN argc, jsval* argv, jsval* rval)
#define JS_FUNCTION_SPEC(class, func, argc) {#func, class::func, argc}

template<class T>
T Cast(JSContext* ctx, JSObject* obj) 
{ 
	if(!obj || !(OBJ_GET_CLASS(ctx, obj)->flags & JSCLASS_HAS_PRIVATE)) return NULL;
	return dynamic_cast<T>((Object*)JS_GetPrivate(ctx,obj)); 
}

template<class T> 
T Cast(JSContext* ctx, jsval val) 
{ 
	JSObject* obj;
	if(JSVAL_IS_NULL(val) || !JS_ValueToObject(ctx, val, &obj) || 
		!obj || !(OBJ_GET_CLASS(ctx, obj)->flags & JSCLASS_HAS_PRIVATE) ) return NULL;
	
	return dynamic_cast<T>((Object*)JS_GetPrivate(ctx,obj)); 
}

template <class T>
class root_ptr
{
public:
	root_ptr(T* ref) : _ref(ref) 
	{ 
		if(_ref)
		{ 
			_jsobj = _ref->jsObject();
			if(_jsobj)
			{
//				JS_SetParent(_ref->jsContext(), _ref->jsObject(), NULL); 
				_ref->Root(&_jsobj);
			}
		}
	}
	root_ptr(const root_ptr& p) : _ref(0) { *this = p;}
	root_ptr() : _ref(0) {}
	~root_ptr() 
	{ 
		if(_ref) 
		{
			_ref->UnRoot(&_jsobj);
			if(_ref->jsObject() == 0)
			{
				_ref->Release();
			}
		}
	}
	root_ptr& operator=(const root_ptr& p) 
	{   
		if(p._ref != _ref)
		{
			if(_ref) 
			{
				_ref->UnRoot(&_jsobj);
				if(_ref->jsObject() == 0)
				{
					_ref->Release();
				}
			}
			
			_ref = p._ref; 
			if(_ref) 
			{
				_jsobj = _ref->jsObject();
				if(_jsobj)
				{
//					JS_SetParent(_ref->jsContext(), _ref->jsObject(), NULL); 
					_ref->Root(&_jsobj);			
				}
			}
		}
		return *this; 
	}
	bool operator==(const root_ptr& p) const { return (_ref == p._ref); }
	T* operator->() { return _ref; }
	T* ptr() const { return _ref; }
	bool valid() const { return _ref != 0; }
	
private:
	T* _ref;
	JSObject* _jsobj;
};

class jsvalRef : public phui::Referenced
{
public:
	jsvalRef(JSContext* ctx, jsval val) : Referenced(), _jsctx(ctx), _jsobj(0), _val(val)
	{ 
		if(JSVAL_IS_GCTHING(_val))
		{
			_jsobj = JSVAL_TO_GCTHING(_val);
			JS_AddRoot(_jsctx, &_jsobj);
		}
	}
	jsval jsVal() { return _val; }
	
protected:
	virtual ~jsvalRef()
	{
		if(JSVAL_IS_GCTHING(_val))
		{
			JS_RemoveRoot(_jsctx, &_jsobj);
		}
	}
	JSContext* _jsctx;
	void* _jsobj;
	jsval _val;
};

typedef root_ptr<Object> obj_root_ptr;

#include <vector>

class Array : public Object
{
public:
	Array();

	void SetElement(Object*, unsigned int idx);
	void SetElement(jsval, unsigned int idx);

	void clear();
	unsigned int size() const { jsuint len=0; if(_jsctx && _jsobj) JS_GetArrayLength(_jsctx, _jsobj, &len); return (unsigned int)len; }

	virtual ~Array();
	
protected:
	class Element
	{
	public:
		Element(JSContext* ctx, JSObject* obj, unsigned int i, jsval v);
		~Element();
		void Acquire();
		void Release();
		
		JSContext* _ctx;
		JSObject* _obj;
		unsigned int _i;
		jsval _val;
		int _refcount;
	};
	
	std::vector<ref_ptr<Element> > _elems;
};

typedef root_ptr<Array> array_root_ptr;


class jsval_root
{
public:
	jsval_root() : _jsctx(NULL), _jsval(JSVAL_NULL){}
	~jsval_root() 
	{ 
		if(_jsctx && _jsval != JSVAL_NULL) 
			JS_RemoveRoot(_jsctx, &_jsval);
	}

	jsval jsVal() const { return _jsval; }

	void set(JSContext* ctx, jsval val)
	{
		if(_jsctx && _jsval != JSVAL_NULL)
			JS_RemoveRoot(_jsctx, &_jsval);
		
		_jsctx = ctx;
		_jsval = val;

		if(_jsctx && _jsval != JSVAL_NULL)
			JS_AddRoot(_jsctx, &_jsval);
	}
	
	jsval_root& operator=(jsval_root const& v)
	{
		set(v._jsctx, v._jsval);
		return *this;
	}
	
private:
	JSContext* _jsctx;
	jsval _jsval;
};

jsval ConstString(const char *s);
jsval NewString(const wchar_t *s);
jsval NewString(const char *s);
const wchar_t* ValueToWideString(jsval);

}
}
