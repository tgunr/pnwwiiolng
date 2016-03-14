/* $Id: WW2AppDelegate.mm 3979 2010-06-18 13:05:19Z dchristopher@CRS $ */
#define OBJECTIVE_C
#include "syscfg.h"
#import "WW2AppDelegate.h"
#include "sysmain.h"
#include "vars.h"
#include "HID_Utilities.h"
#include "HID_Queue_Utilities.h"
#include "cfgPrefs.h"
#include "ww2Client.h"
#include "ddp.h"
#include "VersionNo.h"
#include "mngopt.h"
#include "wwii/wwiiNetPorts.h"
#include "wwii/wwiiClientMessages.h"
#import <objc/message.h>
#import <CommonCrypto/CommonCrypto.h>
#import <HockeySDK/HockeySDK.h>
#include <libxml/xmlerror.h>
#include "vars.h"
#include "clientNetError.h"
#include "uiMain.h"
#include "phui/Util.h"
#include "loclApp.h"
#include "auth/authTypes.h"
#include "auth/authAppIDs.h"
#include <netdb.h>
#include <arpa/inet.h>
#include "sysLaunchURL.h"

#define PLAYGATE_VERSION 0x01010109

//#define DEFAULT_AUTH_SERVER "auth.playnet.com"
#define DEFAULT_AUTH_SERVER "authdevng.playnet.com"
#define DEFAULT_AUTH_PORT WWII::Net::Ports::AuthHost

static void xmlerrorstub (void *ctx, const char *msg, ...) {}
static void xmlstructurederrorstub(void *userData, xmlErrorPtr error) {}

extern unsigned int g_buildTerrain;

enum
{
    WAIT_START_UP,
    WAIT_APPLE_EVT,
    WAIT_AUTH_CONNECT,
    WAIT_USER_PASS,
    WAIT_LOCATE_APP,
    WAIT_LOGIN,
    WAIT_LAUNCH,
    WAIT_GAME_EXIT,
    WAIT_UPDATE,
    WAIT_PATCH_DOWNLOADED,
    WAIT_PATCH_MOUNTED,
    WAIT_PATCH_ABORTED
};

static WW2AppDelegate* gPlayGateAppDelegate;

extern NSString * const PGErrorDomain;
extern NSString * const PGteulErrorDomain;
extern NSString * const PGErrorDataKey;

extern bool appHidden;

// PGErrorDomain error code constants
enum
{
    PGErrorUnkown                       = 0,
    PGErrorBadPlayGateAE                = 1,
    PGErrorFailedHostLookup             = 2,
    PGErrorDownloadFailed               = 3,
    PGErrorLoginInfoRequestCanceled     = 4,
    PGErrorApplicationNotFound          = 5,
    PGErrorUnkownApplicationRequested   = 6,
    PGErrorLaunchFailed                 = 7,
    PGErrorLaunchTimeout                = 8,
    PGErrorSilent           = -1
};

#define MAX_STICK_ELEMENTS	256
#define MAX_HID_STICKS 		64

typedef struct
{
    pRecDevice			dev;
    pRecElement 		ref;
    int pos;
    unsigned int		joynum;
} hidStickElement;

typedef struct joyInfo
{
    char name[64];
    unsigned long identifier;
    int calibrated;
    pRecDevice dev;
} joyInfo;

static unsigned int numSticks;
static unsigned int numStickElements;
static joyInfo sticks[MAX_HID_STICKS];
static hidStickElement stickElements[MAX_STICK_ELEMENTS];


static void setupJoystick(void)
{
    pRecDevice pHIDDevice = NULL;
    pRecElement pHIDElement = NULL;
    
    HIDBuildDeviceList(kHIDPage_GenericDesktop, 0);
    for (pHIDDevice = HIDGetFirstDevice(), numSticks = 0, numStickElements = 0;
         pHIDDevice && numSticks < MAX_HID_STICKS;
         pHIDDevice = HIDGetNextDevice(pHIDDevice), ++numSticks)
    {
        if (pHIDDevice->usage == kHIDUsage_GD_Joystick || pHIDDevice->usage == kHIDUsage_GD_GamePad)
        {
            int elementPos = 0;
            
            sticks[numSticks].dev = pHIDDevice;
            strlcpy(sticks[numSticks].name, (char *)pHIDDevice->product, 64);
            sticks[numSticks].identifier = static_cast<unsigned long>((pHIDDevice->vendorID << 16) | pHIDDevice->productID);

            for (pHIDElement = HIDGetFirstDeviceElement(pHIDDevice, kHIDElementTypeIO), elementPos = 0;
                 pHIDElement && numStickElements < MAX_STICK_ELEMENTS;
                 pHIDElement = HIDGetNextDeviceElement(pHIDElement, kHIDElementTypeIO), ++numStickElements, ++elementPos)
            {
                stickElements[numStickElements].joynum = UINT_MAX;
                if (pHIDElement->usagePage != kHIDPage_GenericDesktop) {
                    continue;
                }
                switch (pHIDElement->usage)
                {
                    case kHIDUsage_GD_X: case kHIDUsage_GD_Y: case kHIDUsage_GD_Z:
                    case kHIDUsage_GD_Rx: case kHIDUsage_GD_Ry: case kHIDUsage_GD_Rz:
                    case kHIDUsage_GD_Slider:
                        stickElements[numStickElements].ref = pHIDElement;
                        stickElements[numStickElements].dev = pHIDDevice;
                        stickElements[numStickElements].joynum = numSticks;
                        stickElements[numStickElements].pos = elementPos;
                        break;
                }
            }
        }
    }
}

static void saveJoystickCalibration()
{
    for (uint32_t i = 0; i < numStickElements; i++)
    {
        auto joynum = stickElements[i].joynum;
        
        if (joynum >= MAX_HID_STICKS || !sticks[joynum].calibrated) {
            continue;
        }
        
        [[NSUserDefaults standardUserDefaults] setObject:@1 forKey:[NSString stringWithFormat:@"%lu", sticks[joynum].identifier]];
        
        [[NSUserDefaults standardUserDefaults] setObject:@(stickElements[i].ref->minReport)
                                                  forKey:[NSString stringWithFormat:@"%lu-%d-min", sticks[joynum].identifier, stickElements[i].pos]];
        
        [[NSUserDefaults standardUserDefaults] setObject:@(stickElements[i].ref->maxReport)
                                                  forKey:[NSString stringWithFormat:@"%lu-%d-max", sticks[joynum].identifier, stickElements[i].pos]];
    }
}

@interface NSMenu (WW2SortedItems)
- (NSMenu *)sortMenuItemsByTitle;
@end

@implementation NSMenu (WW2SortedItems)

- (NSMenu *)sortMenuItemsByTitle
{
    NSArray *menuItems = [[self itemArray] sortedArrayUsingComparator:^ NSComparisonResult (NSMenuItem *obj1, NSMenuItem *obj2) {
        return [obj2.title compare:obj1.title options:NSNumericSearch]; // reverse sort order by reversing order of params to compare
    }];
    
    [self removeAllItems];
    for (NSMenuItem *item in menuItems) {
        [self addItem:item];
    }
    return self;
}
@end

// Per Apple's recommendedations for error domains
NSString * const PGErrorDomain = @"com.playnet.wwiiol.PGErrorDomain";
NSString * const PGteulErrorDomain = @"com.playnet.wwiiol.PGteulErrorDomain";

@interface NSError (WW2Additions)
+ (instancetype)playgateErrorWithDomain:(NSString *)domain code:(int)code userInfo:(NSDictionary *)dict rootError:(NSError *)rootError;
@end

@implementation NSError (WW2Additions)

+ (instancetype)playgateErrorWithDomain:(NSString *)domain code:(int)code userInfo:(NSDictionary *)dict rootError:(NSError *)rootError
{
    NSMutableDictionary *udict = dict ? [[dict mutableCopy] autorelease] : [NSMutableDictionary dictionary];
    
    if (rootError) {
        udict[NSUnderlyingErrorKey] = rootError;
    }
    if (([domain isEqualToString:PGErrorDomain] || [domain isEqualToString:PGteulErrorDomain]) && !udict[NSLocalizedDescriptionKey]) {
        NSString *lookup = [NSString stringWithFormat:@"%@.%d", domain.pathExtension, code]; // pathExtension is a stupid lazy hack here
        NSString *message = [[NSBundle mainBundle] localizedStringForKey:lookup value:@"MISSING_ERROR" table:nil];
        
        if ([message isEqualToString:@"MISSING_ERROR"]) {
            message = [NSString stringWithFormat:@"%@%d%@",
            	[[NSBundle mainBundle] localizedStringForKey:message value:@"UNKNOWN ERROR: " table:nil],
                code, rootError ? [NSString stringWithFormat:@" (%@)", rootError.localizedDescription] : @""];
        }
        udict[NSLocalizedDescriptionKey] = message;
    }
    return [self errorWithDomain:domain code:code userInfo:udict];
}

