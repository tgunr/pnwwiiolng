#include "phui-precompiled-header.h"

#include <ctype.h>
#include <cmath>

#include "phui/TextControl.h"
#include "phui/KeyCodes.h"
#include "phui/UIMessages.h"
#include "phui/GraphicsContext.h"
#include "phui/Util.h"
#include "phui/Message.h"
#include "phui/Viewport.h"

using namespace phui;

/************************************************************************************************
	TextControl
*************************************************************************************************/

TextControl::TextControl(): Control(), TextContainer()
{
	_value.Convert(Scalar::WideStringT);
	_textcolor[0] = _textcolor[1] = _textcolor[2] = 0;
	_textcolor[3] = 1;
	SetAcceptsFocus(true);
	_lmargin = _rmargin = 3;
	_tmargin = _bmargin = 1;
	_maxlength = 0;
	_drawx = _lmargin;
	_alignment = ALIGN_LEFT;
	_historySize = _historyIndex = 0;
	_history = nullptr;
	_autoCompletor = nullptr;
	_lastKeyDown = -1;
	_selecting = false;
}

TextControl::~TextControl()
{
	if(_history)
	{
		_history->clear();
		delete _history;
		_history = nullptr;
		_historySize = 0;
	}
}

/************************************************************************************************
	DrawSelf
*************************************************************************************************/

void TextControl::DrawSelf()
{
	Control::DrawSelf();

	GraphicsContext* ctx = GetGraphicsContext();
	 
	ctx->SetColor(_textcolor[0],_textcolor[1],_textcolor[2],_textcolor[3]);
	
	int tsize				= _text.size();
	float fheight		= ctx->FontHeight(_font);
	float fwidth			= ctx->FontAdvance(_font, _text.c_str());

	if(!tsize)
	{
		_drawx = _lmargin;
		SetSelection(0, -1);
	}
	else
	{
		Constrain(_cursor);
		Constrain(_anchor);
	}
	
	float x				= _drawx;
	float y				= (Height() - fheight)/2;
	
	if(fwidth < _width - _lmargin - _rmargin)
	{
		if(_alignment == ALIGN_CENTER)
		{
			x = (Width() - fwidth) / 2.0f;
		}
		else if(_alignment == ALIGN_RIGHT)
		{
			x = Width() - _rmargin - fwidth;
		}
	}
	
	ctx->EnableTexture();
	ctx->MovePenPosition(x, y);
	ctx->DrawString(_font, _text.c_str());
	ctx->DisableTexture();
	ctx->MovePenPosition(0, -y);
	 
	if(HasSelection())
	{
		float cx, cy;
		float ax, ay;
		
		GetCursorXY(cx, cy);
		GetAnchorXY(ax, ay);
		
		if(ax < cx)
		{
			float placeHolder = cx;
			cx = ax;
			ax = placeHolder;
		}
		
		BoxVerts box;
		box_verts_with_pos(box, cx, 0, ax - cx, fheight);
		ctx->SetColor(0, 0, 1.0f, .2f);
		ctx->DrawQuad(box.verts);
	}
	
	// draw the cursor?
	if(AcceptsFocus() && IsFocus())
	{
		float line[4];
		float spacing = (Height() - fheight)/4;
		float xpos = ctx->FontAdvance(_font, _text.substr(0, _cursor).c_str());
		
		line[0] = xpos; line[1] = spacing;
		line[2] = xpos; line[3] = Height() - spacing;
		
		ctx->SetColor(_textcolor[0],_textcolor[1],_textcolor[2],_textcolor[3]);
		ctx->DrawLineStrip(2, line, false);
	}
}

/************************************************************************************************
	KeyDown
*************************************************************************************************/

