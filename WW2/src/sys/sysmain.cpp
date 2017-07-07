#include <syscfg.h> // Path is project specific.
//
#include <Windows.h>
#include <VersionHelpers.h>

//#define _WIN32_WINNT	0x0400
#include <stdarg.h>
#include "sys/sysAntihack.h"
#include "sys/sysreg.h"
#include "resource.h"
#include "pi/pi.h"
#include "mn/mngopt.h"
#include "mem/mem.h"
#include "rn/rnlogo.h"
#include "sysmain.h"
#include "systhread.h"
#include "sysSteam.h"
#include "sn/sndsSoundControl.h"
#include "gmlp/gmlpmain.h"
#include "fio/fio.h"
#include "wwii/wwiiClientMessages.h"
#include "cfg/cfgPrefs.h"
#include "wwii/wwiiCovert.h"
#include "ui/uiMain.h"
#include "rn/rnmain.h"
#include "wwii/wwiiErrorTypes.h"
#include <math.h>
#include <float.h>
#include "include/vars.h"
#include "locl/loclApp.h"
#include "sn/sndsSoundTypes.h"
#include "build_info.hpp"
#include "auth/authDefaults.h"

#ifdef CRASH_HANDLER
#include "sys/crashhandler.h"
#endif

#ifdef ANTIHACK
#include "tools/diddle.h"

HideawayStorage* storage1;
void* Imagebase;
Hideaway* checksum;
#endif

#ifndef BETA_RELEASE
static LOGICAL abcd();
#endif

#define RELEASE_ALLOW_WINDOW
#undef RELEASE_ALLOW_WINDOW

#define USE_MATH_ERR 0

#ifdef EVENT_LOG
#include <excpt.h>
#endif

//#define GLOBALS
//#undef GLOBALS

#include "fio/fioa.h"

#ifdef DIRECT3D
#include "dx3d.h"
#include "dxmain.h"
#include "dxvars.h"
#endif	// DIRECT3D

#ifdef OPENGL3D
#include "ogl/ogl.h"
#include "ddp.h"

// Externs
extern HWND				myhwnd;
#endif

// Global
LOGICAL	running;
HANDLE	threadhandle;

// Static
static void AddArchives();

#define USE_REFERENCE_RASTERIZER
#undef	USE_REFERENCE_RASTERIZER

void decompressStatus (SINT32 archive_index)
{
	if (archive_index < 0)
	{
		Write_Debug ("Done decompressing, terminating program");
		exit (0);
	}
	else
	{
		Write_Debug ("decompressStatus = %d", archive_index);
	}
}

void GetNamedFileVersion(const TCHAR** ppFFI, const TCHAR* name)
{
	static TCHAR	FFI[256];

	TCHAR*        pFFI, txt[512];
	DWORD       Nothing, VSize, Len;
	LPVOID      pVersion=nullptr;
	struct LANGANDCODEPAGE
	{
		WORD wLanguage;
		WORD wCodePage;

	} *ptranslate;


	do
	{
		VSize = GetFileVersionInfoSize(name, &Nothing);
		if (VSize == 0)
		{
			// file not found
			Write_Debug("No file version info available for %s", name);
			FFI[0] = 0;
			break;
		}

		pVersion = malloc(VSize);
		if (pVersion == nullptr)
		{
			// Severe error
			FFI[0] = 0;
			break;
		}

		if (!GetFileVersionInfo(name, 0, VSize, pVersion))
		{
			Write_Debug("Problem with GetFileVersionInfo");
			FFI[0] = 0;
			break;
		}

		if (!VerQueryValue(pVersion, _T("\\VarFileInfo\\Translation"), (LPVOID*)&ptranslate, (PUINT)&Len))
		{
			Write_Debug("Problem with VerQueryValue");
			FFI[0] = 0;
			break;
		}

		usprintf(txt, 511, _T("\\StringFileInfo\\%04x%04x\\ProductVersion"), ptranslate->wLanguage, ptranslate->wCodePage);
		if (!VerQueryValue(pVersion, txt, (LPVOID*)&pFFI, (PUINT)&Len))
		{
			Write_Debug("Problem with VerQueryValue");
			FFI[0] = 0;
			break;
		}
#ifdef _UNICODE
		lstrcpyW(FFI, pFFI);
#else
		lstrcpyA(FFI, pFFI);
#endif
	}
	while(FALSE);

	if (pVersion != nullptr)
	{
		free(pVersion);
	}

	*ppFFI = FFI;
}

