
// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the EXTTOOLS_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// EXTTOOLS_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#ifdef EXTTOOLS_EXPORTS
#define EXTTOOLS_API __declspec(dllexport)
#else
#define EXTTOOLS_API __declspec(dllimport)
#endif


typedef EXTTOOLS_API BOOL (*SETEXTTOOL)(DWORD function, DWORD hook);
typedef EXTTOOLS_API int  (*EXTTOOLSPROC)(int nCode, WPARAM wParam, LPARAM lParam);
typedef EXTTOOLS_API int  (*EXTTOOLSELAPSE)(void);


EXTTOOLS_API BOOL SetExtTool(DWORD function, DWORD hook);
EXTTOOLS_API int ExtToolsProc(int nCode, WPARAM wParam, LPARAM lParam);
EXTTOOLS_API int ExtToolsElapse(void);


enum
{
	SET_HOOK,
	MAX_SET_TOOLS
};
