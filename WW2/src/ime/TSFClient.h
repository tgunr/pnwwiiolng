#pragma once

#include <Msctf.h>
#include <atlcomcli.h>
#include "ime/imeMain.h"

class TSFClient;

typedef ATL::CComPtr<ITfThreadMgr> TSFManager;
typedef ATL::CComPtr<ITfDocumentMgr> TSFDoc;
typedef ATL::CComPtr<ITfContext> TSFContext;

class TSFCompSink : public ITfContextOwnerCompositionSink
{
public:

	TSFCompSink::TSFCompSink() : client(NULL)
	{
	}

	virtual TSFCompSink::~TSFCompSink()
	{
	}

	// ITfContextOwnerCompositionSink
	STDMETHODIMP OnStartComposition(__RPC__in_opt ITfCompositionView* pComposition, __RPC__out BOOL* pfOk);
    STDMETHODIMP OnUpdateComposition(__RPC__in_opt ITfCompositionView* pComposition, __RPC__in_opt ITfRange* pRangeNew);
    STDMETHODIMP OnEndComposition(__RPC__in_opt ITfCompositionView* pComposition);

	// IUKnown
    STDMETHOD(QueryInterface)(REFIID, LPVOID*);
    STDMETHOD_(DWORD, AddRef)();
    STDMETHOD_(DWORD, Release)();

	TSFClient* client;
};

class TSFUISink : public ITfUIElementSink
{
public:

	TSFUISink::TSFUISink(TSFClient* c) : client(c)
	{
	}

	TSFUISink::TSFUISink() : client(NULL)
	{
	}

	virtual TSFUISink::~TSFUISink()
	{
	}

	//IUnknown methods.
    STDMETHOD(QueryInterface)(REFIID, LPVOID*);
    STDMETHOD_(DWORD, AddRef)();
    STDMETHOD_(DWORD, Release)();

	STDMETHODIMP BeginUIElement(DWORD dwUIElementId, BOOL* pbShow);
	STDMETHODIMP UpdateUIElement(DWORD dwUIElementId);
	STDMETHODIMP EndUIElement(DWORD dwUIElementId);

	bool InitManager();
	void Activate();
	void Deactivate();
	ITfUIElement* GetUI(DWORD id);

protected:

	ATL::CComPtr<ITfUIElementMgr> cpUIMgr;
	TSFClient* client;
	DWORD cookie;
};

class TSFStore : public ITextStoreACP
{
public:

    struct ADVISE_SINK
    {
		DWORD								mask;
		ATL::CComPtr<IUnknown>				id;
		ATL::CComPtr<ITextStoreACPSink>		sink;
    };

    enum LOCK_TYPES
    {
        NONE				= 0,
        READONLY		= TS_LF_READ,
        READWRITE		= TS_LF_READWRITE
    };


	TSFStore() : dwLockFlags(0), dwLock(NONE), dwLockAsync(NONE)
	{
	}

	  //IUnknown methods.
    STDMETHOD(QueryInterface)(REFIID, LPVOID*);
    STDMETHOD_(DWORD, AddRef)();
    STDMETHOD_(DWORD, Release)();

