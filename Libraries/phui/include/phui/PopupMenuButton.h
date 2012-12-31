#ifndef PHUI_POPUPMENUBUTTON_H
#define PHUI_POPUPMENUBUTTON_H

#include "phui/Control.h"

namespace phui
{

class Menu;

/************************************************************************************************
	PopupMenuButton
*************************************************************************************************/
/**
	Menu Item
*/
class PopupMenuButton : public Control
{
public:
	PopupMenuButton();
	virtual ~PopupMenuButton(); 

	Menu* MenuList() const { return _menu; }
	
	virtual void DrawSelf();
	
	virtual bool MouseDown (float x, float y, int button);
	virtual bool MouseUp   (float x, float y, int button);
	virtual bool MouseMove (float x, float y, int state );

	virtual void PreferredSize(float* w, float* h);
	virtual void MinSize(float* w, float* h) { PreferredSize(w,h); }

protected:
	Menu* _menu;
};

}
#endif // PHUI_POPUPMENUBUTTON_H
