#include "phui-precompiled-header.h"

#include <string>
#include <map>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
#define snprintf _snprintf
#endif

#include "libxml/parser.h"
#include "libxml/tree.h"

#include "phui/UIXML.h"
#include "phui/Util.h"
#include "phui/View.h"
#include "phui/ImageView.h"
#include "phui/StencilView.h"
#include "phui/Viewport.h"
#include "phui/DOM.h"
#include "phui/CSSViewDrawer.h"
#include "phui/Message.h"
#include "phui/Caption.h"
#include "phui/TabView.h"
#include "phui/Button.h"
#include "phui/Checkbox.h"
#include "phui/ProgressBar.h"
#include "phui/StackView.h"
#include "phui/ListView.h"
#include "phui/TextView.h"
#include "phui/TextControl.h"
#include "phui/PopupMenuButton.h"
#include "phui/Menu.h"
#include "phui/Scrollbar.h"
//#include "phui/ResizeWidget.h"
#include "phui/Slider.h"
#include "phui/ContextMenu.h"
#include "phui/CtxMenu.h"
#include "phui/RadioButton.h"
#include "phui/Panel.h"
#include "phui/ConsoleView.h"
#include "phui/MirrorView.h"
#include "phui/IMEControl.h"

using namespace phui;

/************************************************************************************************
	UIXML
*************************************************************************************************/

#define UIXML_VIEW_TAG			"view"
#define UIXML_IMAGE_TAG			"image"
#define UIXML_STENCIL_TAG		"stencil"
#define UIXML_SCRIPT_TAG		"script"
#define UIXML_CAPTION_TAG		"caption"
#define UIXML_TABVIEW_TAG		"tabview"
#define UIXML_TAB_TAG			"tab"
#define UIXML_CHECKBOX_TAG		"checkbox"
#define UIXML_PROGRESSBAR_TAG	"progressbar"
#define UIXML_VIEWPORT_TAG		"viewport"
#define UIXML_STACKVIEW_TAG		"stackview"
#define UIXML_LIST_TAG			"list"
#define UIXML_TEXT_TAG			"text"
#define UIXML_TEXTCONTROL_TAG	"textcontrol"
#define UIXML_IMECONTROL_TAG	"imecontrol"
#define UIXML_POPUP_TAG			"popup"
#define UIXML_MENUITEM_TAG		"menuitem"
#define UIXML_INCLUDE_TAG		"include"
#define UIXML_SCROLLBAR_TAG		"scrollbar"
#define UIXML_RESIZEBOX_TAG		"resizebox"
#define UIXML_ROW_TAG			"row"
#define UIXML_SLIDER_TAG		"slider"
#define UIXML_TOOLTIP_TAG		"tooltip"
#define UIXML_WINDOW_TAG		"window"
#define UIXML_CONTEXT_TAG		"contextmenu"
#define UIXML_CTXMENU_TAG		"ctxmenu"
#define UIXML_RADIOBUTTON_TAG	"radiobutton"
#define UIXML_RADIOGROUP_TAG	"radiogroup"
#define UIXML_PANEL_TAG			"panel"
#define UIXML_CONSOLE_TAG		"console"
#define UIXML_MIRROR_TAG		"mirror"

#define UIXML_CLASS_FIELD		"class"
#define UIXML_ID_FIELD			"id"
#define UIXML_WIDTH_FIELD		"width"
#define UIXML_HEIGHT_FIELD		"height"
#define UIXML_X_FIELD			"x"
#define UIXML_Y_FIELD			"y"
#define UIXML_ACCEPTFOCUS_FIELD	"acceptsfocus"
#define UIXML_ENABLE_FIELD 		"enabled"
#define UIXML_SRC_FIELD			"src"
#define UIXML_STYLE_FIELD		"style"
#define UIXML_FRAME_FIELD    	"frame"
#define UIXML_VIEWPORTHEIGHT_FIELD    	"viewportheight"
#define UIXML_VIEWPORTWIDTH_FIELD    	"viewportwidth"
#define UIXML_ASPECTSCALE_FIELD    	"aspectscale"
#define UIXML_ASPECTLIMIT_FIELD    	"aspectlimit"
#define UIXML_ASPECTALIGN_FIELD    	"aspectalign"
#define UIXML_VALUE_FIELD			"value"
#define UIXML_LABEL_FIELD			"label"
#define UIXML_ALIGN_FIELD			"align"
#define UIXML_BUTTON_TAG			"button"
#define UIXML_CHECKED_FIELD 		"checked"
#define UIXML_TOGGLE_FIELD 			"toggle"
#define UIXML_TOP_FIELD 			"top"
#define UIXML_LEFT_FIELD 			"left"
#define UIXML_BOTTOM_FIELD 			"bottom"
#define UIXML_RIGHT_FIELD 			"right"
#define UIXML_MINVALUE_FIELD 		"minvalue"
#define UIXML_MAXVALUE_FIELD 		"maxvalue"
#define UIXML_FILL_FIELD 			"fill"
#define UIXML_BACKGROUND_FIELD 		"background"
#define UIXML_ORIENT_FIELD			"orient"
#define UIXML_FOLLOWTOP_FIELD		"followtop"
#define UIXML_FOLLOWLEFT_FIELD		"followleft"
#define UIXML_FOLLOWBOTTOM_FIELD	"followbottom"
#define UIXML_FOLLOWRIGHT_FIELD		"followright"
#define UIXML_FOLLOWALL_FIELD		"followall"
#define UIXML_FOLLOWFONTWIDTH_FIELD		"followfontwidth"
#define UIXML_FOLLOWFONTHEIGHT_FIELD	"followfontheight"
#define UIXML_FOLLOWFONT_FIELD		"followfont"
#define UIXML_LAYER_FIELD			"layer"
#define UIXML_MOVEONDRAG_FIELD		"moveondrag"
#define UIXML_FILE_FIELD			"file"
#define UIXML_RLEFT_FIELD			"rleft"
#define UIXML_RRIGHT_FIELD			"rright"
#define UIXML_RBOTTOM_FIELD			"rbottom"
#define UIXML_RTOP_FIELD			"rtop"
#define UIXML_ACCEPT_MOUSECLICK		"acceptmouseclick"
#define UIXML_MINBARSIZE_FIELD 		"minbarsize" // scrollbars
#define UIXML_TOOLTIP_FIELD 		"tip"
#define UIXML_SHOWTAB_FIELD 		"showtab"
#define UIXML_HINDENT_FIELD 		"hindent"
#define UIXML_VINDENT_FIELD 		"vindent"
#define UIXML_MAXLENGTH_FIELD 		"maxlength"
#define UIXML_MAXWIDTH_FIELD 		"maxwidth"
#define UIXML_MAXHEIGHT_FIELD 		"maxheight"
#define UIXML_HISTORY_FIELD 		"history"
#define UIXML_FONTSHADOWX_FIELD 	"fontshadowx" // for text drawing views
#define UIXML_FONTSHADOWY_FIELD 	"fontshadowy" // for text drawing views
#define UIXML_MODAL_FIELD 			"modal"			// for cursor show/hide

#define UIXML_UNINITIALIZED_SIZE	-10000
#define UIXML_ASPECT(s,x,y) ((s) ? (x / y) : 1.0f)
#define UIXML_VIEWPORT_WIDTH(fullscreen) ((fullscreen || _viewportwidth <= _maxviewportwidth) ? _viewportwidth : _maxviewportwidth)
#define UIXML_VIEWPORT_OFFSET ((_viewportwidth <= _maxviewportwidth) ? 0 : _maxviewportwidth)
#define UIXML_VIEWPORT_HEIGHT _viewportheight

typedef std::map<std::string, ViewFromUIXML> ViewCreateMap;

static IOCallbacks _io = phui::fdIOCallbacks;
static UIXMLTextureCallback _gettexture = nullptr;
static UIXMLCreateTextureCallback _createtexture = nullptr;
static UIXMLDestroyTextureCallback _destroytexture = nullptr;
static UIXMLClipboardCallback _clipboard = nullptr;
static UIXMLLocalizationCallbackA _localizerA = nullptr;
static UIXMLLocalizationCallbackW _localizerW = nullptr;
static ViewCreateMap _viewcreatemap;
static CSSViewDrawerFactory* _drawerfactory;

static float _maxviewportwidth = 1024;
static float _viewportwidth = 1024, _viewportheight = 768;
static float _uixmlviewportwidth, _uixmlviewportheight;