void GetFileVersion(const TCHAR** ppFFI)
{
	static TCHAR	FFI[256];

	TCHAR* pFFI, txt[512], filename[256];
	DWORD Nothing, VSize, Len;
	LPVOID pVersion = nullptr;

	struct LANGANDCODEPAGE
	{
		WORD wLanguage;
		WORD wCodePage;

	} *ptranslate;


	do
	{
		if (!GetModuleFileName(nullptr, filename, sizeof(filename)))
		{
			ustrcpy(filename, _T("Unknown"));
		}

		VSize = GetFileVersionInfoSize(filename, &Nothing);
		if (VSize == 0)
		{
			// file not found
			Write_Debug("No file version info available for %s", filename);
			FFI[0] = 0;
			break;
		}

		pVersion = malloc(VSize);
		if (pVersion == nullptr)
		{
			// Severe error
			FFI[0] = 0;
			break;
		}

		if (!GetFileVersionInfo(filename, 0, VSize, pVersion))
		{
			Write_Debug("Problem with GetFileVersionInfo");
			FFI[0] = 0;
			break;
		}

		if (!VerQueryValue(pVersion, _T("\\VarFileInfo\\Translation"), (LPVOID*)&ptranslate, (PUINT)&Len))
		{
			Write_Debug("Problem with VerQueryValue");
			FFI[0] = 0;
			break;
		}

		usprintf(txt, 511, _T("\\StringFileInfo\\%04x%04x\\ProductVersion"), (UINT32)ptranslate->wLanguage, (UINT32)ptranslate->wCodePage);
		if (!VerQueryValue(pVersion, txt, (LPVOID*)&pFFI, (PUINT)&Len))
		{
			Write_Debug("Problem with VerQueryValue");
			FFI[0] = 0;
			break;
		}

		ustrcpy(FFI, pFFI);
	}
	while(FALSE);

	if (pVersion != nullptr)
	{
		free(pVersion);
	}

	*ppFFI = FFI;
}

#include "sys/sysmainHackNonsense.h"

BOOL GetWindowsVersion(DWORD* MajorVersion, DWORD* MinorVersion)
{
OSVERSIONINFO		osvi;
BOOL				bOsVersionInfo;

	memset(&osvi, 0, sizeof(OSVERSIONINFO));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

	if (!(bOsVersionInfo = GetVersionEx((OSVERSIONINFO*)&osvi)))
	{
		return FALSE;
	}

	switch (osvi.dwPlatformId)
	{
	case VER_PLATFORM_WIN32_NT:

		if (osvi.dwMajorVersion <= 4)
		{
			Write_Debug("Microsoft Windows NT");
			Write_Debug(" version %d.%d %s (Build %d)",
						osvi.dwMajorVersion,
						osvi.dwMinorVersion,
						osvi.szCSDVersion,
						osvi.dwBuildNumber & 0xFFFF);
		}
		else
		if (osvi.dwMajorVersion == 5)
		{
			if (osvi.dwMinorVersion == 0)
			{
				Write_Debug("Microsoft Windows 2000");
			}
			else
			if (osvi.dwMinorVersion == 1)
			{
				Write_Debug("Microsoft Windows XP");
			}
			else
			{
				Write_Debug("Windows version %d.%d",
							osvi.dwMajorVersion,
							osvi.dwMinorVersion);
			}

			Write_Debug(" %s (Build %d)",
						osvi.szCSDVersion,
						osvi.dwBuildNumber & 0xFFFF);
		}

		break;

	case VER_PLATFORM_WIN32_WINDOWS:

		if (osvi.dwMajorVersion == 4)
		{
			if (osvi.dwMinorVersion == 0)
			{
				Write_Debug("Microsoft Windows 95");
				if (osvi.szCSDVersion[1] == 'C' || osvi.szCSDVersion[1] == 'B')
				{
					Write_Debug(" OSR2");
				}
			}
			else
			if (osvi.dwMinorVersion == 10)
			{
				Write_Debug("Microsoft Windows 98");
				if (osvi.szCSDVersion[1] == 'A')
				{
					Write_Debug(" SE");
				}
			}
			else
			if (osvi.dwMinorVersion == 90)
			{
				Write_Debug("Microsoft Windows Me");
			}
			else
			{
				Write_Debug("Windows version %d.%d %s (Build %d)",
							osvi.dwMajorVersion,
							osvi.dwMinorVersion,
							osvi.szCSDVersion,
							osvi.dwBuildNumber & 0xFFFF);
			}
		}

		break;

	case VER_PLATFORM_WIN32s:

		Write_Debug("Microsoft Win32s");
		break;
	}

	if (MajorVersion != nullptr)
	{
		*MajorVersion = osvi.dwMajorVersion;
	}

	if (MinorVersion != nullptr)
	{
		*MinorVersion = osvi.dwMinorVersion;
	}

	return(TRUE);
}

