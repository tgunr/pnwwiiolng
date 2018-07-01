/* $Id: sysMacCompat.mm 2136 2009-11-16 03:27:03Z ahwulf@CRS $ */
//#include <Carbon/Carbon.h>

#include <CoreFoundation/CoreFoundation.h>

#include "sysmac/syscfg.h"
#include "vars.h"
#include "ddp.h"
#include "rnmain.h"
#include "rnlogo.h"
#include "sys/sysLaunchUrl.h"

#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <Cocoa/Cocoa.h>

//  Folder constants
NSString * const WWIIOnlineFolder = @"WWIIOnLine";
NSString * const BGEFolder = @"Battleground Europe";
//  NSString * const WWIIOnlineFolder = @"com.corneredrats.wwiionline";

UINT64 GetMicroSeconds(void)
{
	struct timeval tv;

	gettimeofday(&tv, nullptr);
    return ((static_cast<UINT64>(tv.tv_sec) * USEC_PER_SEC) + static_cast<UINT64>(tv.tv_usec) /* convert to microseconds*/);
}

UINT64 GetMillisecondTick()
{
    return GetMicroSeconds() / 1000;
}

UINT32 timeGetTime(void)
{
    return static_cast<UINT32>(GetMillisecondTick());
}

void chAppDir()
{
    //  /Applications -> World War II Online.app -> Contents -> Resources
    @autoreleasepool {
        chdir([NSBundle mainBundle].resourcePath.fileSystemRepresentation);
    }
}

char GetOSMajorVersion()
{
    NSOperatingSystemVersion sysVer = [[NSProcessInfo processInfo] operatingSystemVersion];
    return static_cast<char>(sysVer.majorVersion);
}

char GetOSMinorVersion()
{
    NSOperatingSystemVersion sysVer = [[NSProcessInfo processInfo] operatingSystemVersion];
    return static_cast<char>(sysVer.minorVersion);
}

char GetOSRevision()
{
    NSOperatingSystemVersion sysVer = [[NSProcessInfo processInfo] operatingSystemVersion];
    return static_cast<char>(sysVer.patchVersion);
}

char GetOSBuild()
{
    // No build info
    return 0;
}

