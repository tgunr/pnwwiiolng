#pragma once

#include <wchar.h>
#include <vector>
#include <string>

#include "phui/MessageHandler.h"
#include "phui/Scalar.h"
#include "phui/GraphicsContext.h"

namespace phui
{

class Viewport;
class GraphicsContext;
class ViewDrawer;
class Referenced;
class ViewDrawerFactory;

class View;
class ChildList : public std::vector<View*>{};

/************************************************************************************************
	View
*************************************************************************************************/
/**
	This is the base View class used for drawing and event handling.
*/
class View : public MessageHandler
{
public:
	View() : MessageHandler(), _value(),
			_wlimiter(0), _hlimiter(0), _wscaler(0), _hscaler(0),
			_x(0), _y(0), _width(0), _height(0), _owidth(0), _oheight(0),
			_scroll_x(0), _scroll_y(0),
			_parent(NULL),
			_preferredwidth(0), _preferredheight(0),
			_minwidth(0), _minheight(0),
			_maxwidth(10000), _maxheight(10000),
			_isVisible(false), _isWindow(false), _isModal(false),
			_enabled(true),
			_isFocus(false),
			_moveOnDrag(false),
			_acceptsFocus(false),
			_dragx(0.0f), _dragy(0.0f),
			_font(0),
			_rfont(0),
			_clipl(-10000), _clipb(-10000), _clipr(10000), _clipt(10000),
			_resizemode(NO_RESIZE), _aspectmode(ASPECT_SCALE),
			_aspectoffset(0.0f),
			_rmargin(0), _lmargin(0), _tmargin(0), _bmargin(0),
			_acceptsmouseclick(true),
			_layer(0),
			_isclosing(false),
			_drawer(NULL),
			_contentwidth(0), _contentheight(0),
			_tooltipdelay(0),
			_tooltip(),
			_tooltippath(),
			_source(NULL),
			_parentviewport(NULL),
			_graphicscontext(NULL),
			_children(NULL),
			_userdata(NULL),
			_usermsg(NULL)
	{
		_bgcolor[0] = _bgcolor[1] = _bgcolor[2] = _bgcolor[3] = 0;
		_bordercolor[0] = _bordercolor[1] = _bordercolor[2] = _bordercolor[3] = 0;
		_textcolor[0] = _textcolor[1] = _textcolor[2] = 0;
		_textcolor[3] = 1;
	}

	virtual ~View(); ///< detach from parent view before calling delete
    
	virtual void DrawSelf();///< subclasses should override this function, co-ordinate system is relative to view (0,0 is bottom left of view)
	virtual void Draw(); 	///< sets up local co-ordinate system, then calls DrawSelf, then calls subviews' DrawSelf
	virtual void DrawChildren();
	virtual void Clip();

	void DrawShadeShadow(View* v);
	void DrawPointShadow(View* v);

	GraphicsContext* GetGraphicsContext() const;

	Viewport* ParentViewport() const;
	View* ParentWin() const;
	View* Parent() const   { return _parent;}
		
	View* ChildAt(unsigned int) const;
	ChildList* Children() const;
	unsigned int ChildCount() const;
	virtual int AddChild(View* view, View* before=0); 	///< takes ownership of child.
	virtual int RemoveChild(View*); ///< removes from child list, disowns view, call before deleting a view directly.
	virtual int ReorderChild(View*, View* before);
	
	virtual View* FindView(const char* name) const;
	virtual bool FindView(View*) const;
    virtual View* FindViewHit(float x, float y, bool wincheck = false) const; // x,y in local co-ordinates;
	virtual bool FindViewsHit(float x, float y, std::vector<View*>& views, bool wincheck = false) const;
	virtual View* FindSibling(const char *name) const;
	bool HasModalChild(const char* exclude = NULL, bool onlyVisible = false) const;
	
	// co-ordinate system starts bottom left, values are relative to the parent
	float Width()  const { return _width; }
	float Height() const { return _height;}
	float X() const { return _x;}
	float Y() const { return _y;}
	float Left() const { return _x;}
	float Right() const { return _x+_width; }
	float Bottom() const { return _y; }
	float Top() const { return _y+_height;}
	
	void ScreenToLocal(float* x, float* y);
	void LocalToScreen(float* x, float* y);
	
	void SetLeft(float left) { _width = _x+_width-left; _x=left; if(_width<0) _width = 0;}
	void SetRight(float right) { _width=right-_x; if(_width<0){_width=0;_x=right;} }
	void SetBottom(float bottom) {_height=_y+_height-bottom; _y=bottom; if(_height<0)_height=0;}
	void SetTop(float top) { _height=top-_y; if(_height<0){_height=0;_y=top;} }
	void SetOrigSize(float w, float h);
	
	virtual void ResizeToAspect(int follow);
	virtual void ResizeTo(float width, float height);
	virtual void ResizeBy(float dw, float dh);
	virtual void MoveTo(float x, float y);
	virtual void MoveBy(float dx, float dy);
	virtual void ScrollTo(float x, float y);
	virtual void ScrollBy(float dx, float dy);
	
