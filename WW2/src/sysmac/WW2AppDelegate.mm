/* $Id: WW2AppDelegate.mm 3979 2010-06-18 13:05:19Z dchristopher@CRS $ */
#define OBJECTIVE_C
#include "syscfg.h"
#import "WW2AppDelegate.h"
#import "PGDownloader.h"

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
#include "auth/authAppIDs.h"
#include "teul/teulEndpoint.h"
#import <WebKit/WebView.h>
#import <WebKit/WebFrame.h>
#include <openssl/md5.h>

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

#if defined(BETA_RELEASE) || defined(USE_BETA_AUTH)
NSString * const DEFAULT_AUTH_SERVER = @"authbeta.playnet.com";
#else
NSString * const DEFAULT_AUTH_SERVER = @"auth.playnet.com";
#endif

const UINT16 DEFAULT_AUTH_PORT = WWII::Net::Ports::AuthHost ;

const UINT32 PLAYGATE_VERSION = 0x01010007;

const SINT32 ARENA_WW2OL_LIVE = 1;
const SINT32 ARENA_WW2OL_TRAINING = 22;
const SINT32 ARENA_WW2OL_BETA = 222;

NSString * const PGNoPassword = @"";
NSString * const PGUserNameDefaultsKey = @"WW2OL_UserName";
NSString * const PGUserPasswordDefaultsKey = @"WW2OL_UserPassword";
NSString * const PGRememberUserPasswordDefaultsKey = @"WW2OL_RememberUserPassword";
NSString * const PGUnchangedPassword = @"SaVedPaSsWd";

WW2AppDelegate* gPlayGateAppDelegate;

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
	pRecDevice pHIDDevice;
	pRecElement pHIDElement;
	int i = 0;
	int idx = 0;
	
	HIDBuildDeviceList(kHIDPage_GenericDesktop, 0);
	
	pHIDDevice = sticks[0].dev = HIDGetFirstDevice();
	while(pHIDDevice)
	{	
		if(pHIDDevice->usage == kHIDUsage_GD_Joystick || pHIDDevice->usage == kHIDUsage_GD_GamePad)
		{
			strcpy(sticks[idx].name, (char*)pHIDDevice->product);
			sticks[idx].identifier = (pHIDDevice->vendorID << 16) | (pHIDDevice->productID);
			
			idx++;
			if(idx >= MAX_HID_STICKS) break;
		}
		pHIDDevice = sticks[idx].dev = HIDGetNextDevice (pHIDDevice);
	}
	numSticks = idx;
  
	for(idx = 0; idx < (int)numSticks; idx++)
	{
 		int pos = 0;
 		pHIDDevice = sticks[idx].dev;
 		
 		if(pHIDDevice == NULL) break;
 		
 		pHIDElement = HIDGetFirstDeviceElement (pHIDDevice, kHIDElementTypeIO);
		while(pHIDElement)
		{			
			stickElements[i].joynum = -1;
			if(pHIDElement->usagePage == kHIDPage_GenericDesktop)
			{
				switch(pHIDElement->usage)
				{
					case kHIDUsage_GD_X:
					case kHIDUsage_GD_Y:
					case kHIDUsage_GD_Z:
					case kHIDUsage_GD_Slider:
					case kHIDUsage_GD_Rx:
					case kHIDUsage_GD_Ry:
					case kHIDUsage_GD_Rz:
						stickElements[i].ref = pHIDElement;
						stickElements[i].dev = pHIDDevice;
						stickElements[i].joynum = idx;
						stickElements[i].pos = pos;
            break;
				}
			}
			pos++;
			i++;
			if(i >= MAX_STICK_ELEMENTS) break;
			
			pHIDElement = HIDGetNextDeviceElement (pHIDElement, kHIDElementTypeIO);
		}
		
		if(i >= MAX_STICK_ELEMENTS) break;
	}
	numStickElements = i;
}

static void saveJoystickCalibration()
{
	int i, joynum;
	int one = 1;
	
	for(i = 0; i < (int)numStickElements; i++)
	{		
		joynum = stickElements[i].joynum;
		if(joynum < 0 || joynum >= MAX_HID_STICKS || !sticks[joynum].calibrated) continue;
		
		[[NSUserDefaults standardUserDefaults] setObject:[NSNumber numberWithInt:one] 
                                              forKey:[NSString stringWithFormat:@"%d", sticks[joynum].identifier]];
    
		[[NSUserDefaults standardUserDefaults] setObject:[NSNumber numberWithLong:stickElements[i].ref->minReport] 
                                              forKey:[NSString stringWithFormat:@"%d-%d-min", sticks[joynum].identifier, stickElements[i].pos]];
    
		[[NSUserDefaults standardUserDefaults] setObject:[NSNumber numberWithLong:stickElements[i].ref->maxReport] 
                                              forKey:[NSString stringWithFormat:@"%d-%d-max", sticks[joynum].identifier, stickElements[i].pos]];
	}
}

@implementation NSMenuItem (CRSSortItem)

