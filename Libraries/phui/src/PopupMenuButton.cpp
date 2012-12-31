#include "phui-precompiled-header.h"

#include <stdlib.h>
#include <stdio.h>

#include "phui/PopupMenuButton.h"
#include "phui/GraphicsContext.h"
#include "phui/Menu.h"
//#include "phui/Window.h"
#include "phui/Viewport.h"

using namespace phui;

#define YMARGIN 0
#define XMARGIN 3

/************************************************************************************************
	PopupMenuButton
*************************************************************************************************/

PopupMenuButton::PopupMenuButton()
{
	_value.Convert(Scalar::LongT);
	_menu = new Menu;
}

PopupMenuButton::~PopupMenuButton()
{
	if(_menu->Parent() == NULL)
		delete _menu;
}

/************************************************************************************************
	DrawSelf
*************************************************************************************************/

void PopupMenuButton::DrawSelf()
{ 
	Control::DrawSelf();

	if(DrawerEnabled()) return;
		
	GraphicsContext* ctx = GetGraphicsContext();
	
	float box[8];
	box[0] = 0; box[1] = 0;
	box[2] = _width; box[3] = 0;
	box[4] = _width; box[5] = _height;
	box[6] = 0; box[7] = _height;
		
	ctx->DisableTexture();
	
//	if(_pressed)
//		ctx->SetColor(.2,.2,.2,1);
//	else
		ctx->SetColor(.8f,.8f,.8f,1.0f);
	ctx->DrawQuad(box);
	
	ctx->SetColor(0,0,0,1);
	ctx->DrawLineStrip(4, box, true);
	
	MenuItem* curItem = _menu->ItemAt(_value);
	if(curItem)
	{
		const wchar_t* label = curItem->Label();
		
		float fontheight = ctx->FontHeight(_font);
		float x = XMARGIN;
		float y = (_height - fontheight) / 2.0f ;
		
		ctx->SetColor(_textcolor[0],_textcolor[1],_textcolor[2],_textcolor[3]);
		ctx->EnableTexture();
		ctx->MovePenPosition(x, y);
		ctx->DrawString(_font, label);
	}
}

/************************************************************************************************
	MouseDown
*************************************************************************************************/

bool PopupMenuButton::MouseDown (float x, float y, int button)
{
	if(_enabled)
	{
		ParentViewport()->SetActiveMenu(Parent(), _menu);	

		float mw,mh;
		_menu->PreferredSize(&mw, &mh);

		float mx = X(), my = Y();
		Parent()->LocalToScreen(&mx, &my);
	//	ParentWindow()->ScreenToLocal(&mx, &my);

		_menu->ResizeTo(Width(), mh);
		_menu->ShowAt(mx, my, _value, this);
	}
	
	return true;
}

/************************************************************************************************
	MouseUp
*************************************************************************************************/

bool PopupMenuButton::MouseUp   (float x, float y, int button)
{
	return true;
}

/************************************************************************************************
	MouseMove
*************************************************************************************************/

bool PopupMenuButton::MouseMove (float x, float y, int state )
{
	return View::MouseMove(x, y, state);
}


/************************************************************************************************
	PreferredSize
*************************************************************************************************/

void PopupMenuButton::PreferredSize(float* w, float* h)
{
	float menuheight, menuwidth;
	if(_menu->Parent() == NULL) AddChild(_menu);
	_menu->PreferredSize(&menuwidth,&menuheight);
	if(_menu->Parent() == this) RemoveChild(_menu);
	
	*w = menuwidth;
	GraphicsContext* ctx = GetGraphicsContext();
	*h = ctx->FontHeight(_font) + 2*YMARGIN;
}

