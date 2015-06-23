#ifndef PHUI_CONTEXTMENU_H
#define PHUI_CONTEXTMENU_H

#include "phui/Caption.h"
#include <map>

namespace phui
{

/************************************************************************************************
	ContextMenu
*************************************************************************************************/

class ContextMenu;
typedef std::vector<ContextMenu*> MenuItems;

class ContextMenu : public Caption
{
public:
	ContextMenu();
	virtual ~ContextMenu(); 
	
	virtual void DrawSelf();
	virtual void Show();
	virtual bool MouseUp(float x, float y, int button);
	virtual bool MouseMove(float x, float y, int state );
	virtual int AddChild(View* view, View* before=0);
	virtual void MoveTo(float x, float y);

	void ShowChildren();
	void HideChildren();
	void HideGrandChildren();
	void ShowMenu();
	void HideMenu();
	
	bool IsOpen(){ return _open; }

	ContextMenu* FindMenuHit(float x, float y) const;

	void SetHIndent(float hindent){ _hindent = hindent; }
	float GetHIndent(){ return _hindent; }
	void SetVIndent(float vindent){ _vindent = vindent; }
	float GetVIndent(){ return _vindent; }
	
	void SetRootMenu(ContextMenu* root){ _rootmenu = root; _isWindow = true; }
	void SetParentMenu(ContextMenu* parent){ _parentmenu = parent; }
	void SetLastOverMenu(ContextMenu* lastover){ _lastovermenu = lastover; }
	ContextMenu* GetRootMenu(){ return (_rootmenu) ? _rootmenu: (ContextMenu*)this; }
	ContextMenu* GetParentMenu(){ return (_parentmenu) ? dynamic_cast<ContextMenu*>(_parentmenu): nullptr; }
	ContextMenu* GetLastOverMenu(){ return _lastovermenu; }
	MenuItems* GetSubMenus(){ return _submenus; }
	
	bool IsRootMenu() const { return (!_rootmenu) ? true: false; }
	
	void PositionSubMenus();
	void PositionSubMenus(float x, float y);
	void SetNextState(bool n);
	void ClearNextState(){ _nextstatetime = 0; }
	
	int AddSubMenu(ContextMenu* menu, bool isparent = true);
	
protected:
	
	unsigned int _nextstatetime;
	bool _open, _nextstate;
	ContextMenu* _rootmenu;
	ContextMenu* _parentmenu;
	ContextMenu* _lastovermenu;
	MenuItems* _submenus;
	float _hindent, _vindent, _menux, _menuy;
};

}
#endif // PHUI_CONTEXTMENU_H
