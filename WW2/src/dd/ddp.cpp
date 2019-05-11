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
#include "wwii/wwiiUTF.h"

#include "ddp.h"

#include "gl/wglew.h"

//#define GLOBALS
//#undef GLOBALS

extern void wwiiSendUserAbortedApp();

#define DDP_EVENT_LOG
//#undef DDP_EVENT_LOG

#define DDP_EVENT_LOG_EXTREME
//#undef DDP_EVENT_LOG_EXTREME

#define CENTER_WINDOW

extern LOGICAL					running;
static int cursorvisible;
static HCURSOR cursor = NULL;
/*
	Static variables
*/
static ATOM						wndAtom;

static bool						dna;
static bool						dss;
static bool						state;
static bool						resetfps;
static bool						focus;

static SINT32					capture;

static TCHAR					classname[256];

static enum WindowMode {
	FULLSCREEN, BORDERLESS_WINDOW, WINDOWED
} window_mode;
static UINT32					delay;
static UINT32					starttime;
static long						framecount;

static HRESULT					retval;

static HDC						myhdc;
static HGLRC					myhrc;

static GLint default_framebuffer;

static DDWND_DDINITFUNC			ddinitfunc;
static DDWND_D3DINITFUNC		d3dinitfunc;
static DDWND_DDRENDERFUNC		ddrenderfunc;
static DDWND_D3DRENDERFUNC		d3drenderfunc;
static DDWND_DDKILLFUNC			ddkillfunc;
static DDWND_D3DKILLFUNC		d3dkillfunc;
static DDWND_MOUSEFUNC			mousefunc;
static DDWND_TIMERFUNC			timerfunc;
static DDWND_KEYBOARDFUNC		keyboardfunc;

// The rendering portion of the window.
static RECT renderrect;

/*
	Global variables
*/
HANDLE				hinst;
HWND				myhwnd;

/*
	Static functions
*/
static LRESULT CALLBACK			WindowProc(HWND hwnd, UINT umsg, WPARAM wParam, LPARAM lParam);
static void UpdateTimes();

static LOGICAL OnEraseBkgnd(HDC hDC);
static LOGICAL OnNcActivate(LOGICAL bActive);

static void	Destroy();
static void	OnClose();
static void	_OnPaint();
static void	OnSetFocus(HWND pOldWnd);
static void	OnKillFocus(HWND pNewWnd);
static void	OnSize(UINT nType, SINT32 cx, SINT32 cy);
static void	OnSysKeyDown(UINT nChar, UINT nFlags);
static void	OnSysKeyUp(UINT nChar, UINT nFlags);

#define USE_NEWGL 0

//==============================================================================

namespace {
	DISPLAY_DEVICE primary_display;
	DISPLAY_DEVICE display = {};

	bool ddpSetDisplay(void) {
		// We only need to mess with the display in fullscreen mode
		if ( window_mode != FULLSCREEN )
			return true;

		// Do a quick check to see if we need to change anything at all
		DEVMODE dm; dm.dmSize = sizeof(dm);
		if ( EnumDisplaySettings(display.DeviceName, ENUM_CURRENT_SETTINGS, &dm) ) {
			if ( dm.dmPelsWidth == WantedWidth && dm.dmPelsHeight == WantedHeight &&
			     dm.dmBitsPerPel == WantedBpp )
				return true;
		} else {
			dbugLog(_FLW, "Failed to get the current settings for the requested display device");
		}

		memset(&dm, 0, sizeof(dm)); dm.dmSize = sizeof(dm);
		dm.dmPelsWidth = WantedWidth;
		dm.dmPelsHeight = WantedHeight;
		dm.dmBitsPerPel = WantedBpp;
		dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;

		LONG result;
		if ( (result = ChangeDisplaySettingsEx(display.DeviceName, &dm, NULL, CDS_FULLSCREEN, NULL)) != DISP_CHANGE_SUCCESSFUL ) {
			dbugLog(_FLW, "Failed to change display settings: %d", result);
			return false;
		}

		return true;
	}

