/*--------------------------------------------------------------------------------
file.cpp
---------------------------------------------------------------------------------*/
#include "file.h"
#include "registry.h"
#include "language.h"

#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>
#include <string.h>
#include <direct.h>
#include <stdlib.h>
#include <stdio.h>
#include <shlobj.h>
#include <tchar.h>

#include <string>

#define WW2OL_APP 1
#define WW2OL_BETA_APP 12

TCHAR UserDataPath[256];

std::string fileUrlDecode(const char* src)
{
	char decoded[_MAX_PATH+1];
	char code[3] = {0};

	const char* p = src;
	char* d = decoded;
	unsigned long ascii = 0;
	char *end = NULL;
	
	memset(decoded, 0, sizeof(decoded));

	if(strstr(p, "file:///") == p)
		p += 8;
	
	while(*p)
	{
		if(*p == '%')
		{
			memcpy(code, ++p, 2);
			ascii = strtoul(code, &end, 16);
			*d++ = (char)ascii;
			p += 2;
		}
		else
			*d++ = *p++;
	}
	
	return std::string(decoded);
}


/*-------------------------------------------------------------
fileInit
--------------------------------------------------------------*/
void fileInit()
{
#if GREEN_INSTALL == 1
	lstrcat(UserDataPath, _T("."));
#else
    SHGetFolderPath(NULL, CSIDL_MYDOCUMENTS | CSIDL_FLAG_CREATE, NULL, 0, UserDataPath);

	const TCHAR* base = registryInstallValue(_T("Version"));
	
	if(base)
	{
		lstrcat(UserDataPath, _T("\\"));
		lstrcat(UserDataPath, base);
	}
#endif // GREEN_INSTALL
}

/*-------------------------------------------------------------
fileRead
--------------------------------------------------------------*/
int fileRead(int handle, char *buffer, int length)
{
    if(handle < 0)
    {
        return -1;
    }

    // HACK! Remove garbage at beginning of file if necessary
    static bool firstread = true;

    buffer[0] = 0;

    int bytes = _read(handle, buffer, length);

    int i = 0;

    if(firstread)
    {
        firstread = false;

        for(i = 0; i < bytes; i++)
        {
            if(buffer[i] == '<')
            {
                break;
            }
        }

        if(i > 0)
        {
            char *p = buffer + i;
            strncpy(buffer, p, bytes);
        }
    }

    // This function is called in 4000 byte chunks. When the file is finished,
    // bytes is 0. We only want to remove garbage at the beginning of the file read.
    if(bytes == 0)
    {
        firstread = true;
    }

    return bytes - i;
}

/*-------------------------------------------------------------
fileClose
--------------------------------------------------------------*/
int fileClose(int handle)
{
    if(handle > 0)
    {
        _close(handle);
    }

    return 0;
}

/*-------------------------------------------------------------
fileOpen
--------------------------------------------------------------*/
int fileOpen(const char* path)
{
	std::string utf8	= fileUrlDecode(path);
	std::wstring utf16	= (TCHAR*)utf8_to_utf16(utf8.c_str());

	int handle = _wopen(utf16.c_str(), O_RDONLY | O_BINARY);

    return handle;
}

/*-------------------------------------------------------------
fileOpenWrite
--------------------------------------------------------------*/
int fileOpenWrite(const wchar_t *filename)
{
    // Make sure directorys are created since some filenames contain directory paths.
	std::wstring wpath(filename);
	
    for(unsigned int i = 0; i < wpath.length(); i++)
    {
        if(wpath[i] == L'/') wpath[i] = L'\\';
    }
    
    size_t pos = wpath.find(L'\\');
    std::wstring path;

    size_t start = 0;

    // Need to create directories
    while(pos != std::wstring::npos)
    {   
        path += wpath.substr(start, pos - start);
        int error = _wmkdir(path.c_str());
      
        start = pos;
        pos = wpath.find(L'\\', pos + 1);
    }

    return _wopen(wpath.c_str(), O_CREAT|O_TRUNC|O_WRONLY|O_BINARY, S_IREAD|S_IWRITE);
}

/*-------------------------------------------------------------
fileWrite
--------------------------------------------------------------*/
int fileWrite(int handle, const void *data, size_t size)
{
    return _write(handle, data, (unsigned int)size);
}

/*-------------------------------------------------------------
fileWrite
--------------------------------------------------------------*/
int fileSeek(int handle, int offset, int from)
{
	return _lseek(handle, offset, from);
}

/*-------------------------------------------------------------
fileAccess
--------------------------------------------------------------*/
bool fileAccess(const char* file)
{
	std::string utf8	= fileUrlDecode(file);
	std::wstring utf16	= (TCHAR*)utf8_to_utf16(utf8.c_str());

	return(_waccess(utf16.c_str(), 0) == 0);
}
