#include <syscfg.h> // Path is project specific.
#ifdef OPENGL3D

#include <tchar.h>
#include <locale.h>
#include <mmsystem.h>
#include <io.h>
#include "ogl/ogl.h"
#include "dip.h"
#include "include/vars.h"
#include "mem/mem.h"
#include "rn/rnmain.h"
#include "sys/sysreg.h"
#include "str/strpool.h"
#include "sysmain.h"
#include "dimain.h"
#include "gmlp/gmlpmain.h"
#include "cfg/cfgPrefs.h"
#include "ui/uiMain.h"
#include "pi/piinfo.h"
#include "ime/imeMain.h"
#include "fio/fio.h"
#include "ogl/oglGlobalStates.h"
#include "resource.h"

#include "ddp.h"

#if !defined(USE_OSG)
#include "gl/wglew.h"
#endif

//#define GLOBALS
//#undef GLOBALS

#if defined(USE_OSG)
#include "osg/osgGame.h"
#endif

extern void wwiiSendUserAbortedApp();

#define DDP_EVENT_LOG
//#undef DDP_EVENT_LOG

#define DDP_EVENT_LOG_EXTREME
//#undef DDP_EVENT_LOG_EXTREME

#define CENTER_WINDOW

extern LOGICAL					running;
static int cursorvisible;
/*
	Static variables
*/
static ATOM						wndAtom;

static LOGICAL					dna;
static LOGICAL					dss;
static LOGICAL					state;
static LOGICAL					resetfps;
static LOGICAL					focus;

static SINT32					capture;

static TCHAR					classname[256];
static UINT32					flags;
static UINT32					show;
static UINT32					delay;
static UINT32					starttime;
static long						framecount;

static HRESULT					retval;

static HDC						myhdc;
static HGLRC					myhrc;

static DDWND_DDINITFUNC			ddinitfunc;
static DDWND_D3DINITFUNC		d3dinitfunc;
static DDWND_DDRENDERFUNC		ddrenderfunc;
static DDWND_D3DRENDERFUNC		d3drenderfunc;
static DDWND_DDKILLFUNC			ddkillfunc;
static DDWND_D3DKILLFUNC		d3dkillfunc;
static DDWND_MOUSEFUNC			mousefunc;
static DDWND_TIMERFUNC			timerfunc;
static DDWND_KEYBOARDFUNC		keyboardfunc;

static UINT32					modenum, displaynum;
static DEVMODE					devmode;
static DISPLAY_DEVICE			displaydevice;

static RECT gamerect;

/*
	Global variables
*/
HANDLE				hinst;
HWND				myhwnd;

/*
	Static functions
*/
static LRESULT CALLBACK			WindowProc(HWND hwnd, UINT umsg, WPARAM wParam, LPARAM lParam);
static void UpdateTimes(void);

static LOGICAL OnEraseBkgnd(HDC hDC);
static LOGICAL OnNcActivate(LOGICAL bActive);
#if(0)
static LOGICAL MoveSize();
#endif
static LOGICAL Create(HWND hwnd);

static void	Destroy();
static void	OnClose();
static void	_OnPaint();
static void	OnSetFocus(HWND pOldWnd);
static void	OnKillFocus(HWND pNewWnd);
static void	OnSize(UINT nType, SINT32 cx, SINT32 cy);
static void	OnSysKeyDown(UINT nChar, UINT nFlags);
static void	OnSysKeyUp(UINT nChar, UINT nFlags);

#define USE_NEWGL 0

