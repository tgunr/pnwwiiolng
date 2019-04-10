; BOF ========================================================================
; File: WWIIOL_Installer_Template.iss
; Software: Inno Setup Compiler 5.6.1 (u)
; Date: April 8, 2018, April 3, 2019
; Revision by: Mathew "james10" Hogan - MJH

; Addresses:
; DEV-1383: The Windows Installer is still referencing Battleground Europe
; and not World War II Online.

; Also provides the installer functionality to copy the Settings folder to
; another directory as part of the setup process. Part of DEV-1383.

; Setup the possibility of additional installer builds for different purposes
; using a common installer script template.

; ============================================================================


; ===  Editable Script Options.  =============================================

; ===  Notes:
; ===  * Un-comment only One Option.
; ===  * If you Un-comment more than one in any group, ONLY the LAST ONE in the list will be used.
; ===  * Is no option for a option group is selected, the Default will be used.


; ===  Release Type. (Un-comment only One)  ==================================
; #define ReleaseType "ReleaseCandidate"    ;Default
#define ReleaseType "Alpha"
; #define ReleaseType "Beta"


; ===  Installer Build Type. (Un-comment only One)  ==========================
#define InstallerBuildType "installFull"    ;Default
; #define InstallerBuildType "installPatcher"
; #define InstallerBuildType "installRollback"


; ===  Use the Playgate Launcher. (Un-comment only One)  =====================
#define UsePlaygate "yes"    ;Default
; #define UsePlaygate "no"


; ===  Allow Patching. (Un-comment only One)  ================================
#define EnablePatching "yes"    ;Default
; #define EnablePatching "no"


; ===  Script Debug. (Un-comment only One)  ==================================
; #define ScriptDebug "yes"
#define ScriptDebug "no"    ;Default


; ===  Should not edit below this line.   ====================================
; ============================================================================
; ============================================================================


; ===  Game Install Root Names.  =============================================
#define RootNameBGE "Battleground Europe"
#define RootNameWWIIOnline "WWIIOnLine"


; ===  Program Variables: Exe Files - World War II Online.  ==================
#define WWIIOnlineLauncher "Playgate.exe"
#define WWIIOnlineExe "WW2.exe"
#define WWIIOnlineProperties "Settings.exe"
#define WWIIOnlinePatchingTool "xdelta3.exe"
#define WWIIOnlinePropertiesFile "wwiiol.xml"
#define MSDirectXSetup "data\DirectXRedist\DXSetup.exe"
#define MSRuntimeLibraryX64 "data\RuntimeLibraries\vc_redist.x64.exe"
#define MSRuntimeLibraryX86 "data\RuntimeLibraries\vc_redist.x86.exe"
#define PlaygateValuePrams "-serverip $QUAD -serverport $PORT -serverotp $OTP -side $SESSIONID"
#define PlaygateValueRegion "us"


; ===  Program Variables: URL - World War II Online.  ========================
#define InstallerWebsiteURL "https://www.wwiionline.com"
#define InstallerSupportURL "https://support.wwiionline.com"
#define InstallerUpdatesURL "https://www.wwiionline.com/getting-started#sign-up-download"
#define InstallerManualURL "https://wiki.wwiionline.com"
#define InstallerTrainingURL "https://wiki.wwiionline.com/view/Training"
#define InstallerAccountsURL "https://www.wwiionline.com/account"


; ===  Installer Variables.   ================================================
#define InstallerExcludePatterns "*.gitignore, *.placeholder, *.DS_Store"
#define InstallerSoftwareVersion "1.0.0.0"
#define InstallerPublisher "Playnet Inc / CRS."
#define InstallerCopyright "© 1999-2019, Playnet Inc / CRS."
#define InstallerGroupName "Cornered Rat Software"
#define InstallerRootPathName "CRS"
#define InstallerBaseFileName "wwiiol_"
#define InstallerIconFileName "WWIIOnLine.ico"
#define InstallerImageFile "WIZMODERNIMAGE.BMP"
#define InstallerImageFileSmall "WIZMODERNSMALLIMAGE.BMP"

#define WWIIOnlineLicenseFile "license.rtf"
#define WWIIOnlineReadMeFile "readme.rtf"

#if UsePlaygate == "no"
  #define InstallerExeName WWIIOnlineExe

#else
  ; Default
  #define InstallerExeName WWIIOnlineLauncher

#endif


; ===  Release Type Setup:  ==================================================

#if ReleaseType == "Alpha"
; ===  Release Type - Alpha:  ================================================
  ; World War II Online Application ID.
  #define WWIIOnlineID "12"
  #define UseSignTool "no"

  #if InstallerBuildType == "installPatcher"
    #define WWIIOnlineInstallVersionType " - Alpha Patch"

  #else
    #define WWIIOnlineInstallVersionType " - Alpha"

  #endif

  #define WWIIOnlineAppVerName " - Alpha"

  ; Destination Folder - World War II Online installer.
  #define FileOutputPath "release-alpha"
  
  ; Source files folders.
  #define CurrentFilesRoot "release-alpha\current"
  #define PreviousFilesRoot "release-alpha\previous"
  #define PatchFilesRoot "release-alpha\diff"


#elif ReleaseType == "Beta"
; ===  Release Type - Beta:  =================================================
  ; World War II Online Application ID.
  #define WWIIOnlineID "13"
  #define UseSignTool "no"

  #if InstallerBuildType == "installPatcher"
    #define WWIIOnlineInstallVersionType " - Beta Patch"

  #else
    #define WWIIOnlineInstallVersionType " - Beta"

  #endif

  #define WWIIOnlineAppVerName " - Beta"

  ; Destination Folder - World War II Online installer.
  #define FileOutputPath "release-beta"

  ; Source files folders.
  #define CurrentFilesRoot "release-beta\current"
  #define PreviousFilesRoot "release-beta\previous"
  #define PatchFilesRoot "release-beta\diff"


#else
; ===  Release Type - Release Candidate - Default:  ==========================
  ; ReleaseType == "ReleaseCandidate"

  ; World War II Online Application ID.
  #define WWIIOnlineID "1"

  #if ScriptDebug == "yes"
    #define UseSignTool "no"

  #else
    #define UseSignTool "yes"

  #endif  

  #if InstallerBuildType == "installPatcher"
    #define WWIIOnlineInstallVersionType " - RC Patch"

  #else
    #define WWIIOnlineInstallVersionType " - RC"

  #endif

  ; uses NO App Version Name Extension.
  #define WWIIOnlineAppVerName ""

  ; Destination Folder - World War II Online installer.
  #define FileOutputPath "release-candidate"  

  ; Source files folders.
  #define CurrentFilesRoot "release-candidate\current"
  #define PreviousFilesRoot "release-candidate\previous"
  #define PatchFilesRoot "release-candidate\diff"

#endif


; ===  Installer Assets Directories.  ========================================
#define WWIIOnlineAssetsDir "..\.\ww2assets"


; ===  Set - Installer Script Format.
#define WWIIOnlineAppName "World War II Online"
#define WWIIOnlineDirName "WWIIOnLine"
#define WWIIOnlineGroupName "World War II Online"
#define InstallerVersionInfoDescription "World War II Online - Setup"


