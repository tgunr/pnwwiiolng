#ifndef PHUI_CTXMENU_H
#define PHUI_CTXMENU_H

#include "phui/Caption.h"
#include "phui/Referenced.h"
#include <map>

namespace phui
{

/************************************************************************************************
	CtxMenu
*************************************************************************************************/

class CtxMenu : public Caption
{
public:

	typedef std::vector<CtxMenu*> MenuItems;
	
	CtxMenu();
	virtual ~CtxMenu(); 
	
	virtual void DrawSelf();
	virtual void Show();
	virtual bool MouseUp(float x, float y, int button);
	virtual bool MouseMove(float x, float y, int state );
	virtual int AddChild(View* view, View* before=0);
	virtual void Hide();
	virtual void Close();

	void ShowChildren();
	void HideChildren();
	void ShowMenu();
	void HideMenu();
	
	bool IsOpen(){ return _open; }

	void SetHIndent(float hindent){ _hindent = hindent; }
	float GetHIndent(){ return _hindent; }
	void SetVIndent(float vindent){ _vindent = vindent; }
	float GetVIndent(){ return _vindent; }
	
	void SetParentMenu(CtxMenu* parent){ _parentmenu = parent; }
	CtxMenu* GetRootMenu();
	CtxMenu* GetParentMenu(){ return (_parentmenu) ? dynamic_cast<CtxMenu*>(_parentmenu): NULL; }
	MenuItems* GetSubMenus(){ return _submenus; }
	void SetMouseOver(unsigned int o);
	unsigned int IsMouseOver(){ return _mouseover; }
	bool IsRootMenu() const;
	
	void PositionSubMenus();
	void PositionSubMenus(float x, float y);
	
	void GetBranchDimensions(float& w, float& h);

	int AddSubMenu(CtxMenu* menu, bool isparent = true);

	void SetIcons(int icons){ _icons = icons; }
	int GetIcons(){ return _icons; }
		
protected:
	
	bool _open, _tooltips;
	unsigned int _mouseover, _mousestate, _expandmode;
	CtxMenu* _parentmenu;
	MenuItems* _submenus;
	float _hindent, _vindent;
	int _icons;
};

}
#endif // PHUI_CTXMENU_H
