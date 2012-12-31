#ifndef DOM_H
#define DOM_H

#include <string>

#include "phui/Referenced.h"
#include "phui/Javascript.h"
#include "phui/GraphicsContext.h"

namespace phui
{

class View;
class MessageLoop;
class Message;
struct IOCallbacks;
class Periodical;
class CSSViewDrawer;

namespace DOM
{

/************************************************************************************************
	DOM
*************************************************************************************************/
// Object model for phui interface elements

#define DOM_SCRIPT_MSG  'DMRN'

#define DOM_SCRIPT  "dom/script"					// the script - string (may change to a precompiled script)
#define DOM_SCRIPT_PRIVATE  "dom/script/private"	// bool attribute sets scope
#define DOM_OBJECT  "dom/object"   					// a js::Object* pointer


#define DOM_ONCLICK		"dom/onclick"
#define DOM_ONLOAD		"dom/onload"
#define DOM_ONUNLOAD	"dom/onunload"
#define DOM_ONDRAG		"dom/ondrag"
#define DOM_ONSHOW		"dom/onshow"
#define DOM_ONHIDE		"dom/onhide"
#define DOM_ONUPDATE	"dom/onupdate"
#define DOM_ONENTER		"dom/onenter"
#define DOM_CLASS		"dom/class"
#define DOM_ONMOUSEENTER	"dom/onmouseenter"
#define DOM_ONMOUSEEXIT		"dom/onmouseexit"
#define DOM_ONMOUSEOVER		"dom/onmouseover"
#define DOM_ONSELECT		"dom/onselect"
#define DOM_ONDBLCLICK		"dom/ondblclick"
#define DOM_ONDBLSELECT 	"dom/ondblselect"
#define DOM_ONMOUSEDOWN 	"dom/onmousedown"
#define DOM_ONRIGHTCLICK	"dom/onrightclick"
#define DOM_ONTAB			"dom/ontab"

#define DOM_SET_SRC_MSG 'DMSR'
#define DOM_SRC			"dom/src"

#define DOM_VIEW_USERDATA	"dom" // name of UserData attached to view
#define DOM_VIEW_MENUDATA	"menudom" // name of menu data attached to a context menu

class ElementDOM;

typedef ElementDOM* (*ElementFromView)(const char* classname, View* view, ElementDOM* parent);

void RegisterTagWithDOM(const char* tag, ElementFromView);
ElementDOM* CreateElementFromView(const char* classname, View* view, ElementDOM* parent);
void SetTextureCallback(GraphicsContext::TextureInfo (*gettexture)(const char*, bool));

/** Called after parsing XUL to finalize the scripting, stores script, 
	creates a JS object (sets to View's UserData()), add JS message filter.
	Walks down the entire view tree. */
void Init(View*);

/** Adds a common filter to Looper that will execute script messages */
void AddScriptMessageFilter(MessageLoop*); 
void RunScript(MessageLoop*, js::Object*, const char* script);

class ElementDOM : public js::Object
{
public:
	ElementDOM(View*, ElementDOM*, JSClass* jsclass = 0);
	
	virtual JSBool GetProperty(JSContext*, int propid, jsval* val);
	virtual JSBool SetProperty(JSContext*, int propid, jsval* val);

	bool AddEventHandler(const char* event, const char* code);

	View* _view;
	ElementDOM* _window;
	Periodical* _onupdate;
	std::string* _src;
	
enum { WINDOW_PROP, CHECKED_PROP, ENABLED_PROP, VALUE_PROP, 
		LEFT_PROP, RIGHT_PROP, BOTTOM_PROP, TOP_PROP, WIDTH_PROP, HEIGHT_PROP, X_PROP, Y_PROP, SRC_PROP,
		DOCUMENT_PROP, SELECTEDITEM_PROP, LABEL_PROP, NAME_PROP, SELECTEDDATA_PROP, VISIBLE_PROP, ID_PROP, 
		TEXT_PROP, FOCUSED_PROP, SELECTEDROW_PROP, PARENT_PROP, LENGTH_PROP, SCREENX_PROP, SCREENY_PROP,
		SELECTEDINDEX_PROP, SCROLLX_PROP, SCROLLY_PROP, ACCEPTSFOCUS_PROP, MAXWIDTH_PROP, MAXHEIGHT_PROP,
		SHADOWX_PROP, SHADOWY_PROP, ACCEPTSMOUSE_PROP, MODAL_PROP, TABX_PROP, TABY_PROP, TABW_PROP, TABH_PROP,
		STRINGLENGTH_PROP, WINDOWX_PROP, WINDOWY_PROP, PARENTWIN_PROP, NX_PROP, NY_PROP, NWIDTH_PROP, NHEIGHT_PROP};
	
JS_CLASS_DATA
JS_FUNCTION(close);
JS_FUNCTION(findview);
JS_FUNCTION(addrow);
JS_FUNCTION(removerow);
JS_FUNCTION(clearrows);
JS_FUNCTION(getstyleproperty);
JS_FUNCTION(setstyleproperty);
JS_FUNCTION(findparent);
JS_FUNCTION(scrollby);
JS_FUNCTION(scrolltorow);
JS_FUNCTION(addview);
JS_FUNCTION(settip);
JS_FUNCTION(delview);
JS_FUNCTION(scrollto);
JS_FUNCTION(showtab);
JS_FUNCTION(resizetotext);
JS_FUNCTION(isselectable);
JS_FUNCTION(addmenu);
JS_FUNCTION(addevent);
JS_FUNCTION(addcommand);
JS_FUNCTION(getchild);
JS_FUNCTION(resizetoaspect);
JS_FUNCTION(snapshot);
JS_FUNCTION(getimeproperty);
JS_FUNCTION(scalev);
JS_FUNCTION(normv);

protected:
	~ElementDOM();
};

}
}

#endif // DOM_H
