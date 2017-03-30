// RegionSelect.cpp : implementation file
//

#include "stdafx.h"
#include "Playgate.h"
#include "RegionSelect.h"
#include "pnRegion.h"
#include "LoginData.h"
#include "wwii/wwiiUTF.h"
#include <assert.h>

// ------------------------------ Private variables

static const char* asUTF8(const wchar_t* in)
{
	static char utf8Buffer[1024];
	WWII::ConvertChars(in, utf8Buffer, sizeof(utf8Buffer));
	return utf8Buffer;
}

// Regions define the domains we operate within,
// the default is to assume we are running Battleground Europe
// for playnet.com, but otherwise we obtain the data from
// the regions.txt file.

typedef std::vector<PNRegion> PN_REGIONS;
static PN_REGIONS g_regions;
static const PNRegion g_defaultRegion(L"Playnet", ".playnet.com", ".battlegroundeurope.com");
PNRegion g_region;

//////////////////////////////////////////////////////////////////////
// Remove the trailing spaces from the end of a string.

static void
_trimTrailingSpaces(std::wstring& text)
{
	while ( text.empty() == false )
	{
		if ( wcschr(L"\n \t\r", *text.rbegin()) == 0 )
			return;
		text.erase(text.size() - 1);
	}
}

//////////////////////////////////////////////////////////////////////
// Find the next (comma separated) string into a std::wstring

static wchar_t*
_readString(const char* name, wchar_t* p, std::wstring& into, bool allowSpaces)
{
	into.clear();
	if ( p == nullptr )
		throw region_error(name, "No data for column");

	// Skip leading spaces
	while ( *p && wcschr(L"\n \t\r", *p) )
		++p;
	if ( *p == 0 || *p == ',' )
		throw region_error(name, "No data in column");

	// Chop the string at the first ','
	wchar_t* end = wcschr(p, ',');
	if ( end != nullptr )
		*(end++) = 0;

	into = p;

	// Trim trailing spaces.
	_trimTrailingSpaces(into);

	if ( allowSpaces == true )
		return end;

	for ( UINT32 i = 0; i < into.size(); ++i )
	{
		wchar_t wc = into[i];
		if ( wc && wcschr(L"\n \t\r", wc) )
			throw region_error(name, "Spaces in column");
		if ( !isalnum(wc) && wc != '.' )
			throw region_error(name, "Invalid value");
	}

	return end;
}

//////////////////////////////////////////////////////////////////////
// Read the regions.txt file

bool
CRegionSelect::ReadRegionsFile()
{
	if ( g_regions.empty() == false )
		return true;

	g_regions.clear();

	const wchar_t* regionsTxt = L"\\regions.txt";
	std::wstring filename = g_exePath + regionsTxt;
	FILE *fp = nullptr;
	if ( _wfopen_s(&fp, filename.c_str(), L"r, ccs=UTF-8") != 0 || fp == nullptr )
	{
		dbugLog(_FLI, "No %s", asUTF8(filename.c_str()));
		return true;
	}

	static const size_t BUF_SIZE = 4095;
	wchar_t buffer[BUF_SIZE + 1];
	UINT32 line = 0;
	while ( fgetws(buffer, BUF_SIZE, fp) )
	{
		++line;

		wchar_t* p = buffer;
		while ( *p && wcschr(L"\n \t\r", *p) )
			++p;

		wchar_t* comment = wcschr(p, '#');
		if ( comment != nullptr )
		{
			*comment = 0;
		}

		// Empty lines (or lines that had comments removed)
		if ( *p == 0 )
			continue;
		dbugLog(_FLI, "regions.txt: %s", asUTF8(p));

		std::wstring name, service, web;
		try
		{
			p = _readString("Region Name", p, name, true);
			p = _readString("Auth Suffix", p, service, false);
			p = _readString("Web Suffix", p, web, false);
		}
		catch ( const region_error& error )
		{
			dbugLog(_FLW, "regions.txt:%u:%s:%hs", error.name(), error.what());
			continue;
		}
		catch ( ... )
		{
			dbugLog(_FLW, "uncaught exception");
		}

		g_regions.push_back(PNRegion(name, std::string(service.begin(), service.end()), std::string(web.begin(), web.end())));
	}

	return true;
}

// CRegionSelect dialog

IMPLEMENT_DYNAMIC(CRegionSelect, CDialog)

CRegionSelect::CRegionSelect(CWnd* pParent /*=nullptr*/)
	: CBDialog(CRegionSelect::IDD, pParent)
{
	//{{AFX_DATA_INIT(CRegionSelect)
	//}}AFX_DATA_INIT

	SetBitmap(pgLoadImage(L"bg.bmp"));
	SetBitmapStyle(StyleCenter);
}