UINT64 GetMicroSeconds(void)
{
    LARGE_INTEGER result, freq;

    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&result);
    result.QuadPart *= 1000000;
    result.QuadPart /= freq.QuadPart;
    return result.QuadPart;
}

#ifdef EVENT_LOG

#define DIV 1024

static BOOL DisplayInformation()
{
TCHAR        txt[512], filename[256];
char		divisor[]="K";
BOOL        retval=FALSE;
DWORD       Nothing, VSize, Len;
LPVOID      pVersion=nullptr;
char*		pFFI;
MEMORYSTATUS	memstat;
struct LANGANDCODEPAGE
{
	WORD wLanguage;
	WORD wCodePage;

} *ptranslate;

	do
	{
		if (!GetModuleFileName(nullptr, filename, sizeof(filename)))
		{
			ustrcpy(filename, _T("Unknown"));
		}

		VSize = GetFileVersionInfoSize(filename, &Nothing);
		if (VSize == 0)
		{
			// file not found
			Write_Debug("File not found: %s", filename);
			break;
		}

		pVersion = memCalloc(1, VSize);
		if (pVersion == nullptr)
		{
			// Severe error
			break;
		}

		if (!GetFileVersionInfo(filename, 0, VSize, pVersion))
		{
			Write_Debug("Problem with GetFileVersionInfo");
			break;
		}

		if (!VerQueryValue(pVersion, _T("\\VarFileInfo\\Translation"), (LPVOID*)&ptranslate, (PUINT)&Len))
		{
			Write_Debug("Problem with VerQueryValue");
			break;
		}

		usprintf(txt, 511, _T("\\StringFileInfo\\%04x%04x\\ProductVersion"), (UINT32)ptranslate->wLanguage, (UINT32)ptranslate->wCodePage);
		if (!VerQueryValue(pVersion, txt, (LPVOID*)&pFFI, (PUINT)&Len))
		{
			Write_Debug("Problem with VerQueryValue");
			break;
		}

		usprintf(txt, 511, _T("Product: %s\r\nVersion: %hs\r\nCommit: %hs%hs"), filename, pFFI, PN_code_git_commitish_string, PN_data_git_commitish_string);
		Write_Debug(UTF8(txt));

		GlobalMemoryStatus(&memstat);
		Write_Debug("Total physical memory:  %ld%s", memstat.dwTotalPhys/DIV, divisor);
		Write_Debug("Free physical memory:   %ld%s", memstat.dwAvailPhys/DIV, divisor);
		Write_Debug("Physical memory in use: %ld%%", memstat.dwMemoryLoad);
		Write_Debug("Total paging file:      %ld%s", memstat.dwTotalPageFile/DIV, divisor);
		Write_Debug("Free paging file:       %ld%s", memstat.dwAvailPageFile/DIV, divisor);
		Write_Debug("Total virtual memory:   %ld%s", memstat.dwTotalVirtual/DIV, divisor);
		Write_Debug("Free virtual memory:    %ld%s", memstat.dwAvailVirtual/DIV, divisor);

		retval = TRUE;
	}
	while (0);

	if (pVersion != nullptr)
	{
		memFree(pVersion);
	}

	return(retval);
}

#endif	//	EVENT_LOG