@end

static void teulConnectCallback(ENDPOINT& endpoint)
{
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    [gPlayGateAppDelegate teulConnect:endpoint.teulIndex()];
    [pool release];
}
// SINT32 teulIndex, const UCHAR *const data, SINT32 size, const TEUL_CALLBACK* const object, SINT32 error
static void teulErrorCallback(SINT32 teulIndex, const UCHAR* const data, SINT32 size, const TEUL_CALLBACK* object, SINT32 error)
{
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    [gPlayGateAppDelegate teulError:error];
    [pool release];
}

SINT32 authVerifyApp(SINT32 teulIndex, APP_VERIFY* appVerify)
{
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    SINT32 result = [gPlayGateAppDelegate verifyApplication:appVerify];
    [pool release];
    
    return result;
}

SINT32 authLaunchApp(APP_LAUNCH* reply)
{
    switch(reply->appID)
    {
        case Auth::Apps::LOGIN_DENIED:
            [gPlayGateAppDelegate clearUserNameAndPassword:nil];
            [[clang::fallthrough]];
            
        case Auth::Apps::BROWSER:
        case Auth::Apps::WEB_REDIRECT:
            [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:[NSString stringWithUTF8String:reply->data]]];
            gPlayGateAppDelegate.error = [NSError playgateErrorWithDomain:PGErrorDomain code:PGErrorSilent userInfo:nil rootError:nil];
            [gPlayGateAppDelegate advanceState];
            break;
            
        case Auth::Apps::WEB_DOWNLOAD:
            [gPlayGateAppDelegate setAuthReply:reply];
            [gPlayGateAppDelegate changeState:WAIT_UPDATE];
            break;
            
        default: // anything else should be an app launch
            [gPlayGateAppDelegate setAuthReply:reply];
            [gPlayGateAppDelegate advanceState];
            break;
    }
    
    return 0;
}

@interface __NSCFURLSessionTask : NSURLSessionTask
@end

@interface __NSCFLocalSessionTask : __NSCFURLSessionTask
@end

@interface __NSCFLocalDownloadTask : __NSCFLocalSessionTask
@end

@interface __NSCFLocalDownloadTask (WW2Fix32Bit)
@end

@implementation __NSCFLocalDownloadTask (WW2Fix32Bit)
- (id)dataAckCompletion { return objc_getAssociatedObject(self, _cmd); }
- (void)setDataAckCompletion:(id)arg { objc_setAssociatedObject(self, @selector(dataAckCompletion), arg, OBJC_ASSOCIATION_COPY); }
@end

@interface WW2AppDelegate () <NSURLSessionDownloadDelegate>
@end

@implementation WW2AppDelegate

@synthesize error = _error;
@synthesize patchFileURL = _patchFileURL;
@synthesize userName = _userName;
@synthesize userPassword = _userPassword;
@synthesize patchDownloadTask = _patchDownloadTask;

////////////////////////////////////////////////////////////////////////////////
// WebView magic starts here
// Bad bad hack.
static int staticLoadState = 0;

static NSString *placeholderHTML = @""
@"<html>"
@"<head>"
@"<style type=\"text/css\">"
@"body {"
@"    background-image: url('placeholder-background.png');\n"
@"    background-repeat: no-repeat;\n"
@"    background-size: 100% auto;\n"
@"    background-position: top center;\n"
@"    background-color: #f4f4f4;\n"
@"}\n"
@"div.hi { position: absolute; top: 50%; left: 50%; transform: translate(-50%,-50%); }\n"
@"</style>"
@"</head>"
@"<body>"
@"<div class=\"hi\">"
@"<a href=\"http://www.battlegroundeurope.net/\">"
@"<img style=\"height: 90px;\" src=\"placeholder-title.png\"/>"
@"</a>"
@"</div>"
@"</body>"
@"</html>"
;

- (void)webView:(id)sender didFinishLoadForFrame:(id)frame
{
    switch (staticLoadState) {
        case 0: { // loaded the static placeholder
            staticLoadState = 1;
            
            NSURLRequest *playnetHome = [NSURLRequest requestWithURL:[NSURL URLWithString:@"https://webmap.wwiionline.com/"]];

            ((void (*)(id, SEL, NSURLRequest *))objc_msgSend)(frame, @selector(loadRequest:), playnetHome);
            break;
        }
        case 1: // loaded the webmap
            staticLoadState = 2;
            ((void (*)(id, SEL, id))objc_msgSend)(sender, @selector(setFrameLoadDelegate:), nil);
            break;
        case 2: // should never get here
            assert(false);
            [[clang::fallthrough]];
        default: // any nonsense state is even less good
            [NSException raise:NSInternalInconsistencyException format:@"bad load state"];
            break;
    }
}

extern NSString *WebActionOriginalURLKey;

- (void)webView:(id)webView decidePolicyForNavigationAction:(NSDictionary *)actionInformation
        request:(NSURLRequest *)request frame:(id)frame decisionListener:(id)listener
{
    if (staticLoadState > 0 && ![request.URL.host isEqualToString:@"webmap.wwiionline.com"]) {
        // Note: The use of a separate variable here is not simply for
        // verbosity; the openURL method of NSWorkspace is marked _Nonnull, and
        // a dictionary returns _Nullable pointers. Thus the check is necessary
        // for safety (such that failing to apply it would, in case of a nil
        // value, crash the program with an exception).
        NSURL *originalURL = actionInformation[WebActionOriginalURLKey];
        
        if (originalURL) {
            [[NSWorkspace sharedWorkspace] openURL:originalURL];
        }
        ((void (*)(id, SEL))objc_msgSend)(listener, @selector(ignore));
    } else {
        ((void (*)(id, SEL))objc_msgSend)(listener, @selector(use));
    }
}

- (void)webView:(id)sender decidePolicyForNewWindowAction:(NSDictionary *)actionInformation
        request:(NSURLRequest *)request newFrameName:(NSString *)frameName decisionListener:(id)listener
{
    NSURL *originalURL = actionInformation[WebActionOriginalURLKey];
    
    if (originalURL) {
        [[NSWorkspace sharedWorkspace] openURL:originalURL];
    }
    ((void (*)(id, SEL))objc_msgSend)(listener, @selector(ignore));
}

- (void)goHome
{
    staticLoadState = 0;
    
    // We can't actually import WebKit because the JSContext from phui conflicts
    id frame = ((id (*)(id, SEL))objc_msgSend)(webView, @selector(mainFrame));
    
    ((void (*)(id, SEL, id))objc_msgSend)(webView, @selector(setFrameLoadDelegate:), self);
    ((void (*)(id, SEL, id))objc_msgSend)(webView, @selector(setPolicyDelegate:), self);
    
    // Put a static placeholder into the webview until the load is done.
    ((void (*)(id, SEL, NSString *, NSURL *))objc_msgSend)(frame, @selector(loadHTMLString:baseURL:), placeholderHTML, [[NSBundle mainBundle] resourceURL]);
}

// END WebView magic
////////////////////////////////////////////////////////////////////////////////

- (void)setAuthReply:(APP_LAUNCH *)reply
{
    memcpy(&authReply, reply, sizeof(APP_LAUNCH));
}

- (IBAction)clearUserNameAndPassword:(id)sender
{
    if ([sender isKindOfClass:[NSButton class]]) {
        // The request came from the "save password" checkbox. This method is
        // somewhat unfortunately named on that basis.
        if (rememberPasswordCheckbox.state == NSOnState) {
            // The user just turned "Save password" on. We do nothing here,
            // because we don't know if any given entry is valid yet.
            rememberUserPassword = YES;
        } else {
            // The user just turned "Save password" off. We need to clear out
            // our saved password.
            [self removeAllKeychainEntries];
            
            // Now, wipe the password field
            [self setUserPassword:@""];
            [passwordField setStringValue:@""];
            [self updateLoginEnabled];
            
            // There are no user defaults to update. The keychain provides any
            // info we need about the presence or absence of saved account data.
            rememberUserPassword = NO;
        }
    } else {
        // The request came from auth claiming our credentials are bad. It's
        // very poor UX to wipe out user entries without direct user action, so
        // we only wipe out the password. Wipe out any stored credentials,
        // though, in case we originally loaded a bad password from the
        // keychain.
        [self removeAllKeychainEntries];
        [self setUserPassword:@""];
        [passwordField setStringValue:@""];
        _passwordEntryIsInitial = NO;
    }
}

- (void)updateLoginEnabled
{
    BOOL canLogin = ![[userNameField stringValue] isEqualToString:@""] &&  ![[passwordField stringValue] isEqualToString:@""];
    
    [playOnlineButton setEnabled: canLogin];
}

