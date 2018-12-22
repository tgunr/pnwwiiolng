/* $Id: sysMacCompat.mm 2136 2009-11-16 03:27:03Z ahwulf@CRS $ */

#import <Foundation/Foundation.h>
#include "platforms.h" // must come before the platBuild check in order to work

#if platBuild == BUILD_CLIENT

// Warning: gltypes.h must be included AFTER glew.h.
// AppKit pulls in gltypes.h
// syscfg.h pulls in glew.h
// Therefore syscfg.h must be included before AppKit.
// This is an absurd bit of ordering that would break horribly if we ever managed to convert to modules.
#include "sysmac/syscfg.h"
#include <sys/time.h>
#import <AppKit/AppKit.h>
#include "vars.h"
#include "ddp.h"
#include "rnmain.h"
#include "rnlogo.h"
#include "sys/sysLaunchUrl.h"

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

const char* macPreferencesPath()
{
    static NSURL *prefsDir = nil;
    static const char *prefsFSR;
    static dispatch_once_t predicate = 0;
    
    dispatch_once(&predicate, ^ {
        @autoreleasepool {
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
            prefsFSR = strdup(prefsDir.fileSystemRepresentation);
        }
    });
    
    return prefsFSR;
}

const char* macDocumentsPath()
{
    static NSURL *docsDir = nil;
    static const char *docsFSR = nullptr;
    static dispatch_once_t predicate = 0;
    
    dispatch_once(&predicate, ^ {
        @autoreleasepool {
            docsDir = [[NSFileManager defaultManager]
                        URLForDirectory:NSDocumentDirectory inDomain:NSUserDomainMask appropriateForURL:nil create:YES error:nullptr];
            docsDir = [docsDir URLByAppendingPathComponent:@"Battleground Europe"];
            
            // We're giving the docs folder the permissions it would otherwise have
            // gotten by default here. Would 0750 or 0700 be a better choice?
            [[NSFileManager defaultManager] createDirectoryAtURL:docsDir withIntermediateDirectories:YES attributes:@{
                NSFilePosixPermissions: @(0755)
            } error:nullptr];
            docsFSR = strdup(docsDir.fileSystemRepresentation);
        }
    });
    
    return docsFSR;
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
        @"If the problem persists, please visit http://support.playnet.com/.",
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

bool sysExecuteHTTPRequest(const sysBasicHTTPRequest &request, sysBasicHTTPResponse &response, uint64_t &errorCode)
{
    NSURL *url = [NSURL URLWithString:@(request.url.c_str())];
    
    if (!url) {
        errorCode = std::numeric_limits<uint64_t>::max(); // bit of a kludge, but what the hey
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
        // This is evil as hell, but: We "know" that none of the error codes we're even remotely likely to run into
        // will exceed 32 bits, and we "know" that any of the error domains we care about will be uniquely identifiable
        // by the first few characters of their raw string value. It's hacky, but it lets us encode enough info to
        // know what's going on without any other context.
        errorCode = (static_cast<uint64_t>(static_cast<uint32_t>(error.code)) << 32) |
                    // Technically we're exceeding the lifetime of the data's bytes pointer, but in practice, not gonna be a problem.
                    // The assumption is made that domain has at least 6 characters. We'll crash if that's wrong, but we don't really care, it's hideously unlikely.
                    *reinterpret_cast<const uint32_t *>([[error.domain substringWithRange:NSRange{ 2, 4 }] dataUsingEncoding:NSUTF8StringEncoding].bytes);
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

#endif // platBuild == BUILD_CLIENT

std::string Base64Encode(void const *buffer, size_t bufferLength)
{
    return [[NSData dataWithBytesNoCopy:const_cast<void *>(buffer) length:bufferLength] base64EncodedStringWithOptions:0].UTF8String;
}

bool Base64Decode(const std::string &input, void *outputBuffer, size_t *ioBufferLength)
{
    NSData *decoded = [[NSData alloc] initWithBase64EncodedString:[NSString stringWithUTF8String:input.data()] options:0];
    
    if (decoded) {
        [decoded getBytes:outputBuffer length:std::min(*ioBufferLength, static_cast<size_t>(decoded.length))];
        *ioBufferLength = std::min(*ioBufferLength, static_cast<size_t>(decoded.length));
        return true;
    }
    return false;
}