    //ITextStoreACP methods.
    STDMETHODIMP AdviseSink(REFIID riid, IUnknown* punk, DWORD dwMask);
    STDMETHODIMP UnadviseSink(IUnknown* punk);
    STDMETHODIMP RequestLock(DWORD dwLockFlags, HRESULT* phrSession);
    STDMETHODIMP GetStatus(TS_STATUS* pdcs);
    STDMETHODIMP QueryInsert(LONG acpTestStart, LONG acpTestEnd, ULONG cch, LONG* pacpResultStart, LONG* pacpResultEnd);
    STDMETHODIMP GetSelection(ULONG ulIndex, ULONG ulCount, TS_SELECTION_ACP* pSelection, ULONG* pcFetched);
    STDMETHODIMP SetSelection(ULONG ulCount, const TS_SELECTION_ACP* pSelection);
    STDMETHODIMP GetText(LONG acpStart, LONG acpEnd, WCHAR* pchPlain, ULONG cchPlainReq, ULONG* pcchPlainOut, TS_RUNINFO* prgRunInfo, ULONG ulRunInfoReq, ULONG* pulRunInfoOut, LONG* pacpNext);
    STDMETHODIMP SetText(DWORD dwFlags, LONG acpStart, LONG acpEnd, const WCHAR* pchText, ULONG cch, TS_TEXTCHANGE* pChange);
    STDMETHODIMP GetFormattedText(LONG acpStart, LONG acpEnd, IDataObject** ppDataObject);
    STDMETHODIMP GetEmbedded(LONG acpPos, REFGUID rguidService, REFIID riid, IUnknown** ppunk);
    STDMETHODIMP QueryInsertEmbedded(const GUID* pguidService, const FORMATETC* pFormatEtc, BOOL* pfInsertable);
    STDMETHODIMP InsertEmbedded(DWORD dwFlags, LONG acpStart, LONG acpEnd, IDataObject* pDataObject, TS_TEXTCHANGE* pChange);
    STDMETHODIMP RequestSupportedAttrs(DWORD dwFlags, ULONG cFilterAttrs, const TS_ATTRID* paFilterAttrs);
    STDMETHODIMP RequestAttrsAtPosition(LONG acpPos, ULONG cFilterAttrs, const TS_ATTRID* paFilterAttrs, DWORD dwFlags);
    STDMETHODIMP RequestAttrsTransitioningAtPosition(LONG acpPos, ULONG cFilterAttrs, const TS_ATTRID* paFilterAttrs, DWORD dwFlags);
    STDMETHODIMP FindNextAttrTransition(LONG acpStart, LONG acpHalt, ULONG cFilterAttrs, const TS_ATTRID* paFilterAttrs, DWORD dwFlags, LONG* pacpNext, BOOL* pfFound, LONG* plFoundOffset);
    STDMETHODIMP RetrieveRequestedAttrs(ULONG ulCount, TS_ATTRVAL* paAttrVals, ULONG* pcFetched);
    STDMETHODIMP GetEndACP(LONG* pacp);
    STDMETHODIMP GetActiveView(TsViewCookie* pvcView);
    STDMETHODIMP GetACPFromPoint(TsViewCookie vcView, const POINT* pt, DWORD dwFlags, LONG* pacp);
    STDMETHODIMP GetTextExt(TsViewCookie vcView, LONG acpStart, LONG acpEnd, RECT* prc, BOOL* pfClipped);
    STDMETHODIMP GetScreenExt(TsViewCookie vcView, RECT* prc);
    STDMETHODIMP GetWnd(TsViewCookie vcView, HWND* phwnd);
    STDMETHODIMP InsertTextAtSelection(DWORD dwFlags, const WCHAR* pchText, ULONG cch, LONG* pacpStart, LONG* pacpEnd, TS_TEXTCHANGE* pChange);
    STDMETHODIMP InsertEmbeddedAtSelection(DWORD dwFlags, IDataObject* pDataObject, LONG* pacpStart, LONG* pacpEnd, TS_TEXTCHANGE* pChange);

	bool IsLocked(DWORD flags);

	TSFClient* client;

private:

	DWORD dwLockFlags;
	LOCK_TYPES dwLock, dwLockAsync;
	ADVISE_SINK acpSink;
	unsigned int _refCount;
};

class TSFClient : public IMEBase
{
public:

	TSFClient::TSFClient() : IMEBase(), compCookie(0), acpCookie(0), clientID(0)
	{
		api = IME_API_TSF;
		uiSink = TSFUISink(this);
	}

	virtual ~TSFClient();
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

	void GetManager(TSFManager& m){ m = cpThreadMgr; }

	void AddCandidates(ITfCandidateListUIElement* ui);
	void AddString(IME_STRING type, const wchar_t* value);
	void AddString(IME_STRING type, std::wstring& value);

protected:

	// TSF: Private data
    TSFManager cpThreadMgr;
    TSFDoc cpDocMgr;
	TSFUISink uiSink;

	TfClientId clientID;

    TSFContext acpContext;
	TfEditCookie acpCookie;
	TSFStore acp;

    TSFContext compContext;
	TfEditCookie compCookie;
	TSFCompSink compSink;
	//TsViewCookie view;

	/*
    MsgQ msgq;          // queue of pending messages
    MsgQ notifyq;       // queue of pending notifications

    enum LockTypes
    {
        NONE = 0,
        READONLY = TS_LF_READ,
        READWRITE = TS_LF_READWRITE
    } dwLock, dwLockAsync;

    bool ShouldQueueMessage(unsigned int iMessage);
	*/

	/*
	ITfThreadMgrEx* manager;
	TfClientId clientID;
	ITfDocumentMgr* doc;
	ITfContext* context;
	TfEditCookie cookie;
	//TSFCompSink compSink;
	TSFUISink uiSink;
	TsViewCookie view;

	DWORD dwLock;
	ADVISE_SINK acpSink;
	*/
};
