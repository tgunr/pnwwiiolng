#define MyAppID "1"
#define MyAppVersion GetFileVersion(curr+"/WW2.exe")
#define MyOldAppVersion GetFileVersion(prev+"/WW2.exe")

#define MyAppNameRoot "Battleground Europe"
#define MyAppName "Battleground Europe"
#define MyAppPublisher "Playnet Inc."
#define MyAppURL "http://www.battlegroundeurope.net/"
#define MyAppExeName "Playgate.exe"
#define MyPatchSourceDir "diff"
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
OutputBaseFilename=wwiiol_{#MyOldAppVersion}-{#MyAppVersion}

AppName={#MyAppName}
AppVersion={#MyAppVersion}
;AppVerName={#MyAppName} {#MyAppVersion}
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
UsePreviousAppDir=yes
AppCopyright=Copyright 1999 - 2017, Playnet, Inc.
AllowRootDirectory=True         
SetupIconFile={#MyAssetsDir}\wwii1.ico
LicenseFile={#MyAssetsDir}\license.rtf
InfoBeforeFile={#MyAssetsDir}\readme.rtf
WizardImageFile={#MyAssetsDir}\WIZMODERNIMAGE.BMP
WizardSmallImageFile={#MyAssetsDir}\WIZMODERNSMALLIMAGE.BMP
OutputDir={#SourcePath}\release\{#MyOutputDir}
CreateUninstallRegKey=no
UpdateUninstallLogAppName=no

[Registry]
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Windows NT\CurrentVersion\AppCompatFlags\Layers\";ValueType: String; ValueName: "{app}\xdelta3.exe"; ValueData: "RUNASADMIN";Flags: uninsdeletekeyifempty uninsdeletevalue;

[Messages]
english.WelcomeLabel2=This will install the [name/ver] patch on your computer.%n%nIt is recommended that you close all other applications and disable any anti-virus software before continuing.%n%nWARNING: This program is protected by copyright law and international treaties.%n%nUnauthorized reproduction or distribution of this program, or any portion of it, may result in severe civil and criminal penalties, and will be prosecuted to the maximum extent possible under the law.
english.WelcomeLabel1=Welcome to the [name] patcher.
english.SetupWindowTitle=%1 Patch Installer
english.ReadyLabel1=Setup is now ready to begin installing the [name] patch on your computer.
english.InstallingLabel=Please wait while Setup installs the [name] patch on your computer.
english.SelectDirDesc=Where should the [name] patch be installed?
english.WizardSelectDir=Select Patch Destination Folder
english.SelectDirLabel3=Setup will install the patch into the following folder.

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "brazilianportuguese"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"
Name: "catalan"; MessagesFile: "compiler:Languages\Catalan.isl"
Name: "corsican"; MessagesFile: "compiler:Languages\Corsican.isl"
Name: "czech"; MessagesFile: "compiler:Languages\Czech.isl"
Name: "danish"; MessagesFile: "compiler:Languages\Danish.isl"
Name: "dutch"; MessagesFile: "compiler:Languages\Dutch.isl"
Name: "finnish"; MessagesFile: "compiler:Languages\Finnish.isl"
Name: "french"; MessagesFile: "compiler:Languages\French.isl"
Name: "german"; MessagesFile: "compiler:Languages\German.isl"
Name: "greek"; MessagesFile: "compiler:Languages\Greek.isl"
Name: "hebrew"; MessagesFile: "compiler:Languages\Hebrew.isl"
Name: "hungarian"; MessagesFile: "compiler:Languages\Hungarian.isl"
Name: "italian"; MessagesFile: "compiler:Languages\Italian.isl"
Name: "japanese"; MessagesFile: "compiler:Languages\Japanese.isl"
Name: "norwegian"; MessagesFile: "compiler:Languages\Norwegian.isl"
Name: "polish"; MessagesFile: "compiler:Languages\Polish.isl"
Name: "portuguese"; MessagesFile: "compiler:Languages\Portuguese.isl"
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"
Name: "serbiancyrillic"; MessagesFile: "compiler:Languages\SerbianCyrillic.isl"
Name: "serbianlatin"; MessagesFile: "compiler:Languages\SerbianLatin.isl"
Name: "slovenian"; MessagesFile: "compiler:Languages\Slovenian.isl"
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"
Name: "ukrainian"; MessagesFile: "compiler:Languages\Ukrainian.isl"

[Dirs]
Name: "{app}"; Permissions: everyone-full

[Dirs]
Name: "{app}"; Permissions: everyone-full

[Files]                                                                                                                 
Source: "{#MyAssetsDir}\xdelta3.exe"; DestDir: "{app}"; Flags: ignoreversion deleteafterinstall     
Source: "{#MyPatchSourceDir}\playarc01.ww2.patch"; DestDir: "{app}"; BeforeInstall: BackupPatchTgt('playarc01.ww2'); Flags: ignoreversion deleteafterinstall  
Source: "{#MyPatchSourceDir}\playarc02.ww2.patch"; DestDir: "{app}"; BeforeInstall: BackupPatchTgt('playarc02.ww2'); Flags: ignoreversion deleteafterinstall    
Source: "{#MyPatchSourceDir}\playarc03.ww2.patch"; DestDir: "{app}"; BeforeInstall: BackupPatchTgt('playarc03.ww2'); Flags: ignoreversion deleteafterinstall    
Source: "{#MyPatchSourceDir}\playarc04.ww2.patch"; DestDir: "{app}"; BeforeInstall: BackupPatchTgt('playarc04.ww2'); Flags: ignoreversion deleteafterinstall    
Source: "{#MyPatchSourceDir}\playarc05.ww2.patch"; DestDir: "{app}"; BeforeInstall: BackupPatchTgt('playarc05.ww2'); Flags: ignoreversion deleteafterinstall    
Source: "{#MyPatchSourceDir}\playarc06.ww2.patch"; DestDir: "{app}"; BeforeInstall: BackupPatchTgt('playarc06.ww2'); Flags: ignoreversion deleteafterinstall    
Source: "{#MyPatchSourceDir}\playarc07.ww2.patch"; DestDir: "{app}"; BeforeInstall: BackupPatchTgt('playarc07.ww2'); Flags: ignoreversion deleteafterinstall  
Source: "{#MyPatchSourceDir}\playarc08.ww2.patch"; DestDir: "{app}"; BeforeInstall: BackupPatchTgt('playarc08.ww2'); Flags: ignoreversion deleteafterinstall  
Source: "{#MyPatchSourceDir}\playarc09.ww2.patch"; DestDir: "{app}"; BeforeInstall: BackupPatchTgt('playarc09.ww2'); Flags: ignoreversion deleteafterinstall  
Source: "{#MyPatchSourceDir}\playarc10.ww2.patch"; DestDir: "{app}"; BeforeInstall: BackupPatchTgt('playarc10.ww2'); Flags: ignoreversion deleteafterinstall  
Source: "{#MyPatchSourceDir}\Playgate.exe.patch";  DestDir: "{app}"; BeforeInstall: BackupPatchTgt('Playgate.exe');  Flags: ignoreversion deleteafterinstall  
Source: "{#MyPatchSourceDir}\Settings.exe.patch";  DestDir: "{app}"; BeforeInstall: BackupPatchTgt('Settings.exe');  Flags: ignoreversion deleteafterinstall  
Source: "{#MyPatchSourceDir}\WW2.exe.patch";       DestDir: "{app}"; BeforeInstall: BackupPatchTgt('WW2.exe');       Flags: ignoreversion deleteafterinstall  
Source: "{#MyPatchSourceDir}\license.rtf.patch";   DestDir: "{app}"; BeforeInstall: BackupPatchTgt('license.rtf');   Flags: ignoreversion deleteafterinstall  
Source: "{#MyPatchSourceDir}\readme.rtf.patch";    DestDir: "{app}"; BeforeInstall: BackupPatchTgt('readme.rtf');    Flags: ignoreversion deleteafterinstall  
Source: "{#MyPatchSourceDir}\tos-bg.bmp.patch";    DestDir: "{app}"; BeforeInstall: BackupPatchTgt('DATA\playgate\tos-bg.bmp');    Flags: ignoreversion deleteafterinstall  
Source: "current\steam_api.dll"; DestDir: "{app}"; Flags: ignoreversion              
Source: "current\steam_appid.txt"; DestDir: "{app}"; Flags: ignoreversion        

[Run]                          
Filename: "{app}\Xdelta3.exe"; Parameters: "-d -f -q -9 -s ""{app}\playarc01.ww2.backup"" ""{app}\playarc01.ww2.patch"" ""{app}\playarc01.ww2"""; Flags: skipifsilent runhidden; AfterInstall: CleanUp('playarc01.ww2')
Filename: "{app}\Xdelta3.exe"; Parameters: "-d -f -q -9 -s ""{app}\playarc02.ww2.backup"" ""{app}\playarc02.ww2.patch"" ""{app}\playarc02.ww2"""; Flags: skipifsilent runhidden; AfterInstall: CleanUp('playarc02.ww2')
Filename: "{app}\Xdelta3.exe"; Parameters: "-d -f -q -9 -s ""{app}\playarc03.ww2.backup"" ""{app}\playarc03.ww2.patch"" ""{app}\playarc03.ww2"""; Flags: skipifsilent runhidden; AfterInstall: CleanUp('playarc03.ww2')
Filename: "{app}\Xdelta3.exe"; Parameters: "-d -f -q -9 -s ""{app}\playarc04.ww2.backup"" ""{app}\playarc04.ww2.patch"" ""{app}\playarc04.ww2"""; Flags: skipifsilent runhidden; AfterInstall: CleanUp('playarc04.ww2') 
Filename: "{app}\Xdelta3.exe"; Parameters: "-d -f -q -9 -s ""{app}\playarc05.ww2.backup"" ""{app}\playarc05.ww2.patch"" ""{app}\playarc05.ww2"""; Flags: skipifsilent runhidden; AfterInstall: CleanUp('playarc05.ww2')
Filename: "{app}\Xdelta3.exe"; Parameters: "-d -f -q -9 -s ""{app}\playarc06.ww2.backup"" ""{app}\playarc06.ww2.patch"" ""{app}\playarc06.ww2"""; Flags: skipifsilent runhidden; AfterInstall: CleanUp('playarc06.ww2') 
Filename: "{app}\Xdelta3.exe"; Parameters: "-d -f -q -9 -s ""{app}\playarc07.ww2.backup"" ""{app}\playarc07.ww2.patch"" ""{app}\playarc07.ww2"""; Flags: skipifsilent runhidden; AfterInstall: CleanUp('playarc07.ww2') 
Filename: "{app}\Xdelta3.exe"; Parameters: "-d -f -q -9 -s ""{app}\playarc08.ww2.backup"" ""{app}\playarc08.ww2.patch"" ""{app}\playarc08.ww2"""; Flags: skipifsilent runhidden; AfterInstall: CleanUp('playarc08.ww2')
Filename: "{app}\Xdelta3.exe"; Parameters: "-d -f -q -9 -s ""{app}\playarc09.ww2.backup"" ""{app}\playarc09.ww2.patch"" ""{app}\playarc09.ww2"""; Flags: skipifsilent runhidden; AfterInstall: CleanUp('playarc09.ww2')
Filename: "{app}\Xdelta3.exe"; Parameters: "-d -f -q -9 -s ""{app}\playarc10.ww2.backup"" ""{app}\playarc10.ww2.patch"" ""{app}\playarc10.ww2"""; Flags: skipifsilent runhidden; AfterInstall: CleanUp('playarc10.ww2')
Filename: "{app}\Xdelta3.exe"; Parameters: "-d -f -q -9 -s ""{app}\Playgate.exe.backup""  ""{app}\Playgate.exe.patch""  ""{app}\Playgate.exe""";  Flags: skipifsilent runhidden; AfterInstall: CleanUp('Playgate.exe')
Filename: "{app}\Xdelta3.exe"; Parameters: "-d -f -q -9 -s ""{app}\Settings.exe.backup""  ""{app}\Settings.exe.patch""  ""{app}\Settings.exe""";  Flags: skipifsilent runhidden; AfterInstall: CleanUp('Settings.exe')
Filename: "{app}\Xdelta3.exe"; Parameters: "-d -f -q -9 -s ""{app}\WW2.exe.backup""       ""{app}\WW2.exe.patch""       ""{app}\WW2.exe""";       Flags: skipifsilent runhidden; AfterInstall: CleanUp('WW2.exe')
Filename: "{app}\Xdelta3.exe"; Parameters: "-d -f -q -9 -s ""{app}\license.rtf.backup""   ""{app}\license.rtf.patch""   ""{app}\license.rtf""";   Flags: skipifsilent runhidden; AfterInstall: CleanUp('license.rtf') 
Filename: "{app}\Xdelta3.exe"; Parameters: "-d -f -q -9 -s ""{app}\readme.rtf.backup""    ""{app}\readme.rtf.patch""    ""{app}\readme.rtf""";    Flags: skipifsilent runhidden; AfterInstall: CleanUp('readme.rtf')
Filename: "{app}\Xdelta3.exe"; Parameters: "-d -f -q -9 -s ""{app}\DATA\playgate\tos-bg.bmp.backup""    ""{app}\tos-bg.bmp.patch""    ""{app}\DATA\playgate\tos-bg.bmp""";    Flags: skipifsilent runhidden; AfterInstall: CleanUp('DATA\playgate\tos-bg.bmp')

[CODE]
procedure CleanUp(target: String);
begin                            
; DeleteFile( ExpandConstant('{app}')+'\'+target+'.backup');
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
  if not RenameFile( tgt, newname ) then
    MsgBox('File renaming '+ Target +' to '+ Target +'.backup failed!', mbError, MB_OK);
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
  if (Attr and 1) = 1  then          
  begin
    Attr := Attr -1;
    SetFileAttributes(FileName,Attr);
  end;
  Result := FileName;
end;
