#include <syscfg.h> // Path is project specific.
#pragma once

#include "ime/TSFClient.h"
#include "phui/TextControl.h"

#define VALID_RESULT(r) ((r) == S_OK)
#define VALID_OBJECT(r, o) (VALID_RESULT(r) && (o) != NULL)

extern HWND myhwnd;

STDMETHODIMP TSFCompSink::OnStartComposition(__RPC__in_opt ITfCompositionView* pComposition, __RPC__out BOOL* pfOk)
{
	uiIMEShowComposition(true);
	return S_OK;
}

STDMETHODIMP TSFCompSink::OnUpdateComposition(__RPC__in_opt ITfCompositionView* pComposition, __RPC__in_opt ITfRange* pRangeNew)
{
	return S_OK;
}

STDMETHODIMP TSFCompSink::OnEndComposition(__RPC__in_opt ITfCompositionView* pComposition)
{
	uiIMEShowComposition(false);
	return S_OK;
}

  //IUnknown methods.
STDMETHODIMP TSFCompSink::QueryInterface(REFIID riid, LPVOID* punk)
{
    *punk = NULL;

    if (riid == IID_IUnknown)
        *punk = reinterpret_cast<IUnknown*>(this);
    else if (riid == IID_ITfContextOwnerCompositionSink)
        *punk = reinterpret_cast<ITfContextOwnerCompositionSink*>(this);

    if (!*punk)
        return E_NOINTERFACE;

    return S_OK;
}

STDMETHODIMP_(DWORD) TSFCompSink::AddRef()
{
	return 1;
}

STDMETHODIMP_(DWORD) TSFCompSink::Release()
{
	return 1;
}
///////////////////////////////////////////////////////

  //IUnknown methods.
STDMETHODIMP TSFUISink::QueryInterface(REFIID riid, LPVOID* punk)
{
    *punk = NULL;

    if (riid == IID_IUnknown)
        *punk = reinterpret_cast<IUnknown*>(this);
    else if (riid == IID_ITfUIElementSink)
        *punk = reinterpret_cast<ITfUIElementSink*>(this);

    if (!*punk)
        return E_NOINTERFACE;

    return S_OK;
}

STDMETHODIMP_(DWORD) TSFUISink::AddRef()
{
	return 1;
}

STDMETHODIMP_(DWORD) TSFUISink::Release()
{
	return 1;
}

