#include "phui-precompiled-header.h"

#include <wchar.h>
#include <stdlib.h>
#include <stdio.h>

#include "phui/RadioButton.h"
#include "phui/GraphicsContext.h"
#include "phui/UIMessages.h"

using namespace phui;

#define RADIUS 6

/************************************************************************************************
	RadioButton
*************************************************************************************************/

RadioButton::RadioButton(): Control()
{
	_value.Convert(Scalar::BoolT);
	_pressed = _mousedown = false;
	_lmargin=_rmargin = 3;
}

RadioButton::~RadioButton()
{
}

/************************************************************************************************
	DrawSelf
*************************************************************************************************/

void RadioButton::DrawSelf()
{ 
	Control::DrawSelf();
	if(DrawerEnabled()) return;
	
	GraphicsContext* ctx = GetGraphicsContext();

	float fontheight = ctx->FontHeight(_font);
	float x = 2*RADIUS + (_lmargin+_rmargin);
	float y = (_height - fontheight) / 2.0f;
	
	ctx->SetColor(_textcolor[0],_textcolor[1],_textcolor[2],_textcolor[3]);
	ctx->EnableTexture();
	ctx->MovePenPosition(x, y);
	ctx->DrawString(_font, _label.String());
	ctx->MovePenPosition(-x, -y);
	
	x = _lmargin+RADIUS;
	y = _height/ 2.0f;
	ctx->DisableTexture();
	ctx->MovePenPosition(x, y);
		
	if(_pressed)
		ctx->SetColor(.2f,.2f,.2f,1.0f);
	else
		ctx->SetColor(.8f,.8f,.8f,1.0f);
	ctx->DrawCircle(RADIUS, true);

	ctx->SetColor(0,0,0,1);
	ctx->DrawCircle(RADIUS, false);

	if(_value)
	{
		ctx->SetColor(0,0,0,1);
		ctx->DrawCircle(RADIUS-2, true);
	}
}

/************************************************************************************************
	PreferredSize
*************************************************************************************************/

void RadioButton::PreferredSize(float* w, float* h)
{
	GraphicsContext* ctx = GetGraphicsContext();
	
	float fontheight = ctx->FontHeight(_font);
	
	*w = ctx->FontAdvance(_font, _label.String()) + 2*RADIUS + 3*_lmargin;
	*h = fontheight + (_bmargin+_tmargin); 
}

/************************************************************************************************
	MouseDown
*************************************************************************************************/

bool RadioButton::MouseDown (float x, float y, int button)
{
	if(_enabled && !_value.Bool())
	{
		_mousedown = true;
		_pressed = true;
	}
	
	return true;
}

/************************************************************************************************
	MouseUp
*************************************************************************************************/

bool RadioButton::MouseUp   (float x, float y, int button)
{
	if(_enabled && !_value.Bool())
	{
		SetValue(true);
		Invoke();
	
		_mousedown = false;
		_pressed = false;
	}
	
	return true;
}

/************************************************************************************************
	MouseMove
*************************************************************************************************/

bool RadioButton::MouseMove (float x, float y, int state )
{
	if(!_value.Bool())
	{
		if(state == MOUSE_ENTER)
			_pressed = _mousedown;
		else if(state == MOUSE_EXIT)
			_pressed = false;
	}
	
	return View::MouseMove(x, y, state);
}

/************************************************************************************************
	TurnOffOtherRadioButtons
*************************************************************************************************/

void RadioButton::TurnOffOtherRadioButtons()
{
	View* parent = Parent();
	
	if(parent)
	{
		View* v;
		for(int i = 0; (v = parent->ChildAt(i)) != NULL; i++)
		{
			RadioButton* btn = dynamic_cast<RadioButton*>(v);
			if(btn && btn != this)
				btn->SetValue(false);
		}
	}
}


/************************************************************************************************
	SetValue
*************************************************************************************************/

void RadioButton::SetValue(const Scalar& v)
{
	if(v.Bool() == true)
		TurnOffOtherRadioButtons();
		
	View::SetValue(v);
}

/************************************************************************************************
	RadioGroup
*************************************************************************************************/

RadioGroup::RadioGroup() : Control()
{
	_value.Convert(Scalar::LongT);
	_groupview = this;
}

RadioGroup::~RadioGroup()
{
}

/************************************************************************************************
	PreferredSize
*************************************************************************************************/

void RadioGroup::PreferredSize(float* w, float* h)
{
	if(ChildCount())
	{
		ChildAt(0)->PreferredSize(w,h);
	}else
	{
		*w = *h = 0;
	}
}


/************************************************************************************************
	RadioButtonAt
*************************************************************************************************/

RadioButton* RadioGroup::RadioButtonAt(unsigned int idx) const
{
	unsigned int radiocount = 0;
	unsigned int count = _groupview->ChildCount();
	
	for(unsigned int i = 0; i < count; i++)
	{
		RadioButton* btn = dynamic_cast<RadioButton*>(_groupview->ChildAt(i));
		if(btn)
		{
			if(radiocount == idx) 
				return btn;
			radiocount++;
		}
	}
	return NULL;
}

/************************************************************************************************
	SelectedRadioButtonIndex
*************************************************************************************************/

unsigned int RadioGroup::SelectedRadioButtonIndex() const
{
	unsigned int radiocount = 0;
	unsigned int count = _groupview->ChildCount();
	
	for(unsigned int i = 0; i < count; i++)
	{
		RadioButton* btn = dynamic_cast<RadioButton*>(_groupview->ChildAt(i));
		if(btn)
		{
			if(btn->GetValue().Bool()) 
				return radiocount;
			radiocount++;
		}
	}
	return 0;
}

/************************************************************************************************
	SelectRadioButton
*************************************************************************************************/

void RadioGroup::SelectRadioButton(unsigned int i)
{
	RadioButton* btn = RadioButtonAt(i);
	if(btn)
		btn->SetValue(true);
}

/************************************************************************************************
	SetRadioGroupView
*************************************************************************************************/

void RadioGroup::SetRadioGroupView(View* v)
{
	if(v == NULL) 
	{
		_groupview = this;
	}else
	{
		if(v->Parent() == this)
			_groupview = v;
	}
}


