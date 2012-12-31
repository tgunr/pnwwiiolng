#pragma once

#include <Msctf.h>
#include "ime/imeMain.h"
#include "sn/audio.h"

class IMEClient;

class CDisableCicero
{
public:
	CDisableCicero() : m_ptim( NULL ), m_bComInit( false )
	{
	}
	~CDisableCicero()
	{
		Uninitialize();
	}
	void Initialize()
	{
		if ( m_bComInit )
			return;

        HRESULT hr = S_OK;

#if XAUDIO
        // CoInitialize done in sysmain once...
#else
		hr = CoInitializeEx( NULL, COINIT_APARTMENTTHREADED );
#endif

	    if ( SUCCEEDED( hr ) )
		{
			m_bComInit = true;

			hr = CoCreateInstance( CLSID_TF_ThreadMgr,
				NULL,
				CLSCTX_INPROC_SERVER,
				__uuidof(ITfThreadMgr),
				(void**)&m_ptim );

		}
	}

	void Uninitialize()
	{
		if ( m_ptim )
		{
			m_ptim->Release();
			m_ptim = NULL;
		}

		m_bComInit = false;
	}

	void DisableCiceroOnThisWnd( HWND hwnd )
	{
		if ( m_ptim == NULL )
			return;

		ITfDocumentMgr* pdimPrev; // the dim that is associated previously.
		// Associate NULL dim to the window.
		// When this window gets the focus, Cicero does not work and IMM32 IME
		// will be activated.
		if ( SUCCEEDED( m_ptim->AssociateFocus( hwnd, NULL, &pdimPrev ) ) )
		{
			if ( pdimPrev )
				pdimPrev->Release();
		}
	}

private:
	ITfThreadMgr* m_ptim;
	bool m_bComInit;
};

static CDisableCicero g_disableCicero;

class TSFSink : public ITfUIElementSink, public ITfInputProcessorProfileActivationSink, public ITfCompartmentEventSink
{
public:

	TSFSink::TSFSink(IMEClient* c) :
		_cRef(1),
		_isTip(false),
		_client(c),
		_manager(NULL),
		_dwUIElementSinkCookie(TF_INVALID_COOKIE),
		_dwAlpnSinkCookie(TF_INVALID_COOKIE),
		_dwOpenModeSinkCookie(TF_INVALID_COOKIE),
		_dwConvModeSinkCookie(TF_INVALID_COOKIE),
		_nCandidateRefCount(0)
	{
	}

	TSFSink::TSFSink() :
		_cRef(1),
		_isTip(false),
		_client(NULL),
		_manager(NULL),
		_dwUIElementSinkCookie(TF_INVALID_COOKIE),
		_dwAlpnSinkCookie(TF_INVALID_COOKIE),
		_dwOpenModeSinkCookie(TF_INVALID_COOKIE),
		_dwConvModeSinkCookie(TF_INVALID_COOKIE),
		_nCandidateRefCount(0)
	{
	}

	virtual TSFSink::~TSFSink()
	{
	}

	//IUnknown methods.
    STDMETHOD(QueryInterface)(REFIID, LPVOID*);
    STDMETHOD_(DWORD, AddRef)();
    STDMETHOD_(DWORD, Release)();

	STDMETHODIMP BeginUIElement(DWORD dwUIElementId, BOOL* pbShow);
	STDMETHODIMP UpdateUIElement(DWORD dwUIElementId);
	STDMETHODIMP EndUIElement(DWORD dwUIElementId);

	// ITfInputProcessorProfileActivationSink
	//   Notification for keyboard input locale change
	STDMETHODIMP OnActivated(DWORD dwProfileType, LANGID langid, REFCLSID clsid, REFGUID catid,
		REFGUID guidProfile, HKL hkl, DWORD dwFlags);

	// ITfCompartmentEventSink
	//    Notification for open mode (toggle state) change
	STDMETHODIMP OnChange(REFGUID rguid);

	bool InitManager();
	void Activate();
	void Deactivate();
	ITfUIElement* GetUI(DWORD id);

	BOOL SetupSinks();
	void ReleaseSinks();
	void UpdateState(BOOL bResetCompartmentEventSink = FALSE);

	void SetReadingString(ITfReadingInformationUIElement* preading);
	void SetCandidateStrings(ITfCandidateListUIElement* pcandidate);
	BOOL GetCompartments( ITfCompartmentMgr** ppcm, ITfCompartment** ppTfOpenMode, ITfCompartment** ppTfConvMode );
	BOOL SetupCompartmentSinks( BOOL bResetOnly = FALSE, ITfCompartment* pTfOpenMode = NULL, ITfCompartment* ppTfConvMode = NULL );

	bool IsActive(){ return _active; }
	bool IsTIP(bool check = false);
	void GetDescription();

private:

	bool _active;
	bool _isTip;
	LONG _cRef;

	IMEClient* _client;
	ITfThreadMgrEx* _manager;

	DWORD _dwUIElementSinkCookie;
	DWORD _dwAlpnSinkCookie;
	DWORD _dwOpenModeSinkCookie;
	DWORD _dwConvModeSinkCookie;

	int _nCandidateRefCount;
};

class IMEClient : public IMEBase
{
public:

	enum IME_TYPE
	{
		IME_TYPE_NONE = 0,
		IME_TYPE_IMM = 1<<0,
		IME_TYPE_TSF = 1<<1,
		IME_TYPE_ANY = (IME_TYPE_IMM|IME_TYPE_TSF)
	};

	IMEClient::IMEClient() : IMEBase(IME_API_WIN),
		disabled(false),
		native(true),
		initialized(false),
		chineseIME(false),
		currentLocale(0),
		currentContext(0),
		immDLL(0)
	{
		tsfSink = TSFSink(this);
	}

	virtual ~IMEClient();
	virtual void Setup();
	virtual void Init();
	virtual void Toggle();
	virtual void SetOpenStatus(bool on);

	virtual IME_CANDIDATES& GetAllCandidates(bool force = false);
	virtual const std::wstring& GetString(IME_STRING type, bool force = true);
	virtual unsigned int GetCursor(bool force = true);

	// true means return result right away
	// false means let the message fall through to the default handler
	virtual bool OnMessage(UINT& umsg, WPARAM& wParam, LPARAM& lParam);
	virtual void OnFocus();
	virtual void OnUnfocus();
	virtual void OnInputChange();

	virtual void Activate();
	virtual void Deactivate();
	virtual bool IsActive();
	virtual bool IsIME(int type = IME_TYPE_ANY);
	virtual bool IsDisabled(){ return disabled; }

	void InitInputLocale();
	void CheckToggleState();

	// descriptions
	void SetDescription();
	void SetChineseIME(bool c){ chineseIME = c; }

	bool IsChineseIME(){ return chineseIME; }
	bool IsOldIME();
	bool IsTSF();
	bool IsTIP();
	bool IsIMM();

protected:

	HMODULE immDLL;

	bool disabled;
	bool native;
	bool initialized;
	bool chineseIME;

	HIMC currentContext;
	HKL currentLocale;

	TSFSink tsfSink;
};
