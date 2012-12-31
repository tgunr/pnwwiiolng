#include "syscfg.h"

#include <string>

#include "sys/sysMovie.h"
#include "pi/pi.h"
#include "include/vars.h"

#include <strmif.h>

#include <vfw.h>
#include <vfwmsgs.h>

#include <uuids.h>
#include <wmsdkidl.h>

#include <control.h>
#include <evcode.h>

#define SAFE_RELEASE(x) { if (x) x->Release(); x = NULL; }

#define NUMBEROFBUFFERS 3
#define SOUNDBUFFERTIME 100

#define WANTED_WIDTH    512
#define WANTED_HEIGHT   512

#define WM_GRAPHNOTIFY  WM_USER+20

extern HWND	myhwnd;

static struct
{
  WAVEFORMATEX waveformat;
  char extra[64];
} audioformat;

struct sysMovie::platimp
{
	platimp();
	~platimp();

	bool Load(const char*);

	IGraphBuilder* pGB;
	IMediaControl* pMC;
	IMediaEventEx* pME;
	IVideoWindow*  pVW;
	IBasicAudio*   pBA;
	IBasicVideo*   pBV;
	IMediaSeeking* pMS;

	// pos: needed to store video time line
	LONGLONG pos;
	bool loaded;
	// enabled: needed to store movie visability state
	bool enabled;
	bool playing;
};

sysMovie::sysMovie()
{
	_platimp = new platimp;
	CoInitialize(NULL);
}

sysMovie::~sysMovie()
{
	 delete _platimp;
	 CoUninitialize();
}

bool sysMovie::Load(const char* filename)
{
	std::string path = "data\\movies\\";
	path += filename;
	path += ".wmv";

	return _platimp->Load(path.c_str());
}

bool sysMovie::platimp::Load(const char* filename)
{
	return false;
}

void sysMovie::Start(int ms)
{
	if (_platimp->loaded)
	{
		_platimp->playing = true;
		_platimp->pMC->Run();
	}
}

void sysMovie::Pause()
{
	if (_platimp->loaded)
	{
		_platimp->playing = false;
		_platimp->pMC->Pause();
	}
}

void sysMovie::Stop()
{
	if (_platimp->loaded)
	{
		_platimp->pos = 0;
		_platimp->playing = false;
		_platimp->pMC->Stop();
		_platimp->pMS->SetPositions(&_platimp->pos, AM_SEEKING_AbsolutePositioning,
								NULL, AM_SEEKING_NoPositioning);
		_platimp->pMC->Pause();
	}
}

void sysMovie::UpdateFrame(int ms)
{
}

void sysMovie::DrawFrame(float left, float bottom, float right, float top)
{
	if (_platimp->loaded)
	_platimp->pVW->SetWindowPosition((long) left, (long)(WantedHeight-top), (long)(right-left), (long)(top-bottom));
}

void sysMovie::Enable(bool toggle)
{
	if (_platimp->loaded)
	{
		_platimp->pVW->put_Visible(toggle);
		_platimp->enabled = toggle;
		if (toggle)
		{
			_platimp->pMC->Stop();
			_platimp->pMS->SetPositions(&_platimp->pos, AM_SEEKING_AbsolutePositioning,
								NULL, AM_SEEKING_NoPositioning);
			_platimp->pMC->Pause();
		}
		else
		{
			_platimp->pMC->Pause();
			_platimp->pMS->GetPositions(&_platimp->pos, NULL);
		}
	}
}

bool sysMovie::Playing()
{
	long evCode, param1, param2;

	_platimp->pME->GetEvent(&evCode, &param1, &param2, 0);
	if(evCode == EC_COMPLETE)
		_platimp->playing = false;
	_platimp->pME->FreeEventParams(evCode, param1, param2);

	return _platimp->playing;
}

sysMovie::platimp::~platimp()
{
	HRESULT hr;

	// Disable video notifications - if needed
	if (pME)
		hr = pME->SetNotifyWindow((OAHWND)NULL, 0, 0);

	// Release all media handles
	SAFE_RELEASE(pME);
	SAFE_RELEASE(pMS);
	SAFE_RELEASE(pMC);
	SAFE_RELEASE(pBA);
	SAFE_RELEASE(pBV);
	SAFE_RELEASE(pVW);
	SAFE_RELEASE(pGB);
 }

sysMovie::platimp::platimp() : pGB(0), pMC(0), pME(0), pVW(0), pBA(0), pBV(0), pMS(0), pos(0), loaded(false), playing(false), enabled(true)
{
}
