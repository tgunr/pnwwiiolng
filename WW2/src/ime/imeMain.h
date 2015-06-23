#pragma once

#include <map>
#include <vector>
#include "pi/piinfo.h"
#include "phui/TextContainer.h"

namespace phui
{
	class View;
	class TextControl;
	class IMEControl;
};

class IMEBase;

enum IME_API
{
	IME_API_NONE = 0,
	IME_API_WIN,
	IME_API_MAC
};

enum IME_STRING
{
	IME_STRING_DESC,
	IME_STRING_COMP,
	IME_STRING_READ,
	IME_STRING_RESULT
};

enum IME_STATE
{
	IME_STATE_OFF,
	IME_STATE_ON,
	IME_STATE_ENGLISH,
	IME_STATE_UNKNOWN
};

void imeInit();
void imeKill();
void imeSetAPI();
bool imeAPI(IME_API api);
bool imeActive();
void imeUpdate();
void imeToggle();
IME_STATE imeState();
IMEBase* imeClient();
UINT32 imeProcessKey();

typedef std::map<IME_STRING, std::wstring> IME_STRING_MAP;
typedef std::vector<std::wstring> IME_CANDIDATE_LIST;

struct IME_CANDIDATES : public IME_CANDIDATE_LIST
{
	DWORD style;
	DWORD count;
	DWORD selection;
	DWORD start;
	DWORD page;

	bool DisplayList(IME_CANDIDATE_LIST& list)
	{
		list.clear();

		int stop = (count < page) ? count: start + page;

		if(stop > (int)count)
			stop = count;

		if(stop > 0)
		{
			for(int i = start; i < (int)stop; i++)
				list.push_back(at(i));
		}

		return (list.size() > 0);
	}

	DWORD Selection() const
	{
		return selection - start;
	}

	void Reset()
	{
		clear();
		count		= 0;
		selection	= 0;
		start		= 0;
		page		= 0;
	}
};

class IMEBase
{
public:

	IMEBase::IMEBase(IME_API a) :
		composing(false),
		focused(false),
		active(false),
		cursor(0),
		api(a),
		state(IME_STATE_UNKNOWN),
		focusView(0)
	{
	}

	virtual IMEBase::~IMEBase()
	{
	}

	virtual void Update();
	virtual void Setup(){}
	virtual void Init(){}
	virtual void Toggle(){}

	virtual IME_CANDIDATES& GetAllCandidates(bool force = false)
	{
		if(force)
			candidates.DisplayList(displayList);

		return candidates;
	}

	virtual IME_CANDIDATE_LIST& GetCandidates(bool force = false)
	{
		if(force)
			candidates.DisplayList(displayList);

		return displayList;
	}

	virtual void AddString(IME_STRING type, const wchar_t* value)
	{
		strings[type] = value;
	}

	virtual void AddString(IME_STRING type, const std::wstring& value)
	{
		strings[type] = value;
	}

	virtual const std::wstring& GetString(IME_STRING type, bool force = false)
	{
		IME_STRING_MAP::iterator found = strings.find(type);

		if(found != strings.end())
			return found->second;

		return emptyString;
	}

	virtual unsigned int GetCursor(bool force = true)
	{
		return cursor;
	}

	virtual IME_API API() { return api; }
	virtual void Activate() { active = true; }
	virtual void Deactivate() { active = false; }

	virtual bool IsDisabled(){ return false; }
	virtual bool IsIME(int type = 0){ return false; }
	virtual bool IsActive() { return active; }
	virtual bool IsFocused() { return focused; }

	virtual bool OnMessage(UINT& umsg, WPARAM& wParam, LPARAM& lParam){ return false; }
	virtual void OnFocus();
	virtual void OnUnfocus();
	virtual void OnInputChange();

	virtual phui::IMEControl* Control();

	virtual void ShowComposition(bool start);
	virtual void SetCompositionCursor(int position);
	virtual void SetCompositionString(const std::wstring& composition);
	virtual void SetResultString(const std::wstring& result);
	virtual void SetReadingString(const std::wstring& reading);
	virtual void SetCandidateList(long index, const std::vector<std::wstring>* list = nullptr, int selected = 0);
	virtual void ResetControl(bool updateui);

	virtual void SetState(IME_STATE s){ state = s; }
	virtual IME_STATE GetState(){ return state; }
	virtual void SetOpen(){}
	virtual UINT32 ProcessKey(){ return 0; }

	phui::TextContainer* FocusView() const;

	static const std::wstring emptyString;

protected:

	bool composing;
	bool focused;
	bool active;
	unsigned int cursor;

	IME_API api;
	IME_STATE state;
	IME_STRING_MAP strings;
	IME_CANDIDATES candidates;
	IME_CANDIDATE_LIST displayList;

	phui::View* focusView;
};