bool CopyOldSetupFiles()
{
    bool copyFiles = false;
    char buffer [2048];
    int charCount = 0;

    //  const char *folderRename = "_Copied";
    NSString *folderRename = @"_CopiedWWII";
    const char *title1 = "Old Game settings Located.";
    const char *title2 = "Old Game settings Saved.";
    const char *message1 = "An older version of the Game Settings has been found.\n\nDo you wish to re-use them?";
    //  Set Later == const char *message2 = "The Directory:\n%s\n\nhas been re-named to:\n%s_Copied.";

    NSFileManager *fileManager = [NSFileManager defaultManager];
    NSString *documentsDirectory = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) firstObject];
    NSString *bgePath = [documentsDirectory stringByAppendingPathComponent:BGEFolder];
    NSString *ww2Path = [documentsDirectory stringByAppendingPathComponent:WWIIOnlineFolder];

    const char *c_filepath = [bgePath UTF8String];
    buffer[0] = 0;
    //  charCount = sprintf (buffer, "The Directory:\n%s\n\nhas been re-named to:\n%s_Copied.", c_filepath, c_filepath);
    //  charCount = sprintf (buffer, "The Directory:\n%s\n\nhas been re-named to:\n%s%s.", c_filepath, c_filepath, folderRename);
    charCount = sprintf (buffer, "The Directory:\n%s\n\nhas been re-named to:\n%s%s.", c_filepath, c_filepath, [folderRename UTF8String]);
    const char *message2 = &buffer[0];  //  <<=== Set.

    
    //  Locate: /Users -> "currentuser" -> Documents -> Battleground Europe
    if (![fileManager fileExistsAtPath:bgePath])
        return false;
    
    //  Located: /Users -> "currentuser" -> Documents -> Battleground Europe

    
    //  Verify the User wants to copy the old Setup Files.
    NSAlert *alert = [[NSAlert alloc] init];

    alert.messageText = static_cast<NSString * _Nonnull>([NSString stringWithUTF8String:title1] ?: @""); // Force-unwrap those optionals!
    alert.informativeText = static_cast<NSString * _Nonnull>([NSString stringWithUTF8String:message1] ?: @"");
    alert.alertStyle = NSAlertStyleInformational;

    [alert addButtonWithTitle:(@"Yes")];
    [alert addButtonWithTitle:(@"No")];
    
    if ([alert runModal] == NSAlertSecondButtonReturn) {
        // the User clicked: No, Return.
        copyFiles = false;

    } else {
        //  Copy the ENTIRE Contents of:
        //   /Users -> "currentuser" -> Documents -> Battleground Europe
        //   TO
        //   /Users -> "currentuser" -> Documents -> WWIIOnline.
        
        NSDirectoryEnumerator *dirEnum = [fileManager enumeratorAtPath:bgePath];
        NSString *target;
        BOOL success = NO;
        NSError * error = nil;
        
        while ((target = [dirEnum nextObject])) {
            NSString *targetPath = [bgePath stringByAppendingPathComponent:target];
            NSString *destPath = [ww2Path stringByAppendingPathComponent:target];
            
            success = [fileManager copyItemAtPath:targetPath toPath:destPath error:&error];
            if(success != YES)
                //  NSLog(@"Could not copy:-\n\nfrom path:\n%@,\n\nto path:\n%@.\n\nError:\n%@",targetPath, destPath, error);
                NSLog(@"Could not copy:-\n\nfrom path:\n%@,\n\nto path:\n%@.",targetPath, destPath);
            error = nil;
            targetPath = @"";
        }

        //  Copy Completed.

        
        //  Re-Name:
        //   /Users -> "currentuser" -> Documents -> "Battleground Europe"
        //   TO
        //   /Users -> "currentuser" -> Documents -> "Battleground Europe_Copied".
        
        NSURL *oldDir = [NSURL fileURLWithPath:bgePath];
        //  NSURL *newDir = [NSURL fileURLWithPath:[bgePath stringByAppendingString:@"_Copied"]];
        //  NSURL *newDir = [NSURL fileURLWithPath:[bgePath stringByAppendingString:static_cast<NSString * _Nonnull>([NSString stringWithUTF8String:folderRename] ?: @"")]];
        NSURL *newDir = [NSURL fileURLWithPath:[bgePath stringByAppendingString:folderRename]];
        success = [fileManager moveItemAtURL: oldDir toURL: newDir error:&error];
        if(success != YES)
            //  NSLog(@"Could not copy:-\n\nfrom path:\n%@,\n\nto path:\n%@.\n\nError:\n%@",oldDir, newDir, error);
            NSLog(@"Could not rename:-\n\nfrom path:\n%@,\n\nto path:\n%@.",oldDir, newDir);
         error = nil;

        NSAlert *alert1 = [[NSAlert alloc] init];

        alert1.messageText = static_cast<NSString * _Nonnull>([NSString stringWithUTF8String:title2] ?: @""); // Force-unwrap those optionals!
        alert1.informativeText = static_cast<NSString * _Nonnull>([NSString stringWithUTF8String:message2] ?: @"");
        alert1.alertStyle = NSAlertStyleInformational;
        
        [alert1 runModal];
        copyFiles = true;
    }
    
    return copyFiles;
}

const char* macPreferencesPath()
{
    //  /Users -> "currentuser" -> Library -> Preferences -> ww2ol
    //  /Users -> "currentuser" -> Library -> Preferences -> ww2olcfml
    //  /Users -> "currentuser" -> Library -> Preferences -> ww2olwwiiolprefs.xml
    //
    //  NOTICE: This function should no longer be referenced within the New Win Istaller Directory structures.
    //
    
    static NSURL *prefsDir = nil;
    static const char *targetPathFSR;
    static dispatch_once_t predicate = 0;
    
    dispatch_once(&predicate, ^ {
        @autoreleasepool {
            prefsDir = [[NSFileManager defaultManager]
                        URLForDirectory:NSLibraryDirectory
                        inDomain:NSUserDomainMask
                        appropriateForURL:nil
                        create:YES
                        error:nullptr];

            // FIXME: This is a hack to use the prefs directory. Whether this
            // actually belongs there is debatable. Consider Application Support or
            // even Documents instead.
            prefsDir = [prefsDir URLByAppendingPathComponent:@"Preferences"];
            prefsDir = [prefsDir URLByAppendingPathComponent:@"ww2ol"];
            
            // We're giving the prefs folder the permissions it would otherwise have
            // gotten by default here. Would 0750 or 0700 be a better choice?
            [[NSFileManager defaultManager]
                createDirectoryAtURL:prefsDir
                withIntermediateDirectories:YES
                attributes:@{NSFilePosixPermissions: @(0755)
            }
            error:nullptr];
            targetPathFSR = strdup(prefsDir.fileSystemRepresentation);
        }
    });
    
    return targetPathFSR;
}

