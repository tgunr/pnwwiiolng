#include <syscfg.h> // Path is project specific.
#pragma once		// WTF is there a pragma once in a CPP file?

#include <Shlwapi.h>
#include "ime/IMEClient.h"
#include "sys/sysreg.h"
#include "ui/uiChat.h"
#include "ui/uiMain.h"
#include "phui/IMEControl.h"

#if platOS == OS_WIN32
#include <VersionHelpers.h>
#endif

// Definition from Win98DDK version of IMM.H
typedef struct tagINPUTCONTEXT2 {
    HWND                hWnd;
    BOOL                fOpen;
    POINT               ptStatusWndPos;
    POINT               ptSoftKbdPos;
    DWORD               fdwConversion;
    DWORD               fdwSentence;
    union   {
        LOGFONTA        A;
        LOGFONTW        W;
    } lfFont;
    COMPOSITIONFORM     cfCompForm;
    CANDIDATEFORM       cfCandForm[4];
    HIMCC               hCompStr;
    HIMCC               hCandInfo;
    HIMCC               hGuideLine;
    HIMCC               hPrivate;
    DWORD               dwNumMsgBuf;
    HIMCC               hMsgBuf;
    DWORD               fdwInit;
    DWORD               dwReserve[3];
} INPUTCONTEXT2, *PINPUTCONTEXT2, NEAR* NPINPUTCONTEXT2, FAR* LPINPUTCONTEXT2;

#define _ImmGetContext	ImmGetContext
#define _ImmReleaseContext	ImmReleaseContext
#define _ImmAssociateContext	ImmAssociateContext
static LONG (WINAPI* _ImmGetCompositionString)( HIMC himc, DWORD dwIndex, LPVOID lpBuf, DWORD dwBufLen );
#define _ImmGetOpenStatus	ImmGetOpenStatus
#define _ImmSetOpenStatus	ImmSetOpenStatus
#define _ImmGetConversionStatus	ImmGetConversionStatus
static DWORD (WINAPI* _ImmGetCandidateList)( HIMC himc, DWORD deIndex, LPCANDIDATELIST lpCandList, DWORD dwBufLen );
static LPINPUTCONTEXT2 (WINAPI* _ImmLockIMC)(HIMC hIMC);
static BOOL (WINAPI* _ImmUnlockIMC)(HIMC hIMC);
static LPVOID (WINAPI* _ImmLockIMCC)(HIMCC hIMCC);
static BOOL (WINAPI* _ImmUnlockIMCC)(HIMCC hIMCC);
#define _ImmGetDefaultIMEWnd	ImmGetDefaultIMEWnd
#define _ImmGetIMEFileNameA	ImmGetIMEFileNameA
#define _ImmGetVirtualKey	ImmGetVirtualKey
#define _ImmNotifyIME	ImmNotifyIME
#define _ImmSetConversionStatus	ImmSetConversionStatus
#define _ImmSimulateHotKey	ImmSimulateHotKey
#define _ImmIsIME	ImmIsIME
#define _ImmDisableIME	ImmDisableIME

UINT (WINAPI*_GetReadingString)(HIMC himc, UINT uReadingBufLen, LPWSTR lpwReadingBuf, PINT pnErrorIndex, BOOL* pfIsVertical, PUINT puMaxReadingLen);
BOOL (WINAPI*_ShowReadingWindow)(HIMC himc, BOOL bShow);

#define INDICATOR_NON_IME	0
#define INDICATOR_CHS		1
#define INDICATOR_CHT		2
#define INDICATOR_KOREAN	3
#define INDICATOR_JAPANESE	4

#define GETLANG()		LOWORD(currentLocale)
#define GETPRIMLANG()	((WORD)PRIMARYLANGID(GETLANG()))
#define GETSUBLANG()	SUBLANGID(GETLANG())

#define LANG_CHS MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)
#define LANG_CHT MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL)

static LPTSTR g_aszIndicator[] =
{
		TEXT("A"),
		L"\x7B80",
		L"\x7E41",
		L"\xac00",
		L"\x3042",
};

static LPTSTR g_pszIndicatior = g_aszIndicator[0];

extern HWND myhwnd;

STDAPI TSFSink::QueryInterface(REFIID riid, void** ppvObj)
{
    if (ppvObj == NULL)
        return E_INVALIDARG;

    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown))
	{
        *ppvObj = reinterpret_cast<IUnknown *>(this);
	}
	else if (IsEqualIID(riid, __uuidof(ITfUIElementSink)))
    {
        *ppvObj = (ITfUIElementSink*)this;
    }
	else if (IsEqualIID(riid, __uuidof(ITfInputProcessorProfileActivationSink)))
	{
		*ppvObj = (ITfInputProcessorProfileActivationSink*)this;
	}
	else if (IsEqualIID(riid, __uuidof(ITfCompartmentEventSink)))
	{
		*ppvObj = (ITfCompartmentEventSink*)this;
	}

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDAPI_(ULONG) TSFSink::AddRef()
{
    return ++_cRef;
}

STDAPI_(ULONG) TSFSink::Release()
{
    LONG cr = --_cRef;

    if (_cRef == 0)
    {
        delete this;
    }

    return cr;
}

BOOL TSFSink::SetupSinks()
{
	// ITfThreadMgrEx is available on Vista or later.
	HRESULT hr = CoCreateInstance(CLSID_TF_ThreadMgr,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          __uuidof(ITfThreadMgrEx),
                          (void**)&_manager);

    if (hr != S_OK)
    {
        return FALSE;
    }

    // ready to start interacting
	TfClientId cid;	// not used

    if (FAILED(_manager->ActivateEx(&cid, TF_TMAE_UIELEMENTENABLEDONLY)))
    {
        return FALSE;
    }

	// Setup sinks
	BOOL bRc = FALSE;
	ITfSource* srcTm;

	if (SUCCEEDED(hr = _manager->QueryInterface(__uuidof(ITfSource), (void** )&srcTm)))
	{
		// Sink for reading window change
		if (SUCCEEDED(hr = srcTm->AdviseSink(__uuidof(ITfUIElementSink), (ITfUIElementSink*)this, &_dwUIElementSinkCookie)))
		{
			// Sink for input locale change
			if (SUCCEEDED(hr = srcTm->AdviseSink(__uuidof(ITfInputProcessorProfileActivationSink), (ITfInputProcessorProfileActivationSink*)this, &_dwAlpnSinkCookie)))
			{
				if (SetupCompartmentSinks())	// Setup compartment sinks for the first time
				{
					bRc = TRUE;
				}
			}
		}

		srcTm->Release();
	}

	return bRc;
}