- (void)controlTextDidChange:(NSNotification *)notification
{
    if (notification.object == userNameField) {
        // Editing the username counts as a new set of credentials in terms of
        // whether the password we loaded from keychain (if any) is valid. A
        // side effect of this is that substantially editing the username wipes
        // out stored credentials! We check this in end-editing so changes that
        // don't actually alter the username in the end don't wipe out saved
        // credentials arbitrarily.
    } else if (notification.object == passwordField) {
        // Editing the password definitely invalidates any old credentials!
        if (_passwordEntryIsInitial) {
            [self removeAllKeychainEntries];
            _passwordEntryIsInitial = NO;
        }
    }
    [self updateLoginEnabled];
}


- (void)controlTextDidBeginEditing:(NSNotification *)notification
{
    // No, we do NOT unconditionally clear the password field as soon as the
    // user focuses it for any reason.
}

- (void)controlTextDidEndEditing:(NSNotification *)notification
{
    if (notification.object == userNameField) {
        // Did the user change the account name? If so, any credentials in the
        // keychain are invalid, and any loaded password should be cleared.
        if (![userNameField.stringValue isEqualToString:self.userName]) {
            [self removeAllKeychainEntries];
            if (_passwordEntryIsInitial) {
                _passwordEntryIsInitial = NO;
                [self setUserPassword:@""];
                [passwordField setStringValue:@""];
            }
        }
    } else if (notification.object == passwordField) {
        // Editing the password doesn't do anything at this time.
    }
}

// this delegate method is required
- (void) showMainApplicationWindowForCrashManager:(id)crashManager
{
    // launch the main app window
    [launcherWindow setTitle: [NSString stringWithFormat:@"%@ %s", [launcherWindow title], STRVER]];
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
    // Wipe out any storage of insecure user info.
    [[NSUserDefaults standardUserDefaults] removeObjectForKey:@"WW2OL_UserName"];
    [[NSUserDefaults standardUserDefaults] removeObjectForKey:@"WW2OL_UserPassword"];
    [[NSUserDefaults standardUserDefaults] removeObjectForKey:@"WW2OL_RememberUserPassword"];
    
    [[BITHockeyManager sharedHockeyManager] configureWithIdentifier:@"97d81f56ff22d04ccdbc1a0d537e81c0"];
    [[BITHockeyManager sharedHockeyManager] startManager];
    
    xmlSetGenericErrorFunc(0,xmlerrorstub);
    xmlSetStructuredErrorFunc(0,xmlstructurederrorstub);
    
    chAppDir();
    
    fioSet(WantDataDir ? ISDIRECTORY : ISARCHIVE);
    fiodSetPrepath(PREPATH);
    
    if(!WantDataDir) {
        extern void sysmainAddArchives();
        sysmainAddArchives();
    }
    
    cfgPrefsLoad();
    
    // Skip all the auth stuff if we already have an IP as defined by a ticket file
    if(GameServerIP[0])
    {
        state = WAIT_APPLE_EVT;
        [self launchOffline:self];
        return;
    }
    
    //  g_buildTerrain = 1;
    
    gPlayGateAppDelegate = self;
    
    ((void (*)(id, SEL, id))objc_msgSend)(webView, @selector(setFrameLoadDelegate:), self);
    ((void (*)(id, SEL, BOOL))objc_msgSend)(webView, @selector(setShouldCloseWithWindow:), YES);
    
    [self goHome];
    
    [self loadUserNamePassword];
    [userNameField setDelegate:self];
    [passwordField setDelegate:self];
    
    [self updateLoginEnabled];
    
    authServer = [[NSString stringWithCString:DEFAULT_AUTH_SERVER encoding:NSUTF8StringEncoding] retain];
    if (gAuthServer[0]) {
        [authServer release];
        authServer = [[NSString alloc] initWithUTF8String:gAuthServer];
    }
    authPort = DEFAULT_AUTH_PORT;
    
    NSMutableParagraphStyle *style = [[[NSParagraphStyle defaultParagraphStyle] mutableCopy] autorelease];
    style.alignment = NSCenterTextAlignment;
    style = [[[NSParagraphStyle defaultParagraphStyle] mutableCopy] autorelease];
    style.alignment = NSCenterTextAlignment;

    NSAttributedString *str = [[[NSAttributedString alloc]
    	initWithString:playOnlineButton.title
        attributes:@{
          NSFontAttributeName: (NSFont * _Nonnull)playOnlineButton.font,
          NSParagraphStyleAttributeName: style,
          NSBaselineOffsetAttributeName: @(4.0),
        }]
    autorelease];
    
    [playOnlineButton setAttributedTitle:str];
    
    downloadBoxResizeHeight = ([downloadBox frame].size.height + 20);
    [self setShowDownloadProgress:NO];
    
    // For now, we hardcode the available arena list instead of asking auth for it.
    // But just so ya know, auth CAN provide it.
    arenaLoadingIndicator.hidden = YES;
    [arenaMenu.menu removeAllItems];
#if DEV_BUILD
    // Developers get a build which lets you pick an arbitrary arena ID. This
    // bit will go away once we've implemented having auth tell us what arenas
    // are available.
    [arenaMenu.menu addItemWithTitle:@"LIVE (1)" action:nullptr keyEquivalent:@""].tag = 1;
    [arenaMenu.menu addItemWithTitle:@"TRAINING (22)" action:nullptr keyEquivalent:@""].tag = 22;
    [arenaMenu.menu addItemWithTitle:@"BETA (222)" action:nullptr keyEquivalent:@""].tag = 222;
    [arenaMenu.menu addItemWithTitle:@"DEV (250)" action:nullptr keyEquivalent:@""].tag = 250;
    [arenaMenu selectItemWithTag:1];
    arenaMenu.enabled = YES;
#else
    // Everyone else gets a build that (for now) locks the user to the 1.35 dev
    // server, which is apparently pretending to be arena 1 (e.g. "live").
    [arenaMenu.menu addItemWithTitle:@"1.35 Testing" action:nullptr keyEquivalent:@""].tag = 1;
    [arenaMenu selectItemWithTag:1];
    arenaMenu.enabled = NO;
#endif
    
    // Initialize authentication process
    [self changeState:WAIT_START_UP];
}

- (void)applicationWillTerminate:(NSNotification *)aNotification
{
    cfgPrefsSave();
#ifdef RECORD_FILES
    dumpFileRefs();
#endif
}

- (IBAction)cancelDownload:(id)sender
{
    [self.patchDownloadTask cancel];
    self.patchDownloadTask = nil;
    [self changeState:WAIT_PATCH_ABORTED];
}

// Starts the authentication process
- (IBAction)launchOnline:(id)sender
{
    if (state == WAIT_APPLE_EVT) {

        if (gArena[0]) {
            arena = @(gArena).intValue;
        } else {
            arena = arenaMenu.selectedTag;
        }
        
        session = 0;
        
        // Initial state advancement
        [self advanceState];
    }
}

- (void)sysmain:(NSTimer*)timer
{
    dbugInit(LOG_TO_FILE, nullptr);
    [self closeAllWindows];

    try {
        sysmain();
    } catch (const ClientNetError& e) {
        sysmainKillApp();
        CGReleaseAllDisplays();
        NSRunCriticalAlertPanel(@"Network Error", @"", @"OK", NULL,NULL);
    } catch (const char* err) {
        CGReleaseAllDisplays();
        NSRunCriticalAlertPanel([NSString stringWithUTF8String:err] , @"", @"OK", NULL,NULL);
    }
    [NSApp terminate:self];
}

- (IBAction)launchOffline:(id)sender
{
    if (state == WAIT_APPLE_EVT)
    {
        [self launchGame];
        state = WAIT_LAUNCH;
    }
}

- (void)changeState:(int)newState
{
    state = newState;
    [self advanceState];
}

- (NSArray *)allKeychainAccountNames
{
    OSStatus result = noErr;
    NSString *bundleID = [NSBundle mainBundle].bundleIdentifier ?: @"wwiiol";
    NSDictionary *keychainAccountsQuery = @{
                                            (__bridge id)kSecClass: (__bridge id)kSecClassGenericPassword,
                                            (__bridge id)kSecAttrService: bundleID,
                                            (__bridge id)kSecReturnAttributes: @YES,
                                            (__bridge id)kSecMatchLimit: (__bridge id)kSecMatchLimitAll,
                                            };
    CFTypeRef _matches = nullptr;
    NSArray *matches = nil;
    
    result = SecItemCopyMatching((__bridge CFDictionaryRef)keychainAccountsQuery, &_matches);
    if (result != noErr && result != errSecItemNotFound) {
#if IS_DEBUG
        NSLog(@"Error copying matching items from keychain: %@, %d", keychainAccountsQuery, (int)result);
#endif
        return @[];
    }
    if (result == errSecItemNotFound) {
        return @[];
    }
    
    matches = CFBridgingRelease(_matches);
    
    NSMutableArray *results = [NSMutableArray array];
    
    for (NSDictionary *attrs in matches) {
        [results addObject:attrs[(__bridge id)kSecAttrAccount]];
    }
    return results;
}

