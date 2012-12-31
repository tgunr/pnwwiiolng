#include <syscfg.h> // Path is project specific.

#include "cfg/cfgPrefs.h"
#include "ime/imeMain.h"
#include "ime/IMEClient.h"
#include "phui/TextControl.h"
#include "phui/IMEControl.h"
#include "ui/uiMain.h"

static IMEBase* _imeClient = NULL;

const std::wstring IMEBase::emptyString = L"";

void imeInit()
{
	// handle mac?
	imeSetAPI();
}

void imeKill()
{
	if(_imeClient)
		delete _imeClient;

	_imeClient = NULL;
}

void imeSetAPI()
{
	imeKill();

	IME_API api = (piinfoIsMinOS(PI_OS_PC)) ? IME_API_WIN: IME_API_MAC;

	switch(api)
	{
		case IME_API_WIN:
			_imeClient = new IMEClient;
		break;
		default:
			// no ime
		break;
	}

	// call setup?
}

bool imeAPI(IME_API api)
{
	return (_imeClient && _imeClient->API() == api) ? true: false;
}

bool imeActive()
{
	return (_imeClient && _imeClient->IsActive()) ? true: false;
}

void imeUpdate()
{
	if(_imeClient && !_imeClient->IsDisabled())
		_imeClient->Update();
}

void imeToggle()
{
	if(_imeClient)
		_imeClient->Toggle();
}

IME_STATE imeState()
{
	if(_imeClient)
		return _imeClient->GetState();

	return IME_STATE_UNKNOWN;
}

IMEBase* imeClient()
{
	return _imeClient;
}

UINT32 imeProcessKey()
{
	return ImmGetVirtualKey(ddpGetHWND());
}

void IMEBase::Update()
{
	phui::View* uiFocus = uiFocusView();

	if(uiFocus != focusView)
	{
		if(uiFocus && focusView)
		{
			OnUnfocus();
			focusView = uiFocus;
			OnFocus();
		}
		else if(uiFocus)
		{
			focusView = uiFocus;
			OnFocus();
		}
		else
		{
			OnUnfocus();
			focusView = uiFocus;
		}
	}
}

void IMEBase::OnFocus()
{
	Activate();
}

void IMEBase::OnUnfocus()
{
	Deactivate();
}

phui::IMEControl* IMEBase::Control()
{
	phui::Viewport* vp = uiViewport();
	if(!vp) return NULL;

	phui::View* ime = vp->FindView("ime");
	if(!ime)
	{
		ime = uiLoadView("ime");
		if(ime) ime->Hide();
	}

	return (ime) ? dynamic_cast<phui::IMEControl*>(ime): NULL;
}

void IMEBase::OnInputChange()
{
	phui::IMEControl* ime = Control();
	if(ime) ime->SetDescription(strings[IME_STRING_DESC], IsActive());
}

void IMEBase::ShowComposition(bool start)
{
	phui::IMEControl* ime = Control();

	if(ime)
	{
		if(start)
			ime->Activate();
		else
			ime->Deactivate(false);
	}

	composing = start;
}

void IMEBase::SetCompositionCursor(int position)
{
	phui::IMEControl* ime = Control();
	if(ime) ime->SetCursorPosition(position);
}

void IMEBase::SetCompositionString(const std::wstring& composition)
{
	if(!composing)
		ShowComposition(true);

	phui::IMEControl* ime = Control();
	if(ime) ime->SetCompositionString(composition);
}

void IMEBase::SetResultString(const std::wstring& result)
{
	if(!composing)
		ShowComposition(true);

	phui::IMEControl* ime = Control();
	if(ime) ime->SetResultString(result);
}

void IMEBase::SetReadingString(const std::wstring& reading)
{
	phui::IMEControl* ime = Control();
	if(ime) ime->SetReadingString(reading);
}

void IMEBase::SetCandidateList(long index, const std::vector<std::wstring>* list, int selected)
{
	phui::IMEControl* ime = Control();
	if(ime) ime->SetCandidateList(index, list, selected);
}

void IMEBase::ResetControl(bool updateui)
{
	phui::IMEControl* ime = Control();
	if(ime) ime->Deactivate(updateui);
}

phui::TextContainer* IMEBase::FocusView() const
{
	return (focusView) ? dynamic_cast<phui::TextContainer*>(focusView): NULL;
}