//////////////////////////////////////////////////////////////////////
//	void SetupFPU(bool enableExceptions)
//
//	Defines the behaviour of the CPUs floating point unit, including exception.
//	It can be extended for setting the SSE2 unit as well (use __control87_2).
//  This should be condidered a global definition, so make sure these states are not overwritten elsewhere!
//////////////////////////////////////////////////////////////////////
void SetupFPU(bool enableExceptions)
{
	static bool		configHasBeenPrinted	= false;


//#ifdef BETA_RELEASE
#if(DEBUG_FPU)
	unsigned int	FPUexceptionMask	= _MCW_EM;
	// Usually we don't need to detect an underflow
//	unsigned int	FPUexceptionMask = _MCW_EM - (_EM_INVALID + _EM_ZERODIVIDE + _EM_OVERFLOW + _EM_UNDERFLOW);
	if(enableExceptions)	FPUexceptionMask -= (_EM_INVALID + _EM_ZERODIVIDE + _EM_OVERFLOW);

	_clearfp();
	_control87(FPUexceptionMask, _MCW_EM);
#endif

	//float test = 0.0f;
	//float tester = 0.1f / test;
	//float tester2 = sqrt(-1.23f);

	if(configHasBeenPrinted) return;

#if defined(EVENT_LOG)
	char*			status[2]	= { "disabled", "enabled" };
	unsigned int	fp_cw		= 0;
    unsigned int	fp_mask		= 0;
    unsigned int	bits;

    fp_cw = _control87( fp_cw,
                        fp_mask );

    Write_Debug("\n===========================");
    Write_Debug("          FPU-SETUP        ");
    Write_Debug("===========================");
    Write_Debug("Interrupt Exception Masks");
    bits = fp_cw & MCW_EM;
	Write_Debug("  Invalid Operation exception	: %s", status[(bits & EM_INVALID) == 0]);
	Write_Debug("  Denormalized exception		: %s", status[(bits & EM_DENORMAL) == 0]);
	Write_Debug("  Divide-By-Zero exception		: %s", status[(bits & EM_ZERODIVIDE) == 0]);
	Write_Debug("  Overflow exception			: %s", status[(bits & EM_OVERFLOW) == 0]);
	Write_Debug("  Underflow exception			: %s", status[(bits & EM_UNDERFLOW) == 0]);
	Write_Debug("  Precision exception			: %s", status[(bits & EM_INEXACT) == 0]);
    Write_Debug("");

    bits = fp_cw & MCW_IC;
    if(bits == IC_AFFINE)		Write_Debug("Infinity Control = affine");
    if(bits == IC_PROJECTIVE )	Write_Debug("Infinity Control = projective");

    bits = fp_cw & MCW_RC;
    if(bits == RC_NEAR)       Write_Debug("Rounding Control = near");
    if(bits == RC_DOWN)       Write_Debug("Rounding Control = down");
    if(bits == RC_UP)         Write_Debug("Rounding Control = up");
    if(bits == RC_CHOP)       Write_Debug("Rounding Control = chop");

    bits = fp_cw & MCW_PC;
    if(bits == PC_24)         Write_Debug("Precision Control = 24 bits");
    if(bits == PC_53)         Write_Debug("Precision Control = 53 bits");
	if(bits == PC_64)         Write_Debug("Precision Control = 64 bits");
    Write_Debug("===========================\n");
#endif

	configHasBeenPrinted	= true;
}

//#ifndef BETA_RELEASE
#ifdef ANTIHACK
static const unsigned char FIRST_CHAR = '0';
static const unsigned char LAST_CHAR = 'f';
static unsigned short hexmap[LAST_CHAR-FIRST_CHAR+1]=
{
0,1,2,3,4,5,6,7,8,9,0,0,0,0,0,0,0,10,11,12,13,14,15,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,10,11,12,13,14,15
};

static unsigned short valueForChar( unsigned char c )
{
	if ( c >= FIRST_CHAR && c <= LAST_CHAR )
	{
		return hexmap[c-FIRST_CHAR];
	}
	return 0;
}

static void HandleHackID()
{
	if (HackID[0] && GameServerOTP[0] )
	{
		size_t len = strlen(HackID);
		if ( len == 44 )
		{
			unsigned long otp  = strtoul(GameServerOTP, nullptr, 10);
			srand(otp);
			wchar_t text[12];
			int j=0;
			for ( int i=0; i< 44; i+=4,j++ )
			{
				unsigned short valueA = valueForChar(HackID[i]);
				unsigned short valueB = valueForChar(HackID[i+1]);
				unsigned short valueC = valueForChar(HackID[i+2]);
				unsigned short valueD = valueForChar(HackID[i+3]);
				// map ABCD to wchar_t of DCBA
				text[j] = 0;
				text[j] |= valueA;
				text[j] |= valueB<<4;
				text[j] |= valueC<<8;
				text[j] |= valueD<<12;
				text[j] ^= rand();
			}
			text[j]=0;
			if ( wcschr(text,'[')!=nullptr )
			{
				// hacker
			}
			else
			{
				HackID[0]=0;
			}
		}
	}
}
#endif
//#endif

struct WinVersionInfo {
	char major;
	char minor;
	char rev;
	char build;
};

// well known version numbers for each major system
static WinVersionInfo winVersions[] = {
	{ 5, 0, 21, 95 },  // 2k
	{ 5, 1, 26, 0 },  // XP
	{ 6, 0, 6, 0 },   // vista
	{ 6, 0, 6, 2 },   // Vista SP2
	{ 6, 1, 76, 1 },  // Win7
	{ 6, 2, 92, 0 },  // Win8
	{ 6, 3, 96, 0 },  // Win8 update1
	{ 10,0,102,40 }   // Win10
};

static bool win10;

static void CheckWindows10()
{
	HKEY hKey;
	LONG res;
	win10 = false;

	res = RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"), 0, KEY_QUERY_VALUE, &hKey);
	if (res == ERROR_SUCCESS)
	{
		DWORD dwType = REG_SZ;
		char prodName[300];
		DWORD len = 300;
		res = RegQueryValueEx(hKey, _T("ProductName"), NULL, &dwType, (LPBYTE)&prodName, &len);
		if (res == ERROR_SUCCESS)
		{
			//LPWSTR
			//WideCharToMultiByte
			std::wstring pname = reinterpret_cast<LPWSTR>(prodName);
			std::wstring search = L"10";
			if (pname.find(search) != -1)
			{
				win10 = true;
			}
		}

	}

}