	void SetBackgroundColor(float r, float g, float b, float a = 1.0);
	void GetBackgroundColor(float *r, float *g, float *b, float *a);

	void SetBorderColor(float r, float g, float b, float a = 1.0);
	void GetBorderColor(float *r, float *g, float *b, float *a);
	
	// lots of subclasses use font and textcolor so we'll put these functions here
	void SetFont(unsigned int f, int r = 0) { _font = f; _rfont = r; }
	void SetTextColor(float r, float g, float b, float a = 1.0);
	unsigned int GetFont() const { return _font; }
	void GetTextColor(float* r, float* g, float* b, float* a);
	virtual void FontResized(unsigned int size);
	
	virtual void MessageReceived(Message*, MessageHandler*); ///< Dispatches UI messages to functions below, handles tab group navigation, tool help

	virtual void SetValue(const Scalar& v) { _value = v;}
	virtual Scalar GetValue() const { return _value;}

	// ui event handling routines
	// unless noted, co-ordinates are relative to view	
	virtual bool KeyDown(int keycode, unsigned short unicode) { return false; }
	virtual bool KeyUp  (int keycode, unsigned short unicode) { return false; }
	
	virtual bool MouseDown (float x, float y, int button) { return false; }
	virtual bool MouseUp   (float x, float y, int button) { return false; }
	virtual bool MouseMove (float x, float y, int state);
	virtual bool MouseDrag (float x, float y, int button, bool start);
	virtual bool MouseScroll(float x, float y, int dir)   { return false; }
	
	void SetAcceptsFocus(bool b = true) { _acceptsFocus = b; }
	bool AcceptsFocus() { return _acceptsFocus;}
	
	virtual bool MakeFocus(bool focus = true);
	bool IsFocus() const { return _isFocus; }
	View* FindFocusView() const;
	void FindFocusViews(std::vector<View*>& views) const;
	
	virtual void Show();
	virtual void Hide();
	bool IsVisible() const { return _isVisible; }
	bool IsWindow() const { return _isWindow; }
	bool IsModal() const { return _isModal; }
	void SetEnable(bool enable = true) { _enabled = enable; }
	void SetIsWindow(bool isWindow = true) { _isWindow = isWindow; }
	void SetIsModal(bool isModal = true) { _isModal = isModal; }
	bool Enabled() const { return _enabled; }
	void SetMoveOnDrag(bool b = true) { _moveOnDrag = b; } ///< Sets if view will move on a mouse drag
	
	virtual void PreferredSize(float* w, float* h) { *w = _preferredwidth; *h = _preferredheight; }
	virtual void SetPreferredSize(float w, float h) { _preferredwidth = w; _preferredheight = h; }

	virtual void MinSize(float* w, float* h) { *w = _minwidth; *h = _minheight;} 
	virtual void SetMinSize(float w, float h) { _minwidth = w; _minheight = h; } 

	virtual void MaxSize(float* w, float* h) { *w = _maxwidth; *h = _maxheight; } 
	virtual void SetMaxSize(float w, float h) { _maxwidth = w; _maxheight = h; } 
	
	void SetDrawer(ViewDrawer*); ///< takes ownership. 
	void SetDrawerStyle(const char* key, const char* val, bool recursive = false);
	ViewDrawer* GetDrawer() const { return _drawer; }
	virtual void AttachDrawer(ViewDrawerFactory* factory); //  (visitor pattern)
	bool DrawerEnabled() const;
	
    virtual void AttachedToWindow();
    virtual void DetachedFromWindow();
	
	void SetResizeMode(unsigned int mode) { _resizemode = mode; }
	unsigned int ResizeMode() const { return _resizemode; }

	void SetAspectMode(unsigned int m) { _aspectmode = m; }
	void SetAspectOffset(float o) { _aspectoffset = o; }
	unsigned int AspectMode() const { return _aspectmode; }

	void SetScalers(float ws, float hs, float wl, float hl);
	void GetScalers(float& ws, float& hs, float& wl, float& hl);
	
	float ScaleValue(int type, float val);
	float NormalizeValue(int type, float val);
	
	virtual void Pulse() {}
	virtual void Resized(){}
	virtual void Moved() {}
	
	bool Contains(float x, float y) const; /// co-ordinates relative to parent
	
	Message* UserMessage() const; /// you can use this however you like, is owned by View

	void SetUserData(const char* name, Referenced*);
	Referenced* UserData(const char* name) const;
	
	void SetMargins(float left, float right, float top, float bottom) { _lmargin=left;_rmargin=right;_tmargin=top;_bmargin=bottom; }
	void GetMargins(float* left, float* right, float* top, float* bottom) const { *left=_lmargin;*right=_rmargin;*top=_tmargin;*bottom=_bmargin;}
	
	void SetAcceptsMouseClick(bool b) { _acceptsmouseclick = b; }
	bool AcceptsMouseClick() const { return _acceptsmouseclick; }
	
