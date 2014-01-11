#include "phui-precompiled-header.h"

#include "phui/StackView.h"
#include "phui/GraphicsContext.h"
#include "phui/UIMessages.h"
#include "phui/MessageLoop.h"
#include "phui/Message.h"

using namespace phui;

/************************************************************************************************
	StackView
*************************************************************************************************/

StackView::StackView() : View()
{
	_value.Convert(Scalar::LongT); 
}

StackView::~StackView()
{
}

/************************************************************************************************
	AddChild
*************************************************************************************************/

int StackView::AddChild(View* v,View* before)
{
	int err = View::AddChild(v,before);
	if(err != 0) return err;
	
	if(v == ChildAt(_value))
		v->Show();
	else
		v->Hide();
	
	return 0;
}
/************************************************************************************************
	SetValue
*************************************************************************************************/

void StackView::SetValue(const Scalar& v)
{
	unsigned int oldval = _value;
	unsigned int newval = v;
		
	if(newval != oldval)
	{
		View* view = ChildAt(oldval);
		if(view) view->Hide();

		view = ChildAt(newval);
		if(view)
		{
			view->Show();
		}
	}
	
	View::SetValue(newval);
}

/************************************************************************************************
	Show
*************************************************************************************************/

void StackView::Show()
{
	if(ParentLoop() && !_isVisible)
	{
		phui::Message msg(SHOW_MSG);
		ParentLoop()->PostMessage(&msg, this);
	}
	
	_isVisible = true; 
		
	View* v = ChildAt(_value);
	if(v)
		v->Show();
}