#include "phui-precompiled-header.h"

#include <string.h>
#include <map>

#include "jsapi.h"

#include "phui/DOM.h"
#include "phui/View.h"
#include "phui/Viewport.h"
#include "phui/Message.h"
#include "phui/Util.h"
#include "phui/MessageLoop.h"
#include "phui/MessageFilter.h"
#include "phui/UIMessages.h"
#include "phui/Control.h"
#include "phui/Invoker.h"
#include "phui/Periodical.h"
#include "phui/ImageView.h"
#include "phui/ListView.h"
#include "phui/TextContainer.h"
#include "phui/Caption.h"
#include "phui/GraphicsContext.h"
#include "phui/UIXML.h"
#include "phui/CSSViewDrawer.h"
#include "phui/TabView.h"
#include "phui/CtxMenu.h"
#include "phui/ConsoleView.h"
#include "phui/MirrorView.h"
#include "phui/IMEControl.h"
//#include "phui/ShadowView.h"

#ifdef _WIN32
#define snprintf _snprintf
#else
#define stricmp strcasecmp
#endif

using namespace phui;
using namespace phui::DOM;

JSClass ElementDOM::jsclass = JS_CLASS_MACRO("element");

JSFunctionSpec ElementDOM::jsfunctions[] =
{
	JS_FUNCTION_SPEC(ElementDOM, close, 0),
	JS_FUNCTION_SPEC(ElementDOM, findview, 1),
	JS_FUNCTION_SPEC(ElementDOM, addrow, 3),	
	JS_FUNCTION_SPEC(ElementDOM, removerow, 1),
	JS_FUNCTION_SPEC(ElementDOM, clearrows, 0),	
	JS_FUNCTION_SPEC(ElementDOM, getstyleproperty, 1),	
	JS_FUNCTION_SPEC(ElementDOM, setstyleproperty, 2),	
	JS_FUNCTION_SPEC(ElementDOM, findparent, 1),
	JS_FUNCTION_SPEC(ElementDOM, scrollby, 1),
	JS_FUNCTION_SPEC(ElementDOM, scrolltorow, 1),
	JS_FUNCTION_SPEC(ElementDOM, addview, 1),
	JS_FUNCTION_SPEC(ElementDOM, settip, 1),
	JS_FUNCTION_SPEC(ElementDOM, delview, 1),
	JS_FUNCTION_SPEC(ElementDOM, scrollto, 1),
	JS_FUNCTION_SPEC(ElementDOM, showtab, 2),
	JS_FUNCTION_SPEC(ElementDOM, resizetotext, 0),
	JS_FUNCTION_SPEC(ElementDOM, isselectable, 1),
	JS_FUNCTION_SPEC(ElementDOM, addmenu, 2),
	JS_FUNCTION_SPEC(ElementDOM, addevent, 2),
	JS_FUNCTION_SPEC(ElementDOM, addcommand, 1),
	JS_FUNCTION_SPEC(ElementDOM, getchild, 1),
	JS_FUNCTION_SPEC(ElementDOM, resizetoaspect, 1),
	JS_FUNCTION_SPEC(ElementDOM, snapshot, 4),
	JS_FUNCTION_SPEC(ElementDOM, getimeproperty, 2),
	JS_FUNCTION_SPEC(ElementDOM, scalev, 2),
	JS_FUNCTION_SPEC(ElementDOM, normv, 2),
	{0}
};

JSPropertySpec ElementDOM::jsproperties[] =
{
	{"document",ElementDOM::DOCUMENT_PROP,  JSPROP_READONLY | JSPROP_PERMANENT},	
	{"window",  ElementDOM::WINDOW_PROP,	JSPROP_READONLY | JSPROP_PERMANENT},	
	{"length", 	ElementDOM::LENGTH_PROP,	JSPROP_PERMANENT | JSPROP_READONLY},
	{"checked",	ElementDOM::CHECKED_PROP,	JSPROP_PERMANENT},
	{"enabled",	ElementDOM::ENABLED_PROP,	JSPROP_PERMANENT},	
	{"value",	ElementDOM::VALUE_PROP,		JSPROP_PERMANENT},		
	{"label",	ElementDOM::LABEL_PROP,		JSPROP_PERMANENT},	
	{"left",	ElementDOM::LEFT_PROP,		JSPROP_PERMANENT},	
	{"right",	ElementDOM::RIGHT_PROP,		JSPROP_PERMANENT},	
	{"bottom",	ElementDOM::BOTTOM_PROP,	JSPROP_PERMANENT},	
	{"top",		ElementDOM::TOP_PROP,		JSPROP_PERMANENT},	
	{"width",	ElementDOM::WIDTH_PROP,		JSPROP_PERMANENT},	
	{"height",	ElementDOM::HEIGHT_PROP,	JSPROP_PERMANENT},	
	{"x",		ElementDOM::X_PROP,			JSPROP_PERMANENT},	
	{"y",		ElementDOM::Y_PROP,			JSPROP_PERMANENT},	
	{"src",		ElementDOM::SRC_PROP,		JSPROP_PERMANENT},	
	{"name", 	ElementDOM::NAME_PROP,		JSPROP_PERMANENT},	
	{"visible", ElementDOM::VISIBLE_PROP,	JSPROP_PERMANENT},	
	{"id", 		ElementDOM::ID_PROP,		JSPROP_PERMANENT},	
	{"text", 	ElementDOM::TEXT_PROP,		JSPROP_PERMANENT},	
	{"focused",	ElementDOM::FOCUSED_PROP,	JSPROP_PERMANENT},	
	{"parent", 	ElementDOM::PARENT_PROP,	JSPROP_PERMANENT},	
	{"selecteditem", 	ElementDOM::SELECTEDITEM_PROP,	JSPROP_PERMANENT},
	{"selecteddata", 	ElementDOM::SELECTEDDATA_PROP,	JSPROP_PERMANENT | JSPROP_READONLY},
	{"selectedrow", 	ElementDOM::SELECTEDROW_PROP,	JSPROP_PERMANENT | JSPROP_READONLY},
	{"screenx",			ElementDOM::SCREENX_PROP,		JSPROP_PERMANENT},
	{"screeny",			ElementDOM::SCREENY_PROP,		JSPROP_PERMANENT},
	{"selectedindex", 	ElementDOM::SELECTEDINDEX_PROP,	JSPROP_PERMANENT | JSPROP_READONLY},
	{"scrollx",			ElementDOM::SCROLLX_PROP,		JSPROP_PERMANENT},
	{"scrolly",			ElementDOM::SCROLLY_PROP,		JSPROP_PERMANENT},
	{"acceptsfocus",	ElementDOM::ACCEPTSFOCUS_PROP,	JSPROP_PERMANENT},
	{"maxwidth",		ElementDOM::MAXWIDTH_PROP,		JSPROP_PERMANENT},
	{"maxheight",		ElementDOM::MAXHEIGHT_PROP,		JSPROP_PERMANENT},
	{"shadowx",			ElementDOM::SHADOWX_PROP,		JSPROP_PERMANENT},
	{"shadowy",			ElementDOM::SHADOWY_PROP,		JSPROP_PERMANENT},
	{"acceptsmouse",	ElementDOM::ACCEPTSMOUSE_PROP,	JSPROP_PERMANENT},
	{"modal",			ElementDOM::MODAL_PROP,			JSPROP_PERMANENT},
	{"tabx",				ElementDOM::TABX_PROP,			JSPROP_PERMANENT | JSPROP_READONLY},	
	{"taby",				ElementDOM::TABY_PROP,			JSPROP_PERMANENT | JSPROP_READONLY},	
	{"tabw",				ElementDOM::TABW_PROP,			JSPROP_PERMANENT | JSPROP_READONLY},	
	{"tabh",				ElementDOM::TABH_PROP,				JSPROP_PERMANENT | JSPROP_READONLY},	
	{"stringlength",	ElementDOM::STRINGLENGTH_PROP,			JSPROP_PERMANENT | JSPROP_READONLY},	
	{"windowx",			ElementDOM::WINDOWX_PROP,		JSPROP_PERMANENT | JSPROP_READONLY},
	{"windowy",		ElementDOM::WINDOWY_PROP,		JSPROP_PERMANENT | JSPROP_READONLY},
	{"parentwin",		ElementDOM::PARENTWIN_PROP,		JSPROP_PERMANENT | JSPROP_READONLY},
	{"nx",					ElementDOM::NX_PROP,			JSPROP_PERMANENT},	
	{"ny",					ElementDOM::NY_PROP,			JSPROP_PERMANENT},	
	{"nwidth",			ElementDOM::NWIDTH_PROP,		JSPROP_PERMANENT},	
	{"nheight",			ElementDOM::NHEIGHT_PROP,	JSPROP_PERMANENT},	
	{0}
};