	void ddpResetDisplay(void) {
		ChangeDisplaySettingsEx(display.DeviceName, nullptr, nullptr, 0, nullptr);
	}
}

//==============================================================================

HINSTANCE ddpGetInstance()
{
	return (HINSTANCE)hinst;
}

void ddpSetInstance(HINSTANCE hnewInst)
{
	hinst = hnewInst;
}

LOGICAL ddpInit()
{
	state = true;
	delay = 0;
	dna = false;
	dss = false;
	resetfps = false;
	focus = false;
	capture = 0;
	ddinitfunc = nullptr;
	d3dinitfunc = nullptr;
	ddrenderfunc = nullptr;
	d3drenderfunc = nullptr;
	ddkillfunc = nullptr;
	d3dkillfunc = nullptr;
	mousefunc = nullptr;
	timerfunc = nullptr;
	keyboardfunc = nullptr;

	imeInit();

	Write_Debug("Completed ddpInit");

	return TRUE;
}

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
	MessageBox(nullptr, message.c_str(), _T("Video initialization error"), MB_ICONEXCLAMATION|MB_OK);
}

bool CreateGLContext(HDC dc)
{
	HGLRC Context = wglCreateContext(dc);
	wglMakeCurrent(dc, Context);

	GLenum err = glewInit();

	if(err != GLEW_OK)
	{
		static char str[255];
		strprintf(str, "Error: %s\n", glewGetErrorString(err));
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
            // FIXME: Set to nullptr so NVIDIA will not create a nullptr context.
            myhrc = wglCreateContextAttribsARB(dc, 0, nullptr);//attribs);

        if(myhrc)
        {
            Write_Debug("WGL_ARB_create_context supported.");

            wglMakeCurrent(nullptr, nullptr);
            wglDeleteContext(Context);

            wglMakeCurrent(dc, myhrc);

                newGLContext = true;

            // Make sure we get the data from the correct GL context
	        GLenum err = glewInit();

	        if(err != GLEW_OK)
	        {
		        static char str[255];
		        strprintf(str, "Error: %s\n", glewGetErrorString(err));
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
            wglMakeCurrent(nullptr, nullptr);
            wglDeleteContext(myhrc);

	        myhrc = wglCreateContext(dc);
	        wglMakeCurrent(dc, myhrc);

	        GLenum err = glewInit();

	        if(err != GLEW_OK)
	        {
		        static char str[255];
		        strprintf(str, "Error: %s\n", glewGetErrorString(err));
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

	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &default_framebuffer);

	return true;
}

static bool oglBegin(HWND hwnd) {
	if ( myhrc != NULL )
		return true;

	if ( myhdc == NULL ) {
		if ( (myhdc = GetDC(hwnd)) == NULL ) {
			dbugLog(_FLA, "Could not create device context");
			return false;
		}
	}

	PIXELFORMATDESCRIPTOR pfd = {
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,
		(BYTE)WantedBpp,
		0,0,0,0,0,0,
		0,
		0,
		0,
		0,0,0,0,
		32,
		0,
		0,
		PFD_MAIN_PLANE,
		0,
		0,0,0
	};

	int pixelformat = ChoosePixelFormat(myhdc, &pfd);
	if ( pixelformat == 0 ) {
		dbugLog(_FLA, "Could not find compatible pixel format");
		return false;
	}

	if ( ! SetPixelFormat(myhdc, pixelformat, &pfd) ) {
		dbugLog(_FLA, "Could not request pixel format");
		return false;
	}

	try {
		CreateGLContext(myhdc);
	} catch ( std::exception& ex ) {
		_initializationError(ex.what()); return false;
	} catch ( const char* error ) {
		_initializationError(error); return false;
	} catch ( ... ) {
		_initializationError("[unknown error]"); return false;
	}

	if ( wglSwapIntervalEXT )
		wglSwapIntervalEXT(NO_LOCK_TO_REFRESH);

	return true;
}

static void oglEnd()
{
	if (myhdc && myhrc)
	{
		wglMakeCurrent(myhdc, nullptr);
		wglDeleteContext(myhrc);
		myhrc = nullptr;
	}

	if (myhdc)
	{
		ReleaseDC(myhwnd, myhdc);
		myhdc = nullptr;
	}
}

void ddpKill()
{
    // This function is called recursively with Alt-F4
    static bool alreadykilled = false;

    if(alreadykilled)
    {
        return;
    }

    ClipCursor(NULL);

    alreadykilled = true;

	imeKill();

	if ( window_mode == FULLSCREEN )
		ddpResetDisplay();

	if (d3dkillfunc != nullptr)
		d3dkillfunc();

	if (ddkillfunc != nullptr)
		ddkillfunc();

	rnmainKill();

	strpoolKill();

	oglEnd();

	if (wndAtom)
	{
		UnregisterClass((LPCTSTR)wndAtom, (HINSTANCE)hinst);
		wndAtom = 0;
	}
}

namespace {
	bool RegisterWindowClass(void) {
		if ( wndAtom == 0 ) {
			if ( cursor == NULL )
				cursor = LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));

			WNDCLASS wc;
			memset(&wc, 0, sizeof(wc));

			wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
			wc.lpfnWndProc = WindowProc;
			wc.hInstance = (HINSTANCE)hinst;
			wc.hIcon = LoadIcon((HINSTANCE)hinst, MAKEINTRESOURCE(IDI_WWII_ICON));
			wc.hCursor = cursor;
			wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
			wc.lpszMenuName = NULL;
			wc.lpszClassName = L"WWIIOL_WC";

			if ( (wndAtom = RegisterClass(&wc)) == 0 )
				return false;
		}
		return true;
	}
}


namespace {
	typedef std::vector<HMONITOR> MonitorList;
	BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
		MonitorList* monitors = reinterpret_cast<MonitorList*>(dwData);
		monitors->push_back(hMonitor);
		return TRUE;
	}

	MONITORINFOEX GetDisplayMonitor(void) {
		MonitorList monitors;
		if ( EnumDisplayMonitors(NULL, NULL, &MonitorEnumProc, (LPARAM)&monitors) == 0 || monitors.size() == 0 )
			LogAbort("Could not retrieve any monitor info at all");

		MONITORINFOEX minfo; minfo.cbSize = sizeof(minfo);
		MONITORINFOEX primary_minfo;
		for ( HMONITOR monitor : monitors ) {
			GetMonitorInfo(monitor, &minfo);
			if ( minfo.dwFlags & MONITORINFOF_PRIMARY )
				primary_minfo = minfo;

			if ( wcsstr(minfo.szDevice, display.DeviceName) != nullptr ) {
				return minfo;
			}
		}

		dbugLog(_FLA, "WARNING: Could not retrieve the monitor info for the given display device. Resetting both to primary.");
		display = primary_display;
		return primary_minfo;
	}

	void ReadWindowConfig(void) {
		bool display_found = false;
		DISPLAY_DEVICE tmp_display; tmp_display.cb = sizeof(tmp_display);
		for ( DWORD i = 0 ; EnumDisplayDevices(NULL, i, &tmp_display, 0) != 0 ; ++i ) {
			if ( tmp_display.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE )
				primary_display = tmp_display;

			char buf[128]; WWIIConvert(tmp_display.DeviceName, buf);
			if ( strcmp(WantedDisplayDevice, buf) == 0 ) {
				display = tmp_display;
				display_found = true;
			}
		}

		// TODO: Make sure this works (verified broken after removing a monitor; might be an order thing)
		if ( ! display_found ) {
			dbugLog(_FLW, "Requested display '%s' not found. Resorting to primary display.", WantedDisplayDevice);
			display = primary_display;
		}


		// TODO: Actually make this configurable or get rid of it
		if ( WantedBpp < 32 )
			WantedBpp = 32;


		if ( WantedWindow )
			window_mode = WINDOWED;
		else if ( WantedBorderless )
			// TODO: Make borderless default
			window_mode = BORDERLESS_WINDOW;
		else
			window_mode = FULLSCREEN;

		if ( WantedWidth <= 0 || WantedHeight <= 0 ) {
			if ( window_mode == FULLSCREEN || window_mode == BORDERLESS_WINDOW ) {
				MONITORINFOEX minfo = GetDisplayMonitor();
				const RECT& rect = minfo.rcMonitor;
				WantedWidth = rect.right - rect.left;
				WantedHeight = rect.bottom - rect.top;
			} else {
				WantedWidth = 1024;
				WantedHeight = 768;
			}
		}
	}

	bool SetUpDisplay(void) {
		if ( window_mode != FULLSCREEN )
			return true;
		
		if ( ddpSetDisplay() )
			return true;

		dbugLog(_FLW, "Failed to go fullscreen; trying default resolution.");
		WantedWidth = WantedHeight = 0;
		ReadWindowConfig(); // HACK: Using this to default the window size
		if ( ddpSetDisplay() )
			return true;

		dbugLog(_FLW, "Failed to go fullscreen; trying default display device.");
		display = primary_display;
		WantedWidth = WantedHeight = 0;
		ReadWindowConfig(); // HACK
		if ( ddpSetDisplay() )
			return true;

		dbugLog(_FLW, "Failed to go fullscreen. Resorting to windowed mode.");
		WantedWindow = TRUE;
		WantedWidth = WantedHeight = 0;
		ReadWindowConfig(); // HACK

		return true;
	}
}

