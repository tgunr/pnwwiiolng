#ifndef PHUI_TEXTCONTROL_H
#define PHUI_TEXTCONTROL_H

#include <vector>
#include "phui/Control.h"
#include "phui/TextContainer.h"

namespace phui
{

/************************************************************************************************
	TextControl
*************************************************************************************************/

typedef std::vector<std::wstring> TextHistory;
typedef const wchar_t* (*AutoCompleteCallback)(const wchar_t*, bool);

class TextControl : public Control, public TextContainer
{
public:
	TextControl();
	virtual ~TextControl(); 

	virtual View* AsView() override { return this; }
		
	virtual void DrawSelf();

	virtual bool KeyDown(int keycode, unsigned short unicode);
	virtual bool KeyUp(int keycode, unsigned short unicode);
	virtual bool MouseDown (float x, float y, int button);

	virtual void PreferredSize(float* w, float* h);
	virtual void MinSize(float* w, float* h) { PreferredSize(w,h); } 
	
	void SetMaxLength(int l){ _maxlength = l; }
	int GetMaxLength(){ return _maxlength; }
	
	void SetTextAlignment(int alignment) { _alignment = alignment; }

	int  GetHistory(TextHistory* save, bool resetSave = true);
	int  SetHistory(TextHistory* save, bool resetHistory = false);
	void SetHistorySize(int size);
	void SetAutoCompleteCallback(AutoCompleteCallback);
	
	virtual void OnCursorMoved();
	virtual bool GetCursorXY(float& x, float& y) const;
	virtual bool GetAnchorXY(float& x, float& y) const;

	virtual void SetValue(const phui::Scalar& v);
	
	enum
	{
		ALIGN_LEFT, ALIGN_CENTER, ALIGN_RIGHT
	};
	
protected:
	
	int _maxlength, _alignment, _historySize, _historyIndex, _lastKeyDown;
	float _drawx;
	
	TextHistory* _history;
	AutoCompleteCallback _autoCompletor;
};
}
#endif // PHUI_TEXTCONTROL_H