static View* traverse_tree(xmlNodePtr, View*);
static View* parse_node(xmlNodePtr, View*);
static View* create_view(xmlNodePtr, View*);
static View* create_image(xmlNodePtr, View*);
static View* create_stencil(xmlNodePtr, View*);
static View* process_class_field(xmlNodePtr node, View* parent);
static void process_view_fields(View* view, View* parent, xmlNodePtr node);
static void register_phuiclasses();
static void process_style_field(const char* style, View* v);
static void add_script_handler_msg(View*, xmlNodePtr);
static View* script_tag(xmlNodePtr node, View* parent);
static View* create_caption(xmlNodePtr node, View* parent);
static View* create_tabview(xmlNodePtr node, View* parent);
static View* create_tab(xmlNodePtr node, View* parent);
static View* create_button(xmlNodePtr, View*);
static void process_control_fields(Control* ctl, xmlNodePtr);
static View* create_checkbox(xmlNodePtr, View*);
static View* create_progressbar(xmlNodePtr, View*);
static View* create_viewport(xmlNodePtr, View*);
static View* create_stackview(xmlNodePtr node, View* parent);
static View* create_listview(xmlNodePtr node, View* parent);
static View* create_textview(xmlNodePtr node, View* parent);
static View* create_textcontrol(xmlNodePtr node, View* parent);
static View* create_imecontrol(xmlNodePtr node, View* parent);
static View* menu_tags(xmlNodePtr, View*);
static View* process_include_tag(xmlNodePtr, View*);
static View* create_scrollbar(xmlNodePtr, View*);
//static View* create_resizebox(xmlNodePtr, View*);
static View* create_row(xmlNodePtr, View*);
static View* create_slider(xmlNodePtr, View*);
static View* create_tooltip(xmlNodePtr node, View* parent);
static View* create_window(xmlNodePtr node, View* parent);
static View* create_contextmenu(xmlNodePtr node, View* parent);
static View* create_ctxmenu(xmlNodePtr node, View* parent);
static View* create_radiobutton(xmlNodePtr node, View* parent);
static View* create_radiogroup(xmlNodePtr node, View* parent);
static View* create_panel(xmlNodePtr node, View* parent);
static View* create_console(xmlNodePtr node, View* parent);
static View* create_mirror(xmlNodePtr, View*);

static int xmlinputmatch(char const *filename){ return 1;}

const wchar_t* phui::UIXMLLocalize(const wchar_t* text)
{
	if(!text)
		return L"";

	if(!_localizerW)
		return text;
	
	return _localizerW(text);
}

const wchar_t* phui::UIXMLLocalize(const char* text)
{
	if(!text)
		return L"";

	if(!_localizerA)
		return utf8_to_utf16((const unsigned char*)text);
	
	return _localizerA(text);
}

/************************************************************************************************
	SetUIXMLSetViewportSize
*************************************************************************************************/

void phui::SetUIXMLSetViewportSize(float width, float height, float xwRatio)
{
	_viewportwidth			= width;
	_viewportheight			= height;
	_maxviewportwidth		= (((width / height) >= xwRatio)) ? width/3.0f: _viewportwidth;
}

/************************************************************************************************
	RegisterTagWithXUL
*************************************************************************************************/

void phui::RegisterTagWithUIXML(const char* tag, ViewFromUIXML vx)
{
	_viewcreatemap[tag] = vx;
}

/************************************************************************************************
	CreateViewFromUIXML
*************************************************************************************************/

View* phui::CreateViewFromUIXML(const char* classname, _xmlNode* node, View* parent)
{
	View* v = 0;
	ViewFromUIXML create = _viewcreatemap[classname];
	
	if(create)
		v = create(node, parent);
	
	return v;
}

/************************************************************************************************
	SetUIXMLIOCallbacks
*************************************************************************************************/

void phui::SetUIXMLIOCallbacks(IOCallbacks const* io)
{
	_io = *io;
}

/************************************************************************************************
	SetUIXMLTextureCallback
*************************************************************************************************/

void phui::SetUIXMLTextureCallback(UIXMLTextureCallback gettexture)
{
	_gettexture = gettexture;
}

/************************************************************************************************
	SetUIXMLCreateTextureCallback
*************************************************************************************************/

void phui::SetUIXMLCreateTextureCallback(UIXMLCreateTextureCallback createtexture)
{
	_createtexture = createtexture;
}

/************************************************************************************************
	SetUIXMLDestroyTextureCallback
*************************************************************************************************/

void phui::SetUIXMLDestroyTextureCallback(UIXMLDestroyTextureCallback destroytexture)
{
	_destroytexture = destroytexture;
}

/************************************************************************************************
	SetUIXMLDestroyTextureCallback
*************************************************************************************************/

void phui::SetUIXMLClipboardCallback(UIXMLClipboardCallback clipboard)
{
	_clipboard = clipboard;
}

/************************************************************************************************
	SetUIXMLLocalizationCallback
*************************************************************************************************/

void phui::SetUIXMLLocalizationCallback(UIXMLLocalizationCallbackA localizer)
{
	_localizerA = localizer;
}

void phui::SetUIXMLLocalizationCallback(UIXMLLocalizationCallbackW localizer)
{
	_localizerW = localizer;
}

/************************************************************************************************
	SetUIXMLDrawerFactory
*************************************************************************************************/

void phui::SetUIXMLDrawerFactory(CSSViewDrawerFactory* f)
{
	_drawerfactory = f;
}

/************************************************************************************************
	ScaleUIXMLValue
*************************************************************************************************/

float phui::ScaleUIXMLValue(int type, float val, unsigned int options, float scaler, float limiter)
{
	if(type == View::SCALE_WIDTH || type == View::SCALE_X)
	{
		if((options & View::ASPECT_LIMIT_WIDTH) > 0)
		{
			return val * limiter;
		}
		else if((options & View::ASPECT_SCALE_WIDTH) > 0)
		{
			return val * scaler;
		}
	}
	else if(type == View::SCALE_HEIGHT || type == View::SCALE_Y)
	{
		if((options & View::ASPECT_LIMIT_HEIGHT) > 0)
		{
			return val * limiter;
		}
		else if((options & View::ASPECT_SCALE_HEIGHT) > 0)
		{
			return val * scaler;
		}
	}
	
	return val;
}

/************************************************************************************************
	NormUIXMLValue
*************************************************************************************************/

float phui::NormUIXMLValue(int type, float val, unsigned int options, float scaler, float limiter)
{
	if(type == View::SCALE_WIDTH || type == View::SCALE_X)
	{
		if((options & View::ASPECT_LIMIT_WIDTH) > 0)
		{
			return val / limiter;
		}
		else if((options & View::ASPECT_SCALE_WIDTH) > 0)
		{
			return val / scaler;
		}
	}
	else if(type == View::SCALE_HEIGHT || type == View::SCALE_Y)
	{
		if((options & View::ASPECT_LIMIT_HEIGHT) > 0)
		{
			return val / limiter;
		}
		else if((options & View::ASPECT_SCALE_HEIGHT) > 0)
		{
			return val / scaler;
		}
	}
	
	return val;
}

/************************************************************************************************
	LoadUIXML
*************************************************************************************************/

View* phui::LoadUIXML(const char* name, IOCallbacks const *io, View* parent)
{
	register_phuiclasses();
	
	_uixmlviewportwidth =  1024; _uixmlviewportheight = 768; // default values for reference viewport size
	
	if(io)
	{
		xmlRegisterInputCallbacks(xmlinputmatch, 
								(xmlInputOpenCallback)io->openread, 
								(xmlInputReadCallback)io->read, 
								(xmlInputCloseCallback)io->close); 
	}
	else
	{
		xmlRegisterInputCallbacks(xmlinputmatch, 
								(xmlInputOpenCallback)_io.openread, 
								(xmlInputReadCallback)_io.read, 
								(xmlInputCloseCallback)_io.close); 
	}

	xmlDocPtr doc = xmlReadFile(name, nullptr, XML_PARSE_DTDLOAD|XML_PARSE_RECOVER);
	View* v = 0;
	if(doc)
	{
		xmlNodePtr node = xmlDocGetRootElement(doc);
		v = traverse_tree(node, parent);

		// set this as the root of this loading group
		if(v) v->SetSource(name);
	
		xmlFreeDoc(doc);
	}	
	xmlCleanupInputCallbacks();

	return v;
}

GraphicsContext::TextureInfo phui::UIXMLCreateTexture(int width, int height, int bpp)
{
	if(_createtexture)
		return _createtexture(width, height, bpp);
	
	return GraphicsContext::TextureInfo();
}

void phui::UIXMLDestroyTexture(int texid)
{
	if(_destroytexture)
		_destroytexture(texid);
}

const wchar_t* phui::UIXMLClipboard(const wchar_t* c)
{
	if(_clipboard)
		return _clipboard(c);

	return nullptr;
}

/************************************************************************************************
	traverse_tree
*************************************************************************************************/

static View* traverse_tree(xmlNodePtr node, View* parent)
{
	View* v = 0;
	for(xmlNodePtr cur = node; cur; cur = cur->next) 
	{
		if(cur->type != XML_ELEMENT_NODE) continue;
		
		v = parse_node(cur, parent);
		
		if(v)
		{
			if(parent)
				parent->AddChild(v);
		}
		
		if(cur->children)
		{
			traverse_tree(cur->children, v ? v : parent);
		}
	}
	
	return v;
}

/************************************************************************************************
	parse_node
*************************************************************************************************/

static View* parse_node(xmlNodePtr node, View* parent)
{
	View* v = CreateViewFromUIXML((char*)node->name, node, parent);

	if(v)
		add_script_handler_msg(v, node);

	return v;
}


/************************************************************************************************
	create_view
*************************************************************************************************/

