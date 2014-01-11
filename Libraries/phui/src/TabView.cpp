#include "phui-precompiled-header.h"

#include "phui/TabView.h"
#include "phui/GraphicsContext.h"
#include "phui/UIMessages.h"
#include "phui/MessageLoop.h"
#include "phui/Message.h"
#include "phui/Viewport.h"

using namespace phui;

#define TABXMARGIN 5
#define TABYMARGIN 2

class TabView::TabList : public std::vector<Tab> {};

/************************************************************************************************
	Control
*************************************************************************************************/

TabView::TabView() : Control()
{
	_value.Convert(Scalar::LongT); 
	_tabs = new TabList;
	_pressed = _initialized = false;
	_mousedown = _lasthit = -1;
	_lmargin = _rmargin = 5;
	_tmargin = _bmargin = 5;
	_tabalign = ALIGN_TOP;
}

TabView::~TabView()
{
	delete _tabs;
}

/************************************************************************************************
	DrawSelf
*************************************************************************************************/

void TabView::DrawSelf()
{
	if(!_initialized)
		InitTabs();
	
	Control::DrawSelf();
	
	if(DrawerEnabled()) return;
	
	// self drawn TabViews don't currently make use of right aligned tabs
	GraphicsContext* ctx = GetGraphicsContext();
	float y = Height() - ctx->FontHeight(_font) - TABYMARGIN;
	float tabheight = ctx->FontHeight(_font) + 2 * TABYMARGIN;
	
	ctx->MovePenPosition(_lmargin,0);
	
	for(int i = 0; i < (int)_tabs->size(); i++)
	{
		Tab &tab = GetTab(i);
		
		if(tab.visible)
		{
			float tabwidth = tab.width;
			
			ctx->DisableTexture();
			ctx->MovePenPosition(0, Height()-tabheight);
			float box[8];
			box[0] = tabwidth; box[1] = 0;
			box[2] = tabwidth; box[3] = tabheight;
			box[4] = 0; box[5] = tabheight;
			box[6] = 0; box[7] = 0;

			if(_pressed && i == _mousedown)
				ctx->SetColor(.2f,.2f,.2f,1.0f);
			else
				ctx->SetColor(.8f,.8f,.8f,1.0f);
			ctx->DrawQuad(box);
			ctx->SetColor(0,0,0,1);
			ctx->DrawLineStrip(4, box, i != _value.Long());
			ctx->MovePenPosition(0, -(Height()-tabheight));
		
			ctx->EnableTexture();
			ctx->MovePenPosition(TABXMARGIN,y);
			ctx->SetColor(_textcolor[0],_textcolor[1],_textcolor[2],_textcolor[3]);
			ctx->DrawString(_font, tab.name.c_str());
			ctx->MovePenPosition(-TABXMARGIN,-y);
						
			ctx->MovePenPosition(tabwidth, 0);
		}
	}
}

/************************************************************************************************
	AddTab
*************************************************************************************************/

void TabView::AddTab(const char* name, int alignment)
{
	Tab tab;
	
	int len = (int)strlen(name);
	for(int i = 0; i < len; i++)
		tab.name += name[i];
	
	tab.index		= (int)_tabs->size();
	tab.alignment	= alignment;
	
	_tabs->push_back(tab);

	_initialized = false;
}

void TabView::AddTab(const wchar_t* name, int alignment)
{
	Tab tab;
	
	tab.index		= (int)_tabs->size();
	tab.alignment	= alignment;
	tab.name		= std::wstring(name);
	
	_tabs->push_back(tab);

	_initialized = false;
}

void TabView::AddTab(Tab tab)
{
	tab.index = (int)_tabs->size();
	_tabs->push_back(tab);

	_initialized = false;
}

/************************************************************************************************
	AddChild
*************************************************************************************************/

int TabView::AddChild(View* v,View* before)
{
	int err = Control::AddChild(v,before);
	if(err != 0) return err;
	
	if(v == ChildAt(_value))
		v->Show();
	else
		v->Hide();
	
	return 0;
}

/************************************************************************************************
	MinSize
*************************************************************************************************/

void TabView::MinSize(float* w, float* h)
{
	GraphicsContext* ctx = GetGraphicsContext();
	*w = 0; *h = 0;
	
	float tabheight = ctx->FontHeight(_font) + 2 * TABYMARGIN;
	float totaltabwidth = 0;
		
	for(int i = 0; i < (int)_tabs->size(); i++)
		totaltabwidth += GetTab(i).width;
	
	View* v;
	float maxviewwidth = 0, maxviewheight = 0;
	for(int i = 0; (v = ChildAt(i)) != NULL; i++)
	{
		float pw, ph;
		v->MinSize(&pw, &ph);

		if(pw > maxviewwidth)
			maxviewwidth = pw;
		if(ph > maxviewheight)
			maxviewheight = ph;
			
	}
	
	maxviewwidth += _lmargin + _rmargin;
	maxviewheight += _bmargin + _tmargin;
	
	*h = maxviewheight + tabheight;
	
	if(maxviewwidth > totaltabwidth)
		*w = maxviewwidth;
	else
		*w = totaltabwidth;
}

/************************************************************************************************
	FontResized
*************************************************************************************************/

void TabView::FontResized(unsigned int size)
{
	View::FontResized(size);
	InitTabs();
}

/************************************************************************************************
	MouseDown
*************************************************************************************************/

bool TabView::MouseDown (float x, float y, int button)
{
	_mousedown = FindTabHit(x,y);

	if(_mousedown > -1)
		_pressed = true;
	
	return true;
}

/************************************************************************************************
	MouseUp - only return true if we're clicking on a tab and not the body
*************************************************************************************************/

