#ifndef PHUI_STENCILVIEW_H
#define PHUI_STENCILVIEW_H

#include "phui/View.h"

namespace phui
{

/************************************************************************************************
	StencilView
*************************************************************************************************/

class StencilView : public View
{
public:
	StencilView();
	virtual ~StencilView(); 
		
	virtual void DrawChildren();

	void SetTexture(GraphicsContext::TextureInfo const& texid) {_texid=texid;}
	
protected:
	GraphicsContext::TextureInfo _texid;
};

}
#endif // PHUI_IMAGEVIEW_H
