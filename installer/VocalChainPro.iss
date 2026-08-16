; Script Inno Setup pour Vocal Chain Pro
; Installe automatiquement le plugin VST3 au bon endroit sur Windows

#define MyAppName "Vocal Chain Pro"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "MyAudioStudio"

[Setup]
AppId={{B1E4F2A0-VOCL-CHAI-NPRO-000000000001}}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={commoncf}\VST3
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
OutputDir=Output
OutputBaseFilename=VocalChainPro_Installer
Compression=lzma
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64
PrivilegesRequired=admin

[Languages]
Name: "french"; MessagesFile: "compiler:Languages\French.isl"

[Files]
; Le dossier .vst3 complet est copié tel quel (structure de bundle VST3)
Source: "..\build\VocalChainPro_artefacts\Release\VST3\Vocal Chain Pro.vst3\*"; DestDir: "{commoncf}\VST3\Vocal Chain Pro.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\Désinstaller {#MyAppName}"; Filename: "{uninstallexe}"

[Messages]
french.WelcomeLabel2=Ceci va installer [name/ver] sur votre ordinateur, dans le dossier VST3 standard afin qu'il apparaisse automatiquement dans votre DAW (Studio One, Ableton, FL Studio, etc.).