class OnUpdatePeriodical : public phui::Periodical
{
public:
	OnUpdatePeriodical(View*, js::Object*, const char* script, const char* filename);
	virtual ~OnUpdatePeriodical();

	void Execute(unsigned int t);
	void SetScript(const char* s){ _script = s; }

	js::Object* _jsobj;
	JSScript* _jscript;
	View* _view;
	std::string _script;
	std::string _source;
};

class ScriptFilter : public MessageFilter
{
public:
	ScriptFilter();
	virtual bool Filter(Message* msg, MessageHandler** target, MessageHandler**);

	View* _lastclickview;
	unsigned int _lastclicktime;
};

typedef std::map<std::string, ElementFromView> ElementCreateMap;
static ElementCreateMap _elemcreatemap;

static GraphicsContext::TextureInfo (*_gettexture)(const char*,bool);

static void dom_init(View* view, ElementDOM* parent, const char* filename = NULL);
static void dom_script_init(View* view, const char* filename = NULL);

/************************************************************************************************
	RegisterTagWithDOM
*************************************************************************************************/

void phui::DOM::RegisterTagWithDOM(const char* tag, ElementFromView elemcreate)
{
	_elemcreatemap[tag] = elemcreate;
}

/************************************************************************************************
	CreateElementFromView
*************************************************************************************************/

ElementDOM* phui::DOM::CreateElementFromView(const char* classname, View* view, ElementDOM* parent)
{
	ElementFromView elemcreate = _elemcreatemap[classname];
	if(elemcreate)
		return elemcreate(classname, view, parent);
	return 0;
}

/************************************************************************************************
	DOM
*************************************************************************************************/

void phui::DOM::Init(View* view)
{
	dom_init(view, NULL, view->GetSource());	
	dom_script_init(view);
}

void phui::DOM::AddScriptMessageFilter(MessageLoop* loop)
{
	loop->AddCommonFilter(new ScriptFilter);
}

void phui::DOM::RunScript(MessageLoop* loop, js::Object* obj, const char* script)
{
	Message msg(DOM_SCRIPT_MSG);
	msg.AddString(DOM_SCRIPT, script);
	msg.AddPointer(DOM_OBJECT, obj);
	
	loop->PostMessage(&msg, loop);
}

void phui::DOM::SetTextureCallback(GraphicsContext::TextureInfo (*gettexture)(const char*,bool))
{
	_gettexture = gettexture;
}

/************************************************************************************************
	ElementDOM
*************************************************************************************************/

ElementDOM::ElementDOM(View* view, ElementDOM* window, JSClass* jsclass) : 
	js::Object(jsclass ? jsclass : &ElementDOM::jsclass, window ? window->jsContext() : phui::js::GlobalObject()->jsContext(), window ? window->jsObject() : 0,
				jsfunctions, jsproperties), 
	_view(view), _window(window)
{
	if(!_window)
		_window = this;
		
	_view->SetUserData(DOM_VIEW_USERDATA, this);

	Message* msg = view->UserMessage();
	const char* script = NULL;
	
	Message scriptMsg(DOM_SCRIPT_MSG);
	Invoker* ctl = dynamic_cast<Invoker*>(view);

	if(ctl && msg->FindString(DOM_ONCLICK, &script) == Message::OK)
	{
		scriptMsg.AddString(DOM_SCRIPT, script);
		scriptMsg.AddPointer(DOM_OBJECT, this);
	
		ctl->SetMessage(&scriptMsg);
		ctl->SetTarget(view->ParentLoop());
	}
	
	if(ctl && msg->FindString(DOM_ONENTER, &script) == Message::OK)
	{
		scriptMsg.AddString(DOM_SCRIPT, script);
		scriptMsg.AddPointer(DOM_OBJECT, this);
	
		ctl->SetMessage(&scriptMsg);
		ctl->SetTarget(view->ParentLoop());
	}
	
	_onupdate = 0;
	_src = 0;
	
	Root(&_jsobj);
}

ElementDOM::~ElementDOM()
{
	delete _onupdate;
	delete _src;
	
	UnRoot(&_jsobj);
}

/************************************************************************************************
	Property
*************************************************************************************************/