-(NSComparisonResult) sortMenuItemByTitle:(NSMenuItem*)other
{
	return [[self title] compare:[other title] options:NSNumericSearch];
}
@end

@implementation NSMenu (CRSSortItems)

-(NSMenu*)sortMenuItemsByTitle
{
	NSArray* menuItems = 
  [[self itemArray] sortedArrayUsingSelector:@selector(sortMenuItemByTitle:)];
	
	int i;
	for (i = [self numberOfItems] - 1; i >= 0; i--) 	// remove existing items from tail of array
	{
		[self removeItemAtIndex:i];
	}
	
	for (i = [menuItems count] - 1; i >= 0; i--) 		// reverse sort order by going backwards
	{
		[self addItem:[menuItems objectAtIndex:i]];
	}
	
	return self;
}
@end



@interface PGError : NSError {
}
+ (id)errorWithError:(NSError*)rootError domain:(NSString*)domain code:(int)code userInfo:(NSDictionary*)dict;
- (NSString*)localizedDescription;
@end

NSString * const PGErrorDomain = @"PLAYGATE";
NSString * const PGteulErrorDomain = @"PLAYGATE-teul";

NSString * const PGErrorDataKey = @"DATA";
NSString * const PGRootErrorKey = @"ROOT ERROR";

@implementation PGError

+ (id)errorWithError:(NSError*)rootError domain:(NSString*)domain code:(int)code userInfo:(NSDictionary*)dict
{
  if (rootError != nil) {
    if (dict == nil) {
      dict = [NSDictionary dictionaryWithObject:rootError forKey:PGRootErrorKey];
    } else {
      dict = 0;//[[[dict mutableCopy] autorelease] setObject:rootError forKey:PGRootErrorKey];
    }
  }
  
  return [self errorWithDomain:domain code:code userInfo:dict];
}

- (NSString*)localizedDescription {
  NSString* description = [super localizedDescription];
  
  if ([PGErrorDomain  isEqualToString:[self domain]]) {
    // PGErrorDomain - It is from our own error domain
    
    switch([self code]) {
      case PGErrorSilent:
        // return nil for description if no alert dialog box, etc. should be displayed
        description = nil;
        break;
        
        // Drop other special cases in here...
        
      default:
      {
        NSError* rootError = [[self userInfo] objectForKey:PGRootErrorKey];
        id errorData = [[self userInfo] objectForKey:PGErrorDataKey];
        
        NSString* errorNumber = [NSString stringWithFormat:@"ERROR_%d", [self code]];
        NSString* format = NSLocalizedString(errorNumber, @"Error from our domain, keys are ERROR#code.");
        
        if ([errorNumber isEqual:format]) {
          format = NSLocalizedString(@"UNKOWN_ERROR", @"Unkown error, programmer error likely. {code}");
          description = [NSString stringWithFormat:format, [self code]];
        } else if (rootError != nil) {
          description = [NSString stringWithFormat:format, [rootError localizedDescription]];
        } else if (errorData != nil) {
          description = [NSString stringWithFormat:format, errorData];
        } else {
          description = format;
        }               
        break;
      }
    }   
  } else if ([PGteulErrorDomain isEqualToString:[self domain]]) {
    // PGteulErrorDomain - It is from our teul error domain
    
    NSString* format = NSLocalizedString(@"teul_ERROR", @"Error code returned by teul code base. {code}");
    description = [NSString stringWithFormat:format, [self code]];        
  } else {
    // Don't know what this domain is, someone goofed...
    
    NSString* format = NSLocalizedString(@"UNKOWN_DOMAIN", 
                                         @"Unkown error domain, programmer error likely. {description}");
    
    description = [NSString stringWithFormat:format, [super localizedDescription]];
  }
  
  return description;
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
	NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
  
  switch(reply->appID)
	{
		case Auth::Apps::LOGIN_DENIED:
			[gPlayGateAppDelegate clearUserNameAndPassword:nil];
      // fall through
      
		case Auth::Apps::BROWSER:
		case Auth::Apps::WEB_REDIRECT:
			[[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:[NSString stringWithCString:reply->data]]];
      [gPlayGateAppDelegate setError:[PGError errorWithDomain:PGErrorDomain code:PGErrorSilent userInfo:nil]];
      [gPlayGateAppDelegate advanceState];
      break;
      
		case Auth::Apps::WEB_DOWNLOAD:
      memcpy(&gPlayGateAppDelegate->authReply, reply, sizeof(APP_LAUNCH));
			[gPlayGateAppDelegate changeState:WAIT_UPDATE];
      break;
      
		default: // anything else should be an app launch
      memcpy(&gPlayGateAppDelegate->authReply, reply, sizeof(APP_LAUNCH));
      [gPlayGateAppDelegate advanceState];
      break;
	}
  
  [pool release];
	return 0;
}


@implementation WW2AppDelegate