static View* create_view(xmlNodePtr node, View* parent)
{
	View* v = process_class_field(node, parent);
	if(!v)
		v = new phui::View;
	
	if(v)
		process_view_fields(v, parent, node);
	
	return v;
}

/************************************************************************************************
	create_image
*************************************************************************************************/

static View* create_image(xmlNodePtr node, View* parent)
{
	View* v = process_class_field(node, parent);
	if(!v)
		v = new phui::ImageView;
	
	if(v)
	{
		process_view_fields(v, parent, node);

		ImageView* iv = dynamic_cast<ImageView*>(v);
		
		if(iv)
		{
			xmlChar* val;
			val = xmlGetProp(node, (xmlChar*)UIXML_SRC_FIELD);
			if(val)
			{
				GraphicsContext::TextureInfo texid;
				if(_gettexture)
					texid = _gettexture((char*)val,false);
					
				iv->SetTexture(texid);
				xmlFree(val);
			}
		}
	}
	
	return v;
}

/************************************************************************************************
	create_mirror
*************************************************************************************************/

static View* create_mirror(xmlNodePtr node, View* parent)
{
	View* v = process_class_field(node, parent);
	if(!v)
		v = new phui::MirrorView;
	
	if(v)
		process_view_fields(v, parent, node);
	
	return v;
}


/************************************************************************************************
	create_stencil
*************************************************************************************************/

static View* create_stencil(xmlNodePtr node, View* parent)
{
	View* v = process_class_field(node, parent);
	if(!v)
		v = new phui::StencilView;
	
	if(v)
	{
		process_view_fields(v, parent, node);

		StencilView* iv = dynamic_cast<StencilView*>(v);
		
		if(iv)
		{
			xmlChar* val;
			val = xmlGetProp(node, (xmlChar*)UIXML_SRC_FIELD);
			if(val)
			{
				GraphicsContext::TextureInfo  texid;
				if(_gettexture)
					texid = _gettexture((char*)val,false);
					
				iv->SetTexture(texid);
				xmlFree(val);
			}
		}
	}
	
	return v;
}

/************************************************************************************************
	process_view_fields
*************************************************************************************************/

