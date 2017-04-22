#include <OpenGL/OpenGL.h>

#include "ddp.h"
#include "gmlp/gmlpmain.h"
#include "rn/rnlogo.h"
#include "fio/fio.h"
#include "fio/fioa.h"
#include "fio/fiod.h"
#include "sysmain.h"
#include "mn/mngopt.h"
#include "rn/rnmain.h"
#include "mem/mem.h"
#include "dimain.h"
#include <unistd.h>
#include <pthread.h>

#if USE_OSG
#include "osg/osgGame.h"
#endif

//const SINT8 Copyright[] = "Copyright 2016 Playnet, Inc. All Rights Reserved";

void sysmainAddArchives();
static int sysmainInitApp();

int running = 0;
static int _argc;
static const char** _argv;

void sysmainGetCommandLineArgs(int* argc, char*** argv)
{
	*argc = _argc;
	*argv = const_cast<char **>(_argv);
}

void sysmainCommandLine(int argc, const char** argv)
{
	SetCommandLine(argc, argv);
	_argc = argc;
	_argv = argv;

	mngoptParse((char *)"");
}

static size_t frames = 0;
static pthread_t mainthread = NULL;

int sysmain()
{
	unlink(Fio::LogFileName()) ;

	running = 1;

	if(sysmainInitApp() != 0)
	{
		printf("sysmainInitApp: App was not able to initialize!\n");
		exit(0);
	}

	if(!piInit())
	{
		printf("piInit: App was not able to initialize!\n");
		exit(0);
	}

#if USE_OSG
	if(!osgGameInit())
	{
		printf("osgGameInit: App was not able to initialize!\n");
		exit(0);
	}
#else
	if(!rnmainGetOnlineSetting())
	{
		printf("rnmainGetOnlineSetting: App was not able to initialize!\n");
		exit(0);
	}

	if(!rnmainInit())
	{
		printf("rnmainInit: App was not able to initialize!\n");
		exit(0);
	}
#endif

    mainthread = pthread_self(); // used for debugging

	while(running)
    {
        frames++;
		ddpEngine(NULL);
#ifdef DEBUG_WW2
        if (frames % 100 == 0) {
            Write_Debug("Current FPS: %.1f", 1000.0 / gFPS );
        }
#endif
    }

	sysmainKillApp();

	return 0;
}

int sysmainInitApp()
{
	IdentityMatrix.m[0][0] = IdentityMatrix.m[1][1] = IdentityMatrix.m[2][2] = IdentityMatrix.m[3][3] = 1.0;
	chAppDir();

	printf("WW2OL Started\n"); fflush(stdout);
#ifdef BETA_RELEASE
	{
        char buff[PATH_MAX];
        getcwd( buff, PATH_MAX );
        std::string cwd( buff );
        cwd.append("/../../gizmo.key");
        FILE* f = std::fopen(cwd.c_str(), "r");
		UINT32 key = 0;
		if(f)
		{
			fscanf(f,"%d", &key);
			HasKeyFile = (key == 0xDEDDC6CD);
			fclose(f);
		}
	}
#endif

	memInit();

    fioSet(WantDataDir ? ISDIRECTORY : ISARCHIVE);

	if(!WantDataDir)
		sysmainAddArchives();

    b3DSoundEnable = 1; // always want 3D enabled with OpenAL
	//g_bRadialClutter = FALSE; // screwed on the mac at the moment

	Enable_Log(LOG_ALL, WantedUserLogEnabled);
	Set_Log_Level(LOG_INFO);

#if USE_OSG
	ddpSetD3DRender(osgGameUpdate);
#else
	ddpSetD3DRender(gmlpMainUpdate);
#endif
	ddpSetDDRender(rnlogoRender);

	if(!ddpCreateWnd(0, 0, WantedWidth, WantedHeight, 32, !WantedWindow, atoi(WantedVideoCard), WantedRefreshRate))
	{
		printf("ddpCreateWnd failed!\n");
		return -1;
	}
	ddpSetupGL(WantedWidth, WantedHeight);

	return 0;
}

void sysmainQuit()
{
	running = 0;
}

void sysmainKillApp()
{
#if USE_OSG
	osgGameKill();
#endif

	printf("WW2OL Quit\n"); fflush(stdout);

	dimainKill();
    ddpKill();
}

void sysmainAddArchives()
{
	SCHAR** fileList = NULL;
	SINT32 numfiles, i;
	bool foundthem=false;

	fiodGetDirFileList(NULL, &fileList, &numfiles);
	if(!fileList)
	{
		throw std::runtime_error("Archive directory is wrong or no archives");
	}

	for(i = 0; i < numfiles; i++)
	{
		const char* s = fileList[i];
		int len = strlen(s);
		if ( len > 4 )
		{
			if ( s[len-3] == 'w' && s[len-2] == 'w' && s[len-1] == '2' ) // NEW ARCHIVES
			{
				fioaAddArchive(s);
				foundthem = true;
			}
		}
	}

	if(fileList)
	{
		for(i = 0; i < numfiles; i++)
		{
			if(fileList[i])
				memFree(fileList[i]);
		}

		memFree(fileList);
	}

	if ( !foundthem )
	{
		throw std::runtime_error("Archives not found");
	}

/*
	SCHAR** fileList;
	SINT32 numfiles, i;

	fiodGetDirFileList(NULL, &fileList, &numfiles);
	if(!fileList) return;

  bool foundthem = false;

	for(i = 0; i < numfiles; i++)
	{
    const char* s = fileList[i];
    int len = strlen(s);
    if ( len > 4 )
    {
      if ( s[len-1] == '2' && s[len-2] == 'w' && s[len-3] == 'w' )
      {
        fioaAddArchive(s);
        foundthem = true;
      }
      else
      {
        if(!strncmp(s, "playarc.w2", 10))
        {
          fioaAddArchive(fileList[i]);
          foundthem = true;
        }
      }
    }

		memFree(fileList[i]);
	}
	memFree(fileList);

  if ( ! foundthem )
  {
		piExitProgram(_FL, "No archives found");
  }
 */
}
