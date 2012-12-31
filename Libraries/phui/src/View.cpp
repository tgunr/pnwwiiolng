#include "phui-precompiled-header.h"

#include <string>
#include <vector>
#include <algorithm>
#include <map>

#include "phui/Referenced.h"
#include "phui/GraphicsContext.h"
#include "phui/View.h"
#include "phui/Message.h"
#include "phui/Viewport.h"
#include "phui/UIMessages.h"
#include "phui/ViewDrawer.h"
#include "phui/MessageLoop.h"
#include "phui/CSSViewDrawer.h"
#include "phui/TextView.h"
#include "phui/Util.h"
#include "phui/UIXML.h"

using namespace phui;

class View::UserDataMap : public std::map<std::string, referenced_ptr> {};

static View* _focusView = NULL;
/************************************************************************************************
	View
*************************************************************************************************/

View::~View()
{
	if(IsFocus())
		MakeFocus(false);
	if(_source)
		delete [] _source;

	if(_children)
	{
		for(ChildList::iterator it = _children->begin(); it != _children->end(); ++it)
			delete (*it);

		delete _children;
	}

	delete _drawer;
	delete _usermsg;
	delete _userdata;
}

/************************************************************************************************
	ChildCount
*************************************************************************************************/

unsigned int View::ChildCount() const
{
	if(_children)
		return (unsigned int)_children->size();
	
	return 0;
}

/************************************************************************************************
	AddChild
*************************************************************************************************/

int View::AddChild(View* view, View* before)
{
 	if(!_children) _children = new ChildList;
	
	if(view->Parent() == NULL && view->ParentViewport() == NULL)
	{		
		_children->push_back(view);

		view->_parent = this;
		if(ParentLoop())
			view->AttachedToWindow();
	        
      	SortChildrenByLayer();
	    
		return 0;
	}
	
	return -1;
}

/************************************************************************************************
	ReorderChild
*************************************************************************************************/

int View::ReorderChild(View* view, View* before)
{
	if(view->_parent != this || (before && before->_parent != this)) return -1;
	if(view == before) return 0;
	
	ChildList::iterator vit = find(_children->begin(), _children->end(), view);
	if(vit == _children->end()) return -2;
	_children->erase(vit);
	
	ChildList::iterator bit = find(_children->begin(), _children->end(), before);
	_children->insert(bit, view);
	
	return 0;
}

/************************************************************************************************
	AttachedToWindow
*************************************************************************************************/

void View::AttachedToWindow()
{
    _isFocus = false;
	
	if(_parent)
	{
		_parent->ParentLoop()->AddHandler(this);
		SetNextHandler(_parent);
	}
		
	if(_children)
	{
		for(ChildList::iterator it = _children->begin(); it != _children->end(); ++it)
			(*it)->AttachedToWindow();
	}
}

/************************************************************************************************
	DetachedFromWindow
*************************************************************************************************/

void View::DetachedFromWindow()
{
    _isFocus = false;
	_graphicscontext = NULL;
	_parentviewport = NULL;
	
    ParentLoop()->RemoveHandler(this);
    
	if(_children)
	{
		for(ChildList::iterator it = _children->begin(); it != _children->end(); ++it)
			(*it)->DetachedFromWindow();
	}
}

/************************************************************************************************
	RemoveChild
*************************************************************************************************/

int View::RemoveChild(View* view)
{
	if(_children)
	{
		ChildList::iterator it = find(_children->begin(), _children->end(), view);
		if(it != _children->end())
		{
			if(ParentLoop())
				view->DetachedFromWindow();
			(*it)->_parent = NULL;
			_children->erase(it);
			
			return 0;
		}
	}	
	return -1;
}

void View::SetScalers(float ws, float hs, float wl, float hl)
{
	_wscaler		= ws;
	_hscaler		= hs;
	_wlimiter		= wl;
	_hlimiter		= hl;
}

void View::GetScalers(float& ws, float& hs, float& wl, float& hl)
{
	ws		= _wscaler;
	hs		= _hscaler;
	wl		= _wlimiter;
	hl		= _hlimiter;
}

// viewport scaling and aspect mode handling
float View::ScaleValue(int type, float val)
{
	if(type == View::SCALE_WIDTH || type == View::SCALE_X)
	{
		val = ScaleUIXMLValue(type, val, _aspectmode, _wscaler, _wlimiter);
		
		if(type == View::SCALE_X)
			val += _aspectoffset;
	}
	else if(type == View::SCALE_HEIGHT || type == View::SCALE_Y)
	{
		val = ScaleUIXMLValue(type, val, _aspectmode, _hscaler, _hlimiter);
	}

	return val;
}