	virtual void Close();
	
	void SetLayer(int layer) { if(_layer != layer) {_layer =layer; if(_parent) _parent->SortChildrenByLayer();}}
	int Layer() const { return _layer; }
	void SortChildrenByLayer();
	
	bool IsClosing() const { return _isclosing; }
	
	float ContentWidth() const  { return _contentwidth; }
	float ContentHeight() const { return _contentheight;}
	float ScrollX() const { return _scroll_x; }
	float ScrollY() const { return _scroll_y; }

	float WidthAspect() const { return _width / _owidth; }
	float HeightAspect() const { return _height / _oheight; }
	
	void SetSource(const char* name);
	const char* GetSource() const;
	
	// tooltip set methods
	void SetToolTip(const wchar_t *t);
	void SetToolTipPath(const char *p){ _tooltippath = p; }
	void SetToolTipDelay(int d){ _tooltipdelay = d; }
	
	// tooltip get methods
	bool HasToolTip() const;
	const wchar_t* GetToolTip() const;
	const char* GetToolTipPath(){ return _tooltippath.c_str(); }
	int GetToolTipDelay(){ return _tooltipdelay; }
	
// Resize modes
enum { 
	NO_RESIZE = 0x0,
	FOLLOW_LEFT = 0x1,
	FOLLOW_RIGHT = 0x2,
	FOLLOW_TOP = 0x4,
	FOLLOW_BOTTOM = 0x8,
	FOLLOW_V_CENTER = 0x10,
	FOLLOW_H_CENTER = 0x20,
	FOLLOW_FONT_HEIGHT = 0x40,
	FOLLOW_FONT_WIDTH = 0x80,
	FOLLOW_CENTER = FOLLOW_V_CENTER | FOLLOW_H_CENTER,
	FOLLOW_LEFT_RIGHT = FOLLOW_LEFT | FOLLOW_RIGHT,
	FOLLOW_TOP_BOTTOM = FOLLOW_TOP | FOLLOW_BOTTOM,
	FOLLOW_ALL_SIDES = FOLLOW_LEFT_RIGHT | FOLLOW_TOP_BOTTOM,
	FOLLOW_FONT = FOLLOW_FONT_HEIGHT | FOLLOW_FONT_WIDTH
};

// Aspect flags
enum
{ 
	ASPECT_NONE					= 0,
	ASPECT_SCALE_HEIGHT		= 1<<0,
	ASPECT_SCALE_WIDTH		= 1<<1,
	ASPECT_LIMIT_HEIGHT		= 1<<2,
	ASPECT_LIMIT_WIDTH		= 1<<3,
	ASPECT_ALIGN_LEFT			= 1<<4, // centered in left 1/3 of screen
	ASPECT_ALIGN_CENTER		= 1<<5, // centered in center 1/3 of screen
	ASPECT_ALIGN_RIGHT		= 1<<6, // centered in right 1/3 of screen
	ASPECT_LIMIT					= ASPECT_LIMIT_HEIGHT | ASPECT_LIMIT_WIDTH,
	ASPECT_SCALE				= ASPECT_SCALE_HEIGHT | ASPECT_SCALE_WIDTH,
	ASPECT_ALIGN					= ASPECT_ALIGN_LEFT | ASPECT_ALIGN_CENTER | ASPECT_ALIGN_RIGHT
};

enum
{
	SCALE_WIDTH		= 1,
	SCALE_HEIGHT		= 2,
	SCALE_X				= 3,
	SCALE_Y				= 4
};
	
protected:
	Scalar _value;

	float _wlimiter, _hlimiter, _wscaler, _hscaler;
	float _x, _y, _width, _height, _owidth, _oheight;
	float _scroll_x, _scroll_y;
	
	View* _parent;
	
	float _preferredwidth, _preferredheight;
	float _minwidth, _minheight;
	float _maxwidth, _maxheight;
	
	bool _isVisible, _isWindow, _isModal;
	bool _enabled;
	bool _isFocus;
	bool _moveOnDrag;
	bool _acceptsFocus;
	float _dragx, _dragy;
	unsigned int _font;
	int _rfont;
	float _clipl, _clipb, _clipr, _clipt;
	unsigned int _resizemode, _aspectmode;
	float _aspectoffset;
	float _rmargin, _lmargin, _tmargin, _bmargin;
	bool _acceptsmouseclick;
	int _layer;
	bool _isclosing;
	ViewDrawer* _drawer;
	float _contentwidth, _contentheight;
	
	unsigned int _tooltipdelay;
	std::wstring _tooltip;
	std::string _tooltippath;

	char* _source;
	
// never access these directly always use accessor
	mutable Viewport* _parentviewport;
	mutable GraphicsContext* _graphicscontext;
	
private:
class UserDataMap;
	ChildList* _children;
	UserDataMap* _userdata;
	mutable Message* _usermsg;

protected:
	float _bgcolor[4], _bordercolor[4];
	float _textcolor[4];
};

}
