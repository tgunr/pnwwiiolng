#ifndef PHUI_STACKVIEW_H
#define PHUI_STACKVIEW_H

#include <string>

#include "phui/View.h"

namespace phui
{

/************************************************************************************************
	StackView
*************************************************************************************************/
/**
	StackView
*/
class StackView : public View
{
public:
	StackView();
	virtual ~StackView(); 
	
	virtual int AddChild(View* v, View* before=nullptr);
	virtual void SetValue(const Scalar& v);
	virtual void Show();
};

}
#endif // PHUI_STACKVIEW_H
