#include "phui-precompiled-header.h"

#include <wchar.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "phui/Checkbox.h"
#include "phui/GraphicsContext.h"
#include "phui/UIMessages.h"

using namespace phui;

#define BOXSIZE 16

/************************************************************************************************
	Checkbox
*************************************************************************************************/

Checkbox::Checkbox(): Control()
{
	_value.Convert(Scalar::BoolT);
	_pressed = _mousedown = false;
	_lmargin = _rmargin = 3;
}

Checkbox::~Checkbox()
{
}

/************************************************************************************************
	DrawSelf
*************************************************************************************************/

void Checkbox::DrawSelf()
{ 
	Control::DrawSelf();

	if(DrawerEnabled()) return;

	GraphicsContext* ctx = GetGraphicsContext();

	float fontheight = ctx->FontHeight(_font);
	float x = BOXSIZE + (_lmargin+_rmargin);
	float y = (_height - fontheight) / 2.0f;
	
	ctx->SetColor(_textcolor[0],_textcolor[1],_textcolor[2],_textcolor[3]);
	ctx->EnableTexture();
	ctx->MovePenPosition(x, y);
	ctx->DrawString(_font, _label.String());
	ctx->MovePenPosition(-x, -y);
	
	x = _lmargin;
	y = (_height - BOXSIZE) / 2.0f;
	ctx->DisableTexture();
	ctx->MovePenPosition(x, y);
	
	float box[8];
	box[0] = 0; box[1] = 0;
	box[2] = BOXSIZE; box[3] = 0;
	box[4] = BOXSIZE; box[5] = BOXSIZE;
	box[6] = 0; box[7] = BOXSIZE;
	
	if(_pressed)
		ctx->SetColor(.2f,.2f,.2f,1.0f);
	else
		ctx->SetColor(.8f,.8f,.8f,1.0f);
	ctx->DrawQuad(box);
	
	if(_value)
	{
		ctx->SetColor(0,0,0,1);
		float xmark[4];
		xmark[0] = 0; xmark[1] = BOXSIZE; 
		xmark[2] = BOXSIZE; xmark[3] = 0; 
		ctx->DrawLineStrip(2, xmark, false);

		xmark[0] = 0; xmark[1] = 0; 
		xmark[2] = BOXSIZE; xmark[3] = BOXSIZE; 
		ctx->DrawLineStrip(2, xmark, false);

	}

	ctx->SetColor(0,0,0,1);
	ctx->DrawLineStrip(4, box, true);
}

/************************************************************************************************
	PreferredSize
*************************************************************************************************/

void Checkbox::PreferredSize(float* w, float* h)
{
	if(_preferredwidth < 1 || _preferredheight < 1)
	{
		GraphicsContext* ctx = GetGraphicsContext();
		
		float fontheight = ctx->FontHeight(_font);
		
		*w = ctx->FontAdvance(_font, _label.String()) + BOXSIZE + 3*_lmargin;
		*h = fontheight + (_tmargin+_bmargin);
	}else
	{
		*w = _preferredwidth;
		*h = _preferredheight;
	}

}

/************************************************************************************************
	MouseDown
*************************************************************************************************/

bool Checkbox::MouseDown (float x, float y, int button)
{
	if(_enabled)
	{
		_mousedown = true;
		_pressed = true;
	}
	
	return true;
}

/************************************************************************************************
	MouseUp.
*************************************************************************************************/

bool Checkbox::MouseUp   (float x, float y, int button)
{
	if(_pressed)
	{
		_value = (_value.Bool()) ? false : true;
		Invoke();
	}
	
	_mousedown = false;
	_pressed = false;
	
	return true;
}

/************************************************************************************************
	MouseMove
*************************************************************************************************/

bool Checkbox::MouseMove (float x, float y, int state )
{
	if(state == MOUSE_ENTER)
		_pressed = _mousedown;
	else if(state == MOUSE_EXIT)
		_pressed = false;
	
	return View::MouseMove(x, y, state);
}

