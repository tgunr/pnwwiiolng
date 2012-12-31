#include "phui-precompiled-header.h"

#include <list>
#include <algorithm>
#include <string.h>

#include "phui/Viewport.h"
#include "phui/View.h"
#include "phui/UIMessages.h"
#include "phui/Message.h"
#include "phui/GraphicsContext.h"
#include "phui/Periodical.h"
#include "phui/MessageLoop.h"
#include "phui/CSSViewDrawer.h"
#include "phui/UIXML.h"
#include "phui/KeyCodes.h"
#include "phui/TextControl.h"

using namespace phui;

#define TOOLTIP_LIFE 5000

/************************************************************************************************
	Viewport
*************************************************************************************************/

Viewport::Viewport(GraphicsContext* ctx, float x, float y, float width, float height) :  View()
{
	_lastclickview = _lastdragview = _tooltipview = 0;
	_graphicscontext = ctx;
	_activemenu = 0;
	_isVisible = true;
	
	_x = x;
	_y = y;
	_width = width;
	_height = height;
	_parentviewport = this;
	_graphicscontext = ctx;
	_tooltipstart = 0;
	_dotooltips = true;
	_tooltipmode = 0;
	_modifiers = MODIFIER_NONE;
	_focusCallback = NULL;
	
	SetBackgroundColor(0,0,0,0);
}

Viewport::~Viewport()
{
	delete _graphicscontext;
}

/************************************************************************************************
	DrawSelf
*************************************************************************************************/

void Viewport::DrawSelf()
{
	View::DrawSelf();
	
	// tooltip control
	if(_dotooltips && _tooltipview)
	{
		unsigned int frametime = ParentLoop()->FrameTime();
		
		/*
		 HideToolTip nulls out the tooltipview, so this block only gets
		 called for the life of the tooltip, which is at max
		 TOOLTIP_LIFE + tooltipdelay miliseconds. For now TOOLTIP_LIFE
		 is hardcoded, but eventually it will become a style option
		 like tooltipdelay
		 */
		
		if(frametime >= (_tooltipstart + TOOLTIP_LIFE))
			HideToolTip();
		else if(frametime >= _tooltipstart)
			_tooltipview->Show();
	}
}

/************************************************************************************************
	PostMouseDown
*************************************************************************************************/

bool Viewport::PostMouseDown(float x, float y, int button)
{
	HideToolTip();

	// , View::FLAG_MOUSECLICK|View::FLAG_MOUSEDRAG
	View* v = FindViewHit(x, y, true); // within the top level window view
	
	if(_activemenu && v != _activemenu)
		SetActiveMenu(NULL, NULL);
	
	if(v)
	{
		v->MakeFocus();
		_lastclickview = v;
		
		Message msg(MOUSE_DOWN);
		msg.AddLong(UI_MOUSE_BUTTON, button);
		msg.AddFloat(UI_MOUSE_X, x);
		msg.AddFloat(UI_MOUSE_Y, y);
		ParentLoop()->PostMessage(&msg, v);
		
		return true;
	}
	else
	{
		View* focus = FindFocusView();
		if(focus)
			focus->MakeFocus(false);
	}

	return (v != NULL);
}

/************************************************************************************************
	PostMouseUp
*************************************************************************************************/

bool Viewport::PostMouseUp(float x, float y, int button)
{
	_lastdragview = 0;
	
	if(_lastclickview)
	{
		if(FindView(_lastclickview)) // cheesy, in lieu ref counting
		{
			Message msg(MOUSE_UP);
			msg.AddLong(UI_MOUSE_BUTTON, button);
			msg.AddFloat(UI_MOUSE_X, x);
			msg.AddFloat(UI_MOUSE_Y, y);
			ParentLoop()->PostMessage(&msg, _lastclickview);
		}

		_lastclickview = 0;
		
		return true;
	}

	return false;
}

/************************************************************************************************
	PostMouseMove
*************************************************************************************************/

bool Viewport::PostMouseMove(float x, float y)
{
	long state;
	Message msg(MOUSE_MOVE);
	msg.AddFloat(UI_MOUSE_X, x);
	msg.AddFloat(UI_MOUSE_Y, y);

	//, View::FLAG_MOUSEMOVE
	std::vector<View*> views;
	FindViewsHit(x, y, views, true); // within the top level window view

	for(unsigned int i = 0; i < views.size(); i++)
	{
		View* v = views[i];
		std::vector<View*>::iterator it = find(_lastoverviews.begin(), _lastoverviews.end(), v);

		if(it != _lastoverviews.end())
		{
			_lastoverviews.erase(it);
			state = MOUSE_OVER;
		}else
		{
			state = MOUSE_ENTER;
		}	

		msg.ReplaceOrAddLong(UI_MOUSE_MOVESTATE, state);
		ParentLoop()->PostMessage(&msg, v);
	}

	for(unsigned int i = 0; i < _lastoverviews.size(); i++)
	{
		View* v = _lastoverviews[i];
		if(FindView(v))
		{
			state = MOUSE_EXIT;
			msg.ReplaceOrAddLong(UI_MOUSE_MOVESTATE, state);
			ParentLoop()->PostMessage(&msg, v);
		}
	}
	
	_lastoverviews = views;
		
	if(x >= 0 && y >= 0 && x < _width && y < _height)
		return true;
	else
		return false;
}