void TSFSink::ReleaseSinks()
{
	HRESULT hr;
	ITfSource* source;

	// Remove all sinks
	if ( _manager && SUCCEEDED(_manager->QueryInterface(__uuidof(ITfSource), (void** )&source)))
	{
        hr = source->UnadviseSink(_dwUIElementSinkCookie);
		hr = source->UnadviseSink(_dwAlpnSinkCookie);

        source->Release();

		SetupCompartmentSinks(TRUE);	// Remove all compartment sinks

		_manager->Deactivate();
		_manager->Release();
		_manager = NULL;
	}
}


STDAPI TSFSink::BeginUIElement(DWORD dwUIElementId, BOOL* pbShow)
{
    ITfUIElement* pElement = GetUI(dwUIElementId);

    if (!pElement)
        return E_INVALIDARG;

    ITfReadingInformationUIElement* preading = NULL;
	ITfCandidateListUIElement* pcandidate = NULL;

	*pbShow = FALSE;

	if (SUCCEEDED(pElement->QueryInterface(__uuidof(ITfReadingInformationUIElement), (void** )&preading)))
    {
		SetReadingString(preading);
        preading->Release();
    }
	else if (SUCCEEDED(pElement->QueryInterface(__uuidof(ITfCandidateListUIElement), (void** )&pcandidate)))
	{
		_nCandidateRefCount++;
		SetCandidateStrings(pcandidate);
		pcandidate->Release();
    }

    pElement->Release();

    return S_OK;
}

STDAPI TSFSink::UpdateUIElement(DWORD dwUIElementId)
{
    ITfUIElement* pElement = GetUI(dwUIElementId);

    if (!pElement)
        return E_INVALIDARG;

    ITfReadingInformationUIElement* preading = NULL;
    ITfCandidateListUIElement* pcandidate = NULL;

	if (SUCCEEDED(pElement->QueryInterface(__uuidof(ITfReadingInformationUIElement), (void** )&preading)))
    {
		SetReadingString(preading);
        preading->Release();
    }
	else if (SUCCEEDED(pElement->QueryInterface(__uuidof(ITfCandidateListUIElement), (void** )&pcandidate)))
	{
		SetCandidateStrings(pcandidate);
		pcandidate->Release();
    }

    pElement->Release();

    return S_OK;
}

STDAPI TSFSink::EndUIElement(DWORD dwUIElementId)
{
    ITfUIElement* pElement = GetUI(dwUIElementId);

    if (!pElement)
        return E_INVALIDARG;

    ITfReadingInformationUIElement* preading = NULL;

	if (SUCCEEDED(pElement->QueryInterface(__uuidof(ITfReadingInformationUIElement), (void** )&preading)))
    {
		SetReadingString(NULL);
        preading->Release();
    }

	ITfCandidateListUIElement* pcandidate = NULL;

	if (SUCCEEDED(pElement->QueryInterface(__uuidof(ITfCandidateListUIElement), (void** )&pcandidate)))
	{
		_nCandidateRefCount--;

		if (_nCandidateRefCount == 0)
			SetCandidateStrings(NULL);

		pcandidate->Release();
	}

    pElement->Release();

    return S_OK;
}

void TSFSink::UpdateState(BOOL bResetCompartmentEventSink)
{
	ITfCompartmentMgr* pcm;
	ITfCompartment* pTfOpenMode = NULL;
	ITfCompartment* pTfConvMode = NULL;

	if ( GetCompartments( &pcm, &pTfOpenMode, &pTfConvMode ) )
	{
		VARIANT valOpenMode;
		VARIANT valConvMode;

		pTfOpenMode->GetValue( &valOpenMode );
		pTfConvMode->GetValue( &valConvMode );

		if ( valOpenMode.vt == VT_I4 )
		{
			if ( _client->IsChineseIME() )
			{
				_client->SetState(valOpenMode.lVal != 0 && valConvMode.lVal != 0 ? IME_STATE_ON : IME_STATE_ENGLISH);
			}
			else
			{
				_client->SetState(valOpenMode.lVal != 0 ? IME_STATE_ON : IME_STATE_OFF);
			}
		}

		VariantClear( &valOpenMode );
		VariantClear( &valConvMode );

		if ( bResetCompartmentEventSink )
		{
			SetupCompartmentSinks( FALSE, pTfOpenMode, pTfConvMode );	// Reset compartment sinks
		}

		pTfOpenMode->Release();
		pTfConvMode->Release();
		pcm->Release();
	}
}

STDAPI TSFSink::OnActivated(DWORD dwProfileType, LANGID langid, REFCLSID clsid, REFGUID catid, REFGUID guidProfile, HKL hkl, DWORD dwFlags)
{
	static GUID TF_PROFILE_DAYI = { 0x037B2C25, 0x480C, 0x4D7F, 0xB0, 0x27, 0xD6, 0xCA, 0x6B, 0x69, 0x78, 0x8A };

	//g_iCandListIndexBase = IsEqualGUID( TF_PROFILE_DAYI, guidProfile ) ? 0 : 1;

	_isTip = false;

	if ( IsEqualIID( catid, GUID_TFCAT_TIP_KEYBOARD ) && ( dwFlags & TF_IPSINK_FLAG_ACTIVE ) )
	{
		_isTip = (dwProfileType & TF_PROFILETYPE_INPUTPROCESSOR);

		_client->SetChineseIME(_isTip && langid == LANG_CHS);

		if (_isTip )
		{
			UpdateState(TRUE);
		}
		else
		{
			_client->SetState(IME_STATE_OFF);
		}

		_client->OnInputChange();
	}

    return S_OK;
}

STDAPI TSFSink::OnChange(REFGUID rguid)
{
	UpdateState();
    return S_OK;
}