+ (void)initialize
{
  [[NSUserDefaults standardUserDefaults] registerDefaults:
   [NSDictionary dictionaryWithObjectsAndKeys:
    @"", PGUserNameDefaultsKey,
    PGNoPassword, PGUserPasswordDefaultsKey,
    [NSNumber numberWithBool:NO], PGRememberUserPasswordDefaultsKey,
    nil, nil]];
}

- (void)goHome:(id)sender
{
	NSURLRequest* playnetHome = [NSURLRequest requestWithURL: [NSURL URLWithString:@"http://www.battlegroundeurope.com/index.php/campaign-community/campaign-map"] ];
	[[webView mainFrame] loadRequest:playnetHome];
}

- (void)goOGM:(id)sender
{
  NSURLRequest* playnetHome = [NSURLRequest requestWithURL: [NSURL URLWithString:@"http://training.wwiionline.com/scripts/training/index.jsp"]];
	[[webView mainFrame] loadRequest:playnetHome];
}

- (void)webView:(WebView *)sender didFinishLoadForFrame:(WebFrame *)frame
{
  if (frame == [sender mainFrame]) 
	{
		[backButton setEnabled:[sender canGoBack]];
		[forwardButton setEnabled:[sender canGoForward]];
  }
}

#include <libxml/xmlerror.h>

#include "vars.h"

static void xmlerrorstub (void *ctx, const char *msg, ...) {}
static void xmlstructurederrorstub(void *userData, xmlErrorPtr error) {}

- (IBAction)clearUserNameAndPassword:(id)sender
{
	[self setUserName:@""];
	[self setUserPassword:@""];
	
	[userNameField setStringValue:@""];
  [passwordField setStringValue:@""];
	
	[self updateLoginEnabled];
	
	[self saveUserNamePassword];
}

- (IBAction)acceptTOS:(id)sender
{
	[self updateLoginEnabled];
}

- (void)updateLoginEnabled
{
	BOOL canLogin = ![[userNameField stringValue] isEqualToString:@""] &&  ![[passwordField stringValue] isEqualToString:@""];
	
	[playOnlineButton setEnabled: canLogin];
	[trainingServerButton setEnabled: canLogin];
}

- (void)controlTextDidChange:(NSNotification *)notification
{	
	[self updateLoginEnabled];
}


- (void)controlTextDidBeginEditing:(NSNotification *)notification
{
	if([notification object] == passwordField)
	{
		id text = [[notification userInfo] objectForKey:@"NSFieldEditor"];
		[text setSelectedRange:NSMakeRange(0,0)];
		[text delete:self];
	}
}

- (void)closeInstaller
{
	NSArray* apps = [[NSWorkspace sharedWorkspace] launchedApplications];
	
	for(unsigned int i=0; i < [apps count]; i++)
	{
		NSDictionary* appInfo = [apps objectAtIndex:i];
    
		if([[appInfo objectForKey:@"NSApplicationBundleIdentifier"] isEqualToString:@"com.apple.installer"])
		{
			pid_t pid =[[appInfo objectForKey:@"NSApplicationProcessIdentifier"] intValue];
      
			AEDesc addressDesc;
			AppleEvent event, reply;
			OSErr err = AECreateDesc(typeKernelProcessID, &pid, sizeof(pid_t),  &addressDesc);
			if (err == noErr)  err = AECreateAppleEvent(kCoreEventClass, kAEQuitApplication, &addressDesc, kAutoGenerateReturnID, kAnyTransactionID, &event);
			if (err == noErr)  err = AESend(&event, &reply, kAENoReply, kAENormalPriority, kAEDefaultTimeout,0,0);
      
			break;
		}
	}
}

extern void sysmainAddArchives();

#define DEBUG_CHINAx

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
	int argc; 
	char** argv;
	sysmainGetCommandLineArgs(&argc, &argv);
  
	if(argc > 1 && strcmp(argv[1], "-closeinstaller") == 0)
		[self closeInstaller];
  
	xmlSetGenericErrorFunc(0,xmlerrorstub);
	xmlSetStructuredErrorFunc(0,xmlstructurederrorstub);
  
	chAppDir();
  
  fioSet(WantDataDir ? ISDIRECTORY : ISARCHIVE);
	fiodSetPrepath(PREPATH);		

    if(!WantDataDir)
		sysmainAddArchives();

	cfgPrefsLoad();
	mngoptParse("");
#ifndef DEBUG_CHINA
	[self setLanguage];
#else
  gLanguage = gRegion = 5;
#endif
  
  
	if(ServerIP[0])
	{
		self->state = WAIT_APPLE_EVT;
		[self launchOffline:self];
		return;
	}
  