static void process_view_fields(View* view, View* parent, xmlNodePtr node)
{
	if(_drawerfactory)
		view->AttachDrawer(_drawerfactory);

	float w ,h, ow, oh, mw, mh;
	float top, left, bottom, right;

	float wscaler, hscaler;
	float wlimiter, hlimiter;
	wscaler = hscaler = wlimiter = hlimiter = 1.0f;

	mw = mh = 10000;
	ow = oh = w = h = top = left = bottom = right  = UIXML_UNINITIALIZED_SIZE;

	unsigned int aspectmode	= View::ASPECT_SCALE;
	unsigned int aspectalign	= View::ASPECT_NONE;
	
	if(parent)
	{
		aspectmode = parent->AspectMode();
		parent->GetScalers(wscaler, hscaler, wlimiter, hlimiter);
	}
	
	// strip out the aspect mode flags since that is per view
	// other flag types carry over from the parent
	//aspectmode &= ~View::ASPECT_ALIGN;
	
	xmlChar* val = nullptr;
	
	// viewport width, height, needs to come first
	val = xmlGetProp(node, (xmlChar*)UIXML_VIEWPORTWIDTH_FIELD);
	if(val)
	{
		_uixmlviewportwidth = (float)atof((char*)val);
		xmlFree(val);
	}
	
	val = xmlGetProp(node, (xmlChar*)UIXML_VIEWPORTHEIGHT_FIELD);
	if(val)
	{
		_uixmlviewportheight = (float)atof((char*)val);
		xmlFree(val);
	}
	
	val = xmlGetProp(node, (xmlChar*)UIXML_ASPECTALIGN_FIELD);
	if(val)
	{
		if(xmlStrcmp(val, (xmlChar*)"left") == 0)
			aspectalign |= View::ASPECT_ALIGN_LEFT;
		else if(xmlStrcmp(val, (xmlChar*)"center") == 0)
			aspectalign |= View::ASPECT_ALIGN_CENTER;
		else if(xmlStrcmp(val, (xmlChar*)"right") == 0)
			aspectalign |= View::ASPECT_ALIGN_RIGHT;
		
		xmlFree(val);
	}

	// aspect scaling
	val = xmlGetProp(node, (xmlChar*)UIXML_ASPECTSCALE_FIELD);
	if(val)
	{
		if(xmlStrcmp(val, (xmlChar*)"width") == 0)
		{
			aspectmode |= View::ASPECT_SCALE_WIDTH;
			aspectmode &= ~View::ASPECT_SCALE_HEIGHT;
		}
		else if(xmlStrcmp(val, (xmlChar*)"height") == 0)
		{
			aspectmode &= ~View::ASPECT_SCALE_WIDTH;
			aspectmode |= View::ASPECT_SCALE_HEIGHT;
		}
		else if(UIXMLbool(val))
			aspectmode |= View::ASPECT_SCALE;
		else
			aspectmode &= ~View::ASPECT_SCALE;	
		
		xmlFree(val);
	}		

	// aspect limiting
	val = xmlGetProp(node, (xmlChar*)UIXML_ASPECTLIMIT_FIELD);
	if(val)
	{
		if(xmlStrcmp(val, (xmlChar*)"width") == 0)
		{
			aspectmode |= View::ASPECT_LIMIT_WIDTH;
			aspectmode &= ~View::ASPECT_LIMIT_HEIGHT;
		}
		else if(xmlStrcmp(val, (xmlChar*)"height") == 0)
		{
			aspectmode |= View::ASPECT_LIMIT_HEIGHT;
			aspectmode &= ~View::ASPECT_LIMIT_WIDTH;
		}
		else if(UIXMLbool(val))
			aspectmode |= View::ASPECT_LIMIT;
		else
			aspectmode &= ~View::ASPECT_LIMIT;	
		
		xmlFree(val);
	}

	// are we the one who specified an alignment?
	bool fullScreen = (aspectalign & View::ASPECT_ALIGN) == 0;

	// if so, set our offset
	if(!fullScreen)
	{
		if((aspectalign & View::ASPECT_ALIGN_CENTER) > 0 )
			view->SetAspectOffset(UIXML_VIEWPORT_OFFSET);
		else if((aspectalign & View::ASPECT_ALIGN_RIGHT) > 0 )
			view->SetAspectOffset(UIXML_VIEWPORT_OFFSET * 2.0f);
	}
	
	// combine alignment flags into our parent's and ours
	aspectmode |= aspectalign;
	
	// recalculate fullscreen taking all flags into account
	fullScreen = (aspectmode & View::ASPECT_ALIGN) == 0;
	
	wscaler = UIXML_VIEWPORT_WIDTH(fullScreen) / _uixmlviewportwidth;
	hscaler = UIXML_VIEWPORT_HEIGHT / _uixmlviewportheight;
	
	// id
	val = xmlGetProp(node, (xmlChar*)UIXML_ID_FIELD);
	if(val)
	{
		view->SetName((char*)val);
		xmlFree(val);
	}
	
	// enabled
	val = xmlGetProp(node, (xmlChar*)UIXML_ENABLE_FIELD);
	if(val)
	{
		view->SetEnable(UIXMLbool(val));
		xmlFree(val);
	}

	/////////////////// dimensions ///////////////////
	
	// width
	val = xmlGetProp(node, (xmlChar*)UIXML_WIDTH_FIELD);
	if(val)
	{
		ow = w = (float)atof((char*)val); // * _viewportwidth / _uixmlviewportwidth;
		xmlFree(val);
	}

	// height
	val = xmlGetProp(node, (xmlChar*)UIXML_HEIGHT_FIELD);
	if(val)
	{
		oh = h = (float)atof((char*)val); // * _viewportheight / _uixmlviewportheight;
		xmlFree(val);
	}

	// scale it to the viewport
	w = ScaleUIXMLValue(View::SCALE_WIDTH, ow, View::ASPECT_SCALE, wscaler);
	h = ScaleUIXMLValue(View::SCALE_HEIGHT, oh, View::ASPECT_SCALE, hscaler);

	hlimiter = (ow) ? w / ow :  1.0f;
	wlimiter = (oh) ? h / oh :  1.0f;

	// setup scaling/limiting vars
	view->SetAspectMode(aspectmode);
	view->SetScalers(wscaler, hscaler, wlimiter, hlimiter);
	
	// now allow limiting to be donw
	w = view->ScaleValue(View::SCALE_WIDTH, ow);
	h = view->ScaleValue(View::SCALE_HEIGHT, oh);
	
	// maxwidth
	val = xmlGetProp(node, (xmlChar*)UIXML_MAXWIDTH_FIELD);
	if(val)
	{
		mw = view->ScaleValue(View::SCALE_WIDTH, (float)atof((char*)val));
		xmlFree(val);
	}

	// maxheight
	val = xmlGetProp(node, (xmlChar*)UIXML_MAXHEIGHT_FIELD);
	if(val)
	{
		mh = view->ScaleValue(View::SCALE_HEIGHT, (float)atof((char*)val));
		xmlFree(val);
	}

	//view->SetAspectMode(aspectmode);
	view->SetMaxSize(mw, mh);
	view->ResizeTo(w,h);
	view->SetOrigSize(ow, oh);
	
	/////////////////// position ///////////////////

	// x
	val = xmlGetProp(node, (xmlChar*)UIXML_X_FIELD);
	if(val)
	{
		left = view->ScaleValue(View::SCALE_X, (float)atof((char*)val));
		xmlFree(val);
	}

	// y
	val = xmlGetProp(node, (xmlChar*)UIXML_Y_FIELD);
	if(val)
	{
		bottom = view->ScaleValue(View::SCALE_Y, (float)atof((char*)val));
		xmlFree(val);
	}

	// top
	val = xmlGetProp(node, (xmlChar*)UIXML_TOP_FIELD);
	if(val)
	{
		top = view->ScaleValue(View::SCALE_Y, (float)atof((char*)val));
		xmlFree(val);
	}

	// left
	val = xmlGetProp(node, (xmlChar*)UIXML_LEFT_FIELD);
	if(val)
	{
		left = view->ScaleValue(View::SCALE_X, (float)atof((char*)val));
		xmlFree(val);
	}

	// bottom
	val = xmlGetProp(node, (xmlChar*)UIXML_BOTTOM_FIELD);
	if(val)
	{
		bottom = view->ScaleValue(View::SCALE_Y, (float)atof((char*)val));
		xmlFree(val);
	}

	// right
	val = xmlGetProp(node, (xmlChar*)UIXML_RIGHT_FIELD);
	if(val)
	{
		right = view->ScaleValue(View::SCALE_X, (float)atof((char*)val));
		xmlFree(val);
	}
	
	// rleft
	val = xmlGetProp(node, (xmlChar*)UIXML_RLEFT_FIELD);
	if(val)
	{
		left = view->ScaleValue(View::SCALE_X, (float)atof((char*)val));
		xmlFree(val);
	}

	// rbottom
	val = xmlGetProp(node, (xmlChar*)UIXML_RBOTTOM_FIELD);
	if(val)
	{
		bottom = view->ScaleValue(View::SCALE_Y, (float)atof((char*)val));
		xmlFree(val);
	}

	// rright
	val = xmlGetProp(node, (xmlChar*)UIXML_RRIGHT_FIELD);
	if(val)
	{
		right = parent->Width() - view->ScaleValue(View::SCALE_X, (float)atof((char*)val));
		xmlFree(val);
	}

	// rtop
	val = xmlGetProp(node, (xmlChar*)UIXML_RTOP_FIELD);
	if(val)
	{
		top = parent->Height() - view->ScaleValue(View::SCALE_Y, (float)atof((char*)val));
		xmlFree(val);
	}
	
	if(top != UIXML_UNINITIALIZED_SIZE && bottom != UIXML_UNINITIALIZED_SIZE)
		h = top - bottom;
	else if (top != UIXML_UNINITIALIZED_SIZE)
		bottom = top - h;
	else if (bottom != UIXML_UNINITIALIZED_SIZE)
		top = bottom + h;
	else
		bottom = (parent) ? (parent->Height()/2.0f) - (h/2.0f): 0;
		
	if(left != UIXML_UNINITIALIZED_SIZE && right != UIXML_UNINITIALIZED_SIZE)
		w = right - left;
	else if(left != UIXML_UNINITIALIZED_SIZE)
		right = left + w;
	else if(right != UIXML_UNINITIALIZED_SIZE)
		left = right - w;
	else
		left = (parent) ? (parent->Width()/2.0f) - (w/2.0f): 0;
	
	view->MoveTo(left, bottom);
	view->ResizeTo(w, h);
	
	// acceptsfocus
	val = xmlGetProp(node, (xmlChar*)UIXML_ACCEPTFOCUS_FIELD);
	if(val)
	{
		view->SetAcceptsFocus(UIXMLbool(val));
		xmlFree(val);
	}			

	/////////////////// resize options ///////////////////
	
	unsigned int resizemode = view->ResizeMode();

	// followtop
	val = xmlGetProp(node, (xmlChar*)UIXML_FOLLOWTOP_FIELD);
	if(val)
	{
		if(UIXMLbool(val))
			resizemode |= View::FOLLOW_TOP;
		else
			resizemode &= ~View::FOLLOW_TOP;		
		xmlFree(val);
	}			

	// followleft
	val = xmlGetProp(node, (xmlChar*)UIXML_FOLLOWLEFT_FIELD);
	if(val)
	{
		if(UIXMLbool(val))
			resizemode |= View::FOLLOW_LEFT;
		else
			resizemode &= ~View::FOLLOW_LEFT;		
		xmlFree(val);
	}

	// followbottom
	val = xmlGetProp(node, (xmlChar*)UIXML_FOLLOWBOTTOM_FIELD);
	if(val)
	{
		if(UIXMLbool(val))
			resizemode |= View::FOLLOW_BOTTOM;
		else
			resizemode &= ~View::FOLLOW_BOTTOM;		
		xmlFree(val);
	}
	
	// followright
	val = xmlGetProp(node, (xmlChar*)UIXML_FOLLOWRIGHT_FIELD);
	if(val)
	{
		if(UIXMLbool(val))
			resizemode |= View::FOLLOW_RIGHT;
		else
			resizemode &= ~View::FOLLOW_RIGHT;		
		xmlFree(val);
	}

	// followall
	val = xmlGetProp(node, (xmlChar*)UIXML_FOLLOWALL_FIELD);
	if(val)
	{
		if(UIXMLbool(val))
			resizemode |= View::FOLLOW_ALL_SIDES;
		else
			resizemode &= ~View::FOLLOW_ALL_SIDES;		
		xmlFree(val);
	}

	// followfontwidth
	val = xmlGetProp(node, (xmlChar*)UIXML_FOLLOWFONTWIDTH_FIELD);
	if(val)
	{
		if(UIXMLbool(val))
			resizemode |= View::FOLLOW_FONT_WIDTH;
		else
			resizemode &= ~View::FOLLOW_FONT_WIDTH;		
		xmlFree(val);
	}	

	// followfontheight
	val = xmlGetProp(node, (xmlChar*)UIXML_FOLLOWFONTHEIGHT_FIELD);
	if(val)
	{
		if(UIXMLbool(val))
			resizemode |= View::FOLLOW_FONT_HEIGHT;
		else
			resizemode &= ~View::FOLLOW_FONT_HEIGHT;		
		xmlFree(val);
	}	

	// followfont
	val = xmlGetProp(node, (xmlChar*)UIXML_FOLLOWFONT_FIELD);
	if(val)
	{
		if(UIXMLbool(val))
			resizemode |= View::FOLLOW_FONT;
		else
			resizemode &= ~View::FOLLOW_FONT;
		xmlFree(val);
	}	
	
	view->SetResizeMode(resizemode);

	// layer
	val = xmlGetProp(node, (xmlChar*)UIXML_LAYER_FIELD);
	if(val)
	{
		view->SetLayer(atoi((char*)val));
		xmlFree(val);
	}

	// move on drag
	val = xmlGetProp(node, (xmlChar*)UIXML_MOVEONDRAG_FIELD);
	if(val)
	{
		view->SetMoveOnDrag(UIXMLbool(val));
		xmlFree(val);
	}
				
	// style
	val = xmlGetProp(node, (xmlChar*)UIXML_STYLE_FIELD);
	if(val)
	{
		process_style_field((char*)val, view);
		xmlFree(val);
	}
	
	// accept mouse click
	val = xmlGetProp(node, (xmlChar*)UIXML_ACCEPT_MOUSECLICK);
	if(val)
	{
		view->SetAcceptsMouseClick(UIXMLbool(val));
		xmlFree(val);
	}

	// tooltip
	val = xmlGetProp(node, (xmlChar*)UIXML_TOOLTIP_FIELD);
	if(val)
	{
		view->SetToolTip(phui::UIXMLLocalize((const char*)val));
		xmlFree(val);
	}

	// modal
	val = xmlGetProp(node, (xmlChar*)UIXML_MODAL_FIELD);
	if(val)
	{
		view->SetIsModal(UIXMLbool(val));
		xmlFree(val);
	}
}

/************************************************************************************************
	process_class_field
*************************************************************************************************/

static View* process_class_field(xmlNodePtr node, View* parent)
{
	xmlChar* val;
	View* v = nullptr;
	
	val = xmlGetProp(node, (xmlChar*)UIXML_CLASS_FIELD);
	if(val) 
	{
		v = CreateViewFromUIXML((char*)val, node, parent);
	
		if(v)
			v->UserMessage()->AddString(DOM_CLASS, (char*)val);
		
		xmlFree(val);
	}
	
	return v;
}

/************************************************************************************************
	register_phuiclasses
*************************************************************************************************/

