#ifndef PHUI_MIRRORVIEW_H
#define PHUI_MIRRORVIEW_H

#include "phui/View.h"

namespace phui
{

/************************************************************************************************
	MirrorView
*************************************************************************************************/

class MirrorView : public View
{
public:
	MirrorView();
	virtual ~MirrorView(); 
	virtual void DrawSelf();
	
	void SnapShot(float x, float y, float w, float h);

protected:

	void capture();

	GraphicsContext::TextureInfo _texInfo;
	float _sx, _sy, _sw, _sh;
	bool _capture;
};

}
#endif // PHUI_MIRRORVIEW_H
