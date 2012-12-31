// dbug.c
//

#include "BareWnd.h"

// Statics
static char		ofn[_MAX_PATH];
static int		debug_file_handle;

BOOL Open_Debug(char *filename)
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

void Close_Debug(void)
{
	if (debug_file_handle != -1)
	{
		Write_Debug("\r\nDebug Message System Closed.");
		_lclose(debug_file_handle);
	}
}

void __cdecl Write_Debug(char *str, ...)
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
	lstrcat(buf, "\r\n");

	OutputDebugString(buf);

	if (debug_file_handle != -1)
	{
		_lwrite(debug_file_handle, buf, strlen(buf));
#ifdef EVENT_LOG_EXTREME
		_lclose(debug_file_handle);
		{
		int		count=0;

			do
			{
				debug_file_handle = _lopen(ofn, OF_WRITE);
				if (debug_file_handle == -1)
				{
					count += 1;
					if (count > 4)
					{
						break;
					}

					Sleep(100);
				}
			}
			while (debug_file_handle == -1);

			if (debug_file_handle != -1)
			{
				_llseek(debug_file_handle, 0, FILE_END);
			}
		}
#endif
	}
}

