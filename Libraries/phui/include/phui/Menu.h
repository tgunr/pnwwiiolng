#ifndef PHUI_MENU_H
#define PHUI_MENU_H

#include <string>

#include "phui/View.h"
#include "phui/Invoker.h"

namespace phui
{

class Menu;

/************************************************************************************************
	MenuItem
*************************************************************************************************/
/**
	Menu Item
*/
class MenuItem : public Invoker
{
public:
	MenuItem() : _parent(0ULL), _font(0), _selected(0), _label() {}
	virtual ~MenuItem() {}
	
	virtual void Draw();
	
	void SetLabel(const wchar_t* const label) { _label = label; }
	void SetLabel(const char*);
	
	const wchar_t* Label() const { return _label.c_str(); }
	
	Menu* ParentMenu() const { return _parent; }
	 
	virtual void PreferredSize(float* w, float* h);
	
	void SetSelected(bool s) { _selected = s;}
	
protected:
friend class Menu;
	Menu* _parent;
	int _font;
	int _selected;
	
private:
	std::wstring _label;
};

/************************************************************************************************
	Menu
*************************************************************************************************/
/**
	Menu
*/
class Menu : public View
{
public:
	Menu();
	virtual ~Menu(); 
	
	virtual void DrawSelf();
	
	int AddItem(MenuItem*);
	
	MenuItem* ItemAt(unsigned int) const;
	
	virtual bool MouseDown (float x, float y, int button);
	virtual bool MouseUp   (float x, float y, int button);
	virtual bool MouseMove (float x, float y, int state );
	
	unsigned int ItemCount() const;
	
	virtual void PreferredSize(float* w, float* h);
	virtual void MinSize(float* w, float* h) { PreferredSize(w,h); }
	
	void ShowAt(float x, float y, unsigned int item = 0, View* ctl = 0); // offsets to item
		
private:

	int FindItemIndex(float y);
	void SelectItem(int);
	
	class MenuItemList;
	MenuItemList* _items;
	View* _ownercontrol; // sets value of this view when item executes
	int _selecteditem;
};

}
#endif // PHUI_MENU_H
