#include <syscfg.h> // Path is project specific.
#include <stdarg.h>

#include "pi/pi.h"
#include "include/vars.h"
#include "rn/rnlogo.h"
#include "ui/uiMain.h"
#include "gmlp/gmlpmain.h"
#include "dimain.h"
#include "imap/iac.h"
#include "versionNo.h"
#include "cfg/cfgPrefs.h"
#include "ui/uiMovieView.h"
#include "fio/fiod.h"
#include "fio/fio.h"
#include "ogl/ogl.h"
#include "include/vars.h"
#include "ogl/oglGlobalStates.h"
#include "phui/Message.h"
#include "teul/teulBasic.h"

/************************************************************************************************
	rnlogo
*************************************************************************************************/

#if RELEASE_CLIENT != 0
#define DISPLAY_LOGO
#endif

extern LOGICAL running;
SINT32	LoadingScreen;


#if (platOS == OS_WIN32) && (RELEASE_CLIENT != 0)
extern HWND	myhwnd;
#endif

static phui::View* _loadingscreen;
static phui::View* _progress;
static phui::View* _status;

static SINT32 _startloadtime;
static SINT32 _totalloadtime = 10000*60;

extern void UpdateGameClocks(void);
static bool _keyEntered();
static bool _isWidescreen();

#ifdef DISPLAY_LOGO
static void _displayLogo();
#endif

#if(0)
static void _playStartupMovie(const char*);
#endif

static const char* _wideSplashScreen = "data/screens/splashscreen69.bmp";
static const char* _stdSplashScreen = "data/screens/splashscreen43.bmp";
static const char* _wideLoadingScreen = "data/screens/loadingscreen69.bmp";
static const char* _stdLoadingScreen = "data/screens/loadingscreen43.bmp";

#ifdef TESTMAC
std::string currentLoadString;
#endif

/************************************************************************************************
	rnlogoInit
*************************************************************************************************/

LOGICAL rnlogoInit(void)
{
	_keyEntered(); // eat any events that may be in the queue

#ifdef DISPLAY_LOGO
	_displayLogo();
#endif

	UpdateGameClocks();
	_startloadtime = frametime;

	LoadingScreen = 1;

	if(_isWidescreen())
	{
		_loadingscreen = uiLoadView("loadingscreen69");
	}
	else
	{
		_loadingscreen = uiLoadView("loadingscreen43");
	}

	if(!_loadingscreen) return FALSE;

	_status = _loadingscreen->FindView("status");
	_progress = _loadingscreen->FindView("progress");

	phui::View* version = _loadingscreen->FindView("version");
	if(version)
    {
        std::string str = STRVER;

		// For debug and beta releases, append the commit info.
#if defined(DEBUG_WW2) || defined(BETA_RELEASE)
		str += " : ";
		str += WWII::gitCommitStr;
#endif

#if(0)
        if(ogl::UseDeprecatedPath())
        {
            str += " - deprecated path";
        }
        else
        {
            str += " - new path";
        }
#endif

        version->SetValue(str.c_str());
    }

	gAppPrefs->FindLong(CFG_APP_LOADTIME, (long*)&_totalloadtime);

	return TRUE;
}

/************************************************************************************************
	rnlogoRender
*************************************************************************************************/

LOGICAL rnlogoRender(void)
{
	piSetDDRender(nullptr);

	piSetD3DRender(gmlpMainUpdate);
	piResetFPS();

    LoadingScreen = 0;

	return TRUE;
}

/************************************************************************************************
	rnlogoKill
*************************************************************************************************/

void rnlogoKill(void)
{

#if (platOS == OS_WIN32) && (RELEASE_CLIENT != 0)
	// This is used to force the foreground, active and focus window to be ours.
	// Some players were changing active windows during game loadup and causing
	// the game to lose active inputs. We use this to force our app to be the
	// app that gets input and insure this is the case when the UI is displayed.
	SetForegroundWindow(myhwnd);
	SetActiveWindow(myhwnd);
	SetFocus(myhwnd);
#endif

    LoadingScreen = 0;
}

/************************************************************************************************
	rnUpdateLoadingScreen
*************************************************************************************************/

void rnUpdateLoadingScreen ()
{
  static SINT32 lastUpTime = 0;

	if(Online)
		teulUpdateTransport();

	if(_progress)
	{
		UpdateGameClocks();
		_progress->SetValue((frametime - _startloadtime) / (float)_totalloadtime);

    if ( (frametime - lastUpTime ) > 30 )
    {
      lastUpTime = frametime;
      uiUpdate();
      uiDraw();
      piFlip();
    }
	}

	_keyEntered(); // allow OS event loop to process

	// This block of code allows us to terminate the program during the loading screen display
	// Also, during Debugging, Alt+Tab works correctly (if not a full screen app)
	if (!running)
		piExitProgram(__FILE__, __LINE__, "Exit requested during startup");
}

