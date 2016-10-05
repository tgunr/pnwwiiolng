#include <syscfg.h> // Path is project specific.
#include <windows.h>
#include <shellapi.h>
#include <direct.h> // _getcwd
#include <string>
#include "sysLaunchUrl.h"
#include "include/vars.h"

extern HWND	myhwnd;

#define PATH_SIZE 255

LOGICAL syslaunchurl(const char* url, LOGICAL local)
{
	std::string destination;

	// if we're local we'll lookup the working directory and
	// make the url relative to that
	if(local)
	{
		char path[PATH_SIZE + 1] = { 0 };
		if(_getcwd(path, PATH_SIZE))
		{
			destination = "file://";

			destination += path;
			destination += "\\";
			destination += url;
		}
	}
	else
	{
		destination = url;
	}

	if(ShellExecuteA(nullptr, "open", destination.c_str(), nullptr, nullptr, SW_SHOWNORMAL))
	{
		ShowWindow(myhwnd, SW_MINIMIZE);
		return 1;
	}

	return 0;
}