JSBool ElementDOM::GetProperty(JSContext* ctx, int propid, jsval* val)
{
	switch(propid)
	{
		case WINDOW_PROP:
		case DOCUMENT_PROP:
			if(_window)
				*val = OBJECT_TO_JSVAL(_window->jsObject());
			else
				*val = OBJECT_TO_JSVAL(jsObject());
		break;
		
		case PARENTWIN_PROP:
		{
			*val = JSVAL_NULL;

			View* pw = _view->ParentWin();

			if(pw)
			{
				ElementDOM* viewdom = (ElementDOM*)pw->UserData(DOM_VIEW_USERDATA);
				if(viewdom) *val = viewdom->jsVal();
			}
		}
		break;
		case CHECKED_PROP:
			*val = INT_TO_JSVAL(_view->GetValue().Int());
		break;
	
		case ENABLED_PROP:
			*val = INT_TO_JSVAL(_view->Enabled());
		break;
		
		case VALUE_PROP:
			JS_NewDoubleValue(ctx, _view->GetValue().Double(), val);
		break;
		case LEFT_PROP:
			JS_NewDoubleValue(ctx, _view->X(), val);
		break;
		case RIGHT_PROP:
			JS_NewDoubleValue(ctx, _view->X() +_view->Width(), val);
		break;
		case BOTTOM_PROP:
			JS_NewDoubleValue(ctx, _view->Y(), val);
		break;
		case TOP_PROP:
			JS_NewDoubleValue(ctx, _view->Y()+_view->Height(), val);
		break;
		case WIDTH_PROP:
			JS_NewDoubleValue(ctx, _view->Width(), val);
		break;
		case HEIGHT_PROP:
			JS_NewDoubleValue(ctx, _view->Height(), val);
		break;
		case X_PROP:
			JS_NewDoubleValue(ctx, _view->X(), val);
		break;
		case Y_PROP:
			JS_NewDoubleValue(ctx, _view->Y(), val);
		break;
		case SRC_PROP:
			if(_src)
				*val = STRING_TO_JSVAL(JS_NewStringCopyZ(ctx, _src->c_str()));
		break;
		case SELECTEDITEM_PROP:
			*val = INT_TO_JSVAL(_view->GetValue().Int());
		break;
		case NAME_PROP:
		case ID_PROP:
			*val = STRING_TO_JSVAL(JS_NewStringCopyZ(ctx, _view->Name()));
		break;
		case SELECTEDDATA_PROP:
		{
			*val = JSVAL_NULL;
			
			ListView* lv = dynamic_cast<ListView*>(_view);
			if(lv)
			{
				int row = lv->GetValue();
				Referenced* data = lv->GetRowData(row);
				if(!data) return JS_TRUE;

				js::jsvalRef* obj = dynamic_cast<js::jsvalRef*>(data);
				if(!obj) return JS_TRUE;
				
				*val = obj->jsVal();
				return JS_TRUE;
			}
			
			CtxMenu* mv = dynamic_cast<CtxMenu*>(_view);
			if(mv)
			{
				Referenced* data = mv->UserData(DOM_VIEW_MENUDATA);
				if(!data) return JS_TRUE;

				js::jsvalRef* obj = dynamic_cast<js::jsvalRef*>(data);
				if(!obj) return JS_TRUE;
				
				*val = obj->jsVal();
				return JS_TRUE;
			}
		}
		break;

		case TEXT_PROP:
		{
			TextContainer* tc = dynamic_cast<TextContainer*>(_view);
			if(tc)
				*val = phui::js::NewString(tc->WideText());
			else
				*val = phui::js::NewString(_view->GetValue().WideString());
		}
		break;
		
		case LABEL_PROP:
		{
			Control* ctl = dynamic_cast<Control*>(_view);
			if(ctl)
			{
				*val = phui::js::NewString(ctl->WideLabel());
			}else
			{
				Scalar s = _view->GetValue();
				*val = phui::js::NewString(s.WideString());
			}
		}
		break;
		case VISIBLE_PROP:
			*val = BOOLEAN_TO_JSVAL(_view->IsVisible());
		break;	
		case FOCUSED_PROP:
			*val = BOOLEAN_TO_JSVAL(_view->IsFocus());
		break;
		case SELECTEDROW_PROP:
		{
			ListView* lv = dynamic_cast<ListView*>(_view);
			if(lv)
			{
				View* view = lv->SelectedRow();
				if(view)
				{
					ElementDOM* viewdom = (ElementDOM*)view->UserData(DOM_VIEW_USERDATA);
					if(viewdom)
						*val = viewdom->jsVal();
				}	
			}
		}
		break;
		case SELECTEDINDEX_PROP:
		{
			ListView* lv = dynamic_cast<ListView*>(_view);
			if(lv)
				*val = INT_TO_JSVAL(lv->SelectedIndex());
		}
		break;
		case PARENT_PROP:
		{
			*val = JSVAL_NULL;

			View* parent = _view->Parent();
			if(parent)
			{
				ElementDOM* parentdom = dynamic_cast<ElementDOM*>(parent->UserData(DOM_VIEW_USERDATA));
				if(parentdom)
					*val = parentdom->jsVal();			
			}
		}
		break;
		case LENGTH_PROP:
		{
			ListView* lv = dynamic_cast<ListView*>(_view);
			if(lv)
				*val = INT_TO_JSVAL(lv->NumRows());
		}
		break;
		case SCREENX_PROP:
		{
			float x, y;
			x = y = 0;
			_view->LocalToScreen(&x, &y);
			JS_NewDoubleValue(ctx, x, val);
		}
		break;
		case SCREENY_PROP:
		{
			float x, y;
			x = y = 0;
			_view->LocalToScreen(&x, &y);
			JS_NewDoubleValue(ctx, y, val);
		}
		break;
		case WINDOWX_PROP:
		{
			float x, y, wx, wy;
			x = y = wx = wy = 0;

			_view->LocalToScreen(&x, &y);

			View* pw = _view->ParentWin();
			if(pw) pw->LocalToScreen(&wx, &wy);

			x -= wx;
			y -= wy;

			JS_NewDoubleValue(ctx, x, val);
		}
		break;
		case WINDOWY_PROP:
		{
			float x, y, wx, wy;
			x = y = wx = wy = 0;

			_view->LocalToScreen(&x, &y);

			View* pw = _view->ParentWin();
			if(pw) pw->LocalToScreen(&wx, &wy);

			x -= wx;
			y -= wy;

			JS_NewDoubleValue(ctx, y, val);
		}
		break;
		case SCROLLX_PROP:
		{
			float sx = 0;

			ListView* lv = dynamic_cast<ListView*>(_view);
			if(lv)
				sx = lv->ScrollX();
			
			// might want to add other view types that make use of scrollx

			JS_NewDoubleValue(ctx, sx, val);
		}
		break;
		case SCROLLY_PROP:
		{
			float sy = 0;

			ListView* lv = dynamic_cast<ListView*>(_view);
			if(lv)
				sy = lv->ScrollY();
			
			// might want to add other view types that make use of scrolly

			JS_NewDoubleValue(ctx, sy, val);
		}
		break;
		case ACCEPTSFOCUS_PROP:
			*val = BOOLEAN_TO_JSVAL(_view->AcceptsFocus());
		break;
		case MAXWIDTH_PROP:
		{
			float w, h;
			_view->MaxSize(&w, &h);
			JS_NewDoubleValue(ctx, w, val);
		}
		break;
		case MAXHEIGHT_PROP:
		{
			float w, h;
			_view->MaxSize(&w, &h);
			JS_NewDoubleValue(ctx, h, val);
		}
		case SHADOWX_PROP:
		{
			float x = 0;

			if(_view->DrawerEnabled())
			{
				CSSViewDrawer* drawer = dynamic_cast<CSSViewDrawer*>(_view->GetDrawer());
				if(drawer) x = drawer->GetShadowX();
			}
			
			JS_NewDoubleValue(ctx, x, val);
		}
		break;
		case SHADOWY_PROP:
		{
			float y = 0;

			if(_view->DrawerEnabled())
			{
				CSSViewDrawer* drawer = dynamic_cast<CSSViewDrawer*>(_view->GetDrawer());
				if(drawer) y = drawer->GetShadowY();
			}
			
			JS_NewDoubleValue(ctx, y, val);
		}
		break;
		case ACCEPTSMOUSE_PROP:
			*val = BOOLEAN_TO_JSVAL(_view->AcceptsMouseClick());
		break;
		case MODAL_PROP:
			*val = BOOLEAN_TO_JSVAL(_view->IsModal());
		break;
		case TABX_PROP:
		{
			jsdouble x = 0;

			if(_view->Parent())
			{
				TabView* tv = dynamic_cast<TabView*>(_view->Parent());
				TabView::Tab* tab = (tv) ? tv->GetTab(_view): NULL;
				if(tab) x = tab->x;
			}
			
			JS_NewDoubleValue(ctx, x, val);
		}
		break;
		case TABY_PROP:
		{
			jsdouble y = 0;

			if(_view->Parent())
			{
				TabView* tv = dynamic_cast<TabView*>(_view->Parent());
				TabView::Tab* tab = (tv) ? tv->GetTab(_view): NULL;
				if(tab) y = tab->y;
			}
			
			JS_NewDoubleValue(ctx, y, val);
		}
		break;
		case TABW_PROP:
		{
			jsdouble w = 0;

			if(_view->Parent())
			{
				TabView* tv = dynamic_cast<TabView*>(_view->Parent());
				TabView::Tab* tab = (tv) ? tv->GetTab(_view): NULL;
				if(tab) w = tab->width;
			}
			
			JS_NewDoubleValue(ctx, w, val);

		}
		break;
		case TABH_PROP:
		{
			jsdouble h = 0;

			if(_view->Parent())
			{
				TabView* tv = dynamic_cast<TabView*>(_view->Parent());
				TabView::Tab* tab = (tv) ? tv->GetTab(_view): NULL;
				if(tab) h = tab->height;
			}
			
			JS_NewDoubleValue(ctx, h, val);
		}
		break;
		case STRINGLENGTH_PROP:
		{
			float len = 0;

			GraphicsContext* gctx = _view->GetGraphicsContext();
			
			if(gctx)
			{
				unsigned int font = _view->GetFont();
				bool done = false;

				Caption* c = dynamic_cast<Caption*>(_view);
				if(c)
				{
					len = gctx->FontAdvance(font, c->GetValue().WideString());
					done = true;
				}
				
				if(!done)
				{
					TextControl* tc = dynamic_cast<TextControl*>(_view);

					if(tc)
					{
						len = gctx->FontAdvance(font, tc->WideText());
						done = true;
					}
				}
				
				if(!done)
				{
					TextView* tv = dynamic_cast<TextView*>(_view);

					if(tv)
					{
						len = tv->LongestLine();
						done = true;
					}
				}

				if(!done)
				{
					Control* c = dynamic_cast<Control*>(_view);

					if(c)
					{
						len = gctx->FontAdvance(font, c->WideLabel());
						done = true;
					}
				}
			}
			
			JS_NewDoubleValue(ctx, len, val);
		}
		break;
		case NX_PROP:
		{
			double dval = (double)_view->NormalizeValue(View::SCALE_X, _view->X());
			JS_NewDoubleValue(ctx, dval, val);
		}
		break;
		case NY_PROP:
		{
			double dval = (double)_view->NormalizeValue(View::SCALE_Y, _view->Y());
			JS_NewDoubleValue(ctx, dval, val);
		}
		break;
		case NWIDTH_PROP:
		{
			double dval = (double)_view->NormalizeValue(View::SCALE_WIDTH, _view->Width());
			JS_NewDoubleValue(ctx, dval, val);
		}
		break;
		case NHEIGHT_PROP:
		{
			double dval = (double)_view->NormalizeValue(View::SCALE_HEIGHT, _view->Height());
			JS_NewDoubleValue(ctx, dval, val);
		}
		break;
	}
	return JS_TRUE;
}

