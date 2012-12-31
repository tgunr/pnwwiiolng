#ifndef PHUI_SCROLLBAR_H
#define PHUI_SCROLLBAR_H

#include "phui/View.h"

namespace phui
{

/************************************************************************************************
	Scrollbar
*************************************************************************************************/

class Scrollbar : public View
{
public:
	enum { ORIENT_VERTICAL, ORIENT_HORIZONTAL };
	
	Scrollbar(int orient, float minbarsize);
	virtual ~Scrollbar(); 
	
	virtual void Draw();
	virtual void AttachedToWindow();
	virtual bool MouseDrag (float x, float y, int button, bool start);
	virtual bool MouseDown (float x, float y, int button);
	virtual bool MouseUp   (float x, float y, int button);

	void SetBar(int o, float mbs){ _orient = o; _minbarsize = mbs; }
	int Orientation() const { return _orient; }
	float MinBarSize() const { return _minbarsize; }
	
protected:
	int _orient;
	float _dragoffset, _minbarsize;
	bool _dragging;
};

}
#endif // PHUI_SCROLLBAR_H