bool TabView::MouseUp(float x, float y, int button)
{
	bool handled = false;

	if(_mousedown > -1 && _pressed)
	{
		SetValue(_mousedown);
		handled = true;
		
		Invoke();
	}

	_mousedown = -1;
	_pressed = false;

	return handled;
}

/************************************************************************************************
	MouseMove
*************************************************************************************************/

bool TabView::MouseMove (float x, float y, int state )
{
	int hit = -1;
	
	if(_tabs->size() && y > GetTab(0).y) // all tabs have the same y
		hit = FindTabHit(x,y);
	
	if(hit != _lasthit)
	{
		_tooltip.clear();
		ParentViewport()->HideToolTip();
		
		if(ValidTab(hit))
			_tooltip = GetTab(hit).tooltip;	
		
		_lasthit = hit;
	}
	
	if(state == MOUSE_ENTER || state == MOUSE_OVER)
	{
		if(_mousedown > -1)
			_pressed = (_mousedown == hit);
	}
	else if(state == MOUSE_EXIT)
	{
		_pressed = false;
	}
	
	return (ValidTab(hit)) ? View::MouseMove(x, y, state): true;
}

/************************************************************************************************
	FindTabHit
*************************************************************************************************/

int TabView::FindTabHit(float x, float y)
{
	for(int i = 0; i < (int)_tabs->size(); i++)
	{
		Tab &tab = GetTab(i);
		
		if(tab.visible)
		{
			if(_tabalign == ALIGN_RIGHT)
			{
				float left = _width - tab.height;
				float top  = _height - _lmargin - tab.x;

				if(x >= left && x < left + tab.height && y < top + tab.width && y >= top - tab.width)
					return i;
			}
			else
			{
				if(x >= tab.x && x < tab.x + tab.width && y >= tab.y && y < tab.y + tab.height)
					return i;
			}
		}
	}
	
	return -1;
}

/************************************************************************************************
	ValidTab
*************************************************************************************************/

bool TabView::ValidTab(int i)
{
	if(_tabs->size() && i >= 0 && i < (int)_tabs->size())
		return true;
	
	return false;
}

/************************************************************************************************
	SetValue
*************************************************************************************************/

void TabView::SetValue(const Scalar& v)
{
	bool sendMsg = false;
	unsigned int oldval = _value;
	unsigned int newval = v;
	
	if(newval < 0) newval = 0;
	else if(newval >= _tabs->size()) newval = (unsigned int)_tabs->size();
	
	if(newval != oldval)
	{
		View* view = ChildAt(oldval);
		if(view) view->Hide();

		view = ChildAt(newval);
		if(view)
		{
			view->Show();
			sendMsg = true;
		}
	}
	
	View::SetValue(newval);
	
	if(sendMsg)
	{
		phui::Message msg(TAB_MSG);
		ParentLoop()->PostMessage(&msg, this);
	}
}

/************************************************************************************************
	Show
*************************************************************************************************/

void TabView::Show()
{
	if(ParentLoop() && !_isVisible)
	{
		phui::Message msg(SHOW_MSG);
		ParentLoop()->PostMessage(&msg, this);
	}
	
	_isVisible = true; 
		
	View* v = ChildAt(_value);
	if(v)
		v->Show();
}


int TabView::NumTabs() const
{
	return (int)_tabs->size();
}

TabView::Tab& TabView::GetTab(int i)
{
	if(!_initialized)
		InitTabs();

	return _tabs->at(i);
}

TabView::Tab* TabView::GetTab(View* child)
{
	if(child && ChildCount())
	{
		if(!_initialized)
			InitTabs();
		
		ChildList* children = Children();

		for(size_t i = 0; i < children->size(); i++)
		{
			if(children->at(i) == child)
				if(ValidTab(i)) return &_tabs->at(i);
		}
	}	
	
	return NULL;
}

bool TabView::TabPressed() const
{
	return _pressed;
}

int TabView::TabMouseDown() const
{
	return _mousedown;
}

// determines tab dimensions and position
// called during draw, but is only processed once based
// on the _initialized variable
void TabView::InitTabs()
{
	if(!_tabs) return;

	_initialized = true;

	GraphicsContext* ctx	= GetGraphicsContext();
	float tabheight			= ctx->FontHeight(_font) + 2 * TABYMARGIN;

	float y = ((_tabalign == ALIGN_RIGHT) ? Width(): Height()) - ctx->FontHeight(_font) - TABYMARGIN;
	float x = _lmargin;
	
	// left aligned tabs
	for(int i = 0; i < (int)_tabs->size(); i++)
	{
		Tab &tab = GetTab(i);

		if(tab.alignment == TabView::ALIGN_LEFT)
		{
			tab.height	= tabheight;
			tab.width	= ctx->FontAdvance(_font, tab.name.c_str()) + 2 * TABXMARGIN;
			
			tab.x = x;
			tab.y = y;
			
			x += tab.width;
		}
	}
	
	x = (_tabalign == ALIGN_RIGHT) ? _height - _rmargin: _width - _rmargin;
	
	// right aligned tabs
	for(int i = (int)_tabs->size() - 1; i >= 0; i--)
	{
		Tab &tab = GetTab(i);

		if(tab.alignment == TabView::ALIGN_RIGHT)
		{
			tab.height	= tabheight;
			tab.width	= ctx->FontAdvance(_font, tab.name.c_str()) + 2 * TABXMARGIN;
			
			x -= tab.width;
			
			tab.x = x;
			tab.y = y;
		}
	}
}

TabView::Tab::Tab()
{
	x = y = width = height = 0;
	alignment = index = 0;
	visible = true;
}

TabView::Tab::~Tab()
{
}
