/* $Id: sysMacCompat.mm 2136 2009-11-16 03:27:03Z ahwulf@CRS $ */
//#include <Carbon/Carbon.h>

#include "sysmac/syscfg.h"
#include "vars.h"

#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <Cocoa/Cocoa.h>

extern CGrafPtr aglDraw;
extern Point* QDGlobalToLocalPoint(CGrafPtr, Point*); // compiler doesn't find this function prototype for some reason
  
/************************************************************************************************
	GetTickCount
*************************************************************************************************/

/*
 #include <mach/mach.h>
 #include <mach/mach_time.h>

 uint64_t t = mach_absolute_time();
 static mach_timebase_info_data_t sTimebaseInfo;
 if ( sTimebaseInfo.denom == 0 ) 
 {
 mach_timebase_info(&sTimebaseInfo);
 }
 
 if ( sTimebaseInfo.denom != 1 || sTimebaseInfo.numer != 1 ) 
 {
 ticks= (t * sTimebaseInfo.numer / sTimebaseInfo.denom)/1000/1000;
 }
 else 
 {
 ticks= t /1000/1000;
 }
*/

DWORD GetMillisecondTick()
{
	unsigned long ticks = 0;
	
	UnsignedWide time;
	Microseconds(&time);
 	ticks = (unsigned long)((*(unsigned long long*)&time)/1000);
 	
 	return ticks;
}

unsigned long GetTickCount(void)
{
	unsigned long ticks = 0;
	
	UnsignedWide time;
	Microseconds(&time);
 	ticks = (unsigned long)((*(unsigned long long*)&time)/1000);
 	
 	return ticks;
}

/************************************************************************************************
	GetMicroSeconds
*************************************************************************************************/

SINT32 GetMicroSeconds(void)
{
	UnsignedWide time;
	Microseconds(&time);
 	return (SINT32)(time.lo & 0x7fffffff);
}

/************************************************************************************************
	chAppDir
*************************************************************************************************/

void chAppDir()
{
	ProcessSerialNumber psn;
	FSRef ref;
	char path[4192];
	
	GetCurrentProcess(&psn);
	GetProcessBundleLocation(&psn, &ref);
	FSRefMakePath(&ref,(UCHAR*)path,4000);
	strcat(path,"/AppData/");
	chdir((char*)path);
}

char* getPrePath()
{
  static char* thepath = 0;
  
  if ( thepath == 0 )
  {
    //NSString* wd = [[NSFileManager defaultManager]currentDirectoryPath];
    
    //thepath = "\\Users\\ahwulf\\XCode\\data131\\ww2mods\\DATA\\";
    thepath = ".\\Data\\"; // put symnbolic link with this name pointing to the data directory in AppData
  }
  return thepath;
}

/************************************************************************************************
	macPreferencesPath
*************************************************************************************************/

const char* macPreferencesPath()
{
static int prefsPathLength = 4192;
static char* prefsPath = NULL;
	
	if (prefsPath == NULL)
	{
		prefsPath = (char*)malloc(4192);
		
		HFSUniStr255 folderName;
		CFStringRef str = CFSTR("ww2ol");
		folderName.length = CFStringGetLength(str);
		CFStringGetCharacters(str, CFRangeMake(0, folderName.length), folderName.unicode);
		
		OSErr err;
		FSRef parentRef, ourRef;
		err = FSFindFolder(kUserDomain, kPreferencesFolderType, kCreateFolder, &parentRef);
		err = FSMakeFSRefUnicode(&parentRef,
		      					 folderName.length, folderName.unicode, 
								 kTextEncodingUnknown, &ourRef);
		if (err == fnfErr) 
		{
			err = FSCreateDirectoryUnicode(&parentRef,
										   folderName.length, folderName.unicode,
										   kFSCatInfoNone, NULL, &ourRef, NULL, NULL);
		}
		 
		FSRefMakePath(&ourRef, (unsigned char*)prefsPath, prefsPathLength);
		strcat(prefsPath, macFileSeparator());
	}
	
	return prefsPath;
}

/************************************************************************************************
	macFileSeparator
*************************************************************************************************/

const char* macFileSeparator()
{
	return "/";
}

/************************************************************************************************
	SetCommandLine
*************************************************************************************************/

static char cmdLine[1024];

void SetCommandLine(int argc, char** argv)
{
	strcat(cmdLine," ");

	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	NSString* extraOpts = [[NSUserDefaults standardUserDefaults]  stringForKey:
#if USE_OSG
		@"cmdline-opts-osg"
#else
		@"cmdline-opts"
#endif
];
	if(extraOpts)
	{
		strcat(cmdLine, [extraOpts cStringUsingEncoding:NSASCIIStringEncoding]);
		strcat(cmdLine, " ");
	}

	int i;

	for(i = 1; i < argc; i++)
	{
		strcat(cmdLine, argv[i]);
		strcat(cmdLine, " ");
	}
	[pool release];
}

/************************************************************************************************
	GetCommandLine
*************************************************************************************************/

const char* GetCommandLine()
{
	return cmdLine;
}

#include <IOKit/hidsystem/event_status_driver.h>

/************************************************************************************************
	SetMouseScalingEnabled
*************************************************************************************************/

void SetMouseScalingEnabled(int enabled)
{
static int mouseScalingEnabled = 1;
static NXMouseScaling mouseScaling;

	NXEventHandle eventStatus;
    
    return;
/*
#if RELEASE_CLIENT == 0
	return;
#endif

	if (mouseScalingEnabled == enabled) return;
	eventStatus = NXOpenEventStatus();

	// Set or restore the scaling
	if(enabled) 
	{
		NXSetMouseScaling(eventStatus, &mouseScaling);
	}else 
	{
		NXMouseScaling newScaling;
		newScaling.numScaleLevels = 1;
		newScaling.scaleThresholds[0] = 1;
		newScaling.scaleFactors[0] = -1;
	
		NXGetMouseScaling(eventStatus, &mouseScaling);
		NXSetMouseScaling(eventStatus, &newScaling);
	}

	mouseScalingEnabled = enabled;
		
	NXCloseEventStatus(eventStatus);
 */
}

int wstrncasecmp(const wchar_t *ws1, const wchar_t *ws2, int n)
{
	for ( ; ; )
	{
		if ( n <= 0 )
			return 0 ;
		if ( tolower(*ws1) != tolower(*ws2) )
			return 1 ;
		if ( *ws1 == 0 )
			return 0 ;
		++ws1 ;
		++ws2 ;
		--n ;
	} while ( 1 ) ;
}

int wstrcasecmp(const wchar_t* ws1, const wchar_t* ws2)
{
	while ( tolower(*ws1) == tolower(*ws2) )
	{
		if ( *ws1 == 0 ) return 0 ;
		++ws1 ;
		++ws2 ;
	}
	return 1 ;
}

//////////

void MacVideoInitializationFailed(const char* problem)
{
	CFStringRef srTitle = CFSTR("Video initialization") ;
	char buffer[4096] ;
	snprintf(buffer, 4095
				, "The game was uanble to initialize the video display: %s\n\n"
					"Please check your settings and try again. "
					"If the problem persists, please visit http://support.playnet.com/."
					, problem) ;

	CFStringRef srMessage = CFStringCreateWithCString(NULL, buffer, kCFStringEncodingUTF8) ; 

	CFUserNotificationDisplayAlert(0, kCFUserNotificationStopAlertLevel, NULL, NULL, NULL, srTitle, srMessage, NULL, NULL, NULL, NULL) ;
}
