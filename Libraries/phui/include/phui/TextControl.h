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
	
	virtual void SetObscure(bool val) { _obscure = val; }
	virtual void SetFakeLength(int val) { _fakelen = val; }
	virtual void SetMaintainFake(bool val) { _maintainfake = val; }

	virtual void SetText(const wchar_t* text);
	virtual void SetText(const char* text);
	virtual void AddText(const wchar_t* text);
	virtual void AddText(const char* text);
	virtual void InsertText(const wchar_t* text, int start = -1, int stop = -1);
	virtual void RemoveText(int start = -1, int stop = -1);

	virtual void DrawSelf() override;

	virtual bool KeyDown(int keycode, unsigned short unicode) override;
	virtual bool KeyUp(int keycode, unsigned short unicode) override;
	virtual bool MouseDown (float x, float y, int button) override;

	virtual void PreferredSize(float* w, float* h) override;
	virtual void MinSize(float* w, float* h) override { PreferredSize(w,h); }
	
	void SetMaxLength(int l){ _maxlength = l; }
	int GetMaxLength(){ return _maxlength; }
	
	void SetTextAlignment(int alignment) { _alignment = alignment; }

	int  GetHistory(TextHistory* save, bool resetSave = true);
	int  SetHistory(TextHistory* save, bool resetHistory = false);
	void SetHistorySize(int size);
	void SetAutoCompleteCallback(AutoCompleteCallback);
	
	virtual void OnCursorMoved() override;
	virtual bool GetCursorXY(float& x, float& y) const override;
	virtual bool GetAnchorXY(float& x, float& y) const override;

	virtual void SetValue(const phui::Scalar& v) override;
	
	enum
	{
		ALIGN_LEFT, ALIGN_CENTER, ALIGN_RIGHT
	};
	
protected:
	int _maxlength, _alignment, _historySize, _historyIndex, _lastKeyDown, _fakelen;
	float _drawx;
	bool _obscure, _maintainfake;
	
	TextHistory* _history;
	AutoCompleteCallback _autoCompletor;
};
}
#endif // PHUI_TEXTCONTROL_H

