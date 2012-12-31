#ifndef PHUI_VIEWDRAWER_H
#define PHUI_VIEWDRAWER_H


namespace phui
{

class View;
class Button;

/************************************************************************************************
	ViewDrawer
*************************************************************************************************/
/**
	This is the interface to implement drawing of various views and controls. The views have a very
	simple default look which can be overridden by assigning a ViewDrawer implementation to them.
*/

class ViewDrawer
{
public:
	ViewDrawer(View* v): _enabled(false), _view(v) {}
	virtual ~ViewDrawer() {}
	
	virtual void Draw() = 0;
	bool Enabled() const { return _enabled; }
	
protected:
	bool _enabled;
	View* _view;
};

/// Base drawer factory for creating ViewDrawer's
class ViewDrawerFactory
{
public:
	ViewDrawerFactory() {}
	virtual ~ViewDrawerFactory() {}
	
	virtual ViewDrawer* CreateDrawer(View*) = 0;
};
}
#endif // PHUI_VIEWDRAWER_H
