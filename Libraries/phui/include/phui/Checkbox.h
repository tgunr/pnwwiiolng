#ifndef PHUI_CHECKBOX_H
#define PHUI_CHECKBOX_H

#include "phui/Control.h"

namespace phui
{

/************************************************************************************************
	Control
*************************************************************************************************/
/**
	A Checkbox!
*/
class Checkbox : public Control
{
public:
	Checkbox();
	virtual ~Checkbox(); 

	virtual void DrawSelf();

	virtual bool MouseDown (float x, float y, int button);
	virtual bool MouseUp   (float x, float y, int button);
	virtual bool MouseMove (float x, float y, int state );
	
	virtual void PreferredSize(float* w, float* h);
	virtual void MinSize(float* w, float* h) { PreferredSize(w,h); }
	
	bool Pressed() const { return _pressed; }
	
protected:
friend class ViewDrawer;
	bool _pressed;
	bool _mousedown;
};

}
#endif // PHUI_CHECKBOX_H