JSBool ElementDOM::SetProperty(JSContext* ctx, int propid, jsval* val)
{
	int32 ival = 0;
	double dval = 0;
	const wchar_t* text;
	
	switch(propid)
	{
		case CHECKED_PROP:
			JS_ValueToInt32(ctx, *val, &ival);
			_view->SetValue(ival);
		break;
		case ENABLED_PROP:
			JS_ValueToInt32(ctx, *val, &ival);
			_view->SetEnable(ival);
		break;
		
		case VALUE_PROP:
			JS_ValueToNumber(ctx, *val, &dval);
			_view->SetValue(dval);
		break;
		case TEXT_PROP:
			text = phui::js::ValueToWideString(*val);
			if(text)
			{
				TextContainer* tc = dynamic_cast<TextContainer*>(_view);
				if(tc)
					tc->SetText(text);
				else
					_view->SetValue(text);
			}
		break;
		case LABEL_PROP:
			text = phui::js::ValueToWideString(*val);
			if(text)
			{
				Control* ctl = dynamic_cast<Control*>(_view);
				if(ctl)
					ctl->SetLabel(text);
				else
					_view->SetValue(text);
			}
		break;
		case LEFT_PROP:
			JS_ValueToNumber(ctx, *val, &dval);
			_view->SetLeft(dval);
		break;
		case RIGHT_PROP:
			JS_ValueToNumber(ctx, *val, &dval);
			_view->SetRight(dval);
		break;
		case BOTTOM_PROP:
			JS_ValueToNumber(ctx, *val, &dval);
			_view->SetBottom(dval);
		break;
		case TOP_PROP:
			JS_ValueToNumber(ctx, *val, &dval);
			_view->SetTop(dval);
		break;
		case WIDTH_PROP:
			JS_ValueToNumber(ctx, *val, &dval);
			_view->ResizeTo(dval, _view->Height());
		break;
		case HEIGHT_PROP:
			JS_ValueToNumber(ctx, *val, &dval);
			_view->ResizeTo(_view->Width(),dval);
		break;
		case X_PROP:
			JS_ValueToNumber(ctx, *val, &dval);
			_view->MoveTo(dval,_view->Y());
		break;
		case Y_PROP:
			JS_ValueToNumber(ctx, *val, &dval);
			_view->MoveTo(_view->X(),dval);
		break;
		case SCREENX_PROP:
		{
			JS_ValueToNumber(ctx, *val, &dval);
			
			float x = (float)dval, y = 0.0f;
			
			_view->ScreenToLocal(&x, &y);
			_view->MoveTo(x, _view->Y());
		}
		break;
		case SCREENY_PROP:
		{
			JS_ValueToNumber(ctx, *val, &dval);
			
			float x = 0.0f, y = (float)dval;
			
			_view->ScreenToLocal(&x, &y);
			_view->MoveTo(_view->X(), y);
		}
		break;
		case SCROLLX_PROP:
		{
			JS_ValueToNumber(ctx, *val, &dval);

			ListView* lv = dynamic_cast<ListView*>(_view);
			if(lv)
				lv->ScrollTo((float)dval, lv->ScrollY());
		}
		break;
		case SCROLLY_PROP:
		{
			JS_ValueToNumber(ctx, *val, &dval);

			ListView* lv = dynamic_cast<ListView*>(_view);
			if(lv)
				lv->ScrollTo(lv->ScrollX(), (float)dval);
		}
		break;
		case SRC_PROP:
		{
			if(!_src) _src = new std::string;
			*_src = JS_GetStringBytes(JS_ValueToString(ctx, *val));
			
			if(_gettexture)
			{
				ImageView* iv = dynamic_cast<ImageView*>(_view);
				if(iv)
				{
					GraphicsContext::TextureInfo texid = _gettexture(_src->c_str(),false);
					iv->SetTexture(texid);
				}
			}
		}
		break;
		case VISIBLE_PROP:
		{
			JSBool visible = JS_TRUE;
			JS_ValueToBoolean(ctx, *val, &visible);
			if(visible)
				_view->Show();
			else
				_view->Hide();
		}
		break;
		case FOCUSED_PROP:
		{
			JSBool focus = JS_FALSE;
			JS_ValueToBoolean(ctx, *val, &focus);
			_view->MakeFocus(focus);
		}
		break;
		case ACCEPTSFOCUS_PROP:
		{
			JSBool acceptsfocus = JS_FALSE;
			JS_ValueToBoolean(ctx, *val, &acceptsfocus);
			_view->SetAcceptsFocus(acceptsfocus);
		}
		break;
		case MAXWIDTH_PROP:
		{
			float w, h;
			JS_ValueToNumber(ctx, *val, &dval);
			_view->MaxSize(&w, &h);
			_view->SetMaxSize(dval, h);
		}
		break;
		case MAXHEIGHT_PROP:
		{
			float w, h;
			JS_ValueToNumber(ctx, *val, &dval);
			_view->MaxSize(&w, &h);
			_view->SetMaxSize(w, dval);
		}
		break;
		case SHADOWX_PROP:
		{
			JS_ValueToNumber(ctx, *val, &dval);

			if(_view->DrawerEnabled())
			{
				CSSViewDrawer* drawer = dynamic_cast<CSSViewDrawer*>(_view->GetDrawer());
				if(drawer) drawer->SetShadowX((float)dval);
			}
		}
		break;
		case SHADOWY_PROP:
		{
			JS_ValueToNumber(ctx, *val, &dval);

			if(_view->DrawerEnabled())
			{
				CSSViewDrawer* drawer = dynamic_cast<CSSViewDrawer*>(_view->GetDrawer());
				if(drawer) drawer->SetShadowY((float)dval);
			}
		}
		break;
		case ACCEPTSMOUSE_PROP:
		{
			JSBool acceptsmouse = JS_FALSE;
			JS_ValueToBoolean(ctx, *val, &acceptsmouse);
			_view->SetAcceptsMouseClick(acceptsmouse);
		}
		break;
		case MODAL_PROP:
		{
			JSBool modal = JS_FALSE;
			JS_ValueToBoolean(ctx, *val, &modal);
			_view->SetIsModal(modal);
		}
		break;
		default:
		break;
	}

	return JS_TRUE;
}

bool ElementDOM::AddEventHandler(const char* event, const char* code)
{
	Message* msg = _view->UserMessage();
	
	if(msg)
	{
		if(stricmp(event, DOM_ONCLICK) == 0)
			msg->AddString(DOM_ONCLICK, code);
		else if(stricmp(event, DOM_ONLOAD) == 0)
			msg->AddString(DOM_ONLOAD, code);
		else if(stricmp(event, DOM_ONUNLOAD) == 0)
			msg->AddString(DOM_ONUNLOAD, code);
		else if(stricmp(event, DOM_ONDRAG) == 0)
			msg->AddString(DOM_ONDRAG, code);
		else if(stricmp(event, DOM_ONSHOW) == 0)
			msg->AddString(DOM_ONSHOW, code);
		else if(stricmp(event, DOM_ONHIDE) == 0)
			msg->AddString(DOM_ONHIDE, code);
		else if(stricmp(event, DOM_ONUPDATE) == 0)
			msg->AddString(DOM_ONUPDATE, code);
		else if(stricmp(event, DOM_ONENTER) == 0)
			msg->AddString(DOM_ONENTER, code);
		else if(stricmp(event, DOM_ONMOUSEENTER) == 0)
			msg->AddString(DOM_ONMOUSEENTER, code);
		else if(stricmp(event, DOM_ONMOUSEEXIT) == 0)
			msg->AddString(DOM_ONMOUSEEXIT, code);
		else if(stricmp(event, DOM_ONMOUSEOVER) == 0)
			msg->AddString(DOM_ONMOUSEOVER, code);
		else if(stricmp(event, DOM_ONSELECT) == 0)
			msg->AddString(DOM_ONSELECT, code);
		else if(stricmp(event, DOM_ONDBLCLICK) == 0)
			msg->AddString(DOM_ONDBLCLICK, code);
		else if(stricmp(event, DOM_ONDBLSELECT) == 0)
			msg->AddString(DOM_ONDBLSELECT, code);
		else if(stricmp(event, DOM_ONMOUSEDOWN) == 0)
			msg->AddString(DOM_ONMOUSEDOWN, code);
		else if(stricmp(event, DOM_ONRIGHTCLICK) == 0)
			msg->AddString(DOM_ONRIGHTCLICK, code);
		else if(stricmp(event, DOM_ONTAB) == 0)
			msg->AddString(DOM_ONTAB, code);
		
		if(stricmp(event, DOM_ONUPDATE) == 0)
		{
			if(_onupdate)
			{
				OnUpdatePeriodical* up = dynamic_cast<OnUpdatePeriodical*>(_onupdate);
				if(up)
					up->SetScript(code);
			}
			else
			{
				_onupdate = new OnUpdatePeriodical(_view, this, code, _view->GetSource());
			}
		}
		
		const char* script = NULL;
		if(msg->FindString(event, &script) == Message::OK && strcmp(code, script) == 0)
			return true;
	}

	return false;
}

/************************************************************************************************
	close
*************************************************************************************************/

