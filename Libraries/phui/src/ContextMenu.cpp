#include "phui-precompiled-header.h"

#include "phui/ContextMenu.h"
#include "phui/UIMessages.h"
#include "phui/GraphicsContext.h"
#include "phui/Message.h"
#include "phui/MessageLoop.h"

using namespace phui;

#define NEXTSTATE_DELAY 300

/************************************************************************************************
	Button
*************************************************************************************************/

ContextMenu::ContextMenu(): Caption()
{
	_rootmenu = _parentmenu = _lastovermenu = nullptr;
	_submenus = nullptr;
	_acceptsmouseclick = true;
	_hindent = _vindent = _menux = _menuy = 0;
	_isVisible = _isWindow = _open = _nextstate = false;
	_nextstatetime = 0;
}

ContextMenu::~ContextMenu()
{
	if(_submenus)
		delete _submenus;
}

void ContextMenu::DrawSelf()
{
	if(_enabled)
	{
		if(_nextstatetime && ParentLoop()->FrameTime() >= _nextstatetime)
		{
			if(_nextstate)
			{
				ShowChildren();
			}
			else
			{
				if(IsRootMenu())
					Hide();
				else
					HideChildren();
			}
			
			_nextstatetime = 0;
		}
		
		Caption::DrawSelf();
	}
}

void ContextMenu::Show()
{
	if(_enabled)
	{
		if(IsRootMenu())
			PositionSubMenus();
		
		ShowMenu();
		ShowChildren();
	}
}

int ContextMenu::AddChild(View* view, View* before)
{
	ContextMenu* menu = dynamic_cast<ContextMenu*>(view);
	
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

int ContextMenu::AddSubMenu(ContextMenu* menu, bool isparent)
{
	menu->Hide();
	menu->SetRootMenu((ContextMenu*)this);
	
	if(isparent)
		_submenus->push_back(menu);
	
	return Caption::AddChild((View*)menu);
}

/************************************************************************************************
	MouseUp
*************************************************************************************************/
 
bool ContextMenu::MouseUp(float x, float y, int button)
{
	bool handled = Caption::MouseUp(x, y, button);
	
	//GetRootMenu()->SetNextState(false);
	GetRootMenu()->Hide();
	
	return handled;
}

/************************************************************************************************
	MouseMove - this function probably needs to be reworked, its too complex
*************************************************************************************************/

bool ContextMenu::MouseMove(float x, float y, int state )
{
	if(!IsRootMenu())
	{
		ContextMenu* root = GetRootMenu();

		root->ClearNextState();
		
		if(state == MOUSE_ENTER || state == MOUSE_OVER)
		{
			ShowChildren();
			
			ContextMenu* parent = GetParentMenu();
			
			if(parent)
			{
				MenuItems* submenus = parent->GetSubMenus();
				MenuItems::iterator it = submenus->begin();
				
				for(; it != submenus->end(); ++it)
				{
					if((*it) != (ContextMenu*)this && (*it)->IsOpen())
						(*it)->HideChildren();
				}
			}
		}
		else if(state == MOUSE_EXIT)
		{
			float ax	= x + _x;
			float ay	= y + _y;
 			
			ContextMenu* hit = root->FindMenuHit(ax, ay);
			
			if(hit)
			{
				if(!FindMenuHit(ax, ay))
				{
					ContextMenu* parent = GetParentMenu();
					
					if(parent && parent != hit && _layer > hit->Layer())
						parent->HideChildren();
					else
						HideChildren();
				}
			}
			else
			{
				root->SetNextState(false);
			}
		}
	}
	
	return Caption::MouseMove(x, y, state);
}

/************************************************************************************************
	ShowChildren
*************************************************************************************************/

void ContextMenu::MoveTo(float x, float y)
{
	if(IsRootMenu())
	{
		if(x) _menux = x;
		if(y) _menuy = y;
		
		PositionSubMenus();
	}
	else
	{
		Caption::MoveTo(x, y);
	}
}

/************************************************************************************************
	ShowChildren
*************************************************************************************************/

void ContextMenu::ShowChildren()
{
	if(_enabled && _submenus)
	{
		for(unsigned int i = 0; i < _submenus->size(); ++i)
			(*_submenus)[i]->ShowMenu();

		_open = true;
	}
}

/************************************************************************************************
	HideChildren
*************************************************************************************************/

void ContextMenu::HideChildren()
{
	if(_submenus)
	{
		for(unsigned int i = 0; i < _submenus->size(); ++i)
			(*_submenus)[i]->HideMenu();

		_open = false;
	}
}

/************************************************************************************************
	HideGrandChildren
*************************************************************************************************/

void ContextMenu::HideGrandChildren()
{
	if(_submenus)
	{
		for(unsigned int i = 0; i < _submenus->size(); ++i)
			(*_submenus)[i]->HideChildren();
	}
}

/************************************************************************************************
	ShowMenu
*************************************************************************************************/

void ContextMenu::ShowMenu()
{
	if(_enabled)
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
}

/************************************************************************************************
	HideMenu
*************************************************************************************************/

void ContextMenu::HideMenu()
{
	Hide();
}

/************************************************************************************************
	FindMenuHit
*************************************************************************************************/

ContextMenu* ContextMenu::FindMenuHit(float x, float y) const
{
	if(_isVisible && _enabled)
	{
		if(_submenus)
		{
			ContextMenu* child;
			
			for(unsigned int i = 0; i < _submenus->size(); ++i)
			{
				child = (*_submenus)[i]->FindMenuHit(x, y);
				
				if(child)
					return child;
			}
		}
		
		if(!IsRootMenu() && x >= _x && y >= _y && x < _x + _width && y < _y + _height)
			return (ContextMenu*)this;
	}
	
	return nullptr;
}

/************************************************************************************************
	PositionSubMenus
*************************************************************************************************/

void ContextMenu::PositionSubMenus()
{
	if(_submenus && _submenus->size())
		PositionSubMenus(_menux, _menuy);
}

void ContextMenu::PositionSubMenus(float x, float y)
{
	if(_submenus)
	{
		bool init = false;

		for(unsigned int i = 0; i < _submenus->size(); ++i)
		{
			ContextMenu* menu = (*_submenus)[i];
			
			if(menu->Enabled())
			{
				if(init)
					y -= menu->Height();
				
				menu->MoveTo(x, y);
				menu->PositionSubMenus(x + menu->GetHIndent(), y + menu->GetVIndent());

				init = true;
			}
		}
	}
}

void ContextMenu::SetNextState(bool n)
{
	_nextstate = n;
	_nextstatetime = ParentLoop()->FrameTime() + NEXTSTATE_DELAY;
	
	if(!IsRootMenu())
		GetRootMenu()->ClearNextState();
}