// MoreGetRoutines.c
//
#include <windows.h>
#include <mmsystem.h>
//#include "barewnd.h"
#include "dbug/dbug.h"
#include "MoreGetRoutines.h"
#include "include/infoMessageTypes.h"
#include "oglcfg.h"
#include "ogl.h"
#include <Iphlpapi.h>
#include <intrin.h>

// Defines
#define DIRECT_DRAW_DLL		_T("ddraw.dll")
#define DIRECT_3D_DLL		_T("d3dim.dll")

#define DIRECTX_3			0x00040003
#define DIRECTX_4			0x00040004
#define DIRECTX_5			0x00040005
#define DIRECTX_6			0x00040006

#define DIRECTX_REQUIRED	DIRECTX_6

#define DIV 1024

#define divisor				"K"

static DWORD				DXVersionMajor;
static DWORD				DXVersionMinor;

//#pragma managed(push, off)
#pragma optimize("", off)
static unsigned long long GetCPUSpeed(void)
{
	float			speed;
	LARGE_INTEGER	count1, count2, freq;
	__int64			count, rdcount, rdcount1, rdcount2;
	HANDLE			hprocess, hthread;

	hprocess = GetCurrentProcess();
	if (! SetPriorityClass(hprocess, REALTIME_PRIORITY_CLASS))
		SetPriorityClass(hprocess, HIGH_PRIORITY_CLASS);

	hthread = GetCurrentThread();
	if (! SetThreadPriority(hthread, THREAD_PRIORITY_TIME_CRITICAL))
		if (! SetThreadPriority(hthread, THREAD_PRIORITY_HIGHEST))
			SetThreadPriority(hthread, THREAD_PRIORITY_ABOVE_NORMAL);

	speed = 0.0f;
	if (QueryPerformanceCounter(&count1))
	{
		rdcount1 = __rdtsc();
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

		rdcount2 = __rdtsc();
		QueryPerformanceCounter(&count2);
		rdcount = rdcount2 - rdcount1;
		count = count2.QuadPart - count1.QuadPart;
		QueryPerformanceFrequency(&freq);
		speed = (float)((double)freq.LowPart / (double)count * (double)rdcount / 1000000.0);
//		Write_Debug("CPU speed calculated to be %f MHz", speed);
	}

	SetThreadPriority(hthread, THREAD_PRIORITY_NORMAL);
	SetPriorityClass(hprocess, NORMAL_PRIORITY_CLASS);

	return (unsigned long long)speed;
}
#pragma optimize("", on)

void GetMemory(PLAY_MEM* const meminfo)
{
	MEMORYSTATUS	memstat;

	GlobalMemoryStatus(&memstat);

	meminfo->dwLength		= memstat.dwLength;
	meminfo->dwMemoryLoad	= memstat.dwMemoryLoad;
	meminfo->dwTotalPhys		= memstat.dwTotalPhys;
	meminfo->dwAvailPhys		= memstat.dwAvailPhys;
	meminfo->dwTotalPageFile	= memstat.dwTotalPageFile;
	meminfo->dwAvailPageFile	= memstat.dwAvailPageFile;
	meminfo->dwTotalVirtual	= memstat.dwTotalVirtual;
	meminfo->dwAvailVirtual	= memstat.dwAvailVirtual;

	return;
}

