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

//  Folder constants
NSString * const WWIIOnlineFolder = @"WWIIOnline";
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

static std::wstring NSStringToWString ( NSString* Str )
{
    NSStringEncoding pEncode    =   CFStringConvertEncodingToNSStringEncoding ( kCFStringEncodingUTF32LE );
    NSData* pSData              =   [ Str dataUsingEncoding : pEncode ];
    
    return std::wstring ( (wchar_t*) [ pSData bytes ], [ pSData length] / sizeof ( wchar_t ) );
}

static NSString* WStringToNSString ( const std::wstring& Str )
{
    //  Convert std::wstring (wwiiolpath_string) to NSString.
    NSString* pString = [ [ NSString alloc ]
                         initWithBytes : (char*)Str.data()
                         length : Str.size() * sizeof(wchar_t)
                         encoding : CFStringConvertEncodingToNSStringEncoding ( kCFStringEncodingUTF32LE ) ];
    return pString;
}

bool CopyOldSetupFiles_MAC()
{
    char buffer [2048] = "";
    char timeStamp [256] = "";
    int charCount = 0;
    
    if (WantedCopyOldSetup == FALSE) { return FALSE; }

    std::wstring tmpBoxTitle = L"Old Settings - Save";
    std::wstring tmpBoxMsg = L"";
    
    const char *title1 = "Old Game settings Located.";
    const char *title2 = "Old Game settings Saved.";
    const char *message1 = "An older version of the Game Setting files has been found.\n\nPlease Note:\nRe-using the old files WILL overwrite any new files if you proceed.\n\nDo you wish to re-use them?";
    //  const char *message2 - Set later

    NSFileManager *fileManager = [NSFileManager defaultManager];
    NSString *documentsDirectory = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) firstObject];
    NSString *bgePath = [documentsDirectory stringByAppendingPathComponent:BGEFolder];
    NSString *ww2Path = [documentsDirectory stringByAppendingPathComponent:WWIIOnlineFolder];
    NSString *bgeCfmlPath = [bgePath stringByAppendingPathComponent:@"cfml"];
    
    NSString *userLibraryDirectory = [NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSUserDomainMask, YES) firstObject];
    NSString *userPrefsDirectory = [userLibraryDirectory stringByAppendingPathComponent:@"Preferences"];
    NSString *userCfmlPath = [userPrefsDirectory stringByAppendingPathComponent:@"ww2olcfml"];
    
    //  NSLog(@"Old Settings - Checking for the presence of Old Game Settings.");
    Write_Debug("Old Settings - Checking for the presence of Old Game Settings.");
    //  dbugLog(_FLI, "Old Settings - Checking for the presence of Old Game Settings.");
    
    
    //  Create a folder copy date/time stamp.  =================================================
    time_t rawtime;
    struct tm * timeinfo;
    
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    timeStamp[0] = 0;
    charCount = sprintf(timeStamp, "_Copied_%04d%02d%02d-%02d%02d%02d", (int)timeinfo->tm_year + 1900, (int)timeinfo->tm_mon + 1, (int)timeinfo->tm_mday, (int)timeinfo->tm_hour , (int)timeinfo->tm_min, (int)timeinfo->tm_sec);
    NSString *folderRename = [NSString stringWithUTF8String:timeStamp];

    const char *c_filepath = [bgePath UTF8String];
    buffer[0] = 0;
    charCount = sprintf (buffer, "The Directory:\n%s\n\nhas been re-named to:\n%s%s.", c_filepath, c_filepath, [folderRename UTF8String]);
    const char *message2 = &buffer[0];  //  <<=== Set.
    
    
    //  Locate: /Users -> "currentuser" -> Documents -> Battleground Europe
    if (![fileManager fileExistsAtPath:bgePath]) {
        //  NSLog(@"Old Settings - Old Game Settings NOT found.");
        Write_Debug("Old Settings - Old Game Settings NOT found.");
        //  dbugLog(_FLI, "Old Settings - Old Game Settings NOT found.");
        return false;
    }

    //  Located: /Users -> "currentuser" -> Documents -> Battleground Europe
    //  NSLog(@"Old Settings - Old Game Settings found.");
    Write_Debug("Old Settings - Old Game Settings found.");
    //  dbugLog(_FLI, "Old Settings - Old Game Settings found.");

    
    //  Verify the User wants to copy the old Setup Files.
    NSAlert *alertVerify = [[NSAlert alloc] init];
    
    alertVerify.messageText = static_cast<NSString * _Nonnull>([NSString stringWithUTF8String:title1] ?: @""); // Force-unwrap those optionals!
    alertVerify.informativeText = static_cast<NSString * _Nonnull>([NSString stringWithUTF8String:message1] ?: @"");
    alertVerify.alertStyle = NSAlertStyleInformational;
    
    [alertVerify addButtonWithTitle:(@"Yes")];
    [alertVerify addButtonWithTitle:(@"No")];
    
    if ([alertVerify runModal] == NSAlertSecondButtonReturn) {
        // the User selected: No.
        
        //  NSLog(@"Old Settings - Old Game Settings found -> the user declined the copy option.");
        Write_Debug("Old Settings - Old Game Settings found -> the user declined the copy option.");
        //  dbugLog(_FLI, "Old Settings - Old Game Settings found -> the user declined the copy option.");
        return false;

    } else {
        // the User selected: Yes WITH Overwriting.
        
        //  NSLog(@"Old Settings - Old Game Settings found -> archiving Mac Preference files.");
        Write_Debug("Old Settings - Old Game Settings found -> archiving Mac Preference files.");
        //  dbugLog(_FLI, "Old Settings - Old Game Settings found -> archiving Mac Preference files.");

        
        //  Archive the Game Preference files (Mac) to the Documents -> Battleground Europe folder.  =================================================

        NSString *target = @"";
        BOOL success = NO;
        NSError * error = nil;

        NSString *sourcePath = @"";
        NSString *destPath = @"";

        if (![fileManager fileExistsAtPath:bgeCfmlPath]) {
            success = [fileManager createDirectoryAtPath:bgeCfmlPath withIntermediateDirectories: true attributes:nil error:&error];
            if(success != YES) {
                char tmpBuffer [2048] = "";
                tmpBoxMsg = L"";
            
                //  NSLog(@"Old Settings - File Error Creating: %@.\n- error: %s\n\nAborting.", bgeCfmlPath, [error localizedDescription]);
                Write_Debug("Old Settings - File Error Creating: %s.\n- error: %s\n\nAborting.", [bgeCfmlPath UTF8String], [[error localizedDescription] UTF8String]);
            
                tmpBuffer[0] = 0;
                charCount = sprintf (tmpBuffer, "File Error - Could not Create: \n- %s.\n\nError:\n- %s\n\nOld Settings Copy Failed.", [bgeCfmlPath UTF8String], [[error localizedDescription] UTF8String]);
                Char2Wstring(tmpBuffer, tmpBoxMsg, (size_t)charCount);
                
                MessageBoxMAC(tmpBoxMsg, tmpBoxTitle, 0);
                
                tmpBuffer[0] = 0;
            
                return false;
            }
        }

        //  Copy AND Re-Name the Users Current Preferences (Mac) cfml Directory:
        //   /Users -> "currentuser" -> Library -> Preferences -> ww2olcfml -> *.*
        //   TO
        //   /Users -> "currentuser" -> Documents -> Battleground Europe -> cfml
        NSDirectoryEnumerator *cfmlDirEnum = [fileManager enumeratorAtPath:userCfmlPath];
        target = @"";
        success = NO;
        error = nil;

        while ((target = [cfmlDirEnum nextObject])) {
            sourcePath = [userCfmlPath stringByAppendingPathComponent:target];
            destPath = [bgeCfmlPath stringByAppendingPathComponent:target];
            
            if ([fileManager fileExistsAtPath:destPath]) {
                success = [fileManager removeItemAtPath:destPath error:&error];
                if(success != YES) {
                    char tmpBuffer [2048] = "";
                    tmpBoxMsg = L"";
                    
                    //  NSLog(@"Old Settings - Could not delete:\n- to path: %s.\n- error: %s\n\nAborting.", destPath, error);
                    Write_Debug("Old Settings - Could not delete:\n- to path: %s.\n- error: %s\n\nAborting.",[destPath UTF8String], [[error localizedDescription] UTF8String]);

                    tmpBuffer[0] = 0;
                    charCount = sprintf (tmpBuffer, "File Error - Could not delete:\n- to path: %s.\n- error: %s\n\n\nOld Settings Copy Failed.", [bgeCfmlPath UTF8String], [[error localizedDescription] UTF8String]);
                    Char2Wstring(tmpBuffer, tmpBoxMsg, (size_t)charCount);
                    
                    MessageBoxMAC(tmpBoxMsg, tmpBoxTitle, 0);
                    
                    tmpBuffer[0] = 0;
                    
                    return false;
                } else {
                    //  NSLog(@"Old Settings - Existing File Overwritten: %s.", destPath);
                    Write_Debug("Old Settings - Existing File Overwritten: %s.",[destPath UTF8String]);

                }
            }

            success = [fileManager copyItemAtPath:sourcePath toPath:destPath error:&error];
            if(success != YES) {
                char tmpBuffer [2048] = "";
                tmpBoxMsg = L"";
                
                //  NSLog(@"Old Settings - Could not copy:\n- from path: %@,\n- to path: %@.\n- error: %@\n\nAborting.",sourcePath, destPath, error);
                Write_Debug("Old Settings - Could not copy:\n- from path: %s,\n- to path: %s.\n- error: %s\n\nAborting.", [sourcePath UTF8String], [destPath UTF8String], [[error localizedDescription] UTF8String]);

                tmpBuffer[0] = 0;
                charCount = sprintf (tmpBuffer, "File Error - Could not copy:\n- from path: %s,\n- to path: %s.\n- error: %s\n\n\nOld Settings Copy Failed.", [sourcePath UTF8String], [destPath UTF8String], [[error localizedDescription] UTF8String]);
                Char2Wstring(tmpBuffer, tmpBoxMsg, (size_t)charCount);
                
                MessageBoxMAC(tmpBoxMsg, tmpBoxTitle, 0);
                
                tmpBuffer[0] = 0;
                
                return false;
            }

            success = NO;
            error = nil;
            sourcePath = @"";
        }
        
        //  Copy AND Re-Name the Users Current Game Preferences (Mac) File:
        //   /Users -> "currentuser" -> Library -> Preferences -> ww2olwwiiolprefs.xml
        //   TO
        //   /Users -> "currentuser" -> Documents -> Battleground Europe -> wwiiol.xml
        success = NO;
        error = nil;

        sourcePath = [userPrefsDirectory stringByAppendingPathComponent:@"/ww2olwwiiolprefs.xml"];
        destPath = [bgePath stringByAppendingPathComponent:@"/wwiiol.xml"];
        
        if ([fileManager fileExistsAtPath:destPath]) {
            success = [fileManager removeItemAtPath:destPath error:&error];
            if(success != YES) {
                char tmpBuffer [2048] = "";
                tmpBoxMsg = L"";
                
                //  NSLog(@"Old Settings - Could not delete:\n- to path: %s.\n- error: %s\n\nAborting.", destPath, error);
                Write_Debug("Old Settings - Could not delete:\n- to path: %s.\n- error: %s\n\nAborting.",[destPath UTF8String], [[error localizedDescription] UTF8String]);
                
                tmpBuffer[0] = 0;
                charCount = sprintf (tmpBuffer, "File Error - Could not delete:\n- to path: %s.\n- error: %s\n\n\nOld Settings Copy Failed.", [destPath UTF8String], [[error localizedDescription] UTF8String]);
                Char2Wstring(tmpBuffer, tmpBoxMsg, (size_t)charCount);
                
                MessageBoxMAC(tmpBoxMsg, tmpBoxTitle, 0);
                
                tmpBuffer[0] = 0;
                
                return false;
            } else {
                //  NSLog(@"Old Settings - Existing File Overwritten: %s.", destPath);
                Write_Debug("Old Settings - Existing File Overwritten: %s.",[destPath UTF8String]);

            }
        }

        success = [fileManager copyItemAtPath:sourcePath toPath:destPath error:&error];
        if(success != YES) {
            char tmpBuffer [2048] = "";
            tmpBoxMsg = L"";
            
            //  NSLog(@"Old Settings - Could not copy:\n- from path: %@,\n- to path: %@.\n- error: %@\n\nAborting.",sourcePath, destPath, error);
            Write_Debug("Old Settings - Could not copy:\n- from path: %s,\n- to path: %s.\n- error: %s\n\nAborting.",[sourcePath UTF8String], [destPath UTF8String], [[error localizedDescription] UTF8String]);
            
            tmpBuffer[0] = 0;
            charCount = sprintf (tmpBuffer, "File Error - Could not copy:\n- from path: %s,\n- to path: %s.\n- error: %s\n\n\nOld Settings Copy Failed.",[sourcePath UTF8String], [destPath UTF8String], [[error localizedDescription] UTF8String]);
            Char2Wstring(tmpBuffer, tmpBoxMsg, (size_t)charCount);
            
            MessageBoxMAC(tmpBoxMsg, tmpBoxTitle, 0);
            
            tmpBuffer[0] = 0;
            
            return false;
        }

        
        //  NSLog(@"Old Settings - Old Game Settings found -> copying Battleground Europe files.");
        Write_Debug("Old Settings - Old Game Settings found -> copying Battleground Europe files.");
        //  dbugLog(_FLI, "Old Settings - Old Game Settings found -> copying Battleground Europe files.");

        
        //  Copy the ENTIRE Contents of (including the re-formatted preference files):
        //   /Users -> "currentuser" -> Documents -> Battleground Europe
        //   TO
        //   /Users -> "currentuser" -> Documents -> WWIIOnline.
        NSDirectoryEnumerator *dirEnum = [fileManager enumeratorAtPath:bgePath];
        success = NO;
        error = nil;
        
        while ((target = [dirEnum nextObject])) {
            sourcePath = [bgePath stringByAppendingPathComponent:target];
            destPath = [ww2Path stringByAppendingPathComponent:target];
            
            if ([fileManager fileExistsAtPath:destPath]) {
                success = [fileManager removeItemAtPath:destPath error:&error];
                if(success != YES) {
                    char tmpBuffer [2048] = "";
                    tmpBoxMsg = L"";
                    
                    //  NSLog(@"Old Settings - Could not delete:\n- to path: %s.\n- error: %s\n\nAborting.", destPath, error);
                    Write_Debug("Old Settings - Could not delete:\n- to path: %s.\n- error: %s\n\nAborting.",[destPath UTF8String], [[error localizedDescription] UTF8String]);
                    
                    tmpBuffer[0] = 0;
                    charCount = sprintf (tmpBuffer, "File Error - Could not delete:\n- to path: %s.\n- error: %s\n\n\nOld Settings Copy Failed.", [bgeCfmlPath UTF8String], [[error localizedDescription] UTF8String]);
                    Char2Wstring(tmpBuffer, tmpBoxMsg, (size_t)charCount);
                    
                    MessageBoxMAC(tmpBoxMsg, tmpBoxTitle, 0);
                    
                    tmpBuffer[0] = 0;
                    
                    return false;
                } else {
                    //  NSLog(@"Old Settings - Existing File Overwritten: %s.", destPath);
                    Write_Debug("Old Settings - Existing File Overwritten: %s.",[destPath UTF8String]);

                }
            }
            
            success = [fileManager copyItemAtPath:sourcePath toPath:destPath error:&error];
            if(success != YES) {
                char tmpBuffer [2048] = "";
                tmpBoxMsg = L"";
                
                //  NSLog(@"Old Settings - Could not copy:\n- from path: %@,\n- to path: %@.\n- error: %@\n\nAborting.",sourcePath, destPath, error);
                Write_Debug("Old Settings - Could not copy:\n- from path: %s,\n- to path: %s.\n- error: %s\n\nAborting.", [sourcePath UTF8String], [destPath UTF8String], [[error localizedDescription] UTF8String]);
                
                tmpBuffer[0] = 0;
                charCount = sprintf (tmpBuffer, "File Error - Could not copy:\n- from path: %s,\n- to path: %s.\n- error: %s\n\n\nOld Settings Copy Failed.", [sourcePath UTF8String], [destPath UTF8String], [[error localizedDescription] UTF8String]);
                Char2Wstring(tmpBuffer, tmpBoxMsg, (size_t)charCount);
                
                MessageBoxMAC(tmpBoxMsg, tmpBoxTitle, 0);
                
                tmpBuffer[0] = 0;
                
                return false;
            }

            success = NO;
            error = nil;
            sourcePath = @"";
        }
        
        //  Copy Completed.
        
        
        //  Re-Name:
        //   /Users -> "currentuser" -> Documents -> "Battleground Europe"
        //   TO
        //   /Users -> "currentuser" -> Documents -> "Battleground Europe_Copied".
        //  NSLog(@"Old Settings - Old Game Settings found -> re-naming the Original Battleground Europe Directory.");
        Write_Debug("Old Settings - Old Game Settings found -> re-naming the Original Battleground Europe Directory.");
        //  dbugLog(_FLI, "Old Settings - Old Game Settings found -> re-naming the Original Battleground Europe Directory.");


        NSURL *oldDir = [NSURL fileURLWithPath:bgePath];
        NSURL *newDir = [NSURL fileURLWithPath:[bgePath stringByAppendingString:folderRename]];
        success = [fileManager moveItemAtURL: oldDir toURL: newDir error:&error];
        if(success != YES) {
            char tmpBuffer [2048] = "";
            tmpBoxMsg = L"";
            
            //  NSLog(@"Old Settings - Could not rename:\nfrom path: %@,\nto path: %@\n- error: %s\n",oldDir, newDir, error);
            Write_Debug("Old Settings - Could not rename:\nfrom path: %@,\nto path: %@.\n- error: %s\n", oldDir, newDir, [[error localizedDescription] UTF8String]);
            
            tmpBuffer[0] = 0;
            charCount = sprintf (tmpBuffer, "File Error - Could not rename:\n- from path: %s,\n- to path: %s.\n- error: %s\n\nPlease contact Support to resolve this issue.", [sourcePath UTF8String], [destPath UTF8String], [[error localizedDescription] UTF8String]);
            Char2Wstring(tmpBuffer, tmpBoxMsg, (size_t)charCount);
            
            MessageBoxMAC(tmpBoxMsg, tmpBoxTitle, 0);
            
            tmpBuffer[0] = 0;

        } else {
            //  NSLog(@"Old Settings - Old Game Settings found -> Old Game Settings file copy and re-naming completed.");
            Write_Debug("Old Settings - Old Game Settings found -> Old Game Settings file copy and re-naming completed.");
            //  dbugLog(_FLI, "Old Settings - Old Game Settings found -> Old Game Settings file copy and re-naming completed.");
            
//            NSString *titleText = static_cast<NSString * _Nonnull>([NSString stringWithUTF8String:title2] ?: @""); // Force-unwrap those optionals!
//            NSString *messageText = static_cast<NSString * _Nonnull>([NSString stringWithUTF8String:message2] ?: @"");
            
//            tmpBoxTitle = NSStringToWString(titleText);
//            tmpBoxMsg = NSStringToWString(messageText);

//            MessageBoxMAC(tmpBoxMsg, tmpBoxTitle, 0);

            std::wstring tmp1(&message2[0], &message2[strlen(message2)]);
            std::wstring tmp2(&title2[0], &title2[strlen(title2)]);

            MessageBoxMAC(tmp1, tmp2, 0);
        }

        error = nil;
        //  Settings Copy Completed.
    }
    
    return true;
}