void TSFSink::SetReadingString(ITfReadingInformationUIElement* preading)
{
	/*
	UINT cchMax;
	UINT uErrorIndex = 0;
	DWORD dwFlags;

	preading->GetUpdatedFlags(&dwFlags);
	preading->GetMaxReadingStringLength(&cchMax);
	preading->GetErrorIndex(&uErrorIndex);	// errorIndex is zero-based
	*/

	BSTR bstr;

	if (preading && SUCCEEDED(preading->GetString(&bstr)))
	{
		if (bstr)
		{
			_client->AddString(IME_STRING_READ, bstr);
			_client->SetReadingString(_client->GetString(IME_STRING_READ, false));
			SysFreeString(bstr);
		}
	}
	else
	{
		_client->AddString(IME_STRING_READ, L"");
		_client->SetReadingString(_client->GetString(IME_STRING_READ, false));
	}
}

void TSFSink::SetCandidateStrings(ITfCandidateListUIElement* pcandidate)
{
	// get a reference to our list
	IME_CANDIDATES& candidates = _client->GetAllCandidates();

	// clear it
	candidates.Reset();

	if(pcandidate)
	{
		UINT uIndex			= 0;
		UINT uCount			= 0;
		UINT uCurrentPage	= 0;
		UINT* IndexList		= NULL;
		UINT uPageCnt		= 0;
		DWORD dwPageStart	= 0;
		DWORD dwPageSize	= 0;
		BSTR bstr			= NULL;

		pcandidate->GetSelection(&uIndex);
		pcandidate->GetCount(&uCount);
		pcandidate->GetCurrentPage(&uCurrentPage);
		pcandidate->GetPageIndex(NULL, 0, &uPageCnt);

		candidates.selection	= (DWORD)uIndex;
		candidates.count		= (DWORD)uCount;
		candidates.page			= (DWORD)uCurrentPage;

		if(uPageCnt > 0)
		{
			IndexList = (UINT*)malloc(sizeof(UINT)*uPageCnt);

			if(IndexList)
			{
				// get the candidates
				pcandidate->GetPageIndex(IndexList, uPageCnt, &uPageCnt);

				dwPageStart			= IndexList[uCurrentPage];
				candidates.start	= (DWORD)dwPageStart;
				candidates.page		= (uCurrentPage < uPageCnt-1) ? std::min(uCount, IndexList[uCurrentPage+1]) - dwPageStart: uCount - dwPageStart;
			}
		}

		candidates.selection = candidates.selection - candidates.start;

		for (UINT i = candidates.start; (DWORD)i < candidates.start + candidates.page; i++)
		{
			if (SUCCEEDED(pcandidate->GetString( i, &bstr )))
			{
				if(bstr)
				{
					candidates.push_back(bstr);
					SysFreeString(bstr);
				}
			}
		}

		if(IndexList)
			free(IndexList);

		if(candidates.selection >= candidates.size())
			candidates.selection = candidates.size() - 1;

		//IME_CANDIDATE_LIST& display = _client->GetCandidates(true);
		_client->SetCandidateList(0, &candidates, (int)candidates.selection);
	}
	else
	{
		_client->SetCandidateList(0, NULL);
	}
}

ITfUIElement* TSFSink::GetUI(DWORD dwUIElementId)
{
    ITfUIElementMgr* puiem;
    ITfUIElement* pElement = NULL;

	// need to lookup thread manager

    if (SUCCEEDED(_manager->QueryInterface(__uuidof(ITfUIElementMgr), (void** )&puiem)))
    {
        puiem->GetUIElement(dwUIElementId, &pElement);
        puiem->Release();
    }

    return pElement;
}

bool TSFSink::IsTIP(bool check)
{
	if(check)
	{
		_isTip = false;

		HRESULT hr;

		ITfInputProcessorProfiles* pProfiles;

		hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, NULL, CLSCTX_INPROC_SERVER, __uuidof(ITfInputProcessorProfiles), (LPVOID*)&pProfiles);

		if (SUCCEEDED(hr))
		{
			ITfInputProcessorProfileMgr* pProfileMgr;

			hr = pProfiles->QueryInterface(__uuidof(ITfInputProcessorProfileMgr), (LPVOID*)&pProfileMgr);

			if (SUCCEEDED(hr))
			{
				TF_INPUTPROCESSORPROFILE tip;
				hr = pProfileMgr->GetActiveProfile( GUID_TFCAT_TIP_KEYBOARD, &tip );

				if (SUCCEEDED(hr))
				{
					_isTip = ( tip.dwProfileType & TF_PROFILETYPE_INPUTPROCESSOR ) != 0;
				}

				pProfileMgr->Release();
			}

			pProfiles->Release();
		}
	}

	return _isTip;
}

void TSFSink::GetDescription()
{
	HRESULT hr;

	ITfInputProcessorProfiles* pProfiles;

	hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, NULL, CLSCTX_INPROC_SERVER, __uuidof(ITfInputProcessorProfiles), (LPVOID*)&pProfiles);

	if (SUCCEEDED(hr))
	{
		ITfInputProcessorProfileMgr* pProfileMgr;

		hr = pProfiles->QueryInterface(__uuidof(ITfInputProcessorProfileMgr), (LPVOID*)&pProfileMgr);

		if (SUCCEEDED(hr))
		{
			TF_INPUTPROCESSORPROFILE tip;
			hr = pProfileMgr->GetActiveProfile( GUID_TFCAT_TIP_KEYBOARD, &tip );

			if (SUCCEEDED(hr))
			{
				BSTR description;
				hr = pProfiles->GetLanguageProfileDescription(tip.clsid, tip.langid, tip.guidProfile, &description);

				if (SUCCEEDED(hr))
				{
					_client->AddString(IME_STRING_DESC, description);
					SysFreeString(description);
				}
			}

			pProfileMgr->Release();
		}

		pProfiles->Release();
	}
}

