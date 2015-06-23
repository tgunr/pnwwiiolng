#include "phui-precompiled-header.h"

#include <ctype.h>
#include <cmath>

#include "phui/IMEControl.h"
#include "phui/KeyCodes.h"
#include "phui/UIMessages.h"
#include "phui/GraphicsContext.h"
#include "phui/Util.h"
#include "phui/Message.h"
#include "phui/Viewport.h"
#include "phui/TextControl.h"
#include "phui/DOM.h"

using namespace phui;

/************************************************************************************************
	IMEControl
*************************************************************************************************/

IMEControl::IMEControl(): View(),
	_input(nullptr),
	_nativeMode(false),
	_candList(nullptr),
	_candIndex(0),
	_candSelection(0), 
	_cursor(0),
	_anchor(0),
	_origCur(0),
	_origAnc(0),
	_startX(0),
	_startY(0)
{
	_composition	= L"";
	_reading			= L"";
	_result			= L"";
	_description	= L"";
	_origText		= L"";
}

IMEControl::~IMEControl()
{
}

/************************************************************************************************
	DrawSelf
*************************************************************************************************/

void IMEControl::DrawSelf()
{
	if(!_input)
		return;

	// find the focus view and latch on
	//if(_input
	
	//TextControl::DrawSelf();
	/*
	TextView* textview = dynamic_cast<TextView*>(ChildAt(0));

	if(textview)
	{
		std::wstring debug = L"";
		wchar_t buffer[255+1];

		debug += _description; debug += L"\n";
		debug += L"Composition: "; debug += _composition; debug += L"\n";
		debug += L"Reading: "; debug += _reading; debug += L"\n";
		debug += L"Result: "; debug += _result; debug += L"\n";

		swprintf(buffer, 255, L"Cursor: %d\n", _cursorPos);
		debug += buffer;

		swprintf(buffer, 255, L"Cand Index: %d\n", _candIndex);
		debug += buffer;

		swprintf(buffer, 255, L"Cand Selection: %d\n", _candSelection);
		debug += buffer;

		debug += L"Candidates:\n\n";
		
		if(_candList && _candList->size())
		{
			for(int i = 0; i < (int)_candList->size(); i++)
			{
				swprintf(buffer, 255, L"%d. %ls\n", i + 1, _candList->at(i).c_str());
				debug += buffer;
			}
		}
		
		textview->SetText(debug.c_str());
	}
	*/

	View* vinput = _input->AsView();
	
	if(vinput)
	{
		float ix = 0;
		float iy = 0;
		
		vinput->LocalToScreen(&ix, &iy);

		View* reading = FindView("reading");
		View* candidates = FindView("candidates");

		float cwidth = (candidates) ? candidates->Width(): 0;
		float iwidth = vinput->Width();
		
		ix += (_startX > iwidth - cwidth) ? iwidth - cwidth: _startX;
		iy += vinput->Height() + 5.0f;
		
		if(reading && reading->IsVisible())
		{
			reading->MoveTo(ix, iy);
			iy += reading->Height() + 5.0f;
		}
		
		if(candidates && candidates->IsVisible())
		{
			candidates->MoveTo(ix, iy);
			iy += candidates->Height() + 5.0f;
		}
		
		View* description = FindView("description");
		
		if(description && description->IsVisible())
			description->MoveTo(ix, iy);
	}
	
	View::DrawSelf();
}

void IMEControl::Hide()
{
	Deactivate(false);
	View::Hide();
}

void IMEControl::Deactivate(bool reset)
{
	if(_input)
	{
		if(reset)
		{
			_input->SetText(_origText.c_str());
			_input->SetSelection(_origCur, _origAnc);
		}
		
		_input->SetIMEControlled(false);
	}

	// reset everything
	_composition = _reading = _result = L"";
	
	_candList			= nullptr;
	_input				= nullptr;
	_candIndex			= 0;
	_candSelection		= 0;
	_cursor				= 0;
	_anchor				= 0;
	_origText			= L"";
	_origCur			= 0;
	_origAnc			= 0;
}

void IMEControl::Activate()
{
	if(!InitInput())
		return;
	
	_origText			= _input->WideText();
	_origCur			= _input->GetCursor();
	_origAnc			= _input->GetAnchor();
	
	_input->GetCursorXY(_startX, _startY);
	
	if(_startX > _input->AsView()->Width())
		_startX = _input->AsView()->Width();
	
	if(!IsVisible())
		Show();
	
	UpdateUI("description");
	UpdateUI("candidates");
	UpdateUI("reading");
}

bool IMEControl::InitInput()
{
	phui::View* fv = ParentViewport()->FindFocusView();
	phui::TextContainer* in = dynamic_cast<phui::TextContainer*>(fv);

	if(_input && _input == in && fv->IsVisible())
		return true;

	Deactivate(false);

	if(in && fv->IsVisible())
	{
		_input = in;
		//_input->SetIMEControlled(true);
		return true;
	}
	
	return false;
}

void IMEControl::SetCompositionString(const std::wstring& c)
{
	if(!InitInput())
		return;

	_composition = c;
	_input->SetSelection(0, -1);
	_input->SetText(_origText.c_str());

	int start = _origCur;
	int stop = _origAnc;

	_input->OrderStops(start, stop);
	_input->InsertText(_composition.c_str(), start, stop);

	stop = start + _composition.size();
	
	_input->SetSelection(start, stop);
	_input->SetIMEControlled((c.size()) ? true: false);
}

void IMEControl::SetCandidateList(long index, const std::vector<std::wstring>* list, int selected)
{
	if(!InitInput())
		return;

	_candIndex = index;
	_candList = list;
	_candSelection = selected;

	UpdateUI("candidates");
	
	if(list && list->size())
		_input->SetIMEControlled(true);
}

void IMEControl::SetResultString(const std::wstring& r)
{
	if(!InitInput())
		return;

	_input->SetSelection(0, -1);
	_input->SetText(_origText.c_str());
	_input->SetSelection(_origCur, _origAnc);
	_input->InsertText(r.c_str());
	//_input->SetAnchor(-1);
	
	_origText	= _input->WideText();
	_origCur		= _input->GetCursor();
	_origAnc		= _input->GetAnchor();

	_input->GetCursorXY(_startX, _startY);
	
	if(_startX > _input->AsView()->Width())
		_startX = _input->AsView()->Width();
	
	//_reading		= L"";
	
	//UpdateUI("result");
	
	_input->SetIMEControlled(false);
}

void IMEControl::SetReadingString(const std::wstring& r)
{
	if(!InitInput())
		return;

	_reading = r;

	UpdateUI("reading");
}


void IMEControl::UpdateUI(const char* type)
{
	if(!InitInput())
		return;

	phui::DOM::ElementDOM* viewDom = dynamic_cast<phui::DOM::ElementDOM*>(UserData(DOM_VIEW_USERDATA));
	
	if(viewDom)
	{
		char script[128];
		sprintf(script, "Update('%s')", type);
		viewDom->RunScript(script);
	}
}