JS_FUNCTION_DEF(ElementDOM,close)
{
	ElementDOM* dom = js::Cast<ElementDOM*>(ctx,jsobj);
	if(!dom) return JS_FALSE;

	dom->_view->Close();
	
	return JS_TRUE;
}

/************************************************************************************************
	findview
*************************************************************************************************/

JS_FUNCTION_DEF(ElementDOM,findview)
{
	ElementDOM* dom = js::Cast<ElementDOM*>(ctx,jsobj);
	if(!dom) return JS_FALSE;
	
	if(argc < 1) return JS_FALSE;
	
	View* view = dom->_view;
	if(view && argv[0] != JSVAL_NULL)
	{
		const char* name = JS_GetStringBytes(JS_ValueToString(ctx, argv[0]));
		
		view = view->FindView(name);
		if(view)
		{
			ElementDOM* viewdom = (ElementDOM*)view->UserData(DOM_VIEW_USERDATA);
			if(viewdom)
				*rval = OBJECT_TO_JSVAL((JSObject*)viewdom->jsObject());
		}
	}

	return JS_TRUE;
}

/************************************************************************************************
	findparent
*************************************************************************************************/

JS_FUNCTION_DEF(ElementDOM,findparent)
{
	ElementDOM* dom = js::Cast<ElementDOM*>(ctx,jsobj);
	if(!dom) return JS_FALSE;
	
	if(argc < 1) return JS_FALSE;

	View* view = dom->_view;

	if(view && argv[0] != JSVAL_NULL)
	{
		const char* name = JS_GetStringBytes(JS_ValueToString(ctx, argv[0]));
		
		View *parent = view->Parent();
		int tries = 50;
		
		while(parent != NULL && tries)
		{
			if(parent->Name() && std::strcmp(parent->Name(), name) == 0)
			{
				ElementDOM* parentDom = dynamic_cast<ElementDOM*>(parent->UserData(DOM_VIEW_USERDATA));

				if(parentDom)
				{
					*rval = parentDom->jsVal();
					return JS_TRUE;
				}
			}
			
			parent = parent->Parent();
			tries--;
		}
	}
	
	return JS_FALSE;
}

/************************************************************************************************
	addrow
*************************************************************************************************/

JS_FUNCTION_DEF(ElementDOM,addrow)
{
	*rval = JSVAL_NULL;
	
	if(argc < 1) return JS_FALSE;

	ElementDOM* dom = js::Cast<ElementDOM*>(ctx,jsobj);
	if(!dom) return JS_FALSE;
	
	ListView* lv = dynamic_cast<ListView*>(dom->_view);
	if(!lv) return JS_FALSE;
	
	View* row = NULL;
	bool init = true;

	if(JSVAL_IS_OBJECT(argv[0]))
	{
		JSObject* rowobj = JSVAL_TO_OBJECT(argv[0]);

		if(JS_IsArrayObject(ctx, rowobj))
		{
			jsuint len;
			JS_GetArrayLength(ctx, rowobj, &len);

			float colwidth = lv->Width()/len;
			float x = 0;
			row = new View;
			for(jsuint i = 0; i < len; i++)
			{
				jsval val;
				if(JS_GetElement(ctx, rowobj, i, &val))
				{
					float r,g,b,a;
					const wchar_t* wstr = phui::js::ValueToWideString(argv[0]);
					Caption* col = new Caption;
					col->SetValue(wstr);
					col->ResizeTo(lv->Width(), lv->GetGraphicsContext()->FontHeight(lv->GetFont()));
					lv->GetTextColor(&r,&g,&b,&a);
					col->SetTextColor(r,g,b,a);
					col->MoveTo(x,0);
					row->AddChild(col);
					x += colwidth;
				}
			}
			row->ResizeTo(x,lv->GetGraphicsContext()->FontHeight(lv->GetFont()));
		}
		else
		{
			// see if we passed in a view and use that
			ElementDOM* dom = js::Cast<ElementDOM*>(ctx,jsobj);

			if(dom)
			{
				row = dynamic_cast<View*>(dom->_view);
				init = false; // don't need to init, its already done
			}
		}
	}
	else
	{
		const wchar_t* wstr = phui::js::ValueToWideString(argv[0]);
		if(wstr)
		{
			std::string text = (char*)utf16_to_utf8(wstr); 

			if(text[0]=='<')
			{
				phui::memIOAddBuffer("listview.addrow", (void*)text.c_str(), text.length());
				//SetUIXMLIOCallbacks(&memIOCallbacks);
				row = LoadUIXML("listview.addrow", &memIOCallbacks);
				phui::memIORemoveBuffer("listview.addrow");
			}
			else
			{
				float r,g,b,a;
				row = new Caption;
				row->SetValue(wstr);
				row->ResizeTo(lv->Width(), lv->GetGraphicsContext()->FontHeight(lv->GetFont()));
				lv->GetTextColor(&r,&g,&b,&a);
				row->SetTextColor(r,g,b,a);
			}
		}
	}
	
	if(row)
	{
		bool selectable = (argc == 3) ? JSVAL_TO_BOOLEAN(argv[2]): true;

		if(argc > 1)
			lv->AddRow(row, new js::jsvalRef(ctx, argv[1]), selectable);
		else
			lv->AddRow(row, NULL, selectable);
		
		if(init)
		{
			dom_init(row,dom);
			dom_script_init(row);
		}
		
		phui::js::Object* obj = dynamic_cast<phui::js::Object*>(row->UserData(DOM_VIEW_USERDATA));

		if(obj)
			*rval = obj->jsVal();
	}
	
	return JS_TRUE;
}

/************************************************************************************************
	removerow
*************************************************************************************************/

JS_FUNCTION_DEF(ElementDOM,removerow)
{
	*rval = JSVAL_FALSE;
	
	if(argc < 1) return JS_FALSE;

	ElementDOM* dom = js::Cast<ElementDOM*>(ctx,jsobj);
	if(!dom) return JS_FALSE;
	
	ListView* lv = dynamic_cast<ListView*>(dom->_view);
	if(!lv) return JS_FALSE;
	
	int32 index = -1;
	JS_ValueToInt32(ctx, argv[0], &index);

	if(index >= 0)
	{
		lv->RemoveRow((unsigned int)index);
		*rval = JSVAL_TRUE;
	}
	
	return JS_TRUE;
}

/************************************************************************************************
	addview
*************************************************************************************************/

JS_FUNCTION_DEF(ElementDOM, addview)
{
	*rval = JSVAL_NULL;

	if(argc < 1) return JS_FALSE;

	ElementDOM* dom = js::Cast<ElementDOM*>(ctx,jsobj);
	if(!dom) return JS_FALSE;

	const wchar_t* wstr = phui::js::ValueToWideString(argv[0]);
	if(wstr)
	{
		std::string text = (char*)utf16_to_utf8(wstr); 

		phui::memIOAddBuffer("elementdom.addview", (void*)text.c_str(), text.length());
		//SetUIXMLIOCallbacks(&memIOCallbacks);
		View* v = LoadUIXML("elementdom.addview", &memIOCallbacks, dom->_view);
		phui::memIORemoveBuffer("elementdom.addview");

		dom_init(v,dom);
		dom_script_init(v);
		
		dom->_view->AddChild(v);
		v->Show();
		
		ElementDOM* vDom = dynamic_cast<ElementDOM*>(v->UserData(DOM_VIEW_USERDATA));
		
		if(vDom)
			*rval = vDom->jsVal();
	}
	
	return JS_TRUE;
}

/************************************************************************************************
	delview
*************************************************************************************************/

JS_FUNCTION_DEF(ElementDOM, delview)
{
	if(argc < 1) return JS_FALSE;

	ElementDOM* dom = js::Cast<ElementDOM*>(ctx,jsobj);
	if(!dom) return JS_FALSE;

	const wchar_t* wstr = phui::js::ValueToWideString(argv[0]);
	if(wstr)
	{
		std::string text = (char*)utf16_to_utf8(wstr); 
		
		View *found = dom->_view->FindView(text.c_str());
		
		if(found)
			found->Close();
	}
	
	return JS_TRUE;
}

/************************************************************************************************
	clearrows
*************************************************************************************************/

JS_FUNCTION_DEF(ElementDOM,clearrows)
{
	ElementDOM* dom = js::Cast<ElementDOM*>(ctx,jsobj);
	if(!dom) return JS_FALSE;

	ListView* lv = dynamic_cast<ListView*>(dom->_view);
	if(!lv) return JS_FALSE;
	
	lv->ClearRows();

	return JS_TRUE;
}