// Sets up or removes sink for UI element.
// UI element sink should be removed when IME is disabled,
// otherwise the sink can be triggered when a game has multiple instances of IME UI library.
void TSFSink::Activate()
{
	if ( _manager == NULL || _dwUIElementSinkCookie != TF_INVALID_COOKIE )
		return;

	ITfSource* srcTm = NULL;
	HRESULT hr = E_FAIL;

	if (SUCCEEDED(hr = _manager->QueryInterface(__uuidof(ITfSource), (void** )&srcTm)))
	{
		_active = true;
		hr = srcTm->AdviseSink(__uuidof(ITfUIElementSink), (ITfUIElementSink*)this, &_dwUIElementSinkCookie);
		srcTm->Release();
		//ShowComposition(_client->IsIME());
	}
}

void TSFSink::Deactivate()
{
	_active = false;
	//ShowComposition(false);

	if ( _manager == NULL || _dwUIElementSinkCookie == TF_INVALID_COOKIE )
		return;

	ITfSource* srcTm = NULL;
	HRESULT hr = E_FAIL;

	if (SUCCEEDED(hr = _manager->QueryInterface(__uuidof(ITfSource), (void** )&srcTm)))
	{
		hr = srcTm->UnadviseSink(_dwUIElementSinkCookie);
		_dwUIElementSinkCookie = TF_INVALID_COOKIE;

		srcTm->Release();
	}
}

// Returns open mode compartments and compartment manager.
// Function fails if it fails to acquire any of the objects to be returned.
BOOL TSFSink::GetCompartments( ITfCompartmentMgr** ppcm, ITfCompartment** ppTfOpenMode, ITfCompartment** ppTfConvMode )
{
	ITfCompartmentMgr* pcm = NULL;
	ITfCompartment* pTfOpenMode = NULL;
	ITfCompartment* pTfConvMode = NULL;

	static GUID _GUID_COMPARTMENT_KEYBOARD_INPUTMODE_CONVERSION = { 0xCCF05DD8, 0x4A87, 0x11D7, 0xA6, 0xE2, 0x00, 0x06, 0x5B, 0x84, 0x43, 0x5C };

	HRESULT hr;
	if (SUCCEEDED(hr = _manager->QueryInterface( IID_ITfCompartmentMgr, (void**)&pcm )))
	{
		if (SUCCEEDED(hr = pcm->GetCompartment( GUID_COMPARTMENT_KEYBOARD_OPENCLOSE, &pTfOpenMode )))
		{
			if (SUCCEEDED(hr = pcm->GetCompartment( _GUID_COMPARTMENT_KEYBOARD_INPUTMODE_CONVERSION, &pTfConvMode )))
			{
				*ppcm = pcm;
				*ppTfOpenMode = pTfOpenMode;
				*ppTfConvMode = pTfConvMode;
				return TRUE;
			}

			pTfOpenMode->Release();
		}

		pcm->Release();
	}

	return FALSE;
}

// There are three ways to call this function:
// SetupCompartmentSinks() : initialization
// SetupCompartmentSinks(FALSE, openmode, convmode) : Resetting sinks. This is necessary as DaYi and Array IME resets compartment on switching input locale
// SetupCompartmentSinks(TRUE) : clean up sinks
BOOL TSFSink::SetupCompartmentSinks( BOOL bRemoveOnly, ITfCompartment* pTfOpenMode, ITfCompartment* pTfConvMode )
{
	bool bLocalCompartments = false;
	ITfCompartmentMgr* pcm = NULL;
	BOOL bRc = FALSE;
	HRESULT hr = E_FAIL;

	if ( !pTfOpenMode && !pTfConvMode )
	{
		bLocalCompartments = true;
		GetCompartments( &pcm, &pTfOpenMode, &pTfConvMode );
	}

	if ( !( pTfOpenMode && pTfConvMode ) )
	{
		// Invalid parameters or GetCompartments() has failed.
		return FALSE;
	}

	ITfSource* srcOpenMode = NULL;

	if (SUCCEEDED(hr = pTfOpenMode->QueryInterface( IID_ITfSource, (void**)&srcOpenMode )))
	{
		// Remove existing sink for open mode
		if ( _dwOpenModeSinkCookie != TF_INVALID_COOKIE )
		{
			srcOpenMode->UnadviseSink( _dwOpenModeSinkCookie );
			_dwOpenModeSinkCookie = TF_INVALID_COOKIE;
		}

		// Setup sink for open mode (toggle state) change
		if ( bRemoveOnly || SUCCEEDED(hr = srcOpenMode->AdviseSink( IID_ITfCompartmentEventSink, (ITfCompartmentEventSink*)this, &_dwOpenModeSinkCookie )))
		{
			ITfSource* srcConvMode = NULL;
			if (SUCCEEDED(hr = pTfConvMode->QueryInterface( IID_ITfSource, (void**)&srcConvMode )))
			{
				// Remove existing sink for open mode
				if ( _dwConvModeSinkCookie != TF_INVALID_COOKIE )
				{
					srcConvMode->UnadviseSink( _dwConvModeSinkCookie );
					_dwConvModeSinkCookie = TF_INVALID_COOKIE;
				}

				// Setup sink for open mode (toggle state) change
				if ( bRemoveOnly || SUCCEEDED(hr = srcConvMode->AdviseSink( IID_ITfCompartmentEventSink, (ITfCompartmentEventSink*)this, &_dwConvModeSinkCookie )))
				{
					bRc = TRUE;
				}

				srcConvMode->Release();
			}
		}

		srcOpenMode->Release();
	}

	if ( bLocalCompartments )
	{
		pTfOpenMode->Release();
		pTfConvMode->Release();
		pcm->Release();
	}

	return bRc;
}

IMEClient::~IMEClient()
{
	if ( !initialized )
		return;

	tsfSink.ReleaseSinks();

	if(currentContext)
		_ImmReleaseContext(myhwnd, currentContext);

	currentContext = NULL;

	if ( myhwnd )
		_ImmAssociateContext(myhwnd, NULL);

	if ( immDLL )
	{
		FreeLibrary(immDLL);
		immDLL = NULL;
	}

	g_disableCicero.Uninitialize();
	initialized = false;
}