HRESULT STDMETHODCALLTYPE TSFUISink::BeginUIElement(DWORD dwUIElementId, BOOL* pbShow)
{
	*pbShow = TRUE;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE TSFUISink::UpdateUIElement(DWORD dwUIElementId)
{
	ITfUIElement* ui = GetUI(dwUIElementId);

	if(ui)
	{
		ITfCandidateListUIElement* candidatesUI = NULL;

		HRESULT hr = ui->QueryInterface(IID_ITfCandidateListUIElement, (void**)&candidatesUI);

		if(SUCCEEDED(hr))
			client->AddCandidates(candidatesUI);

		ITfReadingInformationUIElement* readingUI = NULL;

		hr = ui->QueryInterface(IID_ITfReadingInformationUIElement, (void**)&readingUI);

		if(SUCCEEDED(hr))
		{
			BSTR* value = NULL;

			hr = readingUI->GetString(value);

			if(SUCCEEDED(hr))
			{
				std::wstring wvalue = (WCHAR*)value;
				client->AddString(IME_STRING_READ, wvalue);
			}
		}
	}

	return S_OK;
}

HRESULT STDMETHODCALLTYPE TSFUISink::EndUIElement(DWORD dwUIElementId)
{
	ITfUIElement* ui = GetUI(dwUIElementId);

	if(ui)
	{
		// figure out which phui thing to hide
	}

	return S_OK;
}

ITfUIElement* TSFUISink::GetUI(DWORD id)
{
	if(InitManager())
	{
		ITfUIElement* ui = NULL;

		HRESULT hr = cpUIMgr->GetUIElement(id, &ui);

		if(SUCCEEDED(hr))
			return ui;
	}

	return NULL;
}

bool TSFUISink::InitManager()
{
	if(cpUIMgr != NULL)
		return true;

	TSFManager manager;
	client->GetManager(manager);
	if(!manager) return false;

	HRESULT hr = manager->QueryInterface(IID_ITfUIElementMgr, (void**)&cpUIMgr);

	return SUCCEEDED(hr);
}

void TSFUISink::Activate()
{
	if(InitManager())
	{
		ITfSource* source = NULL;

		HRESULT hr = cpUIMgr->QueryInterface(IID_ITfSource, (void**)&source);

		if(SUCCEEDED(hr))
			source->AdviseSink(IID_ITfUIElementSink, this, &cookie);
	}
}

void TSFUISink::Deactivate()
{
	if(InitManager())
	{
		ITfSource* source = NULL;

		HRESULT hr = cpUIMgr->QueryInterface(IID_ITfSource, (void**)&source);

		if(SUCCEEDED(hr))
		{
			source->UnadviseSink(cookie);
			cookie = 0;
		}
	}
}

TSFClient::~TSFClient()
{
	// clean up TSF resources
//	if(manager && clientID)
//		manager->Deactivate();

	if(cpDocMgr != NULL && acpContext != NULL)
		cpDocMgr->Pop(TF_POPF_ALL);

    cpDocMgr = NULL;

	if(cpThreadMgr != NULL)
		cpThreadMgr->Deactivate();

	acpContext		= NULL;
	compContext		= NULL;
	cpDocMgr		= NULL;
    cpThreadMgr		= NULL;

	clientID = 0;
	acpCookie = 0;
	compCookie = 0;
}

void TSFClient::Init()
{
	// create our TSF manager
	HRESULT hr = CoCreateInstance(CLSID_TF_ThreadMgr, NULL, CLSCTX_INPROC_SERVER, IID_ITfThreadMgr, (void**)&cpThreadMgr);

	if(SUCCEEDED(hr))
	{
		ATL::CComPtr<ITfThreadMgrEx> threadMgrEx;
		hr = cpThreadMgr->QueryInterface(IID_ITfThreadMgrEx, (LPVOID*)&threadMgrEx);

		if(SUCCEEDED(hr))
			hr = threadMgrEx->ActivateEx(&clientID, TF_TMAE_UIELEMENTENABLEDONLY); // look into other flags to limit activation until focus |TF_TMAE_NOACTIVATEKEYBOARDLAYOUT|TF_TMAE_NOACTIVATETIP

		if(SUCCEEDED(hr))
			hr = cpThreadMgr->CreateDocumentMgr(&cpDocMgr);
	}

	Reset();
}

void TSFClient::OnFocus()
{
	if(!cpThreadMgr || !cpDocMgr)
		return;

	HRESULT hr = cpDocMgr->CreateContext(clientID, 0, &acp, &acpContext, &acpCookie);

	if(SUCCEEDED(hr))
	{
		cpDocMgr->CreateContext(clientID, 0, &compSink, &compContext, &compCookie);
		uiSink.Activate();
		acp.client = this;
		compSink.client = this;
		hr = cpDocMgr->Push(acpContext);

		if(SUCCEEDED(hr))
			cpDocMgr->Push(compContext);
	}

	// we're good
	if(SUCCEEDED(hr) && uiIMEInputChange(GetString(IME_STRING_DESC, false), IsActive()))
		cpThreadMgr->SetFocus(cpDocMgr);
	else
		Reset();
}

void TSFClient::OnUnfocus()
{
	if(!cpThreadMgr || !cpDocMgr)
		return;

	if(acpContext != NULL)
	{
		cpDocMgr->Pop(TF_POPF_ALL);
		acpCookie = 0;
		compCookie = 0;

		acpContext = NULL;
		compContext = NULL;
	}

	uiSink.Deactivate();

	cpThreadMgr->SetFocus(NULL);
	Reset();
}

const IME_CANDIDATE_LIST& TSFClient::GetCandidates(bool force)
{
	if(force)
	{
		candidates.clear();

		// do stuff here

		candidates.DisplayList(displayList);
	}

	return IMEBase::GetCandidates(force);
}

const std::wstring& TSFClient::GetString(IME_STRING type, bool force)
{
	if(force)
	{
		int immType	= -1;

		switch(type)
		{
			case IME_STRING_COMP:
				immType = GCS_COMPSTR;
			break;
			case IME_STRING_READ:
				immType = GCS_COMPREADSTR;
			break;
			case IME_STRING_RESULT:
				immType = GCS_RESULTSTR;
			break;
		}

		// do stuff
	}

	return IMEBase::GetString(type, force);
}

unsigned int TSFClient::GetCursor(bool force)
{
	if(force)
	{
		// do stuff
	}

	return IMEBase::GetCursor(force);
}

void TSFClient::Reset()
{
	// disable
}

// true means return result right away
// false means let the message fall through to the default handler
bool TSFClient::OnMessage(UINT& umsg, WPARAM& wParam, LPARAM& lParam, LRESULT& result)
{
	/*
	switch(umsg)
	{
		// handle windows message
		default:
			// do nothing
		break;
	}
	*/

	return true;
}

void TSFClient::InputChange(bool alertUI)
{
	if(alertUI && !uiIMEInputChange(GetString(IME_STRING_DESC, false), IsActive()))
		Reset();
}

bool TSFClient::IsActive()
{
	return active;
}

void TSFClient::AddCandidates(ITfCandidateListUIElement* ui)
{
	ui->GetCount((UINT*)&candidates.count);
	ui->GetCurrentPage((UINT*)&candidates.page);
	ui->GetSelection((UINT*)&candidates.selection);
	UINT pageCount = 0;
	ui->GetPageIndex(NULL, 0, &pageCount);
	if(pageCount)
	{
		UINT* pages = (UINT*)malloc(pageCount * sizeof(UINT));
		ui->GetPageIndex(pages, sizeof(pages), &pageCount);
		free(pages);
	}
	BSTR* testString = NULL;
	ui->GetString(0, testString);
}

void TSFClient::AddString(IME_STRING type, const wchar_t* value)
{
	strings[type] = value;
}

void TSFClient::AddString(IME_STRING type, std::wstring& value)
{
	AddString(type, value.c_str());
}

bool TSFStore::IsLocked(DWORD flags)
{
	return true; //(dwLockFlags & flags) ? true: false;
}

  //IUnknown methods.
STDMETHODIMP TSFStore::QueryInterface(REFIID riid, LPVOID* punk)
{
    *punk = NULL;

    if (riid == IID_IUnknown)
        *punk = reinterpret_cast<IUnknown*>(this);
    else if (riid == IID_ITextStoreACP)
        *punk = reinterpret_cast<ITextStoreACP*>(this);

    if (!*punk)
        return E_NOINTERFACE;

    return S_OK;
}

STDMETHODIMP_(DWORD) TSFStore::AddRef()
{
	return 1;
}

STDMETHODIMP_(DWORD) TSFStore::Release()
{
	return 1;
}

//ITextStoreACP methods.
STDMETHODIMP TSFStore::AdviseSink(REFIID riid, IUnknown* punk, DWORD dwMask)
{
	HRESULT						hr;
    ATL::CComPtr<IUnknown>		punkID;

    // Determine if the sink interface exists.
    // Get the pointer to the IUnknown interface and check if the IUnknown
    // pointer is the same as a pointer to an existing sink.
    // If the sink exists, update the existing sink with the
    // dwMask parameters passed to this method.
    hr = punk->QueryInterface(IID_IUnknown, (LPVOID*)&punkID);

    if(FAILED(hr))
    {
            hr = E_INVALIDARG;
    }

    if(acpSink.id != NULL && acpSink.id == punkID)
    {
            acpSink.mask = dwMask;
            hr = S_OK;
    }

	if (acpSink.sink != NULL)			// if we already have a sink, we can't add another one.
    {
        return CONNECT_E_ADVISELIMIT;
    }

    // If the sink does not exist, do the following:
    // 1. Install a new sink.
    // 2. Keep the pointer to the IUnknown interface to uniquely
    //        identify this advise sink.
    // 3. Set the dwMask parameter of this new sink to the dwMask
    //    parameters passed to this method.
    // 4. Increment the reference count.
    // 5. Release the IUnknown pointer, since this pointer is no
    //        longer required.

    if(IsEqualIID(riid, IID_ITextStoreACPSink))
    {
            punk->QueryInterface(IID_ITextStoreACPSink, (LPVOID*)&acpSink.sink);

            acpSink.id		= punkID;
            acpSink.mask	= dwMask;

            hr = S_OK;
    }

    return hr;
}

STDMETHODIMP TSFStore::UnadviseSink(IUnknown* punk)
{
	// what to do?
	acpSink.sink	= NULL;
	acpSink.id		= NULL;
	acpSink.mask	= 0;

	return S_OK;
}

STDMETHODIMP TSFStore::RequestLock(DWORD dwLockFlags, HRESULT* phrSession)
{
	/*
When RequestLock is called, the application must determine if the document is currently locked. If the document is not locked, and no other reason to deny the lock exists, the application should set phrSession to S_OK and return S_OK.

If the document is locked and the lock request is synchronous, the application should set phrSession to TS_E_SYNCHRONOUS and return S_OK. This indicates that a synchronous request cannot be granted.

If the document is locked and the lock request is asynchronous, the application should queue the request, set phrSession to TS_S_ASYNC and return S_OK. When the document becomes available, the application should remove the request from the queue and call OnLockGranted. This queuing of lock requests is optional; there is one scenario that an application must support. If the document has a read-only lock, the new lock request is read/write and the request is asynchronous, the application has called into OnLockGranted , which has caused a reentrant call to RequestLock. The application should set phrSession to TS_S_ASYNC and return S_OK. After the call to OnLockGranted returns, the application should process the upgrade request by calling OnLockGranted again with TS_LF_READWRITE.
*/
	if (phrSession == NULL)
        return E_INVALIDARG;

    if (!acpSink.sink)
        return E_UNEXPECTED;

    if (!myhwnd)
        return E_FAIL;

    HRESULT hr = S_OK;

    *phrSession = E_FAIL;

    if (dwLock != NONE)
    {
        // The document is locked, therefore since lock re-entrancy is not allowed
        // we reject synchronous lock requests and postpone asynchronous ones.
        if (dwLockFlags & TS_LF_SYNC)
        {
            // TS_E_SYNCHRONOUS means "the document can not be locked synchronously"
            *phrSession = TS_E_SYNCHRONOUS;
        }
        else if (dwLockAsync == NONE && (dwLock == READONLY) && (dwLockFlags & TS_LF_READWRITE) == TS_LF_READWRITE)
        {
            // While in a synch read-only lock, we allow one asynch read-write request.
            // It'll be postponed until the read-only session is over.
            *phrSession = TS_S_ASYNC;
            dwLockAsync = READWRITE;
        }
        else
        {
            *phrSession = E_FAIL;
            hr = E_FAIL;
        }
    }
    else
    {
        if (dwLockFlags & (TS_LF_READWRITE & ~TS_LF_READ))
        {
            //ImeEndComposition();
        }

        // Lock it and give it to the manager.
        dwLock = static_cast<LOCK_TYPES>(dwLockFlags & TS_LF_READWRITE);

		*phrSession = acpSink.sink->OnLockGranted(dwLock);

        while (dwLockAsync && acpSink.sink != NULL)
        {
            // Grant the pending async read/write lock if there's one.
            dwLock = dwLockAsync;
            dwLockAsync = NONE;
            acpSink.sink->OnLockGranted(dwLock);
        }

        dwLock = NONE;
    }

	/*
    while (!msgq.IsEmpty())
    {
        OutputDebugStringW(L"Processing Queued Message\n");
        unsigned int msg;
        uptr_t wParam;
        sptr_t lParam;
        msgq.RemoveTail(msg, wParam, lParam);
        WndProc(msg, wParam, lParam);
    }

    while (!notifyq.IsEmpty())
    {
        OutputDebugStringW(L"Processing Queued Notification\n");
        unsigned int msg;
        uptr_t wParam;
        sptr_t lParam;
        notifyq.RemoveTail(msg, wParam, lParam);
        ::SendMessage(::GetParent(MainHWND()), msg, wParam, lParam);
        if (msg == WM_NOTIFY)
        {
            delete (reinterpret_cast<SCNotification*>(lParam));
        }
    }
	*/

    return hr;
}

STDMETHODIMP TSFStore::GetStatus(TS_STATUS* pdcs)
{
    // Check if the TS_STATUS pointer is valid.
    if (pdcs == NULL)
		return E_INVALIDARG;

    // Set the dwDynamicFlags to zero to get read/write access to the document
    // and the dwStaticFlags to multiple regions.
    pdcs->dwDynamicFlags = 0; // phui view disabled ? TS_SD_READONLY
    pdcs->dwStaticFlags = 0;

    return S_OK;
}

STDMETHODIMP TSFStore::QueryInsert(LONG acpTestStart, LONG acpTestEnd, ULONG cch, LONG* pacpResultStart, LONG* pacpResultEnd)
{
/*
	The ITextStoreACP::QueryInsert method determines whether the specified start and end character positions are valid. Use this method to adjust an edit to a document before executing the edit. The method must not return values outside the range of the document.

acpTestStart [in]

    Starting application character position for inserted text.
acpTestEnd [in]

    Ending application character position for the inserted text. This value is equal to acpTextStart if the text is inserted at a point instead of replacing selected text.
cch [in]

    Length of replacement text.
pacpResultStart [out]

    Returns the new starting application character position of the inserted text. If this parameter is NULL, then text cannot be inserted at the specified position. This value cannot be outside the document range.
pacpResultEnd [out]

    Returns the new ending application character position of the inserted text. If this parameter is NULL, then pacpResultStart is set to NULL and text cannot be inserted at the specified position. This value cannot be outside the document range.

Return Value

The method can return one of the following values.
Value	Description

S_OK The method was successful.
E_FAIL An unspecified error occurred.
E_INVALIDARG The acpTestStart or acpTestEnd parameters are invalid.
*/

	if(!pacpResultStart || !pacpResultEnd)
        return E_INVALIDARG;

	*pacpResultStart = -1;
	*pacpResultEnd = -1;

    if(dwLock == NONE)
        return TS_E_NOLOCK;

	phui::TextControl* tc = client->Focus();
	if(!tc) return E_FAIL;

	const wchar_t* text = tc->WideText();

	if ((acpTestStart < 0) || (acpTestStart > acpTestEnd) || (acpTestEnd > (LONG)wcslen(text)))
        return E_INVALIDARG;

    // Set the start point to the given start point.
    *pacpResultStart = acpTestStart;
    // Set the end point to the given end point.
    *pacpResultEnd = acpTestEnd;

	return S_OK;
}

STDMETHODIMP TSFStore::GetSelection(ULONG ulIndex, ULONG ulCount, TS_SELECTION_ACP* pSelection, ULONG* pcFetched)
{
	if(!IsLocked(TS_LF_READ))
		return TS_E_NOLOCK;
	/*
The ITextStoreACP::GetSelection method returns the character position of a text selection in a document. This method supports multiple text selections. The caller must have a read-only lock on the document before calling this method.
lIndex [in]

    Specifies the text selections that start the process. If the TF_DEFAULT_SELECTION constant is specified for this parameter, the input selection starts the process.
ulCount [in]

    Specifies the maximum number of selections to return.
pSelection [out]

    Receives the style, start, and end character positions of the selected text. These values are put into the TS_SELECTION_ACP structure.
pcFetched [out]

    Receives the number of pSelection structures returned.

Return Value

The method can return one of the following values.
Value	Description

S_OK The method was successful.
TS_E_NOLOCK The caller does not have a read-only lock on the document.
TS_E_NOSELECTION The document has no selection.
*/

	if (pSelection == 0 || pcFetched == 0)
        return E_INVALIDARG;

    if (dwLock == NONE)
        return TS_E_NOLOCK;

    // we only have one selection, so if you ask for more than one, fail.
    if (ulIndex != 0 && ulIndex != TF_DEFAULT_SELECTION)
        return E_INVALIDARG;

	phui::TextControl* tc = client->Focus();
	if(!tc) return TS_E_NOSELECTION;

    *pcFetched = 1;

    if (ulCount == 0)
        return S_OK;

	int p1 = tc->GetCursorPos();
	int p2 = tc->GetHilightPos();

	if(p2 < p1)
		std::swap<int>(p1, p2);

	if(p1 < 0 || p2 < 0)
		return TS_E_NOSELECTION;

	pSelection->acpStart = p1;
    pSelection->acpEnd = p2;
    pSelection->style.ase = TS_AE_END;
    pSelection->style.fInterimChar = 0;

    return S_OK;
}

STDMETHODIMP TSFStore::SetSelection(ULONG ulCount, const TS_SELECTION_ACP* pSelection)
{
	if(!IsLocked(TS_LF_READWRITE))
		return TS_E_NOLOCK;
	/*
The ITextStoreACP::SetSelection method selects text within the document. The application must have a read/write lock on the document before calling this method.

ulCount [in]

    Specifies the number of text selections in pSelection.
pSelection [in]

    Specifies the style, start, and end character positions of the text selected through the TS_SELECTION_ACP structure.

    When the start and end character positions are equal, the method places a caret at that character position. There can be only one caret at a time in the document.

S_OK The method was successful.
E_FAIL An unspecified error occurred.
TF_E_INVALIDPOS The character positions specified are beyond the text in the document.
TF_E_NOLOCK The caller does not have a read/write lock.
*/

	if (pSelection == 0)
        return E_INVALIDARG;

    if (dwLock != READWRITE)
        return TS_E_NOLOCK;

    // we only have one selection, so if you ask for more than one, fail.
    if (ulCount != 1)
        return E_INVALIDARG;

	phui::TextControl* tc = client->Focus();
	if(!tc) return E_FAIL;

	tc->SetCursorPos((int)pSelection->acpStart);
	tc->SetHilightPos((int)pSelection->acpStart);

	return S_OK;
}

STDMETHODIMP TSFStore::GetText(LONG acpStart, LONG acpEnd, WCHAR* pchPlain, ULONG cchPlainReq, ULONG* pcchPlainOut, TS_RUNINFO* prgRunInfo, ULONG ulRunInfoReq, ULONG* pulRunInfoOut, LONG* pacpNext)
{
	if(!IsLocked(TS_LF_READ))
		return TS_E_NOLOCK;
	/*
The ITextStoreACP::GetText method returns information about text at a specified character position. This method returns the visible and hidden text and indicates if embedded data is attached to the text.
acpStart [in]

    Specifies the starting character position.
acpEnd [in]

    Specifies the ending character position. If this parameter is 1, then return all text in the text store.
pchPlain [out]

    Specifies the buffer to receive the plain text data. If this parameter is NULL, then the cchPlainReq parameter must be 0.
cchPlainReq [in]

    Specifies the number of plain text characters passed to the method.
pcchPlainRet [out]

    Receives the number of characters copied into the plain text buffer. This parameter cannot be NULL. Use a parameter if values are not required.
prgRunInfo [out]

    Receives an array of TS_RUNINFO structures. May be NULL only if cRunInfoReq = 0.
cRunInfoReq [in]

    Specifies the size, in characters, of the text run buffer.
pcRunInfoRet [out]

    Receives the number of TS_RUNINFO structures written to the text run buffer. This parameter cannot be NULL.
pacpNext [out]

    Receives the character position of the next unread character. Cannot be NULL.

S_OK The method was successful.
TF_E_INVALIDPOS The acpStart or acpEnd parameters are outside of the document text.
TF_E_NOLOCK The caller does not have a read-only lock on the document.
*/

   if (dwLock == NONE)
        return TS_E_NOLOCK;

	phui::TextControl* tc = client->Focus();
	if(!tc) return TF_E_INVALIDPOS;

	std::wstring text = tc->WideText();

	// acpEnd can be -1, initialize it properly.
    if (acpEnd == -1)
		acpEnd = (LONG)text.size();

    if ((acpStart < 0) || (acpStart > acpEnd) || (acpEnd > (LONG)text.size()))
        return TF_E_INVALIDPOS;

    // validate plain text buffers.
    bool fDoPlainText = cchPlainReq > 0;
    if (fDoPlainText && (pchPlain == NULL || pcchPlainOut == NULL))
        return E_INVALIDARG;

    // validate text run buffers.
    bool fDoTextRun = ulRunInfoReq > 0;
    if ((prgRunInfo == NULL || pulRunInfoOut == NULL))
        return E_INVALIDARG;

    // Validate window
    if (!myhwnd)
        return E_FAIL;

    *pacpNext = acpStart;
    *pcchPlainOut = 0;
    *pulRunInfoOut = 0;

    if (fDoPlainText || fDoTextRun)
    {
        int startPos		=  acpStart;
        int endPos			= acpEnd;
        ULONG cchFetched	= 0;
        ULONG cRunsFetched	= 0;

		while(cchPlainReq > 0 && startPos < endPos)
		{
			if(fDoPlainText)
			{
				*pchPlain++ = text.at(startPos);
				++cchFetched;
			}

			++startPos;
			--cchPlainReq;
		}

		if(fDoTextRun && cchFetched)
		{
			prgRunInfo->type = TS_RT_PLAIN;
			prgRunInfo->uCount = 1;
			*pulRunInfoOut = 1;
		}

		*pacpNext = startPos + 1;
		*pcchPlainOut = cchFetched;
    }

	return S_OK;
}

STDMETHODIMP TSFStore::SetText(DWORD dwFlags, LONG acpStart, LONG acpEnd, const WCHAR* pchText, ULONG cch, TS_TEXTCHANGE* pChange)
{
	if(!IsLocked(TS_LF_READWRITE))
		return TS_E_NOLOCK;
	/*
The ITextStoreACP::SetText method sets the text selection to the supplied character positions.

dwFlags [in]

    If set to the value of TS_ST_CORRECTION, the text is a transform (correction) of existing content, and any special text markup information (metadata) is retained, such as .wav file data or a language identifier. The client defines the type of markup information to be retained.
acpStart [in]

    Specifies the starting character position of the text to replace.
acpEnd [in]

    Specifies the ending character position of the text to replace. This parameter is ignored if the value is 1.
pchText [in]

    Specifies the pointer to the replacement text. The text string does not have to be NULL terminated, because the text character count is specified in the cch parameter.
cch [in]

    Specifies the number of characters in the replacement text.
pChange [out]

    Pointer to a TS_TEXTCHANGE structure with the following data.
    Value	Meaning

    acpStart The starting application character position before the text is inserted into the document.
    acpOldEnd The ending position before the text is inserted into the document. This value is the same as acpStart for an insertion point. If this value is different from acpStart, then text was selected prior to the text insertion.
    acpNewEnd The ending position after the text insertion occurred.

S_OK The method was successful.
TS_E_INVALIDPOS The acpStart or acpEnd parameter is outside of the document text.
TS_E_NOLOCK The caller does not have a read/write lock.
TS_E_READONLY The document is read-only. Content cannot be modified.
TS_E_REGION An attempt was made to modify text across a region boundary.
*/

	phui::TextControl* tc = client->Focus();
	if(!tc) return TF_E_INVALIDPOS;

    if (pchText == 0)
        return E_POINTER;

    if (dwLock != READWRITE)
        return TS_E_NOLOCK;

	if (!tc->Enabled())
        return TS_E_READONLY;

    // set text by deleting existing chars and inserting new text
    int startPos =  acpStart;
    int len = acpEnd - startPos;

	tc->RemoveText(acpStart, acpEnd);
	tc->InsertText(pchText, startPos);

    if (pChange)
    {
        pChange->acpStart = acpStart;
        pChange->acpOldEnd = acpStart + len;
        pChange->acpNewEnd = acpStart + cch;
    }

	return S_OK;
}

STDMETHODIMP TSFStore::GetFormattedText(LONG acpStart, LONG acpEnd, IDataObject** ppDataObject)
{
	//if(!IsLocked(TS_LF_READWRITE))
	//	return TS_E_NOLOCK;
	/*
The ITextStoreACP::GetFormattedText method returns formatted text data about a specified text string. The caller must have a read/write lock on the document before calling this method.
acpStart [in]

    Specifies the starting character position of the text to get in the document.
acpEnd [in]

    Specifies the ending character position of the text to get in the document. This parameter is ignored if the value is 1.
ppDataObject [out]

    Receives the pointer to the IDataObject object that contains the formatted text.

S_OK The method was successful.
TS_E_NOLOCK The caller does not have a read/write lock on the document.
*/

	return E_NOTIMPL;
}

STDMETHODIMP TSFStore::GetEmbedded(LONG acpPos, REFGUID rguidService, REFIID riid, IUnknown** ppunk)
{
	return E_NOTIMPL;
}

STDMETHODIMP TSFStore::QueryInsertEmbedded(const GUID* pguidService, const FORMATETC* pFormatEtc, BOOL* pfInsertable)
{
   if (!pfInsertable)
        return E_INVALIDARG;

	*pfInsertable = FALSE;
	return S_OK;
}

STDMETHODIMP TSFStore::InsertEmbedded(DWORD dwFlags, LONG acpStart, LONG acpEnd, IDataObject* pDataObject, TS_TEXTCHANGE* pChange)
{
     if (!pDataObject)
         return E_INVALIDARG;

     // We don't support embedded objects

     return TS_E_FORMAT;
}

STDMETHODIMP TSFStore::RequestSupportedAttrs(DWORD dwFlags, ULONG cFilterAttrs, const TS_ATTRID* paFilterAttrs)
{
	return E_NOTIMPL;
}

STDMETHODIMP TSFStore::RequestAttrsAtPosition(LONG acpPos, ULONG cFilterAttrs, const TS_ATTRID* paFilterAttrs, DWORD dwFlags)
{
	return E_NOTIMPL;
}

STDMETHODIMP TSFStore::RequestAttrsTransitioningAtPosition(LONG acpPos, ULONG cFilterAttrs, const TS_ATTRID* paFilterAttrs, DWORD dwFlags)
{
	return E_NOTIMPL;
}

STDMETHODIMP TSFStore::FindNextAttrTransition(LONG acpStart, LONG acpHalt, ULONG cFilterAttrs, const TS_ATTRID* paFilterAttrs, DWORD dwFlags, LONG* pacpNext, BOOL* pfFound, LONG* plFoundOffset)
{
	return E_NOTIMPL;
}

STDMETHODIMP TSFStore::RetrieveRequestedAttrs(ULONG ulCount, TS_ATTRVAL* paAttrVals, ULONG* pcFetched)
{
	return E_NOTIMPL;
}

STDMETHODIMP TSFStore::GetEndACP(LONG* pacp)
{
	if(!IsLocked(TS_LF_READ))
		return TS_E_NOLOCK;
/*
The ITextStoreACP::GetEndACP method returns the number of characters in a document.
pacp [out]

    Receives the character position of the last character in the document plus one.

S_OK The method was successful.
E_NOTIMPL The application has not implemented this method. This is usually an indication that calculating the end position requires excessive resources. If the end position is necessary, you can use ITextStoreACP::GetText to calculate it, though this can also be a memory-intensive operation, paging in arbitrarily large amounts of memory from disk.
TS_E_NOLOCK The caller does not have a read-only lock.
*/

	*pacp = 0;

	phui::TextControl* tc = client->Focus();

	if(tc)
	{
		*pacp = wcslen(tc->WideText());
	}

	return S_OK;
}

// return an identifier for the active view.  Since we only have one view, always return 0. Requires a READ lock.
STDMETHODIMP TSFStore::GetActiveView(TsViewCookie* pvcView)
{
   if (dwLock == NONE)
        return TS_E_NOLOCK;

    if (!pvcView)
        return E_INVALIDARG;
    else
        *pvcView = 0;			// only one view

    return S_OK;
	/*
The ITextStoreACP::GetActiveView method returns a TsViewCookie data type that specifies the current active view.

pvcView [out]

    Receives the TsViewCookie data type that specifies the current active view.
*/
}

STDMETHODIMP TSFStore::GetACPFromPoint(TsViewCookie vcView, const POINT* pt, DWORD dwFlags, LONG* pacp)
{
	/*
The ITextStoreACP::GetACPFromPoint method converts a point in screen coordinates to an application character position.
vcView [in]

    Specifies the context view.
ptScreen [in]

    Pointer to the POINT structure with the screen coordinates of the point.
dwFlags [in]

    Specifies the character position to return based upon the screen coordinates of the point relative to a character bounding box. By default, the character position returned is the character bounding box containing the screen coordinates of the point. If the point is outside a character bounding box, the method returns NULL or TF_E_INVALIDPOINT. Other bit flags for this parameter are as follows.

    The bit flags can be combined.
    Value	Meaning

    GXFPF_ROUND_NEAREST


    If the screen coordinates of the point are contained in a character bounding box, the character position returned is the bounding edge closest to the screen coordinates of the point.

    GXFPF_NEAREST


    If the screen coordinates of the point are not contained in a character bounding box, the closest character position is returned.
pacp [out]

    Receives the character position that corresponds to the screen coordinates of the point.

S_OK The method was successful.
TS_E_INVALIDPOINT The ptScreen parameter is not within the bounding box of any character.
TS_E_NOLAYOUT The application has not calculated a text layout.
*/

	return TS_E_NOLAYOUT;
}

STDMETHODIMP TSFStore::GetTextExt(TsViewCookie vcView, LONG acpStart, LONG acpEnd, RECT* prc, BOOL* pfClipped)
{
	if(!IsLocked(TS_LF_READ))
		return TS_E_NOLOCK;
	/*
The ITextStoreACP::GetTextExt method returns the bounding box, in screen coordinates, of the text at a specified character position. The caller must have a read-only lock on the document before calling this method.
cView [in]

    Specifies the context view.
acpStart [in]

    Specifies the starting character position of the text to get in the document.
acpEnd [in]

    Specifies the ending character position of the text to get in the document.
prc [out]

    Receives the bounding box in screen coordinates of the text at the specified character positions.
pfClipped [out]

    Receives a Boolean value that specifies if the text in the bounding box has been clipped. If this parameter is TRUE, the bounding box contains clipped text and does not include the entire requested text range. The bounding box is clipped because the requested range is not visible.


S_OK The method was successful.
TS_E_INVALIDARG The specified start and end character positions are equal.
TS_E_INVALIDPOS The range specified by the acpStart and acpEnd parameters extends past the beginning or end of the document.
TS_E_NOLAYOUT The application has not calculated a text layout.
TS_E_NOLOCK The caller does not have a read-only lock on the document.

Remarks

If the document window is minimized, or if the specified text is not currently visible, the method returns S_OK with the prc parameter set to {0,0,0,0}.
*/

	return TS_E_NOLAYOUT;
}

STDMETHODIMP TSFStore::GetScreenExt(TsViewCookie vcView, RECT* prc)
{
	/*
The ITextStoreACP::GetScreenExt method returns the bounding box screen coordinates of the display surface where the text stream is rendered.
vcView [in]

    Specifies the context view.
prc [out]

    Receives the bounding box screen coordinates of the display surface of the document.

S_OK The method was successful.
E_INVALIDARG The specified vcView parameter is invalid.

Remarks

If the text is not currently displayed, for example, if the document window is minimized, the prc parameter is set to { 0, 0, 0, 0 }.
*/
	return TS_E_NOLAYOUT;

	/*
	prc->bottom	= 0;
	prc->top			= 0;
	prc->left			= 0;
	prc->right		= 0;

	return S_OK;
	*/
}

STDMETHODIMP TSFStore::GetWnd(TsViewCookie vcView, HWND* phwnd)
{
	/*
The ITextStoreACP::GetWnd method returns the handle to a window that corresponds to the current document.

vcView [in]

    Specifies the TsViewCookie data type that corresponds to the current document.
phwnd [out]

    Receives a pointer to the handle of the window that corresponds to the current document. This parameter can be NULL if the document does not have the corresponding handle to the window.

S_OK The method was successful.
E_INVALIDARG The TsViewCookie data type is invalid.
*/

	*phwnd = myhwnd; // ?

	return S_OK;
}

STDMETHODIMP TSFStore::InsertTextAtSelection(DWORD dwFlags, const WCHAR* pchText, ULONG cch, LONG* pacpStart, LONG* pacpEnd, TS_TEXTCHANGE* pChange)
{
	if(!IsLocked(TS_LF_READWRITE))
		return TS_E_NOLOCK;
/*
The ITextStoreACP::InsertTextAtSelection method inserts text at the insertion point or selection. A caller must have a read/write lock on the document before inserting text.

dwFlags [in]

    Specifies whether the pacpStart and pacpEnd parameters and the TS_TEXTCHANGE structure contain the results of the text insertion.

    The TF_IAS_NOQUERY and TF_IAS_QUERYONLY flags cannot be combined.
    Value	Meaning

    0


    Text insertion will occur, and the pacpStart and pacpEnd parameters will contain the results of the text insertion. The TS_TEXTCHANGE structure must be filled with this flag.

    TF_IAS_NOQUERY


    Text is inserted, the values of the pacpStart and pacpEnd parameters can be NULL, and the TS_TEXTCHANGE structure must be filled. Use this flag to view the results of the text insertion.

    TF_IAS_QUERYONLY


    Text is not inserted, and the values for the pacpStart and pacpEnd parameters contain the results of the text insertion. The values of these parameters depend on how the application implements text insertion into a document. For more information, see the Remarks section. Use this flag to view the results of the text insertion without actually inserting the text. It is not required that you fill the TS_TEXTCHANGE structure if you use this flag.
pchText [in]

    Pointer to the string to insert in the document. The string can be NULL terminated.
cch [in]

    Specifies the text length.
pacpStart [out]

    Pointer to the starting application character position where the text insertion occurs.
pacpEnd [out]

    Pointer to the ending application character position where the text insertion occurs. This parameter value is the same as the value of the pacpStart parameter for an insertion point.
pChange [out]

    Pointer to a TS_TEXTCHANGE structure with the following members.
    Value	Meaning

    acpStart


    The starting application character position before the text is inserted into the document.

    acpOldEnd


    The ending application character position before the text is inserted into the document. This value is the same as acpStart for an insertion point. If this value is different from acpStart, then text was selected prior to the text insertion.

    acpNewEnd


    The end position after the text insertion occurred.

S_OK The method was successful.
TS_E_NOLOCK The caller does not have a lock on the document.
E_INVALIDARG The pchText parameter is invalid.
*/

   if (pchText == 0 || pChange == 0)
        return E_POINTER;

    if ((dwFlags & TS_IAS_QUERYONLY) || !(dwFlags & TS_IAS_NOQUERY))
    {
        if (!pacpStart || !pacpEnd)
            return E_POINTER;
    }

    if (dwLock != READWRITE)
        return TS_E_NOLOCK;

	phui::TextControl* tc = client->Focus();
	if(!tc) return TS_E_READONLY;

	if (!tc->Enabled())
        return TS_E_READONLY;

    TS_SELECTION_ACP acpSelection;
    ULONG cFetched(0);
    HRESULT hr = S_OK;

    hr = GetSelection(TS_DEFAULT_SELECTION, 1, &acpSelection, &cFetched);
    if (hr != S_OK) return hr;

    LONG acpResultStart;
    LONG acpResultEnd;

    hr = QueryInsert(acpSelection.acpStart, acpSelection.acpEnd, cch, &acpResultStart, &acpResultEnd);
    if (hr != S_OK) return hr;

    if (dwFlags & TS_IAS_QUERYONLY)
    {
        // Query only, return data
        *pacpStart = acpResultStart;
        *pacpEnd = acpResultEnd;
        return S_OK;
    }

    hr = SetText(0, acpResultStart, acpResultEnd, pchText, cch, pChange);
    if (hr != S_OK) return hr;

    if (!(dwFlags & TS_IAS_NOQUERY) && pChange)
    {
        *pacpStart = pChange->acpStart;
        *pacpEnd = pChange->acpNewEnd;
        //SetEmptySelection(pChange->acpNewEnd);
    }

    return hr;
}

STDMETHODIMP TSFStore::InsertEmbeddedAtSelection(DWORD dwFlags, IDataObject* pDataObject, LONG* pacpStart, LONG* pacpEnd, TS_TEXTCHANGE* pChange)
{
	return E_NOTIMPL;
}