; ===  Set - Installer Versioning.  ==========================================
#if InstallerBuildType == "installPatcher"
  #define WWIIOnlineInstallVersion GetFileVersion( CurrentFilesRoot + "\" + WWIIOnlineExe )
  #define WWIIOnlineRollbackVersion "None"
  #define WWIIOnlinePreviousVersion GetFileVersion( PreviousFilesRoot + "\" + WWIIOnlineExe )

#elif InstallerBuildType == "installRollback"
  #define WWIIOnlineInstallVersion GetFileVersion( PreviousFilesRoot + "\" + WWIIOnlineExe )
  #define WWIIOnlineRollbackVersion GetFileVersion( CurrentFilesRoot + "\" + WWIIOnlineExe )
  #define WWIIOnlinePreviousVersion "None"

#else
  ; InstallerBuildType == "installFull" - Default
  #define WWIIOnlineInstallVersion GetFileVersion( CurrentFilesRoot + "\" + WWIIOnlineExe )
  #define WWIIOnlineRollbackVersion "None"
  #define WWIIOnlinePreviousVersion "None"

#endif


; ===  Start Installer Setup.  ===============================================

[Setup]
#if UseSignTool == "yes"
  SignedUninstaller=yes
  SignTool=signtool /fd sha256 /d $qBattleground Europe$q /du $qhttp://www.battlegroundeurope.com$q $f

#else
  SignedUninstaller=no
  
#endif


; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)