float View::NormalizeValue(int type, float val)
{
	if(type == View::SCALE_WIDTH || type == View::SCALE_X)
	{
		if(type == View::SCALE_X)
			val -= _aspectoffset;

		val = NormUIXMLValue(type, val, _aspectmode, _wscaler, _wlimiter);
	}
	else if(type == View::SCALE_HEIGHT || type == View::SCALE_Y)
	{
		val = NormUIXMLValue(type, val, _aspectmode, _hscaler, _hlimiter);
	}

	return val;
}

/************************************************************************************************
	Draw
*************************************************************************************************/

void View::Draw()
{
	if(!_isVisible) return;

	if(Children() && Children()->size() > 0 && stricmp(Name(), "DEBUGCTX") == 0)
			bool stop = true;

	GraphicsContext* ctx = GetGraphicsContext();
	float px = _x, py = _y;
	
	ctx->MovePenPosition(px, py);
	Clip();
	
	if(_clipl < _clipr && _clipt > _clipb)
	{
		ctx->PushState();

		if(stricmp(Name(), "DEBUG") == 0)
			bool stop = true;


		DrawSelf();
		ctx->PopState();

		DrawChildren();
	}
	ctx->MovePenPosition(-px, -py);
}

/************************************************************************************************
	Clip
*************************************************************************************************/

void View::Clip()
{
	if(_parent)
	{
		GraphicsContext* ctx = GetGraphicsContext();
	
		_clipl = 0;
		_clipb = 0;
		_clipt = _height;
		_clipr = _width;

		if(_x < _parent->_clipl) 
			_clipl = -_x + _parent->_clipl;
		if(_y < _parent->_clipb ) 
			_clipb = -_y + _parent->_clipb;
		if(_y + _height > _parent->_clipt)
			_clipt = _parent->_clipt - _y;
		if(_x + _width > _parent->_clipr)
			_clipr = _parent->_clipr - _x;
			
		ctx->Clip(_clipl,_clipb,_clipr,_clipt);
	}
}

/************************************************************************************************
	DrawChildren
*************************************************************************************************/

void View::DrawChildren()
{
	if(_children)
	{
		for(ChildList::iterator it = _children->begin(); it != _children->end(); ++it)
			(*it)->Draw();
	}
}

/************************************************************************************************
	DrawSelf
*************************************************************************************************/

void View::DrawSelf(void)
{
	if(DrawerEnabled())
	{
		_drawer->Draw();
	}
	else
	{
		GraphicsContext* ctx = GetGraphicsContext();
		BoxVerts coord = {0, 0, _width, 0, _width, _height, 0, _height};
		
		// draw background
		if(_bgcolor[3])
		{
			if(!_parent || _parent->_bgcolor[0] != _bgcolor[0] || _parent->_bgcolor[1] != _bgcolor[1] ||
				_parent->_bgcolor[2] != _bgcolor[2] || _parent->_bgcolor[3] != _bgcolor[3] || _bgcolor[3] < 1.0f)
			{
				ctx->DisableTexture();
				ctx->SetColor(_bgcolor[0], _bgcolor[1], _bgcolor[2], _bgcolor[3]);
				ctx->DrawQuad(coord);
			}
		}
		
		// draw borders
		if(_bordercolor[3])
		{
			ctx->DisableTexture();
			ctx->SetColor(_bordercolor[0], _bordercolor[1], _bordercolor[2], _bordercolor[3]);
			
			box_verts_with_pos(coord, 0, 0, _width, _height);
			ctx->DrawLineStrip(4, coord.verts, true);
			
			/*
			if(_bmargin)
			{
				coord[0] = _lmargin; coord[1] = _bmargin; coord[2] = _width - _rmargin; coord[3] = _bmargin;
				ctx->DrawLineStrip(2, coord.verts, false, _bmargin);
			}
			if(_rmargin)
			{
				coord[0] = _width - _rmargin; coord[1] = 0; coord[2] = _width - _rmargin; coord[3] = _height;
				ctx->DrawLineStrip(2, coord.verts, false, _rmargin);
			}
			if(_tmargin)
			{
				coord[0] = _width - _rmargin; coord[1] = _height - _tmargin; coord[2] = _lmargin; coord[3] = _height - _tmargin;
				ctx->DrawLineStrip(2, coord.verts, false, _tmargin);
			}
			if(_lmargin)
			{
				coord[0] = _lmargin; coord[1] = _height; coord[2] = _lmargin; coord[3] = 0;
				ctx->DrawLineStrip(2, coord.verts, false, _lmargin);
			}
			*/
		}
		
		ctx->SetColor(1, 1, 1, 1);
	}
}