WinVersionInfo &getVersionInfo()
{
	if (win10)
		return winVersions[7];
	else if (IsWindows8Point1OrGreater())
		return winVersions[6];
	else if (IsWindows8OrGreater())
		return winVersions[5];
	else if (IsWindows7OrGreater())
		return winVersions[4];
	else if (IsWindowsVistaSP2OrGreater())
		return winVersions[3];
	else if (IsWindowsVistaOrGreater())
		return winVersions[2];
	else if (IsWindowsXPOrGreater())
		return winVersions[1];
	else
		return winVersions[0];
}

char GetOSMajorVersion()
{
	return getVersionInfo().major;
}

char GetOSMinorVersion()
{
	return getVersionInfo().minor;
}

char GetOSRevision()
{
	return getVersionInfo().rev;
}

char GetOSBuild()
{
	return getVersionInfo().build;
}


// This is the actual winmain
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	//NetFrameSync::InitializeHashes(nullptr); // TODO: Pass in functions to check right away.

	int		x, y, w, h, wh, xy;
	int		bpp, hdw, stencil, wnd;
	MSG		msg;
	RECT	rect;

	// Pull in the commandline arguments early so that builds
	// which rely upon a DataPath being set will have it for
	// use in cfgPrefsLoad(), and so OpenConsole is available now
	if (!mngoptParse()) {
		return 0;
	}

	// We may need to restart if this is a Steam version that wasn't launched properly.
	if ( !InitSteam() )
		return 0;

#if defined(BETA_RELEASE) || defined(ALPHA_RELEASE) || defined(DEBUG_WW2)
	dbugInit(LogMode::TO_FILE, "wwiiClient");
	dbugSetWriteLevel((Debug::ERROR_TYPE)(Debug::ERR_DBUG | (Debug::ERROR_TYPE)STD_ERR));
	dbugSetLogLevel((Debug::ERROR_TYPE)(Debug::ERR_DBUG | (Debug::ERROR_TYPE)STD_ERR));
	if (OpenConsole) {
		AllocConsole();
		freopen("CONOUT$", "wb", stdout);
		freopen("CONOUT$", "wb", stderr);
	}
#else
	dbugInit(LogMode::OFF, "wwiiClient");
	dbugSetWriteLevel((Debug::ERROR_TYPE)(Debug::ERR_INFO | (Debug::ERROR_TYPE)STD_ERR));
	dbugSetLogLevel((Debug::ERROR_TYPE)(Debug::ERR_INFO | (Debug::ERROR_TYPE)STD_ERR));
#endif

#ifdef CRASH_HANDLER
	InstallCrashHandler();
#endif

#ifdef DIRECT3D
	int		screenmode;
#endif	// DIRECT3D

#ifdef ANTIHACK
	// HACK: Used to be in the old crashhandler() function in basically this position.
	HideawayStorage st1;
	storage1 = &st1;
	Imagebase = (void*) &__ImageBase;
#endif

	CheckWindows10();

    memset(&IdentityMatrix, 0, sizeof(piMATRIX));
	IdentityMatrix.m[0][0] = IdentityMatrix.m[1][1] = 1.0;
	IdentityMatrix.m[2][2] = IdentityMatrix.m[3][3] = 1.0;

	// setup the user data path
	fioInit();

	// delete the log files
	Fio::DeleteLogFile();

	// legacy sake
	_unlink("ww2errinfo.log");
	_unlink("ww2abort.log");
	_unlink("ww2error.log");
	_unlink("ww2exit.log");
	_unlink("ww2stat.log");

	WantedWidth = GetSystemMetrics(SM_CXSCREEN);
	WantedHeight = GetSystemMetrics(SM_CYSCREEN);
	WantedBpp = TEX_BPP;
	WantedAlpha = TEX_ALPHA;
	WantedHdw = TRUE;
	WantedStencil = 0;
	WantedWindow = FALSE;
#ifdef DEBUG_WW2
	WantedUserLogEnabled = TRUE;
	//TBD: Disabled for now, force debug clients to specify so we can use it for QA testing.
	//WantDataDir = TRUE;
	HasKeyFile = TRUE;
#elif RELEASE_CLIENT == 0 && defined(BETA_RELEASE)
	WantedUserLogEnabled = FALSE;
	WantDataDir = TRUE;
	HasKeyFile = FALSE;
