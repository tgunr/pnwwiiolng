#include "phui-precompiled-header.h"

#include "phui/Slider.h"
#include "phui/GraphicsContext.h"
#include "phui/UIMessages.h"

#ifdef _WIN32
#define round(x) floor((x)+.5)
#endif

using namespace phui;

#define SLIDER_SIZE 10.0f
#define THUMB_HEIGHT 12.0f
#define THUMB_WIDTH 5.0f
 
/************************************************************************************************
	Slider
*************************************************************************************************/

Slider::Slider(): Control()
{
	_value.Convert(Scalar::FloatT);
	_pressed = false;
	_mousedown = false;
	_lmargin = _rmargin = 10;
	_ticks = 0;
	_minvalue = 0;
	_maxvalue = 1;
	_orient = ORIENT_HORIZONTAL;
	_thumbwidth = 10.0f;
	_thumbheight = 24.0f;
	_slideColor[0] = _slideColor[1] = _slideColor[2] = 0.0f;
	_thumbOnColor[0] = _thumbOnColor[1] = _thumbOnColor[2] = .2f;
	_thumbOffColor[0] = _thumbOffColor[1] = _thumbOffColor[2] = .8f;
	_slideColor[3] = _thumbOnColor[3] = _thumbOffColor[3] = 1.0f;
}

Slider::Slider(int orient): Control(), _orient(orient)
{
	_value.Convert(Scalar::FloatT);
	_pressed = false;
	_mousedown = false;
	_lmargin = _rmargin = 10;
	_ticks = 0;
	_minvalue = 0;
	_maxvalue = 1;
	
	if(_orient == ORIENT_VERTICAL)
	{
		_thumbwidth = 24.0f;
		_thumbheight = 10.0f;
	}
	else
	{
		_thumbwidth = 10.0f;
		_thumbheight = 24.0f;
	}

	_slideColor[0] = _slideColor[1] = _slideColor[2] = 0.0f;
	_thumbOnColor[0] = _thumbOnColor[1] = _thumbOnColor[2] = .2f;
	_thumbOffColor[0] = _thumbOffColor[1] = _thumbOffColor[2] = .8f;
	_slideColor[3] = _thumbOnColor[3] = _thumbOffColor[3] = 1.0f;
}

Slider::~Slider()
{
}

/************************************************************************************************
	DrawSelf
*************************************************************************************************/

void Slider::DrawSelf()
{ 
	Control::DrawSelf();
	if(DrawerEnabled()) return;
	
	GraphicsContext* ctx = GetGraphicsContext();

	float x,y;
	float box[8];
	float disabledColor[4];
	float* color = NULL;

	disabledColor[0] = disabledColor[1] = disabledColor[2] = disabledColor[3] = .8f;
	
	ctx->DisableTexture();
	
	if(_orient == ORIENT_VERTICAL)
	{
		box[0] = 0; box[1] = 0;
		box[2] = SLIDER_SIZE/2.0f; box[3] = 0;
		box[4] = SLIDER_SIZE/2.0f; box[5] = _height;
		box[6] = 0; box[7] = _height;
		
		x = (Width() - SLIDER_SIZE)/2.0f;
		y = 0;
		
		color = (_enabled) ? _thumbOnColor: disabledColor;
		
		ctx->SetColor(color[0],color[1],color[2],color[3]);
		ctx->MovePenPosition(x, y);	
		ctx->DrawQuad(box);

		color = (_enabled) ? _thumbOffColor: disabledColor;
		
		ctx->SetColor(color[0],color[1],color[2],color[3]);
		ctx->DrawLineStrip(4, box, true);
		ctx->MovePenPosition(-x, -y);

		if(_ticks > 1)
		{
			float line[4];
			float inc = _height/(_ticks - 1);
			line[1] = 0;
			line[2] = _width;
			
			for(int i = 0; i < _ticks; i++)
			{
				line[0] = line[2] = i*inc;
				ctx->DrawLineStrip(2, line, false);
			}
		}
	}
	else
	{
		box[0] = 0; box[1] = 0;
		box[2] = _width; box[3] = 0;
		box[4] = _width; box[5] = SLIDER_SIZE/2.0;
		box[6] = 0; box[7] = SLIDER_SIZE/2.0;

		x = 0;
		y = (Height() - SLIDER_SIZE)/2.0f;
		
		color = (_enabled) ? _thumbOnColor: disabledColor;

		ctx->SetColor(color[0],color[1],color[2],color[3]);
		ctx->MovePenPosition(x, y);	
		ctx->DrawQuad(box);

		color = (_enabled) ? _thumbOffColor: disabledColor;
		
		ctx->SetColor(color[0],color[1],color[2],color[3]);
		ctx->DrawLineStrip(4, box, true);
		ctx->MovePenPosition(-x, -y);

		if(_ticks > 1)
		{
			float line[4];
			float inc = Width()/(_ticks - 1);
			line[1] = 0;
			line[2] = Height();
			
			for(int i = 0; i < _ticks; i++)
			{
				line[0] = line[2] = i*inc;
				ctx->DrawLineStrip(2, line, false);
			}
		}
	}

	ThumbBox(box);

	color = (!_enabled) ? disabledColor: (_pressed) ? _thumbOnColor: _thumbOffColor;
	
	ctx->SetColor(color[0],color[1],color[2],color[3]);
	ctx->DrawQuad(box);

	color = (_enabled) ? _thumbOffColor: disabledColor;

	ctx->SetColor(color[0],color[1],color[2],color[3]);
	ctx->DrawLineStrip(4, box, true);
}