bool TextControl::KeyDown(int keycode, unsigned short unicode)
{
	if(_imeControl)
		return false;
	
	switch(keycode)
	{
		case phui::KEYCODE_LEFT:
			CursorHMove(-1);
		break;
		case phui::KEYCODE_RIGHT:
			CursorHMove(1);
		break;
		case phui::KEYCODE_UP:
		{
			SetAnchor(-1);

			int size = (_history) ? (int)_history->size(): 0;

			if(size)
			{
				if(_historyIndex == 0)
					(*_history)[0] = _text.c_str(); // save the current input line
				
				++_historyIndex;
				
				if(_historyIndex >= size)
					_historyIndex = size - 1;
				
				SetText(_history->at(_historyIndex).c_str());
			}
		}
		break;
		case phui::KEYCODE_DOWN:
		{
			SetAnchor(-1);

			int size = (_history) ? (int)_history->size(): 0;

			if(size)
			{
				--_historyIndex;
				
				if(_historyIndex < 0)
				{
					// we save the current line if there is one because
					// we were already at position 0, the input line
					(*_history)[0] = _text.c_str();
					_historyIndex = 0;
				}
				
				SetText(_history->at(_historyIndex).c_str());
			}
		}
		break;

		case phui::KEYCODE_HOME:
		{
			this->SetCursor(0);
			_drawx = _lmargin;
		}
		break;
		 
		case phui::KEYCODE_END:
		{
			this->SetCursor(_text.size());
			
			float fwidth = GetGraphicsContext()->FontAdvance(_font, _text.c_str());
			
			if(fwidth > _width - _lmargin - _rmargin)
				_drawx = _width - _rmargin - fwidth;
		}
		break;

		case phui::KEYCODE_BACKSPACE:
			if(!_text.empty() && (HasSelection() || _cursor > 0))
			{
				if(HasSelection())
				{
					RemoveText();
				}
				else
				{
					RemoveText(_cursor - 1, _cursor);
				}
			}
		break;

		case phui::KEYCODE_DELETE:
			if(!_text.empty())
			{
				if(HasSelection())
				{
					RemoveText();
				}
				else
				{
					RemoveText(_cursor, _cursor + 1);
				}
			}
		break;

		case phui::KEYCODE_TAB:
		{
			SetAnchor(-1);
			
			if(_autoCompletor)
				SetText(_autoCompletor(_text.c_str(), (_lastKeyDown == phui::KEYCODE_TAB)));
		}
		break;

		case phui::KEYCODE_RSHIFT:
		case phui::KEYCODE_LSHIFT:
		{
			_selecting = true;
		}
		break;

		case phui::KEYCODE_RETURN:
		{
			if(_history && _text.size() && (_history->size() == 1 || _text != _history->at(1)))
			{
				_history->insert(_history->begin() + 1, _text);
				
				if((int)_history->size() > _historySize)
					_history->pop_back();
				
				(*_history)[0] = L"";
			}
			
			Invoke();
			SetSelection(0, -1);
			//_drawx = _lmargin;
			_historyIndex = 0;
		}
		break;
		
		default:
			if((!_maxlength || (int)_text.size() < _maxlength) && (unicode > 127 || isprint(unicode)))
			{
				wchar_t wcstr[2] ={0,0};
				wcstr[0] = unicode;
				InsertText(wcstr);
			}
		break;
	}

	_lastKeyDown = keycode;
	
	return true;
}

/************************************************************************************************
	KeyUp
*************************************************************************************************/

bool TextControl::KeyUp(int keycode, unsigned short unicode)
{
	if(_imeControl)
		return false;
	
	switch(keycode)
	{
		case phui::KEYCODE_RSHIFT:
		case phui::KEYCODE_LSHIFT:
		{
			_selecting = false;
		}
		break;
	
		default:
			// do nothing
		break;
	}
		
	return true;
}

/************************************************************************************************
	MouseDown
*************************************************************************************************/

