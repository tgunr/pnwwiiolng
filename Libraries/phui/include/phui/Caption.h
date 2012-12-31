#ifndef PHUI_CAPTION_H
#define PHUI_CAPTION_H

#include "phui/View.h"

namespace phui
{

/************************************************************************************************
	Caption
*************************************************************************************************/
/**
	View for displaying a single line of text.
*/
class Caption : public View
{
public:
	Caption();
	virtual ~Caption();
	
	virtual void Draw();
	virtual void DrawSelf();
	virtual void SetValue(const Scalar& v);

	virtual void PreferredSize(float* w, float* h);
	virtual void MinSize(float* w, float* h) { PreferredSize(w,h); }
	
	void SetTextAlignment(int alignment) { _alignment = alignment; }
	void SetAutoResize(bool ar){ _autoresize = _resize = ar; }
	
enum
{
	ALIGN_LEFT, ALIGN_CENTER, ALIGN_RIGHT
};

protected:
	int _alignment;
	bool _autoresize;
	bool _resize;
	
};

}
#endif // PHUI_CAPTION_H
