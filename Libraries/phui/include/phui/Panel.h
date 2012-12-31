#ifndef PHUI_PANEL_H
#define PHUI_PANEL_H

#include "phui/View.h"

namespace phui
{

/************************************************************************************************
	Caption
*************************************************************************************************/
/**
	View for displaying a single line of text.
*/
class Panel : public View
{
public:
	Panel();
	virtual ~Panel();
	
	virtual void DrawSelf();

	virtual void PreferredSize(float* w, float* h);
	virtual void MinSize(float* w, float* h) { PreferredSize(w,h); }
	
	void SetTextAlignment(int alignment) { _alignment = alignment; }

enum
{
	ALIGN_LEFT, ALIGN_CENTER, ALIGN_RIGHT
};

protected:
	int _alignment;
	float _panelmargin;
	
};

}
#endif // PHUI_PANEL_H
