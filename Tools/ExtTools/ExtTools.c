// ExtTools.c : Defines the entry point for the DLL application.
//

#define _WIN32_WINNT	0x0400
#include <windows.h>
#include <stdio.h>
#include <dos.h>
#include "ExtTools.h"


static HHOOK		hook;
static FILE			*debug_file_handle;
static BOOL			logfilecreated;
static char			logfilename[256];
static BOOL			OS_IntSup;


#ifdef _DEBUG

static BOOL Open_Debug(char *filename)
{
	if (logfilecreated)
	{
		return(FALSE);
	}

	strcpy(logfilename, filename);

#ifdef _DEBUG
	strcat(logfilename, "_Debug");
#endif

	strcat(logfilename, ".log");

	do
	{
		debug_file_handle = fopen(logfilename, "w+b");
		if (debug_file_handle != NULL)
		{
			break;
		}

		Sleep(100);
	}
	while (TRUE);

	fclose(debug_file_handle);

	logfilecreated = TRUE;
	debug_file_handle = NULL;

	return(TRUE);
}

static void __cdecl Write_Debug(char *str, ...)
{
char    buf[1024];
va_list marker;

	if (str == NULL)
	{
		return;
	}

	va_start(marker, str);
	vsprintf(buf, str, marker);
	va_end(marker);
	strcat(buf, "\r\n");

	do
	{
		debug_file_handle = fopen(logfilename, "a+b");
		if (debug_file_handle != NULL)
		{
			break;
		}

		Sleep(100);
	}
	while (TRUE);

	fwrite(buf, strlen(buf), 1, debug_file_handle);
	fclose(debug_file_handle);

	debug_file_handle = NULL;
}

#else	// _DEBUG

	#define     Open_Debug
	#define     Write_Debug

#endif	// _DEBUG

static BOOL IsInterruptSupported(void)
{
	__try
	{
		__asm
		{
			mov ah,0x02
			int 0x1a
		}

		OS_IntSup = TRUE;
		Write_Debug("Interrupt supported");
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		OS_IntSup = FALSE;
		Write_Debug("Interrupt not supported");
	}

	return(OS_IntSup);
}

#ifdef DONT_USE
static BOOL GetWindowsVersion(DWORD *MajorVersion, DWORD *MinorVersion)
{
OSVERSIONINFO		osvi;

	memset(&osvi, 0, sizeof(OSVERSIONINFO));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

	if (!GetVersionEx((OSVERSIONINFO*)&osvi))
	{
		OS_IntSup = FALSE;	// Use old anti-hack code
		return FALSE;
	}

	switch (osvi.dwPlatformId)
	{
	case VER_PLATFORM_WIN32_NT:

		OS_IntSup = FALSE;
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

		OS_IntSup = TRUE;
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

	if (MajorVersion != NULL)
	{
		*MajorVersion = osvi.dwMajorVersion;
	}

	if (MinorVersion != NULL)
	{
		*MinorVersion = osvi.dwMinorVersion;
	}

	return(TRUE);
}
#endif

BOOL APIENTRY DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			if (!Open_Debug("ExtTools"))
			{
				Write_Debug("ExtTools already attached");
			}

			Write_Debug("ExtTools process attached");

			IsInterruptSupported();
#ifdef DONT_USE
			GetWindowsVersion(NULL, NULL);
#endif
			break;
		case DLL_THREAD_ATTACH:
			Write_Debug("ExtTools thread attached");
			break;
		case DLL_THREAD_DETACH:
			Write_Debug("ExtTools thread detached");
			break;
		case DLL_PROCESS_DETACH:
			Write_Debug("ExtTools process detached");
			break;
    }

    return TRUE;
}


EXTTOOLS_API BOOL SetExtTool(DWORD tool, DWORD param)
{
	switch (tool)
	{
	case SET_HOOK:
		hook = (HHOOK)param;
		return(TRUE);
	default:
		break;
	}

	return(FALSE);
}


EXTTOOLS_API int ExtToolsProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    // By returning a non-zero value from the hook procedure, the
    // message does not get passed to the target window
    KBDLLHOOKSTRUCT *pkbhs = (KBDLLHOOKSTRUCT *) lParam;
    BOOL bControlKeyDown = 0;

    switch (nCode)
    {
        case HC_ACTION:
        {
            // Check to see if the CTRL key is pressed
            bControlKeyDown = GetAsyncKeyState (VK_CONTROL) >> ((sizeof(SHORT) * 8) - 1);
            
            // Disable CTRL+ESC
            if (pkbhs->vkCode == VK_ESCAPE && bControlKeyDown)
			{
                return 1;
			}

            // Disable ALT+TAB
            if (pkbhs->vkCode == VK_TAB && pkbhs->flags & LLKHF_ALTDOWN)
			{
                return 1;
			}

            // Disable ALT+ESC
            if (pkbhs->vkCode == VK_ESCAPE && pkbhs->flags & LLKHF_ALTDOWN)
			{
                return 1;
			}

            break;
        }

        default:
            break;
    }

	return(CallNextHookEx(hook, nCode, wParam, lParam));
}