/************************************************************************************************
	SetBackgroundColor
*************************************************************************************************/

void View::SetBackgroundColor(float r, float g, float b, float a)
{
	_bgcolor[0] = r;
	_bgcolor[1] = g;
	_bgcolor[2] = b;
	_bgcolor[3] = a;
}

/************************************************************************************************
	GetBackgroundColor
*************************************************************************************************/

void View::GetBackgroundColor(float* r, float* g, float* b, float* a)
{
	*r = _bgcolor[0];
	*g = _bgcolor[1];
	*b = _bgcolor[2];
	*a = _bgcolor[3];
}

/************************************************************************************************
	SetBorderColor
*************************************************************************************************/

void View::SetBorderColor(float r, float g, float b, float a)
{
	_bordercolor[0] = r;
	_bordercolor[1] = g;
	_bordercolor[2] = b;
	_bordercolor[3] = a;
}

/************************************************************************************************
	GetBorderColor
*************************************************************************************************/

void View::GetBorderColor(float* r, float* g, float* b, float* a)
{
	*r = _bordercolor[0];
	*g = _bordercolor[1];
	*b = _bordercolor[2];
	*a = _bordercolor[3];
}

/************************************************************************************************
	MessageReceived
*************************************************************************************************/

void View::MessageReceived(Message* msg, MessageHandler* reply)
{
	bool handled = false;
	long button;
	float x, y;
	long unicode,keycode;
	float lx, ly;

	switch(msg->what)
	{
		case phui::KEY_DOWN:
		{
			msg->FindLong(UI_KEYCODE, &keycode);
			msg->FindLong(UI_KEY_UNICODE, &unicode);
			handled = KeyDown(keycode, (unsigned short)unicode);
		}
		break;
		case phui::KEY_UP:
		{
			msg->FindLong(UI_KEYCODE, &keycode);
			msg->FindLong(UI_KEY_UNICODE, &unicode);
			handled = KeyUp(keycode, (unsigned short)unicode);
		}
		break;
		case phui::MOUSE_DOWN:
		{
			msg->FindLong(UI_MOUSE_BUTTON, &button);
			msg->FindFloat(UI_MOUSE_X, &x);
			msg->FindFloat(UI_MOUSE_Y, &y);
			lx = x; ly = y;
			ScreenToLocal(&lx, &ly);
			handled = MouseDown(lx, ly, button);
		}
		break;
		
		case phui::MOUSE_UP:
		{
			msg->FindLong(UI_MOUSE_BUTTON, &button);
			msg->FindFloat(UI_MOUSE_X, &x);
			msg->FindFloat(UI_MOUSE_Y, &y);
			lx = x; ly = y;
			ScreenToLocal(&lx, &ly);
			handled = MouseUp(lx, ly, button);
		}
		break;
		
		case phui::MOUSE_DRAG:
		{
			bool dragstart;
			msg->FindLong(UI_MOUSE_BUTTON, &button);
			msg->FindFloat(UI_MOUSE_X, &x);
			msg->FindFloat(UI_MOUSE_Y, &y);
			msg->FindBool(UI_MOUSE_DRAG_START, &dragstart);
			lx = x; ly = y;
			ScreenToLocal(&lx, &ly);
			handled = MouseDrag(lx, ly, button, dragstart);
		}
		break;

		case phui::MOUSE_MOVE:
		{
			long state;
			msg->FindFloat(UI_MOUSE_X, &x);
			msg->FindFloat(UI_MOUSE_Y, &y);
			msg->FindLong(UI_MOUSE_MOVESTATE, &state);
			lx = x; ly = y;
			ScreenToLocal(&lx, &ly);
			handled = MouseMove(lx, ly, state);;
		}
		break;
				
		case phui::MOUSE_SCROLL:
		{
			long dir;
			msg->FindLong(UI_MOUSE_SCROLLDIR, &dir);
			msg->FindFloat(UI_MOUSE_X, &x);
			msg->FindFloat(UI_MOUSE_Y, &y);
			lx = x; ly = y;
			ScreenToLocal(&lx, &ly);
			handled = MouseScroll(lx, ly, dir);
		}
		break;

		case CLOSE_MSG:
		{
			View* v;
			if(msg->FindPointer(UI_VIEW_PTR, (void**)&v) == Message::OK)
			{
				if(RemoveChild(v) == 0)
				{
					delete v;
				}
			}
		}
		break;
	}
	
	if(!handled)
		MessageHandler::MessageReceived(msg, reply);
}

