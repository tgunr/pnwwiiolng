#pragma once

#include "ime/imeMain.h"

/*
void imeInputChange(bool alertUI = true);
const std::wstring& imeGetCompositionString(HIMC context, int type);
int imeGetCandidateList(long index);
int imeGuideline();
*/

class IMMClient : public IMEBase
{
public:

	IMMClient::IMMClient() : IMEBase(), native(true), currentLocale(0), inputLocale(0)
	{
		api = IME_API_IMM;
	}

	virtual IMMClient::~IMMClient()
	{
		// clean up IMM resources
	}

	virtual void Init();
	virtual void Reset();

	virtual const IME_CANDIDATE_LIST& GetCandidates(bool force = true);
	virtual const std::wstring& GetString(IME_STRING type, bool force = true);
	virtual unsigned int GetCursor(bool force = true);

	// true means return result right away
	// false means let the message fall through to the default handler
	virtual bool OnMessage(UINT& umsg, WPARAM& wParam, LPARAM& lParam, LRESULT& result);
	virtual void OnFocus();
	virtual void OnUnfocus();

	virtual void SetActive(bool a){}
	virtual bool IsActive();

	void InputChange(bool alertUI = true);

protected:

	bool native;
	HKL currentLocale;
	HKL inputLocale;
};