- (void)removeAllKeychainEntries
{
    NSString *bundleID = [NSBundle mainBundle].bundleIdentifier ?: @"wwiiol";

    for (NSArray *account in [self allKeychainAccountNames]) {
        NSDictionary *keychainDeleteQuery = @{
                                              (__bridge id)kSecClass: (__bridge id)kSecClassGenericPassword,
                                              (__bridge id)kSecAttrService: bundleID,
                                              (__bridge id)kSecAttrAccount: account,
                                              (__bridge id)kSecMatchLimit: (__bridge id)kSecMatchLimitAll,
                                              };
        OSStatus result = SecItemDelete((__bridge CFDictionaryRef)keychainDeleteQuery);
        
        if (result != noErr/* && result != errSecItemNotFound*/) {
            // Not much we can really do about it...
#if IS_DEBUG
            NSLog(@"FAILED TO DELETE ITEMS FROM KEYCHAIN! Match: %@, result %ld", keychainDeleteQuery, result);
#endif
        }
    }
}

- (void)saveUserNamePassword
{
    // The entered credentials are valid!
    if (_passwordEntryIsInitial) {
        // We used already-saved credentials successfully. Don't bother.
        return;
    }
    
    [self removeAllKeychainEntries];
    if (!rememberUserPassword) {
        // Bail!
        return;
    }
    
    OSStatus result = noErr;
    NSString *bundleID = [NSBundle mainBundle].bundleIdentifier ?: @"wwiiol";
    NSData *data = [passwordField.stringValue dataUsingEncoding:NSUTF8StringEncoding] ?: [NSData data];
    NSDictionary *keychainUpdateQuery = @{
                                          (__bridge id)kSecClass: (__bridge id)kSecClassGenericPassword,
                                          (__bridge id)kSecAttrService: bundleID,
                                          (__bridge id)kSecAttrAccount: self.userName,
                                          (__bridge id)kSecValueData: data,
                                          //        (__bridge id)kSecAttrAccessible: (__bridge id)kSecAttrAccessibleWhenUnlocked,
                                          };
    
    result = SecItemAdd((__bridge CFDictionaryRef)keychainUpdateQuery, nullptr);
    if (result != noErr) {
#if IS_DEBUG
        NSLog(@"Failed to save password to keychain: %@, %ld", keychainUpdateQuery, result);
#endif
        rememberUserPassword = NO;
    }
}

- (void)loadUserNamePassword
{
    @autoreleasepool {
        
        [self setUserName:@""];
        [userNameField setStringValue:@""];
        [self setUserPassword:@""];
        [passwordField setStringValue:@""];
        rememberUserPassword = NO;
        [rememberPasswordCheckbox setState:NSOffState];
        
        NSArray *accounts = [self allKeychainAccountNames];
        
        if (accounts.count > 1) {
#if IS_DEBUG
            NSLog(@"Too many results in keychain! %@", accounts);
#endif
            [self removeAllKeychainEntries];
            return;
        }
        
        if (accounts.count == 0) {
            // No saved credentials
            return;
        }
        
        // There's exactly one credential!
        OSStatus result = noErr;
        NSString *bundleID = [NSBundle mainBundle].bundleIdentifier ?: @"wwiiol";
        NSDictionary *keychainAccountsQuery = @{
                                                (__bridge id)kSecClass: (__bridge id)kSecClassGenericPassword,
                                                (__bridge id)kSecAttrService: bundleID,
                                                (__bridge id)kSecAttrAccount: accounts[0],
                                                (__bridge id)kSecReturnData: @YES,
                                                (__bridge id)kSecMatchLimit: (__bridge id)kSecMatchLimitOne,
                                                };
        CFTypeRef _accountData = nullptr;
        NSData *accountData = nil;
        
        result = SecItemCopyMatching((__bridge CFDictionaryRef)keychainAccountsQuery, &_accountData);
        if (result != noErr) {
#if IS_DEBUG
            NSLog(@"Got error trying to get account data? %@, %d\n", keychainAccountsQuery, (int)result);
#endif
            [self removeAllKeychainEntries];
            return;
        }
        accountData = CFBridgingRelease(_accountData);
        
        NSString *accountPassword = [[NSString alloc] initWithData:accountData encoding:NSUTF8StringEncoding];
        
        [self setUserName:accounts[0]];
        [self setUserPassword:[self MD5digestForString:accountPassword]];
        [accountPassword release]; // In theory we should really scribble over this
        accountPassword = nil;
        
        [userNameField setStringValue:accounts[0]];
        [passwordField setStringValue:@"__LOADEDFROMKEYCHAINANDLOOKSVALID__"];
        [rememberPasswordCheckbox setState:NSOnState];
        _passwordEntryIsInitial = YES;
        rememberUserPassword = YES;
        
    }
}

- (void)advanceState
{
    if (self.error != nil) {
        if (self.error.localizedDescription) {
            NSAlert *alert = [NSAlert alertWithError:self.error];
            
            [alert runModal];
        }
        
        [NSApp terminate:self];
    }
    
    switch(state)
    {
        case WAIT_START_UP:
            //nothing to do here
            state = WAIT_APPLE_EVT;
            break;
            
        case WAIT_APPLE_EVT:
            [self setPlayGateStatus:NSLocalizedString(@"Connecting to server.", @"")];
            [statusWindow makeKeyAndOrderFront:self];

            if (! isConnectedToAuthenticationServer)
            {
                [self connectToAuthenticationServer];
                state = WAIT_AUTH_CONNECT;
                break;
            }
            [[clang::fallthrough]];

        case WAIT_AUTH_CONNECT:
            [self setPlayGateStatus:NSLocalizedString(@"Getting user login information.", @"")];
            state = WAIT_USER_PASS;
            
            [self setUserName:[userNameField stringValue]];
            if (!_passwordEntryIsInitial) {
                [self setUserPassword:[self MD5digestForString:[passwordField stringValue]]];
            }
            [[clang::fallthrough]];
            
        case WAIT_USER_PASS:
            [self setPlayGateStatus:NSLocalizedString(@"Logging into server to check for updates.", @"")];
            
            [self login];
            state = WAIT_LOGIN;
            break;
            
        case WAIT_LOGIN:
            dispatch_source_cancel(teulTimer);
            dispatch_release(teulTimer);
            teulTimer = nullptr;
            
            if (isConnectedToAuthenticationServer) {
                isConnectedToAuthenticationServer = NO;
                teulClose(teulConnectionIndex);
            }
            
            [self setPlayGateStatus:NSLocalizedString(@"Game is up-to-date, launching game.", @"")];
            
            // Login succeeded, save off our login credentials.
            [self saveUserNamePassword];
            
            snprintf(GameServerIP, 63, "%u.%u.%u.%u",
                     (authReply.serverIP >> 24) & 0xff,
                     (authReply.serverIP >> 16) & 0xff,
                     (authReply.serverIP >> 8) & 0xff,
                     authReply.serverIP & 0xff );
            
            snprintf(GameServerPort, 63, "%d", authReply.serverPort);
            snprintf(GameServerOTP, 63, "%d", authReply.otp);
            [self launchGame];
            state = WAIT_LAUNCH;
            break;
            
        case WAIT_LAUNCH:
            if (isConnectedToAuthenticationServer) {
                isConnectedToAuthenticationServer = NO;
                teulClose(teulConnectionIndex);
            }
            
            state = WAIT_GAME_EXIT;
            break;
            
        case WAIT_GAME_EXIT:
            [NSApp terminate:self];
            break;
            
            
        case WAIT_UPDATE:
            if (isConnectedToAuthenticationServer) {
                isConnectedToAuthenticationServer = NO;
                teulClose(teulConnectionIndex);
            }
            [self setPlayGateStatus:NSLocalizedString(@"Game is out-of-date, downloading patch.", @"")];
            [self downloadPatch];
            state = WAIT_PATCH_DOWNLOADED;
            break;
            
        case WAIT_PATCH_DOWNLOADED:
            if (isConnectedToAuthenticationServer) {
                isConnectedToAuthenticationServer = NO;
                teulClose(teulConnectionIndex);
            }
            [self setPlayGateStatus:NSLocalizedString(@"Patch downloaded, launching patcher.", @"")];
            [self setShowDownloadProgress:NO];
            [self launchPatch];
            break;
            
        case WAIT_PATCH_ABORTED:
            if (isConnectedToAuthenticationServer) {
                isConnectedToAuthenticationServer = NO;
                teulClose(teulConnectionIndex);
            }
            state = WAIT_APPLE_EVT;
            [statusWindow orderOut:self];
            break;
    }
}