int MessageBoxMAC(std::wstring boxMessage, std::wstring boxTitle, int alertIcon = 0)
{
    //  NSAlertStyleWarning = 0,
    //  NSAlertStyleInformational = 1,
    //  NSAlertStyleCritical = 2
    
    NSAlert *alertMessageBox = [[NSAlert alloc] init];
    
    alertMessageBox.messageText = WStringToNSString(boxTitle);
    alertMessageBox.informativeText = WStringToNSString(boxMessage);
    
    switch (alertIcon)
    {
        case 1:
            alertMessageBox.alertStyle = NSAlertStyleInformational;
            break;
        case 2:
            alertMessageBox.alertStyle = NSAlertStyleCritical;
            break;
        default:
            //  case 0
            alertMessageBox.alertStyle = NSAlertStyleWarning;
            break;
    }
    
    [alertMessageBox runModal];
    
    alertMessageBox = nil;
    
    return 0;
}

const char* macPreferencesPath()
{
    //  XXXX - /Users -> "currentuser" -> Library -> Preferences -> ww2ol
    //  XXXX - /Users -> "currentuser" -> Library -> Preferences -> ww2olcfml
    //  XXXX - /Users -> "currentuser" -> Library -> Preferences -> ww2olwwiiolprefs.xml
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
    targetPathFSR = [main.bundlePath UTF8String];

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
    //  XXXX - /Library -> Application Support -> WWIIOnline
    //  XXXX - /Library -> Application Support -> com.corneredrats.wwiionline
    //  XXXX - /Users -> Shared -> Library -> Application Support -> WWIIOnline
    //  XXXX - /Users -> Shared -> Library -> Application Support -> com.corneredrats.wwiionline
    //  XXXX - /Users -> "currentuser" -> Library -> Application Support -> WWIIOnline
    //  /Users -> "currentuser" -> Library -> Application Support -> com.corneredrats.wwiionline

    static NSURL *targetPathDir = nil;
    static const char *targetPathFSR = nullptr;
    static dispatch_once_t predicate = 0;
    
    dispatch_once(&predicate, ^ {
        @autoreleasepool {
            targetPathDir = [[NSFileManager defaultManager]
                             URLForDirectory:NSApplicationSupportDirectory
                             //  inDomain:NSLocalDomainMask
                             inDomain:NSUserDomainMask
                             appropriateForURL:nil
                             create:YES
                             error:nullptr];
            //  targetPathDir = [targetPathDir URLByAppendingPathComponent:WWIIOnlineFolder];
			targetPathDir = [targetPathDir URLByAppendingPathComponent:NSBundle.mainBundle.bundleIdentifier];

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
    //  XXXX - /Library -> Application Support -> WWIIOnline -> CrashLogs
	//  XXXX - /Library -> Application Support -> com.corneredrats.wwiionline -> CrashLogs
    //  XXXX - /Users -> "currentuser" -> Library -> Application Support -> WWIIOnline -> CrashLogs
    //  XXXX - /Users -> "currentuser" -> Library -> Application Support -> com.corneredrats.wwiionline -> CrashLogs
    //
    //  NOTICE: This function is not currently implemented - 13 July 2019.
    //

    static NSURL *targetPathDir = nil;
    static const char *targetPathFSR = nullptr;
    static dispatch_once_t predicate = 0;

    dispatch_once(&predicate, ^ {
        @autoreleasepool {
            targetPathDir = [[NSFileManager defaultManager]
                             URLForDirectory:NSApplicationSupportDirectory
                             //  inDomain:NSLocalDomainMask
							 inDomain:NSUserDomainMask
                             appropriateForURL:nil
                             create:YES
                             error:nullptr];
            //  targetPathDir = [targetPathDir URLByAppendingPathComponent:WWIIOnlineFolder];
			targetPathDir = [targetPathDir URLByAppendingPathComponent:NSBundle.mainBundle.bundleIdentifier];
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

const char* macUserScreenShotsPath()
{
    //  /Users -> "currentuser" -> Documents -> WWIIOnline -> screenshots
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
            targetPathDir = [targetPathDir URLByAppendingPathComponent:@"screenshots"];
            
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