static void register_phuiclasses()
{
	static bool inited;
	if(inited) return;

	RegisterTagWithUIXML(UIXML_VIEW_TAG, create_view);
	RegisterTagWithUIXML(UIXML_IMAGE_TAG, create_image);
	RegisterTagWithUIXML(UIXML_STENCIL_TAG, create_stencil);
	RegisterTagWithUIXML(UIXML_SCRIPT_TAG, script_tag);
	RegisterTagWithUIXML(UIXML_CAPTION_TAG, create_caption);
	RegisterTagWithUIXML(UIXML_TABVIEW_TAG, create_tabview);
	RegisterTagWithUIXML(UIXML_TAB_TAG, create_tab);
	RegisterTagWithUIXML(UIXML_BUTTON_TAG, create_button);
	RegisterTagWithUIXML(UIXML_CHECKBOX_TAG, create_checkbox);
	RegisterTagWithUIXML(UIXML_PROGRESSBAR_TAG, create_progressbar);
	RegisterTagWithUIXML(UIXML_VIEWPORT_TAG, create_viewport);
	RegisterTagWithUIXML(UIXML_STACKVIEW_TAG, create_stackview);
	RegisterTagWithUIXML(UIXML_LIST_TAG, create_listview);
	RegisterTagWithUIXML(UIXML_TEXT_TAG, create_textview);
	RegisterTagWithUIXML(UIXML_TEXTCONTROL_TAG, create_textcontrol);
	RegisterTagWithUIXML(UIXML_IMECONTROL_TAG, create_imecontrol);
	RegisterTagWithUIXML(UIXML_POPUP_TAG, menu_tags);
	RegisterTagWithUIXML(UIXML_MENUITEM_TAG, menu_tags);
	RegisterTagWithUIXML(UIXML_INCLUDE_TAG, process_include_tag);
	RegisterTagWithUIXML(UIXML_SCROLLBAR_TAG, create_scrollbar);
//	RegisterTagWithUIXML(UIXML_RESIZEBOX_TAG, create_resizebox);
	RegisterTagWithUIXML(UIXML_ROW_TAG, create_row);
	RegisterTagWithUIXML(UIXML_SLIDER_TAG, create_slider);
	RegisterTagWithUIXML(UIXML_TOOLTIP_TAG, create_tooltip);
	RegisterTagWithUIXML(UIXML_WINDOW_TAG, create_window);
	RegisterTagWithUIXML(UIXML_CONTEXT_TAG, create_contextmenu);
	RegisterTagWithUIXML(UIXML_CTXMENU_TAG, create_ctxmenu);
	RegisterTagWithUIXML(UIXML_RADIOBUTTON_TAG, create_radiobutton);
	RegisterTagWithUIXML(UIXML_RADIOGROUP_TAG, create_radiogroup);
	RegisterTagWithUIXML(UIXML_PANEL_TAG, create_panel);
	RegisterTagWithUIXML(UIXML_CONSOLE_TAG, create_console);
	RegisterTagWithUIXML(UIXML_MIRROR_TAG, create_mirror);
	
	inited = true;
}

/************************************************************************************************
	UIXMLbool
*************************************************************************************************/

bool phui::UIXMLbool(const xmlChar* val)
{
	if(xmlStrcmp(val, (xmlChar*)"false") == 0)
		return false;

	if(xmlStrcmp(val, (xmlChar*)"no") == 0)
		return false;

	if(xmlStrcmp(val, (xmlChar*)"0") == 0)
		return false;
		
	return true;
}

/************************************************************************************************
	process_style_field
*************************************************************************************************/

static void process_style_field(const char* style, View* v)
{
	if(!_drawerfactory) return;
	
	CSSViewDrawer* drawer = dynamic_cast<CSSViewDrawer*>(v->GetDrawer());
	if(!drawer) return;

	const char* s = style;
	
	while(*s)
	{
		std::string key,val;
		
		// key
		for(; *s && *s == ' '; s++) { }
		for(; *s && (!isspace(*s) && *s != ':'); s++) 
		{ 
			key += *s;
		}
		if(*s) s++;
		
		// val
		for(; *s && *s == ' '; s++) { }
		for(; *s && (!isspace(*s) && *s != ';'); s++) 
		{ 
			val += *s;
		}	
		if(*s) s++;

		drawer->SetStyleProperty(key.c_str(), val.c_str());
	}
}

/************************************************************************************************
	add_script_handler_msg
*************************************************************************************************/

static void add_script_handler_msg(View* view, xmlNodePtr node)
{
	Message* msg = view->UserMessage();
	xmlChar* val;
	val = xmlGetProp(node, (xmlChar*)"onclick");
	if(val)
	{
		msg->AddString(DOM_ONCLICK,(char*)val);
		xmlFree(val);
	}
	
	val = xmlGetProp(node, (xmlChar*)"onload");
	if(val)
	{
		msg->AddString(DOM_ONLOAD,(char*)val);
		xmlFree(val);
	}

	val = xmlGetProp(node, (xmlChar*)"onunload");
	if(val)
	{
		msg->AddString(DOM_ONUNLOAD,(char*)val);
		xmlFree(val);
	}

	val = xmlGetProp(node, (xmlChar*)"ondrag");
	if(val)
	{
		msg->AddString(DOM_ONDRAG,(char*)val);
		xmlFree(val);
	}
	
	val = xmlGetProp(node, (xmlChar*)"onshow");
	if(val)
	{
		msg->AddString(DOM_ONSHOW,(char*)val);
		xmlFree(val);
	}

	val = xmlGetProp(node, (xmlChar*)"onhide");
	if(val)
	{
		msg->AddString(DOM_ONHIDE,(char*)val);
		xmlFree(val);
	}

	val = xmlGetProp(node, (xmlChar*)"onupdate");
	if(val)
	{
		msg->AddString(DOM_ONUPDATE,(char*)val);
		xmlFree(val);
	}

	val = xmlGetProp(node, (xmlChar*)"onenter");
	if(val)
	{
		msg->AddString(DOM_ONENTER,(char*)val);
		xmlFree(val);
	}

	val = xmlGetProp(node, (xmlChar*)"onmouseenter");
	if(val)
	{
		msg->AddString(DOM_ONMOUSEENTER,(char*)val);
		xmlFree(val);
	}
	
	val = xmlGetProp(node, (xmlChar*)"onmouseexit");
	if(val)
	{
		msg->AddString(DOM_ONMOUSEEXIT,(char*)val);
		xmlFree(val);
	}

	val = xmlGetProp(node, (xmlChar*)"onmouseover");
	if(val)
	{
		msg->AddString(DOM_ONMOUSEOVER,(char*)val);
		xmlFree(val);
	}

	val = xmlGetProp(node, (xmlChar*)"onselect");
	if(val)
	{
		msg->AddString(DOM_ONSELECT,(char*)val);
		xmlFree(val);
	}

	val = xmlGetProp(node, (xmlChar*)"ondblselect");
	if(val)
	{
		msg->AddString(DOM_ONDBLSELECT,(char*)val);
		xmlFree(val);
	}

	val = xmlGetProp(node, (xmlChar*)"ondblclick");
	if(val)
	{
		msg->AddString(DOM_ONDBLCLICK,(char*)val);
		xmlFree(val);
	}
	
	val = xmlGetProp(node, (xmlChar*)"onmousedown");
	if(val)
	{
		msg->AddString(DOM_ONMOUSEDOWN,(char*)val);
		xmlFree(val);
	}
	
	val = xmlGetProp(node, (xmlChar*)"onrightclick");
	if(val)
	{
		msg->AddString(DOM_ONRIGHTCLICK,(char*)val);
		xmlFree(val);
	}

	val = xmlGetProp(node, (xmlChar*)"ontab");
	if(val)
	{
		TabView* tv = dynamic_cast<TabView*>(view);
		if(tv) msg->AddString(DOM_ONTAB,(char*)val);
		xmlFree(val);
	}
}

/************************************************************************************************
	script_tag
*************************************************************************************************/

static View* script_tag(xmlNodePtr node, View* parent)
{
	for(xmlNodePtr cur = node->children; cur; cur = cur->next) 
	{
		if(cur->type == XML_TEXT_NODE || cur->type == XML_COMMENT_NODE)
		{
			xmlChar* script = xmlNodeGetContent(cur);
			if(script)
			{
				if(parent)
				{
					bool allspace=true;
					for(char* s = (char*)script; *s; s++)
					{
						if(!isspace(*s))
						{
							allspace=false;
							break;
						}
					}	
					if(!allspace)
					{
						bool scriptprivate = false;
						xmlChar* val = xmlGetProp(node, (xmlChar*)"private");
						if(val)
						{
							scriptprivate = UIXMLbool(val);
							xmlFree(val);
						}
						
						parent->UserMessage()->AddString(DOM_SCRIPT, (char*)script);
						parent->UserMessage()->AddBool(DOM_SCRIPT_PRIVATE, scriptprivate);
					}
				}
				xmlFree(script);
			}
		}
	}
	return nullptr;
}


/************************************************************************************************
	create_caption
*************************************************************************************************/

