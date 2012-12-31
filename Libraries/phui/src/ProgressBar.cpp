#include "phui-precompiled-header.h"

#include "phui/GraphicsContext.h"
#include "phui/ProgressBar.h"

using namespace phui;

/************************************************************************************************
	ProgressBar
*************************************************************************************************/

ProgressBar::ProgressBar()
{
	_value.Convert(Scalar::FloatT);
	_minvalue = 0;
	_maxvalue = 1;
	_orient = ORIENT_LEFT_RIGHT;
}

ProgressBar::~ProgressBar()
{
}
	
/************************************************************************************************
	DrawSelf
*************************************************************************************************/

void ProgressBar::DrawSelf()
{
	GraphicsContext* ctx = GetGraphicsContext();
	ctx->SetColor(1,1,1,1);
	
	double val = GetValue();
	
	val = (val-_minvalue) /(_maxvalue-_minvalue);
	
	if(val < 0) val = 0;
	if(val > 1) val = 1;
	
	float left,bottom,right,top;
	
	switch(_orient)
	{
		case ORIENT_LEFT_RIGHT:
			left = 0; right = _width*(float)val;
			bottom = 0; top = _height;
		break;
		case ORIENT_RIGHT_LEFT:
			left = _width*(1-(float)val); right = _width;
			bottom = 0; top = _height;
		break;
		case ORIENT_BOTTOM_TOP:
			left = 0; right = _width;
			bottom = 0; top = _height*(float)val;
		break;
		case ORIENT_TOP_BOTTOM:
			left = 0; right = _width;
			bottom = _height*(1-(float)val); top = _height;
		break;
	}
	
	ctx->EnableTexture();
	ctx->DrawTexture(_back, 0,0,_width,_height);
	ctx->DrawTexture(_fill, left,bottom, right,top);
}