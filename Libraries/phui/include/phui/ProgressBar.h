#ifndef PHUI_PROGRESSBAR_H
#define PHUI_PROGRESSBAR_H

#include "phui/View.h"

namespace phui
{

/************************************************************************************************
	ProgressBar
*************************************************************************************************/

class ProgressBar : public View
{
public:
	ProgressBar();
	virtual ~ProgressBar(); 
		
	virtual void DrawSelf();

	void SetFillTexture(GraphicsContext::TextureInfo const& texid) { _fill = texid; }
	void SetBackgroundTexture(GraphicsContext::TextureInfo const& texid) { _back = texid; }
	
	void SetMinValue(float v) { _minvalue = v;}
	void SetMaxValue(float v) { _maxvalue = v;}
	
	enum { ORIENT_LEFT_RIGHT, ORIENT_RIGHT_LEFT, ORIENT_BOTTOM_TOP, ORIENT_TOP_BOTTOM};
	void SetOrientation(int o) { _orient = o;}
protected:
	int _orient;
	GraphicsContext::TextureInfo _fill, _back;
	float _minvalue, _maxvalue;
};

}
#endif // PHUI_PROGRESSBAR_H