// same as SetImeApi
void IMEClient::Init()
{
	if ( initialized )
		return;

	// turn init flag on so that subsequent calls to ImeUi functions work.
	initialized = true;

	g_disableCicero.Initialize();

	//g_osi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
	//GetVersionExA(&g_osi);

	immDLL = LoadLibraryA( "imm32.dll" );
	disabled = false;

	if ( immDLL )
	{
		_ImmLockIMC =               (LPINPUTCONTEXT2 (WINAPI*)(HIMC hIMC)) GetProcAddress(immDLL, "ImmLockIMC");
		_ImmUnlockIMC =             (BOOL (WINAPI*)(HIMC hIMC)) GetProcAddress(immDLL, "ImmUnlockIMC");
		_ImmLockIMCC =              (LPVOID (WINAPI*)(HIMCC hIMCC)) GetProcAddress(immDLL, "ImmLockIMCC");
		_ImmUnlockIMCC =            (BOOL (WINAPI*)(HIMCC hIMCC)) GetProcAddress(immDLL, "ImmUnlockIMCC");

		BOOL (WINAPI* _ImmDisableTextFrameService)(DWORD) = (BOOL (WINAPI*)(DWORD))GetProcAddress(immDLL, "ImmDisableTextFrameService");

		if( _ImmDisableTextFrameService )
		{
			_ImmDisableTextFrameService( (DWORD)-1 );
		}
	}
	else
	{
		disabled = true;
		return;
	}

	_ImmGetCompositionString =	ImmGetCompositionStringW;
	_ImmGetCandidateList =		ImmGetCandidateListW;

	// disable the IME to start with
	//_ImmDisableIME(0);

	g_disableCicero.DisableCiceroOnThisWnd( myhwnd );
	currentContext = _ImmGetContext( myhwnd );

	//_ImmReleaseContext( myhwnd, currentContext );

	if ( !currentContext )
		disabled = true;

	// the following pointers to function has to be initialized before this function is called.
	if ( disabled )
	{
		Deactivate();
		return;
	}

	InitInputLocale();
	//Setup();
	//ImeUi_SetSupportLevel(2);

	// SetupTSFSinks has to be called before CheckToggleState to make it work correctly.
	bool tsfSetup = tsfSink.SetupSinks();

	CheckToggleState();

	if ( tsfSetup )
	{
		chineseIME = ( GETPRIMLANG() == LANG_CHINESE ) && tsfSink.IsTIP(true);
		tsfSink.UpdateState();
	}

#if platOS == OS_WIN32
	if (!IsWindowsVistaOrGreater())	// XP or earlier
		ActivateKeyboardLayout(currentLocale, KLF_SETFORPROCESS);
#endif

	Deactivate();

	/*
	// toggle it on
	Toggle();

	// toggle it off;
	Toggle();
	*/

	//if(piinfoIsMaxOS(PI_OS_PC, 5)) // XP
	//{
		// not sure why this is needed yet; its a hack
	//	_ImmSimulateHotKey(myhwnd, IME_CHOTKEY_IME_NONIME_TOGGLE); // turn it off

		/*
		INPUT inputs[1];

		inputs[0].type = INPUT_KEYBOARD;
		inputs[0].ki.wVk = VK_CONTROL;
		inputs[0].ki.wScan = 0;
		inputs[0].ki.dwFlags = 0;
		inputs[0].ki.time = 0;
		inputs[0].ki.dwExtraInfo = GetMessageExtraInfo();
		SendInput(1, inputs, sizeof(inputs)); // ctrl down

		inputs[0].ki.wVk = VK_SPACE; // space down
		SendInput(1, inputs, sizeof(inputs));

		inputs[0].ki.dwFlags = KEYEVENTF_KEYUP; // space up
		SendInput(1, inputs, sizeof(inputs));

		inputs[0].ki.wVk = VK_CONTROL; // ctrl up
		SendInput(1, inputs, sizeof(inputs));

		Deactivate();
		*/
	//}
}

void IMEClient::OnFocus()
{
	//ActivateKeyboardLayout(currentLocale, KLF_SETFORPROCESS);
	IMEBase::OnFocus();
	//OnInputChange();
}

void IMEClient::OnUnfocus()
{
	IMEBase::OnUnfocus();
}

IME_CANDIDATES& IMEClient::GetAllCandidates(bool force)
{
	if(force)
	{
		candidates.Reset();
		displayList.clear();

		if(!IsIME(IME_TYPE_TSF)) //  || IsOldIME()
		{
			HIMC context = ImmGetContext(myhwnd);

			if(context)
			{
				DWORD dwBufLen = 0;
				LPCANDIDATELIST list = 0;
				DWORD lcount = 0;

				ImmGetCandidateListCount(context, &lcount);

				dwBufLen = ImmGetCandidateList(context, 0, NULL, 0);

				if(dwBufLen)
				{
					list = (LPCANDIDATELIST)malloc(dwBufLen);

					// clear it
					memset(list, 0, dwBufLen);

					if(list)
					{
						ImmGetCandidateList(context, 0, list, dwBufLen);

						//wchar_t wcandidate[10+1]; // 2 bytes plus null terminator

						const char* start		= (const char*)list;
						const char* ptr			= NULL;
						const TCHAR* candidate	= NULL;

						/*
						// locale id: 0x0804
						lpLCData locale = 0;
						DWORD lsize = GetLocaleInfoEx("zh-CHS", 0, NULL, 0);

						if(lsize)
						{
							locale = (lpLCData)malloc(lsize);
							GetLocaleInfoEx("zh-CN", 0, locale, lsize);
						}
						*/

						//const char* locale = setlocale(LC_ALL, "chinese-simplified");

						for(DWORD i = 0; i < list->dwCount; i++)
						{
							DWORD offset = list->dwOffset[i];
							ptr = start;
							ptr += offset;

							candidate = (const TCHAR*)ptr;

							//memset(wcandidate, 0, sizeof(wcandidate));

							//const TCHAR* test = mbcandidate;
							//imeCandidates.push_back(utf16);

							size_t bytes = ustrlen(candidate);

							//if(mbstowcs(wcandidate, mbcandidate, bytes) > 0)
							//{
								//wcandidate[10] = L'\0';
								//wcsncpy(wcandidate, uiUTF8toUTF16("大"), 10);
								candidates.push_back(candidate);
							//}
						}

						//locale = setlocale(LC_ALL, "english");

						candidates.style		= list->dwStyle;
						candidates.count		= list->dwCount;
						candidates.selection	= list->dwSelection;
						candidates.start		= list->dwPageStart;
						candidates.page			= list->dwPageSize;

						free(list);
					}
				}

				ImmReleaseContext(myhwnd, context);
			}
		}
	}

	return IMEBase::GetAllCandidates(force);
}

