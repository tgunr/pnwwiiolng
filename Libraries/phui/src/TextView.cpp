#include "phui-precompiled-header.h"

#include <ctype.h>
#include <math.h>
#include <string>
#include <deque>
#include <string>

#include "phui/TextView.h"
#include "phui/KeyCodes.h"
#include "phui/GraphicsContext.h"
#include "phui/Viewport.h"
#include "phui/Util.h"

using namespace phui;
using namespace std;

#define Y_MARGIN 0
#define X_MARGIN 3
#define LINE_SPACING 2

/************************************************************************************************
	TextView
*************************************************************************************************/

TextView::TextView() : View(), TextContainer()
{
	_tab_size = 4; // the "correct" size :)
	_cursorLine = _anchorLine = 0;
	_line_scroll = 0;
	_num_lines = 0;
	_line_spacing = LINE_SPACING;
	_acceptsFocus = false;
	_mousescroll = true;
	_maxlength = 0;
	_fontshadowx = _fontshadowy = 0;
	_value = L"";
}

TextView::~TextView()
{
}

/************************************************************************************************
	DrawSelf
*************************************************************************************************/

void TextView::DrawSelf() // ok
{
	View::DrawSelf();

	GraphicsContext* ctx = GetGraphicsContext();

	ctx->EnableTexture();
	ctx->MovePenPosition(_scroll_x, _scroll_y);
	ctx->SetColor(_textcolor[0],_textcolor[1],_textcolor[2],_textcolor[3]);
	
	int count			= 0;

	float fontheight	= ctx->FontHeight(_font);
	float lineheight		= fontheight + _line_spacing;

	float xposition		= _lmargin;
	float yposition		= _height - _tmargin - fontheight;

	float cxposition	= xposition;
	float cyposition	= yposition;

	float shadowx		= _fontshadowx * fontheight;
	float shadowy		= _fontshadowy * fontheight;

	int start = _cursor;
	int stop = _anchor;
	
	OrderStops(start, stop);
	
	ctx->MovePenPosition(xposition, yposition);
	
	// draw the lines
	for(int i = 0; i < _num_lines; i++)
	{
		yposition -= lineheight;
		
		Line& line = _lines.at(i);
		std::wstring& draw	= line.text;
		//std::wstring draw	= text;
		
		// we don't draw newline characters (they come out as blocks)
		//if(*draw.rbegin() == '\n')
		//	draw.erase(draw.end() - 1);
		
		if(shadowx || shadowy)
		{
			// font is shadowed, draw it here
			ctx->SetColor(0,0,0,_textcolor[3]);
			ctx->MovePenPosition(shadowx, shadowy);
			ctx->DrawString(_font, draw.c_str());
			ctx->MovePenPosition(-shadowx, -shadowy);
		}
		
		ctx->SetColor(_textcolor[0],_textcolor[1],_textcolor[2],_textcolor[3]);
		ctx->DrawString(_font, draw.c_str());

		if(HasSelection())
		{
			float sx1, sx2;
			
			if(line.position <= stop && (line.position + (int)line.text.size()) >= start)
			{
				int relStart = start - line.position;
				int relStop = stop - line.position;
				
				if(relStart < 0)
					relStart = 0;

				if(relStop > (int)line.text.size())
					relStop = line.text.size();
				
				sx1 = ctx->FontAdvance(_font, draw.substr(0, relStart).c_str());
				sx2 = ctx->FontAdvance(_font, draw.substr(0, relStop).c_str());

				BoxVerts box;
				box_verts_with_pos(box, sx1, 0, sx2 - sx1, fontheight);
				
				ctx->DisableTexture();
				ctx->SetColor(0, 0, 1.0f, .2f);
				ctx->DrawQuad(box.verts);
				ctx->EnableTexture();
			}
		}
		
		ctx->SetColor(_textcolor[0],_textcolor[1],_textcolor[2],_textcolor[3]);
		ctx->MovePenPosition(0, -lineheight);
		
		// if this is the cursor's line, determine the cursor position offset
		if(i == _cursorLine)
		{
			int diff = line.Constrain(_cursor - line.position);
			cxposition = ctx->FontAdvance(_font, draw.substr(0, diff).c_str()) + _lmargin;
			cyposition = yposition + lineheight;
		}
		
		// keep track of the number of characters drawn to aid in cursor positioning
		count += (int)line.text.size();
	}
	
	// reset the pen position
	ctx->MovePenPosition(-xposition, -yposition);
	
	// draw the cursor?
	if(_acceptsFocus && IsFocus())
	{
		float line[4];
		
		line[0] = cxposition; line[1] = cyposition;
		line[2] = cxposition; line[3] = cyposition + fontheight;
		
		ctx->DisableTexture();
		ctx->DrawLineStrip(2, line, false);
	}
	
	ctx->SetColor(1, 1, 1, 1);
}

