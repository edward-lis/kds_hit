#define Date GetDateTimeString('ddddd', '', '');
#define MyApp "kds_hit"
#define MyAppName "КДС ХИТ"
#define MyAppVersion "1.0"

[Setup]
AppId={{BE6B7C51-4B27-460E-80D0-A914229EA9B5}
AppName={#MyAppName}
AppVerName={#MyAppName}
DefaultDirName={pf}/{#MyAppName}
DefaultGroupName={#MyAppName}
;DisableProgramGroupPage=yes
OutputDir=./
OutputBaseFilename={#MyApp}_setup
Compression=lzma
SolidCompression=true
UninstallDisplayIcon={app}\kds_hit.exe

[Languages]
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"

[Messages]
BeveledLabel=(сборка {#Date})

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: checkedonce
Name: "quicklaunchicon"; Description: "{cm:CreateQuickLaunchIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "..\build-kds_hit-Desktop_Qt_5_5_1_MinGW_32bit_Static-Release\release\kds_hit.exe"; DestDir: "{app}"; Flags: ignoreversion;
Source: ".\kds_hit.ini"; DestDir: "{app}"; Flags: ignoreversion;

[Icons]
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"; Flags: createonlyiffileexists; IconFilename: "{app}\kds_hit.exe"; IconIndex: 0; Comment: "Деинсталлировать {#MyAppName}"
Name: "{group}\КДС ХИТ"; Filename: "{app}\kds_hit.exe"; IconFilename: "{app}\kds_hit.exe"; IconIndex: 0; Comment: "{#MyAppName}";
Name: "{userdesktop}\КДС ХИТ"; Filename: "{app}\kds_hit.exe"; IconFilename: "{app}\kds_hit.exe"; IconIndex: 0; Comment: "{#MyAppName}"; Tasks: desktopicon
Name: "{userappdata}\Microsoft\Internet Explorer\Quick Launch\КДС ХИТ"; Filename: "{app}\kds_hit.exe"; IconFilename: "{app}\kds_hit.exe"; IconIndex: 0; Comment: "{#MyAppName}"; Tasks: quicklaunchicon

[Run]
Filename: "{app}\kds_hit.exe"; Flags: shellexec runasoriginaluser postinstall; Description: "Запустить КДС ХИТ";

[UninstallRun]
Filename: "cmd.exe"; Parameters: "/c taskkill /f /im kds_hit.exe";

[UninstallDelete]
Type: filesandordirs; Name: "{app}"

[Messages]
SelectDirBrowseLabel=Для продолжения нажмите кнопку далее. Если нужно выбрать другой диск, выберите его из списка.
