#include "phui-precompiled-header.h"

#include "phui/CtxMenu.h"
#include "phui/UIMessages.h"
#include "phui/GraphicsContext.h"
#include "phui/Message.h"
#include "phui/MessageLoop.h"
#include "phui/Viewport.h"

using namespace phui;

#define MOUSE_DELAY 500
#define START_DELAY 5000
#define MENU_CLICK_BORDER 5
#define MENU_CHILD_INDENT 10

/************************************************************************************************
	Button
*************************************************************************************************/

CtxMenu::CtxMenu(): Caption()
{
	_parentmenu = nullptr;
	_submenus = nullptr;
	_acceptsmouseclick = _isWindow = true;
	_isVisible = _open = false;
	_hindent = _vindent = 0;
	_mouseover = 0;
	_mousestate = MOUSE_EXIT;
	_tooltips = true;
	_expandmode = FOLLOW_BOTTOM | FOLLOW_RIGHT;
} 

CtxMenu::~CtxMenu()
{
	if(_submenus)
		delete _submenus;
}

void CtxMenu::DrawSelf()
{
	if(_isVisible)
	{
		if(_submenus && _submenus->size() == 2)
				bool stop = true;

		unsigned int now	= ParentLoop()->FrameTime();
		bool isroot			= IsRootMenu();

		if(_mousestate == MOUSE_OVER || _mousestate == MOUSE_ENTER)
			SetMouseOver(now);
		
		if(_mouseover + MOUSE_DELAY > now)
		{
			ShowChildren();
		}
		else
		{
			HideChildren();

			if(isroot)
				Hide();
		}
		
		if(!isroot && _parentmenu && _parentmenu->IsVisible())
			Caption::DrawSelf();
	}
}

void CtxMenu::Show()
{
	if(IsRootMenu())
	{
		_tooltips = ParentViewport()->GetToolTips();

		if(!_mouseover)
			_mouseover = ParentLoop()->FrameTime() + START_DELAY;
		
		// setup our submenus
		PositionSubMenus();
		
		// no tooltips while context menus are up
		ParentViewport()->SetToolTips(false);
		
		// make sure no tooltips get in the way
		ParentViewport()->HideToolTip();
	}
	
	ShowMenu();
}

int CtxMenu::AddChild(View* view, View* before)
{
	CtxMenu* menu = dynamic_cast<CtxMenu*>(view);
	
	if(menu)
	{
		if(!_submenus)
			_submenus = new MenuItems;
		
		menu->SetParentMenu(this);
		
		if(IsRootMenu())
		{
			return AddSubMenu(menu);
		}
		else
		{
			// we have children, so lets make sure the
			// indents are initialized to something useful
			if(!_hindent)
				_hindent = _width;
			
			// set the child's layer to one more than mine
			menu->SetLayer(Layer() + 1);
			
			// we're the parent
			_submenus->push_back(menu);
			
			// add the submenu
			return GetRootMenu()->AddSubMenu(menu, false);
		}
	}
	else
	{
		return Caption::AddChild(view, before);
	}
}

/************************************************************************************************
	AddSubMenu
*************************************************************************************************/

int CtxMenu::AddSubMenu(CtxMenu* menu, bool isparent)
{
	menu->Hide();
	
	if(isparent)
		_submenus->push_back(menu);
	
	if(!Parent())
		return Caption::AddChild((View*)menu);
	else
		return Parent()->AddChild((View*)menu);
}

/************************************************************************************************
	MouseUp
*************************************************************************************************/
 
bool CtxMenu::MouseUp(float x, float y, int button)
{
	//bool handled = Caption::MouseUp(x, y, button);
	
	GetRootMenu()->Hide();
	
	return true;
}

/************************************************************************************************
	MouseMove - this function probably needs to be reworked, its too complex
*************************************************************************************************/

bool CtxMenu::MouseMove(float x, float y, int state )
{
	_mousestate = state;

	/*
	if(_mousestate == MOUSE_ENTER)
		1;
	else if(_mousestate == MOUSE_EXIT)
		HideChildren();
	*/

	return Caption::MouseMove(x, y, state);
}

/************************************************************************************************
	ShowChildren
*************************************************************************************************/

void CtxMenu::ShowChildren()
{
	if(_submenus && !_open)
	{
		// we need to show our submenus
		// so lets make sure our sibling's
		// menus are closed
		if(_parentmenu)
		{
			MenuItems* submenus = _parentmenu->GetSubMenus();
			
			if(submenus && submenus->size())
			{
				for(unsigned int i = 0; i < submenus->size(); ++i)
				{
					CtxMenu* menu = (*submenus)[i];

					if(menu != this)
						menu->HideChildren();
				}
			}
		}
		
		for(unsigned int i = 0; i < _submenus->size(); ++i)
			(*_submenus)[i]->ShowMenu();

		_open = true;
	}
}

/************************************************************************************************
	HideChildren
*************************************************************************************************/

