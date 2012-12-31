#include "phui-precompiled-header.h"

#include <ctype.h>
#include <cmath>

#include "phui/TextContainer.h"
#include "phui/Util.h"

using namespace phui;

/************************************************************************************************
	TextContainer
*************************************************************************************************/

const char* TextContainer::Text(bool selected) const
{
	return (const char*)utf16_to_utf8(WideText(selected));
}

const wchar_t* TextContainer::WideText(bool selected) const
{
	if(selected)
		return GetSelection();
	
	return _text.c_str();
}

void TextContainer::SetText(const wchar_t* text)
{
	if(HasSelection())
	{
		// set the selected text to the new text
		InsertText(text);
	}
	else
	{
		// set all text to new text
		InsertText(text, 0, _text.size());
	}
}

void TextContainer::SetText(const char* text)
{
	SetText(utf8_to_utf16((const unsigned char*)text));
}

void TextContainer::AddText(const wchar_t* text)
{
	// make sure we append the text if nothing is selected
	if(!HasSelection())
		this->SetCursor(_text.size());	// To distinguish from the Windows SetCursor function.
		
	InsertText(text);
}

void TextContainer::AddText(const char* text)
{
	AddText(utf8_to_utf16((const unsigned char*)text));
}

void TextContainer::InsertText(const wchar_t* text, int start, int stop)
{
	// left to right
	OrderStops(start, stop);
	
	// remove text selection if specified
	if(stop != start)
		_text.erase(start, stop - start);
	
	// add the text at start
	_text.insert(start, text);

	// set our cursor to the end of the newly added text
	SetSelection(start + wcslen(text), -1);
	
	// alert callbacks
	OnTextModified();
}

void TextContainer::RemoveText(int start, int stop)
{
	// left to right
	OrderStops(start, stop);
	
	// nothing to do
	if(start == stop)
		return;
	
	// remove the text
	_text.erase(start, stop - start);
	
	// update cursor position
	SetSelection(start, -1);
	
	// alert callbacks
	OnTextModified();
}

int TextContainer::GetCursor() const
{
	return _cursor;
}

int TextContainer::GetAnchor() const
{
	return _anchor;
}

const wchar_t* TextContainer::GetSelection(int start, int stop) const
{
	static std::wstring selection;
	selection = L"";
	
	OrderStops(start, stop);
	
	selection = _text.substr(start, stop - start);
	
	return selection.c_str();
}

void TextContainer::SetCursor(int c)
{
	_cursor = c;

	Constrain(_cursor);
	
	if(!_selecting)
		SetAnchor(_cursor);

	OnCursorMoved();
}

void TextContainer::SetAnchor(int a)
{
	_anchor = a;

	if(_anchor == -1)
	{
		_anchor = _cursor;
		_selecting = false;
	}
	else
	{
		Constrain(_anchor);
	}
}

void TextContainer::SetSelection(int c, int a)
{
	this->SetCursor(c);
	SetAnchor(a);
}

bool TextContainer::HasSelection() const
{
	return (_anchor != _cursor);
}

void TextContainer::Constrain(int& c) const
{
	if(c < 0)
	{
		c = 0;
		return;
	}
	const int tsize = (int)_text.size();
	if(c > tsize)
		c = tsize;
}

void TextContainer::CursorMove(int hdir, int vdir)
{
	CursorHMove(hdir);
	CursorVMove(vdir);
}

void TextContainer::CursorHMove(int dir, bool stop)
{
	SetCursor(_cursor + dir);
}

void TextContainer::CursorVMove(int dir, bool stop)
{
	// don't do anything by default
}

void TextContainer::OrderStops(int& start, int& stop) const
{
	if(start == -1)
		start = _cursor;
	
	if(stop == -1)
		stop = _anchor;
	
	if(stop < start)
	{
		int placeHolder = start;
		start = stop;
		stop = placeHolder;
	}
	
	Constrain(start);
	Constrain(stop);
}