/************************************************************************************************
	PreferredSize
*************************************************************************************************/

void Slider::PreferredSize(float* w, float* h)
{	
	if(_preferredwidth < 1 || _preferredheight < 1)
	{
		*w = 200; 
		*h = _thumbheight;
	}
	else
	{
		*w = _preferredwidth;
		*h = _preferredheight;
	}
}

/************************************************************************************************
	MouseDown
*************************************************************************************************/

bool Slider::MouseDown (float x, float y, int button)
{
	if(_enabled)
	{
		float box[8];
		ThumbBox(box);
		
		if(x >= box[0] && y >= box[1] && x < box[4] && y < box[5])
			_pressed = true;
		
		_mousedown = true;
	}
	
	return true;
}

/************************************************************************************************
	MouseUp
*************************************************************************************************/

bool Slider::MouseUp   (float x, float y, int button)
{
	if(_enabled)
	{
		if(!_pressed)
		{
			if(_orient == ORIENT_VERTICAL)
				_value = y / (Height() - _thumbheight) * (_maxvalue - _minvalue) + _minvalue;
			else
				_value = x / (Width() - _thumbwidth) * (_maxvalue - _minvalue) + _minvalue;
			
			if(_ticks > 1)
			{
				float inc = (_maxvalue - _minvalue)/(_ticks - 1);
				_value = round(_value.Float()/inc) * inc;
			}

			if(_value.Float() < _minvalue)
				_value = _minvalue;
			else if(_value.Float() > _maxvalue)
				_value = _maxvalue;
		}
		
		if(_mousedown)
			Invoke();
	}
	
	_mousedown = false;
	_pressed = false;
	
	return true;
}

/************************************************************************************************
	MouseDrag
*************************************************************************************************/

bool Slider::MouseDrag (float x, float y, int button, bool start)
{
	if(_enabled && _pressed)
	{
		if(_orient == ORIENT_VERTICAL)
			_value = y / (Height() - _thumbheight) * (_maxvalue - _minvalue) + _minvalue;
		else
			_value = x / (Width() - _thumbwidth) * (_maxvalue - _minvalue) + _minvalue;
		
		if(_ticks > 1)
		{
			float inc = (_maxvalue - _minvalue)/(_ticks - 1);
			_value = round(_value.Float()/inc) * inc;
		}

		if(_value.Float() < _minvalue)
			_value = _minvalue;
		else if(_value.Float() > _maxvalue)
			_value = _maxvalue;
	}
	
	return true;
}

/************************************************************************************************
	ThumbBox
*************************************************************************************************/

void Slider::ThumbBox(float box[8]) const
{
	float x, y;
	float value = _value.Float();
	if(value > _maxvalue) value = _maxvalue;
	if(value < _minvalue) value = _minvalue;
	
	if(_orient == ORIENT_VERTICAL)
	{
		x = 0;
		y = ((value - _minvalue) / (_maxvalue - _minvalue) * (Height() - _thumbheight));
		
	}
	else
	{
		x = (value - _minvalue) / (_maxvalue - _minvalue) * (Width() - _thumbwidth);
		y = (Height() - _thumbheight)/2.0f;
	}
	
	box[0] = x; box[1] = y;
	box[2] = x+_thumbwidth; box[3] = y;
	box[4] = x+_thumbwidth; box[5] = y+_thumbheight;
	box[6] = x; box[7] = y+_thumbheight;
}

/************************************************************************************************
	Color Setters
*************************************************************************************************/

void Slider::SetSlideColor(float r, float g, float b, float a)
{
	_slideColor[0] = r;
	_slideColor[1] = g;
	_slideColor[2] = b;
	_slideColor[3] = a;
}

void Slider::SetThumbOnColor(float r, float g, float b, float a)
{
	_thumbOnColor[0] = r;
	_thumbOnColor[1] = g;
	_thumbOnColor[2] = b;
	_thumbOnColor[3] = a;
}

void Slider::SetThumbOffColor(float r, float g, float b, float a)
{
	_thumbOffColor[0] = r;
	_thumbOffColor[1] = g;
	_thumbOffColor[2] = b;
	_thumbOffColor[3] = a;
}

void Slider::GetThumbSize(float* w, float* h)
{
	if(w) *w = _thumbwidth;
	if(h) *h = _thumbheight;
}

void Slider::SetThumbSize(float* w, float* h)
{
	if(w && *w > 0) _thumbwidth = *w;
	else _thumbwidth = 7; // Default to prevent thumbox disappearing
	if(h && *h > 0) _thumbheight = *h;
	else _thumbheight = 12; // Default to prevent thumbox disappearing
}

