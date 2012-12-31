#include "phui-precompiled-header.h"

#include <string.h>

#include "phui/Caption.h"
#include "phui/GraphicsContext.h"

using namespace phui;

/************************************************************************************************
	Caption
*************************************************************************************************/

Caption::Caption() : View()
{
	_value.Convert(Scalar::WideStringT);
	_textcolor[0] = _textcolor[1] = _textcolor[2] = 0;
	_textcolor[3] = 1;
	_font = 0;
	_alignment = ALIGN_LEFT;
	_lmargin=_rmargin = 3;
	_autoresize = false;
	_resize = false;
}

Caption::~Caption()
{
}

/************************************************************************************************
	Draw
*************************************************************************************************/

void Caption::Draw()
{
	if(_resize && _autoresize)
	{
		float w = 0.0f, h = 0.0f;
		PreferredSize(&w, &h);
		ResizeTo(w, h);
		_resize = false;
	}
	
	View::Draw();
}

/************************************************************************************************
	DrawSelf
*************************************************************************************************/

void Caption::DrawSelf()
{
	View::DrawSelf();

	if(DrawerEnabled()) return;

	GraphicsContext* ctx = GetGraphicsContext();
	
	ctx->SetColor(_textcolor[0],_textcolor[1],_textcolor[2],_textcolor[3]);
	ctx->EnableTexture();
	
	float x = _lmargin;
	float y = (Height() - ctx->FontHeight(_font))/2.0f;
	
	float textwidth = ctx->FontAdvance(_font, _value.WideString());
	
	if(_alignment == ALIGN_CENTER)
	{
		x = (Width() - textwidth) / 2.0f;
	}else if(_alignment == ALIGN_RIGHT)
	{
		x = Width() - _rmargin - textwidth;
	}
	
	ctx->MovePenPosition(x, y);
	ctx->DrawString(_font, _value.WideString());
}

void Caption::SetValue(const Scalar& v)
{
	View::SetValue(v);
	_resize = true;
}

/************************************************************************************************
	PreferredSize
*************************************************************************************************/

void Caption::PreferredSize(float* w, float* h)
{
	GraphicsContext* ctx = GetGraphicsContext();
	
	*w = ctx->FontAdvance(_font, _value.WideString()) + (_lmargin+_rmargin); 
	*h = ctx->FontHeight(_font) + (_tmargin+_bmargin);
}

