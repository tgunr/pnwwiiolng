#import <Cocoa/Cocoa.h>
//#import <WebKit/WebKit.h>

#include "teulauth.h"
#include "auth/authClientMessages.h"
#include "auth/authTypes.h"

// TODO: Remove this
#pragma clang diagnostic ignored "-Wdirect-ivar-access"

@interface WW2AppDelegate : NSObject <NSTextFieldDelegate>
{
    IBOutlet NSPopUpButton* soundType;
    IBOutlet NSButton* capAllDisplays;
    IBOutlet NSButton* calibrate1;
    IBOutlet NSButton* calibrate2;
    IBOutlet NSButton* calibrate3;
    IBOutlet NSButton* calibrate4;
    IBOutlet NSPopUpButton* colorDepth;
    IBOutlet NSPopUpButton* fsaa;
    IBOutlet NSWindow* configWindow;
    IBOutlet NSPopUpButton *connection;
    IBOutlet NSPopUpButton* display;
    IBOutlet id downloadBox;
    IBOutlet id downloadProgressBar;
    IBOutlet id downloadStatus;
    IBOutlet NSPopUpButton* joy1;
    IBOutlet NSPopUpButton* joy2;
    IBOutlet NSPopUpButton* joy3;
    IBOutlet NSPopUpButton* joy4;
    IBOutlet NSWindow *launcherWindow;
    IBOutlet NSTextField *playgateStatus;
    IBOutlet NSPopUpButton *refresh;
    IBOutlet NSButton *rememberPasswordCheckbox;
    IBOutlet NSPopUpButton* resolution;
    IBOutlet NSProgressIndicator *spinner;
    IBOutlet NSWindow *statusWindow;
    IBOutlet NSTextField *userNameField;
    IBOutlet NSSecureTextField *passwordField;
    IBOutlet id webView;
    IBOutlet NSButton *playOnlineButton;
    IBOutlet NSPopUpButton *arenaMenu;
    IBOutlet NSProgressIndicator *arenaLoadingIndicator;

    float downloadBoxResizeHeight;
    BOOL isHidingDownloadProgress;
    NSDate* lastStatusUpdateTime;

    int state;

    NSData* sourcePSN;
    int arena;
    int session;
    BOOL rememberUserPassword;

    NSString* authServer;
    UINT16 authPort;
    SINT32 teulConnectionIndex;
    BOOL isConnectedToAuthenticationServer;
    BOOL isteulInitialized;
    dispatch_source_t teulTimer;

    APP_LAUNCH authReply;
    CGOpenGLDisplayMask selectedMonitor;
    NSDictionary* selectedMode;
    NSTimer* hidTimer;
    
    BOOL _passwordEntryIsInitial;
    
    NSURL *_patchFileURL;
    NSError *_error;
    NSString *_userName;
    NSString *_userPassword;
    NSURLSessionDownloadTask *_patchDownloadTask;
    
    NSURLSession *_patchDownloadSession;
    NSByteCountFormatter *_downloadProgressFormatter;
}

- (IBAction)cancelDownload:(id)sender;
- (IBAction)fsaaMenuChanged:(id)sender;
- (IBAction)soundMenuChanged:(id)sender;
- (IBAction)depthMenuChanged:(id)sender;
- (IBAction)displayMenuChanged:(id)sender;
- (IBAction)doCalibrate:(id)sender;
- (IBAction)doCancel:(id)sender;
- (IBAction)doOK:(id)sender;
- (IBAction)joyChanged:(id)sender;
- (IBAction)launchOffline:(id)sender;
- (IBAction)launchOnline:(id)sender;
- (IBAction)openSettings:(id)sender;
- (IBAction)refreshMenuChanged:(id)sender;
- (IBAction)resolutionMenuChanged:(id)sender;

@property(nonatomic,strong) NSURL *patchFileURL;
@property(nonatomic,strong) NSError *error;
@property(nonatomic,copy) NSString *userName;
@property(nonatomic,copy) NSString *userPassword;
@property(nonatomic,strong) NSURLSessionDownloadTask *patchDownloadTask;

- (void)setCaptureAllDisplays:(bool)x;
- (void)setFSAA:(int)fs;
- (void)setSoundQuality:(int)sq;

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

- (void)goHome;

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

- (void)resetSettings:(id)sender;

- (void)setAuthReply:(APP_LAUNCH *)reply;

@end

