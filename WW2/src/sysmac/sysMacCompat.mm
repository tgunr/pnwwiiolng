/* $Id: sysMacCompat.mm 2136 2009-11-16 03:27:03Z ahwulf@CRS $ */
//#include <Carbon/Carbon.h>

#include "sysmac/syscfg.h"
#include "vars.h"

#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
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
	struct timeval tv;

	gettimeofday(&tv, nullptr);
    return ((tv.tv_sec * USEC_PER_SEC) + tv.tv_usec /* convert to microseconds*/) / 1000; /* convert to millis */
}

unsigned long GetTickCount(void)
{
	struct timeval tv;

	gettimeofday(&tv, nullptr);
    return ((tv.tv_sec * USEC_PER_SEC) + tv.tv_usec /* convert to microseconds*/) / 1000; /* convert to millis */
}

/************************************************************************************************
	GetMicroSeconds
*************************************************************************************************/

SINT32 GetMicroSeconds(void)
{
	struct timeval tv;

	gettimeofday(&tv, nullptr);
    return ((tv.tv_sec * USEC_PER_SEC) + tv.tv_usec /* convert to microseconds*/);
}

/************************************************************************************************
	chAppDir
*************************************************************************************************/

void chAppDir()
{
    chdir([[[NSBundle mainBundle].executablePath stringByDeletingLastPathComponent] stringByAppendingPathComponent:@"AppData"].fileSystemRepresentation);
}

char* getPrePath()
{
  static char* thepath = 0;
  
  if ( thepath == 0 )
  {
//    NSString* wd = [[NSFileManager defaultManager]currentDirectoryPath];
    
//    thepath = "\\Users\\ahwulf\\XCode\\data131\\ww2mods\\DATA\\";
//      thepath = ".\\Data\\"; // put symnbolic link with this name pointing to the data directory in AppData
    return DataPath;
  }
  return thepath;
}

/************************************************************************************************
	macPreferencesPath
*************************************************************************************************/

const char* macPreferencesPath()
{
    static NSURL *prefsDir = nil;
    static std::unique_ptr<char, std::function<void(void*)>> prefsFSR(nullptr, ::free);
    static dispatch_once_t predicate = 0;
    
    dispatch_once(&predicate, ^ {
        // FIXME: This is a hack to use the prefs directory. Whether this
        // actually belongs there is debatable. Consider Application Support or
        // even Documents instead.
        prefsDir = [[NSFileManager defaultManager]
        	URLForDirectory:NSLibraryDirectory inDomain:NSUserDomainMask appropriateForURL:nil create:YES error:nullptr];
        prefsDir = [prefsDir URLByAppendingPathComponent:@"Preferences"];
        prefsDir = [prefsDir URLByAppendingPathComponent:@"ww2ol"];
        
        // We're giving the prefs folder the permissions it would otherwise have
        // gotten by default here. Would 0750 or 0700 be a better choice?
        [[NSFileManager defaultManager] createDirectoryAtURL:prefsDir withIntermediateDirectories:YES attributes:@{
            NSFilePosixPermissions: @(0755)
        } error:nullptr];
        prefsFSR.reset(strdup(prefsDir.fileSystemRepresentation));
    });
    
    return prefsFSR.get();
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
//static int mouseScalingEnabled = 1;
//static NXMouseScaling mouseScaling;
//
//	NXEventHandle eventStatus;
//    
//    return;
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