- (void)teulConnect:(SINT32)index
{
    teulConnectionIndex = index;
    [self advanceState];
}

- (void)teulError:(SINT32)err
{
    dispatch_async(dispatch_get_main_queue(), ^{
        if (isConnectedToAuthenticationServer) {
            self.error = [NSError playgateErrorWithDomain:PGteulErrorDomain code:err userInfo:nil rootError:nil];
            [self advanceState];
        }
    });
}

- (SINT32)verifyApplication:(APP_VERIFY*)verifyInfo
{
    APP_LAUNCH_REQ	launchRequest;
    UINT32 vers[4] = {0,0,0,0};
    
    memset(&launchRequest, 0x0, sizeof(launchRequest));
    strncpy((char *)launchRequest.username, [[self userName] cStringUsingEncoding: NSASCIIStringEncoding], sizeof(launchRequest.username)-1);
    strncpy((char *)launchRequest.password, [[self userPassword] cStringUsingEncoding: NSASCIIStringEncoding], sizeof(launchRequest.password)-1);
    launchRequest.cdKeySize = 0;
    launchRequest.arenaID = verifyInfo->arenaID;
    launchRequest.sessionID = (UINT32)verifyInfo->sessionID;
    launchRequest.appID = verifyInfo->appID;
    
    NSString* appVersion = [[NSString stringWithCString:STRVER encoding:NSUTF8StringEncoding] retain];

    /* command line version override - not sure this is needed, but putting it in anyway */
    if (gClientV[0]) {
        [appVersion release];
        appVersion = [[[NSString alloc] initWithUTF8String:gClientV] retain];
    }
    
    if (appVersion != nil)
    {
        NSScanner* scanner = [NSScanner scannerWithString:appVersion];
        [scanner scanInt:(int *)&vers[0]];
        [scanner scanString:@"." intoString:nil];
        [scanner scanInt:(int *)&vers[1]];
        [scanner scanString:@"." intoString:nil];
        [scanner scanInt:(int *)&vers[2]];
        [scanner scanString:@"." intoString:nil];
        [scanner scanInt:(int *)&vers[3]];
        
        launchRequest.appVersion =
            ((vers[0] & 0xFF) << 24) | ((vers[1] & 0xFF) << 16) |
            ((vers[2] & 0xFF) << 8)  |  (vers[3] & 0xFF);
        
        NSString* msg = [NSString stringWithFormat:NSLocalizedString(@"Found version %ld.%ld.%ld.%ld installed.", @""),
                         vers[0], vers[1], vers[2], vers[3]];
        
        [self setPlayGateStatus:msg];
    } else {
        [self setPlayGateStatus:NSLocalizedString(@"Didn't find game correctly installed.", @"")];
    }
    
    teulSend(teulConnectionIndex, authAPP_LAUNCH_REQ, &launchRequest);
    return 0;
}

- (void)connectToAuthenticationServer
{
    isConnectedToAuthenticationServer = YES;
    
    struct hostent *myhost = gethostbyname(authServer.UTF8String);

    if (myhost != nullptr && myhost->h_addr_list != nullptr) {
        struct in_addr addr;
        memcpy(&addr, myhost->h_addr_list[0], sizeof(struct in_addr));
        UINT32 hostAddress = (UINT32)htonl(addr.s_addr);
        
        if (hostAddress != 0) {
            if (! isteulInitialized) {
                ww2::Client::Init();
                
#if IS_DEBUG
                dbugSetWriteLevel(Debug::ERR_DBUG);
                dbugSetLogLevel(Debug::ERR_DBUG);
#else
                dbugSetWriteLevel(Debug::ERR_ABORT);
                dbugSetLogLevel(Debug::ERR_ABORT);
#endif
                authClientInstallCallbacks();
                isteulInitialized = YES;
            }
            teulTimer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_main_queue());
            dispatch_source_set_timer(teulTimer, DISPATCH_TIME_NOW, static_cast<uint64_t>(0.01 * static_cast<double>(NSEC_PER_SEC)), 100);
            dispatch_source_set_event_handler(teulTimer, ^ { teulUpdateTransport(); });
            dispatch_resume(teulTimer);
            teulOpenSend(hostAddress, authPort, teulConnectCallback, 100000, PROTOCOL_TCP, teulErrorCallback);
        }
    } else {
        self.error = [NSError playgateErrorWithDomain:PGErrorDomain code:PGErrorFailedHostLookup userInfo:nil rootError:nil];
    }
}

- (void)login
{
    ARENA_REQ arenaRequest;
    memset(&arenaRequest, 0x0, sizeof(arenaRequest));
    arenaRequest.arenaID	= arena;
    arenaRequest.sessionID 	= static_cast<UINT32>(session);
    arenaRequest.infoOnly	= 0;
    /*
     int vers[4] = {0,0,0,0};
     
     NSString* appVersion = @STRVER;
     NSScanner* scanner = [NSScanner scannerWithString:appVersion];
     [scanner scanInt:&vers[0]];
     [scanner scanString:@"." intoString:nil];
     [scanner scanInt:&vers[1]];
     [scanner scanString:@"." intoString:nil];
     [scanner scanInt:&vers[2]];
     [scanner scanString:@"." intoString:nil];
     [scanner scanInt:&vers[3]];
     
     arenaRequest.presenterVersion =
     ((vers[0] & 0xFF) << 24) |
     ((vers[1] & 0xFF) << 16) |
     ((vers[2] & 0xFF) << 8) |
     (vers[3] & 0xFF);
     */
    arenaRequest.presenterVersion = PLAYGATE_VERSION;
    
    
    teulSend(teulConnectionIndex, authARENA_REQ, &arenaRequest);
}

- (void)launchGame
{
    [NSTimer scheduledTimerWithTimeInterval:0.1
                                     target:self
                                   selector:@selector(sysmain:)
                                   userInfo:nil
                                    repeats:NO];
    
    state = WAIT_LAUNCH;
}

- (void)URLSession:(NSURLSession *)session
              task:(NSURLSessionTask *)task
willPerformHTTPRedirection:(NSHTTPURLResponse *)response
        newRequest:(NSURLRequest *)request
 completionHandler:(void (^)(NSURLRequest * _Nullable))completionHandler
{
    // Disallow HTTP redirects.
    completionHandler(nil);
}

- (void)URLSession:(NSURLSession *)session
      downloadTask:(NSURLSessionDownloadTask *)downloadTask
      didWriteData:(int64_t)bytesWritten
 totalBytesWritten:(int64_t)totalBytesWritten
totalBytesExpectedToWrite:(int64_t)totalBytesExpectedToWrite
{
    double percentComplete = 0.0;
    
    // The linker seems to have trouble with the named constant for now...
    if (totalBytesExpectedToWrite != /*NSURLSessionTransferSizeUnknown*/-1LL) {
        percentComplete = (static_cast<double>(totalBytesWritten) / static_cast<double>(totalBytesExpectedToWrite)) * 100.0;
        [downloadProgressBar setIndeterminate:NO];
        [downloadProgressBar setDoubleValue:percentComplete];
        if (!_downloadProgressFormatter) {
            _downloadProgressFormatter = [[NSByteCountFormatter alloc] init];
            _downloadProgressFormatter.includesActualByteCount = YES;
            _downloadProgressFormatter.adaptive = NO;
            _downloadProgressFormatter.zeroPadsFractionDigits = YES;
        }
        [downloadStatus setStringValue:[NSString stringWithFormat:NSLocalizedString(@"Downloaded %@ of %@, %3.1f%%", @""),
                                        [_downloadProgressFormatter stringFromByteCount:totalBytesWritten],
                                        [_downloadProgressFormatter stringFromByteCount:totalBytesExpectedToWrite],
                                        percentComplete
                                        ]];
    } else {
        [downloadProgressBar setIndeterminate:YES];
        [downloadStatus setStringValue:[NSString stringWithFormat:NSLocalizedString(@"Downloaded %@...", @""),
                                        [_downloadProgressFormatter stringFromByteCount:totalBytesWritten]
                                        ]];
    }
}

