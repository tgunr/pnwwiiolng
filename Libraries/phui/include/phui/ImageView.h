#ifndef PHUI_IMAGEVIEW_H
#define PHUI_IMAGEVIEW_H

#include "phui/View.h"

namespace phui
{

/************************************************************************************************
	ImageView
*************************************************************************************************/

class ImageView : public View
{
public:
	ImageView();
	virtual ~ImageView(); 
		
	virtual void DrawSelf();

	void SetTexture(GraphicsContext::TextureInfo const& texid) { _texid = texid; }
	
protected:
	GraphicsContext::TextureInfo _texid;
};

}
#endif // PHUI_IMAGEVIEW_H