void
GetCPUFeatures(CPUFeatures& features)
{
    int info[4] = {-1};

	memset(&features, 0, sizeof(features)) ;

	// __cpuid with an InfoType argument of 0 returns the number of
	// valid Ids in info[0] and the CPU identification string in
	// the other three array elements. The CPU identification string is
	// not in linear order. The code below arranges the information 
	// in a human readable form.
	__cpuid(info, 0);

	unsigned int nIds = info[0];

	*((int*)features.manufacturer) = info[1];
	*((int*)(features.manufacturer+4)) = info[3];
	*((int*)(features.manufacturer+8)) = info[2];

	// Get the information associated with each valid Id
	if ( nIds < 1 )
	{
		Write_Debug("Invalid CPU Features nIDs %u", nIds) ;
		return ;
	}

	// Interpret CPU feature information.
	__cpuid(info, 1);
	features.stepping = info[0] & 0xf;
	features.model = (info[0] >> 4) & 0xf;
	features.family = (info[0] >> 8) & 0xf;
	features.processorType = (info[0] >> 12) & 0x3;
	features.extendedModel = (info[0] >> 16) & 0xf;
	features.extendedFamily = (info[0] >> 20) & 0xff;
	features.brandIndex = info[1] & 0xff;
	features.clflushCLS = ((info[1] >> 8) & 0xff) ;
	features.apicPhysicalID = (info[1] >> 24) & 0xff;
	features.extFeatureInfo = info[2] ;
	features.featureInfo = info[3];

	// Calling __cpuid with 0x80000000 as the InfoType argument
	// gets the number of valid extended IDs.
	__cpuid(info, 0x80000000);
	unsigned int nExIds = info[0];

	// Get the information associated with each extended ID.
	for ( unsigned int i = 0x80000002 ; i <= nExIds ; ++i )
	{
		// Interpret CPU brand string and cache information.
		switch ( i )
		{
		case 0x80000002:
			__cpuid(info, i);
			memcpy(features.brandName, info, sizeof(info));
			break ;
		case 0x80000003:
			__cpuid(info, i);
			memcpy(features.brandName + 16, info, sizeof(info));
			break ;
		case 0x80000004:
			__cpuid(info, i);
			memcpy(features.brandName + 32, info, sizeof(info));
			break ;
		case 0x80000006:
			__cpuid(info, i);
			features.cacheLineSize = info[2] & 0xff;
			features.l2Associativity = (info[2] >> 12) & 0xf;
			features.cacheSizeK = (info[2] >> 16) & 0xffff;
			break ;
		default:
			break ;
		}
	}

	features.speed = GetCPUSpeed() ;
}

// Static functions
BOOL GetDirectXFileVersion(TCHAR* DirectXFile, PLAY_DX_VERS *vers, BOOL Messages)
{
	TCHAR				txt[256];
	BOOL				retval=FALSE;
	DWORD				Nothing, VSize;
	UINT				Len;
	LPVOID				pVersion=NULL;
	VS_FIXEDFILEINFO*	pFFI;

	vers->isInstalled = 0;
	vers->hiVersion = 0;
	vers->lowVersion = 0;

	do
	{
		VSize = GetFileVersionInfoSize(DirectXFile, &Nothing);
		if (VSize == 0)
		{
			// DirectX not installed
			if (Messages)
			{
				wsprintf(txt, _T("DirectX is not installed on your system.\n")
							_T("%s requires you to install DirectX "),
							_T("Get DirectX File Version"));
				MessageBox(NULL, txt, _T("Verify DirectX File Version"), MB_OK|MB_ICONSTOP);
			}

			break;
		}

		pVersion = malloc(VSize);
		if (pVersion == NULL)
		{
			// Severe error
			// If we can't alloc memory, then message
			// boxes and such will probably not work
			break;
		}

		if (!GetFileVersionInfo(DirectXFile, 0, VSize, pVersion) ||
				!VerQueryValue(pVersion, L"\\", (LPVOID*)&pFFI, &Len))
		{
			// Problem with the DirectX file
			if (Messages)
			{
				wsprintf(txt, _T("DirectX is not installed properly on your system.\n")
											_T("%s requires you to install DirectX"),
											_T("Get DirectX File Version"));
				MessageBox(NULL, txt, _T("Verify DirectX File Version"), MB_OK|MB_ICONSTOP);
			}

			break;
		}

#ifdef _DEBUG_MORE
		if (Messages)
		{
		char	szVersion[32];

			_tsprintf(szVersion, "DirectX %d.%d", HIWORD(pFFI->dwProductVersionMS), LOWORD(pFFI->dwProductVersionMS));
			_tsprintf(txt, "%s\nVersion:\t%d.%d.%d.%d", DirectXFile,
						HIWORD(pFFI->dwProductVersionMS),
						LOWORD(pFFI->dwProductVersionMS),
						HIWORD(pFFI->dwProductVersionLS),
						LOWORD(pFFI->dwProductVersionLS));
			MessageBox(NULL, txt, "Verify DirectX File Version", MB_OK|MB_ICONINFORMATION);
		}
#endif

		retval = TRUE;

		vers->isInstalled = TRUE;
		vers->hiVersion = pFFI->dwProductVersionMS;
		vers->lowVersion = pFFI->dwProductVersionLS;
	}
	while (0);

	if (pVersion != NULL)
	{

		free(pVersion);
	}

	return(retval);
}