/************************************************************************************************
	isselectable
*************************************************************************************************/

JS_FUNCTION_DEF(ElementDOM,isselectable)
{
	*rval = JSVAL_FALSE;

	if(argc < 1) return JS_FALSE;

	ElementDOM* dom = js::Cast<ElementDOM*>(ctx,jsobj);
	if(!dom) return JS_FALSE;

	ListView* lv = dynamic_cast<ListView*>(dom->_view);
	if(!lv) return JS_FALSE;
	
	int i = JSVAL_TO_INT(argv[0]);
	
	if(lv->IsSelectable(i))
		*rval = JSVAL_TRUE;
	
	return JS_TRUE;
}

/************************************************************************************************
	scrollby
*************************************************************************************************/

JS_FUNCTION_DEF(ElementDOM,scrollby)
{
	if(argc < 1) return JS_FALSE;

	ElementDOM* dom = js::Cast<ElementDOM*>(ctx,jsobj);
	if(!dom) return JS_FALSE;
	
	if(!dom->_view) return JS_FALSE;
	
	int c = JSVAL_TO_INT(argv[0]);
	
	dom->_view->MouseScroll(0, 0, c);

	return JS_TRUE;
}

/************************************************************************************************
	scrollto
*************************************************************************************************/

JS_FUNCTION_DEF(ElementDOM,scrollto)
{
	if(argc < 1) return JS_FALSE;

	ElementDOM* dom = js::Cast<ElementDOM*>(ctx,jsobj);
	if(!dom) return JS_FALSE;
	
	if(!dom->_view) return JS_FALSE;
	
	double sy = 0;
	
	JS_ValueToNumber(ctx, argv[0], &sy);
	
	dom->_view->ScrollTo(dom->_view->ScrollX(), (float)sy);
	
	return JS_TRUE;
}

/************************************************************************************************
	scrolltorow
*************************************************************************************************/

JS_FUNCTION_DEF(ElementDOM,scrolltorow)
{
	if(argc < 1) return JS_FALSE;

	ElementDOM* dom = js::Cast<ElementDOM*>(ctx,jsobj);
	if(!dom) return JS_FALSE;
	
	ListView* lv = dynamic_cast<ListView*>(dom->_view);
	if(!lv) return JS_FALSE;
	
	int i = JSVAL_TO_INT(argv[0]);
	
	lv->ScrollByRowIndex(i);

	return JS_TRUE;
}

/************************************************************************************************
	getstyleproperty
*************************************************************************************************/

JS_FUNCTION_DEF(ElementDOM,getstyleproperty)
{
	*rval = JSVAL_NULL;

	if(argc < 1) return JS_FALSE;
	
	ElementDOM* dom = js::Cast<ElementDOM*>(ctx,jsobj);
	if(!dom) return JS_FALSE;
	
	CSSViewDrawer* drawer = dynamic_cast<CSSViewDrawer*>(dom->_view->GetDrawer());
	if(!drawer) return JS_FALSE;
	
	const char* key = JS_GetStringBytes(JS_ValueToString(ctx, argv[0]));
	if(!key) return JS_FALSE;
	
	const char* val = drawer->GetStyleProperty(key);
	if(val)
		*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(ctx,val));
	
	return JS_TRUE;
}

/************************************************************************************************
	setstyleproperty
*************************************************************************************************/

JS_FUNCTION_DEF(ElementDOM,setstyleproperty)
{
	if(argc < 2) return JS_FALSE;
	
	ElementDOM* dom = js::Cast<ElementDOM*>(ctx,jsobj);
	if(!dom) return JS_FALSE;
	
	const char* key = JS_GetStringBytes(JS_ValueToString(ctx, argv[0]));
	if(!key) return JS_FALSE;
	
	const char* val = JS_GetStringBytes(JS_ValueToString(ctx, argv[1]));
	if(!val) return JS_FALSE;
	
	bool recursive = (argc == 3 && JSVAL_TO_BOOLEAN(argv[2])) ? true: false;
	
	dom->_view->SetDrawerStyle(key, val, recursive);
	
	return JS_TRUE;
}

/************************************************************************************************
	getimeproperty
*************************************************************************************************/

JS_FUNCTION_DEF(ElementDOM,getimeproperty)
{
	*rval = JSVAL_NULL;

	if(argc < 1) return JS_FALSE;
	
	ElementDOM* dom = js::Cast<ElementDOM*>(ctx,jsobj);
	if(!dom) return JS_FALSE;
	
	IMEControl* ime = dynamic_cast<IMEControl*>(dom->_view);
	if(!ime) return JS_FALSE;
	
	const char* key = JS_GetStringBytes(JS_ValueToString(ctx, argv[0]));
	if(!key) return JS_FALSE;
	
	if(!stricmp(key, "description"))
		*rval = phui::js::NewString(ime->GetDescription().c_str());
	else if(!stricmp(key, "composition"))
		*rval = phui::js::NewString(ime->GetCompositionString().c_str());
	else if(!stricmp(key, "reading"))
		*rval = phui::js::NewString(ime->GetReadingString().c_str());
	else if(!stricmp(key, "result"))
		*rval = phui::js::NewString(ime->GetResultString().c_str());
	else if(!stricmp(key, "selection"))
		*rval = INT_TO_JSVAL(ime->GetSelection());
	else if(!stricmp(key, "curpos"))
		*rval = INT_TO_JSVAL(ime->GetCursorPosition());
	else if(!stricmp(key, "candidates"))
	{
		const std::vector<std::wstring>* list = ime->GetCandidateList();

		if(list && list->size())
			*rval = INT_TO_JSVAL(list->size());
		else
			*rval = INT_TO_JSVAL(0);
	}
	else if(!stricmp(key, "candidate"))
	{
		int32 index = -1;

		JS_ValueToInt32(ctx, argv[1], &index);

		const std::vector<std::wstring>* list = ime->GetCandidateList();
		
		if(index >= 0 && list && index < (int32)list->size())
			*rval = phui::js::NewString(list->at(index).c_str());
		else
			*rval = phui::js::NewString("");
	}
	
	return JS_TRUE;
}

/************************************************************************************************
	scalevalue
*************************************************************************************************/

JS_FUNCTION_DEF(ElementDOM,scalev)
{
	*rval = JSVAL_NULL;

	if(argc < 2) return JS_FALSE;
	
	ElementDOM* dom = js::Cast<ElementDOM*>(ctx,jsobj);
	if(!dom) return JS_FALSE;
	
	const char* name = JS_GetStringBytes(JS_ValueToString(ctx, argv[0]));
	if(!name) return JS_FALSE;

	int type = 0;
	double dval = 0.0f;
	
	if(!stricmp(name, "width") || !stricmp(name, "w"))
		type = View::SCALE_WIDTH;
	else if(!stricmp(name, "height") || !stricmp(name, "h"))
		type = View::SCALE_HEIGHT;
	else if(!stricmp(name, "x"))
		type = View::SCALE_X;
	else if(!stricmp(name, "y"))
		type = View::SCALE_Y;

	JS_ValueToNumber(ctx, argv[1], &dval);

	dval = (double)dom->_view->ScaleValue(type, (float)dval);
	
	JS_NewDoubleValue(ctx, dval, rval);

	return JS_TRUE;
}

/************************************************************************************************
	normvalue
*************************************************************************************************/

JS_FUNCTION_DEF(ElementDOM,normv)
{
	*rval = JSVAL_NULL;

	if(argc < 2) return JS_FALSE;
	
	ElementDOM* dom = js::Cast<ElementDOM*>(ctx,jsobj);
	if(!dom) return JS_FALSE;
	
	const char* name = JS_GetStringBytes(JS_ValueToString(ctx, argv[0]));
	if(!name) return JS_FALSE;

	int type = 0;
	double dval = 0.0f;
	
	if(!stricmp(name, "width") || !stricmp(name, "w"))
		type = View::SCALE_WIDTH;
	else if(!stricmp(name, "height") || !stricmp(name, "h"))
		type = View::SCALE_HEIGHT;
	else if(!stricmp(name, "x"))
		type = View::SCALE_X;
	else if(!stricmp(name, "y"))
		type = View::SCALE_Y;

	JS_ValueToNumber(ctx, argv[1], &dval);

	dval = (double)dom->_view->NormalizeValue(type, (float)dval);
	
	JS_NewDoubleValue(ctx, dval, rval);

	return JS_TRUE;
}

/************************************************************************************************
	settip
*************************************************************************************************/