- (void)URLSession:(NSURLSession *)session downloadTask:(NSURLSessionDownloadTask *)downloadTask didFinishDownloadingToURL:(NSURL *)location
{
    [downloadProgressBar setDoubleValue:100.0];
    [downloadStatus setStringValue:NSLocalizedString(@"download completed", @"")];
    self.patchDownloadTask = nil;
    [_patchDownloadSession release];
    _patchDownloadSession = nil;
    
    NSError *error = nil;
    NSURL *patchDir = [[NSFileManager defaultManager] URLForDirectory:NSItemReplacementDirectory
                                                             inDomain:NSUserDomainMask appropriateForURL:location create:YES error:&error];
    NSURL *patchFile = [patchDir URLByAppendingPathComponent:downloadTask.originalRequest.URL.lastPathComponent isDirectory:NO];
    
    if (!patchDir || !patchFile || ![[NSFileManager defaultManager] moveItemAtURL:location toURL:patchFile error:&error]) {
        self.error = [NSError playgateErrorWithDomain:PGErrorDomain code:PGErrorDownloadFailed userInfo:error.userInfo rootError:error];
    } else {
        self.patchFileURL = patchFile;
    }
    
    [self advanceState];
}

- (void)URLSession:(NSURLSession *)session task:(NSURLSessionTask *)task didCompleteWithError:(NSError *)error
{
    if (error) {
        self.error = [NSError playgateErrorWithDomain:PGErrorDomain code:PGErrorDownloadFailed userInfo:nil rootError:error];
    }
    [self advanceState];
}

- (void)URLSession:(NSURLSession *)session didBecomeInvalidWithError:(NSError *)error
{
    self.error = [NSError playgateErrorWithDomain:PGErrorDomain code:PGErrorDownloadFailed userInfo:nil rootError:error];
    [self advanceState];
}

- (void)downloadPatch
{
    [self movePatchToTrash];
    
    NSString *rawPatchURL = [[[NSString alloc]
                              initWithBytesNoCopy:&authReply.data[0]
                              length:strnlen(&authReply.data[0], std::min<size_t>(static_cast<size_t>(authReply.dataSize), MAX_PARAM))
                              encoding:NSASCIIStringEncoding
                              freeWhenDone:NO]
                             autorelease];
    NSURL *patchURL = [NSURL URLWithString:rawPatchURL];
    NSURLSessionConfiguration *patchSessionConfig = [NSURLSessionConfiguration ephemeralSessionConfiguration];
    
    patchSessionConfig.timeoutIntervalForRequest = 60.0;
    patchSessionConfig.allowsCellularAccess = NO;
    
    _patchDownloadSession = [[NSURLSession sessionWithConfiguration:patchSessionConfig delegate:self delegateQueue:[NSOperationQueue mainQueue]] retain];
    self.patchDownloadTask = [_patchDownloadSession downloadTaskWithURL:patchURL];
    
    [self setPlayGateStatus:[NSString stringWithFormat:NSLocalizedString(@"Downloading: %@", @""), patchURL.lastPathComponent]];
    [downloadProgressBar setDoubleValue:0];
    [downloadStatus setStringValue:NSLocalizedString(@"(waiting for download to begin)", @"")];
    [self setShowDownloadProgress:YES];
    
    [self.patchDownloadTask resume];
}

- (void)launchPatch
{
    state = WAIT_PATCH_MOUNTED;
    [[NSWorkspace sharedWorkspace] openURL:self.patchFileURL];
    
    NSAlert *notification = [[[NSAlert alloc] init] autorelease];
    
    notification.messageText = NSLocalizedString(@"launch_patch_heading", @"Text titling the patch launch dialog");
    notification.informativeText = NSLocalizedString(@"launch_patch_message", @"Main text in the patch launch dialog");
    [notification addButtonWithTitle:NSLocalizedString(@"OK", @"OK button in patch launch dialog")];
    notification.showsHelp = NO;
    notification.alertStyle = NSInformationalAlertStyle;
    [notification runModal];
    [NSApp terminate:self];
}

- (NSString*)MD5digestForString:(NSString *)string
{
    NSData *str = [string dataUsingEncoding:NSASCIIStringEncoding];
    NSMutableData *digestData = [NSMutableData dataWithLength:CC_MD5_DIGEST_LENGTH];
    unsigned char *digest = (unsigned char *)digestData.mutableBytes;
    
    CC_MD5(str.bytes, str.length, digest);
    return [NSString stringWithFormat:@"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
            digest[0], digest[1], digest[2], digest[3], digest[4], digest[5], digest[6], digest[7],
            digest[8], digest[9], digest[10], digest[11], digest[12], digest[13], digest[14], digest[15]];
}

- (void)setShowDownloadProgress:(BOOL)showDownloadProgress
{
    if (isHidingDownloadProgress != showDownloadProgress) return;
    
    isHidingDownloadProgress = !showDownloadProgress;
    
    NSRect frame = [statusWindow frame];
    if (showDownloadProgress) {
        frame.size.height = frame.size.height + downloadBoxResizeHeight;
        frame.origin.y = frame.origin.y - downloadBoxResizeHeight;
    } else {
        frame.size.height = frame.size.height - downloadBoxResizeHeight;
        frame.origin.y = frame.origin.y + downloadBoxResizeHeight;
    }
    
    [statusWindow setFrame:frame display:[statusWindow isVisible] animate:YES];
}

- (BOOL)shouldUpdateStatus
{
    NSDate* now = [NSDate date];
    
    if ((lastStatusUpdateTime == nil) || ([now timeIntervalSinceDate:lastStatusUpdateTime] >= 0.25)) {
        [lastStatusUpdateTime release];
        lastStatusUpdateTime = [now retain];
        return YES;
    } else {
        return NO;
    }
}

- (void)movePatchToTrash
{
    if (self.patchFileURL) {
        [[NSFileManager defaultManager] trashItemAtURL:self.patchFileURL resultingItemURL:nullptr error:nullptr];
        self.patchFileURL = nil;
    }
}

- (void)setPlayGateStatus:(NSString*)status
{
    [playgateStatus setStringValue:status];
    [playgateStatus displayIfNeeded];
}

-(IBAction)doCancel:(id)sender
{
    cfgPrefsLoad();
    [launcherWindow endSheet:configWindow returnCode:NSModalResponseCancel];
}

-(IBAction)doOK:(id)sender
{
    int width = [[selectedMode objectForKey:(NSString*)kCGDisplayWidth] intValue];
    int height = [[selectedMode objectForKey:(NSString*)kCGDisplayHeight] intValue];
    int bpp = [[selectedMode objectForKey:(NSString*)kCGDisplayBitsPerPixel] intValue];
    int rate = [[selectedMode objectForKey:(NSString*)kCGDisplayRefreshRate] intValue];
    
    int fsa = [fsaa indexOfSelectedItem];
    cfgPrefsSetInt("app.fsaasamples", fsa==0 ? 0 : fsa*2 );
    
    int sq = [soundType indexOfSelectedItem];
    cfgPrefsSetInt("app.macsound", sq==0 ? 0 : 1 );
    
    int cap = [capAllDisplays state];
    cfgPrefsSetBool("app.capturealldisplays", cap == NSOnState );
    
    NSString* nstr = [NSString stringWithFormat:@"%dx%dx%dx%d", width, height, bpp, rate];
    cfgPrefsSetString("app.resolution", [nstr cStringUsingEncoding: NSASCIIStringEncoding]);
    
    int ival;
    
    nstr = [NSString stringWithFormat:@"%d", selectedMonitor];
    cfgPrefsSetString("app.video", [nstr cStringUsingEncoding: NSASCIIStringEncoding]);
    
    ival = [connection indexOfSelectedItem];
    cfgPrefsSetInt("app.usenetworkroute", ival);
    
    int i;
    unsigned int identifier;
    
    i = [joy1 indexOfSelectedItem] - 1;
    identifier = (i > -1 && i < MAX_HID_STICKS) ? sticks[i].identifier : 0;
    
    [[NSUserDefaults standardUserDefaults] setObject:@(identifier) forKey:@"joy1"];
    
    i = [joy2 indexOfSelectedItem] - 1;
    identifier = (i > -1 && i < MAX_HID_STICKS) ? sticks[i].identifier : 0;
    
    [[NSUserDefaults standardUserDefaults] setObject:@(identifier) forKey:@"joy2"];
    
    i = [joy3 indexOfSelectedItem] - 1;
    identifier = (i > -1 && i < MAX_HID_STICKS) ? sticks[i].identifier : 0;
    
    [[NSUserDefaults standardUserDefaults] setObject:@(identifier) forKey:@"joy3"];
    
    i = [joy4 indexOfSelectedItem] - 1;
    identifier = (i > -1 && i < MAX_HID_STICKS) ? sticks[i].identifier : 0;
    
    [[NSUserDefaults standardUserDefaults] setObject:@(identifier) forKey:@"joy4"];
    
    [[NSUserDefaults standardUserDefaults] synchronize];
    
    cfgPrefsSave();
    cfgPrefsLoad();
    saveJoystickCalibration();
    
    [launcherWindow endSheet:configWindow returnCode:NSModalResponseOK];
}