/************************************************************************************************
	MakeFocus
*************************************************************************************************/

bool View::MakeFocus(bool focus)
{
	Viewport* win = ParentViewport();

    if(win == NULL)
		return false;
    
    if(focus && !_isFocus)
    {
		if(_focusView && _focusView->IsFocus())
			_focusView->MakeFocus(false);
	}

	if(!_acceptsFocus)
		return false;

	bool modified = (focus != _isFocus);
	
	 _isFocus = focus;
	 _focusView = (_isFocus) ? (View*)this : NULL;
    
	if(modified)
		win->OnFocusModified(_isFocus);
	
	return true;
}

/************************************************************************************************
	ChildAt
*************************************************************************************************/

View* View::ChildAt(unsigned int i) const
{
	if(_children && i < _children->size())
        return (*_children)[i];

	return NULL;
}



/************************************************************************************************
	Children
*************************************************************************************************/

ChildList* View::Children() const
{
	return _children;
}

/************************************************************************************************
	MoveBy
*************************************************************************************************/

void View::MoveBy(float dx, float dy)
{
	_x += dx; _y += dy;
	Moved();
}

/************************************************************************************************
	MoveTo
*************************************************************************************************/

void View::MoveTo(float x, float y)
{
	_x = x; _y = y;
	Moved();
}

/************************************************************************************************
	ResizeTo
*************************************************************************************************/

void View::ResizeTo(float width, float height)
{
	float ow, oh;
	
	ow = _width; oh = _height;
	_width = width; _height = height;
	
	Resized();
	
	unsigned int count = ChildCount();
	for(unsigned int i = 0; i < count; i++)
	{
		View* v = ChildAt(i);
	
		if(!v || v->_resizemode == NO_RESIZE) continue;
		
		float vx = v->X();
		float vy = v->Y();
		float vw = v->Width();
		float vh = v->Height();

		if((v->_resizemode & FOLLOW_LEFT) && (v->_resizemode & FOLLOW_RIGHT))
		{
			vw += width - ow;
		}else
		{
			if(v->_resizemode & FOLLOW_RIGHT)
				vx += width - ow;
		}
		
		if((v->_resizemode & FOLLOW_TOP) && (v->_resizemode & FOLLOW_BOTTOM))
		{
			vh += height - oh;
		}else
		{
			if(v->_resizemode & FOLLOW_TOP)
				vy += height - oh;
		}
	
		v->ResizeTo(vw, vh);
		v->MoveTo(vx,vy);
	}
}

void View::SetOrigSize(float w, float h)
{
	_owidth	= w;
	_oheight	= h;
	//_wlimiter	= _height / _oheight;
	//_hlimiter	= _width / _owidth;
}

/************************************************************************************************
	ResizeToAspect
*************************************************************************************************/

void View::ResizeToAspect(int follow)
{
	const Viewport* vp = ParentViewport();
	
	float wAspect	= vp->Width()/1024;
	float hAspect	= vp->Height()/768;
	float diff		= wAspect - hAspect;

	switch(follow)
	{
		case FOLLOW_H_CENTER:
		{
			float origHeight = _height * (768/vp->Height());
			ResizeTo(_width, _height + (origHeight * diff));
		}
		break;
		case FOLLOW_V_CENTER:
		{
			float origWidth = _width * (1024/vp->Width());
			ResizeTo(_width + (origWidth * diff), _height);
		}
		break;
		default:
			// do nothing
		break;
	}
}

/************************************************************************************************
	ResizeBy
*************************************************************************************************/

void View::ResizeBy(float dw, float dh)
{
	ResizeTo(_width + dw, _height + dh);
}

/************************************************************************************************
	ScrollTo
*************************************************************************************************/

void View::ScrollTo(float x, float y)
{
	_scroll_x = x; _scroll_y = y;
}

/************************************************************************************************
	ScrollBy
*************************************************************************************************/

void View::ScrollBy(float dx, float dy)
{
	_scroll_x += dx; _scroll_y += dy;
}

