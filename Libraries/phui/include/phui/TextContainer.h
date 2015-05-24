#ifndef PHUI_TEXTCONTAINER_H
#define PHUI_TEXTCONTAINER_H

#include <string>
#include "phui/View.h"

namespace phui
{

/************************************************************************************************
	TextContainer
*************************************************************************************************/

class View;

class TextContainer
{
public:
	TextContainer() : _text(L""), _cursor(0), _anchor(0), _selecting(false), _imeControl(false)
	{
		_text.clear();
	}
	virtual ~TextContainer(){}

	virtual const char* Text(bool selected = false) const;
	virtual const wchar_t* WideText(bool selected = false) const;
	
	virtual void SetText(const wchar_t* text);
	virtual void SetText(const char* text);
	virtual void AddText(const wchar_t* text);
	virtual void AddText(const char* text);
	virtual void InsertText(const wchar_t* text, int start = -1, int stop = -1);
	virtual void RemoveText(int start = -1, int stop = -1);

	virtual int GetCursor() const;
	virtual int GetAnchor() const;
	virtual void SetCursor(int c);
	virtual void SetAnchor(int a);
	virtual bool GetCursorXY(float& x, float& y) const { return false; }
	virtual bool GetAnchorXY(float& x, float& y) const { return false; }
	
	virtual const wchar_t* GetSelection(int start = -1, int stop = -1) const;
	virtual void SetSelection(int c, int a);
	virtual bool HasSelection() const;

	virtual void OrderStops(int& start, int& stop) const;
	virtual void Constrain(int& position) const;
	
	virtual void CursorMove(int hdir, int vdir);
	virtual void CursorHMove(int dir, bool stop = false);
	virtual void CursorVMove(int dir, bool stop = false);

	virtual void ResizeToText(){}
	virtual void OnCursorMoved(){}
	virtual void OnTextModified(){}

	virtual View* AsView() = 0;
	void SetIMEControlled(bool ime){ _imeControl= ime; }
	
protected:
	
	std::wstring _text;
	int _cursor;
	int _anchor;
	bool _selecting;
	bool _imeControl;
};
}
#endif // PHUI_TEXTCONTAINER_H

