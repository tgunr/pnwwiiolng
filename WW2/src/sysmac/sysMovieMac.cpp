#include <syscfg.h> // Path is project specific.
#include <Quicktime/Quicktime.h>
#include <OpenGL/glu.h>
#include <string>

#include "sysMovie.h"

/************************************************************************************************
	sysMovie
*************************************************************************************************/

struct MacAVI
{
  Movie	m;
  SInt16	refNum;
  SInt32	trackIndex;
  UInt32	frameCount;
  Media	media;	// the media of the video track. we don't own it.
  SampleDescriptionHandle	sdh;
  ImageSequence	seq;
  Handle		h;
  TimeValue	time;
  Ptr		buffer;	// we don't own it.
};
typedef MacAVI* MacAVIRef;

struct sysMovie::platimp
{
	platimp();
	~platimp();

	bool playing;
	int lasttime;
	GLuint texid;
	MacAVI avi;
	GWorldPtr gworld;
	Ptr buffer;
	int lastframe;
	int millisPerFrame;
	int curframe;
};

static void 	MacInitAVI();
static void 	MacShutDownAVI();
static bool 	MacOpenAVI(MacAVI* p, const char* filename, GWorldPtr gw, Ptr buffer);
static SInt32	MacGetFrameCount(MacAVIRef inRef);
static void		MacDisposeAVI(MacAVIRef inRef);
static Ptr		MacGrabAVIFrame(MacAVIRef inRef, int frame);
static SInt32	MacGetDurationInMilli(MacAVIRef inRef);

static bool _inited;

/************************************************************************************************
	sysMovie
*************************************************************************************************/

sysMovie::sysMovie()
{
	_platimp = new platimp;

	MacInitAVI();
}

sysMovie::~sysMovie()
{
	delete _platimp;
}

bool sysMovie::Load(const char* filename)
{
	std::string path = "movies/";
	path += filename;
	path += ".mov";

	if(_platimp->avi.m) return false;

	bool loaded = MacOpenAVI(&_platimp->avi, path.c_str(), _platimp->gworld, _platimp->buffer);
	if(loaded)
	{
	  	glGenTextures(1, &_platimp->texid);
	   	_platimp->lastframe = MacGetFrameCount(&_platimp->avi) - 1;
		_platimp->millisPerFrame = MacGetDurationInMilli(&_platimp->avi) / _platimp->lastframe;
    }

	return loaded;
}

void sysMovie::Start(int ms)
{
	_platimp->lasttime = ms;
	_platimp->playing = true;
}

void sysMovie::Stop()
{
	_platimp->playing = false;
	_platimp->curframe = 0;
	_platimp->avi.time = 0;
	MacGrabAVIFrame(&_platimp->avi, _platimp->curframe);
}

void sysMovie::Pause()
{
 	_platimp->playing = false;
}

void sysMovie::Enable(bool enable)
{
}

void sysMovie::UpdateFrame(int ms)
{
	if(_platimp->texid == 0) return;

	if(_platimp->playing)
	{
		int frame = _platimp->curframe + (ms - _platimp->lasttime) /  _platimp->millisPerFrame;
		if(frame > _platimp->lastframe)
		{
			frame = _platimp->lastframe;
			_platimp->playing = false;
		}
		if(frame > _platimp->curframe)
		{
			_platimp->curframe = frame;
			_platimp->lasttime = ms;
			MacGrabAVIFrame(&_platimp->avi, _platimp->curframe);
		}
	}

  	Ptr buf = _platimp->avi.buffer;
 	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, _platimp->texid);
  	gluBuild2DMipmaps(GL_TEXTURE_2D,
                    GL_RGB,
                    256,
                    256,
                    GL_RGBA,
                    GL_UNSIGNED_BYTE,
                    buf);
}