bool View::HasModalChild(const char* exclude, bool onlyVisible) const
{
	if(_children)
	{
		for(ChildList::reverse_iterator it = _children->rbegin(); it != _children->rend(); ++it)
		{
			View* v = *it;

			if(!exclude || (v->Name() && std::strcmp(v->Name(), exclude) != 0))
			{
				if(!onlyVisible || v->IsVisible())
				{
					if(v->IsModal())
						return true;
					
					if(v->HasModalChild(exclude, onlyVisible)) 
						return true;
				}
			}
		}	
	}
	
	return false;
}

/************************************************************************************************
	FindView
*************************************************************************************************/

View* View::FindView(const char* name) const
{	
	if(_children)
	{
		for(ChildList::reverse_iterator it = _children->rbegin(); it != _children->rend(); ++it)
		{
			View* v = *it;

			if(v->Name() && std::strcmp(v->Name(), name) == 0)
				return (View*)v;

			v = v->FindView(name);
			if(v) 
				return v;
		}	
	}
	
	return NULL;
}

/************************************************************************************************
	FindViewHit
*************************************************************************************************/

View* View::FindViewHit(float x, float y, bool wincheck) const
{
	if(!_isVisible) return NULL;
	
	if(x >=0 && y >=0 && x <_width && y < _height)
	{   
		if(_children)
		{
			for(ChildList::reverse_iterator it = _children->rbegin(); it != _children->rend(); ++it)
			{
				View* v = *it;
				v = v->FindViewHit(x - (v->_x), y - (v->_y), wincheck);
				if(v)   
					return v;
			}
		}

		return (_acceptsmouseclick || (wincheck && _isWindow)) ? (View*)this : NULL;
	}
	
	return NULL;
}

/************************************************************************************************
	FindViewsHit - returns whether is has been blocked
*************************************************************************************************/

bool View::FindViewsHit(float x, float y, std::vector<View*>& views, bool wincheck) const
{
	if(_isVisible)
	{
		if(x >=0 && y >=0 && x <_width && y < _height)
		{   
			if(_acceptsmouseclick)
				views.push_back((View*)this);

			if(_children)
			{
				for(ChildList::reverse_iterator it = _children->rbegin(); it != _children->rend(); ++it)
				{
					View* v = *it;

					if(v->FindViewsHit(x - (v->_x), y - (v->_y), views, wincheck))
						return true;
				}
			}
			
			return wincheck && _isWindow;
		}
	}
	
	return false;
}

/************************************************************************************************
	FindView
*************************************************************************************************/

bool View::FindView(View* v) const
{
	if(v == 0) return false;
	
	if(_children)
	{
		for(ChildList::iterator it = _children->begin(); it != _children->end(); ++it)
		{
			if(*it == v) return true;
			if((*it)->FindView(v)) return true;
		}
	}
		
	return false;
}

/************************************************************************************************
	FindSibling
*************************************************************************************************/

View* View::FindSibling(const char* name) const
{	
	View *p = _parent;
	View *s = NULL;
	
	while(p && p->Parent()) p = p->Parent();
	if(p) s = p->FindView(name);
	
	return s;
}

/************************************************************************************************
	ScreenToLocal
*************************************************************************************************/

void View::ScreenToLocal(float* x, float* y)
{
	*x -= _x; *y -= _y;

	View* parent = _parent;
	while(parent)
	{
		*x -= parent->_x;
		*y -= parent->_y;
		parent = parent->_parent;
	}
}

/************************************************************************************************
	LocalToScreen
*************************************************************************************************/

void View::LocalToScreen(float* x, float* y)
{
	*x += _x; *y += _y;

	View* parent = _parent;
	while(parent)
	{
		*x += parent->_x;
		*y += parent->_y;
		parent = parent->_parent;
	}
}

/************************************************************************************************
	MouseDrag
*************************************************************************************************/

bool View::MouseDrag (float x, float y, int button, bool start)
{
	if(!_moveOnDrag) return false;
	
	LocalToScreen(&x, &y);
	if(!start)
		MoveBy(x-_dragx,y-_dragy);
			
	_dragx = x;
	_dragy = y;
	
	return true;
}

/************************************************************************************************
	MouseMove
*************************************************************************************************/

bool View::MouseMove (float x, float y, int state )
{
	if(HasToolTip())
	{
		Viewport *p = ParentViewport();
		
		if(p)
		{
			if(state == MOUSE_ENTER || state == MOUSE_OVER)
				p->ShowToolTip(x, y, this);
			else if(state == MOUSE_EXIT)
				p->HideToolTip();
		}
	}
	
	return true;
}

/************************************************************************************************
	SetTextColor
*************************************************************************************************/