-(IBAction)joyChanged:(id)sender
{
    id joy = NULL, calibrate = NULL;
    int joynum;
    
    if(sender  == joy1)
    {
        joy = joy1;
        calibrate = calibrate1;
    }else if(sender  == joy2)
    {
        joy = joy2;
        calibrate = calibrate2;
    }else if(sender  == joy3)
    {
        joy = joy3;
        calibrate = calibrate3;
    }else if(sender  == joy4)
    {
        joy = joy4;
        calibrate = calibrate4;
    }
    
    if(!joy || !calibrate) return;
    
    joynum = [joy indexOfSelectedItem] - 1;
    
    if(joynum < 0 || joynum >= MAX_HID_STICKS)
    {
        [calibrate setTitle:@"Calibrate"];
        [calibrate setEnabled:NO];
    }else
    {
        id found = [[NSUserDefaults standardUserDefaults] objectForKey:[NSString stringWithFormat:@"%lu",sticks[joynum].identifier]];
        
        [calibrate setTitle:(sticks[joynum].calibrated || found != nil) ? @"Re-Calibrate" : @"Calibrate"];
        [calibrate setEnabled:YES];
    }
}


-(IBAction)doCalibrate:(id)sender
{
    int joynum = [sender tag];
    id joyMenu = nil;
    static int i;
    
    if(joynum > -1 && joynum < MAX_HID_STICKS)
    {
        if(joynum == 1) joyMenu = joy1;
        else if(joynum == 2) joyMenu = joy2;
        else if(joynum == 3) joyMenu = joy3;
        else if(joynum == 4) joyMenu = joy4;
        else [NSException raise:NSInternalInconsistencyException format:@""];
        
        i = [joyMenu indexOfSelectedItem] - 1;
        
        if(i > -1 && i < (int)numSticks)
        {
            hidTimer = [NSTimer scheduledTimerWithTimeInterval:0.01
                                                        target:self
                                                      selector:@selector(hidUpdate:)
                                                      userInfo:nil
                                                       repeats:YES];
            
            NSBeginAlertSheet(@"Joystick Calibration",
                              @"OK", @"Cancel", NULL, configWindow,
                              self,
                              @selector(calibrationEnd:returnCode:contextInfo:),
                              NULL,
                              &i,
                              @"Move all the axes on your \"%s\" through the entire range of "
                              "motion and then hit OK.", sticks[i].name);
        }
    }
}

-(void)calibrationEnd:(NSWindow*)sheet returnCode:(int)code contextInfo:(int*)i
{
    if((code == NSAlertDefaultReturn) && i)
    {
        if(*i >=0 && *i < MAX_HID_STICKS)
            sticks[*i].calibrated = 1;
    }
    
    [hidTimer invalidate]; hidTimer = nil;
}

-(void)setupJoyMenu:(id)joymenu
{
    NSString* nstr;
    
    [joymenu removeAllItems];
    [joymenu addItemWithTitle:  @"none"];
    
    for(unsigned int i = 0; i < numSticks; i++)
    {
        nstr = [NSString stringWithUTF8String: sticks[i].name];
        [joymenu addItemWithTitle: nstr];
    }
}

-(void)setJoySelection:(id)joymenu joyid:(unsigned long)val
{
    for (unsigned int i = 0; i < numSticks; i++)
    {
        if (sticks[i].identifier == val)
        {
            [joymenu selectItemAtIndex:static_cast<int>(i) + 1];
            break;
        }
    }
    [self joyChanged:joymenu];
}


-(IBAction)displayMenuChanged:(id)sender
{
    selectedMonitor = static_cast<CGOpenGLDisplayMask>(display.selectedItem.tag);
    [self populateResolutionMenu];
}


-(IBAction)resolutionMenuChanged:(id)sender
{
    [self populateDepthMenu];
}


-(IBAction)depthMenuChanged:(id)sender
{
    [self populateRefreshMenu];
}


-(IBAction)refreshMenuChanged:(id)sender
{
    selectedMode = [[refresh selectedItem] representedObject];
}


-(IBAction)fsaaMenuChanged:(id)sender
{
}


-(void)populateDisplayMenu
{
    NSMenu* menu = [[[NSMenu alloc] initWithTitle:@"Displays"] autorelease];
    NSMenuItem* selectItem = nil;
    
    CGDisplayCount 		displayCount;
    CGDirectDisplayID	displayIDs[16];
    CGGetActiveDisplayList(16, displayIDs, &displayCount);
    
    for (unsigned int i = 0; i < displayCount; i++)
    {
        CGDirectDisplayID displayID = displayIDs[i];
        CGOpenGLDisplayMask displayMask = CGDisplayIDToOpenGLDisplayMask(displayID);
        
        
        //TODO limit displays as we want based on capabilities...
        /*
         CGLPixelFormatObj pixelFormat;
         long numberPixelFormats;
         CGLPixelFormatAttribute fullScreenAtts[] =  {	kCGLPFAAccelerated,
         kCGLPFAFullScreen,
         kCGLPFADisplayMask,
         displayMask,
         NULL };
         
         CGLChoosePixelFormat(fullScreenAtts, &pixelFormat, &numberPixelFormats);
         if ((pixelFormat == NULL) || (numberPixelFormats == 0))
         {
         continue; //if this fails to find a pixel format ignore this display
         }
         */
        
        
        NSString* title = nil;
        boolean_t isMain = CGDisplayIsMain(displayID);

        if (isMain) {
            title = [NSString stringWithFormat:	@"%d (Main Display)", CGDisplayUnitNumber(displayID)];
        } else {
            title = [NSString stringWithFormat:@"%d", CGDisplayUnitNumber(displayID)];
        }
        
        NSMenuItem *menuItem = [[[NSMenuItem alloc] initWithTitle:title action:nil keyEquivalent:@""] autorelease];
        
        [menuItem setTag:static_cast<NSInteger>(displayMask)];
        [menu addItem:menuItem];
        if (isMain) {
            selectItem = menuItem;
        }
    }
    
    [display setMenu:[menu sortMenuItemsByTitle]];
    
    if (selectItem != nil)
    {
        [display selectItem:selectItem];
    }
    
    [self displayMenuChanged:self];
}


-(void)populateResolutionMenu
{
    NSMenu* menu = [[[NSMenu alloc] initWithTitle:@"Display Resolution"] autorelease];
    NSArray* modes = (NSArray*)CGDisplayAvailableModes(CGOpenGLDisplayMaskToDisplayID(selectedMonitor));
    
    if (modes == nil) {
        return;
    }
    
    for (NSDictionary *mode in modes)
    {
        if (([mode[(__bridge NSString *)kCGDisplayModeIsSafeForHardware] boolValue] == YES) &&	//include only safe modes
            ([mode[(__bridge NSString *)kCGDisplayModeIsTelevisionOutput] boolValue] == NO)
            //&& ([mode[(__bridge NSString *)kCGDisplayModeIsStretched] boolValue] == NO)
            ) 			//exclude streched (for now)
        {
            int bpp = [[mode objectForKey:(NSString*)kCGDisplayBitsPerPixel] intValue];
            
            if (bpp == 32)
            {
                int width = [[mode objectForKey:(NSString*)kCGDisplayWidth] intValue];
                int height = [[mode objectForKey:(NSString*)kCGDisplayHeight] intValue];
                // float ratio = (float)width / (float)height;
                
                NSString *title = nil;
                
                // if (fabs(ratio) > 1.35) //fudge around the ideal ratio of 1 1/3 the game expects
                // {
                //     title = [NSString stringWithFormat:@"%d x %d (distorted)", width, height];
                // }
                // else
                // {
                    title = [NSString stringWithFormat:@"%d x %d", width, height];
                // }
                
                NSMenuItem* menuItem = [menu itemWithTitle:title];
                
                if (menuItem == nil)
                {
                    menuItem = [[[NSMenuItem alloc] initWithTitle:title action:nil keyEquivalent:@""] autorelease];
                    
                    [menuItem setRepresentedObject:[NSMutableArray array]];
                    [menuItem setTag:width << 16 | height];
                    [menu addItem:menuItem];
                }
                [[menuItem representedObject] addObject:mode];
            }
        }
    }
    [resolution setMenu:[menu sortMenuItemsByTitle]];
    [self resolutionMenuChanged:self];
}

-(void)populateDepthMenu
{
    NSArray *resolutionModes = resolution.selectedItem.representedObject;
    
    if (resolutionModes == nil) {
        return;
    }
    for (NSMenuItem *menuItem in colorDepth.itemArray)
    {
        menuItem.enabled = NO;
        menuItem.representedObject = nil;
    }
    for (NSDictionary *mode in resolutionModes)
    {
        int bpp = [mode[(__bridge NSString *)kCGDisplayBitsPerPixel] intValue];
        NSMenuItem *menuItem = [colorDepth.menu itemWithTag:bpp];
        
        if (menuItem != nil)
        {
            NSMutableArray* depthModes = menuItem.representedObject;
            
            if (depthModes == nil)
            {
                depthModes = [NSMutableArray array];
                menuItem.representedObject = depthModes;
            }
            menuItem.enabled = YES;
            [depthModes addObject:mode];
        }
    }
    [self depthMenuChanged:self];
}