void sysMovie::DrawFrame(float left, float bottom, float right, float top)
{
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, _platimp->texid);

	glBegin(GL_QUADS);
	glTexCoord2f(0,0);
	glVertex2f(left,bottom);
	glTexCoord2f(1,0);
	glVertex2f(right,bottom);
	glTexCoord2f(1,1);
	glVertex2f(right,top);
	glTexCoord2f(0,1);
	glVertex2f(left,top);
	glEnd();
}

bool sysMovie::Playing()
{
	return _platimp->playing;
}

void MacInitAVI()
{
	if(!_inited)
	{
		EnterMovies();
		_inited = true;
	}
}

void MacShutDownAVI()
{
}

static bool MacOpenAVI(MacAVI* p, const char* filename, GWorldPtr gw, Ptr buffer)
{
  Movie mov = NULL;
  SInt16	refNum;
  SInt16	id = movieInDataForkResID;
  FSSpec	fs;
	OSErr err;
	ProcessSerialNumber psn;
	FSRef ref;
	char path[4192];

	GetCurrentProcess(&psn);
	GetProcessBundleLocation(&psn, &ref);
	FSRefMakePath(&ref,(unsigned char*)path,4000);
	strcat(path,"/AppData/");
	strcat(path,filename);
	err = FSPathMakeRef ((unsigned char*)path,&ref,NULL);
	if ( err!=noErr ) return false;
	err = FSGetCatalogInfo (&ref,kFSCatInfoNone, NULL, NULL,  &fs, NULL);
	if ( err!=noErr ) return false;

  err = OpenMovieFile(&fs, &refNum, fsRdPerm);
  if ( err!=noErr ) return false;
  err = NewMovieFromFile(&mov, refNum, &id, NULL, newMovieActive, NULL);
  if ( err!=noErr ) return false;

  p->m = mov;
  p->refNum = refNum;
  long i, iMax;
  iMax = GetMovieTrackCount(p->m);
  for(i = 1; i <= iMax; ++i)
  {
    Track t;
    Fixed widthF, heightF;
    t = GetMovieIndTrack(p->m, i);
    GetTrackDimensions(t, &widthF, &heightF);
    if(0 != widthF && 0 != heightF)
    {
      p->buffer = buffer;
      p->h = NewHandle(0);
      p->trackIndex = i;
      p->media = GetTrackMedia(t);
      p->sdh = (SampleDescriptionHandle)(NewHandle(0));
      GetMediaSampleDescription(p->media, 0, p->sdh);
      SetMovieGWorld(p->m, gw, GetGWorldDevice(gw));

      Rect portRect;

      GetPortBounds(gw, &portRect);

      SetMovieBox(p->m, &portRect);
      p->frameCount = MacGetFrameCount(p);
      GoToBeginningOfMovie(p->m);
      MacGrabAVIFrame(p, 0);
      break;
    }
  }

  return true;
}

SInt32 MacGetFrameCount(MacAVIRef inRef)
{
  SInt32		frameCount = 0;
  OSType		whichMediaType = VIDEO_TYPE;
  short		flags = nextTimeMediaSample + nextTimeEdgeOK;
  TimeValue	duration;
  TimeValue	theTime = 0;

  while (theTime >= 0)
  {
    frameCount++;
    GetMovieNextInterestingTime(inRef->m,
                                flags,
                                1,
                                &whichMediaType,
                                theTime,
                                0,
                                &theTime,
                                &duration);

    //	after the first interesting time, don't include the time we
    //  are currently at.

    flags = nextTimeMediaSample;
  } // while
  return frameCount;
}

SInt32 MacGetDurationInMilli(MacAVIRef inRef)
{
  TimeScale ts = GetMovieTimeScale(inRef->m);
  return (GetMovieDuration(inRef->m)*1000)/ts;
}