/************************************************************************************************
	PostMouseDrag
*************************************************************************************************/

bool Viewport::PostMouseDrag (float x, float y, int button)
{
	PostMouseMove(x,y);
	
	if(_lastclickview && FindView(_lastclickview))
	{
		Message msg(MOUSE_DRAG);

		bool dragstart = (_lastdragview == 0);
		msg.AddLong(UI_MOUSE_BUTTON, button);
		msg.AddFloat(UI_MOUSE_X, x);
		msg.AddFloat(UI_MOUSE_Y, y);
		msg.AddBool(UI_MOUSE_DRAG_START, dragstart);
		ParentLoop()->PostMessage(&msg, _lastclickview);
		
		_lastdragview = _lastclickview;

		return true;
	}

	_lastdragview = 0;
	return false;
}

/************************************************************************************************
	PostMouseScroll
*************************************************************************************************/

bool Viewport::PostMouseScroll(float x, float y, int dir)
{
	// , View::FLAG_MOUSESCROLL
	View* v = FindViewHit(x, y, true); // within the top level window view
	if(v)
	{
		Message msg(MOUSE_SCROLL);
		msg.AddFloat(UI_MOUSE_X, x);
		msg.AddFloat(UI_MOUSE_Y, y);
		msg.AddLong(UI_MOUSE_SCROLLDIR, dir);
		ParentLoop()->PostMessage(&msg, v);

		return true;
	}

	return false;
}

/************************************************************************************************
	PostKeyDown
*************************************************************************************************/

bool Viewport::PostKeyDown(int keycode, unsigned short unicode)
{
	View* focusView = FindFocusView();
	TextControl* t = dynamic_cast<TextControl*>(focusView);

	switch(keycode)
	{
		case phui::KEYCODE_RSHIFT:
		case phui::KEYCODE_LSHIFT:
			_modifiers |= MODIFIER_SHIFT;
		break;
		case phui::KEYCODE_RCTRL:
		case phui::KEYCODE_LCTRL:
			_modifiers |= MODIFIER_CTRL;
		break;
		case phui::KEYCODE_RALT:
		case phui::KEYCODE_LALT:
			_modifiers |= MODIFIER_ALT;
		break;
		case phui::KEYCODE_x:
		{
			if(_modifiers & MODIFIER_CTRL)
			{
				if(t && t->HasSelection())
				{
					// we're cutting
					const wchar_t* text = t->WideText(true);
					t->RemoveText();
					UIXMLClipboard(text);
				}

				_modifiers = MODIFIER_NONE;

				return true;
			}
		}
		break;
		case phui::KEYCODE_c:
		{
			if(_modifiers & MODIFIER_CTRL)
			{
				if(t && t->HasSelection())
				{
					// we're copying
					UIXMLClipboard(t->WideText(true));
				}

				_modifiers = MODIFIER_NONE;

				return true;
			}
		}
		break;
		case phui::KEYCODE_v:
		{
			if(_modifiers & MODIFIER_CTRL)
			{
				if(t)
				{
					// we're pasting
					const wchar_t* htext = UIXMLClipboard(NULL);
					if(htext && wcslen(htext) > 0) t->InsertText(htext);
				}

				_modifiers = MODIFIER_NONE;

				return true;
			}
		}
		break;
		case phui::KEYCODE_INSERT:
		{
			if(_modifiers & MODIFIER_CTRL)
			{
				if(t && t->HasSelection())
				{
					// we're copying
					UIXMLClipboard(t->WideText(true));
				}

				_modifiers = MODIFIER_NONE;

				return true;
			}
			else if(_modifiers & MODIFIER_SHIFT)
			{
				if(t)
				{
					// we're pasting
					const wchar_t* htext = UIXMLClipboard(NULL);
					if(htext && wcslen(htext) > 0) t->InsertText(htext);
				}

				_modifiers = MODIFIER_NONE;

				return true;
			}
		}
		break;
	}
	
	Message msg(KEY_DOWN);
	msg.AddLong(UI_KEYCODE, keycode);
	msg.AddLong(UI_KEY_UNICODE, unicode);
	ParentLoop()->PostMessage(&msg, focusView);   // send message even when there is no focusView as there may be a handler or filter
									// that is interested in getting a key press even if nothing is focused
	return (focusView != NULL);		// but as far as the outside world is concerned we didn't handle it
}

/************************************************************************************************
	PostKeyUp
*************************************************************************************************/

