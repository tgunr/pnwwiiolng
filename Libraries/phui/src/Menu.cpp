#include "phui-precompiled-header.h"

#include <vector>

#include "phui/Menu.h"
#include "phui/GraphicsContext.h"
#include "phui/Viewport.h"
#include "phui/UIMessages.h"
#include "phui/Util.h"

#include <assert.h>

using namespace phui;

#define YMARGIN 0
#define XMARGIN 3

class Menu::MenuItemList : public std::vector<MenuItem*> {};

/************************************************************************************************
	MenuItem
*************************************************************************************************/

/************************************************************************************************
	Draw 
*************************************************************************************************/

void MenuItem::Draw()
{
	Menu* menu = ParentMenu();
	if(menu == nullptr) return;
	
	GraphicsContext* ctx = menu->GetGraphicsContext();
	
	if(_selected)
	{
		float w,h;
		h = ctx->FontHeight(_font);
		w = _parent->Width();
		
		BoxVerts box;
		phui::box_verts_with_pos(box,0,0,w,h);

		ctx->SetColor(.2f,.2f,.2f,1.0f);
		ctx->DisableTexture();
		ctx->DrawQuad(box.verts);
	}
	
	ctx->SetColor(0,0,0,1);
	ctx->EnableTexture();
	ctx->MovePenPosition(XMARGIN, 0);
	ctx->DrawString(_font, _label.c_str());
	ctx->MovePenPosition(-XMARGIN, 0);
}

/************************************************************************************************
	SetLabel
*************************************************************************************************/

void MenuItem::SetLabel(const char* label)
{
	_label.erase();
	for(int i = 0; label[i]; i++)
		_label += label[i];
}

/************************************************************************************************
	PreferredSize
*************************************************************************************************/

void MenuItem::PreferredSize(float* w, float* h)
{
	*w = 0;
	*h = 0;
	Menu* menu = ParentMenu();
	if(menu == nullptr) return;
	
	GraphicsContext* ctx = menu->GetGraphicsContext();
	
	*h = ctx->FontHeight(_font);
	*w = ctx->FontAdvance(_font, _label.c_str()) + 2 * XMARGIN;
}

/************************************************************************************************
	Menu
*************************************************************************************************/

Menu::Menu() : View(), _items(new MenuItemList), _ownercontrol(nullptr), _selecteditem(-1)
{
}

Menu::~Menu()
{
	MenuItemList::iterator it = _items->begin();
	for(; it != _items->end(); ++it)
	{
		delete *it;
	}
	
	delete _items;
}

/************************************************************************************************
	DrawSelf
*************************************************************************************************/

void Menu::DrawSelf()
{
	if(_drawer == 0)
	{
		View::DrawSelf();
		
		GraphicsContext* const ctx = GetGraphicsContext();
		
		BoxVerts box;
		phui::box_verts_with_pos(box,0,0,_width,_height);

		ctx->DisableTexture();
		
		ctx->SetColor(.8f,.8f,.8f,1.0f);
		ctx->DrawQuad(box.verts);
		
		ctx->SetColor(0,0,0,1);
		ctx->DrawLineStrip(4, box.verts, true);
		
		ctx->MovePenPosition(0,_height);
		
		MenuItemList::iterator it = _items->begin();
		for(; it != _items->end(); ++it)
		{
			MenuItem* const item = *it;
			float iw, ih;
			item->PreferredSize(&iw, &ih);
			ctx->MovePenPosition(0, -ih);
			item->Draw();
		}		
	}
}

/************************************************************************************************
	AddItem
*************************************************************************************************/

int Menu::AddItem(MenuItem* item)
{
	assert( item != nullptr );

	if(item->ParentMenu() != nullptr) return -1;
	
	_items->push_back(item);
	item->_parent = this;
	item->_font = _font;
	
	return 0;
}

/************************************************************************************************
	MouseDown
*************************************************************************************************/

bool Menu::MouseDown (float x, float y, int button)
{
	int idx = FindItemIndex(y);
	
	if(_ownercontrol && idx > -1)
		_ownercontrol->SetValue(idx);
		
	ParentViewport()->SetActiveMenu(nullptr, nullptr);
	
	return true;
}

/************************************************************************************************
	MouseUp
*************************************************************************************************/

bool Menu::MouseUp   (float x, float y, int button)
{
	return true;
}

/************************************************************************************************
	MouseMove
*************************************************************************************************/

bool Menu::MouseMove (float x, float y, int state )
{
	SelectItem(state == MOUSE_EXIT ? -1 : FindItemIndex(y));
	
	return View::MouseMove(x, y, state);
}

/************************************************************************************************
	ItemCount
*************************************************************************************************/

unsigned int Menu::ItemCount() const
{
	return (unsigned int)_items->size();
}

/************************************************************************************************
	ItemAt
*************************************************************************************************/

MenuItem* Menu::ItemAt(unsigned int i) const
{
	if(i >= _items->size()) return nullptr;
	
	return (*_items)[i];
}

/************************************************************************************************
	PreferredSize
*************************************************************************************************/

void Menu::PreferredSize(float* w, float* h)
{
	MenuItem* item;
	
	*w = 0;
	*h = 0;
	for(unsigned int i = 0; (item = ItemAt(i)); i++)
	{
		float iw,ih;
		item->PreferredSize(&iw, &ih);
		*h += ih;
		if(iw > *w) 
			*w = iw;
	}
}

/************************************************************************************************
	ShowAt
*************************************************************************************************/

void Menu::ShowAt(float x, float y, unsigned int idx, View* ctl)
{
	//MenuItem* item;
	
	_ownercontrol = ctl;
	SelectItem(idx);
	
	y -= Height();
	
	for(unsigned int i = 0; i <= idx; i++)
	{
		MenuItem* item = ItemAt(i);
		if(!item) break;

		float w = 0, h = 0;
		item->PreferredSize(&w, &h);
		y += h;
	}
	
	MoveTo(x, y);
	Show();
}

/************************************************************************************************
	FindItemIndex
*************************************************************************************************/

int Menu::FindItemIndex(float my)
{
	MenuItem* item;
	
	float y = Height();
	
	for(unsigned int i = 0; (item = ItemAt(i)); i++)
	{
		float w,h;
		item->PreferredSize(&w, &h);
		if(my >= y -h && my < y) return i;
		y -= h;
	}
	
	return -1;
}

/************************************************************************************************
	SelectItem
*************************************************************************************************/

void Menu::SelectItem(int idx)
{
	MenuItem* item;
	if(_selecteditem != idx)
	{
		item = ItemAt(_selecteditem);
		if(item)
			item->SetSelected(false);				
			
		_selecteditem = idx;
		item = ItemAt(idx);
		if(item)
			item->SetSelected(true);
	}
}