BOOL VerifyDirectXFileVersion(PLAY_DX_VERS *vers, DWORD MinProductVersionMS, DWORD MinProductVersionLS, BOOL Messages)
{
	BOOL retval = FALSE;

	if (vers->hiVersion > MinProductVersionMS ||
			(vers->hiVersion == MinProductVersionMS &&
			vers->lowVersion >= MinProductVersionLS))
	{
		retval = TRUE;
	}
	else
	{
		// DirectX not correct version
		if (Messages)
		{
			TCHAR txt[512] ;
			wsprintf(txt, _T("The version of DirectX installed on your system\n")
										_T("is DirectX %d. %s requires you to\n")
										_T("upgrade to DirectX %d."),
										LOWORD(vers->hiVersion),
										_T("Verify DirectX File Version"),
										LOWORD(MinProductVersionMS));
			MessageBox(NULL, txt, _T("Verify DirectX File Version"), MB_OK|MB_ICONSTOP);
		}
	}

	return retval;
}

BOOL VerifyDxVersion(PLAY_DX *info, BOOL Messages)
{
	if (! VerifyDirectXFileVersion(&info->draw, DIRECTX_REQUIRED, 0, Messages) )
		return FALSE;

	if (! VerifyDirectXFileVersion(&info->d3d, DIRECTX_REQUIRED, 0, Messages) )
		return FALSE;

	return TRUE;
}


BOOL GetDxInfo(PLAY_DX *info, BOOL Messages)
{
	if (!GetDirectXFileVersion(DIRECT_DRAW_DLL, &info->draw, Messages))
	{
		return(FALSE);
	}

	if (!GetDirectXFileVersion(DIRECT_3D_DLL, &info->d3d, Messages))
	{
		return(FALSE);
	}

	return(TRUE);
}

void WCHARncpy(WCHAR* dest, char* source, size_t max_size)
{
	UINT32 i; 
	dest[max_size] = 0;
	for (i=0; i < max_size;i++)
	{
		dest[i] = source[i];
		if (source[i] == 0)
			break;
	}
}

void GetSoundDevice(PLAY_SOUND *sound)
{
	UINT_PTR	uMxId;
	MIXERCAPS	mxcaps;
	MMRESULT	result;

	sound->isValid = FALSE;
	
	result = mixerGetID(NULL, &uMxId, MIXER_OBJECTF_WAVEOUT);
	if (result != MMSYSERR_NOERROR)
		return;

	result = mixerGetDevCaps(uMxId, &mxcaps, sizeof(mxcaps));
	if (result != MMSYSERR_NOERROR)
		return;

	sound->isValid = TRUE;

	sound->mixer.cDestinations	= mxcaps.cDestinations;
	sound->mixer.fdwSupport		= mxcaps.fdwSupport;
	sound->mixer.vDriverVersion	= mxcaps.vDriverVersion;
	sound->mixer.wMid			= mxcaps.wMid;
	sound->mixer.wPid			= mxcaps.wPid;

	for ( UINT32 i = 0 ; i < MAX_MIX_NAME && i < sizeof(sound->mixer.szPname) ; ++i )
		sound->mixer.szPname[i] = (char)mxcaps.szPname[i] ;
	sound->mixer.szPname[MAX_MIX_NAME] = 0;

//	Write_Debug("Audio device: %s", mxcaps.szPname);
//	Write_Debug("Audio driver version: %d.%d", HIBYTE(mxcaps.vDriverVersion), LOBYTE(mxcaps.vDriverVersion));

	return;
}

void GetComputerInfo(HINSTANCE hInstance, PLAY_COMPUTER_INFO* const info, BOOL Messages)
{
	info->version = info->CurrentVersion();
	GetMemory(&info->memory);
	GetCPUFeatures(info->cpu) ;
	GetSoundDevice(&info->sound);

	GetDxInfo(&info->video.dx, Messages);
	GetOglInfo(hInstance,&info->video.ogl, Messages);
}
//#pragma managed(pop)