#endif

	SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);
	x = ((rect.right - rect.left) - WantedWidth) / 2;
	y = ((rect.bottom - rect.top) - WantedHeight) / 2;

	// Note: This is an appropriate use of the registry: a user preference it doesn't hurt to lose - Gadget
	if (sysregGetHKeyDword(HKEY_CURRENT_USER, REG_KEY_MAIN, REG_MAIN_SIZE, (DWORD*)&wh) &&
		sysregGetHKeyDword(HKEY_CURRENT_USER, REG_KEY_MAIN, REG_MAIN_LOC, (DWORD*)&xy))
	{
		if (320 <= LOWORD(wh) && 240 <= HIWORD(wh))
		{
			WantedWidth = LOWORD(wh);
			WantedHeight = HIWORD(wh);
		}

		if (rect.right > (LOWORD(xy) + LOWORD(wh)) && rect.bottom > (HIWORD(xy) + HIWORD(wh)))
		{
			x = LOWORD(xy);
			y = HIWORD(xy);
		}
	}

#if defined(BETA_RELEASE) || defined(ALPHA_RELEASE) || defined(DEBUG_WW2)
	LogEnabled = TRUE;
	UserLogEnabled = WantedUserLogEnabled;
	Open_Debug(SYS_TITLE);
	Enable_Log(LOG_ALL, WantedUserLogEnabled);
	Set_Log_Level(LOG_INFO);
#else
	LogEnabled = FALSE;
	UserLogEnabled = FALSE;
#endif

    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    if(hr != S_OK)
    {
        Write_Debug("CoInitializeEx FAILED with error: %x", hr);
    }

	SetupFPU(true);
	memInit();

#ifdef DIRECT3D
	dx3dinfoGetAdaptersAndModes(nullptr, nullptr);
#endif	// DIRECT3D

	// A registry key used to be consulted for this, but the installer never sets anything interesting for
	// that key, so we just assume it's the value it will always be. Honestly, if this actually did change
	// there's a lot else that would have to change anyhow.
	gLanguage = IDE_LANGUAGE_ENGLISH;
	gRegion = IDE_LANGUAGE_ENGLISH;

	// we then try to load wwiiol.xml variables
	// eventually this call will replace the above call
	if(!cfgPrefsLoad())
	{
		return(0);
	}

	// This call* must* follow the call to cfgLoadGlobals
	// Command line options always override configuration file values
	// But since we needed to parse these above also, the eaiest way
	// around the chicken-and-egg problem is to just do this twice.
	if (!mngoptParse())
	{
		return(0);
	}

	// Reset the globals containing auth host information for use by Steam login
	if (!gAuthServer[0]) {
		strsafecpy(gAuthServer, DEFAULT_AUTH_SERVER);
	}
	if (!gAuthServerPort[0]) {
		strprintf(gAuthServerPort, "%hu", DEFAULT_AUTH_PORT);
	}

#ifdef ANTIHACK
	HandleHackID();
#else
	HackID[0]=0;
#endif

#ifndef BETA_RELEASE
    fioSet(ISDIRECTORY);
#ifdef ANTIHACK
	checksum = new Hideaway(*storage1);
    checksum->set(diddle_crc((const unsigned char*)(
			((PIMAGE_OPTIONAL_HEADER)OPTHDROFFSET (Imagebase))->ImageBase +
			((PIMAGE_OPTIONAL_HEADER)OPTHDROFFSET (Imagebase))->BaseOfCode),
			((PIMAGE_OPTIONAL_HEADER)OPTHDROFFSET (Imagebase))->SizeOfCode));
#endif
    fioSet(ISARCHIVE);
	AddArchives();
#else

	if(WantDataDir)
	{
		fioSet(ISDIRECTORY);
		auto fd = fioOpen("blitzWor.csv");
		if ( fd < 0 ) {
			LogAbort("Incorrect/invalid data directory.");
		}
		fioClose(fd);
	}
	else
	{
		fioSet(ISARCHIVE);
		AddArchives();
	}

	// key file check
#ifndef DEBUG_WW2
	{
		//__codeMangle(92783423471, 0xdf);	// See wwiiCovert.h wtf does this do?
		// OGS: It injects a double-negative branch operation across
		// a garbage byte (0xdf) which makes the following 1kb of
		// instructions look like a data segment to disassemblers
		// like BlackIce.

		FILE* f = fopen("gizmo.key","r");
		UINT32 key = 0;
		if(f)
		{
			fscanf(f,"%d", &key);
			HasKeyFile = (key == 0xDEDDC6CD);
			fclose(f);
		}
	}
#endif
#endif

#ifdef DIRECT3D
	dxmainSetInstance(hInstance);
#endif	// DIRECT3D

#ifdef OPENGL3D
	ddpSetInstance(hInstance);
