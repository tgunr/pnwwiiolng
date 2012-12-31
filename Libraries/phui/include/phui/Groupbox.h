#ifndef PHUI_GROUPBOX_H
#define PHUI_GROUPBOX_H

#include <string>

#include "phui/View.h"

namespace phui
{

/************************************************************************************************
	Groupbox
*************************************************************************************************/
/**
	Groupbox
*/
class Groupbox : public View
{
public:
	Groupbox();
	virtual ~Groupbox();
	
	virtual void DrawSelf();

	virtual void PreferredSize(float* w, float* h);
	virtual void MinSize(float* w, float* h);
	
	void SetLabel(const wchar_t*);
	
	virtual int AddChild(View* v, View* before=0);
	virtual int RemoveChild(View*);
	
	virtual void Resized();
	
protected:
	std::wstring _label;

private:
	View* _topview;
};

}
#endif // PHUI_CAPTION_H
