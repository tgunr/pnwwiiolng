#ifndef PHUI_RADIOBUTTON_H
#define PHUI_RADIOBUTTON_H

#include "phui/Control.h"

namespace phui
{

/************************************************************************************************
	RadioButton
*************************************************************************************************/
/**
	A Radio Button! When clicked, it turns off any radio buttons in the same view.
*/
class RadioButton : public Control
{
public:
	RadioButton();
	virtual ~RadioButton(); 

	virtual void DrawSelf();
	
	virtual bool MouseDown (float x, float y, int button);
	virtual bool MouseUp   (float x, float y, int button);
	virtual bool MouseMove (float x, float y, int state );
	
	virtual void PreferredSize(float* w, float* h);
	virtual void MinSize(float* w, float* h) { PreferredSize(w,h); }

	virtual void SetValue(const Scalar& v);

protected:
friend class ViewDrawer;
	bool _pressed;
	bool _mousedown;

private:
	void TurnOffOtherRadioButtons();
};


/************************************************************************************************
	RadioGroup
*************************************************************************************************/
/**
	A view control that returns the value of the currently selected child radio button
*/

class RadioGroup : public Control
{
public:
	RadioGroup();
	virtual ~RadioGroup(); 
	
	virtual void PreferredSize(float* w, float* h);
	virtual void MinSize(float* w, float* h) { PreferredSize(w,h); }

	RadioButton* RadioButtonAt(unsigned int idx) const;
	unsigned int SelectedRadioButtonIndex() const;
	void SelectRadioButton(unsigned int idx);
	
	void SetRadioGroupView(View*); // defaults to itself

private:
	View* _groupview;
};

}
#endif // PHUI_RADIOBUTTON_H