const char* macAppPath()
{
    //  /Applications -> World War II Online.app
    static const char *targetPathFSR;
    
    NSBundle *main = [NSBundle mainBundle];
    //  NSString *tempString = main.executablePath;
    targetPathFSR = [main.bundlePath UTF8String];

    //  NSString *resourcePath = [main pathForResource:@"ww2" ofType:@"icns"];
    //  NSURL *appDir = [main URLForResource:@"ww2" withExtension:@"icns"];
    
    return targetPathFSR;
}

const char* macDataPath()
{
    //  /Applications -> World War II Online.app -> Contents -> Resources
    static const char *targetPathFSR;
    
    NSBundle *main = [NSBundle mainBundle];
    targetPathFSR = [main.resourcePath UTF8String];
    
    return targetPathFSR;
}


const char* macDocsPath()
{
    //  /Applications -> World War II Online.app -> Contents -> Resources -> docs
    static const char *targetPathFSR;
    
    NSBundle *main = [NSBundle mainBundle];
    static NSURL *targetPathDir = [main resourceURL];
    targetPathDir = [targetPathDir URLByAppendingPathComponent:@"docs"];
    
    //NSString *docsPath = [main resourceURL];
    
    //targetPathFSR = [targetPathDir UTF8String];
    targetPathFSR = strdup(targetPathDir.fileSystemRepresentation);
    
    return targetPathFSR;
}

const char* macPlayarcPath()
{
    //  /Applications -> World War II Online.app -> Contents -> Resources -> playarc.ww2
    static const char *targetPathFSR;
    
    NSBundle *main = [NSBundle mainBundle];
    NSString *playarcPath = [main pathForResource:@"playarc" ofType:@"ww2"];
    
    targetPathFSR = [playarcPath UTF8String];
    
    return targetPathFSR;
}

const char* macWorkingPathAPP()
{
    //  /Library -> Application Support -> WWIIOnline
    //  XXXX - /Library -> Application Support -> com.corneredrats.wwiionline
    //  XXXX - /Users -> Shared -> Library -> Application Support -> WWIIOnline
    //  XXXX - /Users -> Shared -> Library -> Application Support -> com.corneredrats.wwiionline
    //  XXXX - /Users -> "currentuser" -> Library -> Application Support -> WWIIOnline
    //  XXXX - /Users -> "currentuser" -> Library -> Application Support -> com.corneredrats.wwiionline
    static NSURL *targetPathDir = nil;
    static const char *targetPathFSR = nullptr;
    static dispatch_once_t predicate = 0;
    
    dispatch_once(&predicate, ^ {
        @autoreleasepool {
            targetPathDir = [[NSFileManager defaultManager]
                             URLForDirectory:NSApplicationSupportDirectory
                             inDomain:NSLocalDomainMask
                             //  inDomain:NSUserDomainMask
                             appropriateForURL:nil
                             create:YES
                             error:nullptr];
            targetPathDir = [targetPathDir URLByAppendingPathComponent:WWIIOnlineFolder];

            // We're giving the prefs folder the permissions it would otherwise have
            // gotten by default here. Would 0750 or 0700 be a better choice?
            [[NSFileManager defaultManager]
                createDirectoryAtURL:targetPathDir
                withIntermediateDirectories:YES
                attributes:@{NSFilePosixPermissions: @(0755)
            }
            error:nullptr];
            targetPathFSR = strdup(targetPathDir.fileSystemRepresentation);
        }
    });
    
    return targetPathFSR;
}

const char* macCrashLogsPath()
{
    //  /Library -> Application Support -> WWIIOnline -> CrashLogs
    static NSURL *targetPathDir = nil;
    static const char *targetPathFSR = nullptr;
    static dispatch_once_t predicate = 0;
    
    dispatch_once(&predicate, ^ {
        @autoreleasepool {
            targetPathDir = [[NSFileManager defaultManager]
                             URLForDirectory:NSApplicationSupportDirectory
                             inDomain:NSLocalDomainMask
                             appropriateForURL:nil
                             create:YES
                             error:nullptr];
            targetPathDir = [targetPathDir URLByAppendingPathComponent:WWIIOnlineFolder];
            targetPathDir = [targetPathDir URLByAppendingPathComponent:@"CrashLogs"];

            // We're giving the prefs folder the permissions it would otherwise have
            // gotten by default here. Would 0750 or 0700 be a better choice?
            [[NSFileManager defaultManager]
                createDirectoryAtURL:targetPathDir
                withIntermediateDirectories:YES
                attributes:@{NSFilePosixPermissions: @(0755)
            }
            error:nullptr];
            targetPathFSR = strdup(targetPathDir.fileSystemRepresentation);
        }
    });
    
    return targetPathFSR;
}