static int ExceptionHandler(int code, EXCEPTION_POINTERS* ep)
{
char*		pStr;
DWORD		i;
SYSTEMTIME	SystemTime;

	FILE* ExceptionLog = Fio::OpenLogFile();
	if ( ExceptionLog )
	{
		Write_Debug("Exception handler information...");

	#ifdef EVENT_LOG
		fprintf(ExceptionLog, "%s Logging Executable\n", SYS_TITLE);
	#endif

		fprintf(ExceptionLog, "Program compile time: %s %s\n", __DATE__, __TIME__);
		Write_Debug("Program compile time: %s %s", __DATE__, __TIME__);

		GetLocalTime(&SystemTime);
		fprintf(ExceptionLog,"Date and time: %d/%d/%d %d:%02d:%02d\n",
					SystemTime.wMonth,
					SystemTime.wDay,
					SystemTime.wYear,
					SystemTime.wHour,
					SystemTime.wMinute,
					SystemTime.wSecond);

		Write_Debug("Date and time: %d/%d/%d %d:%02d:%02d",
					SystemTime.wMonth,
					SystemTime.wDay,
					SystemTime.wYear,
					SystemTime.wHour,
					SystemTime.wMinute,
					SystemTime.wSecond);

		fprintf(ExceptionLog, "Exception code: 0x%08x\n", code);
		Write_Debug("Exception code: 0x%08x", code);

		switch (code)
		{
		case STATUS_WAIT_0:
			pStr = "Exception: STATUS_WAIT_0";
			break;
		case STATUS_ABANDONED_WAIT_0:
			pStr = "Exception: STATUS_ABANDONED_WAIT_0";
			break;
		case STATUS_USER_APC:
			pStr = "Exception: STATUS_USER_APC";
			break;
		case STATUS_TIMEOUT:
			pStr = "Exception: STATUS_TIMEOUT";
			break;
		case STATUS_PENDING:
			pStr = "Exception: STATUS_PENDING";
			break;
		case STATUS_SEGMENT_NOTIFICATION:
			pStr = "Exception: STATUS_SEGMENT_NOTIFICATION";
			break;
		case STATUS_GUARD_PAGE_VIOLATION:
			pStr = "Exception: STATUS_GUARD_PAGE_VIOLATION";
			break;
		case STATUS_DATATYPE_MISALIGNMENT:
			pStr = "Exception: STATUS_DATATYPE_MISALIGNMENT";
			break;
		case STATUS_BREAKPOINT:
			pStr = "Exception: STATUS_BREAKPOINT";
			break;
		case STATUS_SINGLE_STEP:
			pStr = "Exception: STATUS_SINGLE_STEP";
			break;
		case STATUS_ACCESS_VIOLATION:
			pStr = "Exception: STATUS_ACCESS_VIOLATION";
			break;
		case STATUS_IN_PAGE_ERROR:
			pStr = "Exception: STATUS_IN_PAGE_ERROR";
			break;
		case STATUS_INVALID_HANDLE:
			pStr = "Exception: STATUS_INVALID_HANDLE";
			break;
		case STATUS_NO_MEMORY:
			pStr = "Exception: STATUS_NO_MEMORY";
			break;
		case STATUS_ILLEGAL_INSTRUCTION:
			pStr = "Exception: STATUS_ILLEGAL_INSTRUCTION";
			break;
		case STATUS_NONCONTINUABLE_EXCEPTION:
			pStr = "Exception: STATUS_NONCONTINUABLE_EXCEPTION";
			break;
		case STATUS_INVALID_DISPOSITION:
			pStr = "Exception: STATUS_INVALID_DISPOSITION";
			break;
		case STATUS_ARRAY_BOUNDS_EXCEEDED:
			pStr = "Exception: STATUS_ARRAY_BOUNDS_EXCEEDED";
			break;
		case STATUS_FLOAT_DENORMAL_OPERAND:
			pStr = "Exception: STATUS_FLOAT_DENORMAL_OPERAND";
			break;
		case STATUS_FLOAT_DIVIDE_BY_ZERO:
			pStr = "Exception: STATUS_FLOAT_DIVIDE_BY_ZERO";
			break;
		case STATUS_FLOAT_INEXACT_RESULT:
			pStr = "Exception: STATUS_FLOAT_INEXACT_RESULT";
			break;
		case STATUS_FLOAT_INVALID_OPERATION:
			pStr = "Exception: STATUS_FLOAT_INVALID_OPERATION";
			break;
		case STATUS_FLOAT_OVERFLOW:
			pStr = "Exception: STATUS_FLOAT_OVERFLOW";
			break;
		case STATUS_FLOAT_STACK_CHECK:
			pStr = "Exception: STATUS_FLOAT_STACK_CHECK";
			break;
		case STATUS_FLOAT_UNDERFLOW:
			pStr = "Exception: STATUS_FLOAT_UNDERFLOW";
			break;
		case STATUS_INTEGER_DIVIDE_BY_ZERO:
			pStr = "Exception: STATUS_INTEGER_DIVIDE_BY_ZERO";
			break;
		case STATUS_INTEGER_OVERFLOW:
			pStr = "Exception: STATUS_INTEGER_OVERFLOW";
			break;
		case STATUS_PRIVILEGED_INSTRUCTION:
			pStr = "Exception: STATUS_PRIVILEGED_INSTRUCTION";
			break;
		case STATUS_STACK_OVERFLOW:
			pStr = "Exception: STATUS_STACK_OVERFLOW";
			break;
		case STATUS_CONTROL_C_EXIT:
			pStr = "Exception: STATUS_CONTROL_C_EXIT";
			break;
		case STATUS_FLOAT_MULTIPLE_FAULTS:
			pStr = "Exception: STATUS_FLOAT_MULTIPLE_FAULTS";
			break;
		case STATUS_FLOAT_MULTIPLE_TRAPS:
			pStr = "Exception: STATUS_FLOAT_MULTIPLE_TRAPS";
			break;
		default:
			pStr = "Unknown exception";
			break;
		}

		fprintf(ExceptionLog, "%s\n", pStr);
		Write_Debug("%s", pStr);

		if (ep->ExceptionRecord->ExceptionFlags == EXCEPTION_NONCONTINUABLE)
		{
			fprintf(ExceptionLog, "Exception: noncontinuable\n");
			Write_Debug("Exception: noncontinuable");
		}
		else
		{
			fprintf(ExceptionLog, "Exception: continuable\n");
			Write_Debug("Exception: continuable");
		}

		fprintf(ExceptionLog, "Exception address: 0x%08x\n", ep->ExceptionRecord->ExceptionAddress);
		Write_Debug("Exception address: 0x%08x", ep->ExceptionRecord->ExceptionAddress);

		for (i=0; i < ep->ExceptionRecord->NumberParameters; i+=2)
		{
			fprintf(ExceptionLog, "Exception tried to: %s at address 0x%08x\n", ep->ExceptionRecord->ExceptionInformation[i] ? "write" : "read", ep->ExceptionRecord->ExceptionInformation[i+1]);
			Write_Debug("Exception tried to: %s at address 0x%08x", ep->ExceptionRecord->ExceptionInformation[i] ? "write" : "read", ep->ExceptionRecord->ExceptionInformation[i+1]);
		}

		// Has registers
		fprintf(ExceptionLog, "Exception Registers:\n");
		Write_Debug("Exception Registers:");
		if (ep->ContextRecord->ContextFlags & CONTEXT_SEGMENTS)
		{
			fprintf(ExceptionLog, "  DS: 0x%08x,  ES: 0x%08x,  FS: 0x%08x,  GS: 0x%08x\n",
				ep->ContextRecord->SegDs,
				ep->ContextRecord->SegEs,
				ep->ContextRecord->SegFs,
				ep->ContextRecord->SegGs);
			Write_Debug("  DS: 0x%08x,  ES: 0x%08x,  FS: 0x%08x,  GS: 0x%08x",
				ep->ContextRecord->SegDs,
				ep->ContextRecord->SegEs,
				ep->ContextRecord->SegFs,
				ep->ContextRecord->SegGs);
		}

		if (ep->ContextRecord->ContextFlags & CONTEXT_INTEGER)
		{
			fprintf(ExceptionLog, " EBP: 0x%08x, ESP: 0x%08x,  SS: 0x%08x, EIP: 0x%08x\n",
				ep->ContextRecord->Ebp,
				ep->ContextRecord->Esp,
				ep->ContextRecord->SegSs,
				ep->ContextRecord->Eip);
			Write_Debug(" EBP: 0x%08x, ESP: 0x%08x,  SS: 0x%08x, EIP: 0x%08x",
				ep->ContextRecord->Ebp,
				ep->ContextRecord->Esp,
				ep->ContextRecord->SegSs,
				ep->ContextRecord->Eip);
		}

		if (ep->ContextRecord->ContextFlags & CONTEXT_INTEGER)
		{
			fprintf(ExceptionLog, " ESI: 0x%08x, EDI: 0x%08x\n EAX: 0x%08x, EBX: 0x%08x, ECX: 0x%08x, EDX: 0x%08x\n",
				ep->ContextRecord->Esi,
				ep->ContextRecord->Edi,
				ep->ContextRecord->Eax,
				ep->ContextRecord->Ebx,
				ep->ContextRecord->Ecx,
				ep->ContextRecord->Edx);
			Write_Debug(" ESI: 0x%08x, EDI: 0x%08x\r\n EAX: 0x%08x, EBX: 0x%08x, ECX: 0x%08x, EDX: 0x%08x",
				ep->ContextRecord->Esi,
				ep->ContextRecord->Edi,
				ep->ContextRecord->Eax,
				ep->ContextRecord->Ebx,
				ep->ContextRecord->Ecx,
				ep->ContextRecord->Edx);
		}

		// Has subsequent exception records (if present)
		ep->ExceptionRecord->ExceptionRecord;

		fflush(ExceptionLog);
		fclose(ExceptionLog);
	}

	return(EXCEPTION_EXECUTE_HANDLER);
}

HINSTANCE ddpGetInstance(void)
{
	return (HINSTANCE)hinst;
}

void ddpSetInstance(HINSTANCE hnewInst)
{
	hinst = hnewInst;
}

LOGICAL ddpInit(void)
{
	state = TRUE;
	delay = 0;
	show = DDWND_SHOW_NONE;
	dna = FALSE;
	dss = FALSE;
	resetfps = FALSE;
	focus = FALSE;
	capture = 0;
	ddinitfunc = NULL;
	d3dinitfunc = NULL;
	ddrenderfunc = NULL;
	d3drenderfunc = NULL;
	ddkillfunc = NULL;
	d3dkillfunc = NULL;
	mousefunc = NULL;
	timerfunc = NULL;
	keyboardfunc = NULL;

	imeInit();

	Write_Debug("Completed ddpInit");

	return(TRUE);
}

#if(0)
static void DisplayLastError(void)
{
LPVOID lpMsgBuf;

	FormatMessage(	FORMAT_MESSAGE_ALLOCATE_BUFFER |
					FORMAT_MESSAGE_FROM_SYSTEM |
					FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL,
					GetLastError(),
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
					(LPTSTR) &lpMsgBuf,
					0,
					NULL);

	// Display the string
	Write_Debug((const char*)lpMsgBuf);

	// Free the buffer
	LocalFree(lpMsgBuf);
}
#endif

// Stub function that will report an error during the setup of the video card
// and notify the user
static inline void _initializationError(const char* problem)
{
	Write_Log(LOG_RENDER, LOG_WARN, "Error during DDP Initialization: %s", problem);

	static const TCHAR* _ERR_TITLE = _T("Video initialization error");
	static const TCHAR* _ERR_STR_1 = _T("The game was unable to initialize the video display: ");
	static const TCHAR* _ERR_STR_2 = _T("\nPlease check your video card configuration and try again.");

	std::wstring message = _ERR_STR_1;
	message += UTF16(problem);
	message += _ERR_STR_2;
	MessageBox(NULL, message.c_str(), _T("Video initialization error"), MB_ICONEXCLAMATION|MB_OK);
}