void View::SetTextColor(float r, float g, float b, float a)
{
	_textcolor[0] = r; _textcolor[1] = g; _textcolor[2] = b; _textcolor[3] = a;
}
void View::GetTextColor(float* r, float* g, float* b, float* a)
{
	*r = _textcolor[0]; *g = _textcolor[1]; *b = _textcolor[2]; *a = _textcolor[3];
}

/************************************************************************************************
	FindFocusView
*************************************************************************************************/

View* View::FindFocusView() const
{
	if(_focusView && _focusView->IsVisible() && _focusView->IsFocus())
		return _focusView;
	return NULL;
	
	/*
	if(!_isVisible) return NULL;
	
	if(IsFocus()) return (View*)this;
	
	View* v;
	for(int i = 0; (v = ChildAt(i)); i++)
	{
		v = v->FindFocusView();
		if(v)
			return v;
	}
	
	return NULL;
	*/
}

/************************************************************************************************
	FindNextFocusView
*************************************************************************************************/

void View::FindFocusViews(std::vector<View*>& views) const
{
	if(_isVisible)
	{
		if(_acceptsFocus)
			views.push_back((View*)this);
		
		if(_children)
		{
			for(ChildList::iterator it = _children->begin(); it != _children->end(); ++it)
				(*it)->FindFocusViews(views);
		}
	}
}

/************************************************************************************************
	SetDrawer
*************************************************************************************************/

void View::SetDrawer(ViewDrawer* d)
{
	if(d == _drawer) return;
	delete _drawer;
	_drawer = d;
}

/************************************************************************************************
	Contains
*************************************************************************************************/

bool View::Contains(float inx, float iny) const
{
	float x = inx-_x;
	float y = iny-_y;
	return (x >= 0 && y >= 0 && x < _width && y < _height);
}

/************************************************************************************************
	ParentViewport
*************************************************************************************************/

Viewport* View::ParentViewport() const 
{ 
	if(_parentviewport == NULL && _parent)
		_parentviewport = _parent->ParentViewport();
		
	return _parentviewport;
}

View* View::ParentWin() const
{
	if(IsWindow())
		return (View*)this;

	if(_parent)
		return _parent->ParentWin();
	
	return NULL;
}

/************************************************************************************************
	GetGraphicsContext
*************************************************************************************************/

GraphicsContext* View::GetGraphicsContext() const
{	
	if(_graphicscontext == NULL && _parent)
		_graphicscontext = _parent->GetGraphicsContext();
		
	return _graphicscontext;
}

/************************************************************************************************
	UserMessage
*************************************************************************************************/

Message* View::UserMessage() const
{
	if(_usermsg == NULL) _usermsg = new Message;
	return _usermsg;
}

/************************************************************************************************
	SetUserData
*************************************************************************************************/

void View::SetUserData(const char* name, Referenced* ref)
{
	if(!_userdata) 
		_userdata = new UserDataMap;
	
	(*_userdata)[name] = ref;
}

/************************************************************************************************
	UserData
*************************************************************************************************/

Referenced* View::UserData(const char* name) const
{
	if(!_userdata) return NULL;
	
	UserDataMap::iterator it = _userdata->find(name);
	if(it != _userdata->end())
		return it->second.ptr();
		
	return NULL;
}

/************************************************************************************************
	AttachDrawer
*************************************************************************************************/

void View::AttachDrawer(ViewDrawerFactory* factory) 
{ 
	SetDrawer(factory->CreateDrawer(this)); 
}

/************************************************************************************************
	DrawerEnabled
*************************************************************************************************/

bool View::DrawerEnabled() const
{
	if(!_drawer) return false;
	
	return (_drawer->Enabled());
}

/************************************************************************************************
	DrawerEnabled
*************************************************************************************************/

void View::SetDrawerStyle(const char* key, const char* val, bool recursive)
{
	CSSViewDrawer* drawer = dynamic_cast<CSSViewDrawer*>(_drawer);
	
	if(drawer)
		drawer->SetStyleProperty(key, val);
	
	if(recursive && _children)
	{
		for(ChildList::iterator it = _children->begin(); it != _children->end(); ++it)
			(*it)->SetDrawerStyle(key, val, recursive);
	}
}

/************************************************************************************************
	Show
*************************************************************************************************/

void View::Show() 
{ 
	if(ParentLoop() && !_isVisible)
	{
		phui::Message msg(SHOW_MSG);
		ParentLoop()->PostMessage(&msg, this);
	}
	
	_isVisible = true; 
	
	if(_children)
	{
		for(ChildList::iterator it = _children->begin(); it != _children->end(); ++it)
		{
			(*it)->Show();
		}
	}
}