const std::wstring& IMEClient::GetString(IME_STRING type, bool force)
{
	if(force)
	{
		HIMC context	= ImmGetContext(myhwnd);
		int immType		= -1;

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

		std::wstring composition = L"";
		DWORD dwSize = 0;
		char* compstr = 0;

		// updated composition string
		// show in the composition window

		if(immType >= 0)
			dwSize = ImmGetCompositionString(context, immType, NULL, 0);

		if(dwSize) // this is in bytes
		{
			DWORD strSize = dwSize+sizeof(TCHAR);

			// allocate a buffer
			compstr = (char*)malloc(strSize);
			//compstr = new char[dwSize+sizeof(TCHAR)];

			if(compstr)
			{
				memset(compstr, 0x0, strSize);

				// Get the strings that is generated by IME into compstr.
				DWORD copied = ImmGetCompositionString(context, immType, compstr, dwSize);

				if(copied > 0)
				{
					const WCHAR* wcompstr = (const WCHAR*)compstr;
					size_t wsize = wcslen(wcompstr);
					composition =  wcompstr; // save it in our static variable
				}

				// free our dynamically allocated memory
				//delete[] compstr;
				free(compstr);
			}
		}

		strings[type] = composition;

		ImmReleaseContext(myhwnd, context);
	}

	return IMEBase::GetString(type, force);
}

unsigned int IMEClient::GetCursor(bool force)
{
	if(force)
	{
		HIMC context = ImmGetContext(myhwnd);

		if(context)
		{
			cursor = ImmGetCompositionString(context, GCS_CURSORPOS, NULL, 0);
			ImmReleaseContext(myhwnd, context);
		}
	}

	return IMEBase::GetCursor(force);
}

void IMEClient::Toggle()
{
	_ImmSimulateHotKey(myhwnd, IME_CHOTKEY_IME_NONIME_TOGGLE);
	ddpPumpMessage();
	//SetOpenStatus(!IsActive());
}

// true means return result right away
// false means let the message fall through to the default handler
bool IMEClient::OnMessage(UINT& umsg, WPARAM& wParam, LPARAM& lParam)
{
	static bool debug = false;

	bool trapped = false;

	switch(umsg)
	{
		case WM_INPUTLANGCHANGE :
		{
			if(debug) uiChatPrintLine("IME: Input Change");

			OnInputChange();
		}
		break;
		case WM_IME_SETCONTEXT :
		{
			// prevents IME windows from displaying
			// because we're handling that
			lParam = 0;

			if(debug) uiChatPrintLine("IME: Set Context");
		}
		break;
		case WM_IME_STARTCOMPOSITION :
		{
			if(debug) uiChatPrintLine("IME: Start Composition");
			// should tell phui a new composition is about to begin
			// this will bring up a subclass of textcontrol that looks
			// up the current focus window and takes focus from it,
			// remembering which control it took focus from
			// it should position itself to always be "in view"
			ShowComposition(true);

			// setup return value
			trapped = true;
		}
		break;
		case WM_IME_COMPOSITION :
		{
			// handles composition string changes
			// ImmGetCompositionString(
			// params should be sent to phui to update
			// the composition string
			//DWORD dwSize = 0;
			//LPSTR lpstr = 0;

			trapped = true;

			if (lParam & GCS_RESULTSTR)
			{
				if(debug) uiChatPrintLine("IME: Result");
				//SetResultString(ddpIMEGetCompositionString(context, GCS_RESULTSTR));
				SetResultString(GetString(IME_STRING_RESULT));
			}

			if (lParam & GCS_COMPATTR)
			{
				if(debug) uiChatPrintLine("IME: Attribute");
				// composition attribute
				// do nothing for now
			}

			if (lParam & GCS_COMPCLAUSE)
			{
				if(debug) uiChatPrintLine("IME: Comp Clause");
				// composition clause info
				// do nothing for now
			}

			if (lParam & GCS_COMPREADSTR)
			{
				if(debug) uiChatPrintLine("IME: Reading String");
				// set the composition window text buffer
				//SetReadingString(ddpIMEGetCompositionString(context, GCS_COMPREADSTR));
				SetReadingString(GetString(IME_STRING_READ));
			}

			if (lParam & GCS_COMPSTR)
			{
				if(debug) uiChatPrintLine("IME: Composition");
				// set the composition window text buffer
				//SetCompositionString(ddpIMEGetCompositionString(context, GCS_COMPSTR));
				SetCompositionString(GetString(IME_STRING_COMP));

				// update the candidate list
				//ddpIMEGetCandidateList(0);
			}

			if (lParam & GCS_CURSORPOS)
			{
				if(debug) uiChatPrintLine("IME: Cursor Pos");
				// composition window cursor position
				SetCompositionCursor(GetCursor());
			}
		}
		break;
		case WM_IME_ENDCOMPOSITION :
		{
			if(debug) uiChatPrintLine("IME: End Composition");
			// tell phui to close the composition and candidate list
			// if they are visible
			ShowComposition(false);
		}
		break;
		case WM_IME_NOTIFY :
		{
			switch(wParam)
			{
				case IMN_SETOPENSTATUS: //ddpIMEInputChange();
				case IMN_OPENSTATUSWINDOW:
				case IMN_CLOSESTATUSWINDOW:
				{
					if(debug) uiChatPrintLine("IME: Open/Close Status (%d)", wParam);
					// do nothing
				}
				break;
				case IMN_GUIDELINE:
				{
					if(debug) uiChatPrintLine("IME: Guide Line");
					//ddpIMEGuideline();
				}
				break;
				case IMN_OPENCANDIDATE:
				case IMN_CHANGECANDIDATE:
				{
					if(debug) uiChatPrintLine("IME: Candidate List (%d)", wParam);

					if(IsIME(IME_TYPE_TSF)) //  && !IsOldIME()
						break;

					trapped = true;

					if(!active)
						break;

					// these messages show or modify the candidate list
					// each bit in lParam is a list that is about to be opened or changed
					// send to phui
					GetAllCandidates(true);

					// send the candidate display list to the UI
					SetCandidateList(0, &displayList, (int)candidates.Selection());

					//if(ddpIMEGetCandidateList(0) > 0)
					//	SetCandidateList(0, &imeCandidates.DisplayList(), (int)imeCandidates.Selection());
				}
				break;
				case IMN_CLOSECANDIDATE:
				{
					if(debug) uiChatPrintLine("IME: Close Candidate List");

					if(IsIME(IME_TYPE_TSF)) //  && !IsOldIME()
						break;

					trapped = true;

					// hide the candidate list
					// user has made a candidate selection
					// each bit is lParam is a list that is about to close
					// tell phui
					SetCandidateList(0, NULL);
				}
				break;
				case IMN_PRIVATE:
				{
					if(debug) uiChatPrintLine("IME: Private");
					// reading string updated due to user input
					// retrieve the reading string, send to phui and save it for later
					// use the input context content to get the reading string
					// thought it was used to get the reading string, but I think
					// this usage is obsolete
				}
				break;
				default:
				{
					trapped = true;
				}
				break;
			}
		}
		break;
	}

	return trapped;
}