bool CreateGLContext(HDC dc)
{
	HGLRC Context = wglCreateContext(dc);
	wglMakeCurrent(dc, Context);

	GLenum err = glewInit();

	if(err != GLEW_OK)
	{
		static char str[255];
		sprintf(str, "Error: %s\n", glewGetErrorString(err));
        throw str;
	}

    Write_Debug("===============================================================");

#if USE_NEWGL
	//Checking GL version
	const GLubyte* GLVersionString = glGetString(GL_VERSION);
    const char* GLSLVersionString = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);

	int major, minor;

    sscanf((char*)GLVersionString, "%d%*c%d", &major, &minor);

    bool newGLContext = false;

    if(major > 3 || (major >= 3 && minor >= 2))
    {
	int attribs[] =
	{
		    WGL_CONTEXT_MAJOR_VERSION_ARB, major,
		    WGL_CONTEXT_MINOR_VERSION_ARB, minor,
            WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB
	};

    if(wglewIsSupported("WGL_ARB_create_context") == 1)
    {
            // FIXME: Set to NULL so NVIDIA will not create a NULL context.
            myhrc = wglCreateContextAttribsARB(dc, 0, NULL);//attribs);

        if(myhrc)
        {
            Write_Debug("WGL_ARB_create_context supported.");

            wglMakeCurrent(NULL, NULL);
            wglDeleteContext(Context);

            wglMakeCurrent(dc, myhrc);

                newGLContext = true;

            // Make sure we get the data from the correct GL context
	        GLenum err = glewInit();

	        if(err != GLEW_OK)
	        {
		        static char str[255];
		        sprintf(str, "Error: %s\n", glewGetErrorString(err));
                throw str;
	        }
        }
        else
        {
            Write_Debug("wglCreateContextAttribsARB failed. Using old style context.");
            myhrc = Context;
        }
    }
	else
	{
        //It's not possible to make a GL 3.x context. Use the old style context (GL 2.1 and before)
        Write_Debug("WGL_ARB_create_context not supported.");
		myhrc = Context;
	}
    }
    else
    {
        Write_Debug("Using old style context.");
        myhrc = Context;
    }

	//Checking GL version
	GLVersionString = glGetString(GL_VERSION);
    GLSLVersionString = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);

    sscanf((char*)GLVersionString, "%d%*c%d", &major, &minor);

    Write_Debug("OpenGL: %s %d %d GLSL: %s",
        GLVersionString, major, minor, GLSLVersionString);

    if(newGLContext)
    {
        if(major > 3 || (major >= 3 && minor >= 2))
        {
            float glslversion;
            sscanf(GLSLVersionString, "%f", &glslversion);

            if(glslversion >= 1.5f)
            {
                if(GLEW_ARB_vertex_array_object &&
				   GLEW_ARB_uniform_buffer_object &&
				   GLEW_ARB_texture_buffer_object &&
				   GLEW_EXT_gpu_shader4 &&
                   GLEW_ARB_texture_float &&
                   GLEW_ARB_draw_instanced)
                {
                    ogl::SetUseDeprecatedPath(false);
                    Write_Debug("Using OpenGL 3.2+ render path");
                }
            }
        }

        // Above extensions not supported.
        if(ogl::UseDeprecatedPath())
        {
            // Features not supported, go to old path
            wglMakeCurrent(NULL, NULL);
            wglDeleteContext(myhrc);

	        myhrc = wglCreateContext(dc);
	        wglMakeCurrent(dc, myhrc);

	        GLenum err = glewInit();

	        if(err != GLEW_OK)
	        {
		        static char str[255];
		        sprintf(str, "Error: %s\n", glewGetErrorString(err));
                throw str;
	        }
        }
    }
#else
    myhrc = Context;
#endif

    Write_Debug("===============================================================");

	if(!myhrc)
        return false;

	return true;
}

static LOGICAL oglBegin(HWND hwnd)
{
	PIXELFORMATDESCRIPTOR	pfd;

	// Already begun
	if (myhrc != NULL)
	{
		return(TRUE);
	}

	if (myhdc == NULL)
	{
		myhdc = GetDC(hwnd);
	}

	try
	{
		// Choose a compatible pixel format
		if (!ddpChooseScreenFormat(myhdc, &pfd))
		{
			ReleaseDC(hwnd, myhdc);
			myhdc = NULL;
			throw "Configured resolution not supported by video card";
		}

        CreateGLContext(myhdc);

		// Disable vsync during initialization. Enabled in rnmainInit (if user specified)
        // after initialization complete.
		if(wglSwapIntervalEXT)
			wglSwapIntervalEXT(NO_LOCK_TO_REFRESH);

		return(TRUE);
	}

    catch(std::exception& ex) { _initializationError(ex.what()); }
	catch(const char* error) { _initializationError(error); }
	catch(...) { _initializationError("[unknown error]"); }

	return (FALSE);
}

static void oglEnd(void)
{
	if (myhdc && myhrc)
	{
		wglMakeCurrent(myhdc, NULL);
		wglDeleteContext(myhrc);
		myhrc = NULL;
	}

	if (myhdc)
	{
		ReleaseDC(myhwnd, myhdc);
		myhdc = NULL;
	}
}

void ddpKill(void)
{
    // This function is called recursively with Alt-F4
    static LOGICAL alreadykilled = 0;

    if(alreadykilled)
    {
        return;
    }

    ClipCursor(NULL);

    alreadykilled = 1;

	imeKill();

	if (flags & DDWND_FULLSCREEN)
	{
		ddpResetDisplay();
	}

	if (d3dkillfunc != NULL)
	{
		d3dkillfunc();
	}

	if (ddkillfunc != NULL)
	{
		ddkillfunc();
	}

#if !defined(USE_OSG)
	rnmainKill();
#endif

	strpoolKill();

	oglEnd();

	if (wndAtom)
	{
		UnregisterClass(classname, (HINSTANCE)hinst);
		wndAtom = 0;
	}
}
static WNDCLASS	wndClass;