#if WWIIOnlineID == "12"
  ; ReleaseType == "Alpha"
  AppId={{1AE85CC9-7D09-475A-ACBB-AAD74D035701}

#elif WWIIOnlineID == "13"
  ; ReleaseType == "Beta"
  AppId={{1D81F141-0A8A-4D76-A235-A02FC78A212C}

#else
  ; WWIIOnlineID == "1"
  ; ReleaseType == "ReleaseCandidate"
  AppId={{BCA70BBE-EACC-49F4-AC9A-1DFE4E55F739}

#endif


; WWIIOnline Installer Variables.  ===========================================
AppName={#WWIIOnlineAppName}
AppVersion={#WWIIOnlineInstallVersion}
AppVerName={#WWIIOnlineAppName}{#WWIIOnlineAppVerName}
AppPublisher={#InstallerPublisher}
AppPublisherURL={#InstallerWebsiteURL}
AppSupportURL={#InstallerSupportURL}
AppUpdatesURL={#InstallerUpdatesURL}
AppCopyright={#InstallerCopyright}
UninstallDisplayIcon={app}\{#InstallerIconFileName}
; AppComments={#InstallerWebsiteURL}
AppComments=The Virtual Battlefield Does Exist . . . !


; WWIIOnline Application - Installer Variables.  =============================
#if ReleaseType == "Alpha" | ReleaseType == "Beta"
  DefaultDirName={pf}\{#InstallerRootPathName}\{#WWIIOnlineDirName}{#WWIIOnlineAppVerName}
  DefaultGroupName={#InstallerGroupName}\{#WWIIOnlineGroupName}{#WWIIOnlineAppVerName}

  #if InstallerBuildType == "installPatcher"
    OutputBaseFilename={#InstallerBaseFileName}{#WWIIOnlinePreviousVersion}-{#WWIIOnlineInstallVersion}{#WWIIOnlineInstallVersionType}

  #else
    OutputBaseFilename={#InstallerBaseFileName}{#WWIIOnlineInstallVersion}{#WWIIOnlineInstallVersionType}

  #endif

#else
  ; ReleaseType == "ReleaseCandidate" - Default
  DefaultDirName={pf}\{#InstallerRootPathName}\{#WWIIOnlineDirName}
  DefaultGroupName={#InstallerGroupName}\{#WWIIOnlineGroupName}

  #if InstallerBuildType == "installPatcher"
    OutputBaseFilename={#InstallerBaseFileName}{#WWIIOnlinePreviousVersion}-{#WWIIOnlineInstallVersion}

  #else
    OutputBaseFilename={#InstallerBaseFileName}{#WWIIOnlineInstallVersion}

  #endif
#endif


; WWIIOnline - Sources.
; The path to where this script file is located.
; SourceDir={#SourcePath}


; WWIIOnline - Installer software Destination.
; The path relative to the to where this script file is located.
OutputDir={#FileOutputPath}


; Installer - Options
Compression=lzma
SolidCompression=yes
; SolidCompression=no
PrivilegesRequired=admin
AppendDefaultDirName=False
AllowRootDirectory=True
MinVersion=6.0

#if InstallerBuildType == "installPatcher"
  UsePreviousAppDir=yes
  CreateUninstallRegKey=no
  UpdateUninstallLogAppName=no

#else
  UsePreviousAppDir=no
  CreateUninstallRegKey=yes
  UpdateUninstallLogAppName=yes

#endif

     
; Installer Resources.
SetupIconFile={#CurrentFilesRoot}\{#InstallerIconFileName}
LicenseFile={#CurrentFilesRoot}\docs\{#WWIIOnlineLicenseFile}
InfoBeforeFile={#CurrentFilesRoot}\{#WWIIOnlineReadMeFile}
WizardImageFile={#WWIIOnlineAssetsDir}\oswin\{#InstallerImageFile}
WizardSmallImageFile={#WWIIOnlineAssetsDir}\oswin\{#InstallerImageFileSmall}


; Installer - Variables
VersionInfoVersion={#InstallerSoftwareVersion}
VersionInfoCompany={#InstallerPublisher}
VersionInfoDescription={#InstallerVersionInfoDescription}
VersionInfoTextVersion={#InstallerSoftwareVersion}
VersionInfoCopyright={#InstallerCopyright}
VersionInfoProductName={#WWIIOnlineAppName}
VersionInfoProductVersion={#WWIIOnlineInstallVersion}
VersionInfoProductTextVersion={#WWIIOnlineInstallVersion}{#WWIIOnlineInstallVersionType}


[Messages]
ExitSetupTitle={#WWIIOnlineAppName} - Exit Setup
SelectLanguageTitle={#WWIIOnlineAppName} - Setup Language
SetupAppTitle={#WWIIOnlineAppName} - Setup. {#WWIIOnlineInstallVersion}{#WWIIOnlineInstallVersionType}
SetupWindowTitle={#WWIIOnlineAppName} - Setup


[CustomMessages]
AdditionalSettings=Additional Options:
ExtInstallers=External Installers:
InstallDX=Install DirectX Redistributable Files
RunTimeX64=Install Visual C++ Runtime files for x64 Windows
RunTimeX86=Install Visual C++ Runtime files for x86 Windows

; Startmenu shortcuts.
WWIIolLauncher={#WWIIOnlineAppName} - Launcher
WWIIolSettings={#WWIIOnlineAppName} - Settings
WWIIolPractice={#WWIIOnlineAppName} - Practice Offline
WWIIolUninstall={#WWIIOnlineAppName} - Uninstall
WWIIolTraining={#WWIIOnlineAppName} - Online Training Manual

; Message Boxes.
AboutButtonText={#InstallerVersionInfoDescription} - File Information.%n%nSoftware Version -- {#WWIIOnlineInstallVersion}{#WWIIOnlineInstallVersionType}.%nDeveloper: -- Cornered Rat Software, (LLC).%nPublisher: -- Playnet, Inc.
PreviousInstall={#InstallerVersionInfoDescription}. . .  is unable to continue.%n%nPlease Uninstall the crrently installed version,%nthen Re-run this Installer.
RequireFullInstall={#InstallerVersionInfoDescription}. . .  is unable to continue.%n%nThis Patcher  REQUIRES  an installed version to patch.%n%nPlease download and run the Current Full Installer.
IncompatablePatch={#InstallerVersionInfoDescription}. . .  is unable to continue.%n%nThis Patcher is incompatable with your installed version.%n%nPlease Uninstall your installed version,%nthen download and run the Current Full Installer.
SavedSettingsFound=An older version of the Game Settings has been found%n%nDo you wish to re-use them?
RenamedDIR=The Directory:%n%1 has been re-named to:%n%2_Copied_%3.

[InstallDelete]

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "french"; MessagesFile: "compiler:Languages\French.isl"
Name: "german"; MessagesFile: "compiler:Languages\German.isl"
Name: "italian"; MessagesFile: "compiler:Languages\Italian.isl"
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"
; Name: "chinese"; MessagesFile: "{#MyAssetsDir}\Chinese-Simpl.isl"


[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalSettings}"; Flags: unchecked
Name: "quicklaunchicon"; Description: "{cm:CreateQuickLaunchIcon}"; GroupDescription: "{cm:AdditionalSettings}"; Flags: unchecked; OnlyBelowVersion: 0,6.1
Name: "DirectXRedist"; Description: "{cm:InstallDx}"; GroupDescription: "{cm:ExtInstallers}"
; Name: "VCRedistX64"; Description: "{cm:RunTimeX64}"; GroupDescription: "{cm:ExtInstallers}"; Check: IsWin64
Name: "VCRedistX64"; Description: "{cm:RunTimeX64}"; GroupDescription: "{cm:ExtInstallers}"
Name: "VCRedistX86"; Description: "{cm:RunTimeX86}"; GroupDescription: "{cm:ExtInstallers}"


[Files]
#if InstallerBuildType == "installPatcher"
  ; PlayArc Files.
  Source: "{#WWIIOnlineAssetsDir}\oswin\{#WWIIOnlinePatchingTool}"; DestDir: "{app}"; Flags: ignoreversion deleteafterinstall
  Source: "{#PatchFilesRoot}\playarc01.ww2.patch"; DestDir: "{app}\playarc"; BeforeInstall: BackupPatchTgt('playarc\playarc01.ww2'); Flags: ignoreversion deleteafterinstall
  Source: "{#PatchFilesRoot}\playarc02.ww2.patch"; DestDir: "{app}\playarc"; BeforeInstall: BackupPatchTgt('playarc\playarc02.ww2'); Flags: ignoreversion deleteafterinstall
  Source: "{#PatchFilesRoot}\playarc03.ww2.patch"; DestDir: "{app}\playarc"; BeforeInstall: BackupPatchTgt('playarc\playarc03.ww2'); Flags: ignoreversion deleteafterinstall
  Source: "{#PatchFilesRoot}\playarc04.ww2.patch"; DestDir: "{app}\playarc"; BeforeInstall: BackupPatchTgt('playarc\playarc04.ww2'); Flags: ignoreversion deleteafterinstall
  Source: "{#PatchFilesRoot}\playarc05.ww2.patch"; DestDir: "{app}\playarc"; BeforeInstall: BackupPatchTgt('playarc\playarc05.ww2'); Flags: ignoreversion deleteafterinstall
  Source: "{#PatchFilesRoot}\playarc06.ww2.patch"; DestDir: "{app}\playarc"; BeforeInstall: BackupPatchTgt('playarc\playarc06.ww2'); Flags: ignoreversion deleteafterinstall
  Source: "{#PatchFilesRoot}\playarc07.ww2.patch"; DestDir: "{app}\playarc"; BeforeInstall: BackupPatchTgt('playarc\playarc07.ww2'); Flags: ignoreversion deleteafterinstall
  Source: "{#PatchFilesRoot}\playarc08.ww2.patch"; DestDir: "{app}\playarc"; BeforeInstall: BackupPatchTgt('playarc\playarc08.ww2'); Flags: ignoreversion deleteafterinstall
  Source: "{#PatchFilesRoot}\playarc09.ww2.patch"; DestDir: "{app}\playarc"; BeforeInstall: BackupPatchTgt('playarc\playarc09.ww2'); Flags: ignoreversion deleteafterinstall
  Source: "{#PatchFilesRoot}\playarc10.ww2.patch"; DestDir: "{app}\playarc"; BeforeInstall: BackupPatchTgt('playarc\playarc10.ww2'); Flags: ignoreversion deleteafterinstall

  ; Exe Files.
  Source: "{#PatchFilesRoot}\WW2.exe.patch";       DestDir: "{app}"; BeforeInstall: BackupPatchTgt('WW2.exe');       Flags: ignoreversion deleteafterinstall
  Source: "{#PatchFilesRoot}\Settings.exe.patch";  DestDir: "{app}"; BeforeInstall: BackupPatchTgt('Settings.exe');  Flags: ignoreversion deleteafterinstall
  
  #if UsePlaygate == "yes"
    ; Default
    Source: "{#PatchFilesRoot}\Playgate.exe.patch";  DestDir: "{app}"; BeforeInstall: BackupPatchTgt('Playgate.exe');  Flags: ignoreversion deleteafterinstall
  #endif

  ; Other Files.
  Source: "{#PatchFilesRoot}\license.rtf.patch";   DestDir: "{app}\docs"; BeforeInstall: BackupPatchTgt('docs\license.rtf');   Flags: ignoreversion deleteafterinstall
  Source: "{#PatchFilesRoot}\readme.rtf.patch";    DestDir: "{app}"; BeforeInstall: BackupPatchTgt('readme.rtf');    Flags: ignoreversion deleteafterinstall
  Source: "{#PatchFilesRoot}\readme.txt.patch";    DestDir: "{app}"; BeforeInstall: BackupPatchTgt('readme.txt');    Flags: ignoreversion deleteafterinstall

#else
  ; InstallerBuildType == "installFull"
  ; InstallerBuildType == "installRollback"
  Source: "{#CurrentFilesRoot}\data\*"; DestDir: "{app}\data";Excludes: {#InstallerExcludePatterns}; Flags: ignoreversion recursesubdirs
  Source: "{#CurrentFilesRoot}\docs\*"; DestDir: "{app}\docs";Excludes: {#InstallerExcludePatterns}; Flags: ignoreversion recursesubdirs
  Source: "{#CurrentFilesRoot}\playarc\*"; DestDir: "{app}\playarc";Excludes: {#InstallerExcludePatterns}; Flags: ignoreversion recursesubdirs
  Source: "{#CurrentFilesRoot}\*"; DestDir: "{app}";Excludes: {#InstallerExcludePatterns}; Flags: ignoreversion
 
#endif
       

[Registry]
; DirectX Setup: RUNASADMIN
Root: "HKCU"; Subkey: "SOFTWARE\Microsoft\Windows NT\CurrentVersion\AppCompatFlags\Layers"; ValueType: string; ValueName: "{app}\{#MSDirectXSetup}"; ValueData: "RUNASADMIN"; Flags: createvalueifdoesntexist uninsdeletevalue

; VC Runtimes x64 Setup: RUNASADMIN
; Root: "HKCU"; Subkey: "SOFTWARE\Microsoft\Windows NT\CurrentVersion\AppCompatFlags\Layers"; ValueType: string; ValueName: "{app}\{#MSRuntimeLibraryX64}"; ValueData: "RUNASADMIN"; Check: IsWin64; Flags: createvalueifdoesntexist uninsdeletevalue
Root: "HKCU"; Subkey: "SOFTWARE\Microsoft\Windows NT\CurrentVersion\AppCompatFlags\Layers"; ValueType: string; ValueName: "{app}\{#MSRuntimeLibraryX64}"; ValueData: "RUNASADMIN"; Flags: createvalueifdoesntexist uninsdeletevalue

; VC Runtimes x86 Setup: RUNASADMIN
Root: "HKCU"; Subkey: "SOFTWARE\Microsoft\Windows NT\CurrentVersion\AppCompatFlags\Layers"; ValueType: string; ValueName: "{app}\{#MSRuntimeLibraryX86}"; ValueData: "RUNASADMIN"; Flags: createvalueifdoesntexist uninsdeletevalue

#if InstallerBuildType == "installPatcher"
  ; Patching Tool (xdelta3.exe) = RUNASADMIN
  Root: "HKCU"; Subkey: "SOFTWARE\Microsoft\Windows NT\CurrentVersion\AppCompatFlags\Layers\"; ValueType: String; ValueName: "{app}\{#WWIIOnlinePatchingTool}"; ValueData: "RUNASADMIN"; Flags: createvalueifdoesntexist uninsdeletevalue

#endif

#if UsePlaygate == "yes"
; Default
Root: "HKLM32"; Subkey: "SOFTWARE\Playnet\apps\appid{#WWIIOnlineID}\instance1"; ValueType: string; ValueName: "Exec"; ValueData: "{#WWIIOnlineExe}"; Flags: createvalueifdoesntexist uninsdeletekeyifempty uninsdeletevalue
Root: "HKLM32"; Subkey: "SOFTWARE\Playnet\apps\appid{#WWIIOnlineID}\instance1"; ValueType: string; ValueName: "Params"; ValueData: "{#PlaygateValuePrams}"; Flags: createvalueifdoesntexist uninsdeletekeyifempty uninsdeletevalue
Root: "HKLM32"; Subkey: "SOFTWARE\Playnet\apps\appid{#WWIIOnlineID}\instance1"; ValueType: string; ValueName: "Path"; ValueData: "{app}"; Flags: createvalueifdoesntexist uninsdeletekeyifempty uninsdeletevalue
Root: "HKLM32"; Subkey: "SOFTWARE\Playnet\apps\appid{#WWIIOnlineID}\instance1"; ValueType: string; ValueName: "Region"; ValueData: "{#PlaygateValueRegion}"; Flags: createvalueifdoesntexist uninsdeletekeyifempty uninsdeletevalue
Root: "HKLM32"; Subkey: "SOFTWARE\Playnet\apps\appid{#WWIIOnlineID}\instance1"; ValueType: string; ValueName: "Version"; ValueData: "{#WWIIOnlineDirName}"; Flags: createvalueifdoesntexist uninsdeletekeyifempty uninsdeletevalue

Root: "HKLM"; Subkey: "SOFTWARE\Wow6432Node\Playnet\apps\appid{#WWIIOnlineID}\instance1"; ValueType: string; ValueName: "Exec"; ValueData: "{#WWIIOnlineExe}"; Flags: createvalueifdoesntexist uninsdeletekeyifempty uninsdeletevalue
Root: "HKLM"; Subkey: "SOFTWARE\Wow6432Node\Playnet\apps\appid{#WWIIOnlineID}\instance1"; ValueType: string; ValueName: "Params"; ValueData: "{#PlaygateValuePrams}"; Flags: createvalueifdoesntexist uninsdeletekeyifempty uninsdeletevalue
Root: "HKLM"; Subkey: "SOFTWARE\Wow6432Node\Playnet\apps\appid{#WWIIOnlineID}\instance1"; ValueType: string; ValueName: "Path"; ValueData: "{app}"; Flags: createvalueifdoesntexist uninsdeletekeyifempty uninsdeletevalue
Root: "HKLM"; Subkey: "SOFTWARE\Wow6432Node\Playnet\apps\appid{#WWIIOnlineID}\instance1"; ValueType: string; ValueName: "Region"; ValueData: "{#PlaygateValueRegion}"; Flags: createvalueifdoesntexist uninsdeletekeyifempty uninsdeletevalue
Root: "HKLM"; Subkey: "SOFTWARE\Wow6432Node\Playnet\apps\appid{#WWIIOnlineID}\instance1"; ValueType: string; ValueName: "Version"; ValueData: "{#WWIIOnlineDirName}"; Flags: createvalueifdoesntexist uninsdeletekeyifempty uninsdeletevalue

#endif

[Icons]
Name: "{group}\Help"; Filename: "{#InstallerSupportURL}"; IconFilename: "{app}\{#InstallerIconFileName}"; Comment: "{cm:WWIIolLauncher}"
Name: "{group}\My Account"; Filename: "{#InstallerAccountsURL}"; IconFilename: "{app}\{#InstallerIconFileName}"; Comment: "{cm:WWIIolLauncher}"
Name: "{group}\Official Website"; Filename: "{#InstallerWebsiteURL}"; IconFilename: "{app}\{#InstallerIconFileName}"; Comment: "{cm:WWIIolLauncher}"
Name: "{group}\Online Game Manual"; Filename: "{#InstallerManualURL}"; IconFilename: "{app}\{#InstallerIconFileName}"; Comment: "{cm:WWIIolLauncher}"

#if UsePlaygate == "no"
  Name: "{group}\Play Online"; Filename: "{app}\{#WWIIOnlineExe}"; IconFilename: "{app}\{#InstallerIconFileName}"; Comment: "{cm:WWIIolLauncher}"
  Name: "{commondesktop}\{#WWIIOnlineAppName}{#WWIIOnlineAppVerName}"; Filename: "{app}\{#WWIIOnlineExe}"; IconFilename: "{app}\{#InstallerIconFileName}"; Comment: "{cm:WWIIolLauncher}"; Tasks: desktopicon

#else
; Default
  Name: "{group}\Play Online"; Filename: "{app}\{#WWIIOnlineLauncher}"; IconFilename: "{app}\{#InstallerIconFileName}"; Comment: "{cm:WWIIolLauncher}"
  Name: "{commondesktop}\{#WWIIOnlineAppName}{#WWIIOnlineAppVerName}"; Filename: "{app}\{#WWIIOnlineLauncher}"; IconFilename: "{app}\{#InstallerIconFileName}"; Comment: "{cm:WWIIolLauncher}"; Tasks: desktopicon

#endif

Name: "{group}\Practice Offline"; Filename: "{app}\{#WWIIOnlineExe}"; IconFilename: "{app}\{#InstallerIconFileName}"; Comment: "{cm:WWIIolPractice}"
Name: "{group}\Settings"; Filename: "{app}\{#WWIIOnlineProperties}"; IconFilename: "{app}\{#InstallerIconFileName}"; Comment: "{cm:WWIIolSettings}"
Name: "{group}\Training"; Filename: "{#InstallerTrainingURL}"; IconFilename: "{app}\{#InstallerIconFileName}"; Comment: "{cm:WWIIolTraining}"
Name: "{group}\Support Files\DirectX Redistributable Installer"; Filename: "{app}\{#MSDirectXSetup}"; Comment: "{cm:InstallDX}"
; Name: "{group}\Support Files\(x64) Visual C++ Redistributable"; Filename: "{app}\{#MSRuntimeLibraryX64}"; Comment: "{cm:RunTimeX64}"; Check: IsWin64
Name: "{group}\Support Files\(x64) Visual C++ Redistributable"; Filename: "{app}\{#MSRuntimeLibraryX64}"; Comment: "{cm:RunTimeX64}"
Name: "{group}\Support Files\(x86) Visual C++ Redistributable"; Filename: "{app}\{#MSRuntimeLibraryX86}"; Comment: "{cm:RunTimeX86}"
Name: "{group}\Support Files\Uninstall World War II Online"; Filename: "{uninstallexe}"; IconFilename: "{app}\{#InstallerIconFileName}"; Comment: "{cm:WWIIolUninstall}"


[Run]   
#if InstallerBuildType == "installPatcher"
  ; PlayArc Files.
  Filename: "{app}\{#WWIIOnlinePatchingTool}"; Parameters: "-d -f -q -9 -s ""{app}\playarc\playarc01.ww2.backup"" ""{app}\playarc\playarc01.ww2.patch"" ""{app}\playarc\playarc01.ww2"""; Flags: skipifsilent runhidden; AfterInstall: CleanUp('playarc\playarc01.ww2')
  Filename: "{app}\{#WWIIOnlinePatchingTool}"; Parameters: "-d -f -q -9 -s ""{app}\playarc\playarc02.ww2.backup"" ""{app}\playarc\playarc02.ww2.patch"" ""{app}\playarc\playarc02.ww2"""; Flags: skipifsilent runhidden; AfterInstall: CleanUp('playarc\playarc02.ww2')
  Filename: "{app}\{#WWIIOnlinePatchingTool}"; Parameters: "-d -f -q -9 -s ""{app}\playarc\playarc03.ww2.backup"" ""{app}\playarc\playarc03.ww2.patch"" ""{app}\playarc\playarc03.ww2"""; Flags: skipifsilent runhidden; AfterInstall: CleanUp('playarc\playarc03.ww2')
  Filename: "{app}\{#WWIIOnlinePatchingTool}"; Parameters: "-d -f -q -9 -s ""{app}\playarc\playarc04.ww2.backup"" ""{app}\playarc\playarc04.ww2.patch"" ""{app}\playarc\playarc04.ww2"""; Flags: skipifsilent runhidden; AfterInstall: CleanUp('playarc\playarc04.ww2') 
  Filename: "{app}\{#WWIIOnlinePatchingTool}"; Parameters: "-d -f -q -9 -s ""{app}\playarc\playarc05.ww2.backup"" ""{app}\playarc\playarc05.ww2.patch"" ""{app}\playarc\playarc05.ww2"""; Flags: skipifsilent runhidden; AfterInstall: CleanUp('playarc\playarc05.ww2')
  Filename: "{app}\{#WWIIOnlinePatchingTool}"; Parameters: "-d -f -q -9 -s ""{app}\playarc\playarc06.ww2.backup"" ""{app}\playarc\playarc06.ww2.patch"" ""{app}\playarc\playarc06.ww2"""; Flags: skipifsilent runhidden; AfterInstall: CleanUp('playarc\playarc06.ww2') 
  Filename: "{app}\{#WWIIOnlinePatchingTool}"; Parameters: "-d -f -q -9 -s ""{app}\playarc\playarc07.ww2.backup"" ""{app}\playarc\playarc07.ww2.patch"" ""{app}\playarc\playarc07.ww2"""; Flags: skipifsilent runhidden; AfterInstall: CleanUp('playarc\playarc07.ww2') 
  Filename: "{app}\{#WWIIOnlinePatchingTool}"; Parameters: "-d -f -q -9 -s ""{app}\playarc\playarc08.ww2.backup"" ""{app}\playarc\playarc08.ww2.patch"" ""{app}\playarc\playarc08.ww2"""; Flags: skipifsilent runhidden; AfterInstall: CleanUp('playarc\playarc08.ww2')
  Filename: "{app}\{#WWIIOnlinePatchingTool}"; Parameters: "-d -f -q -9 -s ""{app}\playarc\playarc09.ww2.backup"" ""{app}\playarc\playarc09.ww2.patch"" ""{app}\playarc\playarc09.ww2"""; Flags: skipifsilent runhidden; AfterInstall: CleanUp('playarc\playarc09.ww2')
  Filename: "{app}\{#WWIIOnlinePatchingTool}"; Parameters: "-d -f -q -9 -s ""{app}\playarc\playarc10.ww2.backup"" ""{app}\playarc\playarc10.ww2.patch"" ""{app}\playarc\playarc10.ww2"""; Flags: skipifsilent runhidden; AfterInstall: CleanUp('playarc\playarc10.ww2')

  ; Exe Files.
  Filename: "{app}\{#WWIIOnlinePatchingTool}"; Parameters: "-d -f -q -9 -s ""{app}\WW2.exe.backup""       ""{app}\WW2.exe.patch""       ""{app}\WW2.exe""";       Flags: skipifsilent runhidden; AfterInstall: CleanUp('WW2.exe')
  Filename: "{app}\{#WWIIOnlinePatchingTool}"; Parameters: "-d -f -q -9 -s ""{app}\Settings.exe.backup""  ""{app}\Settings.exe.patch""  ""{app}\Settings.exe""";  Flags: skipifsilent runhidden; AfterInstall: CleanUp('Settings.exe')

  #if UsePlaygate == "yes"
    ; Default
    Filename: "{app}\{#WWIIOnlinePatchingTool}"; Parameters: "-d -f -q -9 -s ""{app}\Playgate.exe.backup""  ""{app}\Playgate.exe.patch""  ""{app}\Playgate.exe""";  Flags: skipifsilent runhidden; AfterInstall: CleanUp('Playgate.exe')
  #endif

  ; Other Files.
  Filename: "{app}\{#WWIIOnlinePatchingTool}"; Parameters: "-d -f -q -9 -s ""{app}\docs\license.rtf.backup""   ""{app}\docs\license.rtf.patch""   ""{app}\docs\license.rtf""";   Flags: skipifsilent runhidden; AfterInstall: CleanUp('docs\license.rtf') 
  Filename: "{app}\{#WWIIOnlinePatchingTool}"; Parameters: "-d -f -q -9 -s ""{app}\readme.rtf.backup""    ""{app}\readme.rtf.patch""    ""{app}\readme.rtf""";    Flags: skipifsilent runhidden; AfterInstall: CleanUp('readme.rtf')
  Filename: "{app}\{#WWIIOnlinePatchingTool}"; Parameters: "-d -f -q -9 -s ""{app}\readme.txt.backup""    ""{app}\readme.txt.patch""    ""{app}\readme.rtf""";    Flags: skipifsilent runhidden; AfterInstall: CleanUp('readme.txt')

  ; Re-launch WWIIOnline after Patching.
  #if UsePlaygate == "yes"
    ; Default
    Filename: "{app}\{#WWIIOnlineLauncher}"; Description: "{cm:LaunchProgram,{#StringChange(WWIIOnlineAppName, '&', '&&')}} when finished everything."; Flags: nowait postinstall skipifsilent runascurrentuser

   #else
    Filename: "{app}\{#WWIIOnlineExe}"; Description: "{cm:LaunchProgram,{#StringChange(WWIIOnlineAppName, '&', '&&')}} when finished everything."; Flags: nowait postinstall skipifsilent runascurrentuser

  #endif
  
 #else
  ; InstallerBuildType == "installFull"
  ; InstallerBuildType == "installRollback"
  Filename: "{app}\{#MSDirectXSetup}"; StatusMsg: "Running DirectX Installer"; Tasks: DirectXRedist; Flags: skipifsilent runascurrentuser
  ; Filename: "{app}\{#MSRuntimeLibraryX64}"; StatusMsg: "Running VC++ Redistributable Installer x64"; Tasks: VCRedistX64; Check: IsWin64; Flags: skipifsilent runascurrentuser
  Filename: "{app}\{#MSRuntimeLibraryX64}"; StatusMsg: "Running VC++ Redistributable Installer x64"; Tasks: VCRedistX64; Flags: skipifsilent runascurrentuser
  Filename: "{app}\{#MSRuntimeLibraryX86}"; StatusMsg: "Running VC++ Redistributable Installer x86"; Tasks: VCRedistX86; Flags: skipifsilent runascurrentuser 
  Filename: "{app}\{#WWIIOnlineProperties}"; Description: "Launch Settings Program after install completes."; Flags: postinstall skipifsilent runascurrentuser

  #if UsePlaygate == "yes"
    ; Default
    Filename: "{app}\{#WWIIOnlineLauncher}"; Description: "{cm:LaunchProgram,{#StringChange(WWIIOnlineAppName, '&', '&&')}} when finished everything."; Flags: nowait postinstall skipifsilent runascurrentuser
  #else
    Filename: "{app}\{#WWIIOnlineExe}"; Description: "{cm:LaunchProgram,{#StringChange(WWIIOnlineAppName, '&', '&&')}} when finished everything."; Flags: nowait postinstall skipifsilent runascurrentuser
  #endif
#endif


[Dirs]
Name: "{pf}\{#InstallerRootPathName}"; Flags: uninsalwaysuninstall

#if ReleaseType == "Alpha" | ReleaseType == "Beta"
  Name: "{pf}\{#InstallerRootPathName}\{#WWIIOnlineDirName}{#WWIIOnlineAppVerName}"; Flags: uninsalwaysuninstall
  Name: "{pf}\{#InstallerRootPathName}\{#WWIIOnlineDirName}{#WWIIOnlineAppVerName}\data"; Flags: uninsalwaysuninstall
  Name: "{pf}\{#InstallerRootPathName}\{#WWIIOnlineDirName}{#WWIIOnlineAppVerName}\docs"; Flags: uninsalwaysuninstall
  Name: "{pf}\{#InstallerRootPathName}\{#WWIIOnlineDirName}{#WWIIOnlineAppVerName}\playarc"; Flags: uninsalwaysuninstall

#else
  ; ReleaseType == "ReleaseCandidate" - Default
  Name: "{pf}\{#InstallerRootPathName}\{#WWIIOnlineDirName}"; Flags: uninsalwaysuninstall
  Name: "{pf}\{#InstallerRootPathName}\{#WWIIOnlineDirName}\data"; Flags: uninsalwaysuninstall
  Name: "{pf}\{#InstallerRootPathName}\{#WWIIOnlineDirName}\docs"; Flags: uninsalwaysuninstall
  Name: "{pf}\{#InstallerRootPathName}\{#WWIIOnlineDirName}\playarc"; Flags: uninsalwaysuninstall

#endif

; My Documents Directory.
Name: "{userdocs}\{#WWIIOnlineDirName}"

; ProgramData Directory.
Name: "{commonappdata}\{#WWIIOnlineDirName}"; Permissions: everyone-full


[UninstallDelete]
Type: dirifempty; Name: "{app}"
Type: dirifempty; Name: "{pf}\{#InstallerRootPathName}"


; ============================================================================
; ============================================================================
; ============================================================================


[Code]
{ Code - START.    ==============================}
type
  ReturnCodes = (
  rcNoError,
  rcNotSupported,
  rcUserAborted,
  rcNotFound,
  rcCopyNoPaths,
  rcCopyWarning,
  rcCopyDirCreateError,
  rcCopyDirListError
  );

var
  ReturnCode: ReturnCodes;

  FinishedInstall: boolean;
  FileCopySucess: boolean;

  MsgText: string;
  MsgYN: integer;

  
{ Utility Functions - START.    =================}
function StrSplit(Text: String; Separator: String): TArrayOfString;
{ Source:https://stackoverflow.com/questions/23836703/how-to-split-a-string-in-inno-setup }

var
  i, p: Integer;
  Dest: TArrayOfString; 

begin
  i := 0;
  repeat
    SetArrayLength( Dest, i + 1 );
    p := Pos( Separator, Text );

    if p > 0 then begin
      Dest[ i ] := Copy( Text, 1, p - 1 );
      Text := Copy( Text, p + Length( Separator ), Length( Text ));
      i := i + 1;

    end else begin
      Dest[ i ] := Text;
      Text := '';

    end;
  until Length( Text )=0;

  Result := Dest;
end;


function getVersionNumber( versionText: string ): string;
var
  versionARAY: TArrayOfString;
  versionNumber: Longint;
  arrayLength: longint;
  zz: longint;

begin
  if Trim( versionText ) = '' then begin
    versionNumber := -1;

  end else begin
    versionARAY := StrSplit( Trim( versionText ), '.' );

    arrayLength := GetArrayLength( versionARAY );

    Log(Format( 'versionText: %s', [ versionText ]));
    Log(Format( 'arrayLength: %d', [ arrayLength ]));

    zz := 0;
    versionNumber := 0;

    repeat
      Log(Format( 'versionARAY[XX]: %s', [ versionARAY[zz] ]));
      
      if ( zz = 0 ) then begin
        versionNumber := versionNumber + StrToIntDef( versionARAY[ zz ], 0 ) * 1000000000;

      end else if( zz = 1 ) then begin
        versionNumber := versionNumber + StrToIntDef( versionARAY[ zz ], 0 ) * 1000000;

      end else if ( zz = 2 ) then begin
        versionNumber := versionNumber + StrToIntDef( versionARAY[ zz ], 0 ) * 1000;

      end else if ( zz = 3 ) then begin
        versionNumber := versionNumber + StrToIntDef( versionARAY[ zz ], 0 );

      end;

      zz := zz + 1;
    until zz >= arrayLength;

  end;

  Result := Trim( IntToStr( versionNumber ));
end;
{ Utility Functions - END.    ===================}


{ CheckPreviousInstall - START.    ==============}
function CheckPreviousInstall(): boolean;
var
  FoundWW2exe: boolean;

  ContinueInstall: boolean;

  CurrentFileName: string;
  CurrentVersion: string;
  InstalledVersion: string;

  WorkingPath: string;
  TempText: string;

begin
  { Pre-install Checks. }
  FoundWW2exe := false;

  CurrentFileName := '';
  CurrentVersion := '';
  InstalledVersion := '';
  WorkingPath := '';
  TempText := '';

  { Looking for WW2.exe in the Installer Target Folder }
  WorkingPath := Trim( ExpandConstant( '{app}\' ))

  Log(Format( 'Installer Target Folder: %s', [ WorkingPath ]));

  if DirExists( WorkingPath ) then begin
    TempText := WorkingPath + '\{#WWIIOnlineExe}'

    if FileOrDirExists( TempText ) then begin
      Log(Format( 'Found: %s', [ TempText ]));
      FoundWW2exe := true;

    end;
  end;

  if ( '{#InstallerBuildType}' = 'installFull' ) then begin

    { Full = Check for existing install. EXISTING INSTALL = ABORT - Warn: Uninstall the current install. }
    if ( FoundWW2exe = true ) then begin
      ContinueInstall := false;

    end else begin
      ContinueInstall := true;    

    end;

    if ContinueInstall = false then begin
      MsgText := CustomMessage('PreviousInstall');
      MsgYN := MsgBox( MsgText, mbCriticalError, mb_ok );

    end;

  end else if ( '{#InstallerBuildType}' = 'installPatcher' ) then begin

    { Patcher = Check for existing install. NO EXISTING INSTALL = ABORT - Warn: Requires the full installer. }
    if ( FoundWW2exe = true ) then begin
      ContinueInstall := true;
    
    end else begin
      ContinueInstall := false;

    end;

    if ( '{#EnablePatching}' = 'no' ) then begin
      ContinueInstall := false;
    
    end;
    
    if ContinueInstall = false then begin
      MsgText := CustomMessage('RequireFullInstall');
      MsgYN := MsgBox( MsgText, mbCriticalError, mb_ok );

    end;
  end;

  Result := ContinueInstall;
end;
{ CheckPreviousInstall - END.    ================}


{ CheckPatchCompatability - START.    ===========}
function CheckPatchCompatability(): boolean;
var
  PatchComptatble: boolean;
  
  InstallVersionNumber: longint;
  PatchFromVersionNumber: longint;
  DiskVersionNumber: longint;
  CurrentFileName: string;
  versionARAY: TArrayOfString;
  tempResult: boolean;
  targetFile: string;

begin
  if ( '{#InstallerBuildType}' <> 'installPatcher' ) then begin
      Log('Not Patcher');
      Result := false;

  end;

  PatchComptatble := true;

  { Patch = Check for patch compatability. INCOMPATABLE = ABORT - Warn: Requires the Full Installer. }
  CurrentFileName := AnsiLowercase( ExtractFileName( ExpandConstant( '{srcexe}' )));
  StringChangeEx(CurrentFileName, '.exe', '', True);
  StringChangeEx(CurrentFileName, '{#InstallerBaseFileName}', '', True);
  Log(Format( 'Current File Name: %s', [ CurrentFileName ]));

  versionARAY := StrSplit( CurrentFileName, '-' );

  Log(Format( 'versionARAY[0]: %s', [ versionARAY[0] ]));
  Log(Format( 'versionARAY[1]: %s', [ versionARAY[1] ]));

  PatchFromVersionNumber := StrToIntDef( getVersionNumber( versionARAY[0]), 0 );
  InstallVersionNumber := StrToIntDef( getVersionNumber( versionARAY[1]), 0 );
  tempResult := GetVersionNumbersString( ExpandConstant( '{app}\{#WWIIOnlineExe}' ), targetFile );
  DiskVersionNumber := StrToIntDef( getVersionNumber( targetFile ), 0 );

  Log(Format( 'Patch From Version Number: %d', [ PatchFromVersionNumber ]));
  Log(Format( 'Disk Version Number: %d', [ DiskVersionNumber ]));

  if PatchFromVersionNumber <> DiskVersionNumber then begin
    MsgText := CustomMessage('IncompatablePatch');
    MsgYN := MsgBox( MsgText, mbCriticalError, mb_ok );

    PatchComptatble := false;

  end else begin;
    PatchComptatble := true;

  end;

  Result := PatchComptatble;
end;
{ CheckPatchCompatability - END.    =============}


{ DirectoryCopy - START.    =====================}
procedure DirectoryCopy(SourcePath, DestPath: string);
{ Source:https://stackoverflow.com/questions/33391915/inno-setup-copy-folder-subfolders-and-files-recursively-in-code-section }
{ Edited MJH. }

var
  ReturnCodeLOC: ReturnCodes;

  FindRec: TFindRec;
  SourceFilePath: string;
  DestFilePath: string;

begin
  FileCopySucess := false;
  
  SourcePath := Trim( SourcePath );
  DestPath := Trim( DestPath );

  SourceFilePath :='';
  DestFilePath := '';
  
  if ( SourcePath = '' ) or ( DestPath = '' ) then begin
    { CopyNoPaths = 4 }
    ReturnCode := rcCopyNoPaths;
    exit;

  end;

  if FindFirst( SourcePath + '\*', FindRec ) then  begin
    try
      repeat
        Log(Format( 'FindRec.Name: %s', [ FindRec.Name ]));

        if ( FindRec.Name <> '.' ) and ( FindRec.Name <> '..' ) then begin
          SourceFilePath := SourcePath + '\' + FindRec.Name;
          DestFilePath := DestPath + '\' + FindRec.Name;
          Log(Format( 'Paths: %s to %s', [ SourceFilePath, DestFilePath ]));

          if FindRec.Attributes and FILE_ATTRIBUTE_DIRECTORY = 0 then begin
            { FindRec.Name is a File.}
            if FileCopy( SourceFilePath, DestFilePath, False ) then begin
              Log(Format( 'File Copied: %s to %s', [ SourceFilePath, DestFilePath ]));
            
            end else begin
              Log(Format( 'Failed to copy File: %s to %s', [ SourceFilePath, DestFilePath ]));
              { CopyWarning = 5 }
              ReturnCodeLOC := rcCopyWarning;
            end;
          
          end else begin
            { FindRec.Name is a Directory. }
            
            if ( DirExists( DestFilePath ) = true ) then begin
              { FindRec.Name directory Exists - Use to Continue. }
              
              Log(Format( 'Directory Exists: %s', [ DestFilePath ]));
            
            end else begin
              { FindRec.Name directory does not Exist - Create to Continue. }
              
              if ( CreateDir( DestFilePath ) = true ) then begin
                Log(Format( 'Directory Created: %s', [ DestFilePath ]));
                DirectoryCopy( SourceFilePath, DestFilePath );
              
              end else begin
                Log( Format('Failed to create Directory: %s', [ DestFilePath ]));
                { CopyDirCreateError = 6 }
                ReturnCodeLOC := rcCopyDirCreateError;
              
              end;
            end;
          end;
        end;

      until not FindNext( FindRec );

    finally
      FindClose( FindRec );
      FileCopySucess := true;

    end;
  
  end else begin
    Log( Format( 'Failed to list Directory: %s', [ SourcePath ]));
    { CopyDirListError = 7 }
    ReturnCodeLOC := rcCopyDirListError;
  
  end;
  
  ReturnCode := ReturnCodeLOC;
end;
{ DirectoryCopy - END.    =======================}


{ FindOldGameSettings - START.    ===============}
procedure FindOldPreferencesFolder();
var
  WorkingPath: string;
  
  BGEDocumentsRootPath: string;
  BGEDocumentsRootFound: boolean;

  RootSourcePath: string;
  RootDestiantionPath: string;
  RootFolderName: string;

  TimeStamp: string;
  TempText: string;

begin

  BGEDocumentsRootPath := '';
  BGEDocumentsRootFound := false;

  { Look for BGE in My Documents Folder }
  WorkingPath := Trim( ExpandConstant( '{userdocs}\{#RootNameBGE}' ))

  if DirExists( WorkingPath ) then begin
    TempText := WorkingPath + '\{#WWIIOnlinePropertiesFile}'

    if FileOrDirExists( TempText ) then begin
      BGEDocumentsRootPath := WorkingPath;
      BGEDocumentsRootFound := true;
      Log('Old Preferences Battleground Europe - Found');

    end;
  end;
  
  if ( BGEDocumentsRootFound = true ) then begin
    { Saved Settings Found - Continue }
    MsgText := CustomMessage('SavedSettingsFound');
    MsgYN := MsgBox( MsgText, mbInformation, mb_YesNo );

    if MsgYN = idNo then begin
      { User selected NO - Exit. UserAborted = 2 }
      ReturnCode := rcUserAborted;
      Exit;

    end;
  
  end else begin
    { Saved Settings NOT Found - NotFound = 3}
    Log('Old Preferences Not Found');
    ReturnCode := rcNotFound;
    Exit;

  end;

  { Set Source }
  { Only Looking for BGE Saved Settings - Use BGE Documents Root }
  RootSourcepath := BGEDocumentsRootPath;
  RootFolderName := '{#RootNameBGE}';

  { Set Destination } 
  {revised final}
  { ... \ User \ My Documents \ WWIIOnline - Folder }
  RootDestiantionPath := Trim( ExpandConstant( '{userdocs}\{#RootNameWWIIOnline}' ));

  { Copy Files }
  Log(Format( 'Old Files Copy %s to %s', [ RootSourcepath, RootDestiantionPath ]));
  DirectoryCopy( RootSourcepath, RootDestiantionPath );

  if ( FileCopySucess = true ) then begin
    if SetCurrentDir( Trim( ExpandConstant( '{userdocs}' ))) then begin
      TimeStamp := GetDateTimeString('yyyymmdd-hhnnss', ' ', ' ');
      if  RenameFile( RootFolderName, RootFolderName + '_Copied_' + TimeStamp ) then begin
        MsgText := FmtMessage( CustomMessage('RenamedDIR'), [ RootFolderName, RootFolderName, TimeStamp ]);
        MsgYN := MsgBox( MsgText, mbInformation, mb_ok );

      end;
    end;
  end;
end;
{ FindOldGameSettings - END.    =================}


{ CurStepChanged - START.    ====================}
procedure CurStepChanged(CurStep: TSetupStep);
begin
  if ( CurStep = ssInstall ) then begin
    { Pre-install Checks. }
    Log('Pre-install Checks');

    if CheckPreviousInstall = false then begin
      Abort;

    end;

    if ( '{#InstallerBuildType}' = 'installPatcher' )then begin
      if CheckPatchCompatability = false then begin
        Abort;
       
       end;
    end;

  end else if ( CurStep = ssPostInstall ) then begin
    Log('Find Old preferences Folder begin');
    FindOldPreferencesFolder;

    FinishedInstall := True;

  end;
end;
{ CurStepChanged - END.    ======================}


{ Custom Controls - START.    ===================}
procedure AboutButtonOnClick(Sender: TObject);
begin
  MsgText := CustomMessage('AboutButtonText');
  MsgBox( MsgText, mbInformation, mb_Ok );

end;


procedure URLLabelOnClick(Sender: TObject);
var
  ErrorCode: Integer;

begin
  ShellExecAsOriginalUser('open', '{#InstallerWebsiteURL}/', '', '', SW_SHOWNORMAL, ewNoWait, ErrorCode);

end;


procedure CreateAboutButtonAndURLLabel(ParentForm: TSetupForm; CancelButton: TNewButton);
var
  AboutButton: TNewButton;
  URLLabel: TNewStaticText;

begin
  AboutButton := TNewButton.Create(ParentForm);
  AboutButton.Left := ParentForm.ClientWidth - CancelButton.Left - CancelButton.Width;
  AboutButton.Top := CancelButton.Top;
  AboutButton.Width := CancelButton.Width;
  AboutButton.Height := CancelButton.Height;
  AboutButton.Caption := '&About...';
  AboutButton.OnClick := @AboutButtonOnClick;
  AboutButton.Parent := ParentForm;

  URLLabel := TNewStaticText.Create(ParentForm);
  URLLabel.Caption := 'www.wwiionline.com';
  URLLabel.Cursor := crHand;
  URLLabel.OnClick := @URLLabelOnClick;
  URLLabel.Parent := ParentForm;
  { Alter Font *after* setting Parent so the correct defaults are inherited first }
  URLLabel.Font.Style := URLLabel.Font.Style + [fsUnderline];

  if GetWindowsVersion >= $040A0000 then begin   { Windows 98 or later? }
    URLLabel.Font.Color := clHotLight

  end else begin
    URLLabel.Font.Color := clBlue;

  end;

  URLLabel.Top := AboutButton.Top + AboutButton.Height - URLLabel.Height - 2;
  URLLabel.Left := AboutButton.Left + AboutButton.Width + ScaleX(20);

end;

procedure InitializeWizard();
var
  BackgroundBitmapImage: TBitmapImage;
  BackgroundBitmapText: TNewStaticText;

begin
  { Custom controls }
  CreateAboutButtonAndURLLabel(WizardForm, WizardForm.CancelButton);

  { Custom beveled label }
  WizardForm.BeveledLabel.Caption := '-  {#WWIIOnlineInstallVersion}{#WWIIOnlineInstallVersionType}   -   Playnet, Inc  &  Cornered Rat Software, (LLC).  ';

end;


procedure InitializeUninstallProgressForm();
begin
  { Custom controls }
  // CreateAboutButtonAndURLLabel(UninstallProgressForm, UninstallProgressForm.CancelButton);

end;
{ Custom Controls - END.    =====================}


{ Patching Functions - START.    ================}
procedure CleanUp(target: String);
begin                            
  DeleteFile( ExpandConstant('{app}')+'\'+target+'.backup');

end;


procedure BackupPatchTgt(target: String);
var 
  tgt : String;
  newname : String;

begin
  // break 'em out for debugging
  tgt :=  ExpandConstant('{app}') + '\' + target;   
  newname := ExpandConstant('{app}') + '\' + target + '.backup';    

  // rename file
  if not RenameFile( tgt, newname ) then begin
  MsgBox('File renaming '+ Target +' to '+ Target +'.backup failed!', mbError, MB_OK);

  end;
end;


function GetFileAttributes(lpFileName: PAnsiChar): DWORD;
  external 'GetFileAttributesA@kernel32.dll stdcall';


function SetFileAttributes(lpFileName: PAnsiChar; dwFileAttributes: DWORD): BOOL; 
  external 'SetFileAttributesA@kernel32.dll stdcall';


function RemoveReadOnlyWithEcho(FileName : String): String;
var
  Attr : DWord;

begin
  Attr := GetFileAttributes(FileName);

  if (Attr and 1) = 1  then begin
    Attr := Attr -1;
    SetFileAttributes(FileName,Attr);

  end;

  Result := FileName;

end;
{ Patching Functions - END.    ==================}


{ Code - END.    ================================}
{ EOF ========================================================================}
