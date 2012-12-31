#ifndef PHUI_IMECONTROL_H
#define PHUI_IMECONTROL_H

#include <vector>
#include "phui/View.h"
#include "phui/TextControl.h"

namespace phui
{

/************************************************************************************************
	IMEControl
*************************************************************************************************/

class IMEControl : public View
{
public:
	IMEControl();
	virtual ~IMEControl(); 
		
	virtual void DrawSelf();
	virtual void Hide();

	void SetDescription(const std::wstring& d, bool n){ _description = d; _nativeMode = n; UpdateUI("description"); }
	void SetCursorPosition(int p){ _cursor = p; UpdateUI("curpos"); }
	void SetCompositionString(const std::wstring& c);
	void SetResultString(const std::wstring& r);
	void SetReadingString(const std::wstring& r);
	void SetCandidateList(long index, const std::vector<std::wstring>* list, int selected);

	const std::wstring& GetDescription() const { return _description; }
	int GetCursorPosition() const { return _cursor; }
	int GetSelection() const { return _candSelection; }
	const std::wstring& GetCompositionString() const { return _composition; }
	const std::wstring& GetResultString() const { return _result; }
	const std::wstring& GetReadingString() const { return _reading; }
	const std::vector<std::wstring>* GetCandidateList() const { return _candList; }

	bool InitInput();
	void UpdateUI(const char* type = "");
	
	void Activate();
	void Deactivate(bool reset = true);
		
protected:
	
	bool _nativeMode;
	std::wstring _origText;
	std::wstring _description;
	std::wstring _composition;
	std::wstring _reading;
	std::wstring _result;
	const std::vector<std::wstring>* _candList;
	long _candIndex;
	int _candSelection;
	int _cursor, _anchor, _origCur, _origAnc;
	float _startX, _startY;
	
	// view we're mimicing
	TextContainer* _input;

};
}
#endif // PHUI_IMECONTROL_H