bool TextControl::MouseDown(float x, float y, int button)
{
	// position cursor
	unsigned int size = (unsigned int)_text.size();
	
	if(size)
	{
		GraphicsContext* ctx	= GetGraphicsContext();
		wchar_t chstr[2]		= {0,0};
		float targetx			= x + std::abs(_drawx);
		float cursorx			= 0;
		float cwidth				= 0;
		int cursor				= _cursor;
		
		if(_drawx > 0)
			targetx -= _lmargin;

		for(unsigned int i = 0; i < size; ++i)
		{
			chstr[0]		= _text[i];
			cwidth		= ctx->FontAdvance(_font, chstr);
			cursor		= i;

			if(cursorx + cwidth/2 > targetx)
				break;
			
			cursorx += cwidth;
		}
		
		if(cursor + 1 >= (int)size && targetx > cursorx)
			cursor = size;
		
		this->SetCursor(cursor);
	}
	
	return Control::MouseDown(x, y, button);
}

/************************************************************************************************
	SetValue
*************************************************************************************************/

void TextControl::SetValue(const phui::Scalar& v)
{
	Control::SetValue(v);
	SetText(_value.WideString());
}

/************************************************************************************************
	PreferredSize
*************************************************************************************************/

void TextControl::PreferredSize(float* w, float* h)
{
	GraphicsContext* ctx = GetGraphicsContext();
	*h = ctx->FontHeight(_font) + (_tmargin+_bmargin);
	*w = 300;
}

/************************************************************************************************
	OnCursorMoved
*************************************************************************************************/

void TextControl::OnCursorMoved()
{
	if(_text.size())
	{
		GraphicsContext* ctx = GetGraphicsContext();
		wchar_t chstr[2] = {0,0};
		
		float fwidth = ctx->FontAdvance(_font, _text.substr(0, _cursor).c_str());

		if(_drawx + fwidth > _width - _lmargin)
		{
			chstr[0] = _text[_cursor - 1];
			_drawx -= ctx->FontAdvance(_font, chstr);
		}
		else if(_drawx + fwidth < 0)
		{
			chstr[0] = _text[_cursor];
			_drawx += ctx->FontAdvance(_font, chstr);
		}
	}
}

/************************************************************************************************
	SetHistorySize
*************************************************************************************************/

void TextControl::SetHistorySize(int size)
{
	// first line is used as the current input line so size must be
	// greater than one in order for it to be meaningful
	if(size > 1)
	{
		if(!_history)
			_history = new TextHistory;
		
		if(!_history->size()) // first line is the current input line and begins as an empty string
			_history->insert(_history->begin(), L"");

		if((int)_history->size() > size) // erase anything past the new size
			_history->erase(_history->begin() + (_history->size() - size), _history->end());

		_historySize = size;
	}
	else
	{
		if(_history)
		{
			_history->clear();
			delete _history;
			_history = nullptr;
			_historySize = 0;
		}
	}
}

/************************************************************************************************
	GetHistory
*************************************************************************************************/

int TextControl::GetHistory(TextHistory* save, bool resetSave)
{
	if(!save)
		return 0;

	// reset
	if(resetSave)
		save->clear();
	
	if(_history && _history->size())
	{
		for(int i = 0; i < (int)_history->size(); i++)
			save->push_back((*_history)[i]);
	}
	
	return (int)save->size();
}

/************************************************************************************************
	SetHistory
*************************************************************************************************/

int TextControl::SetHistory(TextHistory* save, bool resetHistory)
{
	// reset
	if(save && save->size())
	{
		if(!_history)
			_history = new TextHistory;

		if(resetHistory)
			_history->clear();
		
		for(int i = 0; i < (int)save->size(); i++)
			_history->push_back((*save)[i]);
		
		return (int)_history->size();
	}
	
	return 0;
}

/************************************************************************************************
	SetAutoCompleteCallback
*************************************************************************************************/

void TextControl::SetAutoCompleteCallback(AutoCompleteCallback callback)
{
	_autoCompletor = callback;
}

bool TextControl::GetCursorXY(float& x, float& y) const
{
	x = GetGraphicsContext()->FontAdvance(_font, _text.substr(0, _cursor).c_str());
	y = 0;

	return true;
}

bool TextControl::GetAnchorXY(float& x, float& y) const
{
	if(HasSelection())
	{
		x =GetGraphicsContext()->FontAdvance(_font, _text.substr(0, _anchor).c_str());
		y = 0;

		return true;
	}
	
	return false;
}
