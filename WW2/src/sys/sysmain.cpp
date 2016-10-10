#include <syscfg.h> // Path is project specific.
//
#include <Windows.h>
#include <VersionHelpers.h>

//#define _WIN32_WINNT	0x0400
#include <stdarg.h>
#include "sys/sysreg.h"
#include "resource.h"
#include "pi/pi.h"
#include "mn/mngopt.h"
#include "mem/mem.h"
#include "rn/rnlogo.h"
#include "sysmain.h"
#include "systhread.h"
#include "sn/sndsSoundControl.h"
#include "gmlp/gmlpmain.h"
#include "fio/fio.h"
#include "VersionNo.h"
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

#ifdef CRASH_HANDLER
#include "sys/crashhandler.h"
#endif

#ifdef ANTIHACK
#include "tools/diddle.h"

extern HideawayStorage* storage1;
extern void* Imagebase;
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

#if platOS == OS_WIN32
#ifdef EVENT_LOG

#define REG_KEY_MAIN				_T("Software\\") SYS_REGISTRY_NAME _T("\\Config")

#define cpuid __asm _emit 0x0f __asm _emit 0xa2

#define DIV 1024

int GetCpuid()
{
int CPUType;

	__asm
	{
		cpuid
		cpuid
		cpuid
		cpuid

		__asm mov CPUType, eax	; Move result of CPUID to CPU variable
	}

	_asm
	{
		_emit 0x0f			; CPUID (00001111 10100010) - This is a Pentium
							; specific instruction which gets information on the
		_emit 0xa2			; processor. A Pentium family processor should set
							; bits 11-8 of eax to 5.

		mov CPUType, eax	; Move result of CPUID to CPU variable
	}

	CPUType = (CPUType >> 8) & 0x000f;

	if (CPUType == 5)
		Write_Debug("This is a Pentium family processor.\n");
	else
		Write_Debug("This is not a Pentium family processor.\n");

	return(CPUType);
}

#pragma optimize("", off)

FLOAT32 GetCPUSpeed()
{
	FLOAT32 speed;
	LARGE_INTEGER	count1, count2, freq;
	__int64			count, rdcount, rdcount1, rdcount2;
	HANDLE			hprocess, hthread;

	hprocess = GetCurrentProcess();
	if (SetPriorityClass(hprocess, REALTIME_PRIORITY_CLASS))
	{
		Write_Debug("Process priority: REALTIME_PRIORITY_CLASS");
	}
	else
	if (SetPriorityClass(hprocess, HIGH_PRIORITY_CLASS))
	{
		Write_Debug("Process priority: HIGH_PRIORITY_CLASS");
	}
	else
	{
		Write_Debug("Process priority: NORMAL_PRIORITY_CLASS");
	}

	hthread = GetCurrentThread();
	if (SetThreadPriority(hthread, THREAD_PRIORITY_TIME_CRITICAL))
	{
		Write_Debug("Thread priority: THREAD_PRIORITY_TIME_CRITICAL");
	}
	else
	if (SetThreadPriority(hthread, THREAD_PRIORITY_HIGHEST))
	{
		Write_Debug("Thread priority: THREAD_PRIORITY_HIGHEST");
	}
	else
	if (SetThreadPriority(hthread, THREAD_PRIORITY_ABOVE_NORMAL))
	{
		Write_Debug("Thread priority: THREAD_PRIORITY_ABOVE_NORMAL");
	}
	else
	{
		Write_Debug("Thread priority: THREAD_PRIORITY_NORMAL");
	}

	speed = 0.0f;
	if (QueryPerformanceCounter(&count1))
	{
		rdcount1 = piReadTimeStamp64();
		__asm
		{
			mov ecx, 2585000
			mov eax, 0
begin:
			mov ebx, eax
			dec ebx
			inc eax
			dec ecx
			jne begin
		}

		rdcount2 = piReadTimeStamp64();
		QueryPerformanceCounter(&count2);
		rdcount = rdcount2 - rdcount1;
		count = count2.QuadPart - count1.QuadPart;
		QueryPerformanceFrequency(&freq);
		speed = (FLOAT32)((FLOAT64)freq.LowPart / (FLOAT64)count * (FLOAT64)rdcount / 1000000.0);
		Write_Debug("CPU speed calculated to be %f MHz", speed);
	}

	SetThreadPriority(hthread, THREAD_PRIORITY_NORMAL);
	SetPriorityClass(hprocess, NORMAL_PRIORITY_CLASS);

	return(speed);
}

