#include "phui/Groupbox.h"
#include "phui/GraphicsContext.h"
#include "phui/Util.h"

using namespace phui;

#define CAPTION_MARGIN 5.0f

/************************************************************************************************
	Groupbox
*************************************************************************************************/

Groupbox::Groupbox() : View()
{
	_topview = new View;

	View::AddChild(_topview);
}

Groupbox::~Groupbox()
{
}

/************************************************************************************************
	DrawSelf
*************************************************************************************************/

void Groupbox::DrawSelf()
{
	View::DrawSelf();
	
	if(_drawer == 0)
	{
		//float box[8];
    BoxVerts box;
		GraphicsContext* ctx = GetGraphicsContext();
		
		float offset = (_label.size() > 0) ? ctx->FontHeight(_font) / 2.0f : 0.0f;

		box_verts_with_pos(box, 0, 0, _width, _height-offset);		
		ctx->SetColor(0,0,0,1);
		ctx->DrawLineStrip(4, box, true);

		float captionheight = ctx->FontHeight(_font) / 2.0f;
		float captionwidth = ctx->FontAdvance(_font, _label.c_str());

		box_verts_with_pos(box, -1, 0, captionwidth+1, captionheight);
		ctx->MovePenPosition(CAPTION_MARGIN, Height() - ctx->FontHeight(_font));

		ctx->SetColor(1,1,1,1);
		ctx->DrawQuad(box);
		
		ctx->SetColor(0,0,0,1);
		ctx->EnableTexture();
		ctx->DrawString(_font, _label.c_str());
	}
}

/************************************************************************************************
	PreferredSize
*************************************************************************************************/

void Groupbox::PreferredSize(float* w, float* h)
{
	*w = *h = 0;
	if(_topview->ChildCount())
		_topview->ChildAt(0)->PreferredSize(w,h);
	
	if(*w < 10 && *w > -1)
		*w = 10;
	if(*h < 10 && *h > -1)
		*h = 10;
		
	if(_label.size() > 0)
	{
		GraphicsContext* ctx = GetGraphicsContext();

		float captionheight = ctx->FontHeight(_font)  / 2.0f + CAPTION_MARGIN;
		float captionwidth = ctx->FontAdvance(_font, _label.c_str()) + 2.0f*CAPTION_MARGIN;

		if(*h > -1)
			*h += captionheight;
		if(*w > -1 && *w < captionwidth)
			*w = captionwidth;
	}
}

/************************************************************************************************
	MinSize
*************************************************************************************************/

void Groupbox::MinSize(float* w, float* h)
{
	*w = *h = 0;

	if(ChildCount())
		_topview->ChildAt(0)->MinSize(w,h);
	
	if(*w < 10)
		*w = 10;
	if(*h < 10)
		*h = 10;

	if(_label.size() > 0)
	{
		GraphicsContext* ctx = GetGraphicsContext();

		float captionheight = ctx->FontHeight(_font) / 2.0f + CAPTION_MARGIN;
		float captionwidth = ctx->FontAdvance(_font, _label.c_str()) + 2.0f*CAPTION_MARGIN;

		*h += captionheight;
		if(*w < captionwidth)
			*w = captionwidth;
	}
}

/************************************************************************************************
	SetLabel
*************************************************************************************************/

void Groupbox::SetLabel(const wchar_t* s)
{
	_label = s;
}

/************************************************************************************************
	AddChild
*************************************************************************************************/

int Groupbox::AddChild(View* v, View* before)
{
	return _topview->AddChild(v,before);
}

/************************************************************************************************
	RemoveChild
*************************************************************************************************/

int Groupbox::RemoveChild(View* v)
{
	return _topview->RemoveChild(v);
}

/************************************************************************************************
	Resized
*************************************************************************************************/

void Groupbox::Resized()
{	
	GraphicsContext* ctx = GetGraphicsContext();

	float offset = (_label.size() > 0) ? ctx->FontHeight(_font) / 2.0f + CAPTION_MARGIN : 0;

	_topview->MoveTo(0, 0);
	_topview->ResizeTo(_width, _height - offset);
}