void MacDisposeAVI(MacAVIRef inRef)
{
  if(NULL == inRef)
  {
    return;
  }
  if(0 != inRef->seq)
  {
    CDSequenceEnd(inRef->seq);
    inRef->seq = 0;
  }
  if(NULL != inRef->h)
  {
    DisposeHandle(inRef->h);
    inRef->h = NULL;
  }
  if(NULL != inRef->sdh)
  {
    DisposeHandle((Handle)(inRef->sdh));
    inRef->sdh = NULL;
  }
  if(NULL != inRef->m)
  {
    DisposeMovie(inRef->m);
    inRef->m = NULL;
  }
  if(0 != inRef->refNum)
  {
    CloseMovieFile(inRef->refNum);
    inRef->refNum = 0;
  }
}


// GWorlds have three problems with respect to GL Textures:
// 1.) they are upside down.
//		(this could be fixed in the u v texture coordinate space)
// 2.) the components of each pixel are in a different order: ARGB on Mac, RGBA on openGL
//		(this could be fixed by pointing at the &(**pmh).baseAddr[1])
// 3.) there is arbitrary buffering at the end of each scan line.
//		(this could be fixed with QTNewGWorldFromPtr(), in Quicktime 4.)
// this copies from the GWorld to the buffer, fixing all three problems.
// Note: the three problems could be fixed by other means at zero cost, as above
static void GWorldToBuffer(GWorldPtr gw, Ptr buffer)
{
  Ptr src, dest, srcbase;
  dest = buffer;
  PixMapHandle pmh = GetGWorldPixMap(gw);
  srcbase = GetPixBaseAddr(pmh);
  SInt32 x, y;
  SInt32 rowBytes = 0x3FFF & (**pmh).rowBytes;
  SInt32 xMax = (**pmh).bounds.right - (**pmh).bounds.left;
  SInt32 yMax = (**pmh).bounds.bottom - (**pmh).bounds.top;
  srcbase += (yMax-1)*rowBytes;
  for(y = 0; y < yMax; ++y, srcbase -= rowBytes)
  {
    src = srcbase;
    for(x = 0; x < xMax; ++x, dest += 4, src += 4)
    {
      dest[0] = src[1];
      dest[1] = src[2];
      dest[2] = src[3];
    }
  }
}

// grab the frame and retun it in the buffer suitable for passing to glTexImage2D
Ptr MacGrabAVIFrame(MacAVIRef inRef, int frame)
{
  TimeValue duration;
  // get the next frame of the source movie
  short 	flags = nextTimeMediaSample;
  OSType	whichMediaType = VIDEO_TYPE;

  // if this is the first frame, include the frame we are currently on
  if(frame == 0)
  {
    flags |= nextTimeEdgeOK;
  }
  // skip to the next interesting time and get the duration for that frame
  GetMovieNextInterestingTime(inRef->m,
                              flags,
                              1,
                              &whichMediaType,
                              inRef->time,
                              0,
                              &inRef->time,
                              &duration);
  if(-1 == inRef->time)
  {
    inRef->time = 0;
  }
  // set the time for the frame and give time to the movie toolbox
  SetMovieTimeValue(inRef->m, inRef->time);
  MoviesTask(inRef->m, 0);	// this grabs the next frame
  GWorldPtr 	gw;
  GDHandle	gd;
  GetMovieGWorld(inRef->m, &gw, &gd);
  GWorldToBuffer(gw, inRef->buffer);
  return inRef->buffer;
}

sysMovie::platimp::platimp(): playing(false), lasttime(0), texid(0), gworld(0), buffer(0),
								lastframe(0),millisPerFrame(0),curframe(0)
{
 	memset(&avi, 0x0, sizeof(avi));

	Rect r = {0, 0, 256, 256};
	buffer = NewPtr(256*256*4);
	NewGWorld(&gworld, 32, &r, NULL, NULL, 0);
	LockPixels(GetGWorldPixMap(gworld));
}

sysMovie::platimp::~platimp()
{
	MacDisposeAVI(&avi);
	DisposePtr(buffer);
	DisposeGWorld(gworld);
}