JS_FUNCTION_DEF(ElementDOM,settip)
{
	if(argc < 1) return JS_FALSE;

	ElementDOM* dom = js::Cast<ElementDOM*>(ctx,jsobj);
	if(!dom) return JS_FALSE;
	
	const wchar_t* tip = js::ValueToWideString(argv[0]);
	if(!tip) return JS_FALSE;
	
	dom->_view->SetToolTip(tip);
	
	return JS_TRUE;
}

/************************************************************************************************
	addrow
*************************************************************************************************/

JS_FUNCTION_DEF(ElementDOM,showtab)
{
	if(argc < 2) return JS_FALSE;

	ElementDOM* dom = js::Cast<ElementDOM*>(ctx,jsobj);
	if(!dom) return JS_FALSE;
	if(!dom->_view) return JS_FALSE;
	
	TabView* tv;
	
	// lets try this view, maybe its the tabview
	tv = dynamic_cast<TabView*>(dom->_view);
	
	// if this view wasn't, then maybe this is a tab and the parent will be a tabview
	if(!tv)	tv = dynamic_cast<TabView*>(dom->_view->Parent());

	// no tabviews found, bye
	if(!tv) return JS_FALSE;
	
	int index		= JSVAL_TO_INT(argv[0]);
	bool visible	= JSVAL_TO_BOOLEAN(argv[1]);
	
	if(tv->ValidTab(index))
		tv->GetTab(index).visible = visible;
	
	return JS_TRUE;
}

/************************************************************************************************
	resizetotext
*************************************************************************************************/

JS_FUNCTION_DEF(ElementDOM,resizetotext)
{
	ElementDOM* dom = js::Cast<ElementDOM*>(ctx,jsobj);
	if(!dom) return JS_FALSE;
	
	TextContainer* tc = dynamic_cast<TextContainer*>(dom->_view);
	if(!tc) return JS_FALSE;
	
	tc->ResizeToText();
	
	return JS_TRUE;
}

/************************************************************************************************
	resizetoaspect
*************************************************************************************************/

JS_FUNCTION_DEF(ElementDOM,resizetoaspect)
{
	if(argc < 1) return JS_FALSE;

	ElementDOM* dom = js::Cast<ElementDOM*>(ctx,jsobj);
	if(!dom) return JS_FALSE;
	
	dom->_view->ResizeToAspect(JSVAL_TO_INT(argv[0]));
	
	return JS_TRUE;
}

/************************************************************************************************
	snapshot
*************************************************************************************************/

JS_FUNCTION_DEF(ElementDOM,snapshot)
{
	if(argc < 4) return JS_FALSE;
	
	ElementDOM* dom = js::Cast<ElementDOM*>(ctx,jsobj);
	if(!dom) return JS_FALSE;
	
	MirrorView* mv = dynamic_cast<MirrorView*>(dom->_view);
	if(!mv) return JS_FALSE;

	double sx, sy, sw, sh;
	sx = sy = sw = sh = 0;
	
	JS_ValueToNumber(ctx, argv[0], &sx);
	JS_ValueToNumber(ctx, argv[1], &sy);
	JS_ValueToNumber(ctx, argv[2], &sw);
	JS_ValueToNumber(ctx, argv[3], &sh);
	
	mv->SnapShot(sx, sy, sw, sh);
	
	return JS_TRUE;
}


/************************************************************************************************
	addmenu
*************************************************************************************************/

JS_FUNCTION_DEF(ElementDOM, addmenu)
{
	*rval = JSVAL_NULL;
	
	if(argc < 1) return JS_FALSE;

	ElementDOM* parentDom = js::Cast<ElementDOM*>(ctx,jsobj);
	if(!parentDom) return JS_FALSE;
	
	View* parentView = parentDom->_view;
	View* childView = NULL;
	
	// lets create the new menu
	const wchar_t* wstr = phui::js::ValueToWideString(argv[0]);
	if(wstr)
	{
		std::string text = (char*)utf16_to_utf8(wstr); 

		phui::memIOAddBuffer("ctxmenu.addmenu", (void*)text.c_str(), text.length());
		childView = LoadUIXML("ctxmenu.addmenu", &memIOCallbacks);
		phui::memIORemoveBuffer("ctxmenu.addmenu");
	}
	
	if(childView)
	{
		dom_init(childView, parentDom);
		dom_script_init(childView);

		CtxMenu* childMenu = dynamic_cast<CtxMenu*>(childView);
		ElementDOM* childDom = dynamic_cast<ElementDOM*>(childView->UserData(DOM_VIEW_USERDATA));

		if(childMenu && argc == 2)
			childMenu->SetUserData(DOM_VIEW_MENUDATA, new js::jsvalRef(childDom->jsContext(), argv[1]));

		if(childDom)
		{
			phui::js::Object* obj = dynamic_cast<phui::js::Object*>(childDom);
			if(obj)
				*rval = obj->jsVal();
		}
		
		parentView->AddChild(childView);
	}
	
	return JS_TRUE;
}

/************************************************************************************************
	addeventhandler
*************************************************************************************************/

JS_FUNCTION_DEF(ElementDOM, addevent)
{
	*rval = JSVAL_FALSE;
	
	if(argc < 2) return JS_FALSE;

	ElementDOM* parentDom = js::Cast<ElementDOM*>(ctx,jsobj);
	if(!parentDom) return JS_FALSE;

	std::string event = "dom/";
	std::string code;
	
	const wchar_t* event_str = phui::js::ValueToWideString(argv[0]);
	if(event_str)
		event += (char*)utf16_to_utf8(event_str); 
	
	const wchar_t* code_str = phui::js::ValueToWideString(argv[1]);
	if(code_str)
		code = (char*)utf16_to_utf8(code_str);

	if(code.size())
		*rval = BOOLEAN_TO_JSVAL(parentDom->AddEventHandler(event.c_str(), code.c_str()));
	
	return JS_TRUE;
}

/************************************************************************************************
	addcommand
*************************************************************************************************/

JS_FUNCTION_DEF(ElementDOM,addcommand)
{
	ElementDOM* dom = js::Cast<ElementDOM*>(ctx,jsobj);
	if(!dom) return JS_FALSE;
	
	ConsoleView* cv = dynamic_cast<ConsoleView*>(dom->_view);
	if(!cv) return JS_FALSE;

	const wchar_t* cmd_str = phui::js::ValueToWideString(argv[0]);
	if(cmd_str)
		cv->executeCommandLine(std::wstring(cmd_str));
	
	return JS_TRUE;
}

/************************************************************************************************
	getchild
*************************************************************************************************/

JS_FUNCTION_DEF(ElementDOM,getchild)
{
	*rval = JSVAL_NULL;
	
	ElementDOM* dom = js::Cast<ElementDOM*>(ctx,jsobj);
	if(!dom) return JS_FALSE;
	
	if(argc < 1) return JS_FALSE;
	
	View* view = dom->_view;
	if(view && argv[0] != JSVAL_NULL)
	{
		int32 index = -1;
		JS_ValueToInt32(ctx, argv[0], &index);
		
		ChildList* children = view->Children();

		if(children && index >= 0 && index < (int32)children->size())
		{
			View* child = (*children)[index];

			if(child)
			{
				ElementDOM* viewdom = (ElementDOM*)child->UserData(DOM_VIEW_USERDATA);
				if(viewdom)
					*rval = OBJECT_TO_JSVAL((JSObject*)viewdom->jsObject());
			}
		}
	}

	return JS_TRUE;
}

/************************************************************************************************
	ScriptFilter
*************************************************************************************************/

ScriptFilter::ScriptFilter() : MessageFilter(ANY_EVENT) 
{
	_lastclickview = 0;
	_lastclicktime = 0;
}