void CtxMenu::HideChildren()
{
	if(_submenus && _open)
	{
		for(unsigned int i = 0; i < _submenus->size(); ++i)
			(*_submenus)[i]->HideMenu();

		_open = false;
	}
}

/************************************************************************************************
	ShowMenu
*************************************************************************************************/

void CtxMenu::ShowMenu()
{
	Caption::Show();
	/*
	if(ParentLoop() && !_isVisible)
	{
		phui::Message msg(SHOW_MSG);
		ParentLoop()->PostMessage(&msg, this);
	}
	
	_isVisible = true;
	*/
}

/************************************************************************************************
	HideMenu
*************************************************************************************************/

void CtxMenu::HideMenu()
{
	Hide();
}

/************************************************************************************************
	Hide
*************************************************************************************************/

void CtxMenu::Hide() 
{ 
	Caption::Hide();

	if(IsRootMenu())
	{
		// set tooltips back to what it was
		ParentViewport()->SetToolTips(_tooltips);

		// now we close the context menu and we're done
		Close();
	}
}

/************************************************************************************************
	Close
*************************************************************************************************/

void CtxMenu::Close() 
{ 
	if(_submenus && _submenus->size())
	{
		for(unsigned int i = 0; i < _submenus->size(); ++i)
			((*_submenus)[i])->Close();
	}
	
	Caption::Close();
}

/************************************************************************************************
	SetMouseOver
*************************************************************************************************/

void CtxMenu::SetMouseOver(unsigned int o)
{
	_mouseover = o;

	if(_parentmenu)
		_parentmenu->SetMouseOver(_mouseover);
}

/************************************************************************************************
	IsRootMenu
*************************************************************************************************/

bool CtxMenu::IsRootMenu() const
{
	return (!_parentmenu) ? true: false;
}

/************************************************************************************************
	GetRootMenu
*************************************************************************************************/

CtxMenu* CtxMenu::GetRootMenu()
{
	if(_parentmenu)
	{
		CtxMenu* menu = _parentmenu;

		while(menu)
		{
			if(menu->IsRootMenu())
				return menu;
			
			menu = menu->GetParentMenu();
		}
	}
	
	return (CtxMenu*)this;
}

void CtxMenu::PositionSubMenus()
{
	if(_submenus && _submenus->size())
	{
		float mx = _x - MENU_CLICK_BORDER;
		float my = _y - _submenus->at(0)->Height() + MENU_CLICK_BORDER;
		
		PositionSubMenus(mx, my);
	}
}

void CtxMenu::PositionSubMenus(float x, float y)
{
	if(_submenus && _submenus->size())
	{
		// calculate the width of the longest submenu
		float bwidth	= 0.0f;
		float bheight	= 0.0f;
		float pwidth	= ParentViewport()->Width();
		float pheight	= ParentViewport()->Height();

		GetBranchDimensions(bwidth, bheight);

		if(_expandmode & FOLLOW_RIGHT && (x + bwidth > pwidth))
		{
			_expandmode &= ~FOLLOW_RIGHT;
			_expandmode |= FOLLOW_LEFT;
		}
		else if(_expandmode & FOLLOW_LEFT && (x - bwidth < 0))
		{
			_expandmode &= ~FOLLOW_LEFT;
			_expandmode |= FOLLOW_RIGHT;
		}
		if(_expandmode & FOLLOW_BOTTOM && (y - bheight < 0))
		{
			_expandmode &= ~FOLLOW_BOTTOM;
			_expandmode |= FOLLOW_TOP;
		}
		else if(_expandmode & FOLLOW_TOP && (y + bheight > pheight))
		{
			_expandmode &= ~FOLLOW_TOP;
			_expandmode |= FOLLOW_BOTTOM;
		}

		if(_expandmode & FOLLOW_LEFT)
			x -= bwidth;
		
		for(unsigned int i = 0; i < _submenus->size(); ++i)
		{
			CtxMenu* menu = (*_submenus)[i];

			// resize to fit branch width
			menu->ResizeTo(bwidth, menu->Height());
			
			if(i) y += (_expandmode & FOLLOW_TOP) ? menu->Height(): -menu->Height();
			else if(!IsRootMenu())
				y -= menu->Height();

			menu->MoveTo(x, y);
			menu->PositionSubMenus(x + menu->Width() - MENU_CHILD_INDENT/2, y + (menu->Height() - MENU_CHILD_INDENT/2));
		}
	}
}

void CtxMenu::GetBranchDimensions(float& w, float& h)
{
	const int iconWidth = 20;
	if(_submenus && _submenus->size())
	{
		GraphicsContext* ctx = GetGraphicsContext();

		for(unsigned int i = 0; i < _submenus->size(); ++i)
		{
			CtxMenu* menu = (*_submenus)[i];

			w = std::max(w, ctx->FontAdvance(_font, menu->GetValue().WideString())) + 37.0f;
			h += menu->Height();
		}
		
		if(w == 0) w = 200;

		w += MENU_CHILD_INDENT;
	}
}