const char* macUserDocumentsPath()
{
    //  /Users -> "currentuser" -> Documents -> WWIIOnline
    static NSURL *targetPathDir = nil;
    static const char *targetPathFSR = nullptr;
    static dispatch_once_t predicate = 0;
    
    dispatch_once(&predicate, ^ {
        @autoreleasepool {
            targetPathDir = [[NSFileManager defaultManager]
                             URLForDirectory:NSDocumentDirectory
                             inDomain:NSUserDomainMask
                             appropriateForURL:nil
                             create:YES
                             error:nullptr];
            targetPathDir = [targetPathDir URLByAppendingPathComponent:WWIIOnlineFolder];
            
            // We're giving the prefs folder the permissions it would otherwise have
            // gotten by default here. Would 0750 or 0700 be a better choice?
            [[NSFileManager defaultManager]
                createDirectoryAtURL:targetPathDir
                withIntermediateDirectories:YES
                attributes:@{NSFilePosixPermissions: @(0755)
            }
            error:nullptr];
            targetPathFSR = strdup(targetPathDir.fileSystemRepresentation);
        }
    });
    
    return targetPathFSR;
}

const char* macUserWorkingPath()
{
    //  /Users -> "currentuser" -> Documents -> WWIIOnline -> bin
    static NSURL *targetPathDir = nil;
    static const char *targetPathFSR = nullptr;
    static dispatch_once_t predicate = 0;
    
    dispatch_once(&predicate, ^ {
        @autoreleasepool {
            targetPathDir = [[NSFileManager defaultManager]
                             URLForDirectory:NSDocumentDirectory
                             inDomain:NSUserDomainMask
                             appropriateForURL:nil
                             create:YES
                             error:nullptr];
            targetPathDir = [targetPathDir URLByAppendingPathComponent:WWIIOnlineFolder];
            targetPathDir = [targetPathDir URLByAppendingPathComponent:@"bin"];
            
            // We're giving the docs folder the permissions it would otherwise have
            // gotten by default here. Would 0750 or 0700 be a better choice?
            [[NSFileManager defaultManager]
                createDirectoryAtURL:targetPathDir
                withIntermediateDirectories:YES
                attributes:@{NSFilePosixPermissions: @(0755)
            }
             error:nullptr];
            targetPathFSR = strdup(targetPathDir.fileSystemRepresentation);
        }
    });
    
    return targetPathFSR;
}

#import <crt_externs.h>
const char *GetCommandLineA()
{
    static std::string allArgs = " "; // Leading space bypasses nasty logic errors in mngoptParse()
    static dispatch_once_t predicate = 0;
    
    dispatch_once(&predicate, ^ {
        const int * const pargc = _NSGetArgc();
        const char * const * const * const pargv = _NSGetArgv();
        
        for (int i = 1; i < *pargc; ++i) {
            allArgs += (*pargv)[i] + std::string{ " " };
        }
    });
	return strdup(allArgs.c_str());
}

void MacVideoInitializationFailed(const char* problem)
{
    NSUserNotification *notification = [[NSUserNotification alloc] init];
    
    notification.title = @"Video Initialization Failed";
    notification.informativeText = [NSString stringWithFormat:
    	@"The game was unable to initialize the video display: %s\n"
        @"\n"
        @"Please check your settings and try again. "
        @"If the problem persists, please visit https://support.wwiionline.com.",
        problem
    ];
    [[NSUserNotificationCenter defaultUserNotificationCenter] deliverNotification:notification];
}

void PostModalErrorDialog(const char *title, const char *message)
{
    NSAlert *alert = [[NSAlert alloc] init];
    
    alert.messageText = static_cast<NSString * _Nonnull>([NSString stringWithUTF8String:title] ?: @""); // Force-unwrap those optionals!
    alert.informativeText = static_cast<NSString * _Nonnull>([NSString stringWithUTF8String:message] ?: @"");
    alert.alertStyle = NSAlertStyleCritical;
    bool wasFullscreen = !WantedWindow;
    if (wasFullscreen) {
        ddpTemporarilyEscapeFullscreen(true);
    }
    rnUpdateLoadingScreen();
    [alert runModal];
    if (wasFullscreen) {
        ddpTemporarilyEscapeFullscreen(false);
    }
    rnUpdateLoadingScreen();
}