bool ScriptFilter::Filter(Message* msg, MessageHandler** target, MessageHandler**)
{
	const char* script = NULL;
	const char* filename = NULL;
	js::Object* obj = NULL;
	View* v = NULL;

	if(target && *target)
		v = dynamic_cast<View*>(*target);
	
	if(v) filename = v->GetSource();
	
	if(msg->what == DOM_SCRIPT_MSG)
	{		
		msg->FindString(DOM_SCRIPT, &script);
		msg->FindPointer(DOM_OBJECT, (void**)&obj);
		
		if(obj == NULL)
			obj = js::GlobalObject();
			
		if(script)
			obj->RunScript(script, filename);
			
		return true;
	}
	else if(msg->what == MOUSE_DRAG)
	{
		if(v && v->UserMessage()->FindString(DOM_ONDRAG,&script) == Message::OK)
		{
			obj = dynamic_cast<js::Object*>(v->UserData(DOM_VIEW_USERDATA));
			if(script && obj)
				obj->RunScript(script, filename);
		}
	}
	else if(msg->what == SHOW_MSG)
	{
		if(v && v->UserMessage()->FindString(DOM_ONSHOW,&script) == Message::OK)
		{
			obj = dynamic_cast<js::Object*>(v->UserData(DOM_VIEW_USERDATA));
			if(script && obj)
				obj->RunScript(script, filename);					
		}		
	}
	else  if(msg->what == HIDE_MSG)
	{
		if(v && v->UserMessage()->FindString(DOM_ONHIDE,&script) == Message::OK)
		{
			obj = dynamic_cast<js::Object*>(v->UserData(DOM_VIEW_USERDATA));
			if(script && obj)
				obj->RunScript(script, filename);			
		}		
	}
	else if(msg->what == CLOSE_MSG)
	{
		View* child;

		if(msg->FindPointer(UI_VIEW_PTR, (void**)&child) == Message::OK && child)
		{
			child = child->ChildAt(0);

			if(child && child->UserMessage()->FindString(DOM_ONUNLOAD, &script) == Message::OK)
			{
				obj = dynamic_cast<js::Object*>(child->UserData(DOM_VIEW_USERDATA));
				if(obj && script)
					obj->RunScript(script, filename);			
			}
		}
	}
	else if(msg->what == MOUSE_MOVE)
	{
		long state;
		if(msg->FindLong(UI_MOUSE_MOVESTATE, &state) == Message::OK)
		{
			if(state == MOUSE_ENTER)
			{
				if(v && v->UserMessage()->FindString(DOM_ONMOUSEENTER, &script) == Message::OK)
				{
					obj = dynamic_cast<js::Object*>(v->UserData(DOM_VIEW_USERDATA));
					if(script && obj)
						obj->RunScript(script, filename);			
				}		
			}
			else if(state == MOUSE_EXIT)
			{
				if(v && v->UserMessage()->FindString(DOM_ONMOUSEEXIT, &script) == Message::OK)
				{
					obj = dynamic_cast<js::Object*>(v->UserData(DOM_VIEW_USERDATA));
					if(script && obj)
						obj->RunScript(script, filename);			
				}		
			}
			else if(state == MOUSE_OVER)
			{
				if(v && v->UserMessage()->FindString(DOM_ONMOUSEOVER, &script) == Message::OK)
				{
					obj = dynamic_cast<js::Object*>(v->UserData(DOM_VIEW_USERDATA));
					if(script && obj)
						obj->RunScript(script, filename);			
				}		
			}
		}
	}
	else if(msg->what == SELECT_MSG)
	{
		if(v && v->UserMessage()->FindString(DOM_ONSELECT, &script) == Message::OK)
		{
			obj = dynamic_cast<js::Object*>(v->UserData(DOM_VIEW_USERDATA));
			if(script && obj)
				obj->RunScript(script, filename);			
		}
	}
	else if(msg->what == DBLSELECT_MSG)
	{
		if(v && v->UserMessage()->FindString(DOM_ONDBLSELECT, &script) == Message::OK)
		{
			obj = dynamic_cast<js::Object*>(v->UserData(DOM_VIEW_USERDATA));
			if(script && obj)
				obj->RunScript(script, filename);			
		}
	}
	else if(msg->what == MOUSE_UP)
	{
		long btn = -1;
		msg->FindLong(UI_MOUSE_BUTTON, &btn);
		if(btn == 0)
		{
			unsigned int t = (*target)->ParentLoop()->FrameTime();

			if(t - _lastclicktime < 500 && _lastclickview == v)
			{
				if(v && v->UserMessage()->FindString(DOM_ONDBLCLICK, &script) == Message::OK)
				{
					obj = dynamic_cast<js::Object*>(v->UserData(DOM_VIEW_USERDATA));
					if(script && obj)
						obj->RunScript(script, filename);			
				}			
			}
			else
			{
				phui::Control* ctl = dynamic_cast<phui::Control*>(*target);
				
				if(!ctl && v && v->UserMessage()->FindString(DOM_ONCLICK, &script) == Message::OK)
				{
					obj = dynamic_cast<js::Object*>(v->UserData(DOM_VIEW_USERDATA));
					if(script && obj)
						obj->RunScript(script, filename);			
				}
			}
			
			_lastclicktime = t;
			_lastclickview = v;
		}
	}
	else if(msg->what == MOUSE_DOWN)
	{
		long btn = -1;
		msg->FindLong(UI_MOUSE_BUTTON, &btn);

		if(btn == 0)
		{
			if(v && v->UserMessage()->FindString(DOM_ONMOUSEDOWN, &script) == Message::OK)
			{
				obj = dynamic_cast<js::Object*>(v->UserData(DOM_VIEW_USERDATA));
				if(script && obj)
					obj->RunScript(script, filename);			
			}			
		}
		else if(btn == 1)
		{
			if(v && v->UserMessage()->FindString(DOM_ONRIGHTCLICK, &script) == Message::OK)
			{
				obj = dynamic_cast<js::Object*>(v->UserData(DOM_VIEW_USERDATA));
				if(script && obj)
					obj->RunScript(script, filename);			
			}		
		}
	}
	else if(msg->what == TAB_MSG)
	{
		phui::TabView* tv = dynamic_cast<phui::TabView*>(*target);

		if(tv && tv->UserMessage()->FindString(DOM_ONTAB, &script) == Message::OK)
		{
			obj = dynamic_cast<js::Object*>(tv->UserData(DOM_VIEW_USERDATA));
			if(script && obj)
				obj->RunScript(script, filename);			
		}
	}
	
	return false;
}

/************************************************************************************************
	dom_init
*************************************************************************************************/

static void dom_init(View* view, ElementDOM* parent, const char* filename)
{
	if(!filename)
		filename = view->GetSource();
	
	ElementDOM* elem = 0;

	const char* classname;
	if(view->UserMessage()->FindString(DOM_CLASS, &classname) == Message::OK)
		elem = CreateElementFromView(classname, view, parent);

	if(!elem)
		elem = new ElementDOM(view, parent);

	if(parent == NULL)
		parent = elem;
	
	const char* script;	
	if(view->UserMessage()->FindString(DOM_ONLOAD, &script) == Message::OK)
		phui::DOM::RunScript(view->ParentLoop(), elem, script);

	unsigned int count = view->ChildCount();
	for(unsigned int i = 0; i < count; i++)
	{
		dom_init(view->ChildAt(i), parent, filename);
	}
		
	if(elem && view->UserMessage()->FindString(DOM_ONUPDATE, &script) == Message::OK)
	{
		elem->_onupdate = new OnUpdatePeriodical(view, elem, script, view->GetSource());
	}
}

/************************************************************************************************
	dom_script_init
*************************************************************************************************/

static void dom_script_init(View* view, const char* filename)
{
	if(!filename)
		filename = view->GetSource();
	
	ElementDOM* elem = (ElementDOM*)view->UserData(DOM_VIEW_USERDATA);
	
	if(elem)
	{
		const char* script;	
		for(int i = 0; ; i++)
		{
			if(view->UserMessage()->FindString(DOM_SCRIPT, &script, i) != Message::OK)
				break;
				
			ElementDOM* ctx = elem->_window;
			bool scriptprivate = false;
			if(view->UserMessage()->FindBool(DOM_SCRIPT_PRIVATE, &scriptprivate, i) == Message::OK && scriptprivate)
				ctx = elem;
				
			ctx->RunScript(script, filename);
		}
	}
	
	unsigned int count = view->ChildCount();
	for(unsigned int i = 0; i < count; i++)
	{
		dom_script_init(view->ChildAt(i), filename);
	}
}

/************************************************************************************************
	OnUpdatePeriodical
*************************************************************************************************/

OnUpdatePeriodical::OnUpdatePeriodical(View* v, js::Object* jsobj, const char* script, const char* filename)
{
	_jsobj = jsobj;
	_jscript = jsobj->CompileScript(script, filename);
	_view = v;
	_script = script;
	_source = filename;

	if(_jscript)
		StartExecuting();

}

OnUpdatePeriodical::~OnUpdatePeriodical()
{
	if(_jscript && _jsobj)
		_jsobj->DestroyScript(_jscript);
}

void OnUpdatePeriodical::Execute(unsigned int t)
{
	if(_view->IsVisible() && _jsobj && _jscript)
		_jsobj->RunScript(_script.c_str(), _source.c_str());

	//		_jsobj->RunScript(_jscript);
}