/************************************************************************************************
	Hide
*************************************************************************************************/

void View::Hide() 
{ 
	if(_children)
	{
		for(ChildList::iterator it = _children->begin(); it != _children->end(); ++it)
		{
			(*it)->Hide();
		}
	}
	
	if(IsFocus())
		MakeFocus(false);

	/* why is this here?
	View* v = FindFocusView(); 
	if(v) v->MakeFocus(false); 
	*/
	
	if(ParentLoop() && _isVisible)
	{
		phui::Message msg(HIDE_MSG);
		ParentLoop()->PostMessage(&msg, this);
	}
	
	_isVisible = false; 
}

/************************************************************************************************
	Close
*************************************************************************************************/

void View::Close()
{	
	if(_isclosing) return;
	
	View* parent = Parent();
	_isclosing = true;
	
	if(parent)
	{
		if(parent->IsClosing()) return;
		
		Hide();
				
		Message msg(CLOSE_MSG);
		msg.AddPointer(UI_VIEW_PTR, this);
		
		ParentLoop()->PostMessage(&msg, parent);
	}else
	{
		delete this; // always fun and dangerous
	}
}

/************************************************************************************************
	FontResized
*************************************************************************************************/

void View::FontResized(unsigned int size)
{
	// lets handle ourself
	if(_rfont)
	{
		GraphicsContext* ctx	= GetGraphicsContext();
		std::string name		= ctx->FontName(_font);

		SetFont(ctx->FindFont(name.c_str(), size + _rfont), _rfont);
		
		float width		= _width;
		float height	= _height;
		
		// resize width?
		// this resize mode should really only be used with views containing text
		if(_resizemode & FOLLOW_FONT_WIDTH)
			width = ctx->FontAdvance(_font, GetValue().WideString()) + _lmargin + _rmargin;
		
		// resize height?
		if(_resizemode & FOLLOW_FONT_HEIGHT)
			height = ctx->FontHeight(_font) + _bmargin + _tmargin;
		
		// resize if something changed
		if(width != _width || height != _height)
			ResizeTo(width, height);
	}
	
	// lets handle our children
	if(_children)
	{
		for(ChildList::iterator it = _children->begin(); it != _children->end(); ++it)
			(*it)->FontResized(size);
	}
}

void View::DrawPointShadow(View* v)
{
	if(!v->DrawerEnabled())	return;
	
	CSSViewDrawer* drawer = dynamic_cast<CSSViewDrawer*>(v->GetDrawer());

	if(!drawer) return;
	
	GraphicsContext* ctx = GetGraphicsContext();
	int numcoords = 4; // we should always be at least 4 points in size
	float coords[10];
	float color[4];
	
	coords[0] = v->X();
	coords[1] = v->Y();
	coords[2] = v->X();
	coords[3] = v->Y() + v->Height();
	coords[4] = v->X() + v->Width();
	coords[5] = v->Y() + v->Height();
	coords[6] = v->X() + v->Width();
	coords[7] = v->Y();
	
	// see if the help point is outside of my bounding box
	int horiz	= (drawer->GetShadowX() < v->X()) ? -1: (drawer->GetShadowX() > v->X() + v->Width()) ? 1: 0;
	int vert	= (drawer->GetShadowY() < v->Y()) ? -1: (drawer->GetShadowY() > v->Y() + v->Height()) ? 1: 0;

	// point lays outside of us?
	if(horiz && vert)
	{
		// point lies on a corner
		if(horiz < 0 && vert < 0) // lower left
		{
			coords[0] = drawer->GetShadowX();
			coords[1] = drawer->GetShadowY();
		}
		else if(horiz < 0 && vert > 0) // upper left
		{
			coords[2] = drawer->GetShadowX();
			coords[3] = drawer->GetShadowY();
		}
		else if(horiz > 0 && vert > 0) // upper right
		{
			coords[4] = drawer->GetShadowX();
			coords[5] = drawer->GetShadowY();
		}
		else if(horiz > 0 && vert < 0) // lower right
		{
			coords[6] = drawer->GetShadowX();
			coords[7] = drawer->GetShadowY();
		}
	}
	else if(horiz || vert)
	{
		numcoords = 5;

		// point lies on a side
		if(horiz < 0 && !vert) // left
		{
			for(int i = 0; i < 6; i++)
                coords[(7 - i) + 2] = coords[7 - i];

			coords[2] = drawer->GetShadowX();
			coords[3] = drawer->GetShadowY();
		}
		else if(!horiz && vert > 0) // top
		{
			for(int i = 0; i < 4; i++)
                coords[(7 - i) + 2] = coords[7 - i];
			
			coords[4] = drawer->GetShadowX();
			coords[5] = drawer->GetShadowY();
		}
		else if(horiz > 0 && !vert) // right
		{
			for(int i = 0; i < 2; i++)
                coords[(7 - i) + 2] = coords[7 - i];

			coords[6] = drawer->GetShadowX();
			coords[7] = drawer->GetShadowY();
		}
		else if(!horiz && vert < 0) // bottom
		{
			coords[8] = drawer->GetShadowX();
			coords[9] = drawer->GetShadowY();
		}
	}
	
	ctx->DisableTexture();
	drawer->GetShadowColor(&color[0], &color[1], &color[2], &color[3]);
	
	// if we have a shadow color, draw it
	if(color[3])
	{
		ctx->SetColor(color[0], color[1], color[2], color[3]);
		ctx->DrawPolygon(numcoords, coords);
	}
	
	drawer->GetShadowBorderColor(&color[0], &color[1], &color[2], &color[3]);
	
	// if we have a shadow border color, draw it
	if(color[3])
	{
		ctx->SetColor(color[0], color[1], color[2], color[3]);
		ctx->DrawLineStrip(numcoords, coords, true);
	}

	ctx->SetColor(1,1,1,1);
}