#pragma optimize("", on)

static BOOL DisplayInformation()
{
TCHAR        txt[512], filename[256];
char		divisor[]="K";
BOOL        retval=FALSE;
DWORD       Nothing, VSize, Len, cpuspeed;
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

		if (1)//!sysregGetHKeyDword(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", "~MHz", &cpuspeed))
		{
			cpuspeed = (UINT32)GetCPUSpeed();
		}

		usprintf(txt, 511, _T("Product: %s\r\nVersion: %hs\r\nCommit: %hs"), filename, pFFI, WWII::gitCommitStr);
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
#endif	//	OS_WIN32

//////////////////////////////////////////////////////////////////////
// void _lookupRegion()
//
// Determine the appID from the build's version number and check the registry for that appID
// to see if which product region (e.g. us or china) we are supposed to be using.
//////////////////////////////////////////////////////////////////////

static void _lookupProductRegion()
{
#if GREEN_INSTALL == 1
	gLanguage = gRegion = IDE_LANGUAGE_CHINESE;
#else
	// set our defaults
	gLanguage = gRegion = IDE_LANGUAGE_ENGLISH;

	const TCHAR* region = sysregGetInstallValue(_T("Region"));

	if(region && (!ustrcmp(region, _T("china")) || !ustrcmp(region, _T("cn")) || !ustrcmp(region, _T("zh"))))
		gLanguage = gRegion = IDE_LANGUAGE_CHINESE;
#endif // GREEN_INSTALL == 1

	// uncomment to force chinese
	//gLanguage = gRegion = IDE_LANGUAGE_CHINESE;
}

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
 int WINAPI WinMainX(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
char	logfilename[_MAX_PATH];
int		x, y, w, h, wh, xy;
int		bpp, hdw, stencil, wnd;
MSG		msg;
RECT	rect;


#ifdef DIRECT3D
int		screenmode;
#endif	// DIRECT3D

	CheckWindows10();

    memset(&IdentityMatrix, 0, sizeof(piMATRIX));
	IdentityMatrix.m[0][0] = IdentityMatrix.m[1][1] = 1.0;
	IdentityMatrix.m[2][2] = IdentityMatrix.m[3][3] = 1.0;

	// setup the user data path
	fioInit();

	wwiiSendLog_WWII_ERR_INFO();

	// delete the log files
	Fio::DeleteLogFile();

	// legacy sake
	_unlink("ww2errinfo.log");
	_unlink("ww2abort.log");
	_unlink("ww2error.log");
	_unlink("ww2exit.log");
	_unlink("ww2stat.log");

	strcpy(logfilename, SYS_TITLE);
#ifdef _DEBUG
	strcat(logfilename, "_Debug");
#endif
	strcat(logfilename, ".log");
	_unlink(logfilename);

#ifdef TERRAIN_EDITOR
	WantedWidth = 800;
	WantedHeight = 600;
	WantedBpp = TEX_BPP;
	WantedAlpha = TEX_ALPHA;
	WantedHdw = TRUE;
	WantedStencil = 0;
	WantedWindow = TRUE;
	WantedUserLogEnabled = FALSE;
#else
	WantedWidth = GetSystemMetrics(SM_CXSCREEN);
	WantedHeight = GetSystemMetrics(SM_CYSCREEN);
	WantedBpp = TEX_BPP;
	WantedAlpha = TEX_ALPHA;
	WantedHdw = TRUE;
	WantedStencil = 0;
	WantedWindow = FALSE;
#ifdef DEBUG_WW2
	WantedUserLogEnabled = TRUE;
	WantDataDir = TRUE;
	HasKeyFile = TRUE;
#elif RELEASE_CLIENT == 0 && defined(BETA_RELEASE)
	WantedUserLogEnabled = FALSE;
	WantDataDir = TRUE;
	HasKeyFile = FALSE;
#endif
#endif

	SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);
	x = ((rect.right - rect.left) - WantedWidth) / 2;
	y = ((rect.bottom - rect.top) - WantedHeight) / 2;

	SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);
	if (sysregGetKeyDword(REG_KEY_MAIN, REG_MAIN_SIZE, (DWORD*)&wh) &&
		sysregGetKeyDword(REG_KEY_MAIN, REG_MAIN_LOC, (DWORD*)&xy))
	{
		if (320 <= LOWORD(wh) &&
			240 <= HIWORD(wh))
		{
			WantedWidth = LOWORD(wh);
			WantedHeight = HIWORD(wh);
		}

		if (rect.right > (LOWORD(xy) + LOWORD(wh)) &&
			rect.bottom > (HIWORD(xy) + HIWORD(wh)))
		{
			x = LOWORD(xy);
			y = HIWORD(xy);
		}
	}
