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

	virtual View* AsView() override { return this; }
	
	virtual void DrawSelf() override;

	virtual bool KeyDown(int keycode, unsigned short unicode) override;
	virtual bool KeyUp(int keycode, unsigned short unicode) override;
	virtual bool MouseScroll(float x, float y, int dir) override;
	virtual bool MouseDown(float x, float y, int button) override;

	// from TextContainer
	virtual void SetCursor(int c) override;
	virtual void SetAnchor(int a) override;
	virtual bool GetCursorXY(float& x, float& y) const override;
	virtual bool GetAnchorXY(float& x, float& y) const override;
	virtual void SetValue(const phui::Scalar& v) override;
	virtual void CursorVMove(int dir, bool stop = false) override;
	virtual void CursorHMove(int dir, bool stop = false) override;
	virtual void OnCursorMoved() override;
	virtual void OnTextModified() override;
	virtual void ResizeToText() override;

	virtual void ResizeTo(float width, float height) override;
	virtual void ResizeBy(float dw, float dh) override;
	virtual void FontResized(unsigned int size) override;
	
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
