#ifndef PHUI_TEXTVIEW_H
#define PHUI_TEXTVIEW_H

#include <string>
#include <vector>

#include "phui/View.h"
#include "phui/TextContainer.h"

namespace phui
{

/************************************************************************************************
	TextView
*************************************************************************************************/
/**
	View for displaying and editing text.
*/
class TextView : public View, public TextContainer
{
public:

	struct Line
	{
		Line() : newline(false), position(-1)
		{
		}

		void Reset()
		{
			text.clear();
			newline = false;
			position = -1;
		}

		int Constrain(int c) const
		{
			if(c < 0)
			{
				c = 0;
			}
			else if(c > (int)text.size())
			{
				c = text.size();
			}

			return c;
		}

		std::wstring text;
		bool newline; // had a newline on the end in the original text; keep a record
		int position; // position in the main text buffer where this line starts
	};

	TextView();
	virtual ~TextView();
	
	virtual void DrawSelf();

	virtual bool KeyDown(int keycode, unsigned short unicode);
	virtual bool KeyUp(int keycode, unsigned short unicode);
	virtual bool MouseScroll(float x, float y, int dir);
	virtual bool MouseDown(float x, float y, int button);

	// from TextContainer
	virtual void SetCursor(int c);
	virtual void SetAnchor(int a);
	virtual bool GetCursorXY(float& x, float& y) const;
	virtual bool GetAnchorXY(float& x, float& y) const;
	virtual void SetValue(const phui::Scalar& v);
	virtual void CursorVMove(int dir, bool stop = false);
	virtual void CursorHMove(int dir, bool stop = false);
	virtual void OnCursorMoved();
	virtual void OnTextModified();
	virtual void ResizeToText();

	virtual void ResizeTo(float width, float height);
	virtual void ResizeBy(float dw, float dh);
	virtual void FontResized(unsigned int size);
	
	void SetLineSpacing(float spacing){ _line_spacing = spacing; }
	void SetFontShadow(float sx, float sy){ _fontshadowx = sx; _fontshadowy = sy; }
	
	int NumLines() const { return _num_lines; }

	void ScrollToLine(int);
	void SetMouseScroll(bool s) { _mousescroll = s; }

	void SetMaxLength(int l){ _maxlength = l; }
	int GetMaxLength(){ return _maxlength; }

	float	LongestLine();								// returns the length of the longest line, used in re-sizing
	void	UpdateLines();								// populates the _lines vector with lines from _text
	void	UpdateScroll();							// recalculate our scroll position
	int		LineForPosition(int position);			// calculate line relative positions
	bool	Full();										// have we reached max text; if so trim it
	
	int  GetPositionFromWidth(std::wstring &line, float width);

protected:
	
	std::vector<Line> _lines;
	int _tab_size, _maxlength;
	int _cursorLine, _anchorLine; // global lines our cursor and index is on
	int _line_scroll;
	int _num_lines;
	float _line_spacing;
	bool _mousescroll;
	float _fontshadowx, _fontshadowy;
};

}
#endif // PHUI_TEXTVIEW_H
