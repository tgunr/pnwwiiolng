#include "phui-precompiled-header.h"

#include "phui/Button.h"
#include "phui/GraphicsContext.h"
#include "phui/UIMessages.h"
#include "phui/Util.h"

using namespace phui;

/************************************************************************************************
	Button
*************************************************************************************************/

/************************************************************************************************
	DrawSelf
*************************************************************************************************/

void Button::DrawSelf()
{ 
	Control::DrawSelf();

	if(DrawerEnabled()) return;
			
	GraphicsContext* ctx = GetGraphicsContext();

	BoxVerts box;
	box_verts_with_pos(box, 0,0, _width, _height);

	float fontheight = ctx->FontHeight(_font);
	float x = (_width - ctx->FontAdvance(_font, _label.String())) / 2.0f;
	float y = (_height - fontheight) / 2.0f;
	
	ctx->DisableTexture();
	
	if(_pressed || (_togglemode && GetValue().Bool()))
		ctx->SetColor(.2f,.2f,.2f,1.0f);
	else
		ctx->SetColor(.8f,.8f,.8f,1.0f);
	ctx->DrawQuad(box.verts);
	
	if(_enabled)
		ctx->SetColor(0,0,0,1);
	else
		ctx->SetColor(.6f,.6f,.6f,1.0f);
		
	ctx->DrawLineStrip(4, box.verts, true);

	if(_enabled)
		ctx->SetColor(_textcolor[0],_textcolor[1],_textcolor[2],_textcolor[3]);

	ctx->EnableTexture();
	ctx->MovePenPosition(x, y);
	ctx->DrawString(_font, _label.String());
}

/************************************************************************************************
	PreferredSize
*************************************************************************************************/

void Button::PreferredSize(float* w, float* h)
{
	if(_preferredwidth < 1 || _preferredheight < 1)
	{
		GraphicsContext* ctx = GetGraphicsContext();
	
		*w = ctx->FontAdvance(_font, _label.String()) + (_lmargin+_rmargin); 
		*h = ctx->FontHeight(_font) +(_tmargin+_bmargin);
	}else
	{
		*w = _preferredwidth;
		*h = _preferredheight;
	}
}

/************************************************************************************************
	MouseDown
*************************************************************************************************/

bool Button::MouseDown (float x, float y, int button)
{
	if(_enabled)
	{
		_mousedown = true;
		_pressed = true;
	}
	
	return true;
}

/************************************************************************************************
	MouseUp
*************************************************************************************************/

bool Button::MouseUp   (float x, float y, int button)
{
	if(_mousedown && _pressed)
	{
		if(_togglemode)
			SetValue(!GetValue().Bool());

		Invoke();
	}
	
	_mousedown = false;
	_pressed = false;
	
	return true;
}

/************************************************************************************************
	MouseMove
*************************************************************************************************/

bool Button::MouseMove (float x, float y, int state )
{
	if(state == MOUSE_ENTER)
		_pressed = _mousedown;
	else if(state == MOUSE_EXIT)
		_pressed = false;
	
	return View::MouseMove(x, y, state);
}