//  g_buildTerrain = 1;
  
	gPlayGateAppDelegate = self;
  
	[webView setFrameLoadDelegate:self];
  [webView setShouldCloseWithWindow:YES];
  
	[self goHome:self];	
  
	[self loadUserNamePassword];
	[userNameField setDelegate:self];
	[passwordField setDelegate:self];
  
  [userNameField setStringValue:[self userName]];
  [rememberPasswordCheckbox setState:rememberUserPassword ? NSOnState : NSOffState];
  
	if(rememberUserPassword)
		[passwordField setStringValue:PGUnchangedPassword];
  
	
	[self updateLoginEnabled];
  
  authServer = DEFAULT_AUTH_SERVER;
  authPort = DEFAULT_AUTH_PORT;
  
  downloadBoxResizeHeight = ([downloadBox frame].size.height + 20);    
  [self setShowDownloadProgress:NO];
  
  [self changeState:WAIT_START_UP];
  
	[launcherWindow setTitle: [NSString stringWithFormat:@"%@ %s", [launcherWindow title], STRVER]];
}

- (void)applicationWillTerminate:(NSNotification *)aNotification
{
	cfgPrefsSave();
#ifdef RECORD_FILES
    extern void dumpFileRefs();
    dumpFileRefs();
#endif
}

- (PGError *)error {
  return error;
}

- (void)setError:(PGError *)newError {
  if (error != newError) {
    [error release];
    error = [newError retain];
  }
}

- (NSString *)patchFilePath {
  return patchFilePath;
}

- (void)setPatchFilePath:(NSString *)newPatchFilePath {
  if (patchFilePath != newPatchFilePath) {
    [patchFilePath release];
    patchFilePath = [newPatchFilePath retain];
  }
}

- (NSString *)userName {
  return userName;
}

- (void)setUserName:(NSString *)newUserName {
  if (userName != newUserName) {
    [userName release];
    userName = [newUserName retain];
  }
}

- (NSString *)userPassword {
  return userPassword;
}

- (void)setUserPassword:(NSString *)newPassword {
  if ((newPassword != PGNoPassword) && [PGNoPassword isEqualToString:newPassword]) {
    newPassword = PGNoPassword;
  }
  
  if (userPassword != newPassword) {
    [userPassword release];
    userPassword = [newPassword retain];
  }
}


- (IBAction)cancelDownload:(id)sender
{
  [downloader abortDownload];   
  [self changeState:WAIT_PATCH_ABORTED];
}

- (IBAction)launchOnline:(id)sender;
{
  if (state == WAIT_APPLE_EVT) 
	{
		int a,b,c,d;
		sscanf(STRVER, "%d.%d.%d.%d", &a,&b,&c,&d);
    arena = (d==255) ? ARENA_WW2OL_LIVE : ARENA_WW2OL_BETA;
    session = 0;
    
    [self advanceState];
  }
}

- (IBAction)launchTraining:(id)sender
{
  if (state == WAIT_APPLE_EVT) 
	{
    arena = ARENA_WW2OL_TRAINING;
    session = 0;
    
    [self advanceState];
  }  
}

#include "clientNetError.h"
#include "uiMain.h"
#include "phui/Util.h"

