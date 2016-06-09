#define MyAppID "12"
#define MyAppVersion GetFileVersion("current/WW2.exe")

#define MyAppNameRoot "Battleground Europe"
#define MyAppName "Battleground Europe - 1.35 Beta"
#define MyAppPublisher "Playnet Inc."
#define MyAppURL "http://www.battlegroundeurope.net/"
#define MyAppExeName "Playgate.exe"
#define MyFullInstallSourceDir "current"
#define MyOutputDir "winrelease"
#define MyAssetsDir "assets"
#define WW2ModsDir "ww2mods"

[Setup]
SignTool=signtool /fd sha256 /d $qBattleground Europe$q /du $qhttp://www.battlegroundeurope.com$q $f

; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)

#if MyAppID == "1"
  AppId={{BCA70BBE-EACC-49F4-AC9A-1DFE4E55F739}
#elif MyAppID == "12"
  AppId={{1AE85CC9-7D09-475A-ACBB-AAD74D035701}
#else
  #error Unknown AppID.
#endif
OutputBaseFilename=wwiiol_{#MyAppVersion}

AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={pf}\CRS\{#MyAppName}
DefaultGroupName=Cornered Rat Software\{#MyAppName}     
SourceDir={#SourcePath}
Compression=lzma
SolidCompression=yes
PrivilegesRequired=admin
AppendDefaultDirName=False
UsePreviousAppDir=False
AppCopyright=Copyright 1999 - 2016, Playnet, Inc.
AllowRootDirectory=True         
SetupIconFile={#MyAssetsDir}\wwii1.ico
LicenseFile={#MyAssetsDir}\license.rtf
InfoBeforeFile={#MyAssetsDir}\readme.rtf
WizardImageFile={#MyAssetsDir}\WIZMODERNIMAGE.BMP
WizardSmallImageFile={#MyAssetsDir}\WIZMODERNSMALLIMAGE.BMP
OutputDir={#SourcePath}\release\{#MyOutputDir}
ShowUndisplayableLanguages=true
                               
[Messages]
WelcomeLabel2=This will install [name/ver] on your computer.%n%nIt is recommended that you close all other applications and disable any anti-virus software before continuing.%n%nWARNING: This program is protected by copyright law and international treaties.%n%nUnauthorized reproduction or distribution of this program, or any portion of it, may result in severe civil and criminal penalties, and will be prosecuted to the maximum extent possible under the law.

[InstallDelete]
Type: filesandordirs; Name: "{app}"
Type: filesandordirs; Name: "{userprograms}\Cornered Rat Software\{#MyAppName}"
Type: filesandordirs; Name: "{commonprograms}\Cornered Rat Software\{#MyAppName}"

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "french"; MessagesFile: "compiler:Languages\French.isl"
Name: "german"; MessagesFile: "compiler:Languages\German.isl"
Name: "italian"; MessagesFile: "compiler:Languages\Italian.isl"
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "quicklaunchicon"; Description: "{cm:CreateQuickLaunchIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked; OnlyBelowVersion: 0,6.1
Name: "DirectXRedist"; Description: "Install DirectX Redistributable Files"; GroupDescription: "External Installers:";


[Files]                                                                                                                 
Source: "{#MyAssetsDir}\license.rtf"; DestDir: "{app}"; Flags: ignoreversion              
Source: "{#MyAssetsDir}\readme.rtf"; DestDir: "{app}"; Flags: ignoreversion        
Source: "{#MyFullInstallSourceDir}\*"; DestDir: "{app}";Excludes: ".gitignore"; Flags: ignoreversion recursesubdirs                       
       
[Registry]  
; this registry configuration requires building the installer on a 64-bit windows machine.
; if you want to build on a 32-bit machine (and have the ww2 app work) then you need to create a HKLM64 section as well.
;                               
Root: "HKCU"; Subkey: "SOFTWARE\Microsoft\Windows NT\CurrentVersion\AppCompatFlags\Layers"; ValueType: string; ValueName: "{app}\{#MyAppExeName}"; Flags: uninsdeletekey
Root: "HKCU"; Subkey: "SOFTWARE\Microsoft\Windows NT\CurrentVersion\AppCompatFlags\Layers"; ValueType: string; ValueName: "{app}\ww2.exe";         Flags: uninsdeletekey
Root: "HKCU"; Subkey: "SOFTWARE\Microsoft\Windows NT\CurrentVersion\AppCompatFlags\Layers"; ValueType: string; ValueName: "{app}\settings.exe";    Flags: uninsdeletekey
Root: "HKCU"; Subkey: "SOFTWARE\Microsoft\Windows NT\CurrentVersion\AppCompatFlags\Layers"; ValueType: string; ValueName: "{app}\DATA\DirectXRedist\DXSetup.exe"; ValueData: "RUNASADMIN"; Flags: uninsdeletekey

; 32-bit only registry keys
Root: "HKLM32"; Subkey: "SOFTWARE\Playnet\apps\appid{#MyAppID}\instance1"; ValueType: string; ValueName: "Exec";    ValueData: "ww2.exe";          Flags: uninsdeletekey
Root: "HKLM32"; Subkey: "SOFTWARE\Playnet\apps\appid{#MyAppID}\instance1"; ValueType: string; ValueName: "Params";  ValueData: "-serverip $QUAD -serverport $PORT -serverotp $OTP -side $SESSIONID"; Flags: uninsdeletekey
Root: "HKLM32"; Subkey: "SOFTWARE\Playnet\apps\appid{#MyAppID}\instance1"; ValueType: string; ValueName: "Path";    ValueData: "{app}";            Flags: uninsdeletekey
Root: "HKLM32"; Subkey: "SOFTWARE\Playnet\apps\appid{#MyAppID}\instance1"; ValueType: string; ValueName: "Region";  ValueData: "us";               Flags: uninsdeletekey
Root: "HKLM32"; Subkey: "SOFTWARE\Playnet\apps\appid{#MyAppID}\instance1"; ValueType: string; ValueName: "Version"; ValueData: "{#MyAppNameRoot}"; Flags: uninsdeletekey

; 32-bit/64-bit registry keys (WOW6432Node subkey)
Root: "HKLM"; Subkey: "SOFTWARE\Wow6432Node\Playnet\apps\appid{#MyAppID}\instance1"; ValueType: string; ValueName: "Exec";    ValueData: "ww2.exe";          Flags: uninsdeletekey
Root: "HKLM"; Subkey: "SOFTWARE\Wow6432Node\Playnet\apps\appid{#MyAppID}\instance1"; ValueType: string; ValueName: "Params";  ValueData: "-serverip $QUAD -serverport $PORT -serverotp $OTP -side $SESSIONID"; Flags: uninsdeletekey
Root: "HKLM"; Subkey: "SOFTWARE\Wow6432Node\Playnet\apps\appid{#MyAppID}\instance1"; ValueType: string; ValueName: "Path";    ValueData: "{app}";            Flags: uninsdeletekey 
Root: "HKLM"; Subkey: "SOFTWARE\Wow6432Node\Playnet\apps\appid{#MyAppID}\instance1"; ValueType: string; ValueName: "Region";  ValueData: "us";               Flags: uninsdeletekey 
Root: "HKLM"; Subkey: "SOFTWARE\Wow6432Node\Playnet\apps\appid{#MyAppID}\instance1"; ValueType: string; ValueName: "Version"; ValueData: "{#MyAppNameRoot}"; Flags: uninsdeletekey

                                      
Root: "HKLM";   Subkey: "SOFTWARE\Wow6432Node\Microsoft\Windows\CurrentVersion\Uninstall\{#MyAppName}"; ValueType: none; Flags: deletekey
Root: "HKLM32"; Subkey: "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\{#MyAppName}";             ValueType: none; Flags: deletekey

[Icons]
Name: "{group}\Help";               Filename: "http://support.playnet.com/"; IconFilename: "{app}\wwii1.ico"
Name: "{group}\My Account";         Filename: "http://www.battlegroundeurope.com/account"; IconFilename: "{app}\wwii1.ico"
Name: "{group}\Official Website";   Filename: "http://www.battlegroundeurope.net/home"; IconFilename: "{app}\wwii1.ico"
Name: "{group}\Online Game Manual"; Filename: "http://wiki.wwiionline.com/"; IconFilename: "{app}\wwii1.ico"
Name: "{group}\Play Online";        Filename: "{app}\{#MyAppExeName}"; IconFilename: "{app}\wwii1.ico"
Name: "{group}\Practice Offline";   Filename: "{app}\ww2.exe"; IconFilename: "{app}\wwii1.ico"
Name: "{group}\Settings";           Filename: "{app}\settings.exe"; IconFilename: "{app}\wwii1.ico"
Name: "{group}\Training";           Filename: "http://wiki.wwiionline.com/index.php/Training"; IconFilename: "{app}\wwii1.ico"
Name: "{group}\Use DirectX Redistributable Installer"; Filename: "{app}\DATA\DirectXRedist\DXSetup.exe"

;Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{commondesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon; IconFilename: "{app}\wwii1.ico"
;Name: "{userappdata}\Microsoft\Internet Explorer\Quick Launch\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: quicklaunchicon; IconFilename: "{app}\wwii1.ico"

[Run]   
Filename: "{app}\DATA\DirectXRedist\DXSetup.exe"; StatusMsg: "Running DirectX Installer"; Tasks: DirectXRedist; Flags: skipifsilent runascurrentuser 
Filename: "{app}\settings.exe";    Description: "Launch Settings Program after install completes."; Flags: postinstall skipifsilent runascurrentuser
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}} when finished everything."; Flags: nowait postinstall skipifsilent runascurrentuser