// NOTE: Actually modifying an existing window (including code elsewhere) is
// completely untested and is probably horribly broken.
static bool ddpResetWindow(void) {
	ReadWindowConfig();
	if ( ! RegisterWindowClass() ) return false;
	if ( ! SetUpDisplay() ) return false;


	DWORD style;
	DWORD exstyle;
	if ( window_mode == WINDOWED ) {
		style = WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_OVERLAPPEDWINDOW;
		exstyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
	} else {
		style = WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_POPUP;
		exstyle = WS_EX_APPWINDOW;
	}

#ifdef CENTER_WINDOW
	if ( window_mode != FULLSCREEN ) {
		MONITORINFOEX minfo = GetDisplayMonitor();
		const RECT& rect = window_mode == WINDOWED ? minfo.rcWork : minfo.rcMonitor;
		WantedX = ((rect.right - rect.left) - WantedWidth) / 2;
		WantedY = ((rect.bottom - rect.top) - WantedHeight) / 2;
	}
#else
	#error "Currently no mechanism for setting up default X/Y window position."
	// TODO: Verify that the window is actually visible.
	// (e.g., previously configured with multi-monitor setup that changed)
#endif

	// TODO: For borderless window, render to an FBO then render that to the actual window.
	if ( window_mode == FULLSCREEN ) {
		MONITORINFOEX minfo = GetDisplayMonitor();
		renderrect = minfo.rcMonitor;

		WantedX = minfo.rcMonitor.left;
		WantedY = minfo.rcMonitor.bottom;
		WantedWidth = minfo.rcMonitor.right - minfo.rcMonitor.left;
		WantedHeight = minfo.rcMonitor.bottom - minfo.rcMonitor.top;
	} else {
		renderrect.left = WantedX;
		renderrect.right = renderrect.left + WantedWidth;
		renderrect.top = WantedY;
		renderrect.bottom = renderrect.top + WantedHeight;
	}

	RECT wrect = renderrect;
	AdjustWindowRectEx(&wrect, style, FALSE, exstyle);

	// HACK: If the window size is the same as the monitor size, alt-tab, etc. acts weird.
	// I'd implement a proper solution, but the Windows API is garbage and I have no idea
	// why this happens.
	if ( window_mode != WINDOWED ) {
		wrect.bottom += 1;
	}

	bool created_window = false;
	if ( myhwnd == NULL ) {
		created_window = true;
		myhwnd = CreateWindowEx(exstyle, (LPCTSTR)wndAtom, _T(SYS_TITLE), style, wrect.left, wrect.top,
		                        wrect.right-wrect.left, wrect.bottom-wrect.top,
		                        NULL, NULL, (HINSTANCE)hinst, NULL);
		if ( myhwnd == NULL )
			return false;
	} else {
		SetWindowLong(myhwnd, GWL_STYLE, style);
		SetWindowLong(myhwnd, GWL_EXSTYLE, exstyle);
		SetWindowPos(myhwnd, NULL,
		             wrect.left, wrect.top,
		             wrect.right-wrect.left, wrect.bottom-wrect.top,
		             SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
	}

#if DEBUG_WW2
	if (!gDisableMouseClipping)
#endif
		ClipCursor(&renderrect);

	//--------------

	if ( ! created_window )
		return true;

	{
		MSG msg;
		while ( PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE) ) {
			if ( GetMessage(&msg, NULL, 0, 0) ) {
				uiIMEMessageProc(msg);
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}

	if ( ! oglBegin(myhwnd) ) {
		return false;
	}

	// post process globals dependent on GLEW if needed
	cfgPrefsInit();

	// Call external init functions; if present
	if ( ddinitfunc != nullptr ) {
		if ( ! ddinitfunc() ) {
			Write_Debug("DD init failed");
			return false;
		}
	}

	if ( d3dinitfunc != nullptr ) {
		if ( ! d3dinitfunc() ) {
			Write_Debug("D3D init failed");
			return false;
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
		MSG msg;
		while ( PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE) ) {
			if ( GetMessage(&msg, NULL, 0, 0) ) {
				uiIMEMessageProc(msg);
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}

	IMEBase* ime = imeClient();
	if ( ime ) ime->Init();

	if ( ! rnmainInit() ) {
		return false;
	}

	// Lock or don't lock based on player's request in Settings app.
	// VSync causes major slowdown during initialization. Enable/Disable here after init done. (ticket #2329)
	// Forced disable in ddp.cpp before initialization starts.
	gSupportSwapInterval = g_enableVSync ? LOCK_TO_REFRESH : NO_LOCK_TO_REFRESH;
	if ( wglSwapIntervalEXT )
		wglSwapIntervalEXT(gSupportSwapInterval);

	return true;
}

bool ddpCreateWnd(void) {
	return ddpResetWindow();
}

LOGICAL ddpUpdate()
{
	if (!focus)
	{
		std::this_thread::yield();
	}

	if (!IsWindow(myhwnd))
	{
		Write_Debug("Not a window");
		return FALSE;
	}

	if (IsIconic(myhwnd))
	{
		return FALSE;
	}

	UpdateTimes();

	if(cursorvisible)
		SetCursor(cursor);

	// Member draw function
	if (!ddpDraw())
	{
		return FALSE;
	}

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

		resetfps = false;
	}

	if (capture > 0)
	{
		ddcapStart();
		capture -= 1;
	}

	if (!ddpFlip())
	{
		return FALSE;
	}

	// activate or deactivate the ime as needed
	//imeUpdate();

	return TRUE;
}

LOGICAL ddpFlip()
{
	if (myhdc && myhrc)
	{
		// Flip the buffers
		SwapBuffers(myhdc);
	}

	{
	MSG		msg;

		while (PeekMessage(&msg, nullptr, 0, 0, PM_NOREMOVE))
		{
			if (GetMessage(&msg, nullptr, 0, 0))
			{
				uiIMEMessageProc(msg);
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else
			{
				////piExitProgram(_FL, "User exit requested!");
				running = FALSE;
				break;
			}
		}
	}

	return TRUE;
}

LOGICAL ddpDraw()
{
	LOGICAL retval = TRUE;

	if (ddrenderfunc != nullptr)
	{
		if (!ddrenderfunc())
		{
			retval = FALSE;
		}
	}

	if (d3drenderfunc != nullptr)
	{
		if (!d3drenderfunc())
		{
			retval = FALSE;
		}
	}

	return(retval);
}

void ddpDrawNotActive(bool newdna)
{
	dna = newdna;
}

void ddpDisableScreenSaver(bool newdss)
{
	dss = newdss;
}

static void UpdateTimes()
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

SINT32 ddpGetTotalPaints()
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
	resetfps = true;
}

void ddpCapture()
{
	capture += 1;
}

SINT32 ddpGetWidth() {
	return WantedWidth;
}

SINT32 ddpGetHeight() {
	return WantedHeight;
}

LOGICAL ddpSetDDInit(DDWND_DDINITFUNC newfunc)
{
	ddinitfunc = newfunc;
	return TRUE;
}

LOGICAL ddpSetD3DInit(DDWND_D3DINITFUNC newfunc)
{
	d3dinitfunc = newfunc;
	return TRUE;
}

LOGICAL ddpSetDDRender(DDWND_DDRENDERFUNC newfunc)
{
	ddrenderfunc = newfunc;
	return TRUE;
}

LOGICAL ddpSetD3DRender(DDWND_D3DRENDERFUNC newfunc)
{
	d3drenderfunc = newfunc;
	return TRUE;
}

LOGICAL ddpSetDDKill(DDWND_DDKILLFUNC newfunc)
{
	ddkillfunc = newfunc;
	return TRUE;
}

LOGICAL ddpSetD3DKill(DDWND_D3DKILLFUNC newfunc)
{
	d3dkillfunc = newfunc;
	return TRUE;
}

LOGICAL ddpSetMouse(DDWND_MOUSEFUNC newfunc)
{
	mousefunc = newfunc;
	return TRUE;
}

LOGICAL ddpSetTimer(DDWND_TIMERFUNC newfunc)
{
	timerfunc = newfunc;
	return TRUE;
}

LOGICAL ddpSetKeyboard(DDWND_KEYBOARDFUNC newfunc)
{
	keyboardfunc = newfunc;
	return TRUE;
}


GLuint ddpDefaultFramebuffer(void) {
	return default_framebuffer;
}


// Detached thread
UINT32 ddpEngine(LPVOID dummy)
{
LOGICAL		retval, *running;

	running = (LOGICAL*)dummy;

	WantedHdw = FALSE;	//// TEMPORARY (until single pass (i.e. hardware driven) multi-texturing is working)

	if (!ddpInit())
	{
		return FALSE;
	}

	ddpDrawNotActive(false);
	ddpDisableScreenSaver(true);

	ddpSetDDInit(nullptr);//oglInit);
	ddpSetDDRender(nullptr);//rnlogoRender);
	ddpSetDDKill(nullptr);//oglKill);

	ddpSetD3DInit(piInit);
	ddpSetD3DRender(gmlpMainUpdate);
	ddpSetD3DKill(piKill);

	if (!ddpCreateWnd())
	{
		ddpKill();
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

	if (myhwnd != nullptr)
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
		if (mousefunc != nullptr)
		{
			mousefunc(umsg, wParam, lParam);
		}
	}
	else
	switch (umsg)
	{
	case WM_CREATE:
	{
		// TODO: Save position/size info?
		myhwnd = hwnd;
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
		myhwnd = nullptr;
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
#if DEBUG_WW2
		if (!gDisableMouseClipping)
#endif
			ClipCursor(&renderrect);
		break;
	}
	case WM_KILLFOCUS:
	{
		// HACK: It seems like we don't update properly when minimized,
		// which can cause problems when connected to a server.
		//ShowWindow(myhwnd, SW_MINIMIZE);
		OnKillFocus((HWND) wParam);
		ClipCursor(nullptr);
		break;
	}
	case WM_NCACTIVATE:
	{
		OnNcActivate((LOGICAL) wParam);
		break;
	}
	case WM_MOVE:
	{
		if (IsWindowVisible(hwnd))
		{
			RECT rect;
			GetClientRect(hwnd, &rect);
			ClientToScreen(hwnd, (LPPOINT)&rect);
			// We save this stuff to the registry since it isn't too important.
			// TODO: Make sure our window is still on the screen.
			DWORD xy = MAKELONG(rect.left, rect.top);
			sysregSetHKeyDword(HKEY_CURRENT_USER, REG_KEY_MAIN, REG_MAIN_LOC, xy);
			DWORD wh = MAKELONG(rect.right, rect.bottom);
			sysregSetHKeyDword(HKEY_CURRENT_USER, REG_KEY_MAIN, REG_MAIN_SIZE, wh);
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
				return FALSE;
			}
			break;
		case SC_SCREENSAVE:
		{
			if (dss)
			{
				return FALSE;
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
				state = true;
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
			state = false;

		dipUnacquire();
		Write_Debug("WM_ENTERMENULOOP");
		break;
	}
	case WM_EXITMENULOOP:
	{
		if (!state)
			state = true;

		dipAcquire();
		Write_Debug("WM_EXITMENULOOP");
		break;
	}
	case WM_ENTERSIZEMOVE:
	{
		if (state)
			state = false;

		dipUnacquire();
		Write_Debug("WM_ENTERSIZEMOVE");
		break;
	}
	case WM_EXITSIZEMOVE:
	{
		if ( ! state )
			state = true;
		dipAcquire();
		Write_Debug("WM_EXITSIZEMOVE");
		break;
	}
	case WM_DISPLAYCHANGE:
	{
		WantedBpp = wParam;
		if ( !state )
			state = true;

		Write_Debug("WM_DISPLAYCHANGE");
		break;
	}
	case WM_TIMER:
	{
		if (timerfunc != nullptr)
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

	return TRUE;
}

static void _OnPaint()
{
	Write_Debug("OnPaint");
}

static void OnSetFocus(HWND pOldWnd)
{
	if ( !dna ) {
		if ( !state )
			state = true;
	}

	focus = true;

	ddpResetWindow();
	dipAcquire();

	Write_Debug("OnSetFocus");
}

static void OnKillFocus(HWND pNewWnd)
{
	if ( !dna ) {
		if ( state )
			state = false;
	}

	focus = false;

	ddpResetDisplay();
	dipUnacquire();

	Write_Debug("OnKillFocus");
}

static void OnSize(UINT nType, int cx, int cy) {
	Write_Debug("OnSize: x=%d, y=%d", cx, cy);

	if ( cx <= 0 || cy <= 0 )
		return;

	if ( window_mode == WINDOWED ) {
		// TODO: Force aspect ratio
		if ( ddpGetWidth() != cx || ddpGetHeight() != cy ) {
			oglResize(cx, cy);
		}

		// Save the new config
		if ( (WantedWidth != cx || WantedHeight != cy) && (cx != 0 && cy != 0) ) {
			WantedWidth = cx;
			WantedWidth = cy;
		}
	} else {
		// TODO: Fullscreen/borderless
	}
}

static LOGICAL OnNcActivate(LOGICAL bActive)
{
	if ( bActive ) {
		if ( !state )
			state = true;
	} else {
		if ( !dna ) {
			if ( state )
				state = false;
		}
	}

	if (bActive)
	{
		HWND insertPos = HWND_TOPMOST;
#if DEBUG_WW2
		if (gDisableGameTopmost) {
			insertPos = HWND_TOP;
		}
#endif
		SetWindowPos(myhwnd, insertPos, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
		dipAcquire();
	}
	else
	{
		SetWindowPos(myhwnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
		dipUnacquire();
	}

	Write_Debug("OnNcActivate %s", state ? "TRUE" : "FALSE");

	return(0);
}

static void OnSysKeyDown(UINT nChar, UINT nFlags)
{
	if ( !dna ) {
		if ( state )
			state = false;
	}

	Write_Debug("OnSysKeyDown: 0x%x", nChar);
}

static void OnSysKeyUp(UINT nChar, UINT nFlags)
{
	if ( !dna ) {
		if ( !state )
			state = true;
	}

	Write_Debug("OnSysKeyUp: 0x%x", nChar);
}

DWORD GetMillisecondTick()
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

static void capSave(UCHAR* const bptr, UINT32 width, UINT32 height, const fullpath_t* const bname)
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
		if (nullptr != bname)
		{
			piBITMAPFILEHEADER	bmfh;

			bmfh.bfType = (UINT16)('B' + ('M' << 8));
			bmfh.bfSize = sizeof(bmfh) + dib->bmiHeader.biSize + twidth * height;
			bmfh.bfReserved1 = 0;
			bmfh.bfReserved2 = 0;
			bmfh.bfOffBits = sizeof(bmfh) + dib->bmiHeader.biSize;

			size = dib->bmiHeader.biSize + dib->bmiHeader.biSizeImage;

#if platOS != OS_WIN32
			FILE* cf = fopen(bname, "wb+");
#else
			FILE* cf = _wfopen(bname, L"wb+");
#endif
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

void ddpSaveImgAsBitmap(unsigned char* img, int w, int h, wchar_t* filename)
{
    capSave(img, w, h, filename);
}

static void glScreenCapture(void) {
	// TODO: Switch file stuff to fio
	// TODO: Switch to writeBMPImage()
#if platOS == OS_WIN32 // HACK
	#define topathstr std::to_wstring
	#define checkfile _waccess

	std::basic_string<fullpath_t> base_filename(UserDataPath);
#else
	#define topathstr std::to_string
	#define checkfile _access

	std::basic_string<fullpath_t> base_filename;
#endif

	base_filename += PATHLit("SShot");

	std::basic_string<fullpath_t> filename;
	int shotCount = 1;
	do {
		if ( shotCount > 1000 )
			return;
		filename = base_filename + topathstr(shotCount++) + PATHLit(".bmp");
	} while ( checkfile(filename.c_str(), 0) == 0 );

	UCHAR* bitmap = (UCHAR*)memAlloc(ddpGetWidth() * ddpGetHeight()* 3);
	if ( bitmap == nullptr )
		return;
	glReadPixels(0, 0, ddpGetWidth(), ddpGetHeight(), GL_RGB, GL_UNSIGNED_BYTE, bitmap);
	// Cap save will release the bitmap memory.
	capSave(bitmap, ddpGetWidth(), ddpGetHeight(), filename.c_str());

	#undef topathstr
	#undef checkfile
}

LOGICAL ddcapStart()
{
	glScreenCapture();
	return TRUE;
}


void ddpShowCursor()
{
	int count = ShowCursor(TRUE);
	while(count <= 1)
		count = ShowCursor(TRUE);
	SetCursor(cursor);
	cursorvisible = 1;
}

void ddpHideCursor()
{
#if DEBUG_WW2
	if (gDisableMouseHiding) {
		return;
	}
#endif
	int count = ShowCursor(FALSE);
	while(count > -1)
		count = ShowCursor(FALSE);
	cursorvisible = 0;
}

void ddpPumpMessage()
{
    static MSG msg;

    while( PeekMessage( &msg, nullptr, 0, 0, PM_NOREMOVE ) )
    {
        if( !GetMessage( &msg, nullptr, 0, 0 ) )
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