#endif	// OPENGL3D

#ifndef BETA_RELEASE
    if(abcd())
    {
        volatile int x = 1;
        volatile int y = 0;

        x = x/y;
    }
#endif

#ifdef EVENT_LOG
	Enable_Log(LOG_ALL, TRUE); // Always want this stuff logged

	Write_Debug("Command line: %s", GetCommandLine());

	GetWindowsVersion(nullptr, nullptr);

	if (!DisplayInformation())
	{
		return(0);
	}

	Enable_Log(LOG_ALL, WantedUserLogEnabled); // back to user setting
	if (g_log_comm || g_log_fps || g_log_weap)
	{
		Enable_Log(LOG_ALL, 0);
		if (g_log_comm)
		{
			Enable_Log(LOG_COMM, 1);
			Write_Log (LOG_COMM, LOG_INFO, "COMM logging enabled");
		}
		if (g_log_fps)
		{
			Enable_Log(LOG_FPS, 1);
			Write_Log (LOG_FPS, LOG_INFO, "FPS logging enabled");
		}
		if (g_log_weap)
		{
			Enable_Log(LOG_WEAP, 1);
			Write_Log (LOG_WEAP, LOG_INFO, "WEAP logging enabled");
		}
	}
#endif	//	EVENT_LOG

	w = WantedWidth;
	h = WantedHeight;
	bpp = WantedBpp;
	hdw = WantedHdw;
	stencil = WantedStencil;
	wnd = WantedWindow;

	if (!sndsDriverConfig(hInstance))
	{
		return 0;
	}

	WantedX				= x;
	WantedY				= y;
	WantedWidth			= w;
	WantedHeight		= h;
	WantedBpp			= bpp;
	WantedHdw			= hdw;
	WantedStencil		= stencil;
	WantedWindow		= wnd;

	TexBpp				= WantedBpp;
	TexAlpha			= WantedAlpha;

#if RELEASE_CLIENT != 0
	// Stencil is an option (for now) so insure it cannot be set by players
	WantedStencil		= FALSE;
#endif

	running = TRUE;

	PlayDemo	= FALSE;//TRUE;
//	Online    = FALSE;
	InFlight	= TRUE;//FALSE;

#ifdef DONT_USE
#if RELEASE_CLIENT != 0
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
#endif
#endif

	rncheckPlaygateParams();

#define DONT_USE_THREAD
#if defined(DONT_USE_THREAD) || defined(DIRECT3D)	// Use this path for a single thread environment else a multi-thread environment

#ifdef OPENGL3D

	WantedHdw = FALSE;	//// TEMPORARY (until single pass (i.e. hardware driven) multi-texturing is working)

	if (!ddpInit())
	{
		return(FALSE);
	}

	ddpDrawNotActive(true);
	ddpDisableScreenSaver(true);

	ddpSetDDInit(nullptr);//oglInit);
	ddpSetDDRender(nullptr);//rnlogoRender);
	ddpSetDDKill(nullptr);//oglKill);

	ddpSetD3DInit(piInit);
	ddpSetD3DRender(gmlpMainUpdate);
	ddpSetD3DKill(piKill);

	if (!ddpCreateWnd(WantedX, WantedY, WantedWidth, WantedHeight, WantedBpp, WantedWindow ? DDWND_FULLWINDOW : DDWND_FULLSCREEN, nullptr))
	{
		ddpKill();
		return(0);
	}
#endif	// OPENGL3D

	do
	{
		while (PeekMessage(&msg, nullptr, 0, 0, PM_NOREMOVE))
		{
			if (GetMessage(&msg, nullptr, 0, 0))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else
			{
				running = FALSE;
			}
		}

		if (running)
		{
#ifdef DIRECT3D
			dx3dRender();
#endif	// DIRECT3D

#ifdef OPENGL3D
			ddpUpdate();
#endif
		}
	}
	while (running);

#else	// DONT_USE_THREAD

	if (threadhandle = systhreadCreate(ddpEngine, &running))
	{
		do
		{
			if (GetMessage(&msg, nullptr, 0, 0))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else
			{
				running = FALSE;
			}
		}
		while (running);

		Sleep(250);	// wait for thread to run down
		CloseHandle(threadhandle);
		threadhandle = nullptr;
	}

#endif	// DONT_USE_THREAD

	fioaKill(); // make sure no fio calls after this!

#ifdef DIRECT3D
	piKill();
#endif

#ifdef OPENGL3D
	ddpKill();
#endif	// OPENGL3D

	// teulShutdownTransport();

	// we're done
	memKill();

	Close_Debug();

	CoUninitialize();

	return(msg.wParam);
}