/************************************************************************************************
	KeyDown
*************************************************************************************************/

bool TextView::KeyDown(int keycode, unsigned short unicode) // ok
{
	if(_imeControl)
		return false;

//	GraphicsContext* ctx = GetGraphicsContext();

	switch(keycode)
	{
		case phui::KEYCODE_LEFT:
			CursorHMove(-1);
		break;
		case phui::KEYCODE_UP:
			CursorVMove(-1);
		break;
		case phui::KEYCODE_RIGHT:
			CursorHMove(1);
		break;
		case phui::KEYCODE_DOWN:
			CursorVMove(1);
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
		case phui::KEYCODE_PAGEUP:
			SetSelection(0, -1);
		break;

		case phui::KEYCODE_PAGEDOWN:
			SetSelection(_text.size(), -1);
		break;

		case phui::KEYCODE_HOME:
			CursorHMove(-1, true);
		break;

		case phui::KEYCODE_END:
			CursorHMove(1, true);
		break;
		case phui::KEYCODE_TAB:
		{
			if(IsFocus())
				ParentViewport()->SetNextFocus();
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
			if(!Full())
			{
				wchar_t wcstr[2] = {0,0};
				wcstr[0] = L'\n';
				
				// add the newline
				InsertText(wcstr);

				// line count has changed
				UpdateScroll();
			}
		}
		break;
		
		default:
		{
			if(!Full() && (unicode > 127 || isprint(unicode)))
			{
				// make sure the scroll is correct
				UpdateScroll();

				// setup the character
				wchar_t wcstr[2] ={0,0};
				wcstr[0] = unicode;

				const wchar_t* p = _text.c_str();
				int textsize			= wcslen(p);
				
				// add it
				InsertText(wcstr);
				
				p						= _text.c_str();
//				wchar_t wc			= _text[_text.size() - 1];
				textsize				= wcslen(p);
//				bool stop			= true;
			}
		}
		break;
	}
	
	return true;
}

/************************************************************************************************
	KeyUp
*************************************************************************************************/

bool TextView::KeyUp(int keycode, unsigned short unicode)
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
	MouseScroll
*************************************************************************************************/

bool TextView::MouseScroll(float x, float y, int dir) // ok
{
	if(!_mousescroll) return false;
	
	_line_scroll -= dir;

	if(_line_scroll < 0)
		_line_scroll = 0;
		
	if(_line_scroll > _num_lines-1)
		_line_scroll = _num_lines-1;

	GraphicsContext* ctx = GetGraphicsContext();
	_scroll_y = _line_scroll * (ctx->FontHeight(_font));
	
	float maxsy = _contentheight - _height;
	if(maxsy < 0) maxsy = 0;
	
	if(_scroll_y < 0) _scroll_y = 0;
	if(_scroll_y > maxsy) _scroll_y = maxsy;
	
	_line_scroll = (int)(_scroll_y/(ctx->FontHeight(_font)));

	return true;
}

/************************************************************************************************
	MouseDown
*************************************************************************************************/