bool Viewport::PostKeyUp  (int keycode, unsigned short unicode)
{
	View* focusView = FindFocusView();

	switch(keycode)
	{
		case phui::KEYCODE_RSHIFT:
		case phui::KEYCODE_LSHIFT:
			_modifiers &= ~MODIFIER_SHIFT;
		break;
		case phui::KEYCODE_RCTRL:
		case phui::KEYCODE_LCTRL:
			_modifiers &= ~MODIFIER_CTRL;
		break;
		case phui::KEYCODE_RALT:
		case phui::KEYCODE_LALT:
			_modifiers &= ~MODIFIER_ALT;
		break;
	}

	Message msg(KEY_UP);
	msg.AddLong(UI_KEYCODE, keycode);
	msg.AddLong(UI_KEY_UNICODE, unicode);
	ParentLoop()->PostMessage(&msg, focusView);   // send message even when there is no focusView as there may be a handler or filter
									// that is interested in getting a key press even if nothing is focused
	return (focusView != NULL);		// but as far as the outside world is concerned we didn't handle it
}

void Viewport::SetNextFocus()
{
	std::vector<View*> views;
	View* next = NULL;

	FindFocusViews(views);
	
	for(unsigned int i = 0; i < views.size(); ++i)
	{
		View* current = views[i];

		if(!next)
			next = current;
		
		if(current->IsFocus())
		{
			if(++i < views.size())
				next = views[i];
			
			break;
		}
	}
	
	if(next)
		next->MakeFocus(true);
}

void Viewport::SetFocusCallback(FocusCallback cb)
{
	_focusCallback = cb;
}

void Viewport::OnFocusModified(bool focus)
{
	if(_focusCallback)
		_focusCallback(focus);
}

/************************************************************************************************
	SetActiveMenu
*************************************************************************************************/

void Viewport::SetActiveMenu(View* menuview, View* v)
{
	if(v == _activemenu) return;
	
	if(_activemenu)
		_activemenuview->RemoveChild(_activemenu);

	_activemenu = v;
	_activemenuview = menuview;
	
	if(_activemenu)
		_activemenuview->AddChild(_activemenu);
}

/************************************************************************************************
	SetDefaultFont
*************************************************************************************************/

bool Viewport::SetDefaultFont(const char* name, unsigned int size)
{
	if(_graphicscontext->SetDefaultFont(name, size))
	{
		FontResized(size);
		return true;
	}
	
	return false;
}

/************************************************************************************************
	SetDefaultFont
*************************************************************************************************/

bool Viewport::SetDefaultFont(unsigned int size)
{
	std::string name = _graphicscontext->FontName(_graphicscontext->GetDefaultFont());
	return SetDefaultFont(name.c_str(), size);
}

/************************************************************************************************
	ShowToolTip - tooltip is actually shown in the DrawSelf method
*************************************************************************************************/

void Viewport::ShowToolTip(float x, float y, View *v)
{
	if(!_dotooltips)
		return;

	if(!v || !wcslen(v->GetToolTip()))
		return;
	
	if(!_tooltipview)
	{
		View *tooltipbase = NULL;
		std::string tooltip; // a tooltip's ID is also its relative path
		
		tooltip = v->GetToolTipPath();
		
		if(!tooltip.length())
			tooltip = GetToolTipPath();
		
		// lets see if the tooltip is already loaded
		if(tooltip.length())
			tooltipbase = FindView(tooltip.c_str());
		
		if(!tooltipbase)
		{
			// since the tooltip wasn't already load it, lets load it
			tooltipbase = LoadUIXML(tooltip.c_str());
			
			if(tooltipbase)
				AddChild(tooltipbase);
		}
		
		if(tooltipbase)
		{
			// lets make it the active tooltip
			_tooltipview = dynamic_cast<TextView*>(tooltipbase);
			
			if(_tooltipview)
			{
				_tooltipview->SetText(v->GetToolTip());					// set the tip text
				_tooltipview->ResizeToText();							// lets make it all fit

				// when to show it?
				_tooltipstart = ParentLoop()->FrameTime();
				_tooltipstart += (v->GetToolTipDelay()) ? v->GetToolTipDelay(): _tooltipdelay;
			}
		}
	}
	
	if(_tooltipview)
	{
		// how far from the screen's edges do we want to be
		float buffer = 10;
		
		// we always position the tooltip based on where the mouse is
		v->LocalToScreen(&x, &y);

		// lets move the tooltip low and right by default
		x += 10.0f,
		y -= _tooltipview->Height() + 30.0f;
		
		// adjust horizontal position
		if(x + _tooltipview->Width() + buffer > Width())
			x -= (_tooltipview->Width() - buffer);
		else if(x < buffer)
			x = buffer;
		
		// adjust vertical position
		if(y < buffer)
			y += _tooltipview->Height() + 40.0f;
		
		// move to the right spot
		// show is handled by DrawSelf()
		_tooltipview->MoveTo(x, y);
	}
}

void Viewport::SetToolTipMode(int mode)
{
	_tooltipmode = mode;
}

int Viewport::GetToolTipMode()
{
	return _tooltipmode;
}

/************************************************************************************************
	HideToolTip
*************************************************************************************************/

void Viewport::HideToolTip()
{
	if(_tooltipview != NULL)
		_tooltipview->Hide();
	
	_tooltipview = NULL;
	_tooltipstart = 0;
}