#ifndef BETA_RELEASE
static LOGICAL abcd()
{
    TCHAR szSysDir[_MAX_PATH+1];

    HMODULE hModule1 = nullptr;
    HMODULE hModule2 = nullptr;

	// Get the handle to the OpenGL dll
    hModule1 = GetModuleHandle(_T("OPENGL32.dll"));

    // This is where the dll is supposed to be when it is loaded.
    GetSystemDirectory(szSysDir, _MAX_PATH);
    ustrcat(szSysDir, _T("\\OPENGL32.dll"));

    hModule2 = GetModuleHandle(szSysDir);

    // No OpenGL in system32 directory
    if(hModule2 == nullptr)
    {
        return TRUE;
    }

    // DLL was not loaded from system32 directory
    if(hModule1 != hModule2)
    {
        return TRUE;
    }

	hModule1 = GetModuleHandle(_T("KERNEL32.dll"));

    GetSystemDirectory(szSysDir, _MAX_PATH);
    ustrcat(szSysDir, _T("\\KERNEL32.dll"));

    hModule2 = GetModuleHandle(szSysDir);
	if(hModule2 == nullptr || hModule1 != hModule2)
	{
		return TRUE;
	}

	return FALSE;
}
#endif

#if USE_MATH_ERR
#ifdef BETA_RELEASE
#include <math.h>
#include "rn/rnmain.h"
int _matherr(struct _exception* except)
{
#pragma message("***************** MATHERR ENABLED ***************** ")

	char s[256];

	switch(except->type)
	{
		case _DOMAIN:
		case _SING:
		case _OVERFLOW:
		case _UNDERFLOW:
		case _TLOSS:
			strprintf(s, "math err type: %d, func: %s, args: %f, %f", except->type, except->name,
													(float)except->arg1, (float)except->arg2);
			rnmainListText(s);
            //piExitProgram(_FL,s);
		break;

		default:
		break;
	}

	return 0;
}
#endif // BETA_RELEASE
#endif // USE_MATH_ERR

void sysmainQuit()
{
	cfgPrefsSave();

    // Notify the ddp stuff
	extern HWND myhwnd;
	PostMessageA(myhwnd, WM_CLOSE, 0, 0);

#if RELEASE_CLIENT == 1
    // TODO: Is this really desirable behavior?
    exit(0);
#endif
}

#ifdef DONT_USE
void sysmainSetKeyboard(SYS_KEYBOARDFUNC NewKeyboardRoute)
{
	ddpSetKeyboard(NewKeyboardRoute);
}


BOOL sysmainSetTimer(SYS_TIMERFUNC newfunc)
{
	return(ddpSetTimer(newfunc));
}


void sysmainSetMouse(SYS_MOUSEFUNC NewMouseRoute)
{
	ddpSetMouse(NewMouseRoute);
}

#endif


/************************************************************************************************
	AddArchives
*************************************************************************************************/
#include "fio/fiod.h"



static void AddArchives()
{
	SCHAR** fileList = nullptr;
	SINT32 numfiles, i;
	bool foundthem=false;

	fiodGetDirFileList(nullptr, &fileList, &numfiles);
	if(!fileList)
	{
		throw std::runtime_error("Archive directory is wrong");
	}

	for(i = 0; i < numfiles; i++)
	{
		const char* s = fileList[i];
		int len = strlen(s);
		if ( len > 4 )
		{
			if ( s[len-3] == 'w' && s[len-2] == 'w' && s[len-1] == '2' ) // NEW ARCHIVES
			{
				if (fioaAddArchive(s) < 0) {
					LogAbort("Could not access archives.");
				}
				foundthem = true;
			}
		}
	}

#ifdef ANTIHACK
	checksum->set(diddle_crc((const unsigned char*)(
			((PIMAGE_OPTIONAL_HEADER)OPTHDROFFSET (Imagebase))->ImageBase +
			((PIMAGE_OPTIONAL_HEADER)OPTHDROFFSET (Imagebase))->BaseOfCode),
			((PIMAGE_OPTIONAL_HEADER)OPTHDROFFSET (Imagebase))->SizeOfCode));
#endif

	if(fileList)
	{
		for(i = 0; i < numfiles; i++)
		{
			if(fileList[i])
				memFree(fileList[i]);
		}

		memFree(fileList);
	}

	if (!foundthem) {
		LogAbort("Archives not found.");
	}
}


void sysGetMousePosition(int* x, int* y)
{
	POINT pt;
	GetCursorPos(&pt);
	ScreenToClient(myhwnd, &pt);
	*x = pt.x;
	*y = pt.y;
}

void sysSetMousePosition(int x, int y)
{
	POINT pt{x,y};
	ClientToScreen(myhwnd, &pt);
	SetCursorPos(pt.x, pt.y);
}
