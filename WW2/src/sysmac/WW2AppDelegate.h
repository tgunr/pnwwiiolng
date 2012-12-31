#import <Cocoa/Cocoa.h>

#include "teulauth.h"
#include "auth/authClientMessages.h"
#include "auth/authTypes.h"

@class PGDownloader;
@class PGError;

@interface NSMenuItem (WW2CSortItem)
-(NSComparisonResult) sortMenuItemByTitle:(NSMenuItem*)other;
@end

@interface NSMenu (WW2CSortItems)
-(NSMenu*)sortMenuItemsByTitle;
@end

@interface WW2AppDelegate : NSObject
{
  IBOutlet NSPopUpButton* soundType;
  IBOutlet NSButton* capAllDisplays;
 IBOutlet id backButton;
  IBOutlet NSButton* calibrate1;
  IBOutlet NSButton* calibrate2;
  IBOutlet NSButton* calibrate3;
  IBOutlet NSButton* calibrate4;
  IBOutlet NSPopUpButton* colorDepth;
  IBOutlet NSPopUpButton* fsaa;
  IBOutlet NSWindow* configWindow;
  IBOutlet id connection;
  IBOutlet NSPopUpButton* display;
  IBOutlet id downloadBox;
  IBOutlet id downloadProgressBar;
  IBOutlet id downloadStatus;
  IBOutlet id forwardButton;
  IBOutlet NSPopUpButton* joy1;
  IBOutlet NSPopUpButton* joy2;
  IBOutlet NSPopUpButton* joy3;
  IBOutlet NSPopUpButton* joy4;
  IBOutlet id launcherWindow;
  IBOutlet id playgateStatus;
  IBOutlet NSPopUpButton* refresh;
  IBOutlet id rememberPasswordCheckbox;
  IBOutlet NSPopUpButton* resolution;
  IBOutlet id spinner;
  IBOutlet id statusWindow;
  IBOutlet id userNameField;
  IBOutlet id passwordField;
  IBOutlet id webView;
  IBOutlet id playOnlineButton;
  IBOutlet id trainingServerButton;

  float downloadBoxResizeHeight;
  BOOL isHidingDownloadProgress;
  NSDate* lastStatusUpdateTime;

  int state;
  PGError* error;

  BOOL justPatch;

  NSData* sourcePSN;
  int arena;
  int session;
  NSString* userName;
  NSString* userPassword;
  BOOL rememberUserPassword;

  NSString* patchFilePath;
  PGDownloader* downloader;

  NSString* authServer;
  UINT16 authPort;
  SINT32 teulConnectionIndex;
  BOOL isConnectedToAuthenticationServer;
  BOOL isteulInitialized;
  NSTimer* teulTimer;
  NSTimer* launchTimer;

  @public APP_LAUNCH authReply;

  int selectedMonitor;
  NSDictionary* selectedMode;
  NSTimer* hidTimer;
}

- (IBAction)cancelDownload:(id)sender;
- (IBAction)fsaaMenuChanged:(id)sender;
- (IBAction)soundMenuChanged:(id)sender;
- (IBAction)depthMenuChanged:(id)sender;
- (IBAction)displayMenuChanged:(id)sender;
- (IBAction)doCalibrate:(id)sender;
- (IBAction)doCancel:(id)sender;
- (IBAction)doOK:(id)sender;
- (IBAction)doReadme:(id)sender;
- (IBAction)goHome:(id)sender;
- (IBAction)goOGM:(id)sender;
- (IBAction)joyChanged:(id)sender;
- (IBAction)launchOffline:(id)sender;
- (IBAction)launchOnline:(id)sender;
- (IBAction)launchTraining:(id)sender;
- (IBAction)openSettings:(id)sender;
- (IBAction)refreshMenuChanged:(id)sender;
- (IBAction)resolutionMenuChanged:(id)sender;
- (IBAction)acceptTOS:(id)sender;

- (PGError*)error;
- (void)setCaptureAllDisplays:(bool)x;
- (void)setFSAA:(int)fs;
- (void)setSoundQuality:(int)sq;
- (void)setError:(PGError*)newError;
- (NSString*)patchFilePath;
- (void)setPatchFilePath:(NSString*)newPatchFilePath;
- (NSString*)userName;
- (void)setUserName:(NSString*)newUserName;
- (NSString*)userPassword;
- (void)setUserPassword:(NSString*)newPassword;

- (void)changeState:(int)newState;
- (void)advanceState;

- (void)teulConnect:(SINT32)index;
- (void)teulError:(SINT32)err;
- (SINT32)verifyApplication:(APP_VERIFY*)verifyInfo;
- (void)connectToAuthenticationServer;
- (void)login;
- (void)launchGame;
- (void)downloadPatch;
- (void)launchPatch;
- (void)movePatchToTrash;
- (void)setPlayGateStatus:(NSString*)status;
- (NSString*)MD5digestForString:(NSString*)string;
- (void)setShowDownloadProgress:(BOOL)showDownloadProgress;
- (BOOL)shouldUpdateStatus;

-(void)setupJoyMenu:(id)joymenu;
-(void)setJoySelection:(id)joymenu joyid:(unsigned long)val;
-(void)hidUpdate:(NSTimer*)timer;
-(void)calibrationEnd:(NSWindow*)sheet returnCode:(int)code contextInfo:(int*)count;

-(void)populateDisplayMenu;
-(void)populateResolutionMenu;
-(void)populateDepthMenu;
-(void)populateRefreshMenu;
-(void)setDisplayMask:(int)mask;
-(void)setResolutionWidth:(int)w height:(int)h;
-(void)setBitDepth:(int)bpp;
-(void)setRefreshRate:(int)rate;

/*
- (void)applicationWillTerminate:(NSNotification*)aNotification;
- (void)applicationDidFinishLaunching:(NSNotification*)notification;
- (void)applicationDidBecomeActive:(NSNotification*)notification;
- (void)applicationWillResignActive:(NSNotification*)notification;
*/

- (void)closeAllWindows;

- (void)sysmain:(NSTimer*)timer;

- (void)controlTextDidChange:(NSNotification*)notification;
- (void)controlTextDidBeginEditing:(NSNotification*)notification;

- (void)updateLoginEnabled;
- (IBAction)clearUserNameAndPassword:(id)sender;

- (void)saveUserNamePassword;
- (void)loadUserNamePassword;

- (void)setLanguage;

- (void)patchMounted:(NSNotification*)notification;
- (void)closeInstaller;

- (void)resetSettings:(id)sender;

@end