/* old
void IMEClient::InputChange(bool alertUI)
{
	HIMC context = ImmGetContext(myhwnd);

	currentLocale = GetKeyboardLayout(GetCurrentThreadId());

	if(!inputLocale)
		inputLocale = currentLocale;

	DWORD conversion = 0;
	DWORD sentence = 0;

	strings[IME_STRING_DESC] = L"IME";
	native = false;

	if(ImmGetConversionStatus(context, &conversion, &sentence))
	{
		native = (conversion & IME_CMODE_NATIVE);
	}

	DWORD dsize = ImmGetDescription(currentLocale, NULL, 0);

	if(dsize)
	{
		TCHAR* description = new TCHAR[dsize+1];

		if(description)
		{
			memset(description, 0, sizeof(description));

			if(ImmGetDescription(currentLocale, description, dsize) > 0)
				strings[IME_STRING_DESC] = description;

			delete[] description;
		}
	}

	if(alertUI && !IMEBase::OnInputChange(GetString(IME_STRING_DESC, false), IsActive()))
		Reset();

	ImmReleaseContext(myhwnd, context);
}
*/

void IMEClient::Activate()
{
	if (!myhwnd) return;

	// set our activate flag
	IMEBase::Activate();

	// associate context
	//_ImmAssociateContext( myhwnd, currentContext);
	SetOpenStatus(true);

	tsfSink.Activate();

	CheckToggleState();

	if(IsIME())
	{
		phui::IMEControl* ime = Control();
		if(ime) ime->Activate();
	}
	else
	{
		Deactivate(); // something failed
	}
}

void IMEClient::Deactivate()
{
	if (!myhwnd) return;

	// set our activate flag
	IMEBase::Deactivate();

	// deactivate our sink if its active
	tsfSink.Deactivate();

	// associate context
	//_ImmAssociateContext( myhwnd, NULL);
	SetOpenStatus(false);

	// kill our UI
	phui::IMEControl* ime = Control();
	if(ime) ime->Hide();
}

bool IMEClient::IsActive()
{
	return (IMEBase::IsActive() && IsIME());
}

bool IMEClient::IsIME(int type)
{
	bool ime = false;

	if(currentLocale)
	{
		if(type & IME_TYPE_IMM)
		{
			if(_ImmIsIME( currentLocale ) && IsOldIME())
				ime = true;
		}

		if(!ime && type & IME_TYPE_TSF)
		{
			ime = IsTIP();
		}
	}

	return ime;
}

bool IMEClient::IsOldIME()
{
	return (( 0xF0000000U & (DWORD)currentLocale ) == 0xE0000000U );
}

bool IMEClient::IsTSF()
{
	return tsfSink.IsActive();
}

bool IMEClient::IsTIP()
{
	return (IsTSF() && tsfSink.IsTIP(true));
}

// called when our input changes
void IMEClient::OnInputChange()
{
	if(currentContext)
		_ImmReleaseContext(myhwnd, currentContext);

	// save our current context
	currentContext = _ImmGetContext( myhwnd );

	if(focusView)
	{
		OnUnfocus();
		focusView = NULL;
	}

	Update();
	//CheckToggleState();
	//Setup();

	// special handling
	HKL locale = GetKeyboardLayout(0);

	if(locale == (HKL)0xe0040804) // this IME causes a CTD so disable it
		Deactivate();
}

// checks whether the input is on or not
void IMEClient::CheckToggleState()
{
	InitInputLocale();

	// Hack to detect IME correctly. When IME is running as TIP, ImmIsIME() returns true for CHT US keyboard.
	bool bIme		= IsIME(IME_TYPE_IMM);
	bool bOldIme	= IsOldIME();

	// In Vista, we have to use TSF since few IMM functions don't work as expected.
	// WARNING: Because of timing, g_dwState and g_bChineseIME may not be updated
	// immediately after the change on IME states by user.
	if ( IsIME(IME_TYPE_TSF) ) // !bOldIme &&
		return;

	chineseIME = ( GETPRIMLANG() == LANG_CHINESE ) && bIme;

	HIMC himc;
	if (NULL != (himc = _ImmGetContext(myhwnd)))
	{
		if (chineseIME)
		{
			DWORD dwConvMode, dwSentMode;
			_ImmGetConversionStatus(himc, &dwConvMode, &dwSentMode);
			state = ( dwConvMode & IME_CMODE_NATIVE ) ? IME_STATE_ON : IME_STATE_ENGLISH;
		}
		else
		{
			state = ( bIme && _ImmGetOpenStatus( himc ) != 0 ) ? IME_STATE_ON : IME_STATE_OFF;
		}

		_ImmReleaseContext(myhwnd, himc);
	}
	else
	{
		state = IME_STATE_OFF;
	}
}

