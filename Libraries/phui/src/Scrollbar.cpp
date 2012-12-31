#include "phui-precompiled-header.h"

#include "phui/Scrollbar.h"
#include "phui/Util.h"
#include "phui/GraphicsContext.h"
 
using namespace phui;
 
/************************************************************************************************
	Scrollbar
*************************************************************************************************/

Scrollbar::Scrollbar(int orient, float minbarsize): View(), _orient(orient), _minbarsize(minbarsize)
{
	_dragoffset = 0;
	_dragging = true;
	_preferredwidth = _preferredheight = 16;
}

Scrollbar::~Scrollbar()
{
}

/************************************************************************************************
	DrawSelf
*************************************************************************************************/

void Scrollbar::Draw()
{
	View::DrawSelf();

	if(DrawerEnabled()) return;

	GraphicsContext* ctx = GetGraphicsContext();

	float mx = _x;
	float my = _y;

	ctx->MovePenPosition(mx,my);
	ctx->Clip(0,0,_width,_height);
	
	BoxVerts box;	
	box_verts_with_pos(box, 0,0, _width, _height);
	
	ctx->DisableTexture();

	ctx->SetColor(0,0,0);
	ctx->DrawLineStrip(4, box.verts, true);

	ctx->SetColor(1.0f,1.0f,1.0f);
	ctx->DrawQuad(box.verts);
	
	View* parent = Parent();
	if(parent)
	{
		if(_orient == ORIENT_VERTICAL)
		{
			float ch = parent->ContentHeight();
			float ph = parent->Height();
			
			if(ch > ph)
			{
				float barheight = ph/ch*_height;
				
				if(_minbarsize > barheight)
						barheight = _minbarsize;
				
				float y2 = _height - barheight/2.0f;
				float y1 = barheight/2.0f;
				
				float y = y2 - (y2-y1) / (ch-ph) * parent->ScrollY();
				SetValue(y);
				box_verts_with_pos(box, 0,y-barheight/2.0f, _width, barheight);
				
				ctx->SetColor(.8f,.8f,.8f);
				ctx->DrawQuad(box.verts);
				
				ctx->SetColor(0.0f,0.0f,0.0f);
				ctx->DrawLineStrip(4, box.verts, true);
			}
		}else
		{
		
		}
	}

	ctx->MovePenPosition(-mx,-my);
}

/************************************************************************************************
	AttachedToWindow
*************************************************************************************************/

void Scrollbar::AttachedToWindow()
{
	View::AttachedToWindow();
	float width = Parent()->Width();
	float height = Parent()->Height();
	
	if(_orient == ORIENT_VERTICAL)
	{
		MoveTo(width-_preferredwidth, 0);
		ResizeTo(_preferredwidth, height);
		SetResizeMode(FOLLOW_TOP_BOTTOM | FOLLOW_RIGHT);
	}else
	{
		MoveTo(0, height-_preferredwidth);
		ResizeTo(width, _preferredwidth);
		SetResizeMode(FOLLOW_LEFT_RIGHT| FOLLOW_BOTTOM);
	}
}

/************************************************************************************************
	MouseDrag
*************************************************************************************************/

bool Scrollbar::MouseDrag (float x, float y, int button, bool start)
{					
	View* parent = Parent();
	if(parent)
	{
		if(_orient == ORIENT_VERTICAL)
		{
			float ch = parent->ContentHeight();
			float ph = parent->Height();
			
			if(ch > ph)
			{
				float barheight = ph/ch*_height;
				
				if(_minbarsize > barheight)
						barheight = _minbarsize;
		
				float y2 = _height - barheight/2.0f;
				float y1 = barheight/2.0f;
			

				float cury = GetValue();
				if(start)
				{
					if(y < cury-barheight/2.0f || y > cury+barheight/2.0f) return true;
					
					_dragoffset = cury - y;
					_dragging = true;
					
				}else if(!_dragging)
					return true;

				float sy = (y2 - (y+_dragoffset)) * (ch-ph) / (y2-y1);
				if(sy < 0) sy = 0;
				if(sy > ch-ph) sy = ch-ph;
				parent->ScrollTo(0,sy);
			}
		}else
		{
		
		}
	}
	
	return true;
}

/************************************************************************************************
	MouseDown
*************************************************************************************************/

bool Scrollbar::MouseDown (float x, float y, int button)
{
	View* parent = Parent();
	if(parent)
	{
		if(_orient == ORIENT_VERTICAL)
		{
			float ch = parent->ContentHeight();
			float ph = parent->Height();
			
			if(ch > ph)
			{
				float barheight = ph/ch*_height;
				
				if(_minbarsize > barheight)
						barheight = _minbarsize;
				
				float y2 = _height - barheight/2.0f;
				float y1 = barheight/2.0f;
				
				float cury = GetValue();
				if(y > cury-barheight/2.0f && y < cury+barheight/2.0f) return true;
				
				float sy = (y2 - y) * (ch-ph) / (y2-y1);
				if(sy < 0) sy = 0;
				if(sy > ch-ph) sy = ch-ph;
				parent->ScrollTo(0,sy);
			}
		}else
		{
		
		}
	}
	return true;
}

/************************************************************************************************
	MouseDown
*************************************************************************************************/

bool Scrollbar::MouseUp (float x, float y, int button)
{
	_dragging = false;
	return true;
}