bool TextView::MouseDown(float x, float y, int button) // ok?
{
	if(_text.size() > 0 && !_lines.empty())
	{
		GraphicsContext* ctx	= GetGraphicsContext();
		float fontheight			= ctx->FontHeight(_font);
		float lineheight				= fontheight + _line_spacing;
		float topy					= _height - y;
		float lines					= topy / lineheight;
		
		_cursorLine					= (lines > 1) ? (int)lines: 0;
		
		if(_cursorLine >= (int)_lines.size())
			_cursorLine = (int)_lines.size() - 1;

		if(_cursorLine < 0)
			_cursorLine = 0;

		Line& line = _lines.at(_cursorLine);
		int offset = GetPositionFromWidth(line.text, x);
		
		// set the cursor
		this->SetCursor(line.position + offset);
	}

	return View::MouseDown(x, y, button);
}

/************************************************************************************************
	ScrollToLine
*************************************************************************************************/

void TextView::ScrollToLine(int line) // ok
{
	GraphicsContext* ctx = GetGraphicsContext();

	_line_scroll = line;
	
	if(_line_scroll < 0)
		_line_scroll = 0;
	
	if(_line_scroll > _num_lines - 1)
		_line_scroll = _num_lines - 1;
	
	_scroll_y = _line_scroll * (ctx->FontHeight(_font));
}

/************************************************************************************************
	UpdateLines - there was an infinite loop bug in this function, but I think its squashed
	UPDATE: Nope
*************************************************************************************************/

void TextView::UpdateLines() // ok
{
	GraphicsContext* ctx = GetGraphicsContext();
	
	_lines.clear();

	if(!ctx)
		return;

	if(_text.empty())
	{
		_cursor = _anchor = 0;
		_num_lines = _cursorLine = _anchorLine = 0;
		return;
	}
	
	if(_maxlength && (int)_text.size() > _maxlength)
		_text.erase(_maxlength, _text.size());
	
	Constrain(_cursor);
	Constrain(_anchor);

	_cursorLine			= 0;
	_anchorLine			= 0;
	_contentwidth		= _width - _rmargin - _lmargin;
	
	wchar_t chstr[2]	= {32,0};
	const wchar_t* p = _text.c_str();
	int textsize			= wcslen(p);

	// if this is too narrow for even a space character, you get an endless loop
	
	if ( _contentwidth <= 20.0 ) // doesn't work
	{
		textsize = 0;
	}
	
	int linestart			= 0;
	int lineend			= 0;
	float xposition		= 0;

	Line current;
	
	// line loop
	for(int i = 0; i < textsize;)
	{
		linestart		= i;
		lineend		= i;
		xposition		= 0;
		
		// character loop
		for(;i < textsize; i++)
		{
			wchar_t currentChar = _text.at(i);
//			const char* test = (const char*)currentChar;

//			if(currentChar == L'\r')
//				bool stop = true;
			
			if(currentChar == '\n')
			{
				current.newline = true;
				lineend = ++i; // include the newline character
				break;
			}
			
			chstr[0] = _text.at(i);
			
			xposition += ctx->FontAdvance(_font, chstr);
			
			// are we past the allowed width or at the last character?
			if(xposition >= _contentwidth || i == textsize - 1)
			{
				if(i == textsize - 1)						// we've reached the last character
				{
					lineend = ++i;
				}
				else if(lineend && lineend != linestart)	// move back to the last encountered space + 1
				{
					i = ++lineend;
				}
				else										// there was no spaces, so we'll move back 1 character
				{
					lineend = --i;

					if(lineend == linestart) // if we only did one character, move forward again to ensure we don't get in a loop
						lineend = ++i;
				}
				
				break;										// we'll break here and add the line below
			}
			
			// record the last encountered space
			if(iswspace(_text.at(i)))
				lineend = i;
		}

		if(lineend <= 0) // lineend should always be greater than 1
			break;
		
		// add the next line
		int end = lineend - ((current.newline) ? 1: 0);

		current.position = linestart;
		current.text = _text.substr(linestart, end - linestart);
		
		_lines.push_back(current);

		// reset our state
		current.Reset();
		
		// is this the cursor's line?
		if(_cursor >= linestart && _cursor <= lineend)
			_cursorLine = (int)_lines.size() - 1;

		if(_anchor >= linestart && _anchor <= lineend)
			_anchorLine = (int)_lines.size() - 1;
	}
	
	_num_lines		= (int)_lines.size();
	_contentheight	= (_num_lines * (ctx->FontHeight(_font) + _line_spacing)) - _line_spacing;
}

