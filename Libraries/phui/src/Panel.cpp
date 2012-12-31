#include "phui-precompiled-header.h"

#include <string.h>
#include <wchar.h>

#include "phui/Panel.h"
#include "phui/GraphicsContext.h"

using namespace phui;

/************************************************************************************************
	Caption
*************************************************************************************************/

Panel::Panel() : View()
{
	_value.Convert(Scalar::WideStringT);
	_textcolor[0] = _textcolor[1] = _textcolor[2] = 0;
	_textcolor[3] = 1;
	_font = 0;
	_alignment = ALIGN_LEFT;
	_lmargin=_rmargin = 3;
	_panelmargin = 10.0f;
}

Panel::~Panel()
{
}

/************************************************************************************************
	DrawSelf
*************************************************************************************************/

#define PANEL_BORDER_BUFFER 10.0f
#define PANEL_SIDE_BUFFER .1f

void Panel::DrawSelf()
{
	float alpha = _bordercolor[3];

	_bordercolor[3] = 0.0f;
	View::DrawSelf();
	_bordercolor[3] = alpha;
	
	if(DrawerEnabled()) return;

	GraphicsContext* ctx = GetGraphicsContext();
	
	float width			= Width();
	float height		= Height();
	float textwidth		= ctx->FontAdvance(_font, _value.WideString());
	float textheight	= ctx->FontHeight(_font);
	float textstart		= _panelmargin;
	int textsize		= (int)wcslen(_value.WideString());
	float margin		= (PANEL_BORDER_BUFFER/2) + (textheight/2);
	float coords[4];
	
	switch(_alignment)
	{
		case ALIGN_CENTER:
			textstart = (Width()/2) - (textwidth/2);
			break;
		case ALIGN_RIGHT:
			textstart = Width() - (Width() * PANEL_SIDE_BUFFER) - textwidth;
			break;
		default: // align left
			textstart = (Width() * PANEL_SIDE_BUFFER);
			break;
	}
	
	// draw the panel border
	#define DRAW_LINE(x1,y1,x2,y2) coords[0]=x1;coords[1]=y1;coords[2]=x2;coords[3]=y2;ctx->DrawLineStrip(2, coords);
	//#define DRAW_LINE(x1,y1,x2,y2) glBegin(GL_LINE_STRIP);glVertex2f(x1, y1);glVertex2f(x2, y2);glEnd();
	
	if(_bordercolor[3])
	{
		ctx->SetColor(_bordercolor[0],_bordercolor[1],_bordercolor[2],_bordercolor[3]);
		ctx->DisableTexture();
		
		// bottom
		DRAW_LINE(margin,margin,width-margin,margin);
		
		// left
		DRAW_LINE(margin,margin,margin,height-margin);
		
		// right
		DRAW_LINE(width-margin,margin,width-margin,height-margin);
		
		if(textsize)
		{
			// top left
			DRAW_LINE(margin,height-margin,textstart-PANEL_BORDER_BUFFER,height-margin);

			// top right
			DRAW_LINE(textstart+textwidth+PANEL_BORDER_BUFFER,height-margin,width-margin,height-margin);
		}
		else
		{
			// top
			DRAW_LINE(margin,height-margin,width-margin,height-margin);
		}
	}

	// draw the caption
	if(textsize)
	{
		ctx->SetColor(_textcolor[0],_textcolor[1],_textcolor[2],_textcolor[3]);
		ctx->EnableTexture();
		ctx->MovePenPosition(textstart, height-margin-(textheight/2));
		ctx->DrawString(_font, _value.WideString());
		ctx->MovePenPosition(-textstart, -(height-margin-(textheight/2)));
	}

	ctx->SetColor(1,1,1,1);
}

/************************************************************************************************
	PreferredSize
*************************************************************************************************/

void Panel::PreferredSize(float* w, float* h)
{
	GraphicsContext* ctx = GetGraphicsContext();
	
	*w = ctx->FontAdvance(_font, _value.WideString()) + (_lmargin+_rmargin); 
	*h = ctx->FontHeight(_font) + (_tmargin+_bmargin);
}