// sets the current locale and and description
void IMEClient::InitInputLocale()
{
	HKL locale = GetKeyboardLayout(0);

	// make sure its the active locale
	if(currentLocale != locale)
		ActivateKeyboardLayout(locale, KLF_SETFORPROCESS);

	currentLocale = locale;

	// try to find a relevant description
	SetDescription();

	/*
	char szCodePage[8];
	int iRc = GetLocaleInfoA( MAKELCID( GETLANG(), SORT_DEFAULT ), LOCALE_IDEFAULTANSICODEPAGE, szCodePage, COUNTOF( szCodePage ) ); iRc;

	g_uCodePage = _strtoul( szCodePage, NULL, 0 );

	for ( int i = 0; i < 256; i++ )
	{
		LeadByteTable[i] = (BYTE)IsDBCSLeadByteEx( g_uCodePage, (BYTE)i );
	}
	*/

	if(initialized)
		IMEBase::OnInputChange();

	return;

	/*
	DWORD conversion = 0;
	DWORD sentence = 0;

	strings[IME_STRING_DESC] = L"IME";
	native = false;

	if(ImmGetConversionStatus(context, &conversion, &sentence))
		native = (conversion & IME_CMODE_NATIVE);

	DWORD dsize = ImmGetDescription(currentLocale, NULL, 0);

	if(dsize)
	{
		TCHAR* description = new TCHAR[dsize+1];

		if(description)
		{
			memset(description, 0, sizeof(description));

			if(ImmGetDescription(currentLocale, description, dsize) > 0)
				strings[IME_STRING_DESC] = description;

			delete[] description;
		}
	}

	if(alertUI && !IMEBase::OnInputChange())
		Reset();

	ImmReleaseContext(myhwnd, context);
	*/
}

// this function tries to find a relevant description for the input
void IMEClient::SetDescription()
{
	strings[IME_STRING_DESC] = L"";

	if(IsTIP())
		tsfSink.GetDescription();

	if(strings[IME_STRING_DESC].size())
		return;

	// try the IMM api 1st
	TCHAR description[255+1];
	DWORD dsize = ImmGetDescription(currentLocale, description, 255);

	if(dsize)
	{
		//description = new TCHAR[dsize+1];

		//if(description)
		//{
			//memset(description, 0, sizeof(description));

			//if(ImmGetDescription(currentLocale, description, dsize) > 0)
				strings[IME_STRING_DESC] = description;

			//delete[] description;
		//}
	}

	if(strings[IME_STRING_DESC].size())
		return;

	// that didn't work, lets try to find the layout display name
	TCHAR localeName[KL_NAMELENGTH];
	localeName[0] = 0;

	// get the localename
	if(GetKeyboardLayoutName(localeName))
	{
		//TCHAR description[255+1];
		TCHAR key[256];

		usprintf(key, 255, _T("SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts\\%ls"), localeName);

		// first we'll see if it has a layout display indirect string
		if(sysregGetHKeyText(HKEY_LOCAL_MACHINE, key, _T("Layout Display Name"), description))
		{
			TCHAR display[256];
			if(SHLoadIndirectString(description, display, 255, NULL) == S_OK && display[0] != L'@')
				strings[IME_STRING_DESC] = display;
		}

		// that didn't work, lets try the layout text
		if(strings[IME_STRING_DESC].empty())
		{
			if(sysregGetHKeyText(HKEY_LOCAL_MACHINE, key, _T("Layout Text"), description))
				strings[IME_STRING_DESC] = description;
		}
	}

	if(strings[IME_STRING_DESC].size())
		return;

	// everything else failed, lets just set something based on the language of the current locale
	switch ( GETPRIMLANG() )
	{
		// Simplified Chinese
		case LANG_CHINESE:
			switch ( GETSUBLANG() )
			{
				case SUBLANG_CHINESE_SIMPLIFIED:
					strings[IME_STRING_DESC] = g_aszIndicator[INDICATOR_CHS];
					break;
				case SUBLANG_CHINESE_TRADITIONAL:
					strings[IME_STRING_DESC] = g_aszIndicator[INDICATOR_CHT];
					break;
				default:	// unsupported sub-language
					strings[IME_STRING_DESC] = g_aszIndicator[INDICATOR_NON_IME];
					break;
			}
			break;
		// Korean
		case LANG_KOREAN:
			strings[IME_STRING_DESC] = g_aszIndicator[INDICATOR_KOREAN];
			break;
		// Japanese
		case LANG_JAPANESE:
			strings[IME_STRING_DESC] = g_aszIndicator[INDICATOR_JAPANESE];
			break;
		default:
			strings[IME_STRING_DESC] = g_aszIndicator[INDICATOR_NON_IME];
	}
}

// this function isn't really useful
void IMEClient::Setup()
{
	_GetReadingString = NULL;
	_ShowReadingWindow = NULL;

	if(IsIME(IME_TYPE_TSF)) //!IsOldIME()
		return;

	char szImeFile[MAX_PATH + 1];

	HKL kl = currentLocale;

	if ( _ImmGetIMEFileNameA( kl, szImeFile, sizeof( szImeFile ) - 1 ) <= 0 )
		return;

	HMODULE hIme = LoadLibraryA( szImeFile );

	if ( !hIme )
		return;

	_GetReadingString = (UINT (WINAPI*)(HIMC, UINT, LPWSTR, PINT, BOOL*, PUINT))(GetProcAddress(hIme, "GetReadingString"));
	_ShowReadingWindow =(BOOL (WINAPI*)(HIMC himc, BOOL))(GetProcAddress(hIme, "ShowReadingWindow"));

	if ( _ShowReadingWindow )
	{
		HIMC himc;
		if ( NULL != ( himc = _ImmGetContext( myhwnd ) ) )
		{
			_ShowReadingWindow( himc, false );
			_ImmReleaseContext( myhwnd, himc );
		}
	}
}


void IMEClient::SetOpenStatus(bool on)
{
	if(currentContext)
	{
		if(ImmSetOpenStatus(currentContext, (BOOL)on))
		{
			INPUTCONTEXT2* ic = _ImmLockIMC(currentContext);

			if(ic)
			{
				//uiChatPrintLine("IME status is %d", ic->fOpen);
				_ImmUnlockIMC(currentContext);
			}
		}
	}
}