/************************************************************************************************
	rnUpdateLoadingEnd
*************************************************************************************************/

void rnUpdateLoadingEnd()
{
	if(_loadingscreen)
	{
		uiViewport()->RemoveChild(_loadingscreen);
		delete _loadingscreen;
		_loadingscreen = _progress = _status = 0;
		if(_isWidescreen())
		{
			uiFreeTexture(_wideLoadingScreen);
		}
		else
		{
			uiFreeTexture(_stdLoadingScreen);
		}
	}

	UpdateGameClocks();
	_totalloadtime = frametime - _startloadtime;
	gAppPrefs->ReplaceOrAddLong(CFG_APP_LOADTIME, _totalloadtime);
}

#ifdef TESTMAC
//void memtestnote();
#endif

/************************************************************************************************
	rnlogoUpdateStatus
*************************************************************************************************/

void rnlogoUpdateStatus(const char* str, ...)
{
	if(!_loadingscreen) return;

	if(_status)
	{
		va_list marker;
		va_start(marker, str);
		char buf[128];
		vsnprintf(buf, 127, str, marker);
		va_end(marker);
#ifdef TESTMAC
//    currentLoadString = buf;
//    memtestnote();
#endif
		_status->SetValue((uiLocString(buf)).c_str());
	}
	rnUpdateLoadingScreen();
}

static bool _keyEntered()
{
	imap_queue_entry* entries;
	UINT32 numentries;
	dimainGetInputs(&entries, &numentries);
	for(UINT32 i=0; i < numentries; i++)
	{
		if(IAC_IS_BUTTON(entries[i].iac) && entries[i].state)
			return true;
	}
	return false;
}

static bool _isWidescreen()
{
	// Determine whether user has selected a widescreen aspect ratio
	double aspectRatio = (double)WantedWidth/(double)WantedHeight;
	if(aspectRatio >= 1.6)
	{
		return true;
	}
	else
	{
		return false;
	}
}

#ifdef DISPLAY_LOGO
static void _displayLogo()
{
	UpdateGameClocks();

	// we dont have startup movies anymore
	//_playStartupMovie("start0");
	//_playStartupMovie("start");

}
#endif

void rnlogoSplash()
{
    extern TEXTURE* textures;

	// save which mode we are in
	SINT32 fioSwitch = fioQuery();

	// force directory lookups
	fioSet(ISDIRECTORY);

	const char* texFile;

	if(_isWidescreen()) texFile = _wideSplashScreen;
	else texFile = _stdSplashScreen;

	int texid = uiGetTexture(texFile);

	// reset what mode we were in
	fioSet(fioSwitch);

	if(texid == -1) return;

    oglSetWorldMatrix(&IdentityMatrix);
    oglSetViewMatrix(&IdentityMatrix);
    oglSetProjectionMatrix(&IdentityMatrix);
    oglSetTextureMatrix(&IdentityMatrix);
    oglSetActiveTexture(0, texid);

	// screen is represented as -1.0f to 1.0f, so we need a third of 2.0f across for the width
	float width = (((WantedWidth / WantedHeight) >= 3.0f)) ? 2.0f * .3f: 1.0f;

    piLVERTEX v[4];
    float u1, v1, u2, v2;

    u1 = 0.0f;
    v1 = 0.0f;
    u2 = textures[texid].widthfactor[0];
    v2 = textures[texid].heightfactor[0];

    float xx = width;
    float yy = 1.0f;

	v[0].x = -xx;
	v[0].y =  yy;
	v[0].z = 0;
	v[0].color = colorWHITEALPHA;
	v[0].specular = colorALPHA100;
	v[0].tu = u1;
	v[0].tv = v2;

	v[1].x = -xx;
	v[1].y = -yy;
	v[1].z = 0;
	v[1].color = colorWHITEALPHA;
	v[1].specular = colorALPHA100;
	v[1].tu = u1;
	v[1].tv = v1;

	v[2].x = xx;
	v[2].y = -yy;
	v[2].z = 0;
	v[2].color = colorWHITEALPHA;
	v[2].specular = colorALPHA100;
	v[2].tu = u2;
	v[2].tv = v1;

	v[3].x = xx;
	v[3].y = yy;
	v[3].z = 0;
	v[3].color = colorWHITEALPHA;
	v[3].specular = colorALPHA100;
	v[3].tu = u2;
	v[3].tv = v2;

    piINDICES indices[2];

    indices[0].v1 = 0;
    indices[0].v2 = 1;
    indices[0].v3 = 3;
    indices[1].v1 = 1;
    indices[1].v2 = 2;
    indices[1].v3 = 3;

    oglRenderIndexScene(oglPT_TRIANGLELIST,
                        oglVT_LVERTEX,
                        v,
                        4,
                        (unsigned short*)indices,
                        6);

	piFlip();
}