static View* create_caption(xmlNodePtr node, View* parent)
{
	View* v;
	
	v = process_class_field(node, parent);
	if(v == nullptr) v = new Caption;
	
	process_view_fields(v, parent, node);
	
	xmlChar* val;
	
	val = xmlGetProp(node, (xmlChar*)UIXML_VALUE_FIELD);
	if(val) 
	{
		v->SetValue(phui::UIXMLLocalize((const char*)val));		
		xmlFree(val);	
	}

	val = xmlGetProp(node, (xmlChar*)UIXML_LABEL_FIELD);
	if(val) 
	{
		v->SetValue(phui::UIXMLLocalize((const char*)val));		
		xmlFree(val);	
	}	
	
	Caption* caption = dynamic_cast<Caption*>(v);
	if(caption)
	{
		val = xmlGetProp(node, (xmlChar*)UIXML_ALIGN_FIELD);
		if(val) 
		{
			if(!xmlStrcmp(val,(xmlChar*)"center"))
			{
				caption->SetTextAlignment(Caption::ALIGN_CENTER);	
			}
			else if(!xmlStrcmp(val,(xmlChar*)"left"))
			{
				caption->SetTextAlignment(Caption::ALIGN_LEFT);	
			}
			else if(!xmlStrcmp(val,(xmlChar*)"right"))
			{
				caption->SetTextAlignment(Caption::ALIGN_RIGHT);					
			}

			xmlFree(val);	
		}

		if(caption->Width() <= 0 || caption->Height() <= 0)
			caption->SetAutoResize(true);
	}
	
	return v;
}

/************************************************************************************************
	create_tabview
*************************************************************************************************/

static View* create_tabview(xmlNodePtr node, View* parent)
{
	View* v;
	
	v = process_class_field(node, parent);
	if(v == nullptr) v = new TabView;	
	
	process_view_fields(v, parent, node);

	return v;
}

/************************************************************************************************
	create_tab
*************************************************************************************************/

static View* create_tab(xmlNodePtr node, View* parent)
{
	View* v;
	
	v = process_class_field(node, parent);
	if(v == nullptr) v = new View;

	process_view_fields(v, parent, node);

	TabView* tv = dynamic_cast<TabView*>(parent);
	if(tv)
	{
		xmlChar* val;
		TabView::Tab tab;
		
		val = xmlGetProp(node, (xmlChar*)UIXML_ALIGN_FIELD);
		if(val) 
		{
			if(!xmlStrcmp(val,(xmlChar*)"right"))
				tab.alignment = TabView::ALIGN_RIGHT;
			
			xmlFree(val);	
		}
		
		val = xmlGetProp(node, (xmlChar*)UIXML_LABEL_FIELD);
		if(val) 
		{
			tab.name = phui::UIXMLLocalize((const char*)val);
			xmlFree(val);	
		}
		
		// showtab
		val = xmlGetProp(node, (xmlChar*)UIXML_SHOWTAB_FIELD);
		if(val)
		{
			tab.visible = (UIXMLbool(val)) ? true: false;
			xmlFree(val);
		}
		
		tab.tooltip = std::wstring(v->GetToolTip());
		
		// the parent tabview controls the tooltip, so we'll clear it here
		v->SetToolTip(L"");
		tv->AddTab(tab);
	}
	
	return v;
}

/************************************************************************************************
	create_button
*************************************************************************************************/

static View* create_button(xmlNodePtr node, View* parent)
{
	View* v;
	
	v = process_class_field(node, parent);
	if(v == nullptr) v = new Button;
	
	process_view_fields(v, parent, node);
	process_control_fields(dynamic_cast<Control*>(v), node);
	
	xmlChar* val;
	
	val = xmlGetProp(node, (xmlChar*)UIXML_TOGGLE_FIELD);
	if(val) 
	{
		Button* btn = dynamic_cast<Button*>(v);
		if(btn)
			btn->SetToggle(!strcmp((char*)val,"true") ? true : false);
			
		xmlFree(val);	
	}
	
	return v;
}

/************************************************************************************************
	create_radiobutton
*************************************************************************************************/

static View* create_radiobutton(xmlNodePtr node, View* parent)
{
	View* v;
	
	v = process_class_field(node, parent);
	if(v == nullptr) v = new RadioButton;
	
	process_view_fields(v, parent, node);
	process_control_fields(dynamic_cast<Control*>(v), node);

	return v;
}

/************************************************************************************************
	create_radiogroup
*************************************************************************************************/

static View* create_radiogroup(xmlNodePtr node, View* parent)
{
	View* v;
	
	v = process_class_field(node, parent);
	if(v == nullptr) v = new RadioGroup;
	
	process_view_fields(v, parent, node);
	process_control_fields(dynamic_cast<Control*>(v), node);

	return v;
}

/************************************************************************************************
	create_panel
*************************************************************************************************/

static View* create_panel(xmlNodePtr node, View* parent)
{
	View* v;
	
	v = process_class_field(node, parent);
	if(v == nullptr) v = new Panel;
	
	process_view_fields(v, parent, node);

	xmlChar* val;
	
	val = xmlGetProp(node, (xmlChar*)UIXML_VALUE_FIELD);
	if(val) 
	{
		v->SetValue(phui::UIXMLLocalize((const char*)val));		
		xmlFree(val);	
	}

	val = xmlGetProp(node, (xmlChar*)UIXML_LABEL_FIELD);
	if(val) 
	{
		v->SetValue(phui::UIXMLLocalize((const char*)val));		
		xmlFree(val);	
	}	

	val = xmlGetProp(node, (xmlChar*)UIXML_ALIGN_FIELD);
	if(val) 
	{
		Panel* panel = dynamic_cast<Panel*>(v);
		if(panel)
		{	
			if(!xmlStrcmp(val,(xmlChar*)"center"))
			{
				panel->SetTextAlignment(Panel::ALIGN_CENTER);	
			}
			else if(!xmlStrcmp(val,(xmlChar*)"left"))
			{
				panel->SetTextAlignment(Panel::ALIGN_LEFT);	
			}
			else if(!xmlStrcmp(val,(xmlChar*)"right"))
			{
				panel->SetTextAlignment(Panel::ALIGN_RIGHT);					
			}
		}

		xmlFree(val);	
	}

	return v;
}

/************************************************************************************************
	process_control_fields
*************************************************************************************************/

static void process_control_fields(Control* v, xmlNodePtr node)
{
	if(v == nullptr) return;
	
	xmlChar* val;
	
	val = xmlGetProp(node, (xmlChar*)UIXML_LABEL_FIELD);
	if(val) 
	{
		v->SetLabel(phui::UIXMLLocalize((const char*)val));
		xmlFree(val);
	}
	
	val = xmlGetProp(node, (xmlChar*)UIXML_CHECKED_FIELD);
	if(val) 
	{
		v->SetValue(!strcmp((char*)val,"true") ? true : false);		
		xmlFree(val);	
	}
}

/************************************************************************************************
	create_checkbox
*************************************************************************************************/

static View* create_checkbox(xmlNodePtr node, View* parent)
{
	View* v;
	
	v = process_class_field(node, parent);
	if(v == nullptr) v = new Checkbox;
	
	process_view_fields(v, parent, node);
	process_control_fields(dynamic_cast<Control*>(v), node);

	return v;
}

/************************************************************************************************
	create_progressbar
*************************************************************************************************/

static View* create_progressbar(xmlNodePtr node, View* parent)
{
	View* v = process_class_field(node, parent);
	if(!v)
		v = new phui::ProgressBar;
	
	if(v)
	{
		process_view_fields(v, parent, node);

		ProgressBar* pb = dynamic_cast<ProgressBar*>(v);
		
		if(pb)
		{
			xmlChar* val;
			
			val = xmlGetProp(node, (xmlChar*)UIXML_FILL_FIELD);
			if(val)
			{
				if(_gettexture) pb->SetFillTexture(_gettexture((char*)val,false));
				xmlFree(val);
			}

			val = xmlGetProp(node, (xmlChar*)UIXML_BACKGROUND_FIELD);
			if(val)
			{
				if(_gettexture) pb->SetBackgroundTexture(_gettexture((char*)val,false));
				xmlFree(val);
			}
			
			val = xmlGetProp(node, (xmlChar*)UIXML_MINVALUE_FIELD);
			if(val)
			{
				pb->SetMinValue((float)atof((char*)val));
				xmlFree(val);
			}
	
			val = xmlGetProp(node, (xmlChar*)UIXML_MAXVALUE_FIELD);
			if(val)
			{
				pb->SetMaxValue((float)atof((char*)val));
				xmlFree(val);
			}

			val = xmlGetProp(node, (xmlChar*)UIXML_VALUE_FIELD);
			if(val)
			{
				pb->SetValue(atof((char*)val));
				xmlFree(val);
			}
		
			val = xmlGetProp(node, (xmlChar*)UIXML_ORIENT_FIELD);
			if(val)
			{
				if(!xmlStrcmp(val, (xmlChar*)"leftright"))
					pb->SetOrientation(ProgressBar::ORIENT_LEFT_RIGHT);
				else if(!xmlStrcmp(val, (xmlChar*)"rightleft"))
					pb->SetOrientation(ProgressBar::ORIENT_RIGHT_LEFT);
				else if(!xmlStrcmp(val, (xmlChar*)"bottomtop"))
					pb->SetOrientation(ProgressBar::ORIENT_BOTTOM_TOP);
				else if(!xmlStrcmp(val, (xmlChar*)"topbottom"))
					pb->SetOrientation(ProgressBar::ORIENT_TOP_BOTTOM);
				
				xmlFree(val);
			}
		}
	}
	
	return v;
}

