// dbug.c
//

#include "syscfg.h"
#include <stdio.h>

// Statics
static char			ofn[MAX_PATH];
static int			debug_file_handle;

BOOL dbugOpen(char *filename)
{
	if (filename == NULL)
	{
		return(FALSE);
	}

	strcpy(ofn, filename);
#ifdef _DEBUG
	strcat(ofn, "_Debug");
#endif
	strcat(ofn, ".log");

	debug_file_handle = _lcreat(ofn, 0);
	if (debug_file_handle != -1)
	{
		Write_Debug("\r\nDebug Message System Open.\r\n");
	}
	else
	{
		return(FALSE);
	}

	return(TRUE);
}

void dbugClose(void)
{
	if (debug_file_handle != -1)
	{
		Write_Debug("\r\nDebug Message System Closed.");
		_lclose(debug_file_handle);
		debug_file_handle = -1;
	}
}

void __cdecl Write_Debug(char *str, ...)
{
char		buf[2048];
va_list		marker;

#ifdef ACTION_LOG_EXTREME
int		retry;
#endif

	if (str == NULL)
	{
		return;
	}

	va_start(marker, str);
	vsprintf(buf, str, marker);
	va_end(marker);
	strcat(buf, "\r\n");

	OutputDebugString(buf);

	if (debug_file_handle != -1)
	{
		_lwrite(debug_file_handle, buf, (UINT)strlen(buf));

#ifdef ACTION_LOG_EXTREME
		_lclose(debug_file_handle);
		for (retry=0 ; retry < 4 ; retry++)
		{
			debug_file_handle = _lopen(ofn, OF_WRITE);
			if (debug_file_handle != HFILE_ERROR)
			{
				_llseek(debug_file_handle, 0, FILE_END);
				break;
			}

			OutputDebugString("Error during reopen of debug file\r\n");
			Sleep(10);
		}
#endif

	}
}