-(void)populateRefreshMenu
{
    NSArray *depthModes = colorDepth.selectedItem.representedObject;
    
    if (depthModes == nil) {
        return;
    }
    
    NSMenu* menu = [[[NSMenu alloc] initWithTitle:@"Refresh Rate"] autorelease];
    
    for (NSDictionary *mode in depthModes)
    {
        int rate = [mode[(__bridge NSString *)kCGDisplayRefreshRate] intValue];
        NSString *title = rate ? [NSString stringWithFormat:@"%d Hz", rate] : @"Fixed (LCD)";
        NSMenuItem* menuItem = [[[NSMenuItem alloc] initWithTitle:title action:nil keyEquivalent:@""] autorelease];
        
        menuItem.representedObject = mode;
        menuItem.tag = rate;
        [menu addItem:menuItem];
    }
    refresh.menu = [menu sortMenuItemsByTitle];
    [self refreshMenuChanged:self];
}

-(void)setDisplayMask:(int)displayMask
{
    int index = [display indexOfItemWithTag:displayMask];
    if (index != -1)
    {
        [display selectItemAtIndex:index];
        [self displayMenuChanged:self];
    }
}

-(void)setResolutionWidth:(int)w height:(int)h
{
    int index = [resolution indexOfItemWithTag:w << 16 | h];
    if (index != -1)
    {
        [resolution selectItemAtIndex:index];
        [self resolutionMenuChanged:self];
    }
}

- (void)setFSAA:(int)fs // 0, 2, 4, 6, 8
{
    int x = fs == 0 ? fs : fs/2;
    if ( x > 4 ) x = 4;
    [fsaa selectItemAtIndex:x];
}

- (void)setCaptureAllDisplays:(bool)x
{
    [capAllDisplays setState:x? NSOnState : NSOffState];
}

-(void)setBitDepth:(int)bpp
{
    int index = [colorDepth indexOfItemWithTag:bpp];
    if (index != -1)
    {
        [colorDepth selectItemAtIndex:index];
        [self depthMenuChanged:self];
    }
}

-(void)setRefreshRate:(int)rate
{
    int index = [refresh indexOfItemWithTag:rate];
    if (index != -1)
    {
        [refresh selectItemAtIndex:index];
        [self refreshMenuChanged:self];
    }
}

-(void)hidUpdate:(NSTimer*)timer
{
    for(unsigned int i = 0; i < numStickElements; i++)
    {
        if(stickElements[i].dev && stickElements[i].ref) {
            HIDGetElementValue(stickElements[i].dev, stickElements[i].ref);
        }
    }
}

- (void)setLanguage
{
    // determine user's current language setting
    NSUserDefaults* defs = [NSUserDefaults standardUserDefaults];
    
    NSArray* languages = [defs objectForKey:@"AppleLanguages"];
    for(unsigned int i = 0; i < [languages count]; i++)
    {
        NSString* preferredLang = [languages objectAtIndex:i];
        NSString* nsstr = [NSLocale canonicalLocaleIdentifierFromString: preferredLang];
        
        if([nsstr isEqualToString:@"en"])
        {
            gLanguage = (UINT32)IDE_LANGUAGE_ENGLISH;
            break;
        }else if([nsstr isEqualToString:@"it"])
        {
            gLanguage = (UINT32)IDE_LANGUAGE_ITALIAN;
            break;
        }else if([nsstr isEqualToString:@"fr"])
        {
            gLanguage =  (UINT32)IDE_LANGUAGE_FRENCH;
            break;
        }else if([nsstr isEqualToString:@"de"])
        {
            gLanguage =  (UINT32)IDE_LANGUAGE_GERMAN;
            break;
        }else if([nsstr isEqualToString:@"es"])
        {
            gLanguage =  (UINT32)IDE_LANGUAGE_SPANISH;
            break;
        }
    }
    cfgPrefsSetInt("app.language", static_cast<int>(gLanguage));
    cfgPrefsSave();
}

- (IBAction)soundMenuChanged:(id)sender
{
}

- (void)setSoundQuality:(int)sq
{
    int x = sq == 0 ? 0 : 1;
    [soundType selectItemAtIndex:x];
}

- (IBAction)openSettings:(id)sender
{
    cfgPrefsLoad();
    
    setupJoystick();
    
    [self populateDisplayMenu];
    
    const char* sval=0;
    
    int n=0;
    cfgPrefsGetInt("app.fsaasamples", &n);
    [self setFSAA:n];
    
    n=0;
    cfgPrefsGetInt("app.macsound", &n);
    [self setSoundQuality:n]; // 0 or 1
    
    bool t = true;
    cfgPrefsGetBool("app.capturealldisplays", &t);
    [self setCaptureAllDisplays:t];
    
    cfgPrefsGetString("app.video", &sval);
    if(sval)
        [self setDisplayMask:atoi(sval)];
    
    int h = 800, w = 600, bpp = 32, refreshRate = 0;
    
    sval=0;
    cfgPrefsGetString("app.resolution", &sval);
    if(sval) 
        sscanf(sval, "%dx%dx%dx%d", &w, &h, &bpp, &refreshRate);
    
    bpp = 32; // no matter what
    
    [self setResolutionWidth:w height:h];
    [self setBitDepth:bpp];
    [self setRefreshRate: refreshRate];
    
    int ival;
    
    if(cfgPrefsGetInt("app.usenetworkroute",&ival))
        [connection selectItemAtIndex: ival];
    
    [self setupJoyMenu:joy1];
    [self setupJoyMenu:joy2];
    [self setupJoyMenu:joy3];
    [self setupJoyMenu:joy4];
    
    unsigned long joyid = 0;
    
    joyid = static_cast<unsigned long>([[NSUserDefaults standardUserDefaults] integerForKey:@"joy1"]);
    [self setJoySelection:joy1 joyid: joyid];
    
    joyid = static_cast<unsigned long>([[NSUserDefaults standardUserDefaults] integerForKey:@"joy2"]);
    [self setJoySelection:joy2 joyid: joyid];
    
    joyid = static_cast<unsigned long>([[NSUserDefaults standardUserDefaults] integerForKey:@"joy3"]);
    [self setJoySelection:joy3 joyid: joyid];
    
    joyid = static_cast<unsigned long>([[NSUserDefaults standardUserDefaults] integerForKey:@"joy4"]);
    [self setJoySelection:joy4 joyid: joyid];
    
    [launcherWindow beginSheet:configWindow completionHandler:^ (NSModalResponse returnCode) {
        // It was all done already. Which is stupid. Oh well.
    }];
}


- (void)closeAllWindows
{
    [configWindow orderOut:self];
    [launcherWindow orderOut:self];
    [statusWindow orderOut:self];
    [launcherWindow close];
}

- (void)doReadme:(id)sender
{
    NSString* path = [[NSBundle mainBundle] pathForResource:@"readme" ofType:@"rtf"];
    [[NSWorkspace sharedWorkspace] openFile:path];
}

- (void)resetSettings:(id)sender
{
    const char* prefsPath = macPreferencesPath();
    [[NSFileManager defaultManager] removeItemAtPath:[@(prefsPath) stringByAppendingPathComponent:@"wwiiol.cfg"] error:nullptr];
    [[NSFileManager defaultManager] removeItemAtPath:[@(prefsPath) stringByAppendingPathComponent:@PREF_FILE_NAME] error:nullptr];
}

- (IBAction)showAbout:(id)sender {
    NSURL *url = [[[NSBundle mainBundle] bundleURL] URLByAppendingPathComponent:@"Contents/Info.plist"];
    NSDictionary *dictionary = [NSDictionary dictionaryWithContentsOfURL:url];
    
    NSString *version = [dictionary objectForKey:@"CFBundleShortVersionString"];
    
    NSRange range = NSMakeRange( 0, 6 );
    NSString *hash = [[dictionary objectForKey:@"GitShaHash"] substringWithRange: range];
    NSString *repoState = [dictionary objectForKey:@"GitState"];
    
    NSString *fullVersion = [NSString stringWithFormat:@"v%@ - %@", version, hash];
    NSDictionary *aboutDict = [NSDictionary dictionaryWithObjectsAndKeys:
                               fullVersion, @"Version", repoState, @"ApplicationVersion", nil];
    [[NSApplication sharedApplication] orderFrontStandardAboutPanelWithOptions:aboutDict];
}

@end

LOGICAL syslaunchurl(const char* url, LOGICAL local)
{
    return TRUE;
}