// CMOS BIOS registers, masks and values
#define BIOS_STATUS_A		0x00
#define BIOS_STATUS_B		0x01
#define BIOS_STATUS_C		0x02

#define BIOS_CLOCK_24HOUR	0x02
#define BIOS_CLOCK_BINARY	0x04

#define BIOS_UPDATE_CYCLE	0x80
#define BIOS_DIVIDER_MASK	0x70
#define BIOS_DIVIDER_VALUE	0x20	// 32.768 Khz time base for clock (default)

#define BIOS_RATE_MASK		0x0f
#define BIOS_RATE_VALUE		0x06	// 976.562 microseconds (default)

EXTTOOLS_API int ExtToolsElapse(void)
{
BYTE	tbytes[2][7];
int		retval;
int		tadr[8]={0, 2, 4, 6, 7, 8, 9, -1};
WORD	ourCX, ourDX;

#ifdef DONT_USE
BYTE	sbyte, sbytes[3][2];
int		sadr[3]={10, 11, -1};
#endif


	if (OS_IntSup == FALSE)
	{
		return(-1);
	}

	__asm
	{
		mov ah,0x02
		int 0x1a
		mov ourCX, cx
		mov ourDX, dx
	}

	tbytes[0][0] = ourDX >> 8;
	tbytes[0][1] = ourCX & 0xff;
	tbytes[0][2] = ourCX >> 8;

	__asm
	{
		mov ah,0x04
		int 0x1a
		mov ourCX, cx
		mov ourDX, dx
	}

	tbytes[0][4] = ourDX & 0xff;
	tbytes[0][5] = ourDX >> 8;
	tbytes[0][6] = ourCX & 0xff;

	Write_Debug("CMOS: time = %02x/%02x/20%02x %02x:%02x:%02x",
		tbytes[0][5], tbytes[0][4], tbytes[0][6], tbytes[0][2], tbytes[0][1], tbytes[0][0]);


#ifdef DONT_USE
	do
	{
	int		i, j;

		_disable();	// Stop interrupts

		// Pre-check status
		for (i=0 ; sadr[i] >= 0 ; i++)
		{
			j = sadr[i];
			__asm
			{
				mov eax, j
				out 0x70, al
				in al, 0x71
				mov j, eax
			}

			sbytes[0][i] = j;
		}

		// Get data; pass one
		for (i=0 ; tadr[i] >= 0 ; i++)
		{
			j = tadr[i];
			__asm
			{
				mov eax, j
				out 0x70, al
				in al, 0x71
				mov j, eax
			}

			tbytes[0][i] = j;
		}

		// Check status
		for (i=0 ; sadr[i] >= 0 ; i++)
		{
			j = sadr[i];
			__asm
			{
				mov eax, j
				out 0x70, al
				in al, 0x71
				mov j, eax
			}

			sbytes[1][i] = j;
		}

		// Get data; pass two
		for (i=0 ; tadr[i] >= 0 ; i++)
		{
			j = tadr[i];
			__asm
			{
				mov eax, j
				out 0x70, al
				in al, 0x71
				mov j, eax
			}

			tbytes[1][i] = j;
		}

		// Post-check status
		for (i=0 ; sadr[i] >= 0 ; i++)
		{
			j = sadr[i];
			__asm
			{
				mov eax, j
				out 0x70, al
				in al, 0x71
				mov j, eax
			}

			sbytes[2][i] = j;
		}

		_enable();	// Resume interrupts

		// Two sets of times must match
		if (memcmp(tbytes[0], tbytes[1], 7) != 0)
		{
			continue;
		}

		// these time data will never have MSB set (precautionary)
		if (tbytes[0][0] & 0x80 ||	// seconds
			tbytes[0][1] & 0x80 ||	// minutes
			tbytes[0][4] & 0x80 ||	// day of month
			tbytes[0][5] & 0x80)	// month
		{
			continue;
		}

		// Three sets of status registers must match
		if (sbytes[0][BIOS_STATUS_A] != sbytes[1][BIOS_STATUS_A] ||
			sbytes[1][BIOS_STATUS_A] != sbytes[2][BIOS_STATUS_A] ||
			sbytes[0][BIOS_STATUS_A] != sbytes[2][BIOS_STATUS_A])
		{
			continue;
		}

		// OR three Status A registers (makes things simple)
		sbyte = sbytes[0][BIOS_STATUS_A] | sbytes[1][BIOS_STATUS_A] | sbytes[2][BIOS_STATUS_A];

		// The MSB of the Status A register tells us whether an update is in progress.
		// If an update is in progress, discard the data and reread it all over.
		// Only continue when the MSB of the Status A register is not set.
		if (!(sbyte & BIOS_UPDATE_CYCLE))
		{
			break;
		}
	}
	while (TRUE);

	Write_Debug("CMOS: time = %02x/%02x/20%02x %02x:%02x:%02x",
		tbytes[0][5], tbytes[0][4], tbytes[0][6], tbytes[0][2], tbytes[0][1], tbytes[0][0]);

	Write_Debug("    status = 0x%02x : 0x%02x",
		sbytes[0][0], sbytes[0][1]);

	// If the divider and rate are not correct default values, tell program to crash!
	if ((sbytes[0][BIOS_STATUS_A] & BIOS_DIVIDER_MASK) != BIOS_DIVIDER_VALUE ||
		((sbytes[0][BIOS_STATUS_A] & BIOS_RATE_MASK) != BIOS_RATE_VALUE))
	{
		Write_Debug("Divider and/or rate values are incorrect");
		return(-2);
	}

	if (sbytes[0][BIOS_STATUS_B] & BIOS_CLOCK_BINARY)
	{
		Write_Debug("BIOS data is in binary");
	}
	else
	{
		Write_Debug("BIOS data is in BCD");

		tbytes[0][0] = ((tbytes[0][0] & 0xf0) >> 4) * 10 + (tbytes[0][0] & 0x0f);	// Seconds
		tbytes[0][1] = ((tbytes[0][1] & 0xf0) >> 4) * 10 + (tbytes[0][1] & 0x0f);	// Minutes

		if (sbytes[0][BIOS_STATUS_B] & BIOS_CLOCK_24HOUR)
		{
			tbytes[0][2] = ((tbytes[0][2] & 0xf0) >> 4) * 10 + (tbytes[0][2] & 0x0f);	// Hours
		}
		else
		{
			tbytes[0][2] = ((tbytes[0][2] & 0x70) >> 4) * 10 + (tbytes[0][2] & 0x0f);	// Hours
			// Is it noon or later?
			if (tbytes[1][2] & 0x80)	// NOTE: Using data from second array since we need the MSB!
			{
				if (tbytes[0][2] > 12)
				{
					tbytes[0][2] = (tbytes[0][2] & 0x7f) + 12;
				}
			}
			else
			{
				if (tbytes[0][2] == 12)
				{
					tbytes[0][2] = 0;
				}
			}
		}

		tbytes[0][4] = ((tbytes[0][4] & 0xf0) >> 4) * 10 + (tbytes[0][4] & 0x0f);	// Day of Month
		tbytes[0][5] = ((tbytes[0][5] & 0xf0) >> 4) * 10 + (tbytes[0][5] & 0x0f);	// Month
		tbytes[0][6] = ((tbytes[0][6] & 0xf0) >> 4) * 10 + (tbytes[0][6] & 0x0f);	// Year
	}
#endif

	Write_Debug("BIOS data is in BCD");

	tbytes[0][0] = ((tbytes[0][0] & 0xf0) >> 4) * 10 + (tbytes[0][0] & 0x0f);	// Seconds
	tbytes[0][1] = ((tbytes[0][1] & 0xf0) >> 4) * 10 + (tbytes[0][1] & 0x0f);	// Minutes
	tbytes[0][2] = ((tbytes[0][2] & 0x70) >> 4) * 10 + (tbytes[0][2] & 0x0f);	// Hours

	tbytes[0][4] = ((tbytes[0][4] & 0xf0) >> 4) * 10 + (tbytes[0][4] & 0x0f);	// Day of Month
	tbytes[0][5] = ((tbytes[0][5] & 0xf0) >> 4) * 10 + (tbytes[0][5] & 0x0f);	// Month
	tbytes[0][6] = ((tbytes[0][6] & 0xf0) >> 4) * 10 + (tbytes[0][6] & 0x0f);	// Year

	retval = tbytes[0][0];								// Seconds
	retval = retval + 60 * tbytes[0][1];				// Minutes to Seconds
	retval = retval + 3600 * tbytes[0][2];				// Hours to Seconds
	retval = retval + 86400 * (tbytes[0][4] - 1);		// Day of Month to Seconds
	retval = retval + 2592000 * (tbytes[0][5] - 1);		// 30 day Months to Seconds (close enough for our work)
	retval = retval + 31104000 * tbytes[0][6];			// 12 month Years to Seconds

	Write_Debug("BIOS count: %d", retval);

	return(retval);
}