- (void)sysmain:(NSTimer*)timer
{
	[self closeAllWindows];
  try
  {	
    sysmain();
  }catch (const ClientNetError& e)
  {
    sysmainKillApp();
    CGReleaseAllDisplays();
    NSRunCriticalAlertPanel(@"Network Error", @"", @"OK", NULL,NULL);
  }catch(const char* err)
  {
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


- (void)saveUserNamePassword
{
  [[NSUserDefaults standardUserDefaults] setBool:rememberUserPassword forKey:PGRememberUserPasswordDefaultsKey];
	if (rememberUserPassword)
		[[NSUserDefaults standardUserDefaults] setObject:userPassword forKey:PGUserPasswordDefaultsKey];
	else 
		[[NSUserDefaults standardUserDefaults] setObject:PGNoPassword forKey:PGUserPasswordDefaultsKey];
  
	[[NSUserDefaults standardUserDefaults] setObject:userName forKey:PGUserNameDefaultsKey];
}

- (void)loadUserNamePassword
{
	[self setUserName:[[NSUserDefaults standardUserDefaults] objectForKey:PGUserNameDefaultsKey]];
  rememberUserPassword = [[NSUserDefaults standardUserDefaults] boolForKey:PGRememberUserPasswordDefaultsKey];
  if(rememberUserPassword)
		[self setUserPassword:[[NSUserDefaults standardUserDefaults] objectForKey:PGUserPasswordDefaultsKey]];
  
}

- (void)advanceState
{
  if (error != nil) {
    NSString* description = [error localizedDescription];
    if (description != nil) {
      NSRunCriticalAlertPanel(NSLocalizedString(@"ERROR", @"Alert dialog title"), 
                              description, nil, nil, nil);      
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
      //fall through to next step
      
    case WAIT_AUTH_CONNECT:
      [self setPlayGateStatus:NSLocalizedString(@"Getting user login information.", @"")];
			state = WAIT_USER_PASS;
      
			[self setUserName:[userNameField stringValue]];
			if(![[passwordField stringValue] isEqualToString: PGUnchangedPassword])
				[self setUserPassword:[self MD5digestForString:[passwordField stringValue]]];
			rememberUserPassword = ([rememberPasswordCheckbox state] == NSOnState);
      
			[self saveUserNamePassword];
			
      //fall through to next step
      
    case WAIT_USER_PASS:
      [self setPlayGateStatus:NSLocalizedString(@"Logging into server to check for updates.", @"")];
      
      [self login];
      state = WAIT_LOGIN;
      break;
      
    case WAIT_LOGIN:
      [teulTimer invalidate]; teulTimer = nil;
      
      if (isConnectedToAuthenticationServer) 
      {
        isConnectedToAuthenticationServer = NO;
        teulClose(teulConnectionIndex);
      } 
      
      if (justPatch) 
      {
        [self setPlayGateStatus:NSLocalizedString(@"Game is up-to-date.", @"")];
        
        justPatch = NO;
        [self changeState:WAIT_START_UP];
      } else 
      {
        [self setPlayGateStatus:NSLocalizedString(@"Game is up-to-date, launching game.", @"")];
				
				snprintf(ServerIP, 63, "%d", authReply.serverIP);
				snprintf(ServerPort, 63, "%d", authReply.serverPort);
				snprintf(ServerOTP, 63, "%d", authReply.otp);
        
				[self launchGame];
				
				state = WAIT_LAUNCH;
      }
      break;
      
    case WAIT_LAUNCH:
      state = WAIT_GAME_EXIT;
			break;   
      
    case WAIT_GAME_EXIT:             
      [NSApp terminate:self];
      break;
      
      
    case WAIT_UPDATE:
      [self setPlayGateStatus:NSLocalizedString(@"Game is out-of-date, downloading patch.", @"")];
      
      [self downloadPatch];
      state = WAIT_PATCH_DOWNLOADED;
      break;
      
    case WAIT_PATCH_DOWNLOADED:
      [self setPlayGateStatus:NSLocalizedString(@"Patch downloaded, launching patcher.", @"")];
      [self setShowDownloadProgress:NO];
      [self launchPatch];
      break;
      
		case WAIT_PATCH_ABORTED:
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
  if (isConnectedToAuthenticationServer) {
    [self setError:[PGError errorWithDomain:PGteulErrorDomain code:err userInfo:nil]];
    [self advanceState];
  }
}

- (void)teulUpdate:(NSTimer*)timer
{
	teulUpdateTransport();
}

#include "versionNo.h"

- (SINT32)verifyApplication:(APP_VERIFY*)verifyInfo
{    
  APP_LAUNCH_REQ	launchRequest;
  int vers[4] = {0,0,0,0};

  memset(&launchRequest, 0x0, sizeof(launchRequest));
  strncpy((char *)launchRequest.username, [[self userName] cStringUsingEncoding: NSASCIIStringEncoding], sizeof(launchRequest.username)-1);
	strncpy((char *)launchRequest.password, [[self userPassword] cStringUsingEncoding: NSASCIIStringEncoding], sizeof(launchRequest.password)-1);
	launchRequest.cdKeySize = 0;
	launchRequest.arenaID = verifyInfo->arenaID;
	launchRequest.sessionID = verifyInfo->sessionID;
	launchRequest.appID = verifyInfo->appID;
  
  NSString* appVersion = @STRVER;
  if (appVersion != nil) 
  {
		NSScanner* scanner = [NSScanner scannerWithString:appVersion];
    [scanner scanInt:&vers[0]];
    [scanner scanString:@"." intoString:nil];
    [scanner scanInt:&vers[1]];
    [scanner scanString:@"." intoString:nil];
    [scanner scanInt:&vers[2]];
    [scanner scanString:@"." intoString:nil];
    [scanner scanInt:&vers[3]];
		
		launchRequest.appVersion = 
    ((vers[0] & 0xFF) << 24) | 
    ((vers[1] & 0xFF) << 16) | 
    ((vers[2] & 0xFF) << 8) | 
    (vers[3] & 0xFF);
    
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
  
	UINT32 host = Teul::GetSingleIP([authServer cStringUsingEncoding: NSASCIIStringEncoding]);
  if (host) {
    if (! isteulInitialized) {
      /*      teulInitTransport(LOG_OFF, "communicationerrorlog", 
       teulConfirmCallback, teulErrorCallback, teulErrorCallback);*/
			ww2::Client::Init();
      
      dbugSetWriteLevel(Debug::ERR_ABORT);
      dbugSetLogLevel(Debug::ERR_ABORT);
      
      authClientInstallCallbacks();
      isteulInitialized = YES;
    }
    
    teulTimer = [NSTimer scheduledTimerWithTimeInterval:0.1 // seconds
                                                 target:self
                                               selector:@selector(teulUpdate:)
                                               userInfo:nil
                                                repeats:YES];
    
		
		teulOpenSend(host, authPort, teulConnectCallback, 100000, TCP, teulErrorCallback);
  } else {
    [self setError:[PGError errorWithDomain:PGErrorDomain
                                       code:PGErrorFailedHostLookup
                                   userInfo:[NSDictionary dictionaryWithObject:authServer forKey:PGErrorDataKey]]];
  }
}


- (void)login
{
 	ARENA_REQ arenaRequest;
    memset(&arenaRequest, 0x0, sizeof(arenaRequest));
	arenaRequest.arenaID	= arena;
	arenaRequest.sessionID 	= session;
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

- (void)downloadPatch
{
  [self movePatchToTrash];
  
  NSString* user = nil;
  NSString* password = nil;
  
  NSString* protocol = @"FTP";
  NSString* server = nil;
  NSString* fileName = nil;
  
  int dataSize = MIN(authReply.dataSize, MAX_PARAM);
  if (((int)strlen(authReply.data) + 1) < dataSize) 
  {
    int patchOffset = strlen(authReply.data) + 1;
    dataSize -= patchOffset;
    
    char* patchData = &(authReply.data[patchOffset]);
    NSString* rawUrl = [NSString stringWithCString:patchData length:MIN(dataSize, (int)strlen(patchData))];
    NSScanner* scanner = [NSScanner scannerWithString:rawUrl];
    [scanner scanUpToString:@"\n" intoString:&server];
    [scanner scanUpToString:@"\n" intoString:nil];
    [scanner scanUpToString:@"\n" intoString:&fileName];
    
    if (arena >= 200)
    {
      user = [self userName];
      password = [self userPassword];
    }
    else
    {
      user = (authReply.protocol == 2) ? @"anonymous" : [NSString stringWithFormat:@"%d", authReply.otp];
      password = [self userName];
    }
    
  } 
	
	[self setPlayGateStatus:[NSString stringWithFormat:NSLocalizedString(@"Downloading: %@", @""), fileName]];
  [downloadProgressBar setDoubleValue:0];
  [downloadStatus setStringValue:NSLocalizedString(@"(waiting for download to begin)", @"")];
  
  NSArray* creds = [NSArray arrayWithObjects:
                    [NSURLCredential credentialWithUser:user
                                               password:password
                                            persistence:NSURLCredentialPersistenceNone], 
                    nil]; 
  
  downloader = [PGDownloader downloaderForFile:fileName
                                        server:server
                                      protocol:protocol
                                   credentials:creds
                                   toDirectory:@"~/Desktop"];
  [downloader setDelagate:self];
  [downloader startDownload];
  
  [self setShowDownloadProgress:YES];
}

- (void)patchMounted:(NSNotification *)notification
{
	if(state != WAIT_PATCH_MOUNTED) return;
	
	NSString* mountPath = [[notification userInfo] objectForKey:@"NSDevicePath"];
  
	if([[mountPath commonPrefixWithString:@"/Volumes/" options:NSLiteralSearch] length] > 0)
	{
		NSArray* files = [[NSFileManager defaultManager] directoryContentsAtPath:mountPath];
		for(unsigned int i=0; i < [files count]; i++)
		{
			NSString* file = [files objectAtIndex:i];
			if ([[file pathExtension] isEqualToString: @"pkg"]) 
			{
				[[NSWorkspace sharedWorkspace] openFile:[mountPath stringByAppendingPathComponent:file]];
				[NSApp terminate:self];
				break;
			}
		}
	}
} 

- (void)launchPatch
{	
	state = WAIT_PATCH_MOUNTED;
  
	[[NSWorkspace sharedWorkspace] openFile: patchFilePath];
	[[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self selector:@selector(patchMounted:) name:NSWorkspaceDidMountNotification object:nil];
}

- (NSString*)MD5digestForString:(NSString *)string
{
  const char* str = [string cStringUsingEncoding: NSASCIIStringEncoding];
  MD5_CTX md5Context;
  MD5_Init(&md5Context);
  MD5_Update(&md5Context, str, strlen(str));
  
  unsigned char digest[MD5_DIGEST_LENGTH];
  MD5_Final(digest, &md5Context); 
  
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
  NSString* path = [self patchFilePath];    
  if (path != nil) { 
    [[NSWorkspace sharedWorkspace] performFileOperation:NSWorkspaceRecycleOperation
                                                 source:[path stringByDeletingLastPathComponent]
                                            destination:@""
                                                  files:[NSArray arrayWithObject:[path lastPathComponent]]
                                                    tag:nil];
    
    [self setPatchFilePath:nil];
  }
}

- (void)setPlayGateStatus:(NSString*)status
{
  [playgateStatus setStringValue:status];
  [playgateStatus displayIfNeeded];
  [NSThread sleepUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.75]];
}


- (void)downloader:(PGDownloader*)downloader didDownloadFile:(NSString*)filePath
{
  [downloadProgressBar setDoubleValue:100.0];
  [downloadStatus setStringValue:NSLocalizedString(@"download completed", @"")];
  
  [self setPatchFilePath:filePath];
  [self advanceState];
}

- (void)downloader:(PGDownloader*)downloader didFailWithError:(NSError*)err
{
  [self setError:[PGError errorWithError:err domain:PGErrorDomain code:PGErrorDownloadFailed userInfo:nil]];   
  [self advanceState];
}

- (void)downloader:(PGDownloader*)downloader bytesReceived:(long long)bytesReceived outOf:(long long)expectedLength
{
  float percentComplete = (bytesReceived / (float)expectedLength) * 100.0;
  
  if ([downloadProgressBar isIndeterminate]) {
    [downloadProgressBar setIndeterminate:NO];
  }
  
  if ([self shouldUpdateStatus]) {
    [downloadProgressBar setDoubleValue:percentComplete];
    [downloadStatus setStringValue:
     [NSString stringWithFormat:NSLocalizedString(@"Downloaded %lld KB of %lld KB, %3.1f%%", @""),
      bytesReceived / 1024, expectedLength / 1024, percentComplete]];
  }
}

- (void)downloader:(PGDownloader*)downloader bytesReceived:(long long)bytesReceived
{
  if (! [downloadProgressBar isIndeterminate]) {
    [downloadProgressBar setIndeterminate:YES];
  }
  
  if ([self shouldUpdateStatus]) {
    [downloadStatus setStringValue:
     [NSString stringWithFormat:NSLocalizedString(@"Downloaded %lld KB", @""), bytesReceived / 1024]];
  }
}


-(IBAction)doCancel:(id)sender
{
	cfgPrefsLoad();
	[configWindow orderOut:self];
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
  
	[[NSUserDefaults standardUserDefaults] setObject:[NSNumber numberWithLong:identifier] forKey:@"joy1"];
  
	i = [joy2 indexOfSelectedItem] - 1;
	identifier = (i > -1 && i < MAX_HID_STICKS) ? sticks[i].identifier : 0;
  
	[[NSUserDefaults standardUserDefaults] setObject:[NSNumber numberWithLong:identifier] forKey:@"joy2"];   
  
	i = [joy3 indexOfSelectedItem] - 1;
	identifier = (i > -1 && i < MAX_HID_STICKS) ? sticks[i].identifier : 0;
  
	[[NSUserDefaults standardUserDefaults] setObject:[NSNumber numberWithLong:identifier] forKey:@"joy3"];
  
	i = [joy4 indexOfSelectedItem] - 1;
	identifier = (i > -1 && i < MAX_HID_STICKS) ? sticks[i].identifier : 0;
  
	[[NSUserDefaults standardUserDefaults] setObject:[NSNumber numberWithLong:identifier] forKey:@"joy4"];
  
	[[NSUserDefaults standardUserDefaults] synchronize];
  
	cfgPrefsSave();
	cfgPrefsLoad();
	saveJoystickCalibration();
	
	[configWindow orderOut:self];
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
		id found = [[NSUserDefaults standardUserDefaults] objectForKey:[NSString stringWithFormat:@"%d",sticks[joynum].identifier]];
    
		[calibrate setTitle:(sticks[joynum].calibrated || found != nil) ? @"Re-Calibrate" : @"Calibrate"];
		[calibrate setEnabled:YES];
	}
}


-(IBAction)doCalibrate:(id)sender
{
	int joynum = [sender tag];
	id joyMenu;
	static int i;
	
	if(joynum > -1 && joynum < MAX_HID_STICKS)
	{
		if(joynum == 1) joyMenu = joy1;
		if(joynum == 2) joyMenu = joy2;
		if(joynum == 3) joyMenu = joy3;
		if(joynum == 4) joyMenu = joy4;
		
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
		nstr = [NSString stringWithCString: sticks[i].name];
		[joymenu addItemWithTitle: nstr];
	}
}

-(void)setJoySelection:(id)joymenu joyid:(unsigned long)val
{
	for(unsigned int i = 0; i < numSticks; i++)
	{
		if(sticks[i].identifier == val)
		{
			[joymenu selectItemAtIndex:i+1];
			break;
		}
	}
	[self joyChanged:joymenu];
}


-(IBAction)displayMenuChanged:(id)sender
{
	selectedMonitor = [[display selectedItem] tag];	
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
    
    
    NSString* title;
    
    boolean_t isMain = CGDisplayIsMain(displayID);
    if (isMain) 
    {
      title = [NSString stringWithFormat:	@"%d (Main Display)", 
               CGDisplayUnitNumber(displayID)];
    }
    else
    {
      title = [NSString stringWithFormat:@"%d", CGDisplayUnitNumber(displayID)];
    }
    
    NSMenuItem* menuItem = [[[NSMenuItem alloc] initWithTitle:title 
                                                       action:nil
                                                keyEquivalent:@""] autorelease];
    
    [menuItem setTag:displayMask];
    [menu addItem:menuItem];
    
    if (isMain) 
    {
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
	
	if (modes == nil) return;
	
	int i, count;
	for (i = 0, count = [modes count]; i < count; i++)
	{
		NSDictionary* mode = [modes objectAtIndex:i];
		
		if (([[mode objectForKey:(NSString*)kCGDisplayModeIsSafeForHardware] boolValue] == YES) &&	//include only safe modes
        ([[mode objectForKey:(NSString*)kCGDisplayModeIsTelevisionOutput] boolValue] == NO)
        //&& ([[mode objectForKey:(NSString*)kCGDisplayModeIsStretched] boolValue] == NO)
        ) 			//exclude streched (for now)
		{
			int bpp = [[mode objectForKey:(NSString*)kCGDisplayBitsPerPixel] intValue];
			
			if (bpp == 32) 
			{
				int width = [[mode objectForKey:(NSString*)kCGDisplayWidth] intValue];
				int height = [[mode objectForKey:(NSString*)kCGDisplayHeight] intValue];
        //		float ratio = (float) width /  (float) height;
				
				NSString* title;
				
        //		if (fabs(ratio) > 1.35) //fudge around the ideal ratio of 1 1/3 the game expects
        //		{
        //			title = [NSString stringWithFormat:@"%d x %d (distorted)", width, height];
        //		}
        //		else
        //		{
        title = [NSString stringWithFormat:@"%d x %d", width, height];
        //		}
				
				NSMenuItem* menuItem = [menu itemWithTitle:title];
				if (menuItem == nil) 
				{
					menuItem = [[[NSMenuItem alloc] initWithTitle:title 
                                                 action:nil
                                          keyEquivalent:@""] autorelease];
          
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
	NSArray* resolutionModes = [[resolution selectedItem] representedObject];
	
	if (resolutionModes == nil) return;
	
	int i, count;
	NSMenuItem* menuItem;
	
	for (i = 0, count = [colorDepth numberOfItems]; i < count; i++)
	{
		menuItem = (NSMenuItem*)[colorDepth itemAtIndex:i]; // not sure why cast is needed
		[menuItem setEnabled:NO];
		[menuItem setRepresentedObject:nil];
	}
	
	for (i = 0, count = [resolutionModes count]; i < count; i++)
	{
		NSDictionary* mode = [resolutionModes objectAtIndex:i];
		int bpp = [[mode objectForKey:(NSString*)kCGDisplayBitsPerPixel] intValue];
		
		menuItem = (NSMenuItem*)[[colorDepth menu] itemWithTag:bpp]; // not sure why cast is needed
		if (menuItem != nil) 
		{
			[menuItem setEnabled:YES];
      
			NSMutableArray* depthModes = [menuItem representedObject];
			if (depthModes == nil) 
			{
				depthModes = [NSMutableArray array];
				[menuItem setRepresentedObject:depthModes];
			}
			
			[depthModes addObject:mode];
		}
	}
	
	[self depthMenuChanged:self];	
}

-(void)populateRefreshMenu
{	
	NSArray* depthModes = [[colorDepth selectedItem] representedObject];
	
	if (depthModes == nil) return;
	
	NSMenu* menu = [[[NSMenu alloc] initWithTitle:@"Refresh Rate"] autorelease];
	
	int i, count;
	for (i = 0, count = [depthModes count]; i < count; i++)
	{
		NSDictionary* mode = [depthModes objectAtIndex:i];
		int rate = [[mode objectForKey:(NSString*)kCGDisplayRefreshRate] intValue];
		
		NSString* title;
		
		if (rate == 0) 
		{
			title = @"Fixed (LCD)";
		}
		else
		{
			title = [NSString stringWithFormat:@"%d Hz", rate];
		}
    
		NSMenuItem* menuItem = [[[NSMenuItem alloc] initWithTitle:title 
                                                       action:nil
                                                keyEquivalent:@""] autorelease];
		[menuItem setRepresentedObject:mode];
		[menuItem setTag:rate];
		[menu addItem:menuItem];
	}
	
	[refresh setMenu:[menu sortMenuItemsByTitle]];
  
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

#include "loclApp.h"

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
	cfgPrefsSetInt("app.language", gLanguage);	
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
	
	int joyid;
	
	joyid = [[NSUserDefaults standardUserDefaults] integerForKey:@"joy1"];
	[self setJoySelection:joy1 joyid: joyid];
  
	joyid = [[NSUserDefaults standardUserDefaults] integerForKey:@"joy2"];
	[self setJoySelection:joy2 joyid: joyid];
  
	joyid = [[NSUserDefaults standardUserDefaults] integerForKey:@"joy3"];
	[self setJoySelection:joy3 joyid: joyid];
  
	joyid = [[NSUserDefaults standardUserDefaults] integerForKey:@"joy4"];
	[self setJoySelection:joy4 joyid: joyid];
  
	[configWindow makeKeyAndOrderFront:self];
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
	NSString* path = [[[NSBundle mainBundle] bundlePath] stringByAppendingString:@"/AppData/README"];
	[[NSWorkspace sharedWorkspace] openFile:path];
}

- (void)resetSettings:(id)sender
{
	const char* prefsPath = macPreferencesPath();
	[[NSFileManager defaultManager] removeFileAtPath: [NSString stringWithFormat: @"%s/wwiiol.cfg",prefsPath] handler:nil];
	[[NSFileManager defaultManager] removeFileAtPath: [NSString stringWithFormat: @"%s/%s",prefsPath,PREF_FILE_NAME] handler:nil];
}

@end

LOGICAL syslaunchurl(const char* url, LOGICAL local)
{
	return TRUE;
}