CRegionSelect::~CRegionSelect()
{
}

void CRegionSelect::DoDataExchange(CDataExchange* pDX)
{
	CBDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_HINT, m_hint);
}


BEGIN_MESSAGE_MAP(CRegionSelect, CBDialog)
	//{{AFX_MSG_MAP(CRegionSelect)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


// CRegionSelect message handlers

// Check the registry for overriding hostIP and hostPort values
// or default to the AUTH_HOST for the name of the auth host.
// Then see if we can resolve the name/address.

#include "wwii/wwiiNetPorts.h"

static bool _lookupAuthHost()
{
	g_authPort = WWII::Net::Ports::AuthHost ;

	bool auth_override = (g_authHostName[0] != '\0') ;
	if ( auth_override )
		dbugLog(_FLI, "'%s' was passed in as a command-line argument. Forcing its use as the authhost.", g_authHostName) ;

	if ( auth_override || !Teul::GetIPs(g_region.authSuffix().c_str(), g_authHosts) || g_authHosts.empty() ) {
		if (!auth_override) {
#ifndef BETA_RELEASE
			strprintf(g_authHostName, "auth%s", g_region.authSuffix().c_str());
#else
			strprintf(g_authHostName, "authdev%s", g_region.authSuffix().c_str());
#endif
		}
		if ( !Teul::GetIPs(g_authHostName, g_authHosts) || g_authHosts.empty() ) {
			dbugLog(_FLW, "lookup(%s) no addresses", g_authHostName);
			return false;
		}
	}
	// Copy the list to a permanent home
	dbugLog(_FLI, "lookup(%s) gave", g_authHostName);
	for ( auto ip : g_authHosts ) {
		dbugLog(_FLI, " %s", teulGetAddress(ip));
	}

	return true;
}

void CRegionSelect::OnOK()
{
	CListBox* lb = (CListBox*)GetDlgItem(IDC_REGION_LIST);
	assert( lb != nullptr );

	UINT32 regionNo = lb->GetCurSel();
	g_region = g_regions[regionNo];

	if ( !_lookupAuthHost() )
	{
		int retry = MessageBox(GetLoc(ERROR_REGION), GetLoc(ERROR_REGION_T), MB_ICONQUESTION|MB_YESNO);
		if ( retry != IDYES )
		{
			playgate.StartBrowser(ERR_AUTH_RESOLVE);
			CBDialog::OnCancel();
		}
		return;
	}

	dbugLog(_FLI, "User selected region #%u %s", regionNo, asUTF8(g_region.name().c_str()));

	CBDialog::OnOK();
}

BOOL CRegionSelect::OnInitDialog()
{
	CBDialog::OnInitDialog();

	CListBox* lb = (CListBox*)GetDlgItem(IDC_REGION_LIST);
	UINT32 curSel = 0;
	for ( UINT32 i = 0; i < g_regions.size(); ++i )
	{
		const wchar_t* name = g_regions[i].name().c_str();
		lb->AddString(name);
		if ( wcscmp(loginData.m_region, name) == 0 )
			curSel = i;
	}
	lb->SetCurSel(curSel);

	return TRUE;
}

bool
RegionSelect()
{
	if ( CRegionSelect::ReadRegionsFile() == false )
	{
		dbugLog(_FLW, "No region selection - bailing");
		MessageBox(nullptr, GetLoc(ERROR_SELECTION), GetLoc(ERROR_SELECTION_T), MB_ICONSTOP|MB_OK);
		return false;
	}

	// In the absence of a regions.txt, assume CRS.
	if ( g_regions.empty() )
	{
		dbugLog(_FLI, "No regions using US defaults");
		g_regions.push_back(g_defaultRegion);
	}

	// If there is only one region, no selection.
	if ( g_regions.size() == 1 )
	{
		dbugLog(_FLI, "Auto-selecting %s", asUTF8(g_regions[0].m_name.c_str()));
		g_region = g_regions[0];
		if ( !_lookupAuthHost() )
		{
			MessageBox(nullptr, GetLoc(ERROR_AUTH_SERVER), GetLoc(ERROR_AUTH), MB_ICONSTOP|MB_OK);
			playgate.StartBrowser(ERR_AUTH_RESOLVE);
			return false;
		}
		return true;
	}

	CRegionSelect dlg;
	hMainWnd = &dlg;
	bool result = ( dlg.DoModal() == IDOK );
	hMainWnd = nullptr;

	return result;
}
