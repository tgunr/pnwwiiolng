#ifndef PHUI_VIEWPORT_H
#define PHUI_VIEWPORT_H

#include "phui/View.h"
#include "phui/TextView.h"

namespace phui
{

class GraphicsContext;

/************************************************************************************************
	Viewport
*************************************************************************************************/
/** 
	Viewport is the top level container and message dispatcher for Views 
*/
class Viewport : public View
{
public:

	typedef void (*FocusCallback)(bool focus);

	enum
	{
		MODIFIER_NONE		= 0,
		MODIFIER_SHIFT		= 1<<0,
		MODIFIER_CTRL		= 1<<1,
		MODIFIER_ALT			= 1<<2
	};

	Viewport(GraphicsContext*, float x, float y, float width, float height);
	virtual ~Viewport();
	
	virtual void DrawSelf();
	
// use these functions to push UI events to views, returns if it will handle the events
	bool PostMouseDown(float x, float y, int button);
	bool PostMouseUp  (float x, float y, int button);
	bool PostMouseMove(float x, float y);
	bool PostMouseDrag (float x, float y, int button);
	bool PostMouseScroll(float x, float y, int dir);
	
	bool PostKeyDown(int keycode, unsigned short unicode);
	bool PostKeyUp  (int keycode, unsigned short unicode);
	
	void SetActiveMenu(View*, View*); ///< Adds as child to window, removes child if clicks outside
	bool SetDefaultFont(unsigned int size);
	bool SetDefaultFont(const char* name, unsigned int size);
	
	void SetNextFocus();
	void SetFocusCallback(FocusCallback cb);
	void OnFocusModified(bool focus);

	void SetToolTips(bool on){ _dotooltips = on; } // set whether tooltips are on
	bool GetToolTips(){ return _dotooltips; } // are tooltips on?
	void ShowToolTip(float x, float y, View *v);
	void HideToolTip();
	void SetToolTipMode(int mode); // set whether the view's name is displayed (1) or the view's tooltip if it has one (0)
	int GetToolTipMode();

private:
	View* _lastclickview;
	View* _lastdragview;
	View* _activemenu;
	View* _activemenuview;
	unsigned int _tooltipstart;
	TextView* _tooltipview;
	bool _dotooltips;
	int _tooltipmode;
	std::vector<View*> _lastoverviews;
	int _modifiers;
	FocusCallback _focusCallback;
};
}
#endif // PHUI_VIEWPORT_H
