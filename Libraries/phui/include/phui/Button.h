#pragma once

#include "phui/Control.h"

namespace phui
{

/************************************************************************************************
	Control
*************************************************************************************************/
/**
	A Button!
*/
class Button : public Control
{
public:
	Button() : Control(), _pressed(false), _mousedown(false), _togglemode(false)
	{
		_lmargin = _rmargin = 10;
	}
	virtual ~Button() {}
		
	virtual void DrawSelf();
	
	virtual bool MouseDown (float x, float y, int button);
	virtual bool MouseUp   (float x, float y, int button);
	virtual bool MouseMove (float x, float y, int state );
	
	virtual void PreferredSize(float* w, float* h);
	virtual void MinSize(float* w, float* h) { PreferredSize(w,h); }
	
	bool Pressed() const { return _pressed; }
	void SetToggle(bool mode=true) { _togglemode = mode; }
	bool Toggles() const { return _togglemode; }
	
protected:
friend class ViewDrawer;
	bool _pressed;
	bool _mousedown;
	bool _togglemode;
};

}