/************************************************************************************************
	create_viewport
*************************************************************************************************/

static View* create_viewport(xmlNodePtr node, View* parent)
{
	View* v = process_class_field(node, parent);
	if(!v)
		v = new phui::View;
	
	if(v)
	{
		process_view_fields(v, parent, node);
		v->SetAcceptsMouseClick(false);
		v->MoveTo(0,0);
		v->ResizeTo(_viewportwidth, _viewportheight);
	}
	
	return v;
}


/************************************************************************************************
	create_stackview
*************************************************************************************************/

static View* create_stackview(xmlNodePtr node, View* parent)
{
	View* v;
	
	v = process_class_field(node, parent);
	if(v == nullptr) v = new StackView;	
	
	process_view_fields(v, parent, node);

	return v;
}

/************************************************************************************************
	UIXMLColor
*************************************************************************************************/

void phui::UIXMLColor(const unsigned char* val, float* r, float* g, float* b, float* a)
{
	*r = *g = *b = 0; 
	if(a) *a = 1;
	
	if(val[0] == '#')
	{
		char bs[3]; bs[2] = 0;
		val++;
		
		if(val[0] && val[1])
		{
			bs[0] = val[0]; bs[1] = val[1];
			*r = strtoul(bs, nullptr, 16) / 255.0f;
			val += 2;				
		}
		if(val[0] && val[1])
		{
			bs[0] = val[0]; bs[1] = val[1];
			*g = strtoul(bs, nullptr, 16) / 255.0f;
			val += 2;				
		}
		if(val[0] && val[1])
		{
			bs[0] = val[0]; bs[1] = val[1];
			*b = strtoul(bs, nullptr, 16) / 255.0f;
			val += 2;				
		}
		if(val[0] && val[1])
		{
			bs[0] = val[0]; bs[1] = val[1];
			if(a) *a = strtoul(bs, nullptr, 16) / 255.0f;
			val += 2;				
		}
	}
}

/************************************************************************************************
	create_listview
*************************************************************************************************/

static View* create_listview(xmlNodePtr node, View* parent)
{
	View* v;
	
	v = process_class_field(node, parent);
	if(v == nullptr) v = new ListView;	
	
	process_view_fields(v, parent, node);

	return v;
}

/************************************************************************************************
	create_textview
*************************************************************************************************/

static View* create_textview(xmlNodePtr node, View* parent)
{
	View* v;
	xmlChar* val;
	
	v = process_class_field(node, parent);
	if(v == nullptr) v = new TextView;	
	
	process_view_fields(v, parent, node);
	
	val = xmlGetProp(node, (xmlChar*)UIXML_MAXLENGTH_FIELD);
	if(val)
	{
		TextView* tv = dynamic_cast<TextView*>(v);

		if(tv)
			tv->SetMaxLength(atoi((char*)val));

		xmlFree(val);
	}

	float sx = 0, sy = 0;
	
	val = xmlGetProp(node, (xmlChar*)UIXML_FONTSHADOWX_FIELD);
	if(val)
	{
		sx = (float)atof((char*)val);
		xmlFree(val);
	}

	val = xmlGetProp(node, (xmlChar*)UIXML_FONTSHADOWY_FIELD);
	if(val)
	{
		sy = (float)atof((char*)val);
		xmlFree(val);
	}
	
	val = xmlNodeGetContent(node);
	if(val)
	{
		TextView *tv = dynamic_cast<TextView*>(v);

		if(tv)
		{
			const wchar_t* wval = phui::UIXMLLocalize((const char*)val);
			
			// make sure there's something besides just space
			for(size_t c = 0; c < wcslen(wval); c++)
			{
				if(!isspace(wval[c]))
				{
					tv->SetText((char*)val);
					break;
				}
			}
		}
		
		xmlFree(val);
	}
	
	if(sx || sy)
	{
		TextView *tv = dynamic_cast<TextView*>(v);
		if(tv) tv->SetFontShadow(sx, sy);
	}
	
	return v;
}

/************************************************************************************************
	create_textview
*************************************************************************************************/

static View* create_console(xmlNodePtr node, View* parent)
{
	View* v;
	
	v = process_class_field(node, parent);
	if(v == nullptr) v = new ConsoleView;	
	
	process_view_fields(v, parent, node);
	
	return v;
}

/************************************************************************************************
	create_tooltip
*************************************************************************************************/

static View* create_tooltip(xmlNodePtr node, View* parent)
{
	View *v = create_textview(node, parent);
	
	v->SetName((const char*)node->doc->URL);
	v->SetMaxSize(v->Width(), v->Height());
	v->Hide();
	
	return v;
}

/************************************************************************************************
	create_window
*************************************************************************************************/

static View* create_window(xmlNodePtr node, View* parent)
{
	View *v = create_view(node, parent);
	
	v->SetIsWindow(true);

	return v;
}

/************************************************************************************************
	create_contextmenu
*************************************************************************************************/

static View* create_contextmenu(xmlNodePtr node, View* parent)
{
	View* v;
	xmlChar* val;

	v = process_class_field(node, parent);
	if(v == nullptr) v = new ContextMenu;	
	
	process_view_fields(v, parent, node);
	
	ContextMenu* menu = dynamic_cast<ContextMenu*>(v);

	if(menu)
	{
		val = xmlGetProp(node, (xmlChar*)UIXML_VALUE_FIELD);
		if(val) 
		{
			v->SetValue(phui::UIXMLLocalize((const char*)val));		
			xmlFree(val);	
		}

		val = xmlGetProp(node, (xmlChar*)UIXML_LABEL_FIELD);
		if(val) 
		{
			v->SetValue(phui::UIXMLLocalize((const char*)val));		
			xmlFree(val);	
		}	

		val = xmlGetProp(node, (xmlChar*)UIXML_ALIGN_FIELD);
		if(val) 
		{
			if(!xmlStrcmp(val,(xmlChar*)"center"))
			{
				menu->SetTextAlignment(Caption::ALIGN_CENTER);	
			}
			else if(!xmlStrcmp(val,(xmlChar*)"left"))
			{
				menu->SetTextAlignment(Caption::ALIGN_LEFT);	
			}
			else if(!xmlStrcmp(val,(xmlChar*)"right"))
			{
				menu->SetTextAlignment(Caption::ALIGN_RIGHT);					
			}
			xmlFree(val);	
		}
		
		val = xmlGetProp(node, (xmlChar*)UIXML_HINDENT_FIELD);
		if(val)
		{
			menu->SetHIndent((float)atof((char*)val));
			xmlFree(val);
		}

		val = xmlGetProp(node, (xmlChar*)UIXML_VINDENT_FIELD);
		if(val)
		{
			menu->SetVIndent((float)atof((char*)val));
			xmlFree(val);
		}
	}
	
	v->Hide();
	
	return v;
}

/************************************************************************************************
	create_ctxmenu
*************************************************************************************************/

static View* create_ctxmenu(xmlNodePtr node, View* parent)
{
	View* v;
	xmlChar* val;

	v = process_class_field(node, parent);
	if(v == nullptr) v = new CtxMenu;	
	
	process_view_fields(v, parent, node);
	
	CtxMenu* menu = dynamic_cast<CtxMenu*>(v);
	
	if(menu)
	{
		val = xmlGetProp(node, (xmlChar*)UIXML_VALUE_FIELD);
		if(val) 
		{
			v->SetValue(phui::UIXMLLocalize((const char*)val));		
			xmlFree(val);	
		}

		val = xmlGetProp(node, (xmlChar*)UIXML_LABEL_FIELD);
		if(val) 
		{
			v->SetValue(phui::UIXMLLocalize((const char*)val));		
			xmlFree(val);	
		}	

		val = xmlGetProp(node, (xmlChar*)UIXML_ALIGN_FIELD);
		if(val) 
		{
			if(!xmlStrcmp(val,(xmlChar*)"center"))
			{
				menu->SetTextAlignment(Caption::ALIGN_CENTER);	
			}
			else if(!xmlStrcmp(val,(xmlChar*)"left"))
			{
				menu->SetTextAlignment(Caption::ALIGN_LEFT);	
			}
			else if(!xmlStrcmp(val,(xmlChar*)"right"))
			{
				menu->SetTextAlignment(Caption::ALIGN_RIGHT);					
			}
			xmlFree(val);	
		}

		val = xmlGetProp(node, (xmlChar*)UIXML_HINDENT_FIELD);
		if(val)
		{
			menu->SetHIndent((float)atof((char*)val));
			xmlFree(val);
		}

		val = xmlGetProp(node, (xmlChar*)UIXML_VINDENT_FIELD);
		if(val)
		{
			menu->SetVIndent((float)atof((char*)val));
			xmlFree(val);
		}
	}
	
	//v->Hide();
	
	return v;
}

/************************************************************************************************
	create_textcontrol
*************************************************************************************************/

