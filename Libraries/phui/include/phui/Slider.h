#ifndef PHUI_SLIDER_H
#define PHUI_SLIDER_H

#include "phui/Control.h"

namespace phui
{

/************************************************************************************************
	Control
*************************************************************************************************/
/**
	A Button!
*/
class Slider : public Control
{
public:
	enum { ORIENT_VERTICAL, ORIENT_HORIZONTAL };

	Slider();
	Slider(int orient);
	virtual ~Slider(); 
		
	virtual void DrawSelf();
	
	virtual bool MouseDown (float x, float y, int button);
	virtual bool MouseUp   (float x, float y, int button);
	virtual bool MouseDrag (float x, float y, int button, bool start);
	
	virtual void PreferredSize(float* w, float* h);
	virtual void MinSize(float* w, float* h) { PreferredSize(w,h); }
	
	void SetMaxValue(float v) { _maxvalue = v; }
	void SetMinValue(float v) { _minvalue = v; }
	void SetTicks(int t) { _ticks = t; }
	
	void SetSlideColor(float r, float g, float b, float a);
	void SetThumbOnColor(float r, float g, float b, float a);
	void SetThumbOffColor(float r, float g, float b, float a);

	void GetThumbSize(float* w, float* h);
	void SetThumbSize(float* w, float* h);

protected:
	void ThumbBox(float box[8]) const;

	bool _pressed;
	bool _mousedown;
	int _ticks, _orient;
	float _minvalue, _maxvalue;
	float _thumbwidth, _thumbheight;
	float _slideColor[4];
	float _thumbOnColor[4];
	float _thumbOffColor[4];
};

}
#endif // PHUI_SLIDER_H