void View::DrawShadeShadow(View* v)
{
	if(!v->DrawerEnabled())	return;
	
	CSSViewDrawer* drawer = dynamic_cast<CSSViewDrawer*>(v->GetDrawer());

	if(!drawer) return;
	
	GraphicsContext* ctx = GetGraphicsContext();
	float coords[8];
	float color[4];
	
	coords[0] = v->X() + drawer->GetShadowX();
	coords[1] = v->Y() + drawer->GetShadowY();
	coords[2] = v->X() + drawer->GetShadowX();
	coords[3] = v->Y() + v->Height() + drawer->GetShadowY();
	coords[4] = v->X() + v->Width() + drawer->GetShadowX();
	coords[5] = v->Y() + v->Height() + drawer->GetShadowY();
	coords[6] = v->X() + v->Width() + drawer->GetShadowX();
	coords[7] = v->Y() + drawer->GetShadowY();

	ctx->DisableTexture();
	drawer->GetShadowColor(&color[0], &color[1], &color[2], &color[3]);

	if(color[3])
	{
		ctx->SetColor(color[0], color[1], color[2], color[3]);
		ctx->DrawQuad(coords);
	}

	drawer->GetShadowBorderColor(&color[0], &color[1], &color[2], &color[3]);
	
	// if we have a shadow border color, draw it
	if(color[3])
	{
		ctx->SetColor(color[0], color[1], color[2], color[3]);
		ctx->DrawLineStrip(4, coords, true);
	}

	ctx->SetColor(1,1,1,1);
}

void View::SetToolTip(const wchar_t *t)
{
	_tooltip = t;
	std::wstring::size_type index = std::wstring::npos;
	
	// incoming strings sometimes don't have newlines interpretted
	// depending on where they came from.
	while((index = _tooltip.find(L"\\n")) != std::wstring::npos)
		_tooltip.replace(index, 2, L"\n");
}

const wchar_t* View::GetToolTip() const
{
	if(ParentViewport() && ParentViewport()->GetToolTipMode())
	{
		if(Name())
			return utf8_to_utf16((const unsigned char*)Name());
		else
			return L"";
	}
	else
	{
		return _tooltip.c_str();
	}
}

bool View::HasToolTip() const
{
	return (ParentViewport()->GetToolTipMode()) ? (strlen(Name()) > 0): (_tooltip.length() > 0);
}

void View::SetSource(const char* name)
{
	if(name && *name)
	{
		int len = strlen(name);
		_source = new char[len+1];
		strcpy(_source, name);
	}
}

const char* View::GetSource() const
{
	if(_source)
		return _source;
	else if(_parent)
		return _parent->GetSource();
	
	return NULL;
}
/************************************************************************************************
	SortChildrenByLayer
*************************************************************************************************/

static bool layer_compare(View* a, View* b)
{
	return a->Layer() < b->Layer();
}

void View::SortChildrenByLayer()
{
	if(_children)
		sort(_children->begin(), _children->end(), layer_compare);
}