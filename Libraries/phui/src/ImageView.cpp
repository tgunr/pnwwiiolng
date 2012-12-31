#include "phui-precompiled-header.h"

#include "phui/GraphicsContext.h"
#include "phui/ImageView.h"

using namespace phui;

/************************************************************************************************
	ImageView
*************************************************************************************************/

ImageView::ImageView()
{
	_textcolor[0] = _textcolor[1] = _textcolor[2] = _textcolor[3] = 0;
}

ImageView::~ImageView()
{
}
	
/************************************************************************************************
	DrawSelf
*************************************************************************************************/

void ImageView::DrawSelf()
{
	GraphicsContext* ctx = GetGraphicsContext();
	
	if(_textcolor[3])
		ctx->SetColor(_textcolor[0],_textcolor[1],_textcolor[2],_textcolor[3]);
	else
		ctx->SetColor(1,1,1,1);
	
	ctx->EnableTexture();
	ctx->DrawTexture(_texid, 0,0, _width, _height);
}