#ifdef TERRAIN_EDITOR
	else
	{
		x = GetSystemMetrics(SM_CXFRAME);
		y = GetSystemMetrics(SM_CYFRAME) +
			GetSystemMetrics(SM_CYCAPTION);
	}
#endif

#ifdef BETA_RELEASE
	if (OpenConsole)
	{
		AllocConsole();
		freopen("CONOUT$", "wb", stdout);
		freopen("CONOUT$", "wb", stderr);
	}

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

	// look up the product region (us/china)
	_lookupProductRegion();

	// Pull in the commandline arguments early so that builds
	// which rely upon a DataPath being set will have it for
	// use in cfgPrefsLoad().
	if (!mngoptParse(""))
	{
		return(0);
	}

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
	if (!mngoptParse(""))
	{
		return(0);
	}

#ifdef ANTIHACK
	HandleHackID();
#else
	HackID[0]=0;
#endif

#ifndef TERRAIN_EDITOR
	// FIXME syscineWinMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
#endif

#ifdef TERRAIN_EDITOR
	WantedWindow = TRUE;
#endif

#ifdef DONT_USE
	if (!syshsCheckHdwSfw(FALSE))
	{
		return(0);
	}
#endif

#ifndef BETA_RELEASE
#if FRES
    fioSet(ISDIRECTORY);
#else
#ifdef ANTIHACK
	checksum = new Hideaway(*storage1);
    checksum->set(diddle_crc((const unsigned char*)(
			((PIMAGE_OPTIONAL_HEADER)OPTHDROFFSET (Imagebase))->ImageBase +
			((PIMAGE_OPTIONAL_HEADER)OPTHDROFFSET (Imagebase))->BaseOfCode),
			((PIMAGE_OPTIONAL_HEADER)OPTHDROFFSET (Imagebase))->SizeOfCode));
#endif
    fioSet(ISARCHIVE);
	AddArchives();
#endif // FRES
#else

	if(WantDataDir)
	{
		fioSet(ISDIRECTORY);
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

#endif	//	EVENT_LOG

#ifdef EVENT_LOG
#if platOS == OS_WIN32
	GetWindowsVersion(nullptr, nullptr);

	if (!DisplayInformation())
	{
		return(0);
	}
#endif	//	OS_WIN32
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

//#ifdef TERRAIN_EDITOR

#ifdef DONT_USE
#if RELEASE_CLIENT != 0
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
#endif
#endif

	UINT32 host = 0, otp = 0;
	UINT16 port = 0;
	if(rnmainGetOnlineSetting() == false)
		return FALSE;

#define DONT_USE_THREAD
#if defined(DONT_USE_THREAD) || defined(DIRECT3D)	// Use this path for a single thread environment else a multi-thread environment

#ifdef OPENGL3D

	WantedHdw = FALSE;	//// TEMPORARY (until single pass (i.e. hardware driven) multi-texturing is working)

	if (!ddpInit())
	{
		return(FALSE);
	}

//	char * ptr = 0; *ptr = 0; // simulated crash

	ddpDrawNotActive(TRUE);
	ddpDisableScreenSaver(TRUE);

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
			sprintf(s, "math err type: %d, func: %s, args: %f, %f", except->type, except->name,
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
				fioaAddArchive(s);
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

	if ( !foundthem )
	{
		throw std::runtime_error("Archives not found");
	}
}


void sysGetMousePosition(int* x, int* y)
{
	POINT pt;
	GetCursorPos(&pt);
	*x = pt.x - WantedX;
	*y = pt.y - WantedY;
}

void sysSetMousePosition(int x, int y)
{
	SetCursorPos(WantedX + x, WantedY + y);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
#ifdef CRASH_HANDLER
	InstallCrashHandler();
#endif

	// Don't uninstall crash handler here, it can still catch stuff happening during teardown.
	return WinMainX(hInstance,hPrevInstance,lpCmdLine,nCmdShow);
}