void TextView::UpdateScroll() // ok
{
	GraphicsContext* ctx	= GetGraphicsContext();
	int displaylines				= (int)(Height() / (ctx->FontHeight(_font)) - 2*Y_MARGIN);
	
	if(_num_lines - _line_scroll > displaylines)
	{
		_line_scroll = _num_lines - displaylines;
		_scroll_y = _line_scroll * (ctx->FontHeight(_font));
	}
}

/************************************************************************************************
	ResizeToText - mainly used by tooltips to resize themselves to the tip
*************************************************************************************************/
void TextView::ResizeToText() // ok
{
	GraphicsContext* ctx = GetGraphicsContext();
	
	float textwidth		= ctx->FontAdvance(_font, _text.c_str());
	float textheight	= ctx->FontHeight(_font) + _line_spacing;

	_width = (textwidth > _maxwidth) ? _maxwidth: textwidth;
	_width += _rmargin + _lmargin;
	
	UpdateLines();
	
	_width				= LongestLine() + _lmargin + _rmargin;
	_height				= ((textheight * _num_lines) + _tmargin + _bmargin) - _line_spacing;
	_contentwidth		= _width - _lmargin - _rmargin;
	_contentheight	= (textheight * _num_lines) - _line_spacing;
} 

void TextView::ResizeTo(float width, float height) // ok
{
	View::ResizeTo(width, height);
	UpdateLines();
}

void TextView::ResizeBy(float dw, float dh) // ok
{
	View::ResizeBy(dw, dh);
	UpdateLines();
}

void TextView::FontResized(unsigned int size) // ok
{
	View::FontResized(size);
	UpdateLines();
}

void TextView::OnTextModified() // ok
{
	UpdateLines();
	View::SetValue(_text.c_str());
}

/************************************************************************************************
	SetValue
*************************************************************************************************/

void TextView::SetValue(const phui::Scalar& v) // ok
{
	SetText(v.WideString());
}

// returns the length of the longest line
float TextView::LongestLine() // ok
{
	float longest = 0;
	GraphicsContext* ctx = GetGraphicsContext();
	
	for(int i = 0; i < _num_lines; i++)
	{
		Line& line = _lines.at(i);
		float width = 0;
		
		width = ctx->FontAdvance(_font, line.text.c_str());
		
		if(width > longest)
			longest = width;
	}
	
	return longest;
}

void TextView::SetCursor(int c) // ok
{
	// save so we can check line boundaries
	//int cursor = _cursor;
	//int cursorLine = _cursorLine;

	TextContainer::SetCursor(c);
	_cursorLine = LineForPosition(_cursor);

	// do the checks
}

void TextView::SetAnchor(int a) // ok
{
	TextContainer::SetAnchor(a);
	_anchorLine = LineForPosition(_anchor);
}

// determines which line the position is on
int TextView::LineForPosition(int position) // ok
{
	int count = 0;
	int line = 0;
	
	for(int i = 0; i < _num_lines; i++)
	{
		Line& current = _lines.at(i);
		int size = current.text.size();

		if(current.newline)
			++size;
		
		if(position >= count && position <= count + size)
		{
			line = i;
			break;
		}
		
		count += size;
	}

	return line;
}

bool TextView::Full() // ok
{
	if(_maxlength && (int)_text.size() >= _maxlength)
	{
		if((int)_text.size() > _maxlength)
			_text.erase(_maxlength, _text.size());

		return true;
	}
	
	return false;
}