static View* create_textcontrol(xmlNodePtr node, View* parent)
{
	View* v;
	
	v = process_class_field(node, parent);
	if(v == nullptr) v = new TextControl;	
	
	process_view_fields(v, parent, node);
	
	xmlChar* val = xmlGetProp(node, (xmlChar*)UIXML_MAXLENGTH_FIELD);
	if(val)
	{
		TextControl* tc = dynamic_cast<TextControl*>(v);

		if(tc)
            tc->SetMaxLength(atoi((char*)val));

		xmlFree(val);
	}
	
	val = xmlGetProp(node, (xmlChar*)UIXML_ALIGN_FIELD);
	if(val) 
	{
		TextControl* tc = dynamic_cast<TextControl*>(v);
		if(tc)
		{	
			if(!xmlStrcmp(val,(xmlChar*)"center"))
			{
				tc->SetTextAlignment(TextControl::ALIGN_CENTER);	
			}else if(!xmlStrcmp(val,(xmlChar*)"left"))
			{
				tc->SetTextAlignment(TextControl::ALIGN_LEFT);	
			}else if(!xmlStrcmp(val,(xmlChar*)"right"))
			{
				tc->SetTextAlignment(TextControl::ALIGN_RIGHT);					
			}
		}
		xmlFree(val);	
	}

	val = xmlGetProp(node, (xmlChar*)UIXML_HISTORY_FIELD);
	if(val)
	{
		TextControl* tc = dynamic_cast<TextControl*>(v);
		if(tc)
            tc->SetHistorySize(atoi((char*)val));
		
		xmlFree(val);
	}

	return v;
}

/************************************************************************************************
	create_imecontrol
*************************************************************************************************/

static View* create_imecontrol(xmlNodePtr node, View* parent)
{
	View* v;
	
	v = process_class_field(node, parent);
	if(v == nullptr) v = new IMEControl;	
	
	process_view_fields(v, parent, node);
	
	/*
	xmlChar* val = xmlGetProp(node, (xmlChar*)UIXML_MAXLENGTH_FIELD);
	if(val)
	{
		TextControl* tc = dynamic_cast<TextControl*>(v);

		if(tc)
            tc->SetMaxLength(atoi((char*)val));

		xmlFree(val);
	}
	
	val = xmlGetProp(node, (xmlChar*)UIXML_ALIGN_FIELD);
	if(val) 
	{
		TextControl* tc = dynamic_cast<TextControl*>(v);
		if(tc)
		{	
			if(!xmlStrcmp(val,(xmlChar*)"center"))
			{
				tc->SetTextAlignment(TextControl::ALIGN_CENTER);	
			}else if(!xmlStrcmp(val,(xmlChar*)"left"))
			{
				tc->SetTextAlignment(TextControl::ALIGN_LEFT);	
			}else if(!xmlStrcmp(val,(xmlChar*)"right"))
			{
				tc->SetTextAlignment(TextControl::ALIGN_RIGHT);					
			}
		}
		xmlFree(val);	
	}

	val = xmlGetProp(node, (xmlChar*)UIXML_HISTORY_FIELD);
	if(val)
	{
		TextControl* tc = dynamic_cast<TextControl*>(v);
		if(tc)
            tc->SetHistorySize(atoi((char*)val));
		
		xmlFree(val);
	}
	*/

	return v;
}

/************************************************************************************************
	menu_tags
*************************************************************************************************/

static View* menu_tags(xmlNodePtr node, View* parent)
{
	xmlChar* val;
	View* v = nullptr;
	
	if(xmlStrcmp((xmlChar*)UIXML_POPUP_TAG, node->name) == 0)
	{
			v = process_class_field(node, parent);
			if(v == nullptr) v = new PopupMenuButton;

			process_view_fields(v, parent, node);
			
			return v;
	}

	if(xmlStrcmp((xmlChar*)UIXML_MENUITEM_TAG, node->name) == 0)
	{
		PopupMenuButton* popup = dynamic_cast<PopupMenuButton*>(parent);
		if(popup)
		{
			MenuItem* item = new MenuItem;
			popup->MenuList()->AddItem(item);
			
			val = xmlGetProp(node, (xmlChar*)UIXML_LABEL_FIELD);
			if(val)
			{
				item->SetLabel(phui::UIXMLLocalize((const char*)val));
				xmlFree(val);
			}
			
			return nullptr;
		}
	}
	
	return nullptr;
}

/************************************************************************************************
	process_include_tag
*************************************************************************************************/

static View* process_include_tag(xmlNodePtr node, View* parent)
{
	View* v = 0;
	xmlChar* val;
	val = xmlGetProp(node, (xmlChar*)UIXML_FILE_FIELD);
	if(val)
	{
		xmlDocPtr doc = xmlReadFile((char*)val, nullptr, XML_PARSE_DTDLOAD | XML_PARSE_RECOVER);
		if(doc)
		{
			xmlNodePtr root = xmlDocGetRootElement(doc);
			if(root)
				v = traverse_tree(root, parent);
		
			xmlFreeDoc(doc);
		}
		xmlFree(val);
	}
	
	return v;
}

/************************************************************************************************
	create_scrollbar
*************************************************************************************************/

static View* create_scrollbar(xmlNodePtr node, View* parent)
{
	View* v;		
	xmlChar* val;
	int orient			= phui::Scrollbar::ORIENT_VERTICAL;
	float minbarsize	= 20.0f;
	
	val = xmlGetProp(node, (xmlChar*)UIXML_ORIENT_FIELD);
	if(val)
	{
		if(!xmlStrcmp(val,(xmlChar*)"horizontal"))
			 orient = phui::Scrollbar::ORIENT_HORIZONTAL;
		
		xmlFree(val);
	}
	
	// minbarsize
	val = xmlGetProp(node, (xmlChar*)UIXML_MINBARSIZE_FIELD);
	if(val)
	{
		minbarsize = (float)atof((char*)val);
		xmlFree(val);
	}
	
	v = process_class_field(node, parent);
	if(v == nullptr) v = new Scrollbar(orient, 0.0f);
	
	if(orient == phui::Scrollbar::ORIENT_VERTICAL)
		minbarsize = v->ScaleValue(View::SCALE_HEIGHT, minbarsize);
	else
		minbarsize = v->ScaleValue(View::SCALE_WIDTH, minbarsize);
	
	Scrollbar* sb = dynamic_cast<Scrollbar*>(v);
	if(sb) sb->SetBar(orient, minbarsize);
	
	process_view_fields(v, parent, node);
	v->SetLayer(100);
	
	return v;
}

/************************************************************************************************
	create_resizebox
*************************************************************************************************/

/*static View* create_resizebox(xmlNodePtr node, View* parent)
{
	View* v;		
		
	v = process_class_field(node, parent);
	if(v == nullptr) v = new ResizeWidget;
	
	process_view_fields(v, parent, node);
	v->SetLayer(100);
	
	return v;
}
*/

/************************************************************************************************
	create_row
*************************************************************************************************/

static View* create_row(xmlNodePtr node, View* parent)
{
	View* v = process_class_field(node, parent);
	if(!v)
		v = new phui::View;
	
	if(v)
		process_view_fields(v, parent, node);
	
	ListView* lv = dynamic_cast<ListView*>(parent);
	if(lv)
		lv->AddRow(v);
	
	return v;
}

/************************************************************************************************
	create_slider
*************************************************************************************************/

static View* create_slider(xmlNodePtr node, View* parent)
{
	View* v;		
	xmlChar* val;
	int orient = phui::Slider::ORIENT_HORIZONTAL;
	
	val = xmlGetProp(node, (xmlChar*)UIXML_ORIENT_FIELD);
	if(val)
	{
		if(!xmlStrcmp(val,(xmlChar*)"vertical"))
			 orient = phui::Slider::ORIENT_VERTICAL;
		
		xmlFree(val);
	}
	
	v = process_class_field(node, parent);
	if(v == nullptr) v = new Slider(orient);
	
	process_view_fields(v, parent, node);
	
	Slider* slider = dynamic_cast<Slider*>(v);
	if(slider)
	{
		val = xmlGetProp(node, (xmlChar*)UIXML_MINVALUE_FIELD);
		if(val)
		{
			slider->SetMinValue((float)atof((char*)val));	
			xmlFree(val);
		}

		val = xmlGetProp(node, (xmlChar*)UIXML_MAXVALUE_FIELD);
		if(val)
		{
			slider->SetMaxValue((float)atof((char*)val));				
			xmlFree(val);
		}

		phui::ViewDrawer* drawer = slider->GetDrawer();

		if(drawer)
		{
			CSSViewDrawer* css = dynamic_cast<CSSViewDrawer*>(drawer);

			if(css)
			{
				float th = 0;
				float tw = 0;

				slider->GetThumbSize(&tw, &th);

				const char* thp = css->GetStyleProperty("thumb-height");
				const char* twp = css->GetStyleProperty("thumb-width");
				
				if(twp) tw = (float)atof(twp);
				if(thp) th = (float)atof(thp);
				
				slider->SetThumbSize(&tw, &th);
			}
		}
	}
		
	return v;
}