LOGICAL ddpCreateWnd(SINT32 myx, SINT32 myy, SINT32 mywidth, SINT32 myheight, SINT32 mybpp, UINT32 myflags, HWND parent)
{
UINT32		style;
RECT		rect, wrect;

	// Mode information
	WantedX		= myx;
	WantedY		= myy;
	WantedWidth	= mywidth;
	WantedHeight	= myheight;
	WantedBpp		= mybpp;

	flags			= 0;
	style			= WS_CLIPCHILDREN|WS_CLIPSIBLINGS;

	if (myflags & DDWND_FULLSCREEN)
	{
		WantedWindow = FALSE;
		flags |= DDWND_FULLSCREEN;
		style |= WS_POPUP|WS_VISIBLE;
	}
	else
	if (myflags & DDWND_WINDOW)
	{
	HDC     hdc;

		WantedWindow = TRUE;

		hdc = GetDC(NULL);
		WantedBpp = GetDeviceCaps(hdc, BITSPIXEL);
		ReleaseDC(NULL, hdc);

		if (parent == NULL)
		{
			style |= WS_POPUP;
		}
		else
		{
			style |= WS_CHILD;
		}

		style |= WS_VISIBLE;
		if (myflags & DDWND_CAPTION)
		{
	//		style |= WS_CAPTION;
		}

		if (myflags & DDWND_SYSMENU)
		{
			style |= WS_SYSMENU;
			if (myflags & DDWND_SIZEABLE)
			{
			//	style |= WS_MINIMIZEBOX|WS_MAXIMIZEBOX;
			}
		}

		if (myflags & DDWND_SIZEABLE)
		{
		//	style |= WS_THICKFRAME;
		}
	}
	else
	{
		// bad flags
		return(FALSE);
	}

	memset(&wndClass, 0, sizeof(WNDCLASS));
	wndClass.style          = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
	wndClass.lpfnWndProc    = WindowProc;
	wndClass.hInstance      = (HINSTANCE)hinst;
	wndClass.hIcon          = LoadIcon((HINSTANCE)hinst, MAKEINTRESOURCE(IDI_WWII_ICON));
	wndClass.hCursor        = LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));
	wndClass.hbrBackground  = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wndClass.lpszMenuName   = NULL;
	swprintf(classname, 255, _T("OpenGLClass_%dx%dx%d"), WantedWidth, WantedHeight, WantedBpp);
	wndClass.lpszClassName  = classname;

	wndAtom = RegisterClass(&wndClass);
	if (wndAtom == 0)
	{
		return(FALSE);
	}

	if (myflags & DDWND_FULLSCREEN)
	{
		WantedX = 0;
		WantedY = 0;

		if (!ddpSetDisplay(flags))
		{
			return(FALSE);
		}

		rect.left = WantedX;
		rect.top = WantedY;
		rect.right = WantedWidth;
		rect.bottom = WantedHeight;
	}
	else
	{
		rect.left = myx;
		rect.top = myy;
		rect.right = myx + WantedWidth;
		rect.bottom = myy + WantedHeight;

		AdjustWindowRect(&rect, style, FALSE);

#ifdef CENTER_WINDOW
		SystemParametersInfo(SPI_GETWORKAREA, 0, &wrect, 0);

		WantedX = myx = ((wrect.right - wrect.left) - WantedWidth) / 2;
		WantedY = myy = ((wrect.bottom - wrect.top) - WantedHeight) / 2;
		rect.right = myx + (rect.right - rect.left);
		rect.left = myx;
		rect.bottom = myy + (rect.bottom - rect.top);
		rect.top = myy;
#else
		if (rect.left < 0)
		{
			rect.right -= rect.left;
			rect.left = 0;
		}

		if (rect.top < 0)
		{
			rect.bottom -= rect.top;
			rect.top = 0;
		}

		if (rect.right > wrect.right)
		{
			rect.left -= rect.right - wrect.right;
			rect.right = wrect.right;
		}

		if (rect.bottom > wrect.bottom)
		{
			rect.top -= rect.bottom - wrect.bottom;
			rect.bottom = wrect.bottom;
		}
#endif
	}

	myhwnd = CreateWindowEx(0, wndClass.lpszClassName, _T(SYS_TITLE),
							style, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
							parent, NULL, (HINSTANCE)hinst, NULL);

	if (!myhwnd)
	{
		return(FALSE);
	}

    gamerect = rect;
    ClipCursor(&gamerect);

	{
	MSG		msg;

		while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
		{
			if (GetMessage(&msg, NULL, 0, 0))
			{
				uiIMEMessageProc(msg);
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}

	GetClientRect(myhwnd, &rect);

//	WantedX = rect.left;
//	WantedY = rect.top;
	WantedWidth = rect.right - rect.left;
	WantedHeight = rect.bottom - rect.top;

	if (!oglBegin(myhwnd))
	{
		ddpResetDisplay();
		return(FALSE);
	}

    // post process globals dependent on GLEW if needed
	cfgPrefsInit();

	// Call external init functions; if present
	if (ddinitfunc != NULL)
	{
		if (!ddinitfunc())
		{
			Write_Debug("DD init failed");
			return(FALSE);
		}
	}

	if (d3dinitfunc != NULL)
	{
		if (!d3dinitfunc())
		{
			Write_Debug("D3D init failed");
			return(FALSE);
		}
	}

	lastframetime = 0;
	ft1000mscount = 0;
	ft1000msticks = 0;
	ft500mscount = 0;
	ft500msticks = 0;
	ft200mscount = 0;
	ft200msticks = 0;
	ft100mscount = 0;
	ft100msticks = 0;
	ft50mscount = 0;
	ft50msticks = 0;
	ft10mscount = 0;
	frametime = 0;
	framecount = 0;

	{
	MSG		msg;

		while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
		{
			if (GetMessage(&msg, NULL, 0, 0))
			{
				uiIMEMessageProc(msg);
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}

	IMEBase* ime = imeClient();
	if(ime) ime->Init();

#if defined(USE_OSG)
	if(!osgGameInit()) return FALSE;
#else
	if (!rnmainInit())
	{
		return(FALSE);
	}

    // Lock or don't lock based on player's request in Settings app.
    // VSync causes major slowdown during initialization. Enable/Disable here after init done. (ticket #2329)
    // Forced disable in ddp.cpp before initialization starts.
    gSupportSwapInterval = g_enableVSync ? LOCK_TO_REFRESH : NO_LOCK_TO_REFRESH;
    if(wglSwapIntervalEXT)
	    wglSwapIntervalEXT(gSupportSwapInterval);

#endif
	return(TRUE);
}

LOGICAL ddpUpdate(void)
{
#ifdef DONT_USE
	if (!state)
	{
		Write_Debug("State is wrong");
		return(FALSE);
	}
#else
	if (!focus)
	{
		std::this_thread::yield();
	}
#endif

	if (!IsWindow(myhwnd))
	{
		Write_Debug("Not a window");
		return(FALSE);
	}

	if (IsIconic(myhwnd))
	{
		return(FALSE);
	}

	UpdateTimes();

	if(cursorvisible)
		SetCursor(wndClass.hCursor);

	// Member draw function
	if (!ddpDraw())
	{
		return(FALSE);
	}

#ifdef BETA_RELEASE
	logUpdateProfiling();
#endif	// #ifdef BETA_RELEASE

	if (resetfps)
	{
		starttime = timeGetTime();
		lastframetime = starttime;

		ft1000mscount = 0;
		ft1000msticks = 0;
		ft500mscount = 0;
		ft500msticks = 0;
		ft200mscount = 0;
		ft200msticks = 0;
		ft100mscount = 0;
		ft100msticks = 0;
		ft50mscount = 0;
		ft50msticks = 0;
		ft10mscount = 0;

		frametime = 0;

		framecount = 0;

		resetfps = FALSE;
	}

	if (capture > 0)
	{
		ddcapStart();
		capture -= 1;
	}

	if (!ddpFlip())
	{
		return(FALSE);
	}

	// activate or deactivate the ime as needed
	//imeUpdate();

	// an exit was declared
	if(GameInProgress == 2)
		PostMessageA(myhwnd, WM_CLOSE, 0, 0);

	return(TRUE);
}

LOGICAL ddpFlip(void)
{
	if (myhdc && myhrc)
	{
		// Flush drawing commands
#if(0)
		glFlush();
#endif
		// Flip the buffers
		SwapBuffers(myhdc);
	}

	{
	MSG		msg;

		while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
		{
			if (GetMessage(&msg, NULL, 0, 0))
			{
				uiIMEMessageProc(msg);
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else
			{
				////piExitProgram(__FILE__, __LINE__, "User exit requested!");
				running = FALSE;
				break;
			}
		}
	}

	return(TRUE);
}

LOGICAL ddpGetDDRect(RECT* rect)
{
	if (myhwnd == NULL ||
		rect == NULL)
	{
		return(FALSE);
	}

	GetClientRect(myhwnd, (RECT*)rect);
	ClientToScreen(myhwnd, (LPPOINT)&rect->left);
	ClientToScreen(myhwnd, (LPPOINT)&rect->right);

	return(TRUE);
}

LOGICAL ddpLockBackbuffer(BYTE** pbyte)
{
	//// FIXME
	return(FALSE);
}

void ddpUnlockBackbuffer()
{
}

LPVOID ddpGetBackbuffer()
{
	//// FIXME
	return(NULL);
}

LOGICAL ddpGetBackbufferSurface(void** lpdds)
{
	//// FIXME
	return(FALSE);
}

LOGICAL ddpDraw()
{
static SINT8	txt[64]="", ssz[64]="";
LOGICAL		retval=TRUE;
RECT		rect;

	if (ddrenderfunc != NULL)
	{
		if (!ddrenderfunc())
		{
			retval = FALSE;
		}
	}

	if (d3drenderfunc != NULL)
	{
		if (!d3drenderfunc())
		{
			retval = FALSE;
		}
	}

	if (show)
	{
		if (flags & DDWND_FULLSCREEN)
		{
			rect.top = rect.left = 0;
			rect.right = WantedWidth;
			rect.bottom = WantedHeight;
		}
		else
		{
			GetClientRect(myhwnd, &rect);
		}

		if ((framecount % 60) == 0)
		{
		float           fps;

			sprintf(ssz, "%dx%d", rect.right - rect.left, rect.bottom - rect.top);
			fps = ddpGetFPS();
			if (fps)
			{
				sprintf(txt,"%6.3f fps", fps);
			}
		}
	}

	return(retval);
}

LOGICAL ddpDrawNotActive(LOGICAL newdna)
{
LOGICAL		olddna = dna;

	dna = newdna;
	return(olddna);
}

LOGICAL ddpDisableScreenSaver(LOGICAL newdss)
{
LOGICAL		olddss = dss;

	dss = newdss;
	return(olddss);
}

static void UpdateTimes(void)
{
/*
UINT32	ticks;

	gLastFrameTime = gFrameTime;
	gFrameTime = timeGetTime() - starttime;

	ticks = gFt1000mscount;
	gFt1000mscount = gFrameTime / 1000;
	gFt1000msticks = gFt1000mscount - ticks;
	ticks = gFt500mscount;
	gFt500mscount = gFrameTime / 500;
	gFt500msticks = gFt500mscount - ticks;
	ticks = gFt333mscount;
	gFt333mscount = gFrameTime / 333;
	gFt333msticks = gFt333mscount - ticks;
	ticks = gFt250mscount;
	gFt250mscount = gFrameTime / 250;
	gFt250msticks = gFt250mscount - ticks;
	ticks = gFt200mscount;
	gFt200mscount = gFrameTime / 200;
	gFt200msticks = gFt200mscount - ticks;
	ticks = gFt100mscount;
	gFt100mscount = gFrameTime / 100;
	gFt100msticks = gFt100mscount - ticks;
	ticks = gFt50mscount;
	gFt50mscount = gFrameTime / 50;
	gFt50msticks = gFt50mscount - ticks;
	ticks = gFt10mscount;
	gFt10mscount = gFrameTime / 10;
	gFt10msticks = gFt10mscount - ticks;
*/
	framecount += 1;
}

SINT32 ddpGetTotalPaints(void)
{
	return(framecount);
}

FLOAT32 ddpGetFPS()
{
FLOAT32		fps=0.0f;

	if (lastframetime)
	{
		fps = (float)framecount * 1000.0f / (float)lastframetime;
	}

	return(fps);
}

FLOAT32 ddpGetInstantFPS()
{
FLOAT32	fps=0.0f;
UINT32	deltatime, deltacount;
//static UINT32	lastframetime;
static UINT32	lastframecount;

	deltatime = frametime - lastframetime;
	if (deltatime)
	{
		deltacount = framecount - lastframecount;
		fps = (FLOAT32)deltacount * 1000.0f / (FLOAT32)deltatime;
	}

	//lastframetime = frametime;
	lastframecount = framecount;

	return(fps);
}

void ddpResetFPS()
{
	resetfps = TRUE;
}

void ddpCapture()
{
	capture += 1;
}

SINT32 ddpGetWidth()
{
	return(WantedWidth);
}

SINT32 ddpGetHeight()
{
	return(WantedHeight);
}

SINT32 ddpGetBPP()
{
	return(WantedBpp);
}

LOGICAL ddpSetDDInit(DDWND_DDINITFUNC newfunc)
{
	ddinitfunc = newfunc;
	return(TRUE);
}

LOGICAL ddpSetD3DInit(DDWND_D3DINITFUNC newfunc)
{
	d3dinitfunc = newfunc;
	return(TRUE);
}

LOGICAL ddpSetDDRender(DDWND_DDRENDERFUNC newfunc)
{
	ddrenderfunc = newfunc;
	return(TRUE);
}

LOGICAL ddpSetD3DRender(DDWND_D3DRENDERFUNC newfunc)
{
	d3drenderfunc = newfunc;
	return(TRUE);
}

LOGICAL ddpSetDDKill(DDWND_DDKILLFUNC newfunc)
{
	ddkillfunc = newfunc;
	return(TRUE);
}

LOGICAL ddpSetD3DKill(DDWND_D3DKILLFUNC newfunc)
{
	d3dkillfunc = newfunc;
	return(TRUE);
}

LOGICAL ddpSetMouse(DDWND_MOUSEFUNC newfunc)
{
	mousefunc = newfunc;
	return(TRUE);
}

LOGICAL ddpSetTimer(DDWND_TIMERFUNC newfunc)
{
	timerfunc = newfunc;
	return(TRUE);
}

LOGICAL ddpSetKeyboard(DDWND_KEYBOARDFUNC newfunc)
{
	keyboardfunc = newfunc;
	return(TRUE);
}


// Detached thread
UINT32 ddpEngine(LPVOID dummy)
{
LOGICAL		retval, *running;

#if !defined(_DEBUG)
	__try
	{
#endif

	running = (LOGICAL*)dummy;

	WantedHdw = FALSE;	//// TEMPORARY (until single pass (i.e. hardware driven) multi-texturing is working)

	if (!ddpInit())
	{
		return(FALSE);
	}

	ddpDrawNotActive(FALSE);
	ddpDisableScreenSaver(TRUE);

	ddpSetDDInit(NULL);//oglInit);
	ddpSetDDRender(NULL);//rnlogoRender);
	ddpSetDDKill(NULL);//oglKill);

	ddpSetD3DInit(piInit);
#if defined(USE_OSG)
	ddpSetD3DRender(osgGameUpdate);
#else
	ddpSetD3DRender(gmlpMainUpdate);
#endif
	ddpSetD3DKill(piKill);

	if (!ddpCreateWnd(WantedX, WantedY, WantedWidth, WantedHeight, WantedBpp, WantedWindow ? DDWND_FULLWINDOW : DDWND_FULLSCREEN, NULL))
	{
#if defined(USE_OSG)
#else
		ddpKill();
#endif
		return(0);
	}

	while (*running)
	{
		retval = ddpUpdate();
		if (!retval)
		{
			Sleep(10);
		}

		if (!state)
		{
			Sleep(1);
		}
	}

#if !defined(_DEBUG)
	}
	__except(ExceptionHandler(GetExceptionCode(), GetExceptionInformation()))
	{
		Write_Debug("Exception caught!");
	}
#endif

	if (myhwnd != NULL)
	{
		*running = 0;
		PostMessageA(myhwnd, WM_CLOSE, 0, 0);
	}

	return(0);
}

UINT32 ddpEngineCycle(LPVOID dummy)
{
LOGICAL		retval;

	retval = ddpUpdate();
	if (!retval)
	{
		Sleep(10);
	}

	return(retval);
}

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT umsg, WPARAM wParam, LPARAM lParam)
{
	if (umsg >= WM_MOUSEFIRST &&
		umsg <= WM_MOUSELAST)
	{
		if (mousefunc != NULL)
		{
			mousefunc(umsg, wParam, lParam);
		}
	}
	else
	switch (umsg)
	{
	case WM_CREATE:
	{
		myhwnd = hwnd;
		if (!Create(hwnd))
		{
			return(-1);
		}

		break;
	}
	case WM_DESTROY:
	{
		Write_Debug("WM_DESTROY");
		break;
	}
	case WM_CLOSE:
	{
		// User has hit Alt-F4. Tell the server.
		cfgPrefsSave();
		wwiiSendUserAbortedApp();
		OnClose();
		myhwnd = NULL;
		PostQuitMessage(0);
		break;
	}
	case WM_ERASEBKGND:
	{
		return(OnEraseBkgnd((HDC) wParam));
	}
	case WM_PAINT:
	{
		// Do you mean to call the local function rather than the local member function _OnPaint?
		_OnPaint();
		break;
	}
	case WM_SETFOCUS:
	{
		OnSetFocus((HWND) wParam);
        ClipCursor(&gamerect);
		break;
	}
	case WM_KILLFOCUS:
	{
		OnKillFocus((HWND) wParam);
        ClipCursor(NULL);
		break;
	}
	case WM_NCACTIVATE:
	{
		OnNcActivate((LOGICAL) wParam);
		break;
	}
	case WM_MOVE:
	{
	DWORD	xy, wh;
	RECT	rect;

		if (!(flags & DDWND_FULLSCREEN))
		{
			if (IsWindowVisible(hwnd))
			{
				GetClientRect(hwnd, &rect);
				ClientToScreen(hwnd, (LPPOINT)&rect);
				xy = MAKELONG(rect.left, rect.top);
				sysregSetKeyDword(REG_KEY_MAIN, REG_MAIN_LOC, &xy);
				wh = MAKELONG(rect.right, rect.bottom);
				sysregSetKeyDword(REG_KEY_MAIN, REG_MAIN_SIZE, &wh);
			}
		}
		break;
	}
	case WM_SIZE:
	{
		OnSize(wParam, LOWORD(lParam), HIWORD(lParam));
		break;
	}
	case WM_SYSKEYDOWN:
	{
		OnSysKeyDown((int) wParam, lParam);
		break;
	}
	case WM_SYSKEYUP:
	{
		OnSysKeyUp((int) wParam, lParam);
		break;
	}
	case WM_SYSCOMMAND:
	{
		// WM_SYSCOMMAND:   return FALSE if wParam is SC_SCREENSAVE
		switch (wParam)
		{
		case SC_KEYMENU:
			if (state)
			{
				return(FALSE);
			}
			break;
		case SC_SCREENSAVE:
		{
			if (dss)
			{
				return(FALSE);
			}

			break;
		}
		case SC_MONITORPOWER:
		{
			Write_Debug("Monitor power off: %d", lParam);
			break;
		}
		case SC_CLOSE:
		{
#ifndef BETA_RELEASE // Ticket# 6554 Allow alt-f4 for beta releases
#if defined(RELEASE_CLIENT)
			if(gCellClientActive)
				return FALSE; // prevent alt-f4's while spawned
#endif
#endif

			if (!state)
			{
				state = TRUE;
			}
			break;
		}
		default:
		{
			Write_Debug("WM_SYSCOMMAND 0x%x", wParam);
			break;
		}
		}

	break;
	}
	case WM_ENTERMENULOOP:
	{
		if (state)
		{
			state = FALSE;
		}

		dipUnacquire();
		Write_Debug("WM_ENTERMENULOOP");
		break;
	}
	case WM_EXITMENULOOP:
	{
		if (!state)
		{
			state = TRUE;
		}

		dipUnacquire();
		Write_Debug("WM_EXITMENULOOP");
		break;
	}
	case WM_ENTERSIZEMOVE:
	{
		if (state)
		{
			state = FALSE;
		}

		dipUnacquire();
		Write_Debug("WM_ENTERSIZEMOVE");
		break;
	}
	case WM_EXITSIZEMOVE:
	{
	RECT  rect;

		if (!state)
		{
			state = TRUE;
		}

		GetClientRect(myhwnd, &rect);
		if (ddpGetWidth() != rect.right ||
			ddpGetHeight() != rect.bottom)
		{
			  Destroy();
			  if (!Create(hwnd))
			  {
				  Write_Debug("An error was encountered in WM_EXITSIZEMOVE");
				  PostMessageA(hwnd, WM_CLOSE, 0, 0);
			  }
		}

		dipAcquire();
		Write_Debug("WM_EXITSIZEMOVE");
		break;
	}
	case WM_DISPLAYCHANGE:
	{
		if (!(flags & DDWND_FULLSCREEN))
		{
			WantedBpp = wParam;
			if (!state)
			{
				state = TRUE;
			}
		}

		Write_Debug("WM_DISPLAYCHANGE");
		break;
	}
	case WM_TIMER:
	{
		if (timerfunc != NULL)
		{
			timerfunc(wParam, lParam);
		}
		break;
	}
	case WM_GETMINMAXINFO:
	{
	LPMINMAXINFO	lpmmi;
	RECT	rect;

		SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);
		lpmmi = (LPMINMAXINFO)lParam;
		lpmmi->ptMaxSize.x = rect.right - rect.left;
		lpmmi->ptMaxSize.y = rect.bottom - rect.top;
		lpmmi->ptMinTrackSize.x = 320;
		lpmmi->ptMinTrackSize.y = 240;
		lpmmi->ptMaxTrackSize.x = lpmmi->ptMaxSize.x;
		lpmmi->ptMaxTrackSize.y = lpmmi->ptMaxSize.y;
		lpmmi->ptMaxPosition.x = ((rect.right - rect.left) - lpmmi->ptMaxSize.x) / 2;
		lpmmi->ptMaxPosition.y = ((rect.bottom - rect.top) - lpmmi->ptMaxSize.y) / 2;
		return(0);
	}
    case WM_IME_CHAR:
        {
			if(dimainAddChar(wParam))
				return 0;

			break;
        }
    case WM_CHAR:
        {
			if(dimainAddChar(wParam))
				return 0;

			break;
        }
    case WM_KEYDOWN:
        {
            UINT32 uScanCode = (lParam >> 16) & 0x000000FF;

			if(dimainVKDown((UINT32)wParam, uScanCode))
				return 0;

			break;
        }

    case WM_KEYUP:
        {
            UINT32 uScanCode = (lParam >> 16) & 0x000000FF;

            if(dimainVKUp((UINT32)wParam, uScanCode))
				return 0;

			break;
        }
	default:
		{
			IMEBase* ime = imeClient();

			if(ime && ime->OnMessage(umsg, wParam, lParam))
				return 0;

			break;
		}
	}

	return(DefWindowProc(hwnd, umsg, wParam, lParam));
}

static LOGICAL Create(HWND hwnd)
{
RECT					rect;

	if (hwnd == NULL)
	{
		return(TRUE);
	}

	GetClientRect(hwnd, &rect);

	WantedWidth = rect.right - rect.left;
	WantedHeight = rect.bottom - rect.top;

	Write_Debug("Create - x=%d, y=%d, w=%d, h=%d, bpp=%d", rect.left, rect.top, WantedWidth, WantedHeight, WantedBpp);

	running = TRUE;

	InvalidateRect(hwnd, NULL, FALSE);

	ddpResetFPS();
	//lastframetime = frametime = timeGetTime();

	Write_Debug("Completed Create");

	return(TRUE);
}

static void Destroy()
{
	if (running)
	{
	float   fps;

		Write_Debug("Total number of paints performed was %d", framecount);
		Write_Debug("Total time for paints performed was %d", frametime);

		fps = ddpGetFPS();
		if (fps)
		{
			Write_Debug("For a frame rate of %6.3f", fps);
		}
	}

	running = FALSE;

	Write_Debug("Completed Destroy");

	return;
}

#if(0)
static LOGICAL MoveSize()
{
LOGICAL		retval;

	Destroy();
	retval = Create(myhwnd);
	if (!retval)
	{
		Write_Debug("An error was encountered in MoveSize");
		PostMessageA(myhwnd, WM_CLOSE, 0, 0);
	}

	oglResize(WantedWidth, WantedHeight);

	Write_Debug("Completed MoveSize");

	return(retval);
}
#endif

static void OnClose()
{
	Destroy();

	Write_Debug("Completed OnClose");
}

static LOGICAL OnEraseBkgnd(HDC hdc)
{
RECT   rect;

	GetClientRect(myhwnd, &rect);
	FillRect(hdc, &rect, (HBRUSH)GetStockObject(BLACK_BRUSH));

	Write_Debug("OnEraseBkgnd");

	return(TRUE);
}

static void _OnPaint()
{
	Write_Debug("OnPaint");
}

static void OnSetFocus(HWND pOldWnd)
{
	if (!(flags & DDWND_FULLSCREEN))
	{
		if (!dna)
		{
			if (!state)
			{
				state = TRUE;
			}
		}
	}

	focus = TRUE;

	dipAcquire();

	Write_Debug("OnSetFocus");
}

static void OnKillFocus(HWND pNewWnd)
{
	if (!(flags & DDWND_FULLSCREEN))
	{
		if (!dna)
		{
			if (state)
			{
				state = FALSE;
			}
		}
	}

	focus = FALSE;

	dipUnacquire();

	Write_Debug("OnKillFocus");
}

static void OnSize(UINT nType, int cx, int cy)
{
	if ((WantedWidth != cx || WantedHeight != cy) &&
		(cx != 0 && cy != 0))
	{
		if (flags & DDWND_FULLSCREEN)
		{
			oglEnd();

			ddpResetDisplay();
			Sleep(1000);
			ddpSetDisplay(flags);

			if (!oglBegin(myhwnd))
			{
				PostQuitMessage(0);
			}
		}
		else
		{
			WantedWidth = cx;
			WantedHeight = cy;
		}

		oglResize(WantedWidth, WantedHeight);
	}

	Write_Debug("OnSize: x=%d, y=%d", cx, cy);
}

static LOGICAL OnNcActivate(LOGICAL bActive)
{
	if (!(flags & DDWND_FULLSCREEN))
	{
		if (bActive)
		{
			if (!state)
			{
				state = TRUE;
			}
		}
		else
		{
			if (!dna)
			{
				if (state)
				{
					state = FALSE;
				}
			}
		}
	}

	if (bActive)
	{
#if RELEASE_CLIENT != 0
		SetWindowPos(myhwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
#endif
		dipAcquire();
	}
	else
	{
#if RELEASE_CLIENT != 0
		SetWindowPos(myhwnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
#endif
		dipUnacquire();
	}

	Write_Debug("OnNcActivate %s", state == TRUE ? "TRUE" : "FALSE");

	return(0);
}

static void OnSysKeyDown(UINT nChar, UINT nFlags)
{
	if (!(flags & DDWND_FULLSCREEN))
	{
		if (!dna)
		{
			if (state)
			{
				state = FALSE;
			}
		}
	}

	Write_Debug("OnSysKeyDown: 0x%x", nChar);
}

static void OnSysKeyUp(UINT nChar, UINT nFlags)
{
	if (!(flags & DDWND_FULLSCREEN))
	{
		if (!dna)
		{
			if (!state)
			{
				state = TRUE;
			}
		}
	}

	Write_Debug("OnSysKeyUp: 0x%x", nChar);
}

LOGICAL ddpChooseScreenFormat(HDC hdc, PIXELFORMATDESCRIPTOR* pfd)
{
SINT32					i, pixelformat, depth, accum, stencil;

	memset(pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));

	pfd->nSize				= sizeof(PIXELFORMATDESCRIPTOR);
	pfd->nVersion			= 1;
	pfd->dwFlags			= PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;
	pfd->iPixelType			= PFD_TYPE_RGBA;
	pfd->cColorBits			= WantedBpp;
	pfd->cRedBits			= 0;
	pfd->cRedShift			= 0;
	pfd->cGreenBits			= 0;
	pfd->cGreenShift		= 0;
	pfd->cBlueBits			= 0;
	pfd->cBlueShift			= 0;
	pfd->cAlphaBits			= 0;
	pfd->cAlphaShift		= 0;
	pfd->cAccumBits			= 0;
	pfd->cAccumRedBits		= 0;
	pfd->cAccumGreenBits	= 0;
	pfd->cAccumBlueBits		= 0;
	pfd->cAccumAlphaBits	= 0;
	pfd->cDepthBits			= 32;
	pfd->cStencilBits		= 8;
	pfd->cAuxBuffers		= 0;
	pfd->iLayerType			= PFD_MAIN_PLANE;
	pfd->bReserved			= 0;
	pfd->dwLayerMask		= 0;
	pfd->dwVisibleMask		= 0;
	pfd->dwDamageMask		= 0;

	pixelformat = ChoosePixelFormat(hdc, pfd);
	if (pixelformat == 0)
	{
		Write_Debug("Unable to choose a compatible pixel format");
		return(FALSE);
	}

	Write_Debug("ChoosePixelFormat recommended #%d", pixelformat);

	depth = 24;
	accum = 0;
	stencil = 8;
	for (i=1;; i++)
	{
		if (!DescribePixelFormat(hdc, i, sizeof(PIXELFORMATDESCRIPTOR), pfd))
		{
			Write_Debug("There were %d pixel formats", i - 1);
			break;
		}

#ifdef DDP_EVENT_LOG
		Write_Debug(" Format #%2d,"
					" Version=%d,"
					" Flags=0x%04x,"
					" PixelType=%s,"
					" ColorBits=%2d,"
					" R=%d,"
					" G=%d,"
					" B=%d,"
					" A=%d,"
					" AccumBits=%2d,"
					" R=%2d,"
					" G=%2d,"
					" B=%2d,"
					" A=%2d,"
					" DepthBits=%2d,"
					" StencilBits=%2d",
					i,
					pfd->nVersion,
					pfd->dwFlags,
					pfd->iPixelType == PFD_TYPE_RGBA ? "RGB" : "Pal",
					pfd->cColorBits,
					pfd->cRedBits,
					pfd->cGreenBits,
					pfd->cBlueBits,
					pfd->cAlphaBits,
					pfd->cAccumBits,
					pfd->cAccumRedBits,
					pfd->cAccumGreenBits,
					pfd->cAccumBlueBits,
					pfd->cAccumAlphaBits,
					pfd->cDepthBits,
					pfd->cStencilBits);
#endif	// DDP_EVENT_LOG

#ifdef DDP_EVENT_LOG_EXTREME
		// Just show all the flags that are set
		if (pfd->dwFlags & PFD_SUPPORT_DIRECTDRAW)
		{
			Write_Debug(" Supports DirectDraw");
		}

		if (pfd->dwFlags & PFD_GENERIC_ACCELERATED)
		{
			Write_Debug(" Hardware accelerated");
		}

		if (pfd->dwFlags & PFD_SWAP_LAYER_BUFFERS)
		{
			Write_Debug(" Swap layer buffers");
		}

		if (pfd->dwFlags & PFD_SWAP_COPY)
		{
			Write_Debug(" Swap Copy");
		}

		if (pfd->dwFlags & PFD_SWAP_EXCHANGE)
		{
			Write_Debug(" Swap exchange");
		}

		if (pfd->dwFlags & PFD_NEED_SYSTEM_PALETTE)
		{
			Write_Debug(" Need system palette");
		}

		if (pfd->dwFlags & PFD_NEED_PALETTE)
		{
			Write_Debug(" Need palette");
		}

		if (pfd->dwFlags & PFD_GENERIC_FORMAT)
		{
			Write_Debug(" Generic format (unaccelerated software)");
		}

		if (pfd->dwFlags & PFD_SUPPORT_OPENGL)
		{
			Write_Debug(" Supports OpenGL");
		}

		if (pfd->dwFlags & PFD_SUPPORT_GDI)
		{
			Write_Debug(" Supports GDI");
		}

		if (pfd->dwFlags & PFD_DRAW_TO_BITMAP)
		{
			Write_Debug(" Can draw to bitmap");
		}

		if (pfd->dwFlags & PFD_DRAW_TO_WINDOW)
		{
			Write_Debug(" Can draw to window");
		}

		if (pfd->dwFlags & PFD_STEREO)
		{
			Write_Debug(" Supports stereoscopic display");
		}

		if (pfd->dwFlags & PFD_DOUBLEBUFFER)
		{
			Write_Debug(" Double buffered");
		}
#endif	// DDP_EVENT_LOG_EXTREME

		if ((WantedWindow ? (pfd->dwFlags & PFD_DRAW_TO_WINDOW) : TRUE) &&
			(pfd->dwFlags & PFD_SUPPORT_OPENGL) &&
			(pfd->dwFlags & PFD_DOUBLEBUFFER) &&
			!(pfd->dwFlags & PFD_GENERIC_FORMAT) &&
			pfd->iLayerType == PFD_MAIN_PLANE &&
			//pfd->iPixelType == PFD_TYPE_RGBA &&
			pfd->cDepthBits >= depth &&		// Largest bit depth
		//	pfd->cAccumBits >= accum &&		// Largest accumulators
			pfd->cStencilBits >= stencil &&	// Largest stencil buffer
			pfd->cColorBits == WantedBpp)
		{
			depth = pfd->cDepthBits;
			accum = pfd->cAccumBits;
			stencil = pfd->cStencilBits;
			pixelformat = i;
		}
	}

	Write_Debug("Deduced pixel format was #%d", pixelformat);

	// Set the pixel format selected
	if (!SetPixelFormat(hdc, pixelformat, pfd))
	{
		Write_Debug("Unable to set the desired pixel format");
		return(FALSE);
	}

	return(TRUE);
}

LOGICAL ddpSetDisplay(UINT32 flags)
{
LOGICAL			found;
UINT32			retval;
DEVMODE			testmode, presentmode;

	if (flags & DDWND_FULLSCREEN)
	{
		found = FALSE;
		displaynum = 0;
		displaydevice.cb = sizeof(DISPLAY_DEVICE);
		do
		{
			if (!EnumDisplayDevices(NULL, displaynum, &displaydevice, 0))
			{
				break;
			}

			if (displaydevice.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
			{
				found = TRUE;
				break;
			}

			displaynum += 1;
		}
		while (TRUE);

		if (found)
		{
			Write_Debug("Found desktop display device");
		}

		EnumDisplaySettings(displaydevice.DeviceName, ENUM_CURRENT_SETTINGS, &presentmode);
		Write_Debug("Present mode: w=%d, h=%d, bpp=%d, ref=%d",
			presentmode.dmPelsWidth,
			presentmode.dmPelsHeight,
			presentmode.dmBitsPerPel,
			presentmode.dmDisplayFrequency);

		found = FALSE;
		modenum = 0;
		Write_Debug("Enumerated modes:");
		do
		{
			if (!EnumDisplaySettings(displaydevice.DeviceName, modenum, &devmode))
			{
				break;
			}

			Write_Debug("                  w=%d, h=%d, bpp=%d, ref=%d",
				devmode.dmPelsWidth,
				devmode.dmPelsHeight,
				devmode.dmBitsPerPel,
				devmode.dmDisplayFrequency);

			if (devmode.dmBitsPerPel == (UINT32)WantedBpp &&
				devmode.dmPelsWidth == (UINT32)WantedWidth &&
				devmode.dmPelsHeight == (UINT32)WantedHeight &&
				devmode.dmDisplayOrientation == 0)
			{
				found = TRUE;
				break;
			}

			modenum += 1;
		}
		while (TRUE);

		if (!found)
		{
			Write_Debug("No compatible display mode exists");
			return(FALSE);
		}

		modenum += 1;
		do
		{
			if (!EnumDisplaySettings(displaydevice.DeviceName, modenum, &testmode))
			{
				break;
			}

			Write_Debug("                  w=%d, h=%d, bpp=%d, ref=%d",
				testmode.dmPelsWidth,
				testmode.dmPelsHeight,
				testmode.dmBitsPerPel,
				testmode.dmDisplayFrequency);

			if (testmode.dmBitsPerPel == (UINT32)WantedBpp &&
				testmode.dmPelsWidth == (UINT32)WantedWidth &&
				testmode.dmPelsHeight == (UINT32)WantedHeight &&
				testmode.dmDisplayOrientation == 0 &&
				testmode.dmDisplayFrequency <= presentmode.dmDisplayFrequency &&
				(testmode.dmDisplayFrequency == 0 ||
				(abs((int)testmode.dmDisplayFrequency - (int)presentmode.dmDisplayFrequency) <=
				abs((int)devmode.dmDisplayFrequency - (int)presentmode.dmDisplayFrequency))))
			{
				devmode = testmode;
				found = TRUE;
			}

			modenum += 1;
		}
		while (TRUE);

		if (!found)
		{
			Write_Debug("No compatible display mode exists");
			return(FALSE);
		}

		retval = ChangeDisplaySettingsEx(displaydevice.DeviceName, &devmode, NULL, CDS_TEST, NULL);
		switch (retval)
		{
		case DISP_CHANGE_BADFLAGS:
			Write_Debug("DISP_CHANGE_BADFLAGS");
			break;
		case DISP_CHANGE_BADMODE:
			Write_Debug("DISP_CHANGE_BADMODE");
			break;
		case DISP_CHANGE_BADPARAM:
			Write_Debug("DISP_CHANGE_BADPARAM");
			break;
		case DISP_CHANGE_FAILED:
			Write_Debug("DISP_CHANGE_FAILED");
			break;
		case DISP_CHANGE_NOTUPDATED:
			Write_Debug("DISP_CHANGE_NOTUPDATED");
			break;
		case DISP_CHANGE_RESTART:
			Write_Debug("DISP_CHANGE_RESTART");
			break;
		case DISP_CHANGE_SUCCESSFUL:
			Write_Debug("DISP_CHANGE_SUCCESSFUL");
			ChangeDisplaySettingsEx(displaydevice.DeviceName, &devmode, NULL, CDS_FULLSCREEN, NULL);

			return(TRUE);
		}

		return(FALSE);
	}

	return(TRUE);
}

void ddpResetDisplay(void)
{
	ChangeDisplaySettingsEx(displaydevice.DeviceName, NULL, NULL, 0, NULL);
}

LOGICAL ddpDrawToScreen(LPVOID lpdds)
{
	return(TRUE);
}

DWORD GetMillisecondTick(void)
{
	SINT32					deltaQueryTime, deltaGetTime;
	_LARGE_INTEGER			frequency			= {0};
	_LARGE_INTEGER			query				= {0};
	static DWORD			lastQueryTime		= 0;
	static DWORD			lastGetTime			= 0;
	static DWORD			correctedQueryTime	= 0;


	QueryPerformanceFrequency(&frequency);				// We should do this every time because the clockrate can vary
	QueryPerformanceCounter(&query);

	DWORD thisQueryTime	= (long)(query.QuadPart * 1000 / frequency.QuadPart);
	DWORD thisGetTime	= timeGetTime();

	deltaQueryTime	= thisQueryTime - lastQueryTime;
	deltaGetTime	= thisGetTime - lastGetTime;

	if(abs(deltaQueryTime - deltaGetTime) > 50)		correctedQueryTime += deltaGetTime;
	else	correctedQueryTime += deltaQueryTime;

	lastQueryTime = thisQueryTime;
	lastGetTime = thisGetTime;

	return	correctedQueryTime;
}

static void capSave(UCHAR* const bptr, UINT32 width, UINT32 height, const char* const bname)
{
	const UCHAR*	ptrin;
	UCHAR*			bits, *ptrout;
	UINT32			i, j;
	UINT32			size, bwidth, twidth, tsize;
	piBITMAPINFO*	dib;

	bwidth = width * 3;		// 3 bytes per pixel
	size = bwidth * height;

	// DIBs must be DWORD aligned
	if (3 & bwidth)
	{
		twidth = (bwidth + 3) & (~3);
		tsize = twidth * height;
	}
	else
	{
		twidth = bwidth;
		tsize = size;
	}

	dib = (piBITMAPINFO*)malloc(tsize + sizeof(piBITMAPINFO));
	if (dib)
	{
		dib->bmiHeader.biSize = sizeof(dib->bmiHeader);
		dib->bmiHeader.biWidth = width;
		dib->bmiHeader.biHeight = height;	//	top down bitmap
		dib->bmiHeader.biPlanes = 1;
		dib->bmiHeader.biBitCount = 24;
		dib->bmiHeader.biCompression = BI_RGB;
		dib->bmiHeader.biSizeImage = tsize;
		dib->bmiHeader.biXPelsPerMeter = 0;
		dib->bmiHeader.biYPelsPerMeter = 0;
		dib->bmiHeader.biClrUsed = 0;
		dib->bmiHeader.biClrImportant = 0;

		bits = (UCHAR*)dib + dib->bmiHeader.biSize;

		ptrin 	= bptr;
		ptrout 	= bits;
		for (i=0; i < height; i++)
		{
			// copy a row
			for (j=0; j < bwidth; j += 3, ptrin += 3)
			{
				*ptrout++ = *(ptrin+2);
				*ptrout++ = *(ptrin+1);
				*ptrout++ = *(ptrin+0);
			}

			// pad rest of DWORD with zeros
			for (; j < twidth; j++)
			{
				*ptrout++ = 0;
			}
		}

		// create a file from the DIB
		if (NULL != bname)
		{
			piBITMAPFILEHEADER	bmfh;

			bmfh.bfType = (UINT16)('B' + ('M' << 8));
			bmfh.bfSize = sizeof(bmfh) + dib->bmiHeader.biSize + twidth * height;
			bmfh.bfReserved1 = 0;
			bmfh.bfReserved2 = 0;
			bmfh.bfOffBits = sizeof(bmfh) + dib->bmiHeader.biSize;

			size = dib->bmiHeader.biSize + dib->bmiHeader.biSizeImage;

			FILE* cf = fopen(bname, "wb+");
			if ( cf )
			{
				fwrite(&bmfh, 1, sizeof(bmfh), cf);
				fwrite(dib, 1, size, cf);
				fclose(cf);
			}
		}
	}

	free(dib);
	free(bptr);
}

void ddpSaveImgAsBitmap(unsigned char* img, int w, int h, const char* filename)
{
    capSave(img, w, h, filename);
}

static void glScreenCapture()
{
	int shotCount=0;
	char fileName[128], *fptr;
	UCHAR* bitmap = (UCHAR*)memAlloc(WantedWidth * WantedHeight * 3);
	if(bitmap == NULL)
		return;

#ifdef BUZZARDCAM
	fptr = "buzzard";
#else	// BUZZARDCAM
	fptr = "SShot";
#endif	// BUZZARDCAM

	do
	{
		shotCount += 1;
#if platOS == OS_WIN32
		sprintf(fileName, "%s%s%d.bmp", UserDataPath, fptr, shotCount);
#else
		sprintf(fileName, "%s%d.bmp", fptr, shotCount);
#endif
	}
	while (_access(fileName, 0) == 0);

	glReadPixels(0, 0, WantedWidth, WantedHeight, GL_RGB, GL_UNSIGNED_BYTE, bitmap);

	// Cap save will release the bitmap memory.
	capSave(bitmap, WantedWidth, WantedHeight, fileName);
}

LOGICAL ddcapStart(void)
{
	glScreenCapture();
	return(TRUE);
}


void ddpShowCursor()
{
	int count = ShowCursor(TRUE);
	while(count <= 1)
			count = ShowCursor(TRUE);
	SetCursor(wndClass.hCursor);
	cursorvisible = 1;
}

void ddpHideCursor()
{
	int count = ShowCursor(FALSE);
	while(count > -1)
		count = ShowCursor(FALSE);
	cursorvisible = 0;
}

void ddpPumpMessage()
{
    static MSG msg;

    while( PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE ) )
    {
        if( !GetMessage( &msg, NULL, 0, 0 ) )
        {
            PostQuitMessage( ( int )msg.wParam );
            return;
        }

		uiIMEMessageProc(msg);
        TranslateMessage( &msg );
        DispatchMessage( &msg );
    }
}

HWND ddpGetHWND()
{
	return myhwnd;
}

#endif