LOGICAL syslaunchurl(const char* url, LOGICAL local)
{
    return TRUE;
}

bool sysExecuteHTTPRequest(const sysBasicHTTPRequest &request, sysBasicHTTPResponse &response)
{
    NSURL *url = [NSURL URLWithString:@(request.url.c_str())];
    
    if (!url) {
        return false;
    }
    
    NSMutableURLRequest *urlRequest = [NSMutableURLRequest requestWithURL:url];
    
    urlRequest.HTTPMethod = @(request.method.c_str());
    for (const std::pair<std::string, std::string> &kv : request.headers) {
    	[urlRequest setValue:@(kv.second.c_str()) forHTTPHeaderField:@(kv.first.c_str())];
    }
    urlRequest.HTTPBody = [NSData dataWithBytes:request.content.c_str() length:request.content.size()];
    
    // NOTE: Deliberately using NSURLConnection, a deprecated API, because
    // NSURLSession doesn't work in 32-bit targets before 10.10 and we still
    // support 10.9, unfortunately.
    NSURLResponse *urlResponse = nil;
    NSError *error = nil;
    NSData *resultData = [NSURLConnection sendSynchronousRequest:urlRequest returningResponse:&urlResponse error:&error];
    
    if (!resultData) {
        return false;
    }
    
    NSHTTPURLResponse *httpURLResponse = static_cast<NSHTTPURLResponse *>(urlResponse);
    
    response.responseCode = httpURLResponse.statusCode;
    response.responseMessage = [NSHTTPURLResponse localizedStringForStatusCode:httpURLResponse.statusCode].UTF8String;
    [httpURLResponse.allHeaderFields enumerateKeysAndObjectsUsingBlock:^ (NSString *key, NSString *obj, BOOL *stop) {
        response.headers[key.lowercaseString.UTF8String] = obj.UTF8String;
    }];
    response.content.assign(static_cast<const char *>(resultData.bytes), resultData.length);
    return true;
}

#if defined(UNIT_TESTING)
static void testHttpRequests()
{
	sysBasicHTTPRequest req;
	sysBasicHTTPResponse resp;

	// To basic case
	req.url = "https://www.apple.com";
	req.method = "GET";
	req.content = "";
	resp.responseMessage = "";
	if (sysExecuteHTTPRequest(req, resp))
	{
		dbugLog(_FLI, "UNIT_TEST: testHttpRequests: Basic: Passed");
		dbugLog(_FLI, "    Code: %d", resp.responseCode);
		dbugLog(_FLI, "    Data: %s", resp.responseMessage.c_str());
	}
	else
		dbugLog(_FLI, "UNIT_TEST: testHttpRequests: Basic: FAILED, response code: %d", resp.responseCode);

	req.url = "https://jsonplaceholder.typicode.com/posts/1";
	req.method = "GET";
	req.content = "";
	resp.responseMessage = "";
	if (sysExecuteHTTPRequest(req, resp))
	{
		dbugLog(_FLI, "UNIT_TEST: testHttpRequests: jsonplaceholder GET: Passed");
		dbugLog(_FLI, "    Code: %d", resp.responseCode);
		dbugLog(_FLI, "    Data: %s", resp.responseMessage.c_str());
	}
	else
		dbugLog(_FLI, "UNIT_TEST: testHttpRequests: jsonplaceholder GET: FAILED");

	req.url = "https://jsonplaceholder.typicode.com/posts";
	req.method = "POST";
	req.content = "{\"title\":\"foo\",\"body\":\"bar\",\"userId\": 1}";
	resp.responseMessage = "";
	req.headers.insert({ "Content-Type", "application/json; charset=utf-8" });
	if (sysExecuteHTTPRequest(req, resp))
	{
		dbugLog(_FLI, "UNIT_TEST: testHttpRequests: jsonplaceholder POST: Passed");
		dbugLog(_FLI, "    Code: %d", resp.responseCode);
		dbugLog(_FLI, "    Data: %s", resp.responseMessage.c_str());
	}
	else
		dbugLog(_FLI, "UNIT_TEST: testHttpRequests: jsonplaceholder POST: FAILED");
}
#endif