// moves the cursor left or right by dir characters
void TextView::CursorHMove(int dir, bool stop)  // ok
{
	if(!_lines.empty() && dir)
	{
		Line& line = _lines.at(_cursorLine);

		if(stop)
		{
			int cursorLine = _cursorLine;

			if(dir < 0)
			{
				this->SetCursor(line.position);
			}
			else
			{
				this->SetCursor(line.position + line.text.size()); // may need to be size - 1
			}
			
			_cursorLine = cursorLine;
		}
		else
		{
			int diff = line.Constrain(_cursor - line.position);
			
			if(dir < 0)
			{
				if(diff)
				{
					this->SetCursor(_cursor - 1);
				}
				else
				{
					if(_cursorLine)
						--_cursorLine;
				}
			}
			else
			{
				if(diff == line.text.size())
				{
					if(_cursorLine < _num_lines - 1)
						++_cursorLine;
				}
				else
				{
					this->SetCursor(_cursor + 1);
				}
			}
		}
	}
}

// move the cursor vertically
void TextView::CursorVMove(int dir, bool stop) // ok
{
	if(!_lines.empty() && dir)
	{
		if(stop)
		{
			if(dir < 0)
			{
				this->SetCursor(0);
				_cursorLine = 0;
			}
			else
			{
				this->SetCursor(_text.size());
				_cursorLine = _num_lines - 1;
			}
		}
		else
		{
			GraphicsContext* ctx	= GetGraphicsContext();
			Line& current				= _lines.at(_cursorLine);
			int diff						= current.Constrain(_cursor - current.position);
			std::wstring pad			= current.text.substr(0, diff);
			float poswidth				= ctx->FontAdvance(_font, pad.c_str());
			int cursorLine				= _cursorLine;
			
			if(dir < 0)
			{
				if(_cursorLine > 0)
				{
					Line& above		= _lines.at(_cursorLine - 1);
					int linePosition		= GetPositionFromWidth(above.text, poswidth);
					
					this->SetCursor(above.position + linePosition);
					_cursorLine = cursorLine - 1;
				}
			}
			else
			{
				if(_cursorLine < _num_lines - 1)
				{
					Line& below			= _lines.at(_cursorLine + 1);
					int linePosition		= GetPositionFromWidth(below.text, poswidth);
					
					this->SetCursor(below.position + linePosition);
					_cursorLine = cursorLine + 1;
				}
			}
		}
	}
}

// tries to determine the character position from the line and width
int TextView::GetPositionFromWidth(std::wstring &line, float width) // ok
{
	GraphicsContext* ctx	= GetGraphicsContext();
	float linewidth			= ctx->FontAdvance(_font, line.c_str());
	
	if(linewidth > width)
	{
		wchar_t chstr[2] = {0,0};
		float	poswidth = 0;
		
		for(int i = 0; i < (int)line.size(); i++)
		{
			chstr[0] = line[i];
			poswidth += ctx->FontAdvance(_font, chstr);
			
			if(poswidth >= width)
			{
				int position = i;
				
				if(position > 0)
				{
					float lastwidth = poswidth - ctx->FontAdvance(_font, chstr);
					
					if(fabs(poswidth - width) < fabs(lastwidth - width))
						++position;
				}
				
				return position;
			}
		}
	}
	
	return (int)line.size();
}

bool TextView::GetCursorXY(float& x, float& y) const
{
	x = y = 0;

	if(!_lines.empty())
	{
		GraphicsContext* ctx = GetGraphicsContext();
		const Line& line = _lines.at(_cursorLine);

		x = ctx->FontAdvance(_font, line.text.substr(0, _cursor - line.position).c_str());
	}

	return true;

}

bool TextView::GetAnchorXY(float& x, float& y) const
{
	x = y = 0;

	if(!_lines.empty())
	{
		GraphicsContext* ctx = GetGraphicsContext();
		const Line& line = _lines.at(_anchorLine);

		x = ctx->FontAdvance(_font, line.text.substr(0, _anchor - line.position).c_str());
	}
	
	return true;
}

void TextView::OnCursorMoved()
{
	// do nothing for